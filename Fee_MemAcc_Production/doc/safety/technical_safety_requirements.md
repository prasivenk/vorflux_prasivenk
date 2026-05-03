# Technical Safety Requirements

**AUTOSAR R24-11 Fee + MemAcc | Derived from Safety Requirements**

---

| TSR ID | Derived From | Title | Description |
|--------|-------------|-------|-------------|
| TSR-001 | SR-001 | CRC-16 CCITT Diagnostic Coverage | The CRC-16 CCITT (polynomial 0x1021, initial value 0xFFFF) shall provide a minimum Hamming distance of 4 for messages up to 32,751 bits (4093 bytes), achieving a residual error probability of ≤ 2⁻¹⁶ per block read. Implementation: `Fee_Crc_CalculateBlock` in `src/Fee_Crc.c`. |
| TSR-002 | SR-002 | ValidMarker Transition Constraints | The `ValidMarker` byte (block header offset 30) shall follow the transition: `FEE_MARKER_ERASED` (0x00) → `FEE_MARKER_VALID` (0x55) → `FEE_MARKER_INVALID` (0xFF). No other transition shall occur. The TC3xx DFLASH erased state is 0x00; writing 0x55 is a 0→1 transition, and writing 0xFF is a further 0→1 transition, both valid without erase. |
| TSR-003 | SR-004 | RAM CRC Check Period | The RAM CRC integrity check (`Fee_Safety_CyclicCheck`) shall execute once per `Fee_MainFunction()` call, i.e., every `FEE_MAIN_FUNCTION_PERIOD` (5 ms). The CRC shall cover `sizeof(Fee_BlockInfoType) × FEE_NUMBER_OF_BLOCKS` + `sizeof(Fee_SectorInfoType) × FEE_NUMBER_OF_SECTORS` bytes. |
| TSR-004 | SR-004 | RAM CRC Update After Legitimate Modification | After any legitimate modification of `Fee_BlockInfoTable[]` or `Fee_SectorInfo[]` (write completion, invalidation, GC block copy, GC erase), `Fee_Safety_UpdateRamCrc()` shall be called to recompute the stored CRC before the next `Fee_Safety_CyclicCheck`. |
| TSR-005 | SR-002 | Block Header CRC Coverage | Each 32-byte block header shall contain a CRC-16 computed over bytes 0–29, stored at bytes 30–31 (with byte 30 sharing the ValidMarker role). `Fee_Sector_ParseBlockHeader` shall reject headers where the recomputed CRC does not match the stored value. |
| TSR-006 | SR-003 | Incomplete Write Detection | During initialization sector scan, a block header with `ValidMarker == FEE_MARKER_ERASED` (0x00) shall be classified as `FEE_BLOCK_INCONSISTENT`, indicating an interrupted write. |
| TSR-007 | SR-003 | GC Crash Recovery | If GC is interrupted before the valid marker is written to the target sector, the original block in the source sector shall remain the authoritative copy (higher or equal sequence counter). Partially erased source sectors shall be treated as `FEE_SECTOR_ERASED`. |
| TSR-008 | SR-006 | Static Buffer Sizing | `Fee_Sector_HeaderBuffer` shall be exactly `FEE_BLOCK_HEADER_SIZE` (32) bytes. `Fee_Sector_WriteBuffer` shall be exactly `FEE_MAX_BLOCK_SIZE` (512) bytes. `Fee_GcCopyBuffer` shall be exactly `FEE_MAX_BLOCK_SIZE` (512) bytes. `MemAcc_CompareBuffer` shall be exactly `MEMACC_COMPARE_BUFFER_SIZE` (32) bytes. |
| TSR-009 | SR-007 | One MemAcc Operation Per MainFunction | `Fee_StateMachine_Process` shall issue at most one `MemAcc_Read`, `MemAcc_Write`, or `MemAcc_Erase` call per invocation. In wait states, it shall only call `MemAcc_GetJobResult`. |
| TSR-010 | SR-008 | Exclusive Area Scope | `SchM_Enter_Fee_FEE_EXCLUSIVE_AREA_0` shall be held only during `Fee_StateMachine_AcceptJob` (reading `Fee_ModuleStatus` and writing `Fee_CurrentJob`, `Fee_ModuleStatus`, `Fee_LastJobResult`, `Fee_InternalState`). |
| TSR-011 | SR-010 | Sequence Counter Overflow | `Fee_BlockSequenceCounters[]` is `uint16`. At 65,535 the counter wraps to 0. For a block written once per power cycle with 10,000 cycles/lifetime, this provides ~6.5 lifetimes of headroom. |
| TSR-012 | SR-011 | Wear Leveling Selection | `Fee_GarbageCollect_Trigger` shall iterate all sectors with `FEE_SECTOR_ERASED` status and select the one with the lowest `EraseCount` value as the GC target. |
| TSR-013 | SR-009 | Read-Back Verification Byte Granularity | When `FEE_READ_BACK_VERIFICATION == STD_ON`, the verification compare in `FEE_STATE_WRITE_VERIFY_COMPARE` shall compare every byte of original data against read-back data for `BlockSize` bytes. |
| TSR-014 | SR-014 | Flow Counter Expected Sequence | `Fee_Safety_FlowCheck(Expected)` shall compare `Fee_Safety_FlowCounter` against `Expected` and increment the counter. `Fee_Safety_ResetFlowCounter` shall set the counter to 0. |
| TSR-015 | SR-015 | ECC Propagation Mapping | MemAcc shall map `MEM_DFLS_JOB_ECC_CORRECTED` (value 3) to `MEMACC_JOB_ECC_CORRECTED` (value 4) and `MEM_DFLS_JOB_ECC_UNCORRECTED` (value 4) to `MEMACC_JOB_ECC_UNCORRECTED` (value 5). `<PLACEHOLDER_REQUIRED>` TC3xx-specific ECC correction capability depends on DMU configuration. |
