# Diagnostic Coverage Analysis

**AUTOSAR R24-11 Fee + MemAcc | ASIL-B**

---

## 1  CRC-16 CCITT Coverage

| Attribute | Value |
|---|---|
| Polynomial | 0x1021 (CRC-16 CCITT) |
| Initial Value | 0xFFFF |
| Implementation | Bit-by-bit (no lookup table), `Fee_Crc_Calculate` in `src/Fee_Crc.c` |
| Hamming Distance | 4 (for messages up to 32,751 bits / 4093 bytes) |
| Undetected Error Probability | ≤ 2⁻¹⁶ ≈ 1.5 × 10⁻⁵ per block read |
| Max Block Size | `FEE_MAX_BLOCK_SIZE` = 512 bytes (well within HD=4 range) |
| Application | Block data integrity (read verification, GC copy verification) |
| Header Protection | Block headers: CRC-16 over 30 header bytes; Sector headers: CRC-16 over 30 header bytes |

### 1.1  Coverage Assessment

| Error Pattern | Detected? | Notes |
|---|---|---|
| Single-bit error | Yes | HD ≥ 2 |
| Double-bit error | Yes | HD ≥ 3 |
| Triple-bit error | Yes | HD = 4 |
| All odd-weight errors ≤ 3 bits | Yes | |
| Burst errors ≤ 16 bits | Yes | CRC-16 property |
| Random multi-bit errors (≥ 4 bits) | Probabilistic | Undetected probability ≤ 2⁻¹⁶ |

---

## 2  Two-Phase Commit Coverage

| Attribute | Value |
|---|---|
| Mechanism | `ValidMarker` byte at block header offset 30 |
| Phase 1 | Header written with `ValidMarker = FEE_MARKER_ERASED` (0x00); data written |
| Phase 2 | `ValidMarker` overwritten to `FEE_MARKER_VALID` (0x55) |
| Failure Point | Between Phase 1 and Phase 2 |
| Detection | Init scan: `ValidMarker == 0x00` → `FEE_BLOCK_INCONSISTENT` |
| Coverage | 100% detection of interrupted writes (assuming flash write is atomic at page granularity) |

### 2.1  TC3xx Considerations

TC3xx DFLASH guarantees atomic page writes (32 bytes). The valid marker is
within the same 32-byte page as the rest of the header. The second write
(Phase 2) rewrites the entire 32-byte header page with the valid marker set.

| Failure Scenario | Block State After Recovery |
|---|---|
| Power loss before any write | `FEE_BLOCK_NOT_FOUND` (no header) |
| Power loss after header write, before data | `FEE_BLOCK_INCONSISTENT` (header exists, no valid marker) |
| Power loss after data write, before valid marker | `FEE_BLOCK_INCONSISTENT` |
| Power loss during valid marker write | Either `0x00` (inconsistent) or `0x55` (valid) — TC3xx atomic |
| Power loss after valid marker write | `FEE_BLOCK_VALID` (success) |

---

## 3  RAM CRC Coverage

| Attribute | Value |
|---|---|
| Mechanism | `Fee_Safety_CyclicCheck` in `src/Fee_Safety.c` |
| Algorithm | Same CRC-16 CCITT as data CRC |
| Protected Data | `Fee_BlockInfoTable[FEE_NUMBER_OF_BLOCKS]` + `Fee_SectorInfo[FEE_NUMBER_OF_SECTORS]` |
| Protected Size | ~272 bytes (with default config: 8 blocks × ~24 bytes + 4 sectors × ~20 bytes) |
| Check Period | Every `Fee_MainFunction()` call = every `FEE_MAIN_FUNCTION_PERIOD` (5 ms) |
| Update Points | After every legitimate modification: `Fee_Safety_UpdateRamCrc()` |
| Detection Latency | ≤ 5 ms (one MainFunction period) |
| Error Probability | ≤ 2⁻¹⁶ per check |

### 3.1  Coverage Limitations

- **Not protected:** `Fee_CurrentJob`, `Fee_InternalState`, `Fee_ModuleStatus`,
  `Fee_LastJobResult`, `Fee_BlockSequenceCounters[]` — these are transient
  operational state, not persistent block metadata.
- **Not protected:** MemAcc module state variables (separate safety boundary).

---

## 4  ECC Coverage

| Attribute | Value |
|---|---|
| Mechanism | TC3xx DMU hardware ECC on DFLASH |
| Detection | Single-bit error correction, double-bit error detection (SECDED) |
| Propagation | `Mem_DFLS_GetJobResult` → MemAcc → Fee |
| Correctable (`MEM_DFLS_JOB_ECC_CORRECTED`) | Data delivered corrected; `MEMACC_JOB_ECC_CORRECTED` |
| Uncorrectable (`MEM_DFLS_JOB_ECC_UNCORRECTED`) | Data invalid; `MEMACC_JOB_ECC_UNCORRECTED`; DEM reported |

`<PLACEHOLDER_REQUIRED>` TC3xx-specific ECC capabilities:
- Number of ECC bits per page: depends on DFLASH variant
- Correctable errors per page: typically 1 bit
- Detectable errors per page: typically 2 bits
- ECC granularity: 8 bytes (64-bit ECC word)
- Values must be verified against specific TC3xx derivative datasheet

---

## 5  Overall Diagnostic Coverage Summary

| Safety Mechanism | Type (ISO 26262) | Diagnostic Coverage | Fault Model |
|---|---|---|---|
| CRC-16 CCITT data integrity | Information redundancy | High (99.998%) | Flash data corruption |
| CRC-16 CCITT header integrity | Information redundancy | High (99.998%) | Header corruption |
| Two-phase commit (ValidMarker) | Temporal redundancy | Very High (~100%) | Power-loss during write |
| RAM CRC cyclic check | Information redundancy | High (99.998%) | RAM bit flip (BlockInfoTable, SectorInfo) |
| Flow counter monitoring | Program flow monitoring | High | Unexpected execution sequence |
| DET parameter validation | Input validation | Very High | Invalid API parameters |
| MemAcc address bounds check | Input validation | Very High | Out-of-bounds flash access |
| ECC (hardware) | Hardware redundancy | `<PLACEHOLDER_REQUIRED>` | Flash cell degradation |
| Read-back verification | Comparison | Very High (~100%) | Flash write failure (silent) |
| Sequence counter monotonicity | Temporal ordering | High | Stale data replay |
