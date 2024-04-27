/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetCheetah2Pcl.h
*
* DESCRIPTION:
*       Cheetah2 Asic Simulation .
*       Ingress Policy Engine processing for incoming frame.
*       header file.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 8 $
*
*******************************************************************************/
#ifndef __snetCheetah2Pclh
#define __snetCheetah2Pclh

#include <asicSimulation/SKernel/suserframes/snetCheetahPclSrv.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* number of compares to be done on Layer 4 ports */
#define SNET_CHT2_L4_TCP_PORT_COMP_RANGE_INDEX_CNS      (0x8)

/* number of rules in extended mode */
#define SNET_CHT2_TCAM_1024_EXTENDED_RULES_CNS                  (0x200)

/* number of rules in standard mode */
#define SNET_CHT2_TCAM_512_STANDARD_RULES_CNS                   (0x400)

/* the range of address between one word to another */
#define CHT_PCL2_TCAM_WORDS_WIDTH_CNS                           (0x4000 / 4)

/* the range of address between one word to another */
#define CHT_PCL2_TCAM_ENTRY_WIDTH_CNS                           (0x10 / 4)

#define CHT2_PCL_TCAM_OFFSET_TO_SECOND_HALF_TABLE_IN_WORD_CNS   (0x10 * 512) / 0x4


/**
    structure : SNET_CHT2_POLICY_SPECIFIC_LOOKUP_CONFIG_STC
    (Table 266: Ingress PCL configuration Table<n> (0<=n<4224) --- page 822)

    Description : The PCL table is a 4224 line table each entry is-32 bit.
                  The PCL Lookup is PCL-ID based , in addition , the search key

                  standard key (24 Bytes/192 bits).
                  extended key (48 Bytes/384 bits).

                  enabled - enables the access to the policy TCAM table.
                  nonIpKeyType - key type for non IP packets :
                        0 - non-ip standard key.
                        1 - non-ip extended key.
                  ipv4ArpKeyType - key type for IPv4 and ARP packets :
                        0 - IP+L2 standard key.
                        1 - IP+L3 standard key.
                        2 - non-IP+IPv4 extended key.
                  ipv6ArpKeyType - key type for IPv6 packets :
                        0 - IP+L2 standard key.
                        1 - IPv6 DIP standard key.
                        2 - L2+IPv6 extended key.
                        3 - IP+L4 extended key.
                  pclID - The PCL-ID for this lookup.
**/

typedef struct {
    GT_BIT                      enabled;
    GT_BIT                      nonIpKeyType;
    GT_U32                      ipv4ArpKeyType;
    GT_U32                      ipv6KeyType;
    GT_U32                      pclID;
    SNET_CHT_PCL_KEY_SIZE       keySize;
}SNET_CHT2_POLICY_SPECIFIC_LOOKUP_CONFIG_STC;

/**
    structure : SNET_CHT2_IPOLICY_LOOKUP_CONFIG_STC
    (Table 266: Ingress PCL configuration Table<n> (0<=n<4224) --- page 822)

    lookup0Config - first lookup cycle configuration (see details above).
    lookup1Config - second lookup cycle configuration (see details above).

**/
typedef struct{
    SNET_CHT2_POLICY_SPECIFIC_LOOKUP_CONFIG_STC lookup0Config;
    SNET_CHT2_POLICY_SPECIFIC_LOOKUP_CONFIG_STC lookup1Config;
}SNET_CHT2_IPOLICY_LOOKUP_CONFIG_STC;


/*
    enum :  CHT2_PCL_KEY_TYPE_ENT

    description : enum of the ingress PCL TCAM fields

*/
typedef enum{
      CHT2_PCL_KEY_TYPE_L2_KEY_SHORT_E,     /* L2  egress , standard non-ip*/
      CHT2_PCL_KEY_TYPE_L2_L3_KEY_SHORT_E,  /* L2+IPv4+QOS , standard L2+Ipv4\6*/
      CHT2_PCL_KEY_TYPE_L3_L4_KEY_SHORT_E , /* IPv4 + L4 , standard ipv4+L4*/
      CHT2_PCL_KEY_TYPE_L2_IPV6_KEY_SHORT_E,/* Standard IPv6 DIP key */
      CHT2_PCL_KEY_TYPE_L2_L3_L4_KEY_LONG_E,/* L2+IPv4+L4 , extended , non-ip+IPv4*/
      CHT2_PCL_KEY_TYPE_L2_IPV6_KEY_LONG_E ,/* L2+IPv6 , Extended L2+Iipv6*/
      CHT2_PCL_KEY_TYPE_L4_IPV6_KEY_LONG_E ,/* L4+IPv6 , extended L4+ipv6*/
      CHT2_PCL_KEY_TYPE_LAST_E
}CHT2_PCL_KEY_TYPE_ENT;


