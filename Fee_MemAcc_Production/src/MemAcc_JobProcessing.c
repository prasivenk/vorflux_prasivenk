/**
 * \file       MemAcc_JobProcessing.c
 * \brief      AUTOSAR MemAcc Module -- Job Processing and MainFunction
 *
 * \details    Implements MemAcc_MainFunction with Mem_DFLS polling,
 *             ECC propagation, DEM reporting, and compare logic.
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
#include "MemAcc_Cfg.h"
#include "Mem_DFLS.h"
#include "Det.h"
#include "Dem.h"
#include "SchM_MemAcc.h"
#include <string.h>

/*============================================================================*
 *  Internal buffer for compare operations
 *============================================================================*/

#define MEMACC_START_SEC_VAR_CLEARED_8
#include "MemAcc_MemMap.h"

/** \brief Internal read buffer for compare operations */
static VAR(uint8, MEMACC_VAR) MemAcc_CompareBuffer[MEMACC_COMPARE_BUFFER_SIZE];

#define MEMACC_STOP_SEC_VAR_CLEARED_8
#include "MemAcc_MemMap.h"

/*============================================================================*
 *  Function implementations
 *============================================================================*/

#define MEMACC_START_SEC_CODE
#include "MemAcc_MemMap.h"

/**
 * \brief  Dispatch job to underlying Mem driver
 */
FUNC(Std_ReturnType, MEMACC_CODE) MemAcc_Internal_DispatchToMemDriver(
    MemAcc_AddressAreaIdType AreaId
)
{
    Std_ReturnType retVal = E_NOT_OK;
    MemAcc_AddressType physAddr;
    MemAcc_JobType jobType;
    MemAcc_LengthType length;

    jobType = MemAcc_CurrentJob[AreaId].JobType;
    physAddr = MemAcc_Internal_TranslateAddress(AreaId, MemAcc_CurrentJob[AreaId].Address);
    length = MemAcc_CurrentJob[AreaId].Length;

    switch (jobType)
    {
        case MEMACC_JOB_READ:
        {
            /* Need non-volatile local copy for Mem_DFLS call */
            P2VAR(uint8, AUTOMATIC, MEMACC_APPL_DATA) readPtr;
            readPtr = MemAcc_CurrentJob[AreaId].ReadDataPtr;
            retVal = Mem_DFLS_Read(physAddr, readPtr, length);
            break;
        }
        case MEMACC_JOB_WRITE:
        {
            /* Need non-volatile local copy for Mem_DFLS call */
            P2CONST(uint8, AUTOMATIC, MEMACC_APPL_DATA) writePtr;
            writePtr = MemAcc_CurrentJob[AreaId].WriteDataPtr;
            retVal = Mem_DFLS_Write(physAddr, writePtr, length);
            break;
        }
        case MEMACC_JOB_ERASE:
            retVal = Mem_DFLS_Erase(physAddr, length);
            break;

        case MEMACC_JOB_BLANKCHECK:
            retVal = Mem_DFLS_BlankCheck(physAddr, length);
            break;

        case MEMACC_JOB_COMPARE:
            /* Compare is handled in MainFunction, dispatch accepted */
            retVal = E_OK;
            break;

        case MEMACC_JOB_NONE:
        default:
            retVal = E_NOT_OK;
            break;
    }

    return retVal;
}

/**
 * \brief  Process compare job in MainFunction context
 *
 * \details Reads from flash into internal buffer, compares byte-by-byte
 *          with CompareDataPtr.
 */
