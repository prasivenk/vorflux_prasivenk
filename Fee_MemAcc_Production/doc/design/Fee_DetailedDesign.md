# Fee Module — Detailed Design Document

**AUTOSAR R24-11 | SWS Flash EEPROM Emulation**
**Module ID:** 21 | **SW Version:** 2.0.0

---

## 1  Internal Submodules

| Source File | Responsibility |
|---|---|
| `src/Fee.c` | Public API wrappers with DET validation. Delegates to `Fee_StateMachine_AcceptJob`. Uses `SchM_Enter_Fee_FEE_EXCLUSIVE_AREA_0`/`SchM_Exit_Fee_FEE_EXCLUSIVE_AREA_0` for job acceptance. Contains `Fee_Internal_FindBlockIndex` helper. |
| `src/Fee_StateMachine.c` | Core async state machine. Handles all 29 states (`FEE_STATE_UNINIT` through `FEE_STATE_ERROR`). Contains `Fee_StateMachine_Init`, `Fee_StateMachine_Process`, `Fee_StateMachine_AcceptJob`, `Fee_StateMachine_CompleteJob`, `Fee_StateMachine_Cancel`, `Fee_StateMachine_GetStatus`, `Fee_StateMachine_GetJobResult`. Internal helpers: `Fee_Internal_LookupBlockIndex`, `Fee_Internal_IsHeaderBlank`. |
| `src/Fee_Sector.c` | Sector and block header parsing/building. Functions: `Fee_Sector_ParseSectorHeader`, `Fee_Sector_BuildSectorHeader`, `Fee_Sector_ParseBlockHeader`, `Fee_Sector_BuildBlockHeader`, `Fee_Sector_AllocateBlock`, `Fee_Sector_PrepareWriteBuffer`, `Fee_Sector_GetFillPercentage`, `Fee_Sector_GetHeaderBuffer`, `Fee_Sector_GetWriteBuffer`. |
| `src/Fee_GarbageCollect.c` | Interruptible multi-cycle GC. 13 states (`FEE_GC_IDLE` through `FEE_GC_COMPLETE_STATE`). Functions: `Fee_GarbageCollect_Init`, `Fee_GarbageCollect_Trigger`, `Fee_GarbageCollect_Process`, `Fee_GarbageCollect_Suspend`, `Fee_GarbageCollect_Resume`, `Fee_GarbageCollect_IsActive`. |
| `src/Fee_Crc.c` | CRC-16 CCITT (polynomial 0x1021, initial 0xFFFF). Functions: `Fee_Crc_Calculate` (with explicit start value), `Fee_Crc_CalculateBlock` (with default 0xFFFF start). |
| `src/Fee_Safety.c` | RAM integrity and flow control. Functions: `Fee_Safety_Init`, `Fee_Safety_CyclicCheck`, `Fee_Safety_UpdateRamCrc`, `Fee_Safety_FlowCheck`, `Fee_Safety_ResetFlowCounter`. Internal: `Fee_Safety_ComputeRamCrc`. |

---

## 2  State Machine Description

### 2.1  Main State Machine (`Fee_InternalStateType`)

The main state machine has 29 enumerated states:

| Value | State Name | Category |
|-------|-----------|----------|
| 0 | `FEE_STATE_UNINIT` | Pre-init |
| 1 | `FEE_STATE_INIT_READ_SECTOR_HEADER` | Init: sector scan |
| 2 | `FEE_STATE_INIT_WAIT_SECTOR_HEADER` | Init: sector scan |
| 3 | `FEE_STATE_INIT_NEXT_SECTOR` | Init: sector scan |
| 4 | `FEE_STATE_INIT_SCAN_READ_RECORD` | Init: record scan |
| 5 | `FEE_STATE_INIT_SCAN_WAIT_RECORD` | Init: record scan |
| 6 | `FEE_STATE_INIT_SCAN_READ_DATA` | Init: reserved |
| 7 | `FEE_STATE_INIT_SCAN_WAIT_DATA` | Init: reserved |
| 8 | `FEE_STATE_INIT_SCAN_NEXT_RECORD` | Init: reserved |
| 9 | `FEE_STATE_IDLE` | Idle |
| 10 | `FEE_STATE_READ_START` | Read |
| 11 | `FEE_STATE_READ_WAIT` | Read |
| 12 | `FEE_STATE_READ_VERIFY_CRC` | Read |
| 13 | `FEE_STATE_WRITE_ALLOC` | Write |
| 14 | `FEE_STATE_WRITE_HEADER` | Write |
| 15 | `FEE_STATE_WRITE_HEADER_WAIT` | Write |
| 16 | `FEE_STATE_WRITE_DATA` | Write |
| 17 | `FEE_STATE_WRITE_DATA_WAIT` | Write |
| 18 | `FEE_STATE_WRITE_VALID` | Write |
| 19 | `FEE_STATE_WRITE_VALID_WAIT` | Write |
| 20 | `FEE_STATE_WRITE_VERIFY_READ` | Write verification |
| 21 | `FEE_STATE_WRITE_VERIFY_WAIT` | Write verification |
| 22 | `FEE_STATE_WRITE_VERIFY_COMPARE` | Write verification |
| 23 | `FEE_STATE_INVALIDATE_WRITE` | Invalidate |
| 24 | `FEE_STATE_INVALIDATE_WAIT` | Invalidate |
| 25 | `FEE_STATE_ERASE_IMMEDIATE` | Erase immediate |
| 26 | `FEE_STATE_ERASE_IMMEDIATE_WAIT` | Erase immediate |
| 27 | `FEE_STATE_GC_ACTIVE` | Garbage collection |
| 28 | `FEE_STATE_ERROR` | Error |

### 2.2  GC State Machine (`Fee_GcInternalStateType`)

| Value | State Name |
|-------|-----------|
| 0 | `FEE_GC_IDLE` |
| 1 | `FEE_GC_SELECT_SOURCE` |
| 2 | `FEE_GC_COPY_READ` |
| 3 | `FEE_GC_COPY_READ_WAIT` |
| 4 | `FEE_GC_COPY_WRITE_HEADER` |
| 5 | `FEE_GC_COPY_WRITE_HEADER_WAIT` |
| 6 | `FEE_GC_COPY_WRITE_DATA` |
| 7 | `FEE_GC_COPY_WRITE_DATA_WAIT` |
| 8 | `FEE_GC_COPY_WRITE_VALID` |
| 9 | `FEE_GC_COPY_WRITE_VALID_WAIT` |
| 10 | `FEE_GC_ERASE_SOURCE` |
| 11 | `FEE_GC_ERASE_WAIT` |
| 12 | `FEE_GC_COMPLETE_STATE` |

---

## 3  Buffer Management

All buffers are **statically allocated** — no dynamic memory.

| Buffer | Location | Size | Usage |
|--------|----------|------|-------|
| `Fee_Sector_HeaderBuffer[FEE_BLOCK_HEADER_SIZE]` | `src/Fee_Sector.c` (static) | 32 bytes | Sector header read/write, block header build/parse |
| `Fee_Sector_WriteBuffer[FEE_MAX_BLOCK_SIZE]` | `src/Fee_Sector.c` (static) | 512 bytes (`FEE_MAX_BLOCK_SIZE`) | Block data write (padded to `FEE_VIRTUAL_PAGE_SIZE`) |
| `Fee_GcCopyBuffer[FEE_MAX_BLOCK_SIZE]` | `src/Fee_GarbageCollect.c` (static) | 512 bytes (`FEE_MAX_BLOCK_SIZE`) | GC block data read for CRC verification and copy |

Access is via getter functions `Fee_Sector_GetHeaderBuffer()` and
`Fee_Sector_GetWriteBuffer()`.

---

## 4  Job Acceptance Model

- **Single job** — Only one user job can be active at a time.
- **Immediate preemption of GC only** — If `Fee_ModuleStatus == MEMIF_BUSY_INTERNAL` (GC in progress) and the incoming job has `IsImmediate == TRUE`, the GC is suspended (`Fee_GcSuspended = TRUE`), the immediate job is accepted, and the state machine jumps to `FEE_STATE_IDLE` for dispatch.
- **Normal jobs during GC** — Rejected with `E_NOT_OK`.
- **Jobs during active user job** — Rejected (`Fee_ModuleStatus == MEMIF_BUSY`).
- **Jobs during init** — Rejected (`Fee_ModuleStatus == MEMIF_BUSY_INTERNAL` and non-immediate).

