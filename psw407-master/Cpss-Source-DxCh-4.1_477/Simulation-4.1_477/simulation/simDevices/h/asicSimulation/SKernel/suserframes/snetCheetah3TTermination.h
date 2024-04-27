/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetCht3TTermination.h
*
* DESCRIPTION:
*       Cheetah3 Asic Simulation .
*       Tunnel Termination Engine processing for incoming frame.
*       header file.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 35 $
*
*******************************************************************************/
#ifndef __snetCht3TTermination
#define __snetCht3TTermination

#include <asicSimulation/SKernel/suserframes/snetCheetahPclSrv.h>
#include <asicSimulation/SKernel/suserframes/snetCheetah2TTermination.h>
#include <asicSimulation/SKernel/suserframes/snetXCatPcl.h>
#include <asicSimulation/SKernel/suserframes/snetLion3Tcam.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* number of rows depends on number of ipv4 entries */
#define SNET_CHT3_TUNNEL_TCAM_SIZE_CNS(dev) ((dev)->routeTcamInfo.numEntriesIpv4 / 4)

/* values for field : vlan assignment */
#define SNET_CHT3_CAPWAP_VLAN_ASSIGNMENT_BASED_MAC_SA_CNS                       0
#define SNET_CHT3_CAPWAP_VLAN_ASSIGNMENT_BASED_THIS_ENTRY_VLAN_ASSIGNMENT_CNS   1
#define SNET_CHT3_CAPWAP_VLAN_ASSIGNMENT_BASED_802_3_TAG_CNS                    2

#define SRC_MAC_FROM_TUNNEL_DSCR(descrPtr) \
    descrPtr->startFramePtr + SGT_MAC_ADDR_BYTES

#define DST_MAC_FROM_TUNNEL_DSCR(descrPtr) \
    descrPtr->startFramePtr


/*
 * typedef: enum SKERNEL_CHT3_TTI_KEY_TYPE_ENT
 *
 * Description:
 *      TTI key type.
 *
 * Enumerations:
 *
 *      SKERNEL_CHT3_TTI_KEY_IPV4_E            - IPv4 TTI key type
 *      SKERNEL_CHT3_TTI_KEY_MPLS_E            - MPLS TTI key type
 *      SKERNEL_CHT3_TTI_KEY_ETH_E             - Ethernet TTI key type
 *      SKERNEL_XCAT_TTI_KEY_MIM_E             - IEEE 802.1ah TTI key type
 *      SKERNEL_LION3_TTI_KEY_UDB_PKT_TYPE_X_E - Packet Type 0-15 UDB TTI key type
 */
typedef enum
{
    SKERNEL_CHT3_TTI_KEY_IPV4_E  = 0,
    SKERNEL_CHT3_TTI_KEY_MPLS_E  ,
    SKERNEL_CHT3_TTI_KEY_ETH_E   ,
    SKERNEL_XCAT_TTI_KEY_MIM_E   ,

    SKERNEL_LION3_TTI_KEY_UDB_PKT_TYPE_0_E,
    SKERNEL_LION3_TTI_KEY_UDB_PKT_TYPE_1_E,
    SKERNEL_LION3_TTI_KEY_UDB_PKT_TYPE_2_E,
    SKERNEL_LION3_TTI_KEY_UDB_PKT_TYPE_3_E,
    SKERNEL_LION3_TTI_KEY_UDB_PKT_TYPE_4_E,
    SKERNEL_LION3_TTI_KEY_UDB_PKT_TYPE_5_E,
    SKERNEL_LION3_TTI_KEY_UDB_PKT_TYPE_6_E,
    SKERNEL_LION3_TTI_KEY_UDB_PKT_TYPE_7_E,
    SKERNEL_LION3_TTI_KEY_UDB_PKT_TYPE_8_E,
    SKERNEL_LION3_TTI_KEY_UDB_PKT_TYPE_9_E,
    SKERNEL_LION3_TTI_KEY_UDB_PKT_TYPE_10_E,
    SKERNEL_LION3_TTI_KEY_UDB_PKT_TYPE_11_E,
    SKERNEL_LION3_TTI_KEY_UDB_PKT_TYPE_12_E,
    SKERNEL_LION3_TTI_KEY_UDB_PKT_TYPE_13_E,
    SKERNEL_LION3_TTI_KEY_UDB_PKT_TYPE_14_E,
    SKERNEL_LION3_TTI_KEY_UDB_PKT_TYPE_15_E,

    SKERNEL_CHT3_TTI_KEY_TYPE_LAST_E    /* must be last */
}SKERNEL_CHT3_TTI_KEY_TYPE_ENT;


