#include "unity.h"
#include "Fee_Internal.h"
#include "Fee_Sector.h"
#include "Fee_Cfg.h"
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

/* --------------- Helper: reset and init --------------- */
static void ResetAndInit(void)
{
    MemAcc_TestResetFlash();
    TEST_ASSERT_EQUAL(E_OK, Fee_Internal_Init(&Fee_Config));
}

void setUp(void)
{
    NvM_JobEndCalled = FALSE;
    NvM_JobErrorCalled = FALSE;
    ResetAndInit();
}

void tearDown(void)
{
}

/* ================================================================
 * Test: Init transitions MEMIF_UNINIT -> MEMIF_BUSY_INTERNAL -> MEMIF_IDLE
 * ================================================================ */
void test_Init_StatusTransition(void)
{
    /* After setUp (which calls ResetAndInit), module should be IDLE */
    TEST_ASSERT_EQUAL(MEMIF_IDLE, Fee_Internal_GetStatus());
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_Internal_GetJobResult());
}

/* ================================================================
 * Test: Init with NULL config returns E_NOT_OK
 * ================================================================ */
void test_Init_NullConfig(void)
{
    TEST_ASSERT_EQUAL(E_NOT_OK, Fee_Internal_Init(NULL_PTR));
}

/* ================================================================
 * Test: QueueJob accepted when idle, rejected when busy
 * ================================================================ */
void test_QueueJob_AcceptedWhenIdle_RejectedWhenBusy(void)
{
    Fee_JobDescriptorType job;
    uint8 readBuf[32];

    memset(&job, 0, sizeof(job));
    job.JobType = FEE_JOB_READ;
    job.BlockNumber = 1u;
    job.BlockOffset = 0u;
    job.DataBufferPtr = readBuf;
    job.Length = 32u;

    /* First queue should succeed */
    TEST_ASSERT_EQUAL(E_OK, Fee_Internal_QueueJob(&job));
    TEST_ASSERT_EQUAL(MEMIF_BUSY, Fee_Internal_GetStatus());
    TEST_ASSERT_EQUAL(MEMIF_JOB_PENDING, Fee_Internal_GetJobResult());

    /* Second queue should fail (busy) */
    TEST_ASSERT_EQUAL(E_NOT_OK, Fee_Internal_QueueJob(&job));
}

/* ================================================================
 * Test: ProcessJob READ with valid block
 * ================================================================ */
void test_ProcessJob_Read_ValidBlock(void)
{
    Fee_JobDescriptorType job;
    Fee_BlockInfoType blockInfo;
    uint8 writeData[32];
    uint8 readBuf[32];
    uint16 i;

    /* Write a block via the sector layer first */
    blockInfo.Status = FEE_BLOCK_NOT_FOUND;
    blockInfo.DataAddress = 0u;
    blockInfo.Immediate = FALSE;
    for (i = 0u; i < 32u; i++)
    {
        writeData[i] = (uint8)(i + 0xA0u);
    }
    TEST_ASSERT_EQUAL(E_OK, Fee_Sector_WriteBlock(1u, writeData, 32u, &blockInfo));

    /* Re-init to pick up the written block via scan */
    MemAcc_Init();
    TEST_ASSERT_EQUAL(E_OK, Fee_Internal_Init(&Fee_Config));

    /* Queue a read job */
    memset(readBuf, 0x00, sizeof(readBuf));
    memset(&job, 0, sizeof(job));
    job.JobType = FEE_JOB_READ;
    job.BlockNumber = 1u;
    job.BlockOffset = 0u;
    job.DataBufferPtr = readBuf;
    job.Length = 32u;

    NvM_JobEndCalled = FALSE;
    NvM_JobErrorCalled = FALSE;

    TEST_ASSERT_EQUAL(E_OK, Fee_Internal_QueueJob(&job));
    Fee_Internal_ProcessJob();

    /* Verify */
    TEST_ASSERT_EQUAL(MEMIF_IDLE, Fee_Internal_GetStatus());
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_Internal_GetJobResult());
    TEST_ASSERT_TRUE(NvM_JobEndCalled);
    TEST_ASSERT_FALSE(NvM_JobErrorCalled);
    TEST_ASSERT_EQUAL_MEMORY(writeData, readBuf, 32u);
}

