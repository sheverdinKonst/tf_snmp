/******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* smemBobcat2.c
*
* DESCRIPTION:
*       Bobcat2 memory mapping implementation
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 76 $
*
*******************************************************************************/
#include <asicSimulation/SInit/sinit.h>
#include <asicSimulation/SKernel/smem/smemBobcat2.h>
#include <asicSimulation/SKernel/cheetahCommon/sregCheetah.h>
#include <asicSimulation/SKernel/cheetahCommon/sregLion2.h>
#include <asicSimulation/SKernel/suserframes/snetLion.h>
#include <asicSimulation/SKernel/suserframes/snetCheetah3Pcl.h>
#include <asicSimulation/SKernel/suserframes/snetLion2Oam.h>
#include <asicSimulation/SKernel/suserframes/snetLion3Tcam.h>
#include <common/Utils/Math/sMath.h>
#include <asicSimulation/SKernel/sfdb/sfdbCheetah.h>
#include <asicSimulation/SKernel/suserframes/snetLion3Tcam.h>
#include <asicSimulation/SKernel/smain/sRemoteTm.h>

#define  REG_PTR_AND_SIZE_MAC(regArr) \
    (GT_U32*)(void*)((((SMEM_CHT_PP_REGS_UNIT_START_INFO_STC*)(void*)&regArr) + 1)) , ((sizeof(regArr) - sizeof(SMEM_CHT_PP_REGS_UNIT_START_INFO_STC))/sizeof(GT_U32))

static void smemBobcat2UnitGopLed
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
);

static void smemBobcat2UnitGopSmi
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
);

static void smemBobcat2UnitGopIlkn
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
);

static void smemBobcat2InitInterrupts
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
);

#define     BOBCAT2_NUM_SERDESES_CNS        36
#define     BOBCAT2_SERDES_OFFSET_CNS     (0x1000)

/* not used memory */
#define DUMMY_UNITS_BASE_ADDR_CNS(index)              0x70000000 + UNIT_BASE_ADDR_MAC(2*index)

#define BOBCAT2_GOP_64_PORTS_MASK_CNS   0xFFFC0FFF  /* support ports 0..55  in steps of 0x1000 */
#define BOBCAT2_GOP_16_PORTS_MASK_CNS   0xFFFF0FFF  /* support ports 56..71 in steps of 0x1000 */
/* active memory entry for GOP registers */
#define GOP_PORTS_0_71_ACTIVE_MEM_MAC(relativeAddr,readFun,readFunParam,writeFun,writeFunParam)    \
    {relativeAddr, BOBCAT2_GOP_64_PORTS_MASK_CNS, readFun, readFunParam , writeFun, writeFunParam}, \
    {relativeAddr+BOBCAT2_PORT_56_START_OFFSET_CNS, BOBCAT2_GOP_16_PORTS_MASK_CNS, readFun, readFunParam , writeFun, writeFunParam}

#define BOBCAT2_IPCL_TCC_UNIT_BASE_ADDR_CNS     DUMMY_UNITS_BASE_ADDR_CNS(2)
#define BOBCAT2_IPVX_TCC_UNIT_BASE_ADDR_CNS     DUMMY_UNITS_BASE_ADDR_CNS(3)


#define BOBCAT2_GOP_ILKN_UNIT_BASE_ADDR_CNS     0x1F000000
#define SMEM_BOBCAT2_B0_BASE_ADDRESS_SMI_0_CNS  0x54000000
#define SMEM_BOBCAT2_B0_BASE_ADDRESS_SMI_1_CNS  0x55000000
#define SMEM_BOBCAT2_B0_BASE_ADDRESS_SMI_2_CNS  0x56000000
#define SMEM_BOBCAT2_B0_BASE_ADDRESS_SMI_3_CNS  0x57000000
#define SMEM_BOBCAT2_B0_BASE_ADDRESS_LED_0_CNS  0x21000000
#define SMEM_BOBCAT2_B0_BASE_ADDRESS_LED_1_CNS  0x22000000
#define SMEM_BOBCAT2_B0_BASE_ADDRESS_LED_2_CNS  0x23000000
#define SMEM_BOBCAT2_B0_BASE_ADDRESS_LED_3_CNS  0x24000000
#define SMEM_BOBCAT2_B0_BASE_ADDRESS_LED_4_CNS  0x50000000


BUILD_STRING_FOR_UNIT_NAME(UNIT_TMDROP                        );
BUILD_STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO                   );
BUILD_STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO                  );
BUILD_STRING_FOR_UNIT_NAME(UNIT_TM_FCU                        );
BUILD_STRING_FOR_UNIT_NAME(UNIT_TM_INGRESS_GLUE               );
BUILD_STRING_FOR_UNIT_NAME(UNIT_TM_EGRESS_GLUE                );
BUILD_STRING_FOR_UNIT_NAME(UNIT_TMQMAP                        );
BUILD_STRING_FOR_UNIT_NAME(UNIT_GOP                           );
BUILD_STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN                      );
BUILD_STRING_FOR_UNIT_NAME(UNIT_BM                            );
BUILD_STRING_FOR_UNIT_NAME(UNIT_SERDES                        );

/* additional registers that are not in Lion3 */
/*extern*/SMEM_REGISTER_DEFAULT_VALUE_STC bobcat2Additional_registersDefaultValueArr[] =
{
    /*TMDROP - for BC2 only */
     {&STRING_FOR_UNIT_NAME(UNIT_TMDROP),    0x00000020,         0x006d2240,      1,    0x0       }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TMDROP),    0x00000024,         0x00ff6b64,      1,    0x0       }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TMDROP),    0x00000028,         0x0001d138,      1,    0x0       }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TMDROP),    0x00000038,         0x0000ffff,      1,    0x0       }

    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x00000000,         0x800002fc,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x00000004,         0x0000ffff,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x00000008,         0x00000001,     73,    0x4   }
/* Jira   ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x00000552,         0x00000001,      1,    0x0   }*/
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x00000600,         0x00000010,     73,    0x4   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x00000800,         0x00004049,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x00000808,         0x83828180,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x0000080c,         0x87868584,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x00000810,         0x8b8a8988,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x00000814,         0x8f8e8d8c,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x00000818,         0x93929190,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x0000081c,         0x97969594,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x00000820,         0x9b9a9998,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x00000824,         0x9f9e9d9c,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x00000828,         0xa3a2a1a0,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x0000082c,         0xa7a6a5a4,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x00000830,         0xabaaa9a8,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x00000834,         0xafaeadac,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x00000838,         0xb3b2b1b0,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x0000083c,         0xb7b6b5b4,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x00000840,         0xbbbab9b8,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x00000844,         0xbfbebdbc,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x00000848,         0xc3c2c1c0,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x0000084c,         0xc7c6c5c4,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x00000850,         0xcbcac9c8,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x00000854,         0xcfcecdcc,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x00000858,         0xd3d2d1d0,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x0000085c,         0xd7d6d5d4,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x00000860,         0xdbdad9d8,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x00000864,         0xdfdedddc,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x00000868,         0xe3e2e1e0,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x0000086c,         0xe7e6e5e4,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x00000870,         0xebeae9e8,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x00000874,         0xefeeedec,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x00000878,         0xf3f2f1f0,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x0000087c,         0xf7f6f5f4,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x00000880,         0xfbfaf9f8,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x00000884,         0x80fefdfc,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x00000888,         0x84838281,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x0000088c,         0x88878685,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x00000890,         0x8c8b8a89,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x00000894,         0x908f8e8d,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x00000898,         0x94939291,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x0000089c,         0x98979695,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x000008a0,         0x9c9b9a99,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x000008a4,         0xa09f9e9d,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x000008a8,         0xa4a3a2a1,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x000008ac,         0xa8a7a6a5,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x000008b0,         0xacabaaa9,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x000008b4,         0xb0afaead,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x000008b8,         0xb4b3b2b1,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x000008bc,         0xb8b7b6b5,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x000008c0,         0xbcbbbab9,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x000008c4,         0xc0bfbebd,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x000008c8,         0xc4c3c2c1,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x000008cc,         0xc8c7c6c5,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x000008d0,         0xcccbcac9,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x000008d4,         0xd0cfcecd,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x000008d8,         0xd4d3d2d1,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x000008dc,         0xd8d7d6d5,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x000008e0,         0xdcdbdad9,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x000008e4,         0xe0dfdedd,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x000008e8,         0xe4e3e2e1,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x000008ec,         0xe8e7e6e5,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x000008f0,         0xecebeae9,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x000008f4,         0xf0efeeed,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x000008f8,         0xf4f3f2f1,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x000008fc,         0xf8f7f6f5,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x00000900,         0xfcfbfaf9,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x00000904,         0x8180fefd,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x00000908,         0x85848382,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x0000090c,         0x89888786,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x00000910,         0x8d8c8b8a,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x00000914,         0x91908f8e,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x00000918,         0x95949392,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x0000091c,         0x99989796,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x00000920,         0x9d9c9b9a,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x00000924,         0xa1a09f9e,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x00000928,         0xa5a4a3a2,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x0000092c,         0xa9a8a7a6,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x00000930,         0xadacabaa,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x00000934,         0xb1b0afae,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x00000938,         0xb5b4b3b2,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x0000093c,         0xb9b8b7b6,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x00000940,         0xbdbcbbba,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x00000944,         0xc1c0bfbe,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x00000948,         0xc5c4c3c2,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x0000094c,         0xc9c8c7c6,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x00000950,         0xcdcccbca,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x00000954,         0xd1d0cfce,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x00000958,         0xd5d4d3d2,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x00003000,         0x00000bac,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO),   0x00003100,         0x80025540,     73,    0x4   }

    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x00000000,         0xffff0000,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x00000200,         0x000000a0,     64,    0x4  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x00000350,         0x00000011,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x00000360,         0x00000001,     64,    0x4  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x00000500,         0x003c0000,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x00000504,         0x003c00f0,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x00000508,         0x003c01e0,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x0000050c,         0x003c02d0,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x00000510,         0x003c03c0,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x00000514,         0x003c04b0,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x00000518,         0x003c05a0,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x0000051c,         0x003c0690,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x00000520,         0x003c0780,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x00000524,         0x003c0870,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x00000528,         0x003c0960,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x0000052c,         0x003c0a50,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x00000530,         0x003c0b40,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x00000534,         0x003c0c30,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x00000538,         0x003c0d20,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x0000053c,         0x003c0e10,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x00000540,         0x003c0f00,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x00000544,         0x003c0ff0,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x00000548,         0x003c10e0,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x0000054c,         0x003c11d0,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x00000550,         0x003c12c0,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x00000554,         0x003c13b0,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x00000558,         0x003c14a0,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x0000055c,         0x003c1590,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x00000560,         0x003c1680,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x00000564,         0x003c1770,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x00000568,         0x003c1860,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x0000056c,         0x003c1950,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x00000570,         0x003c1a40,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x00000574,         0x003c1b30,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x00000578,         0x003c1c20,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x0000057c,         0x003c1d10,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x00000580,         0x003c1e00,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x00000584,         0x003c1ef0,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x00000588,         0x003c1fe0,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x0000058c,         0x003c20d0,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x00000590,         0x003c21c0,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x00000594,         0x003c22b0,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x00000598,         0x003c23a0,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x0000059c,         0x003c2490,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x000005a0,         0x003c2580,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x000005a4,         0x003c2670,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x000005a8,         0x003c2760,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x000005ac,         0x003c2850,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x000005b0,         0x003c2940,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x000005b4,         0x003c2a30,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x000005b8,         0x003c2b20,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x000005bc,         0x003c2c10,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x000005c0,         0x003c2d00,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x000005c4,         0x003c2df0,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x000005c8,         0x003c2ee0,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x000005cc,         0x003c2fd0,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x000005d0,         0x003c30c0,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x000005d4,         0x003c31b0,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x000005d8,         0x003c32a0,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x000005dc,         0x003c3390,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x000005e0,         0x003c3480,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x000005e4,         0x003c3570,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x000005e8,         0x003c3660,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x000005ec,         0x003c3750,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x000005f0,         0x003c3840,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x000005f4,         0x003c3930,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x000005f8,         0x003c3a20,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x000005fc,         0x003c3b10,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x00001000,         0x00000003,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x00001010,         0x00000001,     65,    0x4  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x00001500,         0x00000840,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x00001510,         0x00000040,     17,    0x100}
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x00001514,         0x00000041,     17,    0x100}
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x00001518,         0x00000042,     17,    0x100}
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x0000151c,         0x00000043,     17,    0x100}
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x00001520,         0x00000044,     17,    0x100}
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x00001524,         0x00000045,     17,    0x100}
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x00001528,         0x00000046,     17,    0x100}
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x0000152c,         0x00000047,     17,    0x100}
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x00001530,         0x00000048,     17,    0x100}
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x00001534,         0x00000049,     17,    0x100}
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x00001538,         0x0000004a,     17,    0x100}
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x0000153c,         0x0000004b,     17,    0x100}
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x00001540,         0x0000004c,     17,    0x100}
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x00001544,         0x0000004d,     17,    0x100}
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x00001548,         0x0000004e,     17,    0x100}
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x0000154c,         0x0000004f,     17,    0x100}
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x00001550,         0x00000050,     17,    0x100}
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x00001554,         0x00000051,     17,    0x100}
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x00001558,         0x00000052,     17,    0x100}
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x0000155c,         0x00000053,     17,    0x100}
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x00001560,         0x00000054,     17,    0x100}
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x00001564,         0x00000055,     17,    0x100}
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x00001568,         0x00000056,     17,    0x100}
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x0000156c,         0x00000057,     17,    0x100}
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x00001570,         0x00000058,     17,    0x100}
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x00001574,         0x00000059,     17,    0x100}
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x00001578,         0x0000005a,     17,    0x100}
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x0000157c,         0x0000005b,     17,    0x100}
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x00001580,         0x0000005c,     17,    0x100}
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x00001584,         0x0000005d,     17,    0x100}
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x00001588,         0x0000005e,     17,    0x100}
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x0000158c,         0x0000005f,     17,    0x100}
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x00001590,         0x00000060,     17,    0x100}
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x00001594,         0x00000061,     17,    0x100}
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x00001598,         0x00000062,     17,    0x100}
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x0000159c,         0x00000063,     17,    0x100}
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x000015a0,         0x00000064,     17,    0x100}
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x000015a4,         0x00000065,     17,    0x100}
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x000015a8,         0x00000066,     17,    0x100}
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x000015ac,         0x00000067,     17,    0x100}
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x000015b0,         0x00000068,     16,    0x100}
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x000015b4,         0x00000069,     16,    0x100}
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x000015b8,         0x0000006a,     16,    0x100}
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x000015bc,         0x0000006b,     16,    0x100}
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x000015c0,         0x0000006c,     16,    0x100}
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x000015c4,         0x0000006d,     16,    0x100}
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x000015c8,         0x0000006e,     16,    0x100}
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x000015cc,         0x0000006f,     16,    0x100}
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x000015d0,         0x00000070,     16,    0x100}
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x000015d4,         0x00000071,     16,    0x100}
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x000015d8,         0x00000072,     16,    0x100}
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x000015dc,         0x00000073,     16,    0x100}
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x000015e0,         0x00000074,     16,    0x100}
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x000015e4,         0x00000075,     16,    0x100}
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x000015e8,         0x00000076,     16,    0x100}
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x000015ec,         0x00000077,     16,    0x100}
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x000015f0,         0x00000078,     16,    0x100}
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x000015f4,         0x00000079,     16,    0x100}
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x000015f8,         0x0000007a,     16,    0x100}
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x000015fc,         0x0000007b,     16,    0x100}
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x00001600,         0x0000007c,     16,    0x100}
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x00001604,         0x0000007d,     16,    0x100}
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x00001608,         0x0000007e,     16,    0x100}
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x0000160c,         0x0000007f,     16,    0x100}
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x00006600,         0x00000034,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ILKN_TX_FIFO),   0x00006800,         0x0000000d,      1,    0x0  }


    ,{&STRING_FOR_UNIT_NAME(UNIT_TM_FCU),    0x00000300,         0x0bb2ecbb,     64,    0x4       }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TM_FCU),    0x00000400,         0x0ba2e8ba,     64,    0x4       }

    ,{&STRING_FOR_UNIT_NAME(UNIT_TM_INGRESS_GLUE),    0x00000004,         0x00000054,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TM_INGRESS_GLUE),    0x00000010,         0x0000ffff,      1,    0x0  }

    ,{&STRING_FOR_UNIT_NAME(UNIT_TM_EGRESS_GLUE),     0x00000000,         0x00000008,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TM_EGRESS_GLUE),     0x00000010,         0x00000001,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TM_EGRESS_GLUE),     0x00000034,         0x0000ffff,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TM_EGRESS_GLUE),     0x00000064,         0x00008ca0,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TM_EGRESS_GLUE),     0x00001400,         0x000fffff,    100,    0x4  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TM_EGRESS_GLUE),     0x0000200c,         0x000fffff,      1,    0x0  }

    ,{&STRING_FOR_UNIT_NAME(UNIT_TMQMAP),     0x00000000,         0x007e0000,      1,    0x0       }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TMQMAP),     0x00000004,         0x76543210,      6,    0x4       }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TMQMAP),     0x00000028,         0x0000ffff,      1,    0x0       }

/* GOP of bc2 */
/*FCA*/
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x00180600,         0x00000011,     56,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x00180604,         0x00002003,     56,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x00180654,         0x00000001,      2,    0x1c,     56,    0x1000     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x00180658,         0x0000c200,     56,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x0018065c,         0x00000180,     56,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x0018066c,         0x00008808,     56,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x00180704,         0x0000ff00,     56,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x00380600,         0x00000011,     16,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x00380604,         0x00002003,     16,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x00380654,         0x00000001,      2,    0x1c,     16,    0x1000     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x00380658,         0x0000c200,     16,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x0038065c,         0x00000180,     16,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x0038066c,         0x00008808,     16,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x00380704,         0x0000ff00,     16,    0x1000                      }
/*Giga*/
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x00000090,         0x0000039a,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x00000000,         0x00008be5,     56,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x00000004,         0x00000003,     56,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x00000008,         0x0000c048,     56,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x0000000c,         0x0000bae8,     56,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x00000014,         0x000008c6,     56,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x00000018,         0x000025cd,     56,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x0000001c,         0x00000052,     56,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x0000002c,         0x0000000c,      2,    0x18,     56,    0x1000     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x00000030,         0x0000c815,     56,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x00000048,         0x00000300,     56,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x00000094,         0x00000001,     56,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x000000c0,         0x00001004,     56,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x000000c4,         0x00000100,     56,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x000000c8,         0x000001fd,     56,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x00200000,         0x00008be5,     16,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x00200004,         0x00000003,     16,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x00200008,         0x0000c048,     16,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x0020000c,         0x0000bae8,     16,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x00200014,         0x000008c6,     16,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x00200018,         0x000025cd,     16,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x0020001c,         0x00000052,     16,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x0020002c,         0x0000000c,      2,    0x18,     16,    0x1000     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x00200030,         0x0000c815,     16,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x00200048,         0x00000300,     16,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x00200094,         0x00000001,     16,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x002000c0,         0x00001004,     16,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x002000c4,         0x00000100,     16,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x002000c8,         0x000001fd,     16,    0x1000                      }
/*XLG*/
    /* manually added to set default for ports 48..55,56..71 as XG and not giga */
/*
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x000f001c,         0x00002000,      8,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x002c001c,         0x00002000,     16,    0x1000                      }
*/
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x000f0000,         0x00000201,      8,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x000f0004,         0x000002f9,      8,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x000f0008,         0x00000400,      8,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x000f0010,         0x00007be3,      8,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x000f001c,         0x00006000,      8,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x000f002c,         0x00000080,      8,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x000f0060,         0x000002ff,      8,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x000f0084,         0x00000210,      8,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x000f0088,         0x0000900c,      8,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x002c0000,         0x00000201,     16,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x002c0004,         0x000002f9,     16,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x002c0008,         0x00000400,     16,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x002c0010,         0x00007be3,     16,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x002c001c,         0x00006000,     16,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x002c002c,         0x00000080,     16,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x002c0060,         0x000002ff,     16,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x002c0084,         0x00000210,     16,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x002c0088,         0x0000900c,     16,    0x1000                      }
/*mpcs*/
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x001b0010,         0x00003fff,      8,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x001b0014,         0x00003c20,      8,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x001b0074,         0x000000a0,      8,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x001b0078,         0x0000001d,      4,    0x4,      8,    0x1000      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x001b0088,         0x00007690,      8,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x001b008c,         0x00003e47,      8,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x001b0090,         0x0000896f,      8,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x001b0094,         0x0000c1b8,      8,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x001b0098,         0x0000c4f0,      8,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x001b009c,         0x000063e6,      8,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x001b00a0,         0x00003b0f,      8,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x001b00a4,         0x00009c19,      8,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x001b00a8,         0x000065c5,      8,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x001b00ac,         0x0000339b,      8,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x001b00b0,         0x00009a3a,      8,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x001b00b4,         0x0000cc64,      8,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x001b00b8,         0x000079a2,      8,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x001b00bc,         0x0000d83d,      8,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x001b00c0,         0x0000865d,      8,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x001b00c4,         0x000027c2,      8,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x001b00d4,         0x0000ffff,      8,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x001b00d8,         0x00000040,      8,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x001b014c,         0x00000800,      8,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x001b015c,         0x0000804f,      8,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x001b0160,         0x0000cab6,      8,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x001b0164,         0x0000b44d,      8,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x001b0168,         0x000003c8,      8,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x001b016c,         0x00008884,      8,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x001b0170,         0x000085a3,      8,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x001b0174,         0x000006bb,      8,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x001b0178,         0x00000349,      8,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x001b017c,         0x000000f0,      8,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x00380010,         0x00003fff,     16,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x00380014,         0x00003c20,     16,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x00380074,         0x000000a0,     16,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x00380078,         0x0000001d,      4,    0x4,     16,    0x1000      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x00380088,         0x00007690,     16,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x0038008c,         0x00003e47,     16,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x00380090,         0x0000896f,     16,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x00380094,         0x0000c1b8,     16,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x00380098,         0x0000c4f0,     16,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x0038009c,         0x000063e6,     16,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x003800a0,         0x00003b0f,     16,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x003800a4,         0x00009c19,     16,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x003800a8,         0x000065c5,     16,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x003800ac,         0x0000339b,     16,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x003800b0,         0x00009a3a,     16,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x003800b4,         0x0000cc64,     16,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x003800b8,         0x000079a2,     16,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x003800bc,         0x0000d83d,     16,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x003800c0,         0x0000865d,     16,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x003800c4,         0x000027c2,     16,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x003800d4,         0x0000ffff,     16,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x003800d8,         0x00000040,     16,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x0038014c,         0x00000800,     16,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x0038015c,         0x0000804f,     16,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x00380160,         0x0000cab6,     16,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x00380164,         0x0000b44d,     16,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x00380168,         0x000003c8,     16,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x0038016c,         0x00008884,     16,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x00380170,         0x000085a3,     16,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x00380174,         0x000006bb,     16,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x00380178,         0x00000349,     16,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x0038017c,         0x000000f0,     16,    0x1000                      }
/*xpcs*/
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x001b0400,         0x00000046,      4,    0x2000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x001b0404,         0x000000c0,      4,    0x2000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x001b0408,         0x0000b0a3,      4,    0x2000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x001b040c,         0x000007d0,      4,    0x2000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x001b0440,         0x0000ff00,      4,    0x2000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x001b0450,         0x00000003,      6,    0x44,      4,    0x2000     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x00380400,         0x00000046,      8,    0x2000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x00380404,         0x000000c0,      8,    0x2000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x00380408,         0x0000b0a3,      8,    0x2000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x0038040c,         0x000007d0,      8,    0x2000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x00380440,         0x0000ff00,      8,    0x2000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x00380450,         0x00000003,      6,    0x44,      8,    0x2000     }
/*ilkn*/
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x00000000,         0x00002201,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x00000008,         0x1f51003f,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x0000000c,         0x10010000,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x00000010,         0x00000b0b,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x00000014,         0x00020201,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x00000018,         0x00060040,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x00000024,         0x00000001,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x00000030,         0x00000004,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x00000038,         0x0000ff00,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x00000040,         0x00010000,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x00000048,         0x0000001e,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x00000050,         0x001e0014,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x00000060,         0x0000ff00,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x00000068,         0x00003f00,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x00000200,         0x003f003f,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x00000204,         0x00ffffff,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x00000208,         0x00000007,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x00000214,         0x00000001,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x00000218,         0x00003311,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x0000023c,         0x0000005b,      1,    0x0                    }
/*ilkn wrp*/
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x00000810,         0x0000000c,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x00000820,         0x0000d00c,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x00000840,         0x0000009c,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x00000848,         0x1ff558aa,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x0000084c,         0x0a08003c,      1,    0x0                    }
/*pr*/
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x00001800,         0x0a010040,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x00001808,         0x00a00000,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x0000180c,         0x00a00140,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x00001810,         0x00a00280,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x00001814,         0x00a003c0,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x00001818,         0x00a00500,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x0000181c,         0x00a00640,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x00001820,         0x00a00780,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x00001824,         0x00a008c0,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x00001828,         0x00a00a00,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x0000182c,         0x00a00b40,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x00001830,         0x00a00c80,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x00001834,         0x00a00dc0,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x00001838,         0x00a00f00,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x0000183c,         0x00a01040,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x00001840,         0x00a01180,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x00001844,         0x00a012c0,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x00001848,         0x00a01400,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x0000184c,         0x00a01540,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x00001850,         0x00a01680,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x00001854,         0x00a017c0,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x00001858,         0x00a01900,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x0000185c,         0x00a01a40,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x00001860,         0x00a01b80,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x00001864,         0x00a01cc0,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x00001868,         0x00a01e00,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x0000186c,         0x00a01f40,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x00001870,         0x00a02080,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x00001874,         0x00a021c0,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x00001878,         0x00a02300,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x0000187c,         0x00a02440,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x00001880,         0x00a02580,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x00001884,         0x00a026c0,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x00001888,         0x00a02800,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x0000188c,         0x00a02940,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x00001890,         0x00a02a80,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x00001894,         0x00a02bc0,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x00001898,         0x00a02d00,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x0000189c,         0x00a02e40,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x000018a0,         0x00a02f80,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x000018a4,         0x00a030c0,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x000018a8,         0x00a03200,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x000018ac,         0x00a03340,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x000018b0,         0x00a03480,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x000018b4,         0x00a035c0,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x000018b8,         0x00a03700,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x000018bc,         0x00a03840,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x000018c0,         0x00a03980,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x000018c4,         0x00a03ac0,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x000018c8,         0x00a03c00,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x000018cc,         0x00a03d40,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x000018d0,         0x00a03e80,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x000018d4,         0x00a03fc0,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x000018d8,         0x00a04100,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x000018dc,         0x00a04240,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x000018e0,         0x00a04380,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x000018e4,         0x00a044c0,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x000018e8,         0x00a04600,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x000018ec,         0x00a04740,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x000018f0,         0x00a04880,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x000018f4,         0x00a049c0,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x000018f8,         0x00a04b00,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x000018fc,         0x00a04c40,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x00001900,         0x00a04d80,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x00001904,         0x00a04ec0,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x00001c00,         0x00248822,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x00001c04,         0x00000555,      2,    0x8                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x00001c08,         0x00014019,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x00001c10,         0x00008000,      1,    0x0                    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),            0x00001c14,         0x000000c0,      1,    0x0                    }

    ,{&STRING_FOR_UNIT_NAME(UNIT_BM),             0x00000030,         0x3fff0000,      3,    0x4    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_BM),             0x00000420,         0x00031490,      6,    0x4    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_BM),             0x00000440,         0x000024c9,      6,    0x4    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_BM),             0x000004a0,         0x00080000,      3,    0x4    }

    ,{NULL,            0,         0x00000000,      0,    0x0      }
};

/* additional registers for Bobcat2 B0 */
/*extern*/ SMEM_REGISTER_DEFAULT_VALUE_STC bobcat2B0Additional_registersDefaultValueArr[] =
{
     {&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x00000090,         0x0000ff9a,      1,    0x0                         }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x00000018,         0x00004b4d,     56,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x00200018,         0x00004b4d,     16,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x00000014,         0x000008c4,     56,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x00200014,         0x000008c4,     16,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x000f0030,         0x000007ec,      8,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x002c0030,         0x000007ec,     16,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x0018062c,         0x000000ff,     56,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x000f0080,         0x00001000,      8,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x002c0080,         0x00001000,     16,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x0018062c,         0x000000ff,     56,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x000f0084,         0x00001210,      8,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x002c0084,         0x00001210,     16,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x0018062c,         0x000000ff,     56,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x00180704,         0x000000ff,     56,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x00380704,         0x000000ff,     16,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x00180718,         0x0000ff00,     56,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x00380718,         0x0000ff00,     16,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x000000d4,         0x000000ff,     56,    0x1000                      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x002000d4,         0x000000ff,     16,    0x1000                      }

    /*ilkn wrp*/
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP_ILKN),       0x00000810,         0x0000fc0c,      1,    0x0                         }
    ,{NULL,            0,         0x00000000,      0,    0x0      }
};

/* serdes registers for Bobcat2 */
/*extern*/ SMEM_REGISTER_DEFAULT_VALUE_STC bobcat2Serdes_registersDefaultValueArr[] =
{
    /* SERDES registers */
     {&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x00000000,         0x00000c80,   BOBCAT2_NUM_SERDESES_CNS  ,   BOBCAT2_SERDES_OFFSET_CNS     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x00000004,         0x00000008,   BOBCAT2_NUM_SERDESES_CNS  ,   BOBCAT2_SERDES_OFFSET_CNS     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x00000018,         0x0000001c,   BOBCAT2_NUM_SERDESES_CNS  ,   BOBCAT2_SERDES_OFFSET_CNS     }
    /* took registers of offset 0x200 of Lion2 and convert to offset 0x800 */
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x0000080c,         0x0000000f,   BOBCAT2_NUM_SERDESES_CNS  ,   BOBCAT2_SERDES_OFFSET_CNS     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x00000810,         0x00000005,   BOBCAT2_NUM_SERDESES_CNS  ,   BOBCAT2_SERDES_OFFSET_CNS     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x00000814,         0x0000003c,   BOBCAT2_NUM_SERDESES_CNS  ,   BOBCAT2_SERDES_OFFSET_CNS     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x00000818,         0x00001004,   BOBCAT2_NUM_SERDESES_CNS  ,   BOBCAT2_SERDES_OFFSET_CNS     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x0000081c,         0x00000520,   BOBCAT2_NUM_SERDESES_CNS  ,   BOBCAT2_SERDES_OFFSET_CNS     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x00000828,         0x00000030,   BOBCAT2_NUM_SERDESES_CNS  ,   BOBCAT2_SERDES_OFFSET_CNS     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x0000082c,         0x00000660,   BOBCAT2_NUM_SERDESES_CNS  ,   BOBCAT2_SERDES_OFFSET_CNS     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x00000830,         0x00004000,   BOBCAT2_NUM_SERDESES_CNS  ,   BOBCAT2_SERDES_OFFSET_CNS     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x00000834,         0x00000003,   BOBCAT2_NUM_SERDESES_CNS  ,   BOBCAT2_SERDES_OFFSET_CNS     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x00000840,         0x00003087,   BOBCAT2_NUM_SERDESES_CNS  ,   BOBCAT2_SERDES_OFFSET_CNS     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x00000844,         0x000000f0,   BOBCAT2_NUM_SERDESES_CNS  ,   BOBCAT2_SERDES_OFFSET_CNS     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x00000848,         0x0000000f,   BOBCAT2_NUM_SERDESES_CNS  ,   BOBCAT2_SERDES_OFFSET_CNS     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x0000084c,         0x00000400,   BOBCAT2_NUM_SERDESES_CNS  ,   BOBCAT2_SERDES_OFFSET_CNS     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x00000850,         0x00008060,   BOBCAT2_NUM_SERDESES_CNS  ,   BOBCAT2_SERDES_OFFSET_CNS     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x00000854,         0x00005503,   BOBCAT2_NUM_SERDESES_CNS  ,   BOBCAT2_SERDES_OFFSET_CNS     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x0000085c,         0x00000015,   BOBCAT2_NUM_SERDESES_CNS  ,   BOBCAT2_SERDES_OFFSET_CNS     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x00000860,         0x00004600,   BOBCAT2_NUM_SERDESES_CNS  ,   BOBCAT2_SERDES_OFFSET_CNS     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x00000878,         0x00000013,   BOBCAT2_NUM_SERDESES_CNS  ,   BOBCAT2_SERDES_OFFSET_CNS     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x0000087c,         0x000000a5,   BOBCAT2_NUM_SERDESES_CNS  ,   BOBCAT2_SERDES_OFFSET_CNS     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x00000880,         0x00004800,   BOBCAT2_NUM_SERDESES_CNS  ,   BOBCAT2_SERDES_OFFSET_CNS     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x00000884,         0x00000600,   BOBCAT2_NUM_SERDESES_CNS  ,   BOBCAT2_SERDES_OFFSET_CNS     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x0000088c,         0x00000240,   BOBCAT2_NUM_SERDESES_CNS  ,   BOBCAT2_SERDES_OFFSET_CNS     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x00000894,         0x0000000a,   BOBCAT2_NUM_SERDESES_CNS  ,   BOBCAT2_SERDES_OFFSET_CNS     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x00000938,         0x00000f6a,   BOBCAT2_NUM_SERDESES_CNS  ,   BOBCAT2_SERDES_OFFSET_CNS     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x0000093c,         0x000000cc,   BOBCAT2_NUM_SERDESES_CNS  ,   BOBCAT2_SERDES_OFFSET_CNS     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x00000940,         0x000052ea,   BOBCAT2_NUM_SERDESES_CNS  ,   BOBCAT2_SERDES_OFFSET_CNS     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x00000944,         0x000000cc,   BOBCAT2_NUM_SERDESES_CNS  ,   BOBCAT2_SERDES_OFFSET_CNS     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x00000948,         0x0000562a,   BOBCAT2_NUM_SERDESES_CNS  ,   BOBCAT2_SERDES_OFFSET_CNS     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x0000094c,         0x000000cc,   BOBCAT2_NUM_SERDESES_CNS  ,   BOBCAT2_SERDES_OFFSET_CNS     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x00000954,         0x000000cc,   BOBCAT2_NUM_SERDESES_CNS  ,   BOBCAT2_SERDES_OFFSET_CNS     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x00000958,         0x00000080,   BOBCAT2_NUM_SERDESES_CNS  ,   BOBCAT2_SERDES_OFFSET_CNS     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x0000095c,         0x00004000,   BOBCAT2_NUM_SERDESES_CNS  ,   BOBCAT2_SERDES_OFFSET_CNS     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x00000960,         0x00000081,   BOBCAT2_NUM_SERDESES_CNS  ,   BOBCAT2_SERDES_OFFSET_CNS     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x00000964,         0x00005555,   BOBCAT2_NUM_SERDESES_CNS  ,   BOBCAT2_SERDES_OFFSET_CNS     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x00000968,         0x0000f640,   BOBCAT2_NUM_SERDESES_CNS  ,   BOBCAT2_SERDES_OFFSET_CNS     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x0000096c,         0x00008000,   BOBCAT2_NUM_SERDESES_CNS  ,   BOBCAT2_SERDES_OFFSET_CNS     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x000009ac,         0x00000030,   BOBCAT2_NUM_SERDESES_CNS  ,   BOBCAT2_SERDES_OFFSET_CNS     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x000009b0,         0x00001000,   BOBCAT2_NUM_SERDESES_CNS  ,   BOBCAT2_SERDES_OFFSET_CNS     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x000009c0,         0x00005502,   BOBCAT2_NUM_SERDESES_CNS  ,   BOBCAT2_SERDES_OFFSET_CNS     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x000009cc,         0x00002800,   BOBCAT2_NUM_SERDESES_CNS  ,   BOBCAT2_SERDES_OFFSET_CNS     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x000009e0,         0x00000080,   BOBCAT2_NUM_SERDESES_CNS  ,   BOBCAT2_SERDES_OFFSET_CNS     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x000009f0,         0x0000cccc,   BOBCAT2_NUM_SERDESES_CNS  ,   BOBCAT2_SERDES_OFFSET_CNS     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x000009f4,         0x0000cc00,   BOBCAT2_NUM_SERDESES_CNS  ,   BOBCAT2_SERDES_OFFSET_CNS     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x000009f8,         0x00008000,   BOBCAT2_NUM_SERDESES_CNS  ,   BOBCAT2_SERDES_OFFSET_CNS     }

/*  for SERDESs use values like in Lion2 simulation*/
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x00000000,         0x00000001,   BOBCAT2_NUM_SERDESES_CNS  ,   BOBCAT2_SERDES_OFFSET_CNS     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x00000018,         0x0000001c,   BOBCAT2_NUM_SERDESES_CNS  ,   BOBCAT2_SERDES_OFFSET_CNS     }
    /* took registers of offset 0x200 of Lion2 and convert to offset 0x800 */
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x0000093c,         0x00000017,   BOBCAT2_NUM_SERDESES_CNS  ,   BOBCAT2_SERDES_OFFSET_CNS     }
    ,{NULL,            0,         0x00000000,      0,    0x0      }
};

