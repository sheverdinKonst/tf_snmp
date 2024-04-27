/******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* sregxCat.h
*
* DESCRIPTION:
*       Defines for xCat memory registers access.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 68 $
*
*******************************************************************************/
#ifndef __sregXCath
#define __sregXCath

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Ingress Policer0,1 Interrupt Cause Register Summary bits */
#define SNET_XCAT_INGR_POLICER0_INTR_SUM_E(dev)  \
    (SKERNEL_IS_XCAT_REVISON_A1_DEV(dev) ?      \
        (1 << 22) :                             \
        (0))

#define SNET_XCAT_INGR_POLICER1_INTR_SUM_E(dev)  \
    (SKERNEL_IS_XCAT_REVISON_A1_DEV(dev) ?      \
        (1 << 23) :                             \
        (0))

/*
    get the base address of the policer units
    cycle 0 - iplr 0
    cycle 1 - iplr 1
    cycle 2 - eplr

*/
#define SMEM_XCAT_POLICER_BASE_ADDR_MAC(dev, cycle)      \
    dev->memUnitBaseAddrInfo.policer[cycle]

/*
    get the register address according to offset from the base address of the
    policer unit

    cycle 0 - iplr 0
    cycle 1 - iplr 1
    cycle 2 - eplr


    offset - register offset from start of the unit
*/
#define SMEM_XCAT_POLICER_OFFSET_MAC(dev, cycle , offset)      \
    (SMEM_XCAT_POLICER_BASE_ADDR_MAC(dev, cycle) + (offset))

/* Ingress global TPID */
#define SMEM_XCAT_INGR_GLOBAL_TAG_TPID_REG(dev)                      \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.VLANAssignment.ingrTPIDConfig_[0] : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->TTI.VLANAssignment.ingrTPIDConfig[0]   : \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev) ? 0x01000300 : \
         0x0c000300))

/* Ingress Port Tag0/Tag1 TPID */
#define SMEM_XCAT_INGR_PORT_TPID_REG(dev)                            \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMAIN_NOT_VALID_CNS :            \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->TTI.VLANAssignment.ingrTPIDSelect[0]   : \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev) ? 0x01000310 :         \
         0x0c000310))

/* HA - Egress global TPID */
#define SMEM_XCAT_HA_GLOBAL_TAG_TPID_REG(dev,index)                      \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->HA.TPIDConfigReg[index]  :            \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->HA.TPIDConfigReg[index]   : \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev) ? (0x0E800500 + 4*(index)):         \
         (0x0f000430 + 4*(index))))

/* Egress Port Tag0 TPID */
#define SMEM_XCAT_TAG0_EGR_PORT_TPID_REG(dev)                       \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMAIN_NOT_VALID_CNS :            \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->HA.portTag0TPIDSelect[0]   : \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev) ? 0x0E800510 :         \
         0x0f000440))

/* Egress Port Tag1 TPID */
#define SMEM_XCAT_TAG1_EGR_PORT_TPID_REG(dev)                       \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMAIN_NOT_VALID_CNS :            \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->HA.portTag1TPIDSelect[0]   : \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev) ? 0x0E800530 :         \
         0x0f000460))

/* Header Alteration Global Configuration */
#define SMEM_XCAT_ROUTE_HA_GLB_CNF_REG(dev) \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->HA.haGlobalConfig : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->HA.haGlobalConfig : \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev) ? 0x0E800100 :          \
         0x0f000100))

/* Header Alteration VLAN Translation Enable Register Address */
#define SMEM_XCAT_HA_VLAN_TRANS_EN_REG(dev)                        \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMAIN_NOT_VALID_CNS :            \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->HA.haVLANTranslationEnable[0]   : \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev) ? 0x0E800130 :         \
         0x0f000130))


/* Header Alteration I Ethertype global configuration */
#define SMEM_XCAT_HA_I_ETHERTYPE_REG(dev)                           \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->HA.IEtherType  :            \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->HA.IEthertype  : \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev) ? 0x0E800554 :         \
         0x0f000484))

/* Cascading and Header Insertion Configuration Register */
#define SMEM_XCAT_CAS_HDR_INS_REG(dev)                              \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMAIN_NOT_VALID_CNS : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->HA.cascadingAndHeaderInsertionConfig[0] : \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev) ? 0x0E800004 :         \
         0x0f000004))

/* Router Header Alteration Enable MAC SA Modification Register */
#define SMEM_XCAT_ROUT_HA_MAC_SA_EN_REG(dev)                        \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMAIN_NOT_VALID_CNS : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->HA.routerHaEnableMACSAModification[0] : \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev) ? 0x0E800104 :         \
         0x0F000104))

/* QoSProfile to DP Register<%n> */
#define SMEM_XCAT_HA_QOS_PROFILE_TO_DP_REG(dev)                    \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMAIN_NOT_VALID_CNS : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->HA.qoSProfileToDPReg[0] : \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev) ? 0x0E800340 :         \
         0x0F000340))

/* Router MAC SA Base Register0 - xCat */
#define SMEM_XCAT_MAC_SA_BASE0_REG(dev)                             \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->HA.routerMACSABaseReg0 : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->HA.routerMACSABaseReg0  : \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev) ? 0x0E80010C :         \
         0x0F000108))

