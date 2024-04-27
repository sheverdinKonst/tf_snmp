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
*       Linux Operating System Simulation. Semaphore facility.
*
* DEPENDENCIES:
*       CPU independed.
*
* FILE REVISION NUMBER:
*       $Revision: 7 $
*******************************************************************************/

#include <time.h>

#include <stdio.h>
#include <assert.h>
#include <errno.h>

#include <gtOs/gtOsSem.h>
#include <os/simTypes.h>
#include <os/simTypesBind.h>
#define EXPLICIT_INCLUDE_TO_SIM_OS_H_FILES
#include <os/simOsSync.h>

#if 0
#include <vxw_hdrs.h>
#endif

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

    if (osSemCCreate(NULL, initCount, &returnSem) != GT_OK)
    {
        return 0;
    }

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
    osSemDelete(smid);

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
    if (timeOut == SIM_OS_WAIT_FOREVER)
        timeOut = OS_WAIT_FOREVER;

    return osSemWait(smid, timeOut);
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
    return osSemSignal(smid);
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
    GT_MUTEX returnMtx;

    if (osMutexCreate(NULL, &returnMtx) != GT_OK)
        return 0;
    return returnMtx;
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
    osMutexDelete(mid);

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
    osMutexUnlock(mid);
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
    osMutexLock(mid);
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
    GT_SEM returnSem;

    if (osSemBinCreate(NULL, OS_SEMB_EMPTY, &returnSem) != GT_OK)
    {
        return 0;
    }

    return returnSem;
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
    return osSemSignal(eventHandle);
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
        timeOut = OS_WAIT_FOREVER;

    return osSemWait(emid, timeOut);
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
}
