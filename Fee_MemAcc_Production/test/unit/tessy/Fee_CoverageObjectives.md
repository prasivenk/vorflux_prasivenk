# Fee Module Coverage Objectives — AUTOSAR R24-11

## 1. Statement Coverage Target: 100%

All executable statements in the following safety-relevant files must be
exercised by at least one test case.

| Source File | ASIL | Target |
|-------------|------|--------|
| `src/Fee.c` | B | 100% |
| `src/Fee_StateMachine.c` | B | 100% |
| `src/Fee_GarbageCollect.c` | B | 100% |
| `src/Fee_Sector.c` | B | 100% |
| `src/Fee_Crc.c` | B | 100% |
| `src/Fee_Safety.c` | B | 100% |
| `src/Fee_Cfg.c` | QM | 100% |
| `src/Fee_PBcfg.c` | QM | 100% |
| `src/MemAcc.c` | B | 100% |
| `src/MemAcc_JobProcessing.c` | B | 100% |
| `src/MemAcc_AddressArea.c` | QM | 100% |
| `src/MemAcc_Cfg.c` | QM | 100% |
| `src/MemAcc_PBcfg.c` | QM | 100% |

## 2. Branch Coverage Target: 100%

Every decision point (if/else, switch/case, ternary) must have both TRUE
and FALSE outcomes exercised.

### Critical Branch Points

- **Fee_Init**: NULL config check, state transition
- **Fee_Read**: UNINIT check, block number validation, NULL pointer,
  length validation, offset+length validation, busy check
- **Fee_Write**: UNINIT check, block number validation, NULL pointer,
  busy check, immediate data check during GC
- **Fee_StateMachine_Process**: All 29 state cases
- **Fee_GarbageCollect_Process**: All GC states, error paths
- **Fee_Sector_ParseBlockHeader**: CRC validation, blank check
- **Fee_Sector_AllocateBlock**: Full sector check
- **Fee_Safety_CyclicCheck**: CRC mismatch detection

## 3. MC/DC Coverage Target: 100%

Modified Condition/Decision Coverage required for safety-relevant decisions:

- **Fee_Read offset+length check**: `(BlockOffset + Length) > BlockSize`
- **Fee_Write immediate-during-GC check**: `(ModuleStatus == BUSY_INTERNAL) && (ImmediateData == TRUE)`
- **Fee_Sector_ParseSectorHeader magic check**: `(magic == FEE_SECTOR_MAGIC)`
- **Fee_Safety_CyclicCheck CRC comparison**: `(computed != stored)`
- **Fee_GarbageCollect_Trigger source/target selection**: Multiple conditions

## 4. Test Coverage Mapping

| Requirement | Test File | Test Count |
|-------------|-----------|------------|
| DET checks (all APIs) | `test_Fee_Api.c` | 35 |
| State machine transitions | `test_Fee_StateMachine.c` | 43 |
| Garbage collection | `test_Fee_GarbageCollect.c` | 36 |
| CRC calculation | `test_Fee_Crc.c` | 13 |
| Sector management | `test_Fee_Sector.c` | 38 |
| Safety mechanisms | `test_Fee_Safety.c` | 12 |
| Job queue | `test_Fee_JobQueue.c` | 22 |
| Power loss recovery | `test_Fee_PowerLoss.c` | 16 |
| MemAcc API | `test_MemAcc_Api.c` | 38 |
| MemAcc address areas | `test_MemAcc_AddressArea.c` | 14 |
| MemAcc job processing | `test_MemAcc_JobProcessing.c` | 18 |
| Integration (end-to-end) | `test_Fee_MemAcc_Integration.c` | 21 |
| OTA scenarios | `test_Fee_OTA_Scenarios.c` | 10 |
| Bootloader coexistence | `test_Fee_Bootloader_Coexistence.c` | 8 |
| ECC fault injection | `test_Fee_ECC_FaultInjection.c` | 12 |
| Recovery | `test_Fee_Recovery.c` | 10 |
| Multi-core / SchM | `test_Fee_MultiCore.c` | 8 |
| **Total** | | **354** |

## 5. Exclusions

- Code inside `#ifndef FEE_UNIT_TEST` guards (production MemMap pragmas)
- Code inside `#if (FEE_DEV_ERROR_DETECT == STD_OFF)` (DET is enabled)
- Unreachable defensive code (dead code from MISRA compliance)

## 6. Tools

- **Host testing**: Unity test framework + GCC + CMake/CTest
- **Target testing**: TESSY or equivalent for MC/DC on TriCore
- **Coverage measurement**: gcov (host), TESSY (target)
