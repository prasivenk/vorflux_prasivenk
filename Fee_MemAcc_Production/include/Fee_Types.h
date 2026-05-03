/**
 * \file       Fee_Types.h
 * \brief      AUTOSAR Fee Module -- Type Definitions
 *
 * \details    Provides all type definitions for the Flash EEPROM Emulation
 *             module per AUTOSAR R24-11 SWS Fee.
 *
 * \copyright  Copyright (c) 2024 -- All rights reserved.
 *             AUTOSAR R24-11 compliant.
 *
 * \autosar    AUTOSAR_SWS_FlashEEPROMEmulation
 * \version    2.0.0
 */

#ifndef FEE_TYPES_H
#define FEE_TYPES_H

/*============================================================================*
 *  Includes
 *============================================================================*/

#include "Std_Types.h"
#include "MemIf_Types.h"
#include "MemAcc_Types.h"

/*============================================================================*
 *  Block status type
 *============================================================================*/

/** \brief Block status enumeration */
typedef enum
{
    FEE_BLOCK_NOT_FOUND    = 0,  /**< Block not found in flash */
    FEE_BLOCK_VALID        = 1,  /**< Block data is valid */
    FEE_BLOCK_INVALID      = 2,  /**< Block has been invalidated */
    FEE_BLOCK_INCONSISTENT = 3   /**< Block data is inconsistent */
} Fee_BlockStatusType;

/*============================================================================*
 *  Sector status type
 *============================================================================*/

/** \brief Sector status enumeration */
typedef enum
{
    FEE_SECTOR_ERASED    = 0,  /**< Sector is erased / blank */
    FEE_SECTOR_ACTIVE    = 1,  /**< Sector is active for writes */
    FEE_SECTOR_FULL      = 2,  /**< Sector is full */
    FEE_SECTOR_DEFECTIVE = 3   /**< Sector is defective */
} Fee_SectorStatusType;

/*============================================================================*
 *  Job type
 *============================================================================*/

/** \brief Fee job type enumeration */
typedef enum
{
    FEE_JOB_NONE             = 0,  /**< No job pending */
    FEE_JOB_READ             = 1,  /**< Read job */
    FEE_JOB_WRITE            = 2,  /**< Write job */
    FEE_JOB_INVALIDATE       = 3,  /**< Invalidate block job */
    FEE_JOB_ERASE_IMMEDIATE  = 4   /**< Erase immediate block job */
} Fee_JobType;

/*============================================================================*
 *  Job info type
 *============================================================================*/

/** \brief Fee job information structure */
typedef struct
{
    Fee_JobType Type;                                              /**< Current job type */
    uint16 BlockNumber;                                            /**< Block number */
    uint16 BlockOffset;                                            /**< Offset within block */
    uint16 Length;                                                  /**< Data length */
    P2VAR(uint8, AUTOMATIC, FEE_APPL_DATA) ReadDataPtr;           /**< Read destination */
    P2CONST(uint8, AUTOMATIC, FEE_APPL_CONST) WriteDataPtr;       /**< Write source */
    boolean IsImmediate;                                           /**< Immediate data flag */
} Fee_JobInfoType;

/*============================================================================*
 *  Block info type
 *============================================================================*/

/** \brief Fee block runtime information */
typedef struct
{
    Fee_BlockStatusType Status;           /**< Block status */
    MemAcc_AddressType  HeaderAddress;    /**< Address of block header in flash */
    MemAcc_AddressType  DataAddress;      /**< Address of block data in flash */
    uint16              DataLength;       /**< Data length */
    uint16              DataCrc;          /**< CRC-16 of block data */
    uint16              SequenceCounter;  /**< Write sequence counter */
    uint8               SectorIndex;      /**< Sector containing the block */
    boolean             Immediate;        /**< Immediate data flag */
} Fee_BlockInfoType;

/*============================================================================*
 *  Sector info type
 *============================================================================*/

/** \brief Fee sector runtime information */
typedef struct
{
    Fee_SectorStatusType Status;          /**< Sector status */
    MemAcc_AddressType   BaseAddress;     /**< Sector base address */
    MemAcc_AddressType   WritePointer;    /**< Next free write address */
    uint32               SequenceNumber;  /**< Sector sequence number */
    uint16               EraseCount;      /**< Erase cycle count */
    uint16               FreeSpace;       /**< Free space remaining in bytes */
} Fee_SectorInfoType;

