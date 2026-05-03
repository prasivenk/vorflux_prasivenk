/**
 * \file       test_Fee_Sector.c
 * \brief      Unit Tests -- Fee Sector Management
 *
 * \details    Tests sector header build/parse, block header build/parse,
 *             HeaderCRC validity across ValidMarker transitions, block
 *             allocation, write buffer preparation, fill percentage,
 *             LE16/LE32 round-trips, blank/corrupt detection.
 *
 * \copyright  Copyright (c) 2024 -- All rights reserved.
 */

/*============================================================================*
 *  Includes
 *============================================================================*/

#include "unity/unity.h"
#include "Fee_Sector.h"
#include "Fee_Crc.h"
#include <string.h>

/*============================================================================*
 *  Setup / Teardown
 *============================================================================*/

void setUp(void)
{
    /* Clear static buffers via getters */
    memset(Fee_Sector_GetHeaderBuffer(), 0x00, FEE_BLOCK_HEADER_SIZE);
    memset(Fee_Sector_GetWriteBuffer(), 0x00, FEE_MAX_BLOCK_SIZE + FEE_BLOCK_HEADER_SIZE);
}

void tearDown(void)
{
    /* Nothing to tear down */
}

/*============================================================================*
 *  Helper: create a default SectorInfo for allocation tests
 *============================================================================*/

static Fee_SectorInfoType CreateTestSector(
    MemAcc_AddressType baseAddr,
    uint16 freeSpace)
{
    Fee_SectorInfoType info;
    info.Status         = FEE_SECTOR_ACTIVE;
    info.BaseAddress    = baseAddr;
    info.WritePointer   = baseAddr + FEE_SECTOR_HEADER_SIZE;
    info.SequenceNumber = 1u;
    info.EraseCount     = 0u;
    info.FreeSpace      = freeSpace;
    return info;
}

/*============================================================================*
 *  1. Sector header build/parse round-trip
 *============================================================================*/

static void test_SectorHeader_BuildParse_RoundTrip(void)
{
    uint8 buf[32];
    Fee_SectorInfoType info;
    Std_ReturnType ret;

    Fee_Sector_BuildSectorHeader(buf, 42u, 7u);
    ret = Fee_Sector_ParseSectorHeader(buf, &info);

    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(42u, info.SequenceNumber);
    TEST_ASSERT_EQUAL_UINT16(7u, info.EraseCount);
}

/*============================================================================*
 *  2. Sector header with various sequence numbers and erase counts
 *============================================================================*/

static void test_SectorHeader_VariousValues(void)
{
    uint8 buf[32];
    Fee_SectorInfoType info;
    Std_ReturnType ret;

    /* Max sequence number */
    Fee_Sector_BuildSectorHeader(buf, 0xFFFFFFFFu, 0xFFFFu);
    ret = Fee_Sector_ParseSectorHeader(buf, &info);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(0xFFFFFFFFu, info.SequenceNumber);
    TEST_ASSERT_EQUAL_UINT16(0xFFFFu, info.EraseCount);

    /* Zero values */
    Fee_Sector_BuildSectorHeader(buf, 0u, 0u);
    ret = Fee_Sector_ParseSectorHeader(buf, &info);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(0u, info.SequenceNumber);
    TEST_ASSERT_EQUAL_UINT16(0u, info.EraseCount);

    /* Mid-range */
    Fee_Sector_BuildSectorHeader(buf, 12345u, 100u);
    ret = Fee_Sector_ParseSectorHeader(buf, &info);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(12345u, info.SequenceNumber);
    TEST_ASSERT_EQUAL_UINT16(100u, info.EraseCount);
}

/*============================================================================*
 *  3. Blank sector detection (all 0x00 -> parse returns E_NOT_OK)
 *============================================================================*/

