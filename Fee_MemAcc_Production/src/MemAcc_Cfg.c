/**
 * \file       MemAcc_Cfg.c
 * \brief      AUTOSAR MemAcc Module -- Static Configuration
 *
 * \details    Static configuration with two address areas for the
 *             MemAcc module (Fee DFLASH + bootloader DFLASH).
 *
 *             <PLACEHOLDER_REQUIRED> Actual TC3xx DFLASH base addresses
 *             depend on MCU variant. Verify against MCU datasheet.
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

/*============================================================================*
 *  Static address area configuration
 *============================================================================*/

#define MEMACC_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemAcc_MemMap.h"

/** \brief Address area configuration array */
CONST(MemAcc_AddressAreaConfigType, MEMACC_CONST) MemAcc_AddressAreaConfigs[MEMACC_NUMBER_OF_ADDRESS_AREAS] =
{
    /* Area 0: Fee DFLASH partition */
    {
        0u,                         /* AreaId */
        MEMACC_AREA0_START_ADDRESS, /* StartAddress */
        MEMACC_AREA0_LENGTH,        /* Length */
        MEMACC_AREA0_SECTOR_SIZE,   /* SectorSize */
        MEMACC_AREA0_PAGE_SIZE      /* PageSize */
    },
    /* Area 1: Bootloader DFLASH partition */
    {
        1u,                         /* AreaId */
        MEMACC_AREA1_START_ADDRESS, /* StartAddress */
        MEMACC_AREA1_LENGTH,        /* Length */
        MEMACC_AREA1_SECTOR_SIZE,   /* SectorSize */
        MEMACC_AREA1_PAGE_SIZE      /* PageSize */
    }
};

#define MEMACC_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemAcc_MemMap.h"
