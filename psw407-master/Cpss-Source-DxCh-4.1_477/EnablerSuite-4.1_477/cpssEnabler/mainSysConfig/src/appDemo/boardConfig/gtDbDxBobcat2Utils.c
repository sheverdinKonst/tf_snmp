/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* gtDbDxBobcat2.c
*
* DESCRIPTION:
*       Initialization functions for the Bobcat2 - SIP5 - board.
*
* FILE REVISION NUMBER:
*       $Revision: 97 $
*
*******************************************************************************/
#include <appDemo/boardConfig/appDemoBoardConfig.h>
#include <appDemo/boardConfig/appDemoCfgMisc.h>
#include <appDemo/sysHwConfig/gtAppDemoSysConfigDefaults.h>
#include <appDemo/sysHwConfig/gtAppDemoTmConfig.h>
#include <appDemo/sysHwConfig/appDemoDb.h>
#include <appDemo/boardConfig/gtBoardsConfigFuncs.h>
#include <appDemo/boardConfig/gtDbDxBobcat2PhyConfig.h>
#include <appDemo/boardConfig/gtDbDxBobcat2Mappings.h>

#include <cpss/dxCh/dxChxGen/config/cpssDxChCfgInit.h>
#include <cpss/dxCh/dxChxGen/config/private/prvCpssDxChInfo.h>
#include <cpss/dxCh/dxChxGen/cpssHwInit/cpssDxChHwInit.h>
#include <cpss/dxCh/dxChxGen/cpssHwInit/cpssDxChHwInitLedCtrl.h>
#include <cpss/dxCh/dxChxGen/bridge/cpssDxChBrgGen.h>
#include <cpss/dxCh/dxChxGen/bridge/cpssDxChBrgFdb.h>
#include <cpss/dxCh/dxChxGen/bridge/cpssDxChBrgE2Phy.h>
#include <cpss/dxCh/dxChxGen/bridge/cpssDxChBrgSecurityBreach.h>
/*Phy*/
#include <cpss/dxCh/dxChxGen/phy/cpssDxChPhySmi.h>
#include <cpss/dxCh/dxChxGen/phy/cpssDxChPhySmiPreInit.h>
#include <cpss/dxCh/dxChxGen/phy/prvCpssDxChSmiUnitDrv.h>
/* port, Pizza Arbiter and slices management for specific units */
#include <cpss/dxCh/dxChxGen/port/cpssDxChPortCtrl.h>
#include <cpss/dxCh/dxChxGen/port/cpssDxChPortTx.h>
#include <cpss/dxCh/dxChxGen/port/cpssDxChPortMapping.h>
#include <cpss/dxCh/dxChxGen/port/PortMapping/prvCpssDxChPortMappingShadowDB.h>
#include <cpss/dxCh/dxChxGen/port/cpssDxChPortMapping.h>
#include <cpss/dxCh/dxChxGen/port/PortMapping/prvCpssDxChPortMappingShadowDB.h>
#include <cpss/dxCh/dxChxGen/port/private/prvCpssDxChPortIfModeCfgBcat2Resource.h>
#include <cpss/dxCh/dxChxGen/port/private/prvCpssDxChPortIfModeCfgBobKResource.h>
#include <cpss/dxCh/dxChxGen/port/PizzaArbiter/cpssDxChPortPizzaArbiter.h>
#include <cpss/dxCh/dxChxGen/port/cpssDxChPortInterlaken.h>

#include <cpssDriver/pp/hardware/cpssDriverPpHw.h>
#include <cpss/generic/init/cpssInit.h>
#include <cpss/generic/smi/cpssGenSmi.h>
#ifdef ASIC_SIMULATION
    #include <asicSimulation/SCIB/scib.h>
#endif



#if 0
/* include scib for test purpose */
#include <asicSimulation/SCIB/scib.h>
#include <gmSimulation/GM/GMApi.h>
#endif

#define PRV_CPSS_DXCH_BC2_UNUSED_PARAM_MAC(x) x = x

#define BAD_VALUE (GT_U32)(~0)

typedef struct 
{
    GT_U32   u32_Num;
    GT_CHAR *u32_Str;
}APPDEMO_GT_U32_2_STR_STC;


GT_CHAR * u32_2_STR
(
    IN GT_U32 u32_Num,
    IN APPDEMO_GT_U32_2_STR_STC * tbl
)
{
    GT_U32 i;
    static GT_CHAR badFmt[10];
    for (i = 0 ; tbl[i].u32_Num != BAD_VALUE; i++)
    {
        if (tbl[i].u32_Num == u32_Num)
        {
            return tbl[i].u32_Str;
        }
    }
    cpssOsSprintf(badFmt,"-%d",u32_Num);
    return &badFmt[0];
}


GT_CHAR * RXDMA_IfWidth_2_STR
(
    IN GT_U32 RXDMA_IfWidth
)
{
    static APPDEMO_GT_U32_2_STR_STC prv_mappingTypeStr[] =
    {
          { 0,           "64b" }
         ,{ 2,           "256b" }
         ,{ 3,           "512b" }
         ,{ BAD_VALUE,   (GT_CHAR *)NULL }
    };
    return u32_2_STR(RXDMA_IfWidth,&prv_mappingTypeStr[0]);
}


GT_CHAR * TX_FIFO_IfWidth_2_STR
(
    IN GT_U32 TX_FIFO_IfWidth
)
{
    static APPDEMO_GT_U32_2_STR_STC prv_mappingTypeStr[] =
    {
          { 0,           "1B" }
         ,{ 3,           "8B" }
         ,{ 5,           "32B" }
         ,{ 6,           "64B" }
         ,{ BAD_VALUE,   (GT_CHAR *)NULL }
    };
    return u32_2_STR(TX_FIFO_IfWidth,&prv_mappingTypeStr[0]);
}


