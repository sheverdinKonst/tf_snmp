/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetLion.h
*
* DESCRIPTION:
*       This is a external API definition for snet Lion module of SKernel.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 21 $
*
*******************************************************************************/
#ifndef __snetLionh
#define __snetLionh


#include <asicSimulation/SKernel/smain/smain.h>
#include <common/Utils/FrameInfo/sframeInfoAddr.h>
#include <asicSimulation/SKernel/suserframes/snetCheetahL2.h>
#include <asicSimulation/SKernel/suserframes/snetCheetah3TTermination.h>
#include <asicSimulation/SKernel/suserframes/snetXCatPcl.h>
#include <asicSimulation/SKernel/suserframes/snetCheetahEgress.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define SNET_LION_CRC_HEADER_HASH_BYTES_CNS                     70

/* Convert port number to TOD group to use */
/* {0-15,16-31,32-47,48-51,52-55,56-59,60-63,64-67,68-71}->{0,1,2,3,4,5,6,7,8} */
#define SNET_LION3_PORT_NUM_TO_TOD_GROUP_CONVERT_MAC(dev,portNum)           \
    ((dev->supportSingleTai == 0) ?                                         \
        (((portNum) < 48) ? ((portNum)/0x10) : ((portNum - 48)/4)) :        \
        0)

/**
 * Typedef enum :  SNET_LION_CRC_HASH_INPUT_KEY_FIELDS_ID_ENT
 *
 * Description : The 70B hash input includes various fields from the packet header,
 *               and is selected according to the packet type
 *
*/
typedef enum{
    SNET_LION_CRC_HASH_L4_TARGET_PORT_E,
    SNET_LION_CRC_HASH_L4_SOURCE_PORT_E,
    SNET_LION_CRC_HASH_IPV6_FLOW_E,
    SNET_LION_CRC_HASH_RESERVED_55_52_E,
    SNET_LION_CRC_HASH_IP_DIP_3_E,
    SNET_LION_CRC_HASH_IP_DIP_2_E,
    SNET_LION_CRC_HASH_IP_DIP_1_E,
    SNET_LION_CRC_HASH_IP_DIP_0_E,
    SNET_LION_CRC_HASH_IP_SIP_3_E,
    SNET_LION_CRC_HASH_IP_SIP_2_E,
    SNET_LION_CRC_HASH_IP_SIP_1_E,
    SNET_LION_CRC_HASH_IP_SIP_0_E,
    SNET_LION_CRC_HASH_MAC_DA_E,
    SNET_LION_CRC_HASH_MAC_SA_E,
    SNET_LION_CRC_HASH_MPLS_LABEL0_E,
    SNET_LION_CRC_HASH_RESERVED_431_428_E,
    SNET_LION_CRC_HASH_MPLS_LABEL1_E,
    SNET_LION_CRC_HASH_RESERVED_455_452_E,
    SNET_LION_CRC_HASH_MPLS_LABEL2_E,
    SNET_LION_CRC_HASH_RESERVED_479_476_E,
    SNET_LION_CRC_HASH_LOCAL_SOURCE_PORT_E,
    SNET_LION_CRC_HASH_UDB_14_TO_22_E,
    /*Lion3 only*/
    SNET_LION3_CRC_HASH_UDB_0_TO_11_E,
    SNET_LION3_CRC_HASH_UDB_23_TO_34_E,
    SNET_LION3_CRC_HASH_EVID_E,
    SNET_LION3_CRC_HASH_ORIG_SRC_EPORT_OR_TRNK_E,
    SNET_LION3_CRC_HASH_LOCAL_DEV_SRC_EPORT_E,

    SNET_LION_CRC_HASH_LAST_E,


}SNET_LION_CRC_HASH_INPUT_KEY_FIELDS_ID_ENT;

/**
 *  Typedef struct : SNET_LION_PCL_ACTION_STC
 *
 *  Description :
 *              The policy engine maintains an 1024 entries table,
 *              corresponding to the 1024 rules that may be defined in the
 *              TCAM lookup structure. The line index of the matching rule
 *              is used to index the policy action table and extract the
 *              action to perform.
 *
*/
typedef struct {
    SNET_XCAT_PCL_ACTION_STC baseAction;
    GT_BIT modifyMacDa;
    GT_BIT modifyMacSa;
}SNET_LION_PCL_ACTION_STC;

