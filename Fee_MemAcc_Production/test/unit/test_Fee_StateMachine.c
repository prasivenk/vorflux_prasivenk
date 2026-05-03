/**
 * \file       test_Fee_StateMachine.c
 * \brief      Unit Tests -- Fee Async State Machine
 *
 * \details    Tests init state transitions (UNINIT -> per-sector reads ->
 *             per-record scan -> IDLE), read flow, write flow, invalidate
 *             flow, cancel during wait states, error handling, NvM
 *             notifications, etc.
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
 *  Helper: drive a write job to completion
 *============================================================================*/

static void DriveWriteToCompletion(void)
{
    uint32 maxCycles = 200u;
    uint32 cycle;

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
 *  1. Init: UNINIT -> INIT_READ_SECTOR_HEADER
 *============================================================================*/

static void test_Init_StateTransition_ToReadSectorHeader(void)
{
    Fee_Init(&Fee_Config);
    TEST_ASSERT_EQUAL(FEE_STATE_INIT_READ_SECTOR_HEADER, Fee_InternalState);
    TEST_ASSERT_EQUAL(MEMIF_BUSY_INTERNAL, Fee_ModuleStatus);
}

/*============================================================================*
 *  2. Init: First MainFunction -> INIT_WAIT_SECTOR_HEADER
 *============================================================================*/

static void test_Init_FirstProcess_ToWaitSectorHeader(void)
{
    Fee_Init(&Fee_Config);
    Fee_MainFunction();
    TEST_ASSERT_EQUAL(FEE_STATE_INIT_WAIT_SECTOR_HEADER, Fee_InternalState);
}

/*============================================================================*
 *  3. Init: After MemAcc completes first sector read -> INIT_NEXT_SECTOR
 *============================================================================*/

static void test_Init_AfterSectorRead_ToNextSector(void)
{
    Fee_Init(&Fee_Config);
    Fee_MainFunction(); /* -> WAIT */
    MemAcc_MainFunction(); /* Process MemAcc job */
    Fee_MainFunction(); /* -> NEXT_SECTOR */
    TEST_ASSERT_EQUAL(FEE_STATE_INIT_NEXT_SECTOR, Fee_InternalState);
}

/*============================================================================*
 *  4. Init: Full cycle through all sectors -> scan records
 *============================================================================*/

static void test_Init_AllSectors_ToScanRecords(void)
{
    uint32 cycle;
    Fee_Init(&Fee_Config);

    /* Drive through all sector header reads */
    for (cycle = 0u; cycle < 100u; cycle++)
    {
        DriveOneCycle();
        if (Fee_InternalState == FEE_STATE_INIT_SCAN_READ_RECORD ||
            Fee_InternalState == FEE_STATE_IDLE)
        {
            break;
        }
    }

    /* Should be scanning records or already at IDLE (blank flash) */
    TEST_ASSERT_TRUE(
        (Fee_InternalState == FEE_STATE_INIT_SCAN_READ_RECORD) ||
        (Fee_InternalState == FEE_STATE_IDLE));
}

/*============================================================================*
 *  5. Init: Blank flash -> IDLE immediately after scan starts
 *============================================================================*/

static void test_Init_BlankFlash_GoesToIdle(void)
{
    DriveInitToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_IDLE, Fee_GetStatus());
    TEST_ASSERT_EQUAL(FEE_STATE_IDLE, Fee_InternalState);
}

/*============================================================================*
 *  6. Init: Flash with one active sector and one valid block
 *============================================================================*/

static void test_Init_WithValidBlock(void)
{
    uint8 testData[32];

    memset(testData, 0xAB, 32);

    /* Place sector 0 header at offset 0 */
    PlaceSectorHeader(0u, 1u, 0u);
    /* Place block 1 (32 bytes) at offset 32 (after sector header) */
    PlaceBlockInFlash(32u, 1u, 32u, testData, 1u, FEE_MARKER_VALID);

    DriveInitToCompletion();

    /* Block 1 should be found as VALID */
    TEST_ASSERT_EQUAL(FEE_BLOCK_VALID, Fee_BlockInfoTable[0].Status);
    TEST_ASSERT_EQUAL_UINT16(1u, Fee_BlockInfoTable[0].SequenceCounter);
}

/*============================================================================*
 *  7. Init: Flash with invalid block
 *============================================================================*/

static void test_Init_WithInvalidBlock(void)
{
    uint8 testData[32];

    memset(testData, 0xCD, 32);

    PlaceSectorHeader(0u, 1u, 0u);
    PlaceBlockInFlash(32u, 1u, 32u, testData, 1u, FEE_MARKER_INVALID);

    DriveInitToCompletion();

    TEST_ASSERT_EQUAL(FEE_BLOCK_INVALID, Fee_BlockInfoTable[0].Status);
}

