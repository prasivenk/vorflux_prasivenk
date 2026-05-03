#include "Det.h"

static uint16 Det_LastModuleId = 0u;
static uint8 Det_LastApiId = 0u;
static uint8 Det_LastErrorId = 0u;

void Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId)
{
    (void)InstanceId;
    Det_LastModuleId = ModuleId;
    Det_LastApiId = ApiId;
    Det_LastErrorId = ErrorId;
}

uint16 Det_GetLastModuleId(void)
{
    return Det_LastModuleId;
}

uint8 Det_GetLastApiId(void)
{
    return Det_LastApiId;
}

uint8 Det_GetLastErrorId(void)
{
    return Det_LastErrorId;
}

void Det_ClearLastError(void)
{
    Det_LastModuleId = 0u;
    Det_LastApiId = 0u;
    Det_LastErrorId = 0u;
}
