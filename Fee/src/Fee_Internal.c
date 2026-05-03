#include "Fee_Internal.h"
#include "Fee_Sector.h"
#include "MemAcc.h"
#include <string.h>

/* --------------- Static data --------------- */
static MemIf_StatusType Fee_ModuleStatus = MEMIF_UNINIT;
static MemIf_JobResultType Fee_LastJobResult = MEMIF_JOB_OK;
static Fee_JobDescriptorType Fee_PendingJob;
static boolean Fee_JobPending = FALSE;
static Fee_BlockInfoType Fee_BlockInfoTable[FEE_NUMBER_OF_BLOCKS];
static const Fee_ConfigType* Fee_InternalConfigPtr = NULL_PTR;
static boolean Fee_GcPending = FALSE;

/* --------------- Fee_Internal_Init --------------- */
Std_ReturnType Fee_Internal_Init(const Fee_ConfigType* ConfigPtr)
{
    uint16 i;

    if (ConfigPtr == NULL_PTR)
    {
        return E_NOT_OK;
    }

    /* Step 1: Store config pointer, set status to busy */
    Fee_InternalConfigPtr = ConfigPtr;
    Fee_ModuleStatus = MEMIF_BUSY_INTERNAL;

    /* Step 2: Initialize MemAcc */
    if (MemAcc_Init() != E_OK)
    {
        Fee_ModuleStatus = MEMIF_UNINIT;
        return E_NOT_OK;
    }

    /* Step 3: Initialize sector layer */
    if (Fee_Sector_Init(ConfigPtr) != E_OK)
    {
        Fee_ModuleStatus = MEMIF_UNINIT;
        return E_NOT_OK;
    }

    /* Step 4: Initialize block info table */
    for (i = 0u; i < FEE_NUMBER_OF_BLOCKS; i++)
    {
        Fee_BlockInfoTable[i].Status = FEE_BLOCK_NOT_FOUND;
        Fee_BlockInfoTable[i].DataAddress = 0u;
        /* Step 5: Set Immediate flag from config */
        Fee_BlockInfoTable[i].Immediate = ConfigPtr->BlockConfig[i].ImmediateData;
    }

    /* Step 6: Scan existing blocks from flash */
    (void)Fee_Sector_ScanBlocks(Fee_BlockInfoTable, ConfigPtr->NumberOfBlocks, ConfigPtr->BlockConfig);

    /* Re-apply Immediate flags (scan may reset them for NOT_FOUND blocks) */
    for (i = 0u; i < FEE_NUMBER_OF_BLOCKS; i++)
    {
        Fee_BlockInfoTable[i].Immediate = ConfigPtr->BlockConfig[i].ImmediateData;
    }

    /* Step 7: Set module to idle state */
    Fee_ModuleStatus = MEMIF_IDLE;
    Fee_LastJobResult = MEMIF_JOB_OK;
    Fee_JobPending = FALSE;
    Fee_GcPending = FALSE;

    /* Step 8 */
    return E_OK;
}

/* --------------- Fee_Internal_QueueJob --------------- */
Std_ReturnType Fee_Internal_QueueJob(const Fee_JobDescriptorType* JobDesc)
{
    if (JobDesc == NULL_PTR)
    {
        return E_NOT_OK;
    }

    /* Reject if a job is already pending */
    if (Fee_JobPending != FALSE)
    {
        return E_NOT_OK;
    }

    /* Copy job descriptor and mark as pending */
    Fee_PendingJob = *JobDesc;
    Fee_JobPending = TRUE;
    Fee_ModuleStatus = MEMIF_BUSY;
    Fee_LastJobResult = MEMIF_JOB_PENDING;

    return E_OK;
}

