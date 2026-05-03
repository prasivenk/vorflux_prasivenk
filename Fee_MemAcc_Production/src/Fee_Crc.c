/**
 * \file       Fee_Crc.c
 * \brief      AUTOSAR Fee Module -- CRC-16 CCITT Implementation
 *
 * \details    Bit-by-bit CRC-16 CCITT calculation using polynomial 0x1021.
 *             No lookup table -- suitable for memory-constrained targets.
 *             Test vector: "123456789" -> 0x29B1
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

#include "Fee_Crc.h"

/*============================================================================*
 *  Code Section
 *============================================================================*/

#define FEE_START_SEC_CODE
#include "Fee_MemMap.h"

/**
 * \brief  Calculate CRC-16 CCITT over a data buffer (bit-by-bit)
 *
 * Polynomial: 0x1021 (x^16 + x^12 + x^5 + 1)
 * Processing: MSB-first (big-endian bit order)
 *
 * \param[in] DataPtr     Pointer to data buffer (NULL allowed if Length == 0)
 * \param[in] Length      Number of bytes to process
 * \param[in] StartValue  Initial CRC value
 *
 * \return    Calculated CRC-16 value
 */
FUNC(uint16, FEE_CODE) Fee_Crc_Calculate(
    P2CONST(uint8, AUTOMATIC, FEE_APPL_CONST) DataPtr,
    uint32 Length,
    uint16 StartValue)
{
    VAR(uint16, AUTOMATIC) crc = StartValue;
    VAR(uint32, AUTOMATIC) byteIdx;
    VAR(uint8, AUTOMATIC)  bitIdx;

    for (byteIdx = 0u; byteIdx < Length; byteIdx++)
    {
        crc ^= ((uint16)DataPtr[byteIdx] << 8u);

        for (bitIdx = 0u; bitIdx < 8u; bitIdx++)
        {
            if ((crc & 0x8000u) != 0u)
            {
                crc = (uint16)((uint16)(crc << 1u) ^ FEE_CRC_POLYNOMIAL);
            }
            else
            {
                crc = (uint16)(crc << 1u);
            }
        }
    }

    return crc;
}

/**
 * \brief  Convenience wrapper: CRC-16 CCITT with default start value 0xFFFF
 *
 * \param[in] DataPtr  Pointer to data buffer
 * \param[in] Length   Number of bytes to process
 *
 * \return    Calculated CRC-16 value
 */
FUNC(uint16, FEE_CODE) Fee_Crc_CalculateBlock(
    P2CONST(uint8, AUTOMATIC, FEE_APPL_CONST) DataPtr,
    uint32 Length)
{
    return Fee_Crc_Calculate(DataPtr, Length, FEE_CRC_INITIAL);
}

#define FEE_STOP_SEC_CODE
#include "Fee_MemMap.h"
