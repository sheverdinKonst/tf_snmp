/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* sregCheetah.h
*
* DESCRIPTION:
*       Defines API for Cheetah memory registers access.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 117 $
*
*******************************************************************************/
#ifndef __sregcheetahh
#define __sregcheetahh

#include <asicSimulation/SKernel/cheetahCommon/sregCheetah2.h>
#include <asicSimulation/SKernel/cheetahCommon/sregCheetah3.h>
#include <asicSimulation/SKernel/cheetahCommon/sregXCat.h>
#include <asicSimulation/SKernel/cheetahCommon/sregXCat3.h>
#include <asicSimulation/SKernel/cheetahCommon/sregLion.h>
#include <asicSimulation/SKernel/cheetahCommon/sregLion2.h>
#include <asicSimulation/SKernel/cheetahCommon/sregBobcat2.h>
#include <asicSimulation/SKernel/cheetahCommon/sregBobcat3.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Number of stacking ports in ch2 devices */
#define SMEM_CH2_STACKING_PORTS_NUM     4

/* CPU port */
#define SNET_CHT_CPU_PORT_CNS                          63

/*
base address for the 'MAC ports registers'

for GIG ports
    Port Status
    Tri-Speed Ports MAC Configuration
    PRBS Check Status
    SGMII Configuration
    Port Serial Parameters Configuration
    Port Interrupt
    Port Internal
    Port Auto-Negotiation Configuration

for XG ports
    Port MAC Control Register0
    Port MAC Control Register1
    Port MAC Control Register2
    Port Status
    Port FIFOs Thresholds Configuration
    Port Interrupt Cause
    Port Interrupt Mask
    Port MAC Control Register3
    Port Per Prio Flow Control Status
    Port DFX Register
    Port Metal Fix
    XG MIB Counters Control Register
*/
#define CHT_MAC_PORTS_BASE_ADDRESS_CNS(dev,port)    \
   (IS_CHT_HYPER_GIGA_PORT(dev,port) ? 0x08800000 : \
    IS_CHT_GIG_STACK_A0_PORT(dev,port)  ? 0x08800000 : \
    0x0a800000)

/* check if the port need to use XG port logic for mac counters
   (MIB control registers and MIB counters)  */
#define CHT_MAC_PORT_MIB_COUNTERS_IS_XG_LOGIC_MAC(dev,port)     \
   ((IS_CHT_HYPER_GIGA_PORT(dev,port)    ||                     \
     IS_CHT_GIG_STACK_A0_PORT(dev,port)  ||                     \
     IS_CHT_GIG_STACK_A1_PORT(dev,port)) ?                      \
     1 : 0)

/* base address for the ports mac mib of a port */
#define CHT_MAC_PORTS_MIB_COUNTERS_BASE_ADDRESS_CNS(dev,port)    \
   (((dev)->isMsmMibOnAllPorts || CHT_MAC_PORT_MIB_COUNTERS_IS_XG_LOGIC_MAC(dev,port))    ?     \
    SMEM_CHT_HGS_PORT_MAC_COUNT_REG(dev, port) :                 \
    SMEM_CHT_PORT_MAC_COUNT_REG(dev,port))


/* get the port number from the accessed address of the mac registers */
/* address in range of 0x08800000 (ch3 and above) or 0x0a800000 */
#define CHT_FROM_ADDRESS_GET_PORT_CNS(dev,address)  \
    smemGopPortByAddrGet(dev,address)

/* get the port number from the accessed address of the mac registers */
/* address in range of 0x09000000 (ch3 and above) need to be mapped to port#
    in ch3,xcat,lion2,lion3 - 0x20000 is the step of port
 */
#define CHT3_FROM_XG_PORT_COUNTER_ADDRESS_GET_PORT_CNS(dev,address)  \
    ((dev)->xgCountersStepPerPort ?                    \
        (((address) / (dev)->xgCountersStepPerPort) & 0x3f ) :             \
 /* don't use SMEM_BIT_MASK((dev)->flexFieldNumBitsSupport.phyPort) because port is local to the core*/ \
        0)



/* port 24..27 are hyper giga ports in cheetah */
/* in ch3 ports 0,4,10,12,16,22 may be XG
   in xCat ports 24..27 may be GE ports
   so we need to use the DB and not specific port number
   in lion port group all ports (0..11) can be XG/GE , port 63 (cpu port) --> GE
*/
#define IS_CHT_HYPER_GIGA_PORT(dev,port) \
    (((dev)->portsArr[port].state >= SKERNEL_PORT_STATE_XG_E) ? GT_TRUE : GT_FALSE)

#define IS_CHT_CPU_PORT(port) \
    ((port == SNET_CHT_CPU_PORT_CNS) ? GT_TRUE : GT_FALSE)

#define IS_CHT_REGULAR_PORT(dev,port) \
    ((((dev)->portsArr[port].state == SKERNEL_PORT_STATE_GE_E) || \
      ((dev)->portsArr[port].state == SKERNEL_PORT_STATE_FE_E)) ? GT_TRUE : GT_FALSE)

/* port is in giga mode (G/2.5G) using GIG registers of the stack for XCat A0 */
#define IS_CHT_GIG_STACK_A0_PORT(dev,port) \
    (((dev)->portsArr[port].state == SKERNEL_PORT_STATE_GE_STACK_A0_E) ? GT_TRUE : GT_FALSE)

/* port is in giga mode (G/2.5G) using GIG registers of the stack for XCat A1*/
#define IS_CHT_GIG_STACK_A1_PORT(dev,port) \
    (((dev)->portsArr[port].state == SKERNEL_PORT_STATE_GE_STACK_A1_E) ? GT_TRUE : GT_FALSE)

#define IS_CHT_VALID_PORT(dev,port) \
    (((dev)->portsArr[port].state == SKERNEL_PORT_STATE_NOT_EXISTS_E) \
    ? GT_FALSE : GT_TRUE)

/* Convert CPU port to DB index for xCat2 and above devices */
#define CPU_PORT_INDEX(dev, port) \
    ((dev)->supportRegistersDb ? \
        ((IS_CHT_CPU_PORT(port)) ? 28 : (port)) : (port))

/* Miscellaneous Interrupt Mask Register - Cht/Ch2/Ch3 */
#define SMEM_CHT_MISC_INTR_MASK_REG(dev)                                 \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->MG.globalInterrupt.miscellaneousInterruptMask : \
        (0x0000003C))

/* Miscellaneous Interrupt Cause Register - Cht/Ch2/Ch3 */
#define SMEM_CHT_MISC_INTR_CAUSE_REG(dev)                                \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->MG.globalInterrupt.miscellaneousInterruptCause : \
        (0x00000038))

/* Global Control Register - Cht/Cht2 */
#define SMEM_CHT_GLB_CTRL_REG(dev)                                       \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->MG.globalConfig.globalCtrl : \
        (0x00000058))

/* Global Interrupt Cause Register - Cht/Cht2 */
#define SMEM_CHT_GLB_INT_CAUSE_REG(dev)                                  \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->MG.globalInterrupt.globalInterruptCause : \
        (0x00000030))

/* Global Interrupt Mask Register - Cht/Cht2 */
#define SMEM_CHT_GLB_INT_MASK_REG(dev)                                   \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->MG.globalInterrupt.globalInterruptMask : \
        (0x00000034))

/* Egress STC Interrupt Cause Register - Cht/Cht2/Lion B0/xCat */
#define SMEM_CHT_STC_INT_CAUSE_REG(dev,_port,dqUnitInPipe)\
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer0.transmitQueueInterrupt.egrSTCInterruptCause : \
        SMEM_LION_TXQ_EGR_STC_INTR_CAUSE_REG(dev,_port,dqUnitInPipe) :                                       \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev) ? \
        SMEM_LION_TXQ_EGR_STC_INTR_CAUSE_REG(dev,_port,dqUnitInPipe) : (0x01800130))

/* Egress STC Interrupt Mask Register - Cht/Cht2/Lion B0 */
#define SMEM_CHT_STC_INT_MASK_REG(dev,_port,dqUnitInPipe)\
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
            (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer0.transmitQueueInterrupt.egrSTCInterruptMask : \
            SMEM_LION_TXQ_EGR_STC_INTR_MASK_REG(dev,_port,dqUnitInPipe) :                                       \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev) ? \
        SMEM_LION_TXQ_EGR_STC_INTR_MASK_REG(dev,_port,dqUnitInPipe) : (0x01800134))

/* Cascading and Header Insertion Configuration Register - Cht/Cht2 */
#define SMEM_CHT_CAS_HDR_INS_REG(dev) \
        (SKERNEL_IS_XCAT_REVISON_A1_DEV(dev) ? \
            SMEM_XCAT_CAS_HDR_INS_REG(dev) : (0x07800004))

/* Egress DSA Tag Type Configuration Register - Cht/Cht2 */
#define SMEM_CHT_DSA_TAG_TYPE_REG(dev) \
        (SKERNEL_IS_XCAT_REVISON_A1_DEV(dev) ? \
            SMEM_XCAT_DSA_TAG_TYPE_REG(dev) : (0x07800020))

/* HA route device id modification enable */
#define SMEM_CHT_HA_ROUTE_DEVICE_ID_MODIFICATION_ENABLE_REG(dev)  \
    (SMEM_CHT_IS_SIP5_GET(dev) ? SMAIN_NOT_VALID_CNS : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->HA.deviceIDModificationEnable[0] : \
    (SKERNEL_IS_LION_REVISON_B0_DEV(dev) ? 0x0E800114 : \
    (SKERNEL_IS_XCAT_REVISON_A1_DEV(dev) ? 0x0f000110 : \
     0x07800110))))



/* Ingress VLAN EtherType Configuration Register - Cht/Cht2 */
#define SMEM_CHT_VLAN_ETH_TYPE_REG(dev)                                  \
        (0x0B800004)

/* Egress VLAN EtherType Configuration Register - Cht/Cht2 */
#define SMEM_CHT_EGR_VLAN_ETH_TYPE_REG(dev)                              \
        (0x07800018)

/* Egress VLAN Ethertype Select Register - Cht/Cht2 */
#define SMEM_CHT_EGR_VLAN_ETH_TYPE_SELECT_REG(dev, port)                       \
        (0x0780001C)

/* IPv6 Solicited-Node Multicast Address Configuration Register0 - Cht/Cht2/Cht3 */
#define SMEM_CHT_SOLIC_NODE_MCST_ADDR_REG(dev)\
    ((SKERNEL_IS_XCAT_REVISON_A1_DEV(dev)) ? \
      SMEM_XCAT_SOLIC_NODE_MCST_ADDR_REG(dev) : (0x0B800050))

/* IPv6 Solicited-Node Multicast Address Mask Register0 - Cht/Cht2/Ch3 */
#define SMEM_CHT_SOLIC_NODE_MCST_MASK_REG(dev) \
    ((SKERNEL_IS_XCAT_REVISON_A1_DEV(dev)) ? \
      SMEM_XCAT_SOLIC_NODE_MCST_MASK_REG(dev) : (0x0B800060))

/* Protocols Encapsulation Configuration Register - Cht2/Cht2 */
#define SMEM_CHT_PROT_ENCAP_CONF_REG(dev)           \
    ((SKERNEL_IS_XCAT_REVISON_A1_DEV(dev)) ?        \
        SMEM_XCAT_TTI_PROT_ENCAP_CONF_REG(dev) : (0x0B800520))

/* Protocols<2 n...2n+1> Configuration Register (0<=n<4) - Cht/Cht2 */
#define SMEM_CHT_PROT_CONF_REG(dev)         \
    ((SKERNEL_IS_XCAT_REVISON_A1_DEV(dev)) ?  \
        SMEM_XCAT_TTI_PROT_CONF_REG(dev) : (0x0B800500))

/* Address Update Queue Control Register - Cht/Cht2/Cht3 */
#define SMEM_CHT_AUQ_CTRL_REG(dev)                                       \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->MG.addrUpdateQueueConfig.generalAddrUpdateQueueCtrl   : \
        (0x000000C4))

/* FDB Global Configuration Register - Cht/Cht2/ */
#define SMEM_CHT_MAC_TBL_GLB_CONF_REG(dev)                               \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->FDB.FDBCore.FDBGlobalConfig.FDBGlobalConfig : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->FDB.FDBGlobalConfig.FDBGlobalConfig   : \
        (0x06000000)))

/* Message from CPU Register0 - Cht/Cht2 */
#define SMEM_CHT_MSG_FROM_CPU_0_REG(dev)                                 \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->FDB.FDBCore.FDBAddrUpdateMsgs.AUMsgFromCPU.AUMsgFromCPUWord[0] : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->FDB.FDBAddrUpdateMsgs.AUMsgFromCPUWord[0]   : \
        (0x06000040)))

/* Message From CPU Management - Cht/Cht2 */
#define SMEM_CHT_MSG_FROM_CPU_REG(dev)                                   \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->FDB.FDBCore.FDBAddrUpdateMsgs.AUMsgFromCPU.AUMsgFromCPUCtrl : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->FDB.FDBAddrUpdateMsgs.AUMsgFromCPUCtrl   : \
        (0x06000050)))

/* IPv6 Multicast Bridging Bytes Selection Registers - Cht/Cht2 */
#define SMEM_CHT_IPV6_MC_BRDG_BYTE_SEL_REG(dev)                          \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->L2I.IPv6McBridgingBytesSelection.IPv6MCBridgingBytesSelectionConfig : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->L2I.IPv6McBridgingBytesSelection.IPv6MCBridgingBytesSelectionConfig   : \
        (0x02040010)))

