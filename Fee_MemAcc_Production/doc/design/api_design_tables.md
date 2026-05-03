# API Design Tables

**AUTOSAR R24-11 Fee + MemAcc**

---

## 1  Fee APIs

### Fee_Init

| Attribute | Value |
|---|---|
| **Signature** | `void Fee_Init(const Fee_ConfigType* ConfigPtr)` |
| **Parameters** | `ConfigPtr` — Pointer to Fee module configuration |
| **Return** | `void` |
| **Pre-conditions** | Module may be in any state |
| **Post-conditions** | `Fee_ModuleStatus = MEMIF_BUSY_INTERNAL`; `Fee_InternalState = FEE_STATE_INIT_READ_SECTOR_HEADER` |
| **Re-entrancy** | Non-reentrant |
| **DET Errors** | `FEE_E_PARAM_POINTER` (0x04) if `ConfigPtr == NULL` |
| **DEM Errors** | None |

### Fee_SetMode

| Attribute | Value |
|---|---|
| **Signature** | `void Fee_SetMode(MemIf_ModeType Mode)` |
| **Parameters** | `Mode` — Requested operating mode |
| **Return** | `void` |
| **Pre-conditions** | None |
| **Post-conditions** | No effect (polling mode, no-op) |
| **Re-entrancy** | Non-reentrant |
| **DET Errors** | None |
| **DEM Errors** | None |

### Fee_Read

| Attribute | Value |
|---|---|
| **Signature** | `Std_ReturnType Fee_Read(uint16 BlockNumber, uint16 BlockOffset, uint8* DataBufferPtr, uint16 Length)` |
| **Parameters** | `BlockNumber` — NvM block number; `BlockOffset` — Offset within block; `DataBufferPtr` — Destination buffer; `Length` — Bytes to read |
| **Return** | `E_OK` if job accepted, `E_NOT_OK` if rejected |
| **Pre-conditions** | Module initialized (`MEMIF_IDLE` or `MEMIF_BUSY_INTERNAL`) |
| **Post-conditions** | If accepted: `Fee_ModuleStatus = MEMIF_BUSY`, `Fee_LastJobResult = MEMIF_JOB_PENDING` |
| **Re-entrancy** | Non-reentrant |
| **DET Errors** | `FEE_E_UNINIT` (0x01), `FEE_E_INVALID_BLOCK_NO` (0x02), `FEE_E_INVALID_BLOCK_OFS` (0x03), `FEE_E_PARAM_POINTER` (0x04), `FEE_E_INVALID_BLOCK_LEN` (0x05), `FEE_E_BUSY` (0x06) |
| **DEM Errors** | `FEE_E_HARDWARE_ERROR` on MemAcc failure or CRC mismatch during processing |

### Fee_Write

| Attribute | Value |
|---|---|
| **Signature** | `Std_ReturnType Fee_Write(uint16 BlockNumber, const uint8* DataBufferPtr)` |
| **Parameters** | `BlockNumber` — NvM block number; `DataBufferPtr` — Source data buffer |
| **Return** | `E_OK` if job accepted, `E_NOT_OK` if rejected |
| **Pre-conditions** | Module initialized |
| **Post-conditions** | If accepted: `MEMIF_BUSY`, `MEMIF_JOB_PENDING`; for immediate blocks during GC, GC is suspended |
| **Re-entrancy** | Non-reentrant |
| **DET Errors** | `FEE_E_UNINIT` (0x01), `FEE_E_INVALID_BLOCK_NO` (0x02), `FEE_E_PARAM_POINTER` (0x04), `FEE_E_BUSY` (0x06) |
| **DEM Errors** | `FEE_E_HARDWARE_ERROR` on MemAcc failure during processing |

### Fee_Cancel

| Attribute | Value |
|---|---|
| **Signature** | `void Fee_Cancel(void)` |
| **Parameters** | None |
| **Return** | `void` |
| **Pre-conditions** | Module initialized |
| **Post-conditions** | Current job result set to `MEMIF_JOB_CANCELED`; `MEMIF_IDLE` (or `MEMIF_BUSY_INTERNAL` if GC resumes) |
| **Re-entrancy** | Non-reentrant |
| **DET Errors** | `FEE_E_UNINIT` (0x01) |
| **DEM Errors** | None |

