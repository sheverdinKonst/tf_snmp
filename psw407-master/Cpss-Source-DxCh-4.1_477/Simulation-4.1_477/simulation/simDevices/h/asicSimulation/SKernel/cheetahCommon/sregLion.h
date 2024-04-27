/******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* sregLion.h
*
* DESCRIPTION:
*       Defines for Lion memory registers access.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 50 $
*
*******************************************************************************/
#ifndef __sregLionh
#define __sregLionh

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* the XLG ports registers are in memory space that match the 'port 12', 'port 12' .
 about the 'XLG port' - the ports in Lion B and above that can be in the
 40G speed is port 10 in Lion B(8 and 11 in Lion2) but the actual memory space is as if the
 port is 12 (12 and 14 for Lion2) */
#define SMEM_LION_XLG_PORT_12_NUM_CNS      12
#define SMEM_LION_XLG_PORT_14_NUM_CNS      14

/* Extended Global Control Register - Lion2 / Lion3 */
#define SMEM_LION_EXT_GLB_CTRL_REG(dev)                                       \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->MG.globalConfig.extGlobalCtrl : \
        (0x0000005C))

/*XG Ports Summary Interrupt Cause register*/
#define SMEM_LION_XG_PORTS_SUMMARY_INTERRUPT_CAUSE_REG(dev,port) \
    ((dev)->supportRegistersDb ? \
       (SMEM_CHT_MAC_REG_DB_GET(dev))->GOP.ports.XLGIP[port].externalUnitsInterruptsCause  : \
            (CHT_MAC_PORTS_BASE_ADDRESS_CNS(dev,port) + 0x58 + \
             0x400*(port))) /*0x08800058 + (0x400*port)*/

/* Egress Stp Table */
#define SMEM_LION_EGR_STP_TBL_MEM_REG(dev, entry_index)         \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, egressStp, entry_index)

/* Egress Vlan Table */
#define SMEM_LION_EGR_VLAN_TBL_MEM_REG(dev, entry_index)         \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, egressVlan, entry_index)

/* Device Map Table Address Construct */
#define SMEM_LION_TXQ_DEV_MAP_MEM_CONSTRUCT_REG(dev,_profile)        \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EGF_eft.deviceMapConfigs.devMapTableAddrConstructProfile[_profile] : \
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
        SMAIN_NOT_VALID_CNS :                                         \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer1.TXQ.TXQ_EGR[0].devMapTableAddrConstruct : \
    (0x01801100)))

/* Local Src Port Map Own Dev En <%n> <0-1> */
#define SMEM_LION_TXQ_LOCAL_SRC_PORT_MAP_OWN_DEV_EN_REG(dev, _egrSubUnit)        \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EGF_eft.deviceMapConfigs.localSrcPortMapOwnDevEn[0] : \
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
        SMAIN_NOT_VALID_CNS :                                         \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer1.TXQ.TXQ_EGR[_egrSubUnit].localSrcPortMapOwnDevEn[0] : \
    (0x01801110)))

/* Local Trg Port Map Own Dev En <%n> <0-1> */
#define SMEM_LION_TXQ_LOCAL_TRG_PORT_MAP_OWN_DEV_EN_REG(dev, _egrSubUnit)        \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EGF_eft.deviceMapConfigs.localTrgPortMapOwnDevEn[0] : \
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
        SMAIN_NOT_VALID_CNS :                                         \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer1.TXQ.TXQ_EGR[_egrSubUnit].localTrgPortMapOwnDevEn[0]: \
    (0x01801120)))

/* Egress Filters Global Enables */
#define SMEM_LION_TXQ_EGR_FILTER_GLOBAL_EN_REG(dev)                 \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EGF_eft.egrFilterConfigs.egrFiltersEnable : \
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
        SMAIN_NOT_VALID_CNS :                                         \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer1.TXQ.TXQ_EGR[0].egrFiltersGlobalEnables : \
    (0x01810000)))

/* Unknown UC Filter En <%n> */
#define SMEM_LION_TXQ_UNKN_UC_FILTER_EN_REG(dev, index)\
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EGF_eft.egrFilterConfigs.unknownUCFilterEn[index] : \
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
        SMAIN_NOT_VALID_CNS :                                         \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer1.TXQ.TXQ_EGR[0].unknownUCFilterEn[index] : \
    (0x01810010 | ((index) * 0x4))))

