/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
********************************************************************************
* appDemoDxChDmaUtils.c
*
* DESCRIPTION:
*       App demo DxCh DMA API.
*
* FILE REVISION NUMBER:
*       $Revision: 1 $
*******************************************************************************/

#include <cpss/generic/cpssTypes.h>
#include <cpss/generic/init/cpssInit.h>
#include <cpss/extServices/cpssExtServices.h>

#include <appDemo/sysHwConfig/gtAppDemoSysConfig.h>
#include <appDemo/boardConfig/appDemoBoardConfig.h>
#include <appDemo/sysHwConfig/gtAppDemoSysConfigDefaults.h>
#include <appDemo/boardConfig/appDemoCfgMisc.h>
#include <cpss/dxCh/dxChxGen/networkIf/cpssDxChNetIf.h>
#include <extUtils/trafficEngine/tgfTrafficGenerator.h>
#include <cpss/generic/port/cpssPortStat.h>
#include <cpss/dxCh/dxChxGen/port/private/prvCpssDxChPort.h>
#include <cpss/dxCh/dxChxGen/config/private/prvCpssDxChInfo.h>
#include <cpssCommon/private/prvCpssMath.h>
#include <cpss/dxCh/dxChxGen/mirror/cpssDxChMirror.h>
#include <cpss/dxCh/dxChxGen/port/macCtrl/prvCpssDxChMacCtrl.h>
#include <cpss/dxCh/dxChxGen/port/private/prvCpssDxChPortCtrl.h>




/* include the external services */
#include <cmdShell/common/cmdExtServices.h>

/*16 bytes alignment for tx descriptor*/
#define TX_DESC_DMA_ALIGN_BYTES_CNS  16
/*128 bytes alignment for packets*/
#define PACKET_DMA_ALIGN_BYTES_CNS  128
/*12 bytes of mac DA,SA*/
#define MAC_ADDR_LENGTH_CNS         12

/* max number of words in entry */
#define MAX_ENTRY_SIZE_CNS  32

/* array of MAC counters offsets */
/* gtMacCounterOffset[0] - offsets for not XGMII interface */
static GT_U8 gtMacCounterOffset[CPSS_LAST_MAC_COUNTER_NUM_E];

/*******************************************************************************
* sendPacketByTxDescriptor_HardCodedRegAddr
*
* DESCRIPTION:
*       The function put the descriptor in the queue and then trigger the queue
*
* APPLICABLE DEVICES: All.
*
* INPUTS:
*       devNum     - device number
*       txDesc     - (pointer to) The descriptor used for that packet.
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK           - on success
*       GT_FAIL         - on error
*
* COMMENTS:
*   This function is written for pexOnly mode --> these is no register initialization
*   All register addresses are hard coded
*******************************************************************************/
static GT_STATUS  sendPacketByTxDescriptor_HardCodedRegAddr
(
    GT_U8                   devNum,
    PRV_CPSS_TX_DESC_STC    *txDescPtr
)
{
    GT_STATUS rc;
    GT_U32  portGroupId = 0;
    GT_UINTPTR  physicalAddrOfTxDescPtr;         /* Holds the real buffer pointer.       */
    GT_U32 queue = 0;

    /* 1. put the descriptor in the queue */
    rc = cpssOsVirt2Phy((GT_UINTPTR)txDescPtr,/*OUT*/&physicalAddrOfTxDescPtr);
    if (rc != GT_OK)
    {
        return rc;
    }

    #if __WORDSIZE == 64    /* phyAddr must fit in 32 bit */
        if (0 != (physicalAddrOfTxDescPtr & 0xffffffff00000000L))
        {
            return GT_OUT_OF_RANGE;
        }
    #endif

    rc = prvCpssHwPpPortGroupWriteRegister(devNum,portGroupId,
        0x000026C0 + (4*queue), /*addrPtr->sdmaRegs.txDmaCdp[queue]*/
        (GT_U32)physicalAddrOfTxDescPtr);
    if (rc != GT_OK)
    {
        return rc;
    }

    /* The Enable DMA operation should be done only */
    /* AFTER all desc. operations where completed.  */
    GT_SYNC;

    /* 2. trigger the queue */

    /* Enable the Tx DMA.   */
    rc = prvCpssDrvHwPpPortGroupWriteRegister(devNum,portGroupId,
        0x00002868,/*addrPtr->sdmaRegs.txQCmdReg*/
        (1<<queue));

    return GT_OK;
}

/*******************************************************************************
* portStatInit_HardCodedRegAddr
*
* DESCRIPTION:
*       Init port statistics counter.
*
* INPUTS:
*       devNum   - physical device number
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK                    - on success
*       GT_FAIL                  - on error
*       GT_BAD_PARAM             - on wrong port number or device
*       GT_HW_ERROR              - on hardware error
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*   This function is written for pexOnly mode --> these is no register initialization
*   All register addresses are hard coded
*******************************************************************************/
static GT_STATUS portStatInit_HardCodedRegAddr
(
    IN  GT_U8       devNum
)
{
    GT_U8  i;

    PRV_CPSS_DXCH_DEV_CHECK_MAC(devNum);

    gtMacCounterOffset[CPSS_GOOD_OCTETS_RCV_E]  = 0x0;
    gtMacCounterOffset[CPSS_BAD_OCTETS_RCV_E]   = 0x8;
    gtMacCounterOffset[CPSS_GOOD_OCTETS_SENT_E] = 0x38;
    gtMacCounterOffset[CPSS_GOOD_UC_PKTS_RCV_E] = 0x10;
    gtMacCounterOffset[CPSS_GOOD_UC_PKTS_SENT_E] = 0x40;
    gtMacCounterOffset[CPSS_MULTIPLE_PKTS_SENT_E] = 0x50;
    gtMacCounterOffset[CPSS_DEFERRED_PKTS_SENT_E] = 0x14;

    for (i = CPSS_MAC_TRANSMIT_ERR_E; i <= CPSS_GOOD_OCTETS_SENT_E; i++)
        gtMacCounterOffset[i] = (GT_U8)(0x4 + (i * 4));

    for (i = CPSS_GOOD_OCTETS_SENT_E + 1; i < CPSS_GOOD_UC_PKTS_RCV_E; i++)
        gtMacCounterOffset[i] = (GT_U8)(0x8 + (i * 4));

    return GT_OK;
}

/*******************************************************************************
* appDemoDxChMacCounterGet_HardCodedRegAddr
*
* DESCRIPTION:
*       Gets Ethernet MAC counter /  MAC Captured counter for a
*       specified port on specified device.
*
* INPUTS:
*       devNum         - physical device number
*       portNum        - physical port number,
*                        CPU port if getFromCapture is GT_FALSE
*       cntrName       - specific counter name
*       getFromCapture -  GT_TRUE -  Gets the captured Ethernet MAC counter
*                         GT_FALSE - Gets the Ethernet MAC counter
*
* OUTPUTS:
*       cntrValuePtr - (pointer to) current counter value.
*
* RETURNS:
*       GT_OK        - on success
*       GT_FAIL      - on error
*       GT_BAD_PARAM - on wrong port number, device or counter name
*       GT_HW_ERROR  - on hardware error
*       GT_BAD_PTR   - one of the parameters is NULL pointer
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*     The 10G MAC MIB counters are 64-bit wide.
*     Not supported counters: CPSS_BAD_PKTS_RCV_E, CPSS_UNRECOG_MAC_CNTR_RCV_E,
*     CPSS_BadFC_RCV_E, CPSS_GOOD_PKTS_RCV_E, CPSS_GOOD_PKTS_SENT_E.
*
*     This function is written for pexOnly mode --> these is no register initialization
*     All register addresses are hard coded
*******************************************************************************/
GT_STATUS appDemoDxChMacCounterGet_HardCodedRegAddr
(
    IN  GT_U8                       devNum,
    IN  GT_PHYSICAL_PORT_NUM        portNum,
    IN  CPSS_PORT_MAC_COUNTERS_ENT  cntrName,
    OUT GT_U64                      *cntrValuePtr
)
{
    GT_U32 regAddr;         /* register address */
    GT_U32 baseRegAddr;     /* base register address */
    GT_U32 portGroupId;/*the port group Id - support multi-port-groups device */
    GT_STATUS rc;      /* return code */

    /* convert the 'Physical port' to portGroupId,local port -- supporting multi-port-groups device */
    portGroupId = 0;

    CPSS_NULL_PTR_CHECK_MAC(cntrValuePtr);

    /* init table */
    rc = portStatInit_HardCodedRegAddr(devNum);
    if (rc != GT_OK)
    {
        return rc;
    }

    cntrValuePtr->l[0] = 0;
    cntrValuePtr->l[1] = 0;

    if ((PRV_CPSS_PP_MAC(devNum)->devFamily == CPSS_PP_FAMILY_DXCH_LION_E)||
        (CPSS_PP_FAMILY_DXCH_XCAT3_E == PRV_CPSS_PP_MAC(devNum)->devFamily))
    {
        /*PRV_CPSS_DXCH_DEV_REGS_MAC(devNum)->macRegs.perPortRegs[portMacNum].macCounters; */
        baseRegAddr = (0x09000000 + portNum * 0x20000);
    }
    else
    {
        return GT_NOT_SUPPORTED;
    }

    regAddr = baseRegAddr + gtMacCounterOffset[cntrName];

    if ( prvCpssDrvHwPpPortGroupReadRegister(devNum, portGroupId,
            regAddr, &(cntrValuePtr->l[0])) != GT_OK)
        return GT_HW_ERROR;


    switch (cntrName)
    {
        case CPSS_GOOD_OCTETS_RCV_E:
        case CPSS_GOOD_OCTETS_SENT_E:
            /* this counter has 64 bits */
            regAddr = regAddr + 4;
            if ( prvCpssDrvHwPpPortGroupReadRegister(devNum, portGroupId,
                    regAddr, &(cntrValuePtr->l[1])) != GT_OK)
                return GT_HW_ERROR;
            break;

        default:
            break;
    }

    return GT_OK;
}