/* serdes registers for Bobcat2 */
/*extern*/ SMEM_REGISTER_DEFAULT_VALUE_STC bobcat2B0Serdes_registersDefaultValueArr[] =
{
    /* SERDES registers */
    {&STRING_FOR_UNIT_NAME(UNIT_SERDES),         0x00000004,         0x00000801,   BOBCAT2_NUM_SERDESES_CNS  ,   BOBCAT2_SERDES_OFFSET_CNS     }
   ,{NULL,            0,         0x00000000,      0,    0x0      }
};

extern SMEM_REGISTER_DEFAULT_VALUE_STC sip5_registersDefaultValueArr[];
extern SMEM_REGISTER_DEFAULT_VALUE_STC sip5_only_registersDefaultValueArr[];
extern SMEM_REGISTER_DEFAULT_VALUE_STC sip5_10_registersDefaultValueArr[];
extern SMEM_REGISTER_DEFAULT_VALUE_STC sip5_gop_tai_registersDefaultValueArr[];

static SMEM_LINK_LIST_REGISTER_DEFAULT_VALUE_STC  bc2B0Serdes_RegistersDefaults = 
    {bobcat2B0Serdes_registersDefaultValueArr,      NULL};

static SMEM_LINK_LIST_REGISTER_DEFAULT_VALUE_STC  bc2serdes_RegistersDefaults = 
    {bobcat2Serdes_registersDefaultValueArr,        &bc2B0Serdes_RegistersDefaults};

static SMEM_LINK_LIST_REGISTER_DEFAULT_VALUE_STC  element1_RegistersDefaults =
    {bobcat2Additional_registersDefaultValueArr,    &bc2serdes_RegistersDefaults};

static SMEM_LINK_LIST_REGISTER_DEFAULT_VALUE_STC  element0_RegistersDefaults =
    {sip5_only_registersDefaultValueArr,    &element1_RegistersDefaults};

static SMEM_LINK_LIST_REGISTER_DEFAULT_VALUE_STC  sip5_gop_tai_RegistersDefaults =
    {sip5_gop_tai_registersDefaultValueArr,&element0_RegistersDefaults};

static SMEM_LINK_LIST_REGISTER_DEFAULT_VALUE_STC   linkListElementsBc2A0_RegistersDefaults =
    {sip5_registersDefaultValueArr,    &sip5_gop_tai_RegistersDefaults};

static SMEM_LINK_LIST_REGISTER_DEFAULT_VALUE_STC  element2_RegistersDefaults =
    {sip5_10_registersDefaultValueArr, &element1_RegistersDefaults};

static SMEM_LINK_LIST_REGISTER_DEFAULT_VALUE_STC  element3_RegistersDefaults =
    {bobcat2B0Additional_registersDefaultValueArr, &element2_RegistersDefaults};

static SMEM_LINK_LIST_REGISTER_DEFAULT_VALUE_STC  bc2_b0_sip5_gop_tai_RegistersDefaults =
    {sip5_gop_tai_registersDefaultValueArr,&element3_RegistersDefaults};

static SMEM_LINK_LIST_REGISTER_DEFAULT_VALUE_STC   linkListElementsBc2B0_RegistersDefaults =
    {sip5_registersDefaultValueArr,    &bc2_b0_sip5_gop_tai_RegistersDefaults};


/* bind the MG unit to it's active mem */
static void bindUnitMgActiveMem(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    START_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)

    ACTIVE_MEM_MG_COMMON_MAC ,

    /* XSMI Management Register */
    {0x00040000, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteXSmii, 0},
    /* XSMI1 Management Register */
    {0x00042000, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteXSmii, 0},

    END_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)
}

/* bind the BM unit to it's active mem */
static void bindUnitBmActiveMem(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    START_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)

    /* BM General Cause Reg1 Register */
    {0x00000300, SMEM_FULL_MASK_CNS, smemChtActiveReadIntrCauseReg, 0, smemChtActiveWriteIntrCauseReg, 0},
    /* Write BM General Mask Reg1 Register */
    {0x00000304, SMEM_FULL_MASK_CNS, NULL, 0, smemChtActiveGenericWriteInterruptsMaskReg, 0},

    END_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)
}

/* bind the MG unit to it's active mem */
static void bindUnitMgActiveMem_sip5_10(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    START_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)

    ACTIVE_MEM_MG_COMMON_MAC ,

    /* XSMI Management Register */
    {0x00030000, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteXSmii, 0},
    /* XSMI1 Management Register */
    {0x00032000, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteXSmii, 0},

    END_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)
}


/* bind the GOP unit to it's active mem */
static void bindUnitGopActiveMem(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    START_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)

/*GOP*/
    /* Port<n> Auto-Negotiation Configuration Register */
    GOP_PORTS_0_71_ACTIVE_MEM_MAC(0x0000000C,NULL, 0 , smemChtActiveWriteForceLinkDown, 0),
    /* Port<n> Interrupt Cause Register  */
    GOP_PORTS_0_71_ACTIVE_MEM_MAC(0x00000020,smemChtActiveReadIntrCauseReg, 18, smemChtActiveWriteIntrCauseReg, 0),
    /* Tri-Speed Port<n> Interrupt Mask Register */
    GOP_PORTS_0_71_ACTIVE_MEM_MAC(0x00000024,NULL, 0, smemChtActiveWritePortInterruptsMaskReg, 0),
    /* Port MAC Control Register0 */
    GOP_PORTS_0_71_ACTIVE_MEM_MAC(0x000C0000,NULL, 0 , smemChtActiveWriteForceLinkDownXg, 0),
    /*Port LPI Control 1 */
    GOP_PORTS_0_71_ACTIVE_MEM_MAC(0x000000C4, NULL, 0 , smemBobcat2ActiveWritePortLpiCtrlReg , 1),
    /*Port LPI Status */
    GOP_PORTS_0_71_ACTIVE_MEM_MAC(0x000000CC, smemBobcat2ActiveReadPortLpiStatusReg, 0 , smemChtActiveWriteToReadOnlyReg , 0),

    /* XG Port<n> Interrupt Cause Register  */
    GOP_PORTS_0_71_ACTIVE_MEM_MAC(0x000C0014,smemChtActiveReadIntrCauseReg, 29, smemChtActiveWriteIntrCauseReg, 0),
    /* XG Port<n> Interrupt Mask Register */
    GOP_PORTS_0_71_ACTIVE_MEM_MAC(0x000C0018, NULL, 0, smemChtActiveWritePortInterruptsMaskReg, 0),
    /* stack gig ports - Port<n> Interrupt Cause Register  */
    GOP_PORTS_0_71_ACTIVE_MEM_MAC(0x000C0020, smemChtActiveReadIntrCauseReg, 18, smemChtActiveWriteIntrCauseReg, 0),
    /* stack gig ports - Tri-Speed Port<n> Interrupt Mask Register */
    GOP_PORTS_0_71_ACTIVE_MEM_MAC(0x000C0024, NULL, 0, smemChtActiveWritePortInterruptsMaskReg, 0),
    /* stack gig ports - Port<n> Auto-Negotiation Configuration Register */
    GOP_PORTS_0_71_ACTIVE_MEM_MAC(0x000C000C, NULL, 0 , smemChtActiveWriteForceLinkDown, 0),
    /*Port MAC Control Register3*/
    GOP_PORTS_0_71_ACTIVE_MEM_MAC(0x000C001C, NULL, 0 , smemXcatActiveWriteMacModeSelect, 0),

/* also in GOP unit */
    /* TAI subunit */
    /* Time Counter Function Configuration 0 - Function Trigger */
    {0x00000A00 + 0x10, 0xFF000FFF, NULL, 0 , smemLion3ActiveWriteTodFuncConfReg, 0},
    /* time Capture Value 0 Frac Low */
    {0x00000A00 + 0x84, 0xFF000FFF, smemBobcat2ActiveReadTodTimeCaptureValueFracLow, 0 , smemChtActiveWriteToReadOnlyReg, 0},
    /* time Capture Value 1 Frac Low */
    {0x00000A00 + 0xA0, 0xFF000FFF, smemBobcat2ActiveReadTodTimeCaptureValueFracLow, 1 , smemChtActiveWriteToReadOnlyReg, 0},

    /* PTP subunit */
    /* PTP Interrupt Cause Register */
    {0x00000800, 0xFF000FFF,
        smemChtActiveReadIntrCauseReg, 9, smemChtActiveWriteIntrCauseReg, 0},
    /* PTP Interrupt Mask Register */
    {0x00000800 + 0x04, 0xFF000FFF,
        NULL, 0, smemLion3ActiveWriteGopPtpInterruptsMaskReg, 0},

    /* PTP General Control */
    {0x00000800 + 0x08, 0xFF000FFF, NULL, 0, smemLion3ActiveWriteGopPtpGeneralCtrlReg, 0},

    /* PTP TX Timestamp Queue0 reg2 */
    {0x00000800 + 0x14, 0xFF000FFF, smemLion3ActiveReadGopPtpTxTsQueueReg2Reg, 0, NULL, 0},
    /* PTP TX Timestamp Queue1 reg2 */
    {0x00000800 + 0x20, 0xFF000FFF, smemLion3ActiveReadGopPtpTxTsQueueReg2Reg, 0, NULL, 0},

    END_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)
}

/* bind the L2I unit to it's active mem */
static void bindUnitL2iActiveMem(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    START_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)

/*L2I*/
    /* Bridge Interrupt Cause Register */
    {0x00002100, SMEM_FULL_MASK_CNS,
        smemChtActiveReadIntrCauseReg, 14, smemChtActiveWriteIntrCauseReg, 0},
    /* Bridge Interrupt Mask Register */
    {0x00002104, SMEM_FULL_MASK_CNS,
        NULL, 0, smemChtActiveWriteBridgeInterruptsMaskReg, 0},

    /* Security Breach Status Register3 */
    {0x0004140C, SMEM_FULL_MASK_CNS, smemChtActiveReadBreachStatusReg, 0, NULL, 0},

    /*hostIncomingPktsCount;               */
    {0x00001510, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},
    /*hostOutgoingPktsCount;               */
    {0x00001514, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},
    /*hostOutgoingMcPktCount;              */
    {0x00001520, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},
    /*hostOutgoingBcPktCount;              */
    {0x00001524, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},
    /*matrixSourceDestinationPktCount;     */
    {0x00001528, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},
    /*setVLANIngrFilteredPktCount[n]*/
    {0x0000153c, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},
    {0x0000155c, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},
    /*setIncomingPktCount[n]        */
    {0x00001538, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},
    {0x00001558, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},
    /*setSecurityFilteredPktCount[n]*/
    {0x00001540, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},
    {0x00001560, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},
    /*setBridgeFilteredPktCount[n]  */
    {0x00001544, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},
    {0x00001564, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},

    END_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)
}

/* bind the FDB unit to it's active mem */
static void bindUnitFdbActiveMem(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    START_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)

/*FDB*/
    /* Message to CPU register  */
    {0x00000090, SMEM_FULL_MASK_CNS, smemChtActiveReadMsg2Cpu, 0 , NULL,0},
    /* Mac Table Access Control Register */
    {0x00000130, SMEM_FULL_MASK_CNS, NULL, 0 , smemLion3ActiveWriteFdbMsg,0},
    /* Message From CPU Management */
    {0x000000c0, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteNewFdbMsgFromCpu, 0},

    /* MAC Table Action general Register */
    {0x00000020, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteFdbActionTrigger, 0},
    /* FDB Global Configuration 1 register */
    {0x00000004, SMEM_FULL_MASK_CNS, NULL, 0 , smemLion3ActiveWriteFDBGlobalCfg1Reg, 0},

    {0x000001b0, SMEM_FULL_MASK_CNS,
        smemChtActiveReadIntrCauseReg, 17, smemChtActiveWriteIntrCauseReg, 0},
    /* MAC Table Interrupt Mask Register */
    {0x000001b4, SMEM_FULL_MASK_CNS,
        NULL, 0, smemChtActiveWriteMacInterruptsMaskReg, 0},

    /* FDB Counters Update Control */
    {0x00000340, SMEM_FULL_MASK_CNS, NULL, 0, smemLion3ActiveWriteFDBCountersUpdateControlReg,0},
    /* FDB Counters Update Control */
    {0x00000280, SMEM_FULL_MASK_CNS, NULL, 0, smemLion3ActiveWriteFDBCountersControlReg,0},

    END_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)
}

/* bind the MIB unit to it's active mem */
static void bindUnitMibActiveMem(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    START_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)

/*MIB*/
    /* XG port MAC MIB Counters */
    {0x00000000, SMEM_BOBCAT2_XG_MIB_COUNT_MSK_CNS, smemLion2ActiveReadMsmMibCounters, 0, NULL,0},
    {0x00800000, SMEM_BOBCAT2_XG_MIB_COUNT_MSK_CNS, smemLion2ActiveReadMsmMibCounters, 0, NULL,0},

    END_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)
}

/* bind the SERDES unit to it's active mem */
static void bindUnitSerdesActiveMem(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    START_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)

/*SERDES*/
    /* took registers of offset 0x200 of Lion2 and convert to offset 0x800 */
    /* COMPHY_H %t Registers/KVCO Calibration Control (0x00000800 + t*0x1000: where t (0-35) represents SERDES) */
    {0x00000808, 0xFFF00FFF, NULL, 0, smemLion2ActiveWriteKVCOCalibrationControlReg, 0},

    END_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)
}

#define TXQ_QUEQUE_EGR_PACKET_COUNTERS_CLEAR_ON_READ_ACTIVE_MEM_MAC \
    {0x00093210, 0xFFFFFFF0, smemChtActiveReadCntrs, 0, NULL,0},    \
    {0x00093220, 0xFFFFFFF0, smemChtActiveReadCntrs, 0, NULL,0},    \
    {0x00093230, 0xFFFFFFF0, smemChtActiveReadCntrs, 0, NULL,0},    \
    {0x00093240, 0xFFFFFFF0, smemChtActiveReadCntrs, 0, NULL,0},    \
    {0x00093250, 0xFFFFFFF0, smemChtActiveReadCntrs, 0, NULL,0},    \
    {0x00093260, 0xFFFFFFF0, smemChtActiveReadCntrs, 0, NULL,0},    \
    {0x00093270, 0xFFFFFFF0, smemChtActiveReadCntrs, 0, NULL,0},    \
    {0x00093280, 0xFFFFFFF0, smemChtActiveReadCntrs, 0, NULL,0},    \
    {0x00093290, 0xFFFFFFF0, smemChtActiveReadCntrs, 0, NULL,0}

#define TXQ_QUEQUE_ACTIVE_MEM_MAC \
    {0x00090008, SMEM_FULL_MASK_CNS, smemChtActiveReadIntrCauseReg, 0, smemChtActiveWriteIntrCauseReg, 0}, \
    {0x00090040, SMEM_FULL_MASK_CNS, smemChtActiveReadIntrCauseReg, 0, smemChtActiveWriteIntrCauseReg, 0}, \
    {0x00090048, SMEM_FULL_MASK_CNS, smemChtActiveReadIntrCauseReg, 0, smemChtActiveWriteIntrCauseReg, 0}, \
    {0x00090050, SMEM_FULL_MASK_CNS, smemChtActiveReadIntrCauseReg, 0, smemChtActiveWriteIntrCauseReg, 0}, \
    {0x0009000C, SMEM_FULL_MASK_CNS, NULL, 0, smemChtActiveGenericWriteInterruptsMaskReg, 0}, \
    {0x00090044, SMEM_FULL_MASK_CNS, NULL, 0, smemChtActiveGenericWriteInterruptsMaskReg, 0}, \
    {0x0009004C, SMEM_FULL_MASK_CNS, NULL, 0, smemChtActiveGenericWriteInterruptsMaskReg, 0}, \
    {0x00090054, SMEM_FULL_MASK_CNS, NULL, 0, smemChtActiveGenericWriteInterruptsMaskReg, 0}

/* bind the TXQ_QUEUE unit to it's active mem */
static void bindUnitTxqQueueActiveMem(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    if(devObjPtr->errata.txqEgressMibCountersNotClearOnRead)
    {
        START_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)
            TXQ_QUEQUE_ACTIVE_MEM_MAC,
        END_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)
    }
    else
    {
        START_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)
            TXQ_QUEQUE_ACTIVE_MEM_MAC,
            TXQ_QUEQUE_EGR_PACKET_COUNTERS_CLEAR_ON_READ_ACTIVE_MEM_MAC ,
       END_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)
    }
}

static void bindUnitTxqQcnActiveMem(
IN SKERNEL_DEVICE_OBJECT * devObjPtr,
INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    START_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)
    /* QCN interrupt cause */
    {0x00000100, SMEM_FULL_MASK_CNS, smemChtActiveReadIntrCauseReg, 0, smemChtActiveWriteIntrCauseReg, 0},
    {0x00000110, SMEM_FULL_MASK_CNS, NULL, 0, smemChtActiveGenericWriteInterruptsMaskReg, 0},
    END_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)

}

static void bindUnitCpfcActiveMem(
IN SKERNEL_DEVICE_OBJECT * devObjPtr,
INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    START_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)

    /* PFC parity interrupt cause */
    {0x00001310, SMEM_FULL_MASK_CNS, smemChtActiveReadIntrCauseReg, 0, smemChtActiveWriteIntrCauseReg, 0},
    {0x00001314, SMEM_FULL_MASK_CNS, NULL, 0, smemChtActiveGenericWriteInterruptsMaskReg, 0},
    END_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)

}

static void bindUnitTxDmaActiveMem(
IN SKERNEL_DEVICE_OBJECT * devObjPtr,
INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    START_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)

    /*TxDMA General Interrupt Cause Reg1*/
    {0x00002038, SMEM_FULL_MASK_CNS, smemChtActiveReadIntrCauseReg, 0, smemChtActiveWriteIntrCauseReg, 0},
    /*Write TX DMA General Interrupt Mask Reg1*/
    {0x0000203C, SMEM_FULL_MASK_CNS, NULL, 0, smemChtActiveGenericWriteInterruptsMaskReg, 0},

    END_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)

}

static void bindUnitTxFifoActiveMem(
IN SKERNEL_DEVICE_OBJECT * devObjPtr,
INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    START_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)

    /*TxFIFO General Cause Reg1*/
    {0x00000400, SMEM_FULL_MASK_CNS, smemChtActiveReadIntrCauseReg, 0, smemChtActiveWriteIntrCauseReg, 0},
    /*Write TxFIFO General Interrupt Mask Reg1*/
    {0x00000404, SMEM_FULL_MASK_CNS, NULL, 0, smemChtActiveGenericWriteInterruptsMaskReg, 0},

    END_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)

}

static void bindUnitEthTxFifoActiveMem(
IN SKERNEL_DEVICE_OBJECT * devObjPtr,
INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    START_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)

    /*Ethernet TxFIFO General Cause Reg1*/
    {0x00000400, SMEM_FULL_MASK_CNS, smemChtActiveReadIntrCauseReg, 0, smemChtActiveWriteIntrCauseReg, 0},
    /*Write Ethernet TxFIFO General Interrupt Mask Reg1*/
    {0x00000404, SMEM_FULL_MASK_CNS, NULL, 0, smemChtActiveGenericWriteInterruptsMaskReg, 0},

    END_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)

}

static void bindUnitMppmActiveMem(
IN SKERNEL_DEVICE_OBJECT * devObjPtr,
INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    START_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)

    /*ECC Interrupt Cause Register*/
    {0x00002000, SMEM_FULL_MASK_CNS, smemChtActiveReadIntrCauseReg, 0, smemChtActiveWriteIntrCauseReg, 0},
    {0x00002004, SMEM_FULL_MASK_CNS, NULL, 0, smemChtActiveGenericWriteInterruptsMaskReg, 0},

    END_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)

}

/* bind the iplr0 unit to it's active mem */
static void bindUnitIplr0ActiveMem(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    START_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)

/*policer*/
    /* add common lines of all policers */
    ACTIVE_MEM_POLICER_COMMON_MAC(SMEM_SIP5_PP_PLR_UNIT_IPLR_0_E),

    {0x0000002C, SMEM_FULL_MASK_CNS, NULL, 0 , smemLion3ActiveWriteHierarchicalPolicerControl,  0},

    /* iplr0 policer table */
    {0x00100000, 0xFFFC0000, smemXCatActiveReadIplr0Tables, 0 , smemXCatActiveWriteIplr0Tables,  0},
    /* iplr0 policerCounters table */
    {0x00140000, 0xFFFC0000, smemXCatActiveReadIplr0Tables, 0 , smemXCatActiveWriteIplr0Tables,  0},

    END_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)
}
/* bind the iplr1 unit to it's active mem */
static void bindUnitIplr1ActiveMem(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    START_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)

/*policer*/
    /* add common lines of all policers */
    ACTIVE_MEM_POLICER_COMMON_MAC(SMEM_SIP5_PP_PLR_UNIT_IPLR_1_E),

    /* iplr1 policer table */
    {0x00100000, 0xFFFC0000, smemXCatActiveReadIplr1Tables, 0 , smemXCatActiveWriteIplr1Tables,  0},
    /* iplr1 policerCounters table */
    {0x00140000, 0xFFFC0000, smemXCatActiveReadIplr1Tables, 0 , smemXCatActiveWriteIplr1Tables,  0},

    END_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)



}
/* bind the eplr unit to it's active mem */
static void bindUnitEplrActiveMem(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    START_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)

/*policer*/
    /* add common lines of all policers */
    ACTIVE_MEM_POLICER_COMMON_MAC(SMEM_SIP5_PP_PLR_UNIT_EPLR_E),

    END_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)
}

/* bind the EQ unit to it's active mem */
static void bindUnitEqActiveMem(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    START_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)

    ACTIVE_MEM_EQ_COMMON_MAC,

    END_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)
}

/* NOTE: the cnc0 and the CNC1 are split , because can't share the same
    'static DB' because hold different base addresses !!!! */

/* bind the CNC0 unit to it's active mem */
static void bindUnitCnc0ActiveMem(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    START_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)
/*CNC_0 ,CNC_1 */
    ACTIVE_MEM_CNC_COMMON_MAC,

    END_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)
}

/* bind the CNC1 unit to it's active mem */
static void bindUnitCnc1ActiveMem(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    START_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)
/*CNC_0 ,CNC_1 */
    ACTIVE_MEM_CNC_COMMON_MAC,

    END_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)
}


/* bind the lms0 unit to it's active mem */
static void bindUnitLms0ActiveMem(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    START_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)
/* LMS */
    ACTIVE_MEM_LMS_COMMON_MAC(0),

    END_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)
}

/* bind the lms1 unit to it's active mem */
static void bindUnitLms1ActiveMem(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    START_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)
/* LMS */
    ACTIVE_MEM_LMS_COMMON_MAC(1),

    END_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)
}

/* bind the lms2 unit to it's active mem */
static void bindUnitLms2ActiveMem(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    START_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)
/* LMS */
    ACTIVE_MEM_LMS_COMMON_MAC(2),

    END_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)
}


/* bind the UNIT_TXQ_DQ unit to it's active mem */
static void bindUnitTxqDqActiveMem(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    START_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)
/*UNIT_TXQ_DQ*/
    /* Scheduler Configuration Register */
    {0x00001000, SMEM_FULL_MASK_CNS, NULL, 0 , smemLionActiveWriteSchedulerConfigReg, 0},

    /* Port<%n> Dequeue Enable Register ... 0..63 (64*4 - 1) = 255 */
    {0x00004300, 0xFFFFFF00, NULL, 0, smemLionActiveWriteDequeueEnableReg, 0/*the start port from 0x00*/},
    /* Port<%n> Dequeue Enable Register ... 64..71 (8*4 - 1) = 31 */
    {0x00004400, 0xFFFFFFE0, NULL, 0, smemLionActiveWriteDequeueEnableReg, 64/*the start port from 0x00*/},

    /*Egress STC interrupt register : 4 registers 0x00000630..0x0000063C */
    {0x00000630, 0xFFFFFFF0, smemChtActiveReadIntrCauseReg, 6 , smemChtActiveWriteIntrCauseReg,0},

    END_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)
}

/* bind the MLL unit to it's active mem */
static void bindUnitMllActiveMem(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    START_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)
/*MLL*/
    /* MLL counters */
    /*L2MLLValidProcessedEntriesCntr[0]*/
    {0xe20       , SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},
    /*L2MLLValidProcessedEntriesCntr[1]*/
    {0xe24       , SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},
    /*L2MLLValidProcessedEntriesCntr[2]*/
    {0xe28       , SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},
    /*L2MLLSilentDropCntr*/
    {0x00000e00  , SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},
    /*L2MLLSkippedEntriesCntr*/
    {0x00000e04  , SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},
    /*L2MLLTTLExceptionCntr*/
    {0x00000e08  , SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},
    /*L2MLLOutMcPktsCntr[0]*/
    {0xc00       , SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},
    /*L2MLLOutMcPktsCntr[1]*/
    {0xd00       , SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},
    /*MLLSilentDropCntr*/
    {0x00000800  , SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},
    /*MLLMCFIFODropCntr*/
    {0x00000804  , SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},
    /*IPMLLSkippedEntriesCntr*/
    {0x00000b04  , SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},
    /*IPMLLOutMcPktsCntr[0]*/
    {0x900       , SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},
    /*IPMLLOutMcPktsCntr[1]*/
    {0xa00       , SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},
    /*MLL Interrupt Cause Registers*/
    {0x00000030, SMEM_FULL_MASK_CNS, smemChtActiveReadIntrCauseReg, 0, smemChtActiveWriteIntrCauseReg, 0},
    {0x00000034, SMEM_FULL_MASK_CNS, NULL, 0, smemChtActiveGenericWriteInterruptsMaskReg, 0},

    END_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)
}

/*OAM*/
#define ACTIVE_MEM_OAM_COMMON_SIP5_MAC(oamId)                                                                      \
    /* OAM Unit Interrupt Cause */                                                                                 \
    {0x000000f0, SMEM_FULL_MASK_CNS, smemChtActiveReadIntrCauseReg, oamId, smemChtActiveWriteIntrCauseReg, oamId}, \
    /* OAM Unit Interrupt Mask */                                                                       \
    {0x000000f4, SMEM_FULL_MASK_CNS, NULL, oamId, smemChtActiveWriteMacInterruptsMaskReg, oamId},       \
    /* OAM Table Access Control */                                                                      \
    {0x000000D0, SMEM_FULL_MASK_CNS, NULL, oamId , smemLion3ActiveWriteOamTbl,oamId},                   \
                                                                                                        \
    /* Aging Table */                                                                                   \
    {0x00000c00, SMEM_FULL_MASK_CNS, smemLion3ActiveReadExceptionSummaryBitmap, oamId , NULL,oamId},    \
    /* Meg Exception Table */                                                                           \
    {0x00010000, SMEM_FULL_MASK_CNS, smemLion3ActiveReadExceptionSummaryBitmap, oamId , NULL,oamId},    \
    /* Source Interface Exception Table */                                                              \
    {0x00018000, SMEM_FULL_MASK_CNS, smemLion3ActiveReadExceptionSummaryBitmap, oamId , NULL,oamId},    \
    /* Invalid Keepalive Hash Exception Table */                                                        \
    {0x00020000, SMEM_FULL_MASK_CNS, smemLion3ActiveReadExceptionSummaryBitmap, oamId , NULL,oamId},    \
    /* Excess Keepalive Exception Table */                                                              \
    {0x00028000, SMEM_FULL_MASK_CNS, smemLion3ActiveReadExceptionSummaryBitmap, oamId , NULL,oamId},    \
    /* OAM Exception Summary Table */                                                                   \
    {0x00030000, SMEM_FULL_MASK_CNS, smemLion3ActiveReadExceptionSummaryBitmap, oamId , NULL,oamId},    \
    /* RDI Status Change Exception Table */                                                             \
    {0x00038000, SMEM_FULL_MASK_CNS, smemLion3ActiveReadExceptionSummaryBitmap, oamId , NULL,oamId},    \
    /* Tx Period Exception Table */                                                                     \
    {0x00040000, SMEM_FULL_MASK_CNS, smemLion3ActiveReadExceptionSummaryBitmap, oamId , NULL,oamId},    \
                                                                                                        \
    /* Meg Level Exception Counter */                                                                   \
    {0x00000300, SMEM_FULL_MASK_CNS, smemLion3ActiveReadExceptionCounter, oamId , NULL,oamId},          \
    /* Source Interface Mismatch Counter */                                                             \
    {0x00000304, SMEM_FULL_MASK_CNS, smemLion3ActiveReadExceptionCounter, oamId , NULL,oamId},          \
    /* Invalid Keepalive Hash Counter */                                                                \
    {0x00000308, SMEM_FULL_MASK_CNS, smemLion3ActiveReadExceptionCounter, oamId , NULL,oamId},          \
    OAM_ACTIVE_READ_MEMORY(0x00000308, smemLion3ActiveReadExceptionCounter),                            \
    /* Excess Keepalive Counter */                                                                      \
    {0x0000030c, SMEM_FULL_MASK_CNS, smemLion3ActiveReadExceptionCounter, oamId , NULL,oamId},          \
    /* Keepalive Aging Counter */                                                                       \
    {0x00000310, SMEM_FULL_MASK_CNS, smemLion3ActiveReadExceptionCounter, oamId , NULL,oamId},          \
    /* RDI Status Change Counter */                                                                     \
    {0x00000314, SMEM_FULL_MASK_CNS, smemLion3ActiveReadExceptionCounter, oamId , NULL,oamId},          \
    OAM_ACTIVE_READ_MEMORY(0x00000314, smemLion3ActiveReadExceptionCounter),                            \
    /* Tx Period Exception Counter */                                                                   \
    {0x00000318, SMEM_FULL_MASK_CNS, smemLion3ActiveReadExceptionCounter, oamId , NULL,oamId},          \
    /* Summary Exception Counter */                                                                     \
    {0x0000031c, SMEM_FULL_MASK_CNS, smemLion3ActiveReadExceptionCounter, oamId , NULL,oamId},          \
                                                                                                        \
    /* Aging Entry Group Status */                                                                      \
    {0x000000e0, SMEM_FULL_MASK_CNS, smemLion3ActiveReadExceptionGroupStatus, oamId , NULL,oamId},      \
    {0x000000e4, SMEM_FULL_MASK_CNS, smemLion3ActiveReadExceptionGroupStatus, oamId , NULL,oamId},      \
    /* Excess Keepalive Group Status */                                                                 \
    {0x00000200, SMEM_FULL_MASK_CNS, smemLion3ActiveReadExceptionGroupStatus, oamId , NULL,oamId},      \
    {0x00000204, SMEM_FULL_MASK_CNS, smemLion3ActiveReadExceptionGroupStatus, oamId , NULL,oamId},      \
    /* Invalid Keepalive Group Status */                                                                \
    {0x00000220, SMEM_FULL_MASK_CNS, smemLion3ActiveReadExceptionGroupStatus, oamId , NULL,oamId},      \
    {0x00000224, SMEM_FULL_MASK_CNS, smemLion3ActiveReadExceptionGroupStatus, oamId , NULL,oamId},      \
    /* Meg Level Group Status */                                                                        \
    {0x00000240, SMEM_FULL_MASK_CNS, smemLion3ActiveReadExceptionGroupStatus, oamId , NULL,oamId},      \
    {0x00000244, SMEM_FULL_MASK_CNS, smemLion3ActiveReadExceptionGroupStatus, oamId , NULL,oamId},      \
    /* Source Interface Group Status */                                                                 \
    {0x00000260, SMEM_FULL_MASK_CNS, smemLion3ActiveReadExceptionGroupStatus, oamId , NULL,oamId},      \
    {0x00000264, SMEM_FULL_MASK_CNS, smemLion3ActiveReadExceptionGroupStatus, oamId , NULL,oamId},      \
    /* RDI Status Change Group Status */                                                                \
    {0x00000280, SMEM_FULL_MASK_CNS, smemLion3ActiveReadExceptionGroupStatus, oamId , NULL,oamId},      \
    {0x00000284, SMEM_FULL_MASK_CNS, smemLion3ActiveReadExceptionGroupStatus, oamId , NULL,oamId},      \
    /* Tx Period Exception Group Status */                                                              \
    {0x000002A0, SMEM_FULL_MASK_CNS, smemLion3ActiveReadExceptionGroupStatus, oamId , NULL,oamId},      \
    {0x000002A4, SMEM_FULL_MASK_CNS, smemLion3ActiveReadExceptionGroupStatus, oamId , NULL,oamId},      \
    /* Summary Exception Status */                                                                      \
    {0x000002C0, SMEM_FULL_MASK_CNS, smemLion3ActiveReadExceptionGroupStatus, oamId , NULL,oamId},      \
    {0x000002C4, SMEM_FULL_MASK_CNS, smemLion3ActiveReadExceptionGroupStatus, oamId , NULL,oamId}

#define ACTIVE_MEM_OAM_COMMON_SIP5_15_MAC(oamId)                                                        \
    /* OAM Unit Interrupt Cause */                                                                                 \
    {0x000000f0, SMEM_FULL_MASK_CNS, smemChtActiveReadIntrCauseReg, oamId, smemChtActiveWriteIntrCauseReg, oamId}, \
    /* OAM Unit Interrupt Mask */                                                                       \
    {0x000000f4, SMEM_FULL_MASK_CNS, NULL, oamId, smemChtActiveWriteMacInterruptsMaskReg, oamId},       \
    /* OAM Table Access Control */                                                                      \
    {0x000000D0, SMEM_FULL_MASK_CNS, NULL, oamId , smemLion3ActiveWriteOamTbl,oamId},                   \
                                                                                                        \
    /* Aging Table */                                                                                   \
    {0x00007000, SMEM_FULL_MASK_CNS, smemLion3ActiveReadExceptionSummaryBitmap, oamId , NULL,oamId},    \
    /* Meg Exception Table */                                                                           \
    {0x00010000, SMEM_FULL_MASK_CNS, smemLion3ActiveReadExceptionSummaryBitmap, oamId , NULL,oamId},    \
    /* Source Interface Exception Table */                                                              \
    {0x00018000, SMEM_FULL_MASK_CNS, smemLion3ActiveReadExceptionSummaryBitmap, oamId , NULL,oamId},    \
    /* Invalid Keepalive Hash Exception Table */                                                        \
    {0x00020000, SMEM_FULL_MASK_CNS, smemLion3ActiveReadExceptionSummaryBitmap, oamId , NULL,oamId},    \
    /* Excess Keepalive Exception Table */                                                              \
    {0x00028000, SMEM_FULL_MASK_CNS, smemLion3ActiveReadExceptionSummaryBitmap, oamId , NULL,oamId},    \
    /* OAM Exception Summary Table */                                                                   \
    {0x00030000, SMEM_FULL_MASK_CNS, smemLion3ActiveReadExceptionSummaryBitmap, oamId , NULL,oamId},    \
    /* RDI Status Change Exception Table */                                                             \
    {0x00038000, SMEM_FULL_MASK_CNS, smemLion3ActiveReadExceptionSummaryBitmap, oamId , NULL,oamId},    \
    /* Tx Period Exception Table */                                                                     \
    {0x00040000, SMEM_FULL_MASK_CNS, smemLion3ActiveReadExceptionSummaryBitmap, oamId , NULL,oamId},    \
                                                                                                        \
    /* Meg Level Exception Counter */                                                                   \
    {0x00000a00, SMEM_FULL_MASK_CNS, smemLion3ActiveReadExceptionCounter, oamId , NULL,oamId},          \
    /* Source Interface Mismatch Counter */                                                             \
    {0x00000a04, SMEM_FULL_MASK_CNS, smemLion3ActiveReadExceptionCounter, oamId , NULL,oamId},          \
    /* Invalid Keepalive Hash Counter */                                                                \
    {0x00000a08, SMEM_FULL_MASK_CNS, smemLion3ActiveReadExceptionCounter, oamId , NULL,oamId},          \
    OAM_ACTIVE_READ_MEMORY(0x00000a08, smemLion3ActiveReadExceptionCounter),                            \
    /* Excess Keepalive Counter */                                                                      \
    {0x00000a0c, SMEM_FULL_MASK_CNS, smemLion3ActiveReadExceptionCounter, oamId , NULL,oamId},          \
    /* Keepalive Aging Counter */                                                                       \
    {0x00000a10, SMEM_FULL_MASK_CNS, smemLion3ActiveReadExceptionCounter, oamId , NULL,oamId},          \
    /* RDI Status Change Counter */                                                                     \
    {0x00000a14, SMEM_FULL_MASK_CNS, smemLion3ActiveReadExceptionCounter, oamId , NULL,oamId},          \
    OAM_ACTIVE_READ_MEMORY(0x00000a14, smemLion3ActiveReadExceptionCounter),                            \
    /* Tx Period Exception Counter */                                                                   \
    {0x00000a18, SMEM_FULL_MASK_CNS, smemLion3ActiveReadExceptionCounter, oamId , NULL,oamId},          \
    /* Summary Exception Counter */                                                                     \
    {0x00000a1c, SMEM_FULL_MASK_CNS, smemLion3ActiveReadExceptionCounter, oamId , NULL,oamId},          \
                                                                                                        \
    /* Aging Entry Group Status */                                                                      \
    {0x00000900, SMEM_FULL_MASK_CNS, smemLion3ActiveReadExceptionGroupStatus, oamId , NULL,oamId},      \
    {0x00000904, SMEM_FULL_MASK_CNS, smemLion3ActiveReadExceptionGroupStatus, oamId , NULL,oamId},      \
    /* Excess Keepalive Group Status */                                                                 \
    {0x00000200, SMEM_FULL_MASK_CNS, smemLion3ActiveReadExceptionGroupStatus, oamId , NULL,oamId},      \
    {0x00000204, SMEM_FULL_MASK_CNS, smemLion3ActiveReadExceptionGroupStatus, oamId , NULL,oamId},      \
    /* Invalid Keepalive Group Status */                                                                \
    {0x00000300, SMEM_FULL_MASK_CNS, smemLion3ActiveReadExceptionGroupStatus, oamId , NULL,oamId},      \
    {0x00000304, SMEM_FULL_MASK_CNS, smemLion3ActiveReadExceptionGroupStatus, oamId , NULL,oamId},      \
    /* Meg Level Group Status */                                                                        \
    {0x00000400, SMEM_FULL_MASK_CNS, smemLion3ActiveReadExceptionGroupStatus, oamId , NULL,oamId},      \
    {0x00000404, SMEM_FULL_MASK_CNS, smemLion3ActiveReadExceptionGroupStatus, oamId , NULL,oamId},      \
    /* Source Interface Group Status */                                                                 \
    {0x00000500, SMEM_FULL_MASK_CNS, smemLion3ActiveReadExceptionGroupStatus, oamId , NULL,oamId},      \
    {0x00000504, SMEM_FULL_MASK_CNS, smemLion3ActiveReadExceptionGroupStatus, oamId , NULL,oamId},      \
    /* RDI Status Change Group Status */                                                                \
    {0x00000600, SMEM_FULL_MASK_CNS, smemLion3ActiveReadExceptionGroupStatus, oamId , NULL,oamId},      \
    {0x00000604, SMEM_FULL_MASK_CNS, smemLion3ActiveReadExceptionGroupStatus, oamId , NULL,oamId},      \
    /* Tx Period Exception Group Status */                                                              \
    {0x00000700, SMEM_FULL_MASK_CNS, smemLion3ActiveReadExceptionGroupStatus, oamId , NULL,oamId},      \
    {0x00000704, SMEM_FULL_MASK_CNS, smemLion3ActiveReadExceptionGroupStatus, oamId , NULL,oamId},      \
    /* Summary Exception Status */                                                                      \
    {0x00000800, SMEM_FULL_MASK_CNS, smemLion3ActiveReadExceptionGroupStatus, oamId , NULL,oamId},      \
    {0x00000804, SMEM_FULL_MASK_CNS, smemLion3ActiveReadExceptionGroupStatus, oamId , NULL,oamId}

