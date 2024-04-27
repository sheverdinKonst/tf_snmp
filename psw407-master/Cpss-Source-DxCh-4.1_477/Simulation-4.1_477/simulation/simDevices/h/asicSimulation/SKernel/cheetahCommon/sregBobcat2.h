/******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* sregBobcat2.h
*
* DESCRIPTION:
*       Defines for Bobcat2 memory registers access.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 3 $
*
*******************************************************************************/
#ifndef __sregBobcat2h
#define __sregBobcat2h

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define SIP_5_REG(dev,reg)  \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->reg : \
        SMAIN_NOT_VALID_CNS)

#define SIP_5_10_REG(dev,reg)  \
    (SMEM_CHT_IS_SIP5_10_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->reg : \
        SMAIN_NOT_VALID_CNS)

#define SIP_5_15_REG(dev,reg)  \
    (SMEM_CHT_IS_SIP5_15_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->reg : \
        SMAIN_NOT_VALID_CNS)



/* TM_DROP unit : ports Interface registers. each port hold 1 bit , support 8 registers --> 256 ports*/
#define SMEM_BOBCAT2_TM_DROP_PORTS_INTERFACE_REG(dev,port) \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(devObjPtr)->TMDROP.portsInterface[(port)/32] : \
        SMAIN_NOT_VALID_CNS)

/* TM_DROP unit : map TM TC to TM COS registers. each cos hold 3 bit3, 8 cos in register , support 2 registers --> 16 tmTc */
#define SMEM_BOBCAT2_TM_DROP_TM_TC_TO_TM_COS_REG(dev,tmTc) \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(devObjPtr)->TMDROP.TMDROPTMTCMap8To87[(tmTc)/8] : \
        SMAIN_NOT_VALID_CNS)

/* TM_DROP unit : Queue Profile ID Table */
#define SMEM_BOBCAT2_TM_DROP_QUEUE_PROFILE_ID_TBL_MEM(dev, index) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, tmDropQueueProfileId, index)

/* TM_DROP unit : Drop Masking Table */
#define SMEM_BOBCAT2_TM_DROP_DROP_MASKING_TBL_MEM(dev, index) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, tmDropDropMasking, index)

/* TM_EGRESS_GLUE unit : Target Interface Table . 4 ports in entry */
#define SMEM_BOBCAT2_TM_EGRESS_GLUE_TARGET_INTERFACE_TBL_MEM(dev, port) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, tmEgressGlueTargetInterface, ((port)/4))

/* BMA - Virtual => Physical source port mapping . index is port , value is port */
#define SMEM_BOBCAT2_BMA_PORT_MAPPING_TBL_MEM(dev, port) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, bmaPortMapping, port)

/* HA - NAT Exception Drop Counter Register */
#define SMEM_BOBCAT2_HA_NAT_EXCEPTION_DROP_COUNTER_REG(dev) \
    (SMEM_CHT_IS_SIP5_10_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->HA.NATExceptionDropCounter : \
        SMAIN_NOT_VALID_CNS)

/* HA - NAT Configurations Register */
#define SMEM_BOBCAT2_HA_NAT_CONFIGURATIONS_REG(dev) \
    (SMEM_CHT_IS_SIP5_10_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->HA.NATConfig : \
        SMAIN_NOT_VALID_CNS)

/* Ingress/Egress Policer config table Memory */
#define SMEM_SIP5_15_POLICER_CONFIG_ENTRY_TBL_MEM(dev, cycle, index)            \
     (((cycle) < 2) ?                                                            \
            ( smemChtPolicerTableIndexCheck(dev, cycle, index) ?                 \
                /* ingress */                                                    \
             SMEM_TABLE_ENTRY_2_PARAMS_INDEX_GET_MAC(dev, policerConfig, index, cycle) : SMAIN_NOT_VALID_CNS ) : \
                /*egress*/                                                       \
             SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, egressPolicerConfig, index))

