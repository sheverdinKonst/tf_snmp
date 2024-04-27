/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetXCatPolicer.h
*
* DESCRIPTION:
*       XCat Policing Engine processing for frame -- simulation
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 16 $
*
*******************************************************************************/
#ifndef __snetXCatPolicerh
#define __snetXCatPolicerh

#include <asicSimulation/SKernel/smain/smain.h>
#include <asicSimulation/SKernel/suserframes/snetCheetah3Policer.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Get current TOD clock value */
#define SNET_XCAT_IPFIX_TOD_CLOCK_GET_MAC(dev, cycle) \
    SIM_OS_MAC(simOsTickGet)() - (dev)->ipFixTimeStampClockDiff[(cycle)]

/* Check if counters enabled */
#define SNET_XCAT_POLICER_POLICY_COUNT_ENABLE_GET_MAC(dev, descr, direction)  \
    (((direction) == SMAIN_DIRECTION_INGRESS_E) ?                   \
                        (descr)->policerCounterEn :                 \
                        (descr)->policerEgressCntEn)

/* Enable/Disable counters */
#define SNET_XCAT_POLICER_POLICY_COUNT_ENABLE_SET_MAC(dev, descr, direction, enable)  \
    if((direction) == SMAIN_DIRECTION_INGRESS_E) {                  \
        (descr)->policerCounterEn = enable;                         \
    } else {                                                        \
        (descr)->policerEgressCntEn = enable;                       \
    }

/* define policer memory with 1792 entries */
#define POLICER_MEMORY_1792_CNS         1792
/* define policer memory with 256 entries */
#define POLICER_MEMORY_256_CNS         256

/*
 * Typedef: enum SNET_XCAT_POLICER_IPFIX_COUNT_E
 *
 * Description:
 *      This enum defines Policer IPFIX counters
 * Fields:
 *      SNET_XCAT_POLICER_IPFIX_DROP_PKTS_COUNT_E - IPFIX drop packets counters
 *      SNET_XCAT_POLICER_IPFIX_GOOD_PKTS_COUNT_E - IPFIX good packets counters
 *      SNET_XCAT_POLICER_IPFIX_BYTES_COUNT_E   - IPFIX bytes counters
 *
 *  Comments:
 */
typedef enum {
    SNET_XCAT_POLICER_IPFIX_DROP_PKTS_COUNT_E,
    SNET_XCAT_POLICER_IPFIX_GOOD_PKTS_COUNT_E,
    SNET_XCAT_POLICER_IPFIX_BYTES_COUNT_E,
    SNET_XCAT_POLICER_IPFIX_UNKNOWN_COUNT_E
} SNET_XCAT_POLICER_IPFIX_COUNT_E;


/* SIP5: Enumeration of Policer 'Traffic Packet Size for Metering and Counting' modes */
typedef enum {
    SNET_LION3_POLICER_PACKET_SIZE_FROM_METER_ENTRY_E,       /* calc using byte count modes in the meter entry */
    SNET_LION3_POLICER_PACKET_SIZE_FROM_BILLING_ENTRY_E,     /* calc using byte count modes in the billing entry (but not for IPFIX !!!! IPFIX uses 'other')*/
    SNET_LION3_POLICER_PACKET_SIZE_FROM_GLOBAL_CONFIG_E      /* calc using byte count modes in global configuration */
} SNET_LION3_POLICER_PACKET_SIZE_MODE_ENT;

/* SIP5: Enumeration of Policer metering modes */
typedef enum {
    SNET_LION3_POLICER_METERING_MODE_SrTCM_E,/*The metering algorithm for this entry is Single-rate Three Color Marking.;*/
    SNET_LION3_POLICER_METERING_MODE_TrTCM_E,/*The metering algorithm for this entry is Two-rate Three Color Marking.;*/
    SNET_LION3_POLICER_METERING_MODE_MEF0_E, /*Two rate metering according to MEF with CF=0;                          */
    SNET_LION3_POLICER_METERING_MODE_MEF1_E,  /*Two rate metering according to MEF with CF=1;                          */
    SNET_LION3_POLICER_METERING_MODE_MEF10_3_START_E,  /* MEF10.3 start of envelope */
    SNET_LION3_POLICER_METERING_MODE_MEF10_3_NOT_START_E  /* MEF10.3 not start envelope member */
} SNET_LION3_POLICER_METERING_MODE_ENT;

