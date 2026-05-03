/**
 * \file       test_Fee_Recovery.c
 * \brief      Integration Tests -- Power Loss Recovery
 *
 * \details    10+ integration-level tests for power loss recovery scenarios.
 *             Uses PlaceBlockInFlash to pre-populate flash with valid data
 *             for re-init/recovery tests exercising the scan logic.
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

#define MAX_CYCLES       200u
#define TEST_SECTOR_SIZE FEE_SECTOR_SIZE
#define TEST_SECTOR_HDR  FEE_SECTOR_HEADER_SIZE
#define TEST_BLOCK_HDR   FEE_BLOCK_HEADER_SIZE

/* DriveOneCycle, DriveInitToCompletion, DriveJobToCompletion,
 * PlaceSectorHeader, PlaceBlockInFlash are provided by test_Fee_Helpers. */

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
 *  Tests
 *============================================================================*/

/* 1. Recovery from blank flash – no data, re-init ok */
static void test_Recovery_BlankFlash(void)
{
    DriveInitToCompletion();
    ReInitFee();
    TEST_ASSERT_EQUAL(MEMIF_IDLE, Fee_GetStatus());
}

/* 2. Single committed block survives recovery */
static void test_Recovery_SingleBlock(void)
{
    uint8 d[32], r[32];
    memset(d, 0xAA, 32);

    PlaceSectorHeader(0u, 1u, 0u);
    PlaceBlockInFlash(TEST_SECTOR_HDR, 1u, 32u, d, 1u, FEE_MARKER_VALID);

    DriveInitToCompletion();
    ReInitFee();

    Fee_Read(1u, 0u, r, 32u); DriveJobToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
    TEST_ASSERT_EQUAL_MEMORY(d, r, 32);
}

/* 3. Multiple blocks committed – all survive */
static void test_Recovery_MultipleBlocks(void)
{
    uint8 d1[32], d2[64], d3[128];
    uint8 r1[32], r2[64], r3[128];
    uint32 off;
    memset(d1, 0x11, 32);
    memset(d2, 0x22, 64);
    memset(d3, 0x33, 128);

    PlaceSectorHeader(0u, 1u, 0u);
    off = TEST_SECTOR_HDR;
    PlaceBlockInFlash(off, 1u, 32u, d1, 1u, FEE_MARKER_VALID);
    off += TEST_BLOCK_HDR + 32u;
    PlaceBlockInFlash(off, 2u, 64u, d2, 1u, FEE_MARKER_VALID);
    off += TEST_BLOCK_HDR + 64u;
    PlaceBlockInFlash(off, 3u, 128u, d3, 1u, FEE_MARKER_VALID);

    DriveInitToCompletion();
    ReInitFee();

    Fee_Read(1u, 0u, r1, 32u); DriveJobToCompletion();
    TEST_ASSERT_EQUAL_MEMORY(d1, r1, 32);
    Fee_Read(2u, 0u, r2, 64u); DriveJobToCompletion();
    TEST_ASSERT_EQUAL_MEMORY(d2, r2, 64);
    Fee_Read(3u, 0u, r3, 128u); DriveJobToCompletion();
    TEST_ASSERT_EQUAL_MEMORY(d3, r3, 128);
}

/* 4. Incomplete write of new block is INCONSISTENT (different block) */
static void test_Recovery_IncompleteWriteIgnored(void)
{
    uint8 d_committed[32], d_incomplete[64];
    uint8 r[32];
    uint32 off;
    memset(d_committed, 0xAA, 32);
    memset(d_incomplete, 0xBB, 64);

    PlaceSectorHeader(0u, 1u, 0u);
    off = TEST_SECTOR_HDR;
    /* Block 1 committed */
    PlaceBlockInFlash(off, 1u, 32u, d_committed, 1u, FEE_MARKER_VALID);
    off += TEST_BLOCK_HDR + 32u;
    /* Block 2 incomplete (different block) */
    PlaceBlockInFlash(off, 2u, 64u, d_incomplete, 1u, FEE_MARKER_ERASED);

    DriveInitToCompletion();

    /* Block 1 should be valid */
    Fee_Read(1u, 0u, r, 32u); DriveJobToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
    TEST_ASSERT_EQUAL_MEMORY(d_committed, r, 32);

    /* Block 2 should be inconsistent */
    TEST_ASSERT_EQUAL(FEE_BLOCK_INCONSISTENT, Fee_BlockInfoTable[1].Status);
}

