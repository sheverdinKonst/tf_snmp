/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* sdistributed.c
*
* DESCRIPTION:
*       This module is the distribution manager of simulation.
*
* FILE REVISION NUMBER:
*       $Revision: 15 $
*******************************************************************************/

#include <os/simTypesBind.h>
#include <asicSimulation/SDistributed/sdistributed.h>
#include <asicSimulation/SInit/sinit.h>
#include <common/SMiniBuf/SMiniBuf.h>
#include <common/SQue/squeue.h>

#include <common/SHOST/GEN/INTR/EXP/INTR.H>
#include <common/SHOST/HOST_D/EXP/HOST_D.H>
/* next needed for H file <asicSimulation/SDistributed/new_message.h> */
#define USE_GT_TYPES
#include <asicSimulation/SDistributed/new_message.h>

/*
1. "synchronic socket"
    use this socket for messages initiate by "application"

2. "asynchronous socket"
    use this socket for messages initiate by "device"

3. "Application side" - always "client"
   "interface bridge BUS" - always "server" towards "application" , and client towards "device"
   "device" - always "server"

*/

/* define a pointer to the first field in the message coming after the info of
   header */
#define FIRST_FIELD_AFTER_HEADER_PTR(messagePtr) \
    (void*)((GT_U8*)(messagePtr) + sizeof(MSG_HDR))

/* number of words in BYTES include the header */
#define TOTAL_LEN_BYTES(lenWithoutHeader) (lenWithoutHeader + sizeof(MSG_HDR))

/* number of BYTES not include the header */
#define LEN_NO_HEADER_BYTES(stc) (sizeof(stc) - sizeof(MSG_HDR))

/* number of WORDS not include the header */
#define LEN_NO_HEADER_WORDS(stc) (LEN_NO_HEADER_BYTES(stc) / 4)

/* define replace SKERNEL_NIC because we can't include H file of 'skernel'*/
/*should be same as SKERNEL_NIC */
#define SKERNEL_NIC_CNS     1

/* number of WORDS that the place hold takes, see in messages of:
WRITE_MSG
READ_RESPONSE_MSG
DMA_WRITE_MSG
DMA_READ_RESPONSE_MSG
 */
#define PLACE_HOLDER_FIELD_WORDS_SIZE   (sizeof(GT_U32*) /4)

/* the number of words in message header */
#define HEADER_SIZE_WORDS (sizeof(MSG_HDR) / 4)

/*******************************************************************************
* Private type definition
*******************************************************************************/
/* macro to define prototypes of functions that are 'tasks' */
#define TASK_DECLARE_MAC(taskFunc)  \
    static unsigned __TASKCONV taskFunc(IN void* dummy)

TASK_DECLARE_MAC(distributedDispatcherMainTask);
TASK_DECLARE_MAC(distributedDispatcherNicRxTask);
TASK_DECLARE_MAC(distributedDispatcherServerAccept);

/* prototype of function that represent a task */
typedef unsigned (__TASKCONV *TASK_PTR_FUN)(void*);

/* define most common use of delay that we use , where need to wait for change
   of state without the use of semaphores , and without doing 'Busy wait' */
#define WAIT_SLEEP_TIME_CNS             500

/* define time to wait after sending reset message before continue
   to make sure that the reset message will get to the other side */
#define RESET_WAIT_SLEEP_TIME_CNS       500

/* macro that create info in network order from host order , and vice versa */
#define SIM_HTONL(charPtr)  ((((GT_U8*)charPtr)[0] << 24) |   \
                             (((GT_U8*)charPtr)[1] << 16) |   \
                             (((GT_U8*)charPtr)[2] << 8)  |   \
                             (((GT_U8*)charPtr)[3]))

/* max number of bytes in buffer ,
  for rx/tx frames use dynamic malloc when needed */
#define MAX_BUFFER_SIZE_CNS     (64*1024)

/* max number of bytes in buffer , for reply */
#define MAX_BUFFER_SIZE_SINGLE_MESSAGE_CNS     (2*1024)
/* max number of socket connections to allow */
#define MAX_SOCKETS_CNS             20
/* max number of devices to be supported by this mechanism */
#define MAX_DEVICES_ON_BOARD_CNS    MAX_SOCKETS_CNS
/* define represent that the data need to be sent to the 'NIC' (NIC client)*/
#define NIC_CLIENT  0xFFEEFFEE
/* indication that a client that represent device/application is not ready --
   meaning connection not established yet */
#define CLIENT_NOT_READY    0xaabbccdd
/* define represent that the socket in context is
    CONNECTION_INFO_STC::serverSocket and not one of
    CONNECTION_INFO_STC::socketsArray[] */
#define SERVER_SOCKET    0xffffffff
/* define represent index of first socket in CONNECTION_INFO_STC::socketsArray[] */
#define FIRST_SOCKET    0
/* array that maps the deviceId to client index .
   index in the array is the deviceId.
   value is the clientIndex in the array of: socketsArray
*/
static GT_U32 mapDeviceToClientIndex[MAX_DEVICES_ON_BOARD_CNS];

/*
 * Typedef: struct NIC_DEVICE_INFO
 *
 * Description:
 *      info about NIC device
 *
 * Fields:
 *      nicInitialized  : does the info in the other fields of NIC_DEVICE_INFO
 *                        is valid
 *      nicDevNum  :  the device number of the device that connected to the NIC
 *                  of CPU (via SLAN).
 *
 * Comments:
 */

typedef struct{
    GT_BOOL     nicInitialized;
    GT_U32      nicDevNum;
}NIC_DEVICE_INFO;

/* info about the nic device */
static NIC_DEVICE_INFO nicInfo = {GT_FALSE , 0};

/*
 * Typedef: enum CONNECTION_TYPE_ENT
 *
 * Description:
 *      list the different type of connection
 *
 * Fields:
 *      CONNECTION_TYPE_SYNCH_E  : connection used for messages that Application
 *                          send to Device , and Device will reply on this connection (read register reply)
 *      CONNECTION_TYPE_ASYNCH_E  : connection used for messages that Device
 *                          send to Application , and Application will reply on this connection (read DMA reply)
 *
 *      CONNECTION_TYPE_NIC_RX_FRAME_E : connection used for messages that Rx the NIC
 *                                Rx is send from Device Application , So
 *                                Application Side will need task to wait for those frames
 *
 * Comments:
 */
typedef enum{
    CONNECTION_TYPE_SYNCH_E  ,
    CONNECTION_TYPE_ASYNCH_E ,
    CONNECTION_TYPE_NIC_RX_FRAME_E,
    CONNECTION_TYPE_LAST_E
}CONNECTION_TYPE_ENT;

/* macro to build a string out of symbol name*/
#define SYMBOL_TO_STRING(_symbol)    #_symbol
/* array of strings of the connection types , for debug */
static char *connectionTypeStr[]=
{
    SYMBOL_TO_STRING(CONNECTION_TYPE_SYNCH_E),
    SYMBOL_TO_STRING(CONNECTION_TYPE_ASYNCH_E),
    SYMBOL_TO_STRING(CONNECTION_TYPE_NIC_RX_FRAME_E),
    SYMBOL_TO_STRING(CONNECTION_TYPE_LAST_E)
};


/*
 * Typedef: struct SOCKET_INFO_STC
 *
 * Description:
 *      Describe the socket info
 *
 * Fields:
 *      ready : socket ready to be used
 *      sockId : the socket Id
 *      protocolPort : the TCP port
 *      sockAddr     : A pointer to a socket address structure, represents the
 *                     remote address to connect to. For TCP, it is the server
 *                     address.
 *      sockAddrLen  : Length of the size used in sockAddr
 *      ipAddressStr :string that hold the IP address of the connection in format of "192.21.246.32"
 * Comments:
 */
typedef struct{
    GT_U32          ready;
    GT_SOCKET_FD    sockId;
    GT_U16          protocolPort;
    GT_VOID *       sockAddr;
    GT_SIZE         sockAddrLen;
    /* string that hold the IP address of the socket in format of "192.21.246.32" */
    GT_CHAR  ipAddressStr[SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS];
}SOCKET_INFO_STC;

/*
 * Typedef: struct CONNECTION_INFO_STC
 *
 * Description:
 *      Describe the connection info
 *
 * Fields:
 *      valid : is valid
 *      connectionType : the connection type
 *
 *      taskFuncPtr : pointer to the task function
 *      taskId : the Id of the task
 *      readBufSize : the read buffer size (in bytes)
 *      readBufPtr - pointer to buffer to read into the incoming messages
 *      replyBufSize - the reply buffer size (in bytes)
 *      replyBufPtr - pointer to buffer to use for reply messages
 *      getReplyProtect - semaphore protecting the actions of "wait for reply"
 *                      to simplify the working in context of several "read"
 *                      actions from sender , that waits for reply from other
 *                      side. there may be several task issuing the "read", so
 *                      the reply need to know which task to wake with the
 *                      returned info.
 *
 *      socketServerSide : the socket of this task is server/client
 *      serverSocket   : info about the server socket .
 *                       the socket id ,of the server that accept on it clients.
 *                       relevant only on server side , on accept task.
 *
 *      numSockets   - on server this is current number of sockets that this connection has
 *                     on client this is aggregation number of the clients that
 *                          will want to use same Dispatcher task
 *      socketsArray - array of sockets with the info about them
 *
 *      cookie    - pointer to 'cookie' , this cookie is managed by 'Enhanced
 *                  processes' , that need extra dedicated info per connection.
 *                  see broker implementation that use this option.
 *
 *      debugTask - ability to debug task in Tornado
 *
 * Comments:
 */
typedef struct{
    GT_BOOL         valid;
    CONNECTION_TYPE_ENT connectionType;

    TASK_PTR_FUN    taskFuncPtr;
    GT_TASK_HANDLE  taskId;
    GT_U32          readBufSize;
    GT_U8           *readBufPtr;
    GT_U32          replyBufSize;
    GT_U8           *replyBufPtr;
    GT_MUTEX        getReplyProtect;

    GT_BOOL         socketServerSide;
    SOCKET_INFO_STC serverSocket;/* relevant when socketServerSide == GT_TRUE */
    /****************************************************************************/
    /* info about the sockets on this connection                                */
    /* this array on server will hold the replication of connections to clients */
    /* on client it can be used as aggregation of clients connected to several  */
    /* servers handled by same connection task dispatcher                       */
    /****************************************************************************/
    GT_U32          numSockets;
    SOCKET_INFO_STC socketsArray[MAX_SOCKETS_CNS];

    void*           cookie;

    GT_U32          debugTask;
}CONNECTION_INFO_STC;

/* array of pointers to the connections info.
   each pointer is dynamically allocated if needed , otherwise it is NULL
   each pointer will hold array of connection used for the connection type.
*/
static CONNECTION_INFO_STC *connectionsArray[CONNECTION_TYPE_LAST_E]={NULL};

/*
 * Typedef: enum DISTRIBUTED_MESSAGE_TYPE_ENT
 *
 * Description:
 *      list the different type of messages
 *
 * Fields:
 *      DISTRIBUTED_MESSAGE_TYPE_REGISTER_READ_E  :
 *          when such message received on asic side :
 *              got message with request for read register/memory PCI internal or regular or smi register
 *          when such message received on application side :
 *              got message with the read data
 *      DISTRIBUTED_MESSAGE_TYPE_REGISTER_WRITE_E : write register/memory PCI
 *              internal or regular register or smi
 *      DISTRIBUTED_MESSAGE_TYPE_NIC_TX_FRAME_E : send Ethernet frame from CPU's
 *              Nic (from CPU to PP)
 *      DISTRIBUTED_MESSAGE_TYPE_NIC_RX_FRAME_E : send Ethernet frame to CPU's
 *              Nic (from PP to CPU)
 *      DISTRIBUTED_MESSAGE_TYPE_INTERRUPT_SET_E : set interrupt line of device
 *      DISTRIBUTED_MESSAGE_TYPE_DMA_READ_E :
 *          when such message received on application side :
 *              got message with request for read from DMA memory
 *          when such message received on asic side :
 *              got message with the read data
 *      DISTRIBUTED_MESSAGE_TYPE_DMA_WRITE_E : write to DMA memory
 *
 *      DISTRIBUTED_MESSAGE_TYPE_DEVICE_TO_APPLICATION_INIT_PARAM_E : the Asic side send
 *                  the Application Side initialization parameters that the
 *                  Application need in run time.
 *      DISTRIBUTED_MESSAGE_TYPE_RESET_E : Reset message from the application to all
 *                            distributed processes in the system
 *      DISTRIBUTED_MESSAGE_TYPE_DEBUG_LEVEL_SET : set debug level of the distributed mechanism
 *
 *      DISTRIBUTED_MESSAGE_TYPE_APP_PID_E - application send to the broker it's
 *                  processId , so broker can signal application on interrupt
 *      DISTRIBUTED_MESSAGE_TYPE_REGISTER_DMA_E - application send to the broker
 *                  info about a DMA shared memory. So broker can read/write from
 *                  this shared memory without the application knowledge.
 *      DISTRIBUTED_MESSAGE_TYPE_REGISTER_INTERRUPT_E - application send to the
 *                  broker request to register itself as one that want to be
 *                  signaled when interrupt arrive.
 *      DISTRIBUTED_MESSAGE_TYPE_UNMASK_INTERRUPT_E - application send to the
 *                  broker request that broker will be able to signal
 *                  application on interrupt.
 *                  NOTE : the issue is that once the broker signal the application
 *                  about interrupt, the broker will not signal it again even if
 *                  received another interrupt from device , until application
 *                  will send DISTRIBUTED_MESSAGE_TYPE_UNMASK_INTERRUPT_E
 *                  NOTE : see mode of BROKER_INTERRUPT_MASK_MODE
 *     DISTRIBUTED_MESSAGE_TYPE_MASK_INTERRUPT_E - application send to the
 *                  broker request that broker will NOT be able to signal
 *                  application on interrupt.
 *                  NOTE : the issue is that once the broker signal the application
 *                  about interrupt, the application wants to silent the broker.
 *                  application will 'unmask' the broker when application ready
 *                  for new signal.
 *                  NOTE : see mode of BROKER_INTERRUPT_MASK_MODE
 *
 * Comments:
 */

typedef enum{
    DISTRIBUTED_MESSAGE_TYPE_REGISTER_READ_E  = MSG_TYPE_READ,
    DISTRIBUTED_MESSAGE_TYPE_REGISTER_WRITE_E = MSG_TYPE_WRITE,
    DISTRIBUTED_MESSAGE_TYPE_NIC_TX_FRAME_E   = MSG_TYPE_NIC_TX_FRAME,
    DISTRIBUTED_MESSAGE_TYPE_NIC_RX_FRAME_E   = MSG_TYPE_NIC_RX_FRAME,
    DISTRIBUTED_MESSAGE_TYPE_INTERRUPT_SET_E  = MSG_TYPE_INTERRUPT_SET,
    DISTRIBUTED_MESSAGE_TYPE_DMA_READ_E       = MSG_TYPE_DMA_READ,
    DISTRIBUTED_MESSAGE_TYPE_DMA_WRITE_E      = MSG_TYPE_DMA_WRITE,

    DISTRIBUTED_MESSAGE_TYPE_DEVICE_TO_APPLICATION_INIT_PARAM_E = MSG_TYPE_INIT_PARAMS,

    DISTRIBUTED_MESSAGE_TYPE_RESET_E          = MSG_TYPE_INIT_RESET,
    DISTRIBUTED_MESSAGE_TYPE_DEBUG_LEVEL_SET  = MSG_TYPE_DEBUG_LEVEL_SET,

    /* response (reply) */
    DISTRIBUTED_MESSAGE_TYPE_REGISTER_READ_REPLY_E  = MSG_TYPE_READ_RESPONSE,
    DISTRIBUTED_MESSAGE_TYPE_DMA_READ_REPLY_E       = MSG_TYPE_DMA_READ_RESPONSE,

    /* the following messages are exchanged only between application(s) and broker */
    DISTRIBUTED_MESSAGE_TYPE_APP_PID_E              = MSG_TYPE_APP_PID,
    DISTRIBUTED_MESSAGE_TYPE_REGISTER_DMA_E         = MSG_TYPE_REGISTER_DMA,
    DISTRIBUTED_MESSAGE_TYPE_REGISTER_INTERRUPT_E   = MSG_TYPE_REGISTER_INTERRUPT,
    DISTRIBUTED_MESSAGE_TYPE_UNMASK_INTERRUPT_E     = MSG_TYPE_UNMASK_INTERRUPT,
    DISTRIBUTED_MESSAGE_TYPE_MASK_INTERRUPT_E       = MSG_TYPE_MASK_INTERRUPT,

    DISTRIBUTED_MESSAGE_TYPE_LAST_E = MSG_TYPE_LAST/* must be last */
}DISTRIBUTED_MESSAGE_TYPE_ENT;


/*
 * Typedef: struct DISTRIBUTED_MESSAGE_SYNC_INFO_STC
 *
 * Description:
 *      Describe the info for synchronize the send of "get" and the "reply"
 *      for it
 *
 * Fields:
 *      dataPtr : pointer to the data were the returned data should be placed
 *      semSync : semaphores to synchronize the sender of message and the reply
 *                  for it
 *    dataIsWords : the data is words or bytes
 *                     (if words --> swap network order to cpu order
 *                      if bytes --> NO swap network order to cpu order )
 * Comments:
 */
typedef struct{
    GT_VOID*    dataPtr;
    GT_SEM      semSync;
    GT_U32      dataIsWords;
}DISTRIBUTED_MESSAGE_SYNC_INFO_STC;

/*
 * enum (no name)
 *
 * Description:
 *      value of protections
 *
 * Fields:
 *      PROTECT_REGISTER_READ_INDEX_E : index of protection on 'Read register'
 *      PROTECT_DMA_READ_INDEX_E : index of protection on 'Read DMA'
 * Comments:
 */
enum{
    PROTECT_REGISTER_READ_INDEX_E,
    PROTECT_DMA_READ_INDEX_E,

    PROTECT_LAST_INDEX_E
};

/* array of semaphores to synchronize the sender of message and the reply for it*/
static DISTRIBUTED_MESSAGE_SYNC_INFO_STC distributedMessageSync[PROTECT_LAST_INDEX_E] =
{
    {NULL,((GT_SEM)0),GT_FALSE}
};

/* flag to open logger on this side */
#define FLAG_LOG_REG            (1<<DEBUG_SIM_DISTRIBUTED_TRACE_FLAG_REGISTER_E)
#define FLAG_LOG_DMA            (1<<DEBUG_SIM_DISTRIBUTED_TRACE_FLAG_DMA_E)
#define FLAG_LOG_INTERRUPT      (1<<DEBUG_SIM_DISTRIBUTED_TRACE_FLAG_INTERRUPT_E)
#define FLAG_LOG_NIC_RX         (1<<DEBUG_SIM_DISTRIBUTED_TRACE_FLAG_NIC_RX_E)
#define FLAG_LOG_NIC_TX         (1<<DEBUG_SIM_DISTRIBUTED_TRACE_FLAG_NIC_TX_E)
#define FLAG_LOG_SEQUENCE_NUM   (1<<DEBUG_SIM_DISTRIBUTED_TRACE_FLAG_SEQUENCE_NUM_E)
#define FLAG_LOG_CONNECTION_INIT (1<<DEBUG_SIM_DISTRIBUTED_TRACE_FLAG_CONNECTION_INIT_E)
#define FLAG_LOG_REMOTE_INIT    (1<<DEBUG_SIM_DISTRIBUTED_TRACE_FLAG_REMOTE_INIT_E)
#define FLAG_LOG_SOCKET_ID      (1<<DEBUG_SIM_DISTRIBUTED_TRACE_FLAG_SOCKET_ID_E)
#define FLAG_LOG_DATA_READ_WRITE (1<<DEBUG_SIM_DISTRIBUTED_TRACE_FLAG_DATA_READ_WRITE_E)

/* main controller of debugging activities */
static GT_U32   doDebug = 0;

/* open all the flags when need to debug problems */
static GT_U32   logInfoUsed = 0xFFFFFFFF;/* all flags */

/* change/set flags on runtime */
extern void simDistributedDebugFlagSet(GT_U32 flags)
{
    logInfoUsed = flags;
}


#define MSG_MEMORY_READ_E             REGISTER_MEMORY_ACCESS
#define MSG_MEMORY_WRITE_E            REGISTER_MEMORY_ACCESS
#define MSG_MEMORY_READ_PCI_E         PCI_REGISTERS_ACCESS
#define MSG_MEMORY_WRITE_PCI_E        PCI_REGISTERS_ACCESS


#define OPCODE_TO_STR(read,accessType)                                     \
    (((read) &&(accessType == REGISTER_MEMORY_ACCESS))   ? "read" :        \
     ((!read)&&(accessType == REGISTER_MEMORY_ACCESS))   ? "write" :       \
     ((read) &&(accessType == PCI_REGISTERS_ACCESS))     ? "read_pci" :    \
     ((!read)&&(accessType == PCI_REGISTERS_ACCESS))     ? "write_pci" :   \
                                                           "???")
#define DMA_READ    0
#define DMA_WRITE   1

#define DMA_ACT_TO_STR(readOrWrite)    \
    ((readOrWrite == DMA_READ)? "read" : "write")

/* macro to log the registers read/write sent activity */
#define LOG_INFO_REGISTER_SENT(read,opCode,deviceId,address,length)    \
    if(logInfoUsed & FLAG_LOG_REG)                                     \
    {                                                                  \
        printf("reg : [%s] dev[%d] address[0x%8.8x] length[%d] \n", \
                OPCODE_TO_STR(read,opCode),deviceId,address,length);   \
    }

/* macro to log the registers write data sent activity */
#define LOG_INFO_WRITE_DATA_SENT(length,dataPtr)    \
    if(logInfoUsed & FLAG_LOG_DATA_READ_WRITE)         \
    {                                                  \
        GT_U32  ii;                                    \
        printf("write data Of");                       \
        for(ii = 0 ; ii < length ; ii++)               \
        {                                              \
            printf("[0x%8.8x]",(dataPtr)[ii]);        \
        }                                              \
        printf("\n");                                  \
    }

/* macro to log the registers read data received activity */
#define LOG_INFO_READ_DATA_REPLY(length,dataPtr)    \
    if(logInfoUsed & FLAG_LOG_DATA_READ_WRITE)         \
    {                                                  \
        GT_U32  ii;                                    \
        printf("read data Of");                        \
        for(ii = 0 ; ii < length ; ii++)               \
        {                                              \
            printf("[0x%8.8x]",(dataPtr)[ii]);          \
        }                                              \
        printf("\n");                                  \
    }



/* macro to log the dma read/write sent activity */
#define LOG_INFO_DMA_SENT(readOrWrite,address,length)                  \
    if(logInfoUsed & FLAG_LOG_DMA)                                     \
    {                                                                  \
        GT_U32  _readOrWrite = readOrWrite;                            \
        printf("dma : [%s] address[0x%8.8x] length[%d] \n",          \
            DMA_ACT_TO_STR(_readOrWrite),address,length);              \
    }

#define LOG_INFO_SEQUENCE_NUM_RECEIVE(sequenceNum)                     \
    if(logInfoUsed & FLAG_LOG_SEQUENCE_NUM)                            \
    {                                                                  \
        printf("sequenceNum receive: [%d] \n",sequenceNum);           \
    }

#define LOG_INFO_CONNECTION_INIT(connectionType,sockId)                \
    if(logInfoUsed & FLAG_LOG_CONNECTION_INIT)                         \
    {                                                                  \
        printf("connected :[%s] sockId [%d] \n",                       \
                iniFileTcpPortsStr[connectionType],sockId);             \
    }

#define LOG_INFO_REMOTE_INIT_NUM_DEVICES(numDevices)                   \
    if(logInfoUsed & FLAG_LOG_REMOTE_INIT)                             \
    {                                                                  \
        printf("remote init :numDevices [%d] \n",numDevices);          \
    }

#define LOG_INFO_REMOTE_INIT_DEVICE_INFO(deviceId,deviceHwId,interruptLine,isPp,addressCompletionStatus,nicDevice)\
    if(logInfoUsed & FLAG_LOG_REMOTE_INIT)                             \
    {                                                                  \
        printf("remote init :deviceId[%d],deviceHwId[0x%x],interruptLine[0x%x],isPp[%d],addressCompletionStatus[%d] nicDevice[%d]\n",\
                deviceId,deviceHwId,interruptLine,isPp,addressCompletionStatus,nicDevice);  \
    }

#define LOG_INFO_REMOTE_INTERRUPT_SET(deviceId)                        \
    if(logInfoUsed & FLAG_LOG_INTERRUPT)                               \
    {                                                                  \
        printf("interrupt :deviceId[%d] \n",deviceId);                 \
    }

/* no need for this message for now , it is just put extra print */
#define LOG_SOCKET_ID_SEND(sockId)
/*    if(logInfoUsed & FLAG_LOG_SOCKET_ID)                  \
    {                                                     \
        printf("send :socketId[%ld] \n",sockId);          \
    }*/

#define LOG_SOCKET_ID_RECEIVE(sockId)                     \
    if(logInfoUsed & FLAG_LOG_SOCKET_ID)                  \
    {                                                     \
        printf("receive :socketId[%d] \n",sockId);       \
    }

/*
 * Typedef: enum DISTRIBUTED_SIDE_ENT
 *
 * Description:
 *      the sides of a socket connection : server, client
 *
 * Fields:
 *      SERVER_SIDE_E : the server side of the connection
 *      CLIENT_SIDE_E : the client side of the connection
 *
 * Comments:
 */
typedef enum{
    SERVER_SIDE_E,
    CLIENT_SIDE_E
}DISTRIBUTED_SIDE_ENT;

/* the number of 'sides' that the process need to support :
   the application has only 1 side : connecting to remote simulation / broker
                                     this connection is 'client'
   the broker has 2 sides : one to connect to application(s)
                                     this connection is 'server'
                            and one side to remote simulation
                                     this connection is 'client'
   the 'Interface bridge bus' has 2 sides : one to connect to remote side (broker/application)
                                     this connection is 'server'
                            and one side to local devices
                                     this connection is 'client'
   the devices has only 1 side : connecting to remote application / broker
                                     this connection is 'server'
*/
#define MAX_SIDES   2
/* actual number of sides that this process uses */
static GT_U32   numOfSides = 0;

/*
 * Typedef: struct DISTRIBUTED_SIDE_STC
 *
 * Description:
 *      the info about a distributed side
 *
 * Fields:
 *      type : the type of side : server/client
 *      numConnections : number of connection that this side holds
 *
 * Comments:
 */
typedef struct{
    DISTRIBUTED_SIDE_ENT type;
    GT_U32               numConnections;
}DISTRIBUTED_SIDE_STC;
/* array of distributed sides */
static DISTRIBUTED_SIDE_STC distributedSides[MAX_SIDES]={{0,0}};

/*********************************/
/**** start of INI file info *****/
/*********************************/
/* the name of the section [distribution] */
static char iniFileDistSectionStr[] = "distribution";
/* the name of the field server_ip_addr in section [distribution] */
static char iniFileDistServerIpStr[] = "server_ip_addr";
/* the name of the tcp port as appear in the INI file in section [distribution]*/
static char *iniFileTcpPortsStr[CONNECTION_TYPE_LAST_E]={
    "tcp_port_synchronic",
    "tcp_port_asynchronous",
    "tcp_port_nic_rx"
};

/* the name of the section [interface_bus] */
static char iniFileBoardBusSectionStr[] = "interface_bus";

/* the name of the section [broker] */
static char iniFileBrokerSectionStr[] = "broker";

/* the name of the tcp port as appear in the INI file -- for connecting bus to devices
   in section [interface_bus] */
static char *iniFileBoardSectionTcpPortsStr[CONNECTION_TYPE_LAST_E]={
    "board_section_%ld_tcp_port_synchronic",
    "board_section_%ld_tcp_port_asynchronous",
    "board_section_%ld_tcp_port_nic_rx"
};


/* the name of the tcp port as appear in the INI file -- for connecting bus to devices
   in section [interface_bus] */
static char *iniFileBrokerTcpPortsStr[CONNECTION_TYPE_LAST_E]={
    "client_%ld_tcp_port_synchronic",
    "client_%ld_tcp_port_asynchronous",
    "client_%ld_tcp_port_nic_rx"
};

/* application that will set application_x_not_register_interrupt = 1 will NOT
   register itself to the broker */
static char iniFileAppViaBrokerRegisterInterruptStr[] = "application_%ld_not_register_interrupt";

/*******************************/
/**** end of INI file info *****/
/*******************************/
/* a string that represent IP internal loopback */
static char loopbackIpStr[]="127.0.0.1";

static void brokerInitPart1(void);
static void busInitPart1(void);

static CONNECTION_INFO_STC* convertMessageTypeToConnectionIndex(
    IN DISTRIBUTED_MESSAGE_TYPE_ENT msgType
);

static void brokerMessageExtended
(
    IN DISTRIBUTED_MESSAGE_TYPE_ENT messageType,
    IN CONNECTION_INFO_STC *connectionPtr,
    IN GT_U32               offsetInBuf,
    IN MSG_HDR              *messageHeaderPtr
);

typedef void  (*MSG_SPECIFIC_TREATMENT_INTERRUPT_SET_FUN)
(
    IN  GT_U32        deviceId
);

typedef void  (*MSG_SPECIFIC_TREATMENT_REGISTER_READ_FUN)
(
    IN READ_WRITE_ACCESS_TYPE accessType,
    IN GT_U32 deviceId,
    IN GT_U32 address,
    IN GT_U32 memSize,
    OUT GT_U32 * memPtr,
    IN CONNECTION_INFO_STC *connectionPtr
);

typedef void (*MSG_SPECIFIC_TREATMENT_REGISTER_WRITE_FUN)
(
    IN READ_WRITE_ACCESS_TYPE accessType,
    IN GT_U32 deviceId,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_U32 * memPtr
);

