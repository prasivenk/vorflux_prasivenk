/**
 * \file       test_MemAcc_JobProcessing.c
 * \brief      Unit Tests -- MemAcc Job Processing and MainFunction
 *
 * \details    Tests MainFunction processing for each job type, ECC error
 *             propagation (corrected and uncorrected), DEM reporting,
 *             cancel during processing, ProcessedLength tracking.
 *
 * \copyright  Copyright (c) 2024 -- All rights reserved.
 */

/*============================================================================*
 *  Includes
 *============================================================================*/

#include "unity/unity.h"
#include "MemAcc.h"
#include "MemAcc_Types.h"
#include "MemAcc_Cfg.h"
#include "Det_Stub.h"
#include "Dem_Stub.h"
#include "SchM_Stub.h"
#include "Mem_DFLS_Stub.h"

/*============================================================================*
 *  Test data
 *============================================================================*/

static uint8 ReadBuffer[256];
static const uint8 WriteData[64] = {
    0xAAu, 0xBBu, 0xCCu, 0xDDu, 0x11u, 0x22u, 0x33u, 0x44u,
    0xAAu, 0xBBu, 0xCCu, 0xDDu, 0x11u, 0x22u, 0x33u, 0x44u,
    0xAAu, 0xBBu, 0xCCu, 0xDDu, 0x11u, 0x22u, 0x33u, 0x44u,
    0xAAu, 0xBBu, 0xCCu, 0xDDu, 0x11u, 0x22u, 0x33u, 0x44u,
    0xAAu, 0xBBu, 0xCCu, 0xDDu, 0x11u, 0x22u, 0x33u, 0x44u,
    0xAAu, 0xBBu, 0xCCu, 0xDDu, 0x11u, 0x22u, 0x33u, 0x44u,
    0xAAu, 0xBBu, 0xCCu, 0xDDu, 0x11u, 0x22u, 0x33u, 0x44u,
    0xAAu, 0xBBu, 0xCCu, 0xDDu, 0x11u, 0x22u, 0x33u, 0x44u
};

static const uint8 ZeroData[32] = {0u};

/*============================================================================*
 *  Setup / Teardown
 *============================================================================*/

void setUp(void)
{
    Det_Stub_Reset();
    Dem_Stub_Reset();
    SchM_Stub_Reset();
    Mem_DFLS_Stub_Reset();
    MemAcc_DeInit();
    Det_Stub_Reset();
    MemAcc_Init(&MemAcc_Config);
}

void tearDown(void)
{
    MemAcc_DeInit();
}

/*============================================================================*
 *  Test: MainFunction processes read job to completion
 *============================================================================*/

static void test_MainFunction_ReadComplete(void)
{
    Std_ReturnType ret;

    ret = MemAcc_Read(0u, 0u, ReadBuffer, 32u);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(MEMACC_JOB_PENDING, MemAcc_GetJobResult(0u));

    /* MainFunction drives completion */
    MemAcc_MainFunction();
    TEST_ASSERT_EQUAL(MEMACC_JOB_OK, MemAcc_GetJobResult(0u));
    TEST_ASSERT_EQUAL(FALSE, MemAcc_AreaBusy[0u]);
    TEST_ASSERT_EQUAL(32u, MemAcc_GetProcessedLength(0u));
}

/*============================================================================*
 *  Test: MainFunction processes write job to completion
 *============================================================================*/

static void test_MainFunction_WriteComplete(void)
{
    Std_ReturnType ret;

    ret = MemAcc_Write(0u, 0u, WriteData, 32u);
    TEST_ASSERT_EQUAL(E_OK, ret);

    MemAcc_MainFunction();
    TEST_ASSERT_EQUAL(MEMACC_JOB_OK, MemAcc_GetJobResult(0u));
    TEST_ASSERT_EQUAL(FALSE, MemAcc_AreaBusy[0u]);
    TEST_ASSERT_EQUAL(32u, MemAcc_GetProcessedLength(0u));
}

/*============================================================================*
 *  Test: MainFunction processes erase job to completion
 *============================================================================*/

