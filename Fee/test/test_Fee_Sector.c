#include "unity.h"
#include "Fee_Sector.h"
#include "Fee_Cfg.h"
#include "MemAcc.h"
#include <string.h>

/* NvM callback stubs */
void NvM_JobEndNotification(void) { }
void NvM_JobErrorNotification(void) { }

/* Helper: standard init sequence */
static void InitFresh(void)
{
    MemAcc_TestResetFlash();
    MemAcc_Init();
    Fee_Sector_Init(&Fee_Config);
}

void setUp(void)
{
    /* Default: fresh init for each test. Tests that need different setup override. */
    InitFresh();
}

void tearDown(void)
{
}

/* ================================================================
 * Test: Sector init with blank flash
 * ================================================================ */
void test_SectorInit_BlankFlash_OneSectorActive(void)
{
    /* After init on blank flash, one sector should become active */
    uint8 activeIdx = Fee_Sector_GetActiveSectorIndex();
    TEST_ASSERT_TRUE(activeIdx < FEE_NUMBER_OF_SECTORS);

    /* Sector should have space */
    TEST_ASSERT_TRUE(Fee_Sector_HasSpace(FEE_BLOCK_HEADER_SIZE + 8u));
}

/* ================================================================
 * Test: CRC-16 CCITT test vector
 * ================================================================ */
void test_Crc16_TestVector(void)
{
    const uint8 testData[] = "123456789";
    uint16 crc = Fee_Crc16(testData, 9u, 0xFFFFu);
    TEST_ASSERT_EQUAL_HEX16(0x29B1u, crc);
}

void test_Crc16_EmptyData(void)
{
    uint16 crc = Fee_Crc16(NULL_PTR, 0u, 0xFFFFu);
    TEST_ASSERT_EQUAL_HEX16(0xFFFFu, crc);
}

/* ================================================================
 * Test: Write + Read round-trip for various block sizes
 * ================================================================ */
static void WriteReadRoundTrip(uint16 blockNumber, uint16 blockSize, uint8 fillByte)
{
    Fee_BlockInfoType blockInfo;
    uint8 writeData[512];
    uint8 readData[512];
    uint16 i;
    Std_ReturnType ret;

    blockInfo.Status = FEE_BLOCK_NOT_FOUND;
    blockInfo.DataAddress = 0u;

    /* Fill write buffer */
    for (i = 0u; i < blockSize; i++)
    {
        writeData[i] = (uint8)(fillByte + (uint8)(i & 0xFFu));
    }

    ret = Fee_Sector_WriteBlock(blockNumber, writeData, blockSize, &blockInfo);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(FEE_BLOCK_VALID, blockInfo.Status);
    TEST_ASSERT_TRUE(blockInfo.DataAddress != 0u);

    /* Read back */
    memset(readData, 0x00, sizeof(readData));
    ret = Fee_Sector_ReadBlock(&blockInfo, 0u, readData, blockSize);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_MEMORY(writeData, readData, blockSize);
}

void test_WriteRead_Size8(void)
{
    WriteReadRoundTrip(6u, 8u, 0xA0u);
}

void test_WriteRead_Size32(void)
{
    WriteReadRoundTrip(1u, 32u, 0x10u);
}

void test_WriteRead_Size64(void)
{
    WriteReadRoundTrip(2u, 64u, 0x20u);
}

void test_WriteRead_Size128(void)
{
    WriteReadRoundTrip(3u, 128u, 0x30u);
}

void test_WriteRead_Size256(void)
{
    WriteReadRoundTrip(4u, 256u, 0x40u);
}

void test_WriteRead_Size512(void)
{
    WriteReadRoundTrip(7u, 512u, 0x50u);
}

/* ================================================================
 * Test: ScanBlocks after writes
 * ================================================================ */