typedef void (*MSG_SPECIFIC_TREATMENT_DMA_READ_FUN)
(
    IN GT_U32 deviceId,
    IN GT_U32 address,
    IN GT_U32 memSize,
    OUT GT_U32 * memPtr,
    IN GT_U32  dataIsWords
);

typedef void (*MSG_SPECIFIC_TREATMENT_DMA_WRITE_FUN)
(
    IN GT_U32 deviceId,
    IN GT_U32 address,
    IN GT_U32 memSize,
    INOUT GT_U32 * memPtr,
    IN GT_U32  dataIsWords
);

typedef void (*MSG_SPECIFIC_TREATMENT_DEVICE_INIT_FUN)
(
    IN SIM_DISTRIBUTED_INIT_DEVICE_STC *deviceInfoPtr
);

typedef void (*CONNECTION_INIT_COOKIE_FUN)
(
    IN CONNECTION_INFO_STC *connectionPtr
);


typedef void (*MSG_RESET_FUN)(
    void
);


/* cb for initializing the connection cookie */
static CONNECTION_INIT_COOKIE_FUN                  connectionInitCookie = NULL;


/* messages on the asynchronous sockets */
static MSG_SPECIFIC_TREATMENT_INTERRUPT_SET_FUN    msgInterruptSet = NULL;
static MSG_SPECIFIC_TREATMENT_DMA_READ_FUN         msgDmaRead = NULL;
static MSG_SPECIFIC_TREATMENT_DMA_WRITE_FUN        msgDmaWrite = NULL;

/* messages on the synchronic sockets */
static MSG_SPECIFIC_TREATMENT_REGISTER_READ_FUN    msgRegisterRead = NULL;
static MSG_SPECIFIC_TREATMENT_REGISTER_WRITE_FUN   msgRegisterWrite = NULL;
static MSG_SPECIFIC_TREATMENT_DEVICE_INIT_FUN      msgDeviceInit = NULL;

static MSG_RESET_FUN                               msgReset = NULL;

/* define short names for the process that currently running */
static GT_U32   _broker = 0;
static GT_U32   _bus = 0;
static GT_U32   _app = 0;
static GT_U32   _dev = 0;

#define BROKER_USE_SOCKET       1
#define BROKER_NOT_USE_SOCKET   2
GT_U32   brokerDmaMode = BROKER_USE_SOCKET;
GT_U32   brokerInterruptMode = BROKER_USE_SOCKET;

/* define the indexes in the array of distributedSides[] and in
    connectionsArray[don't care][] for process 'bus' for it's single client side
    (single side but with multi connections !) */
#define BUS_CLIENT_INDEX    0
/* define the indexes in the array of distributedSides[] and in
    connectionsArray[don't care][] for process 'bus' for it's single server side */
#define BUS_SERVER_INDEX    1

/* define the indexes in the array of distributedSides[] and in
    connectionsArray[don't care][] for process 'broker' for it's single client side */
#define BROKER_CLIENT_INDEX    0
/* define the indexes in the array of distributedSides[] and in
    connectionsArray[don't care][] for process 'broker' for it's first server side */
#define BROKER_SERVER_INDEX    1

/* TCP port that is under 1024 , that indicate that it is reserved for internal
   use :

   this defines that the TCP port not used because the application that
   connected to the broker has no asynch socket
*/
#define APP_VIA_BROKER_NO_ASYNCH_CHANNEL_CNS    1

/* dummy reset value that process is returning when ended */
#define REMOTE_RESET_CODE_CNS   1234

static void simDistributedRemoteResetSend
(
    GT_U32  clientIndex
);

/* indication that we are in the 'RESET' handling */
static GT_U32   resetSystem = 0;

/* indication that we do clean exit from a message or from external call
   (terminal command / application call) */
static GT_U32   externalCall = 0;

/*******************************************************************************
*   printConnectionSockets
*
* DESCRIPTION:
*       function to print the sockets info
*
* INPUTS:
*       connectionPtr - pointer to the connection to print it's sockets info
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void printConnectionSockets(
    IN CONNECTION_INFO_STC *connectionPtr
)
{
    SOCKET_INFO_STC *socketPtr;
    GT_U32  ii;
    GT_U32  maxSockets;

    printf("connection of : %s \n",
        connectionTypeStr[connectionPtr->connectionType]);

    if(connectionPtr->socketServerSide == GT_TRUE && connectionPtr->numSockets == 0)
    {
        socketPtr = &connectionPtr->serverSocket;
        maxSockets = 1;
    }
    else
    {
        socketPtr = &connectionPtr->socketsArray[FIRST_SOCKET];
        maxSockets = connectionPtr->numSockets;
    }

    for(ii = 0 ; ii < maxSockets ; ii++ , socketPtr++)
    {
        printf(" act as [%s] on IP[%s],tcp port[%d] \n",
            connectionPtr->socketServerSide == GT_TRUE ? "server":"client",
            socketPtr->ipAddressStr,
            (GT_U32)socketPtr->protocolPort
        );

        if(socketPtr->sockId)
        {
            printf("with sockId[%d]\n",(GT_U32)socketPtr->sockId);
        }
    }

    printf("\n\n");
}

/*******************************************************************************
*   convertDeviceIdToClientIndex
*
* DESCRIPTION:
*       function convert deviceId to client index , so it is possible to access
*       array connectionPtr->socketsArray[]
*
* INPUTS:
*       deviceId - device id
*
* OUTPUTS:
*       clientIndexPtr - pointer to the index of the client
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void convertDeviceIdToClientIndex(
    IN DISTRIBUTED_MESSAGE_TYPE_ENT msgType,
    IN GT_U32   deviceId,
    OUT GT_U32  *clientIndexPtr
)
{
    GT_U32  convert = 0;

    if(_bus)
    {
        switch(msgType)
        {
            case DISTRIBUTED_MESSAGE_TYPE_REGISTER_READ_E:
            case DISTRIBUTED_MESSAGE_TYPE_REGISTER_WRITE_E:
                /* the bus convert only when send to the devices */
                convert = 1;
                break;

            case DISTRIBUTED_MESSAGE_TYPE_INTERRUPT_SET_E:
            case DISTRIBUTED_MESSAGE_TYPE_DMA_READ_E:
            case DISTRIBUTED_MESSAGE_TYPE_DMA_WRITE_E:
            case DISTRIBUTED_MESSAGE_TYPE_DEVICE_TO_APPLICATION_INIT_PARAM_E:
                break;

            case DISTRIBUTED_MESSAGE_TYPE_RESET_E:
            case DISTRIBUTED_MESSAGE_TYPE_DEBUG_LEVEL_SET:
                /* the bus convert only when send to the devices */
                convert = 1;
                break;

            default:
                skernelFatalError(" convertDeviceIdToClientIndex : not supported msgType[%ld]",(GT_U32)msgType);
                return;
        }
    }

    if(convert)
    {
        if((deviceId) >= MAX_DEVICES_ON_BOARD_CNS)
        {
            skernelFatalError(" convertDeviceIdToClientIndex : out of range index[%ld]",(deviceId));
        }

        while(mapDeviceToClientIndex[deviceId] == CLIENT_NOT_READY)
        {
            /* wait for client to wake up */
            SIM_OS_MAC(simOsSleep)(WAIT_SLEEP_TIME_CNS);
        }

        *(clientIndexPtr) = mapDeviceToClientIndex[deviceId];
    }
    else
    {
        *(clientIndexPtr) = FIRST_SOCKET;
    }

    return;
}

/*******************************************************************************
*   convertSocketIdToClientIndex
*
* DESCRIPTION:
*       function convert socketId to client index , so it is possible to access
*       array connectionPtr->socketsArray[]
*
* INPUTS:
*       connectionPtr - pointer to the connection info.
*       socketId - socket Id
*
* OUTPUTS:
*
* RETURNS:
*       index of the client
*
* COMMENTS:
*
*******************************************************************************/
static GT_U32 convertSocketIdToClientIndex(
    IN CONNECTION_INFO_STC *connectionPtr,
    IN GT_SOCKET_FD         socketId
)
{
    GT_U32  ii;

    while(1)
    {
        for(ii = 0 ; ii < connectionPtr->numSockets ;ii++)
        {
            if(connectionPtr->socketsArray[ii].sockId == socketId)
            {
                return ii;
            }
        }

        /* wait for client to wake up */
        SIM_OS_MAC(simOsSleep)(WAIT_SLEEP_TIME_CNS);
    }

}

/*******************************************************************************
* socketInit
*
* DESCRIPTION:
*       create TCP socket , and socket address , set socket mode to blocking
*       allow to destroy the socket before re-creating it
*
* INPUTS:
*       connectionPtr - pointer to connection info
*       clientIndex   - index of the client
*                       index SERVER_SOCKET will apply for the connectionPtr->serverSocket
*       destroyOld    - to destroy old socket connection before create new one
* OUTPUTS:
*       none
*
* RETURNS:
*       none
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static void socketInit(
    IN CONNECTION_INFO_STC *connectionPtr,
    IN GT_U32  clientIndex,
    IN GT_BOOL destroyOld
)
{
    SOCKET_INFO_STC *socketInfoPtr;

    if(clientIndex == SERVER_SOCKET)
    {
        socketInfoPtr = &connectionPtr->serverSocket;
    }
    else
    {
        socketInfoPtr = &connectionPtr->socketsArray[clientIndex];
    }

    if(destroyOld == GT_TRUE)
    {
        /* clean-up of old socket */
        SIM_OS_MAC(osSocketSetSocketNoLinger)(socketInfoPtr->sockId);
        SIM_OS_MAC(osSocketShutDown)(socketInfoPtr->sockId,SOCKET_SHUTDOWN_CLOSE_ALL);
        SIM_OS_MAC(osSocketTcpDestroy)(socketInfoPtr->sockId);
    }

    /* create TCP socket */
    if ((socketInfoPtr->sockId = SIM_OS_MAC(osSocketTcpCreate)(GT_SOCKET_DEFAULT_SIZE)) < 0)
    {
        skernelFatalError(" socketInit : osSocketTcpCreate \n");
    }

    if(socketInfoPtr->sockAddr == NULL)
    {
        /* Build TCP/IP address to be used across all other functions */
        if (GT_OK != SIM_OS_MAC(osSocketCreateAddr)(socketInfoPtr->ipAddressStr, socketInfoPtr->protocolPort,
                                          &socketInfoPtr->sockAddr, &socketInfoPtr->sockAddrLen))
        {
            skernelFatalError(" socketInit : osSocketCreateAddr \n");
        }
    }

    /* set socket to blocking mode */
    if (GT_OK != SIM_OS_MAC(osSocketSetBlock)(socketInfoPtr->sockId))
    {
        skernelFatalError(" socketInit : osSocketSetBlock \n");
    }

}

/*******************************************************************************
*   connectionInit
*
* DESCRIPTION:
*       Init the simulation distribution functionality.
*       initialization is done according to global parameter sasicgSimulationRole
*
* INPUTS:
*       connectionPtr - pointer to the connection
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void connectionInit
(
    IN CONNECTION_INFO_STC *connectionPtr
)
{
    GT_TASK_PRIORITY_ENT taskPriority = GT_TASK_PRIORITY_ABOVE_NORMAL;

    if(connectionPtr->socketServerSide == GT_TRUE)
    {
        if(connectionPtr->serverSocket.protocolPort ==
            APP_VIA_BROKER_NO_ASYNCH_CHANNEL_CNS)
        {
            /* we skip this connection */
            return;
        }
    }
    else
    {
        if(connectionPtr->numSockets != 0 &&
           connectionPtr->socketsArray[0].protocolPort ==
            APP_VIA_BROKER_NO_ASYNCH_CHANNEL_CNS)
        {
            /* we skip this connection */
            return;
        }
    }

    /*************************************************************************/
    /* create preliminary environment , before creating the Dispatcher tasks */
    /*************************************************************************/

    switch(connectionPtr->connectionType)
    {
        case CONNECTION_TYPE_SYNCH_E:
        case CONNECTION_TYPE_ASYNCH_E:
            taskPriority = GT_TASK_PRIORITY_ABOVE_NORMAL; /* same as skernel task */
            connectionPtr->taskFuncPtr = &distributedDispatcherMainTask;
            break;
        case CONNECTION_TYPE_NIC_RX_FRAME_E:
            taskPriority = GT_TASK_PRIORITY_HIGHEST;/* same as SLAN task */
            connectionPtr->taskFuncPtr = &distributedDispatcherNicRxTask;
            break;
        default:
            skernelFatalError(" connectionInit : bad param");

    }

    printf(" going to connect to : \n");
    printConnectionSockets(connectionPtr);

    connectionPtr->getReplyProtect = SIM_OS_MAC(simOsMutexCreate)();
    if (connectionPtr->getReplyProtect == (GT_SEM)0)
    {
        skernelFatalError(" connectionInit : fail to create mutex - getReplyProtect");
    }

    connectionPtr->valid = GT_TRUE;

    /* do cookie connection init if needed , before creating the tasks */
    connectionPtr->cookie = NULL;/* must be done before connectionInitCookie*/
    if(connectionInitCookie)
    {
        connectionInitCookie(connectionPtr);
    }


    if(connectionPtr->connectionType == CONNECTION_TYPE_SYNCH_E ||
       connectionPtr->connectionType == CONNECTION_TYPE_ASYNCH_E)
    {
        /*******************************/
        /* create the Dispatcher tasks */
        /*******************************/
        connectionPtr->taskId = SIM_OS_MAC(simOsTaskCreate)(taskPriority,
                                                connectionPtr->taskFuncPtr,
                                                (void*)connectionPtr);

        if(connectionPtr->taskId == NULL)
        {
            skernelFatalError(" connectionInit : fail to create task");
        }
    }
    else
    {
        /* task not used in this case */
        connectionPtr->taskId = NULL;
        connectionPtr->taskFuncPtr = NULL;
    }

    if(connectionPtr->socketServerSide == GT_TRUE)
    {
        /*************************************/
        /* the server side of the connection */
        /*************************************/

        /* init the socket */
        socketInit(connectionPtr,SERVER_SOCKET,GT_FALSE);

        printf(" initialized socket : \n");
        printConnectionSockets(connectionPtr);

        /***********************************************************/
        /* server side of the accepting connection from the client */
        /***********************************************************/

        SIM_OS_MAC(simOsTaskCreate)(GT_TASK_PRIORITY_IDLE,
                        distributedDispatcherServerAccept,
                        (void*)connectionPtr);
    }

    return;
}

/*******************************************************************************
*   simDistributedInit
*
* DESCRIPTION:
*       Init the simulation distribution functionality.
*       initialization is done according to global parameter sasicgSimulationRole
*
* INPUTS:
*       None
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void simDistributedInit
(
    void
)
{
    GT_CHAR tmpStr[SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS];
    GT_U32  ii,jj,kk,mm;
    GT_U32  numConnections;
    GT_U16 tcpPorts[CONNECTION_TYPE_LAST_E];
    char ipAddressStr[SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS];
    GT_U16 tcpPorts_2[CONNECTION_TYPE_LAST_E];
    char ipAddressStr_2[SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS];
    static char ipAddressStrArray[256][SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS];
    static GT_U16 tcpPortsArray[256][CONNECTION_TYPE_LAST_E];
    CONNECTION_INFO_STC *connectionPtr;
    char    getStr[SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS];/* string to read from INI file */
    GT_U32 tmpValue;
    GT_U32 currIndex;

    if(doDebug == 0)
    {
        /* close the debugging */
        logInfoUsed = 0;
    }


    /* we do some sleep during the Init to allow all parts of the system that
       may have done 'RESET' , to close their connections , so we will not be
       connected to a dying connection */
    SIM_OS_MAC(simOsSleep)(RESET_WAIT_SLEEP_TIME_CNS * 4);

    for(ii = 0 ; ii < MAX_DEVICES_ON_BOARD_CNS; ii++)
    {
        mapDeviceToClientIndex[ii] = CLIENT_NOT_READY;
    }

    switch(sasicgSimulationRole)
    {
        case SASICG_SIMULATION_ROLE_DISTRIBUTED_APPLICATION_SIDE_E:
        case SASICG_SIMULATION_ROLE_DISTRIBUTED_APPLICATION_SIDE_VIA_BROKER_E:
            _app = 1;
            numOfSides = 1;
            distributedSides[0].type = CLIENT_SIDE_E;
            distributedSides[0].numConnections = 1;/*single connection*/
            break;
        case SASICG_SIMULATION_ROLE_DISTRIBUTED_ASIC_SIDE_VIA_INTERFACE_BUS_BRIDGE_E:
        case SASICG_SIMULATION_ROLE_DISTRIBUTED_ASIC_SIDE_E:
            _dev = 1;
            numOfSides = 1;
            distributedSides[0].type = SERVER_SIDE_E;
            distributedSides[0].numConnections = 1;/*single connection*/
            break;
        case SASICG_SIMULATION_ROLE_DISTRIBUTED_INTERFACE_BUS_BRIDGE_E:
            _bus = 1;
            numOfSides = 2;
            distributedSides[BUS_CLIENT_INDEX].type = CLIENT_SIDE_E;
            distributedSides[BUS_CLIENT_INDEX].numConnections = 1;/* single connection pointer with multiple 'clients' */

            distributedSides[BUS_SERVER_INDEX].type = SERVER_SIDE_E;
            distributedSides[BUS_SERVER_INDEX].numConnections = 1;/*single connection*/

            busInitPart1();/*extra special init */
            break;
        case SASICG_SIMULATION_ROLE_BROKER_E:
            _broker = 1;
            numOfSides = 2;
            distributedSides[BROKER_CLIENT_INDEX].type = CLIENT_SIDE_E;
            distributedSides[BROKER_CLIENT_INDEX].numConnections = 1;/*single connection*/

            distributedSides[BROKER_SERVER_INDEX].type = SERVER_SIDE_E;
            distributedSides[BROKER_SERVER_INDEX].numConnections = sinitNumOfApplications;

            brokerInitPart1(); /*extra special init */
            break;
        default:
            return;
    }

    if(_broker || sasicgSimulationRole == SASICG_SIMULATION_ROLE_DISTRIBUTED_APPLICATION_SIDE_VIA_BROKER_E)
    {
        if(SIM_OS_MAC(simOsGetCnfValue)(iniFileBrokerSectionStr, "dma_mode",
                                 SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS, getStr))
        {
            sscanf(getStr, "%d", &tmpValue);
            brokerDmaMode = tmpValue == 1 ? BROKER_NOT_USE_SOCKET : BROKER_USE_SOCKET;
        }

        if(SIM_OS_MAC(simOsGetCnfValue)(iniFileBrokerSectionStr, "interrupt_mode",
                                 SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS, getStr))
        {
            sscanf(getStr, "%d", &tmpValue);
            brokerInterruptMode = tmpValue == 1 ? BROKER_NOT_USE_SOCKET : BROKER_USE_SOCKET;
        }
    }

    switch(sasicgSimulationRole)
    {
        case SASICG_SIMULATION_ROLE_DISTRIBUTED_APPLICATION_SIDE_E:
            break;
        case SASICG_SIMULATION_ROLE_DISTRIBUTED_APPLICATION_SIDE_VIA_BROKER_E:

            for(ii = 0 ; ii < CONNECTION_TYPE_LAST_E; ii ++)
            {
                if(ii == CONNECTION_TYPE_NIC_RX_FRAME_E)
                {
                    /* not tested at this stage yet ... */
                    continue;
                }

                if((ii == CONNECTION_TYPE_ASYNCH_E) &&
                   (brokerDmaMode == BROKER_NOT_USE_SOCKET) &&
                   (brokerInterruptMode == BROKER_NOT_USE_SOCKET))
                {
                    /* no need TCP port to indicate
                       that we work in system where the application need to
                       define "shared memory" for DMA and to supply processID
                       to be signaled on incoming interrupt */
                       tcpPorts[ii] = APP_VIA_BROKER_NO_ASYNCH_CHANNEL_CNS;

                       continue;
                }

                sprintf(tmpStr, iniFileBrokerTcpPortsStr[ii] , sinitMultiProcessOwnApplicationId);

                /* get from the INI file the tcp ports needed for the connections between
                   the 2 sides */
                if(SIM_OS_MAC(simOsGetCnfValue)(iniFileBrokerSectionStr, tmpStr,
                                         SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS, getStr))
                {
                    sscanf(getStr, "%d", &tmpValue);
                    tcpPorts[ii] = (GT_U16)tmpValue;
                    if((GT_U32)tcpPorts[ii] != tmpValue)
                    {
                        skernelFatalError(" simDistributedInit : tcp port[%d] out of range \n",tmpValue);
                    }
                }
                else
                {
                    skernelFatalError(" simDistributedInit : missing in section [%s] the info on[%s] \n",
                        iniFileBrokerSectionStr,tmpStr);
                }
            }

            break;
        case SASICG_SIMULATION_ROLE_DISTRIBUTED_ASIC_SIDE_VIA_INTERFACE_BUS_BRIDGE_E:
            sprintf(tmpStr, "board_section%d_server_ip_addr", sinitOwnBoardSection.ownSectionId);

            if(! SIM_OS_MAC(simOsGetCnfValue)(iniFileBoardBusSectionStr,  tmpStr,
                                     SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS, ipAddressStr))
            {
                /* assign the default loopback port */
                strcpy(ipAddressStr,loopbackIpStr);
            }

            for(ii = 0 ; ii < CONNECTION_TYPE_LAST_E; ii ++)
            {
                if(ii == CONNECTION_TYPE_NIC_RX_FRAME_E)
                {
                    /* not tested at this stage yet ... */
                    continue;
                }
                sprintf(tmpStr, iniFileBoardSectionTcpPortsStr[ii] , sinitOwnBoardSection.ownSectionId);
                /* get from the INI file the tcp ports needed for the connections between
                   the 2 sides */
                if(SIM_OS_MAC(simOsGetCnfValue)(iniFileBoardBusSectionStr, tmpStr,
                                         SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS, getStr))
                {
                    sscanf(getStr, "%d", &tmpValue);
                    tcpPorts[ii] = (GT_U16)tmpValue;
                    if((GT_U32)tcpPorts[ii] != tmpValue)
                    {
                        skernelFatalError(" simDistributedInit : tcp port[%d] out of range \n",tmpValue);
                    }
                }
                else
                {
                    skernelFatalError(" simDistributedInit : missing in section [%s] the info on[%s] \n",
                        iniFileBoardBusSectionStr,tmpStr);
                }
            }


            /* walk through .. (no break) */
        case SASICG_SIMULATION_ROLE_DISTRIBUTED_ASIC_SIDE_E:
            break;
        case SASICG_SIMULATION_ROLE_DISTRIBUTED_INTERFACE_BUS_BRIDGE_E:
            for(ii = 0 ; ii < sinitInterfaceBusNumConnections ; ii++)
            {
                sprintf(tmpStr, "board_section_%d_server_ip_addr", ii);

                if(! SIM_OS_MAC(simOsGetCnfValue)(iniFileBoardBusSectionStr,  tmpStr,
                                         SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS, &ipAddressStrArray[ii][0]))
                {
                    /* assign the default loopback port */
                    strcpy(&ipAddressStrArray[ii][0],loopbackIpStr);
                }

                for(jj = 0 ; jj < CONNECTION_TYPE_LAST_E; jj ++)
                {
                    if(jj == CONNECTION_TYPE_NIC_RX_FRAME_E)
                    {
                        /* not tested at this stage yet ... */
                        continue;
                    }
                    sprintf(tmpStr, iniFileBoardSectionTcpPortsStr[jj] , ii);
                    /* get from the INI file the tcp ports needed for the connections between
                       the 2 sides */
                    if(SIM_OS_MAC(simOsGetCnfValue)(iniFileBoardBusSectionStr, tmpStr,
                                             SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS, getStr))
                    {
                        sscanf(getStr, "%d", &tmpValue);
                        tcpPortsArray[ii][jj] = (GT_U16)tmpValue;
                        if((GT_U32)tcpPortsArray[ii][jj] != tmpValue)
                        {
                            skernelFatalError(" simDistributedInit : tcp port[%d] out of range \n",tmpValue);
                        }
                    }
                    else
                    {
                        skernelFatalError(" simDistributedInit : missing in section [%s] the info on[%s] \n",
                            iniFileBoardBusSectionStr,tmpStr);
                    }
                }
            }

            break;
        case SASICG_SIMULATION_ROLE_BROKER_E:
            /* note this loop is for the TCP ports of the server side */
            /* the info of the client side is in code filled into : tcpPorts_2[xx] */
            for(jj = 0 ; jj < distributedSides[BROKER_SERVER_INDEX].numConnections; jj++)
            {
                for(ii = 0 ; ii < CONNECTION_TYPE_LAST_E; ii ++)
                {
                    if(ii == CONNECTION_TYPE_NIC_RX_FRAME_E)
                    {
                        /* not tested at this stage yet ... */
                        continue;
                    }

                    if((ii == CONNECTION_TYPE_ASYNCH_E) &&
                       (brokerDmaMode == BROKER_NOT_USE_SOCKET) &&
                       (brokerInterruptMode == BROKER_NOT_USE_SOCKET))
                    {
                        /* no need TCP port to indicate
                           that we work in system where the application need to
                           define "shared memory" for DMA and to supply processID
                           to be signaled on incoming interrupt */
                           tcpPortsArray[jj][ii] = APP_VIA_BROKER_NO_ASYNCH_CHANNEL_CNS;

                           continue;
                    }

                    sprintf(tmpStr, iniFileBrokerTcpPortsStr[ii] , jj);

                    /* get from the INI file the tcp ports needed for the connections between
                       the 2 sides */
                    if(SIM_OS_MAC(simOsGetCnfValue)(iniFileBrokerSectionStr, tmpStr,
                                             SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS, getStr))
                    {
                        sscanf(getStr, "%d", &tmpValue);
                        tcpPortsArray[jj][ii] = (GT_U16)tmpValue;
                        if(tcpPortsArray[jj][ii] != tmpValue)
                        {
                            skernelFatalError(" simDistributedInit : tcp port[%d] out of range \n",tmpValue);
                        }
                    }
                    else
                    {
                        skernelFatalError(" simDistributedInit : missing in section [%s] the info on[%s] \n",
                            iniFileBrokerSectionStr,tmpStr);
                    }
                } /* ii */
            }/* jj */
            break;
        default:
            return;
    }

    /* set values for global use */
    sasicgSimulationRoleIsApplication   = _app      ? GT_TRUE : GT_FALSE;
    sasicgSimulationRoleIsDevices       = _dev      ? GT_TRUE : GT_FALSE;
    sasicgSimulationRoleIsBroker        = _broker   ? GT_TRUE : GT_FALSE;
    sasicgSimulationRoleIsBus           = _bus      ? GT_TRUE : GT_FALSE;

    /* get from the INI file the IP address of the Server side */
    /* both sides need it , because server also need to know what IP interface
       to use */
    /*
        NOTE:
            1. for application this IP is one of the broker / device / interface bus. (as client)
            2. for broker this IP is one of the device / interface bus. (as client)
               the broker use 127.0.0.1 as 'loop back' IP for connection to application. (as server)
            3. for interface BUS this IP is own (the IP that application/broker attach to) (as server)
               the interface BUS use 127.0.0.2 as 'loop back' IP for connection to devices. (as client)
               (optional to use dedicated IP in INI file format , via board_section connection(as client)
            4. for device with no interface BUS this IP is my own (as server)
               for device with interface BUS use 127.0.0.2 as 'loop back' IP for connection to interface BUS. (as server)
               (optional to use dedicated IP in INI file format , via board_section connection(as server)
    */
    if(! SIM_OS_MAC(simOsGetCnfValue)(iniFileDistSectionStr,  iniFileDistServerIpStr,
                             SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS, ipAddressStr_2))
    {
        /* we not got IP of the Server side */
        /* assign the default loopback port */
        strcpy(ipAddressStr_2,loopbackIpStr);
    }

    for(ii = 0 ; ii < CONNECTION_TYPE_LAST_E; ii ++)
    {
        if(ii == CONNECTION_TYPE_NIC_RX_FRAME_E)
        {
            /* not tested at this stage yet ... */
            continue;
        }
        /* get from the INI file the tcp ports needed for the connections between
           the 2 sides */
        if(SIM_OS_MAC(simOsGetCnfValue)(iniFileDistSectionStr,  iniFileTcpPortsStr[ii],
                                 SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS, getStr))
        {
            sscanf(getStr, "%d", &tmpValue);
            tcpPorts_2[ii] = (GT_U16)tmpValue;
            if((GT_U32)tcpPorts_2[ii] != tmpValue)
            {
                skernelFatalError(" simDistributedInit : tcp port[%d] out of range \n",tmpValue);
            }

        }
        else
        {
            skernelFatalError(" simDistributedInit : missing in section [%s] the info on[%s] \n",
                iniFileDistSectionStr,iniFileTcpPortsStr[ii]);
        }
    }

    numConnections = 0;

    for(ii = 0 ; ii < numOfSides ; ii++)
    {
        numConnections += distributedSides[ii].numConnections;
    }

    /* create semaphore for the read register action --
       used on side that is waiting for read register reply */
    distributedMessageSync[PROTECT_REGISTER_READ_INDEX_E].semSync = SIM_OS_MAC(simOsSemCreate)(0,1);

    /* create semaphore for the read DMA action --
       used on side that is waiting for read dma reply */
    distributedMessageSync[PROTECT_DMA_READ_INDEX_E].semSync = SIM_OS_MAC(simOsSemCreate)(0,1);

    for(jj = 0 ;jj < CONNECTION_TYPE_LAST_E; jj++)
    {
        if(jj == CONNECTION_TYPE_NIC_RX_FRAME_E)
        {
            /* not tested at this stage yet ... */
            continue;
        }
        connectionsArray[jj] = malloc(numConnections * sizeof(CONNECTION_INFO_STC));

        memset(connectionsArray[jj],0,numConnections * sizeof(CONNECTION_INFO_STC));

        currIndex = 0;
        for(ii = 0 ; ii < numOfSides ; ii++)
        {
            for(kk = 0 ; kk < distributedSides[ii].numConnections ; kk++ , currIndex++)
            {
                connectionPtr = &connectionsArray[jj][currIndex];

                connectionPtr->connectionType = jj;

                connectionPtr->socketServerSide =
                     (distributedSides[ii].type == SERVER_SIDE_E) ? GT_TRUE : GT_FALSE;

                if(distributedSides[ii].type != SERVER_SIDE_E)
                {
                    /* set default of number of connections on the client side */
                    connectionPtr->numSockets = 1;

                    /* NOTE : the interface bus will override this value ,
                               but for all others it's ok */
                }

                /* set the IP of each connection , and the TCP port */
                switch(sasicgSimulationRole)
                {
                    case SASICG_SIMULATION_ROLE_DISTRIBUTED_APPLICATION_SIDE_E:
                        strcpy(connectionPtr->socketsArray[kk].ipAddressStr,ipAddressStr_2);
                        connectionPtr->socketsArray[kk].protocolPort = tcpPorts_2[jj];
                        break;
                    case SASICG_SIMULATION_ROLE_DISTRIBUTED_APPLICATION_SIDE_VIA_BROKER_E:
                        strcpy(connectionPtr->socketsArray[kk].ipAddressStr,ipAddressStr_2);
                        connectionPtr->socketsArray[kk].protocolPort = tcpPorts[jj];
                        break;
                    case SASICG_SIMULATION_ROLE_DISTRIBUTED_ASIC_SIDE_VIA_INTERFACE_BUS_BRIDGE_E:
                        strcpy(connectionPtr->socketsArray[kk].ipAddressStr,ipAddressStr);
                        connectionPtr->socketsArray[kk].protocolPort = tcpPorts[jj];
                        break;
                    case SASICG_SIMULATION_ROLE_DISTRIBUTED_ASIC_SIDE_E:
                        strcpy(connectionPtr->socketsArray[kk].ipAddressStr,ipAddressStr_2);
                        connectionPtr->socketsArray[kk].protocolPort = tcpPorts_2[jj];
                        break;
                    case SASICG_SIMULATION_ROLE_DISTRIBUTED_INTERFACE_BUS_BRIDGE_E:
                        if(distributedSides[ii].type == SERVER_SIDE_E)
                        {
                            strcpy(connectionPtr->socketsArray[kk].ipAddressStr,ipAddressStr_2);
                            connectionPtr->socketsArray[kk].protocolPort = tcpPorts_2[jj];
                        }
                        else
                        {

                            for(mm = 0 ; mm < sinitInterfaceBusNumConnections ; mm++)
                            {
                                strcpy(connectionPtr->socketsArray[mm].ipAddressStr,&ipAddressStrArray[mm][0]);
                                connectionPtr->socketsArray[mm].protocolPort = tcpPortsArray[mm][jj];
                            }

                            connectionPtr->numSockets = mm;
                        }
                        break;
                    case SASICG_SIMULATION_ROLE_BROKER_E:
                        if(distributedSides[ii].type == SERVER_SIDE_E)
                        {
                            strcpy(connectionPtr->socketsArray[kk].ipAddressStr,loopbackIpStr);/* loopback IP address*/
                            connectionPtr->socketsArray[kk].protocolPort = tcpPortsArray[kk][jj];
                        }
                        else
                        {
                            strcpy(connectionPtr->socketsArray[kk].ipAddressStr,ipAddressStr_2);
                            connectionPtr->socketsArray[kk].protocolPort = tcpPorts_2[jj];
                        }
                        break;
                    default:
                        return;
                }

                if(connectionPtr->socketServerSide == GT_TRUE)
                {
                    strcpy(connectionPtr->serverSocket.ipAddressStr,connectionPtr->socketsArray[kk].ipAddressStr);
                    connectionPtr->serverSocket.protocolPort = connectionPtr->socketsArray[kk].protocolPort;
                }

                connectionInit(connectionPtr);
            }/* end of kk */
        } /* end of ii */
    }/* end of jj */

    return;
}

