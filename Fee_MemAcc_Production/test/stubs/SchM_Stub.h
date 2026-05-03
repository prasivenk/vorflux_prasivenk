/**
 * \file       SchM_Stub.h
 * \brief      SchM Stub Header -- Exclusive Area Recording
 *
 * \details    Provides control APIs for the SchM stub to record
 *             enter/exit calls for exclusive areas.
 *
 * \copyright  Copyright (c) 2024 -- All rights reserved.
 */

#ifndef SCHM_STUB_H
#define SCHM_STUB_H

/*============================================================================*
 *  Includes
 *============================================================================*/

#include "Std_Types.h"

/*============================================================================*
 *  Stub control APIs
 *============================================================================*/

/** \brief Reset the SchM stub (clear counters) */
extern void SchM_Stub_Reset(void);

/** \brief Get the number of SchM_Enter calls */
extern uint32 SchM_Stub_GetEnterCount(void);

/** \brief Get the number of SchM_Exit calls */
extern uint32 SchM_Stub_GetExitCount(void);

#endif /* SCHM_STUB_H */
