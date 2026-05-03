# State Transition Tables

**AUTOSAR R24-11 Fee + MemAcc**

---

## 1  Fee Main State Machine (`Fee_InternalStateType`)

| Current State | Event/Trigger | Guard | Next State | Actions |
|---|---|---|---|---|
| `FEE_STATE_UNINIT` | `Fee_Init(ConfigPtr)` | `ConfigPtr != NULL` | `FEE_STATE_INIT_READ_SECTOR_HEADER` | `Fee_StateMachine_Init`: init BlockInfoTable, SectorInfo; set `MEMIF_BUSY_INTERNAL` |
| `FEE_STATE_INIT_READ_SECTOR_HEADER` | `Fee_StateMachine_Process` | — | `FEE_STATE_INIT_WAIT_SECTOR_HEADER` | `MemAcc_Read(AreaId, SectorInfo[cursor].BaseAddress, HeaderBuf, 32)` |
| `FEE_STATE_INIT_WAIT_SECTOR_HEADER` | `Fee_StateMachine_Process` | `MemAcc_GetJobResult == MEMACC_JOB_PENDING` | `FEE_STATE_INIT_WAIT_SECTOR_HEADER` | Wait (return) |
| `FEE_STATE_INIT_WAIT_SECTOR_HEADER` | `Fee_StateMachine_Process` | `MemAcc_GetJobResult == MEMACC_JOB_OK` | `FEE_STATE_INIT_NEXT_SECTOR` | `Fee_Sector_ParseSectorHeader`; map status byte to enum |
| `FEE_STATE_INIT_WAIT_SECTOR_HEADER` | `Fee_StateMachine_Process` | `MemAcc_GetJobResult == FAILED` | `FEE_STATE_INIT_NEXT_SECTOR` | Mark sector `FEE_SECTOR_DEFECTIVE` |
| `FEE_STATE_INIT_NEXT_SECTOR` | `Fee_StateMachine_Process` | `cursor < NumberOfSectors` | `FEE_STATE_INIT_READ_SECTOR_HEADER` | Increment `Fee_InitSectorCursor` |
| `FEE_STATE_INIT_NEXT_SECTOR` | `Fee_StateMachine_Process` | `cursor >= NumberOfSectors` | `FEE_STATE_INIT_SCAN_READ_RECORD` | Determine `Fee_ActiveSectorIndex` (highest SequenceNumber); set `Fee_ScanRecordCursor` |
| `FEE_STATE_INIT_SCAN_READ_RECORD` | `Fee_StateMachine_Process` | `ScanRecordCursor < sectorEnd` | `FEE_STATE_INIT_SCAN_WAIT_RECORD` | `MemAcc_Read(AreaId, ScanRecordCursor, HeaderBuf, 32)` |
| `FEE_STATE_INIT_SCAN_READ_RECORD` | `Fee_StateMachine_Process` | `ScanRecordCursor >= sectorEnd` | `FEE_STATE_IDLE` | Set `MEMIF_IDLE`; `Fee_Safety_UpdateRamCrc` |
| `FEE_STATE_INIT_SCAN_WAIT_RECORD` | `Fee_StateMachine_Process` | `MEMACC_JOB_PENDING` | `FEE_STATE_INIT_SCAN_WAIT_RECORD` | Wait (return) |
| `FEE_STATE_INIT_SCAN_WAIT_RECORD` | `Fee_StateMachine_Process` | `MEMACC_JOB_OK` + header blank | `FEE_STATE_IDLE` | Update WritePointer, FreeSpace; `MEMIF_IDLE` |
| `FEE_STATE_INIT_SCAN_WAIT_RECORD` | `Fee_StateMachine_Process` | `MEMACC_JOB_OK` + header valid | `FEE_STATE_INIT_SCAN_READ_RECORD` | `Fee_Sector_ParseBlockHeader`; update BlockInfoTable; advance cursor |
| `FEE_STATE_INIT_SCAN_WAIT_RECORD` | `Fee_StateMachine_Process` | `MEMACC_JOB_OK` + corrupt header | `FEE_STATE_INIT_SCAN_READ_RECORD` | Advance cursor by `FEE_BLOCK_HEADER_SIZE` |
| `FEE_STATE_INIT_SCAN_WAIT_RECORD` | `Fee_StateMachine_Process` | `MEMACC_JOB_FAILED` | `FEE_STATE_IDLE` | Set `MEMIF_IDLE` |
| `FEE_STATE_IDLE` | `Fee_StateMachine_Process` | `Fee_CurrentJob.Type == FEE_JOB_READ` | `FEE_STATE_READ_START` | — |
| `FEE_STATE_IDLE` | `Fee_StateMachine_Process` | `Fee_CurrentJob.Type == FEE_JOB_WRITE` | `FEE_STATE_WRITE_ALLOC` | — |
| `FEE_STATE_IDLE` | `Fee_StateMachine_Process` | `Fee_CurrentJob.Type == FEE_JOB_INVALIDATE` | `FEE_STATE_INVALIDATE_WRITE` | — |
| `FEE_STATE_IDLE` | `Fee_StateMachine_Process` | `Fee_CurrentJob.Type == FEE_JOB_ERASE_IMMEDIATE` | `FEE_STATE_ERASE_IMMEDIATE` | — |
| `FEE_STATE_IDLE` | `Fee_StateMachine_Process` | `Fee_CurrentJob.Type == FEE_JOB_NONE` | `FEE_STATE_IDLE` | No-op |
| `FEE_STATE_READ_START` | `Fee_StateMachine_Process` | Block not found / invalid | `FEE_STATE_IDLE` | `CompleteJob(MEMIF_BLOCK_INVALID)` |
| `FEE_STATE_READ_START` | `Fee_StateMachine_Process` | Block inconsistent | `FEE_STATE_IDLE` | `CompleteJob(MEMIF_BLOCK_INCONSISTENT)` |
| `FEE_STATE_READ_START` | `Fee_StateMachine_Process` | Block valid | `FEE_STATE_READ_WAIT` | `MemAcc_Read(AreaId, DataAddress+Offset, ReadDataPtr, Length)` |
| `FEE_STATE_READ_WAIT` | `Fee_StateMachine_Process` | `MEMACC_JOB_PENDING` | `FEE_STATE_READ_WAIT` | Wait |
| `FEE_STATE_READ_WAIT` | `Fee_StateMachine_Process` | `MEMACC_JOB_OK` | `FEE_STATE_READ_VERIFY_CRC` | — |
| `FEE_STATE_READ_WAIT` | `Fee_StateMachine_Process` | `MEMACC_JOB_FAILED` | `FEE_STATE_IDLE` | DEM; `CompleteJob(MEMIF_JOB_FAILED)` |
| `FEE_STATE_READ_VERIFY_CRC` | `Fee_StateMachine_Process` | Full read + CRC match | `FEE_STATE_IDLE` | `CompleteJob(MEMIF_JOB_OK)` |
| `FEE_STATE_READ_VERIFY_CRC` | `Fee_StateMachine_Process` | Full read + CRC mismatch | `FEE_STATE_IDLE` | DEM; `CompleteJob(MEMIF_BLOCK_INCONSISTENT)` |
| `FEE_STATE_READ_VERIFY_CRC` | `Fee_StateMachine_Process` | Partial read | `FEE_STATE_IDLE` | `CompleteJob(MEMIF_JOB_OK)` (skip CRC) |
| `FEE_STATE_WRITE_ALLOC` | `Fee_StateMachine_Process` | Block invalid / sector full | `FEE_STATE_IDLE` | `CompleteJob(MEMIF_JOB_FAILED)` |
| `FEE_STATE_WRITE_ALLOC` | `Fee_StateMachine_Process` | Space allocated | `FEE_STATE_WRITE_HEADER` | `Fee_Sector_AllocateBlock`; update SectorInfo |
| `FEE_STATE_WRITE_HEADER` | `Fee_StateMachine_Process` | — | `FEE_STATE_WRITE_HEADER_WAIT` | CRC compute; seq counter++; `BuildBlockHeader`; `MemAcc_Write` |
| `FEE_STATE_WRITE_HEADER_WAIT` | `Fee_StateMachine_Process` | `MEMACC_JOB_OK` | `FEE_STATE_WRITE_DATA` | — |
| `FEE_STATE_WRITE_HEADER_WAIT` | `Fee_StateMachine_Process` | `MEMACC_JOB_FAILED` | `FEE_STATE_IDLE` | DEM; `CompleteJob(MEMIF_JOB_FAILED)` |
| `FEE_STATE_WRITE_DATA` | `Fee_StateMachine_Process` | — | `FEE_STATE_WRITE_DATA_WAIT` | `PrepareWriteBuffer`; `MemAcc_Write` |
| `FEE_STATE_WRITE_DATA_WAIT` | `Fee_StateMachine_Process` | `MEMACC_JOB_OK` | `FEE_STATE_WRITE_VALID` | — |
| `FEE_STATE_WRITE_DATA_WAIT` | `Fee_StateMachine_Process` | `MEMACC_JOB_FAILED` | `FEE_STATE_IDLE` | DEM; `CompleteJob(MEMIF_JOB_FAILED)` |
| `FEE_STATE_WRITE_VALID` | `Fee_StateMachine_Process` | — | `FEE_STATE_WRITE_VALID_WAIT` | Set HeaderBuf[30]=0x55; `MemAcc_Write` |
| `FEE_STATE_WRITE_VALID_WAIT` | `Fee_StateMachine_Process` | `MEMACC_JOB_OK` + `FEE_READ_BACK_VERIFICATION==ON` | `FEE_STATE_WRITE_VERIFY_READ` | — |
| `FEE_STATE_WRITE_VALID_WAIT` | `Fee_StateMachine_Process` | `MEMACC_JOB_OK` + `FEE_READ_BACK_VERIFICATION==OFF` | `FEE_STATE_IDLE` | Update BlockInfoTable; `UpdateRamCrc`; `CompleteJob(OK)` |
| `FEE_STATE_WRITE_VALID_WAIT` | `Fee_StateMachine_Process` | `MEMACC_JOB_FAILED` | `FEE_STATE_IDLE` | DEM; `CompleteJob(MEMIF_JOB_FAILED)` |
| `FEE_STATE_WRITE_VERIFY_READ` | `Fee_StateMachine_Process` | — | `FEE_STATE_WRITE_VERIFY_WAIT` | `MemAcc_Read` for read-back |
| `FEE_STATE_WRITE_VERIFY_WAIT` | `Fee_StateMachine_Process` | `MEMACC_JOB_OK` | `FEE_STATE_WRITE_VERIFY_COMPARE` | — |
| `FEE_STATE_WRITE_VERIFY_COMPARE` | `Fee_StateMachine_Process` | Data matches | `FEE_STATE_IDLE` | Update BlockInfoTable; `UpdateRamCrc`; `CompleteJob(OK)` |
| `FEE_STATE_WRITE_VERIFY_COMPARE` | `Fee_StateMachine_Process` | Data mismatch | `FEE_STATE_IDLE` | DEM; `CompleteJob(MEMIF_JOB_FAILED)` |
| `FEE_STATE_INVALIDATE_WRITE` | `Fee_StateMachine_Process` | Block not found | `FEE_STATE_IDLE` | `CompleteJob(MEMIF_JOB_OK)` |
| `FEE_STATE_INVALIDATE_WRITE` | `Fee_StateMachine_Process` | Block exists | `FEE_STATE_INVALIDATE_WAIT` | Read header; set byte 30=0xFF; `MemAcc_Write` |
| `FEE_STATE_INVALIDATE_WAIT` | `Fee_StateMachine_Process` | `MEMACC_JOB_OK` | `FEE_STATE_IDLE` | Set `FEE_BLOCK_INVALID`; `UpdateRamCrc`; `CompleteJob(OK)` |
| `FEE_STATE_INVALIDATE_WAIT` | `Fee_StateMachine_Process` | `MEMACC_JOB_FAILED` | `FEE_STATE_IDLE` | DEM; `CompleteJob(MEMIF_JOB_FAILED)` |
| `FEE_STATE_ERASE_IMMEDIATE` | `Fee_StateMachine_Process` | Existing valid/inconsistent entry | `FEE_STATE_ERASE_IMMEDIATE_WAIT` | Invalidate header; `MemAcc_Write` |
| `FEE_STATE_ERASE_IMMEDIATE` | `Fee_StateMachine_Process` | No entry or already invalid | `FEE_STATE_IDLE` | `CompleteJob(MEMIF_JOB_OK)` |
| `FEE_STATE_ERASE_IMMEDIATE_WAIT` | `Fee_StateMachine_Process` | `MEMACC_JOB_OK` | `FEE_STATE_IDLE` | Set `FEE_BLOCK_NOT_FOUND`; `UpdateRamCrc`; `CompleteJob(OK)` |
| `FEE_STATE_GC_ACTIVE` | `Fee_StateMachine_Process` | `FEE_GC_COMPLETE` | `FEE_STATE_IDLE` | Set `MEMIF_IDLE` |
| `FEE_STATE_GC_ACTIVE` | `Fee_StateMachine_Process` | `FEE_GC_ERROR` | `FEE_STATE_ERROR` | — |
| `FEE_STATE_GC_ACTIVE` | `Fee_StateMachine_Process` | `FEE_GC_IN_PROGRESS` | `FEE_STATE_GC_ACTIVE` | Continue GC |
| `FEE_STATE_ERROR` | `Fee_StateMachine_Process` | — | `FEE_STATE_IDLE` | DEM report; set `MEMIF_IDLE` |

