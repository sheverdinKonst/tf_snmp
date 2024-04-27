/******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* sregCheetah3.h
*
* DESCRIPTION:
*       Defines for Cheetah3 memory registers access.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 56 $
*
*******************************************************************************/
#ifndef __sregCheetah3h
#define __sregCheetah3h

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* ingress PCL Configuration Table Entry<n> - Base address*/
#define SMEM_CHT3_IPCL_CONFIG_TBL_MEM                               (0x0B840000)

    /* Policer Counters Table Data0 Access Register */
#define SMEM_CHT3_POLICER_CNT_TBL_DATA0_REG(dev)\
((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->IPLR.ingrPolicerTableAccessData[0] : \
0x0C000030)

/* VLAN Table 4K Entries */
#define SMEM_CHT3_VLAN_TBL_MEM                                      (0x0BC00000)

/* Span State Group<n> Entry (0<=n<256) * 2 words in step 0x8 */
#define SMEM_CHT3_STP_TBL_MEM                                       (0x0BB00000)


#define SMEM_CHT3_PCL_CONFIG_TBL_MEM(entry_indx)                         \
            (SMEM_CHT3_IPCL_CONFIG_TBL_MEM + (entry_indx * 0x8))

/* address of the policy tcam table */
#define SMEM_CHT3_PCL_TCAM_DATA_TBL_MEM(dev, entry, word) \
    SMEM_CHT_PCL_TCAM_TBL_MEM(dev, entry, word)

/* address of the policy tcam ctrl table */
#define SMEM_CHT3_PCL_TCAM_CTRL_TBL_MEM(dev, entry, word) \
    (SMEM_CHT3_PCL_TCAM_DATA_TBL_MEM(dev, entry, word) + 0x4)

/* address of the policy tcam data mask table */
#define SMEM_CHT3_PCL_TCAM_MASK_TBL_MEM(dev, entry, word) \
    (SMEM_CHT3_PCL_TCAM_DATA_TBL_MEM(dev, entry, word) + 0x20)

/* address of the policy tcam ctrl mask table */
#define SMEM_CHT3_PCL_TCAM_CTRL_MASK_TBL_MEM(dev, entry, word) \
    (SMEM_CHT3_PCL_TCAM_DATA_TBL_MEM(dev, entry, word) + 0x24)

/* address of the routing tcam table */
#define SSMEM_CHT3_TCAM_X_DATA_TBL_MEM(dev)       \
    SMEM_TABLE_ENTRY_2_PARAMS_INDEX_GET_MAC(dev,routerTcam,0,0)

/* address of the routing tcam ctrl table */
#define SSMEM_CHT3_TCAM_ROUTER_TT_X_CONTROL_TBL_MEM(dev)          (SSMEM_CHT3_TCAM_X_DATA_TBL_MEM(dev)+0x4)

/* address of the routing tcam data mask table */
#define SSMEM_CHT3_TCAM_Y_DATA_TBL_MEM(dev)                       (SSMEM_CHT3_TCAM_X_DATA_TBL_MEM(dev)+0x8)

/* address of the routing tcam ctrl mask table */
#define SSMEM_CHT3_TCAM_ROUTER_TT_Y_CONTROL_TBL_MEM(dev)          (SSMEM_CHT3_TCAM_X_DATA_TBL_MEM(dev)+0xc)

/* cheetah3 pcl counter ??? */
#define SMEM_CHT3_PCL_RULE_MATCH_TBL_MEM                            (0x0B801000)

/* Egress Policy Global Registry */
#define SMEM_CHT3_EPCL_GLOBAL_REG(dev) \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EPCL.egrPolicyGlobalConfig : \
    SKERNEL_IS_XCAT_REVISON_A1_DEV(dev) ? \
        SMEM_XCAT_EPCL_GLOBAL_REG(dev) : (0x07C00000))

/* Egress Policy TO CPU Configuration */
#define SMEM_CHT3_EPCL_TO_CPU_CONFIG_REG(dev) \
    SKERNEL_IS_XCAT_REVISON_A1_DEV(dev) ? \
        SMEM_XCAT_EPCL_TO_CPU_CONFIG_REG(dev) : (0x07C00010)

/* Egress Policy FROM CPU -data - Configuration */
#define SMEM_CHT3_EPCL_FROM_CPU_DATA_CONFIG_REG(dev) \
    SKERNEL_IS_XCAT_REVISON_A1_DEV(dev) ? \
        SMEM_XCAT_EPCL_FROM_CPU_DATA_CONFIG_REG(dev) : (0x07C00014)

/* Egress Policy FROM CPU -control - Configuration */
#define SMEM_CHT3_EPCL_FROM_CPU_CONFIG_REG(dev) \
    SKERNEL_IS_XCAT_REVISON_A1_DEV(dev) ? \
        SMEM_XCAT_EPCL_FROM_CPU_CONFIG_REG(dev) : (0x07C00018)

/* Egress Policy TO ANALYZER configuration */
#define SMEM_CHT3_EPCL_TO_ANALYZER_CONFIG_REG(dev) \
    SKERNEL_IS_XCAT_REVISON_A1_DEV(dev) ? \
        SMEM_XCAT_EPCL_TO_ANALYZER_CONFIG_REG(dev) : (0x07C0001C)

/* Egress Policy Forward configuration ??? */
#define SMEM_CHT3_EPCL_TS_FORWARD_CONFIG_REG(dev) \
    SKERNEL_IS_XCAT_REVISON_A1_DEV(dev) ? \
        SMEM_XCAT_EPCL_TS_FORWARD_CONFIG_REG(dev) : (0x07C0000C)

/* Egress Policy Forward configuration ??? */
#define SMEM_CHT3_EPCL_NTS_FORWARD_CONFIG_REG(dev) \
    SKERNEL_IS_XCAT_REVISON_A1_DEV(dev) ? \
        SMEM_XCAT_EPCL_NTS_FORWARD_CONFIG_REG(dev) : (0x07C00008)


/* Egress Policy Configuration Table Access mode configuration */
#define SMEM_CHT3_EPCL_TBL_ACCESS_MODE_CONFIG_REG(dev) \
    SKERNEL_IS_XCAT_REVISON_A1_DEV(dev) ? \
        SMEM_XCAT_EPCL_TBL_ACCESS_MODE_CONFIG_REG(dev) : (0x07C00004)


