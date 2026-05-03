/**
 * \file       Fee_Sector.c
 * \brief      AUTOSAR Fee Module -- Sector Management Implementation
 *
 * \details    Sector header read/write, block header parsing/building,
 *             block allocation, and write buffer management for TC3xx
 *             (0x00 erased state, 32-byte virtual pages).
 *
 *             Block header layout (32 bytes):
 *               bytes 0-1:   BlockNumber (LE16)
 *               bytes 2-3:   BlockLength (LE16)
 *               bytes 4-5:   DataCRC (LE16)
 *               bytes 6-7:   SequenceCounter (LE16)
 *               bytes 8-27:  Reserved (0x00)
 *               byte 28:     WriteCounter (uint8)
 *               byte 29:     HeaderCRC (XOR of bytes 0-28, EXCLUDES 30-31)
 *               byte 30:     ValidMarker (0x00=erased, 0x55=valid, 0xFF=invalid)
 *               byte 31:     Reserved (0x00)
 *
 *             Sector header layout (32 bytes):
 *               bytes 0-3:   Magic (LE32, 0xFEE0FEE0)
 *               bytes 4-7:   SequenceNumber (LE32)
 *               bytes 8-9:   EraseCount (LE16)
 *               byte 10:     Status (0x00=erased, 0x55=active, 0xFF=full)
 *               bytes 11-31: Reserved (0x00)
 *
 * \copyright  Copyright (c) 2024 -- All rights reserved.
 *             AUTOSAR R24-11 compliant.
 *
 * \autosar    AUTOSAR_SWS_FlashEEPROMEmulation
 * \version    2.0.0
 */

/*============================================================================*
 *  Includes
 *============================================================================*/

#include "Fee_Sector.h"
#include "Fee_Crc.h"

/*============================================================================*
 *  Alignment macro
 *============================================================================*/

/** \brief Align value up to the next multiple of alignment */
#define FEE_ALIGN_UP(value, alignment)  \
    (((uint32)(value) + ((uint32)(alignment) - 1u)) & ~((uint32)(alignment) - 1u))

/*============================================================================*
 *  Static module-scope buffers
 *============================================================================*/

/** \brief Header buffer for staging header reads/writes */
static VAR(uint8, FEE_VAR) Fee_Sector_HeaderBuffer[FEE_BLOCK_HEADER_SIZE];

/** \brief Write buffer for staging padded block writes (header + data) */
static VAR(uint8, FEE_VAR) Fee_Sector_WriteBuffer[FEE_MAX_BLOCK_SIZE + FEE_BLOCK_HEADER_SIZE];

/*============================================================================*
 *  Code Section
 *============================================================================*/

#define FEE_START_SEC_CODE
#include "Fee_MemMap.h"

/*============================================================================*
 *  Static helpers -- Little-Endian byte ordering
 *============================================================================*/

/**
 * \brief  Write a 16-bit value in little-endian format
 */
static FUNC(void, FEE_CODE) Fee_Sector_WriteLE16(
    P2VAR(uint8, AUTOMATIC, FEE_VAR) buf,
    uint16 value)
{
    buf[0] = (uint8)(value & 0xFFu);
    buf[1] = (uint8)((value >> 8u) & 0xFFu);
}

/**
 * \brief  Write a 32-bit value in little-endian format
 */
static FUNC(void, FEE_CODE) Fee_Sector_WriteLE32(
    P2VAR(uint8, AUTOMATIC, FEE_VAR) buf,
    uint32 value)
{
    buf[0] = (uint8)(value & 0xFFu);
    buf[1] = (uint8)((value >> 8u) & 0xFFu);
    buf[2] = (uint8)((value >> 16u) & 0xFFu);
    buf[3] = (uint8)((value >> 24u) & 0xFFu);
}

/**
 * \brief  Read a 16-bit value from little-endian format
 */
static FUNC(uint16, FEE_CODE) Fee_Sector_ReadLE16(
    P2CONST(uint8, AUTOMATIC, FEE_CONST) buf)
{
    return (uint16)((uint16)buf[0] | ((uint16)buf[1] << 8u));
}

/**
 * \brief  Read a 32-bit value from little-endian format
 */
static FUNC(uint32, FEE_CODE) Fee_Sector_ReadLE32(
    P2CONST(uint8, AUTOMATIC, FEE_CONST) buf)
{
    return (uint32)((uint32)buf[0]
                  | ((uint32)buf[1] << 8u)
                  | ((uint32)buf[2] << 16u)
                  | ((uint32)buf[3] << 24u));
}

/*============================================================================*
 *  Sector header operations
 *============================================================================*/

/**
 * \brief  Parse a 32-byte raw sector header into a SectorInfo structure
 *
 * Checks magic word (0xFEE0FEE0). If magic does not match, returns E_NOT_OK.
 * A blank (all 0x00) sector will fail the magic check.
 */