/**
 *  Typedef enum :  SNET_LION_PTP_TOD_EVENT_ENT
 *
 *  Description :
 *              The TOD counter supports four types of time-driven and time-setting
 *              functions — Update, Increment, Capture, and Generate.
 *  Values :
 *              SNET_LION_PTP_TOD_UPDATE_E -
 *                                  Copies the value from the TOD counter shadow
 *                                  to the TOD counter register.
 *              SNET_LION_PTP_TOD_INCREMENT_E -
 *                                  Adds the value of the TOD counter shadow
 *                                  to the TOD counter register.
 *              SNET_LION_PTP_TOD_CAPTURE_E -
 *                                  Copies the value of the TOD counter
 *                                  to the TOD counter shadow register
 *              SNET_LION_PTP_TOD_GENERATE_E -
 *                                  Generates a pulse on the external interface
 *                                  at the configured time, determined
 *                                  by the TOD counter shadow (not supported in simulation).
 *
 *
********************************************************************************/
typedef enum
{
    SNET_LION_PTP_TOD_UPDATE_E,
    SNET_LION_PTP_TOD_INCREMENT_E,
    SNET_LION_PTP_TOD_CAPTURE_E,
    SNET_LION_PTP_TOD_GENERATE_E
}SNET_LION_PTP_TOD_EVENT_ENT;

/**
 *  Typedef enum :  SNET_LION3_PTP_TOD_FUNC_ENT
 *
 *  Description :
 *              The operations supported by the TOD counter .
 *  Values :
 *              SNET_LION3_PTP_TOD_UPDATE_E -
 *                                  Copies the value from the shadow timer to the timer.
 *              SNET_LION3_PTP_TOD_FREQ_UPDATE_E -
 *                                  Copies the value from the shadow timer to the fractional
 *                                  nanosecond drift.
 *              SNET_LION3_PTP_TOD_INCREMENT_E -
 *                                  Adds the value of the shadow timer to the timer.
 *              SNET_LION3_PTP_TOD_DECREMENT_E -
 *                                  Subtracts the value of the shadow timer to the timer.
 *              SNET_LION3_PTP_TOD_CAPTURE_E -
 *                                  Copies the value of the timer to the capture area.
 *              SNET_LION3_PTP_TOD_GRACEFUL_INC_E -
 *                                  Gracefully increment the value of the shadow timer to the timer.
 *              SNET_LION3_PTP_TOD_GRACEFUL_DEC_E -
 *                                  Gracefully decrement the value of the shadow timer to the timer.
 *              SNET_LION3_PTP_TOD_NOP_E -
 *                                  No operation is performed
 *
********************************************************************************/
typedef enum
{
    SNET_LION3_PTP_TOD_UPDATE_E,
    SNET_LION3_PTP_TOD_FREQ_UPDATE_E,
    SNET_LION3_PTP_TOD_INCREMENT_E,
    SNET_LION3_PTP_TOD_DECREMENT_E,
    SNET_LION3_PTP_TOD_CAPTURE_E,
    SNET_LION3_PTP_TOD_GRACEFUL_INC_E,
    SNET_LION3_PTP_TOD_GRACEFUL_DEC_E,
    SNET_LION3_PTP_TOD_NOP_E
}SNET_LION3_PTP_TOD_FUNC_ENT;

/**
 *  Typedef enum :  SNET_LION_PTP_GTS_INTERRUPT_EVENTS_ENT
 *
 *  Description :
 *              The TOD Interrupts
 *
 *
********************************************************************************/
typedef enum
{
    SNET_LION_PTP_GTS_INTERRUPT_GLOBAL_FIFO_FULL_E,
    SNET_LION_PTP_GTS_INTERRUPT_VALID_ENTRY_E
}SNET_LION_PTP_GTS_INTERRUPT_EVENTS_ENT;

