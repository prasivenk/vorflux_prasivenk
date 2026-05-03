/**
 * \file       test_Fee_Crc.c
 * \brief      Unit Tests -- Fee CRC-16 CCITT
 *
 * \details    Tests CRC-16 CCITT implementation against known test vectors,
 *             edge cases (empty data, single byte, all zeros, all 0xFF),
 *             large data, incremental CRC, and block header CRC.
 *
 * \copyright  Copyright (c) 2024 -- All rights reserved.
 */

/*============================================================================*
 *  Includes
 *============================================================================*/

#include "unity/unity.h"
#include "Fee_Crc.h"
#include <string.h>

/*============================================================================*
 *  Setup / Teardown
 *============================================================================*/

void setUp(void)
{
    /* Nothing to set up */
}

void tearDown(void)
{
    /* Nothing to tear down */
}

/*============================================================================*
 *  Test 1: Known test vector "123456789" -> 0x29B1
 *============================================================================*/

static void test_CRC_KnownTestVector(void)
{
    const uint8 data[] = { '1', '2', '3', '4', '5', '6', '7', '8', '9' };
    uint16 crc;

    crc = Fee_Crc_CalculateBlock(data, 9u);
    TEST_ASSERT_EQUAL_HEX16(0x29B1u, crc);
}

/*============================================================================*
 *  Test 2: Known test vector via Fee_Crc_Calculate with explicit start value
 *============================================================================*/

static void test_CRC_KnownTestVector_ExplicitStart(void)
{
    const uint8 data[] = { '1', '2', '3', '4', '5', '6', '7', '8', '9' };
    uint16 crc;

    crc = Fee_Crc_Calculate(data, 9u, 0xFFFFu);
    TEST_ASSERT_EQUAL_HEX16(0x29B1u, crc);
}

/*============================================================================*
 *  Test 3: Empty data (length 0)
 *============================================================================*/

static void test_CRC_EmptyData(void)
{
    uint16 crc;

    crc = Fee_Crc_CalculateBlock(NULL_PTR, 0u);
    /* With no data, CRC should remain the initial value */
    TEST_ASSERT_EQUAL_HEX16(0xFFFFu, crc);
}

/*============================================================================*
 *  Test 4: Single byte (0x00)
 *============================================================================*/

static void test_CRC_SingleByteZero(void)
{
    const uint8 data[1] = { 0x00u };
    uint16 crc;

    crc = Fee_Crc_CalculateBlock(data, 1u);
    /* CRC should not remain 0xFFFF for any input byte */
    TEST_ASSERT_NOT_EQUAL(0xFFFFu, crc);
    /* Known value: CRC-16/CCITT-FALSE of {0x00} = 0xE1F0 */
    TEST_ASSERT_EQUAL_HEX16(0xE1F0u, crc);
}

/*============================================================================*
 *  Test 5: Single byte (0xFF)
 *============================================================================*/

static void test_CRC_SingleByteFF(void)
{
    const uint8 data[1] = { 0xFFu };
    uint16 crc;

    crc = Fee_Crc_CalculateBlock(data, 1u);
    TEST_ASSERT_NOT_EQUAL(0xFFFFu, crc);
}

/*============================================================================*
 *  Test 6: All-zero data (various lengths)
 *============================================================================*/

static void test_CRC_AllZeroData(void)
{
    uint8 data4[4];
    uint8 data8[8];
    uint8 data16[16];
    uint16 crc4, crc8, crc16;

    memset(data4, 0x00, 4);
    memset(data8, 0x00, 8);
    memset(data16, 0x00, 16);

    crc4  = Fee_Crc_CalculateBlock(data4, 4u);
    crc8  = Fee_Crc_CalculateBlock(data8, 8u);
    crc16 = Fee_Crc_CalculateBlock(data16, 16u);

    /* All should be different from each other and from initial value */
    TEST_ASSERT_NOT_EQUAL(0xFFFFu, crc4);
    TEST_ASSERT_NOT_EQUAL(crc4, crc8);
    TEST_ASSERT_NOT_EQUAL(crc8, crc16);
}

/*============================================================================*
 *  Test 7: All-0xFF data
 *============================================================================*/

static void test_CRC_AllFFData(void)
{
    uint8 data[8];
    uint16 crc;

    memset(data, 0xFF, 8);
    crc = Fee_Crc_CalculateBlock(data, 8u);
    TEST_ASSERT_NOT_EQUAL(0xFFFFu, crc);
}

/*============================================================================*
 *  Test 8: Large data (512 bytes)
 *============================================================================*/

