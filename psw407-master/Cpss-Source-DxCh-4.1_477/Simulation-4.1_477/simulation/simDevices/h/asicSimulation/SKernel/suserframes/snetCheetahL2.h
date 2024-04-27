/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetCheetahL2.h
*
* DESCRIPTION:
*       This is a external API definition for L2 bridge engine processing
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 46 $
*
*******************************************************************************/
#ifndef __snetCheetahL2h
#define __snetCheetahL2h

#include <asicSimulation/SKernel/smain/smain.h>
#include <common/Utils/FrameInfo/sframeInfoAddr.h>
#include <common/Utils/PresteraHash/smacHashDx.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* TCP/IP Flags search type */
typedef enum {
    SNET_CHEETAH2_TCP_ZERO_FLAGS_E = 0,
    SNET_CHEETAH2_TCP_SYN_FLAGS_E = 0x2,
    SNET_CHEETAH2_TCP_SYN_ACK_FLAGS_E = 0x12,
    SNET_CHEETAH2_TCP_FIN_URG_PSH_FLAGS_E = 0x29,
    SNET_CHEETAH2_TCP_SYN_FIN_FLAGS_E = 0x3,
    SNET_CHEETAH2_TCP_SYN_RST_FLAGS_E = 0x6,
}SNET_CHEETAH2_TCP_FLAGS_ENT;


typedef enum {
    SNET_CHEETAH_L2I_LOOKUP_MODE_SA_E,
    SNET_CHEETAH_L2I_LOOKUP_MODE_DA_E,
}SNET_CHEETAH_L2I_LOOKUP_MODE_ENT;

typedef struct {
    GT_BIT                      valid;    /* 1 - valid, 0 - not valid entry  */
    GT_BIT                      found;    /* 1 - found, 0 - not found */
    SKERNEL_EXT_PACKET_CMD_ENT  saCmd;    /* command - if not found == FORWARD */
    GT_BIT                      isStatic; /* the entry is static */
    GT_BIT                      isMoved;  /* the port is moved, entry port/device differs from packet src port/device  */
    GT_BIT                      stormPrevent; /* the entry exist but not valid for DA search */
    GT_U32                      saQosProfile; /* QoS profile for SA */
    GT_BIT                      rxSniff;  /* if 1 - send frame to rx sniffer */
    GT_U32                      sstId;    /* SST-ID - 5-bit source-ID assignment for this source MAC entry. The source-ID assignment is used by the
                                            egress source-ID filtering mechanism. */
    GT_U32                      entryIndex ; /* index of entry - of found entry or of 'to be learned into'  */
    GT_BOOL                     sendFailedNa2Cpu;   /* GT_TRUE - send failed to learn New Addr Message to CPU */

}SNET_CHEETAH_L2I_SA_LOOKUP_INFO;

typedef struct {
    GT_BIT                      found;    /* 1 - found, 0 - not found */
    SKERNEL_EXT_PACKET_CMD_ENT  daCmd;    /* command - if not found == FORWARD */
    GT_BIT                      isStatic; /* the entry is static */
    GT_U32                      daQosProfile; /* QoS profile for SA */
    GT_BIT                      rxSniff;  /* if 1 - send frame to rx sniffer */
    GT_U32                      devNum;    /* device Id on which MAC was learned */
    GT_BIT                      useVidx;  /* 1- multicast bridging */
    union {
        GT_U32              vidx;   /* vidx for MAC multicast or IPM bridging
                                    0-0xFFE - The Multicast group table index for this MAC Multicast or IPv4/v6 Multicast entry.
                                    0xFFF - The Multicast packet is forwarded according to the VID group membership.*/
        struct {
            GT_BIT                      isTrunk;    /* 1 - DA learned on trunk */
            GT_U32                      portNum;  /* port number on which MAC was learned */
            GT_U32                      trunkId;  /* trunk number on which MAC was learned */
        }ucast;
    }targed;

    GT_BIT                      multiTrg; /*  For Unicast MAC entries:
                                            0 - Forward to the Unicast target device/port or Trunk group.
                                            1 - Treat this entry as if it were a Multicast and forward according to the entry <VIDX> .
                                            This entry must be configured as static.*/

    GT_BIT                      daRoute;    /* Indicates that this MAC Address is associated with the Router MAC */

    GT_BIT                      appSpecCpuCode; /* If set, and the entry <DA Command> is MIRROR or TRAP, then the CPU code FDB_TRAP/MIRROR
                                               may be overwritten according to the Application Specific CPU Code assignment mechanism. */
    GT_U32                      entryIndex ; /* index of entry */
    GT_U32                      sstId;    /* SST-ID - 5-bit source-ID assignment for this source MAC entry. The source-ID assignment is used by the
                                            egress source-ID filtering mechanism. */
    GT_U32                      vid1; /* field from the DA entry
                                        valid from Lion2 - EArch
                                        */
}SNET_CHEETAH_L2I_DA_LOOKUP_INFO;


