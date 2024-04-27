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
* mvHwsHost2HwsIfWraper.c
*
* DESCRIPTION: Port extrenal interface
*
*
* DEPENDENCIES:
*
* FILE REVISION NUMBER:
*       $Revision: 55 $
******************************************************************************/

#include <common/siliconIf/mvSiliconIf.h>
#include <private/mvHwsPortPrvIf.h>
#include <private/mvHwsPortMiscIf.h>
#include <mvHwsIpcDefs.h>

#ifdef _VISUALC
#pragma warning( disable : 4204)
#endif
/**************************** Globals ****************************************************/



/**************************** Pre-Declaration ********************************************/

/*******************************************************************************
* mvHwsPortInitIpc
*
* DESCRIPTION:
*       Sends to HWS request to init physical port.
*
* INPUTS:
*       devNum    - system device number
*       portGroup - port group (core) number
*       phyPortNum - physical port number
*       portMode   - port standard metric
*       lbPort     - if true, init port without serdes activity
*       refClock   - Reference clock frequency
*       refClockSrc - Reference clock source line
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       0  - on success
*       1  - on error
*
*******************************************************************************/
GT_STATUS mvHwsPortInitIpc
(
    GT_U8                   devNum,
    GT_U32                  portGroup,
    GT_U32                  phyPortNum,
    MV_HWS_PORT_STANDARD    portMode,
    GT_BOOL                 lbPort,
    MV_HWS_REF_CLOCK_SUP_VAL refClock,
    MV_HWS_REF_CLOCK_SOURCE  refClockSource)
{
    MV_HWS_IPC_CTRL_MSG_STRUCT requestMsg;
    MV_HWS_IPC_REPLY_MSG_STRUCT replyData;
    MV_HWS_IPC_PORT_INIT_DATA_STRUCT portInit = {portGroup,phyPortNum,portMode,lbPort,refClock,refClockSource};

    /*construct the msg*/
    requestMsg.msgData.portInit = portInit;

    /* send request to HWS and wait for the reply */
    CHECK_STATUS(mvHwsIpcSendRequestAndGetReply (devNum,&requestMsg,&replyData,
                                                 sizeof(MV_HWS_IPC_PORT_INIT_DATA_STRUCT),
                                                 MV_HWS_IPC_PORT_INIT_MSG));

    return (replyData.returnCode);
}
/*******************************************************************************
* mvHwsPortResetIpc
*
* DESCRIPTION:
*       Sends to HWS request to power down or reset physical port.
*
* INPUTS:
*       devNum     - system device number
*       portGroup  - port group (core) number
*       phyPortNum - physical port number
*       portMode   - port standard metric
*       lbPort     - if true, init port without serdes activity
*       action     - Power down or reset
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       0  - on success
*       1  - on error
*
*******************************************************************************/
GT_STATUS mvHwsPortResetIpc
(
	GT_U8	devNum,
	GT_U32	portGroup,
	GT_U32	phyPortNum,
	MV_HWS_PORT_STANDARD	portMode,
	MV_HWS_PORT_ACTION	action
)
{
    MV_HWS_IPC_CTRL_MSG_STRUCT requestMsg;
    MV_HWS_IPC_REPLY_MSG_STRUCT replyData;
    MV_HWS_IPC_PORT_RESET_DATA_STRUCT portReset = {portGroup,phyPortNum,portMode,action};

    /*construct the msg*/
    requestMsg.msgData.portReset = portReset;

    /* send request to HWS and wait for the reply */
    CHECK_STATUS(mvHwsIpcSendRequestAndGetReply (devNum,&requestMsg,&replyData,
                                                 sizeof(MV_HWS_IPC_PORT_RESET_DATA_STRUCT),
                                                 MV_HWS_IPC_PORT_RESET_MSG));

    return (replyData.returnCode);
}


