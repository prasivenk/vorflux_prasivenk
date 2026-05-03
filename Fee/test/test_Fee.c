#include "unity.h"
#include "Fee.h"
#include "Fee_Internal.h"
#include "Fee_Sector.h"
#include "Fee_Cfg.h"
#include "Det.h"
#include "MemAcc.h"
#include <string.h>

/* --------------- NvM callback stubs with tracking flags --------------- */
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

/* --------------- Helper: process all pending work --------------- */
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
 * DET VALIDATION TESTS -- UNINIT TESTS (must run before any init)
 * ================================================================ */

/* Test: Fee_Read before init -> FEE_E_UNINIT */
void test_DET_Read_BeforeInit(void)
{
    uint8 buf[32];
    Std_ReturnType ret;

    /* Module is UNINIT at this point (first test, setUp doesn't init) */
    Det_ClearLastError();
    ret = Fee_Read(1u, 0u, buf, 32u);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(FEE_E_UNINIT, Det_GetLastErrorId());
}

/* Test: Fee_SetMode before init -> FEE_E_UNINIT */
void test_DET_SetMode_BeforeInit(void)
{
    Det_ClearLastError();
    Fee_SetMode(MEMIF_MODE_FAST);
    TEST_ASSERT_EQUAL(FEE_E_UNINIT, Det_GetLastErrorId());
}

/* Test: Fee_Write before init -> FEE_E_UNINIT */
void test_DET_Write_BeforeInit(void)
{
    uint8 buf[32];
    Std_ReturnType ret;

    Det_ClearLastError();
    ret = Fee_Write(1u, buf);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(FEE_E_UNINIT, Det_GetLastErrorId());
}

/* Test: Fee_GetJobResult before init -> FEE_E_UNINIT */
void test_DET_GetJobResult_BeforeInit(void)
{
    MemIf_JobResultType result;

    Det_ClearLastError();
    result = Fee_GetJobResult();
    TEST_ASSERT_EQUAL(MEMIF_JOB_FAILED, result);
    TEST_ASSERT_EQUAL(FEE_E_UNINIT, Det_GetLastErrorId());
}

/* Test: Fee_Cancel before init -> FEE_E_UNINIT */
void test_DET_Cancel_BeforeInit(void)
{
    Det_ClearLastError();
    Fee_Cancel();
    TEST_ASSERT_EQUAL(FEE_E_UNINIT, Det_GetLastErrorId());
}

/* Test: Fee_InvalidateBlock before init -> FEE_E_UNINIT */
void test_DET_InvalidateBlock_BeforeInit(void)
{
    Std_ReturnType ret;

    Det_ClearLastError();
    ret = Fee_InvalidateBlock(1u);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(FEE_E_UNINIT, Det_GetLastErrorId());
}

/* Test: Fee_EraseImmediateBlock before init -> FEE_E_UNINIT */
void test_DET_EraseImmediateBlock_BeforeInit(void)
{
    Std_ReturnType ret;

    Det_ClearLastError();
    ret = Fee_EraseImmediateBlock(5u);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(FEE_E_UNINIT, Det_GetLastErrorId());
}

/* Test: Fee_GetStatus before init -> MEMIF_UNINIT (no DET check, functional behavior) */
void test_PreInit_GetStatus_ReturnsUninit(void)
{
    TEST_ASSERT_EQUAL(MEMIF_UNINIT, Fee_GetStatus());
}

/* Test: Fee_MainFunction before init -> returns without error (functional behavior) */
void test_PreInit_MainFunction_NoCrash(void)
{
    Det_ClearLastError();
    Fee_MainFunction();
    /* No crash, no DET error (MainFunction just returns if uninit) */
    TEST_ASSERT_EQUAL(0u, Det_GetLastErrorId());
}

/* Test: Fee_Init(NULL_PTR) -> FEE_E_PARAM_POINTER, status stays MEMIF_UNINIT */
void test_DET_Init_NullPtr(void)
{
    Det_ClearLastError();
    Fee_Init(NULL_PTR);
    TEST_ASSERT_EQUAL(FEE_E_PARAM_POINTER, Det_GetLastErrorId());
    TEST_ASSERT_EQUAL(MEMIF_UNINIT, Fee_GetStatus());
}