/*
    enum :  CHT2_PCL_KEY_FIELDS_ID_ENT

    description : enum of the ingress PCL TCAM fields

*/
typedef enum{
    CHT2_PCL_KEY_FIELDS_ID_VALID_E,
    CHT2_PCL_KEY_FIELDS_ID_PCL_ID_E,
    CHT2_PCL_KEY_FIELDS_ID_SRC_PORT_E,
    CHT2_PCL_KEY_FIELDS_ID_IS_TAGGED_E,
    CHT2_PCL_KEY_FIELDS_ID_VID_E,
    CHT2_PCL_KEY_FIELDS_ID_UP_E,
    CHT2_PCL_KEY_FIELDS_ID_QOS_PROFILE_E,
    CHT2_PCL_KEY_FIELDS_ID_IS_IPV4_E,
    CHT2_PCL_KEY_FIELDS_ID_IS_IP_E,
    CHT2_PCL_KEY_FIELDS_ID_RESERVED_42_E,
    CHT2_PCL_KEY_FIELDS_ID_ETHER_TYPE_DSAP_SSAP_E,
    CHT2_PCL_KEY_FIELDS_ID_USER_DEFINED_BYTE2_66_59_SHORT_E,
    CHT2_PCL_KEY_FIELDS_ID_RESERVED_72_67_E,
    CHT2_PCL_KEY_FIELDS_ID_IS_ARP_E,
    CHT2_PCL_KEY_FIELDS_ID_RESERVED_74_E,
    CHT2_PCL_KEY_FIELDS_ID_USER_DEFINED_BYTE0_82_75_SHORT_E,
    CHT2_PCL_KEY_FIELDS_ID_USER_DEFINED_BYTE1_90_83_SHORT_E,
    CHT2_PCL_KEY_FIELDS_ID_ENCAP_TYPE_SHORT_E,
    CHT2_PCL_KEY_FIELDS_ID_MAC_SA_SHORT_E,
    CHT2_PCL_KEY_FIELDS_ID_MAC_DA_SHORT_E,
    CHT2_PCL_KEY_FIELDS_ID_IS_L2_VALID_SHORT_E,
    CHT2_PCL_KEY_FIELDS_ID_USER_DEFINED_VALID_SHORT_E,
    CHT2_PCL_KEY_FIELDS_ID_RESERVED_190_191_E,
    CHT2_PCL_KEY_FIELDS_ID_IP_PROTOCOL_E,
    CHT2_PCL_KEY_FIELDS_ID_DSCP_E,
    CHT2_PCL_KEY_FIELDS_ID_IS_L4_VALID_E,
    CHT2_PCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_2_3_E,
    CHT2_PCL_KEY_FIELDS_ID_IS_IPV6_EH_EXISTS_SHORT_E,
    CHT2_PCL_KEY_FIELDS_ID_TCP_UDP_PORT_COMPS_SHORT_E,
    CHT2_PCL_KEY_FIELDS_ID_LONG_IS_IPV6_EH_HOP_BY_HOP_E,
    CHT2_PCL_KEY_FIELDS_ID_IPV4_FRAGMENTED_SHORT_E,
    CHT2_PCL_KEY_FIELDS_ID_IS_BC_E,
    CHT2_PCL_KEY_FIELDS_ID_USER_DEFINED_BYTE2_98_91_SHORT_E,
    CHT2_PCL_KEY_FIELDS_ID_SIP_31_0_E,
    CHT2_PCL_KEY_FIELDS_ID_DIP_31_0_E,
    CHT2_PCL_KEY_FIELDS_ID_L4_BYTES_OFFSETS_0_1_13_SHORT_E,
    CHT2_PCL_KEY_FIELDS_ID_RESERVED_187_E,
    CHT2_PCL_KEY_FIELDS_ID_IP_HEADER_OK_SHORT_E,
    CHT2_PCL_KEY_FIELDS_ID_IS_IPV6_E,
    CHT2_PCL_KEY_FIELDS_ID_L4_BYTES_OFFSETS_0_1_13_LONG_E,
    CHT2_PCL_KEY_FIELDS_ID_SIP_31_0_LONG_E,
    CHT2_PCL_KEY_FIELDS_ID_DIP_31_0_LONG_E,
    CHT2_PCL_KEY_FIELDS_ID_ENCAP_TYPE_LONG_VALID_E,
    CHT2_PCL_KEY_FIELDS_ID_ETHER_TYPE_DSAP_SSAP_LONG_E,
    CHT2_PCL_KEY_FIELDS_ID_LONG_IPV4_FRAGMENT_E,
    CHT2_PCL_KEY_FIELDS_ID_LONG_RESERVED_179_234_E,
    CHT2_PCL_KEY_FIELDS_ID_MAC_SA_LONG_E,
    CHT2_PCL_KEY_FIELDS_ID_MAC_DA_LONG_E,
    CHT2_PCL_KEY_FIELDS_ID_USER_DEFINED_BYTE3_338_331_LONG_E,
    CHT2_PCL_KEY_FIELDS_ID_USER_DEFINED_BYTE4_346_339_LONG_E,
    CHT2_PCL_KEY_FIELDS_ID_USER_DEFINED_BYTE5_354_347_LONG_E,
    CHT2_PCL_KEY_FIELDS_ID_USER_DEFINED_BYTE0_362_355_LONG_E,
    CHT2_PCL_KEY_FIELDS_ID_USER_DEFINED_BYTE1_370_363_LONG_E,
    CHT2_PCL_KEY_FIELDS_ID_USER_DEFINED_BYTE2_378_371_LONG_E,
    CHT2_PCL_KEY_FIELDS_ID_RESERVED_380_379_LONG_E,
    CHT2_PCL_KEY_FIELDS_ID_IS_L2_VALID_LONG_E,
    CHT2_PCL_KEY_FIELDS_ID_USER_DEFINED_VALID_LONG_E,
    CHT2_PCL_KEY_FIELDS_ID_SIP_127_32_LONG_E,
    CHT2_PCL_KEY_FIELDS_ID_IS_IPV6_EH_EXISTS_LONG_E,
    CHT2_PCL_KEY_FIELDS_ID_IS_IPV6_EH_HOP_BY_HOP_LONG_E,
    CHT2_PCL_KEY_FIELDS_ID_DIP_127_120_LONG_E,
    CHT2_PCL_KEY_FIELDS_ID_DIP_119_32_LONG_E,
    CHT2_PCL_KEY_FIELDS_ID_DIP_0_31_IPV6_LONG_E,
    CHT2_PCL_KEY_FIELDS_ID_DIP_0_15_IPV6_SHORT_E,
    CHT2_PCL_KEY_FIELDS_ID_DIP_16_31_IPV6_SHORT_E,
    CHT2_PCL_KEY_FIELDS_ID_DIP_127_32_SHORT_E,
    CHT2_PCL_KEY_FIELDS_ID_RESERVED_188_E,
    CHT2_PCL_KEY_FIELDS_ID_RESERVED_190_E,
    CHT2_PCL_KEY_FIELDS_ID_IP_HEADER_OK_LONG_E,
    CHT2_PCL_KEY_FIELDS_ID_TCP_UDP_PORT_COMPS_LONG_E,
    CHT2_PCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_4_5_E,
    CHT2_PCL_KEY_FIELDS_ID_LAST_E
}CHT2_PCL_KEY_FIELDS_ID_ENT;