/*============================================================================*
 *  8. Init: Flash with inconsistent block (erased marker)
 *============================================================================*/

static void test_Init_WithInconsistentBlock(void)
{
    uint8 testData[32];

    memset(testData, 0xEF, 32);

    PlaceSectorHeader(0u, 1u, 0u);
    PlaceBlockInFlash(32u, 1u, 32u, testData, 1u, FEE_MARKER_ERASED);

    DriveInitToCompletion();

    TEST_ASSERT_EQUAL(FEE_BLOCK_INCONSISTENT, Fee_BlockInfoTable[0].Status);
}

/*============================================================================*
 *  9. Read: non-existent block -> MEMIF_BLOCK_INVALID
 *============================================================================*/

static void test_Read_NonExistentBlock(void)
{
    uint8 buf[32];

    DriveInitToCompletion();
    NvM_Cbk_Stub_Reset();

    Fee_Read(1u, 0u, buf, 32u);
    DriveWriteToCompletion();

    TEST_ASSERT_EQUAL(MEMIF_BLOCK_INVALID, Fee_GetJobResult());
    TEST_ASSERT_EQUAL(1u, NvM_Cbk_Stub_GetErrorCount());
}

/*============================================================================*
 *  10. Read: valid block -> success
 *============================================================================*/

static void test_Read_ValidBlock(void)
{
    uint8 testData[32];
    uint8 readBuf[32];

    memset(testData, 0x42, 32);
    memset(readBuf, 0x00, 32);

    PlaceSectorHeader(0u, 1u, 0u);
    PlaceBlockInFlash(32u, 1u, 32u, testData, 1u, FEE_MARKER_VALID);

    DriveInitToCompletion();
    NvM_Cbk_Stub_Reset();

    Fee_Read(1u, 0u, readBuf, 32u);
    DriveWriteToCompletion();

    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
    TEST_ASSERT_EQUAL_MEMORY(testData, readBuf, 32);
    TEST_ASSERT_EQUAL(1u, NvM_Cbk_Stub_GetEndCount());
}

/*============================================================================*
 *  11. Read: inconsistent block -> MEMIF_BLOCK_INCONSISTENT
 *============================================================================*/

static void test_Read_InconsistentBlock(void)
{
    uint8 testData[32];
    uint8 readBuf[32];

    memset(testData, 0xEF, 32);

    PlaceSectorHeader(0u, 1u, 0u);
    PlaceBlockInFlash(32u, 1u, 32u, testData, 1u, FEE_MARKER_ERASED);

    DriveInitToCompletion();

    Fee_Read(1u, 0u, readBuf, 32u);
    DriveWriteToCompletion();

    TEST_ASSERT_EQUAL(MEMIF_BLOCK_INCONSISTENT, Fee_GetJobResult());
}

/*============================================================================*
 *  12. Read: CRC mismatch -> MEMIF_BLOCK_INCONSISTENT + DEM
 *============================================================================*/

static void test_Read_CrcMismatch(void)
{
    uint8 testData[32];
    uint8 readBuf[32];
    uint8 *flash;

    memset(testData, 0x42, 32);

    PlaceSectorHeader(0u, 1u, 0u);
    PlaceBlockInFlash(32u, 1u, 32u, testData, 1u, FEE_MARKER_VALID);

    /* Corrupt the data in flash (change one byte) */
    flash = Mem_DFLS_Stub_GetFlashContent();
    flash[64] = 0xFFu;  /* byte 0 of data at offset 32+32=64 */

    DriveInitToCompletion();
    Dem_Stub_Reset();

    Fee_Read(1u, 0u, readBuf, 32u);
    DriveWriteToCompletion();

    TEST_ASSERT_EQUAL(MEMIF_BLOCK_INCONSISTENT, Fee_GetJobResult());
    TEST_ASSERT_TRUE(Dem_Stub_WasEventReported(FEE_E_HARDWARE_ERROR,
                     DEM_EVENT_STATUS_FAILED));
}

/*============================================================================*
 *  13. Read: partial read (CRC skipped)
 *============================================================================*/

