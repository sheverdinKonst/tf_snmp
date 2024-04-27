/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetTwistEgress.c
*
* DESCRIPTION:
*      This is a external API definition for egress frame processing.
*
* DEPENDENCIES:
*      None.
*
* FILE REVISION NUMBER:
*      $Revision: 53 $
*
*******************************************************************************/
#include <asicSimulation/SKernel/suserframes/snetTwistEgress.h>
#include <asicSimulation/SKernel/suserframes/snetTwistL2.h>
#include <asicSimulation/SKernel/suserframes/snet.h>
#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SKernel/suserframes/snetTwistIpV4.h>
#include <asicSimulation/SKernel/twistCommon/sregTwist.h>
#include <asicSimulation/SKernel/twistCommon/sregTiger.h>
#include <asicSimulation/SKernel/smem/smemTwist.h>
#include <asicSimulation/SLog/simLog.h>

#define STWIST_WRAM_ADDR_ALLIGN_CNS  4
#define STWIST_MAX_DEVICE_PORTS     52

static GT_BOOL snetTwistRouterEgress
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr
);

static void snetTwistIpmEgress
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr
);

static void snetTwistOutlif2Descriptor
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN IPV4_OUTLIF_TYPE_ENT outLifType,
    IN GT_U32 outLif
);

static void snetTwistFrwrdMcast
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr
);

static GT_BOOL snetTwistTx2Port
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U32 trgPort,
    IN GT_U32 tagged
);

static GT_U32 snetTwistHa
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U32 trgPort,
    IN GT_U32 tagged
);

static void snetTwistReadMllEntry
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 mllAddr,
    OUT SNET_MLL_STC * mllPtr
);

static GT_VOID snetTwistDrop
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr
);

static GT_VOID snetTwistFrwrdUcast
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
);

static GT_BOOL snetRxDmaFillDescriptor
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U32  cpuCode,
    IN  STRUCT_RX_DESC *rxDesc,
    OUT STRUCT_RX_DESC **nextRxDesc
);

GT_U32 snetTwistFillEgressBuffer
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U32 tagged
);

static void sstackTwistTxSend
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U8 * frameDataPtr,
    IN GT_U32 frameDataLength,
    IN GT_U32 targetDevice
);
static void snetTwistStpPortsApply
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U8  stpPorts[],
    INOUT GT_U8  ports[]
);

static void snetTwistOutlifGet
(
    IN GT_U32  outLifMem,
    OUT LLL_OUTLIF_STC * outLifEntryPtr
);

static GT_U32 snetTwistMplsHa
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U8 * egrBufPtr,
    IN GT_U8 * dataPtr
);
static void snetTwistTxMirror
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U32 txPort
);

GT_VOID snetTigerVmPortEgressConfig
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN GT_U32 port,
    IN GT_U32  tagged,
    OUT TIGER_VM_PORT_EGRESS_STC * egrConfigPtr
);

GT_VOID snetTigerEgressBuildDsaTag
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN  TIGER_VM_PORT_EGRESS_STC * egrConfigPtr,
    IN SKERNEL_MTAG_CMD_ENT mrvlTagCmd,
    OUT GT_U32 *mrvlTag
);

GT_U32 snetTigerDsaEgressBuffer
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U32  tagged,
    IN GT_U32 mrvlTag
);

static GT_U32 snetTigerDsaHa
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U32 trgPort,
    IN GT_U32 mrvlTag
);

/* The actual transmit port of 98EX126 mapped for VM_Port <n> */
#define TIGER_EGR_VM_TO_ACTUAL_PORT(dev_obj_ptr, port, ret_val)                 \
{                                                                               \
    GT_U32 regAddress, fldOffset;                                               \
    EGRESS_VM_PORTS_REG_OFFSET(port, regAddress, fldOffset);                    \
                                                                                \
    smemRegFldGet((dev_obj_ptr), regAddress, fldOffset, 4, &(ret_val));         \
}

/* The DX device transmit port mapped for VM port*/
#define TIGER_EGR_VM_DX_TRANS_PORT(dev_obj_ptr, port, ret_val)                  \
{                                                                               \
    GT_U32 regAddress, fldOffset;                                               \
    EGRESS_VM_PORTS_REG_OFFSET(port, regAddress, fldOffset);                    \
                                                                                \
    smemRegFldGet((dev_obj_ptr), regAddress, fldOffset + 8, 5, &(ret_val));     \
}

/* The target VM port is mapped to cascade or non-cascade port */
#define TIGER_EGR_VM_CSCD_MAP_PORT(dev_obj_ptr, port, ret_val)                  \
{                                                                               \
    GT_U32 regAddress, fldOffset;                                               \
    EGRESS_VM_PORTS_REG_OFFSET(port, regAddress, fldOffset);                    \
                                                                                \
    smemRegFldGet((dev_obj_ptr), regAddress, fldOffset + 15, 1, &(ret_val));    \
}

/* Calculate extended buffer word for specific bit */
#define SNET_EXT_BUF_WORD(bit)                                                       \
    ((((SKERNEL_EXT_DATA_SIZE_CNS  - 1) * 32) - (bit)) / 32)

/* Calculate extended bit offset */
#define SNET_EXT_BUF_OFFSET(bit)                                                     \
    ((bit) % 32)

/* Get extended buffer field */
#define SNET_EXT_BUF_FIELD(ext_data_ptr, bit, len)                              \
    SMEM_U32_GET_FIELD((ext_data_ptr)[SNET_EXT_BUF_WORD((bit))],                \
                                      SNET_EXT_BUF_OFFSET(bit), len)

/*******************************************************************************
*   snetTwistLxOutBlock
*
* DESCRIPTION:
*        Form descriptor according to previous processing
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
* OUTPUTS:
*
*
*******************************************************************************/
GT_VOID snetTwistLxOutBlock
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    if (descrPtr->pktCmd == SKERNEL_PKT_TRAP_CPU_E)
    {
        descrPtr->vlanCmd = SKERNEL_NOT_CHANGE_E;
        descrPtr->doHa = 0;
        descrPtr->useVidx = 0;
        return;
    }
    if (descrPtr->ipv4Done == 1)
    {
        descrPtr->llt = IPV4_ROUT_IPV4_E;
        descrPtr->doHa = 1;
    }
    else
    if (descrPtr->mplsDone == 1)
    {
        descrPtr->llt = IPV4_MPLS_E;
        descrPtr->doHa = 1;
    }
    if ((descrPtr->ipv4Done || descrPtr->mplsDone)
        && !descrPtr->doIpmRout)
    {
        snetTwistOutlif2Descriptor(devObjPtr, descrPtr, descrPtr->outLifType,
                                   descrPtr->LLL_outLif);
    }
}

/*******************************************************************************
*   snetTwistOutlif2Descriptor
*
* DESCRIPTION:
*        Form descriptor according to outLif
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
*        outLifType - output lif type
*        outLif - output logical interface entry
* OUTPUTS:
*
*
*******************************************************************************/
static void snetTwistOutlif2Descriptor
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN IPV4_OUTLIF_TYPE_ENT outLifType,
    IN GT_U32 outLif
)
{
    DECLARE_FUNC_NAME(snetTwistOutlif2Descriptor);

    LLL_OUTLIF_STC outLifEnrty;
    GT_U32 trunkId, trunkHush;
    GT_U32 regAddress;
    GT_U32 fldValue;
    GT_U32 fldFirstBit;
    GT_U16 portsInTrunk;
    GT_U16 origVid = descrPtr->vid;

    snetTwistOutlifGet(outLif, &outLifEnrty);

    descrPtr->vid = (GT_U16)outLifEnrty.vlanId;

    if (outLifEnrty.daTag == 1)
    {
        descrPtr->vlanCmd = SKERNEL_ADD_TAG_E;
    }
    else
    {
        descrPtr->vlanCmd = SKERNEL_NOT_CHANGE_E;
    }

    descrPtr->useVidx = (GT_U8)outLifEnrty.useVidx;

    if (descrPtr->useVidx == 1)
    {
        descrPtr->bits15_2.useVidx_1.vidx = outLifEnrty.target.vidx;
        return;
    }

    /* if we are here we have useVidx = 0 */
    __LOG(("if we are here we have useVidx = 0"));

    descrPtr->bits15_2.useVidx_0.targetIsTrunk =
            (GT_U8)outLifEnrty.target.port.trgIsTrunk;

    if (descrPtr->bits15_2.useVidx_0.targetIsTrunk == 1)
    {
        GT_U32 bit;
        trunkId = outLifEnrty.target.trunk.trgTrunkId;
        trunkHush = outLifEnrty.target.trunk.trgTrunkHush;
        TRUNK_TABLE_REG(devObjPtr->deviceFamily, trunkId, &regAddress);
        fldFirstBit = TRUNK_TABLE_MEMBER_NUM(devObjPtr->deviceFamily, trunkId);
        smemRegFldGet(devObjPtr, regAddress, fldFirstBit, 4, &fldValue);
        /* Trunk <trunkId> MembersNum */
        portsInTrunk = (GT_U16)fldValue;
        descrPtr->bits15_2.useVidx_0.trunkHash = 0;
        for (bit = 0; bit < portsInTrunk; bit++)
        {
            descrPtr->bits15_2.useVidx_0.trunkHash +=
                            (GT_U8)(trunkHush >> bit & 0x01);
        }

        descrPtr->bits15_2.useVidx_0.targetIsTrunk = 0;
        trunkId = descrPtr->bits15_2.useVidx_0.targedPort;
        trunkHush = descrPtr->bits15_2.useVidx_0.trunkHash;

        TRUNK_MEMBER_TABLE_REG(devObjPtr->deviceFamily,
                                            trunkId, trunkHush, &regAddress);
        fldFirstBit = (trunkHush % 2) ? 13 : 0;
        smemRegFldGet(devObjPtr, regAddress, fldFirstBit, 6, &fldValue);
        /* Trunk<trunkId>Member<trunkHush>Port */
        descrPtr->bits15_2.useVidx_0.targedPort = (GT_U8)fldValue;
        fldFirstBit = (trunkHush % 2) ? 19 : 6;
        smemRegFldGet(devObjPtr, regAddress, fldFirstBit, 7, &fldValue);
        /* Trunk<trunkId>Member<trunkHush>Device */
        descrPtr->bits15_2.useVidx_0.targedDevice = (GT_U8)fldValue;
    }
    else
    {
        /* get egress device and port from outlif */
        __LOG(("get egress device and port from outlif"));
        descrPtr->bits15_2.useVidx_0.targedPort =
                (GT_U8)outLifEnrty.target.port.trgPort;
        descrPtr->bits15_2.useVidx_0.targedDevice =
                (GT_U8)outLifEnrty.target.port.trgDev;
    }

    if (descrPtr->bits15_2.useVidx_0.targedPort == TWIST_CPU_PORT_CNS)
    {
        descrPtr->vid = origVid;
    }
}

/*******************************************************************************
*   snetTwistProcessFrameFromUpLink
*
* DESCRIPTION:
*        Process frame from uplink
* INPUTS:
*        devObjPtr - pointer to device object.
*        uplinkDesc  - pointer to the frame's descriptor.
* OUTPUTS:
*
*
*******************************************************************************/
GT_VOID snetTwistProcessFrameFromUpLink
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_UPLINK_DESC_STC    * uplinkDesc
)
{
    SKERNEL_FRAME_DESCR_STC        frameDescriptor;
    GT_U32                         uplinkDescSize = sizeof(SKERNEL_UPLINK_DESC_STC);

    memset(&frameDescriptor,0,sizeof(SKERNEL_FRAME_DESCR_STC));
    frameDescriptor.byteCount   = uplinkDesc->data.PpPacket.source.byteCount;
    frameDescriptor.frameType   = uplinkDesc->data.PpPacket.source.frameType ;
    frameDescriptor.srcTrunkId  = uplinkDesc->data.PpPacket.source.srcTrunkId ;
    frameDescriptor.srcPort     = uplinkDesc->data.PpPacket.source.srcPort  ;
    frameDescriptor.uplink      = (GT_U8)uplinkDesc->data.PpPacket.source.uplink ;
    frameDescriptor.vid         = uplinkDesc->data.PpPacket.source.vid   ;
    frameDescriptor.trafficClass= uplinkDesc->data.PpPacket.source.trafficClass;
    frameDescriptor.pktCmd      = uplinkDesc->data.PpPacket.source.pktCmd ;
    frameDescriptor.srcDevice   = uplinkDesc->data.PpPacket.source.srcDevice   ;
    frameDescriptor.useVidx     = uplinkDesc->data.PpPacket.useVidx ;
    frameDescriptor.macDaType   = uplinkDesc->data.PpPacket.macDaType        ;
    frameDescriptor.llt         = uplinkDesc->data.PpPacket.llt;
    frameDescriptor.srcVlanTagged = (GT_U8)uplinkDesc->data.PpPacket.source.srcVlanTagged ;
    if (uplinkDesc->data.PpPacket.useVidx)
    {
        frameDescriptor.bits15_2.useVidx_1.vidx =
                (GT_U16)uplinkDesc->data.PpPacket.dest.vidx ;
    }
    else
    {
        frameDescriptor.bits15_2.useVidx_0.targedPort =
            (GT_U8)uplinkDesc->data.PpPacket.dest.targetInfo.targetPort ;
        frameDescriptor.bits15_2.useVidx_0.targedDevice =
            (GT_U8)uplinkDesc->data.PpPacket.dest.targetInfo.targetDevice ;
    }

    uplinkDesc->data.PpPacket.source.frameBuf->actualDataPtr += uplinkDescSize;
    uplinkDesc->data.PpPacket.source.frameBuf->actualDataSize -= uplinkDescSize;

    frameDescriptor.frameBuf      = uplinkDesc->data.PpPacket.source.frameBuf;

    snetTwistEqBlock(devObjPtr,&frameDescriptor);
}

