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
*       $Revision: 118 $
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

#include <cpssDriver/pp/hardware/cpssDriverPpHw.h>
#include <cpss/generic/init/cpssInit.h>
#include <cpss/generic/smi/cpssGenSmi.h>
#include <cpss/generic/cpssHwInit/cpssLedCtrl.h>
#include <cpss/generic/config/private/prvCpssConfigTypes.h>
#include <cpss/generic/private/utils/prvCpssTimeRtUtils.h>
#include <cpss/generic/cpssTypes.h>
#include <cpss/generic/systemRecovery/cpssGenSystemRecovery.h>

#include <cpss/dxCh/dxChxGen/cpssHwInit/cpssDxChHwInit.h>
#include <cpss/dxCh/dxChxGen/cpssHwInit/cpssDxChHwInitLedCtrl.h>
#include <cpss/dxCh/dxChxGen/bridge/cpssDxChBrgFdb.h>
#include <cpss/dxCh/dxChxGen/bridge/cpssDxChBrgE2Phy.h>
#include <cpss/dxCh/dxChxGen/bridge/cpssDxChBrgSecurityBreach.h>
#include <cpss/dxCh/dxChxGen/config/cpssDxChCfgInit.h>
#include <cpss/dxCh/dxChxGen/config/private/prvCpssDxChInfo.h>
#include <cpss/dxCh/dxChxGen/port/cpssDxChPortCtrl.h>
#include <cpss/dxCh/dxChxGen/port/cpssDxChPortTx.h>
#include <cpss/dxCh/dxChxGen/port/PortMapping/prvCpssDxChPortMappingShadowDB.h>
#include <cpss/dxCh/dxChxGen/port/PizzaArbiter/DynamicPizzaArbiter/cpssDxChPortDynamicPAUnitBW.h>
#include <cpss/dxCh/dxChxGen/port/PizzaArbiter/cpssDxChPortPizzaArbiter.h>
#include <cpss/dxCh/dxChxGen/port/cpssDxChPortMapping.h>
#include <cpss/dxCh/dxChxGen/phy/cpssDxChPhySmi.h>

/* TM glue */
#include <cpss/dxCh/dxChxGen/tmGlue/cpssDxChTmGlueDram.h>
#include <cpss/dxCh/dxChxGen/tmGlue/cpssDxChTmGlueQueueMap.h>
#include <cpss/dxCh/dxChxGen/tmGlue/cpssDxChTmGlueDrop.h>
#include <cpss/dxCh/dxChxGen/tmGlue/cpssDxChTmGlueFlowControl.h>
#include <cpss/dxCh/dxChxGen/tmGlue/cpssDxChTmGluePfc.h>
/* PTP */
#include <cpss/dxCh/dxChxGen/ptp/cpssDxChPtp.h>


#include <cpss/dxCh/dxChxGen/port/cpssDxChPortCtrl.h>
#include <cpss/dxCh/dxChxGen/port/PortMapping/prvCpssDxChPortMappingShadowDB.h>
#include <cpss/dxCh/dxChxGen/port/cpssDxChPortInterlaken.h>

#include <cpss/dxCh/dxChxGen/bridge/cpssDxChBrgGen.h>
/* remove when no need in direct write to tables */
#include <cpss/dxCh/dxChxGen/cpssHwInit/private/prvCpssDxChHwTables.h>
#include <cpss/dxCh/dxChxGen/diag/cpssDxChDiagPacketGenerator.h>
#include <cpss/dxCh/dxChxGen/nst/cpssDxChNstPortIsolation.h>

#include <gtOs/gtOsGen.h>
#include <gtOs/gtOsExc.h>

#include <gtExtDrv/drivers/gtIntDrv.h>
#include <gtExtDrv/drivers/gtUartDrv.h>
#include <serdes/mvHwsSerdesPrvIf.h>

#define PRV_CPSS_DXCH_BC2_UNUSED_PARAM_MAC(x) x = x

extern GT_STATUS appDemoDxChMaxMappedPortSet
(
    IN  GT_U8  dev,
    IN  GT_U32 mapArrLen,
    IN  CPSS_DXCH_PORT_MAP_STC *mapArrPtr
);

extern void appDemoRtosOnSimulationInit(void);

extern GT_STATUS prvCpssTmCtlGlobalParamsInit
(
    IN GT_U8                   devNum
);

/*******************************************************************************
 * Global variables
 ******************************************************************************/
static GT_U8    ppCounter = 0;     /* Number of Packet processors. */
static GT_BOOL  noBC2CpuPort = GT_FALSE;
static CPSS_DXCH_IMPLEMENT_WA_ENT BC2WaList[] =
{
    CPSS_DXCH_IMPLEMENT_WA_BOBCAT2_REV_A0_40G_NOT_THROUGH_TM_IS_PA_30G_E,
    CPSS_DXCH_IMPLEMENT_WA_LAST_E
};

#define PLL4_FREQUENCY_1250000_KHZ_CNS  1250000
#define PLL4_FREQUENCY_1093750_KHZ_CNS  1093750
#define PLL4_FREQUENCY_1550000_KHZ_CNS  1550000

static GT_U32   bc2BoardType  = APP_DEMO_BC2_BOARD_DB_CNS;
static GT_U32   bc2BoardRevId = 0;
static GT_BOOL  bc2BoardResetDone = GT_FALSE;
static GT_BOOL  isBobkBoard = GT_FALSE;

/*
   when system is initialized with non default ports mapping,
   TM Ports Init is called after custom ports mapping is applied
*/
static GT_BOOL  isCustomPortsMapping = GT_FALSE;

static CPSS_DXCH_TM_GLUE_DRAM_CFG_STC bc2DramCfgArr[] =
{
    { /* DB board */
        5,  /* activeInterfaceNum */
        0x0,/* activeInterfaceMask - calculated from activeInterfaceNum */
        {   /*   csBitmask, mirrorEn, dqsSwapEn, ckSwapEn */
            {{   0x1,       GT_TRUE,  GT_FALSE,  GT_FALSE},
             {   0x1,       GT_TRUE,  GT_FALSE,  GT_FALSE},
             {   0x2,       GT_FALSE, GT_FALSE,  GT_FALSE},
             {   0x2,       GT_FALSE, GT_FALSE,  GT_FALSE}},
            CPSS_DRAM_SPEED_BIN_DDR3_1866M_E, /* speedBin */
            CPSS_DRAM_BUS_WIDTH_16_E,         /* busWidth */
            CPSS_DRAM_512MB_E,                /* memorySize */
            CPSS_DRAM_FREQ_667_MHZ_E,         /* memoryFreq */
            0,                                /* casWL */
            0,                                /* casL */
            CPSS_DRAM_TEMPERATURE_HIGH_E      /* interfaceTemp */
        }
    },
    { /* RD board */
        5,  /* activeInterfaceNum */
        0x0,/* activeInterfaceMask - calculated from activeInterfaceNum */
        {   /*   csBitmask, mirrorEn, dqsSwapEn, ckSwapEn */
            {{   0x1,       GT_FALSE,  GT_FALSE,  GT_FALSE},
             {   0x1,       GT_FALSE,  GT_FALSE,  GT_FALSE},
             {   0x2,       GT_FALSE, GT_FALSE,  GT_FALSE},
             {   0x2,       GT_FALSE, GT_FALSE,  GT_FALSE}},
            CPSS_DRAM_SPEED_BIN_DDR3_1866M_E, /* speedBin */
            CPSS_DRAM_BUS_WIDTH_16_E,         /* busWidth */
            CPSS_DRAM_512MB_E,                /* memorySize */
            CPSS_DRAM_FREQ_667_MHZ_E,         /* memoryFreq */
            0,                                /* casWL */
            0,                                /* casL */
            CPSS_DRAM_TEMPERATURE_HIGH_E      /* interfaceTemp */
        }
    },   /*mtl board */
    {
        4,  /* activeInterfaceNum */
        0x0,/* activeInterfaceMask - calculated from activeInterfaceNum */
        {   /*   csBitmask, mirrorEn, dqsSwapEn, ckSwapEn */
            {{   0x1,       GT_FALSE,  GT_FALSE,  GT_FALSE},
             {   0x1,       GT_FALSE,  GT_FALSE,  GT_FALSE},
             {   0x2,       GT_FALSE, GT_FALSE,  GT_FALSE},
             {   0x2,       GT_FALSE, GT_FALSE,  GT_FALSE}},
            CPSS_DRAM_SPEED_BIN_DDR3_1866M_E, /* speedBin */
            CPSS_DRAM_BUS_WIDTH_16_E,         /* busWidth */
            CPSS_DRAM_512MB_E,                /* memorySize */
            CPSS_DRAM_FREQ_667_MHZ_E,         /* memoryFreq */
            0,                                /* casWL */
            0,                                /* casL */
            CPSS_DRAM_TEMPERATURE_HIGH_E      /* interfaceTemp */
        }
    }
};
static GT_U8 bc2DramCfgArrSize = sizeof(bc2DramCfgArr) / sizeof(bc2DramCfgArr[0]);


#define BOBK_DRAM_CFG_IDX_CAELUM_CNS 0
#define BOBK_DRAM_CFG_IDX_CETUS_CNS  1

/* Dram Cfg by board types from bobkBoardTypeGet */
static CPSS_DXCH_TM_GLUE_DRAM_CFG_STC bobkDramCfgArr[] =
{
    {   /* APP_DEMO_CAELUM_BOARD_DB_CNS */
        3,   /* activeInterfaceNum */
        0x0B,/* activeInterfaceMask - DDR interfaces 0,1,3 are availble for TM */
        {   /*   csBitmask, mirrorEn, dqsSwapEn, ckSwapEn */
            {{   0x1,       GT_FALSE,  GT_FALSE,  GT_FALSE},
             {   0x1,       GT_FALSE,  GT_FALSE,  GT_FALSE},
             {   0x2,       GT_FALSE,  GT_FALSE,  GT_FALSE},
             {   0x2,       GT_FALSE,  GT_FALSE,  GT_FALSE}},
            CPSS_DRAM_SPEED_BIN_DDR3_1866M_E, /* speedBin */
            CPSS_DRAM_BUS_WIDTH_16_E,         /* busWidth */
            CPSS_DRAM_512MB_E,                /* memorySize */
            CPSS_DRAM_FREQ_667_MHZ_E,         /* memoryFreq */
            0,                                /* casWL */
            0,                                /* casL */
            CPSS_DRAM_TEMPERATURE_HIGH_E      /* interfaceTemp */
        }
    },
    {   /* APP_DEMO_CETUS_BOARD_DB_CNS */
        1,  /* activeInterfaceNum */
        0x0,/* activeInterfaceMask - calculated from activeInterfaceNum */
        {   /*   csBitmask, mirrorEn, dqsSwapEn, ckSwapEn */
            {{   0x1,       GT_FALSE,  GT_FALSE,  GT_FALSE},
             {   0x1,       GT_FALSE,  GT_FALSE,  GT_FALSE},
             {   0x2,       GT_FALSE,  GT_FALSE,  GT_FALSE},
             {   0x2,       GT_FALSE,  GT_FALSE,  GT_FALSE}},
            CPSS_DRAM_SPEED_BIN_DDR3_1866M_E, /* speedBin */
            CPSS_DRAM_BUS_WIDTH_16_E,         /* busWidth */
            CPSS_DRAM_512MB_E,                /* memorySize */
            CPSS_DRAM_FREQ_667_MHZ_E,         /* memoryFreq */
            0,                                /* casWL */
            0,                                /* casL */
            CPSS_DRAM_TEMPERATURE_HIGH_E      /* interfaceTemp */
        }
    }
};

static GT_U8 bobkDramCfgArrSize = sizeof(bobkDramCfgArr) / sizeof(bobkDramCfgArr);


GT_STATUS setDRAMIntNumber(GT_U32 intNum)
{
    GT_U8 minIntNum = 2;
    GT_U8 maxIntNum = 5;
    GT_U8 i;

    if (isBobkBoard == GT_TRUE)
    {
        minIntNum = 1;

        if (bc2BoardType == APP_DEMO_CETUS_BOARD_DB_CNS)
            maxIntNum = 1;
        else /* APP_DEMO_CAELUM_BOARD_DB_CNS */
            maxIntNum = 3;
    }

    if(intNum < minIntNum)
        return GT_BAD_PARAM;

    if(intNum > maxIntNum)
        return GT_BAD_PARAM;

    if (isBobkBoard == GT_TRUE)
    {
        for (i = 0; i < bobkDramCfgArrSize; i++)
        {
            bobkDramCfgArr[i].activeInterfaceNum = intNum;
        }
    }
    else
    {
        for (i = 0; i < bc2DramCfgArrSize; i++)
        {
            bc2DramCfgArr[i].activeInterfaceNum = intNum;
        }
    }

    osPrintf("numOfLads has change to: 0x%0x\n", intNum);

    return 0;
}

/*tm scenario mode*/

CPSS_TM_SCENARIO appDemoTmScenarioMode = 0;

extern GT_VOID appDemoTmScenarioModeSet
(
    IN  CPSS_TM_SCENARIO  mode
);

extern GT_STATUS appDemoIsTmSupported(GT_U8 devNum,GT_BOOL *isTmSupported);


/*******************************************************************************
* getBoardInfo
*
* DESCRIPTION:
*       Return the board configuration info.
*
* INPUTS:
*       boardRevId  - The board revision Id.
*
* OUTPUTS:
*       isB2bSystem - GT_TRUE, the system is a B2B system.
*       numOfPp     - Number of Packet processors in system.
*       numOfFa     - Number of Fabric Adaptors in system.
*       numOfXbar   - Number of Crossbar devices in system.
*
* RETURNS:
*       GT_OK   - on success,
*       GT_FAIL - otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static GT_STATUS getBoardInfo
(
    IN  GT_U8   boardRevId,
    OUT GT_U8   *numOfPp,
    OUT GT_U8   *numOfFa,
    OUT GT_U8   *numOfXbar,
    OUT GT_BOOL *isB2bSystem
)
{
    GT_U8       i;                      /* Loop index.                  */
    GT_STATUS   pciStat;

    pciStat = appDemoSysGetPciInfo();
    if(pciStat == GT_OK)
    {
        for(i = 0; i < PRV_CPSS_MAX_PP_DEVICES_CNS; i++)
        {
            if(appDemoPpConfigList[i].valid == GT_TRUE)
            {
                ppCounter++;
            }
        }
    }
    else
        ppCounter = 0;


    /* No PCI devices found */
    if(ppCounter == 0)
    {
        return GT_NOT_FOUND;
    }

    ppCounter = 1;

    *numOfPp    = ppCounter;
    *numOfFa    = 0;
    *numOfXbar  = 0;

    *isB2bSystem = GT_FALSE;/* no B2B with Bobcat2 devices */

    return GT_OK;
}

/*******************************************************************************
* getBoardInfoSimple
*
* DESCRIPTION:
*       Return the board configuration info.
*
* INPUTS:
*       boardRevId  - The board revision Id.
*
* OUTPUTS:
*       numOfPp     - Number of Packet processors in system.
*       ppPhase1Params  - Phase1 parameters struct.
*
* RETURNS:
*       GT_OK   - on success,
*       GT_FAIL - otherwise.
*
* COMMENTS:
*       This is a simplified version of getBoardInfo.
*
*******************************************************************************/
static GT_STATUS getBoardInfoSimple
(
    IN  GT_U8   boardRevId,
    OUT GT_U8   *numOfPp,
    OUT CPSS_DXCH_PP_PHASE1_INIT_INFO_STC  *phase1Params
)
{
    GT_STATUS   rc;
    GT_PCI_INFO pciInfo;

    /* In our case we want to find just one prestera device on PCI bus*/
    rc = gtPresteraGetPciDev(GT_TRUE, &pciInfo);
    if(rc != GT_OK)
    {
        osPrintf("Could not find Prestera device on PCI bus!\n");
        return GT_NOT_FOUND;
    }

    ppCounter++;

    phase1Params->busBaseAddr       = pciInfo.pciBaseAddr;
    phase1Params->internalPciBase   = pciInfo.internalPciBase;
    phase1Params->resetAndInitControllerBase = pciInfo.resetAndInitControllerBase;

#if (defined LINUX_SIM) || ((!defined _linux) && (!defined _FreeBSD))
    /* Correct Base Addresses for PEX devices */
    /* Internal registers BAR is in the 0x18 reg and only 16 MSB are used */
    phase1Params->busBaseAddr = pciInfo.pciHeaderInfo[6] & 0xFFFF0000;

#ifdef ASIC_SIMULATION
    /* correct Internal PCI base to compensate CPSS Driver +
        0x70000 / 0xf0000 PEX offset  */
    phase1Params->internalPciBase &= 0xFFFF;
#else
    /* PEX BAR need to be masked because only 12 MSB are used */
    phase1Params->internalPciBase &= 0xFFF00000;
#endif
#endif

    rc = gtPresteraGetPciDev(GT_FALSE, &pciInfo);
    if(rc == GT_OK)
    {
        osPrintf("More then one Prestera device found on PCI bus!\n");
        return GT_INIT_ERROR;
    }

    *numOfPp    = 1;

    return GT_OK;
}

/*******************************************************************************
* getPpPhase1Config
*
* DESCRIPTION:
*       Returns the configuration parameters for cpssDxChHwPpPhase1Init().
*
* INPUTS:
*       boardRevId      - The board revision Id.
*       devIdx          - The Pp Index to get the parameters for.
*
* OUTPUTS:
*       ppPhase1Params  - Phase1 parameters struct.
*
* RETURNS:
*       GT_OK   - on success,
*       GT_FAIL - otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static GT_STATUS getPpPhase1Config
(
    IN  GT_U8                       boardRevId,
    IN  GT_U8                       devIdx,
    OUT CPSS_PP_PHASE1_INIT_PARAMS  *phase1Params
)
{
    GT_STATUS rc = GT_OK;
    CPSS_PP_PHASE1_INIT_PARAMS    localPpPh1Config = CPSS_PP_PHASE1_DEFAULT;
    GT_U32 coreClockValue;
#ifndef ASIC_SIMULATION
    void    *intVecNum;
#endif
    PRV_CPSS_DXCH_BC2_UNUSED_PARAM_MAC(boardRevId);

    localPpPh1Config.devNum = devIdx;

    localPpPh1Config.baseAddr =
                        appDemoPpConfigList[devIdx].pciInfo.pciBaseAddr;
    localPpPh1Config.internalPciBase =
                        appDemoPpConfigList[devIdx].pciInfo.internalPciBase;
#if 0
    /* Both PP base address & Reset and Init Controller (DFX) are accessed */
    /* through BAR1. The Reset and Init Controller window is the one after */
    /* the PP window, while the PP window is 64MB wide.                    */
#endif
    localPpPh1Config.resetAndInitControllerBase =
                        appDemoPpConfigList[devIdx].pciInfo.resetAndInitControllerBase;

    localPpPh1Config.deviceId =
        ((appDemoPpConfigList[devIdx].pciInfo.pciDevVendorId.devId << 16) |
         (appDemoPpConfigList[devIdx].pciInfo.pciDevVendorId.vendorId));

    localPpPh1Config.mngInterfaceType = CPSS_CHANNEL_PEX_E;
#if 0
    localPpPh1Config.mngInterfaceType = CPSS_CHANNEL_PEX_MBUS_NEWDRV_E;
#endif

    localPpPh1Config.hwAddr.busNo = appDemoPpConfigList[devIdx].pciInfo.pciBusNum;
    localPpPh1Config.hwAddr.devSel = appDemoPpConfigList[devIdx].pciInfo.pciIdSel;
    localPpPh1Config.hwAddr.funcNo = appDemoPpConfigList[devIdx].pciInfo.funcNo;

    localPpPh1Config.numOfPortGroups = appDemoPpConfigList[devIdx].numOfPortGroups;
    localPpPh1Config.portGroupsInfo[0].busBaseAddr =
        appDemoPpConfigList[devIdx].portGroupsInfo[0].portGroupPciInfo.pciBaseAddr;
    localPpPh1Config.portGroupsInfo[0].internalPciBase =
        appDemoPpConfigList[devIdx].portGroupsInfo[0].portGroupPciInfo.internalPciBase;
    rc = extDrvPcieGetInterruptNumber(
            appDemoPpConfigList[devIdx].pciInfo.pciBusNum,
            appDemoPpConfigList[devIdx].pciInfo.pciIdSel,
            appDemoPpConfigList[devIdx].pciInfo.funcNo,
            &(localPpPh1Config.intVecNum));
    if (rc == GT_OK)
    {
        localPpPh1Config.intMask = (GT_UINTPTR)localPpPh1Config.intVecNum;
    }
#ifndef ASIC_SIMULATION
    if (rc != GT_OK)
    {
        /* fallback to old method - msys */
        /* TODO: return an error instead */
        extDrvGetPciIntVec(GT_PCI_INT_A, &intVecNum);
        localPpPh1Config.intVecNum = (GT_U32)((GT_UINTPTR)intVecNum);
        extDrvGetIntMask(GT_PCI_INT_A, &localPpPh1Config.intMask);
        rc = GT_OK;
    }
#endif

/* retrieve PP Core Clock from HW */
    if(GT_OK == appDemoDbEntryGet("coreClock", &coreClockValue))
    {
        localPpPh1Config.coreClk = coreClockValue;
    }
    else
    {
        localPpPh1Config.coreClk = APP_DEMO_CPSS_AUTO_DETECT_CORE_CLOCK_CNS;
#if !defined(ASIC_SIMULATION) && !defined(CPU_MSYS)
    {
        GT_U32  regData;
        GT_BOOL doByteSwap;

        switch (localPpPh1Config.deviceId)
        {
            case CPSS_BOBK_ALL_DEVICES_CASES_MAC:

                #if defined(CPU_LE)
                    doByteSwap = GT_FALSE;
                #else
                    doByteSwap = GT_TRUE;
                #endif

                /* read Device Sample at Reset (SAR) Status<1> register to get PLL 0 Config field */
                rc = cpssDxChDiagRegRead(
                    localPpPh1Config.resetAndInitControllerBase,
                    localPpPh1Config.mngInterfaceType,
                    CPSS_DIAG_PP_REG_PCI_CFG_E, /* use access without address competion for DFX registers */
                    0x000F8204,
                    &regData,
                    doByteSwap);

                if( GT_OK != rc )
                {
                    return rc;
                }

                if (U32_GET_FIELD_MAC(regData, 21, 3) == 7)
                {
                    /* Bobk devices with PLL bypass setting must use predefined coreClock.
                       Use maximal possible coreClock in the case */
                    localPpPh1Config.coreClk = 365;
                }
                break;
            default:
                break;
        }
    }
#endif
    }

    /* current Bobcat2 box has 156.25MHz differencial serdesRefClock */
    localPpPh1Config.serdesRefClock =  CPSS_DXCH_PP_SERDES_REF_CLOCK_EXTERNAL_156_25_DIFF_E;

    localPpPh1Config.uplinkCfg = CPSS_PP_NO_UPLINK_E;

    appDemoPpConfigList[devIdx].flowControlDisable = GT_TRUE;

    localPpPh1Config.enableEarchSupport = GT_TRUE;

    osMemCpy(phase1Params,&localPpPh1Config,sizeof(CPSS_PP_PHASE1_INIT_PARAMS));

    return rc;
}

/*******************************************************************************
* getPpPhase1ConfigSimple
*
* DESCRIPTION:
*       Returns the configuration parameters for cpssDxChHwPpPhase1Init().
*
* INPUTS:
*       boardRevId      - The board revision Id.
*       devIdx          - The Pp Index to get the parameters for.
*
* OUTPUTS:
*       ppPhase1Params  - Phase1 parameters struct.
*
* RETURNS:
*       GT_OK   - on success,
*       GT_FAIL - otherwise.
*
* COMMENTS:
*       This is a simplified version of configBoardAfterPhase1.
*
*******************************************************************************/
static GT_STATUS getPpPhase1ConfigSimple
(
    IN  GT_U8                       boardRevId,
    IN  GT_U8                       devIdx,
    OUT CPSS_DXCH_PP_PHASE1_INIT_INFO_STC  *phase1Params
)
{
    GT_STATUS rc = GT_OK;
#ifndef ASIC_SIMULATION
    void    *intVecNum;
#endif
    PRV_CPSS_DXCH_BC2_UNUSED_PARAM_MAC(boardRevId);

    phase1Params->devNum = devIdx;

#ifndef ASIC_SIMULATION
    extDrvGetPciIntVec(GT_PCI_INT_A, &intVecNum);
    phase1Params->intVecNum = (GT_U32)((GT_UINTPTR)intVecNum);
    extDrvGetIntMask(GT_PCI_INT_A, &(phase1Params->intMask));
#endif
/* retrieve PP Core Clock from HW */
    phase1Params->coreClock             = CPSS_DXCH_AUTO_DETECT_CORE_CLOCK_CNS;

    phase1Params->mngInterfaceType      = CPSS_CHANNEL_PEX_E;
    phase1Params->ppHAState             = CPSS_SYS_HA_MODE_ACTIVE_E;
    /* current Bobcat2 box has 156.25MHz differencial serdesRefClock */
    phase1Params->serdesRefClock        = CPSS_DXCH_PP_SERDES_REF_CLOCK_EXTERNAL_156_25_DIFF_E;
    phase1Params->enableEarchSupport    = GT_TRUE;
    /* 8 Address Completion Region mode                        */
    phase1Params->mngInterfaceType      = CPSS_CHANNEL_PEX_MBUS_E;
    /* Address Completion Region 1 - for Interrupt Handling    */
    phase1Params->isrAddrCompletionRegionsBmp = 0x02;
    /* Address Completion Regions 2,3,4,5 - for Application    */
    phase1Params->appAddrCompletionRegionsBmp = 0x3C;
    /* Address Completion Regions 6,7 - reserved for other CPU */

    phase1Params->initSerdesDefaults    = GT_FALSE;
    phase1Params->numOfPortGroups       = 0; /* irrelevant for BC2*/

    return rc;
}

/*******************************************************************************
* prvBobcat2TmGlueConfig
*
* DESCRIPTION:
*       Bobcat2 TM glue configuration.
*
* APPLICABLE DEVICES:
*        Bobcat2.
*
* NOT APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2; Lion; Lion2; Lion3.
*
* INPUTS:
*       devNum   - physical device number
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
static GT_STATUS prvBobcat2TmGlueConfig
(
    IN  GT_U8  devNum
)
{
    GT_U32      bitIndex;               /* TM queue ID bit */
    CPSS_DXCH_TM_GLUE_QUEUE_MAP_BIT_SELECT_ENTRY_STC   entry;
                                        /* Table entry */
    GT_U32      entryIndex;             /* Table entry index */
    CPSS_DXCH_TM_GLUE_DROP_MASK_STC dropMaskCfg;
                                        /* Drop mask configuration */

    GT_STATUS   rc = GT_OK;             /* Return code */
    GT_U32                              i,j;
    GT_U32                               hwDevNum;
    CPSS_INTERFACE_INFO_STC             physicalInfoPtr;

    entry.queueIdBase = 1;

    /* Qmapper - the bit select table - all queues selected by <TM port, TC> */
    for(entryIndex = 0; entryIndex < 255; entryIndex++)
    {
        if (appDemoTmScenarioMode==CPSS_TM_2 || appDemoTmScenarioMode==CPSS_TM_3) {
            for (bitIndex = 0; bitIndex < 14; bitIndex++)
            {
                if(bitIndex < 3)
                {
                    /* Bits[0...2] - TM TC select type */
                    entry.bitSelectArr[bitIndex].selectType =
                        CPSS_DXCH_TM_GLUE_QUEUE_MAP_BIT_SELECT_TYPE_TM_TC_E;
                    entry.bitSelectArr[bitIndex].bitSelector = bitIndex;
                }
                else if(bitIndex < 14)
                {
                    /* Bits[3...10] - local target PHY port select type */
                    entry.bitSelectArr[bitIndex].selectType =
                        CPSS_DXCH_TM_GLUE_QUEUE_MAP_BIT_SELECT_TYPE_TARGET_EPORT_E;
                    entry.bitSelectArr[bitIndex].bitSelector = bitIndex - 3;
                }
            }
        }
        else
        {
            for (bitIndex = 0; bitIndex < 14; bitIndex++)
            {
                if(bitIndex < 3)
                {
                    /* Bits[0...2] - TM TC select type */
                    entry.bitSelectArr[bitIndex].selectType =
                        CPSS_DXCH_TM_GLUE_QUEUE_MAP_BIT_SELECT_TYPE_TM_TC_E;
                    entry.bitSelectArr[bitIndex].bitSelector = bitIndex;
                }
                else if(bitIndex < 11)
                {
                    /* Bits[3...10] - local target PHY port select type */
                    entry.bitSelectArr[bitIndex].selectType =
                        CPSS_DXCH_TM_GLUE_QUEUE_MAP_BIT_SELECT_TYPE_LOCAL_TARGET_PHY_PORT_E;
                    entry.bitSelectArr[bitIndex].bitSelector = bitIndex - 3;
                }
                else
                {
                    /* Bits[11...13] - zero select type */
                    entry.bitSelectArr[bitIndex].selectType =
                        CPSS_DXCH_TM_GLUE_QUEUE_MAP_BIT_SELECT_TYPE_ZERO_E;
                    entry.bitSelectArr[bitIndex].bitSelector = 0;
                }
            }
        }
        /* Call CPSS API function */
        rc = cpssDxChTmGlueQueueMapBitSelectTableEntrySet(devNum, entryIndex, &entry);
        CPSS_ENABLER_DBG_TRACE_RC_MAC("cpssDxChTmGlueQueueMapBitSelectTableEntrySet", rc);
        if(rc != GT_OK)
        {
            return rc;
        }
    }

    /* Drop profile 0 - consider for drop P level Tail Drop only */
    osMemSet(&dropMaskCfg, 0, sizeof(dropMaskCfg));
    /* Set port tail drop recomenadtion for drop desision */
    dropMaskCfg.portTailDropUnmask = GT_TRUE;
    if (appDemoTmScenarioMode==CPSS_TM_2)
    {
        dropMaskCfg.qTailDropUnmask = GT_TRUE;
        /* Call CPSS API function */
        for (i=0 ; i<16; i++) {
            rc = cpssDxChTmGlueDropProfileDropMaskSet(devNum, 0, i, &dropMaskCfg);
        }
    }
    if (appDemoTmScenarioMode==CPSS_TM_3)
    {
        dropMaskCfg.qTailDropUnmask = GT_TRUE;
        dropMaskCfg.qWredDropUnmask = GT_TRUE;

        /* Call CPSS API function */
        for (i=0 ; i<16; i++) {
            rc = cpssDxChTmGlueDropProfileDropMaskSet(devNum, 0, i, &dropMaskCfg);
        }
    }
    else
    {
        /* Call CPSS API function */
        rc = cpssDxChTmGlueDropProfileDropMaskSet(devNum, 0, 0, &dropMaskCfg);
    }

    CPSS_ENABLER_DBG_TRACE_RC_MAC("cpssDxChTmGlueDropProfileDropMaskSet", rc);
    if(rc != GT_OK)
    {
        return rc;
    }
    /* Enable Egress FIFOs Flow Control for TM */
    rc = cpssDxChTmGlueFlowControlEgressEnableSet(devNum, GT_TRUE);
    CPSS_ENABLER_DBG_TRACE_RC_MAC("cpssDxChTmGlueFlowControlEgressEnableSet", rc);

    hwDevNum = PRV_CPSS_HW_DEV_NUM_MAC(devNum);

    /*configure eport - for each port 200 eports*/
    if (appDemoTmScenarioMode==CPSS_TM_2 || appDemoTmScenarioMode==CPSS_TM_3)
    {
        for (i=0; i<10; i++)
        {
            physicalInfoPtr.devPort.hwDevNum= hwDevNum;
            physicalInfoPtr.devPort.portNum = i;
            physicalInfoPtr.type = CPSS_INTERFACE_PORT_E;
            for (j=0; j<200 ; j++)
            {
                rc= cpssDxChBrgEportToPhysicalPortTargetMappingTableSet(devNum ,(i*200)+ j ,&physicalInfoPtr);
                if(rc != GT_OK)
                {
                    return rc;
                }

                rc= cpssDxChBrgGenMtuPortProfileIdxSet(devNum, (i*200)+ j, 0);
                if(rc != GT_OK)
                {
                    return rc;
                }
            }
        }
    }

    return rc;
}

