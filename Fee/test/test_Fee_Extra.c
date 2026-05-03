#include "unity.h"
#include "Fee.h"
#include "Fee_Internal.h"
#include "Fee_Sector.h"
#include "Fee_Cfg.h"
#include "Det.h"
#include "MemAcc.h"
#include <string.h>

/* --------------- NvM callback stubs --------------- */
static boolean NvM_JobEndCalled = FALSE;
static boolean NvM_JobErrorCalled = FALSE;

void NvM_JobEndNotification(void)
{
    NvM_JobEndCalled = TRUE;
}

void NvM_JobErrorNotification(void)
{
    NvM_JobErrorCalled = TRUE;
}

/* --------------- setUp / tearDown --------------- */
void setUp(void)
{
    NvM_JobEndCalled = FALSE;
    NvM_JobErrorCalled = FALSE;
    Det_ClearLastError();
}

void tearDown(void)
{
}

/* --------------- Helper: full reset and init --------------- */
static void FullResetAndInit(void)
{
    MemAcc_TestResetFlash();
    Fee_Init(&Fee_Config);
}

/* --------------- Helper: process until idle --------------- */
static void ProcessUntilIdle(void)
{
    uint16 guard = 0u;
    while ((Fee_GetStatus() == MEMIF_BUSY) || (Fee_GetStatus() == MEMIF_BUSY_INTERNAL))
    {
        Fee_MainFunction();
        guard++;
        if (guard > 200u)
        {
            break;
        }
    }
}

/* ================================================================
 * Test: GC resets stale DataAddress for INVALID blocks
 *
 * Scenario: Write block 1, invalidate it, trigger GC, then
 * verify the block info has DataAddress=0 and Status=NOT_FOUND.
 * ================================================================ */
void test_GC_ResetsStaleInvalidBlock(void)
{
    uint8 data[32];
    uint16 i;
    const Fee_BlockInfoType* info;

    FullResetAndInit();
    for (i = 0u; i < 32u; i++) { data[i] = (uint8)(i + 0xA0u); }

    /* Write block 1 */
    TEST_ASSERT_EQUAL(E_OK, Fee_Write(1u, data));
    Fee_MainFunction();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());

    /* Invalidate block 1 */
    TEST_ASSERT_EQUAL(E_OK, Fee_InvalidateBlock(1u));
    Fee_MainFunction();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());

    /* Verify it's invalid with a non-zero address */
    info = Fee_Internal_GetBlockInfo(0u);
    TEST_ASSERT_NOT_NULL(info);
    TEST_ASSERT_EQUAL(FEE_BLOCK_INVALID, info->Status);
    TEST_ASSERT_TRUE(info->DataAddress != 0u);

    /* Fill sector to trigger GC on next write */
    {
        uint8 bigData[512];
        memset(bigData, 0xBB, sizeof(bigData));
        for (i = 0u; i < 7u; i++)
        {
            TEST_ASSERT_EQUAL(E_OK, Fee_Write(7u, bigData));
            ProcessUntilIdle();
            TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
        }
    }

    /* This write should trigger GC */
    {
        uint8 smallData[512];
        memset(smallData, 0xCC, sizeof(smallData));
        TEST_ASSERT_EQUAL(E_OK, Fee_Write(7u, smallData));
        ProcessUntilIdle();
        TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
    }

    /* After GC, invalid block 1 should be reset to NOT_FOUND with address 0 */
    info = Fee_Internal_GetBlockInfo(0u);
    TEST_ASSERT_NOT_NULL(info);
    TEST_ASSERT_EQUAL(FEE_BLOCK_NOT_FOUND, info->Status);
    TEST_ASSERT_EQUAL(0u, info->DataAddress);
}

/* ================================================================
 * Test: GC preserves valid blocks and resets non-valid blocks
 *
 * Scenario: Write blocks 1 and 2, invalidate block 1, trigger GC.
 * Block 2 should survive; block 1 should be reset to NOT_FOUND.
 * ================================================================ */