/*******************************************************************************
*   snetTwistEqBlock
*
* DESCRIPTION:
*        Send descriptor to different egress  units
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
* OUTPUTS:
*
*
*******************************************************************************/
GT_VOID snetTwistEqBlock
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetTwistEqBlock);

    SKERNEL_FRAME_DESCR_STC srcDescr;
    GT_U32 mirrCpuEn;
    GT_U32 fldValue;
    GT_BOOL doRouteEgress = GT_FALSE;
    GT_BOOL doUplinkRoute = GT_FALSE;

    srcDescr = *descrPtr;

    if (descrPtr->pktCmd == SKERNEL_PKT_DROP_E)
    {
        snetTwistDrop(devObjPtr, descrPtr);
        return;
    }

    if (descrPtr->pktCmd == SKERNEL_PKT_FORWARD_E &&
        descrPtr->useVidx == 0 &&
        descrPtr->bits15_2.useVidx_0.targedPort == 0x61)
    {
        snetTwistDrop(devObjPtr, descrPtr);
        return;
    }

    if (descrPtr->pktCmd == SKERNEL_PKT_TRAP_CPU_E)
    {
        doRouteEgress = GT_TRUE;
    }
    else if (descrPtr->pktCmd == SKERNEL_PKT_FORWARD_E &&
        descrPtr->useVidx == 0 &&
        descrPtr->bits15_2.useVidx_0.targedDevice == descrPtr->srcDevice)
    {
        doRouteEgress = GT_TRUE;
    }
    else if (descrPtr->pktCmd == SKERNEL_PKT_FORWARD_E &&
        descrPtr->useVidx == 0  &&
        descrPtr->bits15_2.useVidx_0.targedDevice != descrPtr->srcDevice &&
        descrPtr->uplink)
    {
        doRouteEgress = GT_TRUE;
    }
    else if (descrPtr->pktCmd == SKERNEL_PKT_FORWARD_E &&
        descrPtr->useVidx == 1 &&
        descrPtr->vidHasNoLocalMember==0)
    {
        doRouteEgress = GT_TRUE;
    }
    else if (descrPtr->pktCmd == SKERNEL_PKT_FORWARD_E &&
        descrPtr->ipm == 1)
    {
        doRouteEgress = GT_TRUE;
    }

    if(doRouteEgress==GT_TRUE)
    {
        snetTwistRouterEgress(devObjPtr, descrPtr);
        *descrPtr = srcDescr;
    }

    smemRegFldGet(devObjPtr, INGR_MIRR_ANALYZER_REG, 0, 1, &fldValue);
    /* Mirror ToCPUEn */
    __LOG(("Mirror ToCPUEn"));
    mirrCpuEn = fldValue;
    if (mirrCpuEn && descrPtr->pktCmd == SKERNEL_PKT_MIRROR_CPU_E)
    {
        if(doRouteEgress==GT_FALSE)
        {
            descrPtr->pktCmd = SKERNEL_PKT_FORWARD_E;
            snetTwistRouterEgress(devObjPtr, descrPtr);
            *descrPtr = srcDescr;
        }

        descrPtr->pktCmd = SKERNEL_PKT_TRAP_CPU_E;
        snetTwistRouterEgress(devObjPtr, descrPtr);
        *descrPtr = srcDescr;
    }

    /*flood the packet in the VLAN*/
    if (descrPtr->pktCmd == SKERNEL_PKT_FORWARD_E &&
        descrPtr->useVidx == 1 &&
        descrPtr->vidHasNoUplinkMember == 0 &&
        descrPtr->vidHasNoLocalMember==0 &&
        descrPtr->uplink==0)
    {
        doUplinkRoute = GT_TRUE;
    }
    else if (descrPtr->pktCmd == SKERNEL_PKT_FORWARD_E &&
        descrPtr->ipm == 1 &&
        descrPtr->uplink==0)
    {
        doUplinkRoute = GT_TRUE;
    }
    else if (descrPtr->pktCmd == SKERNEL_PKT_FORWARD_E &&
        descrPtr->useVidx == 0  &&
        descrPtr->bits15_2.useVidx_0.targedDevice != descrPtr->srcDevice &&
        descrPtr->uplink == 0)
    {
        doUplinkRoute = GT_TRUE;
    }

    if (doUplinkRoute == GT_TRUE)
    {
        sstackTwistToUplink(devObjPtr, descrPtr);
        *descrPtr = srcDescr;
    }
}

/*******************************************************************************
*   snetTwistRouterEgress
*
* DESCRIPTION:
*        Make router egress processing
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
* OUTPUTS:
*
*
*******************************************************************************/
static GT_BOOL snetTwistRouterEgress
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    if (descrPtr->ipm == 1)
    {
        if (descrPtr->mll > 0)
        {
            snetTwistIpmEgress(devObjPtr, descrPtr);
            return GT_TRUE;
        }
    }
    snetTwistEgress(devObjPtr, descrPtr);

    return GT_TRUE;
}

/*******************************************************************************
*   snetTwistEgress
*
* DESCRIPTION:
*        Make egress processing
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
* OUTPUTS:
*
*
*******************************************************************************/
void snetTwistEgress
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    if (descrPtr->pktCmd == SKERNEL_PKT_FORWARD_E)
    {
        if (descrPtr->useVidx == 0)
        {
            snetTwistFrwrdUcast(devObjPtr, descrPtr);
        }
        else
        {
            snetTwistFrwrdMcast(devObjPtr, descrPtr);
        }
    }
    else
    {
        snetTwistTx2Cpu(devObjPtr, descrPtr,
                        descrPtr->bits15_2.cmd_trap2cpu.cpuCode);
    }
}

/*******************************************************************************
*   snetTwistIpmEgress
*
* DESCRIPTION:
*         Make IPM multiplication of frames
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
* OUTPUTS:
*        descrPtr -  pointer to the frame's descriptor.
*
*******************************************************************************/
static void snetTwistIpmEgress
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetTwistIpmEgress);

    SKERNEL_FRAME_DESCR_STC srcDescr;
    GT_U32 mllAddr;
    GT_U32 mllBaseAddr;
    SNET_MLL_STC mll;
    GT_U32 fldValue;
    srcDescr = *descrPtr;

    smemRegFldGet(devObjPtr, IPV4_IPMC_LINK_LIST_BASE_REG, 0, 22, &fldValue);
    /* MLL base address in memory */
    __LOG(("MLL base address in memory"));
    mllBaseAddr = fldValue;

    mllAddr = ((mllBaseAddr + srcDescr.mll)
                << STWIST_WRAM_ADDR_ALLIGN_CNS) + 0x28000000;

    while(1)
    {
        snetTwistReadMllEntry(devObjPtr, mllAddr, &mll);
        *descrPtr = srcDescr;
        descrPtr->mll = 1;
        if (descrPtr->ttl < mll.first_mll.ttlThres)
        {
            descrPtr->pktCmd = SKERNEL_PKT_DROP_E;
            snetTwistEgress(devObjPtr, descrPtr);
        }
        else
        if (mll.first_mll.excludeSrcVlan != 1 ||
            descrPtr->vid != mll.first_mll.out_lif.lll.vlanId)
        {
            snetTwistOutlif2Descriptor(devObjPtr, descrPtr,
                                       mll.first_mll.outLifType,
                                       mll.first_mll.outLif[0]);
            descrPtr->doHa = 1;
            snetTwistEgress(devObjPtr, descrPtr);
        }
        if (mll.first_mll.last == 1)
        {
            return;
        }
        else
        {
            *descrPtr = srcDescr;
        }

        if (descrPtr->ttl < mll.second_mll.ttlThres)
        {
            descrPtr->pktCmd = SKERNEL_PKT_DROP_E;
            snetTwistEgress(devObjPtr, descrPtr);
        }
        else
        if (mll.second_mll.excludeSrcVlan != 1 ||
            descrPtr->vid != mll.second_mll.out_lif.lll.vlanId)
        {
            snetTwistOutlif2Descriptor(devObjPtr, descrPtr,
                                       mll.second_mll.outLifType,
                                       mll.second_mll.outLif[0]);
            descrPtr->doHa = 1;
            snetTwistEgress(devObjPtr, descrPtr);
        }
        if (mll.second_mll.last == 1)
        {
            return;
        }
        else
        {
            mllAddr = ((mllBaseAddr + mll.second_mll.nextPtr) <<
                STWIST_WRAM_ADDR_ALLIGN_CNS) + 0x28000000;
        }
    }
}
/*******************************************************************************
*   snetTwistFrwrdUcast
*
* DESCRIPTION:
*         Make IPM multiplication of frames
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
* OUTPUTS:
*
*
*******************************************************************************/
static GT_VOID snetTwistFrwrdUcast
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetTwistFrwrdUcast);

    GT_U8 vlan_ports[64], vlan_tagged_ports[64], stp_for_ports[64];
    GT_U32 trunkId, trunkHush, targetPort;
    GT_U32 regAddress, fldFirstBit, fldValue;
    GT_U32 tagged;

    if (descrPtr->bits15_2.useVidx_0.targetIsTrunk == 1)
    {
        trunkId = descrPtr->bits15_2.useVidx_0.targedPort;
        trunkHush = descrPtr->bits15_2.useVidx_0.trunkHash;

        TRUNK_MEMBER_TABLE_REG(devObjPtr->deviceFamily,
                                            trunkId, trunkHush, &regAddress);
        fldFirstBit = (trunkHush % 2) ? 13 : 0;
        smemRegFldGet(devObjPtr, regAddress, fldFirstBit, 6, &fldValue);
        /* Trunk<trunkId>Member<trunkHush>Port */
        __LOG(("Trunk<trunkId>Member<trunkHush>Port"));
        targetPort = fldValue;
    }
    else
    {
        targetPort = descrPtr->bits15_2.useVidx_0.targedPort;
    }
    memset(vlan_ports, 0, sizeof(vlan_ports));
    memset(vlan_tagged_ports, 0, sizeof(vlan_tagged_ports));
    memset(stp_for_ports, 0, sizeof(vlan_tagged_ports));

    snetTwistVltTables(devObjPtr, descrPtr, vlan_ports, vlan_tagged_ports,
                       stp_for_ports);

    smemRegFldGet(devObjPtr, EGRESS_BRIDGING_REG, 5, 1, &fldValue);
    /* BridgedUC EgressFilter En*/
    if (fldValue == 1) {
        if (vlan_ports[targetPort] == 0) {
            snetTwistPortEgressCount(devObjPtr, descrPtr,
                targetPort,SKERNEL_EGRESS_COUNTER_EGRESS_BRG_FILTER_E);
            snetTwistDrop(devObjPtr, descrPtr);
            return;
        }
    }
    /* Exclude from vlan table port with block/listen or learning stp state */
    snetTwistStpPortsApply(devObjPtr, descrPtr, stp_for_ports, vlan_ports);

    if ( (vlan_ports[targetPort] == 1) || (fldValue == 0) )
    {
        tagged = vlan_tagged_ports[targetPort];
        /* Forward frame to target port */
        snetTwistTx2Port(devObjPtr, descrPtr, targetPort, tagged);
    }
}

/*******************************************************************************
*   snetTwistTx2Cpu
*
* DESCRIPTION:
*         Transfer frame to CPU by DMA
*
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
*        cpuCode   - cpu code
* OUTPUTS:
*
*
*******************************************************************************/
GT_BOOL snetTwistTx2Cpu
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U32 cpuCode
)
{
    DECLARE_FUNC_NAME(snetTwistTx2Cpu);

    STRUCT_RX_DESC * curDescrPtr, * nextRxDesc;
    GT_U32 regValue, regAddr, firstBit, fldValue;
    GT_BOOL retVal;

    snetTwistPortEgressCount(devObjPtr, descrPtr,
        TWIST_CPU_PORT_CNS,SKERNEL_EGRESS_COUNTER_NOT_FILTER_E);

    firstBit = 2 + descrPtr->trafficClass ;

    CUR_RX_DESC_POINTER_REG(descrPtr->trafficClass, &regAddr);
    smemRegGet(devObjPtr, regAddr,  &regValue);
    curDescrPtr = (STRUCT_RX_DESC *)((GT_UINTPTR)regValue);
    if (curDescrPtr == NULL)
    {
        smemRegFldGet(devObjPtr, SDMA_INT_MASK_REG,
                      11 + descrPtr->trafficClass, 1, &fldValue);
        if (fldValue)
        {
            smemRegFldSet(devObjPtr, SDMA_INT_REG,
                         11 + descrPtr->trafficClass , 1, 1);
            smemRegFldSet(devObjPtr, SDMA_INT_REG, 0, 1, 1);/*set '0' bit*/
        }
        /* PCI Interrupt Summary Mask */
        __LOG(("PCI Interrupt Summary Mask"));
        smemPciRegFldGet(devObjPtr, PCI_INT_MASK_REG, 17, 1, &fldValue);
        /* RxSDMASumInt */
        if (fldValue)
        {
            smemPciRegFldSet(devObjPtr, PCI_INT_CAUSE_REG, 17, 1, 1);
            smemPciRegFldSet(devObjPtr, PCI_INT_CAUSE_REG, 0, 1, 1);/*set '0' bit*/

            /* Call interrupt */
            scibSetInterrupt(devObjPtr->deviceId);
        }
        return GT_FALSE;
    }

    CUR_RX_DESC_POINTER_REG(descrPtr->trafficClass, &regAddr);
    smemRegGet(devObjPtr, regAddr,  &regValue);
    curDescrPtr = (STRUCT_RX_DESC *)((GT_UINTPTR)regValue);
    if ((descrPtr->uplink) && (descrPtr->useVidx))
    {
        smemRegFldGet(devObjPtr, EGRESS_BRIDGING_REG, 7, 1, &fldValue);
        if (fldValue==0)
        {   /* drop packet that comes from the uplink to the CPU */
            snetTwistDrop(devObjPtr,descrPtr);
            return GT_TRUE;
        }
    }

    retVal = snetRxDmaFillDescriptor(devObjPtr, descrPtr, cpuCode,
                                     curDescrPtr, &nextRxDesc);

    firstBit = (retVal == GT_TRUE) ? 2 + descrPtr->trafficClass :
                        11  + descrPtr->trafficClass;

    smemRegFldSet(devObjPtr, SDMA_INT_REG, firstBit, 1, 1);
    smemRegFldGet(devObjPtr, SDMA_INT_MASK_REG, firstBit, 1, &fldValue);
    if (fldValue)
    {

        smemRegFldSet(devObjPtr, SDMA_INT_REG, 0, 1, 1);
        /* PCI Interrupt Summary Mask */
        smemPciRegFldGet(devObjPtr, PCI_INT_MASK_REG, 17, 1, &fldValue);
        /* RxSDMASumInt */
        if (fldValue)
        {
            smemPciRegFldSet(devObjPtr, PCI_INT_CAUSE_REG, 17, 1, 1);
            smemPciRegFldSet(devObjPtr, PCI_INT_CAUSE_REG, 0, 1, 1);

            /* Call interrupt */
            scibSetInterrupt(devObjPtr->deviceId);
        }
    }

    return GT_TRUE;
}

