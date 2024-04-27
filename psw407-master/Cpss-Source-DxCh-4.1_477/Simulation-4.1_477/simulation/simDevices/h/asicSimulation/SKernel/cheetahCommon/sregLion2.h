/******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* sregLion2.h
*
* DESCRIPTION:
*       Defines for Lion2 memory registers access.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 100 $
*
*******************************************************************************/
#ifndef __sregLion2h
#define __sregLion2h

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <asicSimulation/SKernel/smem/smemLion3.h>

#define SMEM_LION2_OAM_TABLE_WORDS            (4)
/* OAM table entry in bytes */
#define SMEM_LION2_OAM_ENTRY_BYTES            \
    (SMEM_LION2_OAM_TABLE_WORDS * sizeof(GT_U32))

/*
    get the base address of the OAM units
    cycle 0 - ioam 0
    cycle 1 - ioam 1
    cycle 2 - eoam

*/
#define SMEM_LION3_OAM_BASE_ADDR_MAC(dev, cycle)      \
    dev->memUnitBaseAddrInfo.oam[cycle]

/* TTI - Physical Port Attribute Table */
#define SMEM_LION2_TTI_PHYSICAL_PORT_ATTRIBUTE_TBL_MEM(dev, phyPortNum) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, ttiPhysicalPortAttribute, phyPortNum)

/* TTI - - Pre-TTI Lookup Ingress ePort Table */
#define SMEM_LION2_TTI_PRE_TTI_LOOKUP_INGRESS_EPORT_TABLE_TBL_MEM(dev, ePort) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, ttiPreTtiLookupIngressEPort, ePort)

/* TTI - TTI Unit Global Configurations for CC Ports */
#define SMEM_LION2_TTI_UNIT_GLOBAL_CONF_FOR_CC_PORTS_REG(dev)        \
    SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.TTIUnitGlobalConfigs.TTIUnitGlobalConfigForCCPorts

/* TTI - POST-TTI Lookup Ingress ePort Table */
#define SMEM_LION2_TTI_POST_TTI_LOOKUP_INGRESS_EPORT_TABLE_TBL_MEM(dev, ePort) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, ttiPostTtiLookupIngressEPort, ePort)

/*TTI - trill global config 0 */
#define SMEM_LION2_TTI_TRILL_GLOBAL_CONFIG0_REG(dev)        \
    SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.TRILLConfigs.TRILLGlobalConfigs0

/*TTI - trill global config 1 */
#define SMEM_LION2_TTI_TRILL_GLOBAL_CONFIG1_REG(dev)        \
    SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.TRILLConfigs.TRILLGlobalConfigs1

/*TTI - trill global config 2 */
#define SMEM_LION2_TTI_TRILL_GLOBAL_CONFIG2_REG(dev)        \
    SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.TRILLConfigs.TRILLGlobalConfigs2

/*TTI - TRILL General Exception Commands - register 0 */
#define SMEM_LION2_TTI_TRILL_GENERAL_EXCEPTION_COMMANDS_0_REG(dev)        \
    SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.TRILLConfigs.TRILLExceptionCommands0

/*TTI - TRILL General Exception Commands - register 1 */
#define SMEM_LION2_TTI_TRILL_GENERAL_EXCEPTION_COMMANDS_1_REG(dev)        \
    SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.TRILLConfigs.TRILLExceptionCommands1

/* TTI - TRILL adjacency TCAM table */
#define SMEM_LION2_TTI_TRILL_ADJACENCY_TCAM_TBL_MEM(dev, index) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, ttiTrillAdjacencyTcam, index)

/* TTI - TRILL RBID table */
#define SMEM_LION2_TTI_TRILL_RBID_TBL_MEM(dev, index) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, ttiTrillRbid, index)

/* Bridge - Ingress ePort Table*/
#define SMEM_LION2_BRIDGE_INGRESS_EPORT_ATTRIBUTE_TBL_MEM(dev, ePort) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, bridgeIngressEPort, ePort)

/* Bridge - Ingress ePort learn prio Table*/
#define SMEM_LION3_BRIDGE_INGRESS_EPORT_LEARN_PRIO_TBL_MEM(dev, ePort) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, bridgeIngressEPortLearnPrio, ePort)

/* Bridge - Ingress trunk Table*/
#define SMEM_LION3_BRIDGE_INGRESS_TRUNK_TBL_MEM(dev, ePort) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, bridgeIngressTrunk, ePort)

/* Bridge - Ingress physical Port Table*/
#define SMEM_LION2_BRIDGE_INGRESS_PHYSICAL_PORT_TBL_MEM(dev, physicalPort) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, bridgePhysicalPortEntry, physicalPort)

/* Bridge - Ingress physical Port Rate Limit Counters Table*/
#define SMEM_LION2_BRIDGE_INGRESS_PHYSICAL_PORT_RATE_LIMIT_COUNTERS_TBL_MEM(dev, physicalPort) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, bridgePhysicalPortRateLimitCountersEntry, physicalPort)

/* Bridge - Ingress Port Membership Table*/
#define SMEM_LION2_BRIDGE_INGRESS_PORT_MEMBERSHIP_TBL_MEM(dev, eVlan) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, bridgeIngressPortMembership, eVlan)

/* Bridge - Ingress Span State Group Index Table*/
#define SMEM_LION2_BRIDGE_INGRESS_SPAN_STATE_GROUP_INDEX_TBL_MEM(dev, eVlan) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, ingressSpanStateGroupIndex, eVlan)

/* FDB - FDB action general */
#define SMEM_LION3_FDB_FDB_ACTION_GENERAL_REG(dev)                                \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->FDB.FDBCore.FDBAction.FDBActionGeneral : \
     SMAIN_NOT_VALID_CNS)

/* FDB - FDB action 0 */
#define SMEM_LION3_FDB_FDB_ACTION_0_REG(dev)                                \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->FDB.FDBCore.FDBAction.FDBAction0 : \
     SMAIN_NOT_VALID_CNS)

/* FDB - FDB action 1 */
#define SMEM_LION3_FDB_FDB_ACTION_1_REG(dev)                                \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->FDB.FDBCore.FDBAction.FDBAction1 : \
     SMAIN_NOT_VALID_CNS)

/* FDB - FDB action 2 */
#define SMEM_LION3_FDB_FDB_ACTION_2_REG(dev)                                \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->FDB.FDBCore.FDBAction.FDBAction2 : \
     SMAIN_NOT_VALID_CNS)


/* FDB - FDB action 3 */
#define SMEM_LION3_FDB_FDB_ACTION_3_REG(dev)                                \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->FDB.FDBCore.FDBAction.FDBAction3 : \
     SMAIN_NOT_VALID_CNS)


/* FDB - FDB action 4 */
#define SMEM_LION3_FDB_FDB_ACTION_4_REG(dev)                                \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->FDB.FDBCore.FDBAction.FDBAction4 : \
     SMAIN_NOT_VALID_CNS)

/* FDB metal fix register */
#define SMEM_LION2_FDB_METAL_FIX_REG(dev)                           \
    ((dev)->supportRegistersDb ?                                    \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->FDB.FDBInternal.metalFix :  \
    (SMAIN_NOT_VALID_CNS))


/* L2MLL - global control register */
#define SMEM_LION3_L2_MLL_GLB_CONTROL_REG(dev) \
    ((dev)->supportRegistersDb ?               \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->temp._SMEM_LION3_L2_MLL_GLB_CONTROL_REG :  \
    (0x14000000))


/* L2MLL - Lookup Translation Table (LTT). */
#define SMEM_LION3_L2_MLL_LTT_TBL_MEM(dev, lttIndex) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, l2MllLtt, lttIndex)

/*L2MLL - Global ePort info configuration*/
#define SMEM_LION3_L2_MLL_GLOBAL_E_PORT_REG(dev) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->MLL.MLLGlobalCtrl.globalEPortRangeConfig : \
    SMAIN_NOT_VALID_CNS)

/*L2MLL - Global ePort info configuration*/
#define SMEM_LION2_L3_MLL_GLOBAL_E_PORT_MASK_REG(dev) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->MLL.MLLGlobalCtrl.globalEPortRangeMask : \
    SMAIN_NOT_VALID_CNS)

/* L2MLL Global TTL Exception Configuration */
#define SMEM_LION3_L2_MLL_GLOBAL_TTL_EXCEPTION_CONFIG_REG(dev) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->MLL.MLLGlobalCtrl.globalTTLExceptionConfig : \
    SMAIN_NOT_VALID_CNS)

/* L2MLL Valid Processed Entries Counter counters (3 counters) */
#define SMEM_LION3_L2_MLL_VALID_PROCESSED_ENTRIES_COUNTER_REG(dev,index) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->MLL.MLLOutInterfaceCntrs.L2MLLValidProcessedEntriesCntr[index-1] : \
    SMAIN_NOT_VALID_CNS)

/*L2 MLL Exception Counter - Skip Counter*/
/* NOTE: 0x0d800018 is address of BC+ device. */
#define SMEM_LION3_L2_MLL_EXCEPTION_COUNTER_SKIP_COUNTER_REG(dev)   \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->MLL.MLLOutInterfaceCntrs.L2MLLSkippedEntriesCntr : \
    0x0d800018)/* 0x0d800018 : address valid for BC+ only */

/*L2 MLL Silent Drop Counter */
#define SMEM_LION3_L2_MLL_SILENT_DROP_COUNTER_REG(dev)   \
    (SMEM_CHT_IS_SIP5_GET(dev)  ?  SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->MLL.MLLOutInterfaceCntrs.L2MLLSilentDropCntr : \
    SMAIN_NOT_VALID_CNS)

/*L2 MLL Exception Counter - TTL Exception Counter*/
#define SMEM_LION3_L2_MLL_EXCEPTION_COUNTER_TTL_EXCEPTION_COUNTER_REG(dev)   \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->MLL.MLLOutInterfaceCntrs.L2MLLTTLExceptionCntr : \
    SMAIN_NOT_VALID_CNS)

/* EQ - Ingress ePort Table*/
#define SMEM_LION2_EQ_INGRESS_EPORT_TBL_MEM(dev, ePort) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, eqIngressEPort, ePort)

/* EQ - CPU Target Device Configuration Register 2 */
#define SMEM_LION2_EQ_TRG_DEV_CONF2_REG(dev)    \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EQ.CPUTargetDeviceConfig.CPUTargetDeviceConfigReg2 : \
        SMAIN_NOT_VALID_CNS)

/* EQ - CPU Target Device Configuration Register 3 */
#define SMEM_LION2_EQ_TRG_DEV_CONF3_REG(dev)    \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EQ.CPUTargetDeviceConfig.CPUTargetDeviceConfigReg3 : \
        SMAIN_NOT_VALID_CNS)

/* EQ - L2 ECMP ePort Range Configuration : value */
#define SMEM_LION2_EQ_L2_ECMP_EPORT_VALUE_REG(dev) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EQ.L2ECMP.ePortECMPEPortValue : \
        SMAIN_NOT_VALID_CNS)

/* EQ - L2 ECMP ePort Range Configuration : value */
#define SMEM_LION2_EQ_L2_ECMP_EPORT_MASK_REG(dev) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EQ.L2ECMP.ePortECMPEPortMask : \
        SMAIN_NOT_VALID_CNS)

/* EQ - L2 ECMP Index Base ePort */
#define SMEM_LION2_EQ_L2_ECMP_INDEX_BASE_EPORT_REG(dev) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EQ.L2ECMP.ePortECMPLTTIndexBaseEPort : \
        SMAIN_NOT_VALID_CNS)

/* EQ - Trunk LTT table */
#define SMEM_LION2_EQ_TRUNK_LTT_TBL_MEM(dev, index) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, eqTrunkLtt, index)

/* EQ - L2 ECMP LTT table */
#define SMEM_LION2_EQ_L2_ECMP_LTT_TBL_MEM(dev, index) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, eqL2EcmpLtt, index)

/* EQ - L2 ECMP table */
#define SMEM_LION2_EQ_L2_ECMP_TBL_MEM(dev, index) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, eqL2Ecmp, index)

/* EQ - E2PHY table */
#define SMEM_LION2_EQ_E2PHY_TBL_MEM(dev, index) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, eqE2Phy, index)

/* EQ - Physical Port Ingress Mirror Index table */
#define SMEM_LION2_EQ_PHYSICAL_PORT_INGRESS_MIRROR_INDEX_TBL_MEM(dev, index) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, eqPhysicalPortIngressMirrorIndexTable, index)

/* EQ - ePort ECMP Hash Bit Selection Config */
#define SMEM_LION2_EQ_EPORT_ECMP_HASH_BIT_SELECTION_CONFIG_REG(dev) \
(SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EQ.L2ECMP.ePortECMPHashBitSelectionConfig : \
SMAIN_NOT_VALID_CNS)

/* EQ - trunk Hash Bit Selection Config */
#define SMEM_LION2_EQ_TRUNK_HASH_BIT_SELECTION_CONFIG_REG(dev) \
(SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EQ.L2ECMP.trunkHashBitSelectionConfig : \
SMAIN_NOT_VALID_CNS)


/* EQ -  Ingress STC Interrupt Cause summary Register  */
#define SMEM_LION3_EQ_INGRESS_STC_INT_SUMMARY_CAUSE_REG(dev)                          \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EQ.ingrSTCInterrupt.ingrSTCInterruptSummary : \
        SMAIN_NOT_VALID_CNS)

/* EQ -  Ingress STC Interrupt Mask summary Register  */
#define SMEM_LION3_EQ_INGRESS_STC_INT_SUMMARY_MASK_REG(dev)                          \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EQ.ingrSTCInterrupt.ingrSTCInterruptSummaryMask : \
        SMAIN_NOT_VALID_CNS)

/* EQ - EQ To EGF Hash Bit Selection Configuration*/
#define SMEM_LION3_EQ_TO_EGF_HASH_BIT_SELECTION_CONFIG_REG(dev) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EQ.preEgrEngineGlobalConfig.cscdTrunkHashBitSelectionConfig : \
        SMAIN_NOT_VALID_CNS)

/* EQ - pre egress engine - target ePort MTU global configuration*/
#define SMEM_LION3_EQ_PRE_EGRESS_ENGINE_MTU_GLOBAL_CONFIG_REG(dev) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EQ.preEgrEngineGlobalConfig.MTUGlobalConfig : \
        SMAIN_NOT_VALID_CNS)

/* EQ - pre egress engine - MTU exceeded counter */
#define SMEM_LION3_EQ_PRE_EGRESS_ENGINE_MTU_EXCEED_CNTR_REG(dev) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EQ.preEgrEngineGlobalConfig.targetEPortMTUExceededCntr : \
        SMAIN_NOT_VALID_CNS)

/* EQ - pre egress engine - MTU size per profile [0..3] - index is profile index*/
#define SMEM_LION3_EQ_PRE_EGRESS_MTU_SIZE_PER_PROFILE_REG(dev,index)     \
        (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EQ.preEgrEngineGlobalConfig.MTUTableEntry[index] : \
        SMAIN_NOT_VALID_CNS)

/* EQ - l2 ecmp trunk LFSR Configuration register */
#define SMEM_LION3_EQ_L2_ECMP_TRUNK_LFSR_CONFIG_REG(dev) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EQ.L2ECMP.trunkLFSRConfig : \
        SMAIN_NOT_VALID_CNS)


/* EQ - ePort - Global Configuration register */
#define SMEM_LION3_EQ_EPORT_GLOBAL_CONFIG_REG(dev) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EQ.ePort.ePortGlobalConfig : \
        SMAIN_NOT_VALID_CNS)

/* EQ - ePort - Protection LOC Write Mask register */
#define SMEM_LION3_EQ_EPORT_PROTECTION_LOC_WRITE_MASK_REG(dev) \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EQ.ePort.protectionLOCWriteMask : \
        SMAIN_NOT_VALID_CNS)

