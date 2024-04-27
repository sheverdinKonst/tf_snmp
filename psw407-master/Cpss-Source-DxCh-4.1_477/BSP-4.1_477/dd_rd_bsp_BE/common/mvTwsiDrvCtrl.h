/*******************************************************************************
*              Copyright 2001, GALILEO TECHNOLOGY, LTD.
*
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL. NO RIGHTS ARE GRANTED
* HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT OF MARVELL OR ANY THIRD
* PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE DISCRETION TO REQUEST THAT THIS
* CODE BE IMMEDIATELY RETURNED TO MARVELL. THIS CODE IS PROVIDED "AS IS".
* MARVELL MAKES NO WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS
* ACCURACY, COMPLETENESS OR PERFORMANCE. MARVELL COMPRISES MARVELL TECHNOLOGY
* GROUP LTD. (MTGL) AND ITS SUBSIDIARIES, MARVELL INTERNATIONAL LTD. (MIL),
* MARVELL TECHNOLOGY, INC. (MTI), MARVELL SEMICONDUCTOR, INC. (MSI), MARVELL
* ASIA PTE LTD. (MAPL), MARVELL JAPAN K.K. (MJKK), GALILEO TECHNOLOGY LTD. (GTL)
* AND GALILEO TECHNOLOGY, INC. (GTI).
********************************************************************************
* mvTwsiDrvCtrl.h
*
* DESCRIPTION:
*       API implementation for TWSI facilities.
*
* DEPENDENCIES:
*
*       $Revision: 4 $
*******************************************************************************/

#ifndef __mvTwsiDrvCtrl
#define __mvTwsiDrvCtrl

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "mvGenTypes.h"

/*******************************************************************************
* mvDirectTwsiInitDriver
*
* DESCRIPTION:
*       Init the TWSI interface
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK               - on success
*       GT_ERROR   - on hardware error
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS mvDirectTwsiInitDriver
(
    GT_VOID
);

/*******************************************************************************
* mvDirectTwsiWaitNotBusy
*
* DESCRIPTION:
*       Wait for TWSI interface not BUSY
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK               - on success
*       GT_ERROR   - on hardware error
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS mvDirectTwsiWaitNotBusy
(
    GT_VOID
);

/*******************************************************************************
* mvDirectTwsiMasterReadTrans
*
* DESCRIPTION:
*       do TWSI interface Transaction
*
* INPUTS:
*    devId - I2c slave ID
*    pData - Pointer to array of chars (address / data)
*    len   - pData array size (in chars).
*    stop  - Indicates if stop bit is needed.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK               - on success
*       GT_ERROR   - on hardware error
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS mvDirectTwsiMasterReadTrans
(
    IN GT_U8   devId,               /* I2c slave ID                              */
    IN GT_U8 * pData,               /* Pointer to array of chars (address / data)*/
    IN GT_U8   len,                 /* pData array size (in chars).              */
    IN GT_BOOL stop                 /* Indicates if stop bit is needed in the end  */
);

/*******************************************************************************
* mvDirectTwsiMasterWriteTrans
*
* DESCRIPTION:
*       do TWSI interface Transaction
*
* INPUTS:
*    devId - I2c slave ID
*    pData - Pointer to array of chars (address / data)
*    len   - pData array size (in chars).
*    stop  - Indicates if stop bit is needed.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK               - on success
*       GT_ERROR   - on hardware error
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS mvDirectTwsiMasterWriteTrans
(
    IN GT_U8   devId,               /* I2c slave ID                              */
    IN GT_U8 * pData,               /* Pointer to array of chars (address / data)*/
    IN GT_U8   len,                 /* pData array size (in chars).              */
    IN GT_BOOL stop                 /* Indicates if stop bit is needed in the end  */
);

/*******************************************************************************
* mvDirectTwsiSetMuxChannel
*
* DESCRIPTION:
*       Set the TWSI mux channel
*
* INPUTS:
*    channel - the mux channel to set
*    enable  - enable the channel
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK               - on success
*       GT_ERROR   - on hardware error
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS mvDirectTwsiSetMuxChannel
(
    IN GT_U32  channel /* mux channel */,
    IN GT_BOOL enable
);

/*******************************************************************************
* mvTwsiReadWithOffset
*
* DESCRIPTION:
*       do TWSI interface Transaction
*
* INPUTS:
*    devId - I2c slave ID
*    pData - Pointer to array of chars (address / data)
*    len   - pData array size (in chars).
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK               - on success
*       GT_ERROR   - on hardware error
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS mvTwsiReadWithOffset
(
    IN GT_U8  twsiAddr,             /* I2c slave address            */
    IN GT_U8  offset,               /* register address             */
    IN GT_U8 *pData,                /* Pointer to array of chars    */
    IN GT_U8  len                   /* data length                  */
);

/*******************************************************************************
* mvTwsiWriteWithOffset
*
* DESCRIPTION:
*       do TWSI interface Transaction
*
* INPUTS:
*    devId - I2c slave ID
*    pData - Pointer to array of chars (address / data)
*    len   - pData array size (in chars).
*    stop  - Indicates if stop bit is needed.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK               - on success
*       GT_ERROR   - on hardware error
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS mvTwsiWriteWithOffset
(
    IN GT_U8  twsiAddr,             /* I2c slave address            */
    IN GT_U8  offset,               /* register address             */
    IN GT_U8 *pData,                /* Pointer to array of chars    */
    IN GT_U8  len                   /* data length                  */
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __mvTwsiDrvCtrl */

