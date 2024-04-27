/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* gmOsTask.h
*
* DESCRIPTION:
*       Operating System wrapper. Task facility.
*
* FILE REVISION NUMBER:
*       $Revision: 1 $
*******************************************************************************/

#ifndef __gmOsTaskh
#define __gmOsTaskh

#ifdef __cplusplus
extern "C" {
#endif

/************* Includes *******************************************************/
#include <os/simTypes.h>
/*
#include <extServices/os/gtOs/gtGenTypes.h>
*/
/************ Defines  ********************************************************/
#ifdef WIN32
#define __TASKCONV __stdcall
#else
#define __TASKCONV
#endif

/************* Typedefs *******************************************************/

typedef GT_U32  GM_TASK;

/*******************************************************************************
* GM_OS_TASK_CREATE_FUNC
*
* DESCRIPTION:
*       Create OS Task/Thread and start it.
*
* INPUTS:
*       name    - task name, string no longer then OS_MAX_TASK_NAME_LENGTH
*       prio    - task priority 1 - 64 => HIGH
*       stack   - task Stack Size
*       start_addr - task Function to execute
*       arglist    - pointer to list of parameters for task function
*
* OUTPUTS:
*       tid   - Task ID
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on error
*
* COMMENTS:
*       None
*
*******************************************************************************/
typedef GT_STATUS (*GM_OS_TASK_CREATE_FUNC)
(
    IN  char      *name,
    IN  GT_U32    prio,
    IN  GT_U32    stack,
    IN  unsigned  (__TASKCONV *start_addr)(void*),
    IN  void      *arglist,
    OUT GM_TASK *tid
);

/*******************************************************************************
* GM_OS_TASK_DELETE_FUNC
*
* DESCRIPTION:
*       Deletes existing task/thread.
*
* INPUTS:
*       tid   - Task ID
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on error
*
* COMMENTS:
*       If tid = 0, delete calling task (itself)
*
*******************************************************************************/
typedef GT_STATUS (*GM_OS_TASK_DELETE_FUNC)
(
    IN GM_TASK tid
);

/*******************************************************************************
* GM_OS_TASK_LOCK_FUNC
*
* DESCRIPTION:
*       Disable task rescheduling of current task.
*
* INPUTS:
*       None
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on error
*
* COMMENTS:
*       None
*
*******************************************************************************/
typedef GT_STATUS (*GM_OS_TASK_LOCK_FUNC) (void);

/*******************************************************************************
* GM_OS_TASK_UNLOCK_FUNC
*
* DESCRIPTION:
*       Enable task rescheduling.
*
* INPUTS:
*       None
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on error
*
* COMMENTS:
*       None
*
*******************************************************************************/
typedef GT_STATUS (*GM_OS_TASK_UNLOCK_FUNC) (void);

/* GM_OS_TASK_BIND_STC -
    structure that hold the "os Task" functions needed be bound to gm.

*/
typedef struct{
    GM_OS_TASK_CREATE_FUNC               osTaskCreateFunc;
    GM_OS_TASK_DELETE_FUNC               osTaskDeleteFunc;
    GM_OS_TASK_LOCK_FUNC                 osTaskLockFunc;
    GM_OS_TASK_UNLOCK_FUNC               osTaskUnLockFunc;
}GM_OS_TASK_BIND_STC;

#ifdef __cplusplus
}
#endif

#endif  /* __gmOsTaskh */
/* Do Not Add Anything Below This Line */

