/**
 * \file       NvM_Cbk_Stub.c
 * \brief      NvM Callback Stub Implementation -- Notification Recording
 *
 * \details    Records NvM notification calls for test verification.
 *
 * \copyright  Copyright (c) 2024 -- All rights reserved.
 */

/*============================================================================*
 *  Includes
 *============================================================================*/

#include "NvM_Cbk_Stub.h"

/*============================================================================*
 *  Internal state
 *============================================================================*/

/** \brief Job end notification counter */
static uint32 NvM_Cbk_Stub_EndCount = 0u;

/** \brief Job error notification counter */
static uint32 NvM_Cbk_Stub_ErrorCount = 0u;

/*============================================================================*
 *  NvM callback implementations
 *============================================================================*/

void NvM_JobEndNotification(void)
{
    NvM_Cbk_Stub_EndCount++;
}

void NvM_JobErrorNotification(void)
{
    NvM_Cbk_Stub_ErrorCount++;
}

/*============================================================================*
 *  Stub control APIs
 *============================================================================*/

uint32 NvM_Cbk_Stub_GetEndCount(void)
{
    return NvM_Cbk_Stub_EndCount;
}

uint32 NvM_Cbk_Stub_GetErrorCount(void)
{
    return NvM_Cbk_Stub_ErrorCount;
}

void NvM_Cbk_Stub_Reset(void)
{
    NvM_Cbk_Stub_EndCount = 0u;
    NvM_Cbk_Stub_ErrorCount = 0u;
}
