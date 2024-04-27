/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* simOsWin32Sync.c
*
* DESCRIPTION:
*       Win32 Operating System Simulation. Semaphore facility.
*
* DEPENDENCIES:
*       Win32, CPU independed.
*
* FILE REVISION NUMBER:
*       $Revision: 4 $
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

#include <stdio.h>
#include <assert.h>

#include <os/simTypesBind.h>

#define SIM_OS_STR_NAMED_PIPE_CNS  \
   "\\\\.\\pipe\\PipeLineBetweenWhiteModeAndVisualAsic"

/************ Public Functions ************************************************/

/*******************************************************************************
* osSemBinCreate
*
* DESCRIPTION:
*       Create and initialize a binary semaphore.
*
* INPUTS:
*       init       - init value of semaphore (full or empty)
*       maxCount   - maximal number of semaphores
*
* OUTPUTS:
*
* RETURNS:
*       GT_SEM   - semaphor id
*
* COMMENTS:
*       None
*
*******************************************************************************/

GT_SEM  simOsSemCreate
(
    IN  GT_U32           initCount,
    IN  GT_U32           maxCount
)
{
    GT_SEM returnSem;

    returnSem = (GT_SEM)CreateSemaphore(NULL,       /* no security attributes */
                          (unsigned long)initCount, /* initial count          */
                          (unsigned long)maxCount,  /* maximum count          */
                           NULL);                   /* semaphore name         */

    return returnSem;
}

/*******************************************************************************
* simOsSemDelete
*
* DESCRIPTION:
*       Delete semaphore.
*
* INPUTS:
*       smid - semaphore Id
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
GT_STATUS simOsSemDelete
(
    IN GT_SEM smid
)
{
    if (CloseHandle((HANDLE)smid) == 0)
      return GT_FAIL;

    return GT_OK;
}

/*******************************************************************************
* simOsSemWait
*
* DESCRIPTION:
*       Wait on semaphore.
*
* INPUTS:
*       smid    - semaphore Id
*       timeOut - time out in milliseconds or 0 to wait forever
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on error
*       OS_TIMEOUT - on time out
*
* COMMENTS:
*       None
*
*******************************************************************************/
GT_STATUS simOsSemWait
(
    IN GT_SEM smid,
    IN GT_U32 timeOut
)
{
    if (timeOut == 0)
        timeOut = INFINITE;

    switch (WaitForSingleObject((HANDLE)smid, timeOut))
    {
        case WAIT_FAILED:
            return GT_FAIL;

        case WAIT_TIMEOUT:
            return GT_TIMEOUT;

        case WAIT_OBJECT_0:
            return GT_OK;

        default: break;
    }

  return GT_FAIL;
}

/*******************************************************************************
* simOsSemSignal
*
* DESCRIPTION:
*       Signal a semaphore.
*
* INPUTS:
*       smid    - semaphore Id
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
GT_STATUS simOsSemSignal
(
    IN GT_SEM smid
)
{
    ReleaseSemaphore((HANDLE)smid,1,NULL);

    return GT_OK;
}
/*******************************************************************************
* simOsMutexCreate
*
* DESCRIPTION:
*       Create and initialize mutex (critical section).
*       This object is recursive: the owner task can lock it again without
*       wait. simOsMutexUnlock() must be called for every time that mutex
*       successfully locked
*
* INPUTS:
*
* OUTPUTS:
*
* RETURNS:
*       GT_SEM   - mutex id
*
* COMMENTS:
*       None
*
*******************************************************************************/
GT_MUTEX  simOsMutexCreate
(
    void
)
{
    GT_MUTEX returnSem;

    returnSem = (GT_MUTEX)CreateMutex(NULL, /* no security attributes */
                            FALSE,        /* bInitialOwner          */
                            NULL);        /* semaphore name         */
    return returnSem;
}
/*******************************************************************************
* simOsMutexDelete
*
* DESCRIPTION:
*       Delete mutex.
*
* INPUTS:
*       mid - mutex id
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
void simOsMutexDelete
(
    IN GT_MUTEX mid
)
{
    simOsSemDelete(mid);
}
/*******************************************************************************
* simOsMutexUnlock
*
* DESCRIPTION:
*       Leave critical section.
*
* INPUTS:
*       mid    - mutex id
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
void simOsMutexUnlock
(
    IN GT_MUTEX mid
)
{
    ReleaseMutex((HANDLE)mid);
}
/*******************************************************************************
* simOsMutexLock
*
* DESCRIPTION:
*       Enter a critical section.
*
* INPUTS:
*       mid    - mutex id
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
void simOsMutexLock
(
    IN GT_MUTEX mid
)
{
    simOsSemWait(mid, 0 /* wait forever */);
}

/*******************************************************************************
* simOsEventCreate
*
* DESCRIPTION:
*       Create an event.
*
* INPUTS:
*       None
*
* OUTPUTS:
*
* RETURNS:
*       GT_SEM   - event id
*
* COMMENTS:
*       None
*
*******************************************************************************/
GT_SEM  simOsEventCreate
(
    void
)
{
    GT_SEM retEvent;
    retEvent = (GT_SEM)(CreateEvent(NULL,FALSE,FALSE,NULL));
    return retEvent;
}

/*******************************************************************************
* simOsEventSet
*
* DESCRIPTION:
*       Signal that the event was initialized.
*
* INPUTS:
*       eventHandle - Handle of the event
*
* OUTPUTS:
*
* RETURNS:
*       GT_SEM   - event id
*
* COMMENTS:
*       None
*
*******************************************************************************/
GT_STATUS  simOsEventSet
(
    IN GT_SEM eventHandle
)
{
    if(SetEvent((HANDLE)eventHandle) == 0)
    {
        return GT_FALSE;
    }
    return GT_OK;
}

/*******************************************************************************
* simOsEventWait
*
* DESCRIPTION:
*       Wait on event.
*
* INPUTS:
*       smid    - semaphore Id
*       timeOut - time out in milliseconds or 0 to wait forever
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on error
*       GT_TIMEOUT - on time out
*
* COMMENTS:
*       None
*
*******************************************************************************/
GT_STATUS simOsEventWait
(
    IN GT_SEM emid,
    IN GT_U32 timeOut
)
{

    if (timeOut == SIM_OS_WAIT_FOREVER)
        timeOut = INFINITE;

    switch (WaitForSingleObject((HANDLE)emid, timeOut))
    {
        case WAIT_FAILED:
            return GT_FAIL;

        case WAIT_TIMEOUT:
            return GT_TIMEOUT;

        case WAIT_OBJECT_0:
            return GT_OK;

        default: break;
    }

    return GT_FAIL;

}

/*******************************************************************************
* simOsSendDataToVisualAsic
*
* DESCRIPTION:
*       Connects to a message-type pipe and writes to it.
*
* INPUTS:
*       bufferPtr - pointer to the data buffer
*       bufferLen - buffer length
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
void simOsSendDataToVisualAsic
(
    IN void **bufferPtr,
    IN GT_U32 bufferLen
)
{

    CallNamedPipe(SIM_OS_STR_NAMED_PIPE_CNS, bufferPtr, bufferLen, NULL,
                  0,NULL, NMPWAIT_NOWAIT);
}