static void test_Read_PartialRead_CrcSkipped(void)
{
    uint8 testData[32];
    uint8 readBuf[16];

    memset(testData, 0x42, 32);
    memset(readBuf, 0x00, 16);

    PlaceSectorHeader(0u, 1u, 0u);
    PlaceBlockInFlash(32u, 1u, 32u, testData, 1u, FEE_MARKER_VALID);

    DriveInitToCompletion();

    /* Partial read: offset=0, length=16 (< block size 32) */
    Fee_Read(1u, 0u, readBuf, 16u);
    DriveWriteToCompletion();

    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
    /* First 16 bytes should match */
    TEST_ASSERT_EQUAL_MEMORY(testData, readBuf, 16);
}

/*============================================================================*
 *  14. Write: full flow -> success
 *============================================================================*/

static void test_Write_FullFlow(void)
{
    uint8 writeData[32];

    memset(writeData, 0xBB, 32);

    /* Set up active sector with header */
    PlaceSectorHeader(0u, 1u, 0u);

    DriveInitToCompletion();
    NvM_Cbk_Stub_Reset();

    Fee_Write(1u, writeData);
    DriveWriteToCompletion();

    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
    TEST_ASSERT_EQUAL(FEE_BLOCK_VALID, Fee_BlockInfoTable[0].Status);
    TEST_ASSERT_EQUAL(1u, NvM_Cbk_Stub_GetEndCount());
}

/*============================================================================*
 *  15. Write then read back
 *============================================================================*/

static void test_Write_ThenRead(void)
{
    uint8 writeData[32];
    uint8 readBuf[32];

    memset(writeData, 0xCC, 32);
    memset(readBuf, 0x00, 32);

    PlaceSectorHeader(0u, 1u, 0u);

    DriveInitToCompletion();

    /* Write */
    Fee_Write(1u, writeData);
    DriveWriteToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());

    /* Read back */
    Fee_Read(1u, 0u, readBuf, 32u);
    DriveWriteToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
    TEST_ASSERT_EQUAL_MEMORY(writeData, readBuf, 32);
}

/*============================================================================*
 *  16. Write: MemAcc failure during header write -> MEMIF_JOB_FAILED + DEM
 *============================================================================*/

static void test_Write_MemAccFailure_HeaderWrite(void)
{
    uint8 writeData[32];

    memset(writeData, 0xDD, 32);

    PlaceSectorHeader(0u, 1u, 0u);
    DriveInitToCompletion();
    Dem_Stub_Reset();
    NvM_Cbk_Stub_Reset();

    Fee_Write(1u, writeData);

    /* Process until we're in WRITE_HEADER_WAIT */
    DriveOneCycle(); /* -> WRITE_ALLOC */
    DriveOneCycle(); /* -> WRITE_HEADER -> dispatch to MemAcc */

    /* Set MemAcc to fail */
    Mem_DFLS_Stub_SetJobResult(MEM_DFLS_JOB_FAILED);

    DriveOneCycle(); /* Process MemAcc (fail) + Fee sees fail */
    DriveOneCycle(); /* Fee should complete with FAILED */

    /* Drive until idle */
    DriveWriteToCompletion();

    TEST_ASSERT_EQUAL(MEMIF_JOB_FAILED, Fee_GetJobResult());
    TEST_ASSERT_TRUE(Dem_Stub_WasEventReported(FEE_E_HARDWARE_ERROR,
                     DEM_EVENT_STATUS_FAILED));
    TEST_ASSERT_EQUAL(1u, NvM_Cbk_Stub_GetErrorCount());
}

/*============================================================================*
 *  17. Invalidate: flow to completion
 *============================================================================*/

static void test_Invalidate_FullFlow(void)
{
    uint8 testData[32];

    memset(testData, 0xAA, 32);

    PlaceSectorHeader(0u, 1u, 0u);
    PlaceBlockInFlash(32u, 1u, 32u, testData, 1u, FEE_MARKER_VALID);

    DriveInitToCompletion();
    TEST_ASSERT_EQUAL(FEE_BLOCK_VALID, Fee_BlockInfoTable[0].Status);

    NvM_Cbk_Stub_Reset();
    Fee_InvalidateBlock(1u);
    DriveWriteToCompletion();

    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
    TEST_ASSERT_EQUAL(FEE_BLOCK_INVALID, Fee_BlockInfoTable[0].Status);
    TEST_ASSERT_EQUAL(1u, NvM_Cbk_Stub_GetEndCount());
}

/*============================================================================*
 *  18. Invalidate: non-existent block succeeds
 *============================================================================*/

static void test_Invalidate_NonExistentBlock(void)
{
    DriveInitToCompletion();

    Fee_InvalidateBlock(1u);
    DriveWriteToCompletion();

    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
}

/*============================================================================*
 *  19. Cancel during READ_WAIT
 *============================================================================*/

