# Fee Module Test Plan — AUTOSAR R24-11

## 1. Overview

This test plan covers the AUTOSAR R24-11 Fee (Flash EEPROM Emulation) module
and its integration with the MemAcc (Memory Access) module. Tests are
organized by functional area and traceable to SWS requirements.

## 2. Test Case Index

### 2.1 API Layer Tests (`test/unit/test_Fee_Api.c`)

| Test Case ID     | Description | Function | SWS Ref |
|------------------|-------------|----------|---------|
| TC_FEE_API_001 | Fee_Init with NULL config reports DET | `test_Init_NullConfig` | SWS_Fee_00072 |
| TC_FEE_API_002 | Fee_Init with valid config sets BUSY_INTERNAL | `test_Init_ValidConfig` | SWS_Fee_00073 |
| TC_FEE_API_003 | Fee_Read before init reports DET UNINIT | `test_Read_Uninit` | SWS_Fee_00074 |
| TC_FEE_API_004 | Fee_Read with invalid block number | `test_Read_InvalidBlockNumber` | SWS_Fee_00076 |
| TC_FEE_API_005 | Fee_Read with NULL pointer | `test_Read_NullPointer` | SWS_Fee_00077 |
| TC_FEE_API_006 | Fee_Read with zero length | `test_Read_ZeroLength` | SWS_Fee_00078 |
| TC_FEE_API_007 | Fee_Read with offset+length > blockSize | `test_Read_InvalidOffset` | SWS_Fee_00079 |
| TC_FEE_API_008 | Fee_Read acceptance when idle | `test_Read_AcceptWhenIdle` | SWS_Fee_00080 |
| TC_FEE_API_009 | Fee_Read rejection when busy | `test_Read_RejectionWhenBusy` | SWS_Fee_00081 |
| TC_FEE_API_010 | Fee_Write before init reports DET UNINIT | `test_Write_Uninit` | SWS_Fee_00082 |
| TC_FEE_API_011 | Fee_Write with invalid block number | `test_Write_InvalidBlockNumber` | SWS_Fee_00083 |
| TC_FEE_API_012 | Fee_Write with NULL pointer | `test_Write_NullPointer` | SWS_Fee_00084 |
| TC_FEE_API_013 | Fee_Write acceptance when idle | `test_Write_AcceptWhenIdle` | SWS_Fee_00085 |
| TC_FEE_API_014 | Fee_Write rejection when busy | `test_Write_RejectionWhenBusy` | SWS_Fee_00086 |
| TC_FEE_API_015 | Fee_Cancel before init | `test_Cancel_Uninit` | SWS_Fee_00087 |
| TC_FEE_API_016 | Fee_Cancel after init | `test_Cancel_AfterInit` | SWS_Fee_00088 |
| TC_FEE_API_017 | Fee_GetStatus returns UNINIT | `test_GetStatus_Uninit` | SWS_Fee_00089 |
| TC_FEE_API_018 | Fee_GetStatus returns correct status | `test_GetStatus_CorrectStatus` | SWS_Fee_00090 |
| TC_FEE_API_019 | Fee_GetJobResult before init | `test_GetJobResult_Uninit` | SWS_Fee_00091 |
| TC_FEE_API_020 | Fee_GetJobResult correct result | `test_GetJobResult_CorrectResult` | SWS_Fee_00092 |
| TC_FEE_API_021 | Fee_InvalidateBlock before init | `test_InvalidateBlock_Uninit` | SWS_Fee_00093 |
| TC_FEE_API_022 | Fee_InvalidateBlock invalid block | `test_InvalidateBlock_InvalidBlockNo` | SWS_Fee_00094 |
| TC_FEE_API_023 | Fee_InvalidateBlock when idle | `test_InvalidateBlock_AcceptWhenIdle` | SWS_Fee_00095 |
| TC_FEE_API_024 | Fee_EraseImmediateBlock before init | `test_EraseImmediateBlock_Uninit` | SWS_Fee_00096 |
| TC_FEE_API_025 | Fee_EraseImmediateBlock invalid block | `test_EraseImmediateBlock_InvalidBlockNo` | SWS_Fee_00097 |
| TC_FEE_API_026 | Fee_EraseImmediateBlock non-immediate | `test_EraseImmediateBlock_NotImmediate` | SWS_Fee_00098 |
| TC_FEE_API_027 | Fee_EraseImmediateBlock when idle | `test_EraseImmediateBlock_AcceptWhenIdle` | SWS_Fee_00099 |
| TC_FEE_API_028 | Fee_GetVersionInfo NULL pointer | `test_GetVersionInfo_NullPointer` | SWS_Fee_00100 |
| TC_FEE_API_029 | Fee_GetVersionInfo correct values | `test_GetVersionInfo_CorrectValues` | SWS_Fee_00101 |
| TC_FEE_API_030 | Fee_SetMode no-op in polling mode | `test_SetMode_NoOp` | SWS_Fee_00102 |
| TC_FEE_API_031 | Fee_MainFunction before init | `test_MainFunction_Uninit` | SWS_Fee_00103 |
| TC_FEE_API_032 | Immediate write during GC accepted | `test_ImmediateWrite_DuringGC` | SWS_Fee_00104 |
| TC_FEE_API_033 | Normal write during GC rejected | `test_NormalWrite_DuringGC` | SWS_Fee_00105 |
| TC_FEE_API_034 | Init drives to IDLE | `test_Init_DrivesToIdle` | SWS_Fee_00106 |
| TC_FEE_API_035 | Read with valid offset partial | `test_Read_ValidPartialRead` | SWS_Fee_00107 |

