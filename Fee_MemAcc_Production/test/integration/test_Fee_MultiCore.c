/**
 * \file       test_Fee_MultiCore.c
 * \brief      Integration Tests -- SchM Enter/Exit Balance Verification
 *
 * \details    8+ tests verifying that SchM exclusive area enter/exit calls
 *             are balanced, and that MemAcc lock/unlock behaves correctly.
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
#include "test_Fee_Helpers.h"
#include <string.h>

/*============================================================================*
 *  Constants
 *============================================================================*/

#define MAX_CYCLES  200u

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

/* 1. SchM enter/exit balanced after init */
static void test_MC_BalancedAfterInit(void)
{
    SchM_Stub_Reset();
    DriveInitToCompletion();
    TEST_ASSERT_EQUAL(SchM_Stub_GetEnterCount(), SchM_Stub_GetExitCount());
}

/* 2. SchM enter/exit balanced after write */
static void test_MC_BalancedAfterWrite(void)
{
    uint8 d[32];
    memset(d, 0xAA, 32);

    DriveInitToCompletion();
    SchM_Stub_Reset();

    Fee_Write(1u, d); DriveJobToCompletion();
    TEST_ASSERT_EQUAL(SchM_Stub_GetEnterCount(), SchM_Stub_GetExitCount());
}

/* 3. SchM enter/exit balanced after read */
static void test_MC_BalancedAfterRead(void)
{
    uint8 d[32], r[32];
    memset(d, 0xAA, 32);

    DriveInitToCompletion();
    Fee_Write(1u, d); DriveJobToCompletion();
    SchM_Stub_Reset();

    Fee_Read(1u, 0u, r, 32u); DriveJobToCompletion();
    TEST_ASSERT_EQUAL(SchM_Stub_GetEnterCount(), SchM_Stub_GetExitCount());
}

/* 4. SchM enter/exit balanced after cancel */
static void test_MC_BalancedAfterCancel(void)
{
    uint8 d[32];
    memset(d, 0xAA, 32);

    DriveInitToCompletion();
    SchM_Stub_Reset();

    Fee_Write(1u, d);
    DriveOneCycle();
    Fee_Cancel();

    TEST_ASSERT_EQUAL(SchM_Stub_GetEnterCount(), SchM_Stub_GetExitCount());
}

/* 5. SchM enter/exit balanced after invalidate */
static void test_MC_BalancedAfterInvalidate(void)
{
    uint8 d[32];
    memset(d, 0xAA, 32);

    DriveInitToCompletion();
    Fee_Write(1u, d); DriveJobToCompletion();
    SchM_Stub_Reset();

    Fee_InvalidateBlock(1u); DriveJobToCompletion();
    TEST_ASSERT_EQUAL(SchM_Stub_GetEnterCount(), SchM_Stub_GetExitCount());
}

/* 6. MemAcc lock and release */
static void test_MC_MemAccLockRelease(void)
{
    Std_ReturnType ret;

    ret = MemAcc_RequestLock(0u);
    TEST_ASSERT_EQUAL(E_OK, ret);

    ret = MemAcc_ReleaseLock(0u);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

/* 7. MemAcc reject operation when locked */
static void test_MC_MemAccRejectWhenLocked(void)
{
    uint8 d[32];
    Std_ReturnType ret;

    MemAcc_RequestLock(0u);
    ret = MemAcc_Write(0u, 0u, d, 32u);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    MemAcc_ReleaseLock(0u);
}

/* 8. SchM balanced after multiple operations */
static void test_MC_BalancedAfterMultipleOps(void)
{
    uint8 d1[32], d2[64], r[32];
    memset(d1, 0x11, 32);
    memset(d2, 0x22, 64);

    DriveInitToCompletion();
    SchM_Stub_Reset();

    Fee_Write(1u, d1); DriveJobToCompletion();
    Fee_Write(2u, d2); DriveJobToCompletion();
    Fee_Read(1u, 0u, r, 32u); DriveJobToCompletion();
    Fee_InvalidateBlock(2u); DriveJobToCompletion();

    TEST_ASSERT_EQUAL(SchM_Stub_GetEnterCount(), SchM_Stub_GetExitCount());
}

/*============================================================================*
 *  Main
 *============================================================================*/

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_MC_BalancedAfterInit);
    RUN_TEST(test_MC_BalancedAfterWrite);
    RUN_TEST(test_MC_BalancedAfterRead);
    RUN_TEST(test_MC_BalancedAfterCancel);
    RUN_TEST(test_MC_BalancedAfterInvalidate);
    RUN_TEST(test_MC_MemAccLockRelease);
    RUN_TEST(test_MC_MemAccRejectWhenLocked);
    RUN_TEST(test_MC_BalancedAfterMultipleOps);

    return UNITY_END();
}
