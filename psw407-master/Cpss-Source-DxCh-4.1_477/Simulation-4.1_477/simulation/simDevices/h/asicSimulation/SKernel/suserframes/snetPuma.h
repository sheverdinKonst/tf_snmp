/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetPuma.h
*
* DESCRIPTION:
*       This is a external API definition for snet Puma module of SKernel.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 2$
*
*******************************************************************************/
#ifndef __snetPumah
#define __snetPumah


#include <asicSimulation/SKernel/smain/smain.h>
#include <common/Utils/FrameInfo/sframeInfoAddr.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*******************************************************************************
*   snetPumaProcessInit
*
* DESCRIPTION:
*       Init module.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
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
void snetPumaProcessInit
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
);

/*******************************************************************************
*   snetPumaProcessFrameFromSlan
*
* DESCRIPTION:
*       Process the frame, get and do actions for a frame
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       bufferId     - frame data buffer Id
*       srcPort      - source port number
* OUTPUTS:
*       None.
*
* RETURNS:
*      GT_OK - when the data is sent successfully.
*      GT_FAIL - in other case.
*
* COMMENTS:
*      The function is used by the FA and by the PP.
*
*******************************************************************************/
GT_VOID snetPumaProcessFrameFromSlan
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SBUF_BUF_ID bufferId,
    IN GT_U32 srcPort
);

/*******************************************************************************
*   snetGmLinkStateNotify
*
* DESCRIPTION:
*       Notify devices database that link state changed
*
* INPUTS:
*       devObjPtr - pointer to device object.
*       port      - port number.
*       linkState - link state (0 - down, 1 - up)
*
*******************************************************************************/
GT_VOID snetGmLinkStateNotify
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 port,
    IN GT_U32 linkState
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __snetPumah */