/*
    enum :  CHT3_TT_KEY_FIELDS_ID_ENT

    description : enum of the egress PCL TCAM fields

*/
typedef enum{
    CHT3_TT_KEY_FIELDS_ID_PCL_ID_E,
    CHT3_TT_KEY_FIELDS_ID_SOURCE_PORT_TRUNK_E,
    CHT3_TT_KEY_FIELDS_ID_SRC_IS_TRUNK_E,
    CHT3_TT_KEY_FIELDS_ID_VID_E,
    CHT3_TT_KEY_FIELDS_ID_PCKT_TAGGED_E,
    CHT3_TT_KEY_FIELDS_ID_MAC_DA_SA_E,
    CHT3_TT_KEY_FIELDS_ID_SRC_DEV_E,
    CHT3_TT_KEY_FIELDS_ID_TUNNEL_PROTOCOL_E,
    CHT3_TT_KEY_FIELDS_ID_SIP_ADDRESS_E,
    CHT3_TT_KEY_FIELDS_ID_DIP_ADDRESS_E,
    CHT3_TT_KEY_FIELDS_ID_IS_ARP_E,
    CHT3_TT_KEY_FIELDS_ID_RESERVED_155_159_E,
    CHT3_TT_KEY_FIELDS_ID_DSA_SOURCE_PORT_TRUNK_E,
    CHT3_TT_KEY_FIELDS_ID_DSA_SRC_IS_TRUNK_E,
    XCAT3_TT_KEY_FIELDS_ID_DSCP_E, /*xcat3*/
    CHT3_TT_KEY_FIELDS_ID_RESERVED_168_190_E,
    CHT3_TT_KEY_FIELDS_ID_RESERVED_191_E,
    CHT3_TT_KEY_FIELDS_ID_EXP0_E,
    CHT3_TT_KEY_FIELDS_ID_EXP1_E,
    CHT3_TT_KEY_FIELDS_ID_LABLE0_E,
    CHT3_TT_KEY_FIELDS_ID_LABLE1_E,
    CHT3_TT_KEY_FIELDS_ID_EXP2_E,
    CHT3_TT_KEY_FIELDS_ID_LABLE2_E,
    CHT3_TT_KEY_FIELDS_ID_NUM_LABLES_E,
    CHT3_TT_KEY_FIELDS_ID_NLP_ABOVE_MPLS_E,
    CHT3_TT_KEY_FIELDS_ID_UP0_E,
    CHT3_TT_KEY_FIELDS_ID_CFI0_E,
    CHT3_TT_KEY_FIELDS_ID_VLAN1_EXIST_E,
    CHT3_TT_KEY_FIELDS_ID_VLAN1_E,
    CHT3_TT_KEY_FIELDS_ID_UP1_E,
    CHT3_TT_KEY_FIELDS_ID_CFI1_E,
    CHT3_TT_KEY_FIELDS_ID_ETHER_TYPE_E,
    CHT3_TT_KEY_FIELDS_ID_MAC_2_ME_E,
    CHT3_TT_KEY_FIELDS_ID_RESERVED_125_159_E,

    /* XCAT fields */
    XCAT_TT_KEY_FIELDS_ID_DSA_SOURCE_ID_E,
    XCAT_TT_KEY_FIELDS_ID_DSA_QOS_PROFILE_E,
    XCAT3_TT_KEY_FIELDS_ID_GRP_E,/*xcat3*/
    XCAT_TT_KEY_FIELDS_ID_RESERVED_180_190_E,
    XCAT_TT_KEY_FIELDS_ID_RESERVED_91_E,
    XCAT_TT_KEY_FIELDS_ID_I_SID_E,
    XCAT_TT_KEY_FIELDS_ID_I_UP_E,
    XCAT_TT_KEY_FIELDS_ID_I_DP_E,
    XCAT_TT_KEY_FIELDS_ID_I_RES_1_E,
    XCAT_TT_KEY_FIELDS_ID_I_RES_2_E,
    XCAT_TT_KEY_FIELDS_ID_RESERVED_124_159_E,
    /* XCAT-C0 fields (not in xcat2-B,lion2-A) */
    XCAT_TT_KEY_FIELDS_ID_MPLS_CW_FIRST_NIBBLE_E,           /* MPLS only */
    XCAT_TT_KEY_FIELDS_ID_MPLS_INNER_PACKET_TAG_0_EXISTS_E ,/* MPLS only */
    XCAT_TT_KEY_FIELDS_ID_MPLS_INNER_PACKET_TAG_0_VID_E ,   /* MPLS only */

    /* CH3+ (CAPWAP extra support) fields */
    CHT3_TT_KEY_FIELDS_ID_CAPWAP_IPV4_SIP_ADDRESS_E,
    CHT3_TT_KEY_FIELDS_ID_CAPWAP_IPV4_DIP_ADDRESS_E,
    CHT3_TT_KEY_FIELDS_ID_CAPWAP_IPV4_SRC_UDP_PORT_E,
    CHT3_TT_KEY_FIELDS_ID_CAPWAP_IPV4_DEST_UDP_PORT_E,
    CHT3_TT_KEY_FIELDS_ID_CAPWAP_IPV4_CLEAR_OR_ADD_DTLS_PAYLOAD_E,
    CHT3_TT_KEY_FIELDS_ID_CAPWAP_IPV4_HEADER_WORD0_E,
    CHT3_TT_KEY_FIELDS_ID_CAPWAP_IPV4_BSSID_OR_TA_OR_RADIO_MAC_E,
    CHT3_TT_KEY_FIELDS_ID_CAPWAP_IPV4_FRAME_CONTROL_TO_DS_E,
    CHT3_TT_KEY_FIELDS_ID_CAPWAP_IPV4_FRAME_CONTROL_FROM_DS_E,
    CHT3_TT_KEY_FIELDS_ID_CAPWAP_IPV4_FRAME_CONTROL_TYPE_E,
    CHT3_TT_KEY_FIELDS_ID_CAPWAP_IPV4_FRAME_CONTROL_SUBTYPE_E,
    CHT3_TT_KEY_FIELDS_ID_CAPWAP_IPV4_802_11_PROTECTED_E,

    CHT3_TT_KEY_FIELDS_ID_CAPWAP_IPV6_SIP_ADDRESS_E,/*word 0*/
    CHT3_TT_KEY_FIELDS_ID_CAPWAP_IPV6_DIP_ADDRESS_E,/*word 0*/
    CHT3_TT_KEY_FIELDS_ID_CAPWAP_IPV6_SRC_UDP_PORT_E,
    CHT3_TT_KEY_FIELDS_ID_CAPWAP_IPV6_DEST_UDP_PORT_E,
    CHT3_TT_KEY_FIELDS_ID_CAPWAP_IPV6_CLEAR_OR_ADD_DTLS_PAYLOAD_E,
    CHT3_TT_KEY_FIELDS_ID_CAPWAP_IPV6_HEADER_WORD0_E,
    CHT3_TT_KEY_FIELDS_ID_CAPWAP_IPV6_BSSID_OR_TA_OR_RADIO_MAC_E,
    CHT3_TT_KEY_FIELDS_ID_CAPWAP_IPV6_FRAME_CONTROL_TO_DS_E,
    CHT3_TT_KEY_FIELDS_ID_CAPWAP_IPV6_FRAME_CONTROL_FROM_DS_E,
    CHT3_TT_KEY_FIELDS_ID_CAPWAP_IPV6_FRAME_CONTROL_TYPE_E,
    CHT3_TT_KEY_FIELDS_ID_CAPWAP_IPV6_FRAME_CONTROL_SUBTYPE_E,
    CHT3_TT_KEY_FIELDS_ID_CAPWAP_IPV6_802_11_PROTECTED_E,

    CHT3_TT_KEY_FIELDS_ID_CAPWAP_IPV6_SIP_1_ADDRESS_E,/*word 1*/
    CHT3_TT_KEY_FIELDS_ID_CAPWAP_IPV6_SIP_2_ADDRESS_E,/*word 2*/
    CHT3_TT_KEY_FIELDS_ID_CAPWAP_IPV6_SIP_3_ADDRESS_E,/*word 3*/

    CHT3_TT_KEY_FIELDS_ID_CAPWAP_IPV6_DIP_1_ADDRESS_E,/*word 1*/
    CHT3_TT_KEY_FIELDS_ID_CAPWAP_IPV6_DIP_2_ADDRESS_E,/*word 2*/
    CHT3_TT_KEY_FIELDS_ID_CAPWAP_IPV6_DIP_3_ADDRESS_E,/*word 3*/

    /* Lion fields */
    LION_TT_KEY_FIELDS_ID_SRC_PORT_GROUP_BITS_0_1_E,/* bits 0,1 of src core id */

    /* Lion 3 fields */
    LION3_TT_KEY_FIELDS_ID_DSA_SRC_DEV_7_11_E,           /* IPv4 , MPLS , MIM */
    LION3_TT_KEY_FIELDS_ID_DSA_SRC_EPORT_TRUNK_7_11_E,   /* all keys */
    LION3_TT_KEY_FIELDS_ID_E_VLAN_12_13_E,               /* all keys */
    LION2_TT_KEY_FIELDS_ID_SRC_PORT_GROUP_BITS_0_2_E,    /* (from Lion2) all keys : src core id */
    LION3_TT_KEY_FIELDS_ID_ETHERNET_TAG_0_TPID_INDEX_E,  /* ETH only */
    LION3_TT_KEY_FIELDS_ID_ETHERNET_TAG_1_TPID_INDEX_E,  /* ETH only */
    LION3_TT_KEY_FIELDS_ID_ETHERNET_DSA_SRC_DEV_7_11_E,  /* ETH only */
    LION3_TT_KEY_FIELDS_ID_ETHERNET_DSA_SOURCE_ID_5_11_E,/* ETH only */
    LION3_TT_KEY_FIELDS_MIM_ID_MAC_2_ME_E ,              /* MIM only */
    LION3_TT_KEY_FIELDS_MIM_INNER_PACKET_TAG_0_VID_E ,   /* MIM only */
    LION3_TT_KEY_FIELDS_MIM_INNER_PACKET_TAG_0_EXISTS_E ,/* MIM only */
    LION3_TT_KEY_FIELDS_MIM_INNER_PACKET_TAG_0_UP_E ,    /* MIM only */
    LION3_TT_KEY_FIELDS_MIM_INNER_PACKET_TAG_0_DEI_E ,   /* MIM only */


    CHT3_TT_KEY_FIELDS_ID_LAST_E
}CHT3_TT_KEY_FIELDS_ID_ENT;

