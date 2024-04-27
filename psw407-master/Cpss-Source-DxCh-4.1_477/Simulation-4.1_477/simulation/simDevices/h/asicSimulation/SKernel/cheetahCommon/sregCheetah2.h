/******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* sregCheetah2.h
*
* DESCRIPTION:
*       Defines for Cheetah2 (98dx255,98dx265,98dx275) memory registers access.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 38 $
*
*******************************************************************************/
#ifndef __sregcheetah2h
#define __sregcheetah2h

#include <asicSimulation/SKernel/cheetahCommon/sregXCat.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* user defined bytes address for short L2 pcl search key*/
#define SMEM_CHT2_KEY3_USERDEFINED_BYTES_SHORT_L2_KEY_REG(dev)\
    (0x0B800024)

/* TCP port range comparator register - ingress policy */
#define SMEM_CHT2_PCL_TCP_PORT_RANGE_COMPARE_REG(dev)\
    (0x0B800600)

/* UDP port range comparator register - ingress policy */
#define SMEM_CHT2_PCL_UDP_PORT_RANGE_COMPARE_REG(dev)\
    (0x0B800640)

/* TCP port range comparator register - egress pcl */
#define SMEM_CHT2_EPCL_TCP_PORT_RANGE_COMPARE_REG(dev)\
(SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->HA.egrTCPPortRangeComparatorConfig[0] : \
((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->EPCL.egrTCPPortRangeComparatorConfig[0]   : \
    SKERNEL_DEVICE_FAMILY_XCAT_ONLY(dev)    ? 0x0F800100 :                                               \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev)     ? 0x0E000100 :                                               \
    0x07C00100)/*ch2,3*/)


