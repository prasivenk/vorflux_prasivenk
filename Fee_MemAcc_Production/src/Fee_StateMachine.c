/**
 * \file       Fee_StateMachine.c
 * \brief      AUTOSAR Fee Module -- Async State Machine Implementation
 *
 * \details    Core asynchronous state machine for the Fee module. Handles
 *             initialization (per-sector header reads, per-record scanning),
 *             read, write, invalidate, erase-immediate, GC, and error states.
 *             All operations use MemAcc for async flash I/O.
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

#include "Fee_StateMachine.h"
#include "Fee_Internal.h"
#include "Fee_Sector.h"
#include "Fee_Crc.h"
#include "Fee_Safety.h"
#include "Fee_GarbageCollect.h"
#include "MemAcc.h"
#include "Det.h"
#include "Dem.h"
#include "Fee_Cfg.h"

/*============================================================================*
 *  Module state variables (volatile, in VAR_CLEARED sections)
 *============================================================================*/

#define FEE_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "Fee_MemMap.h"

/** \brief Current internal state */
volatile VAR(Fee_InternalStateType, FEE_VAR) Fee_InternalState = FEE_STATE_UNINIT;

/** \brief Module status visible to upper layers */
volatile VAR(MemIf_StatusType, FEE_VAR) Fee_ModuleStatus = MEMIF_UNINIT;

/** \brief Result of last completed job */
volatile VAR(MemIf_JobResultType, FEE_VAR) Fee_LastJobResult = MEMIF_JOB_OK;

/** \brief Block info table (runtime state for each configured block) */
volatile VAR(Fee_BlockInfoType, FEE_VAR) Fee_BlockInfoTable[FEE_NUMBER_OF_BLOCKS];

/** \brief Sector info array (runtime state for each sector) */
volatile VAR(Fee_SectorInfoType, FEE_VAR) Fee_SectorInfo[FEE_NUMBER_OF_SECTORS];

/** \brief Current job being processed */
volatile VAR(Fee_JobInfoType, FEE_VAR) Fee_CurrentJob;

/** \brief Per-block sequence counters */
volatile VAR(uint16, FEE_VAR) Fee_BlockSequenceCounters[FEE_NUMBER_OF_BLOCKS];

/** \brief Active sector index */
volatile VAR(uint8, FEE_VAR) Fee_ActiveSectorIndex = 0u;

/** \brief Sector cursor during init */
volatile VAR(uint16, FEE_VAR) Fee_InitSectorCursor = 0u;

/** \brief Record scan cursor during init */
volatile VAR(MemAcc_AddressType, FEE_VAR) Fee_ScanRecordCursor = 0u;

/** \brief Stored config pointer */
P2CONST(Fee_ConfigType, AUTOMATIC, FEE_CONST) Fee_ConfigPtr = NULL_PTR;

/** \brief Write allocation address (stored between states) */
static volatile VAR(MemAcc_AddressType, FEE_VAR) Fee_WriteAllocAddress = 0u;

/** \brief Block index for current operation */
static volatile VAR(uint16, FEE_VAR) Fee_CurrentBlockIndex = FEE_BLOCK_INDEX_INVALID;

/** \brief Data CRC computed during write */
static volatile VAR(uint16, FEE_VAR) Fee_WriteDataCrc = 0u;

/** \brief Padded data length for write */
static volatile VAR(uint16, FEE_VAR) Fee_WritePaddedLength = 0u;

/** \brief GC was suspended for immediate job preemption in state machine */
static volatile VAR(boolean, FEE_VAR) Fee_SM_GcSuspended = FALSE;

/** \brief Write job is pending resumption after GC completes */
static volatile VAR(boolean, FEE_VAR) Fee_WriteAfterGcPending = FALSE;

/** \brief Sector scan cursor during multi-sector init scan */
static volatile VAR(uint8, FEE_VAR) Fee_InitScanSectorCursor = 0u;

#define FEE_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "Fee_MemMap.h"

/*============================================================================*
 *  Code Section
 *============================================================================*/

#define FEE_START_SEC_CODE
#include "Fee_MemMap.h"

/*============================================================================*
 *  Internal helper: check if header buffer is all blank (erased = 0x00)
 *============================================================================*/

static FUNC(boolean, FEE_CODE) Fee_Internal_IsHeaderBlank(
    P2CONST(uint8, AUTOMATIC, FEE_CONST) HeaderBuf)
{
    uint8 idx;

    for (idx = 0u; idx < FEE_BLOCK_HEADER_SIZE; idx++)
    {
        if (HeaderBuf[idx] != 0x00u)
        {
            return FALSE;
        }
    }

    return TRUE;
}

/*============================================================================*
 *  Fee_StateMachine_Init
 *============================================================================*/

FUNC(void, FEE_CODE) Fee_StateMachine_Init(
    P2CONST(Fee_ConfigType, AUTOMATIC, FEE_CONST) ConfigPtr)
{
    uint16 idx;

    Fee_ConfigPtr = ConfigPtr;
    Fee_InitSectorCursor = 0u;
    Fee_ScanRecordCursor = 0u;
    Fee_ActiveSectorIndex = 0u;
    Fee_SM_GcSuspended = FALSE;
    Fee_WriteAfterGcPending = FALSE;
    Fee_InitScanSectorCursor = 0u;
    Fee_WriteAllocAddress = 0u;
    Fee_CurrentBlockIndex = FEE_BLOCK_INDEX_INVALID;

    /* Initialize block info table */
    for (idx = 0u; idx < FEE_NUMBER_OF_BLOCKS; idx++)
    {
        Fee_BlockInfoTable[idx].Status = FEE_BLOCK_NOT_FOUND;
        Fee_BlockInfoTable[idx].HeaderAddress = 0u;
        Fee_BlockInfoTable[idx].DataAddress = 0u;
        Fee_BlockInfoTable[idx].DataLength = 0u;
        Fee_BlockInfoTable[idx].DataCrc = 0u;
        Fee_BlockInfoTable[idx].SequenceCounter = 0u;
        Fee_BlockInfoTable[idx].SectorIndex = 0u;
        Fee_BlockSequenceCounters[idx] = 0u;
    }

    /* Initialize sector info */
    for (idx = 0u; idx < FEE_NUMBER_OF_SECTORS; idx++)
    {
        Fee_SectorInfo[idx].Status = FEE_SECTOR_ERASED;
        Fee_SectorInfo[idx].BaseAddress = (MemAcc_AddressType)idx * ConfigPtr->SectorSize;
        Fee_SectorInfo[idx].WritePointer = Fee_SectorInfo[idx].BaseAddress + FEE_SECTOR_HEADER_SIZE;
        Fee_SectorInfo[idx].SequenceNumber = 0u;
        Fee_SectorInfo[idx].EraseCount = 0u;
        Fee_SectorInfo[idx].FreeSpace = (uint16)(ConfigPtr->SectorSize - FEE_SECTOR_HEADER_SIZE);
    }

    /* Clear current job */
    Fee_CurrentJob.Type = FEE_JOB_NONE;
    Fee_CurrentJob.BlockNumber = 0u;
    Fee_CurrentJob.BlockOffset = 0u;
    Fee_CurrentJob.Length = 0u;
    Fee_CurrentJob.ReadDataPtr = NULL_PTR;
    Fee_CurrentJob.WriteDataPtr = NULL_PTR;
    Fee_CurrentJob.IsImmediate = FALSE;

    Fee_LastJobResult = MEMIF_JOB_OK;
    Fee_ModuleStatus = MEMIF_BUSY_INTERNAL;
    Fee_InternalState = FEE_STATE_INIT_READ_SECTOR_HEADER;
}

