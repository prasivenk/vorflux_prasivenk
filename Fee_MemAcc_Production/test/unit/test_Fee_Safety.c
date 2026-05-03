/**
 * \file       test_Fee_Safety.c
 * \brief      Unit Tests -- Fee Safety Mechanisms
 *
 * \details    Tests RAM CRC integrity checking, corruption detection,
 *             CRC update after legitimate modification, flow counter
 *             checking and reset.
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

    Fee_ModuleStatus = MEMIF_UNINIT;
    Fee_InternalState = FEE_STATE_UNINIT;
    Fee_LastJobResult = MEMIF_JOB_OK;
    Fee_ConfigPtr = NULL_PTR;
    Fee_CurrentJob.Type = FEE_JOB_NONE;

    MemAcc_Init(&MemAcc_Config);
}

void tearDown(void)
{
    /* Nothing */
}

/*============================================================================*
 *  1. Safety init and cyclic check: no corruption -> no error
 *============================================================================*/

static void test_Safety_InitAndCheck_NoCorruption(void)
{
    DriveInitToCompletion();
    Det_Stub_Reset();
    Dem_Stub_Reset();

    /* Run cyclic check -- should not detect any corruption */
    Fee_Safety_CyclicCheck();

    TEST_ASSERT_EQUAL(0u, Det_Stub_GetRuntimeErrorCount());
    TEST_ASSERT_EQUAL(0u, Dem_Stub_GetEventCount());
}

/*============================================================================*
 *  2. Safety corruption detection: modify BlockInfoTable -> error
 *============================================================================*/

static void test_Safety_CorruptionDetection_BlockInfoTable(void)
{
    DriveInitToCompletion();
    Det_Stub_Reset();
    Dem_Stub_Reset();

    /* Corrupt the BlockInfoTable (simulate RAM error) */
    Fee_BlockInfoTable[0].Status = FEE_BLOCK_VALID;

    /* Cyclic check should detect the corruption */
    Fee_Safety_CyclicCheck();

    TEST_ASSERT_TRUE(Det_Stub_WasRuntimeErrorReported(FEE_MODULE_ID,
                     FEE_SID_MAIN_FUNCTION, FEE_E_RAM_INTEGRITY));
    TEST_ASSERT_TRUE(Dem_Stub_WasEventReported(FEE_E_HARDWARE_ERROR,
                     DEM_EVENT_STATUS_FAILED));
}

/*============================================================================*
 *  3. Safety corruption detection: modify SectorInfo -> error
 *============================================================================*/

static void test_Safety_CorruptionDetection_SectorInfo(void)
{
    DriveInitToCompletion();
    Det_Stub_Reset();
    Dem_Stub_Reset();

    /* Corrupt the SectorInfo */
    Fee_SectorInfo[0].FreeSpace = 42u;

    Fee_Safety_CyclicCheck();

    TEST_ASSERT_TRUE(Det_Stub_WasRuntimeErrorReported(FEE_MODULE_ID,
                     FEE_SID_MAIN_FUNCTION, FEE_E_RAM_INTEGRITY));
    TEST_ASSERT_TRUE(Dem_Stub_WasEventReported(FEE_E_HARDWARE_ERROR,
                     DEM_EVENT_STATUS_FAILED));
}

/*============================================================================*
 *  4. Safety UpdateRamCrc after legitimate modification
 *============================================================================*/

static void test_Safety_UpdateAfterLegitModification(void)
{
    DriveInitToCompletion();

    /* Modify BlockInfoTable legitimately */
    Fee_BlockInfoTable[0].Status = FEE_BLOCK_VALID;

    /* Update the CRC */
    Fee_Safety_UpdateRamCrc();

    /* Reset detection counters */
    Det_Stub_Reset();
    Dem_Stub_Reset();

    /* Now cyclic check should pass (CRC updated) */
    Fee_Safety_CyclicCheck();

    TEST_ASSERT_EQUAL(0u, Det_Stub_GetRuntimeErrorCount());
    TEST_ASSERT_EQUAL(0u, Dem_Stub_GetEventCount());
}

