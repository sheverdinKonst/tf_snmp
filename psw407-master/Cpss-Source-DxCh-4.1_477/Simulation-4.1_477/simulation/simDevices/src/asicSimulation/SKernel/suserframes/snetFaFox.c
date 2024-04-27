/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetFaFox.c
*
* DESCRIPTION:
*       This is a external API definition for fabric adapter frame processing.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 18 $
*
*******************************************************************************/
#include <os/simTypes.h>
#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SKernel/suserframes/snetFaFox.h>
#include <asicSimulation/SKernel/smem/smemFaFox.h>
#include <asicSimulation/SLog/simLog.h>


static void snetFaFoxUcForward
(
    IN SKERNEL_DEVICE_OBJECT      * devObjPtr,
    IN SKERNEL_UPLINK_DESC_STC    * descrPtr
);
static void snetFaFoxMcForward
(
    IN SKERNEL_DEVICE_OBJECT      * devObjPtr,
    IN SKERNEL_UPLINK_DESC_STC    * descrPtr
);
static GT_STATUS snetFaFoxVoqAssignProcess
(
    IN SKERNEL_DEVICE_OBJECT      * devObjPtr,
    IN SKERNEL_UPLINK_DESC_STC    * descrPtr
);
static void snetFaFoxDropPacket
(
    IN SKERNEL_DEVICE_OBJECT      * devObjPtr,
    IN SKERNEL_UPLINK_DESC_STC    * descrPtr
);
static GT_U32 snetFaFoxFillEgressBuffer
(
    IN SKERNEL_DEVICE_OBJECT      * devObjPtr,
    IN SKERNEL_UPLINK_DESC_STC    * descrPtr
);
static void snetFaFoxFrameParsing
(
    IN SKERNEL_DEVICE_OBJECT      * deviceObjPtr,
    INOUT SKERNEL_UPLINK_DESC_STC * descrPtr
);
static void snetFaSendCpuMailToCpu
(
    IN SKERNEL_DEVICE_OBJECT      * devObjPtr,
    IN GT_U8                      * dataPtr,
    IN GT_U32                       srcPort
);
static void snetFaSendPCSPingToCpu
(
    IN SKERNEL_DEVICE_OBJECT      * devObjPtr,
    IN GT_U8                      * dataPtr,
    IN GT_U32                       srcPort
);
static void snetFaSendMessageToCpu
(
    IN SKERNEL_DEVICE_OBJECT      * devObjPtr,
    IN SBUF_BUF_ID                  bufferId,
    IN GT_U32                       srcPort
);


static GT_U32 ingPortOffset[] = {0, 0x8000, 0x10000, 0x100000, 0x108000};

#define SNET_FAFOX_INTERNAL_FPORT_CNS     4