/* Unregistered MC Filter En <%n> */
#define SMEM_LION_TXQ_UNREG_MC_FILTER_EN_REG(dev, index)\
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EGF_eft.egrFilterConfigs.unregedMCFilterEn[index] : \
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
        SMAIN_NOT_VALID_CNS :                                         \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer1.TXQ.TXQ_EGR[0].unregedMCFilterEn[index] : \
    (0x01810020 | ((index) * 0x4))))

/* Unregistered BC Filter En <%n> */
#define SMEM_LION_TXQ_UNREG_BC_FILTER_EN_REG(dev, index)\
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EGF_eft.egrFilterConfigs.unregedBCFilterEn[index] : \
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
        SMAIN_NOT_VALID_CNS :                                         \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer1.TXQ.TXQ_EGR[0].unregedBCFilterEn[index] : \
    (0x01810030 | ((index) * 0x4))))

/* MC Local En <%n> */
#define SMEM_LION_TXQ_UNREG_MC_LOCAL_EN_REG(dev, index)\
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EGF_sht.global.MCLocalEn[index] : \
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
        SMAIN_NOT_VALID_CNS :                                         \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer1.TXQ.TXQ_EGR[0].MCLocalEn[index] : \
    (0x01810040 | ((index) * 0x4))))

/* UC Src ID Filter En <%n> */
#define SMEM_LION_TXQ_UC_SRC_ID_FILTER_EN_REG(dev, index)\
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EGF_sht.global.UCSrcIDFilterEn[index] : \
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
        SMAIN_NOT_VALID_CNS :                                         \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer1.TXQ.TXQ_EGR[0].UCSrcIDFilterEn[index] : \
    (0x01810060 | ((index) * 0x4))))

/* Designated Port Table */
#define SMEM_LION_TXQ_DESIGNATED_PORT_TBL_MAC(dev, entry)    \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, designatedPorts, entry)

/* SHT Global Configurations */
#define SMEM_LION_TXQ_SHT_GLOBAL_CONF_REG(dev)                      \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EGF_sht.global.SHTGlobalConfigs : \
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
        SMAIN_NOT_VALID_CNS :                                         \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer1.TXQ.TXQ_SHT.global.SHTGlobalConfigs : \
    (0x118F0000)))

/* Distributor General Configurations */
#define SMEM_LION_TXQ_DISTR_GENERAL_CONF_REG(dev) \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EGF_qag.distributor.distributorGeneralConfigs : \
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer0.DPToCFIMap.DP2CFIReg : \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer1.TXQ.TXQ_Queue.CongestionNotification.distributorGeneralConfigs : \
    0x0A091000))

/* From CPU Forward Restricted <%n> */
#define SMEM_LION_TXQ_FROM_CPU_FWRD_RESTRICT_REG(dev)               \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EGF_sht.global.eportFromCPUForwardRestricted[0] : \
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
        SMAIN_NOT_VALID_CNS :                                         \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer1.TXQ.TXQ_Queue.CongestionNotification.fromCPUForwardRestricted[0] : \
    (0x0A091090)))

/* Bridged Forward Restricted <%n> */
#define SMEM_LION_TXQ_BRDG_FWRD_RESTRCIT_REG(dev)                   \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EGF_sht.global.eportBridgedForwardRestricted[0] : \
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
        SMAIN_NOT_VALID_CNS :                                         \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer1.TXQ.TXQ_Queue.CongestionNotification.bridgedForwardRestricted[0] : \
    (0x0A0910A0)))
/* Routed Forward Restricted <%n> */
#define SMEM_LION_TXQ_ROUT_FWRD_RESTRCIT_REG(dev)                   \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EGF_sht.global.eportRoutedForwardRestricted[0] : \
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
        SMAIN_NOT_VALID_CNS :                                         \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer1.TXQ.TXQ_Queue.CongestionNotification.routedForwardRestricted[0] : \
    (0x0A0910B0)))

/* TxQ MIB Counters Set <%n> Configuration */
#define SMEM_LION_TXQ_MIB_CNT_SET_CONF_REG(dev, index)\
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->SIP5_TXQ_QUEUE.peripheralAccess.egrMIBCntrs.txQMIBCntrsSetConfig[index] : \
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
        SMAIN_NOT_VALID_CNS :                                         \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer1.TXQ.TXQ_Queue.PeripheralAccess.txQMIBCntrsSetConfig[index] : \
    (0x0A093200  | ((index) * 0x4))))
