/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetTigerL2.c
*
* DESCRIPTION:
*       This is a external API definition for Snet module of SKernel.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 7 $
*
*******************************************************************************/
#include <asicSimulation/SKernel/suserframes/snetTwistL2.h>
#include <asicSimulation/SKernel/suserframes/snetTigerL2.h>
#include <asicSimulation/SKernel/suserframes/snetTwistEgress.h>
#include <asicSimulation/SKernel/smem/smemTiger.h>
#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SKernel/suserframes/snet.h>
#include <asicSimulation/SKernel/suserframes/snetTigerInLif.h>
#include <asicSimulation/SKernel/suserframes/snetTwistIpV4.h>
#include <common/Utils/FrameInfo/sframeInfoAddr.h>
#include <common/Utils/PresteraHash/smacHash.h>
#include <asicSimulation/SKernel/twistCommon/sregTwist.h>
#include <asicSimulation/SKernel/twistCommon/sregTiger.h>
#include <asicSimulation/SKernel/sfdb/sfdb.h>
#include <asicSimulation/SKernel/sfdb/sfdbTwist.h>
#include <asicSimulation/SKernel/suserframes/snetTigerInLif.h>
#include <asicSimulation/SKernel/suserframes/snetTwistPcl.h>
#include <asicSimulation/SKernel/suserframes/snetSambaPolicy.h>
#include <asicSimulation/SKernel/suserframes/snetTwistTrafficCond.h>
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


static GT_BOOL snetTigerRxMacProcess(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
);

static GT_BOOL snetTigerL2Process(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
);

static GT_VOID snetTigerL2RxMirror(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
);

static GT_VOID snetTigerL2DsaRxMirror(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
);

static GT_VOID snetTigerL2VlanTcAssign(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
);

extern GT_VOID snetTwistL2ControlFrame(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
);

extern GT_VOID snetTwistL2MacRange(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
);

extern GT_VOID snetTwistL2FdbLookUp(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
);

extern GT_BOOL snetTigerIPv4Process(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr
);

extern GT_VOID snetTwistRxMacCountUpdate(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U32 port
);

extern GT_U32 snetTwistMacHashCalc(
    IN GT_U8 * macAddrPtr,
    IN GT_U16 vid,
    IN SGT_MAC_HASH  * macHashStructPtr
);

extern GT_VOID snetTwistL2LockPort(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
);

extern GT_VOID snetTwistL2IngressFilter(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
);

extern GT_VOID snetTwistL2Decision(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
);

extern GT_VOID  snetTwistRegularDropPacket(
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U32 dropCode
);

extern GT_BOOL snetTwistProtVidGet(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U32 encapsul,
    IN GT_U32 etherType
);

extern GT_BOOL snetTwistL2IPsubnetVlan(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 etherType,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
);

extern GT_VOID snetTwistCreateAuMsg(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN SNET_TWIST_MAC_TBL_STC * tigerMacEntryPtr,
    OUT GT_U32 hwData[4]
);

extern GT_U16 intrCauseLinkStatusRegInfoGet(
    IN  GT_U32 devType,
    IN  GT_U32 port,
    IN  GT_BOOL isMask,
    OUT GT_U32 * regAddress,
    OUT GT_U32 * fldOffset
);

extern GT_BOOL snetTwistSflow(
    IN SKERNEL_DEVICE_OBJECT   * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN SNET_SFLOW_DIRECTION_ENT direction
);

extern GT_VOID snetTwistMacHashInit
(
    IN SKERNEL_DEVICE_OBJECT   * devObjPtr,
    INOUT SGT_MAC_HASH * macHashPtr
);

static GT_VOID snetTigerFrameProcess
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN SBUF_BUF_ID bufferId,
    IN GT_U32 srcPort
);

extern GT_VOID snetTwistLinkStateNotify
(
    IN SKERNEL_DEVICE_OBJECT   * deviceObjPtr,
    IN GT_U32 port,
    IN GT_U32 linkState
);

extern GT_U32 snetTigerDsaEgressBuffer
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U32  tagged,
    IN GT_U32 mrvlTag
);

extern GT_U32 snetTwistFillEgressBuffer
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U32 tagged
);

/* 98EX126/98EX136 Configuration and Operation in a Cascaded System */

static GT_VOID snetTigerCscdPort2Group
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    OUT GT_U32 * cscdGroupPtr
);