/* bind the IOam unit to it's active mem */
static void bindUnitIOamActiveMem(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    if(0 == SMEM_CHT_IS_SIP5_15_GET(devObjPtr))
    {
        START_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)

        ACTIVE_MEM_OAM_COMMON_SIP5_MAC(0),

        END_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)
    }
    else
    {
        START_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)

        ACTIVE_MEM_OAM_COMMON_SIP5_15_MAC(0),

        END_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)
    }
}

/* bind the IOam unit to it's active mem */
static void bindUnitEOamActiveMem(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    if(0 == SMEM_CHT_IS_SIP5_15_GET(devObjPtr))
    {
        START_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)

        ACTIVE_MEM_OAM_COMMON_SIP5_MAC(1),

        END_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)
    }
    else
    {
        START_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)

        ACTIVE_MEM_OAM_COMMON_SIP5_15_MAC(1),

        END_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)
    }
}

/* bind the TCAM unit to it's active mem */
static void bindUnitTcamActiveMem(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    START_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)
/*TCAM*/
    /* SIP5 Tcam management registers */
    {0x00502000, SMEM_FULL_MASK_CNS, NULL, 0 , smemLion3ActiveWriteTcamMgLookup,0},

    /* next 4 lines catch addresses of : 0x005021BC..0x00502208 (20 registers)
       those are 'read only' registers.  */
    /*exact match - 0x005021BC */
    {0x005021BC, SMEM_FULL_MASK_CNS, NULL, 0, smemLion3ActiveWriteTcamMgHitGroupHitNum,0},
    /* match - 0x005021C0.. 0x005021FC , so need to ignore 5021E0 , 5021F4 */
    {0x005021C0, 0xFFFFFFC0, NULL, 0, smemLion3ActiveWriteTcamMgHitGroupHitNum,0},
    /* match - 0x00502200.. 0x0050221C , so need to ignore 502208, 50221C */
    {0x00502200, 0xFFFFFFE0, NULL,0 , smemLion3ActiveWriteTcamMgHitGroupHitNum,0},
    /* TCAMInterruptCause register */
    {0x0050100c, SMEM_FULL_MASK_CNS, smemChtActiveReadIntrCauseReg, 0, smemChtActiveWriteIntrCauseReg, 0},
    {0x00501008, SMEM_FULL_MASK_CNS, NULL, 0, smemChtActiveGenericWriteInterruptsMaskReg, 0},

    END_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)
}

/* bind the TM unit to it's active mem */
static void bindUnitTmActiveMem(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    START_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)

/* TM unit */
    /* the whole unit is '64 bits' burst on write. need to write the data only after get full info ...
       NOTE : only after write to address ending 0x4,0xC ... trigger the function ! */
    {0x4, 0xFFF00004, NULL,0 , smemBobcat2ActiveWriteTmHostingUnit,0},

    END_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)

}

/* bind the TTI unit to it's active mem */
static void bindUnitTtiActiveMem(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    START_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)
/* TTI */
    /* PTP Exception counter */
    {0x3034, 0xFFFFFFFF, smemLion3ActiveReadExceptionCounter, 0, NULL, 0},

    /* TTI Engine Interrupt Cause */
    {0x00000004, SMEM_FULL_MASK_CNS, smemChtActiveReadIntrCauseReg,0,smemChtActiveWriteIntrCauseReg,0},
    {0x00000008, SMEM_FULL_MASK_CNS, NULL, 0, smemChtActiveGenericWriteInterruptsMaskReg, 0},
    END_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)
}

/* bind the HA unit to it's active mem */
static void bindUnitHaActiveMem(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    START_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)
/* HA */
    /* HA Interrupt Cause Register */
    {0x00000300, SMEM_FULL_MASK_CNS,
        smemChtActiveReadIntrCauseReg, 12, smemChtActiveWriteIntrCauseReg, 0},
    /* HA Interrupt Mask Register */
    {0x00000304, SMEM_FULL_MASK_CNS,
        NULL, 0, smemLion3ActiveWriteHaInterruptsMaskReg, 0},

    /* ROC (read only clear) counters */
    /*Nat Exception Drop Counter*/
    { 0x00000314, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},

    END_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)
}


/* bind the ERMRK unit to it's active mem */
static void bindUnitErmrkActiveMem(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    START_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)
/* ERMRK */
    /* ERMRK Interrupt Cause Register */
    {0x4, SMEM_FULL_MASK_CNS,
        smemChtActiveReadIntrCauseReg, 11, smemChtActiveWriteIntrCauseReg, 0},
    /* ERMRK Interrupt Mask Register */
    {0x8, SMEM_FULL_MASK_CNS,
        NULL, 0, smemLion3ActiveWriteErmrkInterruptsMaskReg, 0},

    /* ERMRK PTP Configuration */
    {0x10, 0xFFFFFFFF, NULL, 0, smemLion3ActiveWriteErmrkPtpConfReg, 0},

    /* ERMRK Timestamp Queue Clear Entry ID */
    {0xE4, 0xFFFFFFFF, NULL, 0, smemLion3ActiveWriteErmrkTsQueueEntryIdClearReg, 0},

    /* ERMRK Timestamp Queue Entry Word1/Word2 */
    {0x120, 0xFFFFFFE0, smemLion3ActiveReadErmrkTsQueueEntryWord1Word2Reg, 0, NULL, 0},

    END_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)
}

/* bind the ERMRK unit to it's active mem - sip5_10 */
static void bindUnitEpclActiveMem_sip5_10(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    START_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)

    /* additional sip5_10 active memory */
    /*EPCL Bad Addr Latch Reg*/
    {0x00000018, SMEM_FULL_MASK_CNS, NULL, 0, smemChtActiveReadCntrs, 0},

    END_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)
}

/* LMS */
#define ACTIVE_MEM_SMI_COMMON_MAC(smiId)                                            \
    /* SMI0 Management Register */                                                  \
    {0x00000000    , SMEM_FULL_MASK_CNS , NULL, smiId, smemChtActiveWriteSmi,smiId}

/* bind the SMI0 unit to it's active mem - sip5_10 */
static void bindUnitSmi0ActiveMem(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    START_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)

    ACTIVE_MEM_SMI_COMMON_MAC(0),

    END_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)
}
/* bind the SMI1 unit to it's active mem - sip5_10 */
static void bindUnitSmi1ActiveMem(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    START_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)

    ACTIVE_MEM_SMI_COMMON_MAC(1),

    END_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)
}
/* bind the SMI2 unit to it's active mem - sip5_10 */
static void bindUnitSmi2ActiveMem(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    START_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)

    ACTIVE_MEM_SMI_COMMON_MAC(2),

    END_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)
}
/* bind the SMI3 unit to it's active mem - sip5_10 */
static void bindUnitSmi3ActiveMem(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    START_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)

    ACTIVE_MEM_SMI_COMMON_MAC(3),

    END_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)
}

/* LMS */
#define ACTIVE_MEM_RXDMA_COMMON_MAC(unitId)                                            \
    /* 'ROC' counters */                                                               \
    /*rxDMA.singleChannelDMAPip.SCDMA_n_Drop_Pkt_Counter[]*/                           \
    /* first 64 ports */                                                               \
    {0x00003500, 0xFFFFFF00  , smemChtActiveReadCntrs, 0, NULL,0},                     \
    /* next  8 ports */                                                                \
    {0x00003600, 0xFFFFFFE0  , smemChtActiveReadCntrs, 0, NULL,0},                     \
    /* next  1 port  */                                                                \
    {0x00003620, 0xFFFFFFFF  , smemChtActiveReadCntrs, 0, NULL,0},                     \
    /*rxDMA.globalRxDMAConfigs.preIngrPrioritizationConfStatus.PIPLowPriorityDropGlobalCntrLSb*/   \
    /*rxDMA.globalRxDMAConfigs.preIngrPrioritizationConfStatus.PIPLowPriorityDropGlobalCntrMsb*/   \
    {0x00000710, 0xFFFFFFF8  , smemChtActiveReadCntrs64Bit, 0, NULL,0},                            \
    /*rxDMA.globalRxDMAConfigs.preIngrPrioritizationConfStatus.PIPMedPriorityDropGlobalCntrLsb*/   \
    /*rxDMA.globalRxDMAConfigs.preIngrPrioritizationConfStatus.PIPMedPriorityDropGlobalCntrMsb*/   \
    {0x00000720, 0xFFFFFFF8  , smemChtActiveReadCntrs64Bit, 0, NULL,0},                            \
    /*rxDMA.globalRxDMAConfigs.preIngrPrioritizationConfStatus.PIPHighPriorityDropGlobalCntrLsb*/  \
    /*rxDMA.globalRxDMAConfigs.preIngrPrioritizationConfStatus.PIPHighPriorityDropGlobalCntrMsb*/  \
    {0x00000730, 0xFFFFFFF8  , smemChtActiveReadCntrs64Bit, 0, NULL,0},                            \
                                                                                           \
    /*globalRxDMAConfigs.preIngrPrioritizationConfStatus.PIPPriorityDropGlobalCntrsClear*/ \
    {0x00000740, 0xFFFFFFFF  , NULL, 0, smemBobcat2ActiveWriteRxDmaGlobalCntrsClear,0}


/* bind the RxDma unit to it's active mem - sip5_10 */
static void bindUnitRxDmaActiveMem(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    START_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)

    ACTIVE_MEM_RXDMA_COMMON_MAC(0),


    END_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)
}

/* bind the RxDma unit to it's active mem - sip5_10 */
static void bindUnitRxDma1ActiveMem(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    START_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)

    ACTIVE_MEM_RXDMA_COMMON_MAC(1),


    END_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)
}

/* bind the PCL unit to it's active mem */
static void bindUnitPclActiveMem(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
 )
{
    START_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)

    /* PCL Unit Interrupt Cause */
    {0x00000004, SMEM_FULL_MASK_CNS, smemChtActiveReadIntrCauseReg, 0, smemChtActiveWriteIntrCauseReg,0},
    {0x00000008, SMEM_FULL_MASK_CNS, NULL, 0, smemChtActiveGenericWriteInterruptsMaskReg, 0},

    END_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)
}
static void bindUnitEgfEft0ActiveMem(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    START_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)

    /* EFTInterruptsCause */
    {0x000010A0, SMEM_FULL_MASK_CNS, smemChtActiveReadIntrCauseReg, 0, smemChtActiveWriteIntrCauseReg,0},
    {0x000010B0, SMEM_FULL_MASK_CNS, NULL, 0, smemChtActiveGenericWriteInterruptsMaskReg, 0},

    END_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)
}

/* Active memory table -- of the device -- empty , because using DB 'per unit' ...
   the 'per unit' is better because :
   1. lookup of address in faster in time
   2. defined with relative address (and not absolute) ,
       which allow to share it with other devices !!! */
static SMEM_ACTIVE_MEM_ENTRY_STC smemBobcat2ActiveTable[] =
{
    /* must be last anyway */
    SMEM_ACTIVE_MEM_ENTRY_LAST_LINE_CNS
};

/*******************************************************************************
 * Typedef: enum SMEM_LION_2_3_UNIT_NAME_ENT
 *
 * Description:
 *      Memory units names -- used only as index into genericUsedUnitsAddressesArray[]
 *
 *******************************************************************************/
typedef enum{
/* 0*/    SMEM___LEGACY___BOBCAT2_UNIT_MG_E,
/* 1*/    SMEM___LEGACY___BOBCAT2_UNIT_IPLR1_E,       /* not for bobcat2 */
/* 2*/    SMEM___LEGACY___BOBCAT2_UNIT_TTI_E,
/* 3*/    SMEM___LEGACY___BOBCAT2_UNIT_DFX_SERVER_E,
/* 4*/    SMEM___LEGACY___BOBCAT2_UNIT_L2I_E,
/* 5*/    SMEM___LEGACY___BOBCAT2_UNIT_IPVX_E,
/* 6*/    SMEM___LEGACY___BOBCAT2_UNIT_BM_E,
/* 7*/    SMEM___LEGACY___BOBCAT2_UNIT_BMA_E,
/* 8*/    SMEM___LEGACY___BOBCAT2_UNIT_LMS_E,
/* 9*/    SMEM___LEGACY___BOBCAT2_UNIT_FDB_E,
/*10*/    SMEM___LEGACY___BOBCAT2_UNIT_MPPM_E,
/*11*/    SMEM___LEGACY___BOBCAT2_UNIT_CTU0_E,
/*12*/    SMEM___LEGACY___BOBCAT2_UNIT_EPLR_E,        /* not for bobcat2 */
/*13*/    SMEM___LEGACY___BOBCAT2_UNIT_CNC_E,         /* not for bobcat2 */
/*14*/    SMEM___LEGACY___BOBCAT2_UNIT_GOP_E,
/*15*/    SMEM___LEGACY___BOBCAT2_UNIT_XG_PORT_MIB_E,
/*16*/    SMEM___LEGACY___BOBCAT2_UNIT_SERDES_E,
/*17*/    SMEM___LEGACY___BOBCAT2_UNIT_MPPM1_E,
/*18*/    SMEM___LEGACY___BOBCAT2_UNIT_CTU1_E,
/*19*/    SMEM___LEGACY___BOBCAT2_UNIT_EQ_E,
/*20*/    SMEM___LEGACY___BOBCAT2_UNIT_IPCL_E,        /* not for bobcat2 */
/*21*/    SMEM___LEGACY___BOBCAT2_UNIT_IPLR_E,        /* not for bobcat2 */
/*22*/    SMEM___LEGACY___BOBCAT2_UNIT_MLL_E,         /* not for bobcat2 */
/*23*/    SMEM___LEGACY___BOBCAT2_UNIT_IPCL_TCC_E,    /* not for bobcat2 */
/*24*/    SMEM___LEGACY___BOBCAT2_UNIT_IPVX_TCC_E,
/*25*/    SMEM___LEGACY___BOBCAT2_UNIT_EPCL_E,        /*not for bobcat2*/
/*26*/    SMEM___LEGACY___BOBCAT2_UNIT_HA_E,          /*not for bobcat2*/
/*27*/    SMEM___LEGACY___BOBCAT2_UNIT_RX_DMA_E,
/*28*/    SMEM___LEGACY___BOBCAT2_UNIT_TX_DMA_E,
/*29*/    SMEM___LEGACY___BOBCAT2_UNIT_TXQ_QUEUE_E,
/*30*/    SMEM___LEGACY___BOBCAT2_UNIT_TXQ_LL_E,
/*31*/    SMEM___LEGACY___BOBCAT2_UNIT_TXQ_DQ_E,
/*32*/    SMEM___LEGACY___BOBCAT2_UNIT_TXQ_SHT_E,      /*not for bobcat2*/
/*33*/    SMEM___LEGACY___BOBCAT2_UNIT_TXQ_EGR0_E,     /*not for bobcat2*/
/*34*/    SMEM___LEGACY___BOBCAT2_UNIT_TXQ_EGR1_E,     /*not for bobcat2*/
/*35*/    SMEM___LEGACY___BOBCAT2_UNIT_TXQ_DIST_E,
/*36*/    SMEM___LEGACY___BOBCAT2_UNIT_CPFC_E,

    SMEM___LEGACY___BOBCAT2_UNIT_LAST_E,

/*37*/    SMEM_BOBCAT2_UNIT_MLL_E                 = SMEM___LEGACY___BOBCAT2_UNIT_LAST_E,
/*38*/    SMEM_BOBCAT2_UNIT_IOAM_E,
/*39*/    SMEM_BOBCAT2_UNIT_EOAM_E,

/*40*/    SMEM_BOBCAT2_UNIT_LPM_E,

/*49*/    SMEM_BOBCAT2_UNIT_EQ_0_E,/*part 0 of EQ*/
/*50*/    SMEM_BOBCAT2_UNIT_EQ_1_E,/*part 1 of EQ*/

/*51*/    SMEM_BOBCAT2_UNIT_FDB_TABLE_0_E,
/*52*/    SMEM_BOBCAT2_UNIT_FDB_TABLE_1_E,

/*53*/    SMEM_BOBCAT2_UNIT_EGF_EFT_E,
/*54*/    SMEM_BOBCAT2_UNIT_EGF_EFT_1_E,

/*55*/    SMEM_BOBCAT2_UNIT_EGF_QAG_E,
/*56*/    SMEM_BOBCAT2_UNIT_EGF_QAG_1_E,

/*57*/    SMEM_BOBCAT2_UNIT_EGF_SHT_E,
/*58*/    SMEM_BOBCAT2_UNIT_EGF_SHT_1_E,
/*59*/    SMEM_BOBCAT2_UNIT_EGF_SHT_2_E,
/*60*/    SMEM_BOBCAT2_UNIT_EGF_SHT_3_E,
/*61*/    SMEM_BOBCAT2_UNIT_EGF_SHT_4_E,
/*62*/    SMEM_BOBCAT2_UNIT_EGF_SHT_5_E, /*!!! EMPTY !!!*/
/*63*/    SMEM_BOBCAT2_UNIT_EGF_SHT_6_E,
/*64*/    SMEM_BOBCAT2_UNIT_EGF_SHT_7_E, /*!!! EMPTY !!!*/
/*65*/    SMEM_BOBCAT2_UNIT_EGF_SHT_8_E,
/*66*/    SMEM_BOBCAT2_UNIT_EGF_SHT_9_E,
/*67*/    SMEM_BOBCAT2_UNIT_EGF_SHT_10_E,
/*68*/    SMEM_BOBCAT2_UNIT_EGF_SHT_11_E,
/*69*/    SMEM_BOBCAT2_UNIT_EGF_SHT_12_E,

/*70*/    SMEM_BOBCAT2_UNIT_HA_E,
/*71*/    SMEM_BOBCAT2_UNIT_IPCL_E,
/*72*/    SMEM_BOBCAT2_UNIT_EPCL_E,

/*73*/    SMEM_BOBCAT2_UNIT_TCAM_E,
/*74*/    SMEM_BOBCAT2_UNIT_TM_HOSTING_E,
/*75*/    SMEM_BOBCAT2_UNIT_ERMRK_E,

/*76*/    SMEM_BOBCAT2_UNIT_GOP_E,

/*77*/    SMEM_BOBCAT2_UNIT_IPLR0_E,
/*78*/    SMEM_BOBCAT2_UNIT_IPLR1_E,
/*79*/    SMEM_BOBCAT2_UNIT_EPLR_E,

/*80*/    SMEM_BOBCAT2_UNIT_CNC_0_E,
/*81*/    SMEM_BOBCAT2_UNIT_CNC_1_E,

/*82*/    SMEM_BOBCAT2_UNIT_TMDROP_E,

    SMEM_BOBCAT2_UNIT_TXQ_QUEUE_E,
    SMEM_BOBCAT2_UNIT_TXQ_LL_E,
    SMEM_BOBCAT2_UNIT_TXQ_DQ_E,
    SMEM_BOBCAT2_UNIT_CPFC_E,
    SMEM_BOBCAT2_UNIT_TXQ_QCN_E,

    SMEM_BOBCAT2_UNIT_TX_FIFO_E,

    SMEM_BOBCAT2_UNIT_ETH_TX_FIFO_E,
    SMEM_BOBCAT2_UNIT_ILKN_TX_FIFO_E,

    SMEM_BOBCAT2_UNIT_TM_FCU_E,
    SMEM_BOBCAT2_UNIT_TM_INGRESS_GLUE_E,
    SMEM_BOBCAT2_UNIT_TM_EGRESS_GLUE_E,
    SMEM_BOBCAT2_UNIT_TMQMAP_E,

    SMEM_BOBCAT2_UNIT_LMS_E,
    SMEM_BOBCAT2_UNIT_LMS1_E,
    SMEM_BOBCAT2_UNIT_LMS2_E,

    SMEM_BOBCAT2_UNIT_GOP_ILKN_E,

    SMEM_BOBCAT2_B0_UNIT_GOP_SMI_0_E,
    SMEM_BOBCAT2_B0_UNIT_GOP_SMI_1_E,
    SMEM_BOBCAT2_B0_UNIT_GOP_SMI_2_E,
    SMEM_BOBCAT2_B0_UNIT_GOP_SMI_3_E,
    SMEM_BOBCAT2_B0_UNIT_GOP_LED_0_E,
    SMEM_BOBCAT2_B0_UNIT_GOP_LED_1_E,
    SMEM_BOBCAT2_B0_UNIT_GOP_LED_2_E,
    SMEM_BOBCAT2_B0_UNIT_GOP_LED_3_E,
    SMEM_BOBCAT2_B0_UNIT_GOP_LED_4_E,

    SMEM_BOBCAT2_UNIT_LAST_E
}SMEM_BOBCAT2_UNIT_NAME_ENT;


/* the addresses of the units that the Lion 2,3 port group use */
static GT_U32   bobcat2UsedUnitsAddressesArray[SMEM_BOBCAT2_UNIT_LAST_E]=
{
/* SMEM___LEGACY___BOBCAT2_UNIT_MG_E,                      */ BOBCAT2_MG_UNIT_BASE_ADDR_CNS              ,
/* SMEM___LEGACY___BOBCAT2_UNIT_IPLR1_E,                   */ SMAIN_NOT_VALID_CNS                        ,  /*not for bobcat2*/
/* SMEM___LEGACY___BOBCAT2_UNIT_TTI_E,                     */ BOBCAT2_TTI_UNIT_BASE_ADDR_CNS             ,
/* SMEM___LEGACY___BOBCAT2_UNIT_DFX_SERVER_E               */ SMAIN_NOT_VALID_CNS      ,
/* SMEM___LEGACY___BOBCAT2_UNIT_L2I_E,                     */ BOBCAT2_L2I_UNIT_BASE_ADDR_CNS             ,
/* SMEM___LEGACY___BOBCAT2_UNIT_IPVX_E,                    */ BOBCAT2_IPVX_UNIT_BASE_ADDR_CNS            ,
/* SMEM___LEGACY___BOBCAT2_UNIT_BM_E,                      */ BOBCAT2_BM_UNIT_BASE_ADDR_CNS              ,
/* SMEM___LEGACY___BOBCAT2_UNIT_BMA_E,                     */ BOBCAT2_BMA_UNIT_BASE_ADDR_CNS             ,
/* SMEM___LEGACY___BOBCAT2_UNIT_LMS_E,                     */ SMAIN_NOT_VALID_CNS             ,
/* SMEM___LEGACY___BOBCAT2_UNIT_FDB_E,                     */ BOBCAT2_FDB_UNIT_BASE_ADDR_CNS             ,
/* SMEM___LEGACY___BOBCAT2_UNIT_MPPM_E,                    */ BOBCAT2_MPPM_UNIT_BASE_ADDR_CNS            ,
/* SMEM___LEGACY___BOBCAT2_UNIT_CTU0_E,                    */ SMAIN_NOT_VALID_CNS                        ,  /*not for bobcat2*/
/* SMEM___LEGACY___BOBCAT2_UNIT_EPLR_E,                    */ SMAIN_NOT_VALID_CNS                        ,  /*not for bobcat2*/
/* SMEM___LEGACY___BOBCAT2_UNIT_CNC_E,    */                  SMAIN_NOT_VALID_CNS                        ,  /*not for bobcat2*/
/* SMEM___LEGACY___BOBCAT2_UNIT_GOP_E,                     */ BOBCAT2_GOP_UNIT_BASE_ADDR_CNS             ,
/* SMEM___LEGACY___BOBCAT2_UNIT_XG_PORT_MIB_E,             */ BOBCAT2_MIB_UNIT_BASE_ADDR_CNS             ,
/* SMEM___LEGACY___BOBCAT2_UNIT_SERDES_E,               */    BOBCAT2_SERDES_UNIT_BASE_ADDR_CNS          ,
/* SMEM___LEGACY___BOBCAT2_UNIT_MPPM1_E,                   */ SMAIN_NOT_VALID_CNS                        ,  /*not for bobcat2*/
/* SMEM___LEGACY___BOBCAT2_UNIT_CTU1_E,                    */ SMAIN_NOT_VALID_CNS                        ,  /*not for bobcat2*/
/* SMEM___LEGACY___BOBCAT2_UNIT_EQ_E,                      */ SMAIN_NOT_VALID_CNS                        ,  /*not for bobcat2*/
/* SMEM___LEGACY___BOBCAT2_UNIT_IPCL_E,                    */ SMAIN_NOT_VALID_CNS                        ,  /*not for bobcat2*/
/* SMEM___LEGACY___BOBCAT2_UNIT_IPLR_E,                    */ SMAIN_NOT_VALID_CNS                        ,  /*not for bobcat2*/
/* SMEM___LEGACY___BOBCAT2_UNIT_MLL_E,                     */ SMAIN_NOT_VALID_CNS                        ,  /*not for bobcat2*/
/* SMEM___LEGACY___BOBCAT2_UNIT_IPCL_TCC_E,                */ BOBCAT2_IPCL_TCC_UNIT_BASE_ADDR_CNS        ,  /*not for bobcat2*/
/* SMEM___LEGACY___BOBCAT2_UNIT_IPVX_TCC_E,                */ BOBCAT2_IPVX_TCC_UNIT_BASE_ADDR_CNS        ,
/* SMEM___LEGACY___BOBCAT2_UNIT_EPCL_E,                    */ SMAIN_NOT_VALID_CNS                        ,  /*not for bobcat2*/
/* SMEM___LEGACY___BOBCAT2_UNIT_HA_E,                      */ SMAIN_NOT_VALID_CNS                        ,  /*not for bobcat2*/

/* SMEM___LEGACY___BOBCAT2_UNIT_RX_DMA_E,                  */ BOBCAT2_RX_DMA_UNIT_BASE_ADDR_CNS          ,
/* SMEM___LEGACY___BOBCAT2_UNIT_TX_DMA_E,                  */ BOBCAT2_TX_DMA_UNIT_BASE_ADDR_CNS          ,
/* SMEM___LEGACY___BOBCAT2_UNIT_TXQ_QUEUE_E,               */ SMAIN_NOT_VALID_CNS           , /*not for bobcat2*/
/* SMEM___LEGACY___BOBCAT2_UNIT_TXQ_LL_E,                  */ SMAIN_NOT_VALID_CNS           , /*not for bobcat2*/
/* SMEM___LEGACY___BOBCAT2_UNIT_TXQ_DQ_E,                  */ SMAIN_NOT_VALID_CNS           , /*not for bobcat2*/
/* SMEM___LEGACY___BOBCAT2_UNIT_TXQ_SHT_E,                 */ SMAIN_NOT_VALID_CNS                        ,  /*not for bobcat2*/
/* SMEM___LEGACY___BOBCAT2_UNIT_TXQ_EGR0_E,                */ SMAIN_NOT_VALID_CNS                        ,  /*not for bobcat2*/
/* SMEM___LEGACY___BOBCAT2_UNIT_TXQ_EGR1_E,                */ SMAIN_NOT_VALID_CNS                        ,  /*not for bobcat2*/
/* SMEM___LEGACY___BOBCAT2_UNIT_TXQ_DIST_E,                */ SMAIN_NOT_VALID_CNS           ,  /*not for bobcat2*/
/* SMEM___LEGACY___BOBCAT2_UNIT_CPFC_E,                    */ SMAIN_NOT_VALID_CNS           , /*not for bobcat2*/

/* SMEM_BOBCAT2_UNIT_MLL_E,                     */ BOBCAT2_MLL_UNIT_BASE_ADDR_CNS  ,
/* SMEM_BOBCAT2_UNIT_IOAM_E,                    */ BOBCAT2_IOAM_UNIT_BASE_ADDR_CNS ,
/* SMEM_BOBCAT2_UNIT_EOAM_E                     */ BOBCAT2_EOAM_UNIT_BASE_ADDR_CNS ,

/* SMEM_BOBCAT2_UNIT_LPM_E,                     */ BOBCAT2_LPM_UNIT_BASE_ADDR_CNS  ,

/* SMEM_BOBCAT2_UNIT_EQ_0_E,                    */ BOBCAT2_EQ_UNIT_BASE_ADDR_CNS(0)  ,/*part 0 of EQ*/
/* SMEM_BOBCAT2_UNIT_EQ_1_E,                    */ BOBCAT2_EQ_UNIT_BASE_ADDR_CNS(1)  ,/*part 1 of EQ*/

/* SMEM_BOBCAT2_UNIT_FDB_TABLE_0_E,             */ BOBCAT2_FDB_TABLE_BASE_ADDR_CNS(0) ,
/* SMEM_BOBCAT2_UNIT_FDB_TABLE_1_E,             */ BOBCAT2_FDB_TABLE_BASE_ADDR_CNS(1) ,

/* SMEM_BOBCAT2_UNIT_EGF_EFT_E,               */ BOBCAT2_EGF_EFT_UNIT_BASE_ADDR_CNS(0 ),
/* SMEM_BOBCAT2_UNIT_EGF_EFT_1_E,               */ BOBCAT2_EGF_EFT_UNIT_BASE_ADDR_CNS(1 ),

/* SMEM_BOBCAT2_UNIT_EGF_QAG_E,               */ BOBCAT2_EGF_QAG_UNIT_BASE_ADDR_CNS(0 ),
/* SMEM_BOBCAT2_UNIT_EGF_QAG_1_E,               */ BOBCAT2_EGF_QAG_UNIT_BASE_ADDR_CNS(1 ),

/* SMEM_BOBCAT2_UNIT_EGF_SHT_E,               */ BOBCAT2_EGF_SHT_UNIT_BASE_ADDR_CNS(0 ),
/* SMEM_BOBCAT2_UNIT_EGF_SHT_1_E,               */ BOBCAT2_EGF_SHT_UNIT_BASE_ADDR_CNS(1 ),
/* SMEM_BOBCAT2_UNIT_EGF_SHT_2_E,               */ BOBCAT2_EGF_SHT_UNIT_BASE_ADDR_CNS(2 ),
/* SMEM_BOBCAT2_UNIT_EGF_SHT_3_E,               */ BOBCAT2_EGF_SHT_UNIT_BASE_ADDR_CNS(3 ),
/* SMEM_BOBCAT2_UNIT_EGF_SHT_4_E,               */ BOBCAT2_EGF_SHT_UNIT_BASE_ADDR_CNS(4 ),
/* SMEM_BOBCAT2_UNIT_EGF_SHT_5_E                */ BOBCAT2_EGF_SHT_UNIT_BASE_ADDR_CNS(5 ),/*              --> EMPTY */
/* SMEM_BOBCAT2_UNIT_EGF_SHT_6_E,               */ BOBCAT2_EGF_SHT_UNIT_BASE_ADDR_CNS(6 ),
/* SMEM_BOBCAT2_UNIT_EGF_SHT_7_E,               */ BOBCAT2_EGF_SHT_UNIT_BASE_ADDR_CNS(7 ),/*              --> EMPTY */
/* SMEM_BOBCAT2_UNIT_EGF_SHT_8_E,               */ BOBCAT2_EGF_SHT_UNIT_BASE_ADDR_CNS(8 ),
/* SMEM_BOBCAT2_UNIT_EGF_SHT_9_E,               */ BOBCAT2_EGF_SHT_UNIT_BASE_ADDR_CNS(9 ),
/* SMEM_BOBCAT2_UNIT_EGF_SHT_10_E,              */ BOBCAT2_EGF_SHT_UNIT_BASE_ADDR_CNS(10),
/* SMEM_BOBCAT2_UNIT_EGF_SHT_11_E,              */ BOBCAT2_EGF_SHT_UNIT_BASE_ADDR_CNS(11),
/* SMEM_BOBCAT2_UNIT_EGF_SHT_12_E,              */ BOBCAT2_EGF_SHT_UNIT_BASE_ADDR_CNS(12),

/* SMEM_BOBCAT2_UNIT_HA_E,                      */ BOBCAT2_HA_UNIT_BASE_ADDR_CNS(0),
/* SMEM_BOBCAT2_UNIT_IPCL_E,                    */ BOBCAT2_IPCL_UNIT_BASE_ADDR_CNS,
/* SMEM_BOBCAT2_UNIT_EPCL_E,                    */ BOBCAT2_EPCL_UNIT_BASE_ADDR_CNS,


/* SMEM_BOBCAT2_UNIT_TCAM_E,                    */ BOBCAT2_TCAM_UNIT_BASE_ADDR_CNS,
/* SMEM_BOBCAT2_UNIT_TM_HOSTING_E,              */ BOBCAT2_TM_HOSTING_UNIT_BASE_ADDR_CNS,
/* SMEM_BOBCAT2_UNIT_ERMRK_E,                   */ BOBCAT2_ERMRK_UNIT_BASE_ADDR_CNS,

/* SMEM_BOBCAT2_UNIT_GOP_E,                 */ BOBCAT2_NEW_GOP_UNIT_BASE_ADDR_CNS,

/* SMEM_BOBCAT2_UNIT_IPLR0_E,                   */ BOBCAT2_IPLR0_UNIT_BASE_ADDR_CNS,
/* SMEM_BOBCAT2_UNIT_IPLR1_E,                   */ BOBCAT2_IPLR1_UNIT_BASE_ADDR_CNS,
/* SMEM_BOBCAT2_UNIT_EPLR_E,                    */ BOBCAT2_EPLR_UNIT_BASE_ADDR_CNS,

/* SMEM_BOBCAT2_UNIT_CNC_0_E,                   */ BOBCAT2_CNC_0_UNIT_BASE_ADDR_CNS,
/* SMEM_BOBCAT2_UNIT_CNC_1_E,                   */ BOBCAT2_CNC_1_UNIT_BASE_ADDR_CNS,

/* SMEM_BOBCAT2_UNIT_TMDROP_E                   */ BOBCAT2_TMDROP_UNIT_BASE_ADDR_CNS

,/*SMEM_BOBCAT2_UNIT_TXQ_QUEUE_E,               */ BOBCAT2_TXQ_QUEUE_UNIT_BASE_ADDR_CNS
,/*SMEM_BOBCAT2_UNIT_TXQ_LL_E,                  */ BOBCAT2_TXQ_LL_UNIT_BASE_ADDR_CNS
,/*SMEM_BOBCAT2_UNIT_TXQ_DQ_E,                  */ BOBCAT2_TXQ_DQ_UNIT_BASE_ADDR_CNS
,/*SMEM_BOBCAT2_UNIT_CPFC_E,                    */ BOBCAT2_CPFC_UNIT_BASE_ADDR_CNS
,/*SMEM_BOBCAT2_UNIT_TXQ_QCN_E                  */ BOBCAT2_TXQ_QCN_UNIT_BASE_ADDR_CNS
,/*SMEM_BOBCAT2_UNIT_TX_FIFO_E                  */ BOBCAT2_TX_FIFO_UNIT_BASE_ADDR_CNS
,/*SMEM_BOBCAT2_UNIT_ETH_TX_FIFO_E,             */ BOBCAT2_ETH_TX_FIFO_UNIT_BASE_ADDR_CNS
,/*SMEM_BOBCAT2_UNIT_ILKN_TX_FIFO_E,            */ BOBCAT2_ILKN_TX_FIFO_UNIT_BASE_ADDR_CNS



,/*SMEM_BOBCAT2_UNIT_TM_FCU_E                   */ BOBCAT2_TM_FCU_UNIT_BASE_ADDR_CNS
,/*SMEM_BOBCAT2_UNIT_TM_INGRESS_GLUE_E          */ BOBCAT2_TM_INGRESS_GLUE_UNIT_BASE_ADDR_CNS
,/*SMEM_BOBCAT2_UNIT_TM_EGRESS_GLUE_E           */ BOBCAT2_TM_EGRESS_GLUE_UNIT_BASE_ADDR_CNS
,/*SMEM_BOBCAT2_UNIT_TMQMAP_E                   */ BOBCAT2_TMQMAP_GLUE_UNIT_BASE_ADDR_CNS

,/*SMEM_BOBCAT2_UNIT_LMS_E,                     */ BOBCAT2_LMS_UNIT_BASE_ADDR_CNS
,/*SMEM_BOBCAT2_UNIT_LMS1_E,                    */ BOBCAT2_LMS1_UNIT_BASE_ADDR_CNS
,/*SMEM_BOBCAT2_UNIT_LMS2_E,                    */ BOBCAT2_LMS2_UNIT_BASE_ADDR_CNS
,/*SMEM_BOBCAT2_UNIT_GOP_ILKN_E                 */ BOBCAT2_GOP_ILKN_UNIT_BASE_ADDR_CNS

,/*SMEM_BOBCAT2_B0_UNIT_GOP_SMI_0_E,            */ SMEM_BOBCAT2_B0_BASE_ADDRESS_SMI_0_CNS
,/*SMEM_BOBCAT2_B0_UNIT_GOP_SMI_1_E,            */ SMEM_BOBCAT2_B0_BASE_ADDRESS_SMI_1_CNS
,/*SMEM_BOBCAT2_B0_UNIT_GOP_SMI_2_E,            */ SMEM_BOBCAT2_B0_BASE_ADDRESS_SMI_2_CNS
,/*SMEM_BOBCAT2_B0_UNIT_GOP_SMI_3_E,            */ SMEM_BOBCAT2_B0_BASE_ADDRESS_SMI_3_CNS
,/*SMEM_BOBCAT2_B0_UNIT_GOP_LED_0_E,            */ SMEM_BOBCAT2_B0_BASE_ADDRESS_LED_0_CNS
,/*SMEM_BOBCAT2_B0_UNIT_GOP_LED_1_E,            */ SMEM_BOBCAT2_B0_BASE_ADDRESS_LED_1_CNS
,/*SMEM_BOBCAT2_B0_UNIT_GOP_LED_2_E,            */ SMEM_BOBCAT2_B0_BASE_ADDRESS_LED_2_CNS
,/*SMEM_BOBCAT2_B0_UNIT_GOP_LED_3_E,            */ SMEM_BOBCAT2_B0_BASE_ADDRESS_LED_3_CNS
,/*SMEM_BOBCAT2_B0_UNIT_GOP_LED_4_E,            */ SMEM_BOBCAT2_B0_BASE_ADDRESS_LED_4_CNS


};