/*******************************************************************************
* mvHwsPortAutoTuneStateCheckIpc
*
* DESCRIPTION:
*
* INPUTS:
*       devNum    - system device number
*       portGroup - port group (core) number
*       phyPortNum - physical port number
*       portMode   - port standard metric
*       portTuningMode - port TX related tuning mode
*
* OUTPUTS:
*       Tuning results for recommended settings.(TBD)
*
* RETURNS:
*       0  - on success
*       1  - on error
*
*******************************************************************************/
GT_STATUS mvHwsPortAutoTuneStateCheckIpc
(
    GT_U8                   devNum,
    GT_U32                  portGroup,
    GT_U32                  phyPortNum,
    MV_HWS_PORT_STANDARD    portMode,
    MV_HWS_AUTO_TUNE_STATUS *rxTune,
    MV_HWS_AUTO_TUNE_STATUS *txTune
)
{
    MV_HWS_IPC_CTRL_MSG_STRUCT requestMsg;
    MV_HWS_IPC_REPLY_MSG_STRUCT replyData;
    MV_HWS_IPC_PORT_INFO_STRUCT  portAutoTuneStateChk = {portGroup,phyPortNum,portMode};

    requestMsg.msgData.portAutoTuneStateChk = portAutoTuneStateChk;

    /* send request to HWS and wait for the reply */
    CHECK_STATUS(mvHwsIpcSendRequestAndGetReply (devNum,&requestMsg,&replyData,
                                                 sizeof(MV_HWS_IPC_PORT_INFO_STRUCT),
                                                 MV_HWS_IPC_PORT_AUTO_TUNE_STATE_CHK_MSG));

    *rxTune = replyData.readData.portAutoTuneStateChk.rxTune;
    *txTune = replyData.readData.portAutoTuneStateChk.txTune;
    return (replyData.returnCode);
}
/*******************************************************************************
* mvHwsPortLinkStatusGet
*
* DESCRIPTION:
*       Returns port link status.
*       Can run at any time.
*
* INPUTS:
*       devNum    - system device number
*       portGroup - port group (core) number
*       phyPortNum - physical port number
*       portMode   - port standard metric
*
* OUTPUTS:
*       Port link UP status (true/false).
*
* RETURNS:
*       0  - on success
*       1  - on error
*
*******************************************************************************/
GT_STATUS mvHwsPortLinkStatusGetIpc
(
    GT_U8                   devNum,
    GT_U32                  portGroup,
    GT_U32                  phyPortNum,
    MV_HWS_PORT_STANDARD    portMode,
    GT_BOOL                 *linkStatus
)
{
    MV_HWS_IPC_CTRL_MSG_STRUCT requestMsg;
    MV_HWS_IPC_REPLY_MSG_STRUCT replyData;
    MV_HWS_IPC_PORT_INFO_STRUCT portLinkStatus = {portGroup,phyPortNum,portMode};

    /*construct the msg*/
    requestMsg.msgData.portLinkStatus = portLinkStatus;

   /* send request to HWS and wait for the reply */
   CHECK_STATUS(mvHwsIpcSendRequestAndGetReply (devNum,&requestMsg,&replyData,
                                                sizeof(MV_HWS_IPC_PORT_INFO_STRUCT),
                                                MV_HWS_IPC_PORT_LINK_STATUS_GET_MSG));

   *linkStatus = replyData.readData.portStatusGet.status;

   return (replyData.returnCode);
}
/*******************************************************************************
* mvHwsPortAutoTuneSetExtIpc
*
* DESCRIPTION:
*
* INPUTS:
*       devNum    - system device number
*       portGroup - port group (core) number
*       phyPortNum - physical port number
*       portMode   - port standard metric
*       portTuningMode - port TX related tuning mode
*       optAlgoMask - bit mask for optimization algorithms
*       portTuningMode - port tuning mode
*
* OUTPUTS:
*       Tuning results for recommended settings.
*
*
* RETURNS:
*       0  - on success
*       1  - on error
*
*******************************************************************************/
GT_STATUS mvHwsPortAutoTuneSetExtIpc
(
    GT_U8                           devNum,
    GT_U32                          portGroup,
    GT_U32                          phyPortNum,
    MV_HWS_PORT_STANDARD            portMode,
    MV_HWS_PORT_AUTO_TUNE_MODE      portTuningMode,
    GT_U32                          optAlgoMask,
    void                            *results
)
{
    MV_HWS_IPC_CTRL_MSG_STRUCT requestMsg;
    MV_HWS_IPC_REPLY_MSG_STRUCT replyData;
    MV_HWS_IPC_PORT_AUTO_TUNE_SET_EXT_DATA_STRUCT   portAutoTuneSetExt = {portGroup,phyPortNum,portMode,portTuningMode,optAlgoMask};;

     results = results; /* to avoid warning */

    /*construct the msg*/
    requestMsg.msgData.portAutoTuneSetExt = portAutoTuneSetExt;

    /* send request to HWS and wait for the reply */
    CHECK_STATUS(mvHwsIpcSendRequestAndGetReply (devNum,&requestMsg,&replyData,
                                                 sizeof(MV_HWS_IPC_PORT_AUTO_TUNE_SET_EXT_DATA_STRUCT),
                                                 MV_HWS_IPC_PORT_AUTO_TUNE_SET_EXT_MSG));
    return (replyData.returnCode);
}
/*******************************************************************************
* mvHwsPortAutoTuneStopIpc
*
* DESCRIPTION:
*       Send IPC message to stop Tx and Rx training
*
* INPUTS:
*       devNum      - system device number
*       portGroup   - port group (core) number
*       phyPortNum  - physical port number
*       portMode    - port standard metric
*       stopRx      - stop RX
*       stopTx      - stop Tx
*
* OUTPUTS:
*       None
*
*
* RETURNS:
*       0  - on success
*       1  - on error
*
*******************************************************************************/
GT_STATUS mvHwsPortAutoTuneStopIpc
(
    GT_U8                           devNum,
    GT_U32                          portGroup,
    GT_U32                          phyPortNum,
    MV_HWS_PORT_STANDARD            portMode,
	GT_BOOL                         stopRx,
	GT_BOOL                         stopTx
)
{
    MV_HWS_IPC_CTRL_MSG_STRUCT requestMsg;
    MV_HWS_IPC_REPLY_MSG_STRUCT replyData;
    MV_HWS_IPC_PORT_AUTO_TUNE_STOP_DATA_STRUCT portAutoTuneStop = {portGroup,phyPortNum,portMode,stopRx,stopTx};

    /*construct the msg*/
    requestMsg.msgData.portAutoTuneStop = portAutoTuneStop;

    /* send request to HWS and wait for the reply */
    CHECK_STATUS(mvHwsIpcSendRequestAndGetReply (devNum,&requestMsg,&replyData,
                                                 sizeof(MV_HWS_IPC_PORT_AUTO_TUNE_STOP_DATA_STRUCT),
                                                 MV_HWS_IPC_PORT_AUTO_TUNE_STOP_MSG));

    return (replyData.returnCode);
}


