/**
 * \file       Det_Stub.c
 * \brief      DET Stub Implementation -- Error Recording
 *
 * \details    Records all DET errors reported via Det_ReportError
 *             for test verification.
 *
 * \copyright  Copyright (c) 2024 -- All rights reserved.
 */

/*============================================================================*
 *  Includes
 *============================================================================*/

#include "Det.h"
#include "Det_Stub.h"

/*============================================================================*
 *  Internal state
 *============================================================================*/

/** \brief Recorded error buffer */
static Det_Stub_ErrorType Det_Stub_Errors[DET_STUB_MAX_ERRORS];

/** \brief Number of recorded errors */
static uint32 Det_Stub_ErrorCount = 0u;

/** \brief Runtime error buffer */
static Det_Stub_ErrorType Det_Stub_RuntimeErrors[DET_STUB_MAX_ERRORS];

/** \brief Number of recorded runtime errors */
static uint32 Det_Stub_RuntimeErrorCount = 0u;

/*============================================================================*
 *  Stub control APIs
 *============================================================================*/

void Det_Stub_Reset(void)
{
    uint32 idx;
    Det_Stub_ErrorCount = 0u;
    Det_Stub_RuntimeErrorCount = 0u;
    for (idx = 0u; idx < DET_STUB_MAX_ERRORS; idx++)
    {
        Det_Stub_Errors[idx].ModuleId = 0u;
        Det_Stub_Errors[idx].InstanceId = 0u;
        Det_Stub_Errors[idx].ApiId = 0u;
        Det_Stub_Errors[idx].ErrorId = 0u;
        Det_Stub_RuntimeErrors[idx].ModuleId = 0u;
        Det_Stub_RuntimeErrors[idx].InstanceId = 0u;
        Det_Stub_RuntimeErrors[idx].ApiId = 0u;
        Det_Stub_RuntimeErrors[idx].ErrorId = 0u;
    }
}

uint32 Det_Stub_GetErrorCount(void)
{
    return Det_Stub_ErrorCount;
}

Det_Stub_ErrorType Det_Stub_GetError(uint32 Index)
{
    Det_Stub_ErrorType emptyError;
    if (Index < Det_Stub_ErrorCount)
    {
        return Det_Stub_Errors[Index];
    }
    emptyError.ModuleId = 0u;
    emptyError.InstanceId = 0u;
    emptyError.ApiId = 0u;
    emptyError.ErrorId = 0u;
    return emptyError;
}

boolean Det_Stub_WasErrorReported(uint16 ModuleId, uint8 ApiId, uint8 ErrorId)
{
    uint32 idx;
    for (idx = 0u; idx < Det_Stub_ErrorCount; idx++)
    {
        if ((Det_Stub_Errors[idx].ModuleId == ModuleId) &&
            (Det_Stub_Errors[idx].ApiId == ApiId) &&
            (Det_Stub_Errors[idx].ErrorId == ErrorId))
        {
            return TRUE;
        }
    }
    return FALSE;
}

/*============================================================================*
 *  DET API implementations (stub)
 *============================================================================*/

FUNC(Std_ReturnType, DET_CODE) Det_ReportError(
    uint16 ModuleId,
    uint8  InstanceId,
    uint8  ApiId,
    uint8  ErrorId
)
{
    if (Det_Stub_ErrorCount < DET_STUB_MAX_ERRORS)
    {
        Det_Stub_Errors[Det_Stub_ErrorCount].ModuleId = ModuleId;
        Det_Stub_Errors[Det_Stub_ErrorCount].InstanceId = InstanceId;
        Det_Stub_Errors[Det_Stub_ErrorCount].ApiId = ApiId;
        Det_Stub_Errors[Det_Stub_ErrorCount].ErrorId = ErrorId;
        Det_Stub_ErrorCount++;
    }
    return E_OK;
}

uint32 Det_Stub_GetRuntimeErrorCount(void)
{
    return Det_Stub_RuntimeErrorCount;
}

boolean Det_Stub_WasRuntimeErrorReported(uint16 ModuleId, uint8 ApiId, uint8 ErrorId)
{
    uint32 idx;
    for (idx = 0u; idx < Det_Stub_RuntimeErrorCount; idx++)
    {
        if ((Det_Stub_RuntimeErrors[idx].ModuleId == ModuleId) &&
            (Det_Stub_RuntimeErrors[idx].ApiId == ApiId) &&
            (Det_Stub_RuntimeErrors[idx].ErrorId == ErrorId))
        {
            return TRUE;
        }
    }
    return FALSE;
}

FUNC(Std_ReturnType, DET_CODE) Det_ReportRuntimeError(
    uint16 ModuleId,
    uint8  InstanceId,
    uint8  ApiId,
    uint8  ErrorId
)
{
    if (Det_Stub_RuntimeErrorCount < DET_STUB_MAX_ERRORS)
    {
        Det_Stub_RuntimeErrors[Det_Stub_RuntimeErrorCount].ModuleId = ModuleId;
        Det_Stub_RuntimeErrors[Det_Stub_RuntimeErrorCount].InstanceId = InstanceId;
        Det_Stub_RuntimeErrors[Det_Stub_RuntimeErrorCount].ApiId = ApiId;
        Det_Stub_RuntimeErrors[Det_Stub_RuntimeErrorCount].ErrorId = ErrorId;
        Det_Stub_RuntimeErrorCount++;
    }
    return E_OK;
}

FUNC(Std_ReturnType, DET_CODE) Det_ReportTransientFault(
    uint16 ModuleId,
    uint8  InstanceId,
    uint8  ApiId,
    uint8  FaultId
)
{
    (void)ModuleId;
    (void)InstanceId;
    (void)ApiId;
    (void)FaultId;
    return E_OK;
}