/* Router Header Alteration MAC SA Modification Mode */
#define SMEM_XCAT_MAC_SA_MODIFICATION_MODE_REG(dev)                 \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMAIN_NOT_VALID_CNS : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->HA.routerHaMACSAModificationMode[0] :  \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev) ? 0x0E800120 :         \
         0x0F000120))

/* Egress DSA Tag Type Configuration */
#define SMEM_XCAT_DSA_TAG_TYPE_REG(dev)                              \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMAIN_NOT_VALID_CNS : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->HA.egrDSATagTypeConfig[0] : \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev) ? 0x0E800020 :         \
         0x0f000020))

/* L2 Port Isolation Table Entry%n */
#define SMEM_XCAT_L2_PORT_ISOLATION_REG(dev, index)  \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, l2PortIsolation, index)

/* L3 Port Isolation Table Entry%n */
#define SMEM_XCAT_L3_PORT_ISOLATION_REG(dev, index)  \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, l3PortIsolation, index)

/* Transmit Queue Resource Sharing */
#define SMEM_XCAT_TXQ_RESOURCE_SHARE_REG(dev)                       \
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer0.transmitQueueGlobalConfig.transmitQueueResourceSharing : \
        SMAIN_NOT_VALID_CNS :                                       \
    (0x018001f0))

/* Transmit Queue Extended Control1 */
#define SMEM_XCAT_TXQ_EXTENDED_CONTROL1_REG(dev)                    \
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer0.transmitQueueGlobalConfig.transmitQueueExtCtrl1 : \
        SMAIN_NOT_VALID_CNS :                                       \
    (0x018001b0))

/* TTI Internal, Metal Fix*/
#define SMEM_XCAT_TTI_INTERNAL_METAL_FIX_REG(dev)                   \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->TTI.metalFix : \
        (SKERNEL_IS_LION_REVISON_B0_DEV(dev) ? 0x010002b0 :         \
     0x0c0002b0))

/* Protocol based VLAN registers - config 0 */
#define SMEM_XCAT_TTI_PROT_ENCAP_CONF_REG(dev)                      \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.protocolMatch.protocolsEncapsulationConfig0 : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->TTI.protocolMatch.protocolsEncapsulationConfig0 : \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev) ? 0x01000098 :         \
         0x0C000098))

/* Protocol based VLAN registers - config 1 */
#define SMEM_XCAT_TTI_PROT_ENCAP_CONF1_REG(dev)                      \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.protocolMatch.protocolsEncapsulationConfig1 : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->TTI.protocolMatch.protocolsEncapsulationConfig1 : \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev) ? 0x0100009C :         \
         0x0C00009C))

#define SMEM_XCAT_TTI_PROT_CONF_REG(dev)                            \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.protocolMatch.protocolsConfig[0] : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->TTI.protocolMatch.protocolsConfig[0] : \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev) ? 0x01000080 :         \
         0x0C000080))

/* IPv6 Solicited-Node Multicast Address Configuration Register0 */
#define SMEM_XCAT_SOLIC_NODE_MCST_ADDR_REG(dev)                     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.IPv6SolicitedNode.IPv6SolicitedNodeMcAddrConfigReg0 : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->TTI.IPv6SolicitedNodeMcAddrConfigReg[0] : \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev) ? 0x01000040 :         \
         0x0C000040))

/* IPv6 Solicited-Node Multicast Address Mask Register0 */
#define SMEM_XCAT_SOLIC_NODE_MCST_MASK_REG(dev)                     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.IPv6SolicitedNode.IPv6SolicitedNodeMcAddrMaskReg0 : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->TTI.IPv6SolicitedNodeMcAddrMaskReg[0] : \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev) ? 0x01000050 :         \
         0x0C000050))

/* Port<n> Protocol<m> VID and QoS Configuration Entry */
#define SMEM_XCAT_TTI_PROT_VLAN_QOS_TBL_MEM(protocol, port) \
            (0xc004000 | ((port) * 0x80) | ((protocol) * 0x8))

/* TTI Unit Global Configuration */
#define SMEM_XCAT_A1_TTI_UNIT_GLB_CONF_REG(dev)                     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.TTIUnitGlobalConfigs.TTIUnitGlobalConfig : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->TTI.TTIUnitGlobalConfig : \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev) ? 0x01000000 :         \
         0x0C000000))

/* TTI Engine Configuration */
#define SMEM_XCAT_TTI_ENGINE_CONFIG_REG(dev)                        \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.TTIEngine.TTIEngineConfig : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->TTI.TTIEngineConfig : \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev) ? 0x0100000C :         \
         0x0C00000C))

/* TTI PCL ID Configuration 0*/
#define SMEM_XCAT_TTI_PCL_ID_CONFIG0_REG(dev)                       \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.TTIEngine.TTIPCLIDConfig0 : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->TTI.TTIPCLIDConfig0 : \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev) ? 0x01000010 :         \
         0x0C000010))

/* TTI PCL ID Configuration 1*/
#define SMEM_XCAT_TTI_PCL_ID_CONFIG1_REG(dev)                       \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.TTIEngine.TTIPCLIDConfig1 : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->TTI.TTIPCLIDConfig1 : \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev) ? 0x01000014 :         \
         0x0C000014))

/* TTI PCL TCP/UDP comparators */
#define SMEM_XCAT_TTI_PCL_TCP_PORT_RANGE_COMPARE_REG(dev)           \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.userDefinedBytes.IPCLTCPPortRangeComparatorConfig[0] : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->TTI.userDefinedBytes.IPCLTCPPortRangeComparatorConfig[0]   : \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev) ? 0x010000A0 :         \
         0x0C0000A0))