/*******************************************************************************
*   snetLionEqInterPortGroupRingFrwrd
*
* DESCRIPTION:
*       Perform Inter-Port Group Ring Forwarding
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - pointer to the frame's descriptor.
* OUTPUT:
*
* COMMENTS:
*       A packet is subject to inter-port group ring forwarding if ALL the following conditions are TRUE:
*       - The Multi-port group Lookup feature is globally enabled
*       - The Bridge FDB DA lookup did not find a match
*       - If the packet is ingressed on a local port group ring port, the ingress port
*         is configured as a multi-port group ring port.
*         OR
*       - If the packet is ingressed on a local port group network port, the ingress port is
*         enabled for multi-port group lookup.
*
*******************************************************************************/
GT_VOID snetLionEqInterPortGroupRingFrwrd
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
);

/*******************************************************************************
*   snetLionL2iPortGroupMaskUnknownDaEnable
*
* DESCRIPTION:
*       Apply/Deny VLAN Unknown/Unregistered commands to unknown/unregistered packets.
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - pointer to the frame's descriptor.
* OUTPUT:
*
* RETURN:
*       GT_TRUE - apply VLAN Unknown/Unregistered commands to unknown/unregistered packets
*       GT_FALSE - deny VLAN Unknown/Unregistered commands to unknown/unregistered packets
*
* COMMENTS:
*       The Bridge engine in each port group supports a set of per-VLAN Unknown/Unregistered commands
*       that can override the default flooding behavior. However, these commands
*       are only applied to unknown/unregistered packets if the bridge engine
*       resides in the last port group of the respective ring.
*
*******************************************************************************/
GT_BOOL snetLionL2iPortGroupMaskUnknownDaEnable
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
);

/*******************************************************************************
*   snetLionIngressSourcePortGroupIdGet
*
* DESCRIPTION:
*       Assign descriptor source port group ID - port group from which
*       the packet ingress the device
*
* INPUTS:
*       devObjPtr    - pointer to device object
*       descrPtr     - pointer to the frame's descriptor
* OUTPUT:
*
* COMMENTS:
*       Source port group ID may be either the local port group Id (when ingress port is not ring port),
*       or a remote port group Id (when ingress port is ring port and value parsed from the DSA)
*
*       The function should be called straight after DSA tag parsing.
*
*******************************************************************************/
GT_VOID snetLionIngressSourcePortGroupIdGet
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
);

/*******************************************************************************
*   snetLionHaEgressMarvellTagSourcePortGroupId
*
* DESCRIPTION:
*       HA - Overrides DSA tag <routed> and <egress filtered registered> bits with
*       Source port group ID.
*
* INPUTS:
*       devObjPtr   - pointer to device object
*       descrPtr    - pointer to the frame's descriptor
*       egressPort  - cascade port to which traffic forwarded
*       mrvlTagPtr  - pointer to marvell tag
*
* OUTPUT:
*       mrvlTagPtr  - pointer to marvell tag
*
* COMMENTS:
*
*       Source port group ID may be either the local port group Id (when ingress port is not ring port),
*       or a remote port group Id (when ingress port is ring port and value parsed from the DSA)
*       It is passed in the DSA tag <routed> and <egress filtered registered> bits.
*
*       The function should be called straight after Marvell Tag creation.
*******************************************************************************/
GT_VOID snetLionHaEgressMarvellTagSourcePortGroupId
(
    IN  SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN  GT_U32  egressPort,
    INOUT GT_U8 * mrvlTagPtr
);

/*******************************************************************************
*   snetLionL2iGetIngressVlanInfo
*
* DESCRIPTION:
*       Get Ingress VLAN Info
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - pointer to the frame's descriptor.
*
* OUTPUT:
*       vlanInfoPtr  - pointer to VLAN info structure
*
*
*******************************************************************************/
GT_VOID snetLionL2iGetIngressVlanInfo
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    OUT SNET_CHEETAH_L2I_VLAN_INFO * vlanInfoPtr
);

