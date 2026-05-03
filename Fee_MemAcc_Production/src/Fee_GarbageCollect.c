/**
 * \file       Fee_GarbageCollect.c
 * \brief      AUTOSAR Fee Module -- Interruptible Garbage Collection
 *
 * \details    Implements multi-cycle garbage collection with immediate-job
 *             preemption support. The GC state machine performs ONE state
 *             transition per Fee_GarbageCollect_Process() call, allowing
 *             higher-priority immediate write jobs to preempt GC.
 *
 *             GC flow:
 *             1. Trigger: select source (fullest ACTIVE/FULL sector) and
 *                target (ERASED sector with lowest EraseCount for wear leveling).
 *             2. Copy: for each VALID block in source sector, read data,
 *                verify CRC, write header+data+valid marker to target sector.
 *             3. Erase: erase source sector after all blocks are copied.
 *             4. Cleanup: reset stale block entries, update SectorInfo.
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

#include "Fee_GarbageCollect.h"
#include "Fee_Internal.h"
#include "Fee_StateMachine.h"
#include "Fee_Sector.h"
#include "Fee_Crc.h"
#include "Fee_Safety.h"
#include "MemAcc.h"
#include "Dem.h"

/*============================================================================*
 *  Internal GC state enumeration
 *============================================================================*/

/** \brief GC internal state machine */
typedef enum
{
    FEE_GC_IDLE                     = 0,
    FEE_GC_SELECT_SOURCE            = 1,
    FEE_GC_COPY_READ                = 2,
    FEE_GC_COPY_READ_WAIT           = 3,
    FEE_GC_COPY_WRITE_HEADER        = 4,
    FEE_GC_COPY_WRITE_HEADER_WAIT   = 5,
    FEE_GC_COPY_WRITE_DATA          = 6,
    FEE_GC_COPY_WRITE_DATA_WAIT     = 7,
    FEE_GC_COPY_WRITE_VALID         = 8,
    FEE_GC_COPY_WRITE_VALID_WAIT    = 9,
    FEE_GC_WRITE_TARGET_HEADER       = 10,
    FEE_GC_WRITE_TARGET_HEADER_WAIT  = 11,
    FEE_GC_ERASE_SOURCE             = 12,
    FEE_GC_ERASE_WAIT               = 13,
    FEE_GC_COMPLETE_STATE           = 14
} Fee_GcInternalStateType;

/*============================================================================*
 *  Module state variables
 *============================================================================*/

/** \brief Current GC internal state */
static VAR(Fee_GcInternalStateType, FEE_VAR) Fee_GcState;

/** \brief Source sector index (to be garbage collected) */
static VAR(uint8, FEE_VAR) Fee_GcSourceSector;

/** \brief Target sector index (copy destination) */
static VAR(uint8, FEE_VAR) Fee_GcTargetSector;

/** \brief Current block index being processed in BlockInfoTable */
static VAR(uint16, FEE_VAR) Fee_GcCurrentBlock;

/** \brief Suspended flag for immediate job preemption (GC internal) */
static VAR(boolean, FEE_VAR) Fee_GcInternal_Suspended;

/** \brief Header address allocated for current block copy in target sector */
static VAR(MemAcc_AddressType, FEE_VAR) Fee_GcAllocAddress;

/*============================================================================*
 *  Static copy buffer (NOT stack allocated)
 *============================================================================*/

/** \brief Buffer for reading block data during GC copy */
static VAR(uint8, FEE_VAR) Fee_GcCopyBuffer[FEE_MAX_BLOCK_SIZE];

/*============================================================================*
 *  Code Section
 *============================================================================*/

#define FEE_START_SEC_CODE
#include "Fee_MemMap.h"

/*============================================================================*
 *  Fee_GarbageCollect_Init
 *============================================================================*/

