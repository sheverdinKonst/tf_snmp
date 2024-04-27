/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* sRemoteTm.c
*
* DESCRIPTION:
*       The file to handle connection to remote TM (traffic manager) device (simulation of TM)
*
* DEPENDENCIES:
*
* FILE REVISION NUMBER:
*       $Revision: 2 $
*******************************************************************************/
#include <os/simTypesBind.h>
#include <asicSimulation/SCIB/scib.h>
#include <asicSimulation/SKernel/smain/smain.h>
#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SKernel/smain/sRemoteTm.h>

#define DUMMY_UDP_PORT_AS_SERVER_CNS 8888
/* indication to use info from INI file or from the caller */
#define FROM_INI_FILE   0

/* macro that create info in network order from host order , and vice versa */
#define SIM_HTONL(charPtr)  ((((GT_U8*)charPtr)[0] << 24) |   \
                             (((GT_U8*)charPtr)[1] << 16) |   \
                             (((GT_U8*)charPtr)[2] << 8)  |   \
                             (((GT_U8*)charPtr)[3]))

/* macro for function parseBufferToStructure*/
#define PTR_AND_SIZE_MAC(stcName)    (GT_VOID*)(&(stcName)) , sizeof(stcName)

/* define most common use of delay that we use , where need to wait for change
   of state without the use of semaphores , and without doing 'Busy wait' */
#define WAIT_SLEEP_TIME_CNS             500

/* max number of bytes in buffer ,
  for rx/tx frames use dynamic malloc when needed */
#define MAX_BUFFER_SIZE_CNS     (64*1024)

/* max number of bytes in buffer , for reply */
#define MAX_BUFFER_SIZE_SINGLE_MESSAGE_CNS     (2*1024)

#define LOG_SOCKET_ID_RECEIVE(sockId)

static GT_U32   allowPrint = 0;
#define PRINTK_PARAM(string , param) if(allowPrint)  printf("%s : %s = [0x%x]\n",string,#param,param)
#define printk(x)       if(allowPrint)  printf x

static GT_SIZE dummySizeTypeSize = 0;
/* debug function to start/stop print of messages */
extern void sRemoteTmAllowPrint(IN GT_U32   allow)
{
    allowPrint = allow;
}

#define STR(strname)    \
    #strname

static char* PPMsim_msg_type_Names[] = {
    STR(PP_TMSIM_DROP_QUERY),             /* PP to TMsim */
    STR(PP_TMSIM_DROP_RESPONSE),          /* TMsim to PP */

    STR(PP_TMSIM_START_PACKET_PROCESS),   /* PP to TMsim */
    STR(PP_TMSIM_PACKET_DONE),            /* TMsim to PP */

    STR(PP_TMSIM_REGISTER_WRITE),         /* PP to TMsim */
    STR(PP_TMSIM_REGISTER_READ),          /* PP to TMsim */
    STR(PP_TMSIM_REGISTER_READ_RESPONSE), /* TMsim to PP */

    STR(PP_TMSIM_ACK_RESPONSE) /* PP to TMsim & TMsim to PP. Send only if ack_needed not 0 */
};

/*
 * Typedef: struct TM_SOCKET_INFO_STC
 *
 * Description:
 *      Describe the socket info
 *
 * Fields:
 *      serverReady : is server ready ?
 *      mySockId : the socket that client use to 'select'
 *                 and also send messages to the server using 'sendTo'
 *      serverProtocolPort : the UDP port that the SERVER bind() to.
 *                     for 'client side' this is the UDP port that the 'server side'
 *                     is connected to.
 *
 *                     for 'server side' this is the 'own PORT' that connected to.
 *                     the server reply to messages from a client via 'senderInfo'
 *                      see senderInfo.tmSockAddr
 *
 *      serverSockAddr     : A pointer to a socket address structure, represents the
 *                     remote address to send messages to.
 *      serverIpAddressStr : string that hold the IP address of the 'SERVER' sockets in format of "192.21.246.32"
 * Comments:
 */
typedef struct{
    GT_U32          serverReady;
    GT_SOCKET_FD    mySockId;

    GT_U16          serverProtocolPort;
    GT_VOID *       serverSockAddr;

    GT_VOID *       localSockAddr;

    GT_CHAR         serverIpAddressStr[SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS];

    struct{
        GT_VOID* tmSockAddr;/*A pointer to a socket address structure, represents
                the source (target) address that has sent the packet.
                the caller can then compare the address later.*/
    }senderInfo;


}TM_SOCKET_INFO_STC;

typedef enum{
    INDEX_TRAFFIC_DRIVEN_E      = 0,  /* processing of traffic driven actions from PP to TM : send packet header , drop query ,
                               processing of traffic driven actions from TM to PP : drop response , packet processing ended */
    INDEX_CPU_ACCESS_E   ,     /* processing of cpu access driven action from PP to TM : write register , read register query
                               processing of cpu access driven action from TM to PP: read register response */
    INDEX__LAST__E
}INDEX_TYPE_E;

static TM_SOCKET_INFO_STC socketInfoArr[INDEX__LAST__E];
#if FROM_INI_FILE
/* the name of the section [tm_connection] */
static char iniFileTmSectionStr[] = "tm_connection";
static char iniFileTmSection_ipAddr[] = "tm_ip_addr";
#endif /*FROM_INI_FILE*/
/* receive (listening) socket - udp port */
static char* iniFileTmSection_udpPort[INDEX__LAST__E] = {
    "tm_udp_port_traffic",  /* processing of traffic driven actions from PP to TM : send packet header , drop query ,
                               processing of traffic driven actions from TM to PP : drop response , packet processing ended */
    "tm_udp_port_cpu_access"/* processing of cpu access driven action from PP to TM : write register , read register query
                               processing of cpu access driven action from TM to PP: read register response */
};
/* a string that represent IP internal loopback */
static char loopbackIpStr[]="127.0.0.1";

/*
 * Typedef: struct DISTRIBUTED_MESSAGE_SYNC_INFO_STC
 *
 * Description:
 *      Describe the info for synchronize the send of "get" and the "reply"
 *      for it
 *
 * Fields:
 *      semSync : semaphores to synchronize the sender of message and the reply
 *                  for it
 *      getReplyMutex : 'mutext' to protect 2 thread try to do same type of
 *                  message sending
 *      replyDevice : the device that got the reply
 *      info: the inf received from the TM device.
 *          union of the types
 * Comments:
 */
typedef struct{
    GT_SEM      semSync;
    GT_MUTEX    getReplyMutex;
    GT_U32      replySimDeviceId;

    union{
        /* info got from TM and saved for use of waiting task */
        PPTMsim_drop_response       dropResponse;
        PPTMsim_packet_done_header  packetDoneHeader;
        PPTMsim_register            readRegisterResponse;
    }info;
}TM_DISTRIBUTED_MESSAGE_SYNC_INFO_STC;

typedef enum {
    TM_MSG_FROM_TM_TO_PP_DROP_RESPONSE_E,
    TM_MSG_FROM_TM_TO_PP_PACKET_DONE_E,
    TM_MSG_FROM_TM_TO_PP_REGISTER_READ_RESPONSE_E,

    TM_MSG_FROM_TM_TO_PP_ACK_E,/* ACK from TM to PP */

    TM_MSG_FROM_TM_TO_PP__LAST__E
}TM_MSG_FROM_TM_TO_PP_ENT;

/* array of semaphores to synchronize the sender of message and the reply for it*/
static TM_DISTRIBUTED_MESSAGE_SYNC_INFO_STC tmDistributedMessageSync[TM_MSG_FROM_TM_TO_PP__LAST__E] =
{
    {0/*,0,0,{{0}}*/}
};

typedef struct{
    PPTMsim_header      messageHeader;
    PPTMsim_register    registerInfo;
}REAGISTER_FULL_INFO_STC;


typedef struct{
    PPTMsim_header      messageHeader;
    PPTMsim_packet_header    packetHeaderInfo;
}PACKET_HEADER_FULL_INFO_STC;

typedef struct{
    PPTMsim_header      messageHeader;
    PPTMsim_drop_query  dropQueryInfo;
}PACKET_DROP_QUERY_FULL_INFO_STC;

/* callback function for 'packet done' */
static SREMOTE_TM_RECEIVED_PACKET_DONE_FROM_TM_CALLBACK_FUNC *sRemoteTmPacketDoneCbPtr = NULL;

/* indication that we work with remote TM memory managers */
GT_U32   sRemoteTmUsed = 0;