/*******************************************************************************
*   snetTwistFrwrdMcast
*
* DESCRIPTION:
*         Make IPM multiplication of frames
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
* OUTPUTS:
*
*
*******************************************************************************/
static void snetTwistFrwrdMcast
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetTwistFrwrdMcast);

    GT_U8 vlan_ports[64], vlan_tagged_ports[64];
    GT_U8 stp_for_ports[64], vidx_ports[64];
    GT_U32 vidx;
    GT_U32 regAddress, firstBit, fldValue;
    GT_U8 unicastCmd;
    GT_U32 memberBits[2];
    GT_U8 trunkGroupId;
    GT_U32 tagged;
    GT_U8 i;
    GT_U8 hashVal;
    GT_U8 cpuBcFilterEn;


    memset(vlan_ports, 0, 64);
    memset(vlan_tagged_ports, 0, 64);
    memset(stp_for_ports, 0, 64);

    /* Get VLAN ports info */
    __LOG(("Get VLAN ports info"));
    snetTwistVltTables(devObjPtr, descrPtr, vlan_ports,
                          vlan_tagged_ports, stp_for_ports );

    unicastCmd = 0;
    /* UCAST */
    if (descrPtr->macDaType == SKERNEL_UNICAST_MAC_E)
    {
        for (i = 0; i < 10; i++) {
            firstBit = PORT_FILTER_UNK(i);
            smemRegFldGet(devObjPtr, EGRESS_BRIDGING_REG, firstBit, 1, &fldValue);
            /* PortNFilterUnk */
            if (fldValue)
            {
                vlan_ports[i] = 0;
                snetTwistPortEgressCount(devObjPtr, descrPtr,
                    i,SKERNEL_EGRESS_COUNTER_EGRESS_BRG_FILTER_E);
            }
        }
        smemRegFldGet(devObjPtr, EGRESS_BRIDGING_REG, 1, 2, &fldValue);
        /* Umknown UnicastCmd */
        unicastCmd = (GT_U8)fldValue;
    }
    else
    if (descrPtr->macDaLookupResult == 0) {
        if (descrPtr->macDaType == SKERNEL_MULTICAST_MAC_E) {
            smemRegFldGet(devObjPtr, EGRESS_BRIDGING_REG, 3, 2, &fldValue);
            /* Unreg UnicastCmd */
            unicastCmd = (GT_U8)fldValue;
        }
        else {
            smemRegFldGet(devObjPtr, EGRESS_BRIDGING_REG, 0, 1, &fldValue);
            /* CPUBC FilterEn */
            cpuBcFilterEn = (GT_U8)fldValue;
            if (cpuBcFilterEn &&
                descrPtr->macDaType == SKERNEL_BROADCAST_ARP_E) {
                vlan_ports[63] = 0;
            }
        }
    }
    else {
        memset(vidx_ports, 0, sizeof(vidx_ports));
        vidx = descrPtr->bits15_2.useVidx_1.vidx;
        snetTwistVidxPortsGet(devObjPtr, vidx, vidx_ports);
        /* Exclude from vidx table ports with blocking/listening or
           learning stp state  */
        snetTwistStpPortsApply(devObjPtr, descrPtr, stp_for_ports, vidx_ports);
        for (i = 0; i < 64; i++)
            vlan_ports[i] &= vidx_ports[i];
    }

    if (unicastCmd == 1) {
        vlan_ports[63] = 1;
    }
    else
    if (unicastCmd == 2) {
        memset(vlan_ports, 0, 64);
        vlan_ports[63] = 1;
    }
    else
    if (unicastCmd == 3) {
        vlan_ports[63] = 0;
    }

    /* Exclude from vlan table ports with blocking/listening or
       learning stp state  */
    snetTwistStpPortsApply(devObjPtr, descrPtr, stp_for_ports, vlan_ports);

    if (descrPtr->srcTrunkId > 0) {
        memberBits[0] = memberBits[1] = 0;
        TRUNK_NON_TRUNK_MEMBER_REG(descrPtr->srcTrunkId, &regAddress);
        smemRegFldGet(devObjPtr, regAddress, 0, 32, &fldValue);
        /* Trunk<n> NonTrunkMem bits 0-31 */
        memberBits[0] = fldValue;
        regAddress += 0x20000;
        smemRegFldGet(devObjPtr, regAddress, 0, 21, &fldValue);
        /* Trunk<n> NonTrunkMem bits 32-52 */
        memberBits[1] = fldValue;
        for (i = 0; i < 32; i++) {
            if ((memberBits[0] & 0x01) == 0)
            {
                vlan_ports[i] = 0;
            }
            memberBits[0] >>= 1;
        }
        for (i = 32; i < 52; i++) {
            if ((memberBits[1] & 0x01) == 0)
            {
                vlan_ports[i] = 0;
            }
            memberBits[1] >>= 1;
        }
    }
    memberBits[0] = memberBits[1] = 0;
    hashVal = descrPtr->bits15_2.useVidx_0.trunkHash;
    TRUNK_DESIGN_PORTS_HASH_REG(hashVal, &regAddress);
    smemRegFldGet(devObjPtr, regAddress, 0, 32, &fldValue);
    /* DesPortTrunk bits 0-31 */
    memberBits[0] = fldValue;
    regAddress += 0x20000;
    smemRegFldGet(devObjPtr, regAddress, 0, 21, &fldValue);
    /* DesPortTrunk bits 32-52 */
    memberBits[1] = fldValue;

    for (i = 0; i < 32; i++) {
        if (vlan_ports[i] == 0)
            continue;

        if ((stp_for_ports[i] == 3) || (stp_for_ports[i] == 0)) {
            /* Bridge Port<n> Control Register */
            BRDG_PORT0_CTRL_REG(i, &regAddress);
            smemRegFldGet(devObjPtr, regAddress, 2, 3, &fldValue);
            /* TrunkGroupId */
            trunkGroupId = (GT_U8)fldValue;
            if (trunkGroupId == 0 && (memberBits[0] & 0x01) == 0) {
                vlan_ports[i] = 0;
            }
        }
        memberBits[0] >>= 1;
    }
    for (i = 32; i < 52; i++) {
        if (vlan_ports[i] == 0)
            continue;

        if ((stp_for_ports[i] == 3) || (stp_for_ports[i] == 0)) {
            /* Bridge Port<n> Control Register */
            BRDG_PORT0_CTRL_REG(i, &regAddress);
            smemRegFldGet(devObjPtr, regAddress, 2, 3, &fldValue);
            /* TrunkGroupId */
            trunkGroupId = (GT_U8)fldValue;
            if (trunkGroupId == 0 && (memberBits[1] & 0x01) == 0) {
                vlan_ports[i] = 0;
            }
        }
        memberBits[1] >>= 1;
    }

    for (i = 0; i < 64; i++) {
        if (vlan_ports[i] == 1) {
            tagged = vlan_tagged_ports[i];
            snetTwistTx2Port(devObjPtr, descrPtr, i, tagged);
        }
    }

}

/*******************************************************************************
*   snetTwistReadMllEntry
*
* DESCRIPTION:
*         Read Multicast Link List entry
* INPUTS:
*        regAddress - MLL memory address.
* OUTPUTS:
*        mllPtr  - pointer to the MLL entry.
*
*
*******************************************************************************/
static void snetTwistReadMllEntry
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 mllAddr,
    OUT SNET_MLL_STC * mllPtr
)
{
    GT_U32 * regValPtr, regVal;
    GT_U32 hWord, lWord;

    /* Get first 32 bits */
    regValPtr = (GT_U32 *)smemMemGet(devObjPtr, mllAddr);
    regVal = *regValPtr;

    mllPtr->first_mll.last = regVal & 0x01;
    mllPtr->first_mll.outLifType = (regVal >> 2) & 0x01;
    mllPtr->first_mll.outLif[0] = (regVal >> 3) & 0x1FFFFFFF;/*29 bits*/

    /* Get next 32 bits */
    regValPtr++;
    regVal = *regValPtr;
    mllPtr->first_mll.outLif[0] |=  regVal << 29;/* 3 bits */
    mllPtr->first_mll.outLif[1] = (regVal >> 3) & 0x7;/* 3 bits */
    mllPtr->first_mll.ttlThres = (regVal >> 7) & 0x1FF;
    mllPtr->first_mll.excludeSrcVlan = (regVal >> 16) & 0x1;

    mllPtr->second_mll.last = (regVal >> 18) & 0x1;
    mllPtr->second_mll.outLifType = (regVal >> 20) & 0x1;
    mllPtr->second_mll.outLif[0] = (regVal >> 21) & 0x7FF;/* 11 bits */

    /* Get next 32 bits */
    regValPtr++;
    regVal = *regValPtr;

    mllPtr->second_mll.outLif[0] |= regVal << 11;/* 21 bits */
    mllPtr->second_mll.outLif[1] = (regVal >> 21) & 0x7  ;/* 3 bits */
    hWord = (regVal >> 25) & 0x7F;

    /* Get next 32 bits */
    regValPtr++;
    regVal = *regValPtr;

    lWord = regVal & 0x3;
    mllPtr->second_mll.ttlThres = hWord | lWord;
    mllPtr->second_mll.excludeSrcVlan = (regVal >> 2) & 0x1;
    mllPtr->second_mll.nextPtr = (regVal >> 4) & 0x7FFF;

    snetTwistOutlifGet(mllPtr->first_mll.outLif[0],
                      &mllPtr->first_mll.out_lif.lll);

    snetTwistOutlifGet(mllPtr->second_mll.outLif[0],
                      &mllPtr->second_mll.out_lif.lll);
}

