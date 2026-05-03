/**
 * \file       MemAcc.c
 * \brief      AUTOSAR MemAcc Module -- Public API Implementation
 *
 * \details    Implements all public APIs for the Memory Access module
 *             per AUTOSAR R24-11 SWS MemAcc.
 *
 * \copyright  Copyright (c) 2024 -- All rights reserved.
 *             AUTOSAR R24-11 compliant.
 *
 * \autosar    AUTOSAR_SWS_MemoryAccess
 * \version    24.11.0
 */

/*============================================================================*
 *  Includes
 *============================================================================*/

#include "MemAcc.h"
#include "MemAcc_Internal.h"
#include "MemAcc_Cfg.h"
#include "Det.h"
#include "Dem.h"
#include "SchM_MemAcc.h"

/*============================================================================*
 *  Module state variables
 *============================================================================*/

#define MEMACC_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemAcc_MemMap.h"

/** \brief Module initialization status */
volatile VAR(MemAcc_StatusType, MEMACC_VAR) MemAcc_ModuleStatus = MEMACC_UNINIT;

/** \brief Per-area current job information */
volatile VAR(MemAcc_JobInfoType, MEMACC_VAR) MemAcc_CurrentJob[MEMACC_NUMBER_OF_ADDRESS_AREAS];

/** \brief Per-area job result */
volatile VAR(MemAcc_JobResultType, MEMACC_VAR) MemAcc_AreaJobResult[MEMACC_NUMBER_OF_ADDRESS_AREAS];

/** \brief Per-area busy flag */
volatile VAR(boolean, MEMACC_VAR) MemAcc_AreaBusy[MEMACC_NUMBER_OF_ADDRESS_AREAS];

/** \brief Per-area lock flag */
volatile VAR(boolean, MEMACC_VAR) MemAcc_AreaLocked[MEMACC_NUMBER_OF_ADDRESS_AREAS];

/** \brief Pointer to active configuration */
P2CONST(MemAcc_ConfigType, AUTOMATIC, MEMACC_CONST) MemAcc_ConfigPtr = NULL_PTR;

#define MEMACC_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemAcc_MemMap.h"

/*============================================================================*
 *  Function implementations
 *============================================================================*/

#define MEMACC_START_SEC_CODE
#include "MemAcc_MemMap.h"

/*============================================================================*
 *  Internal helpers
 *============================================================================*/

/**
 * \brief  Finish a job by setting result and clearing busy state
 */
FUNC(void, MEMACC_CODE) MemAcc_Internal_FinishJob(
    uint8 AreaIndex,
    MemAcc_JobResultType Result
)
{
    SchM_Enter_MemAcc_MEMACC_EXCLUSIVE_AREA_0();
    MemAcc_AreaJobResult[AreaIndex] = Result;
    MemAcc_AreaBusy[AreaIndex] = FALSE;
    MemAcc_CurrentJob[AreaIndex].JobType = MEMACC_JOB_NONE;
    SchM_Exit_MemAcc_MEMACC_EXCLUSIVE_AREA_0();
}

/**
 * \brief  Common job acceptance: DET checks, busy/lock, job setup, dispatch
 */
