#ifndef FEE_INTERNAL_H
#define FEE_INTERNAL_H
#include "Fee_Types.h"
#include "Fee_Cfg.h"

Std_ReturnType Fee_Internal_Init(const Fee_ConfigType* ConfigPtr);
Std_ReturnType Fee_Internal_QueueJob(const Fee_JobDescriptorType* JobDesc);
void Fee_Internal_ProcessJob(void);
Std_ReturnType Fee_Internal_CancelJob(void);
MemIf_StatusType Fee_Internal_GetStatus(void);
MemIf_JobResultType Fee_Internal_GetJobResult(void);
const Fee_BlockInfoType* Fee_Internal_GetBlockInfo(uint16 BlockIndex);
uint16 Fee_Internal_FindBlockIndex(uint16 BlockNumber);
const Fee_ConfigType* Fee_Internal_GetConfigPtr(void);
void Fee_Internal_CheckAndTriggerGC(void);
#endif