/*******************************************************************************
* mvHwsSerdesManualRxConfigIpc
*
* DESCRIPTION:
*   	Send IPC message to configure SERDES Rx parameters for all SERDES lanes.
*
* INPUTS:
*       devNum      - system device number
*       portGroup   - port group (core) number
*       phyPortNum  - physical port number
*       portMode    - port standard metric
*                     config params:
*                                  sqlch,ffeRes,ffeCap,dfeEn,alig
*
* OUTPUTS:
*       None
*
* RETURNS:
*       0  - on success
*       1  - on error
*******************************************************************************/
GT_STATUS mvHwsSerdesManualRxConfigIpc
(
    GT_U8                       devNum,
    GT_U32                      portGroup,
    GT_U32                      serdesNum,
    MV_HWS_PORT_MAN_TUNE_MODE   portTuningMode,
    GT_U32                      sqlch,
    GT_U32                      ffeRes,
    GT_U32                      ffeCap,
    GT_BOOL                     dfeEn,
    GT_U32                      alig
)
{
    MV_HWS_IPC_CTRL_MSG_STRUCT requestMsg;
    MV_HWS_IPC_REPLY_MSG_STRUCT replyData;

    MV_HWS_IPC_PORT_SERDES_MANUAL_RX_CONFIG_DATA_STRUCT        serdesManualRxConfig = {portGroup,serdesNum,portTuningMode,
                                                                                       sqlch,ffeRes,ffeCap,dfeEn,alig};

    /*construct the msg*/
    requestMsg.msgData.serdesManualRxConfig = serdesManualRxConfig;

   /* send request to HWS and wait for the reply */
   CHECK_STATUS(mvHwsIpcSendRequestAndGetReply (devNum,&requestMsg,&replyData,
                                                sizeof(MV_HWS_IPC_PORT_SERDES_MANUAL_RX_CONFIG_DATA_STRUCT),
                                                MV_HWS_IPC_PORT_SERDES_MANUAL_RX_CONFIG_MSG));

   return (replyData.returnCode);

}