/* ================================================================
 * DET VALIDATION TESTS -- POST-INIT (run after init)
 * ================================================================ */

/* Test: Fee_Read with invalid block number (99) -> FEE_E_INVALID_BLOCK_NO */
void test_DET_Read_InvalidBlockNumber(void)
{
    uint8 buf[32];
    Std_ReturnType ret;

    FullResetAndInit();
    Det_ClearLastError();
    ret = Fee_Read(99u, 0u, buf, 32u);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(FEE_E_INVALID_BLOCK_NO, Det_GetLastErrorId());
}

/* Test: Fee_Read with NULL data pointer -> FEE_E_PARAM_POINTER */
void test_DET_Read_NullPointer(void)
{
    Std_ReturnType ret;

    FullResetAndInit();
    Det_ClearLastError();
    ret = Fee_Read(1u, 0u, NULL_PTR, 32u);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(FEE_E_PARAM_POINTER, Det_GetLastErrorId());
}

/* Test: Fee_Read with offset out of range -> FEE_E_INVALID_BLOCK_OFS */
void test_DET_Read_OffsetOutOfRange(void)
{
    uint8 buf[32];
    Std_ReturnType ret;

    FullResetAndInit();
    Det_ClearLastError();
    /* Block 1 is 32 bytes, offset 100 + length 32 > 32 */
    ret = Fee_Read(1u, 100u, buf, 32u);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(FEE_E_INVALID_BLOCK_OFS, Det_GetLastErrorId());
}

/* Test: Fee_Write with NULL pointer -> FEE_E_PARAM_POINTER */
void test_DET_Write_NullPointer(void)
{
    Std_ReturnType ret;

    FullResetAndInit();
    Det_ClearLastError();
    ret = Fee_Write(1u, NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(FEE_E_PARAM_POINTER, Det_GetLastErrorId());
}

/* Test: Busy rejection -- second Fee_Write returns E_NOT_OK with FEE_E_BUSY */
void test_DET_Write_BusyRejection(void)
{
    uint8 data1[32];
    uint8 data2[64];
    Std_ReturnType ret;

    FullResetAndInit();
    memset(data1, 0xAA, sizeof(data1));
    memset(data2, 0xBB, sizeof(data2));

    /* First write should succeed */
    ret = Fee_Write(1u, data1);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(MEMIF_BUSY, Fee_GetStatus());

    /* Second write should fail -- module is busy */
    Det_ClearLastError();
    ret = Fee_Write(2u, data2);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(FEE_E_BUSY, Det_GetLastErrorId());
}

/* Test: Fee_Cancel with no job -> FEE_E_INVALID_CANCEL */
void test_DET_Cancel_NoJob(void)
{
    FullResetAndInit();
    Det_ClearLastError();
    Fee_Cancel();
    TEST_ASSERT_EQUAL(FEE_E_INVALID_CANCEL, Det_GetLastErrorId());
}

/* Test: Fee_EraseImmediateBlock on non-immediate block (1) -> FEE_E_INVALID_BLOCK_NO */
void test_DET_EraseImmediate_NonImmediateBlock(void)
{
    Std_ReturnType ret;

    FullResetAndInit();
    Det_ClearLastError();
    ret = Fee_EraseImmediateBlock(1u);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(FEE_E_INVALID_BLOCK_NO, Det_GetLastErrorId());
}

/* Test: Fee_GetVersionInfo(NULL_PTR) -> FEE_E_PARAM_POINTER */
void test_DET_GetVersionInfo_NullPtr(void)
{
    Det_ClearLastError();
    Fee_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_EQUAL(FEE_E_PARAM_POINTER, Det_GetLastErrorId());
}

/* ================================================================
 * FUNCTIONAL TESTS (all call FullResetAndInit explicitly)
 * ================================================================ */

/* Test: Init success -- verify MEMIF_IDLE and MEMIF_JOB_OK */
void test_Func_Init_Success(void)
{
    FullResetAndInit();
    TEST_ASSERT_EQUAL(MEMIF_IDLE, Fee_GetStatus());
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
}

/* Test: Write + Read round-trip */
void test_Func_WriteRead_RoundTrip(void)
{
    uint8 testData32[32];
    uint8 readBuf[32];
    uint16 i;
    Std_ReturnType ret;

    FullResetAndInit();

    for (i = 0u; i < 32u; i++)
    {
        testData32[i] = (uint8)(i + 0x10u);
    }

    /* Write block 1 */
    ret = Fee_Write(1u, testData32);
    TEST_ASSERT_EQUAL(E_OK, ret);
    Fee_MainFunction();

    TEST_ASSERT_EQUAL(MEMIF_IDLE, Fee_GetStatus());
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());

    /* Read block 1 */
    NvM_JobEndCalled = FALSE;
    memset(readBuf, 0x00, sizeof(readBuf));
    ret = Fee_Read(1u, 0u, readBuf, 32u);
    TEST_ASSERT_EQUAL(E_OK, ret);
    Fee_MainFunction();

    TEST_ASSERT_EQUAL(MEMIF_IDLE, Fee_GetStatus());
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
    TEST_ASSERT_EQUAL_MEMORY(testData32, readBuf, 32u);
}