/*******************************************************************************
* appDemoDxChCfgIngressDropCntrGet_HardCodedRegAddr
*
* DESCRIPTION:
*       Get the Ingress Drop Counter value.
*
* INPUTS:
*       devNum      - Device number.
*
* OUTPUTS:
*       counterPtr  - (pointer to) Ingress Drop Counter value
*
* RETURNS:
*       GT_OK                    - on success.
*       GT_HW_ERROR              - on hardware error.
*       GT_BAD_PARAM             - on wrong devNum.
*       GT_BAD_PTR               - on NULL pointer
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*   This function is written for pexOnly mode --> these is no register initialization
*   All register addresses are hard coded
*******************************************************************************/
GT_STATUS appDemoDxChCfgIngressDropCntrGet_HardCodedRegAddr
(
    IN  GT_U8       devNum,
    OUT GT_U32      *counterPtr
)
{
    GT_U32      regAddr;     /* register address */

    /* validate the pointer */
    CPSS_NULL_PTR_CHECK_MAC(counterPtr);

    if(PRV_CPSS_DXCH_PP_HW_INFO_E_ARCH_SUPPORTED_MAC(devNum) == GT_TRUE)
    {
        regAddr = 0x0D000040;
            /*PRV_DXCH_REG1_UNIT_EQ_MAC(devNum).ingrDropCntr.ingrDropCntr; */
    }
    else if(CPSS_PP_FAMILY_DXCH_XCAT3_E == PRV_CPSS_PP_MAC(devNum)->devFamily)
    {
        regAddr = 0x14000040;
            /*PRV_DXCH_REG1_UNIT_EQ_MAC(devNum).ingrDropCntr.ingrDropCntr; */
    }
    else
    {
        regAddr = 0x0B000040;
            /*PRV_CPSS_DXCH_DEV_REGS_MAC(devNum)->bufferMng.eqBlkCfgRegs.ingressDropCntrReg; */
    }

    return prvCpssPortGroupsBmpCounterSummary(devNum, CPSS_PORT_GROUP_UNAWARE_MODE_CNS,
                                                       regAddr, 0, 32,
                                                       counterPtr, NULL);
}


/*******************************************************************************
* mirrorToAnalyzerForwardingModeGet_HardCodedRegAddr
*
* DESCRIPTION:
*       Get Forwarding mode to Analyzer for egress/ingress mirroring.
*
* INPUTS:
*      devNum    - device number.
*
* OUTPUTS:
*      modePtr   - pointer to mode of forwarding To Analyzer packets.
*
* RETURNS:
*       GT_OK                    - on success.
*       GT_BAD_PARAM             - wrong device.
*       GT_HW_ERROR              - on writing to HW error.
*       GT_BAD_PTR               - on NULL pointer
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*   This function is written for pexOnly mode --> these is no register initialization
*   All register addresses are hard coded
*
*******************************************************************************/
static GT_STATUS mirrorToAnalyzerForwardingModeGet_HardCodedRegAddr
(
    IN  GT_U8     devNum,
    OUT CPSS_DXCH_MIRROR_TO_ANALYZER_FORWARDING_MODE_ENT   *modePtr
)
{
    GT_U32      regAddr;      /* register address */
    GT_U32      regData;      /* register data */
    GT_STATUS   rc;           /* return status */

    /* getting register address */
    if(PRV_CPSS_DXCH_PP_HW_INFO_E_ARCH_ENABLED_MAC(devNum) == GT_TRUE)
    {
        regAddr = 0x0D00B000; /* PRV_DXCH_REG1_UNIT_EQ_MAC(devNum).mirrToAnalyzerPortConfigs.analyzerPortGlobalConfig;*/
    }
    else if(CPSS_PP_FAMILY_DXCH_XCAT3_E == PRV_CPSS_PP_MAC(devNum)->devFamily)
    {
        regAddr = 0x1400B040; /* PRV_DXCH_REG1_UNIT_EQ_MAC(devNum).mirrToAnalyzerPortConfigs.analyzerPortGlobalConfig; */
    }
    else
    {
        regAddr = 0x0B00B040; /* PRV_CPSS_DXCH_DEV_REGS_MAC(devNum)-> bufferMng.eqBlkCfgRegs.analyzerPortGlobalConfig; */
    }

    rc = prvCpssHwPpGetRegField(devNum, regAddr, 0, 2, &regData);
    if(rc != GT_OK)
    {
        return rc;
    }

    if(PRV_CPSS_DXCH_PP_HW_INFO_E_ARCH_ENABLED_MAC(devNum) == GT_TRUE)
    {
        switch(regData)
        {
            case 0:
                *modePtr = CPSS_DXCH_MIRROR_TO_ANALYZER_FORWARDING_END_TO_END_E ;
                break;
            case 1:
                *modePtr = CPSS_DXCH_MIRROR_TO_ANALYZER_FORWARDING_HOP_BY_HOP_E ;
                break;
            case 2:
                *modePtr = CPSS_DXCH_MIRROR_TO_ANALYZER_FORWARDING_SOURCE_BASED_OVERRIDE_E ;
                break;
            default:
                return GT_BAD_PARAM;
        }
    }
    else
    {
        switch(regData)
        {
            case 0:
                *modePtr = CPSS_DXCH_MIRROR_TO_ANALYZER_FORWARDING_SOURCE_BASED_OVERRIDE_E ;
                break;
            case 3:
                *modePtr = CPSS_DXCH_MIRROR_TO_ANALYZER_FORWARDING_HOP_BY_HOP_E;
                break;
            default:
                return GT_BAD_PARAM;
        }
    }

    return rc;
}

/*******************************************************************************
* appDemoDxChMirrorRxPortSet_HardCodedRegAddr
*
* DESCRIPTION:
*       Sets a specific port to be Rx mirrored port.
*
* INPUTS:
*       devNum         - the device number
*       mirrPort       - port number, CPU port supported.
*       enable         - enable/disable Rx mirror on this port
*                        GT_TRUE - Rx mirroring enabled, packets
*                                  received on a mirrPort are
*                                  mirrored to Rx analyzer.
*                        GT_FALSE - Rx mirroring disabled.
*       index          - Analyzer destination interface index. (APPLICABLE RANGES: 0..6)
*                        Supported for xCat and above device.
*                        Used only if forwarding mode to analyzer is Source-based.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK                    - on success.
*       GT_BAD_PARAM             - wrong device, mirrPort.
*       GT_OUT_OF_RANGE          - index is out of range.
*       GT_HW_ERROR              - on writing to HW error.
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*   This function is written for pexOnly mode --> these is no register initialization
*   All register addresses are hard coded
*******************************************************************************/
GT_STATUS appDemoDxChMirrorRxPortSet_HardCodedRegAddr
(
    IN  GT_U8           devNum,
    IN  GT_PORT_NUM     mirrPort,
    IN  GT_BOOL         enable,
    IN  GT_U32          index
)
{
    GT_U32      data;        /* data from Ports VLAN and QoS table entry */
    GT_U32      offset;      /* offset in VLAN and QoS table entry */
    GT_STATUS   rc = GT_OK;  /* function call return value */
    GT_U32      portGroupId;/*the port group Id - support multi-port-groups device */
/*    GT_U8       localPort;  *//* local port - support multi-port-groups device */
    CPSS_DXCH_MIRROR_TO_ANALYZER_FORWARDING_MODE_ENT mode; /* forwarding mode */
/*    GT_U32      regIndex; *//* the index of Port Ingress Mirror Index register */
/*    GT_U32      regAddr;  *//* pp memory address for hw access*/
    GT_U32      entryMemoBufArr[MAX_ENTRY_SIZE_CNS];
    GT_U32      address;    /* address to write to */
    GT_U32      entrySize;  /* table entry size in words */

    portGroupId = 0;

    if(PRV_CPSS_DXCH_XCAT_FAMILY_CHECK_MAC(devNum))
    {
        /* Get Analyzer forwarding mode */
        rc = mirrorToAnalyzerForwardingModeGet_HardCodedRegAddr(devNum, &mode);
        if(rc != GT_OK)
        {
            return rc;
        }
        if (mode != CPSS_DXCH_MIRROR_TO_ANALYZER_FORWARDING_HOP_BY_HOP_E)
        {
            return GT_NOT_SUPPORTED;
        }
        offset = 7;
    }
    else
    {
        offset = 23;
    }

    /* For xCat and above hop-by-hop forwarding mode and other DxCh devices */
    if(PRV_CPSS_DXCH_PP_HW_INFO_E_ARCH_ENABLED_MAC(devNum) == GT_FALSE)
    {
        data = BOOL2BIT_MAC(enable);

        /* configure the Ports VLAN and QoS configuration entry,
           enable MirrorToIngressAnalyzerPort field */

        /* In xCat and above devices the data is updated only when the last */
        /* word in the entry was written. */

        if(CPSS_PP_FAMILY_DXCH_XCAT3_E == PRV_CPSS_PP_MAC(devNum)->devFamily)
        {
            address = 0x16001000 + 0*0x10; /* tablePtr->baseAddress + entryIndex * tablePtr->step */
        }
        else
        {
            address = 0x1001000 + 0*0x10; /* tablePtr->baseAddress + entryIndex * tablePtr->step */
        }

        entrySize = 3;

        /* read whole entry */
        rc = prvCpssDrvHwPpPortGroupReadRam(devNum, portGroupId, address, entrySize, entryMemoBufArr);
        if (rc != GT_OK)
        {
            return rc;
        }

        /* update field */
        U32_SET_FIELD_MAC(entryMemoBufArr[0], offset, 1 /* length */, data);

        /* write whole entry */
        rc = prvCpssHwPpPortGroupWriteRam(devNum, portGroupId, address, entrySize, entryMemoBufArr);
        if (rc != GT_OK)
        {
            return rc;
        }

    }

    return rc;
}

