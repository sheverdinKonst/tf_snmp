/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* prvCpssDxChPortDynamicPizzaArbiterWS.c
*
* DESCRIPTION:
*       bobcat2 and higher dynamic (algorithmic) pizza arbiter Data structures (device specific)
*
* FILE REVISION NUMBER:
*       $Revision: 93 $
*******************************************************************************/

#include <cpss/dxCh/dxChxGen/port/PortMapping/prvCpssDxChPortMappingShadowDB.h>
#include <cpss/dxCh/dxChxGen/port/PizzaArbiter/DynamicPizzaArbiter/prvCpssDxChPortDynamicPATypeDef.h>
#include <cpss/dxCh/dxChxGen/port/PizzaArbiter/DynamicPizzaArbiter/prvCpssDxChPortDynamicPizzaArbiter.h>
#include <cpss/dxCh/dxChxGen/port/PizzaArbiter/DynamicPizzaArbiter/prvCpssDxChPortDynamicPizzaArbiterWS.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern GT_STATUS BuildPizzaDistribution
(
    IN   GT_FLOAT64 *portsConfigArrayPtr,
    IN   GT_U32      size,
    IN   GT_U32      maxPipeSizeMbps,
    IN   GT_U32      minSliceResolutionMpbs,
    IN   GT_U32      pizzaArraySize,
    OUT  GT_U32     *pizzaArray,
    OUT  GT_U32     *numberOfSlicesPtr,
    OUT  GT_U32     *highSpeedPortNumberPtr,
    OUT  GT_U32      highSpeedPortArr[PRV_CPSS_DXCH_PORT_HIGH_SPEED_PORT_NUM_CNS]
);

extern GT_STATUS BuildPizzaDistributionWithDummySlice
(
    IN   GT_FLOAT64  *portsConfigArrayPtr,
    IN   GT_U32       size,
    IN   GT_U32       maxPipeSize,
	IN   GT_U32       minSliceResolution,
    IN   GT_U32       pizzaArraySize,
    OUT  GT_U32      *pizzaArray,
    OUT  GT_U32      *numberOfSlicesPtr,
    OUT  GT_U32      *highSpeedPortNumPtr,
    OUT  GT_U32       highSpeedPortArr[PRV_CPSS_DXCH_PORT_HIGH_SPEED_PORT_NUM_CNS]
);

extern GT_STATUS BuildTxQPizzaDistribution
(
    IN   GT_FLOAT64 *portsConfigArrayPtr,
    IN   GT_U32      size,
    IN   GT_U32      maxPipeSizeMbps,
    IN   GT_U32      minSliceResolutionMpbs,
    IN   GT_U32      pizzaArraySize,
    OUT  GT_U32     *pizzaArray,
    OUT  GT_U32     *numberOfSlicesPtr,
    OUT  GT_U32     *highSpeedPortNumberPtr,
    OUT  GT_U32      highSpeedPortArr[PRV_CPSS_DXCH_PORT_HIGH_SPEED_PORT_NUM_CNS]
);


#ifdef __cplusplus
}
#endif /* __cplusplus */



/*-----------------------------------------*/
/* data declaration                        */
/*-----------------------------------------*/


/*-----------------------------------------------------------------------
 * list of pizza arbiter units to be configured depending on device type
 *     bc2
 *     bobk-pipe0-1
 *     bobk-pipe-1
 *-----------------------------------------------------------------------
 */
#define PRV_CPSS_DXCH_BC2_BOBK_PIPE0_FIRST_MAC_CNS   0
#define PRV_CPSS_DXCH_BC2_BOBK_PIPE0_LAST_MAC_CNS   47
#define PRV_CPSS_DXCH_BC2_BOBK_PIPE1_FIRST_MAC_CNS  48
#define PRV_CPSS_DXCH_BC2_BOBK_PIPE1_LAST_MAC_CNS   72



typedef struct 
{
    CPSS_PORT_SPEED_ENT speedEnm;
    GT_U32              speedGbps;
}PRV_CPSS_DXCH_PA_SPEEDENM_SPEEDGBPS_STC;


typedef struct 
{
    CPSS_DXCH_PORT_MAPPING_TYPE_ENT mappingTye;
    GT_U32                          tmEnable;
    CPSS_DXCH_PA_UNIT_ENT          *unitList2ConfigurePtr;
}PRV_CPSS_DXCH_MAPPING_TM_2_UNITLIST_STC;


typedef GT_STATUS (*BuildPizzaDistributionFUN)
(
    IN   GT_FLOAT64 *portsConfigArrayPtr,
    IN   GT_U32      size,
    IN   GT_U32      maxPipeSizeMbps,
    IN   GT_U32      minSliceResolutionMpbs,
    IN   GT_U32      pizzaArraySize,
    OUT  GT_U32     *pizzaArray,
    OUT  GT_U32     *numberOfSlicesPtr,
    OUT  GT_U32     *highSpeedPortNumberPtr,
    OUT  GT_U32      highSpeedPortArr[PRV_CPSS_DXCH_PORT_HIGH_SPEED_PORT_NUM_CNS]
);

typedef struct 
{
    CPSS_DXCH_PA_UNIT_ENT     unit;
    BuildPizzaDistributionFUN fun;
}PRV_CPSS_DXCH_UNIT_2_PIZZA_FUN_STC;


typedef GT_STATUS  (*prvCpssDxChBc2Mapping2UnitConvFUN)
(
    IN GT_U8                              devNum,
    IN PRV_CPSS_DXCH_PORT_MAP_SHADOW_STC *portMapShadowPtr,
    OUT GT_U32                            portArgArr[CPSS_DXCH_PA_UNIT_MAX_E]
);

/*-----------------------------------------------*/
/*  Pizza Arbiter Worksapce (object int params   */
/*-----------------------------------------------*/
/*
 * strct: PRV_CPSS_DXCH_PA_WS_INIT_PARAMS_STC
 *          this params are used to initilize the specific PA object
 *
 * Description: init params for PA object 
 *
 *    supportedUnitListPtr               - list of unit supported 
 *                                                       (ended by CPSS_DXCH_PA_UNIT_UNDEFINED_E )
 *    unitListNotConfByPipeBwSetListPtr; - list of units not configured by cpssDxChPortPizzaArbiterPipeBWMinPortSpeedResolutionSet() 
 *                                                       (ended by CPSS_DXCH_PA_UNIT_UNDEFINED_E )
 *    speedEnm2GbpsPtr                   - list of converstion CPSS speed to Gbps (ended { CPSS_PORT_SPEED_NA_E, 0}
 *    prvCpssDxChBc2Mapping2UnitConvFUN  - function that convert CPSS mapping to device Mapping 
 *                                         e.g. at DMA      -- to corresponding data path, 
 *                                              at DMA GLUE -- to client 0/1 at BobK 
 *    mappingTm2UnitListPtr              - list of units to be configured at specific mapping with/wo Traffic Manager 
 *                                                     ended     ,{ CPSS_DXCH_PORT_MAPPING_TYPE_INVALID_E, (GT_U32)(~0) ,   (CPSS_DXCH_PA_UNIT_ENT *)NULL}
 *    unit_2_pizzaCompFun                - list of unit --> Pizza Algorithm (for aech configured unit the correspong Pizza Algorithm)
 *                                                     ended  ,{ CPSS_DXCH_PA_UNIT_UNDEFINED_E    , (BuildPizzaDistributionFUN)NULL   }
 *    mppmClientCodingList               - mppm client coding list <unit --> client code> ,
*                                                      ended by { CPSS_DXCH_PA_UNIT_UNDEFINED_E, ~0 , ~0 }
 */      
typedef struct 
{
    CPSS_DXCH_PA_UNIT_ENT                     *supportedUnitListPtr;
    CPSS_DXCH_PA_UNIT_ENT                     *unitListConfByPipeBwSetListPtr;
    PRV_CPSS_DXCH_PA_SPEEDENM_SPEEDGBPS_STC   *speedEnm2GbpsPtr; 
    prvCpssDxChBc2Mapping2UnitConvFUN          mapping2UnitConvFun;
    PRV_CPSS_DXCH_MAPPING_TM_2_UNITLIST_STC   *mappingTm2UnitListPtr;
    PRV_CPSS_DXCH_UNIT_2_PIZZA_FUN_STC        *unit_2_pizzaCompFun;
    PRV_CPSS_DXCH_HIGH_SPEED_PORT_CONFIG_FUN   txQHighSpeedPortSetFun;
    PRV_CPSS_DXCH_MPPM_CLIENT_CODING_STC      *mppmClientCodingList;
    GT_BOOL                                    txqWorkConservingModeOn;
}PRV_CPSS_DXCH_PA_WS_INIT_PARAMS_STC;

/*-------------------------------------------------------------*
 *  BC2                                                        *
 *-------------------------------------------------------------*/