/*******************************************************************************
* mvHwsSerdesManualTxConfigIpc
*
* DESCRIPTION:
*   	Send IPC message to configure SERDES Tx parameters for all SERDES lanes.
*
* INPUTS:
*       devNum      - system device number
*       portGroup   - port group (core) number
*       phyPortNum  - physical port number
*       portMode    - port standard metric
*                     config params:
*                                  txAmp,txAmpAdj,emph0,emph1,txAmpShft
*
* OUTPUTS:
*       None
*
* RETURNS:
*       0  - on success
*       1  - on error
*******************************************************************************/
GT_STATUS mvHwsSerdesManualTxConfigIpc
(
    GT_U8               devNum,
    GT_U32              portGroup,
    GT_U32              serdesNum,
    GT_U32              txAmp,
    GT_BOOL             txAmpAdj,
    GT_U32              emph0,
    GT_U32              emph1,
    GT_BOOL             txAmpShft
)
{
    MV_HWS_IPC_CTRL_MSG_STRUCT requestMsg;
    MV_HWS_IPC_REPLY_MSG_STRUCT replyData;

    MV_HWS_IPC_PORT_SERDES_MANUAL_TX_CONFIG_DATA_STRUCT serdesManualTxConfig = {portGroup,serdesNum,txAmp,
                                                                                txAmpAdj,emph0,emph1,txAmpShft};

    /*construct the msg*/
    requestMsg.msgData.serdesManualTxConfig = serdesManualTxConfig;

   /* send request to HWS and wait for the reply */
   CHECK_STATUS(mvHwsIpcSendRequestAndGetReply (devNum,&requestMsg,&replyData,
                                                sizeof(MV_HWS_IPC_PORT_SERDES_MANUAL_TX_CONFIG_DATA_STRUCT),
                                                MV_HWS_IPC_PORT_SERDES_MANUAL_TX_CONFIG_MSG));

   return (replyData.returnCode);

}

/*******************************************************************************
* mvHwsPortPolaritySetIpc
*
* DESCRIPTION:
*       Send message to set the port polarity of the Serdes lanes (Tx/Rx).
*
* INPUTS:
*       devNum    - system device number
*       portGroup - port group (core) number
*       phyPortNum - physical port number
*       portMode   - port standard metric
*       txInvMask  - bitmap of 32 bit, each bit represent Serdes
*       rxInvMask  - bitmap of 32 bit, each bit represent Serdes
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       0  - on success
*       1  - on error
*
*******************************************************************************/
GT_STATUS mvHwsPortPolaritySetIpc
(
	GT_U8   devNum,
	GT_U32  portGroup,
	GT_U32  phyPortNum,
	MV_HWS_PORT_STANDARD    portMode,
	GT_32   txInvMask,
	GT_32   rxInvMask
)
{
    MV_HWS_IPC_CTRL_MSG_STRUCT requestMsg;
    MV_HWS_IPC_REPLY_MSG_STRUCT replyData;
    MV_HWS_IPC_PORT_POLARITY_SET_DATA_STRUCT portPolaritySet = {portGroup,phyPortNum,portMode,txInvMask,rxInvMask};;

    /*construct the msg*/
    requestMsg.msgData.portPolaritySet = portPolaritySet;

    /* send request to HWS and wait for the reply */
    CHECK_STATUS(mvHwsIpcSendRequestAndGetReply (devNum,&requestMsg,&replyData,
                                                 sizeof(MV_HWS_IPC_PORT_POLARITY_SET_DATA_STRUCT),
                                                 MV_HWS_IPC_PORT_POLARITY_SET_MSG));

    return (replyData.returnCode);

}

/*******************************************************************************
* mvHwsPortFecConfigIpc
*
* DESCRIPTION:
*       Send message to configure FEC disable/enable on port.
*
* INPUTS:
*       devNum     - system device number
*       portGroup  - port group (core) number
*       phyPortNum - physical port number
*       portMode   - port standard metric
*       portFecEn  - GT_TRUE for FEC enable, GT_FALSE otherwise
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       0  - on success
*       1  - on error
*
*******************************************************************************/
GT_STATUS mvHwsPortFecConfigIpc
(
	GT_U8                   devNum,
	GT_U32                  portGroup,
	GT_U32                  phyPortNum,
	MV_HWS_PORT_STANDARD    portMode,
	GT_BOOL                 portFecEn

)
{
    MV_HWS_IPC_CTRL_MSG_STRUCT requestMsg;
    MV_HWS_IPC_REPLY_MSG_STRUCT replyData;
    MV_HWS_IPC_PORT_FEC_CONFIG_DATA_STRUCT portFecConfig = {portGroup,phyPortNum,portMode,portFecEn};

    /*construct the msg*/
    requestMsg.msgData.portFecConfig = portFecConfig;

    /* send request to HWS and wait for the reply */
    CHECK_STATUS(mvHwsIpcSendRequestAndGetReply (devNum,&requestMsg,&replyData,
                                                 sizeof(MV_HWS_IPC_PORT_FEC_CONFIG_DATA_STRUCT),
                                                 MV_HWS_IPC_PORT_FEC_CONFIG_MSG));

    return (replyData.returnCode);
}