static void test_Cancel_DuringReadWait(void)
{
    uint8 testData[32];
    uint8 readBuf[32];

    memset(testData, 0x42, 32);

    PlaceSectorHeader(0u, 1u, 0u);
    PlaceBlockInFlash(32u, 1u, 32u, testData, 1u, FEE_MARKER_VALID);

    DriveInitToCompletion();
    NvM_Cbk_Stub_Reset();

    Fee_Read(1u, 0u, readBuf, 32u);

    /* Drive until in READ_WAIT */
    Fee_MainFunction(); /* -> READ_START -> initiate MemAcc read */
    Fee_MainFunction(); /* -> READ_WAIT (MemAcc pending) */

    Fee_Cancel();

    TEST_ASSERT_EQUAL(MEMIF_JOB_CANCELED, Fee_GetJobResult());
    TEST_ASSERT_EQUAL(MEMIF_IDLE, Fee_GetStatus());
}

/*============================================================================*
 *  20. Cancel during WRITE_HEADER_WAIT
 *============================================================================*/

static void test_Cancel_DuringWriteHeaderWait(void)
{
    uint8 writeData[32];

    memset(writeData, 0xDD, 32);

    PlaceSectorHeader(0u, 1u, 0u);
    DriveInitToCompletion();
    NvM_Cbk_Stub_Reset();

    Fee_Write(1u, writeData);

    /* Drive state machine: IDLE -> WRITE_ALLOC -> WRITE_HEADER */
    Fee_MainFunction(); /* -> WRITE_ALLOC */
    Fee_MainFunction(); /* -> WRITE_HEADER -> dispatch MemAcc write */
    Fee_MainFunction(); /* -> WRITE_HEADER_WAIT */

    Fee_Cancel();

    TEST_ASSERT_EQUAL(MEMIF_JOB_CANCELED, Fee_GetJobResult());
    TEST_ASSERT_EQUAL(MEMIF_IDLE, Fee_GetStatus());
}

/*============================================================================*
 *  21. Cancel during WRITE_DATA_WAIT
 *============================================================================*/

static void test_Cancel_DuringWriteDataWait(void)
{
    uint8 writeData[32];

    memset(writeData, 0xDD, 32);

    PlaceSectorHeader(0u, 1u, 0u);
    DriveInitToCompletion();

    Fee_Write(1u, writeData);

    /* Drive through ALLOC, HEADER, HEADER_WAIT */
    DriveOneCycle(); /* ALLOC */
    DriveOneCycle(); /* HEADER -> write to MemAcc */
    DriveOneCycle(); /* HEADER_WAIT -> MemAcc completes -> DATA */
    DriveOneCycle(); /* DATA -> write to MemAcc */

    /* Now in DATA_WAIT */
    Fee_Cancel();

    TEST_ASSERT_EQUAL(MEMIF_JOB_CANCELED, Fee_GetJobResult());
}

/*============================================================================*
 *  22. Cancel during INVALIDATE_WAIT
 *============================================================================*/

static void test_Cancel_DuringInvalidateWait(void)
{
    uint8 testData[32];

    memset(testData, 0xAA, 32);

    PlaceSectorHeader(0u, 1u, 0u);
    PlaceBlockInFlash(32u, 1u, 32u, testData, 1u, FEE_MARKER_VALID);

    DriveInitToCompletion();

    Fee_InvalidateBlock(1u);

    /* Drive until in INVALIDATE_WAIT */
    Fee_MainFunction(); /* IDLE -> INVALIDATE_WRITE */
    Fee_MainFunction(); /* INVALIDATE_WRITE -> dispatches to MemAcc */

    Fee_Cancel();

    TEST_ASSERT_EQUAL(MEMIF_JOB_CANCELED, Fee_GetJobResult());
}

/*============================================================================*
 *  23. NvM end notification on successful write
 *============================================================================*/

static void test_NvM_EndNotification_OnSuccess(void)
{
    uint8 writeData[32];

    memset(writeData, 0xBB, 32);

    PlaceSectorHeader(0u, 1u, 0u);
    DriveInitToCompletion();
    NvM_Cbk_Stub_Reset();

    Fee_Write(1u, writeData);
    DriveWriteToCompletion();

    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
    TEST_ASSERT_EQUAL(1u, NvM_Cbk_Stub_GetEndCount());
    TEST_ASSERT_EQUAL(0u, NvM_Cbk_Stub_GetErrorCount());
}

/*============================================================================*
 *  24. NvM error notification on read of non-existent block
 *============================================================================*/

