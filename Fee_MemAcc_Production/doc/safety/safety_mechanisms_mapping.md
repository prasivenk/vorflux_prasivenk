# Safety Mechanisms Mapping

**AUTOSAR R24-11 Fee + MemAcc | ASIL-B**

---

| Safety Mechanism | Type | Safety Req | Implementation (file:function) | Diagnostic Coverage |
|---|---|---|---|---|
| CRC-16 CCITT data verification on read | Information redundancy | SR-001 | `src/Fee_StateMachine.c:Fee_StateMachine_Process` (state `FEE_STATE_READ_VERIFY_CRC`) calls `src/Fee_Crc.c:Fee_Crc_CalculateBlock` | High (≥ 99.998%, HD=4) |
| CRC-16 CCITT data verification during GC copy | Information redundancy | SR-001 | `src/Fee_GarbageCollect.c:Fee_GarbageCollect_Process` (state `FEE_GC_COPY_READ_WAIT`) calls `src/Fee_Crc.c:Fee_Crc_CalculateBlock` | High (≥ 99.998%, HD=4) |
| Block header CRC validation | Information redundancy | SR-001 | `src/Fee_Sector.c:Fee_Sector_ParseBlockHeader` | High (≥ 99.998%) |
| Sector header CRC validation | Information redundancy | SR-001 | `src/Fee_Sector.c:Fee_Sector_ParseSectorHeader` | High (≥ 99.998%) |
| Two-phase commit (ValidMarker) | Temporal redundancy | SR-002 | `src/Fee_StateMachine.c:Fee_StateMachine_Process` (states `FEE_STATE_WRITE_HEADER` → `FEE_STATE_WRITE_VALID`) | Very High (~100%) |
| GC two-phase commit (ValidMarker) | Temporal redundancy | SR-002 | `src/Fee_GarbageCollect.c:Fee_GarbageCollect_Process` (states `FEE_GC_COPY_WRITE_HEADER` → `FEE_GC_COPY_WRITE_VALID`) | Very High (~100%) |
| Incomplete write detection (init scan) | Pattern detection | SR-003 | `src/Fee_StateMachine.c:Fee_StateMachine_Process` (state `FEE_STATE_INIT_SCAN_WAIT_RECORD`) | Very High |
| RAM CRC cyclic integrity check | Information redundancy | SR-004 | `src/Fee_Safety.c:Fee_Safety_CyclicCheck` calls `src/Fee_Safety.c:Fee_Safety_ComputeRamCrc` via `src/Fee_Crc.c:Fee_Crc_Calculate` | High (≥ 99.998%) |
| RAM CRC update after modification | Information redundancy | SR-004 | `src/Fee_Safety.c:Fee_Safety_UpdateRamCrc` | N/A (supporting mechanism) |
| DEM production error reporting (Fee) | Error notification | SR-005 | `src/Fee_StateMachine.c:Fee_StateMachine_Process` — `Dem_SetEventStatus(FEE_E_HARDWARE_ERROR, DEM_EVENT_STATUS_FAILED)` | N/A (notification) |
| DEM production error reporting (MemAcc) | Error notification | SR-005 | `src/MemAcc_JobProcessing.c:MemAcc_MainFunction` — `Dem_SetEventStatus(MEMACC_E_HARDWARE_ERROR, DEM_EVENT_STATUS_FAILED)` | N/A (notification) |
| Static buffer allocation | Design constraint | SR-006 | `src/Fee_Sector.c` (HeaderBuffer, WriteBuffer), `src/Fee_GarbageCollect.c` (GcCopyBuffer), `src/MemAcc_JobProcessing.c` (CompareBuffer) | N/A (design rule) |
| One MemAcc op per MainFunction | Temporal bounding | SR-007 | `src/Fee_StateMachine.c:Fee_StateMachine_Process` — single switch-case branch per call | N/A (design rule) |
| SchM exclusive area protection | Mutual exclusion | SR-008 | `src/Fee.c` — `SchM_Enter_Fee_FEE_EXCLUSIVE_AREA_0` / `SchM_Exit_Fee_FEE_EXCLUSIVE_AREA_0` around `Fee_StateMachine_AcceptJob` | N/A (prevention) |
| SchM exclusive area (MemAcc) | Mutual exclusion | SR-008 | `src/MemAcc.c` — `SchM_Enter_MemAcc_MEMACC_EXCLUSIVE_AREA_0` / `SchM_Exit_MemAcc_MEMACC_EXCLUSIVE_AREA_0` | N/A (prevention) |
| Read-back verification | Comparison | SR-009 | `src/Fee_StateMachine.c:Fee_StateMachine_Process` (states `FEE_STATE_WRITE_VERIFY_READ` → `FEE_STATE_WRITE_VERIFY_COMPARE`) | Very High (~100%) |
| Sequence counter monotonicity | Temporal ordering | SR-010 | `src/Fee_StateMachine.c:Fee_StateMachine_Process` — `Fee_BlockSequenceCounters[blkIdx]++` in `FEE_STATE_WRITE_HEADER`; init scan comparison in `FEE_STATE_INIT_SCAN_WAIT_RECORD` | High |
| Wear leveling (lowest EraseCount selection) | Load balancing | SR-011 | `src/Fee_GarbageCollect.c:Fee_GarbageCollect_Trigger` — selects target sector with lowest `EraseCount` | N/A (prevention) |
| Defective sector marking | Error isolation | SR-012 | `src/Fee_StateMachine.c:Fee_StateMachine_Process` (state `FEE_STATE_INIT_WAIT_SECTOR_HEADER`) — MemAcc failure → `FEE_SECTOR_DEFECTIVE` | Very High |
| Error state transition | Safe state | SR-013 | `src/Fee_StateMachine.c:Fee_StateMachine_Process` (state `FEE_STATE_ERROR`) — DEM report → `FEE_STATE_IDLE` | N/A (response) |
| Flow counter monitoring | Program flow | SR-014 | `src/Fee_Safety.c:Fee_Safety_FlowCheck`, `src/Fee_Safety.c:Fee_Safety_ResetFlowCounter` | High |
| ECC error propagation | Error forwarding | SR-015 | `src/MemAcc_JobProcessing.c:MemAcc_MainFunction` — maps `MEM_DFLS_JOB_ECC_*` to `MEMACC_JOB_ECC_*` | `<PLACEHOLDER_REQUIRED>` (TC3xx-specific) |
| DET parameter validation (Fee) | Input validation | SR-008 | `src/Fee.c` — all public APIs check uninit, null ptr, invalid block, etc. | Very High |
| DET parameter validation (MemAcc) | Input validation | SR-008 | `src/MemAcc.c` — all public APIs check uninit, null ptr, invalid area, etc. | Very High |
| Address bounds validation | Range check | SR-008 | `src/MemAcc_AddressArea.c:MemAcc_Internal_ValidateAddress` | Very High |
