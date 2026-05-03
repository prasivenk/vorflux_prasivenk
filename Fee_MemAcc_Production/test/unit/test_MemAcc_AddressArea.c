/**
 * \file       test_MemAcc_AddressArea.c
 * \brief      Unit Tests -- MemAcc Address Area Management
 *
 * \details    Tests address validation (within bounds, at boundary, out of
 *             bounds), translation, and invalid area ID handling.
 *
 * \copyright  Copyright (c) 2024 -- All rights reserved.
 */

/*============================================================================*
 *  Includes
 *============================================================================*/

#include "unity/unity.h"
#include "MemAcc.h"
#include "MemAcc_Types.h"
#include "MemAcc_Cfg.h"
#include "Det_Stub.h"
#include "Dem_Stub.h"
#include "SchM_Stub.h"
#include "Mem_DFLS_Stub.h"

/*============================================================================*
 *  Setup / Teardown
 *============================================================================*/

void setUp(void)
{
    Det_Stub_Reset();
    Dem_Stub_Reset();
    SchM_Stub_Reset();
    Mem_DFLS_Stub_Reset();
    MemAcc_DeInit();
    Det_Stub_Reset();
    MemAcc_Init(&MemAcc_Config);
}

void tearDown(void)
{
    MemAcc_DeInit();
}

/*============================================================================*
 *  Test: ValidateAddress within bounds
 *============================================================================*/

static void test_ValidateAddress_WithinBounds(void)
{
    boolean result;
    /* Area 0: length = 0x10000. Address 0 + 32 = within bounds */
    result = MemAcc_Internal_ValidateAddress(0u, 0u, 32u);
    TEST_ASSERT_TRUE(result);
}

/*============================================================================*
 *  Test: ValidateAddress at exact boundary
 *============================================================================*/

static void test_ValidateAddress_AtBoundary(void)
{
    boolean result;
    /* Area 0: length = 0x10000. Address (0x10000 - 32) + 32 = exactly at boundary */
    result = MemAcc_Internal_ValidateAddress(0u, 0x10000u - 32u, 32u);
    TEST_ASSERT_TRUE(result);
}

/*============================================================================*
 *  Test: ValidateAddress out of bounds
 *============================================================================*/

static void test_ValidateAddress_OutOfBounds(void)
{
    boolean result;
    /* Area 0: length = 0x10000. Address 0x10000 + 1 = out of bounds */
    result = MemAcc_Internal_ValidateAddress(0u, 0x10000u, 1u);
    TEST_ASSERT_FALSE(result);
}

/*============================================================================*
 *  Test: ValidateAddress partially out of bounds
 *============================================================================*/

static void test_ValidateAddress_PartiallyOutOfBounds(void)
{
    boolean result;
    /* Address near end, length extends beyond */
    result = MemAcc_Internal_ValidateAddress(0u, 0x10000u - 16u, 32u);
    TEST_ASSERT_FALSE(result);
}

/*============================================================================*
 *  Test: ValidateAddress area 1 within bounds
 *============================================================================*/

static void test_ValidateAddress_Area1_WithinBounds(void)
{
    boolean result;
    /* Area 1: length = 0x4000 */
    result = MemAcc_Internal_ValidateAddress(1u, 0u, 4096u);
    TEST_ASSERT_TRUE(result);
}

/*============================================================================*
 *  Test: ValidateAddress area 1 out of bounds
 *============================================================================*/

static void test_ValidateAddress_Area1_OutOfBounds(void)
{
    boolean result;
    /* Area 1: length = 0x4000, so address 0x4000 + 1 is out */
    result = MemAcc_Internal_ValidateAddress(1u, 0x4000u, 1u);
    TEST_ASSERT_FALSE(result);
}

/*============================================================================*
 *  Test: ValidateAddress with invalid area ID
 *============================================================================*/

