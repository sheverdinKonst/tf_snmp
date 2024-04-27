/******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* smemBobk.c
*
* DESCRIPTION:
*       Bobk memory mapping implementation
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 4 $
*
*******************************************************************************/

#include <asicSimulation/SKernel/smem/smemBobcat2.h>

/* not used memory */
#define DUMMY_UNITS_BASE_ADDR_CNS(index)        0x70000000 + UNIT_BASE_ADDR_MAC(2*index)


/* addresses of units that exists in bobk but not in bobcat2 */
#define BOBK_RX_DMA_1_UNIT_BASE_ADDR_CNS        0x68000000
#define BOBK_TX_DMA_1_UNIT_BASE_ADDR_CNS        0x66000000
#define BOBK_TX_FIFO_1_UNIT_BASE_ADDR_CNS       0x67000000
#define BOBK_ETH_TX_FIFO_1_UNIT_BASE_ADDR_CNS   0x62000000
#define BOBK_RX_DMA_GLUE_UNIT_BASE_ADDR_CNS     0x63000000
#define BOBK_TX_DMA_GLUE_UNIT_BASE_ADDR_CNS     0x64000000
#define BOBK_TAI_UNIT_BASE_ADDR_CNS             0x65000000
#define BOBK_GOP_SMI_UNIT_BASE_ADDR_CNS(index)  0x54000000 + (UNIT_BASE_ADDR_MAC((2*index)))
#define BOBK_GOP_LED_UNIT_BASE_ADDR_CNS(index)  0x21000000 + (UNIT_BASE_ADDR_MAC((2*index)))
#define BOBK_GOP_LED_4_UNIT_BASE_ADDR_CNS       0x50000000

/* indication of init value in runtime ... not during compilation */
#define INIT_INI_RUNTIME    0

/* will set ports 0..47 */
/* instead of calling:
    {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x10180800 ,0)},
                FORMULA_TWO_PARAMETERS(30, 0x4, 56, 0x00001000)}

    need to call:
    BOBK_PORTS_LOW_PORTS__SUPPORT_FORMULA_1_PARAMS(0x10180800 ,0,
        30, 0x4)
*/
#define BOBK_PORTS_LOW_PORTS__BY_END_ADDR(startAddr,endAddr,numSteps1,stepSize1) \
    BOBK_PORTS_LOW_PORTS__BY_NUM_BYTES(startAddr,(((endAddr)-(startAddr)) + 4),numSteps1,stepSize1)

#define BOBK_PORTS_LOW_PORTS__BY_NUM_BYTES(startAddr,numBytes,numSteps1,stepSize1) \
    {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(startAddr   ,numBytes)},                 \
        FORMULA_TWO_PARAMETERS(48, BOBCAT2_PORT_OFFSET_CNS,numSteps1, stepSize1)}



/* will set ports 56..59,64..71 , for ports with 'steps' */
/* macro to be used by other macros ..  */
#define ___INTERNAL_BOBK_PORTS_HIGH_PORTS__PORTS_STEP(portsStep , startAddr,numBytes,numSteps1,stepSize1) \
    /* ports 56..59 */                                                         \
    {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC((startAddr+BOBCAT2_PORT_56_START_OFFSET_CNS)   ,numBytes)}, \
        FORMULA_TWO_PARAMETERS((4/(portsStep)), (BOBCAT2_PORT_OFFSET_CNS*(portsStep)),numSteps1, stepSize1)}     \
    /* port 64..71 */                                                          \
    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC((startAddr+BOBCAT2_PORT_56_START_OFFSET_CNS + (BOBCAT2_PORT_OFFSET_CNS*(64-56)))   ,numBytes)}, \
        FORMULA_TWO_PARAMETERS((8/(portsStep)), (BOBCAT2_PORT_OFFSET_CNS*(portsStep)),numSteps1, stepSize1)}

/* will set ports 56..59,64..71 */
/* instead of calling:
            {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC((0x10180800 + 0x00200000)   ,0)},
                FORMULA_TWO_PARAMETERS(30, 0x4, 16, 0x00001000)},

    need to call:
    BOBK_PORTS_LOW_PORTS__SUPPORT_FORMULA_1_PARAMS(0x10180800 ,0,
        30, 0x4)
*/
#define BOBK_PORTS_HIGH_PORTS__BY_END_ADDR(startAddr,endAddr,numSteps1,stepSize1) \
    ___INTERNAL_BOBK_PORTS_HIGH_PORTS__PORTS_STEP(1 , startAddr,(((endAddr)-(startAddr)) + 4),numSteps1,stepSize1)

#define BOBK_PORTS_HIGH_PORTS__BY_NUM_BYTES(startAddr,numBytes,numSteps1,stepSize1) \
    ___INTERNAL_BOBK_PORTS_HIGH_PORTS__PORTS_STEP(1 , startAddr,numBytes,numSteps1,stepSize1)

/* set ports in step of 2 ports :
will set ports 56,58(range 56..59),64,66,68,70(range 64..71) */
#define BOBK_PORTS_HIGH_PORTS_STEP_2_PORTS__BY_END_ADDR(startAddr,endAddr,numSteps1,stepSize1) \
    ___INTERNAL_BOBK_PORTS_HIGH_PORTS__PORTS_STEP(2 , startAddr,(((endAddr)-(startAddr)) + 4),numSteps1,stepSize1)

#define BOBK_PORTS_HIGH_PORTS_STEP_2_PORTS__BY_NUM_BYTES(startAddr,numBytes,numSteps1,stepSize1) \
    ___INTERNAL_BOBK_PORTS_HIGH_PORTS__PORTS_STEP(2 , startAddr,numBytes,numSteps1,stepSize1)


/* will set GOPs 0..11 */
#define BOBK_PORTS_LOW_GOPS__BY_NUM_BYTES(startAddr,numBytes,numSteps1,stepSize1) \
    {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(startAddr   ,numBytes)}, \
        FORMULA_TWO_PARAMETERS(12, (BOBCAT2_PORT_OFFSET_CNS * 4),numSteps1, stepSize1)}

#define BOBK_PORTS_LOW_GOPS__BY_END_ADDR(startAddr,endAddr,numSteps1,stepSize1) \
    BOBK_PORTS_LOW_GOPS__BY_NUM_BYTES(startAddr,(((endAddr)-(startAddr)) + 4),numSteps1,stepSize1)


/* will set GOPS 14,16..17 */
#define BOBK_PORTS_HIGH_GOPS__BY_END_ADDR(startAddr,endAddr,numSteps1,stepSize1) \
    ___INTERNAL_BOBK_PORTS_HIGH_PORTS__PORTS_STEP(4 , startAddr,(((endAddr)-(startAddr)) + 4),numSteps1,stepSize1)

/* will set mib ports 0..47 */
/* instead of calling:
            {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00000000,0)}, FORMULA_TWO_PARAMETERS(128/4 , 0x4 , 56 , BOBCAT2_MIB_OFFSET_CNS)}

    need to call:
    BOBK_PORTS_LOW_MIB_PORTS__BY_END_ADDR(0x00000000 ,0,
        128/4, 0x4)
*/
#define BOBK_PORTS_LOW_MIB_PORTS__BY_END_ADDR(startAddr,endAddr,numSteps1,stepSize1) \
    {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(startAddr   ,(((endAddr)-(startAddr)) + 4))},                            \
        FORMULA_TWO_PARAMETERS(48, BOBCAT2_MIB_OFFSET_CNS,numSteps1, stepSize1)}
/* will set mib ports 56..59,64..71 */
/* instead of calling:
            {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00000000 +BOBCAT2_MIB_PORT_56_START_OFFSET_CNS,0)}, FORMULA_TWO_PARAMETERS(128/4 , 0x4 , 16 , BOBCAT2_MIB_OFFSET_CNS)}

    need to call:
    BOBK_PORTS_HIGH_MIB_PORTS__BY_END_ADDR((0x00000000 ) ,0,
        128/4, 0x4)
*/
#define BOBK_PORTS_HIGH_MIB_PORTS__BY_END_ADDR(startAddr,endAddr,numSteps1,stepSize1)\
    /* mib ports 56..59 */                                                         \
    {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC((startAddr+BOBCAT2_MIB_PORT_56_START_OFFSET_CNS)   ,(((endAddr)-(startAddr)) + 4))}, \
        FORMULA_TWO_PARAMETERS(4, BOBCAT2_MIB_OFFSET_CNS,numSteps1, stepSize1)}            \
    /* mib ports 64..71 */                                                          \
    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(((startAddr+BOBCAT2_MIB_PORT_56_START_OFFSET_CNS) + (BOBCAT2_MIB_OFFSET_CNS*(64-56)))   ,(((endAddr)-(startAddr)) + 4))},\
        FORMULA_TWO_PARAMETERS(8, BOBCAT2_MIB_OFFSET_CNS,numSteps1, stepSize1)}

typedef enum{
    /* units that exists in bobcat2 */
    BOBK_UNIT_MG_E,
    BOBK_UNIT_IPLR1_E,
    BOBK_UNIT_TTI_E,
    BOBK_UNIT_L2I_E,
    BOBK_UNIT_IPVX_E,
    BOBK_UNIT_BM_E,
    BOBK_UNIT_BMA_E,
    BOBK_UNIT_FDB_E,
    BOBK_UNIT_EPLR_E,
    BOBK_UNIT_MPPM_E,
    BOBK_UNIT_GOP_E,
    BOBK_UNIT_XG_PORT_MIB_E,
    BOBK_UNIT_SERDES_E,
    BOBK_UNIT_RX_DMA_E,
    BOBK_UNIT_TX_DMA_E,
    BOBK_UNIT_TXQ_QUEUE_E,
    BOBK_UNIT_TXQ_LL_E,
    BOBK_UNIT_TXQ_DQ_E,
    BOBK_UNIT_CPFC_E,
    BOBK_UNIT_MLL_E,
    BOBK_UNIT_IOAM_E,
    BOBK_UNIT_EOAM_E,
    BOBK_UNIT_LPM_E,
    BOBK_UNIT_EQ_0_E,/*part 0 of EQ*/
    BOBK_UNIT_EQ_1_E,/*part 1 of EQ*/
    BOBK_UNIT_FDB_TABLE_0_E,
    BOBK_UNIT_FDB_TABLE_1_E,
    BOBK_UNIT_EGF_EFT_E,
    BOBK_UNIT_EGF_EFT_1_E,
    BOBK_UNIT_EGF_QAG_E,
    BOBK_UNIT_EGF_QAG_1_E,
    BOBK_UNIT_EGF_SHT_E,
    BOBK_UNIT_EGF_SHT_1_E,
    BOBK_UNIT_EGF_SHT_2_E,
    BOBK_UNIT_EGF_SHT_3_E,
    BOBK_UNIT_EGF_SHT_4_E,
    BOBK_UNIT_EGF_SHT_5_E, /*!!! EMPTY !!!*/
    BOBK_UNIT_EGF_SHT_6_E,
    BOBK_UNIT_EGF_SHT_7_E, /*!!! EMPTY !!!*/
    BOBK_UNIT_EGF_SHT_8_E,
    BOBK_UNIT_EGF_SHT_9_E,
    BOBK_UNIT_EGF_SHT_10_E,
    BOBK_UNIT_EGF_SHT_11_E,
    BOBK_UNIT_EGF_SHT_12_E,
    BOBK_UNIT_HA_E,
    BOBK_UNIT_IPCL_E,
    BOBK_UNIT_IPLR0_E,
    BOBK_UNIT_EPCL_E,
    BOBK_UNIT_TCAM_E,
    BOBK_UNIT_TM_HOSTING_E,
    BOBK_UNIT_ERMRK_E,
    BOBK_UNIT_CNC_0_E,
    BOBK_UNIT_CNC_1_E,
    BOBK_UNIT_TMDROP_E,
    BOBK_UNIT_TXQ_QCN_E,
    BOBK_UNIT_TX_FIFO_E,
    BOBK_UNIT_ETH_TX_FIFO_E,
    BOBK_UNIT_TM_FCU_E,
    BOBK_UNIT_TM_INGRESS_GLUE_E,
    BOBK_UNIT_TM_EGRESS_GLUE_E,
    BOBK_UNIT_TMQMAP_E,
    /* units that are new in bobk */
    BOBK_UNIT_RX_DMA_1_E,
    BOBK_UNIT_TX_DMA_1_E,
    BOBK_UNIT_TX_FIFO_1_E,
    BOBK_UNIT_ETH_TX_FIFO_1_E,
    BOBK_UNIT_RX_DMA_GLUE_E,
    BOBK_UNIT_TX_DMA_GLUE_E,
    BOBK_UNIT_TAI_E,
    BOBK_UNIT_GOP_SMI_0_E,
    BOBK_UNIT_GOP_SMI_1_E,
    BOBK_UNIT_GOP_SMI_2_E,
    BOBK_UNIT_GOP_SMI_3_E,
    BOBK_UNIT_GOP_LED_0_E,
    BOBK_UNIT_GOP_LED_1_E,
    BOBK_UNIT_GOP_LED_2_E,
    BOBK_UNIT_GOP_LED_3_E,
    BOBK_UNIT_GOP_LED_4_E,

    BOBK_UNIT_LAST_E
}SMEM_BOBK_UNIT_NAME_ENT;

