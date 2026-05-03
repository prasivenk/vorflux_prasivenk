# MemAcc Module — Architecture Document

**AUTOSAR R24-11 | SWS Memory Access**
**Module ID:** 166 | **SW Version:** 1.0.0 | **AR Version:** 24.11.0

---

## 1  Module Overview

The MemAcc (Memory Access) module provides a unified, address-area-based
abstraction layer for accessing non-volatile memory. It decouples upper-layer
modules (Fee, NvM) from specific Mem driver implementations (Mem_DFLS for TC3xx
DFLASH). MemAcc manages address areas, validates addresses, translates logical
addresses to physical addresses, and dispatches async jobs to the underlying
Mem_DFLS driver.

Key responsibilities:
- Address area management with bounds validation
- Logical-to-physical address translation
- Asynchronous job dispatching to Mem_DFLS driver
- ECC error propagation (corrected and uncorrected)
- Compare operations with chunked internal buffer processing
- Per-area busy/lock management
- DEM reporting for hardware errors

---

## 2  Layered Architecture

```
┌─────────────────────────────────┐
│     Fee (Flash EEPROM Emu)      │  Upper-layer consumer
├─────────────────────────────────┤
│     MemAcc (Memory Access)      │  This module — ASIL-B partition
├─────────────────────────────────┤
│     Mem_DFLS (TC3xx Driver)     │  MCAL flash driver
├─────────────────────────────────┤
│       TC3xx DFLASH Hardware     │  Physical data flash
└─────────────────────────────────┘
```

**Dependency mapping:**

| Consumed module | Usage |
|---|---|
| Mem_DFLS | Flash operations: `Mem_DFLS_Read`, `Mem_DFLS_Write`, `Mem_DFLS_Erase`, `Mem_DFLS_BlankCheck`, `Mem_DFLS_GetJobResult`, `Mem_DFLS_MainFunction` |
| Det | Development error reporting when `MEMACC_DEV_ERROR_DETECT == STD_ON` |
| Dem | Production error reporting (`Dem_SetEventStatus`) for `MEMACC_E_HARDWARE_ERROR` |
| SchM | Exclusive areas (`SchM_Enter_MemAcc_MEMACC_EXCLUSIVE_AREA_0`, `SchM_Exit_MemAcc_MEMACC_EXCLUSIVE_AREA_0`) |

---

## 3  Interface Definitions

### 3.1  MemAcc Public APIs (16 APIs)

| # | API | SID | Brief Description |
|---|-----|-----|-------------------|
| 1 | `MemAcc_Init(ConfigPtr)` | 0x00 | Initialize module; resets per-area state arrays |
| 2 | `MemAcc_DeInit()` | 0x01 | De-initialize module; clears all jobs and resets to `MEMACC_UNINIT` |
| 3 | `MemAcc_Read(AreaId, Address, DataPtr, Length)` | 0x02 | Initiate async read via `Mem_DFLS_Read` |
| 4 | `MemAcc_Write(AreaId, Address, DataPtr, Length)` | 0x03 | Initiate async write via `Mem_DFLS_Write` |
| 5 | `MemAcc_Erase(AreaId, Address, Length)` | 0x04 | Initiate async erase via `Mem_DFLS_Erase` |
| 6 | `MemAcc_BlankCheck(AreaId, Address, Length)` | 0x05 | Initiate async blank check via `Mem_DFLS_BlankCheck` |
| 7 | `MemAcc_Compare(AreaId, Address, DataPtr, Length)` | 0x06 | Initiate chunked compare (processed in `MemAcc_MainFunction`) |
| 8 | `MemAcc_GetProcessedLength(AreaId)` | 0x07 | Return bytes processed in current/last job |
| 9 | `MemAcc_HwSpecificServiceRequest(AreaId, DataPtr, Length)` | 0x08 | HW-specific service — returns `E_NOT_OK` (`<PLACEHOLDER_REQUIRED>` for TC3xx) |
| 10 | `MemAcc_Cancel(AreaId)` | 0x09 | Cancel ongoing job for area; sets `MEMACC_JOB_CANCELED` |
| 11 | `MemAcc_GetJobResult(AreaId)` | 0x0A | Return job result for area |
| 12 | `MemAcc_GetSegmentationInfo(AreaId, SectorSizePtr, PageSizePtr)` | 0x0B | Return sector size and page size for area |
| 13 | `MemAcc_RequestLock(AreaId)` | 0x0C | Acquire exclusive lock on area (prevents new jobs) |
| 14 | `MemAcc_ReleaseLock(AreaId)` | 0x0D | Release exclusive lock on area |
| 15 | `MemAcc_GetVersionInfo(VersionInfoPtr)` | 0x0E | Return version information structure |
| 16 | `MemAcc_MainFunction()` | 0x0F | Cyclic processing: polls `Mem_DFLS_GetJobResult`, processes compare jobs |

### 3.2  Internal Functions

| Function | Description |
|----------|-------------|
| `MemAcc_Internal_ValidateAddress(AreaId, Address, Length)` | Bounds check for address+length within area |
| `MemAcc_Internal_TranslateAddress(AreaId, LogicalAddress)` | Logical → physical address translation (`BaseAddress + LogicalAddress`) |
| `MemAcc_Internal_FindArea(AreaId)` | Validate area ID against `NumberOfAreas` |
| `MemAcc_Internal_DispatchToMemDriver(AreaId)` | Dispatch job to appropriate `Mem_DFLS_*` API |

---

## 4  Initialization Sequence

`MemAcc_Init(ConfigPtr)` is a **synchronous, single-call** initialization:

1. Validate `ConfigPtr` (DET check if enabled).
2. Enter exclusive area `MEMACC_EXCLUSIVE_AREA_0`.
3. Store `MemAcc_ConfigPtr = ConfigPtr`.
4. For each area index `0..MEMACC_NUMBER_OF_ADDRESS_AREAS-1`:
   - Reset `MemAcc_CurrentJob[areaIdx]` to `MEMACC_JOB_NONE`.
   - Reset `MemAcc_AreaJobResult[areaIdx]` to `MEMACC_JOB_OK`.
   - Clear `MemAcc_AreaBusy[areaIdx]` and `MemAcc_AreaLocked[areaIdx]`.
5. Set `MemAcc_ModuleStatus = MEMACC_IDLE`.
6. Exit exclusive area.

---

## 5  Runtime Sequence

### 5.1  Job Acceptance (e.g., `MemAcc_Read`)

```
MemAcc_Read(AreaId, Address, DataPtr, Length)
  → DET validation (uninit, NULL ptr, invalid area, invalid address, zero length)
  → SchM_Enter_MemAcc_MEMACC_EXCLUSIVE_AREA_0()
  → Check MemAcc_AreaBusy[AreaId]  → reject if TRUE
  → Check MemAcc_AreaLocked[AreaId] → reject if TRUE
  → Fill MemAcc_CurrentJob[AreaId]
  → Set MemAcc_AreaJobResult[AreaId] = MEMACC_JOB_PENDING
  → Set MemAcc_AreaBusy[AreaId] = TRUE
  → SchM_Exit_MemAcc_MEMACC_EXCLUSIVE_AREA_0()
  → MemAcc_Internal_DispatchToMemDriver(AreaId)
      → Translates address, calls Mem_DFLS_Read(physAddr, DataPtr, Length)
  → If dispatch fails: reset busy flag, set MEMACC_JOB_FAILED
```

### 5.2  MainFunction Processing

Each `MemAcc_MainFunction()` cycle, for each busy area:

1. **Compare jobs** — processed via `MemAcc_Internal_ProcessCompare` (chunked reads into `MemAcc_CompareBuffer[MEMACC_COMPARE_BUFFER_SIZE]`, byte-by-byte comparison).
2. **All other jobs** — calls `Mem_DFLS_MainFunction()`, then polls `Mem_DFLS_GetJobResult()`:
   - `MEM_DFLS_JOB_PENDING` → continue
   - `MEM_DFLS_JOB_OK` → set `MEMACC_JOB_OK`, clear busy
   - `MEM_DFLS_JOB_FAILED` → DEM report, set `MEMACC_JOB_FAILED`
   - `MEM_DFLS_JOB_ECC_CORRECTED` → set `MEMACC_JOB_ECC_CORRECTED`
   - `MEM_DFLS_JOB_ECC_UNCORRECTED` → DEM report, set `MEMACC_JOB_ECC_UNCORRECTED`

---

## 6  Shutdown Sequence

1. `MemAcc_Cancel(AreaId)` — Cancels area job: clears busy flag, sets `MEMACC_JOB_CANCELED`.
2. `MemAcc_DeInit()` — Resets all per-area state, sets `MEMACC_UNINIT`.

---

## 7  Multi-Core Access Model

MemAcc uses exclusive areas for all shared-state modifications:

| Mechanism | Scope |
|-----------|-------|
| `SchM_Enter_MemAcc_MEMACC_EXCLUSIVE_AREA_0` / `SchM_Exit_MemAcc_MEMACC_EXCLUSIVE_AREA_0` | Protects `MemAcc_AreaBusy[]`, `MemAcc_AreaJobResult[]`, `MemAcc_CurrentJob[]`, `MemAcc_AreaLocked[]` |
| `volatile` qualifiers | All per-area state arrays |

All APIs use enter/exit exclusive area around shared state access.

---

## 8  Safety Partitioning

| Attribute | Value |
|-----------|-------|
| ASIL Rating | ASIL-B |
| Memory Partition | Same ASIL-B partition as Fee |
| Stack Usage | Static buffers only (`MemAcc_CompareBuffer[32]`) |
| ECC Propagation | `MEM_DFLS_JOB_ECC_CORRECTED` → `MEMACC_JOB_ECC_CORRECTED`, `MEM_DFLS_JOB_ECC_UNCORRECTED` → `MEMACC_JOB_ECC_UNCORRECTED` + DEM |

---

## 9  Address Area Configuration

| Area | ID | Start Address | Length | Sector Size | Page Size | Purpose |
|------|----|---------------|--------|-------------|-----------|---------|
| 0 | 0 | `0xAF000000` | 64 KB | 4096 bytes | 32 bytes | Fee DFLASH partition |
| 1 | 1 | `0xAF010000` | 16 KB | 4096 bytes | 32 bytes | Bootloader DFLASH partition |

`<PLACEHOLDER_REQUIRED>` Actual TC3xx DFLASH base addresses depend on MCU
variant. Addresses shown are for TC39x DF0.

---

## 10  Source File Mapping

| Source File | Responsibility |
|---|---|
| `src/MemAcc.c` | Public API implementations, per-area state management |
| `src/MemAcc_JobProcessing.c` | `MemAcc_MainFunction`, `MemAcc_Internal_DispatchToMemDriver`, compare processing |
| `src/MemAcc_AddressArea.c` | Address validation, translation, area lookup |
| `src/MemAcc_PBcfg.c` | Post-build configuration data |
| `src/MemAcc_Cfg.c` | Compile-time configuration instantiation |
