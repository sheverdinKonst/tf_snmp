/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* simOsBindOwn.c
*
* DESCRIPTION:
*       allow bind of the simOs functions to the simulation
*
* FILE REVISION NUMBER:
*       $Revision: 3 $
*******************************************************************************/

#include <os/simTypesBind.h>

/* define next -- must by before any include of next os H files */
#ifndef EXPLICIT_INCLUDE_TO_SIM_OS_H_FILES
#define EXPLICIT_INCLUDE_TO_SIM_OS_H_FILES
#endif

#include <os/simOsBindOwn.h>
#include <os/simOsTask.h>
#include <os/simOsIniFile.h>
#include <os/simOsIntr.h>
#include <os/simOsSlan.h>
#include <os/simOsSync.h>
#include <os/simOsConsole.h>
#include <os/simOsProcess.h>

#define BIND_LEVEL_FUNC(level,funcName)     \
        simOsBindInfo.level.funcName = funcName

/* max tasks num */
#define SIM_OS_TASKS_MAX_NUM 1000

extern GT_STATUS osTaskGetSelf(OUT GT_U32 *tid);

/*
 * Typedef: struct SIM_OS_TASKS_THREAD_TYPE_STC
 *
 * Description:
 *      Describe a tasks info(like task type)
 *
 * Fields:
 *      taskId   - task Id
 *      taskType - task type
 *      cookiePtr  - cookie pointer
 * Comments:
 */
typedef struct{
    GT_U32                       taskId;
    SIM_OS_TASK_PURPOSE_TYPE_ENT taskType;
    GT_PTR                       cookiePtr;
}SIM_OS_TASKS_INFO_STC;

/* running tasks table */
static SIM_OS_TASKS_INFO_STC simOsTasksInfoArr[SIM_OS_TASKS_MAX_NUM] = {{0}};

/*******************************************************************************
* simOsFuncBindOwnSimOs
*
* DESCRIPTION:
*       the functions of simOs will be bound to the simulation
*
* INPUTS:
*       None
*
* OUTPUTS:
*
* RETURNS:
*       none
*
* COMMENTS:
*       None
*
*******************************************************************************/
void simOsFuncBindOwnSimOs
(
    void
)
{
    SIM_OS_FUNC_BIND_STC simOsBindInfo;

    /* reset all fields of simOsBindInfo */
    memset(&simOsBindInfo,0,sizeof(simOsBindInfo));

    BIND_LEVEL_FUNC(sockets,simOsSocketSetSocketNoDelay);

    BIND_LEVEL_FUNC(tasks,simOsTaskCreate);
    BIND_LEVEL_FUNC(tasks,simOsTaskDelete);
    BIND_LEVEL_FUNC(tasks,simOsTaskOwnTaskPurposeSet);
    BIND_LEVEL_FUNC(tasks,simOsTaskOwnTaskPurposeGet);
    BIND_LEVEL_FUNC(tasks,simOsSleep);
    BIND_LEVEL_FUNC(tasks,simOsTickGet);
    BIND_LEVEL_FUNC(tasks,simOsAbort);
#ifndef APPLICATION_SIDE_ONLY
    BIND_LEVEL_FUNC(tasks,simOsLaunchApplication);  /* needed only on devices side */
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
    BIND_LEVEL_FUNC(sync,simOsSendDataToVisualAsic); /* needed only on devices side */
    BIND_LEVEL_FUNC(sync,simOsTime);                 /* needed only on devices side */

    BIND_LEVEL_FUNC(slan,simOsSlanBind);             /* needed only on devices side */
    BIND_LEVEL_FUNC(slan,simOsSlanTransmit);         /* needed only on devices side */
    BIND_LEVEL_FUNC(slan,simOsSlanUnbind);           /* needed only on devices side */
    BIND_LEVEL_FUNC(slan,simOsSlanInit);             /* needed only on devices side */
    BIND_LEVEL_FUNC(slan,simOsSlanStart);            /* needed only on devices side */
    BIND_LEVEL_FUNC(slan,simOsChangeLinkStatus);     /* needed only on devices side */
#endif /*!APPLICATION_SIDE_ONLY*/

#ifndef DEVICES_SIDE_ONLY
    BIND_LEVEL_FUNC(interrupts,simOsInterruptSet);  /* needed only on application side */
    BIND_LEVEL_FUNC(interrupts,simOsInitInterrupt); /* needed only on application side */
#endif /*!DEVICES_SIDE_ONLY*/

    BIND_LEVEL_FUNC(iniFile,simOsGetCnfValue);
    BIND_LEVEL_FUNC(iniFile,simOsSetCnfFile);

    BIND_LEVEL_FUNC(console,simOsGetCommandLine);
    BIND_LEVEL_FUNC(console,simOsAllocConsole);
    BIND_LEVEL_FUNC(console,simOsSetConsoleTitle);

#ifndef DEVICES_SIDE_ONLY
    BIND_LEVEL_FUNC(processes,simOsSharedMemGet);     /* needed only on application side */
    BIND_LEVEL_FUNC(processes,simOsSharedMemAttach);  /* needed only on application side */
    BIND_LEVEL_FUNC(processes,simOsProcessIdGet);     /* needed only on application side */
    BIND_LEVEL_FUNC(processes,simOsProcessNotify);    /* needed only on application side */
    BIND_LEVEL_FUNC(processes,simOsProcessHandler);   /* needed only on application side */
#endif /*!DEVICES_SIDE_ONLY*/

    /* this needed for binding the OS of simulation with our OS functions */
    simOsFuncBind(&simOsBindInfo);
}