/*******************************************************************************
*   simDistributedSystemReset
*
* DESCRIPTION:
*       kill/reset the simulation .
*
* INPUTS:
*       None
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void distributedSystemReset
(
    IN GT_U32   reset
)
{
    resetSystem = reset;

    /* notify that we come from external call and NOT from a message */
    externalCall = 1;

    if(reset)
    {
        /* call to do proper 'reset' action */
        SHOSTG_reset(0);
        /* NOTE : this function will eventually call : simDistributedExit() */
    }
    else
    {
        /* call to do proper 'exit' action */
        SHOSTC_exit(0);
        /* NOTE : this function will eventually call : simDistributedExit() */
    }
}

/*******************************************************************************
*   simDistributedSystemReset
*
* DESCRIPTION:
*       Reset this station and other distribution
*
*       NOTE : only in the next direction app->broker-->bus-->asic(s).
*
* INPUTS:
*       None
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS simDistributedSystemReset
(
    void
)
{
    distributedSystemReset(1);

    return GT_OK;
}

/*******************************************************************************
*   simDistributedSystemExit
*
* DESCRIPTION:
*       Exit (Kill) the simulation distribution functionality.
*
* INPUTS:
*       None
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS simDistributedSystemExit
(
    void
)
{
    distributedSystemReset(0);

    return GT_OK;
}

/*******************************************************************************
*   simDistributedExit
*
* DESCRIPTION:
*       Exit (Kill) the simulation distribution functionality.
*
* INPUTS:
*       None
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void simDistributedExit
(
    void
)
{
    CONNECTION_INFO_STC *connectionPtr;
    GT_U32  ii,jj,kk;
    GT_U32   maxClients,currIndex;

    if((!_dev) && resetSystem && externalCall)
    {
        /* trigger 'RESET' to all other parts of the system */
        simDistributedRemoteResetSend(FIRST_SOCKET);

        /* allow the message to be sent to the next part(s) */
        SIM_OS_MAC(simOsSleep)(RESET_WAIT_SLEEP_TIME_CNS);
    }


    if(resetSystem)
    {
        /* close the serial channel before the new process start */
        SIM_OS_MAC(osSerialDestroySerial)();
    }


    for(jj = 0 ;jj < CONNECTION_TYPE_LAST_E; jj++)
    {
        if(connectionsArray[jj] == NULL)
        {
            /* exit is called before the init was started / fully done */
            continue;
        }

        if(jj == CONNECTION_TYPE_NIC_RX_FRAME_E)
        {
            /* not tested at this stage yet ... */
            continue;
        }

        currIndex = 0;
        for(ii = 0 ; ii < numOfSides ; ii++)
        {
            for(kk = 0 ; kk < distributedSides[ii].numConnections ; kk++ , currIndex++)
            {
                connectionPtr = &connectionsArray[jj][currIndex];
                if(connectionPtr->valid != GT_TRUE)
                {
                    continue;
                }

                connectionPtr->valid = GT_FALSE;

                if(connectionPtr->socketServerSide == GT_TRUE && connectionPtr->serverSocket.sockId)
                {
#ifndef LINUX_SIM /* in UNIX-like systems no need to call shutdown because close() inside osSocketTcpDestroy enough to close the session
                                       * and no_linger not needed because before call close() we define for UNIX-Like to linger for 5 seconds before real close */
                    SIM_OS_MAC(osSocketSetSocketNoLinger)(connectionPtr->serverSocket.sockId);
                    SIM_OS_MAC(osSocketShutDown)(connectionPtr->serverSocket.sockId,
                                            SOCKET_SHUTDOWN_CLOSE_ALL);
#endif /*LINUX_SIM*/
                    SIM_OS_MAC(osSocketTcpDestroy)(connectionPtr->serverSocket.sockId);
                    connectionPtr->serverSocket.sockId = 0;
                }

                maxClients = connectionPtr->numSockets;

                for(ii = 0 ; ii < maxClients ; ii++)
                {
                    if(connectionPtr->socketsArray[ii].sockId)
                    {
#ifndef LINUX_SIM /* in UNIX-like systems no need to call shutdown because close() inside osSocketTcpDestroy enough to close the session
                                       * and no_linger not needed because before call close() we define for UNIX-Like to linger for 5 seconds before real close */
                        SIM_OS_MAC(osSocketSetSocketNoLinger)(connectionPtr->socketsArray[ii].sockId);
                        SIM_OS_MAC(osSocketShutDown)(connectionPtr->socketsArray[ii].sockId,
                                                SOCKET_SHUTDOWN_CLOSE_ALL);
#endif /*LINUX_SIM*/
                        SIM_OS_MAC(osSocketTcpDestroy)(connectionPtr->socketsArray[ii].sockId);
                        connectionPtr->socketsArray[ii].sockId = 0;
                    }
                }

#if 0
/* if I will free the dynamic memory , the task that may use it will cause
    exception , that will break the clean shutdown */
                if(connectionPtr->readBufPtr)
                {
                    free(connectionPtr->readBufPtr);
                    connectionPtr->readBufPtr = NULL;
                }

                if(connectionPtr->replyBufPtr)
                {
                    free(connectionPtr->replyBufPtr);
                    connectionPtr->replyBufPtr = NULL;
                }
#endif /*0*/
            }
        }

#if 0
/* if I will free the dynamic memory , the task that may use it will cause
    exception , that will break the clean shutdown */
        if(connectionsArray[jj])
        {
            free(connectionsArray[jj]);
            connectionsArray[jj] = NULL;
        }
#endif /*0*/
    }
}

/*******************************************************************************
*  simDistributedMessageSend
*
* DESCRIPTION:
*       function send message to connection.
*
* INPUTS:
*       clientIndex - (relevant to server only) client index in the array of
*                   connectionPtr->socketsArray[]
*       connectionPtr - pointer to the connection info.
*       messageHeaderPtr - pointer to the parsed header info
*       dataAfterHeaderPtr - pointer to the data after the header
*       lastDataIsByteStream - the last data is byte stream and not words
*       useDirectSocketId - the clientIndex is actually the client index
*                       (used when reply to request ,and on server side only
*                        that has many clients)
*       isBlocking - is the message is blocking , meaning we wait for connection
*                   if GT_FALSE we not send message if other side not ready
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void simDistributedMessageSend
(
    IN GT_U32                   clientIndex,
    IN CONNECTION_INFO_STC     *connectionPtr,
    IN MSG_HDR                  *messageHeaderPtr,
    IN GT_U32                   numberOfDataAfterTheHeader,
    IN GT_U32                   dataAfterHeaderLen[],
    IN GT_U32                  *dataAfterHeaderPtr[],
    IN GT_BOOL                  lastDataIsByteStream,
    IN GT_BOOL                  useDirectSocketId,
    IN GT_BOOL                  isBlocking
)
{
    GT_U32  ii,jj;
    GT_U32  buffer[(MAX_BUFFER_SIZE_SINGLE_MESSAGE_CNS/4)];
    GT_U32  *bufferPtr = buffer;
    GT_U32  *u32buffPtr;
    GT_U32  *sourcePtr;
    GT_U32  numLoops = numberOfDataAfterTheHeader;
    GT_SOCKET_FD currSocketId;
    GT_U32  actualSent;
/*    GT_U32  disableInterruptValue=0;*/
    GT_U32  disabledInterrupt = 0;

    if(lastDataIsByteStream == GT_TRUE)
    {
        numLoops--;
    }

    if(TOTAL_LEN_BYTES(messageHeaderPtr->msgLen) > sizeof(buffer))
    {
        /* we need to use dynamic allocated memory */
        bufferPtr = malloc(messageHeaderPtr->msgLen + sizeof(*messageHeaderPtr));
        if(bufferPtr == NULL)
        {
            skernelFatalError(" simDistributedMessageSend : malloc failed on size[%ld]",
                            TOTAL_LEN_BYTES(messageHeaderPtr->msgLen));
        }
    }

    /* pointer to the buffer to send */
    u32buffPtr = bufferPtr;

    /* start copy the header */
    sourcePtr = (GT_U32*)(void*)messageHeaderPtr;

    for(ii = 0 ;
        ii < (sizeof(MSG_HDR)/4);/* number of words in header */
        ii++, u32buffPtr++ , sourcePtr++)
    {
        /* copy data from header to the buffer */
         *u32buffPtr =  SIM_HTONL(sourcePtr);
    }

    /* start copy the data after the header */
    for(jj = 0 ; jj < numLoops ; jj++)
    {
        /* pointer to data to copy */
        sourcePtr = dataAfterHeaderPtr[jj];

        for(ii = 0 ;
            ii < dataAfterHeaderLen[jj] ;/* number of words in data */
            ii++, u32buffPtr++ , sourcePtr++)
        {
            /* copy data to the buffer */
             *u32buffPtr =  SIM_HTONL(sourcePtr);
        }
    }

    if(lastDataIsByteStream == GT_TRUE)
    {
        /* copy byte stream --> no HTON needed */
        GT_U8   *byteSourcePtr = (GT_U8*)dataAfterHeaderPtr[jj];
        GT_U8   *byteBuffPtr = (GT_U8*)u32buffPtr;

        /* this time the size is in bytes !!!! */
        memcpy(byteBuffPtr,byteSourcePtr,dataAfterHeaderLen[jj]);
    }

    if(useDirectSocketId)
    {
        /* no conversion needed , and no need to wait , since we reply to
           request */
        currSocketId = clientIndex;
    }
    else
    {
        if(clientIndex == NIC_CLIENT)
        {
            while(nicInfo.nicInitialized == GT_FALSE)
            {
                if(isBlocking == GT_FALSE)
                {
                    if(bufferPtr != buffer)
                    {
                        /* there was dynamic malloc --> free the memory */
                        free(bufferPtr);
                    }
                    return;
                }
                /* wait for client to establish connection */
                SIM_OS_MAC(simOsSleep)(WAIT_SLEEP_TIME_CNS);
            }

            /* convert to the real client index */
            convertDeviceIdToClientIndex(MSG_TYPE_NIC_RX_FRAME,/*tempo value -- not supported yet*/
                                        nicInfo.nicDevNum,&clientIndex);
        }

        while(connectionPtr->socketsArray[clientIndex].ready == 0)
        {
            if(isBlocking == GT_FALSE)
            {
                if(bufferPtr != buffer)
                {
                    /* there was dynamic malloc --> free the memory */
                    free(bufferPtr);
                }
                return;
            }
            /* wait for client to establish connection */
            SIM_OS_MAC(simOsSleep)(WAIT_SLEEP_TIME_CNS);
        }

        currSocketId = connectionPtr->socketsArray[clientIndex].sockId;
    }

    LOG_SOCKET_ID_SEND((GT_U32)currSocketId);

    if(_app)
    {
        disabledInterrupt = 1;
        /* lock the interrupts , so the interrupt will not "suspend our task" ,
           while osSocketSend may take semaphore for locking internal DB
           --> will cause dead lock */
/*        disableInterruptValue = SHOSTG_interrupt_disable();*/
        SCIB_SEM_TAKE;
    }

    /* write to the socket */
    actualSent = SIM_OS_MAC(osSocketSend)(currSocketId,(void*)bufferPtr,TOTAL_LEN_BYTES(messageHeaderPtr->msgLen));

    if(disabledInterrupt)
    {
        /* enable the interrupts */
/*        SHOSTG_interrupt_enable(disableInterruptValue);*/
        SCIB_SEM_SIGNAL;
    }

    if(actualSent != TOTAL_LEN_BYTES(messageHeaderPtr->msgLen))
    {
        skernelFatalError(" simDistributedMessageSend : osSocketSend send only [%ld],instead of[%ld]\n",
            actualSent,TOTAL_LEN_BYTES(messageHeaderPtr->msgLen));
    }

    if(bufferPtr != buffer)
    {
        /* there was dynamic malloc --> free the memory */
        free(bufferPtr);
    }

    return;
}

/*******************************************************************************
*  simDistributedInterruptSetExt
*
* DESCRIPTION:
*      extended Set interrupt line for a device function. --> direction is cpu from asic
*      function should be called only on the asic side
* INPUTS:
*       deviceId    - the device id that set the interrupt
*       connectionPtr - connection pointer to use
* OUTPUTS:
*       none
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void simDistributedInterruptSetExt
(
    IN  GT_U32        deviceId,
    IN  CONNECTION_INFO_STC *connectionPtr
)
{
    MSG_HDR  *messageHeaderPtr;/* header of current treated message */
    INTERRUPT_SET_MSG   message;
    GT_U32  dataLen = LEN_NO_HEADER_BYTES(INTERRUPT_SET_MSG);
    GT_U32  *dataPtr = FIRST_FIELD_AFTER_HEADER_PTR(&message);
    GT_U32  clientId;
    DISTRIBUTED_MESSAGE_TYPE_ENT msgType = DISTRIBUTED_MESSAGE_TYPE_INTERRUPT_SET_E;

    message.deviceId = (VARIABLE_TYPE)deviceId;
    /****************************/
    /* build the message header */
    /****************************/
    messageHeaderPtr = &message.hdr;
    messageHeaderPtr->type = (VARIABLE_TYPE)msgType;
    messageHeaderPtr->msgLen = (VARIABLE_TYPE)dataLen;/* in bytes */

    convertDeviceIdToClientIndex(msgType,deviceId,&clientId);

    /**************************************/
    /* send the message to the other side */
    /**************************************/
    simDistributedMessageSend(clientId,connectionPtr,messageHeaderPtr,1,&dataLen, (GT_U32**)(&dataPtr),
        GT_FALSE,GT_FALSE,GT_TRUE);

    LOG_INFO_REMOTE_INTERRUPT_SET(deviceId);
}

/*******************************************************************************
*  simDistributedInterruptSet
*
* DESCRIPTION:
*      Set interrupt line for a device function. --> direction is cpu from asic
*      function should be called only on the asic side
* INPUTS:
*       deviceId    - the device id that set the interrupt
*
* OUTPUTS:
*       none
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void simDistributedInterruptSet
(
    IN  GT_U32        deviceId
)
{
    CONNECTION_INFO_STC *connectionPtr;
    DISTRIBUTED_MESSAGE_TYPE_ENT msgType = DISTRIBUTED_MESSAGE_TYPE_INTERRUPT_SET_E;

    connectionPtr = convertMessageTypeToConnectionIndex(msgType);

    simDistributedInterruptSetExt(deviceId,connectionPtr);
}


/*******************************************************************************
*  simDistributedRegisterRead
*
* DESCRIPTION:
*      Read register function. --> direction is cpu to asic
*      function should be called only on the cpu side
* INPUTS:
*       accessType  - Define access operation Read or Write.
*       deviceId    - device id.
*       address     - Address for ASIC memory.
*       memSize     - Size of ASIC memory to read or write.
*
* OUTPUTS:
*       memPtr     - pointer to application memory in which
*                     ASIC memory will be copied.
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void simDistributedRegisterRead
(
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 deviceId,
    IN GT_U32 address,
    IN GT_U32 memSize,
    OUT GT_U32 * memPtr
)
{
    DISTRIBUTED_MESSAGE_SYNC_INFO_STC *messageSyncPtr =
            &distributedMessageSync[PROTECT_REGISTER_READ_INDEX_E];
    CONNECTION_INFO_STC *connectionPtr;
    MSG_HDR  *messageHeaderPtr;/* header of current treated message */
    GT_U32  clientId;
    GT_U32  read=1;
    READ_MSG   message;
    GT_U32  dataLenWords = LEN_NO_HEADER_WORDS(READ_MSG);
    GT_U32  *dataPtr = FIRST_FIELD_AFTER_HEADER_PTR(&message);
/*    GT_U32  disableInterruptValue=0;*/
    DISTRIBUTED_MESSAGE_TYPE_ENT msgType = DISTRIBUTED_MESSAGE_TYPE_REGISTER_READ_E;

    connectionPtr = convertMessageTypeToConnectionIndex(msgType);

    /* lock the interrupts , so the interrupt will not "suspend our task" ,
       while we lock the "read" and the interrupt context may need to also do
       "read register" --> will cause dead lock */
/*    disableInterruptValue = SHOSTG_interrupt_disable();*/

    /* lock the waiting operations */
    SIM_OS_MAC(simOsMutexLock)(connectionPtr->getReplyProtect);

    /****************************/
    /* build the message header */
    /****************************/
    messageHeaderPtr = &message.hdr;
    messageHeaderPtr->type = (VARIABLE_TYPE)msgType;
    messageHeaderPtr->msgLen = (VARIABLE_TYPE)(dataLenWords*4);/* in bytes */

    /****************************/
    /* build the message info   */
    /****************************/
    message.deviceId = (VARIABLE_TYPE)deviceId;
    message.accessType = (VARIABLE_TYPE)(((accessType == SCIB_MEMORY_READ_E) ||
                                   (accessType == SCIB_MEMORY_WRITE_E)) ?
                                        REGISTER_MEMORY_ACCESS :
                                        PCI_REGISTERS_ACCESS);
    message.address = address;
    message.readLen = (VARIABLE_TYPE)memSize;

    /* put the pointer where to get the data */
    /* must be done before calling simDistributedMessageSend(...) */
    messageSyncPtr->dataPtr = memPtr;
    /* we read registers (words) */
    messageSyncPtr->dataIsWords = 1;

    convertDeviceIdToClientIndex(msgType,deviceId,&clientId);

    /**************************************/
    /* send the message to the other side */
    /**************************************/
    simDistributedMessageSend(clientId,connectionPtr,messageHeaderPtr,1,&dataLenWords, (GT_U32**)(&dataPtr),
        GT_FALSE,GT_FALSE,GT_TRUE);

    LOG_INFO_REGISTER_SENT(read,message.accessType,deviceId,address,memSize);

    /**************************/
    /* now wait for the reply */
    /**************************/
    SIM_OS_MAC(simOsSemWait)(messageSyncPtr->semSync,SIM_OS_WAIT_FOREVER);

    LOG_INFO_READ_DATA_REPLY(memSize,memPtr);

    /* UnLock the waiting operations */
    SIM_OS_MAC(simOsMutexUnlock)(connectionPtr->getReplyProtect);

    /* enable the interrupts */
/*    SHOSTG_interrupt_enable(disableInterruptValue);*/

}

/*******************************************************************************
*  simDistributedRegisterWrite
*
* DESCRIPTION:
*      write register function. --> direction is cpu to asic
*      function should be called only on the cpu side
* INPUTS:
*       accessType  - Define access operation Read or Write.
*       deviceId    - device id.
*       address     - Address for ASIC memory.
*       memSize     - Size of ASIC memory to read or write.
*       memPtr      - For Write this pointer to application memory,which
*                     will be copied to the ASIC memory .
*
* OUTPUTS:
*       none
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void simDistributedRegisterWrite
(
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 deviceId,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_U32 * memPtr
)
{
    CONNECTION_INFO_STC *connectionPtr;
    MSG_HDR  *messageHeaderPtr;/* header of current treated message */
    GT_U32  dataLen[2];
    GT_U32  *dataPtr[2];
    GT_U32  clientId;
    GT_U32  read=0;
    WRITE_MSG  message;
    GT_U32  dataLenWords = LEN_NO_HEADER_WORDS(WRITE_MSG) - PLACE_HOLDER_FIELD_WORDS_SIZE;/* remove the place holder word */
    DISTRIBUTED_MESSAGE_TYPE_ENT msgType = DISTRIBUTED_MESSAGE_TYPE_REGISTER_WRITE_E;

    connectionPtr = convertMessageTypeToConnectionIndex(msgType);

    dataLen[0] = dataLenWords;/*in words*/
    dataLen[1] = memSize;

    dataPtr[0] = FIRST_FIELD_AFTER_HEADER_PTR(&message);
    dataPtr[1] = memPtr;

    /****************************/
    /* build the message header */
    /****************************/
    messageHeaderPtr = &message.hdr;
    messageHeaderPtr->type = (VARIABLE_TYPE)msgType;
    messageHeaderPtr->msgLen = (VARIABLE_TYPE)((dataLen[0] + dataLen[1]) * 4) ;/* the number of bytes to write */

    /****************************/
    /* build the message info   */
    /****************************/
    message.deviceId = (VARIABLE_TYPE)deviceId;
    message.accessType = (VARIABLE_TYPE)(((accessType == SCIB_MEMORY_READ_E) ||
                                   (accessType == SCIB_MEMORY_WRITE_E)) ?
                                        REGISTER_MEMORY_ACCESS :
                                        PCI_REGISTERS_ACCESS);
    message.address = address;
    message.writeLen = (VARIABLE_TYPE)memSize;

    convertDeviceIdToClientIndex(msgType,deviceId,&clientId);

    /**************************************/
    /* send the message to the other side */
    /**************************************/
    simDistributedMessageSend(clientId,connectionPtr,messageHeaderPtr,2,dataLen, dataPtr,
        GT_FALSE,GT_FALSE,GT_TRUE);

    LOG_INFO_REGISTER_SENT(read,message.accessType,deviceId,address,memSize);
    LOG_INFO_WRITE_DATA_SENT(memSize,memPtr);
}


/*******************************************************************************
*  simDistributedDmaRead
*
* DESCRIPTION:
*      Read HOST CPU DMA memory function. --> direction is asic from cpu
*      function should be called only on the asic side
* INPUTS:
*       deviceId    - device id. (of the device in the simulation)
*       address     - Address in HOST DMA memory.
*       memSize     - Size of DMA memory to read .
*       dataIsWords - the data to read is words or bytes
*                     1 - words --> swap network order to cpu order
*                     0 - bytes --> NO swap network order to cpu order
* OUTPUTS:
*       memPtr     - pointer to application memory in which
*                     ASIC memory will be copied.
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void simDistributedDmaRead
(
    IN GT_U32 deviceId,
    IN GT_U32 address,
    IN GT_U32 memSize,
    OUT GT_U32 * memPtr,
    IN GT_U32  dataIsWords
)
{
    DISTRIBUTED_MESSAGE_SYNC_INFO_STC *messageSyncPtr =
            &distributedMessageSync[PROTECT_DMA_READ_INDEX_E];
    CONNECTION_INFO_STC *connectionPtr;
    MSG_HDR  *messageHeaderPtr;/* header of current treated message */
    DMA_READ_MSG   message;
    GT_U32  dataLenWords = LEN_NO_HEADER_WORDS(DMA_READ_MSG);
    GT_U32  *dataPtr = FIRST_FIELD_AFTER_HEADER_PTR(&message);
    GT_U32  clientId;
    DISTRIBUTED_MESSAGE_TYPE_ENT msgType = DISTRIBUTED_MESSAGE_TYPE_DMA_READ_E;

    connectionPtr = convertMessageTypeToConnectionIndex(msgType);

    /* lock the waiting operations */
    SIM_OS_MAC(simOsMutexLock)(connectionPtr->getReplyProtect);

    /****************************/
    /* build the message header */
    /****************************/
    messageHeaderPtr = &message.hdr;
    messageHeaderPtr->type = (VARIABLE_TYPE)msgType;
    messageHeaderPtr->msgLen = (VARIABLE_TYPE)(dataLenWords*4);/* in bytes */

    /****************************/
    /* build the message info   */
    /****************************/
    message.address = address;
    message.readLen = memSize;

    /* put the pointer where to get the data */
    /* must be done before calling simDistributedMessageSend(...) */
    messageSyncPtr->dataPtr = memPtr;
    /* we read DMA , the info inside is words */
    messageSyncPtr->dataIsWords = dataIsWords ? 1 : 0;

    convertDeviceIdToClientIndex(msgType,deviceId,&clientId);
    /**************************************/
    /* send the message to the other side */
    /**************************************/
    simDistributedMessageSend(clientId,connectionPtr,messageHeaderPtr,1,&dataLenWords, (GT_U32**)(&dataPtr),
        GT_FALSE,GT_FALSE,GT_TRUE);

    LOG_INFO_DMA_SENT(DMA_READ,address,memSize);

    /**************************/
    /* now wait for the reply */
    /**************************/
    SIM_OS_MAC(simOsSemWait)(messageSyncPtr->semSync,SIM_OS_WAIT_FOREVER);

    LOG_INFO_READ_DATA_REPLY(memSize,memPtr);

    /* UnLock the waiting operations */
    SIM_OS_MAC(simOsMutexUnlock)(connectionPtr->getReplyProtect);

}

/*******************************************************************************
*  simDistributedDmaWrite
*
* DESCRIPTION:
*      write HOST CPU DMA memory function. --> direction is asic to cpu
*      function should be called only on the asic side
* INPUTS:
*       deviceId    - device id. (of the device in the simulation)
*       address     - Address in HOST DMA memory.
*       memSize     - Size of ASIC memory to read or write.
*       memPtr      - For Write this pointer to application memory,which
*                     will be copied to the ASIC memory .
*       dataIsWords - the data to read is words or bytes
*                     1 - words --> swap network order to cpu order
*                     0 - bytes --> NO swap network order to cpu order
*
* OUTPUTS:
*       none
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void simDistributedDmaWrite
(
    IN GT_U32 deviceId,
    IN GT_U32 address,
    IN GT_U32 memSize,
    INOUT GT_U32 * memPtr,
    IN GT_U32  dataIsWords
)
{
    CONNECTION_INFO_STC *connectionPtr;
    MSG_HDR  *messageHeaderPtr;/* header of current treated message */
    GT_U32  dataLen[2];
    GT_U32  *dataPtr[2];
    DMA_WRITE_MSG   message;
    GT_U32  dataLenWords = LEN_NO_HEADER_WORDS(DMA_WRITE_MSG) - PLACE_HOLDER_FIELD_WORDS_SIZE;/* remove the place holder word */
    GT_U32  clientId;
    DISTRIBUTED_MESSAGE_TYPE_ENT msgType = DISTRIBUTED_MESSAGE_TYPE_DMA_WRITE_E;

    connectionPtr = convertMessageTypeToConnectionIndex(msgType);

    dataLen[0] = dataLenWords;
    dataLen[1] = dataIsWords ? memSize : (memSize*4);

    dataPtr[0] = FIRST_FIELD_AFTER_HEADER_PTR(&message);
    dataPtr[1] = memPtr;

    /****************************/
    /* build the message header */
    /****************************/
    messageHeaderPtr = &message.hdr;
    messageHeaderPtr->type = (VARIABLE_TYPE)msgType;
    messageHeaderPtr->msgLen = (VARIABLE_TYPE)((dataLen[0] + dataLen[1]) * 4);


    /****************************/
    /* build the message info   */
    /****************************/
    message.address = address;
    message.writeLen = memSize;

    convertDeviceIdToClientIndex(msgType,deviceId,&clientId);

    /**************************************/
    /* send the message to the other side */
    /**************************************/
    simDistributedMessageSend(clientId,connectionPtr,messageHeaderPtr,2,dataLen, dataPtr,
        dataIsWords ? GT_FALSE : GT_TRUE , GT_FALSE,GT_TRUE);

    LOG_INFO_DMA_SENT(DMA_WRITE,address,memSize);
    LOG_INFO_WRITE_DATA_SENT(memSize,memPtr);
}


