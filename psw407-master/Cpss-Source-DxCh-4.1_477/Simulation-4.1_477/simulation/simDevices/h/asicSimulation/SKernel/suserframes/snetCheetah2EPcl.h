/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetCheetah2EPcl.h
*
* DESCRIPTION:
*       Cheetah2 Asic Simulation .
*       Egress Policy Engine processing for outgoing frame.
*       header file.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 6 $
*
*******************************************************************************/
#ifndef __snetCheetah2EPclh
#define __snetCheetah2EPclh

#include <asicSimulation/SKernel/suserframes/snetCheetahPclSrv.h>
#include <asicSimulation/SKernel/suserframes/snetCheetah2Pcl.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
    structure : SNET_CHT2_EPCL_LOOKUP_CONFIG_STC
    (Table 270: Egress PCL configuration Table<n> (0<=n<4160) --- page 826)

    lookupConfig - epcl lookup configuration (see details above).

**/
typedef struct{
    SNET_CHT2_POLICY_SPECIFIC_LOOKUP_CONFIG_STC         lookupConfig;
}SNET_CHT2_EPCL_LOOKUP_CONFIG_STC;

/*
    enum :  CHT2_EPCL_KEY_TYPE_ENT

    description : enum of the egress PCL TCAM KEY fields

*/
typedef enum{
      CHT2_EPCL_KEY_TYPE_L2_KEY_SHORT_E,     /* L2  egress , standard non-ip*/
      CHT2_EPCL_KEY_TYPE_L2_L3_KEY_SHORT_E,  /* L2+IPv4+QOS , standard L2+Ipv4\6*/
      CHT2_EPCL_KEY_TYPE_L3_L4_KEY_SHORT_E , /* IPv4 + L4 , standard ipv4+L4*/
      CHT2_EPCL_KEY_TYPE_L2_L3_L4_KEY_LONG_E,/* L2+IPv4+L4 , extended , non-ip+IPv4*/
      CHT2_EPCL_KEY_TYPE_L2_IPV6_KEY_LONG_E ,/* L2+IPv6 , Extended L2+Iipv6*/
      CHT2_EPCL_KEY_TYPE_L4_IPV6_KEY_LONG_E ,/* L4+IPv6 , extended L4+ipv6*/
      CHT2_EPCL_KEY_TYPE_UNKNOWN_E
}CHT2_EPCL_KEY_TYPE_ENT;


