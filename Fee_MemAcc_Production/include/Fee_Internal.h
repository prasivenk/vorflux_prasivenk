/**
 * \file       Fee_Internal.h
 * \brief      AUTOSAR Fee Module -- Shared Internal Utilities
 *
 * \details    Common macros and internal function declarations shared
 *             across Fee translation units.
 *
 * \copyright  Copyright (c) 2024 -- All rights reserved.
 *             AUTOSAR R24-11 compliant.
 *
 * \autosar    AUTOSAR_SWS_FlashEEPROMEmulation
 * \version    2.0.0
 */

#ifndef FEE_INTERNAL_H
#define FEE_INTERNAL_H

/*============================================================================*
 *  Includes
 *============================================================================*/

#include "Fee_Types.h"
#include "Fee_Cfg.h"

/*============================================================================*
 *  Alignment macro (shared across Fee modules)
 *============================================================================*/

/** \brief Align value up to the next multiple of alignment (power-of-2) */
#define FEE_ALIGN_UP(value, alignment)  \
    (((uint32)(value) + ((uint32)(alignment) - 1u)) & ~((uint32)(alignment) - 1u))

/*============================================================================*
 *  Shared internal function declarations
 *============================================================================*/

/**
 * \brief  Find block index by block number in the configuration table
 *
 * \param[in] BlockNumber  Block number to look up
 *
 * \return  Block index, or FEE_BLOCK_INDEX_INVALID if not found
 */
extern FUNC(uint16, FEE_CODE) Fee_Internal_FindBlockIndex(uint16 BlockNumber);

#endif /* FEE_INTERNAL_H */
