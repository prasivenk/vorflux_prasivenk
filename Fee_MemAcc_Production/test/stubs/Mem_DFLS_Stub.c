/**
 * \file       Mem_DFLS_Stub.c
 * \brief      Mem_DFLS Stub Implementation -- RAM-backed TC3xx DFLASH
 *
 * \details    Simulates TC3xx DFLASH behavior:
 *             - Erased state = 0x00
 *             - Writes can only SET bits (0->1)
 *             - 32-byte page granularity
 *             - Configurable return values
 *             - ECC error injection
 *             - Call recording
 *
 * \copyright  Copyright (c) 2024 -- All rights reserved.
 */

/*============================================================================*
 *  Includes
 *============================================================================*/

#include "Mem_DFLS_Stub.h"
#include <string.h>

/*============================================================================*
 *  Internal state
 *============================================================================*/

/** \brief Simulated flash memory (RAM-backed) */
static uint8 Mem_DFLS_Stub_Flash[MEM_DFLS_STUB_FLASH_SIZE];

/** \brief ECC error flags per byte: 0=none, 1=correctable, 2=uncorrectable */
static uint8 Mem_DFLS_Stub_EccFlags[MEM_DFLS_STUB_FLASH_SIZE];

/** \brief Configurable return value for API calls */
static Std_ReturnType Mem_DFLS_Stub_ReturnValue = E_OK;

/** \brief Configurable job result override -- if not PENDING, overrides auto result */
static Mem_DFLS_JobResultType Mem_DFLS_Stub_ResultOverride = MEM_DFLS_JOB_OK;

/** \brief Whether a job has been accepted and is awaiting processing */
static boolean Mem_DFLS_Stub_JobAccepted = FALSE;

/** \brief Whether the job has been processed (MainFunction called) */
static boolean Mem_DFLS_Stub_JobProcessed = FALSE;

/** \brief Auto-computed result based on actual flash operation */
static Mem_DFLS_JobResultType Mem_DFLS_Stub_AutoResult = MEM_DFLS_JOB_OK;

/** \brief API call counters */
static uint32 Mem_DFLS_Stub_CallCounts[MEM_DFLS_STUB_API_COUNT];

/** \brief Last operation address and length for ECC checking */
static Mem_DFLS_AddressType Mem_DFLS_Stub_LastAddr = 0u;
static Mem_DFLS_LengthType Mem_DFLS_Stub_LastLen = 0u;

/** \brief Whether last operation was a read (ECC relevant) */
static boolean Mem_DFLS_Stub_LastWasRead = FALSE;

/*============================================================================*
 *  Helper: convert physical address to array offset
 *============================================================================*/

static uint32 Mem_DFLS_Stub_AddrToOffset(Mem_DFLS_AddressType Address)
{
    return (uint32)(Address - MEM_DFLS_STUB_BASE_ADDRESS);
}

/*============================================================================*
 *  Helper: check for ECC errors in a range
 *  Returns: 0=none, 1=correctable, 2=uncorrectable
 *============================================================================*/

static uint8 Mem_DFLS_Stub_CheckEcc(Mem_DFLS_AddressType Address,
                                     Mem_DFLS_LengthType Length)
{
    uint32 offset;
    uint32 idx;
    uint8 worstEcc = 0u;

    offset = Mem_DFLS_Stub_AddrToOffset(Address);

    for (idx = 0u; idx < Length; idx++)
    {
        if ((offset + idx) < MEM_DFLS_STUB_FLASH_SIZE)
        {
            if (Mem_DFLS_Stub_EccFlags[offset + idx] > worstEcc)
            {
                worstEcc = Mem_DFLS_Stub_EccFlags[offset + idx];
            }
        }
    }

    return worstEcc;
}

/*============================================================================*
 *  Stub control APIs
 *============================================================================*/

void Mem_DFLS_Stub_Reset(void)
{
    /* TC3xx DFLASH erased state = 0x00 */
    (void)memset(Mem_DFLS_Stub_Flash, MEM_DFLS_STUB_ERASED_VALUE,
                 MEM_DFLS_STUB_FLASH_SIZE);
    (void)memset(Mem_DFLS_Stub_EccFlags, 0u, MEM_DFLS_STUB_FLASH_SIZE);
    (void)memset(Mem_DFLS_Stub_CallCounts, 0u, sizeof(Mem_DFLS_Stub_CallCounts));
    Mem_DFLS_Stub_ReturnValue = E_OK;
    Mem_DFLS_Stub_ResultOverride = MEM_DFLS_JOB_OK;
    Mem_DFLS_Stub_JobAccepted = FALSE;
    Mem_DFLS_Stub_JobProcessed = FALSE;
    Mem_DFLS_Stub_AutoResult = MEM_DFLS_JOB_OK;
    Mem_DFLS_Stub_LastAddr = 0u;
    Mem_DFLS_Stub_LastLen = 0u;
    Mem_DFLS_Stub_LastWasRead = FALSE;
}