/*******************************************************************************
* setOwnDevice_HardCodedRegAddr
*
* DESCRIPTION:
*       Sets the device Device_ID within a Prestera chipset.
*
* INPUTS:
*       devNum         - the device number
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK                    - on success.
*       GT_BAD_PARAM             - wrong device, mirrPort.
*       GT_OUT_OF_RANGE          - index is out of range.
*       GT_HW_ERROR              - on writing to HW error.
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*   This function is written for pexOnly mode --> these is no register initialization
*   All register addresses are hard coded
*******************************************************************************/
GT_STATUS setOwnDevice_HardCodedRegAddr
(
    IN  GT_U8           devNum
)
{
    GT_STATUS rc;

     /* set 'own device' to be 0x10 */
    rc = prvCpssHwPpSetRegField(devNum,0x58,4,5,0x10);
    if(rc != GT_OK)
        return rc;

    return GT_OK;
}

/*******************************************************************************
* appDemoDxChMirrorAnalyzerInterfaceSet_HardCodedRegAddr
*
* DESCRIPTION:
*       This function sets analyzer interface.
*
* INPUTS:
*      devNum         - device number.
*      index          - index of analyzer interface. (APPLICABLE RANGES: 0..6)
*      interfacePtr   - Pointer to analyzer interface.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK                    - on success.
*       GT_BAD_PARAM             - wrong device, index, interface type.
*       GT_HW_ERROR              - on writing to HW error.
*       GT_BAD_PTR               - on NULL pointer
*       GT_OUT_OF_RANGE          - on wrong port or device number in interfacePtr.
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*   This function is written for pexOnly mode --> these is no register initialization
*   All register addresses are hard coded
*
*******************************************************************************/
GT_STATUS appDemoDxChMirrorAnalyzerInterfaceSet_HardCodedRegAddr
(
    IN GT_U8     devNum,
    IN GT_U32    index,
    IN CPSS_DXCH_MIRROR_ANALYZER_INTERFACE_STC   *interfacePtr
)
{
    GT_STATUS   rc = GT_OK;
    GT_U32      regAddr;         /* register address */
    GT_U32      regData = 0;         /* register data */
    /*GT_U32      hwDev, hwPort;*/
 /*   GT_U32      dataLength;*/

    CPSS_NULL_PTR_CHECK_MAC(interfacePtr);


    if(PRV_CPSS_DXCH_PP_HW_INFO_E_ARCH_SUPPORTED_MAC(devNum) == GT_TRUE)
    {
        #if 0
        hwDev = interfacePtr->interface.devPort.hwDevNum;
        hwPort = interfacePtr->interface.devPort.portNum;

        if(hwPort > PRV_CPSS_DXCH_PP_HW_MAX_VALUE_OF_E_PORT_MAC(devNum) ||
           hwDev  > PRV_CPSS_DXCH_PP_HW_MAX_VALUE_OF_HW_DEV_NUM_MAC(devNum))
        {
            return GT_OUT_OF_RANGE;
        }

        regData = (hwDev | hwPort << 10);

        if(PRV_CPSS_DXCH_BOBCAT2_A0_CHECK_MAC(devNum) == GT_FALSE)
        {
            dataLength = 25;
        }
        else
        {
            dataLength = 23;
        }

        /* getting register address */
        regAddr = PRV_DXCH_REG1_UNIT_EQ_MAC(devNum).
                mirrToAnalyzerPortConfigs.mirrorInterfaceParameterReg[index];

        rc = prvCpssHwPpSetRegField(devNum, regAddr, 0, dataLength, regData);
        #endif

    }
    else
    {

        /* set 1 bit for MonitorType (value = 0 incase of portType)
           5 bits for devNum and 13 bits for portNum */
        U32_SET_FIELD_IN_ENTRY_MAC(&regData,0,2,0);
        U32_SET_FIELD_IN_ENTRY_MAC(&regData,2,5,interfacePtr->interface.devPort.hwDevNum);
        U32_SET_FIELD_IN_ENTRY_MAC(&regData,7,13,interfacePtr->interface.devPort.portNum);

        /* getting register address */
        if(CPSS_PP_FAMILY_DXCH_XCAT3_E == PRV_CPSS_PP_MAC(devNum)->devFamily)
        {
            regAddr = 0x1400B020 + index * 4;
        }
        else
        {
            regAddr = 0x0B00B020 + index * 4;
        }

        /*PRV_CPSS_DXCH_DEV_REGS_MAC(devNum)->
        bufferMng.eqBlkCfgRegs.mirrorInterfaceParameterReg[index];*/

        rc = prvCpssHwPpSetRegField(devNum, regAddr, 0, 20, regData);
    }

    return rc;
}

/*******************************************************************************
* appDemoDxChMirrorRxGlobalAnalyzerInterfaceIndexSet_HardCodedRegAddr
*
* DESCRIPTION:
*       This function sets analyzer interface index, used for ingress
*       mirroring from all engines except
*       port-mirroring source-based-forwarding mode.
*       (Port-Based hop-by-hop mode, Policy-Based, VLAN-Based,
*        FDB-Based, Router-Based).
*       If a packet is mirrored by both the port-based ingress mirroring,
*       and one of the other ingress mirroring, the selected analyzer is
*       the one with the higher index.
*
* INPUTS:
*      devNum    - device number.
*      enable    - global enable/disable mirroring for
*                  Port-Based hop-by-hop mode, Policy-Based,
*                  VLAN-Based, FDB-Based, Router-Based.
*                  - GT_TRUE - enable mirroring.
*                  - GT_FALSE - No mirroring.
*      index     - Analyzer destination interface index. (APPLICABLE RANGES: 0..6)
*
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK                    - on success.
*       GT_BAD_PARAM             - wrong device.
*       GT_OUT_OF_RANGE          - index is out of range.
*       GT_HW_ERROR              - on writing to HW error.
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*   This function is written for pexOnly mode --> these is no register initialization
*   All register addresses are hard coded
*
*******************************************************************************/
GT_STATUS appDemoDxChMirrorRxGlobalAnalyzerInterfaceIndexSet_HardCodedRegAddr
(
    IN GT_U8     devNum,
    IN GT_BOOL   enable,
    IN GT_U32    index
)
{
    GT_U32      regAddr;     /* register address */
    GT_U32      regData;     /* register data */

    /* getting register address */
    if(PRV_CPSS_DXCH_PP_HW_INFO_E_ARCH_ENABLED_MAC(devNum) == GT_TRUE)
    {
        regAddr = 0x0D00B000; /*PRV_DXCH_REG1_UNIT_EQ_MAC(devNum).mirrToAnalyzerPortConfigs.analyzerPortGlobalConfig; */
    }
    else if(CPSS_PP_FAMILY_DXCH_XCAT3_E == PRV_CPSS_PP_MAC(devNum)->devFamily)
    {
        regAddr = 0x1400B040; /* PRV_DXCH_REG1_UNIT_EQ_MAC(devNum).mirrToAnalyzerPortConfigs.analyzerPortGlobalConfig; */
    }
    else
    {
        regAddr = 0x0B00B040; /*PRV_CPSS_DXCH_DEV_REGS_MAC(devNum)->bufferMng.eqBlkCfgRegs.analyzerPortGlobalConfig; */
    }

    /* 0 is used for no mirroring */
    regData = (enable == GT_TRUE) ? (index + 1) : 0;

    return prvCpssHwPpSetRegField(devNum, regAddr, 2, 3, regData);
}

