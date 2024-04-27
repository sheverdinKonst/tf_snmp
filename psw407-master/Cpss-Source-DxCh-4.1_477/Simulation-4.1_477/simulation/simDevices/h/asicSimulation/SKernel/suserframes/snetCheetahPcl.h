
/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetCheetahPcl.h
*
* DESCRIPTION:
*       (Cheetah) Policy Engine processing for ingress frame -- simulation
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 5 $
*
*******************************************************************************/
#ifndef __snetCheetahPclh
#define __snetCheetahPclh

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* no match indicator during policy key matching checking */
#define SNET_CHT_POLICY_NO_MATCH_INDEX_CNS          (0xffffffff)

/*
    structure : SNET_CHT_POLICY_SPECIFIC_LOOKUP_CONFIG_STC
    (Table 350: PCL-ID Table entry<n> (0<=n<1152) --- page 484)

    purpose: info about the specific policy lookup configuration info

lookUpKeySize - Key Size for LookUp1
0 = Short key of 24 Bytes.
1 = Long key of 48 Bytes.

lookUpPclId - PCL-ID for LookUp

lookUpIpKeyType - Key Type for IP Packets
When <LookUp1 IPKeyType>= 0 (short keys), the 24 bytes L2+nonIP key is
used for non-IP packets and the 24 bytes L2+IPv4/IPv6 key is used for IPv6
packets.
The key for IPv4 packets is selected as follows:
0 = IP packets use the 24 bytes L2+IPv4/IPv6 key.
1 = IP packets use the 24 bytes IPv4+L4 key.
When <LookUp1 KeySize>= 1 (long keys), the 48 bytes L2+IPv4+L4 key is
used for non-IP packets and IPv4 packets.
The key for IPv6 packets is selected as follows:
0 = IP packets use the 48 bytes L2+IPv6 key.
1 = IP packets use the 48 bytes L4+IPv6 key.

lookUpEn - Enables Policy TCAM Lookup
0 = Search 0 is disabled.
1 = Search 0 is enabled.

Lookup1IPv6ShortKeyType - Determines the short Key Type used in the 2nd lookup
for IPv6 packets:
0 = IPv6 packets use the 24-bytes L2+IPv4/IPv6 QoS IPv6 key
1 = IPv6 packets use the 24-bytes IPv6 DIP key
*/
typedef struct{
    GT_BOOL enabled;
    GT_U32  keySize;
    GT_U32  pclId;
    GT_BIT  ipKeyType;
    GT_BIT  lookUp1IpV6KeyType;
}SNET_CHT_POLICY_SPECIFIC_LOOKUP_CONFIG_STC;

/*
    structure : SNET_CHT_POLICY_SPECIFIC_LOOKUP_CONFIG_STC
    (Table 350: PCL-ID Table entry<n> (0<=n<1152) --- page 484)

    purpose: info about the 2 policy lookup configuration info

valid - Entry Valid bit
0 = This PCL-ID entry is not valid.
1 = This PCL-ID entry is valid.

lkupConf_0 - lookup configuration info about lookup 0
lkupConf_0 - lookup configuration info about lookup 1
*/
typedef struct{
    GT_BOOL valid;
    SNET_CHT_POLICY_SPECIFIC_LOOKUP_CONFIG_STC lkupConf_0;
    SNET_CHT_POLICY_SPECIFIC_LOOKUP_CONFIG_STC lkupConf_1;
}SNET_CHT_POLICY_LOOKUP_CONFIG_STC;