static CPSS_DXCH_PA_UNIT_ENT   prv_bc2_UnitList_with_ilkn[] =            /* list of units used in BC2 just pipe 0 */
{
     CPSS_DXCH_PA_UNIT_RXDMA_E
    ,CPSS_DXCH_PA_UNIT_TXDMA_E
    ,CPSS_DXCH_PA_UNIT_TX_FIFO_E
    ,CPSS_DXCH_PA_UNIT_TXQ_E
    ,CPSS_DXCH_PA_UNIT_ETH_TX_FIFO_E
    ,CPSS_DXCH_PA_UNIT_ILKN_TX_FIFO_E
    ,CPSS_DXCH_PA_UNIT_UNDEFINED_E           /* last unit */
};

static CPSS_DXCH_PA_UNIT_ENT   prv_bc2_with_ilkn_unitListConfByPipeBwSetList[] = 
{
     CPSS_DXCH_PA_UNIT_RXDMA_E
    ,CPSS_DXCH_PA_UNIT_TXDMA_E
    ,CPSS_DXCH_PA_UNIT_TX_FIFO_E
    ,CPSS_DXCH_PA_UNIT_TXQ_E
    ,CPSS_DXCH_PA_UNIT_ETH_TX_FIFO_E
    ,CPSS_DXCH_PA_UNIT_UNDEFINED_E           /* last unit */
};


static CPSS_DXCH_PA_UNIT_ENT   prv_bc2_UnitList_no_ilkn[] =            /* list of units used in BC2 just pipe 0 */
{
     CPSS_DXCH_PA_UNIT_RXDMA_E
    ,CPSS_DXCH_PA_UNIT_TXDMA_E
    ,CPSS_DXCH_PA_UNIT_TX_FIFO_E
    ,CPSS_DXCH_PA_UNIT_TXQ_E
    ,CPSS_DXCH_PA_UNIT_ETH_TX_FIFO_E
    ,CPSS_DXCH_PA_UNIT_UNDEFINED_E           /* last unit */
};

static CPSS_DXCH_PA_UNIT_ENT   prv_bc2_no_ilkn_unitListConfByPipeBwSetList[] = 
{
     CPSS_DXCH_PA_UNIT_RXDMA_E
    ,CPSS_DXCH_PA_UNIT_TXDMA_E
    ,CPSS_DXCH_PA_UNIT_TX_FIFO_E
    ,CPSS_DXCH_PA_UNIT_TXQ_E
    ,CPSS_DXCH_PA_UNIT_ETH_TX_FIFO_E
    ,CPSS_DXCH_PA_UNIT_UNDEFINED_E           /* last unit */
};


static PRV_CPSS_DXCH_PA_SPEEDENM_SPEEDGBPS_STC prv_bc2_speedEnm2Gbps[] = 
{
     { CPSS_PORT_SPEED_1000_E ,  1 }
    ,{ CPSS_PORT_SPEED_2500_E ,  3 }
    ,{ CPSS_PORT_SPEED_10000_E, 10 }
    ,{ CPSS_PORT_SPEED_12000_E, 12 }
    ,{ CPSS_PORT_SPEED_20000_E, 20 }
    ,{ CPSS_PORT_SPEED_40000_E, 40 }
    ,{ CPSS_PORT_SPEED_11800_E, 12 }
    ,{ CPSS_PORT_SPEED_47200_E, 48 }
    ,{ CPSS_PORT_SPEED_NA_E,     0 }
};

static GT_STATUS  prvCpssDxChBc2Mapping2UnitConvFun
(
    IN GT_U8                              devNum,
    IN PRV_CPSS_DXCH_PORT_MAP_SHADOW_STC *portMapShadowPtr,
    OUT GT_U32                            portArgArr[CPSS_DXCH_PA_UNIT_MAX_E]
)
{
    GT_U32 i;
#ifndef GM_USED
    PRV_CPSS_GEN_PP_CONFIG_STC *pDev;
#endif

    PRV_CPSS_DXCH_DEV_CHECK_MAC(devNum);
    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum, CPSS_CH1_E | CPSS_CH1_DIAMOND_E
                                            | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E | CPSS_XCAT3_E
                                            | CPSS_LION_E | CPSS_LION2_E | CPSS_XCAT2_E);




#ifndef GM_USED
    pDev = PRV_CPSS_PP_MAC(devNum);
    if (pDev->devSubFamily != CPSS_PP_SUB_FAMILY_NONE_E)
    {
        return GT_FAIL;
    }
#endif

    for (i = 0; i < CPSS_DXCH_PA_UNIT_MAX_E; i++)
    {
        portArgArr[i] = CPSS_DXCH_PORT_MAPPING_INVALID_PORT_CNS;
    }
    portArgArr[CPSS_DXCH_PA_UNIT_RXDMA_E        ] = portMapShadowPtr->portMap.rxDmaNum;
    portArgArr[CPSS_DXCH_PA_UNIT_TXDMA_E        ] = portMapShadowPtr->portMap.txDmaNum;
    portArgArr[CPSS_DXCH_PA_UNIT_TXQ_E          ] = portMapShadowPtr->portMap.txqNum;
    portArgArr[CPSS_DXCH_PA_UNIT_TX_FIFO_E      ] = portMapShadowPtr->portMap.txDmaNum;
    portArgArr[CPSS_DXCH_PA_UNIT_ETH_TX_FIFO_E  ] = portMapShadowPtr->portMap.rxDmaNum;
    portArgArr[CPSS_DXCH_PA_UNIT_ILKN_TX_FIFO_E ] = portMapShadowPtr->portMap.ilknChannel;

    return GT_OK;
}


/* unit to be configured at ethernet/CPU/remote port wo TM */
static CPSS_DXCH_PA_UNIT_ENT prv_configureUnitList_BC2_noTM_RxDMA_TxDMA_TxQ_TxFIFO[] =
{
    CPSS_DXCH_PA_UNIT_RXDMA_E,
    CPSS_DXCH_PA_UNIT_TXDMA_E,
    CPSS_DXCH_PA_UNIT_TXQ_E,
    CPSS_DXCH_PA_UNIT_TX_FIFO_E,
    CPSS_DXCH_PA_UNIT_UNDEFINED_E
};

/* unit to be configured at ethernet/CPU/remote port with TM */
static CPSS_DXCH_PA_UNIT_ENT prv_configureUnitList_BC2_withTM_RxDMA_TxDMA_TxQ_TxFIFO_EthTxFIFO[] =
{
    CPSS_DXCH_PA_UNIT_RXDMA_E,
    CPSS_DXCH_PA_UNIT_TXDMA_E,
    CPSS_DXCH_PA_UNIT_TXQ_E,
    CPSS_DXCH_PA_UNIT_TX_FIFO_E,
    CPSS_DXCH_PA_UNIT_ETH_TX_FIFO_E,
    CPSS_DXCH_PA_UNIT_UNDEFINED_E
};

static PRV_CPSS_DXCH_MAPPING_TM_2_UNITLIST_STC prv_bc2_mapping_tm_2_unitlist[] = 
{
     { CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E         , GT_FALSE     ,   &prv_configureUnitList_BC2_noTM_RxDMA_TxDMA_TxQ_TxFIFO            [0] }
    ,{ CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E         , GT_TRUE      ,   &prv_configureUnitList_BC2_withTM_RxDMA_TxDMA_TxQ_TxFIFO_EthTxFIFO[0] }
    ,{ CPSS_DXCH_PORT_MAPPING_TYPE_CPU_SDMA_E             , GT_FALSE     ,   &prv_configureUnitList_BC2_noTM_RxDMA_TxDMA_TxQ_TxFIFO            [0] }
    ,{ CPSS_DXCH_PORT_MAPPING_TYPE_CPU_SDMA_E             , GT_TRUE      ,   &prv_configureUnitList_BC2_withTM_RxDMA_TxDMA_TxQ_TxFIFO_EthTxFIFO[0] }
    ,{ CPSS_DXCH_PORT_MAPPING_TYPE_ILKN_CHANNEL_E         , GT_FALSE     ,   &prv_configureUnitList_BC2_noTM_RxDMA_TxDMA_TxQ_TxFIFO            [0] }
    ,{ CPSS_DXCH_PORT_MAPPING_TYPE_ILKN_CHANNEL_E         , GT_TRUE      ,   &prv_configureUnitList_BC2_noTM_RxDMA_TxDMA_TxQ_TxFIFO            [0] }
    ,{ CPSS_DXCH_PORT_MAPPING_TYPE_REMOTE_PHYSICAL_PORT_E , GT_FALSE     ,   &prv_configureUnitList_BC2_noTM_RxDMA_TxDMA_TxQ_TxFIFO            [0] }
    ,{ CPSS_DXCH_PORT_MAPPING_TYPE_REMOTE_PHYSICAL_PORT_E , GT_TRUE      ,   &prv_configureUnitList_BC2_withTM_RxDMA_TxDMA_TxQ_TxFIFO_EthTxFIFO[0] }
    ,{ CPSS_DXCH_PORT_MAPPING_TYPE_INVALID_E              , (GT_U32)(~0) ,   (CPSS_DXCH_PA_UNIT_ENT *)NULL                                         }
};