/* Set <%n> Outgoing Unicast Packet Counter */
#define SMEM_LION_TXQ_SET_OUT_UC_PCKT_CNT_REG(dev, index)\
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->SIP5_TXQ_QUEUE.peripheralAccess.egrMIBCntrs.setOutgoingUcPktCntr[index] : \
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
        SMAIN_NOT_VALID_CNS :                                         \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer1.TXQ.TXQ_Queue.PeripheralAccess.setOutgoingUcPktCntr[index] : \
    (0x0A093210  | ((index) * 0x4))))
/* Set <%n> Outgoing Multicast Packet Counter */
#define SMEM_LION_TXQ_SET_OUT_MC_PCKT_CNT_REG(dev, index)\
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->SIP5_TXQ_QUEUE.peripheralAccess.egrMIBCntrs.setOutgoingMcPktCntr[index] : \
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
        SMAIN_NOT_VALID_CNS :                                         \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer1.TXQ.TXQ_Queue.PeripheralAccess.setOutgoingMcPktCntr[index] : \
    (0x0A093220  | ((index) * 0x4))))
/* Set <%n> Outgoing Broadcast Packet Counter */
#define SMEM_LION_TXQ_SET_OUT_BC_PCKT_CNT_REG(dev, index)\
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->SIP5_TXQ_QUEUE.peripheralAccess.egrMIBCntrs.setOutgoingBcPktCntr[index] : \
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
        SMAIN_NOT_VALID_CNS :                                         \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer1.TXQ.TXQ_Queue.PeripheralAccess.setOutgoingBcPktCntr[index] : \
    (0x0A093230  | ((index) * 0x4))))
/* Set <%n> Bridge Egress Filtered Packet Counter */
#define SMEM_LION_TXQ_SET_BRDG_EGR_FILTER_PCKT_CNT_REG(dev, index)\
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->SIP5_TXQ_QUEUE.peripheralAccess.egrMIBCntrs.setBridgeEgrFilteredPktCntr[index] : \
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
        SMAIN_NOT_VALID_CNS :                                         \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer1.TXQ.TXQ_Queue.PeripheralAccess.setBridgeEgrFilteredPktCntr[index] : \
    (0x0A093240  | ((index) * 0x4))))
/* Set <%n> Control Packet Counter */
#define SMEM_LION_TXQ_SET_CTRL_PCKT_CNT_REG(dev, index)\
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->SIP5_TXQ_QUEUE.peripheralAccess.egrMIBCntrs.setCtrlPktCntr[index] : \
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
        SMAIN_NOT_VALID_CNS :                                         \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer1.TXQ.TXQ_Queue.PeripheralAccess.setCtrlPktCntr[index] : \
    (0x0A093260  | ((index) * 0x4))))
/* Set <%n> Egress Forwarding Restrictions Dropped Packet Counter */
#define SMEM_LION_TXQ_SET_EGR_FWD_RESTRICTIONS_DROPPED_PCKT_CNT_REG(dev, index)\
    /*Reg is turned to Internal in SIP5, because the EGF no longer drops packets in the "egf distributor" stage (the signal egf2txq_egr_fwd_clr_ is always OFF)*/ \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMAIN_NOT_VALID_CNS : \
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
        SMAIN_NOT_VALID_CNS :                                       \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->                            \
            egrAndTxqVer1.TXQ.TXQ_Queue.PeripheralAccess.setEgrForwardingRestrictionDroppedPktsCntr[index] : \
    (0x0A093270  | ((index) * 0x4))))

/* Total Desc Counter Register */
#define SMEM_LION_TXQ_TOTAL_DESC_COUNT_REG(dev)                     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->SIP5_TXQ_QUEUE.tailDrop.tailDropCntrs.totalDescCntr : \
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
        SMAIN_NOT_VALID_CNS :                                         \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer1.TXQ.TXQ_Queue.totalDescCntr : \
    (0x0A0A1000)))
/* Resource Histogram Limit Register 1 */
#define SMEM_LION_TXQ_RES_HIST_LIMIT_1_REG(dev)                     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->SIP5_TXQ_QUEUE.tailDrop.resourceHistogram.resourceHistogramLimits.resourceHistogramLimitReg1 : \
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
        SMAIN_NOT_VALID_CNS :                                         \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer1.TXQ.TXQ_Queue.ResourceHistogram.resourceHistogramLimitReg1 : \
    (0x0A0A6300)))