/* This structure used fo conformance level calculation. */
/* It replaces the pointer to metering entry to support  */
/* the MEF10.3 envelope of entries.                      */
/* It also used for single metering entries both SIP5    */
/* and legacy devices.                                   */
typedef struct
{
    GT_BOOL valid;
    GT_U32  envelopeSize;         /* 1 - 8 */
    GT_U32  envelopeBaseIndex;    /* used entry# envelopeBaseIndex + packetRank */
    GT_U32  packetRank;           /* 0 - 7 */
    GT_U32  *meterCfgEntryPtr;
    GT_U32  *meterConformSignLevelPtr;
    GT_U32  meterGreenBucketNumber; /* 0 or 1 */
    GT_U32  meterGreenBucketEntryIndex; /* index from table origin */
    GT_U32  *meterGreenBucketEntryPtr;
    GT_U32  meterYellowBucketNumber; /* 0 or 1 */
    GT_U32  meterYellowBucketEntryIndex; /* index from table origin */
    GT_U32  *meterYellowBucketEntryPtr;
} SNET_LION3_POLICER_METERING_DATA_STC;

/*******************************************************************************
*   snetXCatCommonPolicerMeteringEnvelopeDataGet
*
* DESCRIPTION:
*        Get metering data for envelope of entries
*        Relevant for both ingeress and egress policer.
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - frame data buffer Id
*       policerMeterEntryPtr - pointer to Policers Metering Entry
*
* OUTPUT:
*       meteringDataPtr  - (pointer to) metering data
*
* RETURN:
*
*
*******************************************************************************/
GT_STATUS snetXCatCommonPolicerMeteringEnvelopeDataGet
(
    IN  SKERNEL_DEVICE_OBJECT                *devObjPtr,
    IN  SKERNEL_FRAME_CHEETAH_DESCR_STC      *descrPtr,
    IN  GT_U32                               *policerMeterEntryPtr,
    OUT SNET_LION3_POLICER_METERING_DATA_STC *meteringDataPtr
);

/*******************************************************************************
*   snetXCatCommonPolicerMeteringSingleDataGet
*
* DESCRIPTION:
*        Get metering data for single entry.
*        Relevant for both ingeress and egress policer.
*
* INPUTS:
*       devObjPtr            - pointer to device object.
*       descrPtr             - frame data buffer Id
*       policerMeterEntryPtr - pointer to Policers Metering Entry
*
* OUTPUT:
*       meteringDataPtr  - (pointer to) metering data
*
* RETURN:
*
*
*******************************************************************************/
GT_VOID snetXCatCommonPolicerMeteringSingleDataGet
(
    IN  SKERNEL_DEVICE_OBJECT                *devObjPtr,
    IN  SKERNEL_FRAME_CHEETAH_DESCR_STC      *descrPtr,
    IN  GT_U32                               *policerMeterEntryPtr,
    OUT SNET_LION3_POLICER_METERING_DATA_STC *meteringDataPtr
);

/*******************************************************************************
*   snetXcatIpfixTimestampFormat
*
* DESCRIPTION:
*       Convert tick clocks to time stamp format
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       clock           - clock value in ticks
*
* OUTPUT:
*       timestampPtr    - pointer to timestamp format value
*
* RETURN:
*
* COMMENTS:
*       Time stamp format:
*           [15:8] The eight least significant bits of the seconds field
*           [7:0] The eight most significant bits of the nanoseconds field
*
*******************************************************************************/
GT_VOID snetXcatIpfixTimestampFormat
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 clock,
    OUT GT_U32 * timeStampPtr
);

