/**
 * \file       test_Fee_Api.c
 * \brief      Unit Tests -- Fee Public API Layer
 *
 * \details    Tests all DET checks for all 11 public APIs, version info,
 *             init sequence, read/write/cancel/invalidate/erase acceptance,
 *             busy rejection, immediate acceptance during GC, normal
 *             rejection during GC, block number validation, offset+length
 *             validation, SetMode no-op.
 *
 * \copyright  Copyright (c) 2024 -- All rights reserved.
 */

/*============================================================================*
 *  Includes
 *============================================================================*/

#include "unity/unity.h"
#include "Fee.h"
#include "Fee_StateMachine.h"
#include "Fee_Safety.h"
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
 *  Helper: drive init to completion
 *============================================================================*/

static void DriveInitToCompletion(void)
{
    uint32 maxCycles = 200u;
    uint32 cycle;

    Fee_Init(&Fee_Config);

    for (cycle = 0u; cycle < maxCycles; cycle++)
    {
        MemAcc_MainFunction();
        Fee_MainFunction();

        if (Fee_GetStatus() == MEMIF_IDLE)
        {
            break;
        }
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

    /* Reset Fee state to UNINIT by directly setting the volatile variables */
    Fee_ModuleStatus = MEMIF_UNINIT;
    Fee_InternalState = FEE_STATE_UNINIT;
    Fee_LastJobResult = MEMIF_JOB_OK;
    Fee_ConfigPtr = NULL_PTR;
    Fee_CurrentJob.Type = FEE_JOB_NONE;

    /* Initialize MemAcc first */
    MemAcc_Init(&MemAcc_Config);
}

void tearDown(void)
{
    /* Nothing */
}

/*============================================================================*
 *  1. Fee_Init with NULL config reports DET
 *============================================================================*/

static void test_Init_NullConfig(void)
{
    Fee_Init(NULL_PTR);
    TEST_ASSERT_TRUE(Det_Stub_WasErrorReported(FEE_MODULE_ID, FEE_SID_INIT,
                                                FEE_E_PARAM_POINTER));
}

/*============================================================================*
 *  2. Fee_Init with valid config sets BUSY_INTERNAL
 *============================================================================*/

static void test_Init_ValidConfig(void)
{
    Fee_Init(&Fee_Config);
    TEST_ASSERT_EQUAL(MEMIF_BUSY_INTERNAL, Fee_GetStatus());
}

/*============================================================================*
 *  3. Fee_Read before init reports DET
 *============================================================================*/

static void test_Read_Uninit(void)
{
    uint8 buf[32];
    Std_ReturnType ret;

    /* Fee_ModuleStatus is MEMIF_UNINIT at start */
    ret = Fee_Read(1u, 0u, buf, 32u);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_TRUE(Det_Stub_WasErrorReported(FEE_MODULE_ID, FEE_SID_READ,
                                                FEE_E_UNINIT));
}

/*============================================================================*
 *  4. Fee_Read with invalid block number
 *============================================================================*/

static void test_Read_InvalidBlockNumber(void)
{
    uint8 buf[32];
    Std_ReturnType ret;

    DriveInitToCompletion();
    Det_Stub_Reset();

    ret = Fee_Read(99u, 0u, buf, 32u);  /* Block 99 does not exist */
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_TRUE(Det_Stub_WasErrorReported(FEE_MODULE_ID, FEE_SID_READ,
                                                FEE_E_INVALID_BLOCK_NO));
}

/*============================================================================*
 *  5. Fee_Read with NULL pointer
 *============================================================================*/

static void test_Read_NullPointer(void)
{
    Std_ReturnType ret;

    DriveInitToCompletion();
    Det_Stub_Reset();

    ret = Fee_Read(1u, 0u, NULL_PTR, 32u);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_TRUE(Det_Stub_WasErrorReported(FEE_MODULE_ID, FEE_SID_READ,
                                                FEE_E_PARAM_POINTER));
}

/*============================================================================*
 *  6. Fee_Read with length == 0
 *============================================================================*/

static void test_Read_ZeroLength(void)
{
    uint8 buf[32];
    Std_ReturnType ret;

    DriveInitToCompletion();
    Det_Stub_Reset();

    ret = Fee_Read(1u, 0u, buf, 0u);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_TRUE(Det_Stub_WasErrorReported(FEE_MODULE_ID, FEE_SID_READ,
                                                FEE_E_INVALID_BLOCK_LEN));
}

/*============================================================================*
 *  7. Fee_Read with offset+length > blockSize
 *============================================================================*/

static void test_Read_InvalidOffset(void)
{
    uint8 buf[32];
    Std_ReturnType ret;

    DriveInitToCompletion();
    Det_Stub_Reset();

    /* Block 1 is 32 bytes. offset=10, length=30 -> 40 > 32 */
    ret = Fee_Read(1u, 10u, buf, 30u);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_TRUE(Det_Stub_WasErrorReported(FEE_MODULE_ID, FEE_SID_READ,
                                                FEE_E_INVALID_BLOCK_OFS));
}

/*============================================================================*
 *  8. Fee_Read acceptance when idle
 *============================================================================*/

static void test_Read_AcceptWhenIdle(void)
{
    uint8 buf[32];
    Std_ReturnType ret;

    DriveInitToCompletion();
    Det_Stub_Reset();

    ret = Fee_Read(1u, 0u, buf, 32u);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(MEMIF_BUSY, Fee_GetStatus());
}

/*============================================================================*
 *  9. Fee_Read rejection when busy
 *============================================================================*/

static void test_Read_RejectionWhenBusy(void)
{
    uint8 buf1[32], buf2[32];
    Std_ReturnType ret;

    DriveInitToCompletion();
    Det_Stub_Reset();

    /* First read accepted */
    ret = Fee_Read(1u, 0u, buf1, 32u);
    TEST_ASSERT_EQUAL(E_OK, ret);

    /* Second read rejected */
    ret = Fee_Read(2u, 0u, buf2, 32u);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_TRUE(Det_Stub_WasErrorReported(FEE_MODULE_ID, FEE_SID_READ,
                                                FEE_E_BUSY));
}

/*============================================================================*
 *  10. Fee_Write before init reports DET
 *============================================================================*/

static void test_Write_Uninit(void)
{
    uint8 data[32];
    Std_ReturnType ret;

    memset(data, 0xAA, 32);
    ret = Fee_Write(1u, data);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_TRUE(Det_Stub_WasErrorReported(FEE_MODULE_ID, FEE_SID_WRITE,
                                                FEE_E_UNINIT));
}

/*============================================================================*
 *  11. Fee_Write with invalid block number
 *============================================================================*/

static void test_Write_InvalidBlockNumber(void)
{
    uint8 data[32];
    Std_ReturnType ret;

    DriveInitToCompletion();
    Det_Stub_Reset();

    ret = Fee_Write(99u, data);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_TRUE(Det_Stub_WasErrorReported(FEE_MODULE_ID, FEE_SID_WRITE,
                                                FEE_E_INVALID_BLOCK_NO));
}

/*============================================================================*
 *  12. Fee_Write with NULL pointer
 *============================================================================*/

static void test_Write_NullPointer(void)
{
    Std_ReturnType ret;

    DriveInitToCompletion();
    Det_Stub_Reset();

    ret = Fee_Write(1u, NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_TRUE(Det_Stub_WasErrorReported(FEE_MODULE_ID, FEE_SID_WRITE,
                                                FEE_E_PARAM_POINTER));
}

/*============================================================================*
 *  13. Fee_Write acceptance when idle
 *============================================================================*/

static void test_Write_AcceptWhenIdle(void)
{
    uint8 data[32];
    Std_ReturnType ret;

    memset(data, 0xAA, 32);
    DriveInitToCompletion();
    Det_Stub_Reset();

    ret = Fee_Write(1u, data);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(MEMIF_BUSY, Fee_GetStatus());
}

/*============================================================================*
 *  14. Fee_Write rejection when busy
 *============================================================================*/

static void test_Write_RejectionWhenBusy(void)
{
    uint8 data1[32], data2[64];
    Std_ReturnType ret;

    memset(data1, 0xAA, 32);
    memset(data2, 0xBB, 64);
    DriveInitToCompletion();
    Det_Stub_Reset();

    ret = Fee_Write(1u, data1);
    TEST_ASSERT_EQUAL(E_OK, ret);

    ret = Fee_Write(2u, data2);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_TRUE(Det_Stub_WasErrorReported(FEE_MODULE_ID, FEE_SID_WRITE,
                                                FEE_E_BUSY));
}

/*============================================================================*
 *  15. Fee_Cancel before init reports DET
 *============================================================================*/

static void test_Cancel_Uninit(void)
{
    Fee_Cancel();
    TEST_ASSERT_TRUE(Det_Stub_WasErrorReported(FEE_MODULE_ID, FEE_SID_CANCEL,
                                                FEE_E_UNINIT));
}

/*============================================================================*
 *  16. Fee_Cancel after init
 *============================================================================*/

static void test_Cancel_AfterInit(void)
{
    uint8 data[32];

    memset(data, 0xAA, 32);
    DriveInitToCompletion();
    Det_Stub_Reset();

    Fee_Write(1u, data);
    Fee_Cancel();

    TEST_ASSERT_EQUAL(MEMIF_IDLE, Fee_GetStatus());
    TEST_ASSERT_EQUAL(MEMIF_JOB_CANCELED, Fee_GetJobResult());
}

/*============================================================================*
 *  17. Fee_GetStatus returns MEMIF_UNINIT when not initialized
 *============================================================================*/

static void test_GetStatus_Uninit(void)
{
    TEST_ASSERT_EQUAL(MEMIF_UNINIT, Fee_GetStatus());
}

/*============================================================================*
 *  18. Fee_GetStatus returns correct status
 *============================================================================*/

static void test_GetStatus_CorrectStatus(void)
{
    DriveInitToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_IDLE, Fee_GetStatus());
}

/*============================================================================*
 *  19. Fee_GetJobResult before init reports DET
 *============================================================================*/

static void test_GetJobResult_Uninit(void)
{
    MemIf_JobResultType result;

    result = Fee_GetJobResult();
    TEST_ASSERT_EQUAL(MEMIF_JOB_FAILED, result);
    TEST_ASSERT_TRUE(Det_Stub_WasErrorReported(FEE_MODULE_ID, FEE_SID_GET_JOB_RESULT,
                                                FEE_E_UNINIT));
}

/*============================================================================*
 *  20. Fee_GetJobResult returns correct result
 *============================================================================*/

static void test_GetJobResult_CorrectResult(void)
{
    DriveInitToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
}

/*============================================================================*
 *  21. Fee_InvalidateBlock before init
 *============================================================================*/

static void test_InvalidateBlock_Uninit(void)
{
    Std_ReturnType ret;

    ret = Fee_InvalidateBlock(1u);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_TRUE(Det_Stub_WasErrorReported(FEE_MODULE_ID,
                     FEE_SID_INVALIDATE_BLOCK, FEE_E_UNINIT));
}

/*============================================================================*
 *  22. Fee_InvalidateBlock with invalid block number
 *============================================================================*/

static void test_InvalidateBlock_InvalidBlockNo(void)
{
    Std_ReturnType ret;

    DriveInitToCompletion();
    Det_Stub_Reset();

    ret = Fee_InvalidateBlock(99u);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_TRUE(Det_Stub_WasErrorReported(FEE_MODULE_ID,
                     FEE_SID_INVALIDATE_BLOCK, FEE_E_INVALID_BLOCK_NO));
}

/*============================================================================*
 *  23. Fee_InvalidateBlock acceptance when idle
 *============================================================================*/

static void test_InvalidateBlock_AcceptWhenIdle(void)
{
    Std_ReturnType ret;

    DriveInitToCompletion();
    Det_Stub_Reset();

    ret = Fee_InvalidateBlock(1u);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(MEMIF_BUSY, Fee_GetStatus());
}

/*============================================================================*
 *  24. Fee_EraseImmediateBlock before init
 *============================================================================*/

static void test_EraseImmediateBlock_Uninit(void)
{
    Std_ReturnType ret;

    ret = Fee_EraseImmediateBlock(5u);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_TRUE(Det_Stub_WasErrorReported(FEE_MODULE_ID,
                     FEE_SID_ERASE_IMMEDIATE_BLOCK, FEE_E_UNINIT));
}

/*============================================================================*
 *  25. Fee_EraseImmediateBlock with invalid block number
 *============================================================================*/

static void test_EraseImmediateBlock_InvalidBlockNo(void)
{
    Std_ReturnType ret;

    DriveInitToCompletion();
    Det_Stub_Reset();

    ret = Fee_EraseImmediateBlock(99u);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_TRUE(Det_Stub_WasErrorReported(FEE_MODULE_ID,
                     FEE_SID_ERASE_IMMEDIATE_BLOCK, FEE_E_INVALID_BLOCK_NO));
}

/*============================================================================*
 *  26. Fee_EraseImmediateBlock on non-immediate block
 *============================================================================*/

static void test_EraseImmediateBlock_NotImmediate(void)
{
    Std_ReturnType ret;

    DriveInitToCompletion();
    Det_Stub_Reset();

    /* Block 1 is NOT immediate */
    ret = Fee_EraseImmediateBlock(1u);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_TRUE(Det_Stub_WasErrorReported(FEE_MODULE_ID,
                     FEE_SID_ERASE_IMMEDIATE_BLOCK, FEE_E_INVALID_BLOCK_NO));
}

/*============================================================================*
 *  27. Fee_EraseImmediateBlock acceptance when idle
 *============================================================================*/

static void test_EraseImmediateBlock_AcceptWhenIdle(void)
{
    Std_ReturnType ret;

    DriveInitToCompletion();
    Det_Stub_Reset();

    /* Block 5 is immediate */
    ret = Fee_EraseImmediateBlock(5u);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(MEMIF_BUSY, Fee_GetStatus());
}

/*============================================================================*
 *  28. Fee_GetVersionInfo with NULL pointer
 *============================================================================*/

static void test_GetVersionInfo_NullPointer(void)
{
    Fee_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(Det_Stub_WasErrorReported(FEE_MODULE_ID,
                     FEE_SID_GET_VERSION_INFO, FEE_E_PARAM_POINTER));
}

/*============================================================================*
 *  29. Fee_GetVersionInfo fills correct values
 *============================================================================*/

static void test_GetVersionInfo_CorrectValues(void)
{
    Std_VersionInfoType vInfo;

    Fee_GetVersionInfo(&vInfo);
    TEST_ASSERT_EQUAL_UINT16(FEE_VENDOR_ID, vInfo.vendorID);
    TEST_ASSERT_EQUAL_UINT16(FEE_MODULE_ID, vInfo.moduleID);
    TEST_ASSERT_EQUAL_UINT8(FEE_SW_MAJOR_VERSION, vInfo.sw_major_version);
    TEST_ASSERT_EQUAL_UINT8(FEE_SW_MINOR_VERSION, vInfo.sw_minor_version);
    TEST_ASSERT_EQUAL_UINT8(FEE_SW_PATCH_VERSION, vInfo.sw_patch_version);
}

/*============================================================================*
 *  30. Fee_SetMode is no-op
 *============================================================================*/

static void test_SetMode_NoOp(void)
{
    DriveInitToCompletion();
    Det_Stub_Reset();

    Fee_SetMode(MEMIF_MODE_FAST);
    Fee_SetMode(MEMIF_MODE_SLOW);

    /* Status should still be IDLE -- no errors */
    TEST_ASSERT_EQUAL(MEMIF_IDLE, Fee_GetStatus());
    TEST_ASSERT_EQUAL(0u, Det_Stub_GetErrorCount());
}

/*============================================================================*
 *  31. Fee_MainFunction before init reports runtime error
 *============================================================================*/

static void test_MainFunction_Uninit(void)
{
    Fee_MainFunction();
    TEST_ASSERT_TRUE(Det_Stub_WasRuntimeErrorReported(FEE_MODULE_ID,
                     FEE_SID_MAIN_FUNCTION, FEE_E_UNINIT));
}

/*============================================================================*
 *  32. Immediate write acceptance during GC (BUSY_INTERNAL)
 *============================================================================*/

static void test_ImmediateWrite_DuringGC(void)
{
    uint8 data[16];
    Std_ReturnType ret;

    memset(data, 0xCC, 16);
    DriveInitToCompletion();
    Det_Stub_Reset();

    /* Force module into BUSY_INTERNAL (simulate GC in progress) */
    Fee_ModuleStatus = MEMIF_BUSY_INTERNAL;
    Fee_InternalState = FEE_STATE_GC_ACTIVE;

    /* Block 5 is immediate -- should be accepted */
    ret = Fee_Write(5u, data);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(MEMIF_BUSY, Fee_GetStatus());
}

/*============================================================================*
 *  33. Normal write rejection during GC (BUSY_INTERNAL)
 *============================================================================*/

static void test_NormalWrite_DuringGC(void)
{
    uint8 data[32];
    Std_ReturnType ret;

    memset(data, 0xDD, 32);
    DriveInitToCompletion();
    Det_Stub_Reset();

    /* Force module into BUSY_INTERNAL */
    Fee_ModuleStatus = MEMIF_BUSY_INTERNAL;
    Fee_InternalState = FEE_STATE_GC_ACTIVE;

    /* Block 1 is NOT immediate -- should be rejected */
    ret = Fee_Write(1u, data);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/*============================================================================*
 *  34. Fee_Init drives to IDLE after completion
 *============================================================================*/

static void test_Init_DrivesToIdle(void)
{
    DriveInitToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_IDLE, Fee_GetStatus());
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
}

/*============================================================================*
 *  35. Fee_Read with valid offset and partial length
 *============================================================================*/

static void test_Read_ValidPartialRead(void)
{
    uint8 buf[16];
    Std_ReturnType ret;

    DriveInitToCompletion();
    Det_Stub_Reset();

    /* Block 1 is 32 bytes. offset=8, length=16 -> 24 <= 32 */
    ret = Fee_Read(1u, 8u, buf, 16u);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

/*============================================================================*
 *  Main
 *============================================================================*/

int main(void)
{
    UNITY_BEGIN();

    /* Init tests */
    RUN_TEST(test_Init_NullConfig);
    RUN_TEST(test_Init_ValidConfig);
    RUN_TEST(test_Init_DrivesToIdle);

    /* Read DET tests */
    RUN_TEST(test_Read_Uninit);
    RUN_TEST(test_Read_InvalidBlockNumber);
    RUN_TEST(test_Read_NullPointer);
    RUN_TEST(test_Read_ZeroLength);
    RUN_TEST(test_Read_InvalidOffset);
    RUN_TEST(test_Read_AcceptWhenIdle);
    RUN_TEST(test_Read_RejectionWhenBusy);
    RUN_TEST(test_Read_ValidPartialRead);

    /* Write DET tests */
    RUN_TEST(test_Write_Uninit);
    RUN_TEST(test_Write_InvalidBlockNumber);
    RUN_TEST(test_Write_NullPointer);
    RUN_TEST(test_Write_AcceptWhenIdle);
    RUN_TEST(test_Write_RejectionWhenBusy);

    /* Cancel tests */
    RUN_TEST(test_Cancel_Uninit);
    RUN_TEST(test_Cancel_AfterInit);

    /* GetStatus tests */
    RUN_TEST(test_GetStatus_Uninit);
    RUN_TEST(test_GetStatus_CorrectStatus);

    /* GetJobResult tests */
    RUN_TEST(test_GetJobResult_Uninit);
    RUN_TEST(test_GetJobResult_CorrectResult);

    /* InvalidateBlock tests */
    RUN_TEST(test_InvalidateBlock_Uninit);
    RUN_TEST(test_InvalidateBlock_InvalidBlockNo);
    RUN_TEST(test_InvalidateBlock_AcceptWhenIdle);

    /* EraseImmediateBlock tests */
    RUN_TEST(test_EraseImmediateBlock_Uninit);
    RUN_TEST(test_EraseImmediateBlock_InvalidBlockNo);
    RUN_TEST(test_EraseImmediateBlock_NotImmediate);
    RUN_TEST(test_EraseImmediateBlock_AcceptWhenIdle);

    /* GetVersionInfo tests */
    RUN_TEST(test_GetVersionInfo_NullPointer);
    RUN_TEST(test_GetVersionInfo_CorrectValues);

    /* SetMode test */
    RUN_TEST(test_SetMode_NoOp);

    /* MainFunction tests */
    RUN_TEST(test_MainFunction_Uninit);

    /* GC interaction tests */
    RUN_TEST(test_ImmediateWrite_DuringGC);
    RUN_TEST(test_NormalWrite_DuringGC);

    return UNITY_END();
}