/* array to remove units from bobcat2UsedUnitsAddressesArray for bc2_A0 */
static SMEM_BOBCAT2_UNIT_NAME_ENT bc2_A0_not_supported_units[] =
{
    SMEM_BOBCAT2_B0_UNIT_GOP_SMI_0_E,
    SMEM_BOBCAT2_B0_UNIT_GOP_SMI_1_E,
    SMEM_BOBCAT2_B0_UNIT_GOP_SMI_2_E,
    SMEM_BOBCAT2_B0_UNIT_GOP_SMI_3_E,
    SMEM_BOBCAT2_B0_UNIT_GOP_LED_0_E,
    SMEM_BOBCAT2_B0_UNIT_GOP_LED_1_E,
    SMEM_BOBCAT2_B0_UNIT_GOP_LED_2_E,
    SMEM_BOBCAT2_B0_UNIT_GOP_LED_3_E,
    SMEM_BOBCAT2_B0_UNIT_GOP_LED_4_E,

    SMEM_BOBCAT2_UNIT_LAST_E/* must be last */
};
/* array to remove units from bobcat2UsedUnitsAddressesArray for bc2_B0 */
static SMEM_BOBCAT2_UNIT_NAME_ENT bc2_B0_not_supported_units[] =
{
    SMEM_BOBCAT2_UNIT_LMS_E,
    SMEM_BOBCAT2_UNIT_LMS1_E,
    SMEM_BOBCAT2_UNIT_LMS2_E,

    SMEM_BOBCAT2_UNIT_LAST_E/* must be last */
};



static SMEM_UNIT_NAME_AND_INDEX_STC bobcat2UnitNameAndIndexArr[]=
{
    {STR(UNIT_MG),                         SMEM___LEGACY___BOBCAT2_UNIT_MG_E                    },
    {STR(UNIT_IPLR1),                      SMEM_BOBCAT2_UNIT_IPLR1_E                            },/*bobcat2*/
    {STR(UNIT_TTI),                        SMEM___LEGACY___BOBCAT2_UNIT_TTI_E                   },
    {STR(UNIT_DFX_SERVER),                 SMEM___LEGACY___BOBCAT2_UNIT_DFX_SERVER_E            },
    {STR(UNIT_L2I),                        SMEM___LEGACY___BOBCAT2_UNIT_L2I_E                   },
    {STR(UNIT_IPVX),                       SMEM___LEGACY___BOBCAT2_UNIT_IPVX_E                  },
    {STR(UNIT_BM),                         SMEM___LEGACY___BOBCAT2_UNIT_BM_E                    },
    {STR(UNIT_BMA),                        SMEM___LEGACY___BOBCAT2_UNIT_BMA_E                   },
    {STR(UNIT_LMS),                        SMEM_BOBCAT2_UNIT_LMS_E                   },
    {STR(UNIT_FDB),                        SMEM___LEGACY___BOBCAT2_UNIT_FDB_E                   },
    {STR(UNIT_MPPM),                       SMEM___LEGACY___BOBCAT2_UNIT_MPPM_E                  },
    {STR(UNIT_CTU0),                       SMEM___LEGACY___BOBCAT2_UNIT_CTU0_E                  },
    {STR(UNIT_EPLR),                       SMEM_BOBCAT2_UNIT_EPLR_E                             },/*bobcat2*/
    {STR(UNIT_CNC),                        SMEM_BOBCAT2_UNIT_CNC_0_E                            },/*bobcat2*/
    {STR(UNIT_GOP),                        SMEM___LEGACY___BOBCAT2_UNIT_GOP_E                   },
    {STR(UNIT_XG_PORT_MIB),                SMEM___LEGACY___BOBCAT2_UNIT_XG_PORT_MIB_E           },
    {STR(UNIT_SERDES),                     SMEM___LEGACY___BOBCAT2_UNIT_SERDES_E                },
    {STR(UNIT_MPPM1),                      SMEM___LEGACY___BOBCAT2_UNIT_MPPM1_E                 },
    {STR(UNIT_CTU1),                       SMEM___LEGACY___BOBCAT2_UNIT_CTU1_E                  },
    {STR(UNIT_EQ),                         SMEM_BOBCAT2_UNIT_EQ_0_E                             },/*bobcat2*/
    {STR(UNIT_IPCL),                       SMEM_BOBCAT2_UNIT_IPCL_E                             },/*bobcat2*/
    {STR(UNIT_IPLR),                       SMEM_BOBCAT2_UNIT_IPLR0_E                            },/*bobcat2*/
    {STR(UNIT_MLL),                        SMEM_BOBCAT2_UNIT_MLL_E                              },/*bobcat2*/
    {STR(UNIT_IPCL_TCC),                   SMEM___LEGACY___BOBCAT2_UNIT_IPCL_TCC_E              },
    {STR(UNIT_IPVX_TCC),                   SMEM___LEGACY___BOBCAT2_UNIT_IPVX_TCC_E              },
    {STR(UNIT_EPCL),                       SMEM_BOBCAT2_UNIT_EPCL_E                             },/*bobcat2*/
    {STR(UNIT_HA),                         SMEM_BOBCAT2_UNIT_HA_E                               },/*bobcat2*/
    {STR(UNIT_RX_DMA),                     SMEM___LEGACY___BOBCAT2_UNIT_RX_DMA_E                },
    {STR(UNIT_TX_DMA),                     SMEM___LEGACY___BOBCAT2_UNIT_TX_DMA_E                },

    /*bobcat2*/
    {STR(UNIT_IOAM),                       SMEM_BOBCAT2_UNIT_IOAM_E                  },
    {STR(UNIT_EOAM),                       SMEM_BOBCAT2_UNIT_EOAM_E                  },
    {STR(UNIT_LPM),                        SMEM_BOBCAT2_UNIT_LPM_E                   },
    {STR(UNIT_EQ_1),                       SMEM_BOBCAT2_UNIT_EQ_1_E                  },
    {STR(UNIT_FDB_TABLE_0),                SMEM_BOBCAT2_UNIT_FDB_TABLE_1_E           },
    {STR(UNIT_FDB_TABLE_1),                SMEM_BOBCAT2_UNIT_FDB_TABLE_1_E           },
    {STR(UNIT_EGF_EFT),                  SMEM_BOBCAT2_UNIT_EGF_EFT_E             },
    {STR(UNIT_EGF_EFT_1),                  SMEM_BOBCAT2_UNIT_EGF_EFT_1_E             },
    {STR(UNIT_EGF_QAG),                  SMEM_BOBCAT2_UNIT_EGF_QAG_E             },
    {STR(UNIT_EGF_QAG_1),                  SMEM_BOBCAT2_UNIT_EGF_QAG_1_E             },
    {STR(UNIT_EGF_SHT),                  SMEM_BOBCAT2_UNIT_EGF_SHT_E             },
    {STR(UNIT_EGF_SHT_1),                  SMEM_BOBCAT2_UNIT_EGF_SHT_1_E             },
    {STR(UNIT_EGF_SHT_2),                  SMEM_BOBCAT2_UNIT_EGF_SHT_2_E             },
    {STR(UNIT_EGF_SHT_3),                  SMEM_BOBCAT2_UNIT_EGF_SHT_3_E             },
    {STR(UNIT_EGF_SHT_4),                  SMEM_BOBCAT2_UNIT_EGF_SHT_4_E             },
    {STR(UNIT_EGF_SHT_5),                  SMEM_BOBCAT2_UNIT_EGF_SHT_5_E             },
    {STR(UNIT_EGF_SHT_6),                  SMEM_BOBCAT2_UNIT_EGF_SHT_6_E             },
    {STR(UNIT_EGF_SHT_7),                  SMEM_BOBCAT2_UNIT_EGF_SHT_7_E             },
    {STR(UNIT_EGF_SHT_8),                  SMEM_BOBCAT2_UNIT_EGF_SHT_8_E             },
    {STR(UNIT_EGF_SHT_9),                  SMEM_BOBCAT2_UNIT_EGF_SHT_9_E             },
    {STR(UNIT_EGF_SHT_10),                 SMEM_BOBCAT2_UNIT_EGF_SHT_10_E            },
    {STR(UNIT_EGF_SHT_11),                 SMEM_BOBCAT2_UNIT_EGF_SHT_11_E            },
    {STR(UNIT_EGF_SHT_12),                 SMEM_BOBCAT2_UNIT_EGF_SHT_12_E            },
    {STR(UNIT_TCAM),                       SMEM_BOBCAT2_UNIT_TCAM_E                  },
    {STR(UNIT_TM_HOSTING),                 SMEM_BOBCAT2_UNIT_TM_HOSTING_E            },
    {STR(UNIT_ERMRK),                      SMEM_BOBCAT2_UNIT_ERMRK_E                 },
    {STR(UNIT_GOP),                    SMEM_BOBCAT2_UNIT_GOP_E               },
    {STR(UNIT_CNC_1),                      SMEM_BOBCAT2_UNIT_CNC_1_E                 },

    {STR(UNIT_TMDROP),                     SMEM_BOBCAT2_UNIT_TMDROP_E                },

    {STR(UNIT_TXQ_QUEUE),                  SMEM_BOBCAT2_UNIT_TXQ_QUEUE_E            },
    {STR(UNIT_TXQ_LL),                     SMEM_BOBCAT2_UNIT_TXQ_LL_E               },
    {STR(UNIT_TXQ_DQ),                     SMEM_BOBCAT2_UNIT_TXQ_DQ_E               },
    {STR(UNIT_CPFC),                       SMEM_BOBCAT2_UNIT_CPFC_E                 },
    {STR(UNIT_TXQ_QCN),                    SMEM_BOBCAT2_UNIT_TXQ_QCN_E              },

    {STR(UNIT_LMS1),                       SMEM_BOBCAT2_UNIT_LMS1_E                 },
    {STR(UNIT_LMS2),                       SMEM_BOBCAT2_UNIT_LMS2_E                 },

    {STR(UNIT_TX_FIFO),                    SMEM_BOBCAT2_UNIT_TX_FIFO_E             },
    {STR(UNIT_ETH_TX_FIFO),                SMEM_BOBCAT2_UNIT_ETH_TX_FIFO_E         },
    {STR(UNIT_ILKN_TX_FIFO),                SMEM_BOBCAT2_UNIT_ILKN_TX_FIFO_E        },

    {STR(UNIT_TM_FCU),                     SMEM_BOBCAT2_UNIT_TM_FCU_E               },
    {STR(UNIT_TM_INGRESS_GLUE),            SMEM_BOBCAT2_UNIT_TM_INGRESS_GLUE_E               },
    {STR(UNIT_TM_EGRESS_GLUE),             SMEM_BOBCAT2_UNIT_TM_EGRESS_GLUE_E               },
    {STR(UNIT_TMQMAP),                     SMEM_BOBCAT2_UNIT_TMQMAP_E               },

    {STR(UNIT_GOP_ILKN),                    SMEM_BOBCAT2_UNIT_GOP_ILKN_E               },

    {STR(UNIT_GOP_SMI_0),                  SMEM_BOBCAT2_B0_UNIT_GOP_SMI_0_E      },
    {STR(UNIT_GOP_SMI_1),                  SMEM_BOBCAT2_B0_UNIT_GOP_SMI_1_E      },
    {STR(UNIT_GOP_SMI_2),                  SMEM_BOBCAT2_B0_UNIT_GOP_SMI_2_E      },
    {STR(UNIT_GOP_SMI_3),                  SMEM_BOBCAT2_B0_UNIT_GOP_SMI_3_E      },
    {STR(UNIT_GOP_LED_0),                  SMEM_BOBCAT2_B0_UNIT_GOP_LED_0_E      },
    {STR(UNIT_GOP_LED_1),                  SMEM_BOBCAT2_B0_UNIT_GOP_LED_1_E      },
    {STR(UNIT_GOP_LED_2),                  SMEM_BOBCAT2_B0_UNIT_GOP_LED_2_E      },
    {STR(UNIT_GOP_LED_3),                  SMEM_BOBCAT2_B0_UNIT_GOP_LED_3_E      },
    {STR(UNIT_GOP_LED_4),                  SMEM_BOBCAT2_B0_UNIT_GOP_LED_4_E      },

    {NULL ,                                SMAIN_NOT_VALID_CNS                     }
};

/* array of the '23 MSBits' (after the 20 LSBits) of addresses of units of the 'TM-device' */
/*addresses must be in ascending order (to allow binary search)*/
static GT_U32 tmDeviceSubUnitsAddrArr[] =
{
/*1 */     0x00000
/*2 */    ,0x08000
/*3 */    ,0x08001
/*4 */    ,0x09000
/*5 */    ,0x09001
/*6 */    ,0x09002
/*7 */    ,0x09003
/*8 */    ,0x09004
/*  */
/*9 */    ,0x10000
/*10*/    ,0x10008
/*11*/    ,0x10010
/*12*/    ,0x10018
/*13*/    ,0x10020
/*14*/    ,0x10028
/*15*/    ,0x10088
/*16*/    ,0x10090
/*17*/    ,0x10098
/*18*/    ,0x100A0
/*19*/    ,0x100A8
/*20*/    ,0x18000
/*  */
/*21*/    ,0x20000
/*22*/    ,0x21000
/*23*/    ,0x28000
/*24*/    ,0x29000
/*  */
/*25*/    ,0x30000
/*26*/    ,0x31000
/*27*/    ,0x38000
/*28*/    ,0x39000
/*  */
/*29*/    ,0x40000
/*30*/    ,0x41000
/*31*/    ,0x48000
/*32*/    ,0x49000
/*  */
/*33*/    ,0x50000
/*34*/    ,0x51000
/*35*/    ,0x58000
/*36*/    ,0x59000
/*  */
/*37*/    ,0x60000
/*38*/    ,0x61000
/*39*/    ,0x68000
/*40*/    ,0x69000
/*  */
/*41*/    ,0x80000
/*42*/    ,0x80008
/*43*/    ,0x80010
/*44*/    ,0x80018
/*45*/    ,0x80020
/*46*/    ,0x80028
/*47*/    ,0x80029
/*48*/    ,0x8002A
/*49*/    ,0x80088
/*50*/    ,0x80090
/*51*/    ,0x80098
/*52*/    ,0x800A0
/*53*/    ,0x800A8
/*  */
/*54*/    ,0x81000
/*55*/    ,0x81008
/*56*/    ,0x81010
/*57*/    ,0x81018
/*58*/    ,0x81020
/*59*/    ,0x81028
/*60*/    ,0x81029
/*  */
/*61*/    ,0x88000
/*62*/    ,0x88088
/*63*/    ,0x880A8
};
/* number of elements in tmDeviceSubUnitsAddrArr */
static GT_U32 tmDeviceSubUnitsAddrArr_numOfEntries = sizeof(tmDeviceSubUnitsAddrArr) / sizeof(tmDeviceSubUnitsAddrArr[0]);

#define MASK_23_BITS_CNS 0x7FFFFF

GT_U32 smemBobcat2RxDmaUnitIndexFromAddressGet (
    IN  SKERNEL_DEVICE_OBJECT *devObjPtr,
    IN  GT_U32                 address
)
{
    GT_U32 unit1Base = SIP_5_10_REG(devObjPtr,rxDMA[1].startUnitInfo.unitBaseAddress);

    if ((unit1Base >> 23) == (address >> 23))
    {
        return 1;/* unit index 1 */
    }

    return 0;/* unit index 0 */
}

/*******************************************************************************
*  smemBobcat2ActiveWriteRxDmaGlobalCntrsClear
*
* DESCRIPTION:
*       sip5_10 : RxDma All global counters may be cleared independently or all together.
*           as long as bit 0 is '0' the global 'low' counters not count
*           as long as bit 1 is '0' the global 'Med' counters not count
*           as long as bit 2 is '0' the global 'High' counters not count
* INPUTS:
*       devObjPtr   - Device object PTR.
*       address     - Address for ASIC memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers' specific parameter.
*       inMemPtr    - Pointer to the memory to get register's content.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void smemBobcat2ActiveWriteRxDmaGlobalCntrsClear (
    IN  SKERNEL_DEVICE_OBJECT *devObjPtr,
    IN  GT_U32                 address,
    IN  GT_U32                 memSize,
    IN  GT_U32                *memPtr,
    IN  GT_UINTPTR             param,
    IN  GT_U32                *inMemPtr
)
{
    GT_U32  regValue = *inMemPtr;
    GT_U32  regAddr;
    GT_U32  *regPtr;
    GT_U32  unitIndex = smemBobcat2RxDmaUnitIndexFromAddressGet(devObjPtr,address);

    if (SMEM_U32_GET_FIELD(regValue,0,1))/* Low */
    {
        regAddr = SMEM_SIP_5_10_RXDMA_PIP_LOW_PRIORITY_DROP_GLOBAL_COUNTER_LSB_REG(devObjPtr,unitIndex);
        regPtr = smemMemGet(devObjPtr, regAddr);
        /* clear the 64 bits counter ... and runtime will not count it */
        regPtr[0] = regPtr[1] = 0;
    }

    if (SMEM_U32_GET_FIELD(regValue,1,1)) /* Medium */
    {
        regAddr = SMEM_SIP_5_10_RXDMA_PIP_MED_PRIORITY_DROP_GLOBAL_COUNTER_LSB_REG(devObjPtr,unitIndex);
        regPtr = smemMemGet(devObjPtr, regAddr);
        /* clear the 64 bits counter ... and runtime will not count it */
        regPtr[0] = regPtr[1] = 0;
    }

    if(SMEM_U32_GET_FIELD(regValue,2,1))/* High */
    {
        regAddr = SMEM_SIP_5_10_RXDMA_PIP_HIGH_PRIORITY_DROP_GLOBAL_COUNTER_LSB_REG(devObjPtr,unitIndex);
        regPtr = smemMemGet(devObjPtr, regAddr);
        /* clear the 64 bits counter ... and runtime will not count it */
        regPtr[0] = regPtr[1] = 0;
    }


    return;
}


/*compare addresses for the 'bsearch' function */
static int comparAddressesFunc (const void *p1, const void *p2)
{
    GT_32  *addr1Ptr = (GT_32  *)p1;
    GT_32  *addr2Ptr = (GT_32  *)p2;

   return ((*addr1Ptr) - (*addr2Ptr));
}

/* return index in unitsAddrArr[] that hold current23MSBitsAfter20LSBits */
static GT_U32   tmDeviceFindUnitIndex(
    IN GT_U32   unitsAddrArr[],
    IN GT_U32   numElements,
    IN GT_U32   current23MSBitsAfter20LSBits
)
{
    GT_U32 *foundAddrPtr;

    foundAddrPtr = bsearch(&current23MSBitsAfter20LSBits, unitsAddrArr, numElements, sizeof(GT_U32), comparAddressesFunc);

    if (foundAddrPtr == NULL)
    {
        skernelFatalError(" not found the base address[0x%8.8x]" ,current23MSBitsAfter20LSBits);
    }

    /* deducting of pointers results in indexes */
    return (foundAddrPtr - unitsAddrArr);
}


/*******************************************************************************
*   smemTmDeviceInit
*
* DESCRIPTION:
*       Init memory module for the 'TM-device' that is part of the hosting device - bobcat2 .
*
* INPUTS:
*       devObjPtr   - pointer to device object. (the hosting device - bobcat2)
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemTmDeviceInit
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = devObjPtr->deviceMemory;
    GT_U32      ii;
    GT_U32      previousAddr = 0;

    /* attach the device (the hosting device - bobcat2) to the 'TM-device' */

    devMemInfoPtr->numOfExtraUnits = tmDeviceSubUnitsAddrArr_numOfEntries;
    devMemInfoPtr->extraUnitsPtr = smemDeviceObjMemoryAlloc(devObjPtr,devMemInfoPtr->numOfExtraUnits, sizeof(SMEM_UNIT_CHUNKS_STC));
    if(devMemInfoPtr->extraUnitsPtr == NULL)
    {
        skernelFatalError(" allocation of memory failed");

        return;
    }

    /* state that those chunks are for '43bits' addresses, and the value of the 23MSBits */
    for(ii = 0 ; ii < devMemInfoPtr->numOfExtraUnits; ii++)
    {
        if(tmDeviceSubUnitsAddrArr[ii] > MASK_23_BITS_CNS)
        {
            skernelFatalError("addresses must be up to 23 bits");
        }

        if(ii == 0)
        {
            previousAddr = 0;
        }
        else if(previousAddr >= tmDeviceSubUnitsAddrArr[ii])
        {
            skernelFatalError("addresses must be in ascending order (to allow binary search)");
        }

        devMemInfoPtr->extraUnitsPtr[ii].chunkIndex = tmDeviceSubUnitsAddrArr[ii];
        devMemInfoPtr->extraUnitsPtr[ii].chunkType = SMEM_UNIT_CHUNK_TYPE_23_MSB_AFTER_20_LSB_E;
    }

    /*allocate the chunks of all sub units */
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_SUB_UNIT_AFTER_20_LSB_MAC (0x00000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000, 0x0000C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00018, 0x0001C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_SUB_UNIT_AFTER_20_LSB_MAC (0x08000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000, 0x00044)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00080, 0x0009C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00100, 0x0011C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00800, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01000, 0x0102C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01080, 0x0108C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01100, 0x0110C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01180, 0x01184)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x01800, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02000, 0x0204C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02080, 0x02084)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02100, 0x0212C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x02180, 32)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x021A0, 0x021AC)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x03000, 0x03004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x08800, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x08E00, 0x08E04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x09000, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x09200, 0x09204)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x09800, 0x0985C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x10000, 768)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x10400, 768)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x18000, 0x18004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x20000, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x80000, 131072)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_SUB_UNIT_AFTER_20_LSB_MAC (0x08001)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000, 131072)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_SUB_UNIT_AFTER_20_LSB_MAC (0x09000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x18008, 0x1804C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_SUB_UNIT_AFTER_20_LSB_MAC (0x09001)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x80000, 131072)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_SUB_UNIT_AFTER_20_LSB_MAC (0x09002)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000, 131072)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x80000, 131072)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_SUB_UNIT_AFTER_20_LSB_MAC (0x09003)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000, 131072)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x80000, 131072)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_SUB_UNIT_AFTER_20_LSB_MAC (0x09004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00800, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_SUB_UNIT_AFTER_20_LSB_MAC (0x10000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000, 0x00034)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00088, 0x000B4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00100, 0x0010C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00180, 0x00184)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_SUB_UNIT_AFTER_20_LSB_MAC (0x10008)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00600, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00800, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00E00, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x01000, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x01600, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x01800, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x01E00, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x02000, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x02600, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x02800, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x02E00, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x03000, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x03600, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x03800, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x04000, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x04600, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x04800, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x04E00, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x05000, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x05600, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x05800, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x05E00, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x06000, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x06600, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x06800, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x06E00, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x07000, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x07600, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x07800, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x08000, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x08600, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x08800, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x08E00, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x09000, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x09600, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x09800, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x09E00, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0A000, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0A600, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0A800, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0AE00, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0B000, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0B600, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0B800, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0C000, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0C600, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0C800, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0CE00, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0D000, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0D600, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0D800, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0DE00, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0E000, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0E600, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0E800, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0EE00, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0F000, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0F600, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0F800, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x10000, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x10600, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x10800, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x10E00, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x11000, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x11600, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x11800, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x11E00, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x12000, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x12600, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x12800, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x13000, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x13600, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x13800, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x18000, 1024)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x18400, 1024)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x18800, 1024)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x18C00, 1024)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x19000, 1024)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x19400, 1024)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x19800, 1024)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x19C00, 1024)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x40000, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x40800, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x41000, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x41800, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x42000, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x48000, 1024)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_SUB_UNIT_AFTER_20_LSB_MAC (0x10010)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00200, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00400, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00600, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00800, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00A00, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00C00, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00E00, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x01000, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x01200, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x01400, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x01600, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x01800, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x01A00, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x01C00, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x01E00, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x02000, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x02200, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x02400, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x02600, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x02800, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x02A00, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x02C00, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x02E00, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x03000, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x03200, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x03400, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x03600, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x03800, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x03A00, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x03C00, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x03E00, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x04000, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x04200, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x04400, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x04600, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x04800, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x04A00, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x04C00, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x04E00, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x05000, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x05200, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x05400, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x05600, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x05800, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x05A00, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x05C00, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x05E00, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x08000, 1024)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x08400, 1024)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x08800, 1024)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x08C00, 1024)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x09000, 1024)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x09400, 1024)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x09800, 1024)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x09C00, 1024)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_SUB_UNIT_AFTER_20_LSB_MAC (0x10018)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000, 2048)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x01000, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x01200, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x01400, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x01600, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x01800, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x02000, 256)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x02100, 256)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x02200, 256)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x02300, 256)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_SUB_UNIT_AFTER_20_LSB_MAC (0x10020)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000, 8192)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x08000, 2048)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x09000, 2048)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0A000, 2048)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0A800, 2048)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0B000, 2048)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0C000, 2048)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0C800, 2048)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0D000, 2048)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_SUB_UNIT_AFTER_20_LSB_MAC (0x10028)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000, 32768)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x20000, 16384)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x24000, 16384)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x28000, 16384)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x2C000, 16384)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x30000, 16384)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x34000, 2048)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x34800, 2048)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x35000, 2048)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x80000, 32768)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_SUB_UNIT_AFTER_20_LSB_MAC (0x10088)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00800, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x04000, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x04600, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x04800, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x04E00, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x05000, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x05600, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x05800, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x05E00, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x06000, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x06600, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x06800, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x06E00, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x07000, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x07600, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x07800, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x08000, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x08600, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x08800, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x08E00, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x09000, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x09600, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x09800, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x09E00, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0A000, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0A600, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0A800, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0AE00, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0B000, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0B600, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0B800, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_SUB_UNIT_AFTER_20_LSB_MAC (0x10090)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000, 4096)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x08000, 32768)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_SUB_UNIT_AFTER_20_LSB_MAC (0x10098)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000, 16384)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x08000, 16384)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_SUB_UNIT_AFTER_20_LSB_MAC (0x100A0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000, 32768)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x20000, 32768)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_SUB_UNIT_AFTER_20_LSB_MAC (0x100A8)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000, 131072)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x80000, 131072)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_SUB_UNIT_AFTER_20_LSB_MAC (0x18000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000, 0x00034)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00080, 0x00084)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00100, 0x00104)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00800, 32)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00820, 0x0082C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01000, 0x0101C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01800, 0x0180C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02008, 0x0200C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x10000, 0x1002C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x10050, 0x10064)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x10080, 64)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x10100, 48)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_SUB_UNIT_AFTER_20_LSB_MAC (0x20000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000, 0x00044)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00050, 0x00064)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00080, 0x000FC)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00180, 0x001F4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00200, 0x00214)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00280, 0x00284)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00290, 16)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x002A0, 0x002A4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x002B0, 16)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x002C0, 0x002C4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x002D0, 16)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x002E0, 0x002E4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x002F0, 16)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00300, 0x0032C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00800, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01000, 0x01004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02000, 0x02004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02800, 0x02814)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_SUB_UNIT_AFTER_20_LSB_MAC (0x21000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000, 0x00014)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00080, 0x000AC)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_SUB_UNIT_AFTER_20_LSB_MAC (0x28000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000, 0x00044)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00050, 0x00064)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00080, 0x000FC)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00180, 0x001F4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00200, 0x00214)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00280, 0x00284)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00290, 16)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x002A0, 0x002A4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x002B0, 16)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x002C0, 0x002C4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x002D0, 16)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x002E0, 0x002E4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x002F0, 16)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00300, 0x0032C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00800, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01000, 0x01004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02000, 0x02004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02800, 0x02814)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_SUB_UNIT_AFTER_20_LSB_MAC (0x29000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000, 0x00014)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00080, 0x000AC)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_SUB_UNIT_AFTER_20_LSB_MAC (0x30000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000, 0x00044)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00050, 0x00064)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00080, 0x000FC)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00180, 0x001F4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00200, 0x00214)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00280, 0x00284)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00290, 16)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x002A0, 0x002A4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x002B0, 16)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x002C0, 0x002C4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x002D0, 16)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x002E0, 0x002E4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x002F0, 16)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00300, 0x0032C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00800, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01000, 0x01004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02000, 0x02004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02800, 0x02814)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_SUB_UNIT_AFTER_20_LSB_MAC (0x31000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000, 0x00014)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00080, 0x000AC)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_SUB_UNIT_AFTER_20_LSB_MAC (0x38000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000, 0x00044)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00050, 0x00064)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00080, 0x000FC)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00180, 0x001F4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00200, 0x00214)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00280, 0x00284)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00290, 16)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x002A0, 0x002A4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x002B0, 16)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x002C0, 0x002C4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x002D0, 16)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x002E0, 0x002E4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x002F0, 16)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00300, 0x0032C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00800, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01000, 0x01004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02000, 0x02004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02800, 0x02814)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_SUB_UNIT_AFTER_20_LSB_MAC (0x39000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000, 0x00014)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00080, 0x000AC)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_SUB_UNIT_AFTER_20_LSB_MAC (0x40000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000, 0x00044)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00050, 0x00064)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00080, 0x000FC)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00180, 0x001F4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00200, 0x00214)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00280, 0x00284)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00290, 16)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x002A0, 0x002A4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x002B0, 16)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x002C0, 0x002C4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x002D0, 16)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x002E0, 0x002E4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x002F0, 16)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00300, 0x0032C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00800, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01000, 0x01004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02000, 0x02004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02800, 0x02814)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_SUB_UNIT_AFTER_20_LSB_MAC (0x41000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000, 0x00014)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00080, 0x000AC)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_SUB_UNIT_AFTER_20_LSB_MAC (0x48000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000, 0x00044)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00050, 0x00064)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00080, 0x000FC)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00180, 0x001F4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00200, 0x00214)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00280, 0x00284)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00290, 16)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x002A0, 0x002A4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x002B0, 16)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x002C0, 0x002C4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x002D0, 16)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x002E0, 0x002E4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x002F0, 16)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00300, 0x0032C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00800, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01000, 0x01004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02000, 0x02004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02800, 0x02814)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_SUB_UNIT_AFTER_20_LSB_MAC (0x49000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000, 0x00014)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00080, 0x000AC)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_SUB_UNIT_AFTER_20_LSB_MAC (0x50000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000, 0x00044)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00050, 0x00064)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00080, 0x000FC)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00180, 0x001F4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00200, 0x00214)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00280, 0x00284)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00290, 16)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x002A0, 0x002A4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x002B0, 16)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x002C0, 0x002C4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x002D0, 16)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x002E0, 0x002E4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x002F0, 16)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00300, 0x0032C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00800, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01000, 0x01004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02000, 0x02004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02800, 0x02814)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_SUB_UNIT_AFTER_20_LSB_MAC (0x51000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000, 0x00014)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00080, 0x000AC)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_SUB_UNIT_AFTER_20_LSB_MAC (0x58000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000, 0x00044)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00050, 0x00064)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00080, 0x000FC)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00180, 0x001F4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00200, 0x00214)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00280, 0x00284)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00290, 16)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x002A0, 0x002A4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x002B0, 16)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x002C0, 0x002C4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x002D0, 16)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x002E0, 0x002E4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x002F0, 16)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00300, 0x0032C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00800, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01000, 0x01004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02000, 0x02004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02800, 0x02814)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_SUB_UNIT_AFTER_20_LSB_MAC (0x59000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000, 0x00014)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00080, 0x000AC)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_SUB_UNIT_AFTER_20_LSB_MAC (0x60000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000, 0x00044)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00050, 0x00064)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00080, 0x000FC)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00180, 0x001F4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00200, 0x00214)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00280, 0x00284)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00290, 16)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x002A0, 0x002A4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x002B0, 16)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x002C0, 0x002C4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x002D0, 16)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x002E0, 0x002E4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x002F0, 16)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00300, 0x0032C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00800, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01000, 0x01004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02000, 0x02004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02800, 0x02814)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_SUB_UNIT_AFTER_20_LSB_MAC (0x61000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000, 0x00014)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00080, 0x000AC)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_SUB_UNIT_AFTER_20_LSB_MAC (0x68000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000, 0x00044)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00050, 0x00064)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00080, 0x000FC)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00180, 0x001F4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00200, 0x00214)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00280, 0x00284)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00290, 16)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x002A0, 0x002A4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x002B0, 16)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x002C0, 0x002C4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x002D0, 16)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x002E0, 0x002E4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x002F0, 16)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00300, 0x0032C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00800, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01000, 0x01004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02000, 0x02004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02800, 0x02814)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_SUB_UNIT_AFTER_20_LSB_MAC (0x69000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000, 0x00014)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00080, 0x000AC)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_SUB_UNIT_AFTER_20_LSB_MAC (0x80000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000, 0x00034)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00080, 0x0008C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x20000, 0x2000C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_SUB_UNIT_AFTER_20_LSB_MAC (0x80008)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000, 0x0001C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x01000, 4096)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x02000, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x02800, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x03000, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x03800, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x04000, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x04800, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x05000, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_SUB_UNIT_AFTER_20_LSB_MAC (0x80010)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000, 0x0000C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x01000, 4096)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x02000, 4096)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x03000, 4096)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x04000, 4096)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x05000, 4096)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x06000, 4096)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x07000, 4096)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_SUB_UNIT_AFTER_20_LSB_MAC (0x80018)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000, 0x0000C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x01000, 4096)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x08000, 16384)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x10000, 16384)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x18000, 16384)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x20000, 16384)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x28000, 16384)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x30000, 16384)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_SUB_UNIT_AFTER_20_LSB_MAC (0x80020)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000, 0x0000C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x01000, 4096)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x20000, 32768)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x40000, 32768)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x60000, 32768)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x80000, 32768)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0xA0000, 32768)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0xC0000, 32768)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_SUB_UNIT_AFTER_20_LSB_MAC (0x80028)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000, 0x0000C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00200, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x80000, 131072)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_SUB_UNIT_AFTER_20_LSB_MAC (0x80029)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000, 131072)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x80000, 131072)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_SUB_UNIT_AFTER_20_LSB_MAC (0x8002A)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000, 131072)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x80000, 131072)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_SUB_UNIT_AFTER_20_LSB_MAC (0x80088)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00800, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x01000, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_SUB_UNIT_AFTER_20_LSB_MAC (0x80090)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000, 4096)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x01000, 4096)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_SUB_UNIT_AFTER_20_LSB_MAC (0x80098)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000, 16384)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x08000, 16384)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_SUB_UNIT_AFTER_20_LSB_MAC (0x800A0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000, 32768)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x20000, 32768)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_SUB_UNIT_AFTER_20_LSB_MAC (0x800A8)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000, 131072)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x80000, 131072)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_SUB_UNIT_AFTER_20_LSB_MAC (0x81000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000, 0x00004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00080, 0x00084)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00800, 0x00804)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_SUB_UNIT_AFTER_20_LSB_MAC (0x81008)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00800, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x01000, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x01800, 1536)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x02000, 48)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02800, 0x02814)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x02880, 24)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x028A0, 24)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028C0, 0x028C4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_SUB_UNIT_AFTER_20_LSB_MAC (0x81010)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000, 4096)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x01000, 4096)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x02000, 4096)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x03000, 4096)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x04000, 128)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x04100, 256)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x04800, 0x0481C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x04880, 64)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x05000, 4096)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x06000, 0x06004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_SUB_UNIT_AFTER_20_LSB_MAC (0x81018)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x18000, 16384)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x20000, 16384)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x28000, 16384)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x30000, 16384)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x38000, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x38800, 1024)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x39000, 128)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x39080, 128)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x40000, 0x40024)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_SUB_UNIT_AFTER_20_LSB_MAC (0x81020)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000, 32768)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x20000, 32768)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x40000, 32768)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x60000, 32768)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x80000, 1024)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x88000, 4096)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x8C000, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x8C800, 512)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x8D000, 128)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x8D080, 128)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x90000, 0x90024)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_SUB_UNIT_AFTER_20_LSB_MAC (0x81028)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000, 131072)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x80000, 4096)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x88000, 16384)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x90000, 2048)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x90800, 2048)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x91000, 128)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x91080, 128)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x98000, 0x9800C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x98080, 0x9809C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_SUB_UNIT_AFTER_20_LSB_MAC (0x81029)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000, 131072)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x80000, 32)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_SUB_UNIT_AFTER_20_LSB_MAC (0x88000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000, 0x00034)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00100, 0x00124)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00140, 32)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00160, 0x00184)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00190, 0x001B4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00800, 0x0080C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01000, 0x01004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_SUB_UNIT_AFTER_20_LSB_MAC (0x88088)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x02000, 1792)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_SUB_UNIT_AFTER_20_LSB_MAC (0x880A8)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000, 262144)},
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);
        GT_U32  current23MSBitsAfter20LSBits = 0;
        SMEM_CHUNK_BASIC_STC    *currentChunksPtr = &chunksMem[0];
        GT_U32 currentChunksArrNumElements = 0;
        GT_U32 unitIndex;

        for(ii = 0; ii < numOfChunks; ii++)
        {
            if(chunksMem[ii].numOfRegisters == SMAM_SUB_UNIT_AFTER_20_LSB_CNS)
            {
processEndOfUnit_lbl:
                if(currentChunksArrNumElements)
                {
                    /* reached the end of previous sub unit */
                    /* so need to generate it's allocation */

                    /* find this address in the array of tmDeviceSubUnitsAddrArr[] */
                    unitIndex = tmDeviceFindUnitIndex(tmDeviceSubUnitsAddrArr,
                            tmDeviceSubUnitsAddrArr_numOfEntries,
                            current23MSBitsAfter20LSBits);

                    /* use the unit index to init the proper unit */
                    smemInitMemChunk(devObjPtr,currentChunksPtr,
                        currentChunksArrNumElements,
                        &devMemInfoPtr->extraUnitsPtr[unitIndex]);

                    if(ii >= numOfChunks)
                    {
                        currentChunksArrNumElements = 0;
                        break;
                    }
                }

                /* start the valid entries from the next entry */
                currentChunksPtr = &chunksMem[ii+1];
                /* get the new chunks unit base address */
                current23MSBitsAfter20LSBits = chunksMem[ii].memFirstAddr;

                currentChunksArrNumElements = 0;
            }
            else
            {
                currentChunksArrNumElements++;
            }
        }/* for ii*/

        /* we need to do also the last unit */
        if(currentChunksArrNumElements)
        {
            goto processEndOfUnit_lbl;
        }
    }

}

