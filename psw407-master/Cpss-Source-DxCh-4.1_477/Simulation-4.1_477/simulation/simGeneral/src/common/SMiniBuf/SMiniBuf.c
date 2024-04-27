/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* sbuf.c
*
* DESCRIPTION:
*       The module is mini buffer management utility for simulation modules.
*
* FILE REVISION NUMBER:
*       $Revision: 2 $
*******************************************************************************/
#include <os/simTypesBind.h>
#include <common/SMiniBuf/SMiniBuf.h>
#include <common/Utils/Error/serror.h>

/*******************************************************************************
* Private type definition
*******************************************************************************/
/* constants */

/* Memory validation on free memory */
#define SBUF_BUF_MAGIC_SIZE_CNS             0xa5a5a551

#define FREE_PTR(ptr) if(ptr) free(ptr); ptr = NULL

#define BUFFER_STATE_FREE   0
#define BUFFER_STATE_USED   1

/* macro to check that the buffer was not corrupted */
#define MAGIC_CHECK(bufPtr) \
    if ( bufPtr->magic != SBUF_BUF_MAGIC_SIZE_CNS )\
    {                                              \
        sUtilsFatalError(" illegal buffer %p : in line[%d]\n", bufPtr,__LINE__);\
    }

/*
 * Typedef: struct SBUF_MINI_POOL_STC
 *
 * Description:
 *      Describe the buffers pool in the simulation.
 *
 * Fields:
 *      poolSize        : Number of buffers in a pool,
 *                      :   0 means that pool is free.
 *      dataPtr         : Pointer to allocated memory for all buffers.
 *      firstFreeBufPtr : Pointer to the first free buffer. Buffer pool
 *                      : is managed in "Stack Like" way - this is
 *                      : "top of the stack"
 *      criticalSection : Critical section object to protect a pool.
 * Comments:
 */
typedef struct  {
    GT_U32                  poolSize;
    SMINI_BUF_STC        *  dataPtr;
    SMINI_BUF_STC        *  firstFreeBufPtr;
    GT_MUTEX                criticalSection;
} SBUF_MINI_POOL_STC;


/* Pointer to the pools array */
static  SBUF_MINI_POOL_STC  * sbufPoolsPtr;

/* Number of pools in the pools array */
static GT_U32            sbufPoolsNumber;

/*******************************************************************************
*   sMiniBufInit
*
* DESCRIPTION:
*       Initialize internal structures for pools and buffers management.
*
* INPUTS:
*       maxPoolsNum - maximal number of buffer pools.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*       In the case of memory lack the function aborts application
*
*******************************************************************************/
void sMiniBufInit
(
    IN  GT_U32              maxPoolsNum
)
{
    GT_U32              i;
    GT_U32              size;

    if (maxPoolsNum == 0)
    {
        sUtilsFatalError("sMiniBufInit: illegal maxPoolsNum %lu\n", maxPoolsNum);
    }

    /* Allocate pull array */
    size = maxPoolsNum * sizeof(SBUF_MINI_POOL_STC);
    sbufPoolsNumber = maxPoolsNum;
    sbufPoolsPtr = malloc(size);
    if ( sbufPoolsPtr == NULL )
    {
        sUtilsFatalError("sbufInit: buffer initialization error\n");
    }
    memset(sbufPoolsPtr, 0, size);
    /* Initialize critical section for every buffer pool in the pool array */
    for ( i = 0; i < sbufPoolsNumber; i++)
    {
        sbufPoolsPtr[i].criticalSection = SIM_OS_MAC(simOsMutexCreate)();
    }
}


/*******************************************************************************
*   sMiniBufPoolCreate
*
* DESCRIPTION:
*       Create new buffers pool.
*
* INPUTS:
*       poolSize - number of buffers in a pool.
*       buffersDataSize - the number of bytes for data in each buffer to be managed by
*                       this buffer.(can be 0 for NULL)
*       buffersCookieSize - the number of bytes in each buffer to be managed by
*                       this buffer.(can be 0 for NULL)
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
SMINI_BUF_POOL_ID sMiniBufPoolCreate
(
    IN  GT_U32              poolSize,
    IN  GT_U32              buffersDataSize,
    IN  GT_U32              buffersCookieSize
)
{
    GT_U32              i,j;
    GT_U32              size;
    SMINI_BUF_STC    *  nextFreeBufPtr;

    if (poolSize == 0)
    {
        sUtilsFatalError("sMiniBufPoolCreate: illegal poolSize %lu\n", poolSize);
    }

    for (i = 0; i < sbufPoolsNumber; i++)
    {
        if (sbufPoolsPtr[i].poolSize == 0)
        {
            break;
        }
    }
    if (i == sbufPoolsNumber)
    {
        sUtilsFatalError("sMiniBufPoolCreate: memory lack in the pool array\n");
    }
    /* Create pull buffers and initialize linked list */
    size = poolSize * sizeof(SMINI_BUF_STC);

    sbufPoolsPtr[i].poolSize = poolSize;
    sbufPoolsPtr[i].dataPtr = malloc(size);
    memset(sbufPoolsPtr[i].dataPtr, 0, size);
    sbufPoolsPtr[i].firstFreeBufPtr = sbufPoolsPtr[i].dataPtr;

    for (j = 0, nextFreeBufPtr = sbufPoolsPtr[i].firstFreeBufPtr;
         j < poolSize;
         j++, nextFreeBufPtr++)
    {
        nextFreeBufPtr->magic = SBUF_BUF_MAGIC_SIZE_CNS;
        nextFreeBufPtr->state = BUFFER_STATE_FREE;
        nextFreeBufPtr->bufferType = 0;
        nextFreeBufPtr->dataPtr = buffersDataSize ? malloc(buffersDataSize) : NULL;
        nextFreeBufPtr->cookiePtr = buffersCookieSize ? malloc(buffersCookieSize) : NULL;

        if((buffersDataSize && nextFreeBufPtr->dataPtr == NULL) ||
           (buffersCookieSize && nextFreeBufPtr->cookiePtr == NULL))
        {
            sUtilsFatalError("sMiniBufPoolCreate: memory lack in the cookie/data array\n");
        }

        /* Chain to the next free buffer if that is not last buffer in list */
        if (j != poolSize - 1)
        {
            nextFreeBufPtr->nextBufPtr = nextFreeBufPtr + 1;
        }
    }

    return (SMINI_BUF_POOL_ID)&sbufPoolsPtr[i];
}


