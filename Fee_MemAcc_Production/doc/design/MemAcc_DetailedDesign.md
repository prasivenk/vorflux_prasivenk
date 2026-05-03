# MemAcc Module — Detailed Design Document

**AUTOSAR R24-11 | SWS Memory Access**
**Module ID:** 166 | **SW Version:** 1.0.0

---

## 1  Internal Submodules

| Source File | Responsibility |
|---|---|
| `src/MemAcc.c` | Public API implementations (16 APIs). DET validation, per-area job/busy/lock management, SchM exclusive areas. Global state: `MemAcc_ModuleStatus`, `MemAcc_CurrentJob[]`, `MemAcc_AreaJobResult[]`, `MemAcc_AreaBusy[]`, `MemAcc_AreaLocked[]`, `MemAcc_ConfigPtr`. |
| `src/MemAcc_JobProcessing.c` | `MemAcc_MainFunction`, `MemAcc_Internal_DispatchToMemDriver`, `MemAcc_Internal_ProcessCompare`. Static buffer: `MemAcc_CompareBuffer[MEMACC_COMPARE_BUFFER_SIZE]`. ECC propagation and DEM reporting. |
| `src/MemAcc_AddressArea.c` | Address management: `MemAcc_Internal_ValidateAddress`, `MemAcc_Internal_TranslateAddress`, `MemAcc_Internal_FindArea`. |
| `src/MemAcc_PBcfg.c` | Post-build configuration data (address area array). |
| `src/MemAcc_Cfg.c` | Compile-time configuration instantiation. |

---

## 2  State Machine Description

### 2.1  Module Status (`MemAcc_StatusType`)

| Value | State | Description |
|-------|-------|-------------|
| 0 | `MEMACC_UNINIT` | Module not initialized; all APIs rejected with DET |
| 1 | `MEMACC_IDLE` | Module initialized, ready to accept jobs |
| 2 | `MEMACC_BUSY` | At least one area has an active job |

### 2.2  Per-Area Job Result (`MemAcc_JobResultType`)

| Value | Result | Description |
|-------|--------|-------------|
| 0 | `MEMACC_JOB_OK` | Job completed successfully |
| 1 | `MEMACC_JOB_PENDING` | Job in progress |
| 2 | `MEMACC_JOB_FAILED` | Job failed (DEM reported for driver failures) |
| 3 | `MEMACC_JOB_CANCELED` | Job canceled by `MemAcc_Cancel` |
| 4 | `MEMACC_JOB_ECC_CORRECTED` | Correctable ECC error detected |
| 5 | `MEMACC_JOB_ECC_UNCORRECTED` | Uncorrectable ECC error (DEM reported) |

### 2.3  Job Types (`MemAcc_JobType`)

| Value | Type |
|-------|------|
| 0 | `MEMACC_JOB_NONE` |
| 1 | `MEMACC_JOB_READ` |
| 2 | `MEMACC_JOB_WRITE` |
| 3 | `MEMACC_JOB_ERASE` |
| 4 | `MEMACC_JOB_BLANKCHECK` |
| 5 | `MEMACC_JOB_COMPARE` |

---

## 3  Buffer Management

| Buffer | Location | Size | Usage |
|--------|----------|------|-------|
| `MemAcc_CompareBuffer[MEMACC_COMPARE_BUFFER_SIZE]` | `src/MemAcc_JobProcessing.c` (static) | 32 bytes | Chunked read buffer for compare operations |

Compare operations read flash data in chunks of `MEMACC_COMPARE_BUFFER_SIZE`
(32 bytes, equal to page size) into `MemAcc_CompareBuffer`, then compare
byte-by-byte against `CompareDataPtr`.

---

## 4  Job Processing (`MemAcc_MainFunction`)

For each area `0..MEMACC_NUMBER_OF_ADDRESS_AREAS-1`:

1. Skip if `MemAcc_AreaBusy[areaIdx] == FALSE`.
2. **Compare jobs**: Process via `MemAcc_Internal_ProcessCompare` (chunked).
3. **All other jobs**: Call `Mem_DFLS_MainFunction()`, then poll `Mem_DFLS_GetJobResult()`:

| Driver Result | MemAcc Action |
|---|---|
| `MEM_DFLS_JOB_PENDING` | Continue (stay busy) |
| `MEM_DFLS_JOB_OK` | Set `MEMACC_JOB_OK`, `ProcessedLength = Length`, clear busy |
| `MEM_DFLS_JOB_FAILED` | DEM report `MEMACC_E_HARDWARE_ERROR`, set `MEMACC_JOB_FAILED`, clear busy |
| `MEM_DFLS_JOB_ECC_CORRECTED` | Set `MEMACC_JOB_ECC_CORRECTED`, `ProcessedLength = Length`, clear busy |
| `MEM_DFLS_JOB_ECC_UNCORRECTED` | DEM report, set `MEMACC_JOB_ECC_UNCORRECTED`, clear busy |

