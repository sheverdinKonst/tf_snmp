/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetTwistL2.c
*
* DESCRIPTION:
*       This is a external API definition for Snet module of SKernel.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 48 $
*
*******************************************************************************/
#include <asicSimulation/SKernel/suserframes/snetTwistL2.h>
#include <asicSimulation/SKernel/suserframes/snetTwistEgress.h>
#include <asicSimulation/SKernel/smem/smemTwist.h>
#include <asicSimulation/SKernel/smem/smemTiger.h>
#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SKernel/suserframes/snet.h>
#include <asicSimulation/SKernel/suserframes/snetTwistInLif.h>
#include <asicSimulation/SKernel/suserframes/snetTwistIpV4.h>
#include <common/Utils/FrameInfo/sframeInfoAddr.h>
#include <common/Utils/PresteraHash/smacHash.h>
#include <asicSimulation/SKernel/twistCommon/sregTwist.h>
#include <asicSimulation/SKernel/sfdb/sfdb.h>
#include <asicSimulation/SKernel/sfdb/sfdbTwist.h>
#include <asicSimulation/SKernel/suserframes/snetTwistInLif.h>
#include <asicSimulation/SKernel/suserframes/snetTwistPcl.h>
#include <asicSimulation/SKernel/suserframes/snetSambaPolicy.h>
#include <asicSimulation/SKernel/suserframes/snetTwistTrafficCond.h>
#include <asicSimulation/SKernel/suserframes/snetTwistMpls.h>
#include <asicSimulation/SKernel/suserframes/snetTwistClassifier.h>
#include <asicSimulation/SLog/simLog.h>

#define INTERNAL_VTABLE_ADDR_OFFSET(port)\
                (port == 7) ? 0x04 : \
                (port == 23) ? 0x04 : \
                (port == 39) ? 0x04 : 0x00


#define INTERNAL_MCAST_PORT_OFFSET(port)\
                (port <  27) ? (4 + port) : (port - 27)

#define INTERNAL_MCAST_ADDR_OFFSET(port)\
                (port ==  27) ? 0x04 : 0x00

#define GT_HW_MAC_HIGH16(macAddr)           \
        ((macAddr)->arEther[1] | ((macAddr)->arEther[0] << 8))

#define GT_HW_MAC_LOW16(macAddr)            \
        ((macAddr)->arEther[5] | ((macAddr)->arEther[4] << 8))


static GT_BOOL snetTwistRxMacProcess(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
);

static GT_VOID snetTwistL2Process(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
);

static GT_VOID snetTwistL2RxMirror(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
);

static GT_VOID snetTwistL2VlanTcAssign(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
);

GT_VOID snetTwistL2ControlFrame(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
);

GT_VOID snetTwistL2MacRange(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
);

GT_VOID snetTwistL2FdbLookUp(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
);

extern GT_VOID snetTwistMacRangeFilterGet(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
);

GT_VOID snetTwistRxMacCountUpdate
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U32 port
);

GT_U32 snetTwistMacHashCalc(
    IN GT_U8 * macAddrPtr,
    IN GT_U16 vid,
    IN SGT_MAC_HASH  * macHashStructPtr
);

GT_VOID snetTwistL2LockPort(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
);

GT_VOID snetTwistL2IngressFilter(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
);

GT_VOID snetTwistL2Decision(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
);

static GT_VOID  snetTwistFwdUcastDest(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr
);
static GT_VOID  snetTwistFwdMcastDest(
    IN SKERNEL_FRAME_DESCR_STC * descrPtr
);
static GT_VOID  snetTwistTrapPacket(
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U32 cpuCode
);
static GT_VOID  snetTwistDropPacket(
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U32 dropCode
);
static GT_VOID  snetTwistSecurityDropPacket(
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U32 dropCode
);
static GT_VOID  snetTwistIngressDropPacket(
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U32 dropCode
);

GT_VOID  snetTwistRegularDropPacket(
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U32 dropCode
);

static GT_VOID snetTwistLearnSa(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr
);

GT_BOOL snetTwistProtVidGet(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U32 encapsul,
    IN GT_U32 etherType
);

GT_BOOL snetTwistL2IPsubnetVlan(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 etherType,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
);

static GT_U16 snetTwistTrunkHash(
    IN SGT_MAC_ADDR_TYP * macDstPtr,
    IN SGT_MAC_ADDR_TYP * macSrcPtr,
    IN GT_U16 trgPort
);

GT_VOID snetTwistCreateAuMsg(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN SNET_TWIST_MAC_TBL_STC * twistMacEntryPtr,
    OUT GT_U32 hwData[4]
);

GT_U16 intrCauseLinkStatusRegInfoGet(
    IN  GT_U32 devType,
    IN  GT_U32 port,
    IN  GT_BOOL isMask,
    OUT GT_U32 * regAddress,
    OUT GT_U32 * fldOffset
);

GT_BOOL snetTwistSflow(
    IN SKERNEL_DEVICE_OBJECT   * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN SNET_SFLOW_DIRECTION_ENT direction
);

static GT_VOID snetTwistSflowCountInit
(
    IN SKERNEL_DEVICE_OBJECT   * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr
);

GT_VOID snetTwistMacHashInit
(
    IN SKERNEL_DEVICE_OBJECT   * devObjPtr,
    INOUT SGT_MAC_HASH * macHashPtr
);

static GT_VOID snetTwistL2MplsParse(
    IN SKERNEL_DEVICE_OBJECT   * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U32 shimOffset
);

static GT_VOID snetTwistL2MatchProtocol(
    IN      SKERNEL_DEVICE_OBJECT *   devObjPtr,
    INOUT   SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN      GT_U32                    regAddress,
    IN      GT_U32                    fldFirstBit,
    IN      GT_U32                    prot
);


static GT_VOID snetTwistFrameProcess
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN SBUF_BUF_ID bufferId,
    IN GT_U32 srcPort
);

GT_VOID snetTwistLinkStateNotify
(
    IN SKERNEL_DEVICE_OBJECT   * deviceObjPtr,
    IN GT_U32 port,
    IN GT_U32 linkState
);

/*******************************************************************************
*   snetTwistProcessInit
*
* DESCRIPTION:
*       Init module.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*
*******************************************************************************/
GT_VOID snetTwistProcessInit
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    devObjPtr->descriptorPtr =
        (void *)calloc(1, sizeof(SKERNEL_FRAME_DESCR_STC));

    if (devObjPtr->descriptorPtr == 0)
    {
        skernelFatalError("smemTwistInit: allocation error\n");
    }

    /* initiation of internal twist functions */
    devObjPtr->devFrameProcFuncPtr = snetTwistFrameProcess;
    devObjPtr->devPortLinkUpdateFuncPtr = snetTwistLinkStateNotify;
    devObjPtr->devFdbMsgProcFuncPtr = sfdbTwistMsgProcess;
    devObjPtr->devMacTblTrigActFuncPtr = sfdbTwistMacTableTriggerAction;
    devObjPtr->devMacTblAgingProcFuncPtr = sfdbTwistMacTableAging;
}

/*******************************************************************************
*   snetTwistFrameProcess
*
* DESCRIPTION:
*       devObjPtr    - pointer to device object.
*       bufferId     - frame data buffer Id
*       srcPort      - source port number
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*
*******************************************************************************/
static GT_VOID snetTwistFrameProcess
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SBUF_BUF_ID bufferId,
    IN GT_U32 srcPort
)
{
    GT_BOOL retVal;

    SKERNEL_FRAME_DESCR_STC * descrPtr; /* pointer to the frame's descriptor */

    descrPtr = (SKERNEL_FRAME_DESCR_STC *)devObjPtr->descriptorPtr;
    memset(descrPtr, 0, sizeof(SKERNEL_FRAME_DESCR_STC));

    descrPtr->frameBuf = bufferId;
    descrPtr->srcPort = srcPort;
    descrPtr->byteCount = (GT_U16)bufferId->actualDataSize;

    /* Parsing the frame, get information from frame and fill descriptor */
    snetFrameParsing(devObjPtr, descrPtr);
    /* Rx MAC layer processing of the Twist  */
    retVal = snetTwistRxMacProcess(devObjPtr, descrPtr);
    if (retVal == GT_FALSE)
        return;
    /* Layer2 frame process */
    snetTwistL2Process(devObjPtr, descrPtr);

    /* InLif processing */
    retVal = snetTwistInLifProcess(devObjPtr, descrPtr);
    if (retVal == GT_FALSE)
        return;

    /* make classifier/pcl/policy processing */
    if(devObjPtr->deviceFamily == SKERNEL_SAMBA_FAMILY)
    {
        snetSambaPolicyEngine(devObjPtr, descrPtr);
    }
    else if(devObjPtr->deviceFamily == SKERNEL_TWIST_D_FAMILY ||
            devObjPtr->deviceFamily == SKERNEL_TIGER_FAMILY)
    {
        retVal = snetTwistPolicyProcess(devObjPtr, descrPtr);
        if (retVal == GT_FALSE)
            return;
    }
    else if(devObjPtr->deviceFamily == SKERNEL_TWIST_C_FAMILY)
    {
        retVal = snetTwistCClassificationEngine(devObjPtr, descrPtr);
        if (retVal == GT_FALSE)
            return;
    }

    /* make Traffic Condition processing */
    retVal = snetTwistTrafficConditionProcess(devObjPtr, descrPtr);
    if (retVal == GT_FALSE)
        return;

    /* make MPLS processing */
    retVal = snetTwistMplsProcess(devObjPtr, descrPtr);
    if (retVal == GT_FALSE)
        return;

    /* make IPv4 processing */
    retVal = snetTwistIPv4Process(devObjPtr, descrPtr);
    if (retVal == GT_FALSE)
        return;

    /* Form descriptor according to previous processing */
    snetTwistLxOutBlock(devObjPtr, descrPtr);

    /* Send descriptor to different egress  units */
    snetTwistEqBlock(devObjPtr, descrPtr);
}

/*******************************************************************************
*   snetTwistLinkStateNotify
*
* DESCRIPTION:
*       Notify devices database that link state changed
*
* INPUTS:
*       devObjPtr - pointer to device object.
*       port            - port number.
*       linkState   - link state (0 - down, 1 - up)
*
*******************************************************************************/
GT_VOID snetTwistLinkStateNotify
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 port,
    IN GT_U32 linkState
)
{
    GT_U32 regAddress = 0;
    GT_U32 fldValue;
    GT_U32 fldFirstBit;
    GT_U16 macSumBit;

    /* MAC Status Register */
    MAC_STATUS_REG(devObjPtr->deviceType, port, regAddress);

    smemRegFldSet(devObjPtr, regAddress, 0, 1, linkState);

    /* update link state in the MAC Status Register - bit 0 - linkUp */
    intrCauseLinkStatusRegInfoGet(devObjPtr->deviceType, port, GT_TRUE,
                                    &regAddress, &fldFirstBit);

    smemRegFldGet(devObjPtr, regAddress, fldFirstBit, 1, &fldValue);
    /* LinkStatus ChangedOn Port(interrupt mask register) */
    if (fldValue != 1)
        return;

    macSumBit = intrCauseLinkStatusRegInfoGet(devObjPtr->deviceType, port,
                                              GT_FALSE,&regAddress,
                                              &fldFirstBit);
    /* Link status changed on port (interrupt cause register) */
    smemRegFldSet(devObjPtr, regAddress, fldFirstBit, 1, 1);
    /* Sum of all GOP interrupts */
    smemRegFldSet(devObjPtr, regAddress, 0, 1, fldValue);


    /* MACSumInt Mask */
    smemPciRegFldGet(devObjPtr, PCI_INT_MASK_REG, macSumBit, 1, &fldValue);
    if (fldValue != 1)
        return;
    /* PCI Interrupt Summary Mask */
    smemPciRegFldGet(devObjPtr, PCI_INT_MASK_REG, 1, 1, &fldValue);
    if (fldValue)
    {
        /* MACSumInt */
        smemPciRegFldSet(devObjPtr, PCI_INT_CAUSE_REG, macSumBit, 1, 1);
        /* IntSum */
        smemPciRegFldSet(devObjPtr, PCI_INT_CAUSE_REG, 0, 1, 1);
    }

    /* Call interrupt */
    scibSetInterrupt(devObjPtr->deviceId);
}

/*******************************************************************************
*   snetTwistRxMacProcess
*
* DESCRIPTION:
*       Rx MAC layer processing of the Twist
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - pointer to the frame's descriptor.
*
* RETURNS:
*       GT_TRUE
*       GT_FALSE
*
*******************************************************************************/
static GT_BOOL snetTwistRxMacProcess
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetTwistRxMacProcess);

    GT_U32 regAddress = 0;
    GT_U32 fldValue;

    smemRegFldGet(devObjPtr, GLOBAL_CONTROL_REG, 0, 1, &fldValue);
    /* Device Enable */
    if (fldValue != 1)
        return GT_FALSE;

    MAC_CONTROL_REG(devObjPtr->portsNumber , descrPtr->srcPort, &regAddress);
    smemRegFldGet(devObjPtr, regAddress, 0, 1, &fldValue);
    /* This bit is set to enable the port */
    __LOG(("This bit is set to enable the port"));
    if (fldValue != 1)
        return GT_FALSE;

    smemRegFldGet(devObjPtr, GLOBAL_CONTROL_REG, 8, 7, &fldValue);
    /* DeviceID[14:8] */
    descrPtr->srcDevice = (GT_U8)fldValue;

    snetTwistRxMacCountUpdate(devObjPtr, descrPtr, descrPtr->srcPort);

    return GT_TRUE;
}

/*******************************************************************************
*   snetTwistL2Process
*
* DESCRIPTION:
*       Layer2 processing of the Twist
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - pointer to the frame's descriptor.
*
* RETURNS:
*
*******************************************************************************/
static GT_VOID snetTwistL2Process
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    snetTwistL2RxMirror(devObjPtr, descrPtr);
    snetTwistL2VlanTcAssign(devObjPtr, descrPtr);
    snetTwistL2ControlFrame(devObjPtr, descrPtr);
    snetTwistL2MacRange(devObjPtr, descrPtr);
    snetTwistL2FdbLookUp(devObjPtr, descrPtr);
    snetTwistL2LockPort(devObjPtr, descrPtr);
    snetTwistL2IngressFilter(devObjPtr, descrPtr);
    snetTwistL2Decision(devObjPtr, descrPtr);
}