### 2.2 State Machine Tests (`test/unit/test_Fee_StateMachine.c`)

| Test Case ID     | Description | Function | SWS Ref |
|------------------|-------------|----------|---------|
| TC_FEE_SM_001 | Init state transition to READ_SECTOR_HEADER | `test_Init_StateTransition_ToReadSectorHeader` | SWS_Fee_00110 |
| TC_FEE_SM_002 | Init scan blank flash goes to IDLE | `test_Init_BlankFlash_GoesToIdle` | SWS_Fee_00111 |
| TC_FEE_SM_003-043 | Full state machine tests including read, write, invalidate, cancel, error handling | See `test_Fee_StateMachine.c` | SWS_Fee_00110-00150 |

### 2.3 Garbage Collection Tests (`test/unit/test_Fee_GarbageCollect.c`)

| Test Case ID     | Description | Function | SWS Ref |
|------------------|-------------|----------|---------|
| TC_FEE_GC_001 | GC init not active | `test_GC_Init_IsNotActive` | SWS_Fee_00160 |
| TC_FEE_GC_002 | GC trigger selects fullest source | `test_GC_Trigger_SelectsFullestSource` | SWS_Fee_00161 |
| TC_FEE_GC_003-036 | Full GC tests including single/multi-block copy, suspend/resume, error handling, wear leveling | See `test_Fee_GarbageCollect.c` | SWS_Fee_00160-00195 |

### 2.4 CRC Tests (`test/unit/test_Fee_Crc.c`)

| Test Case ID     | Description | Function | SWS Ref |
|------------------|-------------|----------|---------|
| TC_FEE_CRC_001 | Known test vector | `test_CRC_KnownTestVector` | SWS_Fee_00200 |
| TC_FEE_CRC_002-013 | CRC edge cases, incremental, block header CRC | See `test_Fee_Crc.c` | SWS_Fee_00200-00212 |

### 2.5 Sector Management Tests (`test/unit/test_Fee_Sector.c`)

| Test Case ID     | Description | Function | SWS Ref |
|------------------|-------------|----------|---------|
| TC_FEE_SEC_001 | Sector header round-trip | `test_SectorHeader_BuildParse_RoundTrip` | SWS_Fee_00220 |
| TC_FEE_SEC_002-038 | Block header, allocation, write buffer tests | See `test_Fee_Sector.c` | SWS_Fee_00220-00257 |

### 2.6 Safety Tests (`test/unit/test_Fee_Safety.c`)

| Test Case ID     | Description | Function | SWS Ref |
|------------------|-------------|----------|---------|
| TC_FEE_SAF_001-012 | RAM CRC integrity, flow counter checks | See `test_Fee_Safety.c` | SWS_Fee_00300-00311 |

### 2.7 Power Loss Tests (`test/unit/test_Fee_PowerLoss.c`)

| Test Case ID     | Description | Function | SWS Ref |
|------------------|-------------|----------|---------|
| TC_FEE_PL_001 | Blank flash re-init | `test_PowerLoss_BlankFlash_ReInit` | SWS_Fee_00400 |
| TC_FEE_PL_002 | After header write before data | `test_PowerLoss_AfterHeaderWrite_BeforeData` | SWS_Fee_00401 |
| TC_FEE_PL_003 | After data write before valid marker | `test_PowerLoss_AfterDataWrite_BeforeValidMarker` | SWS_Fee_00402 |
| TC_FEE_PL_004 | Committed block survives | `test_PowerLoss_CommittedBlock_Survives` | SWS_Fee_00403 |
| TC_FEE_PL_005-016 | Full power loss scenarios | See `test_Fee_PowerLoss.c` | SWS_Fee_00400-00415 |

## 3. Preconditions

- All tests run on host (x86/x64) with GCC and Unity test framework
- `FEE_UNIT_TEST` preprocessor macro defined
- Mem_DFLS_Stub provides RAM-backed flash simulation
- TC3xx flash behavior: erased state = 0x00, writes can only set bits (0→1)

## 4. Expected Results

All tests must pass with 0 failures and 0 ignored.

## 5. Coverage Objectives

See `Fee_CoverageObjectives.md` for detailed coverage targets.