/*******************************************************************************
* appDemoDxChIngressCscdPortSet_HardCodedRegAddr
*
* DESCRIPTION:
*           Set ingress port as cascaded/non-cascade .
*           (effect all traffic on ingress pipe only)
*
* INPUTS:
*       devNum   - physical device number
*       portNum  - The physical port number.
*                   NOTE: this port is not covered and is NOT checked for 'mapping'
*       portType - cascade  type regular/extended DSA tag port or network port
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK                    - on success
*       GT_BAD_PARAM             - on wrong port number or device
*       GT_HW_ERROR              - on hardware error
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*   This function is written for pexOnly mode --> these is no register initialization
*   All register addresses are hard coded
*
*******************************************************************************/
GT_STATUS appDemoDxChIngressCscdPortSet_HardCodedRegAddr
(
    IN GT_U8                        devNum,
    IN GT_PHYSICAL_PORT_NUM         portNum,
    IN CPSS_CSCD_PORT_TYPE_ENT      portType
)
{
/*    GT_STATUS       rc = GT_OK;*/
    GT_U32          value;       /* value of field */
    GT_U32          fieldOffset = 0; /* The start bit number in the register */
    GT_U32          regAddr = 0;     /* address of register */
    GT_U32          portGroupId = 0;      /*the port group Id - support multi port groups device */
    /*GT_U8           localPort;  */ /* local port - support multi-port-group device */

    /*GT_U32          portRxDmaNum = portNum;*/ /* The RXDMA number for cascading */
    /*CPSS_CSCD_PORT_TYPE_ENT portType = CPSS_CSCD_PORT_DSA_MODE_2_WORDS_E; *//* cascade  type regular DSA tag port */

    if(portType == CPSS_CSCD_PORT_NETWORK_E)
    {
        value = 0;
    }
    else
    {
        value = 1;
    }

    if(PRV_CPSS_DXCH_PP_HW_INFO_E_ARCH_SUPPORTED_MAC(devNum) == GT_TRUE)
    {
        #if 0
        /* Used to enable Multicast filtering over cascade ports even if the
           packet is assigned <Phy Src MC Filter En> = 0.
           This value should be set to 1 over cascade ports. */
        rc = prvCpssDxChHwEgfShtFieldSet(devNum,portNum,
            PRV_CPSS_DXCH_HW_EGF_SHT_FIELD_IGNORE_PHY_SRC_MC_FILTER_E,
                GT_TRUE, /*accessPhysicalPort*/
                GT_FALSE, /*accessEPort*/
                (portType == CPSS_CSCD_PORT_NETWORK_E) ? 0 : 1);

        if (rc != GT_OK)
        {
            return rc;
        }

        /* convert the 'Physical port' to portGroupId,local port -- supporting multi port group device */
        portGroupId = PRV_CPSS_GLOBAL_PORT_TO_PORT_GROUP_ID_CONVERT_MAC(devNum, portRxDmaNum);
        localPort = PRV_CPSS_GLOBAL_PORT_TO_LOCAL_PORT_CONVERT_MAC(devNum,portRxDmaNum);

        /* each RXDMA port hold it's own registers , so bit index is not parameter of port number */
        fieldOffset = 3;

        if(localPort >= (sizeof(PRV_DXCH_REG1_UNIT_RXDMA_MAC(devNum).singleChannelDMAConfigs.SCDMAConfig0) /
                         sizeof(PRV_DXCH_REG1_UNIT_RXDMA_MAC(devNum).singleChannelDMAConfigs.SCDMAConfig0[0])))
        {
            /* out of range value */
            return GT_BAD_PARAM;
        }

        regAddr = PRV_DXCH_REG1_UNIT_RXDMA_MAC(devNum).
            singleChannelDMAConfigs.SCDMAConfig0[localPort];
       #endif
    }
    else
    {
        fieldOffset = (portNum == CPSS_CPU_PORT_NUM_CNS) ? 15 : portNum;

        if(PRV_CPSS_PP_MAC(devNum)->devFamily >= CPSS_PP_FAMILY_DXCH_LION2_E)
        {
            regAddr = 0x0F000064;
            /* PRV_CPSS_DXCH_DEV_REGS_MAC(devNum)->cutThroughRegs.ctCascadingPortReg; */
        }
        else
        {
            regAddr = 0x0F000020;
            /* PRV_CPSS_DXCH_DEV_REGS_MAC(devNum)->cutThroughRegs.ctCascadingPortReg; */
        }
    }

    /* Enable/disable DSA tag recognition on rxDma of ingress port */
    return  prvCpssHwPpPortGroupSetRegField(devNum, portGroupId,
                               regAddr, fieldOffset, 1, value);
}

/*******************************************************************************
* waitForSendToEnd_HardCodedRegAddr
*
* DESCRIPTION:
*       The function waits until PP sent the packet.
*       DMA procedure may take a long time for Jumbo packets.
*
* INPUTS:
*       devNum     - device number
*       txDesc     - (pointer to) The descriptor used for that packet.
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK           - on success
*       GT_FAIL         - on error
*
* COMMENTS: copied from dxChNetIfSdmaTxPacketSend()
*   This function is written for pexOnly mode --> these is no register initialization
*   All register addresses are hard coded
*
*******************************************************************************/
static GT_STATUS  waitForSendToEnd_HardCodedRegAddr
(
    GT_U8                   devNum,
    PRV_CPSS_TX_DESC_STC    *txDescPtr
)
{
    GT_U32  numOfBufs = 1;
    GT_U32  numOfAddedBuffs = 0;

#ifndef ASIC_SIMULATION
    GT_U32 loopIndex = (numOfBufs + numOfAddedBuffs) * 104000;
#else
    GT_U32 loopIndex = (numOfBufs + numOfAddedBuffs) * 500;
    GT_U32 sleepTime;/*time to sleep , to allow the 'Asic simulation' process the packet */

    /* allow another task to process it if ready , without 1 millisecond penalty */
    /* this code of sleep 0 Vs sleep 1 boost the performance *20 in enhanced-UT !!! */
    sleepTime = 0;

    tryMore_lbl:
#endif /*ASIC_SIMULATION*/

    /* Wait until PP sent the packet. Or HW error if while block
               run more then loopIndex times */
    while (loopIndex && (TX_DESC_GET_OWN_BIT_MAC(devNum,txDescPtr) == TX_DESC_DMA_OWN))
    {
#ifdef ASIC_SIMULATION
        /* do some sleep allow the simulation process the packet */
        cpssOsTimerWkAfter(sleepTime);
#endif /*ASIC_SIMULATION*/
        loopIndex--;
    }

    if(loopIndex == 0)
    {
#ifdef ASIC_SIMULATION
        if(sleepTime == 0)/* the TX was not completed ? we need to allow more retries with 'sleep (1)'*/
        {
            loopIndex = (numOfBufs + numOfAddedBuffs) * 500;
            sleepTime = 1;
            goto tryMore_lbl;
        }
#endif /*ASIC_SIMULATION*/
        return GT_HW_ERROR;
    }

    return GT_OK;
}

/*******************************************************************************
* buildTxSdmaDescriptor
*
* DESCRIPTION:
*       The function build the Tx descriptor and then trigger the transmit.
*
* INPUTS:
*       txDesc          - (pointer to) The descriptor used for that packet.
*       buffArr         - (pointer to) packet data.
*       length          - packet data length.
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK           - on success
*       GT_FAIL         - on error
*
* COMMENTS:
*
*******************************************************************************/
static GT_STATUS buildTxSdmaDescriptor(
    IN PRV_CPSS_TX_DESC_STC    *txDescPtr,
    IN GT_U8                   *buffArr,
    IN GT_U32                   length
)
{
    GT_STATUS rc = GT_OK;
    GT_UINTPTR  physicalAddrOfBuffPtr;         /* Holds the real buffer pointer.       */

    /* Set the first descriptor parameters. */
    TX_DESC_RESET_MAC(txDescPtr);

    txDescPtr->word1 = (1 << 12);/* recalc CRC */

    /* Set bit for first buffer of a frame for Tx descriptor */
    TX_DESC_SET_FIRST_BIT_MAC(txDescPtr,1);
    /* single descriptor --> send the pull packet */
    TX_DESC_SET_BYTE_CNT_MAC(txDescPtr,length);
    /* Set bit for last buffer of a frame for Tx descriptor */
    TX_DESC_SET_LAST_BIT_MAC(txDescPtr,1);

    /* update the packet header to the first descriptor */
    rc = cpssOsVirt2Phy((GT_UINTPTR)buffArr,&physicalAddrOfBuffPtr);
    if (rc != GT_OK)
    {
        return rc;
    }

    #if __WORDSIZE == 64    /* phyAddr must fit in 32 bit */
    if (0 != (physicalAddrOfBuffPtr & 0xffffffff00000000L))
    {
        return GT_OUT_OF_RANGE;
    }
    #endif

    txDescPtr->buffPointer = CPSS_32BIT_LE((GT_U32)physicalAddrOfBuffPtr);

    /* Set the descriptor own bit to start transmitting.  */
    TX_DESC_SET_OWN_BIT_MAC(txDescPtr,TX_DESC_DMA_OWN);

    txDescPtr->word1  = CPSS_32BIT_LE(txDescPtr->word1);
    txDescPtr->word2  = CPSS_32BIT_LE(txDescPtr->word2);

    txDescPtr->nextDescPointer = 0;/* single descriptor without 'next' */

    return rc;

}
/*******************************************************************************
* setTxSdmaRegConfig_HardCodedRegAddr
*
* DESCRIPTION:
*       Set the needed values for SDMA registers to enable Tx activity.
*
* INPUTS:
*       devNum  - The Pp device number.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK   - on success,
*       GT_FAIL - otherwise.
*
* COMMENTS:
*   This function is written for pexOnly mode --> these is no register initialization
*   All register addresses are hard coded
*
*******************************************************************************/
static GT_STATUS setTxSdmaRegConfig_HardCodedRegAddr
(
    IN GT_U8 devNum
)
{
    GT_STATUS   rc; /* return code */
    GT_U32      portGroupId = 0;

    /********* Since working in SP the configure transmit queue WRR value to 0 ************/
    rc = prvCpssDrvHwPpPortGroupWriteRegister(devNum,portGroupId,
                                              0x00002708,/*addrPtr->sdmaRegs.txQWrrPrioConfig[queue]*/
                                              0);
    if(rc != GT_OK)
        return rc;

    /********* Tx SDMA Token-Bucket Queue<n> Counter ************/
    rc = prvCpssDrvHwPpPortGroupWriteRegister(devNum,portGroupId,
                                              0x00002700,/*addrPtr->sdmaRegs.txSdmaTokenBucketQueueCnt[queue]*/
                                              0);
    if(rc != GT_OK)
        return rc;

    /********** Tx SDMA Token Bucket Queue<n> Configuration ***********/
    rc = prvCpssDrvHwPpPortGroupWriteRegister(devNum,portGroupId,
                                              0x00002704,/*addrPtr->sdmaRegs.txSdmaTokenBucketQueueConfig[queue]*/
                                              0xfffffcff);
    if(rc != GT_OK)
        return rc;

    /*********************/
    rc = prvCpssDrvHwPpPortGroupWriteRegister(devNum,portGroupId,
                                              0x00002874,/*addrPtr->sdmaRegs.txSdmaWrrTokenParameters */
                                              0xffffffc1);
    if(rc != GT_OK)
        return rc;

    /*********** Set all queues to Fix priority **********/
    rc = prvCpssDrvHwPpPortGroupWriteRegister(devNum,portGroupId,
                                              0x00002870,/*addrPtr->sdmaRegs.txQFixedPrioConfig */
                                              0xFF);
    if(rc != GT_OK)
        return rc;

    /*** temp settings ****/
    /* Lion RM#2701: SDMA activation */
    /* the code must be before calling phase1Part4Init(...) because attempt
       to access register 0x2800 will cause the PEX to hang */
    rc = prvCpssDrvHwPpSetRegField(devNum,0x58,20,1,1);

    return rc;
}

