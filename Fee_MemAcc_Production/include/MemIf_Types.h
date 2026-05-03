/**
 * \file       MemIf_Types.h
 * \brief      AUTOSAR Memory Interface Types
 *
 * \details    Defines enumerations for memory abstraction interface
 *             status, job result, and mode per AUTOSAR R24-11
 *             SWS Memory Abstraction Interface.
 *
 * \copyright  Copyright (c) 2024 -- All rights reserved.
 *             AUTOSAR R24-11 compliant.
 *
 * \autosar    AUTOSAR_SWS_MemoryAbstractionInterface
 * \version    24.11.0
 */

#ifndef MEMIF_TYPES_H
#define MEMIF_TYPES_H

/*============================================================================*
 *  Module status type
 *============================================================================*/

/**
 * \brief  Status of the memory abstraction module
 */
typedef enum
{
    MEMIF_UNINIT        = 0,  /**< Module not initialized */
    MEMIF_IDLE          = 1,  /**< Module initialized, no job pending */
    MEMIF_BUSY          = 2,  /**< Module busy with a job */
    MEMIF_BUSY_INTERNAL = 3   /**< Module busy with internal management */
} MemIf_StatusType;

/*============================================================================*
 *  Job result type
 *============================================================================*/

/**
 * \brief  Result of the last requested job
 */
typedef enum
{
    MEMIF_JOB_OK              = 0,  /**< Job completed successfully */
    MEMIF_JOB_FAILED          = 1,  /**< Job failed */
    MEMIF_JOB_PENDING         = 2,  /**< Job is still pending */
    MEMIF_JOB_CANCELED        = 3,  /**< Job was canceled */
    MEMIF_BLOCK_INCONSISTENT  = 4,  /**< Block data is inconsistent */
    MEMIF_BLOCK_INVALID       = 5   /**< Block is invalid (not written) */
} MemIf_JobResultType;

/*============================================================================*
 *  Mode type
 *============================================================================*/

/**
 * \brief  Operating mode of the memory abstraction module
 */
typedef enum
{
    MEMIF_MODE_SLOW = 0,  /**< Slow operating mode */
    MEMIF_MODE_FAST = 1   /**< Fast operating mode */
} MemIf_ModeType;

#endif /* MEMIF_TYPES_H */
