/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetFAP.c
*
* DESCRIPTION:
*       This is a external API definition for snetFAP module of Dune Simulation.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 10 $
*
*******************************************************************************/
#ifdef _WIN32
#include <io.h>
#endif
#include <stdio.h>
#include <stdarg.h>
#include <asicSimulation/SKernel/suserframes/snet.h>
#include <asicSimulation/SKernel/suserframes/snetFap.h>
#include <asicSimulation/SKernel/smem/smemFAP.h>
#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SLog/simLog.h>

/*
 * Typedef: struct SFAP_UNICAST_DIST_MEM_ENTRY_STC
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
}SNET_FAP_UNICAST_DIST_INFO_STC;

SNET_FAP_UNICAST_DIST_INFO_STC     fapUnicastKeyDistArray
                                    [SMAIN_SWITCH_FABRIC_MAX_NUM_OF_DEVICES_CNS]
                                    [SMAIN_SWITCH_FABRIC_MAX_NUM_OF_LINKS_CNS];

typedef struct {
    GT_U32 rxOffset;
    GT_U32 txOffset;
} FAP_PORT_COUNTERS;

/* Calculating FAP Port Counter Addresses */
static FAP_PORT_COUNTERS portsCellCnt[FAP_PORTS_NUMBER] =
{
    {0x4080AC24, 0x4080AC00},{0x4080AC28, 0x4080AC04},{0x4080AC2C, 0x4080AC08},
    {0x4080AC30, 0x4080AC0C},{0x4080AC34, 0x4080AC10},{0x4080AC3C, 0x4080AC14},
    {0x4080AC40, 0x4080AC18},{0x4080AC44, 0x4080AC1C}
};

/* Private declarations of functions */
static GT_STATUS snetFapRouteProcess
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_UPLINK_DESC_STC * descrPtr
);

static void snetFapFrameProcess
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SBUF_BUF_ID bufferId,
    IN GT_U32 srcPort
);

static void snetFapFrameParsing
(
    IN SKERNEL_DEVICE_OBJECT      *     devObjPtr,
    INOUT SKERNEL_UPLINK_DESC_STC *     descrPtr
);

static void snetFapSendsRMTask
(
    IN void * deviceObjPtr
);

static void snetFapSendRmMsg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
);

static void snetFapMessageHandlerTask
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
);

static void snetFapUpdateUnicastDistTable
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
);

static GT_BOOL snetFapRxProcess
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_UPLINK_DESC_STC * descrPtr
);

static GT_U32 snetFapTxQPhase1
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_UPLINK_DESC_STC * descrPtr,
    INOUT GT_U8                destPorts[],
    IN GT_U8                    portBmp
);

static GT_STATUS snetFapEgressProcess
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_UPLINK_DESC_STC * descrPtr
);

static void snetFapTx2Port
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_UPLINK_DESC_STC * descrPtr,
    IN GT_U32 egressPort
);

static GT_BOOL snetFapIngress
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_UPLINK_DESC_STC * descrPtr
);

static void snetFapLbpRxCounters
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_UPLINK_DESC_STC * descrPtr
);

static GT_BOOL snetFapQueueCtrlRx
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_UPLINK_DESC_STC * descrPtr
);

static void snetFapTxCountUpdate
(
    IN SKERNEL_DEVICE_OBJECT      *     devObjPtr,
    IN SKERNEL_UPLINK_DESC_STC    *     descrPtr,
    IN GT_U32                           egressPort
);

static void snetFapRxLinkCountUpdate
(
    IN SKERNEL_DEVICE_OBJECT      *     devObjPtr,
    IN SKERNEL_UPLINK_DESC_STC    *     descrPtr
);

