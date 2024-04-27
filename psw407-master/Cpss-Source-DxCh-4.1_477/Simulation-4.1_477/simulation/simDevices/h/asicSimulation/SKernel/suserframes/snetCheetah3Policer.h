/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetCheetah3Policer.h
*
* DESCRIPTION:
*       Cheetah3 Policing Engine processing for frame -- simulation
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 6 $
*
*******************************************************************************/
#ifndef __snetCheetah3Policerh
#define __snetCheetah3Policerh

#include <asicSimulation/SKernel/smain/smain.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 *
 *  Enum : SNET_CHT3_MNG_COUNT_SET_ENT
 *
 *  Description: 3 global management counters enumerator
 *
**/
typedef enum {
    SNET_CHT3_MNG_SET_0_E = 0,
    SNET_CHT3_MNG_SET_1_E,
    SNET_CHT3_MNG_SET_2_E,
    SNET_CHT3_MNG_SET_DISABLE_E
} SNET_CHT3_MNG_COUNT_SET_ENT;

/**
 *
 *  Enum : SNET_CHT3_PCKT_SIZE_MODE_ENT
 *
 *  Description: Metered packet size
 *  - Layer 1 metering - Entire L2 frame + IPG +Preamble.
 *  - Layer 2 metering - Entire L2 frame
 *  - Layer 3 metering - for non TT packets packet size include L3 information only.
 *    for TT packets packet size include the passenger packet only excluding the tunnel header.
 *
**/
typedef enum {
    SNET_CHT3_LAYER3_PCKT_SIZE_E = 0,
    SNET_CHT3_LAYER2_PCKT_SIZE_E,
    SNET_CHT3_LAYER1_PCKT_SIZE_E
} SNET_CHT3_PCKT_SIZE_MODE_ENT;

/**
 *
 *  Enum : SNET_CHT3_CNT_RES_MODE_ENT
 *
 *  Description: Counter units (1Byte/16Bytes/packets)
 *
 *
**/
typedef enum {
    SNET_CHT3_1_BYTE_COUNT_E = 0,
    SNET_CHT3_16_BYTE_COUNT_E,
    SNET_CHT3_PCKT_COUNT_E
} SNET_CHT3_CNT_RES_MODE_ENT;

/**
 *
 *  Enum : SNET_CHT_POLICER_CMD_ENT
 *
 *  Description: QOS remark - Non confirming Red/Yellow packets QOS profile
 *
 *
**/
typedef enum {
    SNET_CHT_POLICER_CMD_NO_CHG_E = 0,
    SNET_CHT_POLICER_CMD_DROP_E,
    SNET_CHT_POLICER_CMD_REM_QOS_E,
    SNET_CHT_POLICER_CMD_REM_QOS_BY_ENTRY_E
} SNET_CHT_POLICER_CMD_ENT;

/**
 *
 *  Struct : SNET_CHT3_POLICER_MNG_COUNT_STC
 *
 *  Description: Data structure used for update management counters for dropped
 *               packets
 *
 *
**/
typedef struct {
    GT_U32 countSet;
    GT_U32 countScale;
    GT_U32 countByteSize;
} SNET_CHT3_POLICER_MNG_COUNT_STC;

/**
 *
 *  Struct : SNET_CHT3_POLICER_QOS_INFO_STC
 *
 *  Description: QoS initial marking stage.
 *  - Drop Precedence
 *  - Remarked QoS Profile index
 *  - Traffic Class for Egress PCL Remarking
 *  - Conformance Level - the metering result
 *
**/
typedef struct {
    SKERNEL_CONFORMANCE_LEVEL_ENT initialDp;
    GT_U32 qosProfYellowIndex;
    GT_U32 qosProfRedIndex;
    GT_U32 qosProfGreenIndex;/* supported from SIP5 device */
    GT_U32 tc;
    SKERNEL_CONFORMANCE_LEVEL_ENT cl;
} SNET_CHT3_POLICER_QOS_INFO_STC;

#define SNET_CHT3_PCKT_SIZE_RESOLVE(byte, mode) \
    (((mode) == SNET_CHT3_1_BYTE_COUNT_E) ? (byte) : \
     ((mode) == SNET_CHT3_16_BYTE_COUNT_E) ? (byte) / 16 : 1)

/* Management counter field offset */
#define SNET_CHT3_POLICER_MNG_COUNT_FLD_OFFSET_GET_MAC(cnt) \
    ((cnt) == SNET_CHT3_MNG_SET_0_E) ? 8 : \
    ((cnt) == SNET_CHT3_MNG_SET_1_E) ? 9 : 10

/*******************************************************************************
*   snetCht3Policer
*
* DESCRIPTION:
*        Policer Processing  --- Policer Counters updates
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - frame data buffer Id
*
* RETURN:
*
*******************************************************************************/
GT_VOID snetCht3Policer(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
);

/*******************************************************************************
*   snetCht3PolicerPacketSizeGet
*
* DESCRIPTION:
*       Get packet size based on packet size mode
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - frame data buffer Id
*       direction   - ingress/egress policer direction
*       policerCtrlRegPtr - ingress policer control data
*
* OUTPUT
*       bytesCountPtr  - counter increment in bytes
*
* RETURN:
*
*******************************************************************************/
GT_VOID snetCht3PolicerPacketSizeGet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SMAIN_DIRECTION_ENT direction,
    IN GT_U32 * policerCtrlRegPtr,
    OUT GT_U32 * bytesCountPtr
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __snetCheetah3Policerh */


