/**
 * \file       test_MemAcc_Api.c
 * \brief      Unit Tests -- MemAcc Public API
 *
 * \details    Tests init/deinit, all DET checks, read/write/erase/blankcheck/
 *             compare acceptance, cancel, GetJobResult, GetProcessedLength,
 *             GetSegmentationInfo, RequestLock/ReleaseLock, GetVersionInfo,
 *             HwSpecificServiceRequest, busy rejection, locked rejection.
 *
 * \copyright  Copyright (c) 2024 -- All rights reserved.
 */

/*============================================================================*
 *  Includes
 *============================================================================*/

#include "unity/unity.h"
#include "MemAcc.h"
#include "MemAcc_Internal.h"
#include "MemAcc_Types.h"
#include "MemAcc_Cfg.h"
#include "Det_Stub.h"
#include "Dem_Stub.h"
#include "SchM_Stub.h"
#include "Mem_DFLS_Stub.h"

/*============================================================================*
 *  Test data
 *============================================================================*/

static uint8 TestBuffer[256];
static const uint8 TestWriteData[64] = {
    0xAAu, 0xBBu, 0xCCu, 0xDDu, 0x11u, 0x22u, 0x33u, 0x44u,
    0xAAu, 0xBBu, 0xCCu, 0xDDu, 0x11u, 0x22u, 0x33u, 0x44u,
    0xAAu, 0xBBu, 0xCCu, 0xDDu, 0x11u, 0x22u, 0x33u, 0x44u,
    0xAAu, 0xBBu, 0xCCu, 0xDDu, 0x11u, 0x22u, 0x33u, 0x44u,
    0xAAu, 0xBBu, 0xCCu, 0xDDu, 0x11u, 0x22u, 0x33u, 0x44u,
    0xAAu, 0xBBu, 0xCCu, 0xDDu, 0x11u, 0x22u, 0x33u, 0x44u,
    0xAAu, 0xBBu, 0xCCu, 0xDDu, 0x11u, 0x22u, 0x33u, 0x44u,
    0xAAu, 0xBBu, 0xCCu, 0xDDu, 0x11u, 0x22u, 0x33u, 0x44u
};

/*============================================================================*
 *  Setup / Teardown
 *============================================================================*/

void setUp(void)
{
    Det_Stub_Reset();
    Dem_Stub_Reset();
    SchM_Stub_Reset();
    Mem_DFLS_Stub_Reset();
    /* Ensure module is in uninit state */
    MemAcc_DeInit();
    Det_Stub_Reset(); /* Clear DET error from DeInit on uninit module */
}

void tearDown(void)
{
    /* Ensure clean state */
    MemAcc_DeInit();
}

/*============================================================================*
 *  Test: Init with valid config
 *============================================================================*/

static void test_Init_ValidConfig(void)
{
    MemAcc_Init(&MemAcc_Config);
    TEST_ASSERT_EQUAL(MEMACC_IDLE, MemAcc_ModuleStatus);
    TEST_ASSERT_EQUAL(0u, Det_Stub_GetErrorCount());
}

/*============================================================================*
 *  Test: Init with NULL pointer
 *============================================================================*/

static void test_Init_NullPointer(void)
{
    MemAcc_Init(NULL_PTR);
    TEST_ASSERT_EQUAL(MEMACC_UNINIT, MemAcc_ModuleStatus);
    TEST_ASSERT_TRUE(Det_Stub_WasErrorReported(MEMACC_MODULE_ID,
                     MEMACC_SID_INIT, MEMACC_E_PARAM_POINTER));
}

/*============================================================================*
 *  Test: DeInit after init
 *============================================================================*/

static void test_DeInit_AfterInit(void)
{
    MemAcc_Init(&MemAcc_Config);
    MemAcc_DeInit();
    TEST_ASSERT_EQUAL(MEMACC_UNINIT, MemAcc_ModuleStatus);
    TEST_ASSERT_EQUAL(0u, Det_Stub_GetErrorCount());
}

/*============================================================================*
 *  Test: DeInit before init reports DET
 *============================================================================*/

static void test_DeInit_BeforeInit(void)
{
    MemAcc_DeInit();
    TEST_ASSERT_TRUE(Det_Stub_WasErrorReported(MEMACC_MODULE_ID,
                     MEMACC_SID_DEINIT, MEMACC_E_UNINIT));
}

/*============================================================================*
 *  Test: Read before init reports DET UNINIT
 *============================================================================*/

