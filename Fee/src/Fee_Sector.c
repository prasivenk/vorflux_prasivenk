#include "Fee_Sector.h"
#include "Fee_Cfg.h"
#include "MemAcc.h"
#include <string.h>

/* --------------- Static data --------------- */
static Fee_SectorInfoType Fee_SectorInfo[FEE_NUMBER_OF_SECTORS];
static uint8 Fee_ActiveSectorIndex;
static const Fee_ConfigType* Fee_SectorConfigPtr;
static uint32 Fee_NextSequenceNumber;

/* --------------- Helper: little-endian serialization --------------- */
static void WriteLE16(uint8* buf, uint16 val)
{
    buf[0] = (uint8)(val & 0xFFu);
    buf[1] = (uint8)((val >> 8u) & 0xFFu);
}

static void WriteLE32(uint8* buf, uint32 val)
{
    buf[0] = (uint8)(val & 0xFFu);
    buf[1] = (uint8)((val >> 8u) & 0xFFu);
    buf[2] = (uint8)((val >> 16u) & 0xFFu);
    buf[3] = (uint8)((val >> 24u) & 0xFFu);
}

static uint16 ReadLE16(const uint8* buf)
{
    return (uint16)((uint16)buf[0] | ((uint16)buf[1] << 8u));
}

static uint32 ReadLE32(const uint8* buf)
{
    return (uint32)buf[0]
         | ((uint32)buf[1] << 8u)
         | ((uint32)buf[2] << 16u)
         | ((uint32)buf[3] << 24u);
}

/* --------------- Helper: align to virtual page --------------- */
uint32 Fee_Sector_AlignToPage(uint32 size)
{
    return ((size + FEE_VIRTUAL_PAGE_SIZE - 1u) / FEE_VIRTUAL_PAGE_SIZE) * FEE_VIRTUAL_PAGE_SIZE;
}

/* Internal shorthand kept for readability within this file */
static uint32 AlignToPage(uint32 size)
{
    return Fee_Sector_AlignToPage(size);
}

/* --------------- Helper: check if block header bytes are all 0xFF --------------- */
static boolean IsBlankHeader(const uint8* buf)
{
    uint8 i;
    for (i = 0u; i < FEE_BLOCK_HEADER_SIZE; i++)
    {
        if (buf[i] != 0xFFu)
        {
            return FALSE;
        }
    }
    return TRUE;
}

/* --------------- CRC-16 CCITT (polynomial 0x1021, no final XOR) --------------- */
uint16 Fee_Crc16(const uint8* DataPtr, uint32 Length, uint16 InitialValue)
{
    uint16 crc = InitialValue;
    uint32 i;
    uint8 j;

    for (i = 0u; i < Length; i++)
    {
        crc ^= ((uint16)DataPtr[i] << 8u);
        for (j = 0u; j < 8u; j++)
        {
            if ((crc & 0x8000u) != 0u)
            {
                crc = (uint16)((crc << 1u) ^ 0x1021u);
            }
            else
            {
                crc = (uint16)(crc << 1u);
            }
        }
    }
    return crc;
}

/* --------------- Helper: write sector header to flash --------------- */
static Std_ReturnType WriteSectorHeader(uint8 sectorIdx)
{
    uint8 hdr[FEE_SECTOR_HEADER_SIZE];

    memset(hdr, 0x00, FEE_SECTOR_HEADER_SIZE);
    WriteLE32(&hdr[0], FEE_SECTOR_MAGIC);
    WriteLE32(&hdr[4], Fee_SectorInfo[sectorIdx].SequenceNumber);
    WriteLE16(&hdr[8], Fee_SectorInfo[sectorIdx].EraseCount);
    hdr[FEE_SECTOR_STATUS_OFFSET] = Fee_SectorInfo[sectorIdx].Status;
    hdr[FEE_SECTOR_STATUS_OFFSET + 1u] = 0x00u; /* Reserved */

    return MemAcc_Write(Fee_SectorInfo[sectorIdx].StartAddress, hdr, FEE_SECTOR_HEADER_SIZE);
}