/* Ingress/Egress Policer Metering Conformance Level Sign Memory */
#define SMEM_SIP5_15_POLICER_METERING_CONFORMANCE_LEVEL_SIGN_TBL_MEM(dev, cycle, index)            \
     (((cycle) < 2) ?                                                            \
            ( smemChtPolicerTableIndexCheck(dev, cycle, index) ?                 \
                /* ingress */                                                    \
             SMEM_TABLE_ENTRY_2_PARAMS_INDEX_GET_MAC(dev, policerConformanceLevelSign, index, cycle) : SMAIN_NOT_VALID_CNS ) : \
                /*egress*/                                                       \
             SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, egressPolicerConformanceLevelSign, index))


/* PLR : statistical metering configuration 0 register */
#define SMEM_SIP5_15_POLICER_STATISTICAL_METERING_CONFIG_0_REG(dev, stage)         \
    (SMEM_CHT_IS_SIP5_15_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->PLR[stage].statisticalMeteringConfig0 : \
    SMAIN_NOT_VALID_CNS)

/* PLR : statistical metering configuration 1 register */
#define SMEM_SIP5_15_POLICER_STATISTICAL_METERING_CONFIG_1_REG(dev, stage)         \
    (SMEM_CHT_IS_SIP5_15_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->PLR[stage].statisticalMeteringConfig1 : \
    SMAIN_NOT_VALID_CNS)

#define SMEM_SIP_5_10_RXDMA_PIP_PRIO_THRESHOLDS_REG(dev,unitIndex) \
    SIP_5_10_REG(dev,rxDMA[unitIndex].globalRxDMAConfigs.preIngrPrioritizationConfStatus.PIPPrioThresholds)

#define SMEM_SIP_5_10_RXDMA_PIP_ENABLE_REG(dev,unitIndex) \
    SIP_5_10_REG(dev,rxDMA[unitIndex].globalRxDMAConfigs.preIngrPrioritizationConfStatus.preingressPrioritizationEnable)

#define SMEM_SIP_5_10_RXDMA_PIP_VLAN_UP_PRIORITY_ARRAY_PROFILE_N_REG(dev,pip_profile,unitIndex) \
    SIP_5_10_REG(dev,rxDMA[unitIndex].globalRxDMAConfigs.preIngrPrioritizationConfStatus.VLANUPPriorityArrayProfile[pip_profile])

#define SMEM_SIP_5_10_RXDMA_PIP_QOS_DSA_PRIORITY_ARRAY_PROFILE_N_SEGMENT_M_REG(dev,pip_profile,dsaQos,unitIndex)\
    SIP_5_10_REG(dev,rxDMA[unitIndex].globalRxDMAConfigs.preIngrPrioritizationConfStatus.qoSDSAPriorityArrayProfileSegment[pip_profile][(dsaQos) / 16])

#define SMEM_SIP_5_10_RXDMA_PIP_MPLS_EXP_PRIORITY_ARRAY_PROFILE_N_REG(dev,pip_profile,unitIndex) \
    SIP_5_10_REG(dev,rxDMA[unitIndex].globalRxDMAConfigs.preIngrPrioritizationConfStatus.MPLSEXPPriorityArrayProfile[pip_profile])

#define SMEM_SIP_5_10_RXDMA_PIP_IPV4_TOS_PRIORITY_ARRAY_PROFILE_N_SEGMENT_M_REG(dev,pip_profile,ipv4_tos,unitIndex)\
    SIP_5_10_REG(dev,rxDMA[unitIndex].globalRxDMAConfigs.preIngrPrioritizationConfStatus.IPv4ToSPriorityArrayProfileSegment[pip_profile][(ipv4_tos) / 16])

#define SMEM_SIP_5_10_RXDMA_PIP_MAC_DA_PRIORITY_ARRAY_PROFILE_N_REG(dev,pip_profile,unitIndex) \
    SIP_5_10_REG(dev,rxDMA[unitIndex].globalRxDMAConfigs.preIngrPrioritizationConfStatus.MACDAPriorityArrayProfile[pip_profile])

#define SMEM_SIP_5_10_RXDMA_PIP_ETHERTYPE_PRIORITY_ARRAY_PROFILE_N_REG(dev,pip_profile,unitIndex) \
    SIP_5_10_REG(dev,rxDMA[unitIndex].globalRxDMAConfigs.preIngrPrioritizationConfStatus.etherTypePriorityArrayProfile[pip_profile])

