/*******************************************************************************
*              Copyright 2001, GALILEO TECHNOLOGY, LTD.
*
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL. NO RIGHTS ARE GRANTED
* HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT OF MARVELL OR ANY THIRD
* PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE DISCRETION TO REQUEST THAT THIS
* CODE BE IMMEDIATELY RETURNED TO MARVELL. THIS CODE IS PROVIDED "AS IS".
* MARVELL MAKES NO WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS
* ACCURACY, COMPLETENESS OR PERFORMANCE. MARVELL COMPRISES MARVELL TECHNOLOGY
* GROUP LTD. (MTGL) AND ITS SUBSIDIARIES, MARVELL INTERNATIONAL LTD. (MIL),
* MARVELL TECHNOLOGY, INC. (MTI), MARVELL SEMICONDUCTOR, INC. (MSI), MARVELL
* ASIA PTE LTD. (MAPL), MARVELL JAPAN K.K. (MJKK), GALILEO TECHNOLOGY LTD. (GTL)
* AND GALILEO TECHNOLOGY, INC. (GTI).
********************************************************************************
* simOsLinuxTask.c
*
* DESCRIPTION:
*       Linux Operating System Simulation. Task facility implementation.
*
* DEPENDENCIES:
*       CPU independed.
*
* FILE REVISION NUMBER:
*       $Revision: 15 $
*******************************************************************************/

#include <time.h>
#include <assert.h>
#include <os/simTypes.h>
#include <os/simTypesBind.h>
#define EXPLICIT_INCLUDE_TO_SIM_OS_H_FILES
#include <os/simOsTask.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#ifdef  LINUX
#include <execinfo.h>
#include <stdlib.h>
#include <string.h>
#endif

#include <os/simTypes.h>
#include <os/simOsTask.h>

#ifdef SHARED_MEMORY
    /* call LIBC free() to free memory allocated by backtrace_symbols() */
    void __libc_free(void *ptr);
#define free   __libc_free
#endif

extern GT_STATUS osTaskCreate
(
    IN  char    *name,
    IN  GT_U32  prio,
    IN  GT_U32  stack,
    IN  unsigned (__TASKCONV *start_addr)(void*),
    IN  void    *arglist,
    OUT GT_TASK_ID *tid
);

extern GT_STATUS osTaskDelete
(
    IN GT_TASK_ID tid
);

extern GT_STATUS osTimerWkAfter
(
        IN GT_U32 mils
);

extern GT_U32 osTickGet(void);

extern GT_STATUS osTimeRT
(
    OUT GT_U32  *seconds,
    OUT GT_U32  *nanoSeconds
);

/************* Defines ***********************************************/

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
    static GT_U32                taskNum=0;
    GT_CHAR  taskName[16];

    sprintf(taskName, "task%d", taskNum);

    /* setting the priority of the thread*/
    switch(prio)
    {
        case   GT_TASK_PRIORITY_ABOVE_NORMAL:
        thredPriority = 160;
                break;
        case    GT_TASK_PRIORITY_BELOW_NORMAL:
                thredPriority = 180;
                break;
        case    GT_TASK_PRIORITY_HIGHEST:
                thredPriority = 150;
                break;
        case    GT_TASK_PRIORITY_IDLE:
                thredPriority = 200;
                break;
        case    GT_TASK_PRIORITY_LOWEST:
                thredPriority = 190;
                break;
        case    GT_TASK_PRIORITY_NORMAL:
                thredPriority = 170;
                break;
        case     GT_TASK_PRIORITY_TIME_CRITICAL:
                thredPriority = 140;
                break;
        default:
                thredPriority = 170;
                break;

    }

    if (osTaskCreate(taskName, thredPriority, 0x40000,
                        startaddrPtr, arglistPtr,
                        (GT_TASK_ID*)(void*)&retHThread) != GT_OK)
    {
        return 0;
    }

    /* increament task counter */
    taskNum++;

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
    return osTaskDelete((GT_TASK_ID)hThread);
}

/*******************************************************************************
* simOsSleep
*
* DESCRIPTION:
*       Puts current task to sleep for specified number of milisecond.
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
    osTimerWkAfter(timeOut);

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
*       The tick counter value in milliseconds.
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
    GT_U32 milliSeconds; /* tick counter in milliseconds */
    GT_U32 seconds;      /* tick counter in seconds */
    GT_U32 nanoSeconds;  /* tick counter in nanoseconds */

    /* the Linux tick is 10 millisecond.
      Use nanosecond counter to get exact number of milliseconds.  */
    osTimeRT(&seconds, &nanoSeconds);

    milliSeconds = (seconds * 1000) + (nanoSeconds / 1000000);

    return milliSeconds;
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
    abort();
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
    if (system(fileName) == 0)
    {
        exit(1);
    }
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
    int        ret;
    int        level = IPPROTO_TCP;

    ret = setsockopt(socketFd, level, TCP_NODELAY, (char*)&noDelay,
                     sizeof (noDelay));
    if(ret != 0)
    {
        return GT_FAIL;
    }

    return GT_OK;
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
#ifdef  LINUX
    void    *arr[100];
    int     nptrs, j, len;
    char    **strings;
    GT_U32  ret = 0;

    nptrs = backtrace(arr, (maxFramesPrint < 100) ? maxFramesPrint+1 : 100);
    strings = backtrace_symbols(arr, nptrs);
    for (j = 1; j < nptrs; j++)
    {
        len = strlen(strings[j]);
        if (len + 1 >= bufferLen - ret)
            len = bufferLen - ret - 1;
        memcpy(buffer + ret, strings[j], len);
        ret += len;
        buffer[ret++] = '\n';
    }
    free(strings);
    if (ret < bufferLen)
        buffer[ret] = 0;
    return ret;

#else    /* !defined(LINUX) */
    return 0;
#endif  /* !defined(LINUX) */
}