/* EQ - ePort - Protection Switching RX Exception Configuration register */
#define SMEM_LION3_EQ_EPORT_PROTECTION_SWITCHING_RX_EXCEPTION_CONFIG_REG(dev) \
    (SMEM_CHT_IS_SIP5_GET(dev) ?  SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EQ.ePort.protectionSwitchingRXExceptionConfig : \
        SMAIN_NOT_VALID_CNS)

/* EQ: ePort to LOC Mapping Table, index 0..4k */
#define SMEM_LION3_EPORT_TO_LOC_MAPPING_TBL_MEM(dev, index) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, ePortToLocMappingTable, index)

/* EQ: Tx Protection Switching Table, index 0..255 */
#define SMEM_LION3_TX_PROTECTION_SWITCHING_TBL_MEM(dev, index) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, txProtectionSwitchingTable, index)


/* HA - Egress EPort Attribute Table 1 */
#define SMEM_LION2_HA_EGRESS_EPORT_ATTRIBUTE_1_TBL_MEM(dev, ePort) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, haEgressEPortAttr1, ePort)

/* HA - Egress EPort Attribute Table 2 */
#define SMEM_LION2_HA_EGRESS_EPORT_ATTRIBUTE_2_TBL_MEM(dev, ePort) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, haEgressEPortAttr2, ePort)

/* HA - Physical port 1 HA Attributes */
#define SMEM_LION2_HA_PHYSICAL_PORT_1_ATTRIBUTES_TBL_MEM(dev, ePort) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, haEgressPhyPort1, ePort)

/* HA - Physical port 2 HA Attributes */
#define SMEM_LION2_HA_PHYSICAL_PORT_2_ATTRIBUTES_TBL_MEM(dev, ePort) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, haEgressPhyPort2, ePort)

/* HA - TRILL ethertype Configuration*/
#define SMEM_LION2_HA_TRILL_ETHERTYPE_CONFIG_REG(dev)        \
        (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->HA.TRILLEtherType : \
        SMAIN_NOT_VALID_CNS)


/* HA - Timestamp Tag EtherType Register */
#define SMEM_LION3_HA_TST_ETHERTYPE_REG(dev)        \
        (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->HA.PTPTimestampTagEthertype : \
        SMAIN_NOT_VALID_CNS)

/* HA - PTP and Timestamping Exception Configuration Register */
#define SMEM_LION3_HA_PTP_AND_TS_EXCEPTION_CONFIG_REG(dev)        \
        (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->HA.PTPAndTimestampingExceptionConfig : \
        SMAIN_NOT_VALID_CNS)

/* HA - Invalid PTP Header Counter Register */
#define SMEM_LION3_HA_INVALID_PTP_HEADER_CNTR_REG(dev)        \
        (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->HA.invalidPTPHeaderCntr : \
        SMAIN_NOT_VALID_CNS)

/* HA - Invalid PTP Outgoing Piggyback Counter Register */
#define SMEM_LION3_HA_INVALID_PTP_OUT_PIGGYBACK_CNTR_REG(dev)        \
        (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->HA.invalidPTPOutgoingPiggybackCntr : \
        SMAIN_NOT_VALID_CNS)

/* HA - Invalid PTP Incoming Piggyback Counter Register */
#define SMEM_LION3_HA_INVALID_PTP_IN_PIGGYBACK_CNTR_REG(dev)        \
        (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->HA.invalidPTPIncomingPiggybackCntr : \
        SMAIN_NOT_VALID_CNS)

/* HA - Interrupt Cause register */
#define SMEM_LION3_HA_INTERRUPT_CAUSE_REG(dev)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->HA.HAInterruptCause : \
                                 SMAIN_NOT_VALID_CNS)

/* HA - Interrupt Mask register */
#define SMEM_LION3_HA_INTERRUPT_MASK_REG(dev)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->HA.HAInterruptMask : \
                                 SMAIN_NOT_VALID_CNS)

/* HA - PTP Domain table*/
#define SMEM_LION3_HA_PTP_DOMAIN_TBL_MEM(dev, port, domain) \
    SMEM_TABLE_ENTRY_2_PARAMS_INDEX_GET_MAC(dev, haPtpDomain, port, domain)

/* HA - DSA ERSPAN, where index (0-7) represents entry - word 1 */
#define SMEM_LION2_HA_DSA_ERSPAN_REG(dev, index) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMAIN_NOT_VALID_CNS : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->HA.dsaErspan[index]:\
    (0x0E8005D4 + ((index)*0x18) )))

/* TXQ-distributor : Device Map Table Address Construct */
#define SMEM_LION2_TXQ_DISTRIBUTOR_DEV_MAP_MEM_CONSTRUCT_REG(dev)           \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMAIN_NOT_VALID_CNS : \
    ((dev)->supportRegistersDb ?                                    \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer1.TXQ.TXQ_DIST.distDevMapTableAddrConstruct :  \
    (0x12d00200)))

/* TXQ-distributor : Local Src Port Map Own Dev En <%n> <0-1> */
#define SMEM_LION2_TXQ_DISTRIBUTOR_LOCAL_SRC_PORT_MAP_OWN_DEV_EN_REG(dev)        \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMAIN_NOT_VALID_CNS : \
    ((dev)->supportRegistersDb ?                                    \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer1.TXQ.TXQ_DIST.distLocalSrcPortMapOwnDevEn[0] : \
    (0x12d00210)))

/* TXQ-distributor : Local Trg Port Map Own Dev En <%n> <0-1> */
#define SMEM_LION2_TXQ_DISTRIBUTOR_LOCAL_TRG_PORT_MAP_OWN_DEV_EN_REG(dev)        \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMAIN_NOT_VALID_CNS : \
    ((dev)->supportRegistersDb ?                                    \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer1.TXQ.TXQ_DIST.distLocalTrgPortMapOwnDevEn[0] :  \
    (0x12d00220)))

/* TXQ-distributor : device map table */
#define SMEM_LION2_TXQ_DISTRIBUTOR_DEVICE_MAP_TABLE_ENTRY(dev, index) \
        SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev,txqDistributorDeviceMapTable,index)

/* TXQ relayed port number register */
#define SMEM_LION2_TXQ_RELAYED_PORT_NUMBER_REG(dev)      \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMAIN_NOT_VALID_CNS : \
    ((dev)->supportRegistersDb ?                        \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer1.TXQ.TXQ_EGR[0].relayedPortNumber :  \
    (0x01810100)))

/* IPVX - Ecmp Configuration Register */
#define SMEM_LION3_ECMP_CONFIG_REG(dev)        \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->IPvX.ECMPRoutingConfig.ECMPConfig : \
        SMAIN_NOT_VALID_CNS)

/* IPVX - Router Additional Control 2 */
#define SMEM_LION2_IPVX_ROUTER_ADDITIONAL_CONTROL_2_REG(dev)    \
    SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->IPvX.routerGlobalCtrl.routerGlobalCtrl2 : \
    ((dev)->supportRegistersDb ?                        \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->temp._SMEM_LION2_IPVX_ROUTER_ADDITIONAL_CONTROL_2_REG :  \
    (0x02800990))

/* IPvX - Ingress ePort Table */
#define SMEM_LION2_IPVX_INGRESS_EPORT_TBL_MEM(dev,ePort)        \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, ipvxIngressEPort, ePort)

/* IPvX - Ingress eVlan Table */
#define SMEM_LION2_IPVX_INGRESS_EVLAN_TBL_MEM(dev,eVlan)        \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, ipvxIngressEVlan, eVlan)

/* IPvX - Router Extra Management Counters Set 0,1,2,3*/
#define SMEM_LION2_IPVX_ROUTER_EXTRA_MANAGEMENT_COUNTER_CONFIGURATION_REG(dev,index)    \
    SMEM_CHT_IS_SIP5_GET(dev) ? \
            SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->IPvX.routerManagementCntrsSets.routerManagementCntrsSetConfig1[(index)] : \
    ((dev)->supportRegistersDb ?                        \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->temp._SMEM_LION2_IPVX_ROUTER_EXTRA_MANAGEMENT_COUNTER_CONFIGURATION_REG[index] :  \
    (0x02800994 + (4 * (index))))

/* EPCL - EPCL target physical Port Num to Port List Mapping 0..2 */
#define SMEM_LION2_EPCL_TARGET_PORT_NUM_TO_PORT_MAPPING_REG(dev, index) \
    ((dev)->supportRegistersDb ?               \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->temp._SMEM_LION2_EPCL_TARGET_PORT_NUM_TO_PORT_MAPPING_REG[index] :  \
    (0x0E000300 + ((index) * 4)))

/*
policer - per EPort/EVlan trigger table
NOTE: Each of the PLR flavors (IPLR0/IPLR1/EPLR) have this table
*/
#define SMEM_LION2_POLICER_EPORT_EVLAN_TRIGGER_TBL_MEM(dev,index,cycle)   \
        SMEM_TABLE_ENTRY_INDEX_DUP_TBL_GET_MAC(dev, policerEPortEVlanTrigger, index, cycle)

/* External Units Interrupts mask */
#define SMEM_LION2_XG_PORTS_SUMMARY_INTERRUPT_MASK_REG(dev, port)        \
    ((dev)->supportRegistersDb ?               \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->GOP.ports.XLGIP[port].externalUnitsInterruptsMask :  \
    (0x088c005c + ((port) * 0x1000)))


/* Server Reset Control */
#define SMEM_LION2_DFX_SERVER_RESET_CONTROL_REG(dev)        \
    (SMEM_CHT_IS_DFX_SERVER(dev)  ? SMEM_CHT_MAC_REG_DB_DFX_SERVER_GET(dev)->resetAndInitCtrller.DFXServerUnitsBC2SpecificRegs.deviceResetCtrl : \
        ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->DFX_SERVER.serverResetControl :  \
        0x018f800c))

/* Init Done Status */
#define SMEM_LION2_DFX_SERVER_INIT_DONE_STATUS_REG(dev)        \
    (SMEM_CHT_IS_DFX_SERVER(dev)  ? SMEM_CHT_MAC_REG_DB_DFX_SERVER_GET(dev)->resetAndInitCtrller.DFXServerUnits.DFXServerRegs.serverStatus : \
        ((dev)->supportRegistersDb ?               \
            (SMEM_CHT_MAC_REG_DB_GET(dev))->DFX_SERVER.initDoneStatus :  \
        0x018f8014))

/* JTAG Device IDS tatus */
#define SMEM_LION2_DFX_SERVER_JTAG_DEVICE_ID_STATUS_REG(dev)   \
    (SMEM_CHT_IS_DFX_SERVER(dev)  ?                            \
        SMEM_CHT_MAC_REG_DB_DFX_SERVER_GET(dev)->resetAndInitCtrller.DFXServerUnits.DFXServerRegs.JTAGDeviceIDStatus : \
        SMAIN_NOT_VALID_CNS)


/* OAM registers */

/* OAM Global Control */
#define SMEM_LION2_OAM_GLOBAL_CONTROL_REG(dev, stage)      \
        (SMEM_CHT_IS_SIP5_GET(dev) ?               \
            SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->OAMUnit[stage].OAMGlobalCtrl :  \
        SMAIN_NOT_VALID_CNS)

/* OAM Global Control1 */
#define SMEM_LION2_OAM_GLOBAL_CONTROL1_REG(dev, stage)      \
        (SMEM_CHT_IS_SIP5_GET(dev) ?               \
            SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->OAMUnit[stage].OAMGlobalCtrl1 :  \
        SMAIN_NOT_VALID_CNS)

/* OAM Exception configuration */
#define SMEM_LION2_OAM_EXCEPTION_CONF_REG(dev, stage)      \
        (SMEM_CHT_IS_SIP5_GET(dev) ?               \
            SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->OAMUnit[stage].OAMExceptionConfigs :  \
        SMAIN_NOT_VALID_CNS)

/* OAM Exception configuration1 */
#define SMEM_LION2_OAM_EXCEPTION_CONF1_REG(dev, stage)      \
        (SMEM_CHT_IS_SIP5_GET(dev) ?               \
            SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->OAMUnit[stage].OAMExceptionConfigs1 :  \
        SMAIN_NOT_VALID_CNS)

/* OAM Exception configuration2 */
#define SMEM_LION2_OAM_EXCEPTION_CONF2_REG(dev, stage)      \
        (SMEM_CHT_IS_SIP5_GET(dev) ?               \
            SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->OAMUnit[stage].OAMExceptionConfigs2 :  \
        SMAIN_NOT_VALID_CNS)

/*  Dual Ended Loss Measurement */
#define SMEM_LION2_OAM_DUAL_ENDED_LM_REG(dev, stage)      \
        (SMEM_CHT_IS_SIP5_GET(dev) ?               \
            SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->OAMUnit[stage].dualEndedLM :  \
        SMAIN_NOT_VALID_CNS)

/* OAM Invalid Keepalive Exception Configurations */
#define SMEM_LION2_OAM_INVALID_KEEPALIVE_EXCEPTION_CONF_REG(dev, stage)      \
        (SMEM_CHT_IS_SIP5_GET(dev) ?               \
            SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->OAMUnit[stage].OAMInvalidKeepaliveExceptionConfigs :  \
        SMAIN_NOT_VALID_CNS)

/* OAM Loss Measurement Opcodes */
#define SMEM_LION2_OAM_LOSS_MEASURMENT_OPCODE_REG(dev, stage)      \
        (SMEM_CHT_IS_SIP5_GET(dev) ?               \
            SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->OAMUnit[stage].OAMLossMeasurementOpcodes[0] :  \
        SMAIN_NOT_VALID_CNS)

/* OAM Delay Measurement Opcodes */
#define SMEM_LION2_OAM_DELAY_MEASURMENT_OPCODE_REG(dev, stage)      \
        (SMEM_CHT_IS_SIP5_GET(dev) ?               \
            SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->OAMUnit[stage].OAMDelayMeasurementOpcodes[0] :  \
        0x14800034)

/* OAM Keep Alive Opcodes */
#define SMEM_LION2_OAM_KEEP_ALIVE_OPCODE_REG(dev, stage)      \
        (SMEM_CHT_IS_SIP5_GET(dev) ?               \
            SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->OAMUnit[stage].OAMKeepAliveOpcodes[0] :  \
        SMAIN_NOT_VALID_CNS)

/* Aging Period Entry <<%n>> Low */
#define SMEM_LION2_OAM_AGING_PERIOD_ENTRY_LOW_REG(dev, stage, index)      \
        (SMEM_CHT_IS_SIP5_GET(dev) ?               \
            SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->OAMUnit[stage].agingPeriodEntryLow[index] :  \
        SMAIN_NOT_VALID_CNS)

/* Aging Period Entry <<%n>> High */
#define SMEM_LION2_OAM_AGING_PERIOD_ENTRY_HIGH_REG(dev, stage, index)      \
        (SMEM_CHT_IS_SIP5_GET(dev) ?               \
            SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->OAMUnit[stage].agingPeriodEntryHigh[index] :  \
        SMAIN_NOT_VALID_CNS)

/* OAM LM Counted Opcodes */
#define SMEM_LION2_OAM_LOSS_MEASURMENT_COUNTED_OPCODE_REG(dev, stage)      \
        (SMEM_CHT_IS_SIP5_GET(dev) ?               \
            SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->OAMUnit[stage].OAMLMCountedOpcodes[0] :  \
        SMAIN_NOT_VALID_CNS)

/* OAM Unit Interrupt Cause */
#define SMEM_LION2_OAM_INTR_CAUSE_REG(dev, stage)      \
        (SMEM_CHT_IS_SIP5_GET(dev) ?               \
            SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->OAMUnit[stage].OAMUnitInterruptCause :  \
        SMAIN_NOT_VALID_CNS)