/* Test: Partial read */
void test_Func_PartialRead(void)
{
    uint8 testData64[64];
    uint8 readBuf[20];
    uint16 i;
    Std_ReturnType ret;

    FullResetAndInit();

    for (i = 0u; i < 64u; i++)
    {
        testData64[i] = (uint8)(i + 0x20u);
    }

    /* Write block 2 (64 bytes) */
    ret = Fee_Write(2u, testData64);
    TEST_ASSERT_EQUAL(E_OK, ret);
    Fee_MainFunction();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());

    /* Read 20 bytes from offset 10 */
    memset(readBuf, 0x00, sizeof(readBuf));
    ret = Fee_Read(2u, 10u, readBuf, 20u);
    TEST_ASSERT_EQUAL(E_OK, ret);
    Fee_MainFunction();

    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
    TEST_ASSERT_EQUAL_MEMORY(&testData64[10], readBuf, 20u);
}

/* Test: Cancel */
void test_Func_Cancel(void)
{
    uint8 data[32];

    FullResetAndInit();
    memset(data, 0xCC, sizeof(data));

    /* Queue a write */
    TEST_ASSERT_EQUAL(E_OK, Fee_Write(1u, data));
    TEST_ASSERT_EQUAL(MEMIF_BUSY, Fee_GetStatus());

    /* Cancel it */
    Fee_Cancel();
    TEST_ASSERT_EQUAL(MEMIF_IDLE, Fee_GetStatus());
    TEST_ASSERT_EQUAL(MEMIF_JOB_CANCELLED, Fee_GetJobResult());
}

/* Test: InvalidateBlock */
void test_Func_InvalidateBlock(void)
{
    uint8 data[32];
    uint8 readBuf[32];
    uint16 i;

    FullResetAndInit();

    for (i = 0u; i < 32u; i++)
    {
        data[i] = (uint8)i;
    }

    /* Write block 1 */
    TEST_ASSERT_EQUAL(E_OK, Fee_Write(1u, data));
    Fee_MainFunction();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());

    /* Invalidate block 1 */
    NvM_JobEndCalled = FALSE;
    TEST_ASSERT_EQUAL(E_OK, Fee_InvalidateBlock(1u));
    Fee_MainFunction();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
    TEST_ASSERT_TRUE(NvM_JobEndCalled);

    /* Read block 1 -- should get MEMIF_BLOCK_INVALID */
    NvM_JobEndCalled = FALSE;
    NvM_JobErrorCalled = FALSE;
    TEST_ASSERT_EQUAL(E_OK, Fee_Read(1u, 0u, readBuf, 32u));
    Fee_MainFunction();
    TEST_ASSERT_EQUAL(MEMIF_BLOCK_INVALID, Fee_GetJobResult());
    TEST_ASSERT_TRUE(NvM_JobErrorCalled);
    TEST_ASSERT_FALSE(NvM_JobEndCalled);
}