static void test_MainFunction_EraseComplete(void)
{
    Std_ReturnType ret;

    ret = MemAcc_Erase(0u, 0u, 4096u);
    TEST_ASSERT_EQUAL(E_OK, ret);

    MemAcc_MainFunction();
    TEST_ASSERT_EQUAL(MEMACC_JOB_OK, MemAcc_GetJobResult(0u));
    TEST_ASSERT_EQUAL(FALSE, MemAcc_AreaBusy[0u]);
    TEST_ASSERT_EQUAL(4096u, MemAcc_GetProcessedLength(0u));
}

/*============================================================================*
 *  Test: MainFunction processes blankcheck job to completion
 *============================================================================*/

static void test_MainFunction_BlankCheckComplete(void)
{
    Std_ReturnType ret;

    /* Flash is erased (0x00), so blank check should succeed */
    ret = MemAcc_BlankCheck(0u, 0u, 4096u);
    TEST_ASSERT_EQUAL(E_OK, ret);

    MemAcc_MainFunction();
    TEST_ASSERT_EQUAL(MEMACC_JOB_OK, MemAcc_GetJobResult(0u));
    TEST_ASSERT_EQUAL(FALSE, MemAcc_AreaBusy[0u]);
}

/*============================================================================*
 *  Test: BlankCheck fails on written data
 *============================================================================*/

static void test_MainFunction_BlankCheckFails(void)
{
    Std_ReturnType ret;

    /* Write some data first */
    ret = MemAcc_Write(0u, 0u, WriteData, 32u);
    TEST_ASSERT_EQUAL(E_OK, ret);
    MemAcc_MainFunction();
    TEST_ASSERT_EQUAL(MEMACC_JOB_OK, MemAcc_GetJobResult(0u));

    /* Now blank check should fail */
    ret = MemAcc_BlankCheck(0u, 0u, 32u);
    TEST_ASSERT_EQUAL(E_OK, ret);
    MemAcc_MainFunction();
    TEST_ASSERT_EQUAL(MEMACC_JOB_FAILED, MemAcc_GetJobResult(0u));
}

/*============================================================================*
 *  Test: Compare job - matching data
 *============================================================================*/

static void test_MainFunction_CompareMatch(void)
{
    Std_ReturnType ret;

    /* Flash is erased (0x00), compare with zeros should match */
    ret = MemAcc_Compare(0u, 0u, ZeroData, 32u);
    TEST_ASSERT_EQUAL(E_OK, ret);

    MemAcc_MainFunction();
    TEST_ASSERT_EQUAL(MEMACC_JOB_OK, MemAcc_GetJobResult(0u));
    TEST_ASSERT_EQUAL(32u, MemAcc_GetProcessedLength(0u));
}

/*============================================================================*
 *  Test: Compare job - mismatching data
 *============================================================================*/

static void test_MainFunction_CompareMismatch(void)
{
    Std_ReturnType ret;

    /* Flash is erased (0x00), compare with non-zero should fail */
    ret = MemAcc_Compare(0u, 0u, WriteData, 32u);
    TEST_ASSERT_EQUAL(E_OK, ret);

    MemAcc_MainFunction();
    TEST_ASSERT_EQUAL(MEMACC_JOB_FAILED, MemAcc_GetJobResult(0u));
}

/*============================================================================*
 *  Test: ECC corrected error propagation on read
 *============================================================================*/

static void test_MainFunction_EccCorrected(void)
{
    Std_ReturnType ret;

    /* Inject correctable ECC at physical address 0xAF000000 */
    Mem_DFLS_Stub_InjectEccError(0xAF000000u, 32u, FALSE);

    ret = MemAcc_Read(0u, 0u, ReadBuffer, 32u);
    TEST_ASSERT_EQUAL(E_OK, ret);

    MemAcc_MainFunction();
    TEST_ASSERT_EQUAL(MEMACC_JOB_ECC_CORRECTED, MemAcc_GetJobResult(0u));
    TEST_ASSERT_EQUAL(FALSE, MemAcc_AreaBusy[0u]);
}

