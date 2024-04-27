/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* sregTwist.h
*
* DESCRIPTION:
*       Defines API for Twist memory registers access.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 28 $
*
*******************************************************************************/
#ifndef __sregtwisth
#define __sregtwisth

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define TWIST_CPU_PORT_CNS          63

#define FLD_MASK(offset, length) \
                ((length % 32) ? \
                ((~(0xFFFFFFFF << (length))) << offset) : (0xFFFFFFFF))

#define FLD_REVERSE_MASK(offset, length) \
                ((length % 32) ? \
                (~(~(0xFFFFFFFF << (length))) << offset) : ~(0xFFFFFFFF))

#define WORD_FIELD_GET(base, word, offset, length) \
                ((*(base + word) & FLD_MASK(offset, length)) >> offset)

#define WORD_FIELD_SET(base, word, offset, length, val) \
                *(base + word) |= \
                ((*(base + word) & FLD_REVERSE_MASK(offset, length)) | \
                (val << offset))

/* Giga port */
#define IS_GIGA(port) \
                (port == 48 || port == 49 || port == 50 || port == 51)

#define GIGA_OFFSET(port) \
                ((port == 48 || port == 49) ? \
                (23 + (port - 47)) : (23 + (port - 49)))

/* Group number 0 and 1, ports from 0...51 */
#define PORT_TO_GROUP(port) \
                (IS_GIGA(port) ? (port / 0x32) : (port / 0x18))

/* Port offset from 0...25, ports from 0...51 */
#define PORT_OFFSET(port)  \
                (IS_GIGA(port) ? GIGA_OFFSET(port) : (port % 0x18))

/* Port offset from 0...25, ports from 0...51 */
#define OFFSET1(port)  \
            ( ((port % 0x3) ==  0) ? 0x0 : \
              ((port % 0x3) ==  1) ? 0x800 : 0x1000 )

/* Port offset from 0...11 */
#define PORT_OFFSET1(port)  \
                ((port < 0x6) ? OFFSET1(port) : (0x100 * (port % 0x3)))

/* Fields offset */
#define VPT2TC_BIT(tag)             (3 * (tag))

#define TC2VPT_BIT(tc)              VPT2TC_BIT(tc)

/* Protocols Encapsulation 0 and 1 Registers */
#define PROT_ENCAPSULATION_REG(prot) \
                ((prot < 7) ? 0x0204005C : 0x02040060)

/* Subnet Based VLAN Registers */
#define SUBNET_IP_REG(subnet_num, address) \
                *address = ( 0x02040800 + (subnet_num * 0x4) )

#define SUBNET_MASK_REG(subnet_num, address) \
                *address = (subnet_num <  4) ? 0x02040064 : \
                (subnet_num <  8) ? 0x02040068 : \
                (subnet_num < 12) ? 0x0204006C : 0

#define SUBNET_MASK_LEN_BIT(subnet_num) \
                (subnet_num <  4) ?  (subnet_num * 6) : \
                (subnet_num <  8) ? ((subnet_num - 4) * 6) : \
                (subnet_num < 12) ? ((subnet_num - 8) * 6) : 0

#define SUBNET_VID_REG(subnet_num, address) \
                *address = ( 0x02040840 + (subnet_num * 0x4) )

/* EtherType<n>  Ethernet first bit */
#define PROT_ENCAPSULATION_ETHER_BIT(prot) \
                ((prot < 7) ? (3 * prot) : (3 * (prot - 8)))

/* EtherType<n>  LLC first bit */
#define PROT_ENCAPSULATION_LLC_BIT(prot) \
                PROT_ENCAPSULATION_ETHER_BIT(prot) + 1

/* EtherType<n>  LLC/SNAP first bit */
#define PROT_ENCAPSULATION_LLC_SNAP_BIT(prot) \
                PROT_ENCAPSULATION_ETHER_BIT(prot) + 2

#define ETHER_TYPE_REG(prot, address) \
                *address = PROTOCOL0_REG + (0x04 * prot / 2)

#define SPAN_STATE_TABLE_PORT(port) \
                (port < 16) ? (2 * port) : \
                (port < 32) ? (2 * (port - 16)) : \
                (port < 48) ? (2 * (port - 32)) : \
                (port < 52) ? (2 * (port - 48)) : 0

/* port membership in vlan table */
#define INTERNAL_VTABLE_PORT_OFFSET(port)\
                (port <  6) ? (20 + (2 * port)) : \
                (port < 22) ? (2 * (port -  6)) : \
                (port < 38) ? (2 * (port - 22)) : \
                (port < 52) ? (2 * (port - 38)) : 0

#define RANGE_DACMD(range) \
                (4 * range)
#define RANGE_DAMIRROR(range) \
                (2 * range)

#define RANGE_SACMD(range) \
                ((4 * range) + 2)
#define RANGE_SAMIRROR(range) \
                ((2 * range) + 1)