/*============================================================================*
 *  5. Safety: multiple corruptions before update
 *============================================================================*/

static void test_Safety_MultipleCorruptions(void)
{
    DriveInitToCompletion();
    Det_Stub_Reset();
    Dem_Stub_Reset();

    /* Corrupt multiple fields */
    Fee_BlockInfoTable[0].Status = FEE_BLOCK_VALID;
    Fee_BlockInfoTable[1].DataCrc = 0x1234u;
    Fee_SectorInfo[0].EraseCount = 999u;

    Fee_Safety_CyclicCheck();

    TEST_ASSERT_TRUE(Det_Stub_WasRuntimeErrorReported(FEE_MODULE_ID,
                     FEE_SID_MAIN_FUNCTION, FEE_E_RAM_INTEGRITY));
}

/*============================================================================*
 *  6. Flow counter: correct sequence -> no error
 *============================================================================*/

static void test_Safety_FlowCheck_CorrectSequence(void)
{
    Det_Stub_Reset();
    Dem_Stub_Reset();

    Fee_Safety_ResetFlowCounter();

    Fee_Safety_FlowCheck(0u);  /* Expected 0, counter becomes 1 */
    Fee_Safety_FlowCheck(1u);  /* Expected 1, counter becomes 2 */
    Fee_Safety_FlowCheck(2u);  /* Expected 2, counter becomes 3 */

    TEST_ASSERT_EQUAL(0u, Det_Stub_GetRuntimeErrorCount());
    TEST_ASSERT_EQUAL(0u, Dem_Stub_GetEventCount());
}

/*============================================================================*
 *  7. Flow counter: mismatch detection
 *============================================================================*/

static void test_Safety_FlowCheck_Mismatch(void)
{
    Det_Stub_Reset();
    Dem_Stub_Reset();

    Fee_Safety_ResetFlowCounter();

    Fee_Safety_FlowCheck(0u);  /* OK: expected 0, counter becomes 1 */
    Fee_Safety_FlowCheck(5u);  /* Mismatch: expected 5, actual 1 */

    TEST_ASSERT_TRUE(Det_Stub_WasRuntimeErrorReported(FEE_MODULE_ID,
                     FEE_SID_MAIN_FUNCTION, FEE_E_RAM_INTEGRITY));
    TEST_ASSERT_TRUE(Dem_Stub_WasEventReported(FEE_E_HARDWARE_ERROR,
                     DEM_EVENT_STATUS_FAILED));
}

/*============================================================================*
 *  8. Flow counter: reset and re-check
 *============================================================================*/

static void test_Safety_FlowCounter_ResetAndRecheck(void)
{
    Det_Stub_Reset();
    Dem_Stub_Reset();

    Fee_Safety_ResetFlowCounter();
    Fee_Safety_FlowCheck(0u);
    Fee_Safety_FlowCheck(1u);

    /* Reset counter */
    Fee_Safety_ResetFlowCounter();

    /* Should work starting from 0 again */
    Fee_Safety_FlowCheck(0u);
    Fee_Safety_FlowCheck(1u);

    TEST_ASSERT_EQUAL(0u, Det_Stub_GetRuntimeErrorCount());
}

/*============================================================================*
 *  9. Safety init resets flow counter
 *============================================================================*/

static void test_Safety_Init_ResetsFlowCounter(void)
{
    Det_Stub_Reset();
    Dem_Stub_Reset();

    Fee_Safety_ResetFlowCounter();
    Fee_Safety_FlowCheck(0u);
    Fee_Safety_FlowCheck(1u);

    /* Init should reset everything */
    Fee_Safety_Init();

    Fee_Safety_FlowCheck(0u);  /* Should be 0 again */

    TEST_ASSERT_EQUAL(0u, Det_Stub_GetRuntimeErrorCount());
}

