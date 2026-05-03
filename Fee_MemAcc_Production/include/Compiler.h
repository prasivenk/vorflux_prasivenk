/**
 * \file       Compiler.h
 * \brief      AUTOSAR Compiler Abstraction for TASKING / Host GCC
 *
 * \details    Provides compiler abstraction macros per AUTOSAR R24-11
 *             SWS Compiler Abstraction. For host GCC builds, all macros
 *             expand to standard C equivalents.
 *
 * \copyright  Copyright (c) 2024 -- All rights reserved.
 *             AUTOSAR R24-11 compliant.
 *
 * \autosar    AUTOSAR_SWS_CompilerAbstraction
 * \version    24.11.0
 */

#ifndef COMPILER_H
#define COMPILER_H

#include "Compiler_Cfg.h"

/*============================================================================*
 *  Compiler version information
 *============================================================================*/

#define COMPILER_VENDOR_ID                  0xFFFFu
#define COMPILER_AR_RELEASE_MAJOR_VERSION   24u
#define COMPILER_AR_RELEASE_MINOR_VERSION   11u
#define COMPILER_AR_RELEASE_REVISION_VERSION 0u

/*============================================================================*
 *  Memory class keywords
 *============================================================================*/

/** \brief Automatic storage duration */
#define AUTOMATIC

/** \brief Typedef qualifier */
#define TYPEDEF

/** \brief Static storage duration */
#ifndef STATIC
#define STATIC  static
#endif

/** \brief Null statement */
#define NULL_STATEMENT   ((void)0)

/** \brief Inline keyword abstraction */
#define INLINE           static inline

/** \brief Local inline keyword abstraction */
#define LOCAL_INLINE     static inline

/*============================================================================*
 *  Compiler abstraction macros
 *  For host GCC builds, all expand to standard C.
 *============================================================================*/

/**
 * \brief  Function declaration macro
 * \param  rettype    Return type of the function
 * \param  memclass   Memory class (ignored for GCC)
 */
/* MISRA C:2012 Dir 4.9 -- function-like macros required by AUTOSAR spec */
#define FUNC(rettype, memclass)                     rettype

/**
 * \brief  Pointer to variable macro
 * \param  ptrtype    Type of the pointed-to object
 * \param  memclass   Memory class of the pointer (ignored for GCC)
 * \param  ptrclass   Memory class of the pointed-to object (ignored for GCC)
 */
#define P2VAR(ptrtype, memclass, ptrclass)          ptrtype *

/**
 * \brief  Pointer to constant macro
 * \param  ptrtype    Type of the pointed-to object
 * \param  memclass   Memory class of the pointer (ignored for GCC)
 * \param  ptrclass   Memory class of the pointed-to object (ignored for GCC)
 */
#define P2CONST(ptrtype, memclass, ptrclass)        const ptrtype *

/**
 * \brief  Constant pointer to variable macro
 * \param  ptrtype    Type of the pointed-to object
 * \param  memclass   Memory class of the pointer (ignored for GCC)
 * \param  ptrclass   Memory class of the pointed-to object (ignored for GCC)
 */
#define CONSTP2VAR(ptrtype, memclass, ptrclass)     ptrtype * const

/**
 * \brief  Constant pointer to constant macro
 * \param  ptrtype    Type of the pointed-to object
 * \param  memclass   Memory class of the pointer (ignored for GCC)
 * \param  ptrclass   Memory class of the pointed-to object (ignored for GCC)
 */
#define CONSTP2CONST(ptrtype, memclass, ptrclass)   const ptrtype * const

/**
 * \brief  Pointer to function macro
 * \param  rettype    Return type of the function
 * \param  ptrclass   Memory class of the pointer (ignored for GCC)
 * \param  fctname    Name of the function pointer
 */
#define P2FUNC(rettype, ptrclass, fctname)          rettype (* fctname)

/**
 * \brief  Constant declaration macro
 * \param  type       Type of the constant
 * \param  memclass   Memory class (ignored for GCC)
 */
#define CONST(type, memclass)                       const type

/**
 * \brief  Variable declaration macro
 * \param  type       Type of the variable
 * \param  memclass   Memory class (ignored for GCC)
 */
#define VAR(type, memclass)                         type

#endif /* COMPILER_H */