static void test_CRC_LargeData(void)
{
    uint8 data[512];
    uint16 crc;
    uint32 i;

    /* Fill with a pattern */
    for (i = 0u; i < 512u; i++)
    {
        data[i] = (uint8)(i & 0xFFu);
    }

    crc = Fee_Crc_CalculateBlock(data, 512u);

    /* Verify determinism: same data -> same CRC */
    TEST_ASSERT_EQUAL_HEX16(crc, Fee_Crc_CalculateBlock(data, 512u));

    /* Modify one byte and verify CRC changes */
    data[256] ^= 0x01u;
    TEST_ASSERT_NOT_EQUAL(crc, Fee_Crc_CalculateBlock(data, 512u));
}

/*============================================================================*
 *  Test 9: Incremental CRC (compute in two parts, verify same as one-shot)
 *============================================================================*/

static void test_CRC_IncrementalCalculation(void)
{
    const uint8 data[] = { '1', '2', '3', '4', '5', '6', '7', '8', '9' };
    uint16 oneShot;
    uint16 partial;

    oneShot = Fee_Crc_CalculateBlock(data, 9u);

    /* Calculate first 4 bytes, then remaining 5 bytes */
    partial = Fee_Crc_Calculate(data, 4u, FEE_CRC_INITIAL);
    partial = Fee_Crc_Calculate(&data[4], 5u, partial);

    TEST_ASSERT_EQUAL_HEX16(oneShot, partial);
}

/*============================================================================*
 *  Test 10: Incremental CRC with different split points
 *============================================================================*/

static void test_CRC_IncrementalDifferentSplits(void)
{
    const uint8 data[] = { '1', '2', '3', '4', '5', '6', '7', '8', '9' };
    uint16 oneShot;
    uint16 partial;

    oneShot = Fee_Crc_CalculateBlock(data, 9u);

    /* Split at 1 + 8 */
    partial = Fee_Crc_Calculate(data, 1u, FEE_CRC_INITIAL);
    partial = Fee_Crc_Calculate(&data[1], 8u, partial);
    TEST_ASSERT_EQUAL_HEX16(oneShot, partial);

    /* Split at 7 + 2 */
    partial = Fee_Crc_Calculate(data, 7u, FEE_CRC_INITIAL);
    partial = Fee_Crc_Calculate(&data[7], 2u, partial);
    TEST_ASSERT_EQUAL_HEX16(oneShot, partial);
}

/*============================================================================*
 *  Test 11: CRC of block header bytes
 *============================================================================*/

static void test_CRC_BlockHeaderBytes(void)
{
    /* Simulate a block header's first 8 meaningful bytes */
    uint8 headerData[8] = { 0x01, 0x00, 0x40, 0x00, 0xAA, 0xBB, 0x01, 0x00 };
    uint16 crc1, crc2;

    crc1 = Fee_Crc_CalculateBlock(headerData, 8u);

    /* Modify one byte and verify different CRC */
    headerData[0] = 0x02u;
    crc2 = Fee_Crc_CalculateBlock(headerData, 8u);

    TEST_ASSERT_NOT_EQUAL(crc1, crc2);
}

/*============================================================================*
 *  Test 12: Custom start value
 *============================================================================*/

static void test_CRC_CustomStartValue(void)
{
    const uint8 data[] = { 0x01u, 0x02u, 0x03u };
    uint16 crc0, crcFF;

    crc0  = Fee_Crc_Calculate(data, 3u, 0x0000u);
    crcFF = Fee_Crc_Calculate(data, 3u, 0xFFFFu);

    /* Different start values should produce different CRCs */
    TEST_ASSERT_NOT_EQUAL(crc0, crcFF);
}

/*============================================================================*
 *  Test 13: Two-byte data
 *============================================================================*/

static void test_CRC_TwoBytes(void)
{
    const uint8 data[2] = { 0xABu, 0xCDu };
    uint16 crc;

    crc = Fee_Crc_CalculateBlock(data, 2u);
    /* Verify determinism */
    TEST_ASSERT_EQUAL_HEX16(crc, Fee_Crc_CalculateBlock(data, 2u));
}

/*============================================================================*
 *  Main
 *============================================================================*/

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_CRC_KnownTestVector);
    RUN_TEST(test_CRC_KnownTestVector_ExplicitStart);
    RUN_TEST(test_CRC_EmptyData);
    RUN_TEST(test_CRC_SingleByteZero);
    RUN_TEST(test_CRC_SingleByteFF);
    RUN_TEST(test_CRC_AllZeroData);
    RUN_TEST(test_CRC_AllFFData);
    RUN_TEST(test_CRC_LargeData);
    RUN_TEST(test_CRC_IncrementalCalculation);
    RUN_TEST(test_CRC_IncrementalDifferentSplits);
    RUN_TEST(test_CRC_BlockHeaderBytes);
    RUN_TEST(test_CRC_CustomStartValue);
    RUN_TEST(test_CRC_TwoBytes);

    return UNITY_END();
}