/*******************************************************************************
*   snetTwistL2RxMirror
*
* DESCRIPTION:
*       Make RX mirroring
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr        - pointer to the frame's descriptor.
*
*******************************************************************************/
static GT_VOID snetTwistL2RxMirror
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetTwistL2RxMirror);

    GT_U32 regAddress, fldValue;
    GT_U32 targetDevice, targetPort;
    GT_BOOL sflowStatus;

    if (devObjPtr->deviceType == SKERNEL_98EX130D ||
        devObjPtr->deviceType == SKERNEL_98EX135D ||
        devObjPtr->deviceType == SKERNEL_98MX635D ||
        devObjPtr->deviceType == SKERNEL_98EX136)
    {
        sflowStatus = snetTwistSflow(devObjPtr, descrPtr, SNET_SFLOW_RX);
        if (sflowStatus == GT_TRUE)
        {
            return;
        }
    }
    /* Bridge Port<n> Control Register */
    BRDG_PORT0_CTRL_REG(descrPtr->srcPort, &regAddress);
    smemRegFldGet(devObjPtr, regAddress, 24, 1, &fldValue);
    /* If sniffer bit disable */
    if (fldValue != 1)
        return;

    smemRegFldGet(devObjPtr, INGR_MIRR_ANALYZER_REG, 4, 7, &fldValue);
    /* Indicates the target device number for Rx sniffed packets. */
    __LOG(("Indicates the target device number for Rx sniffed packets."));
    targetDevice = fldValue;

    smemRegFldGet(devObjPtr, INGR_MIRR_ANALYZER_REG, 11, 6, &fldValue);
    /* Indicates the target port number for Rx sniffed packets. */
    targetPort = fldValue;

    smemRegFldGet(devObjPtr, INGR_MIRR_ANALYZER_REG, 1, 3, &fldValue);
    /* Indicates the Traffic Class of mirrored packets when they are sent
    to the CPU. */
    descrPtr->trafficClass = (GT_U8)fldValue;
    /* Transmit packet to device */
    /* CPU code should be specifically defined to TX ??? */
    snetTwistTx2Device(devObjPtr, descrPtr, descrPtr->frameBuf->actualDataPtr,
        descrPtr->byteCount,  targetPort, targetDevice, TWIST_CPU_CODE_RX_SNIFF);
}
/*******************************************************************************
*   snetTwistRxMacCountUpdate
*
* DESCRIPTION:
*        Global update counters function (MAC MIB Counters + CPU port)
*
* INPUTS:
*        devObjPtr  - pointer to device object.
*        descrPtr   - pointer to the frame's descriptor.
*        port       - counter's port
*
*******************************************************************************/
GT_VOID snetTwistRxMacCountUpdate
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U32 port
)
{
    DECLARE_FUNC_NAME(snetTwistRxMacCountUpdate);

    GT_U32 regAddress;
    GT_U32 fldValue;
    GT_U32 octets;

    ASSERT_PTR(devObjPtr);
    ASSERT_PTR(descrPtr);

    /* MAC_MIB_COUNTERS_REG(devObjPtr->portsNumber, port, (GT_U32 *)&regAddress); */
    MAC_MIB_COUNTERS_REG(devObjPtr->portsNumber, port, &regAddress);
    if (descrPtr->macDaType == SKERNEL_MULTICAST_MAC_E) {
        smemRegFldGet(devObjPtr, regAddress + 0x1C, 0, 32, &fldValue);
        smemRegFldSet(devObjPtr, regAddress + 0x1C, 0, 32, ++fldValue);
    }
    else
    if (descrPtr->macDaType == SKERNEL_BROADCAST_MAC_E) {
        smemRegFldGet(devObjPtr, regAddress + 0x18, 0, 32, &fldValue);
        smemRegFldSet(devObjPtr, regAddress + 0x18, 0, 32, ++fldValue);
    }

    octets = SNET_GET_NUM_OCTETS_IN_FRAME(descrPtr->byteCount);

    switch (octets) {
    case SNET_FRAMES_1024_TO_MAX_OCTETS:
        smemRegFldGet(devObjPtr, regAddress + 0x34, 0, 32, &fldValue);
        smemRegFldSet(devObjPtr, regAddress + 0x34, 0, 32, ++fldValue);
        break;
    case SNET_FRAMES_512_TO_1023_OCTETS:
        smemRegFldGet(devObjPtr, regAddress + 0x30, 0, 32, &fldValue);
        smemRegFldSet(devObjPtr, regAddress + 0x30, 0, 32, ++fldValue);
        break;
    case SNET_FRAMES_256_TO_511_OCTETS:
        smemRegFldGet(devObjPtr, regAddress + 0x2C, 0, 32, &fldValue);
        smemRegFldSet(devObjPtr, regAddress + 0x2C, 0, 32, ++fldValue);
        break;
    case SNET_FRAMES_128_TO_255_OCTETS:
        smemRegFldGet(devObjPtr, regAddress + 0x28, 0, 32, &fldValue);
        smemRegFldSet(devObjPtr, regAddress + 0x28, 0, 32, ++fldValue);
        break;
    case SNET_FRAMES_65_TO_127_OCTETS:
        smemRegFldGet(devObjPtr, regAddress + 0x24, 0, 32, &fldValue);
        smemRegFldSet(devObjPtr, regAddress + 0x24, 0, 32, ++fldValue);
        break;
    case SNET_FRAMES_64_OCTETS:
        smemRegFldGet(devObjPtr, regAddress + 0x20, 0, 32, &fldValue);
        smemRegFldSet(devObjPtr, regAddress + 0x20, 0, 32, ++fldValue);
        break;
    }
    /* Bad frames received*/
    __LOG(("Bad frames received"));
    smemRegFldGet(devObjPtr, regAddress + 0x14, 0, 32, &fldValue);
    smemRegFldSet(devObjPtr, regAddress + 0x14, 0, 32, ++fldValue);
    /* Good frames received */
    __LOG(("Good frames received"));
    smemRegFldGet(devObjPtr, regAddress + 0x10, 0, 32, &fldValue);
    smemRegFldSet(devObjPtr, regAddress + 0x10, 0, 32, ++fldValue);
}

/*******************************************************************************
*   snetTwistTxMacCountUpdate
*
* DESCRIPTION:
*        Global update counters function (MAC MIB Counters + CPU port)
*
* INPUTS:
*        devObjPtr  - pointer to device object.
*        descrPtr   - pointer to the frame's descriptor.
*        port       - counter's port
*
*******************************************************************************/
GT_VOID snetTwistTxMacCountUpdate
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U32 port
)
{
    DECLARE_FUNC_NAME(snetTwistTxMacCountUpdate);

    GT_U32 regAddress = 0;
    GT_U32 fldValue;
    GT_U32 octets;

    ASSERT_PTR(devObjPtr);
    ASSERT_PTR(descrPtr);

    if (port == TWIST_CPU_PORT_CNS)
        return;

    MAC_MIB_COUNTERS_REG(devObjPtr->portsNumber, port, &regAddress);

    octets = SNET_GET_NUM_OCTETS_IN_FRAME(descrPtr->byteCount);

    switch (octets) {
    case SNET_FRAMES_1024_TO_MAX_OCTETS:
        smemRegFldGet(devObjPtr, regAddress + 0x34, 0, 32, &fldValue);
        smemRegFldSet(devObjPtr, regAddress + 0x34, 0, 32, ++fldValue);
        break;
    case SNET_FRAMES_512_TO_1023_OCTETS:
        smemRegFldGet(devObjPtr, regAddress + 0x30, 0, 32, &fldValue);
        smemRegFldSet(devObjPtr, regAddress + 0x30, 0, 32, ++fldValue);
        break;
    case SNET_FRAMES_256_TO_511_OCTETS:
        smemRegFldGet(devObjPtr, regAddress + 0x2C, 0, 32, &fldValue);
        smemRegFldSet(devObjPtr, regAddress + 0x2C, 0, 32, ++fldValue);
        break;
    case SNET_FRAMES_128_TO_255_OCTETS:
        smemRegFldGet(devObjPtr, regAddress + 0x28, 0, 32, &fldValue);
        smemRegFldSet(devObjPtr, regAddress + 0x28, 0, 32, ++fldValue);
        break;
    case SNET_FRAMES_65_TO_127_OCTETS:
        smemRegFldGet(devObjPtr, regAddress + 0x24, 0, 32, &fldValue);
        smemRegFldSet(devObjPtr, regAddress + 0x24, 0, 32, ++fldValue);
        break;
    case SNET_FRAMES_64_OCTETS:
        smemRegFldGet(devObjPtr, regAddress + 0x20, 0, 32, &fldValue);
        smemRegFldSet(devObjPtr, regAddress + 0x20, 0, 32, ++fldValue);
        break;
    }
    /* GoodOctetsSent */
    __LOG(("GoodOctetsSent"));
    smemRegFldGet(devObjPtr, regAddress + 0x38, 0, 32, &fldValue);
        if (fldValue < 0xFFFFFFFF )
        {
                smemRegFldSet(devObjPtr, regAddress + 0x38, 0, 32, ++fldValue);
        }
        else
        {
                smemRegFldGet(devObjPtr, regAddress + 0x3C, 0, 32, &fldValue);
                smemRegFldSet(devObjPtr, regAddress + 0x3C, 0, 32, ++fldValue);
        }
    /* For Ethernet GoodFramesSent */
    smemRegFldGet(devObjPtr, regAddress + 0x40, 0, 32, &fldValue);
    smemRegFldSet(devObjPtr, regAddress + 0x40, 0, 32, ++fldValue);
    /* For Ethernet MulticastFramesSent */
    smemRegFldGet(devObjPtr, regAddress + 0x48, 0, 32, &fldValue);
    smemRegFldSet(devObjPtr, regAddress + 0x48, 0, 32, ++fldValue);
    /* For Ethernet BroadcastFramesSent */
    smemRegFldGet(devObjPtr, regAddress + 0x4C, 0, 32, &fldValue);
    smemRegFldSet(devObjPtr, regAddress + 0x4C, 0, 32, ++fldValue);
        /* For Ethernet FCSent */
    smemRegFldGet(devObjPtr, regAddress + 0x54, 0, 32, &fldValue);
    smemRegFldSet(devObjPtr, regAddress + 0x54, 0, 32, ++fldValue);


    snetTwistPortEgressCount(devObjPtr, descrPtr,
                            port,SKERNEL_EGRESS_COUNTER_NOT_FILTER_E);

}
/*******************************************************************************
*   snetProtVidGet
*
* DESCRIPTION:
*        Assign VLAN and Traffic Class
* INPUTS:
*        devObjPtr   -  pointer to device object.
*        descrPtr  -    pointer to the frame's descriptor.
*
* OUTPUTS:
*        descrPtr  -    pointer to the frame's descriptor.
*
*******************************************************************************/
GT_BOOL snetTwistProtVidGet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U32 encapsul,
    IN GT_U32 etherType
)
{
    GT_U32 prot;
    GT_U32 regAddress;
    GT_U32 fldValue;
    GT_U32 fldFirstBit;

    for (prot = 0; prot < 12; prot++) {
        regAddress = PROT_ENCAPSULATION_REG(prot);
        if (encapsul == SKERNEL_ENCAP_ETHER_E) {
            fldFirstBit = PROT_ENCAPSULATION_ETHER_BIT(prot);
            smemRegFldGet(devObjPtr, regAddress, fldFirstBit, 1, &fldValue);
            /* EtherType<n> Ethernet */
            if (fldValue == 0) {
                continue;
            }
        }
        else
        if (encapsul == SKERNEL_ENCAP_LLC_E) {
            fldFirstBit = PROT_ENCAPSULATION_LLC_BIT(prot);
            smemRegFldGet(devObjPtr, regAddress, fldFirstBit, 1, &fldValue);
            /* EtherType<n> LLC */
            if (fldValue == 0) {
                continue;
            }
        }
        else
        if (encapsul == SKERNEL_ENCAP_LLC_SNAP_E) {
            fldFirstBit = PROT_ENCAPSULATION_LLC_SNAP_BIT(prot);
            smemRegFldGet(devObjPtr, regAddress, fldFirstBit, 1, &fldValue);
            /* EtherType<n> LLC/SNAP */
            if (fldValue == 0) {
                continue;
            }
        }

        ETHER_TYPE_REG(prot, &regAddress);
        fldFirstBit = ((prot % 2) == 0) ? 0 : 16;
        /* EtherType<prot> */
        smemRegFldGet(devObjPtr, regAddress, fldFirstBit, 16, &fldValue);
        if (fldValue == etherType)
        {
            snetTwistL2MatchProtocol(devObjPtr,descrPtr,regAddress,fldFirstBit,prot);
            return GT_TRUE;
        }
    }

    return GT_FALSE;
}

/*******************************************************************************
*   snetTwistL2MatchProtocol
*
* DESCRIPTION:
*        Assign VLAN and Traffic Class according to the matching of protocol.
* INPUTS:
*        devObjPtr   -  pointer to device object.
*        descrPtr    -  pointer to the frame's descriptor.
*        regAddress  -  address of the vid and tc.
*        fldFirstBit -  field to be read.
*        prot        -  protocol number.
*
* OUTPUTS:
*
*******************************************************************************/
static void snetTwistL2MatchProtocol
(
    IN SKERNEL_DEVICE_OBJECT *      devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U32                       regAddress,
    IN GT_U32                       fldFirstBit,
    IN GT_U32                       prot
)
{
    DECLARE_FUNC_NAME(snetTwistL2MatchProtocol);

    GT_U32 fldValue;

    PROTOCOL_VID_REG(prot, descrPtr->srcPort, &regAddress);
    fldFirstBit = ((prot % 2)==0) ? 0 : 16;
    smemRegFldGet(devObjPtr, regAddress, fldFirstBit, 12, &fldValue);
    /* VidU <protocol>*/
    descrPtr->vid = (GT_U16)fldValue;
    fldFirstBit = ((prot % 2)==0) ? 12 : 28;
    smemRegFldGet(devObjPtr, regAddress, fldFirstBit, 3, &fldValue);
    /* TrafficClassU <protocol>*/
    __LOG(("TrafficClassU <protocol>"));
    descrPtr->trafficClass = (GT_U8)fldValue;

}


/*******************************************************************************
*   snetTwistL2IPsubnetVlan
*
* DESCRIPTION:
*        Assign VLAN and Traffic Class according to the subnet
* INPUTS:
*        devObjPtr   -  pointer to device object.
*        descrPtr  -    pointer to the frame's descriptor.
*
* OUTPUTS:
*        descrPtr  -    pointer to the frame's descriptor.
*
*******************************************************************************/
GT_BOOL snetTwistL2IPsubnetVlan
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 etherType,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetTwistL2IPsubnetVlan);

    GT_U32 subnetNum;
    GT_U32 subnetIp;
    GT_U32 subnetMask;
    GT_U32 regAddress;
    GT_U32 fldValue;
    GT_U32 fldFirstBit;

    for (subnetNum = 0; subnetNum < 12; subnetNum++) {
        SUBNET_IP_REG(subnetNum, &regAddress);
        smemRegFldGet(devObjPtr, regAddress, 0, 32, &fldValue);
        /* Subnet<subnetNum>IP */
        __LOG(("Subnet<subnetNum>IP"));
        subnetIp = fldValue;

        SUBNET_MASK_REG(subnetNum, &regAddress);
        fldFirstBit = SUBNET_MASK_LEN_BIT(subnetNum);
        smemRegFldGet(devObjPtr, regAddress, fldFirstBit + 5, 1, &fldValue);
        /* Subnet<subnetNum>Valid */
        if (fldValue == 0)
            continue;

        smemRegFldGet(devObjPtr, regAddress, fldFirstBit, 5, &fldValue);
        /* Subnet<subnetNum>Length */
        subnetMask = ~(0xFFFFFFFF >> fldValue);

        if ((descrPtr->ipv4SIP & subnetMask) == subnetIp) {
            SUBNET_VID_REG(subnetNum, &regAddress);
            fldFirstBit = ((subnetNum %2)==0) ? 0 : 16;
            smemRegFldGet(devObjPtr, regAddress, fldFirstBit, 12, &fldValue);
            /* VidSubnet<subnetNum> */
            descrPtr->vid = (GT_U16)fldValue;
            smemRegFldGet(devObjPtr, regAddress, fldFirstBit+12, 3, &fldValue);
            /* TrafficClassSubnet<subnetNum> */
            descrPtr->trafficClass = (GT_U8)fldValue;
            return GT_TRUE;
        }
    }
    return GT_FALSE;
}

