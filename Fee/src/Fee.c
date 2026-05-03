#include "Fee.h"
#include "Fee_Internal.h"
#include "Det.h"
#include <string.h>

#if (FEE_DEV_ERROR_DETECT == STD_ON)
/* --------------- Helper: check if module is busy --------------- */
static boolean Fee_IsBusy(void)
{
    MemIf_StatusType status = Fee_Internal_GetStatus();
    return (boolean)((status == MEMIF_BUSY) || (status == MEMIF_BUSY_INTERNAL));
}

/* --------------- Helper: common DET validation for job-accepting APIs ---------------
 * Performs uninit, block-number, and busy checks.
 * Returns E_OK if all checks pass; outBlockIndex is set when block-number validation
 * is performed. */
static Std_ReturnType Fee_ValidateJobRequest(uint16 BlockNumber, uint8 ApiServiceId, uint16* outBlockIndex)
{
    if (Fee_Internal_GetStatus() == MEMIF_UNINIT)
    {
        Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, ApiServiceId, FEE_E_UNINIT);
        return E_NOT_OK;
    }

    *outBlockIndex = Fee_Internal_FindBlockIndex(BlockNumber);
    if (*outBlockIndex == 0xFFFFu)
    {
        Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, ApiServiceId, FEE_E_INVALID_BLOCK_NO);
        return E_NOT_OK;
    }

    if (Fee_IsBusy() != FALSE)
    {
        Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, ApiServiceId, FEE_E_BUSY);
        return E_NOT_OK;
    }

    return E_OK;
}
#endif /* FEE_DEV_ERROR_DETECT */

/* --------------- Fee_Init --------------- */
void Fee_Init(const Fee_ConfigType* ConfigPtr)
{
#if (FEE_DEV_ERROR_DETECT == STD_ON)
    if (ConfigPtr == NULL_PTR)
    {
        Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_INIT, FEE_E_PARAM_POINTER);
        return;
    }
#endif

    (void)Fee_Internal_Init(ConfigPtr);
}

/* --------------- Fee_SetMode --------------- */
void Fee_SetMode(MemIf_ModeType Mode)
{
#if (FEE_DEV_ERROR_DETECT == STD_ON)
    if (Fee_Internal_GetStatus() == MEMIF_UNINIT)
    {
        Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_SET_MODE, FEE_E_UNINIT);
        return;
    }
#endif

    (void)Mode; /* No behavioral difference per R24-11 */
}

/* --------------- Fee_Read --------------- */
Std_ReturnType Fee_Read(uint16 BlockNumber, uint16 BlockOffset, uint8* DataBufferPtr, uint16 Length)
{
    Fee_JobDescriptorType jobDesc;
    uint16 blockIndex;

#if (FEE_DEV_ERROR_DETECT == STD_ON)
    {
        const Fee_ConfigType* cfgPtr;

        if (Fee_ValidateJobRequest(BlockNumber, FEE_SID_READ, &blockIndex) != E_OK)
        {
            return E_NOT_OK;
        }

        if (DataBufferPtr == NULL_PTR)
        {
            Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_READ, FEE_E_PARAM_POINTER);
            return E_NOT_OK;
        }

        cfgPtr = Fee_Internal_GetConfigPtr();
        if (((uint32)BlockOffset + (uint32)Length) > (uint32)cfgPtr->BlockConfig[blockIndex].BlockSize)
        {
            Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_READ, FEE_E_INVALID_BLOCK_OFS);
            return E_NOT_OK;
        }
    }
#else
    blockIndex = Fee_Internal_FindBlockIndex(BlockNumber);
    if (blockIndex == 0xFFFFu)
    {
        return E_NOT_OK;
    }
#endif

    (void)memset(&jobDesc, 0, sizeof(jobDesc));
    jobDesc.JobType = FEE_JOB_READ;
    jobDesc.BlockNumber = BlockNumber;
    jobDesc.BlockOffset = BlockOffset;
    jobDesc.DataBufferPtr = DataBufferPtr;
    jobDesc.Length = Length;

    return Fee_Internal_QueueJob(&jobDesc);
}

/* --------------- Fee_Write --------------- */
Std_ReturnType Fee_Write(uint16 BlockNumber, const uint8* DataBufferPtr)
{
    Fee_JobDescriptorType jobDesc;

#if (FEE_DEV_ERROR_DETECT == STD_ON)
    {
        uint16 blockIndex;

        if (Fee_ValidateJobRequest(BlockNumber, FEE_SID_WRITE, &blockIndex) != E_OK)
        {
            return E_NOT_OK;
        }
        (void)blockIndex;

        if (DataBufferPtr == NULL_PTR)
        {
            Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_WRITE, FEE_E_PARAM_POINTER);
            return E_NOT_OK;
        }
    }