/*******************************************************************************
* appDemoBc2PtpTaiCheck
*
* DESCRIPTION:
*       PTP TOD dump for all TAI instances.
*
* APPLICABLE DEVICES:
*        Bobcat2.
*
* NOT APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2; Lion; Lion2; Lion3.
*
* INPUTS:
*       devNum   - physical device number
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
GT_STATUS appDemoBc2PtpTaiCheck
(
    IN  GT_U8  devNum
)
{
    CPSS_DXCH_PTP_TAI_ID_STC        taiId;      /* TAI Units identification */
    CPSS_DXCH_PTP_TOD_COUNT_STC     tod;        /* TOD Value */
    GT_STATUS                       rc;         /* return code */
    GT_U32                          iter, i, ii;
    CPSS_DXCH_PTP_TAI_TOD_TYPE_ENT  captureId;

    if(PRV_CPSS_DXCH_PP_MAC(devNum)->hwInfo.gop_tai.supportSingleInstance == GT_TRUE)
    {
        CPSS_TBD_BOOKMARK_BOBCAT2_BOBK
        return GT_OK;
    }

    /* Configure Capture function in all TAIs */
    taiId.taiInstance = CPSS_DXCH_PTP_TAI_INSTANCE_ALL_E;
    taiId.taiNumber = CPSS_DXCH_PTP_TAI_NUMBER_ALL_E;
    rc = cpssDxChPtpTodCounterFunctionSet(devNum, CPSS_PORT_DIRECTION_BOTH_E /* ignored*/,
                                          &taiId, CPSS_DXCH_PTP_TOD_COUNTER_FUNC_CAPTURE_E);
    if(rc != GT_OK)
    {
        return rc;
    }

    /* clear all captured TODs */
    for (iter = 0; iter < 2; iter++)
    {
        captureId = CPSS_DXCH_PTP_TAI_TOD_TYPE_CAPTURE_VALUE0_E + iter;

        /* loop on GOPs */
        for ( i = 0 ; i < 10 ; i++ )
        {
                /* GOP0 port0  - port15                               */
                /* GOP1 port16 - port31                               */
                /* GOP2 port32 - port47                               */
                /* GOP3 port48 - port51                               */
                /* GOP4 port52 - port55                               */
                /* GOP5 port56 - port59                               */
                /* GOP6 port60 - port63                               */
                /* GOP7 port64 - port67                               */
                /* GOP8 port68 - port71                               */

            if (i == 9)
            {
                 taiId.taiInstance = CPSS_DXCH_PTP_TAI_INSTANCE_GLOBAL_E;
            }
            else
            {
                taiId.taiInstance = CPSS_DXCH_PTP_TAI_INSTANCE_PORT_E;
                taiId.portNum = 0xFFFFFF; /* not legal value */
                if (i <= 3)
                {
                    taiId.portNum = i * 16;
                }
                else
                {
                    switch(i)
                    {
                        case 4: taiId.portNum = 52; break;
                        case 5: taiId.portNum = 56; break;
                        case 6: taiId.portNum = 80; break;
                        case 7: taiId.portNum = 64; break;
                        case 8: taiId.portNum = 68; break;
                        default: cpssOsPrintf(" ERROR\n"); return GT_FAIL;
                    }
                }
            }
            /* loop on TAIs */
            for ( ii = 0 ; ii < 2 ; ii++ )
            {
                taiId.taiNumber = ii;

                /* call device specific API */
                rc = cpssDxChPtpTaiTodGet(devNum, &taiId, captureId, &tod);
                if(rc != GT_OK)
                {
                    return rc;
                }
            }
        }
    }


    /* trigger all TAIs simultaneously */
    rc = cpssDxChPtpTaiTodCounterFunctionAllTriggerSet(devNum);
    if(rc != GT_OK)
    {
        return rc;
    }

    for (iter = 0; iter < 2; iter++)
    {
        cpssOsPrintf("TOD iteration %d\n", iter);
        captureId = CPSS_DXCH_PTP_TAI_TOD_TYPE_CAPTURE_VALUE0_E;

        /* loop on GOPs */
        for ( i = 0 ; i < 10 ; i++ )
        {
                /* GOP0 port0  - port15                               */
                /* GOP1 port16 - port31                               */
                /* GOP2 port32 - port47                               */
                /* GOP3 port48 - port51                               */
                /* GOP4 port52 - port55                               */
                /* GOP5 port56 - port59                               */
                /* GOP6 port60 - port63                               */
                /* GOP7 port64 - port67                               */
                /* GOP8 port68 - port71                               */

            if (i == 9)
            {
                 taiId.taiInstance = CPSS_DXCH_PTP_TAI_INSTANCE_GLOBAL_E;
            }
            else
            {
                taiId.taiInstance = CPSS_DXCH_PTP_TAI_INSTANCE_PORT_E;
                taiId.portNum = 0xFFFFFF; /* not legal value */
                if (i <= 3)
                {
                    taiId.portNum = i * 16;
                }
                else
                {
                    switch(i)
                    {
                        case 4: taiId.portNum = 52; break;
                        case 5: taiId.portNum = 56; break;
                        case 6: taiId.portNum = 80; break;
                        case 7: taiId.portNum = 64; break;
                        case 8: taiId.portNum = 68; break;
                        default: cpssOsPrintf(" ERROR\n"); return GT_FAIL;
                    }
                }
            }
            /* loop on TAIs */
            for ( ii = 0 ; ii < 2 ; ii++ )
            {
                taiId.taiNumber = ii;

                /* call device specific API */
                rc = cpssDxChPtpTaiTodGet(devNum, &taiId, captureId, &tod);
                if(rc != GT_OK)
                {
                    cpssOsPrintf("cpssDxChPtpTaiTodGet failure Instance %d TAI %d rc %d\n", i, ii, rc);
                    return rc;
                }

                cpssOsPrintf("TOD %d %d seconds %d nanoseconds %d\n", i, ii, tod.seconds.l[0], tod.nanoSeconds);
            }
        }

        if (iter == 0)
        {
            cpssOsTimerWkAfter(1000);

            /* trigger all TAIs simultaneously */
            rc = cpssDxChPtpTaiTodCounterFunctionAllTriggerSet(devNum);
            if(rc != GT_OK)
            {
                return rc;
            }
        }
    }

    return GT_OK;
}

/*******************************************************************************
* appDemoB2PtpConfig
*
* DESCRIPTION:
*       Bobcat2 PTP and TAIs related configuration.
*
* APPLICABLE DEVICES:
*        Bobcat2.
*
* NOT APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2; Lion; Lion2; Lion3.
*
* INPUTS:
*       devNum   - physical device number
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
GT_STATUS appDemoB2PtpConfig
(
    IN  GT_U8  devNum
)
{
    CPSS_DXCH_PTP_TAI_ID_STC        taiId;      /* TAI Units identification */
    CPSS_DXCH_PTP_TAI_TOD_STEP_STC  todStep;    /* TOD Step */
    GT_U32                          regAddr;    /* register address */
    GT_U32                          regData = 0;/* register value*/
    GT_U32                          pll4Frq = 1;/* PLL4 frequency (in khz) */
    GT_STATUS                       rc;         /* return code */
    GT_PHYSICAL_PORT_NUM            portNum;    /* port number */
    CPSS_DXCH_PTP_TSU_CONTROL_STC   control;    /* control structure */
    GT_U32                          ptpClkInKhz; /* PTP clock in KHz */

    /* Get TAIs reference clock (its half of PLL4 frequency) */
    if(PRV_CPSS_DXCH_PP_MAC(devNum)->hwInfo.dfxServer.supported != GT_FALSE)
    {
        regAddr =
            PRV_CPSS_DXCH_DEV_RESET_AND_INIT_CONTROLLER_REGS_MAC(devNum)->
                                    DFXServerUnitsBC2SpecificRegs.deviceSAR2;
        rc = prvCpssDrvHwPpResetAndInitControllerGetRegField(devNum,
                                                         regAddr, 12, 2,
                                                         &regData);
        if(rc != GT_OK)
        {
            return rc;
        }
    }

    if(PRV_CPSS_DXCH_BOBCAT2_A0_CHECK_MAC(devNum))
    {
        switch(regData)
        {
            case 0: pll4Frq = PLL4_FREQUENCY_1250000_KHZ_CNS;
                    break;
            case 1: pll4Frq = PLL4_FREQUENCY_1093750_KHZ_CNS;
                    break;
            case 2: pll4Frq = PLL4_FREQUENCY_1550000_KHZ_CNS;
                    break;
            default: return GT_BAD_STATE;
        }
        ptpClkInKhz = pll4Frq / 2;
    }
    else
    {
        switch(regData)
        {
            case 0: ptpClkInKhz = 500000; /* 500 MHz */
                    break;
            case 1: ptpClkInKhz = 546875; /* 546.875MHz*/
                    break;
            default: return GT_BAD_STATE;
        }
    }

    /* Configure TAIs nanosec step values */
    taiId.taiInstance = CPSS_DXCH_PTP_TAI_INSTANCE_ALL_E;
    taiId.taiNumber = CPSS_DXCH_PTP_TAI_NUMBER_ALL_E;
    todStep.nanoSeconds = 1000000/ptpClkInKhz;
    todStep.fracNanoSeconds = (GT_U32)(0xFFFFFFFF *
                  (1000000.0/ptpClkInKhz - todStep.nanoSeconds) +
                  (1000000.0/ptpClkInKhz - todStep.nanoSeconds));

    rc = cpssDxChPtpTaiTodStepSet(devNum, &taiId, &todStep);
    CPSS_ENABLER_DBG_TRACE_RC_MAC("cpssDxChPtpTaiTodStepSet", rc);
    if(rc != GT_OK)
    {
        return rc;
    }

    for(portNum = 0; portNum < (appDemoPpConfigList[devNum].maxPortNumber); portNum++)
    {
        CPSS_ENABLER_PORT_SKIP_CHECK(devNum, portNum);

        rc = cpssDxChPtpTsuControlGet(devNum, portNum, &control);
        if(rc != GT_OK)
        {
            return rc;
        }

        if(control.unitEnable == GT_FALSE)
        {
            control.unitEnable = GT_TRUE;
            /* Timestamping unit enable */
            rc = cpssDxChPtpTsuControlSet(devNum, portNum, &control);
            if(rc != GT_OK)
            {
                return rc;
            }
        }
    }

    return GT_OK;
}

/*******************************************************************************
* bobcat2BoardTypePrint
*
* DESCRIPTION:
*       This function prints type of Bobcat2 board.
*
* INPUTS:
*       bc2BoardType - board type.
*                         0 - DB board
*                         1 - RD MSI board
*                         2 - RD MTL board
*
* OUTPUTS:
*       none
*
* RETURNS:
*       none
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static GT_VOID bobcat2BoardTypePrint
(
    IN GT_U8    dev,
    IN  GT_U32  bc2BoardType
)
{
    typedef struct
    {
        GT_U32 boardType;
        GT_CHAR *boardNamePtr;
    }APPDEMO_BOARDTYPE_2_STR_STC;

    static APPDEMO_BOARDTYPE_2_STR_STC boardNameLIst[] =
    {
         {  APP_DEMO_BC2_BOARD_DB_CNS      ,  "DB"         }
        ,{  APP_DEMO_BC2_BOARD_RD_MSI_CNS  ,  "RD MSI"     }
        ,{  APP_DEMO_BC2_BOARD_RD_MTL_CNS  ,  "RD MTL"     }
        ,{  APP_DEMO_CAELUM_BOARD_DB_CNS   ,  "CAELUM-DB"  }
        ,{  APP_DEMO_CETUS_BOARD_DB_CNS    ,  "DB"         }
        ,{  (GT_U32)(~0)                   ,  NULL         }
    };
    /* GT_CHAR *boardNameArr[4] = {"DB", "RD MSI", "RD MTL","Bad Type"}; */
    GT_CHAR  *boardName;
    GT_CHAR *devName;
    GT_CHAR *environment;
    GT_U32 i;

    switch(appDemoPpConfigList[dev].deviceId)
    {
        case CPSS_BOBK_CAELUM_DEVICES_CASES_MAC:
            devName = "BobK-Caelum";
            boardName = boardNameLIst[0].boardNamePtr;
            break;
        case CPSS_BOBK_CETUS_DEVICES_CASES_MAC:
            devName = "BobK-Cetus";
            boardName = boardNameLIst[0].boardNamePtr;
            break;
        case PRV_CPSS_BOBCAT3_ALL_DEVICES:
            devName = "Bobcat3";
            boardName = boardNameLIst[0].boardNamePtr;
            break;
        default:
        {
            devName = "Bobcat2";
            boardName = (GT_CHAR *)NULL;
            for (i = 0 ; boardNameLIst[i].boardNamePtr != NULL; i++)
            {
                if (boardNameLIst[i].boardType == bc2BoardType)
                {
                    boardName = boardNameLIst[i].boardNamePtr;
                    break;
                }
            }
            if (boardName == NULL)
            {
                boardName = "Bad Type";
            }
        }
    }

#ifdef GM_USED
    environment = "GM (Golden Model) - simulation";
#elif defined ASIC_SIMULATION
    environment = "WM (White Model) - simulation";
#else
    environment = "HW (Hardware)";
#endif

    osPrintf("%s Board Type: %s [%s]\n", devName , boardName , environment);
}

/*******************************************************************************
* waInit
*
* DESCRIPTION:
*       init the WA needed after phase1.
*
* INPUTS:
*       boardRevId      - The board revision Id.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK   - on success,
*       GT_FAIL - otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static GT_STATUS waInit
(
    IN  GT_U8   boardRevId
)
{
    GT_STATUS   rc;
    static CPSS_DXCH_IMPLEMENT_WA_ENT   waFromCpss[CPSS_DXCH_IMPLEMENT_WA_LAST_E];
    GT_U32  waIndex=0;
    GT_U8   devNum;
    GT_U32  dbEntryValue;
    GT_BOOL enable30GWa = GT_FALSE;
    GT_U32  ii;

    boardRevId = boardRevId;


    if(GT_OK == appDemoDbEntryGet("BC2_RevA0_40G_2_30GWA", &dbEntryValue))
    {
        enable30GWa = (GT_BOOL)dbEntryValue;
    }


    for(ii = SYSTEM_DEV_NUM_MAC(0); ii < SYSTEM_DEV_NUM_MAC(ppCounter); ++ii)
    {
        waIndex = 0;
        devNum = appDemoPpConfigList[ii].devNum;

        /* state that application want CPSS to implement the WA */
        if(GT_FALSE != enable30GWa)
        {/* if WA enable bit 16 in 0x600006C set on by default */
            waFromCpss[waIndex++] = CPSS_DXCH_IMPLEMENT_WA_BOBCAT2_REV_A0_40G_NOT_THROUGH_TM_IS_PA_30G_E;
        }


        if(waIndex)
        {
            rc = cpssDxChHwPpImplementWaInit(devNum,waIndex,&waFromCpss[0], NULL);
            CPSS_ENABLER_DBG_TRACE_RC_MAC("cpssDxChHwPpImplementWaInit", rc);
            if(rc != GT_OK)
            {
                return rc;
            }
        }
    }

    return GT_OK;
}

/*******************************************************************************
* configBoardAfterPhase1 : relevant data structures for port mapping
*
*       This function performs all needed configurations that should be done
*******************************************************************************/

    /* mapping for scenario 8 */
static CPSS_DXCH_PORT_MAP_STC bc2TmEnableE8Map[] =
{ /* Port,            mappingType                           portGroupm, intefaceNum, txQPort,        tmEnable,      tmPortInd*/
     {   0, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,        0,   GT_NA,             GT_TRUE,            0 }
     ,{   1, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,        1,   1,             GT_FALSE,            0 }
     ,{   2, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,        2,   2,             GT_FALSE,            0 }
     ,{   3, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,        3,   3,             GT_FALSE,            0}
     ,{   4, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,        4,   4,             GT_FALSE,            0 }
     ,{   5, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,        5,   5,             GT_FALSE,            0 }
     ,{   6, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,        6,   6,             GT_FALSE,            0 }
     ,{   7, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,        7,   7,             GT_FALSE,            0 }
     ,{   8, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,        8,   8,             GT_FALSE,            0 }
     ,{   9, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,        9,   9,             GT_FALSE,            0 }
    ,{  63, CPSS_DXCH_PORT_MAPPING_TYPE_CPU_SDMA_E,                   0,    GT_NA,      63,           GT_FALSE,              0}
};

    /* mapping for scenarios 2 and 3 */
static CPSS_DXCH_PORT_MAP_STC bc2TmEnableE2Map[] =
{ /* Port,            mappingType                           portGroupm, intefaceNum, txQPort,        tmEnable,      tmPortInd*/
     {   0, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,        0,   GT_NA,             GT_TRUE,            0 }
    ,{   1, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,        1,   GT_NA,             GT_TRUE,            1 }
    ,{   2, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,        2,   GT_NA,             GT_TRUE,            2 }
    ,{   3, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,        3,   GT_NA,             GT_TRUE,            3 }
    ,{   4, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,        4,   GT_NA,             GT_TRUE,            4 }
    ,{   5, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,        5,   GT_NA,             GT_TRUE,            5 }
    ,{   6, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,        6,   GT_NA,             GT_TRUE,            6 }
    ,{   7, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,        7,   GT_NA,             GT_TRUE,            7 }
    ,{   8, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,        8,   GT_NA,             GT_TRUE,            8 }
    ,{   9, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,        9,   GT_NA,             GT_TRUE,            9 }
    ,{  63, CPSS_DXCH_PORT_MAPPING_TYPE_CPU_SDMA_E,                   0,    GT_NA,      63,           GT_FALSE,              0}
};

static CPSS_DXCH_PORT_MAP_STC cetus_365Mhz_Tm_Map[] =
{ /* Port,            mappingType                           portGroupm, intefaceNum, txQPort, trafficManegerEnable , tmPortInd*/
     {  56, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       56,      GT_NA,        GT_TRUE,           56}
    ,{  57, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       57,      GT_NA,        GT_TRUE,           57}
    ,{  58, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       58,      GT_NA,        GT_TRUE,           58}
    ,{  59, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       59,      GT_NA,        GT_TRUE,           59}
    ,{  63, CPSS_DXCH_PORT_MAPPING_TYPE_CPU_SDMA_E,                   0,    GT_NA,      63,           GT_FALSE,          GT_NA}
    ,{  64, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       64,      56,           GT_FALSE,          GT_NA}/*pay attention TXQ 64 is reserved for TM, so use 56*/
    ,{  65, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       65,      65,           GT_FALSE,          GT_NA}
    ,{  66, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       66,      66,           GT_FALSE,          GT_NA}
    ,{  67, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       67,      67,           GT_FALSE,          GT_NA}
    ,{  68, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       68,      68,           GT_FALSE,          GT_NA}
    ,{  69, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       69,      69,           GT_FALSE,          GT_NA}
    ,{  70, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       70,      70,           GT_FALSE,          GT_NA}
    ,{  71, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       71,      71,           GT_FALSE,          GT_NA}
};

static CPSS_DXCH_PORT_MAP_STC cetus_250Mhz_Tm_Map[] =
{ /* Port,            mappingType                           portGroupm, intefaceNum, txQPort, trafficManegerEnable , tmPortInd*/
     {  56, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       56,      GT_NA,        GT_TRUE,           56}
    ,{  57, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       57,      GT_NA,        GT_TRUE,           57}
    ,{  58, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       58,      58,           GT_FALSE,          GT_NA}
    ,{  59, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       59,      59,           GT_FALSE,          GT_NA}
    ,{  63, CPSS_DXCH_PORT_MAPPING_TYPE_CPU_SDMA_E,                   0,    GT_NA,      63,           GT_FALSE,          GT_NA}
    ,{  64, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       64,      56,           GT_FALSE,          GT_NA}
    ,{  65, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       65,      65,           GT_FALSE,          GT_NA}
    ,{  66, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       66,      66,           GT_FALSE,          GT_NA}
    ,{  67, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       67,      67,           GT_FALSE,          GT_NA}
    ,{  68, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       68,      GT_NA,        GT_TRUE,           68}
    ,{  69, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       69,      GT_NA,        GT_TRUE,           69}
    ,{  70, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       70,      GT_NA,        GT_TRUE,           70}
};

static CPSS_DXCH_PORT_MAP_STC cetus_200Mhz_Tm_Map[] =
{ /* Port,            mappingType                           portGroupm, intefaceNum, txQPort, trafficManegerEnable , tmPortInd*/
     {  56, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       56,      GT_NA,        GT_TRUE,           56}
    ,{  57, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       57,      GT_NA,        GT_TRUE,           57}
    ,{  58, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       58,      58,           GT_FALSE,          GT_NA}
    ,{  59, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       59,      59,           GT_FALSE,          GT_NA}
    ,{  63, CPSS_DXCH_PORT_MAPPING_TYPE_CPU_SDMA_E,                   0,    GT_NA,      63,           GT_FALSE,          GT_NA}
    ,{  64, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       64,      56,           GT_FALSE,          GT_NA}
    ,{  65, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       65,      65,           GT_FALSE,          GT_NA}
    ,{  66, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       66,      GT_NA,        GT_TRUE,           66}
    ,{  67, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       67,      GT_NA,        GT_TRUE,           67}
    ,{  68, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       68,      GT_NA,        GT_TRUE,           68}
    ,{  69, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       69,      GT_NA,        GT_TRUE,           69}
    ,{  70, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       70,      GT_NA,        GT_TRUE,           70}
    ,{  71, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       71,      GT_NA,        GT_TRUE,           71}
};

static CPSS_DXCH_PORT_MAP_STC cetus_167Mhz_Tm_Map[] =
{ /* Port,            mappingType                           portGroupm, intefaceNum, txQPort, trafficManegerEnable , tmPortInd*/
     {  56, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       56,      GT_NA,        GT_TRUE,           56}
    ,{  57, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       57,      GT_NA,        GT_TRUE,           57}
    ,{  58, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       58,      58,           GT_FALSE,          GT_NA}
    ,{  59, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       59,      59,           GT_FALSE,          GT_NA}
    ,{  63, CPSS_DXCH_PORT_MAPPING_TYPE_CPU_SDMA_E,                   0,    GT_NA,      63,           GT_FALSE,          GT_NA}
    ,{  64, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       64,      56,           GT_FALSE,          GT_NA}
    ,{  67, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       67,      GT_NA,        GT_TRUE,           67}
    ,{  68, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       68,      GT_NA,        GT_TRUE,           68}
    ,{  69, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       69,      GT_NA,        GT_TRUE,           69}
    ,{  70, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       70,      GT_NA,        GT_TRUE,           70}
    ,{  71, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       71,      GT_NA,        GT_TRUE,           71}
};

static CPSS_DXCH_PORT_MAP_STC bc2defaultMap_4220[] =
{ /* Port,            mappingType                           portGroupm, intefaceNum, txQPort, trafficManegerEnable , tmPortInd*/
     {   0, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,        0,       0,           GT_FALSE,              0}
    ,{   1, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,        1,       1,           GT_FALSE,              0}
    ,{   2, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,        2,       2,           GT_FALSE,              0}
    ,{   3, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,        3,       3,           GT_FALSE,              0}
    ,{   4, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,        4,       4,           GT_FALSE,              0}
    ,{   5, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,        5,       5,           GT_FALSE,              0}
    ,{   6, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,        6,       6,           GT_FALSE,              0}
    ,{   7, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,        7,       7,           GT_FALSE,              0}
    ,{   8, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,        8,       8,           GT_FALSE,              0}
    ,{   9, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,        9,       9,           GT_FALSE,              0}
    ,{  10, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       10,      10,           GT_FALSE,              0}
    ,{  11, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       11,      11,           GT_FALSE,              0}
    ,{  12, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       12,      12,           GT_FALSE,              0}
    ,{  13, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       13,      13,           GT_FALSE,              0}
    ,{  14, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       14,      14,           GT_FALSE,              0}
    ,{  15, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       15,      15,           GT_FALSE,              0}
    ,{  16, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       16,      16,           GT_FALSE,              0}
    ,{  17, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       17,      17,           GT_FALSE,              0}
    ,{  18, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       18,      18,           GT_FALSE,              0}
    ,{  19, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       19,      19,           GT_FALSE,              0}
    ,{  20, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       20,      20,           GT_FALSE,              0}
    ,{  21, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       21,      21,           GT_FALSE,              0}
    ,{  22, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       22,      22,           GT_FALSE,              0}
    ,{  23, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       23,      23,           GT_FALSE,              0}
    ,{  56, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       56,      56,           GT_FALSE,              0}
    ,{  57, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       57,      57,           GT_FALSE,              0}
    ,{  58, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       58,      58,           GT_FALSE,              0}
    ,{  59, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       59,      59,           GT_FALSE,              0}
    ,{  63, CPSS_DXCH_PORT_MAPPING_TYPE_CPU_SDMA_E,                   0,    GT_NA,      63,           GT_FALSE,              0}
    ,{  64, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       64,      64,           GT_FALSE,              0}
    ,{  65, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       65,      65,           GT_FALSE,              0}
    ,{  66, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       66,      66,           GT_FALSE,              0}
    ,{  67, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       67,      67,           GT_FALSE,              0}
    ,{  68, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       68,      68,           GT_FALSE,              0}
    ,{  69, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       69,      69,           GT_FALSE,              0}
    ,{  70, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       70,      70,           GT_FALSE,              0}
    ,{  71, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       71,      71,           GT_FALSE,              0}
};

static CPSS_DXCH_PORT_MAP_STC bc2defaultMap_8216[] =
{ /* Port,            mappingType                           portGroupm, intefaceNum, txQPort, trafficManegerEnable , tmPortInd*/
     {  52, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       52,      52,           GT_FALSE,              0}
    ,{  53, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       53,      53,           GT_FALSE,              0}
    ,{  54, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       54,      54,           GT_FALSE,              0}
    ,{  55, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       55,      55,           GT_FALSE,              0}
    ,{  56, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       56,      56,           GT_FALSE,              0}
    ,{  57, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       57,      57,           GT_FALSE,              0}
    ,{  58, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       58,      58,           GT_FALSE,              0}
    ,{  59, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       59,      59,           GT_FALSE,              0}
    ,{  63, CPSS_DXCH_PORT_MAPPING_TYPE_CPU_SDMA_E,                   0,    GT_NA,      63,           GT_FALSE,              0}
    ,{  64, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       64,      64,           GT_FALSE,              0}
    ,{  65, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       65,      65,           GT_FALSE,              0}
    ,{  66, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       66,      66,           GT_FALSE,              0}
    ,{  67, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       67,      67,           GT_FALSE,              0}
    ,{  68, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       68,      68,           GT_FALSE,              0}
    ,{  69, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       69,      69,           GT_FALSE,              0}
    ,{  70, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       70,      70,           GT_FALSE,              0}
    ,{  71, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       71,      71,           GT_FALSE,              0}
};

static CPSS_DXCH_PORT_MAP_STC bc2defaultMap_8219[] =
{ /* Port,            mappingType                           portGroupm, intefaceNum, txQPort, trafficManegerEnable , tmPortInd*/
     {   0, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,        0,       0,           GT_FALSE,              0}
    ,{   1, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,        1,       1,           GT_FALSE,              0}
    ,{   2, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,        2,       2,           GT_FALSE,              0}
    ,{   3, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,        3,       3,           GT_FALSE,              0}
    ,{   4, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,        4,       4,           GT_FALSE,              0}
    ,{   5, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,        5,       5,           GT_FALSE,              0}
    ,{   6, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,        6,       6,           GT_FALSE,              0}
    ,{   7, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,        7,       7,           GT_FALSE,              0}
    ,{  48, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       48,      48,           GT_FALSE,              0}
    ,{  49, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       49,      49,           GT_FALSE,              0}
    ,{  50, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       50,      50,           GT_FALSE,              0}
    ,{  51, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       51,      51,           GT_FALSE,              0}
    ,{  52, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       52,      52,           GT_FALSE,              0}
    ,{  53, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       53,      53,           GT_FALSE,              0}
    ,{  54, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       54,      54,           GT_FALSE,              0}
    ,{  55, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       55,      55,           GT_FALSE,              0}
    ,{  56, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       56,      56,           GT_FALSE,              0}
    ,{  57, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       57,      57,           GT_FALSE,              0}
    ,{  58, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       58,      58,           GT_FALSE,              0}
    ,{  59, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       59,      59,           GT_FALSE,              0}
    ,{  63, CPSS_DXCH_PORT_MAPPING_TYPE_CPU_SDMA_E,                   0,    GT_NA,      63,           GT_FALSE,              0}
    ,{  64, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       64,      64,           GT_FALSE,              0}
    ,{  65, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       65,      65,           GT_FALSE,              0}
    ,{  66, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       66,      66,           GT_FALSE,              0}
    ,{  67, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       67,      67,           GT_FALSE,              0}
    ,{  68, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       68,      68,           GT_FALSE,              0}
    ,{  69, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       69,      69,           GT_FALSE,              0}
    ,{  70, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       70,      70,           GT_FALSE,              0}
    ,{  71, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       71,      71,           GT_FALSE,              0}
};

static CPSS_DXCH_PORT_MAP_STC bc2defaultMap_8224[] =
{ /* Port,            mappingType                           portGroupm, intefaceNum, txQPort, trafficManegerEnable , tmPortInd*/
     {  48, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       48,      48,           GT_FALSE,              0}
    ,{  49, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       49,      49,           GT_FALSE,              0}
    ,{  50, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       50,      50,           GT_FALSE,              0}
    ,{  51, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       51,      51,           GT_FALSE,              0}
    ,{  52, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       52,      52,           GT_FALSE,              0}
    ,{  53, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       53,      53,           GT_FALSE,              0}
    ,{  54, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       54,      54,           GT_FALSE,              0}
    ,{  55, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       55,      55,           GT_FALSE,              0}
    ,{  56, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       56,      56,           GT_FALSE,              0}
    ,{  57, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       57,      57,           GT_FALSE,              0}
    ,{  58, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       58,      58,           GT_FALSE,              0}
    ,{  59, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       59,      59,           GT_FALSE,              0}
    ,{  80, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       60,      60,           GT_FALSE,              0}
    ,{  81, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       61,      61,           GT_FALSE,              0}
    ,{  82, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       62,      62,           GT_FALSE,              0}
    ,{  63, CPSS_DXCH_PORT_MAPPING_TYPE_CPU_SDMA_E,                   0,    GT_NA,      63,           GT_FALSE,              0}
    ,{  64, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       64,      64,           GT_FALSE,              0}
    ,{  65, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       65,      65,           GT_FALSE,              0}
    ,{  66, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       66,      66,           GT_FALSE,              0}
    ,{  67, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       67,      67,           GT_FALSE,              0}
    ,{  68, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       68,      68,           GT_FALSE,              0}
    ,{  69, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       69,      69,           GT_FALSE,              0}
    ,{  70, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       70,      70,           GT_FALSE,              0}
    ,{  71, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       71,      71,           GT_FALSE,              0}
};

static CPSS_DXCH_PORT_MAP_STC bc2defaultMap_4221[] =
{ /* Port,            mappingType                           portGroupm, intefaceNum, txQPort, trafficManegerEnable , tmPortInd*/
     {   0, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,        0,       0,           GT_FALSE,          GT_NA}
    ,{   1, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,        1,       1,           GT_FALSE,          GT_NA}
    ,{   2, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,        2,       2,           GT_FALSE,          GT_NA}
    ,{   3, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,        3,       3,           GT_FALSE,          GT_NA}
    ,{   4, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,        4,       4,           GT_FALSE,          GT_NA}
    ,{   5, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,        5,       5,           GT_FALSE,          GT_NA}
    ,{   6, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,        6,       6,           GT_FALSE,          GT_NA}
    ,{   7, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,        7,       7,           GT_FALSE,          GT_NA}
    ,{   8, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,        8,       8,           GT_FALSE,          GT_NA}
    ,{   9, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,        9,       9,           GT_FALSE,          GT_NA}
    ,{  10, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       10,      10,           GT_FALSE,          GT_NA}
    ,{  11, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       11,      11,           GT_FALSE,          GT_NA}
    ,{  12, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       12,      12,           GT_FALSE,          GT_NA}
    ,{  13, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       13,      13,           GT_FALSE,          GT_NA}
    ,{  14, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       14,      14,           GT_FALSE,          GT_NA}
    ,{  15, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       15,      15,           GT_FALSE,          GT_NA}
    ,{  16, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       16,      16,           GT_FALSE,          GT_NA}
    ,{  17, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       17,      17,           GT_FALSE,          GT_NA}
    ,{  18, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       18,      18,           GT_FALSE,          GT_NA}
    ,{  19, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       19,      19,           GT_FALSE,          GT_NA}
    ,{  20, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       20,      20,           GT_FALSE,          GT_NA}
    ,{  21, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       21,      21,           GT_FALSE,          GT_NA}
    ,{  22, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       22,      22,           GT_FALSE,          GT_NA}
    ,{  23, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       23,      23,           GT_FALSE,          GT_NA}
    ,{  24, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       24,      24,           GT_FALSE,          GT_NA}
    ,{  25, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       25,      25,           GT_FALSE,          GT_NA}
    ,{  26, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       26,      26,           GT_FALSE,          GT_NA}
    ,{  27, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       27,      27,           GT_FALSE,          GT_NA}
    ,{  28, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       28,      28,           GT_FALSE,          GT_NA}
    ,{  29, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       29,      29,           GT_FALSE,          GT_NA}
    ,{  30, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       30,      30,           GT_FALSE,          GT_NA}
    ,{  31, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       31,      31,           GT_FALSE,          GT_NA}
    ,{  32, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       32,      32,           GT_FALSE,          GT_NA}
    ,{  33, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       33,      33,           GT_FALSE,          GT_NA}
    ,{  34, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       34,      34,           GT_FALSE,          GT_NA}
    ,{  35, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       35,      35,           GT_FALSE,          GT_NA}
    ,{  36, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       36,      36,           GT_FALSE,          GT_NA}
    ,{  37, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       37,      37,           GT_FALSE,          GT_NA}
    ,{  38, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       38,      38,           GT_FALSE,          GT_NA}
    ,{  39, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       39,      39,           GT_FALSE,          GT_NA}
    ,{  40, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       40,      40,           GT_FALSE,          GT_NA}
    ,{  41, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       41,      41,           GT_FALSE,          GT_NA}
    ,{  42, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       42,      42,           GT_FALSE,          GT_NA}
    ,{  43, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       43,      43,           GT_FALSE,          GT_NA}
    ,{  44, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       44,      44,           GT_FALSE,          GT_NA}
    ,{  45, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       45,      45,           GT_FALSE,          GT_NA}
    ,{  46, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       46,      46,           GT_FALSE,          GT_NA}
    ,{  47, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       47,      47,           GT_FALSE,          GT_NA}
    ,{  56, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       56,      56,           GT_FALSE,          GT_NA}
    ,{  57, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       57,      57,           GT_FALSE,          GT_NA}
    ,{  58, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       58,      58,           GT_FALSE,          GT_NA}
    ,{  59, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       59,      59,           GT_FALSE,          GT_NA}
    ,{  63, CPSS_DXCH_PORT_MAPPING_TYPE_CPU_SDMA_E,                   0,    GT_NA,      63,           GT_FALSE,          GT_NA}
    ,{  64, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       64,      64,           GT_FALSE,          GT_NA}
    ,{  65, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       65,      65,           GT_FALSE,          GT_NA}
    ,{  66, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       66,      66,           GT_FALSE,          GT_NA}
    ,{  67, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       67,      67,           GT_FALSE,          GT_NA}
    ,{  68, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       68,      68,           GT_FALSE,          GT_NA}
    ,{  69, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       69,      69,           GT_FALSE,          GT_NA}
    ,{  70, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       70,      70,           GT_FALSE,          GT_NA}
    ,{  71, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       71,      71,           GT_FALSE,          GT_NA}
};