#define SMEM_SIP_5_10_RXDMA_PIP_IPV4_ETHERTYPE_CONF_REG(dev,unitIndex) \
    SIP_5_10_REG(dev,rxDMA[unitIndex].globalRxDMAConfigs.preIngrPrioritizationConfStatus.IPv4EtherTypeConf)

#define SMEM_SIP_5_10_RXDMA_PIP_IPV6_ETHERTYPE_CONF_REG(dev,unitIndex) \
    SIP_5_10_REG(dev,rxDMA[unitIndex].globalRxDMAConfigs.preIngrPrioritizationConfStatus.IPv6EtherTypeConf)

#define SMEM_SIP_5_10_RXDMA_PIP_VLAN_ETHERTYPE_CONF_N_REG(dev,index,unitIndex) \
    SIP_5_10_REG(dev,rxDMA[unitIndex].globalRxDMAConfigs.preIngrPrioritizationConfStatus.VLANEtherTypeConf[index])

#define SMEM_SIP_5_10_RXDMA_PIP_MPLS_ETHERTYPE_CONF_N_REG(dev,index,unitIndex) \
    SIP_5_10_REG(dev,rxDMA[unitIndex].globalRxDMAConfigs.preIngrPrioritizationConfStatus.MPLSEtherTypeConf[index])

#define SMEM_SIP_5_10_RXDMA_PIP_GENERIC_ETHERTYPE_N_REG(dev,index,unitIndex) \
    SIP_5_10_REG(dev,rxDMA[unitIndex].globalRxDMAConfigs.preIngrPrioritizationConfStatus.genericEtherType[index])

#define SMEM_SIP_5_10_RXDMA_PIP_MAC_DA_4_LSB_BYTES_CONF_N_REG(dev,index,unitIndex) \
    SIP_5_10_REG(dev,rxDMA[unitIndex].globalRxDMAConfigs.preIngrPrioritizationConfStatus.MACDA4LsbBytesConf[index])

#define SMEM_SIP_5_10_RXDMA_PIP_MAC_DA_2_MSB_BYTES_CONF_N_REG(dev,index,unitIndex) \
    SIP_5_10_REG(dev,rxDMA[unitIndex].globalRxDMAConfigs.preIngrPrioritizationConfStatus.MACDA2MsbBytesConf[index])

#define SMEM_SIP_5_10_RXDMA_PIP_MAC_DA_MASK_4_LSB_CONF_N_REG(dev,index,unitIndex) \
    SIP_5_10_REG(dev,rxDMA[unitIndex].globalRxDMAConfigs.preIngrPrioritizationConfStatus.MACDAMask4LsbConf[index])

#define SMEM_SIP_5_10_RXDMA_PIP_MAC_DA_MASK_2_MSB_CONF_N_REG(dev,index,unitIndex) \
    SIP_5_10_REG(dev,rxDMA[unitIndex].globalRxDMAConfigs.preIngrPrioritizationConfStatus.MACDAMask2MsbConf[index])

#define SMEM_SIP_5_10_RXDMA_PIP_VID_CONF_N_REG(dev,index,unitIndex) \
    SIP_5_10_REG(dev,rxDMA[unitIndex].globalRxDMAConfigs.preIngrPrioritizationConfStatus.VIDConf[index])

#define SMEM_SIP_5_10_RXDMA_PIP_IPV6_TC_PRIORITY_ARRAY_PROFILE_N_SEGMENT_M_REG(dev,pip_profile,ipv6_tc,unitIndex) \
    SIP_5_10_REG(dev,rxDMA[unitIndex].globalRxDMAConfigs.preIngrPrioritizationConfStatus.IPv6TCPriorityArrayProfileSegment[pip_profile][(ipv6_tc) / 16])

#define SMEM_SIP_5_10_RXDMA_PIP_UP_DSA_TAG_ARRAY_PROFILE_N_REG(dev,pip_profile,unitIndex) \
    SIP_5_10_REG(dev,rxDMA[unitIndex].globalRxDMAConfigs.preIngrPrioritizationConfStatus.UPDSATagArrayProfile[pip_profile])