void test_ScanBlocks_AfterWrites(void)
{
    Fee_BlockInfoType blockTable[FEE_NUMBER_OF_BLOCKS];
    Fee_BlockInfoType blockInfo;
    uint8 data32[32];
    uint8 data64[64];
    uint16 i;
    Std_ReturnType ret;

    /* Write block 1 (32 bytes) */
    blockInfo.Status = FEE_BLOCK_NOT_FOUND;
    blockInfo.DataAddress = 0u;
    for (i = 0u; i < 32u; i++) { data32[i] = (uint8)(i + 1u); }
    ret = Fee_Sector_WriteBlock(1u, data32, 32u, &blockInfo);
    TEST_ASSERT_EQUAL(E_OK, ret);

    /* Write block 2 (64 bytes) */
    blockInfo.Status = FEE_BLOCK_NOT_FOUND;
    blockInfo.DataAddress = 0u;
    for (i = 0u; i < 64u; i++) { data64[i] = (uint8)(i + 0x80u); }
    ret = Fee_Sector_WriteBlock(2u, data64, 64u, &blockInfo);
    TEST_ASSERT_EQUAL(E_OK, ret);

    /* Scan blocks */
    ret = Fee_Sector_ScanBlocks(blockTable, FEE_NUMBER_OF_BLOCKS, Fee_BlockConfigData);
    TEST_ASSERT_EQUAL(E_OK, ret);

    /* Block 1 (config index 0) should be valid */
    TEST_ASSERT_EQUAL(FEE_BLOCK_VALID, blockTable[0].Status);
    TEST_ASSERT_TRUE(blockTable[0].DataAddress != 0u);

    /* Block 2 (config index 1) should be valid */
    TEST_ASSERT_EQUAL(FEE_BLOCK_VALID, blockTable[1].Status);
    TEST_ASSERT_TRUE(blockTable[1].DataAddress != 0u);

    /* Block 3 (config index 2) should not be found */
    TEST_ASSERT_EQUAL(FEE_BLOCK_NOT_FOUND, blockTable[2].Status);

    /* Verify data via ReadBlock */
    {
        uint8 readBuf[64];
        memset(readBuf, 0x00, sizeof(readBuf));
        Fee_Sector_ReadBlock(&blockTable[0], 0u, readBuf, 32u);
        TEST_ASSERT_EQUAL_MEMORY(data32, readBuf, 32u);

        memset(readBuf, 0x00, sizeof(readBuf));
        Fee_Sector_ReadBlock(&blockTable[1], 0u, readBuf, 64u);
        TEST_ASSERT_EQUAL_MEMORY(data64, readBuf, 64u);
    }
}

/* ================================================================
 * Test: Two-phase commit verification
 *   After Phase 1 (header+data written, marker still 0xFF),
 *   scanner should NOT find the block.
 * ================================================================ */