/*
 * Typedef: enum LION3_MPLS_TRANSIT_TUNNEL_PARSING_MODE_ENT
 *
 * Description: sip5 transit tunnels parsing mode
 *
 * Enumerations:
 *
 *     LION3_MPLS_TRANSIT_TUNNEL_PARSING_MODE_IP_ONLY_E              - IPV4 or IPV6 parsing mode
 *     LION3_MPLS_TRANSIT_TUNNEL_PARSING_MODE_IP_OR_CW_PW_ETHERNET_E - skip over CW and parse passenger packet as Ethernet
 *     LION3_MPLS_TRANSIT_TUNNEL_PARSING_MODE_ETHERNET_NO_CW_PW_E    - parse passenger as Ethernet without CW
 */
typedef enum
{
    LION3_MPLS_TRANSIT_TUNNEL_PARSING_MODE_IP_ONLY_E  = 0,
    LION3_MPLS_TRANSIT_TUNNEL_PARSING_MODE_IP_OR_CW_PW_ETHERNET_E,
    LION3_MPLS_TRANSIT_TUNNEL_PARSING_MODE_ETHERNET_NO_CW_PW_E
}LION3_MPLS_TRANSIT_TUNNEL_PARSING_MODE_ENT;


/*  TTI
    actions for the VPLS support.
*/
typedef struct{
    GT_BIT          egressTagStateAssigned; /* flag internal for the simulation to remove the use of value '7' from the egressTagState */
    GT_BIT          unknownSaCmdAssigned; /* flag internal for the simulation to remove the use of value '7' from the unknownSaCmd */

    GT_U32          egressTagState;/*3 bits
                Used by HA
                Note: If there is no TTI match, DescOut<Egress Tag State> has a default value of 0x7
                same enumeration as in VLAN table
                0 = Untagged: Packet is transmitted through this port untagged.
                1 = Tag0: Tag0 should be used for VLAN tagging of outgoing packets.
                2 = Tag1: Tag1 should be used for VLAN tagging of outgoing packets.
                3 = OuterTag0_InnerTag1: The packets should be tagged with two tags, when the outermost is Tag0 and the innermost is Tag1.
                4 = OuterTag1_InnerTag0: The packets should be tagged with two tags, when the outermost is Tag1 and the innermost is Tag0.
                5 = PushTag0: Tag0 should be pushed immediately after the MAC SA.
                6 = PopOuterTag: The outermost VLAN tag of the packet should be removed
                7 = Not assigned by TTI/PCL. Use VLAN entry value (legacy mode)
    */
    GT_U32          unknownSaCmd; /*3 bits
                                    Used by Bridge
                                    0 = Forward
                                    1 = MirrorToCPU
                                    2 = TrapToCPU
                                    3 = HardDrop
                                    4 = SoftDrop
                                    5,6 = Reserved
                                    7 = Not assigned by TTI. Using Bridge port configuration register
                                        */
    GT_U32          srcMeshId; /*2 bits*/
    GT_U32          pwHasCw; /*1 bit
                        Used by TTI, Indicates PW has 32-bit control word after the bottom of label stack
    */
    GT_U32          pwTagMode; /*2 bits
    The following action assigns the proper VID for egress VLAN filtering, and signals the EQ through
    the new descriptor field Desc<Override VID0 with OrigVID> what to use for VID0 for egress tagging.
    0 = Disabled (BWC mode)
       DescOut<Override VID0 with OrigVID> = 0
    1 = User VLAN Mode - packet VID1 is used for egress tagging and egress VLAN filtering
       DescOut<OrigVID> = DescOut<VID1> // after possible TTI AE modification to VID1
       DescOut<Override VID0 with OrigVID> = 0
    2 = Service VLAN Push/Swap Mode - use TTI AE VID1 as VID0 for egress tagging and egress filtering
       DescOut<OrigVID> = TTI AE<VID1>
       DescOut<Override VID0 with OrigVID> = 1
       Do not use the TTI AE <VID1> to assign the DescOut<VID1>, regardless of the TTI AE <VLAN1 Command>
    3 = Service VLAN Keep Mode  use the incoming VID0 for egress tagging and egress filtering
       DescOut<OrigVID> = pre-TTI lookup VID0 assignment
       DescOut<Override VID0 with OrigVID> = 1

     Detailed explanation
    When the TTI AE <PW Tag Mode> is set to 0x2 (Service VLAN Push/Swap Mode), we use the TTI
    AE<VID1> as the value to set in the DescOut<ORigVID>, and not for DescOut<VID1> assignment,
    regardless of the TTI AE <VLAN1 Command>.
    When the TTI AE <PW Tag Mode> =0x1 (User VLAN Mode), then TTI AE<Tag1> command works
    normally. The DescOut<VID1> may be updated by the TTI AE <VID1>. Note that the
    DescOut<OrigVID> is set to the final OutDesc<VID1> after possible TTI AE modification to VID1.
    When the TTI AE <PW Tag Mode> = 0x3 (Service VLAN Keep Mode), then TTI AE<Tag1>
    command works normally. The OutDesc<VID1> may be updated by the TTI AE <VID1>. The
    OutDesc<OrigVID> is set to the pre-TTI lookup VID0 assignment.
    For example, say the TTI AE has the following configuration: <PW Tag Mode> = "Service VLAN
    with Push/Swap Mode", <VID1>=300.
    If packet A arrives and it has 2 tags: VID0=100, VID1=200, and we match the above rule, the TTI
    DescOut<VID1>=200 (it is not modified by the TTI AE), and DescOut<OrigVID>=300.
    If packet B arrives and it has 1 tags: VID0=100, and we match the above rule, the TTI
    DescOut<VID1>=0 (default value - it is not modified by the TTI AE), and DescOut<OrigVID>=300
*/
}SNET_XCAT_TTI_ACTION_VPLS_SUPPORT_STC;


