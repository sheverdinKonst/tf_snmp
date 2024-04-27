/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetCheetah3.h
*
* DESCRIPTION:
*       This is a external API definition for CH3.
*
* FILE REVISION NUMBER:
*       $Revision: 4 $
*
*******************************************************************************/
#ifndef __snetCheetah3h
#define __snetCheetah3h

#include <asicSimulation/SKernel/smain/smain.h>
#include <asicSimulation/SKernel/skernel.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct
{
    GT_U32  startBit;
    GT_U32  lastBit;
}HW_FIELD;


typedef enum{
    SNET_CHT3_MLL_FIELDS_LAST_0_E                                                        ,
    SNET_CHT3_MLL_FIELDS_START_OF_TUNNEL_0_E                                             ,
    SNET_CHT3_MLL_FIELDS_MLL_VID_0_E                                                     ,
    SNET_CHT3_MLL_FIELDS_USE_VIDX_0_E                                                    ,
    SNET_CHT3_MLL_FIELDS_TARGET_IS_TRUNK_0_E                                             ,
    SNET_CHT3_MLL_FIELDS_TRG_PORT_0_E                                                    ,
    SNET_CHT3_MLL_FIELDS_TRG_TRUNK_ID_0_E                                                ,
    SNET_CHT3_MLL_FIELDS_TRG_DEV_0_E                                                     ,
    SNET_CHT3_MLL_FIELDS_VIDX_0_E                                                        ,
    SNET_CHT3_MLL_FIELDS_MLL_RPF_FAIL_CMD_0_E                                            ,
    SNET_CHT3_MLL_FIELDS_TUNNEL_PTR_LSB_0_E                                              ,
    SNET_CHT3_MLL_FIELDS_TUNNEL_TYPE_0_E                                                 ,
    SNET_CHT3_MLL_FIELDS_TTL_THRESHOLD_0_HOP_LIMIT_THRESHOLD_0_E                         ,
    SNET_CHT3_MLL_FIELDS_EXCLUDE_SRC_VLAN_0_E                                             ,
    SNET_CHT3_MLL_FIELDS_TUNNEL_PTR_MSB_0_E                                              ,

    SNET_CHT3_MLL_FIELDS_TUNNEL_PTR_MSB_1_E                                              ,

    SNET_CHT3_MLL_FIELDS_LAST_1_E                                                        ,
    SNET_CHT3_MLL_FIELDS_START_OF_TUNNEL_1_E                                             ,
    SNET_CHT3_MLL_FIELDS_MLL_VID_1_E                                                     ,
    SNET_CHT3_MLL_FIELDS_USE_VIDX_1_E                                                    ,
    SNET_CHT3_MLL_FIELDS_TARGET_IS_TRUNK_1_E                                             ,
    SNET_CHT3_MLL_FIELDS_TRG_PORT_1_E                                                    ,
    SNET_CHT3_MLL_FIELDS_TRG_TRUNK_ID_1_E                                                ,
    SNET_CHT3_MLL_FIELDS_TRG_DEV_1_E                                                     ,
    SNET_CHT3_MLL_FIELDS_VIDX_1_E                                                        ,
    SNET_CHT3_MLL_FIELDS_MLL_RPF_FAIL_CMD_1_E                                            ,
    SNET_CHT3_MLL_FIELDS_TUNNEL_PTR_LSB_1_E                                              ,
    SNET_CHT3_MLL_FIELDS_TUNNEL_TYPE_1_E                                                 ,
    SNET_CHT3_MLL_FIELDS_TTL_THRESHOLD_1_HOP_LIMIT_THRESHOLD_1_E                         ,
    SNET_CHT3_MLL_FIELDS_EXCLUDE_SRC_VLAN_1_E                                             ,

    SNET_CHT3_MLL_FIELDS_NEXT_MLL_PTR_E                                                  ,

/* Added for WLAN bridging -- start */
    SNET_CHT3_MLL_FIELDS_UNREG_BC_FILTERING_0_E                                          ,
    SNET_CHT3_MLL_FIELDS_UNREG_MC_FILTERING_0_E                                          ,
    SNET_CHT3_MLL_FIELDS_UNKNOWN_UC_FILTERING_0_E                                        ,
    SNET_CHT3_MLL_FIELDS_VLAN_EGRESS_TAG_MODE_0_E                                        ,

    SNET_CHT3_MLL_FIELDS_UNREG_BC_FILTERING_1_E                                          ,
    SNET_CHT3_MLL_FIELDS_UNREG_MC_FILTERING_1_E                                          ,
    SNET_CHT3_MLL_FIELDS_UNKNOWN_UC_FILTERING_1_E                                        ,
    SNET_CHT3_MLL_FIELDS_VLAN_EGRESS_TAG_MODE_1_E                                        ,
/* Added for WLAN bridging -- end */

    SNET_CHT3_MLL_FIELDS_LAST_E
}SNET_CHT3_MLL_FIELDS_ENT;

