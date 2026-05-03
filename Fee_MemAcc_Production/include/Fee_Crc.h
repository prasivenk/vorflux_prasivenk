/**
 * \file       Fee_Crc.h
 * \brief      AUTOSAR Fee Module -- CRC-16 CCITT Internal Interface
 *
 * \details    Provides CRC-16 CCITT calculation functions for Fee module
 *             internal use. Polynomial 0x1021, bit-by-bit implementation.
 *
 * \copyright  Copyright (c) 2024 -- All rights reserved.
 *             AUTOSAR R24-11 compliant.
 *
 * \autosar    AUTOSAR_SWS_FlashEEPROMEmulation
 * \version    2.0.0
 */

#ifndef FEE_CRC_H
#define FEE_CRC_H

/*============================================================================*
 *  Includes
 *============================================================================*/

#include "Fee_Types.h"

/*============================================================================*
 *  Function declarations
 *============================================================================*/

/**
 * \brief  Calculate CRC-16 CCITT over a data buffer
 *
 * \param[in] DataPtr     Pointer to data buffer
 * \param[in] Length      Number of bytes to process
 * \param[in] StartValue  Initial CRC value
 *
 * \return    Calculated CRC-16 value
 */
extern FUNC(uint16, FEE_CODE) Fee_Crc_Calculate(
    P2CONST(uint8, AUTOMATIC, FEE_APPL_CONST) DataPtr,
    uint32 Length,
    uint16 StartValue
);

/**
 * \brief  Calculate CRC-16 CCITT over a data buffer with default start value
 *
 * \param[in] DataPtr  Pointer to data buffer
 * \param[in] Length   Number of bytes to process
 *
 * \return    Calculated CRC-16 value (start value = 0xFFFF)
 */
extern FUNC(uint16, FEE_CODE) Fee_Crc_CalculateBlock(
    P2CONST(uint8, AUTOMATIC, FEE_APPL_CONST) DataPtr,
    uint32 Length
);

#endif /* FEE_CRC_H */
