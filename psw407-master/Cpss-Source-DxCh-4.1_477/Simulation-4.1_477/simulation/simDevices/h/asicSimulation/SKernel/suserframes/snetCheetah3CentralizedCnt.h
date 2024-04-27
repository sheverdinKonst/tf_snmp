/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetCheetah3CentralizedCnt.h
*
* DESCRIPTION:
*       Cheetah3 Asic Simulation
*       Centralized Counter Unit.
*       Header file.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 10 $
*
*******************************************************************************/
#ifndef __snetCheetah3CentralizedCnt
#define __snetCheetah3CentralizedCnt

#include <asicSimulation/SKernel/smain/smain.h>
#include <asicSimulation/SKernel/smem/smem.h>
#include <os/simEnvDepTypes.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 *  Enum: INTERNAL_SPECIFIC_HW_VALUES__SNET_CHT3_CNC_CLIENT_E
 *
 *  Description:
 *      Client number enumeration.
 *
 *  Fields:
 *
 *      ch3 clients:
 *   client 0 - L2/L3 Ingress VLAN
 *   client 1 - Ingress PCL0 lookup 0 (out of 2 dual lookups)
 *   client 2 - Ingress PCL0 lookup 1 (out of 2 dual lookups)
 *   client 3 - Ingress PCL1
 *   client 4 - Ingress VLAN Pass/Drop
 *   client 5 - Egress VLAN Pass/Drop
 *   client 6 - Egress Queue Pass/Drop
 *   client 7 - Egress PCL
 *   client 8 - ARP Table access
 *   client 9 - Tunnel Start
 *   client 10 - Tunnel Termination (index of 'hit' by TTI action)
 *
 *      sip5 clients:
 *   Client 0-1 - Tunnel Termination Interface, 2 parallel lookup clients
 *   Client 2-5- Ingress PCL_0, 4 parallel lookup clients
 *   Client 6-9 - Ingress PCL_1, 4 parallel lookup clients
 *   Client 10-13- Ingress PCL_2, 4 parallel lookup clients
 *   Client 14 - L2/L3 Ingress eVLAN
 *   Client 15 - source ePort
 *   Client 16- Ingress eVLAN Pass/Drop
 *   Client 17- packet type Pass/Drop
 *   Client 18- Egress eVLAN Pass/Drop
 *   Client 19- Egress Queue Pass/Drop and QCN Pass/Drop counters
 *   Client 20- ARP Table access
 *   Client 21 - Tunnel-Start
 *   Client 22 - target ePort
 *   Client 23-26 - Egress PCL, 4 parallel lookup clients
 *   Client 27 - TM Pass/Drop unit
 *
 *  Comments:
 *      To bind a counter block to a client, enable the relevant client number
 *      for each required block.
 **/