static void test_NvM_ErrorNotification_OnFailure(void)
{
    uint8 readBuf[32];

    DriveInitToCompletion();
    NvM_Cbk_Stub_Reset();

    Fee_Read(1u, 0u, readBuf, 32u);
    DriveWriteToCompletion();

    /* Block not found -> BLOCK_INVALID -> error notification */
    TEST_ASSERT_EQUAL(MEMIF_BLOCK_INVALID, Fee_GetJobResult());
    TEST_ASSERT_EQUAL(0u, NvM_Cbk_Stub_GetEndCount());
    TEST_ASSERT_EQUAL(1u, NvM_Cbk_Stub_GetErrorCount());
}

/*============================================================================*
 *  25. Write: multiple blocks
 *============================================================================*/

static void test_Write_MultipleBlocks(void)
{
    uint8 data1[32], data2[64];
    uint8 readBuf1[32], readBuf2[64];

    memset(data1, 0x11, 32);
    memset(data2, 0x22, 64);

    PlaceSectorHeader(0u, 1u, 0u);
    DriveInitToCompletion();

    /* Write block 1 */
    Fee_Write(1u, data1);
    DriveWriteToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());

    /* Write block 2 */
    Fee_Write(2u, data2);
    DriveWriteToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());

    /* Read back block 1 */
    Fee_Read(1u, 0u, readBuf1, 32u);
    DriveWriteToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
    TEST_ASSERT_EQUAL_MEMORY(data1, readBuf1, 32);

    /* Read back block 2 */
    Fee_Read(2u, 0u, readBuf2, 64u);
    DriveWriteToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
    TEST_ASSERT_EQUAL_MEMORY(data2, readBuf2, 64);
}

/*============================================================================*
 *  26. Write: overwrites previous block
 *============================================================================*/

static void test_Write_Overwrite(void)
{
    uint8 data1[32], data2[32];
    uint8 readBuf[32];

    memset(data1, 0x11, 32);
    memset(data2, 0x22, 32);

    PlaceSectorHeader(0u, 1u, 0u);
    DriveInitToCompletion();

    /* First write */
    Fee_Write(1u, data1);
    DriveWriteToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());

    /* Overwrite with new data */
    Fee_Write(1u, data2);
    DriveWriteToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());

    /* Read should return new data */
    Fee_Read(1u, 0u, readBuf, 32u);
    DriveWriteToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
    TEST_ASSERT_EQUAL_MEMORY(data2, readBuf, 32);
}

/*============================================================================*
 *  27. Init: multiple valid blocks in flash
 *============================================================================*/

static void test_Init_MultipleValidBlocks(void)
{
    uint8 data1[32], data2[64];

    memset(data1, 0xAA, 32);
    memset(data2, 0xBB, 64);

    PlaceSectorHeader(0u, 1u, 0u);
    /* Block 1 at offset 32 (after sector header) */
    PlaceBlockInFlash(32u, 1u, 32u, data1, 1u, FEE_MARKER_VALID);
    /* Block 2 at offset 32 + 32(hdr) + 32(data) = 96 */
    PlaceBlockInFlash(96u, 2u, 64u, data2, 1u, FEE_MARKER_VALID);

    DriveInitToCompletion();

    TEST_ASSERT_EQUAL(FEE_BLOCK_VALID, Fee_BlockInfoTable[0].Status);
    TEST_ASSERT_EQUAL(FEE_BLOCK_VALID, Fee_BlockInfoTable[1].Status);
}

/*============================================================================*
 *  28. Init: newer block instance supersedes older
 *============================================================================*/

static void test_Init_NewerBlockSupersedes(void)
{
    uint8 data1[32], data2[32];

    memset(data1, 0xAA, 32);
    memset(data2, 0xBB, 32);

    PlaceSectorHeader(0u, 1u, 0u);
    /* First instance of block 1 at offset 32 */
    PlaceBlockInFlash(32u, 1u, 32u, data1, 1u, FEE_MARKER_VALID);
    /* Second instance of block 1 at offset 96 with higher seq counter */
    PlaceBlockInFlash(96u, 1u, 32u, data2, 2u, FEE_MARKER_VALID);

    DriveInitToCompletion();

    /* Should have the newer instance (seq=2) */
    TEST_ASSERT_EQUAL(FEE_BLOCK_VALID, Fee_BlockInfoTable[0].Status);
    TEST_ASSERT_EQUAL_UINT16(2u, Fee_BlockInfoTable[0].SequenceCounter);
}

/*============================================================================*
 *  29. Erase immediate: block not found -> success
 *============================================================================*/

static void test_EraseImmediate_NotFound(void)
{
    DriveInitToCompletion();

    Fee_EraseImmediateBlock(5u);
    DriveWriteToCompletion();

    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
}

