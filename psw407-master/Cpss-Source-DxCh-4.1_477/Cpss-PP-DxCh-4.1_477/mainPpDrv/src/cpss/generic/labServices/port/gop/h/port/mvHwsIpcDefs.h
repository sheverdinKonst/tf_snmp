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
* mvHwsIpcDefs.h
*
* DESCRIPTION:
*       Definitions for HWS IPS feature
*
* DEPENDENCIES:
*
* FILE REVISION NUMBER:
*       $Revision: 1 $
******************************************************************************/

#ifndef __mvHwsIpcDefs_H
#define __mvHwsIpcDefs_H

#ifdef __cplusplus
extern "C" {
#endif

#include <serdes/mvHwsSerdesPrvIf.h>

/**************************** Definition *************************************************/
#define MV_HWS_IPC_DEBUG
#define MV_HWS_IPC_MAX_MESSAGE_LENGTH               40
#define MV_HWS_IPC_PORT_GEN_DATA_LENGTH             16 /* general data includes 4 parameters :
                                                          devNum;
                                                          portGroup;
                                                          phyPortNum;
                                                          portMode;*/

#define MV_HWS_IPC_PORT_MISC_DATA_LENGTH            (MV_HWS_IPC_MAX_MESSAGE_LENGTH - MV_HWS_IPC_PORT_GEN_DATA_LENGTH)

#define MV_HWS_MAX_IPC_REPLY_LENGTH                 40 /* the number should be updated after all APIs are implemented
                                                          = max length of reply data - now max is:
                                                          status per serdes in port = GT_BOOL*MV_HWS_MAX_LANES_NUM_PER_PORT */

#define MV_HWS_MAX_HOST2HWS_REQ_MSG_NUM             4
#define MV_HWS_MAX_HWS2HOST_REPLY_QUEUE_NUM         MV_HWS_MAX_HOST2HWS_REQ_MSG_NUM

#define MV_HWS_IPC_FREE_QUEUE                       0
#define MV_HWS_IPC_QUEUE_BUSY                       0xff

#define MV_HWS_IPC_MAX_PORT_NUM                     72

#define MW_HWS_WRONG_IPC_MSG_TYPE                   0xF0

#define MV_HWS_MAX_LANES_NUM_PER_PORT               HWS_MAX_SERDES_NUM

typedef enum
{
    MV_HWS_IPC_HIGH_PRI_QUEUE,
    MV_HWS_IPC_LOW_PRI_QUEUE

}MV_HWS_IPC_PRIORITY_QUEUE;

typedef enum
{
    MV_HWS_IPC_PORT_INIT_MSG,
    MV_HWS_IPC_PORT_SERDES_RESET_MSG,
    MV_HWS_IPC_PORT_RESET_MSG,
    MV_HWS_IPC_PORT_RESET_EXT_MSG,
    MV_HWS_IPC_PORT_AUTO_TUNE_SET_EXT_MSG,
    MV_HWS_IPC_PORT_AUTO_TUNE_STOP_MSG,
    MV_HWS_IPC_PORT_AUTO_TUNE_STATE_CHK_MSG,
    MV_HWS_IPC_PORT_POLARITY_SET_MSG,
    MV_HWS_IPC_PORT_FEC_CONFIG_MSG,
    MV_HWS_IPC_PORT_FEC_CONFIG_GET_MSG,
    MV_HWS_IPC_PORT_LINK_STATUS_GET_MSG,
    MV_HWS_IPC_PORT_TX_ENABLE_MSG,
    MV_HWS_IPC_PORT_TX_ENABLE_GET_MSG,
    MV_HWS_IPC_PORT_SIGNAL_DETECT_GET_MSG,
    MV_HWS_IPC_PORT_CDR_LOCK_STATUS_GET_MSG,
    MV_HWS_IPC_PORT_LOOPBACK_SET_MSG,
    MV_HWS_IPC_PORT_LOOPBACK_STATUS_GET_MSG,
    MV_HWS_IPC_PORT_PPM_SET_MSG,
    MV_HWS_IPC_PORT_PPM_GET_MSG,
    MV_HWS_IPC_PORT_IF_GET_MSG,
    MV_HWS_IPC_PORT_SERDES_MANUAL_RX_CONFIG_MSG,
    MV_HWS_IPC_PORT_SERDES_MANUAL_TX_CONFIG_MSG,

    /* new messages */
    MV_HWS_IPC_PORT_AP_ENABLE_MSG,
    MV_HWS_IPC_PORT_AP_DISABLE_MSG,

    MV_HWS_IPC_LAST_CTRL_MSG_TYPE

}MV_HWS_IPC_CTRL_MSG_DEF_TYPE;

typedef struct
{
    GT_U32                  portGroup;
    GT_U32                  phyPortNum;
    MV_HWS_PORT_STANDARD    portMode;
}MV_HWS_IPC_PORT_INFO_STRUCT;              /* general structure for all get/check status functions*/

typedef struct
{
    GT_U32                      portGroup;
    GT_U32                      phyPortNum;
    MV_HWS_PORT_STANDARD        portMode;
    GT_U8                       portMiscData[MV_HWS_IPC_PORT_MISC_DATA_LENGTH];
}MV_HWS_IPC_PORT_GENERAL_STRUCT;

typedef struct
{
    GT_U32                  	portGroup;
    GT_U32                  	phyPortNum;
    MV_HWS_PORT_STANDARD    	portMode;
    GT_BOOL                 	lbPort;
    MV_HWS_REF_CLOCK_SUP_VAL 	refClock;
    MV_HWS_REF_CLOCK_SOURCE  	refClockSource;

}MV_HWS_IPC_PORT_INIT_DATA_STRUCT;

typedef struct
{
    GT_U32                  portGroup;
    GT_U32                  phyPortNum;
    MV_HWS_PORT_STANDARD    portMode;
    MV_HWS_PORT_ACTION      action;

}MV_HWS_IPC_PORT_RESET_DATA_STRUCT;

typedef struct
{
    GT_U32                  	portGroup;
    GT_U32                  	phyPortNum;
    MV_HWS_PORT_STANDARD    	portMode;
    MV_HWS_PORT_AUTO_TUNE_MODE  portTuningMode;
    GT_U32                      optAlgoMask;

}MV_HWS_IPC_PORT_AUTO_TUNE_SET_EXT_DATA_STRUCT;

typedef struct
{
    GT_U32                  portGroup;
    GT_U32                  phyPortNum;
    MV_HWS_PORT_STANDARD    portMode;
    GT_BOOL                 stopRx;
    GT_BOOL                 stopTx;

}MV_HWS_IPC_PORT_AUTO_TUNE_STOP_DATA_STRUCT;



typedef struct
{
	GT_U32                  portGroup;
	GT_U32                  phyPortNum;
	MV_HWS_PORT_STANDARD    portMode;
	GT_32                   txInvMask;
	GT_32                   rxInvMask;

}MV_HWS_IPC_PORT_POLARITY_SET_DATA_STRUCT;

typedef struct
{
	GT_U32                  portGroup;
	GT_U32                  phyPortNum;
	MV_HWS_PORT_STANDARD    portMode;
	GT_BOOL                 enable;

}MV_HWS_IPC_PORT_TX_ENABLE_DATA_STRUCT;

typedef struct
{
    GT_U32                  portGroup;
    GT_U32                  phyPortNum;
    MV_HWS_PORT_STANDARD    portMode;
    GT_BOOL                 portFecEn;

}MV_HWS_IPC_PORT_FEC_CONFIG_DATA_STRUCT;

typedef struct
{
	GT_U32                  portGroup;
	GT_U32                  phyPortNum;
	MV_HWS_PORT_STANDARD    portMode;
	MV_HWS_UNIT             lpPlace;
	MV_HWS_PORT_LB_TYPE     lbType;

}MV_HWS_IPC_PORT_LOOPBACK_SET_DATA_STRUCT;


typedef struct
{
	GT_U32                  portGroup;
	GT_U32                  phyPortNum;
	MV_HWS_PORT_STANDARD    portMode;
	MV_HWS_UNIT             lpPlace;

}MV_HWS_IPC_PORT_LOOPBACK_GET_DATA_STRUCT;

typedef struct
{
	GT_U32                  portGroup;
	GT_U32                  phyPortNum;
	MV_HWS_PORT_STANDARD    portMode;
	MV_HWS_PPM_VALUE	    portPPM;

}MV_HWS_IPC_PORT_PPM_SET_DATA_STRUCT;

typedef struct
{
    GT_U32                  portGroup;
    GT_U32                  phyPortNum;

}MV_HWS_IPC_PORT_IF_GET_DATA_STRUCT;

typedef struct
{
    GT_U32                      portGroup;
    GT_U32                      serdesNum;
    MV_HWS_PORT_MAN_TUNE_MODE   portTuningMode;
    GT_U32                      sqlch;
    GT_U32                      ffeRes;
    GT_U32                      ffeCap;
    GT_BOOL                     dfeEn;
    GT_U32                      alig;

}MV_HWS_IPC_PORT_SERDES_MANUAL_RX_CONFIG_DATA_STRUCT;

typedef struct
{
    GT_U32              portGroup;
    GT_U32              serdesNum;
    GT_U32              txAmp;
    GT_BOOL             txAmpAdj;
    GT_U32              emph0;
    GT_U32              emph1;
    GT_BOOL             txAmpShft;

}MV_HWS_IPC_PORT_SERDES_MANUAL_TX_CONFIG_DATA_STRUCT;

typedef union
{
    GT_U8                                               ctrlData[MV_HWS_IPC_MAX_MESSAGE_LENGTH];
    MV_HWS_IPC_PORT_GENERAL_STRUCT                      portGeneral;
    MV_HWS_IPC_PORT_INIT_DATA_STRUCT                    portInit;
    MV_HWS_IPC_PORT_RESET_DATA_STRUCT                   portReset;
    MV_HWS_IPC_PORT_AUTO_TUNE_SET_EXT_DATA_STRUCT       portAutoTuneSetExt;
    MV_HWS_IPC_PORT_AUTO_TUNE_STOP_DATA_STRUCT          portAutoTuneStop;
    MV_HWS_IPC_PORT_INFO_STRUCT                         portAutoTuneStateChk;
    MV_HWS_IPC_PORT_POLARITY_SET_DATA_STRUCT            portPolaritySet;
    MV_HWS_IPC_PORT_TX_ENABLE_DATA_STRUCT               portTxEnableData;
    MV_HWS_IPC_PORT_INFO_STRUCT                         portTxEnableGet;
    MV_HWS_IPC_PORT_INFO_STRUCT                         portSignalDetectGet;
    MV_HWS_IPC_PORT_INFO_STRUCT                         portCdrLockStatus;
    MV_HWS_IPC_PORT_FEC_CONFIG_DATA_STRUCT              portFecConfig;
    MV_HWS_IPC_PORT_INFO_STRUCT                         portFecConfigGet;
    MV_HWS_IPC_PORT_INFO_STRUCT                         portLinkStatus;
    MV_HWS_IPC_PORT_LOOPBACK_SET_DATA_STRUCT            portLoopbackSet;
    MV_HWS_IPC_PORT_LOOPBACK_GET_DATA_STRUCT            portLoopbackGet;
    MV_HWS_IPC_PORT_PPM_SET_DATA_STRUCT                 portPPMSet;
    MV_HWS_IPC_PORT_INFO_STRUCT                         portPPMGet;
    MV_HWS_IPC_PORT_IF_GET_DATA_STRUCT                  portInterfaceGet;
    MV_HWS_IPC_PORT_SERDES_MANUAL_RX_CONFIG_DATA_STRUCT serdesManualRxConfig;
    MV_HWS_IPC_PORT_SERDES_MANUAL_TX_CONFIG_DATA_STRUCT serdesManualTxConfig;

}MV_HWS_IPC_MSG_API_PARAMS;


typedef struct
{
    MV_HWS_AUTO_TUNE_STATUS rxTune;
    MV_HWS_AUTO_TUNE_STATUS txTune;
}MV_HWS_IPC_PORT_REPLY_AUTO_TUNE_STATE_CHK;

typedef struct
{
    GT_BOOL                 status;

}MV_HWS_IPC_PORT_REPLY_STATUS_GET;

typedef struct
{
    GT_BOOL                 status[MV_HWS_MAX_LANES_NUM_PER_PORT];

}MV_HWS_IPC_PORT_REPLY_PER_SERDES_STATUS_GET;



typedef struct
{
    GT_U32                 results;

}MV_HWS_IPC_PORT_REPLY_AUTO_TUNE_SET_EXT;

typedef struct
{
    MV_HWS_PORT_LB_TYPE     lbType;

}MV_HWS_IPC_PORT_REPLY_LOOPBACK_STATUS_GET;

typedef struct
{
    MV_HWS_PPM_VALUE portPpm;

}MV_HWS_IPC_PORT_REPLY_PPM_GET;


typedef struct
{
    MV_HWS_PORT_STANDARD portIf;

}MV_HWS_IPC_PORT_REPLY_IF_GET;

typedef union
{
    GT_U8                                           resultData[MV_HWS_MAX_IPC_REPLY_LENGTH];
    MV_HWS_IPC_PORT_REPLY_AUTO_TUNE_STATE_CHK       portAutoTuneStateChk;
    MV_HWS_IPC_PORT_REPLY_STATUS_GET                portStatusGet;
    MV_HWS_IPC_PORT_REPLY_AUTO_TUNE_SET_EXT         portAutoTuneSetExt;
    MV_HWS_IPC_PORT_REPLY_LOOPBACK_STATUS_GET       portLoopbackStatusGet;
    MV_HWS_IPC_PORT_REPLY_PPM_GET                   portPpmGet;
    MV_HWS_IPC_PORT_REPLY_IF_GET                    portIfGet;
    MV_HWS_IPC_PORT_REPLY_PER_SERDES_STATUS_GET     portSerdesTxEnableGet;
    MV_HWS_IPC_PORT_REPLY_PER_SERDES_STATUS_GET     portSerdesSignalDetectGet;
    MV_HWS_IPC_PORT_REPLY_PER_SERDES_STATUS_GET     portSerdesCdrLockStatusGet;

}MV_HWS_IPC_REPLY_MSG_DATA_TYPE;

typedef struct
{
    GT_U8                      devNum;
    GT_U8                      ctrlMsgType;    /*message type */
    GT_U8                      msgQueueId;     /*queue Id defines message queue Id for reply*/
    GT_U8                      msgLength;      /*Message length*/
    MV_HWS_IPC_MSG_API_PARAMS  msgData;        /*Message content */

 }MV_HWS_IPC_CTRL_MSG_STRUCT;


typedef struct
{
    GT_U32                                     returnCode;
    MV_HWS_IPC_CTRL_MSG_DEF_TYPE               replyTo;
    MV_HWS_IPC_REPLY_MSG_DATA_TYPE             readData;

}MV_HWS_IPC_REPLY_MSG_STRUCT;

typedef struct
{
    GT_U32 mvHwsHostRxMsgCount[MV_HWS_IPC_LAST_CTRL_MSG_TYPE];
    GT_U32 mvHwsHostTxMsgCount[MV_HWS_IPC_LAST_CTRL_MSG_TYPE];

    GT_U32 mvHwsPortIpcFailureCount[MV_HWS_IPC_MAX_PORT_NUM];
    GT_U32 mvHwsIpcGenFailureCount;

}MV_HWS_IPC_STATISTICS_STRUCT;


/*******************************************************************************
* mvHwsIpcSendRequestAndGetReply
*
* DESCRIPTION:
*       Gets reply from HW Services to the Host
*
* INPUTS:
*       devNum      - device number
*       requestMsg  - pointer to request message
*       replyData   - pointer to reply message
*       msgLength   -  message length
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
);

#ifdef __cplusplus
}
#endif

#endif /* __mvHwsIpcDefs_H */