static CPSS_DXCH_PORT_MAP_STC bc2defaultMap_4222[] =
{ /* Port,            mappingType                           portGroupm, intefaceNum, txQPort, trafficManegerEnable , tmPortInd*/
    {   0, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,        0,       0,           GT_FALSE,           GT_NA}
   ,{   1, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,        1,       1,           GT_FALSE,           GT_NA}
   ,{   2, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,        2,       2,           GT_FALSE,           GT_NA}
   ,{   3, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,        3,       3,           GT_FALSE,           GT_NA}
   ,{   4, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,        4,       4,           GT_FALSE,           GT_NA}
   ,{   5, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,        5,       5,           GT_FALSE,           GT_NA}
   ,{   6, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,        6,       6,           GT_FALSE,           GT_NA}
   ,{   7, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,        7,       7,           GT_FALSE,           GT_NA}
   ,{   8, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,        8,       8,           GT_FALSE,           GT_NA}
   ,{   9, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,        9,       9,           GT_FALSE,           GT_NA}
   ,{  10, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       10,      10,           GT_FALSE,           GT_NA}
   ,{  11, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       11,      11,           GT_FALSE,           GT_NA}
   ,{  12, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       12,      12,           GT_FALSE,           GT_NA}
   ,{  13, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       13,      13,           GT_FALSE,           GT_NA}
   ,{  14, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       14,      14,           GT_FALSE,           GT_NA}
   ,{  15, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       15,      15,           GT_FALSE,           GT_NA}
   ,{  16, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       16,      16,           GT_FALSE,           GT_NA}
   ,{  17, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       17,      17,           GT_FALSE,           GT_NA}
   ,{  18, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       18,      18,           GT_FALSE,           GT_NA}
   ,{  19, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       19,      19,           GT_FALSE,           GT_NA}
   ,{  20, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       20,      20,           GT_FALSE,           GT_NA}
   ,{  21, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       21,      21,           GT_FALSE,           GT_NA}
   ,{  22, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       22,      22,           GT_FALSE,           GT_NA}
   ,{  23, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       23,      23,           GT_FALSE,           GT_NA}
   ,{  24, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       24,      24,           GT_FALSE,           GT_NA}
   ,{  25, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       25,      25,           GT_FALSE,           GT_NA}
   ,{  26, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       26,      26,           GT_FALSE,           GT_NA}
   ,{  27, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       27,      27,           GT_FALSE,           GT_NA}
   ,{  28, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       28,      28,           GT_FALSE,           GT_NA}
   ,{  29, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       29,      29,           GT_FALSE,           GT_NA}
   ,{  30, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       30,      30,           GT_FALSE,           GT_NA}
   ,{  31, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       31,      31,           GT_FALSE,           GT_NA}
   ,{  32, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       32,      32,           GT_FALSE,           GT_NA}
   ,{  33, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       33,      33,           GT_FALSE,           GT_NA}
   ,{  34, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       34,      34,           GT_FALSE,           GT_NA}
   ,{  35, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       35,      35,           GT_FALSE,           GT_NA}
   ,{  36, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       36,      36,           GT_FALSE,           GT_NA}
   ,{  37, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       37,      37,           GT_FALSE,           GT_NA}
   ,{  38, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       38,      38,           GT_FALSE,           GT_NA}
   ,{  39, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       39,      39,           GT_FALSE,           GT_NA}
   ,{  40, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       40,      40,           GT_FALSE,           GT_NA}
   ,{  41, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       41,      41,           GT_FALSE,           GT_NA}
   ,{  42, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       42,      42,           GT_FALSE,           GT_NA}
   ,{  43, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       43,      43,           GT_FALSE,           GT_NA}
   ,{  44, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       44,      44,           GT_FALSE,           GT_NA}
   ,{  45, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       45,      45,           GT_FALSE,           GT_NA}
   ,{  46, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       46,      46,           GT_FALSE,           GT_NA}
   ,{  47, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       47,      47,           GT_FALSE,           GT_NA}
   ,{  80, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       60,      60,           GT_FALSE,           GT_NA}
   ,{  81, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       61,      61,           GT_FALSE,           GT_NA} /* virtual router port */
   ,{  82, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       62,      62,           GT_FALSE,           GT_NA}
   ,{  63, CPSS_DXCH_PORT_MAPPING_TYPE_CPU_SDMA_E,                   0,    GT_NA,      63,           GT_FALSE,           GT_NA}
   ,{  64, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       64,      64,           GT_FALSE,           GT_NA}
   ,{  65, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       65,      65,           GT_FALSE,           GT_NA}
   ,{  66, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       66,      66,           GT_FALSE,           GT_NA}
   ,{  67, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       67,      67,           GT_FALSE,           GT_NA}
   ,{  68, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       68,      68,           GT_FALSE,           GT_NA}
   ,{  69, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       69,      69,           GT_FALSE,           GT_NA}
   ,{  70, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       70,      70,           GT_FALSE,           GT_NA}
   ,{  71, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       71,      71,           GT_FALSE,           GT_NA}
};
/* ports 56..59 , 64..71 */
static CPSS_DXCH_PORT_MAP_STC bc2defaultMap_56to59_64to71[] =
{ /* Port,            mappingType                           portGroupm, intefaceNum, txQPort, trafficManegerEnable , tmPortInd*/
     {  56, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       56,      56,           GT_FALSE,          GT_NA}
    ,{  57, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       57,      57,           GT_FALSE,          GT_NA}
    ,{  58, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       58,      58,           GT_FALSE,          GT_NA}
    ,{  59, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       59,      59,           GT_FALSE,          GT_NA}
    ,{  63, CPSS_DXCH_PORT_MAPPING_TYPE_CPU_SDMA_E,                   0,    GT_NA,      63,           GT_FALSE,          GT_NA}
    ,{  64, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       64,      64,           GT_FALSE,          GT_NA}
    ,{  65, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       65,      65,           GT_FALSE,          GT_NA}
    ,{  66, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       66,      66,           GT_FALSE,          GT_NA}
    ,{  67, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       67,      67,           GT_FALSE,          GT_NA}
    ,{  68, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       68,      68,           GT_FALSE,          GT_NA}
    ,{  69, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       69,      69,           GT_FALSE,          GT_NA}
    ,{  70, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       70,      70,           GT_FALSE,          GT_NA}
    ,{  71, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       71,      71,           GT_FALSE,          GT_NA}
};

/* ports 64..71 */
static CPSS_DXCH_PORT_MAP_STC bc2defaultMap_64to71[] =
{ /* Port,            mappingType                           portGroupm, intefaceNum, txQPort, trafficManegerEnable , tmPortInd*/
     {  64, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       64,      64,           GT_FALSE,          GT_NA}
    ,{  65, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       65,      65,           GT_FALSE,          GT_NA}
    ,{  66, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       66,      66,           GT_FALSE,          GT_NA}
    ,{  67, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       67,      67,           GT_FALSE,          GT_NA}
    ,{  68, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       68,      68,           GT_FALSE,          GT_NA}
    ,{  69, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       69,      69,           GT_FALSE,          GT_NA}
    ,{  70, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       70,      70,           GT_FALSE,          GT_NA}
    ,{  71, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       71,      71,           GT_FALSE,          GT_NA}
};

static CPSS_DXCH_PORT_MAP_STC bc2defaultMap_4253[] =
{ /* Port,            mappingType                           portGroupm, intefaceNum, txQPort, trafficManegerEnable , tmPortInd*/
    {   0, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       32,      32,           GT_FALSE,           GT_NA}
   ,{   1, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       33,      33,           GT_FALSE,           GT_NA}
   ,{   2, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       34,      34,           GT_FALSE,           GT_NA}
   ,{   3, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       35,      35,           GT_FALSE,           GT_NA}
   ,{   4, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       36,      36,           GT_FALSE,           GT_NA}
   ,{   5, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       37,      37,           GT_FALSE,           GT_NA}
   ,{   6, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       38,      38,           GT_FALSE,           GT_NA}
   ,{   7, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       39,      39,           GT_FALSE,           GT_NA}
   ,{   8, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       40,      40,           GT_FALSE,           GT_NA}
   ,{   9, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       41,      41,           GT_FALSE,           GT_NA}
   ,{  10, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       42,      42,           GT_FALSE,           GT_NA}
   ,{  11, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       43,      43,           GT_FALSE,           GT_NA}
   ,{  12, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       44,      44,           GT_FALSE,           GT_NA}
   ,{  13, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       45,      45,           GT_FALSE,           GT_NA}
   ,{  14, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       46,      46,           GT_FALSE,           GT_NA}
   ,{  15, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       47,      47,           GT_FALSE,           GT_NA}
   ,{  24, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       52,      52,           GT_FALSE,           GT_NA}
   ,{  25, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       53,      53,           GT_FALSE,           GT_NA}
   ,{  26, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       54,      54,           GT_FALSE,           GT_NA}
   ,{  27, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       55,      55,           GT_FALSE,           GT_NA}
   ,{  28, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       56,      56,           GT_FALSE,           GT_NA}
   ,{  29, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       57,      57,           GT_FALSE,           GT_NA}
   ,{  30, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       58,      58,           GT_FALSE,           GT_NA}
   ,{  31, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       59,      59,           GT_FALSE,           GT_NA}
   ,{  63, CPSS_DXCH_PORT_MAPPING_TYPE_CPU_SDMA_E,                   0,    GT_NA,      63,           GT_FALSE,           GT_NA}
};
/*number of DQ ports in each of the 6 DQ units in TXQ */
#define BC3_NUM_PORTS_PER_DQ_IN_TXQ_CNS     SIP_5_20_DQ_NUM_PORTS_CNS
/*macro to convert local port and data path index to TXQ port */
#define BC3_LOCAL_PORT_IN_DP_TO_TXQ_PORT_MAC(localPort , dpIndex) \
    (localPort) + ((dpIndex) * BC3_NUM_PORTS_PER_DQ_IN_TXQ_CNS)
/* build TXQ_port from global mac port */
#define BC3_TXQ_PORT(globalMacPort)    BC3_LOCAL_PORT_IN_DP_TO_TXQ_PORT_MAC((globalMacPort)%12,(globalMacPort)/12)

/* build TXQ_port for 'cpu port' */
#define BC3_TXQ_CPU_PORT    BC3_LOCAL_PORT_IN_DP_TO_TXQ_PORT_MAC(12,0)

#define BC3_CPU_PORT_MAC_AND_TXQ_PORT_MAC        GT_NA , BC3_TXQ_CPU_PORT

#define BC3_MAC_FROM_PIPE_0_MAC(macPort) ((macPort)%36)
/* build pair of mac port and it's TXQ port*/
#define BC3_MAC_AND_TXQ_PORT_MAC(macPort)  \
    BC3_MAC_FROM_PIPE_0_MAC(macPort), BC3_TXQ_PORT(macPort)

#if 0 /*ndef GM_USED*/
static CPSS_DXCH_PORT_MAP_STC bc3defaultMap[] =
{ /* Port,            mappingType                           portGroupm,         intefaceNum, txQPort,      trafficManegerEnable , tmPortInd*/
     {   0, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC( 0),  GT_FALSE,   GT_NA}
    ,{   1, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC( 1),  GT_FALSE,   GT_NA}
    ,{   2, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC( 2),  GT_FALSE,   GT_NA}
    ,{   3, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC( 3),  GT_FALSE,   GT_NA}
/*  use those macs for ports in high range
    ,{   4, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC( 4),  GT_FALSE,   GT_NA}
    ,{   5, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC( 5),  GT_FALSE,   GT_NA}
    ,{   6, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC( 6),  GT_FALSE,   GT_NA}
    ,{   7, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC( 7),  GT_FALSE,   GT_NA}

    ,{   8, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC( 8),  GT_FALSE,   GT_NA}
    ,{   9, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC( 9),  GT_FALSE,   GT_NA}
    ,{  10, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC(10),  GT_FALSE,   GT_NA}
    ,{  11, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC(11),  GT_FALSE,   GT_NA}
    ,{  12, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC(12),  GT_FALSE,   GT_NA}
    ,{  13, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC(13),  GT_FALSE,   GT_NA}
    ,{  14, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC(14),  GT_FALSE,   GT_NA}
    ,{  15, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC(15),  GT_FALSE,   GT_NA}
*/
    ,{  16, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC(16),  GT_FALSE,   GT_NA}
    ,{  17, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC(17),  GT_FALSE,   GT_NA}
    ,{  18, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC(18),  GT_FALSE,   GT_NA}
    ,{  19, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC(19),  GT_FALSE,   GT_NA}
    ,{  20, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC(20),  GT_FALSE,   GT_NA}
    ,{  21, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC(21),  GT_FALSE,   GT_NA}
    ,{  22, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC(22),  GT_FALSE,   GT_NA}
    ,{  23, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC(23),  GT_FALSE,   GT_NA}
/*  use those macs for ports in high range
    ,{  24, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC(24),  GT_FALSE,   GT_NA}
    ,{  25, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC(25),  GT_FALSE,   GT_NA}
    ,{  26, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC(26),  GT_FALSE,   GT_NA}
    ,{  27, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC(27),  GT_FALSE,   GT_NA}
    ,{  28, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC(28),  GT_FALSE,   GT_NA}
    ,{  29, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC(29),  GT_FALSE,   GT_NA}
    ,{  30, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC(30),  GT_FALSE,   GT_NA}
    ,{  31, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC(31),  GT_FALSE,   GT_NA}

    ,{  32, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC(32),  GT_FALSE,   GT_NA}
    ,{  33, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC(33),  GT_FALSE,   GT_NA}
    ,{  34, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC(34),  GT_FALSE,   GT_NA}
    ,{  35, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC(35),  GT_FALSE,   GT_NA}
*/
    ,{  36, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC(4),  GT_FALSE,   GT_NA}
    ,{  37, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC(5),  GT_FALSE,   GT_NA}
    ,{  38, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC(6),  GT_FALSE,   GT_NA}
    ,{  39, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC(7),  GT_FALSE,   GT_NA}
/*
    ,{  40, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC(40),  GT_FALSE,   GT_NA}
    ,{  41, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC(41),  GT_FALSE,   GT_NA}
    ,{  42, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC(42),  GT_FALSE,   GT_NA}
    ,{  43, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC(43),  GT_FALSE,   GT_NA}
    ,{  44, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC(44),  GT_FALSE,   GT_NA}
    ,{  45, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC(45),  GT_FALSE,   GT_NA}
    ,{  46, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC(46),  GT_FALSE,   GT_NA}
    ,{  47, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC(47),  GT_FALSE,   GT_NA}
*/
    ,{  48, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC( 8),  GT_FALSE,   GT_NA}
    ,{  49, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC( 9),  GT_FALSE,   GT_NA}
    ,{  50, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC(10),  GT_FALSE,   GT_NA}
    ,{  51, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC(11),  GT_FALSE,   GT_NA}
    ,{  52, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC(12),  GT_FALSE,   GT_NA}
    ,{  53, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC(13),  GT_FALSE,   GT_NA}
    ,{  54, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC(14),  GT_FALSE,   GT_NA}
    ,{  55, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC(15),  GT_FALSE,   GT_NA}

    ,{  56, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC(24),  GT_FALSE,   GT_NA}
    ,{  57, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC(25),  GT_FALSE,   GT_NA}
    ,{  58, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC(26),  GT_FALSE,   GT_NA}
    ,{  59, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC(27),  GT_FALSE,   GT_NA}

    ,{  80, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC(28),  GT_FALSE,   GT_NA}
    ,{  81, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC(29),  GT_FALSE,   GT_NA} /* virtual router port */
    ,{  82, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC(30),  GT_FALSE,   GT_NA}

    ,{  63, CPSS_DXCH_PORT_MAPPING_TYPE_CPU_SDMA_E,                   0,       BC3_CPU_PORT_MAC_AND_TXQ_PORT_MAC,  GT_FALSE,   GT_NA}
    ,{  64, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC(31),  GT_FALSE,   GT_NA}
    ,{  65, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC(32),  GT_FALSE,   GT_NA}
    ,{  66, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC(33),  GT_FALSE,   GT_NA}
    ,{  67, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC(34),  GT_FALSE,   GT_NA}
    ,{  68, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC(35),  GT_FALSE,   GT_NA}
/*
    ,{  69, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC(44),  GT_FALSE,   GT_NA}
    ,{  70, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC(45),  GT_FALSE,   GT_NA}
    ,{  71, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC(46),  GT_FALSE,   GT_NA}
*/
};
#else /*GM_USED*/
/*NOTE: the GM supports only single DP units , and single TXQ-DQ due to memory issues. */
/* so use only ports of DMA 0..11 , CPU port that uses the DMA 12 */
static CPSS_DXCH_PORT_MAP_STC bc3defaultMap[] =
{ /* Port,            mappingType                           portGroupm,         intefaceNum, txQPort,      trafficManegerEnable , tmPortInd*/
/*0*/  {   0, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC( 0),  GT_FALSE,   GT_NA}
/*1*/ ,{   1, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC( 1),  GT_FALSE,   GT_NA}
/*2*/ ,{   2, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC( 2),  GT_FALSE,   GT_NA}
/*3*/ ,{   3, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC( 3),  GT_FALSE,   GT_NA}
/*4*/ ,{  18, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC( 4),  GT_FALSE,   GT_NA}
/*5*/ ,{  36, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC( 5),  GT_FALSE,   GT_NA}
/*6*/ ,{  54, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC( 6),  GT_FALSE,   GT_NA}
/*7*/ ,{  58, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC( 7),  GT_FALSE,   GT_NA}
/*8*/ ,{  59, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC( 8),  GT_FALSE,   GT_NA}
/*9*/ ,{  64, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC( 9),  GT_FALSE,   GT_NA}
/*10*/,{  65, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC(10),  GT_FALSE,   GT_NA}
/*11*/,{  80, CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E,               0,       BC3_MAC_AND_TXQ_PORT_MAC(11),  GT_FALSE,   GT_NA}
/*12*/,{  63, CPSS_DXCH_PORT_MAPPING_TYPE_CPU_SDMA_E,                   0,       BC3_CPU_PORT_MAC_AND_TXQ_PORT_MAC,  GT_FALSE,   GT_NA}
};
#endif /*GM_USED*/
/* SERDES number of MSYS OOB Port */
#define BC2_BOARD_MSYS_OOB_PORT_SERDES_CNS 21

/*******************************************************************************
* appDemoBc2IsInternalCpuEnabled
*
* DESCRIPTION:
*       Check if internal CPU enabled for Bobcat2 boards.
*
* APPLICABLE DEVICES:
*        Bobcat2.
*
* NOT APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2; Lion; Lion2; Lion3.
*
* INPUTS:
*       devNum   - physical device number
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_TRUE                  - internal CPU enabled
*       GT_FALSE                 - internal CPU disabled
*
* COMMENTS:
*
*******************************************************************************/
GT_BOOL appDemoBc2IsInternalCpuEnabled
(
    IN  GT_U8  devNum
)
{
    GT_U32                          regAddr;  /* register address */
    GT_U32                          regData;  /* register value*/
    GT_STATUS                       rc;       /* return code */

    /* Get msys_tm_ddr_sel SAR configuration */
    if(PRV_CPSS_DXCH_PP_MAC(devNum)->hwInfo.dfxServer.supported != GT_FALSE)
    {
        regAddr =
            PRV_CPSS_DXCH_DEV_RESET_AND_INIT_CONTROLLER_REGS_MAC(devNum)->
                                    DFXServerUnitsBC2SpecificRegs.deviceSAR1;
        rc = prvCpssDrvHwPpResetAndInitControllerGetRegField(devNum,
                                                         regAddr, 11, 1,
                                                         &regData);
        if(rc != GT_OK)
        {
            return GT_FALSE;
        }

        if (regData == 1)
        {
            return GT_TRUE;
        }
    }

    return GT_FALSE;
}

/*******************************************************************************
* configBoardAfterPhase1
*
* DESCRIPTION:
*       This function performs all needed configurations that should be done
*       after phase1.
*
* INPUTS:
*       boardRevId      - The board revision Id.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK   - on success,
*       GT_FAIL - otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static GT_STATUS configBoardAfterPhase1
(
    IN  GT_U8   boardRevId
)
{
    GT_U8 dev;
    GT_STATUS rc;
    GT_U32                   mapArrLen;
    CPSS_DXCH_PORT_MAP_STC  *mapArrPtr, *mapPtr;
    GT_U32  noCpu;
    GT_U32    i;
    GT_U32         coreClockDB;
    GT_U32         coreClockHW;

    rc = configBoardAfterPhase1MappingGet(boardRevId,/*OUT*/&mapArrPtr,&mapArrLen);
    if(rc != GT_OK)
    {
        return rc;
    }

    if (mapArrPtr == NULL)
    {
        cpssOsPrintf("\n-->ERROR : configBoardAfterPhase1() mapping : rev =%d is not supported", boardRevId);
        return GT_NOT_SUPPORTED;
    }

    rc = cpssDxChHwCoreClockGet(appDemoPpConfigList[SYSTEM_DEV_NUM_MAC(0)].devNum, &coreClockDB, &coreClockHW);
    if(rc != GT_OK)
    {
        return rc;
    }

    switch(appDemoPpConfigList[SYSTEM_DEV_NUM_MAC(0)].deviceId)
    {
        case CPSS_98DX4220_CNS:
        case CPSS_98DX4204_CNS:
        case CPSS_98DX4210_CNS:
            mapArrPtr = &bc2defaultMap_4220[0];
            mapArrLen = sizeof(bc2defaultMap_4220)/sizeof(bc2defaultMap_4220[0]);
            break;
        case CPSS_98DX42KK_CNS:
        case CPSS_98DX4221_CNS:
        case CPSS_98DX4203_CNS:
        case CPSS_98DX4211_CNS:
            mapArrPtr = &bc2defaultMap_4221[0];
            mapArrLen = sizeof(bc2defaultMap_4221)/sizeof(bc2defaultMap_4221[0]);
            break;
        case CPSS_98DX4222_CNS:
            mapArrPtr = &bc2defaultMap_4222[0];
            mapArrLen = sizeof(bc2defaultMap_4222)/sizeof(bc2defaultMap_4222[0]);
            break;
        case CPSS_98DX8216_CNS:
            mapArrPtr = &bc2defaultMap_8216[0];
            mapArrLen = sizeof(bc2defaultMap_8216)/sizeof(bc2defaultMap_8216[0]);
            break;
        case CPSS_98DX8219_CNS:
            mapArrPtr = &bc2defaultMap_8219[0];
            mapArrLen = sizeof(bc2defaultMap_8219)/sizeof(bc2defaultMap_8219[0]);
            break;
        case CPSS_98DX8224_CNS:
            mapArrPtr = &bc2defaultMap_8224[0];
            mapArrLen = sizeof(bc2defaultMap_8224)/sizeof(bc2defaultMap_8224[0]);
            break;
        case CPSS_98DX4253_CNS:
            mapArrPtr = &bc2defaultMap_4253[0];
            mapArrLen = sizeof(bc2defaultMap_4253)/sizeof(bc2defaultMap_4253[0]);
            break;
        case CPSS_BOBK_CETUS_DEVICES_CASES_MAC:
            mapArrPtr = &bc2defaultMap_56to59_64to71[0];
            mapArrLen = sizeof(bc2defaultMap_56to59_64to71)/sizeof(bc2defaultMap_56to59_64to71[0]);
            appDemoPpConfigList[SYSTEM_DEV_NUM_MAC(0)].portInitlist_AllPorts_used = 0;
            if(boardRevId == 2)
            {
                switch(coreClockDB)
                {
                    case 365:
                        mapArrPtr = &cetus_365Mhz_Tm_Map[0];
                        mapArrLen = sizeof(cetus_365Mhz_Tm_Map)/sizeof(cetus_365Mhz_Tm_Map[0]);
                        break;
                    case 250:
                        mapArrPtr = &cetus_250Mhz_Tm_Map[0];
                        mapArrLen = sizeof(cetus_250Mhz_Tm_Map)/sizeof(cetus_250Mhz_Tm_Map[0]);
                        break;
                    case 200:
                        mapArrPtr = &cetus_200Mhz_Tm_Map[0];
                        mapArrLen = sizeof(cetus_200Mhz_Tm_Map)/sizeof(cetus_200Mhz_Tm_Map[0]);
                        break;
                    case 167:
                        mapArrPtr = &cetus_167Mhz_Tm_Map[0];
                        mapArrLen = sizeof(cetus_167Mhz_Tm_Map)/sizeof(cetus_167Mhz_Tm_Map[0]);
                        break;
                    default:
                        cpssOsPrintf("configBoardAfterPhase1: illegal core clock %d\n", coreClockDB);
                        return GT_BAD_PARAM;
                }
            }
            if (appDemoPpConfigList[SYSTEM_DEV_NUM_MAC(0)].deviceId ==  CPSS_98DX8208_CNS)
            {
                mapArrPtr = &bc2defaultMap_64to71[0];
                mapArrLen = sizeof(bc2defaultMap_64to71)/sizeof(bc2defaultMap_64to71[0]);
            }
            break;
        case PRV_CPSS_BOBCAT3_ALL_DEVICES:
            mapArrPtr = &bc3defaultMap[0];
            mapArrLen = sizeof(bc3defaultMap)/sizeof(bc3defaultMap[0]);
            break;
        default:
            break;
    }

    /* TM related port mapping adjustments for BC2 only */
    if (!isBobkBoard)
    {
        if (appDemoTmScenarioMode == CPSS_TM_2 || appDemoTmScenarioMode == CPSS_TM_3)
        {
            mapArrPtr = &bc2TmEnableE2Map[0];
            mapArrLen = sizeof(bc2TmEnableE2Map)/sizeof(bc2TmEnableE2Map[0]);
        }

        if (appDemoTmScenarioMode == CPSS_TM_8)
        {
            mapArrPtr = &bc2TmEnableE8Map[0];
            mapArrLen = sizeof(bc2TmEnableE8Map)/sizeof(bc2TmEnableE8Map[0]);
        }
    }
    else
    {
        if (appDemoTmScenarioMode == CPSS_TM_2 ||
            appDemoTmScenarioMode == CPSS_TM_3 ||
            appDemoTmScenarioMode == CPSS_TM_8)

        cpssOsPrintf("TM Scenario is not Availbale on Current System\n");
    }

    rc = appDemoDbEntryGet("noCpu", &noCpu);
    if(rc != GT_OK)
    {
        noCpu = 0;
    }

    if(1 == noCpu)
    {
        for (i = 0, mapPtr = mapArrPtr; i < mapArrLen; i++, mapPtr++)
        {
            /* default map use MAC 63 for physical port 82.
               Need to use MAC 62 for "no CPU" systems to avoid collisions
               with configuration below. */
            if((82 == mapPtr->physicalPortNumber) &&
               (mapPtr->mappingType == CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E) &&
               (mapPtr->interfaceNum == 63))
            {
                mapPtr->interfaceNum = 62;
            }

            /* change physical port 63 to be usual ethernet port but not SDMA one.*/
            if(63 == mapPtr->physicalPortNumber)
            {
                mapPtr->physicalPortNumber = 83;
                mapPtr->mappingType = CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E;
                mapPtr->interfaceNum = 63;
            }
        }
    }

    for(dev = SYSTEM_DEV_NUM_MAC(0); dev < SYSTEM_DEV_NUM_MAC(ppCounter); dev++)
    {
        /* perform board type recognition */
        rc = bobcat2BoardTypeGet(dev,boardRevId,&bc2BoardType);
        CPSS_ENABLER_DBG_TRACE_RC_MAC("bobcat2BoardTypeGet", rc);
        if (GT_OK != rc)
        {
            return rc;
        }
        bc2BoardRevId = boardRevId;

        /* print board type */
        bobcat2BoardTypePrint(dev,bc2BoardType);

        rc = cpssDxChPortPhysicalPortMapSet(dev, mapArrLen, mapArrPtr);
        CPSS_ENABLER_DBG_TRACE_RC_MAC("cpssDxChPortPhysicalPortMapSet", rc);
        if (GT_OK != rc)
        {
            return rc;
        }

        rc = appDemoDxChMaxMappedPortSet(dev, mapArrLen, mapArrPtr);
        CPSS_ENABLER_DBG_TRACE_RC_MAC("appDemoDxChMaxMappedPortSet", rc);
        if (GT_OK != rc)
        {
            return rc;
        }
 
        /* restore OOB port configuration after systemReset */
        if ((bc2BoardResetDone == GT_TRUE) && appDemoBc2IsInternalCpuEnabled(dev) &&  (!isBobkBoard))
        {
            /* configure SERDES TX interface */
            rc = mvHwsSerdesTxIfSelect(dev, 0, BC2_BOARD_MSYS_OOB_PORT_SERDES_CNS, COM_PHY_28NM, 1);
            CPSS_ENABLER_DBG_TRACE_RC_MAC("mvHwsSerdesTxIfSelect", rc);
            if (GT_OK != rc)
            {
                return rc;
            }

            /* power UP and configure SERDES */
            rc = mvHwsSerdesPowerCtrl(dev, 0, BC2_BOARD_MSYS_OOB_PORT_SERDES_CNS, COM_PHY_28NM, GT_TRUE,
                                      _1_25G, _156dot25Mhz, PRIMARY, XAUI_MEDIA, _10BIT_ON);
            CPSS_ENABLER_DBG_TRACE_RC_MAC("mvHwsSerdesPowerCtrl", rc);
            if (GT_OK != rc)
            {
                return rc;
            }
        }
    }

    rc = waInit(boardRevId);
    CPSS_ENABLER_DBG_TRACE_RC_MAC("waInit", rc);

    bc2BoardResetDone = GT_FALSE;

    return GT_OK;
}
static GT_U32   portInitlist_AllPorts_used = 0;
/*******************************************************************************
* configBoardAfterPhase1
*
* DESCRIPTION:
*       This function performs all needed configurations that should be done
*       after phase1.
*
* INPUTS:
*       boardRevId      - The board revision Id.
*       devNum   - physical device number
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK   - on success,
*       GT_FAIL - otherwise.
*
* COMMENTS:
*       This is a simplified version of configBoardAfterPhase1.
*
*******************************************************************************/
static GT_STATUS configBoardAfterPhase1Simple
(
    IN  GT_U8   boardRevId,
    IN  GT_U8   devNum

)
{
    GT_U32  waIndex=0;
    GT_STATUS rc;
    GT_U32                   mapArrLen;
    CPSS_DXCH_PORT_MAP_STC  *mapArrPtr, *mapPtr;
    GT_U32    i;
    CPSS_PP_DEVICE_TYPE devType;
    CPSS_DXCH_CFG_DEV_INFO_STC   devInfo;

    rc = configBoardAfterPhase1MappingGet(boardRevId,/*OUT*/&mapArrPtr,&mapArrLen);
    if(rc != GT_OK)
        return rc;

    if (mapArrPtr == NULL)
    {
        cpssOsPrintf("\n-->ERROR : configBoardAfterPhase1() mapping : rev =%d is not supported", boardRevId);
        return GT_NOT_SUPPORTED;
    }

    rc = cpssDxChCfgDevInfoGet(devNum, &devInfo);
    CPSS_ENABLER_DBG_TRACE_RC_MAC("cpssDxChCfgDevInfoGet", rc);
    if(rc != GT_OK)
        return rc;

    portInitlist_AllPorts_used = 0;
    devType = devInfo.genDevInfo.devType;

    osPrintf("devType is 0x%X\n",devType);
    switch(devType)
    {
        case CPSS_98DX4220_CNS:
            mapArrPtr = &bc2defaultMap_4220[0];
            mapArrLen = sizeof(bc2defaultMap_4220)/sizeof(bc2defaultMap_4220[0]);
            break;
        case CPSS_98DX4221_CNS:
        case CPSS_BOBK_CAELUM_DEVICES_CASES_MAC:
            mapArrPtr = &bc2defaultMap_4221[0];
            mapArrLen = sizeof(bc2defaultMap_4221)/sizeof(bc2defaultMap_4221[0]);
            break;
        case CPSS_98DX8216_CNS:
            mapArrPtr = &bc2defaultMap_8216[0];
            mapArrLen = sizeof(bc2defaultMap_8216)/sizeof(bc2defaultMap_8216[0]);
            break;
        case CPSS_98DX8219_CNS:
            mapArrPtr = &bc2defaultMap_8219[0];
            mapArrLen = sizeof(bc2defaultMap_8219)/sizeof(bc2defaultMap_8219[0]);
            break;
        case CPSS_98DX8224_CNS:
            mapArrPtr = &bc2defaultMap_8224[0];
            mapArrLen = sizeof(bc2defaultMap_8224)/sizeof(bc2defaultMap_8224[0]);
            break;
        case CPSS_BOBK_CETUS_DEVICES_CASES_MAC:
            mapArrPtr = &bc2defaultMap_56to59_64to71[0];
            mapArrLen = sizeof(bc2defaultMap_56to59_64to71)/sizeof(bc2defaultMap_56to59_64to71[0]);
            portInitlist_AllPorts_used = 1;
            break;
        case PRV_CPSS_BOBCAT3_ALL_DEVICES:
            mapArrPtr = &bc3defaultMap[0];
            mapArrLen = sizeof(bc3defaultMap)/sizeof(bc3defaultMap[0]);
            break;
        default:
            break;
    }

    /* TM related port mapping adjustments */
    if (appDemoTmScenarioMode==CPSS_TM_2 || appDemoTmScenarioMode==CPSS_TM_3)
    {
        mapArrPtr = &bc2TmEnableE2Map[0];
        mapArrLen = sizeof(bc2TmEnableE2Map)/sizeof(bc2TmEnableE2Map[0]);
    }

    if (appDemoTmScenarioMode==CPSS_TM_8)
    {
        mapArrPtr = &bc2TmEnableE8Map[0];
        mapArrLen = sizeof(bc2TmEnableE8Map)/sizeof(bc2TmEnableE8Map[0]);
    }

    if(noBC2CpuPort)
    {
        for (i = 0, mapPtr = mapArrPtr; i < mapArrLen; i++, mapPtr++)
        {
            if(63 == mapPtr->physicalPortNumber)
            {
                mapPtr->physicalPortNumber = 83;
                mapPtr->mappingType = CPSS_DXCH_PORT_MAPPING_TYPE_ETHERNET_MAC_E;
                mapPtr->interfaceNum = 63;
                break;
            }
        }
    }

    /* there is only one device in this setting */

    /* perform board type recognition - for RD and DB boards and their PHY configuration*/
    rc = bobcat2BoardTypeGet(devNum,boardRevId,&bc2BoardType);
    CPSS_ENABLER_DBG_TRACE_RC_MAC("bobcat2BoardTypeGet", rc);
    if (GT_OK != rc)
    {
        return rc;
    }
    bc2BoardRevId = boardRevId;

    /* print board type */
    bobcat2BoardTypePrint(devNum,bc2BoardType);

    rc = cpssDxChPortPhysicalPortMapSet(devNum, mapArrLen, mapArrPtr);
    CPSS_ENABLER_DBG_TRACE_RC_MAC("cpssDxChPortPhysicalPortMapSet", rc);
    if (GT_OK != rc)
    {
        return rc;
    }

        /* it's for appdemo DB only */
        rc = appDemoDxChMaxMappedPortSet(devNum, mapArrLen, mapArrPtr);
        CPSS_ENABLER_DBG_TRACE_RC_MAC("appDemoDxChMaxMappedPortSet", rc);
        if (GT_OK != rc)
        {
            return rc;
        }

    while(BC2WaList[waIndex] != CPSS_DXCH_IMPLEMENT_WA_LAST_E)
        waIndex++;
    rc = cpssDxChHwPpImplementWaInit(devNum,waIndex,BC2WaList, NULL);
    CPSS_ENABLER_DBG_TRACE_RC_MAC("cpssDxChHwPpImplementWaInit", rc);
    if(rc != GT_OK)
        return rc;

    return GT_OK;
}

