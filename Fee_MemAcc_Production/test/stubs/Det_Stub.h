/**
 * \file       Det_Stub.h
 * \brief      DET Stub Header -- Error Recording
 *
 * \details    Provides control APIs for the DET stub to record and
 *             query reported development errors.
 *
 * \copyright  Copyright (c) 2024 -- All rights reserved.
 */

#ifndef DET_STUB_H
#define DET_STUB_H

/*============================================================================*
 *  Includes
 *============================================================================*/

#include "Std_Types.h"

/*============================================================================*
 *  Configuration
 *============================================================================*/

/** \brief Maximum number of DET errors to record */
#define DET_STUB_MAX_ERRORS     32u

/*============================================================================*
 *  Error record type
 *============================================================================*/

/** \brief Recorded DET error */
typedef struct
{
    uint16 ModuleId;    /**< Module ID */
    uint8  InstanceId;  /**< Instance ID */
    uint8  ApiId;       /**< API service ID */
    uint8  ErrorId;     /**< Error code */
} Det_Stub_ErrorType;

/*============================================================================*
 *  Stub control APIs
 *============================================================================*/

/** \brief Reset the DET stub (clear all recorded errors) */
extern void Det_Stub_Reset(void);

/** \brief Get the number of recorded errors */
extern uint32 Det_Stub_GetErrorCount(void);

/** \brief Get a recorded error by index */
extern Det_Stub_ErrorType Det_Stub_GetError(uint32 Index);

/** \brief Check if a specific error was reported */
extern boolean Det_Stub_WasErrorReported(uint16 ModuleId, uint8 ApiId, uint8 ErrorId);

/** \brief Get the number of recorded runtime errors */
extern uint32 Det_Stub_GetRuntimeErrorCount(void);

/** \brief Check if a specific runtime error was reported */
extern boolean Det_Stub_WasRuntimeErrorReported(uint16 ModuleId, uint8 ApiId, uint8 ErrorId);

#endif /* DET_STUB_H */