GT_CHAR * CPSS_SPEED_2_STR
(
    CPSS_PORT_SPEED_ENT speed
)
{
    typedef struct 
    {
        CPSS_PORT_SPEED_ENT speedEnm;
        GT_CHAR            *speedStr;
    }APPDEMO_SPEED_2_STR_STC;

    static APPDEMO_SPEED_2_STR_STC prv_speed2str[] =
    {
         {  CPSS_PORT_SPEED_10_E,     "10M  "}
        ,{  CPSS_PORT_SPEED_100_E,    "100M "}
        ,{  CPSS_PORT_SPEED_1000_E,   "1G   "}
        ,{  CPSS_PORT_SPEED_10000_E,  "10G  "}
        ,{  CPSS_PORT_SPEED_12000_E,  "12G  "}
        ,{  CPSS_PORT_SPEED_2500_E,   "2.5G "}
        ,{  CPSS_PORT_SPEED_5000_E,   "5G   "}
        ,{  CPSS_PORT_SPEED_13600_E,  "13.6G"}
        ,{  CPSS_PORT_SPEED_20000_E,  "20G  "}
        ,{  CPSS_PORT_SPEED_40000_E,  "40G  "}
        ,{  CPSS_PORT_SPEED_16000_E,  "16G  "}
        ,{  CPSS_PORT_SPEED_15000_E,  "15G  "}
        ,{  CPSS_PORT_SPEED_75000_E,  "75G  "}
        ,{  CPSS_PORT_SPEED_100G_E,   "100G "}
        ,{  CPSS_PORT_SPEED_50000_E,  "50G  "}
        ,{  CPSS_PORT_SPEED_140G_E,   "140G "}
        ,{  CPSS_PORT_SPEED_11800_E,  "11.8G"}
        ,{  CPSS_PORT_SPEED_47200_E,  "47.2G"}
        ,{  CPSS_PORT_SPEED_NA_E,     "NA   "}
    };
    GT_U32 i;
    for (i = 0 ; prv_speed2str[i].speedEnm != CPSS_PORT_SPEED_NA_E; i++)
    {
        if (prv_speed2str[i].speedEnm == speed)
        {
            return prv_speed2str[i].speedStr;
        }
    }
    return "-----";
}

GT_CHAR * CPSS_IF_2_STR
(
    CPSS_PORT_INTERFACE_MODE_ENT ifEnm
)
{
    typedef struct 
    {
        CPSS_PORT_INTERFACE_MODE_ENT ifEnm;
        GT_CHAR                     *ifStr;
    }APPDEMO_IF_2_STR_STC;

    APPDEMO_IF_2_STR_STC prv_prvif2str[] =
    {
        {  CPSS_PORT_INTERFACE_MODE_REDUCED_10BIT_E, "REDUCED_10BIT"  }
       ,{  CPSS_PORT_INTERFACE_MODE_REDUCED_GMII_E,  "REDUCED_GMII"   }
       ,{  CPSS_PORT_INTERFACE_MODE_MII_E,           "MII         "   }
       ,{  CPSS_PORT_INTERFACE_MODE_SGMII_E,         "SGMII       "   }
       ,{  CPSS_PORT_INTERFACE_MODE_XGMII_E,         "XGMII       "   }
       ,{  CPSS_PORT_INTERFACE_MODE_MGMII_E,         "MGMII       "   }
       ,{  CPSS_PORT_INTERFACE_MODE_1000BASE_X_E,    "1000BASE_X  "   }
       ,{  CPSS_PORT_INTERFACE_MODE_GMII_E,          "GMII        "   }
       ,{  CPSS_PORT_INTERFACE_MODE_MII_PHY_E,       "MII_PHY     "   }
       ,{  CPSS_PORT_INTERFACE_MODE_QX_E,            "QX          "   }
       ,{  CPSS_PORT_INTERFACE_MODE_HX_E,            "HX          "   }
       ,{  CPSS_PORT_INTERFACE_MODE_RXAUI_E,         "RXAUI       "   }
       ,{  CPSS_PORT_INTERFACE_MODE_100BASE_FX_E,    "100BASE_FX  "   }
       ,{  CPSS_PORT_INTERFACE_MODE_QSGMII_E,        "QSGMII      "   }
       ,{  CPSS_PORT_INTERFACE_MODE_XLG_E,           "XLG         "   }
       ,{  CPSS_PORT_INTERFACE_MODE_LOCAL_XGMII_E,   "LOCAL_XGMII "   }
       ,{  CPSS_PORT_INTERFACE_MODE_KR_E,            "KR          "   }
       ,{  CPSS_PORT_INTERFACE_MODE_HGL_E,           "HGL         "   }
       ,{  CPSS_PORT_INTERFACE_MODE_CHGL_12_E,       "CHGL_12     "   }
       ,{  CPSS_PORT_INTERFACE_MODE_ILKN12_E,        "ILKN12      "   }
       ,{  CPSS_PORT_INTERFACE_MODE_SR_LR_E,         "SR_LR       "   }
       ,{  CPSS_PORT_INTERFACE_MODE_ILKN16_E,        "ILKN16      "   }
       ,{  CPSS_PORT_INTERFACE_MODE_ILKN24_E,        "ILKN24      "   }
       ,{  CPSS_PORT_INTERFACE_MODE_ILKN4_E,         "ILKN4       "   }
       ,{  CPSS_PORT_INTERFACE_MODE_ILKN8_E,         "ILKN8       "   }
       ,{  CPSS_PORT_INTERFACE_MODE_NA_E,            "NA          "   }
    };

    GT_U32 i;
    for (i = 0 ; prv_prvif2str[i].ifEnm != CPSS_PORT_INTERFACE_MODE_NA_E; i++)
    {
        if (prv_prvif2str[i].ifEnm == ifEnm)
        {
            return prv_prvif2str[i].ifStr;
        }
    }
    return "------------";
}