/*
    enum :  SKERNEL_CH3_TT_PACKET_CMD_ENT

    description : enum of the TT forward command
    D.12.8.3 - T.T Action Entry. page 794

*/
typedef enum{
    SKERNEL_CHT3_TT_CMD_FORWARD_E =   0,
    SKERNEL_CHT3_TT_CMD_MIRROR_E    = 1,
    SKERNEL_CHT3_TT_CMD_TRAP_E      = 2,
    SKERNEL_CHT3_TT_CMD_HARD_DROP_E = 3,
    SKERNEL_CHT3_TT_CMD_SOFT_DROP_E = 4,
}SKERNEL_CH3_TT_PACKET_CMD_ENT;

/*
    Struct : SNET_CHT3_TT_ACTION_STC

    description : The tunnel action table is accessed ingress TT and
                  The line index of the matching rule is used
                  to index the TT action table and extract the action
                  to perform.
*/
typedef struct{
  /*  struct _snet_cht2_tt_action_stc; */

    GT_U32          ttPacketCmd;
    GT_U32          ttCpuCode;
    GT_U32          ttPassengerType;
    GT_U32          ttRedirectCmd;
    GT_U32          ttQoSMode;
    GT_U32          ttQoSPrecedence;
    GT_U32          ttQoSProfile;
    GT_U32          ttQoSTtust;
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
    GT_U32          ttVidCmd;
    GT_U32          ttTtl;
    GT_U32          ttVidCmd1;      /* Tag1 VLAN-ID assignment option (xCat and above) */
    GT_U32          ttVid1;         /* Tag1 VLAN-ID (xCat and above) */
    GT_U32          matchCounterIndex;
    GT_BOOL         matchCounterEn;
    GT_U32          tunnelPtr; /* The pointer to the tunnel start entry used to tunnel the packet */
    GT_U32          tunnelStart; /* indicates this action is a tunnel start point */

    GT_U32          tterminate;
    GT_U32          ttRouterLTT;
    GT_U32          ttVrf;
    GT_U32          ttSrcIdSetEn;
    GT_U32          ttSrcId;
    GT_U32          ttVntL2Echo;
    GT_U32          ttActionStop;
    GT_U32          ttTrunk ;
    GT_U32          ttvirtualSrcPort ;
    GT_U32          ttvirtualSrcDev ;
    GT_U32          ttUseVidx;


    GT_U32          policerIndex;
    GT_BOOL         policerEn;
    SNET_DST_INTERFACE_STC      tunnelIf; /* The egress interface to which the packet redirected to... */
}SNET_CHT3_TT_ACTION_STC;

