/**
 * \file       Fee_Cfg.c
 * \brief      AUTOSAR Fee Module -- Static Configuration Data
 *
 * \details    Block configuration table and static configuration for the
 *             Fee module. Contains 8 configured blocks with varying sizes
 *             and immediate-data properties.
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

/*============================================================================*
 *  Block configuration table
 *============================================================================*/

#define FEE_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "Fee_MemMap.h"

/**
 * \brief  Block configuration array
 *
 * \details  8 blocks configured:
 *           - Block 1: 32 bytes, normal
 *           - Block 2: 64 bytes, normal
 *           - Block 3: 128 bytes, normal
 *           - Block 4: 256 bytes, normal
 *           - Block 5: 16 bytes, immediate data
 *           - Block 6: 8 bytes, immediate data
 *           - Block 7: 512 bytes, normal
 *           - Block 8: 64 bytes, normal
 */
CONST(Fee_BlockConfigType, FEE_CONST) Fee_BlockConfigTable[FEE_NUMBER_OF_BLOCKS] =
{
    /* Block 1: 32 bytes, normal data */
    {
        1u,     /* BlockNumber */
        32u,    /* BlockSize */
        FALSE,  /* ImmediateData */
        0u      /* NumberOfWriteCycles (0 = unlimited) */
    },
    /* Block 2: 64 bytes, normal data */
    {
        2u,     /* BlockNumber */
        64u,    /* BlockSize */
        FALSE,  /* ImmediateData */
        0u      /* NumberOfWriteCycles */
    },
    /* Block 3: 128 bytes, normal data */
    {
        3u,     /* BlockNumber */
        128u,   /* BlockSize */
        FALSE,  /* ImmediateData */
        0u      /* NumberOfWriteCycles */
    },
    /* Block 4: 256 bytes, normal data */
    {
        4u,     /* BlockNumber */
        256u,   /* BlockSize */
        FALSE,  /* ImmediateData */
        0u      /* NumberOfWriteCycles */
    },
    /* Block 5: 16 bytes, immediate data */
    {
        5u,     /* BlockNumber */
        16u,    /* BlockSize */
        TRUE,   /* ImmediateData */
        0u      /* NumberOfWriteCycles */
    },
    /* Block 6: 8 bytes, immediate data */
    {
        6u,     /* BlockNumber */
        8u,     /* BlockSize */
        TRUE,   /* ImmediateData */
        0u      /* NumberOfWriteCycles */
    },
    /* Block 7: 512 bytes, normal data */
    {
        7u,     /* BlockNumber */
        512u,   /* BlockSize */
        FALSE,  /* ImmediateData */
        0u      /* NumberOfWriteCycles */
    },
    /* Block 8: 64 bytes, normal data */
    {
        8u,     /* BlockNumber */
        64u,    /* BlockSize */
        FALSE,  /* ImmediateData */
        0u      /* NumberOfWriteCycles */
    }
};

#define FEE_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "Fee_MemMap.h"