/*******************************************************************************
*   smemTmFindMem
*
* DESCRIPTION:
*       Return pointer to the register's or tables's memory. from the 'TM-device'
*
* INPUTS:
*       devObjPtr   - pointer to device object. (the hosting device - bobcat2)
*       accessType  - Read/Write operation
*       address     - address of memory(register or table).
*                       address is assumed to be the '20 LSBits' of the global address.
*                       the rest of the '23 MSBits' should be in internal memory of :
*                       CHT_INTERNAL_SIMULATION_USE_MEM_FOR_TRAFFIC_MANAGER_ADDRESS_23_MSBITS_E
*       memsize     - size of memory
*
* OUTPUTS:
*     activeMemPtrPtr - pointer to the active memory entry or NULL if not exist.
*
* RETURNS:
*        pointer to the memory location
*        NULL - if memory not exist
*
* COMMENTS:
*
*
*******************************************************************************/
static void * smemTmFindMem
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32                  address,
    IN GT_U32                  memSize
)
{
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = devObjPtr->deviceMemory;
    GT_U32      unitIndex;
    GT_U32      current23MSBitsAfter20LSBits; /*23 MSBits of 'full' address starting after first 20 LSBits*/
    GT_U32      current20LSBits;/*20 LSBits of 'full' address */
    GT_U32      *internalMemPtr;
    void*       memoryPtr;
    SMEM_UNIT_CHUNKS_STC    *currentUnitPtr;

    if(address > (0xFFFFF))
    {
        skernelFatalError("address > 20bits");
    }

    internalMemPtr = CHT_INTERNAL_MEM_PTR(devObjPtr,
        CHT_INTERNAL_SIMULATION_USE_MEM_FOR_TRAFFIC_MANAGER_ADDRESS_23_MSBITS_E);
    current23MSBitsAfter20LSBits = *internalMemPtr;

    if(current23MSBitsAfter20LSBits > MASK_23_BITS_CNS)
    {
        skernelFatalError("current23MSBitsAfter20LSBits must be up to 23 bits");
    }

    if(sRemoteTmUsed)
    {
        GT_U64  address64Bits,value64Bits;

        /* use the '20 LSBits' as offset within it */
        current20LSBits = address;
        /* build the 43 bits address */
        address64Bits.l[0] = ((current23MSBitsAfter20LSBits) << 20) | current20LSBits;
        address64Bits.l[1] = (current23MSBitsAfter20LSBits >> 12);

        if(0 == IS_WRITE_OPERATION_MAC(accessType))/*read*/
        {
            if((current20LSBits & 4) == 0)
            {
                /* assume that the read always start with the LSWord */
                sRemoteTmRegisterRead(devObjPtr->deviceId,&address64Bits,&value64Bits);
                /* read MSBits and save it */
                memoryPtr = CHT_INTERNAL_MEM_PTR(devObjPtr,
                    CHT_INTERNAL_SIMULATION_USE_MEM_FOR_REMOTE_TRAFFIC_MANAGER_DATA_32_MSBITS_E);
                *(GT_U32*)memoryPtr = value64Bits.l[1];
                /* read LSBits and save it */
                memoryPtr = CHT_INTERNAL_MEM_PTR(devObjPtr,
                    CHT_INTERNAL_SIMULATION_USE_MEM_FOR_REMOTE_TRAFFIC_MANAGER_DATA_32_LSBITS_E);
                *(GT_U32*)memoryPtr = value64Bits.l[0];
            }
            else
            {
                /* access to the MSWord .. we already got it .. assume that
                   continue the read of the LSWord ... */
                memoryPtr = CHT_INTERNAL_MEM_PTR(devObjPtr,
                    CHT_INTERNAL_SIMULATION_USE_MEM_FOR_REMOTE_TRAFFIC_MANAGER_DATA_32_MSBITS_E);
            }
        }
        else /*write*/
        {
            /* we need to do operation inside smemBobcat2ActiveWriteTmHostingUnit(...) */
            /* we get here only write to 'MSBits of addrsss' */
            memoryPtr = CHT_INTERNAL_MEM_PTR(devObjPtr,
                CHT_INTERNAL_SIMULATION_USE_MEM_FOR_REMOTE_TRAFFIC_MANAGER_DATA_32_MSBITS_E);
        }

        return memoryPtr;
    }

    /* get unit from the device (the hosting device - bobcat2) that hold the 'TM-device' */
    /* find this address in the array of tmDeviceSubUnitsAddrArr[] */
    unitIndex = tmDeviceFindUnitIndex(tmDeviceSubUnitsAddrArr,
            tmDeviceSubUnitsAddrArr_numOfEntries,
            current23MSBitsAfter20LSBits);

    currentUnitPtr = &devMemInfoPtr->extraUnitsPtr[unitIndex];
    /* after that we found the unit we need to use the '20 LSBits' as offset within it */
    current20LSBits = address;

    memoryPtr = smemDevFindInUnitChunk(devObjPtr, accessType,
                    current20LSBits, memSize,
                    (GT_UINTPTR)currentUnitPtr);
    if(memoryPtr == NULL)
    {
        return NULL;
    }

    return memoryPtr;
}

/* the TM hosting address that hold the 23 MSBits of the accessing address from the CPU */
#define TM_HOSTING_MSB_REG_ADDR_CNS     0xFFFF8

/* the TM hosting address that hold the 32 MSBits of the last 'read' from CPU */
#define TM_HOSTING_MSB_GLOBAL_READ_REG_ADDR_CNS     0xFFFF0

/*******************************************************************************
*   smemBobcat2FindInTmUnit
*
* DESCRIPTION:
*       find the memory in the TM unit chunk -- (TM hosting the 'TM-device')
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitChunksPtr - pointer to the unit chunk
*       address     - address of memory(register or table).
*       memSize     - size of the requested memory
*       param       - extra parameter --> used as pointer to the memory unit chunk
*
* OUTPUTS:
*       None.
*
* RETURNS:
*        pointer to the memory location
*        NULL - if memory not exist
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_U32 *  smemBobcat2FindInTmUnit
(
    IN SKERNEL_DEVICE_OBJECT *devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    GT_U32  address20LSBits;
    GT_BOOL isReadOperation = ((accessType & OPERATION_BIT) == 0) ? GT_TRUE : GT_FALSE;
    GT_BIT  saveReadMsb = 0;
    GT_U32 *returnPtr;
    GT_U32  *dataRead32MsbPtr;

    if(address & 0x00F00000)
    {
        skernelFatalError("bits 20..23 of address[0x%8.8x] must be 0",address);
    }

    /*get the 20 LSBits */
    address20LSBits = address & 0x000FFFFF;

    if(address20LSBits == TM_HOSTING_MSB_REG_ADDR_CNS)
    {
        /* return the internal memory register */
        return CHT_INTERNAL_MEM_PTR(devObjPtr,
            CHT_INTERNAL_SIMULATION_USE_MEM_FOR_TRAFFIC_MANAGER_ADDRESS_23_MSBITS_E);
    }
    else if (address20LSBits > TM_HOSTING_MSB_REG_ADDR_CNS)
    {
        static GT_U32   dummyReg;
        /* ignore accessing to this register (address20LSBits =  (TM_HOSTING_MSB_REG_ADDR_CNS + 4) */
        return &dummyReg;
    }
    else
    {
        if(isReadOperation == GT_TRUE)
        {
            if(address20LSBits == TM_HOSTING_MSB_GLOBAL_READ_REG_ADDR_CNS)
            {
                /* access to the MSWord .. we already got it .. when read the LSWord ... */
                return CHT_INTERNAL_MEM_PTR(devObjPtr,
                    CHT_INTERNAL_SIMULATION_USE_MEM_FOR_TRAFFIC_MANAGER_DATA_32_LSBITS_E);
            }
            else
            if((address20LSBits & 0x4) == 0)
            {
                /* read the current address and save value to the 'high 32 bits' register */
                saveReadMsb = 1;
            }
            else
            {
                /* in this read we will get the register value , but on
                   HW for 'clear on read' counters ... it will read value 0 as
                   it is cleared by read of the LS Word !

                   on simulation we not implemented (yet) any 'read counters' that triggers the 'clear on read'
                */
            }


            /* need to read from the 'TM-device' */

        }
        else
        {
            /*write operation */
            if ((address20LSBits & 0x4) == 0)
            {
                /* write the 'high 32 bits' register */
            }
            else
            {
                /* return internal memory register according to 'low' accessing */
                return CHT_INTERNAL_MEM_PTR(devObjPtr,
                                            CHT_INTERNAL_SIMULATION_USE_MEM_FOR_TRAFFIC_MANAGER_DATA_32_LSBITS_E);
            }
        }

    }

    returnPtr = smemTmFindMem(devObjPtr,accessType,address20LSBits,memSize);

    if(saveReadMsb && returnPtr)
    {
        dataRead32MsbPtr = CHT_INTERNAL_MEM_PTR(devObjPtr,
                    CHT_INTERNAL_SIMULATION_USE_MEM_FOR_REMOTE_TRAFFIC_MANAGER_DATA_32_MSBITS_E);
        /* save the info from the read */
        dataRead32MsbPtr[0] = returnPtr[1];
    }

    return returnPtr;

}

/*******************************************************************************
*  smemBobcat2ActiveWriteTmHostingUnit
*
* DESCRIPTION:
*       SIP5 TM hosting unit - the whole unit is '64 bits' burst on write.
*       need to write the data only after get full info
*       NOTE : only after write to address ending 0x4,0xC ... trigger the function !
* INPUTS:
*       devObjPtr   - Device object PTR.
*       address     - Address for ASIC memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers' specific parameter.
*       inMemPtr    - Pointer to the memory to get register's content.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void smemBobcat2ActiveWriteTmHostingUnit (
    IN  SKERNEL_DEVICE_OBJECT *devObjPtr,
    IN  GT_U32                 address,
    IN  GT_U32                 memSize,
    IN  GT_U32                *memPtr,
    IN  GT_UINTPTR             param,
    IN  GT_U32                *inMemPtr
)
{
    GT_U32  *data32LsbitsPtr;


    /*get the 20 LSBits */
    if((address & 0x000FFFFF) >= TM_HOSTING_MSB_REG_ADDR_CNS)
    {
        /* dummy save of value */
        *memPtr = *inMemPtr;
        return;
    }

    /* we got pointer to the 'LSWord' , write to it the 'Saved value' */
    data32LsbitsPtr = CHT_INTERNAL_MEM_PTR(devObjPtr,
        CHT_INTERNAL_SIMULATION_USE_MEM_FOR_TRAFFIC_MANAGER_DATA_32_LSBITS_E);

    if(sRemoteTmUsed)
    {
        GT_U64  address64Bits,value64Bits;
        GT_U32      current23MSBitsAfter20LSBits; /*23 MSBits of 'full' address starting after first 20 LSBits*/
        GT_U32      current20LSBits;/*20 LSBits of 'full' address */

        current23MSBitsAfter20LSBits = *(CHT_INTERNAL_MEM_PTR(devObjPtr,
            CHT_INTERNAL_SIMULATION_USE_MEM_FOR_TRAFFIC_MANAGER_ADDRESS_23_MSBITS_E));

        /* use the '20 LSBits' as offset within it */
        current20LSBits = address & (~0x4);
        /* build the 43 bits address */
        address64Bits.l[0] = ((current23MSBitsAfter20LSBits) << 20) | current20LSBits;
        address64Bits.l[1] = (current23MSBitsAfter20LSBits >> 12);

        value64Bits.l[0] = *data32LsbitsPtr;
        value64Bits.l[1] = *inMemPtr;
        sRemoteTmRegisterWrite(devObjPtr->deviceId,&address64Bits,&value64Bits);

        return;
    }

    memPtr[0] = *data32LsbitsPtr;

    /* now write the 'MSWord' , according to given value */
    memPtr[1] = *inMemPtr;

    return;
}

/*******************************************************************************
*   smemBobcat2UnitTmDrop
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Bobcat2  TM drop unit
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr     - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemBobcat2UnitTmDrop
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000000, 0x00000028)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000030, 0x0000003C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00001000, 512),
                SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(12 , 4),SMEM_BIND_TABLE_MAC(tmDropDropMasking)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00010000, 8192),
                SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(48 , 8),SMEM_BIND_TABLE_MAC(tmDropQueueProfileId)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
}

/*******************************************************************************
*   smemBobcat2UnitEthTxFifo
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Bobcat2 Eth TX Fifo unit
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr     - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
void smemBobcat2UnitEthTxFifo
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000000, 0x00000128)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000400, 0x00000434)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000440, 0x00000444)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000508, 0x00000508)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000518, 0x00000518)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000528, 0x00000528)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000538, 0x00000538)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000548, 0x00000548)}
/* [JIRA] (TMIP-69) Non aligned address (in bc2) 0x1A000552 for regsiter <SIP_ETH_TXFIFO_IP Units/TxFIFO End 2 End ECC Configurations/Lock Erronous Event>
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000552, 0x00000552)}*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000600, 0x00000720)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000800, 0x00000958)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000960, 0x00000960)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00001000, 0x00001018)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00001100, 0x00001220)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00001300, 0x00001420)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00001500, 0x00001620)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00001700, 0x00001820)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00001900, 0x00001A20)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00001B00, 0x00001C20)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00003000, 0x00003004)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00003100, 0x00003220)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00003300, 0x00003300)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00010000, 32136)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00020000, 32)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00022000, 0x00022000)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00030000, 292)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00030200, 292)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00030400, 292)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00030600, 292)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00030800, 292)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00030A00, 292)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00040000, 292)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00040200, 292)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00040400, 292)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00040600, 292)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00040800, 292)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00040A00, 292)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00052000, 4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00054000, 0x00054000)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00055000, 292)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00055200, 292)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00055400, 292)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00055600, 292)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00055800, 292)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00055A00, 292)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00057000, 292)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00057200, 292)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00057400, 292)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00057600, 292)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00057800, 292)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00057A00, 292)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00090000, 32136)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }

}

/*******************************************************************************
*   smemBobcat2UnitIlknTxFifo
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Bobcat2 Eth TX Fifo unit
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr     - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemBobcat2UnitIlknTxFifo
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000000, 0x00000000)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000020, 0x00000020)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000030, 0x00000030)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000040, 0x00000084)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000000A0, 0x000000A4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000200, 0x000002FC)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000350, 0x00000350)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000360, 0x0000045C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000500, 0x000005FC)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001000, 0x00001000)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001010, 0x00001110)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001500, 0x00001504)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001510, 0x000025AC)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00006500, 0x00006504)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00006510, 0x00006510)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00006520, 0x00006520)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00006600, 0x00006600)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00006800, 0x00006800)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00006A00, 0x00006A00)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00006C00, 0x00006C00)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00006D00, 0x00006EFC)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00007500, 0x00007504)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00007520, 0x00007524)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00007540, 0x00007544)}

        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }

}


/*******************************************************************************
*   smemBobcat2UnitTmFcu
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Bobcat2  TM FCU unit
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr     - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemBobcat2UnitTmFcu
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000000, 0x00000008)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000010, 0x00000014)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000200, 0x000004FC)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000500, 128)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000700, 128)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00001000, 1024)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00003000, 256)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00008000, 2048)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
    {
        SMEM_CHUNK_BASIC_STC  chunksMemBc2B0[]=
        {
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000100, 0x000001FC)}
        };


        if(SMEM_CHT_IS_SIP5_10_GET(devObjPtr))
        {
            smemUnitChunkAddBasicChunk(devObjPtr,unitPtr,
                ARRAY_NAME_AND_NUM_ELEMENTS_MAC(chunksMemBc2B0));
        }
    }

}

/*******************************************************************************
*   smemBobcat2UnitTmIngressGlue
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Bobcat2  TM Ingress glue unit
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr     - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemBobcat2UnitTmIngressGlue
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000000, 0x00000010)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00001000, 256)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
}

/*******************************************************************************
*   smemBobcat2UnitTmEgressGlue
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Bobcat2  TM Egress glue unit
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr     - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemBobcat2UnitTmEgressGlue
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000000, 0x00000010)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000024, 0x00000054)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000060, 0x00000064)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000070, 0x00000078)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000100, 0x0000015C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000200, 0x0000038C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000400, 0x0000058C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000600, 0x0000078C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000800, 0x0000098C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000C00, 0x00000D8C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00001000, 0x0000118C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00001200, 0x0000138C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00001400, 0x0000158C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00001600, 0x0000178C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00001A00, 0x00001CFC)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00002000, 0x00002018)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00002020, 0x00002038)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00002040, 0x00002058)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00002100, 0x000023FC)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00004000, 8192)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00006000, 128)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00006200, 256),
                SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32 , 4),SMEM_BIND_TABLE_MAC(tmEgressGlueTargetInterface)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
}

/*******************************************************************************
*   smemBobcat2UnitTmQMap
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Bobcat2  TM Qmap unit
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr     - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemBobcat2UnitTmQMap
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000000, 0x00000018)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000020, 0x00000028)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00001000, 1024)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00010000, 4096)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00020000, 4096)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00030000, 4096)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
}

/*******************************************************************************
*   smemBobcat2UnitTmInit
*
* DESCRIPTION:
*       Init TM unit in the device -- for the bobcat2  TM hosting unit
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemBobcat2UnitTmInit
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_CHT_GENERIC_DEV_MEM_INFO  * devMemInfoPtr = devObjPtr->deviceMemory;
    GT_U32  unitIndex;

    /* the unit should hold place only for 3 addresses :
        address 0xFFFF8 - that hold the 23 MSBits of address
        2 address for any access to consecutive registers {0,4}..{0xFFFF0,0xFFFF4}

        BUT for that we will use :
            CHT_INTERNAL_SIMULATION_USE_MEM_FOR_TRAFFIC_MANAGER_ADDRESS_23_MSBITS_E,
            CHT_INTERNAL_SIMULATION_USE_MEM_FOR_TRAFFIC_MANAGER_DATA_32_LSBITS_E,

        see use in function smemBobcat2FindInTmUnit
     */

    /* get the unitIndex from the name of the unit */
    unitIndex = UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TM_HOSTING);

    /* bind the TM hosting unit to dedicated find function */
    devMemInfoPtr->common.specFunTbl[unitIndex].specFun    = smemBobcat2FindInTmUnit;


}

/*******************************************************************************
*   bindActiveMemoriesOnUnits
*
* DESCRIPTION:
*       bind active memories on units
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void bindActiveMemoriesOnUnits
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_CHT_GENERIC_DEV_MEM_INFO  * devMemInfoPtr = devObjPtr->deviceMemory;
    SMEM_UNIT_CHUNKS_STC    *currUnitChunkPtr;

    /* bind of PEX , DFX already done in smemLion3BindNonPpPerUnitActiveMem(...) */

    /* bind active memories of units in PP memory space */
    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_MG)];
    if(IS_VALID_AND_EMPTY_ACTIVE_MEM_ARR_FOR_CHUNK_MAC(currUnitChunkPtr))
    {
        if(SMEM_CHT_IS_SIP5_10_GET(devObjPtr))
        {
            bindUnitMgActiveMem_sip5_10(devObjPtr,currUnitChunkPtr);
        }
        else
        {
            bindUnitMgActiveMem(devObjPtr,currUnitChunkPtr);
        }
    }

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_BM)];
    if(IS_VALID_AND_EMPTY_ACTIVE_MEM_ARR_FOR_CHUNK_MAC(currUnitChunkPtr))
    {
        bindUnitBmActiveMem(devObjPtr,currUnitChunkPtr);
    }

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_IPCL)];
    if(IS_VALID_AND_EMPTY_ACTIVE_MEM_ARR_FOR_CHUNK_MAC(currUnitChunkPtr))
    {
        bindUnitPclActiveMem(devObjPtr, currUnitChunkPtr);
    }

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_GOP)];
    if(IS_VALID_AND_EMPTY_ACTIVE_MEM_ARR_FOR_CHUNK_MAC(currUnitChunkPtr))
    {
        bindUnitGopActiveMem(devObjPtr,currUnitChunkPtr);
    }

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_L2I)];
    if(IS_VALID_AND_EMPTY_ACTIVE_MEM_ARR_FOR_CHUNK_MAC(currUnitChunkPtr))
    {
        bindUnitL2iActiveMem(devObjPtr,currUnitChunkPtr);
    }

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_FDB)];
    if(IS_VALID_AND_EMPTY_ACTIVE_MEM_ARR_FOR_CHUNK_MAC(currUnitChunkPtr))
    {
        bindUnitFdbActiveMem(devObjPtr,currUnitChunkPtr);
    }

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_XG_PORT_MIB)];
    if(IS_VALID_AND_EMPTY_ACTIVE_MEM_ARR_FOR_CHUNK_MAC(currUnitChunkPtr))
    {
        bindUnitMibActiveMem(devObjPtr,currUnitChunkPtr);
    }

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_SERDES)];
    if(IS_VALID_AND_EMPTY_ACTIVE_MEM_ARR_FOR_CHUNK_MAC(currUnitChunkPtr))
    {
        bindUnitSerdesActiveMem(devObjPtr,currUnitChunkPtr);
    }

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TXQ_QUEUE)];
    if(IS_VALID_AND_EMPTY_ACTIVE_MEM_ARR_FOR_CHUNK_MAC(currUnitChunkPtr))
    {
        bindUnitTxqQueueActiveMem(devObjPtr,currUnitChunkPtr);
    }

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TXQ_QCN)];
    if(IS_VALID_AND_EMPTY_ACTIVE_MEM_ARR_FOR_CHUNK_MAC(currUnitChunkPtr))
    {
        bindUnitTxqQcnActiveMem(devObjPtr,currUnitChunkPtr);
    }

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_CPFC)];
    if(IS_VALID_AND_EMPTY_ACTIVE_MEM_ARR_FOR_CHUNK_MAC(currUnitChunkPtr))
    {
        bindUnitCpfcActiveMem(devObjPtr,currUnitChunkPtr);
    }

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TX_DMA)];
    if(IS_VALID_AND_EMPTY_ACTIVE_MEM_ARR_FOR_CHUNK_MAC(currUnitChunkPtr))
    {
        bindUnitTxDmaActiveMem(devObjPtr,currUnitChunkPtr);
    }

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TX_FIFO)];
    if(IS_VALID_AND_EMPTY_ACTIVE_MEM_ARR_FOR_CHUNK_MAC(currUnitChunkPtr))
    {
        bindUnitTxFifoActiveMem(devObjPtr,currUnitChunkPtr);
    }

    if(devObjPtr->supportTrafficManager)
    {
        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_ETH_TX_FIFO)];
        if(IS_VALID_AND_EMPTY_ACTIVE_MEM_ARR_FOR_CHUNK_MAC(currUnitChunkPtr))
        {
            bindUnitEthTxFifoActiveMem(devObjPtr,currUnitChunkPtr);
        }
    }

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_MPPM)];
    if(IS_VALID_AND_EMPTY_ACTIVE_MEM_ARR_FOR_CHUNK_MAC(currUnitChunkPtr))
    {
        bindUnitMppmActiveMem(devObjPtr,currUnitChunkPtr);
    }

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_IPLR)];
    if(IS_VALID_AND_EMPTY_ACTIVE_MEM_ARR_FOR_CHUNK_MAC(currUnitChunkPtr))
    {
        bindUnitIplr0ActiveMem(devObjPtr,currUnitChunkPtr);
    }

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_IPLR1)];
    if(IS_VALID_AND_EMPTY_ACTIVE_MEM_ARR_FOR_CHUNK_MAC(currUnitChunkPtr))
    {
        bindUnitIplr1ActiveMem(devObjPtr,currUnitChunkPtr);
    }

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_EPLR)];
    if(IS_VALID_AND_EMPTY_ACTIVE_MEM_ARR_FOR_CHUNK_MAC(currUnitChunkPtr))
    {
        bindUnitEplrActiveMem(devObjPtr,currUnitChunkPtr);
    }

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_EQ)];
    if(IS_VALID_AND_EMPTY_ACTIVE_MEM_ARR_FOR_CHUNK_MAC(currUnitChunkPtr))
    {
        bindUnitEqActiveMem(devObjPtr,currUnitChunkPtr);
    }

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_CNC)];
    if(IS_VALID_AND_EMPTY_ACTIVE_MEM_ARR_FOR_CHUNK_MAC(currUnitChunkPtr))
    {
        bindUnitCnc0ActiveMem(devObjPtr,currUnitChunkPtr);
    }

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_CNC_1)];
    if(IS_VALID_AND_EMPTY_ACTIVE_MEM_ARR_FOR_CHUNK_MAC(currUnitChunkPtr))
    {
        bindUnitCnc1ActiveMem(devObjPtr,currUnitChunkPtr);
    }


    if(SMAIN_NOT_VALID_CNS != UNIT_BASE_ADDR_GET_ALLOW_NON_EXIST_UNIT_MAC(devObjPtr,UNIT_LMS))
    {
        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_LMS)];
        if(IS_VALID_AND_EMPTY_ACTIVE_MEM_ARR_FOR_CHUNK_MAC(currUnitChunkPtr))
        {
            bindUnitLms0ActiveMem(devObjPtr,currUnitChunkPtr);
        }
    }

    if(SMAIN_NOT_VALID_CNS != UNIT_BASE_ADDR_GET_ALLOW_NON_EXIST_UNIT_MAC(devObjPtr,UNIT_LMS1))
    {
        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_LMS1)];
        if(IS_VALID_AND_EMPTY_ACTIVE_MEM_ARR_FOR_CHUNK_MAC(currUnitChunkPtr))
        {
            bindUnitLms1ActiveMem(devObjPtr,currUnitChunkPtr);
        }
    }

    if(SMAIN_NOT_VALID_CNS != UNIT_BASE_ADDR_GET_ALLOW_NON_EXIST_UNIT_MAC(devObjPtr,UNIT_LMS2))
    {
        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_LMS2)];
        if(IS_VALID_AND_EMPTY_ACTIVE_MEM_ARR_FOR_CHUNK_MAC(currUnitChunkPtr))
        {
            bindUnitLms2ActiveMem(devObjPtr,currUnitChunkPtr);
        }
    }

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TXQ_DQ)];
    if(IS_VALID_AND_EMPTY_ACTIVE_MEM_ARR_FOR_CHUNK_MAC(currUnitChunkPtr))
    {
        bindUnitTxqDqActiveMem(devObjPtr,currUnitChunkPtr);
    }

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_MLL)];
    if(IS_VALID_AND_EMPTY_ACTIVE_MEM_ARR_FOR_CHUNK_MAC(currUnitChunkPtr))
    {
        bindUnitMllActiveMem(devObjPtr,currUnitChunkPtr);
    }

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_IOAM)];
    if(IS_VALID_AND_EMPTY_ACTIVE_MEM_ARR_FOR_CHUNK_MAC(currUnitChunkPtr))
    {
        bindUnitIOamActiveMem(devObjPtr,currUnitChunkPtr);
    }

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_EOAM)];
    if(IS_VALID_AND_EMPTY_ACTIVE_MEM_ARR_FOR_CHUNK_MAC(currUnitChunkPtr))
    {
        bindUnitEOamActiveMem(devObjPtr,currUnitChunkPtr);
    }

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TCAM)];
    if(IS_VALID_AND_EMPTY_ACTIVE_MEM_ARR_FOR_CHUNK_MAC(currUnitChunkPtr))
    {
        bindUnitTcamActiveMem(devObjPtr,currUnitChunkPtr);
    }


    if(devObjPtr->supportTrafficManager)
    {
        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TM_HOSTING)];
        if(IS_VALID_AND_EMPTY_ACTIVE_MEM_ARR_FOR_CHUNK_MAC(currUnitChunkPtr))
        {
            bindUnitTmActiveMem(devObjPtr,currUnitChunkPtr);
        }
    }


    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TTI)];
    if(IS_VALID_AND_EMPTY_ACTIVE_MEM_ARR_FOR_CHUNK_MAC(currUnitChunkPtr))
    {
        bindUnitTtiActiveMem(devObjPtr,currUnitChunkPtr);
    }

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_HA)];
    if(IS_VALID_AND_EMPTY_ACTIVE_MEM_ARR_FOR_CHUNK_MAC(currUnitChunkPtr))
    {
        bindUnitHaActiveMem(devObjPtr,currUnitChunkPtr);
    }

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_ERMRK)];
    if(IS_VALID_AND_EMPTY_ACTIVE_MEM_ARR_FOR_CHUNK_MAC(currUnitChunkPtr))
    {
        bindUnitErmrkActiveMem(devObjPtr,currUnitChunkPtr);
    }

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_EGF_EFT)];
    if(IS_VALID_AND_EMPTY_ACTIVE_MEM_ARR_FOR_CHUNK_MAC(currUnitChunkPtr))
    {
        bindUnitEgfEft0ActiveMem(devObjPtr,currUnitChunkPtr);
    }

    if(SMEM_CHT_IS_SIP5_10_GET(devObjPtr))
    {
        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_EPCL)];
        if(IS_VALID_AND_EMPTY_ACTIVE_MEM_ARR_FOR_CHUNK_MAC(currUnitChunkPtr))
        {
            bindUnitEpclActiveMem_sip5_10(devObjPtr,currUnitChunkPtr);
        }
    }

    if(SMAIN_NOT_VALID_CNS != UNIT_BASE_ADDR_GET_ALLOW_NON_EXIST_UNIT_MAC(devObjPtr,UNIT_GOP_SMI_0))
    {
        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_GOP_SMI_0)];
        if(IS_VALID_AND_EMPTY_ACTIVE_MEM_ARR_FOR_CHUNK_MAC(currUnitChunkPtr))
        {
            bindUnitSmi0ActiveMem(devObjPtr,currUnitChunkPtr);
        }
    }

    if(SMAIN_NOT_VALID_CNS != UNIT_BASE_ADDR_GET_ALLOW_NON_EXIST_UNIT_MAC(devObjPtr,UNIT_GOP_SMI_1))
    {
        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_GOP_SMI_1)];
        if(IS_VALID_AND_EMPTY_ACTIVE_MEM_ARR_FOR_CHUNK_MAC(currUnitChunkPtr))
        {
            bindUnitSmi1ActiveMem(devObjPtr,currUnitChunkPtr);
        }
    }

    if(SMAIN_NOT_VALID_CNS != UNIT_BASE_ADDR_GET_ALLOW_NON_EXIST_UNIT_MAC(devObjPtr,UNIT_GOP_SMI_2))
    {
        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_GOP_SMI_2)];
        if(IS_VALID_AND_EMPTY_ACTIVE_MEM_ARR_FOR_CHUNK_MAC(currUnitChunkPtr))
        {
            bindUnitSmi2ActiveMem(devObjPtr,currUnitChunkPtr);
        }
    }

    if(SMAIN_NOT_VALID_CNS != UNIT_BASE_ADDR_GET_ALLOW_NON_EXIST_UNIT_MAC(devObjPtr,UNIT_GOP_SMI_3))
    {
        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_GOP_SMI_3)];
        if(IS_VALID_AND_EMPTY_ACTIVE_MEM_ARR_FOR_CHUNK_MAC(currUnitChunkPtr))
        {
            bindUnitSmi3ActiveMem(devObjPtr,currUnitChunkPtr);
        }
    }

    if(SMAIN_NOT_VALID_CNS != UNIT_BASE_ADDR_GET_ALLOW_NON_EXIST_UNIT_MAC(devObjPtr,UNIT_RX_DMA))
    {
        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_RX_DMA)];
        if(IS_VALID_AND_EMPTY_ACTIVE_MEM_ARR_FOR_CHUNK_MAC(currUnitChunkPtr))
        {
            if(SMEM_CHT_IS_SIP5_10_GET(devObjPtr))
            {
                bindUnitRxDmaActiveMem(devObjPtr,currUnitChunkPtr);
            }
        }
    }

    if(SMAIN_NOT_VALID_CNS != UNIT_BASE_ADDR_GET_ALLOW_NON_EXIST_UNIT_MAC(devObjPtr,UNIT_RX_DMA_1))
    {
        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_RX_DMA_1)];
        if(IS_VALID_AND_EMPTY_ACTIVE_MEM_ARR_FOR_CHUNK_MAC(currUnitChunkPtr))
        {
            if(SMEM_CHT_IS_SIP5_10_GET(devObjPtr))
            {
                bindUnitRxDma1ActiveMem(devObjPtr,currUnitChunkPtr);
            }
        }
    }

}

