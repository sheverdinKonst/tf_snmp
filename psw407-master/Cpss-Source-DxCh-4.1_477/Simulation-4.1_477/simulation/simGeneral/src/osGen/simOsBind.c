/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* simOsBind.c
*
* DESCRIPTION:
*       This file implement the binding of OS functions for simulation usage.
*
* FILE REVISION NUMBER:
*       $Revision: 2 $
*******************************************************************************/

#include <os/simTypesBind.h>
#include <asicSimulation/SInit/sinit.h>

/************* Defines ***********************************************/

/************ Public Functions ************************************************/

#define FORCE_CAST          (void*)

#define BIND_FUNC(funcName)                                 \
    if(osFuncBindPtr->funcName)                             \
        SIM_OS_MAC(funcName) =  osFuncBindPtr->funcName;

#define BIND_LEVEL_FUNC(level,funcName)                                 \
    if(osFuncBindPtr->level.funcName)                             \
        SIM_OS_MAC(funcName) =  osFuncBindPtr->level.funcName;

static void fatalErrorBind(void);
static void dummyBind(void);


SIM_OS_WRAPPER_OPEN_FUN             SIM_OS_MAC(osWrapperOpen)       = FORCE_CAST dummyBind;
SIM_OS_SERIAL_DESTROY_SERIAL_FUN    SIM_OS_MAC(osSerialDestroySerial)= FORCE_CAST fatalErrorBind;

SIM_OS_SOCKET_TCP_CREATE_FUN        SIM_OS_MAC(osSocketTcpCreate)    = FORCE_CAST fatalErrorBind;
SIM_OS_SOCKET_UDP_CREATE_FUN        SIM_OS_MAC(osSocketUdpCreate)    = FORCE_CAST fatalErrorBind;
SIM_OS_SOCKET_TCP_DESTROY_FUN       SIM_OS_MAC(osSocketTcpDestroy)   = FORCE_CAST fatalErrorBind;
SIM_OS_SOCKET_UDP_DESTROY_FUN       SIM_OS_MAC(osSocketUdpDestroy)   = FORCE_CAST fatalErrorBind;
SIM_OS_SOCKET_CREATE_ADDR_FUN       SIM_OS_MAC(osSocketCreateAddr)   = FORCE_CAST fatalErrorBind;
SIM_OS_SOCKET_DESTROY_ADDR_FUN      SIM_OS_MAC(osSocketDestroyAddr)  = FORCE_CAST fatalErrorBind;
SIM_OS_SOCKET_BIND_FUN              SIM_OS_MAC(osSocketBind)         = FORCE_CAST fatalErrorBind;
SIM_OS_SOCKET_LISTEN_FUN            SIM_OS_MAC(osSocketListen)       = FORCE_CAST fatalErrorBind;
SIM_OS_SOCKET_ACCEPT_FUN            SIM_OS_MAC(osSocketAccept)       = FORCE_CAST fatalErrorBind;
SIM_OS_SOCKET_CONNECT_FUN           SIM_OS_MAC(osSocketConnect)      = FORCE_CAST fatalErrorBind;
SIM_OS_SOCKET_SET_NON_BLOCK_FUN     SIM_OS_MAC(osSocketSetNonBlock)  = FORCE_CAST fatalErrorBind;
SIM_OS_SOCKET_SET_BLOCK_FUN         SIM_OS_MAC(osSocketSetBlock)     = FORCE_CAST fatalErrorBind;
SIM_OS_SOCKET_SEND_FUN              SIM_OS_MAC(osSocketSend)         = FORCE_CAST fatalErrorBind;
SIM_OS_SOCKET_SEND_TO_FUN           SIM_OS_MAC(osSocketSendTo)       = FORCE_CAST fatalErrorBind;
SIM_OS_SOCKET_RECV_FUN              SIM_OS_MAC(osSocketRecv)         = FORCE_CAST fatalErrorBind;
SIM_OS_SOCKET_RECV_FROM_FUN         SIM_OS_MAC(osSocketRecvFrom)     = FORCE_CAST fatalErrorBind;
SIM_OS_SOCKET_SET_SOCKET_NO_LINGER_FUN SIM_OS_MAC(osSocketSetSocketNoLinger)  = FORCE_CAST fatalErrorBind;
SIM_OS_SOCKET_EXTRACT_IP_ADDR_FROM_SOCKET_ADDR_FUN SIM_OS_MAC(osSocketExtractIpAddrFromSocketAddr)  = FORCE_CAST fatalErrorBind;
SIM_OS_SOCKET_GET_SOCKET_ADDR_SIZE_FUN SIM_OS_MAC(osSocketGetSocketAddrSize)  = FORCE_CAST fatalErrorBind;
SIM_OS_SOCKET_SHUT_DOWN_FUN         SIM_OS_MAC(osSocketShutDown)      = FORCE_CAST fatalErrorBind;

