#include "unity.h"
#include "MemAcc.h"
#include "Det.h"
#include <string.h>

void setUp(void)
{
    MemAcc_TestResetFlash();
    MemAcc_Init();
    Det_ClearLastError();
}

void tearDown(void)
{
}

/* ---- MemAcc_Init tests ---- */

void test_MemAcc_Init_SetsStatusIdle(void)
{
    TEST_ASSERT_EQUAL(MEMIF_IDLE, MemAcc_GetStatus());
}

void test_MemAcc_Init_ReturnsOk(void)
{
    Std_ReturnType ret = MemAcc_Init();
    TEST_ASSERT_EQUAL(E_OK, ret);
}

void test_MemAcc_Init_DoesNotEraseFlash(void)
{
    /* Write some data to flash */
    uint8 data[] = {0xAAu, 0xBBu};
    MemAcc_Write(0u, data, 2u);

    /* Re-init should NOT erase the buffer */
    MemAcc_Init();

    uint8 readback[2];
    MemAcc_Read(0u, readback, 2u);
    TEST_ASSERT_EQUAL_HEX8(0xAAu, readback[0]);
    TEST_ASSERT_EQUAL_HEX8(0xBBu, readback[1]);
}

/* ---- MemAcc_TestResetFlash tests ---- */

void test_MemAcc_TestResetFlash_ErasesEntireBuffer(void)
{
    /* Write data first */
    uint8 data[] = {0x00u};
    MemAcc_Write(0u, data, 1u);

    /* Reset flash */
    MemAcc_TestResetFlash();

    /* Verify entire buffer is erased */
    TEST_ASSERT_EQUAL(E_OK, MemAcc_BlankCheck(0u, MEMACC_TOTAL_SIZE));
}

/* ---- MemAcc_Write / MemAcc_Read round-trip ---- */

void test_MemAcc_WriteRead_RoundTrip(void)
{
    uint8 write_data[] = {0xAAu, 0x55u, 0x00u, 0xFFu};
    uint8 read_data[4];

    Std_ReturnType ret = MemAcc_Write(0u, write_data, 4u);
    TEST_ASSERT_EQUAL(E_OK, ret);

    ret = MemAcc_Read(0u, read_data, 4u);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_HEX8(0xAAu, read_data[0]);
    TEST_ASSERT_EQUAL_HEX8(0x55u, read_data[1]);
    TEST_ASSERT_EQUAL_HEX8(0x00u, read_data[2]);
    TEST_ASSERT_EQUAL_HEX8(0xFFu, read_data[3]);
}

/* ---- Flash bit-clearing semantics ---- */

void test_MemAcc_Write_BitClearingSemantics(void)
{
    /* Write 0xAA (10101010) to erased byte (0xFF = 11111111) -- OK, only clears bits */
    uint8 data1 = 0xAAu;
    Std_ReturnType ret = MemAcc_Write(0u, &data1, 1u);
    TEST_ASSERT_EQUAL(E_OK, ret);

    /* Verify */
    uint8 readback;
    MemAcc_Read(0u, &readback, 1u);
    TEST_ASSERT_EQUAL_HEX8(0xAAu, readback);

    /* Write 0x00 (00000000) to 0xAA (10101010) -- OK, only clears bits */
    uint8 data2 = 0x00u;
    ret = MemAcc_Write(0u, &data2, 1u);
    TEST_ASSERT_EQUAL(E_OK, ret);

    MemAcc_Read(0u, &readback, 1u);
    TEST_ASSERT_EQUAL_HEX8(0x00u, readback);
}

void test_MemAcc_Write_RejectsBitSetting(void)
{
    /* Write 0x00 first */
    uint8 data1 = 0x00u;
    MemAcc_Write(0u, &data1, 1u);

    /* Try to write 0xFF -- this would set bits from 0 to 1, must fail */
    uint8 data2 = 0xFFu;
    Std_ReturnType ret = MemAcc_Write(0u, &data2, 1u);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(MEMIF_JOB_FAILED, MemAcc_GetJobResult());

    /* Verify byte is unchanged */
    uint8 readback;
    MemAcc_Read(0u, &readback, 1u);
    TEST_ASSERT_EQUAL_HEX8(0x00u, readback);
}

