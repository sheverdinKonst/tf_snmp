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
* sbuf.c
*
* DESCRIPTION:
*       The module is buffer management utility for SKernel modules.
*
* DEPENDENCIES:
*
* FILE REVISION NUMBER:
*       $Revision: 11 $
*******************************************************************************/
#include <os/simTypesBind.h>
#include <common/SBUF/sbuf.h>
#include <common/Utils/Error/serror.h>
#include <asicSimulation/SCIB/scib.h>
#include <asicSimulation/SKernel/smain/smain.h>
#include <asicSimulation/SInit/sinit.h>

/*******************************************************************************
* Private type definition
*******************************************************************************/
/* constants */

/* Maximal number of pools in the pool's array */
#define SPULL_ARRAY_SIZE_CNS                1024
/* Maximal number of buffers in the pool's array entry */
#define SPULL_ENTRY_SIZE_CNS                16*1024
/* NULL buffer ID in the case of allocation fail */
#define SBUF_BUF_ID_NULL                    NULL

/* Memory validation on free memory */
#define SBUF_BUF_MAGIC_SIZE_CNS             0xa5a5a551
/*
 * Typedef: struct SBUF_POOL_STC
 *
 * Description:
 *      Describe the buffers pool in the simulation.
 *
 * Fields:
 *      poolSize        : Number of buffers in a pool,
 *                      :   0 means that pool is free.
 *      bufferSize      : number of bytes for each buffer in the pool
 *      buffersMetadataPtr : Pointer to allocated metadata about the buffers.
 *      buffersBytesPtr : Pointer to allocated memory for all buffers.
 *      firstFreeBufPtr : Pointer to the first free buffer. Buffer pool
 *                      : is managed in "Stack Like" way - this is
 *                      : "top of the stack"
 *      numOfFreeBuffers: number of free buffers
 *      poolSuspended  : GT_TRUE  - pool suspended (cause 'sbufAlloc' to return NULL)
 *                        GT_FALSE - pool not suspended (regular operation)
 * Comments:
 */
typedef struct  {
    GT_U32                  poolSize;
    GT_U32                  bufferSize;
    SBUF_BUF_STC        *   buffersMetadataPtr;
    GT_U8               *   buffersBytesPtr;

    SBUF_BUF_STC        *   firstFreeBufPtr;

    GT_U32                  numOfFreeBuffers;
    GT_BOOL                 poolSuspended;
} SBUF_POOL_STC;


/* Pointer to the pools array */
static  SBUF_POOL_STC  * sbufPoolsPtr;

/* Number of pools in the pools array */
static GT_U32            sbufPoolsNumber;



/*******************************************************************************
* sbufInit
*
* DESCRIPTION:
*       Init Sbuff lib, allocate buffers and pools.
*
* INPUTS:
*       maxPoolsNum - maximal number of buffer pools
*
* OUTPUTS:
*       None
*
* RETURNS:
*
* COMMENTS:
*       In the case of memory lack the function aborts application
*
*******************************************************************************/
void sbufInit
(
    IN  GT_U32              maxPoolsNum
)
{
    GT_U32              i;
    GT_U32              size;

    if ( (maxPoolsNum == 0) ||
         (maxPoolsNum > SPULL_ARRAY_SIZE_CNS) ) {
        sUtilsFatalError("sbufInit: illegal maxPoolsNum %lu\n", maxPoolsNum);
    }

    /* Allocate pull array */
    size = maxPoolsNum * sizeof(SBUF_POOL_STC);
    sbufPoolsNumber = maxPoolsNum;
    sbufPoolsPtr = malloc(size);
    if ( sbufPoolsPtr == NULL ) {
        sUtilsFatalError("sbufInit: buffer initialization error\n");
    }
    memset(sbufPoolsPtr, 0, size);
    /* Initialize critical section for every buffer pool in the pool array */
    for ( i = 0; i < sbufPoolsNumber; i++) {
        /*InitializeCriticalSection(&sbufPoolsPtr[i].criticalSection);*/
/*        sbufPoolsPtr[i].criticalSection = SIM_OS_MAC(simOsMutexCreate)();*/
    }
}