SIM_OS_SELECT_CREATE_SET_FUN        SIM_OS_MAC(osSelectCreateSet)     = FORCE_CAST fatalErrorBind;
SIM_OS_SELECT_ERASE_SET_FUN         SIM_OS_MAC(osSelectEraseSet)      = FORCE_CAST fatalErrorBind;
SIM_OS_SELECT_ZERO_SET_FUN          SIM_OS_MAC(osSelectZeroSet)       = FORCE_CAST fatalErrorBind;
SIM_OS_SELECT_ADD_FD_TO_SET_FUN     SIM_OS_MAC(osSelectAddFdToSet)    = FORCE_CAST fatalErrorBind;
SIM_OS_SELECT_CLEAR_FD_FROM_SET_FUN SIM_OS_MAC(osSelectClearFdFromSet) = FORCE_CAST fatalErrorBind;
SIM_OS_SELECT_IS_FD_SET_FUN         SIM_OS_MAC(osSelectIsFdSet)        = FORCE_CAST fatalErrorBind;
SIM_OS_SELECT_COPY_SET_FUN          SIM_OS_MAC(osSelectCopySet)        = FORCE_CAST fatalErrorBind;
SIM_OS_SELECT_FUN                   SIM_OS_MAC(osSelect)               = FORCE_CAST fatalErrorBind;
SIM_OS_SOCKET_GET_SOCKET_FD_SET_SIZE_FUN SIM_OS_MAC(osSocketGetSocketFdSetSize) = FORCE_CAST fatalErrorBind;

SIM_OS_SOCKET_SET_SOCKET_NO_DELAY_FUN SIM_OS_MAC(simOsSocketSetSocketNoDelay) = FORCE_CAST fatalErrorBind;

SIM_OS_TASK_CREATE_FUN           SIM_OS_MAC(simOsTaskCreate)                  = FORCE_CAST fatalErrorBind;
SIM_OS_TASK_DELETE_FUN           SIM_OS_MAC(simOsTaskDelete)                  = FORCE_CAST fatalErrorBind;
SIM_OS_TASK_OWN_TASK_PURPOSE_SET_FUN SIM_OS_MAC(simOsTaskOwnTaskPurposeSet)   = FORCE_CAST fatalErrorBind;
SIM_OS_TASK_OWN_TASK_PURPOSE_GET_FUN SIM_OS_MAC(simOsTaskOwnTaskPurposeGet)   = FORCE_CAST fatalErrorBind;
SIM_OS_SLEEP_FUN                 SIM_OS_MAC(simOsSleep)                       = FORCE_CAST fatalErrorBind;
SIM_OS_TICK_GET_FUN              SIM_OS_MAC(simOsTickGet)                     = FORCE_CAST fatalErrorBind;
SIM_OS_ABORT_FUN                 SIM_OS_MAC(simOsAbort)                       = FORCE_CAST fatalErrorBind;
#ifndef APPLICATION_SIDE_ONLY
SIM_OS_LAUNCH_APPLICATION_FUN    SIM_OS_MAC(simOsLaunchApplication)           = FORCE_CAST fatalErrorBind;
#endif /*!APPLICATION_SIDE_ONLY*/
SIM_OS_BACK_TRACE                SIM_OS_MAC(simOsBacktrace)                   = FORCE_CAST fatalErrorBind;