/*******************************************************************************
*   snetFapProcessInit
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
void snetFapProcessInit
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    GT_U32      ii;
    SNET_FAP_UNICAST_DIST_INFO_STC * fieldInfoPtr;

    ASSERT_PTR(devObjPtr);

    devObjPtr->descriptorPtr =
        (void *)calloc(1, sizeof(SKERNEL_UPLINK_DESC_STC));

    if (devObjPtr->descriptorPtr == 0)
    {
        skernelFatalError("snetFapProcessInit: allocation error\n");
    }

    for (ii = 0 ; ii < (2 * SMAIN_SWITCH_FABRIC_1KB) ; ++ii)
    {
        fieldInfoPtr = &fapUnicastKeyDistArray[devObjPtr->deviceId][ii];
        fieldInfoPtr->deviceIndex = ii;
        fieldInfoPtr->portBmp     = 0;
        fieldInfoPtr->linkState   = 0;
        fieldInfoPtr->Refresh     = 0;
    }

    devObjPtr->devFrameProcFuncPtr = snetFapFrameProcess;

    snetFapMessageHandlerTask(devObjPtr);
}

/*******************************************************************************
*   snetFapFrameProcess
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
static void snetFapFrameProcess
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SBUF_BUF_ID bufferId,
    IN GT_U32 srcPort
)
{
    SKERNEL_UPLINK_DESC_STC * descrPtr; /* pointer to the frame's descriptor */
    GT_BOOL retVal;

    ASSERT_PTR(devObjPtr);

    descrPtr = (SKERNEL_UPLINK_DESC_STC *)devObjPtr->descriptorPtr;
    memset(descrPtr, 0, sizeof(SKERNEL_UPLINK_DESC_STC));

    descrPtr->data.PpPacket.source.frameBuf = bufferId;
    descrPtr->data.PpPacket.source.srcPort = srcPort;
    descrPtr->data.PpPacket.source.byteCount = (GT_U16)bufferId->actualDataSize;

    snetFapFrameParsing(devObjPtr,descrPtr);

    /* Rx layer processing of the Fabric Access Processor  */
    retVal = snetFapRxProcess(devObjPtr,descrPtr);
    if (retVal == GT_FALSE)
        return;

    /* Ingress processing main routine */
    retVal = snetFapIngress(devObjPtr,descrPtr);
    if (retVal == GT_FALSE)
        return;

    /* Make routing decision for the incoming frame/message */
    snetFapRouteProcess(devObjPtr,descrPtr);

    /*  Egress processing main routine    */
    snetFapEgressProcess(devObjPtr,descrPtr);
}

/*******************************************************************************
*   snetFapRpDecision
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
*******************************************************************************/
static GT_STATUS snetFapRouteProcess
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_UPLINK_DESC_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetFapRouteProcess);

    GT_U32  srcDev;
    GT_U32  srcPort;
    GT_U32  regAddr;
    GT_U32  srcChipId;
    GT_U32  fldValue = 0;
    SNET_FAP_UNICAST_DIST_INFO_STC * fieldInfoPtr;
    SKERNEL_UPLINK_PP_SOURCE_DESCR_STC * uplinkSrcDescrPtr;

    uplinkSrcDescrPtr = &(descrPtr->data.PpPacket.source);
    srcDev = uplinkSrcDescrPtr->srcDevice;
    srcPort = uplinkSrcDescrPtr->srcPort;
    srcChipId = uplinkSrcDescrPtr->partnerDeviceID;
    if (uplinkSrcDescrPtr->frameType == SKERNEL_UPLINK_FRAME_DUNE_RM_E)
    {
        /* update unicast distribution table */
        __LOG(("update unicast distribution table"));
        SNET_FAP_UNICAST_DIST_TABLE_ENTRY_REG( srcDev , &regAddr);
        smemRegFldSet(devObjPtr, regAddr,  1 << srcPort,    1 , 1);

        /* update internal unicast distribution table */
        fieldInfoPtr = &fapUnicastKeyDistArray[devObjPtr->deviceId][srcDev];
        fieldInfoPtr->deviceIndex = srcDev;
        fieldInfoPtr->portBmp     |= (1 << srcPort);
        fieldInfoPtr->linkState   = 1;
        fieldInfoPtr->Refresh     = 1;

        /* find connectivity map registers */
        SNET_FAP_CONNECTIVITY_MAP_REG( srcPort , &regAddr);

        /* calculate the connectivity entry */
        SMEM_U32_SET_FIELD(fldValue, 14, 5, srcChipId); /* source pp device */
        SMEM_U32_SET_FIELD(fldValue, 11, 3, srcPort);   /* source port*/
        SMEM_U32_SET_FIELD(fldValue, 0, 11, srcDev);    /* source device */

        /* update connectivity map entry */
        smemRegSet(devObjPtr, regAddr, fldValue);
    }

    return GT_OK ;
}