/* ================================================================
 * Test: ProcessJob READ of non-existent block
 * ================================================================ */
void test_ProcessJob_Read_NonExistentBlock(void)
{
    Fee_JobDescriptorType job;
    uint8 readBuf[32];

    memset(readBuf, 0x00, sizeof(readBuf));
    memset(&job, 0, sizeof(job));
    job.JobType = FEE_JOB_READ;
    job.BlockNumber = 1u;  /* Block 1 was never written */
    job.BlockOffset = 0u;
    job.DataBufferPtr = readBuf;
    job.Length = 32u;

    TEST_ASSERT_EQUAL(E_OK, Fee_Internal_QueueJob(&job));
    Fee_Internal_ProcessJob();

    TEST_ASSERT_EQUAL(MEMIF_IDLE, Fee_Internal_GetStatus());
    TEST_ASSERT_EQUAL(MEMIF_BLOCK_INVALID, Fee_Internal_GetJobResult());
    TEST_ASSERT_TRUE(NvM_JobErrorCalled);
    TEST_ASSERT_FALSE(NvM_JobEndCalled);
}

/* ================================================================
 * Test: ProcessJob WRITE and read-back verification
 * ================================================================ */
void test_ProcessJob_Write_Success(void)
{
    Fee_JobDescriptorType job;
    uint8 writeData[32];
    uint8 readBuf[32];
    uint16 i;
    const Fee_BlockInfoType* info;

    /* Fill write data */
    for (i = 0u; i < 32u; i++)
    {
        writeData[i] = (uint8)(i + 0x10u);
    }

    /* Queue write job */
    memset(&job, 0, sizeof(job));
    job.JobType = FEE_JOB_WRITE;
    job.BlockNumber = 1u;
    job.WriteDataPtr = writeData;
    job.Length = 32u; /* Note: actual write uses configured block size */

    TEST_ASSERT_EQUAL(E_OK, Fee_Internal_QueueJob(&job));
    Fee_Internal_ProcessJob();

    TEST_ASSERT_EQUAL(MEMIF_IDLE, Fee_Internal_GetStatus());
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_Internal_GetJobResult());
    TEST_ASSERT_TRUE(NvM_JobEndCalled);
    TEST_ASSERT_FALSE(NvM_JobErrorCalled);

    /* Verify block info updated */
    info = Fee_Internal_GetBlockInfo(0u); /* Block 1 is at index 0 */
    TEST_ASSERT_NOT_NULL(info);
    TEST_ASSERT_EQUAL(FEE_BLOCK_VALID, info->Status);
    TEST_ASSERT_TRUE(info->DataAddress != 0u);

    /* Read back via sector layer to verify data */
    memset(readBuf, 0x00, sizeof(readBuf));
    TEST_ASSERT_EQUAL(E_OK, Fee_Sector_ReadBlock(info, 0u, readBuf, 32u));
    TEST_ASSERT_EQUAL_MEMORY(writeData, readBuf, 32u);
}

/* ================================================================
 * Test: ProcessJob INVALIDATE
 * ================================================================ */
void test_ProcessJob_Invalidate(void)
{
    Fee_JobDescriptorType job;
    uint8 writeData[32];
    const Fee_BlockInfoType* info;
    uint16 i;

    /* First, write a block */
    for (i = 0u; i < 32u; i++)
    {
        writeData[i] = (uint8)i;
    }
    memset(&job, 0, sizeof(job));
    job.JobType = FEE_JOB_WRITE;
    job.BlockNumber = 1u;
    job.WriteDataPtr = writeData;

    TEST_ASSERT_EQUAL(E_OK, Fee_Internal_QueueJob(&job));
    Fee_Internal_ProcessJob();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_Internal_GetJobResult());

    /* Now invalidate it */
    NvM_JobEndCalled = FALSE;
    NvM_JobErrorCalled = FALSE;

    memset(&job, 0, sizeof(job));
    job.JobType = FEE_JOB_INVALIDATE;
    job.BlockNumber = 1u;

    TEST_ASSERT_EQUAL(E_OK, Fee_Internal_QueueJob(&job));
    Fee_Internal_ProcessJob();

    TEST_ASSERT_EQUAL(MEMIF_IDLE, Fee_Internal_GetStatus());
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_Internal_GetJobResult());
    TEST_ASSERT_TRUE(NvM_JobEndCalled);

    /* Verify block status changed */
    info = Fee_Internal_GetBlockInfo(0u);
    TEST_ASSERT_NOT_NULL(info);
    TEST_ASSERT_EQUAL(FEE_BLOCK_INVALID, info->Status);
}