#define PORT_FILTER_UNK(port) \
                (8 + port)

/* REGISTERS ADDRESSES */

/* Global Control Register */
#define GLOBAL_CONTROL_REG                      0x00000058

/* 10GbE Port Interrupt Mask Register */
#define GB_10_PORT_INT_MASK_REG                 0x03800030

/* 10GbE Port Interrupt Cause Register */
#define GB_10_PORT_INT_CAUSE_REG                0x0380002C

/* PCI Interrupt Summary Mask */
#define PCI_INT_MASK_REG                        0x00000118

/* PCI Interrupt Summary Cause */
#define PCI_INT_CAUSE_REG                       0x00000114

/* MISC Interrupt Summary Mask */
#define PCI_INT_MASK_REG                        0x00000118

/* MISC Interrupt Summary Cause */
#define PCI_INT_CAUSE_REG                       0x00000114


/* SDMA Interrupt Mask Register */
#define SDMA_INT_MASK_REG                       0x00002814

/* SDMA Interrupt Registers */
#define SDMA_INT_REG                            0x0000280C

#define  FDB_SUM_INT                            (1<<11)
#define  AGE_VIA_TRIGGER_END_INT                (1<< 3)


/* Protocols 0 Register */
#define PROTOCOL0_REG                           0x0204003C

/* Device Table Register 0 */
#define DEV_TBL_REG0                            0x02040004

/* Transmit Sniffer Register */
#define TRANSMIT_SNIF_REG                       0x01800008

/* Ingress Mirroring Analyzer Configuration */
#define INGR_MIRR_ANALYZER_REG                  0x03004020

                /* Internal registers */
/* sFlow control register */
#define  SFLOW_CTRL_REG                         0x03800100

/* sFlow value register */
#define  SFLOW_VALUE_REG                        0x03800104

/* sFlow status register */
#define  SFLOW_STATUS_REG                       0x03800108

/* sFlow counter status register */
#define  SFLOW_COUNT_STATUS_REG                 0x0380010C

/* MAC Range<n> Filtering0 Register */
#define MAC_RNG_FLTR0_REG(range, address) \
                *address = (range < 4 ? 0x02040078 + (0x0C *  range     ) \
                                      : 0x02040138 + (0x0C * (range - 4)))
/* MAC Range<n> Filtering1 Register */
#define MAC_RNG_FLTR1_REG(range, address) \
                *address = (range < 4 ? 0x0204007C + (0x0C *  range     ) \
                                      : 0x0204013C + (0x0C * (range - 4)))
/* MAC Range<n> Filtering2 Register */
#define MAC_RNG_FLTR2_REG(range, address) \
                *address = (range < 4 ? 0x02040080 + (0x0C *  range     ) \
                                      : 0x02040140 + (0x0C * (range - 4)))

/* Source Address High Register */
#define SRC_ADDR_HIGH_REG                       0x03800010
/* Source Address Middle Register */
#define SRC_ADDR_MID_REG                        0x0380000C

/* MAC Range Filter Command Register */
#define MAC_RNG_FLTR_CMD_REG                    0x02040074
/* MAC Range Mirror Command Register */
#define MAC_RNG_MIRR_CMD_REG                    0x02040034
/* Layer 2 Ingress Control Register */
#define L2_INGRESS_CTRL_REG                     0x02040000

/* MAC table entry<n> word0 */
#define MAC_TAB_ENTRY_WORD0_REG                 0x06000000
/* MAC table entry<n> word1 */
#define MAC_TAB_ENTRY_WORD1_REG                 0x06000004
/* MAC table entry<n> word2 */
#define MAC_TAB_ENTRY_WORD2_REG                 0x06000008
/* MAC table entry<n> word3 */
#define MAC_TAB_ENTRY_WORD3_REG                 0x0600000C

/* MAC Lookup0 Register */
#define MAC_LOOKUP0_REG                         0x0204001C
/* MAC Lookup1 Register */
#define MAC_LOOKUP1_REG                         0x02040020
/* VLAN Lookup Register */
#define VLAN_LOOKUP_REG                         0x02040024

/* MAC Table Action0 Register */
#define MAC_TABLE_ACTION_0_REG                  0x02040014

/* MAC Table Action1 Register */
#define MAC_TABLE_ACTION_1_REG                  0x02040018

/* Egress Bridging Register */
#define EGRESS_BRIDGING_REG                     0x0180000C

/* Transmit Queue Control Register */
#define TRANS_QUEUE_CONTROL_REG                 0x01800004

/* Transmit Queue Control Register */
#define TRANS_QUEUE_COMMAND_REG                 0x00002868

/* Transmit Current Descriptor Pointer Register */
#define TRANS_CURR_DESC_PTR_REG                 0x000026C0

/* Transmit SDMA Interrupt Cause Register */
#define TRANS_SDMA_INTR_CAUSE_REG               0x2810