/* Test: EraseImmediateBlock on immediate block (5) */
void test_Func_EraseImmediateBlock(void)
{
    uint8 data[16];
    uint16 i;

    FullResetAndInit();

    for (i = 0u; i < 16u; i++)
    {
        data[i] = (uint8)(i + 0x50u);
    }

    /* Write block 5 (immediate, 16 bytes) */
    TEST_ASSERT_EQUAL(E_OK, Fee_Write(5u, data));
    Fee_MainFunction();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());

    /* Erase immediate block 5 */
    NvM_JobEndCalled = FALSE;
    NvM_JobErrorCalled = FALSE;
    TEST_ASSERT_EQUAL(E_OK, Fee_EraseImmediateBlock(5u));
    Fee_MainFunction();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
    TEST_ASSERT_TRUE(NvM_JobEndCalled);
    TEST_ASSERT_FALSE(NvM_JobErrorCalled);
}

/* Test: GetVersionInfo */
void test_Func_GetVersionInfo(void)
{
    Std_VersionInfoType vinfo;

    memset(&vinfo, 0, sizeof(vinfo));
    Fee_GetVersionInfo(&vinfo);

    TEST_ASSERT_EQUAL(21u, vinfo.moduleID);
    TEST_ASSERT_EQUAL(0u, vinfo.vendorID);
    TEST_ASSERT_EQUAL(FEE_SW_MAJOR_VERSION, vinfo.sw_major_version);
    TEST_ASSERT_EQUAL(FEE_SW_MINOR_VERSION, vinfo.sw_minor_version);
    TEST_ASSERT_EQUAL(FEE_SW_PATCH_VERSION, vinfo.sw_patch_version);
}

/* Test: SetMode after init -- no error */
void test_Func_SetMode_AfterInit(void)
{
    FullResetAndInit();
    Det_ClearLastError();
    Fee_SetMode(MEMIF_MODE_FAST);
    TEST_ASSERT_EQUAL(0u, Det_GetLastErrorId());
    TEST_ASSERT_EQUAL(MEMIF_IDLE, Fee_GetStatus());
}

/* Test: Multiple blocks -- write unique data to blocks 1-8, read all back */
void test_Func_MultipleBlocks(void)
{
    static const uint16 blockSizes[8] = { 32u, 64u, 128u, 256u, 16u, 8u, 512u, 64u };
    uint8 writeData[512];
    uint8 readBuf[512];
    uint16 blk;
    uint16 i;

    FullResetAndInit();

    for (blk = 0u; blk < 8u; blk++)
    {
        uint16 blockNum = (uint16)(blk + 1u);
        uint16 size = blockSizes[blk];

        for (i = 0u; i < size; i++)
        {
            writeData[i] = (uint8)((blockNum * 17u) + i);
        }

        TEST_ASSERT_EQUAL(E_OK, Fee_Write(blockNum, writeData));
        Fee_MainFunction();
        TEST_ASSERT_EQUAL_MESSAGE(MEMIF_JOB_OK, Fee_GetJobResult(), "Write failed");
    }

    for (blk = 0u; blk < 8u; blk++)
    {
        uint16 blockNum = (uint16)(blk + 1u);
        uint16 size = blockSizes[blk];

        for (i = 0u; i < size; i++)
        {
            writeData[i] = (uint8)((blockNum * 17u) + i);
        }

        memset(readBuf, 0x00, sizeof(readBuf));
        TEST_ASSERT_EQUAL(E_OK, Fee_Read(blockNum, 0u, readBuf, size));
        Fee_MainFunction();
        TEST_ASSERT_EQUAL_MESSAGE(MEMIF_JOB_OK, Fee_GetJobResult(), "Read failed");
        TEST_ASSERT_EQUAL_MEMORY(writeData, readBuf, size);
    }
}

