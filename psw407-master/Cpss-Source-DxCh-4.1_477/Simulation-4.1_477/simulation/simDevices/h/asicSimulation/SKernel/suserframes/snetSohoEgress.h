/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetSohoEgress.h
*
* DESCRIPTION:
*       API declaration and data type definition for Soho Egress Processing
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 1 $
*
*******************************************************************************/
#ifndef __snetSohoEgressh
#define __snetSohoEgressh

#include <asicSimulation/SKernel/smain/smain.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*******************************************************************************
*   snetSohoEgressPacketProc
*
* DESCRIPTION:
*       Egress frame process
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPrt    - pointer to the frame's descriptor.
*
*
*******************************************************************************/
GT_VOID snetSohoEgressPacketProc
(
    IN SKERNEL_DEVICE_OBJECT        *     devObjPtr,
    IN SKERNEL_FRAME_SOHO_DESCR_STC *     descrPrt
);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __snetSohoEgressh */