static GT_VOID snetTwistDrop
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr
)
{

}
/*******************************************************************************
*   snetTwistTx2Port
*
* DESCRIPTION:
*        Forward frame to target port
* INPUTS:
*        devObjPtr   -  pointer to device object.
*        descrPtr  -    pointer to the frame's descriptor.
*        egressPort -   number of egress port.
*        tagged  -      send frame with tag.
*
*
*******************************************************************************/
static GT_BOOL snetTwistTx2Port
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U32 trgPort,
    IN GT_U32 tagged
)
{
    DECLARE_FUNC_NAME(snetTwistTx2Port);

    GT_U32 regAddress = 0, fldValue;
    GT_U32 egressBufLen;
    GT_U32 cpuCode;
    GT_U32 mrvlTag = 0;
    GT_U32 dsaTagEnable = 0;                            /* Tiger cascading support */
    TIGER_VM_PORT_EGRESS_STC egrConfig;             /* Egress Virtual Port Configuration */

    if (trgPort == TWIST_CPU_PORT_CNS)
    {
        switch(descrPtr->llt )
        {
          case 0 :
                cpuCode = ETH_BRIDGED_LLT;
                break;
          case 1 :
                cpuCode = IPV4_ROUTED_LLT;
                break;
          case 2 :
                cpuCode = IPV6_ROUTED_LLT;
                break;
          case 3 :
                cpuCode = UC_MPLS_LLT;
                break;
          case 4 :
                cpuCode = L2CE_LLT;
                break;
            default :
                return GT_FALSE;
        }
        return snetTwistTx2Cpu(devObjPtr, descrPtr, cpuCode);
    }

    if ((descrPtr->doRout == 0) &&
                (trgPort == descrPtr->srcPort) &&
                (descrPtr->srcDevice == devObjPtr->deviceId))
    {
        /* Bridge Port<n> Control Register */
        __LOG(("Bridge Port<n> Control Register"));
        BRDG_PORT0_CTRL_REG(trgPort, &regAddress);
        smemRegFldGet(devObjPtr, regAddress, 21, 1, &fldValue);
        /* The packet is allowed to be L2 switched back to its local ports */
        if (fldValue != 1)
            return GT_FALSE;
    }

    snetTwistTxMirror(devObjPtr, descrPtr, trgPort);

    if (SKERNEL_DEVICE_FAMILY_TIGER(devObjPtr->deviceType))
    {
        smemRegFldGet(devObjPtr, CASCADE_GRP_1_REG, 24, 1, &dsaTagEnable);
        /* DSA cascade enable value should be 1 (0 - in func. spec is typo) */
        if (dsaTagEnable)
        {
            /* Get VM port egress configuration */
            snetTigerVmPortEgressConfig(devObjPtr, trgPort, tagged, &egrConfig);

            /*  Build DSA tag */
            snetTigerEgressBuildDsaTag(devObjPtr, descrPtr, &egrConfig,
                                       SKERNEL_MTAG_CMD_FROM_CPU_E, &mrvlTag);

            /* Override VM target port by the actual transmit port */
            trgPort = egrConfig.actualPort;
        }
    }

    /*
        FROM THIS POINT the trgPort is PHY port !!!
        __LOG(("        FROM THIS POINT the trgPort is PHY port !!!"));

        Before this point it was "Virtual port" !!!
    */

    MAC_CONTROL_REG(devObjPtr->portsNumber, trgPort, &regAddress);
    smemRegFldGet(devObjPtr, regAddress, 0, 1, &fldValue);
    /* This bit is set to enable the port */
    if (fldValue != 1)
        return GT_FALSE;


    if (descrPtr->doHa == 0)
    {
        if (SKERNEL_DEVICE_FAMILY_TIGER(devObjPtr->deviceType)
            && dsaTagEnable)
        {

            egressBufLen = snetTigerDsaEgressBuffer(devObjPtr, descrPtr, tagged, mrvlTag);
        }
        else
        {
            egressBufLen = snetTwistFillEgressBuffer(devObjPtr, descrPtr, tagged);
        }
    }
    else
    {
        if (SKERNEL_DEVICE_FAMILY_TIGER(devObjPtr->deviceType)
            && dsaTagEnable)
        {
            egressBufLen = snetTigerDsaHa(devObjPtr, descrPtr, trgPort, mrvlTag);
        }
        else
        {
            egressBufLen = snetTwistHa(devObjPtr, descrPtr, trgPort, tagged);
        }
    }

    if(egressBufLen > 0)
    {
        snetTwistTxMacCountUpdate(devObjPtr, descrPtr, trgPort);

        smainFrame2PortSend(devObjPtr,
                            trgPort,
                            devObjPtr->egressBuffer,
                            egressBufLen,
                            GT_FALSE);
    }

    return GT_TRUE;

}

/*******************************************************************************
*   snetTwistTxMirror
*
* DESCRIPTION:
*         Tx mirroring processing
* INPUTS:
*        devObjPtr   -  pointer to device object.
*        descrPtr  -    pointer to the frame's descriptor.
*        txPort - TX port
*
*******************************************************************************/
static void snetTwistTxMirror
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U32 txPort
)
{
    DECLARE_FUNC_NAME(snetTwistTxMirror);

    GT_U32 txMirrorEnable;
    GT_U32 targetDevice, targetPort;
    GT_U32 regAddress, fldValue;
    GT_U32 egressBufLen;
    GT_U32 tagged=0,fldFirstBit=0;
    GT_U32 dsaTagEnable = 0;                            /* Tiger cascading support */
    TIGER_VM_PORT_EGRESS_STC egrConfig;             /* Egress Virtual Port Configuration */
    GT_U32 mrvlTag = 0;                                 /* Marvell tag */

    TRANSMIT_CONF_REG(txPort, &regAddress);
    smemRegFldGet(devObjPtr, regAddress, 21, 1, &fldValue);
    /* PortEgress MirrEn */
    __LOG(("PortEgress MirrEn"));
    txMirrorEnable = fldValue;

    if (txMirrorEnable == 1)
    {
        smemRegFldGet(devObjPtr, TRANSMIT_SNIF_REG, 0, 7, &fldValue);
        /* TxSniffDest Dev */
        targetDevice = fldValue;
        smemRegFldGet(devObjPtr, TRANSMIT_SNIF_REG, 7, 6, &fldValue);
        /* TxSniffDest Port */
        targetPort = fldValue;
        PORTS_VLAN_TABLE_REG(descrPtr->vid, targetPort, &regAddress);
        fldFirstBit = INTERNAL_VTABLE_PORT_OFFSET(targetPort);
        /* tagged mode of targetPort in the vlan entry*/
        smemRegFldGet(devObjPtr, regAddress, fldFirstBit + 1, 1, &fldValue);
        tagged = (GT_U8)fldValue;

        if (SKERNEL_DEVICE_FAMILY_TIGER(devObjPtr->deviceType))
        {
            smemRegFldGet(devObjPtr, CASCADE_GRP_1_REG, 24, 1, &fldValue);
            dsaTagEnable = (fldValue) ? 0 : 1;
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
        }

        if (descrPtr->doHa == 0)
        {
            if (SKERNEL_DEVICE_FAMILY_TIGER(devObjPtr->deviceType)
                && dsaTagEnable)
            {

                egressBufLen = snetTigerDsaEgressBuffer(devObjPtr, descrPtr, tagged, mrvlTag);
            }
            else
            {
                egressBufLen = snetTwistFillEgressBuffer(devObjPtr, descrPtr, tagged);
            }
        }
        else
        {
            if (SKERNEL_DEVICE_FAMILY_TIGER(devObjPtr->deviceType)
                && dsaTagEnable)
            {
                egressBufLen = snetTigerDsaHa(devObjPtr, descrPtr, targetPort, mrvlTag);
            }
            else
            {
                egressBufLen = snetTwistHa(devObjPtr, descrPtr, targetPort, tagged);
            }
        }

        if (egressBufLen > 0)
        {
            snetTwistTx2Device(devObjPtr, descrPtr, devObjPtr->egressBuffer,
                                egressBufLen, targetPort, targetDevice,
                                TWIST_CPU_CODE_RX_SNIFF);
        }
    }
}

/*******************************************************************************
*   snetTwistHa
*
* DESCRIPTION:
*        Make header alteration procedure
* INPUTS:
*        devObjPtr   -  pointer to device object.
*        descrPtr  -    pointer to the frame's descriptor.
*        trgPort -      number of egress port.
*        tagged  -      send frame with tag.
*
*
*******************************************************************************/
static GT_U32 snetTwistHa
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U32 trgPort,
    IN GT_U32 tagged
)
{
    DECLARE_FUNC_NAME(snetTwistHa);

    GT_U32 egressBufLen, copySize;
    GT_U32 regAddress = 0, fldValue;
    GT_U32 macEntryIdx;
    GT_U32 ipCheckSum;
    GT_U8 lsbFromVlan;
    SGT_MAC_ADDR_TYP macAddr;
    SNET_TWIST_MAC_TBL_STC twistMacEntryPtr;
    GT_U8 portSaLsb, saLsb, vlanSa;
    GT_U8_PTR ipHeaderPtr, egrBufPtr, dataPtr, dataPtrActual;
    GT_U8 tagData[4];
    /* Egress buffer pointer to copy to */
    egrBufPtr = devObjPtr->egressBuffer;

    if (descrPtr->mll > 0)
    {
        dataPtr = DST_MAC_FROM_DSCR(descrPtr);
    }
    else
    {
        macEntryIdx = descrPtr->arpPointer;
        regAddress = MAC_TAB_ENTRY_WORD0_REG + macEntryIdx * 0x10;
        snetTwistGetMacEntry(devObjPtr, regAddress, &twistMacEntryPtr);
        dataPtr = twistMacEntryPtr.macAddr.bytes;
    }
    dataPtrActual = descrPtr->frameBuf->actualDataPtr;
    /* Copy Destination MAC address */
    __LOG(("Copy Destination MAC address"));
    MEM_APPEND(egrBufPtr, dataPtr, SGT_MAC_ADDR_BYTES);

    smemRegGet(devObjPtr, SRC_ADDR_HIGH_REG, &fldValue);
    /* SAHi[47:16] */
    macAddr.bytes[0] = (GT_U8)(fldValue >> 24);
    macAddr.bytes[1] = (GT_U8)((fldValue >> 16) & 0xFF);
    macAddr.bytes[2] = (GT_U8)((fldValue >>  8) & 0xFF);
    macAddr.bytes[3] = (GT_U8)(fldValue & 0xFF);

    smemRegFldGet(devObjPtr, SRC_ADDR_MID_REG, 0, 8, &fldValue);
    /* SAMid[15:8] */
    macAddr.bytes[4] = (GT_U8)(fldValue & 0xFF);

    smemRegFldGet(devObjPtr, SRC_ADDR_MID_REG, 8, 1, &fldValue);
    /* SAMode */
    lsbFromVlan = (GT_U8)fldValue;

    MAC_CONTROL_REG(devObjPtr->portsNumber, trgPort, &regAddress);
    smemRegFldGet(devObjPtr, regAddress, 9, 8, &fldValue);
    /* SALow[7:0] */
    portSaLsb = (GT_U8)fldValue;

    if (lsbFromVlan == 1)
    {
        /* Ports Vlan table */
        PORTS_VLAN_TABLE_REG(descrPtr->vid, 0, &regAddress);
        smemRegFldGet(devObjPtr, regAddress, 5, 6, &fldValue);
        /* Vlan_Mac_Sa */
        vlanSa = (GT_U8)fldValue;
        saLsb = (portSaLsb & 0xC0) | (vlanSa & 0x3F);
    }
    else
    {
        saLsb = portSaLsb;
    }
    macAddr.bytes[5] = saLsb;
    dataPtr = macAddr.bytes;
    /* Copy Source MAC address */
    MEM_APPEND(egrBufPtr, dataPtr, SGT_MAC_ADDR_BYTES);
    /* Set data pointer to the first byte following Dst and Src MACs */
    dataPtr = dataPtrActual + 2 * SGT_MAC_ADDR_BYTES;
    if (tagged == 1)
    {
/*
  THIS IMPLEMENTATION IS WORKING WRONG BECAUSE IT DOES NOT INSERT NEW VLAN TAG IF NECESSARY:
  packet that has tag X in the ingress and should have tag Y in the egress


        if (descrPtr->srcVlanTagged == 0) {
             Create and insert VLAN tag *
            snetTagDataGet(descrPtr->userPriorityTag ,
                           descrPtr->vid ,
                           GT_FALSE,
                           tagData);
            copySize = sizeof(tagData);
             Copy VLAN tag
            MEM_APPEND(egrBufPtr, tagData, copySize);
            dataPtr += copySize;
        }
*/
        /* Create and insert VLAN tag */
        snetTagDataGet(descrPtr->userPriorityTag ,
                       descrPtr->vid ,
                       GT_FALSE,
                       tagData);
        copySize = sizeof(tagData);
        /* Copy VLAN tag */
        MEM_APPEND(egrBufPtr, tagData, copySize);

        if (descrPtr->srcVlanTagged == 1) {
            dataPtr += copySize;
        }
    }
    else
    {
        if (descrPtr->srcVlanTagged == 1)
        {
            /* Skip VLAN tag */
            dataPtr += 4;
        }
    }
    /* Do MPLS HA */
    __LOG(("Do MPLS HA"));
    if (descrPtr->frameType == SKERNEL_UCAST_MPLS_E ||
        descrPtr->frameType == SKERNEL_MCAST_MPLS_E)
    {
        return snetTwistMplsHa(devObjPtr, descrPtr, egrBufPtr, dataPtr);
    }

    /* Set data pointer to rest of buffer */
    copySize = descrPtr->frameBuf->actualDataSize -
               (dataPtr - dataPtrActual);

    if (descrPtr->ipHeaderPtr) {
        ipHeaderPtr = descrPtr->ipHeaderPtr;
        /* A timer field (TTL) */
        ipHeaderPtr[8] = descrPtr->ttl - descrPtr->decTtl;
        if (descrPtr->modifyDscpOrExp)
        {
            ipHeaderPtr[1] = descrPtr->dscp << 2;
        }

        ipHeaderPtr[10] = 0;
        ipHeaderPtr[11] = 0;
        /* Calculate IP packet checksum */
        ipCheckSum = ipV4CheckSumCalc(ipHeaderPtr,
                                     (GT_U16)
                                     ((ipHeaderPtr[0] & 0xF) * 4));

        /* Checksum of the IP header and IP options */
        ipHeaderPtr[10] = (GT_U8)(ipCheckSum >> 8);
        ipHeaderPtr[11] = (GT_U8)(ipCheckSum & 0xFF);
    }

    /* Copy rest of frame to egress buffer */
    MEM_APPEND(egrBufPtr, dataPtr, copySize);

    egressBufLen = egrBufPtr - devObjPtr->egressBuffer;

    return egressBufLen;
}

/*******************************************************************************
*   snetTwistTx2Device
*
* DESCRIPTION:
*         Tx mirroring processing
* INPUTS:
*        devObjPtr   -  pointer to device object.
*        descrPtr  -    pointer to the frame's descriptor.
*        frameDataPtr - frame data pointer
*        frameDataLength  - frame data length
*        txPort - TX port
*
*******************************************************************************/
GT_VOID snetTwistTx2Device
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U8_PTR frameDataPtr,
    IN GT_U32 frameDataLength,
    IN GT_U32 targetPort,
    IN GT_U32 targetDevice,
    IN GT_U32 cpuCode
)
{
    if (targetDevice == descrPtr->srcDevice)
    {
        if (targetPort == TWIST_CPU_PORT_CNS)
        {
            snetTwistTx2Cpu(devObjPtr, descrPtr, cpuCode);
        }
        else
        {
            snetTwistTxMacCountUpdate(devObjPtr, descrPtr, targetPort);
            smainFrame2PortSend(devObjPtr,
                                targetPort,
                                frameDataPtr,
                                frameDataLength,
                                GT_FALSE);
        }
        return;
    }

    sstackTwistTxSend(devObjPtr, descrPtr, frameDataPtr, frameDataLength,
                            targetDevice);
}


