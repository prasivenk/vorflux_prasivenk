/**
 * \file       Dem_Stub.h
 * \brief      DEM Stub Header -- Event Recording
 *
 * \details    Provides control APIs for the DEM stub to record and
 *             query reported diagnostic events.
 *
 * \copyright  Copyright (c) 2024 -- All rights reserved.
 */

#ifndef DEM_STUB_H
#define DEM_STUB_H

/*============================================================================*
 *  Includes
 *============================================================================*/

#include "Std_Types.h"
#include "Dem.h"

/*============================================================================*
 *  Configuration
 *============================================================================*/

/** \brief Maximum number of DEM events to record */
#define DEM_STUB_MAX_EVENTS     32u

/*============================================================================*
 *  Event record type
 *============================================================================*/

/** \brief Recorded DEM event */
typedef struct
{
    Dem_EventIdType     EventId;      /**< Event identifier */
    Dem_EventStatusType EventStatus;  /**< Event status */
} Dem_Stub_EventType;

/*============================================================================*
 *  Stub control APIs
 *============================================================================*/

/** \brief Reset the DEM stub (clear all recorded events) */
extern void Dem_Stub_Reset(void);

/** \brief Get the number of recorded events */
extern uint32 Dem_Stub_GetEventCount(void);

/** \brief Get a recorded event by index */
extern Dem_Stub_EventType Dem_Stub_GetEvent(uint32 Index);

/** \brief Check if a specific event was reported */
extern boolean Dem_Stub_WasEventReported(Dem_EventIdType EventId,
                                          Dem_EventStatusType EventStatus);

#endif /* DEM_STUB_H */