SIM_OS_SEM_CREATE_FUN                  SIM_OS_MAC(simOsSemCreate)             = FORCE_CAST fatalErrorBind;
SIM_OS_SEM_DELETE_FUN                  SIM_OS_MAC(simOsSemDelete)             = FORCE_CAST fatalErrorBind;
SIM_OS_SEM_WAIT_FUN                    SIM_OS_MAC(simOsSemWait)               = FORCE_CAST fatalErrorBind;
SIM_OS_SEM_SIGNAL_FUN                  SIM_OS_MAC(simOsSemSignal)             = FORCE_CAST fatalErrorBind;
SIM_OS_MUTEX_CREATE_FUN                SIM_OS_MAC(simOsMutexCreate)           = FORCE_CAST fatalErrorBind;
SIM_OS_MUTEX_DELETE_FUN                SIM_OS_MAC(simOsMutexDelete)           = FORCE_CAST fatalErrorBind;
SIM_OS_MUTEX_UNLOCK_FUN                SIM_OS_MAC(simOsMutexUnlock)           = FORCE_CAST fatalErrorBind;
SIM_OS_MUTEX_LOCK_FUN                  SIM_OS_MAC(simOsMutexLock)             = FORCE_CAST fatalErrorBind;
SIM_OS_EVENT_CREATE_FUN                SIM_OS_MAC(simOsEventCreate)           = FORCE_CAST fatalErrorBind;
SIM_OS_EVENT_SET_FUN                   SIM_OS_MAC(simOsEventSet)              = FORCE_CAST fatalErrorBind;
SIM_OS_EVENT_WAIT_FUN                  SIM_OS_MAC(simOsEventWait)             = FORCE_CAST fatalErrorBind;
#ifndef APPLICATION_SIDE_ONLY
SIM_OS_SEND_DATA_TO_VISUAL_ASIC_FUN    SIM_OS_MAC(simOsSendDataToVisualAsic)  = FORCE_CAST fatalErrorBind;
SIM_OS_TIME_FUN                        SIM_OS_MAC(simOsTime)                  = FORCE_CAST fatalErrorBind;


SIM_OS_SLAN_BIND_FUN              SIM_OS_MAC(simOsSlanBind)                   = FORCE_CAST fatalErrorBind;
SIM_OS_SLAN_TRANSMIT_FUN          SIM_OS_MAC(simOsSlanTransmit)               = FORCE_CAST fatalErrorBind;
SIM_OS_SLAN_UNBIND_FUN            SIM_OS_MAC(simOsSlanUnbind)                 = FORCE_CAST fatalErrorBind;
SIM_OS_SLAN_INIT_FUN              SIM_OS_MAC(simOsSlanInit)                   = FORCE_CAST fatalErrorBind;
SIM_OS_SLAN_START_FUN             SIM_OS_MAC(simOsSlanStart)                  = FORCE_CAST fatalErrorBind;
SIM_OS_CHANGE_LINK_STATUS_FUN     SIM_OS_MAC(simOsChangeLinkStatus)           = FORCE_CAST fatalErrorBind;
#endif /*!APPLICATION_SIDE_ONLY*/


#ifndef DEVICES_SIDE_ONLY
SIM_OS_INTERRUPT_SET_FUN      SIM_OS_MAC(simOsInterruptSet)                   = FORCE_CAST fatalErrorBind;
SIM_OS_INIT_INTERRUPT_FUN     SIM_OS_MAC(simOsInitInterrupt)                  = FORCE_CAST fatalErrorBind;
#endif /*!DEVICES_SIDE_ONLY*/

SIM_OS_GET_CNF_VALUE_FUN       SIM_OS_MAC(simOsGetCnfValue)                   = FORCE_CAST fatalErrorBind;
SIM_OS_SET_CNF_FILE_FUN        SIM_OS_MAC(simOsSetCnfFile)                    = FORCE_CAST fatalErrorBind;

SIM_OS_GET_COMMAND_LINE_FUN      SIM_OS_MAC(simOsGetCommandLine)              = FORCE_CAST fatalErrorBind;
SIM_OS_ALLOC_CONSOLE_FUN         SIM_OS_MAC(simOsAllocConsole)                = FORCE_CAST fatalErrorBind;
SIM_OS_SET_CONSOLE_TITLE_FUN     SIM_OS_MAC(simOsSetConsoleTitle)             = FORCE_CAST fatalErrorBind;