/* ================================================================
 * Test: ProcessJob ERASE_IMMEDIATE
 * ================================================================ */
void test_ProcessJob_EraseImmediate(void)
{
    Fee_JobDescriptorType job;
    uint8 writeData[16];
    const Fee_BlockInfoType* info;
    uint16 i;

    /* Block 5 (index 4) is immediate, size 16 */
    for (i = 0u; i < 16u; i++)
    {
        writeData[i] = (uint8)(i + 0x50u);
    }

    /* Write block 5 first */
    memset(&job, 0, sizeof(job));
    job.JobType = FEE_JOB_WRITE;
    job.BlockNumber = 5u;
    job.WriteDataPtr = writeData;

    TEST_ASSERT_EQUAL(E_OK, Fee_Internal_QueueJob(&job));
    Fee_Internal_ProcessJob();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_Internal_GetJobResult());

    info = Fee_Internal_GetBlockInfo(4u);
    TEST_ASSERT_NOT_NULL(info);
    TEST_ASSERT_EQUAL(FEE_BLOCK_VALID, info->Status);

    /* Erase immediate */
    NvM_JobEndCalled = FALSE;
    NvM_JobErrorCalled = FALSE;

    memset(&job, 0, sizeof(job));
    job.JobType = FEE_JOB_ERASE_IMMEDIATE;
    job.BlockNumber = 5u;

    TEST_ASSERT_EQUAL(E_OK, Fee_Internal_QueueJob(&job));
    Fee_Internal_ProcessJob();

    TEST_ASSERT_EQUAL(MEMIF_IDLE, Fee_Internal_GetStatus());
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_Internal_GetJobResult());
    TEST_ASSERT_TRUE(NvM_JobEndCalled);
    TEST_ASSERT_FALSE(NvM_JobErrorCalled);

    /* After erase immediate, block should be NOT_FOUND */
    info = Fee_Internal_GetBlockInfo(4u);
    TEST_ASSERT_NOT_NULL(info);
    TEST_ASSERT_EQUAL(FEE_BLOCK_NOT_FOUND, info->Status);
}

/* ================================================================
 * Test: CancelJob succeeds when job is pending
 * ================================================================ */
void test_CancelJob_Success(void)
{
    Fee_JobDescriptorType job;
    uint8 readBuf[32];

    memset(&job, 0, sizeof(job));
    job.JobType = FEE_JOB_READ;
    job.BlockNumber = 1u;
    job.BlockOffset = 0u;
    job.DataBufferPtr = readBuf;
    job.Length = 32u;

    TEST_ASSERT_EQUAL(E_OK, Fee_Internal_QueueJob(&job));
    TEST_ASSERT_EQUAL(MEMIF_BUSY, Fee_Internal_GetStatus());

    /* Cancel the job */
    TEST_ASSERT_EQUAL(E_OK, Fee_Internal_CancelJob());
    TEST_ASSERT_EQUAL(MEMIF_IDLE, Fee_Internal_GetStatus());
    TEST_ASSERT_EQUAL(MEMIF_JOB_CANCELLED, Fee_Internal_GetJobResult());
}

/* ================================================================
 * Test: CancelJob with no pending job returns E_NOT_OK
 * ================================================================ */
void test_CancelJob_NoPendingJob(void)
{
    TEST_ASSERT_EQUAL(E_NOT_OK, Fee_Internal_CancelJob());
}

/* ================================================================
 * Test: FindBlockIndex valid and invalid block numbers
 * ================================================================ */