GT_CHAR * CPSS_MAPPING_2_STR
(
    CPSS_DXCH_PORT_MAPPING_TYPE_ENT mapEnm
)
{
    typedef struct 
    {
        CPSS_DXCH_PORT_MAPPING_TYPE_ENT mapEnm;
        GT_CHAR                        *mapStr;
    }APPDEMO_MAPPING_2_STR_STC;


    static APPDEMO_MAPPING_2_STR_STC prv_mappingTypeStr[] =
    {
          { CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,           "ETHERNET" }
         ,{ CPSS_DXCH_PORT_MAPPING_TYPE_CPU_SDMA_E,               "CPU-SDMA" }
         ,{ CPSS_DXCH_PORT_MAPPING_TYPE_ILKN_CHANNEL_E,           "ILKN-CHL" }
         ,{ CPSS_DXCH_PORT_MAPPING_TYPE_REMOTE_PHYSICAL_PORT_E,   "REMOTE-P" }
         ,{ CPSS_DXCH_PORT_MAPPING_TYPE_INVALID_E,                "--------" }
    };
    GT_U32 i;
    for (i = 0 ; prv_mappingTypeStr[i].mapEnm != CPSS_DXCH_PORT_MAPPING_TYPE_INVALID_E; i++)
    {
        if (prv_mappingTypeStr[i].mapEnm == mapEnm)
        {
            return prv_mappingTypeStr[i].mapStr;
        }
    }
    return "--------";
}



GT_STATUS prvBobcat2PortMapShadowPrint
(
    IN  GT_U8   devNum,
    IN  GT_U32  firstPhysicalPortNumber,
    IN  GT_U32  portMapShadowArraySize
)
{
    GT_U32 i; /* loop iterator */

    if((firstPhysicalPortNumber > 255) || ((firstPhysicalPortNumber + portMapShadowArraySize) > 255))
    {
        return GT_BAD_PARAM;
    }

    for(i = firstPhysicalPortNumber; i <= (firstPhysicalPortNumber + portMapShadowArraySize); i++)
    {
        cpssOsPrintf("PhysPort: %d   Valid = %d\r\n", i, PRV_CPSS_DXCH_PP_MAC(devNum)->port.portsMapInfoShadowArr[i].valid);
        cpssOsPrintf("MAC:%d  TXQ:%d  TX_DMA:%d  RX_DMA:%d\r\n", PRV_CPSS_DXCH_PP_MAC(devNum)->port.portsMapInfoShadowArr[i].portMap.macNum,
                                                         PRV_CPSS_DXCH_PP_MAC(devNum)->port.portsMapInfoShadowArr[i].portMap.txqNum,
                                                         PRV_CPSS_DXCH_PP_MAC(devNum)->port.portsMapInfoShadowArr[i].portMap.txDmaNum,
                                                         PRV_CPSS_DXCH_PP_MAC(devNum)->port.portsMapInfoShadowArr[i].portMap.rxDmaNum);
        cpssOsPrintf("portGroupId:%d  MAP_TYPE:%d\r\n\r\n", PRV_CPSS_DXCH_PP_MAC(devNum)->port.portsMapInfoShadowArr[i].portMap.portGroup,
                                                         PRV_CPSS_DXCH_PP_MAC(devNum)->port.portsMapInfoShadowArr[i].portMap.mappingType);
    }


    return GT_OK;
}

/*-------------------------------------------------------------------------------
**     Pizza Arbiter State Print Function :
**         prints configuration and state of all slice for each unit
**         <Rx/Tx DMA DMA_CTU  BM TxQ
**-------------------------------------------------------------------------------*/


#ifdef WIN32
    #define cpssOsPrintf printf
#endif


static void gtBobcat2PortPizzaArbiterIfDumpSliceStatePrint(GT_BOOL isEnable, GT_U32 occupiedBy)
{
    if (GT_FALSE  != isEnable )
    {
        cpssOsPrintf(" %2d",occupiedBy);
    }
    else
    {
        cpssOsPrintf(" %2s","X");
    }
}

static void gtBobcat2PortPizzaArbiterIfDumpUnitStatePrint
(
    CPSS_DXCH_BOBCAT2_SLICES_PIZZA_ARBITER_STATE_STC * pUnitPtr,
    char * msg
)
{
    GT_U32      sliceId;
    GT_U32      slicesNumberPerGop = 20;


    cpssOsPrintf("\n    %s",msg);
    cpssOsPrintf("\n       Total/Configured Slices      = %d/%d" , pUnitPtr->totalSlicesOnUnit,
                                                                   pUnitPtr->totalConfiguredSlices);
    cpssOsPrintf("\n       Work Conserving Bit = %d",pUnitPtr->workConservingBit);
    if (pUnitPtr->totalSlicesOnUnit > 0)
    {
        cpssOsPrintf("\n       Slice to port map   : slice :");
        for (sliceId = 0 ; sliceId < slicesNumberPerGop ; sliceId++)
        {
            cpssOsPrintf(" %2d",sliceId);
        }
        cpssOsPrintf("\n                                   :");
        for (sliceId = 0 ; sliceId < slicesNumberPerGop ; sliceId++)
        {
            cpssOsPrintf("---");
        }


        for (sliceId = 0 ; sliceId < pUnitPtr->totalSlicesOnUnit ; sliceId++)
        {
            if (0 == sliceId % slicesNumberPerGop)
            {
                cpssOsPrintf("\n       Slice %4d : %4d   : port  :",sliceId, sliceId + slicesNumberPerGop -1 );
            }
            gtBobcat2PortPizzaArbiterIfDumpSliceStatePrint(pUnitPtr->slice_enable[sliceId],
                                                                      pUnitPtr->slice_occupied_by[sliceId]);
        }
    }
}



typedef struct PA_UNIT_2_STR_STCT
{
    CPSS_DXCH_PA_UNIT_ENT  paUnit;
    GT_CHAR               *str;
}PA_UNIT_2_STR_STC;