FUNC(Std_ReturnType, FEE_CODE) Fee_Sector_ParseSectorHeader(
    P2CONST(uint8, AUTOMATIC, FEE_CONST) RawHeader,
    P2VAR(Fee_SectorInfoType, AUTOMATIC, FEE_VAR) SectorInfo)
{
    VAR(uint32, AUTOMATIC) magic;

    magic = Fee_Sector_ReadLE32(&RawHeader[0]);

    if (magic != FEE_SECTOR_MAGIC)
    {
        return E_NOT_OK;
    }

    SectorInfo->SequenceNumber = Fee_Sector_ReadLE32(&RawHeader[4]);
    SectorInfo->EraseCount     = Fee_Sector_ReadLE16(&RawHeader[8]);
    SectorInfo->Status         = (Fee_SectorStatusType)RawHeader[10];

    return E_OK;
}

/**
 * \brief  Build a 32-byte sector header buffer
 *
 * Status is set to 0x55 (active). Reserved bytes are 0x00.
 */
FUNC(void, FEE_CODE) Fee_Sector_BuildSectorHeader(
    P2VAR(uint8, AUTOMATIC, FEE_VAR) HeaderBuf,
    uint32 SequenceNumber,
    uint16 EraseCount)
{
    VAR(uint8, AUTOMATIC) idx;

    /* Clear entire header */
    for (idx = 0u; idx < FEE_SECTOR_HEADER_SIZE; idx++)
    {
        HeaderBuf[idx] = 0x00u;
    }

    /* Magic */
    Fee_Sector_WriteLE32(&HeaderBuf[0], FEE_SECTOR_MAGIC);

    /* Sequence number */
    Fee_Sector_WriteLE32(&HeaderBuf[4], SequenceNumber);

    /* Erase count */
    Fee_Sector_WriteLE16(&HeaderBuf[8], EraseCount);

    /* Status: active */
    HeaderBuf[10] = FEE_MARKER_VALID;
}

/*============================================================================*
 *  Block header operations
 *============================================================================*/

/**
 * \brief  Parse a 32-byte raw block header
 *
 * Validates HeaderCRC (XOR of bytes 0-28) against byte 29.
 * Note: byte 30 (ValidMarker) is intentionally EXCLUDED from the CRC
 * so that marker transitions (0x00->0x55->0xFF) do not invalidate
 * the header CRC.
 */
FUNC(Std_ReturnType, FEE_CODE) Fee_Sector_ParseBlockHeader(
    P2CONST(uint8, AUTOMATIC, FEE_CONST) RawHeader,
    P2VAR(uint16, AUTOMATIC, FEE_VAR) BlockNumber,
    P2VAR(uint16, AUTOMATIC, FEE_VAR) BlockLength,
    P2VAR(uint16, AUTOMATIC, FEE_VAR) DataCrc,
    P2VAR(uint16, AUTOMATIC, FEE_VAR) SeqCounter,
    P2VAR(uint8, AUTOMATIC, FEE_VAR) ValidMarker)
{
    VAR(uint8, AUTOMATIC) computedCrc = 0x00u;
    VAR(uint8, AUTOMATIC) idx;

    /* Compute XOR of bytes 0-28 */
    for (idx = 0u; idx < 29u; idx++)
    {
        computedCrc ^= RawHeader[idx];
    }

    /* Compare with stored HeaderCRC at byte 29 */
    if (computedCrc != RawHeader[29])
    {
        return E_NOT_OK;
    }

    /* Extract fields */
    *BlockNumber = Fee_Sector_ReadLE16(&RawHeader[0]);
    *BlockLength = Fee_Sector_ReadLE16(&RawHeader[2]);
    *DataCrc     = Fee_Sector_ReadLE16(&RawHeader[4]);
    *SeqCounter  = Fee_Sector_ReadLE16(&RawHeader[6]);
    *ValidMarker = RawHeader[30];

    return E_OK;
}

/**
 * \brief  Build a 32-byte block header
 *
 * ValidMarker is left as 0x00 (erased, phase 1 of two-phase write).
 * HeaderCRC is XOR of bytes 0-28 (excludes bytes 30-31).
 */
FUNC(void, FEE_CODE) Fee_Sector_BuildBlockHeader(
    P2VAR(uint8, AUTOMATIC, FEE_VAR) HeaderBuf,
    uint16 BlockNumber,
    uint16 BlockLength,
    uint16 DataCrc,
    uint16 SeqCounter,
    uint8 WriteCount)
{
    VAR(uint8, AUTOMATIC) xorCrc = 0x00u;
    VAR(uint8, AUTOMATIC) idx;

    /* Clear entire header (sets reserved bytes and ValidMarker to 0x00) */
    for (idx = 0u; idx < FEE_BLOCK_HEADER_SIZE; idx++)
    {
        HeaderBuf[idx] = 0x00u;
    }

    /* BlockNumber at bytes 0-1 */
    Fee_Sector_WriteLE16(&HeaderBuf[0], BlockNumber);

    /* BlockLength at bytes 2-3 */
    Fee_Sector_WriteLE16(&HeaderBuf[2], BlockLength);

    /* DataCRC at bytes 4-5 */
    Fee_Sector_WriteLE16(&HeaderBuf[4], DataCrc);

    /* SequenceCounter at bytes 6-7 */
    Fee_Sector_WriteLE16(&HeaderBuf[6], SeqCounter);

    /* Reserved bytes 8-27 already 0x00 from memset above */

    /* WriteCounter at byte 28 */
    HeaderBuf[28] = WriteCount;

    /* Compute HeaderCRC: XOR of bytes 0-28 */
    for (idx = 0u; idx < 29u; idx++)
    {
        xorCrc ^= HeaderBuf[idx];
    }
    HeaderBuf[29] = xorCrc;

    /* Byte 30: ValidMarker = 0x00 (erased, phase 1) */
    /* Byte 31: Reserved = 0x00 */
    /* Both already 0x00 from initial clear */
}