/*============================================================================*
 *  10. Safety cyclic check called from MainFunction -> no false positive
 *============================================================================*/

static void test_Safety_CyclicCheck_ViaMainFunction(void)
{
    DriveInitToCompletion();
    Det_Stub_Reset();
    Dem_Stub_Reset();

    /* Call MainFunction several times -- should not trigger safety error */
    Fee_MainFunction();
    Fee_MainFunction();
    Fee_MainFunction();

    TEST_ASSERT_EQUAL(0u, Det_Stub_GetRuntimeErrorCount());
    TEST_ASSERT_EQUAL(0u, Dem_Stub_GetEventCount());
}

/*============================================================================*
 *  11. Safety: write updates CRC so no false positive
 *============================================================================*/

static void test_Safety_WriteUpdatesRamCrc(void)
{
    uint8 data[32];
    uint8 *flash;

    memset(data, 0xBB, 32);

    /* Place active sector */
    flash = Mem_DFLS_Stub_GetFlashContent();
    {
        uint8 hdr[32];
        Fee_Sector_BuildSectorHeader(hdr, 1u, 0u);
        memcpy(&flash[0], hdr, 32);
    }

    DriveInitToCompletion();

    /* Write a block -- this modifies BlockInfoTable */
    Fee_Write(1u, data);

    /* Drive write to completion (includes safety update) */
    {
        uint32 cycle;
        for (cycle = 0u; cycle < 200u; cycle++)
        {
            MemAcc_MainFunction();
            Fee_MainFunction();
            if (Fee_GetStatus() == MEMIF_IDLE)
                break;
        }
    }

    Det_Stub_Reset();
    Dem_Stub_Reset();

    /* Cyclic check after write should not detect corruption */
    Fee_Safety_CyclicCheck();

    TEST_ASSERT_EQUAL(0u, Det_Stub_GetRuntimeErrorCount());
    TEST_ASSERT_EQUAL(0u, Dem_Stub_GetEventCount());
}

/*============================================================================*
 *  12. Safety: DEM event has correct event ID
 *============================================================================*/

static void test_Safety_DemEventId(void)
{
    DriveInitToCompletion();
    Dem_Stub_Reset();

    /* Corrupt and check */
    Fee_BlockInfoTable[0].HeaderAddress = 0xDEADBEEFu;
    Fee_Safety_CyclicCheck();

    TEST_ASSERT_EQUAL(1u, Dem_Stub_GetEventCount());

    {
        Dem_Stub_EventType evt = Dem_Stub_GetEvent(0u);
        TEST_ASSERT_EQUAL(FEE_E_HARDWARE_ERROR, evt.EventId);
        TEST_ASSERT_EQUAL(DEM_EVENT_STATUS_FAILED, evt.EventStatus);
    }
}

/*============================================================================*
 *  Main
 *============================================================================*/

int main(void)
{
    UNITY_BEGIN();

    /* RAM CRC tests */
    RUN_TEST(test_Safety_InitAndCheck_NoCorruption);
    RUN_TEST(test_Safety_CorruptionDetection_BlockInfoTable);
    RUN_TEST(test_Safety_CorruptionDetection_SectorInfo);
    RUN_TEST(test_Safety_UpdateAfterLegitModification);
    RUN_TEST(test_Safety_MultipleCorruptions);
    RUN_TEST(test_Safety_WriteUpdatesRamCrc);
    RUN_TEST(test_Safety_CyclicCheck_ViaMainFunction);
    RUN_TEST(test_Safety_DemEventId);

    /* Flow counter tests */
    RUN_TEST(test_Safety_FlowCheck_CorrectSequence);
    RUN_TEST(test_Safety_FlowCheck_Mismatch);
    RUN_TEST(test_Safety_FlowCounter_ResetAndRecheck);
    RUN_TEST(test_Safety_Init_ResetsFlowCounter);

    return UNITY_END();
}