### Fee_GetStatus

| Attribute | Value |
|---|---|
| **Signature** | `MemIf_StatusType Fee_GetStatus(void)` |
| **Parameters** | None |
| **Return** | `MEMIF_UNINIT`, `MEMIF_IDLE`, `MEMIF_BUSY`, or `MEMIF_BUSY_INTERNAL` |
| **Pre-conditions** | None |
| **Post-conditions** | None |
| **Re-entrancy** | Reentrant |
| **DET Errors** | None |
| **DEM Errors** | None |

### Fee_GetJobResult

| Attribute | Value |
|---|---|
| **Signature** | `MemIf_JobResultType Fee_GetJobResult(void)` |
| **Parameters** | None |
| **Return** | `MEMIF_JOB_OK`, `MEMIF_JOB_PENDING`, `MEMIF_JOB_CANCELED`, `MEMIF_JOB_FAILED`, `MEMIF_BLOCK_INVALID`, `MEMIF_BLOCK_INCONSISTENT` |
| **Pre-conditions** | Module initialized |
| **Post-conditions** | None |
| **Re-entrancy** | Reentrant |
| **DET Errors** | `FEE_E_UNINIT` (0x01) |
| **DEM Errors** | None |

### Fee_InvalidateBlock

| Attribute | Value |
|---|---|
| **Signature** | `Std_ReturnType Fee_InvalidateBlock(uint16 BlockNumber)` |
| **Parameters** | `BlockNumber` — NvM block number |
| **Return** | `E_OK` / `E_NOT_OK` |
| **Pre-conditions** | Module initialized |
| **Post-conditions** | If accepted: `MEMIF_BUSY`, `MEMIF_JOB_PENDING` |
| **Re-entrancy** | Non-reentrant |
| **DET Errors** | `FEE_E_UNINIT` (0x01), `FEE_E_INVALID_BLOCK_NO` (0x02), `FEE_E_BUSY` (0x06) |
| **DEM Errors** | `FEE_E_HARDWARE_ERROR` on MemAcc failure |

### Fee_GetVersionInfo

| Attribute | Value |
|---|---|
| **Signature** | `void Fee_GetVersionInfo(Std_VersionInfoType* VersionInfoPtr)` |
| **Parameters** | `VersionInfoPtr` — Pointer to version info structure |
| **Return** | `void` |
| **Pre-conditions** | None |
| **Post-conditions** | Structure filled with vendorID=0xFFFF, moduleID=21, sw_major=2, sw_minor=0, sw_patch=0 |
| **Re-entrancy** | Reentrant |
| **DET Errors** | `FEE_E_PARAM_POINTER` (0x04) if NULL |
| **DEM Errors** | None |

### Fee_EraseImmediateBlock

| Attribute | Value |
|---|---|
| **Signature** | `Std_ReturnType Fee_EraseImmediateBlock(uint16 BlockNumber)` |
| **Parameters** | `BlockNumber` — NvM block number (must be immediate-data block) |
| **Return** | `E_OK` / `E_NOT_OK` |
| **Pre-conditions** | Module initialized; block must have `ImmediateData == TRUE` |
| **Post-conditions** | If accepted: `MEMIF_BUSY`, `MEMIF_JOB_PENDING` |
| **Re-entrancy** | Non-reentrant |
| **DET Errors** | `FEE_E_UNINIT` (0x01), `FEE_E_INVALID_BLOCK_NO` (0x02), `FEE_E_BUSY` (0x06) |
| **DEM Errors** | `FEE_E_HARDWARE_ERROR` on MemAcc failure |

### Fee_MainFunction

| Attribute | Value |
|---|---|
| **Signature** | `void Fee_MainFunction(void)` |
| **Parameters** | None |
| **Return** | `void` |
| **Pre-conditions** | Module initialized |
| **Post-conditions** | One state machine step processed; safety cyclic check executed |
| **Re-entrancy** | Non-reentrant |
| **DET Errors** | `FEE_E_UNINIT` (0x01) as runtime error |
| **DEM Errors** | `FEE_E_HARDWARE_ERROR` for RAM integrity violation |

---

## 2  MemAcc APIs

### MemAcc_Init

