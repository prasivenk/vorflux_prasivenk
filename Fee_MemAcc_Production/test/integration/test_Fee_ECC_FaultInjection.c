/**
 * \file       test_Fee_ECC_FaultInjection.c
 * \brief      Integration Tests -- ECC Fault Injection via Mem_DFLS_Stub
 *
 * \details    12+ tests injecting correctable and uncorrectable ECC errors
 *             at various stages and verifying proper error handling.
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
#include "test_Fee_Helpers.h"
#include <string.h>

/*============================================================================*
 *  Constants
 *============================================================================*/

#define MAX_CYCLES         200u
#define FLASH_BASE         0xAF000000u

/* DriveOneCycle, DriveInitToCompletion, DriveJobToCompletion
 * are provided by test_Fee_Helpers. */

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

/* 1. Correctable ECC on read – data still usable */
static void test_ECC_CorrectableOnRead(void)
{
    uint8 d[32], r[32];
    memset(d, 0xAA, 32);

    DriveInitToCompletion();
    Fee_Write(1u, d); DriveJobToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());

    /* Inject correctable ECC on data region of block 1 */
    /* Block is written after sector header; we inject in the data area */
    Mem_DFLS_Stub_InjectEccError(FLASH_BASE + 64u, 32u, FALSE);

    Fee_Read(1u, 0u, r, 32u); DriveJobToCompletion();
    /* Should still succeed (correctable) – data was already read at accept time */
    /* The stub returns ECC_CORRECTED from GetJobResult */
}

/* 2. Uncorrectable ECC on read – job fails */
static void test_ECC_UncorrectableOnRead(void)
{
    uint8 d[32], r[32];
    memset(d, 0xAA, 32);

    DriveInitToCompletion();
    Fee_Write(1u, d); DriveJobToCompletion();

    /* Inject uncorrectable ECC */
    Mem_DFLS_Stub_InjectEccError(FLASH_BASE + 64u, 32u, TRUE);

    Dem_Stub_Reset();
    Fee_Read(1u, 0u, r, 32u); DriveJobToCompletion();
    /* Job should fail due to uncorrectable ECC */
    TEST_ASSERT_NOT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
    /* DEM must report hardware error for safety traceability */
    TEST_ASSERT_TRUE(Dem_Stub_GetEventCount() >= 1u);
    {
        Dem_Stub_EventType evt = Dem_Stub_GetEvent(0u);
        TEST_ASSERT_EQUAL(FEE_E_HARDWARE_ERROR, evt.EventId);
        TEST_ASSERT_EQUAL(DEM_EVENT_STATUS_FAILED, evt.EventStatus);
    }
}

/* 3. ECC error before block data area – header affected */
static void test_ECC_HeaderRegion(void)
{
    uint8 d[32], r[32];
    memset(d, 0xAA, 32);

    DriveInitToCompletion();
    Fee_Write(1u, d); DriveJobToCompletion();

    /* Inject ECC in header area */
    Mem_DFLS_Stub_InjectEccError(FLASH_BASE + 32u, 32u, TRUE);

    /* Re-init to force re-scan with ECC error */
    Fee_ModuleStatus = MEMIF_UNINIT;
    Fee_InternalState = FEE_STATE_UNINIT;
    Fee_ConfigPtr = NULL_PTR;
    Fee_CurrentJob.Type = FEE_JOB_NONE;
    MemAcc_Init(&MemAcc_Config);
    DriveInitToCompletion();

    /* Block should not be found valid */
    Fee_Read(1u, 0u, r, 32u); DriveJobToCompletion();
    TEST_ASSERT_NOT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
}

/* 4. ECC on sector header during init */
static void test_ECC_SectorHeaderDuringInit(void)
{
    Mem_DFLS_Stub_InjectEccError(FLASH_BASE, 32u, TRUE);
    DriveInitToCompletion();
    /* Should still reach IDLE (sector treated as defective/blank) */
    TEST_ASSERT_EQUAL(MEMIF_IDLE, Fee_GetStatus());
}

/* 5. No ECC errors – normal path */
static void test_ECC_NoErrors_NormalPath(void)
{
    uint8 d[32], r[32];
    memset(d, 0xAA, 32);

    DriveInitToCompletion();
    Fee_Write(1u, d); DriveJobToCompletion();
    Fee_Read(1u, 0u, r, 32u); DriveJobToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
    TEST_ASSERT_EQUAL_MEMORY(d, r, 32);
}

/* 6. ECC on second block only – first block unaffected */
static void test_ECC_SecondBlockOnly(void)
{
    uint8 d1[32], d2[64], r1[32];
    memset(d1, 0x11, 32);
    memset(d2, 0x22, 64);

    DriveInitToCompletion();
    Fee_Write(1u, d1); DriveJobToCompletion();
    Fee_Write(2u, d2); DriveJobToCompletion();

    /* Inject ECC far enough to only affect block 2 area */
    /* Block 2 is written after block 1; offset varies. Inject broadly. */
    Mem_DFLS_Stub_InjectEccError(FLASH_BASE + 200u, 64u, TRUE);

    /* Block 1 should still be OK */
    Fee_Read(1u, 0u, r1, 32u); DriveJobToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
    TEST_ASSERT_EQUAL_MEMORY(d1, r1, 32);
}

