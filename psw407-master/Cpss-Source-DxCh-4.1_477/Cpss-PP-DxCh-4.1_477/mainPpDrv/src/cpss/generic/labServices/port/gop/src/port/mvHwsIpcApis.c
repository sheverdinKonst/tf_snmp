/*******************************************************************************
*                Copyright 2001, Marvell International Ltd.
* This code contains confidential information of Marvell semiconductor, inc.
* no rights are granted herein under any patent, mask work right or copyright
* of Marvell or any third party.
* Marvell reserves the right at its sole discretion to request that this code
* be immediately returned to Marvell. This code is provided "as is".
* Marvell makes no warranties, express, implied or otherwise, regarding its
* accuracy, completeness or performance.
********************************************************************************
* mvHwsIpcApi.c
*
* DESCRIPTION:
*           This file contains APIs for HWS IPC
*
* DEPENDENCIES:
*
* FILE REVISION NUMBER:
*       $Revision: 1 $
******************************************************************************/
#include <common/siliconIf/mvSiliconIf.h>
#include <private/mvHwsPortMiscIf.h>
#include <mvHwsPortInitIf.h>
#include <mvHwsPortCfgIf.h>
#include <common/siliconIf/mvSiliconIf.h>
#include <mvHwsIpcDefs.h>

/**************************** Globals ****************************************************/

#ifdef MV_HWS_IPC_STATISTICS_SERVICE_CPU_DEBUG

GT_U32 mvHwsSrvCpuRxMsgCount[MV_HWS_IPC_LAST_CTRL_MSG_TYPE] = {0};
GT_U32 mvHwsSrvCpuTxMsgCount[MV_HWS_IPC_LAST_CTRL_MSG_TYPE] = {0};
GT_U32 mvHwsSrvCpuGenFailureCount = 0;

#endif /*MV_HWS_IPC_STATISTICS_SERVICE_CPU_DEBUG*/

#ifndef MV_HWS_REDUCED_BUILD

/* IPC queue IDs pool */
GT_U32 hwsIpcQueueIdPool[HWS_MAX_DEVICE_NUM][MV_HWS_MAX_HOST2HWS_REQ_MSG_NUM] = {{0},{0}};

#ifdef MV_HWS_IPC_DEBUG
/* for debug only  -queues are placed in the global memory*/
MV_HWS_IPC_CTRL_MSG_STRUCT  Host2HostHwsIpcQueue[HWS_MAX_DEVICE_NUM][MV_HWS_MAX_HOST2HWS_REQ_MSG_NUM];
MV_HWS_IPC_REPLY_MSG_STRUCT Host2HostHwsReply[HWS_MAX_DEVICE_NUM][MV_HWS_MAX_HWS2HOST_REPLY_QUEUE_NUM];

#endif /*MV_HWS_IPC_DEBUG*/

MV_HWS_IPC_STATISTICS_STRUCT mvHwsIpcStatistics[HWS_MAX_DEVICE_NUM];  /* the structure to gather HWS IPC statistics on Host*/

/**************************** Pre-Declaration ********************************************/

GT_STATUS mvHwsSetIpcInfo
(
    GT_U8                       devNum,
    GT_U32                      msgType,
    MV_HWS_IPC_CTRL_MSG_STRUCT  *msgDataPtr,
    GT_U32                      msgLength
);

GT_STATUS mvHwsIpcCtrlMsgTx
(
    MV_HWS_IPC_CTRL_MSG_STRUCT *txCtrlMsg
);

GT_STATUS mvHwsIpcCtrlMsgRx
(
    GT_U32                     queueId,
    MV_HWS_IPC_CTRL_MSG_STRUCT *readMsg
);

GT_STATUS mvHwsIpcReplyMsgTx
(
    GT_U32                      devNum,
    GT_U32                      queueId,
    MV_HWS_IPC_REPLY_MSG_STRUCT *txReplyMsg
);

GT_STATUS mvHwsIpcReplyMsgRx
(
    GT_U8                                   devNum,
    MV_HWS_IPC_CTRL_MSG_DEF_TYPE            msgId,
    GT_U32                                  queueId,
    MV_HWS_IPC_REPLY_MSG_STRUCT             *rxReplyData
);

