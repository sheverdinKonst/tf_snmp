/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* simLogInfoTypeTcam.h
*
* DESCRIPTION:
*       simulation logger tcam functions
*
* FILE REVISION NUMBER:
*       $Revision: 2 $
*
*******************************************************************************/
#ifndef __simLogTcam_h__
#define __simLogTcam_h__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <os/simTypes.h>

/*******************************************************************************
* simLogTcamTTNotMatch
*
* DESCRIPTION:
*       log tcam key info
*
* INPUTS:
*       devObjPtr         - device object pointer
*       ttSearchKey16Bits - tcam key (16 bits)
*       ttSearchKey32Bits - tcam key (32 bits)
*       xdataPtr          - pointer to routing TCAM data X entry
*       xctrlPtr          - pointer to routing TCAM ctrl X entry
*       ydataPtr          - pointer to routing TCAM data Y entry
*       yctrlPtr          - pointer to routing TCAM ctrl Y entry
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       None.
*
* COMMENTS:
*
*******************************************************************************/
GT_VOID simLogTcamTTNotMatch
(
    IN SKERNEL_DEVICE_OBJECT  *devObjPtr,
    IN GT_U32                  ttSearchKey16Bits,
    IN GT_U32                  ttSearchKey32Bits,
    IN GT_U32                 *xdataPtr,
    IN GT_U32                 *ydataPtr,
    IN GT_U32                 *xctrlPtr,
    IN GT_U32                 *yctrlPtr
);

/*******************************************************************************
* simLogTcamTTKey
*
* DESCRIPTION:
*       log tcam key info
*
* INPUTS:
*       devObjPtr         - device object pointer
*       ttSearchKey16Bits - tcam key (16 bits)
*       ttSearchKey32Bits - tcam key (32 bits)
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       None.
*
* COMMENTS:
*
*******************************************************************************/
GT_VOID simLogTcamTTKey
(
    IN SKERNEL_DEVICE_OBJECT *devObjPtr,
    IN GT_U32                 ttSearchKey16Bits,
    IN GT_U32                 ttSearchKey32Bits
);

/*******************************************************************************
* simLogTcamBitsCausedNoMatchInTheEntry
*
* DESCRIPTION:
*       log tcam bits caused no match in the entry
*
* INPUTS:
*       devObjPtr - device object pointer
*       bits      - bits caused no match in the entry
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       None.
*
* COMMENTS:
*
*******************************************************************************/
GT_VOID simLogTcamBitsCausedNoMatchInTheEntry
(
    IN SKERNEL_DEVICE_OBJECT *devObjPtr,
    IN GT_U32                 bits
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __simLogTcam_h__ */

