# AUTOSAR R24-11 Fee + MemAcc Production Package

Production-grade AUTOSAR Classic Platform R24-11 implementation of the **Fee** (Flash EEPROM Emulation) and **MemAcc** (Memory Access) modules targeting **Infineon AURIX TC3xx** DFLASH.

## Compliance

| Standard | Level |
|----------|-------|
| AUTOSAR Classic Platform | R24-11 |
| MISRA C:2012 | Compliant (analysis-ready) |
| ASPICE | 3.1 compatible |
| ISO 26262 | ASIL-B capable |
| ISO 21434 | Cybersecurity-aware |

## Architecture

```
NvM  -->  MemIf  -->  Fee  -->  MemAcc  -->  Mem_DFLS  -->  TC3xx DFLASH
```

- **Fee**: Flash EEPROM emulation with fully asynchronous state machine (28 states), interruptible garbage collection (13 GC states), CRC-16 data integrity, two-phase commit write protocol, and power-loss recovery.
- **MemAcc**: Memory access abstraction with 2 address areas (Fee partition + bootloader partition), async job processing, and hardware driver abstraction.

### Key Design Decisions

- **TC3xx DFLASH erased state = `0x00`** (not `0xFF` like NOR flash). Marker chain: `0x00` (erased) -> `0x55` (valid) -> `0xFF` (invalid). All transitions are monotonic bit-setting.
- **32-byte write granularity** (`FEE_VIRTUAL_PAGE_SIZE = 32u`). Block headers padded to 32 bytes. Data padded via static staging buffer.
- **HeaderCRC excludes mutable ValidMarker** so CRC remains valid across marker transitions.
- **Single-job acceptance model**: One external job at a time. Immediate-data writes preempt GC.
- **Polling mode** (`FEE_POLLING_MODE = STD_ON`): Fee polls `MemAcc_GetJobResult()` — no callbacks.

## File Structure

