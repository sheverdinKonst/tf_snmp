/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* sstackSalsa.c
*
* DESCRIPTION:
*       The memory module handles frames from Stack/Cascade ports.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 6 $
*
*******************************************************************************/
#include <asicSimulation/SKernel/sstack/sstackSalsa.h>
#include <asicSimulation/SKernel/suserframes/snet.h>
#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SKernel/smem/smemSalsa.h>
#include <asicSimulation/SKernel/suserframes/snetSalsa.h>
#include <asicSimulation/SKernel/salsaCommon/sregSalsa.h>

static void sstackSalsaFrameFromCpu
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr
);

static void sstackSalsaFrame2Cpu
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr
);
static void sstackSalsaTx2Port
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U32 egressPort,
    IN GT_U32 tagged
);

static void sstackGetMarvellTag
(
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN MRVL_TAG * mrvlTagPtr
);

static void sstackSalsaMcastFromCpu
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U32 vidx,
    IN GT_U32 vid
);

static void sstackSalsaSniffedFrame
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr
);

static void sstackSalsaFrwdFrame
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr
);
/*******************************************************************************
*   sstackSalsaFrameProcess
*
* DESCRIPTION:
*       Process frame from CPU or cascading port
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - pointer to the frame's descriptor.
* OUTPUT:
*       descrPtr     - pointer to the frame's descriptor.
*       GT_BOOL      - if TRUE continue processing, if FALSE - stop processing
*
*******************************************************************************/
GT_BOOL sstackSalsaFrameProcess
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    MRVL_TAG  mrvlTag;

    sstackGetMarvellTag(descrPtr, &mrvlTag);

    if (mrvlTag.tagCommand != TAG_CMD_FROM_CPU_E) {
        if (mrvlTag.srcDevice == descrPtr->srcDevice) {
            return GT_FALSE;
        }
    }
    if (mrvlTag.tagCommand == TAG_CMD_TO_CPU_E) {
        sstackSalsaFrame2Cpu(devObjPtr, descrPtr);
        return GT_FALSE;
    }
    else
    if (mrvlTag.tagCommand == TAG_CMD_FROM_CPU_E) {
        sstackSalsaFrameFromCpu(devObjPtr, descrPtr);
        return GT_FALSE;
    }
    else
    if (mrvlTag.tagCommand == TAG_CMD_TO_TARGET_SNIFFER_E) {
        sstackSalsaSniffedFrame(devObjPtr, descrPtr);
        return GT_FALSE;
    }
    else
        sstackSalsaFrwdFrame(devObjPtr, descrPtr);

    return GT_TRUE;
}
/*******************************************************************************
*   sstackSalsaFrame2Cpu
*
* DESCRIPTION:
*       Process frame with target is CPU
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - pointer to the frame's descriptor.
* OUTPUT:
*
*
*******************************************************************************/
static void sstackSalsaFrame2Cpu
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    GT_U32 cpuPort;
    GT_U32 fldValue;

    smemRegFldGet(devObjPtr, TRANS_QUEUE_CONTROL_REG, 4, 5, &fldValue);
    /* CPU PortNum */
    cpuPort = fldValue;

    sstackSalsaTx2Port(devObjPtr, descrPtr, cpuPort, 1);

}
/*******************************************************************************
*   sstackSalsaTx2Port
*
* DESCRIPTION:
*       Forward cascading frame to egress port
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - pointer to the frame's descriptor.
*       egressPort -   number of egress port.
*       tagged  -      send frame with tag.
* OUTPUT:
*
*
*
*******************************************************************************/
static void sstackSalsaTx2Port
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U32 egressPort,
    IN GT_U32 tagged
)
{
    GT_U32 regAddress;
    GT_U32 fldValue;
    GT_U32 cascadField;
    GT_U32 frameLength;
    GT_U8 * frameBuf;

    ASSERT_PTR(devObjPtr);
    ASSERT_PTR(descrPtr);

    if (egressPort > 24) {
        regAddress = 0x000000A0;
    }
    else {
        MAC_CONTROL_REG(egressPort, &regAddress);
    }
    smemRegFldGet(devObjPtr, regAddress, 0, 1, &fldValue);
    /* Mac control PortEn register field */
    if (fldValue == 0)
        return;

    smemRegFldGet(devObjPtr, regAddress, 12, 1, &fldValue);
    /* Mac control CascadingPort register field */
    cascadField = fldValue;
    if (cascadField == 1) {
        frameBuf = descrPtr->dstMacPtr;
        frameLength = descrPtr->byteCount;
    }
    else
    if (tagged) {
        frameBuf = devObjPtr->egressBuffer;
        memcpy(frameBuf, descrPtr->dstMacPtr, descrPtr->byteCount);
        frameBuf[12] = 0x81;
        frameBuf[13] = 0x00;
        frameLength = descrPtr->byteCount;
    }
    else {
        frameBuf = devObjPtr->egressBuffer;
        memcpy(frameBuf, descrPtr->dstMacPtr, 12);
        memcpy(frameBuf + 12, descrPtr->dstMacPtr + 16,
            descrPtr->byteCount - 16);
        frameLength = descrPtr->byteCount - 4;
    }
    snetSalsaTxMirror(devObjPtr, descrPtr, frameBuf, frameLength, egressPort);

    snetSalsaTxMacCountUpdate(devObjPtr, descrPtr, egressPort);

    smainFrame2PortSend(devObjPtr, egressPort, frameBuf, frameLength,GT_FALSE);
}