/*******************************************************************************
*   snetXcatIpfixCounterWrite
*
* DESCRIPTION:
*       Set packets/data units billing counter value
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       descrPtr            - packet descriptor (ignored when NULL)
*       ipFixCounterRegPtr  - pointer to IPFix counter register write memory
*       bytes               - byte counter write value
*       packets             - packets counter write value
*       stamps              - stamps counter write value
*       drops               - drops counter write value
*
* OUTPUT:
*
* RETURN:
*
* COMMENTS:
*
*******************************************************************************/
GT_VOID snetXcatIpfixCounterWrite
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_U32 * ipFixCounterRegPtr,
    IN GT_U64 * bytesCntPtr,
    IN GT_U32 packets,
    IN GT_U32 stamps,
    IN GT_U32 drops
);

/*******************************************************************************
*   snetXCatPolicerCounterIncrement
*
* DESCRIPTION:
*       Increment Policer Billing/Policy/VLAN Counters
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - frame data buffer Id
*       direction   - ingress/egress policer direction
*       policerCtrlRegPtr - pointer to policer global control register
*       policerMeterEntryPtr - pointer to policer metering entry
*       egressPort     -  egress port.
*       cl             -  conformance level
*       bytes       - counter increment in bytes
*
* OUTPUT
*       bytesCountPtr  - counter increment in bytes
*
* RETURN:
*
*******************************************************************************/
GT_VOID snetXCatPolicerCounterIncrement
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SMAIN_DIRECTION_ENT direction,
    IN GT_U32 * policerCtrlRegPtr,
    IN GT_U32 * policerMeterEntryPtr,
    IN GT_U32 egressPort,
    IN SKERNEL_CONFORMANCE_LEVEL_ENT cl,
    OUT GT_U32 * bytesCountPtr
);

/*******************************************************************************
*   snetXCatPolicerBillingCounterIncrement
*
* DESCRIPTION:
*        Increment Billing Policer Counters
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - frame data buffer Id
*       direction   - ingress/egress policer direction
*       policerMeterEntryPtr  - policer entry pointer
*       egressPort     - local egress port (not global).
*       cl             -  conformance level
*       bytesCount       - counter increment in bytes
*
* RETURN:
*
*******************************************************************************/
GT_VOID snetXCatPolicerBillingCounterIncrement
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SMAIN_DIRECTION_ENT direction,
    IN GT_U32 * policerMeterEntryPtr,
    IN GT_U32 egressPort,
    IN SKERNEL_CONFORMANCE_LEVEL_ENT cl,
    IN GT_U32 bytesCount
);

/*******************************************************************************
*   snetXCatPolicerMngCounterIncrement
*
* DESCRIPTION:
*        Increment Management Counters
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       address     - billing entry address
*       dp          - counter
*       increment   - counter increment
*
* RETURN:
*
*******************************************************************************/
GT_VOID snetXCatPolicerMngCounterIncrement
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 address,
    IN GT_U32 dp,
    IN GT_U32 increment
);

/*******************************************************************************
*   snetXCatEgressPolicer
*
* DESCRIPTION:
*        Egress Policer Processing, Policer Counters updates
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - frame data buffer Id
*       egressPort     - local egress port (not global).
*
* RETURN:
*
*******************************************************************************/
void snetXCatEgressPolicer
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_U32 egressPort
);

/*******************************************************************************
*   snetXCatPolicerOverrideMeterIndex
*
* DESCRIPTION:
*        Override if need index to policer entry
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - frame data buffer Id
*       direction       - direction of PLR (ingress/Egress)
*       entryIndexPtr   - pointer to meter entry index
*
* OUTPUT:
*       entryIndexPtr   - pointer to (new) meter entry index
*
* RETURN:
*
*******************************************************************************/
GT_VOID snetXCatPolicerOverrideMeterIndex
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SMAIN_DIRECTION_ENT direction,
    INOUT GT_U32 * entryIndexPtr
);

