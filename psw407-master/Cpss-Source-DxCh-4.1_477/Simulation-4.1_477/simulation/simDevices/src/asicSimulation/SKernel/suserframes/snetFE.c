/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetFE.c
*
* DESCRIPTION:
*       This is a external API definition for snetFE module of Dune Simulation.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 13 $
*
*******************************************************************************/
#include <asicSimulation/SKernel/suserframes/snet.h>
#include <asicSimulation/SKernel/suserframes/snetFe.h>
#include <asicSimulation/SKernel/smem/smemFe.h>
#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SKernel/smem/smem.h>
#include <asicSimulation/SLog/simLog.h>

/*
 * Typedef: struct SFE_UNICAST_DIST_MEM_ENTRY_STC
 *
 * Description:
 *      internal simulated Unicast Distribution Table.
 *
 * Fields:
 *      DeviceIndex           - frame or fdb message
 *      portMap               - frame descriptor
 *      LinkState             - fdb message
 *      Refresh               - 1 if received RM else 0
 *
 * Comments:
 */
typedef struct
{
    GT_U32  deviceIndex;
    GT_U32  portBmp;
    GT_U32  linkState;
    GT_U32  Refresh;
}SNET_FE_UNICAST_DIST_INFO_STC;

SNET_FE_UNICAST_DIST_INFO_STC     feUnicastKeyDistArray
                                    [SMAIN_SWITCH_FABRIC_MAX_NUM_OF_DEVICES_CNS]
                                    [SMAIN_SWITCH_FABRIC_MAX_NUM_OF_LINKS_CNS];

/* Private declarations */
static GT_STATUS snetFeIngress
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_UPLINK_DESC_STC * descrPtr
);

static void snetFeMessageHandlerTask
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
);

static void snetFeSendsRMTask
(
    IN void * devObjPtr
);

static void snetFeSendRmMsg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
);

static void snetFeUpdateUnicastDistTable
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
);

static void snetFeFrameParsing
(
    IN SKERNEL_DEVICE_OBJECT      *     devObjPtr,
    INOUT SKERNEL_UPLINK_DESC_STC *     descrPtr
);

static GT_U32 snetFeTxQPhase1
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_UPLINK_DESC_STC * descrPtr,
    INOUT GT_U8                destPorts[],
    IN GT_U8                    portBmp
);

static void snetFeTx2Port
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_UPLINK_DESC_STC * descrPtr,
    IN GT_U32 egressPort
);

static GT_STATUS snetFeEgressProcess
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_UPLINK_DESC_STC * descrPtr
);

static void snetFeRxMacCounters
(
    IN SKERNEL_DEVICE_OBJECT      *     devObjPtr,
    IN SKERNEL_UPLINK_DESC_STC *        descrPtr
);

static void snetFeTxMacCounters
(
    IN SKERNEL_DEVICE_OBJECT      *     devObjPtr,
    IN SKERNEL_UPLINK_DESC_STC *        descrPtr ,
    IN  GT_U32                          egressPort
);

/*******************************************************************************
*   snetFeProcessInit
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
void snetFeProcessInit
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    GT_U32      ii;
    SNET_FE_UNICAST_DIST_INFO_STC * fieldInfoPtr;

    ASSERT_PTR(devObjPtr);

    devObjPtr->descriptorPtr =
        (void *)calloc(1, sizeof(SKERNEL_UPLINK_DESC_STC));

    if (devObjPtr->descriptorPtr == 0)
    {
        skernelFatalError("snetFeProcessInit: allocation error\n");
    }

    for (ii = 0 ; ii < (2 * SMAIN_SWITCH_FABRIC_1KB) ; ++ii)
    {
        fieldInfoPtr = &feUnicastKeyDistArray[devObjPtr->deviceId][ii];
        fieldInfoPtr->deviceIndex = ii;
        fieldInfoPtr->portBmp     = 0;
        fieldInfoPtr->linkState   = 0;
        fieldInfoPtr->Refresh     = 0;
    }

    devObjPtr->devFrameProcFuncPtr = snetFeFrameProcess;

    snetFeMessageHandlerTask(devObjPtr);
}


/*******************************************************************************
*   snetFeMessageHandlerTask
*
* DESCRIPTION:
*       Initiate Reachability Message task
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
static void snetFeMessageHandlerTask
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    GT_TASK_HANDLE          taskHandl;        /* task handle */
    GT_TASK_PRIORITY_ENT    taskPriority;

    taskPriority = GT_TASK_PRIORITY_HIGHEST;
    taskHandl = SIM_OS_MAC(simOsTaskCreate)(
                    taskPriority,
                    (unsigned (__TASKCONV *)(void*))snetFeSendsRMTask,
                    (void *) devObjPtr);

    if (taskHandl == NULL)
    {
        skernelFatalError(" snetFeMessageHandlerTask : cannot \
                            create message handler task for device %u", \
                            devObjPtr->deviceId);
    }
}