/*******************************************************************************
* allocateDmaMemSimple
*
* DESCRIPTION:
*       This function allocates memory for phase2 initialization stage, the
*       allocations include: Rx Descriptors / Buffer, Tx descriptors, Address
*       update descriptors.
*
* INPUTS:
*       devType         - The Pp device type to allocate the memory for.
*       rxDescNum       - Number of Rx descriptors (and buffers) to allocate.
*       rxBufSize       - Size of each Rx Buffer to allocate.
*       rxBufAllign     - Ammount of allignment required on the Rx buffers.
*       txDescNum       - Number of Tx descriptors to allocate.
*       auDescNum       - Number of address update descriptors to allocate.
*       ppPhase2Params  - The device's Phase2 parameters.
*
* OUTPUTS:
*       ppPhase2Params  - The device's Phase2 parameters including the required
*                         allocations.
*
* RETURNS:
*       GT_OK   - on success,
*       GT_FAIL - otherwise.
*
* COMMENTS:
*       This is a simplified version of appDemoAllocateDmaMem
*
*******************************************************************************/
GT_STATUS allocateDmaMemSimple
(
    IN      CPSS_PP_DEVICE_TYPE         devType,
    IN      GT_U32                      rxDescNum,
    IN      GT_U32                      rxBufSize,
    IN      GT_U32                      rxBufAllign,
    IN      GT_U32                      txDescNum,
    IN      GT_U32                      auDescNum,
    INOUT   CPSS_DXCH_PP_PHASE2_INIT_INFO_STC  *ppPhase2Params
)
{
    GT_U32                      tmpData;
    GT_U32                      *tmpPtr;
    GT_U32                      fuDescNum; /* number of FU descriptors ... allow to be diff then auDescNum */
    GT_BOOL                     txGenQueue[8];      /* Enable Tx queue to work in generator mode */
    GT_U32                      txGenNumOfDescBuff; /* Number of descriptors and buffers per Tx queue */
    GT_U32                      txGenBuffSize;      /* Size of buffer per Tx queue */
    GT_U32                      txGenDescSize;      /* Size of descriptor per Tx queue */
    GT_U32                      txQue;              /* Tx queue number */
    GT_STATUS                   rc = GT_OK;

    fuDescNum = auDescNum;

    for(txQue = 0; txQue < 8; txQue++)
    {
        txGenQueue[txQue] = GT_FALSE;
    }

    txGenNumOfDescBuff = 512;
    txGenBuffSize = 128 + 16; /* 16 bytes of eDsa tag */

    /* Au block size calc & malloc  */
    rc = cpssDxChHwAuDescSizeGet(devType, &tmpData);
    CPSS_ENABLER_DBG_TRACE_RC_MAC("cpssDxChHwAuDescSizeGet",rc);
    if (GT_OK != rc)
        return rc;

    if (auDescNum == 0)
    {
        ppPhase2Params->auqCfg.auDescBlock = 0;
        ppPhase2Params->auqCfg.auDescBlockSize = 0;
    }
    else
    {
        ppPhase2Params->auqCfg.auDescBlockSize = tmpData * auDescNum;
        ppPhase2Params->auqCfg.auDescBlock =
            osCacheDmaMalloc(ppPhase2Params->auqCfg.auDescBlockSize +
                            tmpData);/*allocate space for one message more for alignment purposes
                                      NOTE: we not add it to the size , only to the buffer ! */
        if(ppPhase2Params->auqCfg.auDescBlock == NULL)
        {
            CPSS_ENABLER_DBG_TRACE_RC_MAC("allocateDmaMemSimple",rc);
            return GT_OUT_OF_CPU_MEM;
        }
        if(((GT_UINTPTR)ppPhase2Params->auqCfg.auDescBlock) % tmpData)
        {
            /* add to the size the extra value for alignment , to the actual size
               will be as needed , after the reduction in the cpss code */
            ppPhase2Params->auqCfg.auDescBlockSize += tmpData;
        }
    }

    if (fuDescNum == 0)
    {
        ppPhase2Params->fuqCfg.auDescBlock = 0;
        ppPhase2Params->fuqCfg.auDescBlockSize = 0;
    }
    else
    {
        /* Fu block size calc & malloc  */
        rc = cpssDxChHwAuDescSizeGet(devType,&tmpData);
        CPSS_ENABLER_DBG_TRACE_RC_MAC("cpssDxChHwAuDescSizeGet",rc);
        if (GT_OK != rc)
            return rc;

        ppPhase2Params->fuqCfg.auDescBlockSize = tmpData * fuDescNum;
        ppPhase2Params->fuqCfg.auDescBlock =
            osCacheDmaMalloc(ppPhase2Params->fuqCfg.auDescBlockSize +
                            tmpData);/*allocate space for one message more for alignment purposes
                                      NOTE: we not add it to the size , only to the buffer ! */
        if(ppPhase2Params->fuqCfg.auDescBlock == NULL)
        {
            CPSS_ENABLER_DBG_TRACE_RC_MAC("allocateDmaMemSimple",GT_OUT_OF_CPU_MEM);
            return GT_OUT_OF_CPU_MEM;
        }

        if(((GT_UINTPTR)ppPhase2Params->fuqCfg.auDescBlock) % tmpData)
        {
            /* add to the size the extra value for alignment , to the actual size
               will be as needed , after the reduction in the cpss code */
            ppPhase2Params->fuqCfg.auDescBlockSize += tmpData;
        }
    }

    /* Tx block size calc & malloc  */
    rc = cpssDxChHwTxDescSizeGet(devType,&tmpData);
    CPSS_ENABLER_DBG_TRACE_RC_MAC("cpssDxChHwTxDescSizeGet",rc);
    if (GT_OK != rc)
        return rc;

    ppPhase2Params->netIfCfg.txDescBlockSize = tmpData * txDescNum;
    ppPhase2Params->netIfCfg.txDescBlock =
        osCacheDmaMalloc(ppPhase2Params->netIfCfg.txDescBlockSize);
    if(ppPhase2Params->netIfCfg.txDescBlock == NULL)
    {
        CPSS_ENABLER_DBG_TRACE_RC_MAC("allocateDmaMemSimple",GT_OUT_OF_CPU_MEM);
        osCacheDmaFree(ppPhase2Params->auqCfg.auDescBlock);
        return GT_OUT_OF_CPU_MEM;
    }

    /* Rx block size calc & malloc  */
    rc = cpssDxChHwRxDescSizeGet(devType,&tmpData);
    CPSS_ENABLER_DBG_TRACE_RC_MAC("cpssDxChHwRxDescSizeGet",rc);
    if (GT_OK != rc)
        return rc;

    ppPhase2Params->netIfCfg.rxDescBlockSize = tmpData * rxDescNum;
    ppPhase2Params->netIfCfg.rxDescBlock =
        osCacheDmaMalloc(ppPhase2Params->netIfCfg.rxDescBlockSize);
    if(ppPhase2Params->netIfCfg.rxDescBlock == NULL)
    {
        osCacheDmaFree(ppPhase2Params->auqCfg.auDescBlock);
        osCacheDmaFree(ppPhase2Params->netIfCfg.txDescBlock);
        CPSS_ENABLER_DBG_TRACE_RC_MAC("allocateDmaMemSimple",GT_OUT_OF_CPU_MEM);
        return GT_OUT_OF_CPU_MEM;
    }

    /* init the Rx buffer allocation method */
    /* Set the system's Rx buffer size.     */
    if((rxBufSize % rxBufAllign) != 0)
    {
        rxBufSize = (rxBufSize + (rxBufAllign - (rxBufSize % rxBufAllign)));
    }

    if (ppPhase2Params->netIfCfg.rxBufInfo.allocMethod == CPSS_RX_BUFF_STATIC_ALLOC_E)
    {
        ppPhase2Params->netIfCfg.rxBufInfo.buffData.staticAlloc.rxBufBlockSize =
        rxBufSize * rxDescNum;

        /* status of RX buffers - cacheable or not, has been set in getPpPhase2ConfigSimple */
        /* If RX buffers should be cachable - allocate it from regular memory */
        if (GT_TRUE == ppPhase2Params->netIfCfg.rxBufInfo.buffersInCachedMem)
        {
            tmpPtr = osMalloc(((rxBufSize * rxDescNum) + rxBufAllign));
        }
        else
        {
            tmpPtr = osCacheDmaMalloc(((rxBufSize * rxDescNum) + rxBufAllign));
        }

        if(tmpPtr == NULL)
        {
            osCacheDmaFree(ppPhase2Params->auqCfg.auDescBlock);
            osCacheDmaFree(ppPhase2Params->netIfCfg.txDescBlock);
            osCacheDmaFree(ppPhase2Params->netIfCfg.rxDescBlock);
            CPSS_ENABLER_DBG_TRACE_RC_MAC("allocateDmaMemSimple",GT_OUT_OF_CPU_MEM);
            return GT_OUT_OF_CPU_MEM;
        }

        if((((GT_UINTPTR)tmpPtr) % rxBufAllign) != 0)
        {
            tmpPtr = (GT_U32*)(((GT_UINTPTR)tmpPtr) +
                               (rxBufAllign - (((GT_UINTPTR)tmpPtr) % rxBufAllign)));
        }
        ppPhase2Params->netIfCfg.rxBufInfo.buffData.staticAlloc.rxBufBlockPtr = tmpPtr;
    }
    else if (ppPhase2Params->netIfCfg.rxBufInfo.allocMethod == CPSS_RX_BUFF_NO_ALLOC_E)
    {
        /* do not allocate rx buffers*/
    }
    else
    {
        /* dynamic RX buffer allocation currently is not supported by appDemo*/
        osCacheDmaFree(ppPhase2Params->auqCfg.auDescBlock);
        osCacheDmaFree(ppPhase2Params->netIfCfg.txDescBlock);
        osCacheDmaFree(ppPhase2Params->netIfCfg.rxDescBlock);
        CPSS_ENABLER_DBG_TRACE_RC_MAC("allocateDmaMemSimple",GT_NOT_SUPPORTED);
        return GT_NOT_SUPPORTED;
    }

    if(ppPhase2Params->useMultiNetIfSdma)
    {
        CPSS_MULTI_NET_IF_TX_SDMA_QUEUE_STC  * sdmaQueuesConfigPtr;

        /* Tx block size calc & malloc  */
        rc = cpssDxChHwTxDescSizeGet(devType,&txGenDescSize);
        CPSS_ENABLER_DBG_TRACE_RC_MAC("cpssDxChHwTxDescSizeGet",rc);
        if (GT_OK != rc)
            return rc;


        for(txQue = 0; txQue < 8; txQue++)
        {
            if(txGenQueue[txQue] == GT_FALSE)
            {
                /* Regular mode is not supported yet */
                continue;
            }

            /* Generator mode */
            sdmaQueuesConfigPtr =
                &ppPhase2Params->multiNetIfCfg.txSdmaQueuesConfig[0][txQue];
            sdmaQueuesConfigPtr->buffAndDescAllocMethod = CPSS_TX_BUFF_STATIC_ALLOC_E;
            sdmaQueuesConfigPtr->buffersInCachedMem = GT_FALSE;
            sdmaQueuesConfigPtr->numOfTxDesc = txGenNumOfDescBuff / 2;

            sdmaQueuesConfigPtr->queueMode =
                CPSS_TX_SDMA_QUEUE_MODE_PACKET_GENERATOR_E;
            sdmaQueuesConfigPtr->numOfTxBuff = sdmaQueuesConfigPtr->numOfTxDesc;

            sdmaQueuesConfigPtr->memData.staticAlloc.buffAndDescMemSize =
                (sdmaQueuesConfigPtr->numOfTxDesc + 1) * (txGenDescSize + txGenBuffSize);
            sdmaQueuesConfigPtr->memData.staticAlloc.buffAndDescMemPtr =
                osCacheDmaMalloc(sdmaQueuesConfigPtr->memData.staticAlloc.buffAndDescMemSize);
            sdmaQueuesConfigPtr->buffSize = txGenBuffSize;
        }
    }

    return rc;
}



/*******************************************************************************
* getPpPhase2Config
*
* DESCRIPTION:
*       Returns the configuration parameters for corePpHwPhase2Init().
*
* INPUTS:
*       boardRevId      - The board revision Id.
*       oldDevNum       - The old Pp device number to get the parameters for.
*
* OUTPUTS:
*       ppPhase2Params  - Phase2 parameters struct.
*
* RETURNS:
*       GT_OK   - on success,
*       GT_FAIL - otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static GT_STATUS getPpPhase2Config
(
    IN  GT_U8                       boardRevId,
    IN  GT_U8                       oldDevNum,
    OUT CPSS_PP_PHASE2_INIT_PARAMS  *phase2Params
)
{
    GT_STATUS                   rc;
    CPSS_PP_PHASE2_INIT_PARAMS  localPpPh2Config = CPSS_PP_PHASE2_DEFAULT;
    GT_U32                      auDescNum;
    GT_U32                      fuDescNum;
    GT_U32                      tmpData;

    PRV_CPSS_DXCH_BC2_UNUSED_PARAM_MAC(boardRevId);

    localPpPh2Config.devNum     = oldDevNum;
    localPpPh2Config.deviceId   = appDemoPpConfigList[oldDevNum].deviceId;

    localPpPh2Config.fuqUseSeparate = GT_TRUE;

    /* Single AUQ, since single FDB unit */
    auDescNum = AU_DESC_NUM_DEF;

    fuDescNum = AU_DESC_NUM_DEF;

    if(appDemoDbEntryGet("fuDescNum", &tmpData) == GT_OK)
    {
        /* Someone already stated the number of fuDescNum, so we NOT override it ! */
    }
    else
    {
        /* Add the fuDescNum to the DB, to allow appDemoAllocateDmaMem(...) to use it ! */
        rc = appDemoDbEntryAdd("fuDescNum", fuDescNum);
        if(rc != GT_OK)
        {
            return rc;
        }
    }

    /* Check if to use the default configuration for appDemo of */
    /* Tx SDMA Packet Generator queues.  */
    if( appDemoDbEntryGet("skipTxSdmaGenDefaultCfg", &tmpData) == GT_NO_SUCH )
    {
        /* allow to work in Tx queue generator mode */
        rc = appDemoDbEntryAdd("useMultiNetIfSdma", GT_TRUE);
        if(rc != GT_OK)
        {
            return rc;
        }

        /* Enable Tx queue 3 to work in Tx queue generator mode */
        rc = appDemoDbEntryAdd("txGenQueue_3", GT_TRUE);
        if(rc != GT_OK)
        {
            return rc;
        }

        /* Enable Tx queue 6 to work in Tx queue generator mode */
        rc = appDemoDbEntryAdd("txGenQueue_6", GT_TRUE);
        if(rc != GT_OK)
        {
            return rc;
        }
    }

    rc = appDemoAllocateDmaMem(localPpPh2Config.deviceId, RX_DESC_NUM_DEF,
                               RX_BUFF_SIZE_DEF, RX_BUFF_ALLIGN_DEF,
                               TX_DESC_NUM_DEF,
                               auDescNum, &localPpPh2Config);

    osMemCpy(phase2Params,&localPpPh2Config, sizeof(CPSS_PP_PHASE2_INIT_PARAMS));

    phase2Params->auMessageLength = CPSS_AU_MESSAGE_LENGTH_8_WORDS_E;

    return rc;
}

/*******************************************************************************
* getPpPhase2ConfigSimple
*
* DESCRIPTION:
*       Returns the configuration parameters for corePpHwPhase2Init().
*
* INPUTS:
*       boardRevId      - The board revision Id.
*       oldDevNum       - The old Pp device number to get the parameters for.
*       devType         - The Pp device type
*
* OUTPUTS:
*       ppPhase2Params  - Phase2 parameters struct.
*
* RETURNS:
*       GT_OK   - on success,
*       GT_FAIL - otherwise.
*
* COMMENTS:
*       This is a simplified version of getPpPhase2Config.
*
*******************************************************************************/
static GT_STATUS getPpPhase2ConfigSimple
(
    IN  GT_U8                       boardRevId,
    IN  GT_U8                       oldDevNum,
    IN  CPSS_PP_DEVICE_TYPE         devType,
    OUT CPSS_DXCH_PP_PHASE2_INIT_INFO_STC  *phase2Params
)
{
    GT_STATUS                   rc;
    GT_U32                      auDescNum;
    GT_U32                      i;

    PRV_CPSS_DXCH_BC2_UNUSED_PARAM_MAC(boardRevId);
    osMemSet(phase2Params,0,sizeof(CPSS_DXCH_PP_PHASE2_INIT_INFO_STC));
    phase2Params->newDevNum                  = oldDevNum;


    phase2Params->fuqUseSeparate = GT_TRUE;
    phase2Params->netIfCfg.rxBufInfo.allocMethod = CPSS_RX_BUFF_STATIC_ALLOC_E;
    for(i=0;i<4;i++)
    {
        phase2Params->netIfCfg.rxBufInfo.bufferPercentage[i] = 13;
        phase2Params->netIfCfg.rxBufInfo.bufferPercentage[i+4] = 12;
    }
    phase2Params->netIfCfg.rxBufInfo.rxBufSize = RX_BUFF_SIZE_DEF;
    phase2Params->netIfCfg.rxBufInfo.headerOffset = 0;
    phase2Params->netIfCfg.rxBufInfo.buffersInCachedMem = GT_FALSE;
    phase2Params->netIfCfg.rxBufInfo.buffData.staticAlloc.rxBufBlockPtr = NULL;
    phase2Params->netIfCfg.rxBufInfo.buffData.staticAlloc.rxBufBlockSize = 16000;

    phase2Params->useSecondaryAuq = GT_FALSE;
    phase2Params->noTraffic2CPU = GT_FALSE;
    phase2Params->netifSdmaPortGroupId = 0;
    phase2Params->auMessageLength = CPSS_AU_MESSAGE_LENGTH_8_WORDS_E;
    phase2Params->useDoubleAuq = GT_FALSE;
    phase2Params->useMultiNetIfSdma = GT_FALSE;

    /* Single AUQ, since single FDB unit */
    auDescNum = AU_DESC_NUM_DEF;

    rc = allocateDmaMemSimple(devType, RX_DESC_NUM_DEF,
                               RX_BUFF_SIZE_DEF, RX_BUFF_ALLIGN_DEF,
                               TX_DESC_NUM_DEF,
                               auDescNum, phase2Params);

    CPSS_ENABLER_DBG_TRACE_RC_MAC("getPpPhase2ConfigSimple", rc);
    return rc;
}


/*******************************************************************************
* internalXGPortConfig
*
* DESCRIPTION:
*       Internal function performs all needed configurations of 10G ports.
*
* INPUTS:
*       devNum      - Device Id.
*       portNum     - Port Number.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK   - on success,
*       GT_FAIL - otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static GT_STATUS internalXGPortConfig
(
    IN  GT_U8     devNum,
    IN  GT_PHYSICAL_PORT_NUM  port_num
)
{

    PRV_CPSS_DXCH_BC2_UNUSED_PARAM_MAC(devNum);
    PRV_CPSS_DXCH_BC2_UNUSED_PARAM_MAC(port_num);

    return GT_OK;
}

/*******************************************************************************
* cascadePortConfig
*
* DESCRIPTION:
*       Additional configuration for cascading ports
*
* INPUTS:
*       devNum  - Device number.
*       portNum - Cascade port number.
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK       - on success,
*       GT_HW_ERROR - HW failure
*       GT_FAIL     - otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static GT_STATUS cascadePortConfig
(
    IN GT_U8                    devNum,
    IN GT_PHYSICAL_PORT_NUM     portNum
)
{
    GT_STATUS       rc;

    /* set the MRU of the cascade port to be big enough for DSA tag */
    rc = cpssDxChPortMruSet(devNum, portNum, 1536);
    CPSS_ENABLER_DBG_TRACE_RC_MAC("cpssDxChPortMruSet", rc);
    return rc;
}

/*******************************************************************************
* bobcat2LedInit
*
* DESCRIPTION:
*       LED init.
*
* INPUTS:
*       devNum - device number
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK                    - on success
*       GT_HW_ERROR              - on hardware error
*       GT_BAD_PARAM             - one of the parameters value is wrong
*       GT_OUT_OF_RANGE          - position out of range
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*       GT_FAIL                  - otherwise
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static GT_STATUS bobcat2LedInit
(
    IN GT_U8 devNum
)
{
    GT_PHYSICAL_PORT_NUM        portNum;  /* physical port number */
    GT_U32                      position; /* LED stream position  */
    CPSS_DXCH_PORT_MAP_STC      portMap;  /* port map             */
    GT_STATUS                   rc;       /* return code          */

    for(portNum = 0; portNum < (appDemoPpConfigList[devNum].maxPortNumber); portNum++)
    {
        CPSS_ENABLER_PORT_SKIP_CHECK(devNum, portNum);

        /* skip ports mapped to MACs 60..71 because they use same LED interface as MACs 36..47 */
        rc = cpssDxChPortPhysicalPortMapGet(devNum, portNum, 1, &portMap);
        if(rc != GT_OK)
        {
            continue;
        }

        if (portMap.interfaceNum >= 60)
        {
            continue;
        }

        /* For every physical port configure 'LEDs number'(in range 0..11) */
        position = portNum % 12;
        rc = cpssDxChLedStreamPortPositionSet(devNum, portNum, position);
        if(rc != GT_OK)
        {
            return rc;
        }
    }

    return GT_OK;
}



/*------------------------------------------------------------------------------------------------------------------------------*
 * 1.  LEDs Interfaces Mode Select = 0 (not BWC)  (DFX)                                                                         *
 *                                                                                                                              *
 *    /Cider/EBU/BobK/BobK {Current}/Reset And Init Controller/DFX Units/Units/DFX Server Registers/Device General Control 19   *
 *     field 0 - 0  Selects the operation mode of the LED interfaces (defines the ports assigned to each LED interface).        *
 *                  Bobcat2 Backward compatible LED mode                                                                        *
 *                  1 - BWC                                                                                                     *
 *                  0 - not BWC                                                                                                 *
 *                                                                                                                              *
 *------------------------------------------------------------------------------------------------------------------------------*/
#define PRV_CPSS_DXCH_BC2_LED_BACKWARD_COMPATIBILITY_OFFS_CNS 0
#define PRV_CPSS_DXCH_BC2_LED_BACKWARD_COMPATIBILITY_LEN_CNS  1


static GT_STATUS bobkLedStreamBC2BackwardCompatibilityModeSet
(
    IN  GT_U8       devNum,
    GT_BOOL         isBc2BackwardCompatible
)
{
    GT_STATUS rc;
    GT_U32 regAddr;
    GT_U32 data;


    data = (isBc2BackwardCompatible == GT_TRUE);

    regAddr = PRV_CPSS_DXCH_DEV_RESET_AND_INIT_CONTROLLER_REGS_MAC(devNum)->DFXServerUnitsBC2SpecificRegs.deviceCtrl19;

    rc = prvCpssDrvHwPpResetAndInitControllerSetRegField(devNum,regAddr,
                                                     PRV_CPSS_DXCH_BC2_LED_BACKWARD_COMPATIBILITY_OFFS_CNS,
                                                     PRV_CPSS_DXCH_BC2_LED_BACKWARD_COMPATIBILITY_LEN_CNS,
                                                     data);
    if (rc != GT_OK)
    {
        return rc;
    }
    return GT_OK;
}

/*-------------------------------------------------------------------------------
 *  BobK Cetus
 *-------------------------------------------------------------------------------
 *    per port configuration 
 *    cpssDxChLedStreamPortPositionSet covers
 *    /Cider/EBU/BobK/BobK {Current}/Switching Core/GOP/Units/GOP/<XLG MAC IP> XLG MAC IP Units%p/XG MIB Counters Control
 *    bit 5-10   Enables selection of the number of this port in the LEDs chain.
 *
 *    0x100C0030 + 0x200000 + 0x1000*(p-56): where p (56-59) represents XLG Port
 *    0x100C0030 + 0x200000 + 0x1000*(p-56): where p (64-71) represents XLG Port
 *
 *
 *                                                        Port    position 
 *    Write to address 0x0102C0030 the value 0x0C     --    56         0
 *    Write to address 0x0102C1030 the value 0x6C     --    57         3
 *    Write to address 0x0102C2030 the value 0x2C     --    58         1
 *    Write to address 0x0102C3030 the value 0x4C     --    59         2
 *                                                                 
 *    Write to address 0x0102C8030 the value 0x10C    --    64         8
 *    Write to address 0x0102C9030 the value 0x16C    --    65        11
 *    Write to address 0x0102CA030 the value 0x12C    --    66         9
 *    Write to address 0x0102CB030 the value 0x14C    --    67        10
 *                                                                 
 *    Write to address 0x0102CC030 the value 0x8C     --    68         4
 *    Write to address 0x0102CD030 the value 0x0EC    --    69         7
 *    Write to address 0x0102CE030 the value 0x0AC    --    70         5
 *    Write to address 0x0102CF030 the value 0x0CC    --    71         6
 *-----------------------------------------------------------------------------------*/

typedef struct 
{
    GT_U32 portMac;
    GT_U32 ledPosition;
}APPDEMO_MAC_LEDPOSITION_STC;

#define APPDEMO_BAD_VALUE (GT_U32)(~0)

APPDEMO_MAC_LEDPOSITION_STC mac_ledPos_Arr[] = 
{
     {      56           ,    0                }
    ,{      57           ,    3                }
    ,{      58           ,    1                }
    ,{      59           ,    2                }
    ,{      64           ,    8                }
    ,{      65           ,   11                }
    ,{      66           ,    9                }
    ,{      67           ,   10                }
    ,{      68           ,    4                }
    ,{      69           ,    7                }
    ,{      70           ,    5                }
    ,{      71           ,    6                }
    ,{APPDEMO_BAD_VALUE  ,   APPDEMO_BAD_VALUE }
};

GT_U32 findLedPositionByMac(GT_U32 mac)
{
    GT_U32 i;
    for (i = 0 ; mac_ledPos_Arr[i].portMac !=  APPDEMO_BAD_VALUE; i++)
    {
        if (mac_ledPos_Arr[i].portMac == mac)
        {
            return mac_ledPos_Arr[i].ledPosition;
        }
    }
    return APPDEMO_BAD_VALUE;
}