/*******************************************************************************
*   sMiniBufPoolFree
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
void sMiniBufPoolFree
(
    IN  SMINI_BUF_POOL_ID    poolId
)
{
    GT_U32              j;
    SBUF_MINI_POOL_STC    *  pullBufPtr;
    SMINI_BUF_STC    *  nextBufPtr;

    if (poolId == NULL)
    {
        sUtilsFatalError("sMiniBufPoolFree: illegal pointer\n");
    }

    pullBufPtr = (SBUF_MINI_POOL_STC *) poolId;

    nextBufPtr = pullBufPtr->dataPtr;
    for (j = 0; j < pullBufPtr->poolSize; j++, nextBufPtr++)
    {
        MAGIC_CHECK(nextBufPtr);
        FREE_PTR(nextBufPtr->dataPtr);
        FREE_PTR(nextBufPtr->cookiePtr);
    }

    /* Free all allocated buffers and reset all properties for that poolId*/
    FREE_PTR(pullBufPtr->dataPtr);
    memset(pullBufPtr, 0, sizeof(SBUF_MINI_POOL_STC));
}


/*******************************************************************************
*   sMiniBufAlloc
*
* DESCRIPTION:
*       Allocate buffer. (according to size giver in sMiniBufPoolCreate(...))
*
* INPUTS:
*       poolId   - id of a pool.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*          pointer to buffer info.(or NULL)
*
* COMMENTS:
*
*
*******************************************************************************/
SMINI_BUF_STC* sMiniBufAlloc
(
    IN  SMINI_BUF_POOL_ID    poolId
)
{
    SBUF_MINI_POOL_STC     * poolBufPtr;
    SMINI_BUF_STC      * freeBufPtr;

    if (poolId == NULL)
    {
        sUtilsFatalError("sMiniBufAlloc: illegal pointer\n");
    }

    poolBufPtr = (SBUF_MINI_POOL_STC *) poolId;

    SIM_OS_MAC(simOsMutexLock)(poolBufPtr->criticalSection);

    /* Get first free buffer from the pool */
    freeBufPtr = poolBufPtr->firstFreeBufPtr;
    if (freeBufPtr != NULL)
    {
        /* Forward free buffer pointer to the next buffer */
        poolBufPtr->firstFreeBufPtr = freeBufPtr->nextBufPtr;
    }

    SIM_OS_MAC(simOsMutexUnlock)(poolBufPtr->criticalSection);

    /* Set buffer attributes and mark his state as SBUF_BUF_STATE_BUZY_E */
    if (freeBufPtr != NULL )
    {
        MAGIC_CHECK(freeBufPtr);
        freeBufPtr->nextBufPtr      = NULL;
        freeBufPtr->state           = BUFFER_STATE_USED;
    }

    return freeBufPtr;

}

/*******************************************************************************
*   sMiniBufFree
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
void sMiniBufFree
(
    IN  SMINI_BUF_POOL_ID    poolId,
    IN  SMINI_BUF_STC        *bufIdPtr
)
{
    SBUF_MINI_POOL_STC     * poolBufPtr;

    if (poolId == NULL)
    {
        sUtilsFatalError("sMiniBufFree: illegal pointer\n");
    }

    poolBufPtr = (SBUF_MINI_POOL_STC *)poolId;

    if (bufIdPtr == NULL)
    {
        sUtilsFatalError("sMiniBufFree: illegal pointer\n");
    }

    MAGIC_CHECK(bufIdPtr);

    if ( bufIdPtr->state == BUFFER_STATE_FREE )
    {
        sUtilsFatalError("sMiniBufFree : buffer already free\n") ;
    }

    bufIdPtr->bufferType = 0;

    /*EnterCriticalSection(&poolBufPtr->criticalSection);*/
    SIM_OS_MAC(simOsMutexLock)(poolBufPtr->criticalSection);

    /* Put back buffer in pool (head of pool) */
    bufIdPtr->nextBufPtr = poolBufPtr->firstFreeBufPtr;
    poolBufPtr->firstFreeBufPtr = bufIdPtr;

    /*LeaveCriticalSection(&poolBufPtr->criticalSection);*/
    SIM_OS_MAC(simOsMutexUnlock)(poolBufPtr->criticalSection);
}

