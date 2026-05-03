/**
 * \file       Det.h
 * \brief      AUTOSAR Default Error Tracer (DET) -- Placeholder Interface
 *
 * \details    Provides DET reporting function declarations per
 *             AUTOSAR R24-11 SWS Default Error Tracer.
 *             This is a placeholder; the actual DET module is provided
 *             by the BSW basic software package.
 *
 * \copyright  Copyright (c) 2024 -- All rights reserved.
 *             AUTOSAR R24-11 compliant.
 *
 * \autosar    AUTOSAR_SWS_DefaultErrorTracer
 * \version    24.11.0
 */

#ifndef DET_H
#define DET_H

/*============================================================================*
 *  Includes
 *============================================================================*/

#include "Std_Types.h"

/*============================================================================*
 *  Version information
 *============================================================================*/

#define DET_VENDOR_ID                       0xFFFFu
#define DET_MODULE_ID                       15u

#define DET_AR_RELEASE_MAJOR_VERSION        24u
#define DET_AR_RELEASE_MINOR_VERSION        11u
#define DET_AR_RELEASE_REVISION_VERSION     0u

/*============================================================================*
 *  Function declarations
 *============================================================================*/

/**
 * \brief  Report a development error
 *
 * \param[in] ModuleId    Module ID of the calling module
 * \param[in] InstanceId  Instance ID of the calling module
 * \param[in] ApiId       API ID of the calling function
 * \param[in] ErrorId     Error ID to report
 *
 * \return    E_OK on success
 */
extern FUNC(Std_ReturnType, DET_CODE) Det_ReportError(
    uint16 ModuleId,
    uint8  InstanceId,
    uint8  ApiId,
    uint8  ErrorId
);

/**
 * \brief  Report a runtime error
 *
 * \param[in] ModuleId    Module ID of the calling module
 * \param[in] InstanceId  Instance ID of the calling module
 * \param[in] ApiId       API ID of the calling function
 * \param[in] ErrorId     Error ID to report
 *
 * \return    E_OK on success
 */
extern FUNC(Std_ReturnType, DET_CODE) Det_ReportRuntimeError(
    uint16 ModuleId,
    uint8  InstanceId,
    uint8  ApiId,
    uint8  ErrorId
);

/**
 * \brief  Report a transient fault
 *
 * \param[in] ModuleId    Module ID of the calling module
 * \param[in] InstanceId  Instance ID of the calling module
 * \param[in] ApiId       API ID of the calling function
 * \param[in] FaultId     Fault ID to report
 *
 * \return    E_OK on success
 */
extern FUNC(Std_ReturnType, DET_CODE) Det_ReportTransientFault(
    uint16 ModuleId,
    uint8  InstanceId,
    uint8  ApiId,
    uint8  FaultId
);

#endif /* DET_H */
