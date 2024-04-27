/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetCheetahTxQ.c
*
* DESCRIPTION:
*       TxQ module processing
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 10 $
*
*******************************************************************************/

#include <asicSimulation/SKernel/suserframes/snetCheetahTxQ.h>
#include <asicSimulation/SKernel/cheetahCommon/sregCheetah.h>
#include <asicSimulation/SKernel/smem/smemCheetah.h>
#include <asicSimulation/SKernel/suserframes/snetCheetahEgress.h>
#include <asicSimulation/SLog/simLog.h>
#include <asicSimulation/SLog/simLogInfoTypePacket.h>

static GT_U32   debug_forceCpuPortEnable = 1;
#define TXQ_BUFF_SIZE              30

/*******************************************************************************
*   simTxqPrintDebugInfo
*
* DESCRIPTION:
*        Print info for given port and queue
*
* INPUTS:
*        devObjPtr   - pointer to device object.
*        port        - port num
*        queue       - queue
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
GT_VOID simTxqPrintDebugInfo
(
    IN SKERNEL_DEVICE_OBJECT   *devObjPtr,
    IN GT_U32                   port,
    IN GT_U32                   queue
)
{
    GT_U32 counter = 0;
    SIM_TRANSMIT_QUEUE_STC *txqPtr;

    txqPtr = &devObjPtr->portsArr[port].onHoldPacketsArr[queue];

    while(txqPtr)
    {
        if(!txqPtr->descrPtr)
        {
            simGeneralPrintf("Empty queue: counter %d, descrPtr %08x, nextDescrPtr %08x, port %d, queue %d , egressPort %d \n",
                    counter, txqPtr->descrPtr, txqPtr->nextDescrPtr, port, queue,txqPtr->egressPort);
        }
        else
        {
            simGeneralPrintf("counter: %d, descrPtr %08x, nextDescrPtr %08x, port %d, queue %d, egressPort %d , descrPtr->numberOfSubscribers %d\n",
                    counter, txqPtr->descrPtr, txqPtr->nextDescrPtr, port, queue,txqPtr->egressPort, txqPtr->descrPtr->numberOfSubscribers);
        }

        counter++;
        txqPtr = txqPtr->nextDescrPtr;
    }
}

/*******************************************************************************
*   simTxqEnqueuePacket
*
* DESCRIPTION:
*        Enqueue the packet for given port and tc (descrPtr->queue_priority).
*
* INPUTS:
*        devObjPtr   - pointer to device object.
*        descrPtr    - pointer to the frame's descriptor.
*        egressPort  - destination port <local dev target physical port>
*        txqPortNum  - the TXQ port that enqueue this packet
*        destVlanTagged - the egress tag state
* OUTPUTS:
*        None.
*
* RETURNS:
*        GT_STATUS   - GT_OK if succeed
*
* COMMENTS:
*
*******************************************************************************/
static GT_STATUS simTxqEnqueuePacket
(
    IN SKERNEL_DEVICE_OBJECT           *devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC *descrPtr,
    IN GT_U32                           egressPort,
    IN GT_U32                           txqPortNum,
    IN GT_U32                           destVlanTagged
)
{
    SIM_TRANSMIT_QUEUE_STC *txqPtr, *tempTxqPtr;

    descrPtr->numberOfSubscribers++;
    /*indicate that this buffer is still used */
    descrPtr->frameBuf->freeState = SBUF_BUFFER_STATE_OTHER_WILL_FREE_E;

    /* NOTE: use 'txqPortNum' even though this is not 'MAC port' to access devObjPtr->portsArr[port]
        just because we not need real 'MAC port' for this operation !!! */
    txqPtr = &devObjPtr->portsArr[txqPortNum].onHoldPacketsArr[descrPtr->queue_priority];
    if(!txqPtr->descrPtr)
    {
        /* this is first struct */
        if(txqPtr->nextDescrPtr)
        {
            skernelFatalError("simTxqEnqueuePacket: nextDescrPtr must be empty here");
        }
        txqPtr->descrPtr = descrPtr;
        txqPtr->egressPort = egressPort;
        txqPtr->destVlanTagged = destVlanTagged;
    }
    else
    {
        /* search end of the list */
        while(txqPtr->nextDescrPtr)
        {
            txqPtr = txqPtr->nextDescrPtr;
        }
        /* create next struct */
        tempTxqPtr = (SIM_TRANSMIT_QUEUE_STC *)calloc(1, sizeof(SIM_TRANSMIT_QUEUE_STC));
        tempTxqPtr->descrPtr = descrPtr;
        tempTxqPtr->egressPort = egressPort;
        tempTxqPtr->destVlanTagged = destVlanTagged;

        txqPtr->nextDescrPtr = tempTxqPtr;
    }


    /*simTxqPrintDebugInfo(devObjPtr, port, descrPtr->queue_priority);*/

    return GT_OK;
}