/*******************************************************************************
*   snetTwistL2VlanTcAssign
*
* DESCRIPTION:
*        Assign VLAN and Traffic Class
* INPUTS:
*        devObjPtr   -  pointer to device object.
*        descrPtr  -    pointer to the frame's descriptor.
*
* OUTPUTS:
*        descrPtr  -    pointer to the frame's descriptor.
*
*******************************************************************************/
static GT_VOID snetTwistL2VlanTcAssign
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    GT_U32 regAddress;
    GT_U32 fldValue;
    GT_U32 fldFirstBit;
    GT_U8 * dataPtr;
    GT_U16 etherType, realEtherType;
    GT_U32 encapsul;
    GT_U32 dsapSsap;
    GT_BOOL result;
    GT_U32  ipOffsetFromL2=12;
    GT_U32 headerSize=0,nonIpDscp=0;

    dataPtr = descrPtr->frameBuf->actualDataPtr;
    etherType = dataPtr[12] << 8 | dataPtr[13];
    if (etherType == 0x8100) {
        descrPtr->srcVlanTagged = 1;
        descrPtr->vid =  ((dataPtr[14] & 0xF) << 8) | dataPtr[15];
        descrPtr->userPriorityTag = (dataPtr[14] & 0xE0) >> 5;
        realEtherType = (descrPtr->dstMacPtr[16] << 8) |
            descrPtr->dstMacPtr[17];
        dsapSsap = (descrPtr->dstMacPtr[18] << 8) | descrPtr->dstMacPtr[19];
        ipOffsetFromL2 += 4;
    }
    else
    {
        realEtherType = (descrPtr->dstMacPtr[12] << 8) |
            descrPtr->dstMacPtr[13];
        dsapSsap = (descrPtr->dstMacPtr[14] << 8) | descrPtr->dstMacPtr[15];
    }

    if (realEtherType <  0x0600) {
        if (dsapSsap == 0xAAAA) {
            encapsul = SKERNEL_ENCAP_LLC_SNAP_E;
            ipOffsetFromL2+=10;
        }
        else {
            encapsul = SKERNEL_ENCAP_LLC_E;
            ipOffsetFromL2+=2;
        }
    }
    else {
        encapsul = SKERNEL_ENCAP_ETHER_E;
        ipOffsetFromL2+=2;
    }

    if (realEtherType == 0x0800) {
        descrPtr->ipHeaderPtr = dataPtr + ipOffsetFromL2;
        /* get dscp */
        descrPtr->dscp = descrPtr->ipHeaderPtr[1] >> 2;
        headerSize = (descrPtr->ipHeaderPtr[0] & 0x0f) * 4;
        /* Calculate IP_OPTIONS size and SET */
        descrPtr->l4HeaderPtr = descrPtr->ipHeaderPtr + headerSize;

        descrPtr->ipv4SIP =  descrPtr->ipHeaderPtr[12] << 24 |
            descrPtr->ipHeaderPtr[13] << 16 |
            descrPtr->ipHeaderPtr[14] <<  8 |
            descrPtr->ipHeaderPtr[15];

        descrPtr->ipv4DIP =
            descrPtr->ipHeaderPtr[16] << 24 |
            descrPtr->ipHeaderPtr[17] << 16 |
            descrPtr->ipHeaderPtr[18] <<  8 |
            descrPtr->ipHeaderPtr[19];
    }
    else
    {
        /* get default dscp for non-IP packets */
        smemRegFldGet(devObjPtr,INBLOCK_CONTROL_REG,2,6,&nonIpDscp);
        descrPtr->dscp = (GT_U8)nonIpDscp;
    }

    if (descrPtr->srcVlanTagged == 1) {

        fldFirstBit = VPT2TC_BIT(descrPtr->userPriorityTag);
        smemRegFldGet(devObjPtr, VPT_2_TRAFFIC_CLASS_REG, fldFirstBit, 3,
                      &fldValue);
        descrPtr->trafficClass = (GT_U8)fldValue;

        /* Bridge Port<n> Control Register */
        BRDG_PORT0_CTRL_REG(descrPtr->srcPort, &regAddress);
        smemRegFldGet(devObjPtr, regAddress, 5, 1, &fldValue);
        /* Override traffic class */
        if (fldValue == 0) {
            smemRegFldGet(devObjPtr, regAddress, 6, 3, &fldValue);
            descrPtr->trafficClass = (GT_U8)fldValue;
        }

        return;
    }

    if (realEtherType == 0x0800) {
        /* Check ip subnet based VLAN */

        /* Bridge Port<n> Control Register */
        BRDG_PORT0_CTRL_REG(descrPtr->srcPort, &regAddress);
        smemRegFldGet(devObjPtr, regAddress, 19, 1, &fldValue);
        /* SubnetBased VLANEn */
        if (fldValue == 1) {
            result = snetTwistL2IPsubnetVlan(devObjPtr, etherType, descrPtr);
            if (result == GT_TRUE)
            {
                /* set userPriorityTag*/
                fldFirstBit = TC2VPT_BIT(descrPtr->trafficClass);
                smemRegFldGet(devObjPtr, TRAFFIC_2_VPT_CLASS_REG, fldFirstBit, 3,
                              &fldValue);
                descrPtr->userPriorityTag = (GT_U8)fldValue;
                return;
            }
        }
    }


    /* Bridge Port<n> Control Register */
    BRDG_PORT0_CTRL_REG(descrPtr->srcPort, &regAddress);
    smemRegFldGet(devObjPtr, regAddress, 18, 1, &fldValue);
    /* ProtBased VLANEn */
    result = GT_FALSE;
    if (fldValue == 1) {
        result = snetTwistProtVidGet(devObjPtr, descrPtr, encapsul,
                                     realEtherType);
    }

    if (result == GT_FALSE) {
        PORT_VID_REG(descrPtr->srcPort, &regAddress);
        smemRegFldGet(devObjPtr, regAddress, 0, 12, &fldValue);
        /* PVid */
        descrPtr->vid = (GT_U16)fldValue;
        /* Bridge Port<n> Control Register */
        BRDG_PORT0_CTRL_REG(descrPtr->srcPort, &regAddress);
        smemRegFldGet(devObjPtr, regAddress, 6, 3, &fldValue);
        /* Default Traffic Class */
        descrPtr->trafficClass = (GT_U8)fldValue;
    }
    /* set userPriorityTag*/
    fldFirstBit = TC2VPT_BIT(descrPtr->trafficClass);
    smemRegFldGet(devObjPtr, TRAFFIC_2_VPT_CLASS_REG, fldFirstBit, 3,
                  &fldValue);
    descrPtr->userPriorityTag = (GT_U8)fldValue;
}

/*******************************************************************************
*   snetTwistMacRangeFilterGet
*
* DESCRIPTION:
*        Get MAC range filter
* INPUTS:
*        devObjPtr -  pointer to the device object.
*        descrPtr  -  pointer to the frame's descriptor.
*
*******************************************************************************/
extern GT_VOID snetTwistMacRangeFilterGet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetTwistMacRangeFilterGet);

    GT_U32 range, maxRange;
    GT_U32 regAddress;
    GT_U32 fldValue;
    GT_U32 firstBit;
    SGT_MAC_ADDR_TYP macAddr;
    SGT_MAC_ADDR_TYP macAddrMask;
    SGT_MAC_ADDR_TYP macAddrDest;
    GT_U32 daAction, saAction;
    GT_U32 cpuMirrorAction = 0;

    daAction = 0;
    saAction = 0;

    if (devObjPtr->deviceType == SKERNEL_98EX130D ||
        devObjPtr->deviceType == SKERNEL_98EX135D ||
        devObjPtr->deviceType == SKERNEL_98MX635D ||
        devObjPtr->deviceType == SKERNEL_98EX116 ||
        devObjPtr->deviceType == SKERNEL_98EX126 ||
        devObjPtr->deviceType == SKERNEL_98EX136)
    {
        maxRange = 4;
    }
    else if (devObjPtr->deviceFamily == SKERNEL_SALSA_FAMILY)
    {
        /* use for salsa same code as for twist !!! */
        __LOG(("use for salsa same code as for twist !!!"));
        maxRange = 4;
    }
    else
    {
        maxRange = 8;
    }

    for (range = 0; range < maxRange; range++) {
        /* Read MAC address */
        MAC_RNG_FLTR0_REG(range, &regAddress);
        smemRegGet(devObjPtr,regAddress ,&fldValue);
        macAddr.w.hword2 = (GT_U16)SMEM_U32_GET_FIELD(fldValue,0,16);
        macAddr.w.word1 = SMEM_U32_GET_FIELD(fldValue,16,16);

        MAC_RNG_FLTR1_REG(range, &regAddress);
        smemRegGet(devObjPtr,regAddress ,&fldValue);
        macAddr.w.word1 |= (SMEM_U32_GET_FIELD(fldValue,0,16)) << 16 ;
        macAddrMask.w.hword2 = (GT_U16)SMEM_U32_GET_FIELD(fldValue,16,16);

        MAC_RNG_FLTR2_REG(range, &regAddress);
        smemRegGet(devObjPtr, regAddress, &fldValue);
        macAddrMask.w.word1 = fldValue;

        macAddr.w.hword2 = SGT_LIB_SWAP16_MAC(macAddr.w.hword2);
        macAddr.w.word1 = SGT_LIB_SWAP_BYTES_AND_WORDS_MAC(macAddr.w.word1);

        macAddrMask.w.hword2 = SGT_LIB_SWAP16_MAC(macAddrMask.w.hword2);
        macAddrMask.w.word1 = SGT_LIB_SWAP_BYTES_AND_WORDS_MAC(macAddrMask.w.word1);


        memcpy(macAddrDest.bytes, descrPtr->dstMacPtr, 6);
        SGT_MAC_ADDR_APPLY_MASK(&macAddrDest, &macAddrMask);
        if (SGT_MAC_ADDR_ARE_EQUAL(&macAddrDest, &macAddr)) {
            /* Range DACMD */
            firstBit = RANGE_DACMD(range);
            smemRegFldGet(devObjPtr, MAC_RNG_FLTR_CMD_REG, firstBit, 2,
                          &fldValue);
            /* Range DACMD  */
            daAction = fldValue;
            firstBit = RANGE_DAMIRROR(range);
            if (devObjPtr->deviceFamily != SKERNEL_TWIST_C_FAMILY)
            {
                smemRegFldGet(devObjPtr, MAC_RNG_MIRR_CMD_REG, firstBit, 1,
                              &fldValue);
                /* Range DAMirror */
                cpuMirrorAction = fldValue;
            }
            else
            {
                cpuMirrorAction = 0;
            }
            if (daAction == TWIST_MAC_RANGE_NOT_ACTIVE_E)
                continue;
            else
                break;
        }
        else {
            memcpy(macAddrDest.bytes, descrPtr->dstMacPtr + 6, 6);
            SGT_MAC_ADDR_APPLY_MASK(&macAddrDest, &macAddrMask);
            if (SGT_MAC_ADDR_ARE_EQUAL(&macAddrDest, &macAddr)) {
                firstBit = RANGE_SACMD(range);
                smemRegFldGet(devObjPtr, MAC_RNG_FLTR_CMD_REG, firstBit, 2,
                              &fldValue);
                /* Range SACMD */
                saAction = fldValue;
                firstBit = RANGE_SAMIRROR(range);
                if (devObjPtr->deviceFamily != SKERNEL_TWIST_C_FAMILY)
                {
                    smemRegFldGet(devObjPtr, MAC_RNG_MIRR_CMD_REG, firstBit, 1,
                                  &fldValue);
                    /* Range2 DAMirror */
                    cpuMirrorAction = fldValue;
                }
                else
                {
                    cpuMirrorAction = 0;
                }
                if (saAction == TWIST_MAC_RANGE_NOT_ACTIVE_E)
                    continue;
                else
                    break;
            }
        }
    }
    if (cpuMirrorAction == 1) {
            descrPtr->macRangeCmd = SKERNEL_INTERVENTION_E;
    }
    if (daAction == TWIST_MAC_RANGE_TRAP_CPU_E ||
            saAction == TWIST_MAC_RANGE_TRAP_CPU_E) {
            descrPtr->macRangeCmd = SKERNEL_TRAP_CPU_E;
            descrPtr->controlTrap = 1;
    }
    else if (daAction == TWIST_MAC_RANGE_DROP_E ||
            saAction == TWIST_MAC_RANGE_DROP_E) {
            descrPtr->macRangeCmd = SKERNEL_DROP_E;
    }

}
/*******************************************************************************
*   snetTwistL2ControlFrame
*
* DESCRIPTION:
*        Control frame's processing
* INPUTS:
*        devObjPtr   -  pointer to device object.
*        descrPtr  -    pointer to the frame's descriptor.
*
* OUTPUTS:
*        descrPtr  -    pointer to the frame's descriptor.
*
*******************************************************************************/
GT_VOID snetTwistL2ControlFrame
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetTwistL2ControlFrame);

    GT_U16 etherType, etherTypeOffset;
    GT_U32 regAddress;
    GT_U32 fldValue;
    GT_U16 result;

    if ( (descrPtr->macDaType == SKERNEL_MULTICAST_MAC_E) &&
         (SGT_MAC_ADDR_IS_BPDU(descrPtr->dstMacPtr)) )
    {
        descrPtr->frameType = SKERNEL_BPDU_E;
        descrPtr->controlTrap = 1;
        descrPtr->pbduTrap = 1;
        return;
    }
    if (descrPtr->srcVlanTagged == 1) {
        etherTypeOffset = 16;
    }
    else {
        etherTypeOffset = 12;
    }
    etherType = descrPtr->dstMacPtr[etherTypeOffset] << 8 |
                descrPtr->dstMacPtr[etherTypeOffset + 1];

    if (etherType == 0x0806) {
        descrPtr->frameType = SKERNEL_ARP_E;

        if(descrPtr->macDaType == SKERNEL_BROADCAST_MAC_E)
        {/* this is not just broadcast -- it is also arp */
            __LOG(("this is not just broadcast -- it is also arp"));
            descrPtr->macDaType = SKERNEL_BROADCAST_ARP_E;
        }

        BRDG_PORT0_CTRL_REG(descrPtr->srcPort, &regAddress);
        smemRegFldGet(devObjPtr, regAddress, 13, 1, &fldValue);
        /* TrapARPEn */
        if (fldValue == 1) {
           descrPtr->arpTrap = 1;
        }
    }
    else if(etherType == 0x8847 || etherType == 0x8848)
    {
        descrPtr->frameType = (etherType == 0x8847) ?
        SKERNEL_UCAST_MPLS_E : SKERNEL_MCAST_MPLS_E;
        snetTwistL2MplsParse(devObjPtr, descrPtr, etherTypeOffset + 2);
    }
    else
    {

        if (descrPtr->ipHeaderPtr == NULL)
            return;

        SGT_IP_ADDR_IS_IGMP(descrPtr->ipHeaderPtr, &result);
        if (result == GT_TRUE) {
            descrPtr->frameType = SKERNEL_IGMP_E;
            /* Bridge Port<n> Control Register */
            BRDG_PORT0_CTRL_REG(descrPtr->srcPort, &regAddress);
            smemRegFldGet(devObjPtr, regAddress, 14, 1, &fldValue);
            /* TrapIGMPEn */
            if (fldValue == 1) {
                descrPtr->igmpTrap = 1;
                return;
            }
        }

        /* RIPv1 over MAC Broadcast */
        if (descrPtr->macDaType == SKERNEL_BROADCAST_MAC_E)
        {
            if (SGT_IP_BCST_ADDR_IS_RIPV1(descrPtr->ipHeaderPtr))
            {
                descrPtr->frameType = SKERNEL_FRAME_TYPE_RIPV1_E;
            }
        }
    }
}