#define SMEM_SIP_5_10_RXDMA_PIP_LOW_PRIORITY_DROP_GLOBAL_COUNTER_LSB_REG(dev,unitIndex) \
    SIP_5_10_REG(dev,rxDMA[unitIndex].globalRxDMAConfigs.preIngrPrioritizationConfStatus.PIPLowPriorityDropGlobalCntrLSb)

#define SMEM_SIP_5_10_RXDMA_PIP_LOW_PRIORITY_DROP_GLOBAL_COUNTER_MSB_REG(dev,unitIndex) \
    SIP_5_10_REG(dev,rxDMA[unitIndex].globalRxDMAConfigs.preIngrPrioritizationConfStatus.PIPLowPriorityDropGlobalCntrMsb)

#define SMEM_SIP_5_10_RXDMA_PIP_MED_PRIORITY_DROP_GLOBAL_COUNTER_LSB_REG(dev,unitIndex) \
    SIP_5_10_REG(dev,rxDMA[unitIndex].globalRxDMAConfigs.preIngrPrioritizationConfStatus.PIPMedPriorityDropGlobalCntrLsb)

#define SMEM_SIP_5_10_RXDMA_PIP_MED_PRIORITY_DROP_GLOBAL_COUNTER_MSB_REG(dev,unitIndex) \
    SIP_5_10_REG(dev,rxDMA[unitIndex].globalRxDMAConfigs.preIngrPrioritizationConfStatus.PIPMedPriorityDropGlobalCntrMsb)

#define SMEM_SIP_5_10_RXDMA_PIP_HIGH_PRIORITY_DROP_GLOBAL_COUNTER_LSB_REG(dev,unitIndex) \
    SIP_5_10_REG(dev,rxDMA[unitIndex].globalRxDMAConfigs.preIngrPrioritizationConfStatus.PIPHighPriorityDropGlobalCntrLsb)

#define SMEM_SIP_5_10_RXDMA_PIP_HIGH_PRIORITY_DROP_GLOBAL_COUNTER_MSB_REG(dev,unitIndex) \
    SIP_5_10_REG(dev,rxDMA[unitIndex].globalRxDMAConfigs.preIngrPrioritizationConfStatus.PIPHighPriorityDropGlobalCntrMsb)

#define SMEM_SIP_5_10_RXDMA_PIP_PRIORITY_DROP_GLOBAL_COUNTERS_CLEAR_REG(dev,unitIndex) \
    SIP_5_10_REG(dev,rxDMA[unitIndex].globalRxDMAConfigs.preIngrPrioritizationConfStatus.PIPPriorityDropGlobalCntrsClear)

#define SMEM_SIP_5_10_RXDMA_PIP_SCDMA_N_PIP_CONFIG_REG(dev,port) \
    SIP_5_10_REG(dev,rxDMA[SMEM_DUAL_UNIT_DMA_UNIT_INDEX_GET(dev,port)].singleChannelDMAPip.SCDMA_n_PIP_Config[SMEM_DATA_PATH_RELATIVE_PORT_GET(dev,port)])

#define SMEM_SIP_5_10_RXDMA_PIP_SCDMA_N_DROP_PKT_COUNTER(dev,port) \
    SIP_5_10_REG(dev,rxDMA[SMEM_DUAL_UNIT_DMA_UNIT_INDEX_GET(dev,port)].singleChannelDMAPip.SCDMA_n_Drop_Pkt_Counter[SMEM_DATA_PATH_RELATIVE_PORT_GET(dev,port)])

/* Port<n> LPI Control Registers (0,1) */
#define SMEM_SIP_5_MAC_LPI_CTRL_REG(dev,port,index) \
        ((SMEM_CHT_MAC_REG_DB_GET(dev))->GOP.ports.gigPort[port].lpiControlReg[index])

/* Port<n> LPI status Register  */
#define SMEM_SIP_5_MAC_LPI_STATUS_REG(dev,port) \
        ((SMEM_CHT_MAC_REG_DB_GET(dev))->GOP.ports.gigPort[port].lpiStatus[index])

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __sregBobcat2h */