/*******************************************************************************
*   snetFeSendsRMTask
*
* DESCRIPTION:
*        Sends Reachability messages every configurable x milliseconds
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
static void snetFeSendsRMTask
(
    IN void * devObjPtr
)
{
    GT_U32      regAddr;
    GT_U32      data;

    regAddr = FE_ROUT_PROCESS_ENA_REG;
    while(1)
    {
        SIM_OS_MAC(simOsSleep)(SMAIN_SWITCH_FABRIC_CHIP_CLOCK_CNS );

        smemRegFldGet(devObjPtr, regAddr,  8 ,  5 , &data);

        if (data > 4)
        {
            snetFeSendRmMsg(devObjPtr);
        }

        snetFeUpdateUnicastDistTable(devObjPtr);
    }
}


/*******************************************************************************
*   snetFeSendRmMsg
*
* DESCRIPTION:
*        Sends Reachability messages every x milliseconds
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
static void snetFeSendRmMsg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    GT_U32                  ii;
    SKERNEL_UPLINK_DESC_STC   frameDesc;
    SBUF_BUF_ID bufferId;       /* buffer */
    GT_U8_PTR   egrBufPtr;
    GT_U8_PTR   dataPtr;
    GT_U32 dataSize;            /* data size */
    SKERNEL_UPLINK_FRAME_TYPE_ENT  msgType;
    GT_U32 copySize = 0;
    GT_U32 egressBufLen ;

    egrBufPtr = devObjPtr->egressBuffer ;
    msgType = SKERNEL_UPLINK_FRAME_DUNE_RM_E;
    memset(&frameDesc,0,sizeof(SKERNEL_UPLINK_DESC_STC));
    frameDesc.data.PpPacket.source.frameType = msgType;
    frameDesc.data.PpPacket.source.srcDevice = (GT_U8)devObjPtr->deviceId ;
    bufferId = frameDesc.data.PpPacket.source.frameBuf;

    /* allocate buffer for the reachability message */
    bufferId = sbufAlloc(devObjPtr->bufPool, FE_REACHABILITY_MSG_SIZE);
    /* Get actual data pointer */
    sbufDataGet(bufferId, (GT_U8 **)&dataPtr, &dataSize);
    /* copy the type of the message  */
    memcpy(dataPtr, &msgType , sizeof(GT_U32) );
    copySize = sizeof(GT_U8);
    MEM_APPEND(egrBufPtr, dataPtr, copySize);
    dataPtr++;
    /* copy the id/number of the device  */
    memcpy(dataPtr, &devObjPtr->deviceId , sizeof(GT_U32) );
    /* Append buffer to device egress buffer */
    MEM_APPEND(egrBufPtr, dataPtr, copySize);
    egressBufLen = egrBufPtr - devObjPtr->egressBuffer;

    for (ii=0 ; ii < devObjPtr->portsNumber ; ii++)
    {
        smainFrame2PortSend(devObjPtr,
                            ii,
                            (GT_U8 *)&devObjPtr->egressBuffer,
                            egressBufLen,
                            GT_FALSE);
    }
}

