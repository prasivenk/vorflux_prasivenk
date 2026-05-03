/**
 * \file       MemAcc_PBcfg.h
 * \brief      AUTOSAR MemAcc Module -- Post-Build Configuration Header
 *
 * \details    Extern declaration for the post-build configuration instance.
 *
 * \copyright  Copyright (c) 2024 -- All rights reserved.
 *             AUTOSAR R24-11 compliant.
 *
 * \autosar    AUTOSAR_SWS_MemoryAccess
 * \version    24.11.0
 */

#ifndef MEMACC_PBCFG_H
#define MEMACC_PBCFG_H

/*============================================================================*
 *  Includes
 *============================================================================*/

#include "MemAcc_Types.h"

/*============================================================================*
 *  External declarations
 *============================================================================*/

/** \brief Post-build configuration instance */
extern CONST(MemAcc_ConfigType, MEMACC_CONST) MemAcc_Config;

#endif /* MEMACC_PBCFG_H */