/*******************************************************************************
*   snetLionTxQGetEgressVidxInfo
*
* DESCRIPTION:
*        Get egress VLAN and STP information
* INPUTS:
*        deviceObj       - pointer to device object.
*        descrPtr        - pointer to the frame's descriptor.
*        packetType      - type of packet.
*
* OUTPUTS:
*        destPorts       - destination ports.
*        destVlanTagged  - destination vlan tagged.
*        stpVector       - stp ports vector.
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
GT_VOID snetLionTxQGetEgressVidxInfo
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SKERNEL_EGR_PACKET_TYPE_ENT packetType,
    OUT GT_U32 destPorts[],
    OUT GT_U8 destVlanTagged[],
    OUT SKERNEL_STP_ENT stpVector[]
);

/*******************************************************************************
* snetLionTxQGetTrunkDesignatedPorts
*
* DESCRIPTION:
*        Trunk designated ports bitmap
*
* INPUTS:
*        deviceObjPtr             - pointer to device object.
*        descrPtr                 - pointer to the frame's descriptor.
*        trunkHashMode            - cascade trunk mode
* OUTPUTS:
*        designatedPortsBmpPtr  - pointer to designated port bitmap
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
GT_VOID snetLionTxQGetTrunkDesignatedPorts
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN TRUNK_HASH_MODE_ENT    trunkHashMode,
    OUT SKERNEL_PORTS_BMP_STC * designatedPortsBmpPtr
);

/*******************************************************************************
* snetLionTxQGetDeviceMapTableAddress
*
* DESCRIPTION:
*           Get table entry address from Device Map table
*           according to source/target device and source/target port
*
* INPUTS:
*           devObjPtr       - pointer to device object.
*           descrPtr        - pointer to the frame's descriptor.
*           trgDev          - target device
*           trgPort         - target port
*           srcDev          - source device
*           srcPort         - source port
* OUTPUTS:
*
* RETURNS:
*           Device Map Entry address
*           value - SMAIN_NOT_VALID_CNS --> meaning : indication that the device
*                                                     map table will not be accessed
*
* COMMENTS:
*
*
*******************************************************************************/
GT_U32 snetLionTxQGetDeviceMapTableAddress
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_U32 trgDev,
    IN GT_U32 trgPort,
    IN GT_U32 srcDev,
    IN GT_U32 srcPort
);

/*******************************************************************************
*   snetLionCrcBasedTrunkHash
*
* DESCRIPTION:
*        CRC Based Hash Index Generation Procedure
*
* INPUTS:
*       deviceObj           - pointer to device object.
*       descrPtr            - pointer to the frame's descriptor.
*
* RETURN:
*
********************************************************************************/
GT_VOID snetLionCrcBasedTrunkHash
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
);

/*******************************************************************************
* snetLionCutThroughTrigger
*
* DESCRIPTION:
*        Cut-Through Mode Triggering
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - pointer to frame data buffer Id.
*
* OUTPUT:
*       descrPtr        - pointer to frame data buffer Id.
*
* RETURN:
*
* COMMENTS:
*       Cut-through mode can be enabled per source port and priority.
*       The priority in this context is the user priority field in the VLAN tag:
*           - A packet is identified as VLAN tagged for cut-through purposes
*           if its Ethertype (TPID) is equal to one of two configurable VLAN Ethertypes.
*           - For cascade ports, the user priority is taken from the DSA tag.
*
*
*******************************************************************************/
GT_VOID snetLionCutThroughTrigger
(
    IN SKERNEL_DEVICE_OBJECT                * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC   * descrPtr
);

/*******************************************************************************
* snetLionHaKeepVlan1Check
*
* DESCRIPTION:
*        Ha - check for forcing 'keeping VLAN1' in the packet even if need to
*        egress without tag1 (when ingress with tag1)
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - pointer to frame data buffer Id.
*       egressPort      - egress port
*
* OUTPUT:
*       none
*
* RETURN:
*
* COMMENTS:
*       Enable keeping VLAN1 in the packet, for packets received with VLAN1 and
*       even-though the tag-state of this {egress-port, VLAN0} is configured in
*       the VLAN-table to {untagged} or {VLAN0}.
*
*******************************************************************************/
GT_VOID snetLionHaKeepVlan1Check
(
    IN SKERNEL_DEVICE_OBJECT                * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC   * descrPtr,
    IN GT_U32   egressPort
);