static GT_VOID snetTigerDxPortAndCscdGrp2VmPort
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    INOUT GT_U32 * portPtr,
    IN GT_U32 cscdGroup
);

extern GT_VOID snetTigerEgressBuildDsaTag
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN  TIGER_VM_PORT_EGRESS_STC * egrConfigPtr,
    IN SKERNEL_MTAG_CMD_ENT mrvlTagCmd,
    OUT GT_U32 *mrvlTag
);

extern GT_VOID snetTigerVmPortEgressConfig
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN GT_U32 port,
    IN GT_U32  tagged,
    OUT TIGER_VM_PORT_EGRESS_STC * egrConfigPtr
);

/*******************************************************************************
*   snetTigerProcessInit
*
* DESCRIPTION:
*       Init module.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*
*******************************************************************************/
GT_VOID snetTigerProcessInit
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    devObjPtr->descriptorPtr =
        (void *)calloc(1, sizeof(SKERNEL_FRAME_DESCR_STC));

    if (devObjPtr->descriptorPtr == 0)
    {
        skernelFatalError("smemTigerInit: allocation error\n");
    }

    /* initiation of internal tiger functions */
    devObjPtr->devFrameProcFuncPtr = snetTigerFrameProcess;
    devObjPtr->devPortLinkUpdateFuncPtr = snetTwistLinkStateNotify;
    devObjPtr->devFdbMsgProcFuncPtr = sfdbTwistMsgProcess;
    devObjPtr->devMacTblTrigActFuncPtr = sfdbTwistMacTableTriggerAction;
    devObjPtr->devMacTblAgingProcFuncPtr = sfdbTwistMacTableAging;
}

/*******************************************************************************
*   snetTigerFrameProcess
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
static GT_VOID snetTigerFrameProcess
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
    /* Rx MAC layer processing of the Tiger  */
    retVal = snetTigerRxMacProcess(devObjPtr, descrPtr);
    if (retVal == GT_FALSE)
        return;

    /* Layer2 frame process */
    retVal = snetTigerL2Process(devObjPtr, descrPtr);
    if (retVal == GT_FALSE)
        return;

    /* InLif processing */
    retVal = snetTigerInLifProcess(devObjPtr, descrPtr);
    if (retVal == GT_FALSE)
        return;

    /* make classifier/pcl/policy processing */
    /* Due to bug in snetTigerInLifProcess PCL was never processed */
    /* the TwistD's engine is different from the needed for Tiger  */
    /* The call commented out                                      */
    /* The needed function not developed yet                       */
    /*                                                             */
    /*retVal = snetTwistPolicyProcess(devObjPtr, descrPtr);        */
    /*if (retVal == GT_FALSE)                                      */
    /*    return;                                                  */

    /* make Traffic Condition processing */
    retVal = snetTwistTrafficConditionProcess(devObjPtr, descrPtr);
    if (retVal == GT_FALSE)
        return;

    /* make IPv4 processing */
    retVal = snetTigerIPv4Process(devObjPtr, descrPtr);
    if (retVal == GT_FALSE)
        return;

    /* Form descriptor according to previous processing */
    snetTwistLxOutBlock(devObjPtr, descrPtr);

    /* Send descriptor to different egress  units */
    snetTwistEqBlock(devObjPtr, descrPtr);
}