typedef struct {
        GT_BIT isValid;                      /* indication that the entry is valid (the FDB matched the DIP lookup) */
        GT_U32 nextHop_hwRouterPacketCommand;/* the packet command associated with the entry (in sip5 due to lack
                                                of bits in the FDB entry those 3 bits come from ‘global register’) */
        GT_U32 ipv6ScopeCheck;
        GT_U32 ipv6DestSiteId;
        GT_U32 decTtlOrHopCount;
        GT_U32 bypassTtlOptionsOrHopExtensionForIpv4Packets;
        GT_U32 ingressMirrorToAnalyzerIndex;
        GT_U32 qosProfileMarkingEn;
        GT_U32 qosProfileIndex;
        GT_U32 qosProfilePrecedence;
        GT_U32 modifyUp;
        GT_U32 modifyDscp;
        GT_U32 counterSetIndex;
        GT_U32 arpBcTrapMirrorEn;
        GT_U32 dipAccessLevel;
        GT_U32 ecmpRedirectExceptionMirror;
        GT_U32 mtuIndex;
        GT_U32 useVidx;
        GT_U32 trgIsTrunk;
        GT_U32 trgTrunkId;
        GT_U32 trgEport;
        GT_U32 evidx;
        GT_U32 trgDev;
        GT_U32 nextHopEvlan;
        GT_U32 startOfTunnel;
        GT_U32 tunnelType;
        GT_U32 tunnelPtrValid;
        GT_U32 arpPtrValid;
        GT_U32 tsIsNat;

}SNET_LION3_FDB_ROUTE_ENTRY_INFO;


typedef struct {
    SKERNEL_EXT_PACKET_CMD_ENT      bpduTrapCmd; /* Trap as BPDU */
    SKERNEL_EXT_PACKET_CMD_ENT      ieeePcktCmd; /* Command for IEEE reserved mcast*/
    SKERNEL_EXT_PACKET_CMD_ENT      ciscoPcktCmd; /* Command for MAC DA = 0x01-00-0C-xx-xx-xx running CISCO Layer 2 proprietary protocols.*/
    SKERNEL_EXT_PACKET_CMD_ENT      arpBcastCmd; /* Command for ARP broadcast.*/
    SKERNEL_EXT_PACKET_CMD_ENT      ripV1Cmd; /* Command for RIP v1 packet.*/
    SKERNEL_EXT_PACKET_CMD_ENT      igmpCmd; /* Command for IGMP packet.*/
    SKERNEL_EXT_PACKET_CMD_ENT      icmpV6Cmd; /* Command for ICMP v6 packet.*/
    SKERNEL_EXT_PACKET_CMD_ENT      solicitationMcastCmd; /* Command for Solicited-Node Multicast packet*/
    SKERNEL_EXT_PACKET_CMD_ENT      ipXMcLinkLocalProtCmd; /* Command for for IPv4: DIP[31:0] = 224:00:00:xx, for IPv6: DIP[127:0] = FF02:0:0:0:0:0:0:xx*/
    SKERNEL_EXT_PACKET_CMD_ENT      udpBcDestPortCmd; /* Cht2 - command for UDP bc destination port comparison */
}SNET_CHEETAH_L2I_CNTRL_PACKET_INFO;

