/**
 * \file       test_Fee_JobQueue.c
 * \brief      Unit Tests -- Fee Types, Configuration, and Job Acceptance Model
 *
 * \details    Verifies Fee_Types structures, Fee_Config table correctness,
 *             block lookup by number, block sizes, immediate flags, config
 *             pointer validity, and job acceptance rules. Since the actual
 *             job acceptance state machine is in Fee.c (Task 5), these tests
 *             exercise the configuration data and type definitions that
 *             underpin the single-job acceptance model.
 *
 * \copyright  Copyright (c) 2024 -- All rights reserved.
 */

/*============================================================================*
 *  Includes
 *============================================================================*/

#include "unity/unity.h"
#include "Fee_Types.h"
#include "Fee_Cfg.h"
#include "Fee_PBcfg.h"
#include "Fee.h"
#include "Fee_StateMachine.h"
#include "Fee_Internal.h"

/* Block lookup uses Fee_Internal_FindBlockIndex (defined in Fee.c) which
 * operates on the global Fee_ConfigPtr.  setUp() points it at Fee_Config. */

/*============================================================================*
 *  Setup / Teardown
 *============================================================================*/

void setUp(void)
{
    /* Fee_Internal_FindBlockIndex requires Fee_ConfigPtr */
    Fee_ConfigPtr = &Fee_Config;
}

void tearDown(void)
{
    /* Nothing to clean up */
}

/*============================================================================*
 *  Test 1: Config table has exactly 8 blocks
 *============================================================================*/

static void test_ConfigTable_NumberOfBlocks(void)
{
    TEST_ASSERT_EQUAL_UINT16(8u, Fee_Config.NumberOfBlocks);
    TEST_ASSERT_EQUAL_UINT16(FEE_NUMBER_OF_BLOCKS, Fee_Config.NumberOfBlocks);
}

/*============================================================================*
 *  Test 2: Config pointer is valid (non-NULL)
 *============================================================================*/

static void test_ConfigPointer_Valid(void)
{
    TEST_ASSERT_NOT_NULL(Fee_Config.BlockConfigTable);
}

/*============================================================================*
 *  Test 3: Block numbers match (1 through 8)
 *============================================================================*/

static void test_BlockNumbers_Match(void)
{
    uint16 idx;
    for (idx = 0u; idx < Fee_Config.NumberOfBlocks; idx++)
    {
        TEST_ASSERT_EQUAL_UINT16(idx + 1u,
            Fee_Config.BlockConfigTable[idx].BlockNumber);
    }
}

/*============================================================================*
 *  Test 4: Block sizes match specification
 *============================================================================*/

static void test_BlockSizes_Match(void)
{
    static const uint16 ExpectedSizes[8] = {
        32u, 64u, 128u, 256u, 16u, 8u, 512u, 64u
    };
    uint16 idx;
    for (idx = 0u; idx < Fee_Config.NumberOfBlocks; idx++)
    {
        TEST_ASSERT_EQUAL_UINT16(ExpectedSizes[idx],
            Fee_Config.BlockConfigTable[idx].BlockSize);
    }
}

/*============================================================================*
 *  Test 5: Immediate flags correct (blocks 5 and 6 only)
 *============================================================================*/

static void test_ImmediateFlags_Correct(void)
{
    /* Blocks 1-4: not immediate */
    TEST_ASSERT_EQUAL(FALSE, Fee_Config.BlockConfigTable[0].ImmediateData);
    TEST_ASSERT_EQUAL(FALSE, Fee_Config.BlockConfigTable[1].ImmediateData);
    TEST_ASSERT_EQUAL(FALSE, Fee_Config.BlockConfigTable[2].ImmediateData);
    TEST_ASSERT_EQUAL(FALSE, Fee_Config.BlockConfigTable[3].ImmediateData);

    /* Block 5: immediate */
    TEST_ASSERT_EQUAL(TRUE, Fee_Config.BlockConfigTable[4].ImmediateData);

    /* Block 6: immediate */
    TEST_ASSERT_EQUAL(TRUE, Fee_Config.BlockConfigTable[5].ImmediateData);

    /* Blocks 7-8: not immediate */
    TEST_ASSERT_EQUAL(FALSE, Fee_Config.BlockConfigTable[6].ImmediateData);
    TEST_ASSERT_EQUAL(FALSE, Fee_Config.BlockConfigTable[7].ImmediateData);
}

