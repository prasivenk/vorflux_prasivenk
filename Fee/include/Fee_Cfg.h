#ifndef FEE_CFG_H
#define FEE_CFG_H
#include "Fee_Types.h"
#include "MemAcc.h"

#define FEE_DEV_ERROR_DETECT    STD_ON
#define FEE_VERSION_INFO_API    STD_ON
#define FEE_VIRTUAL_PAGE_SIZE   8u
#define FEE_NUMBER_OF_BLOCKS    8u
#define FEE_NUMBER_OF_SECTORS   4u
#define FEE_SECTOR_SIZE         MEMACC_SECTOR_SIZE

struct Fee_BlockConfigType {
    uint16  BlockNumber;        /* Unique block identifier (1..65534) */
    uint16  BlockSize;          /* Block data size in bytes */
    boolean ImmediateData;      /* TRUE = immediate write priority */
    uint8   NumberOfWriteCycles; /* Configured write cycles (informational) */
};

struct Fee_ConfigType {
    const Fee_BlockConfigType*  BlockConfig;
    uint16                      NumberOfBlocks;
    uint32                      SectorStartAddress;
    MemAcc_AddressAreaIdType    AddressAreaId;  /* R24-11: MemAcc address area (stub ignores this) */
};

extern const Fee_ConfigType Fee_Config;
extern const Fee_BlockConfigType Fee_BlockConfigData[FEE_NUMBER_OF_BLOCKS];

extern void NvM_JobEndNotification(void);
extern void NvM_JobErrorNotification(void);
#endif
