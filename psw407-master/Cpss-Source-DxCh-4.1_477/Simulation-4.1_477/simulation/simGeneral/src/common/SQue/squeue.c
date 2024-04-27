/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* squeue.c
*
* DESCRIPTION:
*       The module is queue management utility for SKernel modules.
*
* FILE REVISION NUMBER:
*       $Revision: 6 $
*******************************************************************************/
#include <os/simTypesBind.h>
#include <common/Utils/Error/serror.h>
#include <common/SQue/squeue.h>
#include <asicSimulation/SCIB/scib.h>
#include <common/SBUF/sbuf.h>
#include <asicSimulation/SInit/sinit.h>

void sKernelDeviceBuffersPrint(GT_U32 sim_devNum);
/*
 * Typedef: struct SQUE_HEADER_STC
 *
 * Description:
 *      Describe the buffers queue in the simulation.
 *
 * Fields:
 *      magic           : Magic number for consistence check.
 *      event           : Handle o fthe Event object for a queue.
 *      criticalSection : Critical section object to protect a queue.
 *      firstBufId      : First buffer in a queue.
 *      lastBufId       : Last buffer in a queue.
 *      status          : status of queue
 *                        0 - queue empty
 *                        > 0 - queue is not empty
 *      queueSuspended  : GT_TRUE  - queue suspended (cause 'squeBufPut' to put inside the queue
 *                                   message that will be ignored instead of the original buffer)
 *                        GT_FALSE - queue not suspended (regular operation)
 *
 *      debug_lastPoppedBufId - DEBUG purpose only :
 *                          the last buffer that was popped from the queue (
 *                          that was returned by squeBufGet())
 * Comments:
 */
typedef struct  {
    GT_U32                  magic;
    GT_SEM                  event;
/*    GT_MUTEX                criticalSection;-->use SCIB_SEM_TAKE*/
    SIM_BUFFER_ID           firstBufId;
    SIM_BUFFER_ID           lastBufId;
    GT_U32                  status;
    GT_BOOL                 queueSuspended;

    /* DEBUG purpose only */
    SIM_BUFFER_ID           debug_lastPoppedBufId;

} SQUE_HEADER_STC;

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
)
{
}
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
)
{
    SQUE_HEADER_STC * newQueueHeaderPtr;

    newQueueHeaderPtr = malloc(sizeof(SQUE_HEADER_STC));
    if (!newQueueHeaderPtr) {
        sUtilsFatalError( "squeCreate: allocation failed \n");
    }

    /* initialize header */
    /*newQueueHeaderPtr->event  = CreateEvent(NULL,FALSE,FALSE,NULL);*/
    newQueueHeaderPtr->event = SIM_OS_MAC(simOsEventCreate)();
    newQueueHeaderPtr->firstBufId = NULL;
    newQueueHeaderPtr->lastBufId  = NULL;
    newQueueHeaderPtr->status = 0;
    newQueueHeaderPtr->queueSuspended = GT_FALSE;

    newQueueHeaderPtr->debug_lastPoppedBufId = NULL;
    newQueueHeaderPtr->magic = 0x12345678;/* !!! NOT used !!! */

    /*! initilise mutex of buf pool (for alloc / free actions) */
    /*InitializeCriticalSection(&newQueueHeaderPtr->criticalSection);*/
/*    newQueueHeaderPtr->criticalSection = SIM_OS_MAC(simOsMutexCreate)();*/

    return (SQUE_QUEUE_ID) newQueueHeaderPtr;
}

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
)
{
    SQUE_HEADER_STC * queueHeaderPtr;
    if (queId == 0) {
        sUtilsFatalError( "squeDestroy: illegal pointer\n");
    }
    queueHeaderPtr = (SQUE_HEADER_STC *)queId;
    /* Free mutex of buf pool*/
    /*CloseHandle(&queueHeaderPtr->criticalSection);*/
/*    SIM_OS_MAC(simOsMutexDelete)(queueHeaderPtr->criticalSection);*/

    /* Free allocated queu heade */
    free(queueHeaderPtr);
}
/*******************************************************************************
*   internal_squeBufPut
*
* DESCRIPTION:
*       Put SBuf buffer to a queue with option to suspend the queue after enqueue the buffer
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
static void internal_squeBufPut
(
    IN  SQUE_QUEUE_ID    queId,
    IN  SIM_BUFFER_ID      bufId,
    IN  GT_BOOL           suspendAfterQueueing
)
{
    SQUE_HEADER_STC * queueHeadPtr;

    if(queId == 0){
        sUtilsFatalError( "squeBufPut: invalid queue ID\n");
    }
    if(bufId == 0){
        sUtilsFatalError( "squeBufPut: invalid buffer ID\n");
    }
    queueHeadPtr = (SQUE_HEADER_STC *)queId;
    /* Add a new buffer to the tail of the queue */
    /*EnterCriticalSection(&queueHeadPtr->criticalSection);*/
