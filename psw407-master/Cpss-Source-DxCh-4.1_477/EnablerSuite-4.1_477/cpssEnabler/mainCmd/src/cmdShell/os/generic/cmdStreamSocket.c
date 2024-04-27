/*******************************************************************************
*              (c), Copyright 2007, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* cmdStreamSocket.c
*
* DESCRIPTION:
*       This file contains common functions for mainCmd socket streams
*
* FILE REVISION NUMBER:
*       $Revision: 6 $
*
*******************************************************************************/
#include <cmdShell/os/cmdStreamImpl.h>

/*********** generic socket stream implementation *****************************/
typedef struct {
    cmdStreamSTC common;
    GT_SOCKET_FD socket;
    GT_BOOL isConnected;
    GT_VOID *rdSet;
} socketStreamSTC, *socketStreamPTR;

static int socketStreamDestroy(cmdStreamPTR streamP)
{
    socketStreamPTR stream = (socketStreamPTR)streamP;

    if (!stream)
        return -1;

    cmdOsSocketTcpDestroy(stream->socket);
    cmdOsSelectEraseSet(stream->rdSet);

    cmdOsFree(stream);

    return 0;
}
static int socketStreamRead(cmdStreamPTR streamP, void* bufferPtr, int bufferLength)
{
    socketStreamPTR stream = (socketStreamPTR)streamP;
    int k;

    if (!stream)
        return -1;

    if (!stream->isConnected)
        return -1;

    /* set input mode */
    k = cmdOsSocketRecv(stream->socket, bufferPtr, bufferLength, GT_TRUE);
    if (k <= 0)
    {
        stream->isConnected = GT_FALSE;
    }

    return k;
}

static int socketStreamReadChar(cmdStreamPTR streamP, char* charPtr, int timeOut)
{
    socketStreamPTR stream = (socketStreamPTR)streamP;
    int k;

    if (!stream)
        return -1;

    if (!stream->isConnected)
        return -1;

    cmdOsSelectZeroSet(stream->rdSet);
    cmdOsSelectAddFdToSet(stream->rdSet, stream->socket);

    k = cmdOsSelect(stream->socket+1, stream->rdSet, NULL, NULL,
            timeOut < 0 ? 0xffffffff: (GT_U32)timeOut);

    if (k <= 0) /* error or timeout */
        return k;

    if (cmdOsSelectIsFdSet(stream->rdSet, stream->socket) == GT_FALSE)
        return -1; /* unexpected error */

    /* set input mode */
    k = cmdOsSocketRecv(stream->socket, charPtr, 1, GT_TRUE);
    if (k <= 0)
    {
        stream->isConnected = GT_FALSE;
        return -1;
    }

    return k;
}

static int socketStreamWrite(cmdStreamPTR streamP, const void* bufferPtr, int bufferLength)
{
    socketStreamPTR stream = (socketStreamPTR)streamP;
    /* do this binding to fix const cast warning */
    typedef GT_SIZE (*STREAM_SOCKET_SEND_FUN)
    (
        IN  GT_SOCKET_FD   socketFd ,
        IN  const void*    buff,
        IN  GT_SIZE        buffLen
    );
    STREAM_SOCKET_SEND_FUN send_fun = (STREAM_SOCKET_SEND_FUN)cmdOsSocketSend;

    if (!stream)
        return -1;

    if (!stream->isConnected)
        return -1;

    return send_fun(stream->socket, bufferPtr, bufferLength);
}
static int socketStreamConnected(cmdStreamPTR streamP)
{
    socketStreamPTR stream = (socketStreamPTR)streamP;

    if (!stream)
        return -1;

    return stream->isConnected;
}

static int socketGrabStd(cmdStreamPTR streamP);

static int socketSetTtyMode(cmdStreamPTR streamP, int mode)
{
    socketStreamPTR stream = (socketStreamPTR)streamP;

    if (!stream)
        return -1;

    if (mode == 0)
    {
        /* raw mode */
        stream->common.flags &= ~(
                STREAM_FLAG_O_CRLF |
                STREAM_FLAG_I_CANON |
                STREAM_FLAG_I_ECHO
            );
    }
    else
    {
        /* line edit mode */
        stream->common.flags |= 
            STREAM_FLAG_O_CRLF |
            STREAM_FLAG_I_CANON |
            STREAM_FLAG_I_ECHO;
    }
    return 0;
}