/*******************************************************************************
* simOsTaskOwnTaskPurposeSet
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
GT_STATUS simOsTaskOwnTaskPurposeSet
(
    IN SIM_OS_TASK_PURPOSE_TYPE_ENT      type,
    IN SIM_OS_TASK_COOKIE_INFO_STC*      cookiePtr
)
{
    GT_U32  currThread = 0;
    GT_U32  i;

    osTaskGetSelf(&currThread);

    if(0 == currThread)
    {
        return GT_FAIL;
    }

    /* search current thread in the table */
    for(i = 0; i < SIM_OS_TASKS_MAX_NUM; i++)
    {
        if(currThread == simOsTasksInfoArr[i].taskId)
        {
            if(type != SIM_OS_TASK_PURPOSE_TYPE_PP_PIPE_GENERAL____LAST__E)
            {
                simOsTasksInfoArr[i].taskType = type;
            }
            else
            {
                /* allow the caller to modify only the cookie ... */
            }

            if(cookiePtr)
            {
                simOsTasksInfoArr[i].cookiePtr = cookiePtr;
            }
            else
            {
                /* allow the caller to modify only the type of task ... */
            }

            return GT_OK;
        }
    }

    /* add current thread info to the table */
    for(i = 0; i < SIM_OS_TASKS_MAX_NUM; i++)
    {
        if(!simOsTasksInfoArr[i].taskId)
        {
            simOsTasksInfoArr[i].taskId   = currThread;
            simOsTasksInfoArr[i].taskType = type;
            if(cookiePtr)
            {
                simOsTasksInfoArr[i].cookiePtr = cookiePtr;
                cookiePtr->additionalInfo = GT_TRUE;
            }
            else
            {
                cookiePtr =
                    calloc(1,sizeof(SIM_OS_TASK_COOKIE_INFO_STC));
                simOsTasksInfoArr[i].cookiePtr = cookiePtr;
                cookiePtr->additionalInfo = GT_FALSE;
            }
            return GT_OK;
        }
    }

    /* no more space left in the task table */
    return GT_FAIL;
}

/*******************************************************************************
* simOsTaskOwnTaskPurposeGet
*
* DESCRIPTION:
*       Gets type of the thread.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       type         - task type
*
* RETURNS:
*       GT_OK        - success
*       GT_BAD_PTR   - wrong pointer
*       GT_NOT_FOUND - task info not found
*
* COMMENTS:
*       None
*
*******************************************************************************/
GT_STATUS simOsTaskOwnTaskPurposeGet
(
    OUT SIM_OS_TASK_PURPOSE_TYPE_ENT   *type
)
{
    GT_U32 currThread = 0;
    GT_U32 i;

    if(NULL == type)
    {
        return GT_BAD_PTR;
    }

    osTaskGetSelf(&currThread);

    if(0 == currThread)
    {
        return GT_FAIL;
    }

    for(i = 0; i < SIM_OS_TASKS_MAX_NUM; i++)
    {
        if(currThread == simOsTasksInfoArr[i].taskId)
        {
            *type = simOsTasksInfoArr[i].taskType;
            return GT_OK;
        }
    }

    *type = SIM_OS_TASK_PURPOSE_TYPE_CPU_APPLICATION_E;
    return GT_OK;
}

/*******************************************************************************
* simOsTaskOwnTaskCookieGet
*
* DESCRIPTION:
*       Gets 'cookiePtr' of the thread.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       the 'cookiePtr' that was registered via simOsTaskOwnTaskPurposeSet
*
* COMMENTS:
*       None
*
*******************************************************************************/
GT_PTR simOsTaskOwnTaskCookieGet
(
    GT_VOID
)
{
    GT_U32 currThread = 0;
    GT_U32 i;

    osTaskGetSelf(&currThread);

    if(0 == currThread)
    {
        return NULL;
    }

    for(i = 0; i < SIM_OS_TASKS_MAX_NUM; i++)
    {
        if(currThread == simOsTasksInfoArr[i].taskId)
        {
            return simOsTasksInfoArr[i].cookiePtr;
        }
    }

    /* not found the task in the registered list */
    return NULL;
}