/*******************************************************************************
*  sRemoteTmBindCallBack
*
* DESCRIPTION:
*      register callback function(s) to the engine
*
* INPUTS:
*       packetDoneCbPtr - callback for 'packet done' message
*
* OUTPUTS:
*
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void sRemoteTmBindCallBack
(
    IN SREMOTE_TM_RECEIVED_PACKET_DONE_FROM_TM_CALLBACK_FUNC *packetDoneCbPtr
)
{
    sRemoteTmPacketDoneCbPtr = packetDoneCbPtr;
}

/*******************************************************************************
* parseBufferToStructure
*
* DESCRIPTION:
*       parse the header reply info from the buffer.
*       note : this doing "network order to host order" conversion
*
* INPUTS:
*       buffPtr - pointer to the header in the buffer to parse
*       stcPtr - pointer to the Structure
*       stcSize - size of the Structure (in bytes)
*
* OUTPUTS:
*       stcPtr - pointer to the Structure with the info
*
* RETURNS:
*       none
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static void parseBufferToStructure
(
    IN GT_U8    *buffPtr,
    INOUT GT_VOID *stcPtr,
    IN GT_U32   stcSize
)
{
    GT_U32  ii;

    for(ii = 0 ; ii < ((stcSize+3) / 4) ; ii++)
    {
        /* copy data from the buffer */
        ((GT_U32*)stcPtr)[ii] = SIM_HTONL(&buffPtr[4*ii]);
    }

    return;
}

/*******************************************************************************
* buildStructureToBuffer
*
* DESCRIPTION:
*       parse the header reply info from the buffer.
*       note : this doing "network order to host order" conversion
*
* INPUTS:
*       stcPtr - pointer to the Structure
*       stcSize - size of the Structure (in bytes)
*       buffPtr - pointer to the built buffer
*
* OUTPUTS:
*       buffPtr - pointer to the built buffer with info
*
* RETURNS:
*       none
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static void buildStructureToBuffer
(
    IN GT_VOID *stcPtr,
    IN GT_U32   stcSize,
    INOUT GT_U8    *buffPtr
)
{
    GT_U32  ii;
    GT_U32  *u32buffPtr;
    GT_U8   *u8StcPtr;

    u32buffPtr = (GT_U32*)(void*)buffPtr;
    u8StcPtr   = stcPtr;

    for(ii = 0 ; ii < ((stcSize+3)/4) ; ii++)
    {
        /* copy data from the structure to the buffer */
        ((GT_U32*)u32buffPtr)[ii] = SIM_HTONL(&u8StcPtr[4*ii]);
    }

    return;
}

/*******************************************************************************
* processAck
*
* DESCRIPTION:
*       function process an 'ACK' message.
*
* INPUTS:
*       simDeviceId - Simulation device ID.
* OUTPUTS:
*
*
* RETURNS:
*       none
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static void processAck(
    IN GT_U32          simDeviceId
)
{
    TM_DISTRIBUTED_MESSAGE_SYNC_INFO_STC *messageSyncPtr =
        &tmDistributedMessageSync[TM_MSG_FROM_TM_TO_PP_ACK_E];

    messageSyncPtr->replySimDeviceId = simDeviceId;

    /* now we can wake the waiting task */
    SIM_OS_MAC(simOsSemSignal)(messageSyncPtr->semSync);

    return;
}

/*******************************************************************************
* processRegisterReadResponse
*
* DESCRIPTION:
*       function process a message with the read register values , and put the data and
*       signal the waiting task
*
* INPUTS:
*       simDeviceId - Simulation device ID.
*       specificDataPtr - pointer to the specific info
* OUTPUTS:
*
*
* RETURNS:
*       none
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static void processRegisterReadResponse(
    IN GT_U32          simDeviceId,
    IN GT_U8*          specificDataPtr
)
{
    TM_DISTRIBUTED_MESSAGE_SYNC_INFO_STC *messageSyncPtr =
        &tmDistributedMessageSync[TM_MSG_FROM_TM_TO_PP_REGISTER_READ_RESPONSE_E];

    messageSyncPtr->replySimDeviceId = simDeviceId;

    /* parse the read reply info from the buffer.
       note : this doing "network order to host order" conversion */
    parseBufferToStructure(specificDataPtr,
        PTR_AND_SIZE_MAC(messageSyncPtr->info.readRegisterResponse));

    /* now we can wake the waiting task */
    SIM_OS_MAC(simOsSemSignal)(messageSyncPtr->semSync);

    return;
}

/*******************************************************************************
* processPacketDoneResponse
*
* DESCRIPTION:
*       function process a message with the 'packet done' values, and put the data and
*       signal the waiting task
*
* INPUTS:
*       simDeviceId - Simulation device ID.
*       specificDataPtr - pointer to the specific info
* OUTPUTS:
*
*
* RETURNS:
*       none
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static void processPacketDoneResponse(
    IN GT_U32          simDeviceId,
    IN GT_U8*          specificDataPtr
)
{
    TM_DISTRIBUTED_MESSAGE_SYNC_INFO_STC *messageSyncPtr =
        &tmDistributedMessageSync[TM_MSG_FROM_TM_TO_PP_PACKET_DONE_E];
    GT_VOID*    cookiePtr;

    messageSyncPtr->replySimDeviceId = simDeviceId;

    /* parse the read reply info from the buffer.
       note : this doing "network order to host order" conversion */
    parseBufferToStructure(specificDataPtr,
        PTR_AND_SIZE_MAC(messageSyncPtr->info.packetDoneHeader));

    /* we need to call according to the callback to the handling device */
    if(sRemoteTmPacketDoneCbPtr == NULL)
    {
        skernelFatalError(" processPacketDoneResponse : sRemoteTmPacketDoneCbPtr == NULL \n");
    }

    /* copy the cookie from 'packet Id' */
    memcpy(&cookiePtr ,
            &messageSyncPtr->info.packetDoneHeader.packet_id ,
            sizeof(cookiePtr));

    (*sRemoteTmPacketDoneCbPtr)(simDeviceId,cookiePtr,messageSyncPtr->info.packetDoneHeader.egress_port);

    return;
}

/*******************************************************************************
* processDropResponse
*
* DESCRIPTION:
*       function process a message with the 'drop response' values, and put the data and
*       signal the waiting task
*
* INPUTS:
*       simDeviceId - Simulation device ID.
*       specificDataPtr - pointer to the specific info
* OUTPUTS:
*
*
* RETURNS:
*       none
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static void processDropResponse(
    IN GT_U32          simDeviceId,
    IN GT_U8*          specificDataPtr
)
{
    TM_DISTRIBUTED_MESSAGE_SYNC_INFO_STC *messageSyncPtr =
        &tmDistributedMessageSync[TM_MSG_FROM_TM_TO_PP_DROP_RESPONSE_E];

    messageSyncPtr->replySimDeviceId = simDeviceId;

    /* parse the read reply info from the buffer.
       note : this doing "network order to host order" conversion */
    parseBufferToStructure(specificDataPtr,
        PTR_AND_SIZE_MAC(messageSyncPtr->info.dropResponse));

    /* now we can wake the waiting task */
    SIM_OS_MAC(simOsSemSignal)(messageSyncPtr->semSync);

    return;
}

/*******************************************************************************
*  ppSendTmClientInfo
*
* DESCRIPTION:
*      send to the TM server the udp port to reply to .
*
* INPUTS:
*       simDeviceId - Simulation device ID.
*       indexType   - index type
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void ppSendTmClientInfo(
    IN GT_U32               simDeviceId,
    IN INDEX_TYPE_E         indexType
)
{
    GT_U64 address;
    GT_U64 value;

    if(indexType == INDEX_CPU_ACCESS_E)
    {
        address.l[0] = 0;
        address.l[1] = 0;
        value.l[0]   = 0;
        value.l[1]   = 0;

        /* send first dummy message to the server */
        sRemoteTmRegisterWrite(simDeviceId,&address,&value);
    }
}