/* --------------- Helper: scan sector to find write pointer --------------- */
static uint32 FindWritePointer(uint8 sectorIdx)
{
    uint32 startAddr = Fee_SectorInfo[sectorIdx].StartAddress;
    uint32 sectorEnd = startAddr + FEE_SECTOR_SIZE;
    uint32 readPos = startAddr + FEE_SECTOR_HEADER_SIZE;
    uint8 headerBuf[FEE_BLOCK_HEADER_SIZE];

    while ((readPos + FEE_BLOCK_HEADER_SIZE) <= sectorEnd)
    {
        if (MemAcc_Read(readPos, headerBuf, FEE_BLOCK_HEADER_SIZE) != E_OK)
        {
            break;
        }

        /* If all header bytes are 0xFF, this is free space */
        if (IsBlankHeader(headerBuf) == TRUE)
        {
            break;
        }

        /* Deserialize to get block length */
        uint16 blockLength = ReadLE16(&headerBuf[2]);

        /* Validation: if block length is 0 or exceeds sector size, stop */
        if ((blockLength == 0u) || (blockLength > FEE_SECTOR_SIZE))
        {
            break;
        }

        uint32 recordSize = AlignToPage((uint32)FEE_BLOCK_HEADER_SIZE + (uint32)blockLength);

        if ((readPos + recordSize) > sectorEnd)
        {
            break;
        }

        readPos += recordSize;
    }

    return readPos;
}

/* --------------- Fee_Sector_Init --------------- */
Std_ReturnType Fee_Sector_Init(const Fee_ConfigType* ConfigPtr)
{
    uint8 i;
    uint8 headerBuf[FEE_SECTOR_HEADER_SIZE];
    uint8 activeCount = 0u;
    uint8 highestSeqActiveIdx = 0u;
    uint32 highestSeqNum = 0u;
    boolean foundActive = FALSE;

    if (ConfigPtr == NULL_PTR)
    {
        return E_NOT_OK;
    }

    Fee_SectorConfigPtr = ConfigPtr;
    Fee_NextSequenceNumber = 0u;
    Fee_ActiveSectorIndex = 0u;

    /* Step 2: Read all sector headers */
    for (i = 0u; i < FEE_NUMBER_OF_SECTORS; i++)
    {
        Fee_SectorInfo[i].StartAddress = ConfigPtr->SectorStartAddress + ((uint32)i * FEE_SECTOR_SIZE);

        if (MemAcc_Read(Fee_SectorInfo[i].StartAddress, headerBuf, FEE_SECTOR_HEADER_SIZE) != E_OK)
        {
            return E_NOT_OK;
        }

        uint32 magic = ReadLE32(&headerBuf[0]);

        if (magic == FEE_SECTOR_MAGIC)
        {
            Fee_SectorInfo[i].SequenceNumber = ReadLE32(&headerBuf[4]);
            Fee_SectorInfo[i].EraseCount = ReadLE16(&headerBuf[8]);
            Fee_SectorInfo[i].Status = headerBuf[FEE_SECTOR_STATUS_OFFSET];

            /* Find write pointer by scanning block entries */
            Fee_SectorInfo[i].WritePointer = FindWritePointer(i);

            /* Track next sequence number */
            if ((Fee_SectorInfo[i].SequenceNumber + 1u) > Fee_NextSequenceNumber)
            {
                Fee_NextSequenceNumber = Fee_SectorInfo[i].SequenceNumber + 1u;
            }

            /* Track active sectors */
            if (Fee_SectorInfo[i].Status == FEE_SECTOR_STATUS_ACTIVE)
            {
                activeCount++;
                if ((foundActive == FALSE) || (Fee_SectorInfo[i].SequenceNumber > highestSeqNum))
                {
                    highestSeqNum = Fee_SectorInfo[i].SequenceNumber;
                    highestSeqActiveIdx = i;
                }
                foundActive = TRUE;
            }
        }
        else
        {
            /* Not a valid sector header -- treat as erased */
            Fee_SectorInfo[i].EraseCount = 0u;
            Fee_SectorInfo[i].SequenceNumber = 0u;
            Fee_SectorInfo[i].Status = FEE_SECTOR_STATUS_ERASED;
            Fee_SectorInfo[i].WritePointer = Fee_SectorInfo[i].StartAddress + FEE_SECTOR_HEADER_SIZE;
        }
    }

    /* Step 3: Recovery from interrupted GC */
    if (activeCount > 1u)
    {
        /* The sector with the highest sequence number is the true active sector.
         * Mark all other active sectors as full. */
        for (i = 0u; i < FEE_NUMBER_OF_SECTORS; i++)
        {
            if ((Fee_SectorInfo[i].Status == FEE_SECTOR_STATUS_ACTIVE) && (i != highestSeqActiveIdx))
            {
                Fee_SectorInfo[i].Status = FEE_SECTOR_STATUS_FULL;
            }
        }
    }

    /* Step 4: Determine active sector */
    if (foundActive == TRUE)
    {
        Fee_ActiveSectorIndex = highestSeqActiveIdx;
    }
    else
    {
        /* No active sector found -- pick erased sector with lowest erase count */
        boolean found = FALSE;
        uint16 lowestEraseCount = 0xFFFFu;
        uint8 targetIdx = 0u;

        for (i = 0u; i < FEE_NUMBER_OF_SECTORS; i++)
        {
            if (Fee_SectorInfo[i].Status == FEE_SECTOR_STATUS_ERASED)
            {
                if ((found == FALSE) || (Fee_SectorInfo[i].EraseCount < lowestEraseCount))
                {
                    lowestEraseCount = Fee_SectorInfo[i].EraseCount;
                    targetIdx = i;
                    found = TRUE;
                }
            }
        }

        if (found == FALSE)
        {
            return E_NOT_OK; /* No usable sector */
        }

        /* Erase the sector before formatting to ensure flash is in erased state.
         * This handles the case where MemAcc backing store is not pre-erased
         * (e.g., fresh process with zeroed static storage). */
        if (MemAcc_Erase(Fee_SectorInfo[targetIdx].StartAddress, FEE_SECTOR_SIZE) != E_OK)
        {
            return E_NOT_OK;
        }

        /* Format the sector: write header */
        Fee_SectorInfo[targetIdx].SequenceNumber = Fee_NextSequenceNumber;
        Fee_NextSequenceNumber++;
        Fee_SectorInfo[targetIdx].Status = FEE_SECTOR_STATUS_ACTIVE;
        Fee_SectorInfo[targetIdx].WritePointer = Fee_SectorInfo[targetIdx].StartAddress + FEE_SECTOR_HEADER_SIZE;

        if (WriteSectorHeader(targetIdx) != E_OK)
        {
            return E_NOT_OK;
        }

        Fee_ActiveSectorIndex = targetIdx;
    }

    return E_OK;
}