/*============================================================================*
 *  Block configuration type
 *============================================================================*/

/** \brief Fee block configuration (per-block, compile-time) */
typedef struct
{
    uint16  BlockNumber;        /**< Block number */
    uint16  BlockSize;          /**< Block data size in bytes */
    boolean ImmediateData;      /**< TRUE if immediate data block */
    uint8   NumberOfWriteCycles; /**< Max write cycles (0=unlimited) */
} Fee_BlockConfigType;

/*============================================================================*
 *  Module configuration type
 *============================================================================*/

/** \brief Fee module configuration */
typedef struct
{
    P2CONST(Fee_BlockConfigType, AUTOMATIC, FEE_CONST) BlockConfigTable;  /**< Block config array */
    uint16                     NumberOfBlocks;    /**< Number of configured blocks */
    uint16                     VirtualPageSize;   /**< Virtual page size in bytes */
    MemAcc_AddressAreaIdType   MemAccAreaId;      /**< MemAcc address area ID */
    uint16                     NumberOfSectors;   /**< Number of sectors */
    MemAcc_AddressType         SectorSize;        /**< Sector size in bytes */
} Fee_ConfigType;

/*============================================================================*
 *  DET error codes
 *============================================================================*/

#define FEE_E_UNINIT            0x01u  /**< API called before init */
#define FEE_E_INVALID_BLOCK_NO  0x02u  /**< Invalid block number */
#define FEE_E_INVALID_BLOCK_OFS 0x03u  /**< Invalid block offset */
#define FEE_E_PARAM_POINTER     0x04u  /**< NULL pointer parameter */
#define FEE_E_INVALID_BLOCK_LEN 0x05u  /**< Invalid block length */
#define FEE_E_BUSY              0x06u  /**< Module is busy */
#define FEE_E_INVALID_CANCEL    0x07u  /**< Invalid cancel request */

/*============================================================================*
 *  Runtime error codes
 *============================================================================*/

#define FEE_E_RAM_INTEGRITY     0x10u  /**< RAM integrity check failed */

/*============================================================================*
 *  DEM event IDs
 *============================================================================*/

#define FEE_E_HARDWARE_ERROR    ((Dem_EventIdType)0x0001u)

/*============================================================================*
 *  API service IDs
 *============================================================================*/

#define FEE_SID_INIT                  0x00u
#define FEE_SID_SET_MODE              0x01u
#define FEE_SID_READ                  0x02u
#define FEE_SID_WRITE                 0x03u
#define FEE_SID_CANCEL                0x04u
#define FEE_SID_GET_STATUS            0x05u
#define FEE_SID_GET_JOB_RESULT        0x06u
#define FEE_SID_INVALIDATE_BLOCK      0x07u
#define FEE_SID_GET_VERSION_INFO      0x08u
#define FEE_SID_ERASE_IMMEDIATE_BLOCK 0x09u
#define FEE_SID_MAIN_FUNCTION         0x12u

/*============================================================================*
 *  Module constants
 *============================================================================*/

#define FEE_MODULE_ID           21u

#define FEE_BLOCK_HEADER_SIZE   32u      /**< Block header size in bytes */
#define FEE_SECTOR_HEADER_SIZE  32u      /**< Sector header size in bytes */

#define FEE_MARKER_ERASED       0x00u    /**< TC3xx erased state */
#define FEE_MARKER_VALID        0x55u    /**< Valid marker */
#define FEE_MARKER_INVALID      0xFFu    /**< Invalid marker */

#define FEE_SECTOR_MAGIC        0xFEE0FEE0u  /**< Sector header magic word */

#define FEE_BLOCK_INDEX_INVALID 0xFFFFu  /**< Invalid block index sentinel */

#define FEE_CRC_POLYNOMIAL      0x1021u  /**< CRC-16 CCITT polynomial */
#define FEE_CRC_INITIAL         0xFFFFu  /**< CRC-16 CCITT initial value */

#endif /* FEE_TYPES_H */