static SMEM_UNIT_NAME_AND_INDEX_STC bobkUnitNameAndIndexArr[]=
{
    /* units that exists in bobcat2 */
    {STR(UNIT_MG),                         BOBK_UNIT_MG_E                    },
    {STR(UNIT_IPLR1),                      BOBK_UNIT_IPLR1_E                 },
    {STR(UNIT_TTI),                        BOBK_UNIT_TTI_E                   },
    {STR(UNIT_L2I),                        BOBK_UNIT_L2I_E                   },
    {STR(UNIT_IPVX),                       BOBK_UNIT_IPVX_E                  },
    {STR(UNIT_BM),                         BOBK_UNIT_BM_E                    },
    {STR(UNIT_BMA),                        BOBK_UNIT_BMA_E                   },
    {STR(UNIT_FDB),                        BOBK_UNIT_FDB_E                   },
    {STR(UNIT_MPPM),                       BOBK_UNIT_MPPM_E                  },
    {STR(UNIT_EPLR),                       BOBK_UNIT_EPLR_E                  },
    {STR(UNIT_CNC),                        BOBK_UNIT_CNC_0_E                 },
    {STR(UNIT_GOP),                        BOBK_UNIT_GOP_E                   },
    {STR(UNIT_XG_PORT_MIB),                BOBK_UNIT_XG_PORT_MIB_E           },
    {STR(UNIT_SERDES),                     BOBK_UNIT_SERDES_E                },
    {STR(UNIT_EQ),                         BOBK_UNIT_EQ_0_E                  },
    {STR(UNIT_IPCL),                       BOBK_UNIT_IPCL_E                  },
    {STR(UNIT_IPLR),                       BOBK_UNIT_IPLR0_E                 },
    {STR(UNIT_MLL),                        BOBK_UNIT_MLL_E                   },
    {STR(UNIT_EPCL),                       BOBK_UNIT_EPCL_E                  },
    {STR(UNIT_HA),                         BOBK_UNIT_HA_E                    },
    {STR(UNIT_RX_DMA),                     BOBK_UNIT_RX_DMA_E                },
    {STR(UNIT_TX_DMA),                     BOBK_UNIT_TX_DMA_E                },

    {STR(UNIT_IOAM),                       BOBK_UNIT_IOAM_E          },
    {STR(UNIT_EOAM),                       BOBK_UNIT_EOAM_E          },
    {STR(UNIT_LPM),                        BOBK_UNIT_LPM_E           },
    {STR(UNIT_EQ_1),                       BOBK_UNIT_EQ_1_E          },
    {STR(UNIT_FDB_TABLE_0),                BOBK_UNIT_FDB_TABLE_1_E   },
    {STR(UNIT_FDB_TABLE_1),                BOBK_UNIT_FDB_TABLE_1_E   },
    {STR(UNIT_EGF_EFT),                  BOBK_UNIT_EGF_EFT_E     },
    {STR(UNIT_EGF_EFT_1),                  BOBK_UNIT_EGF_EFT_1_E     },
    {STR(UNIT_EGF_QAG),                  BOBK_UNIT_EGF_QAG_E     },
    {STR(UNIT_EGF_QAG_1),                  BOBK_UNIT_EGF_QAG_1_E     },
    {STR(UNIT_EGF_SHT),                  BOBK_UNIT_EGF_SHT_E     },
    {STR(UNIT_EGF_SHT_1),                  BOBK_UNIT_EGF_SHT_1_E     },
    {STR(UNIT_EGF_SHT_2),                  BOBK_UNIT_EGF_SHT_2_E     },
    {STR(UNIT_EGF_SHT_3),                  BOBK_UNIT_EGF_SHT_3_E     },
    {STR(UNIT_EGF_SHT_4),                  BOBK_UNIT_EGF_SHT_4_E     },
    {STR(UNIT_EGF_SHT_5),                  BOBK_UNIT_EGF_SHT_5_E     },
    {STR(UNIT_EGF_SHT_6),                  BOBK_UNIT_EGF_SHT_6_E     },
    {STR(UNIT_EGF_SHT_7),                  BOBK_UNIT_EGF_SHT_7_E     },
    {STR(UNIT_EGF_SHT_8),                  BOBK_UNIT_EGF_SHT_8_E     },
    {STR(UNIT_EGF_SHT_9),                  BOBK_UNIT_EGF_SHT_9_E     },
    {STR(UNIT_EGF_SHT_10),                 BOBK_UNIT_EGF_SHT_10_E    },
    {STR(UNIT_EGF_SHT_11),                 BOBK_UNIT_EGF_SHT_11_E    },
    {STR(UNIT_EGF_SHT_12),                 BOBK_UNIT_EGF_SHT_12_E    },
    {STR(UNIT_TCAM),                       BOBK_UNIT_TCAM_E          },
    {STR(UNIT_TM_HOSTING),                 BOBK_UNIT_TM_HOSTING_E    },
    {STR(UNIT_ERMRK),                      BOBK_UNIT_ERMRK_E         },
    {STR(UNIT_CNC_1),                      BOBK_UNIT_CNC_1_E         },

    {STR(UNIT_TMDROP),                     BOBK_UNIT_TMDROP_E        },

    {STR(UNIT_TXQ_QUEUE),                  BOBK_UNIT_TXQ_QUEUE_E     },
    {STR(UNIT_TXQ_LL),                     BOBK_UNIT_TXQ_LL_E        },
    {STR(UNIT_TXQ_DQ),                     BOBK_UNIT_TXQ_DQ_E        },
    {STR(UNIT_CPFC),                       BOBK_UNIT_CPFC_E          },
    {STR(UNIT_TXQ_QCN),                    BOBK_UNIT_TXQ_QCN_E       },

    {STR(UNIT_TX_FIFO),                    BOBK_UNIT_TX_FIFO_E       },
    {STR(UNIT_ETH_TX_FIFO),                BOBK_UNIT_ETH_TX_FIFO_E   },

    {STR(UNIT_TM_FCU),                     BOBK_UNIT_TM_FCU_E         },
    {STR(UNIT_TM_INGRESS_GLUE),            BOBK_UNIT_TM_INGRESS_GLUE_E},
    {STR(UNIT_TM_EGRESS_GLUE),             BOBK_UNIT_TM_EGRESS_GLUE_E },
    {STR(UNIT_TMQMAP),                     BOBK_UNIT_TMQMAP_E         },
    /* units that are new in bobk */
    {STR(UNIT_RX_DMA_1),                   BOBK_UNIT_RX_DMA_1_E       },
    {STR(UNIT_TX_DMA_1),                   BOBK_UNIT_TX_DMA_1_E       },
    {STR(UNIT_TX_FIFO_1),                  BOBK_UNIT_TX_FIFO_1_E      },
    {STR(UNIT_ETH_TX_FIFO_1),              BOBK_UNIT_ETH_TX_FIFO_1_E  },
    {STR(UNIT_RX_DMA_GLUE),                BOBK_UNIT_RX_DMA_GLUE_E    },
    {STR(UNIT_TX_DMA_GLUE),                BOBK_UNIT_TX_DMA_GLUE_E    },
    {STR(UNIT_TAI),                        BOBK_UNIT_TAI_E            },
    {STR(UNIT_GOP_SMI_0),                  BOBK_UNIT_GOP_SMI_0_E      },
    {STR(UNIT_GOP_SMI_1),                  BOBK_UNIT_GOP_SMI_1_E      },
    {STR(UNIT_GOP_SMI_2),                  BOBK_UNIT_GOP_SMI_2_E      },
    {STR(UNIT_GOP_SMI_3),                  BOBK_UNIT_GOP_SMI_3_E      },
    {STR(UNIT_GOP_LED_0),                  BOBK_UNIT_GOP_LED_0_E      },
    {STR(UNIT_GOP_LED_1),                  BOBK_UNIT_GOP_LED_1_E      },
    {STR(UNIT_GOP_LED_2),                  BOBK_UNIT_GOP_LED_2_E      },
    {STR(UNIT_GOP_LED_3),                  BOBK_UNIT_GOP_LED_3_E      },
    {STR(UNIT_GOP_LED_4),                  BOBK_UNIT_GOP_LED_4_E      },

    {NULL ,                                SMAIN_NOT_VALID_CNS                }
};
/* the addresses of the units that the bobk uses */
static GT_U32   bobkUsedUnitsAddressesArray[BOBK_UNIT_LAST_E]=
{
    /* units that exists in bobcat2 */
     BOBCAT2_MG_UNIT_BASE_ADDR_CNS                  /*BOBK_UNIT_MG_E,           */
    ,BOBCAT2_IPLR1_UNIT_BASE_ADDR_CNS               /*BOBK_UNIT_IPLR1_E,        */
    ,BOBCAT2_TTI_UNIT_BASE_ADDR_CNS                 /*BOBK_UNIT_TTI_E,          */
    ,BOBCAT2_L2I_UNIT_BASE_ADDR_CNS                 /*BOBK_UNIT_L2I_E,          */
    ,BOBCAT2_IPVX_UNIT_BASE_ADDR_CNS                /*BOBK_UNIT_IPVX_E,         */
    ,BOBCAT2_BM_UNIT_BASE_ADDR_CNS                  /*BOBK_UNIT_BM_E,           */
    ,BOBCAT2_BMA_UNIT_BASE_ADDR_CNS                 /*BOBK_UNIT_BMA_E,          */
    ,BOBCAT2_FDB_UNIT_BASE_ADDR_CNS                 /*BOBK_UNIT_FDB_E,          */
    ,BOBCAT2_EPLR_UNIT_BASE_ADDR_CNS                /*BOBK_UNIT_EPLR_E,         */
    ,BOBCAT2_MPPM_UNIT_BASE_ADDR_CNS                /*BOBK_UNIT_MPPM_E,         */
    ,BOBCAT2_GOP_UNIT_BASE_ADDR_CNS                 /*BOBK_UNIT_GOP_E,          */
    ,BOBCAT2_MIB_UNIT_BASE_ADDR_CNS                 /*BOBK_UNIT_XG_PORT_MIB_E,  */
    ,BOBCAT2_SERDES_UNIT_BASE_ADDR_CNS              /*BOBK_UNIT_SERDES_E,       */
    ,BOBCAT2_RX_DMA_UNIT_BASE_ADDR_CNS              /*BOBK_UNIT_RX_DMA_E,       */
    ,BOBCAT2_TX_DMA_UNIT_BASE_ADDR_CNS              /*BOBK_UNIT_TX_DMA_E,       */
    ,BOBCAT2_TXQ_QUEUE_UNIT_BASE_ADDR_CNS           /*BOBK_UNIT_TXQ_QUEUE_E,    */
    ,BOBCAT2_TXQ_LL_UNIT_BASE_ADDR_CNS              /*BOBK_UNIT_TXQ_LL_E,       */
    ,BOBCAT2_TXQ_DQ_UNIT_BASE_ADDR_CNS              /*BOBK_UNIT_TXQ_DQ_E,       */
    ,BOBCAT2_CPFC_UNIT_BASE_ADDR_CNS                /*BOBK_UNIT_CPFC_E,         */
    ,BOBCAT2_MLL_UNIT_BASE_ADDR_CNS                 /*BOBK_UNIT_MLL_E,          */
    ,BOBCAT2_IOAM_UNIT_BASE_ADDR_CNS                /*BOBK_UNIT_IOAM_E,         */
    ,BOBCAT2_EOAM_UNIT_BASE_ADDR_CNS                /*BOBK_UNIT_EOAM_E,         */
    ,BOBCAT2_LPM_UNIT_BASE_ADDR_CNS                 /*BOBK_UNIT_LPM_E,          */
    ,BOBCAT2_EQ_UNIT_BASE_ADDR_CNS(0)               /*BOBK_UNIT_EQ_0_E,         */
    ,BOBCAT2_EQ_UNIT_BASE_ADDR_CNS(1)               /*BOBK_UNIT_EQ_1_E,         */
    ,BOBCAT2_FDB_TABLE_BASE_ADDR_CNS(0)             /*BOBK_UNIT_FDB_TABLE_0_E,  */
    ,BOBCAT2_FDB_TABLE_BASE_ADDR_CNS(1)             /*BOBK_UNIT_FDB_TABLE_1_E,  */
    ,BOBCAT2_EGF_EFT_UNIT_BASE_ADDR_CNS(0 )         /*BOBK_UNIT_EGF_EFT_E,    */
    ,BOBCAT2_EGF_EFT_UNIT_BASE_ADDR_CNS(1 )         /*BOBK_UNIT_EGF_EFT_1_E,    */
    ,BOBCAT2_EGF_QAG_UNIT_BASE_ADDR_CNS(0 )         /*BOBK_UNIT_EGF_QAG_E,    */
    ,BOBCAT2_EGF_QAG_UNIT_BASE_ADDR_CNS(1 )         /*BOBK_UNIT_EGF_QAG_1_E,    */
    ,BOBCAT2_EGF_SHT_UNIT_BASE_ADDR_CNS(0 )         /*BOBK_UNIT_EGF_SHT_E,    */
    ,BOBCAT2_EGF_SHT_UNIT_BASE_ADDR_CNS(1 )         /*BOBK_UNIT_EGF_SHT_1_E,    */
    ,BOBCAT2_EGF_SHT_UNIT_BASE_ADDR_CNS(2 )         /*BOBK_UNIT_EGF_SHT_2_E,    */
    ,BOBCAT2_EGF_SHT_UNIT_BASE_ADDR_CNS(3 )         /*BOBK_UNIT_EGF_SHT_3_E,    */
    ,BOBCAT2_EGF_SHT_UNIT_BASE_ADDR_CNS(4 )         /*BOBK_UNIT_EGF_SHT_4_E,    */
    ,BOBCAT2_EGF_SHT_UNIT_BASE_ADDR_CNS(5 )         /*BOBK_UNIT_EGF_SHT_5_E,    *//*!!! EMPTY !!!*/
    ,BOBCAT2_EGF_SHT_UNIT_BASE_ADDR_CNS(6 )         /*BOBK_UNIT_EGF_SHT_6_E,    */
    ,BOBCAT2_EGF_SHT_UNIT_BASE_ADDR_CNS(7 )         /*BOBK_UNIT_EGF_SHT_7_E,    *//*!!! EMPTY !!!*/
    ,BOBCAT2_EGF_SHT_UNIT_BASE_ADDR_CNS(8 )         /*BOBK_UNIT_EGF_SHT_8_E,    */
    ,BOBCAT2_EGF_SHT_UNIT_BASE_ADDR_CNS(9 )         /*BOBK_UNIT_EGF_SHT_9_E,    */
    ,BOBCAT2_EGF_SHT_UNIT_BASE_ADDR_CNS(10)         /*BOBK_UNIT_EGF_SHT_10_E,   */
    ,BOBCAT2_EGF_SHT_UNIT_BASE_ADDR_CNS(11)         /*BOBK_UNIT_EGF_SHT_11_E,   */
    ,BOBCAT2_EGF_SHT_UNIT_BASE_ADDR_CNS(12)         /*BOBK_UNIT_EGF_SHT_12_E,   */
    ,BOBCAT2_HA_UNIT_BASE_ADDR_CNS(0)               /*BOBK_UNIT_HA_E,           */
    ,BOBCAT2_IPCL_UNIT_BASE_ADDR_CNS                /*BOBK_UNIT_IPCL_E,         */
    ,BOBCAT2_IPLR0_UNIT_BASE_ADDR_CNS               /*BOBK_UNIT_IPLR0_E,         */
    ,BOBCAT2_EPCL_UNIT_BASE_ADDR_CNS                /*BOBK_UNIT_EPCL_E,         */
    ,BOBCAT2_TCAM_UNIT_BASE_ADDR_CNS                /*BOBK_UNIT_TCAM_E,         */
    ,BOBCAT2_TM_HOSTING_UNIT_BASE_ADDR_CNS          /*BOBK_UNIT_TM_HOSTING_E,   */
    ,BOBCAT2_ERMRK_UNIT_BASE_ADDR_CNS               /*BOBK_UNIT_ERMRK_E,        */
    ,BOBCAT2_CNC_0_UNIT_BASE_ADDR_CNS               /*BOBK_UNIT_CNC_0_E,        */
    ,BOBCAT2_CNC_1_UNIT_BASE_ADDR_CNS               /*BOBK_UNIT_CNC_1_E,        */
    ,BOBCAT2_TMDROP_UNIT_BASE_ADDR_CNS              /*BOBK_UNIT_TMDROP_E,       */
    ,BOBCAT2_TXQ_QCN_UNIT_BASE_ADDR_CNS             /*BOBK_UNIT_TXQ_QCN_E,      */
    ,BOBCAT2_TX_FIFO_UNIT_BASE_ADDR_CNS             /*BOBK_UNIT_TX_FIFO_E,      */
    ,BOBCAT2_ETH_TX_FIFO_UNIT_BASE_ADDR_CNS         /*BOBK_UNIT_ETH_TX_FIFO_E,  */
    ,BOBCAT2_TM_FCU_UNIT_BASE_ADDR_CNS              /*BOBK_UNIT_TM_FCU_E,       */
    ,BOBCAT2_TM_INGRESS_GLUE_UNIT_BASE_ADDR_CNS     /*BOBK_UNIT_TM_INGRESS_GLUE_E,*/
    ,BOBCAT2_TM_EGRESS_GLUE_UNIT_BASE_ADDR_CNS      /*BOBK_UNIT_TM_EGRESS_GLUE_E, */
    ,BOBCAT2_TMQMAP_GLUE_UNIT_BASE_ADDR_CNS         /*BOBK_UNIT_TMQMAP_E,       */
     /* units that are new in bobk */
    ,BOBK_RX_DMA_1_UNIT_BASE_ADDR_CNS               /*BOBK_UNIT_RX_DMA_1_E,     */
    ,BOBK_TX_DMA_1_UNIT_BASE_ADDR_CNS               /*BOBK_UNIT_TX_DMA_1_E,     */
    ,BOBK_TX_FIFO_1_UNIT_BASE_ADDR_CNS              /*BOBK_UNIT_TX_FIFO_1_E,    */
    ,BOBK_ETH_TX_FIFO_1_UNIT_BASE_ADDR_CNS          /*BOBK_UNIT_ETH_TX_FIFO_1_E,*/
    ,BOBK_RX_DMA_GLUE_UNIT_BASE_ADDR_CNS            /*BOBK_UNIT_RX_DMA_GLUE_E,  */
    ,BOBK_TX_DMA_GLUE_UNIT_BASE_ADDR_CNS            /*BOBK_UNIT_TX_DMA_GLUE_E,  */
    ,BOBK_TAI_UNIT_BASE_ADDR_CNS                    /*BOBK_UNIT_TAI_E,          */
    ,BOBK_GOP_SMI_UNIT_BASE_ADDR_CNS(0)             /*BOBK_UNIT_GOP_SMI_0_E,      */
    ,BOBK_GOP_SMI_UNIT_BASE_ADDR_CNS(1)             /*BOBK_UNIT_GOP_SMI_1_E,      */
    ,BOBK_GOP_SMI_UNIT_BASE_ADDR_CNS(2)             /*BOBK_UNIT_GOP_SMI_2_E,      */
    ,BOBK_GOP_SMI_UNIT_BASE_ADDR_CNS(3)             /*BOBK_UNIT_GOP_SMI_3_E,      */

    ,BOBK_GOP_LED_UNIT_BASE_ADDR_CNS(0)             /*BOBK_UNIT_GOP_LED_0_E,      */
    ,BOBK_GOP_LED_UNIT_BASE_ADDR_CNS(1)             /*BOBK_UNIT_GOP_LED_1_E,      */
    ,BOBK_GOP_LED_UNIT_BASE_ADDR_CNS(2)             /*BOBK_UNIT_GOP_LED_2_E,      */
    ,BOBK_GOP_LED_UNIT_BASE_ADDR_CNS(3)             /*BOBK_UNIT_GOP_LED_3_E,      */
    ,BOBK_GOP_LED_4_UNIT_BASE_ADDR_CNS              /*BOBK_UNIT_GOP_LED_4_E,      */

};
BUILD_STRING_FOR_UNIT_NAME(UNIT_MG);
BUILD_STRING_FOR_UNIT_NAME(UNIT_IPLR1);
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_TTI);                                */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_L2I);                                */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_IPVX);                               */
BUILD_STRING_FOR_UNIT_NAME(UNIT_BM);
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_BMA);                                */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_FDB);                                */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_MPPM);                               */
BUILD_STRING_FOR_UNIT_NAME(UNIT_EPLR);
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_CNC);                                */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_GOP);                                */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_XG_PORT_MIB);                        */
BUILD_STRING_FOR_UNIT_NAME(UNIT_SERDES);
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_EQ);                                 */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_IPCL);                               */
BUILD_STRING_FOR_UNIT_NAME(UNIT_IPLR);
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_MLL);                                */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_EPCL);                               */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_HA);                                 */
BUILD_STRING_FOR_UNIT_NAME(UNIT_RX_DMA);
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_TX_DMA);                             */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_IOAM);                               */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_EOAM);                               */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_LPM);                                */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_EQ_1);                               */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_FDB_TABLE_0);                        */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_FDB_TABLE_1);                        */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_EGF_EFT);                          */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_EGF_EFT_1);                          */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_EGF_QAG);                          */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_EGF_QAG_1);                          */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_EGF_SHT);                          */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_EGF_SHT_1);                          */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_EGF_SHT_2);                          */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_EGF_SHT_3);                          */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_EGF_SHT_4);                          */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_EGF_SHT_5);                          */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_EGF_SHT_6);                          */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_EGF_SHT_7);                          */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_EGF_SHT_8);                          */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_EGF_SHT_9);                          */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_EGF_SHT_10);                         */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_EGF_SHT_11);                         */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_EGF_SHT_12);                         */
BUILD_STRING_FOR_UNIT_NAME(UNIT_TCAM);
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_TM_HOSTING);                         */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_ERMRK);                              */
BUILD_STRING_FOR_UNIT_NAME(UNIT_GOP);
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_CNC_1);                              */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_TMDROP);                             */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_TXQ_QUEUE);                          */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_TXQ_LL);                             */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_TXQ_DQ);                             */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_CPFC);                               */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_TXQ_QCN);                            */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_TX_FIFO);                             */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_ETH_TX_FIFO);                        */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_TM_FCU);                             */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_TM_INGRESS_GLUE);                    */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_TM_EGRESS_GLUE);                     */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_TMQMAP);                             */
/* units that are new in bobk */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_RX_DMA_1);                           */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_TX_DMA_1);                           */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_TX_FIFO_1);                          */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_RX_DMA_GLUE);                        */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_TX_DMA_GLUE);                        */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_TAI);                                */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_GOP_SMI_0);                          */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_GOP_SMI_1);                          */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_GOP_SMI_2);                          */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_GOP_SMI_3);                          */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_GOP_LED_0);                          */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_GOP_LED_1);                          */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_GOP_LED_2);                          */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_GOP_LED_3);                          */
/*BUILD_STRING_FOR_UNIT_NAME(UNIT_GOP_LED_4);                          */



