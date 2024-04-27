/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* sfdb.h
*
* DESCRIPTION:
*       This is a external API definition for SFDB module of SKernel.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 9 $
*
*******************************************************************************/
#ifndef __sfdbh
#define __sfdbh

#include <common/Utils/FrameInfo/sframeInfoAddr.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * typedef: enum SFDB_UPDATE_MSG_TYPE
 *
 * Description: Enum for FDB update message type
 *
 * Enumerations:
 *      SFDB_UPDATE_MSG_NA_E - New address
 *      SFDB_UPDATE_MSG_QA_E -  Query Address
 *      SFDB_UPDATE_MSG_QR_E  - Query Respond
 *      SFDB_UPDATE_MSG_AA_E  - Aged Out Address
 *      SFDB_UPDATE_MSG_TA_E  - Transplanted Address
 *      SFDB_UPDATE_MSG_FU_E  - FDB Upload message
 *      SFDB_UPDATE_MSG_QI_E  - Index Query Request and Index Query Respond
 *      SFDB_UPDATE_MSG_HR_E  - FDB Hash Request
 *
 */
typedef enum
{
    SFDB_UPDATE_MSG_NA_E = 0,
    SFDB_UPDATE_MSG_QA_E,/*1*/
    SFDB_UPDATE_MSG_QR_E,/*2*/
    SFDB_UPDATE_MSG_AA_E,/*3*/
    SFDB_UPDATE_MSG_TA_E,/*4*/
    SFDB_UPDATE_MSG_FU_E,/*5*/
    SFDB_UPDATE_MSG_QI_E,/*6*/
    SFDB_UPDATE_MSG_HR_E,/*7*/
} SFDB_UPDATE_MSG_TYPE;

/*******************************************************************************
*   sfdbMsgProcess
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
void sfdbMsgProcess
(
    IN SKERNEL_DEVICE_OBJECT * deviceObj,
    IN GT_U8                 * fdbMsgPtr
);

/*******************************************************************************
*   sfdbMsg2Mac
*
* DESCRIPTION:
*       Get mac address from message.
*
* INPUTS:
*       msgPtr     - pointer to first word of message.
*
* OUTPUTS:
*       macAddrPtr - pointer to the allocated mac address.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/

void sfdbMsg2Mac
(
    IN  GT_U32            *  msgPtr,
    OUT SGT_MAC_ADDR_TYP  *  macAddrPtr
);
/*******************************************************************************
*   sfdbMsg2Vid
*
* DESCRIPTION:
*       Get VID from message.
*
* INPUTS:
*       msgPtr   - pointer to first word of message.
*
* OUTPUTS:
*       vid - pointer to the allocated vid memory.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/

void sfdbMsg2Vid
(
    IN  GT_U32            *  msgPtr,
    OUT GT_U16            *  vidPtr
);

/*******************************************************************************
*   sfdbMacTableTriggerAction
*
* DESCRIPTION:
*       Process triggered MAC table entries.
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       fdbMsgPtr   - pointer to fdb message.
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
void sfdbMacTableTriggerAction
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN GT_U8                 * fdbMsgPtr
);

/*******************************************************************************
*   sfdbMacTableAutomaticAging
*
* DESCRIPTION:
*       Process triggered MAC table entries.
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       data_PTR    - pointer to aging information like entries and index.
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
void sfdbMacTableAutomaticAging
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN GT_U8                 * data_PTR
);
/*******************************************************************************
*   sfdbMacTableAging
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
void sfdbMacTableAging(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr
);

/*******************************************************************************
*   sfdbMacTableUploadAction
*
* DESCRIPTION:
*       FDB upload action
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
void sfdbMacTableUploadAction
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN GT_U8                 * tblActPtr
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __sfdbh */


