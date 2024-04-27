/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetCapuera.c
*
* DESCRIPTION:
*       This is a external API definition for xabr capoeira frame processing.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 6 $
*
*******************************************************************************/
#include <os/simTypes.h>
#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SKernel/suserframes/snetCapuera.h>
#include <asicSimulation/SKernel/smem/smemCapuera.h>
#include <asicSimulation/SLog/simLog.h>


static void snetCapoeiraUcForward
(
    IN SKERNEL_DEVICE_OBJECT      * devObjPtr,
    IN SKERNEL_UPLINK_DESC_STC    * descrPtr
);
static void snetCapoeiraMcForward
(
    IN SKERNEL_DEVICE_OBJECT      * devObjPtr,
    IN SKERNEL_UPLINK_DESC_STC    * descrPtr
);
static void snetCapoeiraDropPacket
(
    IN SKERNEL_DEVICE_OBJECT      * devObjPtr,
    IN SKERNEL_UPLINK_DESC_STC    * descrPtr
);
static GT_U32 snetCapoeiraFillEgressBuffer
(
    IN SKERNEL_DEVICE_OBJECT      * devObjPtr,
    IN SKERNEL_UPLINK_DESC_STC    * descrPtr
);
static void snetCapoeiraFrameParsing
(
    IN SKERNEL_DEVICE_OBJECT      * deviceObjPtr,
    INOUT SKERNEL_UPLINK_DESC_STC * descrPtr
);
#if 0
static void snetCapoeiraSendPCSPingToCpu
(
    IN SKERNEL_DEVICE_OBJECT      * devObjPtr,
    IN GT_U8                      * dataPtr,
    IN GT_U32                       srcPort
);
static void snetCapoeiraSendMessageToCpu
(
    IN SKERNEL_DEVICE_OBJECT      * devObjPtr,
    IN SBUF_BUF_ID                  bufferId,
    IN GT_U32                       srcPort
);
static void snetCapoeiraSendCpuMailToCpu
(
    IN SKERNEL_DEVICE_OBJECT      * devObjPtr,
    IN GT_U8                      * dataPtr,
    IN GT_U32                       srcPort
);
#endif


/* macros for converion beween port number to specific address */
#define SNET_CAP_TRG_DEV_2_FPORT_CONFIG_BASE(n , k)  (0x00800000 + ((n)%4)*   \
                                                      0x00100000 + ((n)/4)*   \
                                                      0x00008000+0x00004000+  \
                                                      (k)*0x4)

#define SNET_CAP_PING_CELL_RX(n)                     (0x00880000 + ((n)%4)*   \
                                                      0x00100000 + ((n)/4)*   \
                                                      0x00008000 + 0x1C)

#define SNET_CAP_HG_MAIN_INT_CAUSE_REG(n)            (0x00880000 + ((n)%4)*   \
                                                      0x00100000 + ((n)/4)*   \
                                                      0x00008000 +  0x8)

#define SNET_CAP_HG_MAIN_INT_MASK_REG(n)             (0x00880000 + ((n)%4)*   \
                                                      0x00100000 + ((n)/4)*   \
                                                      0x00008000 + 0xC)