/*******************************************************************************
*   smemBobkUnitGop
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the bobk GOP unit
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
static void smemBobkUnitGop
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        /* ports 56..59 , 64..71 */
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
            /* PTP */
            BOBK_PORTS_HIGH_PORTS__BY_NUM_BYTES(0x00180800,0,31, 0x4)

            /* Mac-TG Generator */
            ,BOBK_PORTS_HIGH_PORTS__BY_NUM_BYTES(0x00180C00,0,52, 0x4)

            /* GIG */
            ,BOBK_PORTS_HIGH_PORTS__BY_END_ADDR(0x00000000, 0x00000094,0, 0)
            ,BOBK_PORTS_HIGH_PORTS__BY_END_ADDR(0x000000A0, 0x000000A4,0, 0)
            ,BOBK_PORTS_HIGH_PORTS__BY_END_ADDR(0x000000C0, 0x000000D8,0, 0)

            /* MPCS */
            ,BOBK_PORTS_HIGH_PORTS__BY_END_ADDR(0x00180008, 0x00180014, 0, 0 )
            ,BOBK_PORTS_HIGH_PORTS__BY_END_ADDR(0x00180030, 0x00180030, 0, 0 )
            ,BOBK_PORTS_HIGH_PORTS__BY_END_ADDR(0x0018003C, 0x001800C8, 0, 0 )
            ,BOBK_PORTS_HIGH_PORTS__BY_END_ADDR(0x001800D0, 0x00180120, 0, 0 )
            ,BOBK_PORTS_HIGH_PORTS__BY_END_ADDR(0x00180128, 0x0018014C, 0, 0 )
            ,BOBK_PORTS_HIGH_PORTS__BY_END_ADDR(0x0018015C, 0x0018017C, 0, 0 )
            ,BOBK_PORTS_HIGH_PORTS__BY_NUM_BYTES(0x00180200, 256, 0, 0 )

            /* XPCS IP */
            /* ports in steps of 2 */
            ,BOBK_PORTS_HIGH_PORTS_STEP_2_PORTS__BY_END_ADDR(0x00180400,0x00180424,0,0)
            ,BOBK_PORTS_HIGH_PORTS_STEP_2_PORTS__BY_END_ADDR(0x0018042C,0x0018044C,0,0)
            /* 6 lanes */
            ,BOBK_PORTS_HIGH_PORTS_STEP_2_PORTS__BY_NUM_BYTES(0x00180450,0x0044, 6 , 0x44)

            /* XLG */
            ,BOBK_PORTS_HIGH_PORTS__BY_END_ADDR(0x000c0000,0x000c0024, 0, 0 )
            ,BOBK_PORTS_HIGH_PORTS__BY_END_ADDR(0x000c002C,0x000c0060, 0, 0 )
            ,BOBK_PORTS_HIGH_PORTS__BY_END_ADDR(0x000c0068,0x000c0088, 0, 0 )
            ,BOBK_PORTS_HIGH_PORTS__BY_END_ADDR(0x000c0090,0x000c0094, 0, 0 )

            /*FCA*/
            ,BOBK_PORTS_HIGH_PORTS__BY_END_ADDR(0x00180600, 0x00180718,0, 0)

        };

        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);
    }

    if (IS_PORT_0_EXISTS(devObjPtr))
    {
        /* ports 0..47 */
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
            /* PTP */
            BOBK_PORTS_LOW_PORTS__BY_NUM_BYTES(0x00180800,0,31, 0x4)

            /* Mac-TG Generator */
            ,BOBK_PORTS_LOW_GOPS__BY_NUM_BYTES(0x00180C00,0,52, 0x4)

            /* GIG */
            ,BOBK_PORTS_LOW_PORTS__BY_END_ADDR(0x00000000, 0x00000094,0, 0)
            ,BOBK_PORTS_LOW_PORTS__BY_END_ADDR(0x000000A0, 0x000000A4,0, 0)
            ,BOBK_PORTS_LOW_PORTS__BY_END_ADDR(0x000000C0, 0x000000D8,0, 0)

            /* MPCS */
            /* no mpcs for ports 0..47 */

            /* XLG */
            /* no XLG for ports 0..47 */

            /*FCA*/
            ,BOBK_PORTS_LOW_PORTS__BY_END_ADDR(0x00180600, 0x00180718,0, 0)

        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);
    }
}

