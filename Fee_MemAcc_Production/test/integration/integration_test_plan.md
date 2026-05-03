# Fee + MemAcc Integration Test Plan — AUTOSAR R24-11

## 1. Scope

Integration tests verify the end-to-end data path through the full
Fee → MemAcc → Mem_DFLS_Stub stack. These tests exercise the complete
write/read/verify flow as seen by the NvM module.

## 2. Test Suites

### 2.1 End-to-End Integration (`test_Fee_MemAcc_Integration.c`)

21 tests covering:
- Init on blank flash
- Write and read all block sizes (8, 16, 32, 64, 128, 256, 512 bytes)
- Multiple blocks written then read
- Overwrite and verify new data
- Invalidate block and verify BLOCK_INVALID result
- Partial read with offset
- Read non-existent block
- NvM end/error notification verification
- Cancel in-progress write
- Erase immediate block
- Write all 8 configured blocks
- Status transitions during operations
- GetVersionInfo through full stack
- Write after invalidate
- Multiple overwrites of same block

### 2.2 OTA Configuration Migration (`test_Fee_OTA_Scenarios.c`)

10 tests covering:
- Data survives OTA reboot (re-init with flash preserved)
- Unknown blocks in flash ignored
- Multiple blocks survive OTA
- Write new block after re-init
- Overwrite after re-init
- Invalidate after re-init
- Double reboot stability
- Immediate block survives OTA
- Blank flash OTA re-init
- Sequential data pattern preservation

### 2.3 Bootloader Coexistence (`test_Fee_Bootloader_Coexistence.c`)

8 tests covering:
- Fee init does not corrupt bootloader area (MemAcc area 1)
- Fee write does not corrupt bootloader area
- MemAcc read from bootloader area works independently
- MemAcc write to bootloader area does not affect Fee data
- Fee uses area 0 configuration
- MemAcc erase in bootloader area does not affect Fee
- Both areas configured correctly
- Concurrent Fee and bootloader area access

### 2.4 ECC Fault Injection (`test_Fee_ECC_FaultInjection.c`)

12 tests covering:
- Correctable ECC on read
- Uncorrectable ECC on read (job failure)
- ECC on header region
- ECC on sector header during init
- Normal path (no ECC errors)
- ECC on second block only (first unaffected)
- Write with flash API rejection
- ECC cleared after erase
- Job result override mechanism
- Large block with partial ECC
- API call count tracking
- Multiple correctable ECCs

### 2.5 Power Loss Recovery (`test_Fee_Recovery.c`)

10 tests covering:
- Recovery from blank flash
- Single committed block recovery
- Multiple blocks recovery
- Incomplete write ignored (different block)
- Recovery then new write
- Invalidated block stays invalid
- Triple reboot stability
- Immediate data block recovery
- Small immediate block (8 bytes) recovery
- Large block (512 bytes) recovery

### 2.6 Multi-Core / SchM (`test_Fee_MultiCore.c`)

8 tests covering:
- SchM enter/exit balanced after init
- SchM enter/exit balanced after write
- SchM enter/exit balanced after read
- SchM enter/exit balanced after cancel
- SchM enter/exit balanced after invalidate
- MemAcc lock and release
- MemAcc reject operation when locked
- SchM balanced after multiple operations

## 3. Preconditions

- MemAcc initialized with 2 address areas (Fee + Bootloader)
- Mem_DFLS_Stub provides RAM-backed flash simulation
- TC3xx flash behavior: erased = 0x00, writes OR bits
- All stubs reset before each test case

## 4. Pass Criteria

All 69 integration tests pass with 0 failures.

## 5. Traceability

| SWS Requirement | Test Suite |
|-----------------|------------|
| SWS_Fee_00073 (Init) | Integration, OTA, Recovery |
| SWS_Fee_00080 (Read) | Integration, ECC, Recovery |
| SWS_Fee_00085 (Write) | Integration, OTA, Recovery |
| SWS_Fee_00095 (Invalidate) | Integration, OTA, Recovery |
| SWS_Fee_00160 (GC) | Integration |
| SWS_MemAcc_00020 (Area isolation) | Bootloader, MultiCore |
| SWS_MemAcc_00030 (Lock) | MultiCore |