/*******************************************************************************
*   snetCapoeiraProcessInit
*
* DESCRIPTION:
*       Init module.
*
* INPUTS:
*       deviceObjPtr   - pointer to device object.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void snetCapoeiraProcessInit
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    devObjPtr->descriptorPtr =
        (void *)calloc(1, sizeof(SKERNEL_UPLINK_DESC_STC));

    if (devObjPtr->descriptorPtr == 0)
    {
        skernelFatalError("snetCapoeiraProcessInit: allocation error\n");
    }

    devObjPtr->devFrameProcFuncPtr = snetCapoeiraProcessFrameFromSLAN;
}

/*******************************************************************************
*   snetCapoeiraProcessFrameFromSLAN
*
* DESCRIPTION:
*       Process the frame, get and do actions for a frame in FA
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       bufferId     - frame data buffer Id
*       srcPort      - sourcu port number
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*      The frame can be traffic or message from remote CPU.
*******************************************************************************/
void snetCapoeiraProcessFrameFromSLAN
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SBUF_BUF_ID bufferId,
    IN GT_U32 srcPort
)
{
    GT_U32                     fldValue;
    SKERNEL_UPLINK_DESC_STC    uplinkDescr, *uplinkDescr_PTR=&uplinkDescr;

    ASSERT_PTR(devObjPtr);

    /* check device state */
    smemRegFldGet(devObjPtr,CAP_GLOB_CONGIF_REG_CNS,0,1,&fldValue);
    if (fldValue==0)
    {
        return;
    }

    uplinkDescr_PTR = (SKERNEL_UPLINK_DESC_STC *)devObjPtr->descriptorPtr;
    memset(uplinkDescr_PTR, 0, sizeof(SKERNEL_UPLINK_DESC_STC));

    uplinkDescr_PTR->data.PpPacket.source.frameBuf     = bufferId;
    uplinkDescr_PTR->data.PpPacket.source.srcData      = srcPort;
    uplinkDescr_PTR->data.PpPacket.source.ingressFport =  srcPort;

    /* Parsing the frame, get information from frame and fill descriptor */
    snetCapoeiraFrameParsing(devObjPtr, uplinkDescr_PTR);

    switch (uplinkDescr_PTR->data.PpPacket.macDaType)
    {
      case SKERNEL_UNICAST_MAC_E: /* unicast packet */
          snetCapoeiraUcForward(devObjPtr, uplinkDescr_PTR);
          break;
      case SKERNEL_MULTICAST_MAC_E: /* multicast or broadcast packet */
      case SKERNEL_BROADCAST_MAC_E:
          snetCapoeiraMcForward(devObjPtr, uplinkDescr_PTR);
          break;
    }
}


/*******************************************************************************
*   snetCapoeiraUcForward
*
* DESCRIPTION:
*       Process unicast frame , the frame is received from slan
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
static void snetCapoeiraUcForward
(
    IN SKERNEL_DEVICE_OBJECT      * devObjPtr,
    IN SKERNEL_UPLINK_DESC_STC    * descrPtr
)
{
    DECLARE_FUNC_NAME(snetCapoeiraUcForward);

    GT_U32  fldValue,fldVal;       /* vlaue of register*/
    GT_U32  baseAddr;       /* address of register */
    GT_U32  egressBufLen;   /* buffer of packet to be sent*/
    GT_U32  tarDev,tarPort; /* vlaues of target device and port */

    /* save the target device number */
    __LOG(("save the target device number"));
    tarDev = descrPtr->data.PpPacket.dest.targetInfo.targetDevice;
    baseAddr = CAP_GLOB_CONGIF_REG_CNS ;
    smemRegFldGet(devObjPtr ,baseAddr ,1 ,1 ,&fldVal);

    /* find port bitmap in the unicast routing table *(0x00804000,0x00904000)*/
    /*  take a look in capXAbarControl.c::xbarSetUcRoute()                   */
    baseAddr = SNET_CAP_TRG_DEV_2_FPORT_CONFIG_BASE(
                                   tarDev,
                                   descrPtr->data.PpPacket.source.ingressFport);

    /* Read the target port . no loop is needed because it is not bitmap */
    __LOG(("Read the target port . no loop is needed because it is not bitmap"));
    smemRegFldGet(devObjPtr ,baseAddr ,(tarDev % 4) * 5 ,5 ,&tarPort);

    /* check the enable bit */
    __LOG(("check the enable bit"));
    if ((tarPort & 0x1) == 0)
    {
        snetCapoeiraDropPacket(devObjPtr,descrPtr);
        return ;
    }
    tarPort >>= 1; /* remove the enable bit*/

    /* check if the egress port is enabled */
    __LOG(("check if the egress port is enabled"));
    baseAddr = CAP_PORT_ENABLE_REGISTER_CNS;
    smemRegFldGet(devObjPtr,baseAddr,tarPort,1,&fldValue);
    if (fldValue==0)
    {
        snetCapoeiraDropPacket(devObjPtr,descrPtr);
        return ;
    }

    /* fill egress buffer - send packet to slan */
    __LOG(("fill egress buffer - send packet to slan"));
    egressBufLen = snetCapoeiraFillEgressBuffer(devObjPtr,descrPtr);
    if (egressBufLen)
    {
        smainFrame2PortSend(devObjPtr,tarPort,
                            devObjPtr->egressBuffer,
                            egressBufLen,
                            GT_FALSE);
    }
}