void test_FindBlockIndex_Valid(void)
{
    /* Block numbers 1-8 are configured at indices 0-7 */
    TEST_ASSERT_EQUAL(0u, Fee_Internal_FindBlockIndex(1u));
    TEST_ASSERT_EQUAL(1u, Fee_Internal_FindBlockIndex(2u));
    TEST_ASSERT_EQUAL(2u, Fee_Internal_FindBlockIndex(3u));
    TEST_ASSERT_EQUAL(3u, Fee_Internal_FindBlockIndex(4u));
    TEST_ASSERT_EQUAL(4u, Fee_Internal_FindBlockIndex(5u));
    TEST_ASSERT_EQUAL(5u, Fee_Internal_FindBlockIndex(6u));
    TEST_ASSERT_EQUAL(6u, Fee_Internal_FindBlockIndex(7u));
    TEST_ASSERT_EQUAL(7u, Fee_Internal_FindBlockIndex(8u));
}

void test_FindBlockIndex_Invalid(void)
{
    TEST_ASSERT_EQUAL(0xFFFFu, Fee_Internal_FindBlockIndex(99u));
    TEST_ASSERT_EQUAL(0xFFFFu, Fee_Internal_FindBlockIndex(0u));
    TEST_ASSERT_EQUAL(0xFFFFu, Fee_Internal_FindBlockIndex(9u));
}

/* ================================================================
 * Test: GetBlockInfo valid and invalid index
 * ================================================================ */
void test_GetBlockInfo_Valid(void)
{
    const Fee_BlockInfoType* info = Fee_Internal_GetBlockInfo(0u);
    TEST_ASSERT_NOT_NULL(info);
}

void test_GetBlockInfo_InvalidIndex(void)
{
    const Fee_BlockInfoType* info = Fee_Internal_GetBlockInfo(FEE_NUMBER_OF_BLOCKS);
    TEST_ASSERT_NULL(info);

    info = Fee_Internal_GetBlockInfo(0xFFFFu);
    TEST_ASSERT_NULL(info);
}

/* ================================================================
 * Test: GC trigger -- fill sector, verify CheckAndTriggerGC sets pending
 * ================================================================ */
void test_GCTrigger_SectorFull(void)
{
    Fee_JobDescriptorType job;
    uint8 writeData[512];
    uint16 i;

    memset(writeData, 0xBB, sizeof(writeData));

    /* Write multiple 512-byte blocks (block 7, index 6) to fill the sector.
     * Sector size = 4096, header = 12. Usable = 4084.
     * Each 512-byte block: aligned(12+512) = 528 bytes.
     * 4084 / 528 = 7 blocks, remainder = 388 bytes */
    for (i = 0u; i < 7u; i++)
    {
        NvM_JobEndCalled = FALSE;
        memset(&job, 0, sizeof(job));
        job.JobType = FEE_JOB_WRITE;
        job.BlockNumber = 7u;
        job.WriteDataPtr = writeData;

        TEST_ASSERT_EQUAL(E_OK, Fee_Internal_QueueJob(&job));
        Fee_Internal_ProcessJob();
        TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_Internal_GetJobResult());
    }

    /* Now the sector should be nearly full -- not enough for largest block (512) */
    /* Largest block needs aligned(12+512) = 528 bytes, but only 388 remain */
    Fee_Internal_CheckAndTriggerGC();

    /* Next ProcessJob should run GC */
    /* Queue another write job that will trigger GC via ProcessJob */
    memset(&job, 0, sizeof(job));
    job.JobType = FEE_JOB_WRITE;
    job.BlockNumber = 7u;
    job.WriteDataPtr = writeData;

    NvM_JobEndCalled = FALSE;
    TEST_ASSERT_EQUAL(E_OK, Fee_Internal_QueueJob(&job));

    /* First ProcessJob handles GC */
    Fee_Internal_ProcessJob();

    /* Second ProcessJob handles the actual write */
    Fee_Internal_ProcessJob();

    TEST_ASSERT_EQUAL(MEMIF_IDLE, Fee_Internal_GetStatus());
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_Internal_GetJobResult());
    TEST_ASSERT_TRUE(NvM_JobEndCalled);
}