#define SMEM_XCAT_TTI_PCL_UDP_PORT_RANGE_COMPARE_REG(dev)           \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.userDefinedBytes.IPCLUDPPortRangeComparatorConfig[0] : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->TTI.userDefinedBytes.IPCLUDPPortRangeComparatorConfig[0]   : \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev) ? 0x010000C0 :         \
         0x0C0000C0))

/* User Defined Bytes Configuration Memory */
#define SMEM_XCAT_POLICY_UDB_CONF_MEM_REG(dev, index) \
      SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, ipclUserDefinedBytesConf, index)

/* Policy Engine User Defined Bytes Configuration */
#define SMEM_XCAT_POLICY_UDB_CONF_REG(dev)                          \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->PCL.policyEngineUserDefinedBytesConfig: \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->IPCL.policyEngineUserDefinedBytesConfig : \
    (0x0b800014)))

/* Policy_Engine_Configuration */
#define SMEM_XCAT_POLICY_ENGINE_CONF_REG(dev)                       \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->PCL.policyEngineConfig: \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->IPCL.policyEngineConfig : \
    (0x0b80000c)))

/* Special EtherTypes */
#define SMEM_XCAT_SPECIAL_ETHERTYPES_REG(dev)                       \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.specialEtherTypes: \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->TTI.specialEtherTypes : \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev) ? 0x01000028 :         \
         0x0c000028))

/* Egress Policy Global Configuration */
#define SMEM_XCAT_EPCL_GLOBAL_REG(dev)                  \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->EPCL.egrPolicyGlobalConfig : \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev) ?               \
        SMEM_LION_EPCL_GLOBAL_REG(dev) : (0x0f800000))

/* Egress Policy TO CPU Configuration */
#define SMEM_XCAT_EPCL_TO_CPU_CONFIG_REG(dev)           \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->EPCL.egrPolicyToCpuConfig[0] : \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev) ?               \
        SMEM_LION_EPCL_TO_CPU_CONFIG_REG(dev) : (0x0f800010))

/* Egress Policy FROM CPU Data Configuration */
#define SMEM_XCAT_EPCL_FROM_CPU_DATA_CONFIG_REG(dev)         \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->EPCL.egrPolicyFromCpuDataConfig[0] : \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev) ?               \
        SMEM_LION_EPCL_FROM_CPU_DATA_CONFIG_REG(dev) : (0x0f800014))

/* Egress Policy FROM CPU control Configuration */
#define SMEM_XCAT_EPCL_FROM_CPU_CONFIG_REG(dev)         \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->EPCL.egrPolicyFromCpuCtrlConfig[0] : \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev) ?               \
        SMEM_LION_EPCL_FROM_CPU_CONFIG_REG(dev) : (0x0f800018))

/* Egress Policy TO ANALYZER Configuration */
#define SMEM_XCAT_EPCL_TO_ANALYZER_CONFIG_REG(dev)      \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->EPCL.egrPolicyToAnalyzerConfig[0] : \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev) ?               \
        SMEM_LION_EPCL_TO_ANALYZER_CONFIG_REG(dev) : (0x0f80001c))

/* Egress Policy Non TS Data Pkts Configuration */
#define SMEM_XCAT_EPCL_NTS_FORWARD_CONFIG_REG(dev)      \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->EPCL.egrPolicyNonTSDataPktsConfig[0] : \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev) ?               \
        SMEM_LION_EPCL_NTS_FORWARD_CONFIG_REG(dev) : (0x0f800008))

/* Egress Policy TS Data Pkts Configuration */
#define SMEM_XCAT_EPCL_TS_FORWARD_CONFIG_REG(dev)       \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->EPCL.egrPolicyTSDataPktsConfig[0] : \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev) ?               \
        SMEM_LION_EPCL_TS_FORWARD_CONFIG_REG(dev) : (0x0f80000c))

/* Egress PCL Configuration Table */
#define SMEM_XCAT_EPCL_CONFIG_TBL_MEM(dev, index)         \
    ((SKERNEL_IS_LION_REVISON_B0_DEV(dev)                 \
        && (! SKERNEL_DEVICE_FAMILY_XCAT2_ONLY(dev)))     \
        ? SMEM_LION_EPCL_CONFIG_TBL_MEM(dev, index) : 0xf808000 + 0x4 * (index))

/* Egress Policy Configuration Table Access mode Configuration */
#define SMEM_XCAT_EPCL_TBL_ACCESS_MODE_CONFIG_REG(dev)      \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->EPCL.egrPolicyConfigTableAccessModeConfig[0] : \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev) ?                   \
        SMEM_LION_EPCL_TBL_ACCESS_MODE_CONFIG_REG(dev) : (0x0f800004))


/* L2/L3 Ingress VLAN Counting Enable Register */
#define SMEM_XCAT_CNC_VLAN_EN_REG(dev, port) \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->PCL.L2L3IngrVLANCountingEnable[(port) / 32]: \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->IPCL.L2L3IngrVLANCountingEnable[(port) / 32] : \
    0xb800080 + 0x4 * ((port) / 32)))

/* Counting Mode Configuration */
#define SMEM_XCAT_CNC_COUNT_MODE_REG(dev) \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->PCL.countingModeConfig : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->IPCL.countingModeConfig : \
    (0x0b800088)))