/*
    structure : SNET_CHT_POLICY_EGRESS_INTERFACE_STC

    purpose: info about the egress interface

This is the egress interface, bits 14:2 when <PCERedirect Cmd> =1
. Bit 14: use_vidx - when use_vidx = 0 The packet is forwarded as a unicast
to a port or a trunk.
use_vidx = 1 The packet is forwarded as a multicast packet to a multi-
cast group.
. Bits 13:2 when use_vidx = 1
- VIDX - the packet.s multicast group
. Bits 13:2 when use_vidx = 0
- Bit 2: TargetIsTrunk- when TargetIsTrunk = 0 The packet is for-
warded to a port.
When TargetIsTrunk = 1 The packet is forwarded to a trunk.
- Bits 13:3 when TargetIsTrunk = 0
Bits 13:9 Target Device
Bits 8:3 Target Port
- Bits 13:3 when TargetIsTrunk = 1
Bits 13:10 reserved - set to 0
Bits 9:3 TargetTrunk

*/
typedef struct{
    GT_BOOL useVidx;
    union{
        GT_U16  vidx;

        struct{
            GT_BOOL targetIsTrunk;

            union{
                GT_U16  targetTrunk;

                struct{
                    GT_U8   targetDev;
                    GT_U8   targetPort;
                }noTrunk;
            }trunkInfo;
        }noVidx;
    }vidxInfo;
}SNET_CHT_POLICY_EGRESS_INTERFACE_STC;