/*******************************************************************************
*   snetFapTxQPhase1
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
static GT_U32 snetFapTxQPhase1
(
    IN SKERNEL_DEVICE_OBJECT *  devObjPtr,
    IN SKERNEL_UPLINK_DESC_STC * descrPtr,
    INOUT GT_U8                  destPorts[],
    IN GT_U8                     portBmp
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

/*******************************************************************************
*   snetFapEgressProcess
*
* DESCRIPTION:
*       Routine Process engine.
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - pointer to the descriptor
*
* OUTPUTS:
*
* RETURNS:
*       GT_OK
*
* COMMENTS:
*
*******************************************************************************/
static GT_STATUS snetFapEgressProcess
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_UPLINK_DESC_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetFapEgressProcess);

    GT_U32  regAddr;
    GT_U8   destPorts[FAP_PORTS_NUMBER];      /* destination untagged port array */
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
            SNET_FAP_MULTICAST_DIST_TABLE_ENTRY_REG(uplinkSrcDescrPtr->vid ,
                                                     &regAddr );
        }
        else
        {   /* unicast distribution table */
            __LOG(("unicast distribution table"));
            SNET_FAP_UNICAST_DIST_TABLE_ENTRY_REG(uplinkSrcDescrPtr->srcDevice ,
                                                 &regAddr );
        }

        smemRegFldGet(devObjPtr, regAddr, 0, 8, (GT_U32*)&portBmp);
        outPorts = snetFapTxQPhase1(devObjPtr, descrPtr, destPorts, portBmp);
        switch (outPorts)
        {
            case 0:
                break ;
            case 1:
                snetFapTx2Port(devObjPtr, descrPtr, destPorts[outPorts]);
                break;
            default:
                while (portInx < FAP_PORTS_NUMBER)
                {
                    if (destPorts[portInx])
                    {
                        targetPort = portInx;
                        snetFapTx2Port(devObjPtr, descrPtr, targetPort);
                    }
                    portInx++;
                }/* while */
        }
    }

    return GT_OK;
}

/*******************************************************************************
* snetFapTx2Port
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
static void snetFapTx2Port
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_UPLINK_DESC_STC * descrPtr,
    IN GT_U32 egressPort
)
{
    SBUF_BUF_ID bufferId;

    snetFapTxCountUpdate(devObjPtr, descrPtr, egressPort);

    bufferId =  descrPtr->data.PpPacket.source.frameBuf ;

    smainFrame2PortSend(devObjPtr, egressPort,
                        bufferId->actualDataPtr,
                        bufferId->actualDataSize, GT_FALSE);

    return ;
}

/*******************************************************************************
*   snetFapFrameParsing
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
void snetFapFrameParsing
(
    IN SKERNEL_DEVICE_OBJECT      *     devObjPtr,
    INOUT SKERNEL_UPLINK_DESC_STC *     descrPtr
)
{
    DECLARE_FUNC_NAME(snetFapFrameParsing);

    SBUF_BUF_STC   * frameBufPtr;
    SKERNEL_UPLINK_PP_SOURCE_DESCR_STC  * uplinkSrcDescPtr;

    uplinkSrcDescPtr = &(descrPtr->data.PpPacket.source);
    frameBufPtr = uplinkSrcDescPtr->frameBuf ;

    uplinkSrcDescPtr->frameType = (frameBufPtr->dataType == SMAIN_REACHABILITY_MSG_E) ?
                                    SKERNEL_UPLINK_FRAME_DUNE_RM_E :
                                    SKERNEL_UPLINK_FRAME_PP_TYPE_E;

    /* Parse the frame */
    __LOG(("Parse the frame"));
    if (SNET_FAP_IS_RM_FRAME_TYPE(uplinkSrcDescPtr->frameType))
    {
        uplinkSrcDescPtr->srcDevice = frameBufPtr->actualDataPtr[1];
        uplinkSrcDescPtr->partnerDeviceID = frameBufPtr->actualDataPtr[2];
    }

    return ;
}