/* ================================================================
 * Test: GC failure -- force scenario where GC cannot find erased sector
 *
 * Strategy: Fill the active sector, then manually mark all remaining
 * erased sectors as FULL by writing sector headers to them.
 * Then re-init (without resetting flash) so the sector layer picks
 * up the state. After re-init the active sector is still nearly full
 * and all others are FULL, so a write that needs space will trigger
 * GC which will fail because no erased sector exists.
 * ================================================================ */
void test_GCFailure_NoErasedSector(void)
{
    Fee_JobDescriptorType job;
    uint8 writeData[512];
    uint16 i;

    memset(writeData, 0xDD, sizeof(writeData));

    /* Fill the active sector with 7 x 512-byte writes */
    for (i = 0u; i < 7u; i++)
    {
        memset(&job, 0, sizeof(job));
        job.JobType = FEE_JOB_WRITE;
        job.BlockNumber = 7u;
        job.WriteDataPtr = writeData;

        TEST_ASSERT_EQUAL(E_OK, Fee_Internal_QueueJob(&job));
        Fee_Internal_ProcessJob();
        TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_Internal_GetJobResult());
    }

    /* Now manually write FULL sector headers to all remaining erased sectors */
    {
        uint8 sIdx;
        uint8 activeIdx = Fee_Sector_GetActiveSectorIndex();
        for (sIdx = 0u; sIdx < FEE_NUMBER_OF_SECTORS; sIdx++)
        {
            if (sIdx != activeIdx)
            {
                uint8 hdr[FEE_SECTOR_HEADER_SIZE];
                uint32 addr = Fee_Config.SectorStartAddress + ((uint32)sIdx * FEE_SECTOR_SIZE);

                memset(hdr, 0x00, FEE_SECTOR_HEADER_SIZE);
                /* FEE_SECTOR_MAGIC in little-endian */
                hdr[0] = 0xE0u; hdr[1] = 0xFEu; hdr[2] = 0xE0u; hdr[3] = 0xFEu;
                /* Seq number = 0 */
                hdr[4] = 0x00u; hdr[5] = 0x00u; hdr[6] = 0x00u; hdr[7] = 0x00u;
                /* Erase count = 0 */
                hdr[8] = 0x00u; hdr[9] = 0x00u;
                /* Status: FULL */
                hdr[10] = FEE_SECTOR_STATUS_FULL;
                hdr[11] = 0x00u;

                MemAcc_Write(addr, hdr, FEE_SECTOR_HEADER_SIZE);
            }
        }
    }

    /* Re-init without resetting flash -- sector layer will see:
     *   - the active sector (nearly full, with 7 blocks)
     *   - 3 FULL sectors (no erased sectors available) */
    MemAcc_Init();
    TEST_ASSERT_EQUAL(E_OK, Fee_Internal_Init(&Fee_Config));

    /* Queue a 512-byte write -- active sector has ~388 bytes free,
     * not enough for aligned(12+512)=528 bytes */
    NvM_JobEndCalled = FALSE;
    NvM_JobErrorCalled = FALSE;

    memset(&job, 0, sizeof(job));
    job.JobType = FEE_JOB_WRITE;
    job.BlockNumber = 7u;
    job.WriteDataPtr = writeData;

    TEST_ASSERT_EQUAL(E_OK, Fee_Internal_QueueJob(&job));

    /* First ProcessJob: write detects no space, sets GcPending, returns */
    Fee_Internal_ProcessJob();

    /* Second ProcessJob: runs GC which fails (no erased sectors) */
    Fee_Internal_ProcessJob();

    TEST_ASSERT_EQUAL(MEMIF_IDLE, Fee_Internal_GetStatus());
    TEST_ASSERT_EQUAL(MEMIF_JOB_FAILED, Fee_Internal_GetJobResult());
    TEST_ASSERT_TRUE(NvM_JobErrorCalled);
    TEST_ASSERT_FALSE(NvM_JobEndCalled);
}

/* ================================================================
 * Test: ProcessJob with no pending job does nothing
 * ================================================================ */