/*
    structure : SNET_CHT_POLICY_ACTION_STC

    (Table 337: Policy Action Entry<n> (0<=n<1024) -- page 469)

    purpose: info about the action to apply on frame

matchCounterIndex - Valid if <Match CounterEn> is set to 1.
A pointer to one of the 32 policy rules match counters,
Policy Rule Match Counter<n> (0<=n<32).
The counter is incremented for every packet satisfying both of the following
conditions:
. Matching this rule.
. The previous packet command is not hard drop.

matchCounterEn - Enables the binding of this policy action entry to the Policy Rule Match
Counter<n> (0<=n<32) indexed by <Match CounterIndex>
0 = Match counter binding is disabled.
1 = Match counter binding is enabled.

pceModifyUp - Enables the modification of the packet.s 802.1p User Priority field.
Only relevant if QoS precedence of the previous QoS assignment mechanisms
(Port, Protocol Based QoS, and previous matching rule) is soft.
Applies to packets that are transmitted as VLAN tagged or transmitted via
Cascading ports with a Marvell tag in a FORWARD format.
When this field is set to 1 and the subsequent QoS assignment engines
have not changed this setting to enable the packet.s 802.1p User Priority
modification, the packet.s 802.1p User Priority field is set to UP extracted
from the QosProfile to QoS Table, when the packet is transmitted. This is
regardless of the incoming packet tag format (tagged or untagged).
0 = Keep previous ModifyUP setting.
1 = Set modify UP to 1.
2 = Set modify UP to 0.
3 = Reserved.

pceModifyDscp - Enables the modification of the packet.s DSCP field.
Only relevant if QoS precedence of the previous QoS assignment mechanisms
(Port, Protocol Based QoS, and previous matching rule) is soft.
Relevant for IPv4/IPv6 packets, only.
When this field is set to 1 and the subsequent QoS assignment engines
have not changed this setting to enable the packet.s DSCP modification,
the packet.s DSCP field is modified, when the packet is transmitted to the
DSCP extracted from the QosProfile to QoS Table.
0 = Keep previous DSCP modification command.
1 = Enable modification of the DSCP field in the packet.
2 = Disable modification of the DSCP field in the packet.
3 = Reserved.

pceQosProfile - Only relevant if the QoS precedence of the previous QoS Assignment
Mechanisms (Port, Protocol Based QoS, and previous matching rule) is soft
and <PCEQoS Profile MarkingEn> is set to 1.
If this is the QoSProfile that is assigned to the packet at the end of the
ingress pipe, <PCEQoSProfile> is used to index the QosProfile to QoS
Table and extract the QoS Parameters for the packet, which are TC, DP, UP
and DSCP

pceQosProfileMarkingEn
Relevant only if QoS precedence of the previous QoS Assignment Mechanisms
(Port, Protocol Based QoS, and previous matching rule) is soft.
0 = Preserve previous QoSProfile setting.
1 = Assign <PCEQoSProfile> to the packet.

pceQosPrecedence - (one of SKERNEL_QOS_PROF_PRECED_ENT)
PCE Marking of the QoSProfile Precedence.
Only relevant only if QoS precedence of the previous QoS Assignment
Mechanisms (Port, Protocol Based QoS, and previous matching rule) is
soft.
Setting this bit to 1, locks the QoS parameters setting from being modified
by subsequent QoS assignment engines in the ingress pipe.
0 = QoS precedence is soft and the packet.s QoS parameters may be over-
ride by subsequent QoS assignment engines
1 = QoS precedence is hard and the packet.s QoS parameters setting done
until this stage is locked. It cannot be overridden by subsequent QoS
assignment engines.
NOTE: The QoS precedence is not relevant in the policing engine. Non-
conforming traffic is re-marked according to the policing engine set-
ting regardless of their QoS precedence.

mirrorToIngressAnalyzerPort -
Enables the mirroring of the packet to the ingress analyzer port.
0 = Packet is not mirrored to ingress analyzer port.
1 = Packet is mirrored to ingress analyzer port configured in Analyzer Port
Configuration Register

pceCpuCode - Only relevant if <FwdCmd> is Mirror-to-CPU or Trap.
If at the end of the ingress pipe, this entry <FwdCmd> is assigned to the
packet. The packet that is forwarded to the CPU contains <PCE_
CPU_CODE> in its TO_CPU Marvell tag.

fwdCmd -  (one of SKERNEL_EXT_PACKET_CMD_ENT)
The entry forwarding command.
0 = Forward
1 = Mirror-to-CPU.
2 = Trap
3 = Hard Drop
4 = Soft Drop
5-7 = Reserved

pceVid - Only relevant if the VID precedence, set by the previous VID assignment
mechanisms (Port, Protocol Based VLANs, and previous matching rule), is
soft and <PCE VLANCmd>= 0.
The VLAN-ID set by this entry

pceVlanCmd - Relevant only if VID precedence set by the previous VID assignment
mechanisms (Port, Protocol Based VLANs, and previous matching rule) is soft.
0 = Don.t modify the VID assigned to the packet so far.
1 = <PCE_VID> is assigned to untagged packets or priority tagged packets.
2 = <PCE_VID> is assigned to tagged packets.
3 = <PCE_VID> is assigned to all packets.

enNestedVlan - When this field is set to 1, this rule matching flow is defined
as an access flow. The VID of all packets received on this flow is discarded and
they are assigned with a VID using the device.s VID assignment algorithms, as if
they are untagged.
When a packet received on an access flow is transmitted via a core port or
a Cascading port, a VLAN tag is added to the packet (on top of the existing
VLAN tag, if any). The VID field is the VID assigned to the packet as a result
of all VLAN assignment algorithms.
The 801.2p User Priority field of this added tag may be one of the following,
depending on the ModifyUP QoS parameter set to the packet at the end of
the Ingress pipe:
. If ModifyUP is 1, it is the UP extracted from the QosProfile to QoS
Table.
. If ModifyUP is 0, it is the original packet 802.1p User Priority field if it is
tagged and is <PUP> if the original packet is untagged.

pceVidPrecedence - The VLAN Assignment precedence for the subsequent VLAN assignment
mechanism, which is the policy engine next rule.
Only relevant if the VID precedence set by the previous VID assignment
mechanisms (Port, Protocol Based VLANs, and previous matching rule) is
soft.
0 = Soft precedence: The VID assignment can be overridden by the subsequent
VLAN assignment mechanism, which is the policy engine.
1 = Hard precedence: The VID assignment is locked to the last VLAN
assigned to the packet and cannot be overridden.

pceEgressInterface - see SNET_CHT_POLICY_EGRESS_INTERFACE_STC

pceRedirectCmd - This field enables the redirection of the matching flow packets
to an egress interface (a port, trunk, or a multicast group).
0 = Do not redirect.
1 = Redirect the packet to the Egress interface specified in
<PCE Egress Interface>.
<PCE Egress Interface>.2-3 = Reserved.
NOTE: Policy flow redirection bypasses the bridge engine.

policerIndex - Only valid if <PolicerEn> =1.

policerEn -
When set to 1, this rule is bonded to the policer indexed by <PolicerIndex>.
0 = Don.t use policer.
1 = Use policer.

*/
typedef struct{
    GT_U32      matchCounterIndex;
    GT_BOOL     matchCounterEn;
    GT_U32      pceModifyUp;
    GT_U32      pceModifyDscp;
    GT_U32      pceQosProfile;
    GT_BOOL     pceQosProfileMarkingEn;
    SKERNEL_PRECEDENCE_ORDER_ENT pceQosPrecedence;
    GT_BOOL     mirrorToIngressAnalyzerPort;
    GT_U32      pceCpuCode;
    SKERNEL_EXT_PACKET_CMD_ENT fwdCmd;
    GT_U32      pceVid;
    GT_U32      pceVlanCmd;
    GT_BOOL     enNestedVlan;
    GT_U32      pceVidPrecedence;
    SNET_CHT_POLICY_EGRESS_INTERFACE_STC pceEgressInterface;
    GT_U32      pceRedirectCmd;
    GT_U32      policerIndex;
    GT_BOOL     policerEn;
}SNET_CHT_POLICY_ACTION_STC;