/*
    enum :  CHT2_EPCL_KEY_FIELDS_ID_ENT

    description : enum of the egress PCL TCAM fields

*/
typedef enum{
    CHT2_EPCL_KEY_FIELDS_ID_VALID_E,
    CHT2_EPCL_KEY_FIELDS_ID_PCL_ID_E,
    CHT2_EPCL_KEY_FIELDS_ID_SRC_PORT_E,
    CHT2_EPCL_KEY_FIELDS_ID_IS_TAGGED_E,
    CHT2_EPCL_KEY_FIELDS_ID_VID_E,
    CHT2_EPCL_KEY_FIELDS_ID_UP_E,
    CHT2_EPCL_KEY_FIELDS_ID_QOS_PROFILE_E,
    CHT2_EPCL_KEY_FIELDS_ID_IS_IPV4_E,
    CHT2_EPCL_KEY_FIELDS_ID_IS_IP_E,
    CHT2_EPCL_KEY_FIELDS_ID_RESERVED_42_E,
    CHT2_EPCL_KEY_FIELDS_ID_ETHER_TYPE_DSAP_SSAP_E,
    CHT2_EPCL_KEY_FIELDS_ID_RESERVED_66_59_SHORT_E,
    CHT2_EPCL_KEY_FIELDS_ID_RESERVED_72_67_E,
    CHT2_EPCL_KEY_FIELDS_ID_IS_ARP_E,
    CHT2_EPCL_KEY_FIELDS_ID_RESERVED_74_E,
    CHT2_EPCL_KEY_FIELDS_ID_SRC_IS_TRUNK_SHORT_E,
    CHT2_EPCL_KEY_FIELDS_ID_IS_ROUTED,
    CHT2_EPCL_KEY_FIELDS_ID_IS_L2_VALID_SHORT_E,
    CHT2_EPCL_KEY_FIELDS_ID_ENCAP_TYPE_SHORT_E,
    CHT2_EPCL_KEY_FIELDS_ID_MAC_SA_SHORT_E,
    CHT2_EPCL_KEY_FIELDS_ID_MAC_DA_SHORT_E,
    CHT2_EPCL_KEY_FIELDS_ID_PACKET_TYPE_SHORT_E,
    CHT2_EPCL_KEY_FIELDS_ID_RESERVED_190_E,
    CHT2_EPCL_KEY_FIELDS_ID_IS_VIDX_SHORT_E,
    CHT2_EPCL_KEY_FIELDS_ID_IP_PROTOCOL_E,
    CHT2_EPCL_KEY_FIELDS_ID_DSCP_E,
    CHT2_EPCL_KEY_FIELDS_ID_IS_L4_VALID_E,
    CHT2_EPCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_2_3_E,
    CHT2_EPCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_4_5_E,
    CHT2_EPCL_KEY_FIELDS_ID_TCP_UDP_PORT_COMPS_SHORT_E,
    CHT2_EPCL_KEY_FIELDS_ID_DIP_31_0_E,
    CHT2_EPCL_KEY_FIELDS_ID_L4_BYTES_OFFSETS_13_SHORT_E,
    CHT2_EPCL_KEY_FIELDS_ID_RESERVED_139_SHORT_E,
    CHT2_EPCL_KEY_FIELDS_ID_IPV4_FRAGMENTED_SHORT_E,
    CHT2_EPCL_KEY_FIELDS_ID_IS_BC_E,
    CHT2_EPCL_KEY_FIELDS_ID_SIP_31_0_SHORT_E,
    CHT2_EPCL_KEY_FIELDS_ID_DIP_31_0_IPV4_L4_SHORT_E,
    CHT2_EPCL_KEY_FIELDS_ID_L4_BYTES_OFFSETS_0_1_13_SHORT_E,
    CHT2_EPCL_KEY_FIELDS_ID_RESERVED_187_E,
    CHT2_EPCL_KEY_FIELDS_ID_IS_IPV6_E,
    CHT2_EPCL_KEY_FIELDS_ID_L4_BYTES_OFFSETS_0_1_13_LONG_E,
    CHT2_EPCL_KEY_FIELDS_ID_SIP_31_0_LONG_E,
    CHT2_EPCL_KEY_FIELDS_ID_DIP_31_0_LONG_E,
    CHT2_EPCL_KEY_FIELDS_ID_ENCAP_TYPE_LONG_VALID_E,
    CHT2_EPCL_KEY_FIELDS_ID_ETHER_TYPE_DSAP_SSAP_LONG_E,
    CHT2_EPCL_KEY_FIELDS_ID_LONG_RESERVED_179_234_E,
    CHT2_EPCL_KEY_FIELDS_ID_USER_DEFINED_BYTE3_338_331_LONG_E,
    CHT2_EPCL_KEY_FIELDS_ID_USER_DEFINED_BYTE4_346_339_LONG_E,
    CHT2_EPCL_KEY_FIELDS_ID_USER_DEFINED_BYTE5_354_347_LONG_E,
    CHT2_EPCL_KEY_FIELDS_ID_USER_DEFINED_BYTE0_362_355_LONG_E,
    CHT2_EPCL_KEY_FIELDS_ID_USER_DEFINED_BYTE1_370_363_LONG_E,
    CHT2_EPCL_KEY_FIELDS_ID_USER_DEFINED_BYTE2_378_371_LONG_E,
    CHT2_EPCL_KEY_FIELDS_ID_TCP_UDP_PORT_COMPS_LONG_E,
    CHT2_EPCL_KEY_FIELDS_ID_SIP_127_32_LONG_E,
    CHT2_EPCL_KEY_FIELDS_ID_IS_IPV6_EH_HOP_BY_HOP_LONG_E,
    CHT2_EPCL_KEY_FIELDS_ID_DIP_127_120_LONG_E,
    CHT2_EPCL_KEY_FIELDS_ID_MAC_SA_LONG_E,
    CHT2_EPCL_KEY_FIELDS_ID_MAC_DA_LONG_E,
    CHT2_EPCL_KEY_FIELDS_ID_RESERVED_380_379_LONG_E,
    CHT2_EPCL_KEY_FIELDS_ID_IS_IPV6_EH_EXISTS_LONG_E,
    CHT2_EPCL_KEY_FIELDS_ID_DIP_119_32_LONG_E,
    CHT2_EPCL_KEY_FIELDS_ID_DIP_0_31_IPV6_LONG_E,
    CHT2_EPCL_KEY_FIELDS_ID_IS_L2_VALID_LONG_E,
    CHT2_EPCL_KEY_FIELDS_ID_USER_DEFINED_VALID_LONG_E,
    CHT2_EPCL_KEY_FIELDS_ID_IP_HEADER_OK_LONG_E,
    CHT2_EPCL_KEY_FIELDS_ID_CPU_CODE_0_6_STANDARD_E,
    CHT2_EPCL_KEY_FIELDS_ID_RESERVED_39_33_E,
    CHT2_EPCL_KEY_FIELDS_ID_TRUNK_ID,
    CHT2_EPCL_KEY_FIELDS_ID_SRCDEV_ID,
    CHT2_EPCL_KEY_FIELDS_ID_SST_ID_E,
    CHT2_EPCL_KEY_FIELDS_ID_IS_UNKNOWN_UC_E,
    CHT2_EPCL_KEY_FIELDS_ID_CPU_CODE_7_STANDARD_E,
    CHT2_EPCL_KEY_FIELDS_ID_LONG_IPV4_FRAGMENT_E,
    CHT2_EPCL_KEY_FIELDS_ID_RESERVED_89_E,
    CHT2_EPCL_KEY_FIELDS_ID_LAST_E
}CHT2_EPCL_KEY_FIELDS_ID_ENT;

