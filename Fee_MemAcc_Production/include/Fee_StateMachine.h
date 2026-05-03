/**
 * \file       Fee_StateMachine.h
 * \brief      AUTOSAR Fee Module -- State Machine Internal Interface
 *
 * \details    Declares the internal state machine types, state variables,
 *             and functions for the Fee async state machine.
 *
 * \copyright  Copyright (c) 2024 -- All rights reserved.
 *             AUTOSAR R24-11 compliant.
 *
 * \autosar    AUTOSAR_SWS_FlashEEPROMEmulation
 * \version    2.0.0
 */

#ifndef FEE_STATEMACHINE_H
#define FEE_STATEMACHINE_H

/*============================================================================*
 *  Includes
 *============================================================================*/

#include "Fee_Types.h"
#include "Fee_Cfg.h"

/*============================================================================*
 *  Internal state enumeration
 *============================================================================*/

/** \brief Fee internal state enumeration */
typedef enum
{
    FEE_STATE_UNINIT                    = 0,
    FEE_STATE_INIT_READ_SECTOR_HEADER   = 1,
    FEE_STATE_INIT_WAIT_SECTOR_HEADER   = 2,
    FEE_STATE_INIT_NEXT_SECTOR          = 3,
    FEE_STATE_INIT_SCAN_READ_RECORD     = 4,
    FEE_STATE_INIT_SCAN_WAIT_RECORD     = 5,
    /* Reserved for future data-verification during init scan */
    FEE_STATE_INIT_SCAN_READ_DATA       = 6,
    FEE_STATE_INIT_SCAN_WAIT_DATA       = 7,
    FEE_STATE_INIT_SCAN_NEXT_RECORD     = 8,
    FEE_STATE_IDLE                      = 9,
    FEE_STATE_READ_START                = 10,
    FEE_STATE_READ_WAIT                 = 11,
    FEE_STATE_READ_VERIFY_CRC           = 12,
    FEE_STATE_WRITE_ALLOC               = 13,
    FEE_STATE_WRITE_HEADER              = 14,
    FEE_STATE_WRITE_HEADER_WAIT         = 15,
    FEE_STATE_WRITE_DATA                = 16,
    FEE_STATE_WRITE_DATA_WAIT           = 17,
    FEE_STATE_WRITE_VALID               = 18,
    FEE_STATE_WRITE_VALID_WAIT          = 19,
    FEE_STATE_WRITE_VERIFY_READ         = 20,
    FEE_STATE_WRITE_VERIFY_WAIT         = 21,
    FEE_STATE_WRITE_VERIFY_COMPARE      = 22,
    FEE_STATE_INVALIDATE_WRITE          = 23,
    FEE_STATE_INVALIDATE_WAIT           = 24,
    FEE_STATE_ERASE_IMMEDIATE           = 25,
    FEE_STATE_ERASE_IMMEDIATE_WAIT      = 26,
    FEE_STATE_GC_ACTIVE                 = 27,
    FEE_STATE_ERROR                     = 28
} Fee_InternalStateType;

/*============================================================================*
 *  Extern state variable declarations (for Fee.c and Fee_Safety.c)
 *============================================================================*/

extern volatile VAR(Fee_BlockInfoType, FEE_VAR)
    Fee_BlockInfoTable[FEE_NUMBER_OF_BLOCKS];

extern volatile VAR(Fee_SectorInfoType, FEE_VAR)
    Fee_SectorInfo[FEE_NUMBER_OF_SECTORS];

extern volatile VAR(MemIf_StatusType, FEE_VAR)
    Fee_ModuleStatus;

extern volatile VAR(Fee_JobInfoType, FEE_VAR)
    Fee_CurrentJob;

extern volatile VAR(MemIf_JobResultType, FEE_VAR)
    Fee_LastJobResult;

extern volatile VAR(Fee_InternalStateType, FEE_VAR)
    Fee_InternalState;

extern volatile VAR(uint16, FEE_VAR)
    Fee_BlockSequenceCounters[FEE_NUMBER_OF_BLOCKS];

extern volatile VAR(uint8, FEE_VAR)
    Fee_ActiveSectorIndex;

extern P2CONST(Fee_ConfigType, AUTOMATIC, FEE_CONST) Fee_ConfigPtr;

/*============================================================================*
 *  Function declarations
 *============================================================================*/

/**
 * \brief  Initialize the state machine
 *
 * \param[in] ConfigPtr  Pointer to Fee configuration
 */
extern FUNC(void, FEE_CODE) Fee_StateMachine_Init(
    P2CONST(Fee_ConfigType, AUTOMATIC, FEE_CONST) ConfigPtr
);

/**
 * \brief  Process one step of the state machine (called from MainFunction)
 */
extern FUNC(void, FEE_CODE) Fee_StateMachine_Process(void);

/**
 * \brief  Cancel the current operation
 */
extern FUNC(void, FEE_CODE) Fee_StateMachine_Cancel(void);

/**
 * \brief  Get the current module status
 *
 * \return  Module status
 */
extern FUNC(MemIf_StatusType, FEE_CODE) Fee_StateMachine_GetStatus(void);

/**
 * \brief  Get the result of the last job
 *
 * \return  Job result
 */
extern FUNC(MemIf_JobResultType, FEE_CODE) Fee_StateMachine_GetJobResult(void);

/**
 * \brief  Accept a new job
 *
 * \param[in] Job  Pointer to job information
 *
 * \return  E_OK if job accepted, E_NOT_OK if rejected
 */
extern FUNC(Std_ReturnType, FEE_CODE) Fee_StateMachine_AcceptJob(
    P2CONST(Fee_JobInfoType, AUTOMATIC, FEE_CONST) Job
);

/**
 * \brief  Complete the current job with a result
 *
 * \param[in] Result  Job result to set
 */
extern FUNC(void, FEE_CODE) Fee_StateMachine_CompleteJob(
    MemIf_JobResultType Result
);

#endif /* FEE_STATEMACHINE_H */