/*******************************************************************************
*   snetTwistL2MacRange
*
* DESCRIPTION:
*        MAC range filtering
* INPUTS:
*        devObjPtr   -  pointer to device object.
*        descrPtr  -    pointer to the frame's descriptor.
*
* OUTPUTS:
*        descrPtr  -    pointer to the frame's descriptor.
*
*******************************************************************************/
GT_VOID snetTwistL2MacRange
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    GT_U32 regAddress;
    GT_U32 fldValue;

    if (descrPtr->controlTrap == 1)
        return;

    /* Bridge Port<n> Control Register */
    BRDG_PORT0_CTRL_REG(descrPtr->srcPort, &regAddress);
    smemRegFldGet(devObjPtr, regAddress, 10, 1, &fldValue);
    /* MACRange FilterEn */
    if (fldValue == 0)
        return;

    snetTwistMacRangeFilterGet(devObjPtr, descrPtr);
}

/*******************************************************************************
*   snetTwistL2FdbLookUp
*
* DESCRIPTION:
*        Look up in the bridge fdb
* INPUTS:
*        devObjPtr   -  pointer to device object.
*        descrPtr  -    pointer to the frame's descriptor.
*
* OUTPUTS:
*        descrPtr  -    pointer to the frame's descriptor.
*
*******************************************************************************/
GT_VOID snetTwistL2FdbLookUp
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetTwistL2FdbLookUp);

    GT_U32      regAddress; /* register field address */
    GT_U32      fldValue;   /* register field value */
    GT_U32      srcPort;    /* source port */
    GT_U8 *     srcMacPtr;  /* source mac address */
    GT_BOOL     found;      /* mac entry found */
    GT_U16      trafClass;  /* applying traffic class */
    GT_U16      vid;        /* vlan id */
    GT_U32      entryIdx; /* index of found entry */

    SNET_TWIST_MAC_TBL_STC macEntry; /* mac table entry */

    srcPort = descrPtr->srcPort;
    /* Bridge Port<n> Control Register */
    BRDG_PORT0_CTRL_REG(srcPort, &regAddress);
    smemRegFldGet(devObjPtr, regAddress, 2, 3, &fldValue);
    /* TrunkGroupId */
    __LOG(("TrunkGroupId"));
    if (fldValue)
        descrPtr->srcTrunkId = (GT_U16)fldValue;

    srcMacPtr = SRC_MAC_FROM_DSCR(descrPtr);
    vid = descrPtr->vid;
    trafClass = 0xFF;
    found = snetTwistFindMacEntry(devObjPtr, srcMacPtr, vid, &macEntry,
                                  &entryIdx);
    if (found) {
        descrPtr->macSaLookupResult = 1;
        descrPtr->saLookUpCmd = (GT_U8)macEntry.saCmd;
        trafClass = macEntry.saClass;
        macEntry.aging = 1;
        if (macEntry.trunk == 1) {
            if (macEntry.trunk != descrPtr->srcTrunkId) {
                if (macEntry.staticEntry == 1)
                    descrPtr->l2Move = SKERNEL_L2_STATIC_MOVE_E;
                else
                    descrPtr->l2Move = SKERNEL_L2_DYNAMIC_MOVE_E;
            }
        }
        else
        if (macEntry.port != descrPtr->srcPort) {
            if (macEntry.staticEntry == 1)
                descrPtr->l2Move = SKERNEL_L2_STATIC_MOVE_E;
            else
                descrPtr->l2Move = SKERNEL_L2_DYNAMIC_MOVE_E;
        }
    }
    if (descrPtr->controlTrap == 1)
        return;

        /* destination MAC lookup */
    found = snetTwistFindMacEntry(devObjPtr, descrPtr->dstMacPtr, vid,
                                  &macEntry, &entryIdx);
    if (found) {
        if (trafClass == 0xFF) {
            trafClass = macEntry.daClass;
        }
        else {
            trafClass = MAX(trafClass, macEntry.daClass);
        }
        descrPtr->daLookUpCmd = (GT_U8)macEntry.daCmd;
        descrPtr->macDaLookupResult = 1;
        descrPtr->doRout = macEntry.daRout;
        descrPtr->doPcl = descrPtr->doClassify = macEntry.daClass;

        if (macEntry.daCmd == 0) {
            if (macEntry.multiple && macEntry.staticEntry
                && descrPtr->macDaType == SKERNEL_UNICAST_MAC_E) {
                descrPtr->useVidx = 1;
                descrPtr->bits15_2.useVidx_1.vidx = macEntry.vidx;
            }
            else if (descrPtr->macDaType != SKERNEL_UNICAST_MAC_E)
            {
                descrPtr->useVidx = 1;
                descrPtr->bits15_2.useVidx_1.vidx = macEntry.vidx;
            }
            else
            {
                if (macEntry.trunk == 1) {
                    descrPtr->bits15_2.useVidx_0.targetIsTrunk = 1;
                    descrPtr->bits15_2.useVidx_0.targedPort =
                        (GT_U8)macEntry.trunk;
                }
                else {
                    descrPtr->bits15_2.useVidx_0.targetIsTrunk = 0;
                    descrPtr->bits15_2.useVidx_0.targedPort =
                        (GT_U8)macEntry.port;
                    descrPtr->bits15_2.useVidx_0.targedDevice =
                        (GT_U8)macEntry.dev;
                }
            }
        }
    }
    else
    if (trafClass != 0xFF) {
        descrPtr->trafficClass = (GT_U8)trafClass;
    }
}

/*******************************************************************************
*   snetTwistFindMacEntry
*
* DESCRIPTION:
*        Calculates the hash index for the mac address table
* INPUTS:
*        macAddrPtr  -  pointer to the first byte of MAC address.
*        vid         -  vlan tag.
*        isVlanUsed  -  use vlan tag.
* RETURNS:
*        The hash index
*******************************************************************************/
GT_U32 snetTwistMacHashCalc
(
    IN GT_U8 * macAddrPtr,
    IN GT_U16 vid,
    IN SGT_MAC_HASH  * macHashStructPtr
)
{
    GT_ETHERADDR addr;
    GT_U32 hash;
    GT_U8 retVal;

    memcpy(addr.arEther, macAddrPtr, 6);
    retVal = sgtMacHashCalc(&addr, vid, macHashStructPtr, &hash);
    if (GT_BAD_PARAM == retVal)
    {
        skernelFatalError("snetTwistMacHashCalc : bad parameters\n");
    }

    return hash;
}

/*******************************************************************************
*   snetTwistFindMacEntry
*
* DESCRIPTION:
*        Look up in the MAC table
* INPUTS:
*        devObjPtr   -  pointer to device object.
*        macAddrPtr  -  pointer to the first byte of MAC address.
*        vid         -  vlan tag.
* OUTPUTS:
*        TRUE - MAC entry found, FALSE - MAC entry not found
*
*******************************************************************************/
GT_BOOL snetTwistFindMacEntry
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U8 * macAddrPtr,
    IN GT_U16 vid,
    OUT SNET_TWIST_MAC_TBL_STC * twistMacEntryPtr,
    OUT GT_U32  * entryIdxPtr
)
{
    GT_U32 regAddress;  /* register address */
    GT_U32 hashIdx;     /* mac entry hush index */
    GT_U32 hashBucketSize; /* size of mac hush table */
    GT_U16 i;
    SGT_MAC_HASH macHashStruct; /* mac entry structure */
    SGT_MAC_ADDR_UNT macAddr;   /* mac address */

    /* Fill  SGT_MAC_HASH with registries values */
    snetTwistMacHashInit(devObjPtr, &macHashStruct);

    hashIdx = snetTwistMacHashCalc(macAddrPtr, vid, &macHashStruct);

    /* NumOfEntries */
    hashBucketSize = (macHashStruct.macChainLen + 1) * 2;
    if(hashBucketSize + hashIdx > 0x4000)
    {
        hashBucketSize = 0x4000 - hashIdx;
    }
    for (i = 0; i < hashBucketSize; i++) {
        regAddress = MAC_TAB_ENTRY_WORD0_REG + (hashIdx + i) * 0x10;
        snetTwistGetMacEntry(devObjPtr, regAddress, twistMacEntryPtr);

        if (twistMacEntryPtr->validEntry && twistMacEntryPtr->skipEntry == 0) {
            memcpy(macAddr.bytes, macAddrPtr, 6);
            if (SGT_MAC_ADDR_ARE_EQUAL(&macAddr, &twistMacEntryPtr->macAddr)) {
                if (macHashStruct.vlanMode && twistMacEntryPtr->vid == vid)
                {
                    /* entry found */
                    if (entryIdxPtr != NULL)
                        *entryIdxPtr = hashIdx + i;

                    return GT_TRUE;
                }
            }
        }
    }
    return GT_FALSE;
}
/*******************************************************************************
*   snetTwistGetFreeMacEntryAddr
*
* DESCRIPTION:
*        Look up in the MAC table for the first free MAC entry address
* INPUTS:
*        devObjPtr   -  pointer to device object.
*        macAddrPtr  -  pointer to the first byte of MAC address.
*        vid         -  vlan tag.
* OUTPUTS:
*******************************************************************************/
GT_U32 snetTwistGetFreeMacEntryAddr
(
    IN  SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN  GT_U8 * macAddrPtr,
    IN  GT_U16 vid
)
{
    GT_U32 regAddress;
    GT_U32 fldValue;
    GT_U32 hashIdx;
    GT_U32 hashBucketSize;
    GT_U16 i;
    SGT_MAC_HASH macHashStruct;

    SNET_TWIST_MAC_TBL_STC twistMacEntryPtr;

    ASSERT_PTR(devObjPtr);
    ASSERT_PTR(macAddrPtr);

    /* Fill  SGT_MAC_HASH with registers values */
    snetTwistMacHashInit(devObjPtr, &macHashStruct);

    hashIdx = snetTwistMacHashCalc(macAddrPtr, vid, &macHashStruct);

    smemRegFldGet(devObjPtr, L2_INGRESS_CTRL_REG, 0, 3, &fldValue);
    /* NumOfEntries */
    hashBucketSize = (fldValue + 1) * 2;
    for (i = 0; i < hashBucketSize; i++) {
        regAddress = MAC_TAB_ENTRY_WORD0_REG + (hashIdx + i) * 0x10;
        snetTwistGetMacEntry(devObjPtr, regAddress, &twistMacEntryPtr);
        if (twistMacEntryPtr.validEntry == 0 || twistMacEntryPtr.skipEntry)
            return regAddress;
    }

    return NOT_VALID_ADDR;
}