void test_MemAcc_Write_RejectsPartialBitSetting(void)
{
    /* Write 0xAA (10101010) first */
    uint8 data1 = 0xAAu;
    MemAcc_Write(0u, &data1, 1u);

    /* Try to write 0x55 (01010101) -- bit 0 of 0xAA is 0, 0x55 wants it as 1 => fail */
    uint8 data2 = 0x55u;
    Std_ReturnType ret = MemAcc_Write(0u, &data2, 1u);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

void test_MemAcc_Write_AllowsSameValue(void)
{
    /* Write 0xAA */
    uint8 data = 0xAAu;
    MemAcc_Write(0u, &data, 1u);

    /* Write 0xAA again -- no bits change, should succeed */
    Std_ReturnType ret = MemAcc_Write(0u, &data, 1u);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

void test_MemAcc_Write_NullPointerFails(void)
{
    Std_ReturnType ret = MemAcc_Write(0u, NULL_PTR, 1u);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(MEMIF_JOB_FAILED, MemAcc_GetJobResult());
}

void test_MemAcc_Write_OutOfBoundsFails(void)
{
    uint8 data = 0xAAu;
    Std_ReturnType ret = MemAcc_Write(MEMACC_TOTAL_SIZE, &data, 1u);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(MEMIF_JOB_FAILED, MemAcc_GetJobResult());
}

/* ---- MemAcc_Read validation ---- */

void test_MemAcc_Read_NullPointerFails(void)
{
    Std_ReturnType ret = MemAcc_Read(0u, NULL_PTR, 1u);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(MEMIF_JOB_FAILED, MemAcc_GetJobResult());
}

void test_MemAcc_Read_OutOfBoundsFails(void)
{
    uint8 buf;
    Std_ReturnType ret = MemAcc_Read(MEMACC_TOTAL_SIZE, &buf, 1u);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(MEMIF_JOB_FAILED, MemAcc_GetJobResult());
}

/* ---- MemAcc_Erase tests ---- */

void test_MemAcc_Erase_ResetsSectorTo0xFF(void)
{
    /* Write data to sector 0 */
    uint8 data = 0x00u;
    MemAcc_Write(0u, &data, 1u);

    /* Erase sector 0 */
    Std_ReturnType ret = MemAcc_Erase(0u, MEMACC_SECTOR_SIZE);
    TEST_ASSERT_EQUAL(E_OK, ret);

    /* Verify sector is blank */
    TEST_ASSERT_EQUAL(E_OK, MemAcc_BlankCheck(0u, MEMACC_SECTOR_SIZE));
}

void test_MemAcc_Erase_RejectsUnalignedAddress(void)
{
    Std_ReturnType ret = MemAcc_Erase(1u, MEMACC_SECTOR_SIZE);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(MEMIF_JOB_FAILED, MemAcc_GetJobResult());
}

void test_MemAcc_Erase_RejectsNonMultipleLength(void)
{
    Std_ReturnType ret = MemAcc_Erase(0u, MEMACC_SECTOR_SIZE + 1u);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(MEMIF_JOB_FAILED, MemAcc_GetJobResult());
}

void test_MemAcc_Erase_RejectsOutOfBounds(void)
{
    Std_ReturnType ret = MemAcc_Erase(MEMACC_TOTAL_SIZE, MEMACC_SECTOR_SIZE);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

void test_MemAcc_Erase_AllowsWriteAfterErase(void)
{
    /* Write 0x00, erase, then write 0xFF -- should work since erase resets to 0xFF */
    uint8 data1 = 0x00u;
    MemAcc_Write(0u, &data1, 1u);

    MemAcc_Erase(0u, MEMACC_SECTOR_SIZE);

    uint8 data2 = 0xAAu;
    Std_ReturnType ret = MemAcc_Write(0u, &data2, 1u);
    TEST_ASSERT_EQUAL(E_OK, ret);

    uint8 readback;
    MemAcc_Read(0u, &readback, 1u);
    TEST_ASSERT_EQUAL_HEX8(0xAAu, readback);
}

/* ---- MemAcc_BlankCheck tests ---- */

void test_MemAcc_BlankCheck_ReturnsOkOnBlank(void)
{
    TEST_ASSERT_EQUAL(E_OK, MemAcc_BlankCheck(0u, MEMACC_SECTOR_SIZE));
}

void test_MemAcc_BlankCheck_ReturnsNotOkOnNonBlank(void)
{
    uint8 data = 0x00u;
    MemAcc_Write(0u, &data, 1u);

    TEST_ASSERT_EQUAL(E_NOT_OK, MemAcc_BlankCheck(0u, 1u));
}

void test_MemAcc_BlankCheck_OutOfBoundsFails(void)
{
    Std_ReturnType ret = MemAcc_BlankCheck(MEMACC_TOTAL_SIZE, 1u);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/* ---- MemAcc_GetRawBuffer test ---- */

void test_MemAcc_GetRawBuffer_ReturnsNonNull(void)
{
    TEST_ASSERT_NOT_NULL(MemAcc_GetRawBuffer());
}

void test_MemAcc_GetRawBuffer_MatchesWrites(void)
{
    uint8 data = 0xAAu;
    MemAcc_Write(100u, &data, 1u);

    uint8* buf = MemAcc_GetRawBuffer();
    TEST_ASSERT_EQUAL_HEX8(0xAAu, buf[100]);
}

/* ---- Det stub tests ---- */

void test_Det_ReportError_StoresValues(void)
{
    Det_ReportError(21u, 0u, 0x03u, 0x01u);
    TEST_ASSERT_EQUAL(21u, Det_GetLastModuleId());
    TEST_ASSERT_EQUAL(0x03u, Det_GetLastApiId());
    TEST_ASSERT_EQUAL(0x01u, Det_GetLastErrorId());
}

void test_Det_ClearLastError_ResetsAll(void)
{
    Det_ReportError(21u, 0u, 0x03u, 0x01u);
    Det_ClearLastError();
    TEST_ASSERT_EQUAL(0u, Det_GetLastModuleId());
    TEST_ASSERT_EQUAL(0u, Det_GetLastApiId());
    TEST_ASSERT_EQUAL(0u, Det_GetLastErrorId());
}

int main(void)
{
    UNITY_BEGIN();

    /* Init */
    RUN_TEST(test_MemAcc_Init_SetsStatusIdle);
    RUN_TEST(test_MemAcc_Init_ReturnsOk);
    RUN_TEST(test_MemAcc_Init_DoesNotEraseFlash);

    /* TestResetFlash */
    RUN_TEST(test_MemAcc_TestResetFlash_ErasesEntireBuffer);

    /* Write/Read round-trip */
    RUN_TEST(test_MemAcc_WriteRead_RoundTrip);

    /* Flash bit-clearing semantics */
    RUN_TEST(test_MemAcc_Write_BitClearingSemantics);
    RUN_TEST(test_MemAcc_Write_RejectsBitSetting);
    RUN_TEST(test_MemAcc_Write_RejectsPartialBitSetting);
    RUN_TEST(test_MemAcc_Write_AllowsSameValue);
    RUN_TEST(test_MemAcc_Write_NullPointerFails);
    RUN_TEST(test_MemAcc_Write_OutOfBoundsFails);

    /* Read validation */
    RUN_TEST(test_MemAcc_Read_NullPointerFails);
    RUN_TEST(test_MemAcc_Read_OutOfBoundsFails);

    /* Erase */
    RUN_TEST(test_MemAcc_Erase_ResetsSectorTo0xFF);
    RUN_TEST(test_MemAcc_Erase_RejectsUnalignedAddress);
    RUN_TEST(test_MemAcc_Erase_RejectsNonMultipleLength);
    RUN_TEST(test_MemAcc_Erase_RejectsOutOfBounds);
    RUN_TEST(test_MemAcc_Erase_AllowsWriteAfterErase);

    /* BlankCheck */
    RUN_TEST(test_MemAcc_BlankCheck_ReturnsOkOnBlank);
    RUN_TEST(test_MemAcc_BlankCheck_ReturnsNotOkOnNonBlank);
    RUN_TEST(test_MemAcc_BlankCheck_OutOfBoundsFails);

    /* GetRawBuffer */
    RUN_TEST(test_MemAcc_GetRawBuffer_ReturnsNonNull);
    RUN_TEST(test_MemAcc_GetRawBuffer_MatchesWrites);

    /* Det */
    RUN_TEST(test_Det_ReportError_StoresValues);
    RUN_TEST(test_Det_ClearLastError_ResetsAll);

    return UNITY_END();
}