/*******************************************************************************
*   snetFapRxProcess
*
* DESCRIPTION:
*       Rx layer processing of the FAP
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
static GT_BOOL snetFapRxProcess
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_UPLINK_DESC_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetFapRxProcess);

    GT_U32 fldValue;

    smemRegFldGet(devObjPtr, FAP_GLOBAL_CONTROL_REG, 0, 1, &fldValue);
    /* Device Enable */
    __LOG(("Device Enable"));
    if (fldValue != 1)
        return GT_FALSE;

    return GT_TRUE;
}

/*******************************************************************************
*   snetFapCpuMessageProcess
*
* DESCRIPTION:
*       process CPU message (reachability message)
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       dataPtr      - pointer to the frame's descriptor.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*          TBD
*
*******************************************************************************/
GT_VOID snetFapCpuMessageProcess
(
    IN SKERNEL_DEVICE_OBJECT      *devObjPTR,
    IN GT_U8                      *dataPtr
)
{
    ASSERT_PTR(devObjPTR);
    ASSERT_PTR(dataPtr);
}

/*******************************************************************************
*   snetFapIngress
*
* DESCRIPTION:
*       FAP Ingress Policy
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
static GT_BOOL snetFapIngress
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_UPLINK_DESC_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetFapIngress);

    GT_U32 fldValue;
    GT_U8  isMC = 0;

    /* LBP Enablers */
    smemRegFldGet(devObjPtr, FAP_LBP_ENABLERS_ENA_REG, 0, 1, &fldValue);
    if (fldValue == 1)
        return GT_FALSE;

    /* type of packet */
    isMC = descrPtr->data.PpPacket.useVidx ;
    if (!isMC)
    {
        /* check if to discard unicast packet  */
        __LOG(("check if to discard unicast packet"));
        smemRegFldGet(devObjPtr, FAP_GLOBAL_CONTROL_REG, 7, 1, &fldValue);
        if (fldValue != 1)
            return GT_FALSE;
    }
    else
    {
        /* check if to discard multicast packet  */
        __LOG(("check if to discard multicast packet"));
        smemRegFldGet(devObjPtr, FAP_GLOBAL_CONTROL_REG, 8, 1, &fldValue);
        if (fldValue != 1)
            return GT_FALSE;
    }

    if (snetFapQueueCtrlRx(devObjPtr,descrPtr) != GT_TRUE)
        return GT_FALSE;

    /* update rx per link counters */
    snetFapRxLinkCountUpdate(devObjPtr,descrPtr);

    /* update LBP rx counters */
    snetFapLbpRxCounters(devObjPtr,descrPtr);

    return GT_TRUE;
}

