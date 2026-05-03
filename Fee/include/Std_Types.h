#ifndef STD_TYPES_H
#define STD_TYPES_H
#include <stdint.h>
typedef uint8_t uint8; typedef uint16_t uint16; typedef uint32_t uint32;
typedef int8_t sint8; typedef int16_t sint16; typedef int32_t sint32;
typedef uint8 Std_ReturnType; typedef uint8 boolean;
#define E_OK ((Std_ReturnType)0x00u)
#define E_NOT_OK ((Std_ReturnType)0x01u)
#define STD_ON 0x01u
#define STD_OFF 0x00u
#define TRUE 1u
#define FALSE 0u
#define NULL_PTR ((void*)0)
typedef struct {
    uint16 vendorID; uint16 moduleID;
    uint8 sw_major_version; uint8 sw_minor_version; uint8 sw_patch_version;
} Std_VersionInfoType;
#endif
