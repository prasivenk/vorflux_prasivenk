/**
 * \file       Std_Types.h
 * \brief      AUTOSAR Standard Types
 *
 * \details    Defines standard return types, version info type, and
 *             common constants per AUTOSAR R24-11 SWS Standard Types.
 *
 * \copyright  Copyright (c) 2024 -- All rights reserved.
 *             AUTOSAR R24-11 compliant.
 *
 * \autosar    AUTOSAR_SWS_StandardTypes
 * \version    24.11.0
 */

#ifndef STD_TYPES_H
#define STD_TYPES_H

/*============================================================================*
 *  Includes
 *============================================================================*/

#include "Platform_Types.h"
#include "Compiler.h"

/*============================================================================*
 *  Version information
 *============================================================================*/

#define STD_TYPES_VENDOR_ID                     0xFFFFu
#define STD_TYPES_MODULE_ID                     197u

#define STD_AR_RELEASE_MAJOR_VERSION            24u
#define STD_AR_RELEASE_MINOR_VERSION            11u
#define STD_AR_RELEASE_REVISION_VERSION         0u

#define STD_TYPES_SW_MAJOR_VERSION              2u
#define STD_TYPES_SW_MINOR_VERSION              0u
#define STD_TYPES_SW_PATCH_VERSION              0u

/*============================================================================*
 *  Standard return type
 *============================================================================*/

/** \brief Standard AUTOSAR return type */
typedef uint8 Std_ReturnType;

/** \brief Operation completed successfully */
#define E_OK        ((Std_ReturnType)0u)

/** \brief Operation failed */
#define E_NOT_OK    ((Std_ReturnType)1u)

/*============================================================================*
 *  Physical state and logical state
 *============================================================================*/

/** \brief Physical state: high voltage level */
#define STD_HIGH    0x01u

/** \brief Physical state: low voltage level */
#define STD_LOW     0x00u

/** \brief Logical state: active / on */
#define STD_ON      0x01u

/** \brief Logical state: inactive / off */
#define STD_OFF     0x00u

/*============================================================================*
 *  Version info type
 *============================================================================*/

/**
 * \brief  Standard version information type
 * \details Used by <Module>_GetVersionInfo APIs
 */
typedef struct
{
    uint16 vendorID;           /**< Vendor ID */
    uint16 moduleID;           /**< Module ID */
    uint8  sw_major_version;   /**< Software major version */
    uint8  sw_minor_version;   /**< Software minor version */
    uint8  sw_patch_version;   /**< Software patch version */
} Std_VersionInfoType;

/*============================================================================*
 *  Transformer error types (AUTOSAR R24-11)
 *============================================================================*/

/** \brief Transformer class for Std_TransformerError */
#define STD_TRANSFORMER_UNSPECIFIED     0x00u
#define STD_TRANSFORMER_SERIALIZER      0x01u
#define STD_TRANSFORMER_SAFETY          0x02u
#define STD_TRANSFORMER_SECURITY        0x03u
#define STD_TRANSFORMER_CUSTOM          0xFFu

#endif /* STD_TYPES_H */