typedef struct {
    GT_BIT                          autoLearnEn; /* 1 - auto learning is enabled , changed due to cht2 */
    GT_BIT                          naIsSecurityBreach; /* packet with unknown SA is security breach one */
    GT_BIT                          naStormPreventEn; /* NA messages Storm Prevention enabled  */
    GT_BIT                          naMsgToCpuEn; /* sending of NA messages to CPU enabled  */
    SKERNEL_EXT_PACKET_CMD_ENT      unknownSaCmd;        /* new address CMD - for control learning only */
    SKERNEL_EXT_PACKET_CMD_ENT      localPortCmd;   /* Bridging to local port command */
    SKERNEL_EXT_PACKET_CMD_ENT      invldVlanCmd;   /* Invalid VLAN command */
    SKERNEL_EXT_PACKET_CMD_ENT      inVlanFilterCmd;   /* Ingress VLAN filtering  command */
    SKERNEL_EXT_PACKET_CMD_ENT      vlanRangeCmd;   /* VLAN range  command */
    GT_BIT                          invalidSa; /* SA MAC is multicast  */
    SKERNEL_EXT_PACKET_CMD_ENT      invalidSaCmd; /* Invalid SA command  */
    SKERNEL_EXT_PACKET_CMD_ENT      stpCmd; /* ingress STP  command  */
    SKERNEL_EXT_PACKET_CMD_ENT      ipmNonIpmDaCmd; /* IPM - IPM DA command  */
    SKERNEL_EXT_PACKET_CMD_ENT      ipmIpmDaCmd;    /* IPM - NON IPM DA command  */
    GT_BIT                          mcMacIsIpmV4; /* DA MC MAC isIPM v4 multicast  */
    GT_BIT                          mcMacIsIpmV6; /* DA MC MAC isIPM v6 multicast  */
    SKERNEL_EXT_PACKET_CMD_ENT      staticMovedCmd; /* Static Moved CMD  */
    GT_U32                          portSstId; /*Source Span Tree ID for a Port */
    GT_BIT                          pvlanEn; /*1 = Private VLAN Edges is enabled on port */
    GT_BIT                          pvlanTrgIsTrunk; /* Private VLAN target is trunk */
    GT_U32                          pvlanPortOrTrunkId; /* Private VLAN target port or Trunk ID */
    GT_U32                          pvlanDevNum; /* Private VLAN target device number */
    GT_BIT                          macQosConflictMode; /* Resolves QoS conflicts between MAC DA and MAC SA QoS assignments.
                                                            QoS assignment conflict occurs when:
                                                            MAC SA and MAC DA are found in the FDB
                                                            and <DAQoSProfile Index> does not equal 0.
                                                            and <SAQoSProfile Index> does not equal 0.
                                                            0 = <SAQoSProfile Index> is used.
                                                            1 = <DAQoSProfile Index> is used.*/
    GT_BIT                          isSecurityBreach; /* A Security Breach event was done for a packet*/
    GT_U32                          dropCounterMode;   /* Bridge drop counter mode.
                                                          in SIP5 this mode is the 'CPU code' (SNET_CHEETAH_CPU_CODE_ENT)
                                                          that match the 'DROP reason'   */
    GT_U32                          sip5SecurityBreachDropMode;/*SIP5 : the security breach drop mode */
    SKERNEL_EXT_PACKET_CMD_ENT      rateLimitCmd; /* rate Limit command */
    /* cheetah2 only */
    SKERNEL_EXT_PACKET_CMD_ENT      frameTypeCmd; /* acceptable frame type */
    SKERNEL_EXT_PACKET_CMD_ENT      vlanMruCmd; /* MRU per VLAN */
    SKERNEL_EXT_PACKET_CMD_ENT      arpMacSaMismatchCmd;/* ARP MAC SA Mismatch */
    SKERNEL_EXT_PACKET_CMD_ENT      secAutoLearnCmd; /* secured automatic learning*/
    SKERNEL_EXT_PACKET_CMD_ENT      accessCmd;  /* bridge access matrix level */
    SKERNEL_EXT_PACKET_CMD_ENT      tcpOverMacMcCmd;  /* TCP over mac mc/bc */
    SKERNEL_EXT_PACKET_CMD_ENT      icmpFragCmd;       /* ICMP V4 fragmentation */
    SKERNEL_EXT_PACKET_CMD_ENT      tcpSynWithData;  /* 12.7.2.1 TCP SYN Packet with Data */
    SKERNEL_EXT_PACKET_CMD_ENT      tcpZeroFlagsCmd; /* 12.7.2.3 A TCP packet with all flags set to zero */
    SKERNEL_EXT_PACKET_CMD_ENT      tcpFinUrgPsgFlagsCmd; /* 12.7.2.4 A TCP flags with FIN-URG-PSH flags are set */
    SKERNEL_EXT_PACKET_CMD_ENT      tcpSynFinFlagsCmd; /* 12.7.2.5 A TCP Flags with SYN-FIN flags are set*/
    SKERNEL_EXT_PACKET_CMD_ENT      tcpSynRstFlagsCmd; /* 12.7.2.6 A TCP Flags with SYN-RST flags are set */
    SKERNEL_EXT_PACKET_CMD_ENT      tcpPortZeroFlagsCmd; /* 12.7.2.7 A TCP/UDP port is zero  */
    SKERNEL_EXT_PACKET_CMD_ENT      autoLearnNoRelearnCmd; /* 12.5.7.3 Auto-Learning with no Relearning */

    SKERNEL_EXT_PACKET_CMD_ENT      unknUcastCmd;           /* 12.13.2 Per-Egress port Unknown Unicast Filter */
    SKERNEL_EXT_PACKET_CMD_ENT      uregL2NonIpmMcCmd;      /* 12.13.1.2 Per-VLAN Unregistered Non-IPv4/6 Multicast Filtering */
    SKERNEL_EXT_PACKET_CMD_ENT      uregL2Ipv4McCmd;        /* 12.13.1.3 Per-VLAN Unregistered IPv4 Multicast Filtering */
    SKERNEL_EXT_PACKET_CMD_ENT      uregL2Ipv6McCmd;        /* 12.13.1.4 Per-VLAN Unregistered IPv6 Multicast Filtering */
    SKERNEL_EXT_PACKET_CMD_ENT      uregL2Ipv4BcCmd;        /* 12.13.1.5 Per-VLAN Unregistered IPv4 Broadcast Filtering */
    SKERNEL_EXT_PACKET_CMD_ENT      uregL2NonIpv4BcCmd;     /* 12.13.1.6 Per-VLAN Unregistered non-IPv4 Broadcast Filtering */
    SKERNEL_EXT_PACKET_CMD_ENT      sipIsDipCmd;            /*  Command for IPv4/IPv6 packets with SIP address equal to DIP address. */
    SKERNEL_EXT_PACKET_CMD_ENT      movedMacCmd; /* Secure Address Movement - Moved MAC CMD  */
    SKERNEL_EXT_PACKET_CMD_ENT      macSpoofProtectionCmd; /* Secure Address Movement - MAC Spoof Protection CMD  */
    GT_BIT                          isMacSpoofProtectionEvent; /* MAC Spoof Protection event happened */

    GT_U32                          securityBreachMode;   /* the highest priority security breach event */

    SKERNEL_EXT_PACKET_CMD_ENT      saEqualDaCmd;           /*  Command for Ethernet packets with source MAC address equal to
                                                                destination MAC address  */
}SNET_CHEETAH_L2I_FILTERS_INFO;