/*******************************************************************************
* nicFrameSendToOtherSide
*
* DESCRIPTION:
*       This function transmits an Ethernet packet , to the other side so the
*       other side can transmit to/from CPU (Nic).
*
* INPUTS:
*       type - message type : nic rx/tx frame
*       frameLength  - length of frame.
*       framePtr  - pointer the frame (array of bytes).
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
static void nicFrameSendToOtherSide
(
    IN DISTRIBUTED_MESSAGE_TYPE_ENT type,
    IN GT_U32       frameLength,
    IN GT_U8        *framePtr
)
{
    skernelFatalError(" nicFrameSendToOtherSide : not implemented \n");
}


/*******************************************************************************
* simDistributedNicTxFrame
*
* DESCRIPTION:
*       This function transmits an Ethernet packet from the NIC of CPU.
*       function should be called only on the cpu side
*
* INPUTS:
*       frameLength  - length of frame.
*       framePtr  - pointer the frame (array of bytes).
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
void simDistributedNicTxFrame
(
    IN GT_U32       frameLength,
    IN GT_U8        *framePtr
)
{
    nicFrameSendToOtherSide(DISTRIBUTED_MESSAGE_TYPE_NIC_TX_FRAME_E,frameLength,framePtr);

    return;
}

/*******************************************************************************
* simDistributedNicRxFrame
*
* DESCRIPTION:
*       This function transmit an Ethernet packet to the NIC of CPU.
*       function should be called only on the asic side
*
* INPUTS:
*       frameLength  - length of frame.
*       framePtr  - pointer the frame (array of bytes).
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
void simDistributedNicRxFrame
(
    IN GT_U32       frameLength,
    IN GT_U8        *framePtr
)
{
    nicFrameSendToOtherSide(DISTRIBUTED_MESSAGE_TYPE_NIC_RX_FRAME_E,frameLength,framePtr);

    return;
}

/*******************************************************************************
* clientConnect
*
* DESCRIPTION:
*       connect the client to it's server
*
* INPUTS:
*       connectionPtr - pointer to connection info
*       clientIndex   - index of the client
* OUTPUTS:
*       none
*
* RETURNS:
*       none
*
* COMMENTS:
*       called only on the client
*
*******************************************************************************/
static void clientConnect(
    IN CONNECTION_INFO_STC *connectionPtr,
    IN GT_U32   clientIndex
)
{
    GT_U32  waitForConnect = 0;/* did we wait for connection with the other side */
    GT_SOCKET_FD *socketIdPtr;

    socketIdPtr = &connectionPtr->socketsArray[clientIndex].sockId;

    /* create sockets for those clients */
    socketInit(connectionPtr,clientIndex,GT_FALSE);

    printf(" client initialized socket : \n");
    printConnectionSockets(connectionPtr);

    /* NOTE :  connectionPtr->socketId is used as the socketId of the first
               client */
    while (GT_OK != SIM_OS_MAC(osSocketConnect) (*socketIdPtr,
                        connectionPtr->socketsArray[clientIndex].sockAddr,
                        connectionPtr->socketsArray[clientIndex].sockAddrLen))
    {
        /* it seems that under some OS , we need to re-start configuration ,
          before trying to 'connect' again */
        socketInit(connectionPtr,clientIndex,GT_TRUE);

        if(waitForConnect == 0)
        {
            waitForConnect = 1;
            printf("clientConnect: %s wait for server to accept clientIndex[%d]\n",
                iniFileTcpPortsStr[connectionPtr->connectionType],clientIndex);
        }
        else
        {
            printf(".");
        }
        /* wait for 'Server' to 'Get up' */
        SIM_OS_MAC(simOsSleep)(WAIT_SLEEP_TIME_CNS);
    }

    printf("\n");
    SIM_OS_MAC(simOsSocketSetSocketNoDelay)(*socketIdPtr);

    /* only here we can state that the client is ready ... (the dispatcher task
        and others waits for this)*/
    connectionPtr->socketsArray[clientIndex].ready = 1;

    LOG_INFO_CONNECTION_INIT(connectionPtr->connectionType,*socketIdPtr);
}

/*******************************************************************************
*  simDistributedRemoteInitExt
*
* DESCRIPTION:
*      do remote Asic init function. --> direction is asic to application
*      function should be called only on the Asic side
* INPUTS:
*       numOfDevices - number of device to initialize
*       devicesArray  - devices array , and info.
*       connectionPtr - pointer to the connection
* OUTPUTS:
*       none
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void simDistributedRemoteInitExt
(
    IN GT_U32   numOfDevices,
    IN SIM_DISTRIBUTED_INIT_DEVICE_STC *devicesArray,
    IN CONNECTION_INFO_STC *connectionPtr
)
{
    GT_U32  ii;
    MSG_HDR  *messageHeaderPtr;/* header of current treated message */
    GT_U32  dataLen[1];
    GT_U32  *dataPtr[1];
    INIT_PARAM_MSG   message;
    GT_U32  dataLenWords = LEN_NO_HEADER_WORDS(INIT_PARAM_MSG);
    GT_U32  clientId;
    DISTRIBUTED_MESSAGE_TYPE_ENT msgType = DISTRIBUTED_MESSAGE_TYPE_DEVICE_TO_APPLICATION_INIT_PARAM_E;

    if(numOfDevices > MAX_DEVICES_ON_BOARD_CNS)
    {
        skernelFatalError(" simDistributedRemoteInit : array too small , need [%d]\n",numOfDevices);
    }

    dataLen[0] = dataLenWords;
    dataPtr[0] = FIRST_FIELD_AFTER_HEADER_PTR(&message);

    /****************************/
    /* build the message header */
    /****************************/
    messageHeaderPtr = &message.hdr;
    messageHeaderPtr->type = (VARIABLE_TYPE)msgType;
    messageHeaderPtr->msgLen = (VARIABLE_TYPE)(dataLen[0] * 4);

    LOG_INFO_REMOTE_INIT_NUM_DEVICES(numOfDevices);

    for(ii = 0; ii < numOfDevices ; ii++)
    {

        /* build the message info */
        message.deviceInfo.deviceId      = devicesArray[ii].deviceId;
        message.deviceInfo.deviceHwId    = devicesArray[ii].deviceHwId;
        message.deviceInfo.interruptLine = devicesArray[ii].interruptLine;
        message.deviceInfo.isPp          = devicesArray[ii].isPp;
        message.deviceInfo.addressCompletionStatus  = devicesArray[ii].addressCompletionStatus;
        message.deviceInfo.nicDevice     = devicesArray[ii].nicDevice;

        convertDeviceIdToClientIndex(msgType,devicesArray[ii].deviceId,&clientId);

        /**************************************/
        /* send the message to the other side */
        /**************************************/
        simDistributedMessageSend(clientId,connectionPtr,messageHeaderPtr,1,dataLen, dataPtr,
            GT_FALSE,GT_FALSE,GT_TRUE);


        LOG_INFO_REMOTE_INIT_DEVICE_INFO(devicesArray[ii].deviceId,
                       devicesArray[ii].deviceHwId,
                       devicesArray[ii].interruptLine,
                       devicesArray[ii].isPp,
                       devicesArray[ii].addressCompletionStatus,
                       devicesArray[ii].nicDevice);
    }
}

/*******************************************************************************
*  simDistributedRemoteInit
*
* DESCRIPTION:
*      do remote Asic init function. --> direction is asic to application
*      function should be called only on the Asic side
* INPUTS:
*       numOfDevices - number of device to initialize
*       devicesArray  - devices array , and info.
* OUTPUTS:
*       none
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void simDistributedRemoteInit
(
    IN GT_U32   numOfDevices,
    IN SIM_DISTRIBUTED_INIT_DEVICE_STC *devicesArray
)
{
    CONNECTION_INFO_STC *connectionPtr;
    DISTRIBUTED_MESSAGE_TYPE_ENT msgType = DISTRIBUTED_MESSAGE_TYPE_DEVICE_TO_APPLICATION_INIT_PARAM_E;

    connectionPtr = convertMessageTypeToConnectionIndex(msgType);

    simDistributedRemoteInitExt(numOfDevices,devicesArray,connectionPtr);
}

/*******************************************************************************
*  simDistributedRemoteResetSend
*
* DESCRIPTION:
*      application send to all the distributed parts on the system
*      the 'Reset' message*
* INPUTS:
*       clientIndex - the client index to get the message
*                     (use FIRST_SOCKET if has single client)
* OUTPUTS:
*       None
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void simDistributedRemoteResetSend
(
    GT_U32  clientIndex
)
{
    CONNECTION_INFO_STC *connectionPtr;
    MSG_HDR  *messageHeaderPtr;/* header of current treated message */
    RESET_PARAM_MSG   message;
    GT_U32  dataLenWords = LEN_NO_HEADER_WORDS(RESET_PARAM_MSG);
    GT_U32  *dataPtr = FIRST_FIELD_AFTER_HEADER_PTR(&message);
    DISTRIBUTED_MESSAGE_TYPE_ENT msgType = DISTRIBUTED_MESSAGE_TYPE_RESET_E;

    connectionPtr = convertMessageTypeToConnectionIndex(msgType);

    /****************************/
    /* build the message header */
    /****************************/
    messageHeaderPtr = &message.hdr;
    messageHeaderPtr->type = (VARIABLE_TYPE)msgType;
    messageHeaderPtr->msgLen = (VARIABLE_TYPE)(dataLenWords*4);/* in bytes */

    /****************************/
    /* build the message info   */
    /****************************/

    /* no extra info */

    /**************************************/
    /* send the message to the other side */
    /**************************************/
    simDistributedMessageSend(clientIndex,connectionPtr,messageHeaderPtr,1,&dataLenWords, (GT_U32**)(&dataPtr),
        GT_FALSE,GT_FALSE,
        GT_FALSE);/* non blocking message */
}

/*******************************************************************************
*  simDistributedRemoteDebugLevelSend
*
* DESCRIPTION:
*      application send to single/all the distributed part(s) on the system
*      the 'Debug level set' message
* INPUTS:
*      mainTarget    - the target of the message (broker/bus/device(s)/all)
*                      on of TARGET_OF_MESSAGE_ENT
*      secondaryTarget - the more specific target with in the main target (mainTarget)
*                      may be 0..0xfffffffe for specific ID of the secondary target
*                      may be ALL_SECONDARY_TARGETS means 'all' secondary targets
*                      for example when there are several devices processes,
*                      we can distinguish between devices by the board part
*                      (secondaryTarget) or set secondaryTarget to
*                      ALL_SECONDARY_TARGETS to apply to all board parts
*      debugLevel   -  the debug level bmp
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK - on success
*       GT_BAD_PARAM - when parameter not valid to this part of simulation
*
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS simDistributedRemoteDebugLevelSend
(
    IN TARGET_OF_MESSAGE_ENT  mainTarget,
    IN GT_U32                 secondaryTarget,
    IN GT_U32                 debugLevel
)
{
    CONNECTION_INFO_STC *connectionPtr;
    MSG_HDR  *messageHeaderPtr;/* header of current treated message */
    DEBUG_LEVEL_MSG   message;
    GT_U32  dataLenWords = LEN_NO_HEADER_WORDS(DEBUG_LEVEL_MSG);
    GT_U32  *dataPtr = FIRST_FIELD_AFTER_HEADER_PTR(&message);
    DISTRIBUTED_MESSAGE_TYPE_ENT msgType = DISTRIBUTED_MESSAGE_TYPE_DEBUG_LEVEL_SET;
    GT_U32  clientIndex;
    GT_U32  ii,iiMax;

    /* check input parameter */
    switch(mainTarget)
    {
        case TARGET_OF_MESSAGE_APPLICATION_E:
            /* no sending to the application */
            return GT_BAD_PARAM;

        case TARGET_OF_MESSAGE_BROKER_E:
            if(!_app)
            {
                /* only application can send to broker */
                return GT_BAD_PARAM;
            }

            break;
        case TARGET_OF_MESSAGE_BUS_E:
            if(!_app && !_broker)
            {
                /* only application/broker can send to bus */
                return GT_BAD_PARAM;
            }

            break;
        case TARGET_OF_MESSAGE_DEVICE_E:
            if(_dev)
            {
                /* device not send those messages */
                return GT_BAD_PARAM;
            }
            break;
        case TARGET_OF_MESSAGE_ALL_E:
            if(_dev)
            {
                /* device not send those messages */
                return GT_BAD_PARAM;
            }

            /* set the debug level on my own too */
            simDistributedDebugFlagSet(debugLevel);

            break;
        default:
            return GT_BAD_PARAM;
    }

    connectionPtr = convertMessageTypeToConnectionIndex(msgType);

    /****************************/
    /* build the message header */
    /****************************/
    messageHeaderPtr = &message.hdr;
    messageHeaderPtr->type = (VARIABLE_TYPE)msgType;
    messageHeaderPtr->msgLen = (VARIABLE_TYPE)(dataLenWords*4);/* in bytes */

    /****************************/
    /* build the message info   */
    /****************************/

    message.mainTarget       = (VARIABLE_TYPE)mainTarget;
    message.secondaryTarget  = (VARIABLE_TYPE)secondaryTarget;
    message.debugLevel       = (VARIABLE_TYPE)debugLevel;


    if(mainTarget == TARGET_OF_MESSAGE_ALL_E ||
       secondaryTarget == ALL_SECONDARY_TARGETS)
    {
        /* need to loop on all the connections to the secondary target(s) */
        iiMax = connectionPtr->numSockets;
    }
    else
    {
        iiMax = 1;
    }


    for(ii = 0 ; ii < iiMax ; ii++)
    {
        if(mainTarget == TARGET_OF_MESSAGE_ALL_E ||
           secondaryTarget == ALL_SECONDARY_TARGETS)
        {
            clientIndex = ii;
        }
        else
        {
            /* convert specific secondary target to client index */
            convertDeviceIdToClientIndex(msgType,secondaryTarget,&clientIndex);
        }

        /**************************************/
        /* send the message to the other side */
        /**************************************/
        simDistributedMessageSend(clientIndex,connectionPtr,messageHeaderPtr,1,&dataLenWords, (GT_U32**)(&dataPtr),
            GT_FALSE,GT_FALSE,
            GT_FALSE);/* non blocking message -- because it is debug utility
                        and we don't want to be stacked  !!! */
    }

    return GT_OK;
}

/*******************************************************************************
*  simDistributedRemoteDebugLevelSendAll
*
* DESCRIPTION:
*      application send to all the distributed parts on the system
*      the FULL bmp of 'Debug level set' message
* INPUTS:
*      doDebug        - open or close debug level
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK - on success
*       GT_BAD_PARAM - when parameter not valid to this part of simulation
*
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS simDistributedRemoteDebugLevelSendAll
(
    IN GT_U32   doDebug
)
{
    return simDistributedRemoteDebugLevelSend(TARGET_OF_MESSAGE_ALL_E,
            ALL_SECONDARY_TARGETS, doDebug ? 0xffffffff : 0);
}


/*******************************************************************************
* parseBufferToHeader
*
* DESCRIPTION:
*       parse the header reply info from the buffer.
*       note : this doing "network order to host order" conversion
*
*
* INPUTS:
*       buffPtr - pointer to the header in the buffer to parse
*
* OUTPUTS:
*       messageHeaderPtr - pointer to the parsed header info
*
*
* RETURNS:
*       none
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static void parseBufferToHeader
(
    IN GT_U8    *buffPtr,
    OUT MSG_HDR  *messageHeaderPtr
)
{
    GT_U32  ii;
    GT_U32  length = sizeof(MSG_HDR)/4;

    for(ii = 0 ; ii < length ; ii++)
    {
        /* copy data from the buffer */
        ((GT_U32*)messageHeaderPtr)[ii] = SIM_HTONL(&buffPtr[4*ii]);
    }

    return;
}


/*******************************************************************************
* parseBufferToRegisterReply
*
* DESCRIPTION:
*       parse the read reply info from the buffer.
*       note : this doing "network order to host order" conversion
*
*
* INPUTS:
*       buffPtr - pointer to the buffer to parse
*       readReplyPtr - pointer to the parsed info
*             readReplyPtr->dataPtr is the place to put the data read from buffer
*             readReplyPtr->readLength is the place to put the data length (size in words)
*
* OUTPUTS:
*       readReplyPtr - pointer to the parsed info
*             readReplyPtr->dataPtr is the place to put the data read from buffer
*             readReplyPtr->readLength is the place to put the data length (size in words)
*
*
* RETURNS:
*       none
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static void parseBufferToRegisterReply
(
    IN GT_U8    *buffPtr,
    INOUT READ_RESPONSE_MSG *readReplyPtr
)
{
    GT_U32  ii;

    readReplyPtr->readLen = SIM_HTONL(buffPtr);/* first word is length */

    for(ii = 0 ; ii < readReplyPtr->readLen ; ii++)
    {
        /* copy data from the buffer */
        readReplyPtr->dataPtr[ii] = SIM_HTONL(&buffPtr[4*(ii+1)]);
    }

    return;
}

/*******************************************************************************
* parseBufferToDmaReply
*
* DESCRIPTION:
*       parse the read DMA reply info from the buffer.
*       note : this doing "network order to host order" conversion
*              according to dataIsWords
*
* INPUTS:
*       buffPtr - pointer to the buffer to parse
*       readReplyPtr - pointer to the parsed info
*             readReplyPtr->dataPtr is the place to put the data read from buffer
*             readReplyPtr->readLength is the place to put the data length (size in words)
*       dataIsWords - the data to read is words or bytes
*                     1 - words --> swap network order to cpu order
*                     0 - bytes --> NO swap network order to cpu order
* OUTPUTS:
*       readReplyPtr - pointer to the parsed info
*             readReplyPtr->dataPtr is the place to put the data read from buffer
*             readReplyPtr->readLength is the place to put the data length (size in words)
*
*
* RETURNS:
*       none
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static void parseBufferToDmaReply
(
    IN GT_U8    *buffPtr,
    INOUT DMA_READ_RESPONSE_MSG *readReplyPtr,
    IN GT_U32   dataIsWords
)
{
    GT_U32  ii;

    readReplyPtr->readLen = SIM_HTONL(buffPtr);/* first word is length */

    for(ii = 0 ; ii < readReplyPtr->readLen ; ii++)
    {
        /* copy data from the buffer */
        if(dataIsWords)
        {
            readReplyPtr->dataPtr[ii] = SIM_HTONL(&buffPtr[4*(ii+1)]);
        }
        else
        {
            ((GT_U8*)(readReplyPtr->dataPtr))[4*ii + 0] = buffPtr[4*(ii+1) + 0];
            ((GT_U8*)(readReplyPtr->dataPtr))[4*ii + 1] = buffPtr[4*(ii+1) + 1];
            ((GT_U8*)(readReplyPtr->dataPtr))[4*ii + 2] = buffPtr[4*(ii+1) + 2];
            ((GT_U8*)(readReplyPtr->dataPtr))[4*ii + 3] = buffPtr[4*(ii+1) + 3];
        }
    }

    return;
}

/*******************************************************************************
* messageRegisterReadReceived
*
* DESCRIPTION:
*       function Received the read register values , and put the data and
*       signal the waiting task
*
*       APPLICABLE SIDE : only on application side --> got the reply from the
*                         asic side
*
* INPUTS:
*       connectionPtr - pointer to connection info
*       offsetInBuf - the offset in the read buffer
*       messageHeaderPtr - pointer to the parsed header info
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
static void messageRegisterReadReceived(
    IN CONNECTION_INFO_STC *connectionPtr,
    IN GT_U32               offsetInBuf,
    IN MSG_HDR  *messageHeaderPtr
)
{
    DISTRIBUTED_MESSAGE_SYNC_INFO_STC *messageSyncPtr =
            &distributedMessageSync[PROTECT_REGISTER_READ_INDEX_E];
    READ_RESPONSE_MSG     readReply;


    /* set the pointer where to put the data itself */
    readReply.dataPtr = messageSyncPtr->dataPtr;

    /* parse the read reply info from the buffer.
       note : this doing "network order to host order" conversion */
    parseBufferToRegisterReply(connectionPtr->readBufPtr + offsetInBuf ,
                               &readReply);

    /* now we can wake the waiting task */
    SIM_OS_MAC(simOsSemSignal)(messageSyncPtr->semSync);

    return;
}

/*******************************************************************************
* parseBufferToRegisterRequest
*
* DESCRIPTION:
*       parse the read/write request info from the buffer.
*       note : this doing "network order to host order" conversion
*
*
* INPUTS:
*       buffPtr - pointer to the buffer to parse
*
* OUTPUTS:
*       readWriteRequestPtr - pointer to the parsed info
*
*
* RETURNS:
*       none
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static void parseBufferToRegisterRequest
(
    IN GT_U8    *buffPtr,
    INOUT READ_MSG *readWriteRequestPtr
)
{
    GT_U32  ii;
    GT_U32  length = LEN_NO_HEADER_WORDS(READ_MSG);

    for(ii = 0 ; ii < length ; ii++)
    {
        /* copy data from the buffer */
        ((GT_U32*)readWriteRequestPtr)[ii+HEADER_SIZE_WORDS] = SIM_HTONL(&buffPtr[ii*4]);
    }

    return;
}

/*******************************************************************************
* parseBufferToDmaRequest
*
* DESCRIPTION:
*       parse the read/write DMA request info from the buffer.
*       note : this doing "network order to host order" conversion
*
*
* INPUTS:
*       buffPtr - pointer to the buffer to parse
*
* OUTPUTS:
*       readWriteRequestPtr - pointer to the parsed info
*
*
* RETURNS:
*       none
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static void parseBufferToDmaRequest
(
    IN GT_U8    *buffPtr,
    INOUT DMA_READ_MSG *readWriteRequestPtr
)
{
    GT_U32  ii;
    GT_U32  length = LEN_NO_HEADER_WORDS(DMA_READ_MSG);

    for(ii = 0 ; ii < length ; ii++)
    {
        /* copy data from the buffer */
        ((GT_U32*)readWriteRequestPtr)[ii+HEADER_SIZE_WORDS] = SIM_HTONL(&buffPtr[ii*4]);

    }

    return;
}


/*******************************************************************************
* messageRegisterReadAndReply
*
* DESCRIPTION:
*       function Received request for read register values , and will read
*       registers and will sent reply with the data
*
*       APPLICABLE SIDE : only on Asic side --> got the request for read from the
*                         application side , and will return reply
*
* INPUTS:
*       connectionPtr - pointer to connection info
*       offsetInBuf - the offset in the read buffer
*       messageHeaderPtr - pointer to the parsed header info
*       clientIndex - client index , to send reply to .
*                      NOTE : reply sent on the "same socket it came from"
* OUTPUTS:
*       messageHeaderPtr - pointer to with the reply header info
*
* RETURNS:
*       none
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static void messageRegisterReadAndReply(
    IN CONNECTION_INFO_STC *connectionPtr,
    IN GT_U32               offsetInBuf,
    INOUT MSG_HDR           *messageHeaderPtr,
    IN GT_U32               clientIndex
)
{
    READ_MSG            readRegisterRequest;
    GT_U32  dataLen[2];
    GT_U32  *dataPtr[2];
    GT_U32  read=1;
    READ_RESPONSE_MSG   message;
    GT_U32  dataLenWords = LEN_NO_HEADER_WORDS(READ_RESPONSE_MSG) - PLACE_HOLDER_FIELD_WORDS_SIZE;/* remove the place holder word */

    /* parse the read request info from the buffer.
       note : this doing "network order to host order" conversion */
    parseBufferToRegisterRequest(connectionPtr->readBufPtr + offsetInBuf ,
                               &readRegisterRequest);

    /*******************************************************************/
    /* the reply use the same socket that the original message came on */

    if((readRegisterRequest.readLen * sizeof(GT_U32)) > connectionPtr->replyBufSize)
    {
        skernelFatalError(" messageRegisterReadAndReply : too many word to read[%d] , buffer not large enough[%d]",
                            readRegisterRequest.readLen,
                            connectionPtr->replyBufSize / sizeof(GT_U32));
    }

    if(msgRegisterRead)
    {
        msgRegisterRead(readRegisterRequest.accessType,
                        readRegisterRequest.deviceId,
                        readRegisterRequest.address,
                        readRegisterRequest.readLen,
                        (GT_U32*)connectionPtr->replyBufPtr,
                        connectionPtr);
    }
    else
    {
        if(_dev)
        {
            while(simulationInitReady == 0)
            {
                /* wait for indication that the Asic is ready */
                SIM_OS_MAC(simOsSleep)(WAIT_SLEEP_TIME_CNS);
            }
        }

        switch(readRegisterRequest.accessType)
        {
            case MSG_MEMORY_READ_E:
                scibReadMemory(readRegisterRequest.deviceId,
                               readRegisterRequest.address,
                               readRegisterRequest.readLen,
                               (GT_U32*)connectionPtr->replyBufPtr);
                break;
            case MSG_MEMORY_READ_PCI_E:
                scibPciRegRead(readRegisterRequest.deviceId,
                               readRegisterRequest.address,
                               readRegisterRequest.readLen,
                               (GT_U32*)connectionPtr->replyBufPtr);
                break;
            default:
                skernelFatalError(" messageRegisterReadAndReply : unknown accessType[%d]",
                       readRegisterRequest.accessType);
        }
    }

    message.readLen = readRegisterRequest.readLen;

    /* we got the data from SCIB , send it to the other side */
    dataLen[0] = dataLenWords;
    dataLen[1] = readRegisterRequest.readLen;

    dataPtr[0] = FIRST_FIELD_AFTER_HEADER_PTR(&message);
    dataPtr[1] = (GT_U32*)connectionPtr->replyBufPtr;

    /****************************/
    /* build the message header */
    /****************************/
    messageHeaderPtr = &message.hdr;
    /* update the message type */
    messageHeaderPtr->type = (VARIABLE_TYPE)MSG_TYPE_READ_RESPONSE;
    /* modify the total length field */
    messageHeaderPtr->msgLen = (VARIABLE_TYPE)((dataLen[0] + dataLen[1]) * 4);

    /**************************************/
    /* send the message to the other side */
    /**************************************/

    simDistributedMessageSend(clientIndex,connectionPtr,messageHeaderPtr,
                2,dataLen,dataPtr,
                GT_FALSE,GT_FALSE,GT_TRUE);

    LOG_INFO_REGISTER_SENT(read,readRegisterRequest.accessType,
                       readRegisterRequest.deviceId,
                       readRegisterRequest.address,
                       readRegisterRequest.readLen);

    LOG_INFO_READ_DATA_REPLY(readRegisterRequest.readLen,
                                      ((GT_U32*)connectionPtr->replyBufPtr));

    return;
}


/*******************************************************************************
* parseBufferToDataToWrite
*
* DESCRIPTION:
*       parse the write data to write to Dma/register/PCI from the buffer.
*       note : this doing "network order to host order" conversion
*
*
* INPUTS:
*       buffPtr - pointer to the buffer to parse
*       length - number of words of words value to parse from buffer
*
* OUTPUTS:
*       dataPtr - pointer to the data to write
*
*
* RETURNS:
*       none
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static void parseBufferToDataToWrite
(
    IN GT_U8    *buffPtr,
    IN GT_U32   length,
    OUT GT_U32  *dataPtr
)
{
    GT_U32  ii;

    for(ii = 0 ; ii < length ; ii++)
    {
        /* copy data from the buffer */
        dataPtr[ii] = SIM_HTONL(&buffPtr[ii*4]);
    }

    return;
}