/* OAM Unit Interrupt Mask */
#define SMEM_LION2_OAM_INTR_MASK_REG(dev, stage)       \
        (SMEM_CHT_IS_SIP5_GET(dev) ?               \
            SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->OAMUnit[stage].OAMUnitInterruptMask :  \
        SMAIN_NOT_VALID_CNS)

/* Excess Keepalive group status */
#define SMEM_LION2_OAM_EXCESS_KEEPALIVE_GROUP_STATUS_REG(dev, stage)       \
        (SMEM_CHT_IS_SIP5_GET(dev) ?               \
            SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->OAMUnit[stage].excessKeepaliveGroupStatus[0] :  \
        SMAIN_NOT_VALID_CNS)

/* Invalid Keepalive group status */
#define SMEM_LION2_OAM_INVALID_KEEPALIVE_GROUP_STATUS_REG(dev, stage)       \
        (SMEM_CHT_IS_SIP5_GET(dev) ?               \
            SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->OAMUnit[stage].invalidKeepaliveGroupStatus[0] :  \
        SMAIN_NOT_VALID_CNS)

/* MEG Level group status */
#define SMEM_LION2_OAM_MEG_LEVEL_GROUP_STATUS_REG(dev, stage)       \
        (SMEM_CHT_IS_SIP5_GET(dev) ?               \
            SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->OAMUnit[stage].MEGLevelGroupStatus[0] :  \
        SMAIN_NOT_VALID_CNS)

/* Source Interface group status */
#define SMEM_LION2_OAM_SOURCE_INTERFACE_GROUP_STATUS_REG(dev, stage)       \
        (SMEM_CHT_IS_SIP5_GET(dev) ?               \
            SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->OAMUnit[stage].sourceInterfaceGroupStatus[0] :  \
        SMAIN_NOT_VALID_CNS)

/* RDI Status Change Exception group status */
#define SMEM_LION2_OAM_RDI_STATUS_CHANGE_EXCEPTION_GROUP_STATUS_REG(dev, stage)       \
        (SMEM_CHT_IS_SIP5_GET(dev) ?               \
            SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->OAMUnit[stage].RDIStatusChangeExceptionGroupStatus[0] :  \
        SMAIN_NOT_VALID_CNS)

/* Tx Period Exception group status */
#define SMEM_LION2_OAM_TX_PERIOD_EXCEPTION_GROUP_STATUS_REG(dev, stage)       \
        (SMEM_CHT_IS_SIP5_GET(dev) ?               \
            SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->OAMUnit[stage].txPeriodExceptionGroupStatus[0] :  \
        SMAIN_NOT_VALID_CNS)

/* MEG Level Exception Counter */
#define SMEM_LION2_OAM_MEG_LEVEL_EXCEPTION_COUNTER_REG(dev, stage)         \
        (SMEM_CHT_IS_SIP5_GET(dev) ?               \
            SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->OAMUnit[stage].MEGLevelExceptionCntr :  \
        SMAIN_NOT_VALID_CNS)

/* Source Interface Mismatch Counter */
#define SMEM_LION2_OAM_SOURCE_INTERFACE_MISMATCH_COUNTER_REG(dev, stage)         \
        (SMEM_CHT_IS_SIP5_GET(dev) ?               \
            SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->OAMUnit[stage].sourceInterfaceMismatchCntr :  \
        SMAIN_NOT_VALID_CNS)

/* Invalid Keepalive Hash Counter */
#define SMEM_LION2_OAM_INVALID_KEEPALIVE_HASH_COUNTER_REG(dev, stage)         \
        (SMEM_CHT_IS_SIP5_GET(dev) ?               \
            SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->OAMUnit[stage].invalidKeepaliveHashCntr :  \
        SMAIN_NOT_VALID_CNS)

/* Excess Keepalive Counter */
#define SMEM_LION2_OAM_EXCESS_KEEPALIVE_COUNTER_REG(dev, stage)         \
        (SMEM_CHT_IS_SIP5_GET(dev) ?               \
            SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->OAMUnit[stage].excessKeepaliveCntr :  \
        SMAIN_NOT_VALID_CNS)

/* Keepalive Aging Counter */
#define SMEM_LION2_OAM_KEEPALIVE_AGING_COUNTER_REG(dev, stage)         \
        (SMEM_CHT_IS_SIP5_GET(dev) ?               \
            SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->OAMUnit[stage].keepaliveAgingCntr : \
        SMAIN_NOT_VALID_CNS)

/* RDI Status Change Counter */
#define SMEM_LION2_OAM_RDI_STATUS_CHANGE_COUNTER_REG(dev, stage)         \
        (SMEM_CHT_IS_SIP5_GET(dev) ?               \
            SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->OAMUnit[stage].RDIStatusChangeCntr : \
        SMAIN_NOT_VALID_CNS)

/* TX Period Exception Counter */
#define SMEM_LION2_OAM_TX_PERIOD_EXCEPTION_COUNTER_REG(dev, stage)         \
        (SMEM_CHT_IS_SIP5_GET(dev) ?               \
            SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->OAMUnit[stage].txPeriodExceptionCntr : \
        SMAIN_NOT_VALID_CNS)

/* Hash Bits Selection */
#define SMEM_LION2_OAM_HASH_BIT_SELECTION_CONFIG_REG(dev, stage) \
        (SMEM_CHT_IS_SIP5_GET(dev) ? \
            SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->OAMUnit[stage].hashBitsSelection : \
        SMAIN_NOT_VALID_CNS)

/* OAM Tables */

/* OAM Table */
#define SMEM_LION2_OAM_BASE_TBL_MEM(dev, stage, index) \
    SMEM_TABLE_ENTRY_INDEX_DUP_TBL_GET_MAC(dev, oamTable, index, stage)

/* Aging Table */
#define SMEM_LION2_OAM_AGING_TBL_MEM(dev, stage, index) \
    SMEM_TABLE_ENTRY_INDEX_DUP_TBL_GET_MAC(dev, oamAgingTable, index, stage)

/* MEG Exception Table */
#define SMEM_LION2_MEG_EXCEPTION_TBL_MEM(dev, stage, index) \
    SMEM_TABLE_ENTRY_INDEX_DUP_TBL_GET_MAC(dev, oamMegExceptionTable, index, stage)

/* Source Interface Exception Table */
#define SMEM_LION2_SRC_INTERFACE_EXCEPTION_TBL_MEM(dev, stage, index) \
    SMEM_TABLE_ENTRY_INDEX_DUP_TBL_GET_MAC(dev, oamSrcInterfaceExceptionTable, index, stage)

/* Invalid Keepalive Hash Table */
#define SMEM_LION2_INVALID_KEEPALIVE_HASH_TBL_MEM(dev, stage, index) \
    SMEM_TABLE_ENTRY_INDEX_DUP_TBL_GET_MAC(dev, oamInvalidKeepAliveHashTable, index, stage)

/* RDI Status Change Exception Table */
#define SMEM_LION2_RDI_STATUS_CHANGE_EXCEPTION_TBL_MEM(dev, engine, index) \
    SMEM_TABLE_ENTRY_INDEX_DUP_TBL_GET_MAC(dev, oamRdiStatusChangeExceptionTable, index, engine)

/* Excess Keepalive Table */
#define SMEM_LION2_EXCESS_KEEPALIVE_TBL_MEM(dev, stage, index) \
    SMEM_TABLE_ENTRY_INDEX_DUP_TBL_GET_MAC(dev, oamExcessKeepAliveTable, index, stage)

/* OAM Exception Summary Table */
#define SMEM_LION2_OAM_EXCEPTION_SUMMARY_TBL_MEM(dev, stage, index) \
    SMEM_TABLE_ENTRY_INDEX_DUP_TBL_GET_MAC(dev, oamExceptionSummaryTable, index, stage)

/* OAM Opcode Packet Command Table */
#define SMEM_LION2_OAM_OPCODE_PACKET_COMMAND_TBL_MEM(dev, stage, index) \
    SMEM_TABLE_ENTRY_INDEX_DUP_TBL_GET_MAC(dev, oamOpCodePacketCommandTable, index, stage)

/* OAM Tx Period Exception Table */
#define SMEM_LION2_OAM_TX_PERIOD_EXCEPTION_TBL_MEM(dev, stage, index) \
    SMEM_TABLE_ENTRY_INDEX_DUP_TBL_GET_MAC(dev, oamTxPeriodExceptionTable, index, stage)

/* EQ: OAM Protection LOC Status Table */
#define SMEM_LION2_OAM_PROTECTION_LOC_STATUS_TBL_MEM(dev, index) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, oamProtectionLocStatusTable, index)

/* ERMRK : OAM LM Offset Table */
#define SMEM_LION2_OAM_LM_OFFSET_TBL_MEM(dev, index) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, oamLmOffsetTable, index)

/* ERMRK : OAM Timestamp Offset Table */
#define SMEM_LION2_OAM_TIMESTAMP_OFFSET_TBL_MEM(dev, index) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, oamTimeStampOffsetTable, index)

/* ERMRK : PTP Local Action Table */
#define SMEM_LION3_PTP_LOCAL_ACTION_TBL_MEM(dev, domain, message, port) \
    SMEM_TABLE_ENTRY_3_PARAMS_INDEX_GET_MAC(dev, ptpLocalActionTable, domain, message, port)

/* ERMRK : PTP Target Port Table */
#define SMEM_LION3_PTP_TARGET_PORT_TBL_MEM(dev, port) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, ptpTargetPortTable, port)

/* ERMRK : Qos Map Table */
#define SMEM_LION3_QOS_MAP_TBL_MEM(dev, index) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, ERMRKQosMapTable, index)

/* Policer OAM */
#define SMEM_LION3_POLICER_OAM_REG(dev, stage)         \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->PLR[stage].policerOAM : \
    SMAIN_NOT_VALID_CNS)

#define SMEM_LION3_POLICER_GLOBAL_BILLING_CNTR_INDEXING_MODE_CONFIG_0_REG(dev, stage)         \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->PLR[stage].globalBillingCntrIndexingModeConfig0 : \
    SMAIN_NOT_VALID_CNS)

#define SMEM_LION3_POLICER_GLOBAL_BILLING_CNTR_INDEXING_MODE_CONFIG_1_REG(dev, stage)         \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->PLR[stage].globalBillingCntrIndexingModeConfig1 : \
    SMAIN_NOT_VALID_CNS)

/* Policer Metering Base Address */
#define SMEM_LION3_POLICER_METERING_BASE_ADDR_REG(dev, stage)         \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->PLR[stage].policerMeteringBaseAddr : \
    SMAIN_NOT_VALID_CNS)

/* Timestamp tag configurations */
#define SMEM_LION2_TTI_TIMESTAMP_CONFIG_REG(dev)         \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.PTP.timestampConfigs : \
        ((dev)->supportRegistersDb ?               \
            (SMEM_CHT_MAC_REG_DB_GET(dev))->TTI.timeStampTagConfiguration :  \
        0x0100002c))

/* Header Alteration Global Configuration1 */
#define SMEM_LION2_HA_GLOBAL_CONF1_REG(dev)                      \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->HA.haGlobalConfig1 : \
        SMAIN_NOT_VALID_CNS)

/* PCL : OAM RDI Configuration */
#define SMEM_LION2_PCL_OAM_RDI_CONFIGURATION_REG(dev)      \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->PCL.OAMConfigs.OAMRDIConfigs : \
        SMAIN_NOT_VALID_CNS)

/* PCL : OAM MEG Level Mode Configurations */
#define SMEM_LION2_PCL_OAM_MEG_LEVEL_MODE_CONFIGURATION_REG(dev)      \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->PCL.OAMConfigs.OAMMEGLevelModeConfigs : \
        SMAIN_NOT_VALID_CNS)

/* PCL : OAM R Flag Configurations */
#define SMEM_LION2_PCL_OAM_R_FLAG_CONFIGURATION_REG(dev)      \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->PCL.OAMConfigs.OAMRFlagConfigs : \
        SMAIN_NOT_VALID_CNS)

/* PCL : R Bit Assignment Enable */
#define SMEM_LION2_PCL_OAM_R_BIT_ASSIGN_EN_REG(dev)      \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->PCL.OAMConfigs.RBitAssignmentEnable : \
        SMAIN_NOT_VALID_CNS)

/* PCL : Channel Type %n OAM Opcode */
#define SMEM_LION2_PCL_OAM_CHANNEL_TYPE_OPCODE_REG(dev,channelType)      \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->PCL.OAMConfigs.channelTypeOAMOpcode[channelType-1] : \
        SMAIN_NOT_VALID_CNS)

/* EPCL : OAM RDI Configuration */
#define SMEM_LION2_EPCL_OAM_RDI_CONFIGURATION_REG(dev)      \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EPCL.EPCLOAMRDI : \
        SMAIN_NOT_VALID_CNS)

/* EPCL : OAM MEG Level Mode Configurations */
#define SMEM_LION2_EPCL_OAM_MEG_LEVEL_MODE_CONFIGURATION_REG(dev)      \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EPCL.EPCLOAMMEGLevelMode : \
        SMAIN_NOT_VALID_CNS)

/* EPCL : OAM Packet Detection  */
#define SMEM_LION2_EPCL_OAM_PACKET_DETECTION_REG(dev)      \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EPCL.EPCLOAMPktDetection : \
        SMAIN_NOT_VALID_CNS)

/* EPCL : Channel Type %s OAM Opcode  */
#define SMEM_LION2_EPCL_OAM_CHANNEL_TYPE_OPCODE_REG(dev,_set)      \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EPCL.EPCLChannelTypeToOAMOpcodeMap[_set] : \
        SMAIN_NOT_VALID_CNS)

/* TXDMA bufferMemoryMiscThresholdsConfig */
#define SMEM_LION2_TXDMA_ENGINE_CONFIG_BUFFER_MEMORY_MISC_CONFIG_REG(dev)      \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->TXDMA.TXDMAEngineConfig.bufferMemoryMiscThresholdsConfig

/* TXDMA Extended ports configuration register */
#define SMEM_LION2_TXDMA_ENGINE_CONFIG_EXT_PORT_CONFIG_REG(dev)      \
        ((dev)->supportRegistersDb  ? (SMEM_CHT_MAC_REG_DB_GET(dev))->TXDMA.TXDMAEngineConfig.extPortsConfigReg : \
            SMAIN_NOT_VALID_CNS)

/* convert local port to RXDMA port :
    in multi core convert to 0..15 by & 0xf
    in sip5 no convert for port 63 for multi core
*/
#define SMEM_LION2_LOCAL_PORT_TO_RX_DMA_PORT_MAC(dev,localPort) \
    (((dev)->portGroupSharedDevObjPtr && SMEM_CHT_IS_SIP5_GET(dev) && ((localPort) == SNET_CHT_CPU_PORT_CNS)) ? (localPort) : \
        (((dev)->portGroupSharedDevObjPtr) ? ((localPort) & 0xf) : \
            (localPort)))

/* get the unit index 0 or 1 of the rxDMA/txDMA/txFIFO according to the port number */
#define SMEM_DUAL_UNIT_DMA_UNIT_INDEX_GET(dev,portNum)                              \
    (((dev)->multiDataPath.supportMultiDataPath == 0) ? 0 /* unit index 0 */:       \
        smemMultiDpUnitIndexGet(dev,portNum))


/* get the relative DataPath port number */
#define SMEM_DATA_PATH_RELATIVE_PORT_GET(dev,portNum)              \
    (((dev)->multiDataPath.supportRelativePortNum == 0) ? portNum: \
        smemMultiDpUnitRelativePortGet(dev,portNum))


