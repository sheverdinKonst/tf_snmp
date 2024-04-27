/******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* smemLion3.c
*
* DESCRIPTION:
*       Lion3 memory mapping implementation: the 'port group' and the shared memory
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 130 $
*
*******************************************************************************/
#include <asicSimulation/SInit/sinit.h>
#include <asicSimulation/SKernel/smem/smemLion3.h>
#include <asicSimulation/SKernel/smem/smemBobcat2.h>
#include <asicSimulation/SKernel/cheetahCommon/sregCheetah.h>
#include <asicSimulation/SKernel/cheetahCommon/sregLion2.h>
#include <asicSimulation/SKernel/suserframes/snetCheetah3Pcl.h>
#include <asicSimulation/SKernel/suserframes/snetLion.h>
#include <asicSimulation/SKernel/suserframes/snetLion2Oam.h>
#include <asicSimulation/SKernel/suserframes/snetLion3Tcam.h>
#include <common/Utils/Math/sMath.h>
#include <asicSimulation/SKernel/sfdb/sfdbCheetah.h>
#include <asicSimulation/SKernel/suserframes/snetLion3Tcam.h>
#include <asicSimulation/SLog/simLog.h>

#define _1K         1024

#define LION3_NUM_PHY_PORTS_BITS_CNS  (8)
#define LION3_NUM_E_PORTS_BITS_CNS   (13)
#define LION3_NUM_E_VLANS_BITS_CNS   (13)
#define LION3_NUM_E_VIDXS_BITS_CNS   (14)
#define LION3_NUM_TRUNKS_BITS_CNS    (12)
#define LION3_NUM_DEVICES_BITS_CNS   (10)
#define LION3_NUM_STG_BITS_CNS       (12)
#define LION3_NUM_TS_PTR_BITS_CNS    (15)/*32K*/
#define LION3_NUM_FID_BITS_CNS       (12)
#define LION3_NUM_SRC_ID_BITS_CNS    (12)

#define LION3_NUM_DEFAULT_E_PORTS_CNS   (512)
#define LION3_NUM_STGS_CNS      (4*_1K)

#define MAX_INIT_WORDS_CNS      32

/* number of lines in the TTI-TRILL TCAM*/
#define LION3_NUM_TTI_TRILL_ADJACENCY_TCAM_LINES_CNS      (512)
/* number of bytes in the TTI-TRILL TCAM entry*/
#define LION3_NUM_BYTES_TTI_TRILL_ADJACENCY_TCAM_ENTRY_CNS    (0x10)


/* the port group# that used by CPU to access the shared memory -->
   the PEX address is the same one  */
#define PORT_GROUP_ID_FOR_PORT_GROUP_SHARED_ACCESS_CNS  0

#define LION2_IP_ROUT_TCAM_CNS   16*1024

/* do not INIT memory for core that is not the single instance */
#define SINGLE_INSTANCE_UNIT_MAC(dev,coreId)    \
    if(dev->portGroupId != coreId)          \
    {                                       \
        return;                             \
    }

#define GET_FDB_DEVICE_FROM_MY_DEVICE_MAC(dev)  \
    (((dev)->portGroupSharedDevObjPtr) ?  \
     ((dev)->portGroupSharedDevObjPtr->coreDevInfoPtr[LION3_UNIT_FDB_TABLE_SINGLE_INSTANCE_PORT_GROUP_CNS].devObjPtr) : \
     (dev))

/* get device memory of other port group */
#define GET_DEVICE_MEMORY_OF_PORT_GROUP_MAC(dev,portGroupId)    \
    ((dev)->portGroupSharedDevObjPtr ?                          \
        ((dev)->portGroupSharedDevObjPtr->coreDevInfoPtr[portGroupId].devObjPtr)->deviceMemory :    \
        (dev)->deviceMemory)

/* get device object of other port group */
#define GET_DEVICE_OBJ_OF_PORT_GROUP_MAC(dev,portGroupId)    \
    ((dev)->portGroupSharedDevObjPtr ?                          \
        ((dev)->portGroupSharedDevObjPtr->coreDevInfoPtr[portGroupId].devObjPtr) :    \
        (dev))

/* bind unit of one port group to unit of another port group (shared units) */
#define BIND_UNIT_TO_OTHER_PORT_GROUP_MAC(_myDevObjPtr,_otherPortGroup,_unitIndex,_currUnitChunkPtr)   \
        if(_unitIndex == _currUnitChunkPtr->chunkIndex)                             \
        {                                                                           \
            SMEM_CHT_GENERIC_DEV_MEM_INFO *representative_devMemInfoPtr;            \
                                                                                    \
            representative_devMemInfoPtr = GET_DEVICE_MEMORY_OF_PORT_GROUP_MAC(_myDevObjPtr,_otherPortGroup);           \
            *_currUnitChunkPtr = representative_devMemInfoPtr->unitMemArr[_unitIndex];      \
            _currUnitChunkPtr->otherPortGroupDevObjPtr = GET_DEVICE_OBJ_OF_PORT_GROUP_MAC(_myDevObjPtr,_otherPortGroup); \
        }                                                                                                                \
        else                                                                                                             \
        {                                                                                                                \
            skernelFatalError("BIND_UNIT_TO_OTHER_PORT_GROUP_MAC:  mismatch between units \n");                          \
        }


/* bind DFX external memory of one port group to DFX external memory of another port group (shared units) */
#define BIND_DFX_EXTERNAL_MEMORY_TO_OTHER_PORT_GROUP_MAC(_myDevObjPtr,_otherPortGroup,_currUnitChunkPtr)   \
        {                                                                           \
            SMEM_CHT_GENERIC_DEV_MEM_INFO *representative_devMemInfoPtr;            \
                                                                                    \
            representative_devMemInfoPtr = GET_DEVICE_MEMORY_OF_PORT_GROUP_MAC(_myDevObjPtr,_otherPortGroup);           \
            *_currUnitChunkPtr = representative_devMemInfoPtr->common.pciExtMemArr[SMEM_UNIT_PCI_BUS_DFX_E].unitMem;      \
            _currUnitChunkPtr->otherPortGroupDevObjPtr = GET_DEVICE_OBJ_OF_PORT_GROUP_MAC(_myDevObjPtr,_otherPortGroup); \
        }

/* the base address for lion3 FDB table :

   NOTE:
   1. this is 'dummy address' for simulation purposes , to be able to access to it .
   2. the CPU can not access to it !
   3. we use 'unused' memory space --> we can change it to any other unit space.
*/
#define UNIT_BASE_ADDR_MAC(unit_index) ((unit_index) << SMEM_CHT_UNIT_INDEX_FIRST_BIT_CNS)
#define LION3_FDB_TABLE_BASE_ADDR_UNIT_INDEX_CNS    62
#define LION3_FDB_TABLE_BASE_ADDR_CNS(index)   UNIT_BASE_ADDR_MAC(LION3_FDB_TABLE_BASE_ADDR_UNIT_INDEX_CNS + (index)) /*0x1F000000*/

#define LION3_EGF_BASE_ADDR_UNIT_INDEX_CNS(index)    (64 + index)
#define LION3_EGF_BASE_ADDR_CNS(index)             UNIT_BASE_ADDR_MAC(LION3_EGF_BASE_ADDR_UNIT_INDEX_CNS(index))

#define LION3_EGF_EFT_BASE_ADDR_CNS(index)      LION3_EGF_BASE_ADDR_CNS(0+(index))  /*0x20000000*/
#define LION3_EGF_QAG_BASE_ADDR_CNS(index)      LION3_EGF_BASE_ADDR_CNS(2+(index))  /*0x21000000*/
#define LION3_EGF_SHT_BASE_ADDR_CNS(index)      LION3_EGF_BASE_ADDR_CNS(4+(index))  /*0x22000000*/

#define LION3_HA_BASE_ADDR_CNS(index)           0x29000000
#define LION3_IPCL_BASE_ADDR_CNS                0x2B000000
#define LION3_EPCL_BASE_ADDR_CNS                0x2C000000

#define LION3_LPM_UNIT_BASE_ADDR_CNS                    0x16000000

#define LION3_TCAM_UNIT_BASE_ADDR_CNS                   0x2A000000

#define LION3_EQ_UNIT_BASE_ADDR_CNS(index)      0x1B000000 + UNIT_BASE_ADDR_MAC(index)

#define LION3_MLL_UNIT_BASE_ADDR_CNS                    0x14000000

#define LION3_MG_UNIT_BASE_ADDR_CNS           0x00000000
#define LION3_TTI_UNIT_BASE_ADDR_CNS          0x01000000
#define LION3_DFX_SERVER_UNIT_BASE_ADDR_CNS   0x01800000
#define LION3_L2I_UNIT_BASE_ADDR_CNS          0x02000000
#define LION3_IPVX_UNIT_BASE_ADDR_CNS         0x02800000
#define LION3_BM_UNIT_BASE_ADDR_CNS           0x03000000
#define LION3_BMA_UNIT_BASE_ADDR_CNS          0x03800000
#define LION3_FDB_UNIT_BASE_ADDR_CNS          0x06000000
#define LION3_MPPM_UNIT_BASE_ADDR_CNS         0x06800000
#define LION3_CTU_UNIT_BASE_ADDR_CNS          0x07000000
#define LION3_GOP_UNIT_BASE_ADDR_CNS          0x08800000
#define LION3_MIB_UNIT_BASE_ADDR_CNS          0x09000000
#define LION3_SERDES_UNIT_BASE_ADDR_CNS       0x09800000
#define LION3_MPPM1_UNIT_BASE_ADDR_CNS        0x0A000000
#define LION3_CTU1_UNIT_BASE_ADDR_CNS         0x0A800000
#define LION3_IPCL_TCC_UNIT_BASE_ADDR_CNS     0x0D000000
#define LION3_IPVX_TCC_UNIT_BASE_ADDR_CNS     0x0D800000
#define LION3_RX_DMA_UNIT_BASE_ADDR_CNS       0x0F000000
/*#define LION3_TXQ_DIST_UNIT_BASE_ADDR_CNS     0x13000000*/

#define LION3_IOAM_UNIT_BASE_ADDR_CNS         0x14800000
#define LION3_EOAM_UNIT_BASE_ADDR_CNS         0x15800000
#define LION3_ERMRK_UNIT_BASE_ADDR_CNS        0x2D000000

#define LION3_NEW_GOP_UNIT_BASE_ADDR_CNS      0x2E000000

#define LION3_IPLR0_UNIT_BASE_ADDR_CNS        0x30000000
#define LION3_IPLR1_UNIT_BASE_ADDR_CNS        0x31000000
#define LION3_EPLR_UNIT_BASE_ADDR_CNS         0x32000000

#define LION3_CNC_0_UNIT_BASE_ADDR_CNS        0x33000000
#define LION3_CNC_1_UNIT_BASE_ADDR_CNS        0x34000000

#define LION3_TXQ_QCN_UNIT_BASE_ADDR_CNS      0x35000000
#define LION3_TXQ_QUEUE_UNIT_BASE_ADDR_CNS    0x36000000 /*was 0x10000000*/
#define LION3_TXQ_LL_UNIT_BASE_ADDR_CNS       0x37000000 /*was 0x10800000*/
#define LION3_TXQ_DQ_UNIT_BASE_ADDR_CNS       0x38000000 /*was 0x11000000*/
#define LION3_CPFC_UNIT_BASE_ADDR_CNS         0x39000000 /*was 0x13800000*/

#define LION3_TX_DMA_UNIT_BASE_ADDR_CNS       0x58000000
#define LION3_TX_FIFO_UNIT_BASE_ADDR_CNS      0x59000000
#define LION3_LMS_UNIT_BASE_ADDR_CNS          0x5a000000 /* LMS1,2,3 0x5b000000, 0x5c000000, 0x5d000000 */
#define LION3_LMS1_UNIT_BASE_ADDR_CNS         0x50000000 /* LMS1,2,3 0x51000000, 0x52000000, 0x53000000 */
#define LION3_LMS2_UNIT_BASE_ADDR_CNS         0x54000000 /* LMS1,2,3 0x55000000, 0x56000000, 0x57000000 */

static SKERNEL_DEVICE_ROUTE_TCAM_INFO_STC  lion3PortGroupRoutTcamInfo =
{
    LION2_IP_ROUT_TCAM_CNS,
    LION2_IP_ROUT_TCAM_CNS / 4,
    1,
    4,/* this is the STEP in words between 2 consecutive bank , and is regardless to fact that there are 5 banks to support TTI 30 bytes data */
    16
};


static SKERNEL_DEVICE_TCAM_INFO_STC  lion3TtiTrillAdjacencyTcanInfo =
{
    256,
    1, /* the banks are consecutive*/
    4, /* the words are consecutive */
    2, /* number of banks to compare : 2 X bank and also 2 Y banks */
    52/* number of bits to compare :
    X part 1 in bank 0:
    48 +  Outer.SA
    == 48
    2  -  reserved
    2  - control bits
    ====== 52

    X part 2 in bank 0:
    1 +   TRILL.M
    16 +  TRILL.E-RBID
    12 +  Source Device
    1 +   Source Is Trunk
    20 +  Source Trunk-ID/ePort
    == 50
    2  - control bits
    ====== 52

    Y part 0 in bank 2
    Y part 1 in bank 3
    */
    ,
    SMAIN_NOT_VALID_CNS,/*xMemoryAddress*/
    SMAIN_NOT_VALID_CNS,/*yMemoryAddress*/
    snetLion2TtiTrillAdjacencyLookUpKeyWordBuild/*lookUpKeyWordBuildPtr*/
};

#define  REG_PTR_AND_SIZE_MAC(regArr) \
    (GT_U32*)(void*)((((SMEM_CHT_PP_REGS_UNIT_START_INFO_STC*)(void*)&regArr) + 1)) , ((sizeof(regArr) - sizeof(SMEM_CHT_PP_REGS_UNIT_START_INFO_STC))/sizeof(GT_U32))

/* Timestamp ingress and egress base addresses */
static GT_U32 timestampBaseAddr_lion3[2] = {LION3_TTI_UNIT_BASE_ADDR_CNS + 0x00002800,LION3_HA_BASE_ADDR_CNS(0) + 0x00000800};

BUILD_STRING_FOR_UNIT_NAME(UNIT_MG                            );
BUILD_STRING_FOR_UNIT_NAME(UNIT_IPLR1                         );
BUILD_STRING_FOR_UNIT_NAME(UNIT_TTI                           );
BUILD_STRING_FOR_UNIT_NAME(UNIT_L2I                           );
BUILD_STRING_FOR_UNIT_NAME(UNIT_IPVX                          );
BUILD_STRING_FOR_UNIT_NAME(UNIT_BM                            );
BUILD_STRING_FOR_UNIT_NAME(UNIT_BMA                           );
BUILD_STRING_FOR_UNIT_NAME(UNIT_LMS                           );
BUILD_STRING_FOR_UNIT_NAME(UNIT_LMS1                          );
BUILD_STRING_FOR_UNIT_NAME(UNIT_LMS2                          );
BUILD_STRING_FOR_UNIT_NAME(UNIT_FDB                           );
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_MPPM                          );      */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_CTU0                          );      */
BUILD_STRING_FOR_UNIT_NAME(UNIT_EPLR                          );
BUILD_STRING_FOR_UNIT_NAME(UNIT_CNC                           );
BUILD_STRING_FOR_UNIT_NAME(UNIT_GOP                           );
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_XG_PORT_MIB                   );      */
BUILD_STRING_FOR_UNIT_NAME(UNIT_SERDES                        );
BUILD_STRING_FOR_UNIT_NAME(UNIT_MPPM1                         );
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_CTU1                          );      */
BUILD_STRING_FOR_UNIT_NAME(UNIT_EQ                            );
BUILD_STRING_FOR_UNIT_NAME(UNIT_IPCL                          );
BUILD_STRING_FOR_UNIT_NAME(UNIT_IPLR                          );
BUILD_STRING_FOR_UNIT_NAME(UNIT_MLL                           );
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_IPCL_TCC                      );      */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_IPVX_TCC                      );      */
BUILD_STRING_FOR_UNIT_NAME(UNIT_EPCL                          );
BUILD_STRING_FOR_UNIT_NAME(UNIT_HA                            );
BUILD_STRING_FOR_UNIT_NAME(UNIT_RX_DMA                        );
BUILD_STRING_FOR_UNIT_NAME(UNIT_TX_DMA                        );
BUILD_STRING_FOR_UNIT_NAME(UNIT_TXQ_QUEUE                     );
BUILD_STRING_FOR_UNIT_NAME(UNIT_TXQ_LL                        );
BUILD_STRING_FOR_UNIT_NAME(UNIT_TXQ_DQ                        );
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_TXQ_SHT                       );      */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_TXQ_EGR0                      );      */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_TXQ_EGR1                      );      */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_TXQ_DIST                      );*/
BUILD_STRING_FOR_UNIT_NAME(UNIT_CPFC                          );

BUILD_STRING_FOR_UNIT_NAME(UNIT_IOAM                          );
BUILD_STRING_FOR_UNIT_NAME(UNIT_EOAM                          );
BUILD_STRING_FOR_UNIT_NAME(UNIT_LPM                           );
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_EQ_1                          );*/
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_FDB_TABLE_0                   );      */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_FDB_TABLE_1                   );      */
BUILD_STRING_FOR_UNIT_NAME(UNIT_EGF_EFT                     );
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_EGF_EFT_1                     );      */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_EGF_QAG                     );      */
BUILD_STRING_FOR_UNIT_NAME(UNIT_EGF_QAG_1                     );
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_EGF_SHT                     );      */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_EGF_SHT_1                     );      */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_EGF_SHT_2                     );      */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_EGF_SHT_3                     );      */
BUILD_STRING_FOR_UNIT_NAME(UNIT_EGF_SHT_4                     );
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_EGF_SHT_5                     );      */
BUILD_STRING_FOR_UNIT_NAME(UNIT_EGF_SHT_6                     );
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_EGF_SHT_7                     );      */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_EGF_SHT_8                     );      */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_EGF_SHT_9                     );      */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_EGF_SHT_10                    );      */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_EGF_SHT_11                    );      */
BUILD_STRING_FOR_UNIT_NAME(UNIT_EGF_SHT_12                    );
BUILD_STRING_FOR_UNIT_NAME(UNIT_TCAM                          );
BUILD_STRING_FOR_UNIT_NAME(UNIT_ERMRK                         );


BUILD_STRING_FOR_UNIT_NAME(UNIT_CNC_1                         );

BUILD_STRING_FOR_UNIT_NAME(UNIT_TX_FIFO                        );

BUILD_STRING_FOR_UNIT_NAME(UNIT_GOP_SMI_0);
BUILD_STRING_FOR_UNIT_NAME(UNIT_GOP_SMI_1);
BUILD_STRING_FOR_UNIT_NAME(UNIT_GOP_SMI_2);
BUILD_STRING_FOR_UNIT_NAME(UNIT_GOP_SMI_3);

BUILD_STRING_FOR_UNIT_NAME(UNIT_GOP_LED_0);
BUILD_STRING_FOR_UNIT_NAME(UNIT_GOP_LED_1);
BUILD_STRING_FOR_UNIT_NAME(UNIT_GOP_LED_2);
BUILD_STRING_FOR_UNIT_NAME(UNIT_GOP_LED_3);
BUILD_STRING_FOR_UNIT_NAME(UNIT_GOP_LED_4);

BUILD_STRING_FOR_UNIT_NAME(UNIT_TXQ_QCN                       );



/*sip5 common to bc2 and lion3 */
/*extern*/ SMEM_REGISTER_DEFAULT_VALUE_STC sip5_registersDefaultValueArr[] =
{
     /*9 MSBits of 'Addr', 23 LSBits of 'Addr',     val,    repeat,    skip*/
      {&STRING_FOR_UNIT_NAME(UNIT_MG),            0x00000010,         0x2c000015,      1,    0x0     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_MG),            0x00000014,         0xffffffff,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_MG),            0x00000018,         0x80100000,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_MG),            0x00000020,         0x00000004,      1,    0x0      }
     ,{&STRING_FOR_UNIT_NAME(UNIT_MG),            0x00000044,         0x00000006,      1,    0x0     }
     ,{&STRING_FOR_UNIT_NAME(UNIT_MG),            0x0000004c,         0x000e0000,      1,    0x0     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_MG),            0x00000050,         0x000011ab,      1,    0x0      }
     ,{&STRING_FOR_UNIT_NAME(UNIT_MG),            0x00000054,         0xffff0000,      1,    0x0     }
     ,{&STRING_FOR_UNIT_NAME(UNIT_MG),            0x00000058,         0x00088002,      1,    0x0     }
     ,{&STRING_FOR_UNIT_NAME(UNIT_MG),            0x0000005c,         0x20002007,      1,    0x0     }
     ,{&STRING_FOR_UNIT_NAME(UNIT_MG),            0x00000060,         0x03030302,      1,    0x0     }
     ,{&STRING_FOR_UNIT_NAME(UNIT_MG),            0x000000c4,         0x00000001,      2,    0x8     }
     ,{&STRING_FOR_UNIT_NAME(UNIT_MG),            0x000000d4,         0x000f0f0f,      1,    0x0     }
     ,{&STRING_FOR_UNIT_NAME(UNIT_MG),            0x000000e4,         0xffffffff,      8,    0x4     }
     ,{&STRING_FOR_UNIT_NAME(UNIT_MG),            0x00000204,         0x000000e0,      1,    0x0     }
     ,{&STRING_FOR_UNIT_NAME(UNIT_MG),            0x00000254,         0x00000007,      6,    0x4     }
     ,{&STRING_FOR_UNIT_NAME(UNIT_MG),            0x00000270,         0x0000001f,      1,    0x0     }
     ,{&STRING_FOR_UNIT_NAME(UNIT_MG),            0x00000274,         0x00000001,      1,    0x0     }
     ,{&STRING_FOR_UNIT_NAME(UNIT_MG),            0x0000027c,         0x00007249,      1,    0x0     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_MG),            0x00000390,         0x000fffff,      1,    0x0      }
     ,{&STRING_FOR_UNIT_NAME(UNIT_MG),            0x00000500,         0x000e1a80,      1,    0x0     }
     ,{&STRING_FOR_UNIT_NAME(UNIT_MG),            0x00000518,         0x00000001,      1,    0x0     }
     ,{&STRING_FOR_UNIT_NAME(UNIT_MG),            0x00000654,         0x00000006,      2,    0x60    }
     ,{&STRING_FOR_UNIT_NAME(UNIT_MG),            0x00002700,         0x3fffffff,      9,    0x10    }
     ,{&STRING_FOR_UNIT_NAME(UNIT_MG),            0x00002800,         0x81800038,      1,    0x0     }
     ,{&STRING_FOR_UNIT_NAME(UNIT_MG),            0x0000281c,         0x000000ff,      1,    0x0     }
     ,{&STRING_FOR_UNIT_NAME(UNIT_MG),            0x00002860,         0x00000400,      2,    0x4     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_MG),            0x00002874,         0x00000024,      1,    0x0      }
     ,{&STRING_FOR_UNIT_NAME(UNIT_MG),            0x00002878,         0x00000400,      6,    0x4     }
     ,{&STRING_FOR_UNIT_NAME(UNIT_MG),            0x000028f0,         0x00000080,      1,    0x0     }
     ,{&STRING_FOR_UNIT_NAME(UNIT_MG),            0x00080000,         0x000000c0,      1,    0x0     }
     ,{&STRING_FOR_UNIT_NAME(UNIT_MG),            0x00080008,         0x00000041,      1,    0x0     }
     ,{&STRING_FOR_UNIT_NAME(UNIT_MG),            0x0008000c,         0x0000007c,      1,    0x0     }
     ,{&STRING_FOR_UNIT_NAME(UNIT_MG),            0x0009100c,         0x00000025,      1,    0x0     }

    ,{&STRING_FOR_UNIT_NAME(UNIT_TCAM),            0x00000000,         0x0000000f,  49152,    0x20     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TCAM),            0x00000010,         0x0000000f,  49152,    0x20     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TCAM),            0x00500000,         0x0000000f,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TCAM),            0x00500004,         0x000000f0,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TCAM),            0x00500008,         0x00000f00,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TCAM),            0x0050000c,         0x0000f000,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TCAM),            0x00500010,         0x000f0000,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TCAM),            0x00500100,         0x00000010,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TCAM),            0x00500108,         0x00000030,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TCAM),            0x00500110,         0x00000019,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TCAM),            0x00500118,         0x00000039,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TCAM),            0x00500120,         0x00000012,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TCAM),            0x00500128,         0x00000032,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TCAM),            0x00500130,         0x0000001b,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TCAM),            0x00500138,         0x0000003b,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TCAM),            0x00500140,         0x00000014,      2,    0x10     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TCAM),            0x00500148,         0x00000034,      2,    0x10     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TCAM),            0x00502304,         0x000000ff,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TCAM),            0x00503000,         0x00000042,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TCAM),            0x00503008,         0x00001ff7,      2,    0xc      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TCAM),            0x0050300c,         0x00000001,      2,    0xc      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TCAM),            0x0050301c,         0x00000001,      2,    0xc      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TCAM),            0x00504010,         0x0000000C,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TCAM),            0x00505004,         0x00000615,     20,    0x8      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TCAM),            0x00506000,         0x00000008,     20,    0x4      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TCAM),            0x00506050,         0x03000000,      3,    0x4      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TCAM),            0x0050606c,         0x03000000,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TCAM),            0x00506074,         0x0000eeee,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TCAM),            0x00507000,         0x24010302,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TCAM),            0x00507004,         0x00000202,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TCAM),            0x00507008,         0x0000000b,      1,    0x0      }

    ,{&STRING_FOR_UNIT_NAME(UNIT_MLL),            0x00000000,         0x00000070,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_MLL),            0x00000004,         0x00008888,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_MLL),            0x00000008,         0x00000003,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_MLL),            0x0000000c,         0x00000fff,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_MLL),            0x00000014,         0x0000003e,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_MLL),            0x00000040,         0x00000074,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_MLL),            0x00000050,         0x0000ffff,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_MLL),            0x00000054,         0x00000001,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_MLL),            0x00000204,         0x08040201,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_MLL),            0x00000210,         0x20000000,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_MLL),            0x00000214,         0x00006300,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_MLL),            0x00000218,         0x00000011,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_MLL),            0x0000021c,         0x00000600,      1,    0x4      }

    ,{&STRING_FOR_UNIT_NAME(UNIT_TTI),            0x00000100,         0xffffffff,      4,    0x4      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TTI),            0x000000f0,         0xffffffff,      4,    0x4      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TTI),            0x00000018,         0x65586558,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TTI),            0x00000028,         0x000088e7,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TTI),            0x00000030,         0x88488847,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TTI),            0x00000034,         0x65586558,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TTI),            0x00000038,         0x00003232,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TTI),            0x0000014c,         0x00ffffff,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TTI),            0x000001c8,         0x0000ffff,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TTI),            0x00000300,         0x81008100,      4,    0x4      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TTI),            0x00000310,         0xffffffff,      4,    0x4      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TTI),            0x00000000,         0x30002503,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TTI),            0x00000140,         0x00a6c01b,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TTI),            0x0000000c,         0x00000020,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TTI),            0x00000068,         0x1b6db81b,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TTI),            0x00000110,         0x0000004b,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TTI),            0x00000118,         0x00001320,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TTI),            0x0000011c,         0x0000001b,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TTI),            0x00000084,         0x000fff00,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TTI),            0x00000088,         0x3fffffff,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TTI),            0x0000008c,         0x3fffffff,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TTI),            0x00000200,         0x030022f3,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TTI),            0x00000204,         0x00400000,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TTI),            0x00000208,         0x12492492,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TTI),            0x0000020c,         0x00092492,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TTI),            0x00000210,         0x0180c200,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TTI),            0x00003000,         0x88f788f7,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TTI),            0x00003004,         0x013f013f,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TTI),            0x0000300c,         0x00000570,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TTI),            0x00003094,         0x88b588b5,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TTI),            0x00003098,         0x00000001,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TTI),            0x00001500,         0x00602492,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TTI),            0x00001000,         0x3fffffff,     43,    0x4      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TTI),            0x00001608,         0xffff0fff,    128,    0x20     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TTI),            0x00001600,         0x00000fff,    128,    0x20     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TTI),            0x0000160c,         0xffffffff,    128,    0x20     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TTI),            0x00000040,         0xff000000,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TTI),            0x00000044,         0x00000001,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TTI),            0x0000004c,         0xff020000,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TTI),            0x00000050,         0xff000000,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TTI),            0x00000054,         0xffffffff,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TTI),            0x00000058,         0xffffffff,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TTI),            0x0000005c,         0xffffffff,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TTI),            0x00000070,         0x00008906,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TTI),            0x00000150,         0x0000301f,      1,    0x0      }


    ,{&STRING_FOR_UNIT_NAME(UNIT_L2I),            0x00000000,         0x00806005,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_L2I),            0x00000004,         0x00002240,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_L2I),            0x00000048,         0x0007e03f,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_L2I),            0x00000120,         0x52103210,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_L2I),            0x00000130,         0x00001ffe,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_L2I),            0x00000140,         0x05f205f2,      4,    0x4      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_L2I),            0x00000160,         0x00001fff,      2,    0x4      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_L2I),            0x00001200,         0x04d85f41,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_L2I),            0x00001204,         0x00008027,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_L2I),            0x00001208,         0x00019019,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_L2I),            0x0000120c,         0x0000ffff,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_L2I),            0x00001600,         0xffffffff,      8,    0x4      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_L2I),            0x00002000,         0x0000007f,      1,    0x0      }

    ,{&STRING_FOR_UNIT_NAME(UNIT_IPVX),           0x00000000,         0x00000047,      1,    0x0    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_IPVX),           0x00000004,         0x017705dc,      4,    0x4    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_IPVX),           0x00000020,         0x0380001c,      1,    0x0    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_IPVX),           0x00000024,         0x00000fff,      1,    0x0    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_IPVX),           0x00000100,         0x1b79b01b,      1,    0x0    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_IPVX),           0x00000104,         0x001b665b,      1,    0x0    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_IPVX),           0x00000200,         0x0179b01b,      1,    0x0    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_IPVX),           0x00000204,         0x0000661B,      1,    0x0    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_IPVX),           0x00000250,         0xffc0fe80,      1,    0x0    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_IPVX),           0x00000254,         0xfe00fc00,      1,    0x0    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_IPVX),           0x00000264,         0x00000001,      1,    0x0    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_IPVX),           0x00000268,         0x00000003,      2,    0x4    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_IPVX),           0x00000360,         0x00006140,      1,    0x0    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_IPVX),           0x00000364,         0xffffffff,      1,    0x0    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_IPVX),           0x00000380,         0x001b9360,      1,    0x0    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_IPVX),           0x00000960,         0x0bbbbbbb,      1,    0x0    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_IPVX),           0x00000968,         0x0000ffff,      1,    0x0    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_IPVX),           0x00000978,         0x99abadad,      1,    0x0    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_IPVX),           0x00000e00,         0x88878685,      1,    0x0    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_IPVX),           0x00000e04,         0x8c8b8a89,      1,    0x0    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_IPVX),           0x00000e08,         0x9f8f8e8d,      1,    0x0    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_IPVX),           0x00000e0c,         0xa3a2a1a0,      1,    0x0    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_IPVX),           0x00000e10,         0xa7a6a5a4,      1,    0x0    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_IPVX),           0x00000e14,         0xabaaa9a8,      1,    0x0    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_IPVX),           0x00000e18,         0xafaeadac,      1,    0x0    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_IPVX),           0x00000e1c,         0x90b6b1b0,      1,    0x0    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_IPVX),           0x00000e20,         0x91b5b4b3,      1,    0x0    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_IPVX),           0x00000e24,         0xcbcac9c8,      1,    0x0    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_IPVX),           0x00000e28,         0x9392cdcc,      1,    0x0    }

    ,{&STRING_FOR_UNIT_NAME(UNIT_FDB),            0x00000000,         0x107bfc00,      1,    0x0    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_FDB),            0x00000004,         0x07012b3c,      1,    0x0    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_FDB),            0x0000000c,         0xffffffff,      1,    0x0    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_FDB),            0x00000014,         0x00040000,      1,    0x0    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_FDB),            0x00000020,         0x000003c1,      1,    0x0    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_FDB),            0x00000090,         0xffffffff,      2,    0x80   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_FDB),            0x00000094,         0x0000002a,      1,    0x0    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_FDB),            0x00000114,         0x0000ffff,      1,    0x0    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_FDB),            0x00000118,         0x00001fff,      1,    0x0    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_FDB),            0x0000011c,         0xffffffff,      2,    0x4    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_FDB),            0x00000170,         0xffff0000,      1,    0x0    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_FDB),            0x00000174,         0x007fffff,      1,    0x0    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_FDB),            0x00000244,         0x00000001,      1,    0x0    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_FDB),            0x00000248,         0x00000002,      1,    0x0    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_FDB),            0x0000024c,         0x00000003,      1,    0x0    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_FDB),            0x00000250,         0x00000004,      1,    0x0    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_FDB),            0x00000254,         0x00000005,      1,    0x0    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_FDB),            0x00000258,         0x00000006,      1,    0x0    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_FDB),            0x0000025c,         0x00000007,      1,    0x0    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_FDB),            0x00000260,         0x00000008,      1,    0x0    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_FDB),            0x00000264,         0x00000009,      1,    0x0    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_FDB),            0x00000268,         0x0000000a,      1,    0x0    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_FDB),            0x0000026c,         0x0000000b,      1,    0x0    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_FDB),            0x00000270,         0x0000000c,      1,    0x0    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_FDB),            0x00000274,         0x0000000d,      1,    0x0    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_FDB),            0x00000278,         0x0000000e,      1,    0x0    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_FDB),            0x0000027c,         0x0000000f,      1,    0x0    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_FDB),            0x00000380,         0x00000002,      2,    0x80   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_FDB),            0x00000384,         0x00010162,      1,    0x0    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_FDB),            0x00000410,         0x00000007,      1,    0x0    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_FDB),            0x00000480,         0x00000066,      1,    0x0    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_FDB),            0x00000484,         0x00008420,      1,    0x0    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_FDB),            0x00000488,         0x002bfdfe,      1,    0x0    }

    ,DEFAULT_REG_FOR_PLR_UNIT_MAC(                0x00000000,         0xbe830003,      1,    0x0    )
    ,DEFAULT_REG_FOR_PLR_UNIT_MAC(                0x00000004,         0x00000008,      1,    0x0    )
    ,DEFAULT_REG_FOR_PLR_UNIT_MAC(                0x0000000c,         0xffffffff,      1,    0x0    )
    ,DEFAULT_REG_FOR_PLR_UNIT_MAC(                0x00000010,         0x00002800,      1,    0x0    )
    ,DEFAULT_REG_FOR_PLR_UNIT_MAC(                0x0000002c,         0x00000020,      1,    0x0    )
    ,DEFAULT_REG_FOR_PLR_UNIT_MAC(                0x00000030,         0x3fffffff,      1,    0x0    )
    ,DEFAULT_REG_FOR_PLR_UNIT_MAC(                0x00000034,         0x00001000,      1,    0x0    )
    ,DEFAULT_REG_FOR_PLR_UNIT_MAC(                0x00000038,         0x00100000,      1,    0x0    )
    ,DEFAULT_REG_FOR_PLR_UNIT_MAC(                0x0000003c,         0x0000000f,      1,    0x0    )
    ,DEFAULT_REG_FOR_PLR_UNIT_MAC(                0x00000060,         0x00001000,      1,    0x0    )
    ,DEFAULT_REG_FOR_PLR_UNIT_MAC(                0x00000064,         0x00000f00,      1,    0x0    )
    ,DEFAULT_REG_FOR_PLR_UNIT_MAC(                0x00000068,         0x00000100,      1,    0x0    )
    ,DEFAULT_REG_FOR_PLR_UNIT_MAC(                0x00000220,         0xffff0000,      1,    0x0    )

    ,DEFAULT_REG_FOR_OAM_UNIT_MAC(0x00000020,         0x000000ff,      1,    0x0      )
    ,DEFAULT_REG_FOR_OAM_UNIT_MAC(0x00000024,         0x00000010,      1,    0x0      )
    ,DEFAULT_REG_FOR_OAM_UNIT_MAC(0x00000028,         0x01010101,      2,    0x4      )
    ,DEFAULT_REG_FOR_OAM_UNIT_MAC(0x00000030,         0x2b2b2b2b,      4,    0x4      )
    ,DEFAULT_REG_FOR_OAM_UNIT_MAC(0x00000050,         0x2d2d2d2d,      4,    0x4      )
    ,DEFAULT_REG_FOR_OAM_UNIT_MAC(0x00000070,         0x01010101,      4,    0x4      )
    ,DEFAULT_REG_FOR_OAM_UNIT_MAC(0x00000084,         0x00000001,      1,    0x0      )

    ,DEFAULT_REG_FOR_CNC_UNIT_MAC(0x00000000,         0x000007df,      1,    0x0      )
    ,DEFAULT_REG_FOR_CNC_UNIT_MAC(0x00000180,         0xffff0000,      1,    0x0      )
    ,DEFAULT_REG_FOR_CNC_UNIT_MAC(0x0000018c,         0x00000183,      1,    0x0      )
    ,DEFAULT_REG_FOR_CNC_UNIT_MAC(0x00001098,         0x00000008,     16,    0x4      )

    ,{&STRING_FOR_UNIT_NAME(UNIT_BM),             0x00000000,         0x3fff0000,      1,    0x0    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_BM),             0x00000004,         0x80000000,      1,    0x0    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_BM),             0x00000010,         0xffffffff,      2,    0x4    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_BM),             0x00000018,         0x0000ffff,      2,    0x8    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_BM),             0x00000020,         0x0000ffff,      1,    0x0    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_BM),             0x00000400,         0x00000009,      4,    0x4    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_BM),             0x000004c0,         0x00fc0fc0,      2,    0x10   }

    ,{&STRING_FOR_UNIT_NAME(UNIT_BMA),            0x00019008,         0x00280249,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_BMA),            0x00019030,         0xffff0000,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_BMA),            0x00019034,         0x00000100,      4,    0x4      }

    ,{&STRING_FOR_UNIT_NAME(UNIT_EGF_QAG_1),            0x00600000,         0x00000003,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_EGF_QAG_1),            0x006001c0,         0xffffffff,      8,    0x4      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_EGF_QAG_1),            0x006001f0,         0x00249249,      4,    0x4      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_EGF_QAG_1),            0x00600200,         0x00492492,      4,    0x4      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_EGF_QAG_1),            0x00600210,         0x006db6db,      4,    0x4      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_EGF_QAG_1),            0x00600220,         0x00924924,      4,    0x4      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_EGF_QAG_1),            0x00600230,         0x00b6db6d,      4,    0x4      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_EGF_QAG_1),            0x00600240,         0x00db6db6,      4,    0x4      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_EGF_QAG_1),            0x00600250,         0x00ffffff,      4,    0x4      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_EGF_QAG_1),            0x006005fc,         0xe4e4e4e4,     16,    0x4      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_EGF_QAG_1),            0x00700030,         0xffff0000,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_EGF_QAG_1),            0x00700100,         0x80000111,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_EGF_QAG_1),            0x00700104,         0x88888888,      2,    0x4      }

    ,{&STRING_FOR_UNIT_NAME(UNIT_EGF_SHT_12),           0x00020000,         0x00000007,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_EGF_SHT_12),           0x00020004,         0x00000001,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_EGF_SHT_12),           0x00020008,         0x0000000f,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_EGF_SHT_12),           0x00020030,         0xffffffff,      8,    0x4,   2,  0x80  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_EGF_SHT_12),           0x000200f0,         0xffffffff,      8,    0x4      }

    ,{&STRING_FOR_UNIT_NAME(UNIT_EGF_SHT_4),            0x00100000,         0x00000002,   4096,    0x4      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_EGF_SHT_4),            0x00120000,         0xffffffff,   1024,    0x4      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_EGF_SHT_4),            0x000a0000,         0xffffffff,  32768,    0x4      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_EGF_SHT_4),            0x000e0000,         0xffffffff,   2048,    0x4      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_EGF_SHT_4),            0x00080000,         0xffffffff,  32768,    0x4      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_EGF_SHT_6),            0x00000040,         0xffffffff,      8,    0x4      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_EGF_SHT_6),            0x00000060,         0x00000001,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_EGF_SHT_4),            0x000dffe0,         0xffffffff,      8,    0x4      }

    ,{&STRING_FOR_UNIT_NAME(UNIT_EGF_EFT),            0x00001000,         0x000007e7,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_EGF_EFT),            0x00001020,         0x0ae00800,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_EGF_EFT),            0x00001030,         0x0000000e,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_EGF_EFT),            0x00001040,         0x3f3f3f3f,      4,    0x4      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_EGF_EFT),            0x00001110,         0xffff0000,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_EGF_EFT),            0x00001150,         0x3f3f3f3f,     64,    0x4      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_EGF_EFT),            0x00002108,         0x08080808,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_EGF_EFT),            0x000021d0,         0x0870e1c3,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_EGF_EFT),            0x000021e0,         0x000002c3,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_EGF_EFT),            0x000021f4,         0x55555555,      4,    0x10     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_EGF_EFT),            0x000021f8,         0xaaaaaaaa,      4,    0x10     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_EGF_EFT),            0x000021fc,         0xffffffff,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_EGF_EFT),            0x0000220c,         0xffffffff,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_EGF_EFT),            0x0000221c,         0xffffffff,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_EGF_EFT),            0x0000222c,         0xffffffff,      2,    0x24     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_EGF_EFT),            0x00002254,         0xffffffff,      3,    0x4      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_EGF_EFT),            0x00003000,         0x00000010,      8,    0x4      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_EGF_EFT),            0x00010000,         0x00000801,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_EGF_EFT),            0x00010004,         0x000fff39,      1,    0x0      }

    /* UNIT_TXQ_DQ*/
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_DQ),            0x00000000,         0x0000017f,      1,    0x0         }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_DQ),            0x00000208,         0x28002800,      1,    0x0         }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_DQ),            0x0000020c,         0x00000002,     16,    0x4         }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_DQ),            0x000002c4,         0x00000001,      5,    0x44        }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_DQ),            0x000002c8,         0x00000002,      5,    0x44        }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_DQ),            0x000002cc,         0x00000003,      5,    0x44        }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_DQ),            0x000002d0,         0x00000004,      4,    0x44        }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_DQ),            0x000002d4,         0x00000005,      4,    0x44        }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_DQ),            0x000002d8,         0x00000006,      4,    0x44        }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_DQ),            0x000002dc,         0x00000007,      4,    0x44        }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_DQ),            0x000002e0,         0x00000008,      4,    0x44        }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_DQ),            0x000002e4,         0x00000009,      4,    0x44        }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_DQ),            0x000002e8,         0x0000000a,      4,    0x44        }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_DQ),            0x000002ec,         0x0000000b,      4,    0x44        }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_DQ),            0x000002f0,         0x0000000c,      4,    0x44        }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_DQ),            0x000002f4,         0x0000000d,      4,    0x44        }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_DQ),            0x000002f8,         0x0000000e,      4,    0x44        }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_DQ),            0x000002fc,         0x0000000f,      4,    0x44        }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_DQ),            0x00000300,         0x00000010,      4,    0x44        }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_DQ),            0x00000a00,         0x0000ffff,      1,    0x0         }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_DQ),            0x00001000,         0x00e44e15,      1,    0x0         }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_DQ),            0x00001040,         0x00000003,     72,    0x4         }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_DQ),            0x00001240,         0x01010101,     16,    0x4         }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_DQ),            0x00001280,         0x05050505,     16,    0x4         }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_DQ),            0x00001300,         0x000000ff,     16,    0x4         }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_DQ),            0x00001500,         0x00001553,      1,    0x0         }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_DQ),            0x00001650,         0x83828180,      5,    0x48        }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_DQ),            0x00001654,         0x87868584,      5,    0x48        }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_DQ),            0x00001658,         0x8b8a8988,      5,    0x48        }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_DQ),            0x0000165c,         0x8f8e8d8c,      5,    0x48        }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_DQ),            0x00001660,         0x93929190,      5,    0x48        }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_DQ),            0x00001664,         0x97969594,      5,    0x48        }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_DQ),            0x00001668,         0x9b9a9998,      5,    0x48        }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_DQ),            0x0000166c,         0x9f9e9d9c,      5,    0x48        }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_DQ),            0x00001670,         0xa3a2a1a0,      5,    0x48        }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_DQ),            0x00001674,         0xa7a6a5a4,      5,    0x48        }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_DQ),            0x00001678,         0xabaaa9a8,      5,    0x48        }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_DQ),            0x0000167c,         0xafaeadac,      5,    0x48        }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_DQ),            0x00001680,         0xb3b2b1b0,      5,    0x48        }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_DQ),            0x00001684,         0xb7b6b5b4,      4,    0x48        }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_DQ),            0x00001688,         0xbbbab9b8,      4,    0x48        }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_DQ),            0x0000168c,         0xbfbebdbc,      4,    0x48        }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_DQ),            0x00001690,         0xc3c2c1c0,      4,    0x48        }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_DQ),            0x00001694,         0xc7c6c5c4,      4,    0x48        }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_DQ),            0x00001904,         0x03030303,      1,    0x0         }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_DQ),            0x00004000,         0x82080000,      1,    0x0         }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_DQ),            0x00004004,         0x00000600,      2,    0x4         }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_DQ),            0x00004300,         0x000000ff,     72,    0x4         }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_DQ),            0x0000c000,         0x00000002,      2,    0x88        }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_DQ),            0x0000c084,         0x00000001,      1,    0x0         }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_DQ),            0x0000c08c,         0x00000003,      1,    0x0         }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_DQ),            0x0000c090,         0x00000004,      1,    0x0         }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_DQ),            0x0000c094,         0x00000005,      1,    0x0         }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_DQ),            0x0000c098,         0x00000006,      1,    0x0         }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_DQ),            0x0000c09c,         0x00000007,      1,    0x0         }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_DQ),            0x0000d000,         0x00000020,      1,    0x0         }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_DQ),            0x0000d014,         0x00000008,      1,    0x0         }

    /* UNIT_TXQ_LL*/
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_LL),            0x000a1000,         0x0000ffff,      1,    0x0         }

    /* UNIT_CPFC*/
    ,{&STRING_FOR_UNIT_NAME(UNIT_CPFC),              0x00001100,         0xffff0000,      1,    0x0       }
    ,{&STRING_FOR_UNIT_NAME(UNIT_CPFC),              0x00001110,         0x00002860,      1,    0x0       }
    ,{&STRING_FOR_UNIT_NAME(UNIT_CPFC),              0x00014000,         0x1fffffff,      8,    0x4,      2,    0x400       }

    /* UNIT_TXQ_QCN*/
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_QCN),           0x00000000,         0x00000002,      1,    0x0       }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_QCN),           0x00000200,         0x00000104,      1,    0x0       }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_QCN),           0x00000210,         0xffff0000,      1,    0x0       }

    /* UNIT_TXQ_QUEUE*/
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_QUEUE),         0x00090a80,         0x0000ffff,      1,    0x0     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_QUEUE),         0x00093000,         0x0009f599,      1,    0x0     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_QUEUE),         0x00093020,         0x00001011,      1,    0x0     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_QUEUE),         0x000a0000,         0x000000c8,      1,    0x0     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_QUEUE),         0x000a0004,         0x00000004,     16,    0x4     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_QUEUE),         0x000a0050,         0x000000ff,     72,    0x4     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_QUEUE),         0x000a03a0,         0xffffffff,      8,    0x4     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_QUEUE),         0x000a0500,         0xffffffff,     16,    0x4,      2,    0x80        }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_QUEUE),         0x000a0800,         0x00003cd8,      1,    0x0     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_QUEUE),         0x000a0810,         0x00004000,      1,    0x0     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_QUEUE),         0x000a0820,         0x00000019,     16,    0x4     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_QUEUE),         0x000a0900,         0x00000028,     16,    0x4     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_QUEUE),         0x000a0a00,         0x00000300,      8,    0x4     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_QUEUE),         0x000a0a20,         0x000002bc,      8,    0x4     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_QUEUE),         0x000a9000,         0x000a000a,      1,    0x0     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_QUEUE),         0x000a9004,         0x0000000a,      2,    0xc     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_QUEUE),         0x000a9020,         0x00000800,      2,    0x10    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_QUEUE),         0x000aa004,         0x00000800,     16,    0x4     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_QUEUE),         0x000aa050,         0x00000400,     32,    0x4     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_QUEUE),         0x000aa0d0,         0x00000100,     16,    0x4     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_QUEUE),         0x000aa110,         0x00001000,      8,    0x4     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_QUEUE),         0x000aa1b0,         0x00000400,      8,    0x4     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_QUEUE),         0x000aa210,         0x00002800,      1,    0x0     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_QUEUE),         0x000aa230,         0x00000100,      1,    0x0     }


    ,{&STRING_FOR_UNIT_NAME(UNIT_EQ),            0x00080000,         0x0000ffff,    255,    0x4      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_EQ),            0x0000a100,         0xffffffff,      8,    0x4      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_EQ),            0x0000a000,         0x000204e2,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_EQ),            0x0000a010,         0x000000a0,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_EQ),            0x0000a020,         0x000007e0,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_EQ),            0x0000b200,         0x00800000,      7,    0x4      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_EQ),            0x0000b000,         0x00004e01,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_EQ),            0x0000b004,         0x00000801,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_EQ),            0x00005020,         0x00000020,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_EQ),            0x00005024,         0xffffffff,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_EQ),            0x00005030,         0x000000a0,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_EQ),            0x0000d000,         0x00000020,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_EQ),            0x0000d004,         0xffffffff,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_EQ),            0x0000d010,         0x000000a0,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_EQ),            0x0000001c,         0x00000158,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_EQ),            0x00020010,         0x00000063,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_EQ),            0x00005054,         0x0000003f,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_EQ),            0x00100000,         0xffffffff,    256,    0x4      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_EQ),            0x00130000,         0x0fffffff,    256,    0x4      }

    ,{&STRING_FOR_UNIT_NAME(UNIT_IPCL),            0x00000000,         0x00000001,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_IPCL),            0x0000000c,         0x02801000,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_IPCL),            0x0000001c,         0x00000008,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_IPCL),            0x00000020,         0x00000042,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_IPCL),            0x00000034,         0x00000fff,      3,    0x4      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_IPCL),            0x00000040,         0x1f000000,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_IPCL),            0x00000044,         0x0000c71c,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_IPCL),            0x00000048,         0x0000ffff,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_IPCL),            0x00000730,         0x00ff0080,      1,    0x0      }

    ,{&STRING_FOR_UNIT_NAME(UNIT_EPCL),            0x00000000,         0x00000115,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_EPCL),            0x0000000c,         0x00ff0080,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_EPCL),            0x00000074,         0x76543210,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_EPCL),            0x000000a0,         0x0000001e,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_EPCL),            0x000000c0,         0xffff0000,      1,    0x0      }

    ,{&STRING_FOR_UNIT_NAME(UNIT_HA),            0x00000000,         0x00109010,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_HA),            0x00000004,         0x00000003,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_HA),            0x00000008,         0x00000004,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_HA),            0x00000080,         0xff000000,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_HA),            0x00000084,         0x00000001,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_HA),            0x0000008c,         0xff020000,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_HA),            0x00000090,         0xffffffff,      4,    0x4      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_HA),            0x000003d0,         0x00000008,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_HA),            0x00000408,         0x81000000,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_HA),            0x0000040c,         0x81000000,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_HA),            0x00000410,         0x00010000,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_HA),            0x00000424,         0x00010000,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_HA),            0x00000500,         0x00008100,      8,    0x4      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_HA),            0x00000550,         0x88488847,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_HA),            0x00000554,         0x00008100,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_HA),            0x00000558,         0x000022f3,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_HA),            0x0000055c,         0x00006558,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_HA),            0x00000710,         0x0000ffff,      1,    0x0      }

    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),            0x00000000,         0x000e0007,      8,    0x8   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),            0x00000004,         0x00003fff,      8,    0x8   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),            0x00000040,         0x00001fff,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),            0x00000084,         0x81008100,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),            0x000000a0,         0x00000003,      1,    0x0   }

    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00000000,         0x00000007,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00000004,         0x00000102,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00000008,         0x0005003f,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x0000000c,         0x000000f0,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00000010,         0x0000ffff,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00000020,         0x03020100,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00000024,         0x07060504,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00000028,         0x0b0a0908,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x0000002c,         0x0f0e0d0c,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00000030,         0x13121110,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00000034,         0x17161514,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00000038,         0x1b1a1918,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x0000003c,         0x1f1e1d1c,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00000040,         0x23222120,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00000044,         0x27262524,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00000048,         0x2b2a2928,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x0000004c,         0x2f2e2d2c,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00000050,         0x33323130,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00000054,         0x37363534,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00000058,         0x3b3a3938,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x0000005c,         0x3f3e3d3c,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00000060,         0x43424140,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00000064,         0x47464544,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00000068,         0x4b4a4948,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x0000006c,         0x4f4e4d4c,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00000070,         0x53525150,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00000074,         0x57565554,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00000078,         0x5b5a5958,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x0000007c,         0x5f5e5d5c,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00000080,         0x63626160,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00000084,         0x67666564,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00000088,         0x6b6a6968,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x0000008c,         0x6f6e6d6c,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00000090,         0x73727170,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00000094,         0x77767574,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00000098,         0x7b7a7978,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x0000009c,         0x7f7e7d7c,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x000000a0,         0x83828180,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x000000a4,         0x87868584,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x000000a8,         0x8b8a8988,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x000000ac,         0x8f8e8d8c,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x000000b0,         0x93929190,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x000000b4,         0x97969594,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x000000b8,         0x9b9a9998,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x000000bc,         0x9f9e9d9c,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x000000c0,         0xa3a2a1a0,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x000000c4,         0xa7a6a5a4,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x000000c8,         0xabaaa9a8,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x000000cc,         0xafaeadac,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x000000d0,         0xb3b2b1b0,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x000000d4,         0xb7b6b5b4,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x000000d8,         0xbbbab9b8,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x000000dc,         0xbfbebdbc,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x000000e0,         0xc3c2c1c0,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x000000e4,         0xc7c6c5c4,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x000000e8,         0xcbcac9c8,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x000000ec,         0xcfcecdcc,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x000000f0,         0xd3d2d1d0,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x000000f4,         0xd7d6d5d4,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x000000f8,         0xdbdad9d8,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x000000fc,         0xdfdedddc,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00000100,         0xe3e2e1e0,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00000104,         0xe7e6e5e4,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00000108,         0xebeae9e8,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x0000010c,         0xefeeedec,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00000110,         0xf3f2f1f0,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00000114,         0xf7f6f5f4,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00000118,         0xfbfaf9f8,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x0000011c,         0xfffefdfc,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00000200,         0x12492492,      8,    0x4   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00001000,         0x000019c8,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00001004,         0x000000f5,      2,    0x8   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00001008,         0x00000006,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00001010,         0x000000f5,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00001014,         0x00000005,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00001020,         0x00003bf7,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00003000,         0xf803e064,     74,    0x20  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00003004,         0x000000ce,     74,    0x20  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00003008,         0x00000c03,     74,    0x20  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00003010,         0x10a20201,     74,    0x20  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00003014,         0x0000000f,     74,    0x20  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00004000,         0x0000404a,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00004004,         0x00000004,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00004008,         0x83828180,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x0000400c,         0x87868584,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00004010,         0x8b8a8988,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00004014,         0x8f8e8d8c,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00004018,         0x93929190,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x0000401c,         0x97969594,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00004020,         0x9b9a9998,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00004024,         0x9f9e9d9c,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00004028,         0xa3a2a1a0,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x0000402c,         0xa7a6a5a4,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00004030,         0xabaaa9a8,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00004034,         0xafaeadac,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00004038,         0xb3b2b1b0,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x0000403c,         0xb7b6b5b4,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00004040,         0xbbbab9b8,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00004044,         0xbfbebdbc,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00004048,         0xc3c2c1c0,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x0000404c,         0xc7c6c5c4,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00004050,         0x8180c9c8,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00004054,         0x85848382,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00004058,         0x89888786,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x0000405c,         0x8d8c8b8a,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00004060,         0x91908f8e,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00004064,         0x95949392,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00004068,         0x99989796,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x0000406c,         0x9d9c9b9a,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00004070,         0xa1a09f9e,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00004074,         0xa5a4a3a2,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00004078,         0xa9a8a7a6,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x0000407c,         0xadacabaa,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00004080,         0xb1b0afae,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00004084,         0xb5b4b3b2,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00004088,         0xb9b8b7b6,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x0000408c,         0xbdbcbbba,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00004090,         0xc1c0bfbe,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00004094,         0xc5c4c3c2,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00004098,         0xc9c8c7c6,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x0000409c,         0x83828180,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x000040a0,         0x87868584,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x000040a4,         0x8b8a8988,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x000040a8,         0x8f8e8d8c,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x000040ac,         0x93929190,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x000040b0,         0x97969594,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x000040b4,         0x9b9a9998,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x000040b8,         0x9f9e9d9c,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x000040bc,         0xa3a2a1a0,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x000040c0,         0xa7a6a5a4,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x000040c4,         0xabaaa9a8,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x000040c8,         0xafaeadac,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x000040cc,         0xb3b2b1b0,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x000040d0,         0xb7b6b5b4,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x000040d4,         0xbbbab9b8,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x000040d8,         0xbfbebdbc,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x000040dc,         0xc3c2c1c0,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x000040e0,         0xc7c6c5c4,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x000040e4,         0x8180c9c8,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x000040e8,         0x85848382,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x000040ec,         0x89888786,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x000040f0,         0x8d8c8b8a,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x000040f4,         0x91908f8e,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x000040f8,         0x95949392,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x000040fc,         0x99989796,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00004100,         0x9d9c9b9a,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00004104,         0xa1a09f9e,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00004108,         0xa5a4a3a2,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x0000410c,         0xa9a8a7a6,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00004110,         0xadacabaa,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00004114,         0xb1b0afae,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00004118,         0xb5b4b3b2,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x0000411c,         0xb9b8b7b6,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00004120,         0xbdbcbbba,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00004124,         0xc1c0bfbe,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00004128,         0xc5c4c3c2,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x0000412c,         0xc9c8c7c6,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00004130,         0x83828180,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00004134,         0x87868584,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00004138,         0x8b8a8988,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x0000413c,         0x8f8e8d8c,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00004140,         0x93929190,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00004144,         0x97969594,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00004148,         0x9b9a9998,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x0000414c,         0x9f9e9d9c,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00004150,         0xa3a2a1a0,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00004154,         0xa7a6a5a4,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),            0x00004158,         0xabaaa9a8,      1,    0x0   }

    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x00000000,         0x800002fc,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x00000004,         0x0000ffff,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x00000008,         0x00000001,     74,    0x4   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x00000600,         0x00000010,     74,    0x4   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x00000800,         0x0000404a,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x00000804,         0x00000004,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x00000808,         0x83828180,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x0000080c,         0x87868584,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x00000810,         0x8b8a8988,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x00000814,         0x8f8e8d8c,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x00000818,         0x93929190,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x0000081c,         0x97969594,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x00000820,         0x9b9a9998,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x00000824,         0x9f9e9d9c,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x00000828,         0xa3a2a1a0,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x0000082c,         0xa7a6a5a4,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x00000830,         0xabaaa9a8,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x00000834,         0xafaeadac,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x00000838,         0xb3b2b1b0,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x0000083c,         0xb7b6b5b4,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x00000840,         0xbbbab9b8,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x00000844,         0xbfbebdbc,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x00000848,         0xc3c2c1c0,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x0000084c,         0xc7c6c5c4,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x00000850,         0x8180c9c8,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x00000854,         0x85848382,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x00000858,         0x89888786,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x0000085c,         0x8d8c8b8a,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x00000860,         0x91908f8e,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x00000864,         0x95949392,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x00000868,         0x99989796,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x0000086c,         0x9d9c9b9a,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x00000870,         0xa1a09f9e,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x00000874,         0xa5a4a3a2,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x00000878,         0xa9a8a7a6,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x0000087c,         0xadacabaa,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x00000880,         0xb1b0afae,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x00000884,         0xb5b4b3b2,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x00000888,         0xb9b8b7b6,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x0000088c,         0xbdbcbbba,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x00000890,         0xc1c0bfbe,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x00000894,         0xc5c4c3c2,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x00000898,         0xc9c8c7c6,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x0000089c,         0x83828180,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x000008a0,         0x87868584,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x000008a4,         0x8b8a8988,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x000008a8,         0x8f8e8d8c,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x000008ac,         0x93929190,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x000008b0,         0x97969594,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x000008b4,         0x9b9a9998,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x000008b8,         0x9f9e9d9c,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x000008bc,         0xa3a2a1a0,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x000008c0,         0xa7a6a5a4,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x000008c4,         0xabaaa9a8,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x000008c8,         0xafaeadac,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x000008cc,         0xb3b2b1b0,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x000008d0,         0xb7b6b5b4,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x000008d4,         0xbbbab9b8,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x000008d8,         0xbfbebdbc,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x000008dc,         0xc3c2c1c0,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x000008e0,         0xc7c6c5c4,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x000008e4,         0x8180c9c8,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x000008e8,         0x85848382,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x000008ec,         0x89888786,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x000008f0,         0x8d8c8b8a,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x000008f4,         0x91908f8e,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x000008f8,         0x95949392,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x000008fc,         0x99989796,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x00000900,         0x9d9c9b9a,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x00000904,         0xa1a09f9e,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x00000908,         0xa5a4a3a2,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x0000090c,         0xa9a8a7a6,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x00000910,         0xadacabaa,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x00000914,         0xb1b0afae,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x00000918,         0xb5b4b3b2,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x0000091c,         0xb9b8b7b6,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x00000920,         0xbdbcbbba,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x00000924,         0xc1c0bfbe,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x00000928,         0xc5c4c3c2,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x0000092c,         0xc9c8c7c6,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x00000930,         0x83828180,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x00000934,         0x87868584,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x00000938,         0x8b8a8988,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x0000093c,         0x8f8e8d8c,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x00000940,         0x93929190,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x00000944,         0x97969594,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x00000948,         0x9b9a9998,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x0000094c,         0x9f9e9d9c,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x00000950,         0xa3a2a1a0,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x00000954,         0xa7a6a5a4,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x00000958,         0xabaaa9a8,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x00003000,         0x00000bac,      1,    0x0   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),           0x00003100,         0x80025540,     74,    0x4   }


    ,{&STRING_FOR_UNIT_NAME(UNIT_MPPM1),            0x00000010,         0x00000001,      1,    0x0     }


    ,{&STRING_FOR_UNIT_NAME(UNIT_ERMRK),            0x00000000,         0x000001df,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ERMRK),            0x00000010,         0x00000400,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ERMRK),            0x00000014,         0x00000444,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ERMRK),            0x000000e0,         0x00000020,      1,    0x0  }
    ,{&STRING_FOR_UNIT_NAME(UNIT_ERMRK),            0x00000108,         0x0000ffff,      1,    0x0  }


    /* PTP */
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),           0x00180808,         0x00000001,      56,    0x1000 }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),           0x00180870,         0x000083aa,      56,    0x1000 }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),           0x00180874,         0x00007e5d,      56,    0x1000 }

    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),           0x00380808,         0x00000001,      16,    0x1000 }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),           0x00380870,         0x000083aa,      16,    0x1000 }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),           0x00380874,         0x00007e5d,      16,    0x1000 }

    /* LPM unit - ETA 0,1 registers */
    ,{&STRING_FOR_UNIT_NAME(UNIT_LPM),               0x00E00000,         0xffff0000,      1,     0x0 ,      2,    0x100000 }
    ,{&STRING_FOR_UNIT_NAME(UNIT_LPM),               0x00E00110,         0x000000ff,      3,     0x10,      2,    0x100000 }


    /*;;;;;;;;;;;; manually added ;;;;;;;;;;;;;;;;;;;;;;;;;*/

    ,{&STRING_FOR_UNIT_NAME(UNIT_MG),             0x000000D4,         0x000F0F0F,      1,    0x0     }

    ,{&STRING_FOR_UNIT_NAME(UNIT_TTI),            0x00000170,         0x0fffffff,      4,    0x4      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TTI),            0x000013fc,         0x000000da,      1,    0x0      }

    ,{&STRING_FOR_UNIT_NAME(UNIT_TTI),            0x00290004,         0x00020000,    256,    0x20     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TTI),            0x00290014,         0x00020000,    256,    0x20     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TTI),            0x00080000,         0x3fffffff,  16384,    0x4      }

    ,{NULL,            0,         0x00000000,      0,    0x0      }
};

/*extern*/ SMEM_REGISTER_DEFAULT_VALUE_STC sip5_gop_tai_registersDefaultValueArr[] =
{
    /* TAI */
     {&STRING_FOR_UNIT_NAME(UNIT_GOP),           0x00180a08,         0x00000306,       3,    0x10000 , 2, 0x1000}
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),           0x00180a0c,         0x00000492,       3,    0x10000 , 2, 0x1000}

    /* Default value different from CIDER due to errata */
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),           0x00180aa4,         0x00000003,       3,    0x10000 , 2, 0x1000}

    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),           0x00180adc,         0x0000ffff,       3,    0x10000 , 2, 0x1000}
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),           0x00180ae0,         0x0000ff00,       3,    0x10000 , 2, 0x1000}

/*    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),           0x00181a08,         0x00000306,       3,    0x10000 }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),           0x00181a0c,         0x00000492,       3,    0x10000 }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),           0x00181adc,         0x0000ffff,       3,    0x10000 }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),           0x00181ae0,         0x0000ff00,       3,    0x10000 }
*/
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),           0x001b0a08,         0x00000306,       2,    0x4000 , 2, 0x1000}
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),           0x001b0a0c,         0x00000492,       2,    0x4000 , 2, 0x1000}

    /* Default value different from CIDER due to errata */
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),           0x001b0aa4,         0x00000003,       2,    0x4000 , 2, 0x1000}

    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),           0x001b0adc,         0x0000ffff,       2,    0x4000 , 2, 0x1000}
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),           0x001b0ae0,         0x0000ff00,       2,    0x4000 , 2, 0x1000}

/*    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),           0x001b1a08,         0x00000306,       2,    0x4000 }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),           0x001b0a0c,         0x00000492,       2,    0x4000 }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),           0x001b0adc,         0x0000ffff,       2,    0x4000 }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),           0x001b0ae0,         0x0000ff00,       2,    0x4000 }
*/
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),           0x00380a08,         0x00000306,       4,    0x4000 , 2, 0x1000}
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),           0x00380a0c,         0x00000492,       4,    0x4000 , 2, 0x1000}

    /* Default value different from CIDER due to errata */
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),           0x00380aa4,         0x00000003,       4,    0x4000 , 2, 0x1000}

    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),           0x00380adc,         0x0000ffff,       4,    0x4000 , 2, 0x1000}
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),           0x00380ae0,         0x0000ff00,       4,    0x4000 , 2, 0x1000}

/*    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),           0x00381a08,         0x00000306,       4,    0x4000 }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),           0x00381a0c,         0x00000492,       4,    0x4000 }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),           0x00381adc,         0x0000ffff,       4,    0x4000 }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),           0x00381ae0,         0x0000ff00,       4,    0x4000 }
*/
    ,{NULL,            0,         0x00000000,      0,    0x0      }
};



/*extern*/ SMEM_REGISTER_DEFAULT_VALUE_STC sip5_only_registersDefaultValueArr[] =
{
     /*9 MSBits of 'Addr', 23 LSBits of 'Addr',     val,    repeat,    skip*/
     {&STRING_FOR_UNIT_NAME(UNIT_MG),             0x00040000,         0x14a00000,      2,    0x2000   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_MG),             0x0004000c,         0x0000002c,      2,    0x2000   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_EGF_EFT),      0x00002300,         0x0002fc7f,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TTI),            0x00000114,         0x000fffff,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_DQ),         0x0000100c,         0x00000200,      1,    0x0      }

    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),         0x00006000,         0x00b7aaa0,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),         0x00006100,         0x80000a00,     74,    0x4      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),         0x00006500,         0x8000000a,      1,    0x0      }

    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x000000d0,         0xffff0000,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000454,         0x00000001,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000458,         0x00000002,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x0000045c,         0x00000003,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000460,         0x00000004,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000464,         0x00000005,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000468,         0x00000006,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x0000046c,         0x00000007,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000470,         0x00000008,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000474,         0x00000009,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000478,         0x0000000a,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x0000047c,         0x0000000b,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000480,         0x0000000c,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000484,         0x0000000d,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000488,         0x0000000e,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x0000048c,         0x0000000f,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000490,         0x00000010,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000494,         0x00000011,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000498,         0x00000012,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x0000049c,         0x00000013,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x000004a0,         0x00000014,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x000004a4,         0x00000015,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x000004a8,         0x00000016,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x000004ac,         0x00000017,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x000004b0,         0x00000018,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x000004b4,         0x00000019,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x000004b8,         0x0000001a,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x000004bc,         0x0000001b,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x000004c0,         0x0000001c,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x000004c4,         0x0000001d,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x000004c8,         0x0000001e,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x000004cc,         0x0000001f,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x000004d0,         0x00000020,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x000004d4,         0x00000021,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x000004d8,         0x00000022,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x000004dc,         0x00000023,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x000004e0,         0x00000024,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x000004e4,         0x00000025,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x000004e8,         0x00000026,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x000004ec,         0x00000027,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x000004f0,         0x00000028,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x000004f4,         0x00000029,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x000004f8,         0x0000002a,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x000004fc,         0x0000002b,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000500,         0x0000002c,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000504,         0x0000002d,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000508,         0x0000002e,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x0000050c,         0x0000002f,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000510,         0x00000030,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000514,         0x00000031,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000518,         0x00000032,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x0000051c,         0x00000033,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000520,         0x00000034,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000524,         0x00000035,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000528,         0x00000036,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x0000052c,         0x00000037,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000530,         0x00000038,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000534,         0x00000039,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000538,         0x0000003a,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x0000053c,         0x0000003b,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000540,         0x0000003c,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000544,         0x0000003d,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000548,         0x0000003e,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x0000054c,         0x0000003f,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000550,         0x00000040,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000554,         0x00000041,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000558,         0x00000042,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x0000055c,         0x00000043,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000560,         0x00000044,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000564,         0x00000045,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000568,         0x00000046,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x0000056c,         0x00000047,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000570,         0x00000048,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000304,         0x001fff00,     73,    0x4      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000600,         0x00000010,     73,    0x4      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00001200,         0x00008021,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00001204,         0x00000005,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00001340,         0x00000040,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00001364,         0x00003fff,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x000014b0,         0x06036cdb,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x000014b4,         0x000e53cb,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00001600,         0x00000015,     73,    0x4      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002500,         0x00004049,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002504,         0x00000100,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002508,         0x80808080,     85,    0x4      }

    ,DEFAULT_REG_FOR_LMS_UNIT_MAC(                0x00004020,         0x00003f60,      1,    0x0    )
    ,DEFAULT_REG_FOR_LMS_UNIT_MAC(                0x01004020,         0x00003f60,      1,    0x0    )
    ,DEFAULT_REG_FOR_LMS_UNIT_MAC(                0x02004020,         0x00003f60,      1,    0x0    )
    ,DEFAULT_REG_FOR_LMS_UNIT_MAC(                0x03004020,         0x00003f60,      1,    0x0    )

    ,DEFAULT_REG_FOR_LMS_UNIT_MAC(                0x00004030,         0x128398a4,      1,    0x0    )
    ,DEFAULT_REG_FOR_LMS_UNIT_MAC(                0x01004030,         0x1ee6b16a,      1,    0x0    )
    ,DEFAULT_REG_FOR_LMS_UNIT_MAC(                0x02004030,         0x128398a4,      1,    0x0    )
    ,DEFAULT_REG_FOR_LMS_UNIT_MAC(                0x03004030,         0x1ee6b16a,      1,    0x0    )

    ,DEFAULT_REG_FOR_LMS_UNIT_MAC(                0x00004034,         0x00000140,      1,    0x0    )
    ,DEFAULT_REG_FOR_LMS_UNIT_MAC(                0x01004034,         0x000001c0,      1,    0x0    )
    ,DEFAULT_REG_FOR_LMS_UNIT_MAC(                0x02004034,         0x00000140,      1,    0x0    )
    ,DEFAULT_REG_FOR_LMS_UNIT_MAC(                0x03004034,         0x000001c0,      1,    0x0    )

    ,DEFAULT_REG_FOR_LMS_UNIT_MAC(                0x00004054,         0xffffffff,      1,    0x0    )
    ,DEFAULT_REG_FOR_LMS_UNIT_MAC(                0x02004054,         0xffffffff,      1,    0x0    )

    ,DEFAULT_REG_FOR_LMS_UNIT_MAC(                0x00004100,         0x4109ff00,      1,    0x0    )
    ,DEFAULT_REG_FOR_LMS_UNIT_MAC(                0x01004100,         0x00010000,      1,    0x0    )
    ,DEFAULT_REG_FOR_LMS_UNIT_MAC(                0x02004100,         0x4109ff00,      1,    0x0    )
    ,DEFAULT_REG_FOR_LMS_UNIT_MAC(                0x03004100,         0x00010000,      1,    0x0    )

    ,DEFAULT_REG_FOR_LMS_UNIT_MAC(                0x00004104,         0xffffffff,      1,    0x0    )
    ,DEFAULT_REG_FOR_LMS_UNIT_MAC(                0x01004104,         0xffffffff,      1,    0x0    )
    ,DEFAULT_REG_FOR_LMS_UNIT_MAC(                0x02004104,         0xffffffff,      1,    0x0    )
    ,DEFAULT_REG_FOR_LMS_UNIT_MAC(                0x03004104,         0xffffffff,      1,    0x0    )

    ,DEFAULT_REG_FOR_LMS_UNIT_MAC(                0x00004140,         0x000ccc00,      1,    0x0    )
    ,DEFAULT_REG_FOR_LMS_UNIT_MAC(                0x01004140,         0x00051e00,      1,    0x0    )
    ,DEFAULT_REG_FOR_LMS_UNIT_MAC(                0x02004140,         0x00014700,      1,    0x0    )
    ,DEFAULT_REG_FOR_LMS_UNIT_MAC(                0x03004140,         0x0000cc00,      1,    0x0    )

    ,DEFAULT_REG_FOR_LMS_UNIT_MAC(                0x00004144,         0x000061a8,      1,    0x0    )
    ,DEFAULT_REG_FOR_LMS_UNIT_MAC(                0x01004144,         0x0000000f,      1,    0x0    )
    ,DEFAULT_REG_FOR_LMS_UNIT_MAC(                0x03004144,         0x0000000f,      1,    0x0    )

    ,DEFAULT_REG_FOR_LMS_UNIT_MAC(                0x00004200,         0x00168000,      1,    0x0    )
    ,DEFAULT_REG_FOR_LMS_UNIT_MAC(                0x02004200,         0x00168000,      1,    0x0    )

    ,DEFAULT_REG_FOR_LMS_UNIT_MAC(                0x00005100,         0x00000001,      1,    0x0    )
    ,DEFAULT_REG_FOR_LMS_UNIT_MAC(                0x02005100,         0x00000001,      1,    0x0    )

    ,DEFAULT_REG_FOR_LMS_UNIT_MAC(                0x01005104,         0x00168000,      1,    0x0    )
    ,DEFAULT_REG_FOR_LMS_UNIT_MAC(                0x03005104,         0x00168000,      1,    0x0    )

    ,{NULL,            0,         0x00000000,      0,    0x0      }
};

/*extern*/ SMEM_REGISTER_DEFAULT_VALUE_STC sip5_10_registersDefaultValueArr[] =
{
     /*9 MSBits of 'Addr', 23 LSBits of 'Addr',     val,    repeat,    skip*/
     {&STRING_FOR_UNIT_NAME(UNIT_MG),             0x0000005c,         0x20000007,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_MG),             0x00000064,         0x800f0f00,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_MG),             0x00000104,         0x0f0f0f0f,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_MG),             0x00000108,         0x00000f0f,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_MG),             0x0000027c,         0x0000d249,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_MG),             0x000002c0,         0x00000007,      3,    0x4      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_MG),             0x00000324,         0x00000006,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_MG),             0x00000b78,         0x00000f0f,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_MG),             0x00030000,         0x14a00000,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_MG),             0x0003000c,         0x0000002c,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TTI),            0x0000001c,         0x0000000e,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TTI),            0x0000003c,         0x0000000d,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TTI),            0x00000144,         0x24924924,      2,    0x4      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_L2I),            0x00000020,         0x22023924,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_L2I),            0x00000024,         0x00000900,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_L2I),            0x00000164,         0x00000000,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_L2I),            0x00001204,         0x18027027,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_EQ),             0x0000b200,         0x02000000,      7,    0x4      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_HA) ,            0x00000004,         0x00001003,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_DQ),         0x00000000,         0x00000141,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TXQ_QCN),        0x00000200,         0x00000100,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_CPFC),           0x00001110,         0x00002860,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_CPFC),           0x00014000,         0x0007ffff,      8,    0x4,     2,    0x400       }
    ,{&STRING_FOR_UNIT_NAME(UNIT_EPCL),           0x000000a0,         0x00000022,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_EPCL),           0x0000001c,         0x00000008,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000000,         0x3fff3fff,      8,    0x8      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000040,         0x00003fff,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x000000b0,         0x3fffffff,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x000000b4,         0x00000001,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000350,         0x00000800,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000374,         0x00008600,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000398,         0x00008100,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x0000039c,         0x00008a88,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x000003a8,         0x00008847,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x000003ac,         0x00008848,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x000005d0,         0xffff0000,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000804,         0x0ffff000,     73,    0x4      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000954,         0x00000001,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000958,         0x00000002,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x0000095c,         0x00000003,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000960,         0x00000004,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000964,         0x00000005,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000968,         0x00000006,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x0000096c,         0x00000007,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000970,         0x00000008,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000974,         0x00000009,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000978,         0x0000000a,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x0000097c,         0x0000000b,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000980,         0x0000000c,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000984,         0x0000000d,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000988,         0x0000000e,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x0000098c,         0x0000000f,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000990,         0x00000010,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000994,         0x00000011,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000998,         0x00000012,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x0000099c,         0x00000013,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x000009a0,         0x00000014,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x000009a4,         0x00000015,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x000009a8,         0x00000016,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x000009ac,         0x00000017,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x000009b0,         0x00000018,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x000009b4,         0x00000019,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x000009b8,         0x0000001a,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x000009bc,         0x0000001b,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x000009c0,         0x0000001c,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x000009c4,         0x0000001d,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x000009c8,         0x0000001e,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x000009cc,         0x0000001f,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x000009d0,         0x00000020,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x000009d4,         0x00000021,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x000009d8,         0x00000022,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x000009dc,         0x00000023,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x000009e0,         0x00000024,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x000009e4,         0x00000025,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x000009e8,         0x00000026,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x000009ec,         0x00000027,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x000009f0,         0x00000028,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x000009f4,         0x00000029,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x000009f8,         0x0000002a,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x000009fc,         0x0000002b,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000a00,         0x0000002c,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000a04,         0x0000002d,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000a08,         0x0000002e,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000a0c,         0x0000002f,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000a10,         0x00000030,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000a14,         0x00000031,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000a18,         0x00000032,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000a1c,         0x00000033,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000a20,         0x00000034,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000a24,         0x00000035,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000a28,         0x00000036,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000a2c,         0x00000037,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000a30,         0x00000038,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000a34,         0x00000039,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000a38,         0x0000003a,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000a3c,         0x0000003b,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000a40,         0x0000003c,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000a44,         0x0000003d,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000a48,         0x0000003e,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000a4c,         0x0000003f,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000a50,         0x00000040,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000a54,         0x00000041,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000a58,         0x00000042,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000a5c,         0x00000043,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000a60,         0x00000044,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000a64,         0x00000045,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000a68,         0x00000046,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000a6c,         0x00000047,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000a70,         0x00000048,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00000b00,         0x00000010,     73,    0x4      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00001700,         0x00008021,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00001704,         0x00080001,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00001840,         0x00000080,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00001850,         0xaaaaaaaa,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00001858,         0x0000001f,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00001864,         0x00003fff,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00001868,         0x00000002,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x000019b0,         0x0601e114,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x000019b4,         0x003896cb,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00001b00,         0x00000015,     73,    0x4      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002a00,         0x00004049,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002a04,         0x00000004,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002a08,         0x83828180,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002a0c,         0x87868584,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002a10,         0x8b8a8988,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002a14,         0x8f8e8d8c,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002a18,         0x93929190,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002a1c,         0x97969594,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002a20,         0x9b9a9998,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002a24,         0x9f9e9d9c,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002a28,         0xa3a2a1a0,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002a2c,         0xa7a6a5a4,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002a30,         0xabaaa9a8,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002a34,         0xafaeadac,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002a38,         0xb3b2b1b0,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002a3c,         0xb7b6b5b4,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002a40,         0xbbbab9b8,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002a44,         0xbfbebdbc,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002a48,         0xc3c2c1c0,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002a4c,         0xc7c6c5c4,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002a50,         0x828180c8,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002a54,         0x86858483,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002a58,         0x8a898887,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002a5c,         0x8e8d8c8b,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002a60,         0x9291908f,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002a64,         0x96959493,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002a68,         0x9a999897,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002a6c,         0x9e9d9c9b,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002a70,         0xa2a1a09f,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002a74,         0xa6a5a4a3,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002a78,         0xaaa9a8a7,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002a7c,         0xaeadacab,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002a80,         0xb2b1b0af,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002a84,         0xb6b5b4b3,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002a88,         0xbab9b8b7,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002a8c,         0xbebdbcbb,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002a90,         0xc2c1c0bf,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002a94,         0xc6c5c4c3,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002a98,         0x8180c8c7,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002a9c,         0x85848382,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002aa0,         0x89888786,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002aa4,         0x8d8c8b8a,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002aa8,         0x91908f8e,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002aac,         0x95949392,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002ab0,         0x99989796,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002ab4,         0x9d9c9b9a,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002ab8,         0xa1a09f9e,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002abc,         0xa5a4a3a2,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002ac0,         0xa9a8a7a6,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002ac4,         0xadacabaa,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002ac8,         0xb1b0afae,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002acc,         0xb5b4b3b2,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002ad0,         0xb9b8b7b6,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002ad4,         0xbdbcbbba,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002ad8,         0xc1c0bfbe,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002adc,         0xc5c4c3c2,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002ae0,         0x80c8c7c6,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002ae4,         0x84838281,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002ae8,         0x88878685,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002aec,         0x8c8b8a89,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002af0,         0x908f8e8d,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002af4,         0x94939291,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002af8,         0x98979695,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002afc,         0x9c9b9a99,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002b00,         0xa09f9e9d,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002b04,         0xa4a3a2a1,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002b08,         0xa8a7a6a5,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002b0c,         0xacabaaa9,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002b10,         0xb0afaead,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002b14,         0xb4b3b2b1,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002b18,         0xb8b7b6b5,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002b1c,         0xbcbbbab9,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002b20,         0xc0bfbebd,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002b24,         0xc4c3c2c1,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002b28,         0xc8c7c6c5,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002b2c,         0x83828180,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002b30,         0x87868584,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002b34,         0x8b8a8988,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002b38,         0x8f8e8d8c,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002b3c,         0x93929190,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002b40,         0x97969594,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002b44,         0x9b9a9998,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002b48,         0x9f9e9d9c,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002b4c,         0xa3a2a1a0,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002b50,         0xa7a6a5a4,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002b54,         0xabaaa9a8,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00002b58,         0xafaeadac,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00003a00,         0x00000001,     73,    0x4      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x000040c0,         0x00000001,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_BM),             0x00000400,         0x00000008,      4,    0x4      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_BM),             0x000004c0,         0x0000f0f0,      2,    0x10     }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),         0x00000200,         0x22222222,      8,    0x4      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),         0x00000004,         0x000bfd02,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),         0x00001004,         0x000000f5,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),         0x00001008,         0x00000007,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_DMA),         0x0000100c,         0x0003d4f8,      2,    0x4      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),        0x00003004,         0x00000bac,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_TX_FIFO),        0x00003300,         0x00025540,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_EGF_SHT_12),     0x00020000,         0x00004007,      1,    0x0      }

                         /*address,    value,    numOfRepetitions,    stepSizeBeteenRepetitions*/
    ,DEFAULT_REG_FOR_SMI_UNIT_MAC( 0x00000000,         0x04000000,      1,    0x0       )
    ,DEFAULT_REG_FOR_SMI_UNIT_MAC( 0x00000004,         0x00000614,      1,    0x0       )
    ,DEFAULT_REG_FOR_SMI_UNIT_MAC( 0x00000008,         0x00010000,      1,    0x0       )
    ,DEFAULT_REG_FOR_SMI_UNIT_MAC( 0x00000010,         0x00000001,      1,    0x0       )
    ,DEFAULT_REG_FOR_SMI_UNIT_MAC( 0x00000014,         0x00000002,      1,    0x0       )
    ,DEFAULT_REG_FOR_SMI_UNIT_MAC( 0x00000018,         0x00000003,      1,    0x0       )
    ,DEFAULT_REG_FOR_SMI_UNIT_MAC( 0x0000001c,         0x00000004,      1,    0x0       )
    ,DEFAULT_REG_FOR_SMI_UNIT_MAC( 0x00000020,         0x00000005,      1,    0x0       )
    ,DEFAULT_REG_FOR_SMI_UNIT_MAC( 0x00000024,         0x00000006,      1,    0x0       )
    ,DEFAULT_REG_FOR_SMI_UNIT_MAC( 0x00000028,         0x00000007,      1,    0x0       )
    ,DEFAULT_REG_FOR_SMI_UNIT_MAC( 0x0000002c,         0x00000008,      1,    0x0       )
    ,DEFAULT_REG_FOR_SMI_UNIT_MAC( 0x00000030,         0x00000009,      1,    0x0       )
    ,DEFAULT_REG_FOR_SMI_UNIT_MAC( 0x00000034,         0x0000000a,      1,    0x0       )
    ,DEFAULT_REG_FOR_SMI_UNIT_MAC( 0x00000038,         0x0000000b,      1,    0x0       )
    ,DEFAULT_REG_FOR_SMI_UNIT_MAC( 0x0000003c,         0x0000000c,      1,    0x0       )
    ,DEFAULT_REG_FOR_SMI_UNIT_MAC( 0x00000040,         0x0000000d,      1,    0x0       )
    ,DEFAULT_REG_FOR_SMI_UNIT_MAC( 0x00000044,         0x0000000e,      1,    0x0       )
    ,DEFAULT_REG_FOR_SMI_UNIT_MAC( 0x00000048,         0x0000000f,      1,    0x0       )
                         /*address,    value,    numOfRepetitions,    stepSizeBeteenRepetitions*/
    ,DEFAULT_REG_FOR_LED_UNIT_MAC( 0x00000000,         0x00ff0037,      1,    0x0       )
    ,DEFAULT_REG_FOR_LED_UNIT_MAC( 0x00000004,         0x00000129,      1,    0x0       )
    ,DEFAULT_REG_FOR_LED_UNIT_MAC( 0x00000008,         0x00000f00,      1,    0x0       )
    ,DEFAULT_REG_FOR_LED_UNIT_MAC( 0x00000028,         0x0a418820,      1,    0x0       )
    ,DEFAULT_REG_FOR_LED_UNIT_MAC( 0x00000120,         0x55555555,      1,    0x0       )
    ,DEFAULT_REG_FOR_LED_UNIT_MAC( 0x00000128,         0x00ffffff,      1,    0x0       )
    ,DEFAULT_REG_FOR_LED_UNIT_MAC( 0x00000160,         0xffff0000,      1,    0x0       )

    ,{NULL,            0,         0x00000000,      0,    0x0      }
};

BUILD_STRING_FOR_UNIT_NAME(DFX_EXT_SERVER);

#define  XSB_DDR_ADDR_MAC(instanceOffset , addrOffset) \
                ((instanceOffset) + (addrOffset))

#define REG_DEFAULT___XSB_XBAR_DUNIT__INSTANCE_MAC(instanceOffset)   \
      {&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER), XSB_DDR_ADDR_MAC(instanceOffset , 0x00001030),         0x00000004}  \
     ,{&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER), XSB_DDR_ADDR_MAC(instanceOffset , 0x00001034),         0x00338000}  \
     ,{&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER), XSB_DDR_ADDR_MAC(instanceOffset , 0x000010f8),         0x000000ff}  \
     ,{&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER), XSB_DDR_ADDR_MAC(instanceOffset , 0x000010fc),         0x0000ffff}  \
     ,{&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER), XSB_DDR_ADDR_MAC(instanceOffset , 0x00001400),         0x7b008400}  \
     ,{&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER), XSB_DDR_ADDR_MAC(instanceOffset , 0x00001404),         0x3630b800}  \
     ,{&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER), XSB_DDR_ADDR_MAC(instanceOffset , 0x00001408),         0x1101333b}  \
     ,{&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER), XSB_DDR_ADDR_MAC(instanceOffset , 0x0000140c),         0x36000034}  \
     ,{&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER), XSB_DDR_ADDR_MAC(instanceOffset , 0x00001410),         0x11004444}  \
     ,{&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER), XSB_DDR_ADDR_MAC(instanceOffset , 0x00001414),         0x00000300}  \
     ,{&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER), XSB_DDR_ADDR_MAC(instanceOffset , 0x0000141c),         0x00000642}  \
     ,{&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER), XSB_DDR_ADDR_MAC(instanceOffset , 0x00001424),         0x0160fa7f}  \
     ,{&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER), XSB_DDR_ADDR_MAC(instanceOffset , 0x00001428),         0x00074410}  \
     ,{&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER), XSB_DDR_ADDR_MAC(instanceOffset , 0x0000142c),         0x000c4f38}  \
     ,{&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER), XSB_DDR_ADDR_MAC(instanceOffset , 0x00001430),         0x76543210}  \
     ,{&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER), XSB_DDR_ADDR_MAC(instanceOffset , 0x00001434),         0xfedcba98}  \
     ,{&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER), XSB_DDR_ADDR_MAC(instanceOffset , 0x00001438),         0x000100ff}  \
     ,{&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER), XSB_DDR_ADDR_MAC(instanceOffset , 0x00001470),         0x00013204}  \
     ,{&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER), XSB_DDR_ADDR_MAC(instanceOffset , 0x00001474),         0x0000030c}  \
     ,{&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER), XSB_DDR_ADDR_MAC(instanceOffset , 0x0000147c),         0x00007441}  \
     ,{&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER), XSB_DDR_ADDR_MAC(instanceOffset , 0x00001488),         0x00000542}  \
     ,{&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER), XSB_DDR_ADDR_MAC(instanceOffset , 0x000014a0),         0x00000001}  \
     ,{&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER), XSB_DDR_ADDR_MAC(instanceOffset , 0x000014b8),         0x00006f67}  \
     ,{&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER), XSB_DDR_ADDR_MAC(instanceOffset , 0x000014c0),         0x192434e9}  \
     ,{&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER), XSB_DDR_ADDR_MAC(instanceOffset , 0x000014c4),         0x092434e9}  \
     ,{&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER), XSB_DDR_ADDR_MAC(instanceOffset , 0x000014c8),         0x05140001}  \
     ,{&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER), XSB_DDR_ADDR_MAC(instanceOffset , 0x000014cc),         0x3d090008}  \
     ,{&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER), XSB_DDR_ADDR_MAC(instanceOffset , 0x00001504),         0x0ffffff1}  \
     ,{&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER), XSB_DDR_ADDR_MAC(instanceOffset , 0x0000150c),         0x0ffffff5}  \
     ,{&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER), XSB_DDR_ADDR_MAC(instanceOffset , 0x00001514),         0x0ffffff9}  \
     ,{&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER), XSB_DDR_ADDR_MAC(instanceOffset , 0x0000151c),         0x0ffffffd}  \
     ,{&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER), XSB_DDR_ADDR_MAC(instanceOffset , 0x00001520),         0x0b4012c0}  \
     ,{&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER), XSB_DDR_ADDR_MAC(instanceOffset , 0x00001524),         0x00000800}  \
     ,{&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER), XSB_DDR_ADDR_MAC(instanceOffset , 0x00001528),         0x000302f1}  \
     ,{&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER), XSB_DDR_ADDR_MAC(instanceOffset , 0x00001590),         0xfffc7f7f}  \
     ,{&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER), XSB_DDR_ADDR_MAC(instanceOffset , 0x000015b0),         0x41000000}  \
     ,{&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER), XSB_DDR_ADDR_MAC(instanceOffset , 0x000015b4),         0xf80000ff}  \
     ,{&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER), XSB_DDR_ADDR_MAC(instanceOffset , 0x000015b8),         0x3400000c}  \
     ,{&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER), XSB_DDR_ADDR_MAC(instanceOffset , 0x000015c0),         0x80000000}  \
     ,{&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER), XSB_DDR_ADDR_MAC(instanceOffset , 0x000015c4),         0x06910000}  \
     ,{&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER), XSB_DDR_ADDR_MAC(instanceOffset , 0x000015c8),         0xfedb6924}  \
     ,{&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER), XSB_DDR_ADDR_MAC(instanceOffset , 0x000015d0),         0x00000610}  \
     ,{&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER), XSB_DDR_ADDR_MAC(instanceOffset , 0x000015e0),         0x00000001}  \
     ,{&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER), XSB_DDR_ADDR_MAC(instanceOffset , 0x000015e4),         0x00203c18}  \
     ,{&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER), XSB_DDR_ADDR_MAC(instanceOffset , 0x000015ec),         0xf800000b}  \
     ,{&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER), XSB_DDR_ADDR_MAC(instanceOffset , 0x00001670),         0x03ff0fff}  \
     ,{&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER), XSB_DDR_ADDR_MAC(instanceOffset , 0x000016ac),         0x0009ff00}  \
     ,{&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER), XSB_DDR_ADDR_MAC(instanceOffset , 0x000016cc),         0x00000001}  \
     ,{&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER), XSB_DDR_ADDR_MAC(instanceOffset , 0x000016d0),         0x8421f116}  \
     ,{&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER), XSB_DDR_ADDR_MAC(instanceOffset , 0x000016d4),         0x000812c0}  \
     ,{&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER), XSB_DDR_ADDR_MAC(instanceOffset , 0x000016d8),         0x00000040}  \
     ,{&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER), XSB_DDR_ADDR_MAC(instanceOffset , 0x00001870),         0x00000610}

static SMEM_REGISTER_DEFAULT_VALUE_STC registersDfxDefaultValueArr[] =
{     {&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER),          0x00000000,         0x0000004c,      1,    0x0                        }
     ,{&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER),          0x00000014,         0x00000160,      1,    0x0                        }
     ,{&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER),          0x000f8004,         0x0102002b,      1,    0x0                        }
     ,{&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER),          0x000f8008,         0x0ffb2c30,      1,    0x0                        }
     ,{&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER),          0x000f8010,         0x0000000f,      1,    0x0                        }
     ,{&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER),          0x000f8014,         0xfffffff3,      1,    0x0                        }
     ,{&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER),          0x000f8024,         0x000001ff,      3,    0x4                        }
     ,{&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER),          0x000f8034,         0x000001ff,      2,    0x4                        }
     ,{&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER),          0x000f8070,         0x01064044,      1,    0x0                        }
     ,{&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER),          0x000f8074,         0x63414819,      1,    0x0                        }
     ,{&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER),          0x000f807c,         0x8011e214,      1,    0x0                        }
     ,{&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER),          0x000f8080,         0x30a38019,      1,    0x0                        }
     ,{&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER),          0x000f8120,         0x0008007f,      1,    0x0                        }
     ,{&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER),          0x000f8244,         0x00000357,      1,    0x0                        }
     ,{&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER),          0x000f8248,         0x0000ffff,      1,    0x0                        }
     ,{&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER),          0x000f82d0,         0xffffffff,      1,    0x0                        }
     ,{&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER),          0x000f82e0,         0x34353637,     12,    0x8                        }
     ,{&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER),          0x000f82e4,         0x0000000a,     12,    0x8                        }
     ,{&STRING_FOR_UNIT_NAME(DFX_EXT_SERVER),          0x000f8340,         0x000013fc,     32,    0x4                        }

     /* DDR 0 */
     ,REG_DEFAULT___XSB_XBAR_DUNIT__INSTANCE_MAC(0x00000)
     /* DDR 1 */
     ,REG_DEFAULT___XSB_XBAR_DUNIT__INSTANCE_MAC(0x28000)
     /* DDR 2 */
     ,REG_DEFAULT___XSB_XBAR_DUNIT__INSTANCE_MAC(0x48000)
     /* DDR 3 */
     ,REG_DEFAULT___XSB_XBAR_DUNIT__INSTANCE_MAC(0x70000)
     /* DDR 4 */
     ,REG_DEFAULT___XSB_XBAR_DUNIT__INSTANCE_MAC(0xB8000)


     ,{NULL,                                           0,                     0x00000000,                          0,    0x0                        }
};

BUILD_STRING_FOR_UNIT_NAME(PEX_MEMORY_SPACE);
/* default values of the PEX register.
   the engine that init the 'default registers' will add the  (0x70000)

*/
static SMEM_REGISTER_DEFAULT_VALUE_STC registersPexDefaultValueArr[] =
{
     {&STRING_FOR_UNIT_NAME(PEX_MEMORY_SPACE),     0x00000000,         0x000011ab,      1,    0x0              }
    ,{&STRING_FOR_UNIT_NAME(PEX_MEMORY_SPACE),     0x00000004,         0x00100000,      1,    0x0              }
    ,{&STRING_FOR_UNIT_NAME(PEX_MEMORY_SPACE),     0x00000008,         0x02000000,      1,    0x0              }
    ,{&STRING_FOR_UNIT_NAME(PEX_MEMORY_SPACE),     0x00000010,         0xd000000c,      1,    0x0              }
    ,{&STRING_FOR_UNIT_NAME(PEX_MEMORY_SPACE),     0x00000018,         0x0000000c,      1,    0x0              }
    ,{&STRING_FOR_UNIT_NAME(PEX_MEMORY_SPACE),     0x00000020,         0xf000000c,      1,    0x0              }
    ,{&STRING_FOR_UNIT_NAME(PEX_MEMORY_SPACE),     0x0000002c,         0x11ab11ab,      1,    0x0              }
    ,{&STRING_FOR_UNIT_NAME(PEX_MEMORY_SPACE),     0x00000034,         0x00000040,      1,    0x0              }
    ,{&STRING_FOR_UNIT_NAME(PEX_MEMORY_SPACE),     0x0000003c,         0x00000100,      1,    0x0              }
    ,{&STRING_FOR_UNIT_NAME(PEX_MEMORY_SPACE),     0x00000040,         0x06035001,      1,    0x0              }
    ,{&STRING_FOR_UNIT_NAME(PEX_MEMORY_SPACE),     0x00000050,         0x00806005,      1,    0x0              }
    ,{&STRING_FOR_UNIT_NAME(PEX_MEMORY_SPACE),     0x00000060,         0x00420010,      1,    0x0              }
    ,{&STRING_FOR_UNIT_NAME(PEX_MEMORY_SPACE),     0x00000064,         0x00008080,      1,    0x0              }
    ,{&STRING_FOR_UNIT_NAME(PEX_MEMORY_SPACE),     0x00000068,         0x00002000,      1,    0x0              }
    ,{&STRING_FOR_UNIT_NAME(PEX_MEMORY_SPACE),     0x0000006c,         0x0007ac00,      1,    0x0              }
    ,{&STRING_FOR_UNIT_NAME(PEX_MEMORY_SPACE),     0x00000070,         0x10010000,      1,    0x0              }
    ,{&STRING_FOR_UNIT_NAME(PEX_MEMORY_SPACE),     0x00000100,         0x00010001,      1,    0x0              }
    ,{&STRING_FOR_UNIT_NAME(PEX_MEMORY_SPACE),     0x0000010c,         0x00060010,      1,    0x0              }
    ,{&STRING_FOR_UNIT_NAME(PEX_MEMORY_SPACE),     0x00000114,         0x00002000,      1,    0x0              }
    ,{&STRING_FOR_UNIT_NAME(PEX_MEMORY_SPACE),     0x00001804,         0x03ff0001,      1,    0x0              }
    ,{&STRING_FOR_UNIT_NAME(PEX_MEMORY_SPACE),     0x00001808,         0x007f0000,      1,    0x0              }
    ,{&STRING_FOR_UNIT_NAME(PEX_MEMORY_SPACE),     0x00001820,         0x0fff0e01,      1,    0x0              }
    ,{&STRING_FOR_UNIT_NAME(PEX_MEMORY_SPACE),     0x00001830,         0x0fff0d01,      1,    0x0              }
    ,{&STRING_FOR_UNIT_NAME(PEX_MEMORY_SPACE),     0x00001834,         0x10000000,      1,    0x0              }
    ,{&STRING_FOR_UNIT_NAME(PEX_MEMORY_SPACE),     0x00001840,         0x0fff0b01,      1,    0x0              }
    ,{&STRING_FOR_UNIT_NAME(PEX_MEMORY_SPACE),     0x00001844,         0x20000000,      1,    0x0              }
    ,{&STRING_FOR_UNIT_NAME(PEX_MEMORY_SPACE),     0x00001850,         0x0fff0701,      1,    0x0              }
    ,{&STRING_FOR_UNIT_NAME(PEX_MEMORY_SPACE),     0x00001854,         0x30000000,      1,    0x0              }
    ,{&STRING_FOR_UNIT_NAME(PEX_MEMORY_SPACE),     0x00001860,         0x07ff3b13,      1,    0x0              }
    ,{&STRING_FOR_UNIT_NAME(PEX_MEMORY_SPACE),     0x00001864,         0xf0000000,      1,    0x0              }
    ,{&STRING_FOR_UNIT_NAME(PEX_MEMORY_SPACE),     0x00001880,         0x07ff2f13,      1,    0x0              }
    ,{&STRING_FOR_UNIT_NAME(PEX_MEMORY_SPACE),     0x00001884,         0xf8000000,      1,    0x0              }
    ,{&STRING_FOR_UNIT_NAME(PEX_MEMORY_SPACE),     0x000018c0,         0x00002f10,      1,    0x0              }
    ,{&STRING_FOR_UNIT_NAME(PEX_MEMORY_SPACE),     0x000018d0,         0x00180000,      1,    0x0              }
    ,{&STRING_FOR_UNIT_NAME(PEX_MEMORY_SPACE),     0x000018e0,         0x76543210,      1,    0x0              }
    ,{&STRING_FOR_UNIT_NAME(PEX_MEMORY_SPACE),     0x000018e4,         0xfedcba98,      1,    0x0              }
    ,{&STRING_FOR_UNIT_NAME(PEX_MEMORY_SPACE),     0x000018e8,         0x000100ff,      1,    0x0              }
    ,{&STRING_FOR_UNIT_NAME(PEX_MEMORY_SPACE),     0x00001a00,         0x4014002f,      1,    0x0              }
    ,{&STRING_FOR_UNIT_NAME(PEX_MEMORY_SPACE),     0x00001a08,         0xffff0007,      1,    0x0              }
    ,{&STRING_FOR_UNIT_NAME(PEX_MEMORY_SPACE),     0x00001a10,         0x00002710,      1,    0x0              }
    ,{&STRING_FOR_UNIT_NAME(PEX_MEMORY_SPACE),     0x00001a18,         0x00000002,      1,    0x0              }
    ,{&STRING_FOR_UNIT_NAME(PEX_MEMORY_SPACE),     0x00001a50,         0x0000000a,      1,    0x0              }
    ,{&STRING_FOR_UNIT_NAME(PEX_MEMORY_SPACE),     0x00001a60,         0x0f6bf0c0,      1,    0x0              }
    ,{&STRING_FOR_UNIT_NAME(PEX_MEMORY_SPACE),     0x00001a80,         0x08010005,      1,    0x0              }
    ,{&STRING_FOR_UNIT_NAME(PEX_MEMORY_SPACE),     0x00001aa4,         0x0fff0000,      1,    0x0              }
    ,{&STRING_FOR_UNIT_NAME(PEX_MEMORY_SPACE),     0x00001aa8,         0x1fff0000,      1,    0x0              }
    ,{&STRING_FOR_UNIT_NAME(PEX_MEMORY_SPACE),     0x00001ab8,         0x000a2a2a,      1,    0x0              }
    ,{&STRING_FOR_UNIT_NAME(PEX_MEMORY_SPACE),     0x00001abc,         0x00000004,      1,    0x0              }
    /* must be last */
     ,{NULL,                                           0,                     0x00000000,                          0,    0x0                        }
};

/* additional registers that are not in Lion3 */
static SMEM_REGISTER_DEFAULT_VALUE_STC lion3Additional_registersDefaultValueArr[] =
{
     {&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x00000000,         0x00008be5,     12,    0x1000   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x00000004,         0x00000003,     12,    0x1000   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x00000008,         0x0000c048,     12,    0x1000   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x0000000c,         0x0000bae8,     12,    0x1000   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x00000014,         0x000008d2,     12,    0x1000   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x0000002c,         0x0000000c,     12,    0x1000   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x00000048,         0x00002300,     12,    0x1000   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x0003f000,         0x00008be5,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x0003f004,         0x00000003,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x0003f008,         0x0000c048,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x0003f00c,         0x0000bae8,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x0003f014,         0x000008d2,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x0003f048,         0x00000300,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x000c0000,         0x00001201,     12,    0x1000   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x000c0004,         0x000002f9,     12,    0x1000   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x000c0008,         0x00001c00,     12,    0x1000   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x000c001c,         0x00002000,     12,    0x1000   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x000c0030,         0x0000000c,     12,    0x1000   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x000c0084,         0x00000010,     12,    0x1000   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x000c0410,         0x00003fff,     12,    0x1000   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x000c0414,         0x00003040,     12,    0x1000   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x000c0474,         0x00001800,     12,    0x1000   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x000c0488,         0x0000b8c1,     12,    0x1000   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x000c048c,         0x00006f89,     12,    0x1000   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x000c0490,         0x0000473e,     12,    0x1000   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x000c0494,         0x00009076,     12,    0x1000   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x000c0498,         0x0000199c,     12,    0x1000   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x000c049c,         0x00000f3b,     12,    0x1000   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x000c04a0,         0x0000e663,     12,    0x1000   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x000c04a4,         0x0000f0c4,     12,    0x1000   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x000c04a8,         0x000064cc,     12,    0x1000   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x000c04ac,         0x00003a9a,     12,    0x1000   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x000c04b0,         0x00009b33,     12,    0x1000   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x000c04b4,         0x0000c565,     12,    0x1000   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x000c04b8,         0x0000c227,     12,    0x1000   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x000c04bc,         0x00005d86,     12,    0x1000   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x000c04c0,         0x00003dd8,     12,    0x1000   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x000c04c4,         0x0000a279,     12,    0x1000   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x000c04d4,         0x0000ffff,     12,    0x1000   }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x000ce000,         0x00001201,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x000ce004,         0x000002f9,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x000ce008,         0x00001c00,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x000ce01c,         0x00002000,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x000ce030,         0x0000000c,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x000ce084,         0x00000010,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x000ce410,         0x00003fff,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x000ce414,         0x00003040,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x000ce474,         0x00001800,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x000ce488,         0x0000b8c1,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x000ce48c,         0x00006f89,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x000ce490,         0x0000473e,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x000ce494,         0x00009076,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x000ce498,         0x0000199c,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x000ce49c,         0x00000f3b,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x000ce4a0,         0x0000e663,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x000ce4a4,         0x0000f0c4,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x000ce4a8,         0x000064cc,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x000ce4ac,         0x00003a9a,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x000ce4b0,         0x00009b33,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x000ce4b4,         0x0000c565,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x000ce4b8,         0x0000c227,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x000ce4bc,         0x00005d86,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x000ce4c0,         0x00003dd8,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x000ce4c4,         0x0000a279,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x000ce4d4,         0x0000ffff,      1,    0x0      }

    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x00000000,         0x00000c80,     24,    0x400    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x00000004,         0x00000008,     24,    0x400    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x0000020c,         0x0000000f,     24,    0x400    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x00000210,         0x00000005,     24,    0x400    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x00000214,         0x0000003c,     24,    0x400    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x00000218,         0x00001004,     24,    0x400    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x0000021c,         0x00000520,     24,    0x400    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x00000228,         0x00000030,     24,    0x400    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x0000022c,         0x00000660,     24,    0x400    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x00000230,         0x00004000,     24,    0x400    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x00000234,         0x00000003,     24,    0x400    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x00000240,         0x00003087,     24,    0x400    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x00000244,         0x000000f0,     24,    0x400    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x00000248,         0x0000000f,     24,    0x400    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x0000024c,         0x00000400,     24,    0x400    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x00000250,         0x00008060,     24,    0x400    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x00000254,         0x00005503,     24,    0x400    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x0000025c,         0x00000015,     24,    0x400    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x00000260,         0x00004600,     24,    0x400    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x00000278,         0x00000013,     24,    0x400    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x0000027c,         0x000000a5,     24,    0x400    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x00000280,         0x00004800,     24,    0x400    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x00000284,         0x00000600,     24,    0x400    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x0000028c,         0x00000240,     24,    0x400    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x00000294,         0x0000000a,     24,    0x400    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x00000338,         0x00000f6a,     24,    0x400    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x0000033c,         0x000000cc,     24,    0x400    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x00000340,         0x000052ea,     24,    0x400    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x00000344,         0x000000cc,     24,    0x400    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x00000348,         0x0000562a,     24,    0x400    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x0000034c,         0x000000cc,     24,    0x400    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x00000354,         0x000000cc,     24,    0x400    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x00000358,         0x00000080,     24,    0x400    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x0000035c,         0x00004000,     24,    0x400    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x00000360,         0x00000081,     24,    0x400    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x00000364,         0x00005555,     24,    0x400    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x00000368,         0x0000f640,     24,    0x400    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x0000036c,         0x00008000,     24,    0x400    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x000003ac,         0x00000030,     24,    0x400    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x000003b0,         0x00001000,     24,    0x400    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x000003c0,         0x00005502,     24,    0x400    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x000003cc,         0x00002800,     24,    0x400    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x000003e0,         0x00000080,     24,    0x400    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x000003f0,         0x0000cccc,     24,    0x400    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x000003f4,         0x0000cc00,     24,    0x400    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x000003f8,         0x00008000,     24,    0x400    }

    ,{&STRING_FOR_UNIT_NAME(UNIT_BM),             0x00000030,         0x3fff0000,      2,    0x4    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_BM),             0x00000420,         0x00031490,      4,    0x4    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_BM),             0x00000440,         0x000024c9,      4,    0x4    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_BM),             0x000004a0,         0x00080000,      2,    0x4    }

    /*;;;;;;;;;;;; manually added ;;;;;;;;;;;;;;;;;;;;;;;;;*/

/*  for SERDESs use values like in Lion2 simulation*/
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x00000000,         0x00000001,     24,    0x400    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x00000018,         0x0000001c,     24,    0x400    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),            0x0000033c,         0x00000017,     24,    0x400    }


    /* must be last */
     ,{NULL,                                           0,                     0x00000000,                          0,    0x0                        }
};

static SMEM_LINK_LIST_REGISTER_DEFAULT_VALUE_STC  element2_RegistersDefaults =
    {sip5_gop_tai_registersDefaultValueArr,NULL};

static SMEM_LINK_LIST_REGISTER_DEFAULT_VALUE_STC  element1_RegistersDefaults =
    {lion3Additional_registersDefaultValueArr,&element2_RegistersDefaults};

static SMEM_LINK_LIST_REGISTER_DEFAULT_VALUE_STC  element0_RegistersDefaults =
    {sip5_registersDefaultValueArr , &element1_RegistersDefaults};

static SMEM_LINK_LIST_REGISTER_DEFAULT_VALUE_STC  element0_DfxDefault_RegistersDefaults =
    {registersDfxDefaultValueArr , NULL};


static SMEM_LINK_LIST_REGISTER_DEFAULT_VALUE_STC  element0_PexDefault_RegistersDefaults =
    {registersPexDefaultValueArr , NULL};


#define LION3_GOP_PORT_MASK_CNS  0xFFFF0FFF
#define LION3_GOP_TRI_PORT_MASK_CNS  0xFFFC0FFF

/* Active memory table */
static SMEM_ACTIVE_MEM_ENTRY_STC smemLion3PortGroupActiveTable[] =
{
/*MG*/
    /* Global interrupt cause register */
    {LION3_MG_UNIT_BASE_ADDR_CNS + 0x00000030, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteIntrCauseReg, 0},

    /* Receive SDMA Interrupt Cause Register (RxSDMAInt) */
    {LION3_MG_UNIT_BASE_ADDR_CNS + 0x0000280C, SMEM_FULL_MASK_CNS,
        smemChtActiveReadIntrCauseReg, 22, smemChtActiveWriteIntrCauseReg, 0},
    /* Receive SDMA Interrupt Mask Register */
    {LION3_MG_UNIT_BASE_ADDR_CNS + 0x00002814, SMEM_FULL_MASK_CNS,
        NULL, 0, smemChtActiveWriteRcvSdmaInterruptsMaskReg, 0},

    /* The SDMA packet count registers */
    {LION3_MG_UNIT_BASE_ADDR_CNS + 0x00002820, 0xFFFFFFF0, smemChtActiveReadCntrs, 0, NULL,0},
    {LION3_MG_UNIT_BASE_ADDR_CNS + 0x00002830, 0xFFFFFFF0, smemChtActiveReadCntrs, 0, NULL,0},

     /* The SDMA byte count registers */
    {LION3_MG_UNIT_BASE_ADDR_CNS + 0x00002840, 0xFFFFFFF0, smemChtActiveReadCntrs, 0, NULL,0},
    {LION3_MG_UNIT_BASE_ADDR_CNS + 0x00002850, 0xFFFFFFF0, smemChtActiveReadCntrs, 0, NULL,0},

    /* The SDMA Receive SDMA Resource Error Count_And_Mode 0x00002860, 0x00002864 */
    {LION3_MG_UNIT_BASE_ADDR_CNS + 0x00002860, 0xFFFFFFF8, smemLion3SdmaRxResourceErrorCountAndModeActiveRead, 0, NULL,0},

    /* The SDMA Receive SDMA Resource Error Count_And_Mode 0x00002878.., 0x0000288c*/
    {LION3_MG_UNIT_BASE_ADDR_CNS + 0x00002878, 0xFFFFFFF8, smemLion3SdmaRxResourceErrorCountAndModeActiveRead, 0, NULL,0},
    {LION3_MG_UNIT_BASE_ADDR_CNS + 0x00002880, 0xFFFFFFF0, smemLion3SdmaRxResourceErrorCountAndModeActiveRead, 0, NULL,0},

    /* Transmit SDMA Packet Generator Config Queue */
    {LION3_MG_UNIT_BASE_ADDR_CNS + 0x000028B0, 0xFFFFFFF0,
        NULL, 0, smemLion3ActiveWriteTransSdmaPacketGeneratorConfigQueueReg, 0},

    /* Transmit SDMA Packet Generator Config Queue */
    {LION3_MG_UNIT_BASE_ADDR_CNS + 0x000028C0, 0xFFFFFFF0,
        NULL, 0, smemLion3ActiveWriteTransSdmaPacketGeneratorConfigQueueReg, 4/*indicates that first queue is 4*/},

    /* Tx SDMA Packet Count Config Queue */
    {LION3_MG_UNIT_BASE_ADDR_CNS + 0x000028D0, 0xFFFFFFF0,
        NULL, 0, smemLion3ActiveWriteTransSdmaPacketCountConfigQueueReg, 0},

    /* Tx SDMA Packet Count Config Queue */
    {LION3_MG_UNIT_BASE_ADDR_CNS + 0x000028E0, 0xFFFFFFF0,
        NULL, 0, smemLion3ActiveWriteTransSdmaPacketCountConfigQueueReg, 0},

    /* Address Update Queue Base Address */
    {LION3_MG_UNIT_BASE_ADDR_CNS + 0x000000C0, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveAuqBaseWrite, 0},

    /* FDB Upload Queue Base Address */
    {LION3_MG_UNIT_BASE_ADDR_CNS + 0x000000C8, SMEM_FULL_MASK_CNS, NULL, 0 , smemCht2ActiveFuqBaseWrite, 0},

    /* AUQHostConfiguration */
    {LION3_MG_UNIT_BASE_ADDR_CNS + 0x000000D8, SMEM_FULL_MASK_CNS, NULL, 0 , smemLion3ActiveAuqHostCfgWrite, 0},

    /* Receive SDMA Queue Command */
    {LION3_MG_UNIT_BASE_ADDR_CNS + 0x00002680, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteSdmaCommand,0},
    /* Transmit SDMA Queue Command Register */
    {LION3_MG_UNIT_BASE_ADDR_CNS + 0x00002868, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteStartFromCpuDma, 0},

    /* Global control register */
    {LION3_MG_UNIT_BASE_ADDR_CNS + 0x00000058, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteGlobalReg, 0},

    /* Transmit SDMA Interrupt Cause Register */
    {LION3_MG_UNIT_BASE_ADDR_CNS + 0x00002810, SMEM_FULL_MASK_CNS,
        smemChtActiveReadIntrCauseReg, 21 , smemChtActiveWriteIntrCauseReg,0},
    /* Transmit SDMA Interrupt Mask Register */
    {LION3_MG_UNIT_BASE_ADDR_CNS + 0x00002818, SMEM_FULL_MASK_CNS,
        NULL, 0, smemChtActiveWriteTransSdmaInterruptsMaskReg, 0},

    /* read interrupts cause registers Misc -- ROC register */
    {LION3_MG_UNIT_BASE_ADDR_CNS + 0x00000038, SMEM_FULL_MASK_CNS,
        smemChtActiveReadIntrCauseReg, 11, smemChtActiveWriteIntrCauseReg, 0},

    /* Write Interrupt Mask MIsc Register */
    {LION3_MG_UNIT_BASE_ADDR_CNS + 0x0000003c, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteMiscInterruptsMaskReg, 0},

    /* SDMA configuration register - 0x00002800 */
    {LION3_MG_UNIT_BASE_ADDR_CNS + 0x00002800, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteSdmaConfigReg, 0},

    /* XSMI Management Register */
    {LION3_MG_UNIT_BASE_ADDR_CNS + 0x00040000, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteXSmii, 0},
    /* XSMI1 Management Register */
    {LION3_MG_UNIT_BASE_ADDR_CNS + 0x00042000, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteXSmii, 0},

    /* CPU Port MIB counters registers */
    {LION3_MG_UNIT_BASE_ADDR_CNS + 0x00000060, 0xFFFFFFE0, smemChtActiveReadCntrs, 0, NULL,0},
/*GOP*/
    /* Port<n> Auto-Negotiation Configuration Register */
    {LION3_GOP_UNIT_BASE_ADDR_CNS + 0x0000000C, LION3_GOP_TRI_PORT_MASK_CNS, NULL, 0 , smemChtActiveWriteForceLinkDown, 0},
    /* Port<n> Interrupt Cause Register  */
    {LION3_GOP_UNIT_BASE_ADDR_CNS + 0x00000020, LION3_GOP_TRI_PORT_MASK_CNS, smemChtActiveReadIntrCauseReg, 18, smemChtActiveWriteIntrCauseReg, 0},
    /* Tri-Speed Port<n> Interrupt Mask Register */
    {LION3_GOP_UNIT_BASE_ADDR_CNS + 0x00000024, LION3_GOP_TRI_PORT_MASK_CNS, NULL, 0, smemChtActiveWritePortInterruptsMaskReg, 0},
    /* Port MAC Control Register0 */
    {LION3_GOP_UNIT_BASE_ADDR_CNS + 0x000C0000, LION3_GOP_PORT_MASK_CNS, NULL, 0 , smemChtActiveWriteForceLinkDownXg, 0},

    /* XG Port<n> Interrupt Cause Register  */
    {LION3_GOP_UNIT_BASE_ADDR_CNS + 0x000C0014, LION3_GOP_PORT_MASK_CNS,  smemChtActiveReadIntrCauseReg, 29, smemChtActiveWriteIntrCauseReg, 0},
    /* XG Port<n> Interrupt Mask Register */
    {LION3_GOP_UNIT_BASE_ADDR_CNS + 0x000C0018, LION3_GOP_PORT_MASK_CNS, NULL, 0, smemChtActiveWritePortInterruptsMaskReg, 0},
    /* stack gig ports - Port<n> Interrupt Cause Register  */
    {LION3_GOP_UNIT_BASE_ADDR_CNS + 0x000C0020, LION3_GOP_PORT_MASK_CNS,smemChtActiveReadIntrCauseReg, 18, smemChtActiveWriteIntrCauseReg, 0},
    /* stack gig ports - Tri-Speed Port<n> Interrupt Mask Register */
    {LION3_GOP_UNIT_BASE_ADDR_CNS + 0x000C0024, LION3_GOP_PORT_MASK_CNS, NULL, 0, smemChtActiveWritePortInterruptsMaskReg, 0},
    /* stack gig ports - Port<n> Auto-Negotiation Configuration Register */
    {LION3_GOP_UNIT_BASE_ADDR_CNS + 0x000C000C, LION3_GOP_PORT_MASK_CNS, NULL, 0 , smemChtActiveWriteForceLinkDown, 0},
    /*Port MAC Control Register3*/
    {LION3_GOP_UNIT_BASE_ADDR_CNS + 0x000C001C, LION3_GOP_PORT_MASK_CNS, NULL, 0 , smemXcatActiveWriteMacModeSelect, 0},

/*L2I*/
    /* Bridge Interrupt Cause Register */
    {LION3_L2I_UNIT_BASE_ADDR_CNS + 0x00002100, SMEM_FULL_MASK_CNS,
        smemChtActiveReadIntrCauseReg, 14, smemChtActiveWriteIntrCauseReg, 0},
    /* Bridge Interrupt Mask Register */
    {LION3_L2I_UNIT_BASE_ADDR_CNS + 0x00002104, SMEM_FULL_MASK_CNS,
        NULL, 0, smemChtActiveWriteBridgeInterruptsMaskReg, 0},

    /* Security Breach Status Register3 */
    {LION3_L2I_UNIT_BASE_ADDR_CNS + 0x0004140C, SMEM_FULL_MASK_CNS, smemChtActiveReadBreachStatusReg, 0, NULL, 0},

    /*hostIncomingPktsCount;               */
    {LION3_L2I_UNIT_BASE_ADDR_CNS + 0x00001510, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},
    /*hostOutgoingPktsCount;               */
    {LION3_L2I_UNIT_BASE_ADDR_CNS + 0x00001514, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},
    /*hostOutgoingMcPktCount;              */
    {LION3_L2I_UNIT_BASE_ADDR_CNS + 0x00001520, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},
    /*hostOutgoingBcPktCount;              */
    {LION3_L2I_UNIT_BASE_ADDR_CNS + 0x00001524, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},
    /*matrixSourceDestinationPktCount;     */
    {LION3_L2I_UNIT_BASE_ADDR_CNS + 0x00001528, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},
    /*setVLANIngrFilteredPktCount[n]*/
    {LION3_L2I_UNIT_BASE_ADDR_CNS + 0x0000153c, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},
    {LION3_L2I_UNIT_BASE_ADDR_CNS + 0x0000155c, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},
    /*setIncomingPktCount[n]        */
    {LION3_L2I_UNIT_BASE_ADDR_CNS + 0x00001538, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},
    {LION3_L2I_UNIT_BASE_ADDR_CNS + 0x00001558, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},
    /*setSecurityFilteredPktCount[n]*/
    {LION3_L2I_UNIT_BASE_ADDR_CNS + 0x00001540, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},
    {LION3_L2I_UNIT_BASE_ADDR_CNS + 0x00001560, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},
    /*setBridgeFilteredPktCount[n]  */
    {LION3_L2I_UNIT_BASE_ADDR_CNS + 0x00001544, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},
    {LION3_L2I_UNIT_BASE_ADDR_CNS + 0x00001564, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},

    /****************/
    /* START OF FDB */
    /****************/
/*FDB*/
    /* Message to CPU register  */
    {LION3_FDB_UNIT_BASE_ADDR_CNS + 0x00000090, SMEM_FULL_MASK_CNS, smemChtActiveReadMsg2Cpu, 0 , NULL,0},
    /* Mac Table Access Control Register */
    {LION3_FDB_UNIT_BASE_ADDR_CNS + 0x00000130, SMEM_FULL_MASK_CNS, NULL, 0 , smemLion3ActiveWriteFdbMsg,0},
    /* Message From CPU Management */
    {LION3_FDB_UNIT_BASE_ADDR_CNS + 0x000000c0, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteNewFdbMsgFromCpu, 0},

    /* MAC Table Action general Register */
    {LION3_FDB_UNIT_BASE_ADDR_CNS + 0x00000020, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteFdbActionTrigger, 0},
    /* FDB Global Configuration 1 register */
    {LION3_FDB_UNIT_BASE_ADDR_CNS + 0x00000004, SMEM_FULL_MASK_CNS, NULL, 0 , smemLion3ActiveWriteFDBGlobalCfg1Reg, 0},

    {LION3_FDB_UNIT_BASE_ADDR_CNS + 0x000001b0, SMEM_FULL_MASK_CNS,
        smemChtActiveReadIntrCauseReg, 17, smemChtActiveWriteIntrCauseReg, 0},
    /* MAC Table Interrupt Mask Register */
    {LION3_FDB_UNIT_BASE_ADDR_CNS + 0x000001b4, SMEM_FULL_MASK_CNS,
        NULL, 0, smemChtActiveWriteMacInterruptsMaskReg, 0},

    /* FDB Counters Update Control */
    {LION3_FDB_UNIT_BASE_ADDR_CNS + 0x00000340, SMEM_FULL_MASK_CNS, NULL, 0, smemLion3ActiveWriteFDBCountersUpdateControlReg,0},
    /* FDB Counters Update Control */
    {LION3_FDB_UNIT_BASE_ADDR_CNS + 0x00000280, SMEM_FULL_MASK_CNS, NULL, 0, smemLion3ActiveWriteFDBCountersControlReg,0},

    /**************/
    /* END OF FDB */
    /**************/
/*MIB*/
    /* XG port MAC MIB Counters */
    {LION3_MIB_UNIT_BASE_ADDR_CNS + 0x00000000, SMEM_LION2_XG_MIB_COUNT_MSK_CNS, smemLion2ActiveReadMsmMibCounters, 0, NULL,0},
/*SERDES*/
    /* COMPHY_H %t Registers/KVCO Calibration Control (0x09800200 + t*0x400: where t (0-23) represents SERDES) */
    {LION3_SERDES_UNIT_BASE_ADDR_CNS + 0x00000208, 0xFFFF03FF, NULL, 0, smemLion2ActiveWriteKVCOCalibrationControlReg, 0},
/*TXQ_QUEUE*/
    /* The Egress packet count registers */
    {LION3_TXQ_QUEUE_UNIT_BASE_ADDR_CNS + 0x00093210, 0xFFFFFFF0, smemChtActiveReadCntrs, 0, NULL,0},
    {LION3_TXQ_QUEUE_UNIT_BASE_ADDR_CNS + 0x00093220, 0xFFFFFFF0, smemChtActiveReadCntrs, 0, NULL,0},
    {LION3_TXQ_QUEUE_UNIT_BASE_ADDR_CNS + 0x00093230, 0xFFFFFFF0, smemChtActiveReadCntrs, 0, NULL,0},
    {LION3_TXQ_QUEUE_UNIT_BASE_ADDR_CNS + 0x00093240, 0xFFFFFFF0, smemChtActiveReadCntrs, 0, NULL,0},
    {LION3_TXQ_QUEUE_UNIT_BASE_ADDR_CNS + 0x00093250, 0xFFFFFFF0, smemChtActiveReadCntrs, 0, NULL,0},
    {LION3_TXQ_QUEUE_UNIT_BASE_ADDR_CNS + 0x00093260, 0xFFFFFFF0, smemChtActiveReadCntrs, 0, NULL,0},
    {LION3_TXQ_QUEUE_UNIT_BASE_ADDR_CNS + 0x00093270, 0xFFFFFFF0, smemChtActiveReadCntrs, 0, NULL,0},
    {LION3_TXQ_QUEUE_UNIT_BASE_ADDR_CNS + 0x00093280, 0xFFFFFFF0, smemChtActiveReadCntrs, 0, NULL,0},

/*policer*/
    /* Policers Table Access Control Register */
    POLICER_ACTIVE_WRITE_MEMORY(0x00000070, smemXCatActiveWritePolicerTbl),

    /* Policer IPFIX memories */
    POLICER_ACTIVE_WRITE_MEMORY(0x00000014, smemXCatActiveWriteIPFixTimeStamp),
    POLICER_ACTIVE_READ_MEMORY(0x00000318, smemXCatActiveReadIPFixNanoTimeStamp),  /*part of Policer Timer Memory */
    POLICER_ACTIVE_READ_MEMORY(0x0000031C, smemXCatActiveReadIPFixSecLsbTimeStamp),/*part of Policer Timer Memory */
    POLICER_ACTIVE_READ_MEMORY(0x00000320, smemXCatActiveReadIPFixSecMsbTimeStamp),/*part of Policer Timer Memory */
    POLICER_ACTIVE_READ_MEMORY(0x00000048, smemXCatActiveReadIPFixSampleLog),
    POLICER_ACTIVE_READ_MEMORY(0x0000004C, smemXCatActiveReadIPFixSampleLog),

    /* Policer : when Read a Data Unit counter part(Entry LSB) of Management Counters Entry.
                 then value of LSB and MSB copied to Shadow registers */
    POLICER_ACTIVE_READ_MEMORY(POLICER_MANAGEMENT_COUNTER_ADDR_CNS, smemXCatActiveReadPolicerManagementCounters),
    {LION3_IPLR0_UNIT_BASE_ADDR_CNS + 0x0000002C, SMEM_FULL_MASK_CNS, NULL, 0 , smemLion3ActiveWriteHierarchicalPolicerControl,  0},

    /* iplr0 policer table */
    {LION3_IPLR0_UNIT_BASE_ADDR_CNS + 0x00100000, 0xFFFC0000, smemXCatActiveReadIplr0Tables, 0 , smemXCatActiveWriteIplr0Tables,  0},
    /* iplr0 policerCounters table */
    {LION3_IPLR0_UNIT_BASE_ADDR_CNS + 0x00140000, 0xFFFC0000, smemXCatActiveReadIplr0Tables, 0 , smemXCatActiveWriteIplr0Tables,  0},

    /* iplr1 policer table */
    {LION3_IPLR1_UNIT_BASE_ADDR_CNS + 0x00100000, 0xFFFC0000, smemXCatActiveReadIplr1Tables, 0 , smemXCatActiveWriteIplr1Tables,  0},
    /* iplr1 policerCounters table */
    {LION3_IPLR1_UNIT_BASE_ADDR_CNS + 0x00140000, 0xFFFC0000, smemXCatActiveReadIplr1Tables, 0 , smemXCatActiveWriteIplr1Tables,  0},


    /***************/
    /* START OF EQ */
    /***************/
/*EQ*/
    /*ingress STC interrupt register*/
    /* need to support 0x0B000620 - 16 registers in steps of 0x10
       and cuase register in  0x0B000600

       AND 0x0B000610 --> not exists in reg file

       so 0x0B000600 , 0x0B000620 ...0x0B000720
    */
    /* 0x0B000600 .. 0x0B0006F0 , steps of 0x10 */
    {LION3_EQ_UNIT_BASE_ADDR_CNS(0) + 0x00000600, 0xFFFFFF0F, smemChtActiveReadIntrCauseReg, 24 , smemChtActiveWriteIntrCauseReg,0},
    /* 0x0B000700 .. 0x0B000710 , steps of 0x10 */
    {LION3_EQ_UNIT_BASE_ADDR_CNS(0) + 0x00000700, 0xFFFFFFEF, smemChtActiveReadIntrCauseReg, 24 , smemChtActiveWriteIntrCauseReg,0},
        /* SMEM_BIND_TABLE_MAC(ingrStc)
           the table is not of type 'write' after 'whole' words :
           word 0 - RW
           word 1 - RO
           word 2 - RW

           the Write to the table word 0,2 is allowed via
            <IngressSTCW0 WrEn> , <IngressSTCW2 WrEn> in SMEM_CHT_INGRESS_STC_CONF_REG(dev)

            addresses are : 0x0B040000 .. 0x0B040FFC
         */
    {LION3_EQ_UNIT_BASE_ADDR_CNS(0) + 0x00040000, 0xFFFFF000 ,NULL,0,smemLion3ActiveWriteIngressStcTable,0},
    /*Trunk LFSR Configuration*/
    {LION3_EQ_UNIT_BASE_ADDR_CNS(0) + 0x0000D000,SMEM_FULL_MASK_CNS,NULL,0,smemLion3ActiveWriteLfsrConfig,0/*trunk*/},
    /*ePort ECMP LFSR Configuration*/
    {LION3_EQ_UNIT_BASE_ADDR_CNS(0) + 0x00005020,SMEM_FULL_MASK_CNS,NULL,0,smemLion3ActiveWriteLfsrConfig,1/*eport ECMP*/},

    /* ROC (read only clear) counters */
    /*Ingress Drop Counter*/
    {LION3_EQ_UNIT_BASE_ADDR_CNS(0) +    0x40    , SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},
    /*CPU Code Rate Limiter Drop Counter*/
    {LION3_EQ_UNIT_BASE_ADDR_CNS(0) +    0x68    , SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},
    /*Drop Codes Counter*/
    {LION3_EQ_UNIT_BASE_ADDR_CNS(0) +    0x70    , SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},
    /*BM Clear Critical ECC Error Counter*/
    {LION3_EQ_UNIT_BASE_ADDR_CNS(0) +    0x78    , SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},
    /*BM Inc Critical ECC Error Counter*/
    {LION3_EQ_UNIT_BASE_ADDR_CNS(0) +    0x88    , SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},
    /*Target ePort MTU Exceeded Counter*/
    {LION3_EQ_UNIT_BASE_ADDR_CNS(0) +  0xa024    , SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},
    /*Ingress Forwarding Restrictions Dropped Packets Counter*/
    {LION3_EQ_UNIT_BASE_ADDR_CNS(0) + 0x2000c    , SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},

    /* read only counters -- without clear ! */
    /*TO CPU Rate Limiter<%n+1> Packet Counter --> 255 registers , but for simple mask do '256' */
    {LION3_EQ_UNIT_BASE_ADDR_CNS(0) + 0x80000    , SMEM_FULL_MASK_CNS & (~(0xFF << 2)), NULL, 0, smemChtActiveWriteToReadOnlyReg,0},

    /* oamProtectionLocStatusTable */
    {LION3_EQ_UNIT_BASE_ADDR_CNS(0) + 0x00B00000, 0xFFFFFF00, NULL, 0, smemLion3ActiveWriteProtectionLocStatusTable, 0},

    /*************/
    /* END OF EQ */
    /*************/
/*CNC_0 ,CNC_1 */
    /* CNC Fast Dump Trigger Register Register */
    CNC_ACTIVE_READ_WRITE_MEMORY(0x00000030,SMEM_FULL_MASK_CNS, NULL, 0 , smemCht3ActiveWriteCncFastDumpTrigger, 0),

    /* CPU direct read from the counters */
    CNC_ACTIVE_READ_WRITE_MEMORY(0x00010000,0xFFFF0000, smemCht3ActiveCncBlockRead, 0x00010000/*base address of block 0*/, NULL, 0),
    CNC_ACTIVE_READ_WRITE_MEMORY(0x00020000,0xFFFF0000, smemCht3ActiveCncBlockRead, 0x00010000/*base address of block 0*/, NULL, 0),

    /* CNC Block Wraparound Status Register */
    CNC_ACTIVE_READ_WRITE_MEMORY(0x00001400,0xFFFFFE00, smemCht3ActiveCncWrapAroundStatusRead, 0, NULL, 0),

    /* read interrupts cause registers CNC -- ROC register */
    CNC_ACTIVE_READ_WRITE_MEMORY(0x00000100,SMEM_FULL_MASK_CNS,smemChtActiveReadIntrCauseReg, 3, smemChtActiveWriteIntrCauseReg, 0),

    /* Write Interrupt Mask CNC Register */
    CNC_ACTIVE_READ_WRITE_MEMORY(0x00000104,SMEM_FULL_MASK_CNS, NULL, 0 , smemCht3ActiveWriteCncInterruptsMaskReg, 0),

/*GTS*/
    /* TOD Counter - Nanoseconds */
    GTS_ACTIVE_READ_MEMORY(0x10, smemLionActiveReadPtpTodNanoSeconds),
    /* TOD Counter - Seconds word 0)*/
    GTS_ACTIVE_READ_MEMORY(0x14, smemLionActiveReadPtpTodSeconds),
    /* TOD Counter - Seconds word 1*/
    GTS_ACTIVE_READ_MEMORY(0x18, smemLionActiveReadPtpTodSeconds),
    /* Global FIFO Current Entry [31:0] register reading*/
    GTS_ACTIVE_READ_MEMORY(0x30, smemLionActiveReadPtpMessage),
    /* Global FIFO Current Entry [63:32] register reading */
    GTS_ACTIVE_READ_MEMORY(0x34, smemLionActiveReadPtpMessage),
    /* Global Configurations */
    GTS_ACTIVE_WRITE_MEMORY(0x0, smemLionActiveWriteTodGlobalReg),
/* LMS */
    /* SMI0 Management Register */
    LION3_LMS_ACTIVE_WRITE_MEMORY(0x00004054, smemChtActiveWriteSmi),
    /* SMI2 Management Register */
    LION3_LMS_ACTIVE_WRITE_MEMORY(0x02004054, smemChtActiveWriteSmi),

/*UNIT_TXQ_DQ*/
    /* Scheduler Configuration Register */
    {LION3_TXQ_DQ_UNIT_BASE_ADDR_CNS + 0x00001000, SMEM_FULL_MASK_CNS, NULL, 0 , smemLionActiveWriteSchedulerConfigReg, 0},

    /* Port<%n> Dequeue Enable Register ... 0..63 (64*4 - 1) = 255 */
    {LION3_TXQ_DQ_UNIT_BASE_ADDR_CNS + 0x00004300, 0xFFFFFF00, NULL, 0, smemLionActiveWriteDequeueEnableReg, 0/*the start port from 0x00*/},
    /* Port<%n> Dequeue Enable Register ... 64..71 (8*4 - 1) = 31 */
    {LION3_TXQ_DQ_UNIT_BASE_ADDR_CNS + 0x00004400, 0xFFFFFFE0, NULL, 0, smemLionActiveWriteDequeueEnableReg, 64/*the start port from 0x00*/},

    /*Egress STC interrupt register*/
    {LION3_TXQ_DQ_UNIT_BASE_ADDR_CNS + 0x00000630, SMEM_FULL_MASK_CNS, smemChtActiveReadIntrCauseReg, 6 , smemChtActiveWriteIntrCauseReg,0},

    /* extra Lion-2 active memories */
    /******************/
    /* start MLL unit */
    /******************/
/*MLL*/
    /* MLL counters */
    /*L2MLLValidProcessedEntriesCntr[0]*/
    {LION3_MLL_UNIT_BASE_ADDR_CNS + 0xe20       , SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},
    /*L2MLLValidProcessedEntriesCntr[1]*/
    {LION3_MLL_UNIT_BASE_ADDR_CNS + 0xe24       , SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},
    /*L2MLLValidProcessedEntriesCntr[2]*/
    {LION3_MLL_UNIT_BASE_ADDR_CNS + 0xe28       , SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},
    /*L2MLLSilentDropCntr*/
    {LION3_MLL_UNIT_BASE_ADDR_CNS + 0x00000e00  , SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},
    /*L2MLLSkippedEntriesCntr*/
    {LION3_MLL_UNIT_BASE_ADDR_CNS + 0x00000e04  , SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},
    /*L2MLLTTLExceptionCntr*/
    {LION3_MLL_UNIT_BASE_ADDR_CNS + 0x00000e08  , SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},
    /*L2MLLOutMcPktsCntr[0]*/
    {LION3_MLL_UNIT_BASE_ADDR_CNS + 0xc00       , SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},
    /*L2MLLOutMcPktsCntr[1]*/
    {LION3_MLL_UNIT_BASE_ADDR_CNS + 0xd00       , SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},
    /*MLLSilentDropCntr*/
    {LION3_MLL_UNIT_BASE_ADDR_CNS + 0x00000800  , SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},
    /*MLLMCFIFODropCntr*/
    {LION3_MLL_UNIT_BASE_ADDR_CNS + 0x00000804  , SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},
    /*IPMLLSkippedEntriesCntr*/
    {LION3_MLL_UNIT_BASE_ADDR_CNS + 0x00000b04  , SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},
    /*IPMLLOutMcPktsCntr[0]*/
    {LION3_MLL_UNIT_BASE_ADDR_CNS + 0x900       , SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},
    /*IPMLLOutMcPktsCntr[1]*/
    {LION3_MLL_UNIT_BASE_ADDR_CNS + 0xa00       , SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},

    /****************/
    /* end MLL unit */
    /****************/
/*OAM*/
    /* OAM Table Access Control */
    OAM_ACTIVE_WRITE_MEMORY(0x000000D0, smemLion3ActiveWriteOamTbl),

    /* Aging Table */
    OAM_ACTIVE_READ_MEMORY(0x00000c00, smemLion3ActiveReadExceptionSummaryBitmap),
    /* Meg Exception Table */
    OAM_ACTIVE_READ_MEMORY(0x00010000, smemLion3ActiveReadExceptionSummaryBitmap),
    /* Source Interface Exception Table */
    OAM_ACTIVE_READ_MEMORY(0x00018000, smemLion3ActiveReadExceptionSummaryBitmap),
    /* Invalid Keepalive Hash Exception Table */
    OAM_ACTIVE_READ_MEMORY(0x00020000, smemLion3ActiveReadExceptionSummaryBitmap),
    /* Excess Keepalive Exception Table */
    OAM_ACTIVE_READ_MEMORY(0x00028000, smemLion3ActiveReadExceptionSummaryBitmap),
    /* OAM Exception Summary Table */
    OAM_ACTIVE_READ_MEMORY(0x00030000, smemLion3ActiveReadExceptionSummaryBitmap),
    /* RDI Status Change Exception Table */
    OAM_ACTIVE_READ_MEMORY(0x00038000, smemLion3ActiveReadExceptionSummaryBitmap),
    /* Tx Period Exception Table */
    OAM_ACTIVE_READ_MEMORY(0x00040000, smemLion3ActiveReadExceptionSummaryBitmap),

    /* Meg Level Exception Counter */
    OAM_ACTIVE_READ_MEMORY(0x00000300, smemLion3ActiveReadExceptionCounter),
    /* Source Interface Mismatch Counter */
    OAM_ACTIVE_READ_MEMORY(0x00000304, smemLion3ActiveReadExceptionCounter),
    /* Invalid Keepalive Hash Counter */
    OAM_ACTIVE_READ_MEMORY(0x00000308, smemLion3ActiveReadExceptionCounter),
    /* Excess Keepalive Counter */
    OAM_ACTIVE_READ_MEMORY(0x0000030c, smemLion3ActiveReadExceptionCounter),
    /* Keepalive Aging Counter */
    OAM_ACTIVE_READ_MEMORY(0x00000310, smemLion3ActiveReadExceptionCounter),
    /* RDI Status Change Counter */
    OAM_ACTIVE_READ_MEMORY(0x00000314, smemLion3ActiveReadExceptionCounter),
    /* Tx Period Exception Counter */
    OAM_ACTIVE_READ_MEMORY(0x00000318, smemLion3ActiveReadExceptionCounter),
    /* Summary Exception Counter */
    OAM_ACTIVE_READ_MEMORY(0x0000031c, smemLion3ActiveReadExceptionCounter),

    /* Aging Entry Group Status */
    OAM_ACTIVE_READ_MEMORY(0x000000e0, smemLion3ActiveReadExceptionGroupStatus),
    OAM_ACTIVE_READ_MEMORY(0x000000e4, smemLion3ActiveReadExceptionGroupStatus),
    /* Excess Keepalive Group Status */
    OAM_ACTIVE_READ_MEMORY(0x00000200, smemLion3ActiveReadExceptionGroupStatus),
    OAM_ACTIVE_READ_MEMORY(0x00000204, smemLion3ActiveReadExceptionGroupStatus),
    /* Invalid Keepalive Group Status */
    OAM_ACTIVE_READ_MEMORY(0x00000220, smemLion3ActiveReadExceptionGroupStatus),
    OAM_ACTIVE_READ_MEMORY(0x00000224, smemLion3ActiveReadExceptionGroupStatus),
    /* Meg Level Group Status */
    OAM_ACTIVE_READ_MEMORY(0x00000240, smemLion3ActiveReadExceptionGroupStatus),
    OAM_ACTIVE_READ_MEMORY(0x00000244, smemLion3ActiveReadExceptionGroupStatus),
    /* Source Interface Group Status */
    OAM_ACTIVE_READ_MEMORY(0x00000260, smemLion3ActiveReadExceptionGroupStatus),
    OAM_ACTIVE_READ_MEMORY(0x00000264, smemLion3ActiveReadExceptionGroupStatus),
    /* RDI Status Change Group Status */
    OAM_ACTIVE_READ_MEMORY(0x00000280, smemLion3ActiveReadExceptionGroupStatus),
    OAM_ACTIVE_READ_MEMORY(0x00000284, smemLion3ActiveReadExceptionGroupStatus),
    /* Tx Period Exception Group Status */
    OAM_ACTIVE_READ_MEMORY(0x000002A0, smemLion3ActiveReadExceptionGroupStatus),
    OAM_ACTIVE_READ_MEMORY(0x000002A4, smemLion3ActiveReadExceptionGroupStatus),
    /* Summary Exception Status */
    OAM_ACTIVE_READ_MEMORY(0x000002C0, smemLion3ActiveReadExceptionGroupStatus),
    OAM_ACTIVE_READ_MEMORY(0x000002C4, smemLion3ActiveReadExceptionGroupStatus),

/*TCAM*/
    /* SIP5 Tcam management registers */
    {LION3_TCAM_UNIT_BASE_ADDR_CNS + 0x00502000, SMEM_FULL_MASK_CNS, NULL, 0 , smemLion3ActiveWriteTcamMgLookup,0},

    /* next 4 lines catch addresses of : 0x005021BC..0x00502208 (20 registers)
       those are 'read only' registers.  */
    /*exact match - 0x005021BC */
    {LION3_TCAM_UNIT_BASE_ADDR_CNS + 0x005021BC, SMEM_FULL_MASK_CNS, NULL, 0, smemLion3ActiveWriteTcamMgHitGroupHitNum,0},
    /* match - 0x005021C0.. 0x005021FC , so need to ignore 5021E0 , 5021F4 */
    {LION3_TCAM_UNIT_BASE_ADDR_CNS + 0x005021C0, 0xFFFFFFC0, NULL, 0, smemLion3ActiveWriteTcamMgHitGroupHitNum,0},
    /* match - 0x00502200.. 0x0050221C , so need to ignore 502208, 50221C */
    {LION3_TCAM_UNIT_BASE_ADDR_CNS + 0x00502200, 0xFFFFFFE0, NULL,0 , smemLion3ActiveWriteTcamMgHitGroupHitNum,0},

/* TTI */
    /* PTP Exception counter */
    {LION3_TTI_UNIT_BASE_ADDR_CNS + 0x3034, 0xFFFFFFFF, smemLion3ActiveReadExceptionCounter, 0, NULL, 0},
    /* Received Flow Control Packets Counter */
    {LION3_TTI_UNIT_BASE_ADDR_CNS + 0x1d0,  SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL, 0},

/* GOP unit */
    /* TAI subunit */
    /* Time Counter Function Configuration 0 - Function Trigger */
    {LION3_NEW_GOP_UNIT_BASE_ADDR_CNS + 0x00000A00 + 0x10, 0xFF000FFF, NULL, 0 , smemLion3ActiveWriteTodFuncConfReg, 0},

    /* PTP subunit */
    /* PTP Interrupt Cause Register */
    {LION3_NEW_GOP_UNIT_BASE_ADDR_CNS + 0x00000800, 0xFF000FFF,
        smemChtActiveReadIntrCauseReg, 9, smemChtActiveWriteIntrCauseReg, 0},
    /* PTP Interrupt Mask Register */
    {LION3_NEW_GOP_UNIT_BASE_ADDR_CNS + 0x00000800 + 0x04, 0xFF000FFF,
        NULL, 0, smemLion3ActiveWriteGopPtpInterruptsMaskReg, 0},

    /* PTP General Control */
    {LION3_NEW_GOP_UNIT_BASE_ADDR_CNS + 0x00000800 + 0x08, 0xFF000FFF, NULL, 0, smemLion3ActiveWriteGopPtpGeneralCtrlReg, 0},

    /* PTP TX Timestamp Queue0 reg2 */
    {LION3_NEW_GOP_UNIT_BASE_ADDR_CNS + 0x00000800 + 0x14, 0xFF000FFF, smemLion3ActiveReadGopPtpTxTsQueueReg2Reg, 0, NULL, 0},
    /* PTP TX Timestamp Queue1 reg2 */
    {LION3_NEW_GOP_UNIT_BASE_ADDR_CNS + 0x00000800 + 0x20, 0xFF000FFF, smemLion3ActiveReadGopPtpTxTsQueueReg2Reg, 0, NULL, 0},

/* HA */
    /* HA Interrupt Cause Register */
    {LION3_HA_BASE_ADDR_CNS(0) + 0x00000300, SMEM_FULL_MASK_CNS,
        smemChtActiveReadIntrCauseReg, 12, smemChtActiveWriteIntrCauseReg, 0},
    /* HA Interrupt Mask Register */
    {LION3_HA_BASE_ADDR_CNS(0) + 0x00000304, SMEM_FULL_MASK_CNS,
        NULL, 0, smemLion3ActiveWriteHaInterruptsMaskReg, 0},

/* ERMRK */
    /* ERMRK Interrupt Cause Register */
    {LION3_ERMRK_UNIT_BASE_ADDR_CNS + 0x4, SMEM_FULL_MASK_CNS,
        smemChtActiveReadIntrCauseReg, 11, smemChtActiveWriteIntrCauseReg, 0},
    /* ERMRK Interrupt Mask Register */
    {LION3_ERMRK_UNIT_BASE_ADDR_CNS + 0x8, SMEM_FULL_MASK_CNS,
        NULL, 0, smemLion3ActiveWriteErmrkInterruptsMaskReg, 0},

    /* ERMRK PTP Configuration */
    {LION3_ERMRK_UNIT_BASE_ADDR_CNS + 0x10, 0xFFFFFFFF, NULL, 0, smemLion3ActiveWriteErmrkPtpConfReg, 0},

    /* ERMRK Timestamp Queue Clear Entry ID */
    {LION3_ERMRK_UNIT_BASE_ADDR_CNS + 0xE4, 0xFFFFFFFF, NULL, 0, smemLion3ActiveWriteErmrkTsQueueEntryIdClearReg, 0},

    /* ERMRK Timestamp Queue Entry Word1/Word2 */
    {LION3_ERMRK_UNIT_BASE_ADDR_CNS + 0x120, 0xFFFFFFE0, smemLion3ActiveReadErmrkTsQueueEntryWord1Word2Reg, 0, NULL, 0},

    /* must be last anyway */
    SMEM_ACTIVE_MEM_ENTRY_LAST_LINE_CNS
};

/* bind the PEX unit to it's active mem */
static void bindUnitPexActiveMem(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    START_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)

    /* PEX Interrupt Cause Register */
    {0x000F1900, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteIntrCauseReg, 0x00FF1F1F},

    END_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)
}

/* bind the DFX unit to it's active mem */
static void bindUnitDfxActiveMem(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    START_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)

    /* SMEM_LION2_DFX_SERVER_RESET_CONTROL_REG() */
    {0x000f800c, SMEM_FULL_MASK_CNS, NULL, 0 , smemLion3ActiveWriteDfxServerResetControlReg, 0},
    /* SMEM_CHT_MAC_REG_DB_DFX_SERVER_GET(devObjPtr)->resetAndInitCtrller.DFXServerUnitsBC2SpecificRegs.deviceCtrl4 */
    {0x000f8260, SMEM_FULL_MASK_CNS, NULL, 0 , smemLion3ActiveWriteDfxServerDeviceCtrl4Reg, 0},

    END_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)
}



/*******************************************************************************
 * Typedef: enum SMEM_LION_2_3_UNIT_NAME_ENT
 *
 * Description:
 *      Memory units names -- used only as index into genericUsedUnitsAddressesArray[]
 *
 *******************************************************************************/
typedef enum{
/* 0*/    SMEM_LION2_UNIT_MG_E,
/* 1*/    SMEM_LION2_UNIT_IPLR1_E,
/* 2*/    SMEM_LION2_UNIT_TTI_E,
/* 3*/    SMEM_LION2_UNIT_DFX_SERVER_E,
/* 4*/    SMEM_LION2_UNIT_L2I_E,
/* 5*/    SMEM_LION2_UNIT_IPVX_E,
/* 6*/    SMEM_LION2_UNIT_BM_E,
/* 7*/    SMEM_LION2_UNIT_BMA_E,
/* 8*/    SMEM_LION2_UNIT_LMS_E,
/* 9*/    SMEM_LION2_UNIT_FDB_E,
/*10*/    SMEM_LION2_UNIT_MPPM_E,
/*11*/    SMEM_LION2_UNIT_CTU0_E,
/*12*/    SMEM_LION2_UNIT_EPLR_E,
/*13*/    SMEM_LION2_UNIT_CNC_E,
/*14*/    SMEM_LION2_UNIT_GOP_E,
/*15*/    SMEM_LION2_UNIT_XG_PORT_MIB_E,
/*16*/    SMEM_LION2_UNIT_SERDES_E,
/*17*/    SMEM_LION2_UNIT_MPPM1_E,
/*18*/    SMEM_LION2_UNIT_CTU1_E,
/*19*/    SMEM_LION2_UNIT_EQ_E,
/*20*/    SMEM_LION2_UNIT_IPCL_E,        /* not for Lion3 */
/*21*/    SMEM_LION2_UNIT_IPLR_E,
/*22*/    SMEM_LION2_UNIT_MLL_E,         /* not for Lion3 */
/*23*/    SMEM_LION2_UNIT_IPCL_TCC_E,    /* not for Lion3 */
/*24*/    SMEM_LION2_UNIT_IPVX_TCC_E,
/*25*/    SMEM_LION2_UNIT_EPCL_E,        /*not for lion3*/
/*26*/    SMEM_LION2_UNIT_HA_E,          /*not for lion3*/
/*27*/    SMEM_LION2_UNIT_RX_DMA_E,
/*28*/    SMEM_LION2_UNIT_TX_DMA_E,
/*29*/    SMEM_LION2_UNIT_TXQ_QUEUE_E,
/*30*/    SMEM_LION2_UNIT_TXQ_LL_E,
/*31*/    SMEM_LION2_UNIT_TXQ_DQ_E,
/*32*/    SMEM_LION2_UNIT_TXQ_SHT_E,      /*not for lion3*/
/*33*/    SMEM_LION2_UNIT_TXQ_EGR0_E,     /*not for lion3*/
/*34*/    SMEM_LION2_UNIT_TXQ_EGR1_E,     /*not for lion3*/
/*35*/    SMEM_LION2_UNIT_TXQ_DIST_E,     /*not for lion3*/
/*36*/    SMEM_LION2_UNIT_CPFC_E,         /*not for lion3*/

    SMEM_LION2_UNIT_LAST_E,

/*37*/    SMEM_LION3_UNIT_MLL_E                 = SMEM_LION2_UNIT_LAST_E,
/*38*/    SMEM_LION3_UNIT_IOAM_E,
/*39*/    SMEM_LION3_UNIT_EOAM_E,

/*40*/    SMEM_LION3_UNIT_LPM_E,

/*49*/    SMEM_LION3_UNIT_EQ_0_E,/*part 0 of EQ*/
/*50*/    SMEM_LION3_UNIT_EQ_1_E,/*part 1 of EQ*/

/*51*/    SMEM_LION3_UNIT_FDB_TABLE_0_E,
/*52*/    SMEM_LION3_UNIT_FDB_TABLE_1_E,

/*53*/    SMEM_LION3_UNIT_EGF_EFT_E,
/*54*/    SMEM_LION3_UNIT_EGF_EFT_1_E,

/*55*/    SMEM_LION3_UNIT_EGF_QAG_E,
/*56*/    SMEM_LION3_UNIT_EGF_QAG_1_E,

/*57*/    SMEM_LION3_UNIT_EGF_SHT_E,
/*58*/    SMEM_LION3_UNIT_EGF_SHT_1_E,
/*59*/    SMEM_LION3_UNIT_EGF_SHT_2_E,
/*60*/    SMEM_LION3_UNIT_EGF_SHT_3_E,
/*61*/    SMEM_LION3_UNIT_EGF_SHT_4_E,
/*62*/    SMEM_LION3_UNIT_EGF_SHT_5_E, /*!!! EMPTY !!!*/
/*63*/    SMEM_LION3_UNIT_EGF_SHT_6_E,
/*64*/    SMEM_LION3_UNIT_EGF_SHT_7_E, /*!!! EMPTY !!!*/
/*65*/    SMEM_LION3_UNIT_EGF_SHT_8_E,
/*66*/    SMEM_LION3_UNIT_EGF_SHT_9_E,
/*67*/    SMEM_LION3_UNIT_EGF_SHT_10_E,
/*68*/    SMEM_LION3_UNIT_EGF_SHT_11_E,
/*69*/    SMEM_LION3_UNIT_EGF_SHT_12_E,

/*70*/    SMEM_LION3_UNIT_HA_E,
/*71*/    SMEM_LION3_UNIT_IPCL_E,
/*72*/    SMEM_LION3_UNIT_EPCL_E,

/*73*/    SMEM_LION3_UNIT_TCAM_E,
/*74*/    SMEM_LION3_UNIT_ERMRK_E,

/*75*/    SMEM_LION3_UNIT_GOP_E,

/*76*/    SMEM_LION3_UNIT_IPLR0_E,
/*77*/    SMEM_LION3_UNIT_IPLR1_E,
/*78*/    SMEM_LION3_UNIT_EPLR_E,

/*80*/    SMEM_LION3_UNIT_CNC_0_E,
/*81*/    SMEM_LION3_UNIT_CNC_1_E,

/*82*/    SMEM_LION3_UNIT_TXQ_QCN_E,

          SMEM_LION3_UNIT_TX_FIFO_E,


/*83*/    SMEM_LION3_UNIT_LMS_E,
/*84*/    SMEM_LION3_UNIT_LMS1_E,
/*85*/    SMEM_LION3_UNIT_LMS2_E,

    SMEM_LION3_UNIT_LAST_E,

    SMEM_LION_2_3_UNIT_LAST_E = SMEM_LION3_UNIT_LAST_E

}SMEM_LION_2_3_UNIT_NAME_ENT;

/*the chunks for the GTS Unit */
static SMEM_UNIT_CHUNKS_STC gtsUnitChunk[2];/* index 0 - ITS (in TTI), index 1 - ETS (in HA)*/

/* the hosting units for the GTS Unit */
static GT_CHAR* hostingUnitForGtsUnit[2]= /* index 0 - ITS (in TTI), index 1 - ETS (in HA)*/
{
    STR(UNIT_TTI),
    STR(UNIT_HA),
};

/* the addresses of the units that the Lion 2,3 port group use */
static GT_U32   lion3UsedUnitsAddressesArray[SMEM_LION_2_3_UNIT_LAST_E]=
{
/* SMEM_LION2_UNIT_MG_E,                      */ LION3_MG_UNIT_BASE_ADDR_CNS  /* 0 */,
/* SMEM_LION2_UNIT_IPLR1_E,                   */ SMAIN_NOT_VALID_CNS          /* 1 */, /*not for lion3*/
/* SMEM_LION2_UNIT_TTI_E,                     */ LION3_TTI_UNIT_BASE_ADDR_CNS  /* 2 */,
/* SMEM_LION2_UNIT_DFX_SERVER_E               */ LION3_DFX_SERVER_UNIT_BASE_ADDR_CNS  /* 3 */,
/* SMEM_LION2_UNIT_L2I_E,                     */ LION3_L2I_UNIT_BASE_ADDR_CNS  /* 4 */,
/* SMEM_LION2_UNIT_IPVX_E,                    */ LION3_IPVX_UNIT_BASE_ADDR_CNS  /* 5 */,
/* SMEM_LION2_UNIT_BM_E,                      */ LION3_BM_UNIT_BASE_ADDR_CNS  /* 6 */,
/* SMEM_LION2_UNIT_BMA_E,                     */ LION3_BMA_UNIT_BASE_ADDR_CNS  /* 7 */,
/* SMEM_LION2_UNIT_LMS_E,                     */ SMAIN_NOT_VALID_CNS  /*0x04800000,0x05000000,0x05800000*//* 8,9,10,11*/ ,
/* SMEM_LION2_UNIT_FDB_E,                     */ LION3_FDB_UNIT_BASE_ADDR_CNS  /* 12*/,
/* SMEM_LION2_UNIT_MPPM_E,                    */ LION3_MPPM_UNIT_BASE_ADDR_CNS  /* 13*/,
/* SMEM_LION2_UNIT_CTU0_E,                    */ LION3_CTU_UNIT_BASE_ADDR_CNS  /* 14*/,
/* SMEM_LION2_UNIT_EPLR_E,                    */ SMAIN_NOT_VALID_CNS           /* 15*/, /*not for lion3*/
/* SMEM_LION2_UNIT_CNC_E,                     */ SMAIN_NOT_VALID_CNS           /* 16*/, /*not for lion3*/
/* SMEM_LION2_UNIT_GOP_E,                     */ LION3_GOP_UNIT_BASE_ADDR_CNS  /* 17*/,
/* SMEM_LION2_UNIT_XG_PORT_MIB_E,             */ LION3_MIB_UNIT_BASE_ADDR_CNS  /* 18*/,
/* SMEM_LION2_UNIT_SERDES_E,               */ LION3_SERDES_UNIT_BASE_ADDR_CNS  /* 19*/,
/* SMEM_LION2_UNIT_MPPM1_E,                   */ LION3_MPPM1_UNIT_BASE_ADDR_CNS   /* 20*/,
/* SMEM_LION2_UNIT_CTU1_E,                    */ LION3_CTU1_UNIT_BASE_ADDR_CNS  /* 21*/,
/* SMEM_LION2_UNIT_EQ_E,                      */ SMAIN_NOT_VALID_CNS  /* 22*/,  /*not for lion3*/
/* SMEM_LION2_UNIT_IPCL_E,                    */ SMAIN_NOT_VALID_CNS  /* 23*/,  /*not for lion3*/
/* SMEM_LION2_UNIT_IPLR_E,                    */ SMAIN_NOT_VALID_CNS  /* 24*/,  /*not for lion3*/
/* SMEM_LION2_UNIT_MLL_E,                     */ SMAIN_NOT_VALID_CNS  /* 25*/,  /*not for lion3*/
/* SMEM_LION2_UNIT_IPCL_TCC_E,                */ LION3_IPCL_TCC_UNIT_BASE_ADDR_CNS  /* 26*/,  /*not for lion3*/
/* SMEM_LION2_UNIT_IPVX_TCC_E,                */ LION3_IPVX_TCC_UNIT_BASE_ADDR_CNS  /* 27*/,
/* SMEM_LION2_UNIT_EPCL_E,                    */ SMAIN_NOT_VALID_CNS  /* 28*/,  /*not for lion3*/
/* SMEM_LION2_UNIT_HA_E,                      */ SMAIN_NOT_VALID_CNS  /* 29*/,  /*not for lion3*/

/* SMEM_LION2_UNIT_RX_DMA_E,                  */ LION3_RX_DMA_UNIT_BASE_ADDR_CNS  /* 30*/,
/* SMEM_LION2_UNIT_TX_DMA_E,                  */ LION3_TX_DMA_UNIT_BASE_ADDR_CNS  /* 31*/,
/* SMEM_LION2_UNIT_TXQ_QUEUE_E,               */ LION3_TXQ_QUEUE_UNIT_BASE_ADDR_CNS  /* 32*/,
/* SMEM_LION2_UNIT_TXQ_LL_E,                  */ LION3_TXQ_LL_UNIT_BASE_ADDR_CNS  /* 33*/,
/* SMEM_LION2_UNIT_TXQ_DQ_E,                  */ LION3_TXQ_DQ_UNIT_BASE_ADDR_CNS  /* 34*/,
/* SMEM_LION2_UNIT_TXQ_SHT_E,                 */ SMAIN_NOT_VALID_CNS  /* 35*/,  /*not for lion3*/
/* SMEM_LION2_UNIT_TXQ_EGR0_E,                */ SMAIN_NOT_VALID_CNS  /* 36*/,  /*not for lion3*/
/* SMEM_LION2_UNIT_TXQ_EGR1_E,                */ SMAIN_NOT_VALID_CNS  /* 37*/,  /*not for lion3*/
/* SMEM_LION2_UNIT_TXQ_DIST_E,                */ SMAIN_NOT_VALID_CNS  /* 38*/,
/* SMEM_LION2_UNIT_CPFC_E,                    */ LION3_CPFC_UNIT_BASE_ADDR_CNS  /* 39*/,

/************************/
/* Lion3 only           */
/************************/

/* SMEM_LION3_UNIT_MLL_E,                     */ LION3_MLL_UNIT_BASE_ADDR_CNS  /* 40*/,
/* SMEM_LION3_UNIT_IOAM_E,                    */ LION3_IOAM_UNIT_BASE_ADDR_CNS  /* 41*/,
/* SMEM_LION3_UNIT_EOAM_E                     */ LION3_EOAM_UNIT_BASE_ADDR_CNS  /* 43*/,

/* SMEM_LION3_UNIT_LPM_E,                     */ LION3_LPM_UNIT_BASE_ADDR_CNS  /* 44 */,

/* SMEM_LION3_UNIT_EQ_0_E,                    */ LION3_EQ_UNIT_BASE_ADDR_CNS(0)  /* 54 */,/*part 0 of EQ*/
/* SMEM_LION3_UNIT_EQ_1_E,                    */ LION3_EQ_UNIT_BASE_ADDR_CNS(1)  /* 55 */,/*part 1 of EQ*/

/* SMEM_LION3_UNIT_FDB_TABLE_0_E,             */ LION3_FDB_TABLE_BASE_ADDR_CNS(0)  /* 62*/,
/* SMEM_LION3_UNIT_FDB_TABLE_1_E,             */ LION3_FDB_TABLE_BASE_ADDR_CNS(1)  /* 63*/,

/* SMEM_LION3_UNIT_EGF_EFT_E,               */ LION3_EGF_EFT_BASE_ADDR_CNS(0 ),/*0x20000000*/ /* 64*/
/* SMEM_LION3_UNIT_EGF_EFT_1_E,               */ LION3_EGF_EFT_BASE_ADDR_CNS(1 ),/*0x20800000*/ /* 65*/

/* SMEM_LION3_UNIT_EGF_QAG_E,               */ LION3_EGF_QAG_BASE_ADDR_CNS(0 ),/*0x21000000*/ /* 66*/
/* SMEM_LION3_UNIT_EGF_QAG_1_E,               */ LION3_EGF_QAG_BASE_ADDR_CNS(1 ),/*0x21800000*/ /* 67*/

/* SMEM_LION3_UNIT_EGF_SHT_E,               */ LION3_EGF_SHT_BASE_ADDR_CNS(0 ),/*0x22000000*/ /* 68*/
/* SMEM_LION3_UNIT_EGF_SHT_1_E,               */ LION3_EGF_SHT_BASE_ADDR_CNS(1 ),/*0x22800000*/ /* 69*/
/* SMEM_LION3_UNIT_EGF_SHT_2_E,               */ LION3_EGF_SHT_BASE_ADDR_CNS(2 ),/*0x23000000*/ /* 70*/
/* SMEM_LION3_UNIT_EGF_SHT_3_E,               */ LION3_EGF_SHT_BASE_ADDR_CNS(3 ),/*0x23800000*/ /* 71*/
/* SMEM_LION3_UNIT_EGF_SHT_4_E,               */ LION3_EGF_SHT_BASE_ADDR_CNS(4 ),/*0x24000000*/ /* 72*/
/* SMEM_LION3_UNIT_EGF_SHT_5_E                */ LION3_EGF_SHT_BASE_ADDR_CNS(5 ),/*0x24800000*/ /* 73*//*              --> EMPTY */
/* SMEM_LION3_UNIT_EGF_SHT_6_E,               */ LION3_EGF_SHT_BASE_ADDR_CNS(6 ),/*0x25000000*/ /* 74*/
/* SMEM_LION3_UNIT_EGF_SHT_7_E,               */ LION3_EGF_SHT_BASE_ADDR_CNS(7 ),/*0x25800000*/ /* 75*//*              --> EMPTY */
/* SMEM_LION3_UNIT_EGF_SHT_8_E,               */ LION3_EGF_SHT_BASE_ADDR_CNS(8 ),/*0x26000000*/ /* 76*/
/* SMEM_LION3_UNIT_EGF_SHT_9_E,               */ LION3_EGF_SHT_BASE_ADDR_CNS(9 ),/*0x26800000*/ /* 77*/
/* SMEM_LION3_UNIT_EGF_SHT_10_E,              */ LION3_EGF_SHT_BASE_ADDR_CNS(10),/*0x27000000*/ /* 78*/
/* SMEM_LION3_UNIT_EGF_SHT_11_E,              */ LION3_EGF_SHT_BASE_ADDR_CNS(11),/*0x27800000*/ /* 79*/
/* SMEM_LION3_UNIT_EGF_SHT_12_E,              */ LION3_EGF_SHT_BASE_ADDR_CNS(12),/*0x28000000*/ /* 80*/

/* SMEM_LION3_UNIT_HA_E,                      */ LION3_HA_BASE_ADDR_CNS(0),      /*0x29000000*/ /* 82*/
/* SMEM_LION3_UNIT_IPCL_E,                    */ LION3_IPCL_BASE_ADDR_CNS,       /*0x2B000000*/ /* 86*/
/* SMEM_LION3_UNIT_EPCL_E,                    */ LION3_EPCL_BASE_ADDR_CNS,       /*0x2C000000*/ /* 88*/


/* SMEM_LION3_UNIT_TCAM_E,                    */ LION3_TCAM_UNIT_BASE_ADDR_CNS,  /*0x2A000000*/ /* 84 */
/* SMEM_LION3_UNIT_ERMRK_E,                   */ LION3_ERMRK_UNIT_BASE_ADDR_CNS,  /*0x2D000000*/ /* 90 */

/* SMEM_LION3_UNIT_GOP_E,                 */ LION3_NEW_GOP_UNIT_BASE_ADDR_CNS,  /*0x2E000000*/ /* 91 */

/* SMEM_LION3_UNIT_IPLR0_E,                   */ LION3_IPLR0_UNIT_BASE_ADDR_CNS, /*0x30000000*/   /*96*/
/* SMEM_LION3_UNIT_IPLR1_E,                   */ LION3_IPLR1_UNIT_BASE_ADDR_CNS, /*0x31000000*/   /*98*/
/* SMEM_LION3_UNIT_EPLR_E,                    */ LION3_EPLR_UNIT_BASE_ADDR_CNS , /*0x32000000*/   /*100*/

/* SMEM_LION3_UNIT_CNC_0_E,                   */ LION3_CNC_0_UNIT_BASE_ADDR_CNS, /*0x33000000*/   /*101*/
/* SMEM_LION3_UNIT_CNC_1_E,                   */ LION3_CNC_1_UNIT_BASE_ADDR_CNS,  /*0x34000000*/   /*102*/
/*SMEM_LION3_UNIT_TXQ_QCN_E                   */ LION3_TXQ_QCN_UNIT_BASE_ADDR_CNS,

/*SMEM_LION3_UNIT_TX_FIFO_E*/                    LION3_TX_FIFO_UNIT_BASE_ADDR_CNS,

/* SMEM_LION3_UNIT_LMS_E,                     */ LION3_LMS_UNIT_BASE_ADDR_CNS,
/* SMEM_LION3_UNIT_LMS1_E,                    */ LION3_LMS1_UNIT_BASE_ADDR_CNS,
/* SMEM_LION3_UNIT_LMS2_E,                    */ LION3_LMS2_UNIT_BASE_ADDR_CNS,
};


static SMEM_UNIT_NAME_AND_INDEX_STC lion3UnitNameAndIndexArr[]=
{
    {STR(UNIT_MG),                         SMEM_LION2_UNIT_MG_E                    },
    {STR(UNIT_IPLR1),                      SMEM_LION3_UNIT_IPLR1_E                 },/*lion3*/
    {STR(UNIT_TTI),                        SMEM_LION2_UNIT_TTI_E                   },
    {STR(UNIT_DFX_SERVER),                 SMEM_LION2_UNIT_DFX_SERVER_E            },
    {STR(UNIT_L2I),                        SMEM_LION2_UNIT_L2I_E                   },
    {STR(UNIT_IPVX),                       SMEM_LION2_UNIT_IPVX_E                  },
    {STR(UNIT_BM),                         SMEM_LION2_UNIT_BM_E                    },
    {STR(UNIT_BMA),                        SMEM_LION2_UNIT_BMA_E                   },
    {STR(UNIT_LMS),                        SMEM_LION3_UNIT_LMS_E                   },
    {STR(UNIT_FDB),                        SMEM_LION2_UNIT_FDB_E                   },
    {STR(UNIT_MPPM),                       SMEM_LION2_UNIT_MPPM_E                  },
    {STR(UNIT_CTU0),                       SMEM_LION2_UNIT_CTU0_E                  },
    {STR(UNIT_EPLR),                       SMEM_LION3_UNIT_EPLR_E                  },/*lion3*/
    {STR(UNIT_CNC),                        SMEM_LION3_UNIT_CNC_0_E                 },/*lion3*/
    {STR(UNIT_GOP),                        SMEM_LION2_UNIT_GOP_E                   },
    {STR(UNIT_XG_PORT_MIB),                SMEM_LION2_UNIT_XG_PORT_MIB_E           },
    {STR(UNIT_SERDES),                     SMEM_LION2_UNIT_SERDES_E                },
    {STR(UNIT_MPPM1),                      SMEM_LION2_UNIT_MPPM1_E                 },
    {STR(UNIT_CTU1),                       SMEM_LION2_UNIT_CTU1_E                  },
    {STR(UNIT_EQ),                         SMEM_LION3_UNIT_EQ_0_E                  },/*lion3*/
    {STR(UNIT_IPCL),                       SMEM_LION3_UNIT_IPCL_E                  },/*lion3*/
    {STR(UNIT_IPLR),                       SMEM_LION3_UNIT_IPLR0_E                 },/*lion3*/
    {STR(UNIT_MLL),                        SMEM_LION3_UNIT_MLL_E                   },/*lion3*/
    {STR(UNIT_IPCL_TCC),                   SMEM_LION2_UNIT_IPCL_TCC_E              },
    {STR(UNIT_IPVX_TCC),                   SMEM_LION2_UNIT_IPVX_TCC_E              },
    {STR(UNIT_EPCL),                       SMEM_LION3_UNIT_EPCL_E                  },/*lion3*/
    {STR(UNIT_HA),                         SMEM_LION3_UNIT_HA_E                    },/*lion3*/
    {STR(UNIT_RX_DMA),                     SMEM_LION2_UNIT_RX_DMA_E                },
    {STR(UNIT_TX_DMA),                     SMEM_LION2_UNIT_TX_DMA_E                },
    {STR(UNIT_TXQ_QUEUE),                  SMEM_LION2_UNIT_TXQ_QUEUE_E             },
    {STR(UNIT_TXQ_LL),                     SMEM_LION2_UNIT_TXQ_LL_E                },
    {STR(UNIT_TXQ_DQ),                     SMEM_LION2_UNIT_TXQ_DQ_E                },
    {STR(UNIT_TXQ_SHT),                    SMEM_LION2_UNIT_TXQ_SHT_E               },
    {STR(UNIT_CPFC),                       SMEM_LION2_UNIT_CPFC_E                  },

    /*lion3*/
    {STR(UNIT_IOAM),                       SMEM_LION3_UNIT_IOAM_E                  },
    {STR(UNIT_EOAM),                       SMEM_LION3_UNIT_EOAM_E                  },
    {STR(UNIT_LPM),                        SMEM_LION3_UNIT_LPM_E                   },
    {STR(UNIT_EQ_1),                       SMEM_LION3_UNIT_EQ_1_E                  },
    {STR(UNIT_FDB_TABLE_0),                SMEM_LION3_UNIT_FDB_TABLE_1_E           },
    {STR(UNIT_FDB_TABLE_1),                SMEM_LION3_UNIT_FDB_TABLE_1_E           },
    {STR(UNIT_EGF_EFT),                  SMEM_LION3_UNIT_EGF_EFT_E             },
    {STR(UNIT_EGF_EFT_1),                  SMEM_LION3_UNIT_EGF_EFT_1_E             },
    {STR(UNIT_EGF_QAG),                  SMEM_LION3_UNIT_EGF_QAG_E             },
    {STR(UNIT_EGF_QAG_1),                  SMEM_LION3_UNIT_EGF_QAG_1_E             },
    {STR(UNIT_EGF_SHT),                  SMEM_LION3_UNIT_EGF_SHT_E             },
    {STR(UNIT_EGF_SHT_1),                  SMEM_LION3_UNIT_EGF_SHT_1_E             },
    {STR(UNIT_EGF_SHT_2),                  SMEM_LION3_UNIT_EGF_SHT_2_E             },
    {STR(UNIT_EGF_SHT_3),                  SMEM_LION3_UNIT_EGF_SHT_3_E             },
    {STR(UNIT_EGF_SHT_4),                  SMEM_LION3_UNIT_EGF_SHT_4_E             },
    {STR(UNIT_EGF_SHT_5),                  SMEM_LION3_UNIT_EGF_SHT_5_E             },
    {STR(UNIT_EGF_SHT_6),                  SMEM_LION3_UNIT_EGF_SHT_6_E             },
    {STR(UNIT_EGF_SHT_7),                  SMEM_LION3_UNIT_EGF_SHT_7_E             },
    {STR(UNIT_EGF_SHT_8),                  SMEM_LION3_UNIT_EGF_SHT_8_E             },
    {STR(UNIT_EGF_SHT_9),                  SMEM_LION3_UNIT_EGF_SHT_9_E             },
    {STR(UNIT_EGF_SHT_10),                 SMEM_LION3_UNIT_EGF_SHT_10_E            },
    {STR(UNIT_EGF_SHT_11),                 SMEM_LION3_UNIT_EGF_SHT_11_E            },
    {STR(UNIT_EGF_SHT_12),                 SMEM_LION3_UNIT_EGF_SHT_12_E            },
    {STR(UNIT_TCAM),                       SMEM_LION3_UNIT_TCAM_E                  },
    {STR(UNIT_ERMRK),                      SMEM_LION3_UNIT_ERMRK_E                 },
    {STR(UNIT_GOP),                    SMEM_LION3_UNIT_GOP_E               },
    {STR(UNIT_CNC_1),                      SMEM_LION3_UNIT_CNC_1_E                 },
    {STR(UNIT_TXQ_QCN),                    SMEM_LION3_UNIT_TXQ_QCN_E               },

    {STR(UNIT_TX_FIFO),                    SMEM_LION3_UNIT_TX_FIFO_E               },

    {STR(UNIT_LMS1),                       SMEM_LION3_UNIT_LMS1_E                  },
    {STR(UNIT_LMS2),                       SMEM_LION3_UNIT_LMS2_E                  },

    {NULL ,                                SMAIN_NOT_VALID_CNS                     }
};



/*******************************************************************************
*   smemLion3SubUnitMemoryGet
*
* DESCRIPTION:
*       Get the port-group object for specific sub-unit.

* INPUTS:
*       devObjPtr   - pointer to device object.
*       accessType  - Memory access type PCI/Regular
*       address     - address of memory(register or table).
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       Pointer to object for specific subunit
*
* COMMENTS:
*       TxQ unit contains several sub-units, that can be classed into three categories:
*           - Sub-units that are duplicated per core — each sub-unit is configured
*             through the management interface bound to its core.
*           - Sub-units that serve a pair of cores, and thus have two instances.
*             Each of the instances is configured through a different management interface.
*           - Sub-units that have a single instance, and serve the entire device.
*
*******************************************************************************/
static SKERNEL_DEVICE_OBJECT * smemLion3SubUnitMemoryGet_fromSkernel
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32                  address
)
{
    return smemLion2SubUnitMemoryGet(devObjPtr,SKERNEL_MEMORY_READ_E,address);
}


/*******************************************************************************
*   smemLion3UnitOamUnify
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  any of the 2 OAMs:
*                   1. ioam
*                   2. eoam
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr     - pointer to the unit chunk
*       bindTable   - bind table with memory chunks
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
static void smemLion3UnitOamUnify
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr,
    IN GT_BOOL bindTable
)
{

    /* chunks with flat memory (no formulas) */
    {
        /* start with tables */
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            /* Aging Table */
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000C00, 256), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4), SMEM_BIND_TABLE_MAC(oamAgingTable)}
            /* Meg Exception Table */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00010000, 256), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4), SMEM_BIND_TABLE_MAC(oamMegExceptionTable)}
            /* Source Interface Exception Table */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00018000, 256), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4), SMEM_BIND_TABLE_MAC(oamSrcInterfaceExceptionTable)}
            /* Invalid Keepalive Hash Table */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00020000, 256), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4), SMEM_BIND_TABLE_MAC(oamInvalidKeepAliveHashTable)}
            /* Excess Keepalive Table */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00028000, 256), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4), SMEM_BIND_TABLE_MAC(oamExcessKeepAliveTable)}
            /* OAM Exception Summary Table */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00030000, 256), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4), SMEM_BIND_TABLE_MAC(oamExceptionSummaryTable)}
            /* RDI Status Change Exception Table */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00038000, 256), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4), SMEM_BIND_TABLE_MAC(oamRdiStatusChangeExceptionTable)}
            /* Tx Period Exception Table */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00040000, 256), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4), SMEM_BIND_TABLE_MAC(oamTxPeriodExceptionTable)}
            /* OAM Opcode Packet Command Table */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00060000, 2048), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(40, 8), SMEM_BIND_TABLE_MAC(oamOpCodePacketCommandTable)}
            /* OAM Table */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00070000, 32768), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(90, 16), SMEM_BIND_TABLE_MAC(oamTable)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);


        GT_U32  ii;

        for(ii = 0 ; ii < numOfChunks; ii++)
        {
            /* update tables address and bits */
            switch(chunksMem[ii].tableOffsetInBytes)
            {
                case FIELD_OFFSET_IN_STC_MAC(oamAgingTable,SKERNEL_TABLES_INFO_STC) :
                    chunksMem[ii].memFirstAddr =
                        SMEM_CHT_IS_SIP5_15_GET(devObjPtr) ? 0x00007000 : 0x00000C00;
                    break;
                case FIELD_OFFSET_IN_STC_MAC(oamTable,SKERNEL_TABLES_INFO_STC) :
                    chunksMem[ii].memFirstAddr =
                        SMEM_CHT_IS_SIP5_15_GET(devObjPtr) ? 0x00080000 : 0x00070000;
                    chunksMem[ii].enrtySizeBits =
                        SMEM_CHT_IS_SIP5_15_GET(devObjPtr) ? 104 : 90;
                    break;
                default :
                    break;
            }


            if (bindTable == GT_FALSE)
            {
                /* make sure that table are not bound to eoam(only to ioam) */
                chunksMem[ii].tableOffsetValid = 0;
                chunksMem[ii].tableOffsetInBytes = 0;
            }
        }

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }

    /* add the registers */
    if(SMEM_CHT_IS_SIP5_15_GET(devObjPtr) == 0)
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000000, 0x00000004)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000014, 0x0000003C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000050, 0x0000005C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000070, 0x0000007C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000084, 0x00000088)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000090, 0x000000E4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000000F0, 0x00000148)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000200, 0x00000204)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000220, 0x00000224)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000240, 0x00000244)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000260, 0x00000264)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000280, 0x00000284)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000002A0, 0x000002A4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000002C0, 0x000002C4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000300, 0x0000031C)}
        };
        smemUnitChunkAddBasicChunk(devObjPtr,unitPtr,
            ARRAY_NAME_AND_NUM_ELEMENTS_MAC(chunksMem));
    }
    else
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000000, 0x00000004)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000014, 0x0000003C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000050, 0x0000005C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000070, 0x0000007C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000084, 0x00000088)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000090, 0x000000E0)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000000F0, 0x00000148)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000160, 0x00000160)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000200, 0x00000204)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000300, 0x00000304)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000400, 0x00000404)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000500, 0x00000504)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000600, 0x00000604)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000700, 0x00000704)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000800, 0x00000804)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000900, 0x00000904)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000A00, 0x00000A1C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000A40, 0x00000A44)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000A50, 0x00000A58)}
        };
        smemUnitChunkAddBasicChunk(devObjPtr,unitPtr,
            ARRAY_NAME_AND_NUM_ELEMENTS_MAC(chunksMem));
    }


}

/*******************************************************************************
*   smemLion3UnitIOamUnify
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the IOAM
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr - pointer to the unit chunk
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
static void smemLion3UnitIOamUnify
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    smemLion3UnitOamUnify(devObjPtr, unitPtr, GT_TRUE);
}
/*******************************************************************************
*   smemLion3UnitEOamUnify
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the EOAM
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr - pointer to the unit chunk
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
static void smemLion3UnitEOamUnify
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    smemLion3UnitOamUnify(devObjPtr, unitPtr, GT_FALSE);
}


/*******************************************************************************
*   smemLion3UnitErmrk
*
* DESCRIPTION:
*       Allocate address type specific memories for ermrk (egress remark):
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr     - pointer to the unit chunk
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
static void smemLion3UnitErmrk
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    /* chunks with flat memory (no formulas) */
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000000, 0x00000008)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000010, 0x00000024)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000038, 0x00000038)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000040, 0x0000009C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000000E0, 0x000000E4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000100, 0x00000134)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00002000, 512),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(7, 4), SMEM_BIND_TABLE_MAC(oamLmOffsetTable)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00003000, 1536),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(24, 4), SMEM_BIND_TABLE_MAC(ERMRKQosMapTable)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00100000, 131072),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(6, 4), SMEM_BIND_TABLE_MAC(ptpLocalActionTable)}
        };

        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);
        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
    {
        SMEM_CHUNK_BASIC_STC  chunksMemBc2A0[]=
        {
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00005000, 512),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(26, 4), SMEM_BIND_TABLE_MAC(oamTimeStampOffsetTable)}
        };
        SMEM_CHUNK_BASIC_STC  chunksMemBc2B0[]=
        {
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00005000, 512),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(27, 4), SMEM_BIND_TABLE_MAC(oamTimeStampOffsetTable)}
           ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000028, 0x00000028)}
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

    /* to support bobk :
       add tables that depend on number of vlans and number of eport and number
       of physical ports */
    {
        GT_U32  numPhyPorts = devObjPtr->limitedResources.phyPort;
        GT_U32  index = 0;
        {
            SMEM_CHUNK_BASIC_STC  chunksMem[]=
            {
                 {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00200000, 16384),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(321, 64), SMEM_BIND_TABLE_MAC(ptpTargetPortTable)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00300000, 16384),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(321, 64)}
            };

            /* number of entries : numPhyPorts . keep alignment and use for memory size */
            chunksMem[index].numOfRegisters = numPhyPorts * (chunksMem[index].enrtyNumBytesAlignement / 4);
            index++;
            /* number of entries : numPhyPorts . keep alignment and use for memory size */
            chunksMem[index].numOfRegisters = numPhyPorts * (chunksMem[index].enrtyNumBytesAlignement / 4);
            index++;

            smemUnitChunkAddBasicChunk(devObjPtr,unitPtr,
                ARRAY_NAME_AND_NUM_ELEMENTS_MAC(chunksMem));
        }
    }

}

/*******************************************************************************
*   smemLion3UnitIpvx
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  ipvx unit
*
* INPUTS:
*       commonDevMemInfoPtr - pointer to common device memory object.
*       unitPtr             - pointer to the unit chunk
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void smemLion3UnitIpvx
(
    IN SKERNEL_DEVICE_OBJECT *devObjPtr,
    IN SMEM_UNIT_CHUNKS_STC  *unitPtr
)
{
    SMEM_CHUNK_BASIC_STC  chunksMem[]=
    {
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000000, 0x00000010)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000020, 0x00000024)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000100, 0x00000104)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000200, 0x00000204)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000250, 0x0000026C)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000278, 0x00000294)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000360, 0x00000364)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000380, 0x00000380)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000900, 0x00000924)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000940, 0x00000948)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000950, 0x00000958)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000960, 0x00000960)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000968, 0x00000968)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000970, 0x00000978)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000980, 0x00000984)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000A00, 0x00000A24)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000A80, 0x00000A84)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000B00, 0x00000B24)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000B80, 0x00000B84)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000C00, 0x00000C24)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000C80, 0x00000C84)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000E00, 0x00000E2C)},


        /* Router QoS Profile Offsets Table */
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00010000, 1024),
            SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(12, 4),SMEM_BIND_TABLE_MAC(ipvxQoSProfileOffsets)},

        /* Router Acces Matrix Table */
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00060000, 3072),
            SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(128, 16),SMEM_BIND_TABLE_MAC(ipvxAccessMatrix)}
    };

    GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);
    smemInitMemChunk(devObjPtr,chunksMem, numOfChunks, unitPtr);


    /* to support bobk :
       add tables that depend on number of vlans and number of eport and number
       of physical ports */
    /* chunks with formulas */
    {
        GT_U32  numEPorts = devObjPtr->limitedResources.ePort;
        GT_U32  numEVlans = devObjPtr->limitedResources.eVid;
        GT_U32  numNextHop = devObjPtr->limitedResources.nextHop;
        GT_U32  index;

        {
            SMEM_CHUNK_BASIC_STC  chunksMem[]=
            {
                /* Router EPort Table */
                {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00200000, 8192),
                    SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(128, 16),SMEM_BIND_TABLE_MAC(ipvxIngressEPort)}
            };

            index = 0;
            /* number of entries : (numEPorts / 16) . keep alignment and use for memory size */
            chunksMem[index].numOfRegisters = (numEPorts / 16) * (chunksMem[index].enrtyNumBytesAlignement / 4);
            index++;

            smemUnitChunkAddBasicChunk(devObjPtr,unitPtr,
                ARRAY_NAME_AND_NUM_ELEMENTS_MAC(chunksMem));
        }
        {
            SMEM_CHUNK_BASIC_STC  chunksMem[]=
            {
                /* Router EVlan Table */
                {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00100000, 131072),
                    SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(66, 16),SMEM_BIND_TABLE_MAC(ipvxIngressEVlan)}
            };

            index = 0;
            /* number of entries : numEVlans . keep alignment and use for memory size */
            chunksMem[index].numOfRegisters = numEVlans * (chunksMem[index].enrtyNumBytesAlignement / 4);
            index++;

            smemUnitChunkAddBasicChunk(devObjPtr,unitPtr,
                ARRAY_NAME_AND_NUM_ELEMENTS_MAC(chunksMem));
        }
        {
            SMEM_CHUNK_BASIC_STC  chunksMem[]=
            {
                /* Router Next Hop Table */
                 {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00400000, 393216),
                    SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(104, 16),SMEM_BIND_TABLE_MAC(ipvxNextHop)}
                /* Router Next Hop Table Age Bits */
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00020000, 3072),
                    SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4),SMEM_BIND_TABLE_MAC(routeNextHopAgeBits)}
            };

            index = 0;
            /* number of entries : numNextHop . keep alignment and use for memory size */
            chunksMem[index].numOfRegisters = numNextHop * (chunksMem[index].enrtyNumBytesAlignement / 4);
            index++;
            /* number of entries : (numNextHop/32) . keep alignment and use for memory size */
            chunksMem[index].numOfRegisters = (numNextHop/32) * (chunksMem[index].enrtyNumBytesAlignement / 4);
            index++;

            smemUnitChunkAddBasicChunk(devObjPtr,unitPtr,
                ARRAY_NAME_AND_NUM_ELEMENTS_MAC(chunksMem));
        }
    }

}

/*******************************************************************************
*   smemLion3UnitLpm
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the lion3 LPM unit
*
* INPUTS:
*       commonDevMemInfoPtr - pointer to common device memory object.
*       unitPtr             - pointer to the unit chunk
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void smemLion3UnitLpm
(
    IN SKERNEL_DEVICE_OBJECT *devObjPtr,
    IN SMEM_UNIT_CHUNKS_STC  *unitPtr
)
{
    if(unitPtr->chunkType == SMEM_UNIT_CHUNK_TYPE_9_MSB_E)
    {
        unitPtr->numOfUnits = 2;/* supports 2 '23 bits' address units */
    }

    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00D10000, 16384),
                     SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(25, 4),SMEM_BIND_TABLE_MAC(lpmIpv4VrfId)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00D20000, 16384),
                     SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(25, 4),SMEM_BIND_TABLE_MAC(lpmIpv6VrfId)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00D30000, 16384),
                     SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(25, 4),SMEM_BIND_TABLE_MAC(lpmFcoeVrfId)},

            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00D00000, 0x00D00004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00D00010, 0x00D00010)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00D00020, 0x00D00020)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00D00100, 0x00D00100)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00D00110, 0x00D00110)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00D00120, 0x00D00120)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00D00130, 0x00D00130)}

            /* ETA 0 */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00E00000, 0x00E00000)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00E00100, 0x00E00104)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00E00110, 0x00E00110)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00E00120, 0x00E00120)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00E00130, 0x00E00130)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00E00204, 0x00E00210)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00E00254, 0x00E00260)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00E00300, 0x00E0034C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00E00400, 0x00E0044C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00E00500, 0x00E0053C)}

            /* ETA 1 */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00F00000, 0x00F00000)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00F00100, 0x00F00104)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00F00110, 0x00F00110)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00F00120, 0x00F00120)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00F00130, 0x00F00130)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00F00204, 0x00F00210)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00F00254, 0x00F00260)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00F00300, 0x00F0034C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00F00400, 0x00F0044C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00F00500, 0x00F0053C)}
        };

        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);
        smemInitMemChunk(devObjPtr,chunksMem, numOfChunks, unitPtr);
    }


    /* to support bobk :
       add tables that depend on number of vlans and number of eport and number
       of physical ports */
    {
        GT_U32  l3LttMll = devObjPtr->limitedResources.l3LttMll;
        GT_U32  perRamNumEntries = devObjPtr->lpmRam.perRamNumEntries;

        GT_U32  index;
        {
            SMEM_CHUNK_BASIC_STC  chunksMem[]=
            {
                {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00D40000, 49152),
                         SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(28, 4),SMEM_BIND_TABLE_MAC(lpmEcmp)},
            };

            index = 0;
            /* number of entries : l3LttMll . keep alignment and use for memory size */
            chunksMem[index].numOfRegisters = l3LttMll * (chunksMem[index].enrtyNumBytesAlignement / 4);
            index++;

            smemUnitChunkAddBasicChunk(devObjPtr,unitPtr,
                ARRAY_NAME_AND_NUM_ELEMENTS_MAC(chunksMem));
        }

        {

            SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
            {
                /*LPM mem : actual 20 rams each with 16k / 6k or 2k entries.
                    step of 16k lines between each ram */
                {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00000000, 1310720),
                     SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4),SMEM_BIND_TABLE_MAC(lpmMemory)},
                     FORMULA_SINGLE_PARAMETER(20, (16*1024)*4)},
            };
            GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
            SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

            index = 0;
            chunksMem[index].memChunkBasic.numOfRegisters = perRamNumEntries * (chunksMem[index].memChunkBasic.enrtyNumBytesAlignement / 4);
            index++;

            smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

            /*add the tmp unit chunks to the main unit */
            smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);
        }
    }


}

/*******************************************************************************
*   smemLion3UnitTcam
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  TCAM unit
*
* INPUTS:
*       commonDevMemInfoPtr - pointer to common device memory object.
*       unitPtr             - pointer to the unit chunk
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void smemLion3UnitTcam
(
    IN SKERNEL_DEVICE_OBJECT *devObjPtr,
    IN SMEM_UNIT_CHUNKS_STC  *unitPtr
)
{
    SMEM_CHUNK_BASIC_STC  chunksMem[]=
    {
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000000, 1572864),
         SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(84, 16),SMEM_BIND_TABLE_MAC(tcamMemory)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00200000, 1572864),
         SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(240, 32),SMEM_BIND_TABLE_MAC(globalActionTable)},

        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00400000, 294912)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00500000, 0x00500010)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00500100, 0x00500100)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00500108, 0x00500108)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00500110, 0x00500110)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00500118, 0x00500118)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00500120, 0x00500120)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00500128, 0x00500128)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00500130, 0x00500130)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00500138, 0x00500138)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00500140, 0x00500140)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00500148, 0x00500148)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00500150, 0x00500150)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00500158, 0x00500158)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00501000, 0x00501010)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00502000, 0x005021C8)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x005021D0, 0x005021DC)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x005021E4, 0x005021F0)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x005021F8, 0x00502204)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0050220C, 0x00502218)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00502300, 0x0050230C)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00503000, 0x00503030)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0050303C, 0x0050303C)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00504010, 0x00504014)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00505000, 0x0050509C)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00506000, 0x00506058)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0050606C, 0x00506078)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00507000, 0x0050700C)}
    };

    GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);
    smemInitMemChunk(devObjPtr,chunksMem, numOfChunks, unitPtr);
}

/*******************************************************************************
*   smemLion3UnitEgfSht
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Lion3  EGF-SHT unit
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
static void smemLion3UnitEgfSht
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {

        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x06020000, 0x06020008)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x06020010, 0x06020010)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x06020020, 0x06020020)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x06020030, 0x0602010C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x06020130, 0x060201AC)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x06020240, 0x0602025C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x06020270, 0x0602028C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x06020300, 0x06020300)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x06020310, 0x06020310)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x06020320, 0x0602039C)}

             /* Eport EVlan Filter*/
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x020A0000 , 131072), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(256, 32),SMEM_BIND_TABLE_MAC(egfShtEportEVlanFilter)}
             /* Device Map Table */
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02100000 ,  16384), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(16, 4),SMEM_BIND_TABLE_MAC(deviceMapTable)}
        };

        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
    {
        SMEM_CHUNK_BASIC_STC  chunksMemBc2B0[]=
        {
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x060203A0, 0x060203BC)}
           ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x06020400, 0x06020600)}
        };


        if(SMEM_CHT_IS_SIP5_10_GET(devObjPtr))
        {
            smemUnitChunkAddBasicChunk(devObjPtr,unitPtr,
                ARRAY_NAME_AND_NUM_ELEMENTS_MAC(chunksMemBc2B0));
        }
    }

    /* to support bobk :
       add tables that depend on number of vlans and number of eport and number
       of physical ports */
    {
        /*GT_U32  numPhyPorts = devObjPtr->limitedResources.phyPort;*/
        GT_U32  numEPorts = devObjPtr->limitedResources.ePort;
        GT_U32  numEVlans = devObjPtr->limitedResources.eVid;
        GT_U32  numStg = 1 << devObjPtr->flexFieldNumBitsSupport.stgId;

        GT_U32  index;
        {
            SMEM_CHUNK_BASIC_STC  chunksMem[]=
            {
                 /* Egress vlan table */
                  {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x03000000 ,524288), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(275, 64),SMEM_BIND_TABLE_MAC(egressVlan)}
                 /* EVlan Attribute table */
                 ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x03400000 , 32768), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(6, 4),SMEM_BIND_TABLE_MAC(egfShtEVlanAttribute)}
                 /* EVlan Spanning table */
                 ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x03440000 , 32768), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(12, 4),SMEM_BIND_TABLE_MAC(egfShtEVlanSpanning)}
                 /* Vid Mapper Table*/
                 ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02110000 ,  32768), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(12, 4),SMEM_BIND_TABLE_MAC(egfShtVidMapper)}
            };

            index = 0;
            /* number of entries : numEVlans . keep alignment and use for memory size */
            chunksMem[index].numOfRegisters = numEVlans * (chunksMem[index].enrtyNumBytesAlignement / 4);
            /*chunksMem[index].enrtySizeBits = 1+numPhyPorts; entry is triggered by last word as if 256 ports exists !!!! */
            index++;
            /* number of entries : numEVlans . keep alignment and use for memory size */
            chunksMem[index].numOfRegisters = numEVlans * (chunksMem[index].enrtyNumBytesAlignement / 4);
            index++;
            /* number of entries : numEVlans . keep alignment and use for memory size */
            chunksMem[index].numOfRegisters = numEVlans * (chunksMem[index].enrtyNumBytesAlignement / 4);
            index++;
            /* number of entries : numEVlans . keep alignment and use for memory size */
            chunksMem[index].numOfRegisters = numEVlans * (chunksMem[index].enrtyNumBytesAlignement / 4);
            index++;

            smemUnitChunkAddBasicChunk(devObjPtr,unitPtr,
                ARRAY_NAME_AND_NUM_ELEMENTS_MAC(chunksMem));
        }
        {
            SMEM_CHUNK_BASIC_STC  chunksMem[]=
            {
                 /* Egress EPort table */
                 {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02200000 , 32768), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(15, 4),SMEM_BIND_TABLE_MAC(egfShtEgressEPort)}
            };

            index = 0;
            /* number of entries : numEPorts . keep alignment and use for memory size */
            chunksMem[index].numOfRegisters = numEPorts * (chunksMem[index].enrtyNumBytesAlignement / 4);
            index++;

            smemUnitChunkAddBasicChunk(devObjPtr,unitPtr,
                ARRAY_NAME_AND_NUM_ELEMENTS_MAC(chunksMem));
        }
        {
            SMEM_CHUNK_BASIC_STC  chunksMem[]=
            {
                 /* Egress Spanning Tree State Table */
                 {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02040000 , 131072), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(256, 32),SMEM_BIND_TABLE_MAC(egressStp)}
            };

            index = 0;
            /* number of entries : numStg . keep alignment and use for memory size */
            chunksMem[index].numOfRegisters = numStg * (chunksMem[index].enrtyNumBytesAlignement / 4);
            /*chunksMem[index].enrtySizeBits = numPhyPorts; entry is triggered by last word as if 256 ports exists !!!! */
            index++;

            smemUnitChunkAddBasicChunk(devObjPtr,unitPtr,
                ARRAY_NAME_AND_NUM_ELEMENTS_MAC(chunksMem));
        }
        {
            SMEM_CHUNK_BASIC_STC  chunksMem[]=
            {
                 /* L2 Port Isolation Table */
                  {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00000000 ,69632), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(256, 32),SMEM_BIND_TABLE_MAC(l2PortIsolation)}
                 /* Non Trunk Members 2 Table */
                 ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02060000 , 131072 ), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(256, 32),SMEM_BIND_TABLE_MAC(nonTrunkMembers2)}
                 /* Source ID Members Table */
                 ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02080000 , 131072), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(256, 32),SMEM_BIND_TABLE_MAC(sst)}
                 /* Multicast Groups Table */
                 ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x020C0000 , 131072), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(256, 32),SMEM_BIND_TABLE_MAC(mcast)}
                 /* Non Trunk Members Table */
                 ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x020E0000 , 8192 ), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(256, 32),SMEM_BIND_TABLE_MAC(nonTrunkMembers)}
                 /* Designated Port Table */
                 ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02120000 ,   4096), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(256, 32),SMEM_BIND_TABLE_MAC(designatedPorts)}
                 /* L3 Port Isolation Table */
                 ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x04000000 ,69632), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(256, 32),SMEM_BIND_TABLE_MAC(l3PortIsolation)}
            };
            /*GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);*/

            /* all those tables hold bits according to number of physical ports */
            /*for(index = 0; index < numOfChunks; index++)
            {
                chunksMem[index].enrtySizeBits = numPhyPorts; entry is triggered by last word as if 256 ports exists !!!!
            }*/

            smemUnitChunkAddBasicChunk(devObjPtr,unitPtr,
                ARRAY_NAME_AND_NUM_ELEMENTS_MAC(chunksMem));
        }
    }


}

/*******************************************************************************
*   smemLion3UnitEgfEft
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Lion3  EGF-EFT unit
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
static void smemLion3UnitEgfEft
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001000, 0x00001000)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001020, 0x00001020)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001030, 0x00001030)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001040, 0x0000104C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000010A0, 0x000010A0)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000010B0, 0x000010B0)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001110, 0x00001110)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001130, 0x00001130)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001140, 0x00001140)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001150, 0x0000124C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002000, 0x0000200C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002100, 0x00002100)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002108, 0x00002108)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000021D0, 0x000021D0)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000021E0, 0x000021E0)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000021F0, 0x00002230)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002240, 0x0000229C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003000, 0x0000345C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00010000, 0x00010004)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00010010, 0x0001006C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000100B0, 0x0001010C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00010120, 0x0001025C)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);


        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }

    {
        SMEM_CHUNK_BASIC_STC  chunksMemBc2A0[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001120, 0x00001120)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002300, 0x00002300)}

        };
        SMEM_CHUNK_BASIC_STC  chunksMemBc2B0[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00004000, 0x00004008)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00005000, 0x00005000)}
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

    /* to support bobk :
       add tables that depend on number of vlans and number of eport and number
       of physical ports */
    {
        GT_U32  numPhyPorts = devObjPtr->limitedResources.phyPort;

        GT_U32  index;
        {
            SMEM_CHUNK_BASIC_STC  chunksMem[]=
            {
                /* Secondary Target Port Table */
                {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000000, 1024), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(8, 4) , SMEM_BIND_TABLE_MAC(secondTargetPort)}
            };

            index = 0;
            /* number of entries : numPhyPorts . keep alignment and use for memory size */
            chunksMem[index].numOfRegisters = numPhyPorts * (chunksMem[index].enrtyNumBytesAlignement / 4);
            index++;

            smemUnitChunkAddBasicChunk(devObjPtr,unitPtr,
                ARRAY_NAME_AND_NUM_ELEMENTS_MAC(chunksMem));
        }
    }


}


/*******************************************************************************
*   smemLion3UnitEgfQag
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Lion3  EGF-QAG unit
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
static void smemLion3UnitEgfQag
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00E00000, 0x00E00000)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00E00010, 0x00E00014)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00E00050, 0x00E000AC)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00E00180, 0x00E0019C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00E001C0, 0x00E0025C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00E005FC, 0x00E00638)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00E00A00, 0x00E00A20)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00F00010, 0x00F00010)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00F00020, 0x00F00020)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00F00030, 0x00F00030)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00F00100, 0x00F0010C)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);


        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
    {
        SMEM_CHUNK_BASIC_STC  chunksMemBc2A0[]=
        {
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00E000F0, 0x00E00170)}
        };
        SMEM_CHUNK_BASIC_STC  chunksMemBc2B0[]=
        {
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00E000F0, 0x00E0017C)}
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

    /* to support bobk :
       add tables that depend on number of vlans and number of eport and number
       of physical ports */
    {
        GT_U32  numPhyPorts = devObjPtr->limitedResources.phyPort;
        GT_U32  numEPorts = devObjPtr->limitedResources.ePort;
        GT_U32  numEVlans = devObjPtr->limitedResources.eVid;

        GT_U32  index;
        {
            SMEM_CHUNK_BASIC_STC  chunksMem[]=
            {
                /* Evlan Descriptor Assignment Attributes */
                 {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000000 , 1048576 ), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(768, 128),SMEM_BIND_TABLE_MAC(egfQagEVlanDescriptorAssignmentAttributes)}
            };

            index = 0;
            /* number of entries : numEVlans . keep alignment and use for memory size */
            chunksMem[index].numOfRegisters = numEVlans * (chunksMem[index].enrtyNumBytesAlignement / 4);
            /*chunksMem[index].enrtySizeBits = 3*numPhyPorts; entry is triggered by last word as if 256 ports exists !!!! */
            index++;

            smemUnitChunkAddBasicChunk(devObjPtr,unitPtr,
                ARRAY_NAME_AND_NUM_ELEMENTS_MAC(chunksMem));
        }

        {
            SMEM_CHUNK_BASIC_STC  chunksMem[]=
            {
                /* Target Port Mapper */
                 {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00900000, 1024 ), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(8, 4),SMEM_BIND_TABLE_MAC(egfQagTargetPortMapper)}
                /*Fwd From CPU To Loopback Mapper*/
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00900800, 1024), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(8, 4)}
                /*Cpu Code To Loopback Mapper Memory*/
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00900400, 1024), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(8, 4)}
            };

            index = 0;
            /* number of entries : numPhyPorts . keep alignment and use for memory size */
            chunksMem[index].numOfRegisters = numPhyPorts * (chunksMem[index].enrtyNumBytesAlignement / 4);
            index++;
            /* number of entries : numPhyPorts . keep alignment and use for memory size */
            chunksMem[index].numOfRegisters = numPhyPorts * (chunksMem[index].enrtyNumBytesAlignement / 4);
            index++;
            /* number of entries : numPhyPorts . keep alignment and use for memory size */
            chunksMem[index].numOfRegisters = numPhyPorts * (chunksMem[index].enrtyNumBytesAlignement / 4);
            index++;

            smemUnitChunkAddBasicChunk(devObjPtr,unitPtr,
                ARRAY_NAME_AND_NUM_ELEMENTS_MAC(chunksMem));
        }



        if(SMEM_CHT_IS_SIP5_10_GET(devObjPtr) == 0)
        {
            SMEM_CHUNK_BASIC_STC  chunksMemBc2A0[]=
            {
                /* ePort Descriptor Assignment Attributes Table  */
                {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00800000 , 65536 ), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(47, 8),SMEM_BIND_TABLE_MAC(egfQagEgressEPort)}

            };
            index = 0;
            /* number of entries : numEPorts . keep alignment and use for memory size */
            chunksMemBc2A0[index].numOfRegisters = numEPorts * (chunksMemBc2A0[index].enrtyNumBytesAlignement / 4);
            index++;
            smemUnitChunkAddBasicChunk(devObjPtr,unitPtr,
                ARRAY_NAME_AND_NUM_ELEMENTS_MAC(chunksMemBc2A0));
        }
        else
        {
            SMEM_CHUNK_BASIC_STC  chunksMemBc2B0[]=
            {
                /* ePort Descriptor Assignment Attributes Table  */
                {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00800000 , 65536 ), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(45, 8),SMEM_BIND_TABLE_MAC(egfQagEgressEPort)}
            };
            index = 0;
            /* number of entries : numEPorts . keep alignment and use for memory size */
            chunksMemBc2B0[index].numOfRegisters = numEPorts * (chunksMemBc2B0[index].enrtyNumBytesAlignement / 4);
            index++;
            smemUnitChunkAddBasicChunk(devObjPtr,unitPtr,
                ARRAY_NAME_AND_NUM_ELEMENTS_MAC(chunksMemBc2B0));
        }
    }



}

/*******************************************************************************
*   smemLion3UnitGts
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  GTS unit
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       gtsIndex    - GTS unit chunk index
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemLion3UnitGts
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32               gtsIndex
)
{
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = devObjPtr->deviceMemory;
    SMEM_UNIT_CHUNKS_STC    *currUnitChunkPtr;
    SMEM_UNIT_CHUNKS_STC  * unitPtr = &gtsUnitChunk[gtsIndex];
    GT_U32  ii;

    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000000 ,0x0000003c)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        for(ii = 0; ii < numOfChunks; ii++)
        {
            chunksMem[ii].memFirstAddr |= devObjPtr->memUnitBaseAddrInfo.gts[gtsIndex];
        }

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }

    /*add the GTS[0] to the TTI unit */
    /*add the GTS[1] to the HA unit */
    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_STRING_GET_MAC(devObjPtr,hostingUnitForGtsUnit[gtsIndex])];
    smemInitMemCombineUnitChunks(devObjPtr,currUnitChunkPtr,unitPtr);
}


/*******************************************************************************
*   policerTablesSupport
*
* DESCRIPTION:
*       manage Policer tables:
*                   1. iplr 0 - all tables
*                   2. iplr 1 - none
*                   3. eplr   - other names for meter+counting
*
* INPUTS:
*       numOfChunks   - pointer to device object.
*       unitPtr - pointer to the unit chunk
*       plrUnit - PLR unit
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void policerTablesSupport
(
    IN GT_U32  numOfChunks,
    INOUT SMEM_CHUNK_BASIC_STC  chunksMem[],
    IN SMEM_SIP5_PP_PLR_UNIT_ENT   plrUnit
)
{
    GT_BIT  eplr  = (plrUnit == SMEM_SIP5_PP_PLR_UNIT_EPLR_E) ? 1 : 0;
    GT_BIT  iplr1 = (plrUnit == SMEM_SIP5_PP_PLR_UNIT_IPLR_1_E) ? 1 : 0;
    GT_U32  ii;

    if(plrUnit == SMEM_SIP5_PP_PLR_UNIT_IPLR_0_E)
    {
        /* no modification needed */
        return;
    }

    for(ii = 0 ; ii < numOfChunks ; ii ++)
    {
        if (iplr1)
        {
            /* make sure that table are not bound to iplr1 (only to iplr0) */
            chunksMem[ii].tableOffsetValid = 0;
            chunksMem[ii].tableOffsetInBytes = 0;
        }
        else if(eplr)
        {
            switch(chunksMem[ii].tableOffsetInBytes)
            {
                case FIELD_OFFSET_IN_STC_MAC(policer,SKERNEL_TABLES_INFO_STC):
                    chunksMem[ii].tableOffsetInBytes =
                        FIELD_OFFSET_IN_STC_MAC(egressPolicerMeters,SKERNEL_TABLES_INFO_STC);
                    break;
                case FIELD_OFFSET_IN_STC_MAC(policerCounters,SKERNEL_TABLES_INFO_STC):
                    chunksMem[ii].tableOffsetInBytes =
                        FIELD_OFFSET_IN_STC_MAC(egressPolicerCounters,SKERNEL_TABLES_INFO_STC);
                    break;
                default:
                    /* make sure that table are not bound to EPLR (only to iplr0) */
                    chunksMem[ii].tableOffsetValid = 0;
                    chunksMem[ii].tableOffsetInBytes = 0;
                    break;
            }
        }
    }
}

/*******************************************************************************
*   smemLion3UnitPolicerUnify
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  any of the 3 Policers:
*                   1. iplr 0
*                   2. iplr 1
*                   3. eplr
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr - pointer to the unit chunk
*       plrUnit - PLR unit
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemLion3UnitPolicerUnify
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr,
    IN SMEM_SIP5_PP_PLR_UNIT_ENT   plrUnit
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000000, 0x0000003C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000048, 0x00000054)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000060, 0x00000068)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000070, 0x00000070)}
            /*registers -- not table/memory !! -- Policer Table Access Data<%n> */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00000074 ,8*4),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32,4),SMEM_BIND_TABLE_MAC(policerTblAccessData)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000000B0, 0x000000B8)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000000C0, 0x000001BC)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000200, 0x0000020C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000220, 0x00000244)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000250, 0x0000028C)}
            /*Policer Timer Memory*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000300, 36), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32,4),SMEM_BIND_TABLE_MAC(policerTimer)}
            /*Policer Descriptor Sample Memory*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000400, 96), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32,4)}
            /*Policer Management Counters Memory*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000500, 192), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(74,16),SMEM_BIND_TABLE_MAC(policerManagementCounters)}
            /*registers (not memory) : Port%p and Packet Type Translation Table*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001800, 0x000019FC)}
            /*Ingress Policer Re-Marking Memory*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00080000, 8192), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(51,8),SMEM_BIND_TABLE_MAC(policerQos)}
            /*Hierarchical Policing Table*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00090000, 32768), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(18,4), SMEM_BIND_TABLE_MAC(policerHierarchicalQos)}
            /*Policer Metering and Counting Memory*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00100000, 524288/2), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(241,32),SMEM_BIND_TABLE_MAC(policer)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00140000, 524288/2), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(241,32),SMEM_BIND_TABLE_MAC(policerCounters)}

        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        policerTablesSupport(numOfChunks,chunksMem,plrUnit);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);

        {
            GT_U32  numOfChunks;
            SMEM_CHUNK_BASIC_STC  chunksMemBc2A0[]=
            {
                /*IPFIX wrap around alert Memory*/
                {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000800, 256), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32,4),SMEM_BIND_TABLE_MAC(policerIpfixWaAlert)}
                /*IPFIX aging alert Memory*/
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000900, 256), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32,4),SMEM_BIND_TABLE_MAC(policerIpfixAgingAlert)}
                /*DFX table*/
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00001000, 2048), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32,4)}
                /*e Attributes Table*/
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00010000, 65536), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(36,8),SMEM_BIND_TABLE_MAC(policerEPortEVlanTrigger)}
            };

            SMEM_CHUNK_BASIC_STC  chunksMemBc2B0[]=
            {
               /*IPFIX wrap around alert Memory*/
               {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000800, 1024), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32,4),SMEM_BIND_TABLE_MAC(policerIpfixWaAlert)}
               /*IPFIX aging alert Memory*/
               ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00001000, 1024), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32,4),SMEM_BIND_TABLE_MAC(policerIpfixAgingAlert)}
               ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002000, 0x00002000)}
                /*e Attributes Table*/
               ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00040000, 65536), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(36,8),SMEM_BIND_TABLE_MAC(policerEPortEVlanTrigger)}
            };

            SMEM_CHUNK_BASIC_STC  * chunksMemPtr;
            SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;


            if(SMEM_CHT_IS_SIP5_10_GET(devObjPtr) == 0)
            {
                chunksMemPtr = chunksMemBc2A0;
                numOfChunks = sizeof(chunksMemBc2A0)/sizeof(SMEM_CHUNK_BASIC_STC);
            }
            else
            {
                chunksMemPtr = chunksMemBc2B0;
                numOfChunks = sizeof(chunksMemBc2B0)/sizeof(SMEM_CHUNK_BASIC_STC);
            }

            policerTablesSupport(numOfChunks,chunksMemPtr,plrUnit);

            smemInitMemChunk(devObjPtr, chunksMemPtr, numOfChunks, &tmpUnitChunk);

            /*add the tmp unit chunks to the main unit */
            smemInitMemCombineUnitChunks(devObjPtr, unitPtr, &tmpUnitChunk);
        }
    }
}

static void smemLion3UnitIplr0
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    smemLion3UnitPolicerUnify(devObjPtr,unitPtr,SMEM_SIP5_PP_PLR_UNIT_IPLR_0_E);
}

static void smemLion3UnitIplr1
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    smemLion3UnitPolicerUnify(devObjPtr,unitPtr,SMEM_SIP5_PP_PLR_UNIT_IPLR_1_E);
}

static void smemLion3UnitEplr
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    smemLion3UnitPolicerUnify(devObjPtr,unitPtr,SMEM_SIP5_PP_PLR_UNIT_EPLR_E);
}

/*******************************************************************************
*   smemLion3UnitCnc
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the CNC unit
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemLion3UnitCnc
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr,
    IN GT_U32   cncUnitIndex
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000000, 0x00000000)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000024, 0x00000024)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000030, 0x00000030)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000040, 0x00000044)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000100, 0x00000104)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000180, 0x00000184)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000018C, 0x00000190)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000001A4, 0x000001A4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000001B8, 0x000001B8)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000001CC, 0x000001CC)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000001E0, 0x000001E4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000300, 0x00000300)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000310, 0x00000310)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001098, 0x000010D4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001298, 0x000012D4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001498, 0x00001594)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001C98, 0x00001C9C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001CA8, 0x00001CAC)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001CB8, 0x00001CBC)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001CC8, 0x00001CCC)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001CD8, 0x00001CDC)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001CE8, 0x00001CEC)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001CF8, 0x00001CFC)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001D08, 0x00001D0C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001D18, 0x00001D1C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001D28, 0x00001D2C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001D38, 0x00001D3C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001D48, 0x00001D4C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001D58, 0x00001D5C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001D68, 0x00001D6C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001D78, 0x00001D7C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001D88, 0x00001D8C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00010000, 8192 * 16), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(64,8),SMEM_BIND_TABLE_MAC(cncMemory)}
        };

        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);
        GT_U32  ii;

        if (cncUnitIndex != 0)
        {
            /* make sure that table are bound only to CNC0 (since use of 'multiple instance') */
            for(ii = 0 ; ii < numOfChunks ; ii ++)
            {
                chunksMem[ii].tableOffsetValid = 0;
                chunksMem[ii].tableOffsetInBytes = 0;
            }
        }

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }

    if(SMEM_CHT_IS_SIP5_20_GET(devObjPtr))
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001398, 0x0000139c)}
        };

        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);
        GT_U32  ii;

        if (cncUnitIndex != 0)
        {
            /* make sure that table are bound only to CNC0 (since use of 'multiple instance') */
            for(ii = 0 ; ii < numOfChunks ; ii ++)
            {
                chunksMem[ii].tableOffsetValid = 0;
                chunksMem[ii].tableOffsetInBytes = 0;
            }
        }

        smemUnitChunkAddBasicChunk(devObjPtr,unitPtr,
            ARRAY_NAME_AND_NUM_ELEMENTS_MAC(chunksMem));
    }



}

/*******************************************************************************
*   smemLion3UnitBm
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  BMA unit
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemLion3UnitBm
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    if(devObjPtr->portGroupSharedDevObjPtr == 0)  /*Bobcat2*/
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000000, 0x00000004)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000010, 0x00000018)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000020, 0x00000020)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000030, 0x00000038)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000050, 0x00000060)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000200, 0x00000228)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000300, 0x00000314)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000400, 0x00000414)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000420, 0x00000434)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000440, 0x00000454)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000460, 0x00000474)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000480, 0x00000494)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x000004A0, 0x000004A8)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x000004C0, 0x000004C8)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x000004D0, 0x000004D8)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000500, 0x000006C4)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);

        {
            SMEM_CHUNK_BASIC_STC  chunksMemBc2A0[]=
            {
                 {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00100000, 131072)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00200000, 131072)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00300000, 131072)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x000004E0, 0x000004E0)}

            };
            SMEM_CHUNK_BASIC_STC  chunksMemBc2B0[]=
            {
                {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00100000, 98304)}
               ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00200000, 98304)}
               ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00300000, 98304)}
               ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x000004E0, 0x000004E4)}
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
    else
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000000, 0x00000004)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000010, 0x00000018)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000020, 0x00000020)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000030, 0x00000034)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000050, 0x00000060)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000200, 0x00000228)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000300, 0x00000314)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000400, 0x0000040C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000420, 0x0000042C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000440, 0x0000044C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000460, 0x0000046C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000480, 0x0000048C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x000004A0, 0x000004A4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x000004C0, 0x000004C8)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x000004D0, 0x000004D8)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x000004E0, 0x000004E0)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000500, 0x0000070C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00100000, 98304)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00120000, 98304)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00140000, 98304)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00160000, 98304)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00200000, 98304)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00220000, 98304)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00240000, 98304)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00260000, 98304)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00300000, 98304)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00320000, 98304)}

            /* Add legacy registers or interrupts */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000040, 0x0000004C)}

        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }

}

/*******************************************************************************
*   smemLion3UnitBma
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  BMA unit
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemLion3UnitBma
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    if(devObjPtr->portGroupSharedDevObjPtr == 0)  /*Bobcat2*/
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000000, 32768)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00019000, 0x0001900C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00019014, 0x00019018)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00019020, 0x00019050)}
            /*Virtual => Physical source port mapping*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0001A000, 1024),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(8 , 4),SMEM_BIND_TABLE_MAC(bmaPortMapping)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0001A400, 1024)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0001A800, 1024)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0001AC00, 1024)}
        };

        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
    else /*Lion3*/
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000000, 98304)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00019000, 0x0001900C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00019014, 0x00019018)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00019020, 0x00019044)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0001904C, 0x00019058)}
            /*Virtual => Physical source port mapping*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0001A000, 1024),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(8 , 4),SMEM_BIND_TABLE_MAC(bmaPortMapping)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0001A400, 1024)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0001A800, 1024)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0001AC00, 1024)}
        };

        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
}

/*******************************************************************************
*   smemLion3UnitTti
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  TTI unit
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemLion3UnitTti
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    /* created by generate_macros.py - generated from SIP --> so 0 based addresses */
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000000, 0x00000018)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000020, 0x00000038)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000040, 0x00000068)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000170, 0x00000184)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000200, 0x00000210)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000300, 0x00000320)}
            /*DSCP To QosProfile Map - holds 12 profiles, each defining a 10bit QoSProfile per each of the 64 DSCP values : 0x400 + 0x40*p: where p (0-11) represents profile*/
#define  lion3_dscpToQoSProfile_size  128
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000800, 1536),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(640 , lion3_dscpToQoSProfile_size),SMEM_BIND_TABLE_MAC(dscpToQoSProfile)}
            /*DSCP To DSCP Map - one entry per translation profile
              Each entry holds 64 DSCP values, one per original DSCP value. */
#define  lion3_dscpToDscpMap_size  64
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000400, 768),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(384 , lion3_dscpToDscpMap_size),SMEM_BIND_TABLE_MAC(dscpToDscpMap)}
            /*EXP To QosProfile Map - Each entry represents Qos translation profile
              Each entry holds QoS profile for each of 8 EXP values*/
#define  lion3_expToQoSProfile_size  16
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000700, 192),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(80, lion3_expToQoSProfile_size),SMEM_BIND_TABLE_MAC(expToQoSProfile)}
            /*UP_CFI To QosProfile Map - Each entry holds a QoS profile per each value of {CFI,UP[2:0]} */
#define  lion3_upToQoSProfile_size  32
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000E00, 384),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(160, lion3_upToQoSProfile_size),SMEM_BIND_TABLE_MAC(upToQoSProfile)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001000, 0x000010A8)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000010B0, 0x0000112C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000013B0, 0x000013DC)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000013F4, 0x00001478)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001500, 0x00001540)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002200, 0x00002220)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002600, 0x00002620)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003000, 0x00003004)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000300C, 0x00003034)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003040, 0x0000307C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003090, 0x000030AC)}
            /* TRILL Adjacency Registers */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00006000, 0x00006BFC)}
            /*PTP Packet Command*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00004000, 8192), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(256, 32),SMEM_BIND_TABLE_MAC(ptpPacketCommandTable)}
            /* PCL User Defined Bytes Configuration Memory -- 50 udb's in each entry */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00020000, 2048), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(550, 128),SMEM_BIND_TABLE_MAC(ipclUserDefinedBytesConf)}
            /* TTI User Defined Bytes Configuration Memory -- TTI keys based on UDB's : 8 entries support 8 keys*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00040000, 1280), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(330, 64),SMEM_BIND_TABLE_MAC(ttiUserDefinedBytesConf)}
            /*TRILL RBID table -  MPLS Dual Label Lookup And RBID*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00080000, 262144), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(23, 4),SMEM_BIND_TABLE_MAC(ttiTrillRbid)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00110000, 0x00110400)}
            /*Default ePort Attributes (pre-tti lookup eport table)*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00210000, 8192), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(123, 16),SMEM_BIND_TABLE_MAC(ttiPreTtiLookupIngressEPort)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);

    }

    { /* sip5: chunks with formulas */
        {
            SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
            {
                /* MAC2ME Registers */ /* registers not table -- SMEM_BIND_TABLE_MAC(macToMe)*/
                 {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00001600,0)}, FORMULA_TWO_PARAMETERS(6 , 0x4 , 128, 0x20)}
            };

            GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
            SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

            smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

            /*add the tmp unit chunks to the main unit */
            smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);
        }
    }


    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            /* 0x010001c0,0x010001c4 - those registers added to help CPSS that hold interrupt register of "TTI Unit Interrupt Summary Mask" */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000001c0, 0x000001c4)}
            /*TRILL adjacency TCAM*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x01290000 ,
                        (LION3_NUM_TTI_TRILL_ADJACENCY_TCAM_LINES_CNS*
                         LION3_NUM_BYTES_TTI_TRILL_ADJACENCY_TCAM_ENTRY_CNS)),/*num of bytes:0x0002000*/
                        SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(64+52, LION3_NUM_BYTES_TTI_TRILL_ADJACENCY_TCAM_ENTRY_CNS),
                        SMEM_BIND_TABLE_MAC(ttiTrillAdjacencyTcam)}

        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

        smemInitMemChunk(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);
    }

    {
        SMEM_CHUNK_BASIC_STC  chunksMemBc2A0[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000070, 0x00000144)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000014C, 0x00000164)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000001A4, 0x000001AC)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000001B4, 0x000001BC)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000001C8, 0x000001DC)}
            /* VLAN Translation Table */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00014000, 16384), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(13, 4),SMEM_BIND_TABLE_MAC(ingressVlanTranslation)}
            /* Default Port Protocol eVID and QoS Configuration Table */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00008000, 32768), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4),SMEM_BIND_TABLE_MAC(portProtocolVidQoSConf)}

        };
        SMEM_CHUNK_BASIC_STC  chunksMemBc2B0[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000001C, 0x0000001C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000003C, 0x0000003C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000070, 0x00000148)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000014C, 0x00000168)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000190, 0x000001DC)}
            /* VLAN Translation Table */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00050000, 16384), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(13, 4),SMEM_BIND_TABLE_MAC(ingressVlanTranslation)}
            /* Default Port Protocol eVID and QoS Configuration Table */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00010000, 32768), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4),SMEM_BIND_TABLE_MAC(portProtocolVidQoSConf)}
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


    /* to support bobk :
       add tables that depend on number of vlans and number of eport and number
       of physical ports */
    {
        GT_U32  numPhyPorts = devObjPtr->limitedResources.phyPort;
        GT_U32  numEPorts = devObjPtr->limitedResources.ePort;

        {
            SMEM_CHUNK_BASIC_STC  chunksMem[]=
            {
                /*Physical Port Attributes*/
                {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00100000, 2048), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(56, 8),SMEM_BIND_TABLE_MAC(ttiPhysicalPortAttribute)}
            };

            if(SMEM_CHT_IS_SIP5_10_GET(devObjPtr))
            {
                chunksMem[0].enrtySizeBits = 55;
            }
            else
            if(SMEM_CHT_IS_SIP5_20_GET(devObjPtr))
            {
                chunksMem[0].enrtySizeBits = 58;
                chunksMem[0].memFirstAddr = 0x00200000;
            }


            /* number of entries : numPhyPorts . keep alignment and use for memory size */
            chunksMem[0].numOfRegisters = (numPhyPorts) * (chunksMem[0].enrtyNumBytesAlignement / 4);

            smemUnitChunkAddBasicChunk(devObjPtr,unitPtr,
                ARRAY_NAME_AND_NUM_ELEMENTS_MAC(chunksMem));
        }

        if(SMEM_CHT_IS_SIP5_10_GET(devObjPtr) == 0)
        {
            SMEM_CHUNK_BASIC_STC  chunksMemBc2A0[]=
            {
                /*ePort Attributes (post-tti lookup eport table)*/
                {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00230000, 32768), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(17, 4),SMEM_BIND_TABLE_MAC(ttiPostTtiLookupIngressEPort)}
            };

            /* number of entries : numEPorts . keep alignment and use for memory size */
            chunksMemBc2A0[0].numOfRegisters = (numEPorts) * (chunksMemBc2A0[0].enrtyNumBytesAlignement / 4);

            smemUnitChunkAddBasicChunk(devObjPtr,unitPtr,
                ARRAY_NAME_AND_NUM_ELEMENTS_MAC(chunksMemBc2A0));
        }
        else
        {
            SMEM_CHUNK_BASIC_STC  chunksMemBc2B0[]=
            {
                /*ePort Attributes (post-tti lookup eport table)*/
                {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00240000, 32768), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(17, 4),SMEM_BIND_TABLE_MAC(ttiPostTtiLookupIngressEPort)}
            };
            /* number of entries : numEPorts . keep alignment and use for memory size */
            chunksMemBc2B0[0].numOfRegisters = (numEPorts) * (chunksMemBc2B0[0].enrtyNumBytesAlignement / 4);

            smemUnitChunkAddBasicChunk(devObjPtr,unitPtr,
                ARRAY_NAME_AND_NUM_ELEMENTS_MAC(chunksMemBc2B0));
        }
    }
}

/*******************************************************************************
*   smemLion3UnitL2i
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Lion3  L2i unit
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
static void smemLion3UnitL2i
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000000, 0x00000004)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000020, 0x0000002C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000040, 0x00000048)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000120, 0x00000120)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000130, 0x00000130)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000140, 0x0000014C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000160, 0x00000164)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000170, 0x00000170)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000180, 0x0000019C)}

            /*,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000600, 0x0000063C)} --> see ieeeRsrvMcCpuIndex */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000700, 0x0000071C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000900, 0x0000093C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000C00, 0x00000C3C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000D00, 0x00000D1C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000E00, 0x00000E08)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001000, 0x00001018)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001100, 0x0000112C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001200, 0x0000120C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001220, 0x00001224)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001300, 0x00001308)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001400, 0x0000140C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001500, 0x00001508)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001510, 0x00001514)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001520, 0x00001528)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001530, 0x00001544)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001550, 0x00001564)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001600, 0x0000161C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002000, 0x00002000)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002010, 0x00002010)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002020, 0x00002020)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002100, 0x00002104)}
            /* Source Trunk Attribute Table */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00202000 ,4096 ),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(40, 8),SMEM_BIND_TABLE_MAC(bridgeIngressTrunk)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

        smemInitMemChunk(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);
    }

    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
            /* IEEE Reserved Multicast Configuration register */ /* register and not table SMEM_BIND_TABLE_MAC(ieeeRsrvMcConfTable) */
             {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00000200 ,0),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4),SMEM_BIND_TABLE_MAC(ieeeRsrvMcConfTable)}, FORMULA_TWO_PARAMETERS(16 , 0x4 , 8 ,0x80)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00000600 ,0),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4),SMEM_BIND_TABLE_MAC(ieeeRsrvMcCpuIndex)}, FORMULA_SINGLE_PARAMETER(16 , 0x4)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);
    }
    {
        SMEM_CHUNK_BASIC_STC  chunksMemBc2A0[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000800, 0x0000081C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000A00, 0x00000A1C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000B00, 0x00000B1C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001240, 0x000012BC)}
        };

        SMEM_CHUNK_BASIC_STC  chunksMemBc2B0[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000800, 0x00000880)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000A00, 0x00000A80)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000B00, 0x00000B80)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001240, 0x000012DC)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002120, 0x00002120)}
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

    /* to support bobk :
       add tables that depend on number of vlans and number of eport and number
       of physical ports */
    {
        GT_U32  numPhyPorts = devObjPtr->limitedResources.phyPort;
        GT_U32  numEPorts = devObjPtr->limitedResources.ePort;
        GT_U32  numEVlans = devObjPtr->limitedResources.eVid;
        GT_U32  numStg = 1 << devObjPtr->flexFieldNumBitsSupport.stgId;

        {
            SMEM_CHUNK_BASIC_STC  chunksMem[]=
            {
                /*Ingress Bridge physical Port Table*/
                 {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00200000 ,1024 ), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(29, 4),SMEM_BIND_TABLE_MAC(bridgePhysicalPortEntry)}
                /*Ingress Bridge physical Port Rate Limit Counters Table*/
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00201000 ,1024 ), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(22, 4),SMEM_BIND_TABLE_MAC(bridgePhysicalPortRateLimitCountersEntry)}
            };

            /* number of entries : numPhyPorts . keep alignment and use for memory size */
            chunksMem[0].numOfRegisters = numPhyPorts * (chunksMem[0].enrtyNumBytesAlignement / 4);
            /* number of entries : numPhyPorts . keep alignment and use for memory size*/
            chunksMem[1].numOfRegisters = numPhyPorts * (chunksMem[1].enrtyNumBytesAlignement / 4);

            smemUnitChunkAddBasicChunk(devObjPtr,unitPtr,
                ARRAY_NAME_AND_NUM_ELEMENTS_MAC(chunksMem));
        }

        {
            SMEM_CHUNK_BASIC_STC  chunksMem[]=
            {
                /* ingress EPort learn prio Table */
                {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00210000 ,8192 ),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(40, 8),SMEM_BIND_TABLE_MAC(bridgeIngressEPortLearnPrio)}
            };

            /* number of entries : numEPorts/8 . keep alignment and use for memory size */
            chunksMem[0].numOfRegisters = (numEPorts / 8) * (chunksMem[0].enrtyNumBytesAlignement / 4);

            smemUnitChunkAddBasicChunk(devObjPtr,unitPtr,
                ARRAY_NAME_AND_NUM_ELEMENTS_MAC(chunksMem));
        }

        {
            SMEM_CHUNK_BASIC_STC  chunksMem[]=
            {
                /* Ingress Spanning Tree State Table */
                {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00240000 , 262144 ),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(512, 64),SMEM_BIND_TABLE_MAC(stp)}
            };
            GT_U32  numLines = numStg;

            /* each entry : 2 bits per physical port */
            chunksMem[0].enrtySizeBits = (2 * numPhyPorts);

            if(SMEM_CHT_IS_SIP5_20_GET(devObjPtr))
            {
                /* there are 2 'entries' per line */
                numLines /= 2;
            }

            /* alignment is according to entry size */
            chunksMem[0].enrtyNumBytesAlignement = ((chunksMem[0].enrtySizeBits + 31)/32)*4;
            /* number of entries : numStg . use calculated alignment and use for memory size */
            chunksMem[0].numOfRegisters = (numLines) * (chunksMem[0].enrtyNumBytesAlignement / 4);

            smemUnitChunkAddBasicChunk(devObjPtr,unitPtr,
                ARRAY_NAME_AND_NUM_ELEMENTS_MAC(chunksMem));
        }

        {
            SMEM_CHUNK_BASIC_STC  chunksMem[]=
            {
                /* Ingress Port Membership Table */
                {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00280000 , 262144), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(256 , 32),SMEM_BIND_TABLE_MAC(bridgeIngressPortMembership)}
            };

            GT_U32  numLines = numEVlans;

            /* each entry : 1 bit per physical port */
            chunksMem[0].enrtySizeBits = numPhyPorts;
            if(SMEM_CHT_IS_SIP5_20_GET(devObjPtr))
            {
                /* there are 2 'entries' per line */
                numLines /= 2;
            }
            /* alignment is according to entry size */
            chunksMem[0].enrtyNumBytesAlignement = ((chunksMem[0].enrtySizeBits + 31)/32)*4;
            /* number of entries : numEVlans . use calculated alignment and use for memory size */
            chunksMem[0].numOfRegisters = (numLines ) * (chunksMem[0].enrtyNumBytesAlignement / 4) ;

            smemUnitChunkAddBasicChunk(devObjPtr,unitPtr,
                ARRAY_NAME_AND_NUM_ELEMENTS_MAC(chunksMem));
        }

        {
            SMEM_CHUNK_BASIC_STC  chunksMem[]=
            {
                /* Ingress Vlan Table */
                {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x002C0000 , 131072), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(113 , 16),SMEM_BIND_TABLE_MAC(vlan)}
            };

            /* number of entries : numEVlans . keep alignment and use for memory size */
            chunksMem[0].numOfRegisters = (numEVlans ) * (chunksMem[0].enrtyNumBytesAlignement / 4) ;

            smemUnitChunkAddBasicChunk(devObjPtr,unitPtr,
                ARRAY_NAME_AND_NUM_ELEMENTS_MAC(chunksMem));
        }

        if(SMEM_CHT_IS_SIP5_10_GET(devObjPtr) == 0)
        {
            SMEM_CHUNK_BASIC_STC  chunksMemBc2A0[]=
            {
                /*Bridge Ingress ePort Table*/
                 {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00220000 ,131072), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(79, 16),SMEM_BIND_TABLE_MAC(bridgeIngressEPort)}
                /* Ingress Span State Group Index Table */
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x002E0000 , 32768), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(12 , 4),SMEM_BIND_TABLE_MAC(ingressSpanStateGroupIndex)}
            };
            /* number of entries : numEPorts . keep alignment and use for memory size */
            chunksMemBc2A0[0].numOfRegisters = (numEPorts ) * (chunksMemBc2A0[0].enrtyNumBytesAlignement / 4) ;
            /* number of entries : numEVlans . keep alignment and use for memory size */
            chunksMemBc2A0[1].numOfRegisters = (numEVlans ) * (chunksMemBc2A0[1].enrtyNumBytesAlignement / 4) ;
            /* number of bits : stgId */
            chunksMemBc2A0[1].enrtySizeBits = devObjPtr->flexFieldNumBitsSupport.stgId;

            smemUnitChunkAddBasicChunk(devObjPtr,unitPtr,
                ARRAY_NAME_AND_NUM_ELEMENTS_MAC(chunksMemBc2A0));
        }
        else
        {
            SMEM_CHUNK_BASIC_STC  chunksMemBc2B0[]=
            {
                /*Bridge Ingress ePort Table*/
                 {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00300000 ,131072), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(79, 16),SMEM_BIND_TABLE_MAC(bridgeIngressEPort)}
                /* Ingress Span State Group Index Table */
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00218000 , 32768), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(12 , 4),SMEM_BIND_TABLE_MAC(ingressSpanStateGroupIndex)}
            };
            /* number of entries : numEPorts . keep alignment and use for memory size */
            chunksMemBc2B0[0].numOfRegisters = (numEPorts ) * (chunksMemBc2B0[0].enrtyNumBytesAlignement / 4) ;
            /* number of entries : numEVlans . keep alignment and use for memory size */
            chunksMemBc2B0[1].numOfRegisters = (numEVlans ) * (chunksMemBc2B0[1].enrtyNumBytesAlignement / 4) ;
            /* number of bits : stgId */
            chunksMemBc2B0[1].enrtySizeBits = devObjPtr->flexFieldNumBitsSupport.stgId;
            smemUnitChunkAddBasicChunk(devObjPtr,unitPtr,
                ARRAY_NAME_AND_NUM_ELEMENTS_MAC(chunksMemBc2B0));
        }

    }


}

/*******************************************************************************
*   smemLion3UnitFdb
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the LION3  FDB unit
*
* INPUTS:
*       devObjPtr   - pointer to device memory.
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
static void smemLion3UnitFdb
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    /* single instance of the unit --- in core 0 */
    SINGLE_INSTANCE_UNIT_MAC(devObjPtr,LION3_UNIT_FDB_TABLE_SINGLE_INSTANCE_PORT_GROUP_CNS);

    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000000, 0x00000004)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000000C, 0x00000014)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000020, 0x0000003C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000060, 0x00000060)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000070, 0x00000070)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000090, 0x00000094)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000000C0, 0x000000C0)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000000D0, 0x000000E4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000110, 0x00000120)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000130, 0x00000144)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000150, 0x00000150)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000170, 0x00000174)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000001B0, 0x000001B4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000200, 0x00000280)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000300, 0x00000324)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000340, 0x00000340)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000380, 0x00000384)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000400, 0x00000400)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000410, 0x00000430)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000480, 0x00000488)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
}

/*******************************************************************************
*   smemLion3UnitFdbTable
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the LION3 FDB TABLE unit
*
* INPUTS:
*       devObjPtr   - pointer to device memory.
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
static void smemLion3UnitFdbTable
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
#define  fdbEntryNumBits        138
#define  fdbEntryNumBytes       ((fdbEntryNumBits + 7)/8)
#define  fdbEntryNumBytes_align (fdbEntryNumBytes <= 16 ? 16 : 32)
#define  fdbSizeNumBytes  ((SMEM_MAC_TABLE_SIZE_256KB) * fdbEntryNumBytes_align)

    /* single instance of the unit --- in core 0 */
    SINGLE_INSTANCE_UNIT_MAC(devObjPtr,LION3_UNIT_FDB_TABLE_SINGLE_INSTANCE_PORT_GROUP_CNS);

    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            /*FDB table*/
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0 ,fdbSizeNumBytes), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(fdbEntryNumBits, fdbEntryNumBytes_align),SMEM_BIND_TABLE_MAC(fdb)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
}

/*******************************************************************************
*   smemLion3UnitHa
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Lion3  HA unit
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
static void smemLion3UnitHa
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{

    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            /* registers */

             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000000, 0x00000058)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000060, 0x0000006c)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000080, 0x0000009C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000100, 0x00000144)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000150, 0x0000016C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000300, 0x00000304)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000003C0, 0x000003C4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000003D0, 0x000003D0)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000408, 0x00000410)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000420, 0x00000424)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000430, 0x00000434)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000500, 0x0000051C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000550, 0x00000560)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000710, 0x00000710)}

            /* memories */

            /*HA Global MAC SA Table*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00003000 , 2048 ), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(48, 8) , SMEM_BIND_TABLE_MAC(haGlobalMacSa)}
            /*HA QoS Profile to EXP Table*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00004000 , 4096 ), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(3, 4) , SMEM_BIND_TABLE_MAC(haQosProfileToExp)}
            /*VLAN MAC SA Table (was 'VLAN/Port MAC SA Table' in legacy device)*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00020000, 16384), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(8, 4) , SMEM_BIND_TABLE_MAC(vlanPortMacSa)}
            /*Generic TS Profile table*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00050000, 1024), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(534,128), SMEM_BIND_TABLE_MAC(tunnelStartGenericIpProfile) }

            /*EPCL User Defined Bytes Configuration Memory*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00060000, 2048), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(600,128) , SMEM_BIND_TABLE_MAC(haEpclUserDefinedBytesConfig)}

        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }


    {
        SMEM_CHUNK_BASIC_STC  chunksMemBc2B0[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000070, 0x00000070)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000308, 0x00000308)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000314, 0x00000314)}
        };

        if(SMEM_CHT_IS_SIP5_10_GET(devObjPtr) == 0)
        {
        }
        else
        {
            smemUnitChunkAddBasicChunk(devObjPtr,unitPtr,
                ARRAY_NAME_AND_NUM_ELEMENTS_MAC(chunksMemBc2B0));
        }
    }
    /* to support bobk :
       add tables that depend on number of vlans and number of eport and number
       of physical ports */
    {
        GT_U32  numPhyPorts = devObjPtr->limitedResources.phyPort;
        GT_U32  numEPorts = devObjPtr->limitedResources.ePort;
        GT_U32  numEVlans = devObjPtr->limitedResources.eVid;
        GT_U32  numTs = 1 << devObjPtr->flexFieldNumBitsSupport.tunnelstartPtr;
        GT_U32  index = 0;
        {
            SMEM_CHUNK_BASIC_STC  chunksMem[]=
            {
                /*HA Physical Port Table 1*/
                 {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00001000, 1024), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(27, 4) , SMEM_BIND_TABLE_MAC(haEgressPhyPort1)}
                /*HA Physical Port Table 2*/
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00002000, 4096), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(77, 16) , SMEM_BIND_TABLE_MAC(haEgressPhyPort2)}
            };

            /* number of entries : numPhyPorts . keep alignment and use for memory size */
            chunksMem[index].numOfRegisters = numPhyPorts * (chunksMem[index].enrtyNumBytesAlignement / 4);
            index++;
            /* number of entries : numPhyPorts . keep alignment and use for memory size */
            chunksMem[index].numOfRegisters = numPhyPorts * (chunksMem[index].enrtyNumBytesAlignement / 4);
            index++;

            smemUnitChunkAddBasicChunk(devObjPtr,unitPtr,
                ARRAY_NAME_AND_NUM_ELEMENTS_MAC(chunksMem));
        }

        {
            SMEM_CHUNK_BASIC_STC  chunksMem[]=
            {
                /*EVLAN Table (was 'vlan translation' in legacy device)*/
                 {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00010000, 65536), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(36, 8) , SMEM_BIND_TABLE_MAC(egressVlanTranslation)}
            };

            index = 0;
            /* number of entries : numEVlans . keep alignment and use for memory size */
            chunksMem[index].numOfRegisters = numEVlans * (chunksMem[index].enrtyNumBytesAlignement / 4);
            index++;

            smemUnitChunkAddBasicChunk(devObjPtr,unitPtr,
                ARRAY_NAME_AND_NUM_ELEMENTS_MAC(chunksMem));
        }

        {
            SMEM_CHUNK_BASIC_STC  chunksMem[]=
            {
                /*PTP Domain table*/
                {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00040000, 10240) , SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(35, 8) , SMEM_BIND_TABLE_MAC(haPtpDomain)}
            };

            index = 0;
            /* number of entries : (numPhyPorts * 5) . keep alignment and use for memory size */
            chunksMem[index].numOfRegisters = (numPhyPorts * 5) * (chunksMem[index].enrtyNumBytesAlignement / 4);
            index++;

            smemUnitChunkAddBasicChunk(devObjPtr,unitPtr,
                ARRAY_NAME_AND_NUM_ELEMENTS_MAC(chunksMem));
        }


        if(SMEM_CHT_IS_SIP5_10_GET(devObjPtr) == 0)
        {
            SMEM_CHUNK_BASIC_STC  chunksMemBc2A0[]=
            {
                /*HA Egress ePort Attribute Table 1*/
                 {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00100000, 131072), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(87,16) , SMEM_BIND_TABLE_MAC(haEgressEPortAttr1)}
                /*HA Egress ePort Attribute Table 2*/
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00030000, 32768), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(24, 4) , SMEM_BIND_TABLE_MAC(haEgressEPortAttr2)}

            };
            index = 0;
            /* number of entries : numEPorts . keep alignment and use for memory size */
            chunksMemBc2A0[index].numOfRegisters = numEPorts * (chunksMemBc2A0[index].enrtyNumBytesAlignement / 4);
            index++;
            /* number of entries : numEPorts . keep alignment and use for memory size */
            chunksMemBc2A0[index].numOfRegisters = numEPorts * (chunksMemBc2A0[index].enrtyNumBytesAlignement / 4);
            index++;

            smemUnitChunkAddBasicChunk(devObjPtr,unitPtr,
                ARRAY_NAME_AND_NUM_ELEMENTS_MAC(chunksMemBc2A0));
        }
        else
        {
            SMEM_CHUNK_BASIC_STC  chunksMemBc2B0[]=
            {
                /*HA Egress ePort Attribute Table 1*/
                 {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00100000, 131072), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(85,16) , SMEM_BIND_TABLE_MAC(haEgressEPortAttr1)}
                /*HA Egress ePort Attribute Table 2*/
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00200000, 32768), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(25, 4) , SMEM_BIND_TABLE_MAC(haEgressEPortAttr2)}
            };
            index = 0;

            if(SMEM_CHT_IS_SIP5_15_GET(devObjPtr))
            {
                chunksMemBc2B0[index].enrtySizeBits = 86;
            }

            /* number of entries : numEPorts . keep alignment and use for memory size */
            chunksMemBc2B0[index].numOfRegisters = numEPorts * (chunksMemBc2B0[index].enrtyNumBytesAlignement / 4);
            index++;
            /* number of entries : numEPorts . keep alignment and use for memory size */
            chunksMemBc2B0[index].numOfRegisters = numEPorts * (chunksMemBc2B0[index].enrtyNumBytesAlignement / 4);
            index++;
            smemUnitChunkAddBasicChunk(devObjPtr,unitPtr,
                ARRAY_NAME_AND_NUM_ELEMENTS_MAC(chunksMemBc2B0));
        }

        if(SMEM_CHT_IS_SIP5_10_GET(devObjPtr) == 0)
        {
            SMEM_CHUNK_BASIC_STC  chunksMemBc2A0[]=
            {
                /*Router ARP DA and Tunnel Start Table*/
                {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00200000, 1048576), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(384,64) , SMEM_BIND_TABLE_MAC(arp)/*tunnelStart*/}
            };
            index = 0;
            /* number of entries : (2*numTs) . keep alignment and use for memory size */
            chunksMemBc2A0[index].numOfRegisters = (2*numTs) * (chunksMemBc2A0[index].enrtyNumBytesAlignement / 4);
            index++;

            smemUnitChunkAddBasicChunk(devObjPtr,unitPtr,
                ARRAY_NAME_AND_NUM_ELEMENTS_MAC(chunksMemBc2A0));
        }
        else
        {
            SMEM_CHUNK_BASIC_STC  chunksMemBc2B0[]=
            {
                /*Router ARP DA and Tunnel Start Table*/
                {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00400000, 1048576), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(384,64) , SMEM_BIND_TABLE_MAC(arp)/*tunnelStart*/}
            };
            index = 0;
            /* number of entries : (2*numTs) . keep alignment and use for memory size */
            chunksMemBc2B0[index].numOfRegisters = (2*numTs) * (chunksMemBc2B0[index].enrtyNumBytesAlignement / 4);
            index++;

            smemUnitChunkAddBasicChunk(devObjPtr,unitPtr,
                ARRAY_NAME_AND_NUM_ELEMENTS_MAC(chunksMemBc2B0));
        }

    }

}

/*******************************************************************************
*   smemLion3UnitRxDma
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Lion3  RX DMA unit
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
static void smemLion3UnitRxDma
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000000, 0x00000040)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000084, 0x00000088)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000000A0, 0x000000A4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003500, 0x00003620)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003a00, 0x00003b20)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
    {
        SMEM_CHUNK_BASIC_STC  chunksMemBc2A0[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000000d0, 0x000000e0)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000304, 0x00000424)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000450, 0x00000570)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000600, 0x00000720)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000900, 0x00000a28)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000d0c, 0x00000e2c)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001028, 0x0000104c)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001200, 0x0000120c)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001220, 0x00001234)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000123c, 0x0000126c)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001320, 0x00001320)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001330, 0x00001334)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001340, 0x00001340)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001364, 0x00001368)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001600, 0x00001720)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000014b0, 0x000014b4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001900, 0x00001a20)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002500, 0x0000265c)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002660, 0x00002660)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001bf0, 0x00001bf0)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001c00, 0x00001d20)}
        };
        SMEM_CHUNK_BASIC_STC  chunksMemBc2B0[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000000b0, 0x000000b4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000000c4, 0x000000d0)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000000e4, 0x00000160)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000174, 0x00000180)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000198, 0x00000294)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000002f0, 0x000002fc)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000320, 0x0000032c)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000350, 0x00000350)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000374, 0x00000374)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000398, 0x000003ac)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000003c8, 0x000003d4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000003e8, 0x000003f4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000408, 0x00000414)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000428, 0x00000434)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000043c, 0x00000448)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000450, 0x0000045c)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000004d0, 0x000005cc)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000630, 0x0000063c)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000710, 0x00000714)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000720, 0x00000724)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000730, 0x00000734)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000740, 0x00000740)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000005d0, 0x000005e0)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000804, 0x00000924)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000950, 0x00000a70)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000b00, 0x00000c20)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000e00, 0x00000f28)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000120c, 0x0000132c)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001528, 0x0000154c)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001700, 0x0000170c)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001720, 0x00001738)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000173c, 0x0000176c)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001820, 0x00001820)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001830, 0x00001834)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001840, 0x00001840)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001850, 0x00001858)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001864, 0x00001870)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000019b0, 0x000019b4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001b00, 0x00001c20)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001e00, 0x00001f20)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000020f0, 0x000020f0)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002100, 0x00002220)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002400, 0x00002520)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002a00, 0x00002b58)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002b60, 0x00002b60)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00004080, 0x00004084)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000040a0, 0x000040a4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000040c0, 0x000040c0)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00005000, 0x00005000)}
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
*   smemLion3UnitEq
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Lion3  Eq unit
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr     - pointer to the unit chunk
*       eqPart      - part of the EQ unit (on Lion3 the EQ hold parts 0,1)
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemLion3UnitEq
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{

    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000001C, 0x0000001C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000003C, 0x00000040)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000058, 0x0000005C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000068, 0x0000007C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000088, 0x0000008C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000000A0, 0x000000A4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000110, 0x0000011C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000600, 0x00000604)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00005000, 0x00005000)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00005010, 0x00005018)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00005020, 0x00005024)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00005030, 0x00005030)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00005050, 0x00005054)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00007000, 0x0000703C),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4),SMEM_BIND_TABLE_MAC(tcpUdpDstPortRangeCpuCode)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00007800, 0x0000783C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00007C00, 0x00007C10),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4),SMEM_BIND_TABLE_MAC(ipProtCpuCode)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000A000, 0x0000A008)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000A010, 0x0000A010)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000A020, 0x0000A034)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000A100, 0x0000A11C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000AA00, 0x0000AA08)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000AB00, 0x0000AB5C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000AC00, 0x0000AC08)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000AD00, 0x0000AD70)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000AF00, 0x0000AF08)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000AF10, 0x0000AF10)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000AF30, 0x0000AF30)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000B000, 0x0000B008)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000B200, 0x0000B218)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0000B300, 128),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4),SMEM_BIND_TABLE_MAC(eqPhysicalPortIngressMirrorIndexTable)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000D000, 0x0000D004)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000D010, 0x0000D010)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00010000, 0x00010004)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0002000C, 0x00020010)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00040000, 4096) }/*SMEM_BIND_TABLE_MAC(ingrStc)*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00080000, 0x000803F8)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00100000, 1024),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4),SMEM_BIND_TABLE_MAC(statisticalRateLimit)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00110000, 4096),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(14, 4),SMEM_BIND_TABLE_MAC(qosProfile)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00120000, 1024),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4),SMEM_BIND_TABLE_MAC(cpuCode)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00130000, 1024)}

            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00160000, 16384),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(27, 4),SMEM_BIND_TABLE_MAC(eqTrunkLtt)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00B00000, 256),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4),SMEM_BIND_TABLE_MAC(oamProtectionLocStatusTable)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }

    /* chunks with formulas */
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
             {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00000620 ,0)}, FORMULA_TWO_PARAMETERS(2,0x4, 16,0x10)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00010020 ,0)}, FORMULA_TWO_PARAMETERS(2,0x4, 16,0x10)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);
    }

    /* to support bobk :
       add tables that depend on number of vlans and number of eport and number
       of physical ports */
    {
        GT_U32  numEPorts = devObjPtr->limitedResources.ePort;
        GT_U32  index = 0;
        {
            SMEM_CHUNK_BASIC_STC  chunksMem[]=
            {
                 {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00700000, 8192),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4),SMEM_BIND_TABLE_MAC(eqIngressEPort)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00800000, 32768),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(27, 4),SMEM_BIND_TABLE_MAC(eqL2EcmpLtt)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00A00000, 1024),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4),SMEM_BIND_TABLE_MAC(txProtectionSwitchingTable)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00A80000, 16384),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(22, 4),SMEM_BIND_TABLE_MAC(ePortToLocMappingTable)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00C00000, 32768),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(22, 4),SMEM_BIND_TABLE_MAC(eqE2Phy)}
            };

            /* number of entries : (numEPorts/4) . keep alignment and use for memory size */
            chunksMem[index].numOfRegisters = (numEPorts / 4) * (chunksMem[index].enrtyNumBytesAlignement / 4);
            index++;
            /* number of entries : numEPorts . keep alignment and use for memory size */
            chunksMem[index].numOfRegisters = numEPorts  * (chunksMem[index].enrtyNumBytesAlignement / 4);
            index++;
            /* number of entries : (numEPorts/32) . keep alignment and use for memory size */
            chunksMem[index].numOfRegisters = (numEPorts / 32) * (chunksMem[index].enrtyNumBytesAlignement / 4);
            index++;
            /* number of entries : (numEPorts/2) . keep alignment and use for memory size */
            chunksMem[index].numOfRegisters = (numEPorts / 2) * (chunksMem[index].enrtyNumBytesAlignement / 4);
            index++;
            /* number of entries : numEPorts . keep alignment and use for memory size */
            chunksMem[index].numOfRegisters = numEPorts  * (chunksMem[index].enrtyNumBytesAlignement / 4);
            index++;

            smemUnitChunkAddBasicChunk(devObjPtr,unitPtr,
                ARRAY_NAME_AND_NUM_ELEMENTS_MAC(chunksMem));
        }

        {
            SMEM_CHUNK_BASIC_STC  chunksMem[]=
            {
                {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00900000, 32768),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(23, 4),SMEM_BIND_TABLE_MAC(eqL2Ecmp)}
            };

            /* number of entries : l2Ecmp . keep alignment and use for memory size */
            chunksMem[0].numOfRegisters = devObjPtr->limitedResources.l2Ecmp * (chunksMem[0].enrtyNumBytesAlignement / 4);

            smemUnitChunkAddBasicChunk(devObjPtr,unitPtr,
                ARRAY_NAME_AND_NUM_ELEMENTS_MAC(chunksMem));
        }

    }

}

/*******************************************************************************
*   smemLion3UnitIpcl
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Lion3 IPCL unit
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
static void smemLion3UnitIpcl
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000001C, 0x00000048)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000070, 0x00000078)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000080, 0x0000009C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000005C0, 0x000005FC),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4),SMEM_BIND_TABLE_MAC(pearsonHash)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000730, 0x0000073C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000744, 0x0000077C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000800, 0x00000850)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000C00, 512),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(74, 16),SMEM_BIND_TABLE_MAC(crcHashMask)}
            /* next are set below as formula of 3 tables
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00010000, 17408)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00020000, 17408)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00030000, 17408)}
            */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00040000, 6144),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(316, 64),SMEM_BIND_TABLE_MAC(ipcl0UdbSelect)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00042000, 6144),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(316, 64),SMEM_BIND_TABLE_MAC(ipcl1UdbSelect)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00044000, 6144),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(316, 64),SMEM_BIND_TABLE_MAC(ipcl2UdbSelect)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }

    {
        SMEM_CHUNK_BASIC_STC  chunksMemBc2A0[]=
        {
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000000, 0x0000000C)}
           ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000014, 0x00000014)}
        };

        SMEM_CHUNK_BASIC_STC  chunksMemBc2B0[]=
        {
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000000, 0x00000010)}
           ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000014, 0x00000018)}
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

    /* to support bobk :
       add tables that depend on number of vlans and number of eport and number
       of physical ports */
    /* chunks with formulas */
    {
        GT_U32  numPhyPorts = devObjPtr->limitedResources.phyPort;

        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
            /* IPCL0,IPCL1,IPCL2 Configuration Table */
            {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00010000 ,17408), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(27, 4),SMEM_BIND_TABLE_MAC(pclConfig)}, FORMULA_SINGLE_PARAMETER(3 , 0x00010000)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

        /* number of entries : 4K + numPhyPorts . keep alignment and use for memory size */
        chunksMem[0].memChunkBasic.numOfRegisters = ((4*1024) + numPhyPorts) * (chunksMem[0].memChunkBasic.enrtyNumBytesAlignement / 4);

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);
    }
}

/*******************************************************************************
*   smemLion3UnitMll
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  MLL unit
*
* INPUTS:
*       commonDevMemInfoPtr   - pointer to common device memory object.
*       unitPtr - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemLion3UnitMll
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{

    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000000, 0x00000018)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000020, 0x00000024)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000030, 0x00000034)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000040, 0x00000040)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000050, 0x00000058)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000060, 0x00000060)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000100, 0x00000204)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000210, 0x00000220)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000300, 0x0000030C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000400, 0x00000404)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000800, 0x00000804)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000900, 0x00000900)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000980, 0x00000984)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000A00, 0x00000A00)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000A80, 0x00000A84)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000B04, 0x00000B04)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000C00, 0x00000C00)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000C80, 0x00000C84)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000D00, 0x00000D00)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000D80, 0x00000D84)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000E00, 0x00000E08)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000E20, 0x00000E28)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }

    /* to support bobk :
       add tables that depend on number of vlans and number of eport and number
       of physical ports */
    {
        GT_U32  numMllPairs = devObjPtr->limitedResources.mllPairs;
        GT_U32  numL2LttMll = devObjPtr->limitedResources.l2LttMll;

        GT_U32  index;
        {
            SMEM_CHUNK_BASIC_STC  chunksMem[]=
            {
                {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00040000, 131072), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(20, 4),SMEM_BIND_TABLE_MAC(l2MllLtt)}
            };

            index = 0;
            /* number of entries : numL2LttMll . keep alignment and use for memory size */
            chunksMem[index].numOfRegisters = numL2LttMll * (chunksMem[index].enrtyNumBytesAlignement / 4);
            index++;

            smemUnitChunkAddBasicChunk(devObjPtr,unitPtr,
                ARRAY_NAME_AND_NUM_ELEMENTS_MAC(chunksMem));
        }
        {
            SMEM_CHUNK_BASIC_STC  chunksMem[]=
            {
                {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00080000, 524288), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(158, 32) , SMEM_BIND_TABLE_MAC(mll)}
            };

            index = 0;
            /* number of entries : numL2LttMll . keep alignment and use for memory size */
            chunksMem[index].numOfRegisters = numMllPairs * (chunksMem[index].enrtyNumBytesAlignement / 4);
            /* update base address too */
            chunksMem[index].memFirstAddr = SMEM_CHT_IS_SIP5_15_GET(devObjPtr) ? 0x00100000 : 0x00080000;
            index++;

            smemUnitChunkAddBasicChunk(devObjPtr,unitPtr,
                ARRAY_NAME_AND_NUM_ELEMENTS_MAC(chunksMem));
        }
    }

}

/*******************************************************************************
*   smemLion3UnitEpcl
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Lion3 EPCL unit
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
static void smemLion3UnitEpcl
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000030, 0x00000040)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000074, 0x00000074)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000000A0, 0x000000A0)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000000C0, 0x000000C0)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000210, 0x00000214)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000220, 0x00000220)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000250, 0x00000254)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00010000, 6144) ,SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(306, 64),SMEM_BIND_TABLE_MAC(epclUdbSelect)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }

    {
        SMEM_CHUNK_BASIC_STC  chunksMemBc2A0[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000000, 0x00000014)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000020, 0x00000020)}
        };
        SMEM_CHUNK_BASIC_STC  chunksMemBc2B0[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000000, 0x00000020)}
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
    /* to support bobk :
       add tables that depend on number of vlans and number of eport and number
       of physical ports */
    {
        GT_U32  numPhyPorts = devObjPtr->limitedResources.phyPort;
        {
            SMEM_CHUNK_BASIC_STC  chunksMem[]=
            {
                {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00008000, 17408),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(27, 4),SMEM_BIND_TABLE_MAC(epclConfigTable)}
            };

            /* number of entries : 4K + numPhyPorts . keep alignment and use for memory size */
            chunksMem[0].numOfRegisters = ((4*1024) + numPhyPorts) * (chunksMem[0].enrtyNumBytesAlignement / 4);

            smemUnitChunkAddBasicChunk(devObjPtr,unitPtr,
                ARRAY_NAME_AND_NUM_ELEMENTS_MAC(chunksMem));
        }
    }



}

/*******************************************************************************
*   smemLion3UnitGop
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Lion3 NEW GOP unit
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
static void smemLion3UnitGop
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    GT_U32  baseAddr = 0x10000000;
    GT_U32  ii;

    /* chunks with formulas */
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
            /* Gig Ports*/
                /* ports 0..55 */
                 {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000000, 0x00000094)} , FORMULA_SINGLE_PARAMETER(56 , 0x1000)}
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x000000A0, 0x000000A4)} , FORMULA_SINGLE_PARAMETER(56 , 0x1000)}

                /* ports 56..71 */
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000000 + BOBCAT2_PORT_56_START_OFFSET_CNS, 0x00000094 + BOBCAT2_PORT_56_START_OFFSET_CNS)} , FORMULA_SINGLE_PARAMETER(16 , 0x1000)}
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x000000A0 + BOBCAT2_PORT_56_START_OFFSET_CNS, 0x000000A4 + BOBCAT2_PORT_56_START_OFFSET_CNS)} , FORMULA_SINGLE_PARAMETER(16 , 0x1000)}

            /* MPCS */
                /* ports 48..55 */
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00180008 + BOBCAT2_PORT_48_START_OFFSET_CNS, 0x00180014 + BOBCAT2_PORT_48_START_OFFSET_CNS)}, FORMULA_SINGLE_PARAMETER(8  , 0x1000)}
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00180030 + BOBCAT2_PORT_48_START_OFFSET_CNS, 0x00180030 + BOBCAT2_PORT_48_START_OFFSET_CNS)}, FORMULA_SINGLE_PARAMETER(8  , 0x1000)}
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0018003C + BOBCAT2_PORT_48_START_OFFSET_CNS, 0x001800C8 + BOBCAT2_PORT_48_START_OFFSET_CNS)}, FORMULA_SINGLE_PARAMETER(8  , 0x1000)}
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x001800D0 + BOBCAT2_PORT_48_START_OFFSET_CNS, 0x00180120 + BOBCAT2_PORT_48_START_OFFSET_CNS)}, FORMULA_SINGLE_PARAMETER(8  , 0x1000)}
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00180128 + BOBCAT2_PORT_48_START_OFFSET_CNS, 0x0018014C + BOBCAT2_PORT_48_START_OFFSET_CNS)}, FORMULA_SINGLE_PARAMETER(8  , 0x1000)}
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0018015C + BOBCAT2_PORT_48_START_OFFSET_CNS, 0x0018017C + BOBCAT2_PORT_48_START_OFFSET_CNS)}, FORMULA_SINGLE_PARAMETER(8  , 0x1000)}
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00180200 + BOBCAT2_PORT_48_START_OFFSET_CNS, 256)}                                         , FORMULA_SINGLE_PARAMETER(8  , 0x1000)}

                /* ports 56..71 */
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00180008 + BOBCAT2_PORT_56_START_OFFSET_CNS, 0x00180014 + BOBCAT2_PORT_56_START_OFFSET_CNS)}, FORMULA_SINGLE_PARAMETER(16 , 0x1000)}
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00180030 + BOBCAT2_PORT_56_START_OFFSET_CNS, 0x00180030 + BOBCAT2_PORT_56_START_OFFSET_CNS)}, FORMULA_SINGLE_PARAMETER(16 , 0x1000)}
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0018003C + BOBCAT2_PORT_56_START_OFFSET_CNS, 0x001800C8 + BOBCAT2_PORT_56_START_OFFSET_CNS)}, FORMULA_SINGLE_PARAMETER(16 , 0x1000)}
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x001800D0 + BOBCAT2_PORT_56_START_OFFSET_CNS, 0x00180120 + BOBCAT2_PORT_56_START_OFFSET_CNS)}, FORMULA_SINGLE_PARAMETER(16 , 0x1000)}
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00180128 + BOBCAT2_PORT_56_START_OFFSET_CNS, 0x0018014C + BOBCAT2_PORT_56_START_OFFSET_CNS)}, FORMULA_SINGLE_PARAMETER(16 , 0x1000)}
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0018015C + BOBCAT2_PORT_56_START_OFFSET_CNS, 0x0018017C + BOBCAT2_PORT_56_START_OFFSET_CNS)}, FORMULA_SINGLE_PARAMETER(16 , 0x1000)}
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00180200 + BOBCAT2_PORT_56_START_OFFSET_CNS, 256)}                                         , FORMULA_SINGLE_PARAMETER(16 , 0x1000)}

            /* XPCS IP */
                /* ports 48..55 in steps of 2*/
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00180400 + BOBCAT2_PORT_48_START_OFFSET_CNS, 0x00180424 + BOBCAT2_PORT_48_START_OFFSET_CNS)}, FORMULA_SINGLE_PARAMETER(8/2  , 0x1000*2)}
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0018042C + BOBCAT2_PORT_48_START_OFFSET_CNS, 0x0018044C + BOBCAT2_PORT_48_START_OFFSET_CNS)}, FORMULA_SINGLE_PARAMETER(8/2  , 0x1000*2)}
                /* 6 lanes */
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00180450 + BOBCAT2_PORT_48_START_OFFSET_CNS, 0x0044)}, FORMULA_TWO_PARAMETERS(6 , 0x44  , 8/2  , 0x1000*2)                          }

                /* ports 56..71 in steps of 2*/
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00180400 + BOBCAT2_PORT_56_START_OFFSET_CNS, 0x00180424 + BOBCAT2_PORT_56_START_OFFSET_CNS)}, FORMULA_SINGLE_PARAMETER(16/2  , 0x1000*2)}
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0018042C + BOBCAT2_PORT_56_START_OFFSET_CNS, 0x0018044C + BOBCAT2_PORT_56_START_OFFSET_CNS)}, FORMULA_SINGLE_PARAMETER(16/2  , 0x1000*2)}
                /* 6 lanes */
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00180450 + BOBCAT2_PORT_56_START_OFFSET_CNS, 0x0044)}, FORMULA_TWO_PARAMETERS(6 , 0x44  , 16/2  , 0x1000*2)                          }

            /* XLG */
                /* ports 48..55 */
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000c0000 + BOBCAT2_PORT_48_START_OFFSET_CNS, 0x000c0024 + BOBCAT2_PORT_48_START_OFFSET_CNS)}, FORMULA_SINGLE_PARAMETER(8  , 0x1000)}
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000c002C + BOBCAT2_PORT_48_START_OFFSET_CNS, 0x000c0060 + BOBCAT2_PORT_48_START_OFFSET_CNS)}, FORMULA_SINGLE_PARAMETER(8  , 0x1000)}
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000c0090 + BOBCAT2_PORT_48_START_OFFSET_CNS, 0x000c0094 + BOBCAT2_PORT_48_START_OFFSET_CNS)}, FORMULA_SINGLE_PARAMETER(8  , 0x1000)}

                /* ports 56..71 */
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000c0000 + BOBCAT2_PORT_56_START_OFFSET_CNS, 0x000c0024 + BOBCAT2_PORT_56_START_OFFSET_CNS)}, FORMULA_SINGLE_PARAMETER(16 , 0x1000)}
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000c002C + BOBCAT2_PORT_56_START_OFFSET_CNS, 0x000c0060 + BOBCAT2_PORT_56_START_OFFSET_CNS)}, FORMULA_SINGLE_PARAMETER(16 , 0x1000)}
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000c0090 + BOBCAT2_PORT_56_START_OFFSET_CNS, 0x000c0094 + BOBCAT2_PORT_56_START_OFFSET_CNS)}, FORMULA_SINGLE_PARAMETER(16 , 0x1000)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, unitPtr);
    }

    {
        GT_U32  numOfChunks;
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;
        SMEM_CHUNK_EXTENDED_STC * chunksMemBc2Ptr;

        SMEM_CHUNK_EXTENDED_STC  chunksMemBc2A0[]=
        {
            {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x000000C0, 0x000000D0)} , FORMULA_SINGLE_PARAMETER(56 , 0x1000)}
           ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x000000C0 + BOBCAT2_PORT_56_START_OFFSET_CNS, 0x000000D0 + BOBCAT2_PORT_56_START_OFFSET_CNS)} , FORMULA_SINGLE_PARAMETER(16 , 0x1000)}
            /*FCA*/
                /* ports 0..55 */
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00180600, 0x00180714)} , FORMULA_SINGLE_PARAMETER(56 , 0x1000)}
                /* ports 56..71 */
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00180600 + BOBCAT2_PORT_56_START_OFFSET_CNS, 0x00180714 + BOBCAT2_PORT_56_START_OFFSET_CNS)} , FORMULA_SINGLE_PARAMETER(16 , 0x1000)}
           /* XLG */
           /* ports 48..55 */
           ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000c0068 + BOBCAT2_PORT_48_START_OFFSET_CNS, 0x000c007C + BOBCAT2_PORT_48_START_OFFSET_CNS)}, FORMULA_SINGLE_PARAMETER(8  , 0x1000)}
           ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000c0084 + BOBCAT2_PORT_48_START_OFFSET_CNS, 0x000c0088 + BOBCAT2_PORT_48_START_OFFSET_CNS)}, FORMULA_SINGLE_PARAMETER(8  , 0x1000)}
           /* ports 56..71 */
           ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000c0068 + BOBCAT2_PORT_56_START_OFFSET_CNS, 0x000c007C + BOBCAT2_PORT_56_START_OFFSET_CNS)}, FORMULA_SINGLE_PARAMETER(16 , 0x1000)}
           ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000c0084 + BOBCAT2_PORT_56_START_OFFSET_CNS, 0x000c0088 + BOBCAT2_PORT_56_START_OFFSET_CNS)}, FORMULA_SINGLE_PARAMETER(16 , 0x1000)}

        };

        SMEM_CHUNK_EXTENDED_STC  chunksMemBc2B0[]=
        {
            {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x000000C0, 0x000000D8)} , FORMULA_SINGLE_PARAMETER(56 , 0x1000)}
           ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x000000C0 + BOBCAT2_PORT_56_START_OFFSET_CNS, 0x000000D8 + BOBCAT2_PORT_56_START_OFFSET_CNS)} , FORMULA_SINGLE_PARAMETER(16 , 0x1000)}
           /*FCA*/
           /* ports 0..55 */
           ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00180600, 0x00180718)} , FORMULA_SINGLE_PARAMETER(56 , 0x1000)}
           /* ports 56..71 */
           ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00180600 + BOBCAT2_PORT_56_START_OFFSET_CNS, 0x00180718 + BOBCAT2_PORT_56_START_OFFSET_CNS)} , FORMULA_SINGLE_PARAMETER(16 , 0x1000)}
           /* XLG */
           /* ports 48..55 */
           ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000c0068 + BOBCAT2_PORT_48_START_OFFSET_CNS, 0x000c0088 + BOBCAT2_PORT_48_START_OFFSET_CNS)}, FORMULA_SINGLE_PARAMETER(8  , 0x1000)}
           /* ports 56..71 */
           ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000c0068 + BOBCAT2_PORT_56_START_OFFSET_CNS, 0x000c0088 + BOBCAT2_PORT_56_START_OFFSET_CNS)}, FORMULA_SINGLE_PARAMETER(16 , 0x1000)}
        };


        if(SMEM_CHT_IS_SIP5_10_GET(devObjPtr) == 0)
        {
            chunksMemBc2Ptr = chunksMemBc2A0;
            numOfChunks = sizeof(chunksMemBc2A0)/sizeof(SMEM_CHUNK_EXTENDED_STC);
        }
        else
        {
            chunksMemBc2Ptr = chunksMemBc2B0;
            numOfChunks = sizeof(chunksMemBc2B0)/sizeof(SMEM_CHUNK_EXTENDED_STC);
        }

        smemInitMemChunkExt(devObjPtr, chunksMemBc2Ptr, numOfChunks, &tmpUnitChunk);

        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr, unitPtr, &tmpUnitChunk);
    }

    /* chunks with formulas */
    if(IS_PORT_48_EXISTS(devObjPtr))
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
            /* TAI */
            {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x10180A00 ,0)},
                FORMULA_THREE_PARAMETERS(63, 0x4, 2, 0x00001000 , 3 , 0x00010000  )},
            {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC((0x10180A00 + 0x00030000)  ,0)},
                FORMULA_THREE_PARAMETERS(63, 0x4, 2, 0x00001000 , 2 , 0x00004000  )},
            {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC((0x10180A00 + 0x00200000)   ,0)},
                FORMULA_THREE_PARAMETERS(63, 0x4, 2, 0x00001000 , 4 , 0x00004000  )},
            /* PTP */
            {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x10180800 ,0)},
                FORMULA_TWO_PARAMETERS(30, 0x4, 56, 0x00001000)},
            {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC((0x10180800 + 0x00200000)   ,0)},
                FORMULA_TWO_PARAMETERS(30, 0x4, 16, 0x00001000)},
            /* Mac-TG Generator */
            {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x10180C00 ,0)},
                FORMULA_TWO_PARAMETERS(52, 0x4, 14, 0x00004000)},
            {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC((0x10180C00 + 0x00200000)   ,0)},
                FORMULA_TWO_PARAMETERS(52, 0x4, 4, 0x00004000)}
        };

        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

        for(ii = 0 ; ii < numOfChunks ; ii++)
        {
            chunksMem[ii].memChunkBasic.memFirstAddr -= baseAddr;
        }

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);
    }
}

/*******************************************************************************
*   smemLion3UnitTxqQueue
*
* DESCRIPTION:
*       Allocate address type specific memories -- TXQ queue
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
static void smemLion3UnitTxqQueue
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00090000, 0x00090014)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00090040, 0x00090054)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00090060, 0x00090074)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00090080, 0x0009019C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00090280, 0x0009039C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00090400, 0x00090430)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00090480, 0x0009059C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00090680, 0x0009079C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00090880, 0x000908AC)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000908C0, 0x00090904)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000909A0, 0x000909E4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00090A00, 0x00090A0C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00090A40, 0x00090A40)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00090A80, 0x00090A80)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00093000, 0x00093010)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00093020, 0x00093020)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00093200, 0x00093214)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00093220, 0x00093224)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00093230, 0x00093234)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00093240, 0x00093244)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00093250, 0x00093254)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00093260, 0x00093264)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00093270, 0x00093274)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00093280, 0x00093284)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00093290, 0x00093294)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000936A0, 0x000936A0)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000A0000, 0x000A0044)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000A0050, 0x000A016C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000A0200, 0x000A031C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000A0360, 0x000A03BC)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000A03D0, 0x000A040C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000A0440, 0x000A044C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000A0500, 0x000A063C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000A0800, 0x000A0800)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000A0810, 0x000A0810)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000A0820, 0x000A085C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000A0900, 0x000A093C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000A0A00, 0x000A0A3C)}
            /*Maximum Queue Limits*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x000A1000, 1024), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(34,8),SMEM_BIND_TABLE_MAC(Shared_Queue_Maximum_Queue_Limits)}
            /*Queue Limits DP0 - Enqueue*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x000A1800, 1024), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(48,8),SMEM_BIND_TABLE_MAC(Queue_Limits_DP0_Enqueue)}
            /*Queue Buffer Limits - Dequeue*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x000A2000, 1024), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(34,8),SMEM_BIND_TABLE_MAC(Queue_Buffer_Limits_Dequeue)}
            /*Queue Descriptor Limits - Dequeue*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x000A2800, 1024), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(14,8),SMEM_BIND_TABLE_MAC(Queue_Descriptor_Limits_Dequeue)}
            /*Queue Limits DP12 - Enqueue*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x000A3000, 2048), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(48,8),SMEM_BIND_TABLE_MAC(Queue_Limits_DP12_Enqueue)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000A4000, 0x000A4004)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000A4010, 0x000A412C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000A4130, 0x000A426C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000A4290, 0x000A42AC)}
            /*Q Main Buff*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x000A7000, 2304), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(14,4)}
            /*Q Main MC Buff*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x000A8000, 2304), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(20,4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000A9000, 0x000A9004)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000A9010, 0x000A9010)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000A9020, 0x000A9020)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000A9030, 0x000A9030)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000A9200, 0x000A9210)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000A9500, 0x000A9504)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000A9510, 0x000A951C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000AA000, 0x000AA040)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000AA050, 0x000AA12C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000AA1B0, 0x000AA1CC)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000AA210, 0x000AA210)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000AA230, 0x000AA230)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
    {
        SMEM_CHUNK_BASIC_STC  chunksMemBc2B0[]=
        {
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00090A90, 0x00090A94)}
           ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x3C0909F0, 0x3C0909F0)}
        };


        if(SMEM_CHT_IS_SIP5_10_GET(devObjPtr))
        {
            smemUnitChunkAddBasicChunk(devObjPtr,unitPtr,
                ARRAY_NAME_AND_NUM_ELEMENTS_MAC(chunksMemBc2B0));
        }
    }

}

/*******************************************************************************
*   smemLion3UnitTxqLl
*
* DESCRIPTION:
*       Allocate address type specific memories -- TXQ ll (link list)
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
static void smemLion3UnitTxqLl
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            /*FBUF*/
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00080000, 32768), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(56,8)}
            /*QTAIL*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00088000, 2304), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(14,4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000A0004, 0x000A0020)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000A0300, 0x000A033C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000A0508, 0x000A050C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000A0528, 0x000A0528)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000A0600, 0x000A0608)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000A1000, 0x000A1000)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
    {
        SMEM_CHUNK_BASIC_STC  chunksMemBc2A0[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000A0040, 0x000A015C)}
            /*QHEAD*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00089000, 2304), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(14,4)}
            /*FQHEAD0*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00089900, 256), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(14,4)}
            /*FQHEAD1*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00089B00, 256), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(14,4)}
            /*FQHEAD2*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00089D00, 256), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(14,4)}
            /*FQTAIL0*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00088900, 256), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(14,4)}
            /*FQTAIL1*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00088B00, 256), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(14,4)}
            /*FQTAIL2*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00088D00, 256), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(14,4)}
            /*Q Main Desc*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0008C000, 2304), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(14,4)}
        };

        SMEM_CHUNK_BASIC_STC  chunksMemBc2B0[]=
        {
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000A0044, 0x000A0160)}
            /*QHEAD*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0008A000, 2304), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(14,4)}
            /*FQHEAD0*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0008AA00, 256), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(14,4)}
            /*FQHEAD1*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0008AC00, 256), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(14,4)}
            /*FQHEAD2*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0008AE00, 256), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(14,4)}
            /*FQTAIL0*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00088A00, 256), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(14,4)}
            /*FQTAIL1*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00088C00, 256), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(14,4)}
            /*FQTAIL2*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00088E00, 256), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(14,4)}
            /*Q Main Desc*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0008D000, 2304), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(14,4)}
            /* Link List ECC Control and Status Bank */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000A1100, 0x000A113C)}
            /* Link List ECC Error Counter Bank */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000A1140, 0x000A117C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000A0510, 0x000A0524)}
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
*   smemLion3UnitTxqPfc
*
* DESCRIPTION:
*       Allocate address type specific memories -- TXQ pfc
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
static void smemLion3UnitTxqPfc
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000000, 0x00000000)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000010, 0x00000030)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000050, 0x00000050)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000090, 0x00000090)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000000B0, 0x000000B0)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000000D0, 0x000000D0)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000000F0, 0x0000016C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000170, 0x0000026C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000900, 0x00000D1C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001100, 0x00001100)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001110, 0x00001110)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001120, 0x00001120)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001140, 0x00001140)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001170, 0x00001170)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001300, 0x00001304)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001310, 0x00001314)}
            /*FC Mode Profile TC XOff Thresholds*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00001800, 256), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(29,4),SMEM_BIND_TABLE_MAC(FC_Mode_Profile_TC_XOff_Thresholds)}
            /*FC Mode Profile TC XOn Thresholds*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00001A00, 256), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(29,4)}
            /*Port Group 0 PFC LLFC Counters*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00002000, 4096), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(30,4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00014000, 0x0001401C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00014200, 0x0001421C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00014400, 0x0001441C)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
    {
        SMEM_CHUNK_BASIC_STC  chunksMemBc2A0[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000070, 0x00000070)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001320, 0x00001324)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001330, 0x000013C4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000013D0, 0x000013DC)}
        };

        SMEM_CHUNK_BASIC_STC  chunksMemBc2B0[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000060, 0x00000060)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000000A0, 0x000000A0)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001320, 0x0000132C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001370, 0x00001370)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001390, 0x00001390)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000013B0, 0x000013BC)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00005400, 0x00005408)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00005500, 0x00005500)}
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
*   smemLion3UnitTxqQcn
*
* DESCRIPTION:
*       Allocate address type specific memories -- TXQ Qcn
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
static void smemLion3UnitTxqQcn
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000000, 0x00000008)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000040, 0x00000040)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000050, 0x00000050)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000060, 0x00000060)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000090, 0x000000AC)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000000D0, 0x000000D0)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000000E0, 0x000000E0)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000000F0, 0x000000F0)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000100, 0x00000100)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000110, 0x00000110)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000120, 0x00000124)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000130, 0x00000130)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000140, 0x00000140)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000200, 0x00000200)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000210, 0x00000210)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000300, 0x000003FC)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000B00, 0x00000B60)}
            /*CN Sample Intervals Table*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00002000, 4608), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(36,8),SMEM_BIND_TABLE_MAC(CN_Sample_Intervals)}

        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
    {
        SMEM_CHUNK_BASIC_STC  chunksMemBc2B0[]=
        {
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000128, 0x00000128)}
           ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000400, 0x000004FC)}
        };


        if(SMEM_CHT_IS_SIP5_10_GET(devObjPtr))
        {
            smemUnitChunkAddBasicChunk(devObjPtr,unitPtr,
                ARRAY_NAME_AND_NUM_ELEMENTS_MAC(chunksMemBc2B0));
        }
    }

}

/*******************************************************************************
*   smemLion3UnitTxqDq
*
* DESCRIPTION:
*       Allocate address type specific memories -- TXQ DQ
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
static void smemLion3UnitTxqDq
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000000, 0x00000120)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000208, 0x00000248)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000260, 0x0000027C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000002C0, 0x000003DC)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000400, 0x0000051C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000600, 0x00000604)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000610, 0x00000650)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000660, 0x00000660)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000670, 0x00000670)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000680, 0x00000680)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000800, 0x00000800)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000808, 0x00000808)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000900, 0x00000904)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000920, 0x00000924)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000A00, 0x00000A00)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001000, 0x00001000)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001040, 0x0000115C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001240, 0x0000133C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001380, 0x0000149C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001500, 0x00001500)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001510, 0x0000162C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001650, 0x000017A0)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000017B0, 0x000017B0)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001800, 0x00001800)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001904, 0x00001904)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000190C, 0x00001A28)}
            /*Scheduler State Variable RAM*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00002000, 2304), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(168,32),SMEM_BIND_TABLE_MAC(Scheduler_State_Variable)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00004000, 0x0000400C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00004100, 0x0000421C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00004300, 0x0000441C)}
            /*Priority Token Bucket Configuration*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00006000, 4608), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(488,64)}
            /*Port Token Bucket Configuration*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0000A000, 576), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(61,8)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000C000, 0x0000C000)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000C008, 0x0000C044)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000C080, 0x0000C09C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000C100, 0x0000C13C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000C180, 0x0000C37C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000C500, 0x0000C8FC)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000D000, 0x0000D004)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000D010, 0x0000D014)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000D040, 0x0000D0BC)}
            /*Egress STC Table*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0000D800, 1152), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(80,16),SMEM_BIND_TABLE_MAC(egressStc)}

        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
    {
        SMEM_CHUNK_BASIC_STC  chunksMemBc2A0[]=
        {
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001008, 0x0000100C)}
        };

        SMEM_CHUNK_BASIC_STC  chunksMemBc2B0[]=
        {
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000b00, 0x00000c24)}
           ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000690, 0x00000690)}
           ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001008, 0x00001010)}
           ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001908, 0x00001908)}
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

#define BC2_XSB_DDR_CONTROLER_DDR_MEMORY_MAC(instanceOffset)   \
    {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (XSB_DDR_ADDR_MAC(instanceOffset ,0x00001030), XSB_DDR_ADDR_MAC(instanceOffset ,0x0000103C))},  \
    {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (XSB_DDR_ADDR_MAC(instanceOffset ,0x000010B0), XSB_DDR_ADDR_MAC(instanceOffset ,0x000010C4))},  \
    {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (XSB_DDR_ADDR_MAC(instanceOffset ,0x000010F0), XSB_DDR_ADDR_MAC(instanceOffset ,0x000010FC))},  \
    {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (XSB_DDR_ADDR_MAC(instanceOffset ,0x00001130), XSB_DDR_ADDR_MAC(instanceOffset ,0x0000113C))},  \
    {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (XSB_DDR_ADDR_MAC(instanceOffset ,0x000011B0), XSB_DDR_ADDR_MAC(instanceOffset ,0x000011CC))},  \
    {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (XSB_DDR_ADDR_MAC(instanceOffset ,0x000011F0), XSB_DDR_ADDR_MAC(instanceOffset ,0x000011FC))},  \
    {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (XSB_DDR_ADDR_MAC(instanceOffset ,0x00001230), XSB_DDR_ADDR_MAC(instanceOffset ,0x0000123C))},  \
    {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (XSB_DDR_ADDR_MAC(instanceOffset ,0x000012B0), XSB_DDR_ADDR_MAC(instanceOffset ,0x000012CC))},  \
    {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (XSB_DDR_ADDR_MAC(instanceOffset ,0x000012F0), XSB_DDR_ADDR_MAC(instanceOffset ,0x000012FC))},  \
    {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (XSB_DDR_ADDR_MAC(instanceOffset ,0x00001400), XSB_DDR_ADDR_MAC(instanceOffset ,0x00001438))},  \
    {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (XSB_DDR_ADDR_MAC(instanceOffset ,0x00001454), XSB_DDR_ADDR_MAC(instanceOffset ,0x00001454))},  \
    {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (XSB_DDR_ADDR_MAC(instanceOffset ,0x00001470), XSB_DDR_ADDR_MAC(instanceOffset ,0x00001474))},  \
    {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (XSB_DDR_ADDR_MAC(instanceOffset ,0x0000147C), XSB_DDR_ADDR_MAC(instanceOffset ,0x00001480))},  \
    {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (XSB_DDR_ADDR_MAC(instanceOffset ,0x00001488), XSB_DDR_ADDR_MAC(instanceOffset ,0x000014A4))},  \
    {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (XSB_DDR_ADDR_MAC(instanceOffset ,0x000014B0), XSB_DDR_ADDR_MAC(instanceOffset ,0x000014B8))},  \
    {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (XSB_DDR_ADDR_MAC(instanceOffset ,0x000014C0), XSB_DDR_ADDR_MAC(instanceOffset ,0x000014DC))},  \
    {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (XSB_DDR_ADDR_MAC(instanceOffset ,0x000014F4), XSB_DDR_ADDR_MAC(instanceOffset ,0x000014F8))},  \
    {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (XSB_DDR_ADDR_MAC(instanceOffset ,0x00001504), XSB_DDR_ADDR_MAC(instanceOffset ,0x00001504))},  \
    {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (XSB_DDR_ADDR_MAC(instanceOffset ,0x0000150C), XSB_DDR_ADDR_MAC(instanceOffset ,0x0000150C))},  \
    {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (XSB_DDR_ADDR_MAC(instanceOffset ,0x00001514), XSB_DDR_ADDR_MAC(instanceOffset ,0x00001514))},  \
    {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (XSB_DDR_ADDR_MAC(instanceOffset ,0x0000151C), XSB_DDR_ADDR_MAC(instanceOffset ,0x00001528))},  \
    {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (XSB_DDR_ADDR_MAC(instanceOffset ,0x00001538), XSB_DDR_ADDR_MAC(instanceOffset ,0x0000153C))},  \
    {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (XSB_DDR_ADDR_MAC(instanceOffset ,0x00001590), XSB_DDR_ADDR_MAC(instanceOffset ,0x000015C8))},  \
    {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (XSB_DDR_ADDR_MAC(instanceOffset ,0x000015D0), XSB_DDR_ADDR_MAC(instanceOffset ,0x000015E4))},  \
    {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (XSB_DDR_ADDR_MAC(instanceOffset ,0x000015EC), XSB_DDR_ADDR_MAC(instanceOffset ,0x000015EC))},  \
    {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (XSB_DDR_ADDR_MAC(instanceOffset ,0x00001600), XSB_DDR_ADDR_MAC(instanceOffset ,0x00001618))},  \
    {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (XSB_DDR_ADDR_MAC(instanceOffset ,0x00001624), XSB_DDR_ADDR_MAC(instanceOffset ,0x00001624))},  \
    {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (XSB_DDR_ADDR_MAC(instanceOffset ,0x00001630), XSB_DDR_ADDR_MAC(instanceOffset ,0x0000163C))},  \
    {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (XSB_DDR_ADDR_MAC(instanceOffset ,0x00001670), XSB_DDR_ADDR_MAC(instanceOffset ,0x00001674))},  \
    {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (XSB_DDR_ADDR_MAC(instanceOffset ,0x00001690), XSB_DDR_ADDR_MAC(instanceOffset ,0x000016A0))},  \
    {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (XSB_DDR_ADDR_MAC(instanceOffset ,0x000016AC), XSB_DDR_ADDR_MAC(instanceOffset ,0x000016DC))},  \
    {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (XSB_DDR_ADDR_MAC(instanceOffset ,0x000016F0), XSB_DDR_ADDR_MAC(instanceOffset ,0x000016FC))},  \
    {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (XSB_DDR_ADDR_MAC(instanceOffset ,0x00001830), XSB_DDR_ADDR_MAC(instanceOffset ,0x0000183C))},  \
    {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (XSB_DDR_ADDR_MAC(instanceOffset ,0x00001870), XSB_DDR_ADDR_MAC(instanceOffset ,0x000018CC))},  \
    {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (XSB_DDR_ADDR_MAC(instanceOffset ,0x000018F0), XSB_DDR_ADDR_MAC(instanceOffset ,0x000018FC))},  \
    {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (XSB_DDR_ADDR_MAC(instanceOffset ,0x00001930), XSB_DDR_ADDR_MAC(instanceOffset ,0x0000193C))},  \
    {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (XSB_DDR_ADDR_MAC(instanceOffset ,0x000019B0), XSB_DDR_ADDR_MAC(instanceOffset ,0x000019CC))},  \
    {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (XSB_DDR_ADDR_MAC(instanceOffset ,0x000019F0), XSB_DDR_ADDR_MAC(instanceOffset ,0x000019FC))},  \
    {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (XSB_DDR_ADDR_MAC(instanceOffset ,0x00001A30), XSB_DDR_ADDR_MAC(instanceOffset ,0x00001A3C))},  \
    {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (XSB_DDR_ADDR_MAC(instanceOffset ,0x00001AB0), XSB_DDR_ADDR_MAC(instanceOffset ,0x00001ACC))},  \
    {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (XSB_DDR_ADDR_MAC(instanceOffset ,0x00001AF0), XSB_DDR_ADDR_MAC(instanceOffset ,0x00001AFC))},  \
    {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (XSB_DDR_ADDR_MAC(instanceOffset ,0x00001B30), XSB_DDR_ADDR_MAC(instanceOffset ,0x00001B3C))},  \
    {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (XSB_DDR_ADDR_MAC(instanceOffset ,0x00001BB0), XSB_DDR_ADDR_MAC(instanceOffset ,0x00001BCC))},  \
    {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (XSB_DDR_ADDR_MAC(instanceOffset ,0x00001BF0), XSB_DDR_ADDR_MAC(instanceOffset ,0x00001BFC))},  \
    {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (XSB_DDR_ADDR_MAC(instanceOffset ,0x00001C30), XSB_DDR_ADDR_MAC(instanceOffset ,0x00001C3C))},  \
    {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (XSB_DDR_ADDR_MAC(instanceOffset ,0x00001CB0), XSB_DDR_ADDR_MAC(instanceOffset ,0x00001CCC))},  \
    {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (XSB_DDR_ADDR_MAC(instanceOffset ,0x00001CF0), XSB_DDR_ADDR_MAC(instanceOffset ,0x00001D00))}

#define XSB_BASE_ADDR               0x4000
/* see code in mvHwsDdr3Bc2.c in interfaceMap_A0 */
#define BC2_A0_XSB_XBAR_0_OFFSET         (XSB_BASE_ADDR + ((17) << 15))
#define BC2_A0_XSB_XBAR_1_OFFSET         (XSB_BASE_ADDR + ((7 ) << 15))
#define BC2_A0_XSB_XBAR_2_OFFSET         (XSB_BASE_ADDR + ((11) << 15))
#define BC2_A0_XSB_XBAR_3_OFFSET         (XSB_BASE_ADDR + ((3 ) << 15))
#define BC2_A0_XSB_XBAR_4_OFFSET         (XSB_BASE_ADDR + ((25) << 15))
#define BC2_A0_XSB_XBAR_MULTICAST_OFFSET (XSB_BASE_ADDR + ((29) << 15))
/* see code in mvHwsDdr3Bc2.c in interfaceMap_B0 */
#define BC2_B0_XSB_XBAR_0_OFFSET         (XSB_BASE_ADDR + ((17) << 15))
#define BC2_B0_XSB_XBAR_1_OFFSET         (XSB_BASE_ADDR + ((6 ) << 15))
#define BC2_B0_XSB_XBAR_2_OFFSET         (XSB_BASE_ADDR + ((10) << 15))
#define BC2_B0_XSB_XBAR_3_OFFSET         (XSB_BASE_ADDR + ((3 ) << 15))
#define BC2_B0_XSB_XBAR_4_OFFSET         (XSB_BASE_ADDR + ((18) << 15))
#define BC2_B0_XSB_XBAR_MULTICAST_OFFSET (XSB_BASE_ADDR + ((29) << 15))

/* XSB XBAR instance .
NOTE: Cider show single instance but the HWS uses 6 instances : 5 for 'unicast' and one for multicast */
#define BC2_XSB_XBAR_MEMORY_MAC(offset)   \
     {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (offset+0x00000000, offset+0x0000008c)}  \
    ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (offset+0x000001FC, offset+0x000001FC)}

/*******************************************************************************
*   smemLion3UnitDfx
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  DFX unit
*
* INPUTS:
*       devObjPtr           - pointer to device memory object.
*       unitPtr             - pointer to the unit chunk
*       unitBaseAddr        - unit base address
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
void smemLion3UnitDfx
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr,
    IN GT_U32 unitBaseAddr
)
{
    {
        GT_U32 i;

        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {

            /* DDR 0 */
            BC2_XSB_DDR_CONTROLER_DDR_MEMORY_MAC(0x00000),
            /* DDR 1 */
            BC2_XSB_DDR_CONTROLER_DDR_MEMORY_MAC(0x28000),
            /* DDR 2 */
            BC2_XSB_DDR_CONTROLER_DDR_MEMORY_MAC(0x48000),
            /* DDR 3 */
            BC2_XSB_DDR_CONTROLER_DDR_MEMORY_MAC(0x70000),
            /* DDR 4 */
            BC2_XSB_DDR_CONTROLER_DDR_MEMORY_MAC(0xB8000),

            /* DFX Server Units - BC2 specific registers */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000F8004, 0x000F8004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000F800C, 0x000F800C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000F8014, 0x000F8014)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000F8020, 0x000F8020)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000F8030, 0x000F8030)},
            /* manually fixed to add 0x000F8050-0x000F805C which exists on HW (although not in Cider!!!!) */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000F8040, 0x000F805C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000F8060, 0x000F806C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000F8200, 0x000F8204)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000F820C, 0x000F821C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000F8230, 0x000F8230)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000F8250, 0x000F82A4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000F82B0, 0x000F82B8)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000F8340, 0x000F83BC)},
            /* DFX Server Units */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000F8000, 0x000F8000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000F8008, 0x000F8008)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000F8010, 0x000F8010)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000F8018, 0x000F8018)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000F8024, 0x000F802C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000F8034, 0x000F8038)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000F8070, 0x000F8080)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000F80A0, 0x000F80A0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x000F80B0, 4),
                SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4), SMEM_BIND_TABLE_MAC(efuseFeaturesDisableBypass)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x000F80C0, 4)         ,
                SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4), SMEM_BIND_TABLE_MAC(efuseDevIdBypass)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000F8100, 0x000F8114)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000F8120, 0x000F813C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000F8208, 0x000F8208)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000F8220, 0x000F822C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000F8240, 0x000F8248)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000F82D0, 0x000F82D0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000F82E0, 0x000F833C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000F83C0, 0x000F843C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000F8454, 0x000F8460)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x000F8500, 64)        ,
                SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(257, 64), SMEM_BIND_TABLE_MAC(idEfuseSlave)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x000F9000, 1024)      ,
                SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(65, 16), SMEM_BIND_TABLE_MAC(hdEfuseSlave)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x000FC000, 12288)     ,
                SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4), SMEM_BIND_TABLE_MAC(debugUnit)}

            /* in AC3 and BC2_B0 it replaces 0xF8520 that was in BC2_A0 */
            /* added manually .. not in Cider */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000F8F20, 0x000F8F20)}

        };

        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        for(i = 0; i < numOfChunks; i++)
        {
            chunksMem[i].memFirstAddr |= unitBaseAddr;
        }

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }

    {
        GT_U32 i;

        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
            /* DFX_RAMs */
             {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000000, 0x0000003C)}, FORMULA_TWO_PARAMETERS(128, 0x40, 28, 0x8000)}
            /* DFX_CLIENTS */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002000, 0x00002000)}, FORMULA_SINGLE_PARAMETER(31 , 0x8000)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002008, 0x00002008)}, FORMULA_SINGLE_PARAMETER(31 , 0x8000)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002040, 0x0000204C)}, FORMULA_SINGLE_PARAMETER(31 , 0x8000)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002070, 0x00002074)}, FORMULA_SINGLE_PARAMETER(31 , 0x8000)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002080, 0x00002080)}, FORMULA_SINGLE_PARAMETER(31 , 0x8000)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000020b0, 0x000020b0)}, FORMULA_SINGLE_PARAMETER(31 , 0x8000)}
        };

        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

        for(i = 0; i < numOfChunks; i++)
        {
            chunksMem[i].memChunkBasic.memFirstAddr |= unitBaseAddr;
        }

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);
    }

    if(SMEM_CHT_IS_SIP5_10_GET(devObjPtr) == 0)
    {
        GT_U32 i;

        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            /* XSB_XBAR_IP units */
            BC2_XSB_XBAR_MEMORY_MAC(BC2_A0_XSB_XBAR_0_OFFSET),
            BC2_XSB_XBAR_MEMORY_MAC(BC2_A0_XSB_XBAR_1_OFFSET),
            BC2_XSB_XBAR_MEMORY_MAC(BC2_A0_XSB_XBAR_2_OFFSET),
            BC2_XSB_XBAR_MEMORY_MAC(BC2_A0_XSB_XBAR_3_OFFSET),
            BC2_XSB_XBAR_MEMORY_MAC(BC2_A0_XSB_XBAR_4_OFFSET),
            BC2_XSB_XBAR_MEMORY_MAC(BC2_A0_XSB_XBAR_MULTICAST_OFFSET)
        };

        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

        for(i = 0; i < numOfChunks; i++)
        {
            chunksMem[i].memFirstAddr |= unitBaseAddr;
        }

        smemInitMemChunk(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);
    }
    else
    {
        GT_U32 i;

        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            /* XSB_XBAR_IP units */
            BC2_XSB_XBAR_MEMORY_MAC(BC2_B0_XSB_XBAR_0_OFFSET),
            BC2_XSB_XBAR_MEMORY_MAC(BC2_B0_XSB_XBAR_1_OFFSET),
            BC2_XSB_XBAR_MEMORY_MAC(BC2_B0_XSB_XBAR_2_OFFSET),
            BC2_XSB_XBAR_MEMORY_MAC(BC2_B0_XSB_XBAR_3_OFFSET),
            BC2_XSB_XBAR_MEMORY_MAC(BC2_B0_XSB_XBAR_4_OFFSET),
            BC2_XSB_XBAR_MEMORY_MAC(BC2_B0_XSB_XBAR_MULTICAST_OFFSET)
        };

        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

        for(i = 0; i < numOfChunks; i++)
        {
            chunksMem[i].memFirstAddr |= unitBaseAddr;
        }

        smemInitMemChunk(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);
    }

    if(SMEM_CHT_IS_SIP5_15_GET(devObjPtr))
    {
        GT_U32 i;

        SMEM_CHUNK_BASIC_STC  chunksMem[]= {
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000f8098, 0X000f8098)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

        for(i = 0; i < numOfChunks; i++)
        {
            chunksMem[i].memFirstAddr |= unitBaseAddr;
        }

        smemInitMemChunk(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);
    }
}

/*******************************************************************************
*   smemLion3UnitDfxServerDummy
*
* DESCRIPTION:
*       Allocate address type specific dummy memories -- for the  DFX unit
*
* INPUTS:
*       devObjPtr           - pointer to device memory object.
*       unitPtr             - pointer to the unit chunk
*       unitBaseAddr        - unit base address
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*       Need to accept access to DFX address in Lion2 without fatal error.
*
*******************************************************************************/
void smemLion3UnitDfxServerDummy
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x018f8500, 0x018f8504)}
        };

        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
}

/*******************************************************************************
*   smemLion3UnitTxDma
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Lion3  TX DMA unit
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
static void smemLion3UnitTxDma
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000000, 0x00000010)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000020, 0x0000011C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000500, 0x00000504)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001000, 0x00001014)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001020, 0x00001020)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002000, 0x00002044)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00004000, 0x00004158)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00004160, 0x00004160)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00005000, 0x00005014)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00005100, 0x00005100)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00005120, 0x00005120)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00005200, 0x00005324)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00005500, 0x00005624)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00005700, 0x00005824)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00005900, 0x00005A24)}

        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
        {
            SMEM_CHUNK_BASIC_STC  chunksMemBc2A0[]=
            {
                 {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000200, 0x0000021C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003000, 0x00003014)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003020, 0x00003034)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003040, 0x00003054)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003060, 0x00003074)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003080, 0x00003094)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000030A0, 0x000030B4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000030C0, 0x000030D4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000030E0, 0x000030F4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003100, 0x00003114)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003120, 0x00003134)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003140, 0x00003154)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003160, 0x00003174)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003180, 0x00003194)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000031A0, 0x000031B4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000031C0, 0x000031D4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000031E0, 0x000031F4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003200, 0x00003214)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003220, 0x00003234)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003240, 0x00003254)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003260, 0x00003274)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003280, 0x00003294)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000032A0, 0x000032B4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000032C0, 0x000032D4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000032E0, 0x000032F4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003300, 0x00003314)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003320, 0x00003334)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003340, 0x00003354)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003360, 0x00003374)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003380, 0x00003394)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000033A0, 0x000033B4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000033C0, 0x000033D4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000033E0, 0x000033F4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003400, 0x00003414)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003420, 0x00003434)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003440, 0x00003454)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003460, 0x00003474)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003480, 0x00003494)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000034A0, 0x000034B4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000034C0, 0x000034D4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000034E0, 0x000034F4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003500, 0x00003514)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003520, 0x00003534)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003540, 0x00003554)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003560, 0x00003574)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003580, 0x00003594)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000035A0, 0x000035B4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000035C0, 0x000035D4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000035E0, 0x000035F4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003600, 0x00003614)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003620, 0x00003634)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003640, 0x00003654)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003660, 0x00003674)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003680, 0x00003694)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000036A0, 0x000036B4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000036C0, 0x000036D4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000036E0, 0x000036F4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003700, 0x00003714)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003720, 0x00003734)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003740, 0x00003754)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003760, 0x00003774)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003780, 0x00003794)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000037A0, 0x000037B4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000037C0, 0x000037D4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000037E0, 0x000037F4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003800, 0x00003814)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003820, 0x00003834)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003840, 0x00003854)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003860, 0x00003874)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003880, 0x00003894)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000038A0, 0x000038B4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000038C0, 0x000038D4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000038E0, 0x000038F4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003900, 0x00003914)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003920, 0x00003934)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000501C, 0x00005060)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00005070, 0x00005074)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00006000, 0x00006004)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00006100, 0x00006224)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00006300, 0x00006300)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00006500, 0x00006500)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00006700, 0x00006700)}
            };

            SMEM_CHUNK_BASIC_STC  chunksMemBc2B0[]=
            {
                 {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000200, 0x00000224)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003000, 0x00003018)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003020, 0x00003038)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003040, 0x00003058)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003060, 0x00003078)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003080, 0x00003098)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000030A0, 0x000030B8)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000030C0, 0x000030D8)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000030E0, 0x000030F8)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003100, 0x00003118)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003120, 0x00003138)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003140, 0x00003158)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003160, 0x00003178)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003180, 0x00003198)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000031A0, 0x000031B8)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000031C0, 0x000031D8)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000031E0, 0x000031F8)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003200, 0x00003218)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003220, 0x00003238)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003240, 0x00003258)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003260, 0x00003278)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003280, 0x00003298)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000032A0, 0x000032B8)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000032C0, 0x000032D8)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000032E0, 0x000032F8)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003300, 0x00003318)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003320, 0x00003338)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003340, 0x00003358)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003360, 0x00003378)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003380, 0x00003398)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000033A0, 0x000033B8)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000033C0, 0x000033D8)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000033E0, 0x000033F8)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003400, 0x00003418)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003420, 0x00003438)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003440, 0x00003458)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003460, 0x00003478)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003480, 0x00003498)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000034A0, 0x000034B8)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000034C0, 0x000034D8)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000034E0, 0x000034F8)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003500, 0x00003518)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003520, 0x00003538)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003540, 0x00003558)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003560, 0x00003578)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003580, 0x00003598)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000035A0, 0x000035B8)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000035C0, 0x000035D8)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000035E0, 0x000035F8)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003600, 0x00003618)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003620, 0x00003638)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003640, 0x00003658)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003660, 0x00003678)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003680, 0x00003698)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000036A0, 0x000036B8)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000036C0, 0x000036D8)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000036E0, 0x000036F8)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003700, 0x00003718)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003720, 0x00003738)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003740, 0x00003758)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003760, 0x00003778)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003780, 0x00003798)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000037A0, 0x000037B8)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000037C0, 0x000037D8)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000037E0, 0x000037F8)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003800, 0x00003818)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003820, 0x00003838)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003840, 0x00003858)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003860, 0x00003878)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003880, 0x00003898)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000038A0, 0x000038B8)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000038C0, 0x000038D8)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000038E0, 0x000038F8)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003900, 0x00003918)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003920, 0x00003938)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000501C, 0x00005078)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00005080, 0x0000508C)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00006100, 0x00006104)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00006300, 0x00006304)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00007000, 0x00007004)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00007100, 0x00007224)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00007300, 0x00007300)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00007500, 0x00007500)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00007700, 0x00007700)}
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

    if(devObjPtr->portGroupSharedDevObjPtr)
    {
        /*patch for lion3 only*/
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            /* add addresses of legacy interrupts */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0130, 0x0140)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

        smemInitMemChunk(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);
    }

}

/*******************************************************************************
*   smemLion3UnitTxdmaDummy
*
* DESCRIPTION:
*       Allocate address type specific dummy memories -- for the dummy TXDMA unit
*
* INPUTS:
*       devObjPtr           - pointer to device memory object.
*       unitPtr             - pointer to the unit chunk
*       unitBaseAddr        - unit base address
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*       Need to accept access to TXDMA interrupt registers.
*
*******************************************************************************/
void smemLion3UnitTxdmaDummy
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0130, 0x0140)}
        };

        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
}

/*******************************************************************************
*   smemLion3UnitTxFifo
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Lion3  TX Fifo unit
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
static void smemLion3UnitTxFifo
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000000, 0x0000012C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000400, 0x00000434)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000440, 0x00000444)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000508, 0x0000050C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000518, 0x0000051C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000600, 0x00000724)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000800, 0x00000958)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000960, 0x00000960)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00001100, 0x00001224)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00001300, 0x00001424)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00001500, 0x00001624)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00001700, 0x00001824)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00001900, 0x00001A24)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00001B00, 0x00001C24)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00003000, 0x00003004)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00003100, 0x00003224)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00003300, 0x00003300)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00010000, 2032)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00012000, 32)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00014000, 0x00014000)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00015000, 296)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00015200, 296)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00015400, 296)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00015600, 296)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00015800, 296)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00015A00, 296)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00017000, 296)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00017200, 296)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00017400, 296)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00017600, 296)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00017800, 296)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00017A00, 296)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00020000, 2032)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00022000, 4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00024000, 0x00024000)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00025000, 296)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00025200, 296)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00025400, 296)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00025600, 296)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00025800, 296)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00025A00, 296)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00027000, 296)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00027200, 296)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00027400, 296)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00027600, 296)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00027800, 296)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00027A00, 296)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
        {
            SMEM_CHUNK_BASIC_STC  chunksMemBc2A0[]=
            {
                {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00001000, 0x00001018)}

            };

            SMEM_CHUNK_BASIC_STC  chunksMemBc2B0[]=
            {
                {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00001000, 0x00001024)}
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

}

/*******************************************************************************
*   smemLion3UnitLmsUnify
*
* DESCRIPTION:
*       Allocate address type specific memories - for the any of the 3 LMS units
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr     - pointer to the unit chunk
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
static void smemLion3UnitLmsUnify
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    unitPtr->numOfUnits = 4;
    unitPtr->chunkType = SMEM_UNIT_CHUNK_TYPE_8_MSB_E;

    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00004000, 0x00004004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00004010, 0x00004010)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00004020, 0x00004028)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00004030, 0x00004034)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00004054, 0x00004054)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00004100, 0x0000410C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00004140, 0x00004144)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00004200, 0x00004200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00005100, 0x00005114)},

            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01004000, 0x01004004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01004020, 0x01004024)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01004030, 0x01004034)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01004100, 0x01004104)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01004108, 0x0100410C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01004140, 0x01004144)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01005100, 0x01005104)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01005110, 0x01005114)},

            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02004000, 0x02004004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02004010, 0x02004010)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02004020, 0x02004020)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02004030, 0x02004034)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02004054, 0x02004054)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02004100, 0x0200410C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02004140, 0x02004144)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02004200, 0x02004200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02005100, 0x02005104)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02005110, 0x02005114)},

            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x03004000, 0x03004004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x03004020, 0x03004024)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x03004030, 0x03004034)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x03004100, 0x0300410C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x03004140, 0x03004144)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x03005100, 0x03005104)}
        };

        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }

}

/*******************************************************************************
*   smemLion3UnitMg
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Lion3 MG unit
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
static void smemLion3UnitMg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    /*NOTE: took the MG memory space from BC2_A0 and BC2_B0 */

    if(SMEM_CHT_IS_SIP5_10_GET(devObjPtr) == 0)
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000000, 0x00000000)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000008, 0x00000008)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000010, 0x00000028)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000030, 0x00000044)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000004C, 0x00000060)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000070, 0x00000084)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000090, 0x00000098)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000000A4, 0x00000100)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000110, 0x00000110)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000120, 0x00000140)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000150, 0x0000017C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000200, 0x00000288)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000380, 0x00000390)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000003A0, 0x000003C0)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000003F0, 0x000003FC)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000500, 0x0000051C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000530, 0x00000530)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000600, 0x00000604)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000610, 0x00000654)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000670, 0x000006B4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002600, 0x00002684)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000026C0, 0x000026DC)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002700, 0x00002708)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002710, 0x00002718)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002720, 0x00002728)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002730, 0x00002738)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002740, 0x00002748)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002750, 0x00002758)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002760, 0x00002768)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002770, 0x00002778)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002780, 0x00002780)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002800, 0x00002800)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000280C, 0x00002868)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002870, 0x000028F0)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00020000, 32768)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00030000, 65536)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00040000, 0x00040014)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00042000 ,0x00042014)}

            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00080000, 0x0008000C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0008001C, 0x0008001C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00091000, 0x00091010)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0009101C, 0x0009101C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000910C0, 0x000910E0)}
        };

        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
    else
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000000, 0x00000000)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000008, 0x00000008)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000010, 0x00000044)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000004C, 0x00000068)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000070, 0x00000084)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000090, 0x00000148)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000150, 0x0000017C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000200, 0x00000288)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000290, 0x00000298)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000002A0, 0x000002A8)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000002B0, 0x000002B8)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000002C0, 0x000002C8)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000002D0, 0x00000324)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000330, 0x00000338)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000340, 0x00000344)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000034C, 0x0000034C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000360, 24)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000380, 0x00000388)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000390, 0x00000390)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000003A0, 0x000003C0)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000003F0, 0x000003FC)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000500, 0x00000520)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000530, 0x00000540)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000600, 0x00000604)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000610, 0x00000654)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000674, 0x000006B4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000B78, 0x00000B78)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002600, 0x00002684)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000026C0, 0x000026DC)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002700, 0x00002708)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002710, 0x00002718)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002720, 0x00002728)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002730, 0x00002738)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002740, 0x00002748)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002750, 0x00002758)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002760, 0x00002768)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002770, 0x00002778)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002780, 0x00002780)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002800, 0x00002800)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000280C, 0x00002868)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002870, 0x000028F4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00030000, 8192)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00032000, 8192)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00040000, 131072)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00080000, 0x0008000C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0008001C, 0x0008001C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00091000, 0x00091010)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0009101C, 0x0009101C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000910C0, 0x000910E0)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
}
/*******************************************************************************
*   smemLion3UnitMemoryBindToChunk
*
* DESCRIPTION:
*       bind the units to the specific unit chunk with the generic function
*       build addresses of the units according to base address
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
void smemLion3UnitMemoryBindToChunk
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    smemLion2UnitMemoryBindToChunk(devObjPtr);
}

/*******************************************************************************
*   smemLion3AllocSpecMemory
*
* DESCRIPTION:
*       Allocate address type specific memories.
*
* INPUTS:
*       commonDevMemInfoPtr   - pointer to common device memory object.
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
void smemLion3AllocSpecMemory
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_CHT_DEV_COMMON_MEM_INFO  * commonDevMemInfoPtr
)
{
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = devObjPtr->deviceMemory;
    SMEM_UNIT_CHUNKS_STC    *currUnitChunkPtr;
    GT_U32 unitIndex;
    GT_U32 portGroupIdMask,portGroupIdPattern;/* port group id mask and pattern for shared units */
    GT_U32 portGroupId;/* port group id for shared units */

    smemLion2AllocSpecMemory(devObjPtr,commonDevMemInfoPtr);

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_MG)];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemLion3UnitMg(devObjPtr, currUnitChunkPtr);
    }

    /* not for bobcat2 */
    if(devObjPtr->portGroupSharedDevObjPtr)
    {
        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_DFX_SERVER)];
        if(currUnitChunkPtr->numOfChunks == 0)
        {
            smemLion3UnitDfxServerDummy(devObjPtr, currUnitChunkPtr);
        }
    }

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TTI)];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemLion3UnitTti(devObjPtr, currUnitChunkPtr);
    }

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_L2I)];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemLion3UnitL2i(devObjPtr, currUnitChunkPtr);
    }

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_RX_DMA)];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemLion3UnitRxDma(devObjPtr,currUnitChunkPtr);
    }

    unitIndex = UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_FDB);
    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[unitIndex];

    /* init single instance */
    portGroupIdMask = 0xFFFFFFFF;/* exact match */
    portGroupIdPattern = 0;      /* single port group 0*/
    portGroupId = (portGroupIdPattern & portGroupIdMask);

    if(portGroupId == (devObjPtr->portGroupId & portGroupIdMask) )
    {
        if(currUnitChunkPtr->numOfChunks == 0)
        {
            /* generate memory space for the FDB unit */
            smemLion3UnitFdb(devObjPtr,currUnitChunkPtr);
        }

        currUnitChunkPtr = &devMemInfoPtr->common.pciExtMemArr[SMEM_UNIT_PCI_BUS_DFX_E].unitMem;
        /* generate memory space for DFX server */
        if(currUnitChunkPtr->numOfChunks == 0)
        {
            smemLion3UnitDfx(devObjPtr,
                             &devMemInfoPtr->common.pciExtMemArr[SMEM_UNIT_PCI_BUS_DFX_E].unitMem,
                             devMemInfoPtr->common.pciExtMemArr[SMEM_UNIT_PCI_BUS_DFX_E].unitBaseAddr);
        }

    }
    else
    {
        /* bind unit of one port group to unit of another port group (shared units) */
        BIND_UNIT_TO_OTHER_PORT_GROUP_MAC(devObjPtr,(devObjPtr->portGroupId & (~portGroupIdMask)),unitIndex,currUnitChunkPtr);
        currUnitChunkPtr = &devMemInfoPtr->common.pciExtMemArr[SMEM_UNIT_PCI_BUS_DFX_E].unitMem;
        /* bind DFX external memory of one port group to DFX external memory of another port group (shared units) */
        BIND_DFX_EXTERNAL_MEMORY_TO_OTHER_PORT_GROUP_MAC(devObjPtr,
                         (devObjPtr->portGroupId & (~portGroupIdMask)), currUnitChunkPtr);
    }

    unitIndex = UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_FDB_TABLE_0);
    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[unitIndex];
    /* init single instance */
    portGroupIdMask = 0xFFFFFFFF;/* exact match */
    portGroupIdPattern = 0;      /* single port group 0*/
    portGroupId = (portGroupIdPattern & portGroupIdMask);

    if(portGroupId == (devObjPtr->portGroupId & portGroupIdMask) )
    {
        if(currUnitChunkPtr->numOfChunks == 0)
        {
            /* generate memory space for the FDB table */
            smemLion3UnitFdbTable(devObjPtr,currUnitChunkPtr);
        }
    }
    else
    {
        /* bind unit of one port group to unit of another port group (shared units) */
        BIND_UNIT_TO_OTHER_PORT_GROUP_MAC(devObjPtr,(devObjPtr->portGroupId & (~portGroupIdMask)),unitIndex,currUnitChunkPtr);
    }
    /* 'connect' the next sub units to the first one
       and connect 'my self' */
    devMemInfoPtr->unitMemArr[unitIndex].hugeUnitSupportPtr =
    devMemInfoPtr->unitMemArr[unitIndex + 1].hugeUnitSupportPtr =
        &devMemInfoPtr->unitMemArr[unitIndex];

    unitIndex = UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_EGF_EFT);
    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[unitIndex];
    /* init only for 'even' port groups */
    portGroupIdMask = 1;/* 'even port groups' */
    portGroupIdPattern = 0;      /* 'even port groups' */
    portGroupId = (portGroupIdPattern & portGroupIdMask);

    if(portGroupId == (devObjPtr->portGroupId & portGroupIdMask) )
    {
        if(currUnitChunkPtr->numOfChunks == 0)
        {
            smemLion3UnitEgfEft(devObjPtr,currUnitChunkPtr);
        }
    }
    else
    {
        /* bind unit of one port group to unit of another port group (shared units) */
        BIND_UNIT_TO_OTHER_PORT_GROUP_MAC(devObjPtr,(devObjPtr->portGroupId & (~portGroupIdMask)),unitIndex,currUnitChunkPtr);
    }
    devMemInfoPtr->unitMemArr[unitIndex].hugeUnitSupportPtr =
    devMemInfoPtr->unitMemArr[unitIndex + 1].hugeUnitSupportPtr =
        &devMemInfoPtr->unitMemArr[unitIndex];

    unitIndex = UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_EGF_SHT);
    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[unitIndex];
    /* init only for 'even' port groups */
    portGroupIdMask = 1;/* 'even port groups' */
    portGroupIdPattern = 0;      /* 'even port groups' */
    portGroupId = (portGroupIdPattern & portGroupIdMask);

    if(portGroupId == (devObjPtr->portGroupId & portGroupIdMask) )
    {
        if(currUnitChunkPtr->numOfChunks == 0)
        {
            smemLion3UnitEgfSht(devObjPtr,currUnitChunkPtr);
        }

    }
    else
    {
        /* bind unit of one port group to unit of another port group (shared units) */
        BIND_UNIT_TO_OTHER_PORT_GROUP_MAC(devObjPtr,(devObjPtr->portGroupId & (~portGroupIdMask)),unitIndex,currUnitChunkPtr);
    }
    /* 'connect' the next sub units to the first one
       and connect 'my self' */
    devMemInfoPtr->unitMemArr[unitIndex+ 0].hugeUnitSupportPtr =
    devMemInfoPtr->unitMemArr[unitIndex+ 1].hugeUnitSupportPtr =
    devMemInfoPtr->unitMemArr[unitIndex+ 2].hugeUnitSupportPtr =
    devMemInfoPtr->unitMemArr[unitIndex+ 3].hugeUnitSupportPtr =
    devMemInfoPtr->unitMemArr[unitIndex+ 4].hugeUnitSupportPtr =
    devMemInfoPtr->unitMemArr[unitIndex+ 5].hugeUnitSupportPtr =
    devMemInfoPtr->unitMemArr[unitIndex+ 6].hugeUnitSupportPtr =
    devMemInfoPtr->unitMemArr[unitIndex+ 7].hugeUnitSupportPtr =
    devMemInfoPtr->unitMemArr[unitIndex+ 8].hugeUnitSupportPtr =
    devMemInfoPtr->unitMemArr[unitIndex+ 9].hugeUnitSupportPtr =
    devMemInfoPtr->unitMemArr[unitIndex+10].hugeUnitSupportPtr =
    devMemInfoPtr->unitMemArr[unitIndex+11].hugeUnitSupportPtr =
    devMemInfoPtr->unitMemArr[unitIndex+12].hugeUnitSupportPtr =
        &devMemInfoPtr->unitMemArr[unitIndex];

    unitIndex = UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_EGF_QAG);
    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[unitIndex];
    /* init only for 'quad' port groups */
    portGroupIdMask = 3;/* 'quad port groups' */
    portGroupIdPattern = 0;      /* 'quad port groups' */
    portGroupId = (portGroupIdPattern & portGroupIdMask);

    if(portGroupId == (devObjPtr->portGroupId & portGroupIdMask) )
    {
        if(currUnitChunkPtr->numOfChunks == 0)
        {
            smemLion3UnitEgfQag(devObjPtr,currUnitChunkPtr);
        }
    }
    else
    {
        /* bind unit of one port group to unit of another port group (shared units) */
        BIND_UNIT_TO_OTHER_PORT_GROUP_MAC(devObjPtr,(devObjPtr->portGroupId & (~portGroupIdMask)),unitIndex,currUnitChunkPtr);
    }
    /* 'connect' the next sub units to the first one
       and connect 'my self' */
    devMemInfoPtr->unitMemArr[unitIndex].hugeUnitSupportPtr =
    devMemInfoPtr->unitMemArr[unitIndex + 1].hugeUnitSupportPtr =
        &devMemInfoPtr->unitMemArr[unitIndex];


    unitIndex = UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TCAM);
    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[unitIndex];
    /* init only for 'quad' port groups */
    portGroupIdMask = 3;/* 'quad port groups' */
    portGroupIdPattern = 0;      /* 'quad port groups' */
    portGroupId = (portGroupIdPattern & portGroupIdMask);

    if(portGroupId == (devObjPtr->portGroupId & portGroupIdMask) )
    {
        if(currUnitChunkPtr->numOfChunks == 0)
        {
            smemLion3UnitTcam(devObjPtr, currUnitChunkPtr);
        }
    }
    else
    {
        /* bind unit of one port group to unit of another port group (shared units) */
        BIND_UNIT_TO_OTHER_PORT_GROUP_MAC(devObjPtr,(devObjPtr->portGroupId & (~portGroupIdMask)),unitIndex,currUnitChunkPtr);
    }

    unitIndex = UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_EQ);
    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[unitIndex];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemLion3UnitEq(devObjPtr,currUnitChunkPtr);
    }
    /* 'connect' the next sub units to the first one
       and connect 'my self' */
    devMemInfoPtr->unitMemArr[unitIndex].hugeUnitSupportPtr =
    devMemInfoPtr->unitMemArr[unitIndex+1].hugeUnitSupportPtr =
        &devMemInfoPtr->unitMemArr[unitIndex];

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_HA)];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemLion3UnitHa(devObjPtr,currUnitChunkPtr);
    }

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_IPCL)];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemLion3UnitIpcl(devObjPtr,currUnitChunkPtr);
    }

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_EPCL)];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemLion3UnitEpcl(devObjPtr,currUnitChunkPtr);
    }

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_IPVX)];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemLion3UnitIpvx(devObjPtr,currUnitChunkPtr);
    }

    unitIndex = UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_MLL);
    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[unitIndex];
    /* init only for 'even' port groups */
    portGroupIdMask = 1;/* 'even port groups' */
    portGroupIdPattern = 0;      /* 'even port groups' */
    portGroupId = (portGroupIdPattern & portGroupIdMask);

    if(portGroupId == (devObjPtr->portGroupId & portGroupIdMask) )
    {
        if(currUnitChunkPtr->numOfChunks == 0)
        {
            smemLion3UnitMll(devObjPtr,currUnitChunkPtr);
        }
    }
    else
    {
        /* bind unit of one port group to unit of another port group (shared units) */
        BIND_UNIT_TO_OTHER_PORT_GROUP_MAC(devObjPtr,(devObjPtr->portGroupId & (~portGroupIdMask)),unitIndex,currUnitChunkPtr);
    }

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_IOAM)];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemLion3UnitIOamUnify(devObjPtr,currUnitChunkPtr);
    }

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_EOAM)];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemLion3UnitEOamUnify(devObjPtr,currUnitChunkPtr);
    }

    /*egress remark */
    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_ERMRK)];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemLion3UnitErmrk(devObjPtr,currUnitChunkPtr);
    }

    /* GOP */
    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_GOP)];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        /* in bc2 'UNIT_GOP' is adding to 'UNIT_GOP' */
        smemLion3UnitGop(devObjPtr,currUnitChunkPtr);
    }

    /* lpm */
    unitIndex = UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_LPM);
    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[unitIndex];
    /* init only for 'even' port groups */
    portGroupIdMask = 1;/* 'even port groups' */
    portGroupIdPattern = 0;      /* 'even port groups' */
    portGroupId = (portGroupIdPattern & portGroupIdMask);

    if(portGroupId == (devObjPtr->portGroupId & portGroupIdMask) )
    {
        if(currUnitChunkPtr->numOfChunks == 0)
        {
            smemLion3UnitLpm(devObjPtr,currUnitChunkPtr);
        }
    }
    else
    {
        /* bind unit of one port group to unit of another port group (shared units) */
        BIND_UNIT_TO_OTHER_PORT_GROUP_MAC(devObjPtr,(devObjPtr->portGroupId & (~portGroupIdMask)),unitIndex,currUnitChunkPtr);
    }
    /* 'connect' the next sub units to the first one
       and connect 'my self' */
    devMemInfoPtr->unitMemArr[unitIndex].hugeUnitSupportPtr =
    devMemInfoPtr->unitMemArr[unitIndex + 1].hugeUnitSupportPtr =
        &devMemInfoPtr->unitMemArr[unitIndex];

    /* not needed for bc2 */
    if(devObjPtr->portGroupSharedDevObjPtr)
    {
        smemLion3UnitGts(devObjPtr,0);
        smemLion3UnitGts(devObjPtr,1);
    }


    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_IPLR)];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemLion3UnitIplr0(devObjPtr,currUnitChunkPtr);
    }

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_IPLR1)];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemLion3UnitIplr1(devObjPtr, currUnitChunkPtr);
    }

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_EPLR)];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemLion3UnitEplr(devObjPtr,currUnitChunkPtr);
    }

    /* CNC[0]*/
    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_CNC)];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemLion3UnitCnc(devObjPtr,currUnitChunkPtr,0);
    }

    /* CNC[1]*/
    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_CNC_1)];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemLion3UnitCnc(devObjPtr,currUnitChunkPtr,1);
    }
    /*
        currently align Lion3 to be like Lion2 in terms of
        core 0..3 in TXQ0
        core 4..7 in TXQ1
    */

    /* init only for 'quad' port groups */
    portGroupIdMask = 3;/* 'quad port groups' */
    portGroupIdPattern = 0;      /* 'quad port groups' */
    portGroupId = (portGroupIdPattern & portGroupIdMask);

    if(portGroupId == (devObjPtr->portGroupId & portGroupIdMask) )
    {
        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TXQ_QUEUE)];
        if(currUnitChunkPtr->numOfChunks == 0)
        {
            smemLion3UnitTxqQueue(devObjPtr,currUnitChunkPtr);
        }

        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TXQ_LL)];
        if(currUnitChunkPtr->numOfChunks == 0)
        {
            smemLion3UnitTxqLl(devObjPtr,currUnitChunkPtr);
        }

        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TXQ_QCN)];
        if(currUnitChunkPtr->numOfChunks == 0)
        {
            smemLion3UnitTxqQcn(devObjPtr,currUnitChunkPtr);
        }
    }
    else
    {
        /* bind unit of one port group to unit of another port group (shared units) */
        unitIndex = UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TXQ_QUEUE);
        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[unitIndex];
        BIND_UNIT_TO_OTHER_PORT_GROUP_MAC(devObjPtr,(devObjPtr->portGroupId & (~portGroupIdMask)),unitIndex,currUnitChunkPtr);
        unitIndex = UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TXQ_LL);
        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[unitIndex];
        BIND_UNIT_TO_OTHER_PORT_GROUP_MAC(devObjPtr,(devObjPtr->portGroupId & (~portGroupIdMask)),unitIndex,currUnitChunkPtr);
        unitIndex = UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TXQ_QCN);
        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[unitIndex];
        BIND_UNIT_TO_OTHER_PORT_GROUP_MAC(devObjPtr,(devObjPtr->portGroupId & (~portGroupIdMask)),unitIndex,currUnitChunkPtr);
    }

    /* init single instance */
    portGroupIdMask = 0xFFFFFFFF;/* exact match */
    portGroupIdPattern = 0;      /* 'quad port groups' */
    portGroupId = (portGroupIdPattern & portGroupIdMask);

    unitIndex = UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_CPFC);
    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[unitIndex];

    if(portGroupId == (devObjPtr->portGroupId & portGroupIdMask) )
    {
        if(currUnitChunkPtr->numOfChunks == 0)
        {
            smemLion3UnitTxqPfc(devObjPtr,currUnitChunkPtr);
        }
    }
    else
    {
        /* bind unit of one port group to unit of another port group (shared units) */
        BIND_UNIT_TO_OTHER_PORT_GROUP_MAC(devObjPtr,(devObjPtr->portGroupId & (~portGroupIdMask)),unitIndex,currUnitChunkPtr);
    }

    /*instance per core*/
    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TXQ_DQ)];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemLion3UnitTxqDq(devObjPtr,currUnitChunkPtr);
    }

    /* BM */
    unitIndex = UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_BM);
    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[unitIndex];

    SIM_TBD_BOOKMARK
#if 0 /*need to be in single core , but currently cpss not aware*/
    /* init single instance */
    portGroupIdMask = 0xFFFFFFFF;/* exact match */
#else
    /* init all instances */
    portGroupIdMask = 0;/* all match */
#endif/**/
    portGroupIdPattern = 0;      /* single port group 0*/
    portGroupId = (portGroupIdPattern & portGroupIdMask);

    if(portGroupId == (devObjPtr->portGroupId & portGroupIdMask) )
    {
        if(currUnitChunkPtr->numOfChunks == 0)
        {
            smemLion3UnitBm(devObjPtr,currUnitChunkPtr);
        }
    }
    else
    {
        /* bind unit of one port group to unit of another port group (shared units) */
        BIND_UNIT_TO_OTHER_PORT_GROUP_MAC(devObjPtr,(devObjPtr->portGroupId & (~portGroupIdMask)),unitIndex,currUnitChunkPtr);
    }


    /* BMA */
    unitIndex = UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_BMA);
    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[unitIndex];

    /* init single instance */
    portGroupIdMask = 0xFFFFFFFF;/* exact match */
    portGroupIdPattern = 0;      /* single port group 0*/
    portGroupId = (portGroupIdPattern & portGroupIdMask);

    if(portGroupId == (devObjPtr->portGroupId & portGroupIdMask) )
    {
        if(currUnitChunkPtr->numOfChunks == 0)
        {
            smemLion3UnitBma(devObjPtr,currUnitChunkPtr);
        }
    }
    else
    {
        /* bind unit of one port group to unit of another port group (shared units) */
        BIND_UNIT_TO_OTHER_PORT_GROUP_MAC(devObjPtr,(devObjPtr->portGroupId & (~portGroupIdMask)),unitIndex,currUnitChunkPtr);
    }

    unitIndex = UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TX_DMA);
    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[unitIndex];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemLion3UnitTxDma(devObjPtr,currUnitChunkPtr);
    }

    unitIndex = UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TX_FIFO);
    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[unitIndex];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemLion3UnitTxFifo(devObjPtr,currUnitChunkPtr);
    }

    /* LMS0 */
    if(SMAIN_NOT_VALID_CNS != UNIT_BASE_ADDR_GET_ALLOW_NON_EXIST_UNIT_MAC(devObjPtr,UNIT_LMS))
    {
        unitIndex = UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr, UNIT_LMS);
        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[unitIndex];
        if(currUnitChunkPtr->numOfChunks == 0)
        {
            smemLion3UnitLmsUnify(devObjPtr, currUnitChunkPtr);
        }
       /* 'connect' the next sub units to the first one
           and connect 'my self' */
        devMemInfoPtr->unitMemArr[unitIndex].hugeUnitSupportPtr =
        devMemInfoPtr->unitMemArr[unitIndex + 2].hugeUnitSupportPtr =
        devMemInfoPtr->unitMemArr[unitIndex + 4].hugeUnitSupportPtr =
        devMemInfoPtr->unitMemArr[unitIndex + 6].hugeUnitSupportPtr =
            &devMemInfoPtr->unitMemArr[unitIndex];
    }

    /* LMS1 */
    if(SMAIN_NOT_VALID_CNS != UNIT_BASE_ADDR_GET_ALLOW_NON_EXIST_UNIT_MAC(devObjPtr,UNIT_LMS1))
    {
        unitIndex = UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr, UNIT_LMS1);
        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[unitIndex];
        if(currUnitChunkPtr->numOfChunks == 0)
        {
            smemLion3UnitLmsUnify(devObjPtr, currUnitChunkPtr);
        }
       /* 'connect' the next sub units to the first one
           and connect 'my self' */
        devMemInfoPtr->unitMemArr[unitIndex].hugeUnitSupportPtr =
        devMemInfoPtr->unitMemArr[unitIndex + 2].hugeUnitSupportPtr =
        devMemInfoPtr->unitMemArr[unitIndex + 4].hugeUnitSupportPtr =
        devMemInfoPtr->unitMemArr[unitIndex + 6].hugeUnitSupportPtr =
            &devMemInfoPtr->unitMemArr[unitIndex];
    }

    /* LMS2 */
    if(SMAIN_NOT_VALID_CNS != UNIT_BASE_ADDR_GET_ALLOW_NON_EXIST_UNIT_MAC(devObjPtr,UNIT_LMS2))
    {
        unitIndex = UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr, UNIT_LMS2);
        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[unitIndex];
        if(currUnitChunkPtr->numOfChunks == 0)
        {
            smemLion3UnitLmsUnify(devObjPtr, currUnitChunkPtr);
        }
       /* 'connect' the next sub units to the first one
           and connect 'my self' */
        devMemInfoPtr->unitMemArr[unitIndex].hugeUnitSupportPtr =
        devMemInfoPtr->unitMemArr[unitIndex + 2].hugeUnitSupportPtr =
        devMemInfoPtr->unitMemArr[unitIndex + 4].hugeUnitSupportPtr =
        devMemInfoPtr->unitMemArr[unitIndex + 6].hugeUnitSupportPtr =
            &devMemInfoPtr->unitMemArr[unitIndex];
    }
}

/*******************************************************************************
*  smemLion3TableInfoSet
*
* DESCRIPTION:
*       set the table info for the device --> fill devObjPtr->tablesInfo
*
* INPUTS:
*       devObjPtr   - device object PTR.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
void smemLion3TableInfoSet
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    /* use lion2 table addresses */
    smemLion2TableInfoSet(devObjPtr);

    /*******************/
    /* override lion2  */
    /*******************/

    /* tti - Physical Port Attribute Table */
    devObjPtr->tablesInfo.ttiPhysicalPortAttribute.paramInfo[0].modulo =
                    (1 << devObjPtr->flexFieldNumBitsSupport.phyPort);


    /*Post-TTI Lookup Ingress ePort Table*/
    devObjPtr->tablesInfo.ttiPreTtiLookupIngressEPort.paramInfo[0].modulo =
                    LION3_NUM_DEFAULT_E_PORTS_CNS;

    /*Post-TTI Lookup Ingress ePort Table*/
    devObjPtr->tablesInfo.ttiPostTtiLookupIngressEPort.paramInfo[0].modulo =
                    (1 << devObjPtr->flexFieldNumBitsSupport.ePort);

    /*Bridge Ingress ePort Table*/
    devObjPtr->tablesInfo.bridgeIngressEPort.paramInfo[0].modulo =
                    (1 << devObjPtr->flexFieldNumBitsSupport.ePort);

    devObjPtr->tablesInfo.eqL2EcmpLtt.paramInfo[0].modulo =
                    (1 << devObjPtr->flexFieldNumBitsSupport.ePort);

    devObjPtr->tablesInfo.eqE2Phy.paramInfo[0].modulo =
                    (1 << devObjPtr->flexFieldNumBitsSupport.ePort);

    devObjPtr->tablesInfo.eqL2Ecmp.paramInfo[0].modulo = devObjPtr->limitedResources.l2Ecmp;

    devObjPtr->tablesInfo.sst.paramInfo[0].modulo =
                    (1 << devObjPtr->flexFieldNumBitsSupport.sstId);

    devObjPtr->tablesInfo.stp.paramInfo[0].modulo =
        1 << devObjPtr->flexFieldNumBitsSupport.stgId;

    devObjPtr->tablesInfo.egressStp.paramInfo[0].modulo =
        1 << devObjPtr->flexFieldNumBitsSupport.stgId;

    devObjPtr->tablesInfo.policerEPortEVlanTrigger.commonInfo.multiInstanceInfo.numBaseAddresses = 3;
    devObjPtr->tablesInfo.policerEPortEVlanTrigger.commonInfo.multiInstanceInfo.multiUnitsBaseAddressPtr = &devObjPtr->memUnitBaseAddrInfo.policer[0];
    /*devObjPtr->tablesInfo.policerEPortEVlanTrigger.commonInfo.multiInstanceInfo.multiUnitsBaseAddressValidPtr = &policerTablesNotInIplr1[0];*/
    devObjPtr->tablesInfo.policerEPortEVlanTrigger.paramInfo[0].modulo =
                    (8 * _1K);
    /* PLR tables */
    devObjPtr->tablesInfo.policerHierarchicalQos.commonInfo.multiInstanceInfo.numBaseAddresses = 3;
    devObjPtr->tablesInfo.policerHierarchicalQos.commonInfo.multiInstanceInfo.multiUnitsBaseAddressPtr = &devObjPtr->memUnitBaseAddrInfo.policer[0];

    devObjPtr->tablesInfo.haEgressEPortAttr1.paramInfo[0].modulo =
                    (1 << devObjPtr->flexFieldNumBitsSupport.ePort);

    devObjPtr->tablesInfo.haEgressEPortAttr2.paramInfo[0].modulo =
                    (1 << devObjPtr->flexFieldNumBitsSupport.ePort);

    /* OAM Tables */
    devObjPtr->tablesInfo.oamTable.commonInfo.baseAddress =
                SMEM_LION3_OAM_OFFSET_MAC(devObjPtr, 0, 0x00070000);
    devObjPtr->tablesInfo.oamTable.paramInfo[0].step = 0x10; /* entry */
    devObjPtr->tablesInfo.oamTable.commonInfo.multiInstanceInfo.numBaseAddresses = 2;
    devObjPtr->tablesInfo.oamTable.commonInfo.multiInstanceInfo.multiUnitsBaseAddressPtr = &devObjPtr->memUnitBaseAddrInfo.oam[0];

    devObjPtr->tablesInfo.oamAgingTable.commonInfo.baseAddress =
                SMEM_LION3_OAM_OFFSET_MAC(devObjPtr, 0, 0x00000C00);
    devObjPtr->tablesInfo.oamAgingTable.paramInfo[0].step = 0x4; /* entry */
    devObjPtr->tablesInfo.oamAgingTable.commonInfo.multiInstanceInfo.numBaseAddresses = 2;
    devObjPtr->tablesInfo.oamAgingTable.commonInfo.multiInstanceInfo.multiUnitsBaseAddressPtr = &devObjPtr->memUnitBaseAddrInfo.oam[0];

    devObjPtr->tablesInfo.oamMegExceptionTable.commonInfo.baseAddress =
                SMEM_LION3_OAM_OFFSET_MAC(devObjPtr, 0, 0x00010000);
    devObjPtr->tablesInfo.oamMegExceptionTable.paramInfo[0].step = 0x4; /* entry */
    devObjPtr->tablesInfo.oamMegExceptionTable.commonInfo.multiInstanceInfo.numBaseAddresses = 2;
    devObjPtr->tablesInfo.oamMegExceptionTable.commonInfo.multiInstanceInfo.multiUnitsBaseAddressPtr = &devObjPtr->memUnitBaseAddrInfo.oam[0];

    devObjPtr->tablesInfo.oamSrcInterfaceExceptionTable.commonInfo.baseAddress =
                SMEM_LION3_OAM_OFFSET_MAC(devObjPtr, 0, 0x00018000);
    devObjPtr->tablesInfo.oamSrcInterfaceExceptionTable.paramInfo[0].step = 0x4; /* entry */
    devObjPtr->tablesInfo.oamSrcInterfaceExceptionTable.commonInfo.multiInstanceInfo.numBaseAddresses = 2;
    devObjPtr->tablesInfo.oamSrcInterfaceExceptionTable.commonInfo.multiInstanceInfo.multiUnitsBaseAddressPtr = &devObjPtr->memUnitBaseAddrInfo.oam[0];

    devObjPtr->tablesInfo.oamInvalidKeepAliveHashTable.commonInfo.baseAddress =
                SMEM_LION3_OAM_OFFSET_MAC(devObjPtr, 0, 0x00020000);
    devObjPtr->tablesInfo.oamInvalidKeepAliveHashTable.paramInfo[0].step = 0x4; /* entry */
    devObjPtr->tablesInfo.oamInvalidKeepAliveHashTable.commonInfo.multiInstanceInfo.numBaseAddresses = 2;
    devObjPtr->tablesInfo.oamInvalidKeepAliveHashTable.commonInfo.multiInstanceInfo.multiUnitsBaseAddressPtr = &devObjPtr->memUnitBaseAddrInfo.oam[0];

    devObjPtr->tablesInfo.oamExcessKeepAliveTable.commonInfo.baseAddress =
                SMEM_LION3_OAM_OFFSET_MAC(devObjPtr, 0, 0x00028000);
    devObjPtr->tablesInfo.oamExcessKeepAliveTable.paramInfo[0].step = 0x4; /* entry */
    devObjPtr->tablesInfo.oamExcessKeepAliveTable.commonInfo.multiInstanceInfo.numBaseAddresses = 2;
    devObjPtr->tablesInfo.oamExcessKeepAliveTable.commonInfo.multiInstanceInfo.multiUnitsBaseAddressPtr = &devObjPtr->memUnitBaseAddrInfo.oam[0];

    devObjPtr->tablesInfo.oamExceptionSummaryTable.commonInfo.baseAddress =
                SMEM_LION3_OAM_OFFSET_MAC(devObjPtr, 0, 0x00030000);
    devObjPtr->tablesInfo.oamExceptionSummaryTable.paramInfo[0].step = 0x4; /* entry */
    devObjPtr->tablesInfo.oamExceptionSummaryTable.commonInfo.multiInstanceInfo.numBaseAddresses = 2;
    devObjPtr->tablesInfo.oamExceptionSummaryTable.commonInfo.multiInstanceInfo.multiUnitsBaseAddressPtr = &devObjPtr->memUnitBaseAddrInfo.oam[0];

    devObjPtr->tablesInfo.oamRdiStatusChangeExceptionTable.commonInfo.baseAddress =
                SMEM_LION3_OAM_OFFSET_MAC(devObjPtr, 0, 0x00038000);
    devObjPtr->tablesInfo.oamRdiStatusChangeExceptionTable.paramInfo[0].step = 0x4; /* entry */
    devObjPtr->tablesInfo.oamRdiStatusChangeExceptionTable.commonInfo.multiInstanceInfo.numBaseAddresses = 2;
    devObjPtr->tablesInfo.oamRdiStatusChangeExceptionTable.commonInfo.multiInstanceInfo.multiUnitsBaseAddressPtr = &devObjPtr->memUnitBaseAddrInfo.oam[0];

    devObjPtr->tablesInfo.oamTxPeriodExceptionTable.commonInfo.baseAddress =
                SMEM_LION3_OAM_OFFSET_MAC(devObjPtr, 0, 0x00040000);
    devObjPtr->tablesInfo.oamTxPeriodExceptionTable.paramInfo[0].step = 0x4; /* entry */
    devObjPtr->tablesInfo.oamTxPeriodExceptionTable.commonInfo.multiInstanceInfo.numBaseAddresses = 2;
    devObjPtr->tablesInfo.oamTxPeriodExceptionTable.commonInfo.multiInstanceInfo.multiUnitsBaseAddressPtr = &devObjPtr->memUnitBaseAddrInfo.oam[0];

    devObjPtr->tablesInfo.oamOpCodePacketCommandTable.commonInfo.baseAddress =
                SMEM_LION3_OAM_OFFSET_MAC(devObjPtr, 0, 0x00060000);
    devObjPtr->tablesInfo.oamOpCodePacketCommandTable.paramInfo[0].step = 0x8; /* entry */
    devObjPtr->tablesInfo.oamOpCodePacketCommandTable.commonInfo.multiInstanceInfo.numBaseAddresses = 2;
    devObjPtr->tablesInfo.oamOpCodePacketCommandTable.commonInfo.multiInstanceInfo.multiUnitsBaseAddressPtr = &devObjPtr->memUnitBaseAddrInfo.oam[0];
}


/*******************************************************************************
*  smemLion3MemoriesInit
*
* DESCRIPTION:
*       init the default values of tables - TXQ unit(s)
*
* INPUTS:
*       devObjPtr   - device object PTR.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemLion3MemoriesInitTxq
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    GT_U32  initValueArr[MAX_INIT_WORDS_CNS];
    GT_U32  numWords;
    GT_U32  numEntries;

    numEntries = 64;
    numWords = 0;
    initValueArr[numWords++] = 0x1FFFFFFF;
    SMEM_TABLE_1_PARAM_INIT_MAC(devObjPtr,FC_Mode_Profile_TC_XOff_Thresholds,initValueArr,numWords,numEntries);


    numEntries = 576;
    numWords = 0;
    initValueArr[numWords++] = 0x000004000;
    initValueArr[numWords++] = 0;
    SMEM_TABLE_1_PARAM_INIT_MAC(devObjPtr,CN_Sample_Intervals,initValueArr,numWords,numEntries);

    numEntries = 72;
    numWords = 0;
    /*0x04140020A001050008280040400202001010008080*/
    initValueArr[numWords++] = 0x10008080;
    initValueArr[numWords++] = 0x02020010;
    initValueArr[numWords++] = 0x28004040;
    initValueArr[numWords++] = 0x01050008;
    initValueArr[numWords++] = 0x140020A0;
    initValueArr[numWords++] = 0x04;
    SMEM_TABLE_1_PARAM_INIT_MAC(devObjPtr,Scheduler_State_Variable,initValueArr,numWords,numEntries);


    numEntries = 128;
    numWords = 0;
    initValueArr[numWords++] = 0x000A0019;
    initValueArr[numWords++] = 0;
    SMEM_TABLE_1_PARAM_INIT_MAC(devObjPtr,Shared_Queue_Maximum_Queue_Limits,initValueArr,numWords,numEntries);


    numEntries = 128;
    numWords = 0;
    initValueArr[numWords++] = 0x000A0028;
    initValueArr[numWords++] = 0;
    SMEM_TABLE_1_PARAM_INIT_MAC(devObjPtr,Queue_Buffer_Limits_Dequeue,initValueArr,numWords,numEntries);


    numEntries = 128;
    numWords = 0;
    initValueArr[numWords++] = 0x00000019;
    SMEM_TABLE_1_PARAM_INIT_MAC(devObjPtr,Queue_Descriptor_Limits_Dequeue,initValueArr,numWords,numEntries);


    numEntries = 128;
    numWords = 0;
    /*0x0002800A0019 */
    initValueArr[numWords++] = 0x800A0019;
    initValueArr[numWords++] = 0x0002;
    SMEM_TABLE_1_PARAM_INIT_MAC(devObjPtr,Queue_Limits_DP0_Enqueue,initValueArr,numWords,numEntries);
    /*this table with the same init values as the table above it , but different numEntries */
    numEntries = 256;
    SMEM_TABLE_1_PARAM_INIT_MAC(devObjPtr,Queue_Limits_DP12_Enqueue,initValueArr,numWords,numEntries);
}

/*******************************************************************************
*  smemLion3MemoriesInit
*
* DESCRIPTION:
*       init the default values of tables
*
* INPUTS:
*       devObjPtr   - device object PTR.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemLion3MemoriesInit
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    smemLion3MemoriesInitTxq(devObjPtr);
}

/*******************************************************************************
*  smemLion3TableInfoSetPart2
*
* DESCRIPTION:
*       set the table info for the device --> fill devObjPtr->tablesInfo
*       AFTER the bound of memories (after calling smemBindTablesToMemories)
*
* INPUTS:
*       devObjPtr   - device object PTR.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
void smemLion3TableInfoSetPart2
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    GT_U32  baseAddress;/* base address for the current unit */

    smemLion2TableInfoSetPart2(devObjPtr);

    devObjPtr->tablesInfo.lpmMemory.paramInfo[0].step = 4;
    /* for bobk this value will be different then 0 , and lower then 16k ! */
    devObjPtr->tablesInfo.lpmMemory.paramInfo[0].outOfRangeIndex = devObjPtr->lpmRam.perRamNumEntries;
    devObjPtr->tablesInfo.lpmMemory.paramInfo[1].step = _16K * /* 16k entries between 2 rams */
        devObjPtr->tablesInfo.lpmMemory.paramInfo[0].step;

    devObjPtr->policerSupport.meterTblBaseAddr = devObjPtr->tablesInfo.policer.commonInfo.baseAddress;
    devObjPtr->policerSupport.countTblBaseAddr = devObjPtr->tablesInfo.policerCounters.commonInfo.baseAddress;
    /* new in sip 5_15 */
    if(SMEM_CHT_IS_SIP5_15_GET(devObjPtr))
    {
        devObjPtr->policerSupport.meterConfigTblBaseAddr = devObjPtr->tablesInfo.policerConfig.commonInfo.baseAddress;/* value != 0 in sip5_15 */
        devObjPtr->policerSupport.policerConformanceLevelSignTblBaseAddr = devObjPtr->tablesInfo.policerConformanceLevelSign.commonInfo.baseAddress;/* value != 0 in sip5_15 */
    }

    /* the table was bound to memory that not match step 0 */
    devObjPtr->tablesInfo.portProtocolVidQoSConf.paramInfo[0].step = 0x40;/* port (default ePort) - 0..511 */
    devObjPtr->tablesInfo.portProtocolVidQoSConf.paramInfo[1].step = 0x4; /* protocol - 0..11 */
    devObjPtr->tablesInfo.portProtocolVidQoSConf.paramInfo[1].modulo =
        LION3_NUM_DEFAULT_E_PORTS_CNS;

    /* the table got info about single line , but the TCAM is build as
        'pattern' entry and then as 'mask' entry , so we need to double the step */
    devObjPtr->tablesInfo.ttiTrillAdjacencyTcam.paramInfo[0].step *= 2;

    devObjPtr->ttiTrillAdjacencyTcanInfo.xMemoryAddress =
        devObjPtr->tablesInfo.ttiTrillAdjacencyTcam.commonInfo.baseAddress;
    devObjPtr->ttiTrillAdjacencyTcanInfo.yMemoryAddress =
        devObjPtr->tablesInfo.ttiTrillAdjacencyTcam.commonInfo.baseAddress +
        devObjPtr->tablesInfo.ttiTrillAdjacencyTcam.paramInfo[0].step / 2;


    devObjPtr->tablesInfo.dscpToQoSProfile.paramInfo[0].divider = 0xFFFFFFFF;/* invalidate the parameter ! */
    devObjPtr->tablesInfo.dscpToQoSProfile.paramInfo[0].step = 4;/* dummy parameter */
    devObjPtr->tablesInfo.dscpToQoSProfile.paramInfo[1].step = lion3_dscpToQoSProfile_size; /*table profile*/

    devObjPtr->tablesInfo.upToQoSProfile.paramInfo[0].divider = 0xFFFFFFFF;/* invalidate the parameter ! */
    devObjPtr->tablesInfo.upToQoSProfile.paramInfo[0].step = 4;/* dummy parameter */
    devObjPtr->tablesInfo.upToQoSProfile.paramInfo[1].divider = 0xFFFFFFFF;/* invalidate the parameter ! */
    devObjPtr->tablesInfo.upToQoSProfile.paramInfo[1].step = 4;/* dummy parameter */
    devObjPtr->tablesInfo.upToQoSProfile.paramInfo[2].step = lion3_upToQoSProfile_size; /*table profile*/

    devObjPtr->tablesInfo.expToQoSProfile.paramInfo[0].divider = 0xFFFFFFFF;/* invalidate the parameter ! */
    devObjPtr->tablesInfo.expToQoSProfile.paramInfo[0].step = 4;/* dummy parameter */
    devObjPtr->tablesInfo.expToQoSProfile.paramInfo[1].step = lion3_expToQoSProfile_size; /*table profile*/

    devObjPtr->tablesInfo.dscpToDscpMap.paramInfo[0].divider = 0xFFFFFFFF;/* invalidate the parameter ! */
    devObjPtr->tablesInfo.dscpToDscpMap.paramInfo[0].step = 4;/* dummy parameter */
    devObjPtr->tablesInfo.dscpToDscpMap.paramInfo[1].step = lion3_dscpToDscpMap_size; /*table profile*/

    baseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_TTI);

    devObjPtr->tablesInfo.macToMe.commonInfo.baseAddress = baseAddress + 0x00001600;
    devObjPtr->tablesInfo.macToMe.paramInfo[0].step = 0x20;

    devObjPtr->tablesInfo.ieeeRsrvMcConfTable.paramInfo[0].step = 0x4;
    devObjPtr->tablesInfo.ieeeRsrvMcConfTable.paramInfo[0].divider = 16;
    devObjPtr->tablesInfo.ieeeRsrvMcConfTable.paramInfo[1].step = 0x80;

    devObjPtr->tablesInfo.ieeeRsrvMcCpuIndex.paramInfo[0].step = 0x4;
    devObjPtr->tablesInfo.ieeeRsrvMcCpuIndex.paramInfo[0].divider = 16;


    devObjPtr->tablesInfo.portVlanQosConfig.commonInfo.baseAddress = 0;/* this table is obsolete */
    devObjPtr->tablesInfo.ieeeTblSelect.commonInfo.baseAddress = 0;/* this table is obsolete */

    if(GET_FDB_DEVICE_FROM_MY_DEVICE_MAC(devObjPtr) != devObjPtr)
    {
        devObjPtr->tablesInfo.fdb.commonInfo.baseAddress    = GET_FDB_DEVICE_FROM_MY_DEVICE_MAC(devObjPtr)->tablesInfo.fdb.commonInfo.baseAddress;
        devObjPtr->tablesInfo.fdb.paramInfo[0].step         = GET_FDB_DEVICE_FROM_MY_DEVICE_MAC(devObjPtr)->tablesInfo.fdb.paramInfo[0].step;
        devObjPtr->tablesInfo.fdb.paramInfo[0].divider      = GET_FDB_DEVICE_FROM_MY_DEVICE_MAC(devObjPtr)->tablesInfo.fdb.paramInfo[0].divider;
    }
    else
    {
        baseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_FDB_TABLE_0);
        devObjPtr->tablesInfo.fdb.commonInfo.baseAddress = baseAddress;
    }

    /* start EQ unit */
    baseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_EQ);

    devObjPtr->tablesInfo.tcpUdpDstPortRangeCpuCode.commonInfo.baseAddress = baseAddress + 0x00007000;
    devObjPtr->tablesInfo.tcpUdpDstPortRangeCpuCode.paramInfo[0].step = 0x4;      /*entry*/
    devObjPtr->tablesInfo.tcpUdpDstPortRangeCpuCode.paramInfo[1].step = 0x800;    /*word*/

    devObjPtr->tablesInfo.ipProtCpuCode.commonInfo.baseAddress = baseAddress + 0x00007C00;
    devObjPtr->tablesInfo.ipProtCpuCode.paramInfo[0].step = 0x4;
    devObjPtr->tablesInfo.ipProtCpuCode.paramInfo[0].divider = 2;

    devObjPtr->tablesInfo.trunkNumOfMembers.commonInfo.baseAddress = 0;/* not used*/
    devObjPtr->tablesInfo.trunkMembers.commonInfo.baseAddress = 0;/* not used*/

    devObjPtr->tablesInfo.eqIngressEPort.paramInfo[0].modulo = (8*_1K);/*modulo done before divider*/
    devObjPtr->tablesInfo.eqIngressEPort.paramInfo[0].divider = 4;/* 4 ePorts in entry */

    /* the table is not of type 'write' after 'whole' words :
       word 0 - RW
       word 1 - RO
       word 2 - RW */
    devObjPtr->tablesInfo.ingrStc.commonInfo.baseAddress = baseAddress + 0x00040000;
    devObjPtr->tablesInfo.ingrStc.paramInfo[0].step = 0x10;
    devObjPtr->tablesInfo.ingrStc.paramInfo[0].divider = 0;
    devObjPtr->tablesInfo.ingrStc.paramInfo[0].modulo = 0;

    devObjPtr->tablesInfo.eqPhysicalPortIngressMirrorIndexTable.paramInfo[0].divider = 8;/* 8 physical Ports in entry */

    devObjPtr->tablesInfo.logicalTrgMappingTable.commonInfo.baseAddress = 0;/* not used*/

    devObjPtr->tablesInfo.oamProtectionLocStatusTable.paramInfo[0].divider = 32;/* 32 flows in entry */
    devObjPtr->tablesInfo.oamTxProtectionLocStatusTable.paramInfo[0].divider = 32;/* 32 flows in entry */

    devObjPtr->tablesInfo.txProtectionSwitchingTable.paramInfo[0].divider = 32; /* 32 flows in entry */

    devObjPtr->tablesInfo.ePortToLocMappingTable.paramInfo[0].divider = 2; /* 2 eports in entry */
    devObjPtr->tablesInfo.ePortToLocMappingTable.paramInfo[0].modulo  =
                    (1 << devObjPtr->flexFieldNumBitsSupport.ePort); /*modulo done before divider*/


    /* end EQ unit */

    /* start HA unit */
    devObjPtr->tablesInfo.haUp0PortKeepVlan1.commonInfo.baseAddress = 0;/* not exist memory */

    devObjPtr->tablesInfo.arp.paramInfo[0].step                 = 0x40;
    devObjPtr->tablesInfo.arp.paramInfo[0].divider              = 8;/* 8 sub entries in each line */

    devObjPtr->tablesInfo.tunnelStart.commonInfo.baseAddress    = devObjPtr->tablesInfo.arp.commonInfo.baseAddress;
    devObjPtr->tablesInfo.tunnelStart.paramInfo[0].step         = devObjPtr->tablesInfo.arp.paramInfo[0].step;
    devObjPtr->tablesInfo.tunnelStart.paramInfo[0].divider      = 2;/* 2 sub entries in each line */

    devObjPtr->tablesInfo.haPtpDomain.paramInfo[0].step = 0x8;       /* port (local target physical port) 0..255 */
    devObjPtr->tablesInfo.haPtpDomain.paramInfo[1].step = devObjPtr->limitedResources.phyPort*0x8; /* domain 0..4 */

    /* end HA unit */


    /* start ERMRK unit */
    devObjPtr->tablesInfo.ptpLocalActionTable.paramInfo[0].step = 4;        /* PTP Domain 0..4 */
    devObjPtr->tablesInfo.ptpLocalActionTable.paramInfo[1].step = 8*4;      /* PTP Message Type 0..15 */
    devObjPtr->tablesInfo.ptpLocalActionTable.paramInfo[2].step = 128*4;  /* port (local target physical port) 0..255 */

    /* end ERMRK unit */

    /* start MLL unit */
    devObjPtr->tablesInfo.mllOutInterfaceConfig.commonInfo.baseAddress = 0;/* use registers and not table */
    devObjPtr->tablesInfo.mllOutInterfaceCounter.commonInfo.baseAddress = 0;/* use registers and not table */
    devObjPtr->memUnitBaseAddrInfo.mll = SMAIN_NOT_VALID_CNS;

    /* end MLL unit*/

    /* start IPCL unit */

    /*obsolete PCL action table --> need to use globalActionTable*/
    devObjPtr->tablesInfo.pclAction.commonInfo.baseAddress = 0;/*obsolete*/

    /* end IPCL unit */

    /* CNC unit (2 CNC units) */
    devObjPtr->tablesInfo.cncMemory.commonInfo.multiInstanceInfo.numBaseAddresses = 2;
    devObjPtr->tablesInfo.cncMemory.commonInfo.multiInstanceInfo.multiUnitsBaseAddressPtr = &devObjPtr->memUnitBaseAddrInfo.CNC[0];
    devObjPtr->tablesInfo.cncMemory.paramInfo[0].step = 8*_1K;/* block --> 1K entries of 2 words (8 bytes)*/
    devObjPtr->tablesInfo.cncMemory.paramInfo[1].step = 8;/*index (2 counters per entry --> 8 bytes)*/

    /* obsolete old TCC units of PCL,TTI of Lion2 */
    devObjPtr->tablesInfo.pclActionTcamData.commonInfo.baseAddress = 0;
    devObjPtr->tablesInfo.tcamBistArea.commonInfo.baseAddress = 0;
    devObjPtr->tablesInfo.tcamArrayCompareEn.commonInfo.baseAddress = 0;
    devObjPtr->tablesInfo.routerTcam.commonInfo.baseAddress = 0;
    devObjPtr->tablesInfo.ttiAction.commonInfo.baseAddress = 0;
    devObjPtr->tablesInfo.pclTcam.commonInfo.baseAddress = 0;
    devObjPtr->tablesInfo.pclTcamMask.commonInfo.baseAddress = 0;

    /* non exists L2i table */
    devObjPtr->tablesInfo.vrfId.commonInfo.baseAddress = 0;

    /*start IPvX*/
    devObjPtr->tablesInfo.ipvxIngressEPort.paramInfo[0].divider = 16;/*16 eports per entry */
    /*end IPvX*/

    /*  the design not modulo on this table !
        devObjPtr->tablesInfo.l2MllLtt.paramInfo[0].modulo = devObjPtr->limitedResources.l2LttMll;
    */

    /* init memories*/
    smemLion3MemoriesInit(devObjPtr);
}

/*******************************************************************************
*   smemLion2InitFuncArray
*
* DESCRIPTION:
*       Init specific functions array.
*
* INPUTS:
*       commonDevMemInfoPtr   - pointer to common device memory object.
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
void smemLion3InitFuncArray
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_CHT_DEV_COMMON_MEM_INFO  * commonDevMemInfoPtr
)
{
    smemLion2InitFuncArray(devObjPtr,commonDevMemInfoPtr);
}

/*******************************************************************************
*   smemLion3InitPerUnitActiveMem
*
* DESCRIPTION:
*       bind per unit active memory
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
void smemLion3InitPerUnitActiveMem
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = devObjPtr->deviceMemory;
    GT_U32  ii;

    /* by default state that all units bound to 'empty' active memory table */
    /* meaning that each unit must declare explicitly what is it's active
       memory table */
    for (ii = 0 ; ii < SMEM_CHT_NUM_UNITS_MAX_CNS; ii++)
    {
        devMemInfoPtr->unitMemArr[ii].unitActiveMemPtr = smemEmptyActiveMemoryArr;
    }
}

/*******************************************************************************
*   initBaseAddressOfUnitActiveMemory
*
* DESCRIPTION:
*       set base address to all entries of the active memory of the unit.
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
static void initBaseAddressOfUnitActiveMemory(
    SMEM_UNIT_CHUNKS_STC        *unitChunksPtr
)
{
    GT_U32              ii;
    GT_U32              baseAddr = unitChunksPtr->chunkIndex << SMEM_CHT_UNIT_INDEX_FIRST_BIT_CNS;
    SMEM_ACTIVE_MEM_ENTRY_STC   *unitActiveMemPtr;

    unitActiveMemPtr = unitChunksPtr->unitActiveMemPtr;

    /* first of all need to check if this active memory was already initialized.
        by other unit (huge unit support)
        or
        by other device (multi devices)
        or
        before system reset
    */
    for (ii = 0; unitActiveMemPtr[ii].address != END_OF_TABLE; ii++)
    {
        /* do nothing .. we look for the index of the last entry */
    }

    if(ii == 0)
    {
        /* one dummy entry ... no need to set indications */
        return;
    }

    /* we found the last entry -- the 'termination' entry */
    if(unitActiveMemPtr[ii].mask == SMEM_FULL_MASK_CNS)
    {
        /* indication that DB was already added base address */
        unitActiveMemPtr[ii].mask = baseAddr;
    }
    else
    {
        /* DB was already added the base address ... must not do it again */
        return;
    }

    /* add the address */
    for (ii = 0; unitActiveMemPtr[ii].address != END_OF_TABLE; ii++)
    {
        /* update the address */
        unitActiveMemPtr[ii].address += baseAddr;
    }
}


/*******************************************************************************
*   smemLion3BindPerUnitActiveMemToSpecFunTbl
*
* DESCRIPTION:
*       bind per unit active memory to 'specFunTbl'
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       commonDevMemInfoPtr   - pointer to common device memory object.
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
void smemLion3BindPerUnitActiveMemToSpecFunTbl
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_CHT_DEV_COMMON_MEM_INFO  * commonDevMemInfoPtr
)
{
    GT_U32              jj;
    SMEM_UNIT_CHUNKS_STC        *unitChunksPtr;

    /* bind the units to the specific unit chunk with the generic function */
    for (jj = 0; jj < SMEM_CHT_NUM_UNITS_MAX_CNS ; jj++)
    {
        /* get the unit info from the 'specParam' */
        unitChunksPtr = (void*)(GT_UINTPTR)commonDevMemInfoPtr->specFunTbl[jj].specParam;
        if(unitChunksPtr == NULL)
        {
            continue;
        }

        commonDevMemInfoPtr->specFunTbl[jj].unitActiveMemPtr =
            unitChunksPtr->unitActiveMemPtr;

        initBaseAddressOfUnitActiveMemory(unitChunksPtr);
    }
}


/*******************************************************************************
*   smemLion3BindNonPpPerUnitActiveMem
*
* DESCRIPTION:
*       bind 'non PP' per unit active memory to the unit.
*       the memory spaces are : 'PEX' and 'DFX' (and 'MBUS')
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       devMemInfoPtr   - pointer to generic device memory object.
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
void smemLion3BindNonPpPerUnitActiveMem
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_CHT_GENERIC_DEV_MEM_INFO  * devMemInfoPtr
)
{
    SMEM_UNIT_CHUNKS_STC    *currUnitChunkPtr;

    /* bind active memories of the PEX memory space*/
    currUnitChunkPtr = &devMemInfoPtr->PEX_UnitMem;
    bindUnitPexActiveMem(devObjPtr,currUnitChunkPtr);

    /* bind active memories of the DFX memory space */
    currUnitChunkPtr = &devMemInfoPtr->common.pciExtMemArr[SMEM_UNIT_PCI_BUS_DFX_E].unitMem;
    bindUnitDfxActiveMem(devObjPtr,currUnitChunkPtr);

}


/*******************************************************************************
*   smemLion3Init
*
* DESCRIPTION:
*       Init memory module for the Lion2 device.
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
void smemLion3Init
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_CHT_DEV_COMMON_MEM_INFO * commonDevMemInfoPtr = NULL;
    GT_U32  ii;
    SMEM_CHT_GENERIC_DEV_MEM_INFO  * devMemInfoPtr;

    if(devObjPtr->shellDevice == GT_TRUE)
    {
        GT_U32  ii;

        /* this is the port group shared device */
        /* so we need to fill info about it's port group devices */
        for( ii = 0 ; ii < devObjPtr->numOfCoreDevs ;ii++)
        {
            /* each of the port group devices start his ports at multiple of 16
               (each port group has 12 ports) */
            devObjPtr->coreDevInfoPtr[ii].startPortNum = 16*ii;
        }

        return;
    }

    if(devObjPtr->activeMemPtr == NULL)
    {
        SMEM_CHT_IS_SIP5_GET(devObjPtr) = 1;

        devObjPtr->devMemUnitNameAndIndexPtr = lion3UnitNameAndIndexArr;
        devObjPtr->genericUsedUnitsAddressesArray = lion3UsedUnitsAddressesArray;
        devObjPtr->genericNumUnitsAddresses = SMEM_LION3_UNIT_LAST_E;
        devObjPtr->specialUnitsBaseAddr.timestampBaseAddr[0] = timestampBaseAddr_lion3[0];
        devObjPtr->specialUnitsBaseAddr.timestampBaseAddr[1] = timestampBaseAddr_lion3[1];

        /* set the pointer to the active memory */
        devObjPtr->activeMemPtr = smemLion3PortGroupActiveTable;

        /* lion3 but not bobcat2 */
        devObjPtr->fdbBankMoveMessageSupported = 1;

        /* Lion3 supports 33 ports in TXQ_DQ per core , BUT currently we need to support 16 like in Lion2 */
        devObjPtr->txqDqNumPorts = 16;/* 33 */SIM_TBD_BOOKMARK
        /* Lion3 supports cpu port 32 TXQ_DQ per core , BUT currently we need to support 63 like in Lion2 */
        devObjPtr->dmaNumOfCpuPort = 63;/*32*/SIM_TBD_BOOKMARK

        devObjPtr->portsArr[SMEM_LION_XLG_PORT_12_NUM_CNS].state = SKERNEL_PORT_STATE_XLG_40G_E;
        devObjPtr->portsArr[SMEM_LION_XLG_PORT_14_NUM_CNS].state = SKERNEL_PORT_STATE_XLG_40G_E;

        if (devObjPtr->registersDefaultsPtr)
        {
            /* attach the link list */
            devObjPtr->registersDefaultsPtr = &element0_RegistersDefaults;
        }
    }

    if(devObjPtr->registersDfxDefaultsPtr == NULL)
    {
        devObjPtr->registersDfxDefaultsPtr = &element0_DfxDefault_RegistersDefaults;
    }

    if(devObjPtr->registersPexDefaultsPtr == NULL)
    {
        devObjPtr->registersPexDefaultsPtr = &element0_PexDefault_RegistersDefaults;
    }


    devObjPtr->specialUnitsBaseAddr.UNIT_TXQ_LL     = SMAIN_NOT_VALID_CNS;/*the Lion3 not need this info for 'shared units' */
    devObjPtr->specialUnitsBaseAddr.UNIT_TXQ_QUEUE  = SMAIN_NOT_VALID_CNS;/*the Lion3 not need this info for 'shared units' */
    devObjPtr->specialUnitsBaseAddr.UNIT_TXQ_DIST   = SMAIN_NOT_VALID_CNS;/*not valid*/
    devObjPtr->specialUnitsBaseAddr.UNIT_BMA        = UNIT_BASE_ADDR_GET_ALLOW_NON_EXIST_UNIT_MAC(devObjPtr,UNIT_BMA);
    devObjPtr->specialUnitsBaseAddr.UNIT_CPFC       = SMAIN_NOT_VALID_CNS;/*the Lion3 not need this info for 'shared units' */
    devObjPtr->specialUnitsBaseAddr.UNIT_DFX_SERVER = UNIT_BASE_ADDR_GET_ALLOW_NON_EXIST_UNIT_MAC(devObjPtr,UNIT_DFX_SERVER);


    devMemInfoPtr = smemDeviceObjMemoryAlloc(devObjPtr,1, sizeof(SMEM_CHT_GENERIC_DEV_MEM_INFO));
    if (devMemInfoPtr == 0)
    {
        skernelFatalError("smemLion3Init: allocation error\n");
    }

    devObjPtr->deviceMemory = devMemInfoPtr;
    commonDevMemInfoPtr= &devMemInfoPtr->common;
    commonDevMemInfoPtr->isPartOfGeneric = 1;

    devObjPtr->supportRegistersDb = 1;
    devObjPtr->designatedPortVersion = 1;
    devObjPtr->numOfTxqUnits =
        devObjPtr->portGroupSharedDevObjPtr ?
            ((devObjPtr->portGroupSharedDevObjPtr->numOfCoreDevs - 1) / 4) + 1 :/* lion3 --> 2 TXQs (TXQ per 4 cores -- like Lion2)*/
            1;

    SET_IF_ZERO_MAC(devObjPtr->txqNumPorts,72);/*the TXQ of Bc2 and Lion3 support 72 ports (per hemisphere)*/

    /* lion3 */
    /* support SIP5 */
    devObjPtr->unitEArchEnable.tti = 1;/* the TTI unit hold no 'enable bit' */
    devObjPtr->unitEArchEnable.bridge = 1;/* the bridge unit hold no 'enable bit' */
    devObjPtr->unitEArchEnable.eq = 1;/* the EQ unit hold no 'enable bit' */
    devObjPtr->unitEArchEnable.txq = 1;/* the TQX unit hold no 'enable bit' */
    devObjPtr->unitEArchEnable.ha = 1;/* the HA unit hold no 'enable bit' */
    devObjPtr->unitEArchEnable.ipvx = 1;/* the IPVx unit hold no 'enable bit' */
    devObjPtr->unitEArchEnable.pcl[SMAIN_DIRECTION_INGRESS_E] = 1;/* the IPCL unit hold no 'enable bit' */
    devObjPtr->unitEArchEnable.pcl[SMAIN_DIRECTION_EGRESS_E] = 1;/* the EPCL unit hold no 'enable bit' */

    devObjPtr->numWordsAlignmentInAuMsg = 8;
    devObjPtr->numOfWordsInAuMsg = 6;

    devObjPtr->defaultEPortNumEntries = LION3_NUM_DEFAULT_E_PORTS_CNS;

    devObjPtr->fdbNumOfBanks = 16;
    devObjPtr->devMemUnitMemoryGetPtr = smemLion3SubUnitMemoryGet_fromSkernel;

    devObjPtr->supportEArch = 1;
    devObjPtr->l2MllVersionSupport = 2;
    devObjPtr->supportTrill = 1;
    devObjPtr->supportCapwap = SKERNEL_CAPWAP_NOT_SUPPORTED_E;/* not designed/implemented yet */
    devObjPtr->supportFid = 1;
    devObjPtr->supportL2Ecmp = 1;
    devObjPtr->ipvxSupport.ecmpIndexFormula = SKERNEL_ECMP_ROUTE_INDEX_FORMULA_TYPE_2_E;

    devObjPtr->iPclKeyFormatVersion = 2;
    devObjPtr->ePclKeyFormatVersion = 2;
    devObjPtr->supportTunnelstartIpTotalLengthAddValue = 1;/* SIP5 (HA unit) support similar to xCat A3 ! */

/*  no such support/registers in Aslan-A so don't add to Aslan-B yet ...
    devObjPtr->supportForwardUnknowMacControlFrames = 1;
    devObjPtr->supportForwardPfcFrames = 1;
    devObjPtr->support1024to1518MibCounter = 1;
*/
    SET_IF_ZERO_MAC(devObjPtr->oamNumEntries,2048);
    devObjPtr->oamSupport.psuSupportPacketModification = 1;
    devObjPtr->oamSupport.plrSupportOamLmCapture = 1;
    devObjPtr->oamSupport.keepAliveSupport = 1;
    devObjPtr->oamSupport.oamSupport = 1;
    {
        GT_U32 stage;

        for (stage = 0; stage < 2; stage++)
        {
            for (ii = 0; ii < 8; ii++)
            {
                devObjPtr->oamSupport.keepAlivePeriodCounter[stage].keepAlivePeriodCounterTimer[ii].l[0] =
                devObjPtr->oamSupport.keepAlivePeriodCounter[stage].keepAlivePeriodCounterTimer[ii].l[1] = 0;
                devObjPtr->oamSupport.keepAlivePeriodCounter[stage].keepAlivePeriodCounterValue[ii].l[0] =
                devObjPtr->oamSupport.keepAlivePeriodCounter[stage].keepAlivePeriodCounterValue[ii].l[1] = 0;
                devObjPtr->oamSupport.keepAlivePeriodCounter[stage].keepAliveAgingRequestBit[ii] = 0;
            }
        }
    }

    /* bind the router TCAM info */
    devObjPtr->routeTcamInfo =  lion3PortGroupRoutTcamInfo;

    devObjPtr->supportMultiPortGroupFdbLookUpMode = 0;/* multi core lookup not supported in SIP5*/
    devObjPtr->notSupportIngressPortRateLimit = 0;/*the device supports rate limit per physical port*/
    devObjPtr->supportGemUserId = 0;/* not support the functionality*/

    /* set the 2 interrupts bound on the same bit in 'global'  temp code to use the same bit (as in legacy device) */
    devObjPtr->globalInterruptCauseRegister.cnc[1] = devObjPtr->globalInterruptCauseRegister.cnc[0];
    devObjPtr->cncNumOfUnits = 2;
    SET_IF_ZERO_MAC(devObjPtr->cncClientSupportBitmap,SNET_LION3_CNC_CLIENTS_BMP_ALL_CNS);
    devObjPtr->supportRemoveVlanTag1WhenEmpty = 0;/* the logic is not 'global' in the 'HA' , but in the EGF + 'per eport' config */
    devObjPtr->pclSupport.ePclSupportVidUpTag1 = 0;/*Due to the SIP5.0 full support of lookup using Tag0 and/or Tag1 fields,
           the existing (SIP 4) global configuration <Use Tag1 in EPCL> is obsolete*/

    devObjPtr->trafficGeneratorSupport.tgSupport = 1;
    devObjPtr->memUnitBaseAddrInfo.policer[0] = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_IPLR);
    devObjPtr->memUnitBaseAddrInfo.policer[1] = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_IPLR1);
    devObjPtr->memUnitBaseAddrInfo.policer[2] = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_EPLR);
    /* Timestamp base addresses */
    devObjPtr->memUnitBaseAddrInfo.gts[0] = devObjPtr->specialUnitsBaseAddr.timestampBaseAddr[0];
    devObjPtr->memUnitBaseAddrInfo.gts[1] = devObjPtr->specialUnitsBaseAddr.timestampBaseAddr[1];
    /* OAM base addresses */
    devObjPtr->memUnitBaseAddrInfo.oam[0] = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_IOAM);
    devObjPtr->memUnitBaseAddrInfo.oam[1] = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_EOAM);
    /* CNC base addresses */
    devObjPtr->memUnitBaseAddrInfo.CNC[0] = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_CNC);
    devObjPtr->memUnitBaseAddrInfo.CNC[1] = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_CNC_1);

    devObjPtr->ttiTrillAdjacencyTcanInfo = lion3TtiTrillAdjacencyTcanInfo;
    /* bind the PCL TCAM info */
    devObjPtr->pclTcamInfoPtr = &lion2PortGroupGlobalPclData;
    devObjPtr->pclTcamMaxNumEntries = devObjPtr->pclTcamInfoPtr->bankEntriesNum * 4;

    devObjPtr->fdbNumEntries = SMEM_MAC_TABLE_SIZE_32KB;
    devObjPtr->supportFdbNewNaToCpuMsgFormat = 1;

    devObjPtr->lpmRam.numOfLpmRams = 20;
    devObjPtr->lpmRam.numOfEntriesBetweenRams = 16*1024;
    SET_IF_ZERO_MAC(devObjPtr->lpmRam.perRamNumEntries,16*1024);

    devObjPtr->supportEqEgressMonitoringNumPorts = 128;
    devObjPtr->deviceMapTableUseDstPort = 1;
    devObjPtr->supportVlanEntryVidxFloodMode = 1;
    devObjPtr->errata.routerGlueResetPartsOfDescriptor = 1;
    devObjPtr->errata.ttCopyToCpuWithAdditionalTag = 1;
    devObjPtr->errata.unregIpmEvidxValue0 = 1;
    devObjPtr->errata.wrongEpclOamRBitReg = 1;

    devObjPtr->isXcatA0Features = 0;/*state that features are NOT like xcat-A0*/
    devObjPtr->supportTunnelStartEthOverIpv4 = 1;
    devObjPtr->errata.lttRedirectIndexError = 0;
    devObjPtr->errata.ieeeReservedMcConfigRegRead = 1;
    devObjPtr->errata.ipfixWrapArroundFreezeMode = 1;
    devObjPtr->errata.srcTrunkPortIsolationAsSrcPort = 1;
    devObjPtr->errata.llcNonSnapWithDoRouterHaCurruptL2Header = 1;
    devObjPtr->errata.fdbAgingDaemonVidxEntries = 1;
    devObjPtr->errata.cncHaIgnoreCrc4BytesInL2ByteCountMode = 1;
    devObjPtr->errata.ttiActionNotSetModifyUpDscpWhenQosPrecedenceHard = 1;
    devObjPtr->errata.ttiActionMayOverrideHardQosPrecedence = 1;
    devObjPtr->errata.l2iFromCpuSubjectToL2iFiltersBridgeBypassModeIsBypassForwardingDecisionOnly = 1;
    devObjPtr->errata.haBuildWrongTagsWhenNumOfTagsToPopIs2 = 1;
    devObjPtr->errata.iplrPolicerBillingForPerPhysicalPortMeteringDoesNotWorksInFlowMode = 1;
    devObjPtr->errata.ipLpmActivityStateDoesNotWorks = 1;
    devObjPtr->errata.fastStackFailover_Drop_on_source = 1;
    devObjPtr->errata.eqToCpuForRxAnalyzerSrcBasedMirrorTrgInfoInsteadOfSrcInfo = 1;
    devObjPtr->errata.ttiWrongPparsingWhenETagSecondWordEqualToEthertype = 1;
    devObjPtr->errata.ttiTtAndPopTagNeedToBeSupportedConcurrently = 1;
    devObjPtr->errata.fdbRouteUcDeleteByMsg = 1;
    devObjPtr->errata.tunnelStartQosRemarkUsePassenger = 1;
    devObjPtr->errata.fdbNaMsgVid1Assign = 1;


    devObjPtr->cncBlocksNum = 16;
    devObjPtr->cncBlockMaxRangeIndex = _1K;
    devObjPtr->supportPortIsolation = 1;
    devObjPtr->supportOamPduTrap = 1;
    devObjPtr->supportPortOamLoopBack = 1;
    devObjPtr->policerEngineNum = 2;
    devObjPtr->supportMultiAnalyzerMirroring = 1;
    devObjPtr->supportCascadeTrunkHashDstPort = 1;
    devObjPtr->supportMultiPortGroupTTILookUpMode = 1;
    devObjPtr->supportTxQGlobalPorts = 1;
    devObjPtr->supportCrcTrunkHashMode = 1;
    devObjPtr->supportMacHeaderModification = 1;
    devObjPtr->supportHa64Ports = 1;
    devObjPtr->txqRevision = 1;
    devObjPtr->supportXgMacMibCountersControllOnAllPorts = 1;

    devObjPtr->supportLogicalTargetMapping = 0;/* not supporting the Logical Target Mapping*/
    devObjPtr->ipFixTimeStampClockDiff[0] =
    devObjPtr->ipFixTimeStampClockDiff[1] =
    devObjPtr->ipFixTimeStampClockDiff[2] = 0;
    devObjPtr->supportConfigurableTtiPclId = 1;
    devObjPtr->supportVlanEntryVidxFloodMode = 1;
    devObjPtr->supportForceNewDsaToCpu = 1;
    devObjPtr->supportCutThrough = 1;
    devObjPtr->supportKeepVlan1 = 1;
    devObjPtr->supportResourceHistogram = 1;

    devObjPtr->ipvxSupport.lttMaxNumberOfPaths = SKERNEL_LTT_MAX_NUMBER_OF_PATHS_64_E;
    devObjPtr->ipvxSupport.qosIndexFormula = SKERNEL_QOS_ROUTE_INDEX_FORMULA_TYPE_1_E;
    devObjPtr->ipvxSupport.supportPerVlanURpfMode = 1;
    devObjPtr->ipvxSupport.supportPerPortSipSaCheckEnable = 1;

    devObjPtr->supportMacSaAssignModePerPort = 1;
    devObjPtr->supportMaskAuFuMessageToCpuOnNonLocal = 1;

    devObjPtr->pclSupport.iPclSupport5Ude = 1;
    devObjPtr->pclSupport.iPclSupportTrunkHash = 1;
    devObjPtr->pclSupport.ipclSupportSeparatedIConfigTables = 1;
    devObjPtr->pclSupport.pclSupportPortList = 1;
    devObjPtr->pclTcamFormatVersion = 1;

    devObjPtr->supportDevMapTableOnOwnDev   = 1;
    devObjPtr->supportPtp = 1;

    if(SMEM_CHT_IS_SIP5_15_GET(devObjPtr))
    {
        /* Meter memories became shared between IPLR0 and IPLR1 starting from SIP 5.15 (BobK)
           Direct access done through IPLR0. supportPolicerMemoryControl became wrong for SIP 5.15 */
        devObjPtr->policerSupport.supportPolicerMemoryControl = 0;
    }
    else
    {
        devObjPtr->policerSupport.supportPolicerMemoryControl = 1;
    }
    devObjPtr->policerSupport.supportPolicerEnableCountingTriggerByPort   = 1;
    devObjPtr->policerSupport.supportPortModeAddressSelect = 1;
    devObjPtr->policerSupport.supportEplrPerPort = 1;
    devObjPtr->policerSupport.supportIpfixTimeStamp = 1;
    devObjPtr->policerSupport.supportCountingEntryFormatSelect = 1;

    devObjPtr->policerSupport.numIplrMemoriesSupported = 1;
    if(devObjPtr->policerSupport.iplrTableSize == 0)
    {
        devObjPtr->policerSupport.iplrTableSize   = (8 * _1K);
        devObjPtr->policerSupport.iplrMemoriesSize[0] = (4 * _1K);
        devObjPtr->policerSupport.iplrMemoriesSize[1] = 0xF00;
        devObjPtr->policerSupport.iplrMemoriesSize[2] =
            devObjPtr->policerSupport.iplrTableSize -
            (devObjPtr->policerSupport.iplrMemoriesSize[0] + devObjPtr->policerSupport.iplrMemoriesSize[1]) ;

        devObjPtr->policerSupport.iplr0TableSize =
                        devObjPtr->policerSupport.iplrMemoriesSize[1] +
                        devObjPtr->policerSupport.iplrMemoriesSize[2] ;
        devObjPtr->policerSupport.iplr1TableSize =
                    devObjPtr->policerSupport.iplrTableSize -
                    devObjPtr->policerSupport.iplr0TableSize;
    }

    devObjPtr->supportForwardFcPackets = 1;

    devObjPtr->isMsmMibOnAllPorts = 1;

    SMEM_CHT_IS_DFX_SERVER(devObjPtr) = GT_TRUE;

    /* The device supports split designated trunk tables */
    devObjPtr->supportSplitDesignatedTrunkTable = 1;

    SET_IF_ZERO_MAC(devObjPtr->flexFieldNumBitsSupport.phyPort , LION3_NUM_PHY_PORTS_BITS_CNS);
    SET_IF_ZERO_MAC(devObjPtr->flexFieldNumBitsSupport.trunkId , LION3_NUM_TRUNKS_BITS_CNS);
    SET_IF_ZERO_MAC(devObjPtr->flexFieldNumBitsSupport.ePort , LION3_NUM_E_PORTS_BITS_CNS);
    SET_IF_ZERO_MAC(devObjPtr->flexFieldNumBitsSupport.eVid , LION3_NUM_E_VLANS_BITS_CNS);
    SET_IF_ZERO_MAC(devObjPtr->flexFieldNumBitsSupport.eVidx , LION3_NUM_E_VIDXS_BITS_CNS);
    SET_IF_ZERO_MAC(devObjPtr->flexFieldNumBitsSupport.hwDevNum , LION3_NUM_DEVICES_BITS_CNS);
    SET_IF_ZERO_MAC(devObjPtr->flexFieldNumBitsSupport.sstId , LION3_NUM_SRC_ID_BITS_CNS);
    SET_IF_ZERO_MAC(devObjPtr->flexFieldNumBitsSupport.tunnelstartPtr , LION3_NUM_TS_PTR_BITS_CNS);
    SET_IF_ZERO_MAC(devObjPtr->flexFieldNumBitsSupport.arpPtr ,  4 * devObjPtr->flexFieldNumBitsSupport.tunnelstartPtr);
    SET_IF_ZERO_MAC(devObjPtr->flexFieldNumBitsSupport.fid , LION3_NUM_FID_BITS_CNS);
    SET_IF_ZERO_MAC(devObjPtr->flexFieldNumBitsSupport.stgId, LION3_NUM_STG_BITS_CNS);

    SET_IF_ZERO_MAC(devObjPtr->limitedResources.eVid,1<<devObjPtr->flexFieldNumBitsSupport.eVid);
    SET_IF_ZERO_MAC(devObjPtr->limitedResources.ePort,1<<devObjPtr->flexFieldNumBitsSupport.ePort);
    SET_IF_ZERO_MAC(devObjPtr->limitedResources.phyPort,1<<devObjPtr->flexFieldNumBitsSupport.phyPort);
    SET_IF_ZERO_MAC(devObjPtr->limitedResources.l2Ecmp,8*1024);
    SET_IF_ZERO_MAC(devObjPtr->limitedResources.nextHop,24*1024);

    SET_IF_ZERO_MAC(devObjPtr->limitedResources.mllPairs,16*1024);
    SET_IF_ZERO_MAC(devObjPtr->limitedResources.l2LttMll,32*1024);
    SET_IF_ZERO_MAC(devObjPtr->limitedResources.l3LttMll,12*1024);/*12k*/


    /* must be called before the units start to bind active memories */
    if(devObjPtr->supportActiveMemPerUnit)
    {
        smemLion3InitPerUnitActiveMem(devObjPtr);
    }

    /* Update active memory table data only once */
    if (devObjPtr->portGroupId == 0)
    {
        for(ii = 0 ; devObjPtr->activeMemPtr[ii].address != END_OF_TABLE ; ii++)
        {
            if(devObjPtr->activeMemPtr[ii].mask ==
                CNC_ACTIVE_MEMORY_PLACE_HOLDER_MASK_CNS)
            {
                CNC_ACTIVE_MEM_ENTRY_INIT_MAC(devObjPtr,&devObjPtr->activeMemPtr[ii]);
                ii += 1;/* the jump of 1 here + the one of the loop will
                           jump to next 2 CNC units */
            }
            else
            if(devObjPtr->activeMemPtr[ii].mask ==
                POLICER_ACTIVE_MEMORY_PLACE_HOLDER_MASK_CNS)
            {
                GT_U32  tmpMask = SMEM_FULL_MASK_CNS;

                if(devObjPtr->activeMemPtr[ii + 0].address == POLICER_MANAGEMENT_COUNTER_ADDR_CNS)
                {
                    /* management counters */
                    /* 0x40 between sets , 0x10 between counters */
                    /* 0x00 , 0x10 , 0x20 , 0x30 */
                    /* 0x40 , 0x50 , 0x60 , 0x70 */
                    /* 0x80 , 0x90 , 0xa0 , 0xb0 */
                    tmpMask = POLICER_MANAGEMENT_COUNTER_MASK_CNS;
                }

                devObjPtr->activeMemPtr[ii + 0].address += devObjPtr->memUnitBaseAddrInfo.policer[0];
                devObjPtr->activeMemPtr[ii + 0].mask = tmpMask;

                devObjPtr->activeMemPtr[ii + 1].address += devObjPtr->memUnitBaseAddrInfo.policer[1];
                devObjPtr->activeMemPtr[ii + 1].mask = tmpMask;

                devObjPtr->activeMemPtr[ii + 2].address += devObjPtr->memUnitBaseAddrInfo.policer[2];
                devObjPtr->activeMemPtr[ii + 2].mask = tmpMask;

                ii += 2;/* the jump of 2 here + the one of the loop will
                           jump to next 3 policer if exists */
            }
            else
            if(devObjPtr->activeMemPtr[ii].mask ==
                GTS_ACTIVE_MEMORY_PLACE_HOLDER_MASK_CNS)
            {
                /* init the GTS entry */
                GENERIC_ACTIVE_MEM_ENTRY_PLACE_HOLDER_INIT_MAC(devObjPtr,gts,&devObjPtr->activeMemPtr[ii]);
            }
            else
            if(devObjPtr->activeMemPtr[ii].mask ==
                OAM_ACTIVE_MEMORY_PLACE_HOLDER_MASK_CNS)
            {
                /* init the OAM entry */
                GENERIC_ACTIVE_MEM_ENTRY_PLACE_HOLDER_INIT_MAC(devObjPtr,oam,&devObjPtr->activeMemPtr[ii]);

                if(devObjPtr->activeMemPtr[ii].readFun == smemLion3ActiveReadExceptionSummaryBitmap)
                {
                    devObjPtr->activeMemPtr[ii].mask = 0xFFFFFF00;
                }
            }
        }
    }
    commonDevMemInfoPtr->pciUnitBaseAddrMask = 0x000f0000;
    commonDevMemInfoPtr->pciUnitBaseAddr = 0x00070000;

    commonDevMemInfoPtr->dfxMemBaseAddr = 0x0;
    commonDevMemInfoPtr->dfxMemBaseAddrMask = 0x0;

    devObjPtr->devIsOwnerMemFunPtr = smemLion2IsDeviceMemoryOwner;
    devObjPtr->devFindMemFunPtr = (void *)smemLion2FindMem;


    /* init specific functions array */
    smemLion3InitFuncArray(devObjPtr,commonDevMemInfoPtr);

    smemLion3AllocSpecMemory(devObjPtr,commonDevMemInfoPtr);

    smemLion3RegsInfoSet(devObjPtr);

    smemLion3TableInfoSet(devObjPtr);

    if(devObjPtr->devMemSpecificDeviceMemInitPart1)
    {
        devObjPtr->devMemSpecificDeviceMemInitPart1(devObjPtr);
    }

    if(devObjPtr->unitsDuplicationsPtr)
    {
        /* do duplications of the units */
        smemCheetahDuplicateUnits(devObjPtr);
    }

    /* no need in bobcat2 to do explicit call to
       smemGenericUnitAddressesAlignToBaseAddress1 for the'extra units' ,
       because smemLion3UnitMemoryBindToChunk will do it. */
    smemLion3UnitMemoryBindToChunk(devObjPtr);

    /* bind now tables to memories --
       MUST be done after calling smemLion2TableInfoSet(...)
       so the tables can override previous settings */
    smemBindTablesToMemories(devObjPtr,
        /* array of x units !!! (the units are not set as the first units only , but can be spread)*/
        devMemInfoPtr->unitMemArr,
        SMEM_CHT_NUM_UNITS_MAX_CNS);

    smemLion3TableInfoSetPart2(devObjPtr);

    /*
        bind 'non PP' per unit active memory to the unit.
        the memory spaces are : 'PEX' and 'DFX' (and 'MBUS')
    */
    smemLion3BindNonPpPerUnitActiveMem(devObjPtr,devMemInfoPtr);

    if(devObjPtr->devMemSpecificDeviceMemInitPart2)
    {
        devObjPtr->devMemSpecificDeviceMemInitPart2(devObjPtr);
    }

    /* must be called only after all units finished bind their active memories
       so called after devMemSpecificDeviceMemInitPart2(...) */
    if(devObjPtr->supportActiveMemPerUnit)
    {
        smemLion3BindPerUnitActiveMemToSpecFunTbl(devObjPtr,commonDevMemInfoPtr);
    }

}

/*******************************************************************************
*   smemLion3Init2
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
void smemLion3Init2
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    GT_U32  regAddr;
    GT_U32  ii;
    SKERNEL_DEVICE_OBJECT * tmpDevObjPtr;

    smemLion2Init2(devObjPtr);

    if(devObjPtr->shellDevice == GT_TRUE)
    {
        /* the shell device ... no extra initialization */
        return;
    }

    if(devObjPtr->portGroupSharedDevObjPtr)
    {
        /* Set port group ID in <DevicePhy Addr> */
        smemRegFldSet(devObjPtr, SMEM_CHT_GLB_CTRL_REG(devObjPtr), 21, 3, devObjPtr->portGroupId);

    /* for multi core - call on the last core , because
       smemLion3ActiveWriteFDBGlobalCfg1Reg will loop on all port groups

       call it with the device of the FDB unit */
        if(devObjPtr->portGroupId == (devObjPtr->portGroupSharedDevObjPtr->numOfCoreDevs - 1))
        {
            tmpDevObjPtr = devObjPtr->portGroupSharedDevObjPtr->coreDevInfoPtr[LION3_UNIT_FDB_TABLE_SINGLE_INSTANCE_PORT_GROUP_CNS].devObjPtr;
            /* cause to check the FDB size mode */
            regAddr = SMEM_LION3_FDB_GLOBAL_CONFIG_1_REG(tmpDevObjPtr);
            smemRegUpdateAfterRegFile(tmpDevObjPtr,regAddr,1);
        }
    }
    /* for non multi core - just call it */
    else
    {
        /* cause to check the FDB size mode */
        regAddr = SMEM_LION3_FDB_GLOBAL_CONFIG_1_REG(devObjPtr);
        smemRegUpdateAfterRegFile(devObjPtr,regAddr,1);
    }

    /*  already called from smemCht3Init2()
        --> ** cause to call smemLion3ActiveWriteHierarchicalPolicerControl **
        regAddr = SMEM_XCAT_PLR_HIERARCHICAL_POLICER_CTRL_REG(devObjPtr,0);
        smemRegUpdateAfterRegFile(devObjPtr,regAddr,1);
    */

    /* cause to init eftGlobalEgressFilterEn */
    regAddr = SMEM_LION_TXQ_EGR_FILTER_GLOBAL_EN_REG(devObjPtr);
    smemRegUpdateAfterRegFile(devObjPtr,regAddr,1);

    regAddr = SMEM_CHT2_ROUTER_ADDITIONAL_CONTROL_REG(devObjPtr);
    smemRegUpdateAfterRegFile(devObjPtr,regAddr,1);

    for( ii = 0 ; ii < devObjPtr->fdbNumOfBanks ; ii++)
    {
        /* save 'Quick references' to those registers */
        devObjPtr->fdbBanksCounterArr[ii] =
            smemMemGet(devObjPtr,SMEM_LION3_FDB_BANK_COUNTER_REG(devObjPtr,ii));

        /* save 'Quick references' to those registers */
        devObjPtr->fdbBanksRankArr[ii] =
            smemMemGet(devObjPtr,SMEM_LION3_FDB_BANK_RANKS_REG(devObjPtr,ii));
    }

    /* timestamp queues init */
    /* set size & overwrite enabling based on ERMRK PTP Configuration register default value */
    regAddr = SMEM_LION3_ERMRK_PTP_CONFIG_REG(devObjPtr);
    smemRegUpdateAfterRegFile(devObjPtr,regAddr,1);

    /* set queue "is empty" flag */
    devObjPtr->timestampQueue[SMAIN_DIRECTION_INGRESS_E][0].isEmpty = 1;
    devObjPtr->timestampQueue[SMAIN_DIRECTION_INGRESS_E][1].isEmpty = 1;
    devObjPtr->timestampQueue[SMAIN_DIRECTION_EGRESS_E][0].isEmpty = 1;
    devObjPtr->timestampQueue[SMAIN_DIRECTION_EGRESS_E][1].isEmpty = 1;

    for( ii = 0 ; ii < devObjPtr->portsNumber ; ii++)
    {
        if(!IS_CHT_VALID_PORT(devObjPtr,ii))
        {
            continue;
        }

        devObjPtr->portsArr[ii].timestampEgressQueue[0].actualSize =
            devObjPtr->portsArr[ii].timestampEgressQueue[1].actualSize = 4;
        devObjPtr->portsArr[ii].timestampEgressQueue[0].isEmpty =
            devObjPtr->portsArr[ii].timestampEgressQueue[1].isEmpty = 1;

        /* overwrite enabling */
        regAddr = SMEM_LION3_GOP_PTP_GENERAL_CTRL_REG(devObjPtr,ii);
        smemRegUpdateAfterRegFile(devObjPtr,regAddr,1);
    }

    return;
}
/*******************************************************************************
*  smemLion3ActiveWriteOamTbl
*
* DESCRIPTION:
*    The OAM table write access.
*
* INPUTS:
*    devObjPtr  - device object PTR.
*    address    - Address for ASIC memory.
*    memPtr     - Pointer to the register's memory in the simulation.
*    memSize    - Size of memory to be written
*    param      - Registers' specific parameter.
*    inMemPtr   - Pointer to the memory to get register's content.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void smemLion3ActiveWriteOamTbl
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32   address,
    IN GT_U32   memSize,
    IN GT_U32 * memPtr,
    IN GT_UINTPTR  param,
    IN GT_U32 * inMemPtr
)
{
    GT_U32 trigCmd;             /* Type of triggered action that the OAM
                                   needs to perform */
    GT_U32 entryOffset;         /* The offset from the base address of the
                                   accessed entry */

    GT_U32 cycle;               /* Ingress 0,1 and egress 2 policer cycle */
    GT_U32 * regPtr;            /* Table entry pointer */
    GT_U32 regAddr;             /* Table base address */
    GT_U32 * regDataPtr;        /* Access data pointer */


    cycle = param;

    trigCmd = SMEM_U32_GET_FIELD(*inMemPtr, 1, 1);
    entryOffset = SMEM_U32_GET_FIELD(*inMemPtr, 16, 16);

    regAddr = SMEM_LION2_OAM_BASE_TBL_MEM(devObjPtr, cycle, entryOffset);
    regPtr = smemMemGet(devObjPtr, regAddr);

    /* OAM Table Access Data */
    regDataPtr = smemMemGet(devObjPtr, address + 4);

    /* OAM write */
    if(trigCmd)
    {
        /* Write data from OAM Table Access Data 0,1,2 to OAM table */
        regPtr[0] = regDataPtr[0];
        regPtr[1] = regDataPtr[1];
        regPtr[2] = regDataPtr[2];
        if(SMEM_CHT_IS_SIP5_15_GET(devObjPtr))
        {
            regPtr[3] = regDataPtr[3];
        }
    }
    else
    {
        /* Read data from OAM table to OAM Table Access Data 0, 1 */
        regDataPtr[0] = regPtr[0];
        regDataPtr[1] = regPtr[1];
        regDataPtr[2] = regPtr[2];
        if(SMEM_CHT_IS_SIP5_15_GET(devObjPtr))
        {
            regDataPtr[3] = regPtr[3];
        }
    }

    /* Cleared by the device when the read or write action is completed. */
    SMEM_U32_SET_FIELD(*inMemPtr, 0, 1, 0);

    *memPtr = *inMemPtr;
}

/*******************************************************************************
*  smemLion3ActiveReadExceptionSummaryBitmap
*
* DESCRIPTION:
*       Read Exception Summary Bitmap
*
* INPUTS:
*       devObjPtr   - device object PTR.
*       address     - Address for ASIC memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers' specific parameter.
*
* OUTPUTS:
*       outMemPtr   - Pointer to the memory to copy register's content.
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void smemLion3ActiveReadExceptionSummaryBitmap
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    OUT        GT_U32 * outMemPtr
)
{
    /* Read register and clear data */
    smemChtActiveReadRocRegister(devObjPtr, memPtr, outMemPtr);
}

/*******************************************************************************
*  smemLion3ActiveReadExceptionCounter
*
* DESCRIPTION:
*       Read Exception Counters
*
* INPUTS:
*       devObjPtr   - device object PTR.
*       address     - Address for ASIC memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers' specific parameter.
*
* OUTPUTS:
*       outMemPtr   - Pointer to the memory to copy register's content.
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void smemLion3ActiveReadExceptionCounter
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    OUT        GT_U32 * outMemPtr
)
{
    /* Read register and clear data */
    smemChtActiveReadRocRegister(devObjPtr, memPtr, outMemPtr);
}

/*******************************************************************************
*  smemLion3ActiveReadExceptionGroupStatus
*
* DESCRIPTION:
*       Read Exception Group Status registers
*
* INPUTS:
*       devObjPtr   - device object PTR.
*       address     - Address for ASIC memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers' specific parameter.
*
* OUTPUTS:
*       outMemPtr   - Pointer to the memory to copy register's content.
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void smemLion3ActiveReadExceptionGroupStatus
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    OUT        GT_U32 * outMemPtr
)
{
    /* Read register and clear data */
    smemChtActiveReadRocRegister(devObjPtr, memPtr, outMemPtr);
}



/*******************************************************************************
*  smemLion3ActiveWriteFdbMsg
*
* DESCRIPTION:
*      Write to the Message from CPU Register4 - activate update FDB message.
* INPUTS:
*       devObjPtr - device object PTR.
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
void smemLion3ActiveWriteFdbMsg(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
)
{
    GT_U32            * regPtr;         /* register's entry pointer */
    GT_U32              line;           /* line number */
    GT_U32              rdWr;           /* Read or write operation */
    GT_U32              regAddr_tbl;    /* table memory address */
    GT_U32              regAddr_msg;    /* message memory address */
    GT_U32              RdWrTrig;       /* When set to 1, an FDB read or write access is performed.
                                           This bit is cleared by the device when the access action is completed*/

    /* data to be written */
    *memPtr = *inMemPtr;

    RdWrTrig = SMEM_U32_GET_FIELD(*inMemPtr, 0, 1);
    if(RdWrTrig == 0)
    {
        return;
    }

    rdWr = SMEM_U32_GET_FIELD(*inMemPtr, 1, 1);
    line = SMEM_U32_GET_FIELD(*inMemPtr, 2, 21);

    regAddr_tbl = SMEM_CHT_MAC_TBL_MEM(devObjPtr,line);
    regAddr_msg = SMEM_CHT_MAC_TBL_ACC_DATA0_REG(devObjPtr);

    if (rdWr) /* write the data from the msg registers to the FDB table  */
    {
        /* Mac Table Access Data */
        regPtr = smemMemGet(devObjPtr, regAddr_msg);
        smemMemSet(devObjPtr, regAddr_tbl, regPtr, 5);
    }
    else /* read the data from the FDB table into the msg registers */
    {
        regPtr = smemMemGet(devObjPtr, regAddr_tbl);
        /* Mac Table Access Data */
        smemMemSet(devObjPtr, regAddr_msg, regPtr, 5);
    }

    /* clear <RdWrTrig> */
    SMEM_U32_SET_FIELD(*memPtr, 0, 1, 0);
}

/*******************************************************************************
*  smemLion3ActiveWriteTcamMgLookup
*
* DESCRIPTION:
*       SIP5 Tcam management lookup
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
void smemLion3ActiveWriteTcamMgLookup (
    IN  SKERNEL_DEVICE_OBJECT *devObjPtr,
    IN  GT_U32                 address,
    IN  GT_U32                 memSize,
    IN  GT_U32                *memPtr,
    IN  GT_UINTPTR             param,
    IN  GT_U32                *inMemPtr
)
{
    GT_U32  group;
    GT_U32  hitNum;
    GT_U32  chunk;

    GT_U32  regAddr;
    GT_U32  regValue;

    SIP5_TCAM_CLIENT_ENT   tcamClient = SIP5_TCAM_CLIENT_IPCL0_E;
    GT_U32                 keyArrayPtr[SIP5_TCAM_MAX_SIZE_OF_KEY_ARRAY_CNS/4 + 1];
    SIP5_TCAM_KEY_SIZE_ENT keySize;
    GT_U32                 resultArr[SIP5_TCAM_MAX_NUM_OF_HITS_CNS];
    GT_U32                 maxNumWords;

    /* data to be written */
    *memPtr = *inMemPtr;

    /* get group num */
    for(group = 0; group < SIP5_TCAM_NUM_OF_GROUPS_CNS; group++)
    {
        if(SMEM_U32_GET_FIELD(*inMemPtr, group, 1))
        {
            /* got group num to process */
            break;
        }
    }

    if(group == SIP5_TCAM_NUM_OF_GROUPS_CNS)
    {
        /* not triggered the action */
        return;
    }

    /* get key size */
    regAddr = SMEM_LION3_TCAM_MG_KEY_SIZE_AND_TYPE_GROUP_REG(devObjPtr, group);
    smemRegGet(devObjPtr, regAddr, &regValue);

    keySize = (SIP5_TCAM_KEY_SIZE_ENT)SMEM_U32_GET_FIELD(regValue,4,4);

    keySize ++;/* update to enum values */

    maxNumWords = ((keySize * 10) + 3) / 4;/* max num of words to get */

    for(chunk = 0; chunk < maxNumWords ; chunk++)
    {
        regAddr = SMEM_LION3_TCAM_MG_KEY_GROUP_CHUNK_REG(devObjPtr, group, chunk);

        /* prepare the key for lookup */
        smemRegGet(devObjPtr, regAddr, &regValue);
        keyArrayPtr[chunk] = regValue;
    }

    /* call lookup function */
    sip5TcamLookup(devObjPtr, tcamClient, keyArrayPtr, keySize, resultArr);

    /* save result to mg register */
    for(hitNum = 0; hitNum < SIP5_TCAM_MAX_NUM_OF_HITS_CNS; hitNum++)
    {
        regAddr = SMEM_LION3_TCAM_MG_HIT_GROUP_HIT_NUM_REG(devObjPtr, group, hitHum);

        if(SNET_CHT_POLICY_NO_MATCH_INDEX_CNS == resultArr[hitNum])
        {
            /* set no hit */
            smemRegFldSet(devObjPtr, regAddr, 0, 1, 0);
        }
        else
        {
            /* set hit, address */
            smemRegSet(devObjPtr, regAddr, (resultArr[hitNum] << 1) | 1);
        }
    }

    /* clear the trigger */
    SMEM_U32_SET_FIELD(*memPtr, group, 1, 0);
}

/*******************************************************************************
*  smemLion3ActiveWriteTcamMgHitGroupHitNum
*
* DESCRIPTION:
*       SIP5 Tcam management lookup, read only registers
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
void smemLion3ActiveWriteTcamMgHitGroupHitNum (
    IN  SKERNEL_DEVICE_OBJECT *devObjPtr,
    IN  GT_U32                 address,
    IN  GT_U32                 memSize,
    IN  GT_U32                *memPtr,
    IN  GT_UINTPTR             param,
    IN  GT_U32                *inMemPtr
)
{
    GT_U32 group,  groupMax  = SIP5_TCAM_NUM_OF_GROUPS_CNS - 1;
    GT_U32 hitNum, hitNumMax = SIP5_TCAM_MAX_NUM_OF_HITS_CNS - 1;

    for(group = 0; group < groupMax ; group++)
    {
        for(hitNum = 0 ; hitNum < hitNumMax; hitNum ++)
        {
            if (address == SMEM_LION3_TCAM_MG_HIT_GROUP_HIT_NUM_REG(devObjPtr, group, hitNum))
            {
                /* the register is 'read only' so not allow to write into it */
                return;
            }
        }
    }

    /* the register not belong to this 'read only' registers . so allow to write to it*/
    *memPtr = *inMemPtr;

    return;
}

/*******************************************************************************
*  smemLion3ActiveWriteFDBGlobalCfg1Reg
*
* DESCRIPTION:
*   The function updates FDB table size according to FDB_Global_Configuration 1
*
*
* INPUTS:
*    devObjPtr  - device object PTR.
*    address    - Address for ASIC memory.
*    memPtr     - Pointer to the register's memory in the simulation.
*    memSize    - Size of memory to be written
*    param      - Registers' specific parameter.
*    inMemPtr   - Pointer to the memory to get register's content.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void smemLion3ActiveWriteFDBGlobalCfg1Reg
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
)
{
    GT_U32 fdbSize;
    GT_U32 fdbNumEntries;
    GT_U32 ii;
    GT_U32 multiHashEnable;
    SKERNEL_DEVICE_OBJECT * tmpDevObjPtr;

    /* data to be written */
    *memPtr = *inMemPtr;

    /* FDBSize */
    fdbSize = SMEM_U32_GET_FIELD(*inMemPtr, 11, 3);
    /* get the <FDBHashMode > and <Multi Hash Enable> bits .
       if both are 1 --> we consider the multi hash mode. */
    multiHashEnable = (SMEM_U32_GET_FIELD(*inMemPtr, 2, 2) == 3) ? 1 : 0;

    switch(fdbSize)
    {
        case 0:
            fdbNumEntries = SMEM_MAC_TABLE_SIZE_4KB;
            break;
        case 1:
            fdbNumEntries = SMEM_MAC_TABLE_SIZE_8KB;
            break;
        case 2:
            fdbNumEntries = SMEM_MAC_TABLE_SIZE_16KB;
            break;
        case 3:
            fdbNumEntries = SMEM_MAC_TABLE_SIZE_32KB;
            break;
        case 4:
            fdbNumEntries = SMEM_MAC_TABLE_SIZE_64KB;
            break;
        case 5:
            fdbNumEntries = SMEM_MAC_TABLE_SIZE_128KB;
            break;
        case 6:
            fdbNumEntries = SMEM_MAC_TABLE_SIZE_256KB;
            break;
        case 7:
            fdbNumEntries = SMEM_MAC_TABLE_SIZE_512KB;
            break;
        default:/*on 3 bits value .. can not get here*/
            fdbNumEntries = devObjPtr->fdbNumEntries;/*unchanged*/
            break;
    }

    if(devObjPtr->portGroupSharedDevObjPtr)
    {
        /* need to set the devObjPtr->fdbNumEntries into all the port groups !!! */
        for(ii = 0 ; ii < devObjPtr->portGroupSharedDevObjPtr->numOfCoreDevs ; ii++)
        {
            tmpDevObjPtr = devObjPtr->portGroupSharedDevObjPtr->coreDevInfoPtr[ii].devObjPtr;

            tmpDevObjPtr->fdbNumEntries = fdbNumEntries;
            tmpDevObjPtr->multiHashEnable = multiHashEnable;
        }
    }
    else
    {
        devObjPtr->fdbNumEntries = fdbNumEntries;
        devObjPtr->multiHashEnable = multiHashEnable;
    }

    if(devObjPtr->fdbMaxNumEntries < devObjPtr->fdbNumEntries)
    {
        skernelFatalError("smemLion3ActiveWriteFDBGlobalCfg1Reg: FDB num entries in HW [0x%4.4x] , but set to use[0x%4.4x] \n",
            devObjPtr->fdbMaxNumEntries , devObjPtr->fdbNumEntries);
    }

    return;
}

/*******************************************************************************
*  smemLion3ActiveWriteFDBCountersUpdateControlReg
*
* DESCRIPTION:
*   The function act according to :
*   FDB Counters Bank Update
*               Valid with FDB Counters Update Triggger Indicates on which bank the action is performed.
*   FDB Counters Inc
*               Valid with FDB Counters Update Triggger
*               Which command to activate:
*               0 = decrement.
*               1 = increment.
*   FDB Counters Update Trigger
*               SC 0x0 Trigger FDB counters update.
*               When writing 1 to this bit, FDB counter which is specified in
*               FDB Counters Bank Update field will be incremented or decremented according
*
*
* INPUTS:
*    devObjPtr  - device object PTR.
*    address    - Address for ASIC memory.
*    memPtr     - Pointer to the register's memory in the simulation.
*    memSize    - Size of memory to be written
*    param      - Registers' specific parameter.
*    inMemPtr   - Pointer to the memory to get register's content.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
void smemLion3ActiveWriteFDBCountersUpdateControlReg
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
)
{
    GT_U32  fdbCountersBankUpdate;/*the bank to update*/
    GT_U32  fdbCountersInc;/*operation: increment/decrement */


    if(SMEM_U32_GET_FIELD(*inMemPtr, 0, 1))
    {
        fdbCountersBankUpdate = SMEM_U32_GET_FIELD(*inMemPtr, 3, 4);
        fdbCountersInc = SMEM_U32_GET_FIELD(*inMemPtr, 1, 1);

        sfdbChtBankCounterAction(devObjPtr,fdbCountersBankUpdate,
             fdbCountersInc ?
                SFDB_CHT_BANK_COUNTER_ACTION_INCREMENT_E :
                SFDB_CHT_BANK_COUNTER_ACTION_DECREMENT_E,
            SFDB_CHT_BANK_COUNTER_UPDATE_CLIENT_CPU_EXACT_BANK_E);

        /* Cleared by the device when action is completed. */
        SMEM_U32_SET_FIELD(*inMemPtr, 0, 1, 0);
    }

    *memPtr = *inMemPtr;
}

/*******************************************************************************
*  smemLion3ActiveWriteFDBCountersControlReg
*
* DESCRIPTION:
*   The function act according to :
*       BLC Hold Status -  Bank Learned Counters (BLC) status
*                           The software sets "BLC Hold Enable" when updating BLC counters.
*                           The hardware reponds by activating this field when ready to allow counters hold.
*                           The software should poll on this field before changing BLC counters.
*                           Once done, the software should set "BLC Hold Enable" field to 0.
*               Please note that activating this field will result in all NA, Update and Aging processes to be halted !
*       BLC Hold Enable - Bank Learned Counters (BLC) hold.
*                           SW sets this bit when wanting to update BLC counters.
*                           HW reponses activating "BLC Hold Status" when ready to allow counters hold.
*                           SW should pole on "BLC Hold Status" before changing BLC counters.
*                           Once done, SW should set this field to 0.
*               Please note that activating this field will result in all NA, Update and Aging processes to be halted !
*
*
* INPUTS:
*    devObjPtr  - device object PTR.
*    address    - Address for ASIC memory.
*    memPtr     - Pointer to the register's memory in the simulation.
*    memSize    - Size of memory to be written
*    param      - Registers' specific parameter.
*    inMemPtr   - Pointer to the memory to get register's content.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
void smemLion3ActiveWriteFDBCountersControlReg
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
)
{
    GT_U32  blcHoldEnable;/*indication that CPU request possession over the BLC (Bank Learned Counters)*/
    GT_U32  value;

    value = *inMemPtr ;

    blcHoldEnable = SMEM_U32_GET_FIELD(*inMemPtr, 0, 1);

    if(blcHoldEnable)
    {
        /*Please note that activating this field will result in all NA, Update and Aging processes to be halted !*/
        devObjPtr->fdbBankCounterInCpuPossess = 1;

        /*wait for devObjPtr->fdbBankCounterUsed to be released*/
        while(devObjPtr->fdbBankCounterUsed)
        {
            /* wait for PP to release the counters */
            SIM_OS_MAC(simOsSleep)(1);

            if(devObjPtr->needToDoSoftReset)
            {
                break;
            }
        }

        /* <BLC Hold Status> state that the CPU is the only one to use the counters */
        /* this bit is 'read only'*/
        SMEM_U32_SET_FIELD(value, 1, 1, 1);
    }
    else
    {
        /* <BLC Hold Status> state that the PP is the only one to update the counters */
        /* this bit is 'read only'*/
        devObjPtr->fdbBankCounterInCpuPossess = 0;

        SMEM_U32_SET_FIELD(value, 1, 1, 0);
    }

    *memPtr = value;
}

/*******************************************************************************
*  smemLion3ActiveWriteIngressStcTable
*
* DESCRIPTION:
*  SMEM_BIND_TABLE_MAC(ingrStc)
*          the table is not of type 'write' after 'whole' words :
*          word 0 - RW
*          word 1 - RO
*          word 2 - RW
*
*          the Write to the table word 0,2 is allowed via
*           <IngressSTCW0 WrEn> , <IngressSTCW2 WrEn> in SMEM_CHT_INGRESS_STC_CONF_REG(dev)
*
*           addresses are : 0x0B040000 .. 0x0B040FFC
*
* INPUTS:
*    devObjPtr  - device object PTR.
*    address    - Address for ASIC memory.
*    memPtr     - Pointer to the register's memory in the simulation.
*    memSize    - Size of memory to be written
*    param      - Registers' specific parameter.
*    inMemPtr   - Pointer to the memory to get register's content.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
void smemLion3ActiveWriteIngressStcTable
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
)
{
    GT_U32  regValue;
    GT_U32  regAddr;
    GT_U32  ingressStcWord0WriteEnable,ingressStcWord2WriteEnable;/* word 0 , word2 write enable */
    GT_U32  currentWordIndexWriten;/* index of the current word that is written */
    GT_U32  *internalMemPtr;       /* pointer to internal memory */
    GT_U32  *word0MemPtr,*word2MemPtr;/* pointer to memory of word 0 , word 2 */

    currentWordIndexWriten = (address & 0xF) / 4;

    switch(currentWordIndexWriten)
    {
        case 1:
        default:
            /* the write is ignored , because:
               word 1 : is read only
               word 3 : is not defined
            */
            return;
        case 0:
            /* the actual write done only after write to word 2 */
            /* but we need to save the value of word0 into the internal memory */
            internalMemPtr = CHT_INTERNAL_MEM_PTR(devObjPtr,
                CHT_INTERNAL_SIMULATION_USE_MEM_INGRESS_STC_TEMP_WORD0_E);
            *internalMemPtr = *inMemPtr;
            return;
        case 2:
            /* write to word 2 triggers the actual write */
            break;

    }

    /* get address of the control register */
    regAddr = SMEM_CHT_INGRESS_STC_CONF_REG(devObjPtr);
    smemRegGet(devObjPtr, regAddr,&regValue);

    ingressStcWord0WriteEnable = SMEM_U32_GET_FIELD(regValue, 3, 1);
    ingressStcWord2WriteEnable = SMEM_U32_GET_FIELD(regValue, 4, 1);

    if(ingressStcWord0WriteEnable)
    {
        word0MemPtr = memPtr - 2;

        internalMemPtr = CHT_INTERNAL_MEM_PTR(devObjPtr,
            CHT_INTERNAL_SIMULATION_USE_MEM_INGRESS_STC_TEMP_WORD0_E);

        *word0MemPtr = *internalMemPtr;
    }

    if(ingressStcWord2WriteEnable)
    {
        word2MemPtr = memPtr;
        *word2MemPtr = *inMemPtr;
    }



    return;
}

/*******************************************************************************
*  smemLion3ActiveWriteLfsrConfig
*
* DESCRIPTION:
*       the 2 next register hold self clear bit <LFSR Seed Load Enable>
*           SMEM_CHT_MAC_REG_DB_SIP5_GET(devObjPtr)->EQ.L2ECMP.ePortECMPLFSRConfig
*           SMEM_CHT_MAC_REG_DB_SIP5_GET(devObjPtr)->EQ.L2ECMP.trunkLFSRConfig
*
* INPUTS:
*    devObjPtr  - device object PTR.
*    address    - Address for ASIC memory.
*    memPtr     - Pointer to the register's memory in the simulation.
*    memSize    - Size of memory to be written
*    param      - Registers' specific parameter.
*    inMemPtr   - Pointer to the memory to get register's content.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
void smemLion3ActiveWriteLfsrConfig
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
)
{
    /* we not simulate the SEED load , but we clear <LFSR Seed Load Enable> */
    *memPtr = (*inMemPtr) & (~1);

    return;
}

/*******************************************************************************
*  smemLion3ActiveWriteProtectionLocStatusTable
*
* DESCRIPTION:
*  SMEM_BIND_TABLE_MAC(oamProtectionLocStatusTable)
*          Protection LOC Bit Write Mask register holds the mask for writing to
*          Protection LOC Status table. A bit will be written to the table only
*          if its corresponding bit in Protection LOC Bit Write Mask register
*          is set to 0.
*
* INPUTS:
*    devObjPtr  - device object PTR.
*    address    - Address for ASIC memory.
*    memPtr     - Pointer to the register's memory in the simulation.
*    memSize    - Size of memory to be written
*    param      - Registers' specific parameter.
*    inMemPtr   - Pointer to the memory to get register's content.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
void smemLion3ActiveWriteProtectionLocStatusTable
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
)
{
    GT_U32  regAddr;
    GT_U32  writeMaskRegValue;

    /* Retreive the value from the Protection LOC Bit Write Mask register */
    regAddr = SMEM_LION3_EQ_EPORT_PROTECTION_LOC_WRITE_MASK_REG(devObjPtr);
    smemRegGet(devObjPtr, regAddr, &writeMaskRegValue);

    /* Change only the unmasked bits, keep the masked bits */
    *memPtr = ((*memPtr & writeMaskRegValue) | (*inMemPtr & ~writeMaskRegValue));
}

/*******************************************************************************
*  smemLion3ConvertTaiRegAddrToTaiGroup
*
* DESCRIPTION:
*       Based on the received address finds the TAI group which this address
*       belongs to its address space.
* INPUTS:
*       address     - Address for ASIC memory.
*
* OUTPUTS:
*       groupPtr    - (pointer to) one of possible 9 ones.
*       taiInstPtr  - (pointer to) 0 or 1 for TAI0 or TAI1 respectibly.
*
* RETURNS:
*       GT_TRUE  - matching group was found.
*       GT_FAlSE - no matching group found.
*
* COMMENTS:
*       There are total of possible 18 TAIs - 9 groups each containing 2 TAIs.
*
*******************************************************************************/
static GT_BOOL smemLion3ConvertTaiRegAddrToTaiGroup (
    IN  GT_U32  address,
    OUT GT_U32  *groupPtr,
    OUT GT_U32  *taiInstPtr
)
{
#define TAI_FIRST_GROUP_BASE  0x00180000
#define TAI_SECOND_GROUP_BASE 0x001B0000
#define TAI_THIRD_GROUP_BASE  0x00380000
#define TAI_GROUP_MASK        0x00FF0000
#define TAI_INST_MASK         0x00001000
#define TAI_MASK              0x00FFFFFF

    if( ((address & TAI_GROUP_MASK) >= TAI_FIRST_GROUP_BASE) &&
        ((address & TAI_GROUP_MASK) < TAI_SECOND_GROUP_BASE) )
    {
        /* first group - indexes 0-2 */
        *groupPtr = ((address - TAI_FIRST_GROUP_BASE) & TAI_MASK)/0x10000;
    }
    else if( (address & TAI_GROUP_MASK) == TAI_SECOND_GROUP_BASE )
    {
        /* second group - indexes 3-4 */
        *groupPtr = ((address - TAI_SECOND_GROUP_BASE) & TAI_MASK)/0x4000 + 3;
    }
    else if ( (address & TAI_GROUP_MASK) == TAI_THIRD_GROUP_BASE )
    {
        /* third group - indexes 5-8 */
        *groupPtr = ((address - TAI_THIRD_GROUP_BASE) & TAI_MASK)/0x4000 + 5;
    }
    else
    {
        return GT_FALSE;
    }


    if( address & TAI_INST_MASK )
    {
        *taiInstPtr = 1;
    }
    else
    {
        *taiInstPtr = 0;
    }

    return GT_TRUE;
}

/*******************************************************************************
*  smemLion3TodFuncUpdate
*
* DESCRIPTION:
*       Global configuration settings and TOD function triggering
* INPUTS:
*       devObjPtr   - Device object PTR.
*       taiGroup    - one out of 9 ones
*       taiInst     - within the group - TAI0 or TAI
*       fromGlobalTrigger - indication if we are called from global trigger.
*                   GT_TRUE  - 'from global trigger' use 'timer of unit taiGroup = 0 , taiInst = 0'
*                   GT_FALSE - not global use current time.
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void smemLion3TodFuncUpdate (
    IN  SKERNEL_DEVICE_OBJECT *devObjPtr,
    IN  GT_U32                  taiGroup,
    IN  GT_U32                  taiInst,
    IN  GT_BOOL                 fromGlobalTrigger
)
{
    GT_U32 fieldVal;
    SNET_LION3_PTP_TOD_FUNC_ENT event;
    GT_U32 regAddr;
    GT_U32 *regPtr;

    regAddr = SMEM_LION3_GOP_TAI_FUNC_CONFIG_0_REG(devObjPtr,taiGroup,taiInst);
    regPtr = smemMemGet(devObjPtr, regAddr);

    fieldVal = SMEM_U32_GET_FIELD(*regPtr, 2, 3);
    /* TOD Counter Function  */
    switch(fieldVal)
    {
        case 0:
            event = SNET_LION3_PTP_TOD_UPDATE_E;
            break;
        case 1:
            event = SNET_LION3_PTP_TOD_FREQ_UPDATE_E;
            break;
        case 2:
            event = SNET_LION3_PTP_TOD_INCREMENT_E;
            break;
        case 3:
            event = SNET_LION3_PTP_TOD_DECREMENT_E;
            break;
        case 4:
            event = SNET_LION3_PTP_TOD_CAPTURE_E;
            break;
        case 5:
            event = SNET_LION3_PTP_TOD_GRACEFUL_INC_E;
            break;
        case 6:
            event = SNET_LION3_PTP_TOD_GRACEFUL_DEC_E;
            break;
        case 7:
            event = SNET_LION3_PTP_TOD_NOP_E;
            break;
        default:
            return;
    }

    snetLion3PtpTodCounterApply(devObjPtr, event, taiGroup, taiInst);

    if( (event != SNET_LION3_PTP_TOD_FREQ_UPDATE_E) &&
        (event != SNET_LION3_PTP_TOD_CAPTURE_E) &&
        (event != SNET_LION3_PTP_TOD_NOP_E) )
    {
        if (fromGlobalTrigger == GT_TRUE && taiGroup != 0 && taiInst != 0)
        {
            /* get the time from 0,0 */
            devObjPtr->eTodTimeStampClockDiff[taiGroup][taiInst] =
                devObjPtr->eTodTimeStampClockDiff[0][0];
        }
        else
        {
            /* Update clock reference value for TOD calculation */
            devObjPtr->eTodTimeStampClockDiff[taiGroup][taiInst] =
                                                SIM_OS_MAC(simOsTickGet)();
        }
    }
}
/*******************************************************************************
*  smemLion3TodFuncUpdate_ALL
*
* DESCRIPTION:
*       update all TAI units about the TOD function triggering
* INPUTS:
*       devObjPtr   - Device object PTR.
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void smemLion3TodFuncUpdate_ALL (
    IN  SKERNEL_DEVICE_OBJECT *devObjPtr
)
{
    GT_U32  taiGroup,taiGroupMax = devObjPtr->numOfTaiUnits;
    GT_U32  taiInst , taiInstMax = 2;

    if(devObjPtr->supportSingleTai)
    {
        taiInstMax = 1;
    }

    for (taiGroup = 0 ; taiGroup < taiGroupMax ; taiGroup++)
    {
        for (taiInst = 0 ; taiInst < taiInstMax ; taiInst++)
        {
            smemLion3TodFuncUpdate(devObjPtr,taiGroup,taiInst,GT_TRUE);
        }
    }
}


/*******************************************************************************
*  smemLion3ActiveWriteTodFuncConfReg
*
* DESCRIPTION:
*       Global configuration settings and TOD function triggering
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
void smemLion3ActiveWriteTodFuncConfReg (
    IN  SKERNEL_DEVICE_OBJECT *devObjPtr,
    IN  GT_U32                 address,
    IN  GT_U32                 memSize,
    IN  GT_U32                *memPtr,
    IN  GT_UINTPTR             param,
    IN  GT_U32                *inMemPtr
)
{
    GT_U32 fieldVal;
    GT_U32 taiGroup;    /* one out of 9 ones */
    GT_U32 taiInst;     /* within the group - TAI0 or TAI */

    /* Output value */
    *memPtr = *inMemPtr;

    fieldVal = SMEM_U32_GET_FIELD(*inMemPtr, 0, 1);
    if(fieldVal == 0)
    {
        /* TOD Counter Function not triggered */
        return;
    }

    if(devObjPtr->supportSingleTai)
    {
        taiGroup = 0;
        taiInst  = 0;
    }
    else
    {
        if( smemLion3ConvertTaiRegAddrToTaiGroup(address, &taiGroup, &taiInst) == GT_FALSE )
        {
            /* No matching TAI group found - not expected to occur */
            simWarningPrintf("smemLion3ActiveWriteTodFuncConfReg : ERROR -  No matching TAI group found - not expected to occur\n");
            simWarningPrintf("smemLion3ActiveWriteTodFuncConfReg : ERROR Address is 0x%08X\n");
            return;
        }
    }

    smemLion3TodFuncUpdate(devObjPtr,taiGroup,taiInst,GT_FALSE);

    /* Clear TOD Counter Function Trigger bit once the action is done */
    SMEM_U32_SET_FIELD(*memPtr, 0, 1, 0);

    return;
}

/*******************************************************************************
*  smemLion3ActiveWriteHaInterruptsMaskReg
*
* DESCRIPTION:
*
*   the application changed the value of the interrupts mask register.
*   check if there is waiting interrupt for that.
*
*
* INPUTS:
*    devObjPtr  - device object PTR.
*    address    - Address for ASIC memory.
*    memPtr     - Pointer to the register's memory in the simulation.
*    memSize    - Size of memory to be written
*    param      - Registers' specific parameter.
*    inMemPtr   - Pointer to the memory to get register's content.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void smemLion3ActiveWriteHaInterruptsMaskReg (
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
)
{
    GT_U32 dataArray[3];                     /* Data array to put into buffer */

    /* Address of HA interrupt cause register */
    dataArray[0] = SMEM_LION3_HA_INTERRUPT_CAUSE_REG(devObjPtr);
    /* Address of HA interrupt mask register */
    dataArray[1] = SMEM_LION3_HA_INTERRUPT_MASK_REG(devObjPtr);
    /* HA Cause register summary bit */
    dataArray[2] = SMEM_LION3_HA_SUM_INT(devObjPtr);

    /* Call for common interrupt mask active write function */
    smemChtActiveWriteInterruptsMaskReg(devObjPtr, address, memSize, memPtr,
                                        (GT_UINTPTR)dataArray, inMemPtr);
}

/*******************************************************************************
*  smemLion3ActiveWriteErmrkInterruptsMaskReg
*
* DESCRIPTION:
*
*   the application changed the value of the interrupts mask register.
*   check if there is waiting interrupt for that.
*
*
* INPUTS:
*    devObjPtr  - device object PTR.
*    address    - Address for ASIC memory.
*    memPtr     - Pointer to the register's memory in the simulation.
*    memSize    - Size of memory to be written
*    param      - Registers' specific parameter.
*    inMemPtr   - Pointer to the memory to get register's content.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void smemLion3ActiveWriteErmrkInterruptsMaskReg (
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
)
{
    GT_U32 dataArray[3];                     /* Data array to put into buffer */

    /* Address of ERMRK interrupt cause register */
    dataArray[0] = SMEM_LION3_ERMRK_INTERRUPT_CAUSE_REG(devObjPtr);
    /* Address of ERMRK interrupt mask register */
    dataArray[1] = SMEM_LION3_ERMRK_INTERRUPT_MASK_REG(devObjPtr);
    /* ERMRK Cause register summary bit */
    dataArray[2] = SMEM_LION3_ERMRK_SUM_INT(devObjPtr);

    /* Call for common interrupt mask active write function */
    smemChtActiveWriteInterruptsMaskReg(devObjPtr, address, memSize, memPtr,
                                        (GT_UINTPTR)dataArray, inMemPtr);
}

/*******************************************************************************
*  smemLion3ActiveWriteErmrkPtpConfReg
*
* DESCRIPTION:
*       ERMRK Timestamp Queues configurations
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
void smemLion3ActiveWriteErmrkPtpConfReg (
    IN  SKERNEL_DEVICE_OBJECT *devObjPtr,
    IN  GT_U32                 address,
    IN  GT_U32                 memSize,
    IN  GT_U32                *memPtr,
    IN  GT_UINTPTR             param,
    IN  GT_U32                *inMemPtr
)
{
    GT_U32 fieldVal;

    /* Output value */
    *memPtr = *inMemPtr;

    fieldVal = SMEM_U32_GET_FIELD(*inMemPtr, 0, 1);

    devObjPtr->timestampQueue[SMAIN_DIRECTION_INGRESS_E][0].overwriteEnable = fieldVal;
    devObjPtr->timestampQueue[SMAIN_DIRECTION_INGRESS_E][1].overwriteEnable = fieldVal;
    devObjPtr->timestampQueue[SMAIN_DIRECTION_EGRESS_E][0].overwriteEnable = fieldVal;
    devObjPtr->timestampQueue[SMAIN_DIRECTION_EGRESS_E][1].overwriteEnable = fieldVal;

    fieldVal = SMEM_U32_GET_FIELD(*inMemPtr, 2, 9);

    devObjPtr->timestampQueue[SMAIN_DIRECTION_INGRESS_E][0].actualSize = fieldVal;
    devObjPtr->timestampQueue[SMAIN_DIRECTION_INGRESS_E][1].actualSize = fieldVal;
    devObjPtr->timestampQueue[SMAIN_DIRECTION_EGRESS_E][0].actualSize = fieldVal;
    devObjPtr->timestampQueue[SMAIN_DIRECTION_EGRESS_E][1].actualSize = fieldVal;
}


/*******************************************************************************
*  smemLion3ActiveWriteErmrkTsQueueEntryIdClearReg
*
* DESCRIPTION:
*       Writing to this register (whatever value) clears the Timestamp Queues
*       Entry ID values.
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
void smemLion3ActiveWriteErmrkTsQueueEntryIdClearReg (
    IN  SKERNEL_DEVICE_OBJECT *devObjPtr,
    IN  GT_U32                 address,
    IN  GT_U32                 memSize,
    IN  GT_U32                *memPtr,
    IN  GT_UINTPTR             param,
    IN  GT_U32                *inMemPtr
)
{
    GT_U32 regAddr;     /* register address */
    GT_U32 *regPtr;     /* register pointer */

    /* Output value */
    *memPtr = *inMemPtr;

    regAddr = SMEM_LION3_ERMRK_TS_QUEUE_ENTRY_ID_REG(devObjPtr);
    regPtr = smemMemGet(devObjPtr, regAddr);
    *regPtr = 0x0;
}

/*******************************************************************************
*  smemLion3ActiveReadErmrkTsQueueEntryWord1Word2Reg
*
* DESCRIPTION:
*       Read Timestamp Queue Entry Word registers
*
* INPUTS:
*       devObjPtr   - device object PTR.
*       address     - Address for ASIC memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers' specific parameter.
*
* OUTPUTS:
*       outMemPtr   - Pointer to the memory to copy register's content.
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void smemLion3ActiveReadErmrkTsQueueEntryWord1Word2Reg
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    OUT        GT_U32 * outMemPtr
)
{
    SMAIN_DIRECTION_ENT direction;
    GT_U32 queueNum;

    *outMemPtr = *memPtr;

    /* Find Timestamp queue based on the queue specific Word1 or Word2 register address */
    queueNum = (address & 0x4) >> 2;
    switch(address & 0xFF)
    {
        case 0x28:
        case 0x2C: direction =  SMAIN_DIRECTION_EGRESS_E;
            break;
        case 0x30:
        case 0x34: direction =  SMAIN_DIRECTION_INGRESS_E;
            break;
        default:
            return;
    }

    snetlion3TimestampQueueRemoveEntry(devObjPtr, direction, queueNum);
}

/*******************************************************************************
*  smemLion3GopPtpPortFromRegAddressGet
*
* DESCRIPTION:
*       Find the port num based on the GOP PTP related register address
*
* INPUTS:
*       address - Address for ASIC memory.
*
* OUTPUTS:
*       portPtr - Pointer to the calculated port number.
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void smemLion3GopPtpPortFromRegAddressGet
(
    IN         GT_U32   address,
    OUT        GT_U32 * portPtr
)
{
    if( ((address & 0xFFF000) >= 0x180000) &&
        ((address & 0xFFF000) <= 0x1B7000) )
    {
        /* port 0-55 */
        *portPtr = ((address & 0xFFF000) - 0x180000)/0x1000;
    }
    else if( ((address & 0xFFF000) >= 0x380000) &&
             ((address & 0xFFF000) <= 0x38F000) )
    {
        /* port 56-71 */
        *portPtr = 56 + (((address & 0xFFF000) - 0x380000)/0x1000);
    }
    else
    {
        skernelFatalError("smemLion3GopPtpPortFromRegAddressGet: illegal port<n> for address 0x%x\n", address);
    }
}

/*******************************************************************************
*  smemLion3ActiveReadGopPtpTxTsQueueReg2Reg
*
* DESCRIPTION:
*       Read Port Timestamp Egress Queue Entry Word registers
*
* INPUTS:
*       devObjPtr   - device object PTR.
*       address     - Address for ASIC memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers' specific parameter.
*
* OUTPUTS:
*       outMemPtr   - Pointer to the memory to copy register's content.
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void smemLion3ActiveReadGopPtpTxTsQueueReg2Reg
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    OUT        GT_U32 * outMemPtr
)
{
    GT_U32 egressPort;
    GT_U32 queueNum;

    *outMemPtr = *memPtr;

    smemLion3GopPtpPortFromRegAddressGet(address, &egressPort);

    queueNum = 0xFFFFFFFF;
    if( (address & 0xFF) == 0x14 )
    {
        queueNum = 0;
    }
    else if( (address & 0xFF) == 0x20 )
    {
        queueNum = 1;
    }
    else
    {
        skernelFatalError("smemLion3ActiveReadGopPtpTxTsQueueReg2Reg: illegal queue<n> address\n");
    }

    snetlion3TimestampPortEgressQueueRemoveEntry(devObjPtr, egressPort, queueNum);
}

/*******************************************************************************
*  smemLion3ActiveWriteGopPtpGeneralCtrlReg
*
* DESCRIPTION:
*       Port Timestamp Egress Queues configurations
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
void smemLion3ActiveWriteGopPtpGeneralCtrlReg (
    IN  SKERNEL_DEVICE_OBJECT *devObjPtr,
    IN  GT_U32                 address,
    IN  GT_U32                 memSize,
    IN  GT_U32                *memPtr,
    IN  GT_UINTPTR             param,
    IN  GT_U32                *inMemPtr
)
{
    GT_U32 egressPort;
    GT_U32 fieldVal;
    GT_U32 regAddr;      /* Register address */
    GT_U32 *regPtr;      /* Register pointer */

    /* Output value */
    *memPtr = *inMemPtr;

    smemLion3GopPtpPortFromRegAddressGet(address, &egressPort);

    fieldVal = SMEM_U32_GET_FIELD(*inMemPtr, 6, 1);

    devObjPtr->portsArr[egressPort].timestampEgressQueue[0].overwriteEnable = fieldVal;
    devObjPtr->portsArr[egressPort].timestampEgressQueue[1].overwriteEnable = fieldVal;

    if( SMEM_U32_GET_FIELD(*inMemPtr, 4, 1) )
    {
        regAddr = SMEM_LION3_GOP_PTP_PTPv2_PKT_CNTR_REG(devObjPtr,egressPort);
        regPtr = smemMemGet(devObjPtr, regAddr);
        *regPtr = 0x0;

        regAddr = SMEM_LION3_GOP_PTP_PTPv1_PKT_CNTR_REG(devObjPtr,egressPort);
        regPtr = smemMemGet(devObjPtr, regAddr);
        *regPtr = 0x0;

        regAddr = SMEM_LION3_GOP_PTP_Y1731_PKT_CNTR_REG(devObjPtr,egressPort);
        regPtr = smemMemGet(devObjPtr, regAddr);
        *regPtr = 0x0;

        regAddr = SMEM_LION3_GOP_PTP_NTPTS_PKT_CNTR_REG(devObjPtr,egressPort);
        regPtr = smemMemGet(devObjPtr, regAddr);
        *regPtr = 0x0;

        regAddr = SMEM_LION3_GOP_PTP_NTPRX_PKT_CNTR_REG(devObjPtr,egressPort);
        regPtr = smemMemGet(devObjPtr, regAddr);
        *regPtr = 0x0;

        regAddr = SMEM_LION3_GOP_PTP_NTPTX_PKT_CNTR_REG(devObjPtr,egressPort);
        regPtr = smemMemGet(devObjPtr, regAddr);
        *regPtr = 0x0;

        regAddr = SMEM_LION3_GOP_PTP_WAMP_PKT_CNTR_REG(devObjPtr,egressPort);
        regPtr = smemMemGet(devObjPtr, regAddr);
        *regPtr = 0x0;

        regAddr = SMEM_LION3_GOP_PTP_TOTAL_PTP_PKTS_CNTR_REG(devObjPtr,egressPort);
        regPtr = smemMemGet(devObjPtr, regAddr);
        *regPtr = 0x0;

        regAddr = SMEM_LION3_GOP_PTP_NONE_ACTION_PKT_CNTR_REG(devObjPtr,egressPort);
        regPtr = smemMemGet(devObjPtr, regAddr);
        *regPtr = 0x0;

        regAddr = SMEM_LION3_GOP_PTP_FORWARD_ACTION_PKT_CNTR_REG(devObjPtr,egressPort);
        regPtr = smemMemGet(devObjPtr, regAddr);
        *regPtr = 0x0;

        regAddr = SMEM_LION3_GOP_PTP_DROP_ACTION_PKT_CNTR_REG(devObjPtr,egressPort);
        regPtr = smemMemGet(devObjPtr, regAddr);
        *regPtr = 0x0;

        regAddr = SMEM_LION3_GOP_PTP_CAPTURE_ACTION_PKT_CNTR_REG(devObjPtr,egressPort);
        regPtr = smemMemGet(devObjPtr, regAddr);
        *regPtr = 0x0;

        regAddr = SMEM_LION3_GOP_PTP_ADD_TIME_ACTION_PKT_CNTR_REG(devObjPtr,egressPort);
        regPtr = smemMemGet(devObjPtr, regAddr);
        *regPtr = 0x0;

        regAddr = SMEM_LION3_GOP_PTP_ADD_CORRECTED_TIME_ACTION_PKT_CNTR_REG(devObjPtr,egressPort);
        regPtr = smemMemGet(devObjPtr, regAddr);
        *regPtr = 0x0;

        regAddr = SMEM_LION3_GOP_PTP_CAPTURE_ADD_TIME_PKT_CNTR_REG(devObjPtr,egressPort);
        regPtr = smemMemGet(devObjPtr, regAddr);
        *regPtr = 0x0;

        regAddr = SMEM_LION3_GOP_PTP_CAPTURE_ADD_CORRECTED_TIME_ACTION_PKT_CNTR_REG(devObjPtr,egressPort);
        regPtr = smemMemGet(devObjPtr, regAddr);
        *regPtr = 0x0;

        regAddr = SMEM_LION3_GOP_PTP_ADD_INGRESS_TIME_ACTION_PKT_CNTR_REG(devObjPtr,egressPort);
        regPtr = smemMemGet(devObjPtr, regAddr);
        *regPtr = 0x0;

        regAddr = SMEM_LION3_GOP_PTP_CAPTURE_ADD_INGRESS_TIME_ACTION_PKT_CNTR_REG(devObjPtr,egressPort);
        regPtr = smemMemGet(devObjPtr, regAddr);
        *regPtr = 0x0;

        regAddr = SMEM_LION3_GOP_PTP_CAPTURE_INGRESS_TIME_ACTION_PKT_CNTR_REG(devObjPtr,egressPort);
        regPtr = smemMemGet(devObjPtr, regAddr);
        *regPtr = 0x0;
    }
}

/*******************************************************************************
*  smemLion3ActiveWriteGopPtpInterruptsMaskReg
*
* DESCRIPTION:
*
*   the application changed the value of the interrupts mask register.
*   check if there is waiting interrupt for that.
*
*
* INPUTS:
*    devObjPtr  - device object PTR.
*    address    - Address for ASIC memory.
*    memPtr     - Pointer to the register's memory in the simulation.
*    memSize    - Size of memory to be written
*    param      - Registers' specific parameter.
*    inMemPtr   - Pointer to the memory to get register's content.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void smemLion3ActiveWriteGopPtpInterruptsMaskReg (
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
)
{
    GT_U32 dataArray[3];                     /* Data array to put into buffer */
    GT_U32 egressPort;

    smemLion3GopPtpPortFromRegAddressGet(address, &egressPort);

    /* Address of ERMRK interrupt cause register */
    dataArray[0] = SMEM_LION3_GOP_PTP_INT_CAUSE_REG(devObjPtr, egressPort),
    /* Address of ERMRK interrupt mask register */
    dataArray[1] = SMEM_LION3_GOP_PTP_INT_MASK_REG(devObjPtr, egressPort),
    /* ERMRK Cause register summary bit */
    dataArray[2] = SMEM_LION3_GOP_SUM_INT(devObjPtr);

    /* Call for common interrupt mask active write function */
    smemChtActiveWriteInterruptsMaskReg(devObjPtr, address, memSize, memPtr,
                                        (GT_UINTPTR)dataArray, inMemPtr);
}

/*******************************************************************************
*  smemLion3ActiveWriteHierarchicalPolicerControl
*
* DESCRIPTION:
*    Set Hierarchical Policer control register in iplr0, and update Policer Memory Control
*         configuration
*
* INPUTS:
*    devObjPtr  - device object PTR.
*    address    - Address for ASIC memory.
*    memPtr     - Pointer to the register's memory in the simulation.
*    memSize    - Size of memory to be written
*    param      - Registers' specific parameter.
*    inMemPtr   - Pointer to the memory to get register's content.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
* 
*    Meter memories (meter configuration memory and token buckets memory) became
*    shared between IPLR0 and IPLR1 starting from SIP 5.15 (BobK).
*    Direct access to meter memories done through IPLR0. Hierarchical Policer
*    control register configuration does not influence on meter memories starting
*    from SIP 5.15.
*    Counting memory for SIP 5.15 is managed as for previous SIP 5 devices.
*******************************************************************************/
void smemLion3ActiveWriteHierarchicalPolicerControl
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
)
{
    GT_U32 meterBase;        /* metering table start addr in iplr0 (tablesInfo.policer) */
    GT_U32 countBase;        /* counters table start addr in iplr0 (tablesInfo.policerCounters) */
    GT_U32 meterEntrySize, countEntrySize;        /* table entry size 0x20, equal for both tables */
    GT_BIT mem0UsedByIplr0,mem1UsedByIplr0,mem2UsedByIplr0;
    GT_U32 numEntriesForIplr0;

    /* set register content */
    *memPtr = *inMemPtr ;

    meterBase = devObjPtr->policerSupport.meterTblBaseAddr;
    countBase = devObjPtr->policerSupport.countTblBaseAddr;

    meterEntrySize = devObjPtr->tablesInfo.policer.paramInfo[0].step;        /* size of entry */
    countEntrySize = devObjPtr->tablesInfo.policerCounters.paramInfo[0].step;        /* size of entry */

    mem0UsedByIplr0 = SMEM_U32_GET_FIELD(*inMemPtr, 5, 1) ? 0 : 1;
    mem1UsedByIplr0 = SMEM_U32_GET_FIELD(*inMemPtr, 6, 1) ? 0 : 1;
    mem2UsedByIplr0 = SMEM_U32_GET_FIELD(*inMemPtr, 7, 1) ? 0 : 1;

    if((mem0UsedByIplr0 == mem2UsedByIplr0) && (mem2UsedByIplr0 != mem1UsedByIplr0))
    {
        /* the memories must be configured 'continues memories' .
            so if last and first memory are of the same type , the middle one must also be of this type !
        */
        skernelFatalError("smemLion3ActiveWriteHierarchicalPolicerControl: the memories must be configured 'continues memories' \n"
            "so if last and first memory are of the same type , the middle one must also be of this type ! \n");
    }

    devObjPtr->policerSupport.iplr1EntriesFirst = mem0UsedByIplr0 ? 0 : 1;

    numEntriesForIplr0 = 0;
    if(mem0UsedByIplr0)
    {
        numEntriesForIplr0 += devObjPtr->policerSupport.iplrMemoriesSize[0];
    }

    if(mem1UsedByIplr0)
    {
        numEntriesForIplr0 += devObjPtr->policerSupport.iplrMemoriesSize[1];
    }

    if(mem2UsedByIplr0)
    {
        numEntriesForIplr0 += devObjPtr->policerSupport.iplrMemoriesSize[2];
    }

    devObjPtr->policerSupport.iplr0TableSize = numEntriesForIplr0;
    devObjPtr->policerSupport.iplr1TableSize = devObjPtr->policerSupport.iplrTableSize - numEntriesForIplr0;

    if(devObjPtr->policerSupport.iplr1EntriesFirst == 0)
    {
        if(!SMEM_CHT_IS_SIP5_15_GET(devObjPtr))
        {
            devObjPtr->tablesInfo.policer.commonInfo.baseAddress = meterBase;
        }

        devObjPtr->tablesInfo.policerCounters.commonInfo.baseAddress = countBase;

        if(0 == devObjPtr->policerSupport.iplr1TableSize)
        {
            /* IPLR1 not uses the memory ! */
            if(!SMEM_CHT_IS_SIP5_15_GET(devObjPtr))
            {
                devObjPtr->tablesInfo.policer.paramInfo[1].step = SMAIN_NOT_VALID_CNS;
            }
            devObjPtr->tablesInfo.policerCounters.paramInfo[1].step = SMAIN_NOT_VALID_CNS;
        }
        else
        {
            /* step to get from IPLR0 to IPLR1 . since IPLR0 is before IPLR1 this is positive value */
            if(!SMEM_CHT_IS_SIP5_15_GET(devObjPtr))
            {
                devObjPtr->tablesInfo.policer.paramInfo[1].step = devObjPtr->policerSupport.iplr0TableSize * meterEntrySize;
            }
            devObjPtr->tablesInfo.policerCounters.paramInfo[1].step = devObjPtr->policerSupport.iplr0TableSize * countEntrySize;
        }
    }
    else
    {
        if(devObjPtr->policerSupport.iplr0TableSize == 0)
        {
            /* IPLR0 not uses the memory ! */
            if(!SMEM_CHT_IS_SIP5_15_GET(devObjPtr))
            {
                devObjPtr->tablesInfo.policer.commonInfo.baseAddress = SMAIN_NOT_VALID_CNS;
            }
            devObjPtr->tablesInfo.policerCounters.commonInfo.baseAddress = SMAIN_NOT_VALID_CNS;
        }
        else
        {
            if(!SMEM_CHT_IS_SIP5_15_GET(devObjPtr))
            {
                devObjPtr->tablesInfo.policer.commonInfo.baseAddress = meterBase +
                    (devObjPtr->policerSupport.iplr1TableSize)*meterEntrySize;
            }
            devObjPtr->tablesInfo.policerCounters.commonInfo.baseAddress = countBase +
                (devObjPtr->policerSupport.iplr1TableSize)*countEntrySize;
        }

        /* step to get from IPLR0 to IPLR1 . since IPLR0 is after IPLR1 this is negative value */
        if(!SMEM_CHT_IS_SIP5_15_GET(devObjPtr))
        {
            devObjPtr->tablesInfo.policer.paramInfo[1].step = meterBase - (devObjPtr->tablesInfo.policer.commonInfo.baseAddress);
        }
        devObjPtr->tablesInfo.policerCounters.paramInfo[1].step = countBase - (devObjPtr->tablesInfo.policerCounters.commonInfo.baseAddress);
    }
}

/*******************************************************************************
*   smemLion3ActiveWriteMeterEntry
*
* DESCRIPTION:
*       when the CPU write the meter entry , the device should copy values of
*       <max_burst_size0> to <bucket_size0>  and <max_burst_size1> to <bucket_size1>
*       function should do nothing when memory access for 'counting' memory.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       meterEntryPtr - pointer to the meter entry in the memory
*       meterAddress - address of the entry
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
void smemLion3ActiveWriteMeterEntry
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32                * meterEntryPtr,
    IN GT_U32                  meterAddress,
    IN GT_U32                  plrId
)
{
    GT_U32 max_bucket_size0;
    GT_U32 max_bucket_size1;
    GT_U32 entryIndex;
    GT_U32 countingBaseRegAddr;
    GT_U32 internalAddrMask = 0x00FFFFFF;
    GT_U32 meterEntrySize;        /* table entry size */
    GT_U32 *startOfMeterEntryPtr;/* pointer to start of meter entry */
    GT_U32 offsetToStartEntry;

    countingBaseRegAddr = internalAddrMask & (SMEM_XCAT_INGR_POLICER_CNT_ENTRY_REG(devObjPtr,0,plrId));

    if(countingBaseRegAddr <= (meterAddress & internalAddrMask))
    {
        /* this address belongs to counting memory space */
        return;
    }

    /* need to copy values of <max_burst_size0> to <bucket_size0> and
       <max_burst_size1> to <bucket_size1> only when 'last word' is updated.
       (at least after word6 was done .. because MAX_BURST_SIZE1 end at word6)
       and then the meterEntryPtr need to be converted to address of start entry
    */
    /*meterEntrySize is in bytes*/
    meterEntrySize = devObjPtr->tablesInfo.policer.paramInfo[0].step;        /* size of entry */
    /* offset in words from start of the entry*/
    offsetToStartEntry = (meterAddress & (meterEntrySize - 1)) / 4;/* /4 to convert bytes to words */
    if(offsetToStartEntry < 6)
    {
        return;
    }
    /* reduce words as the meterEntryPtr is of type pointer to GT_U32 */
    startOfMeterEntryPtr = meterEntryPtr - offsetToStartEntry;
    entryIndex = SMAIN_NOT_VALID_CNS;

    max_bucket_size0 = SMEM_LION3_PLR_METERING_ENTRY_FIELD_GET(devObjPtr,startOfMeterEntryPtr,entryIndex,
        SMEM_LION3_PLR_METERING_TABLE_FIELDS_MAX_BURST_SIZE0);

    SMEM_LION3_PLR_METERING_ENTRY_FIELD_SET(devObjPtr,startOfMeterEntryPtr,entryIndex,
        SMEM_LION3_PLR_METERING_TABLE_FIELDS_BUCKET_SIZE0,
        max_bucket_size0);

    max_bucket_size1 = SMEM_LION3_PLR_METERING_ENTRY_FIELD_GET(devObjPtr,startOfMeterEntryPtr,entryIndex,
        SMEM_LION3_PLR_METERING_TABLE_FIELDS_MAX_BURST_SIZE1);

    SMEM_LION3_PLR_METERING_ENTRY_FIELD_SET(devObjPtr,startOfMeterEntryPtr,entryIndex,
        SMEM_LION3_PLR_METERING_TABLE_FIELDS_BUCKET_SIZE1,
        max_bucket_size1);

}



/*******************************************************************************
* smemLion3RegsInfoSetPart2
*
* DESCRIPTION:
*       align the addresses of registers according to addresses of corresponding
*       units
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
static void smemLion3RegsInfoSetPart2
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = devObjPtr->deviceMemory;
    SMEM_UNIT_CHUNKS_STC    *currUnitChunkPtr;
    SMEM_SIP5_PP_REGS_ADDR_STC      *regAddrDbSip5Ptr;

    regAddrDbSip5Ptr = SMEM_CHT_MAC_REG_DB_SIP5_GET(devObjPtr);

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_IPVX)];
    smemGenericRegistersArrayAlignToUnit(
        REG_PTR_AND_SIZE_MAC(regAddrDbSip5Ptr->IPvX),
        currUnitChunkPtr);

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_IOAM)];
    smemGenericRegistersArrayAlignToUnit(
        REG_PTR_AND_SIZE_MAC(regAddrDbSip5Ptr->OAMUnit[0]),
        currUnitChunkPtr);

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_EOAM)];
    smemGenericRegistersArrayAlignToUnit(
        REG_PTR_AND_SIZE_MAC(regAddrDbSip5Ptr->OAMUnit[1]),
        currUnitChunkPtr);

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_ERMRK)];
    smemGenericRegistersArrayAlignToUnit(
        REG_PTR_AND_SIZE_MAC(regAddrDbSip5Ptr->ERMRK),
        currUnitChunkPtr);

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_GOP)];
    smemGenericRegistersArrayAlignToUnit(
        REG_PTR_AND_SIZE_MAC(regAddrDbSip5Ptr->GOP),
        currUnitChunkPtr);

    currUnitChunkPtr = NULL;
    if(SMAIN_NOT_VALID_CNS != UNIT_BASE_ADDR_GET_ALLOW_NON_EXIST_UNIT_MAC(devObjPtr,UNIT_TAI))
    {
        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TAI)];
    }

    if(currUnitChunkPtr && currUnitChunkPtr->numOfChunks)
    {
        smemGenericRegistersArrayAlignForceUnitReset(GT_TRUE);

        /* the TAI is not part of the GOP base addresses */
        smemGenericRegistersArrayAlignToUnit(
            REG_PTR_AND_SIZE_MAC(regAddrDbSip5Ptr->TAI),
            currUnitChunkPtr);

        smemGenericRegistersArrayAlignForceUnitReset(GT_FALSE);
    }
    else
    {
        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_GOP)];

        /* the TAI is part of the GOP base addresses */
        smemGenericRegistersArrayAlignToUnit(
            REG_PTR_AND_SIZE_MAC(regAddrDbSip5Ptr->TAI),
            currUnitChunkPtr);
    }

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TTI)];
    smemGenericRegistersArrayAlignToUnit(
        REG_PTR_AND_SIZE_MAC(regAddrDbSip5Ptr->TTI),
        currUnitChunkPtr);

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_RX_DMA)];
    smemGenericRegistersArrayAlignToUnit(
        REG_PTR_AND_SIZE_MAC(regAddrDbSip5Ptr->rxDMA[0]),
        currUnitChunkPtr);

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_FDB)];
    smemGenericRegistersArrayAlignToUnit(
        REG_PTR_AND_SIZE_MAC(regAddrDbSip5Ptr->FDB),
        currUnitChunkPtr);

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_L2I)];
    smemGenericRegistersArrayAlignToUnit(
        REG_PTR_AND_SIZE_MAC(regAddrDbSip5Ptr->L2I),
        currUnitChunkPtr);


    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_EQ)];
    smemGenericRegistersArrayAlignToUnit1(
        REG_PTR_AND_SIZE_MAC(regAddrDbSip5Ptr->EQ),
        currUnitChunkPtr,
        GT_TRUE);/* addresses are '0' based and need += of the 'base unit address' */

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_LPM)];
    smemGenericRegistersArrayAlignToUnit1(
        REG_PTR_AND_SIZE_MAC(regAddrDbSip5Ptr->LPM),
        currUnitChunkPtr,
        GT_TRUE);/* addresses are '0' based and need += of the 'base unit address' */

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TCAM)];
    smemGenericRegistersArrayAlignToUnit(
        REG_PTR_AND_SIZE_MAC(regAddrDbSip5Ptr->TCAM),
        currUnitChunkPtr);


    {
        GT_U32  gtsIndex;
        for(gtsIndex = 0 ; gtsIndex < 2 ;gtsIndex++)
        {
            currUnitChunkPtr = &gtsUnitChunk[gtsIndex];
            smemGenericRegistersArrayAlignToUnit(
                REG_PTR_AND_SIZE_MAC(regAddrDbSip5Ptr->GTS[gtsIndex]),
                currUnitChunkPtr);
        }
    }


    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_EGF_EFT)];
    smemGenericRegistersArrayAlignToUnit(
        REG_PTR_AND_SIZE_MAC(regAddrDbSip5Ptr->EGF_eft),
        currUnitChunkPtr);

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_EGF_QAG)];
    smemGenericRegistersArrayAlignToUnit1(
        REG_PTR_AND_SIZE_MAC(regAddrDbSip5Ptr->EGF_qag),
        currUnitChunkPtr,
        GT_TRUE);/* addresses are '0' based and need += of the 'base unit address' */

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_EGF_SHT)];
    smemGenericRegistersArrayAlignToUnit1(
        REG_PTR_AND_SIZE_MAC(regAddrDbSip5Ptr->EGF_sht),
        currUnitChunkPtr,
        GT_TRUE);/* addresses are '0' based and need += of the 'base unit address' */

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_HA)];
    smemGenericRegistersArrayAlignToUnit1(
        REG_PTR_AND_SIZE_MAC(regAddrDbSip5Ptr->HA),
        currUnitChunkPtr,
        GT_TRUE);/* addresses are '0' based and need += of the 'base unit address' */

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_MLL)];
    smemGenericRegistersArrayAlignToUnit1(
        REG_PTR_AND_SIZE_MAC(regAddrDbSip5Ptr->MLL),
        currUnitChunkPtr,
        GT_TRUE);/* addresses are '0' based and need += of the 'base unit address' */

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_IPCL)];
    smemGenericRegistersArrayAlignToUnit1(
        REG_PTR_AND_SIZE_MAC(regAddrDbSip5Ptr->PCL),
        currUnitChunkPtr,
        GT_TRUE);/* addresses are '0' based and need += of the 'base unit address' */

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_EPCL)];
    smemGenericRegistersArrayAlignToUnit1(
        REG_PTR_AND_SIZE_MAC(regAddrDbSip5Ptr->EPCL),
        currUnitChunkPtr,
        GT_TRUE);/* addresses are '0' based and need += of the 'base unit address' */


    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_IPLR)];
    smemGenericRegistersArrayAlignToUnit1(
        REG_PTR_AND_SIZE_MAC(regAddrDbSip5Ptr->PLR[SMEM_SIP5_PP_PLR_UNIT_IPLR_0_E]),
        currUnitChunkPtr,
        GT_TRUE);/* addresses are '0' based and need += of the 'base unit address' */

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_IPLR1)];
    smemGenericRegistersArrayAlignToUnit1(
        REG_PTR_AND_SIZE_MAC(regAddrDbSip5Ptr->PLR[SMEM_SIP5_PP_PLR_UNIT_IPLR_1_E]),
        currUnitChunkPtr,
        GT_TRUE);/* addresses are '0' based and need += of the 'base unit address' */

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_EPLR)];
    smemGenericRegistersArrayAlignToUnit1(
        REG_PTR_AND_SIZE_MAC(regAddrDbSip5Ptr->PLR[SMEM_SIP5_PP_PLR_UNIT_EPLR_E]),
        currUnitChunkPtr,
        GT_TRUE);/* addresses are '0' based and need += of the 'base unit address' */



    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_CNC)];
    smemGenericRegistersArrayAlignToUnit1(
        REG_PTR_AND_SIZE_MAC(regAddrDbSip5Ptr->CNC[0]),
        currUnitChunkPtr,
        GT_TRUE);/* addresses are '0' based and need += of the 'base unit address' */

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_CNC_1)];
    smemGenericRegistersArrayAlignToUnit1(
        REG_PTR_AND_SIZE_MAC(regAddrDbSip5Ptr->CNC[1]),
        currUnitChunkPtr,
        GT_TRUE);/* addresses are '0' based and need += of the 'base unit address' */

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TXQ_QUEUE)];
    smemGenericRegistersArrayAlignToUnit1(
        REG_PTR_AND_SIZE_MAC(regAddrDbSip5Ptr->TXQ.queue),
        currUnitChunkPtr,
        GT_TRUE);/* addresses are '0' based and need += of the 'base unit address' */

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TXQ_LL)];
    smemGenericRegistersArrayAlignToUnit1(
        REG_PTR_AND_SIZE_MAC(regAddrDbSip5Ptr->TXQ.ll),
        currUnitChunkPtr,
        GT_TRUE);/* addresses are '0' based and need += of the 'base unit address' */

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TXQ_DQ)];
    smemGenericRegistersArrayAlignToUnit1(
        REG_PTR_AND_SIZE_MAC(regAddrDbSip5Ptr->TXQ.dq[0]),
        currUnitChunkPtr,
        GT_TRUE);/* addresses are '0' based and need += of the 'base unit address' */

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_CPFC)];
    smemGenericRegistersArrayAlignToUnit1(
        REG_PTR_AND_SIZE_MAC(regAddrDbSip5Ptr->TXQ.pfc),
        currUnitChunkPtr,
        GT_TRUE);/* addresses are '0' based and need += of the 'base unit address' */

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TXQ_QCN)];
    smemGenericRegistersArrayAlignToUnit1(
        REG_PTR_AND_SIZE_MAC(regAddrDbSip5Ptr->TXQ.qcn),
        currUnitChunkPtr,
        GT_TRUE);/* addresses are '0' based and need += of the 'base unit address' */

    if(SMAIN_NOT_VALID_CNS != UNIT_BASE_ADDR_GET_ALLOW_NON_EXIST_UNIT_MAC(devObjPtr,UNIT_LMS))
    {
        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_LMS)];
        smemGenericRegistersArrayAlignToUnit1(
            REG_PTR_AND_SIZE_MAC(regAddrDbSip5Ptr->LMS[0]),
            currUnitChunkPtr,
            GT_TRUE);/* addresses are '0' based and need += of the 'base unit address' */
    }

    if(SMAIN_NOT_VALID_CNS != UNIT_BASE_ADDR_GET_ALLOW_NON_EXIST_UNIT_MAC(devObjPtr,UNIT_LMS1))
    {
        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_LMS1)];
        smemGenericRegistersArrayAlignToUnit1(
            REG_PTR_AND_SIZE_MAC(regAddrDbSip5Ptr->LMS[1]),
            currUnitChunkPtr,
            GT_TRUE);/* addresses are '0' based and need += of the 'base unit address' */
    }

    if(SMAIN_NOT_VALID_CNS != UNIT_BASE_ADDR_GET_ALLOW_NON_EXIST_UNIT_MAC(devObjPtr,UNIT_LMS2))
    {
        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_LMS2)];
        smemGenericRegistersArrayAlignToUnit1(
            REG_PTR_AND_SIZE_MAC(regAddrDbSip5Ptr->LMS[2]),
            currUnitChunkPtr,
            GT_TRUE);/* addresses are '0' based and need += of the 'base unit address' */
    }

    if(SMAIN_NOT_VALID_CNS != UNIT_BASE_ADDR_GET_ALLOW_NON_EXIST_UNIT_MAC(devObjPtr,UNIT_RX_DMA_1))
    {
        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_RX_DMA_1)];
        smemGenericRegistersArrayAlignToUnit(
            REG_PTR_AND_SIZE_MAC(regAddrDbSip5Ptr->rxDMA[1]),
            currUnitChunkPtr);
    }

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_BM)];
    smemGenericRegistersArrayAlignToUnit(
        REG_PTR_AND_SIZE_MAC(regAddrDbSip5Ptr->BM),
        currUnitChunkPtr);
    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TX_FIFO)];
    smemGenericRegistersArrayAlignToUnit(
        REG_PTR_AND_SIZE_MAC(regAddrDbSip5Ptr->TXFIFO[0]),
        currUnitChunkPtr);
    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TX_DMA)];
    smemGenericRegistersArrayAlignToUnit(
        REG_PTR_AND_SIZE_MAC(regAddrDbSip5Ptr->TXDMA[0]),
        currUnitChunkPtr);
    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_MPPM)];
    smemGenericRegistersArrayAlignToUnit(
        REG_PTR_AND_SIZE_MAC(regAddrDbSip5Ptr->MPPM),
        currUnitChunkPtr);

    if(SMAIN_NOT_VALID_CNS != UNIT_BASE_ADDR_GET_ALLOW_NON_EXIST_UNIT_MAC(devObjPtr,UNIT_RX_DMA_2))
    {
        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_RX_DMA_2)];
        smemGenericRegistersArrayAlignToUnit(
            REG_PTR_AND_SIZE_MAC(regAddrDbSip5Ptr->rxDMA[2]),
            currUnitChunkPtr);
    }
    if(SMAIN_NOT_VALID_CNS != UNIT_BASE_ADDR_GET_ALLOW_NON_EXIST_UNIT_MAC(devObjPtr,UNIT_TX_DMA_1))
    {
        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TX_DMA_1)];
        smemGenericRegistersArrayAlignToUnit(
            REG_PTR_AND_SIZE_MAC(regAddrDbSip5Ptr->TXDMA[1]),
            currUnitChunkPtr);
    }
    if(SMAIN_NOT_VALID_CNS != UNIT_BASE_ADDR_GET_ALLOW_NON_EXIST_UNIT_MAC(devObjPtr,UNIT_TX_DMA_2))
    {
        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TX_DMA_2)];
        smemGenericRegistersArrayAlignToUnit(
            REG_PTR_AND_SIZE_MAC(regAddrDbSip5Ptr->TXDMA[2]),
            currUnitChunkPtr);
    }
    if(SMAIN_NOT_VALID_CNS != UNIT_BASE_ADDR_GET_ALLOW_NON_EXIST_UNIT_MAC(devObjPtr,UNIT_TX_FIFO_1))
    {
        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TX_FIFO_1)];
        smemGenericRegistersArrayAlignToUnit(
            REG_PTR_AND_SIZE_MAC(regAddrDbSip5Ptr->TXFIFO[1]),
            currUnitChunkPtr);
    }
    if(SMAIN_NOT_VALID_CNS != UNIT_BASE_ADDR_GET_ALLOW_NON_EXIST_UNIT_MAC(devObjPtr,UNIT_TX_FIFO_2))
    {
        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TX_FIFO_2)];
        smemGenericRegistersArrayAlignToUnit(
            REG_PTR_AND_SIZE_MAC(regAddrDbSip5Ptr->TXFIFO[2]),
            currUnitChunkPtr);
    }

    if(SMAIN_NOT_VALID_CNS != UNIT_BASE_ADDR_GET_ALLOW_NON_EXIST_UNIT_MAC(devObjPtr,UNIT_ETH_TX_FIFO))
    {
        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_ETH_TX_FIFO)];
        smemGenericRegistersArrayAlignToUnit(
            REG_PTR_AND_SIZE_MAC(regAddrDbSip5Ptr->ETH),
            currUnitChunkPtr);
    }

    if(SMAIN_NOT_VALID_CNS != UNIT_BASE_ADDR_GET_ALLOW_NON_EXIST_UNIT_MAC(devObjPtr,UNIT_TXQ_DQ_1))
    {
        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TXQ_DQ_1)];
        smemGenericRegistersArrayAlignToUnit(
            REG_PTR_AND_SIZE_MAC(regAddrDbSip5Ptr->TXQ.dq[1]),
            currUnitChunkPtr);
    }
    if(SMAIN_NOT_VALID_CNS != UNIT_BASE_ADDR_GET_ALLOW_NON_EXIST_UNIT_MAC(devObjPtr,UNIT_TXQ_DQ_2))
    {
        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TXQ_DQ_2)];
        smemGenericRegistersArrayAlignToUnit(
            REG_PTR_AND_SIZE_MAC(regAddrDbSip5Ptr->TXQ.dq[2]),
            currUnitChunkPtr);
    }

}

/*******************************************************************************
* smemLion3RegsInfoSet
*
* DESCRIPTION:
*       Init memory module for Lion2 and above devices.
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
void smemLion3RegsInfoSet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = devObjPtr->deviceMemory;
    GT_U32  unitBaseAddress;

    smemLion2RegsInfoSet(devObjPtr);

    unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_IPVX);
    SMEM_CHT_SIP5_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,IPvX                ,unitBaseAddress);

    unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_IOAM);
    SMEM_CHT_SIP5_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,OAMUnit[0]         ,unitBaseAddress);

    unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_EOAM);
    SMEM_CHT_SIP5_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,OAMUnit[1]         ,unitBaseAddress);

    unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_ERMRK);
    SMEM_CHT_SIP5_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,ERMRK ,unitBaseAddress);

    unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_TTI);
    SMEM_CHT_SIP5_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,TTI                ,unitBaseAddress);

    unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_RX_DMA);
    SMEM_CHT_SIP5_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,rxDMA[0]           ,unitBaseAddress);

    unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_FDB);
    SMEM_CHT_SIP5_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,FDB                ,unitBaseAddress);

    unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_L2I);
    SMEM_CHT_SIP5_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,L2I                ,unitBaseAddress);

    unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_EQ);
    SMEM_CHT_SIP5_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,EQ                 ,unitBaseAddress);

    unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_LPM);
    SMEM_CHT_SIP5_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,LPM                ,unitBaseAddress);

    unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_TCAM);
    SMEM_CHT_SIP5_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,TCAM               ,unitBaseAddress);

    unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_GOP);
    SMEM_CHT_SIP5_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,GOP                ,unitBaseAddress);

    if(SMAIN_NOT_VALID_CNS != UNIT_BASE_ADDR_GET_ALLOW_NON_EXIST_UNIT_MAC(devObjPtr,UNIT_TAI))
    {
        /* TAI is NOT part of the GOP */
        unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_TAI);
    }
    else
    {
        /* TAI is part of the GOP */
        unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_GOP);
    }
    SMEM_CHT_SIP5_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,TAI                ,unitBaseAddress);

    {
        GT_U32  gtsIndex;
        for(gtsIndex = 0 ; gtsIndex < 2 ;gtsIndex++)
        {
            unitBaseAddress = UNIT_BASE_ADDR_FROM_STRING_GET_MAC(devObjPtr,hostingUnitForGtsUnit[gtsIndex]);
            SMEM_CHT_SIP5_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,GTS[gtsIndex]       ,unitBaseAddress);

            gtsUnitChunk[gtsIndex].chunkIndex =
                devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_STRING_GET_MAC(devObjPtr,hostingUnitForGtsUnit[gtsIndex])].chunkIndex;
        }
    }

    unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_EGF_EFT);
    SMEM_CHT_SIP5_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,EGF_eft                 ,unitBaseAddress);

    unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_EGF_QAG);
    SMEM_CHT_SIP5_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,EGF_qag                 ,unitBaseAddress);

    unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_EGF_SHT);
    SMEM_CHT_SIP5_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,EGF_sht                 ,unitBaseAddress);

    unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_HA);
    SMEM_CHT_SIP5_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,HA                 ,unitBaseAddress);

    unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_MLL);
    SMEM_CHT_SIP5_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,MLL                ,unitBaseAddress);

    unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_IPCL);
    SMEM_CHT_SIP5_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,PCL               ,unitBaseAddress);

    unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_EPCL);
    SMEM_CHT_SIP5_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,EPCL               ,unitBaseAddress);

    unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_IPLR);
    SMEM_CHT_SIP5_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,PLR[SMEM_SIP5_PP_PLR_UNIT_IPLR_0_E]              ,unitBaseAddress);
    unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_IPLR1);
    SMEM_CHT_SIP5_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,PLR[SMEM_SIP5_PP_PLR_UNIT_IPLR_1_E]              ,unitBaseAddress);
    unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_EPLR);
    SMEM_CHT_SIP5_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,PLR[SMEM_SIP5_PP_PLR_UNIT_EPLR_E]              ,unitBaseAddress);

    unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_CNC);
    SMEM_CHT_SIP5_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,CNC[0]              ,unitBaseAddress);

    unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_CNC_1);
    SMEM_CHT_SIP5_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,CNC[1]              ,unitBaseAddress);

    unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_TXQ_QUEUE);
    SMEM_CHT_SIP5_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,TXQ.queue           ,unitBaseAddress);

    unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_TXQ_LL);
    SMEM_CHT_SIP5_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,TXQ.ll              ,unitBaseAddress);

    unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_TXQ_DQ);
    SMEM_CHT_SIP5_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,TXQ.dq[0]           ,unitBaseAddress);

    unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_TXQ_QCN);
    SMEM_CHT_SIP5_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,TXQ.qcn              ,unitBaseAddress);

    unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_CPFC);
    SMEM_CHT_SIP5_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,TXQ.pfc             ,unitBaseAddress);

    if(SMAIN_NOT_VALID_CNS != UNIT_BASE_ADDR_GET_ALLOW_NON_EXIST_UNIT_MAC(devObjPtr,UNIT_LMS))
    {
        unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_LMS);
        SMEM_CHT_SIP5_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,LMS[0]              ,unitBaseAddress);
    }

    if(SMAIN_NOT_VALID_CNS != UNIT_BASE_ADDR_GET_ALLOW_NON_EXIST_UNIT_MAC(devObjPtr,UNIT_LMS1))
    {
        unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_LMS1);
        SMEM_CHT_SIP5_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,LMS[1]              ,unitBaseAddress);
    }

    if(SMAIN_NOT_VALID_CNS != UNIT_BASE_ADDR_GET_ALLOW_NON_EXIST_UNIT_MAC(devObjPtr,UNIT_LMS2))
    {
        unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_LMS2);
        SMEM_CHT_SIP5_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,LMS[2]              ,unitBaseAddress);
    }


    unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_BM);
    SMEM_CHT_SIP5_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,BM     ,unitBaseAddress);

    unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_TX_FIFO);
    SMEM_CHT_SIP5_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,TXFIFO[0]     ,unitBaseAddress);

    unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_TX_DMA);
    SMEM_CHT_SIP5_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,TXDMA[0]     ,unitBaseAddress);

    unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_MPPM);
    SMEM_CHT_SIP5_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,MPPM     ,unitBaseAddress);

    if(SMAIN_NOT_VALID_CNS != UNIT_BASE_ADDR_GET_ALLOW_NON_EXIST_UNIT_MAC(devObjPtr,UNIT_RX_DMA_1))
    {
        unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_RX_DMA_1);
        SMEM_CHT_SIP5_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,rxDMA[1]           ,unitBaseAddress);
    }
    if(SMAIN_NOT_VALID_CNS != UNIT_BASE_ADDR_GET_ALLOW_NON_EXIST_UNIT_MAC(devObjPtr,UNIT_RX_DMA_2))
    {
        unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_RX_DMA_2);
        SMEM_CHT_SIP5_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,rxDMA[2]           ,unitBaseAddress);
    }

    if(SMAIN_NOT_VALID_CNS != UNIT_BASE_ADDR_GET_ALLOW_NON_EXIST_UNIT_MAC(devObjPtr,UNIT_TX_DMA_1))
    {
        unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_TX_DMA_1);
        SMEM_CHT_SIP5_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,TXDMA[1]           ,unitBaseAddress);
    }
    if(SMAIN_NOT_VALID_CNS != UNIT_BASE_ADDR_GET_ALLOW_NON_EXIST_UNIT_MAC(devObjPtr,UNIT_TX_DMA_2))
    {
        unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_TX_DMA_2);
        SMEM_CHT_SIP5_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,TXDMA[2]           ,unitBaseAddress);
    }

    if(SMAIN_NOT_VALID_CNS != UNIT_BASE_ADDR_GET_ALLOW_NON_EXIST_UNIT_MAC(devObjPtr,UNIT_TX_FIFO_1))
    {
        unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_TX_FIFO_1);
        SMEM_CHT_SIP5_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,TXFIFO[1]           ,unitBaseAddress);
    }
    if(SMAIN_NOT_VALID_CNS != UNIT_BASE_ADDR_GET_ALLOW_NON_EXIST_UNIT_MAC(devObjPtr,UNIT_TX_FIFO_2))
    {
        unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_TX_FIFO_2);
        SMEM_CHT_SIP5_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,TXFIFO[2]           ,unitBaseAddress);
    }

    if(SMAIN_NOT_VALID_CNS != UNIT_BASE_ADDR_GET_ALLOW_NON_EXIST_UNIT_MAC(devObjPtr,UNIT_TXQ_DQ_1))
    {
        unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_TXQ_DQ_1);
        SMEM_CHT_SIP5_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,TXQ.dq[1]           ,unitBaseAddress);
    }
    if(SMAIN_NOT_VALID_CNS != UNIT_BASE_ADDR_GET_ALLOW_NON_EXIST_UNIT_MAC(devObjPtr,UNIT_TXQ_DQ_2))
    {
        unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_TXQ_DQ_2);
        SMEM_CHT_SIP5_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,TXQ.dq[2]           ,unitBaseAddress);
    }



    if(devObjPtr->supportTrafficManager)
    {
        unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_TMDROP);
        SMEM_CHT_SIP5_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,TMDROP          ,unitBaseAddress);

        unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_TM_INGRESS_GLUE);
        SMEM_CHT_SIP5_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,TM_INGR_GLUE    ,unitBaseAddress);

        unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_TM_EGRESS_GLUE);
        SMEM_CHT_SIP5_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,TM_EGR_GLUE     ,unitBaseAddress);

        unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_TMQMAP);
        SMEM_CHT_SIP5_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,TMQMAP          ,unitBaseAddress);

        unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_ETH_TX_FIFO);
        SMEM_CHT_SIP5_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,ETH     ,unitBaseAddress);

    }

    /* need to be called before smemLion3RegsInfoSetPart2 */
    smemSip5RegsInfoSet(devObjPtr);

    smemDfxServerRegsInfoSet(devObjPtr);

    smemLion3RegsInfoSetPart2(devObjPtr);
}

/*******************************************************************************
*  smemLion3ActiveWriteTransSdmaPacketGeneratorConfigQueueReg
*
* DESCRIPTION:
*       Configure the Packet-Generator mode in selected SDMA
*
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
void smemLion3ActiveWriteTransSdmaPacketGeneratorConfigQueueReg
(
    IN  SKERNEL_DEVICE_OBJECT *devObjPtr,
    IN  GT_U32                 address,
    IN  GT_U32                 memSize,
    IN  GT_U32                *memPtr,
    IN  GT_UINTPTR             param,
    IN  GT_U32                *inMemPtr
)
{
    GT_U32 txQueue;     /* SDMA Tx Queue */

    /* Output value */
    *memPtr = *inMemPtr;

    /* Select SDMA Tx queue */
    txQueue = param + (address & 0xf) / 0x4;

    /* Set register value in DB */
    devObjPtr->sdmaTransmitInfo.
        sdmaTransmitData[txQueue].txQueuePacketGenCfgReg = *memPtr;
}

/*******************************************************************************
*  smemLion3ActiveWriteTransSdmaPacketCountConfigQueueReg
*
* DESCRIPTION:
*       Configure the Packet Count
*
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
void smemLion3ActiveWriteTransSdmaPacketCountConfigQueueReg
(
    IN  SKERNEL_DEVICE_OBJECT *devObjPtr,
    IN  GT_U32                 address,
    IN  GT_U32                 memSize,
    IN  GT_U32                *memPtr,
    IN  GT_UINTPTR             param,
    IN  GT_U32                *inMemPtr
)
{
    /* Output value bits[0:29] except bits [30:31] - Packet Counter Status RO field */
    SMEM_U32_SET_FIELD(*memPtr, 0, 29, *inMemPtr);
}

/*******************************************************************************
*  smemLion3ActiveAuqHostCfgWrite
*
* DESCRIPTION:
*       AUQ Host Configuration active write.
*
* INPUTS:
*       deviceObjPtr - device object PTR.
*       address     - Address for ASIC memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers's specific parameter.
*       inMemPtr    - Pointer to the memory to get register's content.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void smemLion3ActiveAuqHostCfgWrite
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR param,
    IN         GT_U32 * inMemPtr
)
{
    CHT2_FUQ_MEM  *fuqMemPtr;
    CHT_AUQ_MEM   *auqMemPtr;

    /* Bit0 - queues stopped,  bit1 - NA Queue reset, bit2 - FU Queue reset*/

    /* bit0 only not self cleared */
    *memPtr = ((*inMemPtr) & 1);

    if ((*inMemPtr) & 1)/* the stop 'queue'(s) was set */
    {
        if ((*inMemPtr) & (1 << 1))
        {
            /* reset NA queue */
            auqMemPtr = SMEM_CHT_MAC_AUQ_MEM_GET(devObjPtr);
            memset(auqMemPtr, 0, sizeof(CHT_AUQ_MEM));
        }

        if ((*inMemPtr) & (1 << 2))
        {
            /* reset FU queue */
            fuqMemPtr = SMEM_CHT_MAC_FUQ_MEM_GET(devObjPtr);
            memset(fuqMemPtr, 0, sizeof(CHT2_FUQ_MEM));
        }

        /* bit0 only not self cleared */
        *memPtr = ((*inMemPtr) & 1);
    }
    else
    {
        /* bit0==0, self clear bits not triggered any action */
        /* but they saved to the memory                      */
        *memPtr = *inMemPtr;
    }
}

/*******************************************************************************
*  smemLion3ActiveWriteDfxServerResetControlReg
*
* DESCRIPTION:
*       Dfx Server Reset Control Register active write.
*
* INPUTS:
*       deviceObjPtr - device object PTR.
*       address     - Address for ASIC memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers's specific parameter.
*       inMemPtr    - Pointer to the memory to get register's content.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void smemLion3ActiveWriteDfxServerResetControlReg
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR param,
    IN         GT_U32 * inMemPtr
)
{
    DECLARE_FUNC_NAME(smemLion3ActiveWriteDfxServerResetControlReg);

    SBUF_BUF_ID bufferId;               /* Buffer ID */
    GT_U8  * dataPtr;                   /* Pointer to the data in the buffer */
    GT_U32 dataSize;                    /* Data size */

    *memPtr = *inMemPtr;/* save value that written by the CPU*/

    /*<MG Soft Reset Trigger>*/
    if(SMEM_U32_GET_FIELD((*memPtr) ,1 ,1) == 0/* 0 is TRIGGER !!! */)
    {
        /* Get buffer + suspend the pool to ignore other allocations */
        bufferId = sbufAllocAndPoolSuspend(devObjPtr->bufPool, sizeof(GT_U32));

        if (bufferId == NULL)
        {
            simWarningPrintf("smemLion3ActiveWriteDfxServerResetControlReg: no buffers to trigger soft reset \n");
            return;
        }

        /**********************************************************************/
        /* NOTE: skernelDeviceSoftResetGeneric(...) will process this message */
        /**********************************************************************/

        /* allow using __LOG like for packets .. to debug the 'soft reset' feature */
        SIM_OS_MAC(simOsTaskOwnTaskPurposeSet)(SIM_OS_TASK_PURPOSE_TYPE_PP_PIPE_PROCESSING_DAEMON_E, NULL);

        {

            /* according to PP designer :
                even when <PEX Skip Init if MG Soft Reset> = SKIP INIT ON (no pex reset)

                if the CPU will try to access the device while it is doing 'soft reset' :
                Behavior in this case in un-predictable (probably the device will hang)

                so simulation is 'unbind' from SCIB ... so trying to access to it will cause 'FATAL ERROR' !!!
            */

            __LOG(("Unbind SCIB interface for device[%d]\n",
                devObjPtr->deviceId));
            /*************************/
            /* Unbind SCIB interface */
            /*************************/
            scibUnBindDevice(devObjPtr->deviceId);
        }

        /******************************************************************/
        /* prepare message for the device to start process the soft reset */
        /******************************************************************/

        /* Get actual data pointer */
        sbufDataGet(bufferId, &dataPtr, &dataSize);

        /* Copy Global Control Register to buffer */
        memcpy(dataPtr, memPtr, sizeof(GT_U32));

        /* Set source type of buffer */
        bufferId->srcType = SMAIN_SRC_TYPE_CPU_E;

        /* Set message type of buffer */
        bufferId->dataType = SMAIN_MSG_TYPE_SOFT_RESET_E;

        __LOG(("Put buffer to queue  + suspend the queue \n"));
        /* Put buffer to queue  + suspend the queue to ignore other buffers
           trying to ingress the queue (from other tasks in the device)


           NOTE: that CPU can not do any 'Access to PP' because we did 'Unbind SCIB interface'
           and it will be re-open only when we process the soft reset in
           skernelDeviceSoftResetGeneric(...)
        */
        squeBufPutAndQueueSuspend(devObjPtr->queueId, SIM_CAST_BUFF(bufferId));

        /*restore the thread 'purpose' */
        SIM_OS_MAC(simOsTaskOwnTaskPurposeSet)(SIM_OS_TASK_PURPOSE_TYPE_CPU_APPLICATION_E, NULL);

        return;
    }
}

/*******************************************************************************
*  smemLion3ActiveWriteDfxServerDeviceCtrl4Reg
*
* DESCRIPTION:
*       Dfx Server 'device ctrl 4' register. allow to trigger TAI sync to all
*       TAI units with single trigger. (bit 22)
*
* INPUTS:
*       deviceObjPtr - device object PTR.
*       address     - Address for ASIC memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers's specific parameter.
*       inMemPtr    - Pointer to the memory to get register's content.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void smemLion3ActiveWriteDfxServerDeviceCtrl4Reg
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR param,
    IN         GT_U32 * inMemPtr
)
{
    GT_U32  origValue = *memPtr;

    *memPtr = *inMemPtr;/* save value that written by the CPU*/

    /* <ptp_internal_trigger >
       This field enables triggering the PTP mechanism
       (instead of the PTP_PULSE pad) */
    if(0 == SMEM_U32_GET_FIELD((*memPtr) ,22 ,1))
    {
        /* no more to do */
        return;
    }

    if(SMEM_U32_GET_FIELD(origValue ,22 ,1))
    {
        /* the bit was set before the 'current write' , so do not trigger new action */
        return;
    }

    /* update all TAI units about the TOD function triggering */
    smemLion3TodFuncUpdate_ALL(devObjPtr);

    /* DO NOT clear the bit ... because it is 'RW' */
}

/*******************************************************************************
*  smemLion3SdmaRxResourceErrorCountAndModeActiveRead
*
* DESCRIPTION:
*       Read Exception Group Status registers
*
* INPUTS:
*       devObjPtr   - device object PTR.
*       address     - Address for ASIC memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers' specific parameter.
*
* OUTPUTS:
*       outMemPtr   - Pointer to the memory to copy register's content.
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void smemLion3SdmaRxResourceErrorCountAndModeActiveRead
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    OUT        GT_U32 * outMemPtr
)
{
    /* Copy registers content to the output memory */
    *outMemPtr = *memPtr;
    /* Clear only first 8 bits after read */
    *memPtr &= 0xFFFFFF00;
}