#endif

    (void)memset(&jobDesc, 0, sizeof(jobDesc));
    jobDesc.JobType = FEE_JOB_WRITE;
    jobDesc.BlockNumber = BlockNumber;
    jobDesc.WriteDataPtr = DataBufferPtr;

    return Fee_Internal_QueueJob(&jobDesc);
}

/* --------------- Fee_Cancel --------------- */
void Fee_Cancel(void)
{
#if (FEE_DEV_ERROR_DETECT == STD_ON)
    if (Fee_Internal_GetStatus() == MEMIF_UNINIT)
    {
        Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_CANCEL, FEE_E_UNINIT);
        return;
    }
#endif

    if (Fee_Internal_CancelJob() != E_OK)
    {
#if (FEE_DEV_ERROR_DETECT == STD_ON)
        Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_CANCEL, FEE_E_INVALID_CANCEL);
#endif
    }
}

/* --------------- Fee_GetStatus --------------- */
MemIf_StatusType Fee_GetStatus(void)
{
    return Fee_Internal_GetStatus();
}

/* --------------- Fee_GetJobResult --------------- */
MemIf_JobResultType Fee_GetJobResult(void)
{
#if (FEE_DEV_ERROR_DETECT == STD_ON)
    if (Fee_Internal_GetStatus() == MEMIF_UNINIT)
    {
        Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_GET_JOB_RESULT, FEE_E_UNINIT);
        return MEMIF_JOB_FAILED;
    }
#endif

    return Fee_Internal_GetJobResult();
}

/* --------------- Fee_InvalidateBlock --------------- */
Std_ReturnType Fee_InvalidateBlock(uint16 BlockNumber)
{
    Fee_JobDescriptorType jobDesc;

#if (FEE_DEV_ERROR_DETECT == STD_ON)
    {
        uint16 blockIndex;

        if (Fee_ValidateJobRequest(BlockNumber, FEE_SID_INVALIDATE_BLOCK, &blockIndex) != E_OK)
        {
            return E_NOT_OK;
        }
        (void)blockIndex;
    }
#endif

    (void)memset(&jobDesc, 0, sizeof(jobDesc));
    jobDesc.JobType = FEE_JOB_INVALIDATE;
    jobDesc.BlockNumber = BlockNumber;

    return Fee_Internal_QueueJob(&jobDesc);
}

/* --------------- Fee_GetVersionInfo --------------- */
void Fee_GetVersionInfo(Std_VersionInfoType* VersionInfoPtr)
{
#if (FEE_DEV_ERROR_DETECT == STD_ON)
    if (VersionInfoPtr == NULL_PTR)
    {
        Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_GET_VERSION_INFO, FEE_E_PARAM_POINTER);
        return;
    }
#endif

    VersionInfoPtr->vendorID = 0u;
    VersionInfoPtr->moduleID = FEE_MODULE_ID;
    VersionInfoPtr->sw_major_version = FEE_SW_MAJOR_VERSION;
    VersionInfoPtr->sw_minor_version = FEE_SW_MINOR_VERSION;
    VersionInfoPtr->sw_patch_version = FEE_SW_PATCH_VERSION;
}

/* --------------- Fee_EraseImmediateBlock --------------- */
Std_ReturnType Fee_EraseImmediateBlock(uint16 BlockNumber)
{
    Fee_JobDescriptorType jobDesc;
    uint16 blockIndex;

#if (FEE_DEV_ERROR_DETECT == STD_ON)
    {
        const Fee_ConfigType* cfgPtr;

        if (Fee_ValidateJobRequest(BlockNumber, FEE_SID_ERASE_IMMEDIATE_BLOCK, &blockIndex) != E_OK)
        {
            return E_NOT_OK;
        }

        cfgPtr = Fee_Internal_GetConfigPtr();
        if (cfgPtr->BlockConfig[blockIndex].ImmediateData != TRUE)
        {
            Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_ERASE_IMMEDIATE_BLOCK, FEE_E_INVALID_BLOCK_NO);
            return E_NOT_OK;
        }
    }
#else
    blockIndex = Fee_Internal_FindBlockIndex(BlockNumber);
    if (blockIndex == 0xFFFFu)
    {
        return E_NOT_OK;
    }
#endif

    (void)memset(&jobDesc, 0, sizeof(jobDesc));
    jobDesc.JobType = FEE_JOB_ERASE_IMMEDIATE;
    jobDesc.BlockNumber = BlockNumber;

    return Fee_Internal_QueueJob(&jobDesc);
}

/* --------------- Fee_MainFunction --------------- */
void Fee_MainFunction(void)
{
    if (Fee_Internal_GetStatus() == MEMIF_UNINIT)
    {
        return;
    }

    Fee_Internal_ProcessJob();
    Fee_Internal_CheckAndTriggerGC();
}