/*============================================================================*
 *  Test 6: Block lookup finds correct index for each block
 *============================================================================*/

static void test_BlockLookup_FindsCorrectIndex(void)
{
    uint16 idx;
    for (idx = 0u; idx < Fee_Config.NumberOfBlocks; idx++)
    {
        uint16 found;
        found = Fee_Internal_FindBlockIndex(idx + 1u);
        TEST_ASSERT_EQUAL_UINT16(idx, found);
    }
}

/*============================================================================*
 *  Test 7: Block lookup returns INVALID for unknown block number
 *============================================================================*/

static void test_BlockLookup_InvalidBlockNumber(void)
{
    uint16 found;

    /* Block 0 does not exist */
    found = Fee_Internal_FindBlockIndex(0u);
    TEST_ASSERT_EQUAL_UINT16(FEE_BLOCK_INDEX_INVALID, found);

    /* Block 9 does not exist */
    found = Fee_Internal_FindBlockIndex(9u);
    TEST_ASSERT_EQUAL_UINT16(FEE_BLOCK_INDEX_INVALID, found);

    /* Block 0xFFFF does not exist */
    found = Fee_Internal_FindBlockIndex(0xFFFFu);
    TEST_ASSERT_EQUAL_UINT16(FEE_BLOCK_INDEX_INVALID, found);
}

/*============================================================================*
 *  Test 8: Block lookup with NULL config returns INVALID
 *============================================================================*/

static void test_BlockLookup_NullConfig(void)
{
    uint16 found;
    P2CONST(Fee_ConfigType, AUTOMATIC, FEE_CONST) savedPtr = Fee_ConfigPtr;

    Fee_ConfigPtr = NULL_PTR;
    found = Fee_Internal_FindBlockIndex(1u);
    TEST_ASSERT_EQUAL_UINT16(FEE_BLOCK_INDEX_INVALID, found);

    Fee_ConfigPtr = savedPtr;
}

/*============================================================================*
 *  Test 9: Config VirtualPageSize matches
 *============================================================================*/

static void test_Config_VirtualPageSize(void)
{
    TEST_ASSERT_EQUAL_UINT16(FEE_VIRTUAL_PAGE_SIZE, Fee_Config.VirtualPageSize);
    TEST_ASSERT_EQUAL_UINT16(32u, Fee_Config.VirtualPageSize);
}

/*============================================================================*
 *  Test 10: Config sector parameters match
 *============================================================================*/

static void test_Config_SectorParameters(void)
{
    TEST_ASSERT_EQUAL_UINT16(FEE_NUMBER_OF_SECTORS, Fee_Config.NumberOfSectors);
    TEST_ASSERT_EQUAL_UINT16(4u, Fee_Config.NumberOfSectors);
    TEST_ASSERT_EQUAL_UINT32(FEE_SECTOR_SIZE, Fee_Config.SectorSize);
    TEST_ASSERT_EQUAL_UINT32(16384u, Fee_Config.SectorSize);
}

/*============================================================================*
 *  Test 11: Config MemAccAreaId is zero
 *============================================================================*/

static void test_Config_MemAccAreaId(void)
{
    TEST_ASSERT_EQUAL_UINT8(0u, Fee_Config.MemAccAreaId);
}

/*============================================================================*
 *  Test 12: All NumberOfWriteCycles are zero (unlimited)
 *============================================================================*/

static void test_BlockWriteCycles_AllUnlimited(void)
{
    uint16 idx;
    for (idx = 0u; idx < Fee_Config.NumberOfBlocks; idx++)
    {
        TEST_ASSERT_EQUAL_UINT8(0u,
            Fee_Config.BlockConfigTable[idx].NumberOfWriteCycles);
    }
}

/*============================================================================*
 *  Test 13: No block exceeds FEE_MAX_BLOCK_SIZE
 *============================================================================*/