/*******************************************************************************
* freesSdmaMemoryAllocation
*
* DESCRIPTION:
*       This function frees sdma memory allocation
*
* INPUTS:
*       buffArrMAllocPtr - pointer to array of alocated memory
*       txDescAllocPtr - pointer to descriptor
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
static GT_VOID  freesSdmaMemoryAllocation
(
    IN GT_U8 *buffArrMAllocPtr ,
    IN PRV_CPSS_TX_DESC_STC *txDescAllocPtr
)
{
    if(buffArrMAllocPtr)
    {
        cpssOsCacheDmaFree(buffArrMAllocPtr);
    }

    if(txDescAllocPtr)
    {
        cpssOsCacheDmaFree(txDescAllocPtr);
    }

    return;
}

/*******************************************************************************
* printPacket
*
* DESCRIPTION:
*       print packet
*
* INPUTS:
*       bufferPtr  - pointer to buffer of packet data.
*       length     - length of the packet data.
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
static GT_STATUS printPacket
(
    IN GT_U8           *bufferPtr,
    IN GT_U32           length
)
{
    GT_U32  iter = 0;

    CPSS_NULL_PTR_CHECK_MAC(bufferPtr);


    cpssOsPrintf("\nSending brodcast packet with DSA tag\n");

    for(iter = 0; iter < length; iter++)
    {
        if((iter & 0x0F) == 0)
        {
            cpssOsPrintf("0x%4.4x :", iter);
        }

        cpssOsPrintf(" %2.2x", bufferPtr[iter]);

        if((iter & 0x0F) == 0x0F)
        {
            cpssOsPrintf("\n");
        }
    }

    cpssOsPrintf("\n\n");

    return GT_OK;
}

/*******************************************************************************
* appDemoDxChPortEgressCntrsGet_HardCodedRegAddr
*
* DESCRIPTION:
*       Gets a egress counters from specific counter-set.
*
* INPUTS:
*       devNum        - physical device number
*       cntrSetNum - counter set number : 0, 1
*
* OUTPUTS:
*       egrCntrPtr - (pointer to) structure of egress counters current values.
*
* RETURNS:
*       GT_OK                    - on success
*       GT_FAIL                  - on error
*       GT_BAD_PARAM             - on wrong port number or device
*       GT_HW_ERROR              - on hardware error
*       GT_BAD_PTR               - one of the parameters is NULL pointer
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*   This function is written for pexOnly mode --> these is no register initialization
*   All register addresses are hard coded
*
*******************************************************************************/
GT_STATUS appDemoDxChPortEgressCntrsGet_HardCodedRegAddr
(
    IN  GT_U8                       devNum,
    IN  GT_U8                       cntrSetNum,
    OUT CPSS_PORT_EGRESS_CNTR_STC   *egrCntrPtr
)
{
    GT_U32 regAddr;         /* register address */

    CPSS_NULL_PTR_CHECK_MAC(egrCntrPtr);

    /* read Bridge Egress Filtered Packet Count Register */
    if(PRV_CPSS_DXCH_PP_HW_INFO_E_ARCH_SUPPORTED_MAC(devNum) == GT_TRUE)
    {
        regAddr = 0x3C093240+ (cntrSetNum*0x4);
            /*PRV_DXCH_REG1_UNIT_TXQ_Q_MAC(devNum).peripheralAccess.egrMIBCntrs.setBridgeEgrFilteredPktCntr[cntrSetNum];*/
    }
    else if(CPSS_PP_FAMILY_DXCH_XCAT3_E == PRV_CPSS_PP_MAC(devNum)->devFamily)
    {
        regAddr = 0x02B40150 + (cntrSetNum*0x20);
            /*PRV_CPSS_DXCH_DEV_REGS_MAC(devNum)->egrTxQConf.txQCountSet[cntrSetNum].brgEgrFiltered; */
    }
    else if(0 == PRV_CPSS_DXCH_PP_HW_INFO_TXQ_REV_1_OR_ABOVE_MAC(devNum))
    {
        regAddr = 0x01B40150 + (cntrSetNum*0x20);
            /*PRV_CPSS_DXCH_DEV_REGS_MAC(devNum)->egrTxQConf.txQCountSet[cntrSetNum].brgEgrFiltered; */
    }
    else
    {
        regAddr = 0xA093240 + (cntrSetNum*0x4);
            /*PRV_CPSS_DXCH_DEV_REGS_MAC(devNum)->txqVer1.queue.peripheralAccess.egressMibCounterSet.bridgeEgressFilteredPacketCounter[cntrSetNum];*/
    }

    if (prvCpssDrvHwPpPortGroupGetRegField(devNum, 0, regAddr,0,32, &egrCntrPtr->brgEgrFilterDisc) != GT_OK)
    {
        return GT_HW_ERROR;
    }

    /* read Transmit Queue Filtered Packet Count Register */
    if(PRV_CPSS_DXCH_PP_HW_INFO_E_ARCH_SUPPORTED_MAC(devNum) == GT_TRUE)
    {
        regAddr = 0x3C093250 + (cntrSetNum*0x4);
            /*PRV_DXCH_REG1_UNIT_TXQ_Q_MAC(devNum).peripheralAccess.egrMIBCntrs.setTailDroppedPktCntr[cntrSetNum];*/
    }
    else if(CPSS_PP_FAMILY_DXCH_XCAT3_E == PRV_CPSS_PP_MAC(devNum)->devFamily)
    {
        regAddr = 0x02B40154 + (cntrSetNum*0x20);
            /*PRV_CPSS_DXCH_DEV_REGS_MAC(devNum)->egrTxQConf.txQCountSet[cntrSetNum].txQFiltered; */
    }
    else if(0 == PRV_CPSS_DXCH_PP_HW_INFO_TXQ_REV_1_OR_ABOVE_MAC(devNum))
    {
        regAddr = 0x01B40154 + (cntrSetNum*0x20);
            /*PRV_CPSS_DXCH_DEV_REGS_MAC(devNum)->egrTxQConf.txQCountSet[cntrSetNum].txQFiltered; */
    }
    else
    {
        regAddr = 0xA093250 + (cntrSetNum*0x4);
            /*PRV_CPSS_DXCH_DEV_REGS_MAC(devNum)->txqVer1.queue.peripheralAccess.egressMibCounterSet.tailDroppedPacketCounter[cntrSetNum]; */
    }

    if (prvCpssDrvHwPpPortGroupGetRegField(devNum, 0, regAddr,0,32, &egrCntrPtr->txqFilterDisc) != GT_OK)
    {
        return GT_HW_ERROR;
    }

    if((PRV_CPSS_PP_MAC(devNum)->devFamily != CPSS_PP_FAMILY_CHEETAH_E) && (PRV_CPSS_DXCH_PP_HW_INFO_E_ARCH_SUPPORTED_MAC(devNum) != GT_TRUE))
    {
        /* read  egress forwarding restriction dropped Packets Count Register
           Supported from ch2 devices.*/
        if(CPSS_PP_FAMILY_DXCH_XCAT3_E == PRV_CPSS_PP_MAC(devNum)->devFamily)
        {
            regAddr = 0x02B4015C + (cntrSetNum*0x20);
                /*PRV_CPSS_DXCH_DEV_REGS_MAC(devNum)->egrTxQConf.txQCountSet[cntrSetNum].egrFrwDropPkts; */
        }
        else if(0 == PRV_CPSS_DXCH_PP_HW_INFO_TXQ_REV_1_OR_ABOVE_MAC(devNum))
        {
            regAddr = 0x01B4015C + (cntrSetNum*0x20);
                /*PRV_CPSS_DXCH_DEV_REGS_MAC(devNum)->egrTxQConf.txQCountSet[cntrSetNum].egrFrwDropPkts; */
        }
        else
        {
            regAddr = 0xA093270 + (cntrSetNum*0x4);
                /* PRV_CPSS_DXCH_DEV_REGS_MAC(devNum)->txqVer1.queue.peripheralAccess.egressMibCounterSet.
                            egressForwardingRestrictionDroppedPacketsCounter[cntrSetNum]; */
        }

        if (prvCpssDrvHwPpPortGroupGetRegField(devNum, 0, regAddr,0,32, &egrCntrPtr->egrFrwDropFrames) != GT_OK)
        {
            return GT_HW_ERROR;
        }
    }
    else
    {
        egrCntrPtr->egrFrwDropFrames = 0;
    }

    /* read Transmit Queue Filtered Packet Count Register */
    if(PRV_CPSS_DXCH_PP_HW_INFO_E_ARCH_SUPPORTED_MAC(devNum) == GT_TRUE)
    {
        regAddr = 0x3C093280 + (cntrSetNum*0x4);
            /*PRV_DXCH_REG1_UNIT_TXQ_Q_MAC(devNum).peripheralAccess.egrMIBCntrs.setMcFIFO3_0DroppedPktsCntr[cntrSetNum];*/

        if (prvCpssDrvHwPpPortGroupGetRegField(devNum, 0, regAddr,0,32, &egrCntrPtr->mcFifo3_0DropPkts) != GT_OK)
        {
            return GT_HW_ERROR;
        }

        regAddr = 0x0A093290 + (cntrSetNum*0x4);
            /*PRV_DXCH_REG1_UNIT_TXQ_Q_MAC(devNum).peripheralAccess.egrMIBCntrs.setMcFIFO7_4DroppedPktsCntr[cntrSetNum];*/

        if (prvCpssDrvHwPpPortGroupGetRegField(devNum, 0, regAddr,0,32, &egrCntrPtr->mcFifo7_4DropPkts) != GT_OK)
        {
            return GT_HW_ERROR;
        }

    }
    else
    {
        egrCntrPtr->mcFifo3_0DropPkts = 0;
        egrCntrPtr->mcFifo7_4DropPkts = 0;
    }

    #if 0
    if(GT_TRUE == PRV_CPSS_DXCH_ERRATA_GET_MAC(
                        devNum,
                        PRV_CPSS_DXCH_BOBCAT2_EGRESS_MIB_COUNTERS_NOT_ROC_WA_E))
     {
        egressCntrShadowPtr = &(PRV_CPSS_DXCH_PP_MAC(devNum)->errata.
                info_PRV_CPSS_DXCH_BOBCAT2_EGRESS_MIB_COUNTERS_NOT_ROC_WA_E.
                      egressCntrShadow[cntrSetNum]);

        egrCntrPtr->brgEgrFilterDisc -= egressCntrShadowPtr->brgEgrFilterDisc;
        egrCntrPtr->txqFilterDisc    -= egressCntrShadowPtr->txqFilterDisc;
        egrCntrPtr->egrFrwDropFrames -= egressCntrShadowPtr->egrFrwDropFrames;
        egrCntrPtr->mcFifo3_0DropPkts -= egressCntrShadowPtr->mcFifo3_0DropPkts;
        egrCntrPtr->mcFifo7_4DropPkts -= egressCntrShadowPtr->mcFifo7_4DropPkts;

        egressCntrShadowPtr->brgEgrFilterDisc += egrCntrPtr->brgEgrFilterDisc;
        egressCntrShadowPtr->txqFilterDisc    += egrCntrPtr->txqFilterDisc;
        egressCntrShadowPtr->egrFrwDropFrames += egrCntrPtr->egrFrwDropFrames;
        egressCntrShadowPtr->mcFifo3_0DropPkts += egrCntrPtr->mcFifo3_0DropPkts;
        egressCntrShadowPtr->mcFifo7_4DropPkts += egrCntrPtr->mcFifo7_4DropPkts;

     }
    #endif
    return GT_OK;
}