FUNC(void, FEE_CODE) Fee_GarbageCollect_Init(void)
{
    Fee_GcState        = FEE_GC_IDLE;
    Fee_GcSourceSector = 0u;
    Fee_GcTargetSector = 0u;
    Fee_GcCurrentBlock = 0u;
    Fee_GcInternal_Suspended    = FALSE;
    Fee_GcAllocAddress = 0u;
}

/*============================================================================*
 *  Fee_GarbageCollect_Trigger
 *============================================================================*/

FUNC(Std_ReturnType, FEE_CODE) Fee_GarbageCollect_Trigger(void)
{
    VAR(uint8, AUTOMATIC)  idx;
    VAR(uint8, AUTOMATIC)  bestSource;
    VAR(uint32, AUTOMATIC) bestUsed;
    VAR(uint8, AUTOMATIC)  bestTarget;
    VAR(uint16, AUTOMATIC) lowestErase;
    VAR(uint32, AUTOMATIC) usedSpace;
    VAR(boolean, AUTOMATIC) sourceFound;
    VAR(boolean, AUTOMATIC) targetFound;

    bestSource  = 0u;
    bestUsed    = 0u;
    bestTarget  = 0u;
    lowestErase = 0xFFFFu;
    sourceFound = FALSE;
    targetFound = FALSE;

    /* Select source sector: fullest ACTIVE/FULL sector (by used space) */
    for (idx = 0u; idx < Fee_ConfigPtr->NumberOfSectors; idx++)
    {
        if ((Fee_SectorInfo[idx].Status == FEE_SECTOR_ACTIVE) ||
            (Fee_SectorInfo[idx].Status == FEE_SECTOR_FULL))
        {
            usedSpace = (uint32)(Fee_SectorInfo[idx].WritePointer -
                                 Fee_SectorInfo[idx].BaseAddress);

            if ((sourceFound == FALSE) || (usedSpace > bestUsed))
            {
                bestUsed   = usedSpace;
                bestSource = idx;
                sourceFound = TRUE;
            }
        }
    }

    if (sourceFound == FALSE)
    {
        return E_NOT_OK;
    }

    /* Select target sector: ERASED sector with lowest EraseCount */
    for (idx = 0u; idx < Fee_ConfigPtr->NumberOfSectors; idx++)
    {
        if (Fee_SectorInfo[idx].Status == FEE_SECTOR_ERASED)
        {
            if (Fee_SectorInfo[idx].EraseCount < lowestErase)
            {
                lowestErase = Fee_SectorInfo[idx].EraseCount;
                bestTarget  = idx;
                targetFound = TRUE;
            }
        }
    }

    if (targetFound == FALSE)
    {
        return E_NOT_OK;
    }

    Fee_GcSourceSector = bestSource;
    Fee_GcTargetSector = bestTarget;
    Fee_GcCurrentBlock = 0u;
    Fee_GcInternal_Suspended    = FALSE;
    Fee_GcState        = FEE_GC_SELECT_SOURCE;

    return E_OK;
}

/*============================================================================*
 *  Fee_GarbageCollect_Process
 *============================================================================*/