static void test_BlockSizes_WithinMaxLimit(void)
{
    uint16 idx;
    for (idx = 0u; idx < Fee_Config.NumberOfBlocks; idx++)
    {
        TEST_ASSERT_TRUE(
            Fee_Config.BlockConfigTable[idx].BlockSize <= FEE_MAX_BLOCK_SIZE);
    }
}

/*============================================================================*
 *  Test 14: Fee_JobInfoType can be initialized and read back
 *============================================================================*/

static void test_JobInfoType_InitAndReadBack(void)
{
    Fee_JobInfoType job;
    uint8 readBuf[4];
    static const uint8 writeBuf[4] = { 0xAAu, 0xBBu, 0xCCu, 0xDDu };

    job.Type = FEE_JOB_READ;
    job.BlockNumber = 3u;
    job.BlockOffset = 10u;
    job.Length = 4u;
    job.ReadDataPtr = readBuf;
    job.WriteDataPtr = writeBuf;
    job.IsImmediate = FALSE;

    TEST_ASSERT_EQUAL(FEE_JOB_READ, job.Type);
    TEST_ASSERT_EQUAL_UINT16(3u, job.BlockNumber);
    TEST_ASSERT_EQUAL_UINT16(10u, job.BlockOffset);
    TEST_ASSERT_EQUAL_UINT16(4u, job.Length);
    TEST_ASSERT_EQUAL_PTR(readBuf, job.ReadDataPtr);
    TEST_ASSERT_EQUAL_PTR(writeBuf, job.WriteDataPtr);
    TEST_ASSERT_EQUAL(FALSE, job.IsImmediate);
}

/*============================================================================*
 *  Test 15: Fee_BlockInfoType can be initialized and read back
 *============================================================================*/

static void test_BlockInfoType_InitAndReadBack(void)
{
    Fee_BlockInfoType info;

    info.Status = FEE_BLOCK_VALID;
    info.HeaderAddress = 0x1000u;
    info.DataAddress = 0x1020u;
    info.DataLength = 64u;
    info.DataCrc = 0xABCDu;
    info.SequenceCounter = 5u;
    info.SectorIndex = 1u;

    TEST_ASSERT_EQUAL(FEE_BLOCK_VALID, info.Status);
    TEST_ASSERT_EQUAL_UINT32(0x1000u, info.HeaderAddress);
    TEST_ASSERT_EQUAL_UINT32(0x1020u, info.DataAddress);
    TEST_ASSERT_EQUAL_UINT16(64u, info.DataLength);
    TEST_ASSERT_EQUAL_UINT16(0xABCDu, info.DataCrc);
    TEST_ASSERT_EQUAL_UINT16(5u, info.SequenceCounter);
    TEST_ASSERT_EQUAL_UINT8(1u, info.SectorIndex);
}

/*============================================================================*
 *  Test 16: Fee_SectorInfoType can be initialized and read back
 *============================================================================*/

static void test_SectorInfoType_InitAndReadBack(void)
{
    Fee_SectorInfoType sector;

    sector.Status = FEE_SECTOR_ACTIVE;
    sector.BaseAddress = 0x4000u;
    sector.WritePointer = 0x4200u;
    sector.SequenceNumber = 42u;
    sector.EraseCount = 100u;
    sector.FreeSpace = 1024u;

    TEST_ASSERT_EQUAL(FEE_SECTOR_ACTIVE, sector.Status);
    TEST_ASSERT_EQUAL_UINT32(0x4000u, sector.BaseAddress);
    TEST_ASSERT_EQUAL_UINT32(0x4200u, sector.WritePointer);
    TEST_ASSERT_EQUAL_UINT32(42u, sector.SequenceNumber);
    TEST_ASSERT_EQUAL_UINT16(100u, sector.EraseCount);
    TEST_ASSERT_EQUAL_UINT16(1024u, sector.FreeSpace);
}

/*============================================================================*
 *  Test 17: Enum values are correct
 *============================================================================*/

