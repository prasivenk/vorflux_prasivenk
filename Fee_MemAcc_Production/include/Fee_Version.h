/**
 * \file       Fee_Version.h
 * \brief      Fee Module Version Information
 *
 * \details    Defines vendor, module, and version macros for the Fee module
 *             per AUTOSAR R24-11. Includes inter-module version checks
 *             against Std_Types.h AUTOSAR release version.
 *
 * \copyright  Copyright (c) 2024 -- All rights reserved.
 *             AUTOSAR R24-11 compliant.
 *
 * \autosar    AUTOSAR_SWS_FlashEEPROMEmulation
 * \version    2.0.0
 */

#ifndef FEE_VERSION_H
#define FEE_VERSION_H

/*============================================================================*
 *  Includes
 *============================================================================*/

#include "Std_Types.h"

/*============================================================================*
 *  Vendor and module identification
 *============================================================================*/

/** \brief Vendor ID (placeholder -- assign real vendor ID before delivery) */
#define FEE_VENDOR_ID       0xFFFFu

/** \brief AUTOSAR module ID for Fee (SWS identifier) */
#define FEE_MODULE_ID       21u

/** \brief Instance ID of the Fee module */
#define FEE_INSTANCE_ID     0u

/*============================================================================*
 *  Software version information
 *============================================================================*/

/** \brief Software major version */
#define FEE_SW_MAJOR_VERSION    2u

/** \brief Software minor version */
#define FEE_SW_MINOR_VERSION    0u

/** \brief Software patch version */
#define FEE_SW_PATCH_VERSION    0u

/*============================================================================*
 *  AUTOSAR release version information
 *============================================================================*/

/** \brief AUTOSAR release major version */
#define FEE_AR_RELEASE_MAJOR_VERSION        24u

/** \brief AUTOSAR release minor version */
#define FEE_AR_RELEASE_MINOR_VERSION        11u

/** \brief AUTOSAR release revision version */
#define FEE_AR_RELEASE_REVISION_VERSION     0u

/*============================================================================*
 *  Inter-module version checks
 *============================================================================*/

/* Check against Std_Types AUTOSAR release version */
#if (FEE_AR_RELEASE_MAJOR_VERSION != STD_AR_RELEASE_MAJOR_VERSION)
  #error "Fee_Version.h: AR Release Major Version mismatch with Std_Types.h"
#endif

#if (FEE_AR_RELEASE_MINOR_VERSION != STD_AR_RELEASE_MINOR_VERSION)
  #error "Fee_Version.h: AR Release Minor Version mismatch with Std_Types.h"
#endif

#endif /* FEE_VERSION_H */