/* IPv6 ICMP Command Register - Cht/Cht2 */
#define SMEM_CHT_IPV6_ICMP_CMD_REG(dev)                                  \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->L2I.ctrlTrafficToCPUConfig.IPv6ICMPCommand : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->L2I.ctrlTrafficToCPUConfig.IPv6ICMPCommand   : \
        (0x02012800)))


/* Bridge Global Configuration Register0 - Cht/Cht2 */
#define SMEM_CHT_BRDG_GLB_CONF0_REG(dev)                                 \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->L2I.bridgeEngineConfig.bridgeGlobalConfig0 : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->L2I.bridgeEngineConfig.bridgeGlobalConfig[0]   : \
        (0x02040000)))

/* Bridge Global Configuration Register1 - Cht/Cht2 */
#define SMEM_CHT_BRDG_GLB_CONF1_REG(dev)                                 \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->L2I.bridgeEngineConfig.bridgeGlobalConfig1 : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->L2I.bridgeEngineConfig.bridgeGlobalConfig[1]   : \
        (0x02040004)))

/* Ingress VLAN Range Configuration Register - Cht/Cht2 */
#define SMEM_CHT_INGR_VLAN_RANGE_CONF_REG(dev)                          \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->L2I.VLANRangesConfig.ingrVLANRangeConfig : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->L2I.VLANRangesConfig.ingrVLANRangeConfig   : \
        (0x020400A8)))

/* Bridge Filter Counter Cht/Cht2/Cht3  */
#define SMEM_CHT_BRDG_FILTER_CNT_REG(dev)                                \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->L2I.bridgeDropCntrAndSecurityBreachDropCntrs.bridgeFilterCntr : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->L2I.bridgeDropCntrAndSecurityBreachDropCntrs.bridgeFilterCntr   : \
        (0x02040150)))

/* Global Security Breach Drop Counter - Cht/Cht2/Cht3 */
#define SMEM_CHT_GLB_SECUR_BRCH_DROP_CNT_REG(dev)                        \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->L2I.bridgeDropCntrAndSecurityBreachDropCntrs.globalSecurityBreachFilterCntr : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->L2I.bridgeDropCntrAndSecurityBreachDropCntrs.globalSecurityBreachFilterCntr   : \
        (0x02040104)))

/* Port/VLAN Security Breach Drop Counter - Cht/Cht2/Cht3 */
#define SMEM_CHT_PORT_VLAN_SECUR_BRCH_DROP_CNT_REG(dev)                  \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->L2I.bridgeDropCntrAndSecurityBreachDropCntrs.portVLANSecurityBreachDropCntr : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->L2I.bridgeDropCntrAndSecurityBreachDropCntrs.portVLANSecurityBreachDropCntr   : \
        (0x02040108)))

/* Security Breach Status Register0 - Cht/Cht2/Cht3 */
#define SMEM_CHT_SECUR_BRCH_STAT0_REG(dev)                               \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->L2I.securityBreachStatus.securityBreachStatus0 : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->L2I.securityBreachStatus.securityBreachStatus[0]   : \
        (0x020401A0)))

/* Security Breach Status Register1 - Cht/Cht2/Cht3 */
#define SMEM_CHT_SECUR_BRCH_STAT1_REG(dev)                               \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->L2I.securityBreachStatus.securityBreachStatus1 : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->L2I.securityBreachStatus.securityBreachStatus[1]   : \
        (0x020401A4)))

/* Security Breach Status Register2 - Cht/Cht2/Cht3 */
#define SMEM_CHT_SECUR_BRCH_STAT2_REG(dev)                               \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->L2I.securityBreachStatus.securityBreachStatus2 : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->L2I.securityBreachStatus.securityBreachStatus[2]   : \
        (0x020401A8)))

/* MAC Address Count0 Register - Cht/Cht2 */
#define SMEM_CHT_MAC_ADDR_CNT0_REG(dev)                                  \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->L2I.layer2BridgeMIBCntrs.MACAddrCount0 : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->L2I.layer2BridgeMIBCntrs.MACAddrCount[0]   : \
        (0x020400B0)))

#define SMEM_CHT_MAC_ADDR_CNT1_REG(dev)                                  \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->L2I.layer2BridgeMIBCntrs.MACAddrCount1 : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->L2I.layer2BridgeMIBCntrs.MACAddrCount[1]   : \
        (0x020400B4)))

#define SMEM_CHT_MAC_ADDR_CNT2_REG(dev)                                  \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->L2I.layer2BridgeMIBCntrs.MACAddrCount2 : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->L2I.layer2BridgeMIBCntrs.MACAddrCount[2]   : \
        (0x020400B8)))

/* Host Outgoing Packets Count Register - Cht/Cht2 */
#define SMEM_CHT_HOST_OUT_PKT_CNT_REG(dev)                               \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->L2I.layer2BridgeMIBCntrs.hostOutgoingPktsCount : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->L2I.layer2BridgeMIBCntrs.hostOutgoingPktsCount   : \
        (0x020400C0)))

/* Host Outgoing Broadcast Packet Count Register - Cht/Cht2 */
#define SMEM_CHT_HOST_OUT_BCST_PKT_CNT_REG(dev)                          \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->L2I.layer2BridgeMIBCntrs.hostOutgoingBcPktCount : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->L2I.layer2BridgeMIBCntrs.hostOutgoingBcPktCount   : \
        (0x020400D0)))

/* Host Outgoing Multicast Packet Count Register - Cht/Cht2 */
#define SMEM_CHT_HOST_OUT_MCST_PKT_CNT_REG(dev)                          \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->L2I.layer2BridgeMIBCntrs.hostOutgoingMcPktCount : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->L2I.layer2BridgeMIBCntrs.hostOutgoingMcPktCount   : \
        (0x020400CC)))

/* Host Incoming Packets Count Register - Cht/Cht2 */
#define SMEM_CHT_HOST_IN_PKT_CNT_REG(dev)                                \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->L2I.layer2BridgeMIBCntrs.hostIncomingPktsCount : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->L2I.layer2BridgeMIBCntrs.hostIncomingPktsCount   : \
        (0x020400BC)))

/* Matrix Source/Destination Packet Count Register - Cht/Cht2 */
#define SMEM_CHT_MATRIX_SRC_DST_PKT_CNT_REG(dev)                         \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->L2I.layer2BridgeMIBCntrs.matrixSourceDestinationPktCount : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->L2I.layer2BridgeMIBCntrs.matrixSourceDestinationPktCount   : \
        (0x020400D4)))

/* Counters Set0 Configuration Register - Cht/Cht2 */
#define SMEM_CHT_CNT_SET0_CONF_REG(dev)                                  \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMAIN_NOT_VALID_CNS : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->L2I.layer2BridgeMIBCntrs.cntrsSetConfig[0]   : \
        (0x020400DC)))

/* Set0 Incoming Packet Count Register - Cht/Cht2 */
#define SMEM_CHT_SET0_IN_PKT_CNT_REG(dev)                                \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->L2I.layer2BridgeMIBCntrs.setIncomingPktCount[0] : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->L2I.layer2BridgeMIBCntrs.setIncomingPktCount[0]   : \
        (0x020400E0)))

/* Set0 VLAN Ingress Filtered Packet Count Register - Cht/Cht2 */
#define SMEM_CHT_SET0_VLAN_IN_FILTER_CNT_REG(dev)                        \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->L2I.layer2BridgeMIBCntrs.setVLANIngrFilteredPktCount[0] : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->L2I.layer2BridgeMIBCntrs.setVLANIngrFilteredPktCount[0]   : \
        (0x020400E4)))

/* Set0 Security Filtered Packet Count Register - Cht/Cht2 */
#define SMEM_CHT_SET0_SECUR_FILTER_CNT_REG(dev)                          \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->L2I.layer2BridgeMIBCntrs.setSecurityFilteredPktCount[0] : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->L2I.layer2BridgeMIBCntrs.setSecurityFilteredPktCount[0]   : \
        (0x020400E8)))

/* Set0 Bridge Filtered Packet Count Register - Cht/Cht2 */
#define SMEM_CHT_SET0_BRDG_FILTER_CNT_REG(dev)                          \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->L2I.layer2BridgeMIBCntrs.setBridgeFilteredPktCount[0] : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->L2I.layer2BridgeMIBCntrs.setBridgeFilteredPktCount[0]   : \
        (0x020400EC)))

/* AU FIFO to CPU Configuration Register - Cht/Cht2 */
#define SMEM_CHT_AU_FIFO_TO_CPU_CONF_REG(dev)                           \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->FDB.FDBCore.FDBAddrUpdateMsgs.AUMsgToCPU.AUFIFOToCPUConfig : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->FDB.FDBAddrUpdateMsgs.AUFIFOToCPUConfig   : \
        (0x06000038)))

/* MAC Table Interrupt Cause Mask - Cht/Cht2/Ch3 */
#define SMEM_CHT_MAC_TBL_INTR_CAUSE_MASK_REG(dev)                        \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->FDB.FDBCore.FDBInterrupt.FDBInterruptMaskReg : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->FDB.FDBInterrupt.FDBInterruptMaskReg   : \
        (0x0600001C)))

/* MAC Table Interrupt Cause Register - Cht/Cht2/Ch3 */
#define SMEM_CHT_MAC_TBL_INTR_CAUSE_REG(dev)                             \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->FDB.FDBCore.FDBInterrupt.FDBInterruptCauseReg : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->FDB.FDBInterrupt.FDBInterruptCauseReg   : \
        (0x06000018)))

/* Learned Entry Discards Count Register - Cht/Cht2  */
#define SMEM_CHT_LEARN_ENTRY_DISCARD_CNT_REG(dev)                        \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->FDB.FDBCore.sourceAddrNotLearntCntr.learnedEntryDiscardsCount : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->FDB.sourceAddrNotLearntCntr.learnedEntryDiscardsCount   : \
        (0x06000030)))

/* MAC Table Action0 Register - Cht/Cht2 */
#define SMEM_CHT_MAC_TBL_ACTION0_REG(dev)               \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMAIN_NOT_VALID_CNS : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->FDB.FDBAction.FDBAction[0]   : \
        (0x06000004)))

/* MAC Table Action2 Register - Cht/Cht2... */
#define SMEM_CHT_MAC_TBL_ACTION2_REG(dev)               \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMAIN_NOT_VALID_CNS : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->FDB.FDBAction.FDBAction[2]   : \
        (0x06000020)))


/* Policers Global Configuration Register - Cht/Cht2 */
#define SMEM_CHT_POLICER_GLB_CONF_REG(dev, cycle) \
    SKERNEL_IS_XCAT_REVISON_A1_DEV(dev) ? \
        SMEM_XCAT_POLICER_GLB_CONF_REG(dev, cycle) : (0x0C000000)

/* Unicast Routing Engine Configuration Register - Cht/Cht2 */
#define SMEM_CHT_UC_ROUT_ENG_CONF_REG(dev) \
    SKERNEL_IS_XCAT_DEV(dev) ? \
        SMEM_XCAT_UC_ROUT_ENG_CONF_REG(dev) : (0x0C000040)

/* Master XSMI Interface Registers - Cht/Cht2 */
#define SMEM_CHT_XSMI_MNG_REG(dev)                                       \
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer0.XSMIConfig.XSMIManagement   : \
        SMAIN_NOT_VALID_CNS :                                       \
      (0x01CC0000))

#define SMEM_CHT_XSMI_ADDR_REG(dev)                                      \
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer0.XSMIConfig.XSMIAddr   : \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->MG.XSMI.XSMIAddr :                    \
      (0x01CC0008))

/* Ports VLAN, QoS and Protocol Access Registers - Cht/Cht2 */
#define SMEM_CHT_PORTS_VLAN_QOS_WORD0_REG(dev)                      (0x0B800320)

/* Action Table and Policy TCAM Access Data Register - Cht/Cht2 */
#define SMEM_CHT_PCL_ACTION_TCAM_DATA0_REG(dev)                                    \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev,pclActionTcamData,0)

#define SMEM_CHT_PCL_ACTION_TCAM_DATA3_REG(dev)                     (SMEM_CHT_PCL_ACTION_TCAM_DATA0_REG(dev) + 3*4)
#define SMEM_CHT_PCL_ACTION_TCAM_DATA6_REG(dev)                     (SMEM_CHT_PCL_ACTION_TCAM_DATA0_REG(dev) + 6*4)
#define SMEM_CHT_PCL_ACTION_TCAM_DATA9_REG(dev)                     (SMEM_CHT_PCL_ACTION_TCAM_DATA0_REG(dev) + 9*4)

/* Router Header Alteration Enable MAC SA Modification Register */
#define SMEM_CHT_ROUT_HA_MAC_SA_EN_REG(dev) \
        (SKERNEL_IS_XCAT_REVISON_A1_DEV(dev) ? \
            SMEM_XCAT_ROUT_HA_MAC_SA_EN_REG(dev) : (0x07800104))

/* Router MAC SA Base Register0 */
#define SMEM_CHT_MAC_SA_BASE0_REG(dev) \
        (SKERNEL_IS_XCAT_REVISON_A1_DEV(dev) ? \
            SMEM_XCAT_MAC_SA_BASE0_REG(dev) : (0x07800108))

/* Router Header Alteration MAC SA Modification Mode */
#define SMEM_CHT_MAC_SA_MODIFICATION_MODE_REG(dev) \
        (SKERNEL_IS_XCAT_REVISON_A1_DEV(dev) ? \
            SMEM_XCAT_MAC_SA_MODIFICATION_MODE_REG(dev) : (0x07800120))

/* Router Header Alteration Tables Access - Cht/Cht2 */
#define SMEM_CHT_ROUT_HA_DATA0_REG(dev)                             (0x07800200)