static void test_EnumValues_Correct(void)
{
    /* Fee_BlockStatusType */
    TEST_ASSERT_EQUAL_INT(0, FEE_BLOCK_NOT_FOUND);
    TEST_ASSERT_EQUAL_INT(1, FEE_BLOCK_VALID);
    TEST_ASSERT_EQUAL_INT(2, FEE_BLOCK_INVALID);
    TEST_ASSERT_EQUAL_INT(3, FEE_BLOCK_INCONSISTENT);

    /* Fee_SectorStatusType */
    TEST_ASSERT_EQUAL_INT(0, FEE_SECTOR_ERASED);
    TEST_ASSERT_EQUAL_INT(1, FEE_SECTOR_ACTIVE);
    TEST_ASSERT_EQUAL_INT(2, FEE_SECTOR_FULL);
    TEST_ASSERT_EQUAL_INT(3, FEE_SECTOR_DEFECTIVE);

    /* Fee_JobType */
    TEST_ASSERT_EQUAL_INT(0, FEE_JOB_NONE);
    TEST_ASSERT_EQUAL_INT(1, FEE_JOB_READ);
    TEST_ASSERT_EQUAL_INT(2, FEE_JOB_WRITE);
    TEST_ASSERT_EQUAL_INT(3, FEE_JOB_INVALIDATE);
    TEST_ASSERT_EQUAL_INT(4, FEE_JOB_ERASE_IMMEDIATE);
}

/*============================================================================*
 *  Test 18: DET error code values
 *============================================================================*/

static void test_DetErrorCodes(void)
{
    TEST_ASSERT_EQUAL_HEX8(0x01u, FEE_E_UNINIT);
    TEST_ASSERT_EQUAL_HEX8(0x02u, FEE_E_INVALID_BLOCK_NO);
    TEST_ASSERT_EQUAL_HEX8(0x03u, FEE_E_INVALID_BLOCK_OFS);
    TEST_ASSERT_EQUAL_HEX8(0x04u, FEE_E_PARAM_POINTER);
    TEST_ASSERT_EQUAL_HEX8(0x05u, FEE_E_INVALID_BLOCK_LEN);
    TEST_ASSERT_EQUAL_HEX8(0x06u, FEE_E_BUSY);
    TEST_ASSERT_EQUAL_HEX8(0x07u, FEE_E_INVALID_CANCEL);
    TEST_ASSERT_EQUAL_HEX8(0x10u, FEE_E_RAM_INTEGRITY);
}

/*============================================================================*
 *  Test 19: Constants have expected values
 *============================================================================*/

static void test_Constants(void)
{
    TEST_ASSERT_EQUAL_UINT16(21u, FEE_MODULE_ID);
    TEST_ASSERT_EQUAL_UINT16(32u, FEE_BLOCK_HEADER_SIZE);
    TEST_ASSERT_EQUAL_UINT16(32u, FEE_SECTOR_HEADER_SIZE);
    TEST_ASSERT_EQUAL_HEX8(0x00u, FEE_MARKER_ERASED);
    TEST_ASSERT_EQUAL_HEX8(0x55u, FEE_MARKER_VALID);
    TEST_ASSERT_EQUAL_HEX8(0xFFu, FEE_MARKER_INVALID);
    TEST_ASSERT_EQUAL_HEX32(0xFEE0FEE0u, FEE_SECTOR_MAGIC);
    TEST_ASSERT_EQUAL_HEX16(0xFFFFu, FEE_BLOCK_INDEX_INVALID);
    TEST_ASSERT_EQUAL_HEX16(0x1021u, FEE_CRC_POLYNOMIAL);
    TEST_ASSERT_EQUAL_HEX16(0xFFFFu, FEE_CRC_INITIAL);
}

/*============================================================================*
 *  Test 20: Configuration switches have expected values
 *============================================================================*/