/* Transmit Packet DMA Interrupt Mask Register (TxSDMAMask) */
#define TRANS_SDMA_INTR_MASK_REG                0x2818

/* Transmit Queue<n> Interrupt Cause Register */
#define TRANS_QUEUE_1_INTR_CAUSE_REG            0x01800080
#define TRANS_QUEUE_2_INTR_CAUSE_REG            0x01800084
#define TRANS_QUEUE_3_INTR_CAUSE_REG            0x01800088
#define TRANS_QUEUE_4_INTR_CAUSE_REG            0x01800380
#define TRANS_QUEUE_5_INTR_CAUSE_REG            0x01800384

/* Transmit Queue<n> Interrupt Mask Register */
#define TRANS_QUEUE_1_INTR_MASK_REG             0x0180070C
#define TRANS_QUEUE_2_INTR_MASK_REG             0x01800188
#define TRANS_QUEUE_3_INTR_MASK_REG             0x0180008C
#define TRANS_QUEUE_4_INTR_MASK_REG             0x01800388
#define TRANS_QUEUE_5_INTR_MASK_REG             0x0180038C

/* Address Update Queue Control Register */
#define AUQ_CONTROL_REG                         0x00000018

/* Miscellaneous Interrupt Mask Register */
#define MISC_INTR_MASK_REG                      0x00000044

/* Miscellaneous Interrupt Cause Register */
#define MISC_INTR_CAUSE_REG                     0x00000040

/* Ethernet Bridge Interrupts */
#define ETHER_BRDG_INTR_REG                     0x02040130

/* Ethernet Bridge Interrupt Mask Cause Register */
#define ETHER_BRDG_INTR_MASK_REG                0x02040134

/* VPT To Traffic Class Register */
#define VPT_2_TRAFFIC_CLASS_REG                 0x02040028

/* Traffic Class to VPT Register */
#define TRAFFIC_2_VPT_CLASS_REG                 0x0204002C

/* Wide SRAM base address */
#define STWIST_WSRAM_BASE_ADDR                  0x28000000

/* Internal Input Logical Interface Table Register */
#define INTERN_INLIF_TABLE_REG                  0x02C60000

/* Default Input Logical Interface Register 1 */
#define DEFAULT_INLIF_REG                       0x02800018

/* Subnet Mask Size Register */
#define SUB_NET_MASK_SIZE_REG                   0x02800014

/* Virtual Router Table Base Address Register */
#define VIRTUAL_ROUT_TABLE_REG                  0x0280010C

/* Default Multicast Table Base Address Register */
#define DEFAULT_MC_TABLE_REG                    0x02800110

/* IPv4 Control Register */
#define IPV4_CONTROL_REG                        0x02800100

/* IPv4 Global Counter 0 Register */
#define IPV4_GLBL_CNT0_REG                      0x02800120

/* IPv4 Global Counter 1 Register */
#define IPV4_GLBL_CNT1_REG                      0x02800124

/* IPv4 Global Counter 2 Register */
#define IPV4_GLBL_CNT2_REG                      0x02800128

/* IPv4 Discard Packet Counter Register */
#define IPV4_GLBL_DSCRD_CNT_PKT_REG             0x0280012C

/* IPv4 Discard Octet Low Counter Register */
#define IPV4_GLBL_DSCRD_OCT_LOW_REG             0x02800130

/* IPv4 Discard Octet High Counter Register */
#define IPV4_GLBL_DSCRD_OCT_HI_REG              0x02800134

/* IPv4 Shadow Counter Register */
#define IPV4_GLBL_SHADOW_CNT_REG                0x02800138

/* L3L7 Disable Counters Register */
#define IPV4_GLBL_L3_L7_DISABLE_REG             0x028001C8

/* Receive Packet Counter Table Register */
#define IPV4_GLBL_RCV_PCKT_CNT_REG              0x02CE0018

/* Receive Octet Counter Low Register */
#define IPV4_GLBL_RCV_OCT_LOW_CNT_REG           0x02CE0014

/* Receive Octet Counter High Register */
#define IPV4_GLBL_RCV_OCT_HI_CNT_REG            0x02CE000C

/* Discard Packet Counter Register */
#define IPV4_GLBL_DSCRD_CNT_REG                 0x02CE0038

/* Discard Octet Counter */
#define IPV4_GLBL_DSCRD_OCT_CNT_REG             0x02CE0034

/* IP Multicast Packet Counter */
#define IPV4_GLBL_MCAST_CNT_REG                 0x02CE002C

/* Trapped Packet Counter Register */
#define IPV4_GLBL_TRAP_CNT_REG                  0x02CE0058

/* Tunnel Terminate Packet Counter Register */
#define IPV4_GLBL_TUNNEL_TERM_CNT_REG           0x02CE0054

