#ifndef MEMACC_H
#define MEMACC_H
#include "Std_Types.h"
#include "MemIf_Types.h"

typedef uint32 MemAcc_AddressType;
typedef uint32 MemAcc_LengthType;
typedef uint8 MemAcc_AddressAreaIdType;

#define MEMACC_TOTAL_SIZE       (64u * 1024u)   /* 64KB total flash */
#define MEMACC_SECTOR_SIZE      (4u * 1024u)    /* 4KB per sector */
#define MEMACC_PAGE_SIZE        8u              /* 8-byte write page */
#define MEMACC_NUM_SECTORS      (MEMACC_TOTAL_SIZE / MEMACC_SECTOR_SIZE)
#define MEMACC_ERASED_VALUE     0xFFu

Std_ReturnType MemAcc_Init(void);
Std_ReturnType MemAcc_Read(MemAcc_AddressType SourceAddress, uint8* TargetAddressPtr, MemAcc_LengthType Length);
Std_ReturnType MemAcc_Write(MemAcc_AddressType TargetAddress, const uint8* SourceAddressPtr, MemAcc_LengthType Length);
Std_ReturnType MemAcc_Erase(MemAcc_AddressType TargetAddress, MemAcc_LengthType Length);
Std_ReturnType MemAcc_BlankCheck(MemAcc_AddressType Address, MemAcc_LengthType Length);
MemIf_StatusType MemAcc_GetStatus(void);
MemIf_JobResultType MemAcc_GetJobResult(void);

/* Test helpers */
uint8* MemAcc_GetRawBuffer(void);
void MemAcc_TestResetFlash(void);  /* Erases entire flash buffer -- test use only */
#endif