void test_ProcessJob_NoPendingJob(void)
{
    MemIf_StatusType statusBefore = Fee_Internal_GetStatus();
    MemIf_JobResultType resultBefore = Fee_Internal_GetJobResult();

    Fee_Internal_ProcessJob();

    TEST_ASSERT_EQUAL(statusBefore, Fee_Internal_GetStatus());
    TEST_ASSERT_EQUAL(resultBefore, Fee_Internal_GetJobResult());
}

/* ================================================================
 * Test: QueueJob with NULL pointer returns E_NOT_OK
 * ================================================================ */
void test_QueueJob_NullPointer(void)
{
    TEST_ASSERT_EQUAL(E_NOT_OK, Fee_Internal_QueueJob(NULL_PTR));
}

/* ================================================================
 * Test: Multiple write-read cycles for same block
 * ================================================================ */
void test_WriteRead_MultipleUpdates(void)
{
    Fee_JobDescriptorType job;
    uint8 writeData1[32];
    uint8 writeData2[32];
    uint8 readBuf[32];
    const Fee_BlockInfoType* info;
    uint16 i;

    /* First write */
    for (i = 0u; i < 32u; i++)
    {
        writeData1[i] = (uint8)(i + 0x01u);
    }
    memset(&job, 0, sizeof(job));
    job.JobType = FEE_JOB_WRITE;
    job.BlockNumber = 1u;
    job.WriteDataPtr = writeData1;

    TEST_ASSERT_EQUAL(E_OK, Fee_Internal_QueueJob(&job));
    Fee_Internal_ProcessJob();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_Internal_GetJobResult());

    /* Second write (update) */
    for (i = 0u; i < 32u; i++)
    {
        writeData2[i] = (uint8)(i + 0x80u);
    }
    memset(&job, 0, sizeof(job));
    job.JobType = FEE_JOB_WRITE;
    job.BlockNumber = 1u;
    job.WriteDataPtr = writeData2;

    TEST_ASSERT_EQUAL(E_OK, Fee_Internal_QueueJob(&job));
    Fee_Internal_ProcessJob();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_Internal_GetJobResult());

    /* Read back -- should get second write data */
    memset(readBuf, 0x00, sizeof(readBuf));
    info = Fee_Internal_GetBlockInfo(0u);
    TEST_ASSERT_NOT_NULL(info);
    TEST_ASSERT_EQUAL(FEE_BLOCK_VALID, info->Status);
    TEST_ASSERT_EQUAL(E_OK, Fee_Sector_ReadBlock(info, 0u, readBuf, 32u));
    TEST_ASSERT_EQUAL_MEMORY(writeData2, readBuf, 32u);
}

/* ================================================================
 * Test: Read of invalidated block returns MEMIF_BLOCK_INVALID
 * ================================================================ */
void test_Read_AfterInvalidate(void)
{
    Fee_JobDescriptorType job;
    uint8 writeData[32];
    uint8 readBuf[32];
    uint16 i;

    /* Write block 1 */
    for (i = 0u; i < 32u; i++)
    {
        writeData[i] = (uint8)i;
    }
    memset(&job, 0, sizeof(job));
    job.JobType = FEE_JOB_WRITE;
    job.BlockNumber = 1u;
    job.WriteDataPtr = writeData;

    TEST_ASSERT_EQUAL(E_OK, Fee_Internal_QueueJob(&job));
    Fee_Internal_ProcessJob();

    /* Invalidate */
    memset(&job, 0, sizeof(job));
    job.JobType = FEE_JOB_INVALIDATE;
    job.BlockNumber = 1u;

    TEST_ASSERT_EQUAL(E_OK, Fee_Internal_QueueJob(&job));
    Fee_Internal_ProcessJob();

    /* Try to read */
    NvM_JobEndCalled = FALSE;
    NvM_JobErrorCalled = FALSE;

    memset(&job, 0, sizeof(job));
    job.JobType = FEE_JOB_READ;
    job.BlockNumber = 1u;
    job.BlockOffset = 0u;
    job.DataBufferPtr = readBuf;
    job.Length = 32u;

    TEST_ASSERT_EQUAL(E_OK, Fee_Internal_QueueJob(&job));
    Fee_Internal_ProcessJob();

    TEST_ASSERT_EQUAL(MEMIF_BLOCK_INVALID, Fee_Internal_GetJobResult());
    TEST_ASSERT_TRUE(NvM_JobErrorCalled);
    TEST_ASSERT_FALSE(NvM_JobEndCalled);
}