/*******************************************************************************
*   smemBobkUnitLpSerdes
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the bobk  LpSerdes unit
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
static void smemBobkUnitLpSerdes
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    if(!SMEM_CHT_IS_SIP5_20_GET(devObjPtr))
    {
        if (IS_PORT_0_EXISTS(devObjPtr))
        {
            SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
            {
                /* serdes external registers SERDESes for ports 0...47 */
                 {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000000, 0x44)} , FORMULA_SINGLE_PARAMETER(12 , 0x1000)}
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00040000 + 20 * 0x1000, 0x44 )} , FORMULA_SINGLE_PARAMETER(1 , 0)/*single group*/}
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00040000 + 24 * 0x1000, 0x44 )} , FORMULA_SINGLE_PARAMETER(12 , 0x1000)}
            };
            GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);

            smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, unitPtr);
        }
        else
        {
            SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
            {
                /* reduced silicon */
                 {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00040000 + 20 * 0x1000, 0x44 )} , FORMULA_SINGLE_PARAMETER(1 , 0)/*single group*/}
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00040000 + 24 * 0x1000, 0x44 )} , FORMULA_SINGLE_PARAMETER(12 , 0x1000)}
            };
            GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);

            smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, unitPtr);
        }

    }
}

/*******************************************************************************
*   smemBobkUnitXGPortMib
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  XGPortMib unit
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
static void smemBobkUnitXGPortMib
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        /* steps between each port */
        devObjPtr->xgCountersStepPerPort   = BOBCAT2_MIB_OFFSET_CNS;
        devObjPtr->xgCountersStepPerPort_1 = BOBCAT2_MIB_OFFSET_CNS;
        /* offset ot table xgPortMibCounters_1 */
        devObjPtr->offsetToXgCounters_1 = BOBCAT2_MIB_PORT_56_START_OFFSET_CNS;
        devObjPtr->startPortNumInXgCounters_1 = 56;

        if (0 == IS_PORT_0_EXISTS(devObjPtr))
        {
            devObjPtr->tablesInfo.xgPortMibCounters.commonInfo.firstValidAddress =
                UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_XG_PORT_MIB) +
                devObjPtr->offsetToXgCounters_1;
        }


        unitPtr->chunkType = SMEM_UNIT_CHUNK_TYPE_8_MSB_E;

        /* chunks with formulas */
        {
            SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
            {
                /* will set mib ports 56..59,64..71 */
                BOBK_PORTS_HIGH_MIB_PORTS__BY_END_ADDR(0x00000000 ,0,128/4 , 0x4)
            };
            GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);

            smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, unitPtr);
        }

        if (IS_PORT_0_EXISTS(devObjPtr))
        {
            SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
            {
                /* will set mib ports 0..47 */
                BOBK_PORTS_LOW_MIB_PORTS__BY_END_ADDR(0x00000000 ,0,128/4 , 0x4)
            };
            GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
            SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

            smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

            /*add the tmp unit chunks to the main unit */
            smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);
        }
    }

    {
        SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = devObjPtr->deviceMemory;
        GT_U32  unitIndex;
        /* bobk with many ports */
        unitIndex = UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_XG_PORT_MIB);
        devMemInfoPtr->unitMemArr[unitIndex+ 0].hugeUnitSupportPtr =
        devMemInfoPtr->unitMemArr[unitIndex+ 1].hugeUnitSupportPtr =
            &devMemInfoPtr->unitMemArr[unitIndex];
    }
}