static PRV_CPSS_DXCH_UNIT_2_PIZZA_FUN_STC prv_bc2_unit_2_pizzaFun[] = 
{
     { CPSS_DXCH_PA_UNIT_RXDMA_E        , BuildPizzaDistribution            }
    ,{ CPSS_DXCH_PA_UNIT_TXDMA_E        , BuildPizzaDistribution            }
    ,{ CPSS_DXCH_PA_UNIT_TXQ_E          , BuildTxQPizzaDistribution         }
    ,{ CPSS_DXCH_PA_UNIT_TX_FIFO_E      , BuildPizzaDistribution            }
    ,{ CPSS_DXCH_PA_UNIT_ETH_TX_FIFO_E  , BuildPizzaDistribution            }
    ,{ CPSS_DXCH_PA_UNIT_ILKN_TX_FIFO_E , BuildPizzaDistribution            }
    ,{ CPSS_DXCH_PA_UNIT_UNDEFINED_E    , (BuildPizzaDistributionFUN)NULL   }
};

/*-------------------------------------------------------------*
 *  BobK Caelum Pipe-0 pipe-1                                  *
 *-------------------------------------------------------------*/
static CPSS_DXCH_PA_UNIT_ENT   prv_bobk_Caelum_Pipe0_Pipe1_UnitList[] =  /* list of units used in BobK pipe 0 + pipe 1 (Caelum) */
{
     CPSS_DXCH_PA_UNIT_RXDMA_E
    ,CPSS_DXCH_PA_UNIT_TXDMA_E
    ,CPSS_DXCH_PA_UNIT_TX_FIFO_E
    ,CPSS_DXCH_PA_UNIT_ETH_TX_FIFO_E
    ,CPSS_DXCH_PA_UNIT_TXQ_E
    ,CPSS_DXCH_PA_UNIT_RXDMA_1_E
    ,CPSS_DXCH_PA_UNIT_TXDMA_1_E
    ,CPSS_DXCH_PA_UNIT_TX_FIFO_1_E
    ,CPSS_DXCH_PA_UNIT_ETH_TX_FIFO_1_E
    ,CPSS_DXCH_PA_UNIT_RXDMA_GLUE_E
    ,CPSS_DXCH_PA_UNIT_TXDMA_GLUE_E
    ,CPSS_DXCH_PA_UNIT_MPPM_E
    ,CPSS_DXCH_PA_UNIT_UNDEFINED_E           /* last unit */
};

static CPSS_DXCH_PA_UNIT_ENT   prv_bobk_Caelum_pipe0_pipe1_unitListConfByPipeBwSetList[] = 
{
     CPSS_DXCH_PA_UNIT_TXQ_E
    ,CPSS_DXCH_PA_UNIT_RXDMA_1_E
    ,CPSS_DXCH_PA_UNIT_TXDMA_1_E
    ,CPSS_DXCH_PA_UNIT_TX_FIFO_1_E
    ,CPSS_DXCH_PA_UNIT_ETH_TX_FIFO_1_E
    ,CPSS_DXCH_PA_UNIT_RXDMA_GLUE_E
    ,CPSS_DXCH_PA_UNIT_TXDMA_GLUE_E
    ,CPSS_DXCH_PA_UNIT_UNDEFINED_E           /* last unit */
};


static PRV_CPSS_DXCH_PA_SPEEDENM_SPEEDGBPS_STC prv_bobk_speedEnm2Gbps[] = 
{
     { CPSS_PORT_SPEED_1000_E ,  1 }
    ,{ CPSS_PORT_SPEED_2500_E ,  3 }
    ,{ CPSS_PORT_SPEED_10000_E, 10 }
    ,{ CPSS_PORT_SPEED_12000_E, 12 }
    ,{ CPSS_PORT_SPEED_20000_E, 20 }
    ,{ CPSS_PORT_SPEED_40000_E, 40 }
    ,{ CPSS_PORT_SPEED_11800_E, 12 }
    ,{ CPSS_PORT_SPEED_47200_E, 48 }
    ,{ CPSS_PORT_SPEED_NA_E,     0 }
};

/* unit to be configured at ethernet/CPU/remote port wo TM */
static CPSS_DXCH_PA_UNIT_ENT prv_configureUnitList_BobK_noTM_RxDMA_01_TxDMA_01_TxQ_TxFIFO_01[] =
{
     CPSS_DXCH_PA_UNIT_RXDMA_E
    ,CPSS_DXCH_PA_UNIT_TXDMA_E
    ,CPSS_DXCH_PA_UNIT_TX_FIFO_E
    ,CPSS_DXCH_PA_UNIT_RXDMA_1_E
    ,CPSS_DXCH_PA_UNIT_TXDMA_1_E
    ,CPSS_DXCH_PA_UNIT_TX_FIFO_1_E
    ,CPSS_DXCH_PA_UNIT_TXQ_E
    ,CPSS_DXCH_PA_UNIT_RXDMA_GLUE_E
    ,CPSS_DXCH_PA_UNIT_TXDMA_GLUE_E
    ,CPSS_DXCH_PA_UNIT_MPPM_E 
    ,CPSS_DXCH_PA_UNIT_UNDEFINED_E
};

/* unit to be configured at ethernet/CPU/remote port with TM */
static CPSS_DXCH_PA_UNIT_ENT prv_configureUnitList_BobK_withTM_RxDMA_01_TxDMA_01_TxQ_TxFIFO_01_EthTxFIFO_01[] =
{
     CPSS_DXCH_PA_UNIT_RXDMA_E
    ,CPSS_DXCH_PA_UNIT_TXDMA_E
    ,CPSS_DXCH_PA_UNIT_TX_FIFO_E
    ,CPSS_DXCH_PA_UNIT_ETH_TX_FIFO_E
    ,CPSS_DXCH_PA_UNIT_RXDMA_1_E
    ,CPSS_DXCH_PA_UNIT_TXDMA_1_E
    ,CPSS_DXCH_PA_UNIT_TX_FIFO_1_E
    ,CPSS_DXCH_PA_UNIT_ETH_TX_FIFO_1_E
    ,CPSS_DXCH_PA_UNIT_TXQ_E
    ,CPSS_DXCH_PA_UNIT_RXDMA_GLUE_E
    ,CPSS_DXCH_PA_UNIT_TXDMA_GLUE_E
    ,CPSS_DXCH_PA_UNIT_MPPM_E
    ,CPSS_DXCH_PA_UNIT_UNDEFINED_E
};

static PRV_CPSS_DXCH_MAPPING_TM_2_UNITLIST_STC prv_bobk_Caelum_pipe0_pipe1_mapping_tm_2_unitlist[] = 
{
     { CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E         , GT_FALSE     , &prv_configureUnitList_BobK_noTM_RxDMA_01_TxDMA_01_TxQ_TxFIFO_01               [0] }
    ,{ CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E         , GT_TRUE      , &prv_configureUnitList_BobK_withTM_RxDMA_01_TxDMA_01_TxQ_TxFIFO_01_EthTxFIFO_01[0] }
    ,{ CPSS_DXCH_PORT_MAPPING_TYPE_CPU_SDMA_E             , GT_FALSE     , &prv_configureUnitList_BobK_noTM_RxDMA_01_TxDMA_01_TxQ_TxFIFO_01               [0] }
    ,{ CPSS_DXCH_PORT_MAPPING_TYPE_CPU_SDMA_E             , GT_TRUE      , &prv_configureUnitList_BobK_withTM_RxDMA_01_TxDMA_01_TxQ_TxFIFO_01_EthTxFIFO_01[0] }
    ,{ CPSS_DXCH_PORT_MAPPING_TYPE_REMOTE_PHYSICAL_PORT_E , GT_FALSE     , &prv_configureUnitList_BobK_noTM_RxDMA_01_TxDMA_01_TxQ_TxFIFO_01               [0] }
    ,{ CPSS_DXCH_PORT_MAPPING_TYPE_REMOTE_PHYSICAL_PORT_E , GT_TRUE      , &prv_configureUnitList_BobK_withTM_RxDMA_01_TxDMA_01_TxQ_TxFIFO_01_EthTxFIFO_01[0] }
    ,{ CPSS_DXCH_PORT_MAPPING_TYPE_INVALID_E              , (GT_U32)(~0) , (CPSS_DXCH_PA_UNIT_ENT *)NULL                                                      }
};


static GT_STATUS  prvCpssDxChBobK_Caelum_Pipe0_Pipe1_Mapping2UnitConvFun
(
    IN GT_U8                              devNum,
    IN PRV_CPSS_DXCH_PORT_MAP_SHADOW_STC *portMapShadowPtr,
    OUT GT_U32                            portArgArr[CPSS_DXCH_PA_UNIT_MAX_E]
)
{
    GT_U32 i;
#ifndef GM_USED
    PRV_CPSS_DXCH_PP_CONFIG_STC *pDev;
#endif

    PRV_CPSS_DXCH_DEV_CHECK_MAC(devNum);
    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum, CPSS_CH1_E | CPSS_CH1_DIAMOND_E
                                            | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E | CPSS_XCAT3_E
                                            | CPSS_LION_E | CPSS_LION2_E | CPSS_XCAT2_E);