static GT_STATUS bobkCetusLedInit
(
    GT_U8 devNum
)
{
    GT_STATUS                       rc;
    GT_U32                          ledInterfaceNum;
    GT_U32                          classNum;
    GT_U32                          group;
    GT_PHYSICAL_PORT_NUM            portNum;
    CPSS_LED_CONF_STC               ledConfig;
    CPSS_LED_CLASS_MANIPULATION_STC ledClassManip;
    CPSS_LED_GROUP_CONF_STC         groupConf;
    GT_U32                          position;
    CPSS_DXCH_PORT_MAP_STC          portMap;
    GT_U32                          inactiveClassList[7];
    GT_U32                          i;
    /*-------------------------------------------*
     *  set no BC2 BW compatibility              *
     *-------------------------------------------*/
    rc = bobkLedStreamBC2BackwardCompatibilityModeSet(devNum,GT_FALSE);
    if(rc != GT_OK)
    {
        return rc;
    }

    ledInterfaceNum = 4;

    ledConfig.ledOrganize             = CPSS_LED_ORDER_MODE_BY_PORT_E;
    ledConfig.disableOnLinkDown       = GT_FALSE;
    ledConfig.blink0DutyCycle         = CPSS_LED_BLINK_DUTY_CYCLE_1_E; /* 50% */
    ledConfig.blink0Duration          = CPSS_LED_BLINK_DURATION_2_E;   /* 128 ms */
    ledConfig.blink1DutyCycle         = CPSS_LED_BLINK_DUTY_CYCLE_1_E;  /* 50%    */
    ledConfig.blink1Duration          = CPSS_LED_BLINK_DURATION_1_E;    /* 64 ms */
    ledConfig.pulseStretch            = CPSS_LED_PULSE_STRETCH_1_E;
    ledConfig.ledStart                = 0;
    ledConfig.ledEnd                  = 255;
    ledConfig.clkInvert               = GT_FALSE; /* don't care */
    ledConfig.class5select            = CPSS_LED_CLASS_5_SELECT_FIBER_LINK_UP_E;   /* don't care */
    ledConfig.class13select           = CPSS_LED_CLASS_13_SELECT_COPPER_LINK_UP_E; /* don't care */
    ledConfig.invertEnable            = GT_FALSE;        /*   1  Active High  */
    ledConfig.ledClockFrequency       = CPSS_LED_CLOCK_OUT_FREQUENCY_1000_E; 


    rc = cpssDxChLedStreamConfigSet(devNum,ledInterfaceNum, &ledConfig);
    if(rc != GT_OK)
    {
        return rc;
    }

    ledClassManip.invertEnable            = GT_FALSE; /* not relevant for BC2/BobK */
    ledClassManip.blinkEnable             = GT_FALSE;
    ledClassManip.blinkSelect             = CPSS_LED_BLINK_SELECT_0_E;
    ledClassManip.forceEnable             = GT_FALSE;
    ledClassManip.forceData               = 0;
    ledClassManip.pulseStretchEnable      = GT_FALSE;
    ledClassManip.disableOnLinkDown       = GT_FALSE;

    inactiveClassList[0] = 5;
    inactiveClassList[1] = 4;
    inactiveClassList[2] = 3;
    inactiveClassList[3] = 1;
    inactiveClassList[4] = 0;
    inactiveClassList[5] = APPDEMO_BAD_VALUE;

    for (i = 0 ; inactiveClassList[i] != APPDEMO_BAD_VALUE; i++)
    {
        classNum = inactiveClassList[i];
        rc = cpssDxChLedStreamClassManipulationSet(devNum
                                                        ,ledInterfaceNum
                                                        ,CPSS_DXCH_LED_PORT_TYPE_XG_E /* don't care , not applicable for BobK */
                                                        ,classNum
                                                        ,&ledClassManip);
        if(rc != GT_OK)
        {
            return rc;
        }
    }

    ledClassManip.invertEnable            = GT_FALSE; /* not relevant for BC2/BobK */
    ledClassManip.blinkEnable             = GT_TRUE;
    ledClassManip.blinkSelect             = CPSS_LED_BLINK_SELECT_0_E;
    ledClassManip.forceEnable             = GT_FALSE;
    ledClassManip.forceData               = 0;
    ledClassManip.pulseStretchEnable      = GT_FALSE;
    ledClassManip.disableOnLinkDown       = GT_FALSE;

    classNum = 2;
    rc = cpssDxChLedStreamClassManipulationSet(devNum
                                                    ,ledInterfaceNum
                                                    ,CPSS_DXCH_LED_PORT_TYPE_XG_E /* don't care , not applicable for BobK */
                                                    ,classNum
                                                    ,&ledClassManip);
    if(rc != GT_OK)
    {
        return rc;
    }

    groupConf.classA = 1;
    groupConf.classB = 1;
    groupConf.classC = 1;
    groupConf.classD = 1;

    group = 0;
    rc = cpssDxChLedStreamGroupConfigSet(devNum,ledInterfaceNum,CPSS_DXCH_LED_PORT_TYPE_XG_E, group,&groupConf);
    if(rc != GT_OK)
    {
        return rc;
    }
    group = 1;
    rc = cpssDxChLedStreamGroupConfigSet(devNum,ledInterfaceNum,CPSS_DXCH_LED_PORT_TYPE_XG_E, group,&groupConf);
    if(rc != GT_OK)
    {
        return rc;
    }


    for(portNum = 0; portNum < (appDemoPpConfigList[devNum].maxPortNumber); portNum++)
    {
        CPSS_ENABLER_PORT_SKIP_CHECK(devNum, portNum);

        rc = cpssDxChPortPhysicalPortMapGet(devNum, portNum, 1, &portMap);
        if(rc != GT_OK)
        {
            continue;
        }

        position = findLedPositionByMac(portMap.interfaceNum);
        if (position != APPDEMO_BAD_VALUE)
        {
            rc = cpssDxChLedStreamPortPositionSet(devNum, portNum, position);
            if(rc != GT_OK)
            {
                return rc;
            }
        }
    }

    return GT_OK;
}
/*--------------------------------------------------------------*
 * data base for autopolling                                    *
 *--------------------------------------------------------------*/
typedef struct 
{
    CPSS_PHY_SMI_INTERFACE_ENT smiIf;
    GT_U32                     portNumMapped;
    GT_PHYSICAL_PORT_NUM       portNum[17];
    GT_U32                     smiAddr[17];
}APPDEMO_PORT_SMI_DEV_MAPPING_STC; 


typedef struct 
{
    CPSS_DXCH_PHY_SMI_AUTO_POLL_NUM_OF_PORTS_ENT   sutoPollNumber   [CPSS_PHY_SMI_INTERFACE_MAX_E];
    APPDEMO_PORT_SMI_DEV_MAPPING_STC               port2SmiDevMapArr[CPSS_PHY_SMI_INTERFACE_MAX_E];
    GT_BOOL                                        isInbanAutNegEnable;
}APPDEMO_AUTOPOLL_BOARD_DEF_STC;

typedef struct 
{
    GT_U32 boardType;
    APPDEMO_AUTOPOLL_BOARD_DEF_STC *portSmiDevMapping;
}APPDEMPO_AUTOPOLLING_SMI_CONFIG;


/* BUG at DB board of BC2: MAC 0-11 hard wired to SMI-2 instead of SMI-0, */
/* therefore autonegotiation via SMI will never work           */
/* RD board mac  0-15 wired to SMI 0           */
/*          mac 16-23       to SMI 1           */
/*          mac 24-39       to SMI 2           */
/*          mac 40-47       to SMI 3           */
/*-----------------------------------------------------------------------------------------------
 *           Serdes# MAC Port#   M_SMI#  Phy Address[H]         M_SMI#  Phy Address[H]
 *           ++++ DB Board ++++++++++BC2 +++++++++              ++  CAELUM  +++++++++++
 *               0   0 - 3           2   4 - 7                      0   0 - 3
 *               1   4 - 7           2   8 - B                      0   4 - 7
 *               2   8 - 11          2   C - F                      0   8 - B
 *               3   12 - 15         3   4 - 7                      1   4 - 7
 *               4   16 - 19         3   8 - B                      1   8 - B
 *               5   20 - 23         3   C - F                      1   C - F
 *               6   24 - 27         0   4 - 7                      2   0 - 3
 *               7   28 - 31         0   8 - B                      2   4 - 7
 *               8   32 - 35         0   C - F                      2   8 - B
 *               9   36 - 39         1   4 - 7                      3   4 - 7
 *               10  40 - 43         1   8 - B                      3   8 - B
 *               11  44 - 47         1   C - F                      3   C - F
 *           ++++ RD Board +++++++++++++++++++                  +++++++++++++++++++
 *               0   0 - 3           0   0 - 3
 *               1   4 - 7           0   4 - 7
 *               2   8 - 11          0   8 - B
 *               3   12 - 15         0   C - F
 *               4   16 - 19         1   0x10 - 0x13
 *               5   20 - 23         1   0x14 - 0x17
 *               6   24 - 27         2   0 - 3
 *               7   28 - 31         2   4 - 7
 *               8   32 - 35         2   8 - B
 *               9   36 - 39         2   C - F
 *               10  40 - 43         3   0x10 - 0x13
 *               11  44 - 47         3   0x14 - 0x17
 *
 *
 *---------------------------------------------------------------------------------------------------*/


APPDEMO_AUTOPOLL_BOARD_DEF_STC board_bc2_DB = 
{
     { 
         CPSS_DXCH_PHY_SMI_AUTO_POLL_NUM_OF_PORTS_12_E, 
         CPSS_DXCH_PHY_SMI_AUTO_POLL_NUM_OF_PORTS_12_E, 
         CPSS_DXCH_PHY_SMI_AUTO_POLL_NUM_OF_PORTS_12_E, 
         CPSS_DXCH_PHY_SMI_AUTO_POLL_NUM_OF_PORTS_12_E
     }
    ,{
        { 
             CPSS_PHY_SMI_INTERFACE_0_E
            ,12
            ,{  24,  25,  26,  27,  28, 29,   30,  31,  32,  33,  34,  35, APPDEMO_BAD_VALUE }
            ,{ 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF, APPDEMO_BAD_VALUE }
        }
       ,{ 
             CPSS_PHY_SMI_INTERFACE_1_E
            ,12
            ,{  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47, APPDEMO_BAD_VALUE }
            ,{ 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF, APPDEMO_BAD_VALUE }
        }
       ,{ 
             CPSS_PHY_SMI_INTERFACE_2_E
            ,12
            ,{   0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11, APPDEMO_BAD_VALUE }
            ,{ 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF, APPDEMO_BAD_VALUE }
        }
       ,{ 
             CPSS_PHY_SMI_INTERFACE_3_E
            ,12
            ,{  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23, APPDEMO_BAD_VALUE }
            ,{ 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF, APPDEMO_BAD_VALUE }
        }
     }
    ,GT_TRUE /* anable inband polling */
};

APPDEMO_AUTOPOLL_BOARD_DEF_STC board_bc2_RD = 
{
     { 
         CPSS_DXCH_PHY_SMI_AUTO_POLL_NUM_OF_PORTS_16_E, 
         CPSS_DXCH_PHY_SMI_AUTO_POLL_NUM_OF_PORTS_8_E, 
         CPSS_DXCH_PHY_SMI_AUTO_POLL_NUM_OF_PORTS_16_E, 
         CPSS_DXCH_PHY_SMI_AUTO_POLL_NUM_OF_PORTS_8_E
     }
    ,{
        { 
             CPSS_PHY_SMI_INTERFACE_0_E
            ,16
            ,{   0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15, APPDEMO_BAD_VALUE }
            ,{ 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF, APPDEMO_BAD_VALUE }
        }
       ,{ 
             CPSS_PHY_SMI_INTERFACE_1_E
            ,8
            ,{   16,   17,   18,   19,   20,   21,   22,   23, APPDEMO_BAD_VALUE }
            ,{ 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, APPDEMO_BAD_VALUE }
        }
       ,{ 
             CPSS_PHY_SMI_INTERFACE_2_E
            ,16
            ,{   24, 25,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39, APPDEMO_BAD_VALUE }
            ,{ 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF, APPDEMO_BAD_VALUE }
        }
       ,{ 
             CPSS_PHY_SMI_INTERFACE_3_E
            ,8
            ,{   40,   41,   42,   43,   44,   45,   46,   47, APPDEMO_BAD_VALUE }
            ,{ 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, APPDEMO_BAD_VALUE }
        }
     }
    ,GT_FALSE /* don't enable inband polling */
};

APPDEMO_AUTOPOLL_BOARD_DEF_STC board_bobk_caelum_DB = 
{
     { 
         CPSS_DXCH_PHY_SMI_AUTO_POLL_NUM_OF_PORTS_12_E, 
         CPSS_DXCH_PHY_SMI_AUTO_POLL_NUM_OF_PORTS_12_E, 
         CPSS_DXCH_PHY_SMI_AUTO_POLL_NUM_OF_PORTS_12_E, 
         CPSS_DXCH_PHY_SMI_AUTO_POLL_NUM_OF_PORTS_12_E
     }
    ,{
        { 
             CPSS_PHY_SMI_INTERFACE_0_E
            ,12
            ,{   0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11, APPDEMO_BAD_VALUE }
            ,{ 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF, APPDEMO_BAD_VALUE }
        }
       ,{ 
             CPSS_PHY_SMI_INTERFACE_1_E
            ,12
            ,{  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23, APPDEMO_BAD_VALUE }
            ,{ 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF, APPDEMO_BAD_VALUE }
        }
       ,{ 
             CPSS_PHY_SMI_INTERFACE_2_E
            ,12
            ,{  24,  25,  26,  27,  28, 29,   30,  31,  32,  33,  34,  35, APPDEMO_BAD_VALUE }
            ,{ 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF, APPDEMO_BAD_VALUE }
        }
       ,{ 
             CPSS_PHY_SMI_INTERFACE_3_E
            ,12
            ,{  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47, APPDEMO_BAD_VALUE }
            ,{ 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF, APPDEMO_BAD_VALUE }
        }
     }
    ,GT_FALSE /* don't enable inband polling */
};

APPDEMPO_AUTOPOLLING_SMI_CONFIG boardAutoPollingConfArr[] = 
{
       { APP_DEMO_BC2_BOARD_DB_CNS,      &board_bc2_DB                             }
      ,{ APP_DEMO_BC2_BOARD_RD_MSI_CNS,  &board_bc2_RD                             }
      ,{ APP_DEMO_BC2_BOARD_RD_MTL_CNS,  &board_bc2_RD                             }
      ,{ APP_DEMO_CAELUM_BOARD_DB_CNS,   &board_bobk_caelum_DB                     }
      ,{ APP_DEMO_CETUS_BOARD_DB_CNS,    (APPDEMO_AUTOPOLL_BOARD_DEF_STC*)NULL     }
      ,{ APPDEMO_BAD_VALUE,              (APPDEMO_AUTOPOLL_BOARD_DEF_STC*)NULL     }
};


GT_STATUS boardAutopollingInfoGet
(
    IN  GT_U32 boardType, 
    OUT GT_BOOL                         *isFoundPtr,
    OUT APPDEMO_AUTOPOLL_BOARD_DEF_STC **autoPollingInfoPtrPtr
)
{
    GT_U32 i;

    for (i = 0 ; boardAutoPollingConfArr[i].boardType !=  APPDEMO_BAD_VALUE; i++)
    {
        if ( boardAutoPollingConfArr[i].boardType == boardType)
        {
            *isFoundPtr = GT_TRUE;
            *autoPollingInfoPtrPtr = boardAutoPollingConfArr[i].portSmiDevMapping;
            return GT_OK;
        }
    }
    *isFoundPtr = GT_FALSE;
    *autoPollingInfoPtrPtr = (APPDEMO_AUTOPOLL_BOARD_DEF_STC *)NULL;
    return GT_OK;;
}

GT_STATUS boardAutoPollingConfigure
(
    IN  GT_U8   dev,
    IN  GT_U32  boardType    
)
{
    GT_STATUS rc;
    GT_PHYSICAL_PORT_NUM  port;   /* port number */
    GT_U8                 phyAddr;/* PHY address */
    APPDEMO_AUTOPOLL_BOARD_DEF_STC * autoPollingInfoPtr;
    GT_BOOL                          isFound; 
    GT_U32                           smiIfIdx;
    CPSS_PHY_SMI_INTERFACE_ENT       smiIf;
    GT_U32                           portIdx;

    rc = boardAutopollingInfoGet(bc2BoardType, /*OUT*/&isFound,&autoPollingInfoPtr);
    if (rc != GT_OK)
    {
        return rc;
    }
    if (isFound == GT_FALSE)
    {
        return GT_NOT_SUPPORTED;
    }
    if (autoPollingInfoPtr != NULL)
    {
        rc = cpssDxChPhyAutoPollNumOfPortsSet(dev,
                                                autoPollingInfoPtr->sutoPollNumber[0],
                                                autoPollingInfoPtr->sutoPollNumber[1],
                                                autoPollingInfoPtr->sutoPollNumber[2],
                                                autoPollingInfoPtr->sutoPollNumber[3]);
        CPSS_ENABLER_DBG_TRACE_RC_MAC("cpssDxChPhyAutoPollNumOfPortsSet", rc);
        if(rc != GT_OK)
        {
            return rc;
        }
        for (smiIfIdx = 0 ; smiIfIdx < CPSS_PHY_SMI_INTERFACE_MAX_E; smiIfIdx++)
        {
            smiIf = autoPollingInfoPtr->port2SmiDevMapArr[smiIfIdx].smiIf;
            for (portIdx = 0 ; autoPollingInfoPtr->port2SmiDevMapArr[smiIfIdx].portNum[portIdx] != APPDEMO_BAD_VALUE; portIdx++)
            {
                port    = autoPollingInfoPtr->port2SmiDevMapArr[smiIfIdx].portNum[portIdx];
                phyAddr = (GT_U8)autoPollingInfoPtr->port2SmiDevMapArr[smiIfIdx].smiAddr[portIdx];
                        
                CPSS_ENABLER_PORT_SKIP_CHECK(dev,port);
                /* configure PHY address */

                /* cpssOsPrintf("\nPort %3d smiIf = %d phyAddr = 0x%x",port,smiIf,phyAddr);  */
                rc = cpssDxChPhyPortAddrSet(dev, port, phyAddr);
                CPSS_ENABLER_DBG_TRACE_RC_MAC("cpssDxChPhyPortAddrSet", rc);
                if(rc != GT_OK)
                {
                    return rc;
                }

                /* configure SMI interface */
                rc = cpssDxChPhyPortSmiInterfaceSet(dev, port, smiIf);
                CPSS_ENABLER_DBG_TRACE_RC_MAC("cpssDxChPhyPortSmiInterfaceSet", rc);
                if(rc != GT_OK)
                {
                    return rc;
                }
                if (autoPollingInfoPtr->isInbanAutNegEnable == GT_TRUE)
                {
                    /* enable in-band auto-negotiation because out of band cannot
                        work on this DB board */
                    rc = cpssDxChPortInbandAutoNegEnableSet(dev, port, GT_TRUE);
                    CPSS_ENABLER_DBG_TRACE_RC_MAC("cpssDxChPortInbandAutoNegEnableSet", rc);
                    if(rc != GT_OK)
                    {
                        return rc;
                    }
                }
            }
        }
    }
    return GT_OK;
}


/*******************************************************************************
* configBoardAfterPhase2
*
* DESCRIPTION:
*       This function performs all needed configurations that should be done
*       after phase2.
*
* INPUTS:
*       boardRevId      - The board revision Id.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK   - on success,
*       GT_FAIL - otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static GT_STATUS configBoardAfterPhase2
(
    IN  GT_U8   boardRevId
)
{
    GT_U8                 dev;
    GT_PHYSICAL_PORT_NUM  port;   /* port number */
    GT_STATUS             rc;
    PRV_CPSS_DXCH_PP_CONFIG_STC *pDev;

    for(dev = SYSTEM_DEV_NUM_MAC(0); dev < SYSTEM_DEV_NUM_MAC(ppCounter); dev++)
    {
        appDemoPpConfigList[dev].internal10GPortConfigFuncPtr = internalXGPortConfig;
        appDemoPpConfigList[dev].internalCscdPortConfigFuncPtr = cascadePortConfig;

        for(port = 0; port < CPSS_MAX_PORTS_NUM_CNS;port++)
        {
            if( !(CPSS_PORTS_BMP_IS_PORT_SET_MAC(&PRV_CPSS_PP_MAC(dev)->existingPorts, port)))
            {
                continue;
            }

            /* split ports between MC FIFOs for Multicast arbiter */
            rc = cpssDxChPortTxMcFifoSet(dev, port, port%2);
            if( GT_OK != rc )
            {
                return rc;
            }
        }

        /* configure QSGMII ports PHY related mappings */
        if (   boardRevId == BOARD_REV_ID_DB_E     || boardRevId == BOARD_REV_ID_DB_TM_E
            || boardRevId == BOARD_REV_ID_RDMTL_E  || boardRevId == BOARD_REV_ID_RDMTL_TM_E
            || boardRevId == BOARD_REV_ID_RDMSI_E  || boardRevId == BOARD_REV_ID_RDMSI_TM_E)
        {
            rc = boardAutoPollingConfigure(dev,bc2BoardType);
            if (rc != GT_OK)
            {
                return rc;
            }
        }
        else
        {
            cpssOsPrintf("\n-->ERROR : configBoardAfterPhase2() PHY config : rev =%d is not supported", boardRevId);
            return GT_NOT_SUPPORTED;
        }

        /* Init LED interfaces */
        pDev = PRV_CPSS_DXCH_PP_MAC(dev);

        switch(pDev->genInfo.devFamily)
        {
            case CPSS_PP_FAMILY_DXCH_BOBCAT2_E:
                switch(pDev->genInfo.devSubFamily)
                {
                    case CPSS_PP_SUB_FAMILY_NONE_E:
                        rc = bobcat2LedInit(dev);                        
                    break;
                    case CPSS_PP_SUB_FAMILY_BOBCAT2_BOBK_E:
                        rc = bobkCetusLedInit(dev);
                    break;
                    default:
                    {
                        return GT_NOT_SUPPORTED;
                    }
                }
            break;
            case CPSS_PP_FAMILY_DXCH_BOBCAT3_E:
                rc = bobcat2LedInit(dev);
            break;
            default:
            {
                return GT_NOT_SUPPORTED;
            }
        }
    }

    return GT_OK;
}

/*******************************************************************************
* configBoardAfterPhase2Simple
*
* DESCRIPTION:
*       This function performs all needed configurations that should be done
*       after phase2.
*
* INPUTS:
*       boardRevId      - The board revision Id.
*       devNum   - physical device number
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK   - on success,\
*       GT_FAIL - otherwise.
*
* COMMENTS:
*       This is a simplified version of configBoardAfterPhase2.
*
*******************************************************************************/

static GT_STATUS configBoardAfterPhase2Simple
(
    IN  GT_U8   boardRevId,
    GT_U8       devNum
)
{
    GT_PHYSICAL_PORT_NUM  port;   /* port number */
    GT_U8                 phyAddr;/* PHY address */
    CPSS_PHY_SMI_INTERFACE_ENT smiIf; /* SMI Master interface for PHY management */
    GT_STATUS             rc;

    for(port = 0; port < CPSS_MAX_PORTS_NUM_CNS;port++)
    {
        if( !(CPSS_PORTS_BMP_IS_PORT_SET_MAC(&PRV_CPSS_PP_MAC(devNum)->existingPorts, port)))
        {
            continue;
        }

        /* split ports between MC FIFOs for Multicast arbiter */
        rc = cpssDxChPortTxMcFifoSet(devNum, port, port%2);
        CPSS_ENABLER_DBG_TRACE_RC_MAC("cpssDxChPortTxMcFifoSet", rc);
        if( GT_OK != rc )
        {
            return rc;
        }
    }

    /* configure QSGMII ports PHY related mappings */
    if (   boardRevId == BOARD_REV_ID_DB_E     || boardRevId == BOARD_REV_ID_DB_TM_E
        || boardRevId == BOARD_REV_ID_RDMTL_E  || boardRevId == BOARD_REV_ID_RDMTL_TM_E
        || boardRevId == BOARD_REV_ID_RDMSI_E  || boardRevId == BOARD_REV_ID_RDMSI_TM_E)
    {
        switch (bc2BoardType)
        {
            case APP_DEMO_BC2_BOARD_DB_CNS:
                /* configure autopolling to work with proper SMI controllers for Octal PHYs */
                rc = cpssDxChPhyAutoPollNumOfPortsSet(devNum,
                                CPSS_DXCH_PHY_SMI_AUTO_POLL_NUM_OF_PORTS_12_E,
                                CPSS_DXCH_PHY_SMI_AUTO_POLL_NUM_OF_PORTS_12_E,
                                CPSS_DXCH_PHY_SMI_AUTO_POLL_NUM_OF_PORTS_12_E,
                                CPSS_DXCH_PHY_SMI_AUTO_POLL_NUM_OF_PORTS_12_E);
                CPSS_ENABLER_DBG_TRACE_RC_MAC("cpssDxChPhyAutoPollNumOfPortsSet", rc);
                if(rc != GT_OK)
                {
                    return rc;
                }
            break;
            case APP_DEMO_BC2_BOARD_RD_MSI_CNS: /* RD MSI board */
            case APP_DEMO_BC2_BOARD_RD_MTL_CNS: /* RD MTL board */
                /* configure autopolling to work with proper SMI controllers for Octal PHYs */
                rc = cpssDxChPhyAutoPollNumOfPortsSet(devNum,
                                CPSS_DXCH_PHY_SMI_AUTO_POLL_NUM_OF_PORTS_16_E,
                                CPSS_DXCH_PHY_SMI_AUTO_POLL_NUM_OF_PORTS_8_E,
                                CPSS_DXCH_PHY_SMI_AUTO_POLL_NUM_OF_PORTS_16_E,
                                CPSS_DXCH_PHY_SMI_AUTO_POLL_NUM_OF_PORTS_8_E);
                CPSS_ENABLER_DBG_TRACE_RC_MAC("cpssDxChPhyAutoPollNumOfPortsSet", rc);
                if(rc != GT_OK)
                {
                    return rc;
                }

                #if 0
                    /*------------------------------------------------------*/
                    /* configuration for PHY 88E1512 connected to MAC 62,63 */
                    /*------------------------------------------------------*/
                    rc = cpssDxChPhyPortAddrSet(dev, 82, phyAddr);
                    CPSS_ENABLER_DBG_TRACE_RC_MAC("cpssDxChPhyPortAddrSet", rc);
                    if(rc != GT_OK)
                    {
                        return rc;
                    }

                    /* configure SMI interface */
                    rc = cpssDxChPhyPortSmiInterfaceSet(dev, 83, CPSS_PHY_SMI_INTERFACE_3_E);
                    CPSS_ENABLER_DBG_TRACE_RC_MAC("cpssDxChPhyPortSmiInterfaceSet", rc);
                    if(rc != GT_OK)
                    {
                        return rc;
                    }
                #endif
            break;
            default:
            {
                return GT_NOT_SUPPORTED; /* unknown board type*/
            }
        }

        /*
        Serdes# MAC Port#   M_SMI#  Phy Address[H]
        ++++ DB Board +++++++++++++++++++
            0   0 - 3           2   4 - 7
            1   4 - 7           2   8 - B
            2   8 - 11          2   C - F
            3   12 - 15         3   4 - 7
            4   16 - 19         3   8 - B
            5   20 - 23         3   C - F
            6   24 - 27         0   4 - 7
            7   28 - 31         0   8 - B
            8   32 - 35         0   C - F
            9   36 - 39         1   4 - 7
            10  40 - 43         1   8 - B
            11  44 - 47         1   C - F
        ++++ RD Board +++++++++++++++++++
            0   0 - 3           0   0 - 3
            1   4 - 7           0   4 - 7
            2   8 - 11          0   8 - B
            3   12 - 15         0   C - F
            4   16 - 19         1   0x10 - 0x13
            5   20 - 23         1   0x14 - 0x17
            6   24 - 27         2   0 - 3
            7   28 - 31         2   4 - 7
            8   32 - 35         2   8 - B
            9   36 - 39         2   C - F
            10  40 - 43         3   0x10 - 0x13
            11  44 - 47         3   0x14 - 0x17
        */
        for(port = 0; port < 48; port++)
        {
            CPSS_ENABLER_PORT_SKIP_CHECK(devNum,port);
            if (bc2BoardType == APP_DEMO_BC2_BOARD_DB_CNS) /* BUG at DB board : MAC 0-11 hard wired to SMI-2 instead of SMI-0, */
            {                                              /* therefore autonegotiation via SMI will never work           */
                /*DB Board */                              /* RD board mac  0-15 wired to SMI 0           */
                phyAddr = (GT_U8)(4 + (port % 12));        /*          mac 16-23       to SMI 1           */
                                                           /*          mac 24-39       to SMI 2           */
                if (port < 12)                             /*          mac 40-47       to SMI 3           */
                {
                    smiIf = CPSS_PHY_SMI_INTERFACE_2_E;
                }
                else if (port < 24)
                {
                    smiIf = CPSS_PHY_SMI_INTERFACE_3_E;
                }

                else if (port < 36)
                {
                    smiIf = CPSS_PHY_SMI_INTERFACE_0_E;
                }
                else
                {
                    smiIf = CPSS_PHY_SMI_INTERFACE_1_E;
                }
            }
            else if (bc2BoardType == APP_DEMO_BC2_BOARD_RD_MSI_CNS || bc2BoardType == APP_DEMO_BC2_BOARD_RD_MTL_CNS )
            {
                /* RD Board */
                phyAddr = (GT_U8)(port % 24);

                if (port < 16)
                {
                    smiIf = CPSS_PHY_SMI_INTERFACE_0_E;
                }
                else if (port < 24)
                {
                    smiIf = CPSS_PHY_SMI_INTERFACE_1_E;
                }
                else if (port < 40)
                {
                    smiIf = CPSS_PHY_SMI_INTERFACE_2_E;
                }
                else
                {
                    smiIf = CPSS_PHY_SMI_INTERFACE_3_E;
                }
            }
            else
            {
                cpssOsPrintf("\n-->ERROR : configBoardAfterPhase2() bc2BoardType %d is not supported",bc2BoardType);
                return GT_NOT_SUPPORTED;
            }

            /* configure PHY address */
            rc = cpssDxChPhyPortAddrSet(devNum, port, phyAddr);
            CPSS_ENABLER_DBG_TRACE_RC_MAC("cpssDxChPhyPortAddrSet", rc);
            if(rc != GT_OK)
            {
                return rc;
            }

            /* configure SMI interface */
            rc = cpssDxChPhyPortSmiInterfaceSet(devNum, port, smiIf);
            CPSS_ENABLER_DBG_TRACE_RC_MAC("cpssDxChPhyPortSmiInterfaceSet", rc);
            if(rc != GT_OK)
            {
                return rc;
            }

            if (bc2BoardType == APP_DEMO_BC2_BOARD_DB_CNS)
            {
                /* enable in-band auto-negotiation because out of band cannot
                   work on this DB board */
                rc = cpssDxChPortInbandAutoNegEnableSet(devNum, port, GT_TRUE);
                CPSS_ENABLER_DBG_TRACE_RC_MAC("cpssDxChPortInbandAutoNegEnableSet", rc);
                if(rc != GT_OK)
                {
                    return rc;
                }
            }
        }
    }
    else
    {
        cpssOsPrintf("\n-->ERROR : configBoardAfterPhase2() PHY config : rev =%d is not supported", boardRevId);
        return GT_NOT_SUPPORTED;
    }

    return GT_OK;
}


/*******************************************************************************
* appDemoDbBlocksAllocationMethodGet
*
* DESCRIPTION:
*       Get the blocks allocation method configured in the Init System
*
* INPUTS:
*
* OUTPUTS:
*
*       GT_OK           - Operation OK
*       GT_BAD_PARAM    - Internal error
*       GT_BAD_PTR      - Null pointer
*
* COMMENTS:
*       None.
*******************************************************************************/
static GT_STATUS appDemoDbBlocksAllocationMethodGet
(
    IN  GT_U8                                               dev,
    OUT APP_DEMO_CPSS_LPM_RAM_BLOCKS_ALLOCATION_METHOD_ENT  *blocksAllocationMethodGet
)
{
    GT_U32 value;

    if(appDemoDbEntryGet("lpmRamMemoryBlocksCfg.lpmRamBlocksAllocationMethod", &value) == GT_OK)
        *blocksAllocationMethodGet = (APP_DEMO_CPSS_LPM_RAM_BLOCKS_ALLOCATION_METHOD_ENT)value;
    else
        *blocksAllocationMethodGet = APP_DEMO_CPSS_LPM_RAM_BLOCKS_ALLOCATION_METHOD_DYNAMIC_WITHOUT_BLOCK_SHARING_E;

    return GT_OK;
}