void test_TwoPhaseCommit_IncompleteWriteIgnored(void)
{
    Fee_BlockInfoType blockTable[FEE_NUMBER_OF_BLOCKS];
    uint8 activeIdx = Fee_Sector_GetActiveSectorIndex();
    uint8 headerBytes[11]; /* bytes 0-10 */
    uint8 data[32];
    uint16 blockNumber = 1u;
    uint16 blockLength = 32u;
    uint16 crc;
    uint8 crcBuf[4];
    uint32 writeAddr;
    uint16 i;

    /* Manually perform only Phase 1 of a write */
    for (i = 0u; i < 32u; i++) { data[i] = (uint8)(i + 1u); }

    /* Compute CRC */
    crcBuf[0] = (uint8)(blockNumber & 0xFFu);
    crcBuf[1] = (uint8)((blockNumber >> 8u) & 0xFFu);
    crcBuf[2] = (uint8)(blockLength & 0xFFu);
    crcBuf[3] = (uint8)((blockLength >> 8u) & 0xFFu);
    crc = Fee_Crc16(crcBuf, 4u, 0xFFFFu);
    crc = Fee_Crc16(data, 32u, crc);

    /* Build header bytes 0-10 */
    memset(headerBytes, 0x00, 11u);
    headerBytes[0] = (uint8)(blockNumber & 0xFFu);
    headerBytes[1] = (uint8)((blockNumber >> 8u) & 0xFFu);
    headerBytes[2] = (uint8)(blockLength & 0xFFu);
    headerBytes[3] = (uint8)((blockLength >> 8u) & 0xFFu);
    headerBytes[4] = (uint8)(crc & 0xFFu);
    headerBytes[5] = (uint8)((crc >> 8u) & 0xFFu);

    /* Get the current write pointer (we need to know where to write) */
    /* For a fresh sector, write pointer is at StartAddress + SECTOR_HEADER_SIZE */
    (void)activeIdx;

    /* We use the raw buffer approach: read via MemAcc to get where the sector data starts */
    /* First, find where the write pointer is from the active sector */
    /* Since we just did InitFresh(), the write pointer should be at sector_start + 12 */
    writeAddr = Fee_Config.SectorStartAddress + ((uint32)Fee_Sector_GetActiveSectorIndex() * FEE_SECTOR_SIZE) + FEE_SECTOR_HEADER_SIZE;

    /* Write header bytes 0-10 (leaving byte 11 as 0xFF) */
    TEST_ASSERT_EQUAL(E_OK, MemAcc_Write(writeAddr, headerBytes, 11u));

    /* Write data */
    TEST_ASSERT_EQUAL(E_OK, MemAcc_Write(writeAddr + FEE_BLOCK_HEADER_SIZE, data, 32u));

    /* Do NOT write ValidMarker (byte 11 stays 0xFF) */

    /* Scan blocks -- block should not be found */
    TEST_ASSERT_EQUAL(E_OK, Fee_Sector_ScanBlocks(blockTable, FEE_NUMBER_OF_BLOCKS, Fee_BlockConfigData));
    TEST_ASSERT_EQUAL(FEE_BLOCK_NOT_FOUND, blockTable[0].Status);
}

/* ================================================================
 * Test: InvalidateBlock
 * ================================================================ */
void test_InvalidateBlock(void)
{
    Fee_BlockInfoType blockInfo;
    Fee_BlockInfoType blockTable[FEE_NUMBER_OF_BLOCKS];
    uint8 data[32];
    uint16 i;

    /* Write a block first */
    blockInfo.Status = FEE_BLOCK_NOT_FOUND;
    blockInfo.DataAddress = 0u;
    for (i = 0u; i < 32u; i++) { data[i] = (uint8)(i + 1u); }
    TEST_ASSERT_EQUAL(E_OK, Fee_Sector_WriteBlock(1u, data, 32u, &blockInfo));
    TEST_ASSERT_EQUAL(FEE_BLOCK_VALID, blockInfo.Status);

    /* Invalidate it */
    TEST_ASSERT_EQUAL(E_OK, Fee_Sector_InvalidateBlock(1u, &blockInfo));
    TEST_ASSERT_EQUAL(FEE_BLOCK_INVALID, blockInfo.Status);

    /* Re-scan: block should show as invalid */
    TEST_ASSERT_EQUAL(E_OK, Fee_Sector_ScanBlocks(blockTable, FEE_NUMBER_OF_BLOCKS, Fee_BlockConfigData));
    TEST_ASSERT_EQUAL(FEE_BLOCK_INVALID, blockTable[0].Status);
}

/* ================================================================
 * Test: GarbageCollect with valid block preservation
 * ================================================================ */