void Mem_DFLS_Stub_SetReturnValue(Std_ReturnType retVal)
{
    Mem_DFLS_Stub_ReturnValue = retVal;
}

void Mem_DFLS_Stub_SetJobResult(Mem_DFLS_JobResultType result)
{
    Mem_DFLS_Stub_ResultOverride = result;
}

void Mem_DFLS_Stub_InjectEccError(
    Mem_DFLS_AddressType Address,
    Mem_DFLS_LengthType Length,
    boolean Uncorrectable
)
{
    uint32 offset;
    uint32 idx;
    uint8 eccVal;

    offset = Mem_DFLS_Stub_AddrToOffset(Address);
    eccVal = (Uncorrectable == TRUE) ? 2u : 1u;

    for (idx = 0u; idx < Length; idx++)
    {
        if ((offset + idx) < MEM_DFLS_STUB_FLASH_SIZE)
        {
            Mem_DFLS_Stub_EccFlags[offset + idx] = eccVal;
        }
    }
}

uint32 Mem_DFLS_Stub_GetCallCount(uint8 ApiId)
{
    if (ApiId < MEM_DFLS_STUB_API_COUNT)
    {
        return Mem_DFLS_Stub_CallCounts[ApiId];
    }
    return 0u;
}

P2VAR(uint8, AUTOMATIC, MEM_DFLS_VAR) Mem_DFLS_Stub_GetFlashContent(void)
{
    return &Mem_DFLS_Stub_Flash[0];
}

/*============================================================================*
 *  Mem_DFLS API implementations (stub)
 *============================================================================*/

FUNC(Std_ReturnType, MEM_DFLS_CODE) Mem_DFLS_Read(
    Mem_DFLS_AddressType SourceAddress,
    P2VAR(uint8, AUTOMATIC, MEM_DFLS_APPL_DATA) TargetBufferPtr,
    Mem_DFLS_LengthType Length
)
{
    uint32 offset;

    Mem_DFLS_Stub_CallCounts[MEM_DFLS_STUB_API_READ]++;

    if (Mem_DFLS_Stub_ReturnValue != E_OK)
    {
        return Mem_DFLS_Stub_ReturnValue;
    }

    offset = Mem_DFLS_Stub_AddrToOffset(SourceAddress);

    if ((offset + Length) > MEM_DFLS_STUB_FLASH_SIZE)
    {
        return E_NOT_OK;
    }

    /* Copy data from simulated flash */
    (void)memcpy(TargetBufferPtr, &Mem_DFLS_Stub_Flash[offset], Length);

    /* Record operation for ECC check */
    Mem_DFLS_Stub_LastAddr = SourceAddress;
    Mem_DFLS_Stub_LastLen = Length;
    Mem_DFLS_Stub_LastWasRead = TRUE;
    Mem_DFLS_Stub_JobAccepted = TRUE;
    Mem_DFLS_Stub_JobProcessed = FALSE;
    Mem_DFLS_Stub_AutoResult = MEM_DFLS_JOB_OK;

    return E_OK;
}

FUNC(Std_ReturnType, MEM_DFLS_CODE) Mem_DFLS_Write(
    Mem_DFLS_AddressType TargetAddress,
    P2CONST(uint8, AUTOMATIC, MEM_DFLS_APPL_DATA) SourceBufferPtr,
    Mem_DFLS_LengthType Length
)
{
    uint32 offset;
    uint32 idx;

    Mem_DFLS_Stub_CallCounts[MEM_DFLS_STUB_API_WRITE]++;

    if (Mem_DFLS_Stub_ReturnValue != E_OK)
    {
        return Mem_DFLS_Stub_ReturnValue;
    }

    offset = Mem_DFLS_Stub_AddrToOffset(TargetAddress);

    if ((offset + Length) > MEM_DFLS_STUB_FLASH_SIZE)
    {
        return E_NOT_OK;
    }

    /* TC3xx DFLASH: writes can only SET bits (0->1) */
    for (idx = 0u; idx < Length; idx++)
    {
        Mem_DFLS_Stub_Flash[offset + idx] |= SourceBufferPtr[idx];
    }

    Mem_DFLS_Stub_LastAddr = TargetAddress;
    Mem_DFLS_Stub_LastLen = Length;
    Mem_DFLS_Stub_LastWasRead = FALSE;
    Mem_DFLS_Stub_JobAccepted = TRUE;
    Mem_DFLS_Stub_JobProcessed = FALSE;
    Mem_DFLS_Stub_AutoResult = MEM_DFLS_JOB_OK;

    return E_OK;
}