/*******************************************************************************
*   snetChtTxQPhase2
*
* DESCRIPTION:
*        Analyse destPorts for disabled TC
*
* INPUTS:
*        devObjPtr   - pointer to device object.
*        descrPtr    - pointer to the frame's descriptor.
*        destPorts   - egress ports list
*
* OUTPUTS:
*        destPorts   - egress ports list
*
* RETURNS:
*        outPorts    - number of outgoing ports
*
* COMMENTS:
*        Egress ports with (disabled TC) == (packet TC for egress port)
*        need to be removed from destPorts[portInx].
*        All other ports where disabled TC != packet TC
*        need to be handled in the snetChtEgressDev.
*
*******************************************************************************/
GT_U32 snetChtTxQPhase2
(
    IN    SKERNEL_DEVICE_OBJECT           *devObjPtr,
    IN    SKERNEL_FRAME_CHEETAH_DESCR_STC *descrPtr,
    INOUT GT_U32                           destPorts[],
    IN GT_U8                            destVlanTagged[]
)
{
    DECLARE_FUNC_NAME(snetChtTxQPhase2);

    GT_U32  outPorts  = 0;  /* number of output ports */
    GT_U32  port      = 0;  /* port index loop */
    GT_BOOL isSecondRegister;/* is the per port config is in second register -- for bmp of ports */
    GT_U32  outputPortBit;  /* the bit index for the egress port */
    GT_U32  transmissionEnabled;
    GT_U32  regAddr;
    GT_U32  txqPortNum;
    GT_U32  dmaNumOfCpuPortNum;
    GT_U32  dqUnitInPipe;/* the DP unit in the pipe that handle packet */
    GT_U32  localTxqPortNum;/* local DQ port number to the 'PIPE' */

    simLogPacketFrameUnitSet(SIM_LOG_FRAME_UNIT_TXQ_E);

    dmaNumOfCpuPortNum  = devObjPtr->dmaNumOfCpuPort ? devObjPtr->dmaNumOfCpuPort : SNET_CHT_CPU_PORT_CNS;

    for(port = 0; port < SKERNEL_CHEETAH_EGRESS_MAX_PORT_CNS; port++)
    {
        if(destPorts[port] == 0)
        {
            continue;
        }

        if(debug_forceCpuPortEnable && (port == dmaNumOfCpuPortNum))/* for debug only */
        {
            /* do not allow the CPU port to be 'disabled' */
            outPorts++;
            continue;
        }

        if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            snetChtTxqDqPerPortInfoGet(devObjPtr, descrPtr, port, &isSecondRegister, &outputPortBit);
            txqPortNum = outputPortBit;

            if(devObjPtr->numOfPipes)
            {
                /* Update 'currentPipeId' and get new (local) txqPortNum */
                localTxqPortNum = txqPortNum % devObjPtr->multiDataPath.txqDqNumPortsPerDp;
                dqUnitInPipe = (txqPortNum /devObjPtr->multiDataPath.txqDqNumPortsPerDp) %
                    devObjPtr->multiDataPath.numTxqDq;

                __LOG_PARAM(dqUnitInPipe);
            }
            else
            {
                dqUnitInPipe = 0;
                localTxqPortNum = txqPortNum;
            }
            __LOG_PARAM(localTxqPortNum);

            regAddr = SMEM_LION_DEQUEUE_ENABLE_REG(devObjPtr, localTxqPortNum , dqUnitInPipe);
            smemRegFldGet(devObjPtr, regAddr, descrPtr->queue_priority, 1, &transmissionEnabled);
        }
        else
        {

            /* check is TC disabled for this port */
            if(devObjPtr->txqRevision == 0)
            {
                /* single core devices */
                regAddr = SMEM_CHT_TXQ_CONFIG_REG(devObjPtr, port);
                smemRegFldGet(devObjPtr, regAddr, NUM_OF_TRAFFIC_CLASSES+descrPtr->queue_priority, 1,
                                                                   &transmissionEnabled);
            }
            else
            {
                /* multi core devices */
                snetChtTxqDqPerPortInfoGet(devObjPtr, descrPtr, port, &isSecondRegister, &outputPortBit);
                regAddr = SMEM_LION_DEQUEUE_ENABLE_REG(devObjPtr, outputPortBit , 0);
                if(isSecondRegister == GT_TRUE)
                {
                    regAddr +=4;
                }
                smemRegFldGet(devObjPtr, regAddr, descrPtr->queue_priority, 1, &transmissionEnabled);
            }


            txqPortNum = port;/*no conversion*/
        }

        if(!transmissionEnabled)
        {
            __LOG(("WARNING: target physical port[%d] (mapped to tx_port[%d]) tc[%d] is disabled for transmit , so descriptor is enqueue \n",
                port,
                txqPortNum,
                descrPtr->queue_priority));

            /* enqueue packet */
            simTxqEnqueuePacket(devObjPtr, descrPtr, port , txqPortNum, destVlanTagged[port]);

            /*clear destPorts*/
            destPorts[port] = 0;
        }
        else
        {
            outPorts++;
        }


        if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            /* nothing more to do */
        }
        else
        {
            if(port == SNET_CHT_CPU_PORT_CNS)
            {
                /* no more ports to process */
                break;
            }
        }
    }

    return outPorts;
}