void test_GarbageCollect_PreservesValidBlocks(void)
{
    Fee_BlockInfoType blockTable[FEE_NUMBER_OF_BLOCKS];
    Fee_BlockInfoType blockInfo;
    uint8 data1[32];
    uint8 data2[64];
    uint8 readBuf[64];
    uint16 i;
    uint8 oldActiveIdx;

    /* Write block 1 */
    blockInfo.Status = FEE_BLOCK_NOT_FOUND;
    blockInfo.DataAddress = 0u;
    for (i = 0u; i < 32u; i++) { data1[i] = (uint8)(i + 0xA0u); }
    TEST_ASSERT_EQUAL(E_OK, Fee_Sector_WriteBlock(1u, data1, 32u, &blockInfo));

    /* Write block 2 */
    blockInfo.Status = FEE_BLOCK_NOT_FOUND;
    blockInfo.DataAddress = 0u;
    for (i = 0u; i < 64u; i++) { data2[i] = (uint8)(i + 0xB0u); }
    TEST_ASSERT_EQUAL(E_OK, Fee_Sector_WriteBlock(2u, data2, 64u, &blockInfo));

    /* Scan to fill blockTable */
    TEST_ASSERT_EQUAL(E_OK, Fee_Sector_ScanBlocks(blockTable, FEE_NUMBER_OF_BLOCKS, Fee_BlockConfigData));
    TEST_ASSERT_EQUAL(FEE_BLOCK_VALID, blockTable[0].Status);
    TEST_ASSERT_EQUAL(FEE_BLOCK_VALID, blockTable[1].Status);

    oldActiveIdx = Fee_Sector_GetActiveSectorIndex();

    /* Trigger GC */
    TEST_ASSERT_EQUAL(E_OK, Fee_Sector_GarbageCollect(blockTable, FEE_NUMBER_OF_BLOCKS, Fee_BlockConfigData));

    /* Active sector should have changed */
    TEST_ASSERT_TRUE(Fee_Sector_GetActiveSectorIndex() != oldActiveIdx);

    /* Both blocks should still be valid */
    TEST_ASSERT_EQUAL(FEE_BLOCK_VALID, blockTable[0].Status);
    TEST_ASSERT_EQUAL(FEE_BLOCK_VALID, blockTable[1].Status);

    /* Read back and verify data */
    memset(readBuf, 0x00, sizeof(readBuf));
    TEST_ASSERT_EQUAL(E_OK, Fee_Sector_ReadBlock(&blockTable[0], 0u, readBuf, 32u));
    TEST_ASSERT_EQUAL_MEMORY(data1, readBuf, 32u);

    memset(readBuf, 0x00, sizeof(readBuf));
    TEST_ASSERT_EQUAL(E_OK, Fee_Sector_ReadBlock(&blockTable[1], 0u, readBuf, 64u));
    TEST_ASSERT_EQUAL_MEMORY(data2, readBuf, 64u);

    /* Old sector should be erased (erase count incremented) */
    TEST_ASSERT_EQUAL(1u, Fee_Sector_GetEraseCount(oldActiveIdx));
}

/* ================================================================
 * Test: GC crash recovery (two active sectors)
 * ================================================================ */