/*  Struct : SNET_XCAT_TT_ACTION_STC

    description : The tunnel action table is accessed ingress TT and
                  The line index of the matching rule is used
                  to index the TT action table and extract the action
                  to perform.
    Fields:
    =======
    packetCmd               - Command assigned to packets matching this TTI rule. like in CH3
                              0x0 = FORWARD;
                              0x1 = MIRROR;
                              0x2 = TRAP;
                              0x3 = HARD_DROP;
                              0x4 = SOFT_DROP;
    userDefinedCpuCode      - If <Packet Command> = TRAP or MIRROR, then this code is the CPU Code
                              which will be passed to the CPU as part of the packet. like in CH3
    mirrorToAnalyzerPort    - When set , the packet is mirrored to the ingress analyzer port. like in CH3
                              0x0 = Disable;
                              0x1 = Enable;
    redirectCmd             - When this field has a non-zero value, the packet is redirected
                              to the respective forwarding engine. like in CH3
                              0x0 = NoRedirect; Do not redirect this packet.
                              0x1 = Egress Interface; Redirect this packet to the configured
                                    Egress Interface (valid for all TTI lookups).
                              0x2 = IPNextHop; Redirect to an IP Next Hop Entry
                                    (valid for IPv4/6 over X tunnel-terminated packets only).
                              0x4 = Assign VRF-ID;
    vrfId                   - VRF-ID assigned to the packet. This field is valid only
                              when <Redirect Command> = 4 (VRF-ID assignment). like in CH3
    routerLTTIndex          - Pointer to router's next hop entry
                              (more accurately, a pointer to the Router's LTT). like in CH3
    tunnelStart             - When set, packet is redirected to a tunnel start entry
                              pointed by <Tunnel Index>. This field is valid only when
                              <Redirect Command> = Redirect to Egress Interface (Policy-switching). like in CH3
                              0x0 = False; Packet is redirected to a non-Tunnel Egress Interface.
                              0x1 = True; Packet is redirected to a Tunnel Egress Interface
                                    (this is not supported for Tunnel Start entries of type
                                    x-over-IPv4, since Ethernet-over-IP Tunneling is not supported).
    tunnelStartPtr          - pointer to the Tunnel Start entry. This field is valid only when
                              <Redirect Command> = Redirect to Egress Interface (Policy-switching). like in CH3
    arpPtr                  - pointer to the ARP entry for MPLS routed packets. This field is valid only
                              when <Redirect Command> = Redirect to Egress Interface (Policy-switching). like in CH3
    ipclProfileIndex        - Pointer to IPCL configuration entry to be used when fetching
                              IPCL parameters. This may be used to overrides the Port/VLAN
                              based configuration entry selection based on the
                              <PCL[n] Override Config Index> fields below.
    tunnelStartPassengerType- Type of passenger packet being to be encapsulated. This field is valid
                              only when <Redirect Command> = 1 (Policy-switching) and <TunnelStart> = 1. like in CH3
                              0x0 = Ethernet;
                              0x1 = Other;
    vntL2Echo               - If set, then this a network testing flow. Therefore packet should
                              be redirected to an egress interface where prior to transmission
                              the packet's MAC SA and MAC DA are switched. like in CH3
    bindToCncCounter        - If set, the packet is bound to the counter specified in
                              <CNC Counter Index>.
                              0x0 = False;
                              0x1 = True;
    cncCounterIndex         - Counter index to be used if the <Bind To CNC Counter> is set.
    bindToPolicerMeter      - If set, the packet is bound to the meter specified in <Policer Index>.
                              0x0 = False;
                              0x1 = True;
    bindToPolicerCounter    - If set, the packet is bound to the counter specified in <Policer Index>.
                              When both <Bind To Policer Meter> and <Bind To Policer Counter> are set,
                              the <Policer Index> will be used for metering and the counter index
                              will be assigned by the metering entry.
                              0x0 = False;
                              0x1 = True;
    policerIndex            - Traffic profile or counter to be used when the <Bind To Policer Meter>
                              or <Bind To Policer Counter> are set.
    srcIdSetEn              - If set, then the <Source-ID> is assigned to the packet. like in CH3
                              0x0 = False;
                              0x1 = True;
    srcId                   - Source ID that is assign to the packet if
                              <Source-ID Assignment Enable> is set. like in CH3
    actionStop              - If set, the packet will not be affected by any searches in the IPCL mechanism. like in CH3
    bypassBridge            - If set, the Bridge engine is bypassed for this packet. like in CH3
    bypassIngressPipe       - If set, the ingress pipe is bypassed for this packet.
    policy2LookupMode       - Controls the index used for IPCL1 lookup
                              0x0 = Retain; Use SrcPort <Lookup1 PCL Cfg Mode>
                              0x1 = Override; Use TTI Action <IPCL Profile Index> to access the
                                    IPCL configuration table for this lookup.
    policy1LookupMode       - Controls the index used for IPCL0-1 lookup
                              0x0 = Retain; Use SrcPort <Lookup0-1 PCL Cfg Mode>
                              0x1 = Override; Use TTI Action <IPCL Profile Index> to access the
                                    IPCL configuration table for this lookup.
    policy0LookupMode       - Controls the index used for IPCL0-0 lookup
                              0x0 = Retain; Use SrcPort <Lookup0-0 PCL Cfg Mode>
                              0x1 = Override; Use TTI Action <IPCL Profile Index> to access the
                                    IPCL configuration table for this lookup.
    vidPrecedence           - 0x0 = Soft; VID assigned by TTI may be modified by the following
                                    VLAN assignment mechanisms.
                              0x1 = Hard; VID assigned by TTI cannot be modified by the following
                                    VLAN assignment mechanisms. like in CH3
    nestedVidEn             - If set, the packet is classified as Untagged. Relevant only if
                              the packet is Ethernet or tunnel-terminated Ethernet-over-X. like in CH3
                              0x0 = Disable;
                              0x1 = Enable;
    vid0Cmd                 - This field determines the VLAN assigned to the packet matching the TTI entry
                              0x0 = Do Not Modify; VLAN0 ID is unchanged by TTI.
                              0x1 = Untagged or Prio Tagged; The entry's <VLAN-ID0> is assigned ti tag0
                                    untagged or priority tagged packets; for tag0 tagged packets the
                                    VLAN0 ID is set according to the tag0 VID
                              0x2 = Tagged; The entry's <VLAN-ID0> is assigned to tag0 tagged packets;
                                    for untagged or priority-tagged packets the VLAN0 ID assignment
                                    remains as assigned by previous VID0 assignment mechanism.
                              0x3 = All; The entry's <VLAN-ID0> is assigned to all packets
    vid0                    - vlan 0 assignment
    vid1Cmd                 - This field determines the VLAN1 assigned to the packet matching the TTI entry
                              0x0 = Untagged or Prio Tagged; The entry's <VLAN-ID1> is assigned to tag1
                                    untagged or priority tagged packets; for tag1 tagged packets,
                                    i.e. packets with VLAN1, the VLAN1 ID is set according to the tag1 VID
                              0x1 = All; The entry's <VLAN-ID1> is assigned to all packets
    vid1                    - vlan 1 assignment
    ttl                     - TTL to be used in MPLS header that is Pushed or Swapped according
                              to MPLS Command. If (TTL == 0) the TTL is taken from the packet's header,
                              else TTL is set according to this field.
    ttlDecEn                - If set, the TTL is decremented by one.
                              0x0 = False;
                              0x1 = True;
    qoSPrecedence           - 0x0 = Soft; QoS Parameters may be modified by the following engines. like in CH3
                              0x1 = Hard; QoS Parameters cannot be modified by the following engines.
    qoSProfile              - QoS Profile
    modifyDscp              - 0x0 = Keep Previous; Do not modify the previous <Modify DSCP enable> setting
                              0x1 = Enable; Set <Modify DSCP enable> flag to 1
                              0x2 = Disable; Set <Modify DSCP enable> flag to 0
    modifyUp                - 0x0 = Keep Previous; Do not modify the previous <Modify UP enable> setting
                              0x1 = Enable; Set <Modify UP enable> flag to 1
                              0x2 = Disable; Set <Modify UP enable> flag to 0
    keepPreviousQoS         - 0x0 = False; Assign QoS profile based on TTI-AE QoS fields.
                              0x1 = True; Keep prior QoS Profile assignment.
    trustUp                 - 0x0 = Untrust; Do not trust packets L2 QoS.
                              0x1 = Trust; If packet is tunnel-terminated, trust passenger packet's L2 QoS.
                                    If packet is not tunnel-terminated, trust packet's L2 QoS.
    trustDscp               - 0x0 = Untrust; Do not trust packets L3 QoS
                              0x1 = Trust; If packet is tunnel-terminated, trust passenger packet's L3 QoS.
                                    If packet is not tunnel-terminated, trust packet's L3 QoS.
    trustExp                - 0x0 = Untrust; Do not trust packets EXP
                              0x1 = Trust; Use MPLS header's EXP to assign QoS Profile.
                                    The specific EXP used is selected based on the MPLS Command.
    dscpToDscpRemapingEn    - 0x0 = Disable; Packets DSCP is not remapped.
                              0x1 = Enable; Packets DSCP is remapped.
    up0                     - tag0 UP assignment
    up1Cmd                  - 0x0 = Assign VLAN1 Untagged; If packet does not contain Tag1 assign
                                    according to action entry's <UP1>, else use Tag1<UP>
                              0x1 = Assign VLAN0 Untagged; If packet contains Tag0 use Tag0<UP0>,
                                    else use action entry's <UP1> field.
                              0x2 = Assign All; Assign action entry's <UP1> field to all packets.
    up1                     - Action UP1 assignment
    passengerPacketType     - Passenger packet type when packet is tunnel terminated.
                              0x0 = IPv4/6; Final passenger type, 4 or 6, will be determined based on the
                                    first nibble of the passenger header as this is the IP version field.
                              0x1 = MPLS; Passenger is MPLS.
                              0x2 = EthernetCRC; Ethernet with CRC
                              0x3 = EthernetNoCRC; Ethernet without CRC
    copyTtlFromOuterHeader  - If set, the packets TTL is assigned according to the tunnel header
                              rather than from passenger packets TTL/Hop-Limit.
                              0x0 = No Copy;
                              0x1 = Copy;
    tunnelTerminateEn       - Indicates that the packet's tunnel header must be removed and further
                              processing in the pipe will be done on passenger packet only. like in CH3
                              0x0 = Disable;
                              0x1 = Enable;
    mplsCmd                 - MPLS Action applied to the packet
                              0x0 = NOP;
                              0x1 = Swap;
                              0x2 = Push;
                              0x3 = Pop1_label;
                              0x4 = Pop2_labels;
                              0x7 = Pop_and_Swap;
    tunnelIf                - egress interface. like in CH3

    oamProcessEnable        - When enabled the packet is bound to an entry in the OAM Table

    oamProfile              - The OAM profile determines the set of UDBs where the key attributes (opcode,
                              MEG level, RDI, MEG level) of the OAM message are taken from.
    ptpTimeStampEnable      - When enabled, it indicates that a timestamp should be inserted into the packet

    ptpTimeStampOffsetIndex - When <Timestamp Enable> is enabled, this field indicates the offset index for
                              the timestamp offset table.
    oamInfo                 - OAM info. Relevant to eArch devices
    vplsInfo                - vpls Info (xCat-C0)
*/
typedef struct{

    SKERNEL_CH2_TT_PACKET_CMD_ENT  packetCmd;
    GT_U32          userDefinedCpuCode;
    GT_BOOL         mirrorToAnalyzerPort;
    GT_U32          redirectCmd; /* see PCL_TTI_ACTION_REDIRECT_CMD_ENT */
    GT_U32          vrfId;
    GT_U32          routerLTTIndex;
    GT_BOOL         tunnelStart;
    GT_U32          tunnelStartPtr;
    GT_U32          arpPtr;
    GT_U32          ipclProfileIndex;
    GT_U32          tunnelStartPassengerType;
    GT_U32          vntL2Echo;
    GT_BOOL         bindToCncCounter;
    GT_U32          cncCounterIndex;
    GT_BOOL         bindToPolicerMeter;
    GT_BOOL         bindToPolicerCounter;
    GT_U32          policerIndex;
    GT_BOOL         srcIdSetEn;
    GT_U32          srcId;
    GT_U32          actionStop;
    GT_U32          bypassBridge;
    GT_U32          bypassIngressPipe;
    GT_U32          policy2LookupMode;
    GT_U32          policy1LookupMode;
    GT_U32          policy0LookupMode;
    GT_U32          vidPrecedence;
    GT_BOOL         nestedVidEn;
    GT_U32          vid0Cmd;
    GT_U32          vid0;
    GT_U32          vid1Cmd;
    GT_U32          vid1;
    GT_U32          ttl;
    GT_BOOL         ttlDecEn;
    GT_U32          qoSPrecedence;
    GT_U32          qoSProfile;
    GT_U32          modifyDscp;
    GT_U32          modifyUp;
    GT_BOOL         keepPreviousQoS;
    GT_U32          trustUp;
    GT_U32          trustDscp;
    GT_U32          trustExp;
    GT_BOOL         dscpToDscpRemapingEn;
    GT_U32          up0;
    GT_U32          up1Cmd;
    GT_U32          up1;
    GT_U32          passengerPacketType;
    GT_U32          copyTtlFromOuterHeader;
    GT_BOOL         tunnelTerminateEn;
    GT_U32          mplsCmd;
    SNET_DST_INTERFACE_STC tunnelIf;
    SNET_LION3_PCL_ACTION_OAM_STC oamInfo;

    SNET_XCAT_TTI_ACTION_VPLS_SUPPORT_STC   vplsInfo;

    SNET_XCAT_TTI_ASSIGN_LOGIC_PORT_ACTION_STC srcLogicPortInfo;

}SNET_XCAT_TT_ACTION_STC;

