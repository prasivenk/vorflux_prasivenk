/**
 * \file       Fee_Sector.h
 * \brief      AUTOSAR Fee Module -- Sector Management Internal Interface
 *
 * \details    Provides sector and block header parsing/building functions,
 *             block allocation, and write buffer management for the Fee module.
 *             TC3xx target: 0x00 = erased state, 32-byte pages.
 *
 * \copyright  Copyright (c) 2024 -- All rights reserved.
 *             AUTOSAR R24-11 compliant.
 *
 * \autosar    AUTOSAR_SWS_FlashEEPROMEmulation
 * \version    2.0.0
 */

#ifndef FEE_SECTOR_H
#define FEE_SECTOR_H

/*============================================================================*
 *  Includes
 *============================================================================*/

#include "Fee_Types.h"
#include "Fee_Cfg.h"

/*============================================================================*
 *  Function declarations
 *============================================================================*/

/**
 * \brief  Parse a 32-byte raw sector header into a SectorInfo structure
 *
 * \param[in]  RawHeader   Pointer to 32-byte raw header buffer
 * \param[out] SectorInfo  Pointer to sector info structure to fill
 *
 * \return  E_OK if header is valid, E_NOT_OK if corrupt or blank
 */
extern FUNC(Std_ReturnType, FEE_CODE) Fee_Sector_ParseSectorHeader(
    P2CONST(uint8, AUTOMATIC, FEE_CONST) RawHeader,
    P2VAR(Fee_SectorInfoType, AUTOMATIC, FEE_VAR) SectorInfo
);

/**
 * \brief  Build a 32-byte sector header
 *
 * \param[out] HeaderBuf       Pointer to 32-byte output buffer
 * \param[in]  SequenceNumber  Sector sequence number
 * \param[in]  EraseCount      Erase cycle count
 */
extern FUNC(void, FEE_CODE) Fee_Sector_BuildSectorHeader(
    P2VAR(uint8, AUTOMATIC, FEE_VAR) HeaderBuf,
    uint32 SequenceNumber,
    uint16 EraseCount
);

/**
 * \brief  Parse a 32-byte raw block header
 *
 * \param[in]  RawHeader    Pointer to 32-byte raw header buffer
 * \param[out] BlockNumber  Parsed block number
 * \param[out] BlockLength  Parsed block data length
 * \param[out] DataCrc      Parsed data CRC
 * \param[out] SeqCounter   Parsed sequence counter
 * \param[out] ValidMarker  Parsed valid marker byte
 *
 * \return  E_OK if HeaderCRC is valid, E_NOT_OK if corrupt
 */
extern FUNC(Std_ReturnType, FEE_CODE) Fee_Sector_ParseBlockHeader(
    P2CONST(uint8, AUTOMATIC, FEE_CONST) RawHeader,
    P2VAR(uint16, AUTOMATIC, FEE_VAR) BlockNumber,
    P2VAR(uint16, AUTOMATIC, FEE_VAR) BlockLength,
    P2VAR(uint16, AUTOMATIC, FEE_VAR) DataCrc,
    P2VAR(uint16, AUTOMATIC, FEE_VAR) SeqCounter,
    P2VAR(uint8, AUTOMATIC, FEE_VAR) ValidMarker
);

/**
 * \brief  Build a 32-byte block header
 *
 * \param[out] HeaderBuf   Pointer to 32-byte output buffer
 * \param[in]  BlockNumber Block number
 * \param[in]  BlockLength Block data length
 * \param[in]  DataCrc     CRC-16 of block data
 * \param[in]  SeqCounter  Sequence counter
 * \param[in]  WriteCount  Write counter
 */
extern FUNC(void, FEE_CODE) Fee_Sector_BuildBlockHeader(
    P2VAR(uint8, AUTOMATIC, FEE_VAR) HeaderBuf,
    uint16 BlockNumber,
    uint16 BlockLength,
    uint16 DataCrc,
    uint16 SeqCounter,
    uint8 WriteCount
);

/**
 * \brief  Allocate space for a block in a sector
 *
 * \param[in,out] SectorInfo      Sector information (WritePointer and FreeSpace updated)
 * \param[in]     BlockSize       Block data size in bytes
 * \param[in]     VirtualPageSize Virtual page size for alignment
 *
 * \return  Start address of allocated space, or 0 if sector is full
 */
extern FUNC(MemAcc_AddressType, FEE_CODE) Fee_Sector_AllocateBlock(
    P2VAR(Fee_SectorInfoType, AUTOMATIC, FEE_VAR) SectorInfo,
    uint16 BlockSize,
    uint16 VirtualPageSize
);

/**
 * \brief  Prepare the write buffer with source data and padding
 *
 * \param[in] SrcData         Pointer to source data
 * \param[in] SrcLength       Source data length in bytes
 * \param[in] VirtualPageSize Virtual page size for alignment
 *
 * \return  Padded length (aligned up to VirtualPageSize)
 */
extern FUNC(uint16, FEE_CODE) Fee_Sector_PrepareWriteBuffer(
    P2CONST(uint8, AUTOMATIC, FEE_APPL_CONST) SrcData,
    uint16 SrcLength,
    uint16 VirtualPageSize
);

/**
 * \brief  Get fill percentage of a sector
 *
 * \param[in] SectorInfo  Pointer to sector info
 *
 * \return  Fill percentage (0-100)
 */
extern FUNC(uint8, FEE_CODE) Fee_Sector_GetFillPercentage(
    P2CONST(Fee_SectorInfoType, AUTOMATIC, FEE_CONST) SectorInfo
);

/**
 * \brief  Get pointer to the static header buffer
 *
 * \return  Pointer to Fee_Sector_HeaderBuffer[32]
 */
extern FUNC(P2VAR(uint8, AUTOMATIC, FEE_VAR), FEE_CODE) Fee_Sector_GetHeaderBuffer(void);

/**
 * \brief  Get pointer to the static write buffer
 *
 * \return  Pointer to Fee_Sector_WriteBuffer[]
 */
extern FUNC(P2VAR(uint8, AUTOMATIC, FEE_VAR), FEE_CODE) Fee_Sector_GetWriteBuffer(void);

#endif /* FEE_SECTOR_H */