/*******************************************************************************
* sendToTm
*
* DESCRIPTION:
*       Send data to a TM over the UDP socket
*
* INPUTS:
*       socketInfoPtr - pointer to socket info
*       isReply     - GT_TRUE - reply for message that TM sent.
*                     GT_FALSE - new message that we send to TM.
*                     this parameter indicate which sockAddr to use.
*       buff        - Buffer to send
*       buffLen     - Max length to send
*
* OUTPUTS:
*       None
*
* RETURNS:
*       None
*
* COMMENTS:
*       done under SCIB_SEM_TAKE,SCIB_SEM_SIGNAL
*
*******************************************************************************/
static void sendToTm(
    IN  TM_SOCKET_INFO_STC *socketInfoPtr,
    IN GT_BOOL         isReply,
    IN  GT_VOID *      buff,
    IN  GT_SIZE        buffLen
)
{
    GT_U8  networkOrderBuff[MAX_BUFFER_SIZE_SINGLE_MESSAGE_CNS];
    TM_DISTRIBUTED_MESSAGE_SYNC_INFO_STC *messageSyncPtr;

    /* convert CPU order STC to network order bytes */
    buildStructureToBuffer(buff,buffLen,networkOrderBuff);

    /*SCIB_SEM_TAKE;*/

    SIM_OS_MAC(osSocketSendTo)(socketInfoPtr->mySockId,
            networkOrderBuff,
            buffLen,
            (isReply == GT_TRUE) ? socketInfoPtr->senderInfo.tmSockAddr    : socketInfoPtr->serverSockAddr ,
            dummySizeTypeSize
            );

    /*SCIB_SEM_SIGNAL;*/

    if(isReply == GT_FALSE)
    {
        /* wait for ACK */
        messageSyncPtr = &tmDistributedMessageSync[TM_MSG_FROM_TM_TO_PP_ACK_E];
        SIM_OS_MAC(simOsSemWait)(messageSyncPtr->semSync,
            socketInfoPtr->serverReady ?
                SIM_OS_WAIT_FOREVER :
                (2*WAIT_SLEEP_TIME_CNS));/* the init will retry but will print "." */
    }
}

/*******************************************************************************
* sendAckToSender
*
* DESCRIPTION:
*       send 'ACK' to the sender of the message.
*       use the buffer of the message itself
*
* INPUTS:
*       socketInfoPtr  - current socket info.
*       messageHeaderPtr - header of the original message that we got from TM
* OUTPUTS:
*
*
* RETURNS:
*       none
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static void sendAckToSender(
    IN TM_SOCKET_INFO_STC *socketInfoPtr,
    IN PPTMsim_header  *messageHeaderPtr
)
{
    /* update the message type to be 'ACK' */
    messageHeaderPtr->message_type = PP_TMSIM_ACK_RESPONSE;
    /* update the message length */
    messageHeaderPtr->length = sizeof(PPTMsim_header);
    /* not need 'ACK' for this 'ACK'*/
    messageHeaderPtr->ack_needed = 0;

    PRINTK_PARAM("send : sendAckToSender",messageHeaderPtr->message_type);

    sendToTm(socketInfoPtr,
            GT_TRUE,/*reply*/
            (void*)messageHeaderPtr,
            sizeof(*messageHeaderPtr)
            );
}

/*******************************************************************************
* tmDispatcherProcessMsg
*
* DESCRIPTION:
*       process specific message that was received from the remote TM device.
*
* INPUTS:
*       socketInfoPtr  - current socket info.
*       simDeviceId - Simulation device ID.
*       msgBufPtr  - buffer for reading from the socket
*       msgBufSize - size of buffer for reading from the socket
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       none
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static void tmDispatcherProcessMsg(
    IN TM_SOCKET_INFO_STC *socketInfoPtr,
    IN GT_U32          simDeviceId,
    IN GT_U8*          msgBufPtr,
    IN GT_U32          msgBufSize
)
{
    PPMsim_msg_type msgType;
    PPTMsim_header  messageHeader;
    GT_U8*          specificDataPtr = msgBufPtr + sizeof(PPTMsim_header);
    GT_BOOL         sendAckReply = GT_TRUE;

    /* parse the header of the message from the buffer.
       note : this doing "network order to host order" conversion */
    parseBufferToStructure(msgBufPtr,PTR_AND_SIZE_MAC(messageHeader));
    msgType = messageHeader.message_type;

    printk(("got : tmDispatcherProcessMsg : %s \n" ,
        PPMsim_msg_type_Names[msgType]));
    PRINTK_PARAM("got : tmDispatcherProcessMsg",msgType);
    PRINTK_PARAM("got : tmDispatcherProcessMsg",msgBufSize);
    PRINTK_PARAM("got : tmDispatcherProcessMsg",messageHeader.version              );
    PRINTK_PARAM("got : tmDispatcherProcessMsg",messageHeader.message_type         );
    PRINTK_PARAM("got : tmDispatcherProcessMsg",messageHeader.length               );
    PRINTK_PARAM("got : tmDispatcherProcessMsg",messageHeader.tid                  );
    PRINTK_PARAM("got : tmDispatcherProcessMsg",messageHeader.ack_needed           );
    PRINTK_PARAM("got : tmDispatcherProcessMsg",messageHeader.error_code           );

    /* we got something from SERVER */
    socketInfoPtr->serverReady = 1;

    switch(msgType)
    {
        /* from PP to TM */
        case PP_TMSIM_DROP_QUERY:
        case PP_TMSIM_START_PACKET_PROCESS:
        case PP_TMSIM_REGISTER_WRITE:
        case PP_TMSIM_REGISTER_READ:
            printk(("Message[%d] should not come from TM device!!! \n",msgType));
            break;

        case PP_TMSIM_DROP_RESPONSE:
            PRINTK_PARAM("got : processDropResponse",msgType);
            processDropResponse(simDeviceId,specificDataPtr);
            break;
        case PP_TMSIM_PACKET_DONE:
            PRINTK_PARAM("got : processPacketDoneResponse",msgType);
            processPacketDoneResponse(simDeviceId,specificDataPtr);
            break;

        case PP_TMSIM_REGISTER_READ_RESPONSE:
            PRINTK_PARAM("got : processRegisterReadResponse",msgType);
            processRegisterReadResponse(simDeviceId,specificDataPtr);
            break;
        case PP_TMSIM_ACK_RESPONSE:
            PRINTK_PARAM("got : processAck",msgType);
            processAck(simDeviceId);
            sendAckReply = GT_FALSE;
            break;
        default:
            printk(("Message[%d] is unknown !!! \n",msgType));
            sendAckReply = GT_FALSE;
            break;
    }

    if(sendAckReply == GT_TRUE &&
       messageHeader.ack_needed)
    {
        PRINTK_PARAM("send : sendAckToSender",msgType);
        sendAckToSender(socketInfoPtr,&messageHeader);
    }
}

static GT_U32   taskReady = 0;
/*******************************************************************************
* tmDispatcherMainTask
*
* DESCRIPTION:
*       task that handle messages received from the remote TM device.
*
* INPUTS:
*       none
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       none
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static unsigned __TASKCONV tmDispatcherMainTask(IN void* taskInfoPtr)
{
    GT_U32      bufSize;/* current size of buffer that was read from socket */
    GT_SOCK_FD_SET_STC rfds;/* rfds - set of sockets to be checked for readability */
    GT_U32  ii;
    GT_SOCKET_FD    currSockId = 0;/* current socket ID */
    GT_U32  alreadyOnBuffer = 0;/* number of bytes that we have on buffer from
                                the last receive*/
    GT_SOCKET_FD maxSockId;/* the max socket Is for the use of osSocket(..)*/
    GT_BOOL retVal = GT_TRUE;
    /* buffer for read */
    GT_U32  readBufSize = MAX_BUFFER_SIZE_CNS;/* size of buffer for reading from the socket*/
    GT_U8*  readBufPtr = malloc(readBufSize);/* buffer for reading from the socket*/
    /* buffer for reply */
    GT_U32  simDeviceId = (GT_U32)(GT_UINTPTR)(taskInfoPtr);/* simulation device ID */
    TM_SOCKET_INFO_STC *socketInfoPtr;

    rfds.fd_set = SIM_OS_MAC(osSelectCreateSet)();

    while(1)
    {

        /* clear the sets of sockets */
        SIM_OS_MAC(osSelectZeroSet)(rfds.fd_set);

        maxSockId = 0;

        for(ii = 0 ; ii < INDEX__LAST__E ; ii++)
        {
            currSockId = socketInfoArr[ii].mySockId;

            /* add the needed sockets to the needed sets(of sockets) */
            SIM_OS_MAC(osSelectAddFdToSet)(rfds.fd_set,currSockId);
            /* update the max socket id */
            if(maxSockId < currSockId)
            {
                maxSockId = currSockId;
            }
        }

        taskReady = 1;

        /* wait until any socket has info */
        if(0 == SIM_OS_MAC(osSelect)(maxSockId+1,rfds.fd_set,NULL,NULL,(GT_U32)-1/*INFINITE*/))
        {
            /* avoid busy wait in this case */
            SIM_OS_MAC(simOsSleep)(WAIT_SLEEP_TIME_CNS);
            continue;
        }


        currSockId = 0;
        for(ii = 0 ; ii < INDEX__LAST__E ; ii++)
        {
            socketInfoPtr = &socketInfoArr[ii];
            currSockId = socketInfoPtr->mySockId;

            /* add the needed sockets to the needed sets(of sockets) */
            retVal = SIM_OS_MAC(osSelectIsFdSet)(rfds.fd_set,currSockId);
            if(retVal == GT_FALSE)
            {
                continue;
            }

            /*******************************/
            /* we got info from the socket */
            /*******************************/
            printk(("tmDispatcherMainTask : got msg from [%s] \n",
                iniFileTmSection_udpPort[ii]));

            /* set the max size to read to be the size of buffer */
            bufSize = readBufSize;

            LOG_SOCKET_ID_RECEIVE((GT_U32)currSockId);

            /* read the data from socket into local buffer */
            bufSize = SIM_OS_MAC(osSocketRecvFrom)(currSockId,
                                          readBufPtr + alreadyOnBuffer,/*buff*/
                                          bufSize - alreadyOnBuffer,/*buffLen*/
                                          GT_TRUE,/*removeFlag*/
                                          socketInfoPtr->senderInfo.tmSockAddr,/* address of the TM that sent the message */
                                          &dummySizeTypeSize/* length of address of the TM that sent the message */
                                          );

            if(bufSize == 0)
            {
                printf("distributedDispatcherMainTask [%s]: osSocketRecv error -- BUFF 0 \n",iniFileTmSection_udpPort[ii]);
                break;
            }
            else if((bufSize == (GT_U32)(-1)))
            {
                printf("distributedDispatcherMainTask [%s]: osSocketRecv error -- BUFF -1 \n",iniFileTmSection_udpPort[ii]);
                break;
            }

            /* process the message that we got from the TM device */
            tmDispatcherProcessMsg(socketInfoPtr,
                simDeviceId,
                readBufPtr + alreadyOnBuffer,
                bufSize - alreadyOnBuffer);
        }

    }
