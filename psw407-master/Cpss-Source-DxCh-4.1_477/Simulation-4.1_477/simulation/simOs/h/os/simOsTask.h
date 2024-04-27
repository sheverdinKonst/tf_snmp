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
* simOsTask.h
*
* DESCRIPTION:
*       Operating System wrapper. Task facility.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 4 $
*******************************************************************************/

#ifndef EXPLICIT_INCLUDE_TO_SIM_OS_H_FILES
    #error "include to those H files should be only for bind purposes"
#endif /*!EXPLICIT_INCLUDE_TO_SIM_OS_H_FILES*/

#ifndef __simOsTaskh
#define __simOsTaskh

/************* Includes *******************************************************/
#include <os/simTypes.h>


#ifdef __cplusplus
extern "C" {
#endif


/*******************************************************************************
* simOsTaskCreate
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
GT_TASK_HANDLE simOsTaskCreate
(
    IN  GT_TASK_PRIORITY_ENT prio,
    IN  unsigned (__TASKCONV *startaddrPtr)(void*),
    IN  void    *arglistPtr
);


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
);

/*******************************************************************************
* simOsSleep
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
void simOsSleep
(
    IN GT_U32 mils
);

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
);

/*******************************************************************************
* simOsAbort
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
void simOsAbort
(
    void
);

/*******************************************************************************
* simOsLaunchApplication
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
void simOsLaunchApplication
(
    char * fileName
);

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
);

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
);


#ifdef __cplusplus
}
#endif

#endif  /* __simOsTaskh */

