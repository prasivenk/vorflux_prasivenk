/**
 * \file       test_Fee_OTA_Scenarios.c
 * \brief      Integration Tests -- OTA Configuration Migration Scenarios
 *
 * \details    Tests configuration migration scenarios that occur during
 *             OTA (Over-The-Air) updates: data survives reboot, unknown
 *             blocks ignored, new blocks added, config changes, etc.
 *             Uses PlaceBlockInFlash to pre-populate flash for re-init tests.
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
#include <string.h>

/*============================================================================*
 *  Constants / Helpers
 *============================================================================*/

#define MAX_CYCLES       200u
#define TEST_SECTOR_SIZE FEE_SECTOR_SIZE
#define TEST_SECTOR_HDR  FEE_SECTOR_HEADER_SIZE
#define TEST_BLOCK_HDR   FEE_BLOCK_HEADER_SIZE

static void DriveOneCycle(void) { MemAcc_MainFunction(); Fee_MainFunction(); }

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

static void PlaceSectorHeader(uint32 flashOffset, uint32 seqNum, uint16 eraseCount)
{
    uint8 hdr[32];
    uint8 *flash = Mem_DFLS_Stub_GetFlashContent();
    Fee_Sector_BuildSectorHeader(hdr, seqNum, eraseCount);
    memcpy(&flash[flashOffset], hdr, 32);
}

static void PlaceBlockInFlash(uint32 flashOffset, uint16 blockNum, uint16 blockSize,
                               const uint8 *data, uint16 seqCounter, uint8 validMarker)
{
    uint8 hdr[32];
    uint8 *flash = Mem_DFLS_Stub_GetFlashContent();
    uint16 dataCrc;
    dataCrc = Fee_Crc_CalculateBlock(data, (uint32)blockSize);
    Fee_Sector_BuildBlockHeader(hdr, blockNum, blockSize, dataCrc, seqCounter, 0u);
    hdr[30] = validMarker;
    memcpy(&flash[flashOffset], hdr, 32);
    memcpy(&flash[flashOffset + 32], data, blockSize);
}

static void ReInitFee(void)
{
    Det_Stub_Reset();
    Dem_Stub_Reset();
    NvM_Cbk_Stub_Reset();
    Fee_ModuleStatus = MEMIF_UNINIT;
    Fee_InternalState = FEE_STATE_UNINIT;
    Fee_LastJobResult = MEMIF_JOB_OK;
    Fee_ConfigPtr = NULL_PTR;
    Fee_CurrentJob.Type = FEE_JOB_NONE;
    MemAcc_Init(&MemAcc_Config);
    DriveInitToCompletion();
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
 *  OTA Tests
 *============================================================================*/

/* 1. Pre-populated block data survives reboot (re-init) */
static void test_OTA_DataSurvivesReboot(void)
{
    uint8 d[32], r[32];
    memset(d, 0xAA, 32);

    PlaceSectorHeader(0u, 1u, 0u);
    PlaceBlockInFlash(TEST_SECTOR_HDR, 1u, 32u, d, 1u, FEE_MARKER_VALID);

    DriveInitToCompletion();
    TEST_ASSERT_EQUAL(FEE_BLOCK_VALID, Fee_BlockInfoTable[0].Status);

    /* Simulate OTA reboot */
    ReInitFee();
    Fee_Read(1u, 0u, r, 32u); DriveJobToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
    TEST_ASSERT_EQUAL_MEMORY(d, r, 32);
}

/* 2. Unknown block in flash is ignored on re-init */
static void test_OTA_UnknownBlockIgnored(void)
{
    uint8 d1[32], unknownData[32], r[32];
    memset(d1, 0xAA, 32);
    memset(unknownData, 0xBB, 32);

    PlaceSectorHeader(0u, 1u, 0u);
    PlaceBlockInFlash(TEST_SECTOR_HDR, 1u, 32u, d1, 1u, FEE_MARKER_VALID);

    /* Unknown block (number 99) - placed after block 1 */
    {
        uint32 offset2 = TEST_SECTOR_HDR + TEST_BLOCK_HDR + 32u;
        PlaceBlockInFlash(offset2, 99u, 32u, unknownData, 1u, FEE_MARKER_VALID);
    }

    DriveInitToCompletion();

    /* Block 1 should still be valid */
    Fee_Read(1u, 0u, r, 32u); DriveJobToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
    TEST_ASSERT_EQUAL_MEMORY(d1, r, 32);
}

/* 3. Multiple blocks survive OTA reboot */
static void test_OTA_MultipleBlocksSurvive(void)
{
    uint8 d1[32], d2[64], d5[16];
    uint8 r1[32], r2[64], r5[16];
    uint32 off;
    memset(d1, 0x11, 32);
    memset(d2, 0x22, 64);
    memset(d5, 0x55, 16);

    PlaceSectorHeader(0u, 1u, 0u);
    off = TEST_SECTOR_HDR;
    PlaceBlockInFlash(off, 1u, 32u, d1, 1u, FEE_MARKER_VALID);
    off += TEST_BLOCK_HDR + 32u;
    PlaceBlockInFlash(off, 2u, 64u, d2, 1u, FEE_MARKER_VALID);
    off += TEST_BLOCK_HDR + 64u;
    PlaceBlockInFlash(off, 5u, 16u, d5, 1u, FEE_MARKER_VALID);

    DriveInitToCompletion();

    /* Simulate OTA reboot */
    ReInitFee();

    Fee_Read(1u, 0u, r1, 32u); DriveJobToCompletion();
    TEST_ASSERT_EQUAL_MEMORY(d1, r1, 32);
    Fee_Read(2u, 0u, r2, 64u); DriveJobToCompletion();
    TEST_ASSERT_EQUAL_MEMORY(d2, r2, 64);
    Fee_Read(5u, 0u, r5, 16u); DriveJobToCompletion();
    TEST_ASSERT_EQUAL_MEMORY(d5, r5, 16);
}

/* 4. Write new block after OTA re-init */
static void test_OTA_WriteNewBlockAfterReInit(void)
{
    uint8 d1[32], d3[128], r3[128];
    memset(d1, 0xAA, 32);
    memset(d3, 0x33, 128);

    PlaceSectorHeader(0u, 1u, 0u);
    PlaceBlockInFlash(TEST_SECTOR_HDR, 1u, 32u, d1, 1u, FEE_MARKER_VALID);

    DriveInitToCompletion();
    ReInitFee();

    Fee_Write(3u, d3); DriveJobToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());

    Fee_Read(3u, 0u, r3, 128u); DriveJobToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
    TEST_ASSERT_EQUAL_MEMORY(d3, r3, 128);
}