/*******************************************************************************
*   smemBobkUnitMppm
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Bobk MPPM unit
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
static void smemBobkUnitMppm
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {   /*bobcat2*/
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000120, 0x00000124)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000150, 0x00000154)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000001F0, 0x000001F0)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000200, 0x00000378)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000400, 0x00000400)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000500, 0x00000500)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000510, 0x00000518)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000600, 0x00000600)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000610, 0x00000614)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000620, 0x00000624)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000710, 0x00000714)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000800, 0x00000804)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000810, 0x00000814)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000900, 0x00000904)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000910, 0x00000914)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000A00, 0x00000A04)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000B00, 0x00000B04)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000C00, 0x00000C04)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000D00, 0x00000D04)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000E00, 0x00000E04)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000E30, 0x00000E34)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001000, 0x00001004)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001050, 0x00001054)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001100, 0x00001104)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001150, 0x00001154)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001200, 0x00001204)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002000, 0x0000200C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000202C, 0x00002030)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002080, 0x00002084)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000020B0, 0x000020B4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002120, 0x00002124)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002170, 0x00002174)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000021B0, 0x000021B4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000021C0, 0x000021C4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003000, 0x00003004)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003030, 0x00003034)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003200, 0x00003204)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003230, 0x00003234)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003400, 0x00003408)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003420, 0x00003420)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003480, 0x00003480)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000034E0, 0x000034E0)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003540, 0x00003540)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003580, 0x00003580)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000035E0, 0x000035E0)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003640, 0x00003640)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00004000, 0x00004000)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
}

/*******************************************************************************
*   smemBobkUnitRxDma
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Bobk  RX DMA unit
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
static void smemBobkUnitRxDma
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
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000000B0, 0x000000B4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000000C4, 0x000000D0)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000000E4, 0x00000160)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000174, 0x00000180)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000198, 0x00000294)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000002F0, 0x000002FC)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000320, 0x0000032C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000350, 0x00000350)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000374, 0x00000374)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000398, 0x000003AC)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000003C8, 0x000003D4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000003E8, 0x000003F4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000408, 0x00000414)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000428, 0x00000434)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000043C, 0x00000448)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000450, 0x0000045C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000004D0, 0x000005E0)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000630, 0x0000063C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000710, 0x00000714)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000720, 0x00000724)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000730, 0x00000734)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000740, 0x00000740)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000750, 0x00000750)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000804, 0x00000924)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000950, 0x00000A70)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000B00, 0x00000C20)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000E00, 0x00000F28)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000120C, 0x0000132C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001528, 0x0000154C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001704, 0x0000170C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001720, 0x00001744)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001790, 0x00001790)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001800, 0x00001800)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001820, 0x00001820)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001830, 0x00001834)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001840, 0x00001840)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001850, 0x0000185C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001864, 0x00001868)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001870, 0x00001870)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001B00, 0x00001C20)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001E00, 0x00001F20)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000020F0, 0x000020F0)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002100, 0x00002220)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002400, 0x00002520)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002A00, 0x00002B58)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002B60, 0x00002B60)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003500, 0x00003620)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003A00, 0x00003B20)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00004080, 0x00004084)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000040A0, 0x000040A4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000040C0, 0x000040C0)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00005000, 0x00005000)}
        };

        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
}

/*******************************************************************************
*   smemBobkUnitRxDmaGlue
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Bobk  RX DMA Glue unit
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
static void smemBobkUnitRxDmaGlue
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000000, 0x00000098)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000160, 0x00000160)}
        };

        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
}

/*******************************************************************************
*   smemBobkUnitTxDmaGlue
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Bobk  TX DMA Glue unit
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
static void smemBobkUnitTxDmaGlue
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000000, 0x00000098)}
        };

        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
}

/*******************************************************************************
*   smemBobkUnitTai
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Bobk  TAI unit
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
static void smemBobkUnitTai
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000000, 0x00000100)}
        };

        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
}

/*******************************************************************************
*   smemBobkUnitMg
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Bobk MG unit
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
static void smemBobkUnitMg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
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
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000190, 0x00000198)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000001A0, 0x00000288)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000290, 0x00000298)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000002A0, 0x000002A8)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000002B0, 0x000002B8)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000002C0, 0x000002C8)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000002D0, 0x00000324)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000330, 0x00000330)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000338, 0x00000338)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000340, 0x00000344)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000360, 24)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000380, 0x00000388)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000390, 0x00000390)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000003A0, 0x000003C0)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000003F0, 0x000003FC)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000500, 0x00000540)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000600, 0x00000604)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000610, 0x00000654)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000670, 0x000006B4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000830, 0x00000830)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000b64, 0x00000b64)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000b78, 0x00000b7c)}
            /* sdma */
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
            /*xsmi*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00030000, 8192)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00032000, 8192)}
            /* conf processor */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00040000, 131072)}
            /*TWSI*/
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
*   smemBobkUnitFdbTable
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the bobk FDB TABLE unit
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
static void smemBobkUnitFdbTable
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
#define  fdbEntryNumBits        138
#define  fdbEntryNumBytes_align 32

    /* single instance of the unit --- in core 0 */
    SINGLE_INSTANCE_UNIT_MAC(devObjPtr,LION3_UNIT_FDB_TABLE_SINGLE_INSTANCE_PORT_GROUP_CNS);

    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            /*FDB table*/
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0 ,INIT_INI_RUNTIME), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(fdbEntryNumBits, fdbEntryNumBytes_align),SMEM_BIND_TABLE_MAC(fdb)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);
        /* update the number of entries in the FDB */
        chunksMem[0].numOfRegisters = devObjPtr->fdbMaxNumEntries*fdbEntryNumBytes_align;

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
}

/*******************************************************************************
*   smemBobkUnitTxDma
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the bobk  TX DMA unit
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
static void smemBobkUnitTxDma
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000000, 0x00000014)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000020, 0x0000011C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000200, 0x00000248)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000500, 0x00000504)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001000, 0x00001014)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001020, 0x00001024)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002000, 0x00002044)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003000, 0x0000393C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00004000, 0x00004158)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00004160, 0x00004160)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00005000, 0x00005014)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000501C, 0x00005078)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00005080, 0x0000508C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00005100, 0x00005104)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00005120, 0x00005124)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00005200, 0x00005324)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00005500, 0x00005624)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00005700, 0x00005824)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00005900, 0x00005A24)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00006100, 0x00006104)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00006300, 0x00006304)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00007000, 0x00007004)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00007100, 0x00007224)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00007300, 0x00007300)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00007500, 0x00007504)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00007700, 0x00007700)}
        };

        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);
        smemInitMemChunk(devObjPtr,chunksMem, numOfChunks, unitPtr);
    }
}

/*******************************************************************************
*   smemBobkUnitTxFifo
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the bobk  TX Fifo unit
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
static void smemBobkUnitTxFifo
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000000, 0x0000012C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000400, 0x00000434)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000440, 0x00000444)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000508, 0x0000050C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000518, 0x0000051C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000600, 0x00000724)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000800, 0x00000958)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000960, 0x00000960)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001000, 0x00001024)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001100, 0x00001224)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001300, 0x00001424)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001500, 0x00001624)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001700, 0x00001824)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001900, 0x00001A24)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001B00, 0x00001C24)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003000, 0x00003004)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003100, 0x00003224)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003300, 0x00003300)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00010000, 1648)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00012000, 8)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00014000, 0x00014000)}
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
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00020000, 2256)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00022000, 4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00024000, 0x00024000)}
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
        smemInitMemChunk(devObjPtr,chunksMem, numOfChunks, unitPtr);
    }
}



/*******************************************************************************
*   smemBobkUnitBm
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the bobk BM unit
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
static void smemBobkUnitBm
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
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
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x000004E0, 0x000004E4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000500, 0x0000074C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00100000, 65536)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00120000, 65536)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00200000, 65536)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00220000, 65536)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00300000, 65536)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
}

/*******************************************************************************
*   smemBobkUnitBma
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the bobk BMA unit
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
static void smemBobkUnitBma
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000000, 65536)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00019000, 0x0001900C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00019014, 0x00019018)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00019020, 0x00019044)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0001904C, 0x00019058)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00019070, 0x00019070)}
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
*   smemBobkPolicerTablesSupport
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
void smemBobkPolicerTablesSupport
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32  numOfChunks,
    INOUT SMEM_CHUNK_BASIC_STC  chunksMem[],
    IN SMEM_SIP5_PP_PLR_UNIT_ENT   plrUnit
)
{
    GT_BIT  eplr  = (plrUnit == SMEM_SIP5_PP_PLR_UNIT_EPLR_E) ? 1 : 0;
    GT_BIT  iplr1 = (plrUnit == SMEM_SIP5_PP_PLR_UNIT_IPLR_1_E) ? 1 : 0;
    GT_U32  ii;
    GT_U32  numOfMeters = eplr ?
            devObjPtr->policerSupport.eplrTableSize :
            devObjPtr->policerSupport.iplrTableSize;
    GT_BIT  updateTableSizeAccordingToMeters;

    for(ii = 0 ; ii < numOfChunks ; ii ++)
    {
        updateTableSizeAccordingToMeters = 0;
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
                    updateTableSizeAccordingToMeters = 1;
                    break;
                case FIELD_OFFSET_IN_STC_MAC(policerCounters,SKERNEL_TABLES_INFO_STC):
                    chunksMem[ii].tableOffsetInBytes =
                        FIELD_OFFSET_IN_STC_MAC(egressPolicerCounters,SKERNEL_TABLES_INFO_STC);
                    updateTableSizeAccordingToMeters = 1;
                    break;
                case FIELD_OFFSET_IN_STC_MAC(policerConfig,SKERNEL_TABLES_INFO_STC):
                    chunksMem[ii].tableOffsetInBytes =
                        FIELD_OFFSET_IN_STC_MAC(egressPolicerConfig,SKERNEL_TABLES_INFO_STC);
                    updateTableSizeAccordingToMeters = 1;
                    break;
                case FIELD_OFFSET_IN_STC_MAC(policerConformanceLevelSign,SKERNEL_TABLES_INFO_STC):
                    chunksMem[ii].tableOffsetInBytes =
                        FIELD_OFFSET_IN_STC_MAC(egressPolicerConformanceLevelSign,SKERNEL_TABLES_INFO_STC);
                    updateTableSizeAccordingToMeters = 1;
                    break;
                default:
                    /* make sure that table are not bound to EPLR (only to iplr0) */
                    chunksMem[ii].tableOffsetValid = 0;
                    chunksMem[ii].tableOffsetInBytes = 0;
                    break;
            }
        }
        else /* iplr0 */
        {
            switch(chunksMem[ii].tableOffsetInBytes)
            {
                case FIELD_OFFSET_IN_STC_MAC(policer,SKERNEL_TABLES_INFO_STC):
                case FIELD_OFFSET_IN_STC_MAC(policerCounters,SKERNEL_TABLES_INFO_STC):
                case FIELD_OFFSET_IN_STC_MAC(policerConfig,SKERNEL_TABLES_INFO_STC):
                case FIELD_OFFSET_IN_STC_MAC(policerConformanceLevelSign,SKERNEL_TABLES_INFO_STC):
                    updateTableSizeAccordingToMeters = 1;
                    break;
                default:
                    break;
            }
        }

        if(updateTableSizeAccordingToMeters)
        {
            chunksMem[ii].numOfRegisters = numOfMeters * (chunksMem[ii].enrtyNumBytesAlignement / 4);
        }
    }
}