/* Action Table and Policy TCAM Access Control Register - Cht/Cht2 */
#define SMEM_CHT_PCL_ACTION_TCAM_CTRL_REG(dev)                             \
    ((SKERNEL_IS_CHEETAH3_DEV(dev) == 0) ?  (0x0B800130) :                 \
                        SMEM_CHT3_PCL_ACTION_TCAM_DATA_PARAM1_REG(dev))

/* FDB Access Data - Cht/Cht2 */
#define SMEM_CHT_MAC_TBL_ACC_DATA0_REG(dev)                              \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->FDB.FDBCore.FDBIndirectAccess.FDBIndirectAccessData[0] : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->FDB.FDBDirectAccess.FDBDirectAccessData[0]   : \
        (0x06000054)))


/* Port<n> MAC Control Register0 - Cht/Cht2/Ch3 */
/* Bridge Global Configuration Register1 Cht/Cht2 */
/* Global Security Breach Drop Counter Cht/Cht2 */
/* Port/VLAN Security Breach Drop Counter Cht/Cht2 */
/* Security Breach Status Register0 , cht/cht2 */
/* Security Breach Status Register1 , cht/cht2 */
/* Security Breach Status Register0 , cht/Cht2 */

#define SMEM_CHT_MAC_CONTROL0_REG(dev, port) \
        (SKERNEL_IS_CHEETAH3_DEV(dev) == 0) ? \
            SMEM_CHT1_MAC_CONTROL0_REG(dev,port) : \
            SMEM_CHT3_MAC_CONTROL0_REG(dev,port)

#define SMEM_CHT1_MAC_CONTROL0_REG(dev, port) \
        (IS_CHT_HYPER_GIGA_PORT(dev,port) ? \
            (0x0A800000 + (0x100 * (port))) : \
            (0x0A800000 + (0x100 * (port))))

/* Port<n> MAC Control Register2 - Cht/Cht2/Cht3*/
#define SMEM_CHT_MAC_CONTROL2_REG(dev, port) \
        (SKERNEL_IS_CHEETAH3_DEV(dev) == 0) ? \
            SMEM_CHT1_MAC_CONTROL2_REG(dev,port) : \
            SMEM_CHT3_MAC_CONTROL2_REG(dev,port)

/* Port<n> MAC Control Register2 - Cht/Cht2 */
#define SMEM_CHT1_MAC_CONTROL2_REG(dev,port) \
        ((dev)->supportRegistersDb ? (((port) == SNET_CHT_CPU_PORT_CNS  && ((dev)->cpuPortNoSpecialMac == 0)) ? \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->GOP.ports.gigPort63CPU.portMACCtrlReg[2] : \
         IS_CHT_HYPER_GIGA_PORT(dev,port) ? \
            (SMEM_CHT_MAC_REG_DB_GET(dev))->GOP.ports.XLGIP[port].portMACCtrlReg[2]   : \
            (SMEM_CHT_MAC_REG_DB_GET(dev))->GOP.ports.gigPort[port].portMACCtrlReg[2])  : \
        (IS_CHT_HYPER_GIGA_PORT(dev,port) ? \
            (0x0A800000 + (0x100 * (port))) : \
            (0x0A800008 + (0x100 * (port)))))

/* Port<n> Status Register0 - Cht/Cht2/Cht3 */
#define SMEM_CHT_PORT_STATUS0_REG(dev, port)            \
        (SKERNEL_IS_CHEETAH1_ONLY_DEV(dev)) ?        \
            SMEM_CHT1_PORT_STATUS0_REG(dev,port) :        \
        (SKERNEL_IS_CHEETAH2_ONLY_DEV(dev)) ?                \
            SMEM_CHT2_PORT_STATUS0_REG(dev,port) :        \
            SMEM_CHT3_PORT_STATUS0_REG(dev,port)

/* Port Auto-Negotiation Configuration. Not support XG and 'flex' ports (Hyper Giga) */
#define SMEM_CHT_PORT_AUTO_NEG_REG(dev,port)\
    ((dev)->supportRegistersDb ? ((port) == SNET_CHT_CPU_PORT_CNS  && ((dev)->cpuPortNoSpecialMac == 0)) ? \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->GOP.ports.gigPort63CPU.portAutoNegConfig  : \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->GOP.ports.gigPort[port].portAutoNegConfig  : \
        (SKERNEL_IS_CHEETAH3_DEV(dev) == 0) ? \
            (0x0A80000C  + (0x100 * (port))) : \
            (0x0A80000C  + (0x400 * (port))))

/* Port<n> Status Register0 - Cht/Cht2 */
#define SMEM_CHT1_PORT_STATUS0_REG(dev,port)\
        ((IS_CHT_HYPER_GIGA_PORT(dev,port) ? \
                (0x0A80000C  + (0x100 * (port))) : \
         (IS_CHT_REGULAR_PORT(dev,port)) ? \
                (0x0A800010  + (0x100 * (port))) : 0))

/* Port VLAN and QoS Configuration Entry Word0 Rd/Wr Register - Cht/Cht2 */
#define SMEM_CHT_PORTS_VLAN_QOS_PROT_REG(member) \
        (0x0B800300 + (0x4 * (member)))

/* Policy TCAM Entry<n> Mask Word<m> (0<=n<1024, 0<=m<3) - Cheetah only */
#define SMEM_CHT_PCL_TCAM_ENTRY_MASK0_REG(entry_index)                          \
        (0x0B810000 + (0x20 * (entry_index)))

/* Policy TCAM Entry<n> Mask Word<m> (0<=n<1024, 3<=m<6) - Cheetah only */
#define SMEM_CHT_PCL_TCAM_ENTRY_MASK3_REG(entry_index) \
        (SMEM_CHT_PCL_TCAM_ENTRY_MASK0_REG(entry_index) + 0x10)

/* Policy TCAM Mask Entry<n> Control0 Register (0<=n<1024) - Cheetah only */
#define SMEM_CHT_PCL_TCAM_MASK_CTRL0_REG(entry_index) \
        (SMEM_CHT_PCL_TCAM_ENTRY_MASK0_REG(entry_index) + 0xc)

/* Policy TCAM Data Entry<n> Control1 Register (0<=n<1024) - Cheetah only */
#define SMEM_CHT_PCL_TCAM_MASK_CTRL1_REG(entry_index) \
        (SMEM_CHT_PCL_TCAM_ENTRY_MASK0_REG(entry_index) + 0x1c)

/* Policy TCAM Entry<n> Data Word<m> (0<=n<1024, 0<=m<3) - Cheetah only */
#define SMEM_CHT_PCL_TCAM_ENTRY_WORD0_REG(entry_index) \
        SMEM_CHT_PCL_TCAM_ENTRY_MASK0_REG(entry_index) + 0x8000

/* Policy TCAM Entry<n> Data Word<m> (0<=n<1024, 3<=m<6) - Cheetah only */
#define SMEM_CHT_PCL_TCAM_ENTRY_WORD3_REG(entry_index) \
        (SMEM_CHT_PCL_TCAM_ENTRY_WORD0_REG(entry_index) + 0x10)

/* Policy TCAM Data Entry<n> Control0 - Cheetah only */
#define SMEM_CHT_PCL_TCAM_WORD_CTRL0_REG(entry_index) \
        (SMEM_CHT_PCL_TCAM_ENTRY_WORD0_REG(entry_index) + 0xc)

/* Policy TCAM Data Entry<n> Control1 - Cheetah only */
#define SMEM_CHT_PCL_TCAM_WORD_CTRL1_REG(entry_index) \
        (SMEM_CHT_PCL_TCAM_ENTRY_WORD0_REG(entry_index) + 0x1c)

/* DSCP<4n...4n+3>2DSCP Map Table (0<=n<16) - Cht/Cht2 */
#define SMEM_CHT_DSCP_2_DSCP_MAP_TBL_MEM(dev,entry_index,instance) \
            (SMEM_TABLE_ENTRY_2_PARAMS_INDEX_GET_MAC(dev,dscpToDscpMap,entry_index,instance))

/* DSCP<4n...4n+3>2QoSProfile map Table (0<=n<16) - Cht/Cht2 */
#define SMEM_CHT_DSCP_2_QOS_PROF_MAP_REG(dev,entry_index,instance) \
            (SMEM_TABLE_ENTRY_2_PARAMS_INDEX_GET_MAC(dev,dscpToQoSProfile,entry_index,instance))

/* UP<4n...4n+3>2QoSProfile map Table (0<=n<2) - Cht/Cht2/Ch3 */
#define SMEM_CHT_UP_2_QOS_PROF_MAP_TBL_MEM(dev, up, cfi , instance) \
            (SMEM_TABLE_ENTRY_3_PARAMS_INDEX_GET_MAC(dev, upToQoSProfile, up, cfi,instance))

/* Exp<4n...4n+3>2QoSProfile map Table (0<=n<2)  */
#define SMEM_CHT_EXP_2_QOS_PROF_MAP_TBL_MEM(dev,entry_index, instance) \
            (SMEM_TABLE_ENTRY_2_PARAMS_INDEX_GET_MAC(dev,expToQoSProfile,entry_index,instance))

/* Number of Trunk Members Table Entry
   trunkId ---> 1 based number (cant be 0)
   every 8 trunk in a register - Cht/Cht2 */
#define SMEM_CHT_TRUNK_MEM_NUM_TBL_MEM(dev, trunkId)    \
        SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev,trunkNumOfMembers,(trunkId-1))

/* start field bit in Trunk Members Table Entry
   trunkId ---> 1 based number (cant be 0)
   every 8 trunk in a register Cht/Cht2 */
#define SMEM_CHT_TRUNK_MEM_START_FIELD(trunkId) (4 * (((trunkId)-1) % 8))

/* Ingress Port<n> Bridge Configuration
    Register0 (0<=n<27, CPU Port n= 0x3F) , Cht only */
#define SMEM_CHT_INGR_PORT_BRDG_CONF0_REG(dev, port) \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->L2I.ingrPortsBridgeConfig.ingrPortBridgeConfig0[CPU_PORT_INDEX(dev, port)]  : \
        (0x02000000 + (0x1000 * (port))))

/* Ingress Port<n> Bridge Configuration
    Register1 (0<=n<27, CPU Port n= 0x3F) , Cht/Cht2 */
#define SMEM_CHT_INGR_PORT_BRDG_CONF1_REG(dev, port) \
     ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->L2I.ingrPortsBridgeConfig.ingrPortBridgeConfig1[CPU_PORT_INDEX(dev, port)]  : \
        (0x02000010 + (0x1000 * (port))))

/* IEEE Reserved Multicast Configuration Register<n> (0<=n<16) , Cht/Cht2 */
#define SMEM_CHT_IEEE_RSRV_MCST_CONF_TBL_MEM(dev, entry_index, table_set) \
        SMEM_TABLE_ENTRY_2_PARAMS_INDEX_GET_MAC(dev, ieeeRsrvMcConfTable, entry_index, table_set)

/* IEEE Reserved Multicast CPU Index */
#define SMEM_CHT_IEEE_RESERV_MC_CPU_INDEX_TBL_MEM(dev, protocol) \
        SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, ieeeRsrvMcCpuIndex, protocol)

/* IPv6 ICMP Message Type Configuration Register<n> (0<n<=2) , Cht/Cht2 */
#define SMEM_CHT_IPV6_ICMP_MSG_TYPE_CONF_REG(dev, entry_index) \
     (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->L2I.ctrlTrafficToCPUConfig.IPv6ICMPMsgTypeConfig[(entry_index / 4)] : \
     ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->L2I.ctrlTrafficToCPUConfig.IPv6ICMPMsgTypeConfig[(entry_index / 4)]  : \
        (0x02010800 + (0x1000 * ((entry_index) / 4)))))

/* IPv4 Multicast Link-Local Configuration Register<n> (0<=n<8) , Cht/Cht2 */
#define SMEM_CHT_IPV4_MCST_LINK_LOCAL_CONF_REG(dev, entry_index) \
     (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->L2I.ctrlTrafficToCPUConfig.IPv4McLinkLocalConfig[(entry_index)] : \
     ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->L2I.ctrlTrafficToCPUConfig.IPv4McLinkLocalConfig[(entry_index)]  : \
        (0x02020800 + (0x1000 * (entry_index)))))

/* IPv6 Multicast Link-Local Configuration Register<n> (0<=n<8) , Cht/Cht2 */
#define SMEM_CHT_IPV6_MCST_LINK_LOCAL_CONF_REG(dev, entry_index) \
     (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->L2I.ctrlTrafficToCPUConfig.IPv6McLinkLocalConfig[(entry_index)] : \
     ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->L2I.ctrlTrafficToCPUConfig.IPv6McLinkLocalConfig[(entry_index)]  : \
        (0x02028800 + (0x1000 * (entry_index)))))

/* Port<n> Interrupt Cause Register , Cht/Cht2/Ch3 */
#define SMEM_CHT_PORT_INT_CAUSE_REG(dev, port) \
        (SKERNEL_IS_CHEETAH3_DEV(dev) == 0) ?   \
            SMEM_CHT1_PORT_INT_CAUSE_REG(dev,port) : \
            SMEM_CHT3_PORT_INT_CAUSE_REG(dev,port)

/* Port<n> Interrupt Cause Register , Cht/Cht2 */
#define SMEM_CHT1_PORT_INT_CAUSE_REG(dev,port) \
       ((IS_CHT_HYPER_GIGA_PORT(dev,port) ? \
            (0x0A800014  + (0x100 * (port))) : \
        IS_CHT_REGULAR_PORT(dev,port) ? \
            (0x0A800020  + (0x100 * (port))) : 0))

