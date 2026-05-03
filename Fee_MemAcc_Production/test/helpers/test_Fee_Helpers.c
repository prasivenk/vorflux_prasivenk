/**
 * \file       test_Fee_Helpers.c
 * \brief      Shared test helpers for Fee unit and integration tests.
 *
 * \details    Common helper functions that were previously copy-pasted across
 *             multiple test files.  Now built once and linked to all test
 *             executables that need them.
 *
 * \copyright  Copyright (c) 2024 -- All rights reserved.
 */

/*============================================================================*
 *  Includes
 *============================================================================*/

#include "test_Fee_Helpers.h"
#include <string.h>

/*============================================================================*
 *  Constants
 *============================================================================*/

/** Maximum cycles for driving init/job to completion. */
#define TEST_MAX_CYCLES  200u

/*============================================================================*
 *  Function Definitions
 *============================================================================*/

void DriveOneCycle(void)
{
    MemAcc_MainFunction();
    Fee_MainFunction();
}

void DriveInitToCompletion(void)
{
    uint32 cycle;

    Fee_Init(&Fee_Config);

    for (cycle = 0u; cycle < TEST_MAX_CYCLES; cycle++)
    {
        DriveOneCycle();
        if (Fee_GetStatus() == MEMIF_IDLE)
        {
            break;
        }
    }
}

void DriveJobToCompletion(void)
{
    uint32 cycle;

    for (cycle = 0u; cycle < TEST_MAX_CYCLES; cycle++)
    {
        DriveOneCycle();
        if (Fee_GetStatus() == MEMIF_IDLE)
        {
            break;
        }
    }
}

void PlaceSectorHeader(uint32 flashOffset, uint32 seqNum, uint16 eraseCount)
{
    uint8 hdr[32];
    uint8 *flash = Mem_DFLS_Stub_GetFlashContent();

    Fee_Sector_BuildSectorHeader(hdr, seqNum, eraseCount);
    memcpy(&flash[flashOffset], hdr, 32);
}

void PlaceBlockInFlash(uint32 flashOffset, uint16 blockNum, uint16 blockSize,
                       const uint8 *data, uint16 seqCounter,
                       uint8 validMarker)
{
    uint8 hdr[32];
    uint8 *flash = Mem_DFLS_Stub_GetFlashContent();
    uint16 dataCrc;

    dataCrc = Fee_Crc_CalculateBlock(data, (uint32)blockSize);

    Fee_Sector_BuildBlockHeader(hdr, blockNum, blockSize, dataCrc, seqCounter, 0u);
    /* Set valid marker */
    hdr[30] = validMarker;

    memcpy(&flash[flashOffset], hdr, 32);
    memcpy(&flash[flashOffset + 32], data, blockSize);
}
