/**
 * \file       Platform_Types.h
 * \brief      AUTOSAR Platform Types for Infineon TC3xx (AURIX 2G)
 *
 * \details    Defines platform-specific types and CPU characteristics
 *             per AUTOSAR R24-11 SWS Platform Types.
 *
 * \copyright  Copyright (c) 2024 -- All rights reserved.
 *             AUTOSAR R24-11 compliant.
 *
 * \autosar    AUTOSAR_SWS_PlatformTypes
 * \version    24.11.0
 */

#ifndef PLATFORM_TYPES_H
#define PLATFORM_TYPES_H

/*============================================================================*
 *  CPU characteristics for Infineon TC3xx (TriCore)
 *============================================================================*/

/** \brief CPU type: 32-bit */
#define CPU_TYPE_8       8u
#define CPU_TYPE_16      16u
#define CPU_TYPE_32      32u

/** \brief TC3xx is a 32-bit architecture */
#define CPU_TYPE         CPU_TYPE_32

/** \brief Bit ordering: LSB first */
#define CPU_BIT_ORDER    CPU_BIT_ORDER_LSB
#define CPU_BIT_ORDER_LSB   0u
#define CPU_BIT_ORDER_MSB   1u

/** \brief Byte ordering: little-endian */
#define CPU_BYTE_ORDER   CPU_BYTE_ORDER_LOW_ENDIAN
#define CPU_BYTE_ORDER_LOW_ENDIAN   0u
#define CPU_BYTE_ORDER_HIGH_ENDIAN  1u

/*============================================================================*
 *  Standard integer types
 *============================================================================*/

/** \brief Unsigned 8-bit integer (0..255) */
typedef unsigned char       uint8;

/** \brief Unsigned 16-bit integer (0..65535) */
typedef unsigned short      uint16;

/** \brief Unsigned 32-bit integer (0..4294967295) */
typedef unsigned int        uint32;

/** \brief Signed 8-bit integer (-128..127) */
typedef signed char         sint8;

/** \brief Signed 16-bit integer (-32768..32767) */
typedef signed short        sint16;

/** \brief Signed 32-bit integer (-2147483648..2147483647) */
typedef signed int          sint32;

/** \brief Unsigned integer type that can hold a pointer value */
typedef unsigned int        uint32_least;

/** \brief Signed integer type that can hold a pointer value */
typedef signed int          sint32_least;

/** \brief Unsigned 16-bit least type */
typedef unsigned short      uint16_least;

/** \brief Signed 16-bit least type */
typedef signed short        sint16_least;

/** \brief Unsigned 8-bit least type */
typedef unsigned char       uint8_least;

/** \brief Signed 8-bit least type */
typedef signed char         sint8_least;

/** \brief Single-precision floating point */
typedef float               float32;

/** \brief Double-precision floating point */
typedef double              float64;

/*============================================================================*
 *  Boolean type
 *============================================================================*/

/** \brief Boolean type */
typedef unsigned char       boolean;

#ifndef TRUE
/** \brief Boolean TRUE value */
#define TRUE    ((boolean)1u)
#endif

#ifndef FALSE
/** \brief Boolean FALSE value */
#define FALSE   ((boolean)0u)
#endif

/*============================================================================*
 *  Null pointer
 *============================================================================*/

#ifndef NULL_PTR
/** \brief Null pointer constant */
#define NULL_PTR    ((void *)0)
#endif

#endif /* PLATFORM_TYPES_H */