/*============================================================================*
 *  Fee_StateMachine_Process -- Main processing switch
 *============================================================================*/

FUNC(void, FEE_CODE) Fee_StateMachine_Process(void)
{
    VAR(MemAcc_JobResultType, AUTOMATIC) memAccResult;
    VAR(Std_ReturnType, AUTOMATIC) parseResult;
    VAR(uint16, AUTOMATIC) blockNum;
    VAR(uint16, AUTOMATIC) blockLen;
    VAR(uint16, AUTOMATIC) dataCrc;
    VAR(uint16, AUTOMATIC) seqCounter;
    VAR(uint8, AUTOMATIC) validMarker;
    VAR(uint16, AUTOMATIC) blkIdx;
    VAR(uint8, AUTOMATIC) *headerBuf;
    VAR(uint8, AUTOMATIC) *writeBuf;
    VAR(uint32, AUTOMATIC) recordTotalSize;

    if (Fee_InternalState == FEE_STATE_UNINIT)
    {
        return;
    }

    switch (Fee_InternalState)
    {
        /*================================================================*
         *  INIT STATES: Read sector headers
         *================================================================*/
        case FEE_STATE_INIT_READ_SECTOR_HEADER:
        {
            headerBuf = Fee_Sector_GetHeaderBuffer();
            (void)MemAcc_Read(
                Fee_ConfigPtr->MemAccAreaId,
                Fee_SectorInfo[Fee_InitSectorCursor].BaseAddress,
                headerBuf,
                FEE_SECTOR_HEADER_SIZE
            );
            Fee_InternalState = FEE_STATE_INIT_WAIT_SECTOR_HEADER;
            break;
        }

        case FEE_STATE_INIT_WAIT_SECTOR_HEADER:
        {
            memAccResult = MemAcc_GetJobResult(Fee_ConfigPtr->MemAccAreaId);
            if (memAccResult == MEMACC_JOB_PENDING)
            {
                return; /* Wait for next MainFunction cycle */
            }

            if (memAccResult == MEMACC_JOB_OK)
            {
                headerBuf = Fee_Sector_GetHeaderBuffer();
                /* Non-volatile local copy for parse */
                {
                    VAR(Fee_SectorInfoType, AUTOMATIC) tmpSectorInfo;
                    parseResult = Fee_Sector_ParseSectorHeader(headerBuf, &tmpSectorInfo);
                    if (parseResult == E_OK)
                    {
                        /* Map raw status byte to enum values:
                           ParseSectorHeader casts raw byte directly, so we need
                           to map: 0x55 -> ACTIVE, 0xFF -> FULL, 0x00 -> ERASED */
                        if ((uint8)tmpSectorInfo.Status == FEE_MARKER_VALID)
                        {
                            Fee_SectorInfo[Fee_InitSectorCursor].Status = FEE_SECTOR_ACTIVE;
                        }
                        else if ((uint8)tmpSectorInfo.Status == FEE_MARKER_INVALID)
                        {
                            Fee_SectorInfo[Fee_InitSectorCursor].Status = FEE_SECTOR_FULL;
                        }
                        else
                        {
                            Fee_SectorInfo[Fee_InitSectorCursor].Status = FEE_SECTOR_ERASED;
                        }
                        Fee_SectorInfo[Fee_InitSectorCursor].SequenceNumber = tmpSectorInfo.SequenceNumber;
                        Fee_SectorInfo[Fee_InitSectorCursor].EraseCount = tmpSectorInfo.EraseCount;
                    }
                    else
                    {
                        /* Blank or corrupt sector header -- leave as ERASED */
                        Fee_SectorInfo[Fee_InitSectorCursor].Status = FEE_SECTOR_ERASED;
                    }
                }
            }
            else
            {
                /* MemAcc failure during init -- mark sector defective */
                Fee_SectorInfo[Fee_InitSectorCursor].Status = FEE_SECTOR_DEFECTIVE;
            }

            Fee_InternalState = FEE_STATE_INIT_NEXT_SECTOR;
            break;
        }

        case FEE_STATE_INIT_NEXT_SECTOR:
        {
            Fee_InitSectorCursor++;

            if (Fee_InitSectorCursor < Fee_ConfigPtr->NumberOfSectors)
            {
                /* Read next sector header */
                Fee_InternalState = FEE_STATE_INIT_READ_SECTOR_HEADER;
            }
            else
            {
                /* All sector headers read -- determine active sector
                   (highest sequence number among ACTIVE sectors) */
                VAR(uint32, AUTOMATIC) highestSeqNum = 0u;
                VAR(boolean, AUTOMATIC) foundActive = FALSE;
                VAR(uint16, AUTOMATIC) sIdx;

                for (sIdx = 0u; sIdx < Fee_ConfigPtr->NumberOfSectors; sIdx++)
                {
                    if (Fee_SectorInfo[sIdx].Status == FEE_SECTOR_ACTIVE)
                    {
                        if ((foundActive == FALSE) ||
                            (Fee_SectorInfo[sIdx].SequenceNumber > highestSeqNum))
                        {
                            highestSeqNum = Fee_SectorInfo[sIdx].SequenceNumber;
                            Fee_ActiveSectorIndex = (uint8)sIdx;
                            foundActive = TRUE;
                        }
                    }
                }

                if (foundActive == FALSE)
                {
                    /* No active sector found -- use first erased sector */
                    Fee_ActiveSectorIndex = 0u;
                    for (sIdx = 0u; sIdx < Fee_ConfigPtr->NumberOfSectors; sIdx++)
                    {
                        if (Fee_SectorInfo[sIdx].Status == FEE_SECTOR_ERASED)
                        {
                            Fee_ActiveSectorIndex = (uint8)sIdx;
                            break;
                        }
                    }
                }

                /* Scan ALL non-defective sectors for valid block records.
                   This handles recovery after interrupted GC where valid
                   blocks may exist in FULL or older ACTIVE sectors. */
                Fee_InitScanSectorCursor = 0u;

                /* Advance to first scannable sector */
                while ((Fee_InitScanSectorCursor < Fee_ConfigPtr->NumberOfSectors) &&
                       (Fee_SectorInfo[Fee_InitScanSectorCursor].Status == FEE_SECTOR_DEFECTIVE ||
                        Fee_SectorInfo[Fee_InitScanSectorCursor].Status == FEE_SECTOR_ERASED))
                {
                    Fee_InitScanSectorCursor++;
                }

                if (Fee_InitScanSectorCursor < Fee_ConfigPtr->NumberOfSectors)
                {
                    Fee_ScanRecordCursor =
                        Fee_SectorInfo[Fee_InitScanSectorCursor].BaseAddress +
                        FEE_SECTOR_HEADER_SIZE;
                    Fee_InternalState = FEE_STATE_INIT_SCAN_READ_RECORD;
                }
                else
                {
                    /* No scannable sectors -- init complete */
                    Fee_ModuleStatus = MEMIF_IDLE;
                    Fee_InternalState = FEE_STATE_IDLE;
#if (FEE_SAFETY_ENABLE == STD_ON)
                    Fee_Safety_UpdateRamCrc();
#endif
                }
            }
            break;
        }

        /*================================================================*
         *  INIT STATES: Scan records
         *================================================================*/
        case FEE_STATE_INIT_SCAN_READ_RECORD:
        {
            /* Check if we've reached the end of the current scan sector */
            VAR(MemAcc_AddressType, AUTOMATIC) sectorEnd;
            sectorEnd = Fee_SectorInfo[Fee_InitScanSectorCursor].BaseAddress +
                        Fee_ConfigPtr->SectorSize;

            if (Fee_ScanRecordCursor >= sectorEnd)
            {
                /* Past end of current sector -- update its write pointer */
                /* (leave write pointer at sector end) */

                /* Advance to next scannable sector (ACTIVE or FULL) */
                Fee_InitScanSectorCursor++;
                while ((Fee_InitScanSectorCursor < Fee_ConfigPtr->NumberOfSectors) &&
                       (Fee_SectorInfo[Fee_InitScanSectorCursor].Status == FEE_SECTOR_DEFECTIVE ||
                        Fee_SectorInfo[Fee_InitScanSectorCursor].Status == FEE_SECTOR_ERASED))
                {
                    Fee_InitScanSectorCursor++;
                }

                if (Fee_InitScanSectorCursor < Fee_ConfigPtr->NumberOfSectors)
                {
                    /* Start scanning next sector */
                    Fee_ScanRecordCursor =
                        Fee_SectorInfo[Fee_InitScanSectorCursor].BaseAddress +
                        FEE_SECTOR_HEADER_SIZE;
                    /* Stay in FEE_STATE_INIT_SCAN_READ_RECORD */
                }
                else
                {
                    /* All sectors scanned -- init complete */
                    Fee_ModuleStatus = MEMIF_IDLE;
                    Fee_InternalState = FEE_STATE_IDLE;
#if (FEE_SAFETY_ENABLE == STD_ON)
                    Fee_Safety_UpdateRamCrc();
#endif
                }
                break;
            }

            headerBuf = Fee_Sector_GetHeaderBuffer();
            (void)MemAcc_Read(
                Fee_ConfigPtr->MemAccAreaId,
                Fee_ScanRecordCursor,
                headerBuf,
                FEE_BLOCK_HEADER_SIZE
            );
            Fee_InternalState = FEE_STATE_INIT_SCAN_WAIT_RECORD;
            break;
        }

        case FEE_STATE_INIT_SCAN_WAIT_RECORD:
        {
            memAccResult = MemAcc_GetJobResult(Fee_ConfigPtr->MemAccAreaId);
            if (memAccResult == MEMACC_JOB_PENDING)
            {
                return; /* Wait for next cycle */
            }

            if (memAccResult != MEMACC_JOB_OK)
            {
                /* Read failed -- advance to next sector */
                Fee_InitScanSectorCursor++;
                while ((Fee_InitScanSectorCursor < Fee_ConfigPtr->NumberOfSectors) &&
                       (Fee_SectorInfo[Fee_InitScanSectorCursor].Status == FEE_SECTOR_DEFECTIVE ||
                        Fee_SectorInfo[Fee_InitScanSectorCursor].Status == FEE_SECTOR_ERASED))
                {
                    Fee_InitScanSectorCursor++;
                }
                if (Fee_InitScanSectorCursor < Fee_ConfigPtr->NumberOfSectors)
                {
                    Fee_ScanRecordCursor =
                        Fee_SectorInfo[Fee_InitScanSectorCursor].BaseAddress +
                        FEE_SECTOR_HEADER_SIZE;
                    Fee_InternalState = FEE_STATE_INIT_SCAN_READ_RECORD;
                }
                else
                {
                    Fee_ModuleStatus = MEMIF_IDLE;
                    Fee_InternalState = FEE_STATE_IDLE;
#if (FEE_SAFETY_ENABLE == STD_ON)
                    Fee_Safety_UpdateRamCrc();
#endif
                }
                break;
            }

            headerBuf = Fee_Sector_GetHeaderBuffer();

            /* Check if header is blank (all 0x00 = erased area) */
            if (Fee_Internal_IsHeaderBlank(headerBuf) == TRUE)
            {
                /* Reached blank area -- update write pointer for this sector */
                Fee_SectorInfo[Fee_InitScanSectorCursor].WritePointer =
                    Fee_ScanRecordCursor;
                Fee_SectorInfo[Fee_InitScanSectorCursor].FreeSpace =
                    (uint16)(Fee_SectorInfo[Fee_InitScanSectorCursor].BaseAddress +
                             Fee_ConfigPtr->SectorSize -
                             Fee_ScanRecordCursor);

                /* Advance to next scannable sector */
                Fee_InitScanSectorCursor++;
                while ((Fee_InitScanSectorCursor < Fee_ConfigPtr->NumberOfSectors) &&
                       (Fee_SectorInfo[Fee_InitScanSectorCursor].Status == FEE_SECTOR_DEFECTIVE ||
                        Fee_SectorInfo[Fee_InitScanSectorCursor].Status == FEE_SECTOR_ERASED))
                {
                    Fee_InitScanSectorCursor++;
                }

                if (Fee_InitScanSectorCursor < Fee_ConfigPtr->NumberOfSectors)
                {
                    Fee_ScanRecordCursor =
                        Fee_SectorInfo[Fee_InitScanSectorCursor].BaseAddress +
                        FEE_SECTOR_HEADER_SIZE;
                    Fee_InternalState = FEE_STATE_INIT_SCAN_READ_RECORD;
                }
                else
                {
                    /* All sectors scanned -- init complete */
                    Fee_ModuleStatus = MEMIF_IDLE;
                    Fee_InternalState = FEE_STATE_IDLE;
#if (FEE_SAFETY_ENABLE == STD_ON)
                    Fee_Safety_UpdateRamCrc();
#endif
                }
                break;
            }

            /* Parse the block header */
            parseResult = Fee_Sector_ParseBlockHeader(
                headerBuf, &blockNum, &blockLen, &dataCrc,
                &seqCounter, &validMarker);

            if (parseResult == E_OK)
            {
                /* Look up block index in config */
                blkIdx = Fee_Internal_FindBlockIndex(blockNum);

                if (blkIdx != FEE_BLOCK_INDEX_INVALID)
                {
                    /* Only update if this is a newer instance (higher seq counter)
                       or the block hasn't been found yet */
                    if ((Fee_BlockInfoTable[blkIdx].Status == FEE_BLOCK_NOT_FOUND) ||
                        (seqCounter >= Fee_BlockInfoTable[blkIdx].SequenceCounter))
                    {
                        Fee_BlockInfoTable[blkIdx].HeaderAddress =
                            Fee_ScanRecordCursor;
                        Fee_BlockInfoTable[blkIdx].DataAddress =
                            Fee_ScanRecordCursor + FEE_BLOCK_HEADER_SIZE;
                        Fee_BlockInfoTable[blkIdx].DataLength = blockLen;
                        Fee_BlockInfoTable[blkIdx].DataCrc = dataCrc;
                        Fee_BlockInfoTable[blkIdx].SequenceCounter = seqCounter;
                        Fee_BlockInfoTable[blkIdx].SectorIndex =
                            (uint8)Fee_InitScanSectorCursor;

                        if (validMarker == FEE_MARKER_VALID)
                        {
                            Fee_BlockInfoTable[blkIdx].Status = FEE_BLOCK_VALID;
                        }
                        else if (validMarker == FEE_MARKER_INVALID)
                        {
                            Fee_BlockInfoTable[blkIdx].Status = FEE_BLOCK_INVALID;
                        }
                        else
                        {
                            /* Erased or unknown marker -- inconsistent */
                            Fee_BlockInfoTable[blkIdx].Status = FEE_BLOCK_INCONSISTENT;
                        }

                        Fee_BlockSequenceCounters[blkIdx] = seqCounter;
                    }
                }

                /* Advance scan cursor by record size:
                   header + ALIGN_UP(blockLen, VirtualPageSize) */
                recordTotalSize = (uint32)FEE_BLOCK_HEADER_SIZE +
                    FEE_ALIGN_UP(blockLen, Fee_ConfigPtr->VirtualPageSize);
            }
            else
            {
                /* Corrupt header -- skip one header size and try next */
                recordTotalSize = FEE_BLOCK_HEADER_SIZE;
            }

            Fee_ScanRecordCursor += (MemAcc_AddressType)recordTotalSize;
            Fee_InternalState = FEE_STATE_INIT_SCAN_READ_RECORD;
            break;
        }

        /*================================================================*
         *  IDLE STATE
         *================================================================*/
        case FEE_STATE_IDLE:
        {
            if (Fee_CurrentJob.Type != FEE_JOB_NONE)
            {
                switch (Fee_CurrentJob.Type)
                {
                    case FEE_JOB_READ:
                        Fee_InternalState = FEE_STATE_READ_START;
                        break;
                    case FEE_JOB_WRITE:
                        Fee_InternalState = FEE_STATE_WRITE_ALLOC;
                        break;
                    case FEE_JOB_INVALIDATE:
                        Fee_InternalState = FEE_STATE_INVALIDATE_WRITE;
                        break;
                    case FEE_JOB_ERASE_IMMEDIATE:
                        Fee_InternalState = FEE_STATE_ERASE_IMMEDIATE;
                        break;
                    default:
                        Fee_CurrentJob.Type = FEE_JOB_NONE;
                        break;
                }
            }
            break;
        }

        /*================================================================*
         *  READ STATES
         *================================================================*/
        case FEE_STATE_READ_START:
        {
            blkIdx = Fee_Internal_FindBlockIndex(Fee_CurrentJob.BlockNumber);
            Fee_CurrentBlockIndex = blkIdx;

            if (blkIdx == FEE_BLOCK_INDEX_INVALID)
            {
                Fee_StateMachine_CompleteJob(MEMIF_BLOCK_INVALID);
                break;
            }

            if (Fee_BlockInfoTable[blkIdx].Status == FEE_BLOCK_NOT_FOUND)
            {
                Fee_StateMachine_CompleteJob(MEMIF_BLOCK_INVALID);
                break;
            }

            if (Fee_BlockInfoTable[blkIdx].Status == FEE_BLOCK_INCONSISTENT)
            {
                Fee_StateMachine_CompleteJob(MEMIF_BLOCK_INCONSISTENT);
                break;
            }

            if (Fee_BlockInfoTable[blkIdx].Status == FEE_BLOCK_INVALID)
            {
                Fee_StateMachine_CompleteJob(MEMIF_BLOCK_INVALID);
                break;
            }

            /* Valid block -- initiate read from flash */
            (void)MemAcc_Read(
                Fee_ConfigPtr->MemAccAreaId,
                Fee_BlockInfoTable[blkIdx].DataAddress +
                    (MemAcc_AddressType)Fee_CurrentJob.BlockOffset,
                Fee_CurrentJob.ReadDataPtr,
                (MemAcc_LengthType)Fee_CurrentJob.Length
            );
            Fee_InternalState = FEE_STATE_READ_WAIT;
            break;
        }

        case FEE_STATE_READ_WAIT:
        {
            memAccResult = MemAcc_GetJobResult(Fee_ConfigPtr->MemAccAreaId);
            if (memAccResult == MEMACC_JOB_PENDING)
            {
                return;
            }

            if (memAccResult == MEMACC_JOB_OK)
            {
                Fee_InternalState = FEE_STATE_READ_VERIFY_CRC;
            }
            else
            {
                (void)Dem_SetEventStatus(FEE_E_HARDWARE_ERROR,
                                         DEM_EVENT_STATUS_FAILED);
                Fee_StateMachine_CompleteJob(MEMIF_JOB_FAILED);
            }
            break;
        }

        case FEE_STATE_READ_VERIFY_CRC:
        {
            blkIdx = Fee_CurrentBlockIndex;

            /* CRC verification only for full-block reads */
            if ((Fee_CurrentJob.BlockOffset == 0u) &&
                (Fee_CurrentJob.Length == Fee_BlockInfoTable[blkIdx].DataLength))
            {
                VAR(uint16, AUTOMATIC) computedCrc;
                computedCrc = Fee_Crc_CalculateBlock(
                    Fee_CurrentJob.ReadDataPtr,
                    (uint32)Fee_CurrentJob.Length);

                if (computedCrc == Fee_BlockInfoTable[blkIdx].DataCrc)
                {
                    Fee_StateMachine_CompleteJob(MEMIF_JOB_OK);
                }
                else
                {
                    (void)Dem_SetEventStatus(FEE_E_HARDWARE_ERROR,
                                             DEM_EVENT_STATUS_FAILED);
                    Fee_StateMachine_CompleteJob(MEMIF_BLOCK_INCONSISTENT);
                }
            }
            else
            {
                /* Partial read -- skip CRC verification */
                Fee_StateMachine_CompleteJob(MEMIF_JOB_OK);
            }
            break;
        }

        /*================================================================*
         *  WRITE STATES
         *================================================================*/
        case FEE_STATE_WRITE_ALLOC:
        {
            blkIdx = Fee_Internal_FindBlockIndex(Fee_CurrentJob.BlockNumber);
            Fee_CurrentBlockIndex = blkIdx;

            if (blkIdx == FEE_BLOCK_INDEX_INVALID)
            {
                Fee_StateMachine_CompleteJob(MEMIF_JOB_FAILED);
                break;
            }

            {
                /* Make a non-volatile copy of SectorInfo for allocation */
                VAR(Fee_SectorInfoType, AUTOMATIC) tmpSector;
                tmpSector.Status = Fee_SectorInfo[Fee_ActiveSectorIndex].Status;
                tmpSector.BaseAddress = Fee_SectorInfo[Fee_ActiveSectorIndex].BaseAddress;
                tmpSector.WritePointer = Fee_SectorInfo[Fee_ActiveSectorIndex].WritePointer;
                tmpSector.SequenceNumber = Fee_SectorInfo[Fee_ActiveSectorIndex].SequenceNumber;
                tmpSector.EraseCount = Fee_SectorInfo[Fee_ActiveSectorIndex].EraseCount;
                tmpSector.FreeSpace = Fee_SectorInfo[Fee_ActiveSectorIndex].FreeSpace;

                Fee_WriteAllocAddress = Fee_Sector_AllocateBlock(
                    &tmpSector,
                    Fee_ConfigPtr->BlockConfigTable[blkIdx].BlockSize,
                    Fee_ConfigPtr->VirtualPageSize);

                if (Fee_WriteAllocAddress == 0u)
                {
                    /* Sector full -- trigger garbage collection to reclaim space */
                    if (Fee_GarbageCollect_Trigger() == E_OK)
                    {
                        /* GC triggered: save pending write, transition to GC */
                        Fee_WriteAfterGcPending = TRUE;
                        Fee_ModuleStatus = MEMIF_BUSY_INTERNAL;
                        Fee_InternalState = FEE_STATE_GC_ACTIVE;
                    }
                    else
                    {
                        /* No target sector available for GC -- fail the write */
                        Fee_StateMachine_CompleteJob(MEMIF_JOB_FAILED);
                    }
                    break;
                }

                /* Write back updated sector info */
                Fee_SectorInfo[Fee_ActiveSectorIndex].WritePointer = tmpSector.WritePointer;
                Fee_SectorInfo[Fee_ActiveSectorIndex].FreeSpace = tmpSector.FreeSpace;
            }

            Fee_InternalState = FEE_STATE_WRITE_HEADER;
            break;
        }

        case FEE_STATE_WRITE_HEADER:
        {
            blkIdx = Fee_CurrentBlockIndex;
            headerBuf = Fee_Sector_GetHeaderBuffer();

            /* Compute data CRC */
            Fee_WriteDataCrc = Fee_Crc_CalculateBlock(
                Fee_CurrentJob.WriteDataPtr,
                (uint32)Fee_ConfigPtr->BlockConfigTable[blkIdx].BlockSize);

            /* Increment sequence counter */
            Fee_BlockSequenceCounters[blkIdx]++;

            /* Build block header (ValidMarker = 0x00 = erased, phase 1) */
            Fee_Sector_BuildBlockHeader(
                headerBuf,
                Fee_CurrentJob.BlockNumber,
                Fee_ConfigPtr->BlockConfigTable[blkIdx].BlockSize,
                Fee_WriteDataCrc,
                Fee_BlockSequenceCounters[blkIdx],
                0u /* WriteCount */);

            /* Write header to flash */
            (void)MemAcc_Write(
                Fee_ConfigPtr->MemAccAreaId,
                Fee_WriteAllocAddress,
                headerBuf,
                FEE_BLOCK_HEADER_SIZE);

            Fee_InternalState = FEE_STATE_WRITE_HEADER_WAIT;
            break;
        }

        case FEE_STATE_WRITE_HEADER_WAIT:
        {
            memAccResult = MemAcc_GetJobResult(Fee_ConfigPtr->MemAccAreaId);
            if (memAccResult == MEMACC_JOB_PENDING)
            {
                return;
            }

            if (memAccResult == MEMACC_JOB_OK)
            {
                Fee_InternalState = FEE_STATE_WRITE_DATA;
            }
            else
            {
                (void)Dem_SetEventStatus(FEE_E_HARDWARE_ERROR,
                                         DEM_EVENT_STATUS_FAILED);
                Fee_StateMachine_CompleteJob(MEMIF_JOB_FAILED);
            }
            break;
        }

        case FEE_STATE_WRITE_DATA:
        {
            blkIdx = Fee_CurrentBlockIndex;

            /* Prepare write buffer with padding */
            Fee_WritePaddedLength = Fee_Sector_PrepareWriteBuffer(
                Fee_CurrentJob.WriteDataPtr,
                Fee_ConfigPtr->BlockConfigTable[blkIdx].BlockSize,
                Fee_ConfigPtr->VirtualPageSize);

            writeBuf = Fee_Sector_GetWriteBuffer();

            /* Write data after header */
            (void)MemAcc_Write(
                Fee_ConfigPtr->MemAccAreaId,
                Fee_WriteAllocAddress + FEE_BLOCK_HEADER_SIZE,
                writeBuf,
                (MemAcc_LengthType)Fee_WritePaddedLength);

            Fee_InternalState = FEE_STATE_WRITE_DATA_WAIT;
            break;
        }

        case FEE_STATE_WRITE_DATA_WAIT:
        {
            memAccResult = MemAcc_GetJobResult(Fee_ConfigPtr->MemAccAreaId);
            if (memAccResult == MEMACC_JOB_PENDING)
            {
                return;
            }

            if (memAccResult == MEMACC_JOB_OK)
            {
                Fee_InternalState = FEE_STATE_WRITE_VALID;
            }
            else
            {
                (void)Dem_SetEventStatus(FEE_E_HARDWARE_ERROR,
                                         DEM_EVENT_STATUS_FAILED);
                Fee_StateMachine_CompleteJob(MEMIF_JOB_FAILED);
            }
            break;
        }

        case FEE_STATE_WRITE_VALID:
        {
            /* Rebuild header from local data and set valid marker.
               The header content is fully known from the write parameters;
               no async read-before-modify-write is needed. */
            blkIdx = Fee_CurrentBlockIndex;
            headerBuf = Fee_Sector_GetHeaderBuffer();

            Fee_Sector_BuildBlockHeader(
                headerBuf,
                Fee_CurrentJob.BlockNumber,
                Fee_ConfigPtr->BlockConfigTable[blkIdx].BlockSize,
                Fee_WriteDataCrc,
                Fee_BlockSequenceCounters[blkIdx],
                0u);
            headerBuf[30] = FEE_MARKER_VALID;

            /* Write the full header page with valid marker set */
            (void)MemAcc_Write(
                Fee_ConfigPtr->MemAccAreaId,
                Fee_WriteAllocAddress,
                headerBuf,
                FEE_BLOCK_HEADER_SIZE);

            Fee_InternalState = FEE_STATE_WRITE_VALID_WAIT;
            break;
        }

        case FEE_STATE_WRITE_VALID_WAIT:
        {
            memAccResult = MemAcc_GetJobResult(Fee_ConfigPtr->MemAccAreaId);
            if (memAccResult == MEMACC_JOB_PENDING)
            {
                return;
            }

            if (memAccResult == MEMACC_JOB_OK)
            {
#if (FEE_READ_BACK_VERIFICATION == STD_ON)
                Fee_InternalState = FEE_STATE_WRITE_VERIFY_READ;
#else
                /* Update BlockInfoTable and complete */
                blkIdx = Fee_CurrentBlockIndex;
                Fee_BlockInfoTable[blkIdx].Status = FEE_BLOCK_VALID;
                Fee_BlockInfoTable[blkIdx].HeaderAddress = Fee_WriteAllocAddress;
                Fee_BlockInfoTable[blkIdx].DataAddress =
                    Fee_WriteAllocAddress + FEE_BLOCK_HEADER_SIZE;
                Fee_BlockInfoTable[blkIdx].DataLength =
                    Fee_ConfigPtr->BlockConfigTable[blkIdx].BlockSize;
                Fee_BlockInfoTable[blkIdx].DataCrc = Fee_WriteDataCrc;
                Fee_BlockInfoTable[blkIdx].SequenceCounter =
                    Fee_BlockSequenceCounters[blkIdx];
                Fee_BlockInfoTable[blkIdx].SectorIndex = Fee_ActiveSectorIndex;
#if (FEE_SAFETY_ENABLE == STD_ON)
                Fee_Safety_UpdateRamCrc();
#endif
                Fee_StateMachine_CompleteJob(MEMIF_JOB_OK);
#endif
            }
            else
            {
                (void)Dem_SetEventStatus(FEE_E_HARDWARE_ERROR,
                                         DEM_EVENT_STATUS_FAILED);
                Fee_StateMachine_CompleteJob(MEMIF_JOB_FAILED);
            }
            break;
        }

        case FEE_STATE_WRITE_VERIFY_READ:
        {
            blkIdx = Fee_CurrentBlockIndex;
            writeBuf = Fee_Sector_GetWriteBuffer();

            /* Read back the written data for verification */
            (void)MemAcc_Read(
                Fee_ConfigPtr->MemAccAreaId,
                Fee_WriteAllocAddress + FEE_BLOCK_HEADER_SIZE,
                writeBuf,
                (MemAcc_LengthType)Fee_WritePaddedLength);

            Fee_InternalState = FEE_STATE_WRITE_VERIFY_WAIT;
            break;
        }

        case FEE_STATE_WRITE_VERIFY_WAIT:
        {
            memAccResult = MemAcc_GetJobResult(Fee_ConfigPtr->MemAccAreaId);
            if (memAccResult == MEMACC_JOB_PENDING)
            {
                return;
            }

            if (memAccResult == MEMACC_JOB_OK)
            {
                Fee_InternalState = FEE_STATE_WRITE_VERIFY_COMPARE;
            }
            else
            {
                (void)Dem_SetEventStatus(FEE_E_HARDWARE_ERROR,
                                         DEM_EVENT_STATUS_FAILED);
                Fee_StateMachine_CompleteJob(MEMIF_JOB_FAILED);
            }
            break;
        }

        case FEE_STATE_WRITE_VERIFY_COMPARE:
        {
            blkIdx = Fee_CurrentBlockIndex;
            writeBuf = Fee_Sector_GetWriteBuffer();

            /* Compare read-back data with original */
            {
                VAR(uint16, AUTOMATIC) compIdx;
                VAR(uint16, AUTOMATIC) origLen;
                VAR(boolean, AUTOMATIC) mismatch = FALSE;

                origLen = Fee_ConfigPtr->BlockConfigTable[blkIdx].BlockSize;

                for (compIdx = 0u; compIdx < origLen; compIdx++)
                {
                    if (writeBuf[compIdx] != Fee_CurrentJob.WriteDataPtr[compIdx])
                    {
                        mismatch = TRUE;
                        break;
                    }
                }

                if (mismatch == FALSE)
                {
                    /* Verification OK -- update BlockInfoTable */
                    Fee_BlockInfoTable[blkIdx].Status = FEE_BLOCK_VALID;
                    Fee_BlockInfoTable[blkIdx].HeaderAddress = Fee_WriteAllocAddress;
                    Fee_BlockInfoTable[blkIdx].DataAddress =
                        Fee_WriteAllocAddress + FEE_BLOCK_HEADER_SIZE;
                    Fee_BlockInfoTable[blkIdx].DataLength = origLen;
                    Fee_BlockInfoTable[blkIdx].DataCrc = Fee_WriteDataCrc;
                    Fee_BlockInfoTable[blkIdx].SequenceCounter =
                        Fee_BlockSequenceCounters[blkIdx];
                    Fee_BlockInfoTable[blkIdx].SectorIndex = Fee_ActiveSectorIndex;
#if (FEE_SAFETY_ENABLE == STD_ON)
                    Fee_Safety_UpdateRamCrc();
#endif
                    Fee_StateMachine_CompleteJob(MEMIF_JOB_OK);
                }
                else
                {
                    (void)Dem_SetEventStatus(FEE_E_HARDWARE_ERROR,
                                             DEM_EVENT_STATUS_FAILED);
                    Fee_StateMachine_CompleteJob(MEMIF_JOB_FAILED);
                }
            }
            break;
        }

        /*================================================================*
         *  INVALIDATE STATES
         *================================================================*/
        case FEE_STATE_INVALIDATE_WRITE:
        {
            blkIdx = Fee_Internal_FindBlockIndex(Fee_CurrentJob.BlockNumber);
            Fee_CurrentBlockIndex = blkIdx;

            if (blkIdx == FEE_BLOCK_INDEX_INVALID)
            {
                Fee_StateMachine_CompleteJob(MEMIF_JOB_FAILED);
                break;
            }

            if (Fee_BlockInfoTable[blkIdx].Status == FEE_BLOCK_NOT_FOUND)
            {
                /* Nothing to invalidate -- succeed */
                Fee_StateMachine_CompleteJob(MEMIF_JOB_OK);
                break;
            }

            /* Rebuild the header locally from known data and set invalid marker.
               This avoids an async read-before-write race; the header content
               is already known from the block info table. */
            headerBuf = Fee_Sector_GetHeaderBuffer();
            Fee_Sector_BuildBlockHeader(
                headerBuf,
                Fee_CurrentJob.BlockNumber,
                Fee_BlockInfoTable[blkIdx].DataLength,
                Fee_BlockInfoTable[blkIdx].DataCrc,
                Fee_BlockInfoTable[blkIdx].SequenceCounter,
                0u);
            headerBuf[30] = FEE_MARKER_INVALID;

            /* Write the header with invalid marker */
            (void)MemAcc_Write(
                Fee_ConfigPtr->MemAccAreaId,
                Fee_BlockInfoTable[blkIdx].HeaderAddress,
                headerBuf,
                FEE_BLOCK_HEADER_SIZE);

            Fee_InternalState = FEE_STATE_INVALIDATE_WAIT;
            break;
        }

        case FEE_STATE_INVALIDATE_WAIT:
        {
            memAccResult = MemAcc_GetJobResult(Fee_ConfigPtr->MemAccAreaId);
            if (memAccResult == MEMACC_JOB_PENDING)
            {
                return;
            }

            blkIdx = Fee_CurrentBlockIndex;

            if (memAccResult == MEMACC_JOB_OK)
            {
                Fee_BlockInfoTable[blkIdx].Status = FEE_BLOCK_INVALID;
#if (FEE_SAFETY_ENABLE == STD_ON)
                Fee_Safety_UpdateRamCrc();
#endif
                Fee_StateMachine_CompleteJob(MEMIF_JOB_OK);
            }
            else
            {
                (void)Dem_SetEventStatus(FEE_E_HARDWARE_ERROR,
                                         DEM_EVENT_STATUS_FAILED);
                Fee_StateMachine_CompleteJob(MEMIF_JOB_FAILED);
            }
            break;
        }

        /*================================================================*
         *  ERASE IMMEDIATE STATES
         *================================================================*/
        case FEE_STATE_ERASE_IMMEDIATE:
        {
            blkIdx = Fee_Internal_FindBlockIndex(Fee_CurrentJob.BlockNumber);
            Fee_CurrentBlockIndex = blkIdx;

            if (blkIdx == FEE_BLOCK_INDEX_INVALID)
            {
                Fee_StateMachine_CompleteJob(MEMIF_JOB_FAILED);
                break;
            }

            /* For erase immediate: just ensure space is available and complete.
               If existing valid entry exists, invalidate it first. */
            if ((Fee_BlockInfoTable[blkIdx].Status == FEE_BLOCK_VALID) ||
                (Fee_BlockInfoTable[blkIdx].Status == FEE_BLOCK_INCONSISTENT))
            {
                /* Rebuild header from known data and set invalid marker.
                   Avoids async read+write race -- header content is known. */
                headerBuf = Fee_Sector_GetHeaderBuffer();
                Fee_Sector_BuildBlockHeader(
                    headerBuf,
                    Fee_CurrentJob.BlockNumber,
                    Fee_BlockInfoTable[blkIdx].DataLength,
                    Fee_BlockInfoTable[blkIdx].DataCrc,
                    Fee_BlockInfoTable[blkIdx].SequenceCounter,
                    0u);
                headerBuf[30] = FEE_MARKER_INVALID;
                (void)MemAcc_Write(
                    Fee_ConfigPtr->MemAccAreaId,
                    Fee_BlockInfoTable[blkIdx].HeaderAddress,
                    headerBuf,
                    FEE_BLOCK_HEADER_SIZE);
                Fee_InternalState = FEE_STATE_ERASE_IMMEDIATE_WAIT;
            }
            else
            {
                /* No existing entry or already invalid -- just complete */
                Fee_StateMachine_CompleteJob(MEMIF_JOB_OK);
            }
            break;
        }

        case FEE_STATE_ERASE_IMMEDIATE_WAIT:
        {
            memAccResult = MemAcc_GetJobResult(Fee_ConfigPtr->MemAccAreaId);
            if (memAccResult == MEMACC_JOB_PENDING)
            {
                return;
            }

            blkIdx = Fee_CurrentBlockIndex;

            if (memAccResult == MEMACC_JOB_OK)
            {
                Fee_BlockInfoTable[blkIdx].Status = FEE_BLOCK_NOT_FOUND;
#if (FEE_SAFETY_ENABLE == STD_ON)
                Fee_Safety_UpdateRamCrc();
#endif
                Fee_StateMachine_CompleteJob(MEMIF_JOB_OK);
            }
            else
            {
                (void)Dem_SetEventStatus(FEE_E_HARDWARE_ERROR,
                                         DEM_EVENT_STATUS_FAILED);
                Fee_StateMachine_CompleteJob(MEMIF_JOB_FAILED);
            }
            break;
        }

        /*================================================================*
         *  GC_ACTIVE STATE
         *================================================================*/
        case FEE_STATE_GC_ACTIVE:
        {
            VAR(Fee_GcResultType, AUTOMATIC) gcResult;
            gcResult = Fee_GarbageCollect_Process();

            if (gcResult == FEE_GC_COMPLETE)
            {
                if (Fee_WriteAfterGcPending == TRUE)
                {
                    /* GC completed -- retry the pending write.
                       Update active sector index to the GC target. */
                    VAR(uint16, AUTOMATIC) sIdx;
                    VAR(uint32, AUTOMATIC) highSeq = 0u;
                    VAR(boolean, AUTOMATIC) found = FALSE;

                    for (sIdx = 0u; sIdx < Fee_ConfigPtr->NumberOfSectors; sIdx++)
                    {
                        if ((Fee_SectorInfo[sIdx].Status == FEE_SECTOR_ACTIVE) &&
                            ((found == FALSE) ||
                             (Fee_SectorInfo[sIdx].SequenceNumber > highSeq)))
                        {
                            highSeq = Fee_SectorInfo[sIdx].SequenceNumber;
                            Fee_ActiveSectorIndex = (uint8)sIdx;
                            found = TRUE;
                        }
                    }

                    Fee_WriteAfterGcPending = FALSE;
                    Fee_ModuleStatus = MEMIF_BUSY;
                    Fee_InternalState = FEE_STATE_WRITE_ALLOC;
                }
                else
                {
                    Fee_ModuleStatus = MEMIF_IDLE;
                    Fee_InternalState = FEE_STATE_IDLE;
                }
            }
            else if (gcResult == FEE_GC_ERROR)
            {
                if (Fee_WriteAfterGcPending == TRUE)
                {
                    Fee_WriteAfterGcPending = FALSE;
                    Fee_StateMachine_CompleteJob(MEMIF_JOB_FAILED);
                }
                else
                {
                    Fee_InternalState = FEE_STATE_ERROR;
                }
            }
            /* else: FEE_GC_IN_PROGRESS -- stay in GC_ACTIVE */
            break;
        }

        /*================================================================*
         *  ERROR STATE
         *================================================================*/
        case FEE_STATE_ERROR:
        {
            (void)Dem_SetEventStatus(FEE_E_HARDWARE_ERROR,
                                     DEM_EVENT_STATUS_FAILED);
            Fee_ModuleStatus = MEMIF_IDLE;
            Fee_InternalState = FEE_STATE_IDLE;
            break;
        }

        /* Reserved for future data-verification during init scan */
        case FEE_STATE_INIT_SCAN_READ_DATA:
        case FEE_STATE_INIT_SCAN_WAIT_DATA:
        case FEE_STATE_INIT_SCAN_NEXT_RECORD:
        default:
            break;
    }
}

