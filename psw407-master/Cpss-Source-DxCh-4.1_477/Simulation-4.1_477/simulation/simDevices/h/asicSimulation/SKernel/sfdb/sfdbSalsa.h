/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* sfdbSalsa.h
*
* DESCRIPTION:
*       This is a external API definition for SFDB module of SKernel Salsa.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 1 $
*
*******************************************************************************/
#ifndef __sfdbsalsah
#define __sfdbsalsah

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/*******************************************************************************
*   sfdbSalsaMsgProcess
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
void sfdbSalsaMsgProcess
(
    IN SKERNEL_DEVICE_OBJECT * deviceObj,
    IN GT_U8                 * fdbMsgPtr
);
/*******************************************************************************
*   sfdbSalsaMacTableAging
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

void sfdbSalsaMacTableAging
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __sfdbsalsah */