#ifdef MV_HWS_IPC_DEBUG
static GT_VOID mvHwsIpcCtrlMsgRxSimulation
(
    MV_HWS_IPC_CTRL_MSG_STRUCT *readMsg,
    MV_HWS_IPC_REPLY_MSG_STRUCT *reply
);
#endif
/*******************************************************************************
* mvHwsIpcInit
*
* DESCRIPTION:
*       HW Services Ipc initialization
*
*
* INPUTS:
*       devNum     - device number
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       0  - on success
*       1  - on error
*
*******************************************************************************/
GT_VOID mvHwsIpcInit
(
        GT_U8 devNum
)
{
    hwsOsMemSetFuncPtr(hwsIpcQueueIdPool[devNum],0 , MV_HWS_MAX_HOST2HWS_REQ_MSG_NUM);
    hwsOsMemSetFuncPtr(mvHwsIpcStatistics[devNum].mvHwsHostRxMsgCount, 0, MV_HWS_IPC_LAST_CTRL_MSG_TYPE);
	hwsOsMemSetFuncPtr(mvHwsIpcStatistics[devNum].mvHwsHostTxMsgCount, 0, MV_HWS_IPC_LAST_CTRL_MSG_TYPE);
	hwsOsMemSetFuncPtr(mvHwsIpcStatistics[devNum].mvHwsPortIpcFailureCount, 0 , MV_HWS_IPC_MAX_PORT_NUM);

}

/*******************************************************************************
* mvHwsGetQueueId
*
* DESCRIPTION:
*       finds free queue for Host 2 HWS connection
*
*
* INPUTS:
*       devNum      - device number
*       queueId     - pointer to queue ID
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       0  - on success
*       1  - on error
*
*******************************************************************************/
GT_U32 mvHwsGetQueueId
(
    GT_U8 devNum,
    GT_U32 * queueId
)
{
    GT_U32   i;

    for (i=0; i < MV_HWS_MAX_HOST2HWS_REQ_MSG_NUM; i++)
    {
        if (hwsIpcQueueIdPool[devNum][i] == MV_HWS_IPC_FREE_QUEUE)
        {
            *queueId = i;
            hwsIpcQueueIdPool[devNum][i] = MV_HWS_IPC_QUEUE_BUSY;
            return GT_OK;
        }
    }
    if (i == MV_HWS_MAX_HOST2HWS_REQ_MSG_NUM)
    {
        mvHwsIpcStatistics[devNum].mvHwsIpcGenFailureCount++;
        osPrintf("No free Host2Hws TX message\n");
        return GT_NO_RESOURCE;
    }

    return GT_OK;
}

/*******************************************************************************
* mvHwsSetIpcInfo
*
* DESCRIPTION:
*       returns queue Id to the pool
*
*
* INPUTS:
*       devNum     - device number
*       queueId    - queue Id
* OUTPUTS:
*       None.
*
* RETURNS:
*       0  - on success
*       1  - on error
*
*******************************************************************************/
GT_STATUS mvHwsReturnQueueId
(
    GT_U8 devNum,
    GT_U32 queueId
)
{
    if (queueId >= MV_HWS_MAX_HOST2HWS_REQ_MSG_NUM)
    {
        mvHwsIpcStatistics[devNum].mvHwsIpcGenFailureCount++;
        osPrintf("mvHwsReturnQueueId queue ID %d doesn't exist \n", queueId);
        return GT_BAD_PARAM;
    }

    hwsIpcQueueIdPool[devNum][queueId] = MV_HWS_IPC_FREE_QUEUE;

    return GT_OK;

}