/* Resource Histogram Counter  */
#define SMEM_LION_TXQ_RES_HIST_COUNT_REG(dev)                       \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->SIP5_TXQ_QUEUE.tailDrop.resourceHistogram.resourceHistogramCntrs.resourceHistogramCntr[0] : \
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
        SMAIN_NOT_VALID_CNS :                                         \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer1.TXQ.TXQ_Queue.ResourceHistogram.resourceHistogramCntr[0] : \
    (0x0A0A6310)))

/* CNC Blocks Counter Entry Mode Register%r */
#define SMEM_LION_CNC_BLOCK_CNF0_REG(dev, block,_cncUnitIndex) \
    SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->CNC[_cncUnitIndex].perBlockRegs.clientEnable.CNCBlockConfigReg0[block] : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->centralizedCntrs.perBlockRegs.CNCBlockConfigReg[0][block] : \
        (0x08001180 + ((block) * 0x100)))

/* CNC Block %n Entry Mode */
#define SMEM_SIP5_20_CNC_BLOCK_ENTRY_MODE_REG(dev, block,_cncUnitIndex) \
    SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->CNC[_cncUnitIndex].perBlockRegs.entryMode.CNCBlocksCounterEntryModeRegister[block/8] : \
    SMAIN_NOT_VALID_CNS


/* Ports Link Enable Status */
#define SMEM_LION_TXQ_PORTS_LINK_ENABLE_STATUS_REG(dev,_macPort) \
    (SMEM_CHT_IS_SIP5_GET(dev) ? (SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->SIP5_TXQ_LL.global.portsLinkEnableStatus.portsLinkEnableStatus[0] + ((_macPort/32)*4)) : \
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer0.transmitQueueGlobalConfig.portsLinkEnable : \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer1.TXQ.TXQ_LL.global.portsLinkEnableStatus0 : \
      (dev)->txqRevision == 0 ? (0x018002F0) : (0x108A0100)))

/* Transmit Queue DQ Interrupt Summary Cause */
#define SMEM_LION_TXQ_DQ_INTR_SUM_CAUSE_REG(dev,dpUnitInPipe)                    \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->SIP5_TXQ_DQ[dpUnitInPipe].global.interrupt.txQDQInterruptSummaryCause : \
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
        SMAIN_NOT_VALID_CNS :                                         \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer1.TXQ.TXQ_DQ.txQDQInterruptSummaryCause : \
    (0x11000100)))
/* Transmit Queue DQ Interrupt Summary Mask */
#define SMEM_LION_TXQ_DQ_INTR_SUM_MASK_REG(dev,dpUnitInPipe)                     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->SIP5_TXQ_DQ[dpUnitInPipe].global.interrupt.txQDQInterruptSummaryMask : \
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
        SMAIN_NOT_VALID_CNS :                                         \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer1.TXQ.TXQ_DQ.txQDQInterruptSummaryMask : \
    (0x11000104)))

/* Transmit Queue Egress STC Interrupt Cause */
#define SMEM_LION_TXQ_EGR_STC_INTR_CAUSE_REG(dev,_port,dpUnitInPipe)                   \
    (SMEM_CHT_IS_SIP5_GET(dev) ? (SMEM_CHT_IS_SIP5_15_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->SIP5_TXQ_DQ[dpUnitInPipe].global.interrupt.egrSTCInterruptCause[((_port%72) / 31)] : \
        SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->SIP5_TXQ_DQ[dpUnitInPipe].global.interrupt.egrSTCInterruptCause[((_port%72) / 18)]) : \
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
        SMAIN_NOT_VALID_CNS :                                         \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer1.TXQ.TXQ_DQ.egrSTCInterruptCause : \
    (0x11000118)))
/* Transmit Queue Egress STC Interrupt Cause */
#define SMEM_LION_TXQ_EGR_STC_INTR_MASK_REG(dev,_port,dpUnitInPipe)                    \
    (SMEM_CHT_IS_SIP5_GET(dev) ? (SMEM_CHT_IS_SIP5_15_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->SIP5_TXQ_DQ[dpUnitInPipe].global.interrupt.egrSTCInterruptMask[((_port%72) / 31)] : \
        SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->SIP5_TXQ_DQ[dpUnitInPipe].global.interrupt.egrSTCInterruptMask[((_port%72) / 18)]) : \
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
        SMAIN_NOT_VALID_CNS :                                         \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer1.TXQ.TXQ_DQ.egrSTCInterruptMask : \
    (0x1100011C)))

