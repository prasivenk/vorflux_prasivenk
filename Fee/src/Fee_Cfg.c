#include "Fee_Cfg.h"
const Fee_BlockConfigType Fee_BlockConfigData[FEE_NUMBER_OF_BLOCKS] = {
    { .BlockNumber=1, .BlockSize=32,  .ImmediateData=FALSE, .NumberOfWriteCycles=100 },
    { .BlockNumber=2, .BlockSize=64,  .ImmediateData=FALSE, .NumberOfWriteCycles=100 },
    { .BlockNumber=3, .BlockSize=128, .ImmediateData=FALSE, .NumberOfWriteCycles=50  },
    { .BlockNumber=4, .BlockSize=256, .ImmediateData=FALSE, .NumberOfWriteCycles=50  },
    { .BlockNumber=5, .BlockSize=16,  .ImmediateData=TRUE,  .NumberOfWriteCycles=200 },
    { .BlockNumber=6, .BlockSize=8,   .ImmediateData=TRUE,  .NumberOfWriteCycles=200 },
    { .BlockNumber=7, .BlockSize=512, .ImmediateData=FALSE, .NumberOfWriteCycles=25  },
    { .BlockNumber=8, .BlockSize=64,  .ImmediateData=FALSE, .NumberOfWriteCycles=100 },
};
const Fee_ConfigType Fee_Config = {
    .BlockConfig=Fee_BlockConfigData, .NumberOfBlocks=FEE_NUMBER_OF_BLOCKS,
    .SectorStartAddress=0x00000000u, .AddressAreaId=0u
};