/* Ingress Policer Metering Memory */
#define SMEM_XCAT_INGR_POLICER_METER_ENTRY_TBL_MEM(dev, cycle, index) \
    SMEM_TABLE_ENTRY_2_PARAMS_INDEX_GET_MAC(dev, policer, index, cycle)

/* Egress Policer Metering Memory */
#define SMEM_XCAT_EGR_POLICER_METER_ENTRY_TBL_MEM(dev, index) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, egressPolicerMeters, index)

/* Policer Port Metering */
#define SMEM_XCAT_POLICER_PORT_METER_REG(dev, cycle, port) \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->PLR[cycle].policerPortMeteringReg[(port) / 32] : \
    ((dev)->supportRegistersDb ? \
        ((port < 31) ?            \
            (SMEM_CHT_MAC_REG_DB_GET(dev))->PLR[cycle].policerPortMetering0 : \
            (SMEM_CHT_MAC_REG_DB_GET(dev))->PLR[cycle].policerPortMetering1) : \
    (SMEM_XCAT_POLICER_OFFSET_MAC(dev, cycle , 0x00000008) | ((port) / 32 * 0x4))))

/* Ingress_Policer_Initial DP */
#define SMEM_XCAT_POLICER_INITIAL_DP_REG(dev, cycle,qosProfile) \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->PLR[cycle].policerInitialDP[(qosProfile)/16] : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->PLR[cycle].policerInitialDP[(qosProfile)/16] : \
    (SMEM_XCAT_POLICER_OFFSET_MAC(dev, cycle , 0x000000C0) + (((qosProfile)/16) * 4))))

/* Policer Control0 */
#define SMEM_XCAT_POLICER_GLB_CONF_REG(dev, cycle) \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->PLR[cycle].policerCtrl0 : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->PLR[cycle].policerCtrl0 : \
    SMEM_XCAT_POLICER_OFFSET_MAC(dev, cycle , 0x00000000)))

/* Policer Control1 */
#define SMEM_XCAT_POLICER_GLB1_CONF_REG(dev, cycle) \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->PLR[cycle].policerCtrl1 : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->PLR[cycle].policerCtrl1 : \
    SMEM_XCAT_POLICER_OFFSET_MAC(dev, cycle , 0x00000004)))

/* Policer hierarchical policer ctrl register */
#define SMEM_XCAT_PLR_HIERARCHICAL_POLICER_CTRL_REG(dev, cycle) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->PLR[cycle].hierarchicalPolicerCtrl : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->PLR[cycle].hierarchicalPolicerCtrl : \
    SMEM_XCAT_POLICER_OFFSET_MAC(dev, cycle , 0x0000002c)))

#define SMEM_XCAT_POLICER_MNG_CNT_TBL_MEM(dev, cycle, counter) \
    SMEM_TABLE_ENTRY_INDEX_DUP_TBL_GET_MAC(dev, policerManagementCounters , (counter*4) , cycle)

/* Ingress/Egress Policer IPFIX dropped packet count WA threshold */
#define SMEM_XCAT_POLICER_IPFIX_DROP_CNT_WA_TRESH_REG(dev, cycle) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->PLR[cycle].policerIPFIXDroppedPktCountWAThreshold : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->PLR[cycle].policerIPFIXDroppedPktCountWAThreshold : \
    SMEM_XCAT_POLICER_OFFSET_MAC(dev, cycle , 0x00000030)))


/* Ingress/Egress Policer IPFIX packet count WA threshold */
#define SMEM_XCAT_POLICER_IPFIX_PCKT_CNT_WA_TRESH_REG(dev, cycle) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->PLR[cycle].policerIPFIXPktCountWAThreshold : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->PLR[cycle].policerIPFIXPktCountWAThreshold : \
    SMEM_XCAT_POLICER_OFFSET_MAC(dev, cycle , 0x00000034)))

/* Ingress/Egress Policer IPFIX bytecount WA threshold LSB */
#define SMEM_XCAT_POLICER_IPFIX_BYTE_CNT_LSB_WA_TRESH_REG(dev, cycle) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->PLR[cycle].policerIPFIXByteCountWAThresholdLSB : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->PLR[cycle].policerIPFIXBytecountWAThresholdLSB : \
    SMEM_XCAT_POLICER_OFFSET_MAC(dev, cycle , 0x00000038)))

/* Ingress/Egress Policer IPFIX Control */
#define SMEM_XCAT_POLICER_IPFIX_CTRL_REG(dev, cycle) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->PLR[cycle].IPFIXCtrl : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->PLR[cycle].IPFIXCtrl : \
    SMEM_XCAT_POLICER_OFFSET_MAC(dev, cycle , 0x00000014)))


/* Ingress Policer Interrupt Cause */
#define SMEM_XCAT_POLICER_IPFIX_INT_CAUSE_REG(dev, cycle) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->PLR[cycle].policerInterruptCause : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->PLR[cycle].policerInterruptCause : \
    SMEM_XCAT_POLICER_OFFSET_MAC(dev, cycle , 0x00000100)))

/* Ingress/Egress IPFIX Sample Entries Log0 */
#define SMEM_XCAT_POLICER_IPFIX_SAMPLE_LOG0_REG(dev, cycle) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->PLR[cycle].IPFIXSampleEntriesLog0 : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->PLR[cycle].IPFIXSampleEntriesLog0 : \
    SMEM_XCAT_POLICER_OFFSET_MAC(dev, cycle , 0x00000048)))