PA_UNIT_2_STR_STC pa_unit2str_List[] = 
{
     { CPSS_DXCH_PA_UNIT_RXDMA_E,              "PA UNIT RXDMA"         }
    ,{ CPSS_DXCH_PA_UNIT_TXDMA_E,              "PA UNIT TXDMA"         }
    ,{ CPSS_DXCH_PA_UNIT_TXQ_E,                "PA UNIT TXQ"           }
    ,{ CPSS_DXCH_PA_UNIT_TX_FIFO_E,            "PA UNIT TX_FIFO"       }
    ,{ CPSS_DXCH_PA_UNIT_ETH_TX_FIFO_E,        "PA UNIT ETH_TX_FIFO"   } 
    ,{ CPSS_DXCH_PA_UNIT_ILKN_TX_FIFO_E,       "PA UNIT ILKN_TX_FIFO"  } 
    ,{ CPSS_DXCH_PA_UNIT_RXDMA_1_E,            "PA UNIT RXDMA_1"       }
    ,{ CPSS_DXCH_PA_UNIT_TXDMA_1_E,            "PA UNIT TXDMA_1"       }
    ,{ CPSS_DXCH_PA_UNIT_TX_FIFO_1_E,          "PA UNIT TX_FIFO_1"     }
    ,{ CPSS_DXCH_PA_UNIT_ETH_TX_FIFO_1_E,      "PA UNIT ETH_TX_FIFO_1" } 
    ,{ CPSS_DXCH_PA_UNIT_RXDMA_GLUE_E,         "PA UNIT RXDMA_GLUE"    }
    ,{ CPSS_DXCH_PA_UNIT_TXDMA_GLUE_E,         "PA UNIT TXDMA_GLUE"    }
    ,{ CPSS_DXCH_PA_UNIT_MPPM_E,               "PA UNIT MPPM"          }
    ,{ CPSS_DXCH_PA_UNIT_UNDEFINED_E,          (GT_CHAR *)NULL         }
};

static GT_CHAR * paUnit22StrFind
(
    CPSS_DXCH_PA_UNIT_ENT  paUnit
)
{
    GT_U32 i;
    static GT_CHAR badUnitNameMsg[30];
    for (i = 0 ; pa_unit2str_List[i].paUnit != CPSS_DXCH_PA_UNIT_UNDEFINED_E; i++)
    {
        if (pa_unit2str_List[i].paUnit  == paUnit)
        {
            return pa_unit2str_List[i].str;
        }
    }
    sprintf(badUnitNameMsg,"Unit %d : Unknown Name",paUnit);
    return &badUnitNameMsg[0];
}

/*******************************************************************************
* gtBobcat2PortPizzaArbiterIfStateDump
*
* DESCRIPTION:
*       Dump Bobcat2 Pizza Arbiter Registers
*
* APPLICABLE DEVICES:
*        Bobcat2; Caelum; Bobcat3.
*
* NOT APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; Lion; Lion2; xCat2.
*
*
* INPUTS:
*       devNum   - physical device number
*       portGroupId  - number of port group
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK                    - on success
*       GT_BAD_PARAM             - on wrong port number or device
*       GT_HW_ERROR              - on hardware error
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS gtBobcat2PortPizzaArbiterIfStateDump
(
    IN  GT_U8  devNum,
    IN  GT_U32 portGroupId
)
{
    GT_STATUS   rc;         /* return code */
    static CPSS_DXCH_DEV_PIZZA_ARBITER_STATE_STC  pizzaDeviceState;
    GT_U32 unitIdx;
    GT_CHAR * unitNamePtr;
    CPSS_DXCH_PA_UNIT_ENT unitType;


    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum, CPSS_CH1_E | CPSS_CH1_DIAMOND_E
                                            | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E | CPSS_XCAT3_E
                                            | CPSS_LION_E | CPSS_XCAT2_E);

    rc = cpssDxChPortPizzaArbiterDevStateGet(devNum,portGroupId, /*OUT*/&pizzaDeviceState);
    if (rc != GT_OK)
    {
        return rc;
    }

    cpssOsPrintf("\nPizza Arbiter State device = %2d port group = %2d", devNum,portGroupId);
    for (unitIdx = 0 ; pizzaDeviceState.devState.bobK.unitList[unitIdx] != CPSS_DXCH_PA_UNIT_UNDEFINED_E; unitIdx++)
    {
        unitType = pizzaDeviceState.devState.bobK.unitList[unitIdx];
        unitNamePtr = paUnit22StrFind(unitType);
        gtBobcat2PortPizzaArbiterIfDumpUnitStatePrint(&pizzaDeviceState.devState.bobK.unitState[unitIdx], unitNamePtr);
    }
    cpssOsPrintf("\n");

    /*
    cpssOsPrintf("\nPizza Arbiter State device = %2d port group = %2d", devNum,portGroupId);
    gtBobcat2PortPizzaArbiterIfDumpUnitStatePrint(&pizzaDeviceState.devState.bc2.rxDMA,     "RxDMA  Pizza Arbiter:");
    gtBobcat2PortPizzaArbiterIfDumpUnitStatePrint(&pizzaDeviceState.devState.bc2.txDMA,     "TxDMA  Pizza Arbiter:");
    gtBobcat2PortPizzaArbiterIfDumpUnitStatePrint(&pizzaDeviceState.devState.bc2.TxQ,       "TxQ    Pizza Arbiter:");
    gtBobcat2PortPizzaArbiterIfDumpUnitStatePrint(&pizzaDeviceState.devState.bc2.txFIFO,    "TxFIFO Pizza Arbiter:");
    gtBobcat2PortPizzaArbiterIfDumpUnitStatePrint(&pizzaDeviceState.devState.bc2.ethFxFIFO, "ethTxFIFO Pizza Arbiter:");
    gtBobcat2PortPizzaArbiterIfDumpUnitStatePrint(&pizzaDeviceState.devState.bc2.ilknTxFIFO,"IlknTxFIFO Pizza Arbiter");
    cpssOsPrintf("\n");
    */
    return GT_OK;
}