void test_GCCrashRecovery_TwoActiveSectors(void)
{
    Fee_BlockInfoType blockTable[FEE_NUMBER_OF_BLOCKS];
    Fee_BlockInfoType blockInfo;
    uint8 data1[32];
    uint8 readBuf[32];
    uint16 i;
    uint8 activeIdx;

    /* Write a block */
    blockInfo.Status = FEE_BLOCK_NOT_FOUND;
    blockInfo.DataAddress = 0u;
    for (i = 0u; i < 32u; i++) { data1[i] = (uint8)(i + 0xC0u); }
    TEST_ASSERT_EQUAL(E_OK, Fee_Sector_WriteBlock(1u, data1, 32u, &blockInfo));

    /* Now simulate interrupted GC:
     * Manually write a sector header to another erased sector with a higher sequence number.
     * This creates two active sectors -- the recovery path should pick the higher seq. */
    activeIdx = Fee_Sector_GetActiveSectorIndex();

    /* Find an erased sector */
    {
        uint8 targetIdx = 0xFFu;
        uint8 s;
        for (s = 0u; s < FEE_NUMBER_OF_SECTORS; s++)
        {
            if (s != activeIdx)
            {
                /* Check if erased by reading the raw flash */
                uint8 hdrBuf[FEE_SECTOR_HEADER_SIZE];
                MemAcc_Read(Fee_Config.SectorStartAddress + ((uint32)s * FEE_SECTOR_SIZE), hdrBuf, FEE_SECTOR_HEADER_SIZE);

                /* If all 0xFF, it's erased */
                boolean allFF = TRUE;
                uint8 j;
                for (j = 0u; j < FEE_SECTOR_HEADER_SIZE; j++)
                {
                    if (hdrBuf[j] != 0xFFu) { allFF = FALSE; break; }
                }
                if (allFF == TRUE)
                {
                    targetIdx = s;
                    break;
                }
            }
        }

        TEST_ASSERT_TRUE(targetIdx != 0xFFu);

        /* Write sector header to target with higher sequence number */
        {
            uint8 hdr[FEE_SECTOR_HEADER_SIZE];
            uint32 targetAddr = Fee_Config.SectorStartAddress + ((uint32)targetIdx * FEE_SECTOR_SIZE);
            uint32 highSeqNum = 999u; /* Much higher than current */

            memset(hdr, 0x00, FEE_SECTOR_HEADER_SIZE);
            hdr[0] = 0xE0u; hdr[1] = 0xFEu; hdr[2] = 0xE0u; hdr[3] = 0xFEu; /* FEE_SECTOR_MAGIC LE */
            hdr[4] = (uint8)(highSeqNum & 0xFFu);
            hdr[5] = (uint8)((highSeqNum >> 8u) & 0xFFu);
            hdr[6] = (uint8)((highSeqNum >> 16u) & 0xFFu);
            hdr[7] = (uint8)((highSeqNum >> 24u) & 0xFFu);
            hdr[8] = 0x00u; /* EraseCount LE low */
            hdr[9] = 0x00u; /* EraseCount LE high */
            hdr[10] = FEE_SECTOR_STATUS_ACTIVE;
            hdr[11] = 0x00u;

            TEST_ASSERT_EQUAL(E_OK, MemAcc_Write(targetAddr, hdr, FEE_SECTOR_HEADER_SIZE));
        }

        /* Re-init (simulating power-on after crash) -- do NOT reset flash */
        MemAcc_Init();
        TEST_ASSERT_EQUAL(E_OK, Fee_Sector_Init(&Fee_Config));

        /* The sector with higher sequence number should be active */
        TEST_ASSERT_EQUAL(targetIdx, Fee_Sector_GetActiveSectorIndex());

        /* The original block should still be recoverable via scan */
        TEST_ASSERT_EQUAL(E_OK, Fee_Sector_ScanBlocks(blockTable, FEE_NUMBER_OF_BLOCKS, Fee_BlockConfigData));

        /* Block 1 should be valid (found in the old active sector which is now treated as full) */
        TEST_ASSERT_EQUAL(FEE_BLOCK_VALID, blockTable[0].Status);

        /* Read back data */
        memset(readBuf, 0x00, sizeof(readBuf));
        TEST_ASSERT_EQUAL(E_OK, Fee_Sector_ReadBlock(&blockTable[0], 0u, readBuf, 32u));
        TEST_ASSERT_EQUAL_MEMORY(data1, readBuf, 32u);
    }
}

/* ================================================================
 * Test: HasSpace boundary
 * ================================================================ */
void test_HasSpace_Boundary(void)
{
    Fee_BlockInfoType blockInfo;
    uint8 data[512];
    uint16 i;

    memset(data, 0xAA, sizeof(data));

    /* Keep writing 512-byte blocks until sector is near full */
    /* Sector size = 4096, header = 12. Usable = 4084.
     * Each 512-byte block: aligned(12+512) = aligned(524) = 528 bytes.
     * 4084 / 528 = 7 blocks with 4084 - 7*528 = 4084 - 3696 = 388 remaining */
    for (i = 0u; i < 7u; i++)
    {
        blockInfo.Status = FEE_BLOCK_NOT_FOUND;
        blockInfo.DataAddress = 0u;
        TEST_ASSERT_EQUAL(E_OK, Fee_Sector_WriteBlock(7u, data, 512u, &blockInfo));
    }

    /* Now 388 bytes remaining -- not enough for another 512-byte block (needs 528) */
    TEST_ASSERT_FALSE(Fee_Sector_HasSpace(528u));

    /* But should have space for a small block */
    /* 8-byte block: aligned(12+8) = aligned(20) = 24 bytes */
    TEST_ASSERT_TRUE(Fee_Sector_HasSpace(24u));
}

