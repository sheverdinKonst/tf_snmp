/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* simOsBindSync.h
*
* DESCRIPTION:
*       Operating System wrapper. Semaphore facility.
*
*       simOsSemCreate                    SIM_OS_SEM_CREATE_FUN
*       simOsSemDelete                    SIM_OS_SEM_DELETE_FUN
*       simOsSemWait                      SIM_OS_SEM_WAIT_FUN
*       simOsSemSignal                    SIM_OS_SEM_SIGNAL_FUN
*       simOsMutexCreate                  SIM_OS_MUTEX_CREATE_FUN
*       simOsMutexDelete                  SIM_OS_MUTEX_DELETE_FUN
*       simOsMutexUnlock                  SIM_OS_MUTEX_UNLOCK_FUN
*       simOsMutexLock                    SIM_OS_MUTEX_LOCK_FUN
*       simOsEventCreate                  SIM_OS_EVENT_CREATE_FUN
*       simOsEventSet                     SIM_OS_EVENT_SET_FUN
*       simOsEventWait                    SIM_OS_EVENT_WAIT_FUN
*       simOsSendDataToVisualAsic         SIM_OS_SEND_DATA_TO_VISUAL_ASIC_FUN
*       simOsTime                         SIM_OS_TIME_FUN
*
* FILE REVISION NUMBER:
*       $Revision: 3 $
*******************************************************************************/

#ifndef __simOsBindSynch
#define __simOsBindSynch
/************* Includes *******************************************************/