FUNC(Fee_GcResultType, FEE_CODE) Fee_GarbageCollect_Process(void)
{
    VAR(MemAcc_JobResultType, AUTOMATIC) jobResult;
    VAR(uint16, AUTOMATIC) dataCrc;
    VAR(uint16, AUTOMATIC) paddedLen;
    VAR(Std_ReturnType, AUTOMATIC) ret;
    P2VAR(uint8, AUTOMATIC, FEE_VAR) headerBuf;
    P2VAR(uint8, AUTOMATIC, FEE_VAR) writeBuf;

    /* If idle, GC is not active - return complete */
    if (Fee_GcState == FEE_GC_IDLE)
    {
        return FEE_GC_COMPLETE;
    }

    /* If suspended, return in-progress without advancing */
    if (Fee_GcInternal_Suspended == TRUE)
    {
        return FEE_GC_IN_PROGRESS;
    }

    switch (Fee_GcState)
    {
        /*================================================================*
         *  SELECT_SOURCE: find next VALID block in source sector
         *================================================================*/
        case FEE_GC_SELECT_SOURCE:
        {
            VAR(boolean, AUTOMATIC) found = FALSE;

            while (Fee_GcCurrentBlock < Fee_ConfigPtr->NumberOfBlocks)
            {
                if ((Fee_BlockInfoTable[Fee_GcCurrentBlock].SectorIndex ==
                     Fee_GcSourceSector) &&
                    (Fee_BlockInfoTable[Fee_GcCurrentBlock].Status ==
                     FEE_BLOCK_VALID))
                {
                    found = TRUE;
                    break;
                }
                Fee_GcCurrentBlock++;
            }

            if (found == TRUE)
            {
                Fee_GcState = FEE_GC_COPY_READ;
            }
            else
            {
                /* No more valid blocks in source -- write target sector header
                   before erasing source (ensures target is discoverable after reset) */
                Fee_GcState = FEE_GC_WRITE_TARGET_HEADER;
            }
            return FEE_GC_IN_PROGRESS;
        }

        /*================================================================*
         *  COPY_READ: read block data from source into copy buffer
         *================================================================*/
        case FEE_GC_COPY_READ:
        {
            VAR(uint16, AUTOMATIC) blockLen =
                Fee_BlockInfoTable[Fee_GcCurrentBlock].DataLength;

            ret = MemAcc_Read(
                Fee_ConfigPtr->MemAccAreaId,
                Fee_BlockInfoTable[Fee_GcCurrentBlock].DataAddress,
                Fee_GcCopyBuffer,
                (MemAcc_LengthType)blockLen);

            if (ret != E_OK)
            {
                (void)Dem_SetEventStatus(FEE_E_HARDWARE_ERROR,
                                         DEM_EVENT_STATUS_FAILED);
                Fee_GcState = FEE_GC_IDLE;
                return FEE_GC_ERROR;
            }

            Fee_GcState = FEE_GC_COPY_READ_WAIT;
            return FEE_GC_IN_PROGRESS;
        }

        /*================================================================*
         *  COPY_READ_WAIT: poll MemAcc for read completion
         *================================================================*/
        case FEE_GC_COPY_READ_WAIT:
        {
            jobResult = MemAcc_GetJobResult(Fee_ConfigPtr->MemAccAreaId);

            if (jobResult == MEMACC_JOB_PENDING)
            {
                return FEE_GC_IN_PROGRESS;
            }

            if (jobResult != MEMACC_JOB_OK)
            {
                (void)Dem_SetEventStatus(FEE_E_HARDWARE_ERROR,
                                         DEM_EVENT_STATUS_FAILED);
                Fee_GcState = FEE_GC_IDLE;
                return FEE_GC_ERROR;
            }

            /* Verify CRC */
            dataCrc = Fee_Crc_CalculateBlock(
                Fee_GcCopyBuffer,
                (uint32)Fee_BlockInfoTable[Fee_GcCurrentBlock].DataLength);

            if (dataCrc != Fee_BlockInfoTable[Fee_GcCurrentBlock].DataCrc)
            {
                /* CRC mismatch: mark block as INCONSISTENT and skip */
                Fee_BlockInfoTable[Fee_GcCurrentBlock].Status =
                    FEE_BLOCK_INCONSISTENT;
                Fee_GcCurrentBlock++;
                Fee_GcState = FEE_GC_SELECT_SOURCE;
                return FEE_GC_IN_PROGRESS;
            }

            /* CRC OK: proceed to write header */
            Fee_GcState = FEE_GC_COPY_WRITE_HEADER;
            return FEE_GC_IN_PROGRESS;
        }

        /*================================================================*
         *  COPY_WRITE_HEADER: allocate space and write header in target
         *================================================================*/
        case FEE_GC_COPY_WRITE_HEADER:
        {
            VAR(uint16, AUTOMATIC) blockLen =
                Fee_BlockInfoTable[Fee_GcCurrentBlock].DataLength;

            /* Allocate space in target sector */
            Fee_GcAllocAddress = Fee_Sector_AllocateBlock(
                (P2VAR(Fee_SectorInfoType, AUTOMATIC, FEE_VAR))&Fee_SectorInfo[Fee_GcTargetSector],
                blockLen,
                Fee_ConfigPtr->VirtualPageSize);

            if (Fee_GcAllocAddress == 0u)
            {
                /* Target sector full -- error */
                (void)Dem_SetEventStatus(FEE_E_HARDWARE_ERROR,
                                         DEM_EVENT_STATUS_FAILED);
                Fee_GcState = FEE_GC_IDLE;
                return FEE_GC_ERROR;
            }

            /* Build block header */
            headerBuf = Fee_Sector_GetHeaderBuffer();
            Fee_Sector_BuildBlockHeader(
                headerBuf,
                Fee_ConfigPtr->BlockConfigTable[Fee_GcCurrentBlock].BlockNumber,
                blockLen,
                Fee_BlockInfoTable[Fee_GcCurrentBlock].DataCrc,
                Fee_BlockInfoTable[Fee_GcCurrentBlock].SequenceCounter,
                0u);

            /* Write header to target */
            ret = MemAcc_Write(
                Fee_ConfigPtr->MemAccAreaId,
                Fee_GcAllocAddress,
                headerBuf,
                (MemAcc_LengthType)FEE_BLOCK_HEADER_SIZE);

            if (ret != E_OK)
            {
                (void)Dem_SetEventStatus(FEE_E_HARDWARE_ERROR,
                                         DEM_EVENT_STATUS_FAILED);
                Fee_GcState = FEE_GC_IDLE;
                return FEE_GC_ERROR;
            }

            Fee_GcState = FEE_GC_COPY_WRITE_HEADER_WAIT;
            return FEE_GC_IN_PROGRESS;
        }

        /*================================================================*
         *  COPY_WRITE_HEADER_WAIT: poll MemAcc for header write
         *================================================================*/
        case FEE_GC_COPY_WRITE_HEADER_WAIT:
        {
            jobResult = MemAcc_GetJobResult(Fee_ConfigPtr->MemAccAreaId);

            if (jobResult == MEMACC_JOB_PENDING)
            {
                return FEE_GC_IN_PROGRESS;
            }

            if (jobResult != MEMACC_JOB_OK)
            {
                (void)Dem_SetEventStatus(FEE_E_HARDWARE_ERROR,
                                         DEM_EVENT_STATUS_FAILED);
                Fee_GcState = FEE_GC_IDLE;
                return FEE_GC_ERROR;
            }

            Fee_GcState = FEE_GC_COPY_WRITE_DATA;
            return FEE_GC_IN_PROGRESS;
        }

        /*================================================================*
         *  COPY_WRITE_DATA: pad data and write to target
         *================================================================*/
        case FEE_GC_COPY_WRITE_DATA:
        {
            VAR(uint16, AUTOMATIC) blockLen =
                Fee_BlockInfoTable[Fee_GcCurrentBlock].DataLength;

            /* Prepare write buffer (copies data and pads) */
            paddedLen = Fee_Sector_PrepareWriteBuffer(
                Fee_GcCopyBuffer,
                blockLen,
                Fee_ConfigPtr->VirtualPageSize);

            writeBuf = Fee_Sector_GetWriteBuffer();

            /* Data address is immediately after header */
            ret = MemAcc_Write(
                Fee_ConfigPtr->MemAccAreaId,
                Fee_GcAllocAddress + (MemAcc_AddressType)FEE_BLOCK_HEADER_SIZE,
                writeBuf,
                (MemAcc_LengthType)paddedLen);

            if (ret != E_OK)
            {
                (void)Dem_SetEventStatus(FEE_E_HARDWARE_ERROR,
                                         DEM_EVENT_STATUS_FAILED);
                Fee_GcState = FEE_GC_IDLE;
                return FEE_GC_ERROR;
            }

            Fee_GcState = FEE_GC_COPY_WRITE_DATA_WAIT;
            return FEE_GC_IN_PROGRESS;
        }

        /*================================================================*
         *  COPY_WRITE_DATA_WAIT: poll MemAcc for data write
         *================================================================*/
        case FEE_GC_COPY_WRITE_DATA_WAIT:
        {
            jobResult = MemAcc_GetJobResult(Fee_ConfigPtr->MemAccAreaId);

            if (jobResult == MEMACC_JOB_PENDING)
            {
                return FEE_GC_IN_PROGRESS;
            }

            if (jobResult != MEMACC_JOB_OK)
            {
                (void)Dem_SetEventStatus(FEE_E_HARDWARE_ERROR,
                                         DEM_EVENT_STATUS_FAILED);
                Fee_GcState = FEE_GC_IDLE;
                return FEE_GC_ERROR;
            }

            Fee_GcState = FEE_GC_COPY_WRITE_VALID;
            return FEE_GC_IN_PROGRESS;
        }

        /*================================================================*
         *  COPY_WRITE_VALID: write valid marker (0x55) to header
         *================================================================*/
        case FEE_GC_COPY_WRITE_VALID:
        {
            /* Read the header page into header buffer, set valid marker,
             * write back the full page */
            headerBuf = Fee_Sector_GetHeaderBuffer();

            /* Re-build the header to get exact content */
            Fee_Sector_BuildBlockHeader(
                headerBuf,
                Fee_ConfigPtr->BlockConfigTable[Fee_GcCurrentBlock].BlockNumber,
                Fee_BlockInfoTable[Fee_GcCurrentBlock].DataLength,
                Fee_BlockInfoTable[Fee_GcCurrentBlock].DataCrc,
                Fee_BlockInfoTable[Fee_GcCurrentBlock].SequenceCounter,
                0u);

            /* Set byte 30 to valid marker */
            headerBuf[30] = FEE_MARKER_VALID;

            ret = MemAcc_Write(
                Fee_ConfigPtr->MemAccAreaId,
                Fee_GcAllocAddress,
                headerBuf,
                (MemAcc_LengthType)FEE_BLOCK_HEADER_SIZE);

            if (ret != E_OK)
            {
                (void)Dem_SetEventStatus(FEE_E_HARDWARE_ERROR,
                                         DEM_EVENT_STATUS_FAILED);
                Fee_GcState = FEE_GC_IDLE;
                return FEE_GC_ERROR;
            }

            Fee_GcState = FEE_GC_COPY_WRITE_VALID_WAIT;
            return FEE_GC_IN_PROGRESS;
        }

        /*================================================================*
         *  COPY_WRITE_VALID_WAIT: poll MemAcc for valid marker write
         *================================================================*/
        case FEE_GC_COPY_WRITE_VALID_WAIT:
        {
            jobResult = MemAcc_GetJobResult(Fee_ConfigPtr->MemAccAreaId);

            if (jobResult == MEMACC_JOB_PENDING)
            {
                return FEE_GC_IN_PROGRESS;
            }

            if (jobResult != MEMACC_JOB_OK)
            {
                (void)Dem_SetEventStatus(FEE_E_HARDWARE_ERROR,
                                         DEM_EVENT_STATUS_FAILED);
                Fee_GcState = FEE_GC_IDLE;
                return FEE_GC_ERROR;
            }

            /* Update BlockInfoTable with new addresses in target sector */
            Fee_BlockInfoTable[Fee_GcCurrentBlock].HeaderAddress =
                Fee_GcAllocAddress;
            Fee_BlockInfoTable[Fee_GcCurrentBlock].DataAddress =
                Fee_GcAllocAddress + (MemAcc_AddressType)FEE_BLOCK_HEADER_SIZE;
            Fee_BlockInfoTable[Fee_GcCurrentBlock].SectorIndex =
                Fee_GcTargetSector;

            Fee_GcCurrentBlock++;
            Fee_Safety_UpdateRamCrc();

            Fee_GcState = FEE_GC_SELECT_SOURCE;
            return FEE_GC_IN_PROGRESS;
        }

        /*================================================================*
         *  WRITE_TARGET_HEADER: commit target sector header before erase
         *  This ensures the target sector is discoverable after a reset.
         *================================================================*/
        case FEE_GC_WRITE_TARGET_HEADER:
        {
            VAR(uint32, AUTOMATIC) newSeqNum;
            headerBuf = Fee_Sector_GetHeaderBuffer();

            /* Target sector gets sequence number higher than source */
            newSeqNum = Fee_SectorInfo[Fee_GcSourceSector].SequenceNumber + 1u;

            Fee_Sector_BuildSectorHeader(
                headerBuf,
                newSeqNum,
                Fee_SectorInfo[Fee_GcTargetSector].EraseCount);

            ret = MemAcc_Write(
                Fee_ConfigPtr->MemAccAreaId,
                Fee_SectorInfo[Fee_GcTargetSector].BaseAddress,
                headerBuf,
                (MemAcc_LengthType)FEE_SECTOR_HEADER_SIZE);

            if (ret != E_OK)
            {
                (void)Dem_SetEventStatus(FEE_E_HARDWARE_ERROR,
                                         DEM_EVENT_STATUS_FAILED);
                Fee_GcState = FEE_GC_IDLE;
                return FEE_GC_ERROR;
            }

            Fee_GcState = FEE_GC_WRITE_TARGET_HEADER_WAIT;
            return FEE_GC_IN_PROGRESS;
        }

        /*================================================================*
         *  WRITE_TARGET_HEADER_WAIT: poll MemAcc for header write
         *================================================================*/
        case FEE_GC_WRITE_TARGET_HEADER_WAIT:
        {
            VAR(uint32, AUTOMATIC) newSeqNum;
            jobResult = MemAcc_GetJobResult(Fee_ConfigPtr->MemAccAreaId);

            if (jobResult == MEMACC_JOB_PENDING)
            {
                return FEE_GC_IN_PROGRESS;
            }

            if (jobResult != MEMACC_JOB_OK)
            {
                (void)Dem_SetEventStatus(FEE_E_HARDWARE_ERROR,
                                         DEM_EVENT_STATUS_FAILED);
                Fee_GcState = FEE_GC_IDLE;
                return FEE_GC_ERROR;
            }

            /* Update target sector status to ACTIVE */
            newSeqNum = Fee_SectorInfo[Fee_GcSourceSector].SequenceNumber + 1u;
            Fee_SectorInfo[Fee_GcTargetSector].Status = FEE_SECTOR_ACTIVE;
            Fee_SectorInfo[Fee_GcTargetSector].SequenceNumber = newSeqNum;

            Fee_GcState = FEE_GC_ERASE_SOURCE;
            return FEE_GC_IN_PROGRESS;
        }

        /*================================================================*
         *  ERASE_SOURCE: erase the source sector
         *================================================================*/
        case FEE_GC_ERASE_SOURCE:
        {
            ret = MemAcc_Erase(
                Fee_ConfigPtr->MemAccAreaId,
                Fee_SectorInfo[Fee_GcSourceSector].BaseAddress,
                (MemAcc_LengthType)Fee_ConfigPtr->SectorSize);

            if (ret != E_OK)
            {
                (void)Dem_SetEventStatus(FEE_E_HARDWARE_ERROR,
                                         DEM_EVENT_STATUS_FAILED);
                Fee_GcState = FEE_GC_IDLE;
                return FEE_GC_ERROR;
            }

            Fee_GcState = FEE_GC_ERASE_WAIT;
            return FEE_GC_IN_PROGRESS;
        }

        /*================================================================*
         *  ERASE_WAIT: poll MemAcc for erase completion
         *================================================================*/
        case FEE_GC_ERASE_WAIT:
        {
            VAR(uint16, AUTOMATIC) blkIdx;

            jobResult = MemAcc_GetJobResult(Fee_ConfigPtr->MemAccAreaId);

            if (jobResult == MEMACC_JOB_PENDING)
            {
                return FEE_GC_IN_PROGRESS;
            }

            if (jobResult != MEMACC_JOB_OK)
            {
                (void)Dem_SetEventStatus(FEE_E_HARDWARE_ERROR,
                                         DEM_EVENT_STATUS_FAILED);
                Fee_GcState = FEE_GC_IDLE;
                return FEE_GC_ERROR;
            }

            /* Increment source sector erase count */
            Fee_SectorInfo[Fee_GcSourceSector].EraseCount++;

            /* Reset all block entries still pointing to source sector */
            for (blkIdx = 0u; blkIdx < Fee_ConfigPtr->NumberOfBlocks; blkIdx++)
            {
                if (Fee_BlockInfoTable[blkIdx].SectorIndex ==
                    Fee_GcSourceSector)
                {
                    Fee_BlockInfoTable[blkIdx].Status       = FEE_BLOCK_NOT_FOUND;
                    Fee_BlockInfoTable[blkIdx].HeaderAddress = 0u;
                    Fee_BlockInfoTable[blkIdx].DataAddress   = 0u;
                    Fee_BlockInfoTable[blkIdx].DataCrc       = 0u;
                }
            }

            /* Update SectorInfo: status=ERASED, reset write pointer */
            Fee_SectorInfo[Fee_GcSourceSector].Status =
                FEE_SECTOR_ERASED;
            Fee_SectorInfo[Fee_GcSourceSector].WritePointer =
                Fee_SectorInfo[Fee_GcSourceSector].BaseAddress +
                (MemAcc_AddressType)FEE_SECTOR_HEADER_SIZE;
            Fee_SectorInfo[Fee_GcSourceSector].FreeSpace =
                (uint16)(Fee_ConfigPtr->SectorSize -
                         (MemAcc_AddressType)FEE_SECTOR_HEADER_SIZE);

            Fee_Safety_UpdateRamCrc();

            Fee_GcState = FEE_GC_COMPLETE_STATE;
            return FEE_GC_IN_PROGRESS;
        }

        /*================================================================*
         *  COMPLETE_STATE: GC finished
         *================================================================*/
        case FEE_GC_COMPLETE_STATE:
        {
            Fee_GcState = FEE_GC_IDLE;
            return FEE_GC_COMPLETE;
        }

        /* IDLE or unexpected state */
        case FEE_GC_IDLE:
        default:
        {
            return FEE_GC_COMPLETE;
        }
    }
}

/*============================================================================*
 *  Fee_GarbageCollect_Suspend
 *============================================================================*/

FUNC(void, FEE_CODE) Fee_GarbageCollect_Suspend(void)
{
    Fee_GcInternal_Suspended = TRUE;
}

/*============================================================================*
 *  Fee_GarbageCollect_Resume
 *============================================================================*/

FUNC(void, FEE_CODE) Fee_GarbageCollect_Resume(void)
{
    Fee_GcInternal_Suspended = FALSE;
}

/*============================================================================*
 *  Fee_GarbageCollect_IsActive
 *============================================================================*/

FUNC(boolean, FEE_CODE) Fee_GarbageCollect_IsActive(void)
{
    return (Fee_GcState != FEE_GC_IDLE) ? TRUE : FALSE;
}

#define FEE_STOP_SEC_CODE
#include "Fee_MemMap.h"