SIM_OS_SHARED_MEM_GET_FUN           SIM_OS_MAC(simOsSharedMemGet)             = FORCE_CAST fatalErrorBind;
SIM_OS_SHARED_MEM_ATTACH_FUN        SIM_OS_MAC(simOsSharedMemAttach)          = FORCE_CAST fatalErrorBind;

SIM_OS_PROCESS_ID_GET_FUN           SIM_OS_MAC(simOsProcessIdGet)             = FORCE_CAST fatalErrorBind;
SIM_OS_PROCESS_NOTIFY_FUN           SIM_OS_MAC(simOsProcessNotify)            = FORCE_CAST fatalErrorBind;
SIM_OS_PROCESS_HANDLER_FUN          SIM_OS_MAC(simOsProcessHandler)           = FORCE_CAST fatalErrorBind;



/*******************************************************************************
* dummyBind
*
* DESCRIPTION:
*       function to do nothing , if there was no bind by application .
* INPUTS:
*       none
*
* OUTPUTS:
*       none
*
* RETURNS:
*       none
*
* COMMENTS:
*
*
*******************************************************************************/
static void dummyBind(
    void
)
{
    return;
}

/*******************************************************************************
* fatalErrorBind
*
* DESCRIPTION:
*       function to fatal error , if there was no bind by application .
* INPUTS:
*       none
*
* OUTPUTS:
*       none
*
* RETURNS:
*       none
*
* COMMENTS:
*
*
*******************************************************************************/
static void fatalErrorBind(
    void
)
{
    static GT_U32   depth = 0;

    if(depth > 5)
    {
        /* it seems that we in recursive call from skernelFatalError ! */
        exit(0);
    }

    depth++;

    skernelFatalError(" fatalErrorBind : no bind for current function pointer\n");

    depth--;
}