/* Next Hop Policy Action Entry */
typedef struct{
    GT_U32      matchCounterIndex;
    GT_BOOL     matchCounterEn;
    GT_U32      routModifyUp;
    GT_U32      routModifyDscp;
    GT_U32      routQosProfile;
    GT_BOOL     routQosProfileMarkingEn;
    SKERNEL_PRECEDENCE_ORDER_ENT routQosPrecedence;
    GT_BOOL     mirrorToIngressAnalyzerPort;
    GT_U32      arpDaIndex;
    SKERNEL_EXT_PACKET_CMD_ENT fwdCmd;
    GT_U32      vid;
    SNET_CHT_POLICY_EGRESS_INTERFACE_STC routEgressInterface;
    GT_U32      routRedirectCmd;
    GT_BOOL     decTtl;
    GT_BOOL     bypassTtlOptionCheck;
    GT_BOOL     icmpRedirectChk;
}SNET_CHT_NEXT_HOP_POLICY_ACTION_STC;

/*
    Struct : SNET_CHT_PCL_KEY_SIZE

    description : 0 - standard key ,
                  1 - extended key
*/
typedef enum{
    SNET_CHT_PCL_STANDARD_KEY = 0 ,
    SNET_CHT_PCL_EXTENDED_KEY
}SNET_CHT_PCL_KEY_SIZE;

/*
    Struct : CHT_PCL_USER_DEFINED_CONFIG_REG_STC

    description :   hold info about the user defined fields in the registers
    regAddress     - register offset address from address 0x0B800020
    startBitOffset - starting bit of the Offset field (length 7 bits)
    startBitAnchor - starting bit of the Anchor field (length 2 bits)
*/
typedef struct{
    GT_U32  regOffsetAddress;  /* offset from 0x0B800020 in words !!! */
    GT_U32  startBitOffset;
    GT_U32  startBitAnchor;
}CHT_PCL_USER_DEFINED_CONFIG_REG_STC;

/*******************************************************************************
*   snetChtPclUdbKeyValueGet
*
* DESCRIPTION:
*        Get user defined value by user defined key.
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       descrPtr            - pointer to frame data buffer Id.
*       byteKeyArr          - user defined key array (byte offset/anchor).
*       byteIdx             - byte index in user defined key array.
*
* OUTPUTS:
*       byteValuePtr        - pointer to UDB value.
*
* RETURN:
*
* COMMENTS:
*
*******************************************************************************/
extern GT_U32 snetChtPclUdbKeyValueGet
(
    IN SKERNEL_DEVICE_OBJECT                        * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC              * descrPtr,
    IN CHT_PCL_USER_DEFINED_CONFIG_REG_STC          * byteKeyArr,
    IN GT_U32                                         byteIdx,
    OUT GT_U8                                       * byteValuePtr
);

/*******************************************************************************
*   snetChtPcl
*
* DESCRIPTION:
*        (Cheetah) Ingress Policy Engine processing for frame
*        PCL processing , Pcl assignment ,key forming , 2 Lookups ,
*        actions to descriptor processing
*
* INPUTS:
*       devObjPtr    - (pointer to) device object.
*       descrPtr     - (pointer to) frame data buffer Id
* OUTPUTS:
*       descrPtr     - (pointer to) updated frame data buffer Id
*
* RETURN:
*
*******************************************************************************/
extern GT_VOID snetChtPcl
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __snetCheetahPclh */


