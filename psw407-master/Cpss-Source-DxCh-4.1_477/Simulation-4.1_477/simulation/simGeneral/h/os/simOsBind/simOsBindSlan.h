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
*      simOsSlanBind              SIM_OS_SLAN_BIND_FUN
*      simOsSlanTransmit          SIM_OS_SLAN_TRANSMIT_FUN
*      simOsSlanUnbind            SIM_OS_SLAN_UNBIND_FUN
*      simOsSlanInit              SIM_OS_SLAN_INIT_FUN
*      simOsSlanStart             SIM_OS_SLAN_START_FUN
*      simOsChangeLinkStatus      SIM_OS_CHANGE_LINK_STATUS_FUN
*
* FILE REVISION NUMBER:
*       $Revision: 1 $
*******************************************************************************/

#ifndef __simOsBindSlanh
#define __simOsBindSlanh

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef void * SIM_OS_SLAN_ID;

/* this msg has no data in it just notify the user */
#define SIM_OS_SLAN_NORMAL_MSG_RSN_CNS       0

/* call the client for a buffer space */
#define SIM_OS_SLAN_GET_BUFF_RSN_CNS         1

/* success read */
#define SIM_OS_SLAN_GIVE_BUFF_SUCCS_RSN_CNS  2

/* buffer allocation or read failed, empty buffer */
#define SIM_OS_SLAN_GIVE_BUFF_ERR_RSN_CNS    3


typedef enum {

    SIM_OS_SLAN_MSG_CODE_FRAME_CNS

} SIM_OS_SLAN_MSG_TYPE_ENT;


typedef char*  (*SIM_OS_SLAN_RCV_FUN)(
    GT_U32         msg_code,
    GT_U32         Reason,
    GT_U32         sender_tid,
    GT_U32         len,
    void         *usr_info,
    char        *buff
) ;


/*******************************************************************************
*   SIM_OS_SLAN_BIND_FUN
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
typedef SIM_OS_SLAN_ID (*SIM_OS_SLAN_BIND_FUN)
(
    char                            *slanNamePtr,
    char                            *clientNamePtr,
    void                            *usrInfoPtr,
    SIM_OS_SLAN_RCV_FUN             funcPtr
);
/*******************************************************************************
*   SIM_OS_SLAN_TRANSMIT_FUN
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
typedef unsigned int (*SIM_OS_SLAN_TRANSMIT_FUN)
(
    SIM_OS_SLAN_ID              slanId,
    SIM_OS_SLAN_MSG_TYPE_ENT    msgType,
    GT_U32                      len,
    char                        *msgPtr
);

/*******************************************************************************
*   SIM_OS_SLAN_UNBIND_FUN
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
typedef void (*SIM_OS_SLAN_UNBIND_FUN)
(
    SIM_OS_SLAN_ID  slanId
);

/*******************************************************************************
*   SIM_OS_SLAN_INIT_FUN
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
typedef void (*SIM_OS_SLAN_INIT_FUN)
(
    void
);

/*******************************************************************************
*   SIM_OS_SLAN_START_FUN
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
typedef void (*SIM_OS_SLAN_START_FUN)
(
    void
);


/*******************************************************************************
*   SIM_OS_CHANGE_LINK_STATUS_FUN
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
typedef void (*SIM_OS_CHANGE_LINK_STATUS_FUN)
(
    SIM_OS_SLAN_ID  slanId ,
    GT_BOOL      linkState
);


/* SIM_OS_FUNC_BIND_SLAN_STC -
*    structure that hold the "os slan" functions needed be bound to SIM.
*
*/
typedef struct{

    SIM_OS_SLAN_BIND_FUN              simOsSlanBind;           /* needed only on devices side */
    SIM_OS_SLAN_TRANSMIT_FUN          simOsSlanTransmit;       /* needed only on devices side */
    SIM_OS_SLAN_UNBIND_FUN            simOsSlanUnbind;         /* needed only on devices side */
    SIM_OS_SLAN_INIT_FUN              simOsSlanInit;           /* needed only on devices side */
    SIM_OS_SLAN_START_FUN             simOsSlanStart;          /* needed only on devices side */
    SIM_OS_CHANGE_LINK_STATUS_FUN     simOsChangeLinkStatus;   /* needed only on devices side */

}SIM_OS_FUNC_BIND_SLAN_STC;

#ifndef APPLICATION_SIDE_ONLY
extern     SIM_OS_SLAN_BIND_FUN              SIM_OS_MAC(simOsSlanBind);
extern     SIM_OS_SLAN_TRANSMIT_FUN          SIM_OS_MAC(simOsSlanTransmit);
extern     SIM_OS_SLAN_UNBIND_FUN            SIM_OS_MAC(simOsSlanUnbind);
extern     SIM_OS_SLAN_INIT_FUN              SIM_OS_MAC(simOsSlanInit);
extern     SIM_OS_SLAN_START_FUN             SIM_OS_MAC(simOsSlanStart);
extern     SIM_OS_CHANGE_LINK_STATUS_FUN     SIM_OS_MAC(simOsChangeLinkStatus);
#endif /*!APPLICATION_SIDE_ONLY*/

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif   /* __simOsBindSlanh */