#ifndef GM_USED
    pDev = PRV_CPSS_DXCH_PP_MAC(devNum);
    if (pDev->genInfo.devSubFamily != CPSS_PP_SUB_FAMILY_BOBCAT2_BOBK_E)
    {
        return GT_FAIL;
    }
#endif

    for (i = 0; i < CPSS_DXCH_PA_UNIT_MAX_E; i++)
    {
        portArgArr[i] = CPSS_DXCH_PORT_MAPPING_INVALID_PORT_CNS;
    }


    if (portMapShadowPtr->portMap.rxDmaNum <= PRV_CPSS_DXCH_BC2_BOBK_PIPE0_LAST_MAC_CNS)
    {
        portArgArr[CPSS_DXCH_PA_UNIT_RXDMA_E        ] = portMapShadowPtr->portMap.rxDmaNum;
        portArgArr[CPSS_DXCH_PA_UNIT_ETH_TX_FIFO_E  ] = portMapShadowPtr->portMap.rxDmaNum;
        portArgArr[CPSS_DXCH_PA_UNIT_RXDMA_GLUE_E   ] = 0;
    }
    else
    {
        portArgArr[CPSS_DXCH_PA_UNIT_RXDMA_1_E      ] = portMapShadowPtr->portMap.rxDmaNum;
        portArgArr[CPSS_DXCH_PA_UNIT_ETH_TX_FIFO_1_E] = portMapShadowPtr->portMap.rxDmaNum;
        portArgArr[CPSS_DXCH_PA_UNIT_RXDMA_GLUE_E   ] = 1;
    }

    if (portMapShadowPtr->portMap.txDmaNum <= PRV_CPSS_DXCH_BC2_BOBK_PIPE0_LAST_MAC_CNS)
    {
        portArgArr[CPSS_DXCH_PA_UNIT_TXDMA_E        ] = portMapShadowPtr->portMap.txDmaNum;
        portArgArr[CPSS_DXCH_PA_UNIT_TX_FIFO_E      ] = portMapShadowPtr->portMap.txDmaNum;
        portArgArr[CPSS_DXCH_PA_UNIT_TXDMA_GLUE_E   ] = 0;

    }
    else 
    {
        portArgArr[CPSS_DXCH_PA_UNIT_TXDMA_1_E      ] = portMapShadowPtr->portMap.txDmaNum;
        portArgArr[CPSS_DXCH_PA_UNIT_TX_FIFO_1_E    ] = portMapShadowPtr->portMap.txDmaNum;
        portArgArr[CPSS_DXCH_PA_UNIT_TXDMA_GLUE_E   ] = 1;
    }
    portArgArr[CPSS_DXCH_PA_UNIT_TXQ_E          ] = portMapShadowPtr->portMap.txqNum;
    return GT_OK;
}

static PRV_CPSS_DXCH_UNIT_2_PIZZA_FUN_STC prv_bobk_Caelum_pipe0_pipe1_unit_2_pizzaFun[] = 
{
     { CPSS_DXCH_PA_UNIT_RXDMA_E         ,  BuildPizzaDistributionWithDummySlice         }
    ,{ CPSS_DXCH_PA_UNIT_TXDMA_E         ,  BuildPizzaDistributionWithDummySlice         }
    ,{ CPSS_DXCH_PA_UNIT_TX_FIFO_E       ,  BuildPizzaDistributionWithDummySlice         }
    ,{ CPSS_DXCH_PA_UNIT_ETH_TX_FIFO_E   ,  BuildPizzaDistributionWithDummySlice         }
    ,{ CPSS_DXCH_PA_UNIT_TXQ_E           ,  BuildTxQPizzaDistribution                    }
    ,{ CPSS_DXCH_PA_UNIT_RXDMA_1_E       ,  BuildPizzaDistributionWithDummySlice         }
    ,{ CPSS_DXCH_PA_UNIT_TXDMA_1_E       ,  BuildPizzaDistributionWithDummySlice         }
    ,{ CPSS_DXCH_PA_UNIT_TX_FIFO_1_E     ,  BuildPizzaDistributionWithDummySlice         }
    ,{ CPSS_DXCH_PA_UNIT_ETH_TX_FIFO_1_E ,  BuildPizzaDistributionWithDummySlice         }
    ,{ CPSS_DXCH_PA_UNIT_RXDMA_GLUE_E    ,  BuildPizzaDistribution                       }
    ,{ CPSS_DXCH_PA_UNIT_TXDMA_GLUE_E    ,  BuildPizzaDistribution                       }
    ,{ CPSS_DXCH_PA_UNIT_MPPM_E          ,  BuildPizzaDistribution                       }
    ,{ CPSS_DXCH_PA_UNIT_UNDEFINED_E     , (BuildPizzaDistributionFUN )NULL              }
};

PRV_CPSS_DXCH_MPPM_CLIENT_CODING_STC     prv_bobk_Caelum_pipe0_pipe1_mppmCoding[] = 
{
     /* unit                           client code ,   client weight */
     {CPSS_DXCH_PA_UNIT_RXDMA_E     ,            0 ,            1 }
    ,{CPSS_DXCH_PA_UNIT_RXDMA_1_E   ,            1 ,            1 }
    ,{CPSS_DXCH_PA_UNIT_TXDMA_E     ,            2 ,            1 }
    ,{CPSS_DXCH_PA_UNIT_TXDMA_1_E   ,            3 ,            1 }
    ,{CPSS_DXCH_PA_UNIT_UNDEFINED_E , (GT_U32)(~0) ,  (GT_U32)(~0)}
};

/*-------------------------------------------------------------*
 *  BobK Cetus pipe-1                                          *
 *-------------------------------------------------------------*/
static CPSS_DXCH_PA_UNIT_ENT   prv_bobk_Cetus_Pipe_1_UnitList[] =  /* list of units used in BobK pipe 1 (Cetus) */
{
     CPSS_DXCH_PA_UNIT_TXQ_E
    ,CPSS_DXCH_PA_UNIT_RXDMA_1_E
    ,CPSS_DXCH_PA_UNIT_TXDMA_1_E
    ,CPSS_DXCH_PA_UNIT_TX_FIFO_1_E
    ,CPSS_DXCH_PA_UNIT_ETH_TX_FIFO_1_E
    ,CPSS_DXCH_PA_UNIT_RXDMA_GLUE_E
    ,CPSS_DXCH_PA_UNIT_TXDMA_GLUE_E
    ,CPSS_DXCH_PA_UNIT_MPPM_E
    ,CPSS_DXCH_PA_UNIT_UNDEFINED_E           /* last unit */
};

static CPSS_DXCH_PA_UNIT_ENT   prv_bobk_Cetus_pipe1_unitListConfByPipeBwSetList[] = 
{
     CPSS_DXCH_PA_UNIT_TXQ_E
    ,CPSS_DXCH_PA_UNIT_RXDMA_1_E
    ,CPSS_DXCH_PA_UNIT_TXDMA_1_E
    ,CPSS_DXCH_PA_UNIT_TX_FIFO_1_E
    ,CPSS_DXCH_PA_UNIT_ETH_TX_FIFO_1_E
    ,CPSS_DXCH_PA_UNIT_RXDMA_GLUE_E
    ,CPSS_DXCH_PA_UNIT_TXDMA_GLUE_E
    ,CPSS_DXCH_PA_UNIT_UNDEFINED_E           /* last unit */
};


/* unit to be configured at ethernet/CPU/remote port wo TM */
static CPSS_DXCH_PA_UNIT_ENT prv_configureUnitList_BobK_Cetus_Pipe1_noTM_RxDMA_1_TxDMA_1_TxQ_TxFIFO_1[] =
{
     CPSS_DXCH_PA_UNIT_RXDMA_E
    ,CPSS_DXCH_PA_UNIT_TXDMA_E
    ,CPSS_DXCH_PA_UNIT_TX_FIFO_E
    ,CPSS_DXCH_PA_UNIT_RXDMA_1_E
    ,CPSS_DXCH_PA_UNIT_TXDMA_1_E
    ,CPSS_DXCH_PA_UNIT_TX_FIFO_1_E
    ,CPSS_DXCH_PA_UNIT_TXQ_E
    ,CPSS_DXCH_PA_UNIT_RXDMA_GLUE_E
    ,CPSS_DXCH_PA_UNIT_TXDMA_GLUE_E
    ,CPSS_DXCH_PA_UNIT_MPPM_E 
    ,CPSS_DXCH_PA_UNIT_UNDEFINED_E
};

/* unit to be configured at ethernet/CPU/remote port with TM */
static CPSS_DXCH_PA_UNIT_ENT prv_configureUnitList_BobK_Cetus_Pipe1_withTM_RxDMA_1_TxDMA_1_TxQ_TxFIFO_1_EthTxFIFO_1[] =
{
     CPSS_DXCH_PA_UNIT_RXDMA_1_E
    ,CPSS_DXCH_PA_UNIT_TXDMA_1_E
    ,CPSS_DXCH_PA_UNIT_TX_FIFO_1_E
    ,CPSS_DXCH_PA_UNIT_ETH_TX_FIFO_1_E
    ,CPSS_DXCH_PA_UNIT_TXQ_E
    ,CPSS_DXCH_PA_UNIT_RXDMA_GLUE_E
    ,CPSS_DXCH_PA_UNIT_TXDMA_GLUE_E
    ,CPSS_DXCH_PA_UNIT_MPPM_E
    ,CPSS_DXCH_PA_UNIT_UNDEFINED_E
};