/**
 *
 *  Struct : SNET_LION_TT_ACTION_STC
 *
 *
 *  Description : The tunnel action table structure for Lion
 *
 *  Fields:
 *
 *      xcatTTActionData        - xCat TTI action data
 *      multiPortGroupTTIEn     - Relevant only when Redirect_Command="NoRedirect"
 *                                Determines whether the packet is forwarded
 *                                to a ring port for another TTI lookup.
 *                                This field determines the default target port
 *                                in case there is no redirection by the TTI.
 *                                0 = port 62, i.e. NULL port. a SNET_CHT_NULL_PORT_CNS
 *                                1 = port 0.
 *                                The default port is then used by the EQ:
 *                                if it is port 62, the packet is dropped.
 *                                If it is port 0, the packet may be forwarded to a ring port.
 *                                Thus, this field in fact determines whether multi-core TTI is enabled.
 *
 *      hashMaskIndex           - Hash Mask Index
 *      resetSrcPortGroupId     - Relevant when Redirect_Command != "Egress_Interface"
 *      modifyMacDa             - Relevant when Redirect_Command="Egress_Interface"
 *      modifyMacSa             - Relevant when Redirect_Command="Egress_Interface"
 *
*/
typedef struct{
    SNET_XCAT_TT_ACTION_STC xcatTTActionData;
    GT_BIT multiPortGroupTTIEn;
    GT_U32 hashMaskIndex;
    GT_BIT resetSrcPortGroupId;
    GT_BIT modifyMacDa;
    GT_BIT modifyMacSa;
}SNET_LION_TT_ACTION_STC;