/*******************************************************************************
*   snetTigerRxMacProcess
*
* DESCRIPTION:
*       Rx MAC layer processing of the Tiger
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
static GT_BOOL snetTigerRxMacProcess
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetTigerRxMacProcess);

    GT_U32 regAddress = 0;
    GT_U32 fldValue;
    GT_U32 cscdGroup;           /* Cascade mode: 0 - up to 4 cascade groups
                                                 1 - up to 8 cascade groups  */
    GT_U32 mrvlTag;             /* Marvell tag */


    smemRegFldGet(devObjPtr, GLOBAL_CONTROL_REG, 0, 1, &fldValue);
    /* Device Enable */
    if (fldValue != 1)
        return GT_FALSE;

    MAC_CONTROL_REG(devObjPtr->portsNumber , descrPtr->srcPort, &regAddress);
    smemRegFldGet(devObjPtr, regAddress, 0, 1, &fldValue);
    /* This bit is set to enable the port */
    if (fldValue != 1)
        return GT_FALSE;

    smemRegFldGet(devObjPtr, GLOBAL_CONTROL_REG, 8, 7, &fldValue);
    /* DeviceID[14:8] */
    descrPtr->srcDevice = (GT_U8)fldValue;

    snetTwistRxMacCountUpdate(devObjPtr, descrPtr, descrPtr->srcPort);

    smemRegFldGet(devObjPtr, CASCADE_GRP_1_REG, 24, 1, &fldValue);
    /* DSA Cascade disable */
    if (fldValue == 0)
    {
        __LOG(("DSA Cascade disable"));
        return GT_TRUE;
    }

    /* Convert cascade port to correspondent port's group */
    __LOG(("Convert cascade port to correspondent port's group"));
    snetTigerCscdPort2Group(devObjPtr, descrPtr, &cscdGroup);
    /* No DSA traffic over this port */
    if (cscdGroup == 0)
    {
        return GT_TRUE;
    }

    /* Frame came with DSA tag */
    __LOG(("Frame came with DSA tag"));
    descrPtr->dsaTagged = 1;

    /* Obtain MARVELL tag */
    __LOG(("Obtain MARVELL tag"));
    mrvlTag = DSA_TAG(descrPtr->frameBuf->actualDataPtr);

    /* Source port from which the packet was received */
    __LOG(("Source port from which the packet was received"));
    descrPtr->srcPort = SMEM_U32_GET_FIELD(mrvlTag, 19, 5);


    /* Convert DX Device Source Port to Virtual Mapped Port */
    __LOG(("Convert DX Device Source Port to Virtual Mapped Port"));
    snetTigerDxPortAndCscdGrp2VmPort(devObjPtr, &descrPtr->srcPort, cscdGroup);

    return GT_TRUE;
}

/*******************************************************************************
*   snetTigerL2Process
*
* DESCRIPTION:
*       Layer2 processing of the Tiger
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - pointer to the frame's descriptor.
*
* RETURNS:
*
*******************************************************************************/
static GT_BOOL snetTigerL2Process
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    snetTigerL2RxMirror(devObjPtr, descrPtr);
    if (descrPtr->pktCmd == SKERNEL_PKT_DROP_E)
    {
        return GT_FALSE;
    }
    snetTigerL2VlanTcAssign(devObjPtr, descrPtr);
    snetTwistL2ControlFrame(devObjPtr, descrPtr);
    snetTwistL2MacRange(devObjPtr, descrPtr);
    snetTwistL2FdbLookUp(devObjPtr, descrPtr);
    snetTwistL2LockPort(devObjPtr, descrPtr);
    snetTwistL2IngressFilter(devObjPtr, descrPtr);
    snetTwistL2Decision(devObjPtr, descrPtr);

    return GT_TRUE;
}