```
Fee_MemAcc_Production/
├── include/                    # Public and internal headers (26 files)
│   ├── Platform_Types.h        # TC3xx platform types
│   ├── Std_Types.h             # AUTOSAR standard types
│   ├── Compiler.h              # TASKING compiler abstraction
│   ├── Compiler_Cfg.h          # Module memory class definitions
│   ├── MemIf_Types.h           # Memory interface types
│   ├── Det.h                   # Development Error Tracer
│   ├── Dem.h                   # Diagnostic Event Manager
│   ├── Mem_DFLS.h              # TC3xx DFLASH driver interface
│   ├── SchM_Fee.h              # Schedule Manager (Fee)
│   ├── SchM_MemAcc.h           # Schedule Manager (MemAcc)
│   ├── Fee_MemMap.h            # Fee memory mapping sections
│   ├── MemAcc_MemMap.h         # MemAcc memory mapping sections
│   ├── Fee_Version.h           # Version information
│   ├── Fee_Types.h             # Fee type definitions
│   ├── Fee_Cfg.h               # Fee configuration
│   ├── Fee_PBcfg.h             # Fee post-build configuration
│   ├── Fee.h                   # Fee public API (11 APIs)
│   ├── Fee_Crc.h               # CRC-16 CCITT interface
│   ├── Fee_Sector.h            # Sector management interface
│   ├── Fee_StateMachine.h      # State machine interface
│   ├── Fee_GarbageCollect.h    # GC interface
│   ├── Fee_Safety.h            # Safety mechanisms interface
│   ├── MemAcc_Types.h          # MemAcc type definitions
│   ├── MemAcc_Cfg.h            # MemAcc configuration
│   ├── MemAcc_PBcfg.h          # MemAcc post-build configuration
│   └── MemAcc.h                # MemAcc public API (16 APIs)
├── src/                        # Production source code (13 files)
│   ├── Fee.c                   # Fee public API implementation
│   ├── Fee_Cfg.c               # Fee block configuration table
│   ├── Fee_PBcfg.c             # Fee post-build configuration
│   ├── Fee_Crc.c               # CRC-16 CCITT (polynomial 0x1021)
│   ├── Fee_Sector.c            # Sector and block header management
│   ├── Fee_StateMachine.c      # Async state machine (28 states)
│   ├── Fee_GarbageCollect.c    # Interruptible GC (13 states)
│   ├── Fee_Safety.c            # RAM CRC, flow monitoring
│   ├── MemAcc.c                # MemAcc public API implementation
│   ├── MemAcc_Cfg.c            # MemAcc address area configuration
│   ├── MemAcc_PBcfg.c          # MemAcc post-build configuration
│   ├── MemAcc_JobProcessing.c  # Async job processing
│   └── MemAcc_AddressArea.c    # Address area management
├── test/
│   ├── unity/                  # Unity test framework
│   ├── stubs/                  # Test stubs (10 files)
│   │   ├── Mem_DFLS_Stub.h/c   # RAM-backed DFLASH (erased=0x00, ECC injection)
│   │   ├── Det_Stub.h/c        # DET error recording
│   │   ├── Dem_Stub.h/c        # DEM event recording
│   │   ├── SchM_Stub.h/c       # SchM enter/exit recording
│   │   └── NvM_Cbk_Stub.h/c   # NvM callback recording
│   ├── unit/                   # Unit tests (11 suites, 269+ tests)
│   │   ├── test_Fee_Api.c              # 35 tests
│   │   ├── test_Fee_Crc.c              # 13 tests
│   │   ├── test_Fee_GarbageCollect.c   # 36 tests
│   │   ├── test_Fee_JobQueue.c         # 22 tests
│   │   ├── test_Fee_PowerLoss.c        # 16 tests
│   │   ├── test_Fee_Safety.c           # 12 tests
│   │   ├── test_Fee_Sector.c           # 38 tests
│   │   ├── test_Fee_StateMachine.c     # 43 tests
│   │   ├── test_MemAcc_AddressArea.c   # 14 tests
│   │   ├── test_MemAcc_Api.c           # 38 tests
│   │   ├── test_MemAcc_JobProcessing.c # 18 tests
│   │   └── tessy/                      # TESSY artifacts
│   └── integration/            # Integration tests (6 suites, 69+ tests)
│       ├── test_Fee_MemAcc_Integration.c       # 21 tests
│       ├── test_Fee_OTA_Scenarios.c            # 10 tests
│       ├── test_Fee_Bootloader_Coexistence.c   #  8 tests
│       ├── test_Fee_ECC_FaultInjection.c       # 12 tests
│       ├── test_Fee_Recovery.c                 # 10 tests
│       ├── test_Fee_MultiCore.c                #  8 tests
│       └── integration_test_plan.md
├── config/                     # ARXML configuration
│   ├── Fee.arxml
│   ├── MemAcc.arxml
│   └── Fee_MemAcc_DefaultConfig.arxml
├── doc/
│   ├── architecture/           # Architecture documents + 10 PlantUML diagrams
│   ├── design/                 # Detailed design documents
│   └── safety/                 # Safety work products (FMEA, traceability, etc.)
├── CMakeLists.txt              # CMake build system
├── .gitignore
└── README.md
```

## Build Instructions

### Prerequisites

- GCC (or any C99-compliant compiler)
- CMake 3.10+

### Build and Test

```bash
cd Fee_MemAcc_Production
mkdir -p build && cd build
cmake ..
make -j$(nproc)
ctest --output-on-failure
```

### Compiler Flags

The build uses `-Wall -Wextra -Werror -pedantic -std=c99 -DFEE_UNIT_TEST`.

The `FEE_UNIT_TEST` macro disables memory mapping pragmas and SchM exclusive area macros for host testing.

## Configuration

### Fee Configuration (`include/Fee_Cfg.h`)