typedef struct{
    GT_BIT srcEPortIsGlobal;/* is the EPort 'global ePort' or 'local EPort'*/
    GT_U32 FDB_port_trunk;/*the Source port assigned to the FDB<ePort/Trunk> in learning*/
    GT_BIT FDB_isTrunk;/*is the value assigned to FDB<trunk> in learning*/
    GT_U32 FDB_devNum;/*the device assigned to the FDB in learning*/
    GT_U32 FDB_coreId;/*the coreId assigned to the FDB in learning*/
    GT_BIT FDB_isMoved;/* did the learn mac moved ? */
    GT_U32 FDB_fid;/* the FID to learn the entry with */
    GT_U32 FDB_UDB;/* the User Defined bits to learn the entry with - relevant to Auto learning only */
    GT_U32 FDB_srcId;/*the srcId from the FDB  - relevant to Auto learning only */

    GT_U32 FDB_origVid1;/* the <Orig Tag1 VID> to write to the FDB entry  - relevant to Auto learning only */

    struct{
        GT_U32 FDB_port_trunk;/*the <ePort/Trunk> from the FDB*/
        GT_BIT FDB_isTrunk;/*the <isTrunk> from the FDB*/
        GT_U32 FDB_devNum;/*the device from the FDB*/
        GT_U32 FDB_coreId;/*the coreId from the FDB*/
        GT_U32 FDB_srcId;/*the srcId from the FDB*/
    }oldInfo;  /* info that is relevant for 'FDB_isMoved' about the 'old FDB info'*/


}SNET_CHEETAH_L2I_LEARN_INFO_STC;