/*******************************************************************************
*   snetFapQueueCtrlRx
*
* DESCRIPTION:
*
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr     - pointer to the frame's descriptor.
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
static GT_BOOL snetFapQueueCtrlRx
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_UPLINK_DESC_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetFapQueueCtrlRx);

    GT_U32  fldValue;
    GT_U32  qcCounter = 0;

    /* Queue controller enable */
    /* check if to discard packets  */
    smemRegFldGet(devObjPtr, FAP_QUEUE_CONTROLLER_ENA_REG, 0, 1, &fldValue);
    if (fldValue != 1)
        return GT_FALSE;

    /* update QC counter */
    __LOG(("update QC counter"));
    smemRegFldGet(devObjPtr, FAP_LBP_PACKET_COUNTER_REG, 0, 30, &qcCounter);
    smemRegFldSet(devObjPtr, FAP_LBP_PACKET_COUNTER_REG, 0, 30, ++qcCounter);

    return GT_TRUE;
}

/*******************************************************************************
*   snetFapLbpRxCounters
*
* DESCRIPTION:
*
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr     - pointer to the frame's descriptor.
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
static void snetFapLbpRxCounters
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_UPLINK_DESC_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetFapLbpRxCounters);

    GT_U8  isMC = 0;
    SFAP_LBP_COUNTER_MODE_TYPE_ENT lbpCounterMode;
    SFAP_LBP_PACKET_COUNTER_MODE_TYPE_ENT lbpPacketCounterMode;
    GT_U32  lbpCounter = 0;

    isMC = descrPtr->data.PpPacket.useVidx ;

    /* LBP counters */
    __LOG(("LBP counters"));
    /* check if to count ingress packet  */
    smemRegFldGet(devObjPtr, FAP_LBP_ENABLERS_ENA_REG, 12, 2, (void *)&lbpCounterMode);

    if ( (lbpCounterMode != SNET_FAP_LBP_COUNT_ALL_STATE) &&
         (lbpCounterMode !=  SNET_FAP_LBP_COUNT_SPEC_MODE_STATE) )
          return ;

    /* check if to count only specific type of packet */
    if (lbpCounterMode == SNET_FAP_LBP_COUNT_SPEC_MODE_STATE)
    {
        smemRegFldGet(devObjPtr, FAP_LBP_ENABLERS_ENA_REG, 27, 2, (void *)&lbpPacketCounterMode);
        if ((lbpPacketCounterMode ==
                (SFAP_LBP_PACKET_COUNTER_MODE_TYPE_ENT)SKERNEL_UNICAST_MAC_E) && (isMC))
            return ;
        if ((lbpPacketCounterMode ==
              (SFAP_LBP_PACKET_COUNTER_MODE_TYPE_ENT)SKERNEL_MULTICAST_MAC_E) && (isMC == 0))
            return ;
    }
    /* update LBP counter */
    smemRegFldGet(devObjPtr, FAP_LBP_PACKET_COUNTER_REG, 0, 30, &lbpCounter);
    smemRegFldSet(devObjPtr, FAP_LBP_PACKET_COUNTER_REG, 0, 30, ++lbpCounter);
}