/*******************************************************************************
*   snetXCatPolicerCountingEntryFormatGet
*
* DESCRIPTION:
*       Return format of counting entry - Full or Short.
*       If device do not support counting entry format select - this mean "Full" format.
*       See Policer Control1 register, "Counting Entry Format Select" field
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       direction   - ingress/egress policer direction
*       cycle       - ingress policer cycle
*
* OUTPUT
*       None
*
* RETURN:
*       0 - entry format is "Full" (Long)
*       1 - entry format is "Compressed" (short)
*
*******************************************************************************/
GT_U32 snetXCatPolicerCountingEntryFormatGet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SMAIN_DIRECTION_ENT direction,
    IN GT_U32 cycle
);


/*******************************************************************************
*   snetLion2PolicerEArchIndexGet
*
* DESCRIPTION:
*        Override if need index to policer entry
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - frame data buffer Id
*       direction       - direction of PLR (ingress/Egress)
*       egressPort     - local egress port (not global) - relevant only for EPLR
*       entryIndexPtr   - pointer to meter entry index
*
* OUTPUT:
*       entryIndexPtr   - pointer to (new) meter entry index
*
* RETURN:
*        GT_TRUE - eArch (ePort/eVlan) metering used
*        GT_FALSE - eArch (ePort/eVlan) metering NOT used
*
*******************************************************************************/
GT_BOOL snetLion2PolicerEArchIndexGet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SMAIN_DIRECTION_ENT direction,
    IN GT_U32       egressPort,
    INOUT GT_U32 * entryIndexPtr
);

/*******************************************************************************
*   snetLion3PolicerPacketSizeGet
*
* DESCRIPTION:
*       Sip5 : Traffic Packet Size for Metering and Counting.
*             Get number of bytes for metering/billing/other .
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - frame data buffer Id
*       direction   - ingress/egress policer direction
*       packetSizeMode - packet size mode
*       entryPtr - pointer to the meter/billing entry that hold info for the calculation
*                   relevant when : packetSizeMode = SNET_LION3_POLICER_PACKET_SIZE_FROM_METER_ENTRY_E
*                                or packetSizeMode = SNET_LION3_POLICER_PACKET_SIZE_FROM_BILLING_ENTRY_E
*
* OUTPUT
*       bytesCountPtr  - counter in bytes
*
* RETURN:
*
*******************************************************************************/
GT_VOID snetLion3PolicerPacketSizeGet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SMAIN_DIRECTION_ENT direction,
    IN SNET_LION3_POLICER_PACKET_SIZE_MODE_ENT packetSizeMode,
    IN GT_U32           *entryPtr,
    OUT GT_U32          *bytesCountPtr
);

/*******************************************************************************
*   snetLion3PolicerMeterSinglePacketTockenBucketApply
*
* DESCRIPTION:
*       Sip5 : Add logic for meter packet byte count compare and token bucket state.
*       The current state of the bucket is in the "Bucket Size0" and "Bucket Size1" fields.
*       Device updates these field automatically according to "Max Burst Size0" and "Max Burst Size1"
*       The logic is valid with assumption that single packet is used for testing.
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - frame data buffer Id
*       direction       - ingress/egress policer direction
*       meteringDataPtr - related policer entries info pointer
*
* OUTPUT
*       qosProfileInfoPtr   - Set the value of qosProfileInfoPtr->cl
*
* RETURN:
*
*******************************************************************************/
GT_VOID snetLion3PolicerMeterSinglePacketTokenBucketApply
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SMAIN_DIRECTION_ENT direction,
    IN SNET_LION3_POLICER_METERING_DATA_STC *meteringDataPtr,
    OUT SNET_CHT3_POLICER_QOS_INFO_STC * qosProfileInfoPtr
);

/*******************************************************************************
* snetPlrTablesFormatInit
*
* DESCRIPTION:
*        init the format of PLR tables.(IPLR0,1 and EPLR)
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*
* OUTPUTS:
*       None.
*
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
void snetPlrTablesFormatInit(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __snetXCatPolicerh */