/*******************************************************************************
*   sstackSalsaFrameFromCpu
*
* DESCRIPTION:
*       Process frame from CPU
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - pointer to the frame's descriptor.
* OUTPUT:
*
*
*
*******************************************************************************/
static void sstackSalsaFrameFromCpu
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    GT_U32 egressPort;
    GT_U32 fldValue;
    GT_U32 regAddress;
    GT_U32 fldFirstBit;
    MRVL_TAG mrvlTag;

    sstackGetMarvellTag(descrPtr, &mrvlTag);

    if (mrvlTag.vidx == 0) {
        if (mrvlTag.trgDev == descrPtr->srcDevice) {
            /* Trg Port */
            egressPort = mrvlTag.trgPort;
        }
        else {
            TRG_DEV_CASCADE_PORT_REG(mrvlTag.trgDev, &regAddress);
            fldFirstBit = CASCADE_PORT(mrvlTag.trgDev);
            smemRegFldGet(devObjPtr, regAddress, fldFirstBit, 5, &fldValue);
            /* The local port through which packets destined to device are to be
               transmitted */
            egressPort = fldValue;
        }
        sstackSalsaTx2Port(devObjPtr, descrPtr, egressPort, mrvlTag.dstTagged);
        return;
    }

    sstackSalsaMcastFromCpu(devObjPtr, descrPtr, mrvlTag.vidx, mrvlTag.vid);
}