/* Ingress/Egress management counters policer Shadow 0 */
#define SMEM_XCAT_POLICER_MANAGEMENT_COUNTERS_POLICER_SHADOW_REG(dev, cycle) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->PLR[cycle].policerShadow[0] : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->PLR[cycle].policerShadow[0] : \
    SMEM_XCAT_POLICER_OFFSET_MAC(dev, cycle , 0x00000108)))

/* Ingress/Egress policer MRU */
#define SMEM_XCAT_POLICER_MRU_REG(dev, cycle) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->PLR[cycle].policerMRU : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->PLR[cycle].policerMRU : \
    SMEM_XCAT_POLICER_OFFSET_MAC(dev, cycle , 0x00000010)))

/* Ingress/Egress hierarchical table */
#define SMEM_XCAT_POLICER_HIERARCHCIAL_TBL_MEM(dev, cycle, index) \
    SMEM_TABLE_ENTRY_INDEX_DUP_TBL_GET_MAC(dev, policerHierarchicalQos, index, cycle)

/* Egress policer relevant registers */

/* Ingress Policer Counting Memory */
#define SMEM_XCAT_INGR_POLICER_CNT_ENTRY_REG(dev, index, cycle) \
    SMEM_TABLE_ENTRY_2_PARAMS_INDEX_GET_MAC(dev, policerCounters, index, cycle)

/* Egress Policer Counting Memory */
#define SMEM_XCAT_EGR_POLICER_CNT_ENTRY_REG(dev, index) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, egressPolicerCounters, index)

/* VLT Tables Access Data Register */
#define SMEM_XCAT_VLAN_TBL_ACCESS_BASE_REG(dev)                     \
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer0.VLANAndMcGroupAndSpanStateGroupTables.VLTTablesAccessDataReg[5] : \
        SMAIN_NOT_VALID_CNS :                                       \
    (0x0A000100))

/* Links Status Register */
#define SMEM_LION_LINKS_STATUS_REG_MAC(dev)                         \
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer0.transmitQueueGlobalConfig.portsLinkEnable : \
        SMAIN_NOT_VALID_CNS :                                       \
    (0x01800248))

/* Port Ingress Mirror Index */
#define SMEM_XCAT_INGR_MIRROR_INDEX_REG(dev, port) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMAIN_NOT_VALID_CNS : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->EQ.mirrToAnalyzerPortConfigs.portIngrMirrorIndex[((port) / 10)]  : \
     (0x0b00b000 | (((port) / 10) * 0x4))))

/* Port Egress Mirror Index */
#define SMEM_XCAT_EGR_MIRROR_INDEX_REG(dev, port) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMAIN_NOT_VALID_CNS : \
    ((dev)->supportRegistersDb ? \
      (SMEM_CHT_MAC_REG_DB_GET(dev))->EQ.mirrToAnalyzerPortConfigs.portEgrMirrorIndex[(port) >> 6][(((port) & 0x3f) / 10)] : \
      (SKERNEL_IS_LION_REVISON_B0_DEV(dev)) ? \
        SMEM_LION_EGR_MIRROR_INDEX_REG(dev, port) : (0x0b00b010 | (((port) / 10) * 0x4))))

/* Mirror Interface Parameter Register */
#define SMEM_XCAT_MIRROR_INTERFACE_PARAM_REG(dev, index) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? (SMEM_CHT_MAC_REG_DB_SIP5_GET(dev))->EQ.mirrToAnalyzerPortConfigs.mirrorInterfaceParameterReg[index] : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->EQ.mirrToAnalyzerPortConfigs.mirrorInterfaceParameterReg[index] : \
    (0x0b00b020 | ((index) * 0x4))))

/* Analyzer Port Global Configuration */
#define SMEM_XCAT_ANALYZER_PORT_GLOBAL_CONF_REG(dev)                \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? (SMEM_CHT_MAC_REG_DB_SIP5_GET(dev))->EQ.mirrToAnalyzerPortConfigs.analyzerPortGlobalConfig : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->EQ.mirrToAnalyzerPortConfigs.analyzerPortGlobalConfig : \
    (0x0b00b040)))


/* IEEE_table_select< 0, 1 > */
#define SMEM_XCAT_IEEE_TBL_SELECT_REG_MAC(dev, port) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, ieeeTblSelect, port)

/* Unicast Routing Engine Configuration Register */
#define SMEM_XCAT_UC_ROUT_ENG_CONF_REG(dev)                         \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->ucRouter.ucRoutingEngineConfigReg : \
    (0x02800e3c))

/* Trunk Hash Configuration Register0 */
#define SMEM_XCAT_TRUNK_HASH_CONF_REGISTER0(dev) \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.trunkHashSimpleHash.trunkHashConfigReg0 : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->TTI.trunkHash.trunkHashConfigReg0 : \
    SKERNEL_IS_LION_REVISON_B0_DEV(devObjPtr) ? \
        SMEM_LION_TTI_TRUNK_HASH_CONF0_REG(dev) : (0x0c000070)))

/* Logical Target Mapping Table Access Control */
#define SMEM_XCAT_LOGICAL_TRG_MAPPING_TBL_ACCESS_REG(dev)           \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMAIN_NOT_VALID_CNS :            \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->EQ.logicalTargetMapTable.logicalTargetMapTableAccessCtrl : \
    (0x0b0000a0)))