/*******************************************************************************
*   snetFapTxCountUpdate
*
* DESCRIPTION:
*       update of Transmit Cell Per-Link Counters.
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
static void snetFapTxCountUpdate
(
    IN SKERNEL_DEVICE_OBJECT      *     devObjPtr,
    IN SKERNEL_UPLINK_DESC_STC    *     descrPtr,
    IN GT_U32                           egressPort
)
{
    GT_U32  cellCnt = 0 , linkCellCnt = 0 ;
    SFAP_CELL_COUNTER_MODE_TYPE_ENT fldVal ;
    GT_U32  regAddress;

    smemRegFldGet(devObjPtr, FAP_TX_DATA_CELL_CNT_REG, 0, 30, &cellCnt);
    smemRegFldSet(devObjPtr, FAP_TX_DATA_CELL_CNT_REG, 0, 30, ++cellCnt);

    smemRegFldGet(devObjPtr, FAP_CELL_COUNTER_CONFIG_REG, 16, 2, (void *)&fldVal);
    if (fldVal == SNET_CELL_CONTROL_COUNTER_ENT)
        return ;

    regAddress = portsCellCnt[egressPort].txOffset;
    smemRegFldGet(devObjPtr, regAddress, 0, 30, &linkCellCnt);
    smemRegFldSet(devObjPtr, regAddress, 0, 30, ++linkCellCnt);

    return ;

}

/*******************************************************************************
*   snetFapRxLinkCountUpdate
*
* DESCRIPTION:
*       update of received Cell Per-Link Counters.
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
static void snetFapRxLinkCountUpdate
(
    IN SKERNEL_DEVICE_OBJECT      *     devObjPtr,
    IN SKERNEL_UPLINK_DESC_STC    *     descrPtr
)
{
    GT_U32 linkCellCnt = 0 ;
    SFAP_CELL_COUNTER_MODE_TYPE_ENT fldVal ;
    GT_U32  regAddress;
    GT_U32  srcPort = descrPtr->data.PpPacket.source.srcPort;

    smemRegFldGet(devObjPtr, FAP_CELL_COUNTER_CONFIG_REG, 18, 2, (void *)&fldVal);
    if (fldVal == SNET_CELL_CONTROL_COUNTER_ENT)
        return ;

    regAddress = portsCellCnt[srcPort].rxOffset;
    smemRegFldGet(devObjPtr, regAddress, 0, 30, &linkCellCnt);
    smemRegFldSet(devObjPtr, regAddress, 0, 30, ++linkCellCnt);

    return ;
}

/*******************************************************************************
*   snetFapMessageHandlerTask
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
static void snetFapMessageHandlerTask
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    GT_TASK_HANDLE          taskHandl;        /* task handle */
    GT_TASK_PRIORITY_ENT    taskPriority;

    taskPriority = GT_TASK_PRIORITY_HIGHEST;
    taskHandl = SIM_OS_MAC(simOsTaskCreate)(
                    taskPriority,
                    (unsigned (__TASKCONV *)(void*))snetFapSendsRMTask,
                    (void *) devObjPtr);

    if (taskHandl == NULL)
    {
        skernelFatalError(" snetFapMessageHandlerTask : cannot \
                            create message handler task for device %u", \
                            devObjPtr->deviceId);
    }
}

