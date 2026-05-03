#ifndef FEE_H
#define FEE_H
#include "Fee_Types.h"
#include "Fee_Cfg.h"

#define FEE_SW_MAJOR_VERSION    1u
#define FEE_SW_MINOR_VERSION    0u
#define FEE_SW_PATCH_VERSION    0u
#define FEE_AR_RELEASE_MAJOR_VERSION    24u
#define FEE_AR_RELEASE_MINOR_VERSION    11u

void Fee_Init(const Fee_ConfigType* ConfigPtr);
void Fee_SetMode(MemIf_ModeType Mode);
Std_ReturnType Fee_Read(uint16 BlockNumber, uint16 BlockOffset, uint8* DataBufferPtr, uint16 Length);
Std_ReturnType Fee_Write(uint16 BlockNumber, const uint8* DataBufferPtr);
void Fee_Cancel(void);
MemIf_StatusType Fee_GetStatus(void);
MemIf_JobResultType Fee_GetJobResult(void);
Std_ReturnType Fee_InvalidateBlock(uint16 BlockNumber);
void Fee_GetVersionInfo(Std_VersionInfoType* VersionInfoPtr);
Std_ReturnType Fee_EraseImmediateBlock(uint16 BlockNumber);
void Fee_MainFunction(void);
#endif /* FEE_H */
