/**
 * \file       Fee.h
 * \brief      AUTOSAR Fee Module -- Public API
 *
 * \details    Provides the public API for the Flash EEPROM Emulation module
 *             per AUTOSAR R24-11 SWS Fee.
 *
 * \copyright  Copyright (c) 2024 -- All rights reserved.
 *             AUTOSAR R24-11 compliant.
 *
 * \autosar    AUTOSAR_SWS_FlashEEPROMEmulation
 * \version    2.0.0
 */

#ifndef FEE_H
#define FEE_H

/*============================================================================*
 *  Includes
 *============================================================================*/

#include "Fee_Types.h"
#include "Fee_Cfg.h"
#include "Fee_Version.h"

/*============================================================================*
 *  Public API declarations
 *============================================================================*/

extern FUNC(void, FEE_CODE) Fee_Init(
    P2CONST(Fee_ConfigType, AUTOMATIC, FEE_CONST) ConfigPtr
);

extern FUNC(void, FEE_CODE) Fee_SetMode(
    MemIf_ModeType Mode
);

extern FUNC(Std_ReturnType, FEE_CODE) Fee_Read(
    uint16 BlockNumber,
    uint16 BlockOffset,
    P2VAR(uint8, AUTOMATIC, FEE_APPL_DATA) DataBufferPtr,
    uint16 Length
);

extern FUNC(Std_ReturnType, FEE_CODE) Fee_Write(
    uint16 BlockNumber,
    P2CONST(uint8, AUTOMATIC, FEE_APPL_CONST) DataBufferPtr
);

extern FUNC(void, FEE_CODE) Fee_Cancel(void);

extern FUNC(MemIf_StatusType, FEE_CODE) Fee_GetStatus(void);

extern FUNC(MemIf_JobResultType, FEE_CODE) Fee_GetJobResult(void);

extern FUNC(Std_ReturnType, FEE_CODE) Fee_InvalidateBlock(
    uint16 BlockNumber
);

extern FUNC(void, FEE_CODE) Fee_GetVersionInfo(
    P2VAR(Std_VersionInfoType, AUTOMATIC, FEE_APPL_DATA) VersionInfoPtr
);

extern FUNC(Std_ReturnType, FEE_CODE) Fee_EraseImmediateBlock(
    uint16 BlockNumber
);

extern FUNC(void, FEE_CODE) Fee_MainFunction(void);

#endif /* FEE_H */