/* Port<n> Interrupt Mask Register , Cht/Cht2/Ch3 */
#define SMEM_CHT_PORT_INT_CAUSE_MASK_REG(dev, port) \
    (SMEM_CHT_PORT_INT_CAUSE_REG(dev, port) + 0x4)

/* Tri-speed ports MIB Counters Interrupt Summary Register , Cht/Cht2/Cht3 */
#define SMEM_CHT_PORTS_MIB_CNT_INT_SUM_REG(dev)                          \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->LMS.LMS0.MIBCntrsInterruptSummary   : \
        (0x04004010))

/* Tri-speed ports MIB Counters Interrupt Summary Mask Register , Cht/Cht2 */
#define SMEM_CHT_PORTS_MIB_CNT_INT_SUM_MASK_REG(dev)            \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->LMS.LMS0LMS0Group0.portsMIBCntrsInterruptSummaryMask   : \
        (0x04805110))

/* The summary bit of GOP<0-3> , Cht/Cht2 */
#define  SMEM_CHT_PORT_MIB_CNT_GOP_BIT(port) \
        (4 - ((port) / 6))

/* The summary bit of port port<0-23> , Cht/Cht2 */
#define  SMEM_CHT_PORT_MIB_CNT_PORT_BIT(port) \
        (5 + (port))

/* HyperG.Stack Ports MIB Counters and XSMII Configuration Register, Cht/Cht2/Cht3 */
#define SMEM_CHT_HYPG_MIB_COUNT_XSMII_CONF_REG(dev)                      \
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer0.XGCtrl.XGCtrlReg   : \
        SMAIN_NOT_VALID_CNS :                                       \
      (0x01800180))
/* calculate device units base address alignment from num of bits used for alignment parameter */
#define SMEM_CHT_UNIT_BASE_ADDR_ALIGN_FROM_NUM_OF_BITS(dev) \
    ((dev)->unitBaseAddrAlignmentNumBits == 0 ? (1<<23) : (1<<((dev)->unitBaseAddrAlignmentNumBits)))

/* MAC MIB Counters (Ports 0.23) , Cht/Cht2 */
#define SMEM_CHT_PORT_MAC_COUNT_REG(dev,entry_index) \
        ((dev)->supportRegistersDb ?     \
            ((((SMEM_CHT_MAC_REG_DB_GET(dev))->LMS.LMS0.MIBCntrsCtrlReg0forPorts0Through5 & 0xFF800000) + \
                0x10000 + \
                  SMEM_CHT_UNIT_BASE_ADDR_ALIGN_FROM_NUM_OF_BITS(dev) * ((entry_index) / 6)) + \
                    (0x00000080 * ((entry_index) % 6))) : \
             ((0x04010000 + 0x00800000 * ((entry_index) / 6)) + \
                (0x00000080 * ((entry_index) % 6))))

/* MAC MIB Counters Capture Port Cht/Cht2/Ch3*/
#define SMEM_CHT_CAPTURE_PORT_REG(dev,port) \
        ((dev)->supportRegistersDb ?     \
            (((SMEM_CHT_MAC_REG_DB_GET(dev))->LMS.LMS0.MIBCntrsCtrlReg0forPorts0Through5 & 0xFF800000) + \
                0x10300 + \
                  SMEM_CHT_UNIT_BASE_ADDR_ALIGN_FROM_NUM_OF_BITS(dev) * ((port) / 6)) : \
            (0x04010300 + 0x00800000 * ((port) / 6)))

/* port 27 is in offset to the 'formula' of the other 3 XG ports */
#define SMEM_CHT2_PORT27_MIB_OFFSET(port) \
    (((port) == 27) ? 0x00100000 : 0)

/* Hyper.GStack Ports MAC MIB Counters (Ports 24.26) , Cht/Cht2/Cht3 */
#define SMEM_CHT_HGS_PORT_MAC_COUNT_REG(dev, port) \
    (SKERNEL_IS_CHEETAH3_DEV(dev) == 0) ?   \
        (0x01C00000 + 0x00040000 * ((port) % 24) + SMEM_CHT2_PORT27_MIB_OFFSET(port)) : \
        (SMEM_CHT3_HGS_PORT_MAC_MIB_COUNT_REG(dev, port))


/* Hyper.GStack Ports Capture MAC MIB Counters  Cht/Cht2/Cht3 */
#define SMEM_CHT_HGS_PORT_CAPTURE_MAC_MIB_COUNT_REG(dev, port) \
        (SKERNEL_IS_CHEETAH3_DEV(dev) == 0) ?   \
            (0x01C00080 + 0x00040000 * ((port) % 24)  + SMEM_CHT2_PORT27_MIB_OFFSET(port)) : \
        (SMEM_CHT3_HGS_PORT_CAPTURE_MAC_MIB_COUNT_REG(dev, port))

/* CPU Port GoodFramesReceived Counter , Cht/Cht2 */
#define SMEM_CHT_CPU_PORT_GOOD_FRAMES_COUNT_REG(dev)                     \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->MG.cpuPortConfigRegAndMIBCntrs.cpuPortGoodFramesReceivedCntr   : \
        (0x00000070))

/* CPU Port GoodOctetsReceived Counter , Cht/Cht2 */
#define SMEM_CHT_CPU_PORT_GOOD_OCTETS_COUNT_REG(dev)                     \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->MG.cpuPortConfigRegAndMIBCntrs.cpuPortGoodOctetsReceivedCntr   : \
        (0x00000078))

/* MAC QoS Table Entry<n> (1<=n<8) , Cht/Cht2 */
#define SMEM_CHT_MAC_QOS_TBL_ENTRY_REG(dev, entry_index) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->L2I.MACBasedQoSTable.MACQoSTableEntry[entry_index] : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->L2I.MACBasedQoSTable.MACQoSTableEntry[entry_index]   : \
        (0x02000200  + (0x04 * (entry_index)))))

/* PCL-ID Table Access Data Register , Cht/Cht2 */
#define SMEM_CHT_PCL_ID_TBL_ACC_DATA_REG(dev)                            (0x0B800200)

/* Policers QoS Remarking and Initial DP Table Data Access Register , Cht/Cht2 */
#define SMEM_CHT_POLICER_QOS_REM_DATA_REG(dev)                           (0x0C000020)

/* Policer Entry Word0 Access Register , Cht/Cht2 */
#define SMEM_CHT_POLICER_ENT_WORD_0_REG(dev)                             (0x0C00000C)

/* Policer Counters Table Data0 Access Register , Cht/Cht2 */
#define SMEM_CHT_POLICER_CNT_TBL_DATA0_REG(dev)                          (0x0C000030)

/* Trunk table Data Access Register , Cht/Cht2 */
#define SMEM_CHT_TRUNK_TBL_DATA_REG(dev)                               \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMAIN_NOT_VALID_CNS : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->EQ.trunksMembersTable.trunkTableDataAccess   : \
        (0x0B001000)))

/* QoSProfile to QoS table Data Access Register , Cht/Cht2 */
#define SMEM_CHT_QOS_PROFILE_DATA_REG(dev)                               \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMAIN_NOT_VALID_CNS : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->EQ.qoSProfileToQoSTable.qoSProfileToQoSTableDataAccess   : \
        (0x0B002000)))

/* CPU Code Table Data Access Register , Cht/Cht2 */
#define SMEM_CHT_CPU_CODE_DATA_REG(dev)                                  \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMAIN_NOT_VALID_CNS : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->EQ.CPUCodeTable.CPUCodeTableDataAccess   : \
        (0x0B003000)))

/* Statistical Rate Limits Table Data Access Register , Cht/Cht2 */
#define SMEM_CHT_STAT_RATE_LIMIT_DATA_REG(dev)                           \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMAIN_NOT_VALID_CNS : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->EQ.statisticalRateLimitsTable.statisticalRateLimitsTableDataAccess   : \
        (0x0B004000)))

/* Ingress STC Table Word0 Access Register , Cht/Cht2 */
#define SMEM_CHT_INGR_STC_TBL_WORD0_REG(dev)                             \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMAIN_NOT_VALID_CNS : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->EQ.ingrSTCTable.ingrSTCTableWord0Access   : \
        (0x0B005000)))

/* Ingress Drop Counter Register */
#define SMEM_CHT_INGR_DROP_CNT_REG(dev)                          \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EQ.ingrDropCntr.ingrDropCntr : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->EQ.ingrDropCntr.ingrDropCntr : \
        (0x0B000040)))

/* Ingress Drop Counter Configuration Register */
#define SMEM_CHT_INGR_DROP_CNT_CONFIG_REG(dev)                          \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EQ.ingrDropCntr.ingrDropCntrConfig : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->EQ.ingrDropCntr.ingrDropCntrConfig  : \
        (0x0B00003C)))

/* VLT Tables Access Data Register2 , Cht/Cht2 */
#define SMEM_CHT_VLAN_TBL_WORD2_REG(dev)                                 \
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer0.VLANAndMcGroupAndSpanStateGroupTables.globalConfig   : \
        SMAIN_NOT_VALID_CNS :                                       \
      (0x0A000000))

/* VLT Tables Access Data Register1 , Cht/Cht2 */
#define SMEM_CHT_VLAN_TBL_WORD1_REG(dev)                                 (0x0A000004)

/* VLT Tables Access Data Register0 , Cht/Cht2 */
#define SMEM_CHT_VLAN_TBL_WORD0_REG(dev)                                 (0x0A000008)


/* SDMA Configuration Register , Cht1,2,3 */
#define SMEM_CHT_SDMA_CONFIG_REG(dev)                                    \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->MG.SDMARegs.SDMAConfig   : \
        (0x00002800))

/* Transmit SDMA Queue Command Register , Cht/Cht2 */
#define SMEM_CHT_TX_SDMA_QUE_CMD_REG(dev)                                \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->MG.SDMARegs.transmitSDMAQueueCommand   : \
        (0x00002868))

/* Transmit SDMA Current Descriptor Pointer<n> Register , Cht/Cht2 */
#define SMEM_CHT_TX_SDMA_CUR_DESC_PTR0_REG(dev)                          \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->MG.SDMARegs.transmitSDMACurrentDescPointer[0]   : \
        (0x000026C0))

/* Bridge Interrupt Cause Register , Cht/Cht2 */
#define SMEM_CHT_BRIDGE_INT_CAUSE_REG(dev)                               \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->L2I.bridgeEngineInterrupts.bridgeInterruptCause : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->L2I.bridgeEngineInterrupts.bridgeInterruptCause   : \
        (0x02040130)))

/* Ethernet Bridge Interrupt Mask Cause Register , Cht/Cht2 */
#define SMEM_CHT_BRIDGE_INT_MASK_CAUSE_REG(dev)                          \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->L2I.bridgeEngineInterrupts.bridgeInterruptMask : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->L2I.bridgeEngineInterrupts.bridgeInterruptMask   : \
        (0x02040134)))

/* Indirect tables */

/* SDMA Interrupt Mask Register , Cht/Cht2 */
#define SMEM_CHT_SDMA_INT_MASK_REG(dev)                                  \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->MG.SDMARegs.receiveSDMAInterruptMask   : \
        (0x00002814))

/* SDMA Interrupt Registers , Cht/Cht2 */
#define SMEM_CHT_SDMA_INT_CAUSE_REG(dev)                                 \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->MG.SDMARegs.receiveSDMAInterruptCause   : \
         (0x0000280C))

/* Transmit SDMA Interrupt Cause Register , Cht/Cht2 */
#define SMEM_CHT_TX_SDMA_INT_CAUSE_REG(dev)                              \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->MG.SDMARegs.transmitSDMAInterruptCause   : \
        (0x00002810))

/* Transmit SDMA Interrupt Mask Register  , Cht/Cht2 */
#define SMEM_CHT_TX_SDMA_INT_MASK_REG(dev)                               \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->MG.SDMARegs.transmitSDMAInterruptMask   : \
        (0x00002818))

/* EQ process related registers */
/* Ingress STC Configuration Register , Cht/Cht2 */
#define SMEM_CHT_INGRESS_STC_CONF_REG(dev)                               \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EQ.ingrSTCConfig.ingrSTCConfig : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->EQ.ingrSTCConfig.ingrSTCConfig   : \
        (0x0B00001C)))

/* EQ Interrupt Cause Register */
#define SMEM_CHT_EQ_INT_CAUSE_REG(dev)                          \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EQ.preEgrInterrupt.preEgrInterruptSummary : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->EQ.preEgrInterrupt.preEgrInterruptSummary   : \
        (0x0B000058)))

/* EQ Interrupt mask Register */
#define SMEM_CHT_EQ_INT_MASK_REG(dev)                          \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EQ.preEgrInterrupt.preEgrInterruptSummaryMask : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->EQ.preEgrInterrupt.preEgrInterruptSummaryMask   : \
        (0x0B00005c)))


/* Ingress STC Interrupt Cause Register , Cht/Cht2 */
#define SMEM_CHT_INGRESS_STC_INT_CAUSE_REG(dev,port)                          \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EQ.ingrSTCInterrupt.ingrSTCInterruptCause[(port) / 16] : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->EQ.ingrSTCInterrupt.ingrSTCInterruptCause   : \
        (0x0B000020)))

/* Ingress STC Interrupt Mask Register , Cht/Cht2 */
#define SMEM_CHT_INGRESS_STC_INT_MASK_REG(dev,port)                          \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EQ.ingrSTCInterrupt.ingrSTCInterruptMask[(port) / 16] : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->EQ.ingrSTCInterrupt.ingrSTCInterruptMask   : \
        (0x0B000024)))