extern const HW_FIELD snetCh3MllEntryFormat[];

typedef enum{
    SNET_CHT3_TS_CAPWAP_FIELD_TUNNEL_TYPE_E,
    SNET_CHT3_TS_CAPWAP_FIELD_UP_MARKING_MODE_E,
    SNET_CHT3_TS_CAPWAP_FIELD_UP_E,
    SNET_CHT3_TS_CAPWAP_FIELD_TAG_ENABLE_E,
    SNET_CHT3_TS_CAPWAP_FIELD_VID_E,
    SNET_CHT3_TS_CAPWAP_FIELD_TTL_OR_HOT_LIMIT_E,
    SNET_CHT3_TS_CAPWAP_FIELD_CAPWAP_T_BIT_E,
    SNET_CHT3_TS_CAPWAP_FIELD_CAPWAP_W_BIT_E,
    SNET_CHT3_TS_CAPWAP_FIELD_CAPWAP_M_BIT_E,
    SNET_CHT3_TS_CAPWAP_FIELD_EGRESS_OSM_REDIRECT_BIT_E,
    SNET_CHT3_TS_CAPWAP_FIELD_NEXT_HOP_MAC_DA_0_31_E,
    SNET_CHT3_TS_CAPWAP_FIELD_NEXT_HOP_MAC_DA_32_47_E,
    SNET_CHT3_TS_CAPWAP_FIELD_WLAN_ID_BITMAP_E,
    SNET_CHT3_TS_CAPWAP_FIELD_DSCP_E,
    SNET_CHT3_TS_CAPWAP_FIELD_DSCP_MARKING_MODE_E,
    SNET_CHT3_TS_CAPWAP_FIELD_DONT_FRAGMENT_FLAG_E,
    SNET_CHT3_TS_CAPWAP_FIELD_CAPWAP_VERSION_E,
    SNET_CHT3_TS_CAPWAP_FIELD_CAPWAP_RID_E,
    SNET_CHT3_TS_CAPWAP_FIELD_CAPWAP_FLAGS_E,
    SNET_CHT3_TS_CAPWAP_FIELD_CAPWAP_WBID_E,
    SNET_CHT3_TS_CAPWAP_FIELD_ENABLE_802_11_WDS_E,
    SNET_CHT3_TS_CAPWAP_FIELD_DEFAULT_802_11E_ENABLE_E,
    SNET_CHT3_TS_CAPWAP_FIELD_802_11E_MAPPING_PROFILE_E,
    SNET_CHT3_TS_CAPWAP_FIELD_IPV4_DIP_E,
    SNET_CHT3_TS_CAPWAP_FIELD_IPV4_SIP_E,

    SNET_CHT3_TS_CAPWAP_FIELD_IPV4_UDP_SRC_PORT_E,
    SNET_CHT3_TS_CAPWAP_FIELD_IPV4_UDP_DST_PORT_E,
    SNET_CHT3_TS_CAPWAP_FIELD_IPV4_BSSID_OR_TA_0_31_E,
    SNET_CHT3_TS_CAPWAP_FIELD_IPV4_BSSID_OR_TA_32_47_E,
    SNET_CHT3_TS_CAPWAP_FIELD_IPV4_RA_0_15_E,
    SNET_CHT3_TS_CAPWAP_FIELD_IPV4_RA_16_47_E,

    SNET_CHT3_TS_CAPWAP_FIELD_IPV6_DIP_0_31_E,
    SNET_CHT3_TS_CAPWAP_FIELD_IPV6_DIP_32_63_E,
    SNET_CHT3_TS_CAPWAP_FIELD_IPV6_DIP_64_95_E,
    SNET_CHT3_TS_CAPWAP_FIELD_IPV6_DIP_96_127_E,
    SNET_CHT3_TS_CAPWAP_FIELD_IPV6_SIP_0_31_E,
    SNET_CHT3_TS_CAPWAP_FIELD_IPV6_SIP_32_63_E,
    SNET_CHT3_TS_CAPWAP_FIELD_IPV6_SIP_64_95_E,
    SNET_CHT3_TS_CAPWAP_FIELD_IPV6_SIP_96_127_E,

    SNET_CHT3_TS_CAPWAP_FIELD_IPV6_UDP_SRC_PORT_E,
    SNET_CHT3_TS_CAPWAP_FIELD_IPV6_UDP_DST_PORT_E,
    SNET_CHT3_TS_CAPWAP_FIELD_IPV6_BSSID_OR_TA_0_31_E,
    SNET_CHT3_TS_CAPWAP_FIELD_IPV6_BSSID_OR_TA_32_47_E,
    SNET_CHT3_TS_CAPWAP_FIELD_IPV6_RA_0_15_E,
    SNET_CHT3_TS_CAPWAP_FIELD_IPV6_RA_16_47_E,

}SNET_CHT3_TS_CAPWAP_FIELDS_ENT;

extern const HW_FIELD snetCh3TsCapwapEntry[];