/* CPU codes */
typedef enum
{
    SNET_CHT_BPDU_TRAP = 2,
    SNET_CHT_MAC_TABLE_ENTRY_TRAP_MIRROR = 3,
    SNET_LION3_CPU_CODE_L2I_MAC_SA_MOVED = 4,/*sip5*/
    SNET_CHT_ARP_BCAST_TRAP_MIRROR = 5,
    SNET_CHT_IPV4_IGMP_TRAP_MIRROR = 6,
    SNET_LION3_CPU_CODE_L2I_SA_IS_DA = 7,/*sip5*/
    SNET_CHT_BRG_LEARN_DIS_UNK_SRC_MAC_ADDR_TRAP = 8,
    SNET_LION3_CPU_CODE_L2I_SIP_IS_DIP = 9,/*sip5*/
    SNET_CHT_BRG_LEARN_DIS_UNK_SRC_MAC_ADDR_MIRROR = 10,
    SNET_LION3_CPU_CODE_L2I_TCP_UDP_SPORT_IS_DPORT = 11,/*sip5*/
    SNET_LION3_CPU_CODE_L2I_TCP_FLAGS_WITH_FIN_WITHOUT_ACK = 12,/*sip5*/
    SNET_CHT_IEEE_RES_MCAST_ADDR_TRAP_MIRROR = 13,
    SNET_CHT_IPV6_ICMP_TRAP_MIRROR = 14,
    SNET_LION3_CPU_CODE_L2I_TCP_WITHOUT_FULL_HEADER = 15,/*sip5*/
    SNET_CHT_IPV4_IPV6_LINK_LOCAL_MCAST_DIP_TRAP_MIRROR = 16,
    SNET_CHT_IPV4_RIPV1_MIRROR = 17,
    SNET_CHT_IPV6_NEIGHBOR_SOLICITATION_TRAP_MIRROR = 18,
    SNET_CHT_IPV4_BCAST_TRAP_MIRROR = 19,
    SNET_CHT_NON_IPV4_BCAST_TRAP_MIRROR = 20,
    SNET_CHT_CISCO_CONTROL_MCAST_MAC_ADDR_TRAP_MIRROR = 21,
    SNET_CHT_BRG_NON_IPV4_IPV6_UNREG_MCAST_TRAP_MIRROR = 22,
    SNET_CHT_BRG_IPV4_UNREG_MCAST_TRAP_MIRROR = 23,
    SNET_CHT_BRG_IPV6_UNREG_MCAST_TRAP_MIRROR = 24,
    SNET_CHT_BRG_UNKN_UCAST_TRAP_MIRROR = 25,/* not documented value */
    SNET_CHT_IEEE_RES_MC_ADDR_TRAP_MIRROR_1 = 26, /* from cheetah 2 */
    SNET_CHT_IEEE_RES_MC_ADDR_TRAP_MIRROR_2 = 27, /* from cheetah 2 */
    SNET_CHT_IEEE_RES_MC_ADDR_TRAP_MIRROR_3 = 28, /* from cheetah 2 */
    SNET_CHT_IPV4_IPV6_LINK_LOCAL_MCAST_DIP_TRAP_MIRROR_1 = 29, /* from cheetah 2 */
    SNET_CHT_IPV4_IPV6_LINK_LOCAL_MCAST_DIP_TRAP_MIRROR_2 = 30, /* from cheetah 2 */
    SNET_CHT_IPV4_IPV6_LINK_LOCAL_MCAST_DIP_TRAP_MIRROR_3 = 31, /* from cheetah 2 */
    SNET_CHT_UDP_BC_TRAP_MIRROR0 = 32,
    SNET_CHT_UDP_BC_TRAP_MIRROR1 = 33,
    SNET_CHT_UDP_BC_TRAP_MIRROR2 = 34,
    SNET_CHT_UDP_BC_TRAP_MIRROR3 = 35,
    SNET_CHT_SEC_AUTO_LEARN_UNK_SRC_TRAP = 36,
    SNET_LION3_CPU_CODE_L2I_VLAN_NOT_VALID = 38,/*sip5*/
    SNET_LION3_CPU_CODE_L2I_PORT_NOT_VLAN_MEM = 39,/*sip5*/
    SNET_LION3_CPU_CODE_L2I_VLAN_RANGE = 40,/*sip5*/
    SNET_LION3_CPU_CODE_L2I_STATIC_ADDR_MOVED = 41,/*sip5*/
    SNET_LION3_CPU_CODE_L2I_MAC_SPOOF = 42,/*sip5*/
    SNET_LION3_CPU_CODE_L2I_ARP_SA_MISMATCH = 43,/*sip5*/
    SNET_LION3_CPU_CODE_L2I_TCP_SYN_WITH_DATA = 44,/*sip5*/
    SNET_LION3_CPU_CODE_L2I_TCP_OVER_MC_BC = 45,/*sip5*/
    SNET_LION3_CPU_CODE_L2I_FRAGMENT_ICMP = 46,/*sip5*/
    SNET_LION3_CPU_CODE_L2I_TCP_FLAGS_ZERO = 47,/*sip5*/
    SNET_LION3_CPU_CODE_L2I_TCP_FLAGS_FIN_URG_PSH = 48,/*sip5*/
    SNET_LION3_CPU_CODE_L2I_TCP_FLAGS_SYN_FIN = 49,/*sip5*/
    SNET_LION3_CPU_CODE_L2I_TCP_FLAGS_SYN_RST = 50,/*sip5*/
    SNET_LION3_CPU_CODE_L2I_TCP_UDP_SRC_DEST_ZERO = 51,/*sip5*/
    SNET_LION3_CPU_CODE_L2I_BRIDGE_ACCESS_MATRIX = 52,/*sip5*/
    SNET_LION3_CPU_CODE_L2I_ACCEPT_FRAME_TYPE = 53,/*sip5*/
    SNET_LION3_CPU_CODE_L2I_VLAN_MRU = 54,/*sip5*/
    SNET_LION3_CPU_CODE_L2I_RATE_LIMITING = 55,/*sip5*/
    SNET_LION3_CPU_CODE_L2I_LOCAL_PORT = 56,/*sip5*/
    SNET_LION3_CPU_CODE_L2I_IPMC = 57,/*sip5*/
    SNET_LION3_CPU_CODE_L2I_NON_IPMC = 58,/*sip5*/
    SNET_LION3_CPU_CODE_L2I_DSA_TAG_LOCAL_DEV = 59,/*sip5*/
    SNET_LION3_CPU_CODE_L2I_INVALID_SA = 60,/*sip5*/

    SNET_LION3_CPU_CODE_IPV4_6_MC_BIDIR_RPF_FAIL = 61, /*sip5*/

    SNET_CHT_ROUTED_PACKET_FORWARD = 64,
    SNET_CHT_BRIDGED_PACKET_FORWARD = 65,
    SNET_CHT_INGRESS_MIRRORED_TO_ANALYZER = 66,
    SNET_CHT_EGRESS_MIRRORED_TO_ANALYZER = 67,
    SNET_CHT_MAIL_FROM_NEIGHBOR_CPU = 68,
    SNET_CHT_CPU_TO_CPU = 69,
    SNET_CHT_EGRESS_SAMPLED = 70,
    SNET_CHT_INGRESS_SAMPLED = 71,
    SNET_CHT_IPV4_TT_HEADER_ERROR = 75,
    SNET_CHT_IPV4_TT_OPTION_FRAG_ERROR = 76,
    SNET_CHT_IPV4_TT_UNSUP_GRE_ERROR = 77,
    SNET_CHT_EQ_BAD_ANALYZER_INDEX_DROP_ERROR = 78,/*sip5*/
    SNET_XCAT_MPLS_TT_HEADER_CHECK_ERROR = 79, /* XCAT TT MPLS HEADER CHECK ERROR */
    SNET_XCAT_MPLS_TT_LSR_TTL_EXCEEDED = 80,   /* XCAT TT MPLS LSR TTL EXCEEDED */
    SNET_XCAT_OAM_PDU_TRAP = 83,               /* from XCAT A1 */
    SNET_CHT_IPV4_UC_TTL_EXCEEDED = 133,
    SNET_CHT_IPV4_6_MTU_EXCEEDED = 134,
    SNET_CHT_IPV6_UC_HOP_LIMIT_EXCEEDED = 135,
    SNET_CHT_IPV4_6_ILLEGAL_ADDR_ERROR = 136,
    SNET_CHT_IPV4_HEADER_ERROR = 137,
    SNET_CHT_IPV4_6_DIP_DA_MISMATCH = 138,
    SNET_CHT_IPV6_HEADER_ERROR = 139,
    SNET_CHT_IPV4_6_SIP_SA_MISMATCH = 140,
    SNET_CHT_IPV4_UC_OPTIONS = 141,
    SNET_CHT_IPV6_NON_HBH_OPTIONS = 142,
    SNET_CHT_IPV6_UC_HOP_BY_HOP = 143,
    SNET_CHT_IPV4_6_SIP_FILTERING = 144,/*sip5*/
    SNET_CHT_IPV4_6_SIP_IS_ZERO = 145,/*sip5*/
    SNET_CHT_ACCESS_MATRIX = 146,     /*sip5*/
    SNET_CHT_FCOE_EXCEPTION = 147,    /*sip5*/ SNET_CHT_FCOE_DIP_LOOKUP_NOT_FOUND = 147,/*sip5*//* same value as SNET_CHT_FCOE_EXCEPTION ! */
    SNET_CHT_FCOE_SIP_LOOKUP_NOT_FOUND = 148,/*sip5*/
    /* 149 .. 158 :
        SIP5 : user defined
        legacy : reserved

    */

    SNET_CHT_IPV6_SCOPE = 159,
    SNET_CHT_IPV4_UC_ROUTE0 = 160,
    SNET_CHT_IPV4_UC_ROUTE1,
    SNET_CHT_IPV4_UC_ROUTE2,
    SNET_CHT_IPV4_UC_ROUTE3,
    SNET_CHT_IPV4_MC_ROUTE0 = 164,
    SNET_CHT_IPV4_MC_ROUTE1,
    SNET_CHT_IPV4_MC_ROUTE2,
    SNET_CHT_IPV4_MC_ROUTE3,
    SNET_CHT_IPV6_UC_ROUTE0 = 168,
    SNET_CHT_IPV6_UC_ROUTE1,
    SNET_CHT_IPV6_UC_ROUTE2,
    SNET_CHT_IPV6_UC_ROUTE3,
    SNET_CHT_IPV6_MC_ROUTE0 = 172,
    SNET_CHT_IPV6_MC_ROUTE1,
    SNET_CHT_IPV6_MC_ROUTE2,
    SNET_CHT_IPV6_MC_ROUTE3,
    SNET_CHT_IPV4_6_UC_RPF_FAIL = 176,
    SNET_CHT_IPV4_6_MC_RPF_FAIL = 177,
    SNET_CHT_IPV4_6_MC_MLL_RPF_FAIL = 178,
    SNET_CHT_ARP_BC_TO_ME          = 179,
    SNET_CHT_IPV4_UC_ICMP_REDIRECT = 180,
    SNET_CHT_IPV6_UC_ICMP_REDIRECT = 181,
    SNET_CHT_IPV6_TT_HEADER_ERROR = 182,            /* TBD TTI CAPWAP IpV6 header error */
    SNET_CHT_CAPWAP_802_11_MGMT_EXCEPTION = 183,    /* TBD TTI CAPWAP 802.11 management */
    SNET_CHT_IPV6_TT_OPTION_FRAG_ERROR = 184,       /* TBD TTI CAPWAP IpV6 fragment */
    SNET_CHT_CAPWAP_FRAGMENT_EXCEPTION = 185,       /* TBD TTI CAPWAP fragment */
    SNET_CHT_ARP_REPLY_TO_ME = 188,
    SNET_CHT_CPU_TO_ALL_CPUS = 189,
    SNET_CHT_TCP_SYN_TO_CPU = 190,
    SNET_CHT_PACKET_TO_VIRTUAL_ROUTER_PORT = 191,

    /*
        192 - 255 ---> user defined range
    */
    SNET_CHT_PACKET_USER_DEFINED_FIRST = 192,
    SNET_CHT_PACKET_USER_DEFINED_LAST  = 255


}SNET_CHEETAH_CPU_CODE_ENT;