/* Test: GC integration -- write block 7 (512 bytes) repeatedly until sector fills */
void test_Func_GC_Integration(void)
{
    uint8 writeData[512];
    uint8 readBuf[512];
    uint16 i;

    FullResetAndInit();

    for (i = 0u; i < 512u; i++)
    {
        writeData[i] = (uint8)(i & 0xFFu);
    }

    /* Write block 7 repeatedly. Sector = 4096 bytes. Each entry = aligned(12+512)=528.
     * After 7 writes, sector is nearly full. 8th write should trigger GC. */
    for (i = 0u; i < 10u; i++)
    {
        writeData[0] = (uint8)i;
        writeData[511] = (uint8)(0xFFu - i);

        TEST_ASSERT_EQUAL(E_OK, Fee_Write(7u, writeData));
        ProcessUntilIdle();
        TEST_ASSERT_EQUAL_MESSAGE(MEMIF_JOB_OK, Fee_GetJobResult(), "Write with GC failed");
    }

    /* Read back and verify last written data */
    memset(readBuf, 0x00, sizeof(readBuf));
    TEST_ASSERT_EQUAL(E_OK, Fee_Read(7u, 0u, readBuf, 512u));
    Fee_MainFunction();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
    TEST_ASSERT_EQUAL(writeData[0], readBuf[0]);
    TEST_ASSERT_EQUAL(writeData[511], readBuf[511]);
}

/* Test: Power-loss simulation -- write blocks, re-init without flash reset, read back */
void test_Func_PowerLoss_Recovery(void)
{
    uint8 writeData[256];
    uint8 readBuf[256];
    uint16 blk;
    uint16 i;
    static const uint16 blockSizes[4] = { 32u, 64u, 128u, 256u };

    FullResetAndInit();

    /* Write blocks 1-4 */
    for (blk = 0u; blk < 4u; blk++)
    {
        uint16 blockNum = (uint16)(blk + 1u);
        uint16 size = blockSizes[blk];

        for (i = 0u; i < size; i++)
        {
            writeData[i] = (uint8)(blockNum + i);
        }

        TEST_ASSERT_EQUAL(E_OK, Fee_Write(blockNum, writeData));
        Fee_MainFunction();
        TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
    }

    /* Simulate power loss: re-init WITHOUT MemAcc_TestResetFlash */
    Fee_Init(&Fee_Config);
    TEST_ASSERT_EQUAL(MEMIF_IDLE, Fee_GetStatus());

    /* Read blocks 1-4 back and verify data recovered */
    for (blk = 0u; blk < 4u; blk++)
    {
        uint16 blockNum = (uint16)(blk + 1u);
        uint16 size = blockSizes[blk];

        for (i = 0u; i < size; i++)
        {
            writeData[i] = (uint8)(blockNum + i);
        }

        memset(readBuf, 0x00, sizeof(readBuf));
        TEST_ASSERT_EQUAL(E_OK, Fee_Read(blockNum, 0u, readBuf, size));
        Fee_MainFunction();
        TEST_ASSERT_EQUAL_MESSAGE(MEMIF_JOB_OK, Fee_GetJobResult(), "Recovery read failed");
        TEST_ASSERT_EQUAL_MEMORY(writeData, readBuf, size);
    }
}