/* Failed Reverse Path Forwarding Packet Counter Register */
#define IPV4_GLBL_RVRS_FAIL_CNT_REG             0x02CE004C

/* IP Multicast Linked Lists Base Address */
#define IPV4_IPMC_LINK_LIST_BASE_REG            0x01840004

/* Flow Template Configuration Register<n> */
#define FLOW_TEMPLATE_CONF_REG                  0x02800028

/* Flow Template Select Table Register */
#define FLOW_TEMPLATE_SEL_TBL_REG               0x02C40000

/* TCB Control Registers */
#define TCB_CONTROL_REG                         0x02800040

/* TCB extended Control Registers */
#define TCB_EXTENDED_CONTROL_REG                0x028000B0

/* PCL Control Register */
#define PCL_CONTROL_REG                         0x028000E8

/* Default Key Entry Command Table <N> */
#define TWISTD_CMD_KEY_ENTRY_DEFAULT_TBL        0x02C80100
#define SAMBA_CMD_KEY_ENTRY_DEFAULT_TBL         0x02C80080

/* Class of Service Marking, Re-marking Table Register */
#define COS_MARK_REMARK_REG                     0x02CA0000

/* Class of Service Marking, Re-marking Table Register */
#define SAMBA_COS_REMARK_REG                    0x02D60000

/* Class of Service Lookup Table */
#define  COS_LOOK_UP_TBL                        0x02C00000

/* Class of Service Command Table */
#define  COS_COMMAND_TBL                        0x02C80000

/* inblock control register */
#define INBLOCK_CONTROL_REG                     0x02800038

#define FLOW_TEMPLATE_HASH_CONFIGURATION_REG    0x02C28000/*16 registers*/

#define FLOW_TEMPLATE_HASH_SELECT_TABLE_REG     0x02C20000/*-0x02C203fc */

#define POLICY_CONTROL_REG                      0x028001E8

#define TWIST_TRAFFIC_COND_TABLE_BASE_ADDR_REG  0x02800094

#define TCB_PACKET_COUNT_LIMIT_REG              0x02800064
#define TCB_CDU_COUNT_LIMIT_REG                 0x02800068
#define TCB_POLICING_COUNT_LIMIT_LOW_REG        0x0280006C
#define TCB_POLICING_COUNT_LIMIT_HI_REG         0x02800070

#define TCB_COUNTER_ALARM_REG                   0x02800074
#define TCB_POLICING_COUNTER_ALARM_REG          0x02800078

#define SAMBA_TCB_CDU_LARGE_COUNT_LIMIT_LSB_REG     0x028000EC
#define SAMBA_TCB_LARGE_COUNT_LIMIT_MSB_REG         0x028000F4


/* for samba pcl control register --
   for twistD this is SDL data 1 internal register */
#define SAMBA_PCL_CONTROL_REG                   0x028001D8

#define SAMBA_PCL_ACTION_TO_LOOKUP_MAPPING_REG  0x028001E0

#define SAMBA_POLICY_FINAL_ACTION_SETTING_TABLE_REG 0x028001E4

#define SAMBA_KEY_TABLE_BASE_ADDRESS_REG        0x0280004c

#define SAMBA_KEY_TABLE_SIZE_REG                0x02800050

/*
*        See table 466 [4]
*        The base address of the TCAM's data (values) is : 0x02e28000 - 0x02e2fffc
*        1024 entries of 32 bytes (8 words) only first 5 words of entry are used .
*/
#define SAMBA_TCAM_MEMORY_DATA_START            0x02e28000

/*
*        See table 467 [4]
*        The base address of the TCAM's masks  is : 0x02e20000 - 0x02e27ffc
*        1024 entries of 32 bytes (8 words) only first 5 words of entry are used .
*/
#define SAMBA_TCAM_MEMORY_MASK_START            0x02e20000


/*
*        See table 468 [4]
*        The base address of the PCE table (actions)  is :0x02e34000-0x02e37ffc
*        1024 entries of 16 bytes (4 words) only first 3 words of entry are used .
*/

#define SAMBA_PCE_TABLE_START                   0x02e34000


/*
*        PCL PCE default 0-2 in (3 registers)
*        addresses 0x028001A0 , 0x028001A4 , 0x028001A8 .
*/
#define SAMBA_PCL_PCE_DEFAULT_0_REG              0x028001A0
#define SAMBA_PCL_PCE_DEFAULT_1_REG              0x028001A4
#define SAMBA_PCL_PCE_DEFAULT_2_REG              0x028001A8

#define SAMBA_TCB_GENXS_REG                      0x028000C4


#define TRANSMIT_QUEUE_MIB_COUNT_BASE_REG        0x01800E00
#define BRIDGE_EGRESS_FILTERED_BASE_REG          0x01800F00

#define SAMBA_TCB_READ_METER_COUNTER_1_REG       0x028000C8
#define SAMBA_TCB_READ_METER_COUNTER_2_REG       0x028000CC