typedef enum {
    INTERNAL_SPECIFIC_HW_VALUES__SNET_CHT3_CNC_CLIENT_L2_L3_VLAN_INGR_E = 0,
    INTERNAL_SPECIFIC_HW_VALUES__SNET_CHT3_CNC_CLIENT_PCL0_0_LOOKUP_INGR_E,
    INTERNAL_SPECIFIC_HW_VALUES__SNET_CHT3_CNC_CLIENT_PCL0_1_LOOKUP_INGR_E,
    INTERNAL_SPECIFIC_HW_VALUES__SNET_CHT3_CNC_CLIENT_PCL1_LOOKUP_INGR_E,
    INTERNAL_SPECIFIC_HW_VALUES__SNET_CHT3_CNC_CLIENT_VLAN_PASS_DROP_INGR_E,
    INTERNAL_SPECIFIC_HW_VALUES__SNET_CHT3_CNC_CLIENT_VLAN_PASS_DROP_EGR_E,
    INTERNAL_SPECIFIC_HW_VALUES__SNET_CHT3_CNC_CLIENT_TXQ_QUEUE_PASS_DROP_EGR_E,
    INTERNAL_SPECIFIC_HW_VALUES__SNET_CHT3_CNC_CLIENT_PCL_EGR_E,
    INTERNAL_SPECIFIC_HW_VALUES__SNET_CHT3_CNC_CLIENT_ARP_TBL_E,
    INTERNAL_SPECIFIC_HW_VALUES__SNET_CHT3_CNC_CLIENT_TUNNEL_START_E,
    INTERNAL_SPECIFIC_HW_VALUES__SNET_XCAT_CNC_CLIENT_TUNNEL_TERMINATION_E,

    /* new SIP5 clients */
    /*TTI - actions - start*/
    INTERNAL_SPECIFIC_HW_VALUES__SNET_LION3_CNC_CLIENT_TTI_ACTION_0_E = 0,
    INTERNAL_SPECIFIC_HW_VALUES__SNET_LION3_CNC_CLIENT_TTI_ACTION_1_E,
    /*TTI - actions - end */
    /*IPCL - start*/
    INTERNAL_SPECIFIC_HW_VALUES__SNET_LION3_CNC_CLIENT_IPCL_LOOKUP_0_ACTION_0_E,
    INTERNAL_SPECIFIC_HW_VALUES__SNET_LION3_CNC_CLIENT_IPCL_LOOKUP_0_ACTION_1_E,
    INTERNAL_SPECIFIC_HW_VALUES__SNET_LION3_CNC_CLIENT_IPCL_LOOKUP_0_ACTION_2_E,
    INTERNAL_SPECIFIC_HW_VALUES__SNET_LION3_CNC_CLIENT_IPCL_LOOKUP_0_ACTION_3_E,

    INTERNAL_SPECIFIC_HW_VALUES__SNET_LION3_CNC_CLIENT_IPCL_LOOKUP_1_ACTION_0_E,
    INTERNAL_SPECIFIC_HW_VALUES__SNET_LION3_CNC_CLIENT_IPCL_LOOKUP_1_ACTION_1_E,
    INTERNAL_SPECIFIC_HW_VALUES__SNET_LION3_CNC_CLIENT_IPCL_LOOKUP_1_ACTION_2_E,
    INTERNAL_SPECIFIC_HW_VALUES__SNET_LION3_CNC_CLIENT_IPCL_LOOKUP_1_ACTION_3_E,

    INTERNAL_SPECIFIC_HW_VALUES__SNET_LION3_CNC_CLIENT_IPCL_LOOKUP_2_ACTION_0_E,
    INTERNAL_SPECIFIC_HW_VALUES__SNET_LION3_CNC_CLIENT_IPCL_LOOKUP_2_ACTION_1_E,
    INTERNAL_SPECIFIC_HW_VALUES__SNET_LION3_CNC_CLIENT_IPCL_LOOKUP_2_ACTION_2_E,
    INTERNAL_SPECIFIC_HW_VALUES__SNET_LION3_CNC_CLIENT_IPCL_LOOKUP_2_ACTION_3_E,
    /*IPCL - end */

    INTERNAL_SPECIFIC_HW_VALUES__SNET_LION3_CNC_CLIENT_INGRESS_VLAN_L2_L3_E,
    INTERNAL_SPECIFIC_HW_VALUES__SNET_LION3_CNC_CLIENT_SOURCE_EPORT_E,
    INTERNAL_SPECIFIC_HW_VALUES__SNET_LION3_CNC_CLIENT_INGRESS_VLAN_PASS_DROP_E,
    INTERNAL_SPECIFIC_HW_VALUES__SNET_LION3_CNC_CLIENT_PACKET_TYPE_PASS_DROP_E,
    INTERNAL_SPECIFIC_HW_VALUES__SNET_LION3_CNC_CLIENT_EGRESS_VLAN_PASS_DROP_E,

    INTERNAL_SPECIFIC_HW_VALUES__SNET_LION3_CNC_CLIENT_EGRESS_QUEUE_PASS_DROP_AND_QCN_PASS_DROP_E,
    INTERNAL_SPECIFIC_HW_VALUES__SNET_LION3_CNC_CLIENT_ARP_INDEX_E,
    INTERNAL_SPECIFIC_HW_VALUES__SNET_LION3_CNC_CLIENT_TUNNEL_START_INDEX_E,

    INTERNAL_SPECIFIC_HW_VALUES__SNET_LION3_CNC_CLIENT_TARGET_EPORT_E,

    /*EPCL - start*/
    INTERNAL_SPECIFIC_HW_VALUES__SNET_LION3_CNC_CLIENT_EPCL_ACTION_0_E,
    INTERNAL_SPECIFIC_HW_VALUES__SNET_LION3_CNC_CLIENT_EPCL_ACTION_1_E,
    INTERNAL_SPECIFIC_HW_VALUES__SNET_LION3_CNC_CLIENT_EPCL_ACTION_2_E,
    INTERNAL_SPECIFIC_HW_VALUES__SNET_LION3_CNC_CLIENT_EPCL_ACTION_3_E,
    /*EPCL - end */

    INTERNAL_SPECIFIC_HW_VALUES__SNET_LION3_CNC_CLIENT_TRAFFIC_MANAGER_PASS_DROP_E,

    /* sip5_20*/
    INTERNAL_SPECIFIC_HW_VALUES__SNET_LION3_CNC_CLIENT_TTI_ACTION_2_E,
    INTERNAL_SPECIFIC_HW_VALUES__SNET_LION3_CNC_CLIENT_TTI_ACTION_3_E,


    INTERNAL_SPECIFIC_HW_VALUES__SNET_CHT3_CNC_CLIENT_LAST_E

} INTERNAL_SPECIFIC_HW_VALUES__SNET_CHT_CNC_CLIENT_E;

/* CH3 : All CNC clients bitmap */
#define SNET_CHT3_CNC_CLIENTS_BMP_ALL_CNS \
        SMEM_BIT_MASK(1+INTERNAL_SPECIFIC_HW_VALUES__SNET_XCAT_CNC_CLIENT_TUNNEL_TERMINATION_E)

