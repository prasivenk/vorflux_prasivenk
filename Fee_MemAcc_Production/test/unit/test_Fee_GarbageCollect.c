/**
 * \file       test_Fee_GarbageCollect.c
 * \brief      Unit Tests -- Fee Interruptible Garbage Collection
 *
 * \details    Tests GC trigger (source/target sector selection), single and
 *             multi-block copy, mixed block types, suspend/resume, error
 *             handling, wear leveling, stale pointer cleanup, CRC mismatch,
 *             and IsActive state queries.
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
#include <string.h>

/*============================================================================*
 *  Constants
 *============================================================================*/

/** \brief Sector size from configuration */
#define TEST_SECTOR_SIZE     FEE_SECTOR_SIZE    /* 16384 */

/** \brief Sector header size */
#define TEST_SECTOR_HDR      FEE_SECTOR_HEADER_SIZE  /* 32 */

/** \brief Block header size */
#define TEST_BLOCK_HDR       FEE_BLOCK_HEADER_SIZE   /* 32 */

/** \brief Virtual page size */
#define TEST_PAGE_SIZE       FEE_VIRTUAL_PAGE_SIZE   /* 32 */

/*============================================================================*
 *  Helper: drive one main cycle (MemAcc + Fee)
 *============================================================================*/

static void DriveOneCycle(void)
{
    MemAcc_MainFunction();
    Fee_MainFunction();
}

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
        DriveOneCycle();
        if (Fee_GetStatus() == MEMIF_IDLE)
        {
            break;
        }
    }
}

/*============================================================================*
 *  Helper: build a sector header in flash at given offset
 *============================================================================*/

static void PlaceSectorHeader(uint32 flashOffset, uint32 seqNum, uint16 eraseCount)
{
    uint8 hdr[32];
    uint8 *flash = Mem_DFLS_Stub_GetFlashContent();

    Fee_Sector_BuildSectorHeader(hdr, seqNum, eraseCount);
    memcpy(&flash[flashOffset], hdr, 32);
}

/*============================================================================*
 *  Helper: build a block header + data in flash
 *============================================================================*/

static void PlaceBlockInFlash(uint32 flashOffset, uint16 blockNum, uint16 blockSize,
                               const uint8 *data, uint16 seqCounter,
                               uint8 validMarker)
{
    uint8 hdr[32];
    uint8 *flash = Mem_DFLS_Stub_GetFlashContent();
    uint16 dataCrc;

    dataCrc = Fee_Crc_CalculateBlock(data, (uint32)blockSize);

    Fee_Sector_BuildBlockHeader(hdr, blockNum, blockSize, dataCrc, seqCounter, 0u);
    /* Set valid marker */
    hdr[30] = validMarker;

    memcpy(&flash[flashOffset], hdr, 32);
    memcpy(&flash[flashOffset + 32], data, blockSize);
}

/*============================================================================*
 *  Helper: drive a single GC step (MemAcc_MainFunction + GC_Process)
 *  Returns the GC result. Calls MemAcc_MainFunction first to complete
 *  any pending MemAcc job, then calls Fee_GarbageCollect_Process().
 *============================================================================*/

static Fee_GcResultType DriveGcStep(void)
{
    MemAcc_MainFunction();
    return Fee_GarbageCollect_Process();
}

/*============================================================================*
 *  Helper: drive GC to completion (max cycles)
 *============================================================================*/

static Fee_GcResultType DriveGcToCompletion(uint32 maxCycles)
{
    Fee_GcResultType result;
    uint32 cycle;

    for (cycle = 0u; cycle < maxCycles; cycle++)
    {
        result = DriveGcStep();
        if (result != FEE_GC_IN_PROGRESS)
        {
            return result;
        }
    }
    return FEE_GC_IN_PROGRESS;
}

/*============================================================================*
 *  Helper: align up to virtual page
 *============================================================================*/