/*  PCL
    actions for the VPLS support.
*/
typedef struct{
    GT_BIT          unknownSaCmdAssigned; /* flag internal for the simulation to remove the use of value '7' from the unknownSaCmd */

    GT_U32          unknownSaCmd; /*3 bits
                    Used by Bridge
                    0 = Forward
                    1 = MirrorToCPU
                    2 = TrapToCPU
                    3 = HardDrop
                    4 = SoftDrop
                    5,6 = Not assigned by TTI or PCL. Using Bridge port configuration register
                    7 = Bypass assignment. Preserve former assignment by TTI/PCL
    */
    GT_U32          srcMeshIdEn;/*1 bit
                        0 = Preserve former assignment of SrcMeshID by TTI
                        1= Overwrite TTI assignment of SrcMeshID
                                */
    GT_U32          srcMeshId; /*2 bits*/
    GT_U32          userTaggedAcEnable;/*1 bit (was previously called <Service VLAN Keep Mode>)
                    When set, the AC packet outer tag is user tagged and is copied into DescOut<Tag1> to maintain the
                    user tag in the egress packet, and is copied into DescOut<OrigVID> for egress VLAN filtering.
                    Note: this configuration is applied BEFORE applying the IPCL AE<VID0 Command>.
                        */
}SNET_XCAT_PCL_ACTION_VPLS_SUPPORT_STC;

