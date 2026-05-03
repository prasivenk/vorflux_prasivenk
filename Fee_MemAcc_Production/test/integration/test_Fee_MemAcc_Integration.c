/**
 * \file       test_Fee_MemAcc_Integration.c
 * \brief      Integration Tests -- Fee -> MemAcc -> Mem_DFLS_Stub end-to-end
 *
 * \details    20+ end-to-end tests exercising the full write/read/verify path
 *             through Fee -> MemAcc -> Mem_DFLS_Stub.
 *
 * \copyright  Copyright (c) 2024 -- All rights reserved.
 */

/*============================================================================*
 *  Includes
 *============================================================================*/

#include "unity/unity.h"
#include "Fee.h"
#include "Fee_StateMachine.h"
#include "Fee_GarbageCollect.h"
#include "Fee_Safety.h"
#include "Fee_Sector.h"
#include "Fee_Crc.h"
#include "Fee_PBcfg.h"
#include "MemAcc.h"
#include "MemAcc_PBcfg.h"
#include "Det_Stub.h"
#include "Dem_Stub.h"
#include "Mem_DFLS_Stub.h"
#include "SchM_Stub.h"
#include "NvM_Cbk_Stub.h"
#include <string.h>

/*============================================================================*
 *  Constants
 *============================================================================*/

#define MAX_CYCLES  200u

/*============================================================================*
 *  Helpers
 *============================================================================*/

static void DriveOneCycle(void)
{
    MemAcc_MainFunction();
    Fee_MainFunction();
}

static void DriveInitToCompletion(void)
{
    uint32 cycle;
    Fee_Init(&Fee_Config);
    for (cycle = 0u; cycle < MAX_CYCLES; cycle++)
    {
        DriveOneCycle();
        if (Fee_GetStatus() == MEMIF_IDLE) { break; }
    }
}

static void DriveJobToCompletion(void)
{
    uint32 cycle;
    for (cycle = 0u; cycle < MAX_CYCLES; cycle++)
    {
        DriveOneCycle();
        if (Fee_GetStatus() == MEMIF_IDLE) { break; }
    }
}

/*============================================================================*
 *  Setup / Teardown
 *============================================================================*/

void setUp(void)
{
    Det_Stub_Reset();
    Dem_Stub_Reset();
    Mem_DFLS_Stub_Reset();
    NvM_Cbk_Stub_Reset();
    SchM_Stub_Reset();

    Fee_ModuleStatus = MEMIF_UNINIT;
    Fee_InternalState = FEE_STATE_UNINIT;
    Fee_LastJobResult = MEMIF_JOB_OK;
    Fee_ConfigPtr = NULL_PTR;
    Fee_CurrentJob.Type = FEE_JOB_NONE;

    MemAcc_Init(&MemAcc_Config);
}

void tearDown(void) { }

/*============================================================================*
 *  Tests
 *============================================================================*/

/* 1. Init on blank flash reaches IDLE */
static void test_Integration_Init_BlankFlash(void)
{
    DriveInitToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_IDLE, Fee_GetStatus());
    TEST_ASSERT_EQUAL(0u, Det_Stub_GetErrorCount());
}

/* 2. Write and read back 32-byte block */
static void test_Integration_WriteRead_Block1(void)
{
    uint8 writeData[32], readBuf[32];
    memset(writeData, 0xAA, 32);

    DriveInitToCompletion();
    Fee_Write(1u, writeData);
    DriveJobToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());

    Fee_Read(1u, 0u, readBuf, 32u);
    DriveJobToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
    TEST_ASSERT_EQUAL_MEMORY(writeData, readBuf, 32);
}

/* 3. Write and read 64-byte block */
static void test_Integration_WriteRead_Block2(void)
{
    uint8 writeData[64], readBuf[64];
    memset(writeData, 0xBB, 64);

    DriveInitToCompletion();
    Fee_Write(2u, writeData);
    DriveJobToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());

    Fee_Read(2u, 0u, readBuf, 64u);
    DriveJobToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
    TEST_ASSERT_EQUAL_MEMORY(writeData, readBuf, 64);
}