/*******************************************************************************
*   snetCapoeiraMcForward
*
* DESCRIPTION:
*       Process multicast frame . The frame can be recieved from uplink or from
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
*
*******************************************************************************/
static void snetCapoeiraMcForward
(
    IN SKERNEL_DEVICE_OBJECT      * devObjPtr,
    IN SKERNEL_UPLINK_DESC_STC    * descrPtr
)
{
    DECLARE_FUNC_NAME(snetCapoeiraMcForward);

    GT_U32 fldValue,fldVal;/* fa register vlaue */
    GT_U32 baseAddr;/* fa register address */
    GT_U32 mcID,vid;/* multicast id */
    GT_U32 egressBufLen; /* buffer for the egress packet */
    GT_U32 xbarfPortInx;

    if (descrPtr->data.PpPacket.useVidx)
    {
        vid = descrPtr->data.PpPacket.dest.vidx;
    }
    else
    {
        vid = descrPtr->data.PpPacket.source.vid;
    }
    mcID = ( vid > 0xFFF) ? vid - 0xFFF : vid;
    /* note : i preferred to take the first mc group (PG0) although , there *
     *        is no differenct between the four groups .                    */
    baseAddr = CAP_PG0_MC_TABLE_CNS ;
    baseAddr = baseAddr + (mcID / 2) * 8;
    /*  Read the target fports and send the packet ,     *
     *  see capXAbarControl.c::xbarSetMcRoute()          */
    if ((mcID & 0x1) == 0)
    {
        smemRegFldGet(devObjPtr,baseAddr,0,12,&fldValue);
    }
    else
    {
        smemRegFldGet(devObjPtr,baseAddr,13,12,&fldValue);
    }

    /* loop on all the fports . in contrary to the unicast it is port bitmap */
    __LOG(("loop on all the fports . in contrary to the unicast it is port bitmap"));
    for (xbarfPortInx=0;xbarfPortInx<=devObjPtr->portsNumber;++xbarfPortInx)
    {
        if ( (1 << xbarfPortInx) & fldValue )
        {
            /* check if the target port is enabled */
            __LOG(("check if the target port is enabled"));
            baseAddr = CAP_PORT_ENABLE_REGISTER_CNS;
            smemRegFldGet(devObjPtr,baseAddr,xbarfPortInx,1,&fldVal);
            if (fldVal==0)
            {
                snetCapoeiraDropPacket(devObjPtr,descrPtr);
                continue ;
            }

            /* fill egress buffer */
            __LOG(("fill egress buffer"));
            egressBufLen = snetCapoeiraFillEgressBuffer(devObjPtr,descrPtr);
            if (egressBufLen)
            {
               smainFrame2PortSend(devObjPtr,xbarfPortInx,
                                   devObjPtr->egressBuffer,
                                   egressBufLen,
                                   GT_FALSE);
            }
        }
    }
}