/*******************************************************************************
* snetLionTxqDeviceMapTableAccessCheck
*
* DESCRIPTION:
*           Check if need to access the Device map table.
*
* INPUTS:
*           devObjPtr       - pointer to device object.
*           descrPtr        - pointer to the frame's descriptor.
*           srcPort         - the source port
*                               Global for port group shared device.
*                               Local(the same value as localDevSrcPort)
*                               for non-port group device.
*           trgDev          - the target device
*           trgPort         - the target port
* OUTPUTS:
*           none
* RETURNS:
*           GT_TRUE     - device map need to be accessed
*           GT_FALSE    - device map NOT need to be accessed
* COMMENTS:
*           from the FS:
*           The device map table can also be accessed when the target device is
*           the local device.
*           This capability is useful in some modular systems, where the device
*           is used in the line card (see Figure 164).
*           In some cases it is desirable for incoming traffic from the network
*           ports to be forwarded through the fabric when the target device is
*           the local device. This is done by performing a device map lookup for
*           the incoming upstream traffic.
*           Device map lookup for the local device is based on a per-source-port
*           and per-destination-port configuration. The device map table is thus
*           accessed if one of the following conditions holds:
*           1. The target device is not the local device.
*           2. The target device is the local device and both of the following
*              conditions hold:
*              A. The local source port is enabled for device map lookup for
*                 local device according to the per-source-port configuration.
*              B. The target1 port is enabled for device map lookup for local
*                 device according to the per-destination-port configuration.
*
*******************************************************************************/
GT_BOOL snetLionTxqDeviceMapTableAccessCheck
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_U32 srcPort,
    IN GT_U32 trgDev,
    IN GT_U32 trgPort
);

/*******************************************************************************
* snetLionResourceHistogramCount
*
* DESCRIPTION:
*       Gathering information about the resource utilization of the Transmit
*       Queues
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - pointer to frame data buffer Id.
*
* OUTPUT:
*
* RETURN:
*
* COMMENTS:
*       For every successful packet enqueue, if the Global Descriptors Counter
*       exceeds the Threshold(n), the Histogram Counter(n) is incremented by 1.
*
*
*******************************************************************************/
GT_VOID snetLionResourceHistogramCount
(
    IN SKERNEL_DEVICE_OBJECT                * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC   * descrPtr
);

/*******************************************************************************
*   snetLionPtpIngressTimestampProcess
*
* DESCRIPTION:
*        Ingress PTP Timestamp Processing
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - frame data buffer Id
*
* RETURN:
*
*******************************************************************************/
GT_VOID snetLionPtpIngressTimestampProcess
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
);

/*******************************************************************************
*   snetLionHaPtpEgressTimestampProcess
*
* DESCRIPTION:
*        Ha - Egress PTP Timestamp Processing
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - frame data buffer Id
*       egressPort  - egress port
*
* RETURN:
*
*******************************************************************************/
GT_VOID snetLionHaPtpEgressTimestampProcess
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_U32 egressPort
);

/*******************************************************************************
*   snetLionPtpCommandResolution
*
* DESCRIPTION:
*        PTP Trapping/Mirroring to the CPU
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - frame data buffer Id
*
* RETURN:
*
*******************************************************************************/
GT_VOID snetLionPtpCommandResolution
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
);

/*******************************************************************************
*   snetLionPtpTodCounterApply
*
* DESCRIPTION:
*        TOD Counter Functions
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       direction   - ingress or egress direction
*       todEvent    - TOD counter event type
*
* RETURN:
*
* COMMENTS: The TOD counter supports four types of time-driven and time-setting
*           functions — Update, Increment, Capture, and Generate.
*           These functions use a shadow register, which has exactly the same format
*           as the TOD counter
*
*******************************************************************************/
GT_VOID snetLionPtpTodCounterApply
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SMAIN_DIRECTION_ENT direction,
    IN SNET_LION_PTP_TOD_EVENT_ENT todEvent
);

