#include "MemAcc.h"
#include <string.h>

static uint8 MemAcc_FlashBuffer[MEMACC_TOTAL_SIZE];
static MemIf_StatusType MemAcc_Status = MEMIF_UNINIT;
static MemIf_JobResultType MemAcc_LastJobResult = MEMIF_JOB_OK;

Std_ReturnType MemAcc_Init(void)
{
    /* Set status to IDLE but do NOT erase the flash buffer */
    MemAcc_Status = MEMIF_IDLE;
    return E_OK;
}

void MemAcc_TestResetFlash(void)
{
    /* Erase entire flash buffer -- test use only */
    memset(MemAcc_FlashBuffer, MEMACC_ERASED_VALUE, MEMACC_TOTAL_SIZE);
}

Std_ReturnType MemAcc_Write(MemAcc_AddressType TargetAddress, const uint8* SourceAddressPtr, MemAcc_LengthType Length)
{
    MemAcc_LengthType i;

    /* Validate parameters */
    if (SourceAddressPtr == NULL_PTR)
    {
        MemAcc_LastJobResult = MEMIF_JOB_FAILED;
        return E_NOT_OK;
    }
    if ((TargetAddress + Length) > MEMACC_TOTAL_SIZE)
    {
        MemAcc_LastJobResult = MEMIF_JOB_FAILED;
        return E_NOT_OK;
    }

    /* Flash bit-clearing semantics: new value can only clear bits (1->0), never set them (0->1).
     * For each byte, check that (src[i] & flash[addr+i]) == src[i].
     * If any byte would set a 0-bit back to 1, fail. */
    for (i = 0u; i < Length; i++)
    {
        if ((SourceAddressPtr[i] & MemAcc_FlashBuffer[TargetAddress + i]) != SourceAddressPtr[i])
        {
            MemAcc_LastJobResult = MEMIF_JOB_FAILED;
            return E_NOT_OK;
        }
    }

    /* Apply writes: flash[addr+i] &= src[i] */
    for (i = 0u; i < Length; i++)
    {
        MemAcc_FlashBuffer[TargetAddress + i] &= SourceAddressPtr[i];
    }

    MemAcc_LastJobResult = MEMIF_JOB_OK;
    return E_OK;
}

Std_ReturnType MemAcc_Read(MemAcc_AddressType SourceAddress, uint8* TargetAddressPtr, MemAcc_LengthType Length)
{
    /* Validate parameters */
    if (TargetAddressPtr == NULL_PTR)
    {
        MemAcc_LastJobResult = MEMIF_JOB_FAILED;
        return E_NOT_OK;
    }
    if ((SourceAddress + Length) > MEMACC_TOTAL_SIZE)
    {
        MemAcc_LastJobResult = MEMIF_JOB_FAILED;
        return E_NOT_OK;
    }

    memcpy(TargetAddressPtr, &MemAcc_FlashBuffer[SourceAddress], Length);
    MemAcc_LastJobResult = MEMIF_JOB_OK;
    return E_OK;
}

Std_ReturnType MemAcc_Erase(MemAcc_AddressType TargetAddress, MemAcc_LengthType Length)
{
    /* Validate sector-aligned address */
    if ((TargetAddress % MEMACC_SECTOR_SIZE) != 0u)
    {
        MemAcc_LastJobResult = MEMIF_JOB_FAILED;
        return E_NOT_OK;
    }
    /* Validate sector-multiple length */
    if ((Length % MEMACC_SECTOR_SIZE) != 0u)
    {
        MemAcc_LastJobResult = MEMIF_JOB_FAILED;
        return E_NOT_OK;
    }
    /* Validate bounds */
    if ((TargetAddress + Length) > MEMACC_TOTAL_SIZE)
    {
        MemAcc_LastJobResult = MEMIF_JOB_FAILED;
        return E_NOT_OK;
    }

    memset(&MemAcc_FlashBuffer[TargetAddress], MEMACC_ERASED_VALUE, Length);
    MemAcc_LastJobResult = MEMIF_JOB_OK;
    return E_OK;
}

Std_ReturnType MemAcc_BlankCheck(MemAcc_AddressType Address, MemAcc_LengthType Length)
{
    MemAcc_LengthType i;

    /* Validate bounds */
    if ((Address + Length) > MEMACC_TOTAL_SIZE)
    {
        MemAcc_LastJobResult = MEMIF_JOB_FAILED;
        return E_NOT_OK;
    }

    for (i = 0u; i < Length; i++)
    {
        if (MemAcc_FlashBuffer[Address + i] != MEMACC_ERASED_VALUE)
        {
            MemAcc_LastJobResult = MEMIF_JOB_FAILED;
            return E_NOT_OK;
        }
    }

    MemAcc_LastJobResult = MEMIF_JOB_OK;
    return E_OK;
}

MemIf_StatusType MemAcc_GetStatus(void)
{
    return MemAcc_Status;
}

MemIf_JobResultType MemAcc_GetJobResult(void)
{
    return MemAcc_LastJobResult;
}

uint8* MemAcc_GetRawBuffer(void)
{
    return MemAcc_FlashBuffer;
}