/* 4. Write and read 128-byte block */
static void test_Integration_WriteRead_Block3(void)
{
    uint8 writeData[128], readBuf[128];
    memset(writeData, 0xCC, 128);

    DriveInitToCompletion();
    Fee_Write(3u, writeData);
    DriveJobToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());

    Fee_Read(3u, 0u, readBuf, 128u);
    DriveJobToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
    TEST_ASSERT_EQUAL_MEMORY(writeData, readBuf, 128);
}

/* 5. Write and read 256-byte block */
static void test_Integration_WriteRead_Block4(void)
{
    uint8 writeData[256], readBuf[256];
    memset(writeData, 0xDD, 256);

    DriveInitToCompletion();
    Fee_Write(4u, writeData);
    DriveJobToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());

    Fee_Read(4u, 0u, readBuf, 256u);
    DriveJobToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
    TEST_ASSERT_EQUAL_MEMORY(writeData, readBuf, 256);
}

/* 6. Write and read 512-byte (largest) block */
static void test_Integration_WriteRead_Block7(void)
{
    uint8 writeData[512], readBuf[512];
    uint32 i;
    for (i = 0; i < 512; i++) { writeData[i] = (uint8)(i & 0xFFu); }

    DriveInitToCompletion();
    Fee_Write(7u, writeData);
    DriveJobToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());

    Fee_Read(7u, 0u, readBuf, 512u);
    DriveJobToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
    TEST_ASSERT_EQUAL_MEMORY(writeData, readBuf, 512);
}

/* 7. Write immediate block */
static void test_Integration_WriteRead_ImmediateBlock5(void)
{
    uint8 writeData[16], readBuf[16];
    memset(writeData, 0xEE, 16);

    DriveInitToCompletion();
    Fee_Write(5u, writeData);
    DriveJobToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());

    Fee_Read(5u, 0u, readBuf, 16u);
    DriveJobToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
    TEST_ASSERT_EQUAL_MEMORY(writeData, readBuf, 16);
}

/* 8. Multiple blocks written then read */
static void test_Integration_MultipleBlocks_WriteRead(void)
{
    uint8 d1[32], d2[64], d5[16];
    uint8 r1[32], r2[64], r5[16];
    memset(d1, 0x11, 32);
    memset(d2, 0x22, 64);
    memset(d5, 0x55, 16);

    DriveInitToCompletion();

    Fee_Write(1u, d1); DriveJobToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());

    Fee_Write(2u, d2); DriveJobToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());

    Fee_Write(5u, d5); DriveJobToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());

    Fee_Read(1u, 0u, r1, 32u); DriveJobToCompletion();
    TEST_ASSERT_EQUAL_MEMORY(d1, r1, 32);

    Fee_Read(2u, 0u, r2, 64u); DriveJobToCompletion();
    TEST_ASSERT_EQUAL_MEMORY(d2, r2, 64);

    Fee_Read(5u, 0u, r5, 16u); DriveJobToCompletion();
    TEST_ASSERT_EQUAL_MEMORY(d5, r5, 16);
}

/* 9. Overwrite block and verify new data */
static void test_Integration_Overwrite_Block1(void)
{
    uint8 d1[32], d2[32], readBuf[32];
    memset(d1, 0xAA, 32);
    memset(d2, 0xBB, 32);

    DriveInitToCompletion();
    Fee_Write(1u, d1); DriveJobToCompletion();
    Fee_Write(1u, d2); DriveJobToCompletion();

    Fee_Read(1u, 0u, readBuf, 32u); DriveJobToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
    TEST_ASSERT_EQUAL_MEMORY(d2, readBuf, 32);
}