/* --------------- Fee_Internal_ProcessJob --------------- */
void Fee_Internal_ProcessJob(void)
{
    uint16 blockIndex;
    Fee_BlockInfoType* blockInfo;
    Std_ReturnType result;

    /* Step 1: Handle GC if pending */
    if (Fee_GcPending != FALSE)
    {
        Fee_ModuleStatus = MEMIF_BUSY_INTERNAL;
        result = Fee_Sector_GarbageCollect(Fee_BlockInfoTable,
                                           Fee_InternalConfigPtr->NumberOfBlocks,
                                           Fee_InternalConfigPtr->BlockConfig);
        Fee_GcPending = FALSE;

        if (result != E_OK)
        {
            /* GC failed */
            if (Fee_JobPending != FALSE)
            {
                Fee_LastJobResult = MEMIF_JOB_FAILED;
                Fee_JobPending = FALSE;
                NvM_JobErrorNotification();
            }
            Fee_ModuleStatus = MEMIF_IDLE;
            return;
        }

        /* GC succeeded; if no user job pending, go idle */
        if (Fee_JobPending == FALSE)
        {
            Fee_ModuleStatus = MEMIF_IDLE;
        }
        /* GC takes priority this cycle; user job processed next cycle */
        return;
    }

    /* Step 2: If no job pending, nothing to do */
    if (Fee_JobPending == FALSE)
    {
        return;
    }

    /* Step 3: Find block index */
    blockIndex = Fee_Internal_FindBlockIndex(Fee_PendingJob.BlockNumber);

    if (blockIndex >= FEE_NUMBER_OF_BLOCKS)
    {
        /* Block not found in configuration */
        Fee_LastJobResult = MEMIF_BLOCK_INVALID;
        NvM_JobErrorNotification();
        Fee_JobPending = FALSE;
        Fee_ModuleStatus = MEMIF_IDLE;
        return;
    }

    blockInfo = &Fee_BlockInfoTable[blockIndex];

    /* Step 4: Switch on job type */
    switch (Fee_PendingJob.JobType)
    {
        case FEE_JOB_READ:
        {
            if ((blockInfo->Status == FEE_BLOCK_INVALID) ||
                (blockInfo->Status == FEE_BLOCK_NOT_FOUND))
            {
                Fee_LastJobResult = MEMIF_BLOCK_INVALID;
                NvM_JobErrorNotification();
            }
            else if (blockInfo->Status == FEE_BLOCK_INCONSISTENT)
            {
                Fee_LastJobResult = MEMIF_BLOCK_INCONSISTENT;
                NvM_JobErrorNotification();
            }
            else if (blockInfo->Status == FEE_BLOCK_VALID)
            {
                result = Fee_Sector_ReadBlock(blockInfo,
                                              Fee_PendingJob.BlockOffset,
                                              Fee_PendingJob.DataBufferPtr,
                                              Fee_PendingJob.Length);
                if (result == E_OK)
                {
                    Fee_LastJobResult = MEMIF_JOB_OK;
                    NvM_JobEndNotification();
                }
                else
                {
                    Fee_LastJobResult = MEMIF_JOB_FAILED;
                    NvM_JobErrorNotification();
                }
            }
            else
            {
                /* Unexpected status */
                Fee_LastJobResult = MEMIF_JOB_FAILED;
                NvM_JobErrorNotification();
            }
            break;
        }

        case FEE_JOB_WRITE:
        {
            uint16 blockSize = Fee_InternalConfigPtr->BlockConfig[blockIndex].BlockSize;
            uint16 requiredSpace = (uint16)(((uint32)FEE_BLOCK_HEADER_SIZE + (uint32)blockSize +
                                   FEE_VIRTUAL_PAGE_SIZE - 1u) / FEE_VIRTUAL_PAGE_SIZE *
                                   FEE_VIRTUAL_PAGE_SIZE);

            if (Fee_Sector_HasSpace(requiredSpace) == FALSE)
            {
                /* Not enough space -- trigger GC and defer */
                Fee_GcPending = TRUE;
                return;
            }

            result = Fee_Sector_WriteBlock(Fee_PendingJob.BlockNumber,
                                           Fee_PendingJob.WriteDataPtr,
                                           blockSize,
                                           blockInfo);
            if (result == E_OK)
            {
                Fee_LastJobResult = MEMIF_JOB_OK;
                NvM_JobEndNotification();
            }
            else
            {
                Fee_LastJobResult = MEMIF_JOB_FAILED;
                NvM_JobErrorNotification();
            }
            break;
        }

        case FEE_JOB_INVALIDATE:
        {
            result = Fee_Sector_InvalidateBlock(Fee_PendingJob.BlockNumber, blockInfo);
            if (result == E_OK)
            {
                Fee_LastJobResult = MEMIF_JOB_OK;
                NvM_JobEndNotification();
            }
            else
            {
                Fee_LastJobResult = MEMIF_JOB_FAILED;
                NvM_JobErrorNotification();
            }
            break;
        }

        case FEE_JOB_ERASE_IMMEDIATE:
        {
            result = Fee_Sector_EraseImmediate(Fee_PendingJob.BlockNumber,
                                               blockInfo,
                                               Fee_BlockInfoTable,
                                               Fee_InternalConfigPtr->NumberOfBlocks,
                                               Fee_InternalConfigPtr->BlockConfig);
            if (result == E_OK)
            {
                Fee_LastJobResult = MEMIF_JOB_OK;
                NvM_JobEndNotification();
            }
            else
            {
                Fee_LastJobResult = MEMIF_JOB_FAILED;
                NvM_JobErrorNotification();
            }
            break;
        }

        default:
        {
            Fee_LastJobResult = MEMIF_JOB_FAILED;
            NvM_JobErrorNotification();
            break;
        }
    }

    /* Step 5: Clear pending job and go idle */
    Fee_JobPending = FALSE;
    Fee_ModuleStatus = MEMIF_IDLE;
}

