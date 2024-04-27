/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* simOsSlan.h
*
* DESCRIPTION:    SLAN API
*
* DEPENDENCIES:   Operating System.
*
* FILE REVISION NUMBER:
*       $Revision: 2 $
*******************************************************************************/

#ifndef EXPLICIT_INCLUDE_TO_SIM_OS_H_FILES
    #error "include to those H files should be only for bind purposes"
#endif /*!EXPLICIT_INCLUDE_TO_SIM_OS_H_FILES*/

#ifndef __simOsSlanh
#define __simOsSlanh

#include <os/simTypes.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



/*******************************************************************************
*   simOsSlanBind
*
* DESCRIPTION:
*       Binds a slan to client.
*
* INPUTS:
*       slanNamePtr   - pointer to slan name.
*       clientNamePtr - pointer to client name.
*       funcPtr       - pointer to function
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       Pointer to void
*
* COMMENTS:
*
*
*******************************************************************************/
extern SIM_OS_SLAN_ID simOsSlanBind (
    char                            *slanNamePtr,
    char                            *clientNamePtr,
    void                            *usrInfoPtr,
    SIM_OS_SLAN_RCV_FUN     funcPtr

);
/*******************************************************************************
*   simOsSlanTransmit
*
* DESCRIPTION:
*       Transmit a message from slan id.
*
* INPUTS:
*       slanId         -  slan id.
*       msgCode         - message code.
*       len             - message length.
*       msgPtr          - pointer to the message
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
extern unsigned int simOsSlanTransmit (
    SIM_OS_SLAN_ID              slanId,
    SIM_OS_SLAN_MSG_TYPE_ENT    msgType,
    GT_U32                      len,
    char                        *msgPtr
);

/*******************************************************************************
*   simOsSlanUnbind
*
* DESCRIPTION:
*       Unbinds a client from slan.
*
* INPUTS:
*       slanId   - slan id.
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
extern void simOsSlanUnbind (
    SIM_OS_SLAN_ID  slanId
);

/*******************************************************************************
*   simOsSlanInit
*
* DESCRIPTION:
*       Calls Linux related slan-like initialization.
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
extern void simOsSlanInit (
    void
);

/*******************************************************************************
*   simOsSlanStart
*
* DESCRIPTION:
*       Calls Linux related slan-like start mechanism.
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
extern void simOsSlanStart (
    void
);


/*******************************************************************************
*   simOsChangeLinkStatus
*
* DESCRIPTION:
*       Change the state of link for the SLAN .
*
* INPUTS:
*       slanId   - slan id.
*       linkState - 1 for up , 0 for down.
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
extern void simOsChangeLinkStatus
(
    SIM_OS_SLAN_ID  slanId ,
    GT_BOOL      linkState
);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif   /* __simOsSlanh */