/* ================================================================
 * Test: Full sector write rejection
 * ================================================================ */
void test_WriteToFullSector_Rejected(void)
{
    Fee_BlockInfoType blockInfo;
    uint8 data[512];

    memset(data, 0xAA, sizeof(data));

    /* Fill the sector */
    {
        uint16 i;
        for (i = 0u; i < 7u; i++)
        {
            blockInfo.Status = FEE_BLOCK_NOT_FOUND;
            blockInfo.DataAddress = 0u;
            Fee_Sector_WriteBlock(7u, data, 512u, &blockInfo);
        }
    }

    /* Try to write another 512-byte block -- should fail */
    blockInfo.Status = FEE_BLOCK_NOT_FOUND;
    blockInfo.DataAddress = 0u;
    TEST_ASSERT_EQUAL(E_NOT_OK, Fee_Sector_WriteBlock(7u, data, 512u, &blockInfo));
}

/* ================================================================
 * Test: EraseImmediate
 * ================================================================ */
void test_EraseImmediate(void)
{
    Fee_BlockInfoType blockTable[FEE_NUMBER_OF_BLOCKS];
    Fee_BlockInfoType blockInfo;
    uint8 data[32];
    uint16 i;

    /* Write block 1 */
    blockInfo.Status = FEE_BLOCK_NOT_FOUND;
    blockInfo.DataAddress = 0u;
    for (i = 0u; i < 32u; i++) { data[i] = (uint8)i; }
    TEST_ASSERT_EQUAL(E_OK, Fee_Sector_WriteBlock(1u, data, 32u, &blockInfo));
    TEST_ASSERT_EQUAL(FEE_BLOCK_VALID, blockInfo.Status);

    /* EraseImmediate */
    TEST_ASSERT_EQUAL(E_OK, Fee_Sector_EraseImmediate(1u, &blockInfo, blockTable, FEE_NUMBER_OF_BLOCKS, Fee_BlockConfigData));
    TEST_ASSERT_EQUAL(FEE_BLOCK_NOT_FOUND, blockInfo.Status);

    /* Verify on flash: scan should show block as invalid (it was invalidated on flash) */
    TEST_ASSERT_EQUAL(E_OK, Fee_Sector_ScanBlocks(blockTable, FEE_NUMBER_OF_BLOCKS, Fee_BlockConfigData));
    TEST_ASSERT_EQUAL(FEE_BLOCK_INVALID, blockTable[0].Status);
}

/* ================================================================
 * Test: Corrupt header scan termination
 * ================================================================ */
void test_CorruptHeader_ScanTermination(void)
{
    Fee_BlockInfoType blockTable[FEE_NUMBER_OF_BLOCKS];
    Fee_BlockInfoType blockInfo;
    uint8 data[32];
    uint16 i;

    /* Write a valid block first */
    blockInfo.Status = FEE_BLOCK_NOT_FOUND;
    blockInfo.DataAddress = 0u;
    for (i = 0u; i < 32u; i++) { data[i] = (uint8)(i + 1u); }
    TEST_ASSERT_EQUAL(E_OK, Fee_Sector_WriteBlock(1u, data, 32u, &blockInfo));

    /* Now manually write a corrupt header after the valid block.
     * The corrupt header has BlockLength > FEE_SECTOR_SIZE */
    {
        uint8 corruptHdr[11];
        uint32 nextPos;

        /* The next write position: we know block 1 = aligned(12+32)=48 bytes from sector data start */
        nextPos = Fee_Config.SectorStartAddress +
                  ((uint32)Fee_Sector_GetActiveSectorIndex() * FEE_SECTOR_SIZE) +
                  FEE_SECTOR_HEADER_SIZE + 48u;

        memset(corruptHdr, 0x00, sizeof(corruptHdr));
        corruptHdr[0] = 0x02u; /* BlockNumber=2 (LE low) */
        corruptHdr[1] = 0x00u;
        corruptHdr[2] = 0xFFu; /* BlockLength=0xFFFF > FEE_SECTOR_SIZE (LE low) */
        corruptHdr[3] = 0xFFu;
        corruptHdr[4] = 0x00u; /* CRC */
        corruptHdr[5] = 0x00u;

        TEST_ASSERT_EQUAL(E_OK, MemAcc_Write(nextPos, corruptHdr, 11u));
    }

    /* Scan should still find block 1 but stop at the corrupt header */
    TEST_ASSERT_EQUAL(E_OK, Fee_Sector_ScanBlocks(blockTable, FEE_NUMBER_OF_BLOCKS, Fee_BlockConfigData));
    TEST_ASSERT_EQUAL(FEE_BLOCK_VALID, blockTable[0].Status);

    /* Block 2 should not be found (corrupt header caused scan to stop) */
    TEST_ASSERT_EQUAL(FEE_BLOCK_NOT_FOUND, blockTable[1].Status);
}