/* Dequeue enable register (the same as transmission enable for single core devices)*/
#define SMEM_LION_DEQUEUE_ENABLE_REG(dev,port,dpUnitInPipe)                        \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->SIP5_TXQ_DQ[dpUnitInPipe].shaper.portDequeueEnable[port] : \
    ((dev)->supportRegistersDb ?                                      \
      (dev)->txqRevision == 0 ?                                       \
        SMAIN_NOT_VALID_CNS :                                         \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer1.TXQ.TXQ_DQ.Scheduler.portDequeueEnable[port] : \
        ((0x11002014)+0x4*(port))))

/* Egress Analyzer Enable */
#define SMEM_LION_TXQ_EGR_ANALYZER_EN_REG(dev,index,dpUnitInPipe)                      \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->SIP5_TXQ_DQ[dpUnitInPipe].statisticalAndCPUTrigEgrMirrToAnalyzerPort.egrAnalyzerEnableIndex[index] : \
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
        SMAIN_NOT_VALID_CNS :                                         \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer1.TXQ.TXQ_DQ.Scheduler.egrAnalyzerEnable : \
    (0x11004010)))


/* CPU Port Distribution */
#define SMEM_LION_TXQ_CPU_PORT_DISRIBUTE_REG(dev)                   \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EGF_eft.global.cpuPortDist : \
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
        SMAIN_NOT_VALID_CNS :                                         \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer1.TXQ.TXQ_EGR[0].cpuPortDist : \
    (0x01800000)))

/* Hep Interrupt Summary Cause */
#define SMEM_LION_HEP_INTR_CAUSE_REG(dev) \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->HA.HAInterruptCause : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->HA.hepInterruptSummaryCause  :  \
        0x0E8003A8))

/* Start Inter-Port Group Ring Forwarding Registers and macros*/

/* Multi-Port Group Lookup0 Registers */
#define SMEM_LION_MULTI_PORT_GROUP_LOOKUP0_REG(dev)                 \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMAIN_NOT_VALID_CNS : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->EQ.multiCoreLookup.multiCoreLookup[0] : \
    (0x0B00007C)))

/* Port Group Ring Configuration Register */
#define SMEM_LION_HA_PORT_GROUP_RING_CONF_REG(dev)                     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMAIN_NOT_VALID_CNS : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->HA.coreRingConfig[0] : \
    (0x0E800140)))

/* TTI Internal configurations */
#define SMEM_LION_TTI_INTERNAL_CONF_REG(dev)                        \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.TTI_Internal.TTIInternalConfig : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->TTI.TTIInternalConfigurations : \
    (0x01000204)))
/* Port Group Ring Port Enable */
#define SMEM_LION_TTI_PORT_GROUP_RING_PORT_ENABLE_REG(dev)          \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMAIN_NOT_VALID_CNS : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->TTI.coreRingPortEnable[0] : \
    (0x010000F8)))

/* Tunnel-Termination and Interface (TTI) Lookup */

/* Statistical and CPU-Triggered
    Egress Mirroring to Analyzer Port Config. register , Cht/Cht2/Cht3 */
#define SMEM_LION_STAT_EGRESS_MIRROR_REG(dev,dpUnitInPipe)                       \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->SIP5_TXQ_DQ[dpUnitInPipe].statisticalAndCPUTrigEgrMirrToAnalyzerPort.STCStatisticalTxSniffConfig : \
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
        SMAIN_NOT_VALID_CNS :                                         \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer1.TXQ.TXQ_DQ.Scheduler.STCStatisticalTxSniffConfig : \
    (0x11004000)))
/* Trunk Hash Configuration Register0 */
#define SMEM_LION_TTI_TRUNK_HASH_CONF0_REG(dev)                     \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.trunkHashSimpleHash.trunkHashConfigReg0 : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->TTI.trunkHash.trunkHashConfigReg0 : \
    (0x01000070)))

/* User Defined Ethertypes_%i */
#define SMEM_LION_TTI_USER_DEFINED_ETHERTYPES_REG(dev,index)              \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_LION3_TTI_PCL_USER_DEFINED_ETHER_TYPES_REG(dev,index) : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->TTI.userDefineEtherTypes[index] : \
    (0x010000F0 + 4*(index))))
/* PTP Ethertypes */
#define SMEM_LION_TTI_PTP_ETHERTYPES_REG(dev)                       \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.PTP.PTPEtherTypes : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->TTI.PTPEtherTypes : \
    (0x010001A4)))