| Attribute | Value |
|---|---|
| **Signature** | `void MemAcc_Init(const MemAcc_ConfigType* ConfigPtr)` |
| **Parameters** | `ConfigPtr` — Pointer to configuration |
| **Return** | `void` |
| **Pre-conditions** | Module may be in any state |
| **Post-conditions** | `MemAcc_ModuleStatus = MEMACC_IDLE`; all per-area state cleared |
| **Re-entrancy** | Non-reentrant |
| **DET Errors** | `MEMACC_E_PARAM_POINTER` (0x02) |
| **DEM Errors** | None |

### MemAcc_DeInit

| Attribute | Value |
|---|---|
| **Signature** | `void MemAcc_DeInit(void)` |
| **Parameters** | None |
| **Return** | `void` |
| **Pre-conditions** | Module initialized |
| **Post-conditions** | `MemAcc_ModuleStatus = MEMACC_UNINIT` |
| **Re-entrancy** | Non-reentrant |
| **DET Errors** | `MEMACC_E_UNINIT` (0x01) |
| **DEM Errors** | None |

### MemAcc_Read

| Attribute | Value |
|---|---|
| **Signature** | `Std_ReturnType MemAcc_Read(MemAcc_AddressAreaIdType AreaId, MemAcc_AddressType Address, MemAcc_DataType* DataPtr, MemAcc_LengthType Length)` |
| **Parameters** | `AreaId` — Area ID; `Address` — Logical address; `DataPtr` — Destination; `Length` — Bytes |
| **Return** | `E_OK` / `E_NOT_OK` |
| **Pre-conditions** | Module initialized; area not busy; area not locked |
| **Post-conditions** | `MemAcc_AreaBusy[AreaId] = TRUE`; `MEMACC_JOB_PENDING` |
| **Re-entrancy** | Non-reentrant for same AreaId |
| **DET Errors** | `MEMACC_E_UNINIT` (0x01), `MEMACC_E_PARAM_POINTER` (0x02), `MEMACC_E_PARAM_ADDRESS_AREA` (0x03), `MEMACC_E_PARAM_ADDRESS` (0x04), `MEMACC_E_PARAM_LENGTH` (0x05), `MEMACC_E_BUSY` (0x06) |
| **DEM Errors** | None at acceptance; `MEMACC_E_HARDWARE_ERROR` during MainFunction processing |

### MemAcc_Write

| Attribute | Value |
|---|---|
| **Signature** | `Std_ReturnType MemAcc_Write(MemAcc_AddressAreaIdType AreaId, MemAcc_AddressType Address, const MemAcc_DataType* DataPtr, MemAcc_LengthType Length)` |
| **Parameters** | `AreaId`; `Address`; `DataPtr` — Source; `Length` |
| **Return** | `E_OK` / `E_NOT_OK` |
| **Pre-conditions** | Module initialized; area not busy; area not locked |
| **Post-conditions** | `MemAcc_AreaBusy[AreaId] = TRUE`; `MEMACC_JOB_PENDING` |
| **Re-entrancy** | Non-reentrant for same AreaId |
| **DET Errors** | Same as `MemAcc_Read` |
| **DEM Errors** | `MEMACC_E_HARDWARE_ERROR` during MainFunction processing |

### MemAcc_Erase

| Attribute | Value |
|---|---|
| **Signature** | `Std_ReturnType MemAcc_Erase(MemAcc_AddressAreaIdType AreaId, MemAcc_AddressType Address, MemAcc_LengthType Length)` |
| **Parameters** | `AreaId`; `Address`; `Length` |
| **Return** | `E_OK` / `E_NOT_OK` |
| **Pre-conditions** | Module initialized; area not busy; area not locked |
| **Post-conditions** | `MemAcc_AreaBusy[AreaId] = TRUE`; `MEMACC_JOB_PENDING` |
| **Re-entrancy** | Non-reentrant for same AreaId |
| **DET Errors** | `MEMACC_E_UNINIT`, `MEMACC_E_PARAM_ADDRESS_AREA`, `MEMACC_E_PARAM_LENGTH`, `MEMACC_E_PARAM_ADDRESS`, `MEMACC_E_BUSY` |
| **DEM Errors** | `MEMACC_E_HARDWARE_ERROR` during processing |