static PRV_CPSS_DXCH_MAPPING_TM_2_UNITLIST_STC prv_bobk_Cetus_pipe1_mapping_tm_2_unitlist[] = 
{
     { CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E         , GT_FALSE     , &prv_configureUnitList_BobK_Cetus_Pipe1_noTM_RxDMA_1_TxDMA_1_TxQ_TxFIFO_1              [0] }
    ,{ CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E         , GT_TRUE      , &prv_configureUnitList_BobK_Cetus_Pipe1_withTM_RxDMA_1_TxDMA_1_TxQ_TxFIFO_1_EthTxFIFO_1[0] }
    ,{ CPSS_DXCH_PORT_MAPPING_TYPE_CPU_SDMA_E             , GT_FALSE     , &prv_configureUnitList_BobK_Cetus_Pipe1_noTM_RxDMA_1_TxDMA_1_TxQ_TxFIFO_1              [0] }
    ,{ CPSS_DXCH_PORT_MAPPING_TYPE_CPU_SDMA_E             , GT_TRUE      , &prv_configureUnitList_BobK_Cetus_Pipe1_withTM_RxDMA_1_TxDMA_1_TxQ_TxFIFO_1_EthTxFIFO_1[0] }
    ,{ CPSS_DXCH_PORT_MAPPING_TYPE_REMOTE_PHYSICAL_PORT_E , GT_FALSE     , &prv_configureUnitList_BobK_Cetus_Pipe1_noTM_RxDMA_1_TxDMA_1_TxQ_TxFIFO_1              [0] }
    ,{ CPSS_DXCH_PORT_MAPPING_TYPE_REMOTE_PHYSICAL_PORT_E , GT_TRUE      , &prv_configureUnitList_BobK_Cetus_Pipe1_withTM_RxDMA_1_TxDMA_1_TxQ_TxFIFO_1_EthTxFIFO_1[0] }
    ,{ CPSS_DXCH_PORT_MAPPING_TYPE_INVALID_E              , (GT_U32)(~0) , (CPSS_DXCH_PA_UNIT_ENT *)NULL                                                              }
};


static GT_STATUS  prvCpssDxChBobK_Cetus_Pipe1_Mapping2UnitConvFun
(
    IN GT_U8                              devNum,
    IN PRV_CPSS_DXCH_PORT_MAP_SHADOW_STC *portMapShadowPtr,
    OUT GT_U32                            portArgArr[CPSS_DXCH_PA_UNIT_MAX_E]
)
{
    GT_U32 i;
    PRV_CPSS_DXCH_PP_CONFIG_STC *pDev;

    PRV_CPSS_DXCH_DEV_CHECK_MAC(devNum);
    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum, CPSS_CH1_E | CPSS_CH1_DIAMOND_E
                                            | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E | CPSS_XCAT3_E
                                            | CPSS_LION_E | CPSS_LION2_E | CPSS_XCAT2_E);


    pDev = PRV_CPSS_DXCH_PP_MAC(devNum);

    if (pDev->genInfo.devSubFamily != CPSS_PP_SUB_FAMILY_BOBCAT2_BOBK_E)
    {
        return GT_FAIL;
    }

    for (i = 0; i < CPSS_DXCH_PA_UNIT_MAX_E; i++)
    {
        portArgArr[i] = CPSS_DXCH_PORT_MAPPING_INVALID_PORT_CNS;
    }


    if (portMapShadowPtr->portMap.rxDmaNum <= PRV_CPSS_DXCH_BC2_BOBK_PIPE0_LAST_MAC_CNS)
    {
        return GT_FAIL;
    }
    else
    {
        portArgArr[CPSS_DXCH_PA_UNIT_RXDMA_1_E      ] = portMapShadowPtr->portMap.rxDmaNum;
        portArgArr[CPSS_DXCH_PA_UNIT_ETH_TX_FIFO_1_E] = portMapShadowPtr->portMap.rxDmaNum;
        portArgArr[CPSS_DXCH_PA_UNIT_RXDMA_GLUE_E   ] = 1;
    }

    if (portMapShadowPtr->portMap.txDmaNum <= PRV_CPSS_DXCH_BC2_BOBK_PIPE0_LAST_MAC_CNS)
    {
        return GT_FAIL;
    }
    else 
    {
        portArgArr[CPSS_DXCH_PA_UNIT_TXDMA_1_E      ] = portMapShadowPtr->portMap.txDmaNum;
        portArgArr[CPSS_DXCH_PA_UNIT_TX_FIFO_1_E    ] = portMapShadowPtr->portMap.txDmaNum;
        portArgArr[CPSS_DXCH_PA_UNIT_TXDMA_GLUE_E   ] = 1;
    }
    portArgArr[CPSS_DXCH_PA_UNIT_TXQ_E          ] = portMapShadowPtr->portMap.txqNum;
    return GT_OK;
}

static PRV_CPSS_DXCH_UNIT_2_PIZZA_FUN_STC prv_bobk_Cetus_pipe1_unit_2_pizzaFun[] = 
{
     { CPSS_DXCH_PA_UNIT_TXQ_E           ,  BuildTxQPizzaDistribution              }
    ,{ CPSS_DXCH_PA_UNIT_RXDMA_1_E       ,  BuildPizzaDistributionWithDummySlice   }
    ,{ CPSS_DXCH_PA_UNIT_TXDMA_1_E       ,  BuildPizzaDistributionWithDummySlice   }
    ,{ CPSS_DXCH_PA_UNIT_TX_FIFO_1_E     ,  BuildPizzaDistributionWithDummySlice   }
    ,{ CPSS_DXCH_PA_UNIT_ETH_TX_FIFO_1_E ,  BuildPizzaDistributionWithDummySlice   }
    ,{ CPSS_DXCH_PA_UNIT_RXDMA_GLUE_E    ,  BuildPizzaDistribution                 }
    ,{ CPSS_DXCH_PA_UNIT_TXDMA_GLUE_E    ,  BuildPizzaDistribution                 }
    ,{ CPSS_DXCH_PA_UNIT_MPPM_E          ,  BuildPizzaDistribution                 }
    ,{ CPSS_DXCH_PA_UNIT_UNDEFINED_E     , (BuildPizzaDistributionFUN )NULL        }
};


PRV_CPSS_DXCH_MPPM_CLIENT_CODING_STC     prv_bobk_Cetus_pipe1_mppmCoding[] = 
{
     /* unit                           client code ,   client weight */
     {CPSS_DXCH_PA_UNIT_RXDMA_1_E   ,            1 ,            1 }
    ,{CPSS_DXCH_PA_UNIT_TXDMA_1_E   ,            3 ,            1 }
    ,{CPSS_DXCH_PA_UNIT_UNDEFINED_E , (GT_U32)(~0) ,  (GT_U32)(~0)}
};


/*----------------------------------------------------------*
 *  BC3                                                     *
 *----------------------------------------------------------*/
static CPSS_DXCH_PA_UNIT_ENT   prv_bc3_UnitList[] =            /* list of units used in BC3 just pipe 0 */
{
     CPSS_DXCH_PA_UNIT_RXDMA_E
    ,CPSS_DXCH_PA_UNIT_TXDMA_E
    ,CPSS_DXCH_PA_UNIT_TX_FIFO_E
    ,CPSS_DXCH_PA_UNIT_TXQ_E
    ,CPSS_DXCH_PA_UNIT_UNDEFINED_E           /* last unit */
};

static CPSS_DXCH_PA_UNIT_ENT   prv_bc3_unitListConfByPipeBwSetList[] = 
{
     CPSS_DXCH_PA_UNIT_RXDMA_E
    ,CPSS_DXCH_PA_UNIT_TXDMA_E
    ,CPSS_DXCH_PA_UNIT_TX_FIFO_E
    ,CPSS_DXCH_PA_UNIT_TXQ_E
    ,CPSS_DXCH_PA_UNIT_UNDEFINED_E           /* last unit */
};

