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
* simOsWin32Sync.c
*
* DESCRIPTION:
*       Win32 Operating System Simulation. Semaphore facility.
*
* DEPENDENCIES:
*       Win32, CPU independed.
*
* FILE REVISION NUMBER:
*       $Revision: 3 $
*******************************************************************************/

#include <vxWorks.h>
#include <objLib.h>
#include <semLib.h>
#include <sysLib.h>


#include <os/simTypes.h>
#define EXPLICIT_INCLUDE_TO_SIM_OS_H_FILES
#include <asicSimulation/SInit/sinit.h>
#include <os/simOsSync.h>

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

    returnSem = (GT_SEM)semCCreate(SEM_Q_FIFO,initCount);

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
    semDelete((SEM_ID)smid);

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
    STATUS rc;

    if (timeOut == 0)
        rc = semTake ((SEM_ID) smid, WAIT_FOREVER);
    else
    {
        int num, delay;

        num = sysClkRateGet();
        delay = (num * timeOut) / 1000;
        if (delay < 1)
            rc = semTake ((SEM_ID) smid, 1);
        else
            rc = semTake ((SEM_ID) smid, delay);
    }

    if (rc != OK)
    {
        if (errno == S_objLib_OBJ_TIMEOUT)
            return GT_TIMEOUT;
        else
            return GT_FAIL;
    }

    return GT_OK;
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
    if (semGive((SEM_ID)smid) == 0)
        return GT_OK;

    return GT_FAIL;
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

    returnSem = (GT_MUTEX)semMCreate (SEM_Q_FIFO);

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
    semDelete((SEM_ID)mid);
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
    semGive((SEM_ID)mid);
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
    semTake((SEM_ID)mid, WAIT_FOREVER);
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
    FUNCTION_NOT_SUPPORTED(simOsEventCreate);

    return NULL;
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
    FUNCTION_NOT_SUPPORTED(simOsEventSet);

    return GT_NOT_SUPPORTED;
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
    FUNCTION_NOT_SUPPORTED(simOsEventWait);

    return GT_NOT_SUPPORTED;
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
    FUNCTION_NOT_SUPPORTED(simOsSendDataToVisualAsic);
}