---

## 5  Job Dispatch (`MemAcc_Internal_DispatchToMemDriver`)

Translates logical address to physical via `MemAcc_Internal_TranslateAddress`, then calls the appropriate `Mem_DFLS_*` API:

| Job Type | Mem_DFLS API |
|---|---|
| `MEMACC_JOB_READ` | `Mem_DFLS_Read(physAddr, ReadDataPtr, Length)` |
| `MEMACC_JOB_WRITE` | `Mem_DFLS_Write(physAddr, WriteDataPtr, Length)` |
| `MEMACC_JOB_ERASE` | `Mem_DFLS_Erase(physAddr, Length)` |
| `MEMACC_JOB_BLANKCHECK` | `Mem_DFLS_BlankCheck(physAddr, Length)` |
| `MEMACC_JOB_COMPARE` | No dispatch (handled in MainFunction) |

---

## 6  Address Management

### 6.1  `MemAcc_Internal_ValidateAddress(AreaId, Address, Length)`

Returns `TRUE` if:
- `MemAcc_ConfigPtr != NULL_PTR`
- `AreaId < NumberOfAreas`
- `Address + Length` does not overflow
- `Address + Length <= AddressAreas[AreaId].Length`

### 6.2  `MemAcc_Internal_TranslateAddress(AreaId, LogicalAddress)`

Returns: `AddressAreas[AreaId].StartAddress + LogicalAddress`

### 6.3  `MemAcc_Internal_FindArea(AreaId)`

Returns `TRUE` if `AreaId < NumberOfAreas` and `ConfigPtr` is valid.

---

## 7  Error Handling

### 7.1  DET Errors (Development)

| Error Code | Value | Condition |
|------------|-------|-----------|
| `MEMACC_E_UNINIT` | 0x01 | API called before `MemAcc_Init` |
| `MEMACC_E_PARAM_POINTER` | 0x02 | NULL pointer parameter |
| `MEMACC_E_PARAM_ADDRESS_AREA` | 0x03 | Invalid area ID |
| `MEMACC_E_PARAM_ADDRESS` | 0x04 | Address out of bounds |
| `MEMACC_E_PARAM_LENGTH` | 0x05 | Zero-length parameter |
| `MEMACC_E_BUSY` | 0x06 | Area already has active job |

### 7.2  DEM Event (Production)

| Event ID | Value | Condition |
|----------|-------|-----------|
| `MEMACC_E_HARDWARE_ERROR` | 0x0001 | `Mem_DFLS_GetJobResult` returns `MEM_DFLS_JOB_FAILED` or `MEM_DFLS_JOB_ECC_UNCORRECTED` |

---

## 8  Synchronization

| Mechanism | Location | Scope |
|-----------|----------|-------|
| `SchM_Enter_MemAcc_MEMACC_EXCLUSIVE_AREA_0` / `SchM_Exit_MemAcc_MEMACC_EXCLUSIVE_AREA_0` | `src/MemAcc.c` (all APIs), `src/MemAcc_JobProcessing.c` | Protects `MemAcc_AreaBusy[]`, `MemAcc_AreaJobResult[]`, `MemAcc_CurrentJob[]`, `MemAcc_AreaLocked[]` |
| `volatile` qualifier | All per-area arrays and `MemAcc_ModuleStatus` | Ensures visibility across ISR/task boundaries |

---

## 9  Configuration

### 9.1  Address Area Configuration (`MemAcc_AddressAreaConfigType`)

| Field | Type | Description |
|-------|------|-------------|
| `AreaId` | `MemAcc_AddressAreaIdType` (uint8) | Area identifier |
| `StartAddress` | `MemAcc_AddressType` (uint32) | Physical start address |
| `Length` | `MemAcc_LengthType` (uint32) | Area length in bytes |
| `SectorSize` | `MemAcc_LengthType` (uint32) | Sector (erase) size |
| `PageSize` | `MemAcc_LengthType` (uint32) | Page (write) size |

### 9.2  Module Configuration (`MemAcc_ConfigType`)

| Field | Type | Description |
|-------|------|-------------|
| `AddressAreas` | `const MemAcc_AddressAreaConfigType*` | Pointer to area config array |
| `NumberOfAreas` | `uint8` | Number of configured areas |

---

## 10  Memory Mapping

| Section Macro | Content |
|---|---|
| `MEMACC_START_SEC_CODE` / `MEMACC_STOP_SEC_CODE` | All MemAcc function code |
| `MEMACC_START_SEC_VAR_CLEARED_UNSPECIFIED` / `MEMACC_STOP_SEC_VAR_CLEARED_UNSPECIFIED` | Module state variables |
| `MEMACC_START_SEC_VAR_CLEARED_8` / `MEMACC_STOP_SEC_VAR_CLEARED_8` | `MemAcc_CompareBuffer[]` |

Memory map header: `include/MemAcc_MemMap.h`.