static int socketGetFd(cmdStreamPTR streamP)
{
    socketStreamPTR stream = (socketStreamPTR)streamP;

    if (!stream)
        return -1;

    return (int)stream->socket;
}

/*******************************************************************************
* cmdStreamCreateSocket
*
* DESCRIPTION:
*       Create socket stream
*
* INPUTS:
*       socket - socket descriptor
*
* OUTPUTS:
*       None
*
* RETURNS:
*       stream - socket stream
*       NULL if error
*
* COMMENTS:
*       None
*
*******************************************************************************/
cmdStreamPTR cmdStreamCreateSocket
(
    IN GT_SOCKET_FD socket
)
{
    socketStreamPTR stream;

    stream = (socketStreamPTR)cmdOsMalloc(sizeof(*stream));
    if (!stream)
    {
        return NULL;
    }

    cmdOsMemSet(stream, 0, sizeof(*stream));
    stream->common.destroy = socketStreamDestroy;
    stream->common.read = socketStreamRead;
    stream->common.readChar = socketStreamReadChar;
    stream->common.readLine = streamGenericReadLine;
    stream->common.write = socketStreamWrite;
    stream->common.writeBuf = streamGenericWriteBuf;
    stream->common.writeLine = streamGenericWriteLine;
    stream->common.connected = socketStreamConnected;
    stream->common.grabStd = socketGrabStd;
    stream->common.setTtyMode = socketSetTtyMode;
    stream->common.getFd = socketGetFd;
    stream->common.isConsole = GT_FALSE;
    stream->common.flags = 
        STREAM_FLAG_O_CRLF |
        STREAM_FLAG_I_CANON |
        STREAM_FLAG_I_ECHO;
    stream->socket = socket;
    stream->isConnected = GT_TRUE;
    stream->rdSet = cmdOsSelectCreateSet();

    cmdOsSocketSetBlock(socket);

    return (cmdStreamPTR)stream;
}














/*********** Socket listener implementation **********************************/

/*******************************************************************************
* osSocketTcpCreateListen
*
* DESCRIPTION:
*       Create TCP socket, bind it and start listening for incoming connections
*
* INPUTS:
*       ip                - ip address to listen on
*                           NULL means listen on all interfaces
*       port              - tcp port for incoming connections
*
* OUTPUTS:
*       socketPtr         - Store socket descriptior
*
* RETURNS:
*       GT_OK
*       GT_FAIL
*
* COMMENTS:
*       None
*
*******************************************************************************/
GT_STATUS osSocketTcpCreateListen
(
    IN  char *ip,
    IN  int port,
    OUT GT_SOCKET_FD *socketPtr
)
{
    GT_SOCKET_FD    socket;
    GT_VOID*        bindAddr;
    GT_SIZE         bindAddrLen;

    if (!socketPtr)
        return GT_BAD_PARAM;

    /* create master server socket and start listening on specified ip/port */
    socket = cmdOsSocketTcpCreate(GT_SOCKET_DEFAULT_SIZE);
    if (socket < 0)
        return GT_FAIL;

    if (cmdOsSocketCreateAddr(ip, port, &bindAddr, &bindAddrLen) != GT_OK)
    {
        cmdOsSocketTcpDestroy(socket);
        return GT_FAIL;
    }

    if (cmdOsSocketBind(socket, bindAddr, bindAddrLen) != GT_OK)
    {
        cmdOsSocketDestroyAddr(bindAddr);
        cmdOsSocketTcpDestroy(socket);
        return GT_FAIL;
    }

    cmdOsSocketDestroyAddr(bindAddr);
    if (cmdOsSocketListen(socket, 32) != GT_OK)
    {
        cmdOsSocketTcpDestroy(socket);
        return GT_FAIL;
    }

    *socketPtr = socket;
    return GT_OK;
}

#if 0
/*
 * Typedef: struct LISTENER_PARAMS_STC
 *
 * Description:
 *      This structure is used to pass listener parameters
 *      to newly created listener task
 *
 * Fields:
 *      masterSocket
 *      streamTaskName
 *      handler
 *      multipleInstances
 *      isConsole
 */
typedef struct LISTENER_PARAMS_STC {
    GT_SOCKET_FD masterSocket;
    char *streamTaskName;
    eventLoopHandlerPTR handler;
    GT_BOOL multipleInstances;
    GT_BOOL isConsole;
} LISTENER_PARAMS_STC;

