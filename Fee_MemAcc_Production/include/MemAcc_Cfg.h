/**
 * \file       MemAcc_Cfg.h
 * \brief      AUTOSAR MemAcc Module -- Configuration Header
 *
 * \details    Compile-time configuration switches and address area
 *             definitions for the MemAcc module.
 *
 *             <PLACEHOLDER_REQUIRED> Actual TC3xx DFLASH base addresses
 *             depend on MCU variant. Addresses shown are for TC39x DF0.
 *             Verify against MCU datasheet.
 *
 * \copyright  Copyright (c) 2024 -- All rights reserved.
 *             AUTOSAR R24-11 compliant.
 *
 * \autosar    AUTOSAR_SWS_MemoryAccess
 * \version    24.11.0
 */

#ifndef MEMACC_CFG_H
#define MEMACC_CFG_H

/*============================================================================*
 *  Includes
 *============================================================================*/

#include "Std_Types.h"

/*============================================================================*
 *  Configuration switches
 *============================================================================*/

/** \brief Enable/disable development error detection */
#define MEMACC_DEV_ERROR_DETECT         STD_ON

/** \brief Enable/disable version info API */
#define MEMACC_VERSION_INFO_API         STD_ON

/** \brief Number of configured address areas */
#define MEMACC_NUMBER_OF_ADDRESS_AREAS  2u

/** \brief MainFunction call period in milliseconds */
#define MEMACC_MAIN_FUNCTION_PERIOD     5u

/*============================================================================*
 *  Address area 0: Fee DFLASH partition
 *  <PLACEHOLDER_REQUIRED> Actual TC3xx DFLASH base addresses depend on
 *  MCU variant. Addresses shown are for TC39x DF0. Verify against MCU
 *  datasheet.
 *============================================================================*/

/** \brief Fee DFLASH partition start address */
#define MEMACC_AREA0_START_ADDRESS      0xAF000000u

/** \brief Fee DFLASH partition length (64KB) */
#define MEMACC_AREA0_LENGTH             0x00010000u

/** \brief Fee DFLASH partition sector size (4KB) */
#define MEMACC_AREA0_SECTOR_SIZE        4096u

/** \brief Fee DFLASH partition page size (32 bytes) */
#define MEMACC_AREA0_PAGE_SIZE          32u

/*============================================================================*
 *  Address area 1: Bootloader DFLASH partition
 *  <PLACEHOLDER_REQUIRED> Same as area 0 regarding address verification.
 *============================================================================*/

/** \brief Bootloader DFLASH partition start address */
#define MEMACC_AREA1_START_ADDRESS      0xAF010000u

/** \brief Bootloader DFLASH partition length (16KB) */
#define MEMACC_AREA1_LENGTH             0x00004000u

/** \brief Bootloader DFLASH partition sector size (4KB) */
#define MEMACC_AREA1_SECTOR_SIZE        4096u

/** \brief Bootloader DFLASH partition page size (32 bytes) */
#define MEMACC_AREA1_PAGE_SIZE          32u

/*============================================================================*
 *  Internal compare buffer size (one page)
 *============================================================================*/

/** \brief Internal buffer size for compare operations */
#define MEMACC_COMPARE_BUFFER_SIZE      32u

#endif /* MEMACC_CFG_H */