static void test_Read_Uninit(void)
{
    Std_ReturnType ret;
    ret = MemAcc_Read(0u, 0u, TestBuffer, 32u);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_TRUE(Det_Stub_WasErrorReported(MEMACC_MODULE_ID,
                     MEMACC_SID_READ, MEMACC_E_UNINIT));
}

/*============================================================================*
 *  Test: Read with NULL pointer
 *============================================================================*/

static void test_Read_NullPointer(void)
{
    Std_ReturnType ret;
    MemAcc_Init(&MemAcc_Config);
    ret = MemAcc_Read(0u, 0u, NULL_PTR, 32u);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_TRUE(Det_Stub_WasErrorReported(MEMACC_MODULE_ID,
                     MEMACC_SID_READ, MEMACC_E_PARAM_POINTER));
}

/*============================================================================*
 *  Test: Read with invalid area ID
 *============================================================================*/

static void test_Read_InvalidAreaId(void)
{
    Std_ReturnType ret;
    MemAcc_Init(&MemAcc_Config);
    ret = MemAcc_Read(10u, 0u, TestBuffer, 32u);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_TRUE(Det_Stub_WasErrorReported(MEMACC_MODULE_ID,
                     MEMACC_SID_READ, MEMACC_E_PARAM_ADDRESS_AREA));
}

/*============================================================================*
 *  Test: Read with invalid address (out of bounds)
 *============================================================================*/

static void test_Read_InvalidAddress(void)
{
    Std_ReturnType ret;
    MemAcc_Init(&MemAcc_Config);
    /* Area 0 length is 0x10000, so address 0x10000 + 32 is out of bounds */
    ret = MemAcc_Read(0u, 0x10000u, TestBuffer, 32u);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_TRUE(Det_Stub_WasErrorReported(MEMACC_MODULE_ID,
                     MEMACC_SID_READ, MEMACC_E_PARAM_ADDRESS));
}

/*============================================================================*
 *  Test: Read with zero length
 *============================================================================*/

static void test_Read_ZeroLength(void)
{
    Std_ReturnType ret;
    MemAcc_Init(&MemAcc_Config);
    ret = MemAcc_Read(0u, 0u, TestBuffer, 0u);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_TRUE(Det_Stub_WasErrorReported(MEMACC_MODULE_ID,
                     MEMACC_SID_READ, MEMACC_E_PARAM_LENGTH));
}

/*============================================================================*
 *  Test: Read acceptance (valid params)
 *============================================================================*/

static void test_Read_Acceptance(void)
{
    Std_ReturnType ret;
    MemAcc_Init(&MemAcc_Config);
    ret = MemAcc_Read(0u, 0u, TestBuffer, 32u);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(MEMACC_JOB_PENDING, MemAcc_GetJobResult(0u));
}

/*============================================================================*
 *  Test: Write before init
 *============================================================================*/

static void test_Write_Uninit(void)
{
    Std_ReturnType ret;
    ret = MemAcc_Write(0u, 0u, TestWriteData, 32u);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_TRUE(Det_Stub_WasErrorReported(MEMACC_MODULE_ID,
                     MEMACC_SID_WRITE, MEMACC_E_UNINIT));
}

/*============================================================================*
 *  Test: Write with NULL pointer
 *============================================================================*/

static void test_Write_NullPointer(void)
{
    Std_ReturnType ret;
    MemAcc_Init(&MemAcc_Config);
    ret = MemAcc_Write(0u, 0u, NULL_PTR, 32u);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_TRUE(Det_Stub_WasErrorReported(MEMACC_MODULE_ID,
                     MEMACC_SID_WRITE, MEMACC_E_PARAM_POINTER));
}

/*============================================================================*
 *  Test: Write acceptance
 *============================================================================*/

static void test_Write_Acceptance(void)
{
    Std_ReturnType ret;
    MemAcc_Init(&MemAcc_Config);
    ret = MemAcc_Write(0u, 0u, TestWriteData, 32u);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(MEMACC_JOB_PENDING, MemAcc_GetJobResult(0u));
}

/*============================================================================*
 *  Test: Erase acceptance
 *============================================================================*/

static void test_Erase_Acceptance(void)
{
    Std_ReturnType ret;
    MemAcc_Init(&MemAcc_Config);
    ret = MemAcc_Erase(0u, 0u, 4096u);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(MEMACC_JOB_PENDING, MemAcc_GetJobResult(0u));
}

/*============================================================================*
 *  Test: BlankCheck acceptance
 *============================================================================*/

static void test_BlankCheck_Acceptance(void)
{
    Std_ReturnType ret;
    MemAcc_Init(&MemAcc_Config);
    ret = MemAcc_BlankCheck(0u, 0u, 4096u);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(MEMACC_JOB_PENDING, MemAcc_GetJobResult(0u));
}