---

## 2  Fee GC State Machine (`Fee_GcInternalStateType`)

| Current State | Event/Trigger | Guard | Next State | Actions |
|---|---|---|---|---|
| `FEE_GC_IDLE` | `Fee_GarbageCollect_Trigger` | Source + target found | `FEE_GC_SELECT_SOURCE` | Set source/target sector indices |
| `FEE_GC_IDLE` | `Fee_GarbageCollect_Process` | — | (returns `FEE_GC_COMPLETE`) | — |
| `FEE_GC_SELECT_SOURCE` | `Fee_GarbageCollect_Process` | VALID block found | `FEE_GC_COPY_READ` | Set `Fee_GcCurrentBlock` |
| `FEE_GC_SELECT_SOURCE` | `Fee_GarbageCollect_Process` | No more VALID blocks | `FEE_GC_ERASE_SOURCE` | — |
| `FEE_GC_COPY_READ` | `Fee_GarbageCollect_Process` | `MemAcc_Read` accepted | `FEE_GC_COPY_READ_WAIT` | Read block data into `Fee_GcCopyBuffer` |
| `FEE_GC_COPY_READ` | `Fee_GarbageCollect_Process` | `MemAcc_Read` rejected | `FEE_GC_IDLE` | DEM; return `FEE_GC_ERROR` |
| `FEE_GC_COPY_READ_WAIT` | `Fee_GarbageCollect_Process` | `MEMACC_JOB_PENDING` | `FEE_GC_COPY_READ_WAIT` | Return `FEE_GC_IN_PROGRESS` |
| `FEE_GC_COPY_READ_WAIT` | `Fee_GarbageCollect_Process` | `MEMACC_JOB_OK` + CRC OK | `FEE_GC_COPY_WRITE_HEADER` | — |
| `FEE_GC_COPY_READ_WAIT` | `Fee_GarbageCollect_Process` | `MEMACC_JOB_OK` + CRC mismatch | `FEE_GC_SELECT_SOURCE` | Mark block `FEE_BLOCK_INCONSISTENT`; skip |
| `FEE_GC_COPY_READ_WAIT` | `Fee_GarbageCollect_Process` | `MEMACC_JOB_FAILED` | `FEE_GC_IDLE` | DEM; return `FEE_GC_ERROR` |
| `FEE_GC_COPY_WRITE_HEADER` | `Fee_GarbageCollect_Process` | Space allocated | `FEE_GC_COPY_WRITE_HEADER_WAIT` | `AllocateBlock`; `BuildBlockHeader`; `MemAcc_Write` |
| `FEE_GC_COPY_WRITE_HEADER` | `Fee_GarbageCollect_Process` | Target sector full | `FEE_GC_IDLE` | DEM; return `FEE_GC_ERROR` |
| `FEE_GC_COPY_WRITE_HEADER_WAIT` | `Fee_GarbageCollect_Process` | `MEMACC_JOB_OK` | `FEE_GC_COPY_WRITE_DATA` | — |
| `FEE_GC_COPY_WRITE_HEADER_WAIT` | `Fee_GarbageCollect_Process` | `MEMACC_JOB_FAILED` | `FEE_GC_IDLE` | DEM; return `FEE_GC_ERROR` |
| `FEE_GC_COPY_WRITE_DATA` | `Fee_GarbageCollect_Process` | — | `FEE_GC_COPY_WRITE_DATA_WAIT` | `PrepareWriteBuffer`; `MemAcc_Write` |
| `FEE_GC_COPY_WRITE_DATA_WAIT` | `Fee_GarbageCollect_Process` | `MEMACC_JOB_OK` | `FEE_GC_COPY_WRITE_VALID` | — |
| `FEE_GC_COPY_WRITE_DATA_WAIT` | `Fee_GarbageCollect_Process` | `MEMACC_JOB_FAILED` | `FEE_GC_IDLE` | DEM; return `FEE_GC_ERROR` |
| `FEE_GC_COPY_WRITE_VALID` | `Fee_GarbageCollect_Process` | — | `FEE_GC_COPY_WRITE_VALID_WAIT` | `BuildBlockHeader` with valid marker; `MemAcc_Write` |
| `FEE_GC_COPY_WRITE_VALID_WAIT` | `Fee_GarbageCollect_Process` | `MEMACC_JOB_OK` | `FEE_GC_SELECT_SOURCE` | Update BlockInfoTable; `Fee_Safety_UpdateRamCrc` |
| `FEE_GC_COPY_WRITE_VALID_WAIT` | `Fee_GarbageCollect_Process` | `MEMACC_JOB_FAILED` | `FEE_GC_IDLE` | DEM; return `FEE_GC_ERROR` |
| `FEE_GC_ERASE_SOURCE` | `Fee_GarbageCollect_Process` | `MemAcc_Erase` accepted | `FEE_GC_ERASE_WAIT` | `MemAcc_Erase(AreaId, BaseAddress, SectorSize)` |
| `FEE_GC_ERASE_SOURCE` | `Fee_GarbageCollect_Process` | `MemAcc_Erase` rejected | `FEE_GC_IDLE` | DEM; return `FEE_GC_ERROR` |
| `FEE_GC_ERASE_WAIT` | `Fee_GarbageCollect_Process` | `MEMACC_JOB_OK` | `FEE_GC_COMPLETE_STATE` | Increment EraseCount; reset stale entries; set `FEE_SECTOR_ERASED`; `UpdateRamCrc` |
| `FEE_GC_ERASE_WAIT` | `Fee_GarbageCollect_Process` | `MEMACC_JOB_FAILED` | `FEE_GC_IDLE` | DEM; return `FEE_GC_ERROR` |
| `FEE_GC_COMPLETE_STATE` | `Fee_GarbageCollect_Process` | — | `FEE_GC_IDLE` | Return `FEE_GC_COMPLETE` |