/*******************************************************************************
*   snetFeUpdateUnicastDistTable
*
* DESCRIPTION:
*
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
static void snetFeUpdateUnicastDistTable
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    GT_U32                  ii;
    GT_U32                  regAddress;
    GT_U32                  data = 0;
    SNET_FE_UNICAST_DIST_INFO_STC * fieldInfoPtr;

    for (ii=0 ; ii < (2 * SMAIN_SWITCH_FABRIC_1KB) ; ii++)
    {
        fieldInfoPtr = &feUnicastKeyDistArray[devObjPtr->deviceId][ii];
        if ( fieldInfoPtr->Refresh == 0 )
        {
            if ( fieldInfoPtr->linkState == 1 )
            {
                SNET_FE_UNICAST_DIST_TABLE_ENTRY_REG(devObjPtr->deviceId,&regAddress);
                smemMemSet(devObjPtr, regAddress,  &data , sizeof (GT_U32) );
                fieldInfoPtr->linkState = 0;
            }
        }
        else
        {
            fieldInfoPtr->Refresh = 0 ;
        }
    }
}



/*******************************************************************************
*   snetFeFrameProcess
*
* DESCRIPTION:
*       Process the frame, get and do actions for a frame
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       bufferId     - frame data buffer Id
*       srcPort      - source port number
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
void snetFeFrameProcess
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SBUF_BUF_ID bufferId,
    IN GT_U32 srcPort
)
{
    SKERNEL_UPLINK_DESC_STC * descrPtr; /* pointer to the frame's descriptor */

    ASSERT_PTR(devObjPtr);

    descrPtr = (SKERNEL_UPLINK_DESC_STC *)devObjPtr->descriptorPtr;
    memset(descrPtr, 0, sizeof(SKERNEL_UPLINK_DESC_STC));

    descrPtr->data.PpPacket.source.frameBuf = bufferId;
    descrPtr->data.PpPacket.source.srcPort = srcPort;
    descrPtr->data.PpPacket.source.byteCount = (GT_U16)bufferId->actualDataSize;

    /* Parse the frame */
    snetFeFrameParsing(devObjPtr ,descrPtr);

    /* Make routing decision for the incoming frame/message */
    snetFeIngress(devObjPtr ,descrPtr);

    /* Egress processing main routine */
    snetFeEgressProcess(devObjPtr ,descrPtr);
}

/*******************************************************************************
*   snetFeEgressProcess
*
* DESCRIPTION:
*       Routine Process engine.
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - pointer to the descriptor
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
static GT_STATUS snetFeEgressProcess
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_UPLINK_DESC_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetFeEgressProcess);

    GT_U32  regAddr;
    GT_U8   destPorts[FE_PORTS_NUMBER];      /* destination untagged port array */
    GT_U32  outPorts;           /* summery of output ports to send the frame */
    GT_U8  portBmp;
    GT_U32  targetPort;
    GT_U32  portInx = 0;
    SKERNEL_UPLINK_PP_SOURCE_DESCR_STC * uplinkSrcDescrPtr;

    uplinkSrcDescrPtr = &(descrPtr->data.PpPacket.source);

    if (uplinkSrcDescrPtr->frameType != SKERNEL_UPLINK_FRAME_DUNE_RM_E)
    {
        if (descrPtr->data.PpPacket.useVidx)
        {   /* multicast distribution table */
            __LOG(("multicast distribution table"));
            SNET_FE_MULTICAST_DIST_TABLE_ENTRY_REG(uplinkSrcDescrPtr->vid ,
                                                     &regAddr );
        }
        else
        {   /* unicast distribution table */
            __LOG(("unicast distribution table"));
            SNET_FE_UNICAST_DIST_TABLE_ENTRY_REG(uplinkSrcDescrPtr->srcDevice ,
                                                 &regAddr );
        }

        smemRegFldGet(devObjPtr, regAddr, 0, 8, (GT_U32*)&portBmp);
        outPorts = snetFeTxQPhase1(devObjPtr, descrPtr, destPorts, portBmp);
        switch (outPorts)
        {
            case 0:
                break;
            case 1:
                snetFeTx2Port(devObjPtr, descrPtr, destPorts[outPorts]);
                break;
            default:
                while (portInx < FE_PORTS_NUMBER)
                {
                    if (destPorts[portInx])
                    {
                        targetPort = portInx;
                        snetFeTx2Port(devObjPtr,descrPtr,targetPort);
                    }
                    portInx++;
                }/* while */
                break;
        }
    }

    return GT_OK;
}