/*******************************************************************************
*   internalPoolCreate
*
* DESCRIPTION:
*       Create new buffers pool.
*
* INPUTS:
*       poolSize - number of buffers in a pool.
*       bufferSize - size in bytes of each buffer
*
* OUTPUTS:
*       None.
*
* RETURNS:
*         SBUF_POOL_ID - new pool ID
*
* COMMENTS:
*       In the case of memory lack the function aborts application.
*
*******************************************************************************/
static SBUF_POOL_ID internalPoolCreate
(
    IN  GT_U32          poolSize,
    IN  GT_U32              bufferSize
)
{
    GT_U32              i;
    GT_U32              j;
    GT_U32              size1,size2;
    SBUF_BUF_STC    *   nextFreeBufPtr;

    if ( (poolSize == 0) ||
         (poolSize > SPULL_ENTRY_SIZE_CNS) ) {
        sUtilsFatalError("internalPoolCreate: illegal poolSize %lu\n",
            poolSize);
    }
    for (i = 0; i < sbufPoolsNumber; i++) {
        if (sbufPoolsPtr[i].poolSize == 0)
            break;
    }
    if (i == sbufPoolsNumber) {
        sUtilsFatalError("internalPoolCreate: memory lack in the pool array\n");
    }
    /* Create pool of buffers and initialize linked list */

 /*   EnterCriticalSection(&sbufPoolsPtr[i].criticalSection);*/

    sbufPoolsPtr[i].poolSize = poolSize;
    sbufPoolsPtr[i].numOfFreeBuffers = poolSize;
    /* allocate chunk for buffers metadata */
    size1 = poolSize * sizeof(SBUF_BUF_STC);
    sbufPoolsPtr[i].buffersMetadataPtr = calloc(poolSize,sizeof(SBUF_BUF_STC));

    /* allocate chunk for buffers bytes */
    size2 = poolSize * bufferSize;
    sbufPoolsPtr[i].buffersBytesPtr    = calloc(poolSize,bufferSize);

    if(sbufPoolsPtr[i].buffersMetadataPtr == NULL ||
       sbufPoolsPtr[i].buffersBytesPtr == NULL)
    {
        sUtilsFatalError("internalPoolCreate: malloc failed to allocate[%d] bytes \n", size1+size2);
    }

    sbufPoolsPtr[i].poolSuspended = GT_FALSE;
    sbufPoolsPtr[i].bufferSize = bufferSize;
    sbufPoolsPtr[i].firstFreeBufPtr = sbufPoolsPtr[i].buffersMetadataPtr;
    for (j = 0, nextFreeBufPtr = sbufPoolsPtr[i].firstFreeBufPtr;
         j < poolSize; j++, nextFreeBufPtr++) {
        nextFreeBufPtr->magic = SBUF_BUF_MAGIC_SIZE_CNS;
        nextFreeBufPtr->state = SBUF_BUF_STATE_FREE_E;
        nextFreeBufPtr->actualDataPtr = NULL;
        nextFreeBufPtr->actualDataSize = 0;
        /* already allocated as huge chunk to support quick arithmetic of 'sbufGetBufIdByData(...)' */
        nextFreeBufPtr->data =
            &sbufPoolsPtr[i].buffersBytesPtr[j*bufferSize];
        /* Chain to the next free buffer if that is not last buffer in list */
        if (j != poolSize - 1)
            nextFreeBufPtr->nextBufPtr = nextFreeBufPtr + 1;
    }

  /*  LeaveCriticalSection(&sbufPoolsPtr[i].criticalSection);     */
    return (SBUF_POOL_ID)&sbufPoolsPtr[i];
}

/*******************************************************************************
*   sbufPoolCreate
*
* DESCRIPTION:
*       Create new buffers pool.
*
* INPUTS:
*       poolSize - number of buffers in a pool.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*         SBUF_POOL_ID - new pool ID
*
* COMMENTS:
*       In the case of memory lack the function aborts application.
*
*******************************************************************************/
SBUF_POOL_ID sbufPoolCreate
(
    IN  GT_U32          poolSize
)
{
    return internalPoolCreate(poolSize,SBUF_DATA_SIZE_CNS);
}
/*******************************************************************************
*   sbufPoolCreateWithBufferSize
*
* DESCRIPTION:
*       Create new buffers pool.
*
* INPUTS:
*       poolSize - number of buffers in a pool.
*       bufferSize - size in bytes of each buffer
* OUTPUTS:
*       None.
*
* RETURNS:
*         SBUF_POOL_ID - new pool ID
*
* COMMENTS:
*       In the case of memory lack the function aborts application.
*
*******************************************************************************/
SBUF_POOL_ID sbufPoolCreateWithBufferSize
(
    IN  GT_U32              poolSize,
    IN  GT_U32              bufferSize
)
{
    return internalPoolCreate(poolSize,bufferSize);
}