/*******************************************************************************
* mvHwsPortFecConfigGetIpc
*
* DESCRIPTION:
*       Send message to get  FEC status.
*
* INPUTS:
*       devNum     - system device number
*       portGroup  - port group (core) number
*       phyPortNum - physical port number
*       portMode   - port standard metric
*
* OUTPUTS:
       FEC status (true/false).
*
* RETURNS:
*       0  - on success
*       1  - on error
*
*******************************************************************************/
GT_STATUS mvHwsPortFecConfigGetIpc
(
	GT_U8                   devNum,
	GT_U32                  portGroup,
	GT_U32                  phyPortNum,
	MV_HWS_PORT_STANDARD    portMode,
	GT_BOOL                 *portFecEn

)
{
    MV_HWS_IPC_CTRL_MSG_STRUCT requestMsg;
    MV_HWS_IPC_REPLY_MSG_STRUCT replyData;
    MV_HWS_IPC_PORT_INFO_STRUCT portFecConfigGet = {portGroup,phyPortNum,portMode};

    /*construct the msg*/
    requestMsg.msgData.portFecConfigGet = portFecConfigGet;

    /* send request to HWS and wait for the reply */
    CHECK_STATUS(mvHwsIpcSendRequestAndGetReply (devNum,&requestMsg,&replyData,
                                                 sizeof(MV_HWS_IPC_PORT_INFO_STRUCT),
                                                 MV_HWS_IPC_PORT_FEC_CONFIG_GET_MSG));

    *portFecEn = replyData.readData.portStatusGet.status;

    return (replyData.returnCode);
}

/*******************************************************************************
* mvHwsPortTxEnable
*
* DESCRIPTION:
*       Turn of the port Tx according to selection.
*
* INPUTS:
*       devNum    - system device number
*       portGroup - port group (core) number
*       phyPortNum - physical port number
*       portMode   - port standard metric
*       enable     - enable/disable port Tx
*
* OUTPUTS:
*       Tuning results for recommended settings.
*
* RETURNS:
*       0  - on success
*       1  - on error
*
*******************************************************************************/
GT_STATUS mvHwsPortTxEnableIpc
(
	GT_U8                   devNum,
	GT_U32                  portGroup,
	GT_U32                  phyPortNum,
	MV_HWS_PORT_STANDARD    portMode,
	GT_BOOL                 enable
)
{
    MV_HWS_IPC_CTRL_MSG_STRUCT              requestMsg;
    MV_HWS_IPC_REPLY_MSG_STRUCT             replyData;
    MV_HWS_IPC_PORT_TX_ENABLE_DATA_STRUCT   portTxEnableData = {portGroup,phyPortNum,portMode,enable};

    /*construct the msg*/
    requestMsg.msgData.portTxEnableData = portTxEnableData;

    /* send request to HWS and wait for the reply */
    CHECK_STATUS(mvHwsIpcSendRequestAndGetReply (devNum,&requestMsg,&replyData,
                                                 sizeof(MV_HWS_IPC_PORT_TX_ENABLE_DATA_STRUCT),
                                                 MV_HWS_IPC_PORT_TX_ENABLE_MSG));

    return (replyData.returnCode);
}

/*******************************************************************************
* mvHwsPortTxEnableGetIpc
*
* DESCRIPTION:
*       Retrieve the status of all port serdeses.
*
* INPUTS:
*       devNum    - system device number
*       portGroup - port group (core) number
*       phyPortNum - physical port number
*       portMode   - port standard metric
*
* OUTPUTS:
*       TxStatus per serdes.
*
* RETURNS:
*       0  - on success
*       1  - on error
*
*******************************************************************************/
GT_STATUS mvHwsPortTxEnableGetIpc
(
	GT_U8   devNum,
	GT_U32  portGroup,
	GT_U32  phyPortNum,
	MV_HWS_PORT_STANDARD    portMode,
	GT_BOOL serdesTxStatus[]
)
{
    MV_HWS_IPC_CTRL_MSG_STRUCT     requestMsg;
    MV_HWS_IPC_REPLY_MSG_STRUCT    replyData;
    MV_HWS_IPC_PORT_INFO_STRUCT    portTxEnableGet = {portGroup,phyPortNum,portMode};
    MV_HWS_PORT_INIT_PARAMS *curPortParams;
    GT_U32 curLanesList[HWS_MAX_SERDES_NUM];
    GT_U32 i;

    /*construct the msg*/
    requestMsg.msgData.portTxEnableGet = portTxEnableGet;

    /* send request to HWS and wait for the reply */
    CHECK_STATUS(mvHwsIpcSendRequestAndGetReply (devNum,&requestMsg,&replyData,
                                                 sizeof(MV_HWS_IPC_PORT_INFO_STRUCT),
                                                 MV_HWS_IPC_PORT_TX_ENABLE_GET_MSG));

    curPortParams = hwsPortModeParamsGet(devNum, portGroup, phyPortNum, portMode);
    /* rebuild active lanes list according to current configuration (redundancy) */
    CHECK_STATUS(mvHwsRebuildActiveLaneList(devNum, portGroup, phyPortNum, portMode, curLanesList));

    /* on each related serdes */
    for (i = 0; i < curPortParams->numOfActLanes; i++)
    {
        serdesTxStatus[i] = replyData.readData.portSerdesTxEnableGet.status[i];
    }

    return (replyData.returnCode);
}

