/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* sregSalsa.h
*
* DESCRIPTION:
*       Defines API for Salsa memory registers access.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 8 $
*
*******************************************************************************/
#ifndef __sregsalsah
#define __sregsalsah

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Group number from 0...3, ports from 0...24 */
#define PORT_TO_GROUP(port)   (port / 0x6)
/* Port offset from 0...5, ports from 0...24 */
#define PORT_OFFSET(port)   (port % 0x6)
/* Fields offset */
#define UP2TC_BIT(priority)                     (2 * priority)

#define INTR_CAUSE_LNK_STAT_CHANGED_BIT(port)   (1 + PORT_OFFSET(port))

#define GLB_INTR_CAUSE_GOP_SUM0(port)           (7 + 2 * (PORT_TO_GROUP(port)))

#define PORT_COUNT_UPDATE(port)                 (8 + (PORT_OFFSET(port)))

#define CASCADE_PORT(device)                    (5 * (device % 4))

#define RANGE_DACMD(range)                      (4 * range)
#define RANGE_DAMIRROR(range)                   (2 * range)

#define RANGE_SACMD(range)                      ((4 * range) + 1)
#define RANGE_SAMIRROR(range)                   ((2 * range) + 1)

#define VID_INTERNAL_VLAN_ENTRY_PTR(vid)        (vid % 2) ? 9 : 0
#define VID_VALID(vid)                          (vid % 2) ? 17 : 8

#define SPAN_STATE_TABLE_PORT(stgId, port, address) \
    snetSalsaSpanStatePortOffset(stgId, port, address);

#define INTERNAL_VTABLE_PORT_OFFSET(__entry_idx, port, address)\
    snetSalsaInternVtablePortOffset(__entry_idx, port, address);

#define PORT_FILTER_UNK(port)                   (8 + port)

/* REGISTERS ADDRESSES */

/* Global Control Register */
#define GLOBAL_CONTROL_REG                      0x00000000

/* Global Interrupt Mask Register */
#define GLOBAL_INT_MASK_REG                     0x00000034

/* Global Interrupt Cause Register */
#define GLOBAL_INT_CAUSE_REG                    0x00000030

/* Statistic Sniffing and Monitoring
Traffic Class Configuration Register */
#define STATISTIC_SNIF_MONITOR_TC_REG           0x06000064

/* Ingress Mirroring Monitor and
Mirroring to CPU Configuration Register */
#define INGR_MIRR_MONITOR_REG                   0x06000060

/* Rx Time-based Sampling Register (Salsa 2) */
#define RX_TIME_BASED_SAMPLE_REG                0x06000070

/* Protocols Encapsulation 0 Register */
#define PROTOCOL_ENCAP0_REG                     0x0204005C

/* Protocols 0 Register */
#define PROTOCOL0_REG                           0x0204003C
/* Protocols 1 Register */
#define PROTOCOL1_REG                           0x02040040

/* UP2TC Register 0 Register */
#define UP2TC_REG0                              0x02040004

/* UP2TC Register 1 Register */
#define UP2TC_REG1                              0x02040008

/* Transmit Sniffer Register */
#define TRANSMIT_SNIF_REG                       0x01800008

/* MAC Range0 Filtering0 Register */
#define MAC_RNG0_FLTR0_REG                      0x02040078
/* MAC Range0 Filtering1 Register */
#define MAC_RNG0_FLTR1_REG                      0x0204007C
/* MAC Range0 Filtering2 Register */
#define MAC_RNG0_FLTR2_REG                      0x02040080


/* MAC Range1 Filtering0 Register */
#define MAC_RNG1_FLTR0_REG                      0x02040084
/* MAC Range1 Filtering1 Register */
#define MAC_RNG1_FLTR1_REG                      0x02040088
/* MAC Range1 Filtering2 Register */
#define MAC_RNG1_FLTR2_REG                      0x0204008C