#define SAMBA_TCB_READ_POLICY_COUNTER_1_REG      0x028000D0
#define SAMBA_TCB_READ_POLICY_COUNTER_2_REG      0x028000D4
#define SAMBA_TCB_READ_POLICY_COUNTER_3_REG      0x028000D8
#define SAMBA_TCB_READ_POLICY_COUNTER_4_REG      0x028000DC



/* traffic cond management registers */
#define TRAFFIC_COND_MANAGEMENT_OFFSETT_BETWEEN_SETS_CNS 0x80
/* discard */
#define TRAFFIC_COND_DISCARD_OCTET_LOW_REG       0x02CC0074
#define TRAFFIC_COND_DISCARD_PCKT_REG            0x02CC0078

/* green */
#define TRAFFIC_COND_GREEN_OCTET_LOW_REG         0x02CC0014
#define TRAFFIC_COND_GREEN_PCKT_REG              0x02CC0018

/* yellow */
#define TRAFFIC_COND_YELLOW_OCTET_LOW_REG        0x02CC0034
#define TRAFFIC_COND_YELLOW_PCKT_REG             0x02CC0038

/* red */
#define TRAFFIC_COND_RED_OCTET_LOW_REG           0x02CC0054
#define TRAFFIC_COND_RED_PCKT_REG                0x02CC0058


/* TCB global counters */
#define TCB_GLOBAL_RECEIVED_PACKETS_REG          0x0280007C
#define TCB_GLOBAL_RECEIVED_OCTETS_LOW_REG       0x02800080
#define TCB_GLOBAL_RECEIVED_OCTETS_HI_REG        0x02800084

#define TCB_GLOBAL_DROPPED_PACKETS_REG           0x02800088
#define TCB_GLOBAL_DROPPED_OCTETS_LOW_REG        0x0280008C
#define TCB_GLOBAL_DROPPED_OCTETS_HI_REG         0x02800090


#define TRAFFIC_COND_INTERRUPT_CAUSE_REG         0x028001C0
#define TRAFFIC_COND_INTERRUPT_MASK_REG          0x028001C4


/*       MPLS Processor Registers        */

/* Reserved Label Base Address */
#define RSRV_LABEL_BASE_ADDR_REG                 0x02800154
/* MPLS Interface Table Base Address */
#define MPLS_IF_TABL_BASE_ADDR_REG               0x02800158
/* MPLS Control Register */
#define MPLS_CTRL_REG                            0x02800150
/* Default PenUltimate Command Register 0 */
#define DFLT_PENULT_CMD_0_ADDR_REG               0x02800164
/* Default PenUltimate Command Register 1 */
#define DFLT_PENULT_CMD_1_ADDR_REG               0x02800168
/* EXP to CoS Table */
#define EXP_TO_COS_TABLE0_REG                    0x02800188
/*       MPLS counters          */
/* MPLS Management Rx Octet Low SetN */
#define MPLS_MNG_RX_OCT_LOW_0_REG                0x02D00014
/* MPLS Management Rx Packet Counter SetN */
#define MPLS_MNG_RX_PKT_CNT_0_REG                0x02D00018
/* MPLS Management Rx Octet High SetN */
#define MPLS_MNG_RX_OCT_HI_0_REG                 0x02D0000C
/* MPLS Management Discard Octet Low SetN */
#define MPLS_MNG_DSCRD_OCT_LOW_0_REG             0x02D00034
/* MPLS Management Discard Packet SetN */
#define MPLS_MNG_DSCRD_PKT_0_REG                 0x02D00038
/* MPLS Management Discard Octet High SetN */
#define MPLS_MNG_DSCRD_OCT_HI_0_REG              0x02D0002C
/* MPLS Management Trapped Octet Low SetN */
#define MPLS_MNG_TRAP_OCT_LOW_0_REG              0x02D00054
/* MPLS Management Trapped Packets SetN */
#define MPLS_MNG_TRAP_PKT_0_REG                  0x02D00058
/* MPLS Management Trapped Octet High SetN */
#define MPLS_MNG_TRAP_OCT_HI_0_REG               0x02D0004C
/* MPLS Management Invalid Octets Low SetN */
#define MPLS_MNG_INVAL_OCT_LOW_0_REG             0x02D00074
/* MPLS Management Invalid Packets SetN */
#define MPLS_MNG_INVAL_PKT_0_REG                 0x02D00078
/* MPLS Management Invalid Octets High SetN */
#define MPLS_MNG_INVAL_OCT_HI_0_REG              0x02D0006C

/* MPLS Global Received Packets */
#define MPLS_GLB_RCV_PKT_REG                     0x0280016C
/* MPLS Global Received Octets Low */
#define MPLS_GLB_RCV_OCT_LOW_REG                 0x02800170

