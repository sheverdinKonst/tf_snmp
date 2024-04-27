/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* sfdb.c
*
* DESCRIPTION:
*       This is a SFDB module of SKernel.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 16 $
*
*******************************************************************************/

#include <os/simTypes.h>
#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SKernel/smain/smain.h>
#include <asicSimulation/SKernel/sfdb/sfdbSalsa.h>
#include <asicSimulation/SKernel/sfdb/sfdbTwist.h>
#include <asicSimulation/SKernel/sfdb/sfdbSoho.h>
#include <asicSimulation/SKernel/sfdb/sfdbCheetah.h>
#include <common/Utils/FrameInfo/sframeInfoAddr.h>

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
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN GT_U8                 * fdbMsgPtr
)
{
    ASSERT_PTR(deviceObjPtr);

    if (deviceObjPtr->devFdbMsgProcFuncPtr == NULL)
        return ;

    deviceObjPtr->devFdbMsgProcFuncPtr(deviceObjPtr, fdbMsgPtr);
}
/*******************************************************************************
*   sfdbMsg2Mac
*
* DESCRIPTION:
*       Get mac address from message.
*
* INPUTS:
*       msgPtr   - pointer to first word of message.
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
)
{
    macAddrPtr->bytes[5] = (GT_U8)((msgPtr[0] >> 16) & 0xff);
    macAddrPtr->bytes[4] = (GT_U8)((msgPtr[0] >> 24) & 0xff);
    macAddrPtr->bytes[3] = (GT_U8)(msgPtr[1] & 0xff);
    macAddrPtr->bytes[2] = (GT_U8)((msgPtr[1] >> 8) & 0xff);
    macAddrPtr->bytes[1] = (GT_U8)((msgPtr[1] >> 16) & 0xff);
    macAddrPtr->bytes[0] = (GT_U8)((msgPtr[1] >> 24) & 0xff);
}

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
)
{
    *vidPtr = (GT_U16)(msgPtr[2] & 0xfff);
}


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
)
{
    if (deviceObjPtr->devMacTblAgingProcFuncPtr != NULL)
       deviceObjPtr->devMacTblAgingProcFuncPtr(deviceObjPtr);
}


/*******************************************************************************
*   sfdbMacTableTriggerAction
*
* DESCRIPTION:
*       Process triggered MAC table entries.
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       tblActPtr   - pointer to registr of fdb action to perform
*
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
    IN GT_U8                 * tblActPtr
)
{
    ASSERT_PTR(deviceObjPtr);

    if (deviceObjPtr->devMacTblTrigActFuncPtr == NULL)
        return ;

    deviceObjPtr->devMacTblTrigActFuncPtr(deviceObjPtr, tblActPtr);
}

/*******************************************************************************
*   sfdbMacTableUploadAction
*
* DESCRIPTION:
*       Process triggered MAC table entries.
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       tblActPtr   - pointer to registr of fdb action to perform
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
)
{
    ASSERT_PTR(deviceObjPtr);

    if (deviceObjPtr->devMacTblUploadProcFuncPtr == NULL)
        return ;

    deviceObjPtr->devMacTblUploadProcFuncPtr(deviceObjPtr, tblActPtr);
}

/*******************************************************************************
*   sfdbMacTableAutomaticAging
*
* DESCRIPTION:
*       Process automatic aging for the fdb table.
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       data_PTR    - pointer to aging information like entries and index.
*
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
)
{
    ASSERT_PTR(deviceObjPtr);

    if (deviceObjPtr->devMacEntriesAutoAgingFuncPtr == NULL)
        return ;

    deviceObjPtr->devMacEntriesAutoAgingFuncPtr(deviceObjPtr, data_PTR);
}