/*******************************************************************************
*   snetFapSendsRMTask
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
static void snetFapSendsRMTask
(
    IN void * devObjPtr
)
{
    GT_U32      regAddr;
    GT_U32      fldVal;

    regAddr = FAP_ROUT_PROCESS_ENA_REG;
    while(1)
    {
        SIM_OS_MAC(simOsSleep)(SMAIN_SWITCH_FABRIC_CHIP_CLOCK_CNS);

        smemRegFldGet(devObjPtr, regAddr,  8 ,  5 , &fldVal);

        if (fldVal > 4)
        {
            snetFapSendRmMsg(devObjPtr);
        }

        snetFapUpdateUnicastDistTable(devObjPtr);
    }
}

/*******************************************************************************
*   snetFapSendRmMsg
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
*******************************************************************************/
static void snetFapSendRmMsg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    GT_U32                  ii;
    SKERNEL_UPLINK_DESC_STC   frameDesc;
    SKERNEL_UPLINK_PP_SOURCE_DESCR_STC  * uplinkSrcDescPtr;
    SBUF_BUF_ID bufferId;       /* buffer */
    GT_U8_PTR   egrBufPtr;
    GT_U8_PTR   dataPtr;
    GT_U32 dataSize;            /* data size */
    SKERNEL_UPLINK_FRAME_TYPE_ENT  msgType;
    GT_U32 copySize = 0;
    GT_U32 egressBufLen ;

    egrBufPtr = devObjPtr->egressBuffer ;
    msgType = SKERNEL_UPLINK_FRAME_DUNE_RM_E;

    /* Fill the uplink descriptor */
    memset(&frameDesc,0,sizeof(SKERNEL_UPLINK_DESC_STC));

    uplinkSrcDescPtr = &frameDesc.data.PpPacket.source ;
    uplinkSrcDescPtr->frameType = msgType;
    uplinkSrcDescPtr->srcDevice = (GT_U8)devObjPtr->deviceId ;
    uplinkSrcDescPtr->partnerDeviceID =  devObjPtr->uplink.partnerDeviceID;

    /* fill buffer for the reachability message details */
    bufferId = uplinkSrcDescPtr->frameBuf;

    /* allocate buffer for the reachability message */
    bufferId = sbufAlloc(devObjPtr->bufPool, FAP_REACHABILITY_MSG_SIZE);
    if (bufferId == NULL)
    {
      printf(" snetFapSendRmMsg: no buffers for reachability message \n");
      return;
    }
    bufferId->dataType = SMAIN_REACHABILITY_MSG_E;

    /* Get actual data pointer */
    sbufDataGet(bufferId, (GT_U8 **)&dataPtr, &dataSize);
    /* copy the type of the message  */
    memcpy(dataPtr, &msgType , sizeof(GT_U32) );
    copySize = sizeof(GT_U8);
    MEM_APPEND(egrBufPtr, dataPtr, copySize);
    dataPtr++;
    /* copy the id/number of the device  */
    memcpy(dataPtr, &devObjPtr->deviceId , copySize );
    /* Append buffer to device egress buffer */
    MEM_APPEND(egrBufPtr, dataPtr, copySize);
    dataPtr++;

    /* copy the id/number of the connected device id*/
    memcpy(dataPtr, &(devObjPtr->uplink.partnerDeviceID) , copySize );
        MEM_APPEND(egrBufPtr, dataPtr, copySize);

    egressBufLen = egrBufPtr - devObjPtr->egressBuffer;

    for (ii = 0 ; ii < FAP_PORTS_NUMBER ; ii++)
    {
        smainFrame2PortSend(devObjPtr,
                            ii,
                            (GT_U8 *)&devObjPtr->egressBuffer,
                            egressBufLen,
                            GT_FALSE);
    }
}

/*******************************************************************************
*   snetFapUpdateUnicastDistTable
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
static void snetFapUpdateUnicastDistTable
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    GT_U32                  ii;
    GT_U32                  regAddress;
    GT_U32                  data = 0;
    SNET_FAP_UNICAST_DIST_INFO_STC * fieldInfoPtr;

    for (ii = 0 ; ii < (2 * SMAIN_SWITCH_FABRIC_1KB) ; ii++)
    {
        fieldInfoPtr = &fapUnicastKeyDistArray[devObjPtr->deviceId][ii];
        if ( fieldInfoPtr->Refresh == 0 )
        {
            if ( fieldInfoPtr->linkState == 1 )
            {
                SNET_FAP_UNICAST_DIST_TABLE_ENTRY_REG(devObjPtr->deviceId,&regAddress);
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
*   snetFapProcessFrameFromUpLink
*
* DESCRIPTION:
*
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descPtr     - pointer to uplink descriptor.
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
void snetFapProcessFrameFromUpLink
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_UPLINK_DESC_STC    * descPtr
)
{
    DECLARE_FUNC_NAME(snetFapProcessFrameFromUpLink);

    GT_BOOL retVal;

    /* Parse the frame */
    __LOG(("Parse the frame"));
    snetFapFrameParsing(devObjPtr, descPtr);

    /* Rx layer processing of the Fabric Access Processor  */
    retVal = snetFapRxProcess(devObjPtr, descPtr);
    if (retVal == GT_FALSE)
        return;

    /* Ingress processing main routine */
    retVal = snetFapIngress(devObjPtr, descPtr);
    if (retVal == GT_FALSE)
        return;

    /* Make routing decision for the incoming frame/message */
    snetFapRouteProcess(devObjPtr, descPtr);

    /*  Egress processing main routine    */
    snetFapEgressProcess(devObjPtr, descPtr);
}