/*******************************************************************************
*   snetTigerL2RxMirror
*
* DESCRIPTION:
*       Make RX mirroring
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr        - pointer to the frame's descriptor.
*
*******************************************************************************/
static GT_VOID snetTigerL2RxMirror
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetTigerL2RxMirror);

    GT_U32 regAddress, fldValue;
    GT_U32 targetDevice, targetPort;
    GT_BOOL sflowStatus;

    if (devObjPtr->deviceType == SKERNEL_98EX136)
    {
        sflowStatus = snetTwistSflow(devObjPtr, descrPtr, SNET_SFLOW_RX);
        if (sflowStatus == GT_TRUE)
        {
            descrPtr->rxSniffed = 0;
            return;
        }
    }

    descrPtr->rxSniffed = 1;

    /* Frame came with DSA tag */
    __LOG(("Frame came with DSA tag"));
    if (descrPtr->dsaTagged)
    {
        /* Transmit to RX analyzer port */
        snetTigerL2DsaRxMirror(devObjPtr, descrPtr);
        if (descrPtr->pktCmd == SKERNEL_PKT_DROP_E)
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
*   snetTigerL2DsaRxMirror
*
* DESCRIPTION:
*       Make RX mirroring for packet arrived to cascade port with
*       TO_TARGET_SNIFFER DSA command
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr        - pointer to the frame's descriptor.
*
*******************************************************************************/
static GT_VOID snetTigerL2DsaRxMirror
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetTigerL2DsaRxMirror);

    GT_U32 fldValue;
    GT_U32 targetDevice, targetPort;                /* RX target device and port */
    TIGER_VM_PORT_EGRESS_STC egrConfig;             /* Egress Virtual Port Configuration */
    GT_U32 mrvlTag;                                 /* Marvell tag */
    GT_U32 egressBufLen;                            /* Egress buffer length */
    GT_U32 tagged=0;                                /* SRC port tagged */
    GT_U32 dsaTagEnable;                            /* Tiger cascading support */

    /* Obtain MARVELL tag */
    mrvlTag = DSA_TAG(descrPtr->frameBuf->actualDataPtr);

    /* DSA tag command */
    __LOG(("DSA tag command"));
    if (SKERNEL_MTAG_CMD_TO_TRG_SNIFFER_E !=
        SMEM_U32_GET_FIELD(mrvlTag, 30, 2))
    {
        return;
    }

    smemRegFldGet(devObjPtr, INGR_MIRR_ANALYZER_REG, 4, 7, &fldValue);
    /* Indicates the target device number for Rx sniffed packets. */
    targetDevice = fldValue;

    smemRegFldGet(devObjPtr, INGR_MIRR_ANALYZER_REG, 11, 6, &fldValue);
    /* Indicates the target port number for Rx sniffed packets. */
    targetPort = fldValue;

    smemRegFldGet(devObjPtr, INGR_MIRR_ANALYZER_REG, 1, 3, &fldValue);
    /* Indicates the Traffic Class of mirrored packets when they are sent
    to the CPU. */
    descrPtr->trafficClass = (GT_U8)fldValue;

    /* Source DX port tagged/untagged */
    tagged = SMEM_U32_GET_FIELD(mrvlTag, 29, 1);

    /* 802.1 User Priority field assigned to the packet */
    descrPtr->userPriorityTag =
            (GT_U8)SMEM_U32_GET_FIELD(mrvlTag, 13, 3);

    /* The VID assigned to the packet */
    descrPtr->vid =
            (GT_U16)SMEM_U32_GET_FIELD(mrvlTag, 0, 12);

    smemRegFldGet(devObjPtr, CASCADE_GRP_1_REG, 24, 1, &dsaTagEnable);
    /* DSA cascade enable value should be 1 (0 - in func. spec is typo) */
    if (dsaTagEnable)
    {
        /* Get VM port egress configuration */
        snetTigerVmPortEgressConfig(devObjPtr, targetPort, tagged,
                                    &egrConfig);

        /*  Build DSA tag */
        snetTigerEgressBuildDsaTag(devObjPtr, descrPtr, &egrConfig,
                                   SKERNEL_MTAG_CMD_TO_TRG_SNIFFER_E,
                                   &mrvlTag);

        /* Override VM target port by the actual transmit port */
        targetPort = egrConfig.actualPort;
    }

    if (dsaTagEnable)
    {
        egressBufLen = snetTigerDsaEgressBuffer(devObjPtr, descrPtr, tagged, mrvlTag);
    }
    else
    {
        egressBufLen = snetTwistFillEgressBuffer(devObjPtr, descrPtr, tagged);
    }

    /* Transmit packet to device */
    /* CPU code should be specifically defined to TX */
    snetTwistTx2Device(devObjPtr, descrPtr, devObjPtr->egressBuffer,
                       egressBufLen, targetPort, targetDevice,
                       TWIST_CPU_CODE_RX_SNIFF);

    /* The original frame that was already transmitted by the DX device is discarded */
    snetTwistRegularDropPacket(descrPtr, 0);
}

