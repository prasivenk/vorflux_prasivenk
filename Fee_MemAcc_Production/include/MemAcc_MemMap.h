/**
 * \file       MemAcc_MemMap.h
 * \brief      Memory Mapping Header for MemAcc Module
 *
 * \details    Provides memory section start/stop pragmas for the MemAcc module.
 *             For host unit-test builds (FEE_UNIT_TEST defined), all sections
 *             expand to nothing. For TASKING compiler, pragma section
 *             directives map code and data to linker sections.
 *
 *             <PLACEHOLDER_REQUIRED> Pragma directives are TASKING-specific;
 *             adapt for target linker script.
 *
 * \copyright  Copyright (c) 2024 -- All rights reserved.
 *             AUTOSAR R24-11 compliant.
 *
 * \autosar    AUTOSAR_SWS_MemoryMapping
 * \version    24.11.0
 */

/*
 * Note: MemMap headers intentionally omit include guards.
 *       They are included multiple times with different section defines active.
 *       However, for this placeholder we use a simple approach that is safe
 *       for host builds.
 */

/*============================================================================*
 *  MemAcc Code Section
 *============================================================================*/

#if defined(MEMACC_START_SEC_CODE)
  #undef MEMACC_START_SEC_CODE
  #ifndef FEE_UNIT_TEST
    /* <PLACEHOLDER_REQUIRED> #pragma section code "MEMACC_CODE" */
  #endif
#endif

#if defined(MEMACC_STOP_SEC_CODE)
  #undef MEMACC_STOP_SEC_CODE
  #ifndef FEE_UNIT_TEST
    /* <PLACEHOLDER_REQUIRED> #pragma section code restore */
  #endif
#endif

/*============================================================================*
 *  MemAcc Constant Section (Unspecified)
 *============================================================================*/

#if defined(MEMACC_START_SEC_CONST_UNSPECIFIED)
  #undef MEMACC_START_SEC_CONST_UNSPECIFIED
  #ifndef FEE_UNIT_TEST
    /* <PLACEHOLDER_REQUIRED> #pragma section farrom "MEMACC_CONST" */
  #endif
#endif

#if defined(MEMACC_STOP_SEC_CONST_UNSPECIFIED)
  #undef MEMACC_STOP_SEC_CONST_UNSPECIFIED
  #ifndef FEE_UNIT_TEST
    /* <PLACEHOLDER_REQUIRED> #pragma section farrom restore */
  #endif
#endif

/*============================================================================*
 *  MemAcc Variable Section -- Cleared Unspecified
 *============================================================================*/

#if defined(MEMACC_START_SEC_VAR_CLEARED_UNSPECIFIED)
  #undef MEMACC_START_SEC_VAR_CLEARED_UNSPECIFIED
  #ifndef FEE_UNIT_TEST
    /* <PLACEHOLDER_REQUIRED> #pragma section farbss "MEMACC_VAR_CLEARED" */
  #endif
#endif

#if defined(MEMACC_STOP_SEC_VAR_CLEARED_UNSPECIFIED)
  #undef MEMACC_STOP_SEC_VAR_CLEARED_UNSPECIFIED
  #ifndef FEE_UNIT_TEST
    /* <PLACEHOLDER_REQUIRED> #pragma section farbss restore */
  #endif
#endif

/*============================================================================*
 *  MemAcc Variable Section -- Cleared 8-bit
 *============================================================================*/

#if defined(MEMACC_START_SEC_VAR_CLEARED_8)
  #undef MEMACC_START_SEC_VAR_CLEARED_8
  #ifndef FEE_UNIT_TEST
    /* <PLACEHOLDER_REQUIRED> #pragma section farbss "MEMACC_VAR_CLEARED_8" */
  #endif
#endif

#if defined(MEMACC_STOP_SEC_VAR_CLEARED_8)
  #undef MEMACC_STOP_SEC_VAR_CLEARED_8
  #ifndef FEE_UNIT_TEST
    /* <PLACEHOLDER_REQUIRED> #pragma section farbss restore */
  #endif
#endif

/*============================================================================*
 *  MemAcc Variable Section -- Cleared 16-bit
 *============================================================================*/

#if defined(MEMACC_START_SEC_VAR_CLEARED_16)
  #undef MEMACC_START_SEC_VAR_CLEARED_16
  #ifndef FEE_UNIT_TEST
    /* <PLACEHOLDER_REQUIRED> #pragma section farbss "MEMACC_VAR_CLEARED_16" */
  #endif
#endif

#if defined(MEMACC_STOP_SEC_VAR_CLEARED_16)
  #undef MEMACC_STOP_SEC_VAR_CLEARED_16
  #ifndef FEE_UNIT_TEST
    /* <PLACEHOLDER_REQUIRED> #pragma section farbss restore */
  #endif
#endif

/*============================================================================*
 *  MemAcc Variable Section -- Cleared 32-bit
 *============================================================================*/

#if defined(MEMACC_START_SEC_VAR_CLEARED_32)
  #undef MEMACC_START_SEC_VAR_CLEARED_32
  #ifndef FEE_UNIT_TEST
    /* <PLACEHOLDER_REQUIRED> #pragma section farbss "MEMACC_VAR_CLEARED_32" */
  #endif
#endif

#if defined(MEMACC_STOP_SEC_VAR_CLEARED_32)
  #undef MEMACC_STOP_SEC_VAR_CLEARED_32
  #ifndef FEE_UNIT_TEST
    /* <PLACEHOLDER_REQUIRED> #pragma section farbss restore */
  #endif
#endif

/*============================================================================*
 *  MemAcc Variable Section -- Init Unspecified
 *============================================================================*/

#if defined(MEMACC_START_SEC_VAR_INIT_UNSPECIFIED)
  #undef MEMACC_START_SEC_VAR_INIT_UNSPECIFIED
  #ifndef FEE_UNIT_TEST
    /* <PLACEHOLDER_REQUIRED> #pragma section fardata "MEMACC_VAR_INIT" */
  #endif
#endif

#if defined(MEMACC_STOP_SEC_VAR_INIT_UNSPECIFIED)
  #undef MEMACC_STOP_SEC_VAR_INIT_UNSPECIFIED
  #ifndef FEE_UNIT_TEST
    /* <PLACEHOLDER_REQUIRED> #pragma section fardata restore */
  #endif
#endif

/*============================================================================*
 *  MemAcc Configuration Data Section
 *============================================================================*/

#if defined(MEMACC_START_SEC_CONFIG_DATA_UNSPECIFIED)
  #undef MEMACC_START_SEC_CONFIG_DATA_UNSPECIFIED
  #ifndef FEE_UNIT_TEST
    /* <PLACEHOLDER_REQUIRED> #pragma section farrom "MEMACC_CONFIG_DATA" */
  #endif
#endif

#if defined(MEMACC_STOP_SEC_CONFIG_DATA_UNSPECIFIED)
  #undef MEMACC_STOP_SEC_CONFIG_DATA_UNSPECIFIED
  #ifndef FEE_UNIT_TEST
    /* <PLACEHOLDER_REQUIRED> #pragma section farrom restore */
  #endif
#endif
