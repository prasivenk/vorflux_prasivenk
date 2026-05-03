/**
 * \file       MemAcc_Internal.h
 * \brief      AUTOSAR MemAcc Module -- Internal Function Declarations
 *
 * \details    Declares internal helper functions and shared state variables
 *             used across MemAcc translation units. These are not part of
 *             the public API and should not be included by module consumers.
 *
 * \copyright  Copyright (c) 2024 -- All rights reserved.
 *             AUTOSAR R24-11 compliant.
 *
 * \autosar    AUTOSAR_SWS_MemoryAccess
 * \version    24.11.0
 */

#ifndef MEMACC_INTERNAL_H
#define MEMACC_INTERNAL_H

/*============================================================================*
 *  Includes
 *============================================================================*/

#include "MemAcc_Types.h"
#include "MemAcc_Cfg.h"

/*============================================================================*
 *  Internal function declarations (inter-file linkage only)
 *============================================================================*/

/** \brief Validate address and length within area bounds */
extern FUNC(boolean, MEMACC_CODE) MemAcc_Internal_ValidateAddress(
    MemAcc_AddressAreaIdType AreaId,
    MemAcc_AddressType Address,
    MemAcc_LengthType Length
);

/** \brief Translate logical address to physical address */
extern FUNC(MemAcc_AddressType, MEMACC_CODE) MemAcc_Internal_TranslateAddress(
    MemAcc_AddressAreaIdType AreaId,
    MemAcc_AddressType LogicalAddress
);

/** \brief Find address area config by area ID and return the config array index
 *
 *  Maps the configured AreaId to an internal index into the per-area arrays.
 *  Sets *OutIndex to the matching config entry index.
 *
 *  \param[in]  AreaId    Area identifier to look up
 *  \param[out] OutIndex  Pointer to store the resolved config index (may be NULL)
 *
 *  \return TRUE if area exists, FALSE otherwise
 */
extern FUNC(boolean, MEMACC_CODE) MemAcc_Internal_FindArea(
    MemAcc_AddressAreaIdType AreaId,
    P2VAR(uint8, AUTOMATIC, MEMACC_VAR) OutIndex
);

/** \brief Dispatch job to underlying Mem driver */
extern FUNC(Std_ReturnType, MEMACC_CODE) MemAcc_Internal_DispatchToMemDriver(
    uint8 AreaIndex
);

/**
 * \brief  Accept a new MemAcc job (common validation and setup)
 *
 * \details Performs DET checks, exclusive-area, busy/lock checks,
 *          populates the job struct, sets pending/busy, dispatches.
 *
 * \param[in] AreaId     Address area identifier
 * \param[in] JobType    Job type (READ, WRITE, ERASE, BLANKCHECK)
 * \param[in] Address    Logical address within the area
 * \param[in] ReadPtr    Pointer to read destination (or NULL_PTR)
 * \param[in] WritePtr   Pointer to write source (or NULL_PTR)
 * \param[in] ComparePtr Pointer to compare reference (or NULL_PTR)
 * \param[in] Length     Number of bytes
 * \param[in] ServiceId  DET service ID for error reporting
 *
 * \return  E_OK if job was accepted, E_NOT_OK otherwise
 */
extern FUNC(Std_ReturnType, MEMACC_CODE) MemAcc_Internal_AcceptJob(
    MemAcc_AddressAreaIdType AreaId,
    MemAcc_JobType JobType,
    MemAcc_AddressType Address,
    P2VAR(MemAcc_DataType, AUTOMATIC, MEMACC_APPL_DATA) ReadPtr,
    P2CONST(MemAcc_DataType, AUTOMATIC, MEMACC_APPL_DATA) WritePtr,
    P2CONST(MemAcc_DataType, AUTOMATIC, MEMACC_APPL_DATA) ComparePtr,
    MemAcc_LengthType Length,
    uint8 ServiceId
);

/**
 * \brief  Finish a MemAcc job by setting result and clearing busy state
 *
 * \param[in] AreaIndex  Internal config array index
 * \param[in] Result     Job result to set
 */
extern FUNC(void, MEMACC_CODE) MemAcc_Internal_FinishJob(
    uint8 AreaIndex,
    MemAcc_JobResultType Result
);

/*============================================================================*
 *  Shared state variable externs (inter-file access)
 *============================================================================*/

extern volatile VAR(MemAcc_StatusType, MEMACC_VAR) MemAcc_ModuleStatus;
extern volatile VAR(MemAcc_JobInfoType, MEMACC_VAR) MemAcc_CurrentJob[];
extern volatile VAR(MemAcc_JobResultType, MEMACC_VAR) MemAcc_AreaJobResult[];
extern volatile VAR(boolean, MEMACC_VAR) MemAcc_AreaBusy[];
extern volatile VAR(boolean, MEMACC_VAR) MemAcc_AreaLocked[];
extern P2CONST(MemAcc_ConfigType, AUTOMATIC, MEMACC_CONST) MemAcc_ConfigPtr;

#endif /* MEMACC_INTERNAL_H */