/*******************************************************************************
*   smemBobkUnitPolicerUnify
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
static void smemBobkUnitPolicerUnify
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
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000250, 0x00000294)}
            /*Policer Timer Memory*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000300, 36), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32,4),SMEM_BIND_TABLE_MAC(policerTimer)}
            /*Policer Descriptor Sample Memory*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000400, 96), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32,4)}
            /*Policer Management Counters Memory*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000500, 192), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(74,16),SMEM_BIND_TABLE_MAC(policerManagementCounters)}
            /*IPFIX wrap around alert Memory*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000800, 1024), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32,4),SMEM_BIND_TABLE_MAC(policerIpfixWaAlert)}
            /*IPFIX aging alert Memory*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00001000, 1024), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32,4),SMEM_BIND_TABLE_MAC(policerIpfixAgingAlert)}
            /*registers (not memory) : Port%p and Packet Type Translation Table*/
            /*registers -- not table/memory !! -- Port%p and Packet Type Translation Table*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001800 , 0x000019FC), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32,4),SMEM_BIND_TABLE_MAC(policerMeterPointer)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002000, 0x00002008)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002100, 0x000026FC)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003500, 0x00003500)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003510, 0x00003510)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003520, 0x00003524)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003550, 0x00003554)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003560, 0x00003560)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003604, 0x0000360C)}


            /*Ingress Policer Re-Marking Memory*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00080000, 8192), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(51,8),SMEM_BIND_TABLE_MAC(policerQos)}
            /*Hierarchical Policing Table*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00090000, 32768), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(18,4), SMEM_BIND_TABLE_MAC(policerHierarchicalQos)}

            /* NOTE: the number of bytes allocated for the meter/counting tables
              are calculated inside policerTablesSupport(...) so the value we see
              here are overridden !!!*/

            /*Metering Token Bucket Memory*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00100000, 196608), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(211,32),SMEM_BIND_TABLE_MAC(policer)}
            /*Counting Memory*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00140000, 196608), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(229,32),SMEM_BIND_TABLE_MAC(policerCounters)}
            /*Metering Configuration Memory*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00300000, 49152) , SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(51,8),SMEM_BIND_TABLE_MAC(policerConfig)}
            /*Metering Conformance Level Sign Memory*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00400000, 24576) , SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(2,4),SMEM_BIND_TABLE_MAC(policerConformanceLevelSign)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemBobkPolicerTablesSupport(devObjPtr,numOfChunks,chunksMem,plrUnit);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);

        /* eport related */
        {
            GT_U32  index;
            GT_U32  numEPorts = devObjPtr->limitedResources.ePort;

            SMEM_CHUNK_BASIC_STC  chunksMem[]=
            {
                /*e Attributes Table*/
                {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00040000, 65536), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(36,8),SMEM_BIND_TABLE_MAC(policerEPortEVlanTrigger)}
            };

            index = 0;
            chunksMem[index].numOfRegisters = numEPorts * (chunksMem[index].enrtyNumBytesAlignement / 4);

            smemUnitChunkAddBasicChunk(devObjPtr,unitPtr,
                ARRAY_NAME_AND_NUM_ELEMENTS_MAC(chunksMem));
        }


    }
}

static void smemBobkUnitIplr0
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    smemBobkUnitPolicerUnify(devObjPtr,unitPtr,SMEM_SIP5_PP_PLR_UNIT_IPLR_0_E);
}

static void smemBobkUnitIplr1
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    smemBobkUnitPolicerUnify(devObjPtr,unitPtr,SMEM_SIP5_PP_PLR_UNIT_IPLR_1_E);
}

static void smemBobkUnitEplr
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    smemBobkUnitPolicerUnify(devObjPtr,unitPtr,SMEM_SIP5_PP_PLR_UNIT_EPLR_E);
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
    /* iplr0 policer config table */
    {0x00300000, 0xFFFF0000, smemXCatActiveReadIplr0Tables, 0 , smemXCatActiveWriteIplr0Tables,  0},
    /* iplr0 Metering Conformance Level Sign Memory */
    {0x00400000, 0xFFFF0000, smemXCatActiveReadIplr0Tables, 0 , smemXCatActiveWriteIplr0Tables,  0},

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
    /* iplr1 policer config table */
    {0x00300000, 0xFFFF0000, smemXCatActiveReadIplr1Tables, 0 , smemXCatActiveWriteIplr1Tables,  0},
    /* iplr1 Metering Conformance Level Sign Memory */
    {0x00400000, 0xFFFF0000, smemXCatActiveReadIplr1Tables, 0 , smemXCatActiveWriteIplr1Tables,  0},

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

/* bind the GOP unit to it's active mem */
/*GOP - copied from BC2 and TAI under GOP removed */
#define BOBCAT2_GOP_64_PORTS_MASK_CNS   0xFFFC0FFF  /* support ports 0..55  in steps of 0x1000 */
#define BOBCAT2_GOP_16_PORTS_MASK_CNS   0xFFFF0FFF  /* support ports 56..71 in steps of 0x1000 */
/* active memory entry for GOP registers */
#define GOP_PORTS_0_71_ACTIVE_MEM_MAC(relativeAddr,readFun,readFunParam,writeFun,writeFunParam)    \
    {relativeAddr, BOBCAT2_GOP_64_PORTS_MASK_CNS, readFun, readFunParam , writeFun, writeFunParam}, \
    {relativeAddr+BOBCAT2_PORT_56_START_OFFSET_CNS, BOBCAT2_GOP_16_PORTS_MASK_CNS, readFun, readFunParam , writeFun, writeFunParam}
#if 0  /* for debugging */
void smemChtActiveWrite__dummy (
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
)
{
    GT_U32  port = (address / 0x1000) & 0x7f;

    printf("port %d changing [0x%x] \n",
        port,
        (* memPtr)^(* inMemPtr));

    *memPtr = *inMemPtr ;
}
#endif


static void bindUnitGopActiveMem(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    START_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)

#if 0  /* for debugging */
    /* Port<n>  MAC Control Register1 */
    GOP_PORTS_0_71_ACTIVE_MEM_MAC(0x00000004,NULL, 0 , smemChtActiveWrite__dummy, 0),
    GOP_PORTS_0_71_ACTIVE_MEM_MAC(0x000C0004,NULL, 0 , smemChtActiveWrite__dummy, 0),
#endif

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

/* bind the GOP unit to it's active mem */
static void bindUnitTaiActiveMem(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    START_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)
    /* TAI subunit */
    /* Time Counter Function Configuration 0 - Function Trigger */
    {0x10, 0xFFFFFFFF, NULL, 0 , smemLion3ActiveWriteTodFuncConfReg, 0},
    /* time Capture Value 0 Frac Low */
    {0x84, 0xFFFFFFFF, smemBobcat2ActiveReadTodTimeCaptureValueFracLow, 0 , smemChtActiveWriteToReadOnlyReg, 0},
    /* time Capture Value 1 Frac Low */
    {0xA0, 0xFFFFFFFF, smemBobcat2ActiveReadTodTimeCaptureValueFracLow, 1 , smemChtActiveWriteToReadOnlyReg, 0},
    END_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)
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

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_GOP)];
    if(IS_VALID_AND_EMPTY_ACTIVE_MEM_ARR_FOR_CHUNK_MAC(currUnitChunkPtr))
    {
        bindUnitGopActiveMem(devObjPtr,currUnitChunkPtr);
    }

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TAI)];
    if(IS_VALID_AND_EMPTY_ACTIVE_MEM_ARR_FOR_CHUNK_MAC(currUnitChunkPtr))
    {
        bindUnitTaiActiveMem(devObjPtr,currUnitChunkPtr);
    }
}

/*******************************************************************************
*   smemBobkSpecificDeviceUnitAlloc
*
* DESCRIPTION:
*       specific initialization units allocation that called before alloc units
*       of any device
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       Pointer to object for specific subunit
*
* COMMENTS:
*
*******************************************************************************/
static void smemBobkSpecificDeviceUnitAlloc
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = devObjPtr->deviceMemory;
    SMEM_UNIT_CHUNKS_STC    *currUnitChunkPtr;
    GT_U32  unitIndex;

    /* allocate the specific units that we NOT want the bc2_init , lion3_init , lion2_init
       to allocate. */

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_MG)];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemBobkUnitMg(devObjPtr,currUnitChunkPtr);
    }

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_GOP)];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemBobkUnitGop(devObjPtr,currUnitChunkPtr);
    }

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_SERDES)];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemBobkUnitLpSerdes(devObjPtr,currUnitChunkPtr);
    }

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_XG_PORT_MIB)];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemBobkUnitXGPortMib(devObjPtr,currUnitChunkPtr);
    }

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_MPPM)];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemBobkUnitMppm(devObjPtr,currUnitChunkPtr);
    }

    if(devObjPtr->multiDataPath.info[0].dataPathNumOfPorts)
    {
        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_RX_DMA)];
        if(currUnitChunkPtr->numOfChunks == 0)
        {
            smemBobkUnitRxDma(devObjPtr,currUnitChunkPtr);
        }

        unitIndex = UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TX_DMA);
        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[unitIndex];
        if(currUnitChunkPtr->numOfChunks == 0)
        {
            smemBobkUnitTxDma(devObjPtr,currUnitChunkPtr);
        }

        unitIndex = UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TX_FIFO);
        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[unitIndex];
        if(currUnitChunkPtr->numOfChunks == 0)
        {
            smemBobkUnitTxFifo(devObjPtr,currUnitChunkPtr);
        }
    }

    if(devObjPtr->multiDataPath.info[1].dataPathNumOfPorts)
    {
        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_RX_DMA_1)];
        if(currUnitChunkPtr->numOfChunks == 0)
        {
            smemBobkUnitRxDma(devObjPtr,currUnitChunkPtr);
        }

        unitIndex = UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TX_DMA_1);
        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[unitIndex];
        if(currUnitChunkPtr->numOfChunks == 0)
        {
            smemBobkUnitTxDma(devObjPtr,currUnitChunkPtr);
        }

        unitIndex = UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TX_FIFO_1);
        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[unitIndex];
        if(currUnitChunkPtr->numOfChunks == 0)
        {
            smemBobkUnitTxFifo(devObjPtr,currUnitChunkPtr);
        }
        if(devObjPtr->supportTrafficManager)
        {
            unitIndex = UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_ETH_TX_FIFO_1);
            currUnitChunkPtr = &devMemInfoPtr->unitMemArr[unitIndex];
            if(currUnitChunkPtr->numOfChunks == 0)
            {
                smemBobcat2UnitEthTxFifo(devObjPtr,currUnitChunkPtr);
            }
        }
    }

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_RX_DMA_GLUE)];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemBobkUnitRxDmaGlue(devObjPtr,currUnitChunkPtr);
    }

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TX_DMA_GLUE)];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemBobkUnitTxDmaGlue(devObjPtr,currUnitChunkPtr);
    }

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TAI)];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemBobkUnitTai(devObjPtr,currUnitChunkPtr);
    }

    unitIndex = UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_BM);
    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[unitIndex];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemBobkUnitBm(devObjPtr,currUnitChunkPtr);
    }

    unitIndex = UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_BMA);
    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[unitIndex];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemBobkUnitBma(devObjPtr,currUnitChunkPtr);
    }

    unitIndex = UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_FDB_TABLE_0);
    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[unitIndex];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemBobkUnitFdbTable(devObjPtr,currUnitChunkPtr);

        /* 'connect' the next sub units to the first one
           and connect 'my self' */
        devMemInfoPtr->unitMemArr[unitIndex].hugeUnitSupportPtr =
        devMemInfoPtr->unitMemArr[unitIndex + 1].hugeUnitSupportPtr =
            &devMemInfoPtr->unitMemArr[unitIndex];
    }

    unitIndex = UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_IPLR);
    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[unitIndex];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemBobkUnitIplr0(devObjPtr,currUnitChunkPtr);
    }

    unitIndex = UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_IPLR1);
    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[unitIndex];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemBobkUnitIplr1(devObjPtr, currUnitChunkPtr);
    }

    unitIndex = UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_EPLR);
    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[unitIndex];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemBobkUnitEplr(devObjPtr,currUnitChunkPtr);
    }

}
/*******************************************************************************
*   smemBobkInitInterrupts
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
static void smemBobkInitInterrupts
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
}

/*******************************************************************************
*   smemBobkInitPostBobcat2
*
* DESCRIPTION:
*       init after calling bobcat2 init.
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
static void smemBobkInitPostBobcat2
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    GT_U32  taiBaseAddr = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_TAI);
    GT_U32  ii;
    GT_U32* regsAddrPtr32;
    GT_U32  regsAddrPtr32Size;
    GT_U32  firstAddress;

    if(devObjPtr->supportSingleTai)
    {
        /* fix db of registers of TAI to align to new single base address */

        regsAddrPtr32 = (void*)&(SMEM_CHT_MAC_REG_DB_SIP5_GET(devObjPtr)->TAI.TAI[0][0]);
        regsAddrPtr32Size = sizeof(SMEM_CHT_MAC_REG_DB_SIP5_GET(devObjPtr)->TAI.TAI[0][0])/sizeof(GT_U32);

        /* fix addresses in [0][0] */
        firstAddress = regsAddrPtr32[0];
        for(ii = 0 ; ii < regsAddrPtr32Size; ii++ )
        {
            regsAddrPtr32[ii] = taiBaseAddr + (regsAddrPtr32[ii] - firstAddress);
        }

        /* invalidate all other registers !!! in the TAI  */
        regsAddrPtr32Size = sizeof(SMEM_CHT_MAC_REG_DB_SIP5_GET(devObjPtr)->TAI.TAI)/sizeof(GT_U32);
        /* continue with the ii .... */
        for(/* ii continue */; ii < regsAddrPtr32Size; ii++ )
        {
            regsAddrPtr32[ii] = SMAIN_NOT_VALID_CNS;
        }

    }

    if(SMAIN_NOT_VALID_CNS == UNIT_BASE_ADDR_GET_ALLOW_NON_EXIST_UNIT_MAC(devObjPtr,UNIT_LMS))
    {
        /* remove all LMS registers */
        regsAddrPtr32 = (void*)&(SMEM_CHT_MAC_REG_DB_SIP5_GET(devObjPtr)->LMS);
        regsAddrPtr32Size = sizeof(SMEM_CHT_MAC_REG_DB_SIP5_GET(devObjPtr)->LMS)/sizeof(GT_U32);

        for(ii = 0 ; ii < regsAddrPtr32Size; ii++ )
        {
            regsAddrPtr32[ii] = SMAIN_NOT_VALID_CNS;
        }
    }

}

