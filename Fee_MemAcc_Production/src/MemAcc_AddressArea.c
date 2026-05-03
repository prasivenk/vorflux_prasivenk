/**
 * \file       MemAcc_AddressArea.c
 * \brief      AUTOSAR MemAcc Module -- Address Area Management
 *
 * \details    Implements address validation, translation, and area lookup
 *             functions for the MemAcc module. Area IDs are mapped to
 *             configuration indices via the configured AreaId field,
 *             supporting non-dense / non-zero-based area identifiers.
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
#include "MemAcc_Internal.h"
#include "MemAcc_Cfg.h"

/*============================================================================*
 *  Function implementations
 *============================================================================*/

#define MEMACC_START_SEC_CODE
#include "MemAcc_MemMap.h"

/**
 * \brief  Find address area config index by area ID
 *
 * \details Maps the configured AreaId to an internal config array index.
 *          Supports non-dense and non-zero-based area identifiers by
 *          searching the AddressAreas[i].AreaId field.
 *
 * \param[in]  AreaId    Address area identifier
 * \param[out] OutIndex  Pointer to store the resolved config index (may be NULL)
 *
 * \return    TRUE if area exists, FALSE otherwise
 */
FUNC(boolean, MEMACC_CODE) MemAcc_Internal_FindArea(
    MemAcc_AddressAreaIdType AreaId,
    P2VAR(uint8, AUTOMATIC, MEMACC_VAR) OutIndex
)
{
    uint8 idx;

    if (MemAcc_ConfigPtr == NULL_PTR)
    {
        return FALSE;
    }

    for (idx = 0u; idx < MemAcc_ConfigPtr->NumberOfAreas; idx++)
    {
        if (MemAcc_ConfigPtr->AddressAreas[idx].AreaId == AreaId)
        {
            if (OutIndex != NULL_PTR)
            {
                *OutIndex = idx;
            }
            return TRUE;
        }
    }

    return FALSE;
}

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
    uint8 areaIndex;

    if (MemAcc_Internal_FindArea(AreaId, &areaIndex) == FALSE)
    {
        return FALSE;
    }

    areaLength = MemAcc_ConfigPtr->AddressAreas[areaIndex].Length;

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
    uint8 areaIndex;

    if (MemAcc_Internal_FindArea(AreaId, &areaIndex) == FALSE)
    {
        return 0u;
    }

    baseAddress = MemAcc_ConfigPtr->AddressAreas[areaIndex].StartAddress;

    return baseAddress + LogicalAddress;
}

#define MEMACC_STOP_SEC_CODE
#include "MemAcc_MemMap.h"