/*============================================================================*
 *  Fee_StateMachine_AcceptJob
 *============================================================================*/

FUNC(Std_ReturnType, FEE_CODE) Fee_StateMachine_AcceptJob(
    P2CONST(Fee_JobInfoType, AUTOMATIC, FEE_CONST) Job)
{
    if (Fee_ModuleStatus == MEMIF_IDLE)
    {
        /* Accept job */
        Fee_CurrentJob.Type = Job->Type;
        Fee_CurrentJob.BlockNumber = Job->BlockNumber;
        Fee_CurrentJob.BlockOffset = Job->BlockOffset;
        Fee_CurrentJob.Length = Job->Length;
        Fee_CurrentJob.ReadDataPtr = Job->ReadDataPtr;
        Fee_CurrentJob.WriteDataPtr = Job->WriteDataPtr;
        Fee_CurrentJob.IsImmediate = Job->IsImmediate;

        Fee_ModuleStatus = MEMIF_BUSY;
        Fee_LastJobResult = MEMIF_JOB_PENDING;
        return E_OK;
    }

    if (Fee_ModuleStatus == MEMIF_BUSY)
    {
        return E_NOT_OK;
    }

    if (Fee_ModuleStatus == MEMIF_BUSY_INTERNAL)
    {
        /* GC in progress -- only accept immediate jobs */
        if (Job->IsImmediate == TRUE)
        {
            Fee_CurrentJob.Type = Job->Type;
            Fee_CurrentJob.BlockNumber = Job->BlockNumber;
            Fee_CurrentJob.BlockOffset = Job->BlockOffset;
            Fee_CurrentJob.Length = Job->Length;
            Fee_CurrentJob.ReadDataPtr = Job->ReadDataPtr;
            Fee_CurrentJob.WriteDataPtr = Job->WriteDataPtr;
            Fee_CurrentJob.IsImmediate = Job->IsImmediate;

            Fee_SM_GcSuspended = TRUE;
            Fee_ModuleStatus = MEMIF_BUSY;
            Fee_LastJobResult = MEMIF_JOB_PENDING;
            Fee_InternalState = FEE_STATE_IDLE;
            return E_OK;
        }
        return E_NOT_OK;
    }

    return E_NOT_OK;
}