/*******************************************************************************
*   sstackSalsaMcastFromCpu
*
* DESCRIPTION:
*       Process multicast frame from CPU.
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - pointer to the frame's descriptor.
*       vidx         - vidx according to which the packet is to be forwarded
*       vid          - vid of the pkt
* OUTPUT:
*
*
*
*******************************************************************************/
static void sstackSalsaMcastFromCpu
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U32 vidx,
    IN GT_U32 vid
)
{
    GT_U8 vlanPorts[32];
    GT_U8 taggedPorts[32];
    GT_U8 vidxPorts[32];
    GT_U8 stpPorts[32];
    GT_U32 tagged;
    GT_U32 port;
    GT_U32 regAddress;
    GT_U32 fldValue;
    GT_U8 hashVal;
    GT_U32 mcastTrunkBmp;

    snetSalsaVlanPortsGet(devObjPtr, vid, vlanPorts, taggedPorts, stpPorts);

    if (vidx > 255 && vidx < 511) {
        /* multicast frame */
        snetSalsaVidxPortsGet(devObjPtr, vidx, vidxPorts);
        for (port = 0; port < 32; port++) {
            vlanPorts[port] &= vidxPorts[port];
        }
    }


    hashVal = 0;
    TRUNK_DESIGN_PORTS_HASH_REG(hashVal, &regAddress);
    smemRegFldGet(devObjPtr, regAddress, 0, 23, &fldValue);
    /* DesPortTrunk */
    mcastTrunkBmp = fldValue;
    for (port = 0; port < 24; port++) {
        if (vlanPorts[port] == 0)
            continue;
        /* Bridge Port<n> Control Register */
        BRDG_PORT0_CTRL_REG(port, &regAddress);
        smemRegFldGet(devObjPtr, regAddress, 0, 5, &fldValue);
        /* TrunkGroupId */
        if (fldValue == 1) {
            if (((mcastTrunkBmp >> port) & 0x01) == 0) {
                vlanPorts[port] = 0;
            }
        }
    }
    for (port = 0; port < 31; port++) {
        if (vlanPorts[port] == 1) {
            tagged = taggedPorts[port];
            sstackSalsaTx2Port(devObjPtr, descrPtr, port, tagged);
        }
    }

}
/*******************************************************************************
*   sstackSalsaSniffedFrame
*
* DESCRIPTION:
*       Process sniffed frame from cascading port
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - pointer to the frame's descriptor.
*
* OUTPUTS:
*
*
*******************************************************************************/
static void sstackSalsaSniffedFrame
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    MRVL_TAG mrvlTag;
    GT_U32 egressPort;
    GT_U32 egressDev;
    GT_U32 fldValue;
    GT_U32 regAddress;
    GT_U32 fldFirstBit;

    ASSERT_PTR(devObjPtr);
    ASSERT_PTR(descrPtr);

    sstackGetMarvellTag(descrPtr, &mrvlTag);
    if (mrvlTag.rxSniff == 1) {
        smemRegFldGet(devObjPtr, INGR_MIRR_MONITOR_REG, 3, 5, &fldValue);
        /* The device number on which the Rx monitor resides */
        egressDev = fldValue;
        smemRegFldGet(devObjPtr, INGR_MIRR_MONITOR_REG, 11, 5, &fldValue);
        /* The port number on which the Rx monitor resides */
        egressPort = fldValue;
    }
    else {
        smemRegFldGet(devObjPtr, TRANSMIT_SNIF_REG, 0, 5, &fldValue);
        /* Indicates the device number of the destination Tx sniffer */
        egressDev = fldValue;
        smemRegFldGet(devObjPtr, TRANSMIT_SNIF_REG, 7, 5, &fldValue);
        /* Indicates the port number of the destination Tx sniffer */
        egressPort = fldValue;
    }
    if (egressDev != descrPtr->srcDevice) {
        TRG_DEV_CASCADE_PORT_REG(mrvlTag.trgDev, &regAddress);
        fldFirstBit = CASCADE_PORT(mrvlTag.trgDev);
        smemRegFldGet(devObjPtr, regAddress, fldFirstBit, 5, &fldValue);
        /* The local port through which packets destined to device are to be
        transmitted */
        egressPort = fldValue;
    }

    sstackSalsaTx2Port(devObjPtr, descrPtr, egressPort, mrvlTag.srcTagged);
}

/*******************************************************************************
*   sstackSalsaFrwdFrame
*
* DESCRIPTION:
*       Process forward frame from cascading port
*
* INPUTS:
*       descrPtr  - description pointer
*       descrPtr     - pointer to the frame's descriptor.
*
* OUTPUTS:
*
*
*******************************************************************************/
static void sstackSalsaFrwdFrame
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    MRVL_TAG mrvlTag;

    ASSERT_PTR(devObjPtr);
    ASSERT_PTR(descrPtr);

    sstackGetMarvellTag(descrPtr, &mrvlTag);
    if (mrvlTag.srcTrunk) {
        descrPtr->srcTrunkId = mrvlTag.srcTrunk;
    }

    descrPtr->dstMacPtr[12] = 0x81;
    descrPtr->dstMacPtr[13] = 0x00;
}