/*******************************************************************************
*   snetFeIngress
*
* DESCRIPTION:
*       Ingress policy for the fabric element simulated device.
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - pointer to the descriptor
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
static GT_STATUS snetFeIngress
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_UPLINK_DESC_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetFeIngress);

    GT_U32  srcDev;
    GT_U32  srcPort;
    GT_U32  regAddr;
    GT_U32  fldValue=0;
    GT_U32  srcChipId;
    SNET_FE_UNICAST_DIST_INFO_STC * fieldInfoPtr;
    SKERNEL_UPLINK_PP_SOURCE_DESCR_STC * uplinkSrcDescrPtr;

    uplinkSrcDescrPtr = &(descrPtr->data.PpPacket.source);
    srcDev = uplinkSrcDescrPtr->srcDevice;
    srcPort = uplinkSrcDescrPtr->srcPort;
    srcChipId = uplinkSrcDescrPtr->partnerDeviceID;
    if (uplinkSrcDescrPtr->frameType == SKERNEL_UPLINK_FRAME_DUNE_RM_E)
    {
        /* update unicast distribution table */
        __LOG(("update unicast distribution table"));
        SNET_FE_UNICAST_DIST_TABLE_ENTRY_REG( srcDev , &regAddr);
        smemRegFldSet(devObjPtr, regAddr,  1 << srcPort, 1 , 1);

        /* update internal unicast distribution table */
        fieldInfoPtr = &feUnicastKeyDistArray[devObjPtr->deviceId][srcDev];
        fieldInfoPtr->deviceIndex = srcDev;
        fieldInfoPtr->portBmp     |= (1 << srcPort);
        fieldInfoPtr->linkState   = 1;
        fieldInfoPtr->Refresh     = 1;

        /* find connectivity map registers */
        SNET_FE_CONNECTIVITY_MAP_REG( srcPort , &regAddr);

        /* calculate the connectivity entry */
        SMEM_U32_SET_FIELD(fldValue, 14, 5, srcChipId); /* source pp device */
        SMEM_U32_SET_FIELD(fldValue, 11, 3, srcPort)  ; /* source port*/
        SMEM_U32_SET_FIELD(fldValue, 0, 11, srcDev)   ; /* source device */

        /* update connectivity map entry */
        smemRegSet(devObjPtr, regAddr, fldValue);
    }

    /* update rx counters */
    __LOG(("update rx counters"));
    snetFeRxMacCounters(devObjPtr,descrPtr);

    return GT_OK ;
}


/*******************************************************************************
*   snetFeRxMacCounters
*
* DESCRIPTION:
*       Update Rx Mac counters
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - pointer to the descriptor
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*       simulation handles a packet like a cell . This means that can be
*       different counters scores in black mode then in white mode.
*
*******************************************************************************/
static void snetFeRxMacCounters
(
    IN SKERNEL_DEVICE_OBJECT      *     devObjPtr,
    INOUT SKERNEL_UPLINK_DESC_STC *     descrPtr
)
{
    GT_U32  srcPort;
    GT_U32  address;
    GT_U32  fldValue;
    SKERNEL_UPLINK_PP_SOURCE_DESCR_STC * uplinkSrcDescrPtr;

    uplinkSrcDescrPtr = &(descrPtr->data.PpPacket.source);
    srcPort = uplinkSrcDescrPtr->srcPort;

    smemRegFldGet(devObjPtr, FE_MAC_ENABLER_REG, 16, 2, &fldValue);
    if (fldValue != SNET_FE_MAC_COUNT_CTRL_CELL_ONLY_MODE)
    {
        SNET_FE_RX_CELL_COUNTER_REG( srcPort , &address );
        smemRegFldGet(devObjPtr, address, 0, 30, &fldValue);
        smemRegFldSet(devObjPtr, address, 0, 30, ++fldValue);
    }
}