/*******************************************************************************
* cmdStreamSocketListenerTask
*
* DESCRIPTION:
*       Socket listener task
*
* INPUTS:
*       params
*
* OUTPUTS:
*       None
*
* RETURNS:
*       None
*
* COMMENTS:
*       None
*
*******************************************************************************/
static GT_VOID __TASKCONV cmdStreamSocketListenerTask
(
    LISTENER_PARAMS_STC* params
)
{
    GT_SOCKET_FD socket;
    cmdStreamPTR  stream;
    int instance = 0;

    while (1)
    {
        socket = cmdOsSocketAccept(params->masterSocket, NULL, NULL);
        if (socket < 0)
        {
            /* handle? */
            continue;
        }
        if ((stream = cmdStreamCreateSocket(socket)) == NULL)
        {
            cmdOsSocketTcpDestroy(socket);
            /* handle? */
            continue;
        }
        if (params->multipleInstances == GT_FALSE)
        {
            stream->isConsole = params->isConsole;
            params->handler(stream);
        }
        else
        {
            char taskname[80];
            GT_TASK taskId;

            cmdOsSprintf(taskname, "%s%d", params->streamTaskName, instance++);

            if (cmdStreamStartEventLoopHandler(
                        taskname,
                        params->handler,
                        stream,
                        params->isConsole,
                        &taskId) != GT_OK)
            {
                stream->destroy(stream);
            }
        }
    }
}

/*******************************************************************************
* cmdStreamSocketCreateListenerTask
*
* DESCRIPTION:
*       Create socket listenet task
*       This task will accept incoming socket connections and then start
*       handler for each connection
*
* INPUTS:
*       listenerTaskName  - name for listener task
*       ip                - ip address to listen on
*                           NULL means listen on all interfaces
*       port                tcp port for incoming connections
*       streamTaskName    - task name for handler
*       handler           - pointer to handler function
*       multipleInstances - Allow more than one handler at time
*       isConsole         - application flag
*
* OUTPUTS:
*       taskId     - pointer for listener task ID
*
* RETURNS:
*       GT_OK
*       GT_FAIL
*
* COMMENTS:
*       None
*
*******************************************************************************/
GT_STATUS cmdStreamSocketCreateListenerTask(
    IN  char *listenerTaskName,
    IN  char *ip,
    IN  int port,
    IN  char *streamTaskName,
    IN  eventLoopHandlerPTR handler,
    IN  GT_BOOL multipleInstances,
    IN  GT_BOOL isConsole,
    OUT GT_TASK *taskId
)
{
    GT_SOCKET_FD    masterSocket;
    LISTENER_PARAMS_STC *params;

    if (osSocketTcpCreateListen(ip, port, &masterSocket) != GT_OK)
    {
        return GT_FAIL;
    }

    /* Now create listener task */
    params = (LISTENER_PARAMS_STC *)cmdOsMalloc(sizeof(*params));
    if (!params)
    {
        cmdOsSocketTcpDestroy(masterSocket);
        return GT_FAIL;
    }
    params->masterSocket = masterSocket;
    params->streamTaskName = streamTaskName;
    params->handler = handler;
    params->multipleInstances = multipleInstances;
    params->isConsole = isConsole;

    if (cmdOsTaskCreate(
                listenerTaskName,
                STREAM_THREAD_PRIO+1,   /* thread priority          */
                32768,                  /* use default stack size   */
                (unsigned (__TASKCONV *)(void*))
                    cmdStreamSocketListenerTask,
                params,
                taskId) != GT_OK)
    {
        cmdOsSocketTcpDestroy(masterSocket);
        cmdOsFree(params);
        return GT_FAIL;
    }

    return GT_OK;
}
#endif

/*
 * Typedef: struct SERVICE_PARAMS_STC
 *
 * Description:
 *      This structure is used as service description
 *      to newly created listener task
 *
 * Fields:
 *      serviceName
 *      masterSocket
 *      streamTaskName
 *      handler
 *      multipleInstances
 *      isConsole
 */
typedef struct SERVICE_PARAMS_STC {
    const char *serviceName;
    char *ip;
    int port;
    eventLoopHandlerPTR handler;
    GT_BOOL multipleInstances;
    GT_BOOL isConsole;
    GT_SOCKET_FD masterSocket;
    GT_BOOL running;
    int instance;
    GT_TASK taskId;
} SERVICE_PARAMS_STC;
#define SERVICES_MAX 10

