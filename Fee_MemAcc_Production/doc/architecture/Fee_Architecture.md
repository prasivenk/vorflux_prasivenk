# Fee Module — Architecture Document

**AUTOSAR R24-11 | SWS Flash EEPROM Emulation**
**Module ID:** 21 | **SW Version:** 2.0.0 | **AR Version:** 24.11.0

---

## 1  Module Overview

The Fee (Flash EEPROM Emulation) module provides a virtual EEPROM abstraction
over data flash (DFLASH) for AUTOSAR BSW stacks. It translates block-oriented
NvM read/write requests into physical flash operations routed through MemAcc and
Mem_DFLS. The implementation targets Infineon TC3xx DFLASH with 0x00 erased
state and 32-byte page granularity.

Key responsibilities:
- Block-level persistent storage with CRC-16 CCITT data integrity
- Non-blocking, multi-cycle initialization via sector scanning
- Two-phase commit write protocol (header → data → valid marker)
- Interruptible garbage collection with immediate-job preemption
- RAM integrity monitoring via cyclic CRC check
- Flow control monitoring via sequence counters

---

## 2  Layered Architecture

```
┌─────────────────────────────────┐
│           NvM (SWC)             │  Application layer
├─────────────────────────────────┤
│         MemIf (Abstraction)     │  Memory interface abstraction
├─────────────────────────────────┤
│     Fee (Flash EEPROM Emu)      │  This module — ASIL-B partition
├─────────────────────────────────┤
│     MemAcc (Memory Access)      │  Address area management
├─────────────────────────────────┤
│     Mem_DFLS (TC3xx Driver)     │  MCAL flash driver
├─────────────────────────────────┤
│       TC3xx DFLASH Hardware     │  Physical data flash
└─────────────────────────────────┘
```

**Dependency mapping:**

| Consumed module | Usage |
|---|---|
| MemAcc | All async flash I/O: `MemAcc_Read`, `MemAcc_Write`, `MemAcc_Erase`, `MemAcc_GetJobResult`, `MemAcc_Cancel` |
| Det | Development error reporting (`Det_ReportError`, `Det_ReportRuntimeError`) when `FEE_DEV_ERROR_DETECT == STD_ON` |
| Dem | Production error reporting (`Dem_SetEventStatus`) for `FEE_E_HARDWARE_ERROR` |
| SchM | Exclusive areas (`SchM_Enter_Fee_FEE_EXCLUSIVE_AREA_0`, `SchM_Exit_Fee_FEE_EXCLUSIVE_AREA_0`) |
| NvM (callback) | `NvM_JobEndNotification`, `NvM_JobErrorNotification` |

---

## 3  Interface Definitions

### 3.1  Fee Public APIs (11 APIs)

| # | API | SID | Brief Description |
|---|-----|-----|-------------------|
| 1 | `Fee_Init(ConfigPtr)` | 0x00 | Initialize module; starts async sector-scan state machine |
| 2 | `Fee_SetMode(Mode)` | 0x01 | Set operating mode (no-op in polling mode) |
| 3 | `Fee_Read(BlockNumber, BlockOffset, DataBufferPtr, Length)` | 0x02 | Initiate async block read with CRC verification |
| 4 | `Fee_Write(BlockNumber, DataBufferPtr)` | 0x03 | Initiate async block write (two-phase commit) |
| 5 | `Fee_Cancel()` | 0x04 | Cancel current job; calls `MemAcc_Cancel` in wait states |
| 6 | `Fee_GetStatus()` | 0x05 | Return current module status (`MEMIF_UNINIT`, `MEMIF_IDLE`, `MEMIF_BUSY`, `MEMIF_BUSY_INTERNAL`) |
| 7 | `Fee_GetJobResult()` | 0x06 | Return result of last completed job |
| 8 | `Fee_InvalidateBlock(BlockNumber)` | 0x07 | Mark block as invalid by writing `FEE_MARKER_INVALID` (0xFF) to header |
| 9 | `Fee_GetVersionInfo(VersionInfoPtr)` | 0x08 | Return version information structure |
| 10 | `Fee_EraseImmediateBlock(BlockNumber)` | 0x09 | Erase immediate-data block reservation |
| 11 | `Fee_MainFunction()` | 0x12 | Cyclic processing: drives state machine, runs safety cyclic check |

### 3.2  MemAcc APIs Consumed by Fee (subset of 16 total)

| API | Description |
|-----|-------------|
| `MemAcc_Read(AreaId, Address, DataPtr, Length)` | Async read |
| `MemAcc_Write(AreaId, Address, DataPtr, Length)` | Async write |
| `MemAcc_Erase(AreaId, Address, Length)` | Async erase |
| `MemAcc_GetJobResult(AreaId)` | Poll job result |
| `MemAcc_Cancel(AreaId)` | Cancel ongoing job |

---

## 4  Initialization Sequence (Non-Blocking, Multi-Cycle)

The initialization spans multiple `Fee_MainFunction()` cycles:

