/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* simOsBindTask.h
*
* DESCRIPTION:
*       Operating System wrapper. Task facility.
*
*       simOsTaskCreate            SIM_OS_TASK_CREATE_FUN
*       simOsTaskDelete            SIM_OS_TASK_DELETE_FUN
*       simOsTaskOwnTaskPurposeSet SIM_OS_TASK_OWN_TASK_PURPOSE_SET_FUN
*       simOsTaskOwnTaskPurposeGet SIM_OS_TASK_OWN_TASK_PURPOSE_GET_FUN
*       simOsSleep                 SIM_OS_SLEEP_FUN
*       simOsTickGet               SIM_OS_TICK_GET_FUN
*       simOsAbort                 SIM_OS_ABORT_FUN
*       simOsLaunchApplication     SIM_OS_LAUNCH_APPLICATION_FUN
*
* FILE REVISION NUMBER:
*       $Revision: 5 $
*******************************************************************************/

#ifndef __simOsBindTaskh
#define __simOsBindTaskh

/************* Includes *******************************************************/
#ifdef __cplusplus
extern "C" {
#endif

#include <os/simTypes.h>

/************ Defines  ********************************************************/
#ifndef __TASKCONV
    #ifdef WIN32
        #define __TASKCONV __stdcall
    #else
        #define __TASKCONV
    #endif
#endif

#ifndef SIM_OS_MAC
/* macro to convert pss/cpss OS function name to one of OS simulation */
#define SIM_OS_MAC(funcName)   sim_##funcName
#endif

/************* Typedefs *******************************************************/

typedef GT_UINTPTR  GT_TASK_ID;

typedef void * GT_TASK_HANDLE;

typedef enum {
    GT_TASK_PRIORITY_ABOVE_NORMAL  = 1,
    GT_TASK_PRIORITY_BELOW_NORMAL  = 2,
    GT_TASK_PRIORITY_HIGHEST       = 3,
    GT_TASK_PRIORITY_IDLE          = 4,
    GT_TASK_PRIORITY_LOWEST        = 5,
    GT_TASK_PRIORITY_NORMAL        = 6,
    GT_TASK_PRIORITY_TIME_CRITICAL = 7

}GT_TASK_PRIORITY_ENT;

/* task type enum */
typedef enum{
    SIM_OS_TASK_PURPOSE_TYPE_CPU_APPLICATION_E,
    SIM_OS_TASK_PURPOSE_TYPE_CPU_ISR_E,
    SIM_OS_TASK_PURPOSE_TYPE_PP_AGING_DAEMON_E,
    SIM_OS_TASK_PURPOSE_TYPE_PP_PIPE_PROCESSING_DAEMON_E,
    SIM_OS_TASK_PURPOSE_TYPE_PP_PIPE_SDMA_QUEUE_DAEMON_E,
    SIM_OS_TASK_PURPOSE_TYPE_PP_PIPE_OAM_KEEP_ALIVE_DAEMON_E,

    SIM_OS_TASK_PURPOSE_TYPE_PP_PIPE_GENERAL_PURPOSE_E,/* general purpose processing */
    SIM_OS_TASK_PURPOSE_TYPE_PP_PIPE_SOFT_RESET_E,/*soft reset*/

    SIM_OS_TASK_PURPOSE_TYPE_INITIALIZATION_E,/* simulation initialization */

    SIM_OS_TASK_PURPOSE_TYPE_PP_PIPE_GENERAL____LAST__E /* must be last */

}SIM_OS_TASK_PURPOSE_TYPE_ENT;

/***********************************************************************/
/*
 * Typedef: struct SIM_OS_TASK_COOKIE_INFO_STC
 *
 * Description:
 *      cookie info that bound to 'thread/task'
 *
 * Fields:
 *      currentPipeId    : since a task can handle only specific device object ,
 *                               it can have only single pipeId that currently used with this device
 *                               relevant to device with numOfPipes != 0.
 *
 * Comments:
 *      such cookie allocated for tasks 'without' cookie
 */
typedef struct{
    GT_U32      currentPipeId;/* since a task can handle only specific device object ,
                                it can have only single pipeId that currently used with this device
                                relevant to device with numOfPipes != 0
                                */

    GT_BOOL     additionalInfo;/* do we have additional info by the caller after this info */
}SIM_OS_TASK_COOKIE_INFO_STC;



/************* Functions ******************************************************/
/*******************************************************************************
* SIM_OS_TASK_CREATE_FUN
*
* DESCRIPTION:
*       Create OS Task/Thread and start it.
*
* INPUTS:
*       prio    - task priority 1 - 64 => HIGH
*       start_addr - task Function to execute
*       arglist    - pointer to list of parameters for task function
*
* OUTPUTS:
*
* RETURNS:
*       GT_TASK_HAND   - handler to the task
*
* COMMENTS:
*       None
*
*******************************************************************************/
typedef GT_TASK_HANDLE (*SIM_OS_TASK_CREATE_FUN)
(
    IN  GT_TASK_PRIORITY_ENT prio,
    IN  unsigned (__TASKCONV *startaddrPtr)(void*),
    IN  void    *arglistPtr
);


/*******************************************************************************
* SIM_OS_TASK_DELETE_FUN
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
typedef GT_STATUS (*SIM_OS_TASK_DELETE_FUN)
(
    IN GT_TASK_HANDLE hThread
);

/*******************************************************************************
* SIM_OS_SLEEP_FUN
*
* DESCRIPTION:
*       Puts current task to sleep for specified number of millisecond.
*
* INPUTS:
*       mils - time to sleep in milliseconds
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
typedef void (*SIM_OS_SLEEP_FUN)
(
    IN GT_U32 mils
);

/*******************************************************************************
* SIM_OS_TICK_GET_FUN
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
typedef GT_U32 (*SIM_OS_TICK_GET_FUN)
(
    void
);

/*******************************************************************************
* SIM_OS_ABORT_FUN
*
* DESCRIPTION:
*       Perform Warm start up on operational mode and halt in debug mode.
*
* INPUTS:
*       None
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
typedef void (*SIM_OS_ABORT_FUN)
(
    void
);

/*******************************************************************************
* SIM_OS_LAUNCH_APPLICATION_FUN
*
* DESCRIPTION:
*       launch application
*
* INPUTS:
*       None
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
typedef void (*SIM_OS_LAUNCH_APPLICATION_FUN)
(
    char * fileName
);

/*******************************************************************************
* SIM_OS_BACK_TRACE
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
typedef GT_U32 (*SIM_OS_BACK_TRACE)
(
     IN  GT_U32   maxFramesPrint,
     OUT GT_CHAR *buffer,
     IN  GT_U32   bufferLen
);

/*******************************************************************************
* SIM_OS_TASK_OWN_TASK_PURPOSE_SET_FUN
*
* DESCRIPTION:
*       Sets type of the thread.
*
* INPUTS:
*       type       - task type
*       cookiePtr  - cookie pointer
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK      - success
*       GT_FAIL    - fail, should never happen
*
* COMMENTS:
*       None
*
*******************************************************************************/
typedef GT_STATUS (*SIM_OS_TASK_OWN_TASK_PURPOSE_SET_FUN)
(
    IN SIM_OS_TASK_PURPOSE_TYPE_ENT      type,
    IN SIM_OS_TASK_COOKIE_INFO_STC*      cookiePtr
);

/*******************************************************************************
* SIM_OS_TASK_OWN_TASK_PURPOSE_GET_FUN
*
* DESCRIPTION:
*       Gets type of the thread.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       type       - task type
*
* RETURNS:
*       GT_OK      - success
*       GT_FAIL    - fail, should never happen
*
* COMMENTS:
*       None
*
*******************************************************************************/
typedef GT_STATUS (*SIM_OS_TASK_OWN_TASK_PURPOSE_GET_FUN)
(
    OUT SIM_OS_TASK_PURPOSE_TYPE_ENT   *type
);

/* SIM_OS_FUNC_BIND_TASK_STC -
*    structure that hold the "os task" functions needed be bound to SIM.
*
*/
typedef struct{
    SIM_OS_TASK_CREATE_FUN               simOsTaskCreate;
    SIM_OS_TASK_DELETE_FUN               simOsTaskDelete;
    SIM_OS_TASK_OWN_TASK_PURPOSE_SET_FUN simOsTaskOwnTaskPurposeSet;
    SIM_OS_TASK_OWN_TASK_PURPOSE_GET_FUN simOsTaskOwnTaskPurposeGet;
    SIM_OS_SLEEP_FUN                     simOsSleep;
    SIM_OS_TICK_GET_FUN                  simOsTickGet;
    SIM_OS_ABORT_FUN                     simOsAbort;
    SIM_OS_LAUNCH_APPLICATION_FUN        simOsLaunchApplication; /* needed only on devices side */
    SIM_OS_BACK_TRACE                    simOsBacktrace;
}SIM_OS_FUNC_BIND_TASK_STC;

extern    SIM_OS_TASK_CREATE_FUN               SIM_OS_MAC(simOsTaskCreate);
extern    SIM_OS_TASK_DELETE_FUN               SIM_OS_MAC(simOsTaskDelete);
extern    SIM_OS_TASK_OWN_TASK_PURPOSE_SET_FUN SIM_OS_MAC(simOsTaskOwnTaskPurposeSet);
extern    SIM_OS_TASK_OWN_TASK_PURPOSE_GET_FUN SIM_OS_MAC(simOsTaskOwnTaskPurposeGet);
extern    SIM_OS_SLEEP_FUN                     SIM_OS_MAC(simOsSleep);
extern    SIM_OS_TICK_GET_FUN                  SIM_OS_MAC(simOsTickGet);
extern    SIM_OS_ABORT_FUN                     SIM_OS_MAC(simOsAbort);
#ifndef APPLICATION_SIDE_ONLY
extern    SIM_OS_LAUNCH_APPLICATION_FUN        SIM_OS_MAC(simOsLaunchApplication);
#endif /*!APPLICATION_SIDE_ONLY*/
extern    SIM_OS_BACK_TRACE                    SIM_OS_MAC(simOsBacktrace);

#ifdef __cplusplus
}
#endif

#endif  /* __simOsBindTaskh */