/*============================================================================*
 *  Test: ECC uncorrected error propagation on read
 *============================================================================*/

static void test_MainFunction_EccUncorrected(void)
{
    Std_ReturnType ret;

    /* Inject uncorrectable ECC at physical address 0xAF000000 */
    Mem_DFLS_Stub_InjectEccError(0xAF000000u, 32u, TRUE);

    ret = MemAcc_Read(0u, 0u, ReadBuffer, 32u);
    TEST_ASSERT_EQUAL(E_OK, ret);

    MemAcc_MainFunction();
    TEST_ASSERT_EQUAL(MEMACC_JOB_ECC_UNCORRECTED, MemAcc_GetJobResult(0u));
    TEST_ASSERT_EQUAL(FALSE, MemAcc_AreaBusy[0u]);

    /* Should report DEM event */
    TEST_ASSERT_TRUE(Dem_Stub_WasEventReported(MEMACC_E_HARDWARE_ERROR,
                     DEM_EVENT_STATUS_FAILED));
}

/*============================================================================*
 *  Test: DEM reporting on driver failure
 *============================================================================*/

static void test_MainFunction_DriverFailure_DemReport(void)
{
    Std_ReturnType ret;

    ret = MemAcc_Read(0u, 0u, ReadBuffer, 32u);
    TEST_ASSERT_EQUAL(E_OK, ret);

    /* Force driver to report failure */
    Mem_DFLS_Stub_SetJobResult(MEM_DFLS_JOB_FAILED);

    MemAcc_MainFunction();
    TEST_ASSERT_EQUAL(MEMACC_JOB_FAILED, MemAcc_GetJobResult(0u));

    /* Should report DEM event */
    TEST_ASSERT_TRUE(Dem_Stub_WasEventReported(MEMACC_E_HARDWARE_ERROR,
                     DEM_EVENT_STATUS_FAILED));
}

/*============================================================================*
 *  Test: Cancel during processing
 *============================================================================*/

static void test_CancelDuringProcessing(void)
{
    Std_ReturnType ret;

    ret = MemAcc_Read(0u, 0u, ReadBuffer, 32u);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(TRUE, MemAcc_AreaBusy[0u]);

    /* Cancel before MainFunction processes */
    MemAcc_Cancel(0u);
    TEST_ASSERT_EQUAL(MEMACC_JOB_CANCELED, MemAcc_GetJobResult(0u));
    TEST_ASSERT_EQUAL(FALSE, MemAcc_AreaBusy[0u]);

    /* MainFunction should not process cancelled job */
    MemAcc_MainFunction();
    TEST_ASSERT_EQUAL(MEMACC_JOB_CANCELED, MemAcc_GetJobResult(0u));
}

/*============================================================================*
 *  Test: ProcessedLength tracking on read
 *============================================================================*/

static void test_ProcessedLength_Read(void)
{
    Std_ReturnType ret;

    ret = MemAcc_Read(0u, 0u, ReadBuffer, 64u);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(0u, MemAcc_GetProcessedLength(0u));

    MemAcc_MainFunction();
    TEST_ASSERT_EQUAL(64u, MemAcc_GetProcessedLength(0u));
}

/*============================================================================*
 *  Test: ProcessedLength tracking on write
 *============================================================================*/

static void test_ProcessedLength_Write(void)
{
    Std_ReturnType ret;

    ret = MemAcc_Write(0u, 0u, WriteData, 64u);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(0u, MemAcc_GetProcessedLength(0u));

    MemAcc_MainFunction();
    TEST_ASSERT_EQUAL(64u, MemAcc_GetProcessedLength(0u));
}

/*============================================================================*
 *  Test: MainFunction when uninit does nothing
 *============================================================================*/

static void test_MainFunction_Uninit(void)
{
    MemAcc_DeInit();
    /* Should not crash or process anything */
    MemAcc_MainFunction();
    /* Just verify no crash */
    TEST_ASSERT_EQUAL(MEMACC_UNINIT, MemAcc_ModuleStatus);
}

/*============================================================================*
 *  Test: Write and verify flash content via stub
 *============================================================================*/