/* --------------- Fee_Sector_ScanBlocks --------------- */
Std_ReturnType Fee_Sector_ScanBlocks(Fee_BlockInfoType* BlockInfoTable, uint16 NumBlocks, const Fee_BlockConfigType* BlockConfig)
{
    uint16 i;
    uint8 s;
    uint8 scanOrder[FEE_NUMBER_OF_SECTORS];
    uint8 scanCount = 0u;

    if ((BlockInfoTable == NULL_PTR) || (BlockConfig == NULL_PTR))
    {
        return E_NOT_OK;
    }

    /* Step 1: Initialize status and address; preserve Immediate flag (set by caller) */
    for (i = 0u; i < NumBlocks; i++)
    {
        BlockInfoTable[i].Status = FEE_BLOCK_NOT_FOUND;
        BlockInfoTable[i].DataAddress = 0u;
    }

    /* Step 2: Build scan order -- sectors sorted by SequenceNumber ascending */
    /* First, collect sectors that are ACTIVE or FULL */
    for (s = 0u; s < FEE_NUMBER_OF_SECTORS; s++)
    {
        if ((Fee_SectorInfo[s].Status == FEE_SECTOR_STATUS_ACTIVE) ||
            (Fee_SectorInfo[s].Status == FEE_SECTOR_STATUS_FULL))
        {
            scanOrder[scanCount] = s;
            scanCount++;
        }
    }

    /* Simple insertion sort by SequenceNumber ascending */
    {
        uint8 a, b;
        for (a = 1u; a < scanCount; a++)
        {
            uint8 key = scanOrder[a];
            uint32 keySeq = Fee_SectorInfo[key].SequenceNumber;
            b = a;
            while ((b > 0u) && (Fee_SectorInfo[scanOrder[b - 1u]].SequenceNumber > keySeq))
            {
                scanOrder[b] = scanOrder[b - 1u];
                b--;
            }
            scanOrder[b] = key;
        }
    }

    /* Step 3: Scan each sector */
    for (s = 0u; s < scanCount; s++)
    {
        uint8 sectorIdx = scanOrder[s];
        uint32 startAddr = Fee_SectorInfo[sectorIdx].StartAddress;
        uint32 sectorEnd = startAddr + FEE_SECTOR_SIZE;
        uint32 readPos = startAddr + FEE_SECTOR_HEADER_SIZE;

        while ((readPos + FEE_BLOCK_HEADER_SIZE) <= sectorEnd)
        {
            uint8 headerBuf[FEE_BLOCK_HEADER_SIZE];
            uint16 blockNumber;
            uint16 blockLength;
            uint16 crcStored;
            uint8 validMarker;
            uint32 dataAddress;
            uint16 configIndex;
            boolean configFound;
            uint32 recordSize;

            if (MemAcc_Read(readPos, headerBuf, FEE_BLOCK_HEADER_SIZE) != E_OK)
            {
                break;
            }

            /* If all bytes are 0xFF, we reached free space */
            if (IsBlankHeader(headerBuf) == TRUE)
            {
                break;
            }

            blockNumber = ReadLE16(&headerBuf[0]);
            blockLength = ReadLE16(&headerBuf[2]);
            crcStored = ReadLE16(&headerBuf[4]);
            validMarker = headerBuf[11];

            /* Validation: corrupt header check */
            if ((blockLength == 0u) || (blockLength > FEE_SECTOR_SIZE) ||
                ((readPos + FEE_BLOCK_HEADER_SIZE + (uint32)blockLength) > sectorEnd))
            {
                break; /* Stop scanning this sector */
            }

            recordSize = AlignToPage((uint32)FEE_BLOCK_HEADER_SIZE + (uint32)blockLength);
            dataAddress = readPos + FEE_BLOCK_HEADER_SIZE;

            /* If ValidMarker is 0xFF (pending/incomplete), skip */
            if (validMarker == FEE_VALID_MARKER_ERASED)
            {
                readPos += recordSize;
                continue;
            }

            /* Find config index by matching BlockNumber */
            configFound = FALSE;
            configIndex = 0u;
            for (i = 0u; i < NumBlocks; i++)
            {
                if (BlockConfig[i].BlockNumber == blockNumber)
                {
                    configIndex = i;
                    configFound = TRUE;
                    break;
                }
            }

            /* If not found or size mismatch, skip (orphan or mismatched block) */
            if ((configFound == FALSE) || (blockLength != BlockConfig[configIndex].BlockSize))
            {
                readPos += recordSize;
                continue;
            }

            if (validMarker == FEE_VALID_MARKER_VALID)
            {
                /* Validate CRC */
                uint8 crcBuf[4];
                uint16 crcComputed;

                WriteLE16(&crcBuf[0], blockNumber);
                WriteLE16(&crcBuf[2], blockLength);
                crcComputed = Fee_Crc16(crcBuf, 4u, 0xFFFFu);

                /* Continue CRC over data */
                {
                    uint8 dataBuf[FEE_MAX_BLOCK_SIZE];
                    uint16 remaining = blockLength;
                    uint32 dataOff = 0u;

                    while (remaining > 0u)
                    {
                        uint16 chunkSize = (remaining > (uint16)sizeof(dataBuf)) ? (uint16)sizeof(dataBuf) : remaining;
                        if (MemAcc_Read(dataAddress + dataOff, dataBuf, chunkSize) != E_OK)
                        {
                            break;
                        }
                        crcComputed = Fee_Crc16(dataBuf, (uint32)chunkSize, crcComputed);
                        dataOff += chunkSize;
                        remaining -= chunkSize;
                    }
                }

                if (crcComputed == crcStored)
                {
                    BlockInfoTable[configIndex].DataAddress = dataAddress;
                    BlockInfoTable[configIndex].Status = FEE_BLOCK_VALID;
                    BlockInfoTable[configIndex].Immediate = BlockConfig[configIndex].ImmediateData;
                }
                else
                {
                    BlockInfoTable[configIndex].Status = FEE_BLOCK_INCONSISTENT;
                    BlockInfoTable[configIndex].DataAddress = dataAddress;
                }
            }
            else if (validMarker == FEE_VALID_MARKER_INVALID)
            {
                BlockInfoTable[configIndex].Status = FEE_BLOCK_INVALID;
                BlockInfoTable[configIndex].DataAddress = dataAddress;
            }

            readPos += recordSize;
        }
    }

    return E_OK;
}

