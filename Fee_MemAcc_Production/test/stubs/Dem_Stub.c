/**
 * \file       Dem_Stub.c
 * \brief      DEM Stub Implementation -- Event Recording
 *
 * \details    Records all DEM events reported via Dem_SetEventStatus
 *             for test verification.
 *
 * \copyright  Copyright (c) 2024 -- All rights reserved.
 */

/*============================================================================*
 *  Includes
 *============================================================================*/

#include "Dem.h"
#include "Dem_Stub.h"

/*============================================================================*
 *  Internal state
 *============================================================================*/

/** \brief Recorded event buffer */
static Dem_Stub_EventType Dem_Stub_Events[DEM_STUB_MAX_EVENTS];

/** \brief Number of recorded events */
static uint32 Dem_Stub_EventCount = 0u;

/*============================================================================*
 *  Stub control APIs
 *============================================================================*/

void Dem_Stub_Reset(void)
{
    uint32 idx;
    Dem_Stub_EventCount = 0u;
    for (idx = 0u; idx < DEM_STUB_MAX_EVENTS; idx++)
    {
        Dem_Stub_Events[idx].EventId = 0u;
        Dem_Stub_Events[idx].EventStatus = DEM_EVENT_STATUS_PASSED;
    }
}

uint32 Dem_Stub_GetEventCount(void)
{
    return Dem_Stub_EventCount;
}

Dem_Stub_EventType Dem_Stub_GetEvent(uint32 Index)
{
    Dem_Stub_EventType emptyEvent;
    if (Index < Dem_Stub_EventCount)
    {
        return Dem_Stub_Events[Index];
    }
    emptyEvent.EventId = 0u;
    emptyEvent.EventStatus = DEM_EVENT_STATUS_PASSED;
    return emptyEvent;
}

boolean Dem_Stub_WasEventReported(Dem_EventIdType EventId,
                                   Dem_EventStatusType EventStatus)
{
    uint32 idx;
    for (idx = 0u; idx < Dem_Stub_EventCount; idx++)
    {
        if ((Dem_Stub_Events[idx].EventId == EventId) &&
            (Dem_Stub_Events[idx].EventStatus == EventStatus))
        {
            return TRUE;
        }
    }
    return FALSE;
}

/*============================================================================*
 *  DEM API implementation (stub)
 *============================================================================*/

FUNC(Std_ReturnType, DEM_CODE) Dem_SetEventStatus(
    Dem_EventIdType     EventId,
    Dem_EventStatusType EventStatus
)
{
    if (Dem_Stub_EventCount < DEM_STUB_MAX_EVENTS)
    {
        Dem_Stub_Events[Dem_Stub_EventCount].EventId = EventId;
        Dem_Stub_Events[Dem_Stub_EventCount].EventStatus = EventStatus;
        Dem_Stub_EventCount++;
    }
    return E_OK;
}