/*============================================================================*
 *  30. Erase immediate: existing valid block -> invalidated
 *============================================================================*/

static void test_EraseImmediate_ExistingBlock(void)
{
    uint8 data[16];

    memset(data, 0xCC, 16);

    PlaceSectorHeader(0u, 1u, 0u);
    PlaceBlockInFlash(32u, 5u, 16u, data, 1u, FEE_MARKER_VALID);

    DriveInitToCompletion();
    TEST_ASSERT_EQUAL(FEE_BLOCK_VALID, Fee_BlockInfoTable[4].Status);

    Fee_EraseImmediateBlock(5u);
    DriveWriteToCompletion();

    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
    TEST_ASSERT_EQUAL(FEE_BLOCK_NOT_FOUND, Fee_BlockInfoTable[4].Status);
}

/*============================================================================*
 *  31. GC_ACTIVE state (stub) -> returns to IDLE
 *============================================================================*/

static void test_GC_Active_ReturnsToIdle(void)
{
    DriveInitToCompletion();

    /* Force GC state */
    Fee_ModuleStatus = MEMIF_BUSY_INTERNAL;
    Fee_InternalState = FEE_STATE_GC_ACTIVE;

    Fee_MainFunction();

    /* GC stub returns COMPLETE -> should go to IDLE */
    TEST_ASSERT_EQUAL(MEMIF_IDLE, Fee_ModuleStatus);
    TEST_ASSERT_EQUAL(FEE_STATE_IDLE, Fee_InternalState);
}

/*============================================================================*
 *  32. ERROR state -> DEM report -> IDLE
 *============================================================================*/

static void test_Error_State_DemAndIdle(void)
{
    DriveInitToCompletion();
    Dem_Stub_Reset();

    Fee_InternalState = FEE_STATE_ERROR;
    Fee_ModuleStatus = MEMIF_BUSY;

    Fee_MainFunction();

    TEST_ASSERT_EQUAL(MEMIF_IDLE, Fee_ModuleStatus);
    TEST_ASSERT_EQUAL(FEE_STATE_IDLE, Fee_InternalState);
    TEST_ASSERT_TRUE(Dem_Stub_WasEventReported(FEE_E_HARDWARE_ERROR,
                     DEM_EVENT_STATUS_FAILED));
}

/*============================================================================*
 *  33. Write: 8-byte immediate data block
 *============================================================================*/

static void test_Write_ImmediateBlock(void)
{
    uint8 data[8];
    uint8 readBuf[8];

    memset(data, 0xEE, 8);

    PlaceSectorHeader(0u, 1u, 0u);
    DriveInitToCompletion();

    /* Block 6 is 8 bytes immediate */
    Fee_Write(6u, data);
    DriveWriteToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());

    Fee_Read(6u, 0u, readBuf, 8u);
    DriveWriteToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
    TEST_ASSERT_EQUAL_MEMORY(data, readBuf, 8);
}

/*============================================================================*
 *  34. Write: 512-byte block
 *============================================================================*/

static void test_Write_LargeBlock(void)
{
    uint8 data[512];
    uint8 readBuf[512];

    memset(data, 0x77, 512);

    PlaceSectorHeader(0u, 1u, 0u);
    DriveInitToCompletion();

    /* Block 7 is 512 bytes */
    Fee_Write(7u, data);
    DriveWriteToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());

    Fee_Read(7u, 0u, readBuf, 512u);
    DriveWriteToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
    TEST_ASSERT_EQUAL_MEMORY(data, readBuf, 512);
}

/*============================================================================*
 *  35. Read after invalidate -> MEMIF_BLOCK_INVALID
 *============================================================================*/

static void test_Read_AfterInvalidate(void)
{
    uint8 data[32];
    uint8 readBuf[32];

    memset(data, 0xAA, 32);

    PlaceSectorHeader(0u, 1u, 0u);
    PlaceBlockInFlash(32u, 1u, 32u, data, 1u, FEE_MARKER_VALID);

    DriveInitToCompletion();

    /* Invalidate */
    Fee_InvalidateBlock(1u);
    DriveWriteToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());

    /* Read should return BLOCK_INVALID */
    Fee_Read(1u, 0u, readBuf, 32u);
    DriveWriteToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_BLOCK_INVALID, Fee_GetJobResult());
}

/*============================================================================*
 *  36. Write with offset read (partial)
 *============================================================================*/

