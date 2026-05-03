/**
 * \file       Fee.c
 * \brief      AUTOSAR Fee Module -- Public API Implementation
 *
 * \details    Thin API wrappers with DET validation that delegate to the
 *             internal state machine. All functions follow AUTOSAR R24-11
 *             SWS Fee API requirements.
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

#include "Fee.h"
#include "Fee_StateMachine.h"
#include "Fee_Safety.h"
#include "Fee_Cfg.h"
#include "Fee_Version.h"
#include "Det.h"
#include "SchM_Fee.h"

/*============================================================================*
 *  Code Section
 *============================================================================*/

#define FEE_START_SEC_CODE
#include "Fee_MemMap.h"

/*============================================================================*
 *  Internal helper: find block index by block number
 *============================================================================*/

FUNC(uint16, FEE_CODE) Fee_Internal_FindBlockIndex(uint16 BlockNumber)
{
    uint16 idx;

    if (Fee_ConfigPtr == NULL_PTR)
    {
        return FEE_BLOCK_INDEX_INVALID;
    }

    for (idx = 0u; idx < Fee_ConfigPtr->NumberOfBlocks; idx++)
    {
        if (Fee_ConfigPtr->BlockConfigTable[idx].BlockNumber == BlockNumber)
        {
            return idx;
        }
    }

    return FEE_BLOCK_INDEX_INVALID;
}

/*============================================================================*
 *  Fee_Init
 *============================================================================*/

FUNC(void, FEE_CODE) Fee_Init(
    P2CONST(Fee_ConfigType, AUTOMATIC, FEE_CONST) ConfigPtr)
{
#if (FEE_DEV_ERROR_DETECT == STD_ON)
    if (ConfigPtr == NULL_PTR)
    {
        (void)Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID,
                              FEE_SID_INIT, FEE_E_PARAM_POINTER);
        return;
    }
#endif

    Fee_StateMachine_Init(ConfigPtr);

#if (FEE_SAFETY_ENABLE == STD_ON)
    Fee_Safety_Init();
#endif
}

/*============================================================================*
 *  Fee_SetMode
 *============================================================================*/

FUNC(void, FEE_CODE) Fee_SetMode(
    MemIf_ModeType Mode)
{
    /* Fee_SetMode: Mode propagation not applicable in polling mode. */
    (void)Mode;
}

/*============================================================================*
 *  Fee_Read
 *============================================================================*/

FUNC(Std_ReturnType, FEE_CODE) Fee_Read(
    uint16 BlockNumber,
    uint16 BlockOffset,
    P2VAR(uint8, AUTOMATIC, FEE_APPL_DATA) DataBufferPtr,
    uint16 Length)
{
    VAR(Fee_JobInfoType, AUTOMATIC) job;
    VAR(Std_ReturnType, AUTOMATIC) retVal;
    VAR(uint16, AUTOMATIC) blockIdx;

#if (FEE_DEV_ERROR_DETECT == STD_ON)
    /* Check if module is initialized */
    if (Fee_ModuleStatus == MEMIF_UNINIT)
    {
        (void)Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID,
                              FEE_SID_READ, FEE_E_UNINIT);
        return E_NOT_OK;
    }

    /* Validate block number */
    blockIdx = Fee_Internal_FindBlockIndex(BlockNumber);
    if (blockIdx == FEE_BLOCK_INDEX_INVALID)
    {
        (void)Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID,
                              FEE_SID_READ, FEE_E_INVALID_BLOCK_NO);
        return E_NOT_OK;
    }

    /* Validate null pointer */
    if (DataBufferPtr == NULL_PTR)
    {
        (void)Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID,
                              FEE_SID_READ, FEE_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    /* Validate length > 0 */
    if (Length == 0u)
    {
        (void)Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID,
                              FEE_SID_READ, FEE_E_INVALID_BLOCK_LEN);
        return E_NOT_OK;
    }

    /* Validate offset + length <= blockSize */
    if (((uint32)BlockOffset + (uint32)Length) >
        (uint32)Fee_ConfigPtr->BlockConfigTable[blockIdx].BlockSize)
    {
        (void)Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID,
                              FEE_SID_READ, FEE_E_INVALID_BLOCK_OFS);
        return E_NOT_OK;
    }
#else
    blockIdx = Fee_Internal_FindBlockIndex(BlockNumber);
    (void)blockIdx;
#endif

    /* Build job */
    job.Type = FEE_JOB_READ;
    job.BlockNumber = BlockNumber;
    job.BlockOffset = BlockOffset;
    job.Length = Length;
    job.ReadDataPtr = DataBufferPtr;
    job.WriteDataPtr = NULL_PTR;
    job.IsImmediate = FALSE;

    SchM_Enter_Fee_FEE_EXCLUSIVE_AREA_0();
    retVal = Fee_StateMachine_AcceptJob(&job);
    SchM_Exit_Fee_FEE_EXCLUSIVE_AREA_0();

    if (retVal != E_OK)
    {
#if (FEE_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID,
                              FEE_SID_READ, FEE_E_BUSY);
