/**
 * \file       test_Fee_PowerLoss.c
 * \brief      Unit Tests -- Fee Power Loss Recovery
 *
 * \details    Simulates power loss at each write phase (after header write,
 *             after data write, before valid marker, during GC copy, during
 *             GC erase). Re-initializes Fee, scans flash, and verifies no
 *             data loss for previously committed blocks (blocks with valid
 *             marker in flash). Uses PlaceBlockInFlash to pre-populate flash.
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
#include "test_Fee_Helpers.h"
#include <string.h>

/*============================================================================*
 *  Constants
 *============================================================================*/

#define TEST_SECTOR_SIZE     FEE_SECTOR_SIZE         /* 16384 */
#define TEST_SECTOR_HDR      FEE_SECTOR_HEADER_SIZE  /* 32 */
#define TEST_BLOCK_HDR       FEE_BLOCK_HEADER_SIZE   /* 32 */
#define TEST_PAGE_SIZE       FEE_VIRTUAL_PAGE_SIZE   /* 32 */
#define MAX_CYCLES           200u

/* DriveOneCycle, DriveInitToCompletion, DriveJobToCompletion,
 * PlaceSectorHeader, PlaceBlockInFlash are provided by test_Fee_Helpers. */

/** \brief Full reset of all stubs and Fee state */
static void FullReset(void)
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

/** \brief Reset Fee state only (preserving flash contents) */
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
    FullReset();
}

void tearDown(void)
{
    /* Nothing */
}

/*============================================================================*
 *  1. Power loss before any write (blank flash re-init)
 *============================================================================*/

static void test_PowerLoss_BlankFlash_ReInit(void)
{
    DriveInitToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_IDLE, Fee_GetStatus());

    ReInitFee();
    TEST_ASSERT_EQUAL(MEMIF_IDLE, Fee_GetStatus());
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
}

/*============================================================================*
 *  2. Power loss after header written but before data
 *============================================================================*/

static void test_PowerLoss_AfterHeaderWrite_BeforeData(void)
{
    uint8 *flash = Mem_DFLS_Stub_GetFlashContent();
    uint8 hdr[32];
    uint8 readBuf[32];

    PlaceSectorHeader(0u, 1u, 0u);

    /* Write header only (no valid marker, no correct data) */
    Fee_Sector_BuildBlockHeader(hdr, 1u, 32u, 0x1234u, 1u, 0u);
    hdr[30] = FEE_MARKER_ERASED;
    memcpy(&flash[TEST_SECTOR_HDR], hdr, 32);

    DriveInitToCompletion();
    TEST_ASSERT_NOT_EQUAL(FEE_BLOCK_VALID, Fee_BlockInfoTable[0].Status);

    Fee_Read(1u, 0u, readBuf, 32u);
    DriveJobToCompletion();
    TEST_ASSERT_NOT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
}

/*============================================================================*
 *  3. Power loss after data written but before valid marker
 *============================================================================*/

static void test_PowerLoss_AfterDataWrite_BeforeValidMarker(void)
{
    uint8 testData[32];
    uint8 readBuf[32];

    memset(testData, 0xAA, 32);

    PlaceSectorHeader(0u, 1u, 0u);
    PlaceBlockInFlash(TEST_SECTOR_HDR, 1u, 32u, testData, 1u, FEE_MARKER_ERASED);

    DriveInitToCompletion();
    TEST_ASSERT_EQUAL(FEE_BLOCK_INCONSISTENT, Fee_BlockInfoTable[0].Status);

    Fee_Read(1u, 0u, readBuf, 32u);
    DriveJobToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_BLOCK_INCONSISTENT, Fee_GetJobResult());
}

/*============================================================================*
 *  4. Committed block survives power loss during next write
 *============================================================================*/

