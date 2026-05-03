# AUTOSAR Fee Module

## Overview

Implementation of the AUTOSAR R24-11 Fee (Flash EEPROM Emulation) module. The Fee module provides NvM with block-based non-volatile storage over flash memory, handling wear leveling, garbage collection, and data integrity via CRC-16.

Per AUTOSAR R24-11, Fee interfaces with MemAcc (Memory Access) as its lower layer. This project includes a RAM-backed MemAcc test double for host-based testing.

## Project Structure

```
Fee/
├── CMakeLists.txt                    # Build system
├── include/
│   ├── Fee.h                         # Public API declarations
│   ├── Fee_Types.h                   # Type definitions
│   ├── Fee_Cfg.h                     # Configuration parameters
│   ├── Fee_Internal.h                # Internal job engine declarations
│   ├── Fee_Sector.h                  # Sector management declarations
│   ├── Std_Types.h                   # AUTOSAR standard types stub
│   ├── MemIf_Types.h                 # MemIf status/job result types
│   ├── Det.h                         # DET error tracer stub
│   └── MemAcc.h                      # MemAcc stub declarations (R24-11 test double)
├── src/
│   ├── Fee.c                         # Public API + state machine
│   ├── Fee_Internal.c                # Job processing engine
│   ├── Fee_Sector.c                  # Sector management + GC + wear leveling
│   ├── Fee_Cfg.c                     # Block & sector configuration data
│   ├── Det.c                         # DET stub implementation
│   └── MemAcc.c                      # RAM-backed MemAcc stub
├── test/
│   ├── unity/                        # Unity v2.6.0 test framework (embedded)
│   │   ├── unity.h
│   │   ├── unity.c
│   │   └── unity_internals.h
│   ├── test_Fee.c                    # API-level + integration tests
│   ├── test_Fee_Internal.c           # Job engine tests
│   └── test_Fee_Sector.c            # Sector management tests
└── README.md                         # This file
```

## AUTOSAR R24-11 Compliance Scope

This implementation covers the Fee module as specified in AUTOSAR R24-11:
- **Fee module**: Flash EEPROM Emulation with block-based storage
- **MemAcc interface**: Uses MemAcc (Memory Access) as lower layer per R24-11
- **MemAcc test double**: RAM-backed stub simulating flash behavior (bit-clearing semantics, sector erase, blank check) for host-based testing
- **DET integration**: Development Error Tracer stub for error reporting

## Architecture

- **Fee Layer**: Provides NvM with read/write/invalidate/erase operations on logical blocks
- **Fee_Internal**: Job processing engine managing asynchronous operations via a main function state machine
- **Fee_Sector**: Sector management handling flash layout, block headers with CRC-16, two-phase commit, wear leveling, and garbage collection
- **MemAcc**: Lower-layer flash driver interface (RAM-backed test double provided)
- **Det**: Development Error Tracer for AUTOSAR-compliant error reporting

## Build Instructions

```bash
cd Fee
mkdir -p build && cd build
cmake ..
make
```

## Test Instructions

```bash
cd Fee/build
ctest --output-on-failure
```

## Key Design Decisions

- **Flash Semantics**: The MemAcc stub enforces flash bit-clearing semantics — writes can only clear bits (1→0), never set them (0→1). Only sector erase can reset bits to 1.
- **Two-Phase Commit**: Block writes use a two-phase commit for power-loss safety — data written first, valid marker last.
- **CRC-16**: Block headers include CRC-16 for data integrity verification.
- **Crash-Consistent GC**: Garbage collection uses sector sequence numbers for crash consistency.