/*******************************************************************************
* mvHwsPortSignalDetectGetIpc
*
* DESCRIPTION:
*       Retrieve the status of all port serdeses.
*
* INPUTS:
*       devNum    - system device number
*       portGroup - port group (core) number
*       phyPortNum - physical port number
*       portMode   - port standard metric
*
* OUTPUTS:
*       SignalDetect per serdes.
*
* RETURNS:
*       0  - on success
*       1  - on error
*
*******************************************************************************/
GT_STATUS mvHwsPortSignalDetectGetIpc
(
	GT_U8   devNum,
	GT_U32  portGroup,
	GT_U32  phyPortNum,
	MV_HWS_PORT_STANDARD    portMode,
	GT_BOOL signalDetect[]
)
{
    MV_HWS_IPC_CTRL_MSG_STRUCT     requestMsg;
    MV_HWS_IPC_REPLY_MSG_STRUCT    replyData;
    MV_HWS_IPC_PORT_INFO_STRUCT    portSignalDetectGet = {portGroup,phyPortNum,portMode};
    MV_HWS_PORT_INIT_PARAMS *curPortParams;
    GT_U32 curLanesList[HWS_MAX_SERDES_NUM];
    GT_U32 i;

    /*construct the msg*/
    requestMsg.msgData.portSignalDetectGet = portSignalDetectGet;

    /* send request to HWS and wait for the reply */
    CHECK_STATUS(mvHwsIpcSendRequestAndGetReply (devNum,&requestMsg,&replyData,
                                                 sizeof(MV_HWS_IPC_PORT_INFO_STRUCT),
                                                 MV_HWS_IPC_PORT_SIGNAL_DETECT_GET_MSG));

    curPortParams = hwsPortModeParamsGet(devNum, portGroup, phyPortNum, portMode);
    /* rebuild active lanes list according to current configuration (redundancy) */
    CHECK_STATUS(mvHwsRebuildActiveLaneList(devNum, portGroup, phyPortNum, portMode, curLanesList));

    /* on each related serdes */
    for (i = 0; i < curPortParams->numOfActLanes; i++)
    {
        signalDetect[i] = replyData.readData.portSerdesSignalDetectGet.status[i];
    }

    return (replyData.returnCode);
}


/*******************************************************************************
* mvHwsPortCdrLockStatusGetIpc
*
* DESCRIPTION:
*       Send message to get the CDR lock status of all port serdeses.
*
* INPUTS:
*       devNum    - system device number
*       portGroup - port group (core) number
*       phyPortNum - physical port number
*       portMode   - port standard metric
*
* OUTPUTS:
*       SignalDetect per serdes.
*
* RETURNS:
*       0  - on success
*       1  - on error
*
*******************************************************************************/
GT_STATUS mvHwsPortCdrLockStatusGetIpc
(
	GT_U8   devNum,
	GT_U32  portGroup,
	GT_U32  phyPortNum,
	MV_HWS_PORT_STANDARD    portMode,
	GT_BOOL cdrLockStatus[]
)
{
    MV_HWS_IPC_CTRL_MSG_STRUCT     requestMsg;
    MV_HWS_IPC_REPLY_MSG_STRUCT    replyData;
    MV_HWS_IPC_PORT_INFO_STRUCT    portCdrLockStatus = {portGroup,phyPortNum,portMode};
    MV_HWS_PORT_INIT_PARAMS *curPortParams;
    GT_U32 curLanesList[HWS_MAX_SERDES_NUM];
    GT_U32 i;

    /*construct the msg*/
    requestMsg.msgData.portCdrLockStatus = portCdrLockStatus;

    /* send request to HWS and wait for the reply */
    CHECK_STATUS(mvHwsIpcSendRequestAndGetReply (devNum,&requestMsg,&replyData,
                                                 sizeof(MV_HWS_IPC_PORT_INFO_STRUCT),
                                                 MV_HWS_IPC_PORT_CDR_LOCK_STATUS_GET_MSG));

    curPortParams = hwsPortModeParamsGet(devNum, portGroup, phyPortNum, portMode);
    /* rebuild active lanes list according to current configuration (redundancy) */
    CHECK_STATUS(mvHwsRebuildActiveLaneList(devNum, portGroup, phyPortNum, portMode, curLanesList));

    /* on each related serdes */
    for (i = 0; i < curPortParams->numOfActLanes; i++)
    {
        cdrLockStatus[i] = replyData.readData.portSerdesCdrLockStatusGet.status[i];
    }

    return (replyData.returnCode);
}