#ifdef __GNUC__
    /* to avoid warnings in gcc */
    return 0;
#endif
}


/*******************************************************************************
*  sRemoteTmRegisterRead
*
* DESCRIPTION:
*      Read register from TM .
*      logic of operation :
*       1. 'read register' message send from PP to TM.
*       2. wait on semaphore for response from TM
*
*
*           the TM sent to PP 'read register response' with the needed value.
*           signal the semaphore ...
*
*       3. get the info and return
*
* INPUTS:
*       deviceId    - device id.
*       addressPtr  - (pointer to)Address in the TM (64 bits)
*
* OUTPUTS:
*       valuePtr    - (pointer to) get the value of the 'address' (64 bits value)
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void sRemoteTmRegisterRead
(
    IN GT_U32 simDeviceId,
    IN GT_U64 *addressPtr,
    OUT GT_U64 *valuePtr
)
{
    TM_DISTRIBUTED_MESSAGE_SYNC_INFO_STC *messageSyncPtr =
        &tmDistributedMessageSync[TM_MSG_FROM_TM_TO_PP_REGISTER_READ_RESPONSE_E];
    TM_SOCKET_INFO_STC *socketInfoPtr = &socketInfoArr[INDEX_CPU_ACCESS_E];
    REAGISTER_FULL_INFO_STC regInfo;
    PPTMsim_header      *messageHeaderPtr = &regInfo.messageHeader;
    PPTMsim_register    *registerInfoPtr = &regInfo.registerInfo;

    memset(&regInfo,0,sizeof(regInfo));

    messageHeaderPtr->message_type = PP_TMSIM_REGISTER_READ;
    messageHeaderPtr->length = sizeof(regInfo);
    {
        static int operId = 1;
        messageHeaderPtr->tid = operId++;
    }
    PRINTK_PARAM("send : sRemoteTmRegisterRead",messageHeaderPtr->tid);

    messageHeaderPtr->ack_needed =  1;/* need response */

    registerInfoPtr->address = *addressPtr;

    /* lock the waiting operations */
    SIM_OS_MAC(simOsMutexLock)(messageSyncPtr->getReplyMutex);

    /* send the message */
    sendToTm(socketInfoPtr,
            GT_FALSE,/* not reply ... this is new message */
            (void*)&regInfo,
            sizeof(regInfo)
            );

    /* wait for info to be ready */
    SIM_OS_MAC(simOsSemWait)(messageSyncPtr->semSync,SIM_OS_WAIT_FOREVER);

    /* check that DB is on and we got info from expected device */
    if(messageSyncPtr->replySimDeviceId != simDeviceId)
    {
        skernelFatalError(" sRemoteTmRegisterRead : replySimDeviceId[%d] != simDeviceId[%d] \n",
            messageSyncPtr->replySimDeviceId,
            simDeviceId);
    }

    /* check that DB is on and we got info about expected register */
    if(messageSyncPtr->info.readRegisterResponse.address.l[0] != addressPtr->l[0] ||
       messageSyncPtr->info.readRegisterResponse.address.l[1] != addressPtr->l[1]  )
    {
        skernelFatalError(" sRemoteTmRegisterRead : Response.address[0x%x][0x%x] != address[0x%x][0x%x] \n",
            messageSyncPtr->info.readRegisterResponse.address.l[0],
            messageSyncPtr->info.readRegisterResponse.address.l[1],
            addressPtr->l[0],
            addressPtr->l[1]);
    }

    /* copy the value that was returned from the TM device */
    *valuePtr = messageSyncPtr->info.readRegisterResponse.value;


    /* UnLock the waiting operations */
    SIM_OS_MAC(simOsMutexUnlock)(messageSyncPtr->getReplyMutex);
}

/*******************************************************************************
*  sRemoteTmRegisterWrite
*
* DESCRIPTION:
*      Write register To TM .
*
* INPUTS:
*       deviceId    - device id.
*       addressPtr  - (pointer to)Address in the TM (64 bits)
*       valuePtr    - (pointer to) the value of the 'address' (64 bits value)
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void sRemoteTmRegisterWrite
(
    IN GT_U32 simDeviceId,
    IN GT_U64 *addressPtr,
    IN GT_U64 *valuePtr
)
{
    TM_SOCKET_INFO_STC *socketInfoPtr = &socketInfoArr[INDEX_CPU_ACCESS_E];
    REAGISTER_FULL_INFO_STC regInfo;
    PPTMsim_header      *messageHeaderPtr = &regInfo.messageHeader;
    PPTMsim_register    *registerInfoPtr = &regInfo.registerInfo;

    memset(&regInfo,0,sizeof(regInfo));

    messageHeaderPtr->message_type = PP_TMSIM_REGISTER_WRITE;
    messageHeaderPtr->length = sizeof(regInfo);
    {
        static int operId = 1;
        messageHeaderPtr->tid = operId++;
    }
    PRINTK_PARAM("send : sRemoteTmRegisterWrite",messageHeaderPtr->tid);

    messageHeaderPtr->ack_needed =  1;/* need response */

    registerInfoPtr->address = *addressPtr;
    registerInfoPtr->value   = *valuePtr;

    /* send the message */
    sendToTm(socketInfoPtr,
            GT_FALSE,/* not reply ... this is new message */
            (void*)&regInfo,
            sizeof(regInfo)
            );

}


/*******************************************************************************
*  sRemoteTmPacketSendToTm
*
* DESCRIPTION:
*      send packet to TM
*
* INPUTS:
*       deviceId    - device id.
*       packetInfoPtr - (pointer to) packet info
*
* OUTPUTS:
*
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void sRemoteTmPacketSendToTm
(
    IN GT_U32 simDeviceId,
    IN SREMOTE_TM_PP_TO_TM_PACKET_INFO_STC *packetInfoPtr
)
{
    TM_SOCKET_INFO_STC *socketInfoPtr = &socketInfoArr[INDEX_TRAFFIC_DRIVEN_E];
    PACKET_HEADER_FULL_INFO_STC packetHeaderInfo;
    PPTMsim_header          *messageHeaderPtr = &packetHeaderInfo.messageHeader;
    PPTMsim_packet_header   *tmPacketInfoPtr = &packetHeaderInfo.packetHeaderInfo;

    memset(&packetHeaderInfo,0,sizeof(packetHeaderInfo));

    messageHeaderPtr->message_type = PP_TMSIM_START_PACKET_PROCESS;
    messageHeaderPtr->length = sizeof(packetHeaderInfo);
    {
        static int operId = 1;
        messageHeaderPtr->tid = operId++;
    }
    PRINTK_PARAM("send : sRemoteTmPacketSendToTm",messageHeaderPtr->tid);

    messageHeaderPtr->ack_needed =  1;/* need response */

    tmPacketInfoPtr->color          = packetInfoPtr->color;
    tmPacketInfoPtr->cos            = packetInfoPtr->cos;
    tmPacketInfoPtr->pk_len         = packetInfoPtr->pk_len;
    tmPacketInfoPtr->queue          = packetInfoPtr->queue;
    /* copy the cookie to the 'packet Id' */
    memcpy(&tmPacketInfoPtr->packet_id ,
           &packetInfoPtr->cookiePtr ,
            sizeof(packetInfoPtr->cookiePtr));

    /* send the message */
    sendToTm(socketInfoPtr,
            GT_FALSE,/* not reply ... this is new message */
            (void*)&packetHeaderInfo,
            sizeof(packetHeaderInfo)
            );
}


