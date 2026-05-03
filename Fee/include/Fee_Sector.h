#ifndef FEE_SECTOR_H
#define FEE_SECTOR_H
#include "Fee_Types.h"

uint32 Fee_Sector_AlignToPage(uint32 size);
Std_ReturnType Fee_Sector_Init(const Fee_ConfigType* ConfigPtr);
Std_ReturnType Fee_Sector_ScanBlocks(Fee_BlockInfoType* BlockInfoTable, uint16 NumBlocks, const Fee_BlockConfigType* BlockConfig);
Std_ReturnType Fee_Sector_WriteBlock(uint16 BlockNumber, const uint8* DataPtr, uint16 DataLength, Fee_BlockInfoType* BlockInfo);
Std_ReturnType Fee_Sector_ReadBlock(const Fee_BlockInfoType* BlockInfo, uint16 BlockOffset, uint8* DataBufferPtr, uint16 Length);
Std_ReturnType Fee_Sector_InvalidateBlock(Fee_BlockInfoType* BlockInfo);
Std_ReturnType Fee_Sector_EraseImmediate(uint16 BlockNumber, Fee_BlockInfoType* BlockInfo,
    uint16 BlockSize, Fee_BlockInfoType* BlockInfoTable, uint16 NumBlocks, const Fee_BlockConfigType* BlockConfig);
Std_ReturnType Fee_Sector_GarbageCollect(Fee_BlockInfoType* BlockInfoTable, uint16 NumBlocks, const Fee_BlockConfigType* BlockConfig);
boolean Fee_Sector_HasSpace(uint16 RequiredSize);
uint8 Fee_Sector_GetActiveSectorIndex(void);
uint16 Fee_Sector_GetEraseCount(uint8 SectorIndex);
uint16 Fee_Crc16(const uint8* DataPtr, uint32 Length, uint16 InitialValue);
#endif