/*******************************************************************************
*   snetTigerL2VlanTcAssign
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
static GT_VOID snetTigerL2VlanTcAssign
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetTigerL2VlanTcAssign);

    GT_U32 regAddress;
    GT_U32 fldValue;
    GT_U32 fldFirstBit;
    GT_U8 * dataPtr;
    GT_U16 etherType = 0, realEtherType;
    GT_U32 encapsul;
    GT_U32 dsapSsap;
    GT_BOOL result;
    GT_U32  ipOffsetFromL2=12;
    GT_U32 headerSize=0,nonIpDscp=0;
    GT_U32 mrvlTag;

    dataPtr = descrPtr->frameBuf->actualDataPtr;

    /* Frame came with DSA tag */
    __LOG(("Frame came with DSA tag"));
    if (descrPtr->dsaTagged)
    {
        mrvlTag = DSA_TAG(dataPtr);

        /* DX received packet tagged/untagged */
        descrPtr->srcVlanTagged =
            (GT_U8)SMEM_U32_GET_FIELD(mrvlTag, 29, 1);
        /* Frame was received tagged or priority-tagged by the DX device */
        if (descrPtr->srcVlanTagged)
        {
            descrPtr->vid =
                (GT_U16)SMEM_U32_GET_FIELD(mrvlTag, 0, 12);

            /* Tagged (VID==0) */
            if (descrPtr->vid == 0)
            {
                /* Bridge Port<n> Control Register */
                BRDG_PORT0_CTRL_REG(descrPtr->srcPort, &regAddress);
                smemRegFldGet(devObjPtr, regAddress, 0, 12, &fldValue);
                /* Reassign VID */
                descrPtr->vid = (GT_U16)fldValue;
            }
        }
        else
        {
            /* Bridge Port<n> Control Register */
            BRDG_PORT0_CTRL_REG(descrPtr->srcPort, &regAddress);
            smemRegFldGet(devObjPtr, regAddress, 0, 12, &fldValue);
            descrPtr->vid = (GT_U16)fldValue;
        }
        /* User priority tag */
        descrPtr->userPriorityTag =
            (GT_U8)SMEM_U32_GET_FIELD(mrvlTag, 13, 3);

        realEtherType = (descrPtr->dstMacPtr[16] << 8) |
                         descrPtr->dstMacPtr[17];

        dsapSsap = (descrPtr->dstMacPtr[18] << 8) | descrPtr->dstMacPtr[19];

        ipOffsetFromL2 += 4;

    }
    else
    {
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
        __LOG(("get dscp"));
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
*   snetTigerCscdPort2Group
*
* DESCRIPTION:
*        Converts cascade port of Tiger to cascading group
* INPUTS:
*        devObjPtr  - pointer to device object.
*        descrPtr   - pointer to frame descriptor
* OUTPUTS:
*        cscdGroupPtr  - cascade group
*
* RETURN:
*
*
*******************************************************************************/
static GT_VOID snetTigerCscdPort2Group
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    OUT GT_U32 * cscdGroupPtr
)
{
    DECLARE_FUNC_NAME(snetTigerCscdPort2Group);

    GT_U32 regAddress;              /* Register's address */
    GT_U32 fldOffset;               /* Register field's offset */

    regAddress = (descrPtr->srcPort < 6) ?
            CASCADE_GRP_1_REG : CASCADE_GRP_2_REG;

    fldOffset = (descrPtr->srcPort % 6) * 4;
    /* DSA Cascade group ID assigned to port */
    __LOG(("DSA Cascade group ID assigned to port"));
    smemRegFldGet(devObjPtr, regAddress, fldOffset, 4, cscdGroupPtr);
}

/*******************************************************************************
*   snetTigerDxPortAndCscdGrp2VmPort
*
* DESCRIPTION:
*        Convert DX port and cascade group to virtual port
* INPUTS:
*        devObjPtr  - pointer to device object.
*        portPtr    - pointer to cascade port to convert
*        cscdGroup  - cascade group
* OUTPUTS:
*        portPtr    - pointer to converted VM port
*
* RETURN:
*
*
*******************************************************************************/
static GT_VOID snetTigerDxPortAndCscdGrp2VmPort
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    INOUT GT_U32 * portPtr,
    IN GT_U32 cscdGroup
)
{
    GT_U32 regAddress;              /* Register's address */
    GT_U32 regIdx;                  /* registers index    */
    GT_U32 fldValue;                /* Register field's value */
    GT_U32 fldOffset;               /* Register field's offset */
    GT_U32 numOfRegsInGroup;        /* Number of registers in group */

    /* Cascade Mode */
    smemRegFldGet(devObjPtr, CASCADE_GRP_1_REG, 25, 1, &fldValue);

    numOfRegsInGroup = (fldValue == 0) ? 8 : 4;

    /* Virtual port ingress */
    /* group_id+port_id => virtual port */
    regIdx = (cscdGroup * numOfRegsInGroup)  + (portPtr[0] >> 2);
    regAddress = INGRESS_VM_PORTS_REG + regIdx * 0x4;
    /* Four virtual ports per register */
    fldOffset = (portPtr[0] % 4) * 8;

    /* Virtual Mapped Port */
    smemRegFldGet(devObjPtr, regAddress, fldOffset, 6, portPtr);
}