FUNC(Std_ReturnType, MEM_DFLS_CODE) Mem_DFLS_Erase(
    Mem_DFLS_AddressType TargetAddress,
    Mem_DFLS_LengthType Length
)
{
    uint32 offset;

    Mem_DFLS_Stub_CallCounts[MEM_DFLS_STUB_API_ERASE]++;

    if (Mem_DFLS_Stub_ReturnValue != E_OK)
    {
        return Mem_DFLS_Stub_ReturnValue;
    }

    offset = Mem_DFLS_Stub_AddrToOffset(TargetAddress);

    if ((offset + Length) > MEM_DFLS_STUB_FLASH_SIZE)
    {
        return E_NOT_OK;
    }

    /* TC3xx DFLASH erased state = 0x00 */
    (void)memset(&Mem_DFLS_Stub_Flash[offset], MEM_DFLS_STUB_ERASED_VALUE, Length);
    /* Clear ECC flags for erased range */
    (void)memset(&Mem_DFLS_Stub_EccFlags[offset], 0u, Length);

    Mem_DFLS_Stub_LastAddr = TargetAddress;
    Mem_DFLS_Stub_LastLen = Length;
    Mem_DFLS_Stub_LastWasRead = FALSE;
    Mem_DFLS_Stub_JobAccepted = TRUE;
    Mem_DFLS_Stub_JobProcessed = FALSE;
    Mem_DFLS_Stub_AutoResult = MEM_DFLS_JOB_OK;

    return E_OK;
}

FUNC(Std_ReturnType, MEM_DFLS_CODE) Mem_DFLS_BlankCheck(
    Mem_DFLS_AddressType TargetAddress,
    Mem_DFLS_LengthType Length
)
{
    uint32 offset;
    uint32 idx;
    Mem_DFLS_JobResultType result = MEM_DFLS_JOB_OK;

    Mem_DFLS_Stub_CallCounts[MEM_DFLS_STUB_API_BLANKCHECK]++;

    if (Mem_DFLS_Stub_ReturnValue != E_OK)
    {
        return Mem_DFLS_Stub_ReturnValue;
    }

    offset = Mem_DFLS_Stub_AddrToOffset(TargetAddress);

    if ((offset + Length) > MEM_DFLS_STUB_FLASH_SIZE)
    {
        return E_NOT_OK;
    }

    /* Check if all bytes are erased (0x00 for TC3xx) */
    for (idx = 0u; idx < Length; idx++)
    {
        if (Mem_DFLS_Stub_Flash[offset + idx] != MEM_DFLS_STUB_ERASED_VALUE)
        {
            result = MEM_DFLS_JOB_FAILED;
            break;
        }
    }

    Mem_DFLS_Stub_LastAddr = TargetAddress;
    Mem_DFLS_Stub_LastLen = Length;
    Mem_DFLS_Stub_LastWasRead = FALSE;
    Mem_DFLS_Stub_JobAccepted = TRUE;
    Mem_DFLS_Stub_JobProcessed = FALSE;
    Mem_DFLS_Stub_AutoResult = result;

    return E_OK;
}

FUNC(Mem_DFLS_JobResultType, MEM_DFLS_CODE) Mem_DFLS_GetJobResult(void)
{
    uint8 eccStatus;

    Mem_DFLS_Stub_CallCounts[MEM_DFLS_STUB_API_GETJOBRESULT]++;

    /* If job is accepted but not yet processed, it's pending */
    if ((Mem_DFLS_Stub_JobAccepted == TRUE) &&
        (Mem_DFLS_Stub_JobProcessed == FALSE))
    {
        return MEM_DFLS_JOB_PENDING;
    }

    /* If job has been processed, determine result */
    if (Mem_DFLS_Stub_JobProcessed == TRUE)
    {
        /* Check if override is set to something other than OK */
        if (Mem_DFLS_Stub_ResultOverride != MEM_DFLS_JOB_OK)
        {
            return Mem_DFLS_Stub_ResultOverride;
        }

        /* Check ECC for read operations */
        if (Mem_DFLS_Stub_LastWasRead == TRUE)
        {
            eccStatus = Mem_DFLS_Stub_CheckEcc(Mem_DFLS_Stub_LastAddr,
                                                Mem_DFLS_Stub_LastLen);
            if (eccStatus == 2u)
            {
                return MEM_DFLS_JOB_ECC_UNCORRECTED;
            }
            if (eccStatus == 1u)
            {
                return MEM_DFLS_JOB_ECC_CORRECTED;
            }
        }

        return Mem_DFLS_Stub_AutoResult;
    }

    /* No job in progress -- return last auto result */
    return Mem_DFLS_Stub_AutoResult;
}

FUNC(void, MEM_DFLS_CODE) Mem_DFLS_MainFunction(void)
{
    Mem_DFLS_Stub_CallCounts[MEM_DFLS_STUB_API_MAINFUNCTION]++;

    /* In stub, jobs complete in one MainFunction cycle */
    if (Mem_DFLS_Stub_JobAccepted == TRUE)
    {
        Mem_DFLS_Stub_JobProcessed = TRUE;
        Mem_DFLS_Stub_JobAccepted = FALSE;
    }
}