/*******************************************************************************
*  sRemoteTmPacketDropInfoGet
*
* DESCRIPTION:
*      send to TM query about the 'drop' for 'input info' and get reply with
*       pass/drop + drop recommendations.
*
* INPUTS:
*       deviceId    - device id.
*       inDropInfoPtr - (pointer to) input info
*
* OUTPUTS:
*       outDropInfoPtr - (pointer to) output info
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void sRemoteTmPacketDropInfoGet
(
    IN GT_U32 simDeviceId,
    IN SREMOTE_TM_PP_TO_TM_INPUT_DROP_INFO_STC   *inDropInfoPtr,
    OUT SREMOTE_TM_PP_TO_TM_OUTPUT_DROP_INFO_STC *outDropInfoPtr
)
{
    TM_DISTRIBUTED_MESSAGE_SYNC_INFO_STC *messageSyncPtr =
        &tmDistributedMessageSync[TM_MSG_FROM_TM_TO_PP_DROP_RESPONSE_E];
    TM_SOCKET_INFO_STC *socketInfoPtr = &socketInfoArr[INDEX_TRAFFIC_DRIVEN_E];
    PACKET_DROP_QUERY_FULL_INFO_STC dropQuery;
    PPTMsim_header      *messageHeaderPtr = &dropQuery.messageHeader;
    PPTMsim_drop_query  *dropQueryInfoPtr = &dropQuery.dropQueryInfo;
    PPTMsim_drop_response  *dropResponsePtr;

    memset(&dropQuery,0,sizeof(dropQuery));

    messageHeaderPtr->message_type = PP_TMSIM_DROP_QUERY;
    messageHeaderPtr->length = sizeof(dropQuery);
    {
        static int operId = 1;
        messageHeaderPtr->tid = operId++;
    }
    PRINTK_PARAM("send : sRemoteTmPacketDropInfoGet",messageHeaderPtr->tid);

    messageHeaderPtr->ack_needed =  1;/* need response */

    dropQueryInfoPtr->packet_id.l[0]  = messageHeaderPtr->tid;
    dropQueryInfoPtr->packet_id.l[1]  = simDeviceId;
    dropQueryInfoPtr->dropProbabilitySelect = inDropInfoPtr->dropProbabilitySelect;
    dropQueryInfoPtr->cos = inDropInfoPtr->cos;
    dropQueryInfoPtr->color = inDropInfoPtr->color;
    dropQueryInfoPtr->queue = inDropInfoPtr->queue;

    /* lock the waiting operations */
    SIM_OS_MAC(simOsMutexLock)(messageSyncPtr->getReplyMutex);

    /* send the message */
    sendToTm(socketInfoPtr,
            GT_FALSE,/* not reply ... this is new message */
            (void*)&dropQuery,
            sizeof(dropQuery)
            );

    /* wait for info to be ready */
    SIM_OS_MAC(simOsSemWait)(messageSyncPtr->semSync,SIM_OS_WAIT_FOREVER);

    /* check that DB is on and we got info from expected device */
    if(messageSyncPtr->replySimDeviceId != simDeviceId)
    {
        skernelFatalError(" sRemoteTmRegisterRead : replySimDeviceId[%d] != simDeviceId[%d] \n",
            messageSyncPtr->replySimDeviceId,
            simDeviceId);
    }

    dropResponsePtr = &messageSyncPtr->info.dropResponse;

    /* check that DB is on and we got info about expected info */
    if(dropResponsePtr->packet_id.l[0] != dropQueryInfoPtr->packet_id.l[0] ||
       dropResponsePtr->packet_id.l[1] != dropQueryInfoPtr->packet_id.l[1] )
    {
        skernelFatalError(" sRemoteTmRegisterRead : Response.packet_id[0x%x][0x%x] != packet_id[0x%x][0x%x] \n",
            dropResponsePtr->packet_id.l[0],
            dropResponsePtr->packet_id.l[1],
            dropQueryInfoPtr->packet_id.l[0],
            dropQueryInfoPtr->packet_id.l[1]);
    }

    if(dropResponsePtr->queue != dropQueryInfoPtr->queue)
    {
        skernelFatalError(" sRemoteTmRegisterRead : Response.queue[0x%x] != queue[0x%x] \n",
            dropResponsePtr->queue,
            dropQueryInfoPtr->queue);
    }

    /* copy the value that was returned from the TM device */
    outDropInfoPtr->queue                      = dropResponsePtr->queue;
    outDropInfoPtr->tailQueueRecommendation    = dropResponsePtr->tailQueueRecommendation;
    outDropInfoPtr->wredQueueRecommendation    = dropResponsePtr->wredQueueRecommendation;
    outDropInfoPtr->tailAnodeRecommendation    = dropResponsePtr->tailAnodeRecommendation;
    outDropInfoPtr->wredAnodeRecommendation    = dropResponsePtr->wredAnodeRecommendation;
    outDropInfoPtr->tailBnodeRecommendation    = dropResponsePtr->tailBnodeRecommendation;
    outDropInfoPtr->wredBnodeRecommendation    = dropResponsePtr->wredBnodeRecommendation;
    outDropInfoPtr->tailCnodeRecommendation    = dropResponsePtr->tailCnodeRecommendation;
    outDropInfoPtr->wredCnodeRecommendation    = dropResponsePtr->wredCnodeRecommendation;
    outDropInfoPtr->tailPortRecommendation     = dropResponsePtr->tailPortRecommendation;
    outDropInfoPtr->wredPortRecommendation     = dropResponsePtr->wredPortRecommendation;
    outDropInfoPtr->bufferRsv                  = dropResponsePtr->bufferRsv;
    outDropInfoPtr->outOfRsv                   = dropResponsePtr->outOfRsv;
    outDropInfoPtr->dropProbability            = dropResponsePtr->dropProbability;
    outDropInfoPtr->port                       = dropResponsePtr->port;
    outDropInfoPtr->aNode                      = dropResponsePtr->aNode;
    outDropInfoPtr->bNode                      = dropResponsePtr->bNode;
    outDropInfoPtr->cNode                      = dropResponsePtr->cNode;

    /* UnLock the waiting operations */
    SIM_OS_MAC(simOsMutexUnlock)(messageSyncPtr->getReplyMutex);
}