/* UDP Broadcast Destination Port<n> Configuration Table */
#define SMEM_CHT3_UDP_BROADCAST_DEST_PORT_TBL_MEM(entry_index) \
            0x02000500 + ((entry_index) * 4)

/* Ingress VLAN translation table */
#define SMEM_CHT3_INGRESS_VLAN_TRANSLATION_TBL_MEM                  (0x0b804000)

/* Egress VLAN translation table */
#define SMEM_CHT3_EGRESS_VLAN_TRANSLATION_TBL_MEM                   (0x07fc0000)

/* FDB Update Message Queue Configuration Registers ??? */
#define SMEM_CHT3_FU_Q_BASE_ADDR_REG(dev)\
((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->MG.addrUpdateQueueConfig.FUAddrUpdateQueueCtrl : \
0x000000CC)

#define SMEM_CHT3_ROUTER_ACCESS_DATA_CTRL_REG(dev)\
((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->TCCUpperIPvX.TCCIPvX.routerTCAMAccessCtrlReg0 : \
    (SKERNEL_DEVICE_FAMILY_CHEETAH_3_ONLY(dev) ? \
    0x02800418 :                                \
    0x0D800418))

#define SMEM_CHT3_MLL_REGISTERS_BASE_TBL_MEM(dev, offset)      \
    (offset) + (dev)->memUnitBaseAddrInfo.mll

#define SMEM_CHT3_MLL_GLB_CONTROL_REG(dev)\
    SMEM_CHT3_MLL_REGISTERS_BASE_TBL_MEM(dev,0x0)

/* QoS Profile to Route Block Offset Entry ??? */
#define SMEM_CHT3_QOS_ROUTING_TBL_MEM(dev, entry_indx) \
        (0x02800300 + (0x4 * (entry_indx)))

/* Unicast Multicast Router Next Hop Entry ??? */
#define SMEM_CHT3_NEXT_HOP_ENTRY_TBL_MEM(entry_indx)          \
        (0x02B00000 + (0x10 * (entry_indx)))

/* IPv6 Scope Prefix Table */
#define SMEM_CHT3_ROUTER_SCOPE_PREFIX_TBL_MEM(entry_indx) \
        (0x02800250 + (0x4 * (entry_indx)))
/* IPv6 Unicast Scope level Table */
#define SMEM_CHT3_ROUTER_UNICAST_SCOPE_LEVEL_TBL_MEM(entry_indx) \
        (0x02800260 + (0x4 * (entry_indx)))
/* IPv6 Unicast Scope Command Table */
#define SMEM_CHT3_ROUTER_UNICAST_SCOPE_COMMAND_TBL_MEM(entry_indx) \
        (0x02800278 + (0x4 * (entry_indx)))
/* IPv6 Multicast Scope Command Table */
#define SMEM_CHT3_ROUTER_MULTICAST_SCOPE_COMMAND_TBL_MEM(entry_indx) \
        (0x02800288 + (0x4 * (entry_indx)))
/*  Multicast replicator Table  */
#define SMEM_CHT3_ROUTER_MULTICAST_LIST_TBL_MEM(dev,entry_indx) \
        SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev,mll,entry_indx)

#define SMEM_CHT3_VRFID_TBL_MEM(dev, entry_indx)  \
        SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev,vrfId,entry_indx)


#define SMEM_CHT3_QOS_RESERV_OFFSET_INDEX_CNS                        (128)

/* FU upload messages Miscellaneous to CPU has exceeded its configured
  threshold , Cht2 */
#define SMEM_CHT3_MISCELLANEOUS_FU_FULL_INT                          (1 << 11)

/* FU upload Message to CPU is ready in the Miscellaneous  */
#define SMEM_CHT3_MISCELLANEOUS_FU_MSG_PENDING_INT                   (1 << 12)

/* Egress PCL Configuration Table Entry<n> (0<=n<4159)          */
/* offset formula : 0x07F00000 + entry<n>*0x4                   */
#define SMEM_CHT3_EPCL_CONFIG_TBL_MEM(dev, entry_indx)          \
    (dev->tablesInfo.epclConfigTable.commonInfo.baseAddress != 0 ? SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev,epclConfigTable,entry_indx) : \
    (SKERNEL_IS_XCAT_REVISON_A1_DEV(dev)) ?   \
        SMEM_XCAT_EPCL_CONFIG_TBL_MEM(dev, entry_indx) : \
        0x07F00000 + ((entry_indx) * 0x04))

/* Tunnel Action Entry<n> (0<=n<5120)  */
#define SMEM_CHT3_TUNNEL_ACTION_TBL_MEM(dev,entry_indx) \
    SMEM_CHT3_TUNNEL_ACTION_WORD_TBL_MEM(dev,entry_indx,0)

#define SMEM_CHT3_TUNNEL_ACTION_WORD_TBL_MEM(dev,entry_indx,word) \
    SMEM_TABLE_ENTRY_2_PARAMS_INDEX_GET_MAC(dev,ttiAction,entry_indx,word)


/* Action Table and Policy TCAM Access Data Register */
#define SMEM_CHT3_PCL_ACTION_TCAM_DATA_TBL_MEM(dev,entry_indx) \
        SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev,pclActionTcamData,entry_indx)