/* --------------- Fee_Sector_WriteBlock --------------- */
Std_ReturnType Fee_Sector_WriteBlock(uint16 BlockNumber, const uint8* DataPtr, uint16 DataLength, Fee_BlockInfoType* BlockInfo)
{
    uint32 totalSize;
    uint32 writeAddr;
    uint16 crc;
    uint8 headerBytes[FEE_BLOCK_HEADER_SIZE - 1u]; /* 11 bytes: header without ValidMarker */
    uint8 crcBuf[4];
    uint8 marker;

    if ((DataPtr == NULL_PTR) || (BlockInfo == NULL_PTR))
    {
        return E_NOT_OK;
    }

    /* Step 1: Compute total aligned size */
    totalSize = AlignToPage((uint32)FEE_BLOCK_HEADER_SIZE + (uint32)DataLength);

    /* Step 2: Check space */
    if (Fee_Sector_HasSpace((uint16)totalSize) == FALSE)
    {
        return E_NOT_OK;
    }

    /* Step 3: Get write address */
    writeAddr = Fee_SectorInfo[Fee_ActiveSectorIndex].WritePointer;

    /* Step 4: Compute CRC over BlockNumber(2B) + DataLength(2B) + data */
    WriteLE16(&crcBuf[0], BlockNumber);
    WriteLE16(&crcBuf[2], DataLength);
    crc = Fee_Crc16(crcBuf, 4u, 0xFFFFu);
    crc = Fee_Crc16(DataPtr, (uint32)DataLength, crc);

    /* Step 5: Phase 1 -- Build header bytes 0-10 (11 bytes) */
    memset(headerBytes, 0x00, sizeof(headerBytes));
    WriteLE16(&headerBytes[0], BlockNumber);
    WriteLE16(&headerBytes[2], DataLength);
    WriteLE16(&headerBytes[4], crc);
    /* bytes 6-10 = reserved (0x00) -- already zeroed by memset */

    /* Write header bytes 0-10 (leaving byte 11 as 0xFF on flash) */
    if (MemAcc_Write(writeAddr, headerBytes, FEE_BLOCK_HEADER_SIZE - 1u) != E_OK)
    {
        return E_NOT_OK;
    }

    /* Step 6: Write data */
    if (MemAcc_Write(writeAddr + FEE_BLOCK_HEADER_SIZE, DataPtr, DataLength) != E_OK)
    {
        return E_NOT_OK;
    }

    /* Write padding bytes if needed */
    {
        uint32 dataEnd = (uint32)FEE_BLOCK_HEADER_SIZE + (uint32)DataLength;
        if (totalSize > dataEnd)
        {
            uint32 padLen = totalSize - dataEnd;
            uint8 padBuf[8]; /* FEE_VIRTUAL_PAGE_SIZE max */
            memset(padBuf, 0x00, sizeof(padBuf));
            if (MemAcc_Write(writeAddr + dataEnd, padBuf, padLen) != E_OK)
            {
                return E_NOT_OK;
            }
        }
    }

    /* Step 7: Phase 2 -- Write ValidMarker = 0xAA to byte 11 */
    marker = FEE_VALID_MARKER_VALID;
    if (MemAcc_Write(writeAddr + FEE_BLOCK_VALID_MARKER_OFFSET, &marker, 1u) != E_OK)
    {
        return E_NOT_OK;
    }

    /* Step 8: Update write pointer */
    Fee_SectorInfo[Fee_ActiveSectorIndex].WritePointer += totalSize;

    /* Step 9: Update block info */
    BlockInfo->DataAddress = writeAddr + FEE_BLOCK_HEADER_SIZE;
    BlockInfo->Status = FEE_BLOCK_VALID;

    return E_OK;
}