/*******************************************************************************
*   smemBobcat2SpecificDeviceMemInitPart1
*
* DESCRIPTION:
*       specific part 1 of initialization that called from init 1 of
*       Lion3
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemBobcat2SpecificDeviceMemInitPart1
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_CHT_GENERIC_DEV_MEM_INFO  * devMemInfoPtr = devObjPtr->deviceMemory;
    SMEM_UNIT_CHUNKS_STC    *currUnitChunkPtr;
    GT_U32  unitIndex;
    SMEM_SIP5_PP_REGS_ADDR_STC      *regAddrDbSip5Ptr;

    regAddrDbSip5Ptr = SMEM_CHT_MAC_REG_DB_SIP5_GET(devObjPtr);

    smemBobcat2ErrataCleanUp(devObjPtr);

    /* those functions need to be called before that Lion3 code is calling
        smemBindTablesToMemories(...) */

    /* init also other TM glue units of TM */
    if(devObjPtr->supportTrafficManager)
    {
        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TMDROP)];
        smemBobcat2UnitTmDrop(devObjPtr,currUnitChunkPtr);
        smemGenericRegistersArrayAlignToUnit1(
            REG_PTR_AND_SIZE_MAC(regAddrDbSip5Ptr->TMDROP),
            currUnitChunkPtr,
            GT_TRUE);/* addresses are '0' based and need += of the 'base unit address' */

        unitIndex = UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_ETH_TX_FIFO);
        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[unitIndex];
        smemBobcat2UnitEthTxFifo(devObjPtr,currUnitChunkPtr);

        if(SMAIN_NOT_VALID_CNS != UNIT_BASE_ADDR_GET_ALLOW_NON_EXIST_UNIT_MAC(devObjPtr,UNIT_ILKN_TX_FIFO))
        {
            unitIndex = UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_ILKN_TX_FIFO);
            currUnitChunkPtr = &devMemInfoPtr->unitMemArr[unitIndex];
            smemBobcat2UnitIlknTxFifo(devObjPtr,currUnitChunkPtr);
        }

        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TM_FCU)];
        smemBobcat2UnitTmFcu(devObjPtr,currUnitChunkPtr);

        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TM_INGRESS_GLUE)];
        smemBobcat2UnitTmIngressGlue(devObjPtr,currUnitChunkPtr);
        smemGenericRegistersArrayAlignToUnit1(
            REG_PTR_AND_SIZE_MAC(regAddrDbSip5Ptr->TM_INGR_GLUE),
            currUnitChunkPtr,
            GT_TRUE);/* addresses are '0' based and need += of the 'base unit address' */

        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TM_EGRESS_GLUE)];
        smemBobcat2UnitTmEgressGlue(devObjPtr,currUnitChunkPtr);
        smemGenericRegistersArrayAlignToUnit1(
            REG_PTR_AND_SIZE_MAC(regAddrDbSip5Ptr->TM_EGR_GLUE),
            currUnitChunkPtr,
            GT_TRUE);/* addresses are '0' based and need += of the 'base unit address' */

        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TMQMAP)];
        smemBobcat2UnitTmQMap(devObjPtr,currUnitChunkPtr);
        smemGenericRegistersArrayAlignToUnit1(
            REG_PTR_AND_SIZE_MAC(regAddrDbSip5Ptr->TMQMAP),
            currUnitChunkPtr,
            GT_TRUE);/* addresses are '0' based and need += of the 'base unit address' */
    }

    if(SMAIN_NOT_VALID_CNS != UNIT_BASE_ADDR_GET_ALLOW_NON_EXIST_UNIT_MAC(devObjPtr,UNIT_GOP_ILKN))
    {
        /*UNIT_GOP_ILKN*/
        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_GOP_ILKN)];
        smemBobcat2UnitGopIlkn(devObjPtr,currUnitChunkPtr);
    }

    if(SMAIN_NOT_VALID_CNS != UNIT_BASE_ADDR_GET_ALLOW_NON_EXIST_UNIT_MAC(devObjPtr,UNIT_GOP_LED_0))
    {
        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_GOP_LED_0)];
        if(currUnitChunkPtr->numOfChunks == 0)
        {
            smemBobcat2UnitGopLed(devObjPtr,currUnitChunkPtr);
        }
    }

    if(SMAIN_NOT_VALID_CNS != UNIT_BASE_ADDR_GET_ALLOW_NON_EXIST_UNIT_MAC(devObjPtr,UNIT_GOP_LED_1))
    {
        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_GOP_LED_1)];
        if(currUnitChunkPtr->numOfChunks == 0)
        {
            smemBobcat2UnitGopLed(devObjPtr,currUnitChunkPtr);
        }
    }

    if(SMAIN_NOT_VALID_CNS != UNIT_BASE_ADDR_GET_ALLOW_NON_EXIST_UNIT_MAC(devObjPtr,UNIT_GOP_LED_2))
    {
        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_GOP_LED_2)];
        if(currUnitChunkPtr->numOfChunks == 0)
        {
            smemBobcat2UnitGopLed(devObjPtr,currUnitChunkPtr);
        }
    }

    if(SMAIN_NOT_VALID_CNS != UNIT_BASE_ADDR_GET_ALLOW_NON_EXIST_UNIT_MAC(devObjPtr,UNIT_GOP_LED_3))
    {
        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_GOP_LED_3)];
        if(currUnitChunkPtr->numOfChunks == 0)
        {
            smemBobcat2UnitGopLed(devObjPtr,currUnitChunkPtr);
        }
    }

    if(SMAIN_NOT_VALID_CNS != UNIT_BASE_ADDR_GET_ALLOW_NON_EXIST_UNIT_MAC(devObjPtr,UNIT_GOP_LED_4))
    {
        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_GOP_LED_4)];
        if(currUnitChunkPtr->numOfChunks == 0)
        {
            smemBobcat2UnitGopLed(devObjPtr,currUnitChunkPtr);
        }
    }

    if(SMAIN_NOT_VALID_CNS != UNIT_BASE_ADDR_GET_ALLOW_NON_EXIST_UNIT_MAC(devObjPtr,UNIT_GOP_SMI_0))
    {
        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_GOP_SMI_0)];
        if(currUnitChunkPtr->numOfChunks == 0)
        {
            smemBobcat2UnitGopSmi(devObjPtr,currUnitChunkPtr);
        }
    }

    if(SMAIN_NOT_VALID_CNS != UNIT_BASE_ADDR_GET_ALLOW_NON_EXIST_UNIT_MAC(devObjPtr,UNIT_GOP_SMI_1))
    {
        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_GOP_SMI_1)];
        if(currUnitChunkPtr->numOfChunks == 0)
        {
            smemBobcat2UnitGopSmi(devObjPtr,currUnitChunkPtr);
        }
    }

    if(SMAIN_NOT_VALID_CNS != UNIT_BASE_ADDR_GET_ALLOW_NON_EXIST_UNIT_MAC(devObjPtr,UNIT_GOP_SMI_2))
    {
        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_GOP_SMI_2)];
        if(currUnitChunkPtr->numOfChunks == 0)
        {
            smemBobcat2UnitGopSmi(devObjPtr,currUnitChunkPtr);
        }
    }

    if(SMAIN_NOT_VALID_CNS != UNIT_BASE_ADDR_GET_ALLOW_NON_EXIST_UNIT_MAC(devObjPtr,UNIT_GOP_SMI_3))
    {
        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_GOP_SMI_3)];
        if(currUnitChunkPtr->numOfChunks == 0)
        {
            smemBobcat2UnitGopSmi(devObjPtr,currUnitChunkPtr);
        }
    }

    if(SMEM_CHT_IS_SIP5_15_GET(devObjPtr) == 1)
    {
        /* iplr tables */
        {
            GT_U32  iplr0ToIplr1Offset = devObjPtr->memUnitBaseAddrInfo.policer[1] -
                                 devObjPtr->memUnitBaseAddrInfo.policer[0];

            SET_IF_ZERO_MAC(devObjPtr->tablesInfo.policerConfig.paramInfo[1].step , iplr0ToIplr1Offset);/*cycle*/
            SET_IF_ZERO_MAC(devObjPtr->tablesInfo.policerConformanceLevelSign.paramInfo[1].step , iplr0ToIplr1Offset);/*cycle*/

        }
    }

}

/*******************************************************************************
*   smemBobcat2SpecificDeviceMemInitPart2
*
* DESCRIPTION:
*       specific part 2 of initialization that called from init 1 of
*       Lion3
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
void smemBobcat2SpecificDeviceMemInitPart2
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    GT_U32  ii,iiMax=64;
    GT_U32  value1,value2,value3,value4;
    GT_U32  *regPtr;
    GT_U32  fcuBaseAddr;
    GT_U32  tc, cNode = 0, offset = 0;

    if(devObjPtr->supportTrafficManager)
    {
        smemTmDeviceInit(devObjPtr);
        smemBobcat2UnitTmInit(devObjPtr);
    }

    /* bind active memories on units */
    bindActiveMemoriesOnUnits(devObjPtr);

    if(devObjPtr->supportTrafficManager)
    {
       fcuBaseAddr = UNIT_BASE_ADDR_GET_MAC(devObjPtr, UNIT_TM_FCU);

       /* PRV_CPSS_DXCH_LION3_TABLE_TM_FCU_TC_PORT_TO_CNODE_PORT_MAPPING_E init*/
       if(SMEM_CHT_IS_SIP5_10_GET(devObjPtr))
       {
           regPtr = smemMemGet(devObjPtr, fcuBaseAddr + 0x100);

           for(ii = 0 ; ii < iiMax ; ii++)
           {
               value1 = ((ii*4) + 0) & 0x3f;
               value2 = value1 + 1;
               value3 = value1 + 2;
               value4 = value1 + 3;

               regPtr[ii] = (value1 <<  0) | (value2 <<  8) |
                            (value3 << 16) | (value4 << 24);
           }
       }

       /* PRV_CPSS_DXCH_LION3_TABLE_TM_FCU_TC_PORT_TO_CNODE_PORT_MAPPING_E init */
       regPtr = smemMemGet(devObjPtr, fcuBaseAddr + 0x1000);

       for(ii = 0 ; ii < iiMax ; ii++)
       {
           for (tc = 0; tc < 8; tc++, cNode++)
           {
               snetFieldValueSet(&regPtr[offset],
                                 tc * 9,
                                 9,
                                 cNode);
           }

           offset += 4; /* aligned 128 bits (LION3_TM_FCU_TC_PORT_TO_CNODE_PORT_MAPPING_SIZE_CNS) / sizeof(GT_U32) */
       }
    }
}



#define TEST_CONVERT_OFFSET_IN_TM_UNIT_MAC(offset) ((offset) + baseUnitAddr)
void smemBobcat2TmUnitBasicTest(
    IN GT_U32  deviceId  /*device Id as given to SCIB layer */
)
{
    SKERNEL_DEVICE_OBJECT *devObjPtr = smemTestDeviceIdToDevPtrConvert(deviceId);
    GT_U32 baseUnitAddr = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_TM_HOSTING);
    GT_U32 tm23MsbAddr = TM_HOSTING_MSB_REG_ADDR_CNS;
    GT_U32 length = 1;
    GT_U32 tmUnit[4];/*unit in the TM-device (the 23 MSBits)*/
    GT_U32 tm20LsbAddr[4];/*offset in unit in the TM-device (the 20 LSBits)*/
    GT_U32 ii;/*index in tmUnit[],tm20LsbAddr[]*/
    GT_U32 dataArr[2];/*data to read/write to from the registers*/
    GT_U32 iiMax;
    GT_U32 dummyValue = 0;


    /**********************************/
    /* first do all the needed writes */
    /**********************************/

    ii = 0;
    tmUnit[ii]      = 0x800A0;
    tm20LsbAddr[ii] = 0x00440;

    dataArr[0]  = tm20LsbAddr[ii];
    dataArr[1]  = tmUnit[ii];

    /*state new unit to the TM-device*/
    scibWriteMemory (deviceId , TEST_CONVERT_OFFSET_IN_TM_UNIT_MAC(tm23MsbAddr) , length ,& tmUnit[ii]);
    scibWriteMemory (deviceId , TEST_CONVERT_OFFSET_IN_TM_UNIT_MAC(tm23MsbAddr + 4) , length ,& dummyValue);

    /* write data 32 LSBits */
    scibWriteMemory (deviceId , TEST_CONVERT_OFFSET_IN_TM_UNIT_MAC(tm20LsbAddr[ii]) , length ,& dataArr[0]);
    /* write data 32 MSBits */
    scibWriteMemory (deviceId , TEST_CONVERT_OFFSET_IN_TM_UNIT_MAC(tm20LsbAddr[ii] + 4) , length ,& dataArr[1]);

    /* other address in the same unit */
    ii++;

    tmUnit[ii] = tmUnit[ii - 1];
    tm20LsbAddr[ii] = 0x00668;

    dataArr[0]  = tm20LsbAddr[ii];
    dataArr[1]  = tmUnit[ii];

    /* write data 32 LSBits */
    scibWriteMemory (deviceId , TEST_CONVERT_OFFSET_IN_TM_UNIT_MAC(tm20LsbAddr[ii]) , length ,& dataArr[0]);
    /* write data 32 MSBits */
    scibWriteMemory (deviceId , TEST_CONVERT_OFFSET_IN_TM_UNIT_MAC(tm20LsbAddr[ii] + 4) , length ,& dataArr[1]);


    /*state new unit to the TM-device*/
    ii++;
    tmUnit[ii]      = 0x31000;
    tm20LsbAddr[ii] = 0x000A8;

    dataArr[0]  = tm20LsbAddr[ii];
    dataArr[1]  = tmUnit[ii];

    scibWriteMemory (deviceId , TEST_CONVERT_OFFSET_IN_TM_UNIT_MAC(tm23MsbAddr) , length ,& tmUnit[ii]);
    scibWriteMemory (deviceId , TEST_CONVERT_OFFSET_IN_TM_UNIT_MAC(tm23MsbAddr + 4) , length ,& dummyValue);

    /* write data 32 LSBits */
    scibWriteMemory (deviceId , TEST_CONVERT_OFFSET_IN_TM_UNIT_MAC(tm20LsbAddr[ii]) , length ,& dataArr[0]);
    /* write data 32 MSBits */
    scibWriteMemory (deviceId , TEST_CONVERT_OFFSET_IN_TM_UNIT_MAC(tm20LsbAddr[ii] + 4) , length ,& dataArr[1]);


    /* other address in the same unit */
    ii++;
    tmUnit[ii] = tmUnit[ii - 1];
    tm20LsbAddr[ii] = 0x00008;

    dataArr[0]  = tm20LsbAddr[ii];
    dataArr[1]  = tmUnit[ii];

    /* write data 32 LSBits */
    scibWriteMemory (deviceId , TEST_CONVERT_OFFSET_IN_TM_UNIT_MAC(tm20LsbAddr[ii]) , length ,& dataArr[0]);
    /* write data 32 MSBits */
    scibWriteMemory (deviceId , TEST_CONVERT_OFFSET_IN_TM_UNIT_MAC(tm20LsbAddr[ii] + 4) , length ,& dataArr[1]);

    ii++;

    /**********************************************/
    /* now we are ready to read all that we wrote */
    /**********************************************/

    /*NOTE: the read data returns data in 32bits.
        unlike the write that 'waits' for 64 bits data

        so to check it I read first the 32 MSBits and only then the 32 LSBits.
    */

    iiMax = ii;
    for(ii = 0 ; ii < iiMax ; ii++)
    {
        if(0 == (ii & 1))
        {
            /*state new unit to the TM-device*/
            scibWriteMemory (deviceId , TEST_CONVERT_OFFSET_IN_TM_UNIT_MAC(tm23MsbAddr) , length ,& tmUnit[ii]);
            scibWriteMemory (deviceId , TEST_CONVERT_OFFSET_IN_TM_UNIT_MAC(tm23MsbAddr + 4) , length ,& dummyValue);
        }

        /* read data 32 MSBits */
        scibReadMemory (deviceId , TEST_CONVERT_OFFSET_IN_TM_UNIT_MAC(tm20LsbAddr[ii] + 4) , length ,& dataArr[1]);

        /* read data 32 LSBits */
        scibReadMemory (deviceId , TEST_CONVERT_OFFSET_IN_TM_UNIT_MAC(tm20LsbAddr[ii]) , length ,& dataArr[0]);


        /*check that the read data is as expected */
        if(dataArr[0] != tm20LsbAddr[ii])
        {
            simGeneralPrintf("smemBobcat2TmUnitBasicTest : ERROR: from address 43bits[0x%5.5x %4.4x] read data 32 LSBits[0x%8.8x] != written data 32 LSBits[0x%8.8x] \n",
                tmUnit[ii],tm20LsbAddr[ii],
                dataArr[0],tm20LsbAddr[ii]);
        }

        if(dataArr[1] != tmUnit[ii])
        {
            simGeneralPrintf("smemBobcat2TmUnitBasicTest : ERROR: from address 43bits[0x%5.5x %4.4x] read data 32 MSBits[0x%8.8x] != written data 32 MSBits[0x%8.8x] \n",
                tmUnit[ii],tm20LsbAddr[ii],
                dataArr[1],tmUnit[ii]);
        }
    }
    return;
}

/*******************************************************************************
*   smemBobcat2UnitGopIlkn
*
* DESCRIPTION:
*       Allocate address type specific memories -- GOP_ILKN
*
* INPUTS:
*       devObjPtr   - pointer to common device memory object.
*       unitPtr     - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemBobcat2UnitGopIlkn
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{

    /* chunks with formulas */
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            /*Ilkn*/
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000000, 0x00000024)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000030, 0x00000038)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000040, 0x00000040)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000048, 0x00000048)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000050, 0x00000050)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000058, 0x00000058)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000060, 0x00000060)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000068, 0x00000068)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000070, 0x00000074)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000080, 0x00000080)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000000C0, 0x000000F0)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000100, 0x00000114)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000120, 0x0000012C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000140, 0x0000015C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000180, 0x0000019C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000001C0, 0x000001DC)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000200, 0x00000218)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000220, 0x00000230)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000238, 0x00000274)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000280, 0x000003ac)}

            /*Converters registers */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000800, 0x00000804)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000810, 0x00000810)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000820, 0x00000820)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000840, 0x0000084c)}

            /*
            <ILKN WRP IP> Interlaken Wrapper IP Units/Packet Gen Registers Memory 2048 Byte
            <ILKN WRP IP> Interlaken Wrapper IP Units/PTP unit  Memory 2048 Byte
            <ILKN WRP IP> Interlaken Wrapper IP Units/TAI0      Memory 2048 Byte
            <ILKN WRP IP> Interlaken Wrapper IP Units/TAI1      Memory 2048 Byte
            */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001000, 0x000017FC)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002000, 0x000037FC)}

            /*PR unit*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001800, 0x00001904)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001A00, 0x00001A14)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001A20, 0x00001A34)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001A40, 0x00001A54)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001A60, 0x00001A74)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001A80, 0x00001A84)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001A90, 0x00001AA4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001B00, 0x00001B04)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001C00, 0x00001D2c)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001E00, 0x00001E40)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001EF8, 0x00001EFC)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001F00, 0x00001F14)}

        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem, numOfChunks, unitPtr);
    }

    {
        SMEM_CHUNK_BASIC_STC  chunksMemBc2A0[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000085c, 0x00000898)}
        };
        SMEM_CHUNK_BASIC_STC  chunksMemBc2B0[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000085c, 0x000008A0)}
        };


        if(SMEM_CHT_IS_SIP5_10_GET(devObjPtr) == 0)
        {
            smemUnitChunkAddBasicChunk(devObjPtr,unitPtr,
                ARRAY_NAME_AND_NUM_ELEMENTS_MAC(chunksMemBc2A0));
        }
        else
        {
            smemUnitChunkAddBasicChunk(devObjPtr,unitPtr,
                ARRAY_NAME_AND_NUM_ELEMENTS_MAC(chunksMemBc2B0));
        }
    }

}

/*******************************************************************************
*   smemBobcat2UnitGopLed
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Bobk gop LED unit
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr     - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemBobcat2UnitGopLed
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000000, 0x00000160)}
        };

        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);
        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
}
/*******************************************************************************
*   smemBobcat2UnitGopSmi
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the bobk  gop smi unit
*
* INPUTS:
*       devObjPtr   - pointer to common device memory object.
*       unitPtr     - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemBobcat2UnitGopSmi
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000000, 0x00000048)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }

}


/*******************************************************************************
*   smemBobcat2Init
*
* DESCRIPTION:
*       Init memory module for the Bobcat2 device.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
void smemBobcat2Init
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    GT_U32  ii,index;
    SMEM_BOBCAT2_UNIT_NAME_ENT* notSupportedUnitsArr;

    if(devObjPtr->activeMemPtr == NULL)
    {
        SMEM_CHT_IS_SIP5_GET(devObjPtr) = 1;

        if(devObjPtr->deviceFamily == SKERNEL_BOBCAT2_FAMILY)
        {
            if(devObjPtr->deviceRevisionId )
            {
                SMEM_CHT_IS_SIP5_10_GET(devObjPtr) = 1;
                notSupportedUnitsArr = bc2_B0_not_supported_units;
            }
            else
            {
                notSupportedUnitsArr = bc2_A0_not_supported_units;
            }

            /* remove not supported units ... we assume that we will not have A0
               and B0 together running at the same time */
            ii = 0;
            while(notSupportedUnitsArr[ii] != SMEM_BOBCAT2_UNIT_LAST_E)
            {
                index = notSupportedUnitsArr[ii];
                bobcat2UsedUnitsAddressesArray[index] = SMAIN_NOT_VALID_CNS;
                ii++;
            }

        }

        /* support active memory per unit */
        devObjPtr->supportActiveMemPerUnit = 1;

        if(devObjPtr->devMemUnitNameAndIndexPtr == NULL)
        {
            devObjPtr->devMemUnitNameAndIndexPtr = bobcat2UnitNameAndIndexArr;
            devObjPtr->genericUsedUnitsAddressesArray = bobcat2UsedUnitsAddressesArray;
            devObjPtr->genericNumUnitsAddresses = SMEM_BOBCAT2_UNIT_LAST_E;
        }

        devObjPtr->specialUnitsBaseAddr.timestampBaseAddr[0] = 0;/*timestampBaseAddr_Bobcat2[0];*/
        devObjPtr->specialUnitsBaseAddr.timestampBaseAddr[1] = 0;/*timestampBaseAddr_Bobcat2[1];*/

        /* set the pointer to the active memory */
        if(devObjPtr->activeMemPtr == NULL)
        {
            devObjPtr->activeMemPtr = smemBobcat2ActiveTable;
        }

        /* Bobcat2 supports 74 ports in TXQ_DQ */
        SET_IF_ZERO_MAC(devObjPtr->txqDqNumPorts,74);
        /* Bobcat2 supports DMA for cpu port 72 */
        SET_IF_ZERO_MAC(devObjPtr->dmaNumOfCpuPort,72);
        devObjPtr->cpuPortNoSpecialMac = 1;/* not have special mac for CPU port (when work in SGMII) */
        if(devObjPtr->supportTrafficManager_notAllowed == 0)
        {
            devObjPtr->supportTrafficManager = 1;
            devObjPtr->dmaNumOfTmPort = 73;
        }

        devObjPtr->unitBaseAddrAlignmentNumBits = 24;

        /* support device without the LMS unit */
        if(SMAIN_NOT_VALID_CNS != UNIT_BASE_ADDR_GET_ALLOW_NON_EXIST_UNIT_MAC(devObjPtr,UNIT_LMS))
        {
            devObjPtr->memUnitBaseAddrInfo.lms[0] = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_LMS);  /*BOBCAT2_LMS_UNIT_BASE_ADDR_CNS */
        }
        else
        {
            devObjPtr->memUnitBaseAddrInfo.lms[0] = SMAIN_NOT_VALID_CNS;
        }

        if(SMAIN_NOT_VALID_CNS != UNIT_BASE_ADDR_GET_ALLOW_NON_EXIST_UNIT_MAC(devObjPtr,UNIT_LMS1))
        {
            devObjPtr->memUnitBaseAddrInfo.lms[1] = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_LMS1); /*BOBCAT2_LMS1_UNIT_BASE_ADDR_CNS*/
        }
        else
        {
            devObjPtr->memUnitBaseAddrInfo.lms[1] = SMAIN_NOT_VALID_CNS;
        }

        if(SMAIN_NOT_VALID_CNS != UNIT_BASE_ADDR_GET_ALLOW_NON_EXIST_UNIT_MAC(devObjPtr,UNIT_LMS2))
        {
            devObjPtr->memUnitBaseAddrInfo.lms[2] = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_LMS2); /*BOBCAT2_LMS2_UNIT_BASE_ADDR_CNS*/
        }
        else
        {
            devObjPtr->memUnitBaseAddrInfo.lms[2] = SMAIN_NOT_VALID_CNS;
        }

        /* bobcat 2 only erratum (not in Lion3) */
        devObjPtr->errata.ingressPipeTrapToCpuUseNotOrig802dot1Vid = 1;
        devObjPtr->errata.mplsNonLsrNotModifyExp = 1;
        devObjPtr->errata.eqNotAccessE2phyForSniffedOnDropDuplication = 1;
        devObjPtr->errata.noTrafficToCpuOnBridgeBypassForwardingDecisionOnly = 1;
        devObjPtr->errata.l2iApplyMoveStaticCommandRegrdlessToSecurityBreach = 1;
        devObjPtr->errata.incorrectResolutionOfTtiActionFields = 1;
        devObjPtr->errata.dsaTagOrigSrcPortNotUpdated = 1;
        devObjPtr->errata.eplrNotCountCrc = 1;
        devObjPtr->errata.txqEgressMibCountersNotClearOnRead = 1;
        devObjPtr->errata.epclPortListModeForcePortMode = 1;

        if(0 == SMEM_CHT_IS_SIP5_10_GET(devObjPtr))
        {
            devObjPtr->errata.l2echoValidityForCfmInTti = 1;
            /* attach the link list */
            if (devObjPtr->registersDefaultsPtr == NULL)
            {
                devObjPtr->registersDefaultsPtr = &linkListElementsBc2A0_RegistersDefaults;
            }

            devObjPtr->errata.l2MllOutMcPckCnt = 1;
        }
        else
        {
            devObjPtr->pclSupport.pclUseCFMEtherType = 1;
            /* attach the link list */
            if (devObjPtr->registersDefaultsPtr == NULL)
            {
                devObjPtr->registersDefaultsPtr = &linkListElementsBc2B0_RegistersDefaults;
            }

            devObjPtr->supportNat44 = 1;
        }

        devObjPtr->errata.ipfixTodIncrementModeWraparoundIssues = 1;
        devObjPtr->errata.rxdmaPipIpv6WrongBitsTc = 1;


        /* function will be called from inside smemLion3Init(...) */
        if(devObjPtr->devMemSpecificDeviceMemInitPart1 == NULL)
        {
            devObjPtr->devMemSpecificDeviceMemInitPart1 = smemBobcat2SpecificDeviceMemInitPart1;
        }
        /* function will be called from inside smemLion3Init(...) */
        if(devObjPtr->devMemSpecificDeviceMemInitPart2 == NULL)
        {
            devObjPtr->devMemSpecificDeviceMemInitPart2 = smemBobcat2SpecificDeviceMemInitPart2;
        }

        SET_IF_ZERO_MAC(devObjPtr->numOfTaiUnits ,9);
        SET_IF_ZERO_MAC(devObjPtr->tcam_numBanksForHitNumGranularity, 2);
    }

    smemLion3Init(devObjPtr);

    /* Init the bobcat2 interrupts */
    smemBobcat2InitInterrupts(devObjPtr);

}

/*******************************************************************************
*   smemBobcat2Init2
*
* DESCRIPTION:
*       Init memory module for a device - after the load of the default
*           registers file
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
void smemBobcat2Init2
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    smemLion3Init2(devObjPtr);


    return;
}

enum{
    pexIntSum            = 1,
    pexErrSum            = 2,
    FuncUnitsIntsSum     = 3,
    DataPathIntSum       = 4,
    ports_int_sum        = 5,
    MgInternalIntSum     = 7,/*misc*/
    TxSDMASum            = 8,
    RxSDMASum            = 9,

    ports_1_int_sum      = 16,
    ports_2_int_sum      = 17,
};

enum{
    FuncUnitsInterrupts_TTIEngineInterruptCause = 1,
    FuncUnitsInterrupts_bridgeInterruptCause = 3,
    FuncUnitsInterrupts_FDBInterruptCauseReg = 4,
    FuncUnitsInterrupts_policerInterruptCause_0 = 8,
    FuncUnitsInterrupts_eqInterruptSummary = 9,
    FuncUnitsInterrupts_policerInterruptCause_2 = 10,
    FuncUnitsInterrupts_preEgrInterruptSummary  = 9,
    FuncUnitsInterrupts_HAInterruptCause = 11,
    FuncUnitsInterrupts_ERMRKInterruptCause = 13,
    FuncUnitsInterrupts_OAMUnitInterruptCause_1 = 14,
    FuncUnitsInterrupts_OAMUnitInterruptCause_0 = 16,
    FuncUnitsInterrupts_MLLInterruptCauseReg = 17,
    FuncUnitsInterrupts_policerInterruptCause_1 = 21,
    FuncUnitsInterrupts_CNCInterruptSummaryCauseReg_0 = 19,
    FuncUnitsInterrupts_CNCInterruptSummaryCauseReg_1 = 20,
    FuncUnitsInterrupts_txQDQInterruptSummary = 29,
};

static SKERNEL_INTERRUPT_REG_INFO_STC GlobalInterruptsSummary =
{
    /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(MG.globalInterrupt.globalInterruptCause)},/*SMEM_CHT_GLB_INT_CAUSE_REG*/
    /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(MG.globalInterrupt.globalInterruptMask)},
    /*myFatherInfo*/{
        /*myBitIndex*/0 ,
        /*interruptPtr*/NULL /* no father for me , thank you */
        },
    /*isTriggeredByWrite*/ 1
};

static SKERNEL_INTERRUPT_REG_INFO_STC  FuncUnitsInterruptsSummary =
{
    /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(MG.globalInterrupt.functionalInterruptsSummaryCause)},
    /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(MG.globalInterrupt.functionalInterruptsSummaryMask)},
    /*myFatherInfo*/{
        /*myBitIndex*/FuncUnitsIntsSum ,
        /*interruptPtr*/&GlobalInterruptsSummary
        },
    /*isTriggeredByWrite*/ 1
};

static SKERNEL_INTERRUPT_REG_INFO_STC  FuncUnitsInterruptsSummary1 =
{
    /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(MG.globalInterrupt.functionalInterruptsSummary1Cause)},
    /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(MG.globalInterrupt.functionalInterruptsSummary1Mask)},
    /*myFatherInfo*/{
        /*myBitIndex*/11 ,
        /*interruptPtr*/&GlobalInterruptsSummary
        },
    /*isTriggeredByWrite*/ 1
};

static SKERNEL_INTERRUPT_REG_INFO_STC  eqInterruptSummary =
{
    /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(EQ.preEgrInterrupt.preEgrInterruptSummary)}, /*SMEM_CHT_EQ_INT_CAUSE_REG*/
    /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(EQ.preEgrInterrupt.preEgrInterruptSummaryMask)},  /*SMEM_CHT_EQ_INT_MASK_REG*/
    /*myFatherInfo*/{
        /*myBitIndex*/FuncUnitsInterrupts_eqInterruptSummary ,
        /*interruptPtr*/&FuncUnitsInterruptsSummary
        },
    /*isTriggeredByWrite*/ 1
};

static SKERNEL_INTERRUPT_REG_INFO_STC  ingrSTCInterruptSummary =
{
    /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(EQ.ingrSTCInterrupt.ingrSTCInterruptSummary)}, /*SMEM_LION3_EQ_INGRESS_STC_INT_SUMMARY_CAUSE_REG*/
    /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(EQ.ingrSTCInterrupt.ingrSTCInterruptSummaryMask)},  /*SMEM_LION3_EQ_INGRESS_STC_INT_SUMMARY_MASK_REG*/
    /*myFatherInfo*/{
        /*myBitIndex*/1 ,
        /*interruptPtr*/&eqInterruptSummary
        },
    /*isTriggeredByWrite*/ 1
};


static SKERNEL_INTERRUPT_REG_INFO_STC  dataPathInterruptSummary =
{
    /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(MG.globalInterrupt.SERInterruptsSummary)},
    /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(MG.globalInterrupt.SERInterruptsMask)},
    /*myFatherInfo*/{
        /*myBitIndex*/DataPathIntSum ,
        /*interruptPtr*/&GlobalInterruptsSummary
        },
    /*isTriggeredByWrite*/ 1
};

static SKERNEL_INTERRUPT_REG_INFO_STC  BMInterruptSummary =
{
    /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(BM.BMInterrupts.summaryInterruptCause)},
    /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(BM.BMInterrupts.summaryInterruptMask)},
    /*myFatherInfo*/{
        /*myBitIndex*/1 ,
        /*interruptPtr*/&dataPathInterruptSummary
        },
    /*isTriggeredByWrite*/ 1
};

static SKERNEL_INTERRUPT_REG_INFO_STC  TXQInterruptSummary =
{
    /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(TXQ.queue.global.interrupt.transmitQueueInterruptSummaryCause)},
    /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(TXQ.queue.global.interrupt.transmitQueueInterruptSummaryMask)},
    /*myFatherInfo*/{
        /*myBitIndex*/25 ,
        /*interruptPtr*/&FuncUnitsInterruptsSummary
        },
    /*isTriggeredByWrite*/ 1
};

/* node for ingress STC summary of 16 ports */
#define  INGR_STC_INTERRUPT_CAUSE_FOR_16_PORTS_MAC(index) \
{                                                         \
    /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(EQ.ingrSTCInterrupt.ingrSTCInterruptCause[index])}, /*SMEM_CHT_INGRESS_STC_INT_CAUSE_REG*/ \
    /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(EQ.ingrSTCInterrupt.ingrSTCInterruptMask[index])},  /*SMEM_CHT_INGRESS_STC_INT_MASK_REG*/  \
    /*myFatherInfo*/{                                     \
        /*myBitIndex*/((index)+1) ,                       \
        /*interruptPtr*/&ingrSTCInterruptSummary          \
        },                                                \
    /*isTriggeredByWrite*/ 1                              \
}

static SKERNEL_INTERRUPT_REG_INFO_STC  preEgrInterruptSummary =
{
    /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(EQ.preEgrInterrupt.preEgrInterruptSummary)},/*SMEM_CHT_EQ_INT_CAUSE_REG*/
    /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(EQ.preEgrInterrupt.preEgrInterruptSummaryMask)},
    /*myFatherInfo*/{
        /*myBitIndex*/FuncUnitsInterrupts_preEgrInterruptSummary ,
        /*interruptPtr*/&FuncUnitsInterruptsSummary
        },
    /*isTriggeredByWrite*/ 1
};

static SKERNEL_INTERRUPT_REG_INFO_STC  txQDQInterruptSummary =
{
    /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(SIP5_TXQ_DQ[0].global.interrupt.txQDQInterruptSummaryCause)},/*SMEM_CHT_TX_QUEUE_INTERRUPT_CAUSE_REG , SMEM_LION_TXQ_DQ_INTR_SUM_CAUSE_REG*/
    /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(SIP5_TXQ_DQ[0].global.interrupt.txQDQInterruptSummaryMask)},
    /*myFatherInfo*/{
        /*myBitIndex*/FuncUnitsInterrupts_txQDQInterruptSummary ,
        /*interruptPtr*/&FuncUnitsInterruptsSummary
        },
    /*isTriggeredByWrite*/ 1
};

/* node for ingress STC summary */
#define  EGR_STC_INTERRUPT_CAUSE_PORTS_MAC(index)  \
{                                                         \
    /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(SIP5_TXQ_DQ[0].global.interrupt.egrSTCInterruptCause[index])}, /*SMEM_CHT_STC_INT_CAUSE_REG , SMEM_LION_TXQ_EGR_STC_INTR_CAUSE_REG*/ \
    /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(SIP5_TXQ_DQ[0].global.interrupt.egrSTCInterruptMask[index])},  /*SMEM_CHT_STC_INT_MASK_REG , SMEM_LION_TXQ_EGR_STC_INTR_MASK_REG*/  \
    /*myFatherInfo*/{                                     \
        /*myBitIndex*/((index)+6) ,                       \
        /*interruptPtr*/&txQDQInterruptSummary            \
        },                                                \
    /*isTriggeredByWrite*/ 1                              \
}
/* node in MG that aggregates ports 0..30 */
static SKERNEL_INTERRUPT_REG_INFO_STC  portsInterruptsSummary =
{
    /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(MG.globalInterrupt.stackPortsInterruptCause)},/*SMEM_CHT3_XG_PORTS_INTERRUPT_CAUSE_REG*/
    /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(MG.globalInterrupt.stackPortsInterruptMask)},/*SMEM_CHT3_XG_PORTS_INTERRUPT_MASK_REG*/
    /*myFatherInfo*/{
        /*myBitIndex*/ports_int_sum ,
        /*interruptPtr*/&GlobalInterruptsSummary
        },
    /*isTriggeredByWrite*/ 1
};
/* node in MG that aggregates ports 31..55 */
static SKERNEL_INTERRUPT_REG_INFO_STC  ports1InterruptsSummary =
{
    /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(MG.globalInterrupt.ports1InterruptsSummaryCause)},/**/
    /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(MG.globalInterrupt.ports1InterruptsSummaryMask)},/**/
    /*myFatherInfo*/{
        /*myBitIndex*/ports_1_int_sum ,
        /*interruptPtr*/&GlobalInterruptsSummary
        },
    /*isTriggeredByWrite*/ 1
};
/* node in MG that aggregates ports 56..71 */
static SKERNEL_INTERRUPT_REG_INFO_STC  ports2InterruptsSummary =
{
    /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(MG.globalInterrupt.ports2InterruptsSummaryCause)},/**/
    /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(MG.globalInterrupt.ports2InterruptsSummaryMask)},/**/
    /*myFatherInfo*/{
        /*myBitIndex*/ports_2_int_sum ,
        /*interruptPtr*/&GlobalInterruptsSummary
        },
    /*isTriggeredByWrite*/ 1
};

static SKERNEL_INTERRUPT_REG_INFO_STC CpuCodeRateLimitersInterruptSummary =
{
    /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(EQ.SCTRateLimitersInterrupt.CPUCodeRateLimitersInterruptSummary)},/**/
    /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(EQ.SCTRateLimitersInterrupt.CPUCodeRateLimitersInterruptSummaryMask)},/**/
    /*myFatherInfo*/{
        /*myBitIndex*/2 ,
        /*interruptPtr*/&preEgrInterruptSummary
        },
    /*isTriggeredByWrite*/ 1
};

