/**
 * \file       MemAcc_PBcfg.c
 * \brief      AUTOSAR MemAcc Module -- Post-Build Configuration
 *
 * \details    Post-build configuration instance for the MemAcc module.
 *
 * \copyright  Copyright (c) 2024 -- All rights reserved.
 *             AUTOSAR R24-11 compliant.
 *
 * \autosar    AUTOSAR_SWS_MemoryAccess
 * \version    24.11.0
 */

/*============================================================================*
 *  Includes
 *============================================================================*/

#include "MemAcc_Types.h"
#include "MemAcc_Cfg.h"
#include "MemAcc_PBcfg.h"

/*============================================================================*
 *  External references
 *============================================================================*/

/** \brief Address area config array defined in MemAcc_Cfg.c */
extern CONST(MemAcc_AddressAreaConfigType, MEMACC_CONST) MemAcc_AddressAreaConfigs[MEMACC_NUMBER_OF_ADDRESS_AREAS];

/*============================================================================*
 *  Post-build configuration instance
 *============================================================================*/

#define MEMACC_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemAcc_MemMap.h"

/** \brief Post-build configuration */
CONST(MemAcc_ConfigType, MEMACC_CONST) MemAcc_Config =
{
    &MemAcc_AddressAreaConfigs[0],     /* AddressAreas */
    MEMACC_NUMBER_OF_ADDRESS_AREAS     /* NumberOfAreas */
};

#define MEMACC_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemAcc_MemMap.h"