/* ================================================================
 * Test: CheckAndTriggerGC does nothing if already pending
 * ================================================================ */
void test_CheckAndTriggerGC_AlreadyPending(void)
{
    Fee_JobDescriptorType job;
    uint8 writeData[512];
    uint16 i;

    memset(writeData, 0xAA, sizeof(writeData));

    /* Fill the sector */
    for (i = 0u; i < 7u; i++)
    {
        memset(&job, 0, sizeof(job));
        job.JobType = FEE_JOB_WRITE;
        job.BlockNumber = 7u;
        job.WriteDataPtr = writeData;

        TEST_ASSERT_EQUAL(E_OK, Fee_Internal_QueueJob(&job));
        Fee_Internal_ProcessJob();
    }

    /* First call should set GC pending */
    Fee_Internal_CheckAndTriggerGC();

    /* Second call should be a no-op (already pending) */
    Fee_Internal_CheckAndTriggerGC();

    /* Process GC */
    Fee_Internal_ProcessJob();

    /* Should be idle after GC completes */
    TEST_ASSERT_EQUAL(MEMIF_IDLE, Fee_Internal_GetStatus());
}

/* ================================================================
 * Test: Immediate flag preserved in block info table
 * ================================================================ */
void test_ImmediateFlag_PreservedAfterInit(void)
{
    const Fee_BlockInfoType* info;

    /* Block 5 (index 4) is immediate */
    info = Fee_Internal_GetBlockInfo(4u);
    TEST_ASSERT_NOT_NULL(info);
    TEST_ASSERT_EQUAL(TRUE, info->Immediate);

    /* Block 6 (index 5) is immediate */
    info = Fee_Internal_GetBlockInfo(5u);
    TEST_ASSERT_NOT_NULL(info);
    TEST_ASSERT_EQUAL(TRUE, info->Immediate);

    /* Block 1 (index 0) is NOT immediate */
    info = Fee_Internal_GetBlockInfo(0u);
    TEST_ASSERT_NOT_NULL(info);
    TEST_ASSERT_EQUAL(FALSE, info->Immediate);
}

/* ================================================================
 * Unity test runner
 * ================================================================ */
int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_Init_StatusTransition);
    RUN_TEST(test_Init_NullConfig);
    RUN_TEST(test_QueueJob_AcceptedWhenIdle_RejectedWhenBusy);
    RUN_TEST(test_QueueJob_NullPointer);
    RUN_TEST(test_ProcessJob_NoPendingJob);
    RUN_TEST(test_ProcessJob_Read_ValidBlock);
    RUN_TEST(test_ProcessJob_Read_NonExistentBlock);
    RUN_TEST(test_ProcessJob_Write_Success);
    RUN_TEST(test_WriteRead_MultipleUpdates);
    RUN_TEST(test_ProcessJob_Invalidate);
    RUN_TEST(test_ProcessJob_EraseImmediate);
    RUN_TEST(test_CancelJob_Success);
    RUN_TEST(test_CancelJob_NoPendingJob);
    RUN_TEST(test_FindBlockIndex_Valid);
    RUN_TEST(test_FindBlockIndex_Invalid);
    RUN_TEST(test_GetBlockInfo_Valid);
    RUN_TEST(test_GetBlockInfo_InvalidIndex);
    RUN_TEST(test_GCTrigger_SectorFull);
    RUN_TEST(test_GCFailure_NoErasedSector);
    RUN_TEST(test_Read_AfterInvalidate);
    RUN_TEST(test_CheckAndTriggerGC_AlreadyPending);
    RUN_TEST(test_ImmediateFlag_PreservedAfterInit);

    return UNITY_END();
}