/*******************************************************************************
* messageRegisterWrite
*
* DESCRIPTION:
*       function Received request for write register values , and will write
*       registers .(will not send reply)
*
*       APPLICABLE SIDE : only on Asic side --> got the request for write from
*                         the application side (will not send reply)
*
* INPUTS:
*       connectionPtr - pointer to connection info
*       offsetInBuf - the offset in the read buffer
*       messageHeaderPtr - pointer to the parsed header info
* OUTPUTS:
*       none
*
* RETURNS:
*       none
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static void messageRegisterWrite(
    IN CONNECTION_INFO_STC *connectionPtr,
    IN GT_U32               offsetInBuf,
    IN MSG_HDR  *messageHeaderPtr
)
{
    WRITE_MSG     writeRegisterRequest;
    GT_U32  read=0;
    GT_U32  dataLenWords = LEN_NO_HEADER_WORDS(WRITE_MSG) - PLACE_HOLDER_FIELD_WORDS_SIZE;/* remove the place holder word */


    /* first part of :parse the write request info from the buffer.
       get the info about the action
       note : this doing "network order to host order" conversion */
    parseBufferToRegisterRequest(connectionPtr->readBufPtr + offsetInBuf ,
                                (void *)&writeRegisterRequest);

    /* second part of :parse the write request info from the buffer.
       get the data to write
       note : this doing "network order to host order" conversion */
    parseBufferToDataToWrite(connectionPtr->readBufPtr + offsetInBuf + (dataLenWords*4),
                writeRegisterRequest.writeLen,
                (GT_U32*)connectionPtr->replyBufPtr);/* use the reply buffer */

    LOG_INFO_REGISTER_SENT(read,writeRegisterRequest.accessType,
                           writeRegisterRequest.deviceId,
                           writeRegisterRequest.address,
                           writeRegisterRequest.writeLen);
    LOG_INFO_WRITE_DATA_SENT(writeRegisterRequest.writeLen,
                                (GT_U32*)connectionPtr->replyBufPtr);

    if(msgRegisterWrite)
    {
        msgRegisterWrite(writeRegisterRequest.accessType,
                         writeRegisterRequest.deviceId,
                         writeRegisterRequest.address,
                         writeRegisterRequest.writeLen,
                         (GT_U32*)connectionPtr->replyBufPtr);
    }
    else
    {
        if(_dev)
        {
            while(simulationInitReady == 0)
            {
                /* wait for indication that the Asic is ready */
                SIM_OS_MAC(simOsSleep)(WAIT_SLEEP_TIME_CNS);
            }
        }

        switch(writeRegisterRequest.accessType)
        {
            case MSG_MEMORY_WRITE_E:
                scibWriteMemory(writeRegisterRequest.deviceId,
                               writeRegisterRequest.address,
                               writeRegisterRequest.writeLen,
                               (GT_U32*)connectionPtr->replyBufPtr);
                break;
            case MSG_MEMORY_WRITE_PCI_E:
                scibPciRegWrite(writeRegisterRequest.deviceId,
                               writeRegisterRequest.address,
                               writeRegisterRequest.writeLen,
                               (GT_U32*)connectionPtr->replyBufPtr);
                break;
            default:
                skernelFatalError(" messageRegisterWrite : unknown accessType[%d]",
                       writeRegisterRequest.accessType);
        }
    }

    return;
}


/*******************************************************************************
* messageNicTxFrame
*
* DESCRIPTION:
*       function Received request for sending Tx frame from NIC , and will send
*       the frame .(will not send reply)
*
*       APPLICABLE SIDE : only on Asic side --> got the request for sending Tx
*                         frame (from NIC from) the application side
*                         (will not send reply)
*
* INPUTS:
*       connectionPtr - pointer to connection info
*       offsetInBuf - the offset in the read buffer
*       messageHeaderPtr - pointer to the parsed header info
* OUTPUTS:
*       none
*
* RETURNS:
*       none
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static void messageNicTxFrame(
    IN CONNECTION_INFO_STC *connectionPtr,
    IN GT_U32               offsetInBuf,
    IN MSG_HDR  *messageHeaderPtr
)
{
    skernelFatalError(" messageNicTxFrame : not implemented \n");
    return;
}


/*******************************************************************************
* messageInterruptSet
*
* DESCRIPTION:
*       function Received request for setting the interrupt , and will set the
*       interrupt .(will not send reply)
*
*       APPLICABLE SIDE : only on Application side --> got the request for setting
*                         the interrupt from the Asic side
*                         (will not send reply)
*
* INPUTS:
*       connectionPtr - pointer to connection info
*       offsetInBuf - the offset in the read buffer
*       messageHeaderPtr - pointer to the parsed header info
* OUTPUTS:
*       none
*
* RETURNS:
*       none
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static void messageInterruptSet(
    IN CONNECTION_INFO_STC *connectionPtr,
    IN GT_U32               offsetInBuf,
    IN MSG_HDR  *messageHeaderPtr
)
{
    GT_U32  deviceId;

    /*note : this doing "network order to host order" conversion */
    /* the deviceId is right after the offset in the buffer */
    deviceId = (VARIABLE_TYPE)(SIM_HTONL(connectionPtr->readBufPtr + offsetInBuf));

    if(msgInterruptSet)
    {
        msgInterruptSet(deviceId);
    }
    else
    {
        scibSetInterrupt(deviceId);
    }

    LOG_INFO_REMOTE_INTERRUPT_SET(deviceId);

    return;
}


/*******************************************************************************
* messageDmaReadAndReply
*
* DESCRIPTION:
*       function Received request for read DMA values , and will read
*       DMA and will sent reply with the data
*
*       APPLICABLE SIDE : only on Application side --> got the request for read
*                         from the Asic side , and will return reply
*
* INPUTS:
*       connectionPtr - pointer to connection info
*       offsetInBuf - the offset in the read buffer
*       messageHeaderPtr - pointer to the parsed header info
*       clientIndex - client index , to send reply to .
*                      NOTE : reply sent on the "same socket it came from"
* OUTPUTS:
*       messageHeaderPtr - pointer to with the reply header info
*
* RETURNS:
*       none
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static void messageDmaReadAndReply(
    IN CONNECTION_INFO_STC *connectionPtr,
    IN GT_U32               offsetInBuf,
    INOUT MSG_HDR  *messageHeaderPtr,
    IN GT_U32               clientIndex
)
{
    DMA_READ_MSG     readDmaRequest;
    GT_U32  dataLen[2];
    GT_U32  *dataPtr[2];
    DMA_READ_RESPONSE_MSG   message;
    GT_U32  dataLenWords = LEN_NO_HEADER_WORDS(DMA_READ_RESPONSE_MSG) - PLACE_HOLDER_FIELD_WORDS_SIZE;/* remove the place holder word */

    /* parse the read DMA request info from the buffer.
       note : this doing "network order to host order" conversion */
    parseBufferToDmaRequest(connectionPtr->readBufPtr + offsetInBuf ,
                               &readDmaRequest);

    if((readDmaRequest.readLen * sizeof(GT_U32)) > connectionPtr->replyBufSize)
    {
        skernelFatalError(" messageDmaReadAndReply : too many word to read[%d] , buffer not large enough[%d]",
                            readDmaRequest.readLen,
                            connectionPtr->replyBufSize / sizeof(GT_U32));
    }

    if(msgDmaRead)
    {
        /* call CB to do it's action */
        msgDmaRead(SCIB_DMA_ACCESS_DUMMY_DEV_NUM_CNS,readDmaRequest.address,
                    readDmaRequest.readLen,
                    (GT_U32*)connectionPtr->replyBufPtr,
                     SCIB_DMA_WORDS/* don't care on application side*/);
    }
    else
    {
        /* call SCIB to do it's action */
        scibDmaRead(SCIB_DMA_ACCESS_DUMMY_DEV_NUM_CNS,readDmaRequest.address,
                    readDmaRequest.readLen,
                    (GT_U32*)connectionPtr->replyBufPtr,
                     SCIB_DMA_WORDS/* don't care on application side*/);
    }

    message.readLen = readDmaRequest.readLen;
    /* we got the data from SCIB , send it to the other side */

    dataLen[0] = dataLenWords;
    dataLen[1] = readDmaRequest.readLen;

    dataPtr[0] = FIRST_FIELD_AFTER_HEADER_PTR(&message);
    dataPtr[1] = (GT_U32*)connectionPtr->replyBufPtr;

    /****************************/
    /* build the message header */
    /****************************/
    messageHeaderPtr = &message.hdr;
    /* update the message type */
    messageHeaderPtr->type = (VARIABLE_TYPE)MSG_TYPE_DMA_READ_RESPONSE;
    /* modify the total length field */
    messageHeaderPtr->msgLen = (VARIABLE_TYPE)((dataLen[0] + dataLen[1]) * 4);

    /**************************************/
    /* send the message to the other side */
    /**************************************/
    simDistributedMessageSend(clientIndex,connectionPtr,messageHeaderPtr,
                2,dataLen,dataPtr,
                GT_FALSE,GT_FALSE,GT_TRUE);

    LOG_INFO_DMA_SENT(DMA_READ,readDmaRequest.address,readDmaRequest.readLen);
    LOG_INFO_READ_DATA_REPLY(readDmaRequest.readLen,(GT_U32*)connectionPtr->replyBufPtr);

    return;
}


/*******************************************************************************
* messageDmaReadReceived
*
* DESCRIPTION:
*       function Received the read DMA values , and put the data and
*       signal the waiting task
*
*       APPLICABLE SIDE : only on Asic side --> got the reply from the
*                         application side
*
* INPUTS:
*       connectionPtr - pointer to connection info
*       offsetInBuf - the offset in the read buffer
*       messageHeaderPtr - pointer to the parsed header info
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
static void messageDmaReadReceived(
    IN CONNECTION_INFO_STC *connectionPtr,
    IN GT_U32               offsetInBuf,
    IN MSG_HDR  *messageHeaderPtr
)
{
    DISTRIBUTED_MESSAGE_SYNC_INFO_STC *messageSyncPtr =
            &distributedMessageSync[PROTECT_DMA_READ_INDEX_E];
    DMA_READ_RESPONSE_MSG     readReply;


    /* set the pointer where to put the data itself */
    readReply.dataPtr = messageSyncPtr->dataPtr;

    /* parse the read reply info from the buffer.
       note : this doing "network order to host order" conversion,
       according to messageSyncPtr->dataIsWords */
    parseBufferToDmaReply(connectionPtr->readBufPtr + offsetInBuf ,
                               &readReply,messageSyncPtr->dataIsWords);

    /* now we can wake the waiting task */
    SIM_OS_MAC(simOsSemSignal)(messageSyncPtr->semSync);

    return ;
}


/*******************************************************************************
* messageDmaWrite
*
* DESCRIPTION:
*       function Received request for write DMA values , and will write
*       registers .(will not send reply)
*
*       APPLICABLE SIDE : only on Application side --> got the request for write
*                         DMA from the Asic side (will not send reply)
*
* INPUTS:
*       connectionPtr - pointer to connection info
*       offsetInBuf - the offset in the read buffer
*       messageHeaderPtr - pointer to the parsed header info
* OUTPUTS:
*       none
*
* RETURNS:
*       none
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static void messageDmaWrite(
    IN CONNECTION_INFO_STC *connectionPtr,
    IN GT_U32               offsetInBuf,
    IN MSG_HDR  *messageHeaderPtr
)
{
    DMA_WRITE_MSG     writeDmaRequest;
    GT_U32  dataLenWords = LEN_NO_HEADER_WORDS(DMA_WRITE_MSG) - PLACE_HOLDER_FIELD_WORDS_SIZE;/* remove the place holder word */


    /* first part of :parse the write request info from the buffer.
       get the info about the action
       note : this doing "network order to host order" conversion */
    parseBufferToDmaRequest(connectionPtr->readBufPtr + offsetInBuf ,
                               (void *)&writeDmaRequest);

    /* second part of :parse the write request info from the buffer.
       get the data to write
       note : this doing "network order to host order" conversion */
    parseBufferToDataToWrite(connectionPtr->readBufPtr + offsetInBuf + (dataLenWords*4),
                writeDmaRequest.writeLen,
                (GT_U32*)connectionPtr->replyBufPtr);/* use the reply buffer */

    LOG_INFO_DMA_SENT(DMA_WRITE,writeDmaRequest.address,writeDmaRequest.writeLen);
    LOG_INFO_WRITE_DATA_SENT(writeDmaRequest.writeLen,(GT_U32*)connectionPtr->replyBufPtr);

    if(msgDmaWrite)
    {
        msgDmaWrite(SCIB_DMA_ACCESS_DUMMY_DEV_NUM_CNS,
                     writeDmaRequest.address,
                     writeDmaRequest.writeLen,
                     (GT_U32*)connectionPtr->replyBufPtr,
                     SCIB_DMA_WORDS/* don't care on application side*/);
    }
    else
    {
        scibDmaWrite(SCIB_DMA_ACCESS_DUMMY_DEV_NUM_CNS,
                     writeDmaRequest.address,
                     writeDmaRequest.writeLen,
                     (GT_U32*)connectionPtr->replyBufPtr,
                     SCIB_DMA_WORDS/* don't care on application side*/);
    }

    return;
}

/*******************************************************************************
* parseBufferToRemoteInit
*
* DESCRIPTION:
*       parse the read/write DMA request info from the buffer.
*       note : this doing "network order to host order" conversion
*
*
* INPUTS:
*       buffPtr - pointer to the buffer to parse
*
* OUTPUTS:
*       readWriteRequestPtr - pointer to the parsed info
*
*
* RETURNS:
*       none
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static void parseBufferToRemoteInit
(
    IN GT_U8    *buffPtr,
    INOUT INIT_PARAM_MSG *remoteInitMessagePtr
)
{
    GT_U32  ii;
    GT_U32  length;
    GT_U32  *dstU32buffPtr;

    dstU32buffPtr = (GT_U32*)&remoteInitMessagePtr->deviceInfo;

    length = (sizeof(INIT_PARAM_SINGLE_DEVICE_INFO)/4);/* data of the device */

    for(ii = 0 ; ii < length ; ii++ ,dstU32buffPtr++)
    {
        /* copy data from the buffer */
        *dstU32buffPtr =  SIM_HTONL(&buffPtr[4*(ii)]);
    }

    return;
}

/*******************************************************************************
* messageRemoteInit
*
* DESCRIPTION:
*       function Received message to initialize a device of the Asic on SCIB
*       of application .(will not send reply)
*
*       APPLICABLE SIDE : on Application side/broker/bus --> got the init message from
*                        Asic side (will not send reply)
*
* INPUTS:
*       connectionPtr - pointer to connection info
*       offsetInBuf - the offset in the read buffer
*       messageHeaderPtr - pointer to the parsed header info
* OUTPUTS:
*       none
*
* RETURNS:
*       none
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static void messageRemoteInit(
    IN CONNECTION_INFO_STC *connectionPtr,
    IN GT_U32               offsetInBuf,
    IN MSG_HDR              *messageHeaderPtr,
    IN GT_SOCKET_FD         clientSocketId
)
{
    INIT_PARAM_MSG     remoteInitMessage;
    INIT_PARAM_SINGLE_DEVICE_INFO *devInfoPtr;
    GT_U32  ii;
    GT_U32  clientIndex = 0;


    /* first part of :parse the write request info from the buffer.
       get the info about the action
       note : this doing "network order to host order" conversion */
    parseBufferToRemoteInit(connectionPtr->readBufPtr + offsetInBuf ,
                               &remoteInitMessage);

    for(ii = 0 ; ii < connectionPtr->numSockets ; ii++)
    {
        if(clientSocketId == connectionPtr->socketsArray[ii].sockId)
        {
            clientIndex = ii;
            break;
        }
    }

    if(ii == connectionPtr->numSockets)
    {
        skernelFatalError(" messageRemoteInit : client not found");
    }

    devInfoPtr = &remoteInitMessage.deviceInfo;

    LOG_INFO_REMOTE_INIT_DEVICE_INFO(devInfoPtr->deviceId,
                   devInfoPtr->deviceHwId,
                   devInfoPtr->interruptLine,
                   devInfoPtr->isPp,
                   devInfoPtr->addressCompletionStatus,
                   devInfoPtr->nicDevice);

    /* need to send the message towards the application */
    if(msgDeviceInit)
    {
        SIM_DISTRIBUTED_INIT_DEVICE_STC initDev;

        initDev.deviceId                = devInfoPtr->deviceId;
        initDev.deviceHwId              = devInfoPtr->deviceHwId;
        initDev.interruptLine           = devInfoPtr->interruptLine;
        initDev.isPp                    = devInfoPtr->isPp;
        initDev.addressCompletionStatus = devInfoPtr->addressCompletionStatus;
        initDev.nicDevice               = devInfoPtr->nicDevice;

        msgDeviceInit(&initDev);
    }
    else
    {
        /* init the scib with needed info */
        scibRemoteInit(devInfoPtr->deviceId,
                       devInfoPtr->deviceHwId,
                       devInfoPtr->interruptLine,
                       devInfoPtr->isPp,
                       devInfoPtr->addressCompletionStatus);
    }

    if(_bus)
    {
        /* the bus will have device(s) on different sockets */
        if(devInfoPtr->deviceId >= MAX_DEVICES_ON_BOARD_CNS)
        {
            skernelFatalError(" messageRemoteInit : deviceId[%d] >= max supported[%d] \n",
                devInfoPtr->deviceId , MAX_DEVICES_ON_BOARD_CNS);
        }

        if(mapDeviceToClientIndex[devInfoPtr->deviceId] != CLIENT_NOT_READY)
        {
            skernelFatalError(" messageRemoteInit : deviceId[%d] is already registered in DB \n",
                devInfoPtr->deviceId);
        }

        mapDeviceToClientIndex[devInfoPtr->deviceId] = clientIndex;
    }

    /* nic not supported yet */
    if(devInfoPtr->nicDevice == SKERNEL_NIC_CNS)
    {
        nicInfo.nicDevNum = devInfoPtr->deviceId;
        nicInfo.nicInitialized = GT_TRUE;
    }

    return;
}

/*******************************************************************************
* messageReset
*
* DESCRIPTION:
*       function Received RESET message .
*       and it need to send the message on to extra distributed parts of the
*       system before resetting itself
*
*       APPLICABLE SIDE : on broker/bus/device --> got the reset message from
*                        application side (will not send reply)
*
* INPUTS:
*       None
*
* OUTPUTS:
*       None
*
* RETURNS:
*       none
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static void messageReset(
    void
)
{

    if(!_dev)
    {
        if(msgReset)
        {
            msgReset();/* send the message to all other parts */
        }
        else
        {
            simDistributedRemoteResetSend(FIRST_SOCKET);/* send the message to all other parts */
        }

        /* allow the message to be sent to the next part(s) */
        SIM_OS_MAC(simOsSleep)(RESET_WAIT_SLEEP_TIME_CNS);
    }

    /* notify that we are under reset , the function simDistributedExit will be
       called eventually from SHOSTG_reset and we what it to close the com port
       before the reset */
    resetSystem = 1;

    /* notify that we NOT come from external call but from a message */
    externalCall = 0;

    /* call to do proper 'reset' action */
    SHOSTG_reset(REMOTE_RESET_CODE_CNS);
    /* NOTE : this function will eventually call : simDistributedExit() */
}


/*******************************************************************************
* messageDebugLevelSet
*
* DESCRIPTION:
*       function Received message to set debug level
*
*       APPLICABLE SIDE : on broker/bus/device --> got the 'debug level set'
*                        message from Application side (will not send reply)
*
* INPUTS:
*       connectionPtr - pointer to connection info
*       offsetInBuf - the offset in the read buffer
*       messageHeaderPtr - pointer to the parsed header info
* OUTPUTS:
*       none
*
* RETURNS:
*       none
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static void messageDebugLevelSet(
    IN CONNECTION_INFO_STC *connectionPtr,
    IN GT_U32               offsetInBuf,
    IN MSG_HDR              *messageHeaderPtr
)
{
    GT_U32  ii;
    DEBUG_LEVEL_MSG     msgInfo;
    GT_U32  length = LEN_NO_HEADER_WORDS(DEBUG_LEVEL_MSG);
    GT_U8   *buffPtr = connectionPtr->readBufPtr + offsetInBuf;
    GT_U32  *u32Ptr = (GT_U32*)((void*)&msgInfo);
    GT_U32  target = 0; /* indication that my process is the target for message */
    GT_U32  sendToNext = 1;/* indication that we need to send to next part of the system */

    for(ii = 0 ; ii < length ; ii++)
    {
        /* copy data from the buffer */
        u32Ptr[ii+HEADER_SIZE_WORDS] = SIM_HTONL(&buffPtr[ii*4]);
    }

    switch(msgInfo.mainTarget)
    {
        case TARGET_OF_MESSAGE_BROKER_E:
            if(_broker)
            {
                target = 1;
            }
            break;
        case TARGET_OF_MESSAGE_BUS_E:
            if(_bus)
            {
                target = 1;
            }
            break;
        case TARGET_OF_MESSAGE_DEVICE_E:
            if(_dev)
            {
                target = 1;
            }
            break;
        case TARGET_OF_MESSAGE_ALL_E:

            target = 1;

            break;

    }

    if(target == 1 && msgInfo.mainTarget != TARGET_OF_MESSAGE_ALL_E)
    {
        /* we are target and this is not broadcast message */
        sendToNext = 0;
    }
    else if(_dev)
    {
        /* we are the final in chain */
        sendToNext = 0;
    }

    if(target)
    {
        /* set the debug level on my own , because this is target to message */
        simDistributedDebugFlagSet(msgInfo.debugLevel);
    }

    if(sendToNext)
    {
        /* this function send to next */
        simDistributedRemoteDebugLevelSend(msgInfo.mainTarget,
                                           msgInfo.secondaryTarget,
                                           msgInfo.debugLevel);
    }
}

/*******************************************************************************
* distributedDispatcherMainTask
*
* DESCRIPTION:
*       main Task that wait for messages from the sockets and act according to
*       received message.
*       this task runs on the both sides : application side and asic side
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
static unsigned __TASKCONV distributedDispatcherMainTask(IN void* connPtr)
{
    GT_U32      bufSize;/* current size of buffer that was read from socket */
    GT_U32      offsetInBuf;/* current offset in the read buffer */
    GT_SOCK_FD_SET_STC rfds;/* rfds - set of sockets to be checked for readability */
    MSG_HDR  messageHeader;/* header of current treated message */
    GT_U32  ii;
    GT_SOCKET_FD    currSockId = 0;
    GT_U32  currMessageLen;
    GT_U32  doReceiveAgain = 0;/* flag to identify that the current receive has more
                            info beside the given on buffer */
    GT_U32  alreadyOnBuffer = 0;/* number of bytes that we have on buffer from
                                the last receive*/
    GT_U32  waitForConnect = 0;/* did we wait for connection with the other side */
    GT_U32  clientIndex;/* (relevant to server only) client index in the array
                            of socketsArray[] */
    GT_SOCKET_FD maxSockId;/* the max socket Is for the use of osSocket(..)*/
    CONNECTION_INFO_STC *connectionPtr = connPtr;
    CONNECTION_TYPE_ENT connectionType = connectionPtr->connectionType;
    GT_BOOL retVal = GT_TRUE;

#if 0
    connectionPtr->debugTask = 1;

    while(connectionPtr->debugTask == 1)/* this value need to be changed inside tornado */
    {
        printf("distributedDispatcherMainTask: wait on debug task \n");
        SIM_OS_MAC(simOsSleep)(4*WAIT_SLEEP_TIME_CNS);
    }
#endif/*0*/
    rfds.fd_set = SIM_OS_MAC(osSelectCreateSet)();

    /* buffer for read */
    connectionPtr->readBufSize = MAX_BUFFER_SIZE_CNS;
    connectionPtr->readBufPtr = malloc(connectionPtr->readBufSize);

    /* buffer for reply */
    connectionPtr->replyBufSize = MAX_BUFFER_SIZE_SINGLE_MESSAGE_CNS;
    connectionPtr->replyBufPtr = malloc(connectionPtr->replyBufSize);

    if(connectionPtr->socketServerSide == GT_FALSE)
    {
        /* establish connection for this client in connection to server */
        for(ii = 0 ; ii < connectionPtr->numSockets; ii++)
        {
            clientConnect(connectionPtr,ii);
        }
    }


    while(1)
    {
        if(connectionPtr->numSockets != 0)
        {
            for(ii = 0 ; ii < connectionPtr->numSockets; ii++)
            {
                while(connectionPtr->socketsArray[ii].ready == 0)
                {
                    if(waitForConnect == 0)
                    {
                        waitForConnect = 1;
                        printf("distributedDispatcherMainTask: %s wait for client [%d]",
                            iniFileTcpPortsStr[connectionType],ii);
                    }
                    else
                    {
                        printf(".");
                    }

                    SIM_OS_MAC(simOsSleep)(WAIT_SLEEP_TIME_CNS);
                }
            }

            break;/* this breaks the while(1) */
        }

        if(waitForConnect == 0)
        {
            waitForConnect = 1;
            printf("distributedDispatcherMainTask: %s wait for first client ",
                iniFileTcpPortsStr[connectionType]);
        }
        else
        {
            printf(".");
        }

        SIM_OS_MAC(simOsSleep)(WAIT_SLEEP_TIME_CNS);
    }

    if(waitForConnect)
    {
        printf("\n");
    }

    printf(" !!! SUCCESS !!! \n");
    printf(" Dispatcher task connected to : \n");
    printConnectionSockets(connectionPtr);

    while(1)
    {

        /* clear the sets of sockets */
        SIM_OS_MAC(osSelectZeroSet)(rfds.fd_set);

        maxSockId = 0;
        for(ii = 0 ; ii < connectionPtr->numSockets ; ii++)
        {
            /* add the needed sockets to the needed sets(of sockets) */
            SIM_OS_MAC(osSelectAddFdToSet)(rfds.fd_set,connectionPtr->socketsArray[ii].sockId);

            if(maxSockId < connectionPtr->socketsArray[ii].sockId)
            {
                maxSockId = connectionPtr->socketsArray[ii].sockId;
            }
        }

        /* wait until any socket has info */
        if(0 == SIM_OS_MAC(osSelect)(maxSockId+1,rfds.fd_set,NULL,NULL,(GT_U32)-1/*INFINITE*/))
        {
            /* avoid busy wait in this case */
            SIM_OS_MAC(simOsSleep)(WAIT_SLEEP_TIME_CNS);
            continue;
        }

        for(ii = 0 ; ii < connectionPtr->numSockets ; ii++)
        {
            /* add the needed sockets to the needed sets(of sockets) */
            retVal = SIM_OS_MAC(osSelectIsFdSet)(rfds.fd_set,connectionPtr->socketsArray[ii].sockId);
            if(retVal == GT_TRUE)
            {
                currSockId = connectionPtr->socketsArray[ii].sockId;
                break;
            }
        }
        /* file descriptor has not been set */
        if(retVal == GT_FALSE)
        {
            /* avoid busy wait in this case */
            SIM_OS_MAC(simOsSleep)(WAIT_SLEEP_TIME_CNS);
            continue;
        }

        /*******************************/
        /* we got info from the socket */
        /*******************************/
receive_data_from_socket_lbl:

        if(doReceiveAgain == 0)
        {
            alreadyOnBuffer = 0;
        }

        /* set the max size to read to be the size of buffer */
        bufSize = connectionPtr->readBufSize;

        LOG_SOCKET_ID_RECEIVE((GT_U32)currSockId);

        /* read the data from socket into local buffer */
        bufSize = SIM_OS_MAC(osSocketRecv)(currSockId,
                                      connectionPtr->readBufPtr + alreadyOnBuffer,
                                      bufSize - alreadyOnBuffer,GT_TRUE);

        if(bufSize == 0)
        {
            printf("distributedDispatcherMainTask [%s]: osSocketRecv error -- BUFF 0 \n",iniFileTcpPortsStr[connectionType]);
            break;
        }
        else if((bufSize == (GT_U32)(-1)))
        {
            printf("distributedDispatcherMainTask [%s]: osSocketRecv error -- BUFF -1 \n",iniFileTcpPortsStr[connectionType]);
            break;
        }

        /* restore the number of bytes already on buffer */
        bufSize += alreadyOnBuffer;

        /* reset the number for next time */
        alreadyOnBuffer = 0;

        if(bufSize > connectionPtr->readBufSize)
        {
            skernelFatalError(" distributedDispatcherMainTask : bufSize[%d] > connectionPtr->readBufSize[%d]",
                        bufSize,connectionPtr->readBufSize);
        }

        /* check if buffer was not long enough */
        if(bufSize > connectionPtr->readBufSize)
        {
            doReceiveAgain = 1;
            bufSize = connectionPtr->readBufSize;
        }
        else
        {
            doReceiveAgain = 0;
        }

        /***********************************************/
        /* start to treat the messaged from the socket */
        /***********************************************/
        offsetInBuf = 0;

        /* loop on messages in current read buffer */
        while (1)
        {
            if(offsetInBuf >= bufSize)
            {
                /* no more messages to read */
                break;
            }

            if((offsetInBuf + sizeof(messageHeader)) > bufSize)
            {
                /* not enough space for new message to be treated */
                doReceiveAgain = 1;
                break;
            }

            /* parse the header of the message from the buffer.
               note : this doing "network order to host order" conversion */
            parseBufferToHeader(connectionPtr->readBufPtr + offsetInBuf,
                                &messageHeader);

            /* save the original message length include the header and the
               trailer */
            currMessageLen = TOTAL_LEN_BYTES(messageHeader.msgLen);

            /* check that current position allow to get next message */
            if((offsetInBuf + TOTAL_LEN_BYTES(messageHeader.msgLen)) > bufSize)
            {
                /* not enough space for new message to be treated */
                doReceiveAgain = 1;
                break;
            }

            /* update the offset in buffer after the read */
            offsetInBuf += sizeof(messageHeader);

            switch(messageHeader.type)
            {
                case DISTRIBUTED_MESSAGE_TYPE_REGISTER_READ_REPLY_E:
                    /* application side --> got the reply from the asic side */
                    /* put the data and signal the waiting task */
                    messageRegisterReadReceived(connectionPtr,/* pointer to connection info */
                                                offsetInBuf,/* the offset in the read buffer */
                                                &messageHeader);/* pointer to the parsed header info */
                    break;
                case DISTRIBUTED_MESSAGE_TYPE_REGISTER_READ_E:
                    /* asic side --> got the request from the application side */
                    /* read register and reply to message */
                    /* convert socketId to socket index */
                    /* because the reply will be done on the same socket !!! */
                    clientIndex = convertSocketIdToClientIndex(connectionPtr,currSockId);
                    messageRegisterReadAndReply(connectionPtr,/* pointer to connection info */
                                                offsetInBuf,/* the offset in the read buffer */
                                                &messageHeader,/* pointer to the parsed header info */
                                                clientIndex);/* socketId to reply to */
                    break;
                case DISTRIBUTED_MESSAGE_TYPE_REGISTER_WRITE_E:
                    /* write register */
                    messageRegisterWrite(connectionPtr,/* pointer to connection info */
                                         offsetInBuf,/* the offset in the read buffer */
                                         &messageHeader);/* pointer to the parsed header info */
                    break;
                case DISTRIBUTED_MESSAGE_TYPE_NIC_TX_FRAME_E:
                    /* Tx frame to nic */
                    messageNicTxFrame(connectionPtr,/* pointer to connection info */
                                      offsetInBuf,/* the offset in the read buffer */
                                      &messageHeader);/* pointer to the parsed header info */
                    break;
                case DISTRIBUTED_MESSAGE_TYPE_INTERRUPT_SET_E:
                    /* set interrupt */
                    messageInterruptSet(connectionPtr,/* pointer to connection info */
                                        offsetInBuf,/* the offset in the read buffer */
                                        &messageHeader);/* pointer to the parsed header info */
                    break;
                case DISTRIBUTED_MESSAGE_TYPE_DMA_READ_E:
                    /* application side --> got the request from the asic side */
                    /* convert socketId to socket index */
                    /* because the reply will be done on the same socket !!! */
                    clientIndex = convertSocketIdToClientIndex(connectionPtr,currSockId);

                    /* read DMA and reply to message */
                    messageDmaReadAndReply(connectionPtr,/* pointer to connection info */
                                           offsetInBuf,/* the offset in the read buffer */
                                           &messageHeader,/* pointer to the parsed header info */
                                           clientIndex);/* socketId to reply to */
                    break;
                case DISTRIBUTED_MESSAGE_TYPE_DMA_READ_REPLY_E:
                    /* asic side --> got the reply from the application side */
                    /* put the data and signal the waiting task */
                    messageDmaReadReceived(connectionPtr,/* pointer to connection info */
                                           offsetInBuf,/* the offset in the read buffer */
                                           &messageHeader);/* pointer to the parsed header info */
                    break;
                case DISTRIBUTED_MESSAGE_TYPE_DMA_WRITE_E:
                    /* write DMA */
                    messageDmaWrite(connectionPtr,/* pointer to connection info */
                                    offsetInBuf,/* the offset in the read buffer */
                                    &messageHeader);/* pointer to the parsed header info */
                    break;
                case DISTRIBUTED_MESSAGE_TYPE_DEVICE_TO_APPLICATION_INIT_PARAM_E:
                    /* init the scib with the devices */
                    messageRemoteInit(connectionPtr,/* pointer to connection info */
                                    offsetInBuf,/* the offset in the read buffer */
                                    &messageHeader,/* pointer to the parsed header info */
                                    currSockId);/* clientSocketId */
                    break;
                case DISTRIBUTED_MESSAGE_TYPE_RESET_E:
                    /* Reset the process */
                    messageReset();
                    break;
                case DISTRIBUTED_MESSAGE_TYPE_DEBUG_LEVEL_SET:
                    /* debug level set */
                    messageDebugLevelSet(connectionPtr,/* pointer to connection info */
                                    offsetInBuf,/* the offset in the read buffer */
                                    &messageHeader);/* pointer to the parsed header info */
                    break;

                case DISTRIBUTED_MESSAGE_TYPE_APP_PID_E:
                case DISTRIBUTED_MESSAGE_TYPE_REGISTER_DMA_E:
                case DISTRIBUTED_MESSAGE_TYPE_REGISTER_INTERRUPT_E:
                case DISTRIBUTED_MESSAGE_TYPE_UNMASK_INTERRUPT_E:
                case DISTRIBUTED_MESSAGE_TYPE_MASK_INTERRUPT_E:
                    if(_broker == 0)
                    {
                        skernelFatalError(" distributedDispatcherMainTask : not supported type[%d] --> must be broker to get those",
                                            messageHeader.type);
                    }
                    /* broker support for those messages */
                    brokerMessageExtended(messageHeader.type,/* type of message*/
                                        connectionPtr,/* pointer to connection info */
                                        offsetInBuf,/* the offset in the read buffer */
                                        &messageHeader);/* pointer to the parsed header info */
                    break;

                default:
                    skernelFatalError(" distributedDispatcherMainTask : unknown type[%d]",
                                        messageHeader.type);
                    break;
            }/* end of switch-case */

            /* update offset in buffer to the end of current message
               (that we already handled) */
            offsetInBuf = offsetInBuf - sizeof(messageHeader) + currMessageLen;

        }/* end of while on messages in current read buffer */


        if(doReceiveAgain)
        {
            /* we got all possible messages from the buffer */

            /* copy the last part of the message , put it in start of buffer ,
            so we can put the rest of it coming next from the osSocketRecv */
            alreadyOnBuffer = bufSize - offsetInBuf;

            memcpy(connectionPtr->readBufPtr,
                   &connectionPtr->readBufPtr[offsetInBuf],
                   alreadyOnBuffer);

            /* do not goto start of the select ... , go right to the receiving */
            goto receive_data_from_socket_lbl;
        }
    }/* end of while on wait for messages */

    printf(" !! ERROR !!! closing Dispatcher on : \n");
    printConnectionSockets(connectionPtr);

    /* the connection on the other side collapsed ! */
    for(ii = 0 ; ii < connectionPtr->numSockets ; ii++)
    {
        if(connectionPtr->socketsArray[ii].sockId == 0)
        {
            continue;
        }
        connectionPtr->socketsArray[ii].ready = 0;

        /* so close the connection to it */
        SIM_OS_MAC(osSocketSetSocketNoLinger)(connectionPtr->socketsArray[ii].sockId);
        SIM_OS_MAC(osSocketShutDown)(connectionPtr->socketsArray[ii].sockId,SOCKET_SHUTDOWN_CLOSE_ALL);
        SIM_OS_MAC(osSocketTcpDestroy)(connectionPtr->socketsArray[ii].sockId);

        connectionPtr->socketsArray[ii].sockId = 0;

    }

    connectionPtr->numSockets = 0;

    printf("distributedDispatcherMainTask [%s]: abort \n",iniFileTcpPortsStr[connectionType]);

    /***************************************************************/
    /* we have need to 'reset' this side , so just abort this task */
    /***************************************************************/

    /* we got disconnected */
    return 0;
}