/*******************************************************************************
*   snetFaFoxProcessInit
*
* DESCRIPTION:
*       Init module.
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
void snetFaFoxProcessInit
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    devObjPtr->descriptorPtr =
        (void *)calloc(1, sizeof(SKERNEL_UPLINK_DESC_STC));

    if (devObjPtr->descriptorPtr == 0)
    {
        skernelFatalError("smemFaFoxInit: allocation error\n");
    }

    devObjPtr->devFrameProcFuncPtr = snetFaFoxProcessFrameFromSLAN;
}

/*******************************************************************************
*   snetFaFoxProcessFrameFromSLAN
*
* DESCRIPTION:
*       Process the frame, get and do actions for a frame in FA
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       bufferId     - frame data buffer Id
*       srcPort      - source port number
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*      The frame can be traffic or message from remote CPU.
*
*******************************************************************************/
void snetFaFoxProcessFrameFromSLAN
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SBUF_BUF_ID bufferId,
    IN GT_U32 srcPort
)
{
    SKERNEL_UPLINK_DESC_STC    uplinkDescr, *uplinkDescr_PTR=&uplinkDescr;

    ASSERT_PTR(devObjPtr);

    uplinkDescr_PTR = (SKERNEL_UPLINK_DESC_STC *)devObjPtr->descriptorPtr;
    memset(uplinkDescr_PTR, 0, sizeof(SKERNEL_UPLINK_DESC_STC));

    uplinkDescr_PTR->data.PpPacket.source.frameBuf = bufferId;
    uplinkDescr_PTR->data.PpPacket.source.srcData  = srcPort;
    uplinkDescr_PTR->data.PpPacket.source.ingressFport =  srcPort;

    /* Parsing the frame, get information from frame and fill descriptor */
    snetFaFoxFrameParsing(devObjPtr, uplinkDescr_PTR);

    if (uplinkDescr_PTR->type == SKERNEL_UPLINK_FRAME_FA_TYPE_E)/* pcs ping or cpu MB*/
    {
        snetFaSendMessageToCpu(devObjPtr,bufferId,srcPort);
        return ;
    }

    switch (uplinkDescr_PTR->data.PpPacket.macDaType)
    {
        case SKERNEL_UNICAST_MAC_E: /* unicast packet */
            snetFaFoxUcForward(devObjPtr, uplinkDescr_PTR);
            break;
        case SKERNEL_MULTICAST_MAC_E: /* multicast or broadcast packet */
        case SKERNEL_BROADCAST_MAC_E:
            snetFaFoxMcForward(devObjPtr, uplinkDescr_PTR);
            break;
    }
}


/*******************************************************************************
*   snetFaFoxUcForward
*
* DESCRIPTION:
*       Process unicast frame , the frame can be received from uplink and
*       from the slan
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - pointer to the frame's descriptor.
*
* OUTPUT:
*
* RETURNS:
*
* COMMENT:
*
*******************************************************************************/
static void snetFaFoxUcForward
(
    IN SKERNEL_DEVICE_OBJECT      * devObjPtr,
    IN SKERNEL_UPLINK_DESC_STC    * descrPtr
)
{
    DECLARE_FUNC_NAME(snetFaFoxUcForward);

    GT_U32  LBH;            /* LBH from packet */
    GT_U32  fldValue;       /* value of register*/
    GT_U32  baseAddr;       /* address of register */
    GT_U32  egressBufLen;   /* buffer of packet to be sent*/
    GT_U32  tarDev,tarPort; /* values of target device and port */

    /* save the LBH number */
    LBH    = descrPtr->data.PpPacket.dest.targetInfo.lbh;
    /* save the target device number */
    tarDev = descrPtr->data.PpPacket.dest.targetInfo.targetDevice;

    /* find port bitmap in the unicast routing table *(0x42004000,0x4200C000)*/
    /* the result is the row of the table , take a look in foxXbarGunitRegsInit()*/
    baseAddr = FAFOX_TARGET_DEV_FABRIC_PORT_CONFIG_REG_CNS + \
               ingPortOffset[descrPtr->data.PpPacket.source.ingressFport];
    /* find the column of the table by the LBH and the target device */
    /* take a look in the function of xbarSetUcRoute */
    baseAddr = baseAddr + FAFOX_PORT_INDEX_ENTRY_NUM_CNS * LBH;
    baseAddr = baseAddr + (( tarDev / 4) << 2);

    /* Read the target port . no loop is needed because it is not bitmap */
    smemRegFldGet(devObjPtr ,baseAddr ,(tarDev % 4) * 5 ,5 ,&tarPort);

    /* check the enable bit */
    if ((tarPort & 0x1) == 0)
    {
        snetFaFoxDropPacket(devObjPtr,descrPtr);
        return ;
    }
    tarPort >>= 1; /* remove the enable bit*/

    /* check if the egress port is enabled */
    baseAddr = FAFOX_GLOBAL_CONTROL_REGISTER_CNS + ingPortOffset[tarPort];
    smemRegFldGet(devObjPtr,baseAddr,0,1,&fldValue);
    if (fldValue == 0)
    {
        snetFaFoxDropPacket(devObjPtr,descrPtr);
        return ;
    }

    /* check if the local switching is enabled ?*/
    if (tarPort == descrPtr->data.PpPacket.source.ingressFport)
    {
        baseAddr = FAFOX_LOCAL_SWITCH_REG_CNS + ingPortOffset[tarPort];
        smemRegFldGet( devObjPtr, baseAddr + 0x4C , 1, 1, &fldValue);
        if (fldValue==0)
           return;
    }

    /* check if the target port is 4 , i.e. local pp ??*/
    if (tarPort==SNET_FAFOX_INTERNAL_FPORT_CNS)
    {
        smainSwitchFabricFrame2UplinkSend(devObjPtr,descrPtr);
        return;
    }

    /* fill egress buffer  - send packet to slan */
    __LOG(("fill egress buffer  - send packet to slan"));
    egressBufLen = snetFaFoxFillEgressBuffer(devObjPtr,descrPtr);
    if (egressBufLen)
    {
        smainFrame2PortSend(devObjPtr,
                           tarPort,
                           devObjPtr->egressBuffer,
                           egressBufLen,
                           GT_FALSE);
    }
}