/*******************************************************************************
*   sbufPoolFree
*
* DESCRIPTION:
*       Free buffers memory.
*
* INPUTS:
*       poolId - id of a pool.
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
void sbufPoolFree
(
    IN  SBUF_POOL_ID    poolId
)
{
    SBUF_POOL_STC    *  pullBufPtr;

    if (poolId == NULL) {
        sUtilsFatalError("sbufPoolFree: illegal pointer\n");
    }

    pullBufPtr = (SBUF_POOL_STC *) poolId;

    /* Free all allocated metadata info about buffers */
    /* Free all allocated bytes of the buffers */
    free(pullBufPtr->buffersMetadataPtr);
    free(pullBufPtr->buffersBytesPtr);

    /* reset all properties for that poolId*/
    memset(pullBufPtr, 0, sizeof(SBUF_POOL_STC));
}

/*******************************************************************************
*   sbufAlloc
*
* DESCRIPTION:
*       Allocate buffer.
*
* INPUTS:
*       poolId - id of a pool.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*          SBUF_BUF_ID - buffer id if exist.
*          SBUF_BUF_ID_NULL - if no free buffers.
*
* COMMENTS:
*
*
*******************************************************************************/
SBUF_BUF_ID sbufAlloc
(
    IN  SBUF_POOL_ID    poolId,
    IN  GT_U32          dataSize
)
{
    SBUF_POOL_STC     * poolBufPtr;
    SBUF_BUF_STC      * freeBufPtr;

    if (poolId == NULL) {
        sUtilsFatalError("sbufAlloc: illegal pointer\n");
    }

    poolBufPtr = (SBUF_POOL_STC *) poolId;

    if ( (dataSize == 0) ||
         (dataSize > poolBufPtr->bufferSize) ) {
        sUtilsFatalError("sbufAlloc: illegal dataSize [%lu] (valid 1..[%lu])\n",
            dataSize,poolBufPtr->bufferSize);
    }

    /*EnterCriticalSection(&poolBufPtr->criticalSection);*/
/*    SIM_OS_MAC(simOsMutexLock)(poolBufPtr->criticalSection);*/
    SCIB_SEM_TAKE;

    if(poolBufPtr->poolSuspended)
    {
        SCIB_SEM_SIGNAL;

        simWarningPrintf("sbufAlloc: no buffers --> QUEUE is suspended !!! \n");

        return NULL;
    }

    /* Get first free bufer from the pool */
    freeBufPtr = poolBufPtr->firstFreeBufPtr;
    if (freeBufPtr != NULL) {
        /* Forward free bufer pointer to the next buffer */
        poolBufPtr->firstFreeBufPtr = freeBufPtr->nextBufPtr;
        poolBufPtr->numOfFreeBuffers--;
    }

    /*LeaveCriticalSection(&poolBufPtr->criticalSection);*/
/*    SIM_OS_MAC(simOsMutexUnlock)(poolBufPtr->criticalSection);*/
    SCIB_SEM_SIGNAL;


    /* Set buffer attributes and mark his state as SBUF_BUF_STATE_BUZY_E */
    if (freeBufPtr != NULL ) {
        freeBufPtr->nextBufPtr      = NULL;
        freeBufPtr->state           = SBUF_BUF_STATE_BUZY_E;
        freeBufPtr->actualDataSize  = dataSize;
        freeBufPtr->actualDataPtr =   freeBufPtr->data + poolBufPtr->bufferSize - dataSize;

    }
    else
    {
        simWarningPrintf("sbufAlloc: no buffers\n");
    }

    return (SBUF_BUF_ID)freeBufPtr;

}
/*******************************************************************************
*   sbufAllocWithProtectedAmount
*
* DESCRIPTION:
*       Allocate buffer , but only if there are enough free buffers after the alloc.
*
* INPUTS:
*       poolId - id of a pool.
*       dataSize - size for the alloc.
*       protectedAmount - number of free buffers that must be still in the pool
*                   (after alloc of 'this' one)
*
* OUTPUTS:
*       None.
*
* RETURNS:
*          SBUF_BUF_ID - buffer id if exist.
*          SBUF_BUF_ID_NULL - if no free buffers.
*
* COMMENTS:
*
*
*******************************************************************************/
SBUF_BUF_ID sbufAllocWithProtectedAmount
(
    IN  SBUF_POOL_ID    poolId,
    IN  GT_U32          dataSize,
    IN  GT_U32          protectedAmount
)
{
    SBUF_POOL_STC     * poolBufPtr;
    SBUF_BUF_ID     bufferId;

    if (poolId == NULL) {
        sUtilsFatalError("sbufAllocWithProtectedAmount: illegal pointer\n");
    }

    poolBufPtr = (SBUF_POOL_STC *) poolId;

    SCIB_SEM_TAKE;

    if(poolBufPtr->numOfFreeBuffers < protectedAmount)
    {
        SCIB_SEM_SIGNAL;
        return (SBUF_BUF_ID)NULL;
    }

    bufferId = sbufAlloc(poolId,dataSize);

    SCIB_SEM_SIGNAL;

    return bufferId;
}

