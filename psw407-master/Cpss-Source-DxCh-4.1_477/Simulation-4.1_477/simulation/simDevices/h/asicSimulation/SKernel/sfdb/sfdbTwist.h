/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* sfdbTwist.h
*
* DESCRIPTION:
*       This is a external API definition for SFDB module of SKernel Twist.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 2 $
*
*******************************************************************************/
#ifndef __sfdbtwisth
#define __sfdbtwisth

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/*******************************************************************************
*   sfdbTwistMsgProcess
*
* DESCRIPTION:
*       Process FDB update message.
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       fdbMsgPtr   - pointer to device object.
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
void sfdbTwistMsgProcess
(
    IN SKERNEL_DEVICE_OBJECT * deviceObj,
    IN GT_U8                 * fdbMsgPtr
);
/*******************************************************************************
*   sfdbTwistMacTableAging
*
* DESCRIPTION:
*       Age out MAC table entries.
*
* INPUTS:
*       deviceObj   - pointer to device object.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/

void sfdbTwistMacTableAging
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr
);

/*******************************************************************************
*   sfdbTwistMacTableTriggerAction
*
* DESCRIPTION:
*       MAC Table Trigger Action
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       tblActPtr   - pointer table action data
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
void sfdbTwistMacTableTriggerAction
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U8                 * tblActPtr
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __sfdbtwisth */