/* MPLS Global Dropped Packets */
#define MPLS_GLB_DROP_PKT_REG                    0x02800178
/* MPLS Global Dropped Octets Low */
#define MPLS_GLB_DROP_OCT_LOW_REG                0x0280017C

/* Port<n> Transmit Configuration Register */
#define TRANSMIT_CONF_REG(port, address) \
                *address = 0x01800000 + (0x1000 * port);
/* Ports Vlan table) */
#define PORTS_VLAN_TABLE_REG( _vid, _port, address) \
                *address =  \
        (_port <  6) ? 0x0A000000 + (0x4 * 0x4 * _vid) : \
        (_port < 22) ? 0x0A000000 + (0x4 * 0x4 * _vid) + (0x4)  : \
        (_port < 38) ? 0x0A000000 + (0x4 * 0x4 * _vid) + (0x4 * 2)  : \
        (_port < 52) ? 0x0A000000 + (0x4 * 0x4 * _vid) + (0x4 * 3) : 0;

/*Span State Group<n> Entry Word0 */
#define SPAN_STATE_TABLE_REG(_stg, _port, address) \
                *address =  \
        (_port < 16) ? 0x0A020000 + (0x4 * 0x4 * _stg) : \
        (_port < 32) ? 0x0A020000 + (0x4 * 0x4 * _stg) + (0x4)  : \
        (_port < 48) ? 0x0A020000 + (0x4 * 0x4 * _stg) + (0x4 * 2)  : \
        (_port < 52) ? 0x0A020000 + (0x4 * 0x4 * _stg) + (0x4 * 3) : 0;

/* Multicast group Table */
#define MCAST_GROUPS_TABLE_REG(_vidx, _port, address) \
        *address = \
        (_port < 28) ? 0x0A010000 + (0x4 * 0x02 * (_vidx - 4096)) : \
                       0x0A010000 + (0x4 * 0x02 * (_vidx - 4096)) + (0x4);

/* Port<n> MAC Control Register */
#define MAC_CONTROL_REG(portsNum, port, address) \
        if ((portsNum) == 12) { MAC_CONTROL_REG_12G(port, address) } \
        else                  { MAC_CONTROL_REG_52(port, address) }

#define MAC_CONTROL_REG_52(port, address) \
        if (PORT_TO_GROUP(port) == 0) \
                *address = 0x0800000 + (0x100 * PORT_OFFSET(port)); \
        else if (PORT_TO_GROUP(port) == 1) \
                *address = 0x1000000 + (0x100 * PORT_OFFSET(port));

#define MAC_CONTROL_REG_12G(port, address) \
        if (((port)/3) == 0) \
                *address = 0x0800000 + (0x800 * ((port) % 3)); \
        else if (((port)/3) == 1) \
                *address = 0x1000000 + (0x800 * ((port) % 3)); \
        else if (((port)/3) == 2) \
                *address = 0x0801800 + (0x100 * ((port) % 3)); \
        else if (((port)/3) == 3) \
                *address = 0x1001800 + (0x100 * ((port) % 3));

#define MAC_CTRL_ADDR_2_PORT(type, address, group, port) \
{ \
        if (((type) == SKERNEL_98EX126) || ((type) == SKERNEL_98EX125D) || \
            ((type) == SKERNEL_98EX120D) || ((type) == SKERNEL_98MX625D) ) \
        {    MAC_CTRL_12G_ADDR_2_PORT(address, group, port) }\
        else if (((type) == SKERNEL_98EX116) || ((type) == SKERNEL_98EX115D) || \
            ((type) == SKERNEL_98EX110D) || ((type) == SKERNEL_98MX615D) ) \
        {    MAC_CTRL_52_ADDR_2_PORT(address, group, port) }\
        else {port = 0;}\
}

#define MAC_CTRL_52_ADDR_2_PORT(address, group, port) \
{ \
        (port) = ((address) & 0x1fff) >> 8; \
        if ((port) == 0x18) \
            (port) = ((group) == 0) ? 48 : 50; \
        else if ((port) == 0x19) \
            (port) = ((group) == 0) ? 49 : 51; \
        else \
            (port) = ((group) == 0) ? (port) : (port) + 24; \
}

#define MAC_CTRL_12G_ADDR_2_PORT(address, group, port) \
{ \
        (port) = ((address) & 0x1fff) >> 8; \
        if ((port) < 0x18) \
            (port) = ((group) == 0) ? (port / 0x8) : (port / 0x8) + 3; \
        else  \
            (port) = ((group) == 0) ? (port % 0x18) + 6 : (port % 0x18) + 9; \
}