/*******************************************************************************
* mvHwsIpcCtrlMsgTx
*
* DESCRIPTION:
*       Send IPC message from Host to HW Services
*
*
* INPUTS:
*       txCtrlMsg    - pointer to the message
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       0  - on success
*       1  - on error
*
*******************************************************************************/
GT_STATUS mvHwsIpcCtrlMsgTx
(
    MV_HWS_IPC_CTRL_MSG_STRUCT *txCtrlMsg
)
{
    /* check that the message is valid: msg type and queueId are in the defined range */
    if ((txCtrlMsg->ctrlMsgType >= MV_HWS_IPC_LAST_CTRL_MSG_TYPE ) ||
         (txCtrlMsg->msgQueueId >= MV_HWS_MAX_HWS2HOST_REPLY_QUEUE_NUM))
    {
        mvHwsIpcStatistics[txCtrlMsg->devNum].mvHwsPortIpcFailureCount[txCtrlMsg->msgData.portGeneral.phyPortNum]++;
        osPrintf ("mvHwsIpcCtrlMsgTx wrong parameter msg type %d queue ID %d",txCtrlMsg->ctrlMsgType,txCtrlMsg->msgQueueId);
        return GT_BAD_PARAM;
    }

#ifdef MV_HWS_IPC_DEBUG
    {
    GT_U32 i;
    osPrintf ("txCtrlMsg->msgLength =  %d,txCtrlMsg->ctrlMsgType %x \n",txCtrlMsg->msgLength,txCtrlMsg->ctrlMsgType);
    for (i=0; i < txCtrlMsg->msgLength; i++)
        osPrintf ("txCtrlMsg->msgData.ctrlData[%d] = %x\n",i,txCtrlMsg->msgData.ctrlData[i]);
    /*Copy the data to msg queue that is placed in the Global Memory */
    Host2HostHwsIpcQueue[txCtrlMsg->devNum][txCtrlMsg->msgQueueId].devNum = txCtrlMsg->devNum;
    Host2HostHwsIpcQueue[txCtrlMsg->devNum][txCtrlMsg->msgQueueId].msgLength = txCtrlMsg->msgLength;
    Host2HostHwsIpcQueue[txCtrlMsg->devNum][txCtrlMsg->msgQueueId].ctrlMsgType = txCtrlMsg->ctrlMsgType;
    Host2HostHwsIpcQueue[txCtrlMsg->devNum][txCtrlMsg->msgQueueId].msgQueueId  = txCtrlMsg->msgQueueId;
    hwsOsMemCopyFuncPtr (Host2HostHwsIpcQueue[txCtrlMsg->devNum][txCtrlMsg->msgQueueId].msgData.ctrlData,txCtrlMsg->msgData.ctrlData,txCtrlMsg->msgLength);
    }
#endif /* MV_HWS_IPC_DEBUG*/
    /* update counter */
    mvHwsIpcStatistics[txCtrlMsg->devNum].mvHwsHostTxMsgCount[txCtrlMsg->ctrlMsgType]++;

    /* send msg to Service CPU*/

    return GT_OK;
}
/*******************************************************************************
* mvHwsSetIpcInfo
*
* DESCRIPTION:
*       writes IPC data to message structure
*
*
* INPUTS:
*       devNum     - system device number
*       msgDataPtr - pointer to message data
*       msgLength  - message length
*       msgType    - message type
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       0  - on success
*       1  - on error
*
*******************************************************************************/
GT_STATUS mvHwsSetIpcInfo
(
    GT_U8                                   devNum,
    MV_HWS_IPC_CTRL_MSG_DEF_TYPE            msgType,
    MV_HWS_IPC_CTRL_MSG_STRUCT              *msgDataPtr,
    GT_U32                                  msgLength
)
{
    GT_U32 queueId;

    msgDataPtr->devNum = devNum;
    msgDataPtr->msgLength =  (GT_U8)msgLength;
    msgDataPtr->ctrlMsgType = msgType;
    CHECK_STATUS(mvHwsGetQueueId(devNum, &queueId));
    msgDataPtr->msgQueueId =  (GT_U8)queueId;

    return GT_OK;
}

/*******************************************************************************
* mvHwsIpcReplyMsgRx
*
* DESCRIPTION:
*       Gets reply from HW Services to the Host
*
* INPUTS:
*       devNum      - system device number
*       queueId     - queue ID
*       msgId       - message ID
*       rxReplyData - pointer to message
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       0  - on success
*       1  - on error
*
*******************************************************************************/
GT_STATUS mvHwsIpcReplyMsgRx
(
    GT_U8                                   devNum,
    MV_HWS_IPC_CTRL_MSG_DEF_TYPE            msgId,
    GT_U32                                  queueId,
    MV_HWS_IPC_REPLY_MSG_STRUCT             *rxReplyData
)
{
#ifdef MV_HWS_IPC_DEBUG /* for debug only */
    {
	GT_U32 i;
	hwsOsMemCopyFuncPtr (rxReplyData->readData.resultData,Host2HostHwsReply[devNum][queueId].readData.resultData,MV_HWS_MAX_IPC_REPLY_LENGTH);
    rxReplyData->returnCode = Host2HostHwsReply[devNum][queueId].returnCode;
    rxReplyData->replyTo = Host2HostHwsReply[devNum][queueId].replyTo;
    for (i=0; i < MV_HWS_MAX_IPC_REPLY_LENGTH; i++)
        osPrintf ("rxReplyData->readData.resultData[%d] = %x\n",i,rxReplyData->readData.resultData[i]);
	osPrintf ("rxReplyData->returnCode =%x \n",rxReplyData->returnCode);
    osPrintf ("rxReplyData->replyTo =%x \n",rxReplyData->replyTo);
    }
#endif
    /* wait for message from Service CPU*/

    /* free the queue */
    CHECK_STATUS(mvHwsReturnQueueId (devNum,queueId));

    if (rxReplyData->replyTo != msgId)
    {
        mvHwsIpcStatistics[devNum].mvHwsIpcGenFailureCount++;
        osPrintf ("mvHwsIpceplyMsgRx wrong msg ID %d queue ID %d",msgId,queueId);
        return GT_BAD_PARAM;
    }

    /* updte statistics*/
    mvHwsIpcStatistics[devNum].mvHwsHostRxMsgCount[msgId]++;

    return GT_OK;
}