/*******************************************************************************
*   sRemoteTmCreateUdpSockets
*
* DESCRIPTION:
*       create the needed sockets for TM device.
*       using UDP sockets (as used by the TM device)
*
* INPUTS:
*       simDeviceId     - Simulation device ID.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*       must be called before sRemoteTmWaitForTmServer
*******************************************************************************/
void sRemoteTmCreateUdpSockets
(
    IN GT_U32   simDeviceId
)
{
#if FROM_INI_FILE
    char getStr[SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS];/* string to read from INI file */
    GT_U32  tmpValue;
#endif /*FROM_INI_FILE*/
    char ipAddressStr[SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS];
    TM_SOCKET_INFO_STC *socketInfoPtr;
    GT_U32  udpPortArr[INDEX__LAST__E];/* UDP ports that the 'server' uses ... as we are the 'client' */
    GT_U32  ii;
    GT_TASK_HANDLE  taskId;
    GT_U32  dummySize;
    void*  simDeviceIdPtr = (void*)(GT_UINTPTR)simDeviceId;
/*
*       udpPortTrafficDriven     - UDP port (for sockets)
*                         processing of traffic driven actions from PP to TM :
*                            send packet header , drop query
*                         processing of traffic driven actions from TM to PP :
*                            drop response , packet processing ended
*
*                         NOTE: simulation is 'client' so this 'port' is of the
*                               'server' (the TM device)
*
*       udpPortCpuAccess  - UDP port (for sockets)
*                         processing of cpu access driven action from PP to TM :
*                            write register , read register query
*                         processing of cpu access driven action from TM to PP:
*                            read register response.
*
*                         NOTE: simulation is 'client' so this 'port' is of the
*                               'server' (the TM device)
*/

    SIM_OS_MAC(osSocketGetSocketAddrSize)(&dummySize);
    dummySizeTypeSize = dummySize;


#if FROM_INI_FILE
    if(! SIM_OS_MAC(simOsGetCnfValue)(iniFileTmSectionStr,  iniFileTmSection_ipAddr,
                             SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS, ipAddressStr))
    {
        /* assign the default loopback port */
        strcpy(ipAddressStr,loopbackIpStr);
    }
#else /*!FROM_INI_FILE*/

    /* assign the default loopback port */
    strcpy(ipAddressStr,loopbackIpStr);

    udpPortArr[INDEX_TRAFFIC_DRIVEN_E]  = DUMMY_UDP_PORT_AS_SERVER_CNS;
    udpPortArr[INDEX_CPU_ACCESS_E]      = DUMMY_UDP_PORT_AS_SERVER_CNS + 1;
#endif /*!FROM_INI_FILE*/

    sRemoteTmUsed = 1;

    for(ii = 0 ; ii < TM_MSG_FROM_TM_TO_PP__LAST__E ; ii++)
    {
        if(tmDistributedMessageSync[ii].semSync)
        {
            /* already initialized */
            break;
        }

        /* create semaphores for signaling ... that the first 'wait' will wait
            for the first 'signal' from other task */
        tmDistributedMessageSync[ii].semSync = SIM_OS_MAC(simOsSemCreate)(0,1);

        /* create Mutex */
        tmDistributedMessageSync[ii].getReplyMutex = SIM_OS_MAC(simOsMutexCreate)();
    }


    for(ii = 0 ; ii < INDEX__LAST__E; ii++)
    {
        socketInfoPtr = &socketInfoArr[ii];

        if(socketInfoPtr->mySockId)
        {
            /* already exists */
            continue;
        }

        strcpy(socketInfoPtr->serverIpAddressStr,ipAddressStr);

#if FROM_INI_FILE
        if(! SIM_OS_MAC(simOsGetCnfValue)(iniFileTmSectionStr,  iniFileTmSection_udpPort[ii],
                                 SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS, getStr))
        {
            skernelFatalError(" simDistributedInit : missing in section [%s] the info on[%s] \n",
                iniFileTmSectionStr,
                iniFileTmSection_udpPort[ii]);
        }

        sscanf(getStr, "%d", &tmpValue);
        udpPortArr[ii] = tmpValue;
        if(udpPortArr[ii] >= (1<<16))
        {
            skernelFatalError(" simDistributedInit : [%s][%d] out of range \n",
                iniFileTmSection_udpPort[ii],
                udpPortArr[ii]);
        }
#endif /*FROM_INI_FILE*/
        socketInfoPtr->serverProtocolPort = (GT_U16)udpPortArr[ii];

        /* Build IP address for sending to server */
        if (GT_OK != SIM_OS_MAC(osSocketCreateAddr)(
            socketInfoPtr->serverIpAddressStr,
            socketInfoPtr->serverProtocolPort,
            &socketInfoPtr->serverSockAddr,
            &dummySizeTypeSize))
        {
            skernelFatalError(" sRemoteTmCreateUdpSockets : osSocketCreateAddr \n");
        }


        /* create UDP socket - for getting replies from the SERVER */
        if ((socketInfoPtr->mySockId = SIM_OS_MAC(osSocketUdpCreate)(GT_SOCKET_DEFAULT_SIZE)) < 0)
        {
            skernelFatalError(" sRemoteTmCreateUdpSockets : osSocketUdpCreate \n");
        }

        socketInfoPtr->senderInfo.tmSockAddr = calloc(dummySizeTypeSize,1);

        /* Build IP address to be used across all other functions */
        if (GT_OK != SIM_OS_MAC(osSocketCreateAddr)(
            NULL,/* NULL will assign INADDR_ANY --> let the socket manager assign me ALL IPs that my machine hold */
            0,/* let the socket manager assign any 'non used port' */
            &socketInfoPtr->localSockAddr,
            &dummySizeTypeSize))
        {
            skernelFatalError(" sRemoteTmCreateUdpSockets : osSocketCreateAddr \n");
        }

        /* set socket to blocking mode */
        if (GT_OK != SIM_OS_MAC(osSocketSetBlock)(socketInfoPtr->mySockId))
        {
            skernelFatalError(" sRemoteTmCreateUdpSockets : osSocketSetBlock \n");
        }

        /* bind the socket (for the use of select) */
        if (GT_OK != SIM_OS_MAC(osSocketBind) (socketInfoPtr->mySockId,
            socketInfoPtr->localSockAddr,
            dummySizeTypeSize))
        {
            skernelFatalError(" sRemoteTmCreateUdpSockets : osSocketBind");
        }
    }

    /******************************/
    /* create the Dispatcher task */
    /******************************/
    taskReady = 0;
    taskId = SIM_OS_MAC(simOsTaskCreate)(GT_TASK_PRIORITY_ABOVE_NORMAL,/* same as skernel task */
                                         tmDispatcherMainTask,
                                         simDeviceIdPtr);

    if(taskId == NULL)
    {
        skernelFatalError(" sRemoteTmCreateUdpSockets : fail to create task");
    }

    while(taskReady == 0)
    {
        /* wait for task to be ready */
        SIM_OS_MAC(simOsSleep)(WAIT_SLEEP_TIME_CNS);
    }
}

/*******************************************************************************
*   sRemoteTmWaitForTmServer
*
* DESCRIPTION:
*       initiate 'wait' for the TM server to be ready.
*       function does not returns until TM server response.
*
* INPUTS:
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*       must be called after sRemoteTmCreateUdpSockets
*******************************************************************************/
void sRemoteTmWaitForTmServer(IN GT_U32 simDeviceId)
{
    TM_SOCKET_INFO_STC *socketInfoPtr;
    GT_U32  ii;

    /* retry 'cpu access' socket , until server is 'UP' */
    ii = INDEX_CPU_ACCESS_E;
    socketInfoPtr = &socketInfoArr[ii];

    /* when server is not ready we must wait for indication that it is ready */
    while (socketInfoPtr->serverReady == 0)
    {
        ppSendTmClientInfo(simDeviceId,ii);

        simulationPrintf(".");
    }
    simulationPrintf("\n");

    SIM_OS_MAC(simOsSleep)(50);

    /* indicate that ALL sockets are ready in the SERVER */
    for(ii = 0 ; ii < INDEX__LAST__E; ii++)
    {
        socketInfoPtr = &socketInfoArr[ii];
        socketInfoPtr->serverReady = 1;
    }
}


/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
/* tests environment
1. emulate TM device side
2. send from 'pp' side :
    read register
    packet start + wait for packet ended
    drop query + wait for drop response
*/
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

static GT_SIZE         ppTest_SockAddrLen;

static GT_SOCKET_FD testTm_sockId[INDEX__LAST__E];
static GT_VOID*     testPp_sockAddr[INDEX__LAST__E];


static void testTmToPp_processDropQuery(
    IN GT_U8*          specificDataPtr,
    OUT GT_U32         *responseSizePtr
)
{
    PPTMsim_drop_query          dropQuery;
    PPTMsim_drop_response       dropResponse;

    /* parse the read reply info from the buffer.
       note : this doing "network order to host order" conversion */
    parseBufferToStructure(specificDataPtr,
        PTR_AND_SIZE_MAC(dropQuery));

    PRINTK_PARAM("testTmToPp_processDropQuery",dropQuery.packet_id.l[0]          );
    PRINTK_PARAM("testTmToPp_processDropQuery",dropQuery.packet_id.l[1]          );
    PRINTK_PARAM("testTmToPp_processDropQuery",dropQuery.dropProbabilitySelect   );
    PRINTK_PARAM("testTmToPp_processDropQuery",dropQuery.cos                     );
    PRINTK_PARAM("testTmToPp_processDropQuery",dropQuery.color                   );
    PRINTK_PARAM("testTmToPp_processDropQuery",dropQuery.queue                   );

    memset(&dropResponse,0,sizeof(dropResponse));
    dropResponse.packet_id = dropQuery.packet_id;
    dropResponse.queue = dropQuery.queue;
    dropResponse.dropProbability = 15;

    buildStructureToBuffer(PTR_AND_SIZE_MAC(dropResponse),
        specificDataPtr);

    *responseSizePtr = sizeof(dropResponse);
    return;
}