/*******************************************************************************
* snetSetMacEntry
*
* DESCRIPTION:
*        Set MAC table new entry by free MAC address
* INPUTS:
*        devObjPtr   -  pointer to device object.
*        regAddress  -  MAC entry address.
*        twistMacEntryPtr - MAC entry pointer
*******************************************************************************/
GT_VOID snetTwistSetMacEntry
(
    IN SKERNEL_DEVICE_OBJECT *  devObjPtr,
    IN GT_U32                   regAddress,
    IN SNET_TWIST_MAC_TBL_STC * twistMacEntryPtr
)
{
    GT_U32 hwData[4];       /* 4 words of mac address hw entry  */

    memset(hwData, 0, 4 * sizeof(GT_U32));

    /* MAC table entry word0 */
    hwData[0] =
       (twistMacEntryPtr->validEntry |
        twistMacEntryPtr->skipEntry << 1 |
        twistMacEntryPtr->aging << 2 |
        twistMacEntryPtr->trunk << 3 |
        twistMacEntryPtr->vid << 4 |
        (SGT_LIB_SWAP16_MAC(twistMacEntryPtr->macAddr.w.hword2)<<16));


    /* MAC table entry word1 */
    hwData[1] = SGT_LIB_SWAP_BYTES_AND_WORDS_MAC(twistMacEntryPtr->macAddr.w.word1);

    /* MAC table entry word2          */
    hwData[2] =
       (twistMacEntryPtr->dev |
        twistMacEntryPtr->port << 8 |
        twistMacEntryPtr->vidx << 16);

    /* MAC table entry word3            */
    hwData[3] =
       (twistMacEntryPtr->srcTc |
        twistMacEntryPtr->dstTc << 4 |
        twistMacEntryPtr->staticEntry << 8 |
        twistMacEntryPtr->multiple << 9 |
        twistMacEntryPtr->daCmd << 12 |
        twistMacEntryPtr->saCmd << 14 |
        twistMacEntryPtr->saClass << 16 |
        twistMacEntryPtr->saCib << 17 |
        twistMacEntryPtr->daClass << 20 |
        twistMacEntryPtr->daCib << 21 |
        twistMacEntryPtr->daRout << 22);

    smemMemSet(devObjPtr, regAddress, hwData, 3);
}
/*******************************************************************************
*   snetTwistGetMacEntry
*
* DESCRIPTION:
*        Get MAC table entry by index in the hash table
* INPUTS:
*        devObjPtr   -  pointer to device object.
*        regAddress  -  MAC entry address.
* OUTPUTS:
*        twistMacEntryPtr - pointer to the MAC table entry or NULL if not found.
*
*******************************************************************************/
GT_VOID snetTwistGetMacEntry
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 regAddress,
    OUT SNET_TWIST_MAC_TBL_STC * twistMacEntryPtr
)
{
    GT_U32 * entryPtr;

    memset(twistMacEntryPtr, 0, sizeof(SNET_TWIST_MAC_TBL_STC));

    entryPtr = smemMemGet(devObjPtr, regAddress);

    /* Get mac address*/
    sfdbMsg2Mac(entryPtr, &twistMacEntryPtr->macAddr);

        /* MAC table entry word0 */
        /* Valid/Invalid entry */
    twistMacEntryPtr->validEntry =
                        (GT_U16)WORD_FIELD_GET(entryPtr, 0, 0, 1);

        /* Skip (used to delete this entry) */
    twistMacEntryPtr->skipEntry =
                        (GT_U16)WORD_FIELD_GET(entryPtr, 0, 1, 1);

        /* Aging - This bit is used for the Aging process. */
    twistMacEntryPtr->aging =
                        (GT_U16)WORD_FIELD_GET(entryPtr, 0, 2, 1);

        /* Trunk Indicator */
    twistMacEntryPtr->trunk =
                        (GT_U16)WORD_FIELD_GET(entryPtr, 0, 3, 1);

        /* VLAN id - holds the VLAN identification number. */
    twistMacEntryPtr->vid =
                        (GT_U16)WORD_FIELD_GET(entryPtr, 0, 4, 12);

        /* MAC table entry word2 */
        /* Dev# */
    twistMacEntryPtr->dev =
                        (GT_U16)WORD_FIELD_GET(entryPtr, 2, 0, 7);

        /* port# / trunk# */
    twistMacEntryPtr->port =
                        (GT_U16)WORD_FIELD_GET(entryPtr, 2, 8, 6);

        /* Multicast group table index. */
    twistMacEntryPtr->vidx =
                        (GT_U16)WORD_FIELD_GET(entryPtr, 2, 16, 14);

        /* MAC table entry word3 */
        /* Traffic Class associated with a packet with this source MAC */
    twistMacEntryPtr->srcTc =
                        (GT_U8)WORD_FIELD_GET(entryPtr, 3, 0, 3);

        /* Traffic Class associated with a packet with this dest. MAC */
    twistMacEntryPtr->dstTc =
                        (GT_U8)WORD_FIELD_GET(entryPtr, 3, 4, 3);

    /* Static entry */
    twistMacEntryPtr->staticEntry =
                        (GT_U8)WORD_FIELD_GET(entryPtr, 3, 8, 1);

        /* Multiply */
    twistMacEntryPtr->multiple =
                        (GT_U8)WORD_FIELD_GET(entryPtr, 3, 9, 1);

        /* DA_CMD */
    twistMacEntryPtr->daCmd =
                        (GT_U16)WORD_FIELD_GET(entryPtr, 3, 12, 2);

        /* SA_CMD */
    twistMacEntryPtr->saCmd =
                        (GT_U16)WORD_FIELD_GET(entryPtr, 3, 14, 2);

        /* SA class */
    twistMacEntryPtr->saClass =
                        (GT_BOOL)WORD_FIELD_GET(entryPtr, 3, 16, 1);
        /* send the packet to the Customer Interface Bus*/
    twistMacEntryPtr->saCib =
                        (GT_BOOL)WORD_FIELD_GET(entryPtr, 3, 17, 1);
        /* DA_class */
    twistMacEntryPtr->daClass =
                        (GT_BOOL)WORD_FIELD_GET(entryPtr, 3, 20, 1);
        /* send the packet to the Customer Interface Bus*/
    twistMacEntryPtr->daCib =
                        (GT_BOOL)WORD_FIELD_GET(entryPtr, 3, 21, 1);

        /* DA_route */
    twistMacEntryPtr->daRout =
                        (GT_BOOL)WORD_FIELD_GET(entryPtr, 3, 22, 1);
}

/*******************************************************************************
*   snetTwistL2LockPort
*
* DESCRIPTION:
*        Lock port processing
* INPUTS:
*        devObjPtr   -  pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
* OUTPUTS:
*        descrPtr  - pointer to the frame's descriptor.
*
*******************************************************************************/
GT_VOID snetTwistL2LockPort
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetTwistL2LockPort);

    GT_U32 regAddress;
    GT_U32 fldValue;

    /* Bridge Port<n> Control Register */
    BRDG_PORT0_CTRL_REG(descrPtr->srcPort, &regAddress);
    smemRegFldGet(devObjPtr, regAddress, 15, 1, &fldValue);
    /* Locked Port */
    __LOG(("Locked Port"));
    descrPtr->islockPort = (GT_U8)fldValue;
    descrPtr->learnEn = (GT_U8)fldValue^1;

    if (descrPtr->learnEn == 1)
        return;

    if (descrPtr->macSaLookupResult == 0) {
        smemRegFldGet(devObjPtr, regAddress, 16, 2, &fldValue);
        /* LockedPort CMD */
        if (fldValue != 0) {
            descrPtr->lockPortCmd = (GT_U8)fldValue;
        }
    }
    else
    if (descrPtr->l2Move != SKERNEL_L2_NO_MOVE_E)
    {
        smemRegFldGet(devObjPtr, regAddress, 16, 2, &fldValue);
        /* LockedPort CMD */
        if (fldValue != 0) {
            descrPtr->lockPortCmd = (GT_U8)fldValue;
        }
    }
}

/*******************************************************************************
*   snetTwistL2IngressFilter
*
* DESCRIPTION:
*        STP and ingress filter processing
* INPUTS:
*        devObjPtr   -  pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
* OUTPUTS:
*        descrPtr  - pointer to the frame's descriptor.
*
*******************************************************************************/
GT_VOID snetTwistL2IngressFilter
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetTwistL2IngressFilter);

    GT_U32 regAddress;
    GT_U32 fldValue;
    GT_U32 fldFirstBit;
    GT_U32 vlanValid;
    GT_U32 stgId;
    GT_U32 srcPort;
    GT_U32 stpState;

    GT_U8 vlan_ports[64] = {0};
    GT_U8 tagged_ports[64] = {0};
    GT_U8 port_stp[64] = {0};

    srcPort = descrPtr->srcPort;
    /* Ports VLAN Table -- get first word of entry */
    PORTS_VLAN_TABLE_REG(descrPtr->vid , 0, &regAddress);
    smemRegFldGet(devObjPtr, regAddress, 0, 1, &fldValue);
    /* Valid bit for each VLAN Entry (for Ingress filtering) */
    vlanValid = fldValue;
    smemRegFldGet(devObjPtr, regAddress, 12, 8, &fldValue);
    /* Pointer to the Span State table */
    stgId = fldValue;

    if (vlanValid == 1) {
        smemRegFldGet(devObjPtr, regAddress, 1, 1, &fldValue);
        /* learn_en */
        descrPtr->learnEn &= (GT_U8)fldValue;
    }
    /* Bridge Port<n> Control Register */
    BRDG_PORT0_CTRL_REG(srcPort, &regAddress);
    smemRegFldGet(devObjPtr, regAddress, 9, 1, &fldValue);
    /* Secure VLAN */
    __LOG(("Secure VLAN"));
    if (fldValue == 1) {
        if (vlanValid == 1) {
            snetTwistVltTables(devObjPtr, descrPtr,
                               vlan_ports, tagged_ports, port_stp);
        }
        if (vlan_ports[srcPort] != 1 || vlanValid != 1) {
            descrPtr->ingressFilterOut = 1;
            return;
        }
    }
    smemRegFldGet(devObjPtr, L2_INGRESS_CTRL_REG, 5, 1, &fldValue);
    /* PerVLAN SpanEn */
    if (fldValue == 1) {
        if (vlanValid == 1) {
            SPAN_STATE_TABLE_REG(stgId, srcPort, &regAddress);
            fldFirstBit = SPAN_STATE_TABLE_PORT(srcPort);
            smemRegFldGet(devObjPtr, regAddress, fldFirstBit, 2, &fldValue);
            /* port0_span_state */
            stpState = fldValue;
        }
        else {
            /* Forwarding */
            stpState = SKERNEL_STP_FORWARD_E;
        }
    }
    else {
        BRDG_PORT0_CTRL_REG(srcPort, &regAddress);
        smemRegFldGet(devObjPtr, regAddress, 0, 2, &fldValue);
        /* STPortState */
        stpState = fldValue;
    }

    if (stpState == SKERNEL_STP_BLOCK_LISTEN_E) {
        descrPtr->learnEn = 0;
    }

    descrPtr->stpState = (GT_U8)stpState;
}
/*******************************************************************************
*   snetTwistL2Decision
*
* DESCRIPTION:
*        snetTwistL2Decision
* INPUTS:
*        devObjPtr   -  pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
* OUTPUTS:
*        descrPtr  - pointer to the frame's descriptor.
*
*******************************************************************************/
GT_VOID snetTwistL2Decision
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    GT_U32 dropCode;
    GT_U32 cpuCode;
    GT_U32 securityDrop;

    dropCode = 0;
    securityDrop = 0;

    if (descrPtr->pbduTrap == 1) {
        /* Check STP state - if disabled than process BPDU as multicast */
        if (descrPtr->stpState != SKERNEL_STP_DISABLED_E){
            cpuCode = TWIST_CPU_CODE_BPDU;
            snetTwistTrapPacket(descrPtr, cpuCode);
            return;
        }
    }
    else
    if (descrPtr->daLookUpCmd == SKERNEL_TRAP_CPU_E
        && descrPtr->saLookUpCmd != SKERNEL_TRAP_CPU_E) {
        cpuCode = TWIST_CPU_CODE_MAC_ADDR_TRAP;
        snetTwistTrapPacket(descrPtr, cpuCode);
        return;
    }
    else
    if (descrPtr->daLookUpCmd != SKERNEL_TRAP_CPU_E
        && descrPtr->saLookUpCmd == SKERNEL_TRAP_CPU_E) {
        cpuCode = TWIST_CPU_CODE_MAC_ADDR_TRAP;
        snetTwistTrapPacket(descrPtr, cpuCode);
        return;
    }
    if (descrPtr->daLookUpCmd == SKERNEL_TRAP_CPU_E
        && descrPtr->saLookUpCmd == SKERNEL_TRAP_CPU_E) {
        cpuCode = TWIST_CPU_CODE_MAC_ADDR_TRAP;
        snetTwistTrapPacket(descrPtr, cpuCode);
        return;
    }
    else
    if (descrPtr->macRangeCmd == SKERNEL_TRAP_CPU_E) {
        cpuCode = TWIST_CPU_CODE_MAC_RANGE_TRAP;
        snetTwistTrapPacket(descrPtr, cpuCode);
        return;
    }
    else
    if (descrPtr->daLookUpCmd == SKERNEL_DROP_E ||
        descrPtr->saLookUpCmd == SKERNEL_DROP_E ||
        descrPtr->lockPortCmd == SKERNEL_DROP_E ||
        descrPtr->macRangeCmd == SKERNEL_DROP_E ||
        descrPtr->invalidSaDrop == SKERNEL_DROP_E)
    {
        securityDrop = 1;
    }
    if (securityDrop == 1) {
        dropCode = 2;
        snetTwistSecurityDropPacket(descrPtr, dropCode);
        return;
    }
    else
    if (descrPtr->ingressFilterOut == 1) {
        dropCode = 3;
        snetTwistIngressDropPacket(descrPtr, dropCode);
        return;
    }
    else
    if (descrPtr->stpState != SKERNEL_STP_FORWARD_E
        && descrPtr->stpState != SKERNEL_STP_DISABLED_E) {
        dropCode = 4;
        snetTwistRegularDropPacket(descrPtr, dropCode);
        snetTwistLearnSa(devObjPtr, descrPtr);
        return;
    }
    else
    if (descrPtr->arpTrap == 1) {
        if (descrPtr->macDaType == SKERNEL_BROADCAST_MAC_E
            || descrPtr->macDaType == SKERNEL_BROADCAST_ARP_E)
            cpuCode = TWIST_CPU_CODE_ARP_BROADCAST;
        else
            cpuCode = TWIST_CPU_CODE_CONTROL_TO_CPU;

        snetTwistTrapPacket(descrPtr, cpuCode);
        return;
    }
    else
    if (descrPtr->igmpTrap == 1) {
        cpuCode = TWIST_CPU_CODE_IGMP_PACKET;
        snetTwistTrapPacket(descrPtr, cpuCode);
        return;
    }
    else
    if (descrPtr->saLookUpCmd == SKERNEL_INTERVENTION_E
        && descrPtr->daLookUpCmd != SKERNEL_INTERVENTION_E) {
        cpuCode = TWIST_CPU_CODE_INTERVENTION_MAC_ADDR;
        snetTwistTrapPacket(descrPtr, cpuCode);
        return;
    }
    else
    if (descrPtr->saLookUpCmd != SKERNEL_INTERVENTION_E
        && descrPtr->daLookUpCmd == SKERNEL_INTERVENTION_E) {
        cpuCode = TWIST_CPU_CODE_INTERVENTION_MAC_ADDR;
        snetTwistTrapPacket(descrPtr, cpuCode);
        return;
    }
    else
    if (descrPtr->saLookUpCmd == SKERNEL_INTERVENTION_E
        && descrPtr->daLookUpCmd == SKERNEL_INTERVENTION_E) {
        cpuCode = TWIST_CPU_CODE_INTERVENTION_MAC_ADDR;
        snetTwistTrapPacket(descrPtr, cpuCode);
        return;
    }
    else
    if (descrPtr->lockPortCmd == SKERNEL_INTERVENTION_E) {
        cpuCode = TWIST_CPU_CODE_INTERVENTION_LOCK_TO_CPU;
        snetTwistTrapPacket(descrPtr, cpuCode);
        snetTwistLearnSa(devObjPtr, descrPtr); /* send SA message */
        return;
    }
    else
    if (descrPtr->macRangeCmd == SKERNEL_INTERVENTION_E)
    {
        cpuCode = TWIST_CPU_CODE_MAC_RANGE_TRAP;
        snetTwistTrapPacket(descrPtr, cpuCode);
        return;
    }


    /* learn source address */
    snetTwistLearnSa(devObjPtr, descrPtr);

    /* Regular forward frames */
    if (descrPtr->macDaLookupResult == 1 && descrPtr->useVidx == 0)
    {
        snetTwistFwdUcastDest(devObjPtr, descrPtr);
    }
    else
    {
        snetTwistFwdMcastDest(descrPtr);
    }
}

/*******************************************************************************
*   snetTwistFwdUcastDest
*
* DESCRIPTION:
*        Forward unicast packet
* INPUTS:
*        descrPtr  - pointer to the frame's descriptor.
* OUTPUTS:
*        descrPtr  - pointer to the frame's descriptor.
*
*******************************************************************************/
static GT_VOID  snetTwistFwdUcastDest
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    GT_U32 regAddress;
    GT_U32 fldValue;
    GT_U32 fldFirstBit;
    GT_U16 numOfOnes;
    GT_U16 portsInTrunk;
    GT_U8 * dataPtr;
    GT_U8 * macAddrPtr;
    SGT_MAC_ADDR_TYP macDst;
    SGT_MAC_ADDR_TYP macSrc;


    macAddrPtr = macDst.bytes;
    dataPtr = DST_MAC_FROM_DSCR(descrPtr);
    MEM_APPEND(macAddrPtr, dataPtr,  SGT_MAC_ADDR_BYTES);

    macAddrPtr = macSrc.bytes;
    dataPtr = SRC_MAC_FROM_DSCR(descrPtr);
    MEM_APPEND(macAddrPtr, dataPtr,  SGT_MAC_ADDR_BYTES);

    descrPtr->pktCmd = SKERNEL_PKT_FORWARD_E;
    descrPtr->useVidx = 0;
    descrPtr->mdb = 0;

    if (descrPtr->bits15_2.useVidx_0.targetIsTrunk) {
        GT_U32 trunkId;
        trunkId =  descrPtr->bits15_2.useVidx_0.targedPort;
        TRUNK_TABLE_REG(devObjPtr->deviceFamily, trunkId, &regAddress);
        fldFirstBit = TRUNK_TABLE_MEMBER_NUM(devObjPtr->deviceFamily, trunkId);
        smemRegFldGet(devObjPtr, regAddress, fldFirstBit, 4, &fldValue);
        /* Trunk <trunkId> MembersNum */
        portsInTrunk = (GT_U16)fldValue;
        numOfOnes = snetTwistTrunkHash(&macDst, &macSrc, portsInTrunk);
        descrPtr->bits15_2.useVidx_0.trunkHash = (GT_U8)numOfOnes;
    }
}