/* RxDMA - SCDMA %n Configuration 0 */
#define SMEM_LION3_RXDMA_SCDMA_CONFIG_0_REG(dev,RxDMA_localPhysicalPort)        \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->rxDMA[SMEM_DUAL_UNIT_DMA_UNIT_INDEX_GET(dev,RxDMA_localPhysicalPort)].singleChannelDMAConfigs.SCDMAConfig0[SMEM_DATA_PATH_RELATIVE_PORT_GET(dev,RxDMA_localPhysicalPort)] : \
        SMAIN_NOT_VALID_CNS)

/* RxDMA - SCDMA %n Configuration 1 */
#define SMEM_LION3_RXDMA_SCDMA_CONFIG_1_REG(dev,RxDMA_localPhysicalPort)        \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->rxDMA[SMEM_DUAL_UNIT_DMA_UNIT_INDEX_GET(dev,RxDMA_localPhysicalPort)].singleChannelDMAConfigs.SCDMAConfig1[SMEM_DATA_PATH_RELATIVE_PORT_GET(dev,RxDMA_localPhysicalPort)] : \
        SMAIN_NOT_VALID_CNS)

/* RxDMA - High DMA I/F Configuration */
#define SMEM_LION2_RXDMA_HIGH_DMA_IF_CONFIG_REG(dev)        \
    ((dev)->supportRegistersDb  ? SMEM_CHT_MAC_REG_DB_GET(dev)->RXDMA.WRDMABAConfigs.highDMAIFConfig : \
        SMAIN_NOT_VALID_CNS)

/* Traffic generator registers */

/* MAC DA 0 Register - 0x08980000 + p*0x1000: where p (0-12 in steps of 4) represents pkt_gen num */
#define SMEM_LION2_TG_MAC_DA_0_REG(dev, offset)         \
        ((dev)->supportRegistersDb ?               \
            (SMEM_CHT_MAC_REG_DB_GET(dev))->GOP.packGenConfig.packGenConfig[((offset)*4)].macDa0 :  \
        0x08980000)
/* MAC SA 0 Register - 0x0898000C + p*0x1000: where p (0-12 in steps of 4) represents pkt_gen num */
#define SMEM_LION2_TG_MAC_SA_0_REG(dev, offset)         \
        ((dev)->supportRegistersDb ?               \
            (SMEM_CHT_MAC_REG_DB_GET(dev))->GOP.packGenConfig.packGenConfig[((offset)*4)].macSa0 :  \
        0x0898000C)
/* EtherType Register - 0x08980018 + p*0x1000: where p (0-12 in steps of 4) represents pkt_gen num */
#define SMEM_LION2_TG_ETHERTYPE_REG(dev, offset)        \
        ((dev)->supportRegistersDb ?               \
            (SMEM_CHT_MAC_REG_DB_GET(dev))->GOP.packGenConfig.packGenConfig[((offset)*4)].etherType :  \
        0x08980018)
/* Vlan Tag Register - 0x0898001C + p*0x1000: where p (0-12 in steps of 4) represents pkt_gen num */
#define SMEM_LION2_TG_VLAN_TAG_REG(dev, offset)         \
        ((dev)->supportRegistersDb ?               \
            (SMEM_CHT_MAC_REG_DB_GET(dev))->GOP.packGenConfig.packGenConfig[((offset)*4)].vlanTag :  \
        0x0898001C)
/* Data Pattern 0 Register - 0x08980020 + p*0x1000: where p (0-12 in steps of 4) represents pkt_gen num */
#define SMEM_LION2_TG_DATA_PATTERN_REG(dev, offset)     \
        ((dev)->supportRegistersDb ?               \
            (SMEM_CHT_MAC_REG_DB_GET(dev))->GOP.packGenConfig.packGenConfig[((offset)*4)].dataPattern0 :  \
        0x08980020)
/* Packet Length Register */
#define SMEM_LION2_TG_PACKET_LENGTH_REG(dev, offset)    \
        ((dev)->supportRegistersDb ?               \
            (SMEM_CHT_MAC_REG_DB_GET(dev))->GOP.packGenConfig.packGenConfig[((offset)*4)].packetLength :  \
        0x08980040)
/* Packet Count Register - 0x08980044 + p*0x1000: where p (0-12 in steps of 4) represents pkt_gen num */
#define SMEM_LION2_TG_PACKET_COUNT_REG(dev, offset)     \
        ((dev)->supportRegistersDb ?               \
            (SMEM_CHT_MAC_REG_DB_GET(dev))->GOP.packGenConfig.packGenConfig[((offset)*4)].packetCount :  \
        0x08980044)
/* Inter Frame Gap Register - 0x08980048 + p*0x1000: where p (0-12 in steps of 4) represents pkt_gen num */
#define SMEM_LION2_TG_FRAME_GAP_REG(dev, offset)        \
        ((dev)->supportRegistersDb ?               \
            (SMEM_CHT_MAC_REG_DB_GET(dev))->GOP.packGenConfig.packGenConfig[((offset)*4)].ifg :  \
        0x08980048)
/* Control Register 0 Register - 0x08980050 + p*0x1000: where p (0-12 in steps of 4) represents pkt_gen num */
#define SMEM_LION2_TG_CONTROL_0_REG(dev, offset)          \
        ((dev)->supportRegistersDb ?               \
            (SMEM_CHT_MAC_REG_DB_GET(dev))->GOP.packGenConfig.packGenConfig[((offset)*4)].controlReg0 :  \
        0x08980050)

/* TTI User Defined Bytes Configuration Memory */
#define SMEM_LION3_TTI_UDB_CONF_MEM_REG(dev, index) \
      SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, ttiUserDefinedBytesConf, index)

/* TTI IPv6 GRE Ethertype */
#define SMEM_LION3_TTI_IPV6_GRE_ETHERTYPE_REG(dev)                   \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.TTIIPv6GREEtherType : \
                                  SMAIN_NOT_VALID_CNS)

/* TTI - IP Length Checks (for macsec) */
#define SMEM_LION3_TTI_IP_LENGTH_CHECK_REG(dev)                   \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.IPLengthChecks : \
                                  SMAIN_NOT_VALID_CNS)

/* TTI - Interrupt Cause register */
#define SMEM_LION3_TTI_INTERRUPT_CAUSE_REG(dev)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.TTIEngine.TTIEngineInterruptCause : \
                                 SMAIN_NOT_VALID_CNS)

/* TTI - Interrupt Mask register */
#define SMEM_LION3_TTI_INTERRUPT_MASK_REG(dev)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.TTIEngine.TTIEngineInterruptMask : \
                                 SMAIN_NOT_VALID_CNS)

/* TTI - TTI Packet Type %p Keys register*/
#define SMEM_LION3_TTI_PACKET_TYPE_KEY_REG(dev, index)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.TTIEngine.TTIPktTypeKeys[index] : \
                                 SMAIN_NOT_VALID_CNS)

/* TTI - pre-Route Exception Masks register */
#define SMEM_LION3_TTI_PRE_ROUTE_EXCEPTION_MASKS_REG(dev)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.preRouteExceptionMasks : \
                                 SMAIN_NOT_VALID_CNS)


/* TTI - source ID Assignment register */
#define SMEM_LION3_TTI_SOURCE_ID_ASSIGNMENT_REG(dev)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.sourceIDAssignment0 : \
                                 SMAIN_NOT_VALID_CNS)

/* TTI - source ID Assignment1 register */
#define SMEM_LION3_TTI_SOURCE_ID_ASSIGNMENT1_REG(dev)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.sourceIDAssignment1 : \
                                 SMAIN_NOT_VALID_CNS)

/* TTI - PTP 1588 v1 Domain Number register */
#define SMEM_LION3_TTI_PTP_1588_V1_DOMAIN_NUMBER_REG(dev, index, domain)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.PTP._1588V1DomainDomainNumber[index][domain] : \
                                 SMAIN_NOT_VALID_CNS)

/* TTI - PTP Exception Checking Mode Domain register */
#define SMEM_LION3_TTI_PTP_EXCEPTION_CHECK_MODE_DOMAIN_REG(dev, domain)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.PTP.PTPExceptionCheckingModeDomain[domain] : \
                                 SMAIN_NOT_VALID_CNS)

/* TTI - PTP Piggy Back TS Configuration register */
#define SMEM_LION3_TTI_PTP_PIGGY_BACK_TS_CONFIG_REG(dev, group_of_ports)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.PTP.piggyBackTSConfigs[group_of_ports] : \
                                 SMAIN_NOT_VALID_CNS)

/* TTI - PTP Configurations register */
#define SMEM_LION3_TTI_PTP_CONFIG_REG(dev)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.PTP.PTPConfigs : \
                                 SMAIN_NOT_VALID_CNS)

/* TTI - PTP Over UDP Destination Ports register */
#define SMEM_LION3_TTI_PTP_OVER_UDP_DESTINATION_PORTS_REG(dev)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.PTP.PTPOverUDPDestinationPorts : \
                                 SMAIN_NOT_VALID_CNS)

/* TTI - PTP Exceptions and CPU Code Configuration register */
#define SMEM_LION3_TTI_PTP_EXCEPTIONS_AND_CPU_CODE_CONFIG_REG(dev)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.PTP.PTPExceptionsAndCPUCodeConfig: \
                                 SMAIN_NOT_VALID_CNS)

/* TTI - PTP Ethertypes register*/
#define SMEM_LION3_TTI_PTP_ETHERTYPES_REG(dev)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.PTP.PTPEtherTypes : \
                                 SMAIN_NOT_VALID_CNS)

/* TTI - PTP Exceptions Counter register */
#define SMEM_LION3_TTI_PTP_EXCEPTIONS_COUNT_REG(dev)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.PTP.PTPExceptionsCntr : \
                                 SMAIN_NOT_VALID_CNS)

/* TTI - PTP 1588 v2 Domain Number register */
#define SMEM_LION3_TTI_PTP_1588_V2_DOMAIN_NUMBER_REG(dev)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.PTP._1588V2DomainDomainNumber : \
                                 SMAIN_NOT_VALID_CNS)

/* TTI - PTP Timestamp EtherTypes register */
#define SMEM_LION3_TTI_PTP_TS_ETHERTYPES_REG(dev)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.PTP.timestampEtherTypes : \
                                 SMAIN_NOT_VALID_CNS)

/* TTI - PTP Timestamp Configurations register */
#define SMEM_LION3_TTI_PTP_TS_CONFIG_REG(dev)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.PTP.timestampConfigs : \
                                 SMAIN_NOT_VALID_CNS)

/* TTI - PTP Packet Command table*/
#define SMEM_LION3_TTI_PTP_PACKET_CMD_TBL_MEM(dev, index) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, ptpPacketCommandTable, index)

/* TTI - ingress Tag Profile TPID Select - tag : 0 or 1 , profile : 0..7 */
#define SMEM_LION3_TTI_INGR_TAG_PROFILE_TPID_SELECT_REG(dev,tag,profile)        \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.VLANAssignment.ingrTagProfileTPIDSelect[tag][(profile) / 4] : \
        SMAIN_NOT_VALID_CNS)

/* TTI - ingress TPID Tag Type */
#define SMEM_LION3_TTI_INGR_TPID_TAG_TYPE_REG(dev)        \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.VLANAssignment.ingrTPIDTagType : \
        SMAIN_NOT_VALID_CNS)

/* TTI - Received Flow Control Packets Counter Register */
#define SMEM_LION3_TTI_RECEIVED_FLOW_CONTROL_PACKET_COUNTER_REG(dev)        \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.receivedFlowCtrlPktsCntr : \
        SMAIN_NOT_VALID_CNS)

/* TTI - Duplication Configs1 Register */
#define SMEM_LION3_TTI_DUPLICATION_CONFIGS1_REG(dev)        \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.duplicationConfigs.duplicationConfigs1 : \
        SMAIN_NOT_VALID_CNS)

/* TTI - Mpls MC DA Low Register */
#define SMEM_LION3_TTI_MPLS_MC_DA_LOW_REG(dev)        \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.duplicationConfigs.MPLSMCDALow : \
        SMAIN_NOT_VALID_CNS)

/* TTI - Mpls MC DA High Register */
#define SMEM_LION3_TTI_MPLS_MC_DA_HIGH_REG(dev)        \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.duplicationConfigs.MPLSMCDAHigh : \
        SMAIN_NOT_VALID_CNS)

/* TTI - Mpls MC DA Low Mask Register */
#define SMEM_LION3_TTI_MPLS_MC_DA_LOW_MASK_REG(dev)        \
    (SMEM_CHT_IS_SIP5_GET(dev) ?  SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.duplicationConfigs.MPLSMCDALowMask : \
        SMAIN_NOT_VALID_CNS)

/* TTI - Mpls MC DA High Mask Register */
#define SMEM_LION3_TTI_MPLS_MC_DA_HIGH_MASK_REG(dev)        \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.duplicationConfigs.MPLSMCDAHighMask : \
        SMAIN_NOT_VALID_CNS)

/**************************************** FCoE regs ***************************************************/
/* IPVX - FCoE Global Configuration */
#define SMEM_LION3_IPVX_FCOE_GLOBAL_CONFIG_REG(dev) \
    SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->IPvX.FCoEGlobalCtrl.routerFCoEGlobalConfig : \
    ((dev)->supportRegistersDb ? \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->temp._SMEM_LION3_IPVX_FCOE_GLOBAL_CONFIG_REG :\
        0x028009a4)

/* TTI FCoE Global Configuration */
#define SMEM_LION3_TTI_FCOE_GLOBAL_CONFIG_REG(dev)     \
        (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.FCOE.FCoEGlobalConfig : \
        SMAIN_NOT_VALID_CNS)
/* TTI FCoE FCoE Exception Configurations */
#define SMEM_LION3_TTI_FCOE_EXC_CONFIG_REG(dev)     \
        (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.FCOE.FCoEExceptionConfigs : \
        SMAIN_NOT_VALID_CNS)
/* TTI FCoE FCoE Exception Counters 0 */
#define SMEM_LION3_TTI_FCOE_EXC_COUNT0_REG(dev)     \
        (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.FCOE.FCoEExceptionCntrs0 : \
        SMAIN_NOT_VALID_CNS)
/* TTI FCoE FCoE Exception Counters 1 */
#define SMEM_LION3_TTI_FCOE_EXC_COUNT1_REG(dev)     \
        (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.FCOE.FCoEExceptionCntrs1 : \
        SMAIN_NOT_VALID_CNS)
/* TTI FCoE FCoE Exception Counters 2 */
#define SMEM_LION3_TTI_FCOE_EXC_COUNT2_REG(dev)     \
        (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.FCOE.FCoEExceptionCntrs2 : \
        SMAIN_NOT_VALID_CNS)

/* TTI PW Tag Mode Exception Config */
#define SMEM_LION3_TTI_PW_TAG_MODE_EXCEPTION_CONFIG_REG(dev)     \
        (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.pseudoWire.PWTagModeExceptionConfig : \
        SMAIN_NOT_VALID_CNS)

/* TTI - Mpls OAM Channel Type Profile, index range 0..14 */
#define SMEM_LION3_TTI_MPLS_OAM_CHANNEL_TYPE_PROFILE_REG(dev, index)     \
        (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.pseudoWire.MPLSOAMChannelTypeProfile[index] : \
        SMAIN_NOT_VALID_CNS)


/* tti lookup ip exception commands */
#define SMEM_LION3_TTI_LOOKUP_IP_EXCEPTION_COMMANDS_REG(dev)     \
        (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.TTIEngine.TTILookupIPExceptionCommands : \
        SMAIN_NOT_VALID_CNS)

/* tti lookup ip exception codes - 0 */
#define SMEM_LION3_TTI_LOOKUP_IP_EXCEPTION_CODES_0_REG(dev)     \
        (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.TTIEngine.TTILookupIPExceptionCodes0 : \
        SMAIN_NOT_VALID_CNS)