static void test_WriteAndVerifyFlashContent(void)
{
    Std_ReturnType ret;
    uint8 *flash;

    ret = MemAcc_Write(0u, 0u, WriteData, 32u);
    TEST_ASSERT_EQUAL(E_OK, ret);
    MemAcc_MainFunction();
    TEST_ASSERT_EQUAL(MEMACC_JOB_OK, MemAcc_GetJobResult(0u));

    /* Verify flash content directly */
    flash = Mem_DFLS_Stub_GetFlashContent();
    TEST_ASSERT_EQUAL_UINT8(0xAAu, flash[0]);
    TEST_ASSERT_EQUAL_UINT8(0xBBu, flash[1]);
}

/*============================================================================*
 *  Test: Compare after write - match
 *============================================================================*/

static void test_CompareAfterWrite_Match(void)
{
    Std_ReturnType ret;

    /* Write data */
    ret = MemAcc_Write(0u, 0u, WriteData, 32u);
    TEST_ASSERT_EQUAL(E_OK, ret);
    MemAcc_MainFunction();
    TEST_ASSERT_EQUAL(MEMACC_JOB_OK, MemAcc_GetJobResult(0u));

    /* Compare should match */
    ret = MemAcc_Compare(0u, 0u, WriteData, 32u);
    TEST_ASSERT_EQUAL(E_OK, ret);
    MemAcc_MainFunction();
    TEST_ASSERT_EQUAL(MEMACC_JOB_OK, MemAcc_GetJobResult(0u));
}

/*============================================================================*
 *  Test: ECC corrected during compare
 *============================================================================*/

static void test_Compare_EccCorrected(void)
{
    Std_ReturnType ret;

    Mem_DFLS_Stub_InjectEccError(0xAF000000u, 32u, FALSE);

    ret = MemAcc_Compare(0u, 0u, ZeroData, 32u);
    TEST_ASSERT_EQUAL(E_OK, ret);

    MemAcc_MainFunction();
    TEST_ASSERT_EQUAL(MEMACC_JOB_ECC_CORRECTED, MemAcc_GetJobResult(0u));
}

/*============================================================================*
 *  Test: ECC uncorrected during compare
 *============================================================================*/

static void test_Compare_EccUncorrected(void)
{
    Std_ReturnType ret;

    Mem_DFLS_Stub_InjectEccError(0xAF000000u, 32u, TRUE);

    ret = MemAcc_Compare(0u, 0u, ZeroData, 32u);
    TEST_ASSERT_EQUAL(E_OK, ret);

    MemAcc_MainFunction();
    TEST_ASSERT_EQUAL(MEMACC_JOB_ECC_UNCORRECTED, MemAcc_GetJobResult(0u));
    TEST_ASSERT_TRUE(Dem_Stub_WasEventReported(MEMACC_E_HARDWARE_ERROR,
                     DEM_EVENT_STATUS_FAILED));
}

/*============================================================================*
 *  Main
 *============================================================================*/

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_MainFunction_ReadComplete);
    RUN_TEST(test_MainFunction_WriteComplete);
    RUN_TEST(test_MainFunction_EraseComplete);
    RUN_TEST(test_MainFunction_BlankCheckComplete);
    RUN_TEST(test_MainFunction_BlankCheckFails);
    RUN_TEST(test_MainFunction_CompareMatch);
    RUN_TEST(test_MainFunction_CompareMismatch);
    RUN_TEST(test_MainFunction_EccCorrected);
    RUN_TEST(test_MainFunction_EccUncorrected);
    RUN_TEST(test_MainFunction_DriverFailure_DemReport);
    RUN_TEST(test_CancelDuringProcessing);
    RUN_TEST(test_ProcessedLength_Read);
    RUN_TEST(test_ProcessedLength_Write);
    RUN_TEST(test_MainFunction_Uninit);
    RUN_TEST(test_WriteAndVerifyFlashContent);
    RUN_TEST(test_CompareAfterWrite_Match);
    RUN_TEST(test_Compare_EccCorrected);
    RUN_TEST(test_Compare_EccUncorrected);

    return UNITY_END();
}