static void test_Write_ThenPartialRead_WithOffset(void)
{
    uint8 data[32];
    uint8 readBuf[16];
    uint8 i;

    for (i = 0u; i < 32u; i++)
    {
        data[i] = i;
    }

    PlaceSectorHeader(0u, 1u, 0u);
    DriveInitToCompletion();

    Fee_Write(1u, data);
    DriveWriteToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());

    /* Read 16 bytes starting at offset 8 */
    Fee_Read(1u, 8u, readBuf, 16u);
    DriveWriteToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());
    TEST_ASSERT_EQUAL_MEMORY(&data[8], readBuf, 16);
}

/*============================================================================*
 *  37. Cancel during WRITE_VALID_WAIT
 *============================================================================*/

static void test_Cancel_DuringWriteValidWait(void)
{
    uint8 writeData[32];

    memset(writeData, 0xDD, 32);

    PlaceSectorHeader(0u, 1u, 0u);
    DriveInitToCompletion();

    Fee_Write(1u, writeData);

    /* Drive through ALLOC, HEADER, HEADER_WAIT, DATA, DATA_WAIT, VALID */
    DriveOneCycle(); /* ALLOC */
    DriveOneCycle(); /* HEADER -> write */
    DriveOneCycle(); /* HEADER_WAIT -> complete -> DATA */
    DriveOneCycle(); /* DATA -> write */
    DriveOneCycle(); /* DATA_WAIT -> complete -> VALID */
    DriveOneCycle(); /* VALID -> write valid marker */

    /* Should be in WRITE_VALID_WAIT now */
    Fee_Cancel();
    TEST_ASSERT_EQUAL(MEMIF_JOB_CANCELED, Fee_GetJobResult());
}

/*============================================================================*
 *  38. Init: active sector is the one with highest sequence number
 *============================================================================*/

static void test_Init_HighestSeqNumSector(void)
{
    /* Place sector headers in sectors 0 and 1 */
    /* Sector 0 at offset 0 with seqNum=1 */
    PlaceSectorHeader(0u, 1u, 0u);
    /* Sector 1 at offset 16384 with seqNum=5 (higher) */
    PlaceSectorHeader(16384u, 5u, 1u);

    DriveInitToCompletion();

    /* Active sector should be 1 (higher seqNum) */
    TEST_ASSERT_EQUAL(1u, Fee_ActiveSectorIndex);
}

/*============================================================================*
 *  39. Write: block info table updated with correct CRC
 *============================================================================*/

static void test_Write_BlockInfoHasCorrectCrc(void)
{
    uint8 data[32];
    uint16 expectedCrc;

    memset(data, 0x55, 32);
    expectedCrc = Fee_Crc_CalculateBlock(data, 32u);

    PlaceSectorHeader(0u, 1u, 0u);
    DriveInitToCompletion();

    Fee_Write(1u, data);
    DriveWriteToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());

    TEST_ASSERT_EQUAL_UINT16(expectedCrc, Fee_BlockInfoTable[0].DataCrc);
}

/*============================================================================*
 *  40. Write: block info table updated with correct sector index
 *============================================================================*/

static void test_Write_BlockInfoCorrectSectorIndex(void)
{
    uint8 data[32];

    memset(data, 0x55, 32);

    PlaceSectorHeader(0u, 1u, 0u);
    DriveInitToCompletion();

    Fee_Write(1u, data);
    DriveWriteToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());

    TEST_ASSERT_EQUAL(Fee_ActiveSectorIndex, Fee_BlockInfoTable[0].SectorIndex);
}

/*============================================================================*
 *  41. Init: unknown block in flash is ignored
 *============================================================================*/

static void test_Init_UnknownBlockIgnored(void)
{
    uint8 data[32];

    memset(data, 0xDD, 32);

    PlaceSectorHeader(0u, 1u, 0u);
    /* Place block with number 99 (not in config) */
    PlaceBlockInFlash(32u, 99u, 32u, data, 1u, FEE_MARKER_VALID);

    DriveInitToCompletion();

    /* All configured blocks should still be NOT_FOUND */
    TEST_ASSERT_EQUAL(FEE_BLOCK_NOT_FOUND, Fee_BlockInfoTable[0].Status);
}

/*============================================================================*
 *  42. Sequence counter increments on write
 *============================================================================*/

static void test_Write_SequenceCounterIncrements(void)
{
    uint8 data[32];

    memset(data, 0xAA, 32);

    PlaceSectorHeader(0u, 1u, 0u);
    DriveInitToCompletion();

    /* First write */
    Fee_Write(1u, data);
    DriveWriteToCompletion();
    TEST_ASSERT_EQUAL_UINT16(1u, Fee_BlockInfoTable[0].SequenceCounter);

    /* Second write */
    memset(data, 0xBB, 32);
    Fee_Write(1u, data);
    DriveWriteToCompletion();
    TEST_ASSERT_EQUAL_UINT16(2u, Fee_BlockInfoTable[0].SequenceCounter);
}

