#ifndef FEE_TYPES_H
#define FEE_TYPES_H
#include "Std_Types.h"
#include "MemIf_Types.h"

#define FEE_MODULE_ID       21u
#define FEE_INSTANCE_ID     0u

/* API Service IDs */
#define FEE_SID_INIT                    0x00u
#define FEE_SID_SET_MODE                0x01u
#define FEE_SID_READ                    0x02u
#define FEE_SID_WRITE                   0x03u
#define FEE_SID_CANCEL                  0x04u
#define FEE_SID_GET_STATUS              0x05u
#define FEE_SID_GET_JOB_RESULT          0x06u
#define FEE_SID_INVALIDATE_BLOCK        0x07u
#define FEE_SID_GET_VERSION_INFO        0x08u
#define FEE_SID_ERASE_IMMEDIATE_BLOCK   0x09u
#define FEE_SID_MAIN_FUNCTION           0x12u

/* DET Error Codes [SWS_Fee_00016] */
#define FEE_E_UNINIT            0x01u
#define FEE_E_INVALID_BLOCK_NO  0x02u
#define FEE_E_INVALID_BLOCK_OFS 0x03u
#define FEE_E_PARAM_POINTER     0x04u
#define FEE_E_INVALID_BLOCK_LEN 0x05u
#define FEE_E_BUSY              0x06u
#define FEE_E_INVALID_CANCEL    0x08u

typedef enum { FEE_JOB_NONE=0, FEE_JOB_READ, FEE_JOB_WRITE, FEE_JOB_INVALIDATE, FEE_JOB_ERASE_IMMEDIATE } Fee_JobType;
typedef enum { FEE_BLOCK_VALID=0, FEE_BLOCK_INVALID, FEE_BLOCK_INCONSISTENT, FEE_BLOCK_NOT_FOUND } Fee_BlockStatusType;

/*
 * Block header layout in flash (12 bytes):
 *   Bytes 0-1:  BlockNumber (uint16, LE)
 *   Bytes 2-3:  BlockLength (uint16, LE) -- always equals configured block size
 *   Bytes 4-5:  Crc (uint16, LE) -- CRC-16 CCITT over BlockNumber + BlockLength + data
 *   Bytes 6-10: Reserved (5 bytes, written as 0x00)
 *   Byte 11:    ValidMarker -- 0xFF=pending/erased, 0xAA=valid, 0x00=invalid
 *
 * DataAddress is NOT stored in flash. It is derived as headerAddress + FEE_BLOCK_HEADER_SIZE.
 *
 * Two-phase commit sequence:
 *   Phase 1: Write header (bytes 0-10) with ValidMarker=0xFF, then write data after header.
 *   Phase 2: Write ValidMarker=0xAA (0xFF->0xAA is valid flash bit-clearing).
 *   Invalidation: Write ValidMarker=0x00 (0xAA->0x00 is valid flash bit-clearing).
 *
 * Scanner ignores entries with ValidMarker=0xFF (incomplete writes).
 */
#define FEE_BLOCK_HEADER_SIZE       12u
#define FEE_BLOCK_VALID_MARKER_OFFSET 11u

#define FEE_VALID_MARKER_ERASED     0xFFu  /* Pending / not yet committed */
#define FEE_VALID_MARKER_VALID      0xAAu  /* Committed valid entry */
#define FEE_VALID_MARKER_INVALID    0x00u  /* Invalidated entry */

/*
 * Sector header layout in flash (12 bytes):
 *   Bytes 0-3:  Magic (uint32, LE) -- FEE_SECTOR_MAGIC
 *   Bytes 4-7:  SequenceNumber (uint32, LE) -- monotonically increasing, for GC ordering
 *   Bytes 8-9:  EraseCount (uint16, LE) -- cumulative erase cycles for wear leveling
 *   Byte 10:    Status -- 0xAA=active/receiving, 0x00=full/obsolete, 0xFF=erased
 *   Byte 11:    Reserved (alignment)
 */
#define FEE_SECTOR_HEADER_SIZE      12u

#define FEE_SECTOR_MAGIC            0xFEE0FEE0u
#define FEE_SECTOR_STATUS_ACTIVE    0xAAu  /* 0xFF->0xAA: valid flash write */
#define FEE_SECTOR_STATUS_FULL      0x00u  /* 0xAA->0x00: valid flash write */
#define FEE_SECTOR_STATUS_ERASED    0xFFu  /* Default erased state */

/* Block Status Table entry (RAM) */
typedef struct {
    uint32              DataAddress;    /* Flash address of latest valid data (derived from header pos) */
    Fee_BlockStatusType Status;
    boolean             Immediate;
} Fee_BlockInfoType;

/* Sector runtime info (RAM) */
typedef struct {
    uint32 StartAddress;
    uint32 WritePointer;    /* Next free write position */
    uint32 SequenceNumber;  /* Monotonic sequence for ordering */
    uint16 EraseCount;      /* Cumulative erase cycles */
    uint8  Status;          /* Current sector status */
} Fee_SectorInfoType;

/* Pending job descriptor */
typedef struct {
    Fee_JobType     JobType;
    uint16          BlockNumber;
    uint16          BlockOffset;    /* Used by READ only */
    uint8*          DataBufferPtr;  /* Used by READ only */
    const uint8*    WriteDataPtr;   /* Used by WRITE only */
    uint16          Length;         /* Used by READ only; WRITE always uses configured block size */
} Fee_JobDescriptorType;

typedef struct Fee_BlockConfigType Fee_BlockConfigType;
typedef struct Fee_ConfigType Fee_ConfigType;

#endif /* FEE_TYPES_H */