/*============================================================================*
 *  Fee_StateMachine_CompleteJob
 *============================================================================*/

FUNC(void, FEE_CODE) Fee_StateMachine_CompleteJob(
    MemIf_JobResultType Result)
{
    Fee_LastJobResult = Result;

    /* Clear current job */
    Fee_CurrentJob.Type = FEE_JOB_NONE;
    Fee_CurrentJob.BlockNumber = 0u;
    Fee_CurrentJob.BlockOffset = 0u;
    Fee_CurrentJob.Length = 0u;
    Fee_CurrentJob.ReadDataPtr = NULL_PTR;
    Fee_CurrentJob.WriteDataPtr = NULL_PTR;
    Fee_CurrentJob.IsImmediate = FALSE;

    /* Set module status */
    if (Fee_SM_GcSuspended == TRUE)
    {
        /* Resume GC */
        Fee_SM_GcSuspended = FALSE;
        Fee_ModuleStatus = MEMIF_BUSY_INTERNAL;
        Fee_InternalState = FEE_STATE_GC_ACTIVE;
    }
    else
    {
        Fee_ModuleStatus = MEMIF_IDLE;
        Fee_InternalState = FEE_STATE_IDLE;
    }

    /* NvM notifications */
#if (FEE_NVM_JOB_END_NOTIFICATION == STD_ON)
    if (Result == MEMIF_JOB_OK)
    {
        NvM_JobEndNotification();
    }
#endif

#if (FEE_NVM_JOB_ERROR_NOTIFICATION == STD_ON)
    if ((Result != MEMIF_JOB_OK) && (Result != MEMIF_JOB_PENDING))
    {
        NvM_JobErrorNotification();
    }
#endif
}