/*============================================================================*
 *  43. GC suspended for immediate job, then resumes
 *============================================================================*/

static void test_GC_Suspended_ForImmediate(void)
{
    uint8 data[16];

    memset(data, 0xCC, 16);
    PlaceSectorHeader(0u, 1u, 0u);
    DriveInitToCompletion();

    /* Force GC state */
    Fee_ModuleStatus = MEMIF_BUSY_INTERNAL;
    Fee_InternalState = FEE_STATE_GC_ACTIVE;

    /* Submit immediate write (block 5) */
    Fee_Write(5u, data);
    TEST_ASSERT_EQUAL(MEMIF_BUSY, Fee_ModuleStatus);

    /* Drive job to completion */
    DriveWriteToCompletion();
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fee_GetJobResult());

    /* After immediate job completes, should resume GC (BUSY_INTERNAL) */
    /* But GC stub immediately completes, so we end up IDLE */
    /* Drive one more cycle for GC to complete */
    DriveOneCycle();
    TEST_ASSERT_EQUAL(MEMIF_IDLE, Fee_ModuleStatus);
}

/*============================================================================*
 *  Main
 *============================================================================*/

int main(void)
{
    UNITY_BEGIN();

    /* Init state transitions */
    RUN_TEST(test_Init_StateTransition_ToReadSectorHeader);
    RUN_TEST(test_Init_FirstProcess_ToWaitSectorHeader);
    RUN_TEST(test_Init_AfterSectorRead_ToNextSector);
    RUN_TEST(test_Init_AllSectors_ToScanRecords);
    RUN_TEST(test_Init_BlankFlash_GoesToIdle);
    RUN_TEST(test_Init_WithValidBlock);
    RUN_TEST(test_Init_WithInvalidBlock);
    RUN_TEST(test_Init_WithInconsistentBlock);
    RUN_TEST(test_Init_MultipleValidBlocks);
    RUN_TEST(test_Init_NewerBlockSupersedes);
    RUN_TEST(test_Init_HighestSeqNumSector);
    RUN_TEST(test_Init_UnknownBlockIgnored);

    /* Read flow */
    RUN_TEST(test_Read_NonExistentBlock);
    RUN_TEST(test_Read_ValidBlock);
    RUN_TEST(test_Read_InconsistentBlock);
    RUN_TEST(test_Read_CrcMismatch);
    RUN_TEST(test_Read_PartialRead_CrcSkipped);
    RUN_TEST(test_Read_AfterInvalidate);

    /* Write flow */
    RUN_TEST(test_Write_FullFlow);
    RUN_TEST(test_Write_ThenRead);
    RUN_TEST(test_Write_MemAccFailure_HeaderWrite);
    RUN_TEST(test_Write_MultipleBlocks);
    RUN_TEST(test_Write_Overwrite);
    RUN_TEST(test_Write_ImmediateBlock);
    RUN_TEST(test_Write_LargeBlock);
    RUN_TEST(test_Write_ThenPartialRead_WithOffset);
    RUN_TEST(test_Write_BlockInfoHasCorrectCrc);
    RUN_TEST(test_Write_BlockInfoCorrectSectorIndex);
    RUN_TEST(test_Write_SequenceCounterIncrements);

    /* Invalidate flow */
    RUN_TEST(test_Invalidate_FullFlow);
    RUN_TEST(test_Invalidate_NonExistentBlock);

    /* Erase immediate */
    RUN_TEST(test_EraseImmediate_NotFound);
    RUN_TEST(test_EraseImmediate_ExistingBlock);

    /* Cancel tests */
    RUN_TEST(test_Cancel_DuringReadWait);
    RUN_TEST(test_Cancel_DuringWriteHeaderWait);
    RUN_TEST(test_Cancel_DuringWriteDataWait);
    RUN_TEST(test_Cancel_DuringInvalidateWait);
    RUN_TEST(test_Cancel_DuringWriteValidWait);

    /* NvM notifications */
    RUN_TEST(test_NvM_EndNotification_OnSuccess);
    RUN_TEST(test_NvM_ErrorNotification_OnFailure);

    /* GC */
    RUN_TEST(test_GC_Active_ReturnsToIdle);
    RUN_TEST(test_GC_Suspended_ForImmediate);

    /* Error */
    RUN_TEST(test_Error_State_DemAndIdle);

    return UNITY_END();
}