/* 5. Recovery then new write succeeds */
static void test_Recovery_ThenNewWrite(void)
{
    uint8 d1[32], d2[32], r[32];
    memset(d1, 0xAA, 32);
    memset(d2, 0xBB, 32);

    PlaceSectorHeader(0u, 1u, 0u);
    PlaceBlockInFlash(TEST_SECTOR_HDR, 1u, 32u, d1, 1u, FEE_MARKER_VALID);

    DriveInitToCompletion();
    ReInitFee();

    Fee_Write(1u, d2); DriveJobToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());

    Fee_Read(1u, 0u, r, 32u); DriveJobToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
    TEST_ASSERT_EQUAL_MEMORY(d2, r, 32);
}

/* 6. Invalidated block stays invalid after recovery */
static void test_Recovery_InvalidatedBlockStaysInvalid(void)
{
    uint8 d[32], r[32];
    memset(d, 0xAA, 32);

    PlaceSectorHeader(0u, 1u, 0u);
    PlaceBlockInFlash(TEST_SECTOR_HDR, 1u, 32u, d, 1u, FEE_MARKER_INVALID);

    DriveInitToCompletion();

    Fee_Read(1u, 0u, r, 32u); DriveJobToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_BLOCK_INVALID, Fee_GetJobResult());
}

/* 7. Triple reboot with data */
static void test_Recovery_TripleReboot(void)
{
    uint8 d[32], r[32];
    memset(d, 0xDD, 32);

    PlaceSectorHeader(0u, 1u, 0u);
    PlaceBlockInFlash(TEST_SECTOR_HDR, 1u, 32u, d, 1u, FEE_MARKER_VALID);

    DriveInitToCompletion();
    ReInitFee();
    ReInitFee();
    ReInitFee();

    Fee_Read(1u, 0u, r, 32u); DriveJobToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
    TEST_ASSERT_EQUAL_MEMORY(d, r, 32);
}

/* 8. Recovery with immediate data block */
static void test_Recovery_ImmediateData(void)
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

/* 9. Recovery with small immediate block (8 bytes) */
static void test_Recovery_SmallImmediateBlock(void)
{
    uint8 d[8], r[8];
    memset(d, 0xFE, 8);

    PlaceSectorHeader(0u, 1u, 0u);
    PlaceBlockInFlash(TEST_SECTOR_HDR, 6u, 8u, d, 1u, FEE_MARKER_VALID);

    DriveInitToCompletion();
    ReInitFee();

    Fee_Read(6u, 0u, r, 8u); DriveJobToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
    TEST_ASSERT_EQUAL_MEMORY(d, r, 8);
}

/* 10. Recovery with large block (512 bytes) */
static void test_Recovery_LargeBlock(void)
{
    uint8 d[512], r[512];
    uint32 i;
    for (i = 0; i < 512; i++) { d[i] = (uint8)(i & 0xFFu); }

    PlaceSectorHeader(0u, 1u, 0u);
    PlaceBlockInFlash(TEST_SECTOR_HDR, 7u, 512u, d, 1u, FEE_MARKER_VALID);

    DriveInitToCompletion();
    ReInitFee();

    Fee_Read(7u, 0u, r, 512u); DriveJobToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
    TEST_ASSERT_EQUAL_MEMORY(d, r, 512);
}

/*============================================================================*
 *  Main
 *============================================================================*/

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_Recovery_BlankFlash);
    RUN_TEST(test_Recovery_SingleBlock);
    RUN_TEST(test_Recovery_MultipleBlocks);
    RUN_TEST(test_Recovery_IncompleteWriteIgnored);
    RUN_TEST(test_Recovery_ThenNewWrite);
    RUN_TEST(test_Recovery_InvalidatedBlockStaysInvalid);
    RUN_TEST(test_Recovery_TripleReboot);
    RUN_TEST(test_Recovery_ImmediateData);
    RUN_TEST(test_Recovery_SmallImmediateBlock);
    RUN_TEST(test_Recovery_LargeBlock);

    return UNITY_END();
}