#endif
        return E_NOT_OK;
    }

    return E_OK;
}

/*============================================================================*
 *  Fee_Write
 *============================================================================*/

FUNC(Std_ReturnType, FEE_CODE) Fee_Write(
    uint16 BlockNumber,
    P2CONST(uint8, AUTOMATIC, FEE_APPL_CONST) DataBufferPtr)
{
    VAR(Fee_JobInfoType, AUTOMATIC) job;
    VAR(Std_ReturnType, AUTOMATIC) retVal;
    VAR(uint16, AUTOMATIC) blockIdx;

#if (FEE_DEV_ERROR_DETECT == STD_ON)
    if (Fee_ModuleStatus == MEMIF_UNINIT)
    {
        (void)Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID,
                              FEE_SID_WRITE, FEE_E_UNINIT);
        return E_NOT_OK;
    }

    blockIdx = Fee_Internal_FindBlockIndex(BlockNumber);
    if (blockIdx == FEE_BLOCK_INDEX_INVALID)
    {
        (void)Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID,
                              FEE_SID_WRITE, FEE_E_INVALID_BLOCK_NO);
        return E_NOT_OK;
    }

    if (DataBufferPtr == NULL_PTR)
    {
        (void)Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID,
                              FEE_SID_WRITE, FEE_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#else
    blockIdx = Fee_Internal_FindBlockIndex(BlockNumber);
#endif

    /* Build job */
    job.Type = FEE_JOB_WRITE;
    job.BlockNumber = BlockNumber;
    job.BlockOffset = 0u;
    job.Length = Fee_ConfigPtr->BlockConfigTable[blockIdx].BlockSize;
    job.ReadDataPtr = NULL_PTR;
    job.WriteDataPtr = DataBufferPtr;
    job.IsImmediate = Fee_ConfigPtr->BlockConfigTable[blockIdx].ImmediateData;

    SchM_Enter_Fee_FEE_EXCLUSIVE_AREA_0();
    retVal = Fee_StateMachine_AcceptJob(&job);
    SchM_Exit_Fee_FEE_EXCLUSIVE_AREA_0();

    if (retVal != E_OK)
    {
#if (FEE_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID,
                              FEE_SID_WRITE, FEE_E_BUSY);
#endif
        return E_NOT_OK;
    }

    return E_OK;
}

/*============================================================================*
 *  Fee_Cancel
 *============================================================================*/

FUNC(void, FEE_CODE) Fee_Cancel(void)
{
#if (FEE_DEV_ERROR_DETECT == STD_ON)
    if (Fee_ModuleStatus == MEMIF_UNINIT)
    {
        (void)Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID,
                              FEE_SID_CANCEL, FEE_E_UNINIT);
        return;
    }
#endif

    Fee_StateMachine_Cancel();
}

/*============================================================================*
 *  Fee_GetStatus
 *============================================================================*/

FUNC(MemIf_StatusType, FEE_CODE) Fee_GetStatus(void)
{
    return Fee_StateMachine_GetStatus();
}

/*============================================================================*
 *  Fee_GetJobResult
 *============================================================================*/

FUNC(MemIf_JobResultType, FEE_CODE) Fee_GetJobResult(void)
{
#if (FEE_DEV_ERROR_DETECT == STD_ON)
    if (Fee_ModuleStatus == MEMIF_UNINIT)
    {
        (void)Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID,
                              FEE_SID_GET_JOB_RESULT, FEE_E_UNINIT);
        return MEMIF_JOB_FAILED;
    }
#endif

    return Fee_StateMachine_GetJobResult();
}

/*============================================================================*
 *  Fee_InvalidateBlock
 *============================================================================*/

FUNC(Std_ReturnType, FEE_CODE) Fee_InvalidateBlock(
    uint16 BlockNumber)
{
    VAR(Fee_JobInfoType, AUTOMATIC) job;
    VAR(Std_ReturnType, AUTOMATIC) retVal;

#if (FEE_DEV_ERROR_DETECT == STD_ON)
    if (Fee_ModuleStatus == MEMIF_UNINIT)
    {
        (void)Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID,
                              FEE_SID_INVALIDATE_BLOCK, FEE_E_UNINIT);
        return E_NOT_OK;
    }

    if (Fee_Internal_FindBlockIndex(BlockNumber) == FEE_BLOCK_INDEX_INVALID)
    {
        (void)Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID,
                              FEE_SID_INVALIDATE_BLOCK, FEE_E_INVALID_BLOCK_NO);
        return E_NOT_OK;
    }
