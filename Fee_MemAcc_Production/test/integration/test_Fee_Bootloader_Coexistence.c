/**
 * \file       test_Fee_Bootloader_Coexistence.c
 * \brief      Integration Tests -- Fee Area 0 + Bootloader Area 1 Coexistence
 *
 * \details    Tests that Fee uses MemAcc area 0, and area 1 (bootloader)
 *             can be used independently via MemAcc without interfering.
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

#define MAX_CYCLES         200u
#define AREA_FEE           0u
#define AREA_BOOTLOADER    1u
#define BL_BASE_OFFSET     0x10000u  /* Area 1 offset in flash array */

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

/* 1. Fee init does not corrupt bootloader area */
static void test_BL_FeeInitDoesNotCorruptBLArea(void)
{
    uint8 *flash = Mem_DFLS_Stub_GetFlashContent();
    uint8 blPattern[64];
    uint8 verifyBuf[64];
    memset(blPattern, 0xBD, 64);

    /* Pre-populate bootloader area */
    memcpy(&flash[BL_BASE_OFFSET], blPattern, 64);

    DriveInitToCompletion();

    /* Verify bootloader area is untouched */
    memcpy(verifyBuf, &flash[BL_BASE_OFFSET], 64);
    TEST_ASSERT_EQUAL_MEMORY(blPattern, verifyBuf, 64);
}

/* 2. Fee write does not corrupt bootloader area */
static void test_BL_FeeWriteDoesNotCorruptBL(void)
{
    uint8 *flash = Mem_DFLS_Stub_GetFlashContent();
    uint8 blPattern[64], d[32];
    memset(blPattern, 0xBD, 64);
    memset(d, 0xAA, 32);

    memcpy(&flash[BL_BASE_OFFSET], blPattern, 64);

    DriveInitToCompletion();
    Fee_Write(1u, d); DriveJobToCompletion();

    /* Verify bootloader area untouched */
    TEST_ASSERT_EQUAL_MEMORY(blPattern, &flash[BL_BASE_OFFSET], 64);
}

/* 3. MemAcc read from bootloader area works independently */
static void test_BL_MemAccReadArea1(void)
{
    uint8 *flash = Mem_DFLS_Stub_GetFlashContent();
    uint8 blData[32], readBuf[32];
    memset(blData, 0xBE, 32);

    memcpy(&flash[BL_BASE_OFFSET], blData, 32);

    MemAcc_Read(AREA_BOOTLOADER, 0u, readBuf, 32u);
    MemAcc_MainFunction();

    TEST_ASSERT_EQUAL(MEMACC_JOB_OK, MemAcc_GetJobResult(AREA_BOOTLOADER));
    TEST_ASSERT_EQUAL_MEMORY(blData, readBuf, 32);
}

/* 4. MemAcc write to bootloader area does not affect Fee area */
static void test_BL_MemAccWriteArea1_NoFeeImpact(void)
{
    uint8 feeData[32], blWrite[32], readBuf[32];
    memset(feeData, 0xFE, 32);
    memset(blWrite, 0xBE, 32);

    DriveInitToCompletion();
    Fee_Write(1u, feeData); DriveJobToCompletion();

    /* Write to bootloader area via MemAcc */
    MemAcc_Write(AREA_BOOTLOADER, 0u, blWrite, 32u);
    MemAcc_MainFunction();

    /* Verify Fee data unaffected */
    Fee_Read(1u, 0u, readBuf, 32u); DriveJobToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
    TEST_ASSERT_EQUAL_MEMORY(feeData, readBuf, 32);
}

/* 5. Area isolation – Fee uses area 0 config */
static void test_BL_FeeUsesArea0(void)
{
    DriveInitToCompletion();
    TEST_ASSERT_EQUAL(0u, Fee_Config.MemAccAreaId);
}

/* 6. MemAcc erase in BL area does not affect Fee */
static void test_BL_MemAccEraseArea1_NoFeeImpact(void)
{
    uint8 feeData[32], readBuf[32];
    memset(feeData, 0xFE, 32);

    DriveInitToCompletion();
    Fee_Write(1u, feeData); DriveJobToCompletion();

    MemAcc_Erase(AREA_BOOTLOADER, 0u, 4096u);
    MemAcc_MainFunction();

    Fee_Read(1u, 0u, readBuf, 32u); DriveJobToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
    TEST_ASSERT_EQUAL_MEMORY(feeData, readBuf, 32);
}

/* 7. Both areas can be configured */
static void test_BL_BothAreasConfigured(void)
{
    TEST_ASSERT_EQUAL(2u, MemAcc_Config.NumberOfAreas);
    TEST_ASSERT_EQUAL(AREA_FEE, MemAcc_Config.AddressAreas[0].AreaId);
    TEST_ASSERT_EQUAL(AREA_BOOTLOADER, MemAcc_Config.AddressAreas[1].AreaId);
}

/* 8. Concurrent Fee and BL area access */
static void test_BL_ConcurrentAccess(void)
{
    uint8 feeData[32], blData[32];
    memset(feeData, 0xFE, 32);
    memset(blData, 0xBE, 32);

    DriveInitToCompletion();

    Fee_Write(1u, feeData); DriveJobToCompletion();

    /* Write to BL area */
    MemAcc_Write(AREA_BOOTLOADER, 0u, blData, 32u);
    MemAcc_MainFunction();

    /* Verify both */
    {
        uint8 feeRead[32], blRead[32];
        Fee_Read(1u, 0u, feeRead, 32u); DriveJobToCompletion();
        TEST_ASSERT_EQUAL_MEMORY(feeData, feeRead, 32);

        MemAcc_Read(AREA_BOOTLOADER, 0u, blRead, 32u);
        MemAcc_MainFunction();
        TEST_ASSERT_EQUAL_MEMORY(blData, blRead, 32);
    }
}

/*============================================================================*
 *  Main
 *============================================================================*/

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_BL_FeeInitDoesNotCorruptBLArea);
    RUN_TEST(test_BL_FeeWriteDoesNotCorruptBL);
    RUN_TEST(test_BL_MemAccReadArea1);
    RUN_TEST(test_BL_MemAccWriteArea1_NoFeeImpact);
    RUN_TEST(test_BL_FeeUsesArea0);
    RUN_TEST(test_BL_MemAccEraseArea1_NoFeeImpact);
    RUN_TEST(test_BL_BothAreasConfigured);
    RUN_TEST(test_BL_ConcurrentAccess);

    return UNITY_END();
}