/* PTP over UDP Destination Ports */
#define SMEM_LION_TTI_PTP_OVER_UDP_DEST_PORT_REG(dev)               \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.PTP.PTPOverUDPDestinationPorts : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->TTI.PTPOverUDPDestinationPorts : \
    (0x010001A8)))
/* PTP Packet Command Configuration 0 */
#define SMEM_LION_TTI_PTP_PACKET_CMD_CONF0_REG(dev)                 \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMAIN_NOT_VALID_CNS : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->TTI.PTPPktCommandConfig0 : \
    (0x010001AC)))
/* PTP Packet Command Configuration 1 */
#define SMEM_LION_TTI_PTP_PACKET_CMD_CONF1_REG(dev)                 \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMAIN_NOT_VALID_CNS : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->TTI.PTPPktCommandConfig1 : \
    (0x010001BC)))
/* TTI Unit Interrupt Summary Cause */
#define SMEM_LION_TTI_SUM_CAUSE_REG(dev)    \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.TTIEngine.TTIEngineInterruptCause : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->TTI.TTIUnitInterruptSummaryCause  :  \
    0x010001C0))
/*CFI_UP To QoS-Profile Mapping Table Selector -- each 4 ports in register */
#define SMEM_LION_TTI_CFI_UP_TO_QOS_PROFILE_MAPPING_TABLE_SELECTOR_REG(dev,port) \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMAIN_NOT_VALID_CNS : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->TTI.qoSMapTables.CFI_UPToQoSProfileMapTableSelector[(port)/4]  :  \
    0x01000470 + (4*((port)/4))))
/* Policy Engine User Defined Bytes Extended Configuration */
#define SMEM_LION_PCL_UDB_EXTEND_CONF_REG(dev) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->PCL.policyEngineUserDefinedBytesExtConfig : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->IPCL.policyEngineUserDefinedBytesExtConfig  :  \
    0x0B800024))
/* CRC Hash Configuration */
#define SMEM_LION_PCL_CRC_HASH_CONF_REG(dev) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->PCL.HASH.CRCHashConfig : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->IPCL.CRCHashConfig  :  \
    0x0B800020))
/* Pearson Hash Table */
#define SMEM_LION_PCL_PEARSON_HASH_TBL_MEM(dev, entry_index) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, pearsonHash, entry_index)

/* CRC Hash Mask Memory */
#define SMEM_LION_PCL_CRC_HASH_MASK_TBL_MEM(dev, entry_index) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, crcHashMask, entry_index)

/* Mirror to Analyzer Header Configuration Register */
#define SMEM_LION_HA_MIRR_TO_ANALYZER_HEADER_CONF_REG(dev) \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMAIN_NOT_VALID_CNS : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->HA.mirrorToAnalyzerHeaderConfig[0]  :  \
    0x0E800080))

/* Replace VID With UserID */
#define SMEM_LION_HA_REPLACE_VID_WITH_USER_ID_REG(dev)              \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMAIN_NOT_VALID_CNS : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->HA.replaceVIDWithUserID[0] : \
    (0x0E800150)))

/* Keep VLAN1 table */
#define SMEM_LION_HA_UP0_PORT_KEEP_VLAN1_TBL_MEM(dev, entry_index) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, haUp0PortKeepVlan1, entry_index)

/* Cut-Through Feature Enable Per Port */
#define SMEM_LION_RXDMA_CT_ENABLE_PER_PORT_REG(dev)                 \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->RXDMA.cutThrough.CTFeatureEnablePerPort : \
    (0x0F000010))

/* Cut-Through EtherType Identification */
#define SMEM_LION_RXDMA_CT_ETHERTYPE_REG(dev,unitIndex)                       \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->rxDMA[unitIndex].globalRxDMAConfigs.globalConfigCutThrough.CTEtherTypeIdentification : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->RXDMA.cutThrough.CTEtherTypeIdentification : \
    (0x0F000014)))

/* Cut-Through UP Enable */
#define SMEM_LION_RXDMA_CT_UP_REG(dev,unitIndex)                              \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->rxDMA[unitIndex].globalRxDMAConfigs.globalConfigCutThrough.CTUPEnable : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->RXDMA.cutThrough.CTUPEnable : \
    (0x0F000018)))