/* Statistic Sniffing Configuration Register , Cht/Cht2 */
#define SMEM_CHT_STAT_SNIF_CONF_REG(dev)                                 \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EQ.mirrToAnalyzerPortConfigs.ingrStatisticMirrToAnalyzerPortConfig : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->EQ.mirrToAnalyzerPortConfigs.ingrStatisticMirrToAnalyzerPortConfig     : \
        (0x0B000000)))

/* Analyzer Port Configuration Register , Cht/Cht2 */
#define SMEM_CHT_ANALYZER_PORT_CONF_REG(dev)                        (0x0B000008)/*(not used from Lion)*/


/* Egress Monitoring Enable Configuration Register , Cht/Cht2 */
#define SMEM_CHT_EGR_MON_EN_CONF_REG(dev)                                \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMAIN_NOT_VALID_CNS : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->EQ.mirrToAnalyzerPortConfigs.egrMonitoringEnableConfig[0]      : \
        (0x0B00000C)))

/* Ingress and Egress Monitoring to
    Analyzer QoS Configuration Register , Cht/Cht2 */
#define SMEM_CHT_INGR_EGR_MON_TO_ANALYZER_QOS_CONF_REG(dev)              \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EQ.mirrToAnalyzerPortConfigs.ingrAndEgrMonitoringToAnalyzerQoSConfig : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->EQ.mirrToAnalyzerPortConfigs.ingrAndEgrMonitoringToAnalyzerQoSConfig   : \
        (0x0B000004)))

/* CPU Target Device Configuration Register0 , Cht/Cht2 */
#define SMEM_CHT_TRG_DEV_CONF0_REG(dev)                                 \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EQ.CPUTargetDeviceConfig.CPUTargetDeviceConfigReg0 : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->EQ.CPUTargetDeviceConfig.CPUTargetDeviceConfigReg0     : \
        (0x0B000010)))

/* CPU Target Device Configuration Register1 , Cht/Cht2 */
#define SMEM_CHT_TRG_DEV_CONF1_REG(dev)                                  \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EQ.CPUTargetDeviceConfig.CPUTargetDeviceConfigReg1 : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->EQ.CPUTargetDeviceConfig.CPUTargetDeviceConfigReg1     : \
        (0x0b000014)))

/* Cht/Cht2 */
#define SFDB_CHT_DEVICE_TBL_MEM(dev)                             \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMAIN_NOT_VALID_CNS : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->FDB.deviceTable.deviceTable    : \
        (0x06000068)))

/* Cht only , why it is used in Cht2 ?*/
#define SMEM_CHT1_PCL_ACTION_TBL_MEM(entry_index) \
    (0x0D200000 | ((entry_index) * 0xC))

/* Cht/Cht2/Ch3 */
#define SMEM_CHT_PCL_ID_TBL_MEM(dev, entry_index) \
    SMEM_CHT_PCL_CONFIG_TBL_MEM(dev,entry_index,(GT_U32)0)

/* Cht only */
#define SMEM_CHT1_PROT_VLAN_QOS_TBL_MEM(member, entry_index) \
    (0x0D100000 | ((member) << 16) | ((entry_index) * 0x4))

/* Cht/Cht2 */
#define SMEM_CHT_PORT_VLAN_QOS_CONFIG_TBL_MEM(dev, entry_index)                     \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev,portVlanQosConfig,entry_index)

/* Cht/Cht2/Cht3 */
#define SMEM_CHT_MAC_TBL_MEM(dev, entry_index) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev,fdb,entry_index)

/* Cht/Cht2/Cht3 */
#define SMEM_CHT_POLICER_TBL_MEM(dev, cycle, entry_index)  \
    ( smemChtPolicerTableIndexCheck(dev, cycle, entry_index) ?         \
      SMEM_XCAT_POLICER_METER_ENTRY_TBL_MEM(dev,SMAIN_DIRECTION_INGRESS_E,cycle, entry_index) : \
      SMAIN_NOT_VALID_CNS)

/* Cht/Cht2/Cht3 */
#define SMEM_CHT_POLICER_QOS_TBL_MEM(dev, cycle, entry_index) \
    SMEM_TABLE_ENTRY_INDEX_DUP_TBL_GET_MAC(dev,policerQos,entry_index,cycle)

/* Cht/Cht2/Cht3 */
#define SMEM_CHT_POLICER_CNT_TBL_MEM(dev, cycle, entry_index) \
    SMEM_XCAT_POLICER_CNT_ENTRY_TBL_MEM(dev,SMAIN_DIRECTION_INGRESS_E,cycle, entry_index)


/* trunkId is 1 based number -- can't be 0  -- 1-127
   member is 0 base number -- 0-7    , Cht/Cht2*/
#define SMEM_CHT_TRUNK_TBL_MEM(dev, member, trunkId) \
    SMEM_TABLE_ENTRY_2_PARAMS_INDEX_GET_MAC(dev,trunkMembers,member, trunkId)

/* Cht / Cht2 */
#define SMEM_CHT_QOS_TBL_MEM(dev, entry_index)   \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev,qosProfile,entry_index)

/* Cht / Cht2 */
#define SMEM_CHT_CPU_CODE_TBL_MEM(dev, entry_index) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev,cpuCode,entry_index)

/* Cht / Cht2 */
#define SMEM_CHT_STAT_RATE_TBL_MEM(dev, entry_index) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev,statisticalRateLimit,entry_index)

/* Cht / Cht2 */
#define SMEM_CHT_INGR_STC_TBL_MEM(dev, entry_index)                   \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev,ingrStc,entry_index)

/* Cht/Cht2/Cht3 */
#define SMEM_CHT_VLAN_TBL_MEM(dev, entry_index) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev,vlan,entry_index)

/* Cht / Cht2 / xCat2 (BullsEye) */
#define SMEM_CHT_MCST_TBL_MEM(dev, entry_index) \
        SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev,mcast,entry_index)

/* Cht / Cht2/ Cht3 */
#define SMEM_CHT_STP_TBL_MEM(dev, entry_index) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev,stp,entry_index)

#define SMEM_CHT_ARP_TBL_MEM(dev, entry_index) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev,arp,entry_index)

/* VLAN/Port MAC SA Entry<n> (0=<n<4096) Cht/Cht2  */
#define SMEM_CHT_MAC_SA_TBL_MEM(dev, entry_index) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev,vlanPortMacSa,entry_index)

#define SMEM_CHT_SUB_UNIT_INDEX_FIRST_BIT_CNS           20

/* Unit indexes for indirect tables */
#define SMEM_CHT_GROUP_INDEX_FIRST_BIT_CNS              16
#define SMEM_CHT_GROUP_INDEX_SECOND_BIT_CNS             24

/* Sub unit indexes for indirect table */
#define SMEM_CHT_VLAN_QOS_SUB_UNIT_INDEX_CNS             0
#define SMEM_CHT_PROT_VID_QOS_SUB_UNIT_INDEX_CNS         1
#define SMEM_CHT_PCL_ACTION_SUB_UNIT_INDEX_CNS           2
#define SMEM_CHT_PCL_ID_SUB_UNIT_INDEX_CNS               3

#define SMEM_CHT_POLICER_TBL_SUB_UNIT_INDEX_CNS          0
#define SMEM_CHT_POLICER_QOS_REM_UNIT_INDEX_CNS          1
#define SMEM_CHT_POLICER_COUNTER_UNIT_INDEX_CNS          2

#define SMEM_CHT_TRUNK_TBL_MEMBER_SUB_UNIT_INDEX_CNS     0
#define SMEM_CHT_QOS_PROFILE_UNIT_INDEX_CNS              1
#define SMEM_CHT_CPU_CODE_UNIT_INDEX_CNS                 2
#define SMEM_CHT_STAT_RATE_LIM_UNIT_INDEX_CNS            3
#define SMEM_CHT_PORT_INGRESS_STC_UNIT_INDEX_CNS         4

#define SMEM_CHT_VLAN_TBL_SUB_UNIT_INDEX_CNS             0
#define SMEM_CHT_MCST_TBL_SUB_UNIT_INDEX_CNS             1
#define SMEM_CHT_STP_TBL_SUB_UNIT_INDEX_CNS              2

#define SMEM_CHT_ARP_TBL_SUB_UNIT_INDEX_CNS              0
#define SMEM_CHT_MAC_SA_TBL_SUB_UNIT_INDEX_CNS           1

/* port 24..27 are hyper giga ports in cheetah2 */
#define SMEM_CHT_MAC_CONTROL_REG(dev, port) \
    ((SKERNEL_IS_CHEETAH3_DEV(dev) == 0) ?   \
        SMEM_CHT1_MAC_CONTROL1_REG(dev,port) : \
        SMEM_CHT3_MAC_CONTROL1_REG(dev,port))

/* port 24..27 are hyper giga ports in cheetah2 */
#define SMEM_CHT1_MAC_CONTROL1_REG(dev,port) \
    ((IS_CHT_HYPER_GIGA_PORT(dev,port)==GT_FALSE) ? \
       (0x0A800004+(0x100*(port))) : \
       (0x0A800004+(0x100*(port))) )

/* Cht/Cht2 */
#define SMEM_CHT_CPU_PORT_GOOD_FRM_CNT_REG(dev)                          \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->MG.cpuPortConfigRegAndMIBCntrs.cpuPortGoodFramesSentCntr   : \
        (0x00000060))

/* Cht/Cht2 */
#define SMEM_CHT_CPU_PORT_GOOD_OCT_CNT_REG(dev)                          \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->MG.cpuPortConfigRegAndMIBCntrs.cpuPortGoodOctetsSentCntr   : \
        (0x00000068))

/* Receive SDMA Current Rx Descriptor Pointer [7:0] (CRDPx) Register , Cht/Cht2  */
#define SMEM_CHT_RX_QUEUE_EN_REG(dev)                                    \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->MG.SDMARegs.receiveSDMAQueueCommand    : \
        (0x00002680))

/* Port offset from 0...5, ports from 0...27 */
#define SMEM_CHT_PORT_OFFSET(port)                                  ((port) % 0x6)

/* Group number from 0...3, ports from 0...27 */
#define SMEM_CHT_PORT_TO_GROUP(port)                                ((port) / 0x6)

#define SMEM_CHT_PORT_COUNT_UPDATE_ENABLE_BIT(port)  \
    (8 + (SMEM_CHT_PORT_OFFSET(port)));

#define SMEM_CHT_PORT_HIST_COUNT_UPDATE_ENABLE_BIT(port)  \
    (port<24) ? 6 : 9 + SMEM_CHT_PORT_OFFSET(port);

#define SMEM_CHT1_MIB_COUNT_CONTROL_REG(dev,port) \
     ((dev)->supportRegistersDb ? ((port < 6)  ? (SMEM_CHT_MAC_REG_DB_GET(dev))->LMS.LMS0.MIBCntrsCtrlReg0forPorts0Through5 : \
 (port < 12) ? (SMEM_CHT_MAC_REG_DB_GET(dev))->LMS.LMS0LMS0Group0.MIBCntrsCtrlReg0forPorts6Through11 : \
 (port < 18) ? (SMEM_CHT_MAC_REG_DB_GET(dev))->LMS.LMS1.MIBCntrsCtrlReg0forPorts12Through17 : \
               (SMEM_CHT_MAC_REG_DB_GET(dev))->LMS.LMS1LMS1Group0.MIBCntrsCtrlReg0forPorts18Through23 ) : \
     (((!IS_CHT_REGULAR_PORT(dev,port)) || (SMEM_CHT_PORT_TO_GROUP(port) > 3)) ? 0x01800180 : \
     (0x04004020 + (0x00800000 * SMEM_CHT_PORT_TO_GROUP(port)))))

/* Cht/Cht2/Cht3 uses same reg addresses for Gig Ports */
#define SMEM_CHT_MIB_COUNT_CONTROL_REG(dev, port) \
            (SMEM_CHT1_MIB_COUNT_CONTROL_REG(dev, port))


/* Receive SDMA<n> Current Rx Descriptor Pointer [7:0]
            (CRDPx) Register , Cht/Cht2 */
#define SMEM_CHT_CUR_RX_DESC_POINTER_REG(dev, rxQueue) \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->MG.SDMARegs.receiveSDMACurrentDescPointer[rxQueue]     : \
        (0x0000260C + ((rxQueue) * 0x10)))

/* Receive SDMA<n> Packet Count Register , Cht/Cht2 */
#define SMEM_CHT_PACKET_COUNT_REG(dev, rxQueue)  \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->MG.SDMARegs.receiveSDMAPktCount[rxQueue]   : \
        (0x00002820 + ((rxQueue) * 0x4)))

/* Receive SDMA<n> Byte Count Register , Cht/Cht2 */
#define SMEM_CHT_BYTE_COUNT_REG(dev, rxQueue)    \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->MG.SDMARegs.receiveSDMAByteCount[rxQueue]  : \
        (0x00002840 + ((rxQueue) * 0x4)))

/* Receive SDMA Resource Error Count Register */
    /* replaced by SMEM_CHT_RES_ERR_COUNT_AND_MODE_REG */
    /*4 queue in each register */
#define SMEM_CHT_RES_ERR_COUNT_REG(dev, rxQueue) \
    (SMEM_CHT_IS_SIP5_GET(dev)? SMAIN_NOT_VALID_CNS  : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->MG.SDMARegs.receiveSDMAResourceErrorCount[rxQueue/4]     : \
        (0x00002860 + 0x4*((rxQueue) / 0x4))))