/* ================================================================
 * Test: Persistence (re-init without flash reset)
 * ================================================================ */
void test_Persistence_ReInitWithoutFlashReset(void)
{
    Fee_BlockInfoType blockTable[FEE_NUMBER_OF_BLOCKS];
    Fee_BlockInfoType blockInfo;
    uint8 data1[32];
    uint8 data2[64];
    uint8 readBuf[64];
    uint16 i;

    /* Write block 1 */
    blockInfo.Status = FEE_BLOCK_NOT_FOUND;
    blockInfo.DataAddress = 0u;
    for (i = 0u; i < 32u; i++) { data1[i] = (uint8)(i + 0xD0u); }
    TEST_ASSERT_EQUAL(E_OK, Fee_Sector_WriteBlock(1u, data1, 32u, &blockInfo));

    /* Write block 2 */
    blockInfo.Status = FEE_BLOCK_NOT_FOUND;
    blockInfo.DataAddress = 0u;
    for (i = 0u; i < 64u; i++) { data2[i] = (uint8)(i + 0xE0u); }
    TEST_ASSERT_EQUAL(E_OK, Fee_Sector_WriteBlock(2u, data2, 64u, &blockInfo));

    /* Re-init without resetting flash (simulating power cycle) */
    MemAcc_Init();
    TEST_ASSERT_EQUAL(E_OK, Fee_Sector_Init(&Fee_Config));

    /* Scan blocks */
    TEST_ASSERT_EQUAL(E_OK, Fee_Sector_ScanBlocks(blockTable, FEE_NUMBER_OF_BLOCKS, Fee_BlockConfigData));

    /* Both blocks should be recovered */
    TEST_ASSERT_EQUAL(FEE_BLOCK_VALID, blockTable[0].Status);
    TEST_ASSERT_EQUAL(FEE_BLOCK_VALID, blockTable[1].Status);

    /* Read and verify data */
    memset(readBuf, 0x00, sizeof(readBuf));
    TEST_ASSERT_EQUAL(E_OK, Fee_Sector_ReadBlock(&blockTable[0], 0u, readBuf, 32u));
    TEST_ASSERT_EQUAL_MEMORY(data1, readBuf, 32u);

    memset(readBuf, 0x00, sizeof(readBuf));
    TEST_ASSERT_EQUAL(E_OK, Fee_Sector_ReadBlock(&blockTable[1], 0u, readBuf, 64u));
    TEST_ASSERT_EQUAL_MEMORY(data2, readBuf, 64u);
}

/* ================================================================
 * Test: ReadBlock with partial offset
 * ================================================================ */
