# Timing Analysis

**AUTOSAR R24-11 Fee + MemAcc**

---

## 1  MainFunction WCET Model

Each `Fee_MainFunction()` call performs exactly **one state transition** in the
Fee state machine, plus one `Fee_Safety_CyclicCheck()` call. The WCET is
dominated by the single MemAcc operation dispatched per cycle.

### 1.1  Per-Cycle Breakdown

| Component | Description | Estimated WCET |
|-----------|-------------|---------------|
| `Fee_StateMachine_Process` | One switch-case branch | 2–5 µs |
| MemAcc API call (`MemAcc_Read`/`Write`/`Erase`/`GetJobResult`) | One async dispatch or poll | 3–10 µs |
| `Fee_Safety_CyclicCheck` | CRC-16 over `BlockInfoTable` + `SectorInfo` | Size-dependent (see below) |
| `Fee_Safety_ComputeRamCrc` | CRC-16 bit-by-bit, `sizeof(Fee_BlockInfoType)*8 + sizeof(Fee_SectorInfoType)*4` bytes | ~50–100 µs |
| **Total per MainFunction** | | **~60–120 µs** |

### 1.2  RAM CRC Computation Size

With default configuration:
- `Fee_BlockInfoType` = ~24 bytes × `FEE_NUMBER_OF_BLOCKS` (8) = 192 bytes
- `Fee_SectorInfoType` = ~20 bytes × `FEE_NUMBER_OF_SECTORS` (4) = 80 bytes
- Total CRC input: ~272 bytes
- CRC-16 bit-by-bit: 16 operations/byte → ~4352 iterations

---

## 2  Write Operation Timing

A complete write operation spans multiple `Fee_MainFunction()` cycles:

| Phase | States | MainFunction Calls | Description |
|-------|--------|-------------------|-------------|
| 1. Allocate | `WRITE_ALLOC` | 1 | `Fee_Sector_AllocateBlock` (CPU-only) |
| 2. Write Header | `WRITE_HEADER` → `WRITE_HEADER_WAIT` | 2 | CRC compute + `MemAcc_Write(32 bytes)` + poll |
| 3. Write Data | `WRITE_DATA` → `WRITE_DATA_WAIT` | 2 | `PrepareWriteBuffer` + `MemAcc_Write(padded)` + poll |
| 4. Write Valid | `WRITE_VALID` → `WRITE_VALID_WAIT` | 2 | Set marker + `MemAcc_Write(32 bytes)` + poll |
| 5a. Verify (opt) | `WRITE_VERIFY_READ` → `WAIT` → `COMPARE` | 3 | Read-back + compare (if `FEE_READ_BACK_VERIFICATION == STD_ON`) |
| **Total (no verify)** | | **7** | |
| **Total (with verify)** | | **10** | |

### 2.1  Wall-Clock Time

With `FEE_MAIN_FUNCTION_PERIOD = 5 ms`:

| Configuration | Cycles | Wall-Clock |
|---|---|---|
| Write without read-back verification | 7 | 35 ms |
| Write with read-back verification | 10 | 50 ms |

**Note:** Additional cycles may occur if `MemAcc_GetJobResult` returns
`MEMACC_JOB_PENDING` (flash write not yet complete). TC3xx DFLASH write time
is typically ~120 µs per 32-byte page, so within a single 5 ms period.

---

## 3  Read Operation Timing

| Phase | States | MainFunction Calls |
|-------|--------|-------------------|
| 1. Start | `READ_START` | 1 |
| 2. Wait | `READ_WAIT` | 1 (or more if PENDING) |
| 3. CRC verify | `READ_VERIFY_CRC` | 1 |
| **Total** | | **3** (15 ms) |

---

## 4  Garbage Collection Timing

GC duration depends on the number of valid blocks in the source sector.

### 4.1  Per-Block GC Cost