/* engine units */
typedef enum {
    /* ingress units */
    SNET_CHEETAH_ENGINE_UNIT_TTI_E,
    SNET_CHEETAH_ENGINE_UNIT_PCL_E,
    SNET_CHEETAH_ENGINE_UNIT_L2I_E,
    SNET_CHEETAH_ENGINE_UNIT_ROUTER_E,
    SNET_CHEETAH_ENGINE_UNIT_OAM_E,
    SNET_CHEETAH_ENGINE_UNIT_POLICER_E,
    SNET_CHEETAH_ENGINE_UNIT_L2MLL_E,
    SNET_CHEETAH_ENGINE_UNIT_EQ_E,
    /* egress units */
    SNET_CHEETAH_ENGINE_UNIT_TXQ_E,
    SNET_CHEETAH_ENGINE_UNIT_HA_E,
    SNET_CHEETAH_ENGINE_UNIT_EPCL_E
}SNET_CHEETAH_ENGINE_UNIT_ENT;


/*******************************************************************************
* snetL2iTablesFormatInit
*
* DESCRIPTION:
*        init the format of L2i tables.
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
void snetL2iTablesFormatInit(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr
);

/*******************************************************************************
*   snetChetahDoInterrupt
*
* DESCRIPTION:
*       Set cheetah interrupt
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       causeRegAddr        - Interrupt Cause Register
*       causeMaskRegAddr    - Interrupt Cause Register Mask
*       causeBitNum         - Interrupt Cause Register Bit
*       globalBitNum        - Global Interrupt Cause Register Bit
*
*
*******************************************************************************/
GT_VOID snetChetahDoInterrupt
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 causeRegAddr,
    IN GT_U32 causeMaskRegAddr,
    IN GT_U32 causeBitBmp,
    IN GT_U32 globalBitBmp
);