/*******************************************************************************
* gtBobcat2PortPizzaArbiterIfUnitStateDump
*
* DESCRIPTION:
*       Dump Bobcat2 Pizza Arbiter Specific Unit Registers
*
* APPLICABLE DEVICES:
*        Bobcat2; Caelum; Bobcat3.
*
* NOT APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; Lion; Lion2; xCat2.
*
*
* INPUTS:
*       devNum       - physical device number
*       portGroupId  - number of port group
*       unit         - unit (RxDMA/TxDMA/TxQ/Tx-FIFO/Eth-Tx-FIFO/Ilkn-Tx-FIFO)
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK                    - on success
*       GT_BAD_PARAM             - on wrong port number or device
*       GT_HW_ERROR              - on hardware error
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS gtBobcat2PortPizzaArbiterIfUnitStateDump
(
    IN  GT_U8  devNum,
    IN  GT_U32 portGroupId,
    IN  CPSS_DXCH_PA_UNIT_ENT unit
)
{
    GT_STATUS   rc;         /* return code */
    GT_U32      unitIdx;
    GT_BOOL     unitFound;
    static CPSS_DXCH_DEV_PIZZA_ARBITER_STATE_STC  pizzaDeviceState;
    GT_CHAR *   unitNamePtr;

    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum, CPSS_CH1_E | CPSS_CH1_DIAMOND_E
                                            | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E | CPSS_XCAT3_E
                                            | CPSS_LION_E | CPSS_XCAT2_E);

    rc = cpssDxChPortPizzaArbiterDevStateGet(devNum,portGroupId, /*OUT*/&pizzaDeviceState);
    if (rc != GT_OK)
    {
        return rc;
    }

    unitFound = GT_FALSE;
    cpssOsPrintf("\nPizza Arbiter State device = %2d port group = %2d", devNum,portGroupId);
    for (unitIdx = 0 ; pizzaDeviceState.devState.bobK.unitList[unitIdx] != CPSS_DXCH_PA_UNIT_UNDEFINED_E; unitIdx++)
    {
        if (pizzaDeviceState.devState.bobK.unitList[unitIdx] == unit)
        {
            unitFound = GT_TRUE;
            break;
        }
    }
    if (unitFound == GT_TRUE)
    {
        unitNamePtr = paUnit22StrFind(unit);
        gtBobcat2PortPizzaArbiterIfDumpUnitStatePrint(&pizzaDeviceState.devState.bobK.unitState[unitIdx], unitNamePtr);
    }
    else
    {
        cpssOsPrintf("\nunit %d : Undefined Unit",unit);
        return GT_BAD_PARAM;
    }
    cpssOsPrintf("\n");
    return GT_OK;
}


/*******************************************************************************
* gtBobcat2PortMappingDump
*
* DESCRIPTION:
*       Dump Bobcat2 Port Mapping (valid ports only)
*
* APPLICABLE DEVICES:
*        Bobcat2; Caelum; Bobcat3.
*
* NOT APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; Lion; Lion2; xCat2.
*
*
* INPUTS:
*       dev   - physical device number
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK                    - on success
*       GT_BAD_PARAM             - on wrong port number or device
*       GT_HW_ERROR              - on hardware error
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS gtBobcat2PortMappingDump
(
    IN  GT_U8  dev
)
{
    GT_STATUS   rc;         /* return code */
    GT_PHYSICAL_PORT_NUM portIdx;

    cpssOsPrintf("\n+------+-----------------+------------------------------+");
    cpssOsPrintf("\n| Port |   mapping Type  | rxdma txdma mac txq ilkn  tm |");
    cpssOsPrintf("\n+------+-----------------+------------------------------+");

    for (portIdx = 0 ; portIdx < 256; portIdx++)
    {
        PRV_CPSS_DXCH_PORT_MAP_SHADOW_STC *portMapPtr;
        rc = prvCpssDxChPortPhysicalPortMapShadowDBGet(dev,portIdx,/*OUT*/&portMapPtr);
        if (rc != GT_OK)
        {
            return rc;
        }
        if (portMapPtr->valid == GT_TRUE)
        {
            cpssOsPrintf("\n| %4d |",portIdx);
            cpssOsPrintf(" %-15s | %5d %5d %3d %3d %4d %3d |"
                                                ,CPSS_MAPPING_2_STR(portMapPtr->portMap.mappingType)
                                                ,portMapPtr->portMap.rxDmaNum
                                                ,portMapPtr->portMap.txDmaNum
                                                ,portMapPtr->portMap.macNum
                                                ,portMapPtr->portMap.txqNum
                                                ,portMapPtr->portMap.ilknChannel
                                                ,portMapPtr->portMap.tmPortIdx);
        }
    }
    cpssOsPrintf("\n+------+-----------------+------------------------------+");
    return GT_OK;
}


GT_STATUS gtBobcat2SMIMappingDump
(
    IN  GT_U8  devNum,
    IN  GT_U8  portGroupId
)
{
    GT_STATUS   rc;         /* return code */
    GT_U32      smiInstance;
    GT_U32      smiLocalPort;
    static      PRV_CPSS_DXCH_SMI_STATE_STC state;

    rc = prvCpssDxChSMIStateGet(devNum,portGroupId,/*OUT*/&state);
    if (rc != GT_OK)
    {
        return rc;
    }

    cpssOsPrintf("\n+-----+--------------------+");
    cpssOsPrintf("\n| SMI |  Autopolling Number|");
    cpssOsPrintf("\n+-----+--------------------+");
    for (smiInstance = CPSS_PHY_SMI_INTERFACE_0_E; smiInstance < CPSS_PHY_SMI_INTERFACE_MAX_E; smiInstance++)
    {
        cpssOsPrintf("\n| %3d | %4d",smiInstance,state.autoPollNumOfPortsArr[smiInstance]);
    }
    cpssOsPrintf("\n+------+-------+-------+-------+-------+");
    cpssOsPrintf("\n| Port | SMI-0 | SMI-1 | SMI-2 | SMI-3 |");
    cpssOsPrintf("\n+------+-------+-------+-------+-------+");
    for (smiLocalPort = 0; smiLocalPort < NUMBER_OF_PORTS_PER_SMI_UNIT_CNS; smiLocalPort++)
    {
        cpssOsPrintf("\n| %4d |",smiLocalPort);
        for (smiInstance = CPSS_PHY_SMI_INTERFACE_0_E; smiInstance < CPSS_PHY_SMI_INTERFACE_MAX_E; smiInstance++)
        {
            cpssOsPrintf("  0x%02X |",state.phyAddrRegArr[smiInstance][smiLocalPort]);
        }
    }
    cpssOsPrintf("\n+------+-------+-------+-------+-------+");
    cpssOsPrintf("\n");
    return GT_OK;
}