/*******************************************************************************
* mvHwsIpcSendRequestAndGetReply
*
* DESCRIPTION:
*       Gets reply from HW Services to the Host
*
* INPUTS:
*       requestMsg  - pointer to request message
*       replyData   - pointer to reply message
*       msgId       - message ID
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       0  - on success
*       1  - on errorGT_U8 devNum,
                           MV_HWS_IPC_CTRL_MSG_STRUCT *msgDataPtr,
                           GT_U32 msgLength,

*
*******************************************************************************/
GT_STATUS mvHwsIpcSendRequestAndGetReply
(
    GT_U8                                   devNum,
    MV_HWS_IPC_CTRL_MSG_STRUCT              *requestMsg,
    MV_HWS_IPC_REPLY_MSG_STRUCT             *replyData,
    GT_U32                                  msgLength,
    MV_HWS_IPC_CTRL_MSG_DEF_TYPE            msgId
)
{
    /* set ipc info */
    CHECK_STATUS(mvHwsSetIpcInfo (devNum,msgId,requestMsg, msgLength));
    /* send IPC message */
    CHECK_STATUS(mvHwsIpcCtrlMsgTx (requestMsg));

#ifdef MV_HWS_IPC_DEBUG /* for debug only read and print msg from global memory*/
    {
    MV_HWS_IPC_CTRL_MSG_STRUCT readMsg;
    mvHwsIpcCtrlMsgRx (requestMsg->msgQueueId,&readMsg);
    }
#endif
    /* wait for reply */
    CHECK_STATUS(mvHwsIpcReplyMsgRx (requestMsg->devNum,msgId,requestMsg->msgQueueId,replyData));
    return GT_OK;

}

#endif