static uint32 AlignUp(uint32 value, uint32 alignment)
{
    return ((value + alignment - 1u) & ~(alignment - 1u));
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
 *  1. Init: GC starts in IDLE, IsActive returns FALSE
 *============================================================================*/

static void test_GC_Init_IsNotActive(void)
{
    Fee_GarbageCollect_Init();
    TEST_ASSERT_FALSE(Fee_GarbageCollect_IsActive());
}

/*============================================================================*
 *  2. GC Process when IDLE returns COMPLETE
 *============================================================================*/

static void test_GC_Process_WhenIdle_ReturnsComplete(void)
{
    Fee_GarbageCollect_Init();
    TEST_ASSERT_EQUAL(FEE_GC_COMPLETE, Fee_GarbageCollect_Process());
}

/*============================================================================*
 *  3. GC Trigger: source = fullest ACTIVE/FULL sector
 *============================================================================*/

static void test_GC_Trigger_SelectsFullestSource(void)
{
    uint8 data[32];
    memset(data, 0xAA, 32);

    /* Sector 0: active with one block */
    PlaceSectorHeader(0u, 1u, 0u);
    PlaceBlockInFlash(TEST_SECTOR_HDR, 1u, 32u, data, 1u, FEE_MARKER_VALID);

    /* Sector 1: active with two blocks (fullest) */
    PlaceSectorHeader(TEST_SECTOR_SIZE, 2u, 0u);
    PlaceBlockInFlash(TEST_SECTOR_SIZE + TEST_SECTOR_HDR, 2u, 64u, data, 1u,
                      FEE_MARKER_VALID);
    PlaceBlockInFlash(TEST_SECTOR_SIZE + TEST_SECTOR_HDR + TEST_BLOCK_HDR + 64u,
                      3u, 128u, data, 1u, FEE_MARKER_VALID);

    /* Sector 2: erased (target) */
    /* (no header = erased) */

    DriveInitToCompletion();

    Fee_GarbageCollect_Init();
    Std_ReturnType ret = Fee_GarbageCollect_Trigger();
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_TRUE(Fee_GarbageCollect_IsActive());
}

/*============================================================================*
 *  4. GC Trigger: target = ERASED sector with lowest erase count
 *============================================================================*/

static void test_GC_Trigger_WearLeveling_LowestEraseCount(void)
{
    uint8 data[32];
    memset(data, 0xAA, 32);

    /* Sector 0: active with a block (source) */
    PlaceSectorHeader(0u, 1u, 0u);
    PlaceBlockInFlash(TEST_SECTOR_HDR, 1u, 32u, data, 1u, FEE_MARKER_VALID);

    /* Sector 1: erased, erase count = 10 (higher) */
    /* Sector 2: erased, erase count = 5 (lower - should be selected) */
    /* Sector 3: erased, erase count = 8 */

    DriveInitToCompletion();

    /* Manually set erase counts for erased sectors */
    Fee_SectorInfo[1].EraseCount = 10u;
    Fee_SectorInfo[2].EraseCount = 5u;
    Fee_SectorInfo[3].EraseCount = 8u;

    Fee_GarbageCollect_Init();
    Std_ReturnType ret = Fee_GarbageCollect_Trigger();
    TEST_ASSERT_EQUAL(E_OK, ret);

    /* Drive GC to completion to verify it writes to a sector */
    Fee_GcResultType gcResult = DriveGcToCompletion(200u);
    TEST_ASSERT_EQUAL(FEE_GC_COMPLETE, gcResult);

    /* Block 1 (index 0) should now be in sector 2 (lowest erase count) */
    TEST_ASSERT_EQUAL(2u, Fee_BlockInfoTable[0].SectorIndex);
}

/*============================================================================*
 *  5. Single-block copy: all state transitions
 *============================================================================*/

static void test_GC_SingleBlock_FullCycle(void)
{
    uint8 data[32];
    uint8 readBuf[32];
    memset(data, 0x42, 32);

    /* Sector 0: active with one valid block */
    PlaceSectorHeader(0u, 1u, 0u);
    PlaceBlockInFlash(TEST_SECTOR_HDR, 1u, 32u, data, 1u, FEE_MARKER_VALID);

    /* Sector 1: erased (target) */

    DriveInitToCompletion();

    /* Verify initial state */
    TEST_ASSERT_EQUAL(FEE_BLOCK_VALID, Fee_BlockInfoTable[0].Status);
    TEST_ASSERT_EQUAL(0u, Fee_BlockInfoTable[0].SectorIndex);

    Fee_GarbageCollect_Init();
    Fee_GarbageCollect_Trigger();

    /* Drive GC to completion */
    Fee_GcResultType result = DriveGcToCompletion(200u);
    TEST_ASSERT_EQUAL(FEE_GC_COMPLETE, result);

    /* Block should be moved to sector 1 */
    TEST_ASSERT_EQUAL(FEE_BLOCK_VALID, Fee_BlockInfoTable[0].Status);
    TEST_ASSERT_EQUAL(1u, Fee_BlockInfoTable[0].SectorIndex);

    /* Verify data is intact by reading from new location */
    Fee_Read(1u, 0u, readBuf, 32u);
    uint32 cycle;
    for (cycle = 0u; cycle < 200u; cycle++)
    {
        DriveOneCycle();
        if (Fee_GetStatus() == MEMIF_IDLE)
        {
            break;
        }
    }
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
    TEST_ASSERT_EQUAL_MEMORY(data, readBuf, 32);
}

/*============================================================================*
 *  6. Multi-block copy: 3 valid blocks in source
 *============================================================================*/

static void test_GC_MultiBlock_AllCopied(void)
{
    uint8 data1[32], data2[64], data3[128];
    uint32 offset;

    memset(data1, 0x11, 32);
    memset(data2, 0x22, 64);
    memset(data3, 0x33, 128);

    /* Sector 0: active with 3 blocks */
    PlaceSectorHeader(0u, 1u, 0u);

    offset = TEST_SECTOR_HDR;
    PlaceBlockInFlash(offset, 1u, 32u, data1, 1u, FEE_MARKER_VALID);
    offset += TEST_BLOCK_HDR + AlignUp(32u, TEST_PAGE_SIZE);

    PlaceBlockInFlash(offset, 2u, 64u, data2, 1u, FEE_MARKER_VALID);
    offset += TEST_BLOCK_HDR + AlignUp(64u, TEST_PAGE_SIZE);

    PlaceBlockInFlash(offset, 3u, 128u, data3, 1u, FEE_MARKER_VALID);

    /* Sector 1: erased */

    DriveInitToCompletion();

    TEST_ASSERT_EQUAL(FEE_BLOCK_VALID, Fee_BlockInfoTable[0].Status);
    TEST_ASSERT_EQUAL(FEE_BLOCK_VALID, Fee_BlockInfoTable[1].Status);
    TEST_ASSERT_EQUAL(FEE_BLOCK_VALID, Fee_BlockInfoTable[2].Status);

    Fee_GarbageCollect_Init();
    Fee_GarbageCollect_Trigger();

    Fee_GcResultType result = DriveGcToCompletion(500u);
    TEST_ASSERT_EQUAL(FEE_GC_COMPLETE, result);

    /* All 3 blocks should be in sector 1 */
    TEST_ASSERT_EQUAL(1u, Fee_BlockInfoTable[0].SectorIndex);
    TEST_ASSERT_EQUAL(1u, Fee_BlockInfoTable[1].SectorIndex);
    TEST_ASSERT_EQUAL(1u, Fee_BlockInfoTable[2].SectorIndex);

    /* All should still be VALID */
    TEST_ASSERT_EQUAL(FEE_BLOCK_VALID, Fee_BlockInfoTable[0].Status);
    TEST_ASSERT_EQUAL(FEE_BLOCK_VALID, Fee_BlockInfoTable[1].Status);
    TEST_ASSERT_EQUAL(FEE_BLOCK_VALID, Fee_BlockInfoTable[2].Status);
}

/*============================================================================*
 *  7. GC with mixed blocks: valid + invalid + inconsistent
 *     Only valid blocks should be copied
 *============================================================================*/

static void test_GC_MixedBlocks_OnlyValidCopied(void)
{
    uint8 data1[32], data2[64], data3[128];
    uint32 offset;

    memset(data1, 0x11, 32);
    memset(data2, 0x22, 64);
    memset(data3, 0x33, 128);

    /* Sector 0: 3 blocks - valid, invalid, inconsistent */
    PlaceSectorHeader(0u, 1u, 0u);

    offset = TEST_SECTOR_HDR;
    PlaceBlockInFlash(offset, 1u, 32u, data1, 1u, FEE_MARKER_VALID);
    offset += TEST_BLOCK_HDR + AlignUp(32u, TEST_PAGE_SIZE);

    PlaceBlockInFlash(offset, 2u, 64u, data2, 1u, FEE_MARKER_INVALID);
    offset += TEST_BLOCK_HDR + AlignUp(64u, TEST_PAGE_SIZE);

    PlaceBlockInFlash(offset, 3u, 128u, data3, 1u, FEE_MARKER_ERASED);

    /* Sector 1: erased */

    DriveInitToCompletion();

    TEST_ASSERT_EQUAL(FEE_BLOCK_VALID, Fee_BlockInfoTable[0].Status);
    TEST_ASSERT_EQUAL(FEE_BLOCK_INVALID, Fee_BlockInfoTable[1].Status);
    TEST_ASSERT_EQUAL(FEE_BLOCK_INCONSISTENT, Fee_BlockInfoTable[2].Status);

    Fee_GarbageCollect_Init();
    Fee_GarbageCollect_Trigger();

    Fee_GcResultType result = DriveGcToCompletion(500u);
    TEST_ASSERT_EQUAL(FEE_GC_COMPLETE, result);

    /* Only block 1 should be in sector 1 (copied) */
    TEST_ASSERT_EQUAL(FEE_BLOCK_VALID, Fee_BlockInfoTable[0].Status);
    TEST_ASSERT_EQUAL(1u, Fee_BlockInfoTable[0].SectorIndex);

    /* Blocks 2 and 3 should be NOT_FOUND (stale pointers cleared by erase) */
    TEST_ASSERT_EQUAL(FEE_BLOCK_NOT_FOUND, Fee_BlockInfoTable[1].Status);
    TEST_ASSERT_EQUAL(FEE_BLOCK_NOT_FOUND, Fee_BlockInfoTable[2].Status);
}

/*============================================================================*
 *  8. Suspend/Resume: GC still active when suspended
 *============================================================================*/

static void test_GC_SuspendResume(void)
{
    uint8 data[32];
    memset(data, 0xAA, 32);

    PlaceSectorHeader(0u, 1u, 0u);
    PlaceBlockInFlash(TEST_SECTOR_HDR, 1u, 32u, data, 1u, FEE_MARKER_VALID);

    DriveInitToCompletion();

    Fee_GarbageCollect_Init();
    Fee_GarbageCollect_Trigger();

    /* Process a few steps */
    Fee_GcResultType result = DriveGcStep();  /* SELECT_SOURCE */
    TEST_ASSERT_EQUAL(FEE_GC_IN_PROGRESS, result);
    TEST_ASSERT_TRUE(Fee_GarbageCollect_IsActive());

    /* Suspend */
    Fee_GarbageCollect_Suspend();

    /* Process should return IN_PROGRESS but NOT advance state */
    result = DriveGcStep();
    TEST_ASSERT_EQUAL(FEE_GC_IN_PROGRESS, result);
    TEST_ASSERT_TRUE(Fee_GarbageCollect_IsActive());

    /* Process again -- still suspended */
    result = DriveGcStep();
    TEST_ASSERT_EQUAL(FEE_GC_IN_PROGRESS, result);

    /* Resume */
    Fee_GarbageCollect_Resume();

    /* Should now complete */
    result = DriveGcToCompletion(200u);
    TEST_ASSERT_EQUAL(FEE_GC_COMPLETE, result);
    TEST_ASSERT_FALSE(Fee_GarbageCollect_IsActive());
}

/*============================================================================*
 *  9. MemAcc failure during copy read
 *============================================================================*/

static void test_GC_Error_CopyReadFailure(void)
{
    uint8 data[32];
    memset(data, 0xAA, 32);

    PlaceSectorHeader(0u, 1u, 0u);
    PlaceBlockInFlash(TEST_SECTOR_HDR, 1u, 32u, data, 1u, FEE_MARKER_VALID);

    DriveInitToCompletion();

    Fee_GarbageCollect_Init();
    Fee_GarbageCollect_Trigger();
    Dem_Stub_Reset();

    /* SELECT_SOURCE -> finds block */
    DriveGcStep();

    /* COPY_READ -> initiate read */
    DriveGcStep();

    /* Inject failure for the MemAcc read job */
    Mem_DFLS_Stub_SetJobResult(MEM_DFLS_JOB_FAILED);

    /* COPY_READ_WAIT -> poll fails */
    Fee_GcResultType result = DriveGcStep();
    TEST_ASSERT_EQUAL(FEE_GC_ERROR, result);
    TEST_ASSERT_TRUE(Dem_Stub_WasEventReported(FEE_E_HARDWARE_ERROR,
                     DEM_EVENT_STATUS_FAILED));
}

/*============================================================================*
 *  10. MemAcc failure during copy write header
 *============================================================================*/

static void test_GC_Error_CopyWriteHeaderFailure(void)
{
    uint8 data[32];
    memset(data, 0xAA, 32);

    PlaceSectorHeader(0u, 1u, 0u);
    PlaceBlockInFlash(TEST_SECTOR_HDR, 1u, 32u, data, 1u, FEE_MARKER_VALID);

    DriveInitToCompletion();

    Fee_GarbageCollect_Init();
    Fee_GarbageCollect_Trigger();
    Dem_Stub_Reset();

    /* SELECT_SOURCE */
    DriveGcStep();
    /* COPY_READ */
    DriveGcStep();
    /* COPY_READ_WAIT -> OK, CRC OK */
    DriveGcStep();
    /* COPY_WRITE_HEADER -> initiate header write */
    DriveGcStep();

    /* Inject failure for header write job */
    Mem_DFLS_Stub_SetJobResult(MEM_DFLS_JOB_FAILED);

    /* COPY_WRITE_HEADER_WAIT -> poll fails */
    Fee_GcResultType result = DriveGcStep();
    TEST_ASSERT_EQUAL(FEE_GC_ERROR, result);
    TEST_ASSERT_TRUE(Dem_Stub_WasEventReported(FEE_E_HARDWARE_ERROR,
                     DEM_EVENT_STATUS_FAILED));
}

/*============================================================================*
 *  11. MemAcc failure during copy write data
 *============================================================================*/

static void test_GC_Error_CopyWriteDataFailure(void)
{
    uint8 data[32];
    memset(data, 0xAA, 32);

    PlaceSectorHeader(0u, 1u, 0u);
    PlaceBlockInFlash(TEST_SECTOR_HDR, 1u, 32u, data, 1u, FEE_MARKER_VALID);

    DriveInitToCompletion();

    Fee_GarbageCollect_Init();
    Fee_GarbageCollect_Trigger();
    Dem_Stub_Reset();

    /* SELECT_SOURCE */
    DriveGcStep();
    /* COPY_READ */
    DriveGcStep();
    /* COPY_READ_WAIT -> OK */
    DriveGcStep();
    /* COPY_WRITE_HEADER */
    DriveGcStep();
    /* COPY_WRITE_HEADER_WAIT -> OK */
    DriveGcStep();
    /* COPY_WRITE_DATA -> initiate data write */
    DriveGcStep();

    /* Inject failure for data write job */
    Mem_DFLS_Stub_SetJobResult(MEM_DFLS_JOB_FAILED);

    /* COPY_WRITE_DATA_WAIT -> poll fails */
    Fee_GcResultType result = DriveGcStep();
    TEST_ASSERT_EQUAL(FEE_GC_ERROR, result);
    TEST_ASSERT_TRUE(Dem_Stub_WasEventReported(FEE_E_HARDWARE_ERROR,
                     DEM_EVENT_STATUS_FAILED));
}

/*============================================================================*
 *  12. MemAcc failure during copy write valid marker
 *============================================================================*/

static void test_GC_Error_CopyWriteValidFailure(void)
{
    uint8 data[32];
    memset(data, 0xAA, 32);

    PlaceSectorHeader(0u, 1u, 0u);
    PlaceBlockInFlash(TEST_SECTOR_HDR, 1u, 32u, data, 1u, FEE_MARKER_VALID);

    DriveInitToCompletion();

    Fee_GarbageCollect_Init();
    Fee_GarbageCollect_Trigger();
    Dem_Stub_Reset();

    /* Drive through to COPY_WRITE_VALID */
    DriveGcStep(); /* SELECT_SOURCE */
    DriveGcStep(); /* COPY_READ */
    DriveGcStep(); /* COPY_READ_WAIT -> OK */
    DriveGcStep(); /* COPY_WRITE_HEADER */
    DriveGcStep(); /* COPY_WRITE_HEADER_WAIT -> OK */
    DriveGcStep(); /* COPY_WRITE_DATA */
    DriveGcStep(); /* COPY_WRITE_DATA_WAIT -> OK */
    DriveGcStep(); /* COPY_WRITE_VALID -> write valid marker */

    /* Inject failure */
    Mem_DFLS_Stub_SetJobResult(MEM_DFLS_JOB_FAILED);

    /* COPY_WRITE_VALID_WAIT -> poll fails */
    Fee_GcResultType result = DriveGcStep();
    TEST_ASSERT_EQUAL(FEE_GC_ERROR, result);
    TEST_ASSERT_TRUE(Dem_Stub_WasEventReported(FEE_E_HARDWARE_ERROR,
                     DEM_EVENT_STATUS_FAILED));
}

/*============================================================================*
 *  13. MemAcc failure during erase
 *============================================================================*/

static void test_GC_Error_EraseFailure(void)
{
    uint8 data[32];
    memset(data, 0xAA, 32);

    PlaceSectorHeader(0u, 1u, 0u);
    /* Only invalid blocks -> GC skips copy, goes straight to erase */
    PlaceBlockInFlash(TEST_SECTOR_HDR, 1u, 32u, data, 1u, FEE_MARKER_INVALID);

    DriveInitToCompletion();

    Fee_GarbageCollect_Init();
    Fee_GarbageCollect_Trigger();
    Dem_Stub_Reset();

    /* Drive through SELECT_SOURCE (scans all, finds none valid -> ERASE_SOURCE) */
    DriveGcStep(); /* SELECT_SOURCE -> no valid -> ERASE_SOURCE */
    DriveGcStep(); /* ERASE_SOURCE -> initiate erase */

    /* Inject failure for erase job */
    Mem_DFLS_Stub_SetJobResult(MEM_DFLS_JOB_FAILED);

    /* ERASE_WAIT -> poll fails */
    Fee_GcResultType result = DriveGcStep();
    TEST_ASSERT_EQUAL(FEE_GC_ERROR, result);
    TEST_ASSERT_TRUE(Dem_Stub_WasEventReported(FEE_E_HARDWARE_ERROR,
                     DEM_EVENT_STATUS_FAILED));
}

/*============================================================================*
 *  14. Stale pointer cleanup after erase
 *============================================================================*/

static void test_GC_StalePointerCleanup(void)
{
    uint8 data1[32], data2[64];
    uint32 offset;
    memset(data1, 0x11, 32);
    memset(data2, 0x22, 64);

    /* Sector 0: valid block 1, invalid block 2 */
    PlaceSectorHeader(0u, 1u, 0u);
    offset = TEST_SECTOR_HDR;
    PlaceBlockInFlash(offset, 1u, 32u, data1, 1u, FEE_MARKER_VALID);
    offset += TEST_BLOCK_HDR + AlignUp(32u, TEST_PAGE_SIZE);
    PlaceBlockInFlash(offset, 2u, 64u, data2, 1u, FEE_MARKER_INVALID);

    DriveInitToCompletion();

    /* Block 2 (invalid) is in sector 0 */
    TEST_ASSERT_EQUAL(FEE_BLOCK_INVALID, Fee_BlockInfoTable[1].Status);
    TEST_ASSERT_EQUAL(0u, Fee_BlockInfoTable[1].SectorIndex);

    Fee_GarbageCollect_Init();
    Fee_GarbageCollect_Trigger();

    Fee_GcResultType result = DriveGcToCompletion(500u);
    TEST_ASSERT_EQUAL(FEE_GC_COMPLETE, result);

    /* Block 1 should be moved to target sector */
    TEST_ASSERT_EQUAL(FEE_BLOCK_VALID, Fee_BlockInfoTable[0].Status);
    TEST_ASSERT_NOT_EQUAL(0u, Fee_BlockInfoTable[0].SectorIndex);

    /* Block 2 (was invalid in sector 0) should be cleaned up */
    TEST_ASSERT_EQUAL(FEE_BLOCK_NOT_FOUND, Fee_BlockInfoTable[1].Status);
    TEST_ASSERT_EQUAL(0u, Fee_BlockInfoTable[1].HeaderAddress);
    TEST_ASSERT_EQUAL(0u, Fee_BlockInfoTable[1].DataAddress);
    TEST_ASSERT_EQUAL(0u, Fee_BlockInfoTable[1].DataCrc);
}

/*============================================================================*
 *  15. CRC mismatch during copy: block marked INCONSISTENT and skipped
 *============================================================================*/

static void test_GC_CrcMismatch_BlockSkipped(void)
{
    uint8 data[32];
    uint8 *flash;
    memset(data, 0x42, 32);

    PlaceSectorHeader(0u, 1u, 0u);
    PlaceBlockInFlash(TEST_SECTOR_HDR, 1u, 32u, data, 1u, FEE_MARKER_VALID);

    DriveInitToCompletion();

    /* Corrupt data in flash AFTER init (so CRC in BlockInfoTable is correct,
       but actual flash data differs) */
    flash = Mem_DFLS_Stub_GetFlashContent();
    flash[TEST_SECTOR_HDR + TEST_BLOCK_HDR] = 0xFFu;

    Fee_GarbageCollect_Init();
    Fee_GarbageCollect_Trigger();

    Fee_GcResultType result = DriveGcToCompletion(200u);
    TEST_ASSERT_EQUAL(FEE_GC_COMPLETE, result);

    /* Block should be marked INCONSISTENT (CRC mismatch during GC copy) */
    /* After erase, it should be NOT_FOUND (stale pointer cleanup) */
    TEST_ASSERT_EQUAL(FEE_BLOCK_NOT_FOUND, Fee_BlockInfoTable[0].Status);
}

/*============================================================================*
 *  16. GC with no valid blocks: just erase (no copies)
 *============================================================================*/

static void test_GC_NoValidBlocks_JustErase(void)
{
    uint8 data[32];
    memset(data, 0xAA, 32);

    /* Sector 0: only invalid blocks */
    PlaceSectorHeader(0u, 1u, 0u);
    PlaceBlockInFlash(TEST_SECTOR_HDR, 1u, 32u, data, 1u, FEE_MARKER_INVALID);

    DriveInitToCompletion();

    TEST_ASSERT_EQUAL(FEE_BLOCK_INVALID, Fee_BlockInfoTable[0].Status);

    Fee_GarbageCollect_Init();
    Fee_GarbageCollect_Trigger();

    Fee_GcResultType result = DriveGcToCompletion(200u);
    TEST_ASSERT_EQUAL(FEE_GC_COMPLETE, result);

    /* Source sector should be erased, block should be NOT_FOUND */
    TEST_ASSERT_EQUAL(FEE_BLOCK_NOT_FOUND, Fee_BlockInfoTable[0].Status);
    TEST_ASSERT_EQUAL(FEE_SECTOR_ERASED, Fee_SectorInfo[0].Status);
}

/*============================================================================*
 *  17. IsActive: true during GC, false after completion
 *============================================================================*/

static void test_GC_IsActive_TrueDuringGC(void)
{
    uint8 data[32];
    memset(data, 0x42, 32);

    PlaceSectorHeader(0u, 1u, 0u);
    PlaceBlockInFlash(TEST_SECTOR_HDR, 1u, 32u, data, 1u, FEE_MARKER_VALID);

    DriveInitToCompletion();

    Fee_GarbageCollect_Init();
    TEST_ASSERT_FALSE(Fee_GarbageCollect_IsActive());

    Fee_GarbageCollect_Trigger();
    TEST_ASSERT_TRUE(Fee_GarbageCollect_IsActive());

    /* Drive a few steps -- still active */
    DriveGcStep();
    TEST_ASSERT_TRUE(Fee_GarbageCollect_IsActive());

    /* Complete */
    DriveGcToCompletion(200u);
    TEST_ASSERT_FALSE(Fee_GarbageCollect_IsActive());
}

/*============================================================================*
 *  18. Erase count incremented after GC
 *============================================================================*/

static void test_GC_EraseCountIncremented(void)
{
    uint8 data[32];
    memset(data, 0xAA, 32);

    PlaceSectorHeader(0u, 1u, 5u); /* erase count = 5 */
    PlaceBlockInFlash(TEST_SECTOR_HDR, 1u, 32u, data, 1u, FEE_MARKER_VALID);

    DriveInitToCompletion();

    TEST_ASSERT_EQUAL(5u, Fee_SectorInfo[0].EraseCount);

    Fee_GarbageCollect_Init();
    Fee_GarbageCollect_Trigger();

    Fee_GcResultType result = DriveGcToCompletion(200u);
    TEST_ASSERT_EQUAL(FEE_GC_COMPLETE, result);

    /* Erase count should be incremented */
    TEST_ASSERT_EQUAL(6u, Fee_SectorInfo[0].EraseCount);
}

/*============================================================================*
 *  19. Source sector status after GC = ERASED
 *============================================================================*/

static void test_GC_SourceSector_BecomeErased(void)
{
    uint8 data[32];
    memset(data, 0xAA, 32);

    PlaceSectorHeader(0u, 1u, 0u);
    PlaceBlockInFlash(TEST_SECTOR_HDR, 1u, 32u, data, 1u, FEE_MARKER_VALID);

    DriveInitToCompletion();

    TEST_ASSERT_EQUAL(FEE_SECTOR_ACTIVE, Fee_SectorInfo[0].Status);

    Fee_GarbageCollect_Init();
    Fee_GarbageCollect_Trigger();

    Fee_GcResultType result = DriveGcToCompletion(200u);
    TEST_ASSERT_EQUAL(FEE_GC_COMPLETE, result);

    /* Source sector 0 should now be ERASED */
    TEST_ASSERT_EQUAL(FEE_SECTOR_ERASED, Fee_SectorInfo[0].Status);

    /* WritePointer should be reset */
    TEST_ASSERT_EQUAL(
        Fee_SectorInfo[0].BaseAddress + TEST_SECTOR_HDR,
        Fee_SectorInfo[0].WritePointer);

    /* FreeSpace should be reset */
    TEST_ASSERT_EQUAL(
        (uint16)(TEST_SECTOR_SIZE - TEST_SECTOR_HDR),
        Fee_SectorInfo[0].FreeSpace);
}

/*============================================================================*
 *  20. GC Trigger fails when no source sector
 *============================================================================*/

static void test_GC_Trigger_NoSource_Fails(void)
{
    /* All sectors erased (blank flash) */
    DriveInitToCompletion();

    Fee_GarbageCollect_Init();
    Std_ReturnType ret = Fee_GarbageCollect_Trigger();
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_FALSE(Fee_GarbageCollect_IsActive());
}

/*============================================================================*
 *  21. GC Trigger fails when no target (erased) sector
 *============================================================================*/

static void test_GC_Trigger_NoTarget_Fails(void)
{
    uint8 data[32];
    memset(data, 0xAA, 32);

    /* All 4 sectors active */
    PlaceSectorHeader(0u, 1u, 0u);
    PlaceBlockInFlash(TEST_SECTOR_HDR, 1u, 32u, data, 1u, FEE_MARKER_VALID);

    PlaceSectorHeader(TEST_SECTOR_SIZE, 2u, 0u);
    PlaceBlockInFlash(TEST_SECTOR_SIZE + TEST_SECTOR_HDR, 2u, 64u, data, 1u,
                      FEE_MARKER_VALID);

    PlaceSectorHeader(2u * TEST_SECTOR_SIZE, 3u, 0u);
    PlaceBlockInFlash(2u * TEST_SECTOR_SIZE + TEST_SECTOR_HDR, 3u, 128u, data,
                      1u, FEE_MARKER_VALID);

    PlaceSectorHeader(3u * TEST_SECTOR_SIZE, 4u, 0u);
    PlaceBlockInFlash(3u * TEST_SECTOR_SIZE + TEST_SECTOR_HDR, 4u, 256u, data,
                      1u, FEE_MARKER_VALID);

    DriveInitToCompletion();

    Fee_GarbageCollect_Init();
    Std_ReturnType ret = Fee_GarbageCollect_Trigger();
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/*============================================================================*
 *  22. Block data integrity after GC (read-back verify)
 *============================================================================*/

static void test_GC_DataIntegrity_Readback(void)
{
    uint8 data[64];
    uint8 readBuf[64];
    uint8 i;

    for (i = 0u; i < 64u; i++)
    {
        data[i] = i;
    }

    PlaceSectorHeader(0u, 1u, 0u);
    PlaceBlockInFlash(TEST_SECTOR_HDR, 2u, 64u, data, 3u, FEE_MARKER_VALID);

    DriveInitToCompletion();

    Fee_GarbageCollect_Init();
    Fee_GarbageCollect_Trigger();
    DriveGcToCompletion(500u);

    /* Read back from new location */
    Fee_Read(2u, 0u, readBuf, 64u);
    uint32 cycle;
    for (cycle = 0u; cycle < 200u; cycle++)
    {
        DriveOneCycle();
        if (Fee_GetStatus() == MEMIF_IDLE)
        {
            break;
        }
    }
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
    TEST_ASSERT_EQUAL_MEMORY(data, readBuf, 64);
}

/*============================================================================*
 *  23. Sequence counter preserved after GC
 *============================================================================*/

static void test_GC_SequenceCounter_Preserved(void)
{
    uint8 data[32];
    memset(data, 0xBB, 32);

    PlaceSectorHeader(0u, 1u, 0u);
    PlaceBlockInFlash(TEST_SECTOR_HDR, 1u, 32u, data, 42u, FEE_MARKER_VALID);

    DriveInitToCompletion();

    TEST_ASSERT_EQUAL_UINT16(42u, Fee_BlockInfoTable[0].SequenceCounter);

    Fee_GarbageCollect_Init();
    Fee_GarbageCollect_Trigger();
    DriveGcToCompletion(200u);

    /* Sequence counter should be preserved */
    TEST_ASSERT_EQUAL_UINT16(42u, Fee_BlockInfoTable[0].SequenceCounter);
}

/*============================================================================*
 *  24. DataCrc preserved after GC
 *============================================================================*/

static void test_GC_DataCrc_Preserved(void)
{
    uint8 data[32];
    uint16 expectedCrc;
    memset(data, 0xCC, 32);
    expectedCrc = Fee_Crc_CalculateBlock(data, 32u);

    PlaceSectorHeader(0u, 1u, 0u);
    PlaceBlockInFlash(TEST_SECTOR_HDR, 1u, 32u, data, 1u, FEE_MARKER_VALID);

    DriveInitToCompletion();

    TEST_ASSERT_EQUAL_UINT16(expectedCrc, Fee_BlockInfoTable[0].DataCrc);

    Fee_GarbageCollect_Init();
    Fee_GarbageCollect_Trigger();
    DriveGcToCompletion(200u);

    /* CRC should be preserved */
    TEST_ASSERT_EQUAL_UINT16(expectedCrc, Fee_BlockInfoTable[0].DataCrc);
}

/*============================================================================*
 *  25. GC with immediate data block (small block)
 *============================================================================*/

static void test_GC_ImmediateBlock(void)
{
    uint8 data[16];
    uint8 readBuf[16];
    memset(data, 0xDD, 16);

    PlaceSectorHeader(0u, 1u, 0u);
    /* Block 5 is 16 bytes, immediate */
    PlaceBlockInFlash(TEST_SECTOR_HDR, 5u, 16u, data, 1u, FEE_MARKER_VALID);

    DriveInitToCompletion();

    Fee_GarbageCollect_Init();
    Fee_GarbageCollect_Trigger();

    Fee_GcResultType result = DriveGcToCompletion(200u);
    TEST_ASSERT_EQUAL(FEE_GC_COMPLETE, result);

    /* Read back */
    Fee_Read(5u, 0u, readBuf, 16u);
    uint32 cycle;
    for (cycle = 0u; cycle < 200u; cycle++)
    {
        DriveOneCycle();
        if (Fee_GetStatus() == MEMIF_IDLE)
        {
            break;
        }
    }
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
    TEST_ASSERT_EQUAL_MEMORY(data, readBuf, 16);
}

/*============================================================================*
 *  26. GC with large block (512 bytes)
 *============================================================================*/

static void test_GC_LargeBlock(void)
{
    uint8 data[512];
    uint8 readBuf[512];
    uint16 i;

    for (i = 0u; i < 512u; i++)
    {
        data[i] = (uint8)(i & 0xFFu);
    }

    PlaceSectorHeader(0u, 1u, 0u);
    /* Block 7 is 512 bytes */
    PlaceBlockInFlash(TEST_SECTOR_HDR, 7u, 512u, data, 1u, FEE_MARKER_VALID);

    DriveInitToCompletion();

    Fee_GarbageCollect_Init();
    Fee_GarbageCollect_Trigger();

    Fee_GcResultType result = DriveGcToCompletion(500u);
    TEST_ASSERT_EQUAL(FEE_GC_COMPLETE, result);

    /* Read back */
    Fee_Read(7u, 0u, readBuf, 512u);
    uint32 cycle;
    for (cycle = 0u; cycle < 200u; cycle++)
    {
        DriveOneCycle();
        if (Fee_GetStatus() == MEMIF_IDLE)
        {
            break;
        }
    }
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
    TEST_ASSERT_EQUAL_MEMORY(data, readBuf, 512);
}

/*============================================================================*
 *  27. MemAcc_Read rejection (E_NOT_OK from MemAcc_Read)
 *============================================================================*/

static void test_GC_Error_MemAccReadRejected(void)
{
    uint8 data[32];
    memset(data, 0xAA, 32);

    PlaceSectorHeader(0u, 1u, 0u);
    PlaceBlockInFlash(TEST_SECTOR_HDR, 1u, 32u, data, 1u, FEE_MARKER_VALID);

    DriveInitToCompletion();

    Fee_GarbageCollect_Init();
    Fee_GarbageCollect_Trigger();
    Dem_Stub_Reset();

    /* SELECT_SOURCE */
    DriveGcStep();

    /* Before COPY_READ, make MemAcc reject the read */
    Mem_DFLS_Stub_SetReturnValue(E_NOT_OK);

    /* COPY_READ -> MemAcc_Read returns E_NOT_OK */
    Fee_GcResultType result = Fee_GarbageCollect_Process();
    TEST_ASSERT_EQUAL(FEE_GC_ERROR, result);
    TEST_ASSERT_TRUE(Dem_Stub_WasEventReported(FEE_E_HARDWARE_ERROR,
                     DEM_EVENT_STATUS_FAILED));

    /* Reset for other tests */
    Mem_DFLS_Stub_SetReturnValue(E_OK);
}

/*============================================================================*
 *  28. MemAcc_Erase rejection (E_NOT_OK from MemAcc_Erase)
 *============================================================================*/

static void test_GC_Error_MemAccEraseRejected(void)
{
    uint8 data[32];
    memset(data, 0xAA, 32);

    /* Only invalid blocks -> skip copy -> ERASE_SOURCE */
    PlaceSectorHeader(0u, 1u, 0u);
    PlaceBlockInFlash(TEST_SECTOR_HDR, 1u, 32u, data, 1u, FEE_MARKER_INVALID);

    DriveInitToCompletion();

    Fee_GarbageCollect_Init();
    Fee_GarbageCollect_Trigger();
    Dem_Stub_Reset();

    /* SELECT_SOURCE -> no valid -> ERASE_SOURCE */
    DriveGcStep();

    /* Before ERASE_SOURCE, make MemAcc reject erase */
    Mem_DFLS_Stub_SetReturnValue(E_NOT_OK);

    /* ERASE_SOURCE -> MemAcc_Erase returns E_NOT_OK */
    Fee_GcResultType result = Fee_GarbageCollect_Process();
    TEST_ASSERT_EQUAL(FEE_GC_ERROR, result);
    TEST_ASSERT_TRUE(Dem_Stub_WasEventReported(FEE_E_HARDWARE_ERROR,
                     DEM_EVENT_STATUS_FAILED));

    Mem_DFLS_Stub_SetReturnValue(E_OK);
}

/*============================================================================*
 *  29. GC state transitions one-at-a-time (single block)
 *============================================================================*/

static void test_GC_StateTransitions_OnePerCall(void)
{
    uint8 data[32];
    Fee_GcResultType result;
    memset(data, 0x42, 32);

    PlaceSectorHeader(0u, 1u, 0u);
    PlaceBlockInFlash(TEST_SECTOR_HDR, 1u, 32u, data, 1u, FEE_MARKER_VALID);

    DriveInitToCompletion();

    Fee_GarbageCollect_Init();
    Fee_GarbageCollect_Trigger();

    /* Each call should return IN_PROGRESS until completion */
    /* 1. SELECT_SOURCE: find block -> transition to COPY_READ */
    result = Fee_GarbageCollect_Process();
    TEST_ASSERT_EQUAL(FEE_GC_IN_PROGRESS, result);

    /* 2. COPY_READ: initiate read */
    result = Fee_GarbageCollect_Process();
    TEST_ASSERT_EQUAL(FEE_GC_IN_PROGRESS, result);

    /* Drive MemAcc to complete the read */
    MemAcc_MainFunction();

    /* 3. COPY_READ_WAIT: poll OK, CRC OK -> COPY_WRITE_HEADER */
    result = Fee_GarbageCollect_Process();
    TEST_ASSERT_EQUAL(FEE_GC_IN_PROGRESS, result);

    /* 4. COPY_WRITE_HEADER: allocate + write header */
    result = Fee_GarbageCollect_Process();
    TEST_ASSERT_EQUAL(FEE_GC_IN_PROGRESS, result);

    MemAcc_MainFunction();

    /* 5. COPY_WRITE_HEADER_WAIT: poll OK -> COPY_WRITE_DATA */
    result = Fee_GarbageCollect_Process();
    TEST_ASSERT_EQUAL(FEE_GC_IN_PROGRESS, result);

    /* 6. COPY_WRITE_DATA: pad + write data */
    result = Fee_GarbageCollect_Process();
    TEST_ASSERT_EQUAL(FEE_GC_IN_PROGRESS, result);

    MemAcc_MainFunction();

    /* 7. COPY_WRITE_DATA_WAIT: poll OK -> COPY_WRITE_VALID */
    result = Fee_GarbageCollect_Process();
    TEST_ASSERT_EQUAL(FEE_GC_IN_PROGRESS, result);

    /* 8. COPY_WRITE_VALID: write valid marker */
    result = Fee_GarbageCollect_Process();
    TEST_ASSERT_EQUAL(FEE_GC_IN_PROGRESS, result);

    MemAcc_MainFunction();

    /* 9. COPY_WRITE_VALID_WAIT: poll OK, update BlockInfoTable -> SELECT_SOURCE */
    result = Fee_GarbageCollect_Process();
    TEST_ASSERT_EQUAL(FEE_GC_IN_PROGRESS, result);

    /* 10. SELECT_SOURCE: no more valid blocks -> ERASE_SOURCE */
    result = Fee_GarbageCollect_Process();
    TEST_ASSERT_EQUAL(FEE_GC_IN_PROGRESS, result);

    /* 11. ERASE_SOURCE: initiate erase */
    result = Fee_GarbageCollect_Process();
    TEST_ASSERT_EQUAL(FEE_GC_IN_PROGRESS, result);

    MemAcc_MainFunction();

    /* 12. ERASE_WAIT: poll OK, cleanup -> COMPLETE_STATE */
    result = Fee_GarbageCollect_Process();
    TEST_ASSERT_EQUAL(FEE_GC_IN_PROGRESS, result);

    /* 13. COMPLETE_STATE: return COMPLETE */
    result = Fee_GarbageCollect_Process();
    TEST_ASSERT_EQUAL(FEE_GC_COMPLETE, result);
}

/*============================================================================*
 *  30. GC Pending poll: COPY_READ_WAIT returns IN_PROGRESS while pending
 *============================================================================*/

static void test_GC_PendingPoll_ReturnsInProgress(void)
{
    uint8 data[32];
    memset(data, 0x42, 32);

    PlaceSectorHeader(0u, 1u, 0u);
    PlaceBlockInFlash(TEST_SECTOR_HDR, 1u, 32u, data, 1u, FEE_MARKER_VALID);

    DriveInitToCompletion();

    Fee_GarbageCollect_Init();
    Fee_GarbageCollect_Trigger();

    /* SELECT_SOURCE */
    Fee_GarbageCollect_Process();
    /* COPY_READ */
    Fee_GarbageCollect_Process();

    /* Don't call MemAcc_MainFunction -- job is still pending */
    /* COPY_READ_WAIT should return IN_PROGRESS */
    Fee_GcResultType result = Fee_GarbageCollect_Process();
    TEST_ASSERT_EQUAL(FEE_GC_IN_PROGRESS, result);

    /* Call again -- still pending */
    result = Fee_GarbageCollect_Process();
    TEST_ASSERT_EQUAL(FEE_GC_IN_PROGRESS, result);

    /* Now complete it */
    MemAcc_MainFunction();
    result = Fee_GarbageCollect_Process();
    TEST_ASSERT_EQUAL(FEE_GC_IN_PROGRESS, result); /* transitions to next state */
}

/*============================================================================*
 *  31. Header and data addresses updated correctly
 *============================================================================*/

static void test_GC_AddressesUpdated(void)
{
    uint8 data[32];
    memset(data, 0x42, 32);

    PlaceSectorHeader(0u, 1u, 0u);
    PlaceBlockInFlash(TEST_SECTOR_HDR, 1u, 32u, data, 1u, FEE_MARKER_VALID);

    DriveInitToCompletion();

    MemAcc_AddressType oldHeader = Fee_BlockInfoTable[0].HeaderAddress;
    MemAcc_AddressType oldData = Fee_BlockInfoTable[0].DataAddress;

    Fee_GarbageCollect_Init();
    Fee_GarbageCollect_Trigger();
    DriveGcToCompletion(200u);

    /* Addresses should have changed (moved to target sector) */
    TEST_ASSERT_NOT_EQUAL(oldHeader, Fee_BlockInfoTable[0].HeaderAddress);
    TEST_ASSERT_NOT_EQUAL(oldData, Fee_BlockInfoTable[0].DataAddress);

    /* Data address should be header + BLOCK_HEADER_SIZE */
    TEST_ASSERT_EQUAL(
        Fee_BlockInfoTable[0].HeaderAddress + TEST_BLOCK_HDR,
        Fee_BlockInfoTable[0].DataAddress);
}

/*============================================================================*
 *  32. GC with multiple erased sectors: select lowest erase count
 *============================================================================*/

static void test_GC_MultipleErasedSectors_SelectLowestErase(void)
{
    uint8 data[32];
    memset(data, 0xAA, 32);

    /* Sector 0: active with valid block (source) */
    PlaceSectorHeader(0u, 1u, 0u);
    PlaceBlockInFlash(TEST_SECTOR_HDR, 1u, 32u, data, 1u, FEE_MARKER_VALID);

    /* Sectors 1-3 are erased */
    DriveInitToCompletion();

    /* Set erase counts: sector 1=15, sector 2=3, sector 3=10 */
    Fee_SectorInfo[1].EraseCount = 15u;
    Fee_SectorInfo[2].EraseCount = 3u;
    Fee_SectorInfo[3].EraseCount = 10u;

    Fee_GarbageCollect_Init();
    Fee_GarbageCollect_Trigger();
    DriveGcToCompletion(200u);

    /* Block should be in sector 2 (lowest erase count = 3) */
    TEST_ASSERT_EQUAL(2u, Fee_BlockInfoTable[0].SectorIndex);
}

/*============================================================================*
 *  33. Suspend before any processing
 *============================================================================*/

static void test_GC_Suspend_BeforeAnyProcessing(void)
{
    uint8 data[32];
    memset(data, 0xAA, 32);

    PlaceSectorHeader(0u, 1u, 0u);
    PlaceBlockInFlash(TEST_SECTOR_HDR, 1u, 32u, data, 1u, FEE_MARKER_VALID);

    DriveInitToCompletion();

    Fee_GarbageCollect_Init();
    Fee_GarbageCollect_Trigger();

    /* Suspend immediately after trigger */
    Fee_GarbageCollect_Suspend();

    /* Should not advance */
    Fee_GcResultType result = Fee_GarbageCollect_Process();
    TEST_ASSERT_EQUAL(FEE_GC_IN_PROGRESS, result);
    TEST_ASSERT_TRUE(Fee_GarbageCollect_IsActive());

    /* Resume and complete */
    Fee_GarbageCollect_Resume();
    result = DriveGcToCompletion(200u);
    TEST_ASSERT_EQUAL(FEE_GC_COMPLETE, result);
}

/*============================================================================*
 *  34. GC Integration with Fee_StateMachine (GC_ACTIVE state)
 *============================================================================*/

static void test_GC_Integration_FeeStateMachine(void)
{
    uint8 data[32];
    memset(data, 0x42, 32);

    PlaceSectorHeader(0u, 1u, 0u);
    PlaceBlockInFlash(TEST_SECTOR_HDR, 1u, 32u, data, 1u, FEE_MARKER_VALID);

    DriveInitToCompletion();

    /* Set up GC */
    Fee_GarbageCollect_Init();
    Fee_GarbageCollect_Trigger();

    /* Force Fee state machine into GC_ACTIVE */
    Fee_ModuleStatus = MEMIF_BUSY_INTERNAL;
    Fee_InternalState = FEE_STATE_GC_ACTIVE;

    /* Drive main cycles until GC completes */
    uint32 cycle;
    for (cycle = 0u; cycle < 200u; cycle++)
    {
        DriveOneCycle();
        if (Fee_ModuleStatus == MEMIF_IDLE)
        {
            break;
        }
    }

    TEST_ASSERT_EQUAL(MEMIF_IDLE, Fee_ModuleStatus);
    TEST_ASSERT_EQUAL(FEE_STATE_IDLE, Fee_InternalState);
}

/*============================================================================*
 *  35. CRC mismatch with multiple blocks: only bad block skipped
 *============================================================================*/

static void test_GC_CrcMismatch_OnlyBadBlockSkipped(void)
{
    uint8 data1[32], data2[64];
    uint8 *flash;
    uint32 offset;
    memset(data1, 0x11, 32);
    memset(data2, 0x22, 64);

    PlaceSectorHeader(0u, 1u, 0u);

    offset = TEST_SECTOR_HDR;
    PlaceBlockInFlash(offset, 1u, 32u, data1, 1u, FEE_MARKER_VALID);
    offset += TEST_BLOCK_HDR + AlignUp(32u, TEST_PAGE_SIZE);
    PlaceBlockInFlash(offset, 2u, 64u, data2, 1u, FEE_MARKER_VALID);

    DriveInitToCompletion();

    /* Corrupt only block 1's data */
    flash = Mem_DFLS_Stub_GetFlashContent();
    flash[TEST_SECTOR_HDR + TEST_BLOCK_HDR] = 0xFFu;

    Fee_GarbageCollect_Init();
    Fee_GarbageCollect_Trigger();

    Fee_GcResultType result = DriveGcToCompletion(500u);
    TEST_ASSERT_EQUAL(FEE_GC_COMPLETE, result);

    /* Block 1 should be NOT_FOUND (CRC mismatch -> INCONSISTENT, then erased) */
    TEST_ASSERT_EQUAL(FEE_BLOCK_NOT_FOUND, Fee_BlockInfoTable[0].Status);

    /* Block 2 should be moved (data was not corrupted) */
    TEST_ASSERT_EQUAL(FEE_BLOCK_VALID, Fee_BlockInfoTable[1].Status);
    TEST_ASSERT_EQUAL(1u, Fee_BlockInfoTable[1].SectorIndex);
}

/*============================================================================*
 *  36. MemAcc_Write rejection during header write
 *============================================================================*/

static void test_GC_Error_MemAccWriteHeaderRejected(void)
{
    uint8 data[32];
    memset(data, 0xAA, 32);

    PlaceSectorHeader(0u, 1u, 0u);
    PlaceBlockInFlash(TEST_SECTOR_HDR, 1u, 32u, data, 1u, FEE_MARKER_VALID);

    DriveInitToCompletion();

    Fee_GarbageCollect_Init();
    Fee_GarbageCollect_Trigger();
    Dem_Stub_Reset();

    /* SELECT_SOURCE */
    DriveGcStep();
    /* COPY_READ */
    DriveGcStep();
    /* COPY_READ_WAIT -> OK */
    DriveGcStep();

    /* Before COPY_WRITE_HEADER, make MemAcc reject the write */
    Mem_DFLS_Stub_SetReturnValue(E_NOT_OK);

    /* COPY_WRITE_HEADER -> MemAcc_Write returns E_NOT_OK */
    Fee_GcResultType result = Fee_GarbageCollect_Process();
    TEST_ASSERT_EQUAL(FEE_GC_ERROR, result);
    TEST_ASSERT_TRUE(Dem_Stub_WasEventReported(FEE_E_HARDWARE_ERROR,
                     DEM_EVENT_STATUS_FAILED));

    Mem_DFLS_Stub_SetReturnValue(E_OK);
}

/*============================================================================*
 *  Main
 *============================================================================*/

int main(void)
{
    UNITY_BEGIN();

    /* Init and basic state */
    RUN_TEST(test_GC_Init_IsNotActive);
    RUN_TEST(test_GC_Process_WhenIdle_ReturnsComplete);

    /* Trigger: source/target selection */
    RUN_TEST(test_GC_Trigger_SelectsFullestSource);
    RUN_TEST(test_GC_Trigger_WearLeveling_LowestEraseCount);
    RUN_TEST(test_GC_Trigger_NoSource_Fails);
    RUN_TEST(test_GC_Trigger_NoTarget_Fails);

    /* Single-block copy (full flow) */
    RUN_TEST(test_GC_SingleBlock_FullCycle);
    RUN_TEST(test_GC_StateTransitions_OnePerCall);
    RUN_TEST(test_GC_PendingPoll_ReturnsInProgress);

    /* Multi-block copy */
    RUN_TEST(test_GC_MultiBlock_AllCopied);

    /* Mixed blocks */
    RUN_TEST(test_GC_MixedBlocks_OnlyValidCopied);

    /* Suspend/Resume */
    RUN_TEST(test_GC_SuspendResume);
    RUN_TEST(test_GC_Suspend_BeforeAnyProcessing);

    /* Error handling */
    RUN_TEST(test_GC_Error_CopyReadFailure);
    RUN_TEST(test_GC_Error_CopyWriteHeaderFailure);
    RUN_TEST(test_GC_Error_CopyWriteDataFailure);
    RUN_TEST(test_GC_Error_CopyWriteValidFailure);
    RUN_TEST(test_GC_Error_EraseFailure);
    RUN_TEST(test_GC_Error_MemAccReadRejected);
    RUN_TEST(test_GC_Error_MemAccEraseRejected);
    RUN_TEST(test_GC_Error_MemAccWriteHeaderRejected);

    /* Wear leveling */
    RUN_TEST(test_GC_MultipleErasedSectors_SelectLowestErase);

    /* Stale pointer cleanup */
    RUN_TEST(test_GC_StalePointerCleanup);

    /* CRC mismatch */
    RUN_TEST(test_GC_CrcMismatch_BlockSkipped);
    RUN_TEST(test_GC_CrcMismatch_OnlyBadBlockSkipped);

    /* No valid blocks */
    RUN_TEST(test_GC_NoValidBlocks_JustErase);

    /* IsActive */
    RUN_TEST(test_GC_IsActive_TrueDuringGC);

    /* Post-GC state */
    RUN_TEST(test_GC_EraseCountIncremented);
    RUN_TEST(test_GC_SourceSector_BecomeErased);
    RUN_TEST(test_GC_AddressesUpdated);
    RUN_TEST(test_GC_DataIntegrity_Readback);
    RUN_TEST(test_GC_SequenceCounter_Preserved);
    RUN_TEST(test_GC_DataCrc_Preserved);

    /* Block size variations */
    RUN_TEST(test_GC_ImmediateBlock);
    RUN_TEST(test_GC_LargeBlock);

    /* Integration */
    RUN_TEST(test_GC_Integration_FeeStateMachine);

    return UNITY_END();
}