/* Tx SDMA Packet Generator Config Queue, xCat3, Lion3, Bobcat2 */
#define SMEM_CHT_PACKET_GENERATOR_CONFIG_QUEUE_REG(dev, txQueue) \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->MG.SDMARegs.transmitSDMAPacketGeneratorConfigQueue[txQueue]     : \
        SMAIN_NOT_VALID_CNS)

/* Tx SDMA Packet Count Config Queue, xCat3, Lion3, Bobcat2 */
#define SMEM_CHT_PACKET_COUNT_CONFIG_QUEUE_REG(dev, txQueue) \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->MG.SDMARegs.transmitSDMAPacketCountConfigQueue[txQueue]     : \
        SMAIN_NOT_VALID_CNS)

/* Rx SDMA Resource Error Count and Mode Register, xCat3, Lion3, Bobcat2 */
#define SMEM_CHT_RES_ERR_COUNT_AND_MODE_REG(dev, rxQueue)                                    \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->MG.SDMARegs.receiveSDMAResourceErrorCountAndMode[rxQueue]     : \
        SMAIN_NOT_VALID_CNS)

/*  Multicast local enable configuration register , Cht/Cht2/Cht3 */
#define SMEM_CHT_MULTICAST_LCL_ENABLE_REG(dev) \
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer0.bridgeEgrConfig.mcLocalEnableConfig :    \
        SMEM_LION_TXQ_UNREG_MC_LOCAL_EN_REG(dev, 0) :                                       \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev) ? \
        SMEM_LION_TXQ_UNREG_MC_LOCAL_EN_REG(dev, 0) : (0x01800008))

/* Transmit queue extended control register , Cht/Cht2/Cht3 */
#define SMEM_CHT_TX_QUEUE_EXTEND_CTRL_REG(dev) \
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer0.transmitQueueGlobalConfig.transmitQueueExtCtrl :    \
        SMAIN_NOT_VALID_CNS :                                       \
      (0x01800004))

/* Transmit queue control register , Cht/Cht2/Cht3 */
#define SMEM_CHT_TX_QUEUE_CTRL_REG(dev) \
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer0.transmitQueueGlobalConfig.transmitQueueCtrl :    \
        SMAIN_NOT_VALID_CNS :                                       \
      (0x01800000))

/* Statistical & CPU-Triggered Egress Mirroring to Analyzer port , Cht/Cht2 */
#define SMEM_CHT1_TRUNK_DESIGNATED_REG(dev, entry)      \
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer0.trunksFilteringAndMcDistConfig.designatedTrunkPortEntryTable[entry] :    \
        SMAIN_NOT_VALID_CNS :                                       \
        (0x01A40008 + ((entry) * 0x10)))

/* egress unknown UC Filtering Register */
#define SMEM_CHT_EGRESS_UNKN_UC_FILTER_EN_REG(dev) \
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
      (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer0.bridgeEgrConfig.egrFilteringReg0 :   \
      SMEM_LION_TXQ_UNKN_UC_FILTER_EN_REG(dev, 0) : \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev) ? \
        SMEM_LION_TXQ_UNKN_UC_FILTER_EN_REG(dev, 0) : (0x01800010))

/* egress unRegistered MC Filtering Register */
#define SMEM_CHT_EGRESS_UNKN_MC_FILTER_EN_REG(dev)\
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
      (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer0.bridgeEgrConfig.egrFilteringReg1 :   \
      SMEM_LION_TXQ_UNREG_MC_FILTER_EN_REG(dev, 0) : \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev) ? \
        SMEM_LION_TXQ_UNREG_MC_FILTER_EN_REG(dev, 0) : (0x01800014))

/* egress unRegistered BC Filtering Register */
#define SMEM_CHT_EGRESS_UNKN_BC_FILTER_EN_REG(dev)  \
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
      (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer0.bridgeEgrConfig.egrFilteringReg2  :  \
      SMEM_LION_TXQ_UNREG_BC_FILTER_EN_REG(dev, 0) : \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev) ? \
        SMEM_LION_TXQ_UNREG_BC_FILTER_EN_REG(dev, 0) : (0x018001F4))

/* Transmit queue configuration register , Cht/Cht2/Cht3 */
#define SMEM_CHT_TXQ_CONFIG_REG(dev, port)  \
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
      (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer0.perPortTransmitQueueConfig.portTxqConfig[CPU_PORT_INDEX(dev, port)]  :  \
      SMAIN_NOT_VALID_CNS : \
        0x01800080 + (port) * 0x200)


/* Statistical and CPU-Triggered
    Egress Mirroring to Analyzer Port Config. register , Cht/Cht2/Cht3 */
#define SMEM_CHT_STAT_EGRESS_MIRROR_REG(dev,dpUnitInPipe)   \
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
      (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer0.statisticalAndCPUTrigEgrMirrToAnalyzerPort.statisticalAndCPUTrigEgrMirrToAnalyzerPortConfig  : \
      SMEM_LION_STAT_EGRESS_MIRROR_REG(dev,dpUnitInPipe) : \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev) ? \
        SMEM_LION_STAT_EGRESS_MIRROR_REG(dev,dpUnitInPipe) : (0x01800020))


/* Egress STC Table Table Entry Word0 , Cht/Cht2 */
#define SMEM_CHT_STC_TABLE0_REG(dev, port)                  \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev,egressStc,port)

/* Egress STC Table Table Entry Word1 , Cht/Cht2 */
#define SMEM_CHT_STC_TABLE1_REG(dev, port)                  \
    (SMEM_CHT_STC_TABLE0_REG(dev,port) + 4)

/* Egress STC Table Table Entry Word2 , Cht/Cht2 */
#define SMEM_CHT_STC_TABLE2_REG(dev, port)                  \
    (SMEM_CHT_STC_TABLE0_REG(dev,port) + 8)

/* Txq Mib Counters Set0 Configuration Register , Cht/Cht2/Cht3 */
#define SMEM_CHT_TXQ_MIB_COUNTERS_SET0_CONFIG_REG(dev) \
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
       (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer0.transmitQueueTxAndEgrMIBCntrs.txqMIBCntrsSetConfig[0] :  \
       SMEM_LION_TXQ_MIB_CNT_SET_CONF_REG(dev, 0) : \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev) ? \
        SMEM_LION_TXQ_MIB_CNT_SET_CONF_REG(dev, 0) : 0x01800140)

/* Txq Mib Counters Set1 Configuration Register , Cht/Cht2/Cht3 */
#define SMEM_CHT_TXQ_MIB_COUNTERS_SET1_CONFIG_REG(dev)\
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
       (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer0.transmitQueueTxAndEgrMIBCntrs.txqMIBCntrsSetConfig[1] :  \
       SMEM_LION_TXQ_MIB_CNT_SET_CONF_REG(dev, 1) : \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev) ? \
        SMEM_LION_TXQ_MIB_CNT_SET_CONF_REG(dev, 1): 0x01800160)

/* Set0 Outgoing Unicast Packet Counter , Cht/Cht2/Cht3 */
#define SMEM_CHT_SET0_UNICAST_PACKET_CNTR_REG(dev)\
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
      (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer0.transmitQueueTxAndEgrMIBCntrs.setOutgoingUcPktCntr[0] :  \
      SMEM_LION_TXQ_SET_OUT_UC_PCKT_CNT_REG(dev, 0) : \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev) ? \
        SMEM_LION_TXQ_SET_OUT_UC_PCKT_CNT_REG(dev, 0): 0x01B40144)

/* Set0 Outgoing Multicast Packet Counter , Cht/Cht2/Cht3 */
#define SMEM_CHT_SET0_MULTICAST_PACKET_CNTR_REG(dev) \
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
       (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer0.transmitQueueTxAndEgrMIBCntrs.setOutgoingMcPktCntr[0] :  \
       SMEM_LION_TXQ_SET_OUT_MC_PCKT_CNT_REG(dev, 0) : \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev) ? \
        SMEM_LION_TXQ_SET_OUT_MC_PCKT_CNT_REG(dev, 0): 0x01B40148)

/* Set0 Outgoing Broadcast Packet Counter , Cht/Cht2/Cht3 */
#define SMEM_CHT_SET0_BROADCAST_PACKET_CNTR_REG(dev) \
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer0.transmitQueueTxAndEgrMIBCntrs.setOutgoingBcPktCount[0] : \
        SMEM_LION_TXQ_SET_OUT_BC_PCKT_CNT_REG(dev, 0) : \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev) ? \
        SMEM_LION_TXQ_SET_OUT_BC_PCKT_CNT_REG(dev, 0): 0x01B4014C)

/* Set1 Outgoing Unicast Packet Counter , Cht/Cht2/Cht3 */
#define SMEM_CHT_SET1_UNICAST_PACKET_CNTR_REG(dev) \
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
       (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer0.transmitQueueTxAndEgrMIBCntrs.setOutgoingUcPktCntr[1] :  \
       SMEM_LION_TXQ_SET_OUT_UC_PCKT_CNT_REG(dev, 1) : \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev) ? \
        SMEM_LION_TXQ_SET_OUT_UC_PCKT_CNT_REG(dev, 1): 0x01B40164)

/* Set1 Outgoing Multicast Packet Counter , Cht/Cht2/Cht3 */
#define SMEM_CHT_SET1_MULTICAST_PACKET_CNTR_REG(dev) \
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
       (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer0.transmitQueueTxAndEgrMIBCntrs.setOutgoingMcPktCntr[1] :  \
       SMEM_LION_TXQ_SET_OUT_MC_PCKT_CNT_REG(dev, 1) : \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev) ? \
        SMEM_LION_TXQ_SET_OUT_MC_PCKT_CNT_REG(dev, 1): 0x01B40168)

/* Set1 Outgoing Broadcast Packet Counter , Cht/Cht2/Cht3 */
#define SMEM_CHT_SET1_BROADCAST_PACKET_CNTR_REG(dev) \
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
       (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer0.transmitQueueTxAndEgrMIBCntrs.setOutgoingBcPktCount[1] : \
       SMEM_LION_TXQ_SET_OUT_BC_PCKT_CNT_REG(dev, 1) : \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev) ? \
        SMEM_LION_TXQ_SET_OUT_BC_PCKT_CNT_REG(dev, 1): 0x01B4016C)

/* Set0 Bridge Egress Filtered Packet Counter Register , Cht/Cht2/Cht3 */
#define SMEM_CHT_SET0_BRIDGE_FILTERED_CNTR_REG(dev) \
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
       (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer0.transmitQueueTxAndEgrMIBCntrs.setBridgeEgrFilteredPktCount[0] :  \
       SMEM_LION_TXQ_SET_BRDG_EGR_FILTER_PCKT_CNT_REG(dev, 0) : \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev) ? \
        SMEM_LION_TXQ_SET_BRDG_EGR_FILTER_PCKT_CNT_REG(dev, 0): 0x01B40150)

/* Set1 Bridge Egress Filtered Packet Counter Register , Cht/Cht2/Cht3 */
#define SMEM_CHT_SET1_BRIDGE_FILTERED_CNTR_REG(dev) \
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
       (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer0.transmitQueueTxAndEgrMIBCntrs.setBridgeEgrFilteredPktCount[1] :  \
       SMEM_LION_TXQ_SET_BRDG_EGR_FILTER_PCKT_CNT_REG(dev, 1) : \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev) ? \
        SMEM_LION_TXQ_SET_BRDG_EGR_FILTER_PCKT_CNT_REG(dev, 1): 0x01B40170)

/* Set0 Control Packet Counter Register , Cht/Cht2/Cht3 */
#define SMEM_CHT_SET0_CONTROL_PACKET_CNTR_REG(dev) \
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
      (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer0.transmitQueueTxAndEgrMIBCntrs.setCtrlPktCntr[0] :    \
      SMEM_LION_TXQ_SET_CTRL_PCKT_CNT_REG(dev, 0) : \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev) ? \
        SMEM_LION_TXQ_SET_CTRL_PCKT_CNT_REG(dev, 0) : 0x01B40158)

/* Set1 Control Packet Counter Register , Cht/Cht2/Cht3 */
#define SMEM_CHT_SET1_CONTROL_PACKET_CNTR_REG(dev) \
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer0.transmitQueueTxAndEgrMIBCntrs.setCtrlPktCntr[1] :    \
        SMEM_LION_TXQ_SET_CTRL_PCKT_CNT_REG(dev, 1) : \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev) ? \
        SMEM_LION_TXQ_SET_CTRL_PCKT_CNT_REG(dev, 1) : 0x01B40178)

/* Set0 Egress Forward Restrictions Packet Counter Register,  */
#define SMEM_CHT_SET0_EGR_FWD_RESTRICTIONS_DROPPED_PACKET_CNTR_REG(dev) \
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
      (SMEM_CHT_MAC_REG_DB_GET(dev))->\
        egrAndTxqVer0.transmitQueueTxAndEgrMIBCntrs.setEgrForwardingRestrictionDroppedPktsCntr[0] :  \
      SMEM_LION_TXQ_SET_EGR_FWD_RESTRICTIONS_DROPPED_PCKT_CNT_REG(dev, 0) : \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev) ? \
        SMEM_LION_TXQ_SET_EGR_FWD_RESTRICTIONS_DROPPED_PCKT_CNT_REG(dev, 0) : 0x01B4015C)

/* Set1 Egress Forward Restrictions Packet Counter Register,  */
#define SMEM_CHT_SET1_EGR_FWD_RESTRICTIONS_DROPPED_PACKET_CNTR_REG(dev) \
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
      (SMEM_CHT_MAC_REG_DB_GET(dev))->\
        egrAndTxqVer0.transmitQueueTxAndEgrMIBCntrs.setEgrForwardingRestrictionDroppedPktsCntr[1] :  \
      SMEM_LION_TXQ_SET_EGR_FWD_RESTRICTIONS_DROPPED_PCKT_CNT_REG(dev, 0) : \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev) ? \
        SMEM_LION_TXQ_SET_EGR_FWD_RESTRICTIONS_DROPPED_PCKT_CNT_REG(dev, 0) : 0x01B4017C)