| Parameter | Value | Description |
|-----------|-------|-------------|
| `FEE_VIRTUAL_PAGE_SIZE` | 32 | TC3xx DFLASH write granularity (bytes) |
| `FEE_NUMBER_OF_BLOCKS` | 8 | Configured NV blocks |
| `FEE_NUMBER_OF_SECTORS` | 4 | Flash sectors for Fee |
| `FEE_SECTOR_SIZE` | 16384 | Bytes per sector |
| `FEE_MAX_BLOCK_SIZE` | 512 | Maximum block data size |
| `FEE_GC_RESTART_THRESHOLD` | 80 | GC trigger at 80% sector fill |
| `FEE_POLLING_MODE` | STD_ON | Poll MemAcc (no callbacks) |
| `FEE_SAFETY_ENABLE` | STD_ON | RAM CRC + flow monitoring |
| `FEE_READ_BACK_VERIFICATION` | STD_ON | Verify writes by read-back |

### Block Configuration (`src/Fee_Cfg.c`)

| Block # | Size | Immediate | Description |
|---------|------|-----------|-------------|
| 1 | 32 | No | Small config |
| 2 | 64 | No | Medium config |
| 3 | 128 | No | Large config |
| 4 | 256 | No | Calibration data |
| 5 | 16 | Yes | DTC snapshot (immediate) |
| 6 | 8 | Yes | Error counter (immediate) |
| 7 | 512 | No | Maximum size block |
| 8 | 64 | No | General purpose |

### MemAcc Address Areas (`src/MemAcc_Cfg.c`)

| Area | Start Address | Size | Purpose |
|------|---------------|------|---------|
| 0 | `0xAF000000` | 64 KB | Fee DFLASH partition |
| 1 | `0xAF010000` | 16 KB | Bootloader DFLASH partition |

> **Note**: Base addresses are `<PLACEHOLDER_REQUIRED>` — verify against your specific TC3xx variant datasheet.

## Placeholder Inventory

Items marked `<PLACEHOLDER_REQUIRED>` require project-specific values before production deployment:

| Location | Item | Owner | Resolution |
|----------|------|-------|------------|
| `src/MemAcc_Cfg.c` | TC3xx DFLASH base addresses | HW Integration | Verify against MCU datasheet |
| `include/MemAcc_MemMap.h` | TASKING pragma directives | Build Integration | Add TASKING-specific `#pragma section` |
| `include/Fee_MemMap.h` | TASKING pragma directives | Build Integration | Add TASKING-specific `#pragma section` |
| `src/MemAcc.c` | HW-specific service implementation | HW Integration | Implement TC3xx-specific services |
| `config/Fee_MemAcc_DefaultConfig.arxml` | DFLASH base addresses | HW Integration | Match MCU variant |
| `doc/safety/diagnostic_coverage.md` | TC3xx ECC capabilities | Safety Engineer | TC3xx DMU ECC specification |
| `doc/architecture/security_boundary_analysis.md` | Application-level encryption | Security Engineer | AES-128/256 at NvM/SWC layer |
| `doc/architecture/security_boundary_analysis.md` | MPU region configuration | OS Integration | TC3xx MPU setup for ASIL-B partitions |

## Testing

### Test Summary (354 tests, 0 failures)

**Unit Tests (11 suites, 285 tests):**
- Fee API, CRC, Sector, State Machine, GC, Job Queue, Safety, Power Loss
- MemAcc API, Job Processing, Address Area

**Integration Tests (6 suites, 69 tests):**
- End-to-end Fee+MemAcc, OTA scenarios, Bootloader coexistence
- ECC fault injection, Power loss recovery, Multi-core SchM verification

### Running Specific Test Groups

```bash
# Unit tests only
cd build && ctest -L unit --output-on-failure

# Integration tests only
cd build && ctest -L integration --output-on-failure
```

## Safety Documentation

See `doc/safety/` for:
- Safety requirements (SR-001 through SR-015)
- Technical safety requirements derived from each SR
- FMEA with 15+ failure modes
- Failure handling strategy per failure mode
- Diagnostic coverage analysis
- Safety mechanisms mapping to source code
- Full traceability matrix (System Req -> SW Req -> Design -> Code -> Test -> Safety Req)

## License

Proprietary. All rights reserved.
