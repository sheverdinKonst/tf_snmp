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

#include <vxWorks.h>
#include <sockLib.h>
#include <netinet/tcp.h>
#include <ioLib.h>
#include <inetLib.h>
#include <errnoLib.h>
#include <time.h>

#define EXPLICIT_INCLUDE_TO_SIM_OS_H_FILES
#include <asicSimulation/SInit/sinit.h>
#include <os/simOsBind/simOsBindSync.h>
#include <os/simOsTask.h>


typedef GT_U32  GT_TASK;
GT_U32 osTickGet(void);
GT_STATUS osTimerWkAfter(    IN GT_U32 mils);
GT_U32 osTime(void);
GT_STATUS osTaskCreate
(
    IN  char    *name,
    IN  GT_U32  prio,
    IN  GT_U32  stack,
    IN  unsigned (__TASKCONV *start_addr)(void*),
    IN  void    *arglist,
    OUT GT_TASK *tid
);
GT_STATUS osTaskDelete
(
    IN GT_TASK tid
);
extern  void SHOSTG_abort(void);
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
    GT_TASK                     retHThread;
    GT_U32                      thredPriority;
    static GT_U32                taskNum=0;
    GT_U8  taskName[16];

    sprintf(taskName, "simTask%ld", taskNum);

    /* setting the priority of the thread*/
    switch(prio)
    {
        case GT_TASK_PRIORITY_TIME_CRITICAL:
            thredPriority = 50;
            break;
        case GT_TASK_PRIORITY_HIGHEST:
            thredPriority = 55;
            break;
        case GT_TASK_PRIORITY_ABOVE_NORMAL:
            thredPriority = 75;
            break;
        case GT_TASK_PRIORITY_NORMAL:
            thredPriority = 100;
            break;
        case GT_TASK_PRIORITY_BELOW_NORMAL:
            thredPriority = 125;
            break;
        case GT_TASK_PRIORITY_LOWEST:
            thredPriority = 200;
            break;
        case GT_TASK_PRIORITY_IDLE:
            thredPriority = 255;
            break;
        default:
            thredPriority = 100;
            break;
    }

    if (osTaskCreate(taskName, thredPriority, 0x8000, startaddrPtr, arglistPtr, &retHThread)
        != GT_OK)
    {
        return 0;
    }

    /* increment task counter */
    taskNum++;

    return (GT_TASK_HANDLE)retHThread;
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
    return osTickGet();
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
    FUNCTION_NOT_SUPPORTED(simOsLaunchApplication);
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

#if 0
extern int vxWorksSocketNoDelay;/*defined in cpssEnabler ,
    make sure that every one that take this C file will define this variable....*/
#endif /*0*/

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

    ret = setsockopt (socketFd, IPPROTO_TCP, TCP_NODELAY, (char*)&noDelay,
                    sizeof (noDelay));
    if(ret != 0)
    {
        return GT_FAIL;
    }

#if 0
    vxWorksSocketNoDelay = 1;
#endif/*0*/

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
    return 0;
}