static void testTmToPp_processStartPacket(
    IN GT_U8*          specificDataPtr,
    OUT GT_U32         *responseSizePtr
)
{
    PPTMsim_packet_header          packetHeader;
    PPTMsim_packet_done_header     packetDoneHeader;

    /* parse the read reply info from the buffer.
       note : this doing "network order to host order" conversion */
    parseBufferToStructure(specificDataPtr,
        PTR_AND_SIZE_MAC(packetHeader));

    PRINTK_PARAM("testTmToPp_processStartPacket",packetHeader.packet_id.l[0]          );
    PRINTK_PARAM("testTmToPp_processStartPacket",packetHeader.packet_id.l[1]          );
    PRINTK_PARAM("testTmToPp_processStartPacket",packetHeader.cos                     );
    PRINTK_PARAM("testTmToPp_processStartPacket",packetHeader.color                   );
    PRINTK_PARAM("testTmToPp_processStartPacket",packetHeader.pk_len                  );
    PRINTK_PARAM("testTmToPp_processStartPacket",packetHeader.queue                   );

    memset(&packetDoneHeader,0,sizeof(packetDoneHeader));
    packetDoneHeader.packet_id = packetHeader.packet_id;
    packetDoneHeader.egress_port = 18;

    buildStructureToBuffer(PTR_AND_SIZE_MAC(packetDoneHeader),
        specificDataPtr);

    *responseSizePtr = sizeof(packetDoneHeader);
    return;
}

static void testTmToPp_processRegisterRead(
    IN GT_U8*          specificDataPtr,
    OUT GT_U32         *responseSizePtr
)
{
    PPTMsim_register     registerIn;
    PPTMsim_register     registerOut;

    /* parse the read reply info from the buffer.
       note : this doing "network order to host order" conversion */
    parseBufferToStructure(specificDataPtr,
        PTR_AND_SIZE_MAC(registerIn));

    PRINTK_PARAM("testTmToPp_processRegisterRead",registerIn.address.l[0]          );
    PRINTK_PARAM("testTmToPp_processRegisterRead",registerIn.address.l[1]          );

    memset(&registerOut,0,sizeof(registerOut));
    registerOut.address = registerIn.address;
    registerOut.value   = registerIn.address;/* return register value as it's address */

    buildStructureToBuffer(PTR_AND_SIZE_MAC(registerOut),
        specificDataPtr);

    *responseSizePtr = sizeof(registerOut);
    return;
}

static void testTmToPp_processRegisterWrite(
    IN GT_U8*          specificDataPtr
)
{
    PPTMsim_register     registerIn;

    /* parse the read reply info from the buffer.
       note : this doing "network order to host order" conversion */
    parseBufferToStructure(specificDataPtr,
        PTR_AND_SIZE_MAC(registerIn));

    PRINTK_PARAM("testTmToPp_processRegisterWrite",registerIn.address.l[0]          );
    PRINTK_PARAM("testTmToPp_processRegisterWrite",registerIn.address.l[1]          );
    PRINTK_PARAM("testTmToPp_processRegisterWrite",registerIn.value.l[0]          );
    PRINTK_PARAM("testTmToPp_processRegisterWrite",registerIn.value.l[1]          );

    return;
}

static void testTmToPp_sendAckToSender(
    IN INDEX_TYPE_E    indexType  ,
    IN PPTMsim_header  *messageHeaderPtr
)
{
    GT_SOCKET_FD    currSockId = testTm_sockId[indexType];

    /* update the message type to be 'ACK' */
    messageHeaderPtr->message_type = PP_TMSIM_ACK_RESPONSE;
    /* update the message length */
    messageHeaderPtr->length = sizeof(PPTMsim_header);

    /* convert CPU order STC to network order bytes */
    buildStructureToBuffer(messageHeaderPtr,messageHeaderPtr->length,(GT_U8*)messageHeaderPtr);

    SIM_OS_MAC(osSocketSendTo)(currSockId,
            messageHeaderPtr,
            sizeof(*messageHeaderPtr),
            testPp_sockAddr[indexType] ,
            ppTest_SockAddrLen
            );
}


static void tmTest_ProcessMsg(
    IN INDEX_TYPE_E    indexType  ,
    IN GT_U8*          msgBufPtr,
    IN GT_U32          msgBufSize
)
{
    GT_SOCKET_FD    currSockId;
    PPMsim_msg_type msgType;
    PPTMsim_header  messageHeader;
    GT_U8*          specificDataPtr = msgBufPtr + sizeof(PPTMsim_header);
    GT_BOOL         sendAckReply = GT_TRUE;
    GT_U32          responseSize = 0;
    GT_U32          new_Message_type = 0xFFFFFFFF;

    /* parse the header of the message from the buffer.
       note : this doing "network order to host order" conversion */
    parseBufferToStructure(msgBufPtr,PTR_AND_SIZE_MAC(messageHeader));
    msgType = messageHeader.message_type;

    printk(("tmTest_ProcessMsg : %s \n" ,
        PPMsim_msg_type_Names[msgType]));
    PRINTK_PARAM("tmTest_ProcessMsg",messageHeader.version              );
    PRINTK_PARAM("tmTest_ProcessMsg",messageHeader.message_type         );
    PRINTK_PARAM("tmTest_ProcessMsg",messageHeader.length               );
    PRINTK_PARAM("tmTest_ProcessMsg",messageHeader.tid                  );
    PRINTK_PARAM("tmTest_ProcessMsg",messageHeader.ack_needed           );
    PRINTK_PARAM("tmTest_ProcessMsg",messageHeader.error_code           );

    switch(msgType)
    {
        /* from TM to PP */
        case PP_TMSIM_DROP_RESPONSE:
        case PP_TMSIM_PACKET_DONE:
        case PP_TMSIM_REGISTER_READ_RESPONSE:
            printk(("tmTest_ProcessMsg:Message[%d] should not come from PP device (to TM)!!! \n",msgType));
            break;

        case PP_TMSIM_DROP_QUERY:
            testTmToPp_processDropQuery(specificDataPtr,&responseSize);
            new_Message_type = PP_TMSIM_DROP_RESPONSE;
            break;
        case PP_TMSIM_START_PACKET_PROCESS:
            testTmToPp_processStartPacket(specificDataPtr,&responseSize);
            new_Message_type = PP_TMSIM_PACKET_DONE;
            break;
        case PP_TMSIM_REGISTER_READ:
            testTmToPp_processRegisterRead(specificDataPtr,&responseSize);
            new_Message_type = PP_TMSIM_REGISTER_READ_RESPONSE;
            break;
        case PP_TMSIM_ACK_RESPONSE:
            sendAckReply = GT_FALSE;
            break;
        case PP_TMSIM_REGISTER_WRITE:
            /* no more to do (except for 'ACK')*/
            testTmToPp_processRegisterWrite(specificDataPtr);
            break;
        default:
            printk(("tmTest_ProcessMsg:Message[%d] is unknown !!! \n",msgType));
            sendAckReply = GT_FALSE;
            break;
    }

    currSockId = testTm_sockId[indexType];

    if(currSockId == 0)
    {
        skernelFatalError(" tmTest_ProcessMsg : not got 'client init' message ?! \n");
    }

    if(sendAckReply == GT_TRUE &&
       messageHeader.ack_needed)
    {
        /* send ACK even if we going to send 'reply' message */
        testTmToPp_sendAckToSender(indexType,&messageHeader);
    }

    if (responseSize && new_Message_type != 0xFFFFFFFF)
    {
        if(sendAckReply == GT_TRUE && messageHeader.ack_needed)
        {
            /* the send ACK change the 'cpu oderer' of the messageHeader ...
            so get it again from the orig buffer */
            parseBufferToStructure(msgBufPtr,PTR_AND_SIZE_MAC(messageHeader));
        }
        /* message_type */
        messageHeader.message_type = new_Message_type;
        /*length*/
        messageHeader.length = responseSize + sizeof(messageHeader);
        /* in UDP we need 'ack' */
        messageHeader.ack_needed = 1;
        /* 'payload' already ready*/
        buildStructureToBuffer(&messageHeader,sizeof(messageHeader),msgBufPtr);

        SIM_OS_MAC(osSocketSendTo)(currSockId,
                msgBufPtr,
                responseSize + sizeof(messageHeader),
                testPp_sockAddr[indexType] ,
                ppTest_SockAddrLen
                );
    }
}