/* Cht/Cht2/Lion B0 */
#define SMEM_CHT_TX_QUEUE_INTERRUPT_CAUSE_REG(dev,dqUnitInPipe)\
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer0.transmitQueueInterrupt.transmitQueueInterruptSummaryCause :  \
        SMEM_LION_TXQ_DQ_INTR_SUM_CAUSE_REG(dev,dqUnitInPipe) : \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev) ? \
        SMEM_LION_TXQ_DQ_INTR_SUM_CAUSE_REG(dev,dqUnitInPipe) : (0x01800100))

/* Cht/Cht2/Lion B0 */
#define SMEM_CHT_TX_QUEUE_INTERRUPT_MASK_REG(dev,dqUnitInPipe)\
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer0.transmitQueueInterrupt.transmitQueueInterruptSummaryMask :   \
        SMEM_LION_TXQ_DQ_INTR_SUM_MASK_REG(dev,dqUnitInPipe) : \
    SKERNEL_IS_LION_REVISON_B0_DEV(dev) ? \
        SMEM_LION_TXQ_DQ_INTR_SUM_MASK_REG(dev,dqUnitInPipe) : (0x01800104))

/* Interrupts bits */

/* start register SMEM_CHT_BRIDGE_INT_CAUSE_REG */
    /* Security Breach Status registers were updated , Cht/Cht2 */
    #define SMEM_CHT_UPD_SEC_BRICH_INT                     (1 << 28)
/* end register SMEM_CHT_BRIDGE_INT_CAUSE_REG */


/* start register SMEM_CHT_MISC_INTR_CAUSE_REG */

    /* MAC update messages Miscellaneous to CPU has exceeded its configured
      threshold , Cht/Cht2 */
    #define SMEM_CHT_MISCELLANEOUS_AU_FULL_INT             (1 << 9)
    /* Message to CPU is ready in the Miscellaneous , Cht/Cht2 */
    #define SMEM_CHT_MISCELLANEOUS_AU_MSG_PENDING_INT      (1 << 10)


    /* FU upload messages Miscellaneous to CPU has exceeded its configured
      threshold , Cht2 */
    #define SMEM_CHT2_MISCELLANEOUS_FU_FULL_INT                          (1 << 11)

    /* FU upload Message to CPU is ready in the Miscellaneous , Cht2 */
    #define SMEM_CHT2_MISCELLANEOUS_FU_MSG_PENDING_INT                   (1 << 12)

/* end register SMEM_CHT_MISC_INTR_CAUSE_REG */


/* start register SMEM_CHT_MAC_TBL_INTR_CAUSE_REG */

/* number of hops expired (FDB interrupt),Cht/Cht2*/
    #define SMEM_CHT_NUM_OF_HOPS_EXP_INT                   (1 << 1)

    /* A new source MAC Address received was retained (FDB interrupt) , Cht/Cht2 */
    #define SMEM_CHT_NA_LEARN_INT                          (1 << 2)

    /* A new source MAC Address received was not retained (FDB interrupt),Cht/Cht2*/
    #define SMEM_CHT_NA_NOT_LEARN_INT                      (1 << 3)

    /* The aging cycle that was initiated
        by a CPU trigger has ended (FDB interrupt) , Cht/Cht2 */
    #define SMEM_CHT_AGE_VIA_TRIGGER_END_INT               (1 << 4)
    /* in lion3 */
    #define SMEM_LION3_AGE_VIA_TRIGGER_END_INT               (1 << 18)

    /* Processing of an AU Message received
        by the device is completed (FDB interrupt) , Cht/Cht2 */
    #define SMEM_CHT_AU_PROC_COMPLETED_INT                 (1 << 9)
    /* in lion3 */
    #define SMEM_LION3_AU_PROC_COMPLETED_INT                 (1 << 10)

    /* Message to CPU is ready in the FIFO (FDB interrupt) , Cht/Cht2 */
    #define SMEM_CHT_AU_MSG_TO_CPU_READY_INT               (1 << 10)
    /* in lion3 */
    #define SMEM_LION3_AU_MSG_TO_CPU_READY_INT               (1 << 11)

    /* A new address from the CPU was dropped (FDB interrupt), Cht/Cht2 */
    #define SMEM_CHT_NA_DROPPED_INT                        (1 << 14)

    /* MAC update messages FIFO to CPU has exceeded
            its configured threshold (FDB interrupt) , Cht/Cht2 */
    #define SMEM_CHT_AU_FIFO_FULL_INT                      (1 << 16)
    /* in lion3 */
    #define SMEM_LION3_AU_FIFO_FULL_INT                      (1 << 12)



    /* SIP5 :  Bank Learn Counters (BLC) overflow.  Used for overflow or underflow on bank learn counters */
    #define SMEM_LION3_BLC_OVERFLOW_INT               (1 << 20)

/* end register SMEM_CHT_MAC_TBL_INTR_CAUSE_REG */


/* start register SMEM_CHT_TX_SDMA_INT_CAUSE_REG */

    /* TxBuffer - a bit per transmit SDMA */
    #define SMEM_CHT_TX_BUFFER_INT(qid)                    (1 << (1 + (qid)))

    /* TxError - a bit per transmit SDMA */
    #define SMEM_CHT_TX_ERROR_INT(qid)                     (1 << (9 + (qid)))

/* end register SMEM_CHT_TX_SDMA_INT_CAUSE_REG */


/* start register SMEM_CHT_INGRESS_STC_INT_CAUSE_REG */
    /* Port<n>IngressSample-Loaded Interrupt */
    #define SMEM_CHT_INGR_SAMPLE_LOAD_INT(port)            (1 << (1 + (port)))

/* end register SMEM_CHT_INGRESS_STC_INT_CAUSE_REG */


/***/
/* start -- BMP of interrupts in the 'global interrupt register' (0x30,0x34) */
/***/

/* XG Ports cause interrupt sum bmp */
#define  SMEM_CHT_XG_PORT_CAUSE_INT_SUM_BMP(dev,port)      \
    (SKERNEL_IS_CHEETAH3_DEV(dev) ?                        \
     (1 << (dev)->globalInterruptCauseRegister.global1): /* ch3*/  \
     (1 << port))/*ch1,2*/

/* CPU Ports cause interrupt sum bmp */
#define  SMEM_CHT_CPU_PORT_CAUSE_INT_SUM_BMP(dev)          \
        (1 << (dev)->globalInterruptCauseRegister.cpuPort)

/* GE Ports cause interrupt sum bmp */
#define  SMEM_CHT_GE_PORT_CAUSE_INT_SUM_BMP(dev)           \
        (1 << (dev)->globalInterruptCauseRegister.ports)

/* Ports cause interrupt sum bmp */
#define  SMEM_CHT_PORT_CAUSE_INT_SUM_BMP(dev,port)      \
        ((dev)->globalInterruptCauseRegister.supportMultiPorts ?                                                                                                          \
         ((port) <= (dev)->globalInterruptCauseRegister.multiPorts[0].endPort ? (1 << (dev)->globalInterruptCauseRegister.multiPorts[0].globalBit) :\
          (port) <= (dev)->globalInterruptCauseRegister.multiPorts[1].endPort ? (1 << (dev)->globalInterruptCauseRegister.multiPorts[1].globalBit) :\
          (port) <= (dev)->globalInterruptCauseRegister.multiPorts[2].endPort ? (1 << (dev)->globalInterruptCauseRegister.multiPorts[2].globalBit) :\
            (1 << 31)) :                                                                                                                            \
        (IS_CHT_CPU_PORT(port) ?                        \
             /* for cpu port */                         \
             SMEM_CHT_CPU_PORT_CAUSE_INT_SUM_BMP(dev):  \
         IS_CHT_REGULAR_PORT(dev,port) ?                \
             /* for GE port (but not stacking GE !!)*/  \
             SMEM_CHT_GE_PORT_CAUSE_INT_SUM_BMP(dev):   \
             /* for XG port */                          \
             SMEM_CHT_XG_PORT_CAUSE_INT_SUM_BMP(dev,port)))

/* L2 Bridge Interrupt Cause register SUM bit (Global interrupt) */
#define SMEM_CHT_L2I_SUM_INT(dev)                       \
         (1 << (dev)->globalInterruptCauseRegister.l2i)

/* Miscellaneous interrupt bit in the global interrupt register */
#define SMEM_CHT_MISCELLANEOUS_SUM_INT(dev)             \
         (1 << (dev)->globalInterruptCauseRegister.misc)

/* FDB Interrupt Cause register SUM bit (Global interrupt)  */
#define SMEM_CHT_FDB_SUM_INT(dev)                       \
         (1 << (dev)->globalInterruptCauseRegister.fdb)

/* Transmit SDMA Interrupt Cause
        Register Summary bit(global interrupt) */
#define SMEM_CHT_TX_SDMA_SUM_INT(dev)                   \
         (1 << (dev)->globalInterruptCauseRegister.txSdma)

/* Pre-Egress Engine Interrupt Cause
            register SUM bit (global interrupt) */
#define SMEM_CHT_PRE_EGR_ENGINE_INT(dev)             \
         (1 << (dev)->globalInterruptCauseRegister.eq)

/* CNC Interrupt Cause
            register SUM bit (global interrupt) */
#define SMEM_CHT_CNC_ENGINE_INT(dev,_cncUnitIndex)                 \
         (1 << (dev)->globalInterruptCauseRegister.cnc[_cncUnitIndex])

/* TXQ Interrupt Cause
            register SUM bit (global interrupt) */
#define SMEM_CHT_TXQ_ENGINE_INT(dev)                 \
        (((dev)->txqRevision) ? \
         (1 << (dev)->globalInterruptCauseRegister.txqDq) : \
         (1 << (dev)->globalInterruptCauseRegister.txq))


/* Receive SDMA Interrupt Cause Register Summery Bit,
             (global interrupt) , Cht/Cht2*/
#define SMEM_CHT_RX_SDMA_SUM_INT(dev)                \
         (1 << (dev)->globalInterruptCauseRegister.rxSdma)

/* Summary bit of Egress STC Interrupt Cause Register,
             (global interrupt) */
#define SMEM_CHT_EGR_STC_SUM_INT(dev)                \
        (((dev)->txqRevision) ? \
         (1 << 3) : /*Lion B0*/                        \
         (1 << 5))

/* Summary bit of Ingress STC Interrupt Cause Register,
             (global interrupt) */
#define SMEM_CHT_ING_STC_SUM_INT(dev)                \
         (1 << 1)


/***/
/* end -- BMP of interrupts in the 'global interrupt register' (0x30,0x34) */
/***/


/* get CPU bit index in the Cascading and Header Alteration */
#define SMEM_CHT_GET_CPU_PORT_BIT_INDEX_IN_CSCD_STATUS(dev)              \
        ((SKERNEL_IS_LION_REVISON_B0_DEV(dev) && dev->portGroupSharedDevObjPtr) ? \
            15 :                     \
        (SKERNEL_IS_CHEETAH1_ONLY_DEV(dev)==0) ?                    \
            31 :                     \
            27)

/* get CPU port for Cascading and Header Alteration  */
#define SMEM_CHT_GET_CPU_PORT_CSCD_STATUS(dev, regVal)              \
            SMEM_U32_GET_FIELD(regVal, SMEM_CHT_GET_CPU_PORT_BIT_INDEX_IN_CSCD_STATUS(dev), 1)

/* accessing to ch1,2,3,xcat device map table */
#define SMEM_CHT_DEVICE_MAP_TABLE_ENTRY(dev, index)\
        SMEM_TABLE_ENTRY_2_PARAMS_INDEX_GET_MAC(dev,deviceMapTable,index,0)

/* accessing to Lion device map table ,
   the accessing is with 2 parameters:
    trgDev - target device
    trgPort - target port

    the macro will convert trgPort to use only 3 MSB
*/
#define SMEM_CHT_DEVICE_MAP_TABLE_WITH_DST_PORT_TBL_MEM(dev, trgDev, trgPort)\
        SMEM_TABLE_ENTRY_2_PARAMS_INDEX_GET_MAC(dev,deviceMapTable,trgDev,(((trgPort)>>3) & 0x7))

#define SMEM_CHT_NON_TRUNK_MEMBERS_TBL_MEM(dev, trunk) \
        SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, nonTrunkMembers, trunk)

#define SMEM_CHT_SST_EGRESS_FILTERING_TBL_MEM(dev, entry) \
        SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev, sst, entry)

#define SMEM_CHT_FROM_CPU_FILTER_CONF_REG(dev) \
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer0.egrForwardingRestrictions.fromCPUPktsFilterConfig    : \
        SMEM_LION_TXQ_FROM_CPU_FWRD_RESTRICT_REG(dev) : \
    (SKERNEL_IS_LION_REVISON_B0_DEV(dev) ? \
            SMEM_LION_TXQ_FROM_CPU_FWRD_RESTRICT_REG(dev) : SMEM_CHT2_FROM_CPU_FILTER_CONF_REG(dev)))

#define SMEM_CHT_BRIDGE_FILTER_CONF_REG(dev) \
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer0.egrForwardingRestrictions.bridgedPktsFilterConfig    : \
        SMEM_LION_TXQ_BRDG_FWRD_RESTRCIT_REG(dev) : \
    (SKERNEL_IS_LION_REVISON_B0_DEV(dev) ? \
            SMEM_LION_TXQ_BRDG_FWRD_RESTRCIT_REG(dev) : SMEM_CHT2_BRIDGE_FILTER_CONF_REG(dev)))

