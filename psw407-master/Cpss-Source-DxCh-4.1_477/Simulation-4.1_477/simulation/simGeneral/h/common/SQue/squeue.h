/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* squeue.h
*
* DESCRIPTION:
*       This is a API definition for SQueue module of the Simulation.
*
*
* FILE REVISION NUMBER:
*       $Revision: 6 $
*
*******************************************************************************/
#ifndef __squeueh
#define __squeueh

#include <os/simTypes.h>
#include <os/simTypesBind.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * Typedef: struct SIM_BUFFER_STC
 *
 * Description:
 *      Describe the casting type of the buffers managed in the SQUEU lib
 *
 * Fields:
 *      magic           : Magic number for consistence check.
 *      nextBufPtr      : Pointer to the next buffer in the pool or queue.
 * Comments:
 */
typedef struct SIM_BUFFER_STCT{
    GT_U32  magic;
    struct SIM_BUFFER_STCT *nextBufPtr;
}SIM_BUFFER_STC;

typedef SIM_BUFFER_STC * SIM_BUFFER_ID;

/* Queue ID typedef */
typedef  void *     SQUE_QUEUE_ID;
/* API functions */

/*******************************************************************************
*   squeInit
*
* DESCRIPTION:
*       Init Squeue library.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
void squeInit
(
    void
);

/*******************************************************************************
*   squeCreate
*
* DESCRIPTION:
*       Create new queue.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*         SQUE_QUEUE_ID - new queue ID
*
* COMMENTS:
*       In the case of memory lack the function aborts application.
*
*******************************************************************************/
SQUE_QUEUE_ID squeCreate
(
    void
);

/*******************************************************************************
*   squeDestroy
*
* DESCRIPTION:
*       Free memory allocated for queue.
*
* INPUTS:
*       queId  - id of a queue.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
*
* COMMENTS:
*
*
*******************************************************************************/
void squeDestroy
(
    IN  SQUE_QUEUE_ID    queId
);
/*******************************************************************************
*   squeBufPut
*
* DESCRIPTION:
*       Put SBuf buffer to a queue.
*
* INPUTS:
*       queId - id of queue.
*       bufId - id of buffer.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
*
* COMMENTS:
*
*
*******************************************************************************/
void squeBufPut
(
    IN  SQUE_QUEUE_ID    queId,
    IN  SIM_BUFFER_ID    bufId
);
/*******************************************************************************
*   squeStatusGet
*
* DESCRIPTION:
*       Get queue's status
*
* INPUTS:
*       queId - id of queue.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*         0 - queue is empty
*         other value - queue is not empty , or queue is suspended
*
* COMMENTS:
*
*
*******************************************************************************/
GT_U32 squeStatusGet
(
    IN  SQUE_QUEUE_ID    queId
);

/*******************************************************************************
*   squeBufGet
*
* DESCRIPTION:
*       Get Sbuf from a queue, if no buffers wait for it forever.
*
* INPUTS:
*       queId - id of queue.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*         SBUF_BUF_ID - buffer id
*
* COMMENTS:
*
*
*******************************************************************************/
SIM_BUFFER_ID squeBufGet
(
    IN  SQUE_QUEUE_ID    queId
);

/*******************************************************************************
*   squeSuspend
*
* DESCRIPTION:
*       Suspend a queue queue. (cause 'squeBufPut' to put inside the queue
*       message that will be ignored instead of the original buffer)
*
* INPUTS:
*       queId - id of queue.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
*
* COMMENTS:
*
*
*******************************************************************************/
void squeSuspend
(
    IN  SQUE_QUEUE_ID    queId
);

/*******************************************************************************
*   squeResume
*
* DESCRIPTION:
*       squeResume a queue that was suspended by squeSuspend or by squeBufPutAndQueueSuspend.
*       (allow 'squeBufPut' to put buffers into the queue)
*
* INPUTS:
*       queId - id of queue.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
*
* COMMENTS:
*
*
*******************************************************************************/
void squeResume
(
    IN  SQUE_QUEUE_ID    queId
);

/*******************************************************************************
*   squeBufPutAndQueueSuspend
*
* DESCRIPTION:
*       Put SBuf buffer to a queue and then 'suspend' the queue (for any next buffers)
*
* INPUTS:
*       queId - id of queue.
*       bufId - id of buffer.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
*
* COMMENTS:
*
*
*******************************************************************************/
void squeBufPutAndQueueSuspend
(
    IN  SQUE_QUEUE_ID    queId,
    IN  SIM_BUFFER_ID    bufId
);

/*******************************************************************************
*   squeFlush
*
* DESCRIPTION:
*       flush all messages that are in the queue.
*       NOTE: operation valid only if queue is suspended !!!
*
* INPUTS:
*       queId - id of queue.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
*
* COMMENTS:
*      this function not free the buffers ... for that use sbufPoolFlush(...)
*
*******************************************************************************/
void squeFlush
(
    IN  SQUE_QUEUE_ID    queId
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __squeueh */