/*******************************************************************************
* appDemoDxChSdmaTxPacketSend_HardCodedRegAddr
*
* DESCRIPTION:
*       This function sends a single packet.
*       The packet is sent through interface port type to port given by
*       dstPortNum with VLAN vid.
*
* INPUTS:
*       isPacketWithVlanTag - indication that the packetDataPtr[12..15] hold vlan tag.
*       vid                 - VLAN ID. (used when isPacketWithVlanTag ==GT_FALSE,
*                             otherwise taken from packetDataPtr[14..15])
*       packetPtr           - pointer to the packet data and length in bytes.
*       dstPortNum          - Destination port number.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK   - on success,
*       GT_FAIL - otherwise.
*
* COMMENTS:
*   This function is written for pexOnly mode --> these is no register initialization
*   All register addresses are hard coded
*
*******************************************************************************/
GT_STATUS appDemoDxChSdmaTxPacketSend_HardCodedRegAddr
(
    IN GT_U8    devNum,
    IN GT_BOOL  isPacketWithVlanTag,
    IN GT_U8    vid,
    IN TGF_PACKET_PAYLOAD_STC  *packetPtr,
    IN GT_U32   dstPortNum
)
{
    GT_U8 *buffArr,*origBuffArrMAllocPtr = NULL;
    PRV_CPSS_TX_DESC_STC    *txDescPtr,*origTxDescAllocPtr = NULL;/* pointer to txDescriptor*/
    GT_U32 buffSize4alloc;
    CPSS_DXCH_NET_DSA_PARAMS_STC   dsaInfo;
    GT_U32  dsaTagNumOfBytes;   /* DSA tag length */
    GT_U32  dataOffset;/* the offset from start of packetPtr->dataLength till end of vlan tag if exists,
                        if not thet till end of macSa */
    GT_U32  totalLength;/* length of packet including the DSA that may replace the vlan tag */
    GT_STATUS rc = GT_OK;

    CPSS_NULL_PTR_CHECK_MAC(packetPtr);

    /***************************************************/
    /* set SDMA registers to enable Tx activity */
    setTxSdmaRegConfig_HardCodedRegAddr(devNum);

    /***************************************************/
    /* allocate descriptor */
    origTxDescAllocPtr = cpssOsCacheDmaMalloc(16/*length*/ + TX_DESC_DMA_ALIGN_BYTES_CNS/*alignment*/);
    txDescPtr = origTxDescAllocPtr;
    if (txDescPtr == NULL)
    {
        rc = GT_NO_RESOURCE;
        freesSdmaMemoryAllocation(origBuffArrMAllocPtr,origTxDescAllocPtr);
        return rc;
    }

    /* we use 16 bytes alignment */
    if((((GT_UINTPTR)txDescPtr) % TX_DESC_DMA_ALIGN_BYTES_CNS) != 0)
    {
        txDescPtr = (PRV_CPSS_TX_DESC_STC*)(((GT_UINTPTR)txDescPtr) +
                           (TX_DESC_DMA_ALIGN_BYTES_CNS - (((GT_UINTPTR)txDescPtr) % TX_DESC_DMA_ALIGN_BYTES_CNS)));
    }
    /***************************************************/

    /***************************************************/
    /* allocate buffer for packet */

    /* packet length is 128 bytes, and it must be 128-byte aligned */
    buffSize4alloc = (packetPtr->dataLength + PACKET_DMA_ALIGN_BYTES_CNS);

    /* save original allocation pointer ... needed when calling cpssOsFree() */
    origBuffArrMAllocPtr = cpssOsCacheDmaMalloc(buffSize4alloc);

    buffArr = origBuffArrMAllocPtr;
    if (buffArr == NULL)
    {
        rc = GT_NO_RESOURCE;
        freesSdmaMemoryAllocation(origBuffArrMAllocPtr,origTxDescAllocPtr);
        return rc;
    }
    /* we use 128 bytes alignment */
    if((((GT_UINTPTR)buffArr) % PACKET_DMA_ALIGN_BYTES_CNS) != 0)
    {
        buffArr = (GT_U8*)(((GT_UINTPTR)buffArr) +
                           (PACKET_DMA_ALIGN_BYTES_CNS - (((GT_UINTPTR)buffArr) % PACKET_DMA_ALIGN_BYTES_CNS)));
    }

    /***************************************************/
    /* copy packet from caller into buffer allocated in the SDMA */

    /* break to 2 parts : macDa,Sa and rest of the packet */
    cpssOsMemCpy(&buffArr[0],packetPtr->dataPtr,MAC_ADDR_LENGTH_CNS);/* 12 bytes of macDa,Sa */

    dsaTagNumOfBytes = 8;/* number of bytes in the DSA */

    if(isPacketWithVlanTag == GT_TRUE)
    {
        /* skip the 4 bytes of the vlan tag , as it is embedded inside the DSA */
        /* copy the reset of the packet without the vlan tag */
        dataOffset = MAC_ADDR_LENGTH_CNS + 4;/*16*/
    }
    else
    {
        dataOffset = MAC_ADDR_LENGTH_CNS;/*12*/
    }

    /* copy the reset of the packet  */
    cpssOsMemCpy(&buffArr[MAC_ADDR_LENGTH_CNS + dsaTagNumOfBytes] ,
        &packetPtr->dataPtr[dataOffset],
        (packetPtr->dataLength-dataOffset));

    totalLength = (packetPtr->dataLength - dataOffset) + /* length of 'rest of packet' */
                  (MAC_ADDR_LENGTH_CNS + dsaTagNumOfBytes);/*length of DSA + macDa,Sa */

    /***************************************************/
    /* Build Extended FROM_CPU DSA tag which is used also by legacy devices; 2 words */

    cpssOsMemSet(&dsaInfo,0,sizeof(dsaInfo));
    dsaInfo.commonParams.dsaTagType = CPSS_DXCH_NET_DSA_2_WORD_TYPE_ENT;
    if(isPacketWithVlanTag == GT_FALSE)
    {
        /* the buffer hold no vlan tag info */
        dsaInfo.commonParams.vid = vid;
    }
    else
    {
        /* take the 12 bits from the buffer */
        dsaInfo.commonParams.vid = (packetPtr->dataPtr[14] & 0x0F) << 8 |
                                    packetPtr->dataPtr[15];
        dsaInfo.commonParams.vpt = (packetPtr->dataPtr[14] >> 5) & 0x7;
        dsaInfo.commonParams.cfiBit = (packetPtr->dataPtr[14] >> 4) & 1;
    }
    dsaInfo.dsaType = CPSS_DXCH_NET_DSA_CMD_FROM_CPU_E;

    dsaInfo.dsaInfo.fromCpu.isTrgPhyPortValid = GT_TRUE;
    dsaInfo.dsaInfo.fromCpu.dstInterface.type = CPSS_INTERFACE_PORT_E;
    dsaInfo.dsaInfo.fromCpu.dstInterface.devPort.portNum = dstPortNum;
    dsaInfo.dsaInfo.fromCpu.dstInterface.devPort.hwDevNum = 0x10;

    dsaInfo.dsaInfo.fromCpu.srcHwDev = 0x1f;/* different than 'dstInterface.devPort.hwDevNum' */


    /***************************************************/
    /* put the DSA info into the buffer after 12 bytes of macDa,Sa*/
    rc = cpssDxChNetIfDsaTagBuild(devNum, &dsaInfo, &buffArr[MAC_ADDR_LENGTH_CNS]);
    if(rc != GT_OK)
    {
        freesSdmaMemoryAllocation(origBuffArrMAllocPtr,origTxDescAllocPtr);
        return rc;
    }

    printPacket(buffArr,totalLength);

    /***************************************************/
    /* fill the descriptor with proper info */
    rc = buildTxSdmaDescriptor(txDescPtr,buffArr,totalLength);
    if(rc != GT_OK)
    {
        freesSdmaMemoryAllocation(origBuffArrMAllocPtr,origTxDescAllocPtr);
        return rc;
    }

    /***************************************************/
    /* 1. put the descriptor in the queue */
    /* 2. trigger the queue */
    rc = sendPacketByTxDescriptor_HardCodedRegAddr(devNum,txDescPtr);
    if(rc != GT_OK)
    {
        freesSdmaMemoryAllocation(origBuffArrMAllocPtr,origTxDescAllocPtr);
        return rc;
    }

    /***************************************************/
    /* wait for the send of the packet to end (like synchronic send) */
    rc = waitForSendToEnd_HardCodedRegAddr(devNum,txDescPtr);
    if(rc != GT_OK)
    {
        freesSdmaMemoryAllocation(origBuffArrMAllocPtr,origTxDescAllocPtr);
        return rc;
    }

    return rc;
}