/*extern*/ SMEM_REGISTER_DEFAULT_VALUE_STC sip5_15_registersDefaultValueArr[] =
{
     {&STRING_FOR_UNIT_NAME(UNIT_TCAM),            0x00504010,         0x00000006,      1,    0x0      }

    ,DEFAULT_REG_FOR_PLR_UNIT_MAC(                0x0000000c,         0xffff0000,      1,    0x0    )
    ,DEFAULT_REG_FOR_PLR_UNIT_MAC(                0x00000010,         0x00019000,      1,    0x0    )
    ,DEFAULT_REG_FOR_PLR_UNIT_MAC(                0x00000060,         0x00001318,      1,    0x0    )
    ,DEFAULT_REG_FOR_PLR_UNIT_MAC(                0x00000064,         0x000003E8,      1,    0x0    )
    ,DEFAULT_REG_FOR_PLR_UNIT_MAC(                0x00000294,         0x0001ffff,      1,    0x0    )
    ,DEFAULT_REG_FOR_PLR_UNIT_MAC(                0x00002004 ,        0x00014800,      1,    0x0    )
    ,DEFAULT_REG_FOR_PLR_UNIT_MAC(                0x00002008 ,        0x00010040,      1,    0x0    )
    ,DEFAULT_REG_FOR_PLR_UNIT_MAC(                0x00002100 ,        0x0001ffff,      1,    0x0    )
    ,DEFAULT_REG_FOR_PLR_UNIT_MAC(                0x00002300 ,        0x0001ffff,      1,    0x0    )
    ,DEFAULT_REG_FOR_PLR_UNIT_MAC(                0x00003604 ,        0x000000bf,      1,    0x0    )

    /* PTP related ports 0-47 */
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x00180878,         0x00000040,      48,   0x1000 }
    /* PTP related ports 56-59 */
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x00380878,         0x00000040,      4,   0x1000 }
    /* PTP related ports 62 */
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x00386878,         0x00000040,      1,   0x1000 }
    /* PTP related ports 64-71 */
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP),            0x00388878,         0x00000040,      8,   0x1000 }

    ,{NULL,            0,         0x00000000,      0,    0x0      }
};

/*extern*/ SMEM_REGISTER_DEFAULT_VALUE_STC sip5_15_serdes_registersDefaultValueArr[] =
{
    /* SERDES registers */
    {&STRING_FOR_UNIT_NAME(UNIT_SERDES),         0x00000000,         0x00000800,   12  ,   0x1000    }
   ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),         0x00000004,         0x88000001,   12  ,   0x1000    }
   ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),         0x00000008,         0x00000100,   12  ,   0x1000    }
   ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),         0x00054000,         0x00000800,    1  ,   0x1000    }
   ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),         0x00054004,         0x88000001,    1  ,   0x1000    }
   ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),         0x00054008,         0x00000100,    1  ,   0x1000    }
   ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),         0x00058000,         0x00000800,   12  ,   0x1000    }
   ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),         0x00058004,         0x88000001,   12  ,   0x1000    }
   ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),         0x00058008,         0x00000100,   12  ,   0x1000    }
   /* must be last */
   ,{NULL,            0,         0x00000000,      0,    0x0      }
};

static SMEM_LINK_LIST_REGISTER_DEFAULT_VALUE_STC  element5_RegistersDefaults =
    {sip5_15_serdes_registersDefaultValueArr, NULL};
static SMEM_LINK_LIST_REGISTER_DEFAULT_VALUE_STC  element4_RegistersDefaults =
    {bobcat2B0Additional_registersDefaultValueArr, &element5_RegistersDefaults};
static SMEM_LINK_LIST_REGISTER_DEFAULT_VALUE_STC  element3_RegistersDefaults =
    {bobcat2Additional_registersDefaultValueArr, &element4_RegistersDefaults};
static SMEM_LINK_LIST_REGISTER_DEFAULT_VALUE_STC  element2_RegistersDefaults =
    {sip5_15_registersDefaultValueArr, &element3_RegistersDefaults};
static SMEM_LINK_LIST_REGISTER_DEFAULT_VALUE_STC  element1_RegistersDefaults =
    {sip5_10_registersDefaultValueArr,&element2_RegistersDefaults};
static SMEM_LINK_LIST_REGISTER_DEFAULT_VALUE_STC   linkListElementsBobk_RegistersDefaults =
    {sip5_registersDefaultValueArr,    &element1_RegistersDefaults};


/* registers of sip5_10/sip5 that not exists any more ... will be skipped in registers default values */
static SMEM_REGISTER_DEFAULT_VALUE_STC sip5_15_non_exists_registersDefaultValueArr[] =
{
     /*9 MSBits of 'Addr', 23 LSBits of 'Addr',     val,    repeat,    skip*/
     {&STRING_FOR_UNIT_NAME(UNIT_MG),            0x00000390,         0x000fffff,      1,    0x0      }

    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x00001700,         0x00008021,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x000019b0,         0x0601e114,      1,    0x0      }
    ,{&STRING_FOR_UNIT_NAME(UNIT_RX_DMA),         0x000019b4,         0x003896cb,      1,    0x0      }

    ,{&STRING_FOR_UNIT_NAME(UNIT_BM),             0x00000030 + 2*4,     0x3fff0000,   (3-2),    0x4    }/* only 2 in bobk */
    ,{&STRING_FOR_UNIT_NAME(UNIT_BM),             0x00000420 + 4*4,     0x00031490,   (6-4),    0x4    }/* only 4 in bobk */
    ,{&STRING_FOR_UNIT_NAME(UNIT_BM),             0x00000440 + 4*4,     0x000024c9,   (6-4),    0x4    }/* only 4 in bobk */
    ,{&STRING_FOR_UNIT_NAME(UNIT_BM),             0x000004a0 + 2*4,     0x00080000,   (3-2),    0x4    }/* only 2 in bobk */

     /* must be last */
    ,{NULL,            0,         0x00000000,      0,    0x0      }
};


/* registers of non exists ports 0..47 ... will be skipped in registers default values */
static SMEM_REGISTER_DEFAULT_VALUE_STC bobk_non_exists_port_0_47_registersDefaultValueArr[] =
{
    /* GOP */
    /* remove ports 0..47 */ /* SMAIN_NOT_VALID_CNS - indication for range according to step */            /*start index*//*end index*/   /*range*/       /*step of ports*/
     {&STRING_FOR_UNIT_NAME(UNIT_GOP), SMAIN_NOT_VALID_CNS , BOBCAT2_PORT_0_START_OFFSET_CNS             ,  0 , 47 , BOBCAT2_PORT_OFFSET_CNS , 1}
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP), SMAIN_NOT_VALID_CNS , BOBCAT2_PORT_0_START_OFFSET_CNS + 0x00180000,  0 , 47 , BOBCAT2_PORT_OFFSET_CNS , 1}
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP), SMAIN_NOT_VALID_CNS , BOBCAT2_PORT_0_START_OFFSET_CNS + 0x000c0000,  0 , 47 , BOBCAT2_PORT_OFFSET_CNS , 1}
    /* SERDES */
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),         0x00000000,         0x00000800,   12  ,   0x1000    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),         0x00000004,         0x88000001,   12  ,   0x1000    }
    ,{&STRING_FOR_UNIT_NAME(UNIT_SERDES),         0x00000008,         0x00000100,   12  ,   0x1000    }

     /* must be last */
    ,{NULL,            0,         0x00000000,      0,    0x0      }
};