/*******************************************************************************
*   simChtTxqDequeue
*
* DESCRIPTION:
*       Process txq dequeue
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       dataPtr     - pointer to txq info
*       dataLength  - length of the data
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static GT_VOID simChtTxqDequeue
(
    IN SKERNEL_DEVICE_OBJECT *devObjPtr,
    IN GT_U8                 *dataPtr,
    IN GT_U32                 dataLength
)
{
    DECLARE_FUNC_NAME(simChtTxqDequeue);

    GT_U32                  txqPortNum;
    GT_U32                  queue;
    SIM_TRANSMIT_QUEUE_STC *txqPtr, *tempTxqPtr;
    SKERNEL_FRAME_CHEETAH_DESCR_STC *descrPtr;

    /* parse the data from the buffer of the message */
    memcpy(&txqPortNum,dataPtr,sizeof(GT_U32));
    dataPtr+=sizeof(GT_U32);
    memcpy(&queue,dataPtr,sizeof(GT_U32));
    dataPtr+=sizeof(GT_U32);

    /*simGeneralPrintf("simChtTxqDequeue start: port %d, queue %d\n", port, queue);*/

    txqPtr = &devObjPtr->portsArr[txqPortNum].onHoldPacketsArr[queue];

    while(txqPtr->descrPtr)
    {
        descrPtr = txqPtr->descrPtr;
        if(descrPtr->numberOfSubscribers == 0)
        {
            skernelFatalError("simChtTxqDequeue: wrong numberOfSubscribers");
        }

        simLogPacketFrameUnitSet(SIM_LOG_FRAME_UNIT_TXQ_E);

        __LOG(("Start : TXQ port[%d] queue[%d] dequeue descriptor \n",
            txqPortNum,
            queue));

        /* allow the frame to finish it's processing from the same spot where is halted */
        snetChtEgressDev_afterEnqueFromTqxDq(devObjPtr, descrPtr,
            txqPtr->egressPort,
            txqPtr->destVlanTagged);

        __LOG(("Ended : TXQ port[%d] queue[%d] dequeue descriptor \n",
            txqPortNum,
            queue));

        descrPtr->numberOfSubscribers--;
        if(descrPtr->numberOfSubscribers == 0)
        {
            /* this is the subscriber of this descriptor,
               we need to return the buffer to the pool */
            descrPtr->frameBuf->freeState = SBUF_BUFFER_STATE_ALLOCATOR_CAN_FREE_E;
            sbufFree(devObjPtr->bufPool, descrPtr->frameBuf);
        }

        /* delete node */
        tempTxqPtr = txqPtr->nextDescrPtr;
        if(tempTxqPtr)
        {
            txqPtr->descrPtr     = tempTxqPtr->descrPtr;
            txqPtr->nextDescrPtr = tempTxqPtr->nextDescrPtr;
            free(tempTxqPtr);
        }
        else
        {
            txqPtr->descrPtr = NULL;
            break;
        }
    }
}