/* Policy TCAM indirect access Data Parameters1 register */
#define SMEM_CHT3_PCL_ACTION_TCAM_DATA_PARAM1_REG(dev)  \
((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->TCCLowerIPCL.TCCIPCL.actionTableAndPolicyTCAMAccessCtrlParameters1   : \
        (SKERNEL_DEVICE_FAMILY_CHEETAH_3_ONLY(dev) ? 0x0B800130 : \
                                                     0x0d000130))


/* Policy TCAM indirect access Data Parameters2 register */
#define SMEM_CHT3_PCL_ACTION_TCAM_DATA_PARAM2_REG(dev)  \
((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->TCCLowerIPCL.TCCIPCL.actionTableAndPolicyTCAMAccessCtrlParameters2   : \
        (SKERNEL_DEVICE_FAMILY_CHEETAH_3_ONLY(dev) ? 0x0B800134 : \
                                                     0x0d000134))


/* router TCAM Entry<n> Data Word <0> (0<=n<5120, 0<=m<3) */
#define SMEM_CHT3_TCAM_X_DATA_TBL_MEM(dev,entry_indx) \
        (SSMEM_CHT3_TCAM_X_DATA_TBL_MEM(dev) + (0x10 * (entry_indx)))

/* router TCAM Entry<n> control Word <0> (0<=n<5120 0<=m<3) */
#define SMEM_CHT3_TCAM_ROUTER_TT_X_CONTROL_TBL_MEM(dev,entry_indx) \
        (SSMEM_CHT3_TCAM_ROUTER_TT_X_CONTROL_TBL_MEM(dev) + (0x10 * (entry_indx)))

/* router TCAM Entry<n> Data Word Mask <0> (0<=n<5120, 0<=m<3) */
#define SMEM_CHT3_TCAM_Y_DATA_TBL_MEM(dev,entry_indx) \
        (SSMEM_CHT3_TCAM_Y_DATA_TBL_MEM(dev) + (0x10 * (entry_indx)) )

/* router TCAM Entry<n> control Word <0> (0<=n<5120 0<=m<3) */
#define SMEM_CHT3_TCAM_ROUTER_TT_Y_CONTROL_TBL_MEM(dev,entry_indx) \
        (SSMEM_CHT3_TCAM_ROUTER_TT_Y_CONTROL_TBL_MEM(dev) + (0x10 * (entry_indx)))

/* ROUTER TCAM DATA */
#define SMEM_CHT3_ROUTER_TCAM_ACCESS_DATA_TBL_MEM(dev,entry_indx) (SKERNEL_DEVICE_FAMILY_CHEETAH_3_ONLY(dev)? \
        (0x02800400 + ( 0x04 * (entry_indx))) :                                   \
        (0x0D800400 + ( 0x04 * (entry_indx))))

/*  Port<n> Status Register0 (0<=n<24 , CPUPort = 63) */
#define SMEM_CHT3_PORT_STATUS0_REG(dev, port)\
    ((dev)->supportRegistersDb ? ((port) == SNET_CHT_CPU_PORT_CNS && ((dev)->cpuPortNoSpecialMac == 0)) ? \
       (SMEM_CHT_MAC_REG_DB_GET(dev))->GOP.ports.gigPort63CPU.portStatusReg0 : \
            IS_CHT_HYPER_GIGA_PORT(dev,port) ? \
                (SMEM_CHT_MAC_REG_DB_GET(dev))->GOP.ports.XLGIP[port].portStatus  : \
                (SMEM_CHT_MAC_REG_DB_GET(dev))->GOP.ports.gigPort[port].portStatusReg0  : \
            IS_CHT_HYPER_GIGA_PORT(dev,port) ? (CHT_MAC_PORTS_BASE_ADDRESS_CNS(dev,port) + 0xC  + (0x400 * (port))) : \
                                               (CHT_MAC_PORTS_BASE_ADDRESS_CNS(dev,port) + 0x10 + (0x400 * (port))))

#define SNET_CHT3_TCP_SANITY_CHECK_CMD(confVal, command) \
        *command = (confVal) ? \
            SKERNEL_EXT_PKT_CMD_HARD_DROP_E : \
            SKERNEL_EXT_PKT_CMD_FORWARD_E

/* Header Alteration VLAN Translation Enable Register Address: 0x07800130 */
#define SMEM_CHT3_HA_VLAN_TRANS_EN_REG(dev)   \
    SKERNEL_IS_XCAT_REVISON_A1_DEV(dev) ? \
        SMEM_XCAT_HA_VLAN_TRANS_EN_REG(dev) : (0x07800130)


#define SMEM_CHT3_INGR_VLAN_TRANS_TBL_MEM(dev, entry_index) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev,ingressVlanTranslation,entry_index)


#define SMEM_CHT3_EGR_VLAN_TRANS_TBL_MEM(dev, entry_index) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev,egressVlanTranslation,entry_index)


#define SMEM_CHT3_MAC_TO_ME_TBL_MEM(dev, entry_index) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev,macToMe,entry_index)

/* Fast Stack Register */
#define SMEM_CHT3_FAST_STACK_REG(dev) \
    (SKERNEL_IS_XCAT_REVISON_A1_DEV(dev) ? \
        SMEM_XCAT_FAST_STACK_REG(dev) : (0x0b800818))

/* Fast loop port Register */
#define SMEM_CHT3_LOOP_PORT_REG(dev)\
    (SKERNEL_IS_XCAT_REVISON_A1_DEV(dev) ? \
        SMEM_XCAT_LOOP_PORT_REG(dev) : (0x0b80081C))

/* Fast CFM Ethertype Register */
#define SMEM_CHT3_CFM_ETHERTYPE_REG(dev) \
    (SKERNEL_IS_LION_REVISON_B0_DEV(dev) ? \
        SMEM_LION_CFM_ETHERTYPE_REG(dev) : \
        (SKERNEL_IS_XCAT_REVISON_A1_DEV(dev) ? \
            SMEM_XCAT_SPECIAL_ETHERTYPES_REG(dev) : (0x0b800810)))


/* 802.3ah OAM Loopback Enable Per Port  */
#define SMEM_CHT_TXQ_OAM_FIRST_REG_LOOPBACK_FILTER_EN_REG(dev)\
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EGF_eft.egrFilterConfigs.OAMLoopbackFilterEn[0] : \
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer0.oam8023ahLoopbackEnablePerPortReg.oam8023ahLoopbackEnablePerPortReg : \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer1.TXQ.TXQ_EGR[0].OAMLoopbackFilterEn[0] : \
            SKERNEL_IS_LION_REVISON_B0_DEV(dev) ? \
                (0x01810070) : (0x01800094)))

/* Fast DP2CFI Register */
#define SMEM_CHT3_DP2CFI_REG(dev) \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer0.DPToCFIMap.DP2CFIReg : \
        SKERNEL_IS_LION_REVISON_B0_DEV(dev) ? \
                    (0x0A091000) : (0x018000A8))