/* MAC Status Register */
#define MAC_STATUS_REG(type, port, address) \
{ \
        if (((type) == SKERNEL_98EX126) || ((type) == SKERNEL_98EX125D) || \
            ((type) == SKERNEL_98EX120D) || ((type) == SKERNEL_98MX625D) ) \
        {    MAC_STATUS_REG_12G(port, address); }\
        else if (((type) == SKERNEL_98EX116) || ((type) == SKERNEL_98EX115D) || \
            ((type) == SKERNEL_98EX110D) || ((type) == SKERNEL_98MX615D) ) \
        {    MAC_STATUS_REG_52(port, address); }\
        else if (((type) == SKERNEL_98EX136) || ((type) == SKERNEL_98EX135D) || \
            ((type) == SKERNEL_98EX130D) || ((type) == SKERNEL_98MX635D) ) \
            (address) = 0x03800000; \
}

#define MAC_STATUS_REG_52(port, address) \
{ \
        if (PORT_TO_GROUP(port) == 0) \
            address = 0x0800004 + (0x100 * PORT_OFFSET(port)); \
        else if (PORT_TO_GROUP(port) == 1) \
            address = 0x1000004 + (0x100 * PORT_OFFSET(port)); \
}

#define MAC_STATUS_REG_12G(port, address) \
{ \
        if (((port) / 3) == 0) \
            address = 0x0800004 + (0x800 * ((port) % 3)); \
        else if (((port) / 3) == 1) \
            address = 0x1000004 + (0x800 * ((port) % 3)); \
        else if (((port) / 3) == 2) \
            address = 0x0801804 + (0x100 * ((port) % 3)); \
        else if (((port) / 3) == 3) \
            address = 0x1001804 + (0x100 * ((port) % 3)); \
}

#define MAC_STATUS_REG0(port, address) \
        if (PORT_TO_GROUP(port) == 0) \
                *address = 0x0800004 + (0x100 * PORT_OFFSET(port)); \
        else if (PORT_TO_GROUP(port) == 1) \
                *address = 0x1000004 + (0x100 * PORT_OFFSET(port));

/* Group number 0 and 1, ports from 0...51 */
#define PORT_TO_GROUP1(port) (port/3)

/* MAC Status Register */
#define MAC_STATUS_REG1(port, regAddress) \
        if (PORT_TO_GROUP1(port) == 0) \
            *regAddress = 0x00800004 + PORT_OFFSET1(port); \
        else if (PORT_TO_GROUP1(port) == 1) \
                *regAddress = 0x01000004 + PORT_OFFSET1(port);\
        else if (PORT_TO_GROUP1(port) == 2) \
                *regAddress = 0x00801804 + PORT_OFFSET1(port);\
        else *regAddress = 0x01001804 + PORT_OFFSET1(port);

/* MAC Status Register */
#define MAC_STATUS_REG2(port, regAddress) \
        if (port < 6) \
            *regAddress = 0x00800004 + (100 * port); \
        else \
            *regAddress = 0x01000004 + (100 * port);\

/* Port<n> MAC MIB Counters Register */
#define MAC_MIB_COUNTERS_REG(portsNum, port, address) \
                if (portsNum == 12) \
                { \
                    MAC_MIB_COUNTERS_REG_12G(port, address) \
                } \
                else \
                { \
                    MAC_MIB_COUNTERS_REG_52(port, address)\
                }
#define MAC_MIB_COUNTERS_REG_52(port, address) \
                if (PORT_TO_GROUP(port) == 0) \
                {                                                       \
                    *address = 0x00810000 + (0x80 * PORT_OFFSET(port)); \
                }                                                       \
                else if (PORT_TO_GROUP(port) == 1) \
                {                                                       \
                    *address = 0x01010000 + (0x80 * PORT_OFFSET(port)); \
                }                                                       \
                else                                               \
                {                                                  \
                    *address = 0xffffffff;/* will cause fatal error*/\
                }

#define MAC_MIB_COUNTERS_REG_12G(port, address) \
                if (((port)/3) == 0) \
                {                                                  \
                    *address = 0x0810000 + (0x400 * ((port) % 3)); \
                }                                                  \
                else if (((port)/3) == 1) \
                {                                                  \
                    *address = 0x1010000 + (0x400 * ((port) % 3)); \
                }                                                  \
                else if (((port)/3) == 2) \
                {                                                  \
                    *address = 0x0810c00 + (0x80  * ((port) % 3)); \
                }                                                  \
                else if (((port)/3) == 3) \
                {                                                  \
                    *address = 0x1010c00 + (0x80  * ((port) % 3)); \
                }                                                  \
                else                                               \
                {                                                  \
                    *address = 0xffffffff;/* will cause fatal error*/\
                }

/* Target Device to Cascading Port Register0 */
#define TRG_DEV_CASCADE_PORT_REG(device, address) \
                *address = 0x01840000 + (0x4 * (device / 4))

/* Bridge Port<n> Control Register */
#define BRDG_PORT0_CTRL_REG(port, address) \
                *address = 0x02000000 + (0x1000 * port);