/*
    struct : SNET_CHT2_PCL_ACTION_STC

    description : The policy engine maintains an 1024 entries table ,
                corresponding to the 1024 rules that may be defined in the
                TCAM lookup structure . The line index of the matching rule
                is used to index the policy action table and extract the
                action to perform.
*/
typedef struct _cht2_pcl_action_stc{
    GT_U32      matchCounterIndex;  /* pointer to one of the 32 pcl match counters */
    GT_BOOL     matchCounterEn; /* enables the match counter update */
    GT_U32      pceModifyUp; /* enables the modification of the packets UP */
    GT_U32      pceModifyDscp; /* enables the modification of the packets DSCP */
    GT_U32      pceQosProfile; /* index to the QoS Table entry */
    GT_BOOL     pceQosProfileMarkingEn; /* enables the profile remarking */
    SKERNEL_PRECEDENCE_ORDER_ENT pceQosPrecedence; /* PCE marking of the QoS precedence */
    GT_U32      mirrorToIngressAnalyzerPort; /* enables the mirroring of the packet to the ingress analyser port */
    GT_U32      pceCpuCode; /* cpu code*/
    SKERNEL_EXT_PACKET_CMD_ENT fwdCmd; /* forwarding command */
    GT_U32      pceVid; /* The VLAN-ID set byu this entry */
    GT_U32      pceVlanCmd; /* enables the setting of the VLAN command */
    GT_BOOL     enNestedVlan; /* enables nested VLAN */
    GT_U32      pceVidPrecedence; /* The VLAN-ID precedence for the subsequent VLAN assignment mechanism */
    SNET_DST_INTERFACE_STC      pceEgressIf; /* The egress interface to which the packet redirected to... */
    GT_U32      pceRedirectCmd; /* enables the redirection of the matching flow packets */
    GT_U32      tunnelType;  /* The type of the tunneled packet */
    GT_U32      tunnelPtr; /* The pointer to the tunnel start entry used to tunnel the packet */
    GT_U32      tunnelStart; /* indicates this action is a tunnel start point */
    GT_U32      VNTL2Echo; /* enables VNT L2 echo */
    GT_U32      policerIndex; /* policer index */
    GT_U32      policerEn; /* when set to 1 , this rule is bound to the policer indexed by policerIndex */
}SNET_CHT2_PCL_ACTION_STC;

/*
    Struct : CHT2_PCL_USER_DEFINED_CONFIG_REG_STC

    description : hold info about the user defined fields in the registers
    startBitOffset - starting bit of the Offset field (length 7 bits)
    startBitAnchor - starting bit of the Anchor field (length 2 bits)
*/
typedef struct{
    GT_U32  startBitOffset;
    GT_U32  startBitAnchor;
}CHT2_PCL_USER_DEFINED_CONFIG_REG_STC;

/*******************************************************************************
* snetCht2IPcl
*
* DESCRIPTION:
*        Ingress Policy Engine processing for incoming frame  on Cheetah2
*        asic simulation.
*        PCL processing , Pcl assignment ,key forming , 2 Lookups ,
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
extern GT_VOID snetCht2IPcl
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
);

/*******************************************************************************
* snetCht2PCLTcamLookUp
*
* DESCRIPTION:
*        Tcam lookup .
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - pointer to frame descriptor.
*       pclKeyPtr       - pointer to PCL key .
*
* OUTPUTS:
*       matchIndexPtr   - pointer to the matching index.
*
* RETURN:
*
* COMMENTS:
*       C.12.13     Policy Tcam Table : The policy TCAM holds 1024 lines
*                 of 24 bytes or rule data used for ingress and egress PCL.
*******************************************************************************/
extern GT_VOID snetCht2SrvPclTcamLookUp
(
    IN SKERNEL_DEVICE_OBJECT            *devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  *descrPtr  ,
    IN SNET_CHT_POLICY_KEY_STC          *pclKeyPtr,
    OUT GT_U32                          *matchIndexPtr
);

/*******************************************************************************
* snetCht2IPclActionGet
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
*
*******************************************************************************/
extern GT_VOID snetCht2IPclActionGet
(
    IN SKERNEL_DEVICE_OBJECT            *   devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  *   descrPtr  ,
    IN GT_U32                               matchIndex,
    OUT SNET_CHT2_PCL_ACTION_STC        *   actionDataPtr
);

/*******************************************************************************
* snetCht2IPclActionApply
*
* DESCRIPTION:
*        Apply the action entry from the policy action table.
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - pointer to frame descriptor.
*       actionInfoPtr   - pointer to the PCL action entry.
*       isFirst         - is first action apply
*
* OUTPUTS:
*
* RETURN:
*
* COMMENTS:
*       C.12.13     Policy Tcam Table : The policy TCAM holds 1024 lines
*                 of 24 bytes or rule data used for ingress and egress PCL.
*******************************************************************************/
extern GT_VOID snetCht2IPclActionApply
(
    IN SKERNEL_DEVICE_OBJECT                * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC   * descrPtr  ,
    IN SNET_CHT2_PCL_ACTION_STC             * actionDataPtr,
    IN GT_BOOL                              isFirst
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __snetCheetah2Pclh */


