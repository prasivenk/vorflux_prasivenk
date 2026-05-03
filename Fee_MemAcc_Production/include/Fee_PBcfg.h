/**
 * \file       Fee_PBcfg.h
 * \brief      AUTOSAR Fee Module -- Post-Build Configuration Header
 *
 * \details    Extern declaration of the post-build configuration instance
 *             for the Fee module.
 *
 * \copyright  Copyright (c) 2024 -- All rights reserved.
 *             AUTOSAR R24-11 compliant.
 *
 * \autosar    AUTOSAR_SWS_FlashEEPROMEmulation
 * \version    2.0.0
 */

#ifndef FEE_PBCFG_H
#define FEE_PBCFG_H

/*============================================================================*
 *  Includes
 *============================================================================*/

#include "Fee_Types.h"

/*============================================================================*
 *  Extern declarations
 *============================================================================*/

/** \brief Post-build configuration instance */
extern CONST(Fee_ConfigType, FEE_CONST) Fee_Config;

#endif /* FEE_PBCFG_H */