/*******************************************************************************
*   snetCapoeiraDropPacket
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
static void snetCapoeiraDropPacket
(
    IN SKERNEL_DEVICE_OBJECT      * devObjPtr,
    IN SKERNEL_UPLINK_DESC_STC    * descrPtr
)
{
     return ;
}


/*******************************************************************************
* snetCapoeiraFillEgressBuffer
*
* DESCRIPTION:
*       Fill egress buffer in the descriptor
*
* INPUTS:
*        deviceObj   -  pointer to device object.
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
static GT_U32 snetCapoeiraFillEgressBuffer
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_UPLINK_DESC_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetCapoeiraFillEgressBuffer);

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
    __LOG(("Append uplink descriptor to the egress buffer . No tagging anyway"));
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
*   snetCapoeiraFrameParsing
*
* DESCRIPTION:
*       Parsing the frame, get information from frame and fill descriptor
*
* INPUTS:
*       deviceObjPtr - pointer to device object.
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
static void snetCapoeiraFrameParsing
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
*   snetCapoeiraCpuMessageProcess
*
* DESCRIPTION:
*     Process message from cpu , it can be pcs ping message or cpu mailbox.
*
* INPUTS:
*       deviceObjPtr - pointer to device object.
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
GT_VOID snetCapoeiraCpuMessageProcess
(
    IN SKERNEL_DEVICE_OBJECT      *deviceObj_PTR,
    IN GT_U8                      *dataPtr
)
{
    GT_U32  targetFportBMP;
    GT_U32  baseAddr;       /* address of register */
    GT_U32  fldValue;       /* vlaue of register*/
    GT_U32 dataSize,msgLen=0;      /* fix number for the pcs ping message */
    GT_U8 * egrBuff;
    GT_U8 ii;


    /* Actual data pointer to copy from */
    egrBuff = deviceObj_PTR->egressBuffer;

    /* copy the type of message(MB or ping) and source of message(CPU) */
    MEM_APPEND(egrBuff , dataPtr , 2);
    msgLen += 2;
    dataPtr += 2;

    /* find which is the target port */
    targetFportBMP = *dataPtr;
    MEM_APPEND( egrBuff, dataPtr , 1);
    msgLen += 1;
    dataPtr += 1;

    /* find the data length in words and copy it ! , in the case of mb  *
     * it is already multiplied by 12                                   */
    dataSize = *dataPtr;
    MEM_APPEND(egrBuff, dataPtr, 1);
    dataPtr += 1;

    /* find the data itself */
    msgLen += (dataSize + 1) * sizeof(GT_U32);
    MEM_APPEND(egrBuff, dataPtr, dataSize*sizeof(GT_U32));

    /* check enable of device */
    smemRegFldGet(deviceObj_PTR,CAP_GLOB_CONGIF_REG_CNS,0,1,&fldValue);
    if (fldValue==0)
    {
        return;
    }
    for (ii = 0; ii <= deviceObj_PTR->portsNumber ; ii++)
    {
       /* target port is always bmp */
       if ((1 << ii) & (targetFportBMP))
       {
            /* check enable of port */
            baseAddr = CAP_PORT_ENABLE_REGISTER_CNS;
            smemRegFldGet(deviceObj_PTR , baseAddr , ii , 1 , &fldValue);
            if (fldValue==0)
            {
                return ;
            }
            /* send the cpu mail box or ping to the slan */
            smainFrame2PortSend(deviceObj_PTR ,
                                ii ,
                                deviceObj_PTR->egressBuffer ,
                                msgLen,
                                GT_FALSE);
        }
    }

    return ;
}

#if 0

/*******************************************************************************
* snetCapoeiraSendMessageToCpu
*
* DESCRIPTION:
*          invoke interrupt of ping or mailbox that was received from slan
*
* INPUTS:
*       deviceObjPtr - pointer to device object.
*       descrPrt     - pointer to the frame's descriptor.
*
* OUTPUTS:
*
*       descrPrt
* RETURNS:
*
* COMMENTS:
*        send CPU for messages that was recived from remote CPU.
*
*******************************************************************************/
static void snetCapoeiraSendMessageToCpu
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
    if (*dataPtr == SMAIN_CPU_PCSPING_MSG_E)
    {
        snetCapoeiraSendPCSPingToCpu(devObjPtr,dataPtr,srcPort);
    }
    else
    {
        snetCapoeiraSendCpuMailToCpu(devObjPtr,dataPtr,srcPort);
    }
}