/*******************************************************************************
* snetRxDmaFillDescriptor
*
* DESCRIPTION:
*       Fill given Rx descriptor's Command/Status fields
*
* INPUTS:
*       packetBuff      - Packet to transmit.
*       length          - Length (in bytes) of packet to transmit.
*       rxDesc          - Rx descriptor to fill.
*
* OUTPUTS:
*       nextRxDesc      - Pointer to the next Rx descriptor in chain if queueing
*                         succeeded.
*
* RETURNS:
*       GT_OK if packet transmitted successfully,
*       GT_FAIL on resource error.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static GT_BOOL snetRxDmaFillDescriptor
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U32  cpuCode,
    IN  STRUCT_RX_DESC *rxDesc,
    OUT STRUCT_RX_DESC **nextRxDesc
)
{
    DECLARE_FUNC_NAME(snetRxDmaFillDescriptor);


    GT_U8  * packetBuff;  /* Packet to transmit */

    GT_U32  length;                 /* Length (in bytes) of packet to transmit*/

    GT_U32  buffLen;                /* Length of the current buffer.        */
    /*GT_U8   *buffPtr; */          /* Pointer to the current buffer to copy*/
                                    /* the packet data into.                */
    GT_U32  sentBytes;              /* Number of bytes already sent.        */
    STRUCT_RX_DESC *firstRxDesc;    /* First rx desc, used to set the O bit.*/

    GT_U8 vlan_ports[64], vlan_tagged_ports[64], stp_for_ports[64];

    GT_U8   added8Bytes[8]={0};     /*Data that added at the end of the packet*/
    GT_U8   *add8BytesPtr;          /* A pointer to the additional 8 bytes.   */
    GT_U8   temp;

    GT_U32  i,regAddress;
    firstRxDesc = rxDesc;
    sentBytes   = 0;

    memset(vlan_ports, 0, 64);
    memset(vlan_tagged_ports, 0, 64);
    memset(stp_for_ports, 0, 64);

    snetTwistVltTables(devObjPtr, descrPtr,
                       vlan_ports, vlan_tagged_ports, stp_for_ports);

    /* Fill egress buffer for  CPU port */
    __LOG(("Fill egress buffer for  CPU port"));
    length = snetTwistFillEgressBuffer(devObjPtr,
                                       descrPtr,
                                       descrPtr->srcVlanTagged);

    packetBuff = devObjPtr->egressBuffer - 2;

    /* Prepend with 2 bytes to 32-bit align the IP and TCP/UDP */
    length += 2;
    if (devObjPtr->crcBytesAdd)
    {
        /* Add 4 bytes to packet header for CRC (used in RTG/RDE tests) */
        length += 4;
    }

    /* add8BytesPtr points to the end of the packet */
    add8BytesPtr = (GT_U8*)packetBuff+length;

    /*bit  0    */
    added8Bytes[0] = added8Bytes[0] | ((GT_U8)(descrPtr->uplink & 0x1));
    /*bits 3-4  */
    added8Bytes[0] = added8Bytes[0] | ((descrPtr->dropPrecedence & 0x2) << 3);
    /*bit  5    */
    added8Bytes[0] = added8Bytes[0] | ((descrPtr->srcVlanTagged & 0x1) << 5);
    /*bits 6-7*/
    added8Bytes[0] = added8Bytes[0] | ((descrPtr->vlanCmd & 0x2) << 6);
    /*bits 8-10 */
    added8Bytes[1] = descrPtr->userPriorityTag & 0x7;
    /*bits 11-15*/
    added8Bytes[1] = added8Bytes[1] | (descrPtr->vid & 0x1F) << 3;
    /*bits 16-22*/
    added8Bytes[2] = descrPtr->vid >> 5;

/*
    added8Bytes[0] = (GT_U8)hwByteSwap(added8Bytes[0]);
    added8Bytes[1] = (GT_U8)hwByteSwap(added8Bytes[1]);
    added8Bytes[2] = (GT_U8)hwByteSwap(added8Bytes[2]);
*/

    /* Swap two bytes with vlanId because those treatment in the core function */
    temp = added8Bytes[1];
    added8Bytes[1] = added8Bytes[2];
    added8Bytes[2] = temp;

    /* Append with 8 bytes to added data */
    for( i=0; i<8; i++)
    {
       add8BytesPtr[i] = added8Bytes[7-i];
    }

    length += 8;

    while(sentBytes < length)
    {
        /* Resource error.  */
        if(RX_DESC_GET_OWN_BIT(rxDesc) == RX_DESC_CPU_OWN)
        {
            *nextRxDesc = (STRUCT_RX_DESC*)((GT_UINTPTR)rxDesc->nextDescPointer);
            return GT_FALSE;
        }

        buffLen = RX_DESC_GET_BUFF_SIZE_FIELD(rxDesc);
        /*buffPtr = (GT_U8*)rxDesc->buffPointer;*/

        if(buffLen > length - sentBytes)
            buffLen = length - sentBytes;

        /*memcpy(buffPtr, &(packetBuff[sentBytes]), buffLen);*/

        /* write data into the DMA */
        scibDmaWrite(devObjPtr->deviceId,rxDesc->buffPointer,NUM_BYTES_TO_WORDS(buffLen), (GT_U32*)&(packetBuff[sentBytes]),SCIB_DMA_BYTE_STREAM);


        RX_DESC_SET_BYTE_COUNT_FIELD(rxDesc,buffLen);

        if(rxDesc != firstRxDesc)
            RX_DESC_SET_OWN_BIT(rxDesc,RX_DESC_CPU_OWN);
        else
            SMEM_U32_SET_FIELD(rxDesc->word1, 27, 1, 1);

        /* Input encapsulation */
        SMEM_U32_SET_FIELD(rxDesc->word1, 23, 3, descrPtr->inputIncapsulation);
        /* Source device */
        SMEM_U32_SET_FIELD(rxDesc->word1, 16, 7, descrPtr->srcDevice);
        /* Source port */
        SMEM_U32_SET_FIELD(rxDesc->word1, 10, 6, descrPtr->srcPort);
        /* CPU code */
        SMEM_U32_SET_FIELD(rxDesc->word1, 2, 8, cpuCode);
        /* Mirror cause      */
        SMEM_U32_SET_FIELD(rxDesc->word1, 0, 2, descrPtr->flowMirrorCause);

        sentBytes += buffLen;

        if(sentBytes < length)
            rxDesc = (STRUCT_RX_DESC*)((GT_UINTPTR)rxDesc->nextDescPointer);
    }

    /* Set the last bit for the last desc.  */
    SMEM_U32_SET_FIELD(rxDesc->word1, 26, 1, 1);

    /* Set the Own bit for the first desc.  */
    RX_DESC_SET_OWN_BIT(firstRxDesc, RX_DESC_CPU_OWN);

    *nextRxDesc = (STRUCT_RX_DESC*)((GT_UINTPTR)rxDesc->nextDescPointer);

    /* update the next descriptor to handle */
    CUR_RX_DESC_POINTER_REG(descrPtr->trafficClass , &regAddress);
    smemRegSet(devObjPtr, regAddress, (GT_U32)rxDesc->nextDescPointer);

    return GT_TRUE;
}

/*******************************************************************************
* snetTwistModifyDscpOrExp
*
* DESCRIPTION:
*       for non-mpls packet -- modify the dscp in the IP header and re-calc
*                              checksum
*       for mpls packet - TBD
*
* INPUTS:
*        egrBufPtr   -  pointer to the packet(buffer) to update.
*        descrPtr  -    pointer to the frame's descriptor.
*
* OUTPUTS:
*       egrBufPtr      - pointer to the packet(buffer) to update.
*
*
* RETURNS:
*       Length of the filled buffer
*
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static void snetTwistModifyDscpOrExp
(
    INOUT GT_U8                * egrBufPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U32                   tagged
)
{
    GT_U32  diffToIp,ipCheckSum;
    GT_U8   *ipPtr;

    if(descrPtr->flowTemplate == SKERNEL_MPLS_FLOW_E)
    {
        /* TBD */
        return;
    }
    else
    if (descrPtr->ipHeaderPtr)
    {
        /* modify dscp in the IP header */
        diffToIp = (GT_UINTPTR)descrPtr->ipHeaderPtr-
                   (GT_UINTPTR)descrPtr->dstMacPtr;

        if(descrPtr->srcVlanTagged)
        {
            diffToIp-=4;
        }

        if(tagged)
        {
            diffToIp+=4;
        }

        ipPtr = &egrBufPtr[diffToIp];

        /* modify dscp value */
        ipPtr[1] = (ipPtr[1] & 0x03) | (descrPtr->dscp & 0x3f) << 2;

        ipPtr[10] = 0;
        ipPtr[11] = 0;
        /* re-Calculate IP packet checksum */
        ipCheckSum = ipV4CheckSumCalc(ipPtr,
                                     (GT_U16)((ipPtr[0] & 0xF) * 4));

        /* Checksum of the IP header and IP options */
        ipPtr[10] = (GT_U8)((ipCheckSum & 0xff00) >> 8);
        ipPtr[11] = (GT_U8)(ipCheckSum  & 0x00ff);
    }
}

/*******************************************************************************
* snetTwistFillEgressBuffer
*
* DESCRIPTION:
*       Fill egress buffer in the descriptor
*
* INPUTS:
*        devObjPtr   -  pointer to device object.
*        descrPtr  -    pointer to the frame's descriptor.
*
* OUTPUTS:
*       descrPtr      - pointer to the frame's descriptor.
*
*
* RETURNS:
*       Length of the filled buffer
*
*
* COMMENTS:
*       None.
*
*******************************************************************************/
GT_U32 snetTwistFillEgressBuffer
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U32 tagged
)
{
    DECLARE_FUNC_NAME(snetTwistFillEgressBuffer);

    GT_U8_PTR   egrBufPtr;
    GT_U8_PTR   dataPtr;
    GT_U32      egressBufLen, copySize;
    GT_U8       tagData[4];
    SBUF_BUF_ID frameBuf;

    /* Egress buffer pointer to copy to */
    egrBufPtr = devObjPtr->egressBuffer;
    /* Actual data pointer to copy from */
    frameBuf = descrPtr->frameBuf;
    dataPtr = frameBuf->actualDataPtr;
    copySize = 2 * SGT_MAC_ADDR_BYTES;
    /* Append DA and SA to the egress buffer */
    MEM_APPEND(egrBufPtr, dataPtr, copySize);
    dataPtr += copySize;
    if (tagged)
    {
        snetTagDataGet(descrPtr->userPriorityTag,descrPtr->vid,GT_FALSE,tagData);
        copySize = sizeof(tagData);
        MEM_APPEND(egrBufPtr, tagData, copySize);

        if (descrPtr->srcVlanTagged == 0)
        {
            /* Create and insert VLAN tag */
            __LOG(("Create and insert VLAN tag"));
            /* Copy VLAN tag */
        }
        else
        {
            /* the packet already has VLAN tag */
            __LOG(("the packet already has VLAN tag"));
            /* but we need to modify its VPT,VID */
            dataPtr += 4;
        }
    }
    else
    {
        if (descrPtr->srcVlanTagged == 1)
        {
            /* Skip VLAN tag */
            __LOG(("Skip VLAN tag"));
            dataPtr += 4;
        }
    }

    if(descrPtr->frameBuf->actualDataSize < (GT_U32)(dataPtr - frameBuf->actualDataPtr))
        return 0;

    copySize = descrPtr->frameBuf->actualDataSize - (dataPtr - frameBuf->actualDataPtr);

    /* Copy the rest of the frame data */
    MEM_APPEND(egrBufPtr, dataPtr, copySize);

    if(descrPtr->modifyDscpOrExp == GT_TRUE)
    {
        /* we need to modify the IP/MPLS header */
        snetTwistModifyDscpOrExp(devObjPtr->egressBuffer, descrPtr,tagged);
    }

    egressBufLen = egrBufPtr - devObjPtr->egressBuffer;

    return egressBufLen;
}


/*******************************************************************************
*   snetTwistTxBufferIntrCall
*
* DESCRIPTION:
*         simulate tx end interrupt
* INPUTS:
*        devObj_PTR - pointer to device object.
*        descr_PTR  - pointer to the frame's descriptor.
* OUTPUTS:
*
* COMMENTS:
*           The Tx Buffer for Queue 0 bit indicates that a Tx buffer returned to CPU ownership
*           or that the SDMA finished transmission of a Tx frame.
*           This bit is set upon closing any Tx descriptor that has its EI bit set. In order to
*           limit the interrupts to frame (rather than to buffer) boundaries, it is recommended
*           that the user set the EI bit only in the last descriptor.
*
*******************************************************************************/

