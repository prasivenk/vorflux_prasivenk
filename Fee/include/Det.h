#ifndef DET_H
#define DET_H
#include "Std_Types.h"
void Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId);
uint16 Det_GetLastModuleId(void);
uint8 Det_GetLastApiId(void);
uint8 Det_GetLastErrorId(void);
void Det_ClearLastError(void);
#endif