/* tti lookup ip exception codes - 1 */
#define SMEM_LION3_TTI_LOOKUP_IP_EXCEPTION_CODES_1_REG(dev)     \
        (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.TTIEngine.TTILookupIPExceptionCodes1 : \
        SMAIN_NOT_VALID_CNS)


/* tti lookup MPLS exception commands */
#define SMEM_LION3_TTI_LOOKUP_MPLS_EXCEPTION_COMMANDS_REG(dev)     \
        (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.TTIEngine.TTILookupMPLSExceptionCommands : \
        SMAIN_NOT_VALID_CNS)

/* tti MRU exception command */
#define SMEM_LION3_TTI_MRU_EXCEPTION_COMMAND_REG(dev)     \
        (SMEM_CHT_IS_SIP5_GET(dev) ?  SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.MRUException.MRUCommandAndCode : \
        SMAIN_NOT_VALID_CNS)

/* tti - MRU size index range 0..3 */
#define SMEM_LION3_TTI_MRU_SIZE_REG(dev, index)     \
        (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.MRUException.MRUSize[index] : \
        SMAIN_NOT_VALID_CNS)

/* tti - My physical port attributes, index range 0..255 */
#define SMEM_LION3_TTI_MY_PHY_PORT_ATTRIBUTES_REG(dev, index)     \
        (SMEM_CHT_IS_SIP5_GET(dev) ?  SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.phyPortAndEPortAttributes.myPhysicalPortAttributes[index] : \
        SMAIN_NOT_VALID_CNS)

/* tti - Remote Physical Port Assignment Global Config */
#define SMEM_LION3_TTI_REMOTE_PHY_PORT_ASSIGN_GLB_CONF_REG(dev)     \
        (SMEM_CHT_IS_SIP5_GET(dev) ?  SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.phyPortAndEPortAttributes.remotePhysicalPortAssignmentGlobalConfig: \
        SMAIN_NOT_VALID_CNS)

/* tti lookup key segment mode 0 */
#define SMEM_LION3_TTI_LOOKUP_KEY_SEGMENT_MODE_0_REG(dev)     \
        (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.TTIEngine.TTIKeySegmentMode0 : \
        SMAIN_NOT_VALID_CNS)

/* tti lookup key segment mode 1 */
#define SMEM_LION3_TTI_LOOKUP_KEY_SEGMENT_MODE_1_REG(dev)     \
        (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.TTIEngine.TTIKeySegmentMode1 : \
        SMAIN_NOT_VALID_CNS)

/* TTI - TTI User Defined Ether Types */
#define SMEM_LION3_TTI_TTI_USER_DEFINED_ETHER_TYPES_REG(dev,index)     \
        (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.TTIUserDefinedEtherTypes_[index] : \
        SMAIN_NOT_VALID_CNS)

/* TTI - PCL User Defined Ether Types */
#define SMEM_LION3_TTI_PCL_USER_DEFINED_ETHER_TYPES_REG(dev,index)     \
        (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.PCLUserDefinedEtherTypes_[index] : \
        SMAIN_NOT_VALID_CNS)

/* TTI - TTI Unit Global Configurations Ext */
#define SMEM_LION3_TTI_UNIT_GLOBAL_CONFIG_EXT_REG(dev)        \
        (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.TTIUnitGlobalConfigs.TTIUnitGlobalConfigExt : \
        SMAIN_NOT_VALID_CNS)

/* TTI - PCL port Group Map registers */
#define SMEM_LION3_TTI_PCL_PORT_GROUP_MAP_REG(dev,index)        \
        (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.portMap.portGroupMap[index] : \
        SMAIN_NOT_VALID_CNS)

/* TTI - PCL port List Map registers */
#define SMEM_LION3_TTI_PCL_PORT_LIST_MAP_REG(dev,index)        \
        (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.portMap.portListMap[index] : \
        SMAIN_NOT_VALID_CNS)

/*FDB Multi Hash CRC result Register[8]*/
#define SMEM_LION3_FDB_MULTI_HASH_CRC_RESULT_REG(dev,index)  \
        (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->FDB.FDBCore.FDBHashResults.FDBMultiHashCRCResultReg[index] : \
        SMAIN_NOT_VALID_CNS)

/*FDB non multi hash_crc_result*/
#define SMEM_LION3_FDB_NON_MULTI_HASH_CRC_RESULT_REG(dev)     \
        (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->FDB.FDBCore.FDBHashResults.FDBNonMulti_hash_crc_result : \
        SMAIN_NOT_VALID_CNS)

/*FDB non multi hash xor hash*/
#define SMEM_LION3_FDB_NON_MULTI_HASH_XOR_HASH_REG(dev)     \
        (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->FDB.FDBCore.FDBHashResults.FDBXorHash : \
        SMAIN_NOT_VALID_CNS)

/*FDB Counters Update Control*/
#define SMEM_LION3_FDB_COUNTERS_UPDATE_CONTROL_REG(dev)     \
        (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->FDB.FDBCore.FDBCntrsUpdate.FDBCntrsUpdateCtrl : \
        SMAIN_NOT_VALID_CNS)


/*FDB Bank Counter[]*/
#define SMEM_LION3_FDB_BANK_COUNTER_REG(dev,index)     \
        (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->FDB.FDBCore.FDBBankCntrs.FDBBankCntr[index] : \
        SMAIN_NOT_VALID_CNS)

/*FDB Bank Counters Control*/
#define SMEM_LION3_FDB_BANK_COUNTERS_CONTROL_REG(dev)     \
        (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->FDB.FDBCore.FDBBankCntrs.FDBBankCntrsControl : \
        SMAIN_NOT_VALID_CNS)

/*FDB Bank ranks[]*/
#define SMEM_LION3_FDB_BANK_RANKS_REG(dev,index)     \
        (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->FDB.FDBCore.FDBBankCntrs.FDBBankRank[index] : \
        SMAIN_NOT_VALID_CNS)


/* FDB - Global Configuration 1 Register  */
#define SMEM_LION3_FDB_GLOBAL_CONFIG_1_REG(dev)                               \
        (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->FDB.FDBCore.FDBGlobalConfig.FDBGlobalConfig1 : \
        SMAIN_NOT_VALID_CNS)

/* FDB - aging window size Register */
#define SMEM_LION3_FDB_AGING_WINDOW_SIZE_REG(dev)                               \
        (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->FDB.FDBCore.FDBGlobalConfig.FDBAgingWindowSize : \
        SMAIN_NOT_VALID_CNS)


/*L2I - bridge Access Matrix Default register */
#define SMEM_LION3_L2I_BRIDGE_ACCESS_MATRIX_DEFAULT_REG(dev)     \
        (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->L2I.bridgeAccessMatrix.bridgeAccessMatrixDefault : \
        SMAIN_NOT_VALID_CNS)

/*L2I - bridge Command Config 0 register */
#define SMEM_LION3_L2I_BRIDGE_COMMAND_CONFIG_0_REG(dev)     \
        (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->L2I.bridgeEngineConfig.bridgeCommandConfig0 : \
        SMAIN_NOT_VALID_CNS)

/*L2I - bridge Command Config 1 register */
#define SMEM_LION3_L2I_BRIDGE_COMMAND_CONFIG_1_REG(dev)     \
        (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->L2I.bridgeEngineConfig.bridgeCommandConfig1 : \
        SMAIN_NOT_VALID_CNS)

/*L2I - bridge Command Config 2 register */
#define SMEM_LION3_L2I_BRIDGE_COMMAND_CONFIG_2_REG(dev)     \
        (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->L2I.bridgeEngineConfig.bridgeCommandConfig2 : \
        SMAIN_NOT_VALID_CNS)

/*L2I - bridge Command Config 3 register */
#define SMEM_LION3_L2I_BRIDGE_COMMAND_CONFIG_3_REG(dev)     \
        (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->L2I.bridgeEngineConfig.bridgeCommandConfig3 : \
        SMAIN_NOT_VALID_CNS)

/*L2I - bridge Security Breach Drop Cntr Cfg 0 register */
#define SMEM_LION3_L2I_BRIDGE_SECURITY_BREACH_DROP_CNTR_CFG_0_REG(dev)     \
        (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->L2I.bridgeEngineConfig.bridgeSecurityBreachDropCntrCfg0 : \
        SMAIN_NOT_VALID_CNS)

/*L2I - bridge Security Breach Drop Cntr Cfg 1 register */
#define SMEM_LION3_L2I_BRIDGE_SECURITY_BREACH_DROP_CNTR_CFG_1_REG(dev)     \
        (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->L2I.bridgeEngineConfig.bridgeSecurityBreachDropCntrCfg1 : \
        SMAIN_NOT_VALID_CNS)

/* L2I - Security Breach Status 3 Register  */
#define SMEM_LION3_L2I_SECURITY_BREACH_STATUS_3_REG(dev)                               \
        (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->L2I.securityBreachStatus.securityBreachStatus3 : \
        SMAIN_NOT_VALID_CNS)

/* L2I - IPv4 Mc Link Local CPU Code Index */
#define SMEM_LION3_L2I_IPV4_MC_LINK_LOCAL_CPU_CODE_INDEX_REG(dev,protocol) \
        (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->L2I.ctrlTrafficToCPUConfig.IPv4McLinkLocalCPUCodeIndex[(protocol)/32][((protocol % 32) > 15)? 1 : 0] : \
        SMAIN_NOT_VALID_CNS)

/* L2I - IPv6 Mc Link Local CPU Code Index */
#define SMEM_LION3_L2I_IPV6_MC_LINK_LOCAL_CPU_CODE_INDEX_REG(dev,protocol) \
        (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->L2I.ctrlTrafficToCPUConfig.IPv6McLinkLocalCPUCodeIndex[(protocol)/32][((protocol % 32) > 15)? 1 : 0] : \
        SMAIN_NOT_VALID_CNS)

/* L2I - IPv4 Mc Link Local registered */
#define SMEM_LION3_L2I_IPV4_MC_LINK_LOCAL_REGISTERED_REG(dev,protocol) \
        (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->L2I.ctrlTrafficToCPUConfig.IPv4McLinkLocalReged[(protocol)/32] : \
        SMAIN_NOT_VALID_CNS)

/* L2I - IPv6 Mc Link Local registered */
#define SMEM_LION3_L2I_IPV6_MC_LINK_LOCAL_REGISTERED_REG(dev,protocol) \
        (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->L2I.ctrlTrafficToCPUConfig.IPv6McLinkLocalReged[(protocol)/32] : \
        SMAIN_NOT_VALID_CNS)

/* L2I - IEEE reserved Mc registered */
#define SMEM_LION3_L2I_IEEE_RESERVED_MC_REGISTERED_REG(dev,protocol) \
        (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->L2I.ctrlTrafficToCPUConfig.IEEEReservedMcReged[(protocol)/32] : \
        SMAIN_NOT_VALID_CNS)

/* L2I - counters set 0 config registers 0 */
#define SMEM_LION3_L2I_CNTRS_SET_0_CONFIG_0_REG(dev) \
        (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->L2I.layer2BridgeMIBCntrs.cntrsSetConfig0[0] : \
        SMAIN_NOT_VALID_CNS)

/* L2I - counters set 0 config registers 1 */
#define SMEM_LION3_L2I_CNTRS_SET_0_CONFIG_1_REG(dev) \
        (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->L2I.layer2BridgeMIBCntrs.cntrsSetConfig1[0] : \
        SMAIN_NOT_VALID_CNS)


/* L2I - counters set 1 config registers 0 */
#define SMEM_LION3_L2I_CNTRS_SET_1_CONFIG_0_REG(dev) \
        (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->L2I.layer2BridgeMIBCntrs.cntrsSetConfig0[1] : \
        SMAIN_NOT_VALID_CNS)

/* L2I - counters set 1 config registers 1 */
#define SMEM_LION3_L2I_CNTRS_SET_1_CONFIG_1_REG(dev) \
        (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->L2I.layer2BridgeMIBCntrs.cntrsSetConfig1[1] : \
        SMAIN_NOT_VALID_CNS)

/* L2I - Set1 VLAN Ingress Filtered Packet Count Register  */
#define SMEM_LION3_L2I_SET1_VLAN_IN_FILTER_CNT_REG(dev)                        \
        (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->L2I.layer2BridgeMIBCntrs.setVLANIngrFilteredPktCount[1] : \
        SMAIN_NOT_VALID_CNS)

/* L2I - Set1 Security Filtered Packet Count Register  */
#define SMEM_LION3_L2I_SET1_SECUR_FILTER_CNT_REG(dev)                          \
        (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->L2I.layer2BridgeMIBCntrs.setSecurityFilteredPktCount[1] : \
        SMAIN_NOT_VALID_CNS)

/* L2I - Set1 Bridge Filtered Packet Count Register  */
#define SMEM_LION3_L2I_SET1_BRDG_FILTER_CNT_REG(dev)                          \
        (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->L2I.layer2BridgeMIBCntrs.setBridgeFilteredPktCount[1] : \
        SMAIN_NOT_VALID_CNS)

/* L2I - Set1 Incoming Packet Count Register */
#define SMEM_LION3_L2I_SET1_IN_PKT_CNT_REG(dev)                                \
        (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->L2I.layer2BridgeMIBCntrs.setIncomingPktCount[1] : \
        SMAIN_NOT_VALID_CNS)

/*L2I - global EPort Value register */
#define SMEM_LION3_L2I_BRIDGE_GLOBAL_E_PORT_VALUE_REG(dev) \
        (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->L2I.globalEportConifguration.globalEPortValue : \
        SMAIN_NOT_VALID_CNS)

/*L2I - global EPort Mask register */
#define SMEM_LION3_L2I_BRIDGE_GLOBAL_E_PORT_MASK_REG(dev) \
        (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->L2I.globalEportConifguration.globalEPortMask : \
        SMAIN_NOT_VALID_CNS)

/* LPM - global configuration register */
#define SMEM_LION3_LPM_GLOBAL_CONFIG_REG(dev) \
        (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->LPM.globalConfig : \
        SMAIN_NOT_VALID_CNS)

/* LPM Policy Based Routing Configuration Register - LPM Direct Access Mode Register */
#define SMEM_LION3_LPM_DIRECT_ACCESS_MODE_REG(dev) \
        (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->LPM.directAccessMode : \
        SMAIN_NOT_VALID_CNS)

/* LPM Exception Status Register */
#define SMEM_LION3_LPM_EXCEPTION_STATUS_REG(dev) \
        (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->LPM.exceptionStatus : \
        SMAIN_NOT_VALID_CNS)

#define _16K    (16*1024)
/* LPM - lpm memory table (main table) */
#define SMEM_LION3_LPM_MEMORY_TBL_MEM(dev, entry) \
    /* the LPM build of 20 rams of up to 16k lines in each */ \
    SMEM_TABLE_ENTRY_2_PARAMS_INDEX_GET_MAC(dev, lpmMemory, \
        ((entry)%_16K)/*line index*/ , ((entry)/_16K)/* ram index 0..19*/)

/* LPM - lpm Ipv4 Vrf Id table */
#define SMEM_LION3_LPM_IPV4_VRF_ID_TBL_MEM(dev, entry) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, lpmIpv4VrfId, entry)

/* LPM - lpm Ipv6 Vrf Id table */
#define SMEM_LION3_LPM_IPV6_VRF_ID_TBL_MEM(dev, entry) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, lpmIpv6VrfId, entry)

/* LPM - lpm Fcoe Vrf Id table */
#define SMEM_LION3_LPM_FCOE_VRF_ID_TBL_MEM(dev, entry) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, lpmFcoeVrfId, entry)

/* LPM - lpm Ecmp table */
#define SMEM_LION3_LPM_ECMP_TBL_MEM(dev, entry) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, lpmEcmp, entry)

/*EGF_eft - Port Address Construct Mode  */
#define SMEM_LION3_EGF_EFT_PORT_ADDRESS_CONSTRUCT_MODE_REG(dev,port) \
        (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EGF_eft.deviceMapConfigs.portAddrConstructMode[port] : \
        SMAIN_NOT_VALID_CNS)