### MemAcc_BlankCheck

| Attribute | Value |
|---|---|
| **Signature** | `Std_ReturnType MemAcc_BlankCheck(MemAcc_AddressAreaIdType AreaId, MemAcc_AddressType Address, MemAcc_LengthType Length)` |
| **Parameters** | `AreaId`; `Address`; `Length` |
| **Return** | `E_OK` / `E_NOT_OK` |
| **Pre-conditions** | Module initialized; area not busy; area not locked |
| **Post-conditions** | `MemAcc_AreaBusy[AreaId] = TRUE`; `MEMACC_JOB_PENDING` |
| **Re-entrancy** | Non-reentrant for same AreaId |
| **DET Errors** | `MEMACC_E_UNINIT`, `MEMACC_E_PARAM_ADDRESS_AREA`, `MEMACC_E_PARAM_LENGTH`, `MEMACC_E_PARAM_ADDRESS`, `MEMACC_E_BUSY` |
| **DEM Errors** | `MEMACC_E_HARDWARE_ERROR` during processing |

### MemAcc_Compare

| Attribute | Value |
|---|---|
| **Signature** | `Std_ReturnType MemAcc_Compare(MemAcc_AddressAreaIdType AreaId, MemAcc_AddressType Address, const MemAcc_DataType* DataPtr, MemAcc_LengthType Length)` |
| **Parameters** | `AreaId`; `Address`; `DataPtr` — Reference data; `Length` |
| **Return** | `E_OK` / `E_NOT_OK` |
| **Pre-conditions** | Module initialized; area not busy; area not locked |
| **Post-conditions** | `MemAcc_AreaBusy[AreaId] = TRUE`; `MEMACC_JOB_PENDING` |
| **Re-entrancy** | Non-reentrant for same AreaId |
| **DET Errors** | `MEMACC_E_UNINIT`, `MEMACC_E_PARAM_POINTER`, `MEMACC_E_PARAM_ADDRESS_AREA`, `MEMACC_E_PARAM_LENGTH`, `MEMACC_E_PARAM_ADDRESS`, `MEMACC_E_BUSY` |
| **DEM Errors** | `MEMACC_E_HARDWARE_ERROR` during compare read failure |

### MemAcc_GetProcessedLength

| Attribute | Value |
|---|---|
| **Signature** | `MemAcc_LengthType MemAcc_GetProcessedLength(MemAcc_AddressAreaIdType AreaId)` |
| **Parameters** | `AreaId` |
| **Return** | Number of bytes processed |
| **Pre-conditions** | Module initialized |
| **Post-conditions** | None |
| **Re-entrancy** | Reentrant |
| **DET Errors** | `MEMACC_E_UNINIT`, `MEMACC_E_PARAM_ADDRESS_AREA` |
| **DEM Errors** | None |

### MemAcc_HwSpecificServiceRequest

| Attribute | Value |
|---|---|
| **Signature** | `Std_ReturnType MemAcc_HwSpecificServiceRequest(MemAcc_AddressAreaIdType AreaId, MemAcc_DataType* DataPtr, MemAcc_LengthType Length)` |
| **Parameters** | `AreaId`; `DataPtr`; `Length` |
| **Return** | Always `E_NOT_OK` (`<PLACEHOLDER_REQUIRED>`) |
| **Pre-conditions** | N/A |
| **Post-conditions** | None |
| **Re-entrancy** | N/A |
| **DET Errors** | None |
| **DEM Errors** | None |

### MemAcc_Cancel

| Attribute | Value |
|---|---|
| **Signature** | `void MemAcc_Cancel(MemAcc_AddressAreaIdType AreaId)` |
| **Parameters** | `AreaId` |
| **Return** | `void` |
| **Pre-conditions** | Module initialized |
| **Post-conditions** | If area busy: `MEMACC_JOB_CANCELED`, busy cleared |
| **Re-entrancy** | Non-reentrant for same AreaId |
| **DET Errors** | `MEMACC_E_UNINIT`, `MEMACC_E_PARAM_ADDRESS_AREA` |
| **DEM Errors** | None |

### MemAcc_GetJobResult