/*
    enum :  SKERNEL_CH2_EPCL_PACKET_CMD_ENT

    description : enum of the egress forward command
    C.12.12 - Policy Action Entry. page 794

*/
typedef enum{
    SKERNEL_CHT2_EPCL_CMD_FORWARD_E = 0,
    SKERNEL_CHT2_EPCL_CMD_HARD_DROP = 3
}SKERNEL_CH2_EPCL_PACKET_CMD_ENT;


/*
    struct : SNET_CHT2_EPCL_ACTION_STC

    description : The policy action table is accessed of both ingress and
                  egress PCL. The line index of the matching rule is used
                  to index the policy action table and extract the action
                  to perform.

    matchCounterIndex - A pointer to one of the 32 policy rules match counters.
    matchCounterEn    - enables the binding of this policy action entry to
                        the Policy Rule Match Counters<n>
    pceModifyUp       - enables the modification of the 802.1p User Priority field
                        to <epceUp> of packet transmitted tagged.
    pceModifyDscp     - Enables the modification of the IP Header dscp field to
                        <epceDscp> of packet transmitted tagged.
    epceUp            - The UP value.
    epceDscp          - The DSCP value.
    fwdCmd            - 0 - Forward , 1-2 Reserved , 3 - Hard Drop , 4-7 Reserved
    pceVid            - The VID to be set to transmitted
    pceVlanCmd        - Enable modification of transmitted VLAN Tag to PCE_VID

    -- fields relevant from lion2 ---

    vid1              - vlan to set into tag 1 according to  vlan1Cmd
    up1               - UP to set into tag 1 according to  modifyUp1
    vlan1Cmd          - tag 1 vlan command
    modifyUp1         - Enable Modify UP1
*/
typedef struct {
    GT_U32          matchCounterIndex;
    GT_BOOL         matchCounterEn;
    GT_U32          pceModifyUp;
    GT_U32          pceModifyDscp;
    GT_U32          epceUp;
    GT_U32          epceDscp;
    SKERNEL_CH2_EPCL_PACKET_CMD_ENT    fwdCmd;
    GT_U32          epceVid;
    GT_U32          epceVlanCmd;

    GT_U32          vid1;
    GT_U32          up1;
    GT_U32          vlan1Cmd;
    GT_U32          modifyUp1;


}SNET_CHT2_EPCL_ACTION_STC;

/*******************************************************************************
* snetChtEPcl
*
* DESCRIPTION:
*        Egress Policy Engine processing for outgoing frame on Cheetah2
*        asic simulation.
*        ePCL processing , ePcl assignment ,key forming , 1 Lookup ,
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
extern GT_VOID snetCht2EPcl
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN  GT_U32  port
);

/*******************************************************************************
* snetCht2EPclActionGet
*
* DESCRIPTION:
*        Get the action entry from the policy action table.
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - pointer to frame descriptor.
*       matchIndex      - index to the action table .
*
* OUTPUTS:
*       actionInfoPtr   - pointer to the PCL action.
*
* RETURN:
*
* COMMENTS:
*       The policy Engine maintains an 1024 entries table, corresponding
*       to the 1024 rules that may be defined in the TCAM lookup
*       structure.
*
*******************************************************************************/
extern GT_VOID snetCht2EPclActionGet
(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr  ,
    IN GT_U32                             matchIndex,
    OUT SNET_CHT2_EPCL_ACTION_STC       * actionDataPtr
);

/*******************************************************************************
* snetCht2EPclActionApply
*
* DESCRIPTION:
*        Apply the action entry from the policy action table.
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - pointer to frame descriptor.
*       actionInfoPtr   - pointer to the ePCL action entry.
*
* OUTPUTS:
*
* RETURN:
*
* COMMENTS:
*       Egress PCL action table and Policy TCAM access control registers
*
*******************************************************************************/
extern GT_VOID snetCht2EPclActionApply
(
    IN SKERNEL_DEVICE_OBJECT                * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC   * descrPtr  ,
    IN SNET_CHT2_EPCL_ACTION_STC            * actionDataPtr
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __snetCheetah2EPclh */