/*============================================================================*
 *  Test: Compare acceptance
 *============================================================================*/

static void test_Compare_Acceptance(void)
{
    Std_ReturnType ret;
    MemAcc_Init(&MemAcc_Config);
    ret = MemAcc_Compare(0u, 0u, TestWriteData, 32u);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(MEMACC_JOB_PENDING, MemAcc_GetJobResult(0u));
}

/*============================================================================*
 *  Test: Compare with NULL pointer
 *============================================================================*/

static void test_Compare_NullPointer(void)
{
    Std_ReturnType ret;
    MemAcc_Init(&MemAcc_Config);
    ret = MemAcc_Compare(0u, 0u, NULL_PTR, 32u);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_TRUE(Det_Stub_WasErrorReported(MEMACC_MODULE_ID,
                     MEMACC_SID_COMPARE, MEMACC_E_PARAM_POINTER));
}

/*============================================================================*
 *  Test: Busy rejection -- read while area busy
 *============================================================================*/

static void test_Read_BusyRejection(void)
{
    Std_ReturnType ret;
    MemAcc_Init(&MemAcc_Config);
    ret = MemAcc_Read(0u, 0u, TestBuffer, 32u);
    TEST_ASSERT_EQUAL(E_OK, ret);

    /* Second read on same area should be rejected */
    ret = MemAcc_Read(0u, 0u, TestBuffer, 32u);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_TRUE(Det_Stub_WasErrorReported(MEMACC_MODULE_ID,
                     MEMACC_SID_READ, MEMACC_E_BUSY));
}

/*============================================================================*
 *  Test: Locked rejection -- read while area locked
 *============================================================================*/