| Attribute | Value |
|---|---|
| **Signature** | `MemAcc_JobResultType MemAcc_GetJobResult(MemAcc_AddressAreaIdType AreaId)` |
| **Parameters** | `AreaId` |
| **Return** | `MEMACC_JOB_OK`, `MEMACC_JOB_PENDING`, `MEMACC_JOB_FAILED`, `MEMACC_JOB_CANCELED`, `MEMACC_JOB_ECC_CORRECTED`, `MEMACC_JOB_ECC_UNCORRECTED` |
| **Pre-conditions** | Module initialized |
| **Post-conditions** | None |
| **Re-entrancy** | Reentrant |
| **DET Errors** | `MEMACC_E_UNINIT`, `MEMACC_E_PARAM_ADDRESS_AREA` |
| **DEM Errors** | None |

### MemAcc_GetSegmentationInfo

| Attribute | Value |
|---|---|
| **Signature** | `Std_ReturnType MemAcc_GetSegmentationInfo(MemAcc_AddressAreaIdType AreaId, MemAcc_LengthType* SectorSizePtr, MemAcc_LengthType* PageSizePtr)` |
| **Parameters** | `AreaId`; `SectorSizePtr` — Output; `PageSizePtr` — Output |
| **Return** | `E_OK` / `E_NOT_OK` |
| **Pre-conditions** | Module initialized |
| **Post-conditions** | Pointers filled with area's sector/page sizes |
| **Re-entrancy** | Reentrant |
| **DET Errors** | `MEMACC_E_UNINIT`, `MEMACC_E_PARAM_ADDRESS_AREA`, `MEMACC_E_PARAM_POINTER` |
| **DEM Errors** | None |

### MemAcc_RequestLock

| Attribute | Value |
|---|---|
| **Signature** | `Std_ReturnType MemAcc_RequestLock(MemAcc_AddressAreaIdType AreaId)` |
| **Parameters** | `AreaId` |
| **Return** | `E_OK` if lock acquired, `E_NOT_OK` if already locked |
| **Pre-conditions** | Module initialized |
| **Post-conditions** | `MemAcc_AreaLocked[AreaId] = TRUE` |
| **Re-entrancy** | Non-reentrant for same AreaId |
| **DET Errors** | `MEMACC_E_UNINIT`, `MEMACC_E_PARAM_ADDRESS_AREA` |
| **DEM Errors** | None |

### MemAcc_ReleaseLock

| Attribute | Value |
|---|---|
| **Signature** | `Std_ReturnType MemAcc_ReleaseLock(MemAcc_AddressAreaIdType AreaId)` |
| **Parameters** | `AreaId` |
| **Return** | `E_OK` if lock released, `E_NOT_OK` if not locked |
| **Pre-conditions** | Module initialized |
| **Post-conditions** | `MemAcc_AreaLocked[AreaId] = FALSE` |
| **Re-entrancy** | Non-reentrant for same AreaId |
| **DET Errors** | `MEMACC_E_UNINIT`, `MEMACC_E_PARAM_ADDRESS_AREA` |
| **DEM Errors** | None |

### MemAcc_GetVersionInfo

| Attribute | Value |
|---|---|
| **Signature** | `void MemAcc_GetVersionInfo(Std_VersionInfoType* VersionInfoPtr)` |
| **Parameters** | `VersionInfoPtr` |
| **Return** | `void` |
| **Pre-conditions** | None |
| **Post-conditions** | Structure filled: vendorID=0xFFFF, moduleID=166, sw_major=1, sw_minor=0, sw_patch=0 |
| **Re-entrancy** | Reentrant |
| **DET Errors** | `MEMACC_E_PARAM_POINTER` (0x02) |
| **DEM Errors** | None |

### MemAcc_MainFunction

| Attribute | Value |
|---|---|
| **Signature** | `void MemAcc_MainFunction(void)` |
| **Parameters** | None |
| **Return** | `void` |
| **Pre-conditions** | Module initialized |
| **Post-conditions** | Per-area job results updated based on `Mem_DFLS_GetJobResult` |
| **Re-entrancy** | Non-reentrant |
| **DET Errors** | None |
| **DEM Errors** | `MEMACC_E_HARDWARE_ERROR` on driver failure or ECC uncorrected |