static FUNC(void, MEMACC_CODE) MemAcc_Internal_ProcessCompare(
    MemAcc_AddressAreaIdType AreaId
)
{
    MemAcc_AddressType physAddr;
    MemAcc_LengthType remaining;
    MemAcc_LengthType chunkSize;
    MemAcc_LengthType processed;
    MemAcc_LengthType idx;
    Std_ReturnType readResult;
    Mem_DFLS_JobResultType driverResult;
    P2CONST(uint8, AUTOMATIC, MEMACC_APPL_DATA) comparePtr;
    boolean mismatchFound = FALSE;

    processed = MemAcc_CurrentJob[AreaId].ProcessedLength;
    remaining = MemAcc_CurrentJob[AreaId].Length - processed;
    comparePtr = MemAcc_CurrentJob[AreaId].CompareDataPtr;

    if (remaining == 0u)
    {
        /* Compare complete */
        SchM_Enter_MemAcc_MEMACC_EXCLUSIVE_AREA_0();
        MemAcc_AreaJobResult[AreaId] = MEMACC_JOB_OK;
        MemAcc_AreaBusy[AreaId] = FALSE;
        MemAcc_CurrentJob[AreaId].JobType = MEMACC_JOB_NONE;
        SchM_Exit_MemAcc_MEMACC_EXCLUSIVE_AREA_0();
        return;
    }

    chunkSize = remaining;
    if (chunkSize > MEMACC_COMPARE_BUFFER_SIZE)
    {
        chunkSize = MEMACC_COMPARE_BUFFER_SIZE;
    }

    physAddr = MemAcc_Internal_TranslateAddress(AreaId,
        MemAcc_CurrentJob[AreaId].Address + processed);

    /* Initiate synchronous-style read: dispatch and immediately poll */
    readResult = Mem_DFLS_Read(physAddr, MemAcc_CompareBuffer, chunkSize);
    if (readResult != E_OK)
    {
        SchM_Enter_MemAcc_MEMACC_EXCLUSIVE_AREA_0();
        MemAcc_AreaJobResult[AreaId] = MEMACC_JOB_FAILED;
        MemAcc_AreaBusy[AreaId] = FALSE;
        MemAcc_CurrentJob[AreaId].JobType = MEMACC_JOB_NONE;
        SchM_Exit_MemAcc_MEMACC_EXCLUSIVE_AREA_0();
        return;
    }

    /* Drive the read to completion */
    Mem_DFLS_MainFunction();
    driverResult = Mem_DFLS_GetJobResult();

    if (driverResult == MEM_DFLS_JOB_ECC_CORRECTED)
    {
        SchM_Enter_MemAcc_MEMACC_EXCLUSIVE_AREA_0();
        MemAcc_AreaJobResult[AreaId] = MEMACC_JOB_ECC_CORRECTED;
        MemAcc_AreaBusy[AreaId] = FALSE;
        MemAcc_CurrentJob[AreaId].JobType = MEMACC_JOB_NONE;
        SchM_Exit_MemAcc_MEMACC_EXCLUSIVE_AREA_0();
        return;
    }

    if (driverResult == MEM_DFLS_JOB_ECC_UNCORRECTED)
    {
        (void)Dem_SetEventStatus(MEMACC_E_HARDWARE_ERROR, DEM_EVENT_STATUS_FAILED);
        SchM_Enter_MemAcc_MEMACC_EXCLUSIVE_AREA_0();
        MemAcc_AreaJobResult[AreaId] = MEMACC_JOB_ECC_UNCORRECTED;
        MemAcc_AreaBusy[AreaId] = FALSE;
        MemAcc_CurrentJob[AreaId].JobType = MEMACC_JOB_NONE;
        SchM_Exit_MemAcc_MEMACC_EXCLUSIVE_AREA_0();
        return;
    }

    if (driverResult != MEM_DFLS_JOB_OK)
    {
        (void)Dem_SetEventStatus(MEMACC_E_HARDWARE_ERROR, DEM_EVENT_STATUS_FAILED);
        SchM_Enter_MemAcc_MEMACC_EXCLUSIVE_AREA_0();
        MemAcc_AreaJobResult[AreaId] = MEMACC_JOB_FAILED;
        MemAcc_AreaBusy[AreaId] = FALSE;
        MemAcc_CurrentJob[AreaId].JobType = MEMACC_JOB_NONE;
        SchM_Exit_MemAcc_MEMACC_EXCLUSIVE_AREA_0();
        return;
    }

    /* Compare byte-by-byte */
    for (idx = 0u; idx < chunkSize; idx++)
    {
        if (MemAcc_CompareBuffer[idx] != comparePtr[processed + idx])
        {
            mismatchFound = TRUE;
            break;
        }
    }

    if (mismatchFound == TRUE)
    {
        SchM_Enter_MemAcc_MEMACC_EXCLUSIVE_AREA_0();
        MemAcc_CurrentJob[AreaId].ProcessedLength = processed + idx;
        MemAcc_AreaJobResult[AreaId] = MEMACC_JOB_FAILED;
        MemAcc_AreaBusy[AreaId] = FALSE;
        MemAcc_CurrentJob[AreaId].JobType = MEMACC_JOB_NONE;
        SchM_Exit_MemAcc_MEMACC_EXCLUSIVE_AREA_0();
        return;
    }

    /* Chunk matched, update processed length */
    SchM_Enter_MemAcc_MEMACC_EXCLUSIVE_AREA_0();
    MemAcc_CurrentJob[AreaId].ProcessedLength = processed + chunkSize;
    SchM_Exit_MemAcc_MEMACC_EXCLUSIVE_AREA_0();

    /* If all bytes compared, mark complete */
    if ((processed + chunkSize) >= MemAcc_CurrentJob[AreaId].Length)
    {
        SchM_Enter_MemAcc_MEMACC_EXCLUSIVE_AREA_0();
        MemAcc_AreaJobResult[AreaId] = MEMACC_JOB_OK;
        MemAcc_AreaBusy[AreaId] = FALSE;
        MemAcc_CurrentJob[AreaId].JobType = MEMACC_JOB_NONE;
        SchM_Exit_MemAcc_MEMACC_EXCLUSIVE_AREA_0();
    }
}

