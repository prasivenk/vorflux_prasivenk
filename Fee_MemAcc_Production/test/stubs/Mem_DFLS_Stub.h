/**
 * \file       Mem_DFLS_Stub.h
 * \brief      Mem_DFLS Stub Header -- RAM-backed TC3xx DFLASH Simulation
 *
 * \details    Provides control APIs for the Mem_DFLS stub to simulate
 *             TC3xx DFLASH behavior: erased state = 0x00, writes can
 *             only SET bits (0->1), 32-byte page granularity.
 *
 * \copyright  Copyright (c) 2024 -- All rights reserved.
 */

#ifndef MEM_DFLS_STUB_H
#define MEM_DFLS_STUB_H

/*============================================================================*
 *  Includes
 *============================================================================*/

#include "Std_Types.h"
#include "Mem_DFLS.h"

/*============================================================================*
 *  Stub configuration
 *============================================================================*/

/** \brief Total simulated flash size (128KB covers both areas) */
#define MEM_DFLS_STUB_FLASH_SIZE      0x00020000u

/** \brief TC3xx DFLASH base address for stub */
#define MEM_DFLS_STUB_BASE_ADDRESS    0xAF000000u

/** \brief TC3xx DFLASH page size */
#define MEM_DFLS_STUB_PAGE_SIZE       32u

/** \brief TC3xx DFLASH erased byte value */
#define MEM_DFLS_STUB_ERASED_VALUE    0x00u

/*============================================================================*
 *  Stub control APIs
 *============================================================================*/

/** \brief Reset the stub to initial state (all flash erased) */
extern void Mem_DFLS_Stub_Reset(void);

/** \brief Set the return value for the next Mem_DFLS operation */
extern void Mem_DFLS_Stub_SetReturnValue(Std_ReturnType retVal);

/** \brief Set the job result that GetJobResult will return after MainFunction */
extern void Mem_DFLS_Stub_SetJobResult(Mem_DFLS_JobResultType result);

/** \brief Inject an ECC error at a specific address */
extern void Mem_DFLS_Stub_InjectEccError(
    Mem_DFLS_AddressType Address,
    Mem_DFLS_LengthType Length,
    boolean Uncorrectable
);

/** \brief Get the number of times a specific API was called */
extern uint32 Mem_DFLS_Stub_GetCallCount(uint8 ApiId);

/** \brief Get direct access to flash content (for verification) */
extern P2VAR(uint8, AUTOMATIC, MEM_DFLS_VAR) Mem_DFLS_Stub_GetFlashContent(void);

/*============================================================================*
 *  Stub API IDs for GetCallCount
 *============================================================================*/

#define MEM_DFLS_STUB_API_READ          0u
#define MEM_DFLS_STUB_API_WRITE         1u
#define MEM_DFLS_STUB_API_ERASE         2u
#define MEM_DFLS_STUB_API_BLANKCHECK    3u
#define MEM_DFLS_STUB_API_GETJOBRESULT  4u
#define MEM_DFLS_STUB_API_MAINFUNCTION  5u
#define MEM_DFLS_STUB_API_COUNT         6u

#endif /* MEM_DFLS_STUB_H */