typedef struct {
    GT_U32  rpf_fail_cmd;
    GT_U32 ttlThres;
    GT_U32 excludeSrcVlan;
    GT_U32 last;
    SNET_DST_INTERFACE_STC lll;
    GT_U32  vid;
    GT_U32  isTunnelStart;
    struct{
        GT_U32  tunnelStartType;
        GT_U32  tunnelStartPtr;
    }tsInfo;/* tunnel start info */

/* Added for WLAN bridging -- start */
    GT_BIT      unregBcFiltering;
    GT_BIT      unregMcFiltering;
    GT_BIT      unknownUcFiltering;
    GT_BIT      vlanEgressTagMode;
/* Added for WLAN bridging -- end */

}SNET_CHT3_SINGLE_MLL_STC;

/* Multicast Link List */
typedef struct {
    SNET_CHT3_SINGLE_MLL_STC      first_mll;
    SNET_CHT3_SINGLE_MLL_STC      second_mll;
    GT_U32              nextPtr;
}SNET_CHT3_DOUBLE_MLL_STC;

/*******************************************************************************
*   checkSumCalc
*
* DESCRIPTION:
*        Perform ones-complement sum , and ones-complement on the final sum-word.
*        The function can be used to make checksum for various protocols.
* INPUTS:
*        pMsg - pointer to IP header.
*        lMsg  - IP header length.
* OUTPUTS:
*
* COMMENTS:
*        1. If there's a field CHECKSUM within the input-buffer
*           it supposed to be zero before calling this function.
*
*        2. The input buffer is supposed to be in network byte-order.
*******************************************************************************/
GT_U16 checkSumCalc
(
    IN GT_U8 *pMsg,
    IN GT_U16 lMsg
);

/*******************************************************************************
*   snetCht3EgressConvert802dot11ToEthernetV2Frame
*
* DESCRIPTION:
*       Egress conversion from 802.11/11e to Ethernet v2 frame.
*
* INPUTS:
*       devObjPtr       - pointer to device object
*       descrPtr        - pointer to frame descriptor
*       egrBufPtr       - pointer to egress buffer
* OUTPUT:
*       egrBufPtr       - pointer to modified egress buffer
*
* RETURN:
*       pointer to end of modified egress buffer
*
* COMMENTS:
*       called from the HA (header alteration)
*******************************************************************************/
GT_U8 *  snetCht3EgressConvert802dot11ToEthernetV2Frame
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC   * descrPtr,
    INOUT GT_U8                             * egrBufPtr
);

/*******************************************************************************
*   snetCht3IngressTunnelReplication
*
* DESCRIPTION:
*       Ingress L2 tunnel replication
*
* INPUTS:
*       devObjPtr       - pointer to device object
*       descrPtr        - pointer to frame descriptor
*
* OUTPUT:
*
* RETURN:
*
* COMMENTS:
*
*******************************************************************************/
GT_VOID snetCht3IngressL2TunnelReplication
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC   * descrPtr
);

/*******************************************************************************
*   snetCht3ReadMllEntry
*
* DESCRIPTION:
*         Read Multicast Link List entry
* INPUTS:
*        regAddress - MLL memory address.
*        mllIndex   - MLL index (of the line) in the memory
* OUTPUTS:
*        mllPtr  - pointer to the MLL entry.
*
*
*******************************************************************************/
void snetCht3ReadMllEntry
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32                  mllIndex,
    OUT SNET_CHT3_DOUBLE_MLL_STC *mllPtr
);





/*******************************************************************************
*   snetCht3HaTunnelStart
*
* DESCRIPTION:
*       Tunnel start - entry point .
*       This function is part of the HA (header alteration) processing.
*       function should add the tunnel start header before the L2/L3 passenger
*       frame.
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - pointer to the frame's descriptor.
*       egressPort   - the local egress port (not global port)
* OUTPUTS:
*       descrPtr     - pointer to updated frame data buffer Id
*
* RETURN:
*
*******************************************************************************/
GT_VOID snetCht3HaTunnelStart
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_U32   egressPort
);

/*******************************************************************************
*   snetCht3IngressL3IpReplication
*
* DESCRIPTION:
*       Ingress L3 Ipv4/6 replication
*
* INPUTS:
*       devObjPtr       - pointer to device object
*       descrPtr        - pointer to frame descriptor
*
* OUTPUT:
*
* RETURN:
*
* COMMENTS:
*
*******************************************************************************/
GT_VOID snetCht3IngressL3IpReplication
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC   * descrPtr
);

/*******************************************************************************
*   snetCht3mllCounters
*
* DESCRIPTION:
*       Update MLL counters
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       descrPtr            - pointer to the frame's descriptor.
*       isL2Mll             - is counting for L2_MLL or IP_MLL
*                             relevant only to SIP5 devices
*
*******************************************************************************/
GT_VOID snetCht3mllCounters
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_BOOL      isL2Mll
);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __snetCheetah3h */