static PRV_CPSS_DXCH_MAPPING_TM_2_UNITLIST_STC prv_bc3_mapping_tm_2_unitlist[] = 
{
     { CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E         , GT_FALSE     ,   &prv_configureUnitList_BC2_noTM_RxDMA_TxDMA_TxQ_TxFIFO            [0] }
    ,{ CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E         , GT_TRUE      ,   &prv_configureUnitList_BC2_noTM_RxDMA_TxDMA_TxQ_TxFIFO            [0] }
    ,{ CPSS_DXCH_PORT_MAPPING_TYPE_CPU_SDMA_E             , GT_FALSE     ,   &prv_configureUnitList_BC2_noTM_RxDMA_TxDMA_TxQ_TxFIFO            [0] }
    ,{ CPSS_DXCH_PORT_MAPPING_TYPE_CPU_SDMA_E             , GT_TRUE      ,   &prv_configureUnitList_BC2_noTM_RxDMA_TxDMA_TxQ_TxFIFO            [0] }
    ,{ CPSS_DXCH_PORT_MAPPING_TYPE_ILKN_CHANNEL_E         , GT_FALSE     ,   &prv_configureUnitList_BC2_noTM_RxDMA_TxDMA_TxQ_TxFIFO            [0] }
    ,{ CPSS_DXCH_PORT_MAPPING_TYPE_ILKN_CHANNEL_E         , GT_TRUE      ,   &prv_configureUnitList_BC2_noTM_RxDMA_TxDMA_TxQ_TxFIFO            [0] }
    ,{ CPSS_DXCH_PORT_MAPPING_TYPE_REMOTE_PHYSICAL_PORT_E , GT_FALSE     ,   &prv_configureUnitList_BC2_noTM_RxDMA_TxDMA_TxQ_TxFIFO            [0] }
    ,{ CPSS_DXCH_PORT_MAPPING_TYPE_REMOTE_PHYSICAL_PORT_E , GT_TRUE      ,   &prv_configureUnitList_BC2_noTM_RxDMA_TxDMA_TxQ_TxFIFO            [0] }
    ,{ CPSS_DXCH_PORT_MAPPING_TYPE_INVALID_E              , (GT_U32)(~0) ,   (CPSS_DXCH_PA_UNIT_ENT *)NULL                                         }
};

static PRV_CPSS_DXCH_UNIT_2_PIZZA_FUN_STC prv_bc3_unit_2_pizzaFun[] = 
{
     { CPSS_DXCH_PA_UNIT_RXDMA_E        , BuildPizzaDistribution            }
    ,{ CPSS_DXCH_PA_UNIT_TXDMA_E        , BuildPizzaDistribution            }
    ,{ CPSS_DXCH_PA_UNIT_TXQ_E          , BuildTxQPizzaDistribution         }
    ,{ CPSS_DXCH_PA_UNIT_TX_FIFO_E      , BuildPizzaDistribution            }
    ,{ CPSS_DXCH_PA_UNIT_UNDEFINED_E    , (BuildPizzaDistributionFUN)NULL   }
};

static GT_STATUS  prvCpssDxChBc3Mapping2UnitConvFun
(
    IN GT_U8                              devNum,
    IN PRV_CPSS_DXCH_PORT_MAP_SHADOW_STC *portMapShadowPtr,
    OUT GT_U32                            portArgArr[CPSS_DXCH_PA_UNIT_MAX_E]
)
{
    GT_U32 i;

    PRV_CPSS_GEN_PP_CONFIG_STC *pDev;

    PRV_CPSS_DXCH_DEV_CHECK_MAC(devNum);
    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum, CPSS_CH1_E | CPSS_CH1_DIAMOND_E
                                            | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E | CPSS_XCAT3_E
                                            | CPSS_LION_E | CPSS_LION2_E | CPSS_XCAT2_E);



    pDev = PRV_CPSS_PP_MAC(devNum);

    if (pDev->devFamily != CPSS_PP_FAMILY_DXCH_BOBCAT3_E)
    {
        return GT_FAIL;
    }

    for (i = 0; i < CPSS_DXCH_PA_UNIT_MAX_E; i++)
    {
        portArgArr[i] = CPSS_DXCH_PORT_MAPPING_INVALID_PORT_CNS;
    }
    portArgArr[CPSS_DXCH_PA_UNIT_RXDMA_E        ] = portMapShadowPtr->portMap.rxDmaNum;
    portArgArr[CPSS_DXCH_PA_UNIT_TXDMA_E        ] = portMapShadowPtr->portMap.txDmaNum;
    portArgArr[CPSS_DXCH_PA_UNIT_TXQ_E          ] = portMapShadowPtr->portMap.txqNum;
    portArgArr[CPSS_DXCH_PA_UNIT_TX_FIFO_E      ] = portMapShadowPtr->portMap.txDmaNum;
    return GT_OK;
}


/*----------------------------------------------------------
 *  WS related function
 *----------------------------------------------------------*/
GT_BOOL prvCpssDxChPortPaWorSpaceInitStatusGet
(
    INOUT PRV_CPSS_DXCH_PA_WS_SET_STC     *paWsPtr
)
{
    return paWsPtr->isInit;
}

GT_VOID prvCpssDxChPortPaWorSpaceInitStatusSet
(
    INOUT PRV_CPSS_DXCH_PA_WS_SET_STC     *paWsPtr
)
{
    paWsPtr->isInit = GT_TRUE;
}


GT_VOID prvCpssDxChPortDynamicPizzaArbiterWSByParamsInit
(
    INOUT PRV_CPSS_DXCH_PA_WORKSPACE_STC * paWs,
    IN    PRV_CPSS_DXCH_PA_WS_INIT_PARAMS_STC * wsInitParamsPtr   
)
{
    GT_U32      unitIdx;
    CPSS_DXCH_PA_UNIT_ENT unitType;
    GT_U32      i;
    /*-----------------------------------------------------------------------------------*/
    /* BC2  workspace with ILKN init                                                               */
    /*-----------------------------------------------------------------------------------*/
    cpssOsMemSet(paWs->prv_speedEnt2MBitConvArr,  0,sizeof(paWs->prv_speedEnt2MBitConvArr));
    for (i = 0 ; prv_bc2_speedEnm2Gbps[i].speedEnm != CPSS_PORT_SPEED_NA_E; i++)
    {
        CPSS_PORT_SPEED_ENT speedEnm;
        GT_U32              speedGbps;

        speedEnm  = prv_bc2_speedEnm2Gbps[i].speedEnm;
        speedGbps = prv_bc2_speedEnm2Gbps[i].speedGbps;

        paWs->prv_speedEnt2MBitConvArr[speedEnm] = PRV_CPSS_DXCH_PA_BW_COEFF*speedGbps;
    }

    paWs->prv_DeviceUnitListPtr = wsInitParamsPtr->supportedUnitListPtr;
    paWs->prv_unitListConfigByPipeBwSetPtr = wsInitParamsPtr->unitListConfByPipeBwSetListPtr;

    /*-----------------------------*/
    /* init map of supported units */
    /*-----------------------------*/
    for (unitIdx = 0 ; unitIdx < CPSS_DXCH_PA_UNIT_MAX_E; unitIdx++)
    {
        paWs->prv_DeviceUnitListBmp[unitIdx]             = GT_FALSE;
        paWs->prv_unitListConfigByPipeBwSetBmp[unitIdx]  = GT_FALSE;
    }
    for (unitIdx = 0 ; paWs->prv_DeviceUnitListPtr[unitIdx] != CPSS_DXCH_PA_UNIT_UNDEFINED_E; unitIdx++)
    {
        unitType = paWs->prv_DeviceUnitListPtr[unitIdx];
        paWs->prv_DeviceUnitListBmp[unitType] = GT_TRUE;
    }

    for (unitIdx = 0 ; wsInitParamsPtr->unitListConfByPipeBwSetListPtr[unitIdx] != CPSS_DXCH_PA_UNIT_UNDEFINED_E; unitIdx++)
    {
        unitType = wsInitParamsPtr->unitListConfByPipeBwSetListPtr[unitIdx];
        paWs->prv_unitListConfigByPipeBwSetBmp[unitType] = GT_TRUE;
    }

    paWs->mapping2unitConvFunPtr = wsInitParamsPtr->mapping2UnitConvFun;

    /*-----------------------------------------------------------------------------------*/
    /* define list of configuration units for each mapping type  x TM usage(True/False)  */
    /* for interlaken channel Eth_TX_FIFO not needed                                     */
    /*-----------------------------------------------------------------------------------*/
    cpssOsMemSet(&paWs->prv_mappingType2UnitConfArr,0,sizeof(paWs->prv_mappingType2UnitConfArr));
    for (i = 0 ; wsInitParamsPtr->mappingTm2UnitListPtr[i].mappingTye != CPSS_DXCH_PORT_MAPPING_TYPE_INVALID_E ; i++)
    {
        CPSS_DXCH_PORT_MAPPING_TYPE_ENT mappingType = prv_bc2_mapping_tm_2_unitlist[i].mappingTye;
        GT_U32                          tmEnable    = prv_bc2_mapping_tm_2_unitlist[i].tmEnable;
        paWs->prv_mappingType2UnitConfArr[mappingType][tmEnable] =  wsInitParamsPtr->mappingTm2UnitListPtr[i].unitList2ConfigurePtr;
    }

    cpssOsMemSet(&paWs->prv_unit2PizzaAlgoFunArr,0,sizeof(paWs->prv_unit2PizzaAlgoFunArr));
    for ( i = 0 ; wsInitParamsPtr->unit_2_pizzaCompFun[i].unit !=  CPSS_DXCH_PA_UNIT_UNDEFINED_E ; i++)
    {
        BuildPizzaDistributionFUN fun;
        unitType = wsInitParamsPtr->unit_2_pizzaCompFun[i].unit;
        fun      = wsInitParamsPtr->unit_2_pizzaCompFun[i].fun;
        paWs->prv_unit2PizzaAlgoFunArr[unitType] = fun;
    }
    cpssOsMemSet(&paWs->prv_unit2HighSpeedPortConfFunArr,0,sizeof(&paWs->prv_unit2HighSpeedPortConfFunArr));
    paWs->prv_unit2HighSpeedPortConfFunArr[ CPSS_DXCH_PA_UNIT_TXQ_E ] = wsInitParamsPtr->txQHighSpeedPortSetFun;
    /*----------------------------------------*
     * MPPM coding scheme                     *
     *----------------------------------------*/
    paWs->mppmClientCodingListPtr =  wsInitParamsPtr->mppmClientCodingList;             
    paWs->txqWorkConservingModeOn = wsInitParamsPtr->txqWorkConservingModeOn;
}