FUNC(Std_ReturnType, MEMACC_CODE) MemAcc_Internal_AcceptJob(
    MemAcc_AddressAreaIdType AreaId,
    MemAcc_JobType JobType,
    MemAcc_AddressType Address,
    P2VAR(MemAcc_DataType, AUTOMATIC, MEMACC_APPL_DATA) ReadPtr,
    P2CONST(MemAcc_DataType, AUTOMATIC, MEMACC_APPL_DATA) WritePtr,
    P2CONST(MemAcc_DataType, AUTOMATIC, MEMACC_APPL_DATA) ComparePtr,
    MemAcc_LengthType Length,
    uint8 ServiceId
)
{
    Std_ReturnType retVal = E_NOT_OK;
    uint8 areaIndex;

#if (MEMACC_DEV_ERROR_DETECT == STD_ON)
    if (MemAcc_ModuleStatus == MEMACC_UNINIT)
    {
        (void)Det_ReportError(MEMACC_MODULE_ID, MEMACC_INSTANCE_ID,
                              ServiceId, MEMACC_E_UNINIT);
        return E_NOT_OK;
    }
    if (MemAcc_Internal_FindArea(AreaId, &areaIndex) == FALSE)
    {
        (void)Det_ReportError(MEMACC_MODULE_ID, MEMACC_INSTANCE_ID,
                              ServiceId, MEMACC_E_PARAM_ADDRESS_AREA);
        return E_NOT_OK;
    }
    /* Pointer check: Read needs ReadPtr, Write needs WritePtr, Compare needs ComparePtr */
    if (((JobType == MEMACC_JOB_READ) && (ReadPtr == NULL_PTR)) ||
        ((JobType == MEMACC_JOB_WRITE) && (WritePtr == NULL_PTR)) ||
        ((JobType == MEMACC_JOB_COMPARE) && (ComparePtr == NULL_PTR)))
    {
        (void)Det_ReportError(MEMACC_MODULE_ID, MEMACC_INSTANCE_ID,
                              ServiceId, MEMACC_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    if (Length == 0u)
    {
        (void)Det_ReportError(MEMACC_MODULE_ID, MEMACC_INSTANCE_ID,
                              ServiceId, MEMACC_E_PARAM_LENGTH);
        return E_NOT_OK;
    }
    if (MemAcc_Internal_ValidateAddress(AreaId, Address, Length) == FALSE)
    {
        (void)Det_ReportError(MEMACC_MODULE_ID, MEMACC_INSTANCE_ID,
                              ServiceId, MEMACC_E_PARAM_ADDRESS);
        return E_NOT_OK;
    }
#else
    if (MemAcc_Internal_FindArea(AreaId, &areaIndex) == FALSE)
    {
        return E_NOT_OK;
    }
    (void)ServiceId;
#endif

    SchM_Enter_MemAcc_MEMACC_EXCLUSIVE_AREA_0();

    if (MemAcc_AreaBusy[areaIndex] == TRUE)
    {
#if (MEMACC_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(MEMACC_MODULE_ID, MEMACC_INSTANCE_ID,
                              ServiceId, MEMACC_E_BUSY);
#endif
        SchM_Exit_MemAcc_MEMACC_EXCLUSIVE_AREA_0();
        return E_NOT_OK;
    }

    if (MemAcc_AreaLocked[areaIndex] == TRUE)
    {
        SchM_Exit_MemAcc_MEMACC_EXCLUSIVE_AREA_0();
        return E_NOT_OK;
    }

    MemAcc_CurrentJob[areaIndex].JobType = JobType;
    MemAcc_CurrentJob[areaIndex].AreaId = AreaId;
    MemAcc_CurrentJob[areaIndex].Address = Address;
    MemAcc_CurrentJob[areaIndex].ReadDataPtr = ReadPtr;
    MemAcc_CurrentJob[areaIndex].WriteDataPtr = WritePtr;
    MemAcc_CurrentJob[areaIndex].CompareDataPtr = ComparePtr;
    MemAcc_CurrentJob[areaIndex].Length = Length;
    MemAcc_CurrentJob[areaIndex].ProcessedLength = 0u;

    MemAcc_AreaJobResult[areaIndex] = MEMACC_JOB_PENDING;
    MemAcc_AreaBusy[areaIndex] = TRUE;

    SchM_Exit_MemAcc_MEMACC_EXCLUSIVE_AREA_0();

    /* Compare is handled in MainFunction, not dispatched to driver */
    if (JobType == MEMACC_JOB_COMPARE)
    {
        retVal = E_OK;
    }
    else
    {
        retVal = MemAcc_Internal_DispatchToMemDriver(areaIndex);
        if (retVal != E_OK)
        {
            MemAcc_Internal_FinishJob(areaIndex, MEMACC_JOB_FAILED);
        }
    }

    return retVal;
}

/*============================================================================*
 *  Public API implementations
 *============================================================================*/

/**
 * \brief  Initialize the MemAcc module
 */
FUNC(void, MEMACC_CODE) MemAcc_Init(
    P2CONST(MemAcc_ConfigType, AUTOMATIC, MEMACC_APPL_CONST) ConfigPtr
)
{
    uint8 areaIdx;

#if (MEMACC_DEV_ERROR_DETECT == STD_ON)
    if (ConfigPtr == NULL_PTR)
    {
        (void)Det_ReportError(MEMACC_MODULE_ID, MEMACC_INSTANCE_ID,
                              MEMACC_SID_INIT, MEMACC_E_PARAM_POINTER);
        return;
    }
#endif

    SchM_Enter_MemAcc_MEMACC_EXCLUSIVE_AREA_0();

    MemAcc_ConfigPtr = ConfigPtr;

    for (areaIdx = 0u; areaIdx < MEMACC_NUMBER_OF_ADDRESS_AREAS; areaIdx++)
    {
        MemAcc_CurrentJob[areaIdx].JobType = MEMACC_JOB_NONE;
        MemAcc_CurrentJob[areaIdx].AreaId = 0u;
        MemAcc_CurrentJob[areaIdx].Address = 0u;
        MemAcc_CurrentJob[areaIdx].ReadDataPtr = NULL_PTR;
        MemAcc_CurrentJob[areaIdx].WriteDataPtr = NULL_PTR;
        MemAcc_CurrentJob[areaIdx].CompareDataPtr = NULL_PTR;
        MemAcc_CurrentJob[areaIdx].Length = 0u;
        MemAcc_CurrentJob[areaIdx].ProcessedLength = 0u;

        MemAcc_AreaJobResult[areaIdx] = MEMACC_JOB_OK;
        MemAcc_AreaBusy[areaIdx] = FALSE;
        MemAcc_AreaLocked[areaIdx] = FALSE;
    }

    MemAcc_ModuleStatus = MEMACC_IDLE;

    SchM_Exit_MemAcc_MEMACC_EXCLUSIVE_AREA_0();
}

/**
 * \brief  De-initialize the MemAcc module
 */
FUNC(void, MEMACC_CODE) MemAcc_DeInit(void)
{
    uint8 areaIdx;

#if (MEMACC_DEV_ERROR_DETECT == STD_ON)
    if (MemAcc_ModuleStatus == MEMACC_UNINIT)
    {
        (void)Det_ReportError(MEMACC_MODULE_ID, MEMACC_INSTANCE_ID,
                              MEMACC_SID_DEINIT, MEMACC_E_UNINIT);
        return;
    }
#endif

    SchM_Enter_MemAcc_MEMACC_EXCLUSIVE_AREA_0();

    for (areaIdx = 0u; areaIdx < MEMACC_NUMBER_OF_ADDRESS_AREAS; areaIdx++)
    {
        MemAcc_CurrentJob[areaIdx].JobType = MEMACC_JOB_NONE;
        MemAcc_AreaJobResult[areaIdx] = MEMACC_JOB_OK;
        MemAcc_AreaBusy[areaIdx] = FALSE;
        MemAcc_AreaLocked[areaIdx] = FALSE;
    }

    MemAcc_ConfigPtr = NULL_PTR;
    MemAcc_ModuleStatus = MEMACC_UNINIT;

    SchM_Exit_MemAcc_MEMACC_EXCLUSIVE_AREA_0();
}

/**
 * \brief  Initiate an asynchronous read operation
 */
FUNC(Std_ReturnType, MEMACC_CODE) MemAcc_Read(
    MemAcc_AddressAreaIdType AreaId,
    MemAcc_AddressType Address,
    P2VAR(MemAcc_DataType, AUTOMATIC, MEMACC_APPL_DATA) DataPtr,
    MemAcc_LengthType Length
)
{
    return MemAcc_Internal_AcceptJob(
        AreaId, MEMACC_JOB_READ, Address,
        DataPtr, NULL_PTR, NULL_PTR,
        Length, MEMACC_SID_READ);
}

/**
 * \brief  Initiate an asynchronous write operation
 */
FUNC(Std_ReturnType, MEMACC_CODE) MemAcc_Write(
    MemAcc_AddressAreaIdType AreaId,
    MemAcc_AddressType Address,
    P2CONST(MemAcc_DataType, AUTOMATIC, MEMACC_APPL_DATA) DataPtr,
    MemAcc_LengthType Length
)
{
    return MemAcc_Internal_AcceptJob(
        AreaId, MEMACC_JOB_WRITE, Address,
        NULL_PTR, DataPtr, NULL_PTR,
        Length, MEMACC_SID_WRITE);
}

/**
 * \brief  Initiate an asynchronous erase operation
 */
FUNC(Std_ReturnType, MEMACC_CODE) MemAcc_Erase(
    MemAcc_AddressAreaIdType AreaId,
    MemAcc_AddressType Address,
    MemAcc_LengthType Length
)
{
    return MemAcc_Internal_AcceptJob(
        AreaId, MEMACC_JOB_ERASE, Address,
        NULL_PTR, NULL_PTR, NULL_PTR,
        Length, MEMACC_SID_ERASE);
}

/**
 * \brief  Initiate an asynchronous blank check operation
 */
FUNC(Std_ReturnType, MEMACC_CODE) MemAcc_BlankCheck(
    MemAcc_AddressAreaIdType AreaId,
    MemAcc_AddressType Address,
    MemAcc_LengthType Length
)
{
    return MemAcc_Internal_AcceptJob(
        AreaId, MEMACC_JOB_BLANKCHECK, Address,
        NULL_PTR, NULL_PTR, NULL_PTR,
        Length, MEMACC_SID_BLANK_CHECK);
}

/**
 * \brief  Initiate an asynchronous compare operation
 */
FUNC(Std_ReturnType, MEMACC_CODE) MemAcc_Compare(
    MemAcc_AddressAreaIdType AreaId,
    MemAcc_AddressType Address,
    P2CONST(MemAcc_DataType, AUTOMATIC, MEMACC_APPL_DATA) DataPtr,
    MemAcc_LengthType Length
)
{
    return MemAcc_Internal_AcceptJob(
        AreaId, MEMACC_JOB_COMPARE, Address,
        NULL_PTR, NULL_PTR, DataPtr,
        Length, MEMACC_SID_COMPARE);
}

/**
 * \brief  Get the number of bytes processed
 */
FUNC(MemAcc_LengthType, MEMACC_CODE) MemAcc_GetProcessedLength(
    MemAcc_AddressAreaIdType AreaId
)
{
    MemAcc_LengthType processedLen = 0u;
    uint8 areaIndex;

#if (MEMACC_DEV_ERROR_DETECT == STD_ON)
    if (MemAcc_ModuleStatus == MEMACC_UNINIT)
    {
        (void)Det_ReportError(MEMACC_MODULE_ID, MEMACC_INSTANCE_ID,
                              MEMACC_SID_GET_PROCESSED_LENGTH, MEMACC_E_UNINIT);
        return 0u;
    }
    if (MemAcc_Internal_FindArea(AreaId, &areaIndex) == FALSE)
    {
        (void)Det_ReportError(MEMACC_MODULE_ID, MEMACC_INSTANCE_ID,
                              MEMACC_SID_GET_PROCESSED_LENGTH, MEMACC_E_PARAM_ADDRESS_AREA);
        return 0u;
    }
#else
    if (MemAcc_Internal_FindArea(AreaId, &areaIndex) == FALSE)
    {
        return 0u;
    }
#endif

    SchM_Enter_MemAcc_MEMACC_EXCLUSIVE_AREA_0();
    processedLen = MemAcc_CurrentJob[areaIndex].ProcessedLength;
    SchM_Exit_MemAcc_MEMACC_EXCLUSIVE_AREA_0();

    return processedLen;
}

/**
 * \brief  Execute a hardware-specific service request
 *
 * <PLACEHOLDER_REQUIRED> No HW-specific services defined for TC3xx DFLASH
 */
FUNC(Std_ReturnType, MEMACC_CODE) MemAcc_HwSpecificServiceRequest(
    MemAcc_AddressAreaIdType AreaId,
    P2VAR(MemAcc_DataType, AUTOMATIC, MEMACC_APPL_DATA) DataPtr,
    MemAcc_LengthType Length
)
{
    (void)AreaId;
    (void)DataPtr;
    (void)Length;

    /* <PLACEHOLDER_REQUIRED> No HW-specific services defined for TC3xx DFLASH */
    return E_NOT_OK;
}

/**
 * \brief  Cancel an ongoing job
 */
FUNC(void, MEMACC_CODE) MemAcc_Cancel(
    MemAcc_AddressAreaIdType AreaId
)
{
    uint8 areaIndex;

#if (MEMACC_DEV_ERROR_DETECT == STD_ON)
    if (MemAcc_ModuleStatus == MEMACC_UNINIT)
    {
        (void)Det_ReportError(MEMACC_MODULE_ID, MEMACC_INSTANCE_ID,
                              MEMACC_SID_CANCEL, MEMACC_E_UNINIT);
        return;
    }
    if (MemAcc_Internal_FindArea(AreaId, &areaIndex) == FALSE)
    {
        (void)Det_ReportError(MEMACC_MODULE_ID, MEMACC_INSTANCE_ID,
                              MEMACC_SID_CANCEL, MEMACC_E_PARAM_ADDRESS_AREA);
        return;
    }
#else
    if (MemAcc_Internal_FindArea(AreaId, &areaIndex) == FALSE)
    {
        return;
    }
#endif

    SchM_Enter_MemAcc_MEMACC_EXCLUSIVE_AREA_0();

    if (MemAcc_AreaBusy[areaIndex] == TRUE)
    {
        MemAcc_AreaJobResult[areaIndex] = MEMACC_JOB_CANCELED;
        MemAcc_AreaBusy[areaIndex] = FALSE;
        MemAcc_CurrentJob[areaIndex].JobType = MEMACC_JOB_NONE;
    }

    SchM_Exit_MemAcc_MEMACC_EXCLUSIVE_AREA_0();
}

/**
 * \brief  Get job result for the specified area
 */
FUNC(MemAcc_JobResultType, MEMACC_CODE) MemAcc_GetJobResult(
    MemAcc_AddressAreaIdType AreaId
)
{
    MemAcc_JobResultType result = MEMACC_JOB_OK;
    uint8 areaIndex;

#if (MEMACC_DEV_ERROR_DETECT == STD_ON)
    if (MemAcc_ModuleStatus == MEMACC_UNINIT)
    {
        (void)Det_ReportError(MEMACC_MODULE_ID, MEMACC_INSTANCE_ID,
                              MEMACC_SID_GET_JOB_RESULT, MEMACC_E_UNINIT);
        return MEMACC_JOB_FAILED;
    }
    if (MemAcc_Internal_FindArea(AreaId, &areaIndex) == FALSE)
    {
        (void)Det_ReportError(MEMACC_MODULE_ID, MEMACC_INSTANCE_ID,
                              MEMACC_SID_GET_JOB_RESULT, MEMACC_E_PARAM_ADDRESS_AREA);
        return MEMACC_JOB_FAILED;
    }
#else
    if (MemAcc_Internal_FindArea(AreaId, &areaIndex) == FALSE)
    {
        return MEMACC_JOB_FAILED;
    }
#endif

    SchM_Enter_MemAcc_MEMACC_EXCLUSIVE_AREA_0();
    result = MemAcc_AreaJobResult[areaIndex];
    SchM_Exit_MemAcc_MEMACC_EXCLUSIVE_AREA_0();

    return result;
}

/**
 * \brief  Get segmentation info for the specified area
 */
FUNC(Std_ReturnType, MEMACC_CODE) MemAcc_GetSegmentationInfo(
    MemAcc_AddressAreaIdType AreaId,
    P2VAR(MemAcc_LengthType, AUTOMATIC, MEMACC_APPL_DATA) SectorSizePtr,
    P2VAR(MemAcc_LengthType, AUTOMATIC, MEMACC_APPL_DATA) PageSizePtr
)
{
    uint8 areaIndex;

#if (MEMACC_DEV_ERROR_DETECT == STD_ON)
    if (MemAcc_ModuleStatus == MEMACC_UNINIT)
    {
        (void)Det_ReportError(MEMACC_MODULE_ID, MEMACC_INSTANCE_ID,
                              MEMACC_SID_GET_SEGMENTATION_INFO, MEMACC_E_UNINIT);
        return E_NOT_OK;
    }
    if (MemAcc_Internal_FindArea(AreaId, &areaIndex) == FALSE)
    {
        (void)Det_ReportError(MEMACC_MODULE_ID, MEMACC_INSTANCE_ID,
                              MEMACC_SID_GET_SEGMENTATION_INFO, MEMACC_E_PARAM_ADDRESS_AREA);
        return E_NOT_OK;
    }
    if ((SectorSizePtr == NULL_PTR) || (PageSizePtr == NULL_PTR))
    {
        (void)Det_ReportError(MEMACC_MODULE_ID, MEMACC_INSTANCE_ID,
                              MEMACC_SID_GET_SEGMENTATION_INFO, MEMACC_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#else
    if (MemAcc_Internal_FindArea(AreaId, &areaIndex) == FALSE)
    {
        return E_NOT_OK;
    }
#endif

    *SectorSizePtr = MemAcc_ConfigPtr->AddressAreas[areaIndex].SectorSize;
    *PageSizePtr = MemAcc_ConfigPtr->AddressAreas[areaIndex].PageSize;

    return E_OK;
}

/**
 * \brief  Request exclusive lock on the specified area
 */
FUNC(Std_ReturnType, MEMACC_CODE) MemAcc_RequestLock(
    MemAcc_AddressAreaIdType AreaId
)
{
    Std_ReturnType retVal = E_NOT_OK;
    uint8 areaIndex;

#if (MEMACC_DEV_ERROR_DETECT == STD_ON)
    if (MemAcc_ModuleStatus == MEMACC_UNINIT)
    {
        (void)Det_ReportError(MEMACC_MODULE_ID, MEMACC_INSTANCE_ID,
                              MEMACC_SID_REQUEST_LOCK, MEMACC_E_UNINIT);
        return E_NOT_OK;
    }
    if (MemAcc_Internal_FindArea(AreaId, &areaIndex) == FALSE)
    {
        (void)Det_ReportError(MEMACC_MODULE_ID, MEMACC_INSTANCE_ID,
                              MEMACC_SID_REQUEST_LOCK, MEMACC_E_PARAM_ADDRESS_AREA);
        return E_NOT_OK;
    }
#else
    if (MemAcc_Internal_FindArea(AreaId, &areaIndex) == FALSE)
    {
        return E_NOT_OK;
    }
#endif

    SchM_Enter_MemAcc_MEMACC_EXCLUSIVE_AREA_0();

    if (MemAcc_AreaLocked[areaIndex] == FALSE)
    {
        MemAcc_AreaLocked[areaIndex] = TRUE;
        retVal = E_OK;
    }

    SchM_Exit_MemAcc_MEMACC_EXCLUSIVE_AREA_0();

    return retVal;
}

/**
 * \brief  Release exclusive lock on the specified area
 */
FUNC(Std_ReturnType, MEMACC_CODE) MemAcc_ReleaseLock(
    MemAcc_AddressAreaIdType AreaId
)
{
    Std_ReturnType retVal = E_NOT_OK;
    uint8 areaIndex;

#if (MEMACC_DEV_ERROR_DETECT == STD_ON)
    if (MemAcc_ModuleStatus == MEMACC_UNINIT)
    {
        (void)Det_ReportError(MEMACC_MODULE_ID, MEMACC_INSTANCE_ID,
                              MEMACC_SID_RELEASE_LOCK, MEMACC_E_UNINIT);
        return E_NOT_OK;
    }
    if (MemAcc_Internal_FindArea(AreaId, &areaIndex) == FALSE)
    {
        (void)Det_ReportError(MEMACC_MODULE_ID, MEMACC_INSTANCE_ID,
                              MEMACC_SID_RELEASE_LOCK, MEMACC_E_PARAM_ADDRESS_AREA);
        return E_NOT_OK;
    }
#else
    if (MemAcc_Internal_FindArea(AreaId, &areaIndex) == FALSE)
    {
        return E_NOT_OK;
    }
#endif

    SchM_Enter_MemAcc_MEMACC_EXCLUSIVE_AREA_0();

    if (MemAcc_AreaLocked[areaIndex] == TRUE)
    {
        MemAcc_AreaLocked[areaIndex] = FALSE;
        retVal = E_OK;
    }

    SchM_Exit_MemAcc_MEMACC_EXCLUSIVE_AREA_0();

    return retVal;
}

/**
 * \brief  Get module version information
 */
FUNC(void, MEMACC_CODE) MemAcc_GetVersionInfo(
    P2VAR(Std_VersionInfoType, AUTOMATIC, MEMACC_APPL_DATA) VersionInfoPtr
)
{
#if (MEMACC_DEV_ERROR_DETECT == STD_ON)
    if (VersionInfoPtr == NULL_PTR)
    {
        (void)Det_ReportError(MEMACC_MODULE_ID, MEMACC_INSTANCE_ID,
                              MEMACC_SID_GET_VERSION_INFO, MEMACC_E_PARAM_POINTER);
        return;
    }
#endif

    VersionInfoPtr->vendorID = MEMACC_VENDOR_ID;
    VersionInfoPtr->moduleID = MEMACC_MODULE_ID;
    VersionInfoPtr->sw_major_version = MEMACC_SW_MAJOR_VERSION;
    VersionInfoPtr->sw_minor_version = MEMACC_SW_MINOR_VERSION;
    VersionInfoPtr->sw_patch_version = MEMACC_SW_PATCH_VERSION;
}

#define MEMACC_STOP_SEC_CODE
#include "MemAcc_MemMap.h"