static void snetTwistTxBufferIntrCall(
    IN SKERNEL_DEVICE_OBJECT    *devObj_PTR,
    IN SKERNEL_FRAME_DESCR_STC  *descr_PTR
)
{
    GT_U32  fldValue;
    GT_BOOL globIntrNotify = GT_FALSE;

    /* EI(enable interrupt) field of descriptor - should interrupt be sent on TX_BUFFER event
     * not checked because there is no interface TAPI or CORE to configure it and it's
     * always set in coreTransmitPacket
     */

    /* check that interrupt enabled */
    smemRegFldGet(devObj_PTR, TRANS_SDMA_INTR_MASK_REG,
                  1 + descr_PTR->frameBuf->userInfo.data.sapiTxPacket.txQueue,
                  1, &fldValue);
    /* set The Tx Buffer for Queue <n> */
    if (fldValue)
    {
        smemRegFldSet(devObj_PTR, TRANS_SDMA_INTR_CAUSE_REG,
                      1 + descr_PTR->frameBuf->userInfo.data.sapiTxPacket.txQueue, 1, 1);
        smemRegFldSet(devObj_PTR, TRANS_SDMA_INTR_CAUSE_REG,
                      1 + descr_PTR->frameBuf->userInfo.data.sapiTxPacket.txQueue, 1, 1);

        /* PCI interrupt */
        smemPciRegFldGet(devObj_PTR, PCI_INT_MASK_REG, 18, 1, &fldValue);
        if (fldValue)
        {
            smemPciRegFldSet(devObj_PTR, PCI_INT_CAUSE_REG, 18, 1, 1);
            smemPciRegFldSet(devObj_PTR, PCI_INT_CAUSE_REG, 0, 1, 1);
            globIntrNotify = GT_TRUE;
        }
    }

    if (globIntrNotify == GT_TRUE)
    {
        scibSetInterrupt(devObj_PTR->deviceId);
    }

}

/*******************************************************************************
*   snetTwistCpuTxMcast
*
* DESCRIPTION:
*         send frame to multiple ports
* INPUTS:
*        devObj_PTR - pointer to device object.
*        descr_PTR  - pointer to the frame's descriptor.
*        outgoing_ports_PTR - pointer to array of outgoing ports
*                             may be members of vidx or single port
* OUTPUTS:
*        outgoing_ports_PTR - pointer to array of outgoing ports
*                             updated accordingly to stp state etc.
* COMMENTS:
*
*******************************************************************************/
static void snetTwistCpuTxCalcOutgoingPorts
(
    IN  SKERNEL_DEVICE_OBJECT    *deviceObj_PTR,
    IN  SKERNEL_FRAME_DESCR_STC  *descr_PTR,
    INOUT GT_U8                  *outgoing_ports_PTR
)
{
    GT_U8   vlan_ports[64], vlan_tagged_ports[64];
    GT_U8   stp_for_ports[64];
    GT_U32  regAddress, fldValue;
    GT_U32  memberBits[2];
    GT_U8   trunkGroupId;
    GT_U8   i;
    GT_U8   hashVal;
    GT_U32  stpState, stpPerVln;

    /* Get VLAN ports info */
    snetTwistVltTables(deviceObj_PTR, descr_PTR, vlan_ports,
                                  vlan_tagged_ports, stp_for_ports);

    /* forward only to ports of defined vlan */
    for (i = 0; i < 64; i++)
        outgoing_ports_PTR[i] &= vlan_ports[i];

    smemRegFldGet(deviceObj_PTR, L2_INGRESS_CTRL_REG, 5, 1, &stpPerVln);
    /* forward only to ports with STP forward or STP disabled */
    for (i = 0; i < 64; i++)
    {
        if(stpPerVln)
        {
            if ((stp_for_ports[i] != 0) && (stp_for_ports[i] != 3))
            {
                outgoing_ports_PTR[i] = 0;
            }
        }
        else
        {
            /* Bridge Port<n> Control Register */
            BRDG_PORT0_CTRL_REG(i, &regAddress);
            smemRegFldGet(deviceObj_PTR, regAddress, 0, 2, &stpState);
            if ((stpState != 0) && (stpState != 3))
            {
                outgoing_ports_PTR[i] = 0;
            }
        }
    }

    /* calculate outgoing trunk ports if exist */
    hashVal = descr_PTR->bits15_2.useVidx_0.trunkHash;
    TRUNK_DESIGN_PORTS_HASH_REG(hashVal, &regAddress);
    smemRegFldGet(deviceObj_PTR, regAddress, 0, 32, &fldValue);
    /* DesPortTrunk bits 0-31 */
    memberBits[0] = fldValue;
    regAddress += 0x20000;
    smemRegFldGet(deviceObj_PTR, regAddress, 0, 21, &fldValue);
    /* DesPortTrunk bits 32-52 */
    memberBits[1] = fldValue;

    for (i = 0; i < 32; i++)
    {
        if (outgoing_ports_PTR[i] == 0)
            continue;

        if(stpPerVln)
        {
            if ((stp_for_ports[i] == 3) || (stp_for_ports[i] == 0))
            {
                /* Bridge Port<n> Control Register */
                BRDG_PORT0_CTRL_REG(i, &regAddress);
                smemRegFldGet(deviceObj_PTR, regAddress, 2, 3, &fldValue);
                /* TrunkGroupId */
                trunkGroupId = (GT_U8)fldValue;
                if ((trunkGroupId == 0) && ((memberBits[0] & 0x01) == 0))
                {
                    outgoing_ports_PTR[i] = 0;
                }
            }
        }
        else
        {
            /* Bridge Port<n> Control Register */
            BRDG_PORT0_CTRL_REG(i, &regAddress);
            smemRegFldGet(deviceObj_PTR, regAddress, 0, 2, &stpState);
            if ((stpState == 0) || (stpState == 3))
            {
                smemRegFldGet(deviceObj_PTR, regAddress, 2, 3, &fldValue);
                /* TrunkGroupId */
                trunkGroupId = (GT_U8)fldValue;
                if ((trunkGroupId == 0) && ((memberBits[0] & 0x01) == 0))
                {
                    outgoing_ports_PTR[i] = 0;
                }
            }
        }
        memberBits[0] >>= 1;
    }

    for (i = 32; i < 64; i++)
    {
        if (outgoing_ports_PTR[i] == 0)
            continue;

        if(stpPerVln)
        {
            if ((stp_for_ports[i] == 3) || (stp_for_ports[i] == 0))
            {
                /* Bridge Port<n> Control Register */
                BRDG_PORT0_CTRL_REG(i, &regAddress);
                smemRegFldGet(deviceObj_PTR, regAddress, 2, 3, &fldValue);
                /* TrunkGroupId */
                trunkGroupId = (GT_U8)fldValue;
                if ((trunkGroupId == 0) && ((memberBits[1] & 0x01) == 0))
                {
                    outgoing_ports_PTR[i] = 0;
                }
            }
        }
        else
        {
            /* Bridge Port<n> Control Register */
            BRDG_PORT0_CTRL_REG(i, &regAddress);
            smemRegFldGet(deviceObj_PTR, regAddress, 0, 2, &stpState);
            if ((stpState == 0) || (stpState == 3))
            {
                smemRegFldGet(deviceObj_PTR, regAddress, 2, 3, &fldValue);
                /* TrunkGroupId */
                trunkGroupId = (GT_U8)fldValue;
                if ((trunkGroupId == 0) && ((memberBits[1] & 0x01) == 0))
                {
                    outgoing_ports_PTR[i] = 0;
                }
            }
        }

        memberBits[1] >>= 1;
    }
}

/*******************************************************************************
*   snetTwistCpuTxFrameProcess
*
* DESCRIPTION:
*       Process frame sent from CPU accordingly to rules of TWISTD
*
* INPUTS:
*       deviceObj_PTR   - pointer to device object.
*       descr_PTR       - pointer to the frame's descriptor.
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
GT_VOID snetTwistCpuTxFrameProcess
(
    IN SKERNEL_DEVICE_OBJECT      *deviceObj_PTR,
    IN SKERNEL_FRAME_DESCR_STC    *descr_PTR
)
{
    GT_U8   vlan_ports[64], vlan_tagged_ports[64];
    GT_U8   stp_for_ports[64], outgoing_ports[64];
    GT_U32  tagged;
    GT_U8   i;
    GT_BOOL vlnFound;
    GT_U32  fldValue = 0;


    memset(vlan_ports, 0, 64);
    memset(vlan_tagged_ports, 0, 64);
    memset(stp_for_ports, 0, 64);
    memset(outgoing_ports , 0 , 64);



    /* Save pointer to prepended data to packet descriptor memory */
    descr_PTR->extendTxPktPtr =
        (GT_U32 *)descr_PTR->frameBuf->actualDataPtr - 16;

    /* Get VLAN ports info */
    vlnFound = snetTwistVltTables(deviceObj_PTR, descr_PTR, vlan_ports,
                                  vlan_tagged_ports, stp_for_ports);

    if(descr_PTR->useVidx)
    {/* if send to vidx */
        snetTwistVidxPortsGet(deviceObj_PTR,
                              descr_PTR->bits15_2.useVidx_1.vidx,
                              outgoing_ports);
    }
    else
    {/* if send to single port */
        outgoing_ports[descr_PTR->bits15_2.useVidx_0.targedPort] = 1;
    }

    /* if frame has ethernet encapsulation */
    if(descr_PTR->inputIncapsulation == GT_REGULAR_PCKT)
    {
        /* check if the egress filtering is enabled */
        if (descr_PTR->macDaType == SKERNEL_UNICAST_MAC_E)
        {
            smemRegFldGet(deviceObj_PTR, EGRESS_BRIDGING_REG, 5, 1, &fldValue);
        }
        if (fldValue == 1)
        {
            if (!vlnFound)
            {
                snetTwistDrop(deviceObj_PTR, descr_PTR);
                snetTwistTxBufferIntrCall(deviceObj_PTR, descr_PTR);
                return;
            }

            snetTwistCpuTxCalcOutgoingPorts(deviceObj_PTR, descr_PTR, outgoing_ports);
        }
    }

    if(descr_PTR->useVidx)
    {/* if send to vidx */
        for (i = 0; i < 64 ; i++)
        {
            if (outgoing_ports[i] == 1)
            {
                /* if in descriptor field tagged set then
                 * if frame doesn't contain tag it's forwarded untagged
                 */
                if(descr_PTR->vlanCmd)
                    tagged = (vlan_tagged_ports[i] == 1) ? 1 : 0;
                else
                    tagged = 0;

                snetTwistTx2Port(deviceObj_PTR, descr_PTR, i, tagged);
            }
        }
        if (descr_PTR->vidHasNoUplinkMember == 0)
        {
            sstackTwistToUplink(deviceObj_PTR, descr_PTR);
        }
    }
    else
    {/* if send to single port */
        if(descr_PTR->bits15_2.useVidx_0.targedDevice != deviceObj_PTR->deviceId)
        {
            sstackTwistToUplink(deviceObj_PTR, descr_PTR);
        }
        else
        {
            if(outgoing_ports[descr_PTR->bits15_2.useVidx_0.targedPort])
            {
                /* if in descriptor field tagged set then
                 * if frame doesn't contain tag it's forwarded untagged
                 */
                if(descr_PTR->vlanCmd)
                    tagged = (vlan_tagged_ports[descr_PTR->bits15_2.useVidx_0.targedPort] == 1) ? 1 : 0;
                else
                    tagged = 0;
                snetTwistTx2Port(deviceObj_PTR, descr_PTR, descr_PTR->bits15_2.useVidx_0.targedPort,
                                 tagged);
            }
        }
    }

    snetTwistTxBufferIntrCall(deviceObj_PTR, descr_PTR);
}

/*******************************************************************************
* snetTwistFillEgressBuffer
*
* DESCRIPTION:
*    send frame to uplink
*
* INPUTS:
*    devObjPtr - pointer to the device object
*    descrPtr  - pointer to the frame descriptor
*    frameDataPtr - pointer to the data
*    frameDataLength - frame size
*    targetDevice    - target device
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static void sstackTwistTxSend
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U8 * frameDataPtr,
    IN GT_U32 frameDataLength,
    IN GT_U32 targetDevice
)
{
    if (devObjPtr->uplink.partnerDeviceID == targetDevice)
    {
        sstackTwistToUplink(devObjPtr,descrPtr);
    }
}

/*******************************************************************************
* sstackTwistToUplink
*
* DESCRIPTION:
*    fill uplink descriptor and send the frame to uplink
*
* INPUTS:
*    devObjPtr - pointer to the device object
*    descrPtr  - pointer to the frame descriptor
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*       None.
*
*******************************************************************************/
void sstackTwistToUplink
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(sstackTwistToUplink);

    GT_STATUS                       status;
    SKERNEL_UPLINK_DESC_STC         uplinkDesc;
    GT_U32                          fldValue;

    if (devObjPtr->uplink.type == (SKERNEL_UPLINK_TYPE_ENT)SKERNEL_UPLINK_FRAME_TYPE_NONE_E)
        return;
    smemRegFldGet(devObjPtr, GLOBAL_CONTROL_REG, 17, 1, &fldValue);
    if (!fldValue)
        return;

    uplinkDesc.type                      = SKERNEL_UPLINK_FRAME_PP_TYPE_E;
    uplinkDesc.data.PpPacket.source.frameBuf = descrPtr->frameBuf;
    uplinkDesc.data.PpPacket.source.byteCount = descrPtr->byteCount ;
    uplinkDesc.data.PpPacket.source.frameType = descrPtr->frameType ;
    uplinkDesc.data.PpPacket.source.srcTrunkId = descrPtr->srcTrunkId ;
    uplinkDesc.data.PpPacket.source.srcPort = descrPtr->srcPort ;
    uplinkDesc.data.PpPacket.source.uplink = devObjPtr->uplink.partnerDeviceID ;
    uplinkDesc.data.PpPacket.source.vid = descrPtr->vid ;
    uplinkDesc.data.PpPacket.source.trafficClass = descrPtr->trafficClass ;
    uplinkDesc.data.PpPacket.source.pktCmd = descrPtr->pktCmd ;
    uplinkDesc.data.PpPacket.source.srcDevice = descrPtr->srcDevice ;
    uplinkDesc.data.PpPacket.useVidx = descrPtr->useVidx ;
    uplinkDesc.data.PpPacket.macDaType = descrPtr->macDaType;
    uplinkDesc.data.PpPacket.llt = descrPtr->llt ;
    uplinkDesc.data.PpPacket.source.srcVlanTagged = descrPtr->srcVlanTagged  ;

    if (uplinkDesc.data.PpPacket.useVidx)
    {
        uplinkDesc.data.PpPacket.dest.vidx = descrPtr->bits15_2.useVidx_1.vidx;
    }
    else
    {
        uplinkDesc.data.PpPacket.dest.targetInfo.targetPort =
                                descrPtr->bits15_2.useVidx_0.targedPort;
        uplinkDesc.data.PpPacket.dest.targetInfo.targetDevice =
                                descrPtr->bits15_2.useVidx_0.targedDevice;
        uplinkDesc.data.PpPacket.dest.targetInfo.lbh = 0;
    }
    /*send the frame to uplink*/
    __LOG(("send the frame to uplink"));
    status = smainSwitchFabricFrame2UplinkSend(devObjPtr,&uplinkDesc);
    if (status != GT_OK)
    {
       printf(" sstackTwistToUplink : failed to send frame to uplink\n");
    }
}