static PRV_CPSS_DXCH_PA_WS_INIT_PARAMS_STC        prv_bc2_ws_init_params_no_ilkn = 
{
    /* CPSS_DXCH_PA_UNIT_ENT                     *supportedUnitListPtr  */ &prv_bc2_UnitList_no_ilkn[0]
    /* CPSS_DXCH_PA_UNIT_ENT            *unitListConfByPipeBwSetListPtr */,&prv_bc2_no_ilkn_unitListConfByPipeBwSetList[0]
    /* PRV_CPSS_DXCH_PA_SPEEDENM_SPEEDGBPS_STC   *speedEnm2GbpsPtr;     */,&prv_bc2_speedEnm2Gbps[0]
    /* prvCpssDxChBc2Mapping2UnitConvFUN          mapping2UnitConvFun;  */,prvCpssDxChBc2Mapping2UnitConvFun
    /* PRV_CPSS_DXCH_MAPPING_TM_2_UNITLIST_STC   *mappingTm2UnitListPtr */,&prv_bc2_mapping_tm_2_unitlist[0]
    /* PRV_CPSS_DXCH_UNIT_2_PIZZA_FUN_STC        *unit_2_pizzaCompFun;  */,&prv_bc2_unit_2_pizzaFun[0]
    /* PRV_CPSS_DXCH_HIGH_SPEED_PORT_CONFIG_FUN  txQHighSpeedPortSetFun */,prvCpssDxChPortDynamicPATxQHighSpeedPortSet
    /* PRV_CPSS_DXCH_MPPM_CLIENT_CODING_STC      *mppmClientCodingList  */,NULL /* no MPPM */
    /* GT_BOOL                                  txqWorkConservingModeOn */,GT_FALSE
};

static PRV_CPSS_DXCH_PA_WS_INIT_PARAMS_STC        prv_bc2_ws_init_params_with_ilkn = 
{
    /* CPSS_DXCH_PA_UNIT_ENT                     *supportedUnitListPtr   */ &prv_bc2_UnitList_with_ilkn[0]
    /* CPSS_DXCH_PA_UNIT_ENT            *unitListConfByPipeBwSetListPtr  */,&prv_bc2_with_ilkn_unitListConfByPipeBwSetList[0]
    /* PRV_CPSS_DXCH_PA_SPEEDENM_SPEEDGBPS_STC   *speedEnm2GbpsPtr;      */,&prv_bc2_speedEnm2Gbps[0]
    /* prvCpssDxChBc2Mapping2UnitConvFUN          mapping2UnitConvFun;   */,prvCpssDxChBc2Mapping2UnitConvFun
    /* PRV_CPSS_DXCH_MAPPING_TM_2_UNITLIST_STC   *mappingTm2UnitListPtr  */,&prv_bc2_mapping_tm_2_unitlist[0]
    /* PRV_CPSS_DXCH_UNIT_2_PIZZA_FUN_STC        *unit_2_pizzaCompFun;   */,&prv_bc2_unit_2_pizzaFun[0]
    /* PRV_CPSS_DXCH_HIGH_SPEED_PORT_CONFIG_FUN   txQHighSpeedPortSetFun */,prvCpssDxChPortDynamicPATxQHighSpeedPortSet
    /* PRV_CPSS_DXCH_MPPM_CLIENT_CODING_STC      *mppmClientCodingList   */,NULL /* no MPPM */
    /* GT_BOOL                                  txqWorkConservingModeOn */ ,GT_FALSE
};


static PRV_CPSS_DXCH_PA_WS_INIT_PARAMS_STC        prv_bobk_caelum_pipe_0_pipe_1 = 
{
    /* CPSS_DXCH_PA_UNIT_ENT                     *supportedUnitListPtr   */ &prv_bobk_Caelum_Pipe0_Pipe1_UnitList[0]
    /* CPSS_DXCH_PA_UNIT_ENT            *unitListConfByPipeBwSetListPtr  */,&prv_bobk_Caelum_pipe0_pipe1_unitListConfByPipeBwSetList[0]
    /* PRV_CPSS_DXCH_PA_SPEEDENM_SPEEDGBPS_STC   *speedEnm2GbpsPtr;      */,&prv_bobk_speedEnm2Gbps[0]
    /* prvCpssDxChBc2Mapping2UnitConvFUN          mapping2UnitConvFun;   */,prvCpssDxChBobK_Caelum_Pipe0_Pipe1_Mapping2UnitConvFun
    /* PRV_CPSS_DXCH_MAPPING_TM_2_UNITLIST_STC   *mappingTm2UnitListPtr  */,&prv_bobk_Caelum_pipe0_pipe1_mapping_tm_2_unitlist[0]
    /* PRV_CPSS_DXCH_UNIT_2_PIZZA_FUN_STC        *unit_2_pizzaCompFun;   */,&prv_bobk_Caelum_pipe0_pipe1_unit_2_pizzaFun[0]
    /* PRV_CPSS_DXCH_HIGH_SPEED_PORT_CONFIG_FUN   txQHighSpeedPortSetFun */,prvCpssDxChPortDynamicPATxQHighSpeedPortSet
    /* PRV_CPSS_DXCH_MPPM_CLIENT_CODING_STC      *mppmClientCodingList   */,&prv_bobk_Caelum_pipe0_pipe1_mppmCoding[0] 
    /* GT_BOOL                                  txqWorkConservingModeOn */ ,GT_TRUE
};

static PRV_CPSS_DXCH_PA_WS_INIT_PARAMS_STC        prv_bobk_cetus_pipe_1 = 
{
    /* CPSS_DXCH_PA_UNIT_ENT                     *supportedUnitListPtr   */ &prv_bobk_Cetus_Pipe_1_UnitList[0]
    /* CPSS_DXCH_PA_UNIT_ENT            *unitListConfByPipeBwSetListPtr  */,&prv_bobk_Cetus_pipe1_unitListConfByPipeBwSetList[0]
    /* PRV_CPSS_DXCH_PA_SPEEDENM_SPEEDGBPS_STC   *speedEnm2GbpsPtr;      */,&prv_bobk_speedEnm2Gbps[0]
    /* prvCpssDxChBc2Mapping2UnitConvFUN          mapping2UnitConvFun;   */,prvCpssDxChBobK_Cetus_Pipe1_Mapping2UnitConvFun
    /* PRV_CPSS_DXCH_MAPPING_TM_2_UNITLIST_STC   *mappingTm2UnitListPtr  */,&prv_bobk_Cetus_pipe1_mapping_tm_2_unitlist[0]
    /* PRV_CPSS_DXCH_UNIT_2_PIZZA_FUN_STC        *unit_2_pizzaCompFun;   */,&prv_bobk_Cetus_pipe1_unit_2_pizzaFun[0]
    /* PRV_CPSS_DXCH_HIGH_SPEED_PORT_CONFIG_FUN   txQHighSpeedPortSetFun */,prvCpssDxChPortDynamicPATxQHighSpeedPortSet
    /* PRV_CPSS_DXCH_MPPM_CLIENT_CODING_STC      *mppmClientCodingList   */,&prv_bobk_Cetus_pipe1_mppmCoding[0] 
    /* GT_BOOL                                  txqWorkConservingModeOn */ ,GT_TRUE
};


static PRV_CPSS_DXCH_PA_WS_INIT_PARAMS_STC        prv_bc3_ws_init_params = 
{
    /* CPSS_DXCH_PA_UNIT_ENT                     *supportedUnitListPtr   */ &prv_bc3_UnitList[0]
    /* CPSS_DXCH_PA_UNIT_ENT            *unitListConfByPipeBwSetListPtr  */,&prv_bc3_unitListConfByPipeBwSetList[0]
    /* PRV_CPSS_DXCH_PA_SPEEDENM_SPEEDGBPS_STC   *speedEnm2GbpsPtr;      */,&prv_bc2_speedEnm2Gbps[0]
    /* prvCpssDxChBc2Mapping2UnitConvFUN          mapping2UnitConvFun;   */,prvCpssDxChBc3Mapping2UnitConvFun
    /* PRV_CPSS_DXCH_MAPPING_TM_2_UNITLIST_STC   *mappingTm2UnitListPtr  */,&prv_bc3_mapping_tm_2_unitlist[0]
    /* PRV_CPSS_DXCH_UNIT_2_PIZZA_FUN_STC        *unit_2_pizzaCompFun;   */,&prv_bc3_unit_2_pizzaFun[0]
    /* PRV_CPSS_DXCH_HIGH_SPEED_PORT_CONFIG_FUN   txQHighSpeedPortSetFun */,prvCpssDxChPortDynamicPATxQHighSpeedPortSet
    /* PRV_CPSS_DXCH_MPPM_CLIENT_CODING_STC      *mppmClientCodingList   */,NULL /* no MPPM */
    /* GT_BOOL                                  txqWorkConservingModeOn */ ,GT_TRUE
};