**Suspension:** Any state (except `FEE_GC_IDLE`) with `Fee_GcSuspended == TRUE` returns `FEE_GC_IN_PROGRESS` without advancing.

---

## 3  MemAcc Per-Area State Machine

| Current State | Event/Trigger | Guard | Next State | Actions |
|---|---|---|---|---|
| `MEMACC_UNINIT` | `MemAcc_Init(ConfigPtr)` | `ConfigPtr != NULL` | `MEMACC_IDLE` | Init per-area arrays |
| `MEMACC_IDLE` | `MemAcc_Read/Write/Erase/BlankCheck/Compare` | `!Busy && !Locked` | `MEMACC_BUSY` (per area) | Fill `CurrentJob`; set `MEMACC_JOB_PENDING`; dispatch to Mem_DFLS |
| `MEMACC_IDLE` | `MemAcc_Read/Write/Erase/BlankCheck/Compare` | `Busy` | `MEMACC_IDLE` | DET `MEMACC_E_BUSY`; return `E_NOT_OK` |
| `MEMACC_IDLE` | `MemAcc_Read/Write/Erase/BlankCheck/Compare` | `Locked` | `MEMACC_IDLE` | Return `E_NOT_OK` |
| `MEMACC_BUSY` | `MemAcc_MainFunction` | `Mem_DFLS result == PENDING` | `MEMACC_BUSY` | Continue |
| `MEMACC_BUSY` | `MemAcc_MainFunction` | `Mem_DFLS result == OK` | `MEMACC_IDLE` (per area) | Set `MEMACC_JOB_OK`; clear busy |
| `MEMACC_BUSY` | `MemAcc_MainFunction` | `Mem_DFLS result == FAILED` | `MEMACC_IDLE` (per area) | DEM; set `MEMACC_JOB_FAILED`; clear busy |
| `MEMACC_BUSY` | `MemAcc_MainFunction` | `Mem_DFLS result == ECC_CORRECTED` | `MEMACC_IDLE` (per area) | Set `MEMACC_JOB_ECC_CORRECTED`; clear busy |
| `MEMACC_BUSY` | `MemAcc_MainFunction` | `Mem_DFLS result == ECC_UNCORRECTED` | `MEMACC_IDLE` (per area) | DEM; set `MEMACC_JOB_ECC_UNCORRECTED`; clear busy |
| `MEMACC_BUSY` | `MemAcc_Cancel(AreaId)` | — | `MEMACC_IDLE` (per area) | Set `MEMACC_JOB_CANCELED`; clear busy |
| `MEMACC_IDLE` | `MemAcc_DeInit()` | — | `MEMACC_UNINIT` | Reset all; `ConfigPtr = NULL` |
