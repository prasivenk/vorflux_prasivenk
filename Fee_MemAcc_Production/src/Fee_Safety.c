/**
 * \file       Fee_Safety.c
 * \brief      AUTOSAR Fee Module -- Safety Mechanisms Implementation
 *
 * \details    Implements RAM CRC integrity checking over BlockInfoTable
 *             and SectorInfo, and flow counter checking for the Fee module.
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

#include "Fee_Safety.h"
#include "Fee_StateMachine.h"
#include "Fee_Crc.h"
#include "Fee_Version.h"
#include "Det.h"
#include "Dem.h"

/*============================================================================*
 *  Module state variables
 *============================================================================*/

#define FEE_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "Fee_MemMap.h"

/** \brief Stored RAM CRC for integrity checking */
static volatile VAR(uint16, FEE_VAR) Fee_Safety_RamCrc = 0u;

/** \brief Flow counter for sequence checking */
static volatile VAR(uint32, FEE_VAR) Fee_Safety_FlowCounter = 0u;

#define FEE_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "Fee_MemMap.h"

/*============================================================================*
 *  Code Section
 *============================================================================*/

#define FEE_START_SEC_CODE
#include "Fee_MemMap.h"

/*============================================================================*
 *  Internal helper: compute CRC over BlockInfoTable and SectorInfo
 *============================================================================*/

static FUNC(uint16, FEE_CODE) Fee_Safety_ComputeRamCrc(void)
{
    VAR(uint16, AUTOMATIC) crc;

    /* CRC over BlockInfoTable */
    crc = Fee_Crc_Calculate(
        (P2CONST(uint8, AUTOMATIC, FEE_APPL_CONST))&Fee_BlockInfoTable[0],
        (uint32)(sizeof(Fee_BlockInfoType) * FEE_NUMBER_OF_BLOCKS),
        FEE_CRC_INITIAL);

    /* Continue CRC over SectorInfo */
    crc = Fee_Crc_Calculate(
        (P2CONST(uint8, AUTOMATIC, FEE_APPL_CONST))&Fee_SectorInfo[0],
        (uint32)(sizeof(Fee_SectorInfoType) * FEE_NUMBER_OF_SECTORS),
        crc);

    return crc;
}

/*============================================================================*
 *  Fee_Safety_Init
 *============================================================================*/

FUNC(void, FEE_CODE) Fee_Safety_Init(void)
{
    Fee_Safety_RamCrc = Fee_Safety_ComputeRamCrc();
    Fee_Safety_FlowCounter = 0u;
}

/*============================================================================*
 *  Fee_Safety_CyclicCheck
 *============================================================================*/

FUNC(void, FEE_CODE) Fee_Safety_CyclicCheck(void)
{
    VAR(uint16, AUTOMATIC) currentCrc;

    currentCrc = Fee_Safety_ComputeRamCrc();

    if (currentCrc != Fee_Safety_RamCrc)
    {
        (void)Det_ReportRuntimeError(FEE_MODULE_ID, FEE_INSTANCE_ID,
                                      FEE_SID_MAIN_FUNCTION, FEE_E_RAM_INTEGRITY);
        (void)Dem_SetEventStatus(FEE_E_HARDWARE_ERROR, DEM_EVENT_STATUS_FAILED);
    }
}

/*============================================================================*
 *  Fee_Safety_UpdateRamCrc
 *============================================================================*/

FUNC(void, FEE_CODE) Fee_Safety_UpdateRamCrc(void)
{
    Fee_Safety_RamCrc = Fee_Safety_ComputeRamCrc();
}

/*============================================================================*
 *  Fee_Safety_FlowCheck
 *============================================================================*/

FUNC(void, FEE_CODE) Fee_Safety_FlowCheck(uint32 Expected)
{
    if (Fee_Safety_FlowCounter != Expected)
    {
        (void)Det_ReportRuntimeError(FEE_MODULE_ID, FEE_INSTANCE_ID,
                                      FEE_SID_MAIN_FUNCTION, FEE_E_RAM_INTEGRITY);
        (void)Dem_SetEventStatus(FEE_E_HARDWARE_ERROR, DEM_EVENT_STATUS_FAILED);
    }

    Fee_Safety_FlowCounter++;
}

/*============================================================================*
 *  Fee_Safety_ResetFlowCounter
 *============================================================================*/

FUNC(void, FEE_CODE) Fee_Safety_ResetFlowCounter(void)
{
    Fee_Safety_FlowCounter = 0u;
}

#define FEE_STOP_SEC_CODE
#include "Fee_MemMap.h"