GT_STATUS gtBobcat2PortResourceDump
(
    IN  GT_U8  devNum,
    IN  GT_PHYSICAL_PORT_NUM portNum
)
{
    GT_STATUS rc;
    CPSS_DXCH_BCAT2_PORT_RESOURCE_LIST_STC  portRes;
    GT_U32    i;
    CPSS_PORT_SPEED_ENT   speed;

    static GT_CHAR * resId2strConvArr[] = 
    {
         "RXDMA_IfWidth                           "             /* rev(A0, B0), speed */
        ,"TXDMA_SCDMA_TxQDescriptorCredit         "             /* rev(A0, B0), speed, core clock */
        ,"TXDMA_SCDMA_speed                       "             /* rev(A0    ), speed             */
        ,"TXDMA_SCDMA_rateLimitThreshold          "             /* rev(A0    ), speed, core clock */
        ,"TXDMA_SCDMA_burstAlmostFullThreshold    "             /* rev(    B0), speed, core clock */
        ,"TXDMA_SCDMA_burstFullThreshold          "             /* rev(    B0), speed, core clock */
        ,"TXDMA_SCDMA_TxFIFOHeaderCreditThreshold "             /* rev(A0, B0), speed, core clock */ 
        ,"TXDMA_SCDMA_TxFIFOPayloadCreditThreshold"             /* rev(A0, B0), speed, core clock */ 
        ,"TXFIFO_IfWidth                          "             /* speed */
        ,"TXFIFO_shifterThreshold                 "             /* rev(A0,   ), speed , core clock */
        ,"TXFIFO_payloadThreshold                 "             /* rev(A0, B0), speed , core clock */
        ,"Eth_TXFIFO_IfWidth                      "             /* speed */
        ,"Eth_TXFIFO_shifterThreshold             "             /* rev(A0,   ), speed , core clock */
        ,"Eth_TXFIFO_payloadThreshold             "             /* rev(A0, B0), speed , core clock */
    };
    static GT_CHAR resIdStr[40];

    rc = prvCpssDxChBcat2PortResoursesStateGet(devNum,portNum,&portRes);
    if (rc  != GT_OK)
    {
        return rc;
    }

    rc = cpssDxChPortSpeedGet(devNum,portNum,&speed);
    if (rc  != GT_OK)
    {
        return rc;
    }

    cpssOsPrintf("\nport %d",portNum);
    cpssOsPrintf("\n    speed = %s",CPSS_SPEED_2_STR(speed));
    cpssOsPrintf("\n    resources");
    cpssOsPrintf("\n    +----------------------------------------------------------");
    cpssOsPrintf("\n    |   %40s   | %10s |  value","name","idx in unit");
    cpssOsPrintf("\n    +----------------------------------------------------------");
    for (i = 0 ; i < sizeof(portRes.arr)/sizeof(portRes.arr[0]); i++)
    {
        if (portRes.arr[i].fldId != BC2_PORT_FLD_INVALID_E)
        {

            if (portRes.arr[i].fldId < sizeof(resId2strConvArr)/sizeof(resId2strConvArr[0]))
            {
                cpssOsStrCpy(&resIdStr[0],resId2strConvArr[portRes.arr[i].fldId]);
            }
            else
            {
                cpssOsSprintf(&resIdStr[0],"unknown resource id %d",portRes.arr[i].fldId);
            }

            cpssOsPrintf("\n        %40s   <   %3d  > :    %d",&resIdStr[0],portRes.arr[i].fldArrIdx,portRes.arr[i].fldVal);
        }
    }
    cpssOsPrintf("\n");
    return GT_OK;
}



typedef struct 
{
    CPSS_DXCH_BC2_PORT_RESOURCES_FLD_ENT resEnm;
    GT_U32                               fldOffs;
}PRV_CPSS_DXCH_BC2_RESEM_OFFS_STC;

static PRV_CPSS_DXCH_BC2_RESEM_OFFS_STC  prv_bc2res2OffsInitList[] = 
{
     { BC2_PORT_FLD_RXDMA_IfWidth,                             offsetof(PRV_CPSS_DXCH_BC2_PORT_RESOURCE_STC , rxdmaScdmaIncomingBusWidth)              } 
    ,{ BC2_PORT_FLD_TXDMA_SCDMA_TxQDescriptorCredit,           offsetof(PRV_CPSS_DXCH_BC2_PORT_RESOURCE_STC , txdmaTxQCreditValue)                     } 
    ,{ BC2_PORT_FLD_TXDMA_SCDMA_burstAlmostFullThreshold,      offsetof(PRV_CPSS_DXCH_BC2_PORT_RESOURCE_STC , txdmaBurstAlmostFullThreshold)           }
    ,{ BC2_PORT_FLD_TXDMA_SCDMA_burstFullThreshold,            offsetof(PRV_CPSS_DXCH_BC2_PORT_RESOURCE_STC , txdmaBBurstFullThreshold)                }
    ,{ BC2_PORT_FLD_TXDMA_SCDMA_TxFIFOHeaderCreditThreshold,   offsetof(PRV_CPSS_DXCH_BC2_PORT_RESOURCE_STC , txdmaTxfifoHeaderCounterThresholdScdma)  }
    ,{ BC2_PORT_FLD_TXDMA_SCDMA_TxFIFOPayloadCreditThreshold,  offsetof(PRV_CPSS_DXCH_BC2_PORT_RESOURCE_STC , txdmaTxfifoPayloadCounterThresholdScdma) }
    ,{ BC2_PORT_FLD_TXFIFO_IfWidth,                            offsetof(PRV_CPSS_DXCH_BC2_PORT_RESOURCE_STC , txfifoScdmaShiftersOutgoingBusWidth)     }
    ,{ BC2_PORT_FLD_TXFIFO_payloadThreshold,                   offsetof(PRV_CPSS_DXCH_BC2_PORT_RESOURCE_STC , txfifoScdmaPayloadThreshold)             }
    ,{ BC2_PORT_FLD_Eth_TXFIFO_IfWidth,                        offsetof(PRV_CPSS_DXCH_BC2_PORT_RESOURCE_STC , ethTxfifoOutgoingBusWidth)               }
    ,{ BC2_PORT_FLD_Eth_TXFIFO_payloadThreshold,               offsetof(PRV_CPSS_DXCH_BC2_PORT_RESOURCE_STC , ethTxfifoScdmaPayloadThreshold)          }
    ,{ BC2_PORT_FLD_INVALID_E,                                 (GT_U32) (~0)                                                                           }
};