static unsigned __TASKCONV tmTest_DispatcherMainTask(IN void* dummy)
{
    GT_BOOL retVal = GT_TRUE;
    GT_U32      bufSize;/* current size of buffer that was read from socket */
    GT_U32  alreadyOnBuffer = 0;/* number of bytes that we have on buffer from
                                the last receive*/
    GT_U32  ii;
    GT_SOCK_FD_SET_STC rfds;/* rfds - set of sockets to be checked for readability */
    GT_SOCKET_FD maxSockId;/* the max socket Is for the use of osSocket(..)*/
    GT_SOCKET_FD    currSockId = 0;/* current socket ID */
    /* buffer for read */
    GT_U32  readBufSize = MAX_BUFFER_SIZE_CNS;/* size of buffer for reading from the socket*/
    GT_U8*  readBufPtr = malloc(readBufSize);/* buffer for reading from the socket*/
    GT_U32 dummySize;

    rfds.fd_set = SIM_OS_MAC(osSelectCreateSet)();

    SIM_OS_MAC(osSocketGetSocketAddrSize)(&dummySize);
    ppTest_SockAddrLen = dummySize;

    while(1)
    {
        /* clear the sets of sockets */
        SIM_OS_MAC(osSelectZeroSet)(rfds.fd_set);

        maxSockId = 0;

        for(ii = 0 ; ii < INDEX__LAST__E ; ii++)
        {
            if(NULL == testPp_sockAddr[ii])
            {
                testPp_sockAddr[ii] = calloc(ppTest_SockAddrLen,1);
            }

            currSockId = testTm_sockId[ii];

            /* add the needed sockets to the needed sets(of sockets) */
            SIM_OS_MAC(osSelectAddFdToSet)(rfds.fd_set,currSockId);
            /* update the max socket id */
            if(maxSockId < currSockId)
            {
                maxSockId = currSockId;
            }
        }

        /* wait until any socket has info */
        if(0 == SIM_OS_MAC(osSelect)(maxSockId+1,rfds.fd_set,NULL,NULL,(GT_U32)-1/*INFINITE*/))
        {
            /* avoid busy wait in this case */
            SIM_OS_MAC(simOsSleep)(WAIT_SLEEP_TIME_CNS);
            continue;
        }
        currSockId = 0;
        for(ii = 0 ; ii < INDEX__LAST__E ; ii++)
        {
            currSockId = testTm_sockId[ii];

            /* add the needed sockets to the needed sets(of sockets) */
            retVal = SIM_OS_MAC(osSelectIsFdSet)(rfds.fd_set,currSockId);
            if(retVal == GT_FALSE)
            {
                continue;
            }

            printk(("TM_test : got msg from [%s] \n",
                iniFileTmSection_udpPort[ii]));

            /*******************************/
            /* we got info from the socket */
            /*******************************/

            /* set the max size to read to be the size of buffer */
            bufSize = readBufSize;

            LOG_SOCKET_ID_RECEIVE((GT_U32)currSockId);

            /* read the data from socket into local buffer */
            bufSize = SIM_OS_MAC(osSocketRecvFrom)(currSockId,
                                          readBufPtr + alreadyOnBuffer,/*buff*/
                                          bufSize - alreadyOnBuffer,/*buffLen*/
                                          GT_TRUE,/*removeFlag*/
                                          testPp_sockAddr[ii],/* get the ip addr + udp port of sender */
                                          &ppTest_SockAddrLen/* length of address of the PP that sent the message */
                                          );

            if(bufSize == 0)
            {
                printf("tmTest_DispatcherMainTask [%s]: osSocketRecv error -- BUFF 0 \n",iniFileTmSection_udpPort[ii]);
                break;
            }
            else if((bufSize == (GT_U32)(-1)))
            {
                printf("tmTest_DispatcherMainTask [%s]: osSocketRecv error -- BUFF -1 \n",iniFileTmSection_udpPort[ii]);
                break;
            }

            /* process the message that we got from the TM device */
            tmTest_ProcessMsg(ii,
                readBufPtr + alreadyOnBuffer,
                bufSize - alreadyOnBuffer);
        }

    }
#ifdef __GNUC__
    /* to avoid warnings in gcc */
    return 0;
#endif
}
/*******************************************************************************
*   sRemoteTmTest
*
* DESCRIPTION:
*       test the messages between simulation and TM
*
* INPUTS:
*       1. open socket that emulate the TM
*       2. implement responses to 'read register' , 'packet start' , 'drop query'
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void sRemoteTmTest_emulateDummyTmDevice(void)
{
    GT_TASK_HANDLE  taskId;
    GT_U32  ii;
    GT_VOID        *sockAddr;
    GT_SIZE        sockAddrLen;

    /*SIM_OS_MAC(osSocketGetSocketAddrSize)(&sockAddrLen);
    sockAddr = calloc(sockAddrLen,1);*/


    for(ii = 0 ; ii < INDEX__LAST__E ; ii++)
    {
        /* create UDP socket for 'server' that wait for messages from client */
        if ((testTm_sockId[ii] = SIM_OS_MAC(osSocketUdpCreate)(GT_SOCKET_DEFAULT_SIZE)) < 0)
        {
            skernelFatalError(" sRemoteTmTest_emulateDummyTmDevice : osSocketUdpCreate \n");
        }

        /* Build IP address to be used across all other functions */
        if (GT_OK != SIM_OS_MAC(osSocketCreateAddr)(
            loopbackIpStr,
            DUMMY_UDP_PORT_AS_SERVER_CNS+ii,
            &sockAddr,
            &sockAddrLen))
        {
            skernelFatalError(" sRemoteTmTest_emulateDummyTmDevice : osSocketCreateAddr \n");
        }

        /* set socket to blocking mode */
        if (GT_OK != SIM_OS_MAC(osSocketSetBlock)(testTm_sockId[ii]))
        {
            skernelFatalError(" sRemoteTmTest_emulateDummyTmDevice : osSocketSetBlock \n");
        }

        /* bind the socket */
        if (GT_OK != SIM_OS_MAC(osSocketBind) (testTm_sockId[ii],
            sockAddr,
            sockAddrLen))
        {
            skernelFatalError(" sRemoteTmTest_emulateDummyTmDevice : osSocketBind");
        }
    }

    /******************************/
    /* create the Dispatcher task */
    /******************************/
    taskId = SIM_OS_MAC(simOsTaskCreate)(GT_TASK_PRIORITY_ABOVE_NORMAL,
                                         tmTest_DispatcherMainTask,
                                         NULL);

    if(taskId == NULL)
    {
        skernelFatalError(" sRemoteTmTest : fail to create task");
    }

    /* send via simulation 'init message' to the TM server */
    sRemoteTmWaitForTmServer(0);
}


void sRemoteTmTest_emulatePpToTm_RegisterWrite(
    void
)
{
    GT_U64  address,value;
    static GT_U32   extra = 0;

    address.l[0] = 0x12345678+extra;
    address.l[1] = 0x5566AAEE;
    value.l[0]   = 0x99663311+extra;
    value.l[1]   = 0x77553399;

    sRemoteTmRegisterWrite(0,&address,&value);

    extra += 0x11334466;
}

void sRemoteTmTest_emulatePpToTm_RegisterRead(
    void
)
{
    GT_U64  address,value;
    static GT_U32   extra = 0;

    address.l[0] = 0x12345678+extra;
    address.l[1] = 0x5566AAEE + (extra+5); /* when code is "0x5566AAEE+extra+5" I get GCC error : error: invalid suffix "+extra" on integer constant
                                        see hint about GCC issue in : GCC Bugzilla – Bug 3885 (and other related bugs) */

    sRemoteTmRegisterRead(0,&address,&value);

    /*check that value is like address ... behavior that I given to test*/
    if(value.l[0] != address.l[0] ||
       value.l[1] != address.l[1])
    {
        PRINTK_PARAM("RegisterRead : ERROR",address.l[0]);
        PRINTK_PARAM("RegisterRead : ERROR",address.l[1]);
        PRINTK_PARAM("RegisterRead : ERROR",value.l[0]);
        PRINTK_PARAM("RegisterRead : ERROR",value.l[1]);
    }

    extra += 0x11334466;
}

static void sRemoteTmTest_emulatePpToTm_performanceTest(
    GT_U32  isRead,
    GT_U32  maxIterations
)
{
    GT_U32  timeStart,timeEnd;
    GT_U32  ii;

    timeStart = SIM_OS_MAC(simOsTickGet)();

    allowPrint = 0;

    ii = maxIterations;
    if(isRead)
    {
        while(ii--)
        {
            sRemoteTmTest_emulatePpToTm_RegisterRead();
        }
    }
    else
    {
        while(ii--)
        {
            sRemoteTmTest_emulatePpToTm_RegisterWrite();
        }
    }

    allowPrint = 1;

    timeEnd = SIM_OS_MAC(simOsTickGet)();

    simulationPrintf("RegisterRead_performance : total ticks[%d] for [%d] iterations ([%d] [%s] in 1000 ticks) \n",
        timeEnd-timeStart,maxIterations,
        ((1000*maxIterations) / (timeEnd-timeStart)),
        isRead ? "read" : "write");

}

void sRemoteTmTest_emulatePpToTm_RegisterRead_performanceTest(
    GT_U32  maxIterations)
{
    sRemoteTmTest_emulatePpToTm_performanceTest(1/*read*/,maxIterations);
}

void sRemoteTmTest_emulatePpToTm_RegisterWrite_performanceTest(
    GT_U32  maxIterations)
{
    sRemoteTmTest_emulatePpToTm_performanceTest(0/*write*/,maxIterations);
}