On job completion (`Fee_StateMachine_CompleteJob`):
- If GC was suspended: resume GC (`Fee_InternalState = FEE_STATE_GC_ACTIVE`, `Fee_ModuleStatus = MEMIF_BUSY_INTERNAL`).
- Otherwise: return to `FEE_STATE_IDLE`, `Fee_ModuleStatus = MEMIF_IDLE`.

---

## 5  Block Management

### 5.1  Block Info Table (`Fee_BlockInfoType`)

Runtime state for each configured block, indexed `0..FEE_NUMBER_OF_BLOCKS-1`:

| Field | Type | Description |
|-------|------|-------------|
| `Status` | `Fee_BlockStatusType` | `FEE_BLOCK_NOT_FOUND`, `FEE_BLOCK_VALID`, `FEE_BLOCK_INVALID`, `FEE_BLOCK_INCONSISTENT` |
| `HeaderAddress` | `MemAcc_AddressType` | Flash address of block header |
| `DataAddress` | `MemAcc_AddressType` | Flash address of block data (= HeaderAddress + 32) |
| `DataLength` | `uint16` | Data length in bytes |
| `DataCrc` | `uint16` | CRC-16 CCITT of block data |
| `SequenceCounter` | `uint16` | Monotonically increasing write counter |
| `SectorIndex` | `uint8` | Index into `Fee_SectorInfo[]` |
| `Immediate` | `boolean` | TRUE for immediate-data blocks |

### 5.2  Block Configuration (`Fee_BlockConfigType`)

Compile-time configuration per block:

| Field | Type | Description |
|-------|------|-------------|
| `BlockNumber` | `uint16` | Block identifier (NvM block number) |
| `BlockSize` | `uint16` | Data size in bytes |
| `ImmediateData` | `boolean` | TRUE for immediate-data blocks |
| `NumberOfWriteCycles` | `uint8` | Max write cycles (0 = unlimited) |

---

## 6  Sector Management

### 6.1  Sector Info (`Fee_SectorInfoType`)

Runtime state for each sector:

| Field | Type | Description |
|-------|------|-------------|
| `Status` | `Fee_SectorStatusType` | `FEE_SECTOR_ERASED`, `FEE_SECTOR_ACTIVE`, `FEE_SECTOR_FULL`, `FEE_SECTOR_DEFECTIVE` |
| `BaseAddress` | `MemAcc_AddressType` | Sector start address (= index × `FEE_SECTOR_SIZE`) |
| `WritePointer` | `MemAcc_AddressType` | Next free write address |
| `SequenceNumber` | `uint32` | Sector sequence number (for active sector selection) |
| `EraseCount` | `uint16` | Erase cycle count (for wear leveling) |
| `FreeSpace` | `uint16` | Remaining free space in bytes |

### 6.2  Sector Header Layout (32 bytes)

Built/parsed by `Fee_Sector_BuildSectorHeader` / `Fee_Sector_ParseSectorHeader`:

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| 0-3 | 4 | Magic | `FEE_SECTOR_MAGIC` (0xFEE0FEE0), little-endian |
| 4-7 | 4 | SequenceNumber | uint32, little-endian |
| 8-9 | 2 | EraseCount | uint16, little-endian |
| 10 | 1 | Status | Sector status byte |
| 11-29 | 19 | Reserved | Padding (0x00) |
| 30-31 | 2 | HeaderCRC | CRC-16 over bytes 0-29 |

### 6.3  Block Header Layout (32 bytes)

Built/parsed by `Fee_Sector_BuildBlockHeader` / `Fee_Sector_ParseBlockHeader`:

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| 0-1 | 2 | BlockNumber | uint16, little-endian |
| 2-3 | 2 | BlockLength | uint16, little-endian |
| 4-5 | 2 | DataCRC | CRC-16 of block data |
| 6-7 | 2 | SequenceCounter | uint16, little-endian |
| 8 | 1 | WriteCount | uint8 |
| 9-29 | 21 | Reserved | Padding (0x00) |
| 30 | 1 | ValidMarker | `0x00`=erased, `0x55`=valid, `0xFF`=invalid |
| 31 | 1 | HeaderCRC-low | CRC-16 over bytes 0-29 (low byte) |

### 6.4  Block Allocation

