/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* sdistributed_stubs.c
*
* DESCRIPTION:
*       stub files when distributed side is application.
*
* FILE REVISION NUMBER:
*       $Revision: 1 $
*******************************************************************************/
#include <os/simTypes.h>

#define NOT_IMPLEMENTED(func)\
    printf(" function [%s] not implemented ,in line[%ld] in file[%s] \n",#func,(GT_U32)__LINE__,__FILE__)

/*
OSTIMG_ms_in_tick  = 20
sbufDataGet  - dummy
sbufAlloc  - dummy
sbufFree  - dummy
sbufGetBufIdByData  - dummy
sbufPoolCreate  - dummy
squeBufPut   - dummy
sbufDataSet   - dummy
skernelNicOutput - dummy
skernelNicRxBind - dummy

*/

#if (defined RTOS_ON_SIM) || (defined APPLICATION_SIDE_ONLY)

typedef struct SBUF_BUF_STCT {
    GT_U32                  dummy;
} SBUF_BUF_STC;

/* Pool ID typedef */
typedef  void * SBUF_POOL_ID;

/* buffer ID typedef */
typedef SBUF_BUF_STC  * SBUF_BUF_ID;

void sbufDataGet
(
    IN  SBUF_BUF_ID     bufId,
    OUT GT_U8   **      dataPrtPtr,
    OUT GT_U32  *       dataSizePrt
)
{
    NOT_IMPLEMENTED(sbufDataGet);
    return;
}

SBUF_BUF_ID sbufAlloc
(
    IN  SBUF_POOL_ID    poolId,
    IN  GT_U32          dataSize
)
{
    NOT_IMPLEMENTED(sbufAlloc);
    return NULL;
}

void sbufFree
(
    IN  SBUF_POOL_ID    poolId,
    IN  SBUF_BUF_ID     bufId
)
{
    NOT_IMPLEMENTED(sbufFree);
    return;
}

SBUF_BUF_ID sbufGetBufIdByData(
    IN  SBUF_POOL_ID    poolId,
    IN GT_U8   *        dataPtr
)
{
    NOT_IMPLEMENTED(sbufGetBufIdByData);
    return NULL;
}

SBUF_POOL_ID sbufPoolCreate
(
    IN  GT_U32              poolSize
)
{
    NOT_IMPLEMENTED(sbufPoolCreate);
    return NULL;
}

void sbufDataSet
(
    IN  SBUF_BUF_ID     bufId,
    IN  GT_U8   *       dataPrt,
    IN  GT_U32          dataSize
)
{
    NOT_IMPLEMENTED(sbufDataSet);
    return ;
}

GT_STATUS skernelNicOutput
(
    IN GT_U8_PTR        segmentList[],
    IN GT_U32           segmentLen[],
    IN GT_U32           numOfSegments
)
{
    NOT_IMPLEMENTED(skernelNicOutput);
    return GT_OK;
}

typedef GT_STATUS (* SKERNEL_NIC_RX_CB_FUN ) (
    IN GT_U8_PTR   segmentList[],
    IN GT_U32      segmentLen[],
    IN GT_U32      numOfSegments
);

void skernelNicRxBind
(
    SKERNEL_NIC_RX_CB_FUN rxCbFun
)
{
    NOT_IMPLEMENTED(skernelNicRxBind);
    return ;
}


#endif /* (defined RTOS_ON_SIM) || (defined APPLICATION_SIDE_ONLY) */

