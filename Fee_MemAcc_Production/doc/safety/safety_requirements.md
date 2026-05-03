# Safety Requirements

**AUTOSAR R24-11 Fee + MemAcc | ASIL-B**

---

| SR ID | Title | Description | ASIL |
|-------|-------|-------------|------|
| SR-001 | CRC Verification on Read | The Fee module shall verify the CRC-16 CCITT of block data on every full-block read and shall report `MEMIF_BLOCK_INCONSISTENT` if the computed CRC does not match the stored `DataCrc` in `Fee_BlockInfoTable`. | B |
| SR-002 | Write Atomicity (Two-Phase Commit) | The Fee module shall implement a two-phase commit protocol: (1) write block header with `ValidMarker = FEE_MARKER_ERASED` (0x00) and data, (2) write `ValidMarker = FEE_MARKER_VALID` (0x55) only after data write completes successfully. An incomplete write shall be detectable as `FEE_BLOCK_INCONSISTENT`. | B |
| SR-003 | Power-Loss Recovery | After a power loss during any write or GC operation, the Fee module shall recover to a consistent state on the next initialization: blocks with `ValidMarker != FEE_MARKER_VALID` shall be treated as inconsistent or not-found. | B |
| SR-004 | RAM Integrity Monitoring | The Fee module shall compute a CRC-16 CCITT over `Fee_BlockInfoTable[]` and `Fee_SectorInfo[]` at initialization and shall cyclically verify this CRC in every `Fee_MainFunction()` call (when `FEE_SAFETY_ENABLE == STD_ON`). A mismatch shall be reported via `FEE_E_RAM_INTEGRITY` (DET) and `FEE_E_HARDWARE_ERROR` (DEM). | B |
| SR-005 | DEM Production Error Reporting | The Fee module shall report `FEE_E_HARDWARE_ERROR` to the Diagnostic Event Manager for all flash hardware failures (MemAcc job failures) and data integrity violations. | B |
| SR-006 | No Dynamic Memory Allocation | The Fee and MemAcc modules shall not use dynamic memory allocation (`malloc`, `calloc`, `free`). All buffers shall be statically allocated at compile time. | B |
| SR-007 | Deterministic WCET | Each invocation of `Fee_MainFunction()` shall perform at most one MemAcc async operation and one safety check, ensuring bounded worst-case execution time per call. | B |
| SR-008 | Exclusive Area Protection | All concurrent access to module state variables during job acceptance shall be protected by SchM exclusive areas (`SchM_Enter_Fee_FEE_EXCLUSIVE_AREA_0` / `SchM_Exit_Fee_FEE_EXCLUSIVE_AREA_0`). | B |
| SR-009 | Read-Back Verification | When `FEE_READ_BACK_VERIFICATION == STD_ON`, the Fee module shall read back written data and compare byte-by-byte with the original source buffer. A mismatch shall result in `MEMIF_JOB_FAILED` and `FEE_E_HARDWARE_ERROR` DEM report. | B |
| SR-010 | Sequence Counter Monotonicity | The Fee module shall maintain a monotonically increasing `SequenceCounter` per block. During initialization scan, only the block instance with the highest `SequenceCounter` shall be accepted as current. | B |
| SR-011 | Wear Leveling | During garbage collection, the Fee module shall select the target sector with the lowest `EraseCount` among `FEE_SECTOR_ERASED` sectors, distributing erase cycles evenly across sectors. | B |
| SR-012 | Defective Sector Handling | If a sector header read fails during initialization (MemAcc failure), the sector shall be marked as `FEE_SECTOR_DEFECTIVE` and excluded from normal operations. | B |
| SR-013 | Safe State Definition | Upon detection of an unrecoverable error, the Fee module shall transition to `FEE_STATE_ERROR`, report `FEE_E_HARDWARE_ERROR` via DEM, and then return to `FEE_STATE_IDLE`. No further flash operations shall be attempted until a new job is explicitly requested. | B |
| SR-014 | Flow Control Monitoring | The Fee module shall provide a flow counter mechanism (`Fee_Safety_FlowCheck`) that verifies execution follows the expected sequence. A flow counter mismatch shall be reported via DET runtime error `FEE_E_RAM_INTEGRITY` and DEM `FEE_E_HARDWARE_ERROR`. | B |
| SR-015 | ECC Error Propagation | The MemAcc module shall propagate ECC errors from Mem_DFLS: `MEM_DFLS_JOB_ECC_CORRECTED` → `MEMACC_JOB_ECC_CORRECTED`, `MEM_DFLS_JOB_ECC_UNCORRECTED` → `MEMACC_JOB_ECC_UNCORRECTED` (with DEM report). The Fee module shall be able to detect and handle these via MemAcc job results. | B |