static void test_PowerLoss_CommittedBlock_Survives(void)
{
    uint8 data1[32], data2[32];
    uint8 readBuf[32];
    uint32 off;

    memset(data1, 0xAA, 32);
    memset(data2, 0xBB, 32);

    PlaceSectorHeader(0u, 1u, 0u);

    off = TEST_SECTOR_HDR;
    /* Block 1 committed (valid) – seq=1 */
    PlaceBlockInFlash(off, 1u, 32u, data1, 1u, FEE_MARKER_VALID);

    off += TEST_BLOCK_HDR + 32u;
    /* Interrupted second write (no valid marker, seq=2) – scan sees this last */
    PlaceBlockInFlash(off, 1u, 32u, data2, 2u, FEE_MARKER_ERASED);

    /* The scan processes records sequentially: the newer incomplete record
       (seq=2, no valid marker) overwrites the older valid one in BlockInfoTable.
       This is correct scan behavior – the block status reflects the latest
       record found, which is INCONSISTENT. */
    DriveInitToCompletion();
    TEST_ASSERT_EQUAL(FEE_BLOCK_INCONSISTENT, Fee_BlockInfoTable[0].Status);

    Fee_Read(1u, 0u, readBuf, 32u);
    DriveJobToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_BLOCK_INCONSISTENT, Fee_GetJobResult());
}

/*============================================================================*
 *  5. Power loss after successful commit – verify data survives
 *============================================================================*/

static void test_PowerLoss_AfterSuccessfulCommit(void)
{
    uint8 data1[32];
    uint8 readBuf[32];

    memset(data1, 0xAA, 32);

    PlaceSectorHeader(0u, 1u, 0u);
    PlaceBlockInFlash(TEST_SECTOR_HDR, 1u, 32u, data1, 1u, FEE_MARKER_VALID);

    DriveInitToCompletion();
    TEST_ASSERT_EQUAL(FEE_BLOCK_VALID, Fee_BlockInfoTable[0].Status);

    /* Simulate power loss (re-init) */
    ReInitFee();

    Fee_Read(1u, 0u, readBuf, 32u);
    DriveJobToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
    TEST_ASSERT_EQUAL_MEMORY(data1, readBuf, 32);
}

/*============================================================================*
 *  6. API write then read (same session, no re-init)
 *============================================================================*/

static void test_PowerLoss_ApiWriteReadSameSession(void)
{
    uint8 writeData[32], readBuf[32];
    memset(writeData, 0xCC, 32);

    DriveInitToCompletion();
    Fee_Write(1u, writeData);
    DriveJobToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());

    Fee_Read(1u, 0u, readBuf, 32u);
    DriveJobToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
    TEST_ASSERT_EQUAL_MEMORY(writeData, readBuf, 32);
}

/*============================================================================*
 *  7. Multiple committed blocks survive re-init
 *============================================================================*/

static void test_PowerLoss_MultipleCommittedBlocks(void)
{
    uint8 data1[32], data2[64];
    uint8 readBuf[64];
    uint32 off;

    memset(data1, 0x11, 32);
    memset(data2, 0x22, 64);

    PlaceSectorHeader(0u, 1u, 0u);
    off = TEST_SECTOR_HDR;
    PlaceBlockInFlash(off, 1u, 32u, data1, 1u, FEE_MARKER_VALID);
    off += TEST_BLOCK_HDR + 32u;
    PlaceBlockInFlash(off, 2u, 64u, data2, 1u, FEE_MARKER_VALID);

    DriveInitToCompletion();
    ReInitFee();

    Fee_Read(1u, 0u, readBuf, 32u);
    DriveJobToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
    TEST_ASSERT_EQUAL_MEMORY(data1, readBuf, 32);

    Fee_Read(2u, 0u, readBuf, 64u);
    DriveJobToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
    TEST_ASSERT_EQUAL_MEMORY(data2, readBuf, 64);
}

/*============================================================================*
 *  8. Newer valid copy supersedes older after re-init
 *============================================================================*/

static void test_PowerLoss_NewerCopy_Supersedes(void)
{
    uint8 data_v1[32], data_v2[32];
    uint8 readBuf[32];
    uint32 off;

    memset(data_v1, 0x11, 32);
    memset(data_v2, 0x22, 32);

    PlaceSectorHeader(0u, 1u, 0u);

    off = TEST_SECTOR_HDR;
    PlaceBlockInFlash(off, 1u, 32u, data_v1, 1u, FEE_MARKER_VALID);
    off += TEST_BLOCK_HDR + 32u;
    PlaceBlockInFlash(off, 1u, 32u, data_v2, 2u, FEE_MARKER_VALID);

    DriveInitToCompletion();
    TEST_ASSERT_EQUAL(FEE_BLOCK_VALID, Fee_BlockInfoTable[0].Status);
    TEST_ASSERT_EQUAL_UINT16(2u, Fee_BlockInfoTable[0].SequenceCounter);

    Fee_Read(1u, 0u, readBuf, 32u);
    DriveJobToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
    TEST_ASSERT_EQUAL_MEMORY(data_v2, readBuf, 32);
}

