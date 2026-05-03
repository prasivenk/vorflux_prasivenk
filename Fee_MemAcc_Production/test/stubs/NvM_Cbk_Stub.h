/**
 * \file       NvM_Cbk_Stub.h
 * \brief      NvM Callback Stub Header -- Notification Recording
 *
 * \details    Provides stub implementations for NvM_JobEndNotification
 *             and NvM_JobErrorNotification, along with query APIs for
 *             test verification.
 *
 * \copyright  Copyright (c) 2024 -- All rights reserved.
 */

#ifndef NVM_CBK_STUB_H
#define NVM_CBK_STUB_H

/*============================================================================*
 *  Includes
 *============================================================================*/

#include "Std_Types.h"

/*============================================================================*
 *  NvM callback declarations
 *============================================================================*/

/** \brief Job end notification callback */
extern void NvM_JobEndNotification(void);

/** \brief Job error notification callback */
extern void NvM_JobErrorNotification(void);

/*============================================================================*
 *  Stub control APIs
 *============================================================================*/

/** \brief Get number of job end notifications */
extern uint32 NvM_Cbk_Stub_GetEndCount(void);

/** \brief Get number of job error notifications */
extern uint32 NvM_Cbk_Stub_GetErrorCount(void);

/** \brief Reset the stub counters */
extern void NvM_Cbk_Stub_Reset(void);

#endif /* NVM_CBK_STUB_H */