/*******************************************************************************
*   snetCapoeiraSendPCSPingToCpu
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
static void snetCapoeiraSendPCSPingToCpu
(
    IN SKERNEL_DEVICE_OBJECT      * devObjPtr,
    IN GT_U8                      * dataPtr,
    IN GT_U32                       srcPort
)
{
    GT_U32 fldValue,regValue;
    GT_U8  setIntr=0;
    GT_U32 tarFportBmp;
    GT_U32 baseAddr,lData;
    CAP_XBAR_DEV_MEM_INFO  * devMemInfoPtr;

    /* get the recived fport , which has no meaningfull here */
    dataPtr++;
    tarFportBmp= *dataPtr;

    dataPtr++; /* message length which is always 1 -> ping messages */
    /* get the message data , ping is recived on 0x4218001C regsiter*/
    dataPtr++;
    devMemInfoPtr = (CAP_XBAR_DEV_MEM_INFO *)devObjPtr->deviceMemory;
    baseAddr = SNET_CAP_PING_CELL_RX(srcPort);
    memcpy(&lData,dataPtr,sizeof(GT_U32));
    lData = (0x00FFFFFF & lData) | (1 << 24);/*pcs ping is only 24 bits , not 32 */
    smemRegSet(devObjPtr,baseAddr,lData);

    /* check the interrupt mask */
    baseAddr = SNET_CAP_HG_MAIN_INT_MASK_REG(srcPort);
    smemRegFldGet(devObjPtr, baseAddr, 14, 1, &fldValue);
    /* recieved pcs ping interrupt */
    if (fldValue)
    {
        regValue = (1 << 14) | 1;
        baseAddr = SNET_CAP_HG_MAIN_INT_CAUSE_REG(srcPort);
        smemRegSet(devObjPtr,baseAddr,regValue);
        setIntr = 1;
    }
    /* Call interrupt */
    if (setIntr == 1)
    {
        /* check the global interrupt mask Register */
        smemRegFldGet(devObjPtr, CAP_GLOB_INTERUPT_MASK_REG_CNS, (16+srcPort), 1, &fldValue);
        /* Interrupt can be invoked */
        if (fldValue)
        {
            smemRegFldSet(devObjPtr,CAP_GLOB_INTERUPT_CAUSE_REG_CNS,(16+srcPort),1,1);
            smemRegFldSet(devObjPtr,CAP_GLOB_INTERUPT_CAUSE_REG_CNS,0,1,1);
            scibSetInterrupt(devObjPtr->deviceId);
        }
    }
}


/*******************************************************************************
*   snetCapoeiraSendCpuMailToCpu
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
static void snetCapoeiraSendCpuMailToCpu
(
    IN SKERNEL_DEVICE_OBJECT      * devObjPtr,
    IN GT_U8                      * dataPtr,
    IN GT_U32                       srcPort
)
{
    GT_U32 fldValue;
    GT_U8  setIntr=0;
    GT_U32 tarFportBmp;
    GT_U32 msgLen;
    CAP_XBAR_DEV_MEM_INFO  * devMemInfoPtr;

    /* get the target port bmp , nothing to do with it */
    /* get the recived fport , which has no meaningfull here */
    dataPtr++;
    tarFportBmp= *dataPtr;

    /* get the length of the message in word !*/
    dataPtr++;
    msgLen= *dataPtr;

    /* get the message data */
    dataPtr++;
    devMemInfoPtr = (CAP_XBAR_DEV_MEM_INFO  *)devObjPtr->deviceMemory;
    memcpy(devMemInfoPtr->GlobalMem.globRegs ,dataPtr,msgLen*sizeof(GT_U8));

    /* check if the interrupt can be invoked */
    smemRegFldGet(devObjPtr,CAP_RX_CELL_PEND_INTERUPT_MASK_REG_CNS,
                  srcPort+1,
                  1,
                  &fldValue);
    /* recieved mailbox interrupt */
    if (fldValue)
    {
        smemRegFldSet(devObjPtr,CAP_RX_CELL_PEND_INTERUPT_CAUSE_REG_CNS,
                      srcPort+1,
                      1,
                      1);
        setIntr = 1;
    }
    /* Call interrupt */
    if (setIntr == 1)
    {
        /* check the global interrupt mask Register */
        smemRegFldGet(devObjPtr, CAP_GLOB_INTERUPT_MASK_REG_CNS,6, 1,&fldValue);
        /* Interrupt can be invoked */
        if (fldValue)
        {
            smemRegFldSet(devObjPtr,CAP_GLOB_INTERUPT_CAUSE_REG_CNS,6,1,1);
            smemRegFldSet(devObjPtr,CAP_GLOB_INTERUPT_CAUSE_REG_CNS,0,1,1);
            scibSetInterrupt(devObjPtr->deviceId);
        }
    }
#endif