/*******************************************************************************
*   snetTwistTrunkHash
*
* DESCRIPTION:
*        Hash for trunk ports
* INPUTS:
*        devObjPtr   -  pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
* OUTPUTS:
*        descrPtr  - pointer to the frame's descriptor.
*
*******************************************************************************/
GT_U16 snetTwistTrunkHash
(
    IN SGT_MAC_ADDR_TYP * macDstPtr,
    IN SGT_MAC_ADDR_TYP * macSrcPtr,
    IN GT_U16 portsInTrunk
)
{
    GT_U16 firstBit;
    GT_U16 numOfOnes;
    GT_U16 i;

    numOfOnes = 0;
    macDstPtr->bytes[5] ^= macSrcPtr->bytes[5];
    firstBit = (GT_U8)(47 - portsInTrunk) + 1;
    for(i = firstBit%8; i < 8; i++) {
        if (macDstPtr->bytes[5] & (1 << i)) {
            numOfOnes++;
        }
    }
    if (portsInTrunk == numOfOnes) {
        numOfOnes = 0;
        for(i = 5; i < 8; i++) {
            if (macSrcPtr->bytes[4] & (1 << i)) {
                numOfOnes++;
            }
        }
        for(i = 0; i < 8; i++) {
            if (macSrcPtr->bytes[5] & (1 << i)) {
                numOfOnes++;
            }
        }
    }
    return numOfOnes;
}
/*******************************************************************************
*   snetTwistFwdMcastDest
*
* DESCRIPTION:
*        Forward multicast packet
* INPUTS:
*        devObjPtr   -  pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
* OUTPUTS:
*        descrPtr  - pointer to the frame's descriptor.
*
*******************************************************************************/
static GT_VOID  snetTwistFwdMcastDest
(
    IN SKERNEL_FRAME_DESCR_STC * descrPtr
)
{

    descrPtr->pktCmd = SKERNEL_PKT_FORWARD_E;
    descrPtr->useVidx = 1;
    descrPtr->mdb = 1;
}
/*******************************************************************************
*   snetTwistDropPacket
*
* DESCRIPTION:
*        Drop packet
* INPUTS:
*        descrPtr  - pointer to the frame's descriptor.
*        dropCode  - drop code
* OUTPUTS:
*        descrPtr  - pointer to the frame's descriptor.
*
*******************************************************************************/
static GT_VOID  snetTwistDropPacket
(
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U32 dropCode
)
{

    descrPtr->pktCmd = SKERNEL_PKT_DROP_E;
    descrPtr->useVidx = 0;
    descrPtr->bits15_2.useVidx_1.vidx = (GT_U16)dropCode;
}
/*******************************************************************************
*   snetTwistTrapPacket
*
* DESCRIPTION:
*        Drop packet
* INPUTS:
*        descrPtr  - pointer to the frame's descriptor.
*        cpuCode  -  drop code
* OUTPUTS:
*        descrPtr  - pointer to the frame's descriptor.
*
*******************************************************************************/
static GT_VOID  snetTwistTrapPacket
(
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U32 cpuCode
)
{

    descrPtr->pktCmd = SKERNEL_PKT_TRAP_CPU_E;
    descrPtr->useVidx = 0;
    descrPtr->bits15_2.cmd_trap2cpu.cpuCode = (GT_U16)cpuCode;
}
/*******************************************************************************
*   snetTwistSecurityDropPacket
*
* DESCRIPTION:
*        Security drop packet
* INPUTS:
*        descrPtr  - pointer to the frame's descriptor.
*        dropCode  - drop code
* OUTPUTS:
*        descrPtr  - pointer to the frame's descriptor.
*
*******************************************************************************/
static GT_VOID  snetTwistSecurityDropPacket
(
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN    GT_U32 dropCode
)
{
    snetTwistDropPacket(descrPtr, dropCode);
}

/*******************************************************************************
*   snetTwistIngressDropPacket
*
* DESCRIPTION:
*        Security drop packet
* INPUTS:
*        descrPtr  - pointer to the frame's descriptor.
*        dropCode  - drop code
* OUTPUTS:
*        descrPtr  - pointer to the frame's descriptor.
*
*******************************************************************************/
static GT_VOID  snetTwistIngressDropPacket
(
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U32 dropCode
)
{
    snetTwistDropPacket(descrPtr, dropCode);
}
/*******************************************************************************
*   snetTwistRegularDropPacket
*
* DESCRIPTION:
*        Regular drop packet
* INPUTS:
*        descrPtr  - pointer to the frame's descriptor.
*        dropCode  - drop code
* OUTPUTS:
*        descrPtr  - pointer to the frame's descriptor.
*
*******************************************************************************/
GT_VOID  snetTwistRegularDropPacket
(
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U32 dropCode
)
{
    snetTwistDropPacket(descrPtr, dropCode);
}
/*******************************************************************************
*   snetTwistFdbAuSend
*
* DESCRIPTION:
*        Send address update message
* INPUTS:
*        devObjPtr  -  pointer to device object.
*        hwData[4]  -  pointer to CPU message
* OUTPUTS:
*        setIntrPtr -  set interrupt flag
*
*
*******************************************************************************/
GT_BOOL snetTwistFdbAuSend(
    IN SKERNEL_DEVICE_OBJECT   * devObjPtr,
    IN GT_U32 hwData[4],
    OUT GT_BOOL  * setIntrPtr
)
{
    GT_U32 fldValue;    /* register field value */
    GT_U32 auqSize;     /* address update queue in bytes */
    GT_U32 * auqMemPtr; /* pointer to AUQ queue */

    TIGER_DEV_MEM_INFO * devMemTigerPtr;    /* tiger memory pointer */
    TWIST_DEV_MEM_INFO * devMemTwistrPtr;   /* twist memory pointer */

    if(devObjPtr->deviceFamily == SKERNEL_TIGER_FAMILY)
    {
        devMemTigerPtr = (TIGER_DEV_MEM_INFO *)devObjPtr->deviceMemory;
        /* Check validity of AUQ base segment */
        if (devMemTigerPtr->auqMem.auqBaseValid == GT_FALSE)
        {
             /* set interrupt AUQFull if not masked*/
            smemRegFldGet(devObjPtr, MISC_INTR_MASK_REG, 2, 1, &fldValue);
            /* AUQPending Mask */
            if (fldValue) {
                smemRegFldSet(devObjPtr, MISC_INTR_CAUSE_REG, 2, 1, 1);
                smemRegFldSet(devObjPtr, MISC_INTR_CAUSE_REG, 0, 1, 1);
                *setIntrPtr = GT_TRUE;
            }
            return GT_FALSE;
        }

        smemRegFldGet(devObjPtr, AUQ_CONTROL_REG, 0, 31, &fldValue);
        /* AUQSize */
        auqSize = fldValue;
        *setIntrPtr = GT_FALSE;
        if (devMemTigerPtr->auqMem.auqOffset < auqSize) {
            auqMemPtr = (GT_U32*)(devMemTigerPtr->auqMem.auqBase +
                        (devMemTigerPtr->auqMem.auqOffset) * 4 * sizeof(GT_U32));
            /*memcpy(auqMemPtr, hwData, 4 * sizeof(GT_U32));*/
            scibDmaWrite(devObjPtr->deviceId,(GT_U32)((GT_UINTPTR)auqMemPtr),4, hwData,SCIB_DMA_WORDS);

            devMemTigerPtr->auqMem.auqOffset++;

            /* set interrupt AUQPending if not masked*/
            smemRegFldGet(devObjPtr, MISC_INTR_MASK_REG, 1, 1, &fldValue);
            /* AUQPending Mask */
            if (fldValue) {
                smemRegFldSet(devObjPtr, MISC_INTR_CAUSE_REG, 1, 1, 1);
                smemRegFldSet(devObjPtr, MISC_INTR_CAUSE_REG, 0, 1, 1);
                *setIntrPtr = GT_TRUE;
            }
        }
        else {
             /* set interrupt AUQFull if not masked*/
            smemRegFldGet(devObjPtr, MISC_INTR_MASK_REG, 2, 1, &fldValue);
            /* AUQPending Mask */
            if (fldValue) {
                smemRegFldSet(devObjPtr, MISC_INTR_CAUSE_REG, 2, 1, 1);
                smemRegFldSet(devObjPtr, MISC_INTR_CAUSE_REG, 0, 1, 1);
                *setIntrPtr = GT_TRUE;
            }
            return GT_FALSE;
        }

        if (devMemTigerPtr->auqMem.auqOffset >= auqSize) {

            devMemTigerPtr->auqMem.auqBaseValid = GT_FALSE;
            if (devMemTigerPtr->auqMem.auqShadowValid == GT_FALSE)
            {
                return GT_TRUE;
            }

            devMemTigerPtr->auqMem.auqBase = devMemTigerPtr->auqMem.auqShadow;
            devMemTigerPtr->auqMem.auqBaseValid = GT_TRUE;
            devMemTigerPtr->auqMem.auqOffset = 0;
        }
    }
    else
    {
        devMemTwistrPtr = (TWIST_DEV_MEM_INFO *)devObjPtr->deviceMemory;
        /* Check validity of AUQ base segment */
        if (devMemTwistrPtr->auqMem.auqBaseValid == GT_FALSE)
        {
             /* set interrupt AUQFull if not masked*/
            smemRegFldGet(devObjPtr, MISC_INTR_MASK_REG, 2, 1, &fldValue);
            /* AUQPending Mask */
            if (fldValue) {
                smemRegFldSet(devObjPtr, MISC_INTR_CAUSE_REG, 2, 1, 1);
                smemRegFldSet(devObjPtr, MISC_INTR_CAUSE_REG, 0, 1, 1);
                *setIntrPtr = GT_TRUE;
            }
            return GT_FALSE;
        }

        smemRegFldGet(devObjPtr, AUQ_CONTROL_REG, 0, 31, &fldValue);
        /* AUQSize */
        auqSize = fldValue;
        *setIntrPtr = GT_FALSE;
        if (devMemTwistrPtr->auqMem.auqOffset < auqSize) {
            auqMemPtr = (GT_U32*)(devMemTwistrPtr->auqMem.auqBase +
                        (devMemTwistrPtr->auqMem.auqOffset) * 4 * sizeof(GT_U32));
            /*memcpy(auqMemPtr, hwData, 4 * sizeof(GT_U32));*/
            scibDmaWrite(devObjPtr->deviceId,(GT_U32)((GT_UINTPTR)auqMemPtr),4, hwData,SCIB_DMA_WORDS);
            devMemTwistrPtr->auqMem.auqOffset++;

            /* set interrupt AUQPending if not masked*/
            smemRegFldGet(devObjPtr, MISC_INTR_MASK_REG, 1, 1, &fldValue);
            /* AUQPending Mask */
            if (fldValue) {
                smemRegFldSet(devObjPtr, MISC_INTR_CAUSE_REG, 1, 1, 1);
                smemRegFldSet(devObjPtr, MISC_INTR_CAUSE_REG, 0, 1, 1);
                *setIntrPtr = GT_TRUE;
    /*            osPrintf("snetTwistFdbAuSend interrupt\n")); */
            }
        }
        else {
             /* set interrupt AUQFull if not masked*/
            smemRegFldGet(devObjPtr, MISC_INTR_MASK_REG, 2, 1, &fldValue);
            /* AUQPending Mask */
            if (fldValue) {
                smemRegFldSet(devObjPtr, MISC_INTR_CAUSE_REG, 2, 1, 1);
                smemRegFldSet(devObjPtr, MISC_INTR_CAUSE_REG, 0, 1, 1);
                *setIntrPtr = GT_TRUE;
            }
            return GT_FALSE;
        }

        if (devMemTwistrPtr->auqMem.auqOffset >= auqSize) {

            devMemTwistrPtr->auqMem.auqBaseValid = GT_FALSE;
            if (devMemTwistrPtr->auqMem.auqShadowValid == GT_FALSE)
            {
                return GT_TRUE;
            }

            devMemTwistrPtr->auqMem.auqBase = devMemTwistrPtr->auqMem.auqShadow;
            devMemTwistrPtr->auqMem.auqBaseValid = GT_TRUE;
            devMemTwistrPtr->auqMem.auqOffset = 0;
        }
    }

    return GT_TRUE;
}
/*******************************************************************************
*   snetTwistCreateAuMsg
*
* DESCRIPTION:
*        Set MAC Update Message by free FIFO entry pointer
* INPUTS:
*        devObjPtr   -  pointer to device object.
*        freeFifoEntryPtr  -  FIFI entry pointer.
*        twistMacEntryPtr - MAC entry pointer
*        hwData[4] - New address message data
*
*******************************************************************************/
GT_VOID snetTwistCreateAuMsg
(
    IN  SKERNEL_DEVICE_OBJECT   * devObjPtr,
    IN  SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN  SNET_TWIST_MAC_TBL_STC  * twistMacEntryPtr,
    OUT GT_U32                    hwData[4]
)
{
    DECLARE_FUNC_NAME(snetTwistCreateAuMsg);

    GT_U32 messageType; /* message type send to CPU */
    GT_U32 hwDataTmp[4]; /* 4 word message send to CPU  */

    if (descrPtr->islockPort)
    {
        /* SA message */
        __LOG(("SA message"));
        messageType = 5;
    }
    else
    {
        /* NA message */
        __LOG(("NA message"));
        messageType = 0;
    }

    memset(hwData, 0, 4 * sizeof(GT_U32 ));
    hwData[0] =
       (2 | /* Must be 2 */
        messageType << 4 | /* message type */
        SGT_LIB_SWAP16_MAC(twistMacEntryPtr->macAddr.w.hword2) << 16);

    hwData[1] =
        SGT_LIB_SWAP_BYTES_AND_WORDS_MAC(twistMacEntryPtr->macAddr.w.word1);

    hwData[2] =
       (twistMacEntryPtr->vid |
        twistMacEntryPtr->skipEntry << 12 |
        twistMacEntryPtr->aging << 13 |
        twistMacEntryPtr->trunk << 14 |
        twistMacEntryPtr->dev << 16 |
        twistMacEntryPtr->port << 24 |
        (twistMacEntryPtr->vidx & 0x03) << 30);

    hwData[3] =
       ((twistMacEntryPtr->vidx & 0x3FFC) |
        twistMacEntryPtr->srcTc << 12 |
        twistMacEntryPtr->dstTc << 15 |
        twistMacEntryPtr->daCmd << 22 |
        twistMacEntryPtr->staticEntry << 18 |
        twistMacEntryPtr->multiple << 19 |
        twistMacEntryPtr->daCmd << 22 |
        twistMacEntryPtr->saCmd << 24 |
        twistMacEntryPtr->saClass << 26 |
        twistMacEntryPtr->saCib << 27 |
        twistMacEntryPtr->daClass << 28 |
        twistMacEntryPtr->daCib << 29 |
        twistMacEntryPtr->daRout << 30);

    /* For twist 2 work around */
    if (devObjPtr->deviceType == SKERNEL_98EX100D ||
        devObjPtr->deviceType == SKERNEL_98EX110D ||
        devObjPtr->deviceType == SKERNEL_98EX115D ||
        devObjPtr->deviceType == SKERNEL_98EX120D ||
        devObjPtr->deviceType == SKERNEL_98EX125D ||
        SKERNEL_DEVICE_FAMILY_TIGER(devObjPtr->deviceType))
    {
        /* Swap  MAC address data  */
        hwDataTmp[0] = hwData[2];
        hwDataTmp[1] = hwData[3];
        hwDataTmp[2] = hwData[0];
        hwDataTmp[3] = hwData[1];

        memcpy(hwData, hwDataTmp, 4 * sizeof(GT_U32));
    }
}