/*******************************************************************************
* distributedDispatcherNicRxTask
*
* DESCRIPTION:
*       main Task that wait for messages only from the socket of the "nic rx
*       frames"
*       this task runs only on the application side
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
static unsigned __TASKCONV distributedDispatcherNicRxTask(IN void* dummy)
{
    return 0;
}


/*******************************************************************************
* distributedDispatcherServerAccept
*
* DESCRIPTION:
*       Task that run only on a server of sockets to accept connections from
*       clients .
*
*       Task is terminated when no more connections are allowed
*
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
static unsigned __TASKCONV distributedDispatcherServerAccept(IN void* connPtr)
{
    CONNECTION_INFO_STC *connectionPtr = connPtr;
    CONNECTION_TYPE_ENT connectionType = connectionPtr->connectionType;
    GT_SOCKET_FD     newConnection;
    SOCKET_INFO_STC  *socketPtr;

    printf(" start accepting connection to server : \n");
    printConnectionSockets(connectionPtr);

    /* bind the socket */
    if (GT_OK != SIM_OS_MAC(osSocketBind) (connectionPtr->serverSocket.sockId,
        connectionPtr->serverSocket.sockAddr,
        connectionPtr->serverSocket.sockAddrLen))
    {
        skernelFatalError(" distributedDispatcherServerAccept : osSocketBind");
    }

    /*************************************/
    /* wait for client to try to connect */
    /*************************************/
    if (GT_OK != SIM_OS_MAC(osSocketListen) (connectionPtr->serverSocket.sockId, 0))
    {
        skernelFatalError(" distributedDispatcherServerAccept : osSocketListen");
    }

    while(1)
    {
        /*************************/
        /* Accept new connection */
        /*************************/
        newConnection =
            SIM_OS_MAC(osSocketAccept) (connectionPtr->serverSocket.sockId,NULL,NULL);

        if(newConnection == 0)
        {
            /* can happen only when process do 'shutdown' */
            break;
        }

        SIM_OS_MAC(simOsSocketSetSocketNoDelay)(newConnection);

        LOG_INFO_CONNECTION_INIT(connectionType,newConnection);

        socketPtr = &connectionPtr->socketsArray[connectionPtr->numSockets];
        socketPtr->sockId = newConnection;
        socketPtr->ready = 1;
        /* update the num of client only after all fields are done ,
           so no semaphore needed to synch with the other tasks */
        connectionPtr->numSockets++;
    }

    return 0;
}

/*******************************************************************************
* debugSimDistributedPerformanceCheck
*
* DESCRIPTION:
*       debug function to test performances . debug utility
*
*       APPLICABLE SIDE : only on Application side
*
* INPUTS:
*       type        - one of DEBUG_SIM_DISTRIBUTED_PERFORMANCE_CHECK_ENT
*       loopSize    - number of loops to read/write
*       address     - the address of the register to read/write
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK - check was done , successfully
*       GT_BAD_STATE - function called not on application side
*       GT_GET_ERROR - read after write failed , the read value was different
*                      then the write value
*       GT_BAD_PARAM - bad type value
*
* COMMENTS:
*       None.
*
*******************************************************************************/
GT_STATUS debugSimDistributedPerformanceCheck
(
    IN GT_U32   type,
    IN GT_U32   loopSize,
    IN GT_U32   address
)
{
    GT_U32  ii;
    GT_U32  deviceId = 0;
    GT_U32  memSize = 1;
    GT_U32  writeValue;
    GT_U32  origValue = 0x11223344;
    GT_U32  readValue;

    printf("simDistributedPerformanceCheck : start\n");

    switch((DEBUG_SIM_DISTRIBUTED_PERFORMANCE_CHECK_ENT)type)
    {
        case DEBUG_SIM_DISTRIBUTED_PERFORMANCE_CHECK_REGISTER_READ_E:
            for (ii = 0 ; ii < loopSize; ii++)
            {
                simDistributedRegisterRead(MSG_MEMORY_READ_E,deviceId,address,memSize,&readValue);
            }
            break;
        case DEBUG_SIM_DISTRIBUTED_PERFORMANCE_CHECK_REGISTER_WRITE_E:
            for (ii = 0 ; ii < loopSize; ii++)
            {
                /* set different value to the register every time */
                writeValue = origValue + ii;
                simDistributedRegisterWrite(MSG_MEMORY_WRITE_E,deviceId,address,memSize,&writeValue);
            }
            break;
        case DEBUG_SIM_DISTRIBUTED_PERFORMANCE_CHECK_REGISTER_WRITE_AND_READ_E:
            for (ii = 0 ; ii < loopSize; ii++)
            {
                /* set different value to the register every time */
                writeValue = origValue + ii;

                /* write value to register */
                simDistributedRegisterWrite(MSG_MEMORY_WRITE_E,deviceId,address,memSize,&writeValue);

                /* then read the register */
                simDistributedRegisterRead(MSG_MEMORY_READ_E,deviceId,address,memSize,&readValue);

                /* check that read value = write value */
                if(readValue != writeValue)
                {
                    printf(" simDistributedPerformanceCheck : read value[0x%8.8x] != write value[0x%8.8x]",
                        readValue , writeValue);

                    return GT_GET_ERROR;
                }
            }
            break;
        default:
            return GT_BAD_PARAM;
    }

    printf("simDistributedPerformanceCheck : end\n");

    return GT_OK;
}

/*******************************************************************************
* debugSimDistributedTraceSet
*
* DESCRIPTION:
*       debug function to set the flags of the trace printings .  debug utility
*
*       APPLICABLE SIDE : both sides
*
* INPUTS:
*       actionType - the action type : one of DEBUG_SIM_DISTRIBUTED_TRACE_ACTION_ENT
*       flag    - the trace flag to set : one of DEBUG_SIM_DISTRIBUTED_TRACE_FLAG_ENT
*                       revenant when actionType is:
*                           DEBUG_SIM_DISTRIBUTED_TRACE_ACTION_ADD_E or
*                           DEBUG_SIM_DISTRIBUTED_TRACE_ACTION_REMOVE_E
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK - on success
*       GT_BAD_PARAM - bad actionType or flag
*
* COMMENTS:
*       None.
*
*******************************************************************************/
GT_STATUS debugSimDistributedTraceSet
(
    IN GT_U32   actionType,
    IN GT_U32   flag
)
{
    switch(actionType)
    {
        case DEBUG_SIM_DISTRIBUTED_TRACE_ACTION_ADD_E:
        case DEBUG_SIM_DISTRIBUTED_TRACE_ACTION_REMOVE_E:
            switch(flag)
            {
                case DEBUG_SIM_DISTRIBUTED_TRACE_FLAG_REGISTER_E:
                case DEBUG_SIM_DISTRIBUTED_TRACE_FLAG_DMA_E:
                case DEBUG_SIM_DISTRIBUTED_TRACE_FLAG_INTERRUPT_E:
                case DEBUG_SIM_DISTRIBUTED_TRACE_FLAG_NIC_RX_E:
                case DEBUG_SIM_DISTRIBUTED_TRACE_FLAG_NIC_TX_E:
                case DEBUG_SIM_DISTRIBUTED_TRACE_FLAG_SEQUENCE_NUM_E:
                case DEBUG_SIM_DISTRIBUTED_TRACE_FLAG_CONNECTION_INIT_E:
                case DEBUG_SIM_DISTRIBUTED_TRACE_FLAG_REMOTE_INIT_E:
                    break;

                default:
                    return GT_BAD_PARAM;
            }

            if(actionType == DEBUG_SIM_DISTRIBUTED_TRACE_ACTION_ADD_E)
            {
                /* add the flag */
                logInfoUsed |= (1 << flag);
            }
            else
            {
                /* remove the flag */
                logInfoUsed &= ~(1 << flag);
            }
            break;
        case DEBUG_SIM_DISTRIBUTED_TRACE_ACTION_ADD_ALL_E:
            /* set all flags */
            logInfoUsed = 0xFFFFFFFF;
            break;
        case DEBUG_SIM_DISTRIBUTED_TRACE_ACTION_REMOVE_ALL_E:
            /* clear all flags */
            logInfoUsed = 0;
            break;
        default:
            return GT_BAD_PARAM;
    }

    return GT_OK;
}

/*******************************************************************************
* debugSimDistributedSet
*
* DESCRIPTION:
*       debug function to set/unset the option to have debug printings . debug utility
*
*       APPLICABLE SIDE : both sides
*
* INPUTS:
*       usedDebug - 0 - don't use debug printings (trace)
*                   otherwise - use debug printings (trace)
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK - on success
*
* COMMENTS:
*       None.
*
*******************************************************************************/
GT_STATUS debugSimDistributedSet
(
    IN GT_U32   usedDebug
)
{
    doDebug = usedDebug ? 1 : 0;
    if(doDebug == 0)
    {
        /* close the debugging */
        logInfoUsed = 0;
    }
    return GT_OK;
}


/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/**********************   BROKER specifics ************************************/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/

/* define the number of words to allow write in single write action */
#define WRITE_NUM_WORDS_CNS 32

/* max number of buffers to be in the broker queue */
#define BROKER_QUEUE_SIZE           100
/* the size of each buffer in the queue */
#define BROKER_BUFFERS_SIZE         sizeof(BROKER_BUF_INFO_STC)
/* a size to allocate for cookies in the queue -- used as the data of the write
   action */
#define BROKER_BUFFER_COOKIE_SIZE   (WRITE_NUM_WORDS_CNS * sizeof(GT_U32))
/* max number of clients for broker */
#define BROKER_MAX_CLIENTS_CNS  5

/*
 * Typedef: struct BROKER_INFO_STC
 *
 * Description:
 *      the broker related info
 *
 * Fields:
 *      taskId  : the id of the main broker task
 *      taskPriority : the priority of the main broker task
 *      taskFuncPtr  : the function that implement that broker main task
 *      poolId : the pool for buffers use in the 'Q'
 *      queueId : the 'Q' of the broker
 *
 * Comments:
 */
typedef struct{
    GT_TASK_HANDLE  taskId;
    GT_TASK_PRIORITY_ENT taskPriority;
    TASK_PTR_FUN    taskFuncPtr;
    SMINI_BUF_POOL_ID   poolId;
    SQUE_QUEUE_ID       queueId;

}BROKER_INFO_STC;

/* hold the broker info */
static BROKER_INFO_STC brokerInfo;

TASK_DECLARE_MAC(brokerMainTask);

/*
 * Typedef: struct BROKER_BUF_READ_REG_INFO_STC
 *
 * Description:
 *      the info of the buffer when the message is 'Read register'
 *
 * Fields:
 *      read  : main info on the 'Read register'
 *      wakeUpSem : the semaphore on which the one that put in the Q will wait
 *                  for signal , and the signalling will come from the main task
 *                  that process the buffers from the 'Q' and after getting the
 *                  'Read' info from the device .
 *      dataPtr : pointer where to put the 'Read' value(s)
 *
 * Comments:
 */
typedef struct{
    READ_MSG   read; /* what to read */
    GT_SEM     wakeUpSem;/* signal this when dataPtr got the read values */
    void*      dataPtr;/* where to read it to */
}BROKER_BUF_READ_REG_INFO_STC;


/*
 * Typedef: union BROKER_BUF_INFO_UNT
 *
 * Description:
 *      union of the types of info in the buffer
 *
 * Fields:
 *      read  : info on the 'Read register'
 *      write  : info on the 'Write register'
 *
 * Comments:
 */
typedef union{
    BROKER_BUF_READ_REG_INFO_STC    read;
    WRITE_MSG                       write;
}BROKER_BUF_INFO_UNT;

/*
 * Typedef: struct BROKER_BUF_INFO_STC
 *
 * Description:
 *      full info in the buffer
 *
 * Fields:
 *      msgType  : message type
 *      info  : specific info according to message type
 *
 * Comments:
 */
typedef struct{
    GT_U32  msgType;/*DISTRIBUTED_MESSAGE_TYPE_ENT*/
    BROKER_BUF_INFO_UNT info;
}BROKER_BUF_INFO_STC;

/*
 * Typedef: struct BROKER_DMA_MODE_ENT
 *
 * Description:
 *      the broker's mode for handling read/write DMA messages from device
 *
 * Fields:
 *      BROKER_DMA_MODE_SOCKET_E  : send to application via asynch socket
 *      BROKER_DMA_MODE_SHARED_MEMORY_E : use shared memory to do directly read/write
 *
 * Comments:
 */
typedef enum{
    BROKER_DMA_MODE_SOCKET_E = BROKER_DMA_MODE_SOCKET,
    BROKER_DMA_MODE_SHARED_MEMORY_E = BROKER_DMA_MODE_SHARED_MEMORY
}BROKER_DMA_MODE_ENT;

/*
 * Typedef: struct BROKER_DMA_INFO_STC
 *
 * Description:
 *      info about the DMA that broker hold
 *
 * Fields:
 *      mode - the dma mode of working : socket/shared memory
 *      registered - any application registered the DMA
 *      addressStart - start address of the DMA
 *      addressEnd - end address of the DMA
 *      key - key of the shared memory
 *      memoryId - ID of the shared memory
 *      memoryPtr - pointer to the shared memory
 * Comments:
 */
typedef struct{
    BROKER_DMA_MODE_ENT         mode;
    GT_BOOL                     registered;
    GT_U32                      addressStart;
    GT_U32                      addressEnd;
    GT_SH_MEM_KEY               key;
    GT_SH_MEM_ID                memoryId;
    void*                       memoryPtr;

}BROKER_DMA_INFO_STC;

/*info about the DMA that broker hold*/
static BROKER_DMA_INFO_STC brokerDmaInfo;

/*
 * Typedef: struct BROKER_INTERRUPT_SET_MODE_ENT
 *
 * Description:
 *      the broker's mode for handling 'Set interrupt' messages from device
 *
 * Fields:
 *      BROKER_INTERRUPT_SET_MODE_SOCKET_E  : send to application via asynch socket
 *      BROKER_INTERRUPT_SET_MODE_SIGNAL_E : signal the process of the application
 *
 * Comments:
 */
typedef enum{
    BROKER_INTERRUPT_SET_MODE_SOCKET_E,
    BROKER_INTERRUPT_SET_MODE_SIGNAL_E
}BROKER_INTERRUPT_SET_MODE_ENT;

/*
 * Typedef: struct BROKER_INTERRUPT_INFO_STC
 *
 * Description:
 *      info about the interrupt that broker hold
 *
 * Fields:
 *      registered - the application registered the interrupt
 *      interruptLine - the interrupt line
 *      signalId - the ID of the signal when signaling the process of the application
 *      maskMode      - broker interrupt mask mode , one of BROKER_INTERRUPT_MASK_MODE
 *      masked        - indicate that interrupt is masked or unmasked
 * Comments:
 */
typedef struct{
    GT_BOOL                     registered;
    GT_U32                      interruptLine;
    GT_U32                      signalId;
    BROKER_INTERRUPT_MASK_MODE  maskMode;
    GT_BOOL                     masked;
}BROKER_INTERRUPT_INFO_STC;

/* max number of interrupt lines */
#define MAX_INTERRUPT_LINE_CNS 2

/*
 * Typedef: struct BROKER_CONNECTION_CLIENT_COOKIE_STC
 *
 * Description:
 *      info hidden in the cookie of the connection
 *
 * Fields:
 *      getInfoWaitSemaphore  : semaphore for signalling the end of 'Read register'
 *                          operation
 *      protectMutex : Mutex to protect changing of info inside the structure
 *                     from different tasks
 *      applicationProcessId : the process ID of the application on the other
 *                          side of the connection
 *      interruptMode - the interrupt mode of working : socket/signaling
 *      interruptInfoArray : array info about interrupts handling
 *      connectionPtr : pointer to the connection
 * Comments:
 */
typedef struct{
    GT_MUTEX        protectMutex;
    GT_SEM          getInfoWaitSemaphore;
    GT_PROCESS_ID   applicationProcessId;
    BROKER_INTERRUPT_SET_MODE_ENT    interruptMode;
    BROKER_INTERRUPT_INFO_STC   interruptInfoArray[MAX_INTERRUPT_LINE_CNS];
    CONNECTION_INFO_STC *connectionPtr;
}BROKER_CONNECTION_CLIENT_COOKIE_STC;

/* take protection mutex */
#define BROKER_MUTEX_LOCK(appIndex)  \
    SIM_OS_MAC(simOsMutexLock)(brokerClientCookiesArray[appIndex]->protectMutex)

/* signal (release) protection mutex */
#define BROKER_MUTEX_UNLOCK(appIndex)  \
    SIM_OS_MAC(simOsMutexUnlock)(brokerClientCookiesArray[appIndex]->protectMutex)

/* max index used in array of brokerClientCookiesArray */
static GT_U32   brokerNumClients = 0;
/* array of cookies (one per client) that hold info about the client and
   should be connected to the connectionPtr->cookie */
static BROKER_CONNECTION_CLIENT_COOKIE_STC *brokerClientCookiesArray[BROKER_MAX_CLIENTS_CNS]={NULL};

/* array that broker hold to map between device to interrupt line.
    index in the array is : deviceId
    value in the array is : interrupt line (for the device)
*/
static GT_U32 brokerMapDeviceToInterruptLine[MAX_DEVICES_ON_BOARD_CNS] = {0};

/*******************************************************************************
* brokerInterruptSet
*
* DESCRIPTION:
*       function Received request for setting the interrupt , and will set the
*       interrupt .(will not send reply)
*
*       APPLICABLE SIDE : on the main task of broker --> got the request for
*                         setting the interrupt from the Asic side
*                         (will not send reply)
*
* INPUTS:
*       deviceId - the deviceId that set the interrupt
* OUTPUTS:
*       none
*
* RETURNS:
*       none
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static void  brokerInterruptSet
(
    IN  GT_U32        deviceId
)
{
    GT_U32  ii,jj;
    GT_U32  interruptLine;

    for(ii = 0 ; ii < brokerNumClients ; ii++)
    {
        if(deviceId >= MAX_DEVICES_ON_BOARD_CNS)
        {
            skernelFatalError(" brokerInterruptSet : deviceId[%d] out of range[%d]",
                deviceId,MAX_DEVICES_ON_BOARD_CNS);
        }

        interruptLine = brokerMapDeviceToInterruptLine[deviceId];

        for(jj = 0; jj < MAX_INTERRUPT_LINE_CNS; jj ++)
        {
            if(brokerClientCookiesArray[ii]->interruptInfoArray[jj].registered == GT_TRUE &&
               ((brokerClientCookiesArray[ii]->interruptInfoArray[jj].interruptLine == interruptLine) ||
                (brokerClientCookiesArray[ii]->interruptInfoArray[jj].interruptLine == ALL_INTERRUPT_LINES_ID)))
            {
                BROKER_MUTEX_LOCK(ii);
                if(brokerClientCookiesArray[ii]->interruptInfoArray[jj].masked == GT_TRUE)
                {
                    BROKER_MUTEX_UNLOCK(ii);
                    break;
                }

                if(brokerInterruptMode == BROKER_USE_SOCKET)
                {
                    /* ask to send it to the next station , to specific connection */
                    simDistributedInterruptSetExt(deviceId,brokerClientCookiesArray[ii]->connectionPtr);
                }
                else
                {
                    /* signal the application with the specific signal for this interrupt line */
                    SIM_OS_MAC(simOsProcessNotify)(brokerClientCookiesArray[ii]->applicationProcessId,
                                brokerClientCookiesArray[ii]->interruptInfoArray[jj].signalId);
                }

                if(brokerClientCookiesArray[ii]->interruptInfoArray[jj].maskMode ==
                   INTERRUPT_MODE_BROKER_AUTOMATICALLY_MASK_INTERRUPT_LINE)
                {
                    brokerClientCookiesArray[ii]->interruptInfoArray[jj].masked = GT_TRUE;
                }

                BROKER_MUTEX_UNLOCK(ii);
                break;
            }
        }/* end loop on jj */
    }/* end loop on ii */

}

/*******************************************************************************
* brokerRegisterRead
*
* DESCRIPTION:
*       function Received request for read register values , and will read
*       registers and will sent reply with the data
*
*       APPLICABLE SIDE : on client dispatcher tasks --> got the request for
*                         read from the application side , will send the info to
*                         the 'Q' , will wait for the data and will return reply
*
* INPUTS:
*       type - type of access (mem reg/pci reg)
*       deviceId - device Id to read from it
*       address - the address to read from
*       memSize - number of registers to read
*       connectionPtr - pointer to the connection info
* OUTPUTS:
*       memPtr - pointer to the read data
*
* RETURNS:
*       none
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static void  brokerRegisterRead
(
    IN READ_WRITE_ACCESS_TYPE type,
    IN GT_U32 deviceId,
    IN GT_U32 address,
    IN GT_U32 memSize,
    OUT GT_U32 * memPtr,
    IN CONNECTION_INFO_STC *connectionPtr
)
{
    BROKER_BUF_INFO_STC     *msgInfoPtr;
    SMINI_BUF_STC           *miniBufPtr = NULL;
    BROKER_CONNECTION_CLIENT_COOKIE_STC *cookiePtr;

    /* get the cookie info from the connection */
    cookiePtr = (BROKER_CONNECTION_CLIENT_COOKIE_STC*)connectionPtr->cookie;

    while(1)
    {
        /* get buffer on which we put the needed data to send to the 'Q' */
        miniBufPtr = sMiniBufAlloc(brokerInfo.poolId);
        if(miniBufPtr)
        {
            break;
        }
        /* wait for buffer */
        SIM_OS_MAC(simOsSleep)(1);
    }

    /* cast the data section to needed structure */
    msgInfoPtr = miniBufPtr->dataPtr;

    /* fill the buffer with the info */
    msgInfoPtr->msgType = (GT_U32)DISTRIBUTED_MESSAGE_TYPE_REGISTER_READ_E;
    msgInfoPtr->info.read.read.deviceId = deviceId;
    msgInfoPtr->info.read.read.accessType = (GT_U32)type;
    msgInfoPtr->info.read.read.readLen = memSize;
    msgInfoPtr->info.read.read.address = address;
    msgInfoPtr->info.read.wakeUpSem = cookiePtr->getInfoWaitSemaphore;/* the semaphore to wake us */
    msgInfoPtr->info.read.dataPtr = memPtr;

    /* send to the 'Q' */
    squeBufPut(brokerInfo.queueId,SIM_CAST_MINI_BUFF(miniBufPtr));

    /* wait for the signal from the task that process the Q , after it got the
       info from the device */
    SIM_OS_MAC(simOsSemWait)(cookiePtr->getInfoWaitSemaphore,SIM_OS_WAIT_FOREVER);

}

/*******************************************************************************
* brokerRegisterWrite
*
* DESCRIPTION:
*       function Received request for write register values , and will write
*       registers .(will not send reply)
*
*       APPLICABLE SIDE : on client dispatcher tasks --> got the request for
*                         read from the application side , will send the info to
*                         the 'Q' (will not wait for reply),(will not send reply)
*
* INPUTS:
*       type - type of access (mem reg/pci reg)
*       deviceId - device Id to write to
*       address - the address to write to
*       memSize - number of registers to write
*       memPtr - pointer to the write data
* OUTPUTS:
*
* RETURNS:
*       none
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static void brokerRegisterWrite
(
    IN READ_WRITE_ACCESS_TYPE type,
    IN GT_U32 deviceId,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_U32 * memPtr
)
{
    BROKER_BUF_INFO_STC     *msgInfoPtr;
    SMINI_BUF_STC           *miniBufPtr=NULL;

    /* get buffer on which we put the needed data to send to the 'Q' */
    while(1)
    {
        /* get buffer on which we put the needed data to send to the 'Q' */
        miniBufPtr = sMiniBufAlloc(brokerInfo.poolId);
        if(miniBufPtr)
        {
            break;
        }
        /* wait for buffer */
        SIM_OS_MAC(simOsSleep)(1);
    }

    /* cast the data section to needed structure */
    msgInfoPtr = miniBufPtr->dataPtr;

    /* fill the buffer with the info */
    msgInfoPtr->msgType = (GT_U32)DISTRIBUTED_MESSAGE_TYPE_REGISTER_WRITE_E;
    msgInfoPtr->info.write.deviceId = deviceId;
    msgInfoPtr->info.write.accessType = (GT_U32)type;
    msgInfoPtr->info.write.writeLen = memSize;
    msgInfoPtr->info.write.address = address;
    msgInfoPtr->info.write.dataPtr = miniBufPtr->cookiePtr;

    if(memSize >= WRITE_NUM_WORDS_CNS)
    {
        skernelFatalError(" brokerRegisterWrite : currently support write of up to [%d] num of words ",
            (GT_U32)WRITE_NUM_WORDS_CNS);
    }

    /* put the write info as 'cookie' info */
    memcpy(miniBufPtr->cookiePtr,memPtr,memSize*sizeof(GT_U32));


    /* send to the 'Q' */
    squeBufPut(brokerInfo.queueId,SIM_CAST_MINI_BUFF(miniBufPtr));
}