/* --------------- Fee_Internal_CancelJob --------------- */
Std_ReturnType Fee_Internal_CancelJob(void)
{
    if (Fee_JobPending == FALSE)
    {
        return E_NOT_OK;
    }

    Fee_JobPending = FALSE;
    Fee_LastJobResult = MEMIF_JOB_CANCELLED;
    Fee_ModuleStatus = MEMIF_IDLE;

    return E_OK;
}

/* --------------- Fee_Internal_GetStatus --------------- */
MemIf_StatusType Fee_Internal_GetStatus(void)
{
    return Fee_ModuleStatus;
}

/* --------------- Fee_Internal_GetJobResult --------------- */
MemIf_JobResultType Fee_Internal_GetJobResult(void)
{
    return Fee_LastJobResult;
}

/* --------------- Fee_Internal_GetBlockInfo --------------- */
const Fee_BlockInfoType* Fee_Internal_GetBlockInfo(uint16 BlockIndex)
{
    if (BlockIndex >= FEE_NUMBER_OF_BLOCKS)
    {
        return NULL_PTR;
    }

    return &Fee_BlockInfoTable[BlockIndex];
}

/* --------------- Fee_Internal_FindBlockIndex --------------- */
uint16 Fee_Internal_FindBlockIndex(uint16 BlockNumber)
{
    uint16 i;

    if (Fee_InternalConfigPtr == NULL_PTR)
    {
        return 0xFFFFu;
    }

    for (i = 0u; i < Fee_InternalConfigPtr->NumberOfBlocks; i++)
    {
        if (Fee_InternalConfigPtr->BlockConfig[i].BlockNumber == BlockNumber)
        {
            return i;
        }
    }

    return 0xFFFFu;
}

/* --------------- Fee_Internal_CheckAndTriggerGC --------------- */
void Fee_Internal_CheckAndTriggerGC(void)
{
    uint16 i;
    uint16 maxBlockSize = 0u;
    uint16 threshold;

    /* If GC is already pending, nothing to do */
    if (Fee_GcPending != FALSE)
    {
        return;
    }

    if (Fee_InternalConfigPtr == NULL_PTR)
    {
        return;
    }

    /* Find largest configured block size */
    for (i = 0u; i < Fee_InternalConfigPtr->NumberOfBlocks; i++)
    {
        if (Fee_InternalConfigPtr->BlockConfig[i].BlockSize > maxBlockSize)
        {
            maxBlockSize = Fee_InternalConfigPtr->BlockConfig[i].BlockSize;
        }
    }

    /* Compute threshold: aligned header + largest block */
    threshold = (uint16)(((uint32)FEE_BLOCK_HEADER_SIZE + (uint32)maxBlockSize +
                 FEE_VIRTUAL_PAGE_SIZE - 1u) / FEE_VIRTUAL_PAGE_SIZE *
                 FEE_VIRTUAL_PAGE_SIZE);

    /* Check if active sector has enough space */
    if (Fee_Sector_HasSpace(threshold) == FALSE)
    {
        Fee_GcPending = TRUE;
    }
}