/* registers of non exists ports 48..55 ... will be skipped in registers default values */
static SMEM_REGISTER_DEFAULT_VALUE_STC bobk_non_exists_port_48_55_registersDefaultValueArr[] =
{
    /* GOP */
    /* remove ports 48 .. 55 */ /* SMAIN_NOT_VALID_CNS - indication for range according to step */            /*start index*//*end index*/   /*range*/       /*step of ports*/
     {&STRING_FOR_UNIT_NAME(UNIT_GOP), SMAIN_NOT_VALID_CNS , BOBCAT2_PORT_48_START_OFFSET_CNS             ,  (48-48) , (55-48) , BOBCAT2_PORT_OFFSET_CNS , 1}
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP), SMAIN_NOT_VALID_CNS , BOBCAT2_PORT_48_START_OFFSET_CNS + 0x00180000,  (48-48) , (55-48) , BOBCAT2_PORT_OFFSET_CNS , 1}
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP), SMAIN_NOT_VALID_CNS , BOBCAT2_PORT_48_START_OFFSET_CNS + 0x000c0000,  (48-48) , (55-48) , BOBCAT2_PORT_OFFSET_CNS , 1}

    /* remove ports 60 .. 63 */
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP), SMAIN_NOT_VALID_CNS , BOBCAT2_PORT_56_START_OFFSET_CNS            ,  (60-56)  , (63-56) , BOBCAT2_PORT_OFFSET_CNS , 1}
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP), SMAIN_NOT_VALID_CNS , BOBCAT2_PORT_56_START_OFFSET_CNS+ 0x00180000,  (60-56)  , (63-56) , BOBCAT2_PORT_OFFSET_CNS , 1}
    ,{&STRING_FOR_UNIT_NAME(UNIT_GOP), SMAIN_NOT_VALID_CNS , BOBCAT2_PORT_56_START_OFFSET_CNS+ 0x000c0000,  (60-56)  , (63-56) , BOBCAT2_PORT_OFFSET_CNS , 1}

     /* must be last */
    ,{NULL,            0,         0x00000000,      0,    0x0      }
};

static SMEM_LINK_LIST_REGISTER_DEFAULT_VALUE_STC   bobk_non_exists_port_48_55_linkListElements_RegistersDefaults[] =
{
    {bobk_non_exists_port_48_55_registersDefaultValueArr,    NULL}
};

static SMEM_LINK_LIST_REGISTER_DEFAULT_VALUE_STC   bobk_non_exists_linkListElements_RegistersDefaults[] =
{
    {sip5_15_non_exists_registersDefaultValueArr,    bobk_non_exists_port_48_55_linkListElements_RegistersDefaults}
};

static SMEM_LINK_LIST_REGISTER_DEFAULT_VALUE_STC   bobk_cetus_non_exists_linkListElements_RegistersDefaults[] =
{
    {bobk_non_exists_port_0_47_registersDefaultValueArr , bobk_non_exists_linkListElements_RegistersDefaults}
};

static SMEM_UNIT_DUPLICATION_INFO_STC BOBK_duplicatedUnits[] =
{
    /* 'pairs' of duplicated units */
    {STR(UNIT_ETH_TX_FIFO)  ,1} ,{STR(UNIT_ETH_TX_FIFO_1) },
    {STR(UNIT_TX_FIFO)      ,1} ,{STR(UNIT_TX_FIFO_1)     },
    {STR(UNIT_TX_DMA)       ,1} ,{STR(UNIT_TX_DMA_1)      },
    {STR(UNIT_RX_DMA)       ,1} ,{STR(UNIT_RX_DMA_1)      },

    {NULL,0} /* must be last */
};


/*******************************************************************************
*   smemBobkSpecificDeviceMemInitPart2
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
void smemBobkSpecificDeviceMemInitPart2
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    /* bind active memories to units ... need to be before
        smemBobcat2SpecificDeviceMemInitPart2 ... so bc2 will not override it */
    bindActiveMemoriesOnUnits(devObjPtr);

    /* call bobcat2 */
    smemBobcat2SpecificDeviceMemInitPart2(devObjPtr);
}

/*******************************************************************************
*   smemBobkInit
*
* DESCRIPTION:
*       Init memory module for a bobk device.
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
void smemBobkInit
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    GT_BOOL isBobk = GT_FALSE;

    /* state the supported features */
    SMEM_CHT_IS_SIP5_GET(devObjPtr) = 1;
    SMEM_CHT_IS_SIP5_10_GET(devObjPtr) = 1;
    SMEM_CHT_IS_SIP5_15_GET(devObjPtr) = 1;

    if(devObjPtr->devMemUnitNameAndIndexPtr == NULL)
    {
        devObjPtr->devMemUnitNameAndIndexPtr = bobkUnitNameAndIndexArr;
        devObjPtr->genericUsedUnitsAddressesArray = bobkUsedUnitsAddressesArray;
        devObjPtr->genericNumUnitsAddresses = BOBK_UNIT_LAST_E;

        isBobk = GT_TRUE;
    }

    if(devObjPtr->registersDefaultsPtr == NULL)
    {
        devObjPtr->registersDefaultsPtr = &linkListElementsBobk_RegistersDefaults;
    }

    if(devObjPtr->registersDefaultsPtr_unitsDuplications == NULL)
    {
        devObjPtr->registersDefaultsPtr_unitsDuplications = BOBK_duplicatedUnits;
    }


    if (isBobk == GT_TRUE)
    {
        /* state 'data path' structure */
        devObjPtr->multiDataPath.supportMultiDataPath =  1;
        devObjPtr->multiDataPath.maxDp = 2;/* 2 DP units for the device */
        devObjPtr->multiDataPath.localIndexOfDmaOfCpuPortNum = 72;

        devObjPtr->multiDataPath.info[0].dataPathFirstPort = 0;
        devObjPtr->multiDataPath.info[0].dataPathNumOfPorts = 56;

        devObjPtr->multiDataPath.info[1].dataPathFirstPort = 56;
        devObjPtr->multiDataPath.info[1].dataPathNumOfPorts = 16 + 2;/* the CPU,TM */
        devObjPtr->multiDataPath.info[1].cpuPortDmaNum = 72;
        devObjPtr->multiDataPath.info[1].tmDmaNum = 73;

        /* dual data path units addresses */
        devObjPtr->memUnitBaseAddrInfo.rxDma[0] = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_RX_DMA);
        devObjPtr->memUnitBaseAddrInfo.rxDma[1] = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_RX_DMA_1);

        devObjPtr->memUnitBaseAddrInfo.txDma[0] = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_TX_DMA);
        devObjPtr->memUnitBaseAddrInfo.txDma[1] = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_TX_DMA_1);

        devObjPtr->memUnitBaseAddrInfo.txFifo[0] = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_TX_FIFO);
        devObjPtr->memUnitBaseAddrInfo.txFifo[1] = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_TX_FIFO_1);

        devObjPtr->tcam_numBanksForHitNumGranularity = 6;
    }

    devObjPtr->supportSingleTai = 1;
    SET_IF_ZERO_MAC(devObjPtr->numOfTaiUnits ,1);
    SET_IF_ZERO_MAC(devObjPtr->lpmRam.numOfLpmRams , 20);
    SET_IF_ZERO_MAC(devObjPtr->lpmRam.numOfEntriesBetweenRams , 16*1024);

    devObjPtr->supportNat66 = 1;

    if (isBobk == GT_TRUE)
    {
        SET_IF_ZERO_MAC(devObjPtr->flexFieldNumBitsSupport.phyPort,7);
        SET_IF_ZERO_MAC(devObjPtr->flexFieldNumBitsSupport.stgId,10); /*1K*/
        SET_IF_ZERO_MAC(devObjPtr->flexFieldNumBitsSupport.tunnelstartPtr , 14);/*16K*/

        SET_IF_ZERO_MAC(devObjPtr->limitedResources.eVid,4*1024 + 512); /*4.5k*/
        SET_IF_ZERO_MAC(devObjPtr->limitedResources.ePort,6*1024);/*6k*/
        SET_IF_ZERO_MAC(devObjPtr->limitedResources.phyPort,128);
        SET_IF_ZERO_MAC(devObjPtr->limitedResources.l2Ecmp,4*1024);
        SET_IF_ZERO_MAC(devObjPtr->limitedResources.nextHop,8*1024);
        SET_IF_ZERO_MAC(devObjPtr->limitedResources.mllPairs,4*1024);
        SET_IF_ZERO_MAC(devObjPtr->limitedResources.l2LttMll,8*1024);
        SET_IF_ZERO_MAC(devObjPtr->limitedResources.l3LttMll,4*1024);


        if (IS_PORT_0_EXISTS(devObjPtr))
        {
            devObjPtr->fdbMaxNumEntries = SMEM_MAC_TABLE_SIZE_64KB;
            devObjPtr->lpmRam.perRamNumEntries = 6*1024;

            /* list of removed registers that should not be accessed */
            devObjPtr->registersDefaultsPtr_ignored =
                bobk_non_exists_linkListElements_RegistersDefaults;
        }
        else /* reduced silicon */
        {
            devObjPtr->fdbMaxNumEntries = SMEM_MAC_TABLE_SIZE_32KB;
            devObjPtr->lpmRam.perRamNumEntries = 2*1024;

            /* DP 0 is not valid !!! */
            devObjPtr->multiDataPath.info[0].dataPathFirstPort = SMAIN_NOT_VALID_CNS;

            /* list of removed registers that should not be accessed */
            devObjPtr->registersDefaultsPtr_ignored =
                bobk_cetus_non_exists_linkListElements_RegistersDefaults;
        }



        SET_IF_ZERO_MAC(devObjPtr->policerSupport.eplrTableSize   , (4 * _1K));
        SET_IF_ZERO_MAC(devObjPtr->policerSupport.iplrTableSize   , (6 * _1K));
        SET_IF_ZERO_MAC(devObjPtr->policerSupport.iplrMemoriesSize[0] , 4888);
        SET_IF_ZERO_MAC(devObjPtr->policerSupport.iplrMemoriesSize[1] , 1000);
        SET_IF_ZERO_MAC(devObjPtr->policerSupport.iplrMemoriesSize[2] , /*256 */
            devObjPtr->policerSupport.iplrTableSize -
            (devObjPtr->policerSupport.iplrMemoriesSize[0] + devObjPtr->policerSupport.iplrMemoriesSize[1]));

        SET_IF_ZERO_MAC(devObjPtr->policerSupport.iplr0TableSize ,
                        devObjPtr->policerSupport.iplrMemoriesSize[1] +
                        devObjPtr->policerSupport.iplrMemoriesSize[2]) ;
        SET_IF_ZERO_MAC(devObjPtr->policerSupport.iplr1TableSize ,
                    devObjPtr->policerSupport.iplrTableSize -
                    devObjPtr->policerSupport.iplr0TableSize);

    }

    devObjPtr->policerSupport.tablesBaseAddrSetByOrigDev = 1;

    /* function will be called from inside smemLion2AllocSpecMemory(...) */
    if(devObjPtr->devMemSpecificDeviceUnitAlloc == NULL)
    {
        devObjPtr->devMemSpecificDeviceUnitAlloc = smemBobkSpecificDeviceUnitAlloc;
    }

    /* function will be called from inside smemLion3Init(...) */
    if(devObjPtr->devMemSpecificDeviceMemInitPart2 == NULL)
    {
        devObjPtr->devMemSpecificDeviceMemInitPart2 = smemBobkSpecificDeviceMemInitPart2;
    }

    smemBobcat2Init(devObjPtr);

    smemBobkInitPostBobcat2(devObjPtr);

    /* Init the bobk interrupts */
    smemBobkInitInterrupts(devObjPtr);

}

/*******************************************************************************
*   smemBobkInit2
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
void smemBobkInit2
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    smemBobcat2Init2(devObjPtr);
}