void test_GC_PreservesValidResetsInvalid(void)
{
    uint8 data32[32];
    uint8 data64[64];
    uint8 readBuf[64];
    const Fee_BlockInfoType* info;
    uint16 i;

    FullResetAndInit();
    for (i = 0u; i < 32u; i++) { data32[i] = (uint8)(i + 1u); }
    for (i = 0u; i < 64u; i++) { data64[i] = (uint8)(i + 0x40u); }

    /* Write block 1 and block 2 */
    TEST_ASSERT_EQUAL(E_OK, Fee_Write(1u, data32));
    Fee_MainFunction();
    TEST_ASSERT_EQUAL(E_OK, Fee_Write(2u, data64));
    Fee_MainFunction();

    /* Invalidate block 1 */
    TEST_ASSERT_EQUAL(E_OK, Fee_InvalidateBlock(1u));
    Fee_MainFunction();

    /* Fill sector with large writes to trigger GC.
     * Sector = 4096 bytes, header = 12. Usable = 4084.
     * Block 1 = aligned(12+32) = 48. Block 2 = aligned(12+64) = 80.
     * Block 7 = aligned(12+512) = 528.
     * After blocks 1+2: 128 bytes used. 4084-128 = 3956 remaining.
     * 3956 / 528 = 7 blocks fit, remainder = 3956 - 7*528 = 260.
     * So after 7 writes of block 7, only 260 bytes remain (< 528). */
    {
        uint8 bigData[512];
        memset(bigData, 0xAA, sizeof(bigData));
        for (i = 0u; i < 7u; i++)
        {
            TEST_ASSERT_EQUAL(E_OK, Fee_Write(7u, bigData));
            ProcessUntilIdle();
            TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
        }
    }

    /* This write should trigger GC since only 260 bytes remain */
    {
        uint8 bigData2[512];
        memset(bigData2, 0xDD, sizeof(bigData2));
        TEST_ASSERT_EQUAL(E_OK, Fee_Write(7u, bigData2));
        ProcessUntilIdle();
        TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
    }

    /* Block 1 should be NOT_FOUND (was invalid, reset by GC) */
    info = Fee_Internal_GetBlockInfo(0u);
    TEST_ASSERT_NOT_NULL(info);
    TEST_ASSERT_EQUAL(FEE_BLOCK_NOT_FOUND, info->Status);

    /* Block 2 should still be valid and readable */
    memset(readBuf, 0, sizeof(readBuf));
    TEST_ASSERT_EQUAL(E_OK, Fee_Read(2u, 0u, readBuf, 64u));
    Fee_MainFunction();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
    TEST_ASSERT_EQUAL_MEMORY(data64, readBuf, 64u);
}

/* ================================================================
 * Test: Config validation -- NumberOfBlocks > FEE_NUMBER_OF_BLOCKS
 * should fail during init.
 * ================================================================ */
void test_Init_ConfigTooManyBlocks(void)
{
    Fee_ConfigType badConfig;

    MemAcc_TestResetFlash();

    badConfig.BlockConfig = Fee_BlockConfigData;
    badConfig.NumberOfBlocks = FEE_NUMBER_OF_BLOCKS + 1u;
    badConfig.SectorStartAddress = 0u;
    badConfig.AddressAreaId = 0u;

    /* Should fail because NumberOfBlocks > FEE_NUMBER_OF_BLOCKS */
    Fee_Init(&badConfig);
    TEST_ASSERT_EQUAL(MEMIF_UNINIT, Fee_GetStatus());
}

/* ================================================================
 * Test: Init works without MemAcc_TestResetFlash (erase-before-format)
 *
 * This tests the fix for the Review Agent's high-priority finding:
 * Fresh init on zero-filled flash should still succeed because
 * Fee_Sector_Init erases sectors before formatting them.
 * ================================================================ */
void test_Init_WorksWithoutPreErase(void)
{
    /* Do NOT call MemAcc_TestResetFlash(). The MemAcc static buffer starts
     * as all 0x00 when the process starts. However, since previous tests
     * may have already run and set up the flash buffer, we manually zero
     * it to simulate a truly fresh process. */
    uint8* rawFlash = MemAcc_GetRawBuffer();
    memset(rawFlash, 0x00, MEMACC_TOTAL_SIZE);

    /* Init should succeed by erasing sectors before formatting */
    Fee_Init(&Fee_Config);
    TEST_ASSERT_EQUAL(MEMIF_IDLE, Fee_GetStatus());

    /* Should be able to write and read after init */
    {
        uint8 data[32];
        uint8 readBuf[32];
        uint16 i;
        for (i = 0u; i < 32u; i++) { data[i] = (uint8)(i + 0xF0u); }

        TEST_ASSERT_EQUAL(E_OK, Fee_Write(1u, data));
        Fee_MainFunction();
        TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());

        memset(readBuf, 0, sizeof(readBuf));
        TEST_ASSERT_EQUAL(E_OK, Fee_Read(1u, 0u, readBuf, 32u));
        Fee_MainFunction();
        TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
        TEST_ASSERT_EQUAL_MEMORY(data, readBuf, 32u);
    }
}

/* ================================================================
 * Test: AlignToPage public helper
 * ================================================================ */
