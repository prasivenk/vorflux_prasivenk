# Traceability Matrix

**AUTOSAR R24-11 Fee + MemAcc | Full Traceability**

Pipe-delimited for Excel import. Columns:
`System Req ID | SW Req ID | Design Element | Source File:Function | Test Case ID | Safety Req ID`

---

```
System Req ID | SW Req ID | Design Element | Source File:Function | Test Case ID | Safety Req ID
SYS-FEE-INIT-001 | SWR-FEE-001 | Fee initialization | src/Fee.c:Fee_Init | test_Fee_Api.c:test_Init_ValidConfig | —
SYS-FEE-INIT-001 | SWR-FEE-001 | Fee init null check | src/Fee.c:Fee_Init | test_Fee_Api.c:test_Init_NullConfig | —
SYS-FEE-INIT-001 | SWR-FEE-002 | State machine initialization | src/Fee_StateMachine.c:Fee_StateMachine_Init | test_Fee_StateMachine.c:test_Init_StateTransition_ToReadSectorHeader | —
SYS-FEE-INIT-001 | SWR-FEE-002 | Sector header read | src/Fee_StateMachine.c:Fee_StateMachine_Process | test_Fee_StateMachine.c:test_Init_FirstProcess_ToWaitSectorHeader | —
SYS-FEE-INIT-001 | SWR-FEE-002 | Sector header parse | src/Fee_StateMachine.c:Fee_StateMachine_Process | test_Fee_StateMachine.c:test_Init_AfterSectorRead_ToNextSector | —
SYS-FEE-INIT-001 | SWR-FEE-002 | All sectors → scan records | src/Fee_StateMachine.c:Fee_StateMachine_Process | test_Fee_StateMachine.c:test_Init_AllSectors_ToScanRecords | —
SYS-FEE-INIT-001 | SWR-FEE-002 | Blank flash → idle | src/Fee_StateMachine.c:Fee_StateMachine_Process | test_Fee_StateMachine.c:test_Init_BlankFlash_GoesToIdle | —
SYS-FEE-INIT-001 | SWR-FEE-002 | Init drives to idle | src/Fee_StateMachine.c:Fee_StateMachine_Process | test_Fee_Api.c:test_Init_DrivesToIdle | —
SYS-FEE-INIT-001 | SWR-FEE-002 | Init valid block scan | src/Fee_StateMachine.c:Fee_StateMachine_Process | test_Fee_StateMachine.c:test_Init_WithValidBlock | —
SYS-FEE-INIT-001 | SWR-FEE-002 | Init invalid block scan | src/Fee_StateMachine.c:Fee_StateMachine_Process | test_Fee_StateMachine.c:test_Init_WithInvalidBlock | —
SYS-FEE-INIT-001 | SWR-FEE-002 | Init inconsistent block | src/Fee_StateMachine.c:Fee_StateMachine_Process | test_Fee_StateMachine.c:test_Init_WithInconsistentBlock | —
SYS-FEE-INIT-001 | SWR-FEE-002 | Init multiple valid blocks | src/Fee_StateMachine.c:Fee_StateMachine_Process | test_Fee_StateMachine.c:test_Init_MultipleValidBlocks | —
SYS-FEE-INIT-001 | SWR-FEE-002 | Init newer supersedes | src/Fee_StateMachine.c:Fee_StateMachine_Process | test_Fee_StateMachine.c:test_Init_NewerBlockSupersedes | SR-010
SYS-FEE-INIT-001 | SWR-FEE-002 | Init highest seq sector | src/Fee_StateMachine.c:Fee_StateMachine_Process | test_Fee_StateMachine.c:test_Init_HighestSeqNumSector | —
SYS-FEE-INIT-001 | SWR-FEE-002 | Init unknown block ignored | src/Fee_StateMachine.c:Fee_StateMachine_Process | test_Fee_StateMachine.c:test_Init_UnknownBlockIgnored | —
SYS-FEE-READ-001 | SWR-FEE-010 | Read API DET uninit | src/Fee.c:Fee_Read | test_Fee_Api.c:test_Read_Uninit | —
SYS-FEE-READ-001 | SWR-FEE-010 | Read API DET invalid block | src/Fee.c:Fee_Read | test_Fee_Api.c:test_Read_InvalidBlockNumber | —
SYS-FEE-READ-001 | SWR-FEE-010 | Read API DET null ptr | src/Fee.c:Fee_Read | test_Fee_Api.c:test_Read_NullPointer | —
SYS-FEE-READ-001 | SWR-FEE-010 | Read API DET zero length | src/Fee.c:Fee_Read | test_Fee_Api.c:test_Read_ZeroLength | —
SYS-FEE-READ-001 | SWR-FEE-010 | Read API DET invalid offset | src/Fee.c:Fee_Read | test_Fee_Api.c:test_Read_InvalidOffset | —
SYS-FEE-READ-001 | SWR-FEE-010 | Read acceptance when idle | src/Fee.c:Fee_Read | test_Fee_Api.c:test_Read_AcceptWhenIdle | —
SYS-FEE-READ-001 | SWR-FEE-010 | Read rejection when busy | src/Fee.c:Fee_Read | test_Fee_Api.c:test_Read_RejectionWhenBusy | —
SYS-FEE-READ-001 | SWR-FEE-010 | Read valid partial | src/Fee.c:Fee_Read | test_Fee_Api.c:test_Read_ValidPartialRead | —
SYS-FEE-READ-001 | SWR-FEE-011 | Read SM non-existent block | src/Fee_StateMachine.c:Fee_StateMachine_Process | test_Fee_StateMachine.c:test_Read_NonExistentBlock | —
SYS-FEE-READ-001 | SWR-FEE-011 | Read SM valid block | src/Fee_StateMachine.c:Fee_StateMachine_Process | test_Fee_StateMachine.c:test_Read_ValidBlock | SR-001
SYS-FEE-READ-001 | SWR-FEE-011 | Read SM inconsistent block | src/Fee_StateMachine.c:Fee_StateMachine_Process | test_Fee_StateMachine.c:test_Read_InconsistentBlock | —
SYS-FEE-READ-001 | SWR-FEE-012 | Read CRC mismatch | src/Fee_StateMachine.c:Fee_StateMachine_Process | test_Fee_StateMachine.c:test_Read_CrcMismatch | SR-001
SYS-FEE-READ-001 | SWR-FEE-012 | Partial read CRC skipped | src/Fee_StateMachine.c:Fee_StateMachine_Process | test_Fee_StateMachine.c:test_Read_PartialRead_CrcSkipped | —
SYS-FEE-READ-001 | SWR-FEE-011 | Read after invalidate | src/Fee_StateMachine.c:Fee_StateMachine_Process | test_Fee_StateMachine.c:test_Read_AfterInvalidate | —
SYS-FEE-WRITE-001 | SWR-FEE-020 | Write API DET uninit | src/Fee.c:Fee_Write | test_Fee_Api.c:test_Write_Uninit | —
SYS-FEE-WRITE-001 | SWR-FEE-020 | Write API DET invalid block | src/Fee.c:Fee_Write | test_Fee_Api.c:test_Write_InvalidBlockNumber | —
SYS-FEE-WRITE-001 | SWR-FEE-020 | Write API DET null ptr | src/Fee.c:Fee_Write | test_Fee_Api.c:test_Write_NullPointer | —
SYS-FEE-WRITE-001 | SWR-FEE-020 | Write acceptance when idle | src/Fee.c:Fee_Write | test_Fee_Api.c:test_Write_AcceptWhenIdle | —
SYS-FEE-WRITE-001 | SWR-FEE-020 | Write rejection when busy | src/Fee.c:Fee_Write | test_Fee_Api.c:test_Write_RejectionWhenBusy | —
SYS-FEE-WRITE-001 | SWR-FEE-021 | Write full flow | src/Fee_StateMachine.c:Fee_StateMachine_Process | test_Fee_StateMachine.c:test_Write_FullFlow | SR-002
SYS-FEE-WRITE-001 | SWR-FEE-021 | Write then read | src/Fee_StateMachine.c:Fee_StateMachine_Process | test_Fee_StateMachine.c:test_Write_ThenRead | SR-001, SR-002
SYS-FEE-WRITE-001 | SWR-FEE-021 | Write MemAcc failure | src/Fee_StateMachine.c:Fee_StateMachine_Process | test_Fee_StateMachine.c:test_Write_MemAccFailure_HeaderWrite | SR-005
SYS-FEE-WRITE-001 | SWR-FEE-021 | Write multiple blocks | src/Fee_StateMachine.c:Fee_StateMachine_Process | test_Fee_StateMachine.c:test_Write_MultipleBlocks | —
SYS-FEE-WRITE-001 | SWR-FEE-021 | Write overwrite | src/Fee_StateMachine.c:Fee_StateMachine_Process | test_Fee_StateMachine.c:test_Write_Overwrite | SR-010
SYS-FEE-WRITE-001 | SWR-FEE-021 | Write immediate block | src/Fee_StateMachine.c:Fee_StateMachine_Process | test_Fee_StateMachine.c:test_Write_ImmediateBlock | —
SYS-FEE-WRITE-001 | SWR-FEE-021 | Write large block | src/Fee_StateMachine.c:Fee_StateMachine_Process | test_Fee_StateMachine.c:test_Write_LargeBlock | —
SYS-FEE-WRITE-001 | SWR-FEE-021 | Write then partial read | src/Fee_StateMachine.c:Fee_StateMachine_Process | test_Fee_StateMachine.c:test_Write_ThenPartialRead_WithOffset | —
SYS-FEE-WRITE-001 | SWR-FEE-022 | Write block info CRC | src/Fee_StateMachine.c:Fee_StateMachine_Process | test_Fee_StateMachine.c:test_Write_BlockInfoHasCorrectCrc | SR-001
SYS-FEE-WRITE-001 | SWR-FEE-022 | Write block info sector | src/Fee_StateMachine.c:Fee_StateMachine_Process | test_Fee_StateMachine.c:test_Write_BlockInfoCorrectSectorIndex | —
SYS-FEE-WRITE-001 | SWR-FEE-023 | Write seq counter increments | src/Fee_StateMachine.c:Fee_StateMachine_Process | test_Fee_StateMachine.c:test_Write_SequenceCounterIncrements | SR-010
SYS-FEE-CANCEL-001 | SWR-FEE-030 | Cancel API uninit | src/Fee.c:Fee_Cancel | test_Fee_Api.c:test_Cancel_Uninit | —
SYS-FEE-CANCEL-001 | SWR-FEE-030 | Cancel after init | src/Fee.c:Fee_Cancel | test_Fee_Api.c:test_Cancel_AfterInit | —
SYS-FEE-CANCEL-001 | SWR-FEE-031 | Cancel during read wait | src/Fee_StateMachine.c:Fee_StateMachine_Cancel | test_Fee_StateMachine.c:test_Cancel_DuringReadWait | —
SYS-FEE-CANCEL-001 | SWR-FEE-031 | Cancel during write header wait | src/Fee_StateMachine.c:Fee_StateMachine_Cancel | test_Fee_StateMachine.c:test_Cancel_DuringWriteHeaderWait | —
SYS-FEE-CANCEL-001 | SWR-FEE-031 | Cancel during write data wait | src/Fee_StateMachine.c:Fee_StateMachine_Cancel | test_Fee_StateMachine.c:test_Cancel_DuringWriteDataWait | —
SYS-FEE-CANCEL-001 | SWR-FEE-031 | Cancel during write valid wait | src/Fee_StateMachine.c:Fee_StateMachine_Cancel | test_Fee_StateMachine.c:test_Cancel_DuringWriteValidWait | —
SYS-FEE-CANCEL-001 | SWR-FEE-031 | Cancel during invalidate wait | src/Fee_StateMachine.c:Fee_StateMachine_Cancel | test_Fee_StateMachine.c:test_Cancel_DuringInvalidateWait | —
SYS-FEE-STATUS-001 | SWR-FEE-040 | GetStatus uninit | src/Fee.c:Fee_GetStatus | test_Fee_Api.c:test_GetStatus_Uninit | —
SYS-FEE-STATUS-001 | SWR-FEE-040 | GetStatus correct | src/Fee.c:Fee_GetStatus | test_Fee_Api.c:test_GetStatus_CorrectStatus | —
SYS-FEE-STATUS-001 | SWR-FEE-041 | GetJobResult uninit | src/Fee.c:Fee_GetJobResult | test_Fee_Api.c:test_GetJobResult_Uninit | —
SYS-FEE-STATUS-001 | SWR-FEE-041 | GetJobResult correct | src/Fee.c:Fee_GetJobResult | test_Fee_Api.c:test_GetJobResult_CorrectResult | —
SYS-FEE-INVAL-001 | SWR-FEE-050 | Invalidate API uninit | src/Fee.c:Fee_InvalidateBlock | test_Fee_Api.c:test_InvalidateBlock_Uninit | —
SYS-FEE-INVAL-001 | SWR-FEE-050 | Invalidate API invalid block | src/Fee.c:Fee_InvalidateBlock | test_Fee_Api.c:test_InvalidateBlock_InvalidBlockNo | —
SYS-FEE-INVAL-001 | SWR-FEE-050 | Invalidate accept when idle | src/Fee.c:Fee_InvalidateBlock | test_Fee_Api.c:test_InvalidateBlock_AcceptWhenIdle | —
SYS-FEE-INVAL-001 | SWR-FEE-051 | Invalidate full flow | src/Fee_StateMachine.c:Fee_StateMachine_Process | test_Fee_StateMachine.c:test_Invalidate_FullFlow | —
SYS-FEE-INVAL-001 | SWR-FEE-051 | Invalidate non-existent | src/Fee_StateMachine.c:Fee_StateMachine_Process | test_Fee_StateMachine.c:test_Invalidate_NonExistentBlock | —
SYS-FEE-ERASE-001 | SWR-FEE-060 | EraseImmediate uninit | src/Fee.c:Fee_EraseImmediateBlock | test_Fee_Api.c:test_EraseImmediateBlock_Uninit | —
SYS-FEE-ERASE-001 | SWR-FEE-060 | EraseImmediate invalid block | src/Fee.c:Fee_EraseImmediateBlock | test_Fee_Api.c:test_EraseImmediateBlock_InvalidBlockNo | —
SYS-FEE-ERASE-001 | SWR-FEE-060 | EraseImmediate not immediate | src/Fee.c:Fee_EraseImmediateBlock | test_Fee_Api.c:test_EraseImmediateBlock_NotImmediate | —
SYS-FEE-ERASE-001 | SWR-FEE-060 | EraseImmediate accept | src/Fee.c:Fee_EraseImmediateBlock | test_Fee_Api.c:test_EraseImmediateBlock_AcceptWhenIdle | —
SYS-FEE-ERASE-001 | SWR-FEE-061 | EraseImmediate not found | src/Fee_StateMachine.c:Fee_StateMachine_Process | test_Fee_StateMachine.c:test_EraseImmediate_NotFound | —
SYS-FEE-ERASE-001 | SWR-FEE-061 | EraseImmediate existing block | src/Fee_StateMachine.c:Fee_StateMachine_Process | test_Fee_StateMachine.c:test_EraseImmediate_ExistingBlock | —
SYS-FEE-VERSION-001 | SWR-FEE-070 | GetVersionInfo null ptr | src/Fee.c:Fee_GetVersionInfo | test_Fee_Api.c:test_GetVersionInfo_NullPointer | —
SYS-FEE-VERSION-001 | SWR-FEE-070 | GetVersionInfo correct | src/Fee.c:Fee_GetVersionInfo | test_Fee_Api.c:test_GetVersionInfo_CorrectValues | —
SYS-FEE-MODE-001 | SWR-FEE-080 | SetMode no-op | src/Fee.c:Fee_SetMode | test_Fee_Api.c:test_SetMode_NoOp | —
SYS-FEE-MAIN-001 | SWR-FEE-090 | MainFunction uninit | src/Fee.c:Fee_MainFunction | test_Fee_Api.c:test_MainFunction_Uninit | —
SYS-FEE-GC-001 | SWR-FEE-100 | GC preemption immediate | src/Fee_StateMachine.c:Fee_StateMachine_AcceptJob | test_Fee_Api.c:test_ImmediateWrite_DuringGC | SR-007
SYS-FEE-GC-001 | SWR-FEE-100 | GC reject normal during GC | src/Fee_StateMachine.c:Fee_StateMachine_AcceptJob | test_Fee_Api.c:test_NormalWrite_DuringGC | —
SYS-FEE-GC-001 | SWR-FEE-100 | GC suspended for immediate | src/Fee_StateMachine.c:Fee_StateMachine_AcceptJob | test_Fee_StateMachine.c:test_GC_Suspended_ForImmediate | —
SYS-FEE-NVM-001 | SWR-FEE-110 | NvM end notification | src/Fee_StateMachine.c:Fee_StateMachine_CompleteJob | test_Fee_StateMachine.c:test_NvM_EndNotification_OnSuccess | —
SYS-FEE-NVM-001 | SWR-FEE-110 | NvM error notification | src/Fee_StateMachine.c:Fee_StateMachine_CompleteJob | test_Fee_StateMachine.c:test_NvM_ErrorNotification_OnFailure | SR-005
SYS-FEE-GC-002 | SWR-FEE-101 | GC active returns to idle | src/Fee_StateMachine.c:Fee_StateMachine_Process | test_Fee_StateMachine.c:test_GC_Active_ReturnsToIdle | —
SYS-FEE-ERROR-001 | SWR-FEE-120 | Error state DEM and idle | src/Fee_StateMachine.c:Fee_StateMachine_Process | test_Fee_StateMachine.c:test_Error_State_DemAndIdle | SR-013
SYS-FEE-CRC-001 | SWR-FEE-130 | CRC known test vector | src/Fee_Crc.c:Fee_Crc_CalculateBlock | test_Fee_Crc.c:test_CRC_KnownTestVector | SR-001
SYS-FEE-CRC-001 | SWR-FEE-130 | CRC explicit start | src/Fee_Crc.c:Fee_Crc_Calculate | test_Fee_Crc.c:test_CRC_KnownTestVector_ExplicitStart | SR-001
SYS-FEE-CRC-001 | SWR-FEE-130 | CRC empty data | src/Fee_Crc.c:Fee_Crc_CalculateBlock | test_Fee_Crc.c:test_CRC_EmptyData | —
SYS-FEE-CRC-001 | SWR-FEE-130 | CRC single byte zero | src/Fee_Crc.c:Fee_Crc_CalculateBlock | test_Fee_Crc.c:test_CRC_SingleByteZero | —
SYS-FEE-CRC-001 | SWR-FEE-130 | CRC single byte FF | src/Fee_Crc.c:Fee_Crc_CalculateBlock | test_Fee_Crc.c:test_CRC_SingleByteFF | —
SYS-FEE-CRC-001 | SWR-FEE-130 | CRC all zero data | src/Fee_Crc.c:Fee_Crc_CalculateBlock | test_Fee_Crc.c:test_CRC_AllZeroData | —
SYS-FEE-CRC-001 | SWR-FEE-130 | CRC all FF data | src/Fee_Crc.c:Fee_Crc_CalculateBlock | test_Fee_Crc.c:test_CRC_AllFFData | —
SYS-FEE-CRC-001 | SWR-FEE-130 | CRC large data | src/Fee_Crc.c:Fee_Crc_CalculateBlock | test_Fee_Crc.c:test_CRC_LargeData | —
SYS-FEE-CRC-001 | SWR-FEE-130 | CRC incremental | src/Fee_Crc.c:Fee_Crc_Calculate | test_Fee_Crc.c:test_CRC_IncrementalCalculation | SR-004
SYS-FEE-CRC-001 | SWR-FEE-130 | CRC incremental splits | src/Fee_Crc.c:Fee_Crc_Calculate | test_Fee_Crc.c:test_CRC_IncrementalDifferentSplits | SR-004
SYS-FEE-CRC-001 | SWR-FEE-130 | CRC block header bytes | src/Fee_Crc.c:Fee_Crc_CalculateBlock | test_Fee_Crc.c:test_CRC_BlockHeaderBytes | —
SYS-FEE-CRC-001 | SWR-FEE-130 | CRC custom start value | src/Fee_Crc.c:Fee_Crc_Calculate | test_Fee_Crc.c:test_CRC_CustomStartValue | —
SYS-FEE-CRC-001 | SWR-FEE-130 | CRC two bytes | src/Fee_Crc.c:Fee_Crc_CalculateBlock | test_Fee_Crc.c:test_CRC_TwoBytes | —
SYS-FEE-SECTOR-001 | SWR-FEE-140 | Sector header round-trip | src/Fee_Sector.c:Fee_Sector_BuildSectorHeader, Fee_Sector_ParseSectorHeader | test_Fee_Sector.c:test_SectorHeader_BuildParse_RoundTrip | —
SYS-FEE-SECTOR-001 | SWR-FEE-140 | Sector header various values | src/Fee_Sector.c:Fee_Sector_BuildSectorHeader | test_Fee_Sector.c:test_SectorHeader_VariousValues | —
SYS-FEE-SECTOR-001 | SWR-FEE-140 | Sector header blank | src/Fee_Sector.c:Fee_Sector_ParseSectorHeader | test_Fee_Sector.c:test_SectorHeader_BlankSector | —
SYS-FEE-SECTOR-001 | SWR-FEE-140 | Sector header corrupt magic | src/Fee_Sector.c:Fee_Sector_ParseSectorHeader | test_Fee_Sector.c:test_SectorHeader_CorruptMagic | SR-003
SYS-FEE-SECTOR-001 | SWR-FEE-140 | Sector header all FF | src/Fee_Sector.c:Fee_Sector_ParseSectorHeader | test_Fee_Sector.c:test_SectorHeader_AllFF | —
SYS-FEE-SECTOR-001 | SWR-FEE-140 | Sector header status active | src/Fee_Sector.c:Fee_Sector_ParseSectorHeader | test_Fee_Sector.c:test_SectorHeader_StatusActive | —
SYS-FEE-SECTOR-001 | SWR-FEE-140 | Sector header reserved bytes | src/Fee_Sector.c:Fee_Sector_BuildSectorHeader | test_Fee_Sector.c:test_SectorHeader_ReservedBytes | —
SYS-FEE-SECTOR-002 | SWR-FEE-141 | Block header round-trip | src/Fee_Sector.c:Fee_Sector_BuildBlockHeader, Fee_Sector_ParseBlockHeader | test_Fee_Sector.c:test_BlockHeader_BuildParse_RoundTrip | —
SYS-FEE-SECTOR-002 | SWR-FEE-141 | Block header max values | src/Fee_Sector.c:Fee_Sector_BuildBlockHeader | test_Fee_Sector.c:test_BlockHeader_MaxValues | —
SYS-FEE-SECTOR-002 | SWR-FEE-141 | Block header zero values | src/Fee_Sector.c:Fee_Sector_BuildBlockHeader | test_Fee_Sector.c:test_BlockHeader_ZeroValues | —
SYS-FEE-SECTOR-002 | SWR-FEE-141 | Block header valid marker erased | src/Fee_Sector.c:Fee_Sector_ParseBlockHeader | test_Fee_Sector.c:test_BlockHeader_ValidMarker_Erased | SR-002
SYS-FEE-SECTOR-002 | SWR-FEE-141 | Block header valid marker valid | src/Fee_Sector.c:Fee_Sector_ParseBlockHeader | test_Fee_Sector.c:test_BlockHeader_ValidMarker_TransitionToValid | SR-002
SYS-FEE-SECTOR-002 | SWR-FEE-141 | Block header valid marker invalid | src/Fee_Sector.c:Fee_Sector_ParseBlockHeader | test_Fee_Sector.c:test_BlockHeader_ValidMarker_TransitionToInvalid | —
SYS-FEE-SECTOR-002 | SWR-FEE-141 | Block header corrupt CRC | src/Fee_Sector.c:Fee_Sector_ParseBlockHeader | test_Fee_Sector.c:test_BlockHeader_CorruptCRC | SR-001
SYS-FEE-SECTOR-002 | SWR-FEE-141 | Block header corrupt data | src/Fee_Sector.c:Fee_Sector_ParseBlockHeader | test_Fee_Sector.c:test_BlockHeader_CorruptDataByte | —
SYS-FEE-SECTOR-002 | SWR-FEE-141 | Block header reserved bytes | src/Fee_Sector.c:Fee_Sector_BuildBlockHeader | test_Fee_Sector.c:test_BlockHeader_ReservedBytes | —
SYS-FEE-SECTOR-002 | SWR-FEE-141 | Block header write counter | src/Fee_Sector.c:Fee_Sector_BuildBlockHeader | test_Fee_Sector.c:test_BlockHeader_WriteCounter | —
SYS-FEE-SECTOR-003 | SWR-FEE-142 | Allocate single block | src/Fee_Sector.c:Fee_Sector_AllocateBlock | test_Fee_Sector.c:test_AllocateBlock_SingleBlock | —
SYS-FEE-SECTOR-003 | SWR-FEE-142 | Allocate until full | src/Fee_Sector.c:Fee_Sector_AllocateBlock | test_Fee_Sector.c:test_AllocateBlock_UntilFull | —
SYS-FEE-SECTOR-003 | SWR-FEE-142 | Allocate various sizes | src/Fee_Sector.c:Fee_Sector_AllocateBlock | test_Fee_Sector.c:test_AllocateBlock_8Bytes | —
SYS-FEE-SECTOR-003 | SWR-FEE-142 | Allocate 16 bytes | src/Fee_Sector.c:Fee_Sector_AllocateBlock | test_Fee_Sector.c:test_AllocateBlock_16Bytes | —
SYS-FEE-SECTOR-003 | SWR-FEE-142 | Allocate 31 bytes | src/Fee_Sector.c:Fee_Sector_AllocateBlock | test_Fee_Sector.c:test_AllocateBlock_31Bytes | —
SYS-FEE-SECTOR-003 | SWR-FEE-142 | Allocate 32 bytes | src/Fee_Sector.c:Fee_Sector_AllocateBlock | test_Fee_Sector.c:test_AllocateBlock_32Bytes | —
SYS-FEE-SECTOR-003 | SWR-FEE-142 | Allocate 33 bytes | src/Fee_Sector.c:Fee_Sector_AllocateBlock | test_Fee_Sector.c:test_AllocateBlock_33Bytes | —
SYS-FEE-SECTOR-003 | SWR-FEE-142 | Allocate 512 bytes | src/Fee_Sector.c:Fee_Sector_AllocateBlock | test_Fee_Sector.c:test_AllocateBlock_512Bytes | —
SYS-FEE-SECTOR-003 | SWR-FEE-142 | Allocate not enough space | src/Fee_Sector.c:Fee_Sector_AllocateBlock | test_Fee_Sector.c:test_AllocateBlock_NotEnoughSpace | —
SYS-FEE-SECTOR-003 | SWR-FEE-142 | Allocate multiple blocks | src/Fee_Sector.c:Fee_Sector_AllocateBlock | test_Fee_Sector.c:test_AllocateBlock_MultipleBlocks | —
SYS-FEE-SECTOR-004 | SWR-FEE-143 | PrepareWriteBuffer 8 bytes | src/Fee_Sector.c:Fee_Sector_PrepareWriteBuffer | test_Fee_Sector.c:test_PrepareWriteBuffer_8Bytes | —
SYS-FEE-SECTOR-004 | SWR-FEE-143 | PrepareWriteBuffer 32 bytes | src/Fee_Sector.c:Fee_Sector_PrepareWriteBuffer | test_Fee_Sector.c:test_PrepareWriteBuffer_32Bytes | —
SYS-FEE-SECTOR-004 | SWR-FEE-143 | PrepareWriteBuffer 33 bytes | src/Fee_Sector.c:Fee_Sector_PrepareWriteBuffer | test_Fee_Sector.c:test_PrepareWriteBuffer_33Bytes | —
SYS-FEE-SECTOR-004 | SWR-FEE-143 | PrepareWriteBuffer 1 byte | src/Fee_Sector.c:Fee_Sector_PrepareWriteBuffer | test_Fee_Sector.c:test_PrepareWriteBuffer_1Byte | —
SYS-FEE-SECTOR-005 | SWR-FEE-144 | Fill percentage empty | src/Fee_Sector.c:Fee_Sector_GetFillPercentage | test_Fee_Sector.c:test_GetFillPercentage_Empty | —
SYS-FEE-SECTOR-005 | SWR-FEE-144 | Fill percentage half full | src/Fee_Sector.c:Fee_Sector_GetFillPercentage | test_Fee_Sector.c:test_GetFillPercentage_HalfFull | —
SYS-FEE-SECTOR-005 | SWR-FEE-144 | Fill percentage full | src/Fee_Sector.c:Fee_Sector_GetFillPercentage | test_Fee_Sector.c:test_GetFillPercentage_Full | —
SYS-FEE-SECTOR-006 | SWR-FEE-145 | LE round-trip sector hdr | src/Fee_Sector.c:Fee_Sector_BuildSectorHeader | test_Fee_Sector.c:test_LE_RoundTrip_ViaSectorHeader | —
SYS-FEE-SECTOR-006 | SWR-FEE-145 | LE16 round-trip block hdr | src/Fee_Sector.c:Fee_Sector_BuildBlockHeader | test_Fee_Sector.c:test_LE16_RoundTrip_ViaBlockHeader | —
SYS-FEE-SECTOR-007 | SWR-FEE-146 | Get header buffer | src/Fee_Sector.c:Fee_Sector_GetHeaderBuffer | test_Fee_Sector.c:test_GetHeaderBuffer_NotNull | SR-006
SYS-FEE-SECTOR-007 | SWR-FEE-146 | Get write buffer | src/Fee_Sector.c:Fee_Sector_GetWriteBuffer | test_Fee_Sector.c:test_GetWriteBuffer_NotNull | SR-006
SYS-FEE-GC-001 | SWR-FEE-150 | GC init not active | src/Fee_GarbageCollect.c:Fee_GarbageCollect_Init | test_Fee_GarbageCollect.c:test_GC_Init_IsNotActive | —
SYS-FEE-GC-001 | SWR-FEE-150 | GC idle returns complete | src/Fee_GarbageCollect.c:Fee_GarbageCollect_Process | test_Fee_GarbageCollect.c:test_GC_Process_WhenIdle_ReturnsComplete | —
SYS-FEE-GC-001 | SWR-FEE-151 | GC select fullest source | src/Fee_GarbageCollect.c:Fee_GarbageCollect_Trigger | test_Fee_GarbageCollect.c:test_GC_Trigger_SelectsFullestSource | —
SYS-FEE-GC-001 | SWR-FEE-151 | GC wear leveling | src/Fee_GarbageCollect.c:Fee_GarbageCollect_Trigger | test_Fee_GarbageCollect.c:test_GC_Trigger_WearLeveling_LowestEraseCount | SR-011
SYS-FEE-GC-001 | SWR-FEE-152 | GC single block cycle | src/Fee_GarbageCollect.c:Fee_GarbageCollect_Process | test_Fee_GarbageCollect.c:test_GC_SingleBlock_FullCycle | SR-002
SYS-FEE-GC-001 | SWR-FEE-152 | GC multi block all copied | src/Fee_GarbageCollect.c:Fee_GarbageCollect_Process | test_Fee_GarbageCollect.c:test_GC_MultiBlock_AllCopied | —
SYS-FEE-GC-001 | SWR-FEE-152 | GC mixed only valid copied | src/Fee_GarbageCollect.c:Fee_GarbageCollect_Process | test_Fee_GarbageCollect.c:test_GC_MixedBlocks_OnlyValidCopied | —
SYS-FEE-GC-001 | SWR-FEE-153 | GC suspend/resume | src/Fee_GarbageCollect.c:Fee_GarbageCollect_Suspend, Fee_GarbageCollect_Resume | test_Fee_GarbageCollect.c:test_GC_SuspendResume | —
SYS-FEE-GC-001 | SWR-FEE-153 | GC suspend before processing | src/Fee_GarbageCollect.c:Fee_GarbageCollect_Suspend | test_Fee_GarbageCollect.c:test_GC_Suspend_BeforeAnyProcessing | —
SYS-FEE-GC-001 | SWR-FEE-154 | GC error copy read failure | src/Fee_GarbageCollect.c:Fee_GarbageCollect_Process | test_Fee_GarbageCollect.c:test_GC_Error_CopyReadFailure | SR-005
SYS-FEE-GC-001 | SWR-FEE-154 | GC error write header failure | src/Fee_GarbageCollect.c:Fee_GarbageCollect_Process | test_Fee_GarbageCollect.c:test_GC_Error_CopyWriteHeaderFailure | SR-005
SYS-FEE-GC-001 | SWR-FEE-154 | GC error write data failure | src/Fee_GarbageCollect.c:Fee_GarbageCollect_Process | test_Fee_GarbageCollect.c:test_GC_Error_CopyWriteDataFailure | SR-005
SYS-FEE-GC-001 | SWR-FEE-154 | GC error write valid failure | src/Fee_GarbageCollect.c:Fee_GarbageCollect_Process | test_Fee_GarbageCollect.c:test_GC_Error_CopyWriteValidFailure | SR-005
SYS-FEE-GC-001 | SWR-FEE-154 | GC error erase failure | src/Fee_GarbageCollect.c:Fee_GarbageCollect_Process | test_Fee_GarbageCollect.c:test_GC_Error_EraseFailure | SR-005
SYS-FEE-GC-001 | SWR-FEE-155 | GC stale pointer cleanup | src/Fee_GarbageCollect.c:Fee_GarbageCollect_Process | test_Fee_GarbageCollect.c:test_GC_StalePointerCleanup | —
SYS-FEE-GC-001 | SWR-FEE-155 | GC CRC mismatch skipped | src/Fee_GarbageCollect.c:Fee_GarbageCollect_Process | test_Fee_GarbageCollect.c:test_GC_CrcMismatch_BlockSkipped | SR-001
SYS-FEE-GC-001 | SWR-FEE-155 | GC CRC mismatch only bad | src/Fee_GarbageCollect.c:Fee_GarbageCollect_Process | test_Fee_GarbageCollect.c:test_GC_CrcMismatch_OnlyBadBlockSkipped | SR-001
SYS-FEE-GC-001 | SWR-FEE-155 | GC no valid blocks erase | src/Fee_GarbageCollect.c:Fee_GarbageCollect_Process | test_Fee_GarbageCollect.c:test_GC_NoValidBlocks_JustErase | —
SYS-FEE-GC-001 | SWR-FEE-156 | GC is active true | src/Fee_GarbageCollect.c:Fee_GarbageCollect_IsActive | test_Fee_GarbageCollect.c:test_GC_IsActive_TrueDuringGC | —
SYS-FEE-GC-001 | SWR-FEE-156 | GC erase count incremented | src/Fee_GarbageCollect.c:Fee_GarbageCollect_Process | test_Fee_GarbageCollect.c:test_GC_EraseCountIncremented | SR-011
SYS-FEE-GC-001 | SWR-FEE-156 | GC source becomes erased | src/Fee_GarbageCollect.c:Fee_GarbageCollect_Process | test_Fee_GarbageCollect.c:test_GC_SourceSector_BecomeErased | —
SYS-FEE-GC-001 | SWR-FEE-151 | GC trigger no source | src/Fee_GarbageCollect.c:Fee_GarbageCollect_Trigger | test_Fee_GarbageCollect.c:test_GC_Trigger_NoSource_Fails | —
SYS-FEE-GC-001 | SWR-FEE-151 | GC trigger no target | src/Fee_GarbageCollect.c:Fee_GarbageCollect_Trigger | test_Fee_GarbageCollect.c:test_GC_Trigger_NoTarget_Fails | —
SYS-FEE-GC-001 | SWR-FEE-157 | GC data integrity readback | src/Fee_GarbageCollect.c:Fee_GarbageCollect_Process | test_Fee_GarbageCollect.c:test_GC_DataIntegrity_Readback | SR-001
SYS-FEE-GC-001 | SWR-FEE-157 | GC seq counter preserved | src/Fee_GarbageCollect.c:Fee_GarbageCollect_Process | test_Fee_GarbageCollect.c:test_GC_SequenceCounter_Preserved | SR-010
SYS-FEE-GC-001 | SWR-FEE-157 | GC data CRC preserved | src/Fee_GarbageCollect.c:Fee_GarbageCollect_Process | test_Fee_GarbageCollect.c:test_GC_DataCrc_Preserved | SR-001
SYS-FEE-GC-001 | SWR-FEE-152 | GC immediate block | src/Fee_GarbageCollect.c:Fee_GarbageCollect_Process | test_Fee_GarbageCollect.c:test_GC_ImmediateBlock | —
SYS-FEE-GC-001 | SWR-FEE-152 | GC large block | src/Fee_GarbageCollect.c:Fee_GarbageCollect_Process | test_Fee_GarbageCollect.c:test_GC_LargeBlock | —
SYS-FEE-GC-001 | SWR-FEE-154 | GC MemAcc read rejected | src/Fee_GarbageCollect.c:Fee_GarbageCollect_Process | test_Fee_GarbageCollect.c:test_GC_Error_MemAccReadRejected | SR-005
SYS-FEE-GC-001 | SWR-FEE-154 | GC MemAcc erase rejected | src/Fee_GarbageCollect.c:Fee_GarbageCollect_Process | test_Fee_GarbageCollect.c:test_GC_Error_MemAccEraseRejected | SR-005
SYS-FEE-GC-001 | SWR-FEE-154 | GC MemAcc write header rejected | src/Fee_GarbageCollect.c:Fee_GarbageCollect_Process | test_Fee_GarbageCollect.c:test_GC_Error_MemAccWriteHeaderRejected | SR-005
SYS-FEE-GC-001 | SWR-FEE-158 | GC state transitions one/call | src/Fee_GarbageCollect.c:Fee_GarbageCollect_Process | test_Fee_GarbageCollect.c:test_GC_StateTransitions_OnePerCall | SR-007
SYS-FEE-GC-001 | SWR-FEE-158 | GC pending poll in-progress | src/Fee_GarbageCollect.c:Fee_GarbageCollect_Process | test_Fee_GarbageCollect.c:test_GC_PendingPoll_ReturnsInProgress | —
SYS-FEE-GC-001 | SWR-FEE-159 | GC addresses updated | src/Fee_GarbageCollect.c:Fee_GarbageCollect_Process | test_Fee_GarbageCollect.c:test_GC_AddressesUpdated | —
SYS-FEE-GC-001 | SWR-FEE-151 | GC multiple erased select lowest | src/Fee_GarbageCollect.c:Fee_GarbageCollect_Trigger | test_Fee_GarbageCollect.c:test_GC_MultipleErasedSectors_SelectLowestErase | SR-011
SYS-FEE-GC-001 | SWR-FEE-160 | GC integration fee SM | src/Fee_GarbageCollect.c + src/Fee_StateMachine.c | test_Fee_GarbageCollect.c:test_GC_Integration_FeeStateMachine | —
SYS-FEE-SAFETY-001 | SWR-FEE-170 | Safety init and check no corruption | src/Fee_Safety.c:Fee_Safety_Init, Fee_Safety_CyclicCheck | test_Fee_Safety.c:test_Safety_InitAndCheck_NoCorruption | SR-004
SYS-FEE-SAFETY-001 | SWR-FEE-170 | Safety corruption BlockInfoTable | src/Fee_Safety.c:Fee_Safety_CyclicCheck | test_Fee_Safety.c:test_Safety_CorruptionDetection_BlockInfoTable | SR-004
SYS-FEE-SAFETY-001 | SWR-FEE-170 | Safety corruption SectorInfo | src/Fee_Safety.c:Fee_Safety_CyclicCheck | test_Fee_Safety.c:test_Safety_CorruptionDetection_SectorInfo | SR-004
SYS-FEE-SAFETY-001 | SWR-FEE-170 | Safety update after modification | src/Fee_Safety.c:Fee_Safety_UpdateRamCrc | test_Fee_Safety.c:test_Safety_UpdateAfterLegitModification | SR-004
SYS-FEE-SAFETY-001 | SWR-FEE-170 | Safety multiple corruptions | src/Fee_Safety.c:Fee_Safety_CyclicCheck | test_Fee_Safety.c:test_Safety_MultipleCorruptions | SR-004
SYS-FEE-SAFETY-002 | SWR-FEE-171 | Flow check correct sequence | src/Fee_Safety.c:Fee_Safety_FlowCheck | test_Fee_Safety.c:test_Safety_FlowCheck_CorrectSequence | SR-014
SYS-FEE-SAFETY-002 | SWR-FEE-171 | Flow check mismatch | src/Fee_Safety.c:Fee_Safety_FlowCheck | test_Fee_Safety.c:test_Safety_FlowCheck_Mismatch | SR-014
SYS-FEE-SAFETY-002 | SWR-FEE-171 | Flow counter reset recheck | src/Fee_Safety.c:Fee_Safety_ResetFlowCounter | test_Fee_Safety.c:test_Safety_FlowCounter_ResetAndRecheck | SR-014
SYS-FEE-SAFETY-002 | SWR-FEE-171 | Safety init resets flow | src/Fee_Safety.c:Fee_Safety_Init | test_Fee_Safety.c:test_Safety_Init_ResetsFlowCounter | SR-014
SYS-FEE-SAFETY-003 | SWR-FEE-172 | Cyclic check via MainFunction | src/Fee.c:Fee_MainFunction → src/Fee_Safety.c:Fee_Safety_CyclicCheck | test_Fee_Safety.c:test_Safety_CyclicCheck_ViaMainFunction | SR-004
SYS-FEE-SAFETY-003 | SWR-FEE-172 | Write updates RAM CRC | src/Fee_Safety.c:Fee_Safety_UpdateRamCrc | test_Fee_Safety.c:test_Safety_WriteUpdatesRamCrc | SR-004
SYS-FEE-SAFETY-003 | SWR-FEE-173 | DEM event ID | include/Fee_Types.h:FEE_E_HARDWARE_ERROR | test_Fee_Safety.c:test_Safety_DemEventId | SR-005
SYS-FEE-CONFIG-001 | SWR-FEE-180 | Config number of blocks | src/Fee_PBcfg.c | test_Fee_JobQueue.c:test_ConfigTable_NumberOfBlocks | —
SYS-FEE-CONFIG-001 | SWR-FEE-180 | Config pointer valid | src/Fee_PBcfg.c | test_Fee_JobQueue.c:test_ConfigPointer_Valid | —
SYS-FEE-CONFIG-001 | SWR-FEE-180 | Block numbers match | src/Fee_PBcfg.c | test_Fee_JobQueue.c:test_BlockNumbers_Match | —
SYS-FEE-CONFIG-001 | SWR-FEE-180 | Block sizes match | src/Fee_PBcfg.c | test_Fee_JobQueue.c:test_BlockSizes_Match | —
SYS-FEE-CONFIG-001 | SWR-FEE-180 | Immediate flags correct | src/Fee_PBcfg.c | test_Fee_JobQueue.c:test_ImmediateFlags_Correct | —
SYS-FEE-CONFIG-001 | SWR-FEE-181 | Block lookup correct index | src/Fee.c:Fee_Internal_FindBlockIndex | test_Fee_JobQueue.c:test_BlockLookup_FindsCorrectIndex | —
SYS-FEE-CONFIG-001 | SWR-FEE-181 | Block lookup invalid | src/Fee.c:Fee_Internal_FindBlockIndex | test_Fee_JobQueue.c:test_BlockLookup_InvalidBlockNumber | —
SYS-FEE-CONFIG-001 | SWR-FEE-181 | Block lookup null config | src/Fee.c:Fee_Internal_FindBlockIndex | test_Fee_JobQueue.c:test_BlockLookup_NullConfig | —
SYS-FEE-CONFIG-001 | SWR-FEE-182 | Config virtual page size | src/Fee_PBcfg.c | test_Fee_JobQueue.c:test_Config_VirtualPageSize | —
SYS-FEE-CONFIG-001 | SWR-FEE-182 | Config sector parameters | src/Fee_PBcfg.c | test_Fee_JobQueue.c:test_Config_SectorParameters | —
SYS-FEE-CONFIG-001 | SWR-FEE-182 | Config MemAcc area ID | src/Fee_PBcfg.c | test_Fee_JobQueue.c:test_Config_MemAccAreaId | —
SYS-FEE-CONFIG-001 | SWR-FEE-183 | Block write cycles unlimited | src/Fee_PBcfg.c | test_Fee_JobQueue.c:test_BlockWriteCycles_AllUnlimited | —
SYS-FEE-CONFIG-001 | SWR-FEE-183 | Block sizes within max | src/Fee_PBcfg.c | test_Fee_JobQueue.c:test_BlockSizes_WithinMaxLimit | SR-006
SYS-FEE-CONFIG-001 | SWR-FEE-184 | JobInfoType init | include/Fee_Types.h:Fee_JobInfoType | test_Fee_JobQueue.c:test_JobInfoType_InitAndReadBack | —
SYS-FEE-CONFIG-001 | SWR-FEE-184 | BlockInfoType init | include/Fee_Types.h:Fee_BlockInfoType | test_Fee_JobQueue.c:test_BlockInfoType_InitAndReadBack | —
SYS-FEE-CONFIG-001 | SWR-FEE-184 | SectorInfoType init | include/Fee_Types.h:Fee_SectorInfoType | test_Fee_JobQueue.c:test_SectorInfoType_InitAndReadBack | —
SYS-FEE-CONFIG-001 | SWR-FEE-185 | Enum values correct | include/Fee_Types.h | test_Fee_JobQueue.c:test_EnumValues_Correct | —
SYS-FEE-CONFIG-001 | SWR-FEE-185 | DET error codes | include/Fee_Types.h | test_Fee_JobQueue.c:test_DetErrorCodes | —
SYS-FEE-CONFIG-001 | SWR-FEE-185 | Constants | include/Fee_Types.h, include/Fee_Cfg.h | test_Fee_JobQueue.c:test_Constants | —
SYS-FEE-CONFIG-001 | SWR-FEE-185 | Config switches | include/Fee_Cfg.h | test_Fee_JobQueue.c:test_ConfigSwitches | —
SYS-FEE-CONFIG-001 | SWR-FEE-181 | Block lookup immediate | src/Fee.c:Fee_Internal_FindBlockIndex | test_Fee_JobQueue.c:test_BlockLookup_ImmediateBlocks | —
SYS-FEE-CONFIG-001 | SWR-FEE-184 | JobInfo none type | include/Fee_Types.h:Fee_JobInfoType | test_Fee_JobQueue.c:test_JobInfo_NoneType | —
SYS-MEMACC-INIT-001 | SWR-MEMACC-001 | MemAcc init | src/MemAcc.c:MemAcc_Init | test_MemAcc_Api.c:test_Init_ValidConfig | —
SYS-MEMACC-INIT-001 | SWR-MEMACC-001 | MemAcc init null | src/MemAcc.c:MemAcc_Init | test_MemAcc_Api.c:test_Init_NullPointer | —
SYS-MEMACC-INIT-001 | SWR-MEMACC-002 | MemAcc deinit | src/MemAcc.c:MemAcc_DeInit | test_MemAcc_Api.c:test_DeInit_AfterInit | —
SYS-MEMACC-INIT-001 | SWR-MEMACC-002 | MemAcc deinit before init | src/MemAcc.c:MemAcc_DeInit | test_MemAcc_Api.c:test_DeInit_BeforeInit | —
SYS-MEMACC-READ-001 | SWR-MEMACC-010 | MemAcc read uninit | src/MemAcc.c:MemAcc_Read | test_MemAcc_Api.c:test_Read_Uninit | —
SYS-MEMACC-READ-001 | SWR-MEMACC-010 | MemAcc read null ptr | src/MemAcc.c:MemAcc_Read | test_MemAcc_Api.c:test_Read_NullPointer | —
SYS-MEMACC-READ-001 | SWR-MEMACC-010 | MemAcc read invalid area | src/MemAcc.c:MemAcc_Read | test_MemAcc_Api.c:test_Read_InvalidAreaId | —
SYS-MEMACC-READ-001 | SWR-MEMACC-010 | MemAcc read invalid address | src/MemAcc.c:MemAcc_Read | test_MemAcc_Api.c:test_Read_InvalidAddress | —
SYS-MEMACC-READ-001 | SWR-MEMACC-010 | MemAcc read zero length | src/MemAcc.c:MemAcc_Read | test_MemAcc_Api.c:test_Read_ZeroLength | —
SYS-MEMACC-READ-001 | SWR-MEMACC-010 | MemAcc read acceptance | src/MemAcc.c:MemAcc_Read | test_MemAcc_Api.c:test_Read_Acceptance | —
SYS-MEMACC-READ-001 | SWR-MEMACC-010 | MemAcc read busy | src/MemAcc.c:MemAcc_Read | test_MemAcc_Api.c:test_Read_BusyRejection | —
SYS-MEMACC-READ-001 | SWR-MEMACC-010 | MemAcc read locked | src/MemAcc.c:MemAcc_Read | test_MemAcc_Api.c:test_Read_LockedRejection | SR-008
SYS-MEMACC-READ-001 | SWR-MEMACC-010 | MemAcc read area 1 | src/MemAcc.c:MemAcc_Read | test_MemAcc_Api.c:test_Read_Area1_Acceptance | —
SYS-MEMACC-WRITE-001 | SWR-MEMACC-020 | MemAcc write uninit | src/MemAcc.c:MemAcc_Write | test_MemAcc_Api.c:test_Write_Uninit | —
SYS-MEMACC-WRITE-001 | SWR-MEMACC-020 | MemAcc write null ptr | src/MemAcc.c:MemAcc_Write | test_MemAcc_Api.c:test_Write_NullPointer | —
SYS-MEMACC-WRITE-001 | SWR-MEMACC-020 | MemAcc write acceptance | src/MemAcc.c:MemAcc_Write | test_MemAcc_Api.c:test_Write_Acceptance | —
SYS-MEMACC-WRITE-001 | SWR-MEMACC-020 | MemAcc write busy | src/MemAcc.c:MemAcc_Write | test_MemAcc_Api.c:test_Write_BusyRejection | —
SYS-MEMACC-ERASE-001 | SWR-MEMACC-030 | MemAcc erase acceptance | src/MemAcc.c:MemAcc_Erase | test_MemAcc_Api.c:test_Erase_Acceptance | —
SYS-MEMACC-ERASE-001 | SWR-MEMACC-030 | MemAcc erase locked | src/MemAcc.c:MemAcc_Erase | test_MemAcc_Api.c:test_Erase_LockedRejection | —
SYS-MEMACC-ERASE-001 | SWR-MEMACC-030 | MemAcc erase invalid area | src/MemAcc.c:MemAcc_Erase | test_MemAcc_Api.c:test_Erase_InvalidArea | —
SYS-MEMACC-BC-001 | SWR-MEMACC-040 | MemAcc blankcheck acceptance | src/MemAcc.c:MemAcc_BlankCheck | test_MemAcc_Api.c:test_BlankCheck_Acceptance | —
SYS-MEMACC-BC-001 | SWR-MEMACC-040 | MemAcc blankcheck zero len | src/MemAcc.c:MemAcc_BlankCheck | test_MemAcc_Api.c:test_BlankCheck_ZeroLength | —
SYS-MEMACC-CMP-001 | SWR-MEMACC-050 | MemAcc compare acceptance | src/MemAcc.c:MemAcc_Compare | test_MemAcc_Api.c:test_Compare_Acceptance | —
SYS-MEMACC-CMP-001 | SWR-MEMACC-050 | MemAcc compare null ptr | src/MemAcc.c:MemAcc_Compare | test_MemAcc_Api.c:test_Compare_NullPointer | —
SYS-MEMACC-CANCEL-001 | SWR-MEMACC-060 | MemAcc cancel | src/MemAcc.c:MemAcc_Cancel | test_MemAcc_Api.c:test_Cancel | —
SYS-MEMACC-CANCEL-001 | SWR-MEMACC-060 | MemAcc cancel uninit | src/MemAcc.c:MemAcc_Cancel | test_MemAcc_Api.c:test_Cancel_Uninit | —
SYS-MEMACC-STATUS-001 | SWR-MEMACC-070 | MemAcc get job result init | src/MemAcc.c:MemAcc_GetJobResult | test_MemAcc_Api.c:test_GetJobResult_AfterInit | —
SYS-MEMACC-STATUS-001 | SWR-MEMACC-070 | MemAcc get job result uninit | src/MemAcc.c:MemAcc_GetJobResult | test_MemAcc_Api.c:test_GetJobResult_Uninit | —
SYS-MEMACC-STATUS-001 | SWR-MEMACC-071 | MemAcc get processed length | src/MemAcc.c:MemAcc_GetProcessedLength | test_MemAcc_Api.c:test_GetProcessedLength | —
SYS-MEMACC-STATUS-001 | SWR-MEMACC-071 | MemAcc get proc len uninit | src/MemAcc.c:MemAcc_GetProcessedLength | test_MemAcc_Api.c:test_GetProcessedLength_Uninit | —
SYS-MEMACC-SEG-001 | SWR-MEMACC-080 | MemAcc segmentation info | src/MemAcc.c:MemAcc_GetSegmentationInfo | test_MemAcc_Api.c:test_GetSegmentationInfo | —
SYS-MEMACC-SEG-001 | SWR-MEMACC-080 | MemAcc seg info null ptr | src/MemAcc.c:MemAcc_GetSegmentationInfo | test_MemAcc_Api.c:test_GetSegmentationInfo_NullPointer | —
SYS-MEMACC-LOCK-001 | SWR-MEMACC-090 | MemAcc lock/unlock | src/MemAcc.c:MemAcc_RequestLock, MemAcc_ReleaseLock | test_MemAcc_Api.c:test_RequestLock_ReleaseLock | SR-008
SYS-MEMACC-LOCK-001 | SWR-MEMACC-090 | MemAcc lock uninit | src/MemAcc.c:MemAcc_RequestLock | test_MemAcc_Api.c:test_RequestLock_Uninit | —
SYS-MEMACC-LOCK-001 | SWR-MEMACC-090 | MemAcc release uninit | src/MemAcc.c:MemAcc_ReleaseLock | test_MemAcc_Api.c:test_ReleaseLock_Uninit | —
SYS-MEMACC-VERSION-001 | SWR-MEMACC-100 | MemAcc version info | src/MemAcc.c:MemAcc_GetVersionInfo | test_MemAcc_Api.c:test_GetVersionInfo | —
SYS-MEMACC-VERSION-001 | SWR-MEMACC-100 | MemAcc version null | src/MemAcc.c:MemAcc_GetVersionInfo | test_MemAcc_Api.c:test_GetVersionInfo_NullPointer | —
SYS-MEMACC-HWSVC-001 | SWR-MEMACC-110 | MemAcc hw specific | src/MemAcc.c:MemAcc_HwSpecificServiceRequest | test_MemAcc_Api.c:test_HwSpecificServiceRequest | —
SYS-MEMACC-MAIN-001 | SWR-MEMACC-120 | MainFunction read complete | src/MemAcc_JobProcessing.c:MemAcc_MainFunction | test_MemAcc_JobProcessing.c:test_MainFunction_ReadComplete | —
SYS-MEMACC-MAIN-001 | SWR-MEMACC-120 | MainFunction write complete | src/MemAcc_JobProcessing.c:MemAcc_MainFunction | test_MemAcc_JobProcessing.c:test_MainFunction_WriteComplete | —
SYS-MEMACC-MAIN-001 | SWR-MEMACC-120 | MainFunction erase complete | src/MemAcc_JobProcessing.c:MemAcc_MainFunction | test_MemAcc_JobProcessing.c:test_MainFunction_EraseComplete | —
SYS-MEMACC-MAIN-001 | SWR-MEMACC-120 | MainFunction blankcheck complete | src/MemAcc_JobProcessing.c:MemAcc_MainFunction | test_MemAcc_JobProcessing.c:test_MainFunction_BlankCheckComplete | —
SYS-MEMACC-MAIN-001 | SWR-MEMACC-120 | MainFunction blankcheck fails | src/MemAcc_JobProcessing.c:MemAcc_MainFunction | test_MemAcc_JobProcessing.c:test_MainFunction_BlankCheckFails | —
SYS-MEMACC-MAIN-001 | SWR-MEMACC-121 | MainFunction compare match | src/MemAcc_JobProcessing.c:MemAcc_Internal_ProcessCompare | test_MemAcc_JobProcessing.c:test_MainFunction_CompareMatch | —
SYS-MEMACC-MAIN-001 | SWR-MEMACC-121 | MainFunction compare mismatch | src/MemAcc_JobProcessing.c:MemAcc_Internal_ProcessCompare | test_MemAcc_JobProcessing.c:test_MainFunction_CompareMismatch | —
SYS-MEMACC-MAIN-001 | SWR-MEMACC-122 | ECC corrected propagation | src/MemAcc_JobProcessing.c:MemAcc_MainFunction | test_MemAcc_JobProcessing.c:test_MainFunction_EccCorrected | SR-015
SYS-MEMACC-MAIN-001 | SWR-MEMACC-122 | ECC uncorrected propagation | src/MemAcc_JobProcessing.c:MemAcc_MainFunction | test_MemAcc_JobProcessing.c:test_MainFunction_EccUncorrected | SR-015
SYS-MEMACC-MAIN-001 | SWR-MEMACC-123 | Driver failure DEM | src/MemAcc_JobProcessing.c:MemAcc_MainFunction | test_MemAcc_JobProcessing.c:test_MainFunction_DriverFailure_DemReport | SR-005
SYS-MEMACC-MAIN-001 | SWR-MEMACC-124 | Cancel during processing | src/MemAcc.c:MemAcc_Cancel | test_MemAcc_JobProcessing.c:test_CancelDuringProcessing | —
SYS-MEMACC-MAIN-001 | SWR-MEMACC-125 | Processed length read | src/MemAcc_JobProcessing.c:MemAcc_MainFunction | test_MemAcc_JobProcessing.c:test_ProcessedLength_Read | —
SYS-MEMACC-MAIN-001 | SWR-MEMACC-125 | Processed length write | src/MemAcc_JobProcessing.c:MemAcc_MainFunction | test_MemAcc_JobProcessing.c:test_ProcessedLength_Write | —
SYS-MEMACC-MAIN-001 | SWR-MEMACC-120 | MainFunction uninit | src/MemAcc_JobProcessing.c:MemAcc_MainFunction | test_MemAcc_JobProcessing.c:test_MainFunction_Uninit | —
SYS-MEMACC-MAIN-001 | SWR-MEMACC-126 | Write verify flash content | src/MemAcc_JobProcessing.c:MemAcc_Internal_DispatchToMemDriver | test_MemAcc_JobProcessing.c:test_WriteAndVerifyFlashContent | —
SYS-MEMACC-MAIN-001 | SWR-MEMACC-126 | Compare after write match | src/MemAcc_JobProcessing.c:MemAcc_Internal_ProcessCompare | test_MemAcc_JobProcessing.c:test_CompareAfterWrite_Match | —
SYS-MEMACC-MAIN-001 | SWR-MEMACC-127 | Compare ECC corrected | src/MemAcc_JobProcessing.c:MemAcc_Internal_ProcessCompare | test_MemAcc_JobProcessing.c:test_Compare_EccCorrected | SR-015
SYS-MEMACC-MAIN-001 | SWR-MEMACC-127 | Compare ECC uncorrected | src/MemAcc_JobProcessing.c:MemAcc_Internal_ProcessCompare | test_MemAcc_JobProcessing.c:test_Compare_EccUncorrected | SR-015
SYS-MEMACC-ADDR-001 | SWR-MEMACC-130 | Validate within bounds | src/MemAcc_AddressArea.c:MemAcc_Internal_ValidateAddress | test_MemAcc_AddressArea.c:test_ValidateAddress_WithinBounds | —
SYS-MEMACC-ADDR-001 | SWR-MEMACC-130 | Validate at boundary | src/MemAcc_AddressArea.c:MemAcc_Internal_ValidateAddress | test_MemAcc_AddressArea.c:test_ValidateAddress_AtBoundary | —
SYS-MEMACC-ADDR-001 | SWR-MEMACC-130 | Validate out of bounds | src/MemAcc_AddressArea.c:MemAcc_Internal_ValidateAddress | test_MemAcc_AddressArea.c:test_ValidateAddress_OutOfBounds | —
SYS-MEMACC-ADDR-001 | SWR-MEMACC-130 | Validate partially OOB | src/MemAcc_AddressArea.c:MemAcc_Internal_ValidateAddress | test_MemAcc_AddressArea.c:test_ValidateAddress_PartiallyOutOfBounds | —
SYS-MEMACC-ADDR-001 | SWR-MEMACC-130 | Validate area 1 | src/MemAcc_AddressArea.c:MemAcc_Internal_ValidateAddress | test_MemAcc_AddressArea.c:test_ValidateAddress_Area1_WithinBounds | —
SYS-MEMACC-ADDR-001 | SWR-MEMACC-130 | Validate area 1 OOB | src/MemAcc_AddressArea.c:MemAcc_Internal_ValidateAddress | test_MemAcc_AddressArea.c:test_ValidateAddress_Area1_OutOfBounds | —
SYS-MEMACC-ADDR-001 | SWR-MEMACC-130 | Validate invalid area | src/MemAcc_AddressArea.c:MemAcc_Internal_ValidateAddress | test_MemAcc_AddressArea.c:test_ValidateAddress_InvalidArea | —
SYS-MEMACC-ADDR-001 | SWR-MEMACC-130 | Validate zero length | src/MemAcc_AddressArea.c:MemAcc_Internal_ValidateAddress | test_MemAcc_AddressArea.c:test_ValidateAddress_ZeroLength | —
SYS-MEMACC-ADDR-002 | SWR-MEMACC-131 | Translate area 0 | src/MemAcc_AddressArea.c:MemAcc_Internal_TranslateAddress | test_MemAcc_AddressArea.c:test_TranslateAddress_Area0 | —
SYS-MEMACC-ADDR-002 | SWR-MEMACC-131 | Translate area 1 | src/MemAcc_AddressArea.c:MemAcc_Internal_TranslateAddress | test_MemAcc_AddressArea.c:test_TranslateAddress_Area1 | —
SYS-MEMACC-ADDR-002 | SWR-MEMACC-131 | Translate invalid area | src/MemAcc_AddressArea.c:MemAcc_Internal_TranslateAddress | test_MemAcc_AddressArea.c:test_TranslateAddress_InvalidArea | —
SYS-MEMACC-ADDR-002 | SWR-MEMACC-131 | Translate zero offset | src/MemAcc_AddressArea.c:MemAcc_Internal_TranslateAddress | test_MemAcc_AddressArea.c:test_TranslateAddress_ZeroOffset | —
SYS-MEMACC-ADDR-003 | SWR-MEMACC-132 | Find area valid | src/MemAcc_AddressArea.c:MemAcc_Internal_FindArea | test_MemAcc_AddressArea.c:test_FindArea_Valid | —
SYS-MEMACC-ADDR-003 | SWR-MEMACC-132 | Find area invalid | src/MemAcc_AddressArea.c:MemAcc_Internal_FindArea | test_MemAcc_AddressArea.c:test_FindArea_Invalid | —
```