static SERVICE_PARAMS_STC services[SERVICES_MAX];
static int numServices = 0;

#ifdef CMD_LUA_CLI
GT_STATUS luaGlobalGetIntVal(
  IN  const char *key,
  OUT GT_U32     *valuePtr
);
#endif

/*******************************************************************************
* cmdStreamSocketServiceCreate
*
* DESCRIPTION:
*       Create socket listener service
*
* INPUTS:
*       serviceName       - service name
*       ip                - ip address to listen on
*                           NULL means listen on all interfaces
*       port                tcp port for incoming connections
*       handler           - pointer to handler function
*       multipleInstances - Allow more than one handler at time
*       isConsole         - application flag
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK
*       GT_FAIL
*
* COMMENTS:
*       None
*
*******************************************************************************/
GT_STATUS cmdStreamSocketServiceCreate(
    IN  const char *serviceName,
    IN  char *ip,
    IN  GT_U16 port,
    IN  eventLoopHandlerPTR handler,
    IN  GT_BOOL multipleInstances,
    IN  GT_BOOL isConsole
)
{
    int i;
    SERVICE_PARAMS_STC *s;
    for (i = 0; i < numServices; i++)
    {
        if (cmdOsStrCmp(serviceName, services[i].serviceName) == 0)
        {
            return GT_ALREADY_EXIST;
        }
    }
    if (numServices == SERVICES_MAX)
    {
        return GT_NO_RESOURCE;
    }
    s = &(services[numServices]);
    cmdOsMemSet(s, 0, sizeof(*s));

    s->serviceName = serviceName;
    if (ip)
    {
        s->ip = (char*)cmdOsMalloc(cmdOsStrlen(ip)+1);
        cmdOsStrCpy(s->ip, ip);
    }
    s->port = port;
#ifdef CMD_LUA_CLI
    {
        /* override port with _p_${serviceName} from luaGlobal, if exists */
        char varname[80];
        GT_U32 portNum;

        cmdOsSprintf(varname, "_p_%s", s->serviceName);
        if (luaGlobalGetIntVal(varname, &portNum) == GT_OK)
        {
            s->port = (int)portNum;
        }
    }
#endif
    s->handler = handler;
    s->multipleInstances = multipleInstances;
    s->isConsole = isConsole;
    s->running = GT_FALSE;

    numServices++;
    return GT_OK;
}

/*******************************************************************************
* cmdStreamSocketServiceListenerTask
*
* DESCRIPTION:
*       Socket service listener task
*
* INPUTS:
*       params
*
* OUTPUTS:
*       None
*
* RETURNS:
*       None
*
* COMMENTS:
*       None
*
*******************************************************************************/
static GT_VOID __TASKCONV cmdStreamSocketServiceListenerTask
(
    SERVICE_PARAMS_STC* params
)
{
    GT_SOCKET_FD socket;
    cmdStreamPTR stream;
    GT_VOID*     rdSet;
    rdSet = cmdOsSelectCreateSet();

    while (params->running == GT_TRUE)
    {
        /* Use select to check if service need to be stopped */
        cmdOsSelectZeroSet(rdSet);
        cmdOsSelectAddFdToSet(rdSet, params->masterSocket);

        if (cmdOsSelect(params->masterSocket+1, rdSet, NULL, NULL, 1000) <= 0)
            continue;

        socket = cmdOsSocketAccept(params->masterSocket, NULL, NULL);
        if (socket < 0)
        {
            /* handle? */
            continue;
        }
        if ((stream = cmdStreamCreateSocket(socket)) == NULL)
        {
            cmdOsSocketTcpDestroy(socket);
            /* handle? */
            continue;
        }
        if (params->multipleInstances == GT_FALSE)
        {
            stream->isConsole = params->isConsole;
            params->handler(stream);
        }
        else
        {
            char taskname[80];
            GT_TASK taskId;

            cmdOsSprintf(taskname, "%s_%d", params->serviceName, params->instance++);

            if (cmdStreamStartEventLoopHandler(
                        taskname,
                        params->handler,
                        stream,
                        params->isConsole,
                        &taskId) != GT_OK)
            {
                stream->destroy(stream);
            }
        }
    }
    cmdOsSelectEraseSet(rdSet);
}