void test_AlignToPage(void)
{
    /* FEE_VIRTUAL_PAGE_SIZE is 8 */
    TEST_ASSERT_EQUAL(8u, Fee_Sector_AlignToPage(1u));
    TEST_ASSERT_EQUAL(8u, Fee_Sector_AlignToPage(8u));
    TEST_ASSERT_EQUAL(16u, Fee_Sector_AlignToPage(9u));
    TEST_ASSERT_EQUAL(16u, Fee_Sector_AlignToPage(16u));
    TEST_ASSERT_EQUAL(24u, Fee_Sector_AlignToPage(17u));
    TEST_ASSERT_EQUAL(528u, Fee_Sector_AlignToPage(524u)); /* 12+512 = 524 -> 528 */
}

/* ================================================================
 * Test: Immediate flag preserved through scan (item 7 fix)
 * After init and scan, immediate flags from config should be
 * intact even for blocks not found on flash.
 * ================================================================ */
void test_ImmediateFlag_PreservedThroughScan(void)
{
    const Fee_BlockInfoType* info;

    FullResetAndInit();

    /* Block 5 (index 4) is configured as immediate */
    info = Fee_Internal_GetBlockInfo(4u);
    TEST_ASSERT_NOT_NULL(info);
    TEST_ASSERT_EQUAL(TRUE, info->Immediate);
    TEST_ASSERT_EQUAL(FEE_BLOCK_NOT_FOUND, info->Status);

    /* Block 6 (index 5) is configured as immediate */
    info = Fee_Internal_GetBlockInfo(5u);
    TEST_ASSERT_NOT_NULL(info);
    TEST_ASSERT_EQUAL(TRUE, info->Immediate);

    /* Block 1 (index 0) is NOT immediate */
    info = Fee_Internal_GetBlockInfo(0u);
    TEST_ASSERT_NOT_NULL(info);
    TEST_ASSERT_EQUAL(FALSE, info->Immediate);
}

/* ================================================================
 * Test: Fee_ValidateJobRequest via public API
 * Busy check uses the helper that caches status (item 1)
 * ================================================================ */
void test_BusyCheck_ReadsReturnsE_NOT_OK(void)
{
    uint8 data1[32];
    uint8 buf[64];

    FullResetAndInit();
    memset(data1, 0xAA, sizeof(data1));

    /* Queue a write to make module busy */
    TEST_ASSERT_EQUAL(E_OK, Fee_Write(1u, data1));
    TEST_ASSERT_EQUAL(MEMIF_BUSY, Fee_GetStatus());

    /* Read while busy should fail with FEE_E_BUSY */
    Det_ClearLastError();
    TEST_ASSERT_EQUAL(E_NOT_OK, Fee_Read(2u, 0u, buf, 64u));
    TEST_ASSERT_EQUAL(FEE_E_BUSY, Det_GetLastErrorId());

    /* InvalidateBlock while busy should fail with FEE_E_BUSY */
    Det_ClearLastError();
    TEST_ASSERT_EQUAL(E_NOT_OK, Fee_InvalidateBlock(1u));
    TEST_ASSERT_EQUAL(FEE_E_BUSY, Det_GetLastErrorId());

    /* EraseImmediateBlock while busy should fail with FEE_E_BUSY */
    Det_ClearLastError();
    TEST_ASSERT_EQUAL(E_NOT_OK, Fee_EraseImmediateBlock(5u));
    TEST_ASSERT_EQUAL(FEE_E_BUSY, Det_GetLastErrorId());

    /* Process the pending write to clean up */
    Fee_MainFunction();
    TEST_ASSERT_EQUAL(MEMIF_IDLE, Fee_GetStatus());
}

/* ================================================================
 * Unity test runner
 * ================================================================ */
int main(void)
{
    UNITY_BEGIN();

    /* GC stale pointer tests */
    RUN_TEST(test_GC_ResetsStaleInvalidBlock);
    RUN_TEST(test_GC_PreservesValidResetsInvalid);

    /* Config validation */
    RUN_TEST(test_Init_ConfigTooManyBlocks);

    /* Fresh init without pre-erase (Review high finding) */
    RUN_TEST(test_Init_WorksWithoutPreErase);

    /* AlignToPage helper */
    RUN_TEST(test_AlignToPage);

    /* Immediate flag preservation */
    RUN_TEST(test_ImmediateFlag_PreservedThroughScan);

    /* Busy check through helpers */
    RUN_TEST(test_BusyCheck_ReadsReturnsE_NOT_OK);

    return UNITY_END();
}