/*******************************************************************************
*   sbufFree
*
* DESCRIPTION:
*       Free buffer.
*
* INPUTS:
*       poolId - id of a pool.
*       bufId - id of buffer
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
void sbufFree
(
    IN  SBUF_POOL_ID    poolId,
    IN  SBUF_BUF_ID     bufId
)
{
    SBUF_POOL_STC     * poolBufPtr;
    SBUF_BUF_STC      * allocBufPtr;

    if (poolId == NULL) {
        sUtilsFatalError("sbufFree: illegal pointer\n");
    }
    poolBufPtr = (SBUF_POOL_STC *)poolId;

    if (bufId == NULL) {
        sUtilsFatalError("sbufFree: illegal pointer\n");
    }
    allocBufPtr = (SBUF_BUF_STC *)bufId;

    if ( allocBufPtr->magic != SBUF_BUF_MAGIC_SIZE_CNS ) {
        sUtilsFatalError("sbufFree: illegal buffer %p\n", bufId);
    }

    if ( allocBufPtr->state == SBUF_BUF_STATE_FREE_E ) {
        sUtilsFatalError("sbufFree : Warning buffer already free\n") ;
    }

    allocBufPtr->state          = SBUF_BUF_STATE_FREE_E;
    allocBufPtr->actualDataPtr  = NULL;
    allocBufPtr->actualDataSize = 0;

    /*EnterCriticalSection(&poolBufPtr->criticalSection);*/
/*    SIM_OS_MAC(simOsMutexLock)(poolBufPtr->criticalSection);*/
    SCIB_SEM_TAKE;

    /* Put back buffer in pool (head of pool) */
    allocBufPtr->nextBufPtr = poolBufPtr->firstFreeBufPtr;
    poolBufPtr->firstFreeBufPtr = allocBufPtr;
    poolBufPtr->numOfFreeBuffers++;

    /*LeaveCriticalSection(&poolBufPtr->criticalSection);*/
/*    SIM_OS_MAC(simOsMutexUnlock)(poolBufPtr->criticalSection);*/
    SCIB_SEM_SIGNAL;
}

