/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* sEmbeddedCpu.h
*
* DESCRIPTION:
*       This is a definitions of the "embedded CPU" module of SKernel.
*
* FILE REVISION NUMBER:
*       $Revision: 2 $
*
*******************************************************************************/
#include <os/simTypes.h>
#include <asicSimulation/SKernel/smain/smain.h>

#ifndef __sEmbeddedCpuh
#define __sEmbeddedCpuh

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*******************************************************************************
*   snetEmbeddedCpuProcessInit
*
* DESCRIPTION:
*       Init the module.
*
* INPUTS:
*       deviceObjPtr   - pointer to device object.
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
void snetEmbeddedCpuProcessInit
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
);


/*******************************************************************************
*   sEmbeddedCpuRxFrame
*
* DESCRIPTION:
*        Receive frame from the PP (on the DMA) of the Embedded CPU.
* INPUTS:
*        embeddedDevObjPtr  - pointer to the Embedded CPU device object.
*        bufferId   - frame data buffer Id
*        frameData  - pointer to the data
*        frameSize  - data size
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
******************************************************************************/
void sEmbeddedCpuRxFrame
(
    IN void         * embeddedDevObjPtr,
    IN SBUF_BUF_ID    bufferId,
    IN GT_U8        * frameData,
    IN GT_U32         frameSize
);

/*******************************************************************************
*   smemEmbeddedCpuInit
*
* DESCRIPTION:
*       Init memory module for an embedded CPU device.
*
* INPUTS:
*       deviceObjPtr   - pointer to device object.
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
void smemEmbeddedCpuInit
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr
);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __sEmbeddedCpuh */