void test_ReadBlock_WithOffset(void)
{
    Fee_BlockInfoType blockInfo;
    uint8 data[32];
    uint8 readBuf[16];
    uint16 i;

    blockInfo.Status = FEE_BLOCK_NOT_FOUND;
    blockInfo.DataAddress = 0u;
    for (i = 0u; i < 32u; i++) { data[i] = (uint8)i; }
    TEST_ASSERT_EQUAL(E_OK, Fee_Sector_WriteBlock(1u, data, 32u, &blockInfo));

    /* Read 16 bytes starting at offset 10 */
    memset(readBuf, 0xFFu, sizeof(readBuf));
    TEST_ASSERT_EQUAL(E_OK, Fee_Sector_ReadBlock(&blockInfo, 10u, readBuf, 16u));
    TEST_ASSERT_EQUAL_MEMORY(&data[10], readBuf, 16u);
}

/* ================================================================
 * Test: ReadBlock on invalid block returns E_NOT_OK
 * ================================================================ */
void test_ReadBlock_InvalidBlock(void)
{
    Fee_BlockInfoType blockInfo;
    uint8 buf[16];

    blockInfo.Status = FEE_BLOCK_INVALID;
    blockInfo.DataAddress = 100u;

    TEST_ASSERT_EQUAL(E_NOT_OK, Fee_Sector_ReadBlock(&blockInfo, 0u, buf, 16u));
}

/* ================================================================
 * Test: InvalidateBlock on never-written block returns E_NOT_OK
 * ================================================================ */
void test_InvalidateBlock_NeverWritten(void)
{
    Fee_BlockInfoType blockInfo;

    blockInfo.Status = FEE_BLOCK_NOT_FOUND;
    blockInfo.DataAddress = 0u;

    TEST_ASSERT_EQUAL(E_NOT_OK, Fee_Sector_InvalidateBlock(1u, &blockInfo));
}

/* ================================================================
 * Test: GetEraseCount for invalid index returns 0
 * ================================================================ */
void test_GetEraseCount_InvalidIndex(void)
{
    TEST_ASSERT_EQUAL(0u, Fee_Sector_GetEraseCount(FEE_NUMBER_OF_SECTORS));
    TEST_ASSERT_EQUAL(0u, Fee_Sector_GetEraseCount(255u));
}

/* ================================================================
 * Main
 * ================================================================ */
int main(void)
{
    UNITY_BEGIN();

    /* Sector init */
    RUN_TEST(test_SectorInit_BlankFlash_OneSectorActive);

    /* CRC */
    RUN_TEST(test_Crc16_TestVector);
    RUN_TEST(test_Crc16_EmptyData);

    /* Write + Read round-trip */
    RUN_TEST(test_WriteRead_Size8);
    RUN_TEST(test_WriteRead_Size32);
    RUN_TEST(test_WriteRead_Size64);
    RUN_TEST(test_WriteRead_Size128);
    RUN_TEST(test_WriteRead_Size256);
    RUN_TEST(test_WriteRead_Size512);

    /* ScanBlocks */
    RUN_TEST(test_ScanBlocks_AfterWrites);

    /* Two-phase commit */
    RUN_TEST(test_TwoPhaseCommit_IncompleteWriteIgnored);

    /* InvalidateBlock */
    RUN_TEST(test_InvalidateBlock);

    /* GarbageCollect */
    RUN_TEST(test_GarbageCollect_PreservesValidBlocks);

    /* GC crash recovery */
    RUN_TEST(test_GCCrashRecovery_TwoActiveSectors);

    /* HasSpace boundary */
    RUN_TEST(test_HasSpace_Boundary);

    /* Full sector write rejection */
    RUN_TEST(test_WriteToFullSector_Rejected);

    /* EraseImmediate */
    RUN_TEST(test_EraseImmediate);

    /* Corrupt header scan termination */
    RUN_TEST(test_CorruptHeader_ScanTermination);

    /* Persistence */
    RUN_TEST(test_Persistence_ReInitWithoutFlashReset);

    /* Read with offset */
    RUN_TEST(test_ReadBlock_WithOffset);

    /* Read invalid block */
    RUN_TEST(test_ReadBlock_InvalidBlock);

    /* InvalidateBlock never-written */
    RUN_TEST(test_InvalidateBlock_NeverWritten);

    /* GetEraseCount invalid index */
    RUN_TEST(test_GetEraseCount_InvalidIndex);

    return UNITY_END();
}