/*******************************************************************************
*   sbufDataGet
*
* DESCRIPTION:
*       Get pointer on the first byte of data in the buffer
*       and actual size of data.
*
* INPUTS:
*       bufId       - id of buffer
*
* OUTPUTS:
*       dataPrtPtr  - pointer to actual data of buffer
*       dataSizePrt - actual data size of a buffer
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
void sbufDataGet
(
    IN  SBUF_BUF_ID     bufId,
    OUT GT_U8   **      dataPrtPtr,
    OUT GT_U32  *       dataSizePrt
)
{
    SBUF_BUF_STC      * bufPtr;

    if (bufId == NULL) {
        sUtilsFatalError("sbufDataGet: illegal pointer\n");
    }
    bufPtr = (SBUF_BUF_STC *)bufId;

    if ( bufPtr->magic != SBUF_BUF_MAGIC_SIZE_CNS ) {
        sUtilsFatalError("sbufDataSet: illegal buffer %p\n", bufId);
    }
    *dataPrtPtr  = bufPtr->actualDataPtr;
    *dataSizePrt = bufPtr->actualDataSize;
}
/*******************************************************************************
*   sbufDataSet
*
* DESCRIPTION:
*       Set pointer to the start of data and new data size.
*
* INPUTS:
*       bufId    - id of buffer
*       dataPrt  - pointer to actual data of buffer
*       dataSize - actual data size of a buffer
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
void sbufDataSet
(
    IN  SBUF_BUF_ID     bufId,
    IN  GT_U8   *       dataPtr,
    IN  GT_U32          dataSize
)
{
    SBUF_BUF_STC      * bufPtr;

    if (bufId == NULL) {
        sUtilsFatalError("sbufDataSet: illegal pointer\n");
    }
    bufPtr = (SBUF_BUF_STC *)bufId;
    if ( bufPtr->magic != SBUF_BUF_MAGIC_SIZE_CNS ) {
        sUtilsFatalError("sbufDataSet: illegal buffer %p\n", bufId);
    }

    if ( (dataSize == 0) ||
         (dataSize > SBUF_DATA_SIZE_CNS) ) {
        sUtilsFatalError("sbufDataSet: illegal data_size %lu\n", dataSize);
    }

    if ( (dataPtr < bufPtr->data) ||
         (dataPtr + dataSize) > (bufPtr->data + SBUF_DATA_SIZE_CNS) ) {
        sUtilsFatalError("sbufDataSet: data error %lx %lu\n", dataPtr, dataSize);
    }

    bufPtr->actualDataPtr = dataPtr;
    bufPtr->actualDataSize = dataSize;
}

/*******************************************************************************
*   sbufGetBufIdByData
*
* DESCRIPTION:
*       Get buff ID of buffer , by the pointer to any place inside the buffer
*
* INPUTS:
*       poolId - id of a pool.
*       dataPtr  - pointer to actual data of buffer
*
* OUTPUTS:
*
* RETURNS:
*      buff_id - the ID of the buffer that the data is in .
*                NULL if dataPtr is not in a buffer
* COMMENTS:
*
*       dataPtr - the pointer must point to data in the buffer
*
*******************************************************************************/
SBUF_BUF_ID sbufGetBufIdByData(
    IN  SBUF_POOL_ID    poolId,
    IN GT_U8   *        dataPtr
)
{
    SBUF_POOL_STC     * poolBufPtr;/* pointer to the pool */
    GT_U32            bufOffset;   /* offset (index) of the buffer in the pool*/
    SBUF_BUF_STC     *foundBufPtr;/* the buffer that was found */
    GT_U32            poolNumBytes;/*number of bytes in the pool */

    poolBufPtr = (SBUF_POOL_STC *) poolId;
    poolNumBytes = poolBufPtr->bufferSize * poolBufPtr->poolSize;

    if((GT_UINTPTR)poolBufPtr->buffersBytesPtr > (GT_UINTPTR)dataPtr
       ||
       (GT_UINTPTR)(poolBufPtr->buffersBytesPtr + poolNumBytes) < (GT_UINTPTR)dataPtr)
    {
        return (SBUF_BUF_ID)NULL;
    }

    bufOffset = (dataPtr  - poolBufPtr->buffersBytesPtr) /
                    poolBufPtr->bufferSize;

    foundBufPtr = &poolBufPtr->buffersMetadataPtr[bufOffset];

    /* check that we did good calculation */
    if ( foundBufPtr->magic != SBUF_BUF_MAGIC_SIZE_CNS ) {
        sUtilsFatalError("sbufGetBufIdByData: miss calculation \n");
    }

    return (SBUF_BUF_ID)foundBufPtr;
}

/*******************************************************************************
*   sbufFreeBuffersNumGet
*
* DESCRIPTION:
*       get the number of free buffers.
*
* INPUTS:
*       poolId - id of a pool.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*          the number of free buffers.
*
* COMMENTS:
*
*
*******************************************************************************/
GT_U32 sbufFreeBuffersNumGet
(
    IN  SBUF_POOL_ID    poolId
)
{
    SBUF_POOL_STC     * poolBufPtr;

    if (poolId == NULL) {
        sUtilsFatalError("sbufFreeBuffersNumGet: illegal pointer\n");
    }

    poolBufPtr = (SBUF_POOL_STC *) poolId;

    return poolBufPtr->numOfFreeBuffers;
}

/*******************************************************************************
*   sbufAllocatedBuffersNumGet
*
* DESCRIPTION:
*       get the number of allocated buffers.
*
* INPUTS:
*       poolId - id of a pool.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*          the number of allocated buffers.
*
* COMMENTS:
*
*
*******************************************************************************/
GT_U32 sbufAllocatedBuffersNumGet
(
    IN  SBUF_POOL_ID    poolId
)
{
    SBUF_POOL_STC     * poolBufPtr;

    if (poolId == NULL) {
        sUtilsFatalError("sbufAllocatedBuffersNumGet: illegal pointer\n");
    }

    poolBufPtr = (SBUF_POOL_STC *) poolId;

    return (poolBufPtr->poolSize - poolBufPtr->numOfFreeBuffers);
}


