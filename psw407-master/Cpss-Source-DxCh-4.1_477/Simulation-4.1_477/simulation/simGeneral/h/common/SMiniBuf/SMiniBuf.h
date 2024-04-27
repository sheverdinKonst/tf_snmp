/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* sbuf.h
*
* DESCRIPTION:
*       This is a API definition for SBuf module of the Simulation.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 7 $
*
*******************************************************************************/
#ifndef __sMiniBufh
#define __sMiniBufh

#include <os/simTypes.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define SIM_CAST_MINI_BUFF(miniBuf)   ((void*)(SMINI_BUF_STC*)(miniBuf))

/*
 * Typedef: struct SBUF_BUF_STC
 *
 * Description:
 *      Describe the buffer in the simulation.
 *
 * Fields:
 *      magic           : Magic number for consistence check.
 *      nextBufPtr      : Pointer to the next buffer in the pool or queue.
 *      state           : the state of the buffer.(free/used...) internally managed
 *      bufferType      : type of the buffer , so different application know how
 *                        to treat dataPtr and cookiePtr.
 *      data            : pointer to Buffer's data memory.
 *                        (if not 0 size)will be allocated during sMiniBufPoolCreate(...)
 *                        (if not NULL)will be freed during sMiniBufPoolCreate(...)
 *      cookiePtr       : pointer to specific formate extra info.
 *                        (if not 0 size)will be allocated during sMiniBufPoolCreate(...)
 *                        (if not NULL)will be freed during sMiniBufPoolCreate(...)
 * Comments:
 */
typedef struct SMINI_BUF_STCT {
/* first fields bust be the same fields as in : SIM_BUFFER_STC */
    GT_U32                  magic;
    struct SMINI_BUF_STCT * nextBufPtr;
    GT_U32                  state;
    GT_U32                  bufferType;
    void*                   dataPtr;
    void*                   cookiePtr;
} SMINI_BUF_STC;

/* Pool ID typedef */
typedef  void * SMINI_BUF_POOL_ID;

/* API functions */

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
);

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
);

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
);
/*******************************************************************************
*   sMiniBufAlloc
*
* DESCRIPTION:
*       Allocate buffer. (according to size giver in sMiniBufPoolCreate(...))
*
* INPUTS:
*       poolId   - id of a pool
*
* OUTPUTS:
*       None.
*
* RETURNS:
*          pointer to buffer info.(NULL)
*
* COMMENTS:
*
*
*******************************************************************************/
SMINI_BUF_STC* sMiniBufAlloc
(
    IN  SMINI_BUF_POOL_ID    poolId
);

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
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __sMiniBufh */


