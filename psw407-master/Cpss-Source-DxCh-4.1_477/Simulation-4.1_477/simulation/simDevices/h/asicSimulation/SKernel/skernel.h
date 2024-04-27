/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* skernel.h
*
* DESCRIPTION:
*       This is a external API definition for SKernel simulation.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 3 $
*
*******************************************************************************/
#ifndef __skernelh
#define __skernelh

#ifdef APPLICATION_SIDE_ONLY
    /* this file exists only when it is not application side only , because those
       files are on the devices side */
    #error "this file exists only when it is not application side only"
#endif /*APPLICATION_SIDE_ONLY*/

#include <asicSimulation/SInit/sinit.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* typedef of Rx NIC callback function */
typedef GT_STATUS (* SKERNEL_NIC_RX_CB_FUN ) (
    IN GT_U8_PTR   segmentList[],
    IN GT_U32      segmentLen[],
    IN GT_U32      numOfSegments
);

/*******************************************************************************
*   skernelInit
*
* DESCRIPTION:
*       Init SKernel library.
*
* INPUTS:
*       None.
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
void skernelInit
(
    void
);

/*******************************************************************************
*   skernelShutDown
*
* DESCRIPTION:
*       Shut down SKernel library before reboot.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*            The function unbinds ports from SLANs
*
*******************************************************************************/
void skernelShutDown
(
    void
);
/*******************************************************************************
*   skernelNicRxBind
*
* DESCRIPTION:
*       Bind Rx callback routine with skernel.
*
* INPUTS:
*       rxCbFun - callback function for NIC Rx.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void skernelNicRxBind
(
    SKERNEL_NIC_RX_CB_FUN rxCbFun
);
/*******************************************************************************
*   skernelNicRxUnBind
*
* DESCRIPTION:
*       UnBind Rx callback routine with skernel.
*
* INPUTS:
*       rxCbFun - callback function for unbind NIC Rx.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void skernelNicRxUnBind
(
    SKERNEL_NIC_RX_CB_FUN rxCbFun
);

/*******************************************************************************
* skernelNicOutput
*
* DESCRIPTION:
*       This function transmits an Ethernet packet to the NIC
*
* INPUTS:
*       header_PTR  - pointer the 14 bytes packet header.
*       payload_PTR - pointer to the reset of the packet.
*       payloadLen  - length of the payload.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK if successful, or
*       GT_FAIL otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
GT_STATUS skernelNicOutput
(
    IN GT_U8_PTR        segmentList[],
    IN GT_U32           segmentLen[],
    IN GT_U32           numOfSegments
);

/*******************************************************************************
*   skernelStatusGet
*
* DESCRIPTION:
*       Get status (Idle or Busy) of all Simulation Kernel tasks.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       0 - Simulation Kernel Tasks are Idle
*       other - Simulation Kernel Tasks are busy
* COMMENTS:
*          
*
*******************************************************************************/
GT_U32 skernelStatusGet
(
    void
);

#define ASSERT_PTR(ptr)   \
    if (ptr == 0) skernelFatalError("Access violation: illegal pointer\n");

#define NOT_VALID_ADDR  0xBAD0ADD0

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __skernelh */