/* Test: Power-loss during write -- incomplete entry (header+data but no valid marker) */
void test_Func_PowerLoss_DuringWrite(void)
{
    uint8 writeData[32];
    uint8 readBuf[32];
    uint8* rawFlash;
    uint32 writePtr;
    uint8 hdr[FEE_BLOCK_HEADER_SIZE];
    uint16 crc;
    uint8 crcInput[4 + 32]; /* BlockNumber(2) + BlockLength(2) + data(32) */
    uint16 i;

    FullResetAndInit();

    /* First, write block 1 with valid data */
    for (i = 0u; i < 32u; i++)
    {
        writeData[i] = (uint8)(i + 0xA0u);
    }
    TEST_ASSERT_EQUAL(E_OK, Fee_Write(1u, writeData));
    Fee_MainFunction();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());

    /* Now simulate an incomplete second write of block 1 directly into flash.
     * Write header + data but leave ValidMarker as 0xFF (pending/erased). */
    rawFlash = MemAcc_GetRawBuffer();
    TEST_ASSERT_NOT_NULL(rawFlash);

    /* Next write position: sector header(12) + aligned(12+32)=48 = offset 60 */
    writePtr = FEE_SECTOR_HEADER_SIZE;
    writePtr += (uint32)(((FEE_BLOCK_HEADER_SIZE + 32u) + FEE_VIRTUAL_PAGE_SIZE - 1u)
                 / FEE_VIRTUAL_PAGE_SIZE * FEE_VIRTUAL_PAGE_SIZE);

    /* Build incomplete header for block 1 with different data */
    memset(hdr, 0x00, FEE_BLOCK_HEADER_SIZE);
    hdr[0] = 1u; hdr[1] = 0u;    /* BlockNumber = 1 (LE) */
    hdr[2] = 32u; hdr[3] = 0u;   /* BlockLength = 32 (LE) */

    /* Compute CRC over BlockNumber + BlockLength + new data */
    crcInput[0] = 1u; crcInput[1] = 0u;
    crcInput[2] = 32u; crcInput[3] = 0u;
    for (i = 0u; i < 32u; i++)
    {
        crcInput[4u + i] = (uint8)(i + 0xB0u); /* Different data */
    }
    crc = Fee_Crc16(crcInput, sizeof(crcInput), 0xFFFFu);
    hdr[4] = (uint8)(crc & 0xFFu);
    hdr[5] = (uint8)((crc >> 8u) & 0xFFu);

    /* Bytes 6-10 reserved (already 0) */
    hdr[11] = FEE_VALID_MARKER_ERASED; /* 0xFF -- NOT committed */

    /* Write header directly to flash */
    (void)memcpy(&rawFlash[writePtr], hdr, FEE_BLOCK_HEADER_SIZE);

    /* Write data directly to flash */
    for (i = 0u; i < 32u; i++)
    {
        rawFlash[writePtr + FEE_BLOCK_HEADER_SIZE + i] = (uint8)(i + 0xB0u);
    }

    /* Re-init (simulating power-up after crash) */
    Fee_Init(&Fee_Config);
    TEST_ASSERT_EQUAL(MEMIF_IDLE, Fee_GetStatus());

    /* Read block 1 -- should get the FIRST valid write, not the incomplete one */
    memset(readBuf, 0x00, sizeof(readBuf));
    TEST_ASSERT_EQUAL(E_OK, Fee_Read(1u, 0u, readBuf, 32u));
    Fee_MainFunction();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
    TEST_ASSERT_EQUAL_MEMORY(writeData, readBuf, 32u); /* Original data */
}

/* Test: Read of never-written block -> MEMIF_BLOCK_INVALID */
void test_Func_Read_NeverWrittenBlock(void)
{
    uint8 readBuf[128];

    FullResetAndInit();
    NvM_JobEndCalled = FALSE;
    NvM_JobErrorCalled = FALSE;

    /* Block 3 (128 bytes) was never written */
    TEST_ASSERT_EQUAL(E_OK, Fee_Read(3u, 0u, readBuf, 128u));
    Fee_MainFunction();

    TEST_ASSERT_EQUAL(MEMIF_BLOCK_INVALID, Fee_GetJobResult());
    TEST_ASSERT_TRUE(NvM_JobErrorCalled);
    TEST_ASSERT_FALSE(NvM_JobEndCalled);
}