/* Fast DP2CFI Enable Register\DP To CFI Remap En <%n> */
#define SMEM_CHT3_DP2CFI_ENABLE_REG(dev)\
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EGF_qag.distributor.DPRemap[0] : \
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer0.DPToCFIMap.DP2CFIEnablePerPortReg : \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer1.TXQ.TXQ_Queue.CongestionNotification.DPToCFIRemapEn[0] : \
            SKERNEL_IS_LION_REVISON_B0_DEV(dev) ? \
                    (0x0A091080) : (0x018000A4)))

/* Statistical & CPU-Triggered Egress Mirroring to Analyzer port */
#define SMEM_CHT3_TRUNK_DESIGNATED_TBL_MEM(entry)           \
            (0x03240008 + ((entry) * 0x10))


/* Device<n> Map Table Entry */
#define SMEM_CHT3_DEVICE_MAP_TBL_MEM(dev) \
            (0x03240004 + ((dev) * 0x10))

/* Non-Trunk Members Table */
#define SMEM_CHT3_NON_TRUNK_MEMBERS_TBL_MEM(trunkID) \
            (0x01A40000 + ((trunkID) * 0x10))

/* Ingress Policer Port Metering Enable */
#define SMEM_CHT3_POLICER_PORT_METER_REG(dev, cycle, port) \
        SKERNEL_IS_XCAT_REVISON_A1_DEV(dev) ? \
            SMEM_XCAT_POLICER_PORT_METER_REG(dev, cycle, port) : (0x0C000004)

/* Ingress Policer Management Counters Memory */
#define SMEM_CHT3_POLICER_MNG_CNT_TBL_MEM(dev, cycle, counter) \
        SKERNEL_IS_XCAT_REVISON_A1_DEV(dev) ? \
            SMEM_XCAT_POLICER_MNG_CNT_TBL_MEM(dev, cycle, counter) : (0x0C0C0000)

#define SMEM_CHT3_IPLR_MNG_CNT_SET_GREEN_TBL_MEM(dev,counter) \
        (SMEM_CHT3_POLICER_MNG_CNT_TBL_MEM(dev, 0, counter) + ((counter) * 0x20))

#define SMEM_CHT3_IPLR_MNG_CNT_SET_YELLOW_TBL_MEM(dev,counter) \
        (SMEM_CHT3_POLICER_MNG_CNT_TBL_MEM(dev,  0, counter) + 0x8 + ((counter) * 0x20))

#define SMEM_CHT3_IPLR_MNG_CNT_SET_RED_TBL_MEM(dev,counter) \
        (SMEM_CHT3_POLICER_MNG_CNT_TBL_MEM(dev,  0, counter) + 0x10 + ((counter) * 0x20))

#define SMEM_CHT3_IPLR_MNG_CNT_SET_DROP_TBL_MEM(dev,counter) \
        (SMEM_CHT3_POLICER_MNG_CNT_TBL_MEM(dev, 0, counter) + 0x18 + ((counter) * 0x20))

/* Ingress Policer Initial DP<n> */
#define SMEM_CHT3_POLICER_INITIAL_DP_REG(dev, cycle,qosProfile) \
    SKERNEL_IS_XCAT_REVISON_A1_DEV(dev) ? \
        SMEM_XCAT_POLICER_INITIAL_DP_REG(dev, cycle,qosProfile) : (0x0C000060 + (((qosProfile)/16)*4))