#endif

    /* Build invalidate job */
    job.Type = FEE_JOB_INVALIDATE;
    job.BlockNumber = BlockNumber;
    job.BlockOffset = 0u;
    job.Length = 0u;
    job.ReadDataPtr = NULL_PTR;
    job.WriteDataPtr = NULL_PTR;
    job.IsImmediate = FALSE;

    SchM_Enter_Fee_FEE_EXCLUSIVE_AREA_0();
    retVal = Fee_StateMachine_AcceptJob(&job);
    SchM_Exit_Fee_FEE_EXCLUSIVE_AREA_0();

    if (retVal != E_OK)
    {
#if (FEE_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID,
                              FEE_SID_INVALIDATE_BLOCK, FEE_E_BUSY);
#endif
        return E_NOT_OK;
    }

    return E_OK;
}

/*============================================================================*
 *  Fee_GetVersionInfo
 *============================================================================*/

FUNC(void, FEE_CODE) Fee_GetVersionInfo(
    P2VAR(Std_VersionInfoType, AUTOMATIC, FEE_APPL_DATA) VersionInfoPtr)
{
#if (FEE_DEV_ERROR_DETECT == STD_ON)
    if (VersionInfoPtr == NULL_PTR)
    {
        (void)Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID,
                              FEE_SID_GET_VERSION_INFO, FEE_E_PARAM_POINTER);
        return;
    }
#endif

    VersionInfoPtr->vendorID = FEE_VENDOR_ID;
    VersionInfoPtr->moduleID = FEE_MODULE_ID;
    VersionInfoPtr->sw_major_version = FEE_SW_MAJOR_VERSION;
    VersionInfoPtr->sw_minor_version = FEE_SW_MINOR_VERSION;
    VersionInfoPtr->sw_patch_version = FEE_SW_PATCH_VERSION;
}

/*============================================================================*
 *  Fee_EraseImmediateBlock
 *============================================================================*/

FUNC(Std_ReturnType, FEE_CODE) Fee_EraseImmediateBlock(
    uint16 BlockNumber)
{
    VAR(Fee_JobInfoType, AUTOMATIC) job;
    VAR(Std_ReturnType, AUTOMATIC) retVal;
    VAR(uint16, AUTOMATIC) blockIdx;

#if (FEE_DEV_ERROR_DETECT == STD_ON)
    if (Fee_ModuleStatus == MEMIF_UNINIT)
    {
        (void)Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID,
                              FEE_SID_ERASE_IMMEDIATE_BLOCK, FEE_E_UNINIT);
        return E_NOT_OK;
    }

    blockIdx = Fee_Internal_FindBlockIndex(BlockNumber);
    if (blockIdx == FEE_BLOCK_INDEX_INVALID)
    {
        (void)Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID,
                              FEE_SID_ERASE_IMMEDIATE_BLOCK, FEE_E_INVALID_BLOCK_NO);
        return E_NOT_OK;
    }

    /* Must be immediate data block */
    if (Fee_ConfigPtr->BlockConfigTable[blockIdx].ImmediateData == FALSE)
    {
        (void)Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID,
                              FEE_SID_ERASE_IMMEDIATE_BLOCK, FEE_E_INVALID_BLOCK_NO);
        return E_NOT_OK;
    }
#else
    blockIdx = Fee_Internal_FindBlockIndex(BlockNumber);
    (void)blockIdx;
#endif

    /* Build erase immediate job */
    job.Type = FEE_JOB_ERASE_IMMEDIATE;
    job.BlockNumber = BlockNumber;
    job.BlockOffset = 0u;
    job.Length = 0u;
    job.ReadDataPtr = NULL_PTR;
    job.WriteDataPtr = NULL_PTR;
    job.IsImmediate = TRUE;

    SchM_Enter_Fee_FEE_EXCLUSIVE_AREA_0();
    retVal = Fee_StateMachine_AcceptJob(&job);
    SchM_Exit_Fee_FEE_EXCLUSIVE_AREA_0();

    if (retVal != E_OK)
    {
#if (FEE_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID,
                              FEE_SID_ERASE_IMMEDIATE_BLOCK, FEE_E_BUSY);
#endif
        return E_NOT_OK;
    }

    return E_OK;
}

/*============================================================================*
 *  Fee_MainFunction
 *============================================================================*/

FUNC(void, FEE_CODE) Fee_MainFunction(void)
{
    if (Fee_ModuleStatus == MEMIF_UNINIT)
    {
#if (FEE_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportRuntimeError(FEE_MODULE_ID, FEE_INSTANCE_ID,
                                      FEE_SID_MAIN_FUNCTION, FEE_E_UNINIT);
#endif
        return;
    }

    Fee_StateMachine_Process();

#if (FEE_SAFETY_ENABLE == STD_ON)
    Fee_Safety_CyclicCheck();
#endif
}

#define FEE_STOP_SEC_CODE
#include "Fee_MemMap.h"
