/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* sregSoho.h
*
* DESCRIPTION:
*        Defines API for Soho memory registers access.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 18 $
*
*******************************************************************************/
#ifndef __sregsohoh
#define __sregsohoh

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GLB_STATUS_REG              (0x001b0000)
#define GLB_CTRL_REG                (0x001b0040)
#define GLB_MON_CTRL_REG            (0x001b01a0)
#define GLB_CTRL_2_REG              (0x001b01c0)
/* IP QPRI Global Mapping Register */
#define GLB_IP_QPRI_MAP_REG         (0x001b0100)

/* Global Stats Counters registers */
#define GLB_STATS_CNT3_2_REG        (0x001b01e0)
#define GLB_STATS_CNT1_0_REG        (0x001b01f0)

/* Global VTU registers */
#define GLB_VTU_DATA0_3_REG         (0x001b0070)
#define GLB_VTU_DATA4_7_REG         (0x001b0080)
#define GLB_VTU_DATA8_9_REG         (0x001b0090)

/* Global ATU registers */
#define GLB_ATU_CTRL_REG            (0x001b00a0)
#define GLB_ATU_OPER_REG            (0x001b00b0)
#define GLB_ATU_DATA_REG            (0x001b00c0)
#define GLB_ATU_MAC0_1_REG          (0x001b00d0)
#define GLB_ATU_MAC2_3_REG          (0x001b00e0)
#define GLB_ATU_MAC4_5_REG          (0x001b00f0)
/* Global IEEE-PRI register */
#define GLB_IEEE_PRI_REG            (0x001b0180)
/* Global core tag type */
#define GLB_CORE_TAG_TYPE_REG       (0x001b0190)

/* Global Opal Plus registers */
#define GLB_ATU_FID_REG             (0x001b0010)
#define GLB_VTU_SID_REG             (0x001b0030)
#define GLB_MON_CTRL_REG            (0x001b01a0)



/* Global 2 registers */
#define GLB2_INTERUPT_SOURCE        (0x001c0000)
#define GLB2_MGMT_EN_REG_2X         (0x001c0020)
#define GLB2_MGMT_EN_REG            (0x001c0030)
#define GLB2_FLOW_CTRL_REG          (0x001c0040)
#define GLB2_MNG_REG                (0x001c0050)
#define GLB2_ROUT_REG               (0x001c0060)
#define GLB2_TRUNK_MASK_REG         (0x001c0070)
#define GLB2_TRUNK_ROUT_REG         (0x001c0080)
#define GLB2_CROSS_CHIP_ADDR_REG    (0x001c00b0)
#define GLB2_CROSS_CHIP_DATA_REG    (0x001c00c0)
#define GLB2_ATU_STST_REG           (0x001c00d0)
#define GLB2_DEST_POLARITY_REG      (0x001c01d0)


/* Port Registers Addresses */
#define SWITCH_PORT0_STATUS_REG     (0x10100000)
/* Port based VLAN map */
#define PORT_BASED_VLAN_MAP_REG     (0x10100060)
/* Default port Vlan ID & Priority */
#define PORT_DFLT_VLAN_PRI_REG      (0x10100070)
/* Port control 2 register */
#define PORT_CTRL_2_REG             (0x10100080)
/* Port switch identifier  */
#define PORT_SWTC_ID_REG            (0x10100030)
/* Port control register */
#define PORT_CTRL_REG               (0x10100040)
/* Port control 1 register */
#define PORT_CTRL1_REG              (0x10100050)
/* Port Rate Control 2 */
#define PORT_RATE_CTRL_2_REG        (0x101000a0)
/* Port Association Vector */
#define PORT_ASSOC_VECTOR_REG       (0x101000b0)
/* Port Atu Control */
#define PORT_ATU_CONTROL            (0x101000c0)
/* Port Rate Override */
#define PORT_PRIORITY_OVERRIDE      (0x101000d0)
/* Port Policy control */
#define PORT_POLICY_CONTROL         (0x101000e0)
/* Port Etype          */
#define PORT_ETYPE                  (0x101000f0)

/* Port IEEE Priority remapping register */
#define PORT_IEEE_PRIO_REMAP_REG    (0x10100180)
/* Port In Discard low Counter */
#define PORT_INDISCARDLOW_CNTR_REG  (0x10100100)
/* Port Out Discard low  Counter */
#define PORT_INDISCARDHGH_CNTR_REG  (0x10100110)
/* Port InFiltered Counter */
#define PORT_INFILTERED_CNTR_REG    (0x10100120)
/* Port OutFiltered Counter */
#define PORT_OUTFILTERED_CNTR_REG   (0x10100130)

#define PHY_STATUS_REG              (0x20000010)
#define PHY_PORT_STATUS_REG         (0x20000110)
#define PHY_INTERRUPT_ENABLE_REG    (0x20000120)
#define PHY_INTERRUPT_STATUS_REG    (0x20000130)
#define PHY_INTERRUPT_PORT_SUM_REG  (0x20000140)

/* Stats ingress counters */
#define CNT_IN_GOOD_OCTETS_LO_REG   (0x50100000)
#define CNT_IN_UCAST_FRAMES_REG     (0x50100040)
#define CNT_IN_BCST_REG             (0x50100060)
#define CNT_IN_MCST_REG             (0x50100070)

/* Egress counters */
#define CNT_OUT_OCTETS_LO_REG       (0x501000e0)
#define CNT_OUT_UNICAST_FRAMES_REG  (0x50100100)
#define CNT_OUT_MCST_REG            (0x50100120)
#define CNT_OUT_BCST_REG            (0x50100130)

/* Stats egress counters */
#define CNT_64_OCTETS_REG           (0x50100080)
#define CNT_65_TO_127_OCTETS_REG    (0x50100090)
#define CNT_128_TO_255_OCTETS_REG   (0x501000a0)
#define CNT_256_TO_511_OCTETS_REG   (0x501000b0)
#define CNT_512_TO_1023_OCTETS_REG  (0x501000c0)
#define CNT_1024_OCTETS_REG         (0x501000d0)





/* Per port IEEE priority remapping register */
#define IEEE_PORT_PRIORITY_MAP_REG(_pri, _address, _field)  \
        _address = (_pri / 3) + 0x10100180;                 \
        _field = (_pri % 3) * 2;

/* Global IP priority mapping register */
#define IP_GLOBAL_PRIORITY_MAP_REG(_pri, _address, _field)  \
    _address = (_pri / 0x1c) + 0x001b0100;                  \
    _field = (_pri % 0x1c) * 2;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __sregsohoh */