/*******************************************************************************
* mvHwsPortLoopbackSetIpc
*
* DESCRIPTION:
*       Send message to activates the port loopback modes.
*
* INPUTS:
*       devNum    - system device number
*       portGroup - port group (core) number
*       phyPortNum - physical port number
*       portMode   - port standard metric
*       lpPlace    - unit for loopback configuration
*       lpType     - loopback type
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       0  - on success
*       1  - on error

*******************************************************************************/
GT_STATUS mvHwsPortLoopbackSetIpc
(
	GT_U8                   devNum,
	GT_U32                  portGroup,
	GT_U32                  phyPortNum,
	MV_HWS_PORT_STANDARD    portMode,
	MV_HWS_UNIT             lpPlace,
	MV_HWS_PORT_LB_TYPE     lbType
)
{
    MV_HWS_IPC_CTRL_MSG_STRUCT              requestMsg;
    MV_HWS_IPC_REPLY_MSG_STRUCT             replyData;
    MV_HWS_IPC_PORT_LOOPBACK_SET_DATA_STRUCT   portLoopbackSet = {portGroup,phyPortNum,portMode,lpPlace,lbType};

    /*construct the msg*/
    requestMsg.msgData.portLoopbackSet = portLoopbackSet;

    /* send request to HWS and wait for the reply */
    CHECK_STATUS(mvHwsIpcSendRequestAndGetReply (devNum,&requestMsg,&replyData,
                                                 sizeof(MV_HWS_IPC_PORT_LOOPBACK_SET_DATA_STRUCT),
                                                 MV_HWS_IPC_PORT_LOOPBACK_SET_MSG));

    return (replyData.returnCode);
}

/*******************************************************************************
* mvHwsPortLoopbackStatusGetIpc
*
* DESCRIPTION:
*       Send IPC message to retrive MAC loopback status.
*
* INPUTS:
*       devNum    - system device number
*       portGroup - port group (core) number
*       phyPortNum - physical port number
*       portMode   - port standard metric
*       lpPlace    - unit for loopback configuration
*
* OUTPUTS:
*       lbType    - supported loopback type
*
* RETURNS:
*       0  - on success
*       1  - on error
*
*******************************************************************************/
GT_STATUS mvHwsPortLoopbackStatusGetIpc
(
	GT_U8                   devNum,
	GT_U32                  portGroup,
	GT_U32                  phyPortNum,
	MV_HWS_PORT_STANDARD    portMode,
	MV_HWS_UNIT             lpPlace,
	MV_HWS_PORT_LB_TYPE     *lbType
)
{
    MV_HWS_IPC_CTRL_MSG_STRUCT                  requestMsg;
    MV_HWS_IPC_REPLY_MSG_STRUCT                 replyData;
    MV_HWS_IPC_PORT_LOOPBACK_GET_DATA_STRUCT    portLoopbackGet = {portGroup,phyPortNum,portMode,lpPlace};;

    /*construct the msg*/
    requestMsg.msgData.portLoopbackGet = portLoopbackGet;

    /* send request to HWS and wait for the reply */
    CHECK_STATUS(mvHwsIpcSendRequestAndGetReply (devNum,&requestMsg,&replyData,
                                                 sizeof(MV_HWS_IPC_PORT_LOOPBACK_GET_DATA_STRUCT),
                                                 MV_HWS_IPC_PORT_LOOPBACK_STATUS_GET_MSG));

     *lbType = replyData.readData.portLoopbackStatusGet.lbType;

     return (replyData.returnCode);
}

