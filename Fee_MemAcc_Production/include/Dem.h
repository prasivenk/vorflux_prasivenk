/**
 * \file       Dem.h
 * \brief      AUTOSAR Diagnostic Event Manager (DEM) -- Placeholder Interface
 *
 * \details    Provides DEM type definitions and function declarations
 *             per AUTOSAR R24-11 SWS Diagnostic Event Manager.
 *             This is a placeholder; the actual DEM module is provided
 *             by the BSW diagnostic package.
 *
 * \copyright  Copyright (c) 2024 -- All rights reserved.
 *             AUTOSAR R24-11 compliant.
 *
 * \autosar    AUTOSAR_SWS_DiagnosticEventManager
 * \version    24.11.0
 */

#ifndef DEM_H
#define DEM_H

/*============================================================================*
 *  Includes
 *============================================================================*/

#include "Std_Types.h"

/*============================================================================*
 *  Version information
 *============================================================================*/

#define DEM_VENDOR_ID                       0xFFFFu
#define DEM_MODULE_ID                       54u

#define DEM_AR_RELEASE_MAJOR_VERSION        24u
#define DEM_AR_RELEASE_MINOR_VERSION        11u
#define DEM_AR_RELEASE_REVISION_VERSION     0u

/*============================================================================*
 *  Type definitions
 *============================================================================*/

/** \brief DEM event identifier type */
typedef uint16 Dem_EventIdType;

/**
 * \brief  DEM event status type
 */
typedef enum
{
    DEM_EVENT_STATUS_PASSED     = 0x00u,  /**< Event test passed */
    DEM_EVENT_STATUS_FAILED     = 0x01u,  /**< Event test failed */
    DEM_EVENT_STATUS_PREPASSED  = 0x02u,  /**< Event test pre-passed */
    DEM_EVENT_STATUS_PREFAILED  = 0x03u   /**< Event test pre-failed */
} Dem_EventStatusType;

/*============================================================================*
 *  Function declarations
 *============================================================================*/

/**
 * \brief  Set the status of a diagnostic event
 *
 * \param[in] EventId       Identification of an event by assigned EventId
 * \param[in] EventStatus   Status of the event to set
 *
 * \return    E_OK on success, E_NOT_OK on failure
 */
extern FUNC(Std_ReturnType, DEM_CODE) Dem_SetEventStatus(
    Dem_EventIdType     EventId,
    Dem_EventStatusType EventStatus
);

#endif /* DEM_H */