1. **`FEE_STATE_INIT_READ_SECTOR_HEADER`** — Issues `MemAcc_Read` for sector header at `SectorInfo[cursor].BaseAddress`.
2. **`FEE_STATE_INIT_WAIT_SECTOR_HEADER`** — Polls `MemAcc_GetJobResult`. Parses header via `Fee_Sector_ParseSectorHeader`. Maps status: `0x55`→`FEE_SECTOR_ACTIVE`, `0xFF`→`FEE_SECTOR_FULL`, `0x00`→`FEE_SECTOR_ERASED`.
3. **`FEE_STATE_INIT_NEXT_SECTOR`** — Increments `Fee_InitSectorCursor`. If more sectors remain, loops to step 1. Otherwise, determines `Fee_ActiveSectorIndex` (highest `SequenceNumber` among ACTIVE sectors) and begins record scanning.
4. **`FEE_STATE_INIT_SCAN_READ_RECORD`** — Reads 32-byte block header at `Fee_ScanRecordCursor`.
5. **`FEE_STATE_INIT_SCAN_WAIT_RECORD`** — Parses block header via `Fee_Sector_ParseBlockHeader`. Updates `Fee_BlockInfoTable[]` if block is known and sequence counter is >= existing. Advances cursor by `FEE_BLOCK_HEADER_SIZE + ALIGN_UP(blockLen, VirtualPageSize)`.
6. **Repeat steps 4–5** until blank area (all 0x00) is reached or sector end.
7. **Transition to `FEE_STATE_IDLE`** — Sets `Fee_ModuleStatus = MEMIF_IDLE`, computes initial RAM CRC.

---

## 5  Runtime Sequence

### 5.1  Job Acceptance

```
Fee_Write(BlockNumber, DataBufferPtr)
  → DET validation (uninit, null pointer, invalid block number)
  → Build Fee_JobInfoType { Type=FEE_JOB_WRITE, ... }
  → SchM_Enter_Fee_FEE_EXCLUSIVE_AREA_0()
  → Fee_StateMachine_AcceptJob(&job)
      if MEMIF_IDLE:     accept, set MEMIF_BUSY
      if MEMIF_BUSY:     reject (E_NOT_OK)
      if BUSY_INTERNAL:  accept only if IsImmediate==TRUE (GC preemption)
  → SchM_Exit_Fee_FEE_EXCLUSIVE_AREA_0()
```

### 5.2  MainFunction Processing

Each `Fee_MainFunction()` call:
1. Calls `Fee_StateMachine_Process()` — advances one state transition.
2. Calls `Fee_Safety_CyclicCheck()` — verifies RAM CRC integrity.

### 5.3  NvM Notification

On job completion (`Fee_StateMachine_CompleteJob`):
- `MEMIF_JOB_OK` → calls `NvM_JobEndNotification()` (when `FEE_NVM_JOB_END_NOTIFICATION == STD_ON`)
- Any failure → calls `NvM_JobErrorNotification()` (when `FEE_NVM_JOB_ERROR_NOTIFICATION == STD_ON`)

---

## 6  Shutdown Sequence

1. `Fee_Cancel()` — Cancels current job; in wait states issues `MemAcc_Cancel(AreaId)`. Job result set to `MEMIF_JOB_CANCELED`.
2. If GC was suspended for an immediate job and the immediate job completes, GC resumes (`Fee_InternalState = FEE_STATE_GC_ACTIVE`).
3. No explicit `Fee_DeInit` API — module returns to IDLE after cancel.

---

## 7  Multi-Core Access Model

Fee operates in **single-core** mode. Concurrent access is protected by:

| Mechanism | Scope |
|-----------|-------|
| `SchM_Enter_Fee_FEE_EXCLUSIVE_AREA_0` / `SchM_Exit_Fee_FEE_EXCLUSIVE_AREA_0` | Protects `Fee_StateMachine_AcceptJob` (job acceptance critical section) |
| `volatile` qualifiers | All module-level state variables (`Fee_InternalState`, `Fee_ModuleStatus`, `Fee_LastJobResult`, `Fee_BlockInfoTable[]`, `Fee_SectorInfo[]`, etc.) |

In unit-test builds (`FEE_UNIT_TEST` defined), exclusive areas expand to empty
macros. In production, they are generated by the RTE/SchM generator and
typically map to interrupt disable/enable or spinlock acquire/release.

---

## 8  Safety Partitioning

| Attribute | Value |
|-----------|-------|
| ASIL Rating | ASIL-B |
| Memory Partition | Fee and MemAcc reside in the same ASIL-B partition |
| Stack Usage | All buffers are statically allocated (no dynamic memory) |
| Safety Mechanisms | CRC-16 data integrity, RAM CRC cyclic check, flow counter monitoring, DET/DEM error reporting |
| Safe State | `FEE_STATE_ERROR` → DEM event reported, then transition to `FEE_STATE_IDLE` |

---

## 9  Source File Mapping

| Source File | Responsibility |
|---|---|
| `src/Fee.c` | Public API wrappers, DET validation, SchM exclusive areas |
| `src/Fee_StateMachine.c` | Core async state machine (init, read, write, invalidate, erase-immediate, GC, error) |
| `src/Fee_Sector.c` | Sector/block header parse/build, block allocation, write buffer management |
| `src/Fee_GarbageCollect.c` | Interruptible multi-cycle GC with suspend/resume |
| `src/Fee_Crc.c` | CRC-16 CCITT calculation (polynomial 0x1021) |
| `src/Fee_Safety.c` | RAM CRC integrity check, flow counter check |
| `src/Fee_PBcfg.c` | Post-build configuration data |
| `src/Fee_Cfg.c` | Compile-time configuration instantiation |