/*******************************************************************************
* mvHwsPortPPMSetIpc
*
* DESCRIPTION:
*       Send IPC message to increase/decrease  Tx clock on port (added/sub ppm).
*       Can be run only after create port not under traffic.
*
* INPUTS:
*       devNum    - system device number
*       portGroup - port group (core) number
*       phyPortNum - physical port number
*       portMode   - port standard metric
*       portPPM    - limited to +/- 3 taps
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       0  - on success
*       1  - on error
*
*******************************************************************************/
GT_STATUS mvHwsPortPPMSetIpc
(
	GT_U8                   devNum,
	GT_U32                  portGroup,
	GT_U32                  phyPortNum,
	MV_HWS_PORT_STANDARD    portMode,
	MV_HWS_PPM_VALUE	    portPPM
)
{
    MV_HWS_IPC_CTRL_MSG_STRUCT                  requestMsg;
    MV_HWS_IPC_REPLY_MSG_STRUCT                 replyData;
    MV_HWS_IPC_PORT_PPM_SET_DATA_STRUCT         portPPMSet = {portGroup,phyPortNum,portMode,portPPM};

    /*construct the msg*/
    requestMsg.msgData.portPPMSet = portPPMSet;

    /* send request to HWS and wait for the reply */
    CHECK_STATUS(mvHwsIpcSendRequestAndGetReply (devNum,&requestMsg,&replyData,
                                                 sizeof(MV_HWS_IPC_PORT_PPM_SET_DATA_STRUCT),
                                                 MV_HWS_IPC_PORT_PPM_SET_MSG));

    return (replyData.returnCode);
}

/*******************************************************************************
* mvHwsPortPPMGetIpc
*
* DESCRIPTION:
*       Send message to check the entire line configuration
*
* INPUTS:
*       devNum    - system device number
*       portGroup - port group (core) number
*       phyPortNum - physical port number
*       portMode   - port standard metric
*
* OUTPUTS:
*       portPPM    - current PPM
*
* RETURNS:
*       0  - on success
*       1  - on error
*
*******************************************************************************/
GT_STATUS mvHwsPortPPMGetIpc
(
	GT_U8                   devNum,
	GT_U32                  portGroup,
	GT_U32                  phyPortNum,
	MV_HWS_PORT_STANDARD    portMode,
	MV_HWS_PPM_VALUE        *portPPM
)
{
    MV_HWS_IPC_CTRL_MSG_STRUCT      requestMsg;
    MV_HWS_IPC_REPLY_MSG_STRUCT     replyData;
    MV_HWS_IPC_PORT_INFO_STRUCT     portPPMGet = {portGroup,phyPortNum,portMode};

    /*construct the msg*/
    requestMsg.msgData.portPPMGet = portPPMGet;

    /* send request to HWS and wait for the reply */
    CHECK_STATUS(mvHwsIpcSendRequestAndGetReply (devNum,&requestMsg,&replyData,
                                                 sizeof(MV_HWS_IPC_PORT_INFO_STRUCT),
                                                 MV_HWS_IPC_PORT_PPM_GET_MSG));

    *portPPM = replyData.readData.portPpmGet.portPpm;

    return (replyData.returnCode);
}

/*******************************************************************************
* mvHwsPortInterfaceGetIpc
*
* DESCRIPTION:
*       Send message to gets Interface mode and speed of a specified port.
*
*
* INPUTS:
*       devNum   - physical device number
*       portGroup - core number
*       phyPortNum  - physical port number (or CPU port)
*
* OUTPUTS:
*       portModePtr - interface mode
*
* RETURNS:
*       GT_OK   		 - on success
*       GT_BAD_PARAM    	 - on wrong port number or device
*       GT_BAD_PTR      	 - one of the parameters is NULL pointer
*       GT_HW_ERROR     	 - on hardware error
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS mvHwsPortInterfaceGetIpc
(
	GT_U8	                devNum,
	GT_U32	                portGroup,
	GT_U32	                phyPortNum,
	MV_HWS_PORT_STANDARD    *portModePtr
)
{
    MV_HWS_IPC_CTRL_MSG_STRUCT      requestMsg;
    MV_HWS_IPC_REPLY_MSG_STRUCT     replyData;
    MV_HWS_IPC_PORT_IF_GET_DATA_STRUCT   portInterfaceGet = {portGroup,phyPortNum};

    /*construct the msg*/
    requestMsg.msgData.portInterfaceGet = portInterfaceGet;

    /* send request to HWS and wait for the reply */
    CHECK_STATUS(mvHwsIpcSendRequestAndGetReply (devNum,&requestMsg,&replyData,
                                                 sizeof(MV_HWS_IPC_PORT_IF_GET_DATA_STRUCT),
                                                 MV_HWS_IPC_PORT_IF_GET_MSG));

    *portModePtr = replyData.readData.portIfGet.portIf;

    return (replyData.returnCode);
}