/*******************************************************************************
*   snetFaFoxMcForward
*
* DESCRIPTION:
*       Process multicast frame . The frame can be received from uplink or from
*       slan.
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - pointer to the frame's descriptor.
*
* OUTPUT:
*
* RETURN:
*
* COMMENT:
*   send the mc packets according to the mc routing table.
*******************************************************************************/
static void snetFaFoxMcForward
(
    IN SKERNEL_DEVICE_OBJECT      * devObjPtr,
    IN SKERNEL_UPLINK_DESC_STC    * descrPtr
)
{
    DECLARE_FUNC_NAME(snetFaFoxMcForward);

    GT_U32 fldValue,fldVal;/* fa register value */
    GT_U32 baseAddr;/* fa register address */
    GT_U32 basePort;/* address of fport register configuration */
    GT_U32 mcID,vid;/* multicast id */
    GT_U32 egressBufLen; /* buffer for the egress packet */
    GT_U32 portEnValue,xbarfPortInx;

    if (descrPtr->data.PpPacket.useVidx)
    {
        vid = descrPtr->data.PpPacket.dest.vidx;
    }
    else
    {
        vid = descrPtr->data.PpPacket.source.vid;
    }
    mcID = ( vid > 0xFFF) ? vid - 0xFFF : vid;
    baseAddr = FAFOX_MULTI_GROUP_CONFIG_REG_CNS + (mcID / 4) * 0x10;
    /* read the target fports and send the packet is enabled */
    smemRegFldGet(devObjPtr,baseAddr,(mcID % 4) * 5,5,&fldValue);

    /* loop on all the fports . in contrary to the unicast it is port bitmap */
    for (xbarfPortInx=0;xbarfPortInx<=4;++xbarfPortInx)
    {
        if ( (1 << xbarfPortInx) & fldValue )
        {
            /* check if the target port is enabled */
            __LOG(("check if the target port is enabled"));
            basePort = FAFOX_GLOBAL_CONTROL_REGISTER_CNS + ingPortOffset[xbarfPortInx];
            smemRegFldGet(devObjPtr,basePort,0,1,&portEnValue);
            if (portEnValue)
            {
                /* check if the local switching is enabled ?*/
                __LOG(("check if the local switching is enabled ?"));
                if (xbarfPortInx == descrPtr->data.PpPacket.source.ingressFport)
                {
                    baseAddr = FAFOX_LOCAL_SWITCH_REG_CNS + ingPortOffset[xbarfPortInx];
                    smemRegFldGet( devObjPtr, baseAddr + 0x4C , 1, 1, &fldVal);
                    if (fldVal==0)
                        continue;
                }

                /* check if the target port is 4 , i.e. local pp ??*/
                if (xbarfPortInx==SNET_FAFOX_INTERNAL_FPORT_CNS)
                {
                    smainSwitchFabricFrame2UplinkSend(devObjPtr,descrPtr);
                    continue;
                }

                /* fill egress buffer */
                __LOG(("fill egress buffer"));
                egressBufLen = snetFaFoxFillEgressBuffer(devObjPtr,descrPtr);
                if (egressBufLen)
                {
                     smainFrame2PortSend(devObjPtr,
                                         xbarfPortInx,
                                         devObjPtr->egressBuffer,
                                         egressBufLen,
                                         GT_FALSE);
                }
            }
        }
    }
}