/*******************************************************************************
*   snetTwistCreateAAMsg
*
* DESCRIPTION:
*        Set MAC Update Message by free FIFO entry pointer
* INPUTS:
*        devObjPtr   -  pointer to device object.
*        twistMacEntryPtr - MAC entry pointer
*        hwData[4] - New address message data
*
*******************************************************************************/
GT_VOID snetTwistCreateAAMsg
(
    IN  SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN  SNET_TWIST_MAC_TBL_STC * twistMacEntryPtr,
    OUT GT_U32                   hwData[4]
)
{
    GT_U32 messageType; /* message type send to CPU */
    GT_U32 hwDataTmp[4]; /* 4 word message send to CPU  */

    /* AA message */
    messageType = 3;

    memset(hwData, 0, 4 * sizeof(GT_U32 ));
    hwData[0] =
       (2 | /* Must be 2 */
        messageType << 4 | /* message type */
        SGT_LIB_SWAP16_MAC(twistMacEntryPtr->macAddr.w.hword2) << 16);

    hwData[1] =
        SGT_LIB_SWAP_BYTES_AND_WORDS_MAC(twistMacEntryPtr->macAddr.w.word1);

    hwData[2] =
       (twistMacEntryPtr->vid |
        twistMacEntryPtr->skipEntry << 12 |
        twistMacEntryPtr->aging << 13 |
        twistMacEntryPtr->trunk << 14 |
        twistMacEntryPtr->dev << 16 |
        twistMacEntryPtr->port << 24 |
        (twistMacEntryPtr->vidx & 0x03) << 30);

    hwData[3] =
       ((twistMacEntryPtr->vidx & 0x3FFC) |
        twistMacEntryPtr->srcTc << 12 |
        twistMacEntryPtr->dstTc << 15 |
        twistMacEntryPtr->daCmd << 22 |
        twistMacEntryPtr->staticEntry << 18 |
        twistMacEntryPtr->multiple << 19 |
        twistMacEntryPtr->daCmd << 22 |
        twistMacEntryPtr->saCmd << 24 |
        twistMacEntryPtr->saClass << 26 |
        twistMacEntryPtr->saCib << 27 |
        twistMacEntryPtr->daClass << 28 |
        twistMacEntryPtr->daCib << 29 |
        twistMacEntryPtr->daRout << 30);

    /* For twist 2 work around */
    if (devObjPtr->deviceType == SKERNEL_98EX100D ||
        devObjPtr->deviceType == SKERNEL_98EX110D ||
        devObjPtr->deviceType == SKERNEL_98EX115D ||
        devObjPtr->deviceType == SKERNEL_98EX120D ||
        devObjPtr->deviceType == SKERNEL_98EX125D ||
        SKERNEL_DEVICE_FAMILY_TIGER(devObjPtr->deviceType))
    {
        /* Swap  MAC address data  */
        hwDataTmp[0] = hwData[2];
        hwDataTmp[1] = hwData[3];
        hwDataTmp[2] = hwData[0];
        hwDataTmp[3] = hwData[1];

        memcpy(hwData, hwDataTmp, 4 * sizeof(GT_U32));
    }
}


/*******************************************************************************
*   snetTwistLearnSa
*
* DESCRIPTION:
*        Learn new MAC address
* INPUTS:
*        devObjPtr   -  pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
* OUTPUTS:
*
*
*******************************************************************************/
static GT_VOID snetTwistLearnSa
(
    IN SKERNEL_DEVICE_OBJECT   * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetTwistLearnSa);

    GT_U32                          fldValue;
    GT_U8                        *  srcMacPtr;
    SNET_TWIST_MAC_TBL_STC          newMacEntry;

    if ( (descrPtr->macSaLookupResult == 1) && (descrPtr->l2Move==0) )
        return;

    smemRegFldGet(devObjPtr, L2_INGRESS_CTRL_REG, 3, 1, &fldValue);

    /* Create new MAC entry */
    __LOG(("Create new MAC entry"));
    memset(&newMacEntry, 0, sizeof(SNET_TWIST_MAC_TBL_STC));
    srcMacPtr = SRC_MAC_FROM_DSCR(descrPtr);
    memcpy(newMacEntry.macAddr.bytes, srcMacPtr, SGT_MAC_ADDR_BYTES);
    newMacEntry.vid = descrPtr->vid;
    if (descrPtr->srcTrunkId) {
        newMacEntry.trunk = 1;
        newMacEntry.port = descrPtr->srcTrunkId;
    }
    else
        newMacEntry.port = (GT_U16)descrPtr->srcPort;

    newMacEntry.dev = descrPtr->srcDevice;
    newMacEntry.aging = 1;
    newMacEntry.validEntry = 1;
    newMacEntry.vidx    = descrPtr->vid;  /* needs to be equal , thus the msg
                                             would not be corrupted          */
    snetTwistCreateMessage(devObjPtr,descrPtr,NULL,&newMacEntry);
}

/*******************************************************************************
*   snetTwistCreateMessage
*
* DESCRIPTION:
*        Create new MAC address message
* INPUTS:
*        devObjPtr      - pointer to device object.
*        descrPtr       - pointer to the frame's descriptor.
*        sourceMacPtr   - pointer to the source MAC address.
*        newMacEntryPtr - pointer to the MAC entry
*
* OUTPUTS:
*
*******************************************************************************/
GT_VOID snetTwistCreateMessage(
    IN      SKERNEL_DEVICE_OBJECT   * devObjPtr,
    IN      SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN      GT_U8                   * SourceMacPtr,
    INOUT   SNET_TWIST_MAC_TBL_STC  * newMacEntryPtr
)
{
    DECLARE_FUNC_NAME(snetTwistCreateMessage);

    GT_U32            hwData[4];
    GT_BOOL           setMiscIntr = GT_FALSE;
    GT_BOOL           setPciIntr  = GT_FALSE;
    GT_BOOL           auqResult = GT_FALSE;
    GT_U8            *srcMacPtr;
    GT_U32            fldValue;
    GT_U32            freeMacEntryAddr = NOT_VALID_ADDR;
    GT_BOOL           setIntr          = GT_FALSE;

    if (SourceMacPtr==NULL)
    {
        srcMacPtr = SRC_MAC_FROM_DSCR(descrPtr);
    }
    else
    {
        srcMacPtr = (GT_U8*)SourceMacPtr;
    }
    /* Create new address message */
    __LOG(("Create new address message"));
    snetTwistCreateAuMsg(devObjPtr, descrPtr, newMacEntryPtr, hwData);
    /* Send AA message to CPU */
    __LOG(("Send AA message to CPU"));
    while(auqResult == GT_FALSE)
    {
        auqResult = snetTwistFdbAuSend(devObjPtr, hwData, &setMiscIntr);
        /* wait for SW to free buffers */
        SIM_OS_MAC(simOsSleep)(50);
    }

    if (descrPtr->learnEn == 1) {
        freeMacEntryAddr =
        snetTwistGetFreeMacEntryAddr(devObjPtr, srcMacPtr, descrPtr->vid);
        if (freeMacEntryAddr == NOT_VALID_ADDR || auqResult == GT_FALSE) {
            smemRegFldGet(devObjPtr, ETHER_BRDG_INTR_MASK_REG, 5, 1, &fldValue);
            /* NaNotLearned Mask */
            if (fldValue) {
                smemRegFldSet(devObjPtr, ETHER_BRDG_INTR_REG, 5, 1, 1);
                smemRegFldSet(devObjPtr, ETHER_BRDG_INTR_REG, 0, 1, 1);
                setIntr = GT_TRUE;
            }
        }
        else
        if (freeMacEntryAddr != NOT_VALID_ADDR && auqResult != GT_FALSE) {
            /* FILL *freeMacEntryAddr */
            snetTwistSetMacEntry(devObjPtr, freeMacEntryAddr, newMacEntryPtr);

            smemRegFldGet(devObjPtr, ETHER_BRDG_INTR_MASK_REG, 4, 1, &fldValue);
            /* NaLearned Mask*/
            if (fldValue) {
                smemRegFldSet(devObjPtr, ETHER_BRDG_INTR_REG, 4, 1, 1);
                smemRegFldSet(devObjPtr, ETHER_BRDG_INTR_REG, 0, 1, 1);
                setIntr = GT_TRUE;
            }
        }
    }

    /* Call interrupt */
    if (setIntr == GT_TRUE)
    {
        /* PCI Interrupt Summary Mask */
        smemPciRegFldGet(devObjPtr, PCI_INT_MASK_REG, 11, 1, &fldValue);
        if (fldValue) {
            /* Ethernet BridgeSumInt */
            smemPciRegFldSet(devObjPtr, PCI_INT_CAUSE_REG, 11, 1, 1);
            /* IntSum */
            smemPciRegFldSet(devObjPtr, PCI_INT_CAUSE_REG, 0, 1, 1);
            setPciIntr = GT_TRUE;
        }
    }

    /* Call interrupt */
    if (setMiscIntr == GT_TRUE)
    {
        /* PCI Interrupt Summary Mask */
        smemPciRegFldGet(devObjPtr, PCI_INT_MASK_REG, 16, 1, &fldValue);
        if (fldValue) {
            /* Ethernet BridgeSumInt */
            smemPciRegFldSet(devObjPtr, PCI_INT_CAUSE_REG, 16, 1, 1);
            /* IntSum */
            smemPciRegFldSet(devObjPtr, PCI_INT_CAUSE_REG, 0, 1, 1);
            setPciIntr = GT_TRUE;
        }
    }

    if (setPciIntr == GT_TRUE)
    {
        scibSetInterrupt(devObjPtr->deviceId);
    }
}

/*******************************************************************************
*   snetTwistVidxPortsGet
*
* DESCRIPTION:
*        Get VIDX ports information
* INPUTS:
*       devObjPtr     - pointer to device object.
*       vidx          - VLAN id
*
* OUTPUTS:
*       vidx_ports    - array of the vidx
*
*******************************************************************************/
GT_BOOL snetTwistVidxPortsGet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 vidx,
    OUT GT_U8 vidx_ports[]
)
{
    GT_U32 regAddress;
    GT_U32 fldFirstBit;
    GT_U32 fldValue;
    GT_U8 i;

    if (vidx < 4096) {
        for (i = 0; i < 52; i++) {
            /* Internal VLAN Table */
            PORTS_VLAN_TABLE_REG(vidx, i, &regAddress);
            fldFirstBit = INTERNAL_VTABLE_PORT_OFFSET(i);
            smemRegFldGet(devObjPtr, regAddress, fldFirstBit, 1, &fldValue);
            /* port<i>_member */
            vidx_ports[i] = (GT_U8)fldValue;
        }
        /* Get CPU port  */
        PORTS_VLAN_TABLE_REG(vidx, 0, &regAddress);
        smemRegFldGet(devObjPtr, regAddress, 4, 1, &fldValue);
        /* cpu_member */
        vidx_ports[63] = (GT_U8)fldValue;
    }
    else {
        for (i = 0; i < 52; i++) {
            MCAST_GROUPS_TABLE_REG(vidx, i, &regAddress);
            fldFirstBit = INTERNAL_MCAST_PORT_OFFSET(i);
            smemRegFldGet(devObjPtr, regAddress, fldFirstBit, 1, &fldValue);
            /* port<i>_member */
            vidx_ports[i] = (GT_U8)fldValue;
        }
        /* Get CPU port  */
        MCAST_GROUPS_TABLE_REG(vidx, 0, &regAddress);
        smemRegFldGet(devObjPtr, regAddress, 0, 1, &fldValue);
        /* cpu_member */
        vidx_ports[63] = (GT_U8)fldValue;
    }

    return GT_TRUE;
}
/*******************************************************************************
*   snetTwistVltTables
*
* DESCRIPTION:
*        Get VLAN information (vlan ports, tagged ports, STP state)
* INPUTS:
*       devObjPtr          - pointer to device object.
*       descPtr            - frame descriptor
*
* OUTPUTS:
*       vlan_ports         - array to the VLAN port members
*       vlan_tagged_ports  - array to the VLAN tagged ports
*       stp_for_ports      - array to the STP ports.
*
*******************************************************************************/
GT_BOOL  snetTwistVltTables
(
    IN SKERNEL_DEVICE_OBJECT   * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descPtr,
    OUT GT_U8 vlan_member_ports[],
    OUT GT_U8 vlan_tagged_ports[],
    OUT GT_U8 stp_for_ports[]
)
{
    DECLARE_FUNC_NAME(snetTwistVltTables);

    GT_U32 regAddress;
    GT_U32 fldFirstBit;
    GT_U32 fldValue , fldVal;
    GT_U32 vlanValid;
    GT_U32 stgId;
    GT_U8 i;

    ASSERT_PTR(devObjPtr);

    /* Ports Vlan table */
    PORTS_VLAN_TABLE_REG(descPtr->vid, 0, &regAddress);
    /* VLAN table valid bit */
    __LOG(("VLAN table valid bit"));
    smemRegFldGet(devObjPtr, regAddress, 0, 1, &fldValue);
    vlanValid = fldValue;
    if (vlanValid == 0)
        return GT_FALSE;

    smemRegFldGet(devObjPtr, regAddress, 12, 8, &fldValue);
    /* Pointer to the Span State table */
    stgId = fldValue;

    for (i = 0; i < 52; i++)
    {
        PORTS_VLAN_TABLE_REG(descPtr->vid, i, &regAddress);
        fldFirstBit = INTERNAL_VTABLE_PORT_OFFSET(i);
        smemRegFldGet(devObjPtr, regAddress, fldFirstBit, 1, &fldValue);
        /* port<i>_member */
        vlan_member_ports[i] = (GT_U8)fldValue;
        /* port<i>_tagged */
        smemRegFldGet(devObjPtr, regAddress, fldFirstBit + 1, 1, &fldValue);
        vlan_tagged_ports[i] = (GT_U8)fldValue;
        /* port<i>_span_state */
        SPAN_STATE_TABLE_REG(stgId, i, &regAddress);
        fldFirstBit = SPAN_STATE_TABLE_PORT(i);
        smemRegFldGet(devObjPtr, regAddress, fldFirstBit, 2, &fldValue);
        stp_for_ports[i] = (GT_U8)fldValue;
    }
    /* Get CPU port  */
    PORTS_VLAN_TABLE_REG(descPtr->vid, 0, &regAddress);
    smemRegFldGet(devObjPtr, regAddress, 4, 1, &fldValue);
    /* cpu_member */
    vlan_member_ports[63] = (GT_U8)fldValue;

    /* indication if vlan has ports members on uplink */
    smemRegFldGet(devObjPtr, regAddress , 3,1, &fldVal);
    descPtr->vidHasNoUplinkMember = fldVal ;

    return GT_TRUE;
}