/* CNC Fast Dump Trigger Register */
#define SMEM_CHT3_CNC_FAST_DUMP_TRIG_REG(dev,_cncUnitIndex)                            \
SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->CNC[_cncUnitIndex].globalRegs.CNCFastDumpTriggerReg : \
((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->centralizedCntrs.globalRegs.CNCFastDumpTriggerReg   : \
0x08000030)

/* CNC Interrupt Cause Register */
#define SMEM_CHT3_CNC_INTR_CAUSE_REG(dev,_cncUnitIndex)                                \
SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->CNC[_cncUnitIndex].globalRegs.CNCInterruptSummaryCauseReg : \
((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->centralizedCntrs.globalRegs.CNCInterruptSummaryCauseReg   : \
0x08000100)

/* CNC Interrupt Mask Register */
#define SMEM_CHT3_CNC_INTR_MASK_REG(dev,_cncUnitIndex)                                 \
SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->CNC[_cncUnitIndex].globalRegs.CNCInterruptSummaryMaskReg : \
((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->centralizedCntrs.globalRegs.CNCInterruptSummaryMaskReg   : \
0x08000104)

/* CNC Block Configuration Register */
#define SMEM_CHT3_CNC_BLOCK_CNF_REG(dev, block, client,_cncUnitIndex) \
SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->CNC[_cncUnitIndex].perBlockRegs.clientEnable.CNCBlockConfigReg0[block] : \
((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->centralizedCntrs.perBlockRegs. \
                                    CNCBlockConfigReg[block][ SKERNEL_DEVICE_FAMILY_XCAT3_ONLY(dev) ? (client/2) : (client) ]   : \
        (0x08001080 + ((block) * 0x100) + ((client) / 2 * 0x4)))

#define SMEM_CHT_ANY_CNC_BLOCK_CNF_REG(dev, block, client,_cncUnitIndex) \
        SKERNEL_IS_LION_REVISON_B0_DEV(dev)                    \
        ? SMEM_LION_CNC_BLOCK_CNF0_REG(dev, block,_cncUnitIndex)        \
        : SMEM_CHT3_CNC_BLOCK_CNF_REG(dev, block, client,_cncUnitIndex)

/* CNC Block<n> Wraparound Status Register<m> */
#define SMEM_CHT3_CNC_BLOCK_WRAP_AROUND_STATUS_REG(dev, block, entry,_cncUnitIndex) \
SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->CNC[_cncUnitIndex].perBlockRegs.wraparound.CNCBlockWraparoundStatusReg[block][entry] : \
((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->centralizedCntrs.perBlockRegs.CNCBlockWraparoundStatusReg[block][entry]  : \
        (0x080010A0 + ((block) * 0x100) + (entry) * 0x4))

/* Counters blocks memory */
#define SMEM_CHT3_CNC_BLOCK_COUNTER_TBL_MEM(dev,block,index , cncUnit) \
        SMEM_TABLE_ENTRY_2_PARAMS_INDEX_DUP_TBL_GET_MAC(dev, cncMemory, block , index , cncUnit)


/* L2/L3 Ingress VLAN Counting Enable Register */
#define SMEM_CHT3_CNC_VLAN_EN_REG(dev, port) \
        SKERNEL_IS_XCAT_REVISON_A1_DEV(dev) ? \
            SMEM_XCAT_CNC_VLAN_EN_REG(dev, port) : (0x0b800824)

/* CNC Counting Mode Register */
#define SMEM_CHT3_CNC_COUNT_MODE_REG(dev) \
        SKERNEL_IS_XCAT_REVISON_A1_DEV(dev) ? \
            SMEM_XCAT_CNC_COUNT_MODE_REG(dev) : (0x0B800820)

/* CNC modes register  */
#define SMEM_CHT3_CNC_MODES_REG(dev)\
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->SIP5_TXQ_QUEUE.peripheralAccess.CNCModes.CNCModes : \
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer0.CNCModes.CNCModesReg : \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer1.TXQ.TXQ_Queue.PeripheralAccess.CNCModesReg : \
            SKERNEL_IS_LION_REVISON_B0_DEV(dev) ? \
                (0x0A0936A0): (0x018000A0)))

/* Router Header Alteration Global Configuration */
#define SMEM_CHT3_ROUTE_HA_GLB_CNF_REG(dev)\
    SKERNEL_IS_XCAT_REVISON_A1_DEV(dev) ? \
        SMEM_XCAT_ROUTE_HA_GLB_CNF_REG(dev) : 0x07800100

/* CNC Global Configuration Register */
#define SMEM_CHT3_CNC_GLB_CONF_REG(dev,_cncUnitIndex)                                  \
SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->CNC[_cncUnitIndex].globalRegs.CNCGlobalConfigReg : \
((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->centralizedCntrs.globalRegs.CNCGlobalConfigReg   : \
0x08000000)
/* CNC Clear By Read Value Register Word 0 */
#define SMEM_CHT3_CNC_ROC_WORD0_REG(dev,_cncUnitIndex)                                 \
SMEM_CHT_IS_SIP5_GET(dev) ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->CNC[_cncUnitIndex].globalRegs.CNCClearByReadValueRegWord0 : \
((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->centralizedCntrs.globalRegs.CNCClearByReadValueRegWord0   : \
0x08000040)

/* L2i - ingress log flow registers */
#define SMEM_CHT3_INGRESS_LOG_BASE_REG(dev)                    \
    (SKERNEL_DEVICE_FAMILY_LION_PORT_GROUP_DEV(dev) ?                    \
        0x02000700:                                                 \
        0x02040700)
                                                                                                                     /* other    , lion     */
#define SMEM_CHT3_INGRESS_LOG_ETHER_TYPE_REG(dev)             (SMEM_CHT3_INGRESS_LOG_BASE_REG(dev) + 0x00) /*0x02040700 0x02000700*/
#define SMEM_CHT3_INGRESS_LOG_DA_LOW_REG(dev)                 (SMEM_CHT3_INGRESS_LOG_BASE_REG(dev) + 0x04) /*0x02040704 0x02000704*/
#define SMEM_CHT3_INGRESS_LOG_DA_HIGH_REG(dev)                (SMEM_CHT3_INGRESS_LOG_BASE_REG(dev) + 0x08) /*0x02040708 0x02000708*/
#define SMEM_CHT3_INGRESS_LOG_DA_LOW_MASK_REG(dev)            (SMEM_CHT3_INGRESS_LOG_BASE_REG(dev) + 0x0c) /*0x0204070c 0x0200070c*/
#define SMEM_CHT3_INGRESS_LOG_DA_HIGH_MASK_REG(dev)           (SMEM_CHT3_INGRESS_LOG_BASE_REG(dev) + 0x10) /*0x02040710 0x02000710*/
#define SMEM_CHT3_INGRESS_LOG_SA_LOW_REG(dev)                 (SMEM_CHT3_INGRESS_LOG_BASE_REG(dev) + 0x14) /*0x02040714 0x02000714*/
#define SMEM_CHT3_INGRESS_LOG_SA_HIGH_REG(dev)                (SMEM_CHT3_INGRESS_LOG_BASE_REG(dev) + 0x18) /*0x02040718 0x02000718*/
#define SMEM_CHT3_INGRESS_LOG_SA_LOW_MASK_REG(dev)            (SMEM_CHT3_INGRESS_LOG_BASE_REG(dev) + 0x1c) /*0x0204071c 0x0200071c*/
#define SMEM_CHT3_INGRESS_LOG_SA_HIGH_MASK_REG(dev)           (SMEM_CHT3_INGRESS_LOG_BASE_REG(dev) + 0x20) /*0x02040720 0x02000720*/
#define SMEM_CHT3_INGRESS_LOG_MAC_DA_RESULT_REG(dev)          (SMEM_CHT3_INGRESS_LOG_BASE_REG(dev) + 0x24) /*0x02040724 0x02000724*/
#define SMEM_CHT3_INGRESS_LOG_MAC_SA_RESULT_REG(dev)          (SMEM_CHT3_INGRESS_LOG_BASE_REG(dev) + 0x28) /*0x02040728 0x02000728*/

/* IPvX - log flow registers */
#define SMEM_CHT3_IP_HIT_LOG_DIP_REG(dev, n)                        \
((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->IPvX.IPLog.IPHitLogDIPAddrReg[n]   : \
(0x02800d00 + (n)*4)) /* + n*4: where n (0-3) represents Word */
#define SMEM_CHT3_IP_HIT_LOG_DIP_MASK_REG(dev, n)                   \
((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->IPvX.IPLog.IPHitLogDIPMaskAddrReg[n]   : \
(0x02800d10 + (n)*4)) /* + n*4: where n (0-3) represents Word */
#define SMEM_CHT3_IP_HIT_LOG_SIP_REG(dev, n)                        \
((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->IPvX.IPLog.IPHitLogSIPAddrReg[n]   : \
(0x02800d20 + (n)*4)) /* + n*4: where n (0-3) represents Word */
#define SMEM_CHT3_IP_HIT_LOG_SIP_MASK_REG(dev, n)                   \
((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->IPvX.IPLog.IPHitLogSIPMaskAddrReg[n]   : \
(0x02800d30 + (n)*4)) /* + n*4: where n (0-3) represents Word */
#define SMEM_CHT3_IP_HIT_LOG_LAYER4_DESTINATION_PORT_REG(dev)       \
((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->IPvX.IPLog.IPHitLogLayer4DestinationPortReg   : \
0x02800d40)
#define SMEM_CHT3_IP_HIT_LOG_LAYER4_SOURCE_PORT_PORT_REG(dev)       \
((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->IPvX.IPLog.IPHitLogLayer4SourcePortReg   : \
0x02800d44)
#define SMEM_CHT3_IP_HIT_LOG_PROTOCOL_REG(dev)                      \
((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->IPvX.IPLog.IPHitLogProtocolReg   : \
0x02800d48)
#define SMEM_CHT3_IP_HIT_LOG_DESTINATION_IP_NHE_ENTRY_REG(dev, n)   \
((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->IPvX.IPLog.IPHitLogDestinationIPNHEEntryRegWord[n]   : \
0x02800d50 + (n)*4)) /* + n*4: where n (0-2) represents Word */
#define SMEM_CHT3_IP_HIT_LOG_SOURCE_IP_NHE_ENTRY_REG(dev, n)   \
((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->IPvX.IPLog.IPHitLogSourceIPNHEEntryRegWord[n]   : \
0x02800d5c + (n)*4)) /* + n*4: where n (0-2) represents Word */

/* base address for all the memories needed extra in CH3+ */
#define SMEM_CHT3_PLUS_EXTRA_MEM_INDEX                              60 /*0x3c*/ /*60 to 64*/
#define SMEM_CHT3_PLUS_EXTRA_MEM_NUM_INDEX                          4  /*4 indexes to occupy*/
#define SMEM_CHT3_PLUS_EXTRA_MEM_SIZE       /* size */              (SMEM_CHT3_PLUS_EXTRA_MEM_NUM_INDEX << 23)
#define SMEM_CHT3_PLUS_EXTRA_MEM_BASE       /* base Addr */         (SMEM_CHT3_PLUS_EXTRA_MEM_INDEX << 23)
/* Secondary Target Port Map  Table */
#define SMEM_CHT3_SECONDARY_TARGET_PORT_MAP_TBL_MEM(dev, port) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, secondTargetPort, port)

/* Port<n> Serial Parameters 1 Configuration Register (xCat2)
   n = 0..27 */
#define SMEM_XCAT2_PORT_SERIAL_PARAMS1_REG(dev, port)    \
     ((dev)->supportRegistersDb ? ((port) == SNET_CHT_CPU_PORT_CNS && ((dev)->cpuPortNoSpecialMac == 0) ? \
          SMAIN_NOT_VALID_CNS : \
          (SMEM_CHT_MAC_REG_DB_GET(dev))->GOP.ports.gigPort[port].portSerialParameters1Config) : \
            (0x0a800094 + ((port) * 0x400)))

/* Port<n> MAC Control Register0 */
#define SMEM_CHT3_MAC_CONTROL0_REG(dev,port) \
     ((dev)->supportRegistersDb ? ((port) == SNET_CHT_CPU_PORT_CNS && ((dev)->cpuPortNoSpecialMac == 0) ? \
             (SMEM_CHT_MAC_REG_DB_GET(dev))->GOP.ports.gigPort63CPU.portMACCtrlReg[0] : \
         IS_CHT_HYPER_GIGA_PORT(dev,port) ? \
             (SMEM_CHT_MAC_REG_DB_GET(dev))->GOP.ports.XLGIP[port].portMACCtrlReg[0]   : \
             (SMEM_CHT_MAC_REG_DB_GET(dev))->GOP.ports.gigPort[port].portMACCtrlReg[0])   : \
         (CHT_MAC_PORTS_BASE_ADDRESS_CNS(dev,port) + (0x400 * (port))))

/* Port<n> MAC Control Register1, Cht3 */
#define SMEM_CHT3_MAC_CONTROL1_REG(dev,port)\
            (SMEM_CHT3_MAC_CONTROL0_REG(dev,port) + 0x4)

/* Port<n> MAC Control Register2 */
#define SMEM_CHT3_MAC_CONTROL2_REG(dev,port) \
            (SMEM_CHT3_MAC_CONTROL0_REG(dev,port) + 0x8)

/* Port<n> Interrupt Cause Register , Cht3 */
#define SMEM_CHT3_PORT_INT_CAUSE_REG(dev,port) \
    ((dev)->supportRegistersDb ? ((port) == SNET_CHT_CPU_PORT_CNS && ((dev)->cpuPortNoSpecialMac == 0)) ? \
       (SMEM_CHT_MAC_REG_DB_GET(dev))->GOP.ports.gigPort63CPU.portInterruptCause : \
            (IS_CHT_HYPER_GIGA_PORT(dev,port)) ? \
                (SMEM_CHT_MAC_REG_DB_GET(dev))->GOP.ports.XLGIP[port].portInterruptCause  : \
                (SMEM_CHT_MAC_REG_DB_GET(dev))->GOP.ports.gigPort[port].portInterruptCause  : \
            (IS_CHT_HYPER_GIGA_PORT(dev,port)) ? \
                (CHT_MAC_PORTS_BASE_ADDRESS_CNS(dev,port) + 0x14 + (0x400 * (port))) : \
                (CHT_MAC_PORTS_BASE_ADDRESS_CNS(dev,port) + 0x20 + (0x400 * (port))))


/* Port<n> Serial Parameters Configuration , Cht3 */
/* not for XG ports */
#define SMEM_CHT3_PORT_SERIAL_PARAMETERS_REG(dev,port) \
        ((IS_CHT_HYPER_GIGA_PORT(dev,port)) ?  0 :  \
            ((dev)->supportRegistersDb ?             \
                (((port) == SNET_CHT_CPU_PORT_CNS && ((dev)->cpuPortNoSpecialMac == 0)) ? \
                   (SMEM_CHT_MAC_REG_DB_GET(dev))->GOP.ports.gigPort63CPU.portSerialParametersConfig :  \
                   (SMEM_CHT_MAC_REG_DB_GET(dev))->GOP.ports.gigPort[port].portSerialParametersConfig) : \
            (CHT_MAC_PORTS_BASE_ADDRESS_CNS(dev,port) + 0x14 + (0x400 * (port)))))


/* CAPWAP registers */
#define SMEM_CHT3_CAPWAP_BASE                                       (SMEM_CHT3_PLUS_EXTRA_MEM_BASE) /*0x1E000000*/
/* 8 tables to hold the mapping between tid to vpt,qosIndex for 802.11 frames*/
/* 8 tables , each table 16 entries , each entry is single register */
/* profile is the table index 0..7 , tid is the entry index 0..15 */
#define SMEM_CHT3_802_11E_MAP_TID_TO_QOS_REG(profile,tid)           ((SMEM_CHT3_CAPWAP_BASE + 0x1000) + \
                                                                        (((profile) * 16) + (tid))*4)
/* table that convert virtual {dev,port} to tunnelStart pointer and
    physical {dev,port}/trunk :

    entry size is 2 words

    word 0 -

    bit 0 - useVidx
        when useVidx is 0
            bit 1 - TargetIsTrunk
            when TargetIsTrunk is 0
                bits 2..7  - TargetPort[5:0]
                bits 8..12 - TargetDevice[4:0]
            when TargetIsTrunk is 1
                bits 2..5  - Reserved
                bits 6..12 - TargetTrunkID[6:0]
        when useVidx is 1
            bits 1..12 - Vidx
        bit 13 - TunnelStart
        bit 14..26 - TunnelPtr[12:0]

        bits 27..31 - reserved (5 bits)

    word 1 -
        bits 0..31 - SrcID Filter Bitmap

*/
/* each entry is 2 registers */
/* every 64 entries the device is incremented */
#define SMEM_CHT3_VIRTUAL_DEV_PORT_MAP_TO_TS_AND_PHY_DEV_PORT_TRUNK_REG(virtDev,virtPort) \
            (SMEM_CHT3_802_11E_MAP_TID_TO_QOS_REG(8,0) + \
                (64 * (virtDev) + (virtPort))*8)

/* register that hold the action when CAPWAP/ipv4/ipv6 fragment :
    redirect to reassembly engine
    trap
    hard drop
*/
#define SMEM_CHT3_CAPWAP_FRAGMENT_EXCEPTION_REG \
    SMEM_CHT3_VIRTUAL_DEV_PORT_MAP_TO_TS_AND_PHY_DEV_PORT_TRUNK_REG(32,0)

/* there is a need of designated portNum added to the devNum when send to local
   CPU */
#define  SMEM_CHT3_DESIGNATED_PORT_REG     \
        (SMEM_CHT3_CAPWAP_FRAGMENT_EXCEPTION_REG + 4)

/* use bit 20 as "use designated port" other then 63 */
/* in table of SMEM_CHT_CPU_CODE_MEM entry */
#define  SMEM_CHT3_USE_DESIGNATED_PORT_BIT   20  /* bit 20 is first free */


/* 32K MAC table for MAC SA,DA lookups */
/* each entry hold next fields:
--- WORD 0,1
    1. macAddress - 48 bits
--- WORD 1
    2. valid - 1 bit (for SA and DA)
    3. SA packetCommand - 3 bits (only for SA)
        - values : forward , hard drop , soft drop , trap
    4. DA packetCommand - 3 bits  (only for DA)
        - values : forward , hard drop , soft drop , trap
    5. rssiWeightProfile - 2 bits (only for SA)
    6. useThisEntryVlanId - 1 bit (only for DA)
--- WORD 2
    7. saVlanId - 12 bits (only for SA)
    8. daVlanId - 12 bits (only for DA)
    9. rssiAverage - 8 bits (only for SA)
--- WORD 3
    10. redirectEnable - 1 bit (only for SA)
    11. redirectEgressInterface - virtual {dev,port} - 5+6 = 11 bits (only for SA)
    12. da802dot11eEgressEnable - 1 bit (only for DA)
    13. da802dot11eEgressMappingTableProfile - 2 bit (only for DA)
    =================
    total of:
    4 words entry

    --> keep 4 words entry anyway (for alignment)
*/
#define SMEM_CHT3_CAPWAP_MAC_TABLE_ENTRY_TBL_MEM(index)   \
        ((SMEM_CHT3_DESIGNATED_PORT_REG + 4) + \
            (index) * 0x10)


/* 4 registers that hold weight profiles :
   each register format is:
   weight -- 7 bits for each value in range of : 0..100
   rssiMin -- 8 bits of minimum RSSI packet threshold.
   rssiMinAverage -- 8 bits of minimum RSS average threshold.
*/
#define SMEM_CHT3_CAPWAP_RSSI_WEIGHT_PROFILE_REG(profile)   \
    (SMEM_CHT3_CAPWAP_MAC_TABLE_ENTRY_TBL_MEM(32*1024) + \
        ((profile)*4))

/* 2 registers that hold:
    the MAC SA and its associated RSSI value , that cause on of the 2 interrupts:
    - incoming packet RSSI is less than the configured RSSI packet threshold
    - the updated average RSSI is less than the configured RSSI average threshold

   the format is:
   word 0 - 32 bits - bytes 0..3 of MAC SA
   word 1 - 16 bits - bytes 4,5 of MAC SA
            8 bits  - the associated RSSI value
*/
#define SMEM_CHT3_CAPWAP_RSSI_EXCEPTION_REG \
    SMEM_CHT3_CAPWAP_RSSI_WEIGHT_PROFILE_REG(4)

/* a single register that hold 32 bits representing the devices that are
    "virtual devices"
*/
#define SMEM_CHT3_VIRTUAL_DEVICES_BMP_REG(dev)   \
    (SMEM_CHT3_CAPWAP_RSSI_EXCEPTION_REG + 4)


/* 16 registers of :
   secondary set of registers for -->
   IEEE Reserved Multicast Configuration Register<n> (0<=n<16)

   every protocol (0..255) has 2 bits in the register. (each register hold
        commands for 16 protocols)
   --> total of 16 registers hold 256 protocols.

   see also use of SMEM_CHT_IEEE_RSRV_MCST_CONF_TBL_MEM , which is the primary set
   of registers
*/
#define SMEM_CHT3_IEEE_RSRV_MCST_CONF_SECONDARY_TBL_MEM(protocol) \
        ((SMEM_CHT3_VIRTUAL_DEVICES_BMP_REG(dev) + 4) + \
         ((protocol) / 16) * 4 )

/* 4K entries that indexed by the VID
   each entry hold 12 bits of the mllPointer for linked list of : virtual ports

   each entry:
   bits 0..11 - mllPointer


   NOTE :
   1.   if pointer is NULL (0)  -- it is ignored.
   2. this list is in addition to the physical ports of vid

*/
#define SMEM_CHT3_VID_TO_MLL_MAP_TBL_MEM(vidIndex)\
    (SMEM_CHT3_IEEE_RSRV_MCST_CONF_SECONDARY_TBL_MEM(256) + \
        ((vidIndex)*4))


/* 4K entries that indexed by the VIDX
   each entry hold 12 bits of the mllPointer for linked list of : virtual ports

   each entry:
   bits 0..11 - mllPointer


   NOTE :
   1. if pointer is NULL (0)  -- it is ignored.
   2. this list is in addition to the physical ports of vidx

*/
#define SMEM_CHT3_VIDX_TO_MLL_MAP_ENTRY_TBL_MEM(vidxIndex)\
    (SMEM_CHT3_VID_TO_MLL_MAP_TBL_MEM(4096) + \
        ((vidxIndex)*4))


/* 4 tables to hold the mapping between qosIndex to tid , AckPolicy for 802.11e frames*/
/* 4 tables , each table 128 entries , each entry is single register */
/* profile is the table index 0..3 , qosProfile is the entry index 0..127 */
/* TID - bits 0..3 */
/* AckPolicy - bits 4..5 */
#define SMEM_CHT3_QOS_MAP_TO_802_11E_TID_TBL_MEM(profile, qosProfile)   ((SMEM_CHT3_VIDX_TO_MLL_MAP_ENTRY_TBL_MEM(4096)) + \
                                                                        (((profile) * 128) + (qosProfile))*4)

/* register to hold the <OSM CPU code> used by the TTI action and the TS */
/* single register :
    bits 0..7 - OSM CPU code (8 bits)
*/
#define SMEM_CHT3_OSM_CPU_CODE_TBL_MEM   \
    SMEM_CHT3_QOS_MAP_TO_802_11E_TID_TBL_MEM(4, 0)

#define SMEM_CHT3_HGS_PORT_MIB_COUNT_CONTROL_REG(dev,port)  \
((dev)->supportRegistersDb ? \
    (((SMEM_CHT_MAC_REG_DB_GET(dev))->GOP.ports.XLGIP[(port)].xgMIBCountersControl == SMAIN_NOT_VALID_CNS) ? \
      (SMEM_CHT_MAC_REG_DB_GET(dev))->GOP.ports.gigPort[(port)].stackPortMIBCntrsCtrl : \
      (SMEM_CHT_MAC_REG_DB_GET(dev))->GOP.ports.XLGIP[(port)].xgMIBCountersControl) : \
    (CHT_MAC_PORTS_BASE_ADDRESS_CNS(dev, port) + 0x30 + (port * 0x0400)))

#define SMEM_CHT3_HGS_PORT_MAC_MIB_COUNT_REG(dev, port) \
    smemMibCounterAddrByPortGet(dev, port)

#define SMEM_CHT3_HGS_PORT_CAPTURE_MAC_MIB_COUNT_REG(dev, port) \
    (SMEM_CHT3_HGS_PORT_MAC_MIB_COUNT_REG(dev, port) + 0x80)

#define SMEM_CHT3_HGS_PORT_MAC_MIB_COUNT_TABLE0_REG(dev, port) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, xgPortMibCounters, port)


#define SMEM_CHT3_HGS_PORT_MAC_MIB_COUNT_TABLE1_REG(dev, index) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, xgPortMibCounters_1, index)


/* MLL Out Interface counter Entry */
#define SMEM_CHT3_MLL_OUT_INTERFACE_COUNTER_TBL_MEM(dev,entry_indx)         \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev,mllOutInterfaceCounter,entry_indx)

/* MLL Out Interface counter Entry */
#define SMEM_CHT3_MLL_OUT_INTERFACE_CNFG_TBL_MEM(dev,entry_indx)         \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev,mllOutInterfaceConfig,entry_indx)

/* MLL fifo drop counter Entry  */
#define SMEM_CHT3_MLL_DROP_CNTR_TBL_MEM(dev)                             \
    SMEM_CHT3_MLL_REGISTERS_BASE_TBL_MEM(dev,0x984)

/* disable lookup1 for not routed packets */
#define SMEM_CHT3_PCL_LOOKUP1_NOT_ROUTED_PKTS_REG(dev)              \
    (0x0B800808)

/*XG Ports Interrupt Cause register*/
#define SMEM_CHT3_XG_PORTS_INTERRUPT_CAUSE_REG(dev)                  \
((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->MG.globalInterrupt.stackPortsInterruptCause   : \
0x00000080)

/*XG Ports Interrupt Cause mask*/
#define SMEM_CHT3_XG_PORTS_INTERRUPT_MASK_REG(dev)                  \
((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->MG.globalInterrupt.stackPortsInterruptMask   : \
0x00000084)

#define SMEM_CHT3_SERDES_SPEED_1_REG(dev)                           \
((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->GOP.ports.gigPort[0].portMACCtrlReg[0]   : \
0x09800014)

/* Ingress Policer Table Access Data */
#define SMEM_CHT3_INGRESS_POLICER_TBL_DATA_TBL_MEM(dev, cycle, entry_index)         \
    SMEM_TABLE_ENTRY_INDEX_DUP_TBL_GET_MAC(dev, policerTblAccessData, entry_index, cycle)


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __sregCheetah3h */

