/**
 * \file       Compiler_Cfg.h
 * \brief      AUTOSAR Compiler Configuration -- Module Memory Classes
 *
 * \details    Defines module-specific memory class symbols used by
 *             Compiler.h macros. For host GCC builds, all memory
 *             classes expand to nothing (empty defines).
 *
 * \copyright  Copyright (c) 2024 -- All rights reserved.
 *             AUTOSAR R24-11 compliant.
 *
 * \autosar    AUTOSAR_SWS_CompilerAbstraction
 * \version    24.11.0
 */

#ifndef COMPILER_CFG_H
#define COMPILER_CFG_H

/*============================================================================*
 *  Fee module memory classes
 *============================================================================*/

/** \brief Fee code section memory class */
#define FEE_CODE

/** \brief Fee constant data memory class */
#define FEE_CONST

/** \brief Fee application data memory class */
#define FEE_APPL_DATA

/** \brief Fee application constant memory class */
#define FEE_APPL_CONST

/** \brief Fee uninitialized variable memory class */
#define FEE_VAR_NOINIT

/** \brief Fee power-on initialized variable memory class */
#define FEE_VAR_POWER_ON_INIT

/** \brief Fee general variable memory class */
#define FEE_VAR

/*============================================================================*
 *  MemAcc module memory classes
 *============================================================================*/

/** \brief MemAcc code section memory class */
#define MEMACC_CODE

/** \brief MemAcc constant data memory class */
#define MEMACC_CONST

/** \brief MemAcc application data memory class */
#define MEMACC_APPL_DATA

/** \brief MemAcc application constant memory class */
#define MEMACC_APPL_CONST

/** \brief MemAcc uninitialized variable memory class */
#define MEMACC_VAR_NOINIT

/** \brief MemAcc power-on initialized variable memory class */
#define MEMACC_VAR_POWER_ON_INIT

/** \brief MemAcc general variable memory class */
#define MEMACC_VAR

/*============================================================================*
 *  Mem driver module memory classes
 *============================================================================*/

/** \brief Mem_DFLS code section memory class */
#define MEM_DFLS_CODE

/** \brief Mem_DFLS constant data memory class */
#define MEM_DFLS_CONST

/** \brief Mem_DFLS application data memory class */
#define MEM_DFLS_APPL_DATA

/** \brief Mem_DFLS variable memory class */
#define MEM_DFLS_VAR

/*============================================================================*
 *  NvM module memory classes
 *============================================================================*/

/** \brief NvM code section memory class */
#define NVM_CODE

/** \brief DEM code section memory class */
#ifndef DEM_CODE
#define DEM_CODE
#endif

#endif /* COMPILER_CFG_H */