/* 10. Invalidate block, read returns BLOCK_INVALID */
static void test_Integration_Invalidate(void)
{
    uint8 d[32], r[32];
    memset(d, 0xAA, 32);

    DriveInitToCompletion();
    Fee_Write(1u, d); DriveJobToCompletion();

    Fee_InvalidateBlock(1u); DriveJobToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());

    Fee_Read(1u, 0u, r, 32u); DriveJobToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_BLOCK_INVALID, Fee_GetJobResult());
}

/* 11. Partial read with offset */
static void test_Integration_PartialRead(void)
{
    uint8 d[32], r[16];
    uint32 i;
    for (i = 0; i < 32; i++) { d[i] = (uint8)i; }

    DriveInitToCompletion();
    Fee_Write(1u, d); DriveJobToCompletion();

    Fee_Read(1u, 8u, r, 16u); DriveJobToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
    TEST_ASSERT_EQUAL_MEMORY(&d[8], r, 16);
}

/* 12. Read non-existent block returns BLOCK_INVALID */
static void test_Integration_ReadNonExistent(void)
{
    uint8 r[32];
    DriveInitToCompletion();
    Fee_Read(1u, 0u, r, 32u); DriveJobToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_BLOCK_INVALID, Fee_GetJobResult());
}

/* 13. NvM end notification on successful write */
static void test_Integration_NvMEndNotification_Write(void)
{
    uint8 d[32];
    memset(d, 0xAA, 32);

    DriveInitToCompletion();
    NvM_Cbk_Stub_Reset();

    Fee_Write(1u, d); DriveJobToCompletion();
    TEST_ASSERT_EQUAL(1u, NvM_Cbk_Stub_GetEndCount());
    TEST_ASSERT_EQUAL(0u, NvM_Cbk_Stub_GetErrorCount());
}

/* 14. NvM end notification on successful read */
static void test_Integration_NvMEndNotification_Read(void)
{
    uint8 d[32], r[32];
    memset(d, 0xAA, 32);

    DriveInitToCompletion();
    Fee_Write(1u, d); DriveJobToCompletion();
    NvM_Cbk_Stub_Reset();

    Fee_Read(1u, 0u, r, 32u); DriveJobToCompletion();
    TEST_ASSERT_EQUAL(1u, NvM_Cbk_Stub_GetEndCount());
}

/* 15. Cancel in-progress write */
static void test_Integration_Cancel_Write(void)
{
    uint8 d[32];
    memset(d, 0xAA, 32);

    DriveInitToCompletion();
    Fee_Write(1u, d);
    DriveOneCycle();
    Fee_Cancel();
    TEST_ASSERT_EQUAL(MEMIF_IDLE, Fee_GetStatus());
    TEST_ASSERT_EQUAL(MEMIF_JOB_CANCELED, Fee_GetJobResult());
}

/* 16. Erase immediate block */
static void test_Integration_EraseImmediate(void)
{
    uint8 d[16];
    memset(d, 0xAA, 16);

    DriveInitToCompletion();
    Fee_Write(5u, d); DriveJobToCompletion();

    Fee_EraseImmediateBlock(5u); DriveJobToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
}

/* 17. Write all 8 blocks */
static void test_Integration_WriteAll8Blocks(void)
{
    uint8 data[512];
    uint16 blockSizes[] = {32, 64, 128, 256, 16, 8, 512, 64};
    uint16 blockNums[]  = {1, 2, 3, 4, 5, 6, 7, 8};
    uint32 i;

    DriveInitToCompletion();

    for (i = 0; i < 8u; i++)
    {
        memset(data, (uint8)(0x10u + i), blockSizes[i]);
        Fee_Write(blockNums[i], data);
        DriveJobToCompletion();
        TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
    }

    /* Verify all blocks */
    for (i = 0; i < 8u; i++)
    {
        uint8 readBuf[512];
        memset(data, (uint8)(0x10u + i), blockSizes[i]);
        Fee_Read(blockNums[i], 0u, readBuf, blockSizes[i]);
        DriveJobToCompletion();
        TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
        TEST_ASSERT_EQUAL_MEMORY(data, readBuf, blockSizes[i]);
    }
}