/*******************************************************************************
*   snetFeTxMacCounters
*
* DESCRIPTION:
*       Update Tx Mac counters
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - pointer to the descriptor.
*       egressPort   - tx port
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*       simulation handles a packet like a cell . This means that can be
*       different counters scores in black mode then in white mode.
*
*******************************************************************************/
static void snetFeTxMacCounters
(
    IN SKERNEL_DEVICE_OBJECT      *     devObjPtr,
    IN SKERNEL_UPLINK_DESC_STC *     descrPtr ,
    IN  GT_U32                          egressPort
)
{
    GT_U32  address;
    GT_U32  fldValue;

    smemRegFldGet(devObjPtr, FE_MAC_ENABLER_REG, 18, 2, &fldValue);
    if (fldValue != SNET_FE_MAC_COUNT_CTRL_CELL_ONLY_MODE)
    {
        SNET_FE_TX_CELL_COUNTER_REG( egressPort , &address );
        smemRegFldGet(devObjPtr, address, 0, 30, &fldValue);
        smemRegFldSet(devObjPtr, address, 0, 30, ++fldValue);
    }
}

/*******************************************************************************
*   snetFeFrameParsing
*
* DESCRIPTION:
*       Parse packet.
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - pointer to the descriptor
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
static void snetFeFrameParsing
(
    IN SKERNEL_DEVICE_OBJECT      *     devObjPtr,
    INOUT SKERNEL_UPLINK_DESC_STC *     descrPtr
)
{
    DECLARE_FUNC_NAME(snetFeFrameParsing);

    SBUF_BUF_STC   * frameBufPtr;
    SKERNEL_UPLINK_PP_SOURCE_DESCR_STC  * uplinkSrcDescPtr;

    uplinkSrcDescPtr = &(descrPtr->data.PpPacket.source);
    frameBufPtr = uplinkSrcDescPtr->frameBuf ;

    uplinkSrcDescPtr->frameType = (frameBufPtr->dataType == SMAIN_REACHABILITY_MSG_E) ?
                                    SKERNEL_UPLINK_FRAME_DUNE_RM_E :
                                    SKERNEL_UPLINK_FRAME_PP_TYPE_E;

    /* Parse the frame */
    __LOG(("Parse the frame"));
    if (SNET_FE_IS_RM_FRAME_TYPE(uplinkSrcDescPtr->frameType))
    {
        uplinkSrcDescPtr->srcDevice = frameBufPtr->actualDataPtr[1];
        uplinkSrcDescPtr->partnerDeviceID = frameBufPtr->actualDataPtr[2];
    }

    return ;
}

/*******************************************************************************
* snetFeTx2Port
*
* DESCRIPTION:
*        Forward frame to target port
* INPUTS:
*        devObjPtr   -  pointer to device object.
*        descrPtr  -    pointer to the frame's descriptor.
*        egressPort -   number of egress port.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void snetFeTx2Port
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_UPLINK_DESC_STC * descrPtr,
    IN GT_U32 egressPort
)
{
    SBUF_BUF_ID bufferId;

    snetFeTxMacCounters(devObjPtr, descrPtr , egressPort);

    bufferId = descrPtr->data.PpPacket.source.frameBuf ;

    smainFrame2PortSend(devObjPtr, egressPort,
                        bufferId->actualDataPtr,
                        bufferId->actualDataSize, GT_FALSE);

    return ;
}

/*******************************************************************************
*   snetFeTxQPhase1
*
* DESCRIPTION:
*        Create destination ports vector
* INPUTS:
*        devObjPtr       -  pointer to device object.
*        descrPtr        - pointer to the frame's descriptor.
*        portBmp         - egress port bitmap.
*
* OUTPUTS:
*        destPorts       - number of egress port.
*
* RETURNS:   GT_U32 - number of outgoing ports
*
* COMMENTS:
*
*******************************************************************************/
static GT_U32 snetFeTxQPhase1
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_UPLINK_DESC_STC * descrPtr,
    INOUT GT_U8                destPorts[],
    IN GT_U8                    portBmp
)
{
    GT_U32 outPorts=0;  /* number of output ports */
    GT_U32    ii;

    memset(destPorts,0,sizeof(destPorts));
    {
        for (ii =0 ; ii <= sizeof(GT_U8) ; ++ii)
        {
            if ( (portBmp & (1 << ii)) > 0 )
            {
                destPorts[ii] = 1;
                ++outPorts;
                if (descrPtr->data.PpPacket.useVidx == 0)
                {
                    break;
                }
            }
        }
    }

    return outPorts;
}

