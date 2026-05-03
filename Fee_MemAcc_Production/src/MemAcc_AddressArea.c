/**
 * \file       MemAcc_AddressArea.c
 * \brief      AUTOSAR MemAcc Module -- Address Area Management
 *
 * \details    Implements address validation, translation, and area lookup
 *             functions for the MemAcc module.
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

#include "MemAcc.h"
#include "MemAcc_Cfg.h"

/*============================================================================*
 *  Function implementations
 *============================================================================*/

#define MEMACC_START_SEC_CODE
#include "MemAcc_MemMap.h"

/**
 * \brief  Validate address and length within area bounds
 *
 * \param[in] AreaId    Address area identifier
 * \param[in] Address   Logical address within the area
 * \param[in] Length    Number of bytes
 *
 * \return    TRUE if valid, FALSE otherwise
 */
FUNC(boolean, MEMACC_CODE) MemAcc_Internal_ValidateAddress(
    MemAcc_AddressAreaIdType AreaId,
    MemAcc_AddressType Address,
    MemAcc_LengthType Length
)
{
    MemAcc_LengthType areaLength;
    MemAcc_AddressType endAddress;

    if (MemAcc_ConfigPtr == NULL_PTR)
    {
        return FALSE;
    }

    if (AreaId >= MemAcc_ConfigPtr->NumberOfAreas)
    {
        return FALSE;
    }

    areaLength = MemAcc_ConfigPtr->AddressAreas[AreaId].Length;

    /* Check for overflow */
    endAddress = Address + Length;
    if (endAddress < Address)
    {
        return FALSE;
    }

    if (endAddress > areaLength)
    {
        return FALSE;
    }

    return TRUE;
}

/**
 * \brief  Translate logical address to physical address
 *
 * \param[in] AreaId          Address area identifier
 * \param[in] LogicalAddress  Logical address within the area
 *
 * \return    Physical address
 */
FUNC(MemAcc_AddressType, MEMACC_CODE) MemAcc_Internal_TranslateAddress(
    MemAcc_AddressAreaIdType AreaId,
    MemAcc_AddressType LogicalAddress
)
{
    MemAcc_AddressType baseAddress;

    if (MemAcc_ConfigPtr == NULL_PTR)
    {
        return 0u;
    }

    if (AreaId >= MemAcc_ConfigPtr->NumberOfAreas)
    {
        return 0u;
    }

    baseAddress = MemAcc_ConfigPtr->AddressAreas[AreaId].StartAddress;

    return baseAddress + LogicalAddress;
}

/**
 * \brief  Check if area ID is valid
 *
 * \param[in] AreaId  Address area identifier
 *
 * \return    TRUE if area exists, FALSE otherwise
 */
FUNC(boolean, MEMACC_CODE) MemAcc_Internal_FindArea(
    MemAcc_AddressAreaIdType AreaId
)
{
    if (MemAcc_ConfigPtr == NULL_PTR)
    {
        return FALSE;
    }

    if (AreaId >= MemAcc_ConfigPtr->NumberOfAreas)
    {
        return FALSE;
    }

    return TRUE;
}

#define MEMACC_STOP_SEC_CODE
#include "MemAcc_MemMap.h"