/*******************************************************************************
*   intrCauseLinkStatusRegInfoGet
*
* DESCRIPTION:
*        Get port's link status (Ports Interrupt Cause Register)
* INPUTS:
*       devType          -  device type.
*       port             -  cause interrupt port
* OUTPUTS:
*       regAddress       -  register address
*       fldOffset        -  port's link status field
*
*******************************************************************************/
GT_U16 intrCauseLinkStatusRegInfoGet
(

    IN  GT_U32 devType,
    IN  GT_U32 port,
    IN  GT_BOOL isMask,
    OUT GT_U32 * regAddress,
    OUT GT_U32 * fldOffset
)
{
        GT_U16 regOffset;
        GT_U16 macSumBit = 0;

        regOffset = (isMask) ? 0x04 : 0x00;

        switch (devType)
        {
            case SKERNEL_98EX100D:
            case SKERNEL_98EX110D:
            case SKERNEL_98EX115D:
            case SKERNEL_98EX110B:
            case SKERNEL_98EX111B:
            case SKERNEL_98EX112B:
            case SKERNEL_98MX615D:
                if (port <  12)
                {
                        *fldOffset = port +  8;
                        *regAddress = 0x03800098 + regOffset;
                }
                else
                if ((port <  24))
                {
                        *fldOffset = port - 11;
                        *regAddress = 0x038000A0 + regOffset;
                }
                else
                if (port <  36)
                {
                        *fldOffset = port - 23;
                        *regAddress = 0x038000A8 + regOffset;
                }
                else
                if (port <  48)
                {
                        *fldOffset = port - 35;
                        *regAddress = 0x038000B0 + regOffset;
                }
                else
                if (IS_GIGA(port))
                {
                        *fldOffset = (port == 48 || port == 49) ? port - 41 :
                                     (port == 50) ? 10 : 21;
                        *regAddress = 0x03800090 + regOffset;
                }
                macSumBit = 19;
                break;
            case SKERNEL_98EX120D:
            case SKERNEL_98EX125D:
            case SKERNEL_98EX120B:
            case SKERNEL_98EX121B:
            case SKERNEL_98EX122B:
            case SKERNEL_98EX128B:
            case SKERNEL_98EX129B:
            case SKERNEL_98MX625D:
                *fldOffset = (port <  10)  ?  port + 1 :
                             (port == 10)  ? 21 : 22;
                *regAddress = 0x03800090 + regOffset;
                macSumBit = 14;
                break;
            case SKERNEL_98EX130D:
            case SKERNEL_98EX135D:
            case SKERNEL_98MX635D:
            case SKERNEL_98EX136:
                /* 10GBe MAC status register */
                if (isMask == GT_TRUE)
                {
                    *regAddress = 0x03800030;
                }
                else
                {
                    *regAddress = 0x0380002c;
                }

                *fldOffset = 27;
                 macSumBit = 20;
                break;
        }
        return macSumBit;
}
/*******************************************************************************
*   snetTwistSflow
*
* DESCRIPTION:
*         SFlow processing
* INPUTS:
*        devObjPtr   -  pointer to device object.
*        descrPtr  -  pointer to the frame's descriptor.
*        direction  - packet flow direction RX/TX
* OUTPUTS:
*
* RETURN:
*        TRUE - frame processed by sflow engine, FALSE - not processed
*
*
*******************************************************************************/
GT_BOOL snetTwistSflow
(
    IN SKERNEL_DEVICE_OBJECT   * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN SNET_SFLOW_DIRECTION_ENT direction
)
{
    DECLARE_FUNC_NAME(snetTwistSflow);

    GT_U32 fldValue;
    GT_U32 sflowCounter;

    if (direction == SNET_SFLOW_RX)
    {
        smemRegFldGet(devObjPtr, SFLOW_CTRL_REG, 1, 1, &fldValue);
        /* EnIngressSam */
        if (fldValue == 0)
        {
            return GT_FALSE;
        }
    }
    else
    {
        smemRegFldGet(devObjPtr, SFLOW_CTRL_REG, 2, 1, &fldValue);
        /* EnEgressSam */
        if (fldValue == 0)
        {
            return GT_FALSE;
        }
    }
    /* Get sFlow counter status register */
    smemRegGet(devObjPtr, SFLOW_COUNT_STATUS_REG, &sflowCounter);
    if (sflowCounter == 0)
    {
        return GT_TRUE;
    }

    sflowCounter--;

    /* Update sflow down counter */
    __LOG(("Update sflow down counter"));
    smemRegSet(devObjPtr, SFLOW_COUNT_STATUS_REG, sflowCounter);

    if (sflowCounter == 0)
    {
        snetTwistSflowCountInit(devObjPtr, descrPtr);
    }
    else
    if (sflowCounter == 1)
    {
        /* Trap the packet to CPU */
/*      descrPtr->pktCmd = SKERNEL_PKT_TRAP_CPU_E;
        descrPtr->bits15_2.cmd_trap2cpu.cpuCode = TWIST_CPU_CODE_RX_SNIFF;
*/
        snetTwistTx2Cpu(devObjPtr, descrPtr, TWIST_CPU_CODE_RX_SNIFF);

        /* get sflowSamplecounter field 0-15,SampledPackets */
        smemRegFldGet(devObjPtr, SFLOW_STATUS_REG, 0, 16, &fldValue);

                /* increment counter */
        fldValue ++;

        /* sflowSamplecounter update */
        smemRegFldSet(devObjPtr, SFLOW_STATUS_REG, 0, 16, fldValue);
    }

    return GT_TRUE;
}
/*******************************************************************************
*   snetTwistSflowCountInit
*
* DESCRIPTION:
*         Reload Sflow Counter
* INPUTS:
*        devObjPtr   -  pointer to device object.
*        descrPtr  -  pointer to the frame's descriptor.
* OUTPUTS:
*
* RETURN:
*
*
*******************************************************************************/
static GT_VOID snetTwistSflowCountInit
(
    IN SKERNEL_DEVICE_OBJECT   * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetTwistSflowCountInit);

    GT_U32 fldValue;
    GT_U32 sflowCounter, fifoStatus, fifoStatusLimit;
    GT_U32 * sflowFifoPtr;
    TIGER_DEV_MEM_INFO * devMemTigerPtr;    /* tiger memory pointer */
    TWIST_DEV_MEM_INFO * devMemTwistrPtr;   /* twist memory pointer */

    smemRegFldGet(devObjPtr, SFLOW_STATUS_REG, 20, 5, &fldValue);
    /* FifoStatus */
    __LOG(("FifoStatus"));
    fifoStatus = fldValue;

    smemRegFldGet(devObjPtr, SFLOW_CTRL_REG, 3, 5, &fldValue);
    /* FifoFullLevel */
    fifoStatusLimit = fldValue;

    if (fifoStatus == 0)
    {
        smemRegFldGet(devObjPtr, SFLOW_CTRL_REG, 0, 1, &fldValue);
        /* CounterLoadMode */
        __LOG(("CounterLoadMode"));
        if (fldValue == 1)
        {
            return;
        }
        /* Get sFlow value register last value */
        smemRegGet(devObjPtr, SFLOW_VALUE_REG, &sflowCounter);
    }
    else
    {
        if (fifoStatus == fifoStatusLimit)
        {
            smemRegFldGet(devObjPtr, GB_10_PORT_INT_MASK_REG, 21, 1, &fldValue);
            if (fldValue)
            {
                /* sFlow */
                __LOG(("sFlow"));
                smemRegFldSet(devObjPtr, GB_10_PORT_INT_CAUSE_REG, 21, 1, 1);
                smemRegFldSet(devObjPtr, GB_10_PORT_INT_CAUSE_REG, 0, 1, 1);
            }
            /* PCI Interrupt Summary Mask */
            smemPciRegFldGet(devObjPtr, PCI_INT_MASK_REG, 14, 1, &fldValue);
            if (fldValue) {
                /* MACSumInt2 */
                smemPciRegFldSet(devObjPtr, PCI_INT_CAUSE_REG, 14, 1, 1);
                /* IntSum */
                smemPciRegFldSet(devObjPtr, PCI_INT_CAUSE_REG, 0, 1, 1);
                scibSetInterrupt(devObjPtr->deviceId);
            }
        }


        if(devObjPtr->deviceFamily == SKERNEL_TIGER_FAMILY)
        {
            devMemTigerPtr = (TIGER_DEV_MEM_INFO *)devObjPtr->deviceMemory;
            sflowFifoPtr = devMemTigerPtr->macMem.sflowFifo;
        }
        else
        {
            devMemTwistrPtr = (TWIST_DEV_MEM_INFO *)devObjPtr->deviceMemory;
            sflowFifoPtr = devMemTwistrPtr->macMem.sflowFifo;
        }

        sflowCounter = sflowFifoPtr[fifoStatus - 1];
        fifoStatus = fifoStatus - 1;
        fldValue = fifoStatus;
        /* FifoStatatus */
        smemRegFldSet(devObjPtr, SFLOW_STATUS_REG, 20, 5, fldValue);
    }

    /* Update sflow down counter */
    smemRegSet(devObjPtr, SFLOW_COUNT_STATUS_REG, sflowCounter);
}

/*******************************************************************************
*   snetTwistMacHashInit
*
* DESCRIPTION:
*         Fill SGT_MAC_HASH pointer with registers value
* INPUTS:
*        devObjPtr   -  pointer to device object.
*        macHashPtr  -  pointer to mac hash structure
* OUTPUTS:
*        macHashPtr  -  mac hash structure filled by registers values
*
* RETURN:
*
*
*******************************************************************************/
GT_VOID snetTwistMacHashInit
(
    IN SKERNEL_DEVICE_OBJECT   * devObjPtr,
    INOUT SGT_MAC_HASH * macHashPtr
)
{
    GT_U32 fldValue;
    SGT_MAC_ADDR_UNT macAddr;

    smemRegFldGet(devObjPtr, MAC_LOOKUP0_REG, 0, 32, &fldValue);
    /* MACLookup Mask[31:0] */
    macAddr.w.word1 = fldValue;
    smemRegFldGet(devObjPtr, MAC_LOOKUP1_REG, 0, 16, &fldValue);
    /* MACLookup Mask[15:0] */
    macAddr.w.hword2 = (GT_U16)fldValue;
    memcpy(macHashPtr->macMask.arEther, macAddr.bytes, 6);

    smemRegFldGet(devObjPtr, MAC_LOOKUP1_REG, 16, 3, &fldValue);
    /* MACLookup CyclicShiftLeft */
    macHashPtr->macShift = (GT_U8)fldValue;

    smemRegFldGet(devObjPtr, VLAN_LOOKUP_REG, 0, 12, &fldValue);
    /* VidLookup Mask */
    macHashPtr->vidMask = (GT_U16)fldValue;

    smemRegFldGet(devObjPtr, VLAN_LOOKUP_REG, 12, 2, &fldValue);
    /* VidLookup CyclicShiftLeft */
    macHashPtr->vidShift = (GT_U8)fldValue;

    smemRegFldGet(devObjPtr, L2_INGRESS_CTRL_REG, 6, 3, &fldValue);
    /* NumOf EntriesIn MACLookup */
    macHashPtr->macChainLen = fldValue;

    smemRegFldGet(devObjPtr, L2_INGRESS_CTRL_REG, 9, 1, &fldValue);
    /* VlanLookup-Mode */
    macHashPtr->vlanMode = (GT_U8)fldValue;

    smemRegFldGet(devObjPtr, L2_INGRESS_CTRL_REG, 10, 1, &fldValue);
    /* Address Lookup Mode */
    macHashPtr->addressMode = (fldValue == 1)
        ? GT_MAC_SQN_VLAN_SQN : GT_MAC_RND_VLAN_RND;

    /* MACTableSize */
    macHashPtr->size = SGT_MAC_TBL_16K;
    /* improved hash function */
    macHashPtr->macHashKind = GT_NEW_MAC_HASH_FUNCTION;
}


/*******************************************************************************
*   snetTwistL2MplsParse
*
* DESCRIPTION:
*         The mpls parser extracts two shim headers
* INPUTS:
*        devObjPtr   -  pointer to device object.
*        descrPtr  -  pointer to the frame's descriptor.
*        shimOffset - offset of the first label in frame
* OUTPUTS:
*
* RETURN:
*
*
*******************************************************************************/
static GT_VOID snetTwistL2MplsParse
(
    IN SKERNEL_DEVICE_OBJECT   * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U32 shimOffset
)
{
    DECLARE_FUNC_NAME(snetTwistL2MplsParse);

    GT_U8 * dataPtr;

    if (descrPtr->frameType != SKERNEL_UCAST_MPLS_E &&
        descrPtr->frameType != SKERNEL_MCAST_MPLS_E)
    {
        return;
    }

    dataPtr = descrPtr->frameBuf->actualDataPtr + shimOffset;

    descrPtr->mplsTopLablePtr = &dataPtr[0];
    /* Label 1 */
    __LOG(("Label 1"));
    descrPtr->mplsLabels[0].label = dataPtr[0] << 12 | dataPtr[1] << 4 |
                                    dataPtr[2] >> 4;

    descrPtr->mplsLabels[0].exp = dataPtr[2] & 0xE;
    descrPtr->mplsLabels[0].sbit = dataPtr[2] & 0x1;
    descrPtr->mplsLabels[0].ttl = dataPtr[3];

    if (descrPtr->mplsLabels[0].sbit == 0)
    {
        /* Label 2 */
        __LOG(("Label 2"));
        descrPtr->mplsLabels[1].label = dataPtr[4] << 12 | dataPtr[5] << 4 |
                                        dataPtr[6] >> 4;

        descrPtr->mplsLabels[1].exp = dataPtr[6] & 0xE;
        descrPtr->mplsLabels[1].sbit = dataPtr[6] & 0x1;
        descrPtr->mplsLabels[1].ttl = dataPtr[7];
    }
}