/* Logical Target Mapping Table Data Access */
#define SMEM_XCAT_LOGICAL_TRG_MAPPING_TBL_DATA_REG(dev)             \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMAIN_NOT_VALID_CNS :            \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->EQ.logicalTargetMapTable.logicalTargetMapTableDataAccess : \
    (0x0b00a000)))

/* Logical Target Mapping Table Data Access 2 */
#define SMEM_XCAT_LOGICAL_TRG_MAPPING_TBL_DATA2_REG(dev)             \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMAIN_NOT_VALID_CNS :            \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->EQ.logicalTargetMapTable.logicalTargetMapTableDataAccess2 : \
    (0x0b00a004)))


/* Logical Target Device Mapping Configuration */
#define SMEM_XCAT_LOGICAL_TRG_DEV_MAPPING_CONF_REG(dev)             \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMAIN_NOT_VALID_CNS : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->EQ.preEgrEngineGlobalConfig.logicalTargetDeviceMapConfig : \
    (0x0b050004)))

/* Ingress/Egress Policer Metering Memory */
#define SMEM_XCAT_POLICER_METER_ENTRY_TBL_MEM(dev, direction, cycle, index) \
     ((direction) == SMAIN_DIRECTION_INGRESS_E) ?                  \
        SMEM_XCAT_INGR_POLICER_METER_ENTRY_TBL_MEM(dev, cycle, index) : \
        SMEM_XCAT_EGR_POLICER_METER_ENTRY_TBL_MEM(dev, index)

/* Ingress/Egress Policer Metering Memory */
#define SMEM_XCAT_POLICER_METER_ENTRY_TBL_MEM_ANY(dev, cycle, index) \
     ((cycle) < 2) ?                  \
        SMEM_XCAT_INGR_POLICER_METER_ENTRY_TBL_MEM(dev, cycle, index) : \
        SMEM_XCAT_EGR_POLICER_METER_ENTRY_TBL_MEM(dev, index)


 /* Ingress/Egress Policer Counter Memory */
#define SMEM_XCAT_POLICER_CNT_ENTRY_TBL_MEM(dev, direction, cycle, index) \
     ((direction) == SMAIN_DIRECTION_INGRESS_E) ?                  \
            ( smemChtPolicerTableIndexCheck(dev, cycle, index) ?                 \
             SMEM_XCAT_INGR_POLICER_CNT_ENTRY_REG(dev, index, cycle) : SMAIN_NOT_VALID_CNS ) : \
             SMEM_XCAT_EGR_POLICER_CNT_ENTRY_REG(dev, index)

/* Ingress/Egress Policer Port metering pointer index */
#define SMEM_XCAT_POLICER_PORT_METER_POINTER_TBL_MEM(dev, cycle, index) \
    SMEM_TABLE_ENTRY_INDEX_DUP_TBL_GET_MAC(dev, policerMeterPointer, index, cycle)

/* Logical Target Mapping Table */
#define SMEM_XCAT_LOGICAL_TRG_MAPPING_TBL_MEM(dev, trg_dev, trg_port)         \
    SMEM_TABLE_ENTRY_2_PARAMS_INDEX_GET_MAC(dev, logicalTrgMappingTable, trg_dev, trg_port)

/* Ingress Policer IPFIX nano timer stamp upload */
#define SMEM_XCAT_POLICER_IPFIX_NANO_TS_UPLOAD_REG(dev, cycle) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->PLR[cycle].IPFIXNanoTimerStampUpload : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->PLR[cycle].IPFIXNanoTimerStampUpload : \
    SMEM_XCAT_POLICER_OFFSET_MAC(dev, cycle , 0x00000018)))


/* Ingress/Egress IPFIX wrap around alert Memory */
#define SMEM_XCAT_POLICER_IPFIX_WA_ALERT_TBL_MEM(dev, cycle, index) \
    SMEM_TABLE_ENTRY_INDEX_DUP_TBL_GET_MAC(dev, policerIpfixWaAlert, index, cycle)

/* Ingress/Egress IPFIX aging alert Memory */
#define SMEM_XCAT_POLICER_IPFIX_AGING_ALERT_TBL_MEM(dev, cycle, index) \
    SMEM_TABLE_ENTRY_INDEX_DUP_TBL_GET_MAC(dev, policerIpfixAgingAlert, index, cycle)

/* Policer Interrupt Mask */
#define SMEM_XCAT_POLICER_IPFIX_INT_MASK_REG(dev, cycle) \
    ((SMEM_XCAT_POLICER_IPFIX_INT_CAUSE_REG(dev, cycle)) + 0x4)

/* Policer Timer Memory */
#define SMEM_XCAT_POLICER_TIMER_TBL_MEM(dev, cycle, index)  \
    SMEM_TABLE_ENTRY_INDEX_DUP_TBL_GET_MAC(dev, policerTimer, index, cycle)

/* Policer Table Access Data */
#define SMEM_XCAT_POLICER_INDIRECT_ACCESS_DATA_ENTRY_REG(dev, cycle, entry_index) \
    SMEM_TABLE_ENTRY_INDEX_DUP_TBL_GET_MAC(dev, policerTblAccessData, entry_index, cycle)

/* DSA configuration and fast stack register */
#define SMEM_XCAT_FAST_STACK_REG(dev) \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.DSAConfig : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->TTI.DSAConfig  : \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev) ? \
        (0x01000060) : (0x0C000060)))

/* Loop Port<%n> */
#define SMEM_XCAT_LOOP_PORT_REG(dev)\
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMAIN_NOT_VALID_CNS : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->TTI.loopPort[0]    : \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev) ? \
        (0x01000064) : (0x0C000064)))

