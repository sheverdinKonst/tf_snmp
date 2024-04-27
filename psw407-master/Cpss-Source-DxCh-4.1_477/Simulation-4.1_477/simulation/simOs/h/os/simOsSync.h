/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* simOsSync.h
*
* DESCRIPTION:
*       Operating System wrapper. Semaphore facility.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 2 $
*******************************************************************************/

#ifndef EXPLICIT_INCLUDE_TO_SIM_OS_H_FILES
    #error "include to those H files should be only for bind purposes"
#endif /*!EXPLICIT_INCLUDE_TO_SIM_OS_H_FILES*/

#ifndef __simOsSynch
#define __simOsSynch
/************* Includes *******************************************************/



#ifdef __cplusplus
extern "C" {
#endif


/************* Functions ******************************************************/

/*******************************************************************************
* simOsSemCreate
*
* DESCRIPTION:
*       Create and initialize universal semaphore.
*
* INPUTS:
*       init       - init value of semaphore (full or empty)
*
*       maxCount   - maximal number of semaphores
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
);


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
);

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
*       GT_TIMEOUT - on time out
*
* COMMENTS:
*       None
*
*******************************************************************************/
GT_STATUS simOsSemWait
(
    IN GT_SEM smid,
    IN GT_U32 timeOut
);

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
);

/************* Functions ******************************************************/

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
);


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
);

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
);

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
);
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
);
/*******************************************************************************
* simOsEventSet
*
* DESCRIPTION:
*       Signal that the event was initialized.
*
* INPUTS:
*       eventId - Id of the event
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
    IN GT_SEM eventId
);

/*******************************************************************************
* simOsEventWait
*
* DESCRIPTION:
*       Wait on event.
*
* INPUTS:
*       eventId - event Id
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
    IN GT_SEM eventId,
    IN GT_U32 timeOut
);

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
);

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
GT_U32 simOsTime(void);

#ifdef __cplusplus
}
#endif

#endif  /* __simOsSynch */