/*******************************************************************************
* appDemoBc2IpLpmRamDefaultConfigCalc
*
* DESCRIPTION:
*       This function calculate the default RAM LPM DB configuration for LPM management.
*
* INPUTS:
*       devNum             - The Pp device number to get the parameters for.
*       maxNumOfPbrEntries - number of PBR entries to deduct from the LPM memory calculations
*
* OUTPUTS:
*       ramDbCfgPtr        - (pointer to) ramDbCfg structure to hold the defaults calculated
*
* RETURNS:
*       GT_OK   - on success,
*       GT_FAIL - otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
GT_STATUS appDemoBc2IpLpmRamDefaultConfigCalc
(
    IN  GT_U8                                    devNum,
    IN  GT_U32                                   maxNumOfPbrEntries,
    OUT APP_DEMO_CPSS_LPM_RAM_MEM_BLOCKS_CFG_STC *ramDbCfgPtr
)
{
    GT_U32 i=0;
    GT_STATUS rc = GT_OK;
    GT_U32 blockSizeInBytes;
    GT_U32 blockSizeInLines;
    GT_U32 lastBlockSizeInLines;
    GT_U32 lastBlockSizeInBytes;
    GT_U32 lpmRamNumOfLines;
    GT_U32 numOfPbrBlocks;
    GT_U32 maxNumOfPbrEntriesToUse;
    GT_U32 value;

    if(appDemoDbEntryGet("maxNumOfPbrEntries", &value) == GT_OK)
        maxNumOfPbrEntriesToUse = value;
    else
        maxNumOfPbrEntriesToUse = maxNumOfPbrEntries;

    lpmRamNumOfLines = PRV_CPSS_DXCH_LPM_RAM_GET_NUM_OF_LINES_MAC(PRV_CPSS_DXCH_PP_MAC(devNum)->fineTuning.tableSize.lpmRam);
    blockSizeInLines = (lpmRamNumOfLines/APP_DEMO_CPSS_MAX_NUM_OF_LPM_BLOCKS_CNS);
    if (blockSizeInLines==0)
    {
        /* can not create a shadow with the current lpmRam size */
        return GT_FAIL;
    }
    blockSizeInBytes = blockSizeInLines * 4;

    if (maxNumOfPbrEntriesToUse >= lpmRamNumOfLines)
    {
        /* No memory for Ip LPM */
        return GT_FAIL;
    }

    if(maxNumOfPbrEntriesToUse > blockSizeInLines)
    {
        numOfPbrBlocks = (maxNumOfPbrEntriesToUse + blockSizeInLines - 1) / blockSizeInLines;
        lastBlockSizeInLines = (numOfPbrBlocks*blockSizeInLines)-maxNumOfPbrEntriesToUse;
        if (lastBlockSizeInLines==0)/* PBR will fit exactly in numOfPbrBlocks */
        {
            ramDbCfgPtr->numOfBlocks = APP_DEMO_CPSS_MAX_NUM_OF_LPM_BLOCKS_CNS - numOfPbrBlocks;
            lastBlockSizeInLines = blockSizeInLines; /* all of last block for IP LPM */
        }
        else/* PBR will not fit exactly in numOfPbrBlocks and we will have in the last block LPM lines together with PBR lines*/
        {
            ramDbCfgPtr->numOfBlocks = APP_DEMO_CPSS_MAX_NUM_OF_LPM_BLOCKS_CNS - numOfPbrBlocks + 1;
        }
    }
    else
    {
        if (maxNumOfPbrEntriesToUse == blockSizeInLines)
        {
            ramDbCfgPtr->numOfBlocks = APP_DEMO_CPSS_MAX_NUM_OF_LPM_BLOCKS_CNS-1;
            lastBlockSizeInLines = blockSizeInLines;
        }
        else
        {
            ramDbCfgPtr->numOfBlocks = APP_DEMO_CPSS_MAX_NUM_OF_LPM_BLOCKS_CNS;
            lastBlockSizeInLines = blockSizeInLines - maxNumOfPbrEntriesToUse;
        }
    }

    /* number of LPM bytes ONLY when last block is shared between LPM and PBR */
    lastBlockSizeInBytes = lastBlockSizeInLines * 4;

    for (i=0;i<ramDbCfgPtr->numOfBlocks-1;i++)
    {
        ramDbCfgPtr->blocksSizeArray[i] = blockSizeInBytes;
    }

    ramDbCfgPtr->blocksSizeArray[ramDbCfgPtr->numOfBlocks-1] =
        lastBlockSizeInBytes == 0 ?
            blockSizeInBytes :   /* last block is fully LPM (not PBR) */
            lastBlockSizeInBytes;/* last block uses 'x' for LPM , rest for PBR */

    /* reset other sections */
    i = ramDbCfgPtr->numOfBlocks;
    for (/*continue i*/;i<APP_DEMO_CPSS_MAX_NUM_OF_LPM_BLOCKS_CNS;i++)
    {
        ramDbCfgPtr->blocksSizeArray[i] = 0;
    }

    rc = appDemoDbBlocksAllocationMethodGet(devNum,&ramDbCfgPtr->blocksAllocationMethod);
    if (rc != GT_OK)
    {
        return rc;
    }
    return GT_OK;
}

/*******************************************************************************
* appDemoBc2IpLpmRamDefaultConfigCalc_elements
*
* DESCRIPTION:
*       This function calculate the default RAM LPM DB configuration for LPM management.
*       and return the elements in the struct
*
* INPUTS:
*       devNum             - The Pp device number to get the parameters for.
*       maxNumOfPbrEntries - number of PBR entries to deduct from the LPM memory calculations
*
* OUTPUTS:
*      numOfBlocksPtr                - (pointe to)the number of RAM blocks that LPM uses.
*      blocksSizeArray               - array that holds the sizes of the RAM blocks in bytes
*      blocksAllocationMethodPtr     - (pointe to)the method of blocks allocation
*
* RETURNS:
*       GT_OK           - on success,
*       GT_FAIL         - otherwise.
*       GT_BAD_VALUE    - on bad value.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
GT_STATUS appDemoBc2IpLpmRamDefaultConfigCalc_elements
(
    IN  GT_U8       devNum,
    IN  GT_U32      maxNumOfPbrEntries,
    OUT GT_U32      *numOfBlocksPtr,
    OUT GT_U32      blocksSizeArray[],
    OUT GT_U32      *blocksAllocationMethodPtr
)
{
    GT_U32 i=0;
    GT_STATUS rc = GT_OK;
    APP_DEMO_CPSS_LPM_RAM_MEM_BLOCKS_CFG_STC ramDbCfg;

    cpssOsMemSet(&ramDbCfg,0,sizeof(APP_DEMO_CPSS_LPM_RAM_MEM_BLOCKS_CFG_STC));

    rc =appDemoBc2IpLpmRamDefaultConfigCalc(devNum,maxNumOfPbrEntries,&ramDbCfg);
    if (rc!=GT_OK)
    {
        return rc;
    }

    *numOfBlocksPtr = ramDbCfg.numOfBlocks;
    for (i=0;i<ramDbCfg.numOfBlocks;i++)
    {
        blocksSizeArray[i]=ramDbCfg.blocksSizeArray[i];
    }
    switch (ramDbCfg.blocksAllocationMethod)
    {
        case APP_DEMO_CPSS_LPM_RAM_BLOCKS_ALLOCATION_METHOD_DYNAMIC_WITHOUT_BLOCK_SHARING_E:
            *blocksAllocationMethodPtr=0;
            break;
        case APP_DEMO_CPSS_LPM_RAM_BLOCKS_ALLOCATION_METHOD_DYNAMIC_WITH_BLOCK_SHARING_E:
            *blocksAllocationMethodPtr=1;
            break;
        default:
            return GT_BAD_VALUE;
    }
    return rc;
}

/*******************************************************************************
* getPpLogicalInitParams
*
* DESCRIPTION:
*       Returns the parameters needed for sysPpLogicalInit() function.
*
* INPUTS:
*       boardRevId      - The board revision Id.
*       devNum          - The Pp device number to get the parameters for.
*
* OUTPUTS:
*       ppLogInitParams - Pp logical init parameters struct.
*
* RETURNS:
*       GT_OK   - on success,
*       GT_FAIL - otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static GT_STATUS getPpLogicalInitParams
(
    IN  GT_U8           boardRevId,
    IN  GT_U8           devNum,
    OUT CPSS_PP_CONFIG_INIT_STC    *ppLogInitParams
)
{
    GT_STATUS rc = GT_OK;
    CPSS_PP_CONFIG_INIT_STC  localPpCfgParams = PP_LOGICAL_CONFIG_DEFAULT;

    PRV_CPSS_DXCH_BC2_UNUSED_PARAM_MAC(boardRevId);

    localPpCfgParams.numOfTrunks = _8K-1;
    localPpCfgParams.maxNumOfPbrEntries = _8K;

    rc = appDemoBc2IpLpmRamDefaultConfigCalc(devNum,localPpCfgParams.maxNumOfPbrEntries,&(localPpCfgParams.lpmRamMemoryBlocksCfg));
    if (rc != GT_OK)
    {
        return rc;
    }
    osMemCpy(ppLogInitParams, &localPpCfgParams, sizeof(CPSS_PP_CONFIG_INIT_STC));

    return rc;
}

/*******************************************************************************
* getPpLogicalInitParamsSimple
*
* DESCRIPTION:
*       Returns the parameters needed for sysPpLogicalInit() function.
*
* INPUTS:
*       boardRevId      - The board revision Id.
*       devNum          - The Pp device number to get the parameters for.
*
* OUTPUTS:
*       ppLogInitParams - Pp logical init parameters struct.
*
* RETURNS:
*       GT_OK   - on success,
*       GT_FAIL - otherwise.
*
* COMMENTS:
*       This is a simplified version of getPpLogicalInitParams.
*
*******************************************************************************/

static GT_STATUS getPpLogicalInitParamsSimple
(
    IN  GT_U8           boardRevId,
    IN  GT_U8           devNum,
    OUT CPSS_DXCH_PP_CONFIG_INIT_STC    *localPpCfgParams
)
{

    PRV_CPSS_DXCH_BC2_UNUSED_PARAM_MAC(boardRevId);
    PRV_CPSS_DXCH_BC2_UNUSED_PARAM_MAC(devNum);

    localPpCfgParams->maxNumOfVirtualRouters = 1;
    localPpCfgParams->maxNumOfIpNextHop = 100;
    localPpCfgParams->maxNumOfIpv4Prefixes = 3920;
    localPpCfgParams->maxNumOfMll = 500;
    localPpCfgParams->maxNumOfIpv4McEntries = 100;
    localPpCfgParams->maxNumOfIpv6Prefixes  = 100;
    localPpCfgParams->maxNumOfTunnelEntries  = 200;
    localPpCfgParams->maxNumOfIpv4TunnelTerms  = 400;
    localPpCfgParams->routingMode  = CPSS_DXCH_TCAM_ROUTER_BASED_E;
    localPpCfgParams->maxNumOfIpv6Prefixes  = 100;
    localPpCfgParams->maxNumOfPbrEntries = _8K;


    return GT_OK;
}


/*******************************************************************************
* getTapiLibInitParams
*
* DESCRIPTION:
*       Returns Tapi library initialization parameters.
*
* INPUTS:
*       boardRevId      - The board revision Id.
*
* OUTPUTS:
*       libInitParams   - Library initialization parameters.
*
* RETURNS:
*       GT_OK   - on success,
*       GT_FAIL - otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static GT_STATUS getTapiLibInitParams
(
    IN  GT_U8                       boardRevId,
    IN  GT_U8                       devNum,
    OUT APP_DEMO_LIB_INIT_PARAMS    *libInitParams
)
{
    APP_DEMO_LIB_INIT_PARAMS  localLibInitParams = LIB_INIT_PARAMS_DEFAULT;

    PRV_CPSS_DXCH_BC2_UNUSED_PARAM_MAC(boardRevId);
    PRV_CPSS_DXCH_BC2_UNUSED_PARAM_MAC(devNum);

    localLibInitParams.initClassifier            = GT_FALSE;
    localLibInitParams.initIpv6                  = GT_TRUE;
    localLibInitParams.initIpv4                  = GT_TRUE;

    localLibInitParams.initIpv4Filter            = GT_FALSE;
    localLibInitParams.initIpv4Tunnel            = GT_FALSE;
    localLibInitParams.initMpls                  = GT_FALSE;
    localLibInitParams.initMplsTunnel            = GT_FALSE;
    localLibInitParams.initPcl                   = GT_TRUE;
    localLibInitParams.initTcam                  = GT_TRUE;

    /* there is no Policer lib init for Bobcat2 devices */
    localLibInitParams.initPolicer               = GT_FALSE;

    osMemCpy(libInitParams,&localLibInitParams,
             sizeof(APP_DEMO_LIB_INIT_PARAMS));
    return GT_OK;
}


/*******************************************************************************
* bcat2PortInterfaceInit
*
*******************************************************************************/
static PortInitList_STC portInitlist_29_100_48x1G_QXGMII_7x10G_KR[] =
{
     { PORT_LIST_TYPE_INTERVAL,  { 0,47,1,     APP_INV_PORT_CNS },  CPSS_PORT_SPEED_1000_E,   CPSS_PORT_INTERFACE_MODE_QSGMII_E  }
    ,{ PORT_LIST_TYPE_LIST,      {49,50,51,    APP_INV_PORT_CNS },  CPSS_PORT_SPEED_10000_E,  CPSS_PORT_INTERFACE_MODE_KR_E      }
    ,{ PORT_LIST_TYPE_LIST,      {57,58,80,82, APP_INV_PORT_CNS },  CPSS_PORT_SPEED_10000_E,  CPSS_PORT_INTERFACE_MODE_KR_E      }
    ,{ PORT_LIST_TYPE_EMPTY,     {             APP_INV_PORT_CNS },  CPSS_PORT_SPEED_NA_E,     CPSS_PORT_INTERFACE_MODE_NA_E      }
};
/* next port list is 'theoretical' as the BW is larger then actual device supports.
   but the devices that should use it hold actual less MACs (ports).

   like : Cetus  : 0  GE ports , 12 XG ports.
          Caelum : 47 GE ports , 12 XG ports.

*/
static PortInitList_STC portInitlist_AllPorts[] =
{
     { PORT_LIST_TYPE_INTERVAL,  { 0,47,1,  /* 0..47*/     APP_INV_PORT_CNS },  CPSS_PORT_SPEED_1000_E,   CPSS_PORT_INTERFACE_MODE_QSGMII_E  }
    ,{ PORT_LIST_TYPE_INTERVAL,  {48,59,1,  /*48..59*/     APP_INV_PORT_CNS },  CPSS_PORT_SPEED_10000_E,  CPSS_PORT_INTERFACE_MODE_KR_E      }
    ,{ PORT_LIST_TYPE_INTERVAL,  {64,71,1,  /*64..71*/     APP_INV_PORT_CNS },  CPSS_PORT_SPEED_10000_E,  CPSS_PORT_INTERFACE_MODE_KR_E      }
    ,{ PORT_LIST_TYPE_LIST,      {80,81,82, /*80,81,82*/   APP_INV_PORT_CNS },  CPSS_PORT_SPEED_10000_E,  CPSS_PORT_INTERFACE_MODE_KR_E      }
    ,{ PORT_LIST_TYPE_EMPTY,     {             APP_INV_PORT_CNS },  CPSS_PORT_SPEED_NA_E,     CPSS_PORT_INTERFACE_MODE_NA_E      }
};

static PortInitList_STC portInitlist_29_1_16x1G_QXGMII_8x10G_KR[] =
{
     { PORT_LIST_TYPE_INTERVAL,  { 0,15,1,      APP_INV_PORT_CNS }, CPSS_PORT_SPEED_1000_E,   CPSS_PORT_INTERFACE_MODE_QSGMII_E     }
    ,{ PORT_LIST_TYPE_INTERVAL,  { 24,31,1,     APP_INV_PORT_CNS }, CPSS_PORT_SPEED_10000_E,  CPSS_PORT_INTERFACE_MODE_KR_E      }
    ,{ PORT_LIST_TYPE_EMPTY,     {              APP_INV_PORT_CNS }, CPSS_PORT_SPEED_NA_E,     CPSS_PORT_INTERFACE_MODE_NA_E         }
};

static char * prv_speed2str[CPSS_PORT_SPEED_NA_E+1] =
{
    "10M  ",   /* 0 */
    "100M ",   /* 1 */
    "1G   ",   /* 2 */
    "10G  ",   /* 3 */
    "12G  ",   /* 4 */
    "2.5G ",   /* 5 */
    "5G   ",   /* 6 */
    "13.6G",   /* 7 */
    "20G  ",   /* 8 */
    "40G  ",   /* 9 */
    "16G  ",   /* 10 */
    "15G  ",   /* 11 */
    "75G  ",   /* 12 */
    "100G ",   /* 13 */
    "50G  ",   /* 14 */
    "140G "    /* 15 */
    ,"NA "     /* 16 */
};

static char * prv_prvif2str[CPSS_PORT_INTERFACE_MODE_NA_E+1] =
{
    "REDUCED_10BIT"   /* 0   CPSS_PORT_INTERFACE_MODE_REDUCED_10BIT_E,    */
    ,"REDUCED_GMII"    /* 1   CPSS_PORT_INTERFACE_MODE_REDUCED_GMII_E,     */
    ,"MII         "    /* 2   CPSS_PORT_INTERFACE_MODE_MII_E,              */
    ,"SGMII       "    /* 3   CPSS_PORT_INTERFACE_MODE_SGMII_E,            */
    ,"XGMII       "    /* 4   CPSS_PORT_INTERFACE_MODE_XGMII_E,            */
    ,"MGMII       "    /* 5   CPSS_PORT_INTERFACE_MODE_MGMII_E,            */
    ,"1000BASE_X  "    /* 6   CPSS_PORT_INTERFACE_MODE_1000BASE_X_E,       */
    ,"GMII        "    /* 7   CPSS_PORT_INTERFACE_MODE_GMII_E,             */
    ,"MII_PHY     "    /* 8   CPSS_PORT_INTERFACE_MODE_MII_PHY_E,          */
    ,"QX          "    /* 9   CPSS_PORT_INTERFACE_MODE_QX_E,               */
    ,"HX          "    /* 10  CPSS_PORT_INTERFACE_MODE_HX_E,                */
    ,"RXAUI       "    /* 11  CPSS_PORT_INTERFACE_MODE_RXAUI_E,             */
    ,"100BASE_FX  "    /* 12  CPSS_PORT_INTERFACE_MODE_100BASE_FX_E,        */
    ,"QSGMII      "    /* 13  CPSS_PORT_INTERFACE_MODE_QSGMII_E,            */
    ,"XLG         "    /* 14  CPSS_PORT_INTERFACE_MODE_XLG_E,               */
    ,"LOCAL_XGMII "    /* 15  CPSS_PORT_INTERFACE_MODE_LOCAL_XGMII_E,       */
    ,"KR          "    /* 16  CPSS_PORT_INTERFACE_MODE_KR_E,                */
    ,"HGL         "    /* 17  CPSS_PORT_INTERFACE_MODE_HGL_E,               */
    ,"CHGL_12     "    /* 18  CPSS_PORT_INTERFACE_MODE_CHGL_12_E,           */
    ,"ILKN12      "    /* 19  CPSS_PORT_INTERFACE_MODE_ILKN12_E,            */
    ,"SR_LR       "    /* 20  CPSS_PORT_INTERFACE_MODE_SR_LR_E,             */
    ,"ILKN16      "    /* 21  CPSS_PORT_INTERFACE_MODE_ILKN16_E,            */
    ,"ILKN24      "    /* 22  CPSS_PORT_INTERFACE_MODE_ILKN24_E,            */
    ,"ILKN4       "    /* 23  CPSS_PORT_INTERFACE_MODE_ILKN4_E,             */
    ,"ILKN8       "    /* 24  CPSS_PORT_INTERFACE_MODE_ILKN8_E,             */
    ,"NA_E        "    /* 25  CPSS_PORT_INTERFACE_MODE_NA_E                 */
};

static GT_CHAR * prv_mappingTypeStr[CPSS_DXCH_PORT_MAPPING_TYPE_MAX_E] =
{
    "ETHERNET"
   ,"CPU-SDMA"
   ,"ILKN-CHL"
   ,"REMOTE-P"
};



#ifdef WIN32
    #define cpssOsPrintf printf
#endif

extern GT_STATUS prvCpssDxChPortPATimeTakeEnable
(
    GT_VOID
);

extern GT_STATUS prvCpssDxChPortPATimeTakeDisable
(
    GT_VOID
);

extern GT_STATUS prvCpssDxChPortPATimeDiffGet
(
    OUT GT_U32 *prv_paTime_msPtr,
    OUT GT_U32 *prv_paTime_usPtr
);



GT_BOOL  prv_portInitimeTake = GT_FALSE;

GT_STATUS prvAppDeoPortInitTimeTakeEnable
(
    GT_VOID
)
{
    prv_portInitimeTake = GT_TRUE;
    return prvCpssDxChPortPATimeTakeEnable();
}

GT_STATUS prvAppDeoPortInitTimeTimeDisable
(
    GT_VOID
)
{
    prv_portInitimeTake = GT_FALSE;
    return prvCpssDxChPortPATimeTakeDisable();
}


GT_STATUS appDemoBc2PortListInit
(
    IN GT_U8 dev,
    IN PortInitList_STC * portInitList
)
{
    GT_STATUS rc;
    GT_U32 i;
    GT_U32 portIdx;
    GT_U32 maxPortIdx;
    CPSS_PORTS_BMP_STC initPortsBmp,/* bitmap of ports to init */
                      *initPortsBmpPtr;/* pointer to bitmap */
    GT_PHYSICAL_PORT_NUM portNum;
    /* PortInitList_STC * portInitList; */
    PortInitList_STC * portInitPtr;
    GT_U32         coreClockDB;
    GT_U32         coreClockHW;
    static PortInitInternal_STC    portList[256];
    PRV_CPSS_DXCH_PORT_MAP_SHADOW_STC *portMapPtr;

    GT_U32 prv_paTime_ms;
    GT_U32 prv_paTime_us;
    GT_U32 prv_portTime_ms;
    GT_U32 prv_portTime_us;
    GT_TIME timeStart = {0,0};




    for (portIdx = 0 ; portIdx < sizeof(portList)/sizeof(portList[0]); portIdx++)
    {
        portList[portIdx].portNum       = APP_INV_PORT_CNS;
        portList[portIdx].speed         = CPSS_PORT_SPEED_NA_E;
        portList[portIdx].interfaceMode = CPSS_PORT_INTERFACE_MODE_NA_E;
    }

    maxPortIdx = 0;
    portInitPtr = portInitList;
    for (i = 0 ; portInitPtr->entryType != PORT_LIST_TYPE_EMPTY; i++,portInitPtr++)
    {
        switch (portInitPtr->entryType)
        {
            case PORT_LIST_TYPE_INTERVAL:
                for (portNum = portInitPtr->portList[0] ; portNum <= portInitPtr->portList[1]; portNum += portInitPtr->portList[2])
                {
                    CPSS_ENABLER_PORT_SKIP_CHECK(dev, portNum);
                    portList[maxPortIdx].portNum       = portNum;
                    portList[maxPortIdx].speed         = portInitPtr->speed;
                    portList[maxPortIdx].interfaceMode = portInitPtr->interfaceMode;
                    maxPortIdx++;
                }
            break;
            case PORT_LIST_TYPE_LIST:
                for (portIdx = 0 ; portInitPtr->portList[portIdx] != APP_INV_PORT_CNS; portIdx++)
                {
                    portNum = portInitPtr->portList[portIdx];
                    CPSS_ENABLER_PORT_SKIP_CHECK(dev, portNum);
                    portList[maxPortIdx].portNum       = portNum;
                    portList[maxPortIdx].speed         = portInitPtr->speed;
                    portList[maxPortIdx].interfaceMode = portInitPtr->interfaceMode;
                    maxPortIdx++;
                }
            break;
            default:
                {
                    return GT_NOT_SUPPORTED;
                }
        }
    }

    rc = cpssDxChHwCoreClockGet(dev,/*OUT*/&coreClockDB,&coreClockHW);
    if(rc != GT_OK)
    {
        return rc;
    }

    cpssOsPrintf("\nClock %d MHz",coreClockHW);
    cpssOsPrintf("\n+----+------+-------+--------------+-----------------+------------------------------+------+");
    cpssOsPrintf("\n| #  | Port | Speed |    IF        |   mapping Type  | rxdma txdma mac txq ilkn  tm |  res |");
    cpssOsPrintf("\n+----+------+-------+--------------+-----------------+------------------------------+------+");

    initPortsBmpPtr = &initPortsBmp;

    for (portIdx = 0 ; portIdx < maxPortIdx; portIdx++)
    {
        timeStart.sec = 0;
        timeStart.nanoSec = 0;

        rc = prvCpssDxChPortPhysicalPortMapShadowDBGet(dev,portList[portIdx].portNum,/*OUT*/&portMapPtr);
        if (rc != GT_OK)
        {
            return rc;
        }

        cpssOsPrintf("\n| %2d | %4d | %s | %s |",portIdx,
                                              portList[portIdx].portNum,
                                              prv_speed2str[portList[portIdx].speed],
                                              prv_prvif2str[portList[portIdx].interfaceMode]);
        cpssOsPrintf(" %-15s | %5d %5d %3d %3d %4d %3d |"
                                            ,prv_mappingTypeStr[portMapPtr->portMap.mappingType]
                                            ,portMapPtr->portMap.rxDmaNum
                                            ,portMapPtr->portMap.txDmaNum
                                            ,portMapPtr->portMap.macNum
                                            ,portMapPtr->portMap.txqNum
                                            ,portMapPtr->portMap.ilknChannel
                                            ,portMapPtr->portMap.tmPortIdx);

        if (prv_portInitimeTake == GT_TRUE)
        {
            rc = prvCpssOsTimeRTns(&timeStart);
            if (rc != GT_OK)
            {
                return rc;
            }
        }

        CPSS_PORTS_BMP_PORT_CLEAR_ALL_MAC(initPortsBmpPtr);
        CPSS_PORTS_BMP_PORT_SET_MAC(initPortsBmpPtr,portList[portIdx].portNum);
        rc = cpssDxChPortModeSpeedSet(dev, initPortsBmp, GT_TRUE,
                                        portList[portIdx].interfaceMode,
                                        portList[portIdx].speed);

        if(rc != GT_OK)
        {
            cpssOsPrintf("\n--> ERROR : cpssDxChPortModeSpeedSet(portNum=%d, ifMode=%d, speed=%d) :rc=%d\n",
            portList[portIdx].portNum, portList[portIdx].interfaceMode, portList[portIdx].speed, rc);
            return rc;
        }

        if (prv_portInitimeTake == GT_TRUE)
        {
            prvCpssOsTimeRTDiff(timeStart, /*OUT*/&prv_portTime_ms, &prv_portTime_us);
        }


        cpssOsPrintf("  OK  |");

        if (prv_portInitimeTake == GT_TRUE)
        {
            cpssOsPrintf(" total : %4d.%03d ms",prv_portTime_ms,prv_portTime_us);
            if (GT_OK == prvCpssDxChPortPATimeDiffGet(&prv_paTime_ms,&prv_paTime_us))
            {
                cpssOsPrintf(" PA-time : %4d.%03d ms",prv_paTime_ms,prv_paTime_us);

            }
        }
    }
    cpssOsPrintf("\n+----+------+-------+--------------+-----------------+------------------------------+------+");
    cpssOsPrintf("\nPORT INTERFACE Init Done.\n");
    return GT_OK;
}


GT_STATUS appDemoBc2PortListDelete
(
    IN GT_U8 dev,
    IN PortInitList_STC * portInitList
)
{
    GT_STATUS rc;
    GT_U32 i;
    GT_U32 portIdx;
    GT_U32 maxPortIdx;
    CPSS_PORTS_BMP_STC initPortsBmp,/* bitmap of ports to init */
                      *initPortsBmpPtr;/* pointer to bitmap */
    GT_PHYSICAL_PORT_NUM portNum;
    /* PortInitList_STC * portInitList; */
    PortInitList_STC * portInitPtr;
    GT_U32         coreClockDB;
    GT_U32         coreClockHW;
    static PortInitInternal_STC    portList[256];


    for (portIdx = 0 ; portIdx < sizeof(portList)/sizeof(portList[0]); portIdx++)
    {
        portList[portIdx].portNum       = APP_INV_PORT_CNS;
        portList[portIdx].speed         = CPSS_PORT_SPEED_NA_E;
        portList[portIdx].interfaceMode = CPSS_PORT_INTERFACE_MODE_NA_E;
    }

    maxPortIdx = 0;
    portInitPtr = portInitList;
    for (i = 0 ; portInitPtr->entryType != PORT_LIST_TYPE_EMPTY; i++,portInitPtr++)
    {
        switch (portInitPtr->entryType)
        {
            case PORT_LIST_TYPE_INTERVAL:
                for (portNum = portInitPtr->portList[0] ; portNum <= portInitPtr->portList[1]; portNum += portInitPtr->portList[2])
                {
                    CPSS_ENABLER_PORT_SKIP_CHECK(dev, portNum);
                    portList[maxPortIdx].portNum       = portNum;
                    portList[maxPortIdx].speed         = portInitPtr->speed;
                    portList[maxPortIdx].interfaceMode = portInitPtr->interfaceMode;
                    maxPortIdx++;
                }
            break;
            case PORT_LIST_TYPE_LIST:
                for (portIdx = 0 ; portInitPtr->portList[portIdx] != APP_INV_PORT_CNS; portIdx++)
                {
                    portNum = portInitPtr->portList[portIdx];
                    CPSS_ENABLER_PORT_SKIP_CHECK(dev, portNum);
                    portList[maxPortIdx].portNum       = portNum;
                    portList[maxPortIdx].speed         = portInitPtr->speed;
                    portList[maxPortIdx].interfaceMode = portInitPtr->interfaceMode;
                    maxPortIdx++;
                }
            break;
            default:
                {
                    return GT_NOT_SUPPORTED;
                }
        }
    }

    rc = cpssDxChHwCoreClockGet(dev,/*OUT*/&coreClockDB,&coreClockHW);
    if(rc != GT_OK)
    {
        return rc;
    }

    cpssOsPrintf("\nClock %d MHz",coreClockHW);
    cpssOsPrintf("\n+----+------+-------+--------------+-----------------+------------------------------+------+");
    cpssOsPrintf("\n| #  | Port | Speed |    IF        |   mapping Type  | rxdma txdma mac txq ilkn  tm |  res |");
    cpssOsPrintf("\n+----+------+-------+--------------+-----------------+------------------------------+------+");

    initPortsBmpPtr = &initPortsBmp;

    for (portIdx = 0 ; portIdx < maxPortIdx; portIdx++)
    {
        PRV_CPSS_DXCH_PORT_MAP_SHADOW_STC *portMapPtr;
        rc = prvCpssDxChPortPhysicalPortMapShadowDBGet(dev,portList[portIdx].portNum,/*OUT*/&portMapPtr);
        if (rc != GT_OK)
        {
            return rc;
        }

        cpssOsPrintf("\n| %2d | %4d | %s | %s |",portIdx,
                                              portList[portIdx].portNum,
                                              prv_speed2str[portList[portIdx].speed],
                                              prv_prvif2str[portList[portIdx].interfaceMode]);
        cpssOsPrintf(" %-15s | %5d %5d %3d %3d %4d %3d |"
                                            ,prv_mappingTypeStr[portMapPtr->portMap.mappingType]
                                            ,portMapPtr->portMap.rxDmaNum
                                            ,portMapPtr->portMap.txDmaNum
                                            ,portMapPtr->portMap.macNum
                                            ,portMapPtr->portMap.txqNum
                                            ,portMapPtr->portMap.ilknChannel
                                            ,portMapPtr->portMap.tmPortIdx);


        CPSS_PORTS_BMP_PORT_CLEAR_ALL_MAC(initPortsBmpPtr);
        CPSS_PORTS_BMP_PORT_SET_MAC(initPortsBmpPtr,portList[portIdx].portNum);
        rc = cpssDxChPortModeSpeedSet(dev, initPortsBmp, GT_FALSE,
                                        portList[portIdx].interfaceMode,
                                        portList[portIdx].speed);
        if(rc != GT_OK)
        {
            cpssOsPrintf("\n--> ERROR : cpssDxChPortModeSpeedSet(portNum=%d, ifMode=%d, speed=%d) :rc=%d\n",
            portList[portIdx].portNum, portList[portIdx].interfaceMode, portList[portIdx].speed, rc);
            return rc;
        }
        cpssOsPrintf("  OK  |");
    }
    cpssOsPrintf("\n+----+------+-------+--------------+-----------------+------------------------------+------+");
    cpssOsPrintf("\nPORT List Delete Done.");
    return GT_OK;
}


static GT_U32   convertSpeedToGig[CPSS_PORT_SPEED_NA_E] =
{
    /*CPSS_PORT_SPEED_10_E,     */ 1 /* 0 */     ,
    /*CPSS_PORT_SPEED_100_E,    */ 1 /* 1 */     ,
    /*CPSS_PORT_SPEED_1000_E,   */ 1 /* 2 */     ,
    /*CPSS_PORT_SPEED_10000_E,  */ 10 /* 3 */    ,
    /*CPSS_PORT_SPEED_12000_E,  */ 12 /* 4 */    ,
    /*CPSS_PORT_SPEED_2500_E,   */ 3 /* 5 */     ,
    /*CPSS_PORT_SPEED_5000_E,   */ 5 /* 6 */     ,
    /*CPSS_PORT_SPEED_13600_E,  */ 14 /* 7 */    ,
    /*CPSS_PORT_SPEED_20000_E,  */ 20 /* 8 */    ,
    /*CPSS_PORT_SPEED_40000_E,  */ 40 /* 9 */    ,
    /*CPSS_PORT_SPEED_16000_E,  */ 16 /* 10 */   ,
    /*CPSS_PORT_SPEED_15000_E,  */ 15 /* 11 */   ,
    /*CPSS_PORT_SPEED_75000_E,  */ 75 /* 12 */   ,
    /*CPSS_PORT_SPEED_100G_E,   */ 100 /* 13 */  ,
    /*CPSS_PORT_SPEED_50000_E,  */ 50 /* 14 */   ,
    /*CPSS_PORT_SPEED_140G_E,   */ 140 /* 15 */  ,

    /*CPSS_PORT_SPEED_11800_E,  */ 12 /* 16  */ ,/*used in combination with CPSS_PORT_INTERFACE_MODE_XHGS_E */
    /*CPSS_PORT_SPEED_47200_E,  */ 48 /* 17  */ ,/*used in combination with CPSS_PORT_INTERFACE_MODE_XHGS_E */
};
/* function to summarize the number of GIG needed for the port list */
static GT_U32  getNumGigBw(
    IN GT_U8             dev,
    IN PortInitList_STC * portInitPtr
)
{
    GT_U32  i;
    GT_U32  portNum,portIdx;
    GT_U32  summary = 1;/* 1G needed for the CPU SDMA port */

    for (i = 0 ; portInitPtr->entryType != PORT_LIST_TYPE_EMPTY; i++,portInitPtr++)
    {
        switch (portInitPtr->entryType)
        {
            case PORT_LIST_TYPE_INTERVAL:
                for (portNum = portInitPtr->portList[0] ; portNum <= portInitPtr->portList[1]; portNum += portInitPtr->portList[2])
                {
                    CPSS_ENABLER_PORT_SKIP_CHECK(dev, portNum);

                    summary += convertSpeedToGig[portInitPtr->speed];
                }
                break;
            case PORT_LIST_TYPE_LIST:
                for (portIdx = 0 ; portInitPtr->portList[portIdx] != APP_INV_PORT_CNS; portIdx++)
                {
                    portNum = portInitPtr->portList[portIdx];
                    CPSS_ENABLER_PORT_SKIP_CHECK(dev, portNum);

                    summary += convertSpeedToGig[portInitPtr->speed];
                }
                break;
            default:
                break;
        }
    }

    return summary;
}