/* --------------- Fee_Sector_ReadBlock --------------- */
Std_ReturnType Fee_Sector_ReadBlock(const Fee_BlockInfoType* BlockInfo, uint16 BlockOffset, uint8* DataBufferPtr, uint16 Length)
{
    if ((BlockInfo == NULL_PTR) || (DataBufferPtr == NULL_PTR))
    {
        return E_NOT_OK;
    }

    if (BlockInfo->Status != FEE_BLOCK_VALID)
    {
        return E_NOT_OK;
    }

    return MemAcc_Read(BlockInfo->DataAddress + (uint32)BlockOffset, DataBufferPtr, (MemAcc_LengthType)Length);
}

/* --------------- Fee_Sector_InvalidateBlock --------------- */
Std_ReturnType Fee_Sector_InvalidateBlock(Fee_BlockInfoType* BlockInfo)
{
    uint32 headerAddr;
    uint32 markerAddr;
    uint8 invalidByte = FEE_VALID_MARKER_INVALID;

    if (BlockInfo == NULL_PTR)
    {
        return E_NOT_OK;
    }

    if (BlockInfo->DataAddress == 0u)
    {
        return E_NOT_OK; /* Never written */
    }

    headerAddr = BlockInfo->DataAddress - FEE_BLOCK_HEADER_SIZE;
    markerAddr = headerAddr + FEE_BLOCK_VALID_MARKER_OFFSET;

    if (MemAcc_Write(markerAddr, &invalidByte, 1u) != E_OK)
    {
        return E_NOT_OK;
    }

    BlockInfo->Status = FEE_BLOCK_INVALID;

    return E_OK;
}