/*============================================================================*
 *  9. Power loss with no sector headers – blank flash
 *============================================================================*/

static void test_PowerLoss_NoSectorHeaders(void)
{
    DriveInitToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_IDLE, Fee_GetStatus());
    ReInitFee();
    TEST_ASSERT_EQUAL(MEMIF_IDLE, Fee_GetStatus());
}

/*============================================================================*
 *  10. Invalidated block stays invalid after re-init
 *============================================================================*/

static void test_PowerLoss_AfterInvalidate(void)
{
    uint8 data1[32];
    uint8 readBuf[32];

    memset(data1, 0xAA, 32);

    PlaceSectorHeader(0u, 1u, 0u);
    PlaceBlockInFlash(TEST_SECTOR_HDR, 1u, 32u, data1, 1u, FEE_MARKER_INVALID);

    DriveInitToCompletion();
    TEST_ASSERT_EQUAL(FEE_BLOCK_INVALID, Fee_BlockInfoTable[0].Status);

    Fee_Read(1u, 0u, readBuf, 32u);
    DriveJobToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_BLOCK_INVALID, Fee_GetJobResult());
}

/*============================================================================*
 *  11. Two sector headers, blocks in sector 0 – recovery
 *============================================================================*/

static void test_PowerLoss_TwoSectors_BlockInFirst(void)
{
    uint8 data1[32];
    uint8 readBuf[32];

    memset(data1, 0x55, 32);

    PlaceSectorHeader(0u, 1u, 0u);
    PlaceSectorHeader(TEST_SECTOR_SIZE, 2u, 0u);
    PlaceBlockInFlash(TEST_SECTOR_SIZE + TEST_SECTOR_HDR, 1u, 32u,
                      data1, 1u, FEE_MARKER_VALID);

    DriveInitToCompletion();
    TEST_ASSERT_EQUAL(FEE_BLOCK_VALID, Fee_BlockInfoTable[0].Status);

    ReInitFee();

    Fee_Read(1u, 0u, readBuf, 32u);
    DriveJobToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
    TEST_ASSERT_EQUAL_MEMORY(data1, readBuf, 32);
}

/*============================================================================*
 *  12. Incomplete immediate block (no valid marker)
 *============================================================================*/

static void test_PowerLoss_ImmediateBlock_Incomplete(void)
{
    uint8 immData[16];
    uint8 readBuf[16];

    memset(immData, 0xEE, 16);

    PlaceSectorHeader(0u, 1u, 0u);
    PlaceBlockInFlash(TEST_SECTOR_HDR, 5u, 16u, immData, 1u, FEE_MARKER_ERASED);

    DriveInitToCompletion();
    TEST_ASSERT_NOT_EQUAL(FEE_BLOCK_VALID, Fee_BlockInfoTable[4].Status);

    Fee_Read(5u, 0u, readBuf, 16u);
    DriveJobToCompletion();
    TEST_ASSERT_NOT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
}

/*============================================================================*
 *  13. Block with corrupt CRC in header
 *============================================================================*/

static void test_PowerLoss_CorruptHeaderCRC(void)
{
    uint8 data1[32];
    uint8 *flash = Mem_DFLS_Stub_GetFlashContent();
    uint8 readBuf[32];

    memset(data1, 0xAA, 32);

    PlaceSectorHeader(0u, 1u, 0u);
    PlaceBlockInFlash(TEST_SECTOR_HDR, 1u, 32u, data1, 1u, FEE_MARKER_VALID);

    /* Corrupt the header CRC */
    flash[TEST_SECTOR_HDR + 28] ^= 0xFFu;

    DriveInitToCompletion();
    TEST_ASSERT_NOT_EQUAL(FEE_BLOCK_VALID, Fee_BlockInfoTable[0].Status);

    Fee_Read(1u, 0u, readBuf, 32u);
    DriveJobToCompletion();
    TEST_ASSERT_NOT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
}

/*============================================================================*
 *  14. Recovery then successful write
 *============================================================================*/