/*******************************************************************************
*   snetFaFoxDropPacket
*
* DESCRIPTION:
*       drop packet.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - pointer to the frame's descriptor.
*
* OUTPUT:
*
* COMMENT:
*
*******************************************************************************/
static void snetFaFoxDropPacket
(
    IN SKERNEL_DEVICE_OBJECT      * devObjPtr,
    IN SKERNEL_UPLINK_DESC_STC    * descrPtr
)
{
     return ;
}

/*******************************************************************************
*   snetFaFoxVoqAssignProcess
*
* DESCRIPTION:
*       assign voq to the device.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - pointer to the frame's descriptor.
* OUTPUT:
*
* RETURNS:
*       GT_OK - succeeded to find a voq.
*       GT_FAIL - failure
*
* COMMENT:
*       device id to Fabric Port Mapping
*******************************************************************************/
static GT_STATUS snetFaFoxVoqAssignProcess
(
    IN SKERNEL_DEVICE_OBJECT      * devObjPtr,
    IN SKERNEL_UPLINK_DESC_STC    * descrPtr
)
{
    DECLARE_FUNC_NAME(snetFaFoxVoqAssignProcess);

    GT_U32 voqRegValue;
    GT_U32 fldValue;/* register field value */
    GT_U32 PortVOQ,tarDev;
    GT_U32 baseAddr;/* register address */
    GT_U32 LBH;

    /* check the voq enable bit */
    smemRegFldGet(devObjPtr,FAFOX_VOQ_GLOB_CONFIG_REG_CNS,20,1,&fldValue);
    /* assign lbh number for VOQ assignment . if lbhEnable bit is 0 , LBH=0 */
    LBH = (fldValue == 0) ? 0 : descrPtr->data.PpPacket.dest.targetInfo.lbh;

    /* get the mapping of Device ID to Fabric Port */
    /* ignore the set parameters bits , which are not relevant for the simulation */
    tarDev   = descrPtr->data.PpPacket.dest.targetInfo.targetDevice;
    baseAddr = FAFOX_DEVID_FA_MAP_REG_CNS + 4 * tarDev;
    smemRegFldGet( devObjPtr, baseAddr, 8 * LBH, 6, &voqRegValue);

    /* check if the target device voq is enabled */
    baseAddr = FAFOX_DEVICE_ENABLE_REGISTER_CNS + 4 * (tarDev / 32 );
    smemRegFldGet( devObjPtr, baseAddr, tarDev % 32, 1, &fldValue);
    if (fldValue==0)
    {
        /* update counters */
        __LOG(("update counters"));
        smemRegGet(devObjPtr,FSFOX_DEV_ENABLE_DRP_COUNTER_REG,&fldValue);
        fldValue++;
        smemRegSet(devObjPtr,FSFOX_DEV_ENABLE_DRP_COUNTER_REG,fldValue);
        return GT_FAIL;
    }

    /* check if the target port (for the above voq mapping) is enabled */
    PortVOQ = (voqRegValue & 0x3F);
    /* check if the target port voq is enabled */
    smemRegFldGet(devObjPtr,FAFOX_FABRIC_PORT_ENABLE_REG_CNS + 4 * (PortVOQ / 32),
                  PortVOQ % 32,1,&fldValue);
    if (fldValue==0)
    {
        return GT_FAIL;
    }

    return GT_OK;
}