/**
 * \brief  Cyclic main function for MemAcc processing
 */
FUNC(void, MEMACC_CODE) MemAcc_MainFunction(void)
{
    uint8 areaIdx;
    Mem_DFLS_JobResultType driverResult;

    if (MemAcc_ModuleStatus == MEMACC_UNINIT)
    {
        return;
    }

    for (areaIdx = 0u; areaIdx < MEMACC_NUMBER_OF_ADDRESS_AREAS; areaIdx++)
    {
        if (MemAcc_AreaBusy[areaIdx] == FALSE)
        {
            continue;
        }

        /* Handle compare jobs separately */
        if (MemAcc_CurrentJob[areaIdx].JobType == MEMACC_JOB_COMPARE)
        {
            MemAcc_Internal_ProcessCompare(areaIdx);
            continue;
        }

        /* Drive the underlying flash driver */
        Mem_DFLS_MainFunction();

        /* Poll the driver result */
        driverResult = Mem_DFLS_GetJobResult();

        switch (driverResult)
        {
            case MEM_DFLS_JOB_PENDING:
                /* Still in progress -- update processed length estimate */
                break;

            case MEM_DFLS_JOB_OK:
                SchM_Enter_MemAcc_MEMACC_EXCLUSIVE_AREA_0();
                MemAcc_CurrentJob[areaIdx].ProcessedLength =
                    MemAcc_CurrentJob[areaIdx].Length;
                MemAcc_AreaJobResult[areaIdx] = MEMACC_JOB_OK;
                MemAcc_AreaBusy[areaIdx] = FALSE;
                MemAcc_CurrentJob[areaIdx].JobType = MEMACC_JOB_NONE;
                SchM_Exit_MemAcc_MEMACC_EXCLUSIVE_AREA_0();
                break;

            case MEM_DFLS_JOB_FAILED:
                (void)Dem_SetEventStatus(MEMACC_E_HARDWARE_ERROR,
                                         DEM_EVENT_STATUS_FAILED);
                SchM_Enter_MemAcc_MEMACC_EXCLUSIVE_AREA_0();
                MemAcc_AreaJobResult[areaIdx] = MEMACC_JOB_FAILED;
                MemAcc_AreaBusy[areaIdx] = FALSE;
                MemAcc_CurrentJob[areaIdx].JobType = MEMACC_JOB_NONE;
                SchM_Exit_MemAcc_MEMACC_EXCLUSIVE_AREA_0();
                break;

            case MEM_DFLS_JOB_ECC_CORRECTED:
                SchM_Enter_MemAcc_MEMACC_EXCLUSIVE_AREA_0();
                MemAcc_CurrentJob[areaIdx].ProcessedLength =
                    MemAcc_CurrentJob[areaIdx].Length;
                MemAcc_AreaJobResult[areaIdx] = MEMACC_JOB_ECC_CORRECTED;
                MemAcc_AreaBusy[areaIdx] = FALSE;
                MemAcc_CurrentJob[areaIdx].JobType = MEMACC_JOB_NONE;
                SchM_Exit_MemAcc_MEMACC_EXCLUSIVE_AREA_0();
                break;

            case MEM_DFLS_JOB_ECC_UNCORRECTED:
                (void)Dem_SetEventStatus(MEMACC_E_HARDWARE_ERROR,
                                         DEM_EVENT_STATUS_FAILED);
                SchM_Enter_MemAcc_MEMACC_EXCLUSIVE_AREA_0();
                MemAcc_AreaJobResult[areaIdx] = MEMACC_JOB_ECC_UNCORRECTED;
                MemAcc_AreaBusy[areaIdx] = FALSE;
                MemAcc_CurrentJob[areaIdx].JobType = MEMACC_JOB_NONE;
                SchM_Exit_MemAcc_MEMACC_EXCLUSIVE_AREA_0();
                break;

            default:
                /* Unexpected result */
                break;
        }
    }
}

#define MEMACC_STOP_SEC_CODE
#include "MemAcc_MemMap.h"