/*******************************************************************************
*   sstackGetMarvellTag
*
* DESCRIPTION:
*       Get Marvell tag from descriptor
*
* INPUTS:
*       descrPtr  - description pointer
*
* OUTPUTS:
*       Returns 32 bits Marvell TAG
*
*******************************************************************************/
static void sstackGetMarvellTag
(
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN MRVL_TAG * mrvlTagPtr
)
{
    GT_U32 mrvlTag = 0;

    ASSERT_PTR(descrPtr);
    ASSERT_PTR(mrvlTagPtr);

    memset(mrvlTagPtr, 0, sizeof(MRVL_TAG));

    mrvlTag = descrPtr->dstMacPtr[12] << 24 | descrPtr->dstMacPtr[13] << 16 |
              descrPtr->dstMacPtr[14] << 8 | descrPtr->dstMacPtr[15];
    /* Tag_Command */
    mrvlTagPtr->tagCommand = (GT_U16)(mrvlTag >> 30) & 0x03;

    if (mrvlTagPtr->tagCommand == TAG_CMD_FORWARD_E) {
        /* Src_Is_Trunk */
        mrvlTagPtr->srcIsTrunk = (GT_U16)(mrvlTag >> 18) & 0x01;
        if (mrvlTagPtr->srcIsTrunk == 1) {
            /* Src_Port */
            mrvlTagPtr->srcTrunk = (GT_U16)(mrvlTag >> 19) & 0x1F;
        }
        else
        if (mrvlTagPtr->srcIsTrunk == 0) {
            /* Src_Trunk */
            mrvlTagPtr->srcPort = (GT_U16)(mrvlTag >> 19) & 0x1F;
        }
    }
    else
    if (mrvlTagPtr->tagCommand == TAG_CMD_TO_TARGET_SNIFFER_E) {
        /* rx_sniff */
        mrvlTagPtr->rxSniff = (GT_U16)(mrvlTag >> 18) & 0x01;
    }

    if (mrvlTagPtr->tagCommand == TAG_CMD_FROM_CPU_E) {
        /* use_vidx */
        mrvlTagPtr->useVidx = (GT_U16)(mrvlTag >> 18) & 0x01;
        if (mrvlTagPtr->useVidx == 0)
        {
            /* Trg Dev */
            mrvlTagPtr->trgDev = (GT_U16)(mrvlTag >> 24) & 0x1F;
            /* Trg Port */
            mrvlTagPtr->trgPort = (GT_U16)(mrvlTag >> 19) & 0x1F;
        }
        else
        {
            mrvlTagPtr->vidx = (GT_U16)(mrvlTag >> 20) & 0x1FF;
        }

        /* prio */
        mrvlTagPtr->prio = (GT_U16)(mrvlTag >> 16) & 0x03;

        /* Dst_Tagged */
        mrvlTagPtr->dstTagged = (GT_U16)(mrvlTag >> 29) & 0x01;
    }
    else
    if (mrvlTagPtr->tagCommand == TAG_CMD_TO_CPU_E) {
        /* CPU_Code */
        mrvlTagPtr->cpuCode = (GT_U16)(mrvlTag >> 16) & 0x0F;

        /* Src_Tagged */
        mrvlTagPtr->srcTagged = (GT_U16)(mrvlTag >> 29) & 0x01;
    }

    if (mrvlTagPtr->tagCommand == TAG_CMD_TO_CPU_E ||
        mrvlTagPtr->tagCommand == TAG_CMD_TO_TARGET_SNIFFER_E) {
        /* Src_Port */
        mrvlTagPtr->srcPort = (GT_U16)(mrvlTag >> 19) & 0x1F;
    }
    if (mrvlTagPtr->tagCommand == TAG_CMD_FROM_CPU_E ||
        mrvlTagPtr->tagCommand == TAG_CMD_TO_TARGET_SNIFFER_E ||
        mrvlTagPtr->tagCommand == TAG_CMD_FORWARD_E) {
        /* Src Dev */
        mrvlTagPtr->srcDevice = (GT_U16)(mrvlTag >> 24) & 0x1F;
    }

    /* The pkts VPT */
    mrvlTagPtr->vpt = (GT_U16)(mrvlTag >> 13) & 0x7;
    /* The vid of the pkt */
    mrvlTagPtr->vid = (GT_U16)mrvlTag & 0xfff;
}