/**
 *
 *  Struct : SNET_E_ARCH_TT_ACTION_STC
 *
 *
 *  Description : The tunnel action table structure for E-Arch
 *
 *  Fields:
 *
 *      baseActionData          - base TTI action data
 *      analyzerIndex           - mirror analyzer index
 *      sourceEPortAssignmentEnable - Enable assignment of Source ePort number from this entry
 *                                       Logic:
 *                                       0 - Disabled
 *                                       1 - Enabled - New source ePort number is taken from TTI Action Entry<Source ePort>.
 *
 *      sourceEPort             - Source ePort number assigned by TTI entry when
 *                                  <Source ePort Assignment Enable> = Enabled
 *
*/
typedef struct{
    SNET_LION_TT_ACTION_STC baseActionData;

    GT_U32      analyzerIndex;
    GT_BIT      sourceEPortAssignmentEnable;
    GT_U32      sourceEPort;
    GT_U32  trustQosMappingTableIndex;/*    There are 8 global Trust QoS Mapping Table used for Trust L2, L3, EXP, DSCP-to-DSCP mapping.
                                            This field selects which of the 8 global sets of Trust QoS Mapping tables is used
                            relevant only in eArch*/
    GT_BIT  up2QosProfileMappingMode;/*Selects the index used to select a specific UP2QoS-Profile mapping table
                        0: The index is <Trust QoS Mapping Table Index>
                        1: The index is UP0 or UP1 as the UP configured for indexing a specific UP2QoS-Profile table. In this mode only tables 0-7 can be accessed
                            relevant only in eArch*/
    GT_U32  lLspQosProfileEnable;/* L-LSP QoSProfile Enable --> This is used for supporting MPLS L-LSP QoS, where the scheduling traffic class is derived from the MPLS label and the drop precedence is derived from the EXP.
                                Logic:  0 - Disabled
                                        1 - Enabled - the packet is assigned a QoSProfile that is the sum of the TTI Action<QoS Profile> + packet outer label EXP.
                                    Note This field is only relevant if TTI Action <TRUST EXP> is unset
                            relevant only in eArch*/

    GT_BIT  ipclOverrideProfileIndex;/*enable override of the IPCL profile index (relevant when Redirect_Command !=""Egress_Interface")*/
    GT_BIT  ttiReservedAssignEnable;/*enable the assignment of reserved bits enable the assignment of reserved bits Redirect_Command !="Egress_Interface"*/
    GT_U32  ttiReservedAssignValue;/*"Internal field - reserved bits for future use. These bits are assigned by TTI action, and are passed in the descriptor
                                    to the TxQ, for future use of units futher up in the pipeline.
                                    Internal field - reserved bits for future use. These bits are assigned by TTI action, and are passed in the descriptor
                                    to the TxQ, for future use of units futher up in the pipeline. Redirect_Command !=""Egress_Interface"*/
    GT_BIT  egressFilterRegistered;
    GT_U32  flowId;
    GT_U32  ttHeaderLength;/*" If 0, then the TT Header Len is default TT algorithm for each key type
                               Else this is the Tunnel header length in units of 2B.
                               Note: Tunnel header begins at the start of the L3 header,
                               i.e. immediately after the EtherType field.*/
    GT_BIT  isPtpPacket;
/*    GT_BIT  timestampEnable; called ptpTimeStampEnable already in SNET_LION3_PCL_ACTION_OAM_STC*/
/*    GT_U32  offsetIndex; called ptpTimeStampOffsetIndex already in SNET_LION3_PCL_ACTION_OAM_STC*/
    GT_U32  ptpTriggerType;
    GT_U32  ptpOffset;
/*  GT_U32      GT_BIT  oamProcessingEnable; called oamProcessEnable already in SNET_LION3_PCL_ACTION_OAM_STC */
    GT_U32  cwBasedPseudoWire;
    GT_BIT  ttlExpiryVccvEnable;
    GT_U32  pwe3FlowLabelExist;
    GT_BIT  pwCwBasedETreeEnable;
    GT_BIT  rxIsProtectionPath;
    GT_BIT  rxEnableProtectionSwitching;
    GT_BIT  setMAC2ME;
/*  GT_U32  oamProfile; already in SNET_LION3_PCL_ACTION_OAM_STC */
    GT_BIT  applynonDataCwCommand;
    GT_U32  pwTagMode;
    GT_BIT  continueToNextTtiLookup;
/*    GT_BIT  channelTypeToOpcodeMappingEn; already in SNET_LION3_PCL_ACTION_OAM_STC */
    GT_U32  passengerParsingOfTransitMplsTunnelEnable;
    GT_BIT  passengerParsingOfNonMplsTransitTunnelEnable;
    GT_U32  ipclUdbConfigurationTableUdeIndex;


}SNET_E_ARCH_TT_ACTION_STC;

typedef struct {
    GT_U32          vid0Cmd;
    GT_U32          vid0;
    GT_U32          vid1Cmd;
    GT_U32          vid1;
    GT_U32          up0;
    GT_U32          up1Cmd;
    GT_U32          up1;
    GT_BIT          vidModified;
    GT_BIT          allowVid1Assignment;
}INTERNAL_TTI_DESC_INFO_VLAN_CMD_INFO_STC;