/*******************************************************************************
* brokerDmaRead
*
* DESCRIPTION:
*       function Received request for read DMA values , and will read
*       DMA and will sent reply with the data
*
*       APPLICABLE SIDE : on main broker task --> got the request for
*                         read DMA from device(s) side . and will return reply.
*                         support 2 modes:
*                         1. send this request to the application client(s) on
*                            asynch socket
*                         2. do read of 'shared' memory (given by the application(s))
*
*
* INPUTS:
*       deviceId - not used
*       address - DMA address
*       memSize - number of words to read from DMA
*       dataIsWords - not used
* OUTPUTS:
*       memPtr - pointer to the read data
*
* RETURNS:
*       none
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static void brokerDmaRead
(
    IN GT_U32 deviceId,
    IN GT_U32 address,
    IN GT_U32 memSize,
    OUT GT_U32 * memPtr,
    IN GT_U32  dataIsWords
)
{
    GT_U32  memOffset;
    GT_U32  memByteSize;

    switch(brokerDmaInfo.mode)
    {
        case BROKER_DMA_MODE_SOCKET_E:
            /* ask to send it to the application */
            simDistributedDmaRead(deviceId,address,memSize,memPtr,dataIsWords);
            break;
        case BROKER_DMA_MODE_SHARED_MEMORY_E:
            if(brokerDmaInfo.registered == GT_FALSE)
            {
                skernelFatalError(" brokerDmaWrite : DMA was not registered yet");
            }

            memByteSize = (memSize*4);
            if(address < brokerDmaInfo.addressStart ||
               (address + memByteSize) >= brokerDmaInfo.addressEnd)
            {
                skernelFatalError(" brokerDmaRead : access out of range , range is[0x%8.8x] to [0x%8.8x]"\
                        "access to address [0x%8.8x] memSize(bytes) is[0x%8.8x]",
                        brokerDmaInfo.addressStart , brokerDmaInfo.addressEnd,
                        address,memByteSize);
            }

            memOffset =  address - brokerDmaInfo.addressStart;

            /* copy from the memory in the needed offset to the requested pointer */
            memcpy(memPtr,(char*)brokerDmaInfo.memoryPtr + memOffset , memByteSize);

            break;
        default:
            skernelFatalError(" brokerDmaRead : unknown mode[%d]",(GT_U32)brokerDmaInfo.mode);
    }

    return;
}

/*******************************************************************************
* brokerDmaWrite
*
* DESCRIPTION:
*       function Received request for write DMA values , and will write
*       DMA
*
*       APPLICABLE SIDE : on main broker task --> got the request for
*                         write DMA from device(s) side .
*                         support 2 modes:
*                         1. send this request to the application client(s) on
*                            asynch socket
*                         2. do write of 'shared' memory (given by the application(s))
*
*
* INPUTS:
*       deviceId - not used
*       address - DMA address
*       memSize - number of words to write to DMA
*       memPtr - pointer to the data
*       dataIsWords - not used
* OUTPUTS:
*       none
*
* RETURNS:
*       none
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static void brokerDmaWrite
(
    IN GT_U32 deviceId,
    IN GT_U32 address,
    IN GT_U32 memSize,
    INOUT GT_U32 * memPtr,
    IN GT_U32  dataIsWords
)
{
    GT_U32  memOffset;
    GT_U32  memByteSize;

    switch(brokerDmaInfo.mode)
    {
        case BROKER_DMA_MODE_SOCKET_E:
            /* ask to send it to the application */
            simDistributedDmaWrite(deviceId,address,memSize,memPtr,dataIsWords);
            break;
        case BROKER_DMA_MODE_SHARED_MEMORY_E:
            if(brokerDmaInfo.registered == GT_FALSE)
            {
                skernelFatalError(" brokerDmaWrite : DMA was not registered yet");
            }

            memByteSize = (memSize*4);
            if(address < brokerDmaInfo.addressStart ||
               (address + memByteSize) >= brokerDmaInfo.addressEnd)
            {
                skernelFatalError(" brokerDmaWrite : access out of range , range is[0x%8.8x] to [0x%8.8x]"\
                        "access to address [0x%8.8x] memSize(bytes) is[0x%8.8x]",
                        brokerDmaInfo.addressStart , brokerDmaInfo.addressEnd,
                        address,memByteSize);
            }

            memOffset =  address - brokerDmaInfo.addressStart;

            /* copy from the memory in the needed offset to the requested pointer */
            memcpy((char*)brokerDmaInfo.memoryPtr + memOffset, memPtr, memByteSize);

            break;
        default:
            skernelFatalError(" brokerDmaWrite : unknown mode[%d]",(GT_U32)brokerDmaInfo.mode);
    }

    return;
}

/*******************************************************************************
* brokerDeviceInit
*
* DESCRIPTION:
*       function Received message to initialize a device of the Asic on SCIB
*       of application .(will not send reply)
*
*       APPLICABLE SIDE : on main broker task --> got the request for init device
*                         from device(s) side .
*                         support 2 modes:
*                         1. send this request to the application client(s) on
*                            asynch socket
*                         2. discard message
*
* INPUTS:
*       deviceInfoPtr - pointer to info about the device that is 'ready'
*
* OUTPUTS:
*       none
*
* RETURNS:
*       none
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static void brokerDeviceInit
(
    IN SIM_DISTRIBUTED_INIT_DEVICE_STC *deviceInfoPtr
)
{
    GT_U32  ii;
    CONNECTION_INFO_STC *connectionPtr;

    /*we need to save to mapping between the device and the interrupt line*/

    if(deviceInfoPtr->deviceId >= MAX_DEVICES_ON_BOARD_CNS)
    {
        skernelFatalError(" brokerDeviceInit : deviceId[%d] >= max supported[%d] \n",
            deviceInfoPtr->deviceId , MAX_DEVICES_ON_BOARD_CNS);
    }

    brokerMapDeviceToInterruptLine[deviceInfoPtr->deviceId] = deviceInfoPtr->interruptLine;


    /* loop on all connections to applications and send the info to them */
    /* NOTE : we send it on the 'synchronic' socket !!! */

    for(ii = 0 ; ii < brokerNumClients; ii++)
    {
        if(brokerClientCookiesArray[ii] == NULL)
        {
            continue;
        }

        connectionPtr = brokerClientCookiesArray[ii]->connectionPtr;
        /* ask to send it to the next station */
        simDistributedRemoteInitExt(1,deviceInfoPtr,connectionPtr);
    }
}

/*******************************************************************************
* brokerConnectionInitCookie
*
* DESCRIPTION:
*       function set the cookie in the connection info
*
* INPUTS:
*       connectionPtr - pointer to the connection info
*
* OUTPUTS:
*       none
*
* RETURNS:
*       none
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static void brokerConnectionInitCookie(
    IN CONNECTION_INFO_STC *connectionPtr
)
{
    BROKER_CONNECTION_CLIENT_COOKIE_STC *cookiePtr;
    GT_U32  ii;

    if(connectionPtr->socketServerSide == GT_TRUE)
    {
        /* we need to bind the connection with info about the client task that
           wait for 'read register' from the main broker , to be able to send it
           bask to waiting application */

        cookiePtr = malloc(sizeof(BROKER_CONNECTION_CLIENT_COOKIE_STC));
        if(cookiePtr == NULL)
        {
            skernelFatalError(" brokerConnectionInitCookie : fail to alloc memory");
        }

        memset(cookiePtr,0,sizeof(BROKER_CONNECTION_CLIENT_COOKIE_STC));

        if(connectionPtr->connectionType == CONNECTION_TYPE_SYNCH_E)
        {
            cookiePtr->getInfoWaitSemaphore = SIM_OS_MAC(simOsSemCreate)(0,1);
        }

        cookiePtr->protectMutex = SIM_OS_MAC(simOsMutexCreate)();

        cookiePtr->applicationProcessId = 0;
        cookiePtr->interruptMode = BROKER_INTERRUPT_SET_MODE_SOCKET_E;
        for(ii = 0 ; ii < MAX_INTERRUPT_LINE_CNS ; ii++)
        {
            cookiePtr->interruptInfoArray[ii].registered = GT_FALSE;
            cookiePtr->interruptInfoArray[ii].interruptLine = 0;
            cookiePtr->interruptInfoArray[ii].signalId = 0;
            cookiePtr->interruptInfoArray[ii].maskMode = INTERRUPT_MODE_BROKER_USE_MASK_INTERRUPT_LINE_MSG;
            cookiePtr->interruptInfoArray[ii].masked = GT_FALSE;
        }

        connectionPtr->cookie = cookiePtr;

        /* also bind the connection to the cookie */
        cookiePtr->connectionPtr = connectionPtr;

        /* also set pointer to the global array */
        brokerClientCookiesArray[brokerNumClients++] = cookiePtr;
    }
}

/*******************************************************************************
* brokerInitPart1
*
* DESCRIPTION:
*       function to do broker initializations
*       function bind CB functions , create the 'Broker main task' ,
*       initialize queue,buffers pool functionality for the 'Q'
*
* INPUTS:
*       none.
*
* OUTPUTS:
*       none
*
* RETURNS:
*       none
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static void brokerInitPart1(void)
{
    connectionInitCookie        = brokerConnectionInitCookie;

    msgInterruptSet             = brokerInterruptSet;
    msgDmaRead                  = brokerDmaRead;
    msgDmaWrite                 = brokerDmaWrite;
    msgRegisterRead             = brokerRegisterRead;
    msgRegisterWrite            = brokerRegisterWrite;
    msgDeviceInit               = brokerDeviceInit;

    /* Init Squeue library. */
    squeInit();

    /* create SQueue */
    brokerInfo.queueId  = squeCreate();

    /* init the mini buffer management */
    sMiniBufInit(1);/* currently need single pool for the use of the broker */

    /* create the single pool , with buffers in the needed side */
    brokerInfo.poolId = sMiniBufPoolCreate(BROKER_QUEUE_SIZE,
                                           BROKER_BUFFERS_SIZE,
                                           BROKER_BUFFER_COOKIE_SIZE);

    brokerInfo.taskPriority = GT_TASK_PRIORITY_ABOVE_NORMAL; /* same as other dispatchers tasks */
    brokerInfo.taskFuncPtr = brokerMainTask;

    /* create the task that will handle the messages from the queue */
    brokerInfo.taskId = SIM_OS_MAC(simOsTaskCreate)(brokerInfo.taskPriority,
                                            brokerInfo.taskFuncPtr,
                                            NULL);

    if(brokerInfo.taskId == NULL)
    {
        skernelFatalError(" brokerInitPart1 : fail to create task");
    }

}


/*******************************************************************************
* brokerMainTask
*
* DESCRIPTION:
*       main broker Task that wait for messages on it's Queue and send those
*       messages to the socket towards the devices.
*       this task runs only in the broker process.
*
* INPUTS:
*       dummyPtr - pointer to extra info specific for this task --
*                  NOT used in this task
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
static unsigned __TASKCONV brokerMainTask(IN void* dummyPtr)
{
    SMINI_BUF_STC* bufferPtr;
    BROKER_BUF_INFO_STC    *msgInfoPtr;
    SCIB_MEMORY_ACCESS_TYPE accessType;

    while(1)
    {
        /* get buffer */
        bufferPtr = SIM_CAST_MINI_BUFF(squeBufGet(brokerInfo.queueId));

        msgInfoPtr = bufferPtr->dataPtr;

        switch(msgInfoPtr->msgType)/*DISTRIBUTED_MESSAGE_TYPE_ENT*/
        {
            case DISTRIBUTED_MESSAGE_TYPE_REGISTER_READ_E:
                accessType = (msgInfoPtr->info.read.read.accessType == REGISTER_MEMORY_ACCESS) ?
                                SCIB_MEMORY_READ_E :
                                SCIB_MEMORY_READ_PCI_E;

                /* ask to send it to the next station */
                simDistributedRegisterRead(accessType,
                                           msgInfoPtr->info.read.read.deviceId,
                                           msgInfoPtr->info.read.read.address,
                                           msgInfoPtr->info.read.read.readLen,
                                           msgInfoPtr->info.read.dataPtr);

                /* now we can wake the waiting task */
                SIM_OS_MAC(simOsSemSignal)(msgInfoPtr->info.read.wakeUpSem);

                break;

            case DISTRIBUTED_MESSAGE_TYPE_REGISTER_WRITE_E:
                accessType = (msgInfoPtr->info.write.accessType == REGISTER_MEMORY_ACCESS) ?
                                SCIB_MEMORY_WRITE_E :
                                SCIB_MEMORY_WRITE_PCI_E;

                /* ask to send it to the next station */
                simDistributedRegisterWrite(accessType,
                                            msgInfoPtr->info.write.deviceId,
                                            msgInfoPtr->info.write.address,
                                            msgInfoPtr->info.write.writeLen,
                                            (void*)msgInfoPtr->info.write.dataPtr);/* use this field as actual pointer */
                break;

            default:
                skernelFatalError(" brokerMainTask : msg not supported msgType[%d]",(GT_U32)msgInfoPtr->msgType);
        }


        /* free buffer */
        sMiniBufFree(brokerInfo.poolId, bufferPtr);
    }

#ifdef __GNUC__
    /* to avoid warnings in gcc */
    return 0;
#endif
}


/*******************************************************************************
* brokerMessageAppPid
*
* DESCRIPTION:
*       function Received message setting the application PID of client
*
*       APPLICABLE SIDE : only on broker side --> got PID of application
*
* INPUTS:
*       connectionPtr - pointer to connection info
*       offsetInBuf - the offset in the read buffer
*       messageHeaderPtr - pointer to the parsed header info
* OUTPUTS:
*       none
*
* RETURNS:
*       none
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static void brokerMessageAppPid
(
    IN CONNECTION_INFO_STC *connectionPtr,
    IN GT_U32               offsetInBuf,
    IN MSG_HDR              *messageHeaderPtr
)
{
    BROKER_CONNECTION_CLIENT_COOKIE_STC *cookiePtr;
    GT_U32  ii;
    APP_PID_MSG     msgAppPid;
    GT_U32  length = LEN_NO_HEADER_WORDS(APP_PID_MSG);
    GT_U8   *buffPtr = connectionPtr->readBufPtr + offsetInBuf;
    GT_U32  *u32Ptr = (GT_U32*)((void*)&msgAppPid);

    for(ii = 0 ; ii < length ; ii++)
    {
        /* copy data from the buffer */
        u32Ptr[ii+HEADER_SIZE_WORDS] = SIM_HTONL(&buffPtr[ii*4]);
    }

    /* get the cookie info from the connection */
    cookiePtr = (BROKER_CONNECTION_CLIENT_COOKIE_STC*)connectionPtr->cookie;

    if(msgAppPid.processId == 0)
    {
        skernelFatalError(" brokerMessageAppPid : processId not valid[%d]",(GT_U32)msgAppPid.processId);
    }
    else if(cookiePtr->applicationProcessId != 0 &&
            cookiePtr->applicationProcessId != msgAppPid.processId)
    {
        /* already initialized ?! */
        skernelFatalError(" brokerMessageAppPid : processId already initialized");
    }

    cookiePtr->applicationProcessId = msgAppPid.processId;
}

/*******************************************************************************
* brokerMessageRegisterDma
*
* DESCRIPTION:
*       function Received message registering DMA parameters
*
*       APPLICABLE SIDE : only on broker side --> got DMA info from application
*
* INPUTS:
*       connectionPtr - pointer to connection info
*       offsetInBuf - the offset in the read buffer
*       messageHeaderPtr - pointer to the parsed header info
* OUTPUTS:
*       none
*
* RETURNS:
*       none
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static void brokerMessageRegisterDma
(
    IN CONNECTION_INFO_STC *connectionPtr,
    IN GT_U32               offsetInBuf,
    IN MSG_HDR              *messageHeaderPtr
)
{
    GT_STATUS   rc;
    GT_U32  ii;
    REGISTER_DMA_MSG     msgRegisterDma;
    GT_U32  length = LEN_NO_HEADER_WORDS(REGISTER_DMA_MSG);
    GT_U8   *buffPtr = connectionPtr->readBufPtr + offsetInBuf;
    GT_U32  *u32Ptr = (GT_U32*)((void*)&msgRegisterDma);

    for(ii = 0 ; ii < length ; ii++)
    {
        /* copy data from the buffer */
        u32Ptr[ii+HEADER_SIZE_WORDS] = SIM_HTONL(&buffPtr[ii*4]);
    }

    if(brokerDmaInfo.registered == GT_TRUE)
    {
        /* already initialized ?! */
        skernelFatalError(" brokerMessageRegisterDma : DMA registration already done");
    }

    brokerDmaInfo.mode = msgRegisterDma.dmaMode;
    brokerDmaInfo.registered = GT_TRUE;

    switch(brokerDmaInfo.mode)
    {
        case BROKER_DMA_MODE_SHARED_MEMORY_E:
            brokerDmaInfo.addressStart = msgRegisterDma.startAddress;
            brokerDmaInfo.addressEnd = msgRegisterDma.startAddress + msgRegisterDma.size;
            brokerDmaInfo.key = msgRegisterDma.key;

            /* get hold of the shared memory , according to a key */
            rc = SIM_OS_MAC(simOsSharedMemGet)(msgRegisterDma.key,
                                               msgRegisterDma.size,
                                               0,
                                               &brokerDmaInfo.memoryId);
            if(rc != GT_OK)
            {
                skernelFatalError(" brokerMessageRegisterDma : failed to get shared memory , key[%d]",
                    (GT_U32)brokerDmaInfo.key);
            }

            /* Now we attach (map) the shared memory to our process memory */
            brokerDmaInfo.memoryPtr = SIM_OS_MAC(simOsSharedMemAttach)(brokerDmaInfo.memoryId,
                    NULL ,/* NULL means we attach it to no specific memory , we let OS to decide */
                    0);/*no specific flags*/

            if(brokerDmaInfo.memoryPtr == NULL)
            {
                skernelFatalError(" brokerMessageRegisterDma : failed to attach to shared memory , memoryId[%d]",
                    (GT_U32)brokerDmaInfo.memoryId);
            }

            break;
        case BROKER_DMA_MODE_SOCKET_E:
            break;
        default:
            skernelFatalError(" brokerMessageRegisterDma : unknown mode[%d]\n",(GT_U32)brokerDmaInfo.mode);
            break;
    }
}

/*******************************************************************************
* brokerMessageRegisterInterrupt
*
* DESCRIPTION:
*       function Received message registering the application for interrupts
*
*       APPLICABLE SIDE : only on broker side --> got from application
*
* INPUTS:
*       connectionPtr - pointer to connection info
*       offsetInBuf - the offset in the read buffer
*       messageHeaderPtr - pointer to the parsed header info
* OUTPUTS:
*       none
*
* RETURNS:
*       none
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static void brokerMessageRegisterInterrupt
(
    IN CONNECTION_INFO_STC *connectionPtr,
    IN GT_U32               offsetInBuf,
    IN MSG_HDR              *messageHeaderPtr
)
{
    BROKER_CONNECTION_CLIENT_COOKIE_STC *cookiePtr;
    GT_U32  ii;
    REGISTER_INTERRUPT_MSG     msgRegisterInterrupt;
    GT_U32  length = LEN_NO_HEADER_WORDS(REGISTER_INTERRUPT_MSG);
    GT_U8   *buffPtr = connectionPtr->readBufPtr + offsetInBuf;
    GT_U32  *u32Ptr = (GT_U32*)((void*)&msgRegisterInterrupt);

    for(ii = 0 ; ii < length ; ii++)
    {
        /* copy data from the buffer */
        u32Ptr[ii+HEADER_SIZE_WORDS] = SIM_HTONL(&buffPtr[ii*4]);
    }

    /* get the cookie info from the connection */
    cookiePtr = (BROKER_CONNECTION_CLIENT_COOKIE_STC*)connectionPtr->cookie;

    cookiePtr->interruptMode = (brokerInterruptMode == BROKER_USE_SOCKET) ?
                BROKER_INTERRUPT_SET_MODE_SOCKET_E :
                BROKER_INTERRUPT_SET_MODE_SIGNAL_E;

    for(ii = 0 ; ii < MAX_INTERRUPT_LINE_CNS ; ii++)
    {
        if(cookiePtr->interruptInfoArray[ii].registered == GT_FALSE)
        {
            /* free entry */
            break;
        }

        if(cookiePtr->interruptInfoArray[ii].interruptLine == msgRegisterInterrupt.interruptLineId)
        {
            skernelFatalError(" brokerMessageRegisterInterrupt : interrupt line[%d] already registered",
                msgRegisterInterrupt.interruptLineId);
        }
    }

    if(ii == MAX_INTERRUPT_LINE_CNS)
    {
        /* full - need to set bigger value to MAX_INTERRUPT_LINE_CNS */
        skernelFatalError(" brokerMessageRegisterInterrupt : array of interrupt lines is full");
    }

    cookiePtr->interruptInfoArray[ii].registered = GT_TRUE;
    if(cookiePtr->interruptMode == BROKER_INTERRUPT_SET_MODE_SIGNAL_E)
    {
        cookiePtr->interruptInfoArray[ii].signalId = msgRegisterInterrupt.signalId;
    }

    cookiePtr->interruptInfoArray[ii].interruptLine = msgRegisterInterrupt.interruptLineId;
    cookiePtr->interruptInfoArray[ii].maskMode = msgRegisterInterrupt.maskMode;
    cookiePtr->interruptInfoArray[ii].masked = GT_FALSE;

}

/*******************************************************************************
* brokerMessageUnmaskInterrupt
*
* DESCRIPTION:
*       function Received message for mask/unmask the interrupt , allowing
*       application to be/not signaled
*
*       APPLICABLE SIDE : only on broker side --> got from application
*
* INPUTS:
*       connectionPtr - pointer to connection info
*       offsetInBuf - the offset in the read buffer
*       messageHeaderPtr - pointer to the parsed header info
*       mask - mask/unmask
* OUTPUTS:
*       none
*
* RETURNS:
*       none
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static void brokerMessageMaskUnmaskInterrupt
(
    IN CONNECTION_INFO_STC *connectionPtr,
    IN GT_U32               offsetInBuf,
    IN MSG_HDR              *messageHeaderPtr,
    IN GT_BOOL              mask
)
{
    BROKER_CONNECTION_CLIENT_COOKIE_STC *cookiePtr;
    GT_U32  ii;
    UNMASK_INTERRUPT_MSG     msgUnmaskInterrupt;
    GT_U32  length = LEN_NO_HEADER_WORDS(UNMASK_INTERRUPT_MSG);
    GT_U8   *buffPtr = connectionPtr->readBufPtr + offsetInBuf;
    GT_U32  *u32Ptr = (GT_U32*)((void*)&msgUnmaskInterrupt);

    for(ii = 0 ; ii < length ; ii++)
    {
        /* copy data from the buffer */
        u32Ptr[ii+HEADER_SIZE_WORDS] = SIM_HTONL(&buffPtr[ii*4]);
    }

    /* get the cookie info from the connection */
    cookiePtr = (BROKER_CONNECTION_CLIENT_COOKIE_STC*)connectionPtr->cookie;

    for(ii = 0 ; ii < MAX_INTERRUPT_LINE_CNS ; ii++)
    {
        if(cookiePtr->interruptInfoArray[ii].registered == GT_FALSE)
        {
            continue;
        }

        if((msgUnmaskInterrupt.interruptLineId == ALL_INTERRUPT_LINES_ID) ||
           (msgUnmaskInterrupt.interruptLineId == cookiePtr->interruptInfoArray[ii].interruptLine))
        {
            BROKER_MUTEX_LOCK(ii);
            cookiePtr->interruptInfoArray[ii].masked = mask;
            BROKER_MUTEX_UNLOCK(ii);
        }
    }
}

/*******************************************************************************
* brokerMessageUnmaskInterrupt
*
* DESCRIPTION:
*       function Received message for unmask the interrupt , allowing
*       application to be signaled
*
*       APPLICABLE SIDE : only on broker side --> got from application
*
* INPUTS:
*       connectionPtr - pointer to connection info
*       offsetInBuf - the offset in the read buffer
*       messageHeaderPtr - pointer to the parsed header info
* OUTPUTS:
*       none
*
* RETURNS:
*       none
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static void brokerMessageUnmaskInterrupt
(
    IN CONNECTION_INFO_STC *connectionPtr,
    IN GT_U32               offsetInBuf,
    IN MSG_HDR              *messageHeaderPtr
)
{
    brokerMessageMaskUnmaskInterrupt(connectionPtr,offsetInBuf,messageHeaderPtr,GT_FALSE);
}

/*******************************************************************************
* brokerMessageMaskInterrupt
*
* DESCRIPTION:
*       function Received message for mask the interrupt , NOT allowing
*       application to be signaled
*
*       APPLICABLE SIDE : only on broker side --> got from application
*
* INPUTS:
*       connectionPtr - pointer to connection info
*       offsetInBuf - the offset in the read buffer
*       messageHeaderPtr - pointer to the parsed header info
* OUTPUTS:
*       none
*
* RETURNS:
*       none
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static void brokerMessageMaskInterrupt
(
    IN CONNECTION_INFO_STC *connectionPtr,
    IN GT_U32               offsetInBuf,
    IN MSG_HDR              *messageHeaderPtr
)
{
    brokerMessageMaskUnmaskInterrupt(connectionPtr,offsetInBuf,messageHeaderPtr,GT_TRUE);
}

/*******************************************************************************
* brokerMessageExtended
*
* DESCRIPTION:
*       function Received message for the broker to process
*
*       APPLICABLE SIDE : only on broker side --> got the message from the application
*
* INPUTS:
*       messageType - message type to process
*       connectionPtr - pointer to connection info
*       offsetInBuf - the offset in the read buffer
*       messageHeaderPtr - pointer to the parsed header info
* OUTPUTS:
*       none
*
* RETURNS:
*       none
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static void brokerMessageExtended
(
    IN DISTRIBUTED_MESSAGE_TYPE_ENT messageType,
    IN CONNECTION_INFO_STC *connectionPtr,
    IN GT_U32               offsetInBuf,
    IN MSG_HDR              *messageHeaderPtr
)
{
    switch(messageType)
    {
        case DISTRIBUTED_MESSAGE_TYPE_APP_PID_E:
            brokerMessageAppPid(connectionPtr,offsetInBuf,messageHeaderPtr);
            break;
        case DISTRIBUTED_MESSAGE_TYPE_REGISTER_DMA_E:
            brokerMessageRegisterDma(connectionPtr,offsetInBuf,messageHeaderPtr);
            break;
        case DISTRIBUTED_MESSAGE_TYPE_REGISTER_INTERRUPT_E:
            brokerMessageRegisterInterrupt(connectionPtr,offsetInBuf,messageHeaderPtr);
            break;
        case DISTRIBUTED_MESSAGE_TYPE_UNMASK_INTERRUPT_E:
            brokerMessageUnmaskInterrupt(connectionPtr,offsetInBuf,messageHeaderPtr);
            break;
        case DISTRIBUTED_MESSAGE_TYPE_MASK_INTERRUPT_E:
            brokerMessageMaskInterrupt(connectionPtr,offsetInBuf,messageHeaderPtr);
            break;
        default:
            skernelFatalError(" brokerMessageExtended : msg not supported msgType[%d]",(GT_U32)messageType);
            break;
    }

    return;
}


/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/**********************   BUS specifics ***************************************/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/

/*******************************************************************************
* busInterruptSet
*
* DESCRIPTION:
*       function Received request for setting the interrupt , and will set the
*       interrupt .(will not send reply)
*
*       APPLICABLE SIDE : on the slave dispatcher task of bus --> got the request
*                         for setting the interrupt from the Asic side , and
*                         will send it to the application side
*
* INPUTS:
*       deviceId - the deviceId that set the interrupt
*
* OUTPUTS:
*       none
*
* RETURNS:
*       none
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static void  busInterruptSet
(
    IN  GT_U32        deviceId
)
{
    /* ask to send it to the next station */
    simDistributedInterruptSet(deviceId);
}

/*******************************************************************************
* busRegisterRead
*
* DESCRIPTION:
*       function Received request for read register values , and will read
*       registers and will sent reply with the data
*
*       APPLICABLE SIDE : on server dispatcher tasks --> got the request for
*                         read from the application side , will send the info to
*                         the device(s) side , will wait for the data and will
*                         return reply
*
* INPUTS:
*       type - type of access (mem reg/pci reg)
*       deviceId - device Id to read from it
*       address - the address to read from
*       memSize - number of registers to read
*       connectionPtr - pointer to the connection info
* OUTPUTS:
*       memPtr - pointer to the read data
*
* RETURNS:
*       none
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static void  busRegisterRead
(
    IN READ_WRITE_ACCESS_TYPE type,
    IN GT_U32 deviceId,
    IN GT_U32 address,
    IN GT_U32 memSize,
    OUT GT_U32 * memPtr,
    IN CONNECTION_INFO_STC *connectionPtr
)
{
    SCIB_MEMORY_ACCESS_TYPE accessType = (type == REGISTER_MEMORY_ACCESS) ?
                                    SCIB_MEMORY_READ_E : SCIB_MEMORY_READ_PCI_E;

    connectionPtr = connectionPtr;/* not needed parameter */

    /* ask to send it to the next station */
    simDistributedRegisterRead(accessType,deviceId,address,memSize,memPtr);
}