/*    SIM_OS_MAC(simOsMutexLock)(queueHeadPtr->criticalSection);*/
    SCIB_SEM_TAKE;

    if(queueHeadPtr->queueSuspended == GT_TRUE)
    {
        /* because we not know the the 'bufPool' so we can not free it */
        /* but we will replace the buffer with 'dummy message' that 'do nothing' */

        /* NOTE: such casting will not work with 'mini buffers' !!!
           luckily mini buffers are used only by 'broker' (see sMiniBufAlloc)
        */

        /* set source type of buffer --> not used and ignored ... will only free the buffer */
        ((SBUF_BUF_STC*)bufId)->srcType = /*SMAIN_SRC_TYPE___LAST__E*/0x0000FFFF;
    }
    else
    if(suspendAfterQueueing == GT_TRUE)
    {
        /* current buffer will be enqueue regularly but next buffers will
            be treated as 'dummy message' that 'do nothing' */
        queueHeadPtr->queueSuspended = GT_TRUE;
    }

    if ( queueHeadPtr->firstBufId == NULL ){
         queueHeadPtr->firstBufId = bufId;
         queueHeadPtr->lastBufId  = bufId;
    }
    else {
        queueHeadPtr->lastBufId->nextBufPtr = bufId;
        queueHeadPtr->lastBufId = bufId;
    }

    queueHeadPtr->status++;

/*
    simulationPrintf("squeBufPut: \n");
    sKernelDeviceBuffersPrint(0);
*/

    /*LeaveCriticalSection(&queueHeadPtr->criticalSection);*/
/*    SIM_OS_MAC(simOsMutexUnlock)(queueHeadPtr->criticalSection);*/
    SCIB_SEM_SIGNAL;

    /* Signal to the receive task that new buffer has inserted */
    /*SetEvent(queueHeadPtr->event);*/
    SIM_OS_MAC(simOsEventSet)(queueHeadPtr->event);
}

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
    IN  SIM_BUFFER_ID      bufId
)
{
    internal_squeBufPut(queId,bufId,GT_FALSE);
}
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
)
{
    SQUE_HEADER_STC * quePtr;

    if(queId == 0){
        sUtilsFatalError("squeStatusGet: invalid queue ID\n");
    }
    quePtr = (SQUE_HEADER_STC *)queId;

    if(quePtr->queueSuspended == GT_TRUE && quePtr->status == 0)
    {
        /* even if queue is empty but the queue is still under suspension ,
           give indication that queue is not empty */
        return 1;
    }

    return quePtr->status;
}
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
*         SIM_BUFFER_ID - buffer id
*
* COMMENTS:
*
*
*******************************************************************************/
SIM_BUFFER_ID squeBufGet
(
    IN  SQUE_QUEUE_ID    queId
)
{
    SIM_BUFFER_ID     bufId;
    SQUE_HEADER_STC * quePtr;
    GT_U32          ret;

    if(queId == 0){
        sUtilsFatalError( "squeBufPut: invalid queue ID\n");
    }
    quePtr = (SQUE_HEADER_STC *)queId;

    /*EnterCriticalSection(&quePtr->criticalSection);*/
/*    SIM_OS_MAC(simOsMutexLock)(quePtr->criticalSection);*/
    SCIB_SEM_TAKE;

    /* Wait for the first data buffer in the queue */
    while (1) {
        if ( quePtr->firstBufId != NULL )
        {
            break;
        }

        /* queue is empty */
        quePtr->status = 0;

        /* release mutex before event wait */
        SCIB_SEM_SIGNAL;

        /*ret = WaitForSingleObject(quePtr->event, INFINITE);*/
        ret = SIM_OS_MAC(simOsEventWait)(quePtr->event, SIM_OS_WAIT_FOREVER);
        switch ( ret ) {
            case GT_OK:
                break;
            case GT_TIMEOUT:
                simulationPrintf("squeBufGet: timeout\n");
                break;
            case GT_FAIL:
                simulationPrintf("squeBufGet: failed\n");
                break;
        }
        /* take mutex before queue linked list update */
        SCIB_SEM_TAKE;
    }

    /* Get pointer to the head bufer in the queue */
    bufId = quePtr->firstBufId;
    quePtr->firstBufId = quePtr->firstBufId->nextBufPtr;

    bufId->nextBufPtr = NULL;

    quePtr->debug_lastPoppedBufId = bufId;/* save the last that we popped */
/*
    simulationPrintf("squeBufGet: \n");
    sKernelDeviceBuffersPrint(0);
*/
    /*LeaveCriticalSection(&quePtr->criticalSection);*/
/*    SIM_OS_MAC(simOsMutexUnlock)(quePtr->criticalSection);*/
    SCIB_SEM_SIGNAL;

    return bufId;
}

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
)
{
    SQUE_HEADER_STC * quePtr;

    if(queId == 0){
        sUtilsFatalError( "squeSuspend: invalid queue ID\n");
    }
    quePtr = (SQUE_HEADER_STC *)queId;

    SCIB_SEM_TAKE;
    quePtr->queueSuspended = GT_TRUE;
    SCIB_SEM_SIGNAL;

    return;
}

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
)
{
    SQUE_HEADER_STC * quePtr;

    if(queId == 0){
        sUtilsFatalError( "squeResume: invalid queue ID\n");
    }
    quePtr = (SQUE_HEADER_STC *)queId;

    SCIB_SEM_TAKE;
    quePtr->queueSuspended = GT_FALSE;
    SCIB_SEM_SIGNAL;

    return;
}

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
)
{
    internal_squeBufPut(queId,bufId,GT_TRUE/*suspend the queue after enqueue buffer*/);
}

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
)
{
    SQUE_HEADER_STC * quePtr;

    if(queId == 0){
        sUtilsFatalError( "squeFlush: invalid queue ID\n");
        return;
    }

    quePtr = (SQUE_HEADER_STC *)queId;
    if(quePtr->queueSuspended == GT_FALSE)
    {
        sUtilsFatalError( "squeFlush: queue must be suspended for flush operation \n");
        return;
    }

    SCIB_SEM_TAKE;

    quePtr->firstBufId = NULL;
    quePtr->lastBufId  = NULL;
    quePtr->status = 0;
    quePtr->debug_lastPoppedBufId = NULL;

    SCIB_SEM_SIGNAL;

    return;
}