/* extra info that need to be passed within the TTI functions ,
   and not need to overload the frame descriptor ,.

   this info is 'for simulation only' purpose and not try to simulate any
   internal TTI HW unit !
*/
typedef struct INTERNAL_TTI_DESC_INFO_STCT{
    GT_U32      cncIndex;        /* valid when != SMAIN_NOT_VALID_CNS.
                                    indicate the cnc index that relate to the action */
    GT_BOOL     keepPreviousQoS;/* do we keep QoS prior to TTI action assignment (port/protocol based) */

    GT_BIT              ttiActionReassignsTheSourceEPort;  /* indication that the TTI action re-assigned new srcEport */
    /* info that the TRILL use/set */
    struct{
        GT_BIT              reassignsTheSourceEPort;  /* indication that the TRILL re-assigned new srcEport */
        /*NOTE: If TRILL engine reassigns the source ePort (and thus also resets source device to be local device),
            the assigned values does not affect the TTI key assignment logic for source ePort and source device fields */
        GT_U32              newSrcEPort;    /* the new src EPort that the TRILL assigns */
    }trillInfo;

    GT_U32          isEthOverIpGre;/* is ethernet over IP GRE packet TT */
    GT_U32          ipv4KeyProtocol;/*ipv4 key - L4 protocol type */

    struct{
        SKERNEL_EXT_PACKET_CMD_ENT packetCmd;/* packet command from the matched action */
    }actionInfo;/*info from the action that may be needed outside the scope of lookup */

    SKERNEL_CHT3_TTI_KEY_TYPE_ENT   ttiKeyType; /* TTI key type */


    GT_BIT ttiPassengerOuterTagIsTag0_1_valid;/* indicate that the ttiPassengerOuterTagIsTag0_1 is valid for current processing
                                            0 - <ttiPassengerOuterTagIsTag0_1> not valid
                                            1 - <ttiPassengerOuterTagIsTag0_1> valid (trill,MIM)

                                           in the preTtiLookup we have <ttiPassengerOuterTagIsTag0_1>
                                           0 - Outer Tag exists if passenger outer VLAN Tag TPID = Tag0 TPID
                                           1 - Outer Tag exists if passenger outer VLAN tag TPID = Tag1 TPID
                                            NOTE: This applied to 802.1ah TTI Key and TRILL TTI Key */
    struct{ /* fcoeL3Info */
        GT_U8   ver;          /* ver field value */
        GT_U8   sof;          /* SOF field value */

        struct{ /* VftInfo */
            GT_U8  rCtl;      /* R_CTL     field value */
            GT_U8  ver;       /* Ver       field value */
            GT_U8  type;      /* Type      field value */
            GT_U32 vfId;      /* VF_ID (Virtual fabric ID) field value */
            GT_U8  hopCt;     /* HopCt     field value */
        }vftInfo; /* Outermost Virtual Fabric Tags information */
    }fcoeL3Info;

    GT_U32 actionId;/* SIP5: action ID : 0 or 1 . support 2 actions for the lookup */
    GT_U32 srcIdMask;/* SIP5 : mask on the SSTID bits that the action can modify */

    struct{
        GT_BOOL                    isValid;         /* indication that this lookup is valid and need to be processed */

        GT_BOOL                    isActionDataValid;  /* indication that this action data is valid */
        SNET_E_ARCH_TT_ACTION_STC  actionEarchData; /* lookup action data */
        SNET_XCAT_TT_ACTION_STC   *actionXcatPtr;   /* pointer to xCat action data */
        SNET_LION_TT_ACTION_STC   *actionLionPtr;   /* pointer to lion action data */
    }lookupsInfo[SIP5_TCAM_MAX_NUM_OF_HITS_CNS];

    GT_U32 maxNumOfLookups; /* TTI unit of sip5 support only 2 lookups , sip5_10 support 4 */

    /* fields from the header parser , that need to be used from the 'tunnel header'
       and not from the passenger */
    struct{
        GT_BIT   ipHeaderError;
        GT_BIT   ipTtiHeaderError;
        GT_BIT   isIpV6EhHopByHop;
        GT_BIT   isIpV6EhExists;
        GT_BIT   greHeaderError;
        GT_BIT   isIPv4;
        GT_BIT   isIp;
    }preTunnelTerminationInfo;

    GT_BOOL      needToDoExceptionCheckPart2;/* indication to do exception check part 2 */

    /* mpls info */
    GT_U32  firstNibbleAfterSBitLabel;/*First nibble after S_BIT_LABEL*/
    GT_U32  pwe3CwWord;               /* PWE3 CW CONTROL/data word */

    INTERNAL_TTI_DESC_INFO_VLAN_CMD_INFO_STC vlan_origInfo;
    INTERNAL_TTI_DESC_INFO_VLAN_CMD_INFO_STC vlan_actionInfo;

}INTERNAL_TTI_DESC_INFO_STC;

/*******************************************************************************
* snetCht3TTermination
*
* DESCRIPTION:
*        T.T Engine processing for outgoing frame on Cheetah3
*        asic simulation.
*        T.T processing , T.T assignment ,key forming , 1 Lookup ,
*        actions to descriptor processing
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - pointer to frame data buffer Id
*       internalTtiInfoPtr -  pointer to internal TTI info
* OUTPUTS:
*       descrPtr     - pointer to updated frame data buffer Id
*
* RETURN:
*
* COMMENTS:
*   T.T has only one lookup cycle.
*   D.12.8.3 : T.T Registers
*
*******************************************************************************/
GT_VOID snetCht3TTermination
(
    IN    SKERNEL_DEVICE_OBJECT             * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC   * descrPtr,
    IN    INTERNAL_TTI_DESC_INFO_STC        * internalTtiInfoPtr
);

typedef enum{
    PACKET_TYPE_USED_FOR_CLIENT_TTI_E,
    PACKET_TYPE_USED_FOR_CLIENT_IPCL_E,
    PACKET_TYPE_USED_FOR_CLIENT_EPCL_E,

    PACKET_TYPE_USED_FOR_CLIENT___LAST___E/* must be last*/

}PACKET_TYPE_USED_FOR_CLIENT_ENT;

/*******************************************************************************
*   pcktTypeAssign
*
* DESCRIPTION:
*        Packet type assignment (for xCat and above)
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
*        clientType - client type.
* OUTPUTS:
*        descrPtr  - pointer to the frame's descriptor.
*
*******************************************************************************/
GT_VOID pcktTypeAssign
(
    IN    SKERNEL_DEVICE_OBJECT             *devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC   *descrPtr,
    IN    PACKET_TYPE_USED_FOR_CLIENT_ENT   clientType
);

/*******************************************************************************
* snetTtiTablesFormatInit
*
* DESCRIPTION:
*        init the format of TTI tables.
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*
* OUTPUTS:
*       None.
*
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
void snetTtiTablesFormatInit(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __snetCht3TTermination */