/*EGF_eft - egr Filters Global register */
#define SMEM_LION3_EGF_EFT_EGR_FILTERS_GLOBAL_REG(dev) \
        (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EGF_eft.egrFilterConfigs.egrFiltersGlobal : \
        SMAIN_NOT_VALID_CNS)

/*EGF_eft - port To Hemisphere Map Reg */
#define SMEM_LION3_EGF_EFT_EGR_PORT_TO_HEMISPHERE_MAP_REG(dev) \
        (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EGF_eft.MCFIFO.MCFIFOConfigs.portToHemisphereMapReg[0] : \
        SMAIN_NOT_VALID_CNS)

/* EGF_QAG -  Target Port Mapper table */
#define SMEM_LION2_EGF_QAG_TARGET_PORT_MAPPER_TBL_MEM(dev, index) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, egfQagTargetPortMapper, index)

/* EGF_QAG -  eVlan Descriptor Assignment Attributes table */
#define SMEM_LION2_EGF_QAG_EVLAN_DESCRIPTOR_ASSIGNMENT_ATTRIBUTES_TBL_MEM(dev, index) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, egfQagEVlanDescriptorAssignmentAttributes, index)

/* EGF_SHT -  Vid Mapper table  */
#define SMEM_LION2_EGF_SHT_VID_MAPPER_TBL_MEM(dev, index) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, egfShtVidMapper, index)

/* EGF_SHT - Eport EVlan Filter table  */
#define SMEM_LION2_EGF_SHT_EPORT_EVLAN_FILTER_TBL_MEM(dev, index) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, egfShtEportEVlanFilter, index)

/* EGF_SHT - egress Eport table  */
#define SMEM_LION2_EGF_SHT_EGRESS_EPORT_TBL_MEM(dev, index) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, egfShtEgressEPort, index)

/* EGF_SHT - non Trunk Members 2 table  */
#define SMEM_LION2_EGF_SHT_NON_TRUNK_MEMBERS_2_TBL_MEM(dev, index) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, nonTrunkMembers2, index)

/* EGF_QAG - egress Eport table  */
#define SMEM_LION2_EGF_QAG_EGRESS_EPORT_TBL_MEM(dev, index) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, egfQagEgressEPort, index)

/* EGF_QAG - EVlan Attribute table  */
#define SMEM_LION2_EGF_QAG_EGRESS_EVLAN_ATTRIBUTE_TBL_MEM(dev, index) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, egfShtEVlanAttribute, index)

/* EGF_QAG - EVlan Spanning table  */
#define SMEM_LION2_EGF_QAG_EGRESS_EVLAN_SPANNING_TBL_MEM(dev, index) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, egfShtEVlanSpanning, index)

/* EGF_EFT - virtual 2 physical port remap registers */
#define SMEM_LION2_EGF_EFT_VIRTUAL_2_PHYSICAL_PORT_REMAP_REG(dev,index) \
        (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EGF_eft.egrFilterConfigs.virtual2PhysicalPortRemap[index] : \
        SMAIN_NOT_VALID_CNS)

/* EGF_EFT - physical Port Link Status Mask registers */
#define SMEM_LION2_EGF_EFT_PHYSICAL_PORT_LINK_STATUS_MASK_REG(dev,index) \
        (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EGF_eft.egrFilterConfigs.physicalPortLinkStatusMask[index] : \
        SMAIN_NOT_VALID_CNS)

/* EGF_EFT - port Isolation Lookup 0 */
#define SMEM_LION3_EGF_EFT_PORT_ISOLATION_LOOKUP_0_REG(dev) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EGF_eft.global.portIsolationLookup0 : \
    SMAIN_NOT_VALID_CNS)

/* EGF_EFT - port Isolation Lookup 1 */
#define SMEM_LION3_EGF_EFT_PORT_ISOLATION_LOOKUP_1_REG(dev) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EGF_eft.global.portIsolationLookup1 : \
    SMAIN_NOT_VALID_CNS)

/* EGF_QAG - first register of eVlan Mirr Enable */
#define SMEM_LION3_EGF_QAG_EVLAN_MIRR_ENABLE_REG(dev) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EGF_qag.distributor.evlanMirrEnable[0] : \
    SMAIN_NOT_VALID_CNS)

/* EGF_QAG - first register of use Vlan Tag1 For Tag State */
#define SMEM_LION3_EGF_QAG_USE_VLAN_TAG_1_FOR_TAG_STATE_REG(dev) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EGF_qag.distributor.useVlanTag1ForTagStateReg[0] : \
    SMAIN_NOT_VALID_CNS)


/* EGF_SHT - first register of ignore Phy Src Mc Filter En */
#define SMEM_LION3_EGF_SHT_IGNORE_PHY_SRC_MC_FILTER_EN_REG(dev) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EGF_sht.global.ignorePhySrcMcFilterEn[0] : \
    SMAIN_NOT_VALID_CNS)

/* EGF_SHT - first register of eport Stp State Mode */
#define SMEM_LION3_EGF_SHT_EPORT_STP_STATE_MODE_REG(dev) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EGF_sht.global.eportStpStateMode[0] : \
    SMAIN_NOT_VALID_CNS)

/* EGF_SHT - first register of eport Stp State */
#define SMEM_LION3_EGF_SHT_EPORT_STP_STATE_REG(dev) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EGF_sht.global.eportStpState[0] : \
    SMAIN_NOT_VALID_CNS)

/* EGF_SHT - Egr Filters Enable */
#define SMEM_LION3_EGF_SHT_EGR_FILTERS_ENABLE_REG(dev) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EGF_sht.global.SHTEgrFiltersEnable : \
    SMAIN_NOT_VALID_CNS)

/* EGF_SHT - Mesh Id Configuration */
#define SMEM_LION3_EGF_SHT_MESH_ID_CONFIGS_REG(dev) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EGF_sht.global.MESHIDConfigs : \
    SMAIN_NOT_VALID_CNS)

/* EGF_SHT - first register of eport Mesh Id */
#define SMEM_LION3_EGF_SHT_EPORT_MESH_ID_REG(dev) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EGF_sht.global.ePortMeshID[0] : \
    SMAIN_NOT_VALID_CNS)

/* EGF_SHT - first register of vlan Filtering Enable */
#define SMEM_LION3_EGF_SHT_VLAN_FILTERING_ENABLE_REG(dev) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EGF_sht.global.eportEvlanFilteringEnable[0] : \
    SMAIN_NOT_VALID_CNS)

/* EGF_SHT - EVlan Filtering Enable */
#define SMEM_LION3_EGF_SHT_EVLAN_EGR_FILTERING_REG(dev) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EGF_sht.global.eportVlanEgrFiltering : \
    SMAIN_NOT_VALID_CNS)

/* EGF_SHT - first register of eport Port Isolation Mode */
#define SMEM_LION3_EGF_SHT_EPORT_PORT_ISOLATION_MODE_REG(dev) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EGF_sht.global.eportPortIsolationMode[0] : \
    SMAIN_NOT_VALID_CNS)

/* EGF_SHT - first register of eport Drop on ePort VID1 mismatch */
#define SMEM_LION3_EGF_SHT_EPORT_DROP_ON_EPORT_VID1_MISMATCH_REG(dev) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EGF_sht.global.eportDropOnEportVid1Mismatch[0] : \
    SMAIN_NOT_VALID_CNS)

/* EGF_SHT - first register of eport Associated VID1 */
#define SMEM_LION3_EGF_SHT_EPORT_ASSOCIATED_VID1_REG(dev) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EGF_sht.global.eportAssociatedVid1[0] : \
    SMAIN_NOT_VALID_CNS)

/* HA - IP Length Offset register */
#define SMEM_LION3_HA_IP_LENGTH_OFFSET_REG(dev) \
        (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->HA.IPLengthOffset : \
        SMAIN_NOT_VALID_CNS)

/* HA - Global Mac Sa table */
#define SMEM_LION2_HA_GLOBAL_MAC_SA_TBL_MEM(dev, index) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, haGlobalMacSa, index)

/* HA - HA QoS Profile to EXP table */
#define SMEM_LION2_HA_QOS_PROFILE_TO_EXP_TBL_MEM(dev, index) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, haQosProfileToExp, index)

/* HA - ethernet Over GRE Protocol Type register */
#define SMEM_LION3_HA_ETHERNET_OVER_GRE_PROTOCOL_TYPE_REG(dev) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->HA.ethernetOverGREProtocolType : \
    SMAIN_NOT_VALID_CNS)

/* HA - EPCL DIP Solicitation Data 0 */
#define SMEM_LION3_HA_EGR_POLICY_DIP_SOLICITATION_DATA_0_REG(dev) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->HA.egrPolicyDIPSolicitationData0 : \
    SMAIN_NOT_VALID_CNS)

/* HA - EPCL DIP Solicitation Mask 0 */
#define SMEM_LION3_HA_EGR_POLICY_DIP_SOLICITATION_MASK_0_REG(dev) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->HA.egrPolicyDIPSolicitationMask0 : \
    SMAIN_NOT_VALID_CNS)


/*EPCL - 12 profiles of UDB0..49 */
#define SMEM_LION3_HA_EPCL_UDB_CONFIGURATION_TBL_MEM(dev, index) \
      SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, haEpclUserDefinedBytesConfig, index)
/*MLL - IP MLL Table Resource Sharing Configuration register */
#define SMEM_LION3_MLL_IP_MLL_TABLE_RESOURCE_SHARING_CONFIG_REG(dev) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->MLL.MLLGlobalCtrl.IPMLLTableResourceSharingConfig : \
    SMAIN_NOT_VALID_CNS)

/*MLL - L2 MLL Table Resource Sharing Configuration register */
#define SMEM_LION3_MLL_L2_MLL_TABLE_RESOURCE_SHARING_CONFIG_REG(dev) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->MLL.MLLGlobalCtrl.L2MLLTableResourceSharingConfig : \
    SMAIN_NOT_VALID_CNS)

/*MLL - MLL Interrupt Cause register */
#define SMEM_LION3_MLL_INTERRUPT_CAUSE_REG(dev) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->MLL.MLLGlobalCtrl.MLLInterruptCauseReg : \
    SMAIN_NOT_VALID_CNS)

/*MLL - MLL Interrupt Mask register */
#define SMEM_LION3_MLL_INTERRUPT_MASK_REG(dev) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->MLL.MLLGlobalCtrl.MLLInterruptMaskReg : \
    SMAIN_NOT_VALID_CNS)

/*MLL - Source Based L2 MLL Filtering */
#define SMEM_LION3_MLL_SOURCE_BASED_L2_MLL_FILTERING_REG(dev) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->MLL.MLLGlobalCtrl.sourceBasedL2MLLFiltering : \
    SMAIN_NOT_VALID_CNS)

/*MLL - MLL Lookup Trigger Configuration */
#define SMEM_LION3_MLL_LOOKUP_TRIGGER_CONFIG_REG(dev) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->MLL.MLLGlobalCtrl.MLLLookupTriggerConfig : \
    SMAIN_NOT_VALID_CNS)

/*MLL - L2 MLL Out Interface Counter Configuration register*/
#define SMEM_LION3_L2_MLL_OUT_INTERFACE_CNTR_CONFIG_REG(dev,index) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->MLL.MLLOutInterfaceCntrs.L2MLLOutInterfaceCntrConfig[index] : \
    SMAIN_NOT_VALID_CNS)

/*MLL - L2 MLL Out Interface Counter Configuration 1 register*/
#define SMEM_LION3_L2_MLL_OUT_INTERFACE_CNTR_CONFIG_1_REG(dev,index) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->MLL.MLLOutInterfaceCntrs.L2MLLOutInterfaceCntrConfig1[index] : \
    SMAIN_NOT_VALID_CNS)

/*MLL - L2 MLL Out Multicast Packets Counter<%n>*/
#define SMEM_LION3_L2_MLL_OUT_MULTICAST_PACKETS_COUNTER_REG(dev,index) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->MLL.MLLOutInterfaceCntrs.L2MLLOutMcPktsCntr[index] : \
    SMAIN_NOT_VALID_CNS)

/*MLL - IP MLL Out Interface Counter Configuration register*/
#define SMEM_LION3_IP_MLL_OUT_INTERFACE_CNTR_CONFIG_REG(dev,index) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->MLL.MLLOutInterfaceCntrs.IPMLLOutInterfaceCntrConfig[index] : \
    SMAIN_NOT_VALID_CNS)

/*MLL - IP MLL Out Interface Counter Configuration 1 register*/
#define SMEM_LION3_IP_MLL_OUT_INTERFACE_CNTR_CONFIG_1_REG(dev,index) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->MLL.MLLOutInterfaceCntrs.IPMLLOutInterfaceCntrConfig1[index] : \
    SMAIN_NOT_VALID_CNS)

/*MLL - IP MLL Out Multicast Packets Counter<%n>*/
#define SMEM_LION3_IP_MLL_OUT_MULTICAST_PACKETS_COUNTER_REG(dev,index) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->MLL.MLLOutInterfaceCntrs.IPMLLOutMcPktsCntr[index] : \
    SMAIN_NOT_VALID_CNS)

/*MLL - IP MLL Skipped Entries Counter */
#define SMEM_LION3_IP_MLL_SKIPPED_ENTRIES_COUNTER_REG(dev) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ?  SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->MLL.MLLOutInterfaceCntrs.IPMLLSkippedEntriesCntr : \
    SMAIN_NOT_VALID_CNS)

/*MLL - MLL FIFO drop Counter */
#define SMEM_LION3_MLL_MC_FIFO_DROP_CNTR_REG(dev) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->MLL.MLLOutInterfaceCntrs.MLLMCFIFODropCntr : \
    SMAIN_NOT_VALID_CNS)


/*MLL - IP MLL MC Source-ID */
#define SMEM_LION3_IP_MLL_MC_SOURCE_ID_REG(dev) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->MLL.MLLGlobalCtrl.MLLMCSourceID : \
    SMAIN_NOT_VALID_CNS)

/*MLL - Replicated Traffic Ctrl */
#define SMEM_LION3_MLL_REPLICATED_TRAFFIC_CTRL_REG(dev) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->MLL.MLLGlobalCtrl.MLLReplicatedTrafficCtrl : \
    SMAIN_NOT_VALID_CNS)

/*MLL - multi Target EPort Map : multi Target EPort Value */
#define SMEM_LION3_MLL_MULTI_TARGET_EPORT_VALUE_REG(dev) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->MLL.multiTargetEPortMap.multiTargetEPortValue : \
    SMAIN_NOT_VALID_CNS)

/*MLL - multi Target EPort Map : multi Target EPort Mask */
#define SMEM_LION3_MLL_MULTI_TARGET_EPORT_MASK_REG(dev) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->MLL.multiTargetEPortMap.multiTargetEPortMask : \
    SMAIN_NOT_VALID_CNS)

/*MLL - multi Target EPort Map : multi Target EPort base */
#define SMEM_LION3_MLL_MULTI_TARGET_EPORT_BASE_REG(dev) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->MLL.multiTargetEPortMap.multiTargetEPortBase : \
    SMAIN_NOT_VALID_CNS)

/*MLL - multi Target EPort Map : ePort To EVIDX Base */
#define SMEM_LION3_MLL_MULTI_TARGET_EPORT_TO_EVIDX_BASE_REG(dev) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->MLL.multiTargetEPortMap.ePortToEVIDXBase : \
    SMAIN_NOT_VALID_CNS)

/* TCAM - Group Client Enable */
#define SMEM_LION3_TCAM_GROUP_CLIENT_ENABLE_REG(dev, group) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TCAM.groupClientEnable[group] : \
    SMAIN_NOT_VALID_CNS)

