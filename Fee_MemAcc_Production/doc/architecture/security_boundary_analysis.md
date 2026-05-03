# Security Boundary Analysis — ISO 21434

**AUTOSAR R24-11 Fee + MemAcc | ISO 21434 Threat Analysis and Risk Assessment**

---

## 1  Scope

This analysis covers the Fee and MemAcc modules and their interfaces to NvM
(upper), Mem_DFLS (lower), and TC3xx DFLASH hardware. The analysis follows
ISO 21434 TARA methodology.

---

## 2  Trust Boundaries

```
┌─ Trust Boundary 1: Application Partition ─────────────────────┐
│  NvM, SWCs (untrusted callers)                                │
├───────────────────────────────────────────────────────────────┤
│                   MemIf API interface                          │
├─ Trust Boundary 2: ASIL-B BSW Partition ──────────────────────┤
│  Fee (Fee.c, Fee_StateMachine.c, Fee_Sector.c,                │
│       Fee_GarbageCollect.c, Fee_Crc.c, Fee_Safety.c)          │
│  MemAcc (MemAcc.c, MemAcc_JobProcessing.c,                   │
│          MemAcc_AddressArea.c)                                 │
├───────────────────────────────────────────────────────────────┤
│                 MemAcc → Mem_DFLS interface                    │
├─ Trust Boundary 3: MCAL Partition ────────────────────────────┤
│  Mem_DFLS (MCAL vendor driver)                                 │
├───────────────────────────────────────────────────────────────┤
│            Physical DFLASH interface                            │
├─ Trust Boundary 4: Hardware ──────────────────────────────────┤
│  TC3xx DFLASH cells, ECC logic, DMU registers                  │
└───────────────────────────────────────────────────────────────┘
```

### Boundary descriptions

| Boundary | Assets Protected | Control Mechanism |
|----------|------------------|-------------------|
| TB1 → TB2 | Fee/MemAcc internal state | DET validation of all API parameters; SchM exclusive areas; `Fee_Internal_FindBlockIndex` rejects unknown block numbers |
| TB2 → TB3 | Mem_DFLS interface integrity | `MemAcc_Internal_ValidateAddress` bounds-checks every address+length; `MemAcc_Internal_TranslateAddress` maps logical→physical |
| TB3 → TB4 | Flash cell integrity | ECC hardware (TC3xx DMU); Mem_DFLS propagates `MEM_DFLS_JOB_ECC_CORRECTED` / `MEM_DFLS_JOB_ECC_UNCORRECTED` |

---

## 3  Threat Model

### 3.1  STRIDE Analysis

| ID | Threat Category | Threat | Attack Surface | Risk | Mitigation | Residual Risk |
|----|----------------|--------|----------------|------|------------|---------------|
| T-01 | Tampering | Unauthorized modification of stored NV data | Flash cells, DMA, bus snooping | High | CRC-16 CCITT data integrity verification on every read (`Fee_Crc_CalculateBlock`); header CRC validated during sector scan (`Fee_Sector_ParseBlockHeader`) | CRC-16 has collision probability 2⁻¹⁶ per block; acceptable for ASIL-B |
| T-02 | Information Disclosure | Unauthorized read of NV data | Flash dump, debug interfaces | Medium | `<PLACEHOLDER_REQUIRED>` — Application-level encryption required. Fee does not provide confidentiality. Recommend AES-128/256 at NvM/SWC layer. | **Open** — Not mitigated at Fee/MemAcc layer |
| T-03 | Denial of Service | Flash wear-out via excessive writes | Rogue NvM write requests | Medium | Wear leveling in GC: target sector selected by lowest `EraseCount`; `Fee_BlockConfigType.NumberOfWriteCycles` limits per-block writes; GC restart threshold `FEE_GC_RESTART_THRESHOLD` (80%) | Residual: rapid writes under threshold cannot be prevented by Fee alone |
| T-04 | Replay | Replay of stale NV block data | Flash content manipulation | Medium | Monotonically increasing `SequenceCounter` per block (`Fee_BlockSequenceCounters[]`); during init scan, only the highest sequence counter instance is accepted | SequenceCounter is uint16 — wraps at 65535; acceptable for typical vehicle lifetime |
| T-05 | Tampering | RAM corruption of BlockInfoTable/SectorInfo | Bit flips, buffer overflows | High | Cyclic RAM CRC check (`Fee_Safety_CyclicCheck`); runtime error `FEE_E_RAM_INTEGRITY` (0x10) reported via DET and DEM | Detection only — no automatic recovery |
| T-06 | Tampering | Sector header corruption | Flash cell failure | High | Magic word `FEE_SECTOR_MAGIC` (0xFEE0FEE0) + header CRC; corrupt headers fall through to `FEE_SECTOR_ERASED` during init | Sector data may be lost if header is corrupt |
| T-07 | Elevation of Privilege | Execution of Fee APIs from unauthorized core/partition | Multi-core system | Low | Fee designed for single-core access; SchM exclusive areas prevent data races; MPU partitioning enforced by OS (`<PLACEHOLDER_REQUIRED>` OS configuration) | Requires correct MPU setup |

---

## 4  Asset Inventory

| Asset | Classification | Location |
|-------|---------------|----------|
| NV block data | Safety-critical (ASIL-B) | DFLASH sectors |
| Block headers (CRC, SequenceCounter, ValidMarker) | Safety-critical | DFLASH sectors |
| Sector headers (Magic, SequenceNumber, EraseCount) | Safety-critical | DFLASH sector base addresses |
| `Fee_BlockInfoTable[]` | Safety-critical (RAM) | ASIL-B RAM partition |
| `Fee_SectorInfo[]` | Safety-critical (RAM) | ASIL-B RAM partition |
| `Fee_Safety_RamCrc` | Safety mechanism | ASIL-B RAM partition |
| Configuration data (`Fee_ConfigPtr`, `MemAcc_ConfigPtr`) | Integrity-critical | Read-only flash / RAM |

---

## 5  Security Controls Summary

| Control | Type | Coverage |
|---------|------|----------|
| CRC-16 CCITT (0x1021, initial 0xFFFF) | Detection | Block data integrity, header integrity, RAM integrity |
| Two-phase commit (write header → write data → write valid marker 0x55) | Prevention | Power-loss during write — incomplete writes detected as `FEE_BLOCK_INCONSISTENT` |
| `FEE_MARKER_VALID` (0x55) / `FEE_MARKER_INVALID` (0xFF) / `FEE_MARKER_ERASED` (0x00) | State tracking | Distinguish valid, invalidated, and incomplete block instances |
| DET parameter validation | Prevention | Reject invalid API calls at module boundary |
| SchM exclusive areas | Prevention | Prevent concurrent modification of shared state |
| `MemAcc_Internal_ValidateAddress` | Prevention | Prevent out-of-bounds flash access |
| `MemAcc_AreaLocked[]` | Prevention | Prevent concurrent flash access to same area |

---

## 6  Recommendations

1. **`<PLACEHOLDER_REQUIRED>`** — Implement application-level encryption (AES-128 or AES-256) for confidential NV data blocks at the SWC or NvM layer.
2. **`<PLACEHOLDER_REQUIRED>`** — Configure TC3xx MPU regions to enforce ASIL-B partition boundaries at OS level.
3. Consider CRC-32 upgrade for safety-critical blocks requiring Hamming distance ≥ 5.
4. Implement sequence counter overflow handling (currently wraps silently at uint16 max).