/*******************************************************************************
*   snetChetahDoInterruptFromTree
*
* DESCRIPTION:
*       Set cheetah interrupt according to the 'tree' of the interrupts
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       causeRegAddr        - Interrupt Cause Register
*       causeBitNum         - Interrupt Cause Register Bit
*       setInterrupt        - set the interrupt (or not)
*
*   RETURNS:
*       GT_TRUE - register was found and treated
*       GT_FALSE - register was NOT found and therefore not treated
*
*******************************************************************************/
GT_BOOL snetChetahDoInterruptFromTree
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 causeRegAddr,
    IN GT_U32 causeBitBmp
);

/*******************************************************************************
*   snetChetahDoInterruptLimited
*
* DESCRIPTION:
*       Set cheetah interrupt
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       causeRegAddr        - Interrupt Cause Register
*       causeMaskRegAddr    - Interrupt Cause Register Mask
*       causeBitNum         - Interrupt Cause Register Bit
*       globalBitNum        - Global Interrupt Cause Register Bit
*       setInterrupt        - set the interrupt (or not)
*
*******************************************************************************/
GT_VOID snetChetahDoInterruptLimited
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 causeRegAddr,
    IN GT_U32 causeMaskRegAddr,
    IN GT_U32 causeBitBmp,
    IN GT_U32 globalBitBmp,
    IN GT_U32 setInterrupt
);

/*******************************************************************************
*   snetChetahReadCauseInterruptFromTree
*
* DESCRIPTION:
*       Read may causing 'clear' of my father's cause register
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       causeRegAddr        - Interrupt Cause Register
*       bmpOfBitsToClear    - bmp of bits to clear in the register
*   RETURNS:
*       GT_TRUE - register was found and treated
*       GT_FALSE - register was NOT found and therefore not treated
*
*******************************************************************************/
GT_BOOL snetChetahReadCauseInterruptFromTree
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 causeRegAddr,
    IN GT_U32 bmpOfBitsToClear
);

/*******************************************************************************
*   lookForInterruptBasicInfoByCauseReg
*
* DESCRIPTION:
*       look for match in the DB for cause register.
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       causeRegAddr        - Interrupt Cause Register
*       currDbPtr           - the place to check
*
*   RETURNS:
*       GT_TRUE  - register was found
*       GT_FALSE - register was NOT found
*
*******************************************************************************/
GT_BOOL lookForInterruptBasicInfoByCauseReg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 causeRegAddr,
    IN SKERNEL_INTERRUPT_REG_INFO_STC  *currDbPtr
);

/*******************************************************************************
*   doInterruptsSubTree
*
* DESCRIPTION:
*       handle a sub tree in the interrupts DB.
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       currDbPtr           - the place to check
*       causeBitBmp         - bmp of the bits that need to be set/clear in the cause register
*       isReadAction        - GT_TRUE  - read action by the CPU (the device will clear interrupts if needed)
*                             GT_FALSE - the device sets interrupts if needed
*   OUTPUT:
*
*   RETURNS:
*       None
*
*******************************************************************************/
void doInterruptsSubTree
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_INTERRUPT_REG_INFO_STC  *currDbPtr,
    IN GT_U32 causeBitBmp,
    IN GT_BOOL  isReadAction
);

/*******************************************************************************
*   getInterruptCauseRegAddr
*
* DESCRIPTION:
*       get the cause register address from DB of interrupts.
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       currDbPtr           - the entry in the DB
*
*   RETURNS:
*       regAddr
*
*******************************************************************************/
GT_U32 getInterruptCauseRegAddr
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_INTERRUPT_REG_INFO_STC  *currDbPtr
);

/*******************************************************************************
*   getInterruptMaskRegAddr
*
* DESCRIPTION:
*       get the mask register address from DB of interrupts.
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       currDbPtr           - the entry in the DB
*
*   RETURNS:
*       regAddr
*
*******************************************************************************/
GT_U32 getInterruptMaskRegAddr
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_INTERRUPT_REG_INFO_STC  *currDbPtr
);
/*******************************************************************************
*   snetChtL2iSendUpdMsg2Cpu
*
* DESCRIPTION:
*       Send MAC Update message to CPU
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       macUpdateMsgPtr     - MAC update message
*
*
*******************************************************************************/
GT_BOOL snetChtL2iSendUpdMsg2Cpu
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 * macUpdateMsgPtr
);

