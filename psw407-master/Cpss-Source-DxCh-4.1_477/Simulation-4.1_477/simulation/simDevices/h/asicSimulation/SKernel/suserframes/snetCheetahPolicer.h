/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetCheetahPolicer.h
*
* DESCRIPTION:
*       (Cheetah) Policing Engine processing for frame -- simulation
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 1 $
*
*******************************************************************************/
#ifndef __snetCheetahPolicerh
#define __snetCheetahPolicerh

#include <asicSimulation/SKernel/smain/smain.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



/*******************************************************************************
*   snetChtPolicer
*
* DESCRIPTION:
*        Policer Processing  --- Policer Counters updates
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - frame data buffer Id
*
* RETURN:
*
*******************************************************************************/
GT_VOID snetChtPolicer(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr, 
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __snetCheetahPolicerh */