/*******************************************************************************
*   sbufPoolSuspend
*
* DESCRIPTION:
*       suspend a pool (for any next alloc)
*
* INPUTS:
*       poolId   - id of a pool.
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
void sbufPoolSuspend
(
    IN  SBUF_POOL_ID    poolId
)
{
    SBUF_POOL_STC     * poolBufPtr;

    if (poolId == NULL) {
        sUtilsFatalError("sbufPoolSuspend: illegal pointer\n");
    }

    poolBufPtr = (SBUF_POOL_STC *) poolId;

    SCIB_SEM_TAKE;
    poolBufPtr->poolSuspended = GT_TRUE;
    SCIB_SEM_SIGNAL;

    return;
}
/*******************************************************************************
*   sbufPoolResume
*
* DESCRIPTION:
*       Resume a pool that was suspended by sbufAllocAndPoolSuspend or by sbufPoolSuspend
*
* INPUTS:
*       poolId   - id of a pool.
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
void sbufPoolResume
(
    IN  SBUF_POOL_ID    poolId
)
{
    SBUF_POOL_STC     * poolBufPtr;

    if (poolId == NULL) {
        sUtilsFatalError("sbufPoolSuspend: illegal pointer\n");
    }

    poolBufPtr = (SBUF_POOL_STC *) poolId;

    SCIB_SEM_TAKE;
    poolBufPtr->poolSuspended = GT_FALSE;
    SCIB_SEM_SIGNAL;

    return;
}

/*******************************************************************************
*   sbufAllocAndPoolSuspend
*
* DESCRIPTION:
*       Allocate buffer and then 'suspend' the pool (for any next alloc).
*
* INPUTS:
*       poolId   - id of a pool.
*       dataSize - size of the data.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*          SBUF_BUF_ID - buffer id if exist.
*          SBUF_BUF_ID_NULL - if no free buffers.
*
* COMMENTS:
*
*
*******************************************************************************/
SBUF_BUF_ID sbufAllocAndPoolSuspend
(
    IN  SBUF_POOL_ID    poolId,
    IN  GT_U32          dataSize
)
{
    SBUF_BUF_ID bufferPtr;

    SCIB_SEM_TAKE;
    bufferPtr = sbufAlloc(poolId,dataSize);
    sbufPoolSuspend(poolId);
    SCIB_SEM_SIGNAL;

    return bufferPtr;
}

/*******************************************************************************
*   sbufPoolFlush
*
* DESCRIPTION:
*       flush all buffers in a pool (state that all buffers are free).
*       NOTE: operation valid only if pool is suspended !!!
*
* INPUTS:
*       poolId - id of a pool.
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
void sbufPoolFlush
(
    IN  SBUF_POOL_ID    poolId
)
{
    SBUF_POOL_STC     * poolBufPtr;
    GT_U32              index,poolSize;
    SBUF_BUF_STC    *   nextFreeBufPtr;

    if (poolId == NULL) {
        sUtilsFatalError("sbufPoolFlush: illegal pointer\n");
        return;
    }

    poolBufPtr = (SBUF_POOL_STC *) poolId;
    if(poolBufPtr->poolSuspended == GT_FALSE)
    {
        sUtilsFatalError( "sbufPoolFlush: pool must be suspended for flush operation \n");
        return;
    }

    SCIB_SEM_TAKE;
    poolSize = poolBufPtr->poolSize;
    /* code similar to sbufPoolCreate(...) */
    poolBufPtr->numOfFreeBuffers = poolSize;
    poolBufPtr->firstFreeBufPtr = poolBufPtr->buffersMetadataPtr;
    for (index = 0, nextFreeBufPtr = poolBufPtr->firstFreeBufPtr;
         index < poolSize; index++, nextFreeBufPtr++)
    {
        nextFreeBufPtr->magic = SBUF_BUF_MAGIC_SIZE_CNS;
        nextFreeBufPtr->state = SBUF_BUF_STATE_FREE_E;
        nextFreeBufPtr->actualDataPtr = NULL;
        nextFreeBufPtr->actualDataSize = 0;
        /* Chain to the next free buffer if that is not last buffer in list */
        if (index != poolSize - 1)
        {
            nextFreeBufPtr->nextBufPtr = nextFreeBufPtr + 1;
        }
        else
        {
            nextFreeBufPtr->nextBufPtr = NULL;
        }
    }

    SCIB_SEM_SIGNAL;

    return ;

}