/*******************************************************************************
*   snetChtL2iPciFuMsg
*
* DESCRIPTION:
*        PCI Master write to host memory FDB Upload Queue (FUQ)
* INPUTS:
*        devObjPtr          -  pointer to device object.
*        fuMsgSizePtr       -  pointer to FDB/CNC Update Message
*
*******************************************************************************/
GT_BOOL snetChtL2iPciFuMsg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 * fuMsgPtr,
    IN GT_U32   numWordsData
);

/*******************************************************************************
*   snetCht2L2iSendFuMsg2Cpu
*
* DESCRIPTION:
*       Send FDB Upload message to CPU
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       macFuMsgPtr         - FDB Upload message
*
*
*******************************************************************************/
GT_BOOL snetCht2L2iSendFuMsg2Cpu
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 * macFuMsgPtr
);

/*******************************************************************************
*   snetChtL2iMacEntryAddress
*
* DESCRIPTION:
*       Perform FDB Lookup and if success return FDB entry address
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       entryInfoPtr    - (pointer to) entry hash info
*
* OUTPUT:
*       entryIndexPtr   - index for the FDB found or free entry
*       entryOffsetPtr  - FDB offset within the 'bucket'
*
* RETURN:
*       GT_OK           - FDB entry was found
*       GT_NOT_FOUND    - FDB entry was not found and assigned with new address
*       GT_FAIL         - FDB entry was not found and no free address space
*
*******************************************************************************/
GT_STATUS snetChtL2iMacEntryAddress
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SNET_CHEETAH_FDB_ENTRY_HASH_INFO_STC *entryInfoPtr,
    OUT GT_U32 * fdbEntryAddrPtr,
    OUT GT_U32 * entryOffsetPtr
);

/*******************************************************************************
* snetChtL2iPortCurrentTimeWindowGet
*
* DESCRIPTION:
*       Gets current time window.
*
* APPLICABLE DEVICES:  All DxCh Devices.
*
*
* INPUTS:
*       devObjPtr - pointer to device object
*       localSrcPort - rate limiting ingress port
*
* OUTPUTS:
*       currentTimeWindowPtr - current time window
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void snetChtL2iPortCurrentTimeWindowGet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 localSrcPort,
    OUT GT_U32 * currentTimeWindowPtr
);

/*******************************************************************************
*   fdbEntryHashInfoGet
*
* DESCRIPTION:
*       get the FDB entry hash info .
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       entryType    - entry type
*       fid          - FID
*       macAddr      - mac address
*       sip          - sip
*       dip          - dip
* OUTPUTS:
*       entryInfoPtr - (pointer to) entry hash info
*
* RETURN:
*       GT_OK
*       GT_FAIL
*
* COMMENTS:
*
*
*******************************************************************************/
GT_STATUS fdbEntryHashInfoGet
(
    IN SKERNEL_DEVICE_OBJECT                    *devObjPtr,
    IN SNET_CHEETAH_FDB_ENTRY_ENT                entryType,
    IN GT_U16                                    fid,
    IN GT_U8                                     macAddr[],
    IN GT_U32                                    sip,
    IN GT_U32                                    dip,
    OUT SNET_CHEETAH_FDB_ENTRY_HASH_INFO_STC    *entryInfoPtr
);

/*******************************************************************************
*   snetChtL2iFdbLookup
*
* DESCRIPTION:
*       find in the FDB 2 places for the needed entry info
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       entryInfoPtr    - (pointer to) entry hash info
* OUTPUT:
*       numValidPlacesPtr  - (pointer to) indication of how many indexes valid in
*                           entryIndexPtr[]
*                            the value it 0 or 1 or 2
*       entryIndexPtr[2]   - FDB entry index 0,1
*
* RETURN:
*       None
*
*******************************************************************************/
void snetChtL2iFdbLookupFor2Places
(
    IN  SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SNET_CHEETAH_FDB_ENTRY_HASH_INFO_STC *entryInfoPtr,
    IN GT_U32   *numValidPlacesPtr,
    OUT GT_U32   entryIndexPtr[2]
);

/*******************************************************************************
*   snetChtL2iFdbDipLookUp
*
* DESCRIPTION:
*       Performs FDB Routing lookup
*
* INPUTS:
*       devObjPtr        - pointer to device object.
*       descrPtr         - pointer to the frame's descriptor.
*
* OUTPUTS:
*       descrPtr         - pointer to the frame's descriptor.
*       dipLookupInfoPtr - (pointer to) dip lookup info
*
*******************************************************************************/
GT_VOID snetChtL2iFdbDipLookUp
(
    IN    SKERNEL_DEVICE_OBJECT           *devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC *descrPtr,
    OUT   SNET_LION3_FDB_ROUTE_ENTRY_INFO *dipLookupInfoPtr
);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __snetCheetahL2h */


