/**
 * \file       Fee_Cfg.h
 * \brief      AUTOSAR Fee Module -- Configuration
 *
 * \details    Configuration switches and parameters for the Fee module
 *             per AUTOSAR R24-11 SWS Fee.
 *
 * \copyright  Copyright (c) 2024 -- All rights reserved.
 *             AUTOSAR R24-11 compliant.
 *
 * \autosar    AUTOSAR_SWS_FlashEEPROMEmulation
 * \version    2.0.0
 */

#ifndef FEE_CFG_H
#define FEE_CFG_H

/*============================================================================*
 *  Includes
 *============================================================================*/

#include "Std_Types.h"

/*============================================================================*
 *  Configuration switches
 *============================================================================*/

/** \brief Enable/Disable Development Error Detection */
#define FEE_DEV_ERROR_DETECT        STD_ON

/** \brief Enable/Disable Version Info API */
#define FEE_VERSION_INFO_API        STD_ON

/** \brief Enable/Disable Polling Mode (Fee polls MemAcc, no callbacks) */
#define FEE_POLLING_MODE            STD_ON

/** \brief Enable/Disable NvM Job End Notification */
#define FEE_NVM_JOB_END_NOTIFICATION    STD_ON

/** \brief Enable/Disable NvM Job Error Notification */
#define FEE_NVM_JOB_ERROR_NOTIFICATION  STD_ON

/** \brief Enable/Disable Safety checks */
#define FEE_SAFETY_ENABLE           STD_ON

/** \brief Enable/Disable Read-back Verification */
#define FEE_READ_BACK_VERIFICATION  STD_ON

/*============================================================================*
 *  Configuration parameters
 *============================================================================*/

/** \brief Virtual page size in bytes (TC3xx DFLASH page size) */
#define FEE_VIRTUAL_PAGE_SIZE       32u

/** \brief Number of configured blocks */
#define FEE_NUMBER_OF_BLOCKS        8u

/** \brief Number of configured sectors */
#define FEE_NUMBER_OF_SECTORS       4u

/** \brief Sector size in bytes (4 physical 4KB sectors = 16KB virtual sector) */
#define FEE_SECTOR_SIZE             16384u

/** \brief Main function period in milliseconds */
#define FEE_MAIN_FUNCTION_PERIOD    5u

/** \brief Maximum block data size in bytes */
#define FEE_MAX_BLOCK_SIZE          512u

/** \brief GC restart fill threshold percentage */
#define FEE_GC_RESTART_THRESHOLD    80u

/*============================================================================*
 *  NvM notification function declarations
 *============================================================================*/

#if (FEE_NVM_JOB_END_NOTIFICATION == STD_ON)
extern void NvM_JobEndNotification(void);
#endif

#if (FEE_NVM_JOB_ERROR_NOTIFICATION == STD_ON)
extern void NvM_JobErrorNotification(void);
#endif

#endif /* FEE_CFG_H */