/* Cut-Through Packet Identification */
#define SMEM_LION_RXDMA_CT_PACKET_IDENTIFY_REG(dev,RxDMA_localPhysicalPort)                 \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->rxDMA[SMEM_DUAL_UNIT_DMA_UNIT_INDEX_GET(dev,RxDMA_localPhysicalPort)].singleChannelDMAConfigs.SCDMACTPktIndentification[SMEM_DATA_PATH_RELATIVE_PORT_GET(dev,RxDMA_localPhysicalPort)] : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->RXDMA.cutThrough.CTPktIndentification : \
    (0x0F00001C)))

/* Cut-Through Global Configuration */
#define SMEM_LION_MG_CT_GLOBAL_CONF_REG(dev)                        \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->MG.globalConfig.CTGlobalConfig : \
    (0x00000390))

/* Router Per-Port SIP/SA Enable Register */
#define SMEM_LION_ROUTER_PER_PORT_SIPSA_REG(dev)                    \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->IPvX.routerGlobalCtrl.routerPerPortSIPSAEnable0 : \
    (0x02800014))

/* Router Per-Vlan uRPF mode table */
#define SMEM_LION_ROUTER_PER_VLAN_URPF_MODE_TBL_MEM(dev) \
    ((dev)->supportRegistersDb ? ((SMEM_CHT_MAC_REG_DB_GET(dev))->IPvX.startUnitInfo.unitBaseAddress + 0x1400): \
    (0x02801400))

/* Egress Policy Global Configuration */
#define SMEM_LION_EPCL_GLOBAL_REG(dev)                              \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->EPCL.egrPolicyGlobalConfig : \
    (0x0e000000))

/* Egress Policy TO CPU Configuration */
#define SMEM_LION_EPCL_TO_CPU_CONFIG_REG(dev)                       \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->EPCL.egrPolicyToCpuConfig[0] : \
    (0x0E000040))


/* Egress Policy FROM CPU data Configuration */
#define SMEM_LION_EPCL_FROM_CPU_DATA_CONFIG_REG(dev)                     \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->EPCL.egrPolicyFromCpuDataConfig[0] : \
    (0x0E000050))

/* Egress Policy FROM CPU control Configuration */
#define SMEM_LION_EPCL_FROM_CPU_CONFIG_REG(dev)                     \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->EPCL.egrPolicyFromCpuCtrlConfig[0] : \
    (0x0E000060))

/* Egress Policy TO ANALYZER Configuration */
#define SMEM_LION_EPCL_TO_ANALYZER_CONFIG_REG(dev)                  \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->EPCL.egrPolicyToAnalyzerConfig[0] : \
    (0x0E000070))

/* Egress Policy Non TS Data Pkts Configuration */
#define SMEM_LION_EPCL_NTS_FORWARD_CONFIG_REG(dev)                  \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->EPCL.egrPolicyNonTSDataPktsConfig[0] : \
    (0x0E000020))

/* Egress Policy TS Data Pkts Configuration */
#define SMEM_LION_EPCL_TS_FORWARD_CONFIG_REG(dev)                   \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->EPCL.egrPolicyTSDataPktsConfig[0] : \
    (0x0E000030))

/* Egress PCL Configuration Table */
#define SMEM_LION_EPCL_CONFIG_TBL_MEM(dev, index) \
    (0x0E008000 + 0x4 * (index))

/* Egress Policy Configuration Table Access mode Configuration */
#define SMEM_LION_EPCL_TBL_ACCESS_MODE_CONFIG_REG(dev)              \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->EPCL.egrPolicyConfigTableAccessModeConfig[0] : \
    (0x0E000010))

/* Timestamp Queue related registers */

    /*
    get the base address of the timestamp units
    direction - gtsIngress
    direction - gtsEgress
*/
#define SMEM_LION_GTS_BASE_ADDR_MAC(dev, direction)      \
        (dev)->memUnitBaseAddrInfo.gts[direction]

/*
    get the register address according to offset from the base address of the
    timestamp unit

    ingress - tsIngress
    egress  - tsEgress

    offset - register offset from start of the unit
*/
#define SMEM_LION_GTS_OFFSET_MAC(dev, direction, offset)      \
    (SMEM_LION_GTS_BASE_ADDR_MAC(dev, direction) + (offset))

/* Global Configurations */
#define SMEM_LION_GTS_SIP_GLOBAL_REG(dev, direction) \
    SMEM_LION_GTS_OFFSET_MAC(dev, direction, 0x0)