/* 5. Overwrite block after OTA re-init */
static void test_OTA_OverwriteAfterReInit(void)
{
    uint8 d_old[32], d_new[32], r[32];
    memset(d_old, 0x11, 32);
    memset(d_new, 0x22, 32);

    PlaceSectorHeader(0u, 1u, 0u);
    PlaceBlockInFlash(TEST_SECTOR_HDR, 1u, 32u, d_old, 1u, FEE_MARKER_VALID);

    DriveInitToCompletion();
    ReInitFee();

    /* Write through the API – in-memory state updated */
    Fee_Write(1u, d_new); DriveJobToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());

    Fee_Read(1u, 0u, r, 32u); DriveJobToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
    TEST_ASSERT_EQUAL_MEMORY(d_new, r, 32);
}

/* 6. Invalidate after OTA reboot */
static void test_OTA_InvalidateAfterReInit(void)
{
    uint8 d[32], r[32];
    memset(d, 0xAA, 32);

    PlaceSectorHeader(0u, 1u, 0u);
    PlaceBlockInFlash(TEST_SECTOR_HDR, 1u, 32u, d, 1u, FEE_MARKER_VALID);

    DriveInitToCompletion();
    ReInitFee();

    Fee_InvalidateBlock(1u); DriveJobToCompletion();
    Fee_Read(1u, 0u, r, 32u); DriveJobToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_BLOCK_INVALID, Fee_GetJobResult());
}

/* 7. Double reboot with pre-populated data */
static void test_OTA_DoubleReboot(void)
{
    uint8 d[32], r[32];
    memset(d, 0xCC, 32);

    PlaceSectorHeader(0u, 1u, 0u);
    PlaceBlockInFlash(TEST_SECTOR_HDR, 1u, 32u, d, 1u, FEE_MARKER_VALID);

    DriveInitToCompletion();
    ReInitFee();
    ReInitFee();

    Fee_Read(1u, 0u, r, 32u); DriveJobToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
    TEST_ASSERT_EQUAL_MEMORY(d, r, 32);
}

/* 8. Immediate block survives OTA */
static void test_OTA_ImmediateBlockSurvives(void)
{
    uint8 d[16], r[16];
    memset(d, 0xEE, 16);

    PlaceSectorHeader(0u, 1u, 0u);
    PlaceBlockInFlash(TEST_SECTOR_HDR, 5u, 16u, d, 1u, FEE_MARKER_VALID);

    DriveInitToCompletion();
    ReInitFee();

    Fee_Read(5u, 0u, r, 16u); DriveJobToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
    TEST_ASSERT_EQUAL_MEMORY(d, r, 16);
}

/* 9. Blank flash OTA – no blocks written */
static void test_OTA_BlankFlashReInit(void)
{
    DriveInitToCompletion();
    ReInitFee();
    TEST_ASSERT_EQUAL(MEMIF_IDLE, Fee_GetStatus());
}

/* 10. Sequential data pattern verified after OTA */
static void test_OTA_SequentialPattern(void)
{
    uint8 d[32], r[32];
    uint32 i;
    for (i = 0; i < 32; i++) { d[i] = (uint8)i; }

    PlaceSectorHeader(0u, 1u, 0u);
    PlaceBlockInFlash(TEST_SECTOR_HDR, 1u, 32u, d, 1u, FEE_MARKER_VALID);

    DriveInitToCompletion();
    ReInitFee();

    Fee_Read(1u, 0u, r, 32u); DriveJobToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
    TEST_ASSERT_EQUAL_MEMORY(d, r, 32);
}

/*============================================================================*
 *  Main
 *============================================================================*/

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_OTA_DataSurvivesReboot);
    RUN_TEST(test_OTA_UnknownBlockIgnored);
    RUN_TEST(test_OTA_MultipleBlocksSurvive);
    RUN_TEST(test_OTA_WriteNewBlockAfterReInit);
    RUN_TEST(test_OTA_OverwriteAfterReInit);
    RUN_TEST(test_OTA_InvalidateAfterReInit);
    RUN_TEST(test_OTA_DoubleReboot);
    RUN_TEST(test_OTA_ImmediateBlockSurvives);
    RUN_TEST(test_OTA_BlankFlashReInit);
    RUN_TEST(test_OTA_SequentialPattern);

    return UNITY_END();
}