#ifdef __cplusplus
extern "C" {
#endif


/************* Defines ********************************************************/

#define SIM_OS_WAIT_FOREVER            0  /*INFINITE*/


/************* Typedefs *******************************************************/
#if (!defined __gtOsSemh) && (!defined __cmdExtServices_h_)
typedef GT_UINTPTR GT_SEM;
typedef GT_UINTPTR GT_MUTEX;
#endif /*!__gtOsSemh*/

/************* Functions ******************************************************/

/*******************************************************************************
* SIM_OS_SEM_CREATE_FUN
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
*       GT_SEM   - semaphore id
*
* COMMENTS:
*       None
*
*******************************************************************************/
typedef GT_SEM  (*SIM_OS_SEM_CREATE_FUN)
(
    IN  GT_U32           initCount,
    IN  GT_U32           maxCount
);


/*******************************************************************************
* SIM_OS_SEM_DELETE_FUN
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
typedef GT_STATUS (*SIM_OS_SEM_DELETE_FUN)
(
    IN GT_SEM smid
);

/*******************************************************************************
* SIM_OS_SEM_WAIT_FUN
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
typedef GT_STATUS (*SIM_OS_SEM_WAIT_FUN)
(
    IN GT_SEM smid,
    IN GT_U32 timeOut
);

/*******************************************************************************
* SIM_OS_SEM_SIGNAL_FUN
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
typedef GT_STATUS (*SIM_OS_SEM_SIGNAL_FUN)
(
    IN GT_SEM smid
);

/************* Functions ******************************************************/

/*******************************************************************************
* SIM_OS_MUTEX_CREATE_FUN
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
typedef GT_MUTEX  (*SIM_OS_MUTEX_CREATE_FUN)
(
    void
);


/*******************************************************************************
* SIM_OS_MUTEX_DELETE_FUN
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
typedef void (*SIM_OS_MUTEX_DELETE_FUN)
(
    IN GT_MUTEX mid
);

/*******************************************************************************
* SIM_OS_MUTEX_UNLOCK_FUN
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
typedef void (*SIM_OS_MUTEX_UNLOCK_FUN)
(
    IN GT_MUTEX mid
);

/*******************************************************************************
* SIM_OS_MUTEX_LOCK_FUN
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
typedef void (*SIM_OS_MUTEX_LOCK_FUN)
(
    IN GT_MUTEX mid
);
/*******************************************************************************
* SIM_OS_EVENT_CREATE_FUN
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
typedef GT_SEM  (*SIM_OS_EVENT_CREATE_FUN)
(
    void
);
/*******************************************************************************
* SIM_OS_EVENT_SET_FUN
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
typedef GT_STATUS  (*SIM_OS_EVENT_SET_FUN)
(
    IN GT_SEM eventId
);

/*******************************************************************************
* SIM_OS_EVENT_WAIT_FUN
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
typedef GT_STATUS (*SIM_OS_EVENT_WAIT_FUN)
(
    IN GT_SEM eventId,
    IN GT_U32 timeOut
);

/*******************************************************************************
* SIM_OS_SEND_DATA_TO_VISUAL_ASIC_FUN
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
typedef void (*SIM_OS_SEND_DATA_TO_VISUAL_ASIC_FUN)
(
    IN void **bufferPtr,
    IN GT_U32 bufferLen
);

/*******************************************************************************
* SIM_OS_TIME_FUN
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
typedef GT_U32 (*SIM_OS_TIME_FUN)(void);



/* SIM_OS_FUNC_BIND_SYNC_STC -
*    structure that hold the "os synchronize" functions needed be bound to SIM.
*
*/
typedef struct{
    SIM_OS_SEM_CREATE_FUN                  simOsSemCreate;
    SIM_OS_SEM_DELETE_FUN                  simOsSemDelete;
    SIM_OS_SEM_WAIT_FUN                    simOsSemWait;
    SIM_OS_SEM_SIGNAL_FUN                  simOsSemSignal;
    SIM_OS_MUTEX_CREATE_FUN                simOsMutexCreate;
    SIM_OS_MUTEX_DELETE_FUN                simOsMutexDelete;
    SIM_OS_MUTEX_UNLOCK_FUN                simOsMutexUnlock;
    SIM_OS_MUTEX_LOCK_FUN                  simOsMutexLock;
    SIM_OS_EVENT_CREATE_FUN                simOsEventCreate;
    SIM_OS_EVENT_SET_FUN                   simOsEventSet;
    SIM_OS_EVENT_WAIT_FUN                  simOsEventWait;
    SIM_OS_SEND_DATA_TO_VISUAL_ASIC_FUN    simOsSendDataToVisualAsic;/* needed only on devices side */
    SIM_OS_TIME_FUN                        simOsTime;                /* needed only on devices side */

}SIM_OS_FUNC_BIND_SYNC_STC;


extern    SIM_OS_SEM_CREATE_FUN                  SIM_OS_MAC(simOsSemCreate);
extern    SIM_OS_SEM_DELETE_FUN                  SIM_OS_MAC(simOsSemDelete);
extern    SIM_OS_SEM_WAIT_FUN                    SIM_OS_MAC(simOsSemWait);
extern    SIM_OS_SEM_SIGNAL_FUN                  SIM_OS_MAC(simOsSemSignal);
extern    SIM_OS_MUTEX_CREATE_FUN                SIM_OS_MAC(simOsMutexCreate);
extern    SIM_OS_MUTEX_DELETE_FUN                SIM_OS_MAC(simOsMutexDelete);
extern    SIM_OS_MUTEX_UNLOCK_FUN                SIM_OS_MAC(simOsMutexUnlock);
extern    SIM_OS_MUTEX_LOCK_FUN                  SIM_OS_MAC(simOsMutexLock);

extern    SIM_OS_EVENT_CREATE_FUN                SIM_OS_MAC(simOsEventCreate);
extern    SIM_OS_EVENT_SET_FUN                   SIM_OS_MAC(simOsEventSet);
extern    SIM_OS_EVENT_WAIT_FUN                  SIM_OS_MAC(simOsEventWait);
#ifndef APPLICATION_SIDE_ONLY
extern    SIM_OS_SEND_DATA_TO_VISUAL_ASIC_FUN    SIM_OS_MAC(simOsSendDataToVisualAsic);
extern    SIM_OS_TIME_FUN                        SIM_OS_MAC(simOsTime);
#endif /*!APPLICATION_SIDE_ONLY*/

#ifdef __cplusplus
}
#endif

#endif  /* __simOsBindSynch */