/*============================================================================*
 *  Fee_StateMachine_Cancel
 *============================================================================*/

FUNC(void, FEE_CODE) Fee_StateMachine_Cancel(void)
{
    /* Check if in a wait state where we can cancel */
    switch (Fee_InternalState)
    {
        case FEE_STATE_READ_WAIT:
        case FEE_STATE_WRITE_HEADER_WAIT:
        case FEE_STATE_WRITE_DATA_WAIT:
        case FEE_STATE_WRITE_VALID_WAIT:
        case FEE_STATE_WRITE_VERIFY_WAIT:
        case FEE_STATE_INVALIDATE_WAIT:
        case FEE_STATE_ERASE_IMMEDIATE_WAIT:
        case FEE_STATE_INIT_WAIT_SECTOR_HEADER:
        case FEE_STATE_INIT_SCAN_WAIT_RECORD:
        {
            MemAcc_Cancel(Fee_ConfigPtr->MemAccAreaId);
            Fee_StateMachine_CompleteJob(MEMIF_JOB_CANCELED);
            break;
        }

        default:
        {
            /* Not in a cancellable state -- still complete the cancel */
            if (Fee_CurrentJob.Type != FEE_JOB_NONE)
            {
                Fee_StateMachine_CompleteJob(MEMIF_JOB_CANCELED);
            }
            break;
        }
    }
}

/*============================================================================*
 *  Fee_StateMachine_GetStatus
 *============================================================================*/

FUNC(MemIf_StatusType, FEE_CODE) Fee_StateMachine_GetStatus(void)
{
    return Fee_ModuleStatus;
}

/*============================================================================*
 *  Fee_StateMachine_GetJobResult
 *============================================================================*/

FUNC(MemIf_JobResultType, FEE_CODE) Fee_StateMachine_GetJobResult(void)
{
    return Fee_LastJobResult;
}

#define FEE_STOP_SEC_CODE
#include "Fee_MemMap.h"