`Fee_Sector_AllocateBlock(SectorInfo, BlockSize, VirtualPageSize)`:
1. Total size = `FEE_BLOCK_HEADER_SIZE` + `ALIGN_UP(BlockSize, VirtualPageSize)`.
2. If `FreeSpace < TotalSize`, return 0 (sector full).
3. Allocated address = `WritePointer`.
4. `WritePointer += TotalSize`, `FreeSpace -= TotalSize`.

---

## 7  Error Handling

### 7.1  DET Errors (Development)

| Error Code | Value | Condition | APIs |
|------------|-------|-----------|------|
| `FEE_E_UNINIT` | 0x01 | API called before `Fee_Init` | All except `Fee_Init` |
| `FEE_E_INVALID_BLOCK_NO` | 0x02 | Block number not in config | `Fee_Read`, `Fee_Write`, `Fee_InvalidateBlock`, `Fee_EraseImmediateBlock` |
| `FEE_E_INVALID_BLOCK_OFS` | 0x03 | Offset+Length > BlockSize | `Fee_Read` |
| `FEE_E_PARAM_POINTER` | 0x04 | NULL pointer parameter | `Fee_Init`, `Fee_Read`, `Fee_Write`, `Fee_GetVersionInfo` |
| `FEE_E_INVALID_BLOCK_LEN` | 0x05 | Zero-length read | `Fee_Read` |
| `FEE_E_BUSY` | 0x06 | Module busy, job rejected | `Fee_Read`, `Fee_Write`, `Fee_InvalidateBlock`, `Fee_EraseImmediateBlock` |
| `FEE_E_INVALID_CANCEL` | 0x07 | Invalid cancel request | (reserved) |

### 7.2  Runtime Error

| Error Code | Value | Condition |
|------------|-------|-----------|
| `FEE_E_RAM_INTEGRITY` | 0x10 | RAM CRC mismatch in `Fee_Safety_CyclicCheck` or flow counter mismatch |

### 7.3  DEM Event (Production)

| Event ID | Value | Condition |
|----------|-------|-----------|
| `FEE_E_HARDWARE_ERROR` | 0x0001 | MemAcc job failure, CRC mismatch on read, RAM integrity violation |

---

## 8  Recovery Logic

### 8.1  Incomplete Write Detection

During initialization scan (`FEE_STATE_INIT_SCAN_WAIT_RECORD`):
- Block header with `ValidMarker == FEE_MARKER_ERASED` (0x00) → `FEE_BLOCK_INCONSISTENT` (write started but not committed).
- Block header with valid CRC but corrupt data CRC → block data stored but may be corrupt; detected on read via `FEE_STATE_READ_VERIFY_CRC`.

### 8.2  GC Crash Recovery

If power loss occurs during GC:
- **Before valid marker write** — New block copy in target sector has `ValidMarker == 0x00`, detected as `FEE_BLOCK_INCONSISTENT` during next init. Original block in source sector still valid.
- **After valid marker write** — Block successfully copied. Source sector may still contain old copy, which is harmless (lower sequence counter ignored during scan).
- **During erase** — Source sector may be partially erased. Init scan will find corrupt headers → sector treated as `FEE_SECTOR_ERASED`.

---

## 9  Synchronization

| Mechanism | Location | Scope |
|-----------|----------|-------|
| `SchM_Enter_Fee_FEE_EXCLUSIVE_AREA_0` / `SchM_Exit_Fee_FEE_EXCLUSIVE_AREA_0` | `src/Fee.c` (all API functions that call `AcceptJob`) | Protects job acceptance (reads/writes `Fee_ModuleStatus`, `Fee_CurrentJob`, `Fee_InternalState`) |
| `volatile` qualifier | All module-level variables in `src/Fee_StateMachine.c` | Ensures compiler does not cache state in registers across function calls |

In unit-test builds (`FEE_UNIT_TEST` defined), exclusive areas are empty macros.

---

## 10  Memory Mapping

| Section Macro | Content |
|---|---|
| `FEE_START_SEC_CODE` / `FEE_STOP_SEC_CODE` | All Fee function code |
| `FEE_START_SEC_VAR_CLEARED_UNSPECIFIED` / `FEE_STOP_SEC_VAR_CLEARED_UNSPECIFIED` | Module state variables (cleared at startup) |
| `FEE_START_SEC_CONST` / `FEE_STOP_SEC_CONST` | Configuration constants |

Memory map headers: `include/Fee_MemMap.h` (empty in host builds, mapped to linker sections in production).