/*******************************************************************************
* appDemoBc2PortInterfaceInit
*
* DESCRIPTION:
*     Execute predefined ports configuration.
*
* INPUTS:
*       dev - device number
*       boardRevId - revision ID
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK   - on success,
*       GT_FAIL - otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
GT_STATUS appDemoBc2PortInterfaceInit
(
    IN  GT_U8 dev,
    IN  GT_U8 boardRevId
)
{
    GT_STATUS   rc;             /* return code */
    GT_U32  initSerdesDefaults, /* should appDemo configure ports modes */
            bcat2PortsConfigType; /* which ports modes configuration wanted:
                                    0 - ports 0-47-QSGMII,48-SGMII,49-51-KR_10G,56-62-RXAUI;
                                    */
    GT_U32         coreClockDB;
    GT_U32         coreClockHW;

    PortInitList_STC * portInitList;

    /* check if user wants to init ports to default values */
    rc = appDemoDbEntryGet("initSerdesDefaults", &initSerdesDefaults);
    if(rc != GT_OK)
    {
        initSerdesDefaults = 1;
    }

    if(0 == initSerdesDefaults)
    {
        return GT_OK;
    }

    rc = cpssDxChHwCoreClockGet(dev,/*OUT*/&coreClockDB,&coreClockHW);
    if(rc != GT_OK)
    {
        return rc;
    }

    rc = bcat2PortInterfaceInitPortInitListGet(dev, boardRevId, coreClockDB,/*OUT*/&portInitList);
    if(rc != GT_OK)
    {
        return rc;
    }

    if (NULL == portInitList)
    {
        cpssOsPrintf("\n-->ERROR : bcat2PortInterfaceInit() : revId % is not supported\n", boardRevId);
        return GT_NOT_SUPPORTED;
    }

    rc = appDemoDbEntryGet("bcat2PortsConfigType", &bcat2PortsConfigType);
    if(rc != GT_OK)
    {
        if (boardRevId == 1)
        {
            if(PRV_CPSS_DXCH_PP_MAC(dev)->genInfo.devFamily ==
               CPSS_PP_FAMILY_DXCH_BOBCAT3_E)
            {
                /* currently use the array given by
                   bcat2PortInterfaceInitPortInitListGet */
                bcat2PortsConfigType = 0;
            }
            else
            if (coreClockDB == 362 || coreClockDB == 521)
            {
                /* use Port Config 100 (advanced UT mode) for 29,1,0 revision */
                bcat2PortsConfigType = 100;
            }
        }
        else
        {
            bcat2PortsConfigType = 0;
        }
    }

    switch(bcat2PortsConfigType)
    {
        case 100:
            portInitList = &portInitlist_29_100_48x1G_QXGMII_7x10G_KR[0];
        break;
    }
    if (PRV_CPSS_DXCH_PP_MAC(dev)->genInfo.devType == CPSS_98DX4253_CNS)
    {
        portInitList = &portInitlist_29_1_16x1G_QXGMII_8x10G_KR[0];
    }

    if(portInitlist_AllPorts_used ||
       appDemoPpConfigList[dev].portInitlist_AllPorts_used)
    {
        GT_U32  numGigNeeded;

        portInitList = portInitlist_AllPorts;

        /* calc the needed BW needed */
        numGigNeeded = getNumGigBw(dev,portInitList);

        /*
            state to get more BW than given by :
            Total Bandwidth @64B
            or by Max Bandwidth
        */
        rc = cpssDxChPortPizzaArbiterPipeBWMinPortSpeedResolutionSet(dev,numGigNeeded,CPSS_DXCH_MIN_SPEED_1000_Mbps_E);
        if (rc != GT_OK)
        {
            return rc;
        }
    }


    rc = appDemoBc2PortListInit(dev,portInitList);
    if (rc != GT_OK)
    {
        return rc;
    }

    if (boardRevId == 4)
    {
        /*40G to 4*10G split cabel to run the test */
        cpssDxChPortSerdesPolaritySet(0, 59, 1, GT_TRUE, GT_TRUE);
        cpssDxChPortSerdesPolaritySet(0, 58, 1, GT_TRUE, GT_TRUE);
        cpssDxChPortSerdesPolaritySet(0, 57, 1, GT_FALSE, GT_FALSE);
        cpssDxChPortSerdesPolaritySet(0, 56, 1, GT_FALSE, GT_FALSE);

        cpssDxChPortSerdesPolaritySet(0, 71, 1, GT_FALSE, GT_FALSE);
        cpssDxChPortSerdesPolaritySet(0, 70, 1, GT_FALSE, GT_FALSE);
        cpssDxChPortSerdesPolaritySet(0, 69, 1, GT_FALSE, GT_FALSE);
        cpssDxChPortSerdesPolaritySet(0, 68, 1, GT_FALSE, GT_FALSE);
        cpssDxChPortSerdesPolaritySet(0, 64, 1, GT_FALSE, GT_TRUE);
        cpssDxChPortSerdesPolaritySet(0, 65, 1, GT_FALSE, GT_TRUE);
        cpssDxChPortSerdesPolaritySet(0, 66, 1, GT_FALSE, GT_TRUE);
        cpssDxChPortSerdesPolaritySet(0, 67, 1, GT_TRUE, GT_TRUE);
    }

    if (boardRevId == 3)
    {
        cpssDxChPortSerdesPolaritySet(0, 66, 1, GT_FALSE, GT_TRUE);
        cpssDxChPortSerdesPolaritySet(0, 67, 1, GT_TRUE, GT_TRUE);
        cpssDxChPortSerdesPolaritySet(0, 64, 1, GT_FALSE, GT_TRUE);
        cpssDxChPortSerdesPolaritySet(0, 65, 1, GT_FALSE, GT_TRUE);
    }

    if (boardRevId == 4  || boardRevId == 3)
    {
        if (prvCpssHwPpPortGroupWriteRegister(dev,
                                              CPSS_PORT_GROUP_UNAWARE_MODE_CNS,
                                              0x102c8030, 0xc) != GT_OK)
        {
            return GT_HW_ERROR;
        }

        /*led programming */
        prvCpssHwPpPortGroupWriteRegister(dev, CPSS_PORT_GROUP_UNAWARE_MODE_CNS, 0x102c9030, 0x2c);
        prvCpssHwPpPortGroupWriteRegister(dev, CPSS_PORT_GROUP_UNAWARE_MODE_CNS, 0x102ca030, 0x4c);
        prvCpssHwPpPortGroupWriteRegister(dev, CPSS_PORT_GROUP_UNAWARE_MODE_CNS, 0x102cb030, 0x6c);

        prvCpssHwPpPortGroupWriteRegister(dev, CPSS_PORT_GROUP_UNAWARE_MODE_CNS, 0x102cc030, 0x8c);
        prvCpssHwPpPortGroupWriteRegister(dev, CPSS_PORT_GROUP_UNAWARE_MODE_CNS, 0x102cd030, 0xac);
        prvCpssHwPpPortGroupWriteRegister(dev, CPSS_PORT_GROUP_UNAWARE_MODE_CNS, 0x102ce030, 0xcc);
        prvCpssHwPpPortGroupWriteRegister(dev, CPSS_PORT_GROUP_UNAWARE_MODE_CNS, 0x102cf030, 0xec);

        prvCpssHwPpPortGroupWriteRegister(dev, CPSS_PORT_GROUP_UNAWARE_MODE_CNS, 0x102c0030, 0xc);
        prvCpssHwPpPortGroupWriteRegister(dev, CPSS_PORT_GROUP_UNAWARE_MODE_CNS, 0x102c1030, 0x2c);
        prvCpssHwPpPortGroupWriteRegister(dev, CPSS_PORT_GROUP_UNAWARE_MODE_CNS, 0x102c2030, 0x4c);
        prvCpssHwPpPortGroupWriteRegister(dev, CPSS_PORT_GROUP_UNAWARE_MODE_CNS, 0x102c3030, 0x6c);

        prvCpssHwPpPortGroupWriteRegister(dev, CPSS_PORT_GROUP_UNAWARE_MODE_CNS, 0x5000000c, 0x492249);
        prvCpssHwPpPortGroupWriteRegister(dev, CPSS_PORT_GROUP_UNAWARE_MODE_CNS, 0x50000000, 0xffc037);
        prvCpssHwPpPortGroupWriteRegister(dev, CPSS_PORT_GROUP_UNAWARE_MODE_CNS, 0x50000004, 0x2000137);

        prvCpssHwPpPortGroupWriteRegister(dev, CPSS_PORT_GROUP_UNAWARE_MODE_CNS, 0x2400000c, 0x492249);
        prvCpssHwPpPortGroupWriteRegister(dev, CPSS_PORT_GROUP_UNAWARE_MODE_CNS, 0x24000000, 0xffc037);
        prvCpssHwPpPortGroupWriteRegister(dev, CPSS_PORT_GROUP_UNAWARE_MODE_CNS, 0x24000004, 0x2000137);

    }

    return GT_OK;
}

/*******************************************************************************
* appDemoBc2DramOrTmInit
*
* DESCRIPTION:
*       Board specific configurations of TM and all related HW
*       or of TM related DRAM only.
*
* INPUTS:
*       dev             - device number
*       flags           - initialization flags
*                         currently supported value 1 that means "DRAM only"
*                         otherwise TM and all related HW initialized
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK   - on success,
*       GT_FAIL - otherwise.
*
* COMMENTS:
*       Used type of board recognized at last device (as in other code)
*
*******************************************************************************/
GT_STATUS appDemoBc2DramOrTmInit
(
    IN  GT_U8       dev,
    IN  GT_U32      flags
)
{
    GT_STATUS                            rc;
    CPSS_DXCH_TM_GLUE_DRAM_ALGORITHM_STC algoParams;
    CPSS_DXCH_TM_GLUE_DRAM_CFG_STC       *dramCfgPtr = NULL;

#ifdef ASIC_SIMULATION
    algoParams.algoType = CPSS_DXCH_TM_GLUE_DRAM_CONFIGURATION_TYPE_STATIC_E;
#else
    algoParams.algoType = CPSS_DXCH_TM_GLUE_DRAM_CONFIGURATION_TYPE_DYNAMIC_E;
#endif

    switch (bc2BoardType)
    {
        case APP_DEMO_CAELUM_BOARD_DB_CNS:
        case APP_DEMO_CETUS_BOARD_DB_CNS:
        case APP_DEMO_BC2_BOARD_DB_CNS:
            algoParams.performWriteLeveling = GT_TRUE;
            break;
        default:
            /*
            APP_DEMO_BC2_BOARD_RD_MSI_CNS:
            APP_DEMO_BC2_BOARD_RD_MTL_CNS:
            */
            algoParams.performWriteLeveling = GT_FALSE;
    }

    rc = cpssDxChTmGlueDramInitFlagsSet(dev, flags);
    CPSS_ENABLER_DBG_TRACE_RC_MAC("cpssDxChTmGlueDramInitFlagsSet", rc);
    if (rc != GT_OK)
    {
        return rc;
    }

    if (isBobkBoard)
    {
        if (bc2BoardType == APP_DEMO_CETUS_BOARD_DB_CNS)
            dramCfgPtr = &bobkDramCfgArr[BOBK_DRAM_CFG_IDX_CETUS_CNS];
        else
            dramCfgPtr = &bobkDramCfgArr[BOBK_DRAM_CFG_IDX_CAELUM_CNS];
    }
    else
    {
        dramCfgPtr = &bc2DramCfgArr[bc2BoardType];
    }
    
    /*    
    if (isBobkBoard)
    {
        cpssOsPrintf("\n\n!!!\n");
        cpssOsPrintf("SKIPPING cpssDxChTmGlueDramInit as its currently not supported (hwServices) for BobK!!!");
        cpssOsPrintf("\n!!!\n\n");
        return rc;
    }
    */

    rc = cpssDxChTmGlueDramInit(dev, dramCfgPtr, &algoParams);
    CPSS_ENABLER_DBG_TRACE_RC_MAC("cpssDxChTmGlueDramInit", rc);

    return rc;
}

/*******************************************************************************
* prvBobcat2TmPort73Config
*
* DESCRIPTION:
*       Port 73 (TM) specific configurations; for now for "Mode 2" (above_100g)
*
* INPUTS:
*       devNum   - physical device number
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK   - on success,
*       GT_FAIL - otherwise.
*
* COMMENTS:
*
*******************************************************************************/
static GT_STATUS prvBobcat2TmPort73Config
(
    IN GT_U8       devNum
)
{
    GT_STATUS   rc;
    GT_U32      regAddr, value;
    GT_U32      fieldOffset;
    GT_U32      fieldSize;

    if((PRV_CPSS_PP_MAC(devNum)->devFamily == CPSS_PP_FAMILY_DXCH_BOBCAT2_E) &&
       (PRV_CPSS_PP_MAC(devNum)->devSubFamily == CPSS_PP_SUB_FAMILY_BOBCAT2_BOBK_E))
    {
        /* nothing to do, there is no static init configuration for TM,
           it will be configured dynamically per add/delete port */

        return GT_OK;
    }

    /* TM port 73 Outgoing Bus %p Width and Shifter %p Threshold */

    if(PRV_CPSS_DXCH_BOBCAT2_A0_CHECK_MAC(devNum) == GT_FALSE)
    {
        value = 5;
    }
    else
    {
        value = 4;
    }

    regAddr = PRV_DXCH_REG1_UNIT_TX_FIFO_MAC(devNum).
                                txFIFOShiftersConfig.SCDMAShiftersConf[73];
    rc = prvCpssHwPpPortGroupSetRegField(devNum,
                                            CPSS_PORT_GROUP_UNAWARE_MODE_CNS,
                                            regAddr, 0, 6,
                                            /* 64B and port speed >= 100G */
                                            (6 | (value << 3)));
    if(rc != GT_OK)
    {
        return rc;
    }

    /* 'scdma_73_payload_threshold' */
    regAddr = PRV_DXCH_REG1_UNIT_TX_FIFO_MAC(devNum).
                                txFIFOGlobalConfig.SCDMAPayloadThreshold[73];
    rc = prvCpssHwPpPortGroupSetRegField(devNum,
                                            CPSS_PORT_GROUP_UNAWARE_MODE_CNS,
                                            regAddr, 0, 7, 0x20);
    if(rc != GT_OK)
    {
        return rc;
    }

    if(PRV_CPSS_DXCH_BOBCAT2_A0_CHECK_MAC(devNum) == GT_FALSE)
    {
        fieldOffset = 16;
        fieldSize = 14;
        value = (0<<4) | 0xa;
    }
    else
    {
        fieldOffset = 16;
        fieldSize = 13;
        value = (0<<3) | 0x6;
    }

    /* 'scdma_73_speed', 'rate_limit_threshold_scdma73'  */
    /* rate limit was 4 but is changed to 0 */
    regAddr = PRV_DXCH_REG1_UNIT_TXDMA_MAC(devNum).
                                        txDMAPerSCDMAConfigs.SCDMAConfigs[73];
    rc = prvCpssHwPpPortGroupSetRegField(devNum,
                                            CPSS_PORT_GROUP_UNAWARE_MODE_CNS,
                                                regAddr, fieldOffset, fieldSize, value);
    if(rc != GT_OK)
    {
        return rc;
    }

    if(PRV_CPSS_DXCH_BOBCAT2_A0_CHECK_MAC(devNum) == GT_FALSE)
    {
        fieldOffset = 0;
        fieldSize = 9;
    }
    else
    {
        fieldOffset = 17;
        fieldSize = 8;
    }

    /* 'desc_credits_scdma73' */
    regAddr = PRV_DXCH_REG1_UNIT_TXDMA_MAC(devNum).
                    txDMAPerSCDMAConfigs.FIFOsThresholdsConfigsSCDMAReg1[73];
    rc = prvCpssHwPpPortGroupSetRegField(devNum,
                                            CPSS_PORT_GROUP_UNAWARE_MODE_CNS,
                                                regAddr, fieldOffset, fieldSize, 0x3b);
    if(rc != GT_OK)
    {
        return rc;
    }

    /* 'txfifo_payload_counter_threshold_scdma73', 'txfifo_header_counter_threshold_scdma73' */
    regAddr = PRV_DXCH_REG1_UNIT_TXDMA_MAC(devNum).
                    txDMAPerSCDMAConfigs.txFIFOCntrsConfigsSCDMA[73];
    rc = prvCpssHwPpPortGroupSetRegField(devNum,
                                            CPSS_PORT_GROUP_UNAWARE_MODE_CNS,
                                            regAddr, 0, 20, ((0xfa << 10) | 0xfa));
    if(rc != GT_OK)
    {
        return rc;
    }

    if(PRV_CPSS_DXCH_BOBCAT2_A0_CHECK_MAC(devNum) == GT_FALSE)
    {
        fieldOffset = 16;
        fieldSize = 16;
    }
    else
    {
        fieldOffset = 15;
        fieldSize = 16;
    }

    /* 'burst_almost_full_threshold_scdma_73' */
    regAddr = PRV_DXCH_REG1_UNIT_TXDMA_MAC(devNum).
                    txDMAPerSCDMAConfigs.burstLimiterSCDMA[73];
    rc = prvCpssHwPpPortGroupSetRegField(devNum,
                                            CPSS_PORT_GROUP_UNAWARE_MODE_CNS,
                        /* Burst Almost Full Threshold + enable burst limit */
                                            regAddr, fieldOffset, fieldSize, 0x001f);
    return rc;
}

/*
    API to enable Custom ports mapping (called befor initSystem)
    when system is initialized with custom ports mapping,
    TM Ports Init is called after custom ports mapping is applied.

*/
void appDemoBc2SetCustomPortsMapping
(
    IN GT_BOOL enable
)
{
    isCustomPortsMapping = enable;
}

/*
    API to configure TmPorts, with non default ports mapping
*/

GT_STATUS appDemoBc2TmPortsInit
(
    IN GT_U8 dev
)
{
    CPSS_PORT_TX_DROP_PROFILE_SET_ENT profileSet;
    GT_BOOL                           enableSharing;
    GT_U32                            portMaxBuffLimit;
    GT_U32                            portMaxDescrLimit;
    GT_U8                             trafficClass;
    CPSS_PORT_TX_Q_TAIL_DROP_PROF_TC_PARAMS tailDropProfileParams;
    GT_U32      hwArray[2];   /* HW table data */
    PRV_CPSS_DXCH_TABLE_ENT tableType; /*table type*/
    GT_U32      appDemoDbValue;
    GT_STATUS   rc;
    GT_BOOL     isTmSupported;
    GT_U32      i;
    APP_DEMO_PP_CONFIG *appDemoPpConfigPtr;
    CPSS_DXCH_PORT_MAP_STC  *portsMappingPtr;

    rc = appDemoIsTmSupported(dev,/*OUT*/&isTmSupported);
    if (rc != GT_OK)
    {
        return rc;
    }

    if (!isTmSupported)
    {
        return GT_NOT_SUPPORTED;
    }

    if (GT_OK != appDemoDbEntryGet("externalMemoryInitFlags", &appDemoDbValue))
    {
        appDemoDbValue = 0;
    }

    rc = appDemoBc2DramOrTmInit(dev, appDemoDbValue);
    CPSS_ENABLER_DBG_TRACE_RC_MAC("appDemoBc2DramOrTmInit", rc);
    if (rc != GT_OK)
    {
        return rc;
    }

    appDemoPpConfigPtr = &appDemoPpConfigList[SYSTEM_DEV_NUM_MAC(dev)];
    portsMappingPtr = (CPSS_DXCH_PORT_MAP_STC*)appDemoPpConfigPtr->portsMapArrPtr;

    if (portsMappingPtr == NULL || appDemoPpConfigPtr->portsMapArrLen == 0)
    {
        cpssOsPrintf("appDemoBc2TmPortsInit: Unknown ports mapping\n");
        return GT_FAIL;
    }

    switch (appDemoTmScenarioMode)
    {
        case CPSS_TM_48_PORT:
            rc = appDemoTmGeneral48PortsInit(dev);
            CPSS_ENABLER_DBG_TRACE_RC_MAC("appDemoTmGeneral48PortsInit", rc);
            if (rc != GT_OK)
            {
                return rc;
            }
            break;
        case CPSS_TM_2:
            rc = appDemoTmScenario2Init(dev);
            CPSS_ENABLER_DBG_TRACE_RC_MAC("appDemoTmScenario2Init", rc);
            if (rc != GT_OK)
            {
                return rc;
            }
            break;
        case CPSS_TM_3:
            rc = appDemoTmScenario3Init(dev);
            CPSS_ENABLER_DBG_TRACE_RC_MAC("appDemoTmScenario3Init", rc);
            if (rc != GT_OK)
            {
                return rc;
            }
            break;
        case CPSS_TM_4:
            rc = appDemoTmScenario4Init(dev);
            CPSS_ENABLER_DBG_TRACE_RC_MAC("appDemoTmScenario4Init", rc);
            if (rc != GT_OK)
            {
                return rc;
            }
            break;
        case CPSS_TM_5:
            rc = appDemoTmScenario5Init(dev);
            CPSS_ENABLER_DBG_TRACE_RC_MAC("appDemoTmScenario5Init", rc);
            if (rc != GT_OK)
            {
                return rc;
            }
            break;
        case CPSS_TM_6:
            rc = appDemoTmScenario6Init(dev);
            CPSS_ENABLER_DBG_TRACE_RC_MAC("appDemoTmScenario6Init", rc);
            if (rc != GT_OK)
            {
                return rc;
            }
            break;

        case CPSS_TM_7:
            rc = appDemoTmScenario7Init(dev);
            CPSS_ENABLER_DBG_TRACE_RC_MAC("appDemoTmScenario7Init", rc);
            if (rc != GT_OK)
            {
                return rc;
            }
            break;

        case CPSS_TM_8:
            rc = appDemoTmScenario8Init(dev);
            CPSS_ENABLER_DBG_TRACE_RC_MAC("appDemoTmScenario8Init", rc);
            if (rc != GT_OK)
            {
                return rc;
            }
            break;

        case CPSS_TM_9:
            rc = appDemoTmScenario9Init(dev);
            CPSS_ENABLER_DBG_TRACE_RC_MAC("appDemoTmScenario9Init", rc);
            if (rc != GT_OK)
            {
                    return rc;
            }
            break;
        case CPSS_TM_STRESS:
            rc = appDemoTmStressScenarioInit(dev,0);
            CPSS_ENABLER_DBG_TRACE_RC_MAC("appDemoTmStressScenarioInit 0", rc);
            if (rc != GT_OK)
            {
                return rc;
            }
            break;
        case CPSS_TM_STRESS_1:
            rc = appDemoTmStressScenarioInit(dev,1);
            CPSS_ENABLER_DBG_TRACE_RC_MAC("appDemoTmStressScenarioInit 1", rc);
            if (rc != GT_OK)
            {
                return rc;
            }
            break;
        case CPSS_TM_USER_20: /*Private version  test for customer */
            rc = appDemoTmScenario20Init(dev);
            CPSS_ENABLER_DBG_TRACE_RC_MAC("appDemoTmScenario20Init", rc);
            if (rc != GT_OK)
            {
                return rc;
            }
            break;
        default:
            rc = appDemoTmLibInitOnly(dev);
            CPSS_ENABLER_DBG_TRACE_RC_MAC("appDemoTmLibInitOnly", rc);
            if (rc != GT_OK)
            {
                return rc;
            }
    }

    /* TM related configurations */
    rc = prvBobcat2TmGlueConfig(dev);
    if (rc != GT_OK)
    {
        return rc;
    }

    for(profileSet = CPSS_PORT_TX_DROP_PROFILE_1_E;
         profileSet <= CPSS_PORT_TX_DROP_PROFILE_16_E; profileSet++)
    {
        rc = cpssDxChPortTxTailDropProfileGet(dev, profileSet,
                                              &enableSharing,
                                              &portMaxBuffLimit,
                                              &portMaxDescrLimit);
        if (rc != GT_OK)
        {
            return rc;
        }
        portMaxBuffLimit = 0xfffff;
        rc = cpssDxChPortTxTailDropProfileSet(dev, profileSet,
                                              enableSharing,
                                              portMaxBuffLimit,
                                              portMaxDescrLimit);
        if (rc != GT_OK)
        {
            return rc;
        }

        for(trafficClass = 0; trafficClass < 8; trafficClass++)
        {
            rc = cpssDxChPortTx4TcTailDropProfileGet(dev, profileSet,
                                                     trafficClass,
                                                     &tailDropProfileParams);
            if (rc != GT_OK)
            {
                return rc;
            }
            tailDropProfileParams.dp0MaxBuffNum = BIT_14-1; /* max value ?????  0x3FFFF; */
            rc = cpssDxChPortTx4TcTailDropProfileSet(dev, profileSet,
                                                     trafficClass,
                                                     &tailDropProfileParams);
            if (rc != GT_OK)
            {
                return rc;
            }
        }
    }

    for(i = 0; i < 128; i++)
    {
        CPSS_TBD_BOOKMARK_BOBCAT2
        /* remove this when cpssDxChPortTx4TcTailDropProfileSet/Get will support new table structure */
        /* Queue Limits DP0 - Enqueue */
        tableType = PRV_CPSS_DXCH_LION3_TABLE_TAIL_DROP_EQ_QUEUE_LIMITS_DP0_E;
        hwArray[0] = 0xFFFFC019;
        hwArray[1] = 0xffff;
        rc = prvCpssDxChWriteTableEntry(dev, tableType, i, hwArray);
        if(rc != GT_OK)
        {
            return rc;
        }

        /* Queue Buffer Limits - Dequeue */
        tableType = PRV_CPSS_DXCH_LION3_TABLE_TAIL_DROP_DQ_QUEUE_BUF_LIMITS_E;
        hwArray[0] = 0xffffffff;
        hwArray[1] = 0x3;
        rc = prvCpssDxChWriteTableEntry(dev, tableType, i, hwArray);
        if(rc != GT_OK)
        {
            return rc;
        }
    }

    rc = prvBobcat2TmPort73Config(dev);

    return rc;
}

/*******************************************************************************
* afterInitBoardConfig
*
* DESCRIPTION:
*       Board specific configurations that should be done after board
*       initialization.
*
* INPUTS:
*       boardRevId      - The board revision Id.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK   - on success,
*       GT_FAIL - otherwise.
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_STATUS afterInitBoardConfig
(
    IN  GT_U8       boardRevId
)
{
    GT_STATUS   rc;
    GT_U8       dev;
    GT_U32      i;
    GT_U32      skipPhyInit;
    GT_U32      initSerdesDefaults;
    GT_U32      portNum;
    CPSS_PORTS_BMP_STC  portsMembers; /* VLAN members */
    CPSS_PORTS_BMP_STC  portsTagging; /* VLAN tagging */
    CPSS_DXCH_BRG_VLAN_INFO_STC  cpssVlanInfo;   /* cpss vlan info format    */
    CPSS_DXCH_BRG_VLAN_PORTS_TAG_CMD_STC portsTaggingCmd; /* ports tagging command */
    CPSS_PORTS_BMP_STC  portsAdditionalMembers; /* VLAN members to add */
    GT_BOOL             isValid, isTmSupported; /* is Valid flag */
#if(defined _linux)
#ifdef CPU_ARMADAXP_3_4_69
#ifndef ASIC_SIMULATION
    GT_U32      regValue;
#endif
#endif
#endif

    PRV_CPSS_DXCH_BC2_UNUSED_PARAM_MAC(boardRevId);

    /* allow processing of AA messages */
    appDemoSysConfig.supportAaMessage = GT_TRUE;

#if(defined _linux)
#ifdef CPU_ARMADAXP_3_4_69
#ifndef ASIC_SIMULATION

    rc = hwIfTwsiInitDriver();
    if(GT_OK != rc)
    {
        return rc;
    }