/*******************************************************************************
* appDemoDxChPortForceLinkPassEnableSet_HardCodedRegAddr
*
* DESCRIPTION:
*       Enable/disable Force Link Pass on specified port on specified device.
*
* INPUTS:
*       devNum   - physical device number
*       portNum  - physical port number (or CPU port)
*       state    - GT_TRUE for force link pass, GT_FALSE otherwise
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
*   This function is written for pexOnly mode --> these is no register initialization
*   All register addresses are hard coded
*
*******************************************************************************/
GT_STATUS appDemoDxChPortForceLinkPassEnableSet_HardCodedRegAddr
(
    IN  GT_U8     devNum,
    IN  GT_PHYSICAL_PORT_NUM     portNum,
    IN  GT_BOOL   state
)
{
    GT_U32  regAddr;    /* register address */
    GT_U32  value;      /* data to write to register */
    PRV_CPSS_DXCH_PORT_REG_CONFIG_STC   regDataArray[PRV_CPSS_PORT_NOT_APPLICABLE_E];
    PRV_CPSS_PORT_TYPE_ENT  portMacType; /* MAC unit of port */

    PRV_CPSS_DXCH_DEV_CHECK_MAC(devNum);

    value = BOOL2BIT_MAC(state);

    if(prvCpssDxChPortMacConfigurationClear(regDataArray) != GT_OK)
        return GT_INIT_ERROR;

    /*PRV_CPSS_DXCH_DEV_REGS_MAC(devNum)->macRegs.perPortRegs[portNum].autoNegCtrl; */
    if(PRV_CPSS_PP_MAC(devNum)->devFamily == CPSS_PP_FAMILY_DXCH_LION_E)
    {
        regAddr = 0x0A80000C + (portNum & 0xf) * 0x400;
    }
    else
    {
        regAddr = 0x0880000C + (portNum * 0x1000);
    }
    if(regAddr != PRV_CPSS_SW_PTR_ENTRY_UNUSED)
    {
        regDataArray[PRV_CPSS_PORT_GE_E].regAddr = regAddr;
        regDataArray[PRV_CPSS_PORT_GE_E].fieldData = value;
        regDataArray[PRV_CPSS_PORT_GE_E].fieldLength = 1;
        regDataArray[PRV_CPSS_PORT_GE_E].fieldOffset = 1;
    }

    /* PRV_CPSS_DXCH_DEV_REGS_MAC(devNum)->macRegs.perPortRegs[portNum].
                            macRegsPerType[PRV_CPSS_PORT_XG_E].macCtrl; */
    if(PRV_CPSS_PP_MAC(devNum)->devFamily == CPSS_PP_FAMILY_DXCH_LION_E)
    {
        regAddr = 0x08800000 + 0x0 + (portNum & 0xf) * 0x400;
    }
    else if(PRV_CPSS_PP_MAC(devNum)->devFamily == CPSS_PP_FAMILY_DXCH_XCAT3_E)
    {
        regAddr = 0x120C0000 + 0x0 + (portNum * 0x1000);
    }
    else
    {
        regAddr = 0x08800000 + 0x0 + (portNum * 0x400);
    }
    if(regAddr != PRV_CPSS_SW_PTR_ENTRY_UNUSED)
    {
        regDataArray[PRV_CPSS_PORT_XG_E].regAddr = regAddr;
        regDataArray[PRV_CPSS_PORT_XG_E].fieldData = value;
        regDataArray[PRV_CPSS_PORT_XG_E].fieldLength = 1;
        regDataArray[PRV_CPSS_PORT_XG_E].fieldOffset = 3;
    }

    /* PRV_CPSS_DXCH_DEV_REGS_MAC(devNum)->macRegs.perPortRegs[portNum].
                            macRegsPerType[PRV_CPSS_PORT_XLG_E].macCtrl; */
    if(PRV_CPSS_PP_MAC(devNum)->devFamily == CPSS_PP_FAMILY_DXCH_LION_E)
    {
        regAddr = PRV_CPSS_SW_PTR_ENTRY_UNUSED;
    }
    else if(PRV_CPSS_PP_MAC(devNum)->devFamily == CPSS_PP_FAMILY_DXCH_XCAT3_E)
    {
        regAddr = 0x120C0000 + 0x0 + (portNum * 0x1000);
    }
    else
    {
        regAddr = 0x08800000 + 0x0 + (portNum * 0x400);
    }
    if(regAddr != PRV_CPSS_SW_PTR_ENTRY_UNUSED)
    {
        regDataArray[PRV_CPSS_PORT_XLG_E].regAddr = regAddr;
        regDataArray[PRV_CPSS_PORT_XLG_E].fieldData = value;
        regDataArray[PRV_CPSS_PORT_XLG_E].fieldLength = 1;
        regDataArray[PRV_CPSS_PORT_XLG_E].fieldOffset = 3;
    }

    for(portMacType = PRV_CPSS_PORT_GE_E; portMacType < PRV_CPSS_PORT_NOT_APPLICABLE_E; portMacType++)
    {
        if(PRV_CPSS_DXCH_PP_MAC(devNum)->hwInfo.dedicatedCpuMac.isNotSupported == GT_FALSE)
        {
            if((portNum == CPSS_CPU_PORT_NUM_CNS) && (portMacType >= PRV_CPSS_PORT_XG_E))
                continue;
        }

        if(regDataArray[portMacType].regAddr != PRV_CPSS_SW_PTR_ENTRY_UNUSED)
        {
            if (prvCpssDrvHwPpPortGroupSetRegField(devNum, 0,
                                                    regDataArray[portMacType].regAddr,
                                                    regDataArray[portMacType].fieldOffset,
                                                    regDataArray[portMacType].fieldLength,
                                                    regDataArray[portMacType].fieldData) != GT_OK)
            {
                return GT_HW_ERROR;
            }
        }
    }
    return GT_OK;
}