/*******************************************************************************
* busRegisterWrite
*
* DESCRIPTION:
*       function Received request for write register values , and will write
*       registers .(will not send reply)
*
*       APPLICABLE SIDE : on client dispatcher tasks --> got the request for
*                         read from the application side , will send the info to
*                         the device(s) side ,(will not wait for reply),(will not send reply)
*
* INPUTS:
*       type - type of access (mem reg/pci reg)
*       deviceId - device Id to write to
*       address - the address to write to
*       memSize - number of registers to write
*       memPtr - pointer to the write data
* OUTPUTS:
*
* RETURNS:
*       none
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static void busRegisterWrite
(
    IN READ_WRITE_ACCESS_TYPE type,
    IN GT_U32 deviceId,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_U32 * memPtr
)
{
    SCIB_MEMORY_ACCESS_TYPE accessType = (type == REGISTER_MEMORY_ACCESS) ?
                                    SCIB_MEMORY_WRITE_E : SCIB_MEMORY_WRITE_PCI_E;
    /* ask to send it to the next station */
    simDistributedRegisterWrite(accessType,deviceId,address,memSize,memPtr);
}

/*******************************************************************************
* busDmaRead
*
* DESCRIPTION:
*       function Received request for read DMA values , and will read
*       DMA and will sent reply with the data
*
*       APPLICABLE SIDE : on server dispatcher task --> got the request for
*                         read from the device(s) side , will send the info to
*                         the application side ,(will  wait for reply),(will send reply)
*
* INPUTS:
*       deviceId - not used
*       address - DMA address
*       memSize - number of words to read from DMA
*       dataIsWords - not used
* OUTPUTS:
*       memPtr - pointer to the read data
*
* RETURNS:
*       none
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static void busDmaRead
(
    IN GT_U32 deviceId,
    IN GT_U32 address,
    IN GT_U32 memSize,
    OUT GT_U32 * memPtr,
    IN GT_U32  dataIsWords
)
{
    /* ask to send it to the next station */
    simDistributedDmaRead(deviceId,address,memSize,memPtr,dataIsWords);
}

/*******************************************************************************
* busDmaWrite
*
* DESCRIPTION:
*       function Received request for write DMA values , and will write
*       DMA
*
*       APPLICABLE SIDE : on server dispatcher task --> got the request for
*                         write from the device(s) side , will send the info to
*                         the application side ,(will not wait for reply),(will not send reply)
*
* INPUTS:
*       deviceId - not used
*       address - DMA address
*       memSize - number of words to write to DMA
*       memPtr - pointer to the data
*       dataIsWords - not used
* OUTPUTS:
*       none
*
* RETURNS:
*       none
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static void busDmaWrite
(
    IN GT_U32 deviceId,
    IN GT_U32 address,
    IN GT_U32 memSize,
    INOUT GT_U32 * memPtr,
    IN GT_U32  dataIsWords
)
{
    /* ask to send it to the next station */
    simDistributedDmaWrite(deviceId,address,memSize,memPtr,dataIsWords);
}

/*******************************************************************************
* busDeviceInit
*
* DESCRIPTION:
*       function Received message to initialize a device of the Asic on SCIB
*       of application .(will not send reply)
*       function called from the dispatcher of messages from device ,
*       and function should send it to the connection of application/broker
*
*       APPLICABLE SIDE : on server dispatcher task --> got the request for
*                         write from the device(s) side , will send the info to
*                         the application side ,(will not wait for reply),(will not send reply)
*
* INPUTS:
*       deviceInfoPtr - pointer to info about the device that is 'ready'
*
* OUTPUTS:
*       none
*
* RETURNS:
*       none
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static void busDeviceInit
(
    IN SIM_DISTRIBUTED_INIT_DEVICE_STC *deviceInfoPtr
)
{
    /* ask to send it to the next station */
    simDistributedRemoteInit(1,deviceInfoPtr);
}


/*******************************************************************************
*  busReset
*
* DESCRIPTION:
*      application send to all the distributed parts on the system
*      the 'Reset' message
*
*      so we need to send this message to all clients
*
* INPUTS:
*       None
*
* OUTPUTS:
*       None
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void busReset
(
    void
)
{
    GT_U32  ii;

    for(ii = 0 ; ii < MAX_DEVICES_ON_BOARD_CNS; ii++)
    {
        if(mapDeviceToClientIndex[ii] == CLIENT_NOT_READY)
        {
            continue;
        }

        /* send reset to this client */
        simDistributedRemoteResetSend(mapDeviceToClientIndex[ii]);
    }
}

/*******************************************************************************
* busInitPart1
*
* DESCRIPTION:
*       function to do bus initializations
*       function bind CB functions
*
* INPUTS:
*       none.
*
* OUTPUTS:
*       none
*
* RETURNS:
*       none
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static void busInitPart1(void)
{
    msgInterruptSet             = busInterruptSet;
    msgDmaRead                  = busDmaRead;
    msgDmaWrite                 = busDmaWrite;
    msgRegisterRead             = busRegisterRead;
    msgRegisterWrite            = busRegisterWrite;
    msgDeviceInit               = busDeviceInit;
    msgReset                    = busReset;
}

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/********   application via broker specifics **********************************/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/

/*******************************************************************************
*  simDistributedAppPidSet
*
* DESCRIPTION:
*      set application ID --> direction is application to broker
* INPUTS:
*       pid  - processId
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void simDistributedAppPidSet
(
    IN GT_PROCESS_ID pid
)
{
    CONNECTION_INFO_STC *connectionPtr;
    MSG_HDR  *messageHeaderPtr;/* header of current treated message */
    APP_PID_MSG   message;
    GT_U32  dataLenWords = LEN_NO_HEADER_WORDS(APP_PID_MSG);
    GT_U32  *dataPtr = FIRST_FIELD_AFTER_HEADER_PTR(&message);
    DISTRIBUTED_MESSAGE_TYPE_ENT msgType = DISTRIBUTED_MESSAGE_TYPE_APP_PID_E;

    connectionPtr = convertMessageTypeToConnectionIndex(msgType);

    /****************************/
    /* build the message header */
    /****************************/
    messageHeaderPtr = &message.hdr;
    messageHeaderPtr->type = (VARIABLE_TYPE)msgType;
    messageHeaderPtr->msgLen = (VARIABLE_TYPE)(dataLenWords*4);/* in bytes */

    /****************************/
    /* build the message info   */
    /****************************/
    message.processId = (VARIABLE_TYPE)pid;

    /**************************************/
    /* send the message to the other side */
    /**************************************/
    simDistributedMessageSend(FIRST_SOCKET,connectionPtr,messageHeaderPtr,1,&dataLenWords, (GT_U32**)(&dataPtr),
        GT_FALSE,GT_FALSE,GT_TRUE);

}

/*******************************************************************************
*  simDistributedRegisterDma
*
* DESCRIPTION:
*      register DMA info to broker --> direction is application to broker
* INPUTS:
*       startAddress  - DMA start address
*       size - size of DMA in bytes
*       key - the key of shared memory that represent the DMA
*       dmaMode - the DMA mode (socket/shared memory)
*
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void simDistributedRegisterDma
(
    IN GT_U32  startAddress,
    IN GT_U32  size,
    IN GT_SH_MEM_KEY  key,
    IN BROKER_DMA_MODE dmaMode
)
{
    CONNECTION_INFO_STC *connectionPtr;
    MSG_HDR  *messageHeaderPtr;/* header of current treated message */
    REGISTER_DMA_MSG   message;
    GT_U32  dataLenWords = LEN_NO_HEADER_WORDS(REGISTER_DMA_MSG);
    GT_U32  *dataPtr = FIRST_FIELD_AFTER_HEADER_PTR(&message);
    DISTRIBUTED_MESSAGE_TYPE_ENT msgType = DISTRIBUTED_MESSAGE_TYPE_REGISTER_DMA_E;

    connectionPtr = convertMessageTypeToConnectionIndex(msgType);

    /****************************/
    /* build the message header */
    /****************************/
    messageHeaderPtr = &message.hdr;
    messageHeaderPtr->type = (VARIABLE_TYPE)msgType;
    messageHeaderPtr->msgLen = (VARIABLE_TYPE)(dataLenWords*4);/* in bytes */

    /****************************/
    /* build the message info   */
    /****************************/
    message.startAddress = (VARIABLE_TYPE)startAddress;
    message.size = (VARIABLE_TYPE)size;
    message.key = (VARIABLE_TYPE)key;
    message.dmaMode = (VARIABLE_TYPE)dmaMode;

    /**************************************/
    /* send the message to the other side */
    /**************************************/
    simDistributedMessageSend(FIRST_SOCKET,connectionPtr,messageHeaderPtr,1,&dataLenWords, (GT_U32**)(&dataPtr),
        GT_FALSE,GT_FALSE,GT_TRUE);

}

/*******************************************************************************
*  simDistributedInterruptUnmask
*
* DESCRIPTION:
*      send "unmask interrupt" message : application send to the
*             broker request that broker will be able to signal
*             application on interrupt.
*             NOTE : the issue is that once the broker signal the application
*             about interrupt, the broker will not signal it again even if
*             received another interrupt from device , until application
*             will send MSG_TYPE_UNMASK_INTERRUPT
*          -- see modes of BROKER_INTERRUPT_MASK_MODE --
* INPUTS:
*       interruptLineId - interrupt line ID that when device triggers it ,
*                       the broker will signal the application with the signalId
*                       may use value ALL_INTERRUPT_LINES_ID
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void simDistributedInterruptUnmask
(
    IN GT_U32 interruptLineId
)
{
    CONNECTION_INFO_STC *connectionPtr;
    MSG_HDR  *messageHeaderPtr;/* header of current treated message */
    UNMASK_INTERRUPT_MSG   message;
    GT_U32  dataLenWords = LEN_NO_HEADER_WORDS(UNMASK_INTERRUPT_MSG);
    GT_U32  *dataPtr = FIRST_FIELD_AFTER_HEADER_PTR(&message);
    DISTRIBUTED_MESSAGE_TYPE_ENT msgType = DISTRIBUTED_MESSAGE_TYPE_UNMASK_INTERRUPT_E;

    connectionPtr = convertMessageTypeToConnectionIndex(msgType);

    /****************************/
    /* build the message header */
    /****************************/
    messageHeaderPtr = &message.hdr;
    messageHeaderPtr->type = (VARIABLE_TYPE)msgType;
    messageHeaderPtr->msgLen = (VARIABLE_TYPE)(dataLenWords*4);/* in bytes */

    /****************************/
    /* build the message info   */
    /****************************/
    message.interruptLineId = (VARIABLE_TYPE)interruptLineId;

    /**************************************/
    /* send the message to the other side */
    /**************************************/
    simDistributedMessageSend(FIRST_SOCKET,connectionPtr,messageHeaderPtr,1,&dataLenWords, (GT_U32**)(&dataPtr),
        GT_FALSE,GT_FALSE,GT_TRUE);

}

/*******************************************************************************
*  simDistributedInterruptMask
*
* DESCRIPTION:
*      send "mask interrupt" message : application send to the
*             broker request that broker will NOT be able to signal
*             application on interrupt.
*             NOTE : see modes of BROKER_INTERRUPT_MASK_MODE
* INPUTS:
*       interruptLineId - interrupt line ID that when device triggers it ,
*                       the broker will NOT signal the application with the
*                       signalId
*                       may use value ALL_INTERRUPT_LINES_ID
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void simDistributedInterruptMask
(
    IN GT_U32 interruptLineId
)
{
    CONNECTION_INFO_STC *connectionPtr;
    MSG_HDR  *messageHeaderPtr;/* header of current treated message */
    MASK_INTERRUPT_MSG   message;
    GT_U32  dataLenWords = LEN_NO_HEADER_WORDS(MASK_INTERRUPT_MSG);
    GT_U32  *dataPtr = FIRST_FIELD_AFTER_HEADER_PTR(&message);
    DISTRIBUTED_MESSAGE_TYPE_ENT msgType = DISTRIBUTED_MESSAGE_TYPE_MASK_INTERRUPT_E;

    connectionPtr = convertMessageTypeToConnectionIndex(msgType);

    /****************************/
    /* build the message header */
    /****************************/
    messageHeaderPtr = &message.hdr;
    messageHeaderPtr->type = (VARIABLE_TYPE)msgType;
    messageHeaderPtr->msgLen = (VARIABLE_TYPE)(dataLenWords*4);/* in bytes */

    /****************************/
    /* build the message info   */
    /****************************/
    message.interruptLineId = (VARIABLE_TYPE)interruptLineId;

    /**************************************/
    /* send the message to the other side */
    /**************************************/
    simDistributedMessageSend(FIRST_SOCKET,connectionPtr,messageHeaderPtr,1,&dataLenWords, (GT_U32**)(&dataPtr),
        GT_FALSE,GT_FALSE,GT_TRUE);

}

/*******************************************************************************
*  appViaBrokerRegisterInterruptMsgSend
*
* DESCRIPTION:
*      register Interrupt info to broker --> direction is application to broker
* INPUTS:
*       interruptLineId - interrupt line ID that when device triggers it ,
*                       the broker will signal the application with the signalId
*       signalId      - the signal ID to send on 'Interrupt set'
*       mode          - broker interrupt mode , one of BROKER_INTERRUPT_MASK_MODE
*
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void appViaBrokerRegisterInterruptMsgSend
(
    IN GT_U32  interruptLineId,
    IN GT_U32  signalId,
    IN BROKER_INTERRUPT_MASK_MODE  maskMode
)
{
    CONNECTION_INFO_STC *connectionPtr;
    MSG_HDR  *messageHeaderPtr;/* header of current treated message */
    REGISTER_INTERRUPT_MSG   message;
    GT_U32  dataLenWords = LEN_NO_HEADER_WORDS(REGISTER_INTERRUPT_MSG);
    GT_U32  *dataPtr = FIRST_FIELD_AFTER_HEADER_PTR(&message);
    DISTRIBUTED_MESSAGE_TYPE_ENT msgType = DISTRIBUTED_MESSAGE_TYPE_REGISTER_INTERRUPT_E;

    connectionPtr = convertMessageTypeToConnectionIndex(msgType);

    /****************************/
    /* build the message header */
    /****************************/
    messageHeaderPtr = &message.hdr;
    messageHeaderPtr->type = (VARIABLE_TYPE)msgType;
    messageHeaderPtr->msgLen = (VARIABLE_TYPE)(dataLenWords*4);/* in bytes */

    /****************************/
    /* build the message info   */
    /****************************/
    message.interruptLineId = (VARIABLE_TYPE)interruptLineId;
    message.signalId = (VARIABLE_TYPE)signalId;
    message.maskMode = (VARIABLE_TYPE)maskMode;

    /**************************************/
    /* send the message to the other side */
    /**************************************/
    simDistributedMessageSend(FIRST_SOCKET,connectionPtr,messageHeaderPtr,1,&dataLenWords, (GT_U32**)(&dataPtr),
        GT_FALSE,GT_FALSE,GT_TRUE);
}



/*******************************************************************************
*  processAppViaBrokerInterruptHadler
*
* DESCRIPTION:
*      handler to be triggered from the broker process , indicating the device
*      that issued interrupt
* INPUTS:
*       signalId      - the signal ID to send on 'Interrupt set'
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void processAppViaBrokerInterruptHadler(
    IN GT_SIGNAL_ID signalId
)
{
    /* signal Id used as device ID */
    scibSetInterrupt((GT_U32)signalId);
}

/*******************************************************************************
*  simDistributedRegisterInterrupt
*
* DESCRIPTION:
*      register Interrupt info to broker --> direction is application to broker
* INPUTS:
*       interruptLineId - interrupt line ID that when device triggers it ,
*                       the broker will signal the application with the signalId
*       signalId      - the signal ID to send on 'Interrupt set'
*       mode          - broker interrupt mode , one of BROKER_INTERRUPT_MASK_MODE
*
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void simDistributedRegisterInterrupt
(
    IN GT_U32  interruptLineId,
    IN GT_U32  signalId,
    IN BROKER_INTERRUPT_MASK_MODE  maskMode
)
{
    char    getStr[SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS];/* string to read from INI file */
    GT_CHAR tmpStr[SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS];
    GT_U32          tmpValue;
    GT_BOOL toRegister;
    GT_STATUS   rc;

    /* set the distributed INI file as the working INI file */
    sinitIniFileSet(sdistIniFile);

    /* get the info if we need to register the application to the interrupts */
    sprintf(tmpStr, iniFileAppViaBrokerRegisterInterruptStr , sinitMultiProcessOwnApplicationId);
    if(SIM_OS_MAC(simOsGetCnfValue)(iniFileBrokerSectionStr, tmpStr,
                             SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS, getStr))
    {
        sscanf(getStr, "%d", &tmpValue);
        toRegister = (tmpValue == 1) ? GT_FALSE : GT_TRUE;
    }
    else
    {
        toRegister = GT_TRUE;
    }

    /* restore working with the main INI file */
    sinitIniFileRestoreMain();

    if(toRegister == GT_TRUE)
    {

        if(brokerInterruptMode != BROKER_USE_SOCKET)
        {
            /* bind our process with handler for the specific signalId */
            rc = SIM_OS_MAC(simOsProcessHandler)(signalId,
                                    &processAppViaBrokerInterruptHadler);
            if(rc != GT_OK)
            {
                skernelFatalError(" applicationViaBrokerInitPart2 : failed to set process handler \n");
            }
        }
        /* send the message to the broker */
        appViaBrokerRegisterInterruptMsgSend(interruptLineId,signalId,maskMode);
    }
}


/*******************************************************************************
*   applicationViaBrokerInitPart2
*
* DESCRIPTION:
*       do initialization that needed for application that communicate with the
*       broker
*
* INPUTS:
*       None.
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
void applicationViaBrokerInitPart2(void)
{
    GT_STATUS       rc;
    GT_PROCESS_ID   ownProcessId;

    rc = SIM_OS_MAC(simOsProcessIdGet)(&ownProcessId);
    if(rc != GT_OK)
    {
        skernelFatalError("applicationViaBrokerInitPart2: failed to get ownProcessId \n");
    }

    /* send the PID */
    simDistributedAppPidSet(ownProcessId);

    /*******************************/
    /* init the DMA parameters     */
    /* will be done by application */
    /*******************************/

    /*********************************/
    /* init the interrupt parameters */
    /* will be done by application   */
    /*********************************/

}



/*******************************************************************************
*   convertMessageTypeToConnectionIndex
*
* DESCRIPTION:
*       function convert
*
* INPUTS:
*       deviceId - device id
*
* OUTPUTS:
*       clientIndexPtr - pointer to the index of the client
*
* RETURNS:
*       pointer to the connection
* COMMENTS:
*
*******************************************************************************/
static CONNECTION_INFO_STC* convertMessageTypeToConnectionIndex(
    IN DISTRIBUTED_MESSAGE_TYPE_ENT msgType
)
{
    switch(sasicgSimulationRole)
    {
        case SASICG_SIMULATION_ROLE_DISTRIBUTED_APPLICATION_SIDE_E:
        case SASICG_SIMULATION_ROLE_DISTRIBUTED_APPLICATION_SIDE_VIA_BROKER_E:
            switch(msgType)
            {
                case DISTRIBUTED_MESSAGE_TYPE_REGISTER_READ_E:
                case DISTRIBUTED_MESSAGE_TYPE_REGISTER_WRITE_E:
                    return &connectionsArray[CONNECTION_TYPE_SYNCH_E][0];
                case DISTRIBUTED_MESSAGE_TYPE_NIC_TX_FRAME_E:
                case DISTRIBUTED_MESSAGE_TYPE_NIC_RX_FRAME_E:
                    skernelFatalError(" convertMessageTypeToConnectionIndex : msg not implemented msgType[%d]",(GT_U32)msgType);
                    break;
                case DISTRIBUTED_MESSAGE_TYPE_INTERRUPT_SET_E:
                case DISTRIBUTED_MESSAGE_TYPE_DMA_READ_E:
                case DISTRIBUTED_MESSAGE_TYPE_DMA_WRITE_E:
                case DISTRIBUTED_MESSAGE_TYPE_DEVICE_TO_APPLICATION_INIT_PARAM_E:
                    skernelFatalError(" convertMessageTypeToConnectionIndex : msg not for this side msgType[%d]",(GT_U32)msgType);
                    break;
                case DISTRIBUTED_MESSAGE_TYPE_RESET_E:
                case DISTRIBUTED_MESSAGE_TYPE_DEBUG_LEVEL_SET:
                    return &connectionsArray[CONNECTION_TYPE_SYNCH_E][0];
                case DISTRIBUTED_MESSAGE_TYPE_REGISTER_READ_REPLY_E:
                case DISTRIBUTED_MESSAGE_TYPE_DMA_READ_REPLY_E:
                    skernelFatalError(" convertMessageTypeToConnectionIndex : should not be called msgType[%d]",(GT_U32)msgType);
                    break;
                case DISTRIBUTED_MESSAGE_TYPE_APP_PID_E:
                case DISTRIBUTED_MESSAGE_TYPE_REGISTER_DMA_E:
                case DISTRIBUTED_MESSAGE_TYPE_REGISTER_INTERRUPT_E:
                case DISTRIBUTED_MESSAGE_TYPE_UNMASK_INTERRUPT_E:
                case DISTRIBUTED_MESSAGE_TYPE_MASK_INTERRUPT_E:
                    if(sasicgSimulationRole != SASICG_SIMULATION_ROLE_DISTRIBUTED_APPLICATION_SIDE_VIA_BROKER_E)
                    {
                        skernelFatalError(" convertMessageTypeToConnectionIndex : out of range msgType[%d]",(GT_U32)msgType);
                    }
                    return &connectionsArray[CONNECTION_TYPE_SYNCH_E][0];
                default:
                    skernelFatalError(" convertMessageTypeToConnectionIndex : out of range msgType[%d]",(GT_U32)msgType);
                    break;
            }

            break;

        case SASICG_SIMULATION_ROLE_DISTRIBUTED_ASIC_SIDE_E:
        case SASICG_SIMULATION_ROLE_DISTRIBUTED_ASIC_SIDE_VIA_INTERFACE_BUS_BRIDGE_E:
            switch(msgType)
            {
                case DISTRIBUTED_MESSAGE_TYPE_REGISTER_READ_E:
                case DISTRIBUTED_MESSAGE_TYPE_REGISTER_WRITE_E:
                    skernelFatalError(" convertMessageTypeToConnectionIndex : msg not for this side msgType[%d]",(GT_U32)msgType);
                    break;
                case DISTRIBUTED_MESSAGE_TYPE_NIC_TX_FRAME_E:
                case DISTRIBUTED_MESSAGE_TYPE_NIC_RX_FRAME_E:
                    skernelFatalError(" convertMessageTypeToConnectionIndex : msg not implemented msgType[%d]",(GT_U32)msgType);
                    break;
                case DISTRIBUTED_MESSAGE_TYPE_INTERRUPT_SET_E:
                case DISTRIBUTED_MESSAGE_TYPE_DMA_READ_E:
                case DISTRIBUTED_MESSAGE_TYPE_DMA_WRITE_E:
                    return &connectionsArray[CONNECTION_TYPE_ASYNCH_E][0];
                case DISTRIBUTED_MESSAGE_TYPE_DEVICE_TO_APPLICATION_INIT_PARAM_E:
                    return &connectionsArray[CONNECTION_TYPE_ASYNCH_E][0];
                case DISTRIBUTED_MESSAGE_TYPE_RESET_E:
                case DISTRIBUTED_MESSAGE_TYPE_DEBUG_LEVEL_SET:
                    skernelFatalError(" convertMessageTypeToConnectionIndex : should not be called msgType[%d]",(GT_U32)msgType);
                case DISTRIBUTED_MESSAGE_TYPE_REGISTER_READ_REPLY_E:
                case DISTRIBUTED_MESSAGE_TYPE_DMA_READ_REPLY_E:
                    skernelFatalError(" convertMessageTypeToConnectionIndex : should not be called msgType[%d]",(GT_U32)msgType);
                    break;
                default:
                    skernelFatalError(" convertMessageTypeToConnectionIndex : out of range msgType[%d]",(GT_U32)msgType);
                    break;
            }

            break;

        case SASICG_SIMULATION_ROLE_BROKER_E:
            switch(msgType)
            {
                case DISTRIBUTED_MESSAGE_TYPE_REGISTER_READ_E:
                case DISTRIBUTED_MESSAGE_TYPE_REGISTER_WRITE_E:
                /*
                    send to synch socket of the internal task (that get from Q) - to single client
                */
                    return &connectionsArray[CONNECTION_TYPE_SYNCH_E][BROKER_CLIENT_INDEX];
                case DISTRIBUTED_MESSAGE_TYPE_NIC_TX_FRAME_E:
                case DISTRIBUTED_MESSAGE_TYPE_NIC_RX_FRAME_E:
                    skernelFatalError(" convertMessageTypeToConnectionIndex : msg not implemented msgType[%d]",(GT_U32)msgType);
                    break;
                case DISTRIBUTED_MESSAGE_TYPE_INTERRUPT_SET_E:
                    /*
                        cb from broker call it (on async socket dispatcher context from asic)(if in mode of interrupt via socket)  - to all registered servers.
                        otherwise signal registered applications (processes)
                    */
                    skernelFatalError(" convertMessageTypeToConnectionIndex : msg not for this side msgType[%d]",(GT_U32)msgType);
                    break;
                case DISTRIBUTED_MESSAGE_TYPE_DMA_READ_E:
                case DISTRIBUTED_MESSAGE_TYPE_DMA_WRITE_E:
                    /*
                        cb from broker call it (on async socket dispatcher context from asic)(if in mode of DMA via socket)  - to single registered server.
                        otherwise do read/write DMA from/to shared memory
                    */
                    return &connectionsArray[CONNECTION_TYPE_ASYNCH_E][BROKER_SERVER_INDEX];
                case DISTRIBUTED_MESSAGE_TYPE_DEVICE_TO_APPLICATION_INIT_PARAM_E:
                    /*
                        cb from broker send that message (on async socket dispatcher context from asic) - to all registered servers.
                    */
                    skernelFatalError(" convertMessageTypeToConnectionIndex : msg not for this side msgType[%d]",(GT_U32)msgType);
                    break;
                case DISTRIBUTED_MESSAGE_TYPE_RESET_E:
                case DISTRIBUTED_MESSAGE_TYPE_DEBUG_LEVEL_SET:
                /*
                    send to synch socket of the internal task (that get from Q) - to single client
                */
                    return &connectionsArray[CONNECTION_TYPE_SYNCH_E][BROKER_CLIENT_INDEX];

                case DISTRIBUTED_MESSAGE_TYPE_REGISTER_READ_REPLY_E:
                case DISTRIBUTED_MESSAGE_TYPE_DMA_READ_REPLY_E:
                    skernelFatalError(" convertMessageTypeToConnectionIndex : should not be called msgType[%d]",(GT_U32)msgType);
                    break;
                default:
                    skernelFatalError(" convertMessageTypeToConnectionIndex : out of range msgType[%d]",(GT_U32)msgType);
                    break;
            }

            break;

        case SASICG_SIMULATION_ROLE_DISTRIBUTED_INTERFACE_BUS_BRIDGE_E:
            switch(msgType)
            {
                case DISTRIBUTED_MESSAGE_TYPE_REGISTER_READ_E:
                case DISTRIBUTED_MESSAGE_TYPE_REGISTER_WRITE_E:
                /*
                    cb from bus call (on sync dispatcher context from application)  - to single client
                */
                    return &connectionsArray[CONNECTION_TYPE_SYNCH_E][BUS_CLIENT_INDEX];
                case DISTRIBUTED_MESSAGE_TYPE_NIC_TX_FRAME_E:
                case DISTRIBUTED_MESSAGE_TYPE_NIC_RX_FRAME_E:
                    skernelFatalError(" convertMessageTypeToConnectionIndex : msg not implemented msgType[%d]",(GT_U32)msgType);
                    break;
                case DISTRIBUTED_MESSAGE_TYPE_INTERRUPT_SET_E:
                /*
                    cb from bus call (on async dispatcher context from asic)  - to single server
                */
                case DISTRIBUTED_MESSAGE_TYPE_DMA_READ_E:
                case DISTRIBUTED_MESSAGE_TYPE_DMA_WRITE_E:
                /*
                    cb from bus call (on async dispatcher context from asic)  - to single server
                */
                    return &connectionsArray[CONNECTION_TYPE_ASYNCH_E][BUS_SERVER_INDEX];
                case DISTRIBUTED_MESSAGE_TYPE_DEVICE_TO_APPLICATION_INIT_PARAM_E:
                /*
                    cb from bus call (on async dispatcher context from asic)  - to single server
                */
                    return &connectionsArray[CONNECTION_TYPE_ASYNCH_E][BUS_SERVER_INDEX];
                case DISTRIBUTED_MESSAGE_TYPE_RESET_E:
                case DISTRIBUTED_MESSAGE_TYPE_DEBUG_LEVEL_SET:
                /*
                    cb from bus call (on sync dispatcher context from application)  - to single client
                */
                    return &connectionsArray[CONNECTION_TYPE_SYNCH_E][BUS_CLIENT_INDEX];

                case DISTRIBUTED_MESSAGE_TYPE_REGISTER_READ_REPLY_E:
                case DISTRIBUTED_MESSAGE_TYPE_DMA_READ_REPLY_E:
                    skernelFatalError(" convertMessageTypeToConnectionIndex : should not be called msgType[%d]",(GT_U32)msgType);
                    break;
                default:
                    skernelFatalError(" convertMessageTypeToConnectionIndex : out of range msgType[%d]",(GT_U32)msgType);
                    break;
            }

            break;

            default:
                skernelFatalError(" convertMessageTypeToConnectionIndex : not supported system",(GT_U32)sasicgSimulationRole);
                break;
    }

    /* should not get here */
    /* put fatal error just in case... */
    skernelFatalError(" convertMessageTypeToConnectionIndex : in system[%s] not covered case",
                    consoleDistStr[sasicgSimulationRole],msgType);
    return NULL;
}