/*******************************************************************************
* simOsFuncBind
*
* DESCRIPTION:
*       bind the Simulation with OS functions.
* INPUTS:
*       osFuncBindPtr - (pointer to) set of OS call back functions
*
* OUTPUTS:
*       none
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on error
*
* COMMENTS:
*       function must be called by application before simulationLibInit()
*
*******************************************************************************/
GT_STATUS   simOsFuncBind(
    IN SIM_OS_FUNC_BIND_STC        *osFuncBindPtr
)
{
    BIND_FUNC(osWrapperOpen);
    BIND_FUNC(osSerialDestroySerial);

    BIND_LEVEL_FUNC(tasks,simOsTaskCreate);
    BIND_LEVEL_FUNC(tasks,simOsTaskDelete);
    BIND_LEVEL_FUNC(tasks,simOsTaskOwnTaskPurposeSet);
    BIND_LEVEL_FUNC(tasks,simOsTaskOwnTaskPurposeGet);
    BIND_LEVEL_FUNC(tasks,simOsSleep);
    BIND_LEVEL_FUNC(tasks,simOsTickGet);
    BIND_LEVEL_FUNC(tasks,simOsAbort);
#ifndef APPLICATION_SIDE_ONLY
    BIND_LEVEL_FUNC(tasks,simOsLaunchApplication);
#endif /*!APPLICATION_SIDE_ONLY*/
    BIND_LEVEL_FUNC(tasks,simOsBacktrace);

    BIND_LEVEL_FUNC(sync,simOsSemCreate);
    BIND_LEVEL_FUNC(sync,simOsSemDelete);
    BIND_LEVEL_FUNC(sync,simOsSemWait);
    BIND_LEVEL_FUNC(sync,simOsSemSignal);
    BIND_LEVEL_FUNC(sync,simOsMutexCreate);
    BIND_LEVEL_FUNC(sync,simOsMutexDelete);
    BIND_LEVEL_FUNC(sync,simOsMutexUnlock);
    BIND_LEVEL_FUNC(sync,simOsMutexLock);
    BIND_LEVEL_FUNC(sync,simOsEventCreate);
    BIND_LEVEL_FUNC(sync,simOsEventSet);
    BIND_LEVEL_FUNC(sync,simOsEventWait);
#ifndef APPLICATION_SIDE_ONLY
    BIND_LEVEL_FUNC(sync,simOsSendDataToVisualAsic);
    BIND_LEVEL_FUNC(sync,simOsTime);

    BIND_LEVEL_FUNC(slan,simOsSlanBind);
    BIND_LEVEL_FUNC(slan,simOsSlanTransmit);
    BIND_LEVEL_FUNC(slan,simOsSlanUnbind);
    BIND_LEVEL_FUNC(slan,simOsSlanInit);
    BIND_LEVEL_FUNC(slan,simOsSlanStart);
    BIND_LEVEL_FUNC(slan,simOsChangeLinkStatus);
#endif /*!APPLICATION_SIDE_ONLY*/

#ifndef DEVICES_SIDE_ONLY
    BIND_LEVEL_FUNC(interrupts,simOsInterruptSet);
    BIND_LEVEL_FUNC(interrupts,simOsInitInterrupt);
#endif /*!DEVICES_SIDE_ONLY*/

    BIND_LEVEL_FUNC(iniFile,simOsGetCnfValue);
    BIND_LEVEL_FUNC(iniFile,simOsSetCnfFile);

    BIND_LEVEL_FUNC(console,simOsGetCommandLine);
    BIND_LEVEL_FUNC(console,simOsAllocConsole);
    BIND_LEVEL_FUNC(console,simOsSetConsoleTitle);

    BIND_LEVEL_FUNC(sockets,osSocketTcpCreate);
    BIND_LEVEL_FUNC(sockets,osSocketUdpCreate);
    BIND_LEVEL_FUNC(sockets,osSocketTcpDestroy);
    BIND_LEVEL_FUNC(sockets,osSocketUdpDestroy);
    BIND_LEVEL_FUNC(sockets,osSocketCreateAddr);
    BIND_LEVEL_FUNC(sockets,osSocketDestroyAddr);
    BIND_LEVEL_FUNC(sockets,osSocketBind);
    BIND_LEVEL_FUNC(sockets,osSocketListen);
    BIND_LEVEL_FUNC(sockets,osSocketAccept);
    BIND_LEVEL_FUNC(sockets,osSocketConnect);
    BIND_LEVEL_FUNC(sockets,osSocketSetNonBlock);
    BIND_LEVEL_FUNC(sockets,osSocketSetBlock);
    BIND_LEVEL_FUNC(sockets,osSocketSend);
    BIND_LEVEL_FUNC(sockets,osSocketSendTo);
    BIND_LEVEL_FUNC(sockets,osSocketRecv);
    BIND_LEVEL_FUNC(sockets,osSocketRecvFrom);
    BIND_LEVEL_FUNC(sockets,osSocketSetSocketNoLinger);
    BIND_LEVEL_FUNC(sockets,osSocketExtractIpAddrFromSocketAddr);
    BIND_LEVEL_FUNC(sockets,osSocketGetSocketAddrSize);
    BIND_LEVEL_FUNC(sockets,osSocketShutDown);

    BIND_LEVEL_FUNC(sockets,osSelectCreateSet);
    BIND_LEVEL_FUNC(sockets,osSelectEraseSet);
    BIND_LEVEL_FUNC(sockets,osSelectZeroSet);
    BIND_LEVEL_FUNC(sockets,osSelectAddFdToSet);
    BIND_LEVEL_FUNC(sockets,osSelectClearFdFromSet);
    BIND_LEVEL_FUNC(sockets,osSelectIsFdSet);
    BIND_LEVEL_FUNC(sockets,osSelectCopySet);
    BIND_LEVEL_FUNC(sockets,osSelect);
    BIND_LEVEL_FUNC(sockets,osSocketGetSocketFdSetSize);

    BIND_LEVEL_FUNC(sockets,simOsSocketSetSocketNoDelay);

    BIND_LEVEL_FUNC(processes,simOsSharedMemGet);
    BIND_LEVEL_FUNC(processes,simOsSharedMemAttach);
    BIND_LEVEL_FUNC(processes,simOsProcessIdGet);
    BIND_LEVEL_FUNC(processes,simOsProcessNotify);
    BIND_LEVEL_FUNC(processes,simOsProcessHandler);

    return GT_OK;
}