/*******************************************************************************
*   snetFaFoxProcessFrameFromUplink
*
* DESCRIPTION:
*       Process the frame, get and do actions for a frame
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - pointer to the frame's descriptor.
*
* OUTPUT:
*
* RETURNS:
*       GT_OK - succeeded to find a voq.
*       GT_FAIL - failure
*
* COMMENT:
*
*******************************************************************************/
void snetFaFoxProcessFrameFromUplink
(
    IN SKERNEL_DEVICE_OBJECT      * devObjPtr,
    IN SKERNEL_UPLINK_DESC_STC    * descrPtr
)
{
    DECLARE_FUNC_NAME(snetFaFoxProcessFrameFromUplink);

    /* process frame from uplink */
    GT_U32      fldValue=0;

    ASSERT_PTR(devObjPtr);
    ASSERT_PTR(descrPtr);

    /* check enable of device */
    smemRegFldGet(devObjPtr,FAFOX_GLOB_CONGIF_REG_CNS,0,1,&fldValue);
    if (fldValue==0)
    {
        return;
    }

    /* mc , bc or unknown unicast frame routing algorithm */
    __LOG(("mc , bc or unknown unicast frame routing algorithm"));
    if((descrPtr->data.PpPacket.macDaType != (GT_MAC_TYPE)SKERNEL_UNICAST_MAC_E) ||
       (descrPtr->data.PpPacket.useVidx))
    {
        smemRegFldGet(devObjPtr,FAFOX_VOQ_GLOB_CONFIG_REG_CNS,27,1,&fldValue);
        if (fldValue!=0)
        {
            snetFaFoxMcForward(devObjPtr,descrPtr);
        }
    }
    /* uc frame routing algorithm */
    else if (snetFaFoxVoqAssignProcess(devObjPtr,descrPtr)==GT_OK)
    {
        snetFaFoxUcForward(devObjPtr,descrPtr);
    }
}

/*******************************************************************************
* snetFaFoxFillEgressBuffer
*
* DESCRIPTION:
*       Fill egress buffer in the descriptor
*
* INPUTS:
*        devObjPtr   -  pointer to device object.
*        descrPtr  -    pointer to the frame's descriptor.
*
* OUTPUTS:
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
static GT_U32 snetFaFoxFillEgressBuffer
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_UPLINK_DESC_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetFaFoxFillEgressBuffer);

    GT_U8_PTR   egrBufPtr;
    GT_U8_PTR   dataPtr;
    GT_U32      egressBufLen, copySize;
    SBUF_BUF_ID frameBuf;

    /* Egress buffer pointer to copy to */
    egrBufPtr = devObjPtr->egressBuffer;
    /* Actual data pointer to copy from */
    frameBuf = descrPtr->data.PpPacket.source.frameBuf;
    dataPtr = frameBuf->actualDataPtr;
    copySize = sizeof(SKERNEL_UPLINK_DESC_STC);
    /* Append uplink descriptor to the egress buffer . No tagging anyway*/
    MEM_APPEND(egrBufPtr, dataPtr, copySize);
    dataPtr = frameBuf->actualDataPtr;
    if (frameBuf->actualDataSize < (GT_U32)(dataPtr - frameBuf->actualDataPtr))
        return 0;
    copySize = frameBuf->actualDataSize - (dataPtr - frameBuf->actualDataPtr);

    /* Copy the frame data */
    __LOG(("Copy the frame data"));
    MEM_APPEND(egrBufPtr, dataPtr, copySize);
    egressBufLen = egrBufPtr - devObjPtr->egressBuffer;

    return egressBufLen;
}