static void test_ValidateAddress_InvalidArea(void)
{
    boolean result;
    result = MemAcc_Internal_ValidateAddress(99u, 0u, 32u);
    TEST_ASSERT_FALSE(result);
}

/*============================================================================*
 *  Test: TranslateAddress area 0
 *============================================================================*/

static void test_TranslateAddress_Area0(void)
{
    MemAcc_AddressType phys;
    /* Area 0: StartAddress = 0xAF000000, logical 0x100 => 0xAF000100 */
    phys = MemAcc_Internal_TranslateAddress(0u, 0x100u);
    TEST_ASSERT_EQUAL_UINT32(0xAF000100u, phys);
}

/*============================================================================*
 *  Test: TranslateAddress area 1
 *============================================================================*/

static void test_TranslateAddress_Area1(void)
{
    MemAcc_AddressType phys;
    /* Area 1: StartAddress = 0xAF010000, logical 0 => 0xAF010000 */
    phys = MemAcc_Internal_TranslateAddress(1u, 0u);
    TEST_ASSERT_EQUAL_UINT32(0xAF010000u, phys);
}

/*============================================================================*
 *  Test: TranslateAddress with invalid area
 *============================================================================*/

static void test_TranslateAddress_InvalidArea(void)
{
    MemAcc_AddressType phys;
    phys = MemAcc_Internal_TranslateAddress(99u, 0u);
    TEST_ASSERT_EQUAL_UINT32(0u, phys);
}

/*============================================================================*
 *  Test: FindArea valid
 *============================================================================*/

static void test_FindArea_Valid(void)
{
    boolean found;
    found = MemAcc_Internal_FindArea(0u);
    TEST_ASSERT_TRUE(found);
    found = MemAcc_Internal_FindArea(1u);
    TEST_ASSERT_TRUE(found);
}

/*============================================================================*
 *  Test: FindArea invalid
 *============================================================================*/

static void test_FindArea_Invalid(void)
{
    boolean found;
    found = MemAcc_Internal_FindArea(2u);
    TEST_ASSERT_FALSE(found);
    found = MemAcc_Internal_FindArea(255u);
    TEST_ASSERT_FALSE(found);
}

/*============================================================================*
 *  Test: ValidateAddress zero length (edge case)
 *============================================================================*/

static void test_ValidateAddress_ZeroLength(void)
{
    boolean result;
    /* Address 0 + length 0 = address 0 which is <= 0x10000 */
    result = MemAcc_Internal_ValidateAddress(0u, 0u, 0u);
    TEST_ASSERT_TRUE(result);
}

/*============================================================================*
 *  Test: TranslateAddress at logical address 0
 *============================================================================*/

static void test_TranslateAddress_ZeroOffset(void)
{
    MemAcc_AddressType phys;
    phys = MemAcc_Internal_TranslateAddress(0u, 0u);
    TEST_ASSERT_EQUAL_UINT32(MEMACC_AREA0_START_ADDRESS, phys);
}

/*============================================================================*
 *  Main
 *============================================================================*/

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_ValidateAddress_WithinBounds);
    RUN_TEST(test_ValidateAddress_AtBoundary);
    RUN_TEST(test_ValidateAddress_OutOfBounds);
    RUN_TEST(test_ValidateAddress_PartiallyOutOfBounds);
    RUN_TEST(test_ValidateAddress_Area1_WithinBounds);
    RUN_TEST(test_ValidateAddress_Area1_OutOfBounds);
    RUN_TEST(test_ValidateAddress_InvalidArea);
    RUN_TEST(test_TranslateAddress_Area0);
    RUN_TEST(test_TranslateAddress_Area1);
    RUN_TEST(test_TranslateAddress_InvalidArea);
    RUN_TEST(test_FindArea_Valid);
    RUN_TEST(test_FindArea_Invalid);
    RUN_TEST(test_ValidateAddress_ZeroLength);
    RUN_TEST(test_TranslateAddress_ZeroOffset);

    return UNITY_END();
}