#ifdef MV_HWS_IPC_DEBUG
/*******************************************************************************
* mvHwsIpcCtrlMsgRxSimulation
*
* DESCRIPTION:
*       Simulate results of IPC message receiving from host.
*
* INPUTS:
*       readMsg   - pointer to message
*       reply     - pointer to reply
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       0  - on success
*       1  - on error
*
*******************************************************************************/
static GT_VOID mvHwsIpcCtrlMsgRxSimulation
(
    MV_HWS_IPC_CTRL_MSG_STRUCT *readMsg,
    MV_HWS_IPC_REPLY_MSG_STRUCT *reply
)
{
    GT_U32   i;

    switch (readMsg->ctrlMsgType)
	{
    case MV_HWS_IPC_PORT_INIT_MSG:
		reply->returnCode = GT_OK;
		break;
	case MV_HWS_IPC_PORT_AUTO_TUNE_STATE_CHK_MSG:
        reply->returnCode = GT_OK;
        reply->readData.portAutoTuneStateChk.rxTune = TUNE_PASS;
        reply->readData.portAutoTuneStateChk.txTune = TUNE_PASS;
		break;
    case MV_HWS_IPC_PORT_AUTO_TUNE_SET_EXT_MSG:
        reply->returnCode = GT_OK;
        break;
    case MV_HWS_IPC_PORT_AUTO_TUNE_STOP_MSG:
        reply->returnCode = GT_OK;
        break;
    case MV_HWS_IPC_PORT_SERDES_MANUAL_RX_CONFIG_MSG:
        reply->returnCode = GT_OK;
        break;
    case MV_HWS_IPC_PORT_SERDES_MANUAL_TX_CONFIG_MSG:
        reply->returnCode = GT_OK;
        break;
    case MV_HWS_IPC_PORT_POLARITY_SET_MSG:
        reply->returnCode = GT_OK;
        break;
    case MV_HWS_IPC_PORT_FEC_CONFIG_MSG:
        reply->returnCode = GT_OK;
        break;
    case MV_HWS_IPC_PORT_FEC_CONFIG_GET_MSG:
        reply->returnCode = GT_OK;
        reply->readData.portStatusGet.status = GT_TRUE;
        break;
	case MV_HWS_IPC_PORT_LINK_STATUS_GET_MSG:
        reply->returnCode = GT_OK;
        reply->readData.portStatusGet.status = GT_TRUE;
        break;
    case MV_HWS_IPC_PORT_TX_ENABLE_MSG:
        reply->returnCode = GT_OK;
        break;
    case MV_HWS_IPC_PORT_LOOPBACK_SET_MSG:
        reply->returnCode = GT_OK;
        break;
    case  MV_HWS_IPC_PORT_LOOPBACK_STATUS_GET_MSG:
        reply->returnCode = GT_OK;
        reply->readData.portLoopbackStatusGet.lbType = DISABLE_LB;
        break;
    case MV_HWS_IPC_PORT_PPM_SET_MSG:
        reply->returnCode = GT_OK;
        break;
    case  MV_HWS_IPC_PORT_PPM_GET_MSG:
        reply->returnCode = GT_OK;
        reply->readData.portLoopbackStatusGet.lbType = DISABLE_LB;
        break;
    case  MV_HWS_IPC_PORT_IF_GET_MSG:
        reply->returnCode = GT_OK;
        reply->readData.portIfGet.portIf = SGMII;
        break;
    case  MV_HWS_IPC_PORT_TX_ENABLE_GET_MSG:
        reply->returnCode = GT_OK;
        for (i = 0; i < MV_HWS_MAX_LANES_NUM_PER_PORT; i++)
            reply->readData.portSerdesTxEnableGet.status[i] = GT_TRUE;
        break;
    case MV_HWS_IPC_PORT_SIGNAL_DETECT_GET_MSG:
        reply->returnCode = GT_OK;
        for (i = 0; i < MV_HWS_MAX_LANES_NUM_PER_PORT; i++)
            reply->readData.portSerdesSignalDetectGet.status[i] = GT_TRUE;
        break;
    case MV_HWS_IPC_PORT_CDR_LOCK_STATUS_GET_MSG:
        reply->returnCode = GT_OK;
        for (i = 0; i < MV_HWS_MAX_LANES_NUM_PER_PORT; i++)
            reply->readData.portSerdesCdrLockStatusGet.status[i] = GT_TRUE;
        break;
    default:
        reply->returnCode = GT_NOT_SUPPORTED;

   }
}
#endif
/*******************************************************************************
* mvHwsIpcCtrlMsgRx
*
* DESCRIPTION:
*       Gets IPC message from host.
*
* INPUTS:
*       queueId   - queue ID
*       readMsg   - pointer to message
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       0  - on success
*       1  - on error
*
*******************************************************************************/
GT_STATUS mvHwsIpcCtrlMsgRx
(
    GT_U32                      queueId,
    MV_HWS_IPC_CTRL_MSG_STRUCT *readMsg
)
{
    MV_HWS_IPC_REPLY_MSG_STRUCT reply;

#ifdef MV_HWS_IPC_DEBUG
    {
    GT_U32   i;

    readMsg->devNum      = Host2HostHwsIpcQueue[0][queueId].devNum;
    readMsg->msgLength   = Host2HostHwsIpcQueue[readMsg->devNum][queueId].msgLength;
    readMsg->ctrlMsgType = Host2HostHwsIpcQueue[readMsg->devNum][queueId].ctrlMsgType;
    readMsg->msgQueueId  = (GT_U8)queueId;
    hwsOsMemCopyFuncPtr (readMsg->msgData.ctrlData,Host2HostHwsIpcQueue[readMsg->devNum][queueId].msgData.ctrlData,readMsg->msgLength);
    osPrintf ("readMsg->msgLength =  %d,readMsg->ctrlMsgType %x \n",readMsg->msgLength,readMsg->ctrlMsgType);
    for (i=0; i < readMsg->msgLength; i++)
        osPrintf ("readMsg->msgData.ctrlData[%d] = %x\n",i,readMsg->msgData.ctrlData[i]);
    }
#endif /* MV_HWS_IPC_DEBUG */
    /* call IPC rx function to read the request*/
    /* check that the message is valid: msg type and queueId are in the defined range */
    if ((readMsg->ctrlMsgType >= MV_HWS_IPC_LAST_CTRL_MSG_TYPE ) ||
         (readMsg->msgQueueId >= MV_HWS_MAX_HWS2HOST_REPLY_QUEUE_NUM))
    {
#ifdef MV_HWS_HWS_IPC_STATISTICS_SERVICE_CPU_DEBUG
        mvHwsSrvCpuGenFailureCount++;
#endif /*MV_HWS_HWS_IPC_STATISTICS_SERVICE_CPU_DEBUG*/
        reply.returnCode = GT_BAD_PARAM;
        return GT_BAD_PARAM;
    }

#ifdef MV_HWS_HWS_IPC_STATISTICS_SERVICE_CPU_DEBUG
    mvHwsSrvCpuRxMsgCount[readMsg->ctrlMsgType]++;
#endif /*MV_HWS_HWS_IPC_STATISTICS_SERVICE_CPU_DEBUG*/
	reply.replyTo = readMsg->ctrlMsgType;
	/*	Decodes received IPC message */
#ifdef MV_HWS_IPC_DEBUG
    mvHwsIpcCtrlMsgRxSimulation (readMsg,&reply);
#else /* not MV_HWS_IPC_DEBUG*/
    switch (readMsg->ctrlMsgType)
    {
    case MV_HWS_IPC_PORT_INIT_MSG:
    {
        MV_HWS_IPC_PORT_INIT_DATA_STRUCT *msgParams;
        msgParams = &readMsg->msgData.portInit;
        reply.returnCode = mvHwsPortInit(readMsg->devNum,
                                         msgParams->portGroup,
                                         msgParams->phyPortNum,
                                         msgParams->portMode,
                                         msgParams->lbPort,
                                         msgParams->refClock,
                                         msgParams->refClockSource);
        break;
    }
    case MV_HWS_IPC_PORT_AUTO_TUNE_STATE_CHK_MSG:
    {
        MV_HWS_IPC_PORT_INFO_STRUCT     *msgParams;
        msgParams = &readMsg->msgData.portAutoTuneStateChk;
        reply.returnCode = mvHwsPortAutoTuneStateCheck(readMsg->devNum,
                                                       msgParams->portGroup,
                                                       msgParams->phyPortNum,
                                                       msgParams->portMode,
                                                       &reply.readData.portAutoTuneStateChk.rxTune,
                                                       &reply.readData.portAutoTuneStateChk.txTune);
        break;
    }
    case MV_HWS_IPC_PORT_AUTO_TUNE_SET_EXT_MSG:
    {
        MV_HWS_IPC_PORT_AUTO_TUNE_SET_EXT_DATA_STRUCT     *msgParams;
        msgParams = &readMsg->msgData.portAutoTuneSetExt;
        reply.returnCode = mvHwsPortAutoTuneSetExt(readMsg->devNum,
                                                   msgParams->portGroup,
                                                   msgParams->phyPortNum,
                                                   msgParams->portMode,
                                                   msgParams->portTuningMode,
                                                   msgParams->optAlgoMask,
                                                   &reply.readData.portAutoTuneSetExt.results);
        break;
    }
    case MV_HWS_IPC_PORT_AUTO_TUNE_STOP_MSG:
    {
        MV_HWS_IPC_PORT_AUTO_TUNE_STOP_DATA_STRUCT     *msgParams;
        msgParams = &readMsg->msgData.portAutoTuneStop;
        reply.returnCode = mvHwsPortAutoTuneStop(readMsg->devNum,
                                                 msgParams->portGroup,
                                                 msgParams->phyPortNum,
                                                 msgParams->portMode,
                                                 msgParams->stopRx,
                                                 msgParams->stopTx);
        break;
    }
    case MV_HWS_IPC_PORT_SERDES_MANUAL_RX_CONFIG_MSG:
     {
        MV_HWS_IPC_PORT_SERDES_MANUAL_RX_CONFIG_DATA_STRUCT  *msgParams;
        msgParams = &readMsg->msgData.serdesManualRxConfig;
        reply.returnCode = mvHwsSerdesManualRxConfig(readMsg->devNum,
                                               msgParams->portGroup,
                                               msgParams->phyPortNum,
                                               msgParams->portMode,
                                               msgParams->portTuningMode,
                                               msgParams->sqlch,
                                               msgParams->ffeRes,
                                               msgParams->ffeCap,
                                               msgParams->dfeEn,
                                               msgParams->alig);
        break;
    }
    case MV_HWS_IPC_PORT_SERDES_MANUAL_TX_CONFIG_MSG:
    {
        MV_HWS_IPC_PORT_SERDES_MANUAL_TX_CONFIG_DATA_STRUCT  *msgParams;
        msgParams = &readMsg->msgData.serdesManualTxConfig;
        reply.returnCode = mvHwsSerdesManualTxConfig(readMsg->devNum,
                                               msgParams->portGroup,
                                               msgParams->phyPortNum,
                                               msgParams->portMode,
                                               msgParams->portTuningMode,
                                               msgParams->txAmp,
                                               msgParams->txAmpAdj,
                                               msgParams->emph0,
                                               msgParams->emph1,
                                               msgParams->txAmpShft);
        break;
    }
    case MV_HWS_IPC_PORT_POLARITY_SET_MSG:
    {
        MV_HWS_IPC_PORT_POLARITY_SET_DATA_STRUCT     *msgParams;
        msgParams = &readMsg->msgData.portPolaritySet;
        reply.returnCode = mvHwsPortPolaritySet(readMsg->devNum,
                                                msgParams->portGroup,
                                                msgParams->phyPortNum,
                                                msgParams->portMode,
                                                msgParams->txInvMask,
                                                msgParams->rxInvMask);
        break;
    }
    case MV_HWS_IPC_PORT_FEC_CONFIG_MSG:
    {
        MV_HWS_IPC_PORT_FEC_CONFIG_DATA_STRUCT     *msgParams;
        msgParams = &readMsg->msgData.portFecConfig;
        reply.returnCode = mvHwsPortFecCofig(readMsg->devNum,
                                             msgParams->portGroup,
                                             msgParams->phyPortNum,
                                             msgParams->portMode,
                                             msgParams->portFecEn);
        break;
    }
    case MV_HWS_IPC_PORT_FEC_CONFIG_GET_MSG:
    {
        MV_HWS_IPC_PORT_INFO_STRUCT     *msgParams;
        msgParams = &readMsg->msgData.portFecConfigGet;
        reply.returnCode = mvHwsPortFecCofigGet(readMsg->devNum,
                                                msgParams->portGroup,
                                                msgParams->phyPortNum,
                                                msgParams->portMode,
                                                &reply.readData.portStatusGet.status);
        break;
    }
    case MV_HWS_IPC_PORT_LINK_STATUS_GET_MSG:
    {
        MV_HWS_IPC_PORT_INFO_STRUCT         *msgParams;
        msgParams = &readMsg->msgData.portLinkStatus;
        reply.returnCode = mvHwsPortLinkStatusGet(readMsg->devNum,
                                                  msgParams->portGroup,
                                                  msgParams->phyPortNum,
                                                  msgParams->portMode,
                                                  &reply.readData.portStatusGet.status);
        break;
    }
    case MV_HWS_IPC_PORT_TX_ENABLE_MSG:
    {
        MV_HWS_IPC_PORT_TX_ENABLE_DATA_STRUCT         *msgParams;
        msgParams = &readMsg->msgData.portTxEnableData;
        reply.returnCode = mvHwsPortTxEnable(readMsg->devNum,
                                             msgParams->portGroup,
                                             msgParams->phyPortNum,
                                             msgParams->portMode,
                                             msgParams->enable);
        break;
    }
    case  MV_HWS_IPC_PORT_TX_ENABLE_GET_MSG:
    {
        MV_HWS_IPC_PORT_INFO_STRUCT         *msgParams;
        msgParams = &readMsg->msgData.portTxEnableGet;
        reply.returnCode = mvHwsPortTxEnableGet(readMsg->devNum,
                                                msgParams->portGroup,
                                                msgParams->phyPortNum,
                                                msgParams->portMode,
                                                &replyData.readData.portSerdesTxEnableGet.status);
        break;
    }
    case MV_HWS_IPC_PORT_LOOPBACK_SET_MSG:
    {
        MV_HWS_IPC_PORT_LOOPBACK_SET_DATA_STRUCT         *msgParams;
        msgParams = &readMsg->msgData.portLoopbackSet;
        reply.returnCode = mvHwsPortLoopbackSet(readMsg->devNum,
                                                msgParams->portGroup,
                                                msgParams->phyPortNum,
                                                msgParams->portMode,
                                                msgParams->lpPlace,
                                                msgParams->lbType);
        break;
    }
    case  MV_HWS_IPC_PORT_LOOPBACK_STATUS_GET_MSG:
    {
        MV_HWS_IPC_PORT_LOOPBACK_GET_DATA_STRUCT         *msgParams;
        msgParams = &readMsg->msgData.portLoopbackGet;
        reply.returnCode = mvHwsPortLoopbackStatusGet(readMsg->devNum,
                                                      msgParams->portGroup,
                                                      msgParams->phyPortNum,
                                                      msgParams->portMode,
                                                      msgParams->lpPlace,
                                                      &reply.readData.portLoopbackStatusGet.lbType);
        break;
    }
    case MV_HWS_IPC_PORT_PPM_SET_MSG:
    {
        MV_HWS_IPC_PORT_PPM_SET_DATA_STRUCT         *msgParams;
        msgParams = &readMsg->msgData.portPPMSet;
        reply.returnCode = mvHwsPortPPMSet(readMsg->devNum,
                                           msgParams->portGroup,
                                           msgParams->phyPortNum,
                                           msgParams->portMode,
                                           msgParams->portPPM);

        break;
    }
    case  MV_HWS_IPC_PORT_PPM_GET_MSG:
    {
        MV_HWS_IPC_PORT_INFO_STRUCT         *msgParams;
        msgParams = &readMsg->msgData.portPPMGet;
        reply.returnCode = mvHwsPortPPMGet(readMsg->devNum,
                                           msgParams->portGroup,
                                           msgParams->phyPortNum,
                                           msgParams->portMode,
                                           &reply.readData.portPpmGet.portPpm);
        break;
    }
    case  MV_HWS_IPC_PORT_IF_GET_MSG:
    {
        MV_HWS_IPC_PORT_IF_GET_DATA_STRUCT         *msgParams;
        msgParams = &readMsg->msgData.portInterfaceGet;
        reply.returnCode = mvHwsPortInterfaceGet(readMsg->devNum,
                                                 msgParams->portGroup,
                                                 msgParams->phyPortNum,
                                                 &reply.readData.portIfGet.portIf);
        break;
    }
    case MV_HWS_IPC_PORT_SIGNAL_DETECT_GET_MSG:
        {
            MV_HWS_IPC_PORT_INFO_STRUCT         *msgParams;
            msgParams = &readMsg->msgData.portSignalDetectGet;
            reply.returnCode = mvHwsPortSignalDetectGet(readMsg->devNum,
                                                        msgParams->portGroup,
                                                        msgParams->phyPortNum,
                                                        msgParams->portMode,
                                                        &replyData.readData.portSerdesSignalDetectGet.status);
            break;
        }
    case MV_HWS_IPC_PORT_CDR_LOCK_STATUS_GET_MSG:
        {
            MV_HWS_IPC_PORT_INFO_STRUCT         *msgParams;
            msgParams = &readMsg->msgData.portSignalDetectGet;
            reply.returnCode = mvHwsPortCdrLockStatusGet(readMsg->devNum,
                                                         msgParams->portGroup,
                                                         msgParams->phyPortNum,
                                                         msgParams->portMode,
                                                         &replyData.readData.portSerdesCdrLockStatusGet.status);
            break;
        }

   	default:
		reply.returnCode = GT_NOT_SUPPORTED;
	}

#endif /*MV_HWS_IPC_DEBUG*/
	/* Construct IPC replay message with HWS API return code and returned information. */
	CHECK_STATUS(mvHwsIpcReplyMsgTx (readMsg->devNum,queueId,&reply));

#ifdef MV_HWS_HWS_IPC_STATISTICS_SERVICE_CPU_DEBUG
    mvHwsSrvCpuTxMsgCount[readMsg->ctrlMsgType]++;
#endif

	return GT_OK;
}