static void test_ConfigSwitches(void)
{
    TEST_ASSERT_EQUAL(STD_ON, FEE_DEV_ERROR_DETECT);
    TEST_ASSERT_EQUAL(STD_ON, FEE_VERSION_INFO_API);
    TEST_ASSERT_EQUAL(STD_ON, FEE_POLLING_MODE);
    TEST_ASSERT_EQUAL(STD_ON, FEE_NVM_JOB_END_NOTIFICATION);
    TEST_ASSERT_EQUAL(STD_ON, FEE_NVM_JOB_ERROR_NOTIFICATION);
    TEST_ASSERT_EQUAL(STD_ON, FEE_SAFETY_ENABLE);
    TEST_ASSERT_EQUAL(STD_ON, FEE_READ_BACK_VERIFICATION);
    TEST_ASSERT_EQUAL_UINT16(32u, FEE_VIRTUAL_PAGE_SIZE);
    TEST_ASSERT_EQUAL_UINT16(8u, FEE_NUMBER_OF_BLOCKS);
    TEST_ASSERT_EQUAL_UINT16(4u, FEE_NUMBER_OF_SECTORS);
    TEST_ASSERT_EQUAL_UINT32(16384u, FEE_SECTOR_SIZE);
    TEST_ASSERT_EQUAL_UINT16(5u, FEE_MAIN_FUNCTION_PERIOD);
    TEST_ASSERT_EQUAL_UINT16(512u, FEE_MAX_BLOCK_SIZE);
    TEST_ASSERT_EQUAL_UINT16(80u, FEE_GC_RESTART_THRESHOLD);
}

/*============================================================================*
 *  Test 21: Block lookup for immediate blocks specifically
 *============================================================================*/

static void test_BlockLookup_ImmediateBlocks(void)
{
    uint16 idx5;
    uint16 idx6;

    idx5 = Fee_Internal_FindBlockIndex(5u);
    idx6 = Fee_Internal_FindBlockIndex(6u);

    TEST_ASSERT_NOT_EQUAL(FEE_BLOCK_INDEX_INVALID, idx5);
    TEST_ASSERT_NOT_EQUAL(FEE_BLOCK_INDEX_INVALID, idx6);

    TEST_ASSERT_EQUAL(TRUE, Fee_Config.BlockConfigTable[idx5].ImmediateData);
    TEST_ASSERT_EQUAL(TRUE, Fee_Config.BlockConfigTable[idx6].ImmediateData);
    TEST_ASSERT_EQUAL_UINT16(16u, Fee_Config.BlockConfigTable[idx5].BlockSize);
    TEST_ASSERT_EQUAL_UINT16(8u, Fee_Config.BlockConfigTable[idx6].BlockSize);
}

/*============================================================================*
 *  Test 22: Job type NONE initializes correctly for idle state
 *============================================================================*/

static void test_JobInfo_NoneType(void)
{
    Fee_JobInfoType job;

    job.Type = FEE_JOB_NONE;
    job.BlockNumber = 0u;
    job.BlockOffset = 0u;
    job.Length = 0u;
    job.ReadDataPtr = NULL_PTR;
    job.WriteDataPtr = NULL_PTR;
    job.IsImmediate = FALSE;

    TEST_ASSERT_EQUAL(FEE_JOB_NONE, job.Type);
    TEST_ASSERT_EQUAL_UINT16(0u, job.BlockNumber);
    TEST_ASSERT_NULL(job.ReadDataPtr);
    TEST_ASSERT_NULL(job.WriteDataPtr);
}

/*============================================================================*
 *  Main
 *============================================================================*/

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_ConfigTable_NumberOfBlocks);
    RUN_TEST(test_ConfigPointer_Valid);
    RUN_TEST(test_BlockNumbers_Match);
    RUN_TEST(test_BlockSizes_Match);
    RUN_TEST(test_ImmediateFlags_Correct);
    RUN_TEST(test_BlockLookup_FindsCorrectIndex);
    RUN_TEST(test_BlockLookup_InvalidBlockNumber);
    RUN_TEST(test_BlockLookup_NullConfig);
    RUN_TEST(test_Config_VirtualPageSize);
    RUN_TEST(test_Config_SectorParameters);
    RUN_TEST(test_Config_MemAccAreaId);
    RUN_TEST(test_BlockWriteCycles_AllUnlimited);
    RUN_TEST(test_BlockSizes_WithinMaxLimit);
    RUN_TEST(test_JobInfoType_InitAndReadBack);
    RUN_TEST(test_BlockInfoType_InitAndReadBack);
    RUN_TEST(test_SectorInfoType_InitAndReadBack);
    RUN_TEST(test_EnumValues_Correct);
    RUN_TEST(test_DetErrorCodes);
    RUN_TEST(test_Constants);
    RUN_TEST(test_ConfigSwitches);
    RUN_TEST(test_BlockLookup_ImmediateBlocks);
    RUN_TEST(test_JobInfo_NoneType);

    return UNITY_END();
}