/* Test: GetStatus/GetJobResult at each lifecycle stage */
void test_Func_StatusLifecycle(void)
{
    uint8 data[32];
    uint8 readBuf[32];
    uint16 i;

    FullResetAndInit();

    for (i = 0u; i < 32u; i++)
    {
        data[i] = (uint8)i;
    }

    /* After init: IDLE, JOB_OK */
    TEST_ASSERT_EQUAL(MEMIF_IDLE, Fee_GetStatus());
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());

    /* After queuing write: BUSY, JOB_PENDING */
    TEST_ASSERT_EQUAL(E_OK, Fee_Write(1u, data));
    TEST_ASSERT_EQUAL(MEMIF_BUSY, Fee_GetStatus());
    TEST_ASSERT_EQUAL(MEMIF_JOB_PENDING, Fee_GetJobResult());

    /* After processing: IDLE, JOB_OK */
    Fee_MainFunction();
    TEST_ASSERT_EQUAL(MEMIF_IDLE, Fee_GetStatus());
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());

    /* After queuing read: BUSY, JOB_PENDING */
    TEST_ASSERT_EQUAL(E_OK, Fee_Read(1u, 0u, readBuf, 32u));
    TEST_ASSERT_EQUAL(MEMIF_BUSY, Fee_GetStatus());
    TEST_ASSERT_EQUAL(MEMIF_JOB_PENDING, Fee_GetJobResult());

    /* After processing read: IDLE, JOB_OK */
    Fee_MainFunction();
    TEST_ASSERT_EQUAL(MEMIF_IDLE, Fee_GetStatus());
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());

    /* Cancel scenario */
    TEST_ASSERT_EQUAL(E_OK, Fee_Write(1u, data));
    TEST_ASSERT_EQUAL(MEMIF_BUSY, Fee_GetStatus());
    Fee_Cancel();
    TEST_ASSERT_EQUAL(MEMIF_IDLE, Fee_GetStatus());
    TEST_ASSERT_EQUAL(MEMIF_JOB_CANCELLED, Fee_GetJobResult());
}

/* ================================================================
 * Unity test runner
 * ================================================================ */
int main(void)
{
    /* Ensure flash is in clean state but module is UNINIT for initial tests.
     * Fee_Internal's static Fee_ModuleStatus initializes to MEMIF_UNINIT.
     * We only call MemAcc_Init (not Fee_Init) so the Fee module stays UNINIT. */
    MemAcc_TestResetFlash();
    MemAcc_Init();

    UNITY_BEGIN();

    /* UNINIT DET tests -- MUST run first while module is still UNINIT */
    RUN_TEST(test_DET_Read_BeforeInit);
    RUN_TEST(test_DET_SetMode_BeforeInit);
    RUN_TEST(test_DET_Write_BeforeInit);
    RUN_TEST(test_DET_GetJobResult_BeforeInit);
    RUN_TEST(test_DET_Cancel_BeforeInit);
    RUN_TEST(test_DET_InvalidateBlock_BeforeInit);
    RUN_TEST(test_DET_EraseImmediateBlock_BeforeInit);
    RUN_TEST(test_PreInit_GetStatus_ReturnsUninit);
    RUN_TEST(test_PreInit_MainFunction_NoCrash);
    RUN_TEST(test_DET_Init_NullPtr);

    /* POST-INIT DET tests */
    RUN_TEST(test_DET_Read_InvalidBlockNumber);
    RUN_TEST(test_DET_Read_NullPointer);
    RUN_TEST(test_DET_Read_OffsetOutOfRange);
    RUN_TEST(test_DET_Write_NullPointer);
    RUN_TEST(test_DET_Write_BusyRejection);
    RUN_TEST(test_DET_Cancel_NoJob);
    RUN_TEST(test_DET_EraseImmediate_NonImmediateBlock);
    RUN_TEST(test_DET_GetVersionInfo_NullPtr);

    /* Functional tests */
    RUN_TEST(test_Func_Init_Success);
    RUN_TEST(test_Func_WriteRead_RoundTrip);
    RUN_TEST(test_Func_PartialRead);
    RUN_TEST(test_Func_Cancel);
    RUN_TEST(test_Func_InvalidateBlock);
    RUN_TEST(test_Func_EraseImmediateBlock);
    RUN_TEST(test_Func_GetVersionInfo);
    RUN_TEST(test_Func_SetMode_AfterInit);
    RUN_TEST(test_Func_MultipleBlocks);
    RUN_TEST(test_Func_GC_Integration);
    RUN_TEST(test_Func_PowerLoss_Recovery);
    RUN_TEST(test_Func_PowerLoss_DuringWrite);
    RUN_TEST(test_Func_Read_NeverWrittenBlock);
    RUN_TEST(test_Func_StatusLifecycle);

    return UNITY_END();
}