/*******************************************************************************
* mvHwsIpcReplyMsgTx
*
* DESCRIPTION:
*       Sends reply from HW Services to the Host
*
* INPUTS:
*       devNum     - system device number
*       queueId    - queue ID
*       txReplyMsg - pointer to message
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       0  - on success
*       1  - on error
*
*******************************************************************************/
GT_STATUS mvHwsIpcReplyMsgTx
(
    GT_U32                      devNum,
    GT_U32                      queueId,
    MV_HWS_IPC_REPLY_MSG_STRUCT *txReplyMsg
)
{

    if (hwsDeviceSpecInfo[devNum].ipcConnType != HOST2SERVICE_CPU_IPC_CONNECTION)
    {
        return GT_NOT_SUPPORTED;
    }
#ifdef MV_HWS_IPC_DEBUG /* for debug only */
    {
    GT_U32 i;

    for (i=0; i < MV_HWS_MAX_IPC_REPLY_LENGTH; i++)
        osPrintf ("txReplyMsg->readData.resultData[%d] = %x\n",i,txReplyMsg->readData.resultData[i]);
    osPrintf ("txReplyMsg->returnCode =%x \n",txReplyMsg->returnCode);
    hwsOsMemCopyFuncPtr (Host2HostHwsReply[devNum][queueId].readData.resultData,txReplyMsg->readData.resultData,MV_HWS_MAX_IPC_REPLY_LENGTH);
    Host2HostHwsReply[devNum][queueId].replyTo = txReplyMsg->replyTo;
    Host2HostHwsReply[devNum][queueId].returnCode = txReplyMsg->returnCode;
   }
#endif
    /* send msg to Host CPU*/

    return GT_OK;

}

