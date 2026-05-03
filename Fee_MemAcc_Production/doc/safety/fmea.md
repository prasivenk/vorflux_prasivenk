# Failure Mode and Effects Analysis (FMEA)

**AUTOSAR R24-11 Fee + MemAcc | ASIL-B**

---

| FM ID | Failure Mode | Effect | Severity | Detection Method | Detection Coverage | Occurrence | RPN |
|-------|-------------|--------|----------|------------------|--------------------|------------|-----|
| FM-001 | Flash write failure (Mem_DFLS_Write returns E_NOT_OK or MEM_DFLS_JOB_FAILED) | Block data not written; NvM data loss for current write request | High | MemAcc polls `Mem_DFLS_GetJobResult`; `MEMACC_JOB_FAILED` propagated to Fee; DEM `FEE_E_HARDWARE_ERROR` reported; `MEMIF_JOB_FAILED` returned to NvM | High | Low | Low |
| FM-002 | Flash read ECC error (MEM_DFLS_JOB_ECC_UNCORRECTED) | Block data corrupted; read returns invalid data | High | ECC hardware in TC3xx DMU; propagated as `MEMACC_JOB_ECC_UNCORRECTED`; DEM reported | High | Very Low | Low |
| FM-003 | Flash read correctable ECC error (MEM_DFLS_JOB_ECC_CORRECTED) | Data corrected by hardware; potential early wear indicator | Medium | Propagated as `MEMACC_JOB_ECC_CORRECTED`; logged for monitoring | High | Low | Low |
| FM-004 | RAM corruption of Fee_BlockInfoTable | Incorrect block addresses/CRC/status; wrong data served to NvM | High | `Fee_Safety_CyclicCheck` detects CRC mismatch; `FEE_E_RAM_INTEGRITY` + DEM | High (cyclic, every 5 ms) | Very Low | Low |
| FM-005 | RAM corruption of Fee_SectorInfo | Incorrect write pointer or sector status; data written to wrong address or over existing data | High | `Fee_Safety_CyclicCheck` covers SectorInfo in RAM CRC | High (cyclic, every 5 ms) | Very Low | Low |
| FM-006 | Power loss during write (before ValidMarker) | Block header written with `ValidMarker = 0x00`; data may be partially written | Medium | Init scan: `ValidMarker == 0x00` → `FEE_BLOCK_INCONSISTENT`; previous valid instance used | High | Low | Low |
| FM-007 | Power loss during write (after ValidMarker) | Block successfully committed; no data loss | None | N/A — write is complete | N/A | Low | None |
| FM-008 | Power loss during GC (before valid marker on target) | New copy in target incomplete; original in source still valid | Medium | Init scan: original has higher seq counter or target has `ValidMarker != 0x55` | High | Low | Low |
| FM-009 | Power loss during GC erase | Source sector partially erased; headers corrupted | Medium | Init scan: corrupt/blank headers → `FEE_SECTOR_ERASED`; copied blocks in target sector are valid | Medium | Low | Low |
| FM-010 | CRC error on read (computed CRC ≠ stored DataCrc) | Data corruption detected; `MEMIF_BLOCK_INCONSISTENT` returned | High | `Fee_Crc_CalculateBlock` in `FEE_STATE_READ_VERIFY_CRC`; DEM reported | High | Very Low | Low |
| FM-011 | Sector header corruption (bad magic or header CRC) | Sector treated as `FEE_SECTOR_ERASED` during init; blocks in sector lost | High | `Fee_Sector_ParseSectorHeader` validates magic `0xFEE0FEE0` and header CRC | Medium (detection only, no recovery of blocks) | Very Low | Medium |
| FM-012 | Block header corruption (header CRC mismatch) | Block record skipped during init scan; advance cursor by `FEE_BLOCK_HEADER_SIZE` | Medium | `Fee_Sector_ParseBlockHeader` rejects corrupt headers | High | Very Low | Low |
| FM-013 | Write pointer corruption (RAM) | Blocks written to wrong flash address; potential overlap | High | Covered by `Fee_Safety_CyclicCheck` (SectorInfo includes WritePointer) | High | Very Low | Low |
| FM-014 | Sequence number overflow (uint16 wrap at 65535) | Newer block instance may have lower sequence counter than older; wrong instance selected during init | Medium | No runtime detection; design constraint: uint16 provides 65535 writes per block | Low (design limit) | Very Low | Low |
| FM-015 | Erase count overflow (uint16 wrap at 65535) | Wear leveling may select wrong target sector | Low | No runtime detection; design constraint: 65535 erase cycles far exceeds TC3xx DFLASH rated endurance (~100K cycles per physical sector) | Low | Very Low | Low |
| FM-016 | GC failure (target sector full during block copy) | GC cannot complete; source sector cannot be reclaimed | High | `Fee_Sector_AllocateBlock` returns 0; GC transitions to `FEE_GC_IDLE` with `FEE_GC_ERROR`; DEM reported | High | Low | Low |
| FM-017 | MemAcc timeout (Mem_DFLS_GetJobResult stays PENDING indefinitely) | Fee state machine stuck in wait state; module appears permanently busy | High | No explicit timeout in current implementation; watchdog at OS level provides backup | Low (design gap) | Very Low | Medium |
| FM-018 | Stack overflow | Undefined behavior; potential RAM corruption | Critical | No runtime detection in Fee/MemAcc; relies on OS stack monitoring and MPU | Low (all buffers static) | Very Low | Low |
| FM-019 | Flow control violation (Fee_Safety_FlowCheck mismatch) | Unexpected execution sequence detected | High | `Fee_Safety_FlowCheck` compares counter to expected value; DET + DEM reported | High | Very Low | Low |
