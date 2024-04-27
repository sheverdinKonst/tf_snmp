/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetCht2TTermination.h
*
* DESCRIPTION:
*       Cheetah2 Asic Simulation .
*       Tunnel Termination Engine processing for incoming frame.
*       header file.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 6 $
*
*******************************************************************************/
#ifndef __snetCht2TTermination
#define __snetCht2TTermination

#include <asicSimulation/SKernel/suserframes/snetCheetahPclSrv.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct INTERNAL_TTI_DESC_INFO_STCT* INTERNAL_TTI_DESC_INFO_STCT_PTR;


/* no match indicator during policy key matching checking */
#define SNET_CHT2_TUNNEL_NO_MATCH_INDEX_CNS                  (0xffffffff)

#define SNET_CHT2_TUNNEL_TCAM_1024_CNS                             (1024)

/*
    enum :  CHT2_TT_KEY_FIELDS_ID_ENT

    description : enum of the egress PCL TCAM fields

*/
typedef enum{
    CHT2_TT_SOURCE_PORT_TRUNK_E,
    CHT2_TT_SRC_IS_TRUNK_E,
    CHT2_TT_SRC_DEV_E,
    CHT2_TT_KEY_FIELDS_ID_RESERVED_13_E,
    CHT2_TT_FIELDS_ID_VID_E,
    CHT2_TT_MAC_DA_ID_UP_E,
    CHT2_TT_TUNNEL_KEY_TYPE_E,
    CHT2_TT_IPV4_OVER_IPV4_E,
    CHT2_TT_IPV6_OVER_IPV4_E,
    CHT2_TT_IPV4_OVER_IPV4_GRE_E,
    CHT2_TT_IPV6_OVER_IPV4_GRE_E,
    CHT2_TT_KEY_FIELDS_ID_RESERVED_79_E,
    CHT2_TT_SIP_ADDRESS,
    CHT2_TT_KEY_FIELDS_ID_RESERVED_112_126_E,
    CHT2_TT_KEY_TUNNEL_TERMINATION_KEY,
    CHT2_TT_DIP_ADDRESS,
    CHT2_TT_KEY_FIELDS_ID_RESERVED_160_191_E,
    CHT2_TT_KEY_FIELDS_ID_RESERVED_75_E,
    CHT2_TT_EXP2_IN_LABEL2_E,
    CHT2_TT_SBIT_IN_LABEL2_E,
    CHT2_TT_LABEL2_E,
    CHT2_TT_EXP1_IN_LABEL1_E,
    CHT2_TT_SBIT_IN_LABEL1_E,
    CHT2_TT_LABEL1_E,
    CHT2_TT_KEY_FIELDS_ID_RESERVED_124_126_E,
    CHT2_TT_KEY_FIELDS_ID_RESERVED_128_191_E,
    CHT2_TT_KEY_FIELDS_ID_LAST_E
}CHT2_TT_KEY_FIELDS_ID_ENT;




/*
    enum :  SKERNEL_CH2_TT_PACKET_CMD_ENT

    description : enum of the TT forward command
    D.12.8.3 - T.T Action Entry. page 794

*/
typedef enum{
    SKERNEL_CHT2_TT_CMD_FORWARD_E =   0,
    SKERNEL_CHT2_TT_CMD_MIRROR_E    = 1,
    SKERNEL_CHT2_TT_CMD_TRAP_E      = 2,
    SKERNEL_CHT2_TT_CMD_HARD_DROP_E = 3,
    SKERNEL_CHT2_TT_CMD_SOFT_DROP_E = 4,
}SKERNEL_CH2_TT_PACKET_CMD_ENT;

/*
    struct : SNET_CHT2_TT_ACTION_STC

    description : The tunnel action table is accessed ingress TT and
                  The line index of the matching rule is used
                  to index the TT action table and extract the action
                  to perform.

    matchCounterIndex - A pointer to one of the 32 policy rules match counters.
    matchCounterEn    - enables the binding of this policy action entry to
                        the Policy Rule Match Counters<n>
    ttModifyUp        - enables the modification of the 802.1p User Priority field
                        to <epceUp> of packet transmitted tagged.
    ttModifyDscp      - Enables the modification of the IP Header dscp field to
                        <epceDscp> of packet transmitted tagged.
    ttUp              - The UP value.
    ttDscp            - The DSCP value.
    fwdCmd            - 0 - Forward , 1-2 Reserved , 3 - Hard Drop , 4-7 Reserved
    ttVid             - The VID to be set to transmitted
    ttVlanCmd         - Enable modification of transmitted VLAN Tag to PCE_VID
*/
typedef struct _snet_cht2_tt_action_stc{
    GT_U32          ttCpuCode;
    GT_U32          ttPassengerType;
    GT_U32          ttRedirectCmd;
    GT_U32          ttQoSMode;
    GT_U32          ttQoSPrecedence;
    GT_U32          ttQoSProfile;
    GT_U32          ttModifyUp;
    GT_U32          ttModifyDscp;
    GT_U32          ttRemapDscp;
    GT_U32          ttUp;
    GT_U32          ttPassType;
    GT_U32          ttDscp;
    GT_U32          ttMirrorAnlze;
    SKERNEL_CH2_TT_PACKET_CMD_ENT    fwdCmd;
    GT_U32          ttVid;
    GT_U32          ttVidPrecedence;
    GT_U32          ttNestedVidEn;
    GT_U32          ttTtl;
    GT_U32          matchCounterIndex;
    GT_BOOL         matchCounterEn;
    GT_U32          tunnelPtr; /* The pointer to the tunnel start entry used to tunnel the packet */
    GT_U32          tunnelStart; /* indicates this action is a tunnel start point */
    GT_U32          policerIndex;
    GT_BOOL         policerEn;
    SNET_DST_INTERFACE_STC      tunnelIf; /* The egress interface to which the packet redirected to... */
}SNET_CHT2_TT_ACTION_STC;

/*******************************************************************************
* snetCht2TTermination
*
* DESCRIPTION:
*        TT Engine processing for outgoing frame on Cheetah2
*        asic simulation.
*        TT processing , tt assignment ,key forming , 1 Lookup ,
*        actions to descriptor processing
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - pointer to frame data buffer Id
* OUTPUTS:
*       descrPtr     - pointer to updated frame data buffer Id
*
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
GT_VOID snetCht2TTermination
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
);

/*******************************************************************************
* snetCht2TTIPv4ExceptionCheck
*
* DESCRIPTION:
*        IPv4 tunnels and MPLS tunnels have separate T.T trigger
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - pointer to frame data buffer Id
*
* OUTPUTS:
*
*
* RETURN:
*       GT_BOOL TRUE DOING T.T false
*
* COMMENTS:
*         13.4.3 page 358
*
*******************************************************************************/
GT_BOOL snetCht2TTIPv4ExceptionCheck
(
    IN SKERNEL_DEVICE_OBJECT             *  devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC   *  descrPtr,
    IN INTERNAL_TTI_DESC_INFO_STCT_PTR      internalTtiInfoPtr

);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __snetCht2TTermination */