/*******************************************************************************
* portPcsLoopbackEnableSet_HardCodedRegAddr
*
* DESCRIPTION:
*       Set the PCS Loopback state in the packet processor MAC port.
*
* INPUTS:
*       devNum    - physical device number
*       portNum   - physical port number
*       enable    - If GT_TRUE, enable loopback
*                   If GT_FALSE, disable loopback
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
*   This function is written for pexOnly mode --> these is no register initialization
*   All register addresses are hard coded
*
*******************************************************************************/
static GT_STATUS portPcsLoopbackEnableSet_HardCodedRegAddr
(
    IN  GT_U8       devNum,
    IN  GT_PHYSICAL_PORT_NUM portNum,
    IN  GT_BOOL     enable
)
{
    GT_U32 regAddr;     /* register address                    */
    GT_U32 value;       /* value to write into the register    */
    GT_U32          portMacNum;      /* MAC number */
    PRV_CPSS_DXCH_PORT_REG_CONFIG_STC   regDataArray[PRV_CPSS_PORT_NOT_APPLICABLE_E];
    PRV_CPSS_PORT_TYPE_ENT portMacType; /* MAC unit used by port */

    PRV_CPSS_DXCH_PORT_NUM_OR_CPU_PORT_CHECK_AND_MAC_NUM_GET_MAC(devNum,portNum,portMacNum);

    if(prvCpssDxChPortMacConfigurationClear(regDataArray) != GT_OK)
        return GT_INIT_ERROR;

    value = BOOL2BIT_MAC(enable);

    /* PRV_CPSS_DXCH_DEV_REGS_MAC(devNum)->macRegs.perPortRegs[portNum].
                            macRegsPerType[PRV_CPSS_PORT_GE_E].macCtrl1; */
    if(PRV_CPSS_PP_MAC(devNum)->devFamily == CPSS_PP_FAMILY_DXCH_LION_E)
    {
        regAddr = 0x0A800004 + 0x0 + (portNum & 0xf) * 0x400;
    }
    else if(PRV_CPSS_PP_MAC(devNum)->devFamily == CPSS_PP_FAMILY_DXCH_XCAT3_E)
    {
        regAddr = 0x12000004 + 0x0 + (portNum * 0x1000);
    }
    else
    {
        regAddr = 0x08800004 + 0x0 + (portNum * 0x400);
    }

    if(regAddr != PRV_CPSS_SW_PTR_ENTRY_UNUSED)
    {
        regDataArray[PRV_CPSS_PORT_GE_E].regAddr = regAddr;
        regDataArray[PRV_CPSS_PORT_GE_E].fieldData = value;
        regDataArray[PRV_CPSS_PORT_GE_E].fieldLength = 1;
        regDataArray[PRV_CPSS_PORT_GE_E].fieldOffset = 6;
    }

    /* PRV_CPSS_DXCH_DEV_REGS_MAC(devNum)->macRegs.perPortRegs[portNum].
                            macRegsPerType[PRV_CPSS_PORT_XG_E].macCtrl1; */
    if(PRV_CPSS_PP_MAC(devNum)->devFamily == CPSS_PP_FAMILY_DXCH_LION_E)
    {
        regAddr = 0x08800004 + 0x0 + (portNum & 0xf) * 0x400;
    }
    else if(PRV_CPSS_PP_MAC(devNum)->devFamily == CPSS_PP_FAMILY_DXCH_XCAT3_E)
    {
        regAddr = 0x120C0004 + 0x0 + (portNum * 0x1000);
    }
    else
    {
        regAddr = 0x08800004 + 0x0 + (portNum * 0x400);
    }
    if(regAddr != PRV_CPSS_SW_PTR_ENTRY_UNUSED)
    {
        regDataArray[PRV_CPSS_PORT_XG_E].regAddr = regAddr;
        regDataArray[PRV_CPSS_PORT_XG_E].fieldData = value;
        regDataArray[PRV_CPSS_PORT_XG_E].fieldLength = 1;
        regDataArray[PRV_CPSS_PORT_XG_E].fieldOffset = 13;
    }

    /* PRV_CPSS_DXCH_DEV_REGS_MAC(devNum)->macRegs.perPortRegs[portNum].
                            macRegsPerType[PRV_CPSS_PORT_XLG_E].macCtrl1; */
    if(PRV_CPSS_PP_MAC(devNum)->devFamily == CPSS_PP_FAMILY_DXCH_LION_E)
    {
        regAddr = PRV_CPSS_SW_PTR_ENTRY_UNUSED;
    }
    else if(PRV_CPSS_PP_MAC(devNum)->devFamily == CPSS_PP_FAMILY_DXCH_XCAT3_E)
    {
        regAddr = 0x120C0004 + 0x0 + (portNum * 0x1000);
    }
    else
    {
        regAddr = 0x08800004 + 0x0 + (portNum * 0x400);
    }
    if(regAddr != PRV_CPSS_SW_PTR_ENTRY_UNUSED)
    {
        regDataArray[PRV_CPSS_PORT_XLG_E].regAddr = regAddr;
        regDataArray[PRV_CPSS_PORT_XLG_E].fieldData = value;
        regDataArray[PRV_CPSS_PORT_XLG_E].fieldLength = 1;
        regDataArray[PRV_CPSS_PORT_XLG_E].fieldOffset = 13;
    }

    for(portMacType = PRV_CPSS_PORT_GE_E; portMacType < PRV_CPSS_PORT_NOT_APPLICABLE_E; portMacType++)
    {
        if(PRV_CPSS_DXCH_PP_MAC(devNum)->hwInfo.dedicatedCpuMac.isNotSupported == GT_FALSE)
        {
            if((portNum == CPSS_CPU_PORT_NUM_CNS) && (portMacType >= PRV_CPSS_PORT_XG_E))
                continue;
        }

        if(regDataArray[portMacType].regAddr != PRV_CPSS_SW_PTR_ENTRY_UNUSED)
        {
            if (prvCpssDrvHwPpPortGroupSetRegField(devNum, 0,
                                                    regDataArray[portMacType].regAddr,
                                                    regDataArray[portMacType].fieldOffset,
                                                    regDataArray[portMacType].fieldLength,
                                                    regDataArray[portMacType].fieldData) != GT_OK)
            {
                return GT_HW_ERROR;
            }
        }
    }
    return GT_OK;
}

/*******************************************************************************
* portPreLion2InternalLoopbackEnableSet_HardCodedRegAddr
*
* DESCRIPTION:
*       Configure MAC and PCS TX2RX loopbacks on port.
*
* INPUTS:
*       devNum    - physical device number
*       portNum   - physical port number (not CPU port)
*       enable    - If GT_TRUE, enable loopback
*                   If GT_FALSE, disable loopback
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
*   This function is written for pexOnly mode --> these is no register initialization
*   All register addresses are hard coded
*
*******************************************************************************/
static GT_STATUS portPreLion2InternalLoopbackEnableSet_HardCodedRegAddr
(
    IN  GT_U8                   devNum,
    IN  GT_PHYSICAL_PORT_NUM    portNum,
    IN  GT_BOOL                 enable
)
{
    GT_U32 regAddr;               /* register address                    */
    GT_U32 value;                 /* value to write into the register    */
    GT_U32 fieldOffset;           /* bit field offset */
    GT_U32 fieldLength;           /* number of bits to be written to register */
    PRV_CPSS_PORT_TYPE_ENT portMacType; /* MAC unit used by port */
    GT_STATUS rc;       /* return code */
    GT_U32          portMacNum;      /* MAC number */

    PRV_CPSS_DXCH_PORT_NUM_CHECK_AND_MAC_NUM_GET_MAC(devNum,portNum,portMacNum);


    if((rc = portPcsLoopbackEnableSet_HardCodedRegAddr(devNum,portNum,enable)) != GT_OK)
        return rc;

    for(portMacType = PRV_CPSS_PORT_GE_E; portMacType < PRV_CPSS_PORT_NOT_APPLICABLE_E; portMacType++)
    {
        if((portMacNum == CPSS_CPU_PORT_NUM_CNS) && (portMacType >= PRV_CPSS_PORT_XG_E))
                continue;

        if(portMacType == PRV_CPSS_PORT_GE_E)
        /* Set GMII loopback mode */
        {
            fieldOffset = 5;
            fieldLength = 1;
            value = (enable == GT_TRUE) ? 1 : 0;

            /* PRV_CPSS_DXCH_DEV_REGS_MAC(devNum)->macRegs.perPortRegs[portNum].
                            macRegsPerType[PRV_CPSS_PORT_GE_E].macCtrl1; */
            if(PRV_CPSS_PP_MAC(devNum)->devFamily == CPSS_PP_FAMILY_DXCH_LION_E)
            {
                regAddr = 0x0A800004 + 0x0 + (portNum & 0xf) * 0x400;
            }
            else if(PRV_CPSS_PP_MAC(devNum)->devFamily == CPSS_PP_FAMILY_DXCH_XCAT3_E)
            {
                regAddr = 0x12000004 + 0x0 + (portNum * 0x1000);
            }
            else
            {
                regAddr = 0x08800004 + 0x0 + (portNum * 0x400);
            }

            if (prvCpssDrvHwPpPortGroupSetRegField(devNum, 0,regAddr,
                                                   fieldOffset, fieldLength,
                                                   value) != GT_OK)
            {
                return GT_HW_ERROR;
            }
        }

        else
            continue;


    }

    return GT_OK;
}

/*******************************************************************************
* appDemoDxChPortLoopbackModeEnableSet_HardCodedRegAddr
*
* DESCRIPTION:
*       Set port in 'loopback' mode
*
* INPUTS:
*       devNum    - physical device number
*       portInterfacePtr - (pointer to) port interface
*       enable           - enable / disable (loopback/no loopback)
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK        - on success
*       GT_FAIL      - on error
*       GT_HW_ERROR  - on hardware error
*       GT_BAD_PARAM - wrong interface
*       GT_BAD_PTR   - on NULL pointer
*
* COMMENTS:
*   This function is written for pexOnly mode --> these is no register initialization
*   All register addresses are hard coded
*
*******************************************************************************/
GT_STATUS appDemoDxChPortLoopbackModeEnableSet_HardCodedRegAddr
(
    IN  GT_U8                   devNum,
    IN CPSS_INTERFACE_INFO_STC  *portInterfacePtr,
    IN GT_BOOL                   enable
)
{
    GT_U32      portNum;

    CPSS_NULL_PTR_CHECK_MAC(portInterfacePtr);

    if(portInterfacePtr->type != CPSS_INTERFACE_PORT_E)
    {
        return GT_BAD_PARAM;
    }

    devNum = devNum;
    portNum = portInterfacePtr->devPort.portNum;

    if (PRV_CPSS_PP_MAC(devNum)->devFamily == CPSS_PP_FAMILY_DXCH_LION2_E)
    {
        #if 0
        return cpssDxChPortPcsLoopbackModeSet(devNum,
                                               portInterfacePtr->devPort.portNum,
                                               enable ? CPSS_DXCH_PORT_PCS_LOOPBACK_TX2RX_E :
                                                        CPSS_DXCH_PORT_PCS_LOOPBACK_DISABLE_E);
        #endif
    }
    else
    {
        if((CPSS_PP_FAMILY_DXCH_LION2_E <= PRV_CPSS_PP_MAC(devNum)->devFamily) ||
           (PRV_CPSS_PP_MAC(devNum)->devFamily == CPSS_PP_FAMILY_DXCH_XCAT3_E))
        {
            #if 0
            if(portNum != CPSS_CPU_PORT_NUM_CNS)
            {
                return prvCpssDxChPortLion2InternalLoopbackEnableSet(devNum, portNum,
                                                                     enable);
            }
            #endif
        }

        /* for CPU port and older devices */
        return portPreLion2InternalLoopbackEnableSet_HardCodedRegAddr(devNum, portNum, enable);
    }

    return GT_OK;

}