| Phase | States | MainFunction Calls |
|-------|--------|-------------------|
| Select source | `GC_SELECT_SOURCE` | 1 |
| Copy read | `GC_COPY_READ` → `GC_COPY_READ_WAIT` | 2 |
| Copy write header | `GC_COPY_WRITE_HEADER` → `GC_COPY_WRITE_HEADER_WAIT` | 2 |
| Copy write data | `GC_COPY_WRITE_DATA` → `GC_COPY_WRITE_DATA_WAIT` | 2 |
| Copy write valid | `GC_COPY_WRITE_VALID` → `GC_COPY_WRITE_VALID_WAIT` | 2 |
| **Per block total** | | **9** |

### 4.2  Erase Phase

| Phase | States | MainFunction Calls |
|-------|--------|-------------------|
| Select (last, no block found) | `GC_SELECT_SOURCE` | 1 |
| Erase source | `GC_ERASE_SOURCE` → `GC_ERASE_WAIT` | 2 |
| Complete | `GC_COMPLETE_STATE` | 1 |
| **Erase phase total** | | **4** |

### 4.3  Total GC Duration Formula

```
GC_cycles = (9 × N_valid_blocks) + 4

GC_wall_clock = GC_cycles × FEE_MAIN_FUNCTION_PERIOD
```

**Example:** 8 valid blocks, 5 ms period:
```
GC_cycles = (9 × 8) + 4 = 76
GC_wall_clock = 76 × 5 ms = 380 ms
```

### 4.4  Suspension Impact

When an immediate job preempts GC:
1. GC is suspended (returns `FEE_GC_IN_PROGRESS` without advancing).
2. Immediate job takes 7–10 cycles (write) or 3 cycles (read).
3. GC resumes from saved state.

Total GC wall-clock increases by the duration of the immediate job.

---

## 5  Initialization Timing

| Phase | Cycles per sector/record |
|-------|--------------------------|
| Sector header read + parse | 3 per sector |
| Record scan (per record) | 2 per record |

```
Init_cycles = (3 × FEE_NUMBER_OF_SECTORS) + (2 × N_records_in_active_sector) + 1

Init_wall_clock = Init_cycles × FEE_MAIN_FUNCTION_PERIOD
```

**Example:** 4 sectors, 8 records in active sector:
```
Init_cycles = (3 × 4) + (2 × 8) + 1 = 29
Init_wall_clock = 29 × 5 ms = 145 ms
```

---

## 6  Timing Diagram — Write Operation

```
Time (ms) →  0    5    10   15   20   25   30   35   40   45   50
             │    │    │    │    │    │    │    │    │    │    │
MainFn #1    ├─── WRITE_ALLOC (allocate space) ──►
MainFn #2    │    ├─── WRITE_HEADER (CRC + build + MemAcc_Write) ──►
MainFn #3    │    │    ├─── WRITE_HEADER_WAIT (poll MemAcc) ──►
MainFn #4    │    │    │    ├─── WRITE_DATA (pad + MemAcc_Write) ──►
MainFn #5    │    │    │    │    ├─── WRITE_DATA_WAIT (poll) ──►
MainFn #6    │    │    │    │    │    ├─── WRITE_VALID (marker 0x55) ──►
MainFn #7    │    │    │    │    │    │    ├─── WRITE_VALID_WAIT (poll) ──►
             │    │    │    │    │    │    │    │
             │    │    │    │    │    │    │    ├── [No verify] CompleteJob(OK)
             │    │    │    │    │    │    │    │
MainFn #8    │    │    │    │    │    │    │    ├─── WRITE_VERIFY_READ ──►
MainFn #9    │    │    │    │    │    │    │    │    ├── WRITE_VERIFY_WAIT ──►
MainFn #10   │    │    │    │    │    │    │    │    │   ├── VERIFY_COMPARE ──►
             │    │    │    │    │    │    │    │    │   │   CompleteJob(OK)
             │    │    │    │    │    │    │    │    │   │
             ▼    ▼    ▼    ▼    ▼    ▼    ▼    ▼    ▼   ▼
```