static SKERNEL_INTERRUPT_REG_INFO_STC TcamInterruptsSummary =
{
    /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(TCAM.tcamInterrupts.TCAMInterruptsSummaryCause)},/**/
    /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(TCAM.tcamInterrupts.TCAMInterruptsSummaryMask)},/**/
    /*myFatherInfo*/{
        /*myBitIndex*/5 ,
        /*interruptPtr*/&FuncUnitsInterruptsSummary
        },
    /*isTriggeredByWrite*/ 1
};

static SKERNEL_INTERRUPT_REG_INFO_STC RxDmaInterruptsSummary =
{
    /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(rxDMA[0].interrupts.rxDMAInterruptSummaryCause)},/**/
    /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(rxDMA[0].interrupts.rxDMAInterruptSummaryMask)},/**/
    /*myFatherInfo*/{
        /*myBitIndex*/2 ,
        /*interruptPtr*/&dataPathInterruptSummary
        },
    /*isTriggeredByWrite*/ 1
};

static SKERNEL_INTERRUPT_REG_INFO_STC RxDma1InterruptsSummary =
{
    /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(rxDMA[1].interrupts.rxDMAInterruptSummaryCause)},/**/
    /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(rxDMA[1].interrupts.rxDMAInterruptSummaryMask)},/**/
    /*myFatherInfo*/{
        /*myBitIndex*/12 ,
        /*interruptPtr*/&dataPathInterruptSummary
        },
    /*isTriggeredByWrite*/ 1
};

static SKERNEL_INTERRUPT_REG_INFO_STC TxDmaInterruptsSummary =
{
    /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(TXDMA[0].interrupts.txDMAInterruptSummary)},/**/
    /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(TXDMA[0].interrupts.txDMAInterruptSummaryMask)},/**/
    /*myFatherInfo*/{
        /*myBitIndex*/6 ,
        /*interruptPtr*/&dataPathInterruptSummary
        },
    /*isTriggeredByWrite*/ 1
};

static SKERNEL_INTERRUPT_REG_INFO_STC TxDma1InterruptsSummary =
{
    /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(TXDMA[1].interrupts.txDMAInterruptSummary)},/**/
    /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(TXDMA[1].interrupts.txDMAInterruptSummaryMask)},/**/
    /*myFatherInfo*/{
        /*myBitIndex*/10 ,
        /*interruptPtr*/&dataPathInterruptSummary
        },
    /*isTriggeredByWrite*/ 1
};

static SKERNEL_INTERRUPT_REG_INFO_STC TxFifoInterruptsSummary =
{
    /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(TXFIFO[0].interrupts.txFIFOInterruptSummary)},/**/
    /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(TXFIFO[0].interrupts.txFIFOInterruptSummaryMask)},/**/
    /*myFatherInfo*/{
        /*myBitIndex*/7 ,
        /*interruptPtr*/&dataPathInterruptSummary
        },
    /*isTriggeredByWrite*/ 1
};

static SKERNEL_INTERRUPT_REG_INFO_STC TxFifo1InterruptsSummary =
{
    /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(TXFIFO[1].interrupts.txFIFOInterruptSummary)},/**/
    /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(TXFIFO[1].interrupts.txFIFOInterruptSummaryMask)},/**/
    /*myFatherInfo*/{
        /*myBitIndex*/11 ,
        /*interruptPtr*/&dataPathInterruptSummary
        },
    /*isTriggeredByWrite*/ 1
};

static SKERNEL_INTERRUPT_REG_INFO_STC EthTxFifoInterruptsSummary =
{
    /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(ETH.TXFIFO.interrupts.EthTxFIFOInterruptsSummary)},/**/
    /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(ETH.TXFIFO.interrupts.EthTxFIFOInterruptsSummaryMask)},/**/
    /*myFatherInfo*/{
        /*myBitIndex*/4 ,
        /*interruptPtr*/&FuncUnitsInterruptsSummary1
        },
    /*isTriggeredByWrite*/ 1
};

static SKERNEL_INTERRUPT_REG_INFO_STC MppmInterruptsSummary =
{
    /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(MPPM.interrupts.MPPMInterruptsSummary)},/**/
    /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(MPPM.interrupts.MPPMInterruptsSummaryMask)},/**/
    /*myFatherInfo*/{
        /*myBitIndex*/9 ,
        /*interruptPtr*/&dataPathInterruptSummary
        },
    /*isTriggeredByWrite*/ 1,
};

static SKERNEL_INTERRUPT_REG_INFO_STC PfcInterruptSummary =
{
    /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(TXQ.pfc.PFCInterrupts.PFCInterruptSummaryCause)},/**/
    /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(TXQ.pfc.PFCInterrupts.PFCInterruptSummaryMask)},/**/
    /*myFatherInfo*/{
        /*myBitIndex*/27 ,
        /*interruptPtr*/&FuncUnitsInterruptsSummary
        },
    /*isTriggeredByWrite*/ 1,
};

/* node for XLG mac ports */
#define  XLG_MAC_INTERRUPT_CAUSE_MAC(port) \
{                                                         \
    /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(GOP.ports.XLGIP[port].portInterruptCause)}, /*SMEM_CHT_PORT_INT_CAUSE_REG*/ \
    /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(GOP.ports.XLGIP[port].portInterruptMask)},  /**/  \
    /*myFatherInfo*/{                                     \
        /*myBitIndex*/1 ,                                 \
        /*interruptPtr*/&xlgInterruptSummary_##port       \
        },                                                \
    /*isTriggeredByWrite*/ 1                              \
}


/* define the XLG port summary directed from MG ports summary */
#define  DEFINE_XLG_SUMMARY_PORT_IN_MG_INTERRUPT_SUMMARY_MAC(port,bitInMg,regInMg) \
static SKERNEL_INTERRUPT_REG_INFO_STC  xlgInterruptSummary_##port =  \
{                                                  \
    /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(GOP.ports.XLGIP[port].externalUnitsInterruptsCause)}, /*SMEM_LION_XG_PORTS_SUMMARY_INTERRUPT_CAUSE_REG*/  \
    /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(GOP.ports.XLGIP[port].externalUnitsInterruptsMask)},  /*SMEM_LION2_XG_PORTS_SUMMARY_INTERRUPT_MASK_REG*/  \
    /*myFatherInfo*/{                              \
        /*myBitIndex*/ bitInMg ,                   \
        /*interruptPtr*/&regInMg                   \
        },                                         \
    /*isTriggeredByWrite*/ 1                       \
}


/* define the XLG port summary  , for port 48..55 in ports_1_int_sum */
#define  DEFINE_XLG_SUMMARY_PORT_48_55_INTERRUPT_SUMMARY_MAC(port) \
    DEFINE_XLG_SUMMARY_PORT_IN_MG_INTERRUPT_SUMMARY_MAC(port,((port - 31) + 1)/*bitInMg*/,ports1InterruptsSummary/*regInMg*/)

/* define the XLG port summary  , for port 56..71 in ports_2_int_sum */
#define  DEFINE_XLG_SUMMARY_PORT_56_71_INTERRUPT_SUMMARY_MAC(port) \
    DEFINE_XLG_SUMMARY_PORT_IN_MG_INTERRUPT_SUMMARY_MAC(port,((port - 56) + 1)/*bitInMg*/,ports2InterruptsSummary/*regInMg*/)

/* define the nodes of XLG summary interrupt register , ports 48..71 */
DEFINE_XLG_SUMMARY_PORT_48_55_INTERRUPT_SUMMARY_MAC(48);
DEFINE_XLG_SUMMARY_PORT_48_55_INTERRUPT_SUMMARY_MAC(49);
DEFINE_XLG_SUMMARY_PORT_48_55_INTERRUPT_SUMMARY_MAC(50);
DEFINE_XLG_SUMMARY_PORT_48_55_INTERRUPT_SUMMARY_MAC(51);
DEFINE_XLG_SUMMARY_PORT_48_55_INTERRUPT_SUMMARY_MAC(52);
DEFINE_XLG_SUMMARY_PORT_48_55_INTERRUPT_SUMMARY_MAC(53);
DEFINE_XLG_SUMMARY_PORT_48_55_INTERRUPT_SUMMARY_MAC(54);
DEFINE_XLG_SUMMARY_PORT_48_55_INTERRUPT_SUMMARY_MAC(55);

DEFINE_XLG_SUMMARY_PORT_56_71_INTERRUPT_SUMMARY_MAC(56);
DEFINE_XLG_SUMMARY_PORT_56_71_INTERRUPT_SUMMARY_MAC(57);
DEFINE_XLG_SUMMARY_PORT_56_71_INTERRUPT_SUMMARY_MAC(58);
DEFINE_XLG_SUMMARY_PORT_56_71_INTERRUPT_SUMMARY_MAC(59);
DEFINE_XLG_SUMMARY_PORT_56_71_INTERRUPT_SUMMARY_MAC(60);
DEFINE_XLG_SUMMARY_PORT_56_71_INTERRUPT_SUMMARY_MAC(61);
DEFINE_XLG_SUMMARY_PORT_56_71_INTERRUPT_SUMMARY_MAC(62);
DEFINE_XLG_SUMMARY_PORT_56_71_INTERRUPT_SUMMARY_MAC(63);
DEFINE_XLG_SUMMARY_PORT_56_71_INTERRUPT_SUMMARY_MAC(64);
DEFINE_XLG_SUMMARY_PORT_56_71_INTERRUPT_SUMMARY_MAC(65);
DEFINE_XLG_SUMMARY_PORT_56_71_INTERRUPT_SUMMARY_MAC(66);
DEFINE_XLG_SUMMARY_PORT_56_71_INTERRUPT_SUMMARY_MAC(67);
DEFINE_XLG_SUMMARY_PORT_56_71_INTERRUPT_SUMMARY_MAC(68);
DEFINE_XLG_SUMMARY_PORT_56_71_INTERRUPT_SUMMARY_MAC(69);
DEFINE_XLG_SUMMARY_PORT_56_71_INTERRUPT_SUMMARY_MAC(70);
DEFINE_XLG_SUMMARY_PORT_56_71_INTERRUPT_SUMMARY_MAC(71);

/* node for Gig mac ports */
#define  GIGA_MAC_INTERRUPT_CAUSE_MAC(port)               \
{                                                         \
    /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(GOP.ports.gigPort[port].portInterruptCause)}, /*SMEM_CHT_PORT_INT_CAUSE_REG*/ \
    /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(GOP.ports.gigPort[port].portInterruptMask)},  /**/                            \
    /*myFatherInfo*/{                                     \
        /*myBitIndex*/1 ,                                 \
        /*interruptPtr*/&gigaInterruptSummary_##port      \
    },                                                    \
    /*isTriggeredByWrite*/ 1                              \
}

/* define the Gig port summary directed from MG ports summary */
#define  DEFINE_GIGA_SUMMARY_PORT_IN_MG_INTERRUPT_SUMMARY_MAC(port,bitInMg,regInMg) \
static SKERNEL_INTERRUPT_REG_INFO_STC  gigaInterruptSummary_##port = \
{                                                  \
    /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(GOP.ports.gigPort[port].portInterruptSummaryCause)}, /**/  \
    /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(GOP.ports.gigPort[port].portInterruptSummaryMask)},  /**/  \
    /*myFatherInfo*/{                              \
        /*myBitIndex*/ bitInMg ,                   \
        /*interruptPtr*/&regInMg                   \
        },                                         \
    /*isTriggeredByWrite*/ 1                       \
}

/* define the Gig port summary directed from XLG summary */
#define  DEFINE_GIGA_SUMMARY_PORT_IN_XLG_INTERRUPT_SUMMARY_MAC(port) \
static SKERNEL_INTERRUPT_REG_INFO_STC  gigaInterruptSummary_##port = \
{                                                  \
    /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(GOP.ports.gigPort[port].portInterruptSummaryCause)}, /**/  \
    /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(GOP.ports.gigPort[port].portInterruptSummaryMask)},  /**/  \
    /*myFatherInfo*/{                               \
        /*myBitIndex*/ 2 ,                          \
        /*interruptPtr*/&xlgInterruptSummary_##port \
        },                                          \
    /*isTriggeredByWrite*/ 1                        \
}

/* define the Gig port summary  , for port 0..30 in ports_int_sum */
#define  DEFINE_GIGA_SUMMARY_PORT_0_30_INTERRUPT_SUMMARY_MAC(port) \
    DEFINE_GIGA_SUMMARY_PORT_IN_MG_INTERRUPT_SUMMARY_MAC(port,(port + 1)/*bitInMg*/,portsInterruptsSummary/*regInMg*/)

/* define the Gig port summary  , for port 31..47 in ports_1_int_sum */
#define  DEFINE_GIGA_SUMMARY_PORT_31_47_INTERRUPT_SUMMARY_MAC(port) \
    DEFINE_GIGA_SUMMARY_PORT_IN_MG_INTERRUPT_SUMMARY_MAC(port,((port - 31) + 1)/*bitInMg*/,ports1InterruptsSummary/*regInMg*/)

/* define the Gig port summary  , for port 48..71 (in XLG summary) */
#define  DEFINE_GIGA_SUMMARY_PORT_48_71_INTERRUPT_SUMMARY_MAC(port) \
    DEFINE_GIGA_SUMMARY_PORT_IN_XLG_INTERRUPT_SUMMARY_MAC(port)

/* define the nodes of Giga summary interrupt register , ports 0..71 */
DEFINE_GIGA_SUMMARY_PORT_0_30_INTERRUPT_SUMMARY_MAC (0 );
DEFINE_GIGA_SUMMARY_PORT_0_30_INTERRUPT_SUMMARY_MAC (1 );
DEFINE_GIGA_SUMMARY_PORT_0_30_INTERRUPT_SUMMARY_MAC (2 );
DEFINE_GIGA_SUMMARY_PORT_0_30_INTERRUPT_SUMMARY_MAC (3 );
DEFINE_GIGA_SUMMARY_PORT_0_30_INTERRUPT_SUMMARY_MAC (4 );
DEFINE_GIGA_SUMMARY_PORT_0_30_INTERRUPT_SUMMARY_MAC (5 );
DEFINE_GIGA_SUMMARY_PORT_0_30_INTERRUPT_SUMMARY_MAC (6 );
DEFINE_GIGA_SUMMARY_PORT_0_30_INTERRUPT_SUMMARY_MAC (7 );
DEFINE_GIGA_SUMMARY_PORT_0_30_INTERRUPT_SUMMARY_MAC (8 );
DEFINE_GIGA_SUMMARY_PORT_0_30_INTERRUPT_SUMMARY_MAC (9 );
DEFINE_GIGA_SUMMARY_PORT_0_30_INTERRUPT_SUMMARY_MAC (10);
DEFINE_GIGA_SUMMARY_PORT_0_30_INTERRUPT_SUMMARY_MAC (11);
DEFINE_GIGA_SUMMARY_PORT_0_30_INTERRUPT_SUMMARY_MAC (12);
DEFINE_GIGA_SUMMARY_PORT_0_30_INTERRUPT_SUMMARY_MAC (13);
DEFINE_GIGA_SUMMARY_PORT_0_30_INTERRUPT_SUMMARY_MAC (14);
DEFINE_GIGA_SUMMARY_PORT_0_30_INTERRUPT_SUMMARY_MAC (15);
DEFINE_GIGA_SUMMARY_PORT_0_30_INTERRUPT_SUMMARY_MAC (16);
DEFINE_GIGA_SUMMARY_PORT_0_30_INTERRUPT_SUMMARY_MAC (17);
DEFINE_GIGA_SUMMARY_PORT_0_30_INTERRUPT_SUMMARY_MAC (18);
DEFINE_GIGA_SUMMARY_PORT_0_30_INTERRUPT_SUMMARY_MAC (19);
DEFINE_GIGA_SUMMARY_PORT_0_30_INTERRUPT_SUMMARY_MAC (20);
DEFINE_GIGA_SUMMARY_PORT_0_30_INTERRUPT_SUMMARY_MAC (21);
DEFINE_GIGA_SUMMARY_PORT_0_30_INTERRUPT_SUMMARY_MAC (22);
DEFINE_GIGA_SUMMARY_PORT_0_30_INTERRUPT_SUMMARY_MAC (23);
DEFINE_GIGA_SUMMARY_PORT_0_30_INTERRUPT_SUMMARY_MAC (24);
DEFINE_GIGA_SUMMARY_PORT_0_30_INTERRUPT_SUMMARY_MAC (25);
DEFINE_GIGA_SUMMARY_PORT_0_30_INTERRUPT_SUMMARY_MAC (26);
DEFINE_GIGA_SUMMARY_PORT_0_30_INTERRUPT_SUMMARY_MAC (27);
DEFINE_GIGA_SUMMARY_PORT_0_30_INTERRUPT_SUMMARY_MAC (28);
DEFINE_GIGA_SUMMARY_PORT_0_30_INTERRUPT_SUMMARY_MAC (29);
DEFINE_GIGA_SUMMARY_PORT_0_30_INTERRUPT_SUMMARY_MAC (30);

DEFINE_GIGA_SUMMARY_PORT_31_47_INTERRUPT_SUMMARY_MAC(31);
DEFINE_GIGA_SUMMARY_PORT_31_47_INTERRUPT_SUMMARY_MAC(32);
DEFINE_GIGA_SUMMARY_PORT_31_47_INTERRUPT_SUMMARY_MAC(33);
DEFINE_GIGA_SUMMARY_PORT_31_47_INTERRUPT_SUMMARY_MAC(34);
DEFINE_GIGA_SUMMARY_PORT_31_47_INTERRUPT_SUMMARY_MAC(35);
DEFINE_GIGA_SUMMARY_PORT_31_47_INTERRUPT_SUMMARY_MAC(36);
DEFINE_GIGA_SUMMARY_PORT_31_47_INTERRUPT_SUMMARY_MAC(37);
DEFINE_GIGA_SUMMARY_PORT_31_47_INTERRUPT_SUMMARY_MAC(38);
DEFINE_GIGA_SUMMARY_PORT_31_47_INTERRUPT_SUMMARY_MAC(39);
DEFINE_GIGA_SUMMARY_PORT_31_47_INTERRUPT_SUMMARY_MAC(40);
DEFINE_GIGA_SUMMARY_PORT_31_47_INTERRUPT_SUMMARY_MAC(41);
DEFINE_GIGA_SUMMARY_PORT_31_47_INTERRUPT_SUMMARY_MAC(42);
DEFINE_GIGA_SUMMARY_PORT_31_47_INTERRUPT_SUMMARY_MAC(43);
DEFINE_GIGA_SUMMARY_PORT_31_47_INTERRUPT_SUMMARY_MAC(44);
DEFINE_GIGA_SUMMARY_PORT_31_47_INTERRUPT_SUMMARY_MAC(45);
DEFINE_GIGA_SUMMARY_PORT_31_47_INTERRUPT_SUMMARY_MAC(46);
DEFINE_GIGA_SUMMARY_PORT_31_47_INTERRUPT_SUMMARY_MAC(47);

DEFINE_GIGA_SUMMARY_PORT_48_71_INTERRUPT_SUMMARY_MAC(48);
DEFINE_GIGA_SUMMARY_PORT_48_71_INTERRUPT_SUMMARY_MAC(49);
DEFINE_GIGA_SUMMARY_PORT_48_71_INTERRUPT_SUMMARY_MAC(50);
DEFINE_GIGA_SUMMARY_PORT_48_71_INTERRUPT_SUMMARY_MAC(51);
DEFINE_GIGA_SUMMARY_PORT_48_71_INTERRUPT_SUMMARY_MAC(52);
DEFINE_GIGA_SUMMARY_PORT_48_71_INTERRUPT_SUMMARY_MAC(53);
DEFINE_GIGA_SUMMARY_PORT_48_71_INTERRUPT_SUMMARY_MAC(54);
DEFINE_GIGA_SUMMARY_PORT_48_71_INTERRUPT_SUMMARY_MAC(55);
DEFINE_GIGA_SUMMARY_PORT_48_71_INTERRUPT_SUMMARY_MAC(56);
DEFINE_GIGA_SUMMARY_PORT_48_71_INTERRUPT_SUMMARY_MAC(57);
DEFINE_GIGA_SUMMARY_PORT_48_71_INTERRUPT_SUMMARY_MAC(58);
DEFINE_GIGA_SUMMARY_PORT_48_71_INTERRUPT_SUMMARY_MAC(59);
DEFINE_GIGA_SUMMARY_PORT_48_71_INTERRUPT_SUMMARY_MAC(60);
DEFINE_GIGA_SUMMARY_PORT_48_71_INTERRUPT_SUMMARY_MAC(61);
DEFINE_GIGA_SUMMARY_PORT_48_71_INTERRUPT_SUMMARY_MAC(62);
DEFINE_GIGA_SUMMARY_PORT_48_71_INTERRUPT_SUMMARY_MAC(63);
DEFINE_GIGA_SUMMARY_PORT_48_71_INTERRUPT_SUMMARY_MAC(64);
DEFINE_GIGA_SUMMARY_PORT_48_71_INTERRUPT_SUMMARY_MAC(65);
DEFINE_GIGA_SUMMARY_PORT_48_71_INTERRUPT_SUMMARY_MAC(66);
DEFINE_GIGA_SUMMARY_PORT_48_71_INTERRUPT_SUMMARY_MAC(67);
DEFINE_GIGA_SUMMARY_PORT_48_71_INTERRUPT_SUMMARY_MAC(68);
DEFINE_GIGA_SUMMARY_PORT_48_71_INTERRUPT_SUMMARY_MAC(69);
DEFINE_GIGA_SUMMARY_PORT_48_71_INTERRUPT_SUMMARY_MAC(70);
DEFINE_GIGA_SUMMARY_PORT_48_71_INTERRUPT_SUMMARY_MAC(71);

#define CPU_CODE_RATE_LIMITERS_INTERRUPT_CAUSE_MAC(n)                                                                                  \
        {                                                                                                                              \
            /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(EQ.SCTRateLimitersInterrupt.CPUCodeRateLimitersInterruptCause[n-1])},    \
            /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(EQ.SCTRateLimitersInterrupt.CPUCodeRateLimitersInterruptMask[n-1])},     \
            /*myFatherInfo*/{                                                                                                          \
                /*myBitIndex*/n ,                                                                                                      \
                /*interruptPtr*/&CpuCodeRateLimitersInterruptSummary                                                                   \
                },                                                                                                                     \
            /*isTriggeredByWrite*/ 1                                                                                                   \
        }


static SKERNEL_INTERRUPT_REG_INFO_STC mg_sdma_interrupts[] =
{
    {
        /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(MG.SDMARegs.receiveSDMAInterruptCause)}, /*SMEM_CHT_SDMA_INT_CAUSE_REG*/
        /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(MG.SDMARegs.receiveSDMAInterruptMask)},
        /*myFatherInfo*/{
            /*myBitIndex*/RxSDMASum ,
            /*interruptPtr*/&GlobalInterruptsSummary
            },
        /*isTriggeredByWrite*/ 1
    },
    {
        /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(MG.SDMARegs.transmitSDMAInterruptCause)},/*SMEM_CHT_TX_SDMA_INT_CAUSE_REG*/
        /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(MG.SDMARegs.transmitSDMAInterruptMask)},
        /*myFatherInfo*/{
            /*myBitIndex*/TxSDMASum ,
            /*interruptPtr*/&GlobalInterruptsSummary
            },
        /*isTriggeredByWrite*/ 1
    }
};

static SKERNEL_INTERRUPT_REG_INFO_STC tti_interrupts[] =
{
    {
        /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(TTI.TTIEngine.TTIEngineInterruptCause)},/*SMEM_LION3_TTI_INTERRUPT_CAUSE_REG*/
        /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(TTI.TTIEngine.TTIEngineInterruptMask)},
        /*myFatherInfo*/{
            /*myBitIndex*/FuncUnitsInterrupts_TTIEngineInterruptCause ,
            /*interruptPtr*/&FuncUnitsInterruptsSummary
            },
        /*isTriggeredByWrite*/ 1
    }
};

static SKERNEL_INTERRUPT_REG_INFO_STC bridge_interrupts[] =
{
    {
        /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(L2I.bridgeEngineInterrupts.bridgeInterruptCause)},/*SMEM_CHT_BRIDGE_INT_CAUSE_REG*/
        /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(L2I.bridgeEngineInterrupts.bridgeInterruptMask)},
        /*myFatherInfo*/{
            /*myBitIndex*/FuncUnitsInterrupts_bridgeInterruptCause ,
            /*interruptPtr*/&FuncUnitsInterruptsSummary
            },
        /*isTriggeredByWrite*/ 1
    }
};

static SKERNEL_INTERRUPT_REG_INFO_STC mac_tbl_intrrupts[] =
{
    {
    /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(FDB.FDBCore.FDBInterrupt.FDBInterruptCauseReg)},/*SMEM_CHT_MAC_TBL_INTR_CAUSE_REG*/
    /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(FDB.FDBCore.FDBInterrupt.FDBInterruptMaskReg)},
    /*myFatherInfo*/{
        /*myBitIndex*/FuncUnitsInterrupts_FDBInterruptCauseReg ,
        /*interruptPtr*/&FuncUnitsInterruptsSummary
        },
    /*isTriggeredByWrite*/ 1
    }
};

static SKERNEL_INTERRUPT_REG_INFO_STC ha_interrupts[] =
{
    {
    /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(HA.HAInterruptCause)},/*SMEM_LION3_HA_INTERRUPT_CAUSE_REG*/
    /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(HA.HAInterruptMask)},
    /*myFatherInfo*/{
        /*myBitIndex*/FuncUnitsInterrupts_HAInterruptCause ,
        /*interruptPtr*/&FuncUnitsInterruptsSummary
        },
    /*isTriggeredByWrite*/ 1
    }
};

static SKERNEL_INTERRUPT_REG_INFO_STC ermrk_interrupts[] =
{
    {
        /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(ERMRK.ERMRKInterruptCause)},/*SMEM_LION3_ERMRK_INTERRUPT_CAUSE_REG*/
        /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(ERMRK.ERMRKInterruptMask)},
        /*myFatherInfo*/{
            /*myBitIndex*/FuncUnitsInterrupts_ERMRKInterruptCause ,
            /*interruptPtr*/&FuncUnitsInterruptsSummary
            },
        /*isTriggeredByWrite*/ 1
    }
};

static SKERNEL_INTERRUPT_REG_INFO_STC oam_interrupts[] =
{
    {
        /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(OAMUnit[0].OAMUnitInterruptCause)},/*SMEM_LION2_OAM_INTR_CAUSE_REG*/
        /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(OAMUnit[0].OAMUnitInterruptMask)},
        /*myFatherInfo*/{
            /*myBitIndex*/FuncUnitsInterrupts_OAMUnitInterruptCause_0 ,
            /*interruptPtr*/&FuncUnitsInterruptsSummary
            },
        /*isTriggeredByWrite*/ 1
    }
   ,{
        /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(OAMUnit[1].OAMUnitInterruptCause)},/*SMEM_LION2_OAM_INTR_CAUSE_REG*/
        /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(OAMUnit[1].OAMUnitInterruptMask)},
        /*myFatherInfo*/{
            /*myBitIndex*/FuncUnitsInterrupts_OAMUnitInterruptCause_1 ,
            /*interruptPtr*/&FuncUnitsInterruptsSummary
            },
            /*isTriggeredByWrite*/ 1
    }
};

static SKERNEL_INTERRUPT_REG_INFO_STC mll_interrupts[] =
{
    {
        /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(MLL.MLLGlobalCtrl.MLLInterruptCauseReg)},/*SMEM_LION3_MLL_INTERRUPT_CAUSE_REG*/
        /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(MLL.MLLGlobalCtrl.MLLInterruptMaskReg)},
        /*myFatherInfo*/{
            /*myBitIndex*/FuncUnitsInterrupts_MLLInterruptCauseReg ,
            /*interruptPtr*/&FuncUnitsInterruptsSummary
            },
        /*isTriggeredByWrite*/ 1
    }
};
static SKERNEL_INTERRUPT_REG_INFO_STC cnc_interrupts[] =
{
    {
        /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(CNC[0].globalRegs.CNCInterruptSummaryCauseReg)},/*SMEM_CHT3_CNC_INTR_CAUSE_REG*/
        /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(CNC[0].globalRegs.CNCInterruptSummaryMaskReg)},
        /*myFatherInfo*/{
            /*myBitIndex*/FuncUnitsInterrupts_CNCInterruptSummaryCauseReg_0 ,
            /*interruptPtr*/&FuncUnitsInterruptsSummary
            },
        /*isTriggeredByWrite*/ 1
    }
   ,{
        /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(CNC[1].globalRegs.CNCInterruptSummaryCauseReg)},/*SMEM_CHT3_CNC_INTR_CAUSE_REG*/
        /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(CNC[1].globalRegs.CNCInterruptSummaryMaskReg)},
        /*myFatherInfo*/{
            /*myBitIndex*/FuncUnitsInterrupts_CNCInterruptSummaryCauseReg_1 ,
            /*interruptPtr*/&FuncUnitsInterruptsSummary
            },
        /*isTriggeredByWrite*/ 1
    }
};

static SKERNEL_INTERRUPT_REG_INFO_STC policer_ipfix_interrupts[] =
{
    {
        /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(PLR[0].policerInterruptCause)},/*SMEM_XCAT_POLICER_IPFIX_INT_CAUSE_REG*/
        /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(PLR[0].policerInterruptMask)},
        /*myFatherInfo*/{
            /*myBitIndex*/FuncUnitsInterrupts_policerInterruptCause_0 ,
            /*interruptPtr*/&FuncUnitsInterruptsSummary
            },
        /*isTriggeredByWrite*/ 1
    }
   ,{
        /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(PLR[1].policerInterruptCause)},/*SMEM_XCAT_POLICER_IPFIX_INT_CAUSE_REG*/
        /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(PLR[1].policerInterruptMask)},
        /*myFatherInfo*/{
            /*myBitIndex*/FuncUnitsInterrupts_policerInterruptCause_1 ,
            /*interruptPtr*/&FuncUnitsInterruptsSummary
            },
        /*isTriggeredByWrite*/ 1
    }
   ,{
        /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(PLR[2].policerInterruptCause)},/*SMEM_XCAT_POLICER_IPFIX_INT_CAUSE_REG*/
        /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(PLR[2].policerInterruptMask)},
        /*myFatherInfo*/{
            /*myBitIndex*/FuncUnitsInterrupts_policerInterruptCause_2 ,
            /*interruptPtr*/&FuncUnitsInterruptsSummary
            },
        /*isTriggeredByWrite*/ 1
    }
};

static SKERNEL_INTERRUPT_REG_INFO_STC mg_global_misc_interrupts[] =
{
    {
        /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(MG.globalInterrupt.miscellaneousInterruptCause)},/*SMEM_CHT_MISC_INTR_CAUSE_REG*/
        /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(MG.globalInterrupt.miscellaneousInterruptMask)},
        /*myFatherInfo*/{
            /*myBitIndex*/MgInternalIntSum ,
            /*interruptPtr*/&GlobalInterruptsSummary
            },
        /*isTriggeredByWrite*/ 1
    }
};

static SKERNEL_INTERRUPT_REG_INFO_STC bm_general_cause_interrupts[] =
{
    {
    /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(BM.BMInterrupts.generalInterruptCause1)},/*BM General Cause Reg1*/
    /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(BM.BMInterrupts.generalInterruptMask1)},/*BM General Mask Reg1*/
    /*myFatherInfo*/{
        /*myBitIndex*/1 ,
        /*interruptPtr*/&BMInterruptSummary
        },
    /*isTriggeredByWrite*/ 1
    }
};

static SKERNEL_INTERRUPT_REG_INFO_STC txq_queue_global_interrupts[] =
{
    {
        /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(TXQ.queue.global.interrupt.generalInterrupt1Cause)},
        /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(TXQ.queue.global.interrupt.generalInterrupt1Mask)},
        /*myFatherInfo*/{
            /*myBitIndex*/1 ,
            /*interruptPtr*/&TXQInterruptSummary
            },
        /*isTriggeredByWrite*/ 1
    }
   ,{
        /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(TXQ.queue.global.interrupt.portDescFullInterruptCause0)},
        /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(TXQ.queue.global.interrupt.portDescFullInterruptMask0)},
        /*myFatherInfo*/{
            /*myBitIndex*/2 ,
            /*interruptPtr*/&TXQInterruptSummary
            },
        /*isTriggeredByWrite*/ 1
    }
   ,{
        /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(TXQ.queue.global.interrupt.portDescFullInterruptCause1)},
        /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(TXQ.queue.global.interrupt.portDescFullInterruptMask1)},
        /*myFatherInfo*/{
            /*myBitIndex*/3 ,
            /*interruptPtr*/&TXQInterruptSummary
            },
        /*isTriggeredByWrite*/ 1
     }
    ,{
            /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(TXQ.qcn.QCNInterruptCause)},
            /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(TXQ.qcn.QCNInterruptMask)},
            /*myFatherInfo*/{
                /*myBitIndex*/28 ,
                /*interruptPtr*/&FuncUnitsInterruptsSummary
                },
            /*isTriggeredByWrite*/ 1
    }
   ,{
            /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(TXQ.pfc.PFCInterrupts.PFCParityInterruptCause)},
            /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(TXQ.pfc.PFCInterrupts.PFCParityInterruptMask)},
            /*myFatherInfo*/{
                /*myBitIndex*/1 ,
                /*interruptPtr*/&PfcInterruptSummary
                },
            /*isTriggeredByWrite*/ 1
    }
   ,{
        /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(TXQ.queue.global.interrupt.portDescFullInterruptCause2)},
        /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(TXQ.queue.global.interrupt.portDescFullInterruptMask2)},
        /*myFatherInfo*/{
            /*myBitIndex*/4 ,
            /*interruptPtr*/&TXQInterruptSummary
            },
        /*isTriggeredByWrite*/ 1
    }
};

static SKERNEL_INTERRUPT_REG_INFO_STC pcl_unit_interrupts[] =
{
    {
            /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(PCL.PCLUnitInterruptCause)},
            /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(PCL.PCLUnitInterruptMask)},
            /*myFatherInfo*/{
                /*myBitIndex*/2 ,
                /*interruptPtr*/&FuncUnitsInterruptsSummary
                },
            /*isTriggeredByWrite*/ 1
    }
};

static SKERNEL_INTERRUPT_REG_INFO_STC tti_engine_interrupts[] =
{
    {
            /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(TTI.TTIEngine.TTIEngineInterruptCause)},
            /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(TTI.TTIEngine.TTIEngineInterruptMask)},
            /*myFatherInfo*/{
                /*myBitIndex*/1 ,
                /*interruptPtr*/&FuncUnitsInterruptsSummary
                },
            /*isTriggeredByWrite*/ 1
     }
};

static SKERNEL_INTERRUPT_REG_INFO_STC eq_critical_ecc_cntrs_interrupts[] =
{
    {
            /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(EQ.criticalECCCntrs.BMCriticalECCInterruptCause)},
            /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(EQ.criticalECCCntrs.BMCriticalECCInterruptMask)},
            /*myFatherInfo*/{
                /*myBitIndex*/3 ,
                /*interruptPtr*/&preEgrInterruptSummary
                },
            /*isTriggeredByWrite*/ 1
     }
};

static SKERNEL_INTERRUPT_REG_INFO_STC tcam_interrupts[] =
{
    {
            /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(TCAM.tcamInterrupts.TCAMInterruptCause)},
            /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(TCAM.tcamInterrupts.TCAMInterruptMask)},
            /*myFatherInfo*/{
                /*myBitIndex*/1 ,
                /*interruptPtr*/&TcamInterruptsSummary
                },
            /*isTriggeredByWrite*/ 1
    }
};

static SKERNEL_INTERRUPT_REG_INFO_STC mll_global_ctrl_interrupts[] =
{
    {
            /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(MLL.MLLGlobalCtrl.MLLInterruptCauseReg)},
            /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(MLL.MLLGlobalCtrl.MLLInterruptMaskReg)},
            /*myFatherInfo*/{
                /*myBitIndex*/FuncUnitsInterrupts_MLLInterruptCauseReg ,
                /*interruptPtr*/&FuncUnitsInterruptsSummary
                },
            /*isTriggeredByWrite*/ 1
     }
};

static SKERNEL_INTERRUPT_REG_INFO_STC egf_eft_global_interrupts[] =
{
    {
            /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(EGF_eft.global.EFTInterruptsCause)},
            /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(EGF_eft.global.EFTInterruptsMask)},
            /*myFatherInfo*/{
                /*myBitIndex*/22 ,
                /*interruptPtr*/&FuncUnitsInterruptsSummary
                },
            /*isTriggeredByWrite*/ 1
    }
};

static SKERNEL_INTERRUPT_REG_INFO_STC txdma_interrupts[] =
{
    {
            /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(TXDMA[0].interrupts.txDMAInterruptCause)},
            /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(TXDMA[0].interrupts.txDMAInterruptMask)},
            /*myFatherInfo*/{
                /*myBitIndex*/7 ,
                /*interruptPtr*/&TxDmaInterruptsSummary
                },
            /*isTriggeredByWrite*/ 1
    }
};