#endif
#endif
#endif

    if(appDemoSysConfig.forceAutoLearn == GT_FALSE)
    {
        /* the Bobcat2 need 'Controlled aging' because the port groups can't share refresh info */
        /* the AA to CPU enabled from appDemoDxChFdbInit(...) --> call cpssDxChBrgFdbAAandTAToCpuSet(...)*/
        for(dev = SYSTEM_DEV_NUM_MAC(0); dev < SYSTEM_DEV_NUM_MAC(ppCounter); dev++)
        {
            rc = cpssDxChBrgFdbActionModeSet(dev,CPSS_FDB_ACTION_AGE_WITHOUT_REMOVAL_E);
            if(rc != GT_OK)
            {
                return rc;
            }
        }
    }

    switch (bc2BoardType)
    {
        case APP_DEMO_CAELUM_BOARD_DB_CNS:
        case APP_DEMO_CETUS_BOARD_DB_CNS:
            isBobkBoard = GT_TRUE;
            break;
        default:
            /*
            APP_DEMO_BC2_BOARD_DB_CNS:
            APP_DEMO_BC2_BOARD_RD_MSI_CNS:
            APP_DEMO_BC2_BOARD_RD_MTL_CNS:
            */
            isBobkBoard = GT_FALSE;
    }

    for(dev = SYSTEM_DEV_NUM_MAC(0); dev < SYSTEM_DEV_NUM_MAC(ppCounter); dev++)
    {
        if (CPSS_SYSTEM_RECOVERY_PROCESS_NOT_ACTIVE_E ==
                                        systemRecoveryInfo.systemRecoveryProcess)
        {
            rc = appDemoBc2PortInterfaceInit(dev, boardRevId);
            if(rc != GT_OK)
            {
                return rc;
            }
        }

        /* clear additional default VLAN members */
        CPSS_PORTS_BMP_PORT_CLEAR_ALL_MAC(&portsAdditionalMembers);

        rc = appDemoIsTmSupported(dev,/*OUT*/&isTmSupported);
        if (rc != GT_OK)
        {
            return rc;
        }

        if(isTmSupported == GT_TRUE && !isCustomPortsMapping &&
           (boardRevId == 2 || boardRevId == 4 || boardRevId == 12)) /* for TM configs only */
        {
            /*
                when system is initialized with non default ports mapping,
                TM Ports Init is called after custom ports mapping is applied
            */
            rc = appDemoBc2TmPortsInit(dev);
            if (rc != GT_OK)
            {
                return rc;
            }
        }
        else
        {
            if(PRV_CPSS_DXCH_PP_MAC(dev)->hwInfo.gop_ilkn.supported == GT_TRUE)
            {
                GT_PHYSICAL_PORT_NUM    chId;


                /* add ILKN channels to default VLAN, although they are not mapped yet */
                for(chId = 128; chId < 192; chId++)
                {
                    /* set the port as member of vlan */
                    CPSS_PORTS_BMP_PORT_SET_MAC(&portsAdditionalMembers, chId);
                }
            }

            if(isTmSupported == GT_TRUE)
            {
                /*------------------------------------------------------------------------------------------------------------------------
                 *  /Cider/EBU/Bobcat2B/Bobcat2 {Current}/Reset and Init Controller/DFX Server Units - BC2 specific registers/Device SAR2
                 *  address : 0x000F8204
                 *  15-17  PLL_2_Config (Read-Only) TM clock frequency
                 *    0x0 = Disabled;  Disabled; TM clock is disabled 
                 *    0x1 = 400MHz;    TM runs 400MHz, DDR3 runs 800MHz 
                 *    0x2 = 466MHz;    TM runs 466MHz, DDR3 runs 933MHz 
                 *    0x3 = 333MHz;    TM runs 333MHz, DDR3 runs 667MHz 
                 *    0x5 = Reserved0; TM runs from core clock, DDR3 runs 800MHz.; (TM design does not support DDR and TM_core from different source) 
                 *    0x6 = Reserved1; TM runs from core clock, DDR3 runs 933MHz.; (TM design does not support DDR and TM_core from different source) 
                 *    0x7 = Reserved; Reserved; PLL bypass 	
                 *
                 *    if SAR2 - PLL2  is 0 (disable)  , do we need to configure TM ????
                 *
                 *------------------------------------------------------------------------------------------------------------------------*/

                /* enable TM regs access*/
                rc= prvCpssTmCtlGlobalParamsInit(dev);
                if (rc)
                {
                    cpssOsPrintf("%s %d  ERROR : rc=%d\n ",__FILE__, __LINE__ , rc);
                    return rc;
                }
            }
            else
            {
                if(boardRevId == 2 || boardRevId == 4 || boardRevId == 12)
                    cpssOsPrintf("Dev: %d is not supported TM, has initialized without TM!\n", dev);
            }
        }

        /* add port 83 to default VLAN, although it could be not mapped */
        CPSS_PORTS_BMP_PORT_SET_MAC(&portsAdditionalMembers, 83);

        /* read VLAN entry */
        rc = cpssDxChBrgVlanEntryRead(dev, 1, &portsMembers, &portsTagging,
                                      &cpssVlanInfo, &isValid, &portsTaggingCmd);
        CPSS_ENABLER_DBG_TRACE_RC_MAC("cpssDxChBrgVlanEntryRead", rc);
        if (rc != GT_OK)
           return rc;

        /* add new ports as members, portsTaggingCmd is default - untagged */
        CPSS_PORTS_BMP_BITWISE_OR_MAC(&portsMembers, &portsMembers, &portsAdditionalMembers);

        /* Write default VLAN entry */
        rc = cpssDxChBrgVlanEntryWrite(dev, 1,
                                       &portsMembers,
                                       &portsTagging,
                                       &cpssVlanInfo,
                                       &portsTaggingCmd);

        CPSS_ENABLER_DBG_TRACE_RC_MAC("cpssDxChBrgVlanEntryWrite", rc);
        if (rc != GT_OK)
           return rc;

        rc = appDemoDbEntryGet("initSerdesDefaults", /*out */&initSerdesDefaults);
        if(rc != GT_OK)
        {
            initSerdesDefaults = 1;
        }

        if (initSerdesDefaults != 0) /* if initSerdesDefaults == 0 .i.e. skip port config, */
        {                            /*     than skip also phy config */
            rc = appDemoDbEntryGet("skipPhyInit", &skipPhyInit);
            if(rc != GT_OK)
            {
                skipPhyInit = 0;
            }
        }
        else
        {
            skipPhyInit = 1;
        }

        if(bc2BoardType == APP_DEMO_CETUS_BOARD_DB_CNS)
        {
            /* board without PHY */
            skipPhyInit = 1;
        }

        if(skipPhyInit != 1)
        {
            /* configure PHYs */
            rc = bobcat2BoardPhyConfig(boardRevId, dev);
            CPSS_ENABLER_DBG_TRACE_RC_MAC("boardPhyConfig", rc);
            if(rc != GT_OK)
            {
                return rc;
            }
        }

        /* PTP (and TAIs) configurations */
        rc = appDemoB2PtpConfig(dev);
        CPSS_ENABLER_DBG_TRACE_RC_MAC("prvBobcat2PtpConfig", rc);
        if (rc != GT_OK)
        {
            return rc;
        }

        if(PRV_CPSS_SIP_5_10_CHECK_MAC(dev))
        {
            /* TBD: FE HA-3259 fix and removed from CPSS.
               Allow to the CPU to get the original vlan tag as payload after
               the DSA tag , so the info is not changed. */
            rc = cpssDxChBrgVlanForceNewDsaToCpuEnableSet(dev, GT_TRUE);
            CPSS_ENABLER_DBG_TRACE_RC_MAC("cpssDxChBrgVlanForceNewDsaToCpuEnableSet", rc);
            if( GT_OK != rc )
            {
                return rc;
            }

            /* RM of Bridge default values of Command registers have fixed in B0.
               But some bits need to be changed to A0 values.
               set bits 15..17 in PRV_DXCH_REG1_UNIT_L2I_MAC(devNum).bridgeEngineConfig.bridgeCommandConfig0 */
            /* set the command of 'SA static moved' to be 'forward' because this command
               applied also on non security breach event ! */
            rc = cpssDxChBrgSecurBreachEventPacketCommandSet(dev,
                CPSS_BRG_SECUR_BREACH_EVENTS_MOVED_STATIC_E,
                CPSS_PACKET_CMD_FORWARD_E);
            CPSS_ENABLER_DBG_TRACE_RC_MAC("cpssDxChBrgSecurBreachEventPacketCommandSet", rc);
            if(rc != GT_OK)
            {
                return rc;
            }
        }

        /* Flow Control Initializations */
        for(portNum = 0; portNum < (appDemoPpConfigList[dev].maxPortNumber); portNum++)
        {
            CPSS_ENABLER_PORT_SKIP_CHECK(dev, portNum);

            rc = cpssDxChPortMacSaLsbSet(dev, portNum, (GT_U8)portNum);
            CPSS_ENABLER_DBG_TRACE_RC_MAC("cpssDxChPortMacSaLsbSet", rc);
        }

        if(PRV_CPSS_DXCH_PP_MAC(dev)->hwInfo.trafficManager.supported)
        {
            /* Configure PFC response mode to TXQ in order:
                - to be backward compatible
                - to fix default registers value
            */
            rc = cpssDxChTmGluePfcResponseModeSet(dev, CPSS_DXCH_TM_GLUE_PFC_RESPONSE_MODE_TXQ_E);
            CPSS_ENABLER_DBG_TRACE_RC_MAC("cpssDxChTmGluePfcResponseModeSet", rc);
        }

        /* PLR MRU : needed for bobk that hold default different then our tests expect */
        /* NOTE: for bobk it is not good value for packets > (10K/8) bytes */
        for(i = CPSS_DXCH_POLICER_STAGE_INGRESS_0_E;
            i <= CPSS_DXCH_POLICER_STAGE_EGRESS_E ;
            i++)
        {
            rc = cpssDxCh3PolicerMruSet(dev,i,_10K);
            CPSS_ENABLER_DBG_TRACE_RC_MAC("cpssDxCh3PolicerMruSet", rc);
        }

#if(defined _linux)
#ifdef CPU_ARMADAXP_3_4_69
#ifndef ASIC_SIMULATION

        /* Fix I2C access (for some random registers) from Armada XP card */
        rc = cpssDrvPpHwInternalPciRegRead(dev, 0, 0x1108c, &regValue);
        CPSS_ENABLER_DBG_TRACE_RC_MAC("cpssDrvPpHwInternalPciRegRead", rc);

        /* Set '0' to bit_18 */
        U32_SET_FIELD_MAC(regValue, 18, 1, 0);

        rc = cpssDrvPpHwInternalPciRegWrite(dev, 0, 0x1108c, regValue);
        CPSS_ENABLER_DBG_TRACE_RC_MAC("cpssDrvPpHwInternalPciRegWrite", rc);
#endif
#endif
#endif
    }

    return GT_OK;
}


/*******************************************************************************
* afterInitBoardConfigSimple
*
* DESCRIPTION:
*       Board specific configurations that should be done after board
*       initialization.
*
* INPUTS:
*       boardRevId      - The board revision Id.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK   - on success,
*       GT_FAIL - otherwise.
*
* COMMENTS:
*
*       This is a simplified version of afterInitBoardConfig.
*
*******************************************************************************/
static GT_STATUS afterInitBoardConfigSimple
(
    IN  GT_U8       dev,
    IN  GT_U8       boardRevId
)
{
    GT_STATUS   rc;

    PRV_CPSS_DXCH_BC2_UNUSED_PARAM_MAC(boardRevId);

    if (CPSS_SYSTEM_RECOVERY_PROCESS_NOT_ACTIVE_E ==
                                    systemRecoveryInfo.systemRecoveryProcess)
    {
        rc = appDemoBc2PortInterfaceInit(dev, boardRevId);
        CPSS_ENABLER_DBG_TRACE_RC_MAC("afterInitBoardConfigSimple", rc);
        if(rc != GT_OK)
            return rc;
    }

    if(boardRevId == 2 || boardRevId == 4 || boardRevId == 5 || boardRevId == 12) /* for TM configs only */
    {
        appDemoBc2DramOrTmInit(dev, 0);

        osPrintf("build TM tree after init\n");

        /* TM related configurations */
        rc = prvBobcat2TmGlueConfig(dev);
        CPSS_ENABLER_DBG_TRACE_RC_MAC("prvBobcat2TmGlueConfig", rc);
        if (rc != GT_OK)
            return rc;

        rc = prvBobcat2TmPort73Config(dev);
        if(rc != GT_OK)
        {
            return rc;
        }
    }
#if 0
    rc = appDemoDbEntryGet("initSerdesDefaults", /*out */&initSerdesDefaults);
    if(rc != GT_OK)
    {
        initSerdesDefaults = 1;
    }
#endif
    /* configure PHYs */
    rc = bobcat2BoardPhyConfig(boardRevId, dev);
    CPSS_ENABLER_DBG_TRACE_RC_MAC("boardPhyConfig", rc);
    if(rc != GT_OK)
    {
        return rc;
    }

    if(PRV_CPSS_SIP_5_10_CHECK_MAC(dev))
    {
        /* TBD: FE HA-3259 fix and removed from CPSS.
           Allow to the CPU to get the original vlan tag as payload after
           the DSA tag , so the info is not changed. */
        rc = cpssDxChBrgVlanForceNewDsaToCpuEnableSet(dev, GT_TRUE);
        CPSS_ENABLER_DBG_TRACE_RC_MAC("cpssDxChBrgVlanForceNewDsaToCpuEnableSet", rc);
        if( GT_OK != rc )
        {
            CPSS_ENABLER_DBG_TRACE_RC_MAC("cpssDxChBrgVlanForceNewDsaToCpuEnableSet", rc);
            return rc;
        }

        /* RM of Bridge default values of Command registers have fixed in B0.
           But some bits need to be changed to A0 values.
           set bits 15..17 in PRV_DXCH_REG1_UNIT_L2I_MAC(devNum).bridgeEngineConfig.bridgeCommandConfig0 */
        /* set the command of 'SA static moved' to be 'forward' because this command
           applied also on non security breach event ! */
        rc = cpssDxChBrgSecurBreachEventPacketCommandSet(dev,
            CPSS_BRG_SECUR_BREACH_EVENTS_MOVED_STATIC_E,
            CPSS_PACKET_CMD_FORWARD_E);
        CPSS_ENABLER_DBG_TRACE_RC_MAC("cpssDxChBrgSecurBreachEventPacketCommandSet", rc);
        if(rc != GT_OK)
        {
            return rc;
        }
    }


    if(PRV_CPSS_DXCH_PP_MAC(dev)->hwInfo.trafficManager.supported)
    {
        /* Configure PFC response mode to TXQ in order:
            - to be backward compatible
            - to fix default registers value
        */
        rc = cpssDxChTmGluePfcResponseModeSet(dev, CPSS_DXCH_TM_GLUE_PFC_RESPONSE_MODE_TXQ_E);
        CPSS_ENABLER_DBG_TRACE_RC_MAC("cpssDxChTmGluePfcResponseModeSet", rc);
        if(rc != GT_OK)
        {
            return rc;
        }
    }

    /* set VLAN 1 for TM check - if you don't want - comment it out */
    {
        CPSS_PORTS_BMP_STC  portsMembers;
        CPSS_PORTS_BMP_STC  portsTagging;
        CPSS_DXCH_BRG_VLAN_INFO_STC  cpssVlanInfo;   /* cpss vlan info format    */
        CPSS_DXCH_BRG_VLAN_PORTS_TAG_CMD_STC portsTaggingCmd; /* ports tagging command */
        GT_U8               port;           /* current port number      */

        /* Fill Vlan info */
        osMemSet(&cpssVlanInfo, 0, sizeof(cpssVlanInfo));
        /* default IP MC VIDX */
        cpssVlanInfo.unregIpmEVidx = 0xFFF;
        cpssVlanInfo.naMsgToCpuEn           = GT_TRUE;
        cpssVlanInfo.floodVidx              = 0xFFF;
        cpssVlanInfo.fidValue = 1;

        /* Fill ports and tagging members */
        CPSS_PORTS_BMP_PORT_CLEAR_ALL_MAC(&portsMembers);
        CPSS_PORTS_BMP_PORT_CLEAR_ALL_MAC(&portsTagging);
        osMemSet(&portsTaggingCmd, 0, sizeof(CPSS_DXCH_BRG_VLAN_PORTS_TAG_CMD_STC));

        /* set all ports as VLAN members */
        for (port = 0; port < 48; port++)
        {
            CPSS_ENABLER_PORT_SKIP_CHECK(dev,port);

            /* set the port as member of vlan */
            CPSS_PORTS_BMP_PORT_SET_MAC(&portsMembers, port);

            /* Set port pvid */
            rc = cpssDxChBrgVlanPortVidSet(dev, port, CPSS_DIRECTION_INGRESS_E,1);
            CPSS_ENABLER_DBG_TRACE_RC_MAC("cpssDxChBrgVlanPortVidSet", rc);
            if(rc != GT_OK)
                 return rc;

            portsTaggingCmd.portsCmd[port] = CPSS_DXCH_BRG_VLAN_PORT_UNTAGGED_CMD_E;
        }

        /* Write default VLAN entry */
        rc = cpssDxChBrgVlanEntryWrite(dev, 1,
                                       &portsMembers,
                                       &portsTagging,
                                       &cpssVlanInfo,
                                       &portsTaggingCmd);

        CPSS_ENABLER_DBG_TRACE_RC_MAC("cpssDxChBrgVlanEntryWrite", rc);
        if (rc != GT_OK)
           return rc;

    }


    return GT_OK;
}



/*******************************************************************************
* gtDbDxBobcat2BoardCleanDbDuringSystemReset
*
* DESCRIPTION:
 *      clear the DB of the specific board config file , as part of the 'system rest'
 *      to allow the 'cpssInitSystem' to run again as if it is the first time it runs
*
* INPUTS:
*       boardRevId      - The board revision Id.
*
* OUTPUTS:
*
*
* RETURNS:
*       GT_OK   - on success,
*       GT_FAIL - otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static GT_STATUS gtDbDxBobcat2BoardCleanDbDuringSystemReset
(
    IN  GT_U8   boardRevId
)
{
    /* next static are and maybe changed from compilation time */
    ppCounter = 0;
    bc2BoardType = APP_DEMO_BC2_BOARD_DB_CNS;
    bc2BoardResetDone = GT_TRUE;

    return GT_OK;
}


/*******************************************************************************
* bcat2AppDemoPortsConfig
*
* DESCRIPTION:
*     Init required ports of Bobcat2 to specified ifMode and speed
*
* INPUTS:
*       dev - device number
*       ifMode - port interface mode
*       speed - port speed
*       powerUp -   GT_TRUE - port power up
*                   GT_FALSE - serdes power down
*       numOfPorts - quantity of ports to configure
*       ... - numbers of ports to configure
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK   - on success,
*       GT_FAIL - otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
GT_STATUS bcat2AppDemoPortsConfig
(
    IN  GT_U8                           dev,
    IN  CPSS_PORT_INTERFACE_MODE_ENT    ifMode,
    IN  CPSS_PORT_SPEED_ENT             speed,
    IN  GT_BOOL                         powerUp,
    IN  GT_U32                          numOfPorts,
    ...
)
{
    GT_STATUS   rc;
    GT_U32      i;                  /* interator */
    GT_PHYSICAL_PORT_NUM portNum;   /* port number */
    va_list     ap;                 /* arguments list pointer */
    CPSS_PORTS_BMP_STC initPortsBmp,/* bitmap of ports to init */
                        *initPortsBmpPtr;/* pointer to bitmap */
    CPSS_DXCH_PORT_MAP_STC    portMap;  /* port mapping structure */

    initPortsBmpPtr = &initPortsBmp;
    CPSS_PORTS_BMP_PORT_CLEAR_ALL_MAC(initPortsBmpPtr);

    va_start(ap, numOfPorts);
    for(i = 1; i <= numOfPorts; i++)
    {
        portNum = va_arg(ap, GT_U32);
        rc = cpssDxChPortPhysicalPortMapGet(dev, portNum,
                                            1, /* portMapArraySize */
                                            &portMap);
        if (rc != GT_OK)
        {
            cpssOsPrintSync("bcat2AppDemoPortsConfig:cpssDxChPortPhysicalPortMapGet(%d):rc=%d\n",
                            portNum, rc);
            return rc;
        }
        CPSS_PORTS_BMP_PORT_SET_MAC(initPortsBmpPtr,portNum);
    }
    va_end(ap);

    rc = cpssDxChPortModeSpeedSet(dev, initPortsBmp, powerUp, ifMode, speed);
    if(rc != GT_OK)
    {
        cpssOsPrintSync("bcat2AppDemoPortsConfig:cpssDxChPortModeSpeedSet:rc=%d\n",
                        rc);
        return rc;
    }

    return GT_OK;
}

/*******************************************************************************
* appDemoTmScenarioModeSet
*
* DESCRIPTION:
*       set tm scenario for 29,2
*
* APPLICABLE DEVICES:
*        Bobcat2; Caelum; Bobcat3.
*
* NOT APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; Lion; Lion2; xCat2.
*
*
* INPUTS:
*       mode   - tm scenario mode
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
GT_VOID appDemoTmScenarioModeSet
(
    IN  CPSS_TM_SCENARIO  mode
)
{
    appDemoTmScenarioMode=mode;
}


/*------------------------------------------------------------------------------------------------------------------------
 *  /Cider/EBU/Bobcat2B/Bobcat2 {Current}/Reset and Init Controller/DFX Server Units - BC2 specific registers/Device SAR2
 *  address : 0x000F8204
 *  15-17  PLL_2_Config (Read-Only) TM clock frequency
 *    0x0 = Disabled;  Disabled; TM clock is disabled 
 *    0x1 = 400MHz;    TM runs 400MHz, DDR3 runs 800MHz 
 *    0x2 = 466MHz;    TM runs 466MHz, DDR3 runs 933MHz 
 *    0x3 = 333MHz;    TM runs 333MHz, DDR3 runs 667MHz 
 *    0x5 = Reserved0; TM runs from core clock, DDR3 runs 800MHz.; (TM design does not support DDR and TM_core from different source) 
 *    0x6 = Reserved1; TM runs from core clock, DDR3 runs 933MHz.; (TM design does not support DDR and TM_core from different source) 
 *    0x7 = Reserved; Reserved; PLL bypass 	
 *
 *    if SAR2 - PLL2  is 0 (disable)  , do we need to configure TM ????
 *    is TM supported ? probably not !!!
 *------------------------------------------------------------------------------------------------------------------------*/

GT_STATUS appDemoIsTmSupported(GT_U8 devNum, GT_BOOL * isSupportedPtr)
{
    GT_STATUS rc;
    GT_BOOL isSupported;
    GT_U32  tmFreq;

    if(!appDemoPpConfigList[devNum].valid)
    {
        osPrintf("appDemoIsTmSupported: wrong devNum: %d\n", devNum);
        isSupported =  GT_FALSE;
    }
    else
    {
        isSupported = PRV_CPSS_DXCH_PP_MAC(devNum)->fineTuning.featureInfo.TmSupported;
    }
    if (isSupported == GT_TRUE)
    {
        extern GT_STATUS prvCpssDxChTMFreqGet(IN  GT_U8    devNum, OUT GT_U32 * tmFreqPtr);

        rc = prvCpssDxChTMFreqGet(devNum,/*OUT*/&tmFreq);
        if (rc != GT_OK)
        {
            return rc;
        }
        if (tmFreq == 0)
        {
            isSupported =  GT_FALSE;
        }
    }
    *isSupportedPtr = isSupported;
    return GT_OK;
}

GT_STATUS portsConfigTest(GT_VOID)
{
    GT_STATUS   rc;
    GT_U32  portNum;
    CPSS_PORTS_BMP_STC portsBmp;
    GT_U32  i;

    CPSS_PORTS_BMP_PORT_CLEAR_ALL_MAC(&portsBmp);
    for(portNum = 0; portNum < 48; portNum++)
    {
        CPSS_PORTS_BMP_PORT_SET_MAC(&portsBmp, portNum);
    }

    for(i = 0; i < 50; i++)
    {
        rc = cpssDxChPortModeSpeedSet(0, portsBmp, GT_TRUE,
                                      CPSS_PORT_INTERFACE_MODE_QSGMII_E,
                                      CPSS_PORT_SPEED_1000_E);
        if(rc != GT_OK)
        {
            cpssOsPrintf("!!!qsgmii failed!!!\n");
            return rc;
        }
    }

    return GT_OK;
}


/*******************************************************************************
* cpssInitSimpleBobcat2
*
* DESCRIPTION:
*       This is the main board initialization function for BC2 device.
*
* INPUTS:
*       boardRevId    - Board revision Id.

* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK   - on success,
*       GT_FAIL - otherwise.
*
* COMMENTS:
*       This is a simplified version of cpssInitSystem.
*
*******************************************************************************/

static GT_STATUS cpssInitSimpleBobcat2
(
    IN  GT_U8  boardRevId
)
{
    GT_STATUS               rc = GT_OK;
    GT_U8                   numOfPp;        /* Number of Pp's in system.                */
    GT_U32      start_sec  = 0;
    GT_U32      start_nsec = 0;
    GT_U32      end_sec  = 0;
    GT_U32      end_nsec = 0;
    GT_U32      diff_sec;
    GT_U32      diff_nsec;

    GT_U8                   devNum = 0;
    GT_U8                   hwDevNum = 16;
    GT_PHYSICAL_PORT_NUM    portNum;
    CPSS_INTERFACE_INFO_STC physicalInfo;
    GT_32                   intKey;     /* Interrupt key     */
    CPSS_DXCH_PP_PHASE1_INIT_INFO_STC   cpssPpPhase1Info;     /* CPSS phase 1 PP params */
    CPSS_PP_DEVICE_TYPE     devType;
    static CPSS_REG_VALUE_INFO_STC regCfgList[] = GT_DUMMY_REG_VAL_INFO_LIST;            /* register values */
    GT_U32                  regCfgListSize;         /* Number of config functions for this board    */
    CPSS_DXCH_PP_PHASE2_INIT_INFO_STC   cpssPpPhase2Info;
    CPSS_DXCH_PP_CONFIG_INIT_STC    ppLogInitParams;
    GT_U32  numOfEports, numOfPhysicalPorts;

    GT_U32  secondsStart, secondsEnd,
            nanoSecondsStart, nanoSecondsEnd,
            seconds, nanoSec; /* time of init */

    rc = cpssOsTimeRT(&secondsStart,&nanoSecondsStart);
    CPSS_ENABLER_DBG_TRACE_RC_MAC("cpssOsTimeRT", rc);
    if(rc != GT_OK)
        return rc;

#if (defined ASIC_SIMULATION) &&  (defined RTOS_ON_SIM)
    /*simulation initialization*/
    appDemoRtosOnSimulationInit();

#endif /*(defined ASIC_SIMULATION) &&  (defined RTOS_ON_SIM)*/

    /* Enable printing inside interrupt routine - supplied by extrernal drive */
    extDrvUartInit();

    /* Call to fatal_error initialization, use default fatal error call_back - supplied by mainOs */
    rc = osFatalErrorInit((FATAL_FUNC_PTR)NULL);
    if (rc != GT_OK)
        return rc;
    osMemSet(&cpssPpPhase1Info, 0, sizeof(cpssPpPhase1Info));

    /* this function finds all Prestera devices on PCI bus */
    getBoardInfoSimple(boardRevId,
        &numOfPp,
        &cpssPpPhase1Info);
    if (rc != GT_OK)
        return rc;

    /* take time from the 'phase1 init' stage (not including the 'PCI scan' operations) */
    cpssOsTimeRT(&start_sec, &start_nsec);

    rc = getPpPhase1ConfigSimple(boardRevId, devNum, &cpssPpPhase1Info);
    if (rc != GT_OK)
        return rc;

    osPrintf("cpssDxChHwPpPhase1Init\n");
    /* devType is retrieved in hwPpPhase1Part1*/
    rc = cpssDxChHwPpPhase1Init(&cpssPpPhase1Info, &devType);
    CPSS_ENABLER_DBG_TRACE_RC_MAC("cpssDxChHwPpPhase1Init", rc);
    if (rc != GT_OK)
        return rc;

    /* should be 10 */
    regCfgListSize = (sizeof(regCfgList)) /
                       (sizeof(CPSS_REG_VALUE_INFO_STC));

    rc = cpssDxChHwPpStartInit(devNum, GT_FALSE, regCfgList, regCfgListSize);
    if (rc != GT_OK)
        return rc;

    rc = configBoardAfterPhase1Simple(boardRevId, devNum);
    if (rc != GT_OK)
        return rc;

    /* memory related data, such as addresses and block lenghts, are set in this funtion*/
    rc = getPpPhase2ConfigSimple(boardRevId, devNum, devType, &cpssPpPhase2Info);
    if (rc != GT_OK)
        return rc;

    /* Lock the interrupts, this phase changes the interrupts nodes pool data */
    extDrvSetIntLockUnlock(INTR_MODE_LOCK, &intKey);

    rc = cpssDxChHwPpPhase2Init(devNum,  &cpssPpPhase2Info);
    if (rc != GT_OK)
        return rc;


    rc = cpssDxChCfgHwDevNumSet(cpssPpPhase2Info.newDevNum, hwDevNum);
    if (rc != GT_OK)
        return rc;

    /* this is close to application job, if you want - you may remove this part*/
    /* set all hwDevNum in E2Phy mapping table for all ePorts */
    /* Enable configuration of drop for ARP MAC SA mismatch due to check per port */

    /* Loop on the first 256 (num of physical ports , and CPU port (63)) entries of the table */
    numOfPhysicalPorts =
        PRV_CPSS_DXCH_PP_HW_MAX_VALUE_OF_PHY_PORT_MAC(cpssPpPhase2Info.newDevNum)+1;

    for(portNum=0; portNum < numOfPhysicalPorts; portNum++)
    {
        rc = cpssDxChBrgEportToPhysicalPortTargetMappingTableGet(cpssPpPhase2Info.newDevNum,
                                                                 portNum,
                                                                 &physicalInfo);
        if(rc != GT_OK)
            return rc;

        /* If <Target Is Trunk> == 0 && <Use VIDX> == 0 */
        if(physicalInfo.type == CPSS_INTERFACE_PORT_E)
        {
            physicalInfo.devPort.hwDevNum = hwDevNum;
            rc = cpssDxChBrgEportToPhysicalPortTargetMappingTableSet(cpssPpPhase2Info.newDevNum,
                                                                     portNum,
                                                                     &physicalInfo);
            CPSS_ENABLER_DBG_TRACE_RC_MAC("cpssDxChBrgEportToPhysicalPortTargetMappingTableSet", rc);
            if(rc != GT_OK)
                return rc;

            /* ARP MAC SA mismatch check per port configuration enabling */
            rc = cpssDxChBrgGenPortArpMacSaMismatchDropEnable(cpssPpPhase2Info.newDevNum,
                                                              portNum,
                                                              GT_TRUE);
            CPSS_ENABLER_DBG_TRACE_RC_MAC("cpssDxChBrgGenPortArpMacSaMismatchDropEnable", rc);
            if(rc != GT_OK)
                return rc;
        }
    }

    /* Port Isolation is enabled if all three configurations are enabled:
       In the global TxQ registers, AND
       In the eVLAN egress entry, AND
       In the ePort entry

       For legacy purpose loop on all ePort and Trigger L2 & L3 Port
       Isolation filter for all ePorts */

        numOfEports = PRV_CPSS_DXCH_MAX_PORT_NUMBER_MAC(cpssPpPhase2Info.newDevNum);
    for(portNum=0; portNum < numOfEports; portNum++)
    {
        rc = cpssDxChNstPortIsolationModeSet(cpssPpPhase2Info.newDevNum,
                                             portNum,
                                             CPSS_DXCH_NST_PORT_ISOLATION_ALL_ENABLE_E);
        CPSS_ENABLER_DBG_TRACE_RC_MAC("cpssDxChNstPortIsolationModeSet", rc);
        if(rc != GT_OK)
            return rc;

        /* for legacy : enable per eport <Egress eVLAN Filtering Enable>
           because Until today there was no enable bit, egress VLAN filtering is
           always performed, subject to the global <BridgedUcEgressFilterEn>. */
        rc = cpssDxChBrgVlanPortEgressVlanFilteringEnableSet(cpssPpPhase2Info.newDevNum,
                                                          portNum,
                                                          GT_TRUE);
        CPSS_ENABLER_DBG_TRACE_RC_MAC("cpssDxChBrgVlanPortEgressVlanFilteringEnableSet", rc);
        if(rc != GT_OK)
            return rc;
    }

    extDrvSetIntLockUnlock(INTR_MODE_UNLOCK, &intKey);

/* we forced it to be DB board, change it for a different one if needed */
    rc = configBoardAfterPhase2Simple(BOARD_REV_ID_DB_E, devNum);
    if(rc != GT_OK)
        return rc;

    osMemSet(&ppLogInitParams ,0, sizeof(ppLogInitParams));
    rc = getPpLogicalInitParamsSimple(boardRevId, devNum, &ppLogInitParams);
    if(rc != GT_OK)
        return rc;

    rc = cpssDxChCfgPpLogicalInit(devNum, &ppLogInitParams);
    if(rc != GT_OK)
        return rc;

/* LIBs to initialize - use these functions as refference
    prvIpLibInit
    prvPclLibInit
    prvTcamLibInit
    */
 /*   getTapiLibInitParams
    localLibInitParams.initTcam                  = GT_TRUE;
    localLibInitParams.initNst                   = GT_TRUE;
 */

/* SMI init is a must in order to configure PHY's */
    rc = cpssDxChPhyPortSmiInit(devNum);
    if( rc != GT_OK)
        return rc;

/* specific board features configurations - see appDemoDxPpGeneralInit, afterInitBoardConfig,
appDemoEventRequestDrvnModeInit for reference*/

    /* Enable ports */
    for (portNum = 0; portNum < 0x53; portNum++)
    {
        CPSS_ENABLER_PORT_SKIP_CHECK(devNum, portNum);

        rc = cpssDxChPortEnableSet(devNum, portNum, GT_TRUE);
        if (GT_OK != rc)
            return rc;
    }

    /* Enable device */
    rc = cpssDxChCfgDevEnable(devNum, GT_TRUE);
    if (GT_OK != rc)
        return rc;

    /* after board init configurations are relevant only for TM case */
    if(boardRevId == 6)
    {
        rc = afterInitBoardConfigSimple(devNum, 2);
        if (GT_OK != rc)
            return rc;
    }

    /* CPSSinit time measurement */

    rc = cpssOsTimeRT(&secondsEnd,&nanoSecondsEnd);
    if(rc != GT_OK)
    {
        return rc;
    }

    seconds = secondsEnd-secondsStart;
    if(nanoSecondsEnd >= nanoSecondsStart)
    {
        nanoSec = nanoSecondsEnd-nanoSecondsStart;
    }
    else
    {
        nanoSec = (1000000000 - nanoSecondsStart) + nanoSecondsEnd;
        seconds--;
    }
    cpssOsPrintf("cpssInitSystem time: %d sec., %d nanosec.\n", seconds, nanoSec);


    cpssOsTimeRT(&end_sec, &end_nsec);
    if(end_nsec < start_nsec)
    {
        end_nsec += 1000000000;
        end_sec  -= 1;
    }
    diff_sec  = end_sec  - start_sec;
    diff_nsec = end_nsec - start_nsec;

    cpssOsPrintf("Time processing the cpssInitSimple (from 'phase1 init') is [%d] seconds + [%d] nanoseconds \n" , diff_sec , diff_nsec);

    return rc;
}


/*******************************************************************************
* gtDbDxBobcat2BoardReg
*
* DESCRIPTION:
*       Registration function for the BobCat2 (SIP5) board .
*
* INPUTS:
*       boardRevId      - The board revision Id.
*
* OUTPUTS:
*       boardCfgFuncs   - Board configuration functions.
*
* RETURNS:
*       GT_OK   - on success,
*       GT_FAIL - otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
GT_STATUS gtDbDxBobcat2BoardReg
(
    IN  GT_U8                   boardRevId,
    OUT GT_BOARD_CONFIG_FUNCS   *boardCfgFuncs
)
{
    PRV_CPSS_DXCH_BC2_UNUSED_PARAM_MAC(boardRevId);

    if(boardCfgFuncs == NULL)
    {
        return GT_FAIL;
    }

    if((boardRevId == 5) || (boardRevId == 6) )
    {
        boardCfgFuncs->boardSimpleInit =  cpssInitSimpleBobcat2;
        return GT_OK;
    }

    boardCfgFuncs->boardGetInfo                 = getBoardInfo;
    boardCfgFuncs->boardGetPpPh1Params          = getPpPhase1Config;
    boardCfgFuncs->boardAfterPhase1Config       = configBoardAfterPhase1;
    boardCfgFuncs->boardGetPpPh2Params          = getPpPhase2Config;
    boardCfgFuncs->boardAfterPhase2Config       = configBoardAfterPhase2;
    boardCfgFuncs->boardGetPpLogInitParams      = getPpLogicalInitParams;
    boardCfgFuncs->boardGetLibInitParams        = getTapiLibInitParams;
    boardCfgFuncs->boardAfterInitConfig         = afterInitBoardConfig;
    boardCfgFuncs->boardCleanDbDuringSystemReset= gtDbDxBobcat2BoardCleanDbDuringSystemReset;

    return GT_OK;
}