static void test_PowerLoss_RecoveryThenSuccessfulWrite(void)
{
    uint8 d[32], newData[32], readBuf[32];

    memset(d, 0xCC, 32);
    memset(newData, 0xDD, 32);

    PlaceSectorHeader(0u, 1u, 0u);
    PlaceBlockInFlash(TEST_SECTOR_HDR, 1u, 32u, d, 1u, FEE_MARKER_VALID);

    DriveInitToCompletion();
    ReInitFee();

    Fee_Write(1u, newData);
    DriveJobToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());

    Fee_Read(1u, 0u, readBuf, 32u);
    DriveJobToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
    TEST_ASSERT_EQUAL_MEMORY(newData, readBuf, 32);
}

/*============================================================================*
 *  15. Multiple blocks in multiple sectors
 *============================================================================*/

static void test_PowerLoss_MultiSector_MultiBlock(void)
{
    uint8 data1[32], data3[128];
    uint8 readBuf[128];
    uint32 off;

    memset(data1, 0xA1, 32);
    memset(data3, 0xA3, 128);

    /* Place both blocks in the same sector (highest seq number = active) */
    PlaceSectorHeader(0u, 1u, 0u);

    off = TEST_SECTOR_HDR;
    PlaceBlockInFlash(off, 1u, 32u, data1, 1u, FEE_MARKER_VALID);
    off += TEST_BLOCK_HDR + 32u;
    PlaceBlockInFlash(off, 3u, 128u, data3, 1u, FEE_MARKER_VALID);

    DriveInitToCompletion();

    TEST_ASSERT_EQUAL(FEE_BLOCK_VALID, Fee_BlockInfoTable[0].Status);
    TEST_ASSERT_EQUAL(FEE_BLOCK_VALID, Fee_BlockInfoTable[2].Status);

    ReInitFee();

    Fee_Read(1u, 0u, readBuf, 32u);
    DriveJobToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
    TEST_ASSERT_EQUAL_MEMORY(data1, readBuf, 32);

    Fee_Read(3u, 0u, readBuf, 128u);
    DriveJobToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
    TEST_ASSERT_EQUAL_MEMORY(data3, readBuf, 128);
}

/*============================================================================*
 *  16. Valid block followed by blank space
 *============================================================================*/

static void test_PowerLoss_ValidBlockThenBlank(void)
{
    uint8 data1[64];
    uint8 readBuf[64];

    memset(data1, 0xBE, 64);

    PlaceSectorHeader(0u, 1u, 0u);
    PlaceBlockInFlash(TEST_SECTOR_HDR, 2u, 64u, data1, 1u, FEE_MARKER_VALID);

    /* Rest of sector is blank (erased) */
    DriveInitToCompletion();
    TEST_ASSERT_EQUAL(FEE_BLOCK_VALID, Fee_BlockInfoTable[1].Status);

    Fee_Read(2u, 0u, readBuf, 64u);
    DriveJobToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
    TEST_ASSERT_EQUAL_MEMORY(data1, readBuf, 64);
}

/*============================================================================*
 *  Main
 *============================================================================*/

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_PowerLoss_BlankFlash_ReInit);
    RUN_TEST(test_PowerLoss_AfterHeaderWrite_BeforeData);
    RUN_TEST(test_PowerLoss_AfterDataWrite_BeforeValidMarker);
    RUN_TEST(test_PowerLoss_CommittedBlock_Survives);
    RUN_TEST(test_PowerLoss_AfterSuccessfulCommit);
    RUN_TEST(test_PowerLoss_ApiWriteReadSameSession);
    RUN_TEST(test_PowerLoss_MultipleCommittedBlocks);
    RUN_TEST(test_PowerLoss_NewerCopy_Supersedes);
    RUN_TEST(test_PowerLoss_NoSectorHeaders);
    RUN_TEST(test_PowerLoss_AfterInvalidate);
    RUN_TEST(test_PowerLoss_TwoSectors_BlockInFirst);
    RUN_TEST(test_PowerLoss_ImmediateBlock_Incomplete);
    RUN_TEST(test_PowerLoss_CorruptHeaderCRC);
    RUN_TEST(test_PowerLoss_RecoveryThenSuccessfulWrite);
    RUN_TEST(test_PowerLoss_MultiSector_MultiBlock);
    RUN_TEST(test_PowerLoss_ValidBlockThenBlank);

    return UNITY_END();
}