static void test_SectorHeader_BlankSector(void)
{
    uint8 buf[32];
    Fee_SectorInfoType info;
    Std_ReturnType ret;

    memset(buf, 0x00, 32);
    ret = Fee_Sector_ParseSectorHeader(buf, &info);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/*============================================================================*
 *  4. Corrupt sector header (bad magic)
 *============================================================================*/

static void test_SectorHeader_CorruptMagic(void)
{
    uint8 buf[32];
    Fee_SectorInfoType info;
    Std_ReturnType ret;

    Fee_Sector_BuildSectorHeader(buf, 1u, 1u);
    /* Corrupt magic byte */
    buf[0] = 0xFFu;
    ret = Fee_Sector_ParseSectorHeader(buf, &info);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/*============================================================================*
 *  5. All-0xFF sector header (bad magic)
 *============================================================================*/

static void test_SectorHeader_AllFF(void)
{
    uint8 buf[32];
    Fee_SectorInfoType info;
    Std_ReturnType ret;

    memset(buf, 0xFF, 32);
    ret = Fee_Sector_ParseSectorHeader(buf, &info);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/*============================================================================*
 *  6. Sector header status byte is set to active (0x55)
 *============================================================================*/

static void test_SectorHeader_StatusActive(void)
{
    uint8 buf[32];
    Fee_SectorInfoType info;

    Fee_Sector_BuildSectorHeader(buf, 1u, 0u);
    (void)Fee_Sector_ParseSectorHeader(buf, &info);

    /* Status byte should be FEE_MARKER_VALID (0x55) = active */
    TEST_ASSERT_EQUAL_UINT8(FEE_MARKER_VALID, buf[10]);
}

/*============================================================================*
 *  7. Block header build/parse round-trip
 *============================================================================*/

static void test_BlockHeader_BuildParse_RoundTrip(void)
{
    uint8 buf[32];
    uint16 blockNum, blockLen, dataCrc, seqCnt;
    uint8  validMarker;
    Std_ReturnType ret;

    Fee_Sector_BuildBlockHeader(buf, 0x0001u, 64u, 0xABCDu, 5u, 3u);
    ret = Fee_Sector_ParseBlockHeader(buf, &blockNum, &blockLen, &dataCrc,
                                       &seqCnt, &validMarker);

    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT16(0x0001u, blockNum);
    TEST_ASSERT_EQUAL_UINT16(64u, blockLen);
    TEST_ASSERT_EQUAL_HEX16(0xABCDu, dataCrc);
    TEST_ASSERT_EQUAL_UINT16(5u, seqCnt);
    TEST_ASSERT_EQUAL_UINT8(FEE_MARKER_ERASED, validMarker);
}

/*============================================================================*
 *  8. Block header with max values
 *============================================================================*/

static void test_BlockHeader_MaxValues(void)
{
    uint8 buf[32];
    uint16 blockNum, blockLen, dataCrc, seqCnt;
    uint8  validMarker;
    Std_ReturnType ret;

    Fee_Sector_BuildBlockHeader(buf, 0xFFFFu, 0xFFFFu, 0xFFFFu, 0xFFFFu, 0xFFu);
    ret = Fee_Sector_ParseBlockHeader(buf, &blockNum, &blockLen, &dataCrc,
                                       &seqCnt, &validMarker);

    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_HEX16(0xFFFFu, blockNum);
    TEST_ASSERT_EQUAL_HEX16(0xFFFFu, blockLen);
    TEST_ASSERT_EQUAL_HEX16(0xFFFFu, dataCrc);
    TEST_ASSERT_EQUAL_HEX16(0xFFFFu, seqCnt);
}

/*============================================================================*
 *  9. Block header with zero values
 *============================================================================*/

static void test_BlockHeader_ZeroValues(void)
{
    uint8 buf[32];
    uint16 blockNum, blockLen, dataCrc, seqCnt;
    uint8  validMarker;
    Std_ReturnType ret;

    Fee_Sector_BuildBlockHeader(buf, 0u, 0u, 0u, 0u, 0u);
    ret = Fee_Sector_ParseBlockHeader(buf, &blockNum, &blockLen, &dataCrc,
                                       &seqCnt, &validMarker);

    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT16(0u, blockNum);
    TEST_ASSERT_EQUAL_UINT16(0u, blockLen);
    TEST_ASSERT_EQUAL_HEX16(0u, dataCrc);
    TEST_ASSERT_EQUAL_UINT16(0u, seqCnt);
}

/*============================================================================*
 *  10-12. HeaderCRC validity after ValidMarker transitions
 *  CRITICAL: CRC covers bytes 0-28 only. Byte 30 (ValidMarker) is excluded.
 *============================================================================*/

static void test_BlockHeader_ValidMarker_Erased(void)
{
    uint8 buf[32];
    uint16 blockNum, blockLen, dataCrc, seqCnt;
    uint8  validMarker;
    Std_ReturnType ret;

    Fee_Sector_BuildBlockHeader(buf, 100u, 32u, 0x1234u, 1u, 1u);
    /* Marker is 0x00 (erased) after build */
    TEST_ASSERT_EQUAL_UINT8(0x00u, buf[30]);

    ret = Fee_Sector_ParseBlockHeader(buf, &blockNum, &blockLen, &dataCrc,
                                       &seqCnt, &validMarker);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(FEE_MARKER_ERASED, validMarker);
}

static void test_BlockHeader_ValidMarker_TransitionToValid(void)
{
    uint8 buf[32];
    uint16 blockNum, blockLen, dataCrc, seqCnt;
    uint8  validMarker;
    Std_ReturnType ret;

    Fee_Sector_BuildBlockHeader(buf, 100u, 32u, 0x1234u, 1u, 1u);

    /* Transition marker: 0x00 -> 0x55 (valid) */
    buf[30] = FEE_MARKER_VALID;

    /* CRC should STILL be valid because byte 30 is excluded from CRC */
    ret = Fee_Sector_ParseBlockHeader(buf, &blockNum, &blockLen, &dataCrc,
                                       &seqCnt, &validMarker);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(FEE_MARKER_VALID, validMarker);
    TEST_ASSERT_EQUAL_UINT16(100u, blockNum);
}

static void test_BlockHeader_ValidMarker_TransitionToInvalid(void)
{
    uint8 buf[32];
    uint16 blockNum, blockLen, dataCrc, seqCnt;
    uint8  validMarker;
    Std_ReturnType ret;

    Fee_Sector_BuildBlockHeader(buf, 100u, 32u, 0x1234u, 1u, 1u);

    /* Transition marker: 0x00 -> 0xFF (invalid) */
    buf[30] = FEE_MARKER_INVALID;

    /* CRC should STILL be valid because byte 30 is excluded from CRC */
    ret = Fee_Sector_ParseBlockHeader(buf, &blockNum, &blockLen, &dataCrc,
                                       &seqCnt, &validMarker);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(FEE_MARKER_INVALID, validMarker);
    TEST_ASSERT_EQUAL_UINT16(100u, blockNum);
}

/*============================================================================*
 *  13. Block header with corrupt HeaderCRC -> parse returns E_NOT_OK
 *============================================================================*/

static void test_BlockHeader_CorruptCRC(void)
{
    uint8 buf[32];
    uint16 blockNum, blockLen, dataCrc, seqCnt;
    uint8  validMarker;
    Std_ReturnType ret;

    Fee_Sector_BuildBlockHeader(buf, 100u, 32u, 0x1234u, 1u, 1u);

    /* Corrupt CRC byte (byte 29) */
    buf[29] ^= 0xFFu;

    ret = Fee_Sector_ParseBlockHeader(buf, &blockNum, &blockLen, &dataCrc,
                                       &seqCnt, &validMarker);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/*============================================================================*
 *  14. Block header with corrupt data byte -> parse returns E_NOT_OK
 *============================================================================*/

static void test_BlockHeader_CorruptDataByte(void)
{
    uint8 buf[32];
    uint16 blockNum, blockLen, dataCrc, seqCnt;
    uint8  validMarker;
    Std_ReturnType ret;

    Fee_Sector_BuildBlockHeader(buf, 100u, 32u, 0x1234u, 1u, 1u);

    /* Corrupt a data byte (byte 5) */
    buf[5] ^= 0x01u;

    ret = Fee_Sector_ParseBlockHeader(buf, &blockNum, &blockLen, &dataCrc,
                                       &seqCnt, &validMarker);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/*============================================================================*
 *  15. AllocateBlock: allocate single block, verify address
 *============================================================================*/

static void test_AllocateBlock_SingleBlock(void)
{
    Fee_SectorInfoType sector;
    MemAcc_AddressType addr;

    /* Sector starting at 0x1000, 16KB - 32 bytes header = ~16352 free */
    sector = CreateTestSector(0x1000u, (uint16)(FEE_SECTOR_SIZE - FEE_SECTOR_HEADER_SIZE));

    addr = Fee_Sector_AllocateBlock(&sector, 64u, FEE_VIRTUAL_PAGE_SIZE);

    /* Should return the initial WritePointer */
    TEST_ASSERT_EQUAL_HEX32(0x1000u + FEE_SECTOR_HEADER_SIZE, addr);
    /* WritePointer should advance by header(32) + ALIGN_UP(64, 32) = 32 + 64 = 96 */
    TEST_ASSERT_EQUAL_HEX32(0x1000u + FEE_SECTOR_HEADER_SIZE + 96u, sector.WritePointer);
}

/*============================================================================*
 *  16. AllocateBlock: allocate until full, verify returns 0
 *============================================================================*/

static void test_AllocateBlock_UntilFull(void)
{
    Fee_SectorInfoType sector;
    MemAcc_AddressType addr;
    uint32 allocCount = 0u;

    /* Small sector for easy testing: 256 bytes free */
    sector = CreateTestSector(0x0000u, 256u);

    /* Each 32-byte block: 32 header + 32 data = 64 bytes. 256/64 = 4 blocks */
    while (1)
    {
        addr = Fee_Sector_AllocateBlock(&sector, 32u, FEE_VIRTUAL_PAGE_SIZE);
        if (addr == 0u)
        {
            break;
        }
        allocCount++;
    }

    TEST_ASSERT_EQUAL_UINT32(4u, allocCount);
    TEST_ASSERT_EQUAL_UINT16(0u, sector.FreeSpace);
}

/*============================================================================*
 *  17. AllocateBlock: 8-byte block
 *============================================================================*/

static void test_AllocateBlock_8Bytes(void)
{
    Fee_SectorInfoType sector;
    MemAcc_AddressType addr;

    sector = CreateTestSector(0x0000u, 1024u);
    addr = Fee_Sector_AllocateBlock(&sector, 8u, FEE_VIRTUAL_PAGE_SIZE);

    /* 8 bytes aligned to 32 = 32 data + 32 header = 64 total */
    TEST_ASSERT_NOT_EQUAL(0u, addr);
    TEST_ASSERT_EQUAL_UINT16(1024u - 64u, sector.FreeSpace);
}

/*============================================================================*
 *  18. AllocateBlock: 16-byte block
 *============================================================================*/

static void test_AllocateBlock_16Bytes(void)
{
    Fee_SectorInfoType sector;
    MemAcc_AddressType addr;

    sector = CreateTestSector(0x0000u, 1024u);
    addr = Fee_Sector_AllocateBlock(&sector, 16u, FEE_VIRTUAL_PAGE_SIZE);

    /* 16 bytes aligned to 32 = 32 data + 32 header = 64 total */
    TEST_ASSERT_NOT_EQUAL(0u, addr);
    TEST_ASSERT_EQUAL_UINT16(1024u - 64u, sector.FreeSpace);
}

/*============================================================================*
 *  19. AllocateBlock: 31-byte block (just under page size)
 *============================================================================*/

static void test_AllocateBlock_31Bytes(void)
{
    Fee_SectorInfoType sector;
    MemAcc_AddressType addr;

    sector = CreateTestSector(0x0000u, 1024u);
    addr = Fee_Sector_AllocateBlock(&sector, 31u, FEE_VIRTUAL_PAGE_SIZE);

    /* 31 bytes aligned to 32 = 32 data + 32 header = 64 total */
    TEST_ASSERT_NOT_EQUAL(0u, addr);
    TEST_ASSERT_EQUAL_UINT16(1024u - 64u, sector.FreeSpace);
}

/*============================================================================*
 *  20. AllocateBlock: 32-byte block (exactly page size)
 *============================================================================*/

static void test_AllocateBlock_32Bytes(void)
{
    Fee_SectorInfoType sector;
    MemAcc_AddressType addr;

    sector = CreateTestSector(0x0000u, 1024u);
    addr = Fee_Sector_AllocateBlock(&sector, 32u, FEE_VIRTUAL_PAGE_SIZE);

    /* 32 bytes aligned to 32 = 32 data + 32 header = 64 total */
    TEST_ASSERT_NOT_EQUAL(0u, addr);
    TEST_ASSERT_EQUAL_UINT16(1024u - 64u, sector.FreeSpace);
}

/*============================================================================*
 *  21. AllocateBlock: 33-byte block (just over page size)
 *============================================================================*/

static void test_AllocateBlock_33Bytes(void)
{
    Fee_SectorInfoType sector;
    MemAcc_AddressType addr;

    sector = CreateTestSector(0x0000u, 1024u);
    addr = Fee_Sector_AllocateBlock(&sector, 33u, FEE_VIRTUAL_PAGE_SIZE);

    /* 33 bytes aligned to 32 = 64 data + 32 header = 96 total */
    TEST_ASSERT_NOT_EQUAL(0u, addr);
    TEST_ASSERT_EQUAL_UINT16(1024u - 96u, sector.FreeSpace);
}

/*============================================================================*
 *  22. AllocateBlock: 512-byte block (max size)
 *============================================================================*/

static void test_AllocateBlock_512Bytes(void)
{
    Fee_SectorInfoType sector;
    MemAcc_AddressType addr;

    sector = CreateTestSector(0x0000u, (uint16)(FEE_SECTOR_SIZE - FEE_SECTOR_HEADER_SIZE));
    addr = Fee_Sector_AllocateBlock(&sector, 512u, FEE_VIRTUAL_PAGE_SIZE);

    /* 512 bytes aligned to 32 = 512 data + 32 header = 544 total */
    TEST_ASSERT_NOT_EQUAL(0u, addr);
    TEST_ASSERT_EQUAL_UINT16((uint16)(FEE_SECTOR_SIZE - FEE_SECTOR_HEADER_SIZE - 544u),
                              sector.FreeSpace);
}

/*============================================================================*
 *  23. AllocateBlock: not enough space returns 0
 *============================================================================*/

static void test_AllocateBlock_NotEnoughSpace(void)
{
    Fee_SectorInfoType sector;
    MemAcc_AddressType addr;

    /* Only 32 bytes free -- not enough for header(32) + any data */
    sector = CreateTestSector(0x0000u, 32u);
    addr = Fee_Sector_AllocateBlock(&sector, 8u, FEE_VIRTUAL_PAGE_SIZE);
    TEST_ASSERT_EQUAL(0u, addr);
}

/*============================================================================*
 *  24. PrepareWriteBuffer: 8-byte source padded to 32 bytes
 *============================================================================*/

static void test_PrepareWriteBuffer_8Bytes(void)
{
    uint8 src[8] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };
    uint16 paddedLen;
    const uint8* writeBuf;
    uint16 i;

    paddedLen = Fee_Sector_PrepareWriteBuffer(src, 8u, FEE_VIRTUAL_PAGE_SIZE);
    writeBuf = Fee_Sector_GetWriteBuffer();

    TEST_ASSERT_EQUAL_UINT16(32u, paddedLen);

    /* First 8 bytes should be the source data */
    TEST_ASSERT_EQUAL_MEMORY(src, writeBuf, 8);

    /* Remaining bytes should be 0x00 (padding) */
    for (i = 8u; i < 32u; i++)
    {
        TEST_ASSERT_EQUAL_UINT8(0x00u, writeBuf[i]);
    }
}

/*============================================================================*
 *  25. PrepareWriteBuffer: 32-byte source (no padding needed)
 *============================================================================*/

static void test_PrepareWriteBuffer_32Bytes(void)
{
    uint8 src[32];
    uint16 paddedLen;
    const uint8* writeBuf;

    memset(src, 0xAA, 32);
    paddedLen = Fee_Sector_PrepareWriteBuffer(src, 32u, FEE_VIRTUAL_PAGE_SIZE);
    writeBuf = Fee_Sector_GetWriteBuffer();

    TEST_ASSERT_EQUAL_UINT16(32u, paddedLen);
    TEST_ASSERT_EQUAL_MEMORY(src, writeBuf, 32);
}

/*============================================================================*
 *  26. PrepareWriteBuffer: 33-byte source padded to 64 bytes
 *============================================================================*/

static void test_PrepareWriteBuffer_33Bytes(void)
{
    uint8 src[33];
    uint16 paddedLen;
    const uint8* writeBuf;
    uint16 i;

    memset(src, 0xBB, 33);
    paddedLen = Fee_Sector_PrepareWriteBuffer(src, 33u, FEE_VIRTUAL_PAGE_SIZE);
    writeBuf = Fee_Sector_GetWriteBuffer();

    TEST_ASSERT_EQUAL_UINT16(64u, paddedLen);

    /* First 33 bytes should be source data */
    TEST_ASSERT_EQUAL_MEMORY(src, writeBuf, 33);

    /* Bytes 33-63 should be 0x00 */
    for (i = 33u; i < 64u; i++)
    {
        TEST_ASSERT_EQUAL_UINT8(0x00u, writeBuf[i]);
    }
}

/*============================================================================*
 *  27. GetFillPercentage: empty sector
 *============================================================================*/

static void test_GetFillPercentage_Empty(void)
{
    Fee_SectorInfoType sector;
    uint8 pct;

    sector = CreateTestSector(0x0000u, (uint16)(FEE_SECTOR_SIZE - FEE_SECTOR_HEADER_SIZE));
    pct = Fee_Sector_GetFillPercentage(&sector);
    TEST_ASSERT_EQUAL_UINT8(0u, pct);
}

/*============================================================================*
 *  28. GetFillPercentage: half-full sector
 *============================================================================*/

static void test_GetFillPercentage_HalfFull(void)
{
    Fee_SectorInfoType sector;
    uint8 pct;
    uint16 totalData;

    totalData = (uint16)(FEE_SECTOR_SIZE - FEE_SECTOR_HEADER_SIZE);
    sector = CreateTestSector(0x0000u, totalData);

    /* Simulate half-full: move WritePointer halfway */
    sector.WritePointer = sector.BaseAddress + FEE_SECTOR_HEADER_SIZE + (totalData / 2u);
    sector.FreeSpace = totalData - (totalData / 2u);

    pct = Fee_Sector_GetFillPercentage(&sector);
    TEST_ASSERT_EQUAL_UINT8(50u, pct);
}

/*============================================================================*
 *  29. GetFillPercentage: full sector
 *============================================================================*/

static void test_GetFillPercentage_Full(void)
{
    Fee_SectorInfoType sector;
    uint8 pct;
    uint16 totalData;

    totalData = (uint16)(FEE_SECTOR_SIZE - FEE_SECTOR_HEADER_SIZE);
    sector = CreateTestSector(0x0000u, totalData);

    /* Simulate full */
    sector.WritePointer = sector.BaseAddress + FEE_SECTOR_SIZE;
    sector.FreeSpace = 0u;

    pct = Fee_Sector_GetFillPercentage(&sector);
    TEST_ASSERT_EQUAL_UINT8(100u, pct);
}

/*============================================================================*
 *  30. LE16/LE32 round-trip via sector header build/parse
 *============================================================================*/

static void test_LE_RoundTrip_ViaSectorHeader(void)
{
    uint8 buf[32];
    Fee_SectorInfoType info;

    /* Test specific bit patterns for LE encoding */
    Fee_Sector_BuildSectorHeader(buf, 0xDEADBEEFu, 0x1234u);
    (void)Fee_Sector_ParseSectorHeader(buf, &info);

    TEST_ASSERT_EQUAL_HEX32(0xDEADBEEFu, info.SequenceNumber);
    TEST_ASSERT_EQUAL_HEX16(0x1234u, info.EraseCount);

    /* Verify raw LE bytes for magic */
    TEST_ASSERT_EQUAL_HEX8(0xE0u, buf[0]);
    TEST_ASSERT_EQUAL_HEX8(0xFEu, buf[1]);
    TEST_ASSERT_EQUAL_HEX8(0xE0u, buf[2]);
    TEST_ASSERT_EQUAL_HEX8(0xFEu, buf[3]);

    /* Verify raw LE bytes for sequence number */
    TEST_ASSERT_EQUAL_HEX8(0xEFu, buf[4]);
    TEST_ASSERT_EQUAL_HEX8(0xBEu, buf[5]);
    TEST_ASSERT_EQUAL_HEX8(0xADu, buf[6]);
    TEST_ASSERT_EQUAL_HEX8(0xDEu, buf[7]);

    /* Verify raw LE bytes for erase count */
    TEST_ASSERT_EQUAL_HEX8(0x34u, buf[8]);
    TEST_ASSERT_EQUAL_HEX8(0x12u, buf[9]);
}

/*============================================================================*
 *  31. LE16 round-trip via block header build/parse
 *============================================================================*/

static void test_LE16_RoundTrip_ViaBlockHeader(void)
{
    uint8 buf[32];
    uint16 blockNum, blockLen, dataCrc, seqCnt;
    uint8  validMarker;

    Fee_Sector_BuildBlockHeader(buf, 0xABCDu, 0x1234u, 0x5678u, 0x9ABCu, 0xEFu);
    (void)Fee_Sector_ParseBlockHeader(buf, &blockNum, &blockLen, &dataCrc,
                                       &seqCnt, &validMarker);

    TEST_ASSERT_EQUAL_HEX16(0xABCDu, blockNum);
    TEST_ASSERT_EQUAL_HEX16(0x1234u, blockLen);
    TEST_ASSERT_EQUAL_HEX16(0x5678u, dataCrc);
    TEST_ASSERT_EQUAL_HEX16(0x9ABCu, seqCnt);

    /* Verify raw LE bytes for BlockNumber */
    TEST_ASSERT_EQUAL_HEX8(0xCDu, buf[0]);
    TEST_ASSERT_EQUAL_HEX8(0xABu, buf[1]);
}

/*============================================================================*
 *  32. GetHeaderBuffer returns non-NULL
 *============================================================================*/

static void test_GetHeaderBuffer_NotNull(void)
{
    uint8* ptr = Fee_Sector_GetHeaderBuffer();
    TEST_ASSERT_NOT_NULL(ptr);
}

/*============================================================================*
 *  33. GetWriteBuffer returns non-NULL
 *============================================================================*/

static void test_GetWriteBuffer_NotNull(void)
{
    uint8* ptr = Fee_Sector_GetWriteBuffer();
    TEST_ASSERT_NOT_NULL(ptr);
}

/*============================================================================*
 *  34. Block header reserved bytes are 0x00
 *============================================================================*/

static void test_BlockHeader_ReservedBytes(void)
{
    uint8 buf[32];
    uint8 i;

    Fee_Sector_BuildBlockHeader(buf, 0x0001u, 64u, 0xABCDu, 5u, 3u);

    /* Bytes 8-27 should be reserved (0x00) */
    for (i = 8u; i < 28u; i++)
    {
        TEST_ASSERT_EQUAL_UINT8(0x00u, buf[i]);
    }

    /* Byte 31 reserved = 0x00 */
    TEST_ASSERT_EQUAL_UINT8(0x00u, buf[31]);
}

/*============================================================================*
 *  35. Block header WriteCounter at byte 28
 *============================================================================*/

static void test_BlockHeader_WriteCounter(void)
{
    uint8 buf[32];
    uint16 blockNum, blockLen, dataCrc, seqCnt;
    uint8  validMarker;

    Fee_Sector_BuildBlockHeader(buf, 1u, 32u, 0u, 0u, 42u);

    /* Byte 28 should contain WriteCounter */
    TEST_ASSERT_EQUAL_UINT8(42u, buf[28]);

    /* Parse should still work */
    TEST_ASSERT_EQUAL(E_OK,
        Fee_Sector_ParseBlockHeader(buf, &blockNum, &blockLen, &dataCrc,
                                     &seqCnt, &validMarker));
}

/*============================================================================*
 *  36. Sector header reserved bytes (11-31 except status byte) are 0x00
 *============================================================================*/

static void test_SectorHeader_ReservedBytes(void)
{
    uint8 buf[32];
    uint8 i;

    Fee_Sector_BuildSectorHeader(buf, 1u, 1u);

    /* Bytes 11-31 should be reserved (0x00) */
    for (i = 11u; i < 32u; i++)
    {
        TEST_ASSERT_EQUAL_UINT8(0x00u, buf[i]);
    }
}

/*============================================================================*
 *  37. AllocateBlock: multiple blocks with tracking
 *============================================================================*/

static void test_AllocateBlock_MultipleBlocks(void)
{
    Fee_SectorInfoType sector;
    MemAcc_AddressType addr1, addr2, addr3;

    sector = CreateTestSector(0x0000u, 1024u);

    /* Block 1: 32 bytes -> total 64 */
    addr1 = Fee_Sector_AllocateBlock(&sector, 32u, FEE_VIRTUAL_PAGE_SIZE);
    /* Block 2: 64 bytes -> total 96 */
    addr2 = Fee_Sector_AllocateBlock(&sector, 64u, FEE_VIRTUAL_PAGE_SIZE);
    /* Block 3: 128 bytes -> total 160 */
    addr3 = Fee_Sector_AllocateBlock(&sector, 128u, FEE_VIRTUAL_PAGE_SIZE);

    TEST_ASSERT_EQUAL_HEX32(FEE_SECTOR_HEADER_SIZE, addr1);
    TEST_ASSERT_EQUAL_HEX32(FEE_SECTOR_HEADER_SIZE + 64u, addr2);
    TEST_ASSERT_EQUAL_HEX32(FEE_SECTOR_HEADER_SIZE + 64u + 96u, addr3);

    /* FreeSpace: 1024 - 64 - 96 - 160 = 704 */
    TEST_ASSERT_EQUAL_UINT16(704u, sector.FreeSpace);
}

/*============================================================================*
 *  38. PrepareWriteBuffer: 1-byte source padded to 32 bytes
 *============================================================================*/

static void test_PrepareWriteBuffer_1Byte(void)
{
    uint8 src[1] = { 0xAA };
    uint16 paddedLen;
    const uint8* writeBuf;

    paddedLen = Fee_Sector_PrepareWriteBuffer(src, 1u, FEE_VIRTUAL_PAGE_SIZE);
    writeBuf = Fee_Sector_GetWriteBuffer();

    TEST_ASSERT_EQUAL_UINT16(32u, paddedLen);
    TEST_ASSERT_EQUAL_UINT8(0xAA, writeBuf[0]);
    TEST_ASSERT_EQUAL_UINT8(0x00, writeBuf[1]);
    TEST_ASSERT_EQUAL_UINT8(0x00, writeBuf[31]);
}

/*============================================================================*
 *  Main
 *============================================================================*/

int main(void)
{
    UNITY_BEGIN();

    /* Sector header tests */
    RUN_TEST(test_SectorHeader_BuildParse_RoundTrip);
    RUN_TEST(test_SectorHeader_VariousValues);
    RUN_TEST(test_SectorHeader_BlankSector);
    RUN_TEST(test_SectorHeader_CorruptMagic);
    RUN_TEST(test_SectorHeader_AllFF);
    RUN_TEST(test_SectorHeader_StatusActive);
    RUN_TEST(test_SectorHeader_ReservedBytes);

    /* Block header tests */
    RUN_TEST(test_BlockHeader_BuildParse_RoundTrip);
    RUN_TEST(test_BlockHeader_MaxValues);
    RUN_TEST(test_BlockHeader_ZeroValues);
    RUN_TEST(test_BlockHeader_ValidMarker_Erased);
    RUN_TEST(test_BlockHeader_ValidMarker_TransitionToValid);
    RUN_TEST(test_BlockHeader_ValidMarker_TransitionToInvalid);
    RUN_TEST(test_BlockHeader_CorruptCRC);
    RUN_TEST(test_BlockHeader_CorruptDataByte);
    RUN_TEST(test_BlockHeader_ReservedBytes);
    RUN_TEST(test_BlockHeader_WriteCounter);

    /* LE round-trip tests */
    RUN_TEST(test_LE_RoundTrip_ViaSectorHeader);
    RUN_TEST(test_LE16_RoundTrip_ViaBlockHeader);

    /* Allocation tests */
    RUN_TEST(test_AllocateBlock_SingleBlock);
    RUN_TEST(test_AllocateBlock_UntilFull);
    RUN_TEST(test_AllocateBlock_8Bytes);
    RUN_TEST(test_AllocateBlock_16Bytes);
    RUN_TEST(test_AllocateBlock_31Bytes);
    RUN_TEST(test_AllocateBlock_32Bytes);
    RUN_TEST(test_AllocateBlock_33Bytes);
    RUN_TEST(test_AllocateBlock_512Bytes);
    RUN_TEST(test_AllocateBlock_NotEnoughSpace);
    RUN_TEST(test_AllocateBlock_MultipleBlocks);

    /* Write buffer tests */
    RUN_TEST(test_PrepareWriteBuffer_8Bytes);
    RUN_TEST(test_PrepareWriteBuffer_32Bytes);
    RUN_TEST(test_PrepareWriteBuffer_33Bytes);
    RUN_TEST(test_PrepareWriteBuffer_1Byte);

    /* Fill percentage tests */
    RUN_TEST(test_GetFillPercentage_Empty);
    RUN_TEST(test_GetFillPercentage_HalfFull);
    RUN_TEST(test_GetFillPercentage_Full);

    /* Buffer accessor tests */
    RUN_TEST(test_GetHeaderBuffer_NotNull);
    RUN_TEST(test_GetWriteBuffer_NotNull);

    return UNITY_END();
}