#define SMEM_CHT_ROUTE_FILTER_CONF_REG(dev) \
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer0.egrForwardingRestrictions.routedPktsFilterConfig     : \
        SMEM_LION_TXQ_ROUT_FWRD_RESTRCIT_REG(dev) : \
    (SKERNEL_IS_LION_REVISON_B0_DEV(dev) ? \
            SMEM_LION_TXQ_ROUT_FWRD_RESTRCIT_REG(dev) : SMEM_CHT2_ROUTE_FILTER_CONF_REG(dev)))

#define SMEM_CHT_TRUNK_DESIGNATED_REG(dev, entry) \
            SMEM_CHT1_TRUNK_DESIGNATED_REG(dev, entry)

/* xcat3 ‘old’ designated trunk table */
#define SMEM_CHT_TRUNK_DESIGNATED_OLD_REG(dev, entry) \
    ((dev)->supportRegistersDb ?                                    \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer0.trunksFilteringAndMcDistConfig.old_designatedTrunkPortEntryTable[entry] :    \
        SMAIN_NOT_VALID_CNS)


#define SMEM_CHT_PROT_VLAN_QOS_TBL_MEM(dev, protocol, port) \
    SMEM_TABLE_ENTRY_2_PARAMS_INDEX_GET_MAC(dev, portProtocolVidQoSConf, port, protocol)

#define SMEM_CHT_PCL_GLOBAL_REG(dev) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->PCL.ingrPolicyGlobalConfig : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->IPCL.ingrPolicyGlobalConfig   : \
    0x0B800000))

/* PCL configuration table address */
#define SMEM_CHT_PCL_CONFIG_TBL_MEM(dev, entry_index, table_index)             \
    (((table_index == SNET_IPCL_LOOKUP_0_0_E) || ((dev)->pclSupport.ipclSupportSeparatedIConfigTables)) ? \
        SMEM_TABLE_ENTRY_2_PARAMS_INDEX_GET_MAC(dev, pclConfig, entry_index, table_index) :               \
        SMEM_TABLE_ENTRY_2_PARAMS_INDEX_GET_MAC(dev, pclConfig, entry_index, (table_index-1) ))

/* TCP port range comparator register - ingress policy */
#define SMEM_CHT_PCL_TCP_PORT_RANGE_COMPARE_REG(dev)                \
        (SKERNEL_IS_XCAT_REVISON_A1_DEV(dev) ?                      \
            SMEM_XCAT_TTI_PCL_TCP_PORT_RANGE_COMPARE_REG(dev) :     \
            ((SKERNEL_IS_CHEETAH1_ONLY_DEV(dev)==0) ?                \
                SMEM_CHT2_PCL_TCP_PORT_RANGE_COMPARE_REG(dev) : 0))

/* UDP port range comparator register - ingress policy */
#define SMEM_CHT_PCL_UDP_PORT_RANGE_COMPARE_REG(dev)\
        (SKERNEL_IS_XCAT_REVISON_A1_DEV(dev) ?                      \
            SMEM_XCAT_TTI_PCL_UDP_PORT_RANGE_COMPARE_REG(dev) :     \
            ((SKERNEL_IS_CHEETAH1_ONLY_DEV(dev)==0) ?                \
                SMEM_CHT2_PCL_UDP_PORT_RANGE_COMPARE_REG(dev) : 0))

/* Bridge Global Configuration Register2 - Cht2/Cht3 */
#define SMEM_CHT_BRIDGE_GLOBAL_CONF2_REG(dev)   \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMAIN_NOT_VALID_CNS : \
     ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->L2I.bridgeEngineConfig.bridgeGlobalConfig[2]    : \
        (SKERNEL_IS_CHEETAH1_ONLY_DEV(dev)==0) ?    \
            SMEM_CHT2_BRIDGE_GLOBAL_CONF2_REG(dev) : 0))

/* Policy action table address */
#define SMEM_CHT_PCL_ACTION_TBL_MEM(dev, entry_index)\
    ((SMEM_CHT_IS_SIP5_GET(dev)) ?                                  \
        SMEM_LION3_TCAM_ACTION_TBL_MEM(dev, entry_index) :     \
        SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev,pclAction,entry_index))  \

/* Policy tcam table */
#define SMEM_CHT_PCL_TCAM_TBL_MEM(dev, entry, word)\
        SMEM_TABLE_ENTRY_2_PARAMS_INDEX_GET_MAC(dev,pclTcam,entry, word)

/* Policy tcam control table */
#define SMEM_CHT_PCL_TCAM_CTRL_TBL_MEM(dev, entry, word)\
        (SMEM_CHT_PCL_TCAM_TBL_MEM(dev, entry, word) + 0x4)

/* Policy tcam data mask table */
#define SMEM_CHT_PCL_TCAM_MASK_TBL_MEM(dev, entry, word)\
        SMEM_TABLE_ENTRY_2_PARAMS_INDEX_GET_MAC(dev,pclTcamMask,entry, word)

/* Policy tcam ctrl mask table */
#define SMEM_CHT_PCL_TCAM_CTRL_MASK_TBL_MEM(dev, entry, word)\
        (SMEM_CHT_PCL_TCAM_MASK_TBL_MEM(dev, entry, word) + 0x4)

/* Policy action entry base address */
#define SMEM_CHT_PCL_POLICY_ACTION_TBL_MEM(dev)\
        (SKERNEL_IS_CHEETAH1_ONLY_DEV(dev)==0) ?    \
            0x0B8C0000 : 0

/* FDB Update Message Queue Configuration Registers */
#define SMEM_CHT_FU_QUE_BASE_ADDR_REG(dev)\
     ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->MG.addrUpdateQueueConfig.FUAddrUpdateQueueCtrl    : \
        (SKERNEL_IS_CHEETAH1_ONLY_DEV(dev)==0) ?    \
            SMEM_CHT2_FU_Q_BASE_ADDR_REG(dev) : 0)


/* Unicast SrcId Egress Filtering Configuration Register */
#define SMEM_CHT_SST_EGRESS_CONFIG_REG(dev)                 \
    ((dev)->supportRegistersDb ?                                    \
      (dev)->txqRevision == 0 ?                                     \
        (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer0.sourceIDEgrFiltering.ucSrcIDEgrFilteringConfig   : \
        SMEM_LION_TXQ_UC_SRC_ID_FILTER_EN_REG(dev, 0) : \
    (SKERNEL_IS_LION_REVISON_B0_DEV(dev)) ?                \
        SMEM_LION_TXQ_UC_SRC_ID_FILTER_EN_REG(dev, 0) :     \
     (SKERNEL_IS_CHEETAH1_ONLY_DEV(dev)==0) ?               \
        SMEM_CHT2_SST_EGRESS_CONFIG_REG(dev) :                   \
        0)

/* Pre-Egress Engine Global Configuration Registers */
#define SMEM_CHT_PRE_EGR_GLB_CONF_REG(dev) \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->EQ.preEgrEngineGlobalConfig.preEgrEngineGlobalConfig : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->EQ.preEgrEngineGlobalConfig.preEgrEngineGlobalConfig   : \
    (SKERNEL_IS_CHEETAH1_ONLY_DEV(dev)==0) ?    \
        0x0B050000 : 0))

#define SMEM_CHT_TCP_UDP_DST_PORT_RANGE_CPU_CODE_TBL_MEM(dev, entry,word) \
    SMEM_TABLE_ENTRY_2_PARAMS_INDEX_GET_MAC(dev,tcpUdpDstPortRangeCpuCode,entry, word)

/* TCP/UDP Destination Port Ranges CPU Code Table Word 0 */
#define SMEM_CHT_TCP_UDP_DST_PORT_RANGE_CPU_CODE_WORD0_TBL_MEM(dev, entry) \
    SMEM_CHT_TCP_UDP_DST_PORT_RANGE_CPU_CODE_TBL_MEM(dev, entry,0)

/* TCP/UDP Destination Port Ranges CPU Code Table Word 1 */
#define SMEM_CHT_TCP_UDP_DST_PORT_RANGE_CPU_CODE_WORD1_TBL_MEM(dev, entry) \
    SMEM_CHT_TCP_UDP_DST_PORT_RANGE_CPU_CODE_TBL_MEM(dev, entry,1)


/* IP Protocol CPU Code Valid Configuration Register */
#define SMEM_CHT_IP_PROT_CPU_CODE_VALID_CONF_REG(dev) \
    (SKERNEL_IS_CHEETAH1_ONLY_DEV(dev)==0) ?    \
        SMEM_CHT2_IP_PROT_CPU_CODE_VALID_CONF_REG(dev) : 0

/* IP Protocol CPU Code Entry */
#define SMEM_CHT_IP_PROT_CPU_CODE_ENTRY_TBL_MEM(dev, prot) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev,ipProtCpuCode,prot)

/* Port<n> Interrupt Cause Register Mask */
#define     SMEM_CHT_GOPINT_MSK_CNS                         0xfff000ff
/* MAC Counters address mask */
#define     SMEM_CHT_COUNT_MSK_CNS                          0xfffff000

/* not applicable to CH1 devices */
#define     SMEM_CHT_TRI_SPEED_PORTS_INTERRUPT_SUMMARY_REG(dev)     \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->LMS.LMS1.portsInterruptSummary   : \
        0x05004010)

/* not applicable to CH1 devices */
#define     SMEM_CHT_TRI_SPEED_PORTS_INTERRUPT_SUMMARY_MASK_REG(dev)     \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->LMS.LMS0LMS0Group0.portsInterruptSummaryMask   : \
        0x04805114)

    /* vendor ID of Marvell's products */
#define SMEM_CHT_MARVELL_VENDOR_ID_CNS      0x11AB
/* device Id register (non PCI/PEX register) */
#define SMEM_CHT_DEVICE_ID_REG(dev)     \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->MG.globalConfig.deviceID   : \
        0x0000004c)
/* vendor Id register (non PCI/PEX register) */
#define SMEM_CHT_VENDOR_ID_REG(dev)     \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->MG.globalConfig.vendorID   : \
        0x00000050)

/* TO ANALYZER VLAN Tag Add Enable Configuration Register */
#define SMEM_CHT_TO_ANALYZER_VLAN_TAG_ADD_EN_REG(dev) \
    (SKERNEL_IS_XCAT_REVISON_A1_DEV(dev)) ?  \
        SMEM_XCAT_TO_ANALYZER_VLAN_TAG_ADD_EN_REG(dev) : (0x07800404)

/* Ingress Analyzer VLAN Tag Configuration */
#define SMEM_CHT_HA_INGR_ANALYZER_VLAN_TAG_CONF_REG(dev) \
     (SKERNEL_IS_XCAT_REVISON_A1_DEV(dev)) ?  \
        SMEM_XCAT_HA_INGR_ANALYZER_VLAN_TAG_CONF_REG(dev) : (0x07800408)

/* Egress Analyzer VLAN Tag Configuration */
#define SMEM_CHT_HA_EGR_ANALYZER_VLAN_TAG_CONF_REG(dev) \
    (SKERNEL_IS_XCAT_REVISON_A1_DEV(dev)) ?  \
        SMEM_XCAT_HA_EGR_ANALYZER_VLAN_TAG_CONF_REG(dev) : (0x0780040C)

/* Source Address Middle Register */
#define SMEM_CHT_SRC_ADDRESS_MID_REG(dev)               \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->MG.globalConfig.vendorID   : \
        (0x04004024))

/* Ingress Rate limit Configuration Register0 */
#define SMEM_CHT_INGR_RATE_LIMIT_CONF0_REG(dev)               \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->L2I.ingrPortsRateLimitConfig.ingrRateLimitConfig0 : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->L2I.ingrPortsRateLimitConfig.ingrRateLimitConfig0 : \
        (0x02040140)))

/* Ingress Rate limit Configuration Register1 */
#define SMEM_CHT_INGR_RATE_LIMIT_CONF1_REG(dev)               \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->L2I.ingrPortsRateLimitConfig.ingrRateLimitConfig1 : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->L2I.ingrPortsRateLimitConfig.ingrRateLimitConfig1 : \
        (0x02040144)))

/* Port<n> Rate Limit Counter (0<=n<27)
    Offset: Port0:0x02000400, Port1:0x02001400... Port26: 0x0201A400 (27 Registers in steps of 0x1000) */
#define SMEM_CHT_INGR_PORT_RATE_LIMIT_CNT_REG(dev, port) \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->L2I.ingrPortsRateLimitConfig.portRateLimitCntr[port]  : \
        (0x02000400 + (0x1000 * (port))))

/* Ingress Rate limit Drop Counter 0 */
#define SMEM_CHT_INGR_RATE_LIMIT_DROP_CNT_0_REG(dev)               \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->L2I.ingrPortsRateLimitConfig.ingrRateLimitDropCntr310 : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->L2I.ingrPortsRateLimitConfig.ingrRateLimitDropCntr310 : \
        (0x02040148)))

/* Ingress Rate limit Drop Counter 1 */
#define SMEM_CHT_INGR_RATE_LIMIT_DROP_CNT_1_REG(dev)               \
    (SMEM_CHT_IS_SIP5_GET(dev)  ? SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->L2I.ingrPortsRateLimitConfig.ingrRateLimitDropCntr3932 : \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->L2I.ingrPortsRateLimitConfig.ingrRateLimitDropCntr3932 : \
        (0x0204014c)))


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __sregcheetahh */