static GT_U32 * prv_bc2res2OffsArr[BC2_PORT_FLD_MAX];

GT_STATUS appDemoBobcat2PortListResourcesPrint
(
    IN GT_U8 dev
)
{
    GT_STATUS rc;
    GT_U32 i;
    GT_U32 portIdx;
    GT_PHYSICAL_PORT_NUM portNum;
    PRV_CPSS_DXCH_PORT_MAP_SHADOW_STC *portMapPtr;
    CPSS_PORT_SPEED_ENT speed;
    CPSS_PORT_INTERFACE_MODE_ENT   ifMode;
    PRV_CPSS_DXCH_BC2_PORT_RESOURCE_STC     resource;
    CPSS_DXCH_BCAT2_PORT_RESOURCE_LIST_STC  resList;
    GT_U32 maxPortNum;


    for (i = 0 ; i < sizeof(prv_bc2res2OffsArr)/sizeof(prv_bc2res2OffsArr[0]); i++)
    {
        prv_bc2res2OffsArr[i] = (GT_U32*)NULL;
    }
    for (i = 0 ; prv_bc2res2OffsInitList[i].resEnm != BC2_PORT_FLD_INVALID_E; i++)
    {
        prv_bc2res2OffsArr[prv_bc2res2OffsInitList[i].resEnm] = (GT_U32*)((GT_U8*)&resource + prv_bc2res2OffsInitList[i].fldOffs);
    }

    rc = prvCpssDxChPortPhysicalPortMapShadowDBPossiblePortNumGet(dev,/*OUT*/&maxPortNum);
    if (rc != GT_OK)
    {
        return rc;
    }


    cpssOsPrintf("\n+------------------------------------------------------------------------------------------------------------------------------+");
    cpssOsPrintf("\n|                             Port resources                                                                                   |");
    cpssOsPrintf("\n+----+------+----------+-------+--------------+--------------------+-----+-----------------------------+-----------+-----------+");
    cpssOsPrintf("\n|    |      |          |       |              |                    |RXDMA|      TXDMA SCDMA            | TX-FIFO   |Eth-TX-FIFO|");
    cpssOsPrintf("\n|    |      |          |       |              |                    |-----|-----+-----+-----+-----------|-----+-----|-----+-----|");
    cpssOsPrintf("\n| #  | Port | map type | Speed |    IF        | rxdma txq txdma tm |  IF | TxQ |Burst|Burst| TX-FIFO   | Out | Pay | Out | Pay |");
    cpssOsPrintf("\n|    |      |          |       |              |                    |Width|Descr| All | Full|-----+-----|Going| Load|Going| Load|");
    cpssOsPrintf("\n|    |      |          |       |              |                    |     |     | Most|Thrsh| Hdr | Pay | Bus |Thrsh| Bus |Thrsh|");
    cpssOsPrintf("\n|    |      |          |       |              |                    |     |     | Full|     |Thrsh|Load |Width|     |Width|     |");  
    cpssOsPrintf("\n|    |      |          |       |              |                    |     |     |Thrsh|     |     |Thrsh|     |     |     |     |");
    cpssOsPrintf("\n+----+------+----------+-------+--------------+--------------------+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+");

    portIdx = 0;                                                                                    

    for (portNum = 0 ; portNum < maxPortNum; portNum++)
    {
        rc = prvCpssDxChPortPhysicalPortMapShadowDBGet(dev,portNum,/*OUT*/&portMapPtr);
        if (rc != GT_OK)
        {
            return rc;
        }
        if (portMapPtr->valid == GT_FALSE)
        {
            continue;
        }
        if (portMapPtr->portMap.mappingType != CPSS_DXCH_PORT_MAPPING_TYPE_CPU_SDMA_E)
        {
            rc = cpssDxChPortSpeedGet(dev,portNum,/*OUT*/&speed);
            if (rc != GT_OK)
            {
                return rc;
            }
            rc = cpssDxChPortInterfaceModeGet(dev,portNum,/*OUT*/&ifMode);
            if (rc != GT_OK)
            {
                return rc;
            }
        }
        else /* CPU */
        {
            speed  = CPSS_PORT_SPEED_1000_E;
            ifMode = CPSS_PORT_INTERFACE_MODE_NA_E;
        }

        if (speed != CPSS_PORT_SPEED_NA_E)
        {
            rc = prvCpssDxChBcat2PortResoursesStateGet(dev,portNum,&resList);
            if (rc  != GT_OK)
            {
                return rc;
            }

            for (i = 0 ; i < sizeof(resList.arr)/sizeof(resList.arr[0]); i++)
            {
                if (resList.arr[i].fldId != BC2_PORT_FLD_INVALID_E)
                {
                    if (prv_bc2res2OffsArr[resList.arr[i].fldId] != NULL)
                    {
                        *prv_bc2res2OffsArr[resList.arr[i].fldId] = resList.arr[i].fldVal;
                    }
                }
            }

            cpssOsPrintf("\n| %2d | %4d | %-8s | %s | %s |",portIdx
                                                    ,portNum
                                                    ,CPSS_MAPPING_2_STR(portMapPtr->portMap.mappingType)
                                                    ,CPSS_SPEED_2_STR(speed)
                                                    ,CPSS_IF_2_STR(ifMode));
            cpssOsPrintf(" %5d %3d %5d %2d |"  ,portMapPtr->portMap.rxDmaNum
                                               ,portMapPtr->portMap.txqNum
                                               ,portMapPtr->portMap.txDmaNum
                                               ,portMapPtr->portMap.tmPortIdx);

            cpssOsPrintf("%4s |",RXDMA_IfWidth_2_STR(resource.rxdmaScdmaIncomingBusWidth));
            cpssOsPrintf("%4d |",resource.txdmaTxQCreditValue);
            cpssOsPrintf("%4d |",resource.txdmaBurstAlmostFullThreshold);
            cpssOsPrintf("%4d |",resource.txdmaBBurstFullThreshold);
            cpssOsPrintf("%4d |",resource.txdmaTxfifoHeaderCounterThresholdScdma);
            cpssOsPrintf("%4d |",resource.txdmaTxfifoPayloadCounterThresholdScdma);
            cpssOsPrintf("%4s |",TX_FIFO_IfWidth_2_STR(resource.txfifoScdmaShiftersOutgoingBusWidth));
            cpssOsPrintf("%4d |",resource.txfifoScdmaPayloadThreshold);
            if (portMapPtr->portMap.trafficManagerEn == GT_FALSE)
            {
                cpssOsPrintf("%4s |%4s |","--","--");
            }
            else
            {
                cpssOsPrintf("%4s |",TX_FIFO_IfWidth_2_STR(resource.ethTxfifoOutgoingBusWidth));
                cpssOsPrintf("%4d |",resource.ethTxfifoScdmaPayloadThreshold);
            }
            portIdx++;
        }
    }
    cpssOsPrintf("\n+----+------+----------+-------+--------------+--------------------+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+");
    cpssOsPrintf("\n");
    return GT_OK;
}