/*******************************************************************************
* snetTwistStpPortsApply
*
* DESCRIPTION:
*    Check VLAN or VIDX ports for STP state
*
* INPUTS:
*    devObjPtr - pointer to the device object
*    descrPtr  - pointer to the frame descriptor
*    stpPorts  - stp members ports array
*    port      - vlan/vidx members ports
*
* OUTPUTS:
*    port      - vlan/vidx members ports
*
* RETURNS:
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static void snetTwistStpPortsApply
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U8  stpPorts[],
    INOUT GT_U8  ports[]
)
{
    DECLARE_FUNC_NAME(snetTwistStpPortsApply);

    GT_U32 regAddress;
    GT_U32 fldValue;
    GT_U32 stpPerVln;
    GT_U32 stpState;
    GT_U32 i;

    smemRegFldGet(devObjPtr, L2_INGRESS_CTRL_REG, 5, 1, &fldValue);
    /* PerVLAN SpanEn */
    stpPerVln = fldValue;
    /* forward only to ports with STP forward or STP disabled */
    __LOG(("forward only to ports with STP forward or STP disabled"));
    for (i = 0; i < STWIST_MAX_DEVICE_PORTS; i++)
    {
        if(stpPerVln)
        {
            if ((stpPorts[i] != 0) && (stpPorts[i] != 3))
            {
                ports[i] = 0;
            }
        }
        else
        {
            /* Bridge Port<n> Control Register */
            BRDG_PORT0_CTRL_REG(i, &regAddress);
            smemRegFldGet(devObjPtr, regAddress, 0, 2, &fldValue);
            /* STPortState */
            stpState = fldValue;
            if ((stpState != 0) && (stpState != 3))
            {
                ports[i] = 0;
            }
        }
    }
}

/*******************************************************************************
*   snetTwistPortEgressCount
*
* DESCRIPTION:
*       do port egress counting
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descr_PTR       - pointer to the frame's descriptor.
*       egressPort      - port that packet is transmitted to (can be CPU port)
*       egressCounter   - type of counting to perform
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
GT_VOID snetTwistPortEgressCount
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U32  egressPort,
    IN SKERNEL_EGRESS_COUNTER_ENT egressCounter
)
{
    DECLARE_FUNC_NAME(snetTwistPortEgressCount);

    GT_U32  regCountConfig[2];
    GT_U32  regBaseAddr;
    GT_U32  ii;
    GT_U32  port,vid,tc,dp;
    GT_BOOL updateCounters[2];
    GT_U32  regCounterIndex;
    GT_U32  tmpCounterVal,tmpCounterAddr;

    for(ii=0;ii<2;ii++)
    {
        updateCounters[ii]=GT_FALSE ;
        regBaseAddr =TRANSMIT_QUEUE_MIB_COUNT_BASE_REG + ii*0x1000;

        smemRegGet(devObjPtr,regBaseAddr,&regCountConfig[ii]);

        /* check vid */
        __LOG(("check vid"));
        if(SMEM_U32_GET_FIELD(regCountConfig[ii],1,1))
        {
            vid = SMEM_U32_GET_FIELD(regCountConfig[ii],10,12);
            if(vid!=descrPtr->vid)
            {
                continue;
            }
        }

        if(egressCounter!=SKERNEL_EGRESS_COUNTER_EGRESS_BRG_FILTER_E)
        {
            /* check port */
            if(SMEM_U32_GET_FIELD(regCountConfig[ii],0,1))
            {
                port = SMEM_U32_GET_FIELD(regCountConfig[ii],4,6);
                if(port!=egressPort)
                {
                    continue;
                }
            }

            /* check tc */
            if(SMEM_U32_GET_FIELD(regCountConfig[ii],2,1))
            {
                tc = SMEM_U32_GET_FIELD(regCountConfig[ii],22,3);
                if(tc!=descrPtr->trafficClass)
                {
                    continue;
                }
            }

            /* check dp */
            if(SMEM_U32_GET_FIELD(regCountConfig[ii],3,1))
            {
                dp = SMEM_U32_GET_FIELD(regCountConfig[ii],25,2);
                if(dp!=descrPtr->dropPrecedence)
                {
                    continue;
                }
            }
        }

        updateCounters[ii]=GT_TRUE ;
    }

    if(updateCounters[0]==GT_FALSE && updateCounters[1]==GT_FALSE )
    {
        /* nothing to update */
        return;
    }

    /* check type of packet */
    if(egressCounter!=SKERNEL_EGRESS_COUNTER_EGRESS_BRG_FILTER_E)
    {
        regCounterIndex = 0;
        switch(descrPtr->macDaType)
        {
            case SKERNEL_UNICAST_MAC_E:
                /* unicast*/
                regCounterIndex=1;
                break;
            case SKERNEL_MULTICAST_MAC_E:
                /* multicast*/
                regCounterIndex=2;
                break;
            case SKERNEL_BROADCAST_MAC_E:
            case SKERNEL_BROADCAST_ARP_E:
                /* broadcast*/
                regCounterIndex=3;
                break;
            default:
                /* unicast*/
                regCounterIndex=1;
                break;
        }

        if(descrPtr->macDaLookupResult==0 &&
           regCounterIndex==1 &&
           descrPtr->srcPort != TWIST_CPU_PORT_CNS)
        {
            /* unknown unicast */
            regCounterIndex=2;
        }

        tmpCounterAddr = TRANSMIT_QUEUE_MIB_COUNT_BASE_REG+4*regCounterIndex;
    }
    else
    {
        /* egress filtering */
        tmpCounterAddr = BRIDGE_EGRESS_FILTERED_BASE_REG;
    }


    for(ii=0;ii<2;ii++)
    {
        if(updateCounters[ii]==GT_FALSE)
        {
            continue;
        }

        tmpCounterAddr+=(ii*0x1000);

        /* get counter old value */
        smemRegGet(devObjPtr,tmpCounterAddr,&tmpCounterVal);

        tmpCounterVal++;

        /* update counter value */
        smemRegSet(devObjPtr,tmpCounterAddr,tmpCounterVal);
    }

    return;
}

/*******************************************************************************
*   snetTwistOutlifGet
*
* DESCRIPTION:
*       snetTwistOutlifGet
*
* INPUTS:
*       outLifMem   -  outLif structure
* OUTPUTS:
*       outLifEntryPtr  - pointer to LLL_OUTLIF_UNT
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void snetTwistOutlifGet
(
    IN GT_U32  outLifMem,
    OUT LLL_OUTLIF_STC * outLifEntryPtr
)
{
    outLifEntryPtr->bottomEncap =
        (GT_U8) WORD_FIELD_GET(&outLifMem, 0, 0, 2);

    outLifEntryPtr->vlanId =
        (GT_U16)WORD_FIELD_GET(&outLifMem, 0, 2, 12);
    outLifEntryPtr->daTag =
        (GT_U8) WORD_FIELD_GET(&outLifMem, 0, 14, 1);
    outLifEntryPtr->useVidx =
        (GT_U8) WORD_FIELD_GET(&outLifMem, 0, 15, 1);

    if (outLifEntryPtr->useVidx == 1)
    {
        outLifEntryPtr->target.vidx =
            (GT_U16) WORD_FIELD_GET(&outLifMem, 0, 16, 14);
    }
    else
    {
        outLifEntryPtr->target.trunk.trgIsTrunk =
            (GT_U8) WORD_FIELD_GET(&outLifMem, 0, 16, 1);

        if (outLifEntryPtr->target.trunk.trgIsTrunk == 1)
        {
            outLifEntryPtr->target.trunk.trgTrunkId =
                (GT_U8) WORD_FIELD_GET(&outLifMem, 0, 17, 3);
            outLifEntryPtr->target.trunk.trgTrunkHush =
                (GT_U8) WORD_FIELD_GET(&outLifMem, 0, 20, 10);
        }
        else
        {
            outLifEntryPtr->target.port.trgPort =
                (GT_U8) WORD_FIELD_GET(&outLifMem, 0, 17, 6);
            outLifEntryPtr->target.port.trgDev =
                (GT_U8) WORD_FIELD_GET(&outLifMem, 0, 23, 7);
        }
    }
}

/*******************************************************************************
*   snetTwistMplsHa
*
* DESCRIPTION:
*        Make MPLS specific header alteration procedure
* INPUTS:
*        devObjPtr   -  pointer to device object.
*        descrPtr  -    pointer to the frame's descriptor.
*        egrBufPtr -    pointer to egress buffer
*        dataPtr  -     pointer to actual data to be copied
*
*
*******************************************************************************/
static GT_U32 snetTwistMplsHa
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U8 * egrBufPtr,
    IN GT_U8 * dataPtr
)
{
    DECLARE_FUNC_NAME(snetTwistMplsHa);

    GT_U32 copySize;
    GT_U32 labelEnt = 0;
    GT_U8  * ipHeaderPtr;
    GT_U32 ipCheckSum;
    GT_BOOL calcCheckSum;

    if (descrPtr->mplsDone == 0 ||
        descrPtr->mplsTopLablePtr == 0)
    {
        return 0;
    }

    if (descrPtr->mplsCmd == SKERNEL_MPLS_CMD_SWAP_E)
    {
        WORD_FIELD_SET(&labelEnt, 0, 12, 20, descrPtr->label);
        WORD_FIELD_SET(&labelEnt, 0, 10,  3, descrPtr->exp);
        /* Bottom of stack */
        __LOG(("Bottom of stack"));
        WORD_FIELD_SET(&labelEnt, 0,  9,  1, 1);
        WORD_FIELD_SET(&labelEnt, 0,  0,  8, descrPtr->ttl);

        MEM_APPEND(egrBufPtr, &labelEnt, sizeof(GT_U32));
        dataPtr += sizeof(GT_U32);
    }
    else
    if (descrPtr->mplsCmd == SKERNEL_MPLS_CMD_POP_SWAP_E)
    {
        WORD_FIELD_SET(&labelEnt, 0, 12, 20, descrPtr->label);
        WORD_FIELD_SET(&labelEnt, 0, 10,  3, descrPtr->exp);
        /* Bottom of stack */
        WORD_FIELD_SET(&labelEnt, 0,  9,  1, 1);
        WORD_FIELD_SET(&labelEnt, 0,  0,  8, descrPtr->ttl);

        MEM_APPEND(egrBufPtr, &labelEnt, sizeof(GT_U32));
        dataPtr += 2 * sizeof(GT_U32);
    }
    else
    if (descrPtr->mplsCmd == SKERNEL_MPLS_CMD_PUSH1_LABLE_E)
    {
        WORD_FIELD_SET(&labelEnt, 0, 12, 20, descrPtr->label);
        WORD_FIELD_SET(&labelEnt, 0, 10,  3, descrPtr->exp);
        WORD_FIELD_SET(&labelEnt, 0,  9,  1, 0);
        WORD_FIELD_SET(&labelEnt, 0,  0,  8, descrPtr->ttl);

        MEM_APPEND(egrBufPtr, &labelEnt, sizeof(GT_U32));
        MEM_APPEND(egrBufPtr, descrPtr->mplsTopLablePtr, sizeof(GT_U32));
        dataPtr += 2 * sizeof(GT_U32);
    }
    else
    if (descrPtr->mplsCmd == SKERNEL_MPLS_CMD_POP1_LABLE_E)
    {
        dataPtr += sizeof(GT_U32);
    }
    else
    if (descrPtr->mplsCmd == SKERNEL_MPLS_CMD_POP2_LABLE_E)
    {
        dataPtr += 2 * sizeof(GT_U32);
    }
    else
    if (descrPtr->mplsCmd == SKERNEL_MPLS_CMD_POP3_LABLE_E)
    {
        dataPtr += 3 * sizeof(GT_U32);
    }

    /* Set data pointer to rest of buffer */
    copySize = descrPtr->frameBuf->actualDataSize -
               (egrBufPtr - devObjPtr->egressBuffer);
    /* Copy rest of frame to egress buffer */
    MEM_APPEND(egrBufPtr, dataPtr, copySize);

    if (descrPtr->mplsCmd == SKERNEL_MPLS_CMD_POP1_LABLE_E ||
        descrPtr->mplsCmd == SKERNEL_MPLS_CMD_POP2_LABLE_E ||
        descrPtr->mplsCmd == SKERNEL_MPLS_CMD_POP3_LABLE_E)
    {
        /* Set pointer to beginning of the IP header */
        ipHeaderPtr = dataPtr;
        calcCheckSum = GT_FALSE;
        if (descrPtr->copyTtl && descrPtr->nlpAboveMpls == NHLFE_NLP_IPV4_E)
        {
            /*  Change TTL */
            ipHeaderPtr[8] = descrPtr->ttl - descrPtr->decTtl;
            calcCheckSum = GT_TRUE;
        }
        if (descrPtr->modifyDscpOrExp)
        {
            /* Change DSCP */
            ipHeaderPtr[1] = descrPtr->dscp << 2;
            calcCheckSum = GT_TRUE;
        }
        if (calcCheckSum == GT_TRUE)
        {
            ipHeaderPtr[10] = 0;
            ipHeaderPtr[11] = 0;
            /* Calculate IP packet checksum */
            ipCheckSum = ipV4CheckSumCalc(ipHeaderPtr,
                                         (GT_U16)((ipHeaderPtr[0] & 0xF) * 4));

            /* Checksum of the IP header and IP options */
            ipHeaderPtr[10] = (GT_U8)(ipCheckSum >> 8);
            ipHeaderPtr[11] = (GT_U8)(ipCheckSum & 0xFF);
        }
    }

    return (egrBufPtr - devObjPtr->egressBuffer);
}