/*******************************************************************************
*  smemTxqSendDequeueMessages
*
* DESCRIPTION:
*       Send dequeue messages
*
* INPUTS:
*       devObjPtr     - device object PTR.
*       regValue      - register value.
*       port          - port num
*       startBitToSum - start bit to read
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
GT_VOID smemTxqSendDequeueMessages
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32  regValue,
    IN GT_U32  port,
    IN GT_U32  startBitToSum
)
{
    DECLARE_FUNC_NAME(smemTxqSendDequeueMessages);

    SBUF_BUF_ID      bufferId;       /* buffer */
    GT_U8           *dataPtr;        /* pointer to the data in the buffer */
    GT_U32           dataSize;       /* data size */
    GENERIC_MSG_FUNC genFunc = simChtTxqDequeue;/* generic function */

    GT_U32           queue = 0;
    GT_BOOL          isTcTxEnabled = GT_FALSE;

    for(queue = 0; queue < NUM_OF_TRAFFIC_CLASSES; queue++)
    {
        isTcTxEnabled = SMEM_U32_GET_FIELD(regValue, startBitToSum+queue, 1);
        if(isTcTxEnabled)
        {
            /* check that the queue is not empty */
            if(!devObjPtr->portsArr[port].onHoldPacketsArr[queue].descrPtr)
            {
                continue;
            }

            __LOG(("port[%d] queue[%d] is re-enabled for transmit send message for dequeu",port,queue));

            /* queue not empty and queue transmission enabled - so sendMessage */
            /* Get buffer */
            bufferId = sbufAlloc(devObjPtr->bufPool, TXQ_BUFF_SIZE);
            if (bufferId == NULL)
            {
                simWarningPrintf("smemTxqSendDequeueMessages: no buffers for TxQ\n");
                return;
            }

            /* Get actual data pointer */
            sbufDataGet(bufferId, (GT_U8 **)&dataPtr, &dataSize);

            /* put the name of the function into the message */
            memcpy(dataPtr,&genFunc,sizeof(GENERIC_MSG_FUNC));
            dataPtr+=sizeof(GENERIC_MSG_FUNC);

            /* save port */
            memcpy(dataPtr,&port,sizeof(GT_U32));
            dataPtr+=sizeof(GT_U32);

            /* save queue */
            memcpy(dataPtr,&queue,sizeof(GT_U32));
            dataPtr+=sizeof(GT_U32);

            /* set source type of buffer  */
            bufferId->srcType = SMAIN_SRC_TYPE_CPU_E;

            /* set message type of buffer */
            bufferId->dataType = SMAIN_MSG_TYPE_GENERIC_FUNCTION_E;

            /* put buffer to queue        */
            squeBufPut(devObjPtr->queueId, SIM_CAST_BUFF(bufferId));
        }
        else
        {
            /* queue transmission disabled - do nothing */
            /*simWarningPrintf("smemChtActiveWriteTxQConfigReg: port %d tc %d disabled\n", port, tc);*/
        }
    }
}




