/**
 * \file       Fee_Safety.h
 * \brief      AUTOSAR Fee Module -- Safety Mechanisms Internal Interface
 *
 * \details    Declares the safety check functions for the Fee module:
 *             RAM CRC integrity checking, flow counter checking.
 *
 * \copyright  Copyright (c) 2024 -- All rights reserved.
 *             AUTOSAR R24-11 compliant.
 *
 * \autosar    AUTOSAR_SWS_FlashEEPROMEmulation
 * \version    2.0.0
 */

#ifndef FEE_SAFETY_H
#define FEE_SAFETY_H

/*============================================================================*
 *  Includes
 *============================================================================*/

#include "Fee_Types.h"
#include "Fee_Cfg.h"

/*============================================================================*
 *  Function declarations
 *============================================================================*/

/**
 * \brief  Initialize safety mechanisms (compute initial RAM CRC)
 */
extern FUNC(void, FEE_CODE) Fee_Safety_Init(void);

/**
 * \brief  Cyclic check of RAM integrity
 *
 * \details  Recomputes CRC over BlockInfoTable and SectorInfo,
 *           reports errors if mismatch detected.
 */
extern FUNC(void, FEE_CODE) Fee_Safety_CyclicCheck(void);

/**
 * \brief  Update the stored RAM CRC after legitimate modification
 */
extern FUNC(void, FEE_CODE) Fee_Safety_UpdateRamCrc(void);

/**
 * \brief  Check flow counter against expected value
 *
 * \param[in] Expected  Expected flow counter value
 */
extern FUNC(void, FEE_CODE) Fee_Safety_FlowCheck(uint32 Expected);

/**
 * \brief  Reset the flow counter to 0
 */
extern FUNC(void, FEE_CODE) Fee_Safety_ResetFlowCounter(void);

#endif /* FEE_SAFETY_H */
