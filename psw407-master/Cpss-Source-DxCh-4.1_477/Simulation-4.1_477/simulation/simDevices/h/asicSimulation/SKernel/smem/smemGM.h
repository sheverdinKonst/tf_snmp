/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* smemGM.h
*
* DESCRIPTION:
*       Data definitions for Twist memories.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 2 $
*
*******************************************************************************/
#ifndef __smemGmh
#define __smemGmh

#include <asicSimulation/SKernel/smem/smem.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define SMEM_GM_GET_GM_DEVICE_ID(deviceObjPtr) \
    ((deviceObjPtr->portGroupSharedDevObjPtr == NULL) ? deviceObjPtr->deviceId : \
    deviceObjPtr->portGroupSharedDevObjPtr->deviceId )

/*******************************************************************************
*   smemGmInit
*
* DESCRIPTION:
*       Init memory module for a GM Puma device.
*
* INPUTS:
*       deviceObjPtr   - pointer to device object.
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
void smemGmInit
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr
);

/*******************************************************************************
*   smemGmInit2
*
* DESCRIPTION:
*       Init memory module for a GM device (Phase 2).
*
* INPUTS:
*       deviceObjPtr   - pointer to device object.
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
void smemGmInit2
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __smemGmh */