/* TO ANALYZER VLAN Tag Add Enable Configuration Register */
#define SMEM_XCAT_TO_ANALYZER_VLAN_TAG_ADD_EN_REG(dev)              \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMAIN_NOT_VALID_CNS : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->HA.toAnalyzerVLANTagAddEnableConfig[0] : \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev) ? 0x0E800400 :         \
         0x0F000404))

/* HA : Ingress Analyzer VLAN Tag Configuration */
#define SMEM_XCAT_HA_INGR_ANALYZER_VLAN_TAG_CONF_REG(dev)              \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->HA.ingrAnalyzerVLANTagConfig : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->HA.ingrAnalyzerVLANTagConfig   : \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev) ? 0x0E800408 :         \
         0x0F000408))

/* Egress Analyzer VLAN Tag Configuration */
#define SMEM_XCAT_HA_EGR_ANALYZER_VLAN_TAG_CONF_REG(dev)               \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->HA.egrAnalyzerVLANTagConfig : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->HA.egrAnalyzerVLANTagConfig    : \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev) ? 0x0E80040C :         \
         0x0F00040C))

/* Metal Fix */
#define SMEM_XCAT_MG_METAL_FIX_REG(dev)                             \
    ((dev)->supportRegistersDb ? SMAIN_NOT_VALID_CNS    : \
    (0x00000054))


/* Egress Analyzer VLAN Tag Configuration */
#define SMEM_XCAT_HA_QOS_PROFILE_TO_EXP_REG(dev)               \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMAIN_NOT_VALID_CNS : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->HA.qoSProfileToEXPReg[0]   : \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev) ? 0x0E800300 : \
         0x0F000300))

/* Port<4n...4n+3> TrunkNum Configuration Register<%n> - the register holds trunk ID per egress port */
#define SMEM_XCAT_HA_TRUNK_ID_FOR_PORT_BASE_REG(dev, port)   \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMAIN_NOT_VALID_CNS : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->HA.portTrunkNumConfigReg[(port) >> 2]   : \
    (SKERNEL_IS_LION_REVISON_B0_DEV(dev) ?              \
          0xE80002C : 0xF00002C )  + (((port)/4) * 4)))

/* CPID Register1*/
#define SMEM_XCAT_HA_CPID_1_REG(dev)                         \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMAIN_NOT_VALID_CNS : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->HA.CPIDReg1   : \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev) ?               \
          0xE800434 : 0xF00042C ))

/* TTI MPLS Ethertypes Configuration Register */
#define SMEM_XCAT_TTI_MPLS_ETHERTYPES_REG(dev)                   \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.MPLSEtherTypes : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->TTI.MPLSEtherTypes   : \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev) ? 0x01000030 :         \
         0x0C000030))

/* TTI IPv6 Extension Value Register */
#define SMEM_XCAT_TTI_IPV6_EXTENSION_VALUE_REG(dev)                   \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.IPv6ExtensionValue : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->TTI.IPv6ExtensionValue   : \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev) ? 0x01000038 :         \
         0x0C000038))

/* HA MPLS Ethertypes Configuration Register */
#define SMEM_XCAT_HA_MPLS_ETHERTYPES_REG(dev)                \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->HA.MPLSEtherType : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->HA.MPLSEthertype   : \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev) ?               \
          0xE800550 : 0xF000480 ))

/* TTI IPv4 GRE Ethertype */
#define SMEM_XCAT_TTI_IPV4_GRE_ETHERTYPE_REG(dev)                   \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.TTIIPv4GREEthertype : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->TTI.TTIIPv4GREEthertype : \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev) ?                           \
          0x01000018 : 0x0C000018 ))

/* XG/XLG Port MAC Control Register3 */
#define SMEM_XCAT_XG_MAC_CONTROL3_REG(dev, port)                   \
     ((dev)->supportRegistersDb ? ((SMEM_CHT_IS_SIP5_GET(dev) == 0) && (port) == SNET_CHT_CPU_PORT_CNS) ? \
            SMAIN_NOT_VALID_CNS : \
            (SMEM_CHT_MAC_REG_DB_GET(dev))->GOP.ports.XLGIP[(port)].portMACCtrlReg[3]  : \
      SKERNEL_IS_LION2_DEV(dev) ?                                                  \
      (CHT_MAC_PORTS_BASE_ADDRESS_CNS(dev,port) + 0x1c + (0x1000 * (port))) :      \
      (CHT_MAC_PORTS_BASE_ADDRESS_CNS(dev,port) + 0x1c + ( 0x400 * (port)))        \
     )

/* GE Port MAC Control Register3 */
#define SMEM_XCAT_GE_MAC_CONTROL3_REG(dev, port)                   \
     ((dev)->supportRegistersDb ? ((port) == SNET_CHT_CPU_PORT_CNS) ? \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->GOP.ports.gigPort63CPU.portMACCtrlReg[3] : \
            (IS_CHT_HYPER_GIGA_PORT(dev,port)) ? \
                SMAIN_NOT_VALID_CNS  : \
                (SMEM_CHT_MAC_REG_DB_GET(dev))->GOP.ports.gigPort[(port)].portMACCtrlReg[3]  : \
      SKERNEL_IS_LION2_DEV(dev) ?                                                  \
      (CHT_MAC_PORTS_BASE_ADDRESS_CNS(dev,port) + 0x48 + (0x1000 * (port))) :      \
      (CHT_MAC_PORTS_BASE_ADDRESS_CNS(dev,port) + 0x48 + ( 0x400 * (port)))        \
     )