/* UDP port range comparator register - egress pcl */
#define SMEM_CHT2_EPCL_UDP_PORT_RANGE_COMPARE_REG(dev)\
(SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->HA.egrUDPPortRangeComparatorConfig[0] : \
((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->EPCL.egrUDPPortRangeComparatorConfig[0]   : \
SMEM_CHT2_EPCL_TCP_PORT_RANGE_COMPARE_REG(dev) + 0x40))

/* user defined bytes address for short l2-l3 pcl search key*/
#define SMEM_CHT2_KEY0_USERDEFINED_BYTES_SHORT_L2_L3_KEY_REG(dev)\
((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->IPCL.lookupKeyUserDefinedBytes.key3AndKey4UserDefinedBytesConfig   : \
0x0B800038)

/* user defined bytes address for long l2-l3-l4 pcl search key*/
#define SMEM_CHT2_KEY5_0_USERDEFINED_BYTES_LONG_L3_L4_KEY_REG(dev)  \
    SMEM_CHT2_KEY3_USERDEFINED_BYTES_SHORT_L2_KEY_REG(dev)

/* user defined bytes address for long l2-l3-l4 pcl search key*/
#define SMEM_CHT2_KEY5_1_USERDEFINED_BYTES_LONG_L3_L4_KEY_REG(dev)\
((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->IPCL.lookupKeyUserDefinedBytes.key0UserDefinedBytesConfigReg0   : \
0x0B800020)

/* user defined bytes address for standard l3-l4 pcl search key*/
#define SMEM_CHT2_KEY5_USERDEFINED_BYTES_SHORT_L3_L4_KEY_REG(dev)\
((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->IPCL.lookupKeyUserDefinedBytes.key5UserDefinedBytesConfig   : \
0x0B80003C)

/* user defined bytes address for extended l2-ipv6 pcl search key*/
#define SMEM_CHT2_KEY1_0_USERDEFINED_BYTES_LONG_L2_IPv6_KEY_REG(dev)\
((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->IPCL.lookupKeyUserDefinedBytes.key1UserDefinedBytesConfigReg0   : \
0x0B800028)

/* user defined bytes address for extended l2-ipv6 pcl search key*/
#define SMEM_CHT2_KEY1_1_USERDEFINED_BYTES_LONG_L2_IPv6_KEY_REG(dev)\
((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->IPCL.lookupKeyUserDefinedBytes.key1UserDefinedBytesConfigReg1   : \
0x0B80002C)

/* user defined bytes address for extended l3-ipv6 pcl search key*/
#define SMEM_CHT2_KEY5_USERDEFINED_BYTES_LONG_L4_IPv6_KEY_REG(dev)\
((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->IPCL.lookupKeyUserDefinedBytes.key2UserDefinedBytesConfig   : \
0x0B800030)

/* address of the policy tcam table */
#define SMEM_CHT2_PCL_TCAM_DATA_TBL_MEM                           (0x0B880000)

/* address of the policy tcam ctrl table */
#define SMEM_CHT2_PCL_TCAM_CTRL_TBL_MEM                           (0x0B880004)

/* address of the policy tcam data mask table */
#define SMEM_CHT2_PCL_TCAM_DATA_MASK_TBL_MEM                      (0x0B880008)

/* address of the policy tcam ctrl mask table */
#define SMEM_CHT2_PCL_TCAM_CTRL_MASK_TBL_MEM                      (0x0B88000C)

/* address of the routing tcam table */
#define SMEM_CHT2_ROUTING_TCAM_DATA_TBL_MEM                       (0x02A00000)

/* address of the routing tcam ctrl table */
#define SMEM_CHT2_ROUTING_TCAM_CTRL_TBL_MEM                       (0x02A00004)

/* address of the routing tcam data mask table */
#define SMEM_CHT2_ROUTING_TCAM_DATA_MASK_TBL_MEM                  (0x02A00008)

/* address of the routing tcam ctrl mask table */
#define SMEM_CHT2_ROUTING_TCAM_CTRL_MASK_TBL_MEM                  (0x02A0000C)


/* cheetah2 pcl counter */
#define SMEM_CHT2_PCL_RULE_MATCH_REG(dev)\
((dev)->supportRegistersDb ? SMAIN_NOT_VALID_CNS : \
0x0B801000)

/* Egress Policy Global Registry */
#define SMEM_CHT2_EPCL_GLOBAL_REG(dev)\
((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->EPCL.egrPolicyGlobalConfig   : \
0x07C00000)

/* Egress Policy TO CPU Configuration */
#define SMEM_CHT2_EPCL_TO_CPU_CONFIG_REG(dev)\
((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->EPCL.egrPolicyToCpuConfig[0]   : \
0x07C00010)

/* Egress Policy FROM CPU Configuration */
#define SMEM_CHT2_EPCL_FROM_CPU_CONFIG_REG(dev)\
((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->EPCL.egrPolicyFromCpuDataConfig[0]   : \
0x07C00014)

/* Egress Policy TO ANALYSER configuration */
#define SMEM_CHT2_EPCL_TO_ANALYSER_CONFIG_REG(dev)\
((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->EPCL.egrPolicyToAnalyzerConfig[0]   : \
0x07C0001C)

/* Egress Policy Forward configuration */
#define SMEM_CHT2_EPCL_TS_FORWARD_CONFIG_REG(dev)\
((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->EPCL.egrPolicyTSDataPktsConfig[0]   : \
0x07C0000C)

/* Egress Policy Forward configuration */
#define SMEM_CHT2_EPCL_NTS_FORWARD_CONFIG_REG(dev)\
((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->EPCL.egrPolicyNonTSDataPktsConfig[0]   : \
0x07C00008)

/* Egress Policy Configuration Table Access mode configuration */
#define SMEM_CHT2_EPCL_TBL_ACCESS_MODE_CONFIG_REG(dev)\
((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->EPCL.egrPolicyConfigTableAccessModeConfig[0]   : \
0x07C00004)

/* Bridge Security Levels Configuration Register */
#define SMEM_CHT2_SECURITY_LEVEL_CONF_REG(dev)\
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->FDB.FDBCore.FDBGlobalConfig.bridgeAccessLevelConfig : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->FDB.FDBGlobalConfig.bridgeAccessLevelConfig   : \
    0x0600003C))

/* VLT Tables Access Data Register0 ,  */
#define SMEM_CHT2_VLAN_TBL_WORD0_REG(dev)\
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer0.VLANAndMcGroupAndSpanStateGroupTables.VLTTablesAccessDataReg[0]   : \
        SMAIN_NOT_VALID_CNS :                                       \
    0x0A00000C)


/* UDP Broadcast Destination Port<n> Configuration Table (index 0..11)*/
#define SMEM_CHT2_UDP_BROADCAST_DEST_PORT_TBL_MEM(dev,index) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->L2I.UDPBcMirrorTrapUDPRelayConfig.UDPBcDestinationPortConfigTable[index] : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->L2I.UDPBcMirrorTrapUDPRelayConfig.UDPBcDestinationPortConfigTable[index] : \
        (0x02000500 + ((index) * 4))))

/* Port<n> Protocol<m> VID and QoS Configuration Entry
   (0<=n<27, for CPU port n=0x3F 0<=M<8)
   in steps: ports - 0x4, members - 0x00010000 */
#define SMEM_CHT2_PROT_VLT_QOS_TBL_MEM                    (0x0b810800)

/* FDB Update Message Queue Configuration Registers */
#define SMEM_CHT2_FU_Q_BASE_ADDR_REG(dev)\
(0x000000CC)

/*  Bridge Access Matrix Line<n> Register (0<=n<7 */
#define SMEM_CHT2_BRIDGE_MATRIX_ACCESS_LINE_REG(dev,index) \
(SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->L2I.bridgeAccessMatrix.bridgeAccessMatrixLine[index] : \
 ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->L2I.bridgeAccessMatrix.bridgeAccessMatrixLine[index] : \
(0x02000600 + ((index) * 0x4))))


/* Bridge Global Configuration Register2 - Cht2 */
#define SMEM_CHT2_BRIDGE_GLOBAL_CONF2_REG(dev)\
(0x0204000C)

/* TO NETWORK Ingress Forwarding Restrictions Configuration Register - Cht2 */
#define SMEM_CHT2_TO_NW_INGR_FORWARD_RESTICT_REG(dev)\
(SMEM_CHT_IS_SIP5_GET(dev)  ?  SMAIN_NOT_VALID_CNS : \
((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->EQ.ingrForwardingRestrictions.TONETWORKIngrForwardingRestrictionsConfig   : \
0x0B020004))

/* TO ANALYZER Ingress Forwarding Restrictions Configuration Register - Cht2 */
#define SMEM_CHT2_TO_ANALYZER_INGR_FORWARD_RESTICT_REG(dev)\
(SMEM_CHT_IS_SIP5_GET(dev)  ?  SMAIN_NOT_VALID_CNS : \
((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->EQ.ingrForwardingRestrictions.toAnalyzerIngrForwardingRestrictionsConfig   : \
0x0B020008))

/* TO CPU Ingress Forwarding Restrictions Configuration Register - Cht2 */
#define SMEM_CHT2_TO_CPU_INGR_FORWARD_RESTICT_REG(dev)\
(SMEM_CHT_IS_SIP5_GET(dev)  ?  SMAIN_NOT_VALID_CNS : \
((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->EQ.ingrForwardingRestrictions.toCpuIngrForwardingRestrictionsConfig   : \
0x0B020000))

/* Ingress Forwarding Restrictions dropped packet counter - Cht2 */
#define SMEM_CHT2_INGR_FORWARD_RESTICT_COUNTER_REG(dev)\
(SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EQ.ingrForwardingRestrictions.ingrForwardingRestrictionsDroppedPktsCntr : \
((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->EQ.ingrForwardingRestrictions.ingrForwardingRestrictionsDroppedPktsCntr   : \
0x0B02000C))

/* Bridged Packets Filter Configuration Register - Cht2 */
#define SMEM_CHT2_BRIDGE_FILTER_CONF_REG(dev)\
    (0x018001DC)

/* FROM_CPU Packets Filter Configuration Register - Cht2 */
#define SMEM_CHT2_FROM_CPU_FILTER_CONF_REG(dev)\
    (0x018001D8)

/* Bridged Packets Filter Configuration Register - Cht2 */
#define SMEM_CHT2_ROUTE_FILTER_CONF_REG(dev)\
    (0x018001E0)

#define SMEM_CHT2_SRC_ID_ASSIGN_MOD_REG(dev)\
((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->L2I.bridgeEngineConfig.sourceIDAssignmentModeConfigPorts   : \
0x02040070)

#define SMEM_CHT2_ROUTER_ACCESS_DATA_CTRL_REG(dev)\
((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->TCCUpperIPvX.TCCIPvX.routerTCAMAccessCtrlReg0   : \
0x02800418)

#define        SMEM_CHT2_TRUNK_TBL_MEMBER_SUB_UNIT_INDEX_CNS        (0x4)

#define        SMEM_CHT2_QOS_PROFILE_UNIT_INDEX_CNS                 (0x3)

#define        SMEM_CHT2_CPU_CODE_UNIT_INDEX_CNS                    (0x2)

#define        SMEM_CHT2_STAT_RATE_LIM_UNIT_INDEX_CNS               (0x1)

#define        SMEM_CHT2_PORT_INGRESS_STC_UNIT_INDEX_CNS            (0x0)


/* *******************************  IPvX *************************************** */

/* Router Global Configuration Register  */
#define SMEM_CHT2_L3_ROUTE_GLB_CONTROL_REG(dev)\
    SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->IPvX.routerGlobalCtrl.routerGlobalCtrl0   : \
((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->IPvX.routerGlobalCtrl.routerGlobalCtrl0   : \
0x02800000)

/* Router Additional Control Register  */
/* SIP5 name: Router Global Control 1  */
#define SMEM_CHT2_ROUTER_ADDITIONAL_CONTROL_REG(dev)\
    SMEM_CHT_IS_SIP5_GET(dev) ?  SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->IPvX.routerGlobalCtrl.routerGlobalCtrl1  : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->IPvX.routerGlobalCtrl.routerAdditionalCtrl   : \
    0x02800964)

/* IPv4 Control 0 Register  */
#define SMEM_CHT2_L3_ROUTE_CONTROL0_IPV4_REG(dev)\
    SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->IPvX.IPv4GlobalCtrl.IPv4CtrlReg0 : \
((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->IPvX.IPv4GlobalCtrl.IPv4CtrlReg0   : \
0x02800100)
/* IPv4 Control 1 Register  */
#define SMEM_CHT2_L3_ROUTE_CONTROL1_IPV4_REG(dev)\
    SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->IPvX.IPv4GlobalCtrl.IPv4CtrlReg1 : \
((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->IPvX.IPv4GlobalCtrl.IPv4CtrlReg1   : \
0x02800104)

/* IPv4 Unicast Enable Control Register  */
#define SMEM_CHT2_L3_UNICAST_CONTROL_IPV4_REG(dev)\
((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->IPvX.IPv4GlobalCtrl.IPv4UcEnableCtrl   : \
0x02800108)
/* IPv4 Multicast Enable Control Register  */
#define SMEM_CHT2_L3_MULTICAST_CONTROL_IPV4_REG(dev)\
((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->IPvX.IPv4GlobalCtrl.IPv4McEnableCtrl   : \
0x0280010C)

/* IPv6 Control 0 Register  */
#define SMEM_CHT2_L3_ROUTE_CONTROL0_IPV6_REG(dev)\
    SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->IPvX.IPv6GlobalCtrl.IPv6CtrlReg0 : \
((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->IPvX.IPv6GlobalCtrl.IPv6CtrlReg0   : \
0x02800200)
/* IPv6 Control 1 Register  */
#define SMEM_CHT2_L3_ROUTE_CONTROL1_IPV6_REG(dev)\
    SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->IPvX.IPv6GlobalCtrl.IPv6CtrlReg1 : \
((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->IPvX.IPv6GlobalCtrl.IPv6CtrlReg1   : \
0x02800204)

/* IPv6 Unicast Enable Control Register  */
#define SMEM_CHT2_L3_UNICAST_CONTROL_IPV6_REG(dev)\
((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->IPvX.IPv6GlobalCtrl.IPv6UcEnableCtrl   : \
0x02800208)
/* IPv6 Multicast Enable Control Register  */
#define SMEM_CHT2_L3_MULTICAST_CONTROL_IPV6_REG(dev)\
((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->IPvX.IPv6GlobalCtrl.IPv6McEnableCtrl   : \
0x0280020C)

/* Router Drop Counter */
#define SMEM_CHT2_ROUTER_DROP_COUNTER_REG(dev)\
    SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->IPvX.routerDropCntr.routerDropCntr : \
((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->IPvX.routerDropCntr.routerDropCntr   : \
0x02800950)

/* Router Drop Counter Configuration*/
#define SMEM_CHT2_ROUTER_DROP_COUNTER_CONFIGURATION_REG(dev)\
    SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->IPvX.routerDropCntr.routerDropCntrConfig : \
((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->IPvX.routerDropCntr.routerDropCntrConfig   : \
0x02800954)


/* LTT Action Table Entry */
#define SMEM_CHT2_IP_ROUTE_ENTRY_TBL_MEM(dev,entry_indx)         \
    ((dev)->supportRegistersDb ? ((SMEM_CHT_MAC_REG_DB_GET(dev))->IPvX.startUnitInfo.unitBaseAddress + (0x00100000 + ((entry_indx) *0x10))): \
        (0x02900000  + (0x10* (entry_indx))))

/* QoS Profile to Route Block Offset Entry */
#define SMEM_CHT2_QOS_ROUTING_REG(dev, entry_indx) \
    SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, ipvxQoSProfileOffsets, entry_indx):\
    ((dev)->supportRegistersDb ? ((SMEM_CHT_MAC_REG_DB_GET(dev))->IPvX.startUnitInfo.unitBaseAddress + (0x00000300 + ((entry_indx) *0x4))): \
        (0x02800300 + (0x4 * (entry_indx))))

/* Unicast Multicast Router Next Hop Entry */
#define SMEM_CHT2_NEXT_HOP_ENTRY_TBL_MEM(dev,entry_indx)          \
    SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, ipvxNextHop, entry_indx):\
    ((dev)->supportRegistersDb ? ((SMEM_CHT_MAC_REG_DB_GET(dev))->IPvX.startUnitInfo.unitBaseAddress + (0x00300000 + ((entry_indx) *0x10))): \
    ((0x02B00000 + ((entry_indx) *0x10))))

/* Router Access Matrix Line */
#define SMEM_CHT2_ROUTER_ACCESS_MATRIX_REG(dev, entry_indx) \
    SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, ipvxAccessMatrix, entry_indx):\
    ((dev)->supportRegistersDb ? ((SMEM_CHT_MAC_REG_DB_GET(dev))->IPvX.startUnitInfo.unitBaseAddress + (0x00000440 + ((entry_indx) *0x4))): \
        (0x02800440 + (0x4 * (entry_indx))))

/* router table aging bits */
#define SMEM_CHT2_ROUTER_NEXT_HOP_AGE_BITS_TBL_MEM(dev, entry_indx) \
        SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, routeNextHopAgeBits, entry_indx)

/* Register 0..7 MTU Configuration */
#define SMEM_CHT2_ROUTER_MTU_CONFIGURATION_REG(dev, entry_indx) \
    SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->IPvX.routerGlobalCtrl.routerMTUConfigReg[(entry_indx)]: \
    ((dev)->supportRegistersDb ? ((SMEM_CHT_MAC_REG_DB_GET(dev))->IPvX.startUnitInfo.unitBaseAddress + (0x00000004 + ((entry_indx) *0x4))): \
        (0x02800004 + (0x4 * (entry_indx))))


/* IPv6 Scope Prefix Table */
#define SMEM_CHT2_ROUTER_SCOPE_PREFIX_TBL_MEM(dev, entry_indx) \
    SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->IPvX.IPv6Scope.IPv6UcScopePrefix[(entry_indx)] : \
    ((dev)->supportRegistersDb ? ((SMEM_CHT_MAC_REG_DB_GET(dev))->IPvX.startUnitInfo.unitBaseAddress + (0x00000250 + ((entry_indx) *0x4))): \
        (0x02800250 + (0x4 * (entry_indx))))

/* IPv6 Unicast Scope level Table */
#define SMEM_CHT2_ROUTER_UNICAST_SCOPE_LEVEL_TBL_MEM(dev, entry_indx) \
    SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->IPvX.IPv6Scope.IPv6UcScopeLevel[(entry_indx)] : \
    ((dev)->supportRegistersDb ? ((SMEM_CHT_MAC_REG_DB_GET(dev))->IPvX.startUnitInfo.unitBaseAddress + (0x00000260 + ((entry_indx) *0x4))): \
        (0x02800260 + (0x4 * (entry_indx))))


/* IPv6 Unicast Scope Command Table */
#define SMEM_CHT2_ROUTER_UNICAST_SCOPE_COMMAND_TBL_MEM(dev, entry_indx) \
    SMEM_CHT_IS_SIP5_GET(dev) ?  SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->IPvX.IPv6Scope.IPv6UcScopeTableReg[(entry_indx)] : \
    ((dev)->supportRegistersDb ? ((SMEM_CHT_MAC_REG_DB_GET(dev))->IPvX.startUnitInfo.unitBaseAddress + (0x00000278 + ((entry_indx) *0x4))): \
        (0x02800278 + (0x4 * (entry_indx))))

/* IPv6 Multicast Scope Command Table */
#define SMEM_CHT2_ROUTER_MULTICAST_SCOPE_COMMAND_TBL_MEM(dev, entry_indx) \
    SMEM_CHT_IS_SIP5_GET(dev) ?  SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->IPvX.IPv6Scope.IPv6McScopeTableReg[(entry_indx)] : \
    ((dev)->supportRegistersDb ? ((SMEM_CHT_MAC_REG_DB_GET(dev))->IPvX.startUnitInfo.unitBaseAddress + (0x00000288 + ((entry_indx) *0x4))): \
        (0x02800288 + (0x4 * (entry_indx))))



/* Router management in unicast packets counter */
#define SMEM_CHT2_ROUTER_MANAGEMENT_COUNTER_UNICAST_PACKET_REG(dev, entry_indx) \
    SMEM_CHT_IS_SIP5_GET(dev) ? \
            SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->IPvX.routerManagementCntrsSets.routerManagementInUcPktsCntrSet[(entry_indx)] : \
    ((dev)->supportRegistersDb ? ((SMEM_CHT_MAC_REG_DB_GET(dev))->IPvX.startUnitInfo.unitBaseAddress + (0x0000900 + ((entry_indx) *0x100))): \
        (0x02800900 + (0x100 * (entry_indx))))

/* Router management in multicast packets counter */
#define SMEM_CHT2_ROUTER_MANAGEMENT_COUNTER_MULTICAST_PACKET_REG(dev, entry_indx) \
    SMEM_CHT_IS_SIP5_GET(dev) ? \
            SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->IPvX.routerManagementCntrsSets.routerManagementInMcPktsCntrSet[(entry_indx)] : \
    ((dev)->supportRegistersDb ? ((SMEM_CHT_MAC_REG_DB_GET(dev))->IPvX.startUnitInfo.unitBaseAddress + (0x0000904 + ((entry_indx) *0x100))): \
        (0x02800904 + (0x100 * (entry_indx))))


/* Router Management In Unicast Non-Routed Non-Exception */
#define SMEM_CHT2_ROUTER_MANAGEMENT_COUNTER_NON_ROUTED_UNICAST_NON_EXPECTED_PACKET_REG(dev, entry_indx) \
    SMEM_CHT_IS_SIP5_GET(dev) ? \
            SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->IPvX.routerManagementCntrsSets.routerManagementInUcNonRoutedNonException[(entry_indx)] : \
    ((dev)->supportRegistersDb ? ((SMEM_CHT_MAC_REG_DB_GET(dev))->IPvX.startUnitInfo.unitBaseAddress + (0x0000908 + ((entry_indx) *0x100))): \
        (0x02800908 + (0x100 * (entry_indx))))

/* Router Management In Unicast Non-Routed Exception */
#define SMEM_CHT2_ROUTER_MANAGEMENT_COUNTER_UNICAST_EXPECTED_PACKET_REG(dev, entry_indx) \
    SMEM_CHT_IS_SIP5_GET(dev) ? \
            SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->IPvX.routerManagementCntrsSets.routerManagementInUcNonRoutedExceptionPktsCntrSet[(entry_indx)] : \
    ((dev)->supportRegistersDb ? ((SMEM_CHT_MAC_REG_DB_GET(dev))->IPvX.startUnitInfo.unitBaseAddress + (0x000090C + ((entry_indx) *0x100))): \
        (0x0280090C + (0x100 * (entry_indx))))


/* Router Management In Multicast Non-Routed Non-Exception */
#define SMEM_CHT2_ROUTER_MANAGEMENT_COUNTER_NON_ROUTED_MULTICAST_NON_EXPECTED_PACKET_REG(dev, entry_indx) \
    SMEM_CHT_IS_SIP5_GET(dev) ? \
            SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->IPvX.routerManagementCntrsSets.routerManagementInMcNonRoutedNonExceptionPktsCntrSet[(entry_indx)] : \
    ((dev)->supportRegistersDb ? ((SMEM_CHT_MAC_REG_DB_GET(dev))->IPvX.startUnitInfo.unitBaseAddress + (0x0000910 + ((entry_indx) *0x100))): \
        (0x02800910 + (0x100 * (entry_indx))))

/* Router Management In Multicast Non-Routed Exception */
#define SMEM_CHT2_ROUTER_MANAGEMENT_COUNTER_MULTICAST_EXPECTED_PACKET_REG(dev, entry_indx) \
    SMEM_CHT_IS_SIP5_GET(dev) ? \
            SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->IPvX.routerManagementCntrsSets.routerManagementInMcNonRoutedExceptionPktsCntrSet[(entry_indx)] : \
    ((dev)->supportRegistersDb ? ((SMEM_CHT_MAC_REG_DB_GET(dev))->IPvX.startUnitInfo.unitBaseAddress + (0x0000914 + ((entry_indx) *0x100))): \
        (0x02800914 + (0x100 * (entry_indx))))

/* Router Management In Unicast Trapped / Mirrored Packet Counter Set */
#define SMEM_CHT2_ROUTER_MANAGEMENT_COUNTER_UNICAST_TRAPPED_MIRRORED_PACKET_REG(dev, entry_indx) \
    SMEM_CHT_IS_SIP5_GET(dev) ? \
            SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->IPvX.routerManagementCntrsSets.routerManagementInUcTrappedMirroredPktCntrSet[(entry_indx)] : \
    ((dev)->supportRegistersDb ? ((SMEM_CHT_MAC_REG_DB_GET(dev))->IPvX.startUnitInfo.unitBaseAddress + (0x0000918 + ((entry_indx) *0x100))): \
        (0x02800918 + (0x100 * (entry_indx))))

/* Router Management In Multicast Trapped / Mirrored Packet Counter Set */
#define SMEM_CHT2_ROUTER_MANAGEMENT_COUNTER_MULTICAST_TRAPPED_MIRRORED_PACKET_REG(dev, entry_indx) \
    SMEM_CHT_IS_SIP5_GET(dev) ? \
            SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->IPvX.routerManagementCntrsSets.routerManagementInMcTrappedMirroredPktCntrSet[(entry_indx)] : \
    ((dev)->supportRegistersDb ? ((SMEM_CHT_MAC_REG_DB_GET(dev))->IPvX.startUnitInfo.unitBaseAddress + (0x000091C + ((entry_indx) *0x100))): \
        (0x0280091C + (0x100 * (entry_indx))))

/* Router Management In Multicast RPF Fail Counter Set<%n> */
#define SMEM_CHT2_ROUTER_MANAGEMENT_COUNTER_RPF_FAIL_MULTICAST_PACKET_REG(dev, entry_indx) \
    SMEM_CHT_IS_SIP5_GET(dev) ? \
            SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->IPvX.routerManagementCntrsSets.routerManagementInMcRPFFailCntrSet[(entry_indx)] : \
    ((dev)->supportRegistersDb ? ((SMEM_CHT_MAC_REG_DB_GET(dev))->IPvX.startUnitInfo.unitBaseAddress + (0x0000920 + ((entry_indx) *0x100))): \
        (0x02800920 + (0x100 * (entry_indx))))

/* Router Management Out Unicast Packet Counter Set<%n> */
#define SMEM_CHT2_ROUTER_MANAGEMENT_COUNTER_OUT_PACKET_REG(dev, entry_indx) \
    SMEM_CHT_IS_SIP5_GET(dev) ? \
            SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->IPvX.routerManagementCntrsSets.routerManagementOutUcPktCntrSet[(entry_indx)] : \
    ((dev)->supportRegistersDb ? ((SMEM_CHT_MAC_REG_DB_GET(dev))->IPvX.startUnitInfo.unitBaseAddress + (0x0000924 + ((entry_indx) *0x100))): \
        (0x02800924 + (0x100 * (entry_indx))))

/* Router Management Counters Set<%n> Configuration0 */
#define SMEM_CHT2_ROUTER_MANAGEMENT_COUNTER_CONFIGURATION_REG(dev, entry_indx) \
    SMEM_CHT_IS_SIP5_GET(dev) ? \
            SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->IPvX.routerManagementCntrsSets.routerManagementCntrsSetConfig0[(entry_indx)] : \
    ((dev)->supportRegistersDb ? ((SMEM_CHT_MAC_REG_DB_GET(dev))->IPvX.startUnitInfo.unitBaseAddress + (0x0000980 + ((entry_indx) *0x100))): \
        (0x02800980 + (0x100 * (entry_indx))))

/* *******************************  IPvX end *************************************** */


#define SMEM_CHT2_MLL_GLB_CONTROL_REG(dev)\
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->MLL.MLLGlobalCtrl.MLLGlobalCtrl : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->MLL.MLLGlobalCtrl.MLLGlobalCtrl   : \
    SMEM_CHT3_MLL_GLB_CONTROL_REG(dev)))

/* Unicast SrcId Egress Filtering Configuration Register */
#define SMEM_CHT2_SST_EGRESS_CONFIG_REG(dev)\
    (0x018001D4)

/* Tunnel Egress Filtering Configuration Register */
#define SMEM_CHT2_TUNNEL_EGRESS_CONFIG_REG(dev)\
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer0.transmitQueueGlobalConfig.transmitQueueResourceSharing   : \
        SMAIN_NOT_VALID_CNS :                                       \
    0x018001F0)


#define SMEM_CHT2_TUNNEL_START_TBL_MEM(dev, entry_indx) \
        SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev,tunnelStart,entry_indx)

#define        SMEM_CHT2_QOS_RESERV_OFFSET_INDEX_CNS                (128)


/* Egress PCL Configuration Table Entry<n> (0<=n<4159)          */
/* offset formula : 0x07700000 + entry<n>*0x4                   */
#define SMEM_CHT2_EPCL_CONFIG_TBL_MEM(entry_indx)          \
                (0x07700000 + ((entry_indx)*0x04))

/* Action Table and Policy TCAM Access Data Register */
#define SMEM_CHT2_PCL_ACTION_TCAM_TBL_MEM(entry_indx) \
                (0x0B800100 + ( 0x04 * (entry_indx)))

/* Policy TCAM Entry<n> Data Word<0> (0<=n<1024, 0<=m<3) */
#define SMEM_CHT2_PCL_TCAM_ENTRY_WORD0_TBL_MEM(entry_indx) \
                (0x0B880000 + (0x10 * (entry_indx)))

/* Policy TCAM Data Entry<n> Control0 */
#define SMEM_CHT2_PCL_TCAM_WORD_CTRL0_TBL_MEM(entry_indx) \
                (0x0B880004 + (0x10 * (entry_indx)))

/* Policy TCAM Entry<n> Mask Word<0> (0<=n<1024, 0<=m<3) */
#define SMEM_CHT2_PCL_TCAM_ENTRY_MASK0_TBL_MEM(entry_indx) \
                (0x0B880008 + (0x10 * (entry_indx)))

/* Policy TCAM Mask Entry<n> Control0 */
#define SMEM_CHT2_PCL_TCAM_WORD_CTRL_MASK0_TBL_MEM(entry_indx) \
                (0x0B88000C + (0x10 * (entry_indx)))


/* router TCAM Entry<n> Data Word <0> (0<=n<1024, 0<=m<3) */
#define SMEM_CHT2_ROUTER_TCAM_ENTRY_DATA_WORD0_TBL_MEM(entry_indx) \
                (0x02A00000 + (0x10 * (entry_indx)))

/* router TCAM Entry<n> control Word <0> (0<=n<1024, 0<=m<3) */
#define SMEM_CHT2_ROUTER_TCAM_ENTRY_CTRL_WORD0_TBL_MEM(entry_indx) \
                (0x02A00004 + (0x10 * (entry_indx)))

/* router TCAM Entry<n> Data Word Mask <0> (0<=n<1024, 0<=m<3) */
#define SMEM_CHT2_ROUTER_TCAM_ENTRY_MASK_WORD0_TBL_MEM(entry_indx) \
                (0x02A00008 + (0x10 * (entry_indx)))

/* router TCAM Entry<n> Ctrl Word Mask <0> (0<=n<1024, 0<=m<3) */
#define SMEM_CHT2_ROUTER_TCAM_ENTRY_CTRL_MASK_WORD0_TBL_MEM(entry_indx) \
            (0x02A0000C + (0x10 * (entry_indx)))

#define SMEM_CHT2_ROUTER_TCAM_ACCESS_DATA_TBL_MEM(entry_indx) \
            (0x02800400 + ( 0x04 * (entry_indx)))

/*  Port<n> Status Register0 (0<=n<24 , CPUPort = 63) ,
  Offset : 0x0A800010 + port * 0x0100 , CPUPort : 0x0A803F10 */
#define SMEM_CHT2_PORT_STATUS0_REG(dev,port)\
            (0x0A800010 + 0x100*(port))

/* VLAN MRU Profile<2n...2n+1> Configuration register ;*/
#define SMEM_CHT2_MRU_PROFILE_REG(dev, entry_index) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->L2I.VLANMRUProfilesConfig.VLANMRUProfileConfig[entry_index] : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->L2I.VLANMRUProfilesConfig.VLANMRUProfileConfig[entry_index] : \
        (0x02000300 + (4 *  (entry_index))) ))

/* Return address for only even word 0 or 2 */
#define SMEM_CHT2_ARP_TBL_EVEN_WORD_OFFSET_CNS(entry_index) \
            ((entry_index) / 2) * 0xc

/* IP Protocol CPU Code Valid Configuration Register */
#define SMEM_CHT2_IP_PROT_CPU_CODE_VALID_CONF_REG(dev)\
(SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EQ.applicationSpecificCPUCodes.IPProtocolCPUCodeValidConfig : \
((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->EQ.applicationSpecificCPUCodes.IPProtocolCPUCodeValidConfig   : \
0x0B008010))

/* HA - Tunnel Start Fragment ID Configuration register */
#define SMEM_CHT2_HA_TUNNEL_START_FRAGMENT_ID_CONFIG_REG(dev) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->HA.tunnelStartFragmentIDConfig : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->HA.tunnelStartFragmentIDConfig : \
    ((SMEM_CHT3_ROUTE_HA_GLB_CNF_REG(dev) & (~0xFFFF)) | 0x410)))


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __sregcheetah2h */