/* 18. GetStatus transitions during write */
static void test_Integration_StatusTransitions(void)
{
    uint8 d[32];
    memset(d, 0xAA, 32);

    DriveInitToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_IDLE, Fee_GetStatus());

    Fee_Write(1u, d);
    TEST_ASSERT_EQUAL(MEMIF_BUSY, Fee_GetStatus());

    DriveJobToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_IDLE, Fee_GetStatus());
}

/* 19. GetVersionInfo through full stack */
static void test_Integration_GetVersionInfo(void)
{
    Std_VersionInfoType vInfo;
    Fee_GetVersionInfo(&vInfo);
    TEST_ASSERT_EQUAL_UINT16(FEE_MODULE_ID, vInfo.moduleID);
    TEST_ASSERT_EQUAL_UINT8(FEE_SW_MAJOR_VERSION, vInfo.sw_major_version);
}

/* 20. Write after invalidate writes new valid copy */
static void test_Integration_WriteAfterInvalidate(void)
{
    uint8 d1[32], d2[32], r[32];
    memset(d1, 0x11, 32);
    memset(d2, 0x22, 32);

    DriveInitToCompletion();
    Fee_Write(1u, d1); DriveJobToCompletion();
    Fee_InvalidateBlock(1u); DriveJobToCompletion();
    Fee_Write(1u, d2); DriveJobToCompletion();

    Fee_Read(1u, 0u, r, 32u); DriveJobToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
    TEST_ASSERT_EQUAL_MEMORY(d2, r, 32);
}

/* 21. Multiple overwrites of same block */
static void test_Integration_MultipleOverwrites(void)
{
    uint8 d[32], r[32];
    uint32 i;

    DriveInitToCompletion();

    for (i = 0; i < 5u; i++)
    {
        memset(d, (uint8)(0x10u + i), 32);
        Fee_Write(1u, d);
        DriveJobToCompletion();
        TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
    }

    memset(d, (uint8)(0x10u + 4u), 32); /* Last value */
    Fee_Read(1u, 0u, r, 32u);
    DriveJobToCompletion();
    TEST_ASSERT_EQUAL_MEMORY(d, r, 32);
}

/*============================================================================*
 *  Main
 *============================================================================*/

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_Integration_Init_BlankFlash);
    RUN_TEST(test_Integration_WriteRead_Block1);
    RUN_TEST(test_Integration_WriteRead_Block2);
    RUN_TEST(test_Integration_WriteRead_Block3);
    RUN_TEST(test_Integration_WriteRead_Block4);
    RUN_TEST(test_Integration_WriteRead_Block7);
    RUN_TEST(test_Integration_WriteRead_ImmediateBlock5);
    RUN_TEST(test_Integration_MultipleBlocks_WriteRead);
    RUN_TEST(test_Integration_Overwrite_Block1);
    RUN_TEST(test_Integration_Invalidate);
    RUN_TEST(test_Integration_PartialRead);
    RUN_TEST(test_Integration_ReadNonExistent);
    RUN_TEST(test_Integration_NvMEndNotification_Write);
    RUN_TEST(test_Integration_NvMEndNotification_Read);
    RUN_TEST(test_Integration_Cancel_Write);
    RUN_TEST(test_Integration_EraseImmediate);
    RUN_TEST(test_Integration_WriteAll8Blocks);
    RUN_TEST(test_Integration_StatusTransitions);
    RUN_TEST(test_Integration_GetVersionInfo);
    RUN_TEST(test_Integration_WriteAfterInvalidate);
    RUN_TEST(test_Integration_MultipleOverwrites);

    return UNITY_END();
}
