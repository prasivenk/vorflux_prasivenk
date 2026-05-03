/**
 * \file       Fee_PBcfg.c
 * \brief      AUTOSAR Fee Module -- Post-Build Configuration
 *
 * \details    Post-build configuration instance for the Fee module.
 *             References the block configuration table from Fee_Cfg.c.
 *
 * \copyright  Copyright (c) 2024 -- All rights reserved.
 *             AUTOSAR R24-11 compliant.
 *
 * \autosar    AUTOSAR_SWS_FlashEEPROMEmulation
 * \version    2.0.0
 */

/*============================================================================*
 *  Includes
 *============================================================================*/

#include "Fee_Types.h"
#include "Fee_Cfg.h"
#include "Fee_PBcfg.h"

/*============================================================================*
 *  External references
 *============================================================================*/

/** \brief Block configuration table defined in Fee_Cfg.c */
extern CONST(Fee_BlockConfigType, FEE_CONST) Fee_BlockConfigTable[FEE_NUMBER_OF_BLOCKS];

/*============================================================================*
 *  Post-build configuration instance
 *============================================================================*/

#define FEE_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "Fee_MemMap.h"

/** \brief Post-build configuration */
CONST(Fee_ConfigType, FEE_CONST) Fee_Config =
{
    &Fee_BlockConfigTable[0],   /* BlockConfigTable */
    FEE_NUMBER_OF_BLOCKS,       /* NumberOfBlocks = 8 */
    FEE_VIRTUAL_PAGE_SIZE,      /* VirtualPageSize = 32 */
    0u,                         /* MemAccAreaId */
    FEE_NUMBER_OF_SECTORS,      /* NumberOfSectors = 4 */
    FEE_SECTOR_SIZE             /* SectorSize = 16384 */
};

#define FEE_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "Fee_MemMap.h"
