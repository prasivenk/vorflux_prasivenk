/**
 * \file       MemAcc.h
 * \brief      AUTOSAR MemAcc Module -- Public API Header
 *
 * \details    Declares all public APIs for the Memory Access module
 *             per AUTOSAR R24-11 SWS MemAcc.
 *
 * \copyright  Copyright (c) 2024 -- All rights reserved.
 *             AUTOSAR R24-11 compliant.
 *
 * \autosar    AUTOSAR_SWS_MemoryAccess
 * \version    24.11.0
 */

#ifndef MEMACC_H
#define MEMACC_H

/*============================================================================*
 *  Includes
 *============================================================================*/

#include "MemAcc_Types.h"
#include "MemAcc_Cfg.h"
#include "MemAcc_PBcfg.h"

/*============================================================================*
 *  Public API declarations
 *============================================================================*/

/**
 * \brief  Initialize the MemAcc module
 *
 * \param[in] ConfigPtr  Pointer to configuration structure
 */
extern FUNC(void, MEMACC_CODE) MemAcc_Init(
    P2CONST(MemAcc_ConfigType, AUTOMATIC, MEMACC_APPL_CONST) ConfigPtr
);

/**
 * \brief  De-initialize the MemAcc module
 */
extern FUNC(void, MEMACC_CODE) MemAcc_DeInit(void);

/**
 * \brief  Initiate an asynchronous read operation
 *
 * \param[in]  AreaId       Address area identifier
 * \param[in]  Address      Logical address within the area
 * \param[out] DataPtr      Pointer to destination buffer
 * \param[in]  Length       Number of bytes to read
 *
 * \return     E_OK if job was accepted, E_NOT_OK otherwise
 */
extern FUNC(Std_ReturnType, MEMACC_CODE) MemAcc_Read(
    MemAcc_AddressAreaIdType AreaId,
    MemAcc_AddressType Address,
    P2VAR(MemAcc_DataType, AUTOMATIC, MEMACC_APPL_DATA) DataPtr,
    MemAcc_LengthType Length
);

/**
 * \brief  Initiate an asynchronous write operation
 *
 * \param[in] AreaId       Address area identifier
 * \param[in] Address      Logical address within the area
 * \param[in] DataPtr      Pointer to source data buffer
 * \param[in] Length       Number of bytes to write
 *
 * \return    E_OK if job was accepted, E_NOT_OK otherwise
 */
extern FUNC(Std_ReturnType, MEMACC_CODE) MemAcc_Write(
    MemAcc_AddressAreaIdType AreaId,
    MemAcc_AddressType Address,
    P2CONST(MemAcc_DataType, AUTOMATIC, MEMACC_APPL_DATA) DataPtr,
    MemAcc_LengthType Length
);

/**
 * \brief  Initiate an asynchronous erase operation
 *
 * \param[in] AreaId       Address area identifier
 * \param[in] Address      Logical address within the area
 * \param[in] Length       Number of bytes to erase
 *
 * \return    E_OK if job was accepted, E_NOT_OK otherwise
 */
extern FUNC(Std_ReturnType, MEMACC_CODE) MemAcc_Erase(
    MemAcc_AddressAreaIdType AreaId,
    MemAcc_AddressType Address,
    MemAcc_LengthType Length
);

/**
 * \brief  Initiate an asynchronous blank check operation
 *
 * \param[in] AreaId       Address area identifier
 * \param[in] Address      Logical address within the area
 * \param[in] Length       Number of bytes to check
 *
 * \return    E_OK if job was accepted, E_NOT_OK otherwise
 */
extern FUNC(Std_ReturnType, MEMACC_CODE) MemAcc_BlankCheck(
    MemAcc_AddressAreaIdType AreaId,
    MemAcc_AddressType Address,
    MemAcc_LengthType Length
);

/**
 * \brief  Initiate an asynchronous compare operation
 *
 * \param[in] AreaId       Address area identifier
 * \param[in] Address      Logical address within the area
 * \param[in] DataPtr      Pointer to reference data buffer
 * \param[in] Length       Number of bytes to compare
 *
 * \return    E_OK if job was accepted, E_NOT_OK otherwise
 */
extern FUNC(Std_ReturnType, MEMACC_CODE) MemAcc_Compare(
    MemAcc_AddressAreaIdType AreaId,
    MemAcc_AddressType Address,
    P2CONST(MemAcc_DataType, AUTOMATIC, MEMACC_APPL_DATA) DataPtr,
    MemAcc_LengthType Length
);

/**
 * \brief  Get the number of bytes processed in the last or current job
 *
 * \param[in] AreaId  Address area identifier
 *
 * \return    Number of bytes processed
 */
extern FUNC(MemAcc_LengthType, MEMACC_CODE) MemAcc_GetProcessedLength(
    MemAcc_AddressAreaIdType AreaId
);

