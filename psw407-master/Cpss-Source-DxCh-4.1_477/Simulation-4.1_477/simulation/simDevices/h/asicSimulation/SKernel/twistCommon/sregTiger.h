/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* sregTiger.h
*
* DESCRIPTION:
*       Defines API for Twist memory registers access.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 6 $
*
*******************************************************************************/
#ifndef __sregTigerh
#define __sregTigerh

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define TIGER_CPU_PORT_CNS          63

/* REGISTERS ADDRESSES */

/* InLif control register */
#define SMEM_TIGER_INLIF_CONTROL_REG                    0x02800038

/* InLif lookup mode register 0 */
#define SMEM_TIGER_INLIF_LOOKUPMODE_REG0                0x02800000

/* InLif lookup mode register 1 */
#define SMEM_TIGER_INLIF_LOOKUPMODE_REG1                0x028000E4

/* InLif lookup mode register 2 */
#define SMEM_TIGER_INLIF_LOOKUPMODE_REG2                0x02800204

/* InLif lookup mode register 3 */
#define SMEM_TIGER_INLIF_LOOKUPMODE_REG3                0x02800208

/* InLif per port table offset */
#define SMEM_TIGER_PER_PORT_INLIF_TABLE_OFFSET          0x02C60000

/* InLif per vlan table offset */
#define SMEM_TIGER_PER_VLAN_INLIF_TABLE_OFFSET_REG      0x0280022C

/* InLif per vlan mapping table */
#define SMEM_TIGER_PER_VLAN_INLIF_MAPPING_TABLE_OFFSET  0x02D80000

/* control memory offset   */
#define SMEM_TIGER_CONTROL_MEMORY_OFFSET_REG            0x00000000

/* TCB global counters */
#define TCB_GLOBAL_RECEIVED_PACKETS_REG                 0x0280007C
#define TCB_GLOBAL_RECEIVED_OCTETS_LOW_REG              0x02800080
#define TCB_GLOBAL_RECEIVED_OCTETS_HI_REG               0x02800084

#define TCB_GLOBAL_DROPPED_PACKETS_REG                  0x02800088
#define TCB_GLOBAL_DROPPED_OCTETS_LOW_REG               0x0280008C
#define TCB_GLOBAL_DROPPED_OCTETS_HI_REG                0x02800090


#define TRAFFIC_COND_INTERRUPT_CAUSE_REG                0x028001C0
#define TRAFFIC_COND_INTERRUPT_MASK_REG                 0x028001C4

/* PCI Interrupt Summary Cause */
#define PCI_INT_CAUSE_REG                               0x00000114

/* sFlow control register */
#define  SFLOW_CTRL_REG                                 0x03800100

/* sFlow value register */
#define  SFLOW_VALUE_REG                                0x03800104

/* sFlow status register */
#define  SFLOW_STATUS_REG                               0x03800108

/* sFlow counter status register */
#define  SFLOW_COUNT_STATUS_REG                         0x0380010C

/* NSRAM address */
#define SMEM_TIGER_NSRAM_ADDR(base, offset) (base | (offset * 0x4))

/* Cascade Group Register 1 */
#define CASCADE_GRP_1_REG                               0x03024048

/* Cascade Group Register 2 */
#define CASCADE_GRP_2_REG                               0x0302404C

/* Virtual Port Mapping Line N Register */
#define INGRESS_VM_PORTS_REG                            0x03022000

/* Egress Virtual Port Configuration <n> Register */
#define EGRESS_VM_PORTS_REG                             0x01800880

/* Egress Virtual Port Configuration port's offsets  */
#define EGRESS_VM_PORTS_REG_OFFSET(port, reg_address, fld_offset)               \
{                                                                               \
    GT_U32  __regIdx = ((port)/2);                                              \
    (reg_address) = EGRESS_VM_PORTS_REG + 4 *(__regIdx%4) + 0x100*(__regIdx/4); \
    (fld_offset) = ((port) % 2) * 16;                                           \
}

/* DSA Tag Priority Mapping Register */
#define DSA_TAG_PRI_MAP_REG                             0x03024070

/* TCAM write access registers mask */
#define TCAM_WRITE_ACCESS_MASK_CNS                      0x2E70000

/* Policy Engine Control Register #1 */
#define POLICY_ENGINE_CTRL1_REG                         0x028001A4


/* Policy TCAM Entry<n> Data Word<m> (0<=n<1024, 0<=m<3) */
#define SMEM_TIGER_PCL_TCAM_ENTRY_WORD0_REG(address) \
        (0x0A800000 | (address & 0xfffff))

/* Policy TCAM Entry<n> Data Word<m> (0<=n<1024, 3<=m<6)  */
#define SMEM_TIGER_PCL_TCAM_ENTRY_WORD3_REG(address) \
        (SMEM_TIGER_PCL_TCAM_ENTRY_WORD0_REG(address) + 0x10)

/* Policy TCAM Entry<n> Data Word<m> (1024<=n<2048, 0<=m<3) */
#define SMEM_TIGER_PCL_TCAM_ENTRY_WORD0_EXT_REG(address) \
        (0x0A800000 | (address & 0xfffff) | (0x200 << 6))

/* Policy TCAM Entry<n> Data Word<m> (1024<=n<2048, 3<=m<6)  */
#define SMEM_TIGER_PCL_TCAM_ENTRY_WORD3_EXT_REG(address) \
        (SMEM_TIGER_PCL_TCAM_ENTRY_WORD0_EXT_REG(address) + 0x10)

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __sregTigerh */