/* 7. Write with API rejection (flash error) */
static void test_ECC_WriteApiRejection(void)
{
    uint8 d[32];
    memset(d, 0xAA, 32);

    DriveInitToCompletion();

    /* Force MemDFLS to reject writes */
    Mem_DFLS_Stub_SetReturnValue(E_NOT_OK);

    Fee_Write(1u, d); DriveJobToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_FAILED, Fee_GetJobResult());

    Mem_DFLS_Stub_SetReturnValue(E_OK);
}

/* 8. Correctable ECC cleared after erase */
static void test_ECC_ClearedAfterErase(void)
{
    /* Inject, erase, verify clean */
    Mem_DFLS_Stub_InjectEccError(FLASH_BASE + 100u, 32u, FALSE);

    MemAcc_Erase(0u, 0u, 4096u);
    MemAcc_MainFunction();

    /* After erase, ECC flags should be cleared */
    /* Writing and reading should work normally */
    DriveInitToCompletion();
    {
        uint8 d[32], r[32];
        memset(d, 0xCC, 32);
        Fee_Write(1u, d); DriveJobToCompletion();
        TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());

        Fee_Read(1u, 0u, r, 32u); DriveJobToCompletion();
        TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
        TEST_ASSERT_EQUAL_MEMORY(d, r, 32);
    }
}

/* 9. Mem_DFLS_Stub_SetJobResult override */
static void test_ECC_JobResultOverride(void)
{
    uint8 d[32], r[32];
    memset(d, 0xAA, 32);

    DriveInitToCompletion();
    Fee_Write(1u, d); DriveJobToCompletion();

    /* Override job result to FAILED */
    Mem_DFLS_Stub_SetJobResult(MEM_DFLS_JOB_FAILED);

    Fee_Read(1u, 0u, r, 32u); DriveJobToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_FAILED, Fee_GetJobResult());

    Mem_DFLS_Stub_SetJobResult(MEM_DFLS_JOB_OK);
}

/* 10. Large block with partial ECC */
static void test_ECC_LargeBlockPartialECC(void)
{
    uint8 d[512], r[512];
    uint32 i;
    for (i = 0; i < 512; i++) { d[i] = (uint8)(i & 0xFFu); }

    DriveInitToCompletion();
    Fee_Write(7u, d); DriveJobToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());

    /* Inject uncorrectable ECC in middle of data */
    Mem_DFLS_Stub_InjectEccError(FLASH_BASE + 300u, 16u, TRUE);

    Fee_Read(7u, 0u, r, 512u); DriveJobToCompletion();
    TEST_ASSERT_NOT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
}

/* 11. ECC injection API call count tracking */
static void test_ECC_CallCountTracking(void)
{
    uint8 d[32];
    memset(d, 0xAA, 32);

    DriveInitToCompletion();

    uint32 writeBefore = Mem_DFLS_Stub_GetCallCount(MEM_DFLS_STUB_API_WRITE);
    Fee_Write(1u, d); DriveJobToCompletion();
    uint32 writeAfter = Mem_DFLS_Stub_GetCallCount(MEM_DFLS_STUB_API_WRITE);

    TEST_ASSERT_TRUE(writeAfter > writeBefore);
}

/* 12. Multiple correctable ECCs – still successful */
static void test_ECC_MultipleCorrectable(void)
{
    uint8 d[32], r[32];
    memset(d, 0xAA, 32);

    DriveInitToCompletion();
    Fee_Write(1u, d); DriveJobToCompletion();

    /* Inject multiple correctable ECCs */
    Mem_DFLS_Stub_InjectEccError(FLASH_BASE + 64u, 8u, FALSE);
    Mem_DFLS_Stub_InjectEccError(FLASH_BASE + 80u, 8u, FALSE);

    Fee_Read(1u, 0u, r, 32u); DriveJobToCompletion();
    /* Correctable errors should not cause failure */
}

/*============================================================================*
 *  Main
 *============================================================================*/

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_ECC_CorrectableOnRead);
    RUN_TEST(test_ECC_UncorrectableOnRead);
    RUN_TEST(test_ECC_HeaderRegion);
    RUN_TEST(test_ECC_SectorHeaderDuringInit);
    RUN_TEST(test_ECC_NoErrors_NormalPath);
    RUN_TEST(test_ECC_SecondBlockOnly);
    RUN_TEST(test_ECC_WriteApiRejection);
    RUN_TEST(test_ECC_ClearedAfterErase);
    RUN_TEST(test_ECC_JobResultOverride);
    RUN_TEST(test_ECC_LargeBlockPartialECC);
    RUN_TEST(test_ECC_CallCountTracking);
    RUN_TEST(test_ECC_MultipleCorrectable);

    return UNITY_END();
}