/*******************************************************************************
*   snetLionPtpTodCounterSecondsRead
*
* DESCRIPTION:
*        Read TOD counter seconds
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       direction   - ingress or egress direction
*
* OUTPUT:
*       seconds     - TOD counter seconds
*
* RETURN:
*
*
*******************************************************************************/
GT_VOID snetLionPtpTodCounterSecondsRead
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SMAIN_DIRECTION_ENT direction,
    OUT GT_U64 * seconds64
);

/*******************************************************************************
*   snetLionPtpTodCounterNanosecondsRead
*
* DESCRIPTION:
*        Read TOD counter nanoseconds
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       direction   - ingress or egress direction
*
* OUTPUT:
*       nanoseconds - TOD counter nanoseconds
*
* RETURN:
*
*
*******************************************************************************/
GT_VOID snetLionPtpTodCounterNanosecondsRead
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SMAIN_DIRECTION_ENT direction,
    OUT GT_U32 * nanoseconds
);

/*******************************************************************************
*   snetLion3PtpTodCounterApply
*
* DESCRIPTION:
*        TOD Counter Functions
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       todEvent    - TOD counter event type
*       taiGroup    - The TAI selected group - one of 9
*       taiInst     - within the selected group - TAI0 or TAI1

*
* RETURN:
*
* COMMENTS:
*           The functions are based on shadow\capture registers which have the
*           same format as the TOD counter
*
*******************************************************************************/
GT_VOID snetLion3PtpTodCounterApply
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SNET_LION3_PTP_TOD_FUNC_ENT todEvent,
    IN GT_U32 taiGroup,
    IN GT_U32 taiInst
);

/*******************************************************************************
*   snetLionPclUdbKeyValueGet
*
* DESCRIPTION:
*        Get user defined value by user defined key.
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       descrPtr            - pointer to frame data buffer Id.
*       direction           - ingress or egress direction
*       udbType             - UDB type
*       udbIdx              - UDB index in UDB configuration entry.
*
* OUTPUTS:
*       byteValuePtr        - pointer to UDB value.
*
* RETURN:
*       GT_OK - OK
*       GT_FAIL - Not valid byte
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS snetLionPclUdbKeyValueGet
(
    IN SKERNEL_DEVICE_OBJECT                        * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC              * descrPtr,
    IN SMAIN_DIRECTION_ENT                            direction,
    IN GT_U32                                         udbIdx,
    OUT GT_U8                                       * byteValuePtr
);

/*******************************************************************************
*   snetLion3PtpTodGetTimeCounter
*
* DESCRIPTION:
*        Get the TOD content
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       taiGroup    - The TAI selected group - one of 9
*       taiInst     - within the selected group - TAI0 or TAI1
*
* OUTPUT:
*       timeCounterPtr - (pointer to) the TOD contetnt
*
* RETURN:
*
* COMMENTS:
*
*******************************************************************************/
GT_VOID snetLion3PtpTodGetTimeCounter
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 taiGroup,
    IN GT_U32 taiInst,
    OUT SNET_TOD_TIMER_STC *timeCounterPtr
);

/*******************************************************************************
*   snetlion3TimestampQueueRemoveEntry
*
* DESCRIPTION:
*       Remove entry from timestamp queue (due to CPU read)
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       direction   - ingress or egress.
*       queueNum    - 0 or 1.
*       queueEntryPtr - pointer to queue entry data.
*
* OUTPUT:
*       None
*
* COMMENTS:
*
*******************************************************************************/
GT_VOID snetlion3TimestampQueueRemoveEntry
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SMAIN_DIRECTION_ENT   direction,
    IN GT_U32 queueNum
);

/*******************************************************************************
*   snetlion3TimestampPortEgressQueueRemoveEntry
*
* DESCRIPTION:
*       Remove entry from timestamp port egress queue (due to CPU read)
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       egressPort  - Egress port.
*       queueNum    - 0 or 1.
*
* OUTPUT:
*       None
*
* COMMENTS:
*
*******************************************************************************/
GT_VOID snetlion3TimestampPortEgressQueueRemoveEntry
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 egressPort,
    IN GT_U32 queueNum
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __snetLionh */