/*******************************************************************************
*   snetFaFoxFrameParsing
*
* DESCRIPTION:
*       Parsing the frame, get information from frame and fill descriptor
*
* INPUTS:
*       devObjPtr - pointer to device object.
*       descrPtr    - pointer to the frame's descriptor.
*
* OUTPUTS:
*
*       descrPtr
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void snetFaFoxFrameParsing
(
    IN SKERNEL_DEVICE_OBJECT        *     deviceObjPtr,
    INOUT SKERNEL_UPLINK_DESC_STC   *     descrPtr
)
{
    SBUF_BUF_STC   * frameBufPtr;
    GT_U8          * dataPtr; /* pointer to the start of buffer's data */
    GT_U32           dataSize; /* size of buffer's data */

    frameBufPtr = descrPtr->data.PpPacket.source.frameBuf;

    sbufDataGet(frameBufPtr,&dataPtr,&dataSize);
    /* the simulation needs to know if this is a traffic or message to cpu    *
     * if it is message to cpu then the first byte is "SMAIN_SRC_TYPE_CPU_E" ,*
     * take a look in the functions of smemFaActiveWritePingMessage           */
    if (*dataPtr==SMAIN_SRC_TYPE_CPU_E)
    {
      descrPtr->type = SKERNEL_UPLINK_FRAME_FA_TYPE_E;
    }
    else
    {
      memcpy(descrPtr,frameBufPtr->actualDataPtr,sizeof(SKERNEL_UPLINK_DESC_STC));
      descrPtr->data.PpPacket.source.srcData  = frameBufPtr->srcData;
      descrPtr->data.PpPacket.source.ingressFport =  frameBufPtr->srcData;
    }
}


/*******************************************************************************
*   snetFoxCpuMessageProcess
*
* DESCRIPTION:
*          send message of mailbox or pcs ping from cpu to the slan
*
* INPUTS:
*       devObjPtr - pointer to device object.
*       descrPtr     - pointer to the frame's descriptor.
*
* OUTPUTS:
*
*       descrPtr
* RETURNS:
*
* COMMENTS:
*        This messages are sent according to the targetpotBMP that was set on
*        the functions of smemFaActiveWriteTxdControl and
*        smemFaActiveWritePingMessage.
*******************************************************************************/
void snetFoxCpuMessageProcess
(
    IN SKERNEL_DEVICE_OBJECT      *devObjPtr,
    IN GT_U8                      *dataPtr
)
{
    DECLARE_FUNC_NAME(snetFoxCpuMessageProcess);

    GT_U32  targetFportBMP;
    GT_U32  baseAddr;       /* address of register */
    GT_U32  fldValue;       /* value of register*/
    GT_U32 dataSize,msgLen=0;      /* fix number for the pcs ping message */
    GT_U8 * egrBuff;
    GT_U8 ii;


    /* Actual data pointer to copy from copy */
    egrBuff = devObjPtr->egressBuffer;

    /* copy the type of message(MB or ping) and source of message(CPU) */
    MEM_APPEND(egrBuff , dataPtr , 2);
    msgLen+=2;
    dataPtr+=2;

    /* find which is the target port */
    targetFportBMP = *dataPtr;
    MEM_APPEND( egrBuff, dataPtr , 1);
    msgLen+=1;
    dataPtr+=1;

    /* find the data length in words and copy it ! */
    dataSize = *dataPtr;
    MEM_APPEND(egrBuff, dataPtr, 1);
    dataPtr+=1;

    /* find the data itself */
    msgLen+=(dataSize+1)*sizeof(GT_U32);
    MEM_APPEND(egrBuff, dataPtr, dataSize*sizeof(GT_U32));

    /* check enable of device */
    smemRegFldGet(devObjPtr,FAFOX_GLOB_CONGIF_REG_CNS,0,1,&fldValue);
    if (fldValue==0)
    {
        return;
    }
    for (ii = 0; ii <= 4; ii++)
    {
       /* target port is always bmp */
       if ((1 << ii) & (targetFportBMP))
       {
            /* check enable of port */
            __LOG(("check enable of port"));
            baseAddr = FAFOX_GLOBAL_CONTROL_REGISTER_CNS + ingPortOffset[ii];
            smemRegFldGet(devObjPtr,baseAddr,0,1,&fldValue);
            if (fldValue==0)
            {
                return ;
            }
            /* send the cpu mail box or ping to the slan */
            __LOG(("send the cpu mail box or ping to the slan"));
            smainFrame2PortSend(devObjPtr,
                                ii,
                                devObjPtr->egressBuffer,
                                msgLen,
                                GT_FALSE);
       }
    }

    return ;
}

