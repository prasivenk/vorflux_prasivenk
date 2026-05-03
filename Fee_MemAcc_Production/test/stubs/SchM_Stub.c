/**
 * \file       SchM_Stub.c
 * \brief      SchM Stub Implementation -- Exclusive Area Recording
 *
 * \details    Records SchM enter/exit calls for exclusive areas.
 *             Note: With FEE_UNIT_TEST defined, the SchM macros in
 *             SchM_MemAcc.h expand to empty. This stub is available
 *             for additional tracking if needed.
 *
 * \copyright  Copyright (c) 2024 -- All rights reserved.
 */

/*============================================================================*
 *  Includes
 *============================================================================*/

#include "SchM_Stub.h"

/*============================================================================*
 *  Internal state
 *============================================================================*/

/** \brief Enter call counter */
static uint32 SchM_Stub_EnterCount = 0u;

/** \brief Exit call counter */
static uint32 SchM_Stub_ExitCount = 0u;

/*============================================================================*
 *  Stub control APIs
 *============================================================================*/

void SchM_Stub_Reset(void)
{
    SchM_Stub_EnterCount = 0u;
    SchM_Stub_ExitCount = 0u;
}

uint32 SchM_Stub_GetEnterCount(void)
{
    return SchM_Stub_EnterCount;
}

uint32 SchM_Stub_GetExitCount(void)
{
    return SchM_Stub_ExitCount;
}
