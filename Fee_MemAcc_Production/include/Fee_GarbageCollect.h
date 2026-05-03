/**
 * \file       Fee_GarbageCollect.h
 * \brief      AUTOSAR Fee Module -- Garbage Collection Internal Interface
 *
 * \details    Declares the garbage collection functions for the Fee module.
 *             Implements interruptible multi-cycle GC with immediate-job
 *             preemption support.
 *
 * \copyright  Copyright (c) 2024 -- All rights reserved.
 *             AUTOSAR R24-11 compliant.
 *
 * \autosar    AUTOSAR_SWS_FlashEEPROMEmulation
 * \version    2.0.0
 */

#ifndef FEE_GARBAGECOLLECT_H
#define FEE_GARBAGECOLLECT_H

/*============================================================================*
 *  Includes
 *============================================================================*/

#include "Fee_Types.h"

/*============================================================================*
 *  GC result type
 *============================================================================*/

/** \brief GC process result */
typedef enum
{
    FEE_GC_IN_PROGRESS = 0,  /**< GC still in progress */
    FEE_GC_COMPLETE    = 1,  /**< GC completed */
    FEE_GC_ERROR       = 2   /**< GC encountered an error */
} Fee_GcResultType;

/*============================================================================*
 *  Function declarations
 *============================================================================*/

/**
 * \brief  Initialize the garbage collection module
 *
 * \details Sets the internal GC state to IDLE.
 */
extern FUNC(void, FEE_CODE) Fee_GarbageCollect_Init(void);

/**
 * \brief  Trigger garbage collection
 *
 * \details Selects source sector (fullest ACTIVE/FULL sector with valid
 *          blocks) and target sector (ERASED sector with lowest EraseCount
 *          for wear leveling). Transitions to SELECT_SOURCE.
 *
 * \return  E_OK if GC was triggered, E_NOT_OK if no valid source/target
 */
extern FUNC(Std_ReturnType, FEE_CODE) Fee_GarbageCollect_Trigger(void);

/**
 * \brief  Process one step of garbage collection
 *
 * \details Performs ONE state transition per call. Returns the current
 *          GC result: IN_PROGRESS, COMPLETE, or ERROR.
 *
 * \return  GC result (IN_PROGRESS, COMPLETE, or ERROR)
 */
extern FUNC(Fee_GcResultType, FEE_CODE) Fee_GarbageCollect_Process(void);

/**
 * \brief  Suspend garbage collection for immediate job preemption
 *
 * \details Sets the suspended flag. GC will return IN_PROGRESS without
 *          advancing state while suspended.
 */
extern FUNC(void, FEE_CODE) Fee_GarbageCollect_Suspend(void);

/**
 * \brief  Resume garbage collection after suspension
 *
 * \details Clears the suspended flag. GC will continue from saved state.
 */
extern FUNC(void, FEE_CODE) Fee_GarbageCollect_Resume(void);

/**
 * \brief  Check if garbage collection is active
 *
 * \return  TRUE if GC is active (state != IDLE), FALSE otherwise
 */
extern FUNC(boolean, FEE_CODE) Fee_GarbageCollect_IsActive(void);

#endif /* FEE_GARBAGECOLLECT_H */