/*******************************************************************************
*   snetFaSendMessageToCpu
*
* DESCRIPTION:
*          invoke interrupt of ping or mailbox that was received from slan
*
* INPUTS:
*       devObjPtr - pointer to device object.
*       descrPtr     - pointer to the frame's descriptor.
*
* OUTPUTS:
*
*       descrPtr
* RETURNS:
*
* COMMENTS:
*        inform CPU for messaged that was received from remote CPU.
*
*******************************************************************************/
static void snetFaSendMessageToCpu
(
    IN SKERNEL_DEVICE_OBJECT      * devObjPtr,
    IN SBUF_BUF_ID                  bufferId,
    IN GT_U32                       srcPort
)
{
    GT_U8          * dataPtr; /* pointer to the start of buffer's data */
    GT_U32           dataSize; /* size of buffer's data */

    sbufDataGet(bufferId,&dataPtr,&dataSize);
    dataPtr++;

    /* in this function , the simulation knows that it is a ping or cpu       *
     * so what the decision if it is MB or Ping depends on the second byte of *
     * the message.Take a look in the function of smemFaActiveWritePingMessage*/
    if (*dataPtr == SMAIN_CPU_MAILBOX_MSG_E)
    {
        snetFaSendCpuMailToCpu(devObjPtr,dataPtr,srcPort);
    }
    else
    {
        snetFaSendPCSPingToCpu(devObjPtr,dataPtr,srcPort);
    }
}

/*******************************************************************************
*   snetFaSendCpuMailToCpu
*
* DESCRIPTION:
*       Process cpu mailbox message from slan to the cpu.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - pointer to the frame's descriptor.
*
* OUTPUT:
*
* RETURNS:
*
* COMMENT:
*
*******************************************************************************/
static void snetFaSendCpuMailToCpu
(
    IN SKERNEL_DEVICE_OBJECT      * devObjPtr,
    IN GT_U8                      * dataPtr,
    IN GT_U32                       srcPort
)
{
    DECLARE_FUNC_NAME(snetFaSendCpuMailToCpu);

    GT_U32 fldValue;
    GT_U8  setIntr=0;
    GT_U32 msgLen;
    FA_DEV_MEM_INFO  * devMemInfoPtr;

    /* get the target port bmp , nothing to do with it */
    dataPtr++;

    /* get the length of the message in word !*/
    dataPtr++;
    msgLen= *dataPtr;

    /* get the message data */
    dataPtr++;
    devMemInfoPtr = (FA_DEV_MEM_INFO  *)devObjPtr->deviceMemory;
    memcpy(devMemInfoPtr->RcvCpuMailMsg.CPUMailMsgData ,dataPtr,msgLen*sizeof(GT_U32));

    /* saved data on the mailbox received message register */
    devMemInfoPtr->RcvCpuMailMsg.CPUMailMsgCounter = 0;
    smemRegSet(devObjPtr,
               FA_CPU_MAIL_READ_REG,
               devMemInfoPtr->RcvCpuMailMsg.CPUMailMsgData[0]);
    /* update mailbox counter */
    smemRegGet(devObjPtr,FAFOX_MAIL_RCV_COUNTER_REG,&fldValue);
    fldValue++;
    smemRegSet(devObjPtr,FAFOX_MAIL_RCV_COUNTER_REG,fldValue);

    /* check if the interrupt can be invoked */
    smemRegFldGet(devObjPtr, FA_REASSEMBLY_CAUSE_INTER_MASK_REG, 1, 1, &fldValue);
    /* received mailbox interrupt */
    if (fldValue)
    {
        smemRegFldSet(devObjPtr, FA_REASSEMBLY_CAUSE_INTER_REG, 0, 2, 3);
        setIntr = 1;
    }
    /* Call interrupt */
    if (setIntr == 1)
    {
        /* Global Interrupt Mask Register */
        smemRegFldGet(devObjPtr, FA_GLOBAL_INT_MASK_REG, 6, 1, &fldValue);
        /* crx mask*/
        if (fldValue)
        {
            smemRegFldSet(devObjPtr, FA_GLOBAL_INT_CAUSE_REG, 6, 1, 1);
            smemRegFldSet(devObjPtr, FA_GLOBAL_INT_CAUSE_REG, 0, 1, 1);
            scibSetInterrupt(devObjPtr->deviceId);
        }
    }
    else/* dropped mailbox message */
    {
        /* update dropped mailbox counter */
        __LOG(("update dropped mailbox counter"));
        /* the mailbox message is dropped because , no interrupt in invoked */
        smemRegGet(devObjPtr,FAFOX_MAIL_DRP_COUNTER_REG,&fldValue);
        fldValue++;
        smemRegSet(devObjPtr,FAFOX_MAIL_DRP_COUNTER_REG,fldValue);
    }
}