#define STR(strname)    \
    #strname

#define CASE_AND_TYPE_MAC(_caseType,caseName)  \
    case _caseType:                  \
        caseName = STR(_caseType);   \
        break

#define SRC_TYPE_MAC(buff,_srcType,srcName) \
    if((buff)->dataType == (_srcType))       \
    {                                      \
        srcName = STR(_srcType);            \
    }

#include <asicSimulation/SKernel/smain/smain.h>

static void printBufferInfo(
    IN SBUF_BUF_STC    *   nextBufPtr,
    IN GT_U32 usedCount)
{
    char*   str1;
    char*   str2;

    str1 = "unknown  source  type";
    str2 = "unknown  message type";

    /* source type of buffer */
    switch(nextBufPtr->srcType)
    {
        CASE_AND_TYPE_MAC(SMAIN_SRC_TYPE_SLAN_E,str1);
        CASE_AND_TYPE_MAC(SMAIN_SRC_TYPE_CPU_E,str1);
        CASE_AND_TYPE_MAC(SMAIN_SRC_TYPE_UPLINK_E,str1);
        CASE_AND_TYPE_MAC(SMAIN_SRC_TYPE_LOOPBACK_PORT_E,str1);
        CASE_AND_TYPE_MAC(SMAIN_SRC_TYPE_INTERNAL_CONNECTION_E,str1);
        default:
            break;
    }

    /* message type of buffer */
    switch(nextBufPtr->srcType)
    {
        case SMAIN_SRC_TYPE_SLAN_E:
            SRC_TYPE_MAC(nextBufPtr,SMAIN_MSG_TYPE_FRAME_E            ,str2);
            break;

        case SMAIN_SRC_TYPE_CPU_E:
            SRC_TYPE_MAC(nextBufPtr,SMAIN_MSG_TYPE_FDB_UPDATE_E       ,str2);
            SRC_TYPE_MAC(nextBufPtr,SMAIN_CPU_MAILBOX_MSG_E           ,str2);
            SRC_TYPE_MAC(nextBufPtr,SMAIN_CPU_PCSPING_MSG_E           ,str2);
            SRC_TYPE_MAC(nextBufPtr,SMAIN_CPU_FDB_ACT_TRG_E           ,str2);
            SRC_TYPE_MAC(nextBufPtr,SMAIN_CPU_TX_SDMA_QUEUE_E         ,str2);
            SRC_TYPE_MAC(nextBufPtr,SMAIN_CPU_FDB_AUTO_AGING_E        ,str2);
            SRC_TYPE_MAC(nextBufPtr,SMAIN_LINK_CHG_MSG_E              ,str2);
            SRC_TYPE_MAC(nextBufPtr,SMAIN_REACHABILITY_MSG_E          ,str2);
            SRC_TYPE_MAC(nextBufPtr,SMAIN_INTERRUPTS_MASK_REG_E       ,str2);
            SRC_TYPE_MAC(nextBufPtr,SMAIN_MSG_TYPE_FDB_UPLOAD_E       ,str2);
            SRC_TYPE_MAC(nextBufPtr,SMAIN_MSG_TYPE_CNC_FAST_DUMP_E    ,str2);
            SRC_TYPE_MAC(nextBufPtr,SMAIN_MSG_TYPE_SOFT_RESET_E       ,str2);
            SRC_TYPE_MAC(nextBufPtr,SMAIN_MSG_TYPE_GENERIC_FUNCTION_E ,str2);
            break;
        case SMAIN_SRC_TYPE_UPLINK_E:
            SRC_TYPE_MAC(nextBufPtr,SMAIN_MSG_TYPE_FRAME_E            ,str2);
            break;
        case SMAIN_SRC_TYPE_LOOPBACK_PORT_E:
            SRC_TYPE_MAC(nextBufPtr,SMAIN_MSG_TYPE_FRAME_E            ,str2);
            break;
        case SMAIN_SRC_TYPE_INTERNAL_CONNECTION_E:
            SRC_TYPE_MAC(nextBufPtr,SMAIN_MSG_TYPE_FRAME_E            ,str2);
            break;
        default:
            break;
    }

    printf("#%d - SRC [%s] , msg [%s] \n",
        usedCount,str1,str2);
}