/**
 * \brief  Execute a hardware-specific service request
 *
 * \param[in] AreaId       Address area identifier
 * \param[in] DataPtr      Pointer to data buffer
 * \param[in] Length       Data length
 *
 * \return    E_NOT_OK always -- no HW-specific services defined
 *
 * <PLACEHOLDER_REQUIRED> No HW-specific services defined for TC3xx DFLASH
 */
extern FUNC(Std_ReturnType, MEMACC_CODE) MemAcc_HwSpecificServiceRequest(
    MemAcc_AddressAreaIdType AreaId,
    P2VAR(MemAcc_DataType, AUTOMATIC, MEMACC_APPL_DATA) DataPtr,
    MemAcc_LengthType Length
);

/**
 * \brief  Cancel an ongoing job for the specified area
 *
 * \param[in] AreaId  Address area identifier
 */
extern FUNC(void, MEMACC_CODE) MemAcc_Cancel(
    MemAcc_AddressAreaIdType AreaId
);

/**
 * \brief  Get the result of the last or current job for the specified area
 *
 * \param[in] AreaId  Address area identifier
 *
 * \return    Job result
 */
extern FUNC(MemAcc_JobResultType, MEMACC_CODE) MemAcc_GetJobResult(
    MemAcc_AddressAreaIdType AreaId
);

/**
 * \brief  Get segmentation info for the specified area
 *
 * \param[in]  AreaId         Address area identifier
 * \param[out] SectorSizePtr  Pointer to store sector size
 * \param[out] PageSizePtr    Pointer to store page size
 *
 * \return     E_OK if successful, E_NOT_OK otherwise
 */
extern FUNC(Std_ReturnType, MEMACC_CODE) MemAcc_GetSegmentationInfo(
    MemAcc_AddressAreaIdType AreaId,
    P2VAR(MemAcc_LengthType, AUTOMATIC, MEMACC_APPL_DATA) SectorSizePtr,
    P2VAR(MemAcc_LengthType, AUTOMATIC, MEMACC_APPL_DATA) PageSizePtr
);

/**
 * \brief  Request exclusive lock on the specified area
 *
 * \param[in] AreaId  Address area identifier
 *
 * \return    E_OK if lock was acquired, E_NOT_OK otherwise
 */
extern FUNC(Std_ReturnType, MEMACC_CODE) MemAcc_RequestLock(
    MemAcc_AddressAreaIdType AreaId
);

/**
 * \brief  Release exclusive lock on the specified area
 *
 * \param[in] AreaId  Address area identifier
 *
 * \return    E_OK if lock was released, E_NOT_OK otherwise
 */
extern FUNC(Std_ReturnType, MEMACC_CODE) MemAcc_ReleaseLock(
    MemAcc_AddressAreaIdType AreaId
);

/**
 * \brief  Get module version information
 *
 * \param[out] VersionInfoPtr  Pointer to version info structure
 */
extern FUNC(void, MEMACC_CODE) MemAcc_GetVersionInfo(
    P2VAR(Std_VersionInfoType, AUTOMATIC, MEMACC_APPL_DATA) VersionInfoPtr
);

/**
 * \brief  Cyclic main function for MemAcc processing
 *
 * \details Called periodically by the BSW scheduler to drive
 *          asynchronous memory operations to completion.
 */
extern FUNC(void, MEMACC_CODE) MemAcc_MainFunction(void);

/*============================================================================*
 *  Internal function declarations (for inter-file linkage)
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

/** \brief Find address area config by area ID */
extern FUNC(boolean, MEMACC_CODE) MemAcc_Internal_FindArea(
    MemAcc_AddressAreaIdType AreaId
);

/** \brief Dispatch job to underlying Mem driver */
extern FUNC(Std_ReturnType, MEMACC_CODE) MemAcc_Internal_DispatchToMemDriver(
    MemAcc_AddressAreaIdType AreaId
);

/*============================================================================*
 *  Shared state externs (for inter-file access)
 *============================================================================*/

extern volatile VAR(MemAcc_StatusType, MEMACC_VAR) MemAcc_ModuleStatus;
extern volatile VAR(MemAcc_JobInfoType, MEMACC_VAR) MemAcc_CurrentJob[];
extern volatile VAR(MemAcc_JobResultType, MEMACC_VAR) MemAcc_AreaJobResult[];
extern volatile VAR(boolean, MEMACC_VAR) MemAcc_AreaBusy[];
extern volatile VAR(boolean, MEMACC_VAR) MemAcc_AreaLocked[];
extern P2CONST(MemAcc_ConfigType, AUTOMATIC, MEMACC_CONST) MemAcc_ConfigPtr;

#endif /* MEMACC_H */
