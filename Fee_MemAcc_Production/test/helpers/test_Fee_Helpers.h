/**
 * \file       test_Fee_Helpers.h
 * \brief      Shared test helpers for Fee unit and integration tests.
 *
 * \details    Provides DriveOneCycle(), DriveInitToCompletion(),
 *             DriveJobToCompletion(), PlaceSectorHeader(), and
 *             PlaceBlockInFlash() so each test file need not redefine them.
 *
 * \copyright  Copyright (c) 2024 -- All rights reserved.
 */
#ifndef TEST_FEE_HELPERS_H
#define TEST_FEE_HELPERS_H

/*============================================================================*
 *  Includes
 *============================================================================*/

#include "Fee.h"
#include "Fee_StateMachine.h"
#include "Fee_Sector.h"
#include "Fee_Crc.h"
#include "Fee_PBcfg.h"
#include "MemAcc.h"
#include "MemAcc_PBcfg.h"
#include "Mem_DFLS_Stub.h"

/*============================================================================*
 *  Function Declarations
 *============================================================================*/

/**
 * \brief Drive one main cycle: MemAcc_MainFunction + Fee_MainFunction.
 */
void DriveOneCycle(void);

/**
 * \brief Call Fee_Init and drive main-function cycles until Fee reaches IDLE
 *        (or max 200 cycles).
 */
void DriveInitToCompletion(void);

/**
 * \brief Drive main-function cycles (without Fee_Init) until Fee reaches IDLE
 *        (or max 200 cycles).
 */
void DriveJobToCompletion(void);

/**
 * \brief Build a sector header in the simulated flash at the given byte offset.
 *
 * \param[in] flashOffset  Byte offset into flash memory.
 * \param[in] seqNum       Sequence number for the sector header.
 * \param[in] eraseCount   Erase count for the sector header.
 */
void PlaceSectorHeader(uint32 flashOffset, uint32 seqNum, uint16 eraseCount);

/**
 * \brief Build a block header + data in the simulated flash.
 *
 * \param[in] flashOffset  Byte offset into flash memory.
 * \param[in] blockNum     Block number.
 * \param[in] blockSize    Block data size in bytes.
 * \param[in] data         Pointer to block data.
 * \param[in] seqCounter   Block sequence counter.
 * \param[in] validMarker  Validity marker byte (e.g. FEE_MARKER_VALID).
 */
void PlaceBlockInFlash(uint32 flashOffset, uint16 blockNum, uint16 blockSize,
                       const uint8 *data, uint16 seqCounter,
                       uint8 validMarker);

#endif /* TEST_FEE_HELPERS_H */
