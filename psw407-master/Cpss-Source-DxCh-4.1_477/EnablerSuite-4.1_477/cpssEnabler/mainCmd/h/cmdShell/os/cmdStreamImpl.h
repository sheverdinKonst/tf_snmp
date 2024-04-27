/*******************************************************************************
*              (c), Copyright 2007, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* cmdStreamImpl.h
*
* DESCRIPTION:
*       This file contains types and routines for mainCmd I/O streams
*
* FILE REVISION NUMBER:
*       $Revision: 2 $
*
*******************************************************************************/
#ifndef __cmdStreamImplh
#define __cmdStreamImplh

#include <cmdShell/common/cmdCommon.h>
#include <cmdShell/os/cmdStream.h>

#define STREAM_THREAD_PRIO 5

/*******************************************************************************
* cmdStreamGrabSystemOutput
*
* DESCRIPTION:
*       cmdStreamRedirectStdout() will grab stdout and stderr streams
*       if this flags set to GT_TRUE
*
*******************************************************************************/
extern GT_BOOL cmdStreamGrabSystemOutput;

/*******************************************************************************
* cmdStreamRedirectStdout
*
* DESCRIPTION:
*       Redirect stdout/stderr to given stream
*
* INPUTS:
*       stream - destination stream
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
GT_STATUS cmdStreamRedirectStdout
(
    IN cmdStreamPTR stream
);


/*******************************************************************************
* cmdStreamCreateNULL
*
* DESCRIPTION:
*       Create null stream. This stream equals to /dev/null
*
* INPUTS:
*       None
*
* OUTPUTS:
*       None
*
* RETURNS:
*       stream - null stream
*
* COMMENTS:
*       None
*
*******************************************************************************/
cmdStreamPTR cmdStreamCreateNULL(void);


/*******************************************************************************
* cmdStreamSerialInit
*
* DESCRIPTION:
*       Initialize serial engine
*
* INPUTS:
*       None
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
GT_STATUS cmdStreamSerialInit(void);

/*******************************************************************************
* cmdStreamSerialFinish
*
* DESCRIPTION:
*       Close serial engine
*
* INPUTS:
*       None
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
GT_STATUS cmdStreamSerialFinish(void);

/*******************************************************************************
* cmdStreamCreateSerial
*
* DESCRIPTION:
*       Create serial port stream
*
* INPUTS:
*       devNum  - the serial device port number (0 = COM1, ...)
*
* OUTPUTS:
*       None
*
* RETURNS:
*       stream - serial stream
*       NULL if error
*
* COMMENTS:
*       None
*
*******************************************************************************/
cmdStreamPTR cmdStreamCreateSerial
(
    IN GT_U32 devNum
);

/*******************************************************************************
* cmdStreamSocketInit
*
* DESCRIPTION:
*       Initialize TCP/IP socket engine
*
* INPUTS:
*       None
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
GT_STATUS cmdStreamSocketInit(void);

/*******************************************************************************
* cmdStreamSocketFinish
*
* DESCRIPTION:
*       Close socket engine
*
* INPUTS:
*       None
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
GT_STATUS cmdStreamSocketFinish(void);

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
);

/*******************************************************************************
* cmdStreamCreateTelnet
*
* DESCRIPTION:
*       Create Telnet protocol stream
*
* INPUTS:
*       socket - socket I/O stream
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
cmdStreamPTR cmdStreamCreateTelnet
(
    IN cmdStreamPTR socket
);

#if defined(_linux) || defined(__FreeBSD__)
/*******************************************************************************
* cmdStreamCreatePipe
*
* DESCRIPTION:
*       Create pipe stream
*
* INPUTS:
*       None
*
* OUTPUTS:
*       None
*
* RETURNS:
*       stream - pipe stream
*       NULL if error
*
* COMMENTS:
*       Input and output file descriptors must be specified in environment
*       variables PIPE_STREAM_INFD and PIPE_STREAM_OUTFD
*
*******************************************************************************/
cmdStreamPTR cmdStreamCreatePipe(void);
#endif /* defined(_linux) || defined(__FreeBSD__) */


/*
 * Typedef: eventLoopHandlerPTR
 *
 * Description:
 *      Pointer to application stream handler
 */
typedef GT_STATUS (*eventLoopHandlerPTR)(
        cmdStreamPTR stream
);

/*******************************************************************************
* cmdStreamStartEventLoopHandler
*
* DESCRIPTION:
*       Create new task to run stream handler
*
* INPUTS:
*       taskName   - task name for handler
*       handler    - pointer to handler function
*       stream     - stream
*       isConsole  - application flag
*
* OUTPUTS:
*       taskId     - pointer for task ID
*
* RETURNS:
*       GT_OK
*       GT_FAIL
*
* COMMENTS:
*       None
*
*******************************************************************************/
GT_STATUS cmdStreamStartEventLoopHandler
(
    IN  char *taskName,
    IN  eventLoopHandlerPTR handler,
    IN  cmdStreamPTR stream,
    IN  GT_BOOL isConsole,
    OUT GT_TASK *taskId
);

#if 0
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
);

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
);

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
);

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
);

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
);

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
);

/*******************************************************************************
* streamGenericReadLine
*
* DESCRIPTION:
*       Generic readLine method for cmdStreamSTC
*       Read line from stream. Perform input editing/translations
*       Returned line is zero terminated with stripped EOL symbols
*
* INPUTS:
*       stream       - destination stream
*       bufferLength - buffer length
*
* OUTPUTS:
*       bufferPtr    - pointer to output buffer
*
* RETURNS:
*       < 0 if error occured
*       number of received bytes
*
*******************************************************************************/
int streamGenericReadLine(cmdStreamPTR streamP, char* bufferPtr, int bufferLength);

/*******************************************************************************
* streamGenericWriteBuf
*
* DESCRIPTION:
*       Generic writeBuf method for cmdStreamSTC
*       Write to stream. Perform output translations
*
* INPUTS:
*       stream       - destination stream
*       s            - pointer to buffer
*       bufferLength - number of bytes to send
*
* OUTPUTS:
*       None
*
* RETURNS:
*       < 0 if error occured
*       number of bytes was sent
*
*******************************************************************************/
int streamGenericWriteBuf(cmdStreamPTR streamP, const char *s, int len);

/***************************************************************************
* streamGenericWriteLine
*
* DESCRIPTION:
*       Generic writeLine method for cmdStreamSTC
*       Write line to stream. Perform output translations
*
* INPUTS:
*       stream       - destination stream
*       s            - pointer to string
*
* OUTPUTS:
*       None
*
* RETURNS:
*       < 0 if error occured
*       number of bytes was sent
*
***************************************************************************/
int streamGenericWriteLine(cmdStreamPTR streamP, const char *s);

#endif /* __cmdStreamImplh */