/*******************************************************************************
*   snetTigerVmPortEgressConfig
*
* DESCRIPTION:
*       Convert virtual port to cascade port
* INPUTS:
*       devObjPtr   - pointer to device object
*       port        - VM port
*       tagged      - VM target port tagged or untagged

* OUTPUTS:
*       egrConfigPtr - pointer to VM port egress configuration
*
* RETURN:
*
*
*******************************************************************************/
GT_VOID snetTigerVmPortEgressConfig
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN GT_U32 port,
    IN GT_U32  tagged,
    OUT TIGER_VM_PORT_EGRESS_STC * egrConfigPtr
)
{
    TIGER_EGR_VM_TO_ACTUAL_PORT(devObjPtr, port,
                                egrConfigPtr->actualPort);

    TIGER_EGR_VM_DX_TRANS_PORT(devObjPtr, port,
                                egrConfigPtr->dxPort);

    TIGER_EGR_VM_CSCD_MAP_PORT(devObjPtr, port,
                                egrConfigPtr->addDsaTag);

    /* Target port tagged/untagged */
    egrConfigPtr->dxPortTagged = tagged;
}
/*******************************************************************************
*   snetTigerEgressBuildDsaTag
*
* DESCRIPTION:
*       Supply DSA tag info according to DX destination device
* INPUTS:
*       devObjPtr   - pointer to device object
*       descrPtr    - pointer to packet descriptor
*       egrConfigPtr - pointer to VM port egress info
*       mrvlTagCmd - DSA tag command
* OUTPUTS:
*       mrvlTag - DSA tag
*
* RETURN:
*
*
*******************************************************************************/
GT_VOID snetTigerEgressBuildDsaTag
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN TIGER_VM_PORT_EGRESS_STC * egrConfigPtr,
    IN SKERNEL_MTAG_CMD_ENT mrvlTagCmd,
    OUT GT_U32 *mrvlTag
)
{
    DECLARE_FUNC_NAME(snetTigerEgressBuildDsaTag);


    GT_U32 fldOffset;               /* Register field's offset */
    GT_U32 fldValue;                /* Register field's value */
    GT_U8 autoDsaTag;               /* Enable egress tag the packet with DSA tag
                                       for target device/port is configured as DSA enabled. */

    /* Control frame with CPU formatted DSA tag */
    __LOG(("Control frame with CPU formatted DSA tag"));
    if (descrPtr->inputIncapsulation == GT_CONTROL_PCKT)
    {
        return;
    }

    mrvlTag[0] = 0;

    if (descrPtr->extendTxPktPtr)
    {
        /* Packet from CPU with extended TX info */
        autoDsaTag = (GT_U8)SNET_EXT_BUF_FIELD(descrPtr->extendTxPktPtr, 73, 1);

        /* No tag. Packet is destine to DSA disabled device */
        if (autoDsaTag == 0)
        {
            return;
        }
    }

    /* Non-cascade port */
    if (egrConfigPtr->addDsaTag == 0)
    {
        return;
    }

    /* DSA tag command */
    SMEM_U32_SET_FIELD(mrvlTag[0], 30, 2, mrvlTagCmd);

    /* Packet is sent via network port tagged/untagged */
    SMEM_U32_SET_FIELD(mrvlTag[0], 29, 1, egrConfigPtr->dxPortTagged);

    /* Target DX port */
    SMEM_U32_SET_FIELD(mrvlTag[0], 19, 5, egrConfigPtr->dxPort);

    /* 802.1 User Priority field assigned to the packet */
    SMEM_U32_SET_FIELD(mrvlTag[0], 13, 3, descrPtr->userPriorityTag);

    /* The VID assigned to the packet */
    SMEM_U32_SET_FIELD(mrvlTag[0], 0, 12, descrPtr->vid);


    if (mrvlTagCmd == SKERNEL_MTAG_CMD_FORWARD_E)
    {
        /* <TC <n> Prio> field from the DSA Tag Priority Mapping Register */
        fldOffset = descrPtr->trafficClass * 2;
        smemRegFldGet(devObjPtr, DSA_TAG_PRI_MAP_REG, fldOffset, 2, &fldValue);

        /* The packets Traffic Class (TC) */
        SMEM_U32_SET_FIELD(mrvlTag[0], 16, 2, fldValue);


    }
    else
    if (mrvlTagCmd == SKERNEL_MTAG_CMD_TO_TRG_SNIFFER_E)
    {
        if (descrPtr->rxSniffed == 0)
        {
            /* Set SRC_DEV to 1 on transmit */
            SMEM_U32_SET_FIELD(mrvlTag[0], 24, 5, 1);
            /* Set as Rx sniff (=1) on transmit. */
            SMEM_U32_SET_FIELD(mrvlTag[0], 18, 1, 1);
        }
    }

    mrvlTag[0] = SGT_LIB_SWAP_BYTES_AND_WORDS_MAC((mrvlTag[0]));
}

/*******************************************************************************
* snetTigerDsaEgressBuffer
*
* DESCRIPTION:
*       Fill egress buffer in the descriptor included DSA tag
*
* INPUTS:
*       devObjPtr -  pointer to device object.
*       descrPtr  -  pointer to the frame's descriptor.
*       tagged    -  Tagged/untagged frame
*       mrvlTag   -  marvell tag data
*
* OUTPUTS:
*
*
*
* RETURNS:
*       Length of the filled buffer
*
*
* COMMENTS:
*       None.
*
*******************************************************************************/
GT_U32 snetTigerDsaEgressBuffer
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U32  tagged,
    IN GT_U32 mrvlTag
)
{
    GT_U8_PTR   egrBufPtr;
    GT_U8_PTR   dataPtr;
    GT_U32      egressBufLen, copySize;
    SBUF_BUF_ID frameBuf;

    /* Egress buffer pointer to copy to */
    egrBufPtr = devObjPtr->egressBuffer;
    /* Actual data pointer to copy from */
    frameBuf = descrPtr->frameBuf;
    dataPtr = frameBuf->actualDataPtr;
    copySize = 2 * SGT_MAC_ADDR_BYTES;
    /* Append DA and SA to the egress buffer */
    MEM_APPEND(egrBufPtr, dataPtr, copySize);
    dataPtr += copySize;

    copySize = sizeof(mrvlTag);
    /* Append DSA tag to the egress buffer */
    MEM_APPEND(egrBufPtr, (GT_U8*)&mrvlTag, copySize);
    dataPtr += copySize;

    if(descrPtr->frameBuf->actualDataSize < (GT_U32)(dataPtr - frameBuf->actualDataPtr))
        return 0;

    copySize = descrPtr->frameBuf->actualDataSize - (dataPtr - frameBuf->actualDataPtr);

    /* Copy the rest of the frame data */
    MEM_APPEND(egrBufPtr, dataPtr, copySize);

    if(descrPtr->modifyDscpOrExp == GT_TRUE)
    {
        /* we need to modify the IP/MPLS header */
        snetTwistModifyDscpOrExp(devObjPtr->egressBuffer, descrPtr, tagged);
    }

    egressBufLen = egrBufPtr - devObjPtr->egressBuffer;

    return egressBufLen;
}

/*******************************************************************************
*   snetTigerDsaHa
*
* DESCRIPTION:
*        Make header alteration procedure for Tiger DSA tagged frame
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
*        trgPort   - number of egress port.
*        mrvlTag   - DSA tag data
*
*
*******************************************************************************/
static GT_U32 snetTigerDsaHa
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U32 trgPort,
    IN GT_U32 mrvlTag
)
{
    DECLARE_FUNC_NAME(snetTigerDsaHa);

    GT_U32 egressBufLen, copySize;
    GT_U32 regAddress = 0, fldValue;
    GT_U32 macEntryIdx;
    GT_U32 ipCheckSum;
    GT_U8 lsbFromVlan;
    SGT_MAC_ADDR_TYP macAddr;
    SNET_TWIST_MAC_TBL_STC twistMacEntryPtr;
    GT_U8 portSaLsb, saLsb, vlanSa;
    GT_U8_PTR ipHeaderPtr, egrBufPtr, dataPtr, dataPtrActual;

    /* Egress buffer pointer to copy to */
    egrBufPtr = devObjPtr->egressBuffer;

    if (descrPtr->mll > 0)
    {
        dataPtr = DST_MAC_FROM_DSCR(descrPtr);
    }
    else
    {
        macEntryIdx = descrPtr->arpPointer;
        regAddress = MAC_TAB_ENTRY_WORD0_REG + macEntryIdx * 0x10;
        snetTwistGetMacEntry(devObjPtr, regAddress, &twistMacEntryPtr);
        dataPtr = twistMacEntryPtr.macAddr.bytes;
    }
    dataPtrActual = descrPtr->frameBuf->actualDataPtr;
    /* Copy Destination MAC address */
    __LOG(("Copy Destination MAC address"));
    MEM_APPEND(egrBufPtr, dataPtr, SGT_MAC_ADDR_BYTES);

    smemRegGet(devObjPtr, SRC_ADDR_HIGH_REG, &fldValue);
    /* SAHi[47:16] */
    macAddr.bytes[0] = (GT_U8)(fldValue >> 24);
    macAddr.bytes[1] = (GT_U8)((fldValue >> 16) & 0xFF);
    macAddr.bytes[2] = (GT_U8)((fldValue >>  8) & 0xFF);
    macAddr.bytes[3] = (GT_U8)(fldValue & 0xFF);

    smemRegFldGet(devObjPtr, SRC_ADDR_MID_REG, 0, 8, &fldValue);
    /* SAMid[15:8] */
    macAddr.bytes[4] = (GT_U8)(fldValue & 0xFF);

    smemRegFldGet(devObjPtr, SRC_ADDR_MID_REG, 8, 1, &fldValue);
    /* SAMode */
    lsbFromVlan = (GT_U8)fldValue;

    MAC_CONTROL_REG(devObjPtr->portsNumber, trgPort, &regAddress);
    smemRegFldGet(devObjPtr, regAddress, 9, 8, &fldValue);
    /* SALow[7:0] */
    portSaLsb = (GT_U8)fldValue;

    if (lsbFromVlan == 1)
    {
        /* Ports Vlan table */
        PORTS_VLAN_TABLE_REG(descrPtr->vid, 0, &regAddress);
        smemRegFldGet(devObjPtr, regAddress, 5, 6, &fldValue);
        /* Vlan_Mac_Sa */
        vlanSa = (GT_U8)fldValue;
        saLsb = (portSaLsb & 0xC0) | (vlanSa & 0x3F);
    }
    else
    {
        saLsb = portSaLsb;
    }
    macAddr.bytes[5] = saLsb;
    dataPtr = macAddr.bytes;
    /* Copy Source MAC address */
    MEM_APPEND(egrBufPtr, dataPtr, SGT_MAC_ADDR_BYTES);
    /* Set data pointer to the first byte following Dst and Src MACs */
    dataPtr = dataPtrActual + 2 * SGT_MAC_ADDR_BYTES;

    copySize = sizeof(mrvlTag);
    /* Copy Marvell tag */
    MEM_APPEND(egrBufPtr, (GT_U8 *)&mrvlTag, copySize);
    dataPtr += copySize;

    /* Set data pointer to rest of buffer */
    copySize = descrPtr->frameBuf->actualDataSize -
               (dataPtr - dataPtrActual);

    if (descrPtr->ipHeaderPtr) {
        ipHeaderPtr = descrPtr->ipHeaderPtr;
        /* A timer field (TTL) */
        ipHeaderPtr[8] = descrPtr->ttl - descrPtr->decTtl;
        if (descrPtr->modifyDscpOrExp)
        {
            ipHeaderPtr[1] = descrPtr->dscp << 2;
        }
        ipHeaderPtr[10] = 0;
        ipHeaderPtr[11] = 0;
        /* Calculate IP packet checksum */
        ipCheckSum = ipV4CheckSumCalc(ipHeaderPtr,
                                     (GT_U16)
                                     ((ipHeaderPtr[0] & 0xF) * 4));

        /* Checksum of the IP header and IP options */
        ipHeaderPtr[10] = (GT_U8)(ipCheckSum >> 8);
        ipHeaderPtr[11] = (GT_U8)(ipCheckSum & 0xFF);
    }

    /* Copy rest of frame to egress buffer */
    MEM_APPEND(egrBufPtr, dataPtr, copySize);

    egressBufLen = egrBufPtr - devObjPtr->egressBuffer;

    return egressBufLen;
}