/*******************************************************************************
* cmdStreamSocketServiceStart
*
* DESCRIPTION:
*       Start socket service
*
* INPUTS:
*       serviceName
*
* OUTPUTS:
*       None
*
* RETURNS:
*       status
*
* COMMENTS:
*       None
*
*******************************************************************************/
GT_STATUS cmdStreamSocketServiceStart(
    IN  const char *serviceName
)
{
    char listenerTaskName[80];
#define SERVICE_FIND() \
    int i; \
    SERVICE_PARAMS_STC *s; \
    for (i = 0; i < numServices; i++) \
    { \
        if (cmdOsStrCmp(serviceName, services[i].serviceName) == 0) \
        { \
            break; \
        } \
    } \
    if (i == numServices) \
    { \
        return GT_NO_SUCH; \
    } \
    s = &(services[i]);

    SERVICE_FIND();
    if (s->running == GT_TRUE)
    {
        return GT_OK;
    }

    if (osSocketTcpCreateListen(s->ip, s->port, &(s->masterSocket)) != GT_OK)
    {
        return GT_FAIL;
    }
    
    cmdOsSprintf(listenerTaskName, "%s_lsn", s->serviceName);
    s->running = GT_TRUE;
    /* Now create listener task */
    if (cmdOsTaskCreate(
                listenerTaskName,
                STREAM_THREAD_PRIO+1,   /* thread priority          */
                32768,                  /* use default stack size   */
                (unsigned (__TASKCONV *)(void*))
                    cmdStreamSocketServiceListenerTask,
                s,
                &(s->taskId)) != GT_OK)
    {
        cmdOsSocketTcpDestroy(s->masterSocket);
        s->running = GT_FALSE;
        return GT_FAIL;
    }
    return GT_OK;
}

/*******************************************************************************
* cmdStreamSocketServiceStop
*
* DESCRIPTION:
*       Stop socket service
*
* INPUTS:
*       serviceName
*
* OUTPUTS:
*       None
*
* RETURNS:
*       status
*
* COMMENTS:
*       None
*
*******************************************************************************/
GT_STATUS cmdStreamSocketServiceStop(
    IN  const char *serviceName
)
{
    SERVICE_FIND();
    if (s->running == GT_FALSE)
    {
        return GT_OK;
    }

    s->running = GT_FALSE;
    cmdOsSocketTcpDestroy(s->masterSocket); /* listen() may be interrupted */
    s->masterSocket = -1;
    return GT_OK;
}

/*******************************************************************************
* cmdStreamSocketServiceIsRunning
*
* DESCRIPTION:
*       Return service running status
*
* INPUTS:
*       serviceName
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_TRUE or GT_FALSE
*
* COMMENTS:
*       None
*
*******************************************************************************/
GT_BOOL cmdStreamSocketServiceIsRunning(
    IN  const char *serviceName
)
{
    int i;
    for (i = 0; i < numServices; i++)
    {
        if (cmdOsStrCmp(serviceName, services[i].serviceName) == 0)
        {
            return services[i].running;
        }
    }
    return GT_FALSE;
}

/*******************************************************************************
* cmdStreamSocketServiceGetPort
*
* DESCRIPTION:
*       Get service port number
*
* INPUTS:
*       serviceName
*
* OUTPUTS:
*       portNumberPtr
*
* RETURNS:
*       status
*
* COMMENTS:
*       None
*
*******************************************************************************/
GT_STATUS cmdStreamSocketServiceGetPort(
    IN  const char *serviceName,
    OUT GT_U16     *portNumberPtr
)
{
    SERVICE_FIND();

    if (portNumberPtr == NULL)
        return GT_BAD_PARAM;
    *portNumberPtr = (GT_U16)(s->port);
    return GT_OK;
}

/*******************************************************************************
* cmdStreamSocketServiceSetPort
*
* DESCRIPTION:
*       Get service port number
*
* INPUTS:
*       serviceName
*       portNumber
*
* OUTPUTS:
*
* RETURNS:
*       status
*
* COMMENTS:
*       None
*
*******************************************************************************/
GT_STATUS cmdStreamSocketServiceSetPort(
    IN  const char *serviceName,
    IN  GT_U16     portNumber
)
{
    SERVICE_FIND();
    s->port = portNumber;
    return GT_OK;
}