/* TCAM - Hit Num and Group Sel Floor */
#define SMEM_LION3_TCAM_HIT_NUM_AND_GROUP_SEL_FLOOR_REG(dev, floor) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TCAM.tcamHitNumAndGroupSelFloor[floor] : \
    SMAIN_NOT_VALID_CNS)

/* TCAM - Banks Array Power On 0 */
#define SMEM_LION3_TCAM_POWER_ON_REG(dev) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TCAM.tcamArrayConf.tcamActiveFloors : \
    SMAIN_NOT_VALID_CNS)

/* TCAM - TCAM memory table */
#define SMEM_LION3_TCAM_MEMORY_TBL_MEM(dev, entry) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, tcamMemory, entry)

/* TCAM - TCAM action memory table */
#define SMEM_LION3_TCAM_ACTION_TBL_MEM(dev, entry) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, globalActionTable, entry)

/* TCAM - Mg Comp Request */
#define SMEM_LION3_TCAM_MG_COMP_REQUEST_REG(dev) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TCAM.mgAccess.mgCompRequest : \
    SMAIN_NOT_VALID_CNS)

/* TCAM - Mg Key Group Chunk */
#define SMEM_LION3_TCAM_MG_KEY_GROUP_CHUNK_REG(dev, group, chunk) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TCAM.mgAccess.mgKeyGroupChunk[group][chunk] : \
    SMAIN_NOT_VALID_CNS)

/* TCAM - Mg Key Size And Type Group */
#define SMEM_LION3_TCAM_MG_KEY_SIZE_AND_TYPE_GROUP_REG(dev, group) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TCAM.mgAccess.mgKeySizeAndTypeGroup[group] : \
    SMAIN_NOT_VALID_CNS)

/* TCAM - Mg Hit Group Hit Num */
#define SMEM_LION3_TCAM_MG_HIT_GROUP_HIT_NUM_REG(dev, group, hitHum) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TCAM.mgAccess.mgHitGroupHitNum[group][hitNum] : \
    SMAIN_NOT_VALID_CNS)

/* TCAM - Mg Key Type Data */
#define SMEM_LION3_TCAM_MG_KEY_TYPE_DATA(dev) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TCAM.mgAccess.keyTypeData : \
    SMAIN_NOT_VALID_CNS)


/* TCAM - global register */
#define SMEM_LION3_TCAM_GLOBAL_REG(dev) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TCAM.tcamGlobalConfig : \
    SMAIN_NOT_VALID_CNS)


/*PCL - hash CRC 32 Salt registers */
#define SMEM_LION3_PCL_HASH_CRC_32_SALT_REG(dev,index) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->PCL.HASH.CRC32Salt[index] : \
    SMAIN_NOT_VALID_CNS)

/*PCL - hash CRC 32 Seed register */
#define SMEM_LION3_PCL_HASH_CRC_32_SEED_REG(dev) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->PCL.HASH.CRC32Seed : \
    SMAIN_NOT_VALID_CNS)

/*PCL - lookup sourceId mask register */
#define SMEM_LION3_PCL_LOOKUP_SOURCE_ID_MASK_REG(dev,index) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->PCL.lookupSourceIDMask[index] : \
    SMAIN_NOT_VALID_CNS)

/*PCL -  udb select table */
#define SMEM_LION3_PCL_UDB_SELECT_TBL_MEM(dev, entry_index , _cycle)             \
    ((_cycle == 0) ? SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, ipcl0UdbSelect, entry_index) : \
     (_cycle == 1) ? SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, ipcl1UdbSelect, entry_index) : \
     (_cycle == 2) ? SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, ipcl2UdbSelect, entry_index) : \
        SMAIN_NOT_VALID_CNS)

/*EPCL -  udb select table */
#define SMEM_LION3_EPCL_UDB_SELECT_TBL_MEM(dev, entry_index)             \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, epclUdbSelect, entry_index)


/* GOP unit */
/* TAI subunit */

/* GOP, TAI - TAI interrupt cause register */
#define SMEM_LION3_GOP_TAI_INT_CAUSE_REG(dev,group,inst)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TAI.TAI[group][inst].TAIInterruptCause : \
                                 SMAIN_NOT_VALID_CNS)

/* GOP, TAI - TAI interrupt mask register */
#define SMEM_LION3_GOP_TAI_INT_MASK_REG(dev,group,inst)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TAI.TAI[group][inst].TAIInterruptMask : \
                                 SMAIN_NOT_VALID_CNS)

/* GOP, TAI - Time Counter Function Configuration 0 register */
#define SMEM_LION3_GOP_TAI_FUNC_CONFIG_0_REG(dev,group,inst)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TAI.TAI[group][inst].timeCntrFunctionConfig0 : \
                                 SMAIN_NOT_VALID_CNS)

/* GOP, TAI - Time Adjustment Propagation Delay Configuration high register */
#define SMEM_LION3_GOP_TAI_PROP_DELAY_CONFIG_HIGH_REG(dev,group,inst)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TAI.TAI[group][inst].timeAdjustmentPropagationDelayConfigHigh : \
                                 SMAIN_NOT_VALID_CNS)

/* GOP, TAI - Time Adjustment Propagation Delay Configuration low register */
#define SMEM_LION3_GOP_TAI_PROP_DELAY_CONFIG_LOW_REG(dev,group,inst)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TAI.TAI[group][inst].timeAdjustmentPropagationDelayConfigLow : \
                                 SMAIN_NOT_VALID_CNS)

/* GOP, TAI - Time Load Value sec high register */
#define SMEM_LION3_GOP_TAI_LOAD_VALUE_SEC_HIGH_REG(dev,group,inst)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TAI.TAI[group][inst].timeLoadValueSecHigh : \
                                 SMAIN_NOT_VALID_CNS)

/* GOP, TAI - Time Load Value sec med register */
#define SMEM_LION3_GOP_TAI_LOAD_VALUE_SEC_MED_REG(dev,group,inst)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TAI.TAI[group][inst].timeLoadValueSecMed : \
                                 SMAIN_NOT_VALID_CNS)

/* GOP, TAI - Time Load Value sec low register */
#define SMEM_LION3_GOP_TAI_LOAD_VALUE_SEC_LOW_REG(dev,group,inst)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TAI.TAI[group][inst].timeLoadValueSecLow : \
                                 SMAIN_NOT_VALID_CNS)

/* GOP, TAI - Time Load Value nano high register */
#define SMEM_LION3_GOP_TAI_LOAD_VALUE_NANO_HIGH_REG(dev,group,inst)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TAI.TAI[group][inst].timeLoadValueNanoHigh : \
                                 SMAIN_NOT_VALID_CNS)

/* GOP, TAI - Time Load Value nano low register */
#define SMEM_LION3_GOP_TAI_LOAD_VALUE_NANO_LOW_REG(dev,group,inst)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TAI.TAI[group][inst].timeLoadValueNanoLow : \
                                 SMAIN_NOT_VALID_CNS)

/* GOP, TAI - Time Load Value frac high register */
#define SMEM_LION3_GOP_TAI_LOAD_VALUE_FRAC_HIGH_REG(dev,group,inst)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TAI.TAI[group][inst].timeLoadValueFracHigh : \
                                 SMAIN_NOT_VALID_CNS)

/* GOP, TAI - Time Load Value frac low register */
#define SMEM_LION3_GOP_TAI_LOAD_VALUE_FRAC_LOW_REG(dev,group,inst)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TAI.TAI[group][inst]. timeLoadValueNanoLow: \
                                 SMAIN_NOT_VALID_CNS)

/* GOP, TAI - Time Capture Value 0\1 sec high register */
#define SMEM_LION3_GOP_TAI_CAPTURE_VALUE_SEC_HIGH_REG(dev,group,inst,cap)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? ( (cap == 0) ? \
                                  SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TAI.TAI[group][inst].timeCaptureValue0SecHigh :  \
                                  SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TAI.TAI[group][inst].timeCaptureValue1SecHigh ) :\
                                 SMAIN_NOT_VALID_CNS)

/* GOP, TAI - Time Capture Value 0\1 sec med register */
#define SMEM_LION3_GOP_TAI_CAPTURE_VALUE_SEC_MED_REG(dev,group,inst,cap)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? ( (cap == 0) ? \
                                  SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TAI.TAI[group][inst].timeCaptureValue0SecMed :  \
                                  SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TAI.TAI[group][inst].timeCaptureValue1SecMed ) :\
                                 SMAIN_NOT_VALID_CNS)

/* GOP, TAI - Time Capture Value 0\1 sec low register */
#define SMEM_LION3_GOP_TAI_CAPTURE_VALUE_SEC_LOW_REG(dev,group,inst,cap)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? ( (cap == 0) ? \
                                  SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TAI.TAI[group][inst].timeCaptureValue0SecLow :  \
                                  SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TAI.TAI[group][inst].timeCaptureValue1SecLow ) :\
                                 SMAIN_NOT_VALID_CNS)

/* GOP, TAI - Time Capture Value 0\1 nano high register */
#define SMEM_LION3_GOP_TAI_CAPTURE_VALUE_NANO_HIGH_REG(dev,group,inst,cap)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? ( (cap == 0) ? \
                                  SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TAI.TAI[group][inst].timeCaptureValue0NanoHigh :  \
                                  SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TAI.TAI[group][inst].timeCaptureValue1NanoHigh ) :\
                                 SMAIN_NOT_VALID_CNS)

/* GOP, TAI - Time Capture Value 0\1 nano low register */
#define SMEM_LION3_GOP_TAI_CAPTURE_VALUE_NANO_LOW_REG(dev,group,inst,cap)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? ( (cap == 0) ? \
                                  SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TAI.TAI[group][inst].timeCaptureValue0NanoLow :  \
                                  SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TAI.TAI[group][inst].timeCaptureValue1NanoLow ) :\
                                 SMAIN_NOT_VALID_CNS)

/* GOP, TAI - Time Capture Value 0\1 frac high register */
#define SMEM_LION3_GOP_TAI_CAPTURE_VALUE_FRAC_HIGH_REG(dev,group,inst,cap)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? ( (cap == 0) ? \
                                  SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TAI.TAI[group][inst].timeCaptureValue0FracHigh :  \
                                  SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TAI.TAI[group][inst].timeCaptureValue1FracHigh ) :\
                                 SMAIN_NOT_VALID_CNS)

/* GOP, TAI -  Time Capture Value 0\1 frac low register */
#define SMEM_LION3_GOP_TAI_CAPTURE_VALUE_FRAC_LOW_REG(dev,group,inst,cap)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? ( (cap == 0) ? \
                                  SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TAI.TAI[group][inst].timeCaptureValue0FracLow :  \
                                  SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TAI.TAI[group][inst].timeCaptureValue1FracLow ) :\
                                 SMAIN_NOT_VALID_CNS)

/* GOP, TAI - Time Capture Status register */
#define SMEM_LION3_GOP_TAI_CAPTURE_STATUS_REG(dev,group,inst)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TAI.TAI[group][inst].timeCaptureStatus : \
                                 SMAIN_NOT_VALID_CNS)

/* GOP, TAI - Time Update Counter register */
#define SMEM_LION3_GOP_TAI_UPDATE_COUNTER_REG(dev,group,inst)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TAI.TAI[group][inst].timeUpdateCntr : \
                                 SMAIN_NOT_VALID_CNS)

/* GOP, TAI - Drift Adjustment Configuration high register */
#define SMEM_LION3_GOP_TAI_DFIFT_ADJUST_CONFIG_HIGH_REG(dev,group,inst)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TAI.TAI[group][inst].driftAdjustmentConfigHigh : \
                                 SMAIN_NOT_VALID_CNS)

/* GOP, TAI - Drift Adjustment Configuration low register */
#define SMEM_LION3_GOP_TAI_DFIFT_ADJUST_CONFIG_LOW_REG(dev,group,inst)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TAI.TAI[group][inst].driftAdjustmentConfigLow : \
                                 SMAIN_NOT_VALID_CNS)

/* GOP, PTP - PTP interrupt cause register */
#define SMEM_LION3_GOP_PTP_INT_CAUSE_REG(dev,port)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->GOP.PTP[(port)%72].PTPInterruptCause : \
                                 SMAIN_NOT_VALID_CNS)

/* GOP, PTP - PTP interrupt mask register */
#define SMEM_LION3_GOP_PTP_INT_MASK_REG(dev,port)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->GOP.PTP[(port)%72].PTPInterruptMask : \
                                 SMAIN_NOT_VALID_CNS)

/* GOP, PTP - PTP general control register */
#define SMEM_LION3_GOP_PTP_GENERAL_CTRL_REG(dev,port)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->GOP.PTP[(port)%72].PTPGeneralCtrl : \
                                 SMAIN_NOT_VALID_CNS)

/* GOP, PTP - PTP TX Timestamp Queue0/1 reg0 register */
#define SMEM_LION3_GOP_PTP_TX_TS_QUEUE_REG0_REG(dev,port,index)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? ( (index == 0) ? \
                                  SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->GOP.PTP[(port)%72].PTPTXTimestampQueue0Reg0 :  \
                                  SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->GOP.PTP[(port)%72].PTPTXTimestampQueue1Reg0 ) :\
                                 SMAIN_NOT_VALID_CNS)

/* GOP, PTP - PTP TX Timestamp Queue0/1 reg1 register */
#define SMEM_LION3_GOP_PTP_TX_TS_QUEUE_REG1_REG(dev,port,index)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? ( (index == 0) ? \
                                  SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->GOP.PTP[(port)%72].PTPTXTimestampQueue0Reg1 :  \
                                  SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->GOP.PTP[(port)%72].PTPTXTimestampQueue1Reg1 ) :\
                                 SMAIN_NOT_VALID_CNS)

/* GOP, PTP - PTP TX Timestamp Queue0/1 reg2 register */
#define SMEM_LION3_GOP_PTP_TX_TS_QUEUE_REG2_REG(dev,port,index)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? ( (index == 0) ? \
                                  SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->GOP.PTP[(port)%72].PTPTXTimestampQueue0Reg2 :  \
                                  SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->GOP.PTP[(port)%72].PTPTXTimestampQueue1Reg2 ) :\
                                 SMAIN_NOT_VALID_CNS)

/* GOP, PTP - Total PTP packets counter register */
#define SMEM_LION3_GOP_PTP_TOTAL_PTP_PKTS_CNTR_REG(dev,port)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->GOP.PTP[(port)%72].totalPTPPktsCntr : \
                                 SMAIN_NOT_VALID_CNS)

/* GOP, PTP - PTPv1 packet counter register */
#define SMEM_LION3_GOP_PTP_PTPv1_PKT_CNTR_REG(dev,port)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->GOP.PTP[(port)%72].PTPv1PktCntr : \
                                 SMAIN_NOT_VALID_CNS)

/* GOP, PTP - PTPv2 packet counter register */
#define SMEM_LION3_GOP_PTP_PTPv2_PKT_CNTR_REG(dev,port)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->GOP.PTP[(port)%72].PTPv2PktCntr : \
                                 SMAIN_NOT_VALID_CNS)

/* GOP, PTP - Y1731 packet counter register */
#define SMEM_LION3_GOP_PTP_Y1731_PKT_CNTR_REG(dev,port)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->GOP.PTP[(port)%72].Y1731PktCntr : \
                                 SMAIN_NOT_VALID_CNS)

/* GOP, PTP - NTPTs packet counter register */
#define SMEM_LION3_GOP_PTP_NTPTS_PKT_CNTR_REG(dev,port)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->GOP.PTP[(port)%72].NTPTsPktCntr : \
                                 SMAIN_NOT_VALID_CNS)

/* GOP, PTP - NTPReceive packet counter register */
#define SMEM_LION3_GOP_PTP_NTPRX_PKT_CNTR_REG(dev,port)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->GOP.PTP[(port)%72].NTPReceivePktCntr : \
                                 SMAIN_NOT_VALID_CNS)