static void test_Read_LockedRejection(void)
{
    Std_ReturnType ret;
    MemAcc_Init(&MemAcc_Config);

    ret = MemAcc_RequestLock(0u);
    TEST_ASSERT_EQUAL(E_OK, ret);

    ret = MemAcc_Read(0u, 0u, TestBuffer, 32u);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/*============================================================================*
 *  Test: Cancel an ongoing job
 *============================================================================*/

static void test_Cancel(void)
{
    Std_ReturnType ret;
    MemAcc_Init(&MemAcc_Config);
    ret = MemAcc_Read(0u, 0u, TestBuffer, 32u);
    TEST_ASSERT_EQUAL(E_OK, ret);

    MemAcc_Cancel(0u);
    TEST_ASSERT_EQUAL(MEMACC_JOB_CANCELED, MemAcc_GetJobResult(0u));
    TEST_ASSERT_EQUAL(FALSE, MemAcc_AreaBusy[0u]);
}

/*============================================================================*
 *  Test: Cancel before init
 *============================================================================*/

static void test_Cancel_Uninit(void)
{
    MemAcc_Cancel(0u);
    TEST_ASSERT_TRUE(Det_Stub_WasErrorReported(MEMACC_MODULE_ID,
                     MEMACC_SID_CANCEL, MEMACC_E_UNINIT));
}

/*============================================================================*
 *  Test: GetJobResult returns OK after init
 *============================================================================*/

static void test_GetJobResult_AfterInit(void)
{
    MemAcc_Init(&MemAcc_Config);
    TEST_ASSERT_EQUAL(MEMACC_JOB_OK, MemAcc_GetJobResult(0u));
}

/*============================================================================*
 *  Test: GetJobResult before init
 *============================================================================*/

static void test_GetJobResult_Uninit(void)
{
    MemAcc_JobResultType result;
    result = MemAcc_GetJobResult(0u);
    TEST_ASSERT_EQUAL(MEMACC_JOB_FAILED, result);
    TEST_ASSERT_TRUE(Det_Stub_WasErrorReported(MEMACC_MODULE_ID,
                     MEMACC_SID_GET_JOB_RESULT, MEMACC_E_UNINIT));
}

/*============================================================================*
 *  Test: GetProcessedLength
 *============================================================================*/

static void test_GetProcessedLength(void)
{
    MemAcc_LengthType len;
    MemAcc_Init(&MemAcc_Config);
    len = MemAcc_GetProcessedLength(0u);
    TEST_ASSERT_EQUAL(0u, len);
}

/*============================================================================*
 *  Test: GetSegmentationInfo
 *============================================================================*/

static void test_GetSegmentationInfo(void)
{
    Std_ReturnType ret;
    MemAcc_LengthType sectorSize = 0u;
    MemAcc_LengthType pageSize = 0u;

    MemAcc_Init(&MemAcc_Config);
    ret = MemAcc_GetSegmentationInfo(0u, &sectorSize, &pageSize);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(4096u, sectorSize);
    TEST_ASSERT_EQUAL(32u, pageSize);
}

/*============================================================================*
 *  Test: GetSegmentationInfo with NULL pointers
 *============================================================================*/

static void test_GetSegmentationInfo_NullPointer(void)
{
    Std_ReturnType ret;
    MemAcc_LengthType dummy = 0u;

    MemAcc_Init(&MemAcc_Config);
    ret = MemAcc_GetSegmentationInfo(0u, NULL_PTR, &dummy);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_TRUE(Det_Stub_WasErrorReported(MEMACC_MODULE_ID,
                     MEMACC_SID_GET_SEGMENTATION_INFO, MEMACC_E_PARAM_POINTER));
}

/*============================================================================*
 *  Test: RequestLock / ReleaseLock
 *============================================================================*/

static void test_RequestLock_ReleaseLock(void)
{
    Std_ReturnType ret;
    MemAcc_Init(&MemAcc_Config);

    ret = MemAcc_RequestLock(0u);
    TEST_ASSERT_EQUAL(E_OK, ret);

    /* Second lock attempt should fail */
    ret = MemAcc_RequestLock(0u);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);

    ret = MemAcc_ReleaseLock(0u);
    TEST_ASSERT_EQUAL(E_OK, ret);

    /* Second release should fail (already unlocked) */
    ret = MemAcc_ReleaseLock(0u);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/*============================================================================*
 *  Test: RequestLock before init
 *============================================================================*/

static void test_RequestLock_Uninit(void)
{
    Std_ReturnType ret;
    ret = MemAcc_RequestLock(0u);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_TRUE(Det_Stub_WasErrorReported(MEMACC_MODULE_ID,
                     MEMACC_SID_REQUEST_LOCK, MEMACC_E_UNINIT));
}

/*============================================================================*
 *  Test: GetVersionInfo
 *============================================================================*/

static void test_GetVersionInfo(void)
{
    Std_VersionInfoType vinfo;
    vinfo.vendorID = 0u;
    vinfo.moduleID = 0u;
    vinfo.sw_major_version = 0u;
    vinfo.sw_minor_version = 0u;
    vinfo.sw_patch_version = 0u;

    MemAcc_GetVersionInfo(&vinfo);
    TEST_ASSERT_EQUAL(MEMACC_VENDOR_ID, vinfo.vendorID);
    TEST_ASSERT_EQUAL(MEMACC_MODULE_ID, vinfo.moduleID);
    TEST_ASSERT_EQUAL(MEMACC_SW_MAJOR_VERSION, vinfo.sw_major_version);
    TEST_ASSERT_EQUAL(MEMACC_SW_MINOR_VERSION, vinfo.sw_minor_version);
    TEST_ASSERT_EQUAL(MEMACC_SW_PATCH_VERSION, vinfo.sw_patch_version);
}

/*============================================================================*
 *  Test: GetVersionInfo with NULL pointer
 *============================================================================*/

static void test_GetVersionInfo_NullPointer(void)
{
    MemAcc_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(Det_Stub_WasErrorReported(MEMACC_MODULE_ID,
                     MEMACC_SID_GET_VERSION_INFO, MEMACC_E_PARAM_POINTER));
}

/*============================================================================*
 *  Test: HwSpecificServiceRequest returns E_NOT_OK
 *============================================================================*/

static void test_HwSpecificServiceRequest(void)
{
    Std_ReturnType ret;
    ret = MemAcc_HwSpecificServiceRequest(0u, TestBuffer, 32u);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/*============================================================================*
 *  Test: Write busy rejection
 *============================================================================*/

static void test_Write_BusyRejection(void)
{
    Std_ReturnType ret;
    MemAcc_Init(&MemAcc_Config);
    ret = MemAcc_Write(0u, 0u, TestWriteData, 32u);
    TEST_ASSERT_EQUAL(E_OK, ret);

    ret = MemAcc_Write(0u, 0u, TestWriteData, 32u);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_TRUE(Det_Stub_WasErrorReported(MEMACC_MODULE_ID,
                     MEMACC_SID_WRITE, MEMACC_E_BUSY));
}

/*============================================================================*
 *  Test: Erase locked rejection
 *============================================================================*/

static void test_Erase_LockedRejection(void)
{
    Std_ReturnType ret;
    MemAcc_Init(&MemAcc_Config);
    (void)MemAcc_RequestLock(0u);
    ret = MemAcc_Erase(0u, 0u, 4096u);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/*============================================================================*
 *  Test: Erase with invalid area
 *============================================================================*/

static void test_Erase_InvalidArea(void)
{
    Std_ReturnType ret;
    MemAcc_Init(&MemAcc_Config);
    ret = MemAcc_Erase(99u, 0u, 4096u);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_TRUE(Det_Stub_WasErrorReported(MEMACC_MODULE_ID,
                     MEMACC_SID_ERASE, MEMACC_E_PARAM_ADDRESS_AREA));
}

/*============================================================================*
 *  Test: BlankCheck with zero length
 *============================================================================*/

static void test_BlankCheck_ZeroLength(void)
{
    Std_ReturnType ret;
    MemAcc_Init(&MemAcc_Config);
    ret = MemAcc_BlankCheck(0u, 0u, 0u);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_TRUE(Det_Stub_WasErrorReported(MEMACC_MODULE_ID,
                     MEMACC_SID_BLANK_CHECK, MEMACC_E_PARAM_LENGTH));
}

/*============================================================================*
 *  Test: Operations on area 1
 *============================================================================*/

static void test_Read_Area1_Acceptance(void)
{
    Std_ReturnType ret;
    MemAcc_Init(&MemAcc_Config);
    ret = MemAcc_Read(1u, 0u, TestBuffer, 32u);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(MEMACC_JOB_PENDING, MemAcc_GetJobResult(1u));
}

/*============================================================================*
 *  Test: GetProcessedLength before init
 *============================================================================*/

static void test_GetProcessedLength_Uninit(void)
{
    MemAcc_LengthType len;
    len = MemAcc_GetProcessedLength(0u);
    TEST_ASSERT_EQUAL(0u, len);
    TEST_ASSERT_TRUE(Det_Stub_WasErrorReported(MEMACC_MODULE_ID,
                     MEMACC_SID_GET_PROCESSED_LENGTH, MEMACC_E_UNINIT));
}

/*============================================================================*
 *  Test: ReleaseLock before init
 *============================================================================*/

static void test_ReleaseLock_Uninit(void)
{
    Std_ReturnType ret;
    ret = MemAcc_ReleaseLock(0u);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_TRUE(Det_Stub_WasErrorReported(MEMACC_MODULE_ID,
                     MEMACC_SID_RELEASE_LOCK, MEMACC_E_UNINIT));
}

/*============================================================================*
 *  Main
 *============================================================================*/

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_Init_ValidConfig);
    RUN_TEST(test_Init_NullPointer);
    RUN_TEST(test_DeInit_AfterInit);
    RUN_TEST(test_DeInit_BeforeInit);
    RUN_TEST(test_Read_Uninit);
    RUN_TEST(test_Read_NullPointer);
    RUN_TEST(test_Read_InvalidAreaId);
    RUN_TEST(test_Read_InvalidAddress);
    RUN_TEST(test_Read_ZeroLength);
    RUN_TEST(test_Read_Acceptance);
    RUN_TEST(test_Write_Uninit);
    RUN_TEST(test_Write_NullPointer);
    RUN_TEST(test_Write_Acceptance);
    RUN_TEST(test_Erase_Acceptance);
    RUN_TEST(test_BlankCheck_Acceptance);
    RUN_TEST(test_Compare_Acceptance);
    RUN_TEST(test_Compare_NullPointer);
    RUN_TEST(test_Read_BusyRejection);
    RUN_TEST(test_Read_LockedRejection);
    RUN_TEST(test_Cancel);
    RUN_TEST(test_Cancel_Uninit);
    RUN_TEST(test_GetJobResult_AfterInit);
    RUN_TEST(test_GetJobResult_Uninit);
    RUN_TEST(test_GetProcessedLength);
    RUN_TEST(test_GetSegmentationInfo);
    RUN_TEST(test_GetSegmentationInfo_NullPointer);
    RUN_TEST(test_RequestLock_ReleaseLock);
    RUN_TEST(test_RequestLock_Uninit);
    RUN_TEST(test_GetVersionInfo);
    RUN_TEST(test_GetVersionInfo_NullPointer);
    RUN_TEST(test_HwSpecificServiceRequest);
    RUN_TEST(test_Write_BusyRejection);
    RUN_TEST(test_Erase_LockedRejection);
    RUN_TEST(test_Erase_InvalidArea);
    RUN_TEST(test_BlankCheck_ZeroLength);
    RUN_TEST(test_Read_Area1_Acceptance);
    RUN_TEST(test_GetProcessedLength_Uninit);
    RUN_TEST(test_ReleaseLock_Uninit);

    return UNITY_END();
}