/* Lion3 : All CNC clients bitmap */
#define SNET_LION3_CNC_CLIENTS_BMP_ALL_CNS                          \
        (SMEM_BIT_MASK(1+INTERNAL_SPECIFIC_HW_VALUES__SNET_LION3_CNC_CLIENT_TRAFFIC_MANAGER_PASS_DROP_E))

/* sip5_20 : All CNC clients bitmap */
#define SNET_SIP5_20_CNC_CLIENTS_BMP_ALL_CNS                          \
        (SMEM_BIT_MASK(1+INTERNAL_SPECIFIC_HW_VALUES__SNET_LION3_CNC_CLIENT_TTI_ACTION_3_E))

typedef enum {
    /* new SIP5 clients */
    /*TTI - actions - start*/
    SNET_CNC_CLIENT_TTI_ACTION_0_E,
    SNET_CNC_CLIENT_TTI_ACTION_1_E,
    /*TTI - actions - end */
    /*IPCL - start*/
    SNET_CNC_CLIENT_IPCL_LOOKUP_0_ACTION_0_E,
    SNET_CNC_CLIENT_IPCL_LOOKUP_0_ACTION_1_E,
    SNET_CNC_CLIENT_IPCL_LOOKUP_0_ACTION_2_E,
    SNET_CNC_CLIENT_IPCL_LOOKUP_0_ACTION_3_E,

    SNET_CNC_CLIENT_IPCL_LOOKUP_1_ACTION_0_E,
    SNET_CNC_CLIENT_IPCL_LOOKUP_1_ACTION_1_E,
    SNET_CNC_CLIENT_IPCL_LOOKUP_1_ACTION_2_E,
    SNET_CNC_CLIENT_IPCL_LOOKUP_1_ACTION_3_E,

    SNET_CNC_CLIENT_IPCL_LOOKUP_2_ACTION_0_E,
    SNET_CNC_CLIENT_IPCL_LOOKUP_2_ACTION_1_E,
    SNET_CNC_CLIENT_IPCL_LOOKUP_2_ACTION_2_E,
    SNET_CNC_CLIENT_IPCL_LOOKUP_2_ACTION_3_E,
    /*IPCL - end */

    SNET_CNC_CLIENT_INGRESS_VLAN_L2_L3_E,
    SNET_CNC_CLIENT_SOURCE_EPORT_E,
    SNET_CNC_CLIENT_INGRESS_VLAN_PASS_DROP_E,
    SNET_CNC_CLIENT_PACKET_TYPE_PASS_DROP_E,
    SNET_CNC_CLIENT_EGRESS_VLAN_PASS_DROP_E,

    SNET_CNC_CLIENT_EGRESS_QUEUE_PASS_DROP_AND_QCN_PASS_DROP_E,
    SNET_CNC_CLIENT_ARP_INDEX_E,
    SNET_CNC_CLIENT_TUNNEL_START_INDEX_E,

    SNET_CNC_CLIENT_TARGET_EPORT_E,

    /*EPCL - start*/
    SNET_CNC_CLIENT_EPCL_ACTION_0_E,
    SNET_CNC_CLIENT_EPCL_ACTION_1_E,
    SNET_CNC_CLIENT_EPCL_ACTION_2_E,
    SNET_CNC_CLIENT_EPCL_ACTION_3_E,
    /*EPCL - end */

    SNET_CNC_CLIENT_TRAFFIC_MANAGER_PASS_DROP_E,

    /*TTI - actions - start*/ /* sip5_20*/
    SNET_CNC_CLIENT_TTI_ACTION_2_E,
    SNET_CNC_CLIENT_TTI_ACTION_3_E,
    /*TTI - actions - end */

    SNET_CNC_CLIENT_LAST_E

} SNET_CNC_CLIENT_E;

/*******************************************************************************
*   snetCht3CncCount
*
* DESCRIPTION:
*       Trigger CNC event for specified client and set CNC counter block
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - pointer to the frame's descriptor.
*       client          - client ID.
*       userDefined     - pointer user defined parameter
*
* RETURNS:
*       None.
*
*******************************************************************************/
void snetCht3CncCount
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_CNC_CLIENT_E client,
    IN GT_UINTPTR userDefined
);

/*******************************************************************************
*   snetCht3CncFastDumpFuncPtr
*
* DESCRIPTION:
*       Process upload CNC block demanded by CPU
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       cncTrigPtr  - pointer to CNC Fast Dump Trigger Register
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
GT_VOID snetCht3CncFastDumpFuncPtr
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 * cncTrigPtr
);

/*******************************************************************************
*   snetCht3CncBlockReset
*
* DESCRIPTION:
*       Centralize counters block reset
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       block       - block index
*       entryStart  - start entry index
*       entryNum    - number of entry to reset
*       cncUnitIndex - the CNC unit index (0/1) (Sip5 devices)
*
* OUTPUTS:
*
*
*******************************************************************************/
GT_VOID snetCht3CncBlockReset
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 block,
    IN GT_U32 entryStart,
    IN GT_U32 entryNum,
    IN GT_U32 cncUnitIndex
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __snetCheetah3CentralizedCnt */