/* GOP, PTP - NTPTransmit packet counter register */
#define SMEM_LION3_GOP_PTP_NTPTX_PKT_CNTR_REG(dev,port)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->GOP.PTP[(port)%72].NTPTransmitPktCntr : \
                                 SMAIN_NOT_VALID_CNS)

/* GOP, PTP - WAMP packet counter register */
#define SMEM_LION3_GOP_PTP_WAMP_PKT_CNTR_REG(dev,port)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->GOP.PTP[(port)%72].WAMPPktCntr : \
                                 SMAIN_NOT_VALID_CNS)

/* GOP, PTP - None action packet counter register */
#define SMEM_LION3_GOP_PTP_NONE_ACTION_PKT_CNTR_REG(dev,port)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->GOP.PTP[(port)%72].noneActionPktCntr : \
                                 SMAIN_NOT_VALID_CNS)

/* GOP, PTP - Forward action packet counter register */
#define SMEM_LION3_GOP_PTP_FORWARD_ACTION_PKT_CNTR_REG(dev,port)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->GOP.PTP[(port)%72].forwardActionPktCntr : \
                                 SMAIN_NOT_VALID_CNS)

/* GOP, PTP - Drop action packet counter register */
#define SMEM_LION3_GOP_PTP_DROP_ACTION_PKT_CNTR_REG(dev,port)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->GOP.PTP[(port)%72].dropActionPktCntr : \
                                 SMAIN_NOT_VALID_CNS)

/* GOP, PTP - Capture action packet counter register */
#define SMEM_LION3_GOP_PTP_CAPTURE_ACTION_PKT_CNTR_REG(dev,port)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->GOP.PTP[(port)%72].captureActionPktCntr : \
                                 SMAIN_NOT_VALID_CNS)

/* GOP, PTP - AddTime action packet counter register */
#define SMEM_LION3_GOP_PTP_ADD_TIME_ACTION_PKT_CNTR_REG(dev,port)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->GOP.PTP[(port)%72].captureActionPktCntr : \
                                 SMAIN_NOT_VALID_CNS)

/* GOP, PTP - AddCorrectedTime action packet counter register */
#define SMEM_LION3_GOP_PTP_ADD_CORRECTED_TIME_ACTION_PKT_CNTR_REG(dev,port)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->GOP.PTP[(port)%72].addCorrectedTimeActionPktCntr : \
                                 SMAIN_NOT_VALID_CNS)

/* GOP, PTP - CaptureAddTime action packet counter register */
#define SMEM_LION3_GOP_PTP_CAPTURE_ADD_TIME_PKT_CNTR_REG(dev,port)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->GOP.PTP[(port)%72].captureAddTimeActionPktCntr : \
                                 SMAIN_NOT_VALID_CNS)

/* GOP, PTP - CaptureAddCorrectedTime action packet counter register */
#define SMEM_LION3_GOP_PTP_CAPTURE_ADD_CORRECTED_TIME_ACTION_PKT_CNTR_REG(dev,port)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->GOP.PTP[(port)%72].captureAddCorrectedTimeActionPktCntr : \
                                 SMAIN_NOT_VALID_CNS)

/* GOP, PTP - AddIngressTime action packet counter register */
#define SMEM_LION3_GOP_PTP_ADD_INGRESS_TIME_ACTION_PKT_CNTR_REG(dev,port)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->GOP.PTP[(port)%72].addIngrTimeActionPktCntr : \
                                 SMAIN_NOT_VALID_CNS)

/* GOP, PTP - CaptureAddIngressTime action packet counter register */
#define SMEM_LION3_GOP_PTP_CAPTURE_ADD_INGRESS_TIME_ACTION_PKT_CNTR_REG(dev,port)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->GOP.PTP[(port)%72].captureAddIngrTimeActionPktCntr: \
                                 SMAIN_NOT_VALID_CNS)

/* GOP, PTP - CaptureIngressTime action packet counter register */
#define SMEM_LION3_GOP_PTP_CAPTURE_INGRESS_TIME_ACTION_PKT_CNTR_REG(dev,port)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->GOP.PTP[(port)%72].captureIngrTimeActionPktCntr : \
                                 SMAIN_NOT_VALID_CNS)

/* GOP, PTP - NTP PTP Offset High register */
#define SMEM_LION3_GOP_PTP_NTP_OFFSET_HIGH_REG(dev,port)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->GOP.PTP[(port)%72].NTPPTPOffsetHigh : \
                                 SMAIN_NOT_VALID_CNS)

/* GOP, PTP - NTP PTP Offset Low register */
#define SMEM_LION3_GOP_PTP_NTP_OFFSET_LOW_REG(dev,port)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->GOP.PTP[(port)%72].NTPPTPOffsetLow : \
                                 SMAIN_NOT_VALID_CNS)


/* HA - tunnel Start Generic Ip Profile table*/
#define SMEM_LION3_HA_TUNNEL_START_GENERIC_IP_PROFILE_TBL_MEM(dev, index) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, tunnelStartGenericIpProfile, index)


/* ERMRK - Global Configuration register */
#define SMEM_LION3_ERMRK_GLOBAL_CONFIG_REG(dev)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->ERMRK.ERMRKGlobalConfig : \
                                 SMAIN_NOT_VALID_CNS)

/* ERMRK - Interrupt Cause register */
#define SMEM_LION3_ERMRK_INTERRUPT_CAUSE_REG(dev)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->ERMRK.ERMRKInterruptCause : \
                                 SMAIN_NOT_VALID_CNS)

/* ERMRK - Interrupt Mask register */
#define SMEM_LION3_ERMRK_INTERRUPT_MASK_REG(dev)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->ERMRK.ERMRKInterruptMask : \
                                 SMAIN_NOT_VALID_CNS)

/* ERMRK - PTP Configuration register */
#define SMEM_LION3_ERMRK_PTP_CONFIG_REG(dev)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->ERMRK.ERMRKPTPConfig : \
                                 SMAIN_NOT_VALID_CNS)

/* ERMRK - Timestamping UDP Checksum Mode register */
#define SMEM_LION3_ERMRK_TS_UDP_CHECKSUM_MODE_REG(dev)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->ERMRK.timestampingUDPChecksumMode : \
                                 SMAIN_NOT_VALID_CNS)

 /* ERMRK - Timestamp Queue Message Type register */
#define SMEM_LION3_ERMRK_TS_QUEUE_MESSAGE_TYPE_REG(dev)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->ERMRK.timestampQueueMsgType : \
                                 SMAIN_NOT_VALID_CNS)

/* ERMRK - Timestamp Queue Entry ID register */
#define SMEM_LION3_ERMRK_TS_QUEUE_ENTRY_ID_REG(dev)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->ERMRK.timestampQueueEntryID : \
                                 SMAIN_NOT_VALID_CNS)

/* ERMRK - PTP NTP Offset register */
#define SMEM_LION3_ERMRK_PTP_NTP_OFFSET_REG(dev)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->ERMRK.PTPNTPOffset : \
                                 SMAIN_NOT_VALID_CNS)

/* ERMRK - UP2UP Map table register */
#define SMEM_LION3_ERMRK_UP2UP_MAP_TBL_REG(dev,set_index)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->ERMRK.ERMRKUP2UPMapTable[set_index] : \
                                 SMAIN_NOT_VALID_CNS)

 /* ERMRK - PTP Invalid Timestamp Counter register */
#define SMEM_LION3_ERMRK_PTP_INVALID_TS_COUNTER_REG(dev)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->ERMRK.ERMRKPTPInvalidTimestampCntr : \
                                 SMAIN_NOT_VALID_CNS)

/* ERMRK - DP2CFI table register */
#define SMEM_LION3_ERMRK_DP2CFI_TBL_REG(dev)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->ERMRK.ERMRKDP2CFITable : \
                                 SMAIN_NOT_VALID_CNS)

/* ERMRK - EXP2EXP Map table register */
#define SMEM_LION3_ERMRK_EXP2EXP_MAP_TBL_REG(dev,set_index)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->ERMRK.ERMRKEXP2EXPMapTable[set_index] : \
                                 SMAIN_NOT_VALID_CNS)

/* ERMRK - Timestamp Queue Entry Word0\1\2 register */
#define SMEM_LION3_ERMRK_TS_QUEUE_ENTRY_WORD_REG(dev,index,word)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? ((word == 0) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->ERMRK.timestampQueueEntryWord0[index] : \
                                  ((word == 1) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->ERMRK.timestampQueueEntryWord1[index] : \
                                                 SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->ERMRK.timestampQueueEntryWord2[index])) : \
                                 SMAIN_NOT_VALID_CNS)

/* ERMRK - Timestamp Queue Entry Word0 register */
#define SMEM_LION3_ERMRK_TS_QUEUE_ENTRY_WORD0_REG(dev,index)    \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->ERMRK.timestampQueueEntryWord0[index] : \
                                 SMAIN_NOT_VALID_CNS)

/* ERMRK - Timestamp Queue Entry Word1 register */
#define SMEM_LION3_ERMRK_TS_QUEUE_ENTRY_WORD1_REG(dev,index)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->ERMRK.timestampQueueEntryWord1[index] : \
                                 SMAIN_NOT_VALID_CNS)

/* ERMRK - Timestamp Queue Entry Word2 register */
#define SMEM_LION3_ERMRK_TS_QUEUE_ENTRY_WORD2_REG(dev,index)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->ERMRK.timestampQueueEntryWord2[index] : \
                                 SMAIN_NOT_VALID_CNS)

/* ERMRK - Timestamp Queue Entry ID Clear register */
#define SMEM_LION3_ERMRK_TS_QUEUE_ENTRY_ID_CLEAR_REG(dev)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->ERMRK.timestampQueueEntryIDClear : \
                                 SMAIN_NOT_VALID_CNS)

 /* ERMRK - ECC Error injection register */
#define SMEM_LION3_ERMRK_ECC_ERROR_INJECTION_REG(dev)     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->ERMRK.ECCErrorInjection : \
                                 SMAIN_NOT_VALID_CNS)


/* TTI Cause register SUM bit (Global interrupt)  */
#define SMEM_LION3_TTI_SUM_INT(dev)                       \
         (1 << (dev)->globalInterruptCauseRegister.tti)

/* HA Cause register SUM bit (Global interrupt)  */
#define SMEM_LION3_HA_SUM_INT(dev)                       \
         (1 << (dev)->globalInterruptCauseRegister.ha)

/* ERMRK Cause register SUM bit (Global interrupt)  */
#define SMEM_LION3_ERMRK_SUM_INT(dev)                       \
         (1 << (dev)->globalInterruptCauseRegister.ermrk)

/* GOP Cause register SUM bit (Global interrupt)  */
#define SMEM_LION3_GOP_SUM_INT(dev)                       \
         (1 << (dev)->globalInterruptCauseRegister.gop)

/* Port<n> MPCS PCS40G Common Control */
#define SMEM_LION2_GOP_MPCS_PCS40G_COMMON_CONTROL_REG(dev, port) \
     ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->GOP.ports.MPCSIP[port].PCS40GCommonControl   : \
             SMAIN_NOT_VALID_CNS)

/* HA - Mpls Channel Type Profile, index range 0..6 */
#define SMEM_LION3_HA_MPLS_CHANNEL_TYPE_PROFILE_REG(dev, index)     \
        (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->HA.MPLSChannelTypeProfileReg[index] : \
        SMAIN_NOT_VALID_CNS)

/* HA - Mpls Channel Type Profile, index 7 */
#define SMEM_LION3_HA_MPLS_CHANNEL_TYPE_PROFILE7_REG(dev)     \
        (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->HA.MPLSChannelTypeProfileReg7 : \
        SMAIN_NOT_VALID_CNS)

/* HA - BPE Config Reg 1 */
#define SMEM_LION3_HA_BPE_CONFIG_1_REG(dev)     \
        (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->HA.BPEConfigReg1 : \
        SMAIN_NOT_VALID_CNS)

/* HA - BPE Config Reg 2 */
#define SMEM_LION3_HA_BPE_CONFIG_2_REG(dev)     \
        (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->HA.BPEConfigReg2 : \
        SMAIN_NOT_VALID_CNS)



    /* EGF_QaG - TC remapping register */
#define SMEM_LION3_EGF_QAG_TC_REMAP_REG(dev,index)     \
        (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EGF_qag.distributor.TCRemap[index] : \
        SMAIN_NOT_VALID_CNS)
    /* EGF_QaG - Stack port remap enable register */
#define SMEM_LION3_EGF_QAG_STACK_REMAP_EN_REG(dev,index)     \
        (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EGF_qag.distributor.stackRemapEn[index] : \
        SMAIN_NOT_VALID_CNS)


/* MG : general Config register */
#define SMEM_LION3_MG_GENERAL_CONFIG_REG(dev)                                       \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->MG.globalConfig.generalConfig : \
        SMAIN_NOT_VALID_CNS)

#define SMEM_LION3_MG_AUQ_GENERAL_CONFIG_REG(dev)                                       \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->MG.addrUpdateQueueConfig.GeneralControl : \
        SMAIN_NOT_VALID_CNS)

#define SMEM_LION3_MG_AUQ_HOST_CONFIG_REG(dev)                                      \
    ((dev)->supportRegistersDb ?                                                    \
    (SMEM_CHT_MAC_REG_DB_GET(dev))->MG.addrUpdateQueueConfig.AUQHostConfiguration : \
        SMAIN_NOT_VALID_CNS)



/*CNC : CNC Block Range Count Enable register 0 */
#define SMEM_LION3_CNC_BLOCK_COUNT_EN_0_REG(dev,_block,_cncUnitIndex) \
        (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->CNC[_cncUnitIndex].perBlockRegs.rangeCount.CNCBlockRangeCountEnable[0][_block] :  \
        SMAIN_NOT_VALID_CNS)

/*CNC : CNC Block Range Count Enable register 1 */
#define SMEM_LION3_CNC_BLOCK_COUNT_EN_1_REG(dev,_block,_cncUnitIndex) \
        (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->CNC[_cncUnitIndex].perBlockRegs.rangeCount.CNCBlockRangeCountEnable[1][_block] :  \
        SMAIN_NOT_VALID_CNS)


/* Txq Mib Counters Set port Configuration Register */
#define SMEM_LION3_TXQ_MIB_COUNTERS_PORT_CONFIG_SET_N_CONFIG_REG(dev,index) \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->SIP5_TXQ_QUEUE.peripheralAccess.egrMIBCntrs.txQMIBCntrsPortSetConfig[index] : \
        SMAIN_NOT_VALID_CNS)

/* Txq DQ : map TXQ port to TX_DMA port */
#define SMEM_LION3_TXQ_DQ_TXQ_PORT_TO_TX_DMA_MAP_REG(dev,index,dpUnitInPipe) \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->SIP5_TXQ_DQ[dpUnitInPipe].global.globalDQConfig.portToDMAMapTable[index] : \
        SMAIN_NOT_VALID_CNS)


/* Txq DQ : global Dequeue Config register */
#define SMEM_LION3_TXQ_DQ_GLOBAL_DEQUEUE_CONFIG_REG(dev,dpUnitInPipe) \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->SIP5_TXQ_DQ[dpUnitInPipe].global.globalDQConfig.globalDequeueConfig : \
        SMAIN_NOT_VALID_CNS)

/* Txq DQ : STC Analyzer Index register */
#define SMEM_LION3_TXQ_DQ_STC_ANALYZER_INDEX_REG(dev,dpUnitInPipe) \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->SIP5_TXQ_DQ[dpUnitInPipe].statisticalAndCPUTrigEgrMirrToAnalyzerPort.STCAnalyzerIndex : \
        SMAIN_NOT_VALID_CNS)



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __sregLion2h */