/* --------------- Fee_Sector_EraseImmediate --------------- */
Std_ReturnType Fee_Sector_EraseImmediate(uint16 BlockNumber, Fee_BlockInfoType* BlockInfo,
    uint16 BlockSize, Fee_BlockInfoType* BlockInfoTable, uint16 NumBlocks, const Fee_BlockConfigType* BlockConfig)
{
    uint32 requiredSpace;

    (void)BlockNumber; /* Kept for AUTOSAR API consistency; operation uses BlockInfo */

    if ((BlockInfo == NULL_PTR) || (BlockInfoTable == NULL_PTR) || (BlockConfig == NULL_PTR))
    {
        return E_NOT_OK;
    }

    /* Step 1: If block has valid data, invalidate first */
    if (BlockInfo->Status == FEE_BLOCK_VALID)
    {
        if (Fee_Sector_InvalidateBlock(BlockInfo) != E_OK)
        {
            return E_NOT_OK;
        }
    }

    /* Step 2: Compute required space */
    requiredSpace = AlignToPage((uint32)FEE_BLOCK_HEADER_SIZE + (uint32)BlockSize);

    /* Step 3: If no space, trigger GC */
    if (Fee_Sector_HasSpace((uint16)requiredSpace) == FALSE)
    {
        if (Fee_Sector_GarbageCollect(BlockInfoTable, NumBlocks, BlockConfig) != E_OK)
        {
            return E_NOT_OK;
        }
    }

    /* Step 4: Set block status */
    BlockInfo->Status = FEE_BLOCK_NOT_FOUND;

    return E_OK;
}