/* Port <0> Protocol Vid Register */
#define PROTOCOL_VID_REG(prot, port, address) \
                *address = 0x02000800 + (0x04 * (prot / 2)) + (port * 0x1000);

/* Port Vid Register */
#define PORT_VID_REG(port, address) \
                *address = 0x02000004 + (0x1000 * port)

/* Trunk Table Register */
#define TRUNK_TABLE_REG(devFamily, trunk, address) \
        if (devFamily == SKERNEL_TWIST_C_FAMILY) \
                    TWISTC_TRUNK_TABLE_REG(trunk, address); \
        else \
                    TWISTD_TRUNK_TABLE_REG(trunk, address);

#define TWISTD_TRUNK_TABLE_REG(trunk, address) \
                *address = (trunk <  9) ? 0x02040170 : \
        (trunk < 17) ? 0x02040174 : \
        (trunk < 25) ? 0x02040178 : \
        (trunk < 32) ? 0x0204017C : 0

#define TWISTC_TRUNK_TABLE_REG(trunk, address) \
                *address = (trunk <  8) ? 0x02040034 : 0

#define TRUNK_TABLE_MEMBER_NUM(devFamily, trunk) \
                ( ((devFamily) == SKERNEL_TWIST_C_FAMILY)? \
                     TWISTC_TRUNK_TABLE_MEMBER_NUM(trunk): \
                     TWISTD_TRUNK_TABLE_MEMBER_NUM(trunk) )

#define TWISTD_TRUNK_TABLE_MEMBER_NUM(trunk) \
                (trunk <  9) ? ((trunk -  1) * 4) : \
                (trunk < 17) ? ((trunk -  9) * 4) : \
                (trunk < 25) ? ((trunk - 17) * 4) : \
                (trunk < 32) ? ((trunk - 25) * 4) : 0

#define TWISTC_TRUNK_TABLE_MEMBER_NUM(trunk) \
                (trunk <  8) ? ((trunk -  1) * 4) : 0

/* Trunk Designated Ports hash<n> */
#define TRUNK_DESIGN_PORTS_HASH_REG(hash, address) \
                *address = 0x01808280 + (0x1000 * hash);

/* Trunk<n> Non-Trunk Members Register */
#define TRUNK_NON_TRUNK_MEMBER_REG(trunk, address) \
                *address = 0x01801280 + (0x1000 * trunk);

/* Trunk Member Table Register( */
#define TRUNK_MEMBER_TABLE_REG(devFamily, trunk, member, address) \
        if (devFamily == SKERNEL_TWIST_C_FAMILY) \
                    TWISTC_TRUNK_MEMBER_TABLE_REG(trunk, member, address) \
        else \
                    TWISTD_TRUNK_MEMBER_TABLE_REG(trunk, member, address)

#define TWISTD_TRUNK_MEMBER_TABLE_REG(trunk, member, address) \
        *address = 0x02D80000 + ((trunk - 1) * 4 + member / 2) * 4;

#define TWISTC_TRUNK_MEMBER_TABLE_REG(trunk, member, address) \
        *address = 0x03000000 + ((trunk - 1) * 6 + member / 2);

/* Port to Input Logical Interface Register */
#define PORT_TO_IN_LIF_REG(port, address) \
                *address = (port < 32) ? 0x02800000 : 0x028000E4;

#define IS_SINGLE_IN_LIF(port) \
                (port < 32) ? port : (port - 32);

/* DSCP to CoS Table */
#define DSCP_TO_COS_REG(dscp, address) \
        *address = 0x02D20000 + 0x04* (dscp / 4);

/* Internal DIP Registers */
#define INTERN_DIP_REG(reg, address) \
        *address = (reg == 0) ? 0x02800004 : 0x02800008;

/* Receive SDMA Current Rx Descriptor Pointer [7:0] (CRDPx) Register */
#define CUR_RX_DESC_POINTER_REG(rxQueue, address) \
        *address = 0x260C + rxQueue * 0x10;

/* NSRAM address */
#define SMEM_TWIST_NSRAM_ADDR(base, offset) (base | (offset * 0x4))

/* MAC Control Register Mask */
#define     SMEM_TWIST_MAC_CTRL_MASK_CNS    0xffffe0ff

/*******************************************************************************
*   sTwistDoInterrupt
*
* DESCRIPTION:
*       Set Twist interrupt
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       causeRegAddr        - Interrupt Cause Register
*       causeMaskRegAddr    - Interrupt Cause Register Mask
*       causeBitNum         - Interrupt Cause Register Bit
*       globalBitNum        - Global Interrupt Cause Register Bit
*
*       based on function snetChetahDoInterrupt(...)
*******************************************************************************/
GT_VOID sTwistDoInterrupt
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 causeRegAddr,
    IN GT_U32 causeMaskRegAddr,
    IN GT_U32 causeBitBmp,
    IN GT_U32 globalBitBmp
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __sregtwisth */