/* L2MLL - virtual port to mll mapping. */
#define SMEM_XCAT_L2_MLL_VIRTUAL_PORT_TO_MLL_MAPPING_TBL_MEM(dev, index) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, l2MllVirtualPortToMllMapping, index)

/* L2MLL - vidx to mll mapping. */
#define SMEM_XCAT_L2_MLL_VIDX_TO_MLL_MAPPING_TBL_MEM(dev, index) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, l2MllVidxToMllMapping, index)

/* EQ - LP Egress VLAN member table. */
#define SMEM_XCAT_EQ_LOGICAL_PORT_EGRESS_VLAN_MEMBER_TBL_MEM(dev, index) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, eqLogicalPortEgressVlanMember, index)

/* EQ - VLAN mapping table. */
#define SMEM_XCAT_EQ_VLAN_MAPPING_TABLE_TBL_MEM(dev, index) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, eqVlanMapping, index)

/*EQ - vlan Egress Filter Counter */
#define SMEM_XCAT_EQ_VLAN_EGRESS_FILTER_COUNTER_REG(dev)                       \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMAIN_NOT_VALID_CNS : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->EQ.vlanEgressFilter.vlanEgressFilterCounter   : \
         0x0B000074))

/* EQ - egress Filter Vlan Map table -- indirect data register */
#define SMEM_XCAT_EQ_EGRESS_FILTER_VLAN_MAP_TBL_DATA_REG(dev)             \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMAIN_NOT_VALID_CNS : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->EQ.egressFilterVlanMap.egressFilterVlanMapTableDataAccess : \
    (0x0b0000A8)))

/* L2MLL VIDX enable table -- indirect data register */
#define SMEM_XCAT_L2_MLL_VIDX_ENABLE_TBL_DATA_REG(dev)             \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->MLL.l2MllVidxEnableTable.l2MllVidxEnableTableDataAccess : \
    (0x0D800460))

/* L2MLL pointer map table -- indirect data register */
#define SMEM_XCAT_L2_MLL_POINTER_MAP_TBL_DATA_REG(dev)             \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->MLL.l2MllPointerMapTable.l2MllPointerMapTableDataAccess : \
    (0x0D800470))

/* EQ - Egress Filter Vlan Member Table -- indirect data register */
#define SMEM_XCAT_EQ_EGRESS_FILTER_VLAN_MEMBER_TABLE_TBL_DATA_REG(dev)             \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMAIN_NOT_VALID_CNS : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->EQ.egressFilterVlanMemberTable.egressFilterVlanMemberTableDataAccess : \
    (0x0b0000B0)))


/* TTI - Pseudo Wire Configurations */
#define SMEM_XCAT_TTI_PSEUDO_WIRE_CONFIG_CONF_REG(dev)                     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.pseudoWire.PWE3Config : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->TTI.pseudoWireConfig : \
         0x0C000034))

/* TTI - Ingress Port Tag0/Tag1 TPID - for ethernet passenger .*/
#define SMEM_XCAT_PASSENGER_INGR_PORT_TPID_REG(dev)                            \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMAIN_NOT_VALID_CNS : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->TTI.VLANAssignment.passengerIngrTPIDSelect[0]   : \
         0x0c000524))

/* HA - tunnel start Egress Port Tag TPID */
#define SMEM_XCAT_HA_TS_EGR_PORT_TPID_REG(dev)                       \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMAIN_NOT_VALID_CNS : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->HA.portTsTagTpidSelect[0]   : \
         0x0f000710))

/* HA - Egress global TPID */
#define SMEM_XCAT_HA_TS_EGR_GLOBAL_TAG_TPID_REG(dev)                      \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMAIN_NOT_VALID_CNS : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->HA.tsTPIDConfigReg[0]   : \
         0x0f000700))


/* HA - Egress global TPID (index is 1..7) */
#define SMEM_XCAT_HA_TS_MPLS_CONTROL_WORD_REG(dev,index)                      \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->HA.PWCtrlWord[(index-1)] : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->HA.mplsControlWord[(index-1)]   : \
         0x0f000764 + ((index-1)*4)))

/* HA - Passenger TPID Index source */
#define SMEM_XCAT_PASSENGER_TPID_INDEX_SOURCE_REG(dev)                       \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMAIN_NOT_VALID_CNS : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->HA.passengerTpidIndexSource   : \
         0x0f000730))

/* Passenger TPID0_TPID1 table */
#define SMEM_XCAT_PASSENGER_TPID_CONFIG_REG(dev)                       \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMAIN_NOT_VALID_CNS : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->HA.passengerTpid0Tpid1[0]   : \
         0x0f000750))

/* HA - FROM_CPU constants to fanout devices */
#define SMEM_XCAT_HA_DSA_FROM_CPU_CONSTANTS_TO_FANOUT_DEVICES_REG(dev)                       \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMAIN_NOT_VALID_CNS : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->HA.from_cpuConstantBits   : \
         0x0f000780))

/* HA - dsa Port is Fanout Device */
#define SMEM_XCAT_HA_DSA_PORT_IS_FANOUT_DEVICE_REG(dev)                       \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMAIN_NOT_VALID_CNS : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->HA.dsaPortisFanoutDevice   : \
         0x0f000790))

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __sregXCath */

