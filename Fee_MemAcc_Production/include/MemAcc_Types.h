/**
 * \file       MemAcc_Types.h
 * \brief      AUTOSAR MemAcc Module -- Type Definitions
 *
 * \details    Provides all type definitions for the Memory Access module
 *             per AUTOSAR R24-11 SWS MemAcc.
 *
 * \copyright  Copyright (c) 2024 -- All rights reserved.
 *             AUTOSAR R24-11 compliant.
 *
 * \autosar    AUTOSAR_SWS_MemoryAccess
 * \version    24.11.0
 */

#ifndef MEMACC_TYPES_H
#define MEMACC_TYPES_H

/*============================================================================*
 *  Includes
 *============================================================================*/

#include "Std_Types.h"

/*============================================================================*
 *  Type definitions
 *============================================================================*/

/** \brief Address area identifier type */
typedef uint8 MemAcc_AddressAreaIdType;

/** \brief Address type for memory operations */
typedef uint32 MemAcc_AddressType;

/** \brief Length type for memory operations */
typedef uint32 MemAcc_LengthType;

/** \brief Data type for memory operations */
typedef uint8 MemAcc_DataType;

/**
 * \brief  Job result type
 */
typedef enum
{
    MEMACC_JOB_OK              = 0,  /**< Job completed successfully */
    MEMACC_JOB_PENDING         = 1,  /**< Job is still in progress */
    MEMACC_JOB_FAILED          = 2,  /**< Job failed */
    MEMACC_JOB_CANCELED        = 3,  /**< Job was canceled */
    MEMACC_JOB_ECC_CORRECTED   = 4,  /**< Correctable ECC error detected */
    MEMACC_JOB_ECC_UNCORRECTED = 5   /**< Uncorrectable ECC error detected */
} MemAcc_JobResultType;

/**
 * \brief  Module status type
 */
typedef enum
{
    MEMACC_UNINIT = 0,  /**< Module not initialized */
    MEMACC_IDLE   = 1,  /**< Module initialized, no job pending */
    MEMACC_BUSY   = 2   /**< Module is processing a job */
} MemAcc_StatusType;

/**
 * \brief  Job type enumeration
 */
typedef enum
{
    MEMACC_JOB_NONE       = 0,  /**< No job */
    MEMACC_JOB_READ       = 1,  /**< Read job */
    MEMACC_JOB_WRITE      = 2,  /**< Write job */
    MEMACC_JOB_ERASE      = 3,  /**< Erase job */
    MEMACC_JOB_BLANKCHECK = 4,  /**< Blank check job */
    MEMACC_JOB_COMPARE    = 5   /**< Compare job */
} MemAcc_JobType;

/**
 * \brief  Address area configuration type
 */
typedef struct
{
    MemAcc_AddressAreaIdType AreaId;        /**< Area identifier */
    MemAcc_AddressType       StartAddress;  /**< Physical start address */
    MemAcc_LengthType        Length;        /**< Area length in bytes */
    MemAcc_LengthType        SectorSize;   /**< Sector (erase) size in bytes */
    MemAcc_LengthType        PageSize;     /**< Page (write) size in bytes */
} MemAcc_AddressAreaConfigType;

/**
 * \brief  Module configuration type
 */
typedef struct
{
    P2CONST(MemAcc_AddressAreaConfigType, AUTOMATIC, MEMACC_CONST) AddressAreas;
    uint8 NumberOfAreas;  /**< Number of configured address areas */
} MemAcc_ConfigType;

/**
 * \brief  Job information type
 */
typedef struct
{
    MemAcc_JobType          JobType;         /**< Current job type */
    MemAcc_AddressAreaIdType AreaId;         /**< Target area ID */
    MemAcc_AddressType      Address;         /**< Logical address within area */
    P2VAR(MemAcc_DataType, AUTOMATIC, MEMACC_APPL_DATA) ReadDataPtr;    /**< Read destination buffer */
    P2CONST(MemAcc_DataType, AUTOMATIC, MEMACC_APPL_DATA) WriteDataPtr; /**< Write source buffer */
    P2CONST(MemAcc_DataType, AUTOMATIC, MEMACC_APPL_DATA) CompareDataPtr; /**< Compare reference buffer */
    MemAcc_LengthType       Length;          /**< Job length in bytes */
    MemAcc_LengthType       ProcessedLength; /**< Bytes processed so far */
} MemAcc_JobInfoType;

/*============================================================================*
 *  DET error codes
 *============================================================================*/

/** \brief API called before module initialization */
#define MEMACC_E_UNINIT             0x01u

/** \brief API called with NULL pointer argument */
#define MEMACC_E_PARAM_POINTER      0x02u

/** \brief API called with invalid address area ID */
#define MEMACC_E_PARAM_ADDRESS_AREA 0x03u

/** \brief API called with invalid address */
#define MEMACC_E_PARAM_ADDRESS      0x04u

/** \brief API called with invalid length */
#define MEMACC_E_PARAM_LENGTH       0x05u

/** \brief API called while module is busy */
#define MEMACC_E_BUSY               0x06u

/*============================================================================*
 *  API service IDs
 *============================================================================*/

#define MEMACC_SID_INIT                     0x00u
#define MEMACC_SID_DEINIT                   0x01u
#define MEMACC_SID_READ                     0x02u
#define MEMACC_SID_WRITE                    0x03u
#define MEMACC_SID_ERASE                    0x04u
#define MEMACC_SID_BLANK_CHECK              0x05u
#define MEMACC_SID_COMPARE                  0x06u
#define MEMACC_SID_GET_PROCESSED_LENGTH     0x07u
#define MEMACC_SID_HW_SPECIFIC_SERVICE      0x08u
#define MEMACC_SID_CANCEL                   0x09u
#define MEMACC_SID_GET_JOB_RESULT           0x0Au
#define MEMACC_SID_GET_SEGMENTATION_INFO    0x0Bu
#define MEMACC_SID_REQUEST_LOCK             0x0Cu
#define MEMACC_SID_RELEASE_LOCK             0x0Du
#define MEMACC_SID_GET_VERSION_INFO         0x0Eu
#define MEMACC_SID_MAIN_FUNCTION            0x0Fu

/*============================================================================*
 *  DEM event IDs
 *============================================================================*/

/** \brief DEM event for hardware errors */
#define MEMACC_E_HARDWARE_ERROR  ((Dem_EventIdType)0x0001u)

/*============================================================================*
 *  Module identification
 *============================================================================*/

#define MEMACC_VENDOR_ID                    0xFFFFu
#define MEMACC_MODULE_ID                    166u
#define MEMACC_INSTANCE_ID                  0u

#define MEMACC_SW_MAJOR_VERSION             1u
#define MEMACC_SW_MINOR_VERSION             0u
#define MEMACC_SW_PATCH_VERSION             0u

#define MEMACC_AR_RELEASE_MAJOR_VERSION     24u
#define MEMACC_AR_RELEASE_MINOR_VERSION     11u
#define MEMACC_AR_RELEASE_REVISION_VERSION  0u

#endif /* MEMACC_TYPES_H */