static SKERNEL_INTERRUPT_REG_INFO_STC rxdma_interrupts[] =
{
    {
            /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(rxDMA[0].interrupts.rxDMAInterrupt0Cause)},
            /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(rxDMA[0].interrupts.rxDMAInterrupt0Cause)},
            /*myFatherInfo*/{
                /*myBitIndex*/8 ,
                /*interruptPtr*/&RxDmaInterruptsSummary
                },
            /*isTriggeredByWrite*/ 1
    }
};

static SKERNEL_INTERRUPT_REG_INFO_STC txfifo_interrupts[] =
{
    {
            /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(TXFIFO[0].interrupts.txFIFOInterruptCause)},
            /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(TXFIFO[0].interrupts.txFIFOInterruptMask)},
            /*myFatherInfo*/{
                /*myBitIndex*/1 ,
                /*interruptPtr*/&TxFifoInterruptsSummary
                },
            /*isTriggeredByWrite*/ 1
    }
};

static SKERNEL_INTERRUPT_REG_INFO_STC eth_txfifo_interrupts[] =
{
    {
            /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(ETH.TXFIFO.interrupts.EthTxFIFOInterruptCause)},
            /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(ETH.TXFIFO.interrupts.EthTxFIFOInterruptMask)},
            /*myFatherInfo*/{
                /*myBitIndex*/1 ,
                /*interruptPtr*/&EthTxFifoInterruptsSummary
                },
            /*isTriggeredByWrite*/ 1
    }
};

static SKERNEL_INTERRUPT_REG_INFO_STC mppm_ecc_interrupts[] =
{
    {
            /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(MPPM.ECC.interrupts.EccErrorInterruptCause)},
            /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(MPPM.ECC.interrupts.EccErrorInterruptMask)},
            /*myFatherInfo*/{
                /*myBitIndex*/27 ,
                /*interruptPtr*/&MppmInterruptsSummary
                },
            /*isTriggeredByWrite*/ 1
    }
};

static SKERNEL_INTERRUPT_REG_INFO_STC cpu_code_rate_limiters_interrupts[] =
{
     CPU_CODE_RATE_LIMITERS_INTERRUPT_CAUSE_MAC(1)
    ,CPU_CODE_RATE_LIMITERS_INTERRUPT_CAUSE_MAC(2)
    ,CPU_CODE_RATE_LIMITERS_INTERRUPT_CAUSE_MAC(3)
    ,CPU_CODE_RATE_LIMITERS_INTERRUPT_CAUSE_MAC(4)
    ,CPU_CODE_RATE_LIMITERS_INTERRUPT_CAUSE_MAC(5)
    ,CPU_CODE_RATE_LIMITERS_INTERRUPT_CAUSE_MAC(6)
    ,CPU_CODE_RATE_LIMITERS_INTERRUPT_CAUSE_MAC(7)
    ,CPU_CODE_RATE_LIMITERS_INTERRUPT_CAUSE_MAC(8)
    ,CPU_CODE_RATE_LIMITERS_INTERRUPT_CAUSE_MAC(9)
    ,CPU_CODE_RATE_LIMITERS_INTERRUPT_CAUSE_MAC(10)
    ,CPU_CODE_RATE_LIMITERS_INTERRUPT_CAUSE_MAC(11)
    ,CPU_CODE_RATE_LIMITERS_INTERRUPT_CAUSE_MAC(12)
    ,CPU_CODE_RATE_LIMITERS_INTERRUPT_CAUSE_MAC(13)
    ,CPU_CODE_RATE_LIMITERS_INTERRUPT_CAUSE_MAC(14)
    ,CPU_CODE_RATE_LIMITERS_INTERRUPT_CAUSE_MAC(15)
    ,CPU_CODE_RATE_LIMITERS_INTERRUPT_CAUSE_MAC(16)
};

static SKERNEL_INTERRUPT_REG_INFO_STC ingr_stc_ports_interrupts[] =
{
    /* ingress stc - support 256 physical ports of EQ */
     INGR_STC_INTERRUPT_CAUSE_FOR_16_PORTS_MAC(0)
    ,INGR_STC_INTERRUPT_CAUSE_FOR_16_PORTS_MAC(1)
    ,INGR_STC_INTERRUPT_CAUSE_FOR_16_PORTS_MAC(2)
    ,INGR_STC_INTERRUPT_CAUSE_FOR_16_PORTS_MAC(3)
    ,INGR_STC_INTERRUPT_CAUSE_FOR_16_PORTS_MAC(4)
    ,INGR_STC_INTERRUPT_CAUSE_FOR_16_PORTS_MAC(5)
    ,INGR_STC_INTERRUPT_CAUSE_FOR_16_PORTS_MAC(6)
    ,INGR_STC_INTERRUPT_CAUSE_FOR_16_PORTS_MAC(7)
    ,INGR_STC_INTERRUPT_CAUSE_FOR_16_PORTS_MAC(8)
    ,INGR_STC_INTERRUPT_CAUSE_FOR_16_PORTS_MAC(9)
    ,INGR_STC_INTERRUPT_CAUSE_FOR_16_PORTS_MAC(10)
    ,INGR_STC_INTERRUPT_CAUSE_FOR_16_PORTS_MAC(11)
    ,INGR_STC_INTERRUPT_CAUSE_FOR_16_PORTS_MAC(12)
    ,INGR_STC_INTERRUPT_CAUSE_FOR_16_PORTS_MAC(13)
    ,INGR_STC_INTERRUPT_CAUSE_FOR_16_PORTS_MAC(14)
    ,INGR_STC_INTERRUPT_CAUSE_FOR_16_PORTS_MAC(15)
};

static SKERNEL_INTERRUPT_REG_INFO_STC egr_stc_ports_interrupts[] =
{
    /* egress stc - support 72 ports of DQ */
     EGR_STC_INTERRUPT_CAUSE_PORTS_MAC(0)
    ,EGR_STC_INTERRUPT_CAUSE_PORTS_MAC(1)
    ,EGR_STC_INTERRUPT_CAUSE_PORTS_MAC(2)
    ,EGR_STC_INTERRUPT_CAUSE_PORTS_MAC(3)
};

static SKERNEL_INTERRUPT_REG_INFO_STC giga_mac_ports_interrupts[] =
{
    /* the Giga ports 0..71 */
     GIGA_MAC_INTERRUPT_CAUSE_MAC(0 )
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(1 )
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(2 )
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(3 )
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(4 )
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(5 )
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(6 )
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(7 )
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(8 )
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(9 )
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(10)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(11)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(12)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(13)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(14)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(15)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(16)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(17)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(18)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(19)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(20)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(21)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(22)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(23)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(24)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(25)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(26)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(27)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(28)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(29)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(30)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(31)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(32)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(33)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(34)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(35)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(36)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(37)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(38)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(39)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(40)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(41)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(42)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(43)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(44)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(45)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(46)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(47)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(48)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(49)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(50)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(51)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(52)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(53)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(54)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(55)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(56)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(57)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(58)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(59)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(60)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(61)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(62)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(63)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(64)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(65)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(66)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(67)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(68)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(69)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(70)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(71)
};

static SKERNEL_INTERRUPT_REG_INFO_STC xlg_mac_ports_interrupts[] =
{
    /* the XLG ports 48..71 */
     XLG_MAC_INTERRUPT_CAUSE_MAC (48)
    ,XLG_MAC_INTERRUPT_CAUSE_MAC (49)
    ,XLG_MAC_INTERRUPT_CAUSE_MAC (50)
    ,XLG_MAC_INTERRUPT_CAUSE_MAC (51)
    ,XLG_MAC_INTERRUPT_CAUSE_MAC (52)
    ,XLG_MAC_INTERRUPT_CAUSE_MAC (53)
    ,XLG_MAC_INTERRUPT_CAUSE_MAC (54)
    ,XLG_MAC_INTERRUPT_CAUSE_MAC (55)
    ,XLG_MAC_INTERRUPT_CAUSE_MAC (56)
    ,XLG_MAC_INTERRUPT_CAUSE_MAC (57)
    ,XLG_MAC_INTERRUPT_CAUSE_MAC (58)
    ,XLG_MAC_INTERRUPT_CAUSE_MAC (59)
    ,XLG_MAC_INTERRUPT_CAUSE_MAC (60)
    ,XLG_MAC_INTERRUPT_CAUSE_MAC (61)
    ,XLG_MAC_INTERRUPT_CAUSE_MAC (62)
    ,XLG_MAC_INTERRUPT_CAUSE_MAC (63)
    ,XLG_MAC_INTERRUPT_CAUSE_MAC (64)
    ,XLG_MAC_INTERRUPT_CAUSE_MAC (65)
    ,XLG_MAC_INTERRUPT_CAUSE_MAC (66)
    ,XLG_MAC_INTERRUPT_CAUSE_MAC (67)
    ,XLG_MAC_INTERRUPT_CAUSE_MAC (68)
    ,XLG_MAC_INTERRUPT_CAUSE_MAC (69)
    ,XLG_MAC_INTERRUPT_CAUSE_MAC (70)
    ,XLG_MAC_INTERRUPT_CAUSE_MAC (71)
};

/* SIP 5.15 interrupts */
static SKERNEL_INTERRUPT_REG_INFO_STC caelum_txdma1_interrupts[] =
{
    {
            /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(TXDMA[1].interrupts.txDMAInterruptCause)},
            /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(TXDMA[1].interrupts.txDMAInterruptMask)},
            /*myFatherInfo*/{
                /*myBitIndex*/7 ,
                /*interruptPtr*/&TxDma1InterruptsSummary
                },
            /*isTriggeredByWrite*/ 1
    }
};

static SKERNEL_INTERRUPT_REG_INFO_STC caelum_rxdma1_interrupts[] =
{
    {
            /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(rxDMA[0].interrupts.rxDMAInterrupt0Cause)},
            /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(rxDMA[0].interrupts.rxDMAInterrupt0Cause)},
            /*myFatherInfo*/{
                /*myBitIndex*/1 ,
                /*interruptPtr*/&RxDma1InterruptsSummary
                },
            /*isTriggeredByWrite*/ 1
    }
};

static SKERNEL_INTERRUPT_REG_INFO_STC caelum_txfifo1_interrupts[] =
{
    {
            /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(TXFIFO[1].interrupts.txFIFOInterruptCause)},
            /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_SIP5_MAC(TXFIFO[1].interrupts.txFIFOInterruptMask)},
            /*myFatherInfo*/{
                /*myBitIndex*/1 ,
                /*interruptPtr*/&TxFifo1InterruptsSummary
                },
            /*isTriggeredByWrite*/ 1
    }
};

static SKERNEL_INTERRUPT_REG_INFO_STC caelum_egr_stc_ports_interrupts[] =
{
    /* egress stc - support 72 ports of DQ */
     EGR_STC_INTERRUPT_CAUSE_PORTS_MAC(0)
    ,EGR_STC_INTERRUPT_CAUSE_PORTS_MAC(1)
    ,EGR_STC_INTERRUPT_CAUSE_PORTS_MAC(2)
};

static SKERNEL_INTERRUPT_REG_INFO_STC caelum_giga_mac_ports_interrupts[] =
{
     GIGA_MAC_INTERRUPT_CAUSE_MAC(0 )
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(1 )
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(2 )
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(3 )
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(4 )
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(5 )
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(6 )
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(7 )
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(8 )
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(9 )
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(10)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(11)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(12)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(13)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(14)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(15)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(16)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(17)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(18)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(19)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(20)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(21)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(22)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(23)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(24)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(25)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(26)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(27)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(28)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(29)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(30)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(31)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(32)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(33)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(34)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(35)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(36)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(37)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(38)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(39)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(40)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(41)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(42)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(43)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(44)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(45)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(46)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(47)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(56)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(57)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(58)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(59)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(64)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(65)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(66)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(67)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(68)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(69)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(70)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(71)
};

static SKERNEL_INTERRUPT_REG_INFO_STC caelum_xlg_mac_ports_interrupts[] =
{
     XLG_MAC_INTERRUPT_CAUSE_MAC (56)
    ,XLG_MAC_INTERRUPT_CAUSE_MAC (57)
    ,XLG_MAC_INTERRUPT_CAUSE_MAC (58)
    ,XLG_MAC_INTERRUPT_CAUSE_MAC (59)
    ,XLG_MAC_INTERRUPT_CAUSE_MAC (64)
    ,XLG_MAC_INTERRUPT_CAUSE_MAC (65)
    ,XLG_MAC_INTERRUPT_CAUSE_MAC (66)
    ,XLG_MAC_INTERRUPT_CAUSE_MAC (67)
    ,XLG_MAC_INTERRUPT_CAUSE_MAC (68)
    ,XLG_MAC_INTERRUPT_CAUSE_MAC (69)
    ,XLG_MAC_INTERRUPT_CAUSE_MAC (70)
    ,XLG_MAC_INTERRUPT_CAUSE_MAC (71)
};

static SKERNEL_INTERRUPT_REG_INFO_STC cetus_giga_mac_ports_interrupts[] =
{
     GIGA_MAC_INTERRUPT_CAUSE_MAC(56)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(57)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(58)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(59)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(64)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(65)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(66)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(67)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(68)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(69)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(70)
    ,GIGA_MAC_INTERRUPT_CAUSE_MAC(71)
};

static SKERNEL_INTERRUPT_REG_INFO_STC skernel_interrupt_reg_info__last_line__cns[] =
{{{SKERNEL_REGISTERS_DB_TYPE__LAST___E}}};


/* Bobcat2 interrupt tree database */
static SKERNEL_INTERRUPT_REG_INFO_STC bobcat2InterruptsTreeDb[BOBCAT2_MAX_INTERRUPT_NODES];

static GT_VOID smemBobcat2InterruptTreeAddNode
(
    INOUT SKERNEL_INTERRUPT_REG_INFO_STC ** currDbPtrPtr,
    IN    SKERNEL_INTERRUPT_REG_INFO_STC * currNodePtr,
    IN    GT_U32 nodeCount
)
{
    GT_U32 index;
    SKERNEL_INTERRUPT_REG_INFO_STC * currDbPtr = *currDbPtrPtr;
    for (index = 0; index < nodeCount; index++)
    {
        *(currDbPtr++) = *(currNodePtr++);
    }
    *currDbPtrPtr = currDbPtr;
}

/* Macro increment index before nodes insertion and checks index range for current interrupt */
#define INTERRUPT_TREE_ADD_NODE_WITH_CHECK(index, dbPtrPtr, nodeArr) \
    { \
        GT_U32 nodesNum = sizeof(nodeArr)/sizeof(SKERNEL_INTERRUPT_REG_INFO_STC); \
        index += nodesNum; \
        if (index >= BOBCAT2_MAX_INTERRUPT_NODES) \
            skernelFatalError("Interrupt index node is out of range: index[%d]", index); \
        smemBobcat2InterruptTreeAddNode(dbPtrPtr, nodeArr, nodesNum); \
    }

GT_VOID smemBobcat2InterruptTreeInit
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_INTERRUPT_REG_INFO_STC ** currDbPtrPtr
)
{
    GT_U32 index = 0;
    SKERNEL_INTERRUPT_REG_INFO_STC * currDbPtr = bobcat2InterruptsTreeDb;

    INTERRUPT_TREE_ADD_NODE_WITH_CHECK(index, &currDbPtr, mg_sdma_interrupts                );
    INTERRUPT_TREE_ADD_NODE_WITH_CHECK(index, &currDbPtr, tti_interrupts                    );
    INTERRUPT_TREE_ADD_NODE_WITH_CHECK(index, &currDbPtr, bridge_interrupts                 );
    INTERRUPT_TREE_ADD_NODE_WITH_CHECK(index, &currDbPtr, mac_tbl_intrrupts                 );
    INTERRUPT_TREE_ADD_NODE_WITH_CHECK(index, &currDbPtr, ha_interrupts                     );
    INTERRUPT_TREE_ADD_NODE_WITH_CHECK(index, &currDbPtr, ermrk_interrupts                  );
    INTERRUPT_TREE_ADD_NODE_WITH_CHECK(index, &currDbPtr, oam_interrupts                    );
    INTERRUPT_TREE_ADD_NODE_WITH_CHECK(index, &currDbPtr, mll_interrupts                    );
    INTERRUPT_TREE_ADD_NODE_WITH_CHECK(index, &currDbPtr, cnc_interrupts                    );
    INTERRUPT_TREE_ADD_NODE_WITH_CHECK(index, &currDbPtr, policer_ipfix_interrupts          );
    INTERRUPT_TREE_ADD_NODE_WITH_CHECK(index, &currDbPtr, mg_global_misc_interrupts         );
    INTERRUPT_TREE_ADD_NODE_WITH_CHECK(index, &currDbPtr, bm_general_cause_interrupts       );
    INTERRUPT_TREE_ADD_NODE_WITH_CHECK(index, &currDbPtr, txq_queue_global_interrupts       );
    INTERRUPT_TREE_ADD_NODE_WITH_CHECK(index, &currDbPtr, pcl_unit_interrupts               );
    INTERRUPT_TREE_ADD_NODE_WITH_CHECK(index, &currDbPtr, tti_engine_interrupts             );
    INTERRUPT_TREE_ADD_NODE_WITH_CHECK(index, &currDbPtr, eq_critical_ecc_cntrs_interrupts  );
    INTERRUPT_TREE_ADD_NODE_WITH_CHECK(index, &currDbPtr, tcam_interrupts                   );
    INTERRUPT_TREE_ADD_NODE_WITH_CHECK(index, &currDbPtr, mll_global_ctrl_interrupts        );
    INTERRUPT_TREE_ADD_NODE_WITH_CHECK(index, &currDbPtr, egf_eft_global_interrupts         );
    INTERRUPT_TREE_ADD_NODE_WITH_CHECK(index, &currDbPtr, eth_txfifo_interrupts             );
    INTERRUPT_TREE_ADD_NODE_WITH_CHECK(index, &currDbPtr, mppm_ecc_interrupts               );
    INTERRUPT_TREE_ADD_NODE_WITH_CHECK(index, &currDbPtr, cpu_code_rate_limiters_interrupts );
    INTERRUPT_TREE_ADD_NODE_WITH_CHECK(index, &currDbPtr, ingr_stc_ports_interrupts         );

    switch (devObjPtr->deviceType)
    {
        case SKERNEL_BOBCAT2:
        case SKERNEL_BOBCAT3:
            INTERRUPT_TREE_ADD_NODE_WITH_CHECK(index, &currDbPtr, rxdma_interrupts                  );
            INTERRUPT_TREE_ADD_NODE_WITH_CHECK(index, &currDbPtr, txdma_interrupts                  );
            INTERRUPT_TREE_ADD_NODE_WITH_CHECK(index, &currDbPtr, txfifo_interrupts                 );
            INTERRUPT_TREE_ADD_NODE_WITH_CHECK(index, &currDbPtr, giga_mac_ports_interrupts         );
            INTERRUPT_TREE_ADD_NODE_WITH_CHECK(index, &currDbPtr, xlg_mac_ports_interrupts          );
            INTERRUPT_TREE_ADD_NODE_WITH_CHECK(index, &currDbPtr, egr_stc_ports_interrupts          );
            break;
        case SKERNEL_BOBK_CAELUM:
            INTERRUPT_TREE_ADD_NODE_WITH_CHECK(index, &currDbPtr, rxdma_interrupts                  );
            INTERRUPT_TREE_ADD_NODE_WITH_CHECK(index, &currDbPtr, txdma_interrupts                  );
            INTERRUPT_TREE_ADD_NODE_WITH_CHECK(index, &currDbPtr, txfifo_interrupts                 );
            INTERRUPT_TREE_ADD_NODE_WITH_CHECK(index, &currDbPtr, caelum_rxdma1_interrupts          );
            INTERRUPT_TREE_ADD_NODE_WITH_CHECK(index, &currDbPtr, caelum_txdma1_interrupts          );
            INTERRUPT_TREE_ADD_NODE_WITH_CHECK(index, &currDbPtr, caelum_txfifo1_interrupts         );
            INTERRUPT_TREE_ADD_NODE_WITH_CHECK(index, &currDbPtr, caelum_egr_stc_ports_interrupts   );
            INTERRUPT_TREE_ADD_NODE_WITH_CHECK(index, &currDbPtr, caelum_giga_mac_ports_interrupts  );
            INTERRUPT_TREE_ADD_NODE_WITH_CHECK(index, &currDbPtr, caelum_xlg_mac_ports_interrupts   );
            break;
        case SKERNEL_BOBK_CETUS:
            INTERRUPT_TREE_ADD_NODE_WITH_CHECK(index, &currDbPtr, caelum_rxdma1_interrupts          );
            INTERRUPT_TREE_ADD_NODE_WITH_CHECK(index, &currDbPtr, caelum_txdma1_interrupts          );
            INTERRUPT_TREE_ADD_NODE_WITH_CHECK(index, &currDbPtr, caelum_txfifo1_interrupts         );
            INTERRUPT_TREE_ADD_NODE_WITH_CHECK(index, &currDbPtr, caelum_egr_stc_ports_interrupts   );
            INTERRUPT_TREE_ADD_NODE_WITH_CHECK(index, &currDbPtr, cetus_giga_mac_ports_interrupts   );
            INTERRUPT_TREE_ADD_NODE_WITH_CHECK(index, &currDbPtr, caelum_xlg_mac_ports_interrupts   );
            break;
        default:
            break;
    }
    /* End of interrupts - must be last */
    INTERRUPT_TREE_ADD_NODE_WITH_CHECK(index, &currDbPtr, skernel_interrupt_reg_info__last_line__cns);


    *currDbPtrPtr = bobcat2InterruptsTreeDb;
}

/*******************************************************************************
*   smemBobcat2InitInterrupts
*
* DESCRIPTION:
*       Init interrupts for a device.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemBobcat2InitInterrupts
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    devObjPtr->globalInterruptCauseRegister.pexInt     = pexIntSum;
    devObjPtr->globalInterruptCauseRegister.pexErr     = pexErrSum;
    devObjPtr->globalInterruptCauseRegister.cnc[0]     = FuncUnitsIntsSum;
    devObjPtr->globalInterruptCauseRegister.cnc[1]     = FuncUnitsIntsSum;
    devObjPtr->globalInterruptCauseRegister.txqDq      = FuncUnitsIntsSum;
    devObjPtr->globalInterruptCauseRegister.txqSht     = FuncUnitsIntsSum;
    devObjPtr->globalInterruptCauseRegister.tti        = FuncUnitsIntsSum;
    devObjPtr->globalInterruptCauseRegister.tccLower   = SMAIN_NOT_VALID_CNS;
    devObjPtr->globalInterruptCauseRegister.tccUpper   = SMAIN_NOT_VALID_CNS;
    devObjPtr->globalInterruptCauseRegister.bcn        = SMAIN_NOT_VALID_CNS;
    devObjPtr->globalInterruptCauseRegister.ha         = FuncUnitsIntsSum;
    devObjPtr->globalInterruptCauseRegister.misc       = MgInternalIntSum;
    devObjPtr->globalInterruptCauseRegister.mem        = 12;
    devObjPtr->globalInterruptCauseRegister.txq        = SMAIN_NOT_VALID_CNS;
    devObjPtr->globalInterruptCauseRegister.txqll      = FuncUnitsIntsSum;
    devObjPtr->globalInterruptCauseRegister.txqQueue   = FuncUnitsIntsSum;
    devObjPtr->globalInterruptCauseRegister.l2i        = FuncUnitsIntsSum;
    devObjPtr->globalInterruptCauseRegister.bm0        = DataPathIntSum;
    devObjPtr->globalInterruptCauseRegister.bm1        = SMAIN_NOT_VALID_CNS;
    devObjPtr->globalInterruptCauseRegister.txqEgr     = SMAIN_NOT_VALID_CNS;
    devObjPtr->globalInterruptCauseRegister.xlgPort    = SMAIN_NOT_VALID_CNS;
    devObjPtr->globalInterruptCauseRegister.fdb        = FuncUnitsIntsSum;
    devObjPtr->globalInterruptCauseRegister.ports      = SMAIN_NOT_VALID_CNS;
    devObjPtr->globalInterruptCauseRegister.cpuPort    = SMAIN_NOT_VALID_CNS;
    devObjPtr->globalInterruptCauseRegister.global1    = SMAIN_NOT_VALID_CNS;
    devObjPtr->globalInterruptCauseRegister.txSdma     = TxSDMASum;
    devObjPtr->globalInterruptCauseRegister.rxSdma     = RxSDMASum;
    devObjPtr->globalInterruptCauseRegister.pcl        = FuncUnitsIntsSum;
    devObjPtr->globalInterruptCauseRegister.port24     = SMAIN_NOT_VALID_CNS;
    devObjPtr->globalInterruptCauseRegister.port25     = SMAIN_NOT_VALID_CNS;
    devObjPtr->globalInterruptCauseRegister.port26     = SMAIN_NOT_VALID_CNS;
    devObjPtr->globalInterruptCauseRegister.port27     = SMAIN_NOT_VALID_CNS;
    devObjPtr->globalInterruptCauseRegister.iplr0      = FuncUnitsIntsSum;
    devObjPtr->globalInterruptCauseRegister.iplr1      = FuncUnitsIntsSum;
    devObjPtr->globalInterruptCauseRegister.rxDma      = DataPathIntSum;
    devObjPtr->globalInterruptCauseRegister.txDma      = DataPathIntSum;
    devObjPtr->globalInterruptCauseRegister.eq         = FuncUnitsIntsSum;/*for ingress STC  */
    devObjPtr->globalInterruptCauseRegister.bma        = DataPathIntSum;
    devObjPtr->globalInterruptCauseRegister.eplr       = FuncUnitsIntsSum;

    devObjPtr->globalInterruptCauseRegister.supportMultiPorts = 1;
    /* ports 0..30 */
    devObjPtr->globalInterruptCauseRegister.multiPorts[0].globalBit = ports_int_sum;
    devObjPtr->globalInterruptCauseRegister.multiPorts[0].startPort = 0;
    devObjPtr->globalInterruptCauseRegister.multiPorts[0].endPort   = /* 'end' after 31 bits */
        devObjPtr->globalInterruptCauseRegister.multiPorts[0].startPort + 30;
    /* ports 31..55 */
    devObjPtr->globalInterruptCauseRegister.multiPorts[1].globalBit = ports_1_int_sum;
    devObjPtr->globalInterruptCauseRegister.multiPorts[1].startPort = /*start from end port of previous register*/
        devObjPtr->globalInterruptCauseRegister.multiPorts[0].endPort + 1;
    devObjPtr->globalInterruptCauseRegister.multiPorts[1].endPort   =
        devObjPtr->globalInterruptCauseRegister.multiPorts[1].startPort + 24;/* 'end' after 25 bits */
    /* ports 56..71 */
    devObjPtr->globalInterruptCauseRegister.multiPorts[2].globalBit = ports_2_int_sum;
    devObjPtr->globalInterruptCauseRegister.multiPorts[2].startPort = /*start from end port of previous register*/
        devObjPtr->globalInterruptCauseRegister.multiPorts[1].endPort + 1;
    devObjPtr->globalInterruptCauseRegister.multiPorts[2].endPort      =
        devObjPtr->globalInterruptCauseRegister.multiPorts[2].startPort + 15;/* 'end' after 16 bits */

    smemBobcat2InterruptTreeInit(devObjPtr, &devObjPtr->myInterruptsDbPtr);
}

/*******************************************************************************
*   smemBobcat2ErrataCleanUp
*
* DESCRIPTION:
*       Clean up erratum for Bobcat2 device.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
void smemBobcat2ErrataCleanUp
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    if(0 == SMEM_CHT_IS_SIP5_10_GET(devObjPtr))
    {
        return;
    }
    /* code for bc_B0 */
    devObjPtr->errata.ingressPipeTrapToCpuUseNotOrig802dot1Vid = 0; /* JIRA: HA-3259 */
    devObjPtr->errata.mplsNonLsrNotModifyExp = 0; /* JIRA: EQ-437  */
    devObjPtr->errata.eqNotAccessE2phyForSniffedOnDropDuplication = 0 /* JIRA: EQ-500 */ ;
    devObjPtr->errata.noTrafficToCpuOnBridgeBypassForwardingDecisionOnly = 0; /* JIRA: L2I-482 */
    devObjPtr->errata.l2iApplyMoveStaticCommandRegrdlessToSecurityBreach = 0; /* JIRA: L2I-485 */
    devObjPtr->errata.incorrectResolutionOfTtiActionFields = 0; /* JIRA: TTI-1114 */
    devObjPtr->errata.dsaTagOrigSrcPortNotUpdated = 0;  /* JIRA: TTI-1133 */
    devObjPtr->errata.l2echoValidityForCfmInTti = 0; /* JIRA: TTI-1182 */

    devObjPtr->errata.ieeeReservedMcConfigRegRead = 0; /* JIRA: L2I-9 */
    devObjPtr->errata.llcNonSnapWithDoRouterHaCurruptL2Header = 0; /* JIRA: HA-3365 */
    devObjPtr->errata.fdbAgingDaemonVidxEntries = 0; /* JIRA: L2I-452 */
    devObjPtr->errata.cncHaIgnoreCrc4BytesInL2ByteCountMode = 0; /* JIRA: HA-3338 */
    devObjPtr->errata.ttiActionNotSetModifyUpDscpWhenQosPrecedenceHard = 0; /* JIRA: TTI-1265 */
    devObjPtr->errata.l2iFromCpuSubjectToL2iFiltersBridgeBypassModeIsBypassForwardingDecisionOnly = 0; /* JIRA: L2I-514 */
    devObjPtr->errata.haBuildWrongTagsWhenNumOfTagsToPopIs2 = 0; /* JIRA: HA-3374 */
    devObjPtr->errata.iplrPolicerBillingForPerPhysicalPortMeteringDoesNotWorksInFlowMode = 0; /* JIRA: POLICER-1322 */
    devObjPtr->errata.eplrNotCountCrc = 0;/* JIRA : POLICER-1304 */
    devObjPtr->errata.epclPortListModeForcePortMode = 0;/*[JIRA]  [EPCL-443] */

    devObjPtr->errata.ttiWrongPparsingWhenETagSecondWordEqualToEthertype = 0;/* [JIRA] [TTI-1364] */
    devObjPtr->errata.ttiTtAndPopTagNeedToBeSupportedConcurrently = 0;/*[JIRA] : [TTI-1077]*/
    devObjPtr->errata.tunnelStartQosRemarkUsePassenger = 0; /* [JIRA]:[HA-3279] */
    devObjPtr->errata.fdbNaMsgVid1Assign = 0; /* [JIRA]:[L2I-497] */

    if(0 == SMEM_CHT_IS_SIP5_15_GET(devObjPtr))
    {
        return;
    }

    /* code for bobk */
    devObjPtr->errata.txqEgressMibCountersNotClearOnRead = 0;/*[JIRA] (TXQ-1147) */

}

/*******************************************************************************
*   smemBobcat2DoPortInterrupt
*
* DESCRIPTION:
*       Set port interrupt
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       macPort             - the mac port number.
*       intIndex            - bit index in Interrupt Cause Register
*
*******************************************************************************/
void smemBobcat2DoPortInterrupt(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32                   macPort,
    IN GT_U32                   intIndex
)
{
    snetChetahDoInterrupt(devObjPtr,
                          SMEM_CHT_PORT_INT_CAUSE_REG(devObjPtr,macPort),
                          SMEM_CHT_PORT_INT_CAUSE_MASK_REG(devObjPtr,macPort),
                          (1 << intIndex),
                          SMEM_CHT_PORT_CAUSE_INT_SUM_BMP(devObjPtr,macPort));
}

/*******************************************************************************
*  smemBobcat2ActiveReadPortLpiStatusReg
*
* DESCRIPTION:
*      Read the port LPI status register.
* INPUTS:
*       devObjPtr   - device object PTR.
*       address     - address for ASIC memory.
*       memSize     - memory size to be read.
*       memPtr      - pointer to the register's memory in the simulation.
*       sumBit      - global summary interrupt bit
*
* OUTPUTS:
*       outMemPtr   - Pointer to the memory to copy register's content.
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void smemBobcat2ActiveReadPortLpiStatusReg
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR sumBit,
    OUT        GT_U32 * outMemPtr
)
{
    GT_U32 macPort;
    GT_U32  value,fldValue;

/*
4 MAC Tx path LP idle status RO 0x0 Indicates that the MAC is sending LPI to the Tx PHY.
3 MAC Tx path LP wait status RO 0x0 Indicates that the MAC is sending IDLE to the Tx PHY and waiting for Tx timer to end.
2 MAC Rx path LP idle status RO 0x0 Indicates that the PHY has detected the assertion / de-assertion of LP_IDLE from link partner (through the PCS).
1 PCS Tx path LPI status RO 0x0 PCS Tx path LPI status 0 = Does not receive: Tx PCS does not receive LP_IDLE. 1 = Receives: Tx PCS receives LP_IDLE.
0 PCS Rx path LPI status RO 0x0 PCS Rx path LPI status 0 = Does not receive: Rx PCS does not receive LP_IDLE 1 = Receives: Rx PCS receives LP_IDLE.
*/
    /* if EEE enabled and link UP  ---> TX indications (mac,pcs) */

    /* if EEE enabled and link UP and mac loopback --> RX indications (mac,pcs) */

    macPort = CHT_FROM_ADDRESS_GET_PORT_CNS(devObjPtr,address);

    if(IS_CHT_HYPER_GIGA_PORT(devObjPtr, macPort))
    {
        value = 0; /*all bits 0 */
    }
    else
    {
        /*<LinkUp>*/
        smemRegFldGet(devObjPtr,
            SMEM_CHT_PORT_STATUS0_REG(devObjPtr, macPort),
            0, 1, &fldValue);
        if (fldValue != 1)
        {
            /* link is down */
            value = 0;
        }
        else
        {
            /* check if the EEE enabled on the port */
            /*<LPI request enable>*/
            smemRegFldGet(devObjPtr,
                SMEM_SIP_5_MAC_LPI_CTRL_REG(devObjPtr,macPort,1),
                0,1,&fldValue);

            if (fldValue == 0)
            {
                value = 0; /*all bits 0 */
            }
            else
            {
                value = 1<<4 /*MAC Tx path LP idle*/    |
                        1<<1 /*PCS Tx path LPI status*/ ;

                /*portInLoopBack*/
                smemRegFldGet(devObjPtr,
                    SMEM_CHT_MAC_CONTROL_REG(devObjPtr, macPort),
                    5, 2, &fldValue);
                if (fldValue)
                {
                    value |= 1<<2 /*MAC Rx path LP idle*/    |
                             1<<0 /*PCS Rx path LPI status*/ ;
                }
                else
                {
                    /* we not now if the PHY will send up LPI indication */
                }
            }
        }
    }

    /* return the caller with the 'up to date' value */
    *outMemPtr = value;

}
/*******************************************************************************
*  smemBobcat2ActiveWritePortLpiCtrlReg
*
* DESCRIPTION:
*      Write to port LPI control register
*
* INPUTS:
*       devObjPtr - device object PTR.
*       address     - Address for ASIC memory.
*       memSize     - size of the requested memory
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers' specific parameter. - the control register index.
*       inMemPtr    - Pointer to the memory to get register's content.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void smemBobcat2ActiveWritePortLpiCtrlReg (
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
)
{
    GT_U32  oldValue = *memPtr;
    GT_U32  newVlaue = *inMemPtr;
    GT_U32  macPort;

    *memPtr = *inMemPtr;

    if (param != 1)
    {
        /* implemented active memory for '1' only */
        return;
    }

    /*<LPI request enable>*/
    if ((oldValue & 1) == (newVlaue & 1))
    {
        /* no change from previous state */
        return;
    }
    else if ((newVlaue & 1) == 0)
    {
        /* new state not raise interrupt */
        return;
    }

     macPort = CHT_FROM_ADDRESS_GET_PORT_CNS(devObjPtr,address);
   /* check for EEE interrupt support */
    snetBobcat2EeeCheckInterrupts(devObjPtr,macPort);
}

/*******************************************************************************
*  smemBobcat2ActiveReadTodTimeCaptureValueFracLow
*
* DESCRIPTION:
*      Read the 'last register' of the TOD capture (set 0/ set 1),
*      to invoke the clear of the status of this TOD capture.
* INPUTS:
*       devObjPtr   - device object PTR.
*       address     - address for ASIC memory.
*       memSize     - memory size to be read.
*       memPtr      - pointer to the register's memory in the simulation.
*       sumBit      - global summary interrupt bit
*
* OUTPUTS:
*       outMemPtr   - Pointer to the memory to copy register's content.
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void smemBobcat2ActiveReadTodTimeCaptureValueFracLow
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR param,
    OUT        GT_U32 * outMemPtr
)
{
    GT_U32  index = (GT_U32)param;/*set 0 or set 1 */
    GT_U32  diffToRegister =
        SMEM_LION3_GOP_TAI_CAPTURE_STATUS_REG(devObjPtr,0,0) -
        SMEM_LION3_GOP_TAI_CAPTURE_VALUE_FRAC_LOW_REG(devObjPtr,0,0,index);
    GT_U32  *regPtr;/* pointer to memory of address of SMEM_LION3_GOP_TAI_CAPTURE_STATUS_REG() in current TAI unit */

    /* return the caller with the needed value */
    *outMemPtr = * memPtr;

    /* set valid bit as 'not valid' */
    regPtr = smemMemGet(devObjPtr, (address + diffToRegister));

    SMEM_U32_SET_FIELD((*regPtr), index, 1, 0);
}