extern SKERNEL_DEVICE_OBJECT* smemTestDeviceIdToDevPtrConvert
(
    IN  GT_U32                      deviceId
);

void squeBuffersPrint
(
    IN  SQUE_QUEUE_ID    queId
)
{
    GT_U32              usedCount = 0;

    SIM_BUFFER_ID     nextBufId;
    SQUE_HEADER_STC * quePtr;
    SIM_BUFFER_ID     debug_lastPoppedBufId;

    if(queId == 0){
        sUtilsFatalError( "squeBufPut: invalid queue ID\n");
    }
    quePtr = (SQUE_HEADER_STC *)queId;

    SCIB_SEM_TAKE;
    nextBufId = quePtr->firstBufId;
    debug_lastPoppedBufId = quePtr->debug_lastPoppedBufId;

    if(debug_lastPoppedBufId &&
       nextBufId != debug_lastPoppedBufId &&
        ((SBUF_BUF_ID)(debug_lastPoppedBufId))->state != SBUF_BUF_STATE_FREE_E)
    {
        printf("currently treated by SKernel task (and is not in the queue any more) \n");
        usedCount++;
        printBufferInfo(SIM_CAST_BUFF(debug_lastPoppedBufId),usedCount);
    }

    /* Wait for the first data buffer in the queue */
    while (nextBufId)
    {
        usedCount++;

        printBufferInfo(SIM_CAST_BUFF(nextBufId),usedCount);

        nextBufId = nextBufId->nextBufPtr;
    }

    printf("Summary : used[%d] buffers \n",usedCount);

    SCIB_SEM_SIGNAL;

    return ;
}

/* debug function to print the used buffers of the device */
void sKernelDeviceBuffersPrint(GT_U32 sim_devNum)
{
    SKERNEL_DEVICE_OBJECT * devObjPtr = smemTestDeviceIdToDevPtrConvert(sim_devNum);

    if(devObjPtr->shellDevice == GT_TRUE)
    {
        GT_U32                 coreId;           /* core iterator */
        SKERNEL_DEVICE_OBJECT *currDevObjPtr; /* current device object pointer */
        for(coreId = 0 ; coreId < devObjPtr->numOfCoreDevs ; coreId++)
        {
            currDevObjPtr = devObjPtr->coreDevInfoPtr[coreId].devObjPtr;
            if(currDevObjPtr != NULL)
            {
                SCIB_SEM_TAKE;
                printf("start coreId [%d] \n",coreId);
                SCIB_SEM_SIGNAL;
                squeBuffersPrint(currDevObjPtr->queueId);
            }
        }
    }
    else
    {
        squeBuffersPrint(devObjPtr->queueId);
    }
}


