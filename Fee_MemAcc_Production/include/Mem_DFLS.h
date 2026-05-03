/**
 * \file       Mem_DFLS.h
 * \brief      Mem Driver Interface for TC3xx DFLASH -- Placeholder
 *
 * \details    Provides type definitions and asynchronous API declarations
 *             for the TC3xx Data Flash (DFLASH) memory driver, conforming
 *             to AUTOSAR R24-11 Mem driver interface.
 *
 *             <PLACEHOLDER_REQUIRED> Actual implementation provided by
 *             MCAL vendor for TC3xx.
 *
 * \copyright  Copyright (c) 2024 -- All rights reserved.
 *             AUTOSAR R24-11 compliant.
 *
 * \autosar    AUTOSAR_SWS_MemDriver
 * \version    24.11.0
 */

#ifndef MEM_DFLS_H
#define MEM_DFLS_H

/*============================================================================*
 *  Includes
 *============================================================================*/

#include "Std_Types.h"

/*============================================================================*
 *  Version information
 *============================================================================*/

#define MEM_DFLS_VENDOR_ID                      0xFFFFu
#define MEM_DFLS_MODULE_ID                      91u

#define MEM_DFLS_AR_RELEASE_MAJOR_VERSION       24u
#define MEM_DFLS_AR_RELEASE_MINOR_VERSION       11u
#define MEM_DFLS_AR_RELEASE_REVISION_VERSION    0u

#define MEM_DFLS_SW_MAJOR_VERSION               1u
#define MEM_DFLS_SW_MINOR_VERSION               0u
#define MEM_DFLS_SW_PATCH_VERSION               0u

/*============================================================================*
 *  Type definitions
 *============================================================================*/

/** \brief Address type for DFLASH operations */
typedef uint32 Mem_DFLS_AddressType;

/** \brief Length type for DFLASH operations */
typedef uint32 Mem_DFLS_LengthType;

/**
 * \brief  Job result type for DFLASH operations
 *
 * <PLACEHOLDER_REQUIRED> Values must match MCAL vendor driver.
 */
typedef enum
{
    MEM_DFLS_JOB_OK              = 0,  /**< Job completed successfully */
    MEM_DFLS_JOB_PENDING         = 1,  /**< Job is still in progress */
    MEM_DFLS_JOB_FAILED          = 2,  /**< Job failed */
    MEM_DFLS_JOB_ECC_CORRECTED   = 3,  /**< Job completed, correctable ECC error */
    MEM_DFLS_JOB_ECC_UNCORRECTED = 4   /**< Job failed, uncorrectable ECC error */
} Mem_DFLS_JobResultType;

/*============================================================================*
 *  Function declarations
 *
 *  <PLACEHOLDER_REQUIRED> Actual implementation provided by MCAL vendor
 *  for TC3xx.
 *============================================================================*/

/**
 * \brief  Initiate an asynchronous read from DFLASH
 *
 * \param[in]  SourceAddress   Source address in DFLASH
 * \param[out] TargetBufferPtr Pointer to target data buffer
 * \param[in]  Length          Number of bytes to read
 *
 * \return     E_OK if job was accepted, E_NOT_OK otherwise
 */
extern FUNC(Std_ReturnType, MEM_DFLS_CODE) Mem_DFLS_Read(
    Mem_DFLS_AddressType SourceAddress,
    P2VAR(uint8, AUTOMATIC, MEM_DFLS_APPL_DATA) TargetBufferPtr,
    Mem_DFLS_LengthType Length
);

/**
 * \brief  Initiate an asynchronous write to DFLASH
 *
 * \param[in] TargetAddress   Target address in DFLASH
 * \param[in] SourceBufferPtr Pointer to source data buffer
 * \param[in] Length          Number of bytes to write
 *
 * \return    E_OK if job was accepted, E_NOT_OK otherwise
 */
extern FUNC(Std_ReturnType, MEM_DFLS_CODE) Mem_DFLS_Write(
    Mem_DFLS_AddressType TargetAddress,
    P2CONST(uint8, AUTOMATIC, MEM_DFLS_APPL_DATA) SourceBufferPtr,
    Mem_DFLS_LengthType Length
);

/**
 * \brief  Initiate an asynchronous erase of DFLASH sectors
 *
 * \param[in] TargetAddress   Target address in DFLASH
 * \param[in] Length          Number of bytes to erase
 *
 * \return    E_OK if job was accepted, E_NOT_OK otherwise
 */
extern FUNC(Std_ReturnType, MEM_DFLS_CODE) Mem_DFLS_Erase(
    Mem_DFLS_AddressType TargetAddress,
    Mem_DFLS_LengthType Length
);

/**
 * \brief  Initiate an asynchronous blank check on DFLASH
 *
 * \param[in] TargetAddress   Target address in DFLASH
 * \param[in] Length          Number of bytes to check
 *
 * \return    E_OK if job was accepted, E_NOT_OK otherwise
 */
extern FUNC(Std_ReturnType, MEM_DFLS_CODE) Mem_DFLS_BlankCheck(
    Mem_DFLS_AddressType TargetAddress,
    Mem_DFLS_LengthType Length
);

/**
 * \brief  Get the result of the most recent or current job
 *
 * \return    Job result status
 */
extern FUNC(Mem_DFLS_JobResultType, MEM_DFLS_CODE) Mem_DFLS_GetJobResult(void);

/**
 * \brief  Cyclic main function for DFLASH driver processing
 *
 * \details   Called periodically by the BSW scheduler to drive
 *            asynchronous flash operations to completion.
 */
extern FUNC(void, MEM_DFLS_CODE) Mem_DFLS_MainFunction(void);

#endif /* MEM_DFLS_H */