/* MAC Range2 Filtering0 Register */
#define MAC_RNG2_FLTR0_REG                      0x02040090
/* MAC Range2 Filtering1 Register */
#define MAC_RNG2_FLTR1_REG                      0x02040094
/* MAC Range2 Filtering2 Register */
#define MAC_RNG2_FLTR2_REG                      0x02040098

/* MAC Range3 Filtering0 Register */
#define MAC_RNG3_FLTR0_REG                      0x0204009C
/* MAC Range3 Filtering1 Register */
#define MAC_RNG3_FLTR1_REG                      0x020400A0
/* MAC Range3 Filtering2 Register */
#define MAC_RNG3_FLTR2_REG                      0x020400A4

/* MAC Range Filter Command Register */
#define MAC_RNG_FLTR_CMD_REG                    0x02040074
/* MAC Range Mirror Command Register */
#define MAC_RNG_MIRR_CMD_REG                    0x02040034
/* MAC Table Control Register */
#define MAC_TABLE_CTRL_REG                      0x06000000
/* MAC Table ActionX Register */
#define MAC_TABLE_ACTION_0_REG                  0x06000004
#define MAC_TABLE_ACTION_1_REG                  0x06000008
#define MAC_TABLE_ACTION_2_REG                  0x06000020

/* MAC table entry<n> word0 */
#define MAC_TAB_ENTRY_WORD0_REG                 0x06100000
/* MAC table entry<n> word1 */
#define MAC_TAB_ENTRY_WORD1_REG                 0x06100004

/* Egress Bridging Register */
#define EGRESS_BRIDGING_REG                     0x0180000C

/* Transmit Queue Control Register */
#define TRANS_QUEUE_CONTROL_REG                 0x01800004

/* FIFO to CPU Control Register */
#define FIFO_TO_CPU_CONTROL_REG                 0x06000038

/* MAC Table Interrupt Cause Mask */
#define MAC_TAB_INTR_CAUSE_MASK_REG             0x0600001C
/* MAC Table Interrupt Cause Register */
#define MAC_TAB_INTR_CAUSE_REG                  0x06000018

/* CPU Port Control Register */
#define CPU_PORT_CONTROL_REG                    0x000000A0

/* CPU Port Threshold and MIB Count Enable Registers */
#define CPU_PORT_THLD_MIB_CNT_REG               0x000000B8

/* CPU Port GoodFramesReceived Counter */
#define CPU_PORT_GOOD_FRM_CNT_REG               0x00000070

/* CPU Port GoodOctetsReceived Counter */
#define CPU_PORT_GOOD_OCT_CNT_REG               0x00000078

/* Traffic Class to VPT Register */
#define TC_TO_VPT_REG                           0x0204002C

/* ForceL3Cos Register */
#define FORCE_L3_COS_REG                        0x0204000C

/* Layer 2 Ingress Control Register */
#define L2_INGRESS_CNT_REG                      0x02040000


/*Span State Group<n> Entry Word0 */
#define SPAN_STATE_TABLE(entry, address) \
    *address = 0x0A006000 + (0x8 * entry)
/* Port<n> Transmit Configuration Register */
#define TRANSMIT_CONF_REG(port, address) \
    *address = 0x01800000 + (0x1000 * port);
/* VLAN Translation Table (VLT) */
#define VLAN_TRANS_TABLE_REG(entry, address) \
    *address = 0x0A000000 + (0x4 * entry);
/* Internal VID<n> Entry word0 */
#define INTERNAL_VLAN_TABLE(_entry_idx, address) \
    *address = 0x0A002000 + (0x10 * _entry_idx);
/* Multicast Groups Table */
#define MCAST_GROUPS_TABLE(entry, address) \
    *address = 0x0A004000 + (0x4 * entry);