/*******************************************************************************
*   snetFaSendPCSPingToCpu
*
* DESCRIPTION:
*       Process pcs ping message from slan to the cpu.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - pointer to the frame's descriptor.
*
* OUTPUT:
*
* RETURNS:
*
* COMMENT:
*
*******************************************************************************/
static void snetFaSendPCSPingToCpu
(
    IN SKERNEL_DEVICE_OBJECT      * devObjPtr,
    IN GT_U8                      * dataPtr,
    IN GT_U32                       srcPort
)
{
    DECLARE_FUNC_NAME(snetFaSendPCSPingToCpu);

    GT_U32 fldValue,regValue;
    GT_U8  setIntr=0;
    GT_U32 baseAddr,lData;

    /* get the received fport , which has no meaningful here */
    dataPtr++;

    dataPtr++; /* message length which is always 1 is ping messages */
    /* get the message data , ping is received on 0x4218001C register*/
    dataPtr++;
    baseAddr = FA_PING_CELL_RX_REG + ingPortOffset[srcPort];
    memcpy(&lData,dataPtr,sizeof(GT_U32));
    lData = 0x00FFFFFF & lData;/*pcs ping is only 24 bits , not 32 */
    smemRegSet(devObjPtr,baseAddr,lData);

    /* check the interrupt mask */
    baseAddr = FA_HYPERGLINK_INT_MASK_REG + ingPortOffset[srcPort];
    smemRegFldGet(devObjPtr, FA_HYPERGLINK_INT_MASK_REG, 14, 1, &fldValue);
    /* received pcs ping */
    __LOG(("received pcs ping"));
    if (fldValue)
    {
        regValue = (1 << 14) | 1;
        baseAddr = FA_HYPERGLINK_INT_CAUSE_REG + ingPortOffset[srcPort];
        smemRegSet(devObjPtr,FA_HYPERGLINK_INT_CAUSE_REG,regValue);
        setIntr = 1;
    }
    /* Call interrupt */
    if (setIntr == 1)
    {
        /* check the  Global Interrupt Mask Register */
        smemRegFldGet(devObjPtr, FA_GLOBAL_INT_MASK_REG, 9+srcPort, 1, &fldValue);
        /* Interrupt can be invoked */
        if (fldValue)
        {
            smemRegFldSet(devObjPtr, FA_GLOBAL_INT_CAUSE_REG, 9+srcPort, 1, 1);
            smemRegFldSet(devObjPtr, FA_GLOBAL_INT_CAUSE_REG,  0, 1, 1);
            scibSetInterrupt(devObjPtr->deviceId);
        }
    }
}
