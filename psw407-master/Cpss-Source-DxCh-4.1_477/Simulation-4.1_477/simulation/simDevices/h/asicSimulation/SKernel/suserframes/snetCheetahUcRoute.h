/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetCheetahUcRoute.h
*
* DESCRIPTION:
*       This is a external API definition for Router Engine Processing
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 2 $
*
*******************************************************************************/
#ifndef __snetCheetahUcRouteh
#define __snetCheetahUcRouteh

#include <asicSimulation/SKernel/smain/smain.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#ifdef __cplusplus
}
#endif /* __cplusplus */

/*******************************************************************************
*   snetChtUcRouting
*
* DESCRIPTION:
*        (Cheetah) IPv4 and IPv6 Unicast Routing
*
*
* INPUTS:
*       devObjPtr    - (pointer to) device object.
*       descrPtr     - (pointer to) frame data buffer Id
* OUTPUTS:
*       descrPtr     - (pointer to) updated frame data buffer Id
*
* RETURN:
*
*******************************************************************************/
GT_VOID  snetChtUcRouting(
    SKERNEL_DEVICE_OBJECT * devObjPtr,
    SKERNEL_FRAME_CHEETAH_DESCR_STC *  descrPtr
);

#endif  /* __snetCheetahUcRouteh */