/* --------------- Fee_Sector_GarbageCollect --------------- */
Std_ReturnType Fee_Sector_GarbageCollect(Fee_BlockInfoType* BlockInfoTable, uint16 NumBlocks, const Fee_BlockConfigType* BlockConfig)
{
    uint8 i;
    uint8 targetIdx = 0u;
    boolean foundErased = FALSE;
    uint16 lowestEraseCount = 0xFFFFu;
    uint8 oldActiveIdx;
    uint32 oldStart;
    uint8 fullByte = FEE_SECTOR_STATUS_FULL;

    if ((BlockInfoTable == NULL_PTR) || (BlockConfig == NULL_PTR))
    {
        return E_NOT_OK;
    }

    /* Step 1: Find target sector -- erased sector with lowest erase count */
    for (i = 0u; i < FEE_NUMBER_OF_SECTORS; i++)
    {
        if (Fee_SectorInfo[i].Status == FEE_SECTOR_STATUS_ERASED)
        {
            if ((foundErased == FALSE) || (Fee_SectorInfo[i].EraseCount < lowestEraseCount))
            {
                lowestEraseCount = Fee_SectorInfo[i].EraseCount;
                targetIdx = i;
                foundErased = TRUE;
            }
        }
    }

    if (foundErased == FALSE)
    {
        return E_NOT_OK;
    }

    /* Step 2: Write sector header to target */
    Fee_SectorInfo[targetIdx].SequenceNumber = Fee_NextSequenceNumber;
    Fee_NextSequenceNumber++;
    Fee_SectorInfo[targetIdx].Status = FEE_SECTOR_STATUS_ACTIVE;
    Fee_SectorInfo[targetIdx].WritePointer = Fee_SectorInfo[targetIdx].StartAddress + FEE_SECTOR_HEADER_SIZE;

    if (WriteSectorHeader(targetIdx) != E_OK)
    {
        return E_NOT_OK;
    }

    /* Step 3: Save old active and switch to new */
    oldActiveIdx = Fee_ActiveSectorIndex;
    Fee_ActiveSectorIndex = targetIdx;

    /* Step 4: Copy valid blocks; reset stale entries for non-copied blocks */
    for (i = 0u; i < (uint8)NumBlocks; i++)
    {
        if (BlockInfoTable[i].Status == FEE_BLOCK_VALID)
        {
            uint8 tempBuf[FEE_MAX_BLOCK_SIZE];
            uint16 blockSize = BlockConfig[i].BlockSize;

            if (MemAcc_Read(BlockInfoTable[i].DataAddress, tempBuf, (MemAcc_LengthType)blockSize) != E_OK)
            {
                return E_NOT_OK;
            }

            if (Fee_Sector_WriteBlock(BlockConfig[i].BlockNumber, tempBuf, blockSize, &BlockInfoTable[i]) != E_OK)
            {
                return E_NOT_OK;
            }
        }
        else if ((BlockInfoTable[i].Status == FEE_BLOCK_INVALID) ||
                 (BlockInfoTable[i].Status == FEE_BLOCK_INCONSISTENT))
        {
            /* Clear stale DataAddress for entries not copied to the new sector.
             * The old sector will be erased, so these addresses become invalid. */
            BlockInfoTable[i].DataAddress = 0u;
            BlockInfoTable[i].Status = FEE_BLOCK_NOT_FOUND;
        }
    }

    /* Step 5: Mark old sector as full */
    oldStart = Fee_SectorInfo[oldActiveIdx].StartAddress;
    if (MemAcc_Write(oldStart + FEE_SECTOR_STATUS_OFFSET, &fullByte, 1u) != E_OK)
    {
        return E_NOT_OK;
    }
    Fee_SectorInfo[oldActiveIdx].Status = FEE_SECTOR_STATUS_FULL;

    /* Step 6: Erase old sector */
    if (MemAcc_Erase(oldStart, FEE_SECTOR_SIZE) != E_OK)
    {
        return E_NOT_OK;
    }

    /* Step 7: Update RAM for old sector */
    Fee_SectorInfo[oldActiveIdx].EraseCount += 1u;
    Fee_SectorInfo[oldActiveIdx].Status = FEE_SECTOR_STATUS_ERASED;
    Fee_SectorInfo[oldActiveIdx].SequenceNumber = 0u;
    Fee_SectorInfo[oldActiveIdx].WritePointer = oldStart + FEE_SECTOR_HEADER_SIZE;

    return E_OK;
}

/* --------------- Fee_Sector_HasSpace --------------- */
boolean Fee_Sector_HasSpace(uint16 RequiredSize)
{
    uint32 wp = Fee_SectorInfo[Fee_ActiveSectorIndex].WritePointer;
    uint32 endAddr = Fee_SectorInfo[Fee_ActiveSectorIndex].StartAddress + FEE_SECTOR_SIZE;

    if ((wp + (uint32)RequiredSize) <= endAddr)
    {
        return TRUE;
    }
    return FALSE;
}

/* --------------- Fee_Sector_GetActiveSectorIndex --------------- */
uint8 Fee_Sector_GetActiveSectorIndex(void)
{
    return Fee_ActiveSectorIndex;
}

/* --------------- Fee_Sector_GetEraseCount --------------- */
uint16 Fee_Sector_GetEraseCount(uint8 SectorIndex)
{
    if (SectorIndex >= FEE_NUMBER_OF_SECTORS)
    {
        return 0u;
    }
    return Fee_SectorInfo[SectorIndex].EraseCount;
}