/* MAC Status Register */
#define MAC_STATUS_REG(port, address) \
    if (PORT_TO_GROUP(port) == 0) \
        *address = 0x04000004 + (0x100 * PORT_OFFSET(port)); \
    else if (PORT_TO_GROUP(port) == 1) \
        *address = 0x04800004 + (0x100 * PORT_OFFSET(port)); \
    else if (PORT_TO_GROUP(port) == 2) \
        *address = 0x05000004 + (0x100 * PORT_OFFSET(port)); \
    else \
        *address = 0x05800004 + (0x100 * PORT_OFFSET(port));

/* GOP<n> Interrupt Mask Register */
#define GOP_INTR_MASK_REG(port, address) \
    if (PORT_TO_GROUP(port) == 0) \
        *address = 0x04004004; \
    else if (PORT_TO_GROUP(port) == 1) \
        *address = 0x04804004; \
    else if (PORT_TO_GROUP(port) == 2) \
        *address = 0x05004004; \
    else \
        *address = 0x05804004;

/* GOP<n> Interrupt Cause Register */
#define GOP_INTR_CAUSE_REG(port, address) \
    if (PORT_TO_GROUP(port) == 0) \
        *address = 0x04004000; \
    else if (PORT_TO_GROUP(port) == 1) \
        *address = 0x04804000; \
    else if (PORT_TO_GROUP(port) == 2) \
        *address = 0x05004000; \
    else \
        *address = 0x05804000;

/* Port<n> MAC Control Register */
#define MAC_CONTROL_REG(port, address) \
    if (PORT_TO_GROUP(port) == 0) \
        *address = 0x04000000 + (0x100 * PORT_OFFSET(port)); \
    else if (PORT_TO_GROUP(port) == 1) \
        *address = 0x04800000 + (0x100 * PORT_OFFSET(port)); \
    else if (PORT_TO_GROUP(port) == 2) \
        *address = 0x05000000 + (0x100 * PORT_OFFSET(port)); \
    else \
        *address = 0x05800000 + (0x100 * PORT_OFFSET(port));

/* MIB Counters Control Register */
#define MIB_COUNT_CONTROL_REG0(port, address) \
    if (PORT_TO_GROUP(port) == 0) \
        *address = 0x04004020; \
    else if (PORT_TO_GROUP(port) == 1) \
        *address = 0x04804020; \
    else if (PORT_TO_GROUP(port) == 2) \
        *address = 0x05004020; \
    else \
        *address = 0x05804020;

/* Target Device to Cascading Port Register0 */
#define TRG_DEV_CASCADE_PORT_REG(device, address) \
    *address = 0x01840000 + (0x4 * (device / 4))

/* Bridge Port<n> Control Register */
#define BRDG_PORT0_CTRL_REG(port, address) \
    *address = 0x02000000 + (0x1000 * port);

/* DSCP To Cos Register n (0 <= n < 32) */
#define DSCP_TO_COS_REG(dscp, address) \
    *address = 0x02040200 + (0x4 * (dscp / 2));

/* PVLAN Port<n> Register */
#define PVLAN_PORT0_REG(port, address) \
    *address = 0x02000010 + (0x1000 * port);

/* Port <n> Protocol Vid0 Register */
#define PROTOCOL_VID0_REG(port, address) \
    *address = 0x02000800 + (0x1000 * port);

/* Port <n> Protocol Vid0 Register */
#define PROTOCOL_VID1_REG(port, address) \
    *address = 0x02000804 + (0x1000 * port);

/* Trunk Table Register */
#define TRUNK_TABLE_REG(_trunk_id, address) \
    snetSalsaTrunkTableReg(_trunk_id, address);

/* Trunk Designated Ports hash<n> */
#define TRUNK_DESIGN_PORTS_HASH_REG(hash, address) \
    *address = 0x01800288 + (0x1000 * hash);

/* Trunk<n> Non-Trunk Members Register */
#define TRUNK_NON_TRUNK_MEMBER_REG(trunk, address) \
    *address = 0x01800280 + (0x1000 * trunk);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __sregsalsah */


