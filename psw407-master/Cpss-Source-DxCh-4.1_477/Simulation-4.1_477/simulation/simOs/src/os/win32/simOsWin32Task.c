/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* simOsWin32Task.c
*
* DESCRIPTION:
*       Win32 Operating System Simulation. Task facility implementation.
*
* DEPENDENCIES:
*       Win32, CPU independed.
*
* FILE REVISION NUMBER:
*       $Revision: 5 $
*******************************************************************************/

/* WA to avoid next warning :
   due to include to : #include <windows.h>
    c:\program files\microsoft visual studio\vc98\include\rpcasync.h(45) :
    warning C4115: '_RPC_ASYNC_STATE' : named type definition in parentheses
*/
struct _RPC_ASYNC_STATE;
#include <windows.h>
#include <process.h>
#include <time.h>
#include <winbase.h>
#include <assert.h>
#include <os/simTypesBind.h>
#include <common/SHOST/HOST_D/EXP/HOST_D.H>


/************* Defines ***********************************************/

/************* Externs ***********************************************/

int PrintCallStack(char *buffer, int bufferLen, int framesToSkip);

/************ Public Functions ************************************************/

/*******************************************************************************
* simOsTaskCreate
*
* DESCRIPTION:
*       Create OS Task and start it.
*
* INPUTS:
*       prio    - task priority 1 - 64 => HIGH
*       start_addr - task Function to execute
*       arglist    - pointer to list of parameters for task function
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_TASK_HANDLE   - the handle of the thread
*
* COMMENTS:
*       None
*
*******************************************************************************/
GT_TASK_HANDLE simOsTaskCreate
(
    IN  GT_TASK_PRIORITY_ENT prio,
    IN  unsigned (__TASKCONV *startaddrPtr)(void*),
    IN  void    *arglistPtr
)
{
    HANDLE                      retHThread;
    GT_32                       thredPriority;

    /* Create thread and save Handle */
    retHThread = CreateThread(NULL, 0,(LPTHREAD_START_ROUTINE)startaddrPtr,arglistPtr,0,NULL);

    /* A handle to the new thread indicates success. NULL indicates failure. */
    if(retHThread != NULL)
    {

        /* setting the priority of the thread*/
        switch(prio) {
            case   GT_TASK_PRIORITY_ABOVE_NORMAL:
                thredPriority = THREAD_PRIORITY_ABOVE_NORMAL;
                break;
            case    GT_TASK_PRIORITY_BELOW_NORMAL:
                thredPriority = THREAD_PRIORITY_BELOW_NORMAL;
                break;
            case    GT_TASK_PRIORITY_HIGHEST:
                thredPriority = THREAD_PRIORITY_HIGHEST;
                break;
            case    GT_TASK_PRIORITY_IDLE:
                thredPriority = THREAD_PRIORITY_IDLE;
                break;
            case    GT_TASK_PRIORITY_LOWEST:
                thredPriority = THREAD_PRIORITY_LOWEST;
                break;
            case    GT_TASK_PRIORITY_NORMAL:
                thredPriority = THREAD_PRIORITY_NORMAL;
                break;
            case     GT_TASK_PRIORITY_TIME_CRITICAL:
                thredPriority = THREAD_PRIORITY_TIME_CRITICAL;
                break;
            default:
                thredPriority = THREAD_PRIORITY_NORMAL;
                break;
        }

        SetThreadPriority(retHThread,  thredPriority);
    }


    return retHThread;
}

/*******************************************************************************
* simOsTaskDelete
*
* DESCRIPTION:
*       Deletes existing task.
*
* INPUTS:
*       hThread - the handle of the thread
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on error
*
* COMMENTS:
*
*
*******************************************************************************/
GT_STATUS simOsTaskDelete
(
    IN GT_TASK_HANDLE hThread
)
{
    return (TerminateThread(hThread, 0) == 0) ? GT_FAIL : GT_OK;
}

/*******************************************************************************
* simOsSleep
*
* DESCRIPTION:
*       Puts current task to sleep for specified number of millisecond.
*
* INPUTS:
*       timeOut - time to sleep in milliseconds
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
void simOsSleep
(
    IN GT_U32 timeOut
)
{
    Sleep(timeOut);

}

/*******************************************************************************
* simOsTickGet
*
* DESCRIPTION:
*       Gets the value of the kernel's tick counter.
*
* INPUTS:
*       None
*
* OUTPUTS:
*       None
*
* RETURNS:
*       The tick counter value.
*
* COMMENTS:
*       None
*
*******************************************************************************/
GT_U32 simOsTickGet
(
    void
)
{
    clock_t clockT = clock();

    if(clockT==-1)
    {
        return 0;
    }

    return clockT;
}

/*******************************************************************************
* simOsAbort
*
* DESCRIPTION:
*       Perform Warm start up on operational mode and halt in debug mode.
*
* INPUTS:
*       Npne
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
void simOsAbort
(
    void
)
{
    SHOSTG_abort();
}

/*******************************************************************************
* simOsLaunchApplication
*
* DESCRIPTION:
*       launch application
*
* INPUTS:
*       Npne
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
void simOsLaunchApplication
(
    char * fileName
)
{
    WinExec(fileName, 0);
}



/*******************************************************************************
* simOsTime
*
* DESCRIPTION:
*       Gets number of seconds passed since system boot
*
* INPUTS:
*       None
*
* OUTPUTS:
*       None
*
* RETURNS:
*       The second counter value.
*
* COMMENTS:
*       None
*
*******************************************************************************/
GT_U32 simOsTime(void)
{
    time_t t ;
    return (GT_U32) time(&t);
}

/*******************************************************************************
* simOsSocketSetSocketNoDelay()
*
* DESCRIPTION:
*       Set the socket option to be no delay.
*
* INPUTS:
*       socketFd    - Socket descriptor
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on fail
*
* COMMENTS:
*       None
*
*******************************************************************************/
GT_STATUS simOsSocketSetSocketNoDelay
(
    IN  GT_SOCKET_FD     socketFd
)
{
    int        noDelay=1;
    int        ret,err;

    ret = setsockopt (socketFd, IPPROTO_TCP, TCP_NODELAY, (char*)&noDelay,
                    sizeof (noDelay));
    if(ret != 0)
    {
        err = WSAGetLastError();
        return GT_FAIL;
    }

    return GT_OK ;
}

/*******************************************************************************
* simOsBacktrace()
*
* DESCRIPTION:
*      return buffer with backtrace
*
* INPUTS:
*       maxFramesPrint - max number of frames to print
*       bufferLen      - length of buffer
*
* OUTPUTS:
*       buffer       - buffer with backtrace info
*
* RETURNS:
*       buflen       - size of returned data in buffer
*
* COMMENTS:
*       None
*
*******************************************************************************/
GT_U32 simOsBacktrace
(
    IN  GT_U32   maxFramesPrint,
    OUT GT_CHAR *buffer,
    IN  GT_U32   bufferLen
)
{
#ifdef _VISUALC
    GT_U32 buflen = 0;
#ifdef PRINT_CALL_STACK_USED
    buflen = PrintCallStack(buffer, bufferLen, maxFramesPrint);
#endif /* PRINT_CALL_STACK_USED */
    return buflen;
#else
    return 0;
#endif
}