/*============================================================================*
 *  Allocation and buffer management
 *============================================================================*/

/**
 * \brief  Allocate space for a block in a sector
 *
 * Total size = FEE_BLOCK_HEADER_SIZE + ALIGN_UP(BlockSize, VirtualPageSize).
 * If free space is insufficient, returns 0 (invalid address).
 * Otherwise, returns current WritePointer and advances it.
 */
FUNC(MemAcc_AddressType, FEE_CODE) Fee_Sector_AllocateBlock(
    P2VAR(Fee_SectorInfoType, AUTOMATIC, FEE_VAR) SectorInfo,
    uint16 BlockSize,
    uint16 VirtualPageSize)
{
    VAR(uint32, AUTOMATIC)  alignedDataSize;
    VAR(uint32, AUTOMATIC)  totalSize;
    VAR(MemAcc_AddressType, AUTOMATIC) allocAddress;

    alignedDataSize = FEE_ALIGN_UP(BlockSize, VirtualPageSize);
    totalSize = (uint32)FEE_BLOCK_HEADER_SIZE + alignedDataSize;

    if ((uint32)SectorInfo->FreeSpace < totalSize)
    {
        return 0u;
    }

    allocAddress = SectorInfo->WritePointer;

    SectorInfo->WritePointer += (MemAcc_AddressType)totalSize;
    SectorInfo->FreeSpace    -= (uint16)totalSize;

    return allocAddress;
}

/**
 * \brief  Prepare the static write buffer with source data and padding
 *
 * Copies SrcLength bytes from SrcData into Fee_Sector_WriteBuffer.
 * Pads remaining bytes up to ALIGN_UP(SrcLength, VirtualPageSize) with
 * 0x00 (TC3xx erased value). This prevents out-of-bounds reads from
 * caller buffers for blocks smaller than VirtualPageSize.
 *
 * \return  Padded length (aligned up to VirtualPageSize)
 */
FUNC(uint16, FEE_CODE) Fee_Sector_PrepareWriteBuffer(
    P2CONST(uint8, AUTOMATIC, FEE_APPL_CONST) SrcData,
    uint16 SrcLength,
    uint16 VirtualPageSize)
{
    VAR(uint16, AUTOMATIC) paddedLength;
    VAR(uint16, AUTOMATIC) idx;

    paddedLength = (uint16)FEE_ALIGN_UP(SrcLength, VirtualPageSize);

    /* Copy source data */
    for (idx = 0u; idx < SrcLength; idx++)
    {
        Fee_Sector_WriteBuffer[idx] = SrcData[idx];
    }

    /* Pad remaining with 0x00 (TC3xx erased value) */
    for (idx = SrcLength; idx < paddedLength; idx++)
    {
        Fee_Sector_WriteBuffer[idx] = 0x00u;
    }

    return paddedLength;
}

/**
 * \brief  Get fill percentage of a sector
 *
 * Calculates: (usedSpace * 100) / totalSpace
 * where totalSpace = usedSpace + FreeSpace
 * and usedSpace = WritePointer - BaseAddress - FEE_SECTOR_HEADER_SIZE
 */
FUNC(uint8, FEE_CODE) Fee_Sector_GetFillPercentage(
    P2CONST(Fee_SectorInfoType, AUTOMATIC, FEE_CONST) SectorInfo)
{
    VAR(uint32, AUTOMATIC) usedSpace;
    VAR(uint32, AUTOMATIC) totalSpace;

    usedSpace  = (uint32)(SectorInfo->WritePointer - SectorInfo->BaseAddress)
                 - (uint32)FEE_SECTOR_HEADER_SIZE;
    totalSpace = usedSpace + (uint32)SectorInfo->FreeSpace;

    if (totalSpace == 0u)
    {
        return 0u;
    }

    return (uint8)((usedSpace * 100u) / totalSpace);
}

/**
 * \brief  Get pointer to the static header buffer
 */
FUNC(P2VAR(uint8, AUTOMATIC, FEE_VAR), FEE_CODE) Fee_Sector_GetHeaderBuffer(void)
{
    return Fee_Sector_HeaderBuffer;
}

/**
 * \brief  Get pointer to the static write buffer
 */
FUNC(P2VAR(uint8, AUTOMATIC, FEE_VAR), FEE_CODE) Fee_Sector_GetWriteBuffer(void)
{
    return Fee_Sector_WriteBuffer;
}

#define FEE_STOP_SEC_CODE
#include "Fee_MemMap.h"