/* Enable Timestamping */
#define SMEM_LION_GTS_TIME_STAMP_EN_REG(dev, direction) \
    SMEM_LION_GTS_OFFSET_MAC(dev, direction, 0x4)
/* Timestamping Port Enable */
#define SMEM_LION_GTS_TIME_STAMP_PORT_EN_REG(dev, direction) \
    SMEM_LION_GTS_OFFSET_MAC(dev, direction, 0x8)
/* TOD Counter - Nanoseconds */
#define SMEM_LION_GTS_TOD_COUNT_NANOSEC_REG(dev, direction) \
    SMEM_LION_GTS_OFFSET_MAC(dev, direction, 0x10)
/* TOD Counter(Shadow) - Nanoseconds */
#define SMEM_LION_GTS_TOD_SHADOW_COUNT_NANOSEC_REG(dev, direction) \
    SMEM_LION_GTS_OFFSET_MAC(dev, direction, 0x1C)
/* GTS Interrupt Cause */
#define SMEM_LION_GTS_INTERRUPT_CAUSE_REG(dev, direction) \
    SMEM_LION_GTS_OFFSET_MAC(dev, direction, 0x28)
/* GTS Interrupt Mask */
#define SMEM_LION_GTS_INTERRUPT_MASK_REG(dev, direction) \
    SMEM_LION_GTS_OFFSET_MAC(dev, direction, 0x2C)
/* Global FIFO Current Entry [31:0] */
#define SMEM_LION_GTS_GLOBAL_FIFO_CUR_ENTRY_REG(dev, direction) \
    SMEM_LION_GTS_OFFSET_MAC(dev, direction, 0x30)
/* Overrun Status */
#define SMEM_LION_GTS_OVERRUN_STATUS_REG(dev, direction) \
    SMEM_LION_GTS_OFFSET_MAC(dev, direction, 0x38)
/* Underrun Status */
#define SMEM_LION_GTS_UNDERRUN_STATUS_REG(dev, direction) \
    SMEM_LION_GTS_OFFSET_MAC(dev, direction, 0x3c)

/* Port Egress Mirror Index */
#define SMEM_LION_EGR_MIRROR_INDEX_REG(dev, port) \
    (((port) < 30) ? (0x0b00b010 | (((port) / 10) * 0x4)) : \
     ((port) < 60) ? (0x0B00B044 | (((port - 30) / 10) * 0x4)) : (0x0B00B070 | ((port - 60) / 10)))

/* Egress Monitoring Enable Configuration Register - by index  */
#define SMEM_LION_EGR_MON_EN_CONF_REG_BY_INDEX_REG(dev,index)                         \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMAIN_NOT_VALID_CNS : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->EQ.mirrToAnalyzerPortConfigs.egrMonitoringEnableConfig[index] : \
    (index == 0 ? SMEM_CHT_EGR_MON_EN_CONF_REG(dev): 0x0B000070)))

/* Cascade Egress Monitoring Enable Configuration  */
#define SMEM_LION_EQ_CASCADE_EGRESS_MONITORING_ENABLE_CONFIGURATION_REG(dev)                         \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMAIN_NOT_VALID_CNS : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->EQ.mirrToAnalyzerPortConfigs.cascadeEgressMonitoringEnableConfiguration : \
    (0x0B000074)))


/* DQ Metal Fix Register */
#define SMEM_LION_TXQ_DQ_METAL_FIX_REG(dev)                    \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMAIN_NOT_VALID_CNS : \
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
        SMAIN_NOT_VALID_CNS :                                         \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer1.TXQ.TXQ_DQ.dqMetalFixRegister : \
    (0x11000180)))

/* RxDMA - bmp of Cascade port enable (used by ingress pipe -- detached from the HA unit) */
#define SMEM_LION_RXDMA_CUT_THROUGH_CASCADING_PORT_REG(dev)        \
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
        SMAIN_NOT_VALID_CNS :                                         \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->RXDMA.cutThrough.cascadingPort : \
    (0x0F000020))/* in Lion 0x0F000020 */

/* CFM EtherTypes */
#define SMEM_LION_CFM_ETHERTYPE_REG(dev)                       \
    (SMEM_CHT_IS_SIP5_GET(dev) ?                                \
    ((dev)->pclSupport.pclUseCFMEtherType ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->PCL.CFMEtherType : SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->TTI.CFMEtherType) : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->TTI.CFMEtherType : \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev) ? 0x010001c8 :         \
         0x0c0001c8))



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __sregLionh */