static PRV_CPSS_DXCH_PA_WS_SET_STC                prv_paWsSet;

/*******************************************************************************
* prvCpssDxChPortDynamicPizzaArbiterWSGet
*
* DESCRIPTION:
*       Get Pizza Work space (object) by device
*
* APPLICABLE DEVICES:
*        Bobcat2; Caelum; Bobcat3.
*
* NOT APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; Lion; Lion2; xCat2.
*
* INPUTS:
*       devNum   - device number
*
* OUTPUTS:
*       workSpacePtrPtr - pointer to pointer on workspace.
*
* RETURNS:
*       GT_OK           - on success
*       GT_BAD_PARAM    - wrong devNum, portNum
*       GT_BAD_PTR      - NULL ptr
*       GT_FAIL         - on error
*
* COMMENTS:
*       I forced to place this function here, because it needs number of port
*       group where CPU port is connected and there is just no more suitable
*       place.
*
*******************************************************************************/
GT_STATUS prvCpssDxChPortDynamicPizzaArbiterWSGet
(
    IN  GT_U8                            devNum,
    OUT PRV_CPSS_DXCH_PA_WORKSPACE_STC **workSpacePtrPtr
)
{
    PRV_CPSS_DXCH_PA_WORKSPACE_STC *paWsPtr;


    PRV_CPSS_DXCH_DEV_CHECK_MAC(devNum);
    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum, CPSS_CH1_E | CPSS_CH1_DIAMOND_E
                                            | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E | CPSS_XCAT3_E
                                            | CPSS_LION_E | CPSS_LION2_E | CPSS_XCAT2_E);


    CPSS_NULL_PTR_CHECK_MAC(workSpacePtrPtr);

    paWsPtr = PRV_CPSS_PP_MAC(devNum)->paWsPtr;

    *workSpacePtrPtr = paWsPtr;
    return GT_OK;
}

/*******************************************************************************
* prvCpssDxChPortDynamicPizzaArbiterWSSuportedUnitListGet
*
* DESCRIPTION:
*       Get list of supported units
*
* APPLICABLE DEVICES:
*        Bobcat2; Caelum; Bobcat3.
*
* NOT APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; Lion; Lion2; xCat2.
*
* INPUTS:
*       devNum   - device number
*
* OUTPUTS:
*       supportedUnitListPtr - pointer to pointer to list of supported units
*
* RETURNS:
*       GT_OK           - on success
*       GT_BAD_PARAM    - wrong devNum, portNum
*       GT_BAD_PTR      - NULL ptr
*       GT_FAIL         - on error
*
* COMMENTS:
*       I forced to place this function here, because it needs number of port
*       group where CPU port is connected and there is just no more suitable
*       place.
*
*******************************************************************************/
GT_STATUS prvCpssDxChPortDynamicPizzaArbiterWSSuportedUnitListGet
(
    IN  GT_U8                    devNum,
    OUT CPSS_DXCH_PA_UNIT_ENT **supportedUnitListPtrPtr
)
{
    PRV_CPSS_DXCH_PA_WORKSPACE_STC *paWsPtr;


    PRV_CPSS_DXCH_DEV_CHECK_MAC(devNum);
    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum, CPSS_CH1_E | CPSS_CH1_DIAMOND_E
                                            | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E | CPSS_XCAT3_E
                                            | CPSS_LION_E | CPSS_LION2_E | CPSS_XCAT2_E);

    CPSS_NULL_PTR_CHECK_MAC(supportedUnitListPtrPtr);

    paWsPtr = PRV_CPSS_PP_MAC(devNum)->paWsPtr;

    *supportedUnitListPtrPtr = paWsPtr->prv_DeviceUnitListPtr;
    return GT_OK;
}



/*******************************************************************************
* prvCpssDxChPortDynamicPizzaArbiterWSInit
*
* DESCRIPTION:
*       Init Pizza Arbiter WSs (objects)
*
* APPLICABLE DEVICES:
*       Bobcat2; Caelum; Bobcat3.
*
* NOT APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; Lion; Lion2; xCat2.
*
* INPUTS:
*       devNum   - device number
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK           - on success
*       GT_BAD_PARAM    - wrong devNum, portNum
*       GT_FAIL         - on error
*
* COMMENTS:
*       I forced to place this function here, because it needs number of port
*       group where CPU port is connected and there is just no more suitable
*       place.
*
*******************************************************************************/
GT_STATUS prvCpssDxChPortDynamicPizzaArbiterWSInit
(
    IN  GT_U8                   devNum
)
{
    PRV_CPSS_DXCH_PP_CONFIG_STC *pDev;

    PRV_CPSS_DXCH_DEV_CHECK_MAC(devNum);
    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum, CPSS_CH1_E | CPSS_CH1_DIAMOND_E
                                            | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E | CPSS_XCAT3_E
                                            | CPSS_LION_E | CPSS_LION2_E | CPSS_XCAT2_E);

    if (GT_FALSE == prvCpssDxChPortPaWorSpaceInitStatusGet(&prv_paWsSet))
    {
        prvCpssDxChPortDynamicPizzaArbiterWSByParamsInit(&prv_paWsSet.paWorkSpace_BC2_with_ilkn,              &prv_bc2_ws_init_params_with_ilkn);
        prvCpssDxChPortDynamicPizzaArbiterWSByParamsInit(&prv_paWsSet.paWorkSpace_BC2_wo_ilkn,                &prv_bc2_ws_init_params_no_ilkn);
        prvCpssDxChPortDynamicPizzaArbiterWSByParamsInit(&prv_paWsSet.paWorkSpace_BobK_Caelum_pipe0_pipe1,    &prv_bobk_caelum_pipe_0_pipe_1);
        prvCpssDxChPortDynamicPizzaArbiterWSByParamsInit(&prv_paWsSet.paWorkSpace_BobK_Cetus_pipe1,           &prv_bobk_cetus_pipe_1);

        /* BC3 : WA - call BC2 function */
        prvCpssDxChPortDynamicPizzaArbiterWSByParamsInit(&prv_paWsSet.paWorkSpace_BC3,&prv_bc3_ws_init_params);

        prvCpssDxChPortPaWorSpaceInitStatusSet(&prv_paWsSet);
    }


    pDev = PRV_CPSS_DXCH_PP_MAC(devNum);

    switch (pDev->genInfo.devFamily)
    {
        case CPSS_PP_FAMILY_DXCH_BOBCAT2_E:
            switch (pDev->genInfo.devSubFamily)
            {
                case CPSS_PP_SUB_FAMILY_NONE_E:
                    if (pDev->hwInfo.gop_ilkn.supported)
                    {
                        PRV_CPSS_PP_MAC(devNum)->paWsPtr = &prv_paWsSet.paWorkSpace_BC2_with_ilkn;
                    }
                    else
                    {
                        PRV_CPSS_PP_MAC(devNum)->paWsPtr = &prv_paWsSet.paWorkSpace_BC2_wo_ilkn;
                    }
                break;
                case CPSS_PP_SUB_FAMILY_BOBCAT2_BOBK_E:
#ifndef GM_USED
                        switch (pDev->genInfo.devType)
                        {
                            case CPSS_BOBK_CETUS_DEVICES_CASES_MAC:
                                PRV_CPSS_PP_MAC(devNum)->paWsPtr = &prv_paWsSet.paWorkSpace_BobK_Cetus_pipe1;
                            break;
                            case CPSS_BOBK_CAELUM_DEVICES_CASES_MAC:
                                PRV_CPSS_PP_MAC(devNum)->paWsPtr = &prv_paWsSet.paWorkSpace_BobK_Caelum_pipe0_pipe1;
                            break;
                            default:
                            {
                                return GT_NOT_SUPPORTED;
                            }
                        }
#else
                        PRV_CPSS_PP_MAC(devNum)->paWsPtr = &prv_paWsSet.paWorkSpace_BC2_wo_ilkn;
#endif
                break;
                default:
                {
                    return GT_NOT_SUPPORTED;
                }
            }
        break;
        case  CPSS_PP_FAMILY_DXCH_BOBCAT3_E:
             PRV_CPSS_PP_MAC(devNum)->paWsPtr = &prv_paWsSet.paWorkSpace_BC3;
        break;
        default:
        {
            return GT_NOT_SUPPORTED;
        }
    }

    return GT_OK;
}