GT_STATUS appDemoPortsDisconnectConnectSimulation
(
    IN  GT_U8    devNum,
    IN  GT_U16   NumOfLoops,
    IN  GT_U16   timeout,
    IN  GT_U8    NumOfPorts,
    ...
)
{

    va_list  ap;                 /* arguments list pointer */
    GT_U32   i,j;
    GT_U32 portNum,LocalReturnCode,phyType;
    GT_U16 data;

    va_start(ap, NumOfPorts);

    data = 0;

    cpssOsPrintf("\nNumOfLoops= %d\n",NumOfLoops);
    cpssOsPrintf("\nNumOfPorts= %d\n",NumOfPorts);

    for(i = 1; i <= NumOfPorts;i++)
    {
        portNum = va_arg(ap, GT_U32);
        phyType = (portNum & 0xFF00)>> 8;
        portNum = portNum & 0xFF;

        cpssOsPrintf("\nphyType= %d\n",phyType);
        cpssOsPrintf("\nportNum= %d\n",portNum);

        if (portNum != CPSS_CPU_PORT_NUM_CNS)
        {
            if((portNum >= PRV_CPSS_PP_MAC(devNum)->numOfPorts) ||
                !PRV_CPSS_PHY_PORT_IS_EXIST_MAC(devNum,portNum))
            {
                va_end(ap);
                return GT_BAD_PARAM;

            }
        }

        for (j=1; j <=NumOfLoops; j++)
        {
            if (phyType== 0) /*No PHY*/
            {
                LocalReturnCode=cpssDxChPortSerdesTxEnableSet(devNum,portNum,0);
                if (LocalReturnCode!=GT_OK)
                {
                    cpssOsPrintf("\n Port %d Error Disable %d\n",portNum,LocalReturnCode);			   
                    va_end(ap);
                    return LocalReturnCode;
                }
            }
            else
            {
                LocalReturnCode=cpssDxChPhyPortSmiRegisterRead(devNum,portNum,16,&data); /*Read reg 16 - Copper Specific Control reg 1*/
                if (LocalReturnCode!=GT_OK)
                {
                    cpssOsPrintf("\n SmiRegisterRead on port %d Error %d\n",portNum,LocalReturnCode);			   
                    va_end(ap);
                    return LocalReturnCode;
                }

                data=data | 0x8 ; /*Set bit 3 Copper transmitter disable */
                LocalReturnCode=cpssDxChPhyPortSmiRegisterWrite(devNum,portNum,16,data);
                if (LocalReturnCode!=GT_OK)
                {
                    cpssOsPrintf("\n SmiRegisterWrite on port %d Error %d\n",portNum,LocalReturnCode);			   
                    va_end(ap);
                    return LocalReturnCode;
                }
            }


            if (timeout)
            {
                osTimerWkAfter(timeout);
            }

            if (phyType== 0) /*No PHY*/
            {
                LocalReturnCode=cpssDxChPortSerdesTxEnableSet(devNum,portNum,1);
                if (LocalReturnCode!=GT_OK)
                {
                    cpssOsPrintf("\n Port %d Error Enable %d\n",portNum,LocalReturnCode);
                    va_end(ap);
                    return LocalReturnCode;
                }
            }
            else
            {
                data=data & 0xFFFFFFF7 ; /*Clear bit 3 -Copper transmitter enable*/
                LocalReturnCode=cpssDxChPhyPortSmiRegisterWrite(devNum,portNum,16,data);
                if (LocalReturnCode!=GT_OK)
                {
                    cpssOsPrintf("\n SmiRegisterWrite on port %d Error %d\n",portNum,LocalReturnCode);			   
                    va_end(ap);
                    return LocalReturnCode;
                }
            }
            if (timeout)
            {
                osTimerWkAfter(timeout);
            }
        }
    }
    va_end(ap);
    return GT_OK;
}



