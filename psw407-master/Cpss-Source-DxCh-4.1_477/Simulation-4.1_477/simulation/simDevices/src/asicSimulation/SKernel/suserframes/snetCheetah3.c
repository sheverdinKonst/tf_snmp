/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetCheetah.c
*
* DESCRIPTION:
*       This is the file for cheetah3 network file.
*
* FILE REVISION NUMBER:
*       $Revision: 31 $
*
*******************************************************************************/

#include <asicSimulation/SKernel/suserframes/snet.h>
#include <asicSimulation/SKernel/smem/smemCheetah.h>
#include <asicSimulation/SKernel/cheetahCommon/sregCheetah.h>
#include <asicSimulation/SKernel/suserframes/snetCheetah3.h>
#include <asicSimulation/SKernel/suserframes/snetCheetah3MacLookup.h>
#include <asicSimulation/SKernel/suserframes/snetCheetah3Reassembly.h>
#include <asicSimulation/SKernel/suserframes/snetCheetahPclSrv.h>
#include <asicSimulation/SKernel/suserframes/snetCheetahEq.h>
#include <asicSimulation/SKernel/suserframes/snetCheetahEgress.h>
#include <asicSimulation/SKernel/suserframes/snetCheetah2TStart.h>
#include <asicSimulation/SLog/simLog.h>
#include <asicSimulation/SLog/simLogInfoTypePacket.h>

#define UDP_LITE_CHECKSUM_LENGTH    8

typedef enum{
    HEADER_BUILD_STAGE_1_E,/* set most fields , set the checksum field to 0 */
    HEADER_BUILD_STAGE_2_E,/* set the length that include the payload*/
    HEADER_BUILD_STAGE_3_E /* set the checksum field */
}HEADER_BUILD_STAGE_ENT;

/* get field from MLL entry according to index in snetCh3MllEntryFormat[] */
#define CHT3_MLL_FIELD_VALUE_GET_MAC(_mllEntryPtr,_index)  \
    snetFieldValueGet(_mllEntryPtr,                       \
        snetCh3MllEntryFormat[_index].startBit,           \
        (snetCh3MllEntryFormat[_index].lastBit - snetCh3MllEntryFormat[_index].startBit + 1))

/* the fields of the MLL in CH3+ */
const  HW_FIELD snetCh3MllEntryFormat[]=
{
    {  0, 0},  /*Last0*/                              /*SNET_CHT3_MLL_FIELDS_LAST_0_E                                 */
    {  1, 1},  /*StartOfTunnel0*/                     /*SNET_CHT3_MLL_FIELDS_START_OF_TUNNEL_0_E                      */
    {  3,14},  /*MLLVID0*/                            /*SNET_CHT3_MLL_FIELDS_MLL_VID_0_E                              */
    { 15,15},  /*Use_Vidx0*/                          /*SNET_CHT3_MLL_FIELDS_USE_VIDX_0_E                             */
    { 16,16},  /*TargetIsTrunk0*/                     /*SNET_CHT3_MLL_FIELDS_TARGET_IS_TRUNK_0_E                      */
    { 17,22},  /*TrgPort0*/                           /*SNET_CHT3_MLL_FIELDS_TRG_PORT_0_E                             */
    { 21,27},  /*TrgTrunkId0*/                        /*SNET_CHT3_MLL_FIELDS_TRG_TRUNK_ID_0_E                         */
    { 23,27},  /*TrgDev0*/                            /*SNET_CHT3_MLL_FIELDS_TRG_DEV_0_E                              */
    { 16,28},  /*VIDX0*/                              /*SNET_CHT3_MLL_FIELDS_VIDX_0_E                                 */
    { 29,31},  /*MLLRPFFailCMD0*/                     /*SNET_CHT3_MLL_FIELDS_MLL_RPF_FAIL_CMD_0_E                     */
    { 32,41},  /*TunnelPtr0[9:0]*/                    /*SNET_CHT3_MLL_FIELDS_TUNNEL_PTR_LSB_0_E                       */
    { 42,42},  /*TunnelType0*/                        /*SNET_CHT3_MLL_FIELDS_TUNNEL_TYPE_0_E                          */
    { 43,50},  /*TTLThreshold0/HopLimitThreshold0 */  /*SNET_CHT3_MLL_FIELDS_TTL_THRESHOLD_0_HOP_LIMIT_THRESHOLD_0_E  */
    { 51,51},  /*EcludeSrcVlan0*/                     /*SNET_CHT3_MLL_FIELDS_EXCLUDE_SRC_VLAN_0_E                     */
    { 52,53},  /*TunnelPtr0[11:10]*/                  /*SNET_CHT3_MLL_FIELDS_TUNNEL_PTR_MSB_0_E                       */
    { 54,55},  /*TunnelPtr1[11:10]*/                  /*SNET_CHT3_MLL_FIELDS_TUNNEL_PTR_MSB_1_E                       */
    { 64,64},  /*Last1*/                              /*SNET_CHT3_MLL_FIELDS_LAST_1_E                                 */
    { 65,65},  /*StartOfTunnel1*/                     /*SNET_CHT3_MLL_FIELDS_START_OF_TUNNEL_1_E                      */
    { 67,78},  /*MLLVID1*/                            /*SNET_CHT3_MLL_FIELDS_MLL_VID_1_E                              */
    { 79,79},  /*Use_Vidx1*/                          /*SNET_CHT3_MLL_FIELDS_USE_VIDX_1_E                             */
    { 80,80},  /*TargetIsTrunk1*/                     /*SNET_CHT3_MLL_FIELDS_TARGET_IS_TRUNK_1_E                      */
    { 81,86},  /*TrgPort1*/                           /*SNET_CHT3_MLL_FIELDS_TRG_PORT_1_E                             */
    { 85,91},  /*TrgTrunkId1*/                        /*SNET_CHT3_MLL_FIELDS_TRG_TRUNK_ID_1_E                         */
    { 87,91},  /*TrgDev1*/                            /*SNET_CHT3_MLL_FIELDS_TRG_DEV_1_E                              */
    { 80,92},  /*VIDX1*/                              /*SNET_CHT3_MLL_FIELDS_VIDX_1_E                                 */
    { 93,95},  /*MLLRPFFailCMD1*/                     /*SNET_CHT3_MLL_FIELDS_MLL_RPF_FAIL_CMD_1_E                     */
    { 96,105}, /*TunnelPtr1[9:0]*/                    /*SNET_CHT3_MLL_FIELDS_TUNNEL_PTR_LSB_1_E                       */
    {106,106}, /*TunnelType1*/                        /*SNET_CHT3_MLL_FIELDS_TUNNEL_TYPE_1_E                          */
    {107,114}, /*TTLThreshold0/HopLimitThreshold1*/   /*SNET_CHT3_MLL_FIELDS_TTL_THRESHOLD_1_HOP_LIMIT_THRESHOLD_1_E  */
    {115,115}, /*EcludeSrcVlan1*/                     /*SNET_CHT3_MLL_FIELDS_EXCLUDE_SRC_VLAN_1_E                     */
    {116,127}, /*NextMLLPtr*/                         /*SNET_CHT3_MLL_FIELDS_NEXT_MLL_PTR_E                           */

/* Added for WLAN bridging -- start */

    { 56,56},  /*unregBcFilterring0  */               /*SNET_CHT3_MLL_FIELDS_UNREG_BC_FILTERING_0_E                   */
    { 57,57},  /*unregMcFilterring0  */               /*SNET_CHT3_MLL_FIELDS_UNREG_MC_FILTERING_0_E                   */
    { 58,58},  /*unknownUcFilterring0*/               /*SNET_CHT3_MLL_FIELDS_UNKNOWN_UC_FILTERING_0_E                 */
    { 59,59},  /*vlanEgressTagMode0  */               /*SNET_CHT3_MLL_FIELDS_VLAN_EGRESS_TAG_MODE_0_E                 */

    { 60,60},  /*unregBcFilterring1  */               /*SNET_CHT3_MLL_FIELDS_UNREG_BC_FILTERING_1_E                   */
    { 61,61},  /*unregMcFilterring1  */               /*SNET_CHT3_MLL_FIELDS_UNREG_MC_FILTERING_1_E                   */
    { 62,62},  /*unknownUcFilterring1*/               /*SNET_CHT3_MLL_FIELDS_UNKNOWN_UC_FILTERING_1_E                 */
    { 63,63},  /*vlanEgressTagMode1  */               /*SNET_CHT3_MLL_FIELDS_VLAN_EGRESS_TAG_MODE_1_E                 */

/* Added for WLAN bridging -- end */

    { 0,0} /* dummy */
};

/********************************************************/
/* the entries contain 6 words but aligned on 8 words ! */
/********************************************************/
#define WORD(x)   ((((x)/6)*8 + ((x)%6)) * 32)

/* the fields of the tunnel start in the CAPWAP over IPV4/6 format */
const HW_FIELD snetCh3TsCapwapEntry[]=
{
    /* word 0 */
    {WORD(0) +   0,  WORD(0) +   1},/*TunnelType*/
    {WORD(0) +   3,  WORD(0) +   3},/*upMarkingMode*/
    {WORD(0) +   4,  WORD(0) +   6},/*up*/
    {WORD(0) +   7,  WORD(0) +   7},/*tagEnable*/
    {WORD(0) +   8,  WORD(0) +  19},/*vid*/
    {WORD(0) +  20,  WORD(0) +  27},/*ttl or hot limit */
    {WORD(0) +  28,  WORD(0) +  28},/*capwap t bit */
    {WORD(0) +  29,  WORD(0) +  29},/*capwap w bit */
    {WORD(0) +  30,  WORD(0) +  30},/*capwap m bit */
    {WORD(0) +  31,  WORD(0) +  31},/*egress OSM redirect*/

    /* word 1 */
    {WORD(1) +   0,  WORD(1) +  31},/*nextHopMacDa[0..31]*/

    /* word 2 */
    {WORD(2) +   0,  WORD(2) +  15},/*nextHopMacDa[32..47]*/
    {WORD(2) +  16,  WORD(2) +  31},/*WLAN ID bitmap*/

    /* word 3 */
    {WORD(3) +   0,  WORD(3) +   5},/*dscp*/
    {WORD(3) +   6,  WORD(3) +   6},/*dscpMarkingMode*/
    {WORD(3) +   7,  WORD(3) +   7},/*dontFragmentFlag*/
    {WORD(3) +   8,  WORD(3) +  11},/*capwap version*/
    {WORD(3) +  12,  WORD(3) +  16},/*capwap RID*/
    {WORD(3) +  17,  WORD(3) +  23},/*capwap flags*/
    {WORD(3) +  24,  WORD(3) +  25},/*capwap WBID*/
    {WORD(3) +  26,  WORD(3) +  26},/*enable 802.11 WDS*/
    {WORD(3) +  27,  WORD(3) +  27},/*default 802.11e enable*/
    {WORD(3) +  28,  WORD(3) +  29},/*802.11e mapping profile*/


/***************/
/* IPV4 fields */
/***************/

    /* word 4 */
    {WORD(4) +   0,  WORD(4) +  31},/*ipv4 dip*/

    /* word 5 */
    {WORD(5) +   0,  WORD(5) +  31},/*ipv4 sip*/

    /* word 6 */
    {WORD(6) +   0,  WORD(6) +  15},/*udp src port*/
    {WORD(6) +  16,  WORD(6) +  31},/*udp dst port*/

    /* word 7 */
    {WORD(7) +   0,  WORD(7) +  31},/*bssidOrTa[0..31]*/

    /* word 8 */
    {WORD(8) +   0,  WORD(8) +  15},/*bssidOrTa[32..47]*/
    {WORD(8) +  16,  WORD(8) +  31},/*ra[0..15]*/

    /* word 9 */
    {WORD(9) +   0,  WORD(9) +  31},/*ra[16..47]*/

/***************/
/* IPV6 fields */
/***************/

    /* word 4 */
    {WORD(4) +   0,  WORD(4) +  31},/*ipv6 dip[0..31]*/
    /* word 5 */
    {WORD(5) +   0,  WORD(5) +  31},/*ipv6 dip[32..63]*/
    /* word 6 */
    {WORD(6) +   0,  WORD(6) +  31},/*ipv6 dip[64..95]*/
    /* word 7 */
    {WORD(7) +   0,  WORD(7) +  31},/*ipv6 dip[96..127]*/

    /* word 8 */
    {WORD(8) +   0,  WORD(8) +  31},/*ipv6 sip[0..31]*/
    /* word 9 */
    {WORD(9) +   0,  WORD(9) +  31},/*ipv6 sip[32..63]*/
    /* word 10 */
    {WORD(10) +  0,  WORD(10) +  31},/*ipv6 sip[64..95]*/
    /* word 11 */
    {WORD(11) +  0,  WORD(11) +  31},/*ipv6 sip[96..127]*/


    /* word 12 */
    {WORD(12) +   0,  WORD(12) +  15},/*udp src port*/
    {WORD(12) +  16,  WORD(12) +  31},/*udp dst port*/

    /* word 13 */
    {WORD(13) +   0,  WORD(13) +  31},/*bssidOrTa[0..31]*/

    /* word 14 */
    {WORD(14) +   0,  WORD(14) +  15},/*bssidOrTa[32..47]*/
    {WORD(14) +  16,  WORD(14) +  31},/*ra[0..15]*/

    /* word 15 */
    {WORD(15) +   0,  WORD(15) +  31},/*ra[16..47]*/

    { 0,0} /* dummy */
};

typedef struct{
    GT_U32  tunnelType;
    GT_BIT  upMarkingMode;
    GT_U32  up;
    GT_BIT  tagEnable;
    GT_U32  vid;
    GT_U32  ttlOrHotLimit;
    struct{
        GT_BIT  tBit;
        GT_BIT  wBit;
        GT_BIT  mBit;
        GT_U32  version;
        GT_U32  rid;
        GT_U32  flags;
        GT_U32  wbid;
    }capwap;
    GT_BIT  egressOsmRedirect;
    GT_U8   nextHopMacDa[6];
    GT_U32  wlanIdBitmap;
    GT_U32  dscp;
    GT_BIT  dscpMarkingMode;
    GT_BIT  dontFragmentFlag;
    GT_BIT  enable802dot11wds;
    struct{
        GT_U8   bssidOrTa[6];
        GT_U8   ra[6];
    }wds; /* relevant when enable802dot11wds = 1 */
    GT_BIT  default802dot11eEnable;
    GT_U32  _802dot11eMappingProfile;

    GT_U8   dip[16];/* when ipv4 use bytes 0..3 , when ipv6 use bytes 0..15 */
    GT_U8   sip[16];/* when ipv4 use bytes 0..3 , when ipv6 use bytes 0..15 */
    GT_U32  udpSrcPort;
    GT_U32  udpDstPort;
}SNET_CHT3_TS_CAPWAP_STC;

/*
    enum :  CHT3_TS_CAPWAP_802_11_MAC_HEADER_ENT

    description : enum of the fields needed to build the 802.11 MAC header frame

*/
typedef enum{
    CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_PROTOCOL_E,
    CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_TYPE_E,
    CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_SUBTYPE_E,
    CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_TO_DS_E,
    CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_FROM_DS_E,
    CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_MORE_FRAG_E,
    CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_RETRY_E,
    CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_PWR_MGT_E,
    CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_MORE_DATA_E,
    CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_WEP_E,
    CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_ORDER_E,
    CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_DURATION_E,
    CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_ADDRESS_1_E,
    CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_ADDRESS_2_E,
    CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_ADDRESS_3_E,
    CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_SEQUENCE_CONTROL_E,
    CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_ADDRESS_4_E,
    CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_QOS_CONTROL_0_3_E,
    CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_QOS_CONTROL_4_E,
    CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_QOS_CONTROL_5_6_E,
    CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_QOS_CONTROL_7_15_E,
    CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_LLC_DSAP_SSAP_CONTROL_E,
    CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_LLC_ETHER_TYPE_E,

    CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_LAST_E /* must be last */
}CHT3_TS_CAPWAP_802_11_MAC_HEADER_ENT;

/* array that holds the info about the fields of:
   802.11 MAC header -- needed when Tunnel start (TS) build a frame

   -- I will build the header of the frame in the same manner that PCL key is
   built.
*/
static CHT_PCL_KEY_FIELDS_INFO_STC cht3TsCapwap802dot11MacHeaderFields[CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_LAST_E+1]=
{
    {   0,      1,  GT_FALSE,   " CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_PROTOCOL_E "},
    {   2,      3,  GT_FALSE,   " CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_TYPE_E "},
    {   4,      7,  GT_FALSE,   " CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_SUBTYPE_E "},
    {   8,      8,  GT_FALSE,   " CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_TO_DS_E "},
    {   9,      9,  GT_FALSE,   " CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_FROM_DS_E "},
    {  10,     10,  GT_FALSE,   " CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_MORE_FRAG_E "},
    {  11,     11,  GT_FALSE,   " CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_RETRY_E "},
    {  12,     12,  GT_FALSE,   " CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_PWR_MGT_E "},
    {  13,     13,  GT_FALSE,   " CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_MORE_DATA_E "},
    {  14,     14,  GT_FALSE,   " CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_WEP_E "},
    {  15,     15,  GT_FALSE,   " CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_ORDER_E "},
    {  16,     31,  GT_FALSE,   " CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_DURATION_E "},
    {  32,     79,  GT_FALSE,   " CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_ADDRESS_1_E "},
    {  80,    127,  GT_FALSE,   " CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_ADDRESS_2_E "},
    { 128,    175,  GT_FALSE,   " CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_ADDRESS_3_E "},
    { 176,    191,  GT_FALSE,   " CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_SEQUENCE_CONTROL_E "},

    /* this field optional */
    { 192,    239,  GT_FALSE,   " CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_ADDRESS_4_E "},

    /* next fields optional - 802.11e QoS Control */
    { 240,    243,  GT_FALSE,   " CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_QOS_CONTROL_0_3_E "},
    { 244,    244,  GT_FALSE,   " CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_QOS_CONTROL_4_E "},
    { 245,    246,  GT_FALSE,   " CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_QOS_CONTROL_5_6_E "},
    { 247,    255,  GT_FALSE,   " CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_QOS_CONTROL_7_15_E "},

    /* next fields are mandatory - 802.2 LLC header */
    { 256,    303,  GT_FALSE,   " CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_LLC_DSAP_SSAP_CONTROL_E "},
    { 304,    319,  GT_FALSE,   " CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_LLC_ETHER_TYPE_E "},

    /* the next field to put on the frame if the "payload" of the frame ... */

    /* dummy */
    {0, 0,  GT_FALSE,   " ------ "}
};

/*
    enum :  CHT3_TS_CAPWAP_HEADER_ENT

    description : enum of the fields needed to build the CAPWAP header

*/
typedef enum{
    CHT3_TS_CAPWAP_HEADER_FIELD_PREAMBLE_VERSION_E,
    CHT3_TS_CAPWAP_HEADER_FIELD_PREAMBLE_TYPE_E,
    CHT3_TS_CAPWAP_HEADER_FIELD_PREAMBLE_RESERVED_E,

    CHT3_TS_CAPWAP_HEADER_FIELD_VERSION_E,
    CHT3_TS_CAPWAP_HEADER_FIELD_RID_E,
    CHT3_TS_CAPWAP_HEADER_FIELD_HLEN_E,
    CHT3_TS_CAPWAP_HEADER_FIELD_WBID_E,
    CHT3_TS_CAPWAP_HEADER_FIELD_T_BIT_E,
    CHT3_TS_CAPWAP_HEADER_FIELD_F_BIT_E,
    CHT3_TS_CAPWAP_HEADER_FIELD_L_BIT_E,
    CHT3_TS_CAPWAP_HEADER_FIELD_W_BIT_E,
    CHT3_TS_CAPWAP_HEADER_FIELD_M_BIT_E,
    CHT3_TS_CAPWAP_HEADER_FIELD_K_BIT_E,
    CHT3_TS_CAPWAP_HEADER_FIELD_FLAGS_E,
    CHT3_TS_CAPWAP_HEADER_FIELD_FRAGMENT_ID_E,
    CHT3_TS_CAPWAP_HEADER_FIELD_FRAGMENT_OFFSET_E,
    CHT3_TS_CAPWAP_HEADER_FIELD_RESERVED_1_E,
    CHT3_TS_CAPWAP_HEADER_FIELD_WIRELESS_ID_E,
    CHT3_TS_CAPWAP_HEADER_FIELD_LENGTH_E,
    CHT3_TS_CAPWAP_HEADER_FIELD_WLAN_ID_BITMAP_E,
    CHT3_TS_CAPWAP_HEADER_FIELD_RESERVED_2_E,
    CHT3_TS_CAPWAP_HEADER_FIELD_PADDING_FOR_4_BYTES_ALIGN_E,


    CHT3_TS_CAPWAP_HEADER_FIELD_LAST_E /* must be last */
}CHT3_TS_CAPWAP_HEADER_ENT;
/* array that holds the info about the fields of:
   CAPWAP header (include the CAPWAP preamble header) --
    needed when Tunnel start (TS) build a frame

   -- I will build the header of the frame in the same manner that PCL key is
   built.
*/
static CHT_PCL_KEY_FIELDS_INFO_STC cht3TsCapwapHeaderFields[CHT3_TS_CAPWAP_HEADER_FIELD_LAST_E+1]=
{
    /* preamble */
    {   0,      3,  GT_FALSE,   " CHT3_TS_CAPWAP_HEADER_FIELD_PREAMBLE_VERSION_E "},
    {   4,      7,  GT_FALSE,   " CHT3_TS_CAPWAP_HEADER_FIELD_PREAMBLE_TYPE_E "},
    {   8,     31,  GT_FALSE,   " CHT3_TS_CAPWAP_HEADER_FIELD_PREAMBLE_RESERVED_E "},

    /* actual header */
    {  32,     35,  GT_FALSE,   " CHT3_TS_CAPWAP_HEADER_FIELD_VERSION_E "},
    {  36,     40,  GT_FALSE,   " CHT3_TS_CAPWAP_HEADER_FIELD_RID_E "},
    {  41,     45,  GT_FALSE,   " CHT3_TS_CAPWAP_HEADER_FIELD_HLEN_E "},
    {  46,     50,  GT_FALSE,   " CHT3_TS_CAPWAP_HEADER_FIELD_WBID_E "},
    {  51,     51,  GT_FALSE,   " CHT3_TS_CAPWAP_HEADER_FIELD_T_BIT_E "},
    {  52,     52,  GT_FALSE,   " CHT3_TS_CAPWAP_HEADER_FIELD_F_BIT_E "},
    {  53,     53,  GT_FALSE,   " CHT3_TS_CAPWAP_HEADER_FIELD_L_BIT_E "},
    {  54,     54,  GT_FALSE,   " CHT3_TS_CAPWAP_HEADER_FIELD_W_BIT_E "},
    {  55,     55,  GT_FALSE,   " CHT3_TS_CAPWAP_HEADER_FIELD_M_BIT_E "},
    {  56,     56,  GT_FALSE,   " CHT3_TS_CAPWAP_HEADER_FIELD_K_BIT_E "},
    {  57,     63,  GT_FALSE,   " CHT3_TS_CAPWAP_HEADER_FIELD_FLAGS_E "},

    {  64,     79,  GT_FALSE,   " CHT3_TS_CAPWAP_HEADER_FIELD_FRAGMENT_ID_E "},
    {  80,     92,  GT_FALSE,   " CHT3_TS_CAPWAP_HEADER_FIELD_FRAGMENT_OFFSET_E "},
    {  93,     95,  GT_FALSE,   " CHT3_TS_CAPWAP_HEADER_FIELD_RESERVED_1_E "},

    /* optional fields - Wireless Specific Information */
    {  96,    103,  GT_FALSE,   " CHT3_TS_CAPWAP_HEADER_FIELD_WIRELESS_ID_E "},
    { 104,    111,  GT_FALSE,   " CHT3_TS_CAPWAP_HEADER_FIELD_LENGTH_E "},
    { 112,    127,  GT_FALSE,   " CHT3_TS_CAPWAP_HEADER_FIELD_WLAN_ID_BITMAP_E "},
    { 128,    143,  GT_FALSE,   " CHT3_TS_CAPWAP_HEADER_FIELD_RESERVED_2_E "},
    { 144,    159,  GT_FALSE,   " CHT3_TS_CAPWAP_HEADER_FIELD_PADDING_FOR_4_BYTES_ALIGN_E "},


    /* dummy */
    {0, 0,  GT_FALSE,   " ------ "}
};

/*
    enum :  CHT3_TS_CAPWAP_UDP_TUNNEL_HEADER_ENT

    description : enum of the fields needed to build the UDP tunnel CAPWAP header

*/
typedef enum{
    CHT3_TS_CAPWAP_UDP_TUNNEL_HEADER_FIELD_UDP_SRC_PORT_E,
    CHT3_TS_CAPWAP_UDP_TUNNEL_HEADER_FIELD_UDP_DST_PORT_E,

    CHT3_TS_CAPWAP_UDP_TUNNEL_HEADER_FIELD_UDP_LENGTH_E,
    CHT3_TS_CAPWAP_UDP_TUNNEL_HEADER_FIELD_UDP_CHECKSUM_E,

    CHT3_TS_CAPWAP_UDP_TUNNEL_HEADER_FIELD_UDP_LITE_CHECKSUM_COVERAGE_E,
    CHT3_TS_CAPWAP_UDP_TUNNEL_HEADER_FIELD_UDP_LITE_CHECKSUM_E,

    CHT3_TS_CAPWAP_UDP_TUNNEL_HEADER_FIELD_LAST_E /* must be last */
}CHT3_TS_CAPWAP_UDP_TUNNEL_HEADER_ENT;
/* array that holds the info about the fields of:
   UDP header that comes before the CAPWAP header --
    needed when Tunnel start (TS) build a frame

   -- I will build the header of the frame in the same manner that PCL key is
   built.
*/
static CHT_PCL_KEY_FIELDS_INFO_STC cht3TsCapwapUdpTunnelHeaderFields[CHT3_TS_CAPWAP_UDP_TUNNEL_HEADER_FIELD_LAST_E+1]=
{
    {   0,    15,  GT_FALSE,   " CHT3_TS_CAPWAP_UDP_TUNNEL_HEADER_FIELD_UDP_SRC_PORT_E "},
    {  16,    31,  GT_FALSE,   " CHT3_TS_CAPWAP_UDP_TUNNEL_HEADER_FIELD_UDP_DST_PORT_E "},

    /* udp format */
    {  32,    47,  GT_FALSE,   " CHT3_TS_CAPWAP_UDP_TUNNEL_HEADER_FIELD_UDP_LENGTH_E "},
    {  48,    63,  GT_FALSE,   " CHT3_TS_CAPWAP_UDP_TUNNEL_HEADER_FIELD_UDP_CHECKSUM_E "},

    /* udp-lite format */
    {  32,    47,  GT_FALSE,   " CHT3_TS_CAPWAP_UDP_TUNNEL_HEADER_FIELD_UDP_LITE_CHECKSUM_COVERAGE_E "},
    {  48,    63,  GT_FALSE,   " CHT3_TS_CAPWAP_UDP_TUNNEL_HEADER_FIELD_UDP_LITE_CHECKSUM_E "},

    /* dummy */
    {0, 0,  GT_FALSE,   " ------ "}
};

/*
    enum :  CHT3_TS_CAPWAP_IPV4_HEADER_ENT

    description : enum of the fields needed to build the IPv4 tunnel CAPWAP header

*/
typedef enum{
    CHT3_TS_CAPWAP_IPV4_HEADER_FIELD_VERSION_E,
    CHT3_TS_CAPWAP_IPV4_HEADER_FIELD_IHL_E,
    CHT3_TS_CAPWAP_IPV4_HEADER_FIELD_TOS_E,
    CHT3_TS_CAPWAP_IPV4_HEADER_FIELD_TOTAL_LEN_E,
    CHT3_TS_CAPWAP_IPV4_HEADER_FIELD_IDENTIFICATION_E,
    CHT3_TS_CAPWAP_IPV4_HEADER_FIELD_RESERVED_E,
    CHT3_TS_CAPWAP_IPV4_HEADER_FIELD_DONT_FRAG_E,
    CHT3_TS_CAPWAP_IPV4_HEADER_FIELD_MORE_FRAG_E,
    CHT3_TS_CAPWAP_IPV4_HEADER_FIELD_FRAG_OFFSET_E,
    CHT3_TS_CAPWAP_IPV4_HEADER_FIELD_TTL_E,
    CHT3_TS_CAPWAP_IPV4_HEADER_FIELD_PROTOCOL_E,
    CHT3_TS_CAPWAP_IPV4_HEADER_FIELD_HEADER_CHECKSUM_E,
    CHT3_TS_CAPWAP_IPV4_HEADER_FIELD_SIP_E,
    CHT3_TS_CAPWAP_IPV4_HEADER_FIELD_DIP_E,

    CHT3_TS_CAPWAP_IPV4_HEADER_FIELD_LAST_E /* must be last */
}CHT3_TS_CAPWAP_IPV4_HEADER_ENT;
/* array that holds the info about the fields of:
   IPv4 header that comes before the UDP of the CAPWAP header --
    needed when Tunnel start (TS) build a frame

   -- I will build the header of the frame in the same manner that PCL key is
   built.
*/
static CHT_PCL_KEY_FIELDS_INFO_STC cht3TsCapwapIpv4HeaderFields[CHT3_TS_CAPWAP_IPV4_HEADER_FIELD_LAST_E+1]=
{
    {   0,    3,  GT_FALSE,   " CHT3_TS_CAPWAP_IPV4_HEADER_FIELD_VERSION_E "},
    {   4,    7,  GT_FALSE,   " CHT3_TS_CAPWAP_IPV4_HEADER_FIELD_IHL_E "},
    {   8,   15,  GT_FALSE,   " CHT3_TS_CAPWAP_IPV4_HEADER_FIELD_TOS_E "},
    {  16,   31,  GT_FALSE,   " CHT3_TS_CAPWAP_IPV4_HEADER_FIELD_TOTAL_LEN_E "},
    {  32,   47,  GT_FALSE,   " CHT3_TS_CAPWAP_IPV4_HEADER_FIELD_IDENTIFICATION_E "},
    {  48,   48,  GT_FALSE,   " CHT3_TS_CAPWAP_IPV4_HEADER_FIELD_RESERVED_E "},
    {  49,   49,  GT_FALSE,   " CHT3_TS_CAPWAP_IPV4_HEADER_FIELD_DONT_FRAG_E "},
    {  50,   50,  GT_FALSE,   " CHT3_TS_CAPWAP_IPV4_HEADER_FIELD_MORE_FRAG_E "},
    {  51,   63,  GT_FALSE,   " CHT3_TS_CAPWAP_IPV4_HEADER_FIELD_FRAG_OFFSET_E "},
    {  64,   71,  GT_FALSE,   " CHT3_TS_CAPWAP_IPV4_HEADER_FIELD_TTL_E "},
    {  72,   79,  GT_FALSE,   " CHT3_TS_CAPWAP_IPV4_HEADER_FIELD_PROTOCOL_E "},
    {  80,   95,  GT_FALSE,   " CHT3_TS_CAPWAP_IPV4_HEADER_FIELD_HEADER_CHECKSUM_E "},
    {  96,  127,  GT_FALSE,   " CHT3_TS_CAPWAP_IPV4_HEADER_FIELD_SIP_E "},
    { 128,  159,  GT_FALSE,   " CHT3_TS_CAPWAP_IPV4_HEADER_FIELD_DIP_E "},

    /* dummy */
    {0, 0,  GT_FALSE,   " ------ "}
};

/*
    enum :  CHT3_TS_CAPWAP_IPV6_HEADER_ENT

    description : enum of the fields needed to build the IPv6 tunnel CAPWAP header

*/
typedef enum{
    CHT3_TS_CAPWAP_IPV6_HEADER_FIELD_VERSION_E,
    CHT3_TS_CAPWAP_IPV6_HEADER_FIELD_TRAFFIC_CLASS_E,
    CHT3_TS_CAPWAP_IPV6_HEADER_FIELD_FLOW_E,
    CHT3_TS_CAPWAP_IPV6_HEADER_FIELD_PAYLOAD_LEN_E,
    CHT3_TS_CAPWAP_IPV6_HEADER_FIELD_NEXT_HEADER_E,
    CHT3_TS_CAPWAP_IPV6_HEADER_FIELD_HOP_LIMIT_E,
    CHT3_TS_CAPWAP_IPV6_HEADER_FIELD_SIP_E,
    CHT3_TS_CAPWAP_IPV6_HEADER_FIELD_DIP_E,

    CHT3_TS_CAPWAP_IPV6_HEADER_FIELD_LAST_E /* must be last */
}CHT3_TS_CAPWAP_IPV6_HEADER_ENT;
/* array that holds the info about the fields of:
   IPv6 header that comes before the UDP of the CAPWAP header --
    needed when Tunnel start (TS) build a frame

   -- I will build the header of the frame in the same manner that PCL key is
   built.
*/
static CHT_PCL_KEY_FIELDS_INFO_STC cht3TsCapwapIpv6HeaderFields[CHT3_TS_CAPWAP_IPV6_HEADER_FIELD_LAST_E+1]=
{
    {   0,    3,  GT_FALSE,   " CHT3_TS_CAPWAP_IPV6_HEADER_FIELD_VERSION_E "},
    {   4,   11,  GT_FALSE,   " CHT3_TS_CAPWAP_IPV6_HEADER_FIELD_TRAFFIC_CLASS_E "},
    {  12,   31,  GT_FALSE,   " CHT3_TS_CAPWAP_IPV6_HEADER_FIELD_FLOW_E "},
    {  32,   47,  GT_FALSE,   " CHT3_TS_CAPWAP_IPV6_HEADER_FIELD_PAYLOAD_LEN_E "},
    {  48,   55,  GT_FALSE,   " CHT3_TS_CAPWAP_IPV6_HEADER_FIELD_NEXT_HEADER_E "},
    {  56,   63,  GT_FALSE,   " CHT3_TS_CAPWAP_IPV6_HEADER_FIELD_HOP_LIMIT_E "},
    {  64,  191,  GT_FALSE,   " CHT3_TS_CAPWAP_IPV6_HEADER_FIELD_SIP_E "},
    { 192,  319,  GT_FALSE,   " CHT3_TS_CAPWAP_IPV6_HEADER_FIELD_DIP_E "},

    /* dummy */
    {0, 0,  GT_FALSE,   " ------ "}
};

/*
    enum :  CHT3_TS_CAPWAP_IPV6_HEADER_ENT

    description : enum of the fields needed to build the Ethernet tunnel CAPWAP header

*/
typedef enum{
    CHT3_TS_CAPWAP_ETHERNET_HEADER_FIELD_DST_MAC_E,
    CHT3_TS_CAPWAP_ETHERNET_HEADER_FIELD_SRC_MAC_E,
    CHT3_TS_CAPWAP_ETHERNET_HEADER_FIELD_ETHER_TYPE_E,
    CHT3_TS_CAPWAP_ETHERNET_HEADER_FIELD_VPT_E,
    CHT3_TS_CAPWAP_ETHERNET_HEADER_FIELD_CFI_E,
    CHT3_TS_CAPWAP_ETHERNET_HEADER_FIELD_VID_E,
    CHT3_TS_CAPWAP_ETHERNET_HEADER_FIELD_ETHER_TYPE_2_E,

    CHT3_TS_CAPWAP_ETHERNET_HEADER_FIELD_LAST_E /* must be last */
}CHT3_TS_CAPWAP_ETHERNET_HEADER_ENT;
/* array that holds the info about the fields of:
   IPv6 header that comes before the UDP of the CAPWAP header --
    needed when Tunnel start (TS) build a frame

   -- I will build the header of the frame in the same manner that PCL key is
   built.
*/
static CHT_PCL_KEY_FIELDS_INFO_STC cht3TsCapwapEthernetHeaderFields[CHT3_TS_CAPWAP_ETHERNET_HEADER_FIELD_LAST_E+1]=
{
    {   0,    47,  GT_FALSE,   " CHT3_TS_CAPWAP_ETHERNET_HEADER_FIELD_DST_MAC_E "},
    {  48,    95,  GT_FALSE,   " CHT3_TS_CAPWAP_ETHERNET_HEADER_FIELD_SRC_MAC_E "},
    {  96,   111,  GT_FALSE,   " CHT3_TS_CAPWAP_ETHERNET_HEADER_FIELD_ETHER_TYPE_E "},
    { 112,   114,  GT_FALSE,   " CHT3_TS_CAPWAP_ETHERNET_HEADER_FIELD_VPT_E "},
    { 115,   115,  GT_FALSE,   " CHT3_TS_CAPWAP_ETHERNET_HEADER_FIELD_CFI_E "},
    { 116,   127,  GT_FALSE,   " CHT3_TS_CAPWAP_ETHERNET_HEADER_FIELD_VID_E"},
    { 128,   143,  GT_FALSE,   " CHT3_TS_CAPWAP_ETHERNET_HEADER_FIELD_ETHER_TYPE_2_E "},

    /* dummy */
    {0, 0,  GT_FALSE,   " ------ "}
};


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
)
{
    /* the UDP use the same checksum as the IPv4 */
    return (GT_U16)ipV4CheckSumCalc(pMsg,lMsg);
}


/*******************************************************************************
*   lion3ReadMllEntrySection
*
* DESCRIPTION:
*         Lion3 : Read 1/2 Multicast Link List entry .
*               when read second half , also get the 'next pointer'
* INPUTS:
*        regAddress - MLL memory address.
*        mllIndex   - MLL index (of the line) in the memory
*       selectorIndex - selector index (0 or 1) to define which half to read
* OUTPUTS:
*        mllPtr  - pointer to the MLL entry.
*
*
*******************************************************************************/
static void lion3ReadMllEntrySection
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32                  mllIndex,
    IN GT_U32                  selectorIndex,
    OUT SNET_CHT3_DOUBLE_MLL_STC *mllPtr
)
{
    SNET_CHT3_SINGLE_MLL_STC *singleMllPtr;
    GT_U32  tmpVal;
    GT_U32  *mllEntryPtr;

    mllEntryPtr = smemMemGet(devObjPtr,
        SMEM_CHT3_ROUTER_MULTICAST_LIST_TBL_MEM(devObjPtr,mllIndex));

    singleMllPtr = selectorIndex ?
                        &mllPtr->second_mll :
                        &mllPtr->first_mll;

    singleMllPtr->rpf_fail_cmd =
        SMEM_LION3_IP_MLL_ENTRY_FIELD_GET(devObjPtr,mllEntryPtr,mllIndex,
            selectorIndex ?
            SMEM_LION3_IP_MLL_TABLE_FIELDS_MLL_RPF_FAIL_CMD_1:
            SMEM_LION3_IP_MLL_TABLE_FIELDS_MLL_RPF_FAIL_CMD_0);
    singleMllPtr->ttlThres =
        SMEM_LION3_IP_MLL_ENTRY_FIELD_GET(devObjPtr,mllEntryPtr,mllIndex,
            selectorIndex ?
            SMEM_LION3_IP_MLL_TABLE_FIELDS_TTL_THRESHOLD_1_OR_HOP_LIMIT_THRESHOLD_1:
            SMEM_LION3_IP_MLL_TABLE_FIELDS_TTL_THRESHOLD_0_OR_HOP_LIMIT_THRESHOLD_0);
    singleMllPtr->excludeSrcVlan =
        SMEM_LION3_IP_MLL_ENTRY_FIELD_GET(devObjPtr,mllEntryPtr,mllIndex,
            selectorIndex ?
            SMEM_LION3_IP_MLL_TABLE_FIELDS_EXCLUDE_SRC_VLAN_1:
            SMEM_LION3_IP_MLL_TABLE_FIELDS_EXCLUDE_SRC_VLAN_0);
    singleMllPtr->last =
        SMEM_LION3_IP_MLL_ENTRY_FIELD_GET(devObjPtr,mllEntryPtr,mllIndex,
            selectorIndex ?
            SMEM_LION3_IP_MLL_TABLE_FIELDS_LAST_1:
            SMEM_LION3_IP_MLL_TABLE_FIELDS_LAST_0);
    singleMllPtr->vid =
        SMEM_LION3_IP_MLL_ENTRY_FIELD_GET(devObjPtr,mllEntryPtr,mllIndex,
            selectorIndex ?
            SMEM_LION3_IP_MLL_TABLE_FIELDS_MLL_EVID_1:
            SMEM_LION3_IP_MLL_TABLE_FIELDS_MLL_EVID_0);
    singleMllPtr->isTunnelStart =
        SMEM_LION3_IP_MLL_ENTRY_FIELD_GET(devObjPtr,mllEntryPtr,mllIndex,
            selectorIndex ?
            SMEM_LION3_IP_MLL_TABLE_FIELDS_START_OF_TUNNEL_1:
            SMEM_LION3_IP_MLL_TABLE_FIELDS_START_OF_TUNNEL_0);
    if(singleMllPtr->isTunnelStart)
    {
        singleMllPtr->tsInfo.tunnelStartType =
            SMEM_LION3_IP_MLL_ENTRY_FIELD_GET(devObjPtr,mllEntryPtr,mllIndex,
                selectorIndex ?
                SMEM_LION3_IP_MLL_TABLE_FIELDS_TUNNEL_TYPE_1:
                SMEM_LION3_IP_MLL_TABLE_FIELDS_TUNNEL_TYPE_0);
        singleMllPtr->tsInfo.tunnelStartPtr =
            SMEM_LION3_IP_MLL_ENTRY_FIELD_GET(devObjPtr,mllEntryPtr,mllIndex,
                selectorIndex ?
                SMEM_LION3_IP_MLL_TABLE_FIELDS_TUNNEL_PTR_1:
                SMEM_LION3_IP_MLL_TABLE_FIELDS_TUNNEL_PTR_0);
    }

    tmpVal =
        SMEM_LION3_IP_MLL_ENTRY_FIELD_GET(devObjPtr,mllEntryPtr,mllIndex,
            selectorIndex ?
            SMEM_LION3_IP_MLL_TABLE_FIELDS_USE_VIDX_1:
            SMEM_LION3_IP_MLL_TABLE_FIELDS_USE_VIDX_0);
    if(tmpVal)/* useVidx */
    {
        singleMllPtr->lll.dstInterface = SNET_DST_INTERFACE_VIDX_E;
        singleMllPtr->lll.interfaceInfo.vidx =
            SMEM_LION3_IP_MLL_ENTRY_FIELD_GET(devObjPtr,mllEntryPtr,mllIndex,
                selectorIndex ?
                SMEM_LION3_IP_MLL_TABLE_FIELDS_EVIDX_1:
                SMEM_LION3_IP_MLL_TABLE_FIELDS_EVIDX_0);
    }
    else
    {
        tmpVal =
            SMEM_LION3_IP_MLL_ENTRY_FIELD_GET(devObjPtr,mllEntryPtr,mllIndex,
                selectorIndex ?
                SMEM_LION3_IP_MLL_TABLE_FIELDS_TARGET_IS_TRUNK_1:
                SMEM_LION3_IP_MLL_TABLE_FIELDS_TARGET_IS_TRUNK_0);
        if(tmpVal)/* isTrunk */
        {
            singleMllPtr->lll.dstInterface = SNET_DST_INTERFACE_TRUNK_E;
            singleMllPtr->lll.interfaceInfo.trunkId =
                SMEM_LION3_IP_MLL_ENTRY_FIELD_GET(devObjPtr,mllEntryPtr,mllIndex,
                    selectorIndex ?
                    SMEM_LION3_IP_MLL_TABLE_FIELDS_TRG_TRUNK_ID_1:
                    SMEM_LION3_IP_MLL_TABLE_FIELDS_TRG_TRUNK_ID_0);
        }
        else
        {
            singleMllPtr->lll.dstInterface = SNET_DST_INTERFACE_PORT_E;
            singleMllPtr->lll.interfaceInfo.devPort.port =
                SMEM_LION3_IP_MLL_ENTRY_FIELD_GET(devObjPtr,mllEntryPtr,mllIndex,
                    selectorIndex ?
                    SMEM_LION3_IP_MLL_TABLE_FIELDS_TRG_EPORT_1:
                    SMEM_LION3_IP_MLL_TABLE_FIELDS_TRG_EPORT_0);
            singleMllPtr->lll.interfaceInfo.devPort.devNum =
                SMEM_LION3_IP_MLL_ENTRY_FIELD_GET(devObjPtr,mllEntryPtr,mllIndex,
                    selectorIndex ?
                    SMEM_LION3_IP_MLL_TABLE_FIELDS_TRG_DEV_1:
                    SMEM_LION3_IP_MLL_TABLE_FIELDS_TRG_DEV_0);
        }
    }

    if(selectorIndex)
    {
        mllPtr->nextPtr =
            SMEM_LION3_IP_MLL_ENTRY_FIELD_GET(devObjPtr,mllEntryPtr,mllIndex,
                SMEM_LION3_IP_MLL_TABLE_FIELDS_NEXT_MLL_PTR);
    }

    return;
}

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
)
{
    SNET_CHT3_MLL_FIELDS_ENT  index;
    SNET_CHT3_SINGLE_MLL_STC *singleMllPtr;
    GT_U32  tmpVal;
    GT_U32  *mllEntryPtr;

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        /* read first half of the entry */
        lion3ReadMllEntrySection(devObjPtr,mllIndex,0,mllPtr);
        /* read second half of the entry .
           this also read the 'next pointer' */
        lion3ReadMllEntrySection(devObjPtr,mllIndex,1,mllPtr);

        return;
    }

    mllEntryPtr = smemMemGet(devObjPtr,
        SMEM_CHT3_ROUTER_MULTICAST_LIST_TBL_MEM(devObjPtr,mllIndex));
    /**************/
    /* first  MLL */
    /**************/
    singleMllPtr = &mllPtr->first_mll;

    index = SNET_CHT3_MLL_FIELDS_MLL_RPF_FAIL_CMD_0_E;
    tmpVal = CHT3_MLL_FIELD_VALUE_GET_MAC(mllEntryPtr,index);
    singleMllPtr->rpf_fail_cmd = tmpVal;

    index = SNET_CHT3_MLL_FIELDS_TTL_THRESHOLD_0_HOP_LIMIT_THRESHOLD_0_E;
    tmpVal = CHT3_MLL_FIELD_VALUE_GET_MAC(mllEntryPtr,index);
    singleMllPtr->ttlThres = tmpVal;

    index = SNET_CHT3_MLL_FIELDS_EXCLUDE_SRC_VLAN_0_E;
    tmpVal = CHT3_MLL_FIELD_VALUE_GET_MAC(mllEntryPtr,index);
    singleMllPtr->excludeSrcVlan = tmpVal;

    index = SNET_CHT3_MLL_FIELDS_LAST_0_E;
    tmpVal = CHT3_MLL_FIELD_VALUE_GET_MAC(mllEntryPtr,index);
    singleMllPtr->last =tmpVal;

    index = SNET_CHT3_MLL_FIELDS_MLL_VID_0_E;
    tmpVal = CHT3_MLL_FIELD_VALUE_GET_MAC(mllEntryPtr,index);
    singleMllPtr->vid =tmpVal;


    index = SNET_CHT3_MLL_FIELDS_START_OF_TUNNEL_0_E;
    tmpVal = CHT3_MLL_FIELD_VALUE_GET_MAC(mllEntryPtr,index);
    singleMllPtr->isTunnelStart =tmpVal;

    if(singleMllPtr->isTunnelStart)
    {
        index = SNET_CHT3_MLL_FIELDS_TUNNEL_TYPE_0_E;
        tmpVal = CHT3_MLL_FIELD_VALUE_GET_MAC(mllEntryPtr,index);
        singleMllPtr->tsInfo.tunnelStartType =tmpVal;

        index = SNET_CHT3_MLL_FIELDS_TUNNEL_PTR_LSB_0_E;
        tmpVal = CHT3_MLL_FIELD_VALUE_GET_MAC(mllEntryPtr,index);
        singleMllPtr->tsInfo.tunnelStartPtr =tmpVal;

        index = SNET_CHT3_MLL_FIELDS_TUNNEL_PTR_MSB_0_E;
        tmpVal = CHT3_MLL_FIELD_VALUE_GET_MAC(mllEntryPtr,index);
        singleMllPtr->tsInfo.tunnelStartPtr |= tmpVal<<10;
    }


    index = SNET_CHT3_MLL_FIELDS_USE_VIDX_0_E;
    tmpVal = CHT3_MLL_FIELD_VALUE_GET_MAC(mllEntryPtr,index);

    if(tmpVal)/* use vidx */
    {
        singleMllPtr->lll.dstInterface = SNET_DST_INTERFACE_VIDX_E;
        index = SNET_CHT3_MLL_FIELDS_VIDX_0_E;
        tmpVal = CHT3_MLL_FIELD_VALUE_GET_MAC(mllEntryPtr,index);
        singleMllPtr->lll.interfaceInfo.vidx =tmpVal;
    }
    else
    {
        index = SNET_CHT3_MLL_FIELDS_TARGET_IS_TRUNK_0_E;
        tmpVal = CHT3_MLL_FIELD_VALUE_GET_MAC(mllEntryPtr,index);

        if(tmpVal)/* isTrunk */
        {
            singleMllPtr->lll.dstInterface = SNET_DST_INTERFACE_TRUNK_E;
            index = SNET_CHT3_MLL_FIELDS_TRG_TRUNK_ID_0_E;
            tmpVal = CHT3_MLL_FIELD_VALUE_GET_MAC(mllEntryPtr,index);
            singleMllPtr->lll.interfaceInfo.trunkId =tmpVal;
        }
        else
        {
            singleMllPtr->lll.dstInterface = SNET_DST_INTERFACE_PORT_E;
            index = SNET_CHT3_MLL_FIELDS_TRG_PORT_0_E;
            tmpVal = CHT3_MLL_FIELD_VALUE_GET_MAC(mllEntryPtr,index);
            singleMllPtr->lll.interfaceInfo.devPort.port =tmpVal;

            index = SNET_CHT3_MLL_FIELDS_TRG_DEV_0_E;
            tmpVal = CHT3_MLL_FIELD_VALUE_GET_MAC(mllEntryPtr,index);
            singleMllPtr->lll.interfaceInfo.devPort.devNum =tmpVal;
        }
    }

    index = SNET_CHT3_MLL_FIELDS_UNREG_BC_FILTERING_0_E;
    tmpVal = CHT3_MLL_FIELD_VALUE_GET_MAC(mllEntryPtr,index);
    singleMllPtr->unregBcFiltering =tmpVal;

    index = SNET_CHT3_MLL_FIELDS_UNREG_MC_FILTERING_0_E;
    tmpVal = CHT3_MLL_FIELD_VALUE_GET_MAC(mllEntryPtr,index);
    singleMllPtr->unregMcFiltering =tmpVal;

    index = SNET_CHT3_MLL_FIELDS_UNKNOWN_UC_FILTERING_0_E;
    tmpVal = CHT3_MLL_FIELD_VALUE_GET_MAC(mllEntryPtr,index);
    singleMllPtr->unknownUcFiltering =tmpVal;

    index = SNET_CHT3_MLL_FIELDS_VLAN_EGRESS_TAG_MODE_0_E;
    tmpVal = CHT3_MLL_FIELD_VALUE_GET_MAC(mllEntryPtr,index);
    singleMllPtr->vlanEgressTagMode =tmpVal;


    /**************/
    /* second MLL */
    /**************/

    singleMllPtr = &mllPtr->second_mll;

    index = SNET_CHT3_MLL_FIELDS_MLL_RPF_FAIL_CMD_1_E;
    tmpVal = CHT3_MLL_FIELD_VALUE_GET_MAC(mllEntryPtr,index);
    singleMllPtr->rpf_fail_cmd =tmpVal;

    index = SNET_CHT3_MLL_FIELDS_TTL_THRESHOLD_1_HOP_LIMIT_THRESHOLD_1_E;
    tmpVal = CHT3_MLL_FIELD_VALUE_GET_MAC(mllEntryPtr,index);
    singleMllPtr->ttlThres =tmpVal;

    index = SNET_CHT3_MLL_FIELDS_EXCLUDE_SRC_VLAN_1_E;
    tmpVal = CHT3_MLL_FIELD_VALUE_GET_MAC(mllEntryPtr,index);
    singleMllPtr->excludeSrcVlan =tmpVal;

    index = SNET_CHT3_MLL_FIELDS_LAST_1_E;/* on the second MLL there is no specific
                                         bit for last , it is implicitly from
                                         the next pointer value */
    tmpVal = CHT3_MLL_FIELD_VALUE_GET_MAC(mllEntryPtr,index);
    singleMllPtr->last =tmpVal;

    index = SNET_CHT3_MLL_FIELDS_MLL_VID_1_E;
    tmpVal = CHT3_MLL_FIELD_VALUE_GET_MAC(mllEntryPtr,index);
    singleMllPtr->vid =tmpVal;

    index = SNET_CHT3_MLL_FIELDS_START_OF_TUNNEL_1_E;
    tmpVal = CHT3_MLL_FIELD_VALUE_GET_MAC(mllEntryPtr,index);
    singleMllPtr->isTunnelStart =tmpVal;

    if(singleMllPtr->isTunnelStart)
    {
        index = SNET_CHT3_MLL_FIELDS_TUNNEL_TYPE_1_E;
        tmpVal = CHT3_MLL_FIELD_VALUE_GET_MAC(mllEntryPtr,index);
        singleMllPtr->tsInfo.tunnelStartType =tmpVal;

        index = SNET_CHT3_MLL_FIELDS_TUNNEL_PTR_LSB_1_E;
        tmpVal = CHT3_MLL_FIELD_VALUE_GET_MAC(mllEntryPtr,index);
        singleMllPtr->tsInfo.tunnelStartPtr =tmpVal;

        index = SNET_CHT3_MLL_FIELDS_TUNNEL_PTR_MSB_1_E;
        tmpVal = CHT3_MLL_FIELD_VALUE_GET_MAC(mllEntryPtr,index);
        singleMllPtr->tsInfo.tunnelStartPtr |= tmpVal<<10;
    }

    index = SNET_CHT3_MLL_FIELDS_USE_VIDX_1_E;
    tmpVal = CHT3_MLL_FIELD_VALUE_GET_MAC(mllEntryPtr,index);

    if(tmpVal)/* use vidx */
    {
        singleMllPtr->lll.dstInterface = SNET_DST_INTERFACE_VIDX_E;
        index = SNET_CHT3_MLL_FIELDS_VIDX_1_E;
        tmpVal = CHT3_MLL_FIELD_VALUE_GET_MAC(mllEntryPtr,index);
        singleMllPtr->lll.interfaceInfo.vidx =tmpVal;
    }
    else
    {
        index = SNET_CHT3_MLL_FIELDS_TARGET_IS_TRUNK_1_E;
        tmpVal = CHT3_MLL_FIELD_VALUE_GET_MAC(mllEntryPtr,index);

        if(tmpVal)/* isTrunk */
        {
            singleMllPtr->lll.dstInterface = SNET_DST_INTERFACE_TRUNK_E;
            index = SNET_CHT3_MLL_FIELDS_TRG_TRUNK_ID_1_E;
            tmpVal = CHT3_MLL_FIELD_VALUE_GET_MAC(mllEntryPtr,index);
            singleMllPtr->lll.interfaceInfo.trunkId =tmpVal;
        }
        else
        {
            singleMllPtr->lll.dstInterface = SNET_DST_INTERFACE_PORT_E;
            index = SNET_CHT3_MLL_FIELDS_TRG_PORT_1_E;
            tmpVal = CHT3_MLL_FIELD_VALUE_GET_MAC(mllEntryPtr,index);
            singleMllPtr->lll.interfaceInfo.devPort.port =tmpVal;

            index = SNET_CHT3_MLL_FIELDS_TRG_DEV_1_E;
            tmpVal = CHT3_MLL_FIELD_VALUE_GET_MAC(mllEntryPtr,index);
            singleMllPtr->lll.interfaceInfo.devPort.devNum =tmpVal;
        }
    }

    index = SNET_CHT3_MLL_FIELDS_UNREG_BC_FILTERING_1_E;
    tmpVal = CHT3_MLL_FIELD_VALUE_GET_MAC(mllEntryPtr,index);
    singleMllPtr->unregBcFiltering =tmpVal;

    index = SNET_CHT3_MLL_FIELDS_UNREG_MC_FILTERING_1_E;
    tmpVal = CHT3_MLL_FIELD_VALUE_GET_MAC(mllEntryPtr,index);
    singleMllPtr->unregMcFiltering =tmpVal;

    index = SNET_CHT3_MLL_FIELDS_UNKNOWN_UC_FILTERING_1_E;
    tmpVal = CHT3_MLL_FIELD_VALUE_GET_MAC(mllEntryPtr,index);
    singleMllPtr->unknownUcFiltering =tmpVal;

    index = SNET_CHT3_MLL_FIELDS_VLAN_EGRESS_TAG_MODE_1_E;
    tmpVal = CHT3_MLL_FIELD_VALUE_GET_MAC(mllEntryPtr,index);
    singleMllPtr->vlanEgressTagMode =tmpVal;


    /********************/
    /* next MLL pointer */
    /********************/
    index = SNET_CHT3_MLL_FIELDS_NEXT_MLL_PTR_E;
    tmpVal = CHT3_MLL_FIELD_VALUE_GET_MAC(mllEntryPtr,index);
    mllPtr->nextPtr =tmpVal;

}


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
)
{
    DECLARE_FUNC_NAME(snetCht3mllCounters);

    GT_U32 regAddr;                 /* Register address                            */
    GT_U32 * regPtr;                /* Register entry pointer                      */
    GT_U32 cntset_port_trunk_mode;  /* port trunk mode for interface bind          */
    GT_U32 cntset_port_trunk;       /* port/trunk value for interface bind              */
    GT_U32 cntset_ip_mode;          /* counting mode interface route entry         */
    GT_U32 cntset_vlan_mode;        /* counting mode interface route entry         */
    GT_U32 cntset_vid;              /* vlan value  for interface bind              */
    GT_U32 cntset_dev;              /* device value  for interface bind            */
    GT_BOOL isCountVid;             /* whether packet's eVid is matching this counter-set             */
    GT_BOOL isCountProtocol;        /* whether packet's protocol is matching this counter-set        */
    GT_BOOL isCountIf;              /* whether packet's out interface is matching this counter-set   */
    GT_U32  ii;                     /* index for MLL */
    GT_U32  value;                  /* register's value*/
    GT_U32  value1;                 /* register's value 1*/


    for(ii = 0 ; ii < 2 ; ii++)
    {
        if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            /* Counter Set  ROUTER_MNG_COUNTER_CONFIGURATION */

            if(isL2Mll == GT_TRUE)
            {
                regAddr = SMEM_LION3_L2_MLL_OUT_INTERFACE_CNTR_CONFIG_REG(devObjPtr,ii);
                smemRegGet(devObjPtr , regAddr, &value);
                regAddr = SMEM_LION3_L2_MLL_OUT_INTERFACE_CNTR_CONFIG_1_REG(devObjPtr,ii);
                smemRegGet(devObjPtr , regAddr, &value1);
            }
            else
            {
                regAddr = SMEM_LION3_IP_MLL_OUT_INTERFACE_CNTR_CONFIG_REG(devObjPtr,ii);
                smemRegGet(devObjPtr , regAddr, &value);
                regAddr = SMEM_LION3_IP_MLL_OUT_INTERFACE_CNTR_CONFIG_1_REG(devObjPtr,ii);
                smemRegGet(devObjPtr , regAddr, &value1);
            }

            cntset_port_trunk_mode = SMEM_U32_GET_FIELD(value, 30 , 2 );
            cntset_ip_mode         = SMEM_U32_GET_FIELD(value, 27 , 2 );
            cntset_vlan_mode       = SMEM_U32_GET_FIELD(value, 26 , 1 );
            cntset_vid             = SMEM_U32_GET_FIELD(value, 10 , 16);

            cntset_dev             = SMEM_U32_GET_FIELD(value1, 20  , 12 );
            cntset_port_trunk      = SMEM_U32_GET_FIELD(value1, 0  , 20 );

            if(cntset_port_trunk_mode == 2)
            {
                /*use only 12 bits*/
                cntset_port_trunk &= 0xFFF;
            }
        }
        else
        {
            /* Counter Set  ROUTER_MNG_COUNTER_CONFIGURATION */
            regAddr = SMEM_CHT3_MLL_OUT_INTERFACE_CNFG_TBL_MEM(devObjPtr,ii);
            smemRegGet(devObjPtr , regAddr, &value);

            cntset_port_trunk_mode = SMEM_U32_GET_FIELD(value, 30 , 2 );
            cntset_ip_mode         = SMEM_U32_GET_FIELD(value, 27 , 2 );
            cntset_vlan_mode       = SMEM_U32_GET_FIELD(value, 26 , 1 );
            cntset_vid             = SMEM_U32_GET_FIELD(value, 14 , 12);
            cntset_dev             = SMEM_U32_GET_FIELD(value, 8  , 5 );
            cntset_port_trunk      = SMEM_U32_GET_FIELD(value, 0  , 8 );
        }


        if(descrPtr->useVidx == 1)
        {
            isCountIf = GT_TRUE;
        }
        else
        if (
            (cntset_port_trunk_mode == 0) ||    /* disregard port/trunk */
                                                /* counts port+dev      */
            ((cntset_port_trunk_mode == 1) && (cntset_dev == descrPtr->trgDev) && (cntset_port_trunk == descrPtr->trgEPort)) ||
                                                /* counts trunk         */
            ((cntset_port_trunk_mode == 2) && (descrPtr->targetIsTrunk) && (cntset_port_trunk == descrPtr->trgTrunkId))
           )
        {
            isCountIf = GT_TRUE;
        }
        else
        {
            isCountIf = GT_FALSE;
        }

        if  ((cntset_vlan_mode == 0 )  ||             /* disregard eVid        */
             (descrPtr->eVid == cntset_vid))              /* counts specific eVid  */

        {
            isCountVid = GT_TRUE;
        }
        else
        {
            isCountVid = GT_FALSE;
        }

        /* [JIRA] (MLL-300) L2 MLL Out Multicast Packets Counter - Do not count non IP packets */
        if((SMEM_CHT_IS_SIP5_GET(devObjPtr)) && (devObjPtr->errata.l2MllOutMcPckCnt) && (isL2Mll==GT_TRUE))
        {
           if ((((descrPtr->isIPv4 == 1) && (descrPtr->isIp == 1)) && (cntset_ip_mode < 2)) ||
               (((descrPtr->isIp == 1) && (descrPtr->isIPv4 == 0)) && ((cntset_ip_mode == 0) || (cntset_ip_mode == 2))))
           {
               isCountProtocol = GT_TRUE;
           }
           else
           {
               isCountProtocol = GT_FALSE;
               __LOG(("Warning (Errata): L2 MLL Out Multicast Packets Counter - Do not count non IP packets although it should \n"));
           }
        }
        else
        {
            if (
                (cntset_ip_mode  == 0) ||               /* disregard protocol   */
                                                        /* counts IPv4 packets  */
                ((cntset_ip_mode   == 1)  && (descrPtr->isIPv4 == 1)) ||
                                                        /* counts IPv6 packets  */
                ((cntset_ip_mode   == 2)  && (descrPtr->isIPv4 == 0))
               )
            {
                isCountProtocol = GT_TRUE;
            }
            else
            {
                isCountProtocol = GT_FALSE;
            }
        }

        /* Address associated with DIP search */
        if ((isCountIf == GT_TRUE) & (isCountProtocol == GT_TRUE)
           && (isCountVid == GT_TRUE))
        {
            if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
            {
                if(isL2Mll == GT_TRUE)
                {
                    __LOG(("L2 MLL increment count on counter set[%d] \n", ii));
                    regAddr = SMEM_LION3_L2_MLL_OUT_MULTICAST_PACKETS_COUNTER_REG(devObjPtr,ii);
                }
                else
                {
                    __LOG(("IP MLL increment count on counter set[%d] \n", ii));
                    regAddr = SMEM_LION3_IP_MLL_OUT_MULTICAST_PACKETS_COUNTER_REG(devObjPtr,ii);
                }
            }
            else
            {
                __LOG(("IP MLL increment count on counter set[%d] \n", ii));
                regAddr = SMEM_CHT3_MLL_OUT_INTERFACE_COUNTER_TBL_MEM(devObjPtr,ii);
            }

            regPtr = smemMemGet(devObjPtr,regAddr);
            (*regPtr)++;
        }
    }

    /*************************** DROP COUNTER REGISTERS *******************/

    if ((descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_HARD_DROP_E) ||
        (descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_SOFT_DROP_E))
    {
        if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            regAddr = SMEM_LION3_MLL_MC_FIFO_DROP_CNTR_REG(devObjPtr);
        }
        else
        {
            regAddr = SMEM_CHT3_MLL_DROP_CNTR_TBL_MEM(devObjPtr);
        }

        regPtr = smemMemGet(devObjPtr,regAddr);
        (*regPtr)++;
    }



    /************************** DROP COUNTER REGISTERS *******************/

}

/*******************************************************************************
*   snetCht3IngressMllSingleMllOutlifSet
*
* DESCRIPTION:
*        set the target outLif into descriptor - for single MLL
*        and Update mll counters
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
*        singleMllPtr - single mll pointer
* OUTPUTS:
*
*
*******************************************************************************/
void snetCht3IngressMllSingleMllOutlifSet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN   SNET_CHT3_SINGLE_MLL_STC * singleMllPtr
)
{
    DECLARE_FUNC_NAME(snetCht3IngressMllSingleMllOutlifSet);

    descrPtr->eVid = singleMllPtr->vid;
    descrPtr->useVidx = 0;
    descrPtr->targetIsTrunk = 0;
    descrPtr->eVidx = 0xfff;

    if(descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_MIRROR_TO_CPU_E)
    {
        /* the replication should not send copy to the CPU */
        __LOG(("the replication should not send copy to the CPU"));
        descrPtr->packetCmd = SKERNEL_EXT_PKT_CMD_FORWARD_E;
    }

    switch(singleMllPtr->lll.dstInterface)
    {
        case SNET_DST_INTERFACE_PORT_E:
            descrPtr->trgEPort = singleMllPtr->lll.interfaceInfo.devPort.port;
            /* call after setting trgEPort */
            __LOG(("call after setting trgEPort"));
            SNET_E_ARCH_CLEAR_IS_TRG_PHY_PORT_VALID_MAC(devObjPtr,descrPtr,ipvx);
            descrPtr->trgDev  = singleMllPtr->lll.interfaceInfo.devPort.devNum;
            break;
        case SNET_DST_INTERFACE_TRUNK_E:
            descrPtr->targetIsTrunk = 1;
            descrPtr->trgTrunkId = singleMllPtr->lll.interfaceInfo.trunkId;
            break;
        case SNET_DST_INTERFACE_VIDX_E:
            descrPtr->useVidx = 1;
            descrPtr->eVidx = singleMllPtr->lll.interfaceInfo.vidx;
            break;
        default:
            skernelFatalError(" snetCht3Descriptor: bad interface[%d]", singleMllPtr->lll.dstInterface);
    }

    /* Update IP mll counters */
    __LOG(("Update IP_MLL counters \n"));
    snetCht3mllCounters(devObjPtr, descrPtr , GT_FALSE/*count IP MLL (not L2 MLL)*/);

    /* ts mll support */
    if (singleMllPtr->isTunnelStart == GT_TRUE)
    {
        descrPtr->tunnelStart = GT_TRUE;
        descrPtr->tunnelPtr = singleMllPtr->tsInfo.tunnelStartPtr;
        descrPtr->tunnelStartPassengerType = singleMllPtr->tsInfo.tunnelStartType;

         __LOG(("Tunnel Start [0x%4.4x] passengerType[%d] from MLL \n",
                 descrPtr->tunnelPtr, descrPtr->tunnelStartPassengerType));
    }

}



/*******************************************************************************
*   snetCht3IngressMllReplication
*
* DESCRIPTION:
*       Ingress MLL replication
*
* INPUTS:
*       devObjPtr       - pointer to device object
*       descrPtr        - pointer to frame descriptor
*       mllAddr         - index to first MLL
* OUTPUT:
*
*
* RETURN:
*
* COMMENTS:
*
*******************************************************************************/
static GT_VOID snetCht3IngressMllReplication
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC   * descrPtr,
    IN GT_U32 mllIndex
)
{
    DECLARE_FUNC_NAME(snetCht3IngressMllReplication);

    SKERNEL_FRAME_CHEETAH_DESCR_STC *origInfoDescrPtr;/* original descriptor info */
    SKERNEL_FRAME_CHEETAH_DESCR_STC *nextDescPtr;/* (pointer to)'next' descriptor info */
    SNET_CHT3_DOUBLE_MLL_STC mll;/* current MLL pair entry (hold 2 single MLLs)*/
    SNET_CHT3_SINGLE_MLL_STC *singleMllPtr; /* pointer to the info about single
                                            MLL (first/second half of the pair)*/
    GT_U32  ii;/* iterator */
    GT_BOOL sendReplication; /* check if to send replication to current single
                                MLL , or skip it */
    GT_U32  routerMCSourceID;/*RouterMC Source-ID */
    GT_U32  value;/*register value*/
    GT_U32  srcIdMask,srcIdValue;
    GT_U32  *memPtr; /*pointer to memory*/

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        smemRegFldGet(devObjPtr, SMEM_LION3_IP_MLL_MC_SOURCE_ID_REG(devObjPtr), 0, 24, &value);
        srcIdMask  = SMEM_U32_GET_FIELD(value,0,12);
        srcIdValue = SMEM_U32_GET_FIELD(value,12,12);

        routerMCSourceID = (descrPtr->sstId & (~srcIdMask)) | (srcIdValue & srcIdMask);
    }
    else
    {
        smemRegFldGet(devObjPtr, SMEM_CHT2_MLL_GLB_CONTROL_REG(devObjPtr), 0, 5, &value);
        routerMCSourceID = SMEM_U32_GET_FIELD(value,0,5);
    }
    __LOG(("Source ID assigned to IP Multicast packets duplicated to the down streams [%x] \n",
        routerMCSourceID));


    /*********************************/
    /* save original descriptor info */
    /*********************************/
    /* duplicate the descriptor and save it's info */
    origInfoDescrPtr = snetChtEqDuplicateDescr(devObjPtr,descrPtr);
    /* 'next replication' : duplicate descriptor from the ingress core */
    nextDescPtr = snetChtEqDuplicateDescr(devObjPtr,descrPtr);

    while(1)
    {
        if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            if(GT_TRUE ==
                snetLion3IngressMllAccessCheck(devObjPtr,descrPtr,GT_TRUE,mllIndex))
            {
                /* the mllIndex is 'out of range' */
                return;
            }
        }

        snetCht3ReadMllEntry(devObjPtr,mllIndex,&mll);

        /* check the 2 MLL sections */
        for(ii = 0 ; ii < 2 ; ii++)
        {
            __LOG(("check the MLL [%d] section \n",
                    ii));
            sendReplication = GT_TRUE;
            if(ii == 0)
            {
                singleMllPtr = &mll.first_mll;
            }
            else
            {
                singleMllPtr = &mll.second_mll;
            }

            if (singleMllPtr->excludeSrcVlan != 1 ||
             descrPtr->eVid != singleMllPtr->vid)
            {
                if (descrPtr->ttl < singleMllPtr->ttlThres)
                {
                    sendReplication = GT_FALSE;
                }

                if(descrPtr->mllPtrForWlanBridging)
                {
                    /* current replication is for WLAN */
                    __LOG(("current replication is for WLAN"));

                    /* set the tagging mode for the passenger VLAN egress */
                    __LOG(("set the tagging mode for the passenger VLAN egress"));
                    descrPtr->passengerTag = singleMllPtr->vlanEgressTagMode;

                    /* check replication for the unknown/unregistered traffic */
                    __LOG(("check replication for the unknown/unregistered traffic"));
                    if(descrPtr->macDaFound == GT_FALSE)
                    {
                        switch(descrPtr->macDaType)
                        {
                            case SKERNEL_UNICAST_MAC_E:
                                if(singleMllPtr->unknownUcFiltering)
                                {
                                    sendReplication = GT_FALSE;
                                }
                                break;
                            case SKERNEL_MULTICAST_MAC_E:
                                if(singleMllPtr->unregMcFiltering)
                                {
                                    sendReplication = GT_FALSE;
                                }
                                break;
                            default:/* BC traffic */
                                if(singleMllPtr->unregBcFiltering)
                                {
                                    sendReplication = GT_FALSE;
                                }
                                break;
                        }
                    }
                }

                if(sendReplication == GT_TRUE)
                {
                    if(descrPtr->eVid != singleMllPtr->vid)
                    {
                        /* The Source ID assigned to IP Multicast packets duplicated
                           to the down streams*/
                        descrPtr->sstId = routerMCSourceID;
                    }
                    else
                    {
                        /* The Bridge copy of the IP Multicast packet is assigned
                          with a Source-ID assigned by the Bridge*/
                    }

                    /* set the target outLif into descriptor - for single MLL
                       and Update mll counters */
                    snetCht3IngressMllSingleMllOutlifSet(devObjPtr, descrPtr,singleMllPtr);

                    if(simLogIsOpen())
                    {
                        *nextDescPtr = *descrPtr;

                        /*restore original values for the 'compare' of descriptors 'old and new'*/
                        *descrPtr = *origInfoDescrPtr;
                        SIM_LOG_PACKET_DESCR_SAVE

                        *descrPtr = *nextDescPtr;
                        SIM_LOG_PACKET_DESCR_COMPARE("(IPMC replication) IPMllEngine replication \n");
                    }


                    /* call the rest of the 'Ingress pipe' */
                    __LOG(("call the rest of the 'Ingress pipe'"));
                    snetChtIngressAfterL3IpReplication(devObjPtr, descrPtr);
                    /* restore original descriptor */
                    __LOG(("restore original descriptor"));
                    *descrPtr = *origInfoDescrPtr;

                }
                else
                {
                    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
                    {
                        memPtr = smemMemGet(devObjPtr, SMEM_LION3_IP_MLL_SKIPPED_ENTRIES_COUNTER_REG (devObjPtr));
                        __LOG(("increment IP MLL Skipped Entries Counter from [%d]\n", *memPtr));
                        (*memPtr)++;
                    }
                }

            }

            if (singleMllPtr->last == 1)
            {
                return;
            }
        }

        if(mll.nextPtr == 0)
        {
            /* NOTE : we should not get here because the second section of MLL
                should have been set to singleMllPtr->last = 0 */
            return;
        }

        /* update the address for the next MLL */
        __LOG(("update the address for the next MLL"));
        mllIndex = mll.nextPtr;
    }
}

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
)
{
    DECLARE_FUNC_NAME(snetCht3IngressL2TunnelReplication);

    GT_U32  tmpAddress;

    /* get WLAN bridging parameters */
    __LOG(("get WLAN bridging parameters"));

    if(descrPtr->useVidx && descrPtr->eVidx != 0xFFF)
    {
        /* get the vidx MLL */
        __LOG(("get the vidx MLL"));
        tmpAddress = SMEM_CHT3_VIDX_TO_MLL_MAP_ENTRY_TBL_MEM(descrPtr->eVidx);
    }
    else
    {
        /* get the vlanId MLL */
        __LOG(("get the vlanId MLL"));
        tmpAddress = SMEM_CHT3_VID_TO_MLL_MAP_TBL_MEM(descrPtr->eVid);
    }

    smemRegGet(devObjPtr,tmpAddress,&descrPtr->mllPtrForWlanBridging);

    if(descrPtr->mllPtrForWlanBridging == 0)
    {
        /* no MLL attached to this vlan/Vidx */
        __LOG(("no MLL attached to this vlan/Vidx"));
        return;
    }

    /* do replications according to MLL */
    __LOG(("do replications according to MLL"));
    snetCht3IngressMllReplication(devObjPtr,descrPtr,descrPtr->mllPtrForWlanBridging);

    return;
}

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
)
{
    DECLARE_FUNC_NAME(snetCht3IngressL3IpReplication);

    GT_U32  mllIndex;/* MLL index */
    GT_BIT  isIpv6 = (descrPtr->isIPv4 || descrPtr->isFcoe) ? 0 : 1;

    /* get IPMC routing parameters */
    __LOG(("get IPMC routing parameters"));
    if (isIpv6 && (descrPtr->mllSelector == SKERNEL_MLL_SELECT_EXTERNAL_E))
    {
        mllIndex = descrPtr->mllexternal;
    }
    else
    {
        mllIndex = descrPtr->mll;
    }

    /* do replications according to MLL */
    __LOG(("do replications according to MLL"));
    snetCht3IngressMllReplication(devObjPtr,descrPtr,mllIndex);

    /* all mll replications already were done.  So doRouterHa and routed are cleared. */
    __LOG(("all mll replications already were done.  So doRouterHa and routed are cleared."));

    descrPtr->doRouterHa = 0;
    descrPtr->routed = 0;
    return;
}


/*******************************************************************************
*   snetCht3Egress802dot11eFrame
*
* DESCRIPTION:
*       Egress setting the needed TID,AckPolicy fields for the 802.11e frames.
*       the "e" is for QOS parameters in the 802.11 frame (native wireless)
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
*       call this function only if the 802.11 needed to egress as 802.11e (with "e")
*******************************************************************************/
static GT_VOID snetCht3Egress802dot11eFrame
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC   * descrPtr
)
{
    DECLARE_FUNC_NAME(snetCht3Egress802dot11eFrame);

    GT_U32  address;
    GT_U32  regValue;

    address = SMEM_CHT3_QOS_MAP_TO_802_11E_TID_TBL_MEM(
            descrPtr->capwap.egressInfo.tsOrDa802dot11eEgressMappingTableProfile,
            descrPtr->qos.qosProfile);

    smemRegGet(devObjPtr, address, &regValue);


    /* map the qosProfile to TID , AckPolicy of the 802.11e */
    __LOG(("map the qosProfile to TID , AckPolicy of the 802.11e"));
    descrPtr->capwap.egressInfo.tid       = SMEM_U32_GET_FIELD(regValue,0,4);
    descrPtr->capwap.egressInfo.ackPolicy = SMEM_U32_GET_FIELD(regValue,4,2);

    return;
}

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
)
{
    DECLARE_FUNC_NAME(snetCht3EgressConvert802dot11ToEthernetV2Frame);

    GT_U32  copySize;
    SBUF_BUF_ID   frameBuf;  /* pointer to frame buffer */

    /***********************************************/
    /* the mac addresses already set by the caller */
    /***********************************************/

    /* we need to set the ether type */
    __LOG(("we need to set the ether type"));
    *egrBufPtr = (GT_U8)(descrPtr->capwap.frame802dot11Info.etherType >> 8);
    egrBufPtr++;
    *egrBufPtr = (GT_U8)descrPtr->capwap.frame802dot11Info.etherType;
    egrBufPtr++;

    frameBuf = descrPtr->frameBuf;

    /* calculate the pay load length (of the payload in the 802.11 passenger)*/
    __LOG(("calculate the pay load length (of the payload in the 802.11 passenger)"));
    copySize = frameBuf->actualDataSize -
                (descrPtr->capwap.frame802dot11Info.payloadPtr -
                 frameBuf->actualDataPtr);

    /* we need to set the pay load */
    __LOG(("we need to set the pay load"));
    MEM_APPEND(egrBufPtr, descrPtr->capwap.frame802dot11Info.payloadPtr, copySize);

    return egrBufPtr;
}

/*******************************************************************************
*   snetCht3TsCapwapEntryRead
*
* DESCRIPTION:
*         Read Multicast Link List entry
* INPUTS:
*        tsAddr - TS capwap memory address.
* OUTPUTS:
*        tsCapwapPtr  - pointer to the TS CAPWAP entry.
*
*
*******************************************************************************/
static void snetCht3TsCapwapEntryRead
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32                  tsAddr,
    OUT SNET_CHT3_TS_CAPWAP_STC *tsCapwapPtr
)
{
    SNET_CHT3_TS_CAPWAP_FIELDS_ENT    index;
    GT_BOOL                 isIpv4 =GT_FALSE;
    GT_U32                  tmpVal;
    GT_U32                  ii,iiMax = 0;
    GT_U32                  difBetweenIpv4ToIp6 = 0;
    GT_U32                  *tsCapwapEntryPtr;

    tsCapwapEntryPtr = smemMemGet(devObjPtr, tsAddr);

    index = SNET_CHT3_TS_CAPWAP_FIELD_TUNNEL_TYPE_E;
    tmpVal = snetFieldValueGet(tsCapwapEntryPtr,
        snetCh3TsCapwapEntry[index].startBit,
        (snetCh3TsCapwapEntry[index].lastBit - snetCh3TsCapwapEntry[index].startBit + 1));
    tsCapwapPtr->tunnelType = tmpVal;

    switch(tsCapwapPtr->tunnelType)
    {
        case 2:
            iiMax = 1;/* single field for ip address */
            difBetweenIpv4ToIp6 = 0;
            isIpv4 = GT_TRUE; /* ipv4 */
            break;
        case 3:
            iiMax = 4;/* 4 fields for ip address */
            /* the diff between the fields of ipv4 ad those of  ipv6 */
            difBetweenIpv4ToIp6 = SNET_CHT3_TS_CAPWAP_FIELD_IPV6_UDP_SRC_PORT_E -
                                  SNET_CHT3_TS_CAPWAP_FIELD_IPV4_UDP_SRC_PORT_E;
            isIpv4 = GT_FALSE;/* ipv6 */
            break;
        default:
            skernelFatalError(" snetCht3ReadTsCapwapEntry: not capwap tunnel start [%d]",tsCapwapPtr->tunnelType );
    }

    index = SNET_CHT3_TS_CAPWAP_FIELD_UP_MARKING_MODE_E;
    tmpVal = snetFieldValueGet(tsCapwapEntryPtr,
        snetCh3TsCapwapEntry[index].startBit,
        (snetCh3TsCapwapEntry[index].lastBit - snetCh3TsCapwapEntry[index].startBit + 1));
    tsCapwapPtr->upMarkingMode = tmpVal;


    index = SNET_CHT3_TS_CAPWAP_FIELD_UP_E;
    tmpVal = snetFieldValueGet(tsCapwapEntryPtr,
        snetCh3TsCapwapEntry[index].startBit,
        (snetCh3TsCapwapEntry[index].lastBit - snetCh3TsCapwapEntry[index].startBit + 1));
    tsCapwapPtr->up = tmpVal;

    index = SNET_CHT3_TS_CAPWAP_FIELD_TAG_ENABLE_E;
    tmpVal = snetFieldValueGet(tsCapwapEntryPtr,
        snetCh3TsCapwapEntry[index].startBit,
        (snetCh3TsCapwapEntry[index].lastBit - snetCh3TsCapwapEntry[index].startBit + 1));
    tsCapwapPtr->tagEnable = tmpVal;

    index = SNET_CHT3_TS_CAPWAP_FIELD_VID_E;
    tmpVal = snetFieldValueGet(tsCapwapEntryPtr,
        snetCh3TsCapwapEntry[index].startBit,
        (snetCh3TsCapwapEntry[index].lastBit - snetCh3TsCapwapEntry[index].startBit + 1));
    tsCapwapPtr->vid = tmpVal;

    index = SNET_CHT3_TS_CAPWAP_FIELD_TTL_OR_HOT_LIMIT_E;
    tmpVal = snetFieldValueGet(tsCapwapEntryPtr,
        snetCh3TsCapwapEntry[index].startBit,
        (snetCh3TsCapwapEntry[index].lastBit - snetCh3TsCapwapEntry[index].startBit + 1));
    tsCapwapPtr->ttlOrHotLimit = tmpVal;


    index = SNET_CHT3_TS_CAPWAP_FIELD_CAPWAP_T_BIT_E;
    tmpVal = snetFieldValueGet(tsCapwapEntryPtr,
        snetCh3TsCapwapEntry[index].startBit,
        (snetCh3TsCapwapEntry[index].lastBit - snetCh3TsCapwapEntry[index].startBit + 1));
    tsCapwapPtr->capwap.tBit = tmpVal;

    index = SNET_CHT3_TS_CAPWAP_FIELD_CAPWAP_W_BIT_E;
    tmpVal = snetFieldValueGet(tsCapwapEntryPtr,
        snetCh3TsCapwapEntry[index].startBit,
        (snetCh3TsCapwapEntry[index].lastBit - snetCh3TsCapwapEntry[index].startBit + 1));
    tsCapwapPtr->capwap.wBit = tmpVal;


    index = SNET_CHT3_TS_CAPWAP_FIELD_CAPWAP_M_BIT_E;
    tmpVal = snetFieldValueGet(tsCapwapEntryPtr,
        snetCh3TsCapwapEntry[index].startBit,
        (snetCh3TsCapwapEntry[index].lastBit - snetCh3TsCapwapEntry[index].startBit + 1));
    tsCapwapPtr->capwap.mBit = tmpVal;

    index = SNET_CHT3_TS_CAPWAP_FIELD_EGRESS_OSM_REDIRECT_BIT_E;
    tmpVal = snetFieldValueGet(tsCapwapEntryPtr,
        snetCh3TsCapwapEntry[index].startBit,
        (snetCh3TsCapwapEntry[index].lastBit - snetCh3TsCapwapEntry[index].startBit + 1));
    tsCapwapPtr->egressOsmRedirect = tmpVal;

    index = SNET_CHT3_TS_CAPWAP_FIELD_NEXT_HOP_MAC_DA_0_31_E;
    tmpVal = snetFieldValueGet(tsCapwapEntryPtr,
        snetCh3TsCapwapEntry[index].startBit,
        (snetCh3TsCapwapEntry[index].lastBit - snetCh3TsCapwapEntry[index].startBit + 1));

    tsCapwapPtr->nextHopMacDa[2] = (GT_U8)(tmpVal >> 24);
    tsCapwapPtr->nextHopMacDa[3] = (GT_U8)(tmpVal >> 16);
    tsCapwapPtr->nextHopMacDa[4] = (GT_U8)(tmpVal >>  8);
    tsCapwapPtr->nextHopMacDa[5] = (GT_U8)(tmpVal >>  0);

    index = SNET_CHT3_TS_CAPWAP_FIELD_NEXT_HOP_MAC_DA_32_47_E;
    tmpVal = snetFieldValueGet(tsCapwapEntryPtr,
        snetCh3TsCapwapEntry[index].startBit,
        (snetCh3TsCapwapEntry[index].lastBit - snetCh3TsCapwapEntry[index].startBit + 1));

    tsCapwapPtr->nextHopMacDa[0] = (GT_U8)(tmpVal >>  8);
    tsCapwapPtr->nextHopMacDa[1] = (GT_U8)(tmpVal >>  0);

    index = SNET_CHT3_TS_CAPWAP_FIELD_WLAN_ID_BITMAP_E;
    tmpVal = snetFieldValueGet(tsCapwapEntryPtr,
        snetCh3TsCapwapEntry[index].startBit,
        (snetCh3TsCapwapEntry[index].lastBit - snetCh3TsCapwapEntry[index].startBit + 1));
    tsCapwapPtr->wlanIdBitmap = tmpVal;

    index = SNET_CHT3_TS_CAPWAP_FIELD_DSCP_E;
    tmpVal = snetFieldValueGet(tsCapwapEntryPtr,
        snetCh3TsCapwapEntry[index].startBit,
        (snetCh3TsCapwapEntry[index].lastBit - snetCh3TsCapwapEntry[index].startBit + 1));
    tsCapwapPtr->dscp = tmpVal;

    index = SNET_CHT3_TS_CAPWAP_FIELD_DSCP_MARKING_MODE_E;
    tmpVal = snetFieldValueGet(tsCapwapEntryPtr,
        snetCh3TsCapwapEntry[index].startBit,
        (snetCh3TsCapwapEntry[index].lastBit - snetCh3TsCapwapEntry[index].startBit + 1));
    tsCapwapPtr->dscpMarkingMode = tmpVal;

    index = SNET_CHT3_TS_CAPWAP_FIELD_DONT_FRAGMENT_FLAG_E;
    tmpVal = snetFieldValueGet(tsCapwapEntryPtr,
        snetCh3TsCapwapEntry[index].startBit,
        (snetCh3TsCapwapEntry[index].lastBit - snetCh3TsCapwapEntry[index].startBit + 1));
    tsCapwapPtr->dontFragmentFlag = tmpVal;

    index = SNET_CHT3_TS_CAPWAP_FIELD_CAPWAP_VERSION_E;
    tmpVal = snetFieldValueGet(tsCapwapEntryPtr,
        snetCh3TsCapwapEntry[index].startBit,
        (snetCh3TsCapwapEntry[index].lastBit - snetCh3TsCapwapEntry[index].startBit + 1));
    tsCapwapPtr->capwap.version = tmpVal;

    index = SNET_CHT3_TS_CAPWAP_FIELD_CAPWAP_RID_E;
    tmpVal = snetFieldValueGet(tsCapwapEntryPtr,
        snetCh3TsCapwapEntry[index].startBit,
        (snetCh3TsCapwapEntry[index].lastBit - snetCh3TsCapwapEntry[index].startBit + 1));
    tsCapwapPtr->capwap.rid = tmpVal;

    index = SNET_CHT3_TS_CAPWAP_FIELD_CAPWAP_FLAGS_E;
    tmpVal = snetFieldValueGet(tsCapwapEntryPtr,
        snetCh3TsCapwapEntry[index].startBit,
        (snetCh3TsCapwapEntry[index].lastBit - snetCh3TsCapwapEntry[index].startBit + 1));
    tsCapwapPtr->capwap.flags = tmpVal;

    index = SNET_CHT3_TS_CAPWAP_FIELD_CAPWAP_WBID_E;
    tmpVal = snetFieldValueGet(tsCapwapEntryPtr,
        snetCh3TsCapwapEntry[index].startBit,
        (snetCh3TsCapwapEntry[index].lastBit - snetCh3TsCapwapEntry[index].startBit + 1));
    tsCapwapPtr->capwap.wbid = tmpVal;

    index = SNET_CHT3_TS_CAPWAP_FIELD_ENABLE_802_11_WDS_E;
    tmpVal = snetFieldValueGet(tsCapwapEntryPtr,
        snetCh3TsCapwapEntry[index].startBit,
        (snetCh3TsCapwapEntry[index].lastBit - snetCh3TsCapwapEntry[index].startBit + 1));
    tsCapwapPtr->enable802dot11wds = tmpVal;

    index = SNET_CHT3_TS_CAPWAP_FIELD_DEFAULT_802_11E_ENABLE_E;
    tmpVal = snetFieldValueGet(tsCapwapEntryPtr,
        snetCh3TsCapwapEntry[index].startBit,
        (snetCh3TsCapwapEntry[index].lastBit - snetCh3TsCapwapEntry[index].startBit + 1));
    tsCapwapPtr->default802dot11eEnable = tmpVal;

    index = SNET_CHT3_TS_CAPWAP_FIELD_802_11E_MAPPING_PROFILE_E;
    tmpVal = snetFieldValueGet(tsCapwapEntryPtr,
        snetCh3TsCapwapEntry[index].startBit,
        (snetCh3TsCapwapEntry[index].lastBit - snetCh3TsCapwapEntry[index].startBit + 1));
    tsCapwapPtr->_802dot11eMappingProfile = tmpVal;

    if(isIpv4 == GT_TRUE)
    {
        index = SNET_CHT3_TS_CAPWAP_FIELD_IPV4_DIP_E;
    }
    else
    {
        index = SNET_CHT3_TS_CAPWAP_FIELD_IPV6_DIP_0_31_E;
    }

    for(ii = 0; ii< iiMax ; ii++ , index++)
    {
        tmpVal = snetFieldValueGet(tsCapwapEntryPtr,
            snetCh3TsCapwapEntry[index].startBit,
            (snetCh3TsCapwapEntry[index].lastBit - snetCh3TsCapwapEntry[index].startBit + 1));

        tsCapwapPtr->dip[0 + 4*(iiMax-ii-1)] = (GT_U8)(tmpVal >> 24);
        tsCapwapPtr->dip[1 + 4*(iiMax-ii-1)] = (GT_U8)(tmpVal >> 16);
        tsCapwapPtr->dip[2 + 4*(iiMax-ii-1)] = (GT_U8)(tmpVal >>  8);
        tsCapwapPtr->dip[3 + 4*(iiMax-ii-1)] = (GT_U8)(tmpVal >>  0);
    }


    if(isIpv4 == GT_TRUE)
    {
        index = SNET_CHT3_TS_CAPWAP_FIELD_IPV4_SIP_E;
    }
    else
    {
        index = SNET_CHT3_TS_CAPWAP_FIELD_IPV6_SIP_0_31_E;
    }

    for(ii = 0; ii< iiMax ; ii++ , index++)
    {
        tmpVal = snetFieldValueGet(tsCapwapEntryPtr,
            snetCh3TsCapwapEntry[index].startBit,
            (snetCh3TsCapwapEntry[index].lastBit - snetCh3TsCapwapEntry[index].startBit + 1));

        tsCapwapPtr->sip[0 + 4*(iiMax-ii-1)] = (GT_U8)(tmpVal >> 24);
        tsCapwapPtr->sip[1 + 4*(iiMax-ii-1)] = (GT_U8)(tmpVal >> 16);
        tsCapwapPtr->sip[2 + 4*(iiMax-ii-1)] = (GT_U8)(tmpVal >>  8);
        tsCapwapPtr->sip[3 + 4*(iiMax-ii-1)] = (GT_U8)(tmpVal >>  0);
    }

    index = SNET_CHT3_TS_CAPWAP_FIELD_IPV4_UDP_SRC_PORT_E;
    index += difBetweenIpv4ToIp6;
    tmpVal = snetFieldValueGet(tsCapwapEntryPtr,
        snetCh3TsCapwapEntry[index].startBit,
        (snetCh3TsCapwapEntry[index].lastBit - snetCh3TsCapwapEntry[index].startBit + 1));
    tsCapwapPtr->udpSrcPort = tmpVal;

    index = SNET_CHT3_TS_CAPWAP_FIELD_IPV4_UDP_DST_PORT_E;
    index += difBetweenIpv4ToIp6;
    tmpVal = snetFieldValueGet(tsCapwapEntryPtr,
        snetCh3TsCapwapEntry[index].startBit,
        (snetCh3TsCapwapEntry[index].lastBit - snetCh3TsCapwapEntry[index].startBit + 1));
    tsCapwapPtr->udpDstPort = tmpVal;

    index = SNET_CHT3_TS_CAPWAP_FIELD_IPV4_BSSID_OR_TA_0_31_E;
    index += difBetweenIpv4ToIp6;
    tmpVal = snetFieldValueGet(tsCapwapEntryPtr,
        snetCh3TsCapwapEntry[index].startBit,
        (snetCh3TsCapwapEntry[index].lastBit - snetCh3TsCapwapEntry[index].startBit + 1));

    tsCapwapPtr->wds.bssidOrTa[2] = (GT_U8)(tmpVal >> 24);
    tsCapwapPtr->wds.bssidOrTa[3] = (GT_U8)(tmpVal >> 16);
    tsCapwapPtr->wds.bssidOrTa[4] = (GT_U8)(tmpVal >>  8);
    tsCapwapPtr->wds.bssidOrTa[5] = (GT_U8)(tmpVal >>  0);

    index = SNET_CHT3_TS_CAPWAP_FIELD_IPV4_BSSID_OR_TA_32_47_E;
    index += difBetweenIpv4ToIp6;
    tmpVal = snetFieldValueGet(tsCapwapEntryPtr,
        snetCh3TsCapwapEntry[index].startBit,
        (snetCh3TsCapwapEntry[index].lastBit - snetCh3TsCapwapEntry[index].startBit + 1));

    tsCapwapPtr->wds.bssidOrTa[0] = (GT_U8)(tmpVal >>  8);
    tsCapwapPtr->wds.bssidOrTa[1] = (GT_U8)(tmpVal >>  0);



    index = SNET_CHT3_TS_CAPWAP_FIELD_IPV4_RA_0_15_E;
    index += difBetweenIpv4ToIp6;
    tmpVal = snetFieldValueGet(tsCapwapEntryPtr,
        snetCh3TsCapwapEntry[index].startBit,
        (snetCh3TsCapwapEntry[index].lastBit - snetCh3TsCapwapEntry[index].startBit + 1));

    tsCapwapPtr->wds.ra[4] = (GT_U8)(tmpVal >>  8);
    tsCapwapPtr->wds.ra[5] = (GT_U8)(tmpVal >>  0);

    index = SNET_CHT3_TS_CAPWAP_FIELD_IPV4_RA_16_47_E;
    index += difBetweenIpv4ToIp6;
    tmpVal = snetFieldValueGet(tsCapwapEntryPtr,
        snetCh3TsCapwapEntry[index].startBit,
        (snetCh3TsCapwapEntry[index].lastBit - snetCh3TsCapwapEntry[index].startBit + 1));

    tsCapwapPtr->wds.ra[0] = (GT_U8)(tmpVal >> 24);
    tsCapwapPtr->wds.ra[1] = (GT_U8)(tmpVal >> 16);
    tsCapwapPtr->wds.ra[2] = (GT_U8)(tmpVal >>  8);
    tsCapwapPtr->wds.ra[3] = (GT_U8)(tmpVal >>  0);


    return;
}

/*******************************************************************************
*   snetCht3TsCapwap802dot11MacHeaderFrameFieldBuildByPointer
*
* DESCRIPTION:
*       function insert data of the field to the frame header.
*
* INPUTS:
*       capwap802dot11MacHeaderPtr - (pointer to) the CAPWAP 802.11 MAC header
*       fieldValPtr - (pointer to) data of field to insert to header
*       fieldId -- field id
*
* OUTPUTS:
*       capwap802dot11MacHeaderPtr - (pointer to) the updated CAPWAP 802.11 MAC header
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID snetCht3TsCapwap802dot11MacHeaderFrameFieldBuildByPointer
(
    INOUT SNET_CHT_POLICY_KEY_STC           *capwap802dot11MacHeaderPtr,
    IN GT_U8                                *fieldValPtr,
    IN CHT3_TS_CAPWAP_802_11_MAC_HEADER_ENT fieldId
)
{
    CHT_PCL_KEY_FIELDS_INFO_STC *fieldInfoPtr = &cht3TsCapwap802dot11MacHeaderFields[fieldId];

    snetChtPclSrvKeyFieldBuildByPointerNetworkOrder(capwap802dot11MacHeaderPtr, fieldValPtr, fieldInfoPtr);

    return;
}

/*******************************************************************************
*   snetCht3TsCapwapFrame802dot11MacHeaderFieldBuildByValue
*
* DESCRIPTION:
*       function insert data of the field to the frame header.
*
* INPUTS:
*       capwap802dot11MacHeaderPtr - (pointer to) the CAPWAP 802.11 MAC header
*       fieldVal - data of field to insert to header
*       fieldId -- field id
*
* OUTPUTS:
*       capwap802dot11MacHeaderPtr - (pointer to) the updated CAPWAP 802.11 MAC header
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID snetCht3TsCapwap802dot11MacHeaderFrameFieldBuildByValue
(
    INOUT SNET_CHT_POLICY_KEY_STC           *capwap802dot11MacHeaderPtr,
    IN GT_U32                               fieldVal,
    IN CHT3_TS_CAPWAP_802_11_MAC_HEADER_ENT fieldId
)
{
    CHT_PCL_KEY_FIELDS_INFO_STC *fieldInfoPtr = &cht3TsCapwap802dot11MacHeaderFields[fieldId];

    snetChtPclSrvKeyFieldBuildByValueNetworkOrder(capwap802dot11MacHeaderPtr, fieldVal, fieldInfoPtr);

    return;
}
/*******************************************************************************
*   snetCht3TsCapwap802dot11MacHeaderGenerate
*
* DESCRIPTION:
*   Data packets that are forwarded to a WTP over a CAPWAP tunnel must be sent
*   as 802.11 frames.
*   The packet MAC DA, MAC SA, and EtherType are extracted from the original
*   packet - this is done for both Ethernet or 802.11.
*   The original MAC header is removed, and the 802.11 header is generated
*
* INPUTS:
*        tsAddr - TS CAPWAP memory address.
*        descrPtr     - pointer to the frame's descriptor.
*        tsCapwapPtr  - pointer to the TS CAPWAP entry.
*        egressBufferPtr - egress buffer to put the header into
* OUTPUTS:
*       descrPtr     - pointer to updated frame data buffer Id
*
*   RETURNS:
*       pointer to first byte in the buffer that was not modified.
*
*******************************************************************************/
static GT_U8* snetCht3TsCapwap802dot11MacHeaderGenerate
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    OUT SNET_CHT3_TS_CAPWAP_STC *tsCapwapPtr,
    IN GT_U8*      egressBufferPtr
)
{
    DECLARE_FUNC_NAME(snetCht3TsCapwap802dot11MacHeaderGenerate);

    CHT3_TS_CAPWAP_802_11_MAC_HEADER_ENT fieldIndex;/* index of current field */
    SNET_CHT_POLICY_KEY_STC capwap802dot11MacHeaderInfo;
    GT_U32      numBytesOffset;
    GT_U32      tmpValue;
    GT_U8       llcHeader[6] = {0xaa,0xaa,0x03,0x00,0x00,0x00};
    GT_U32      maxBytesToWrite = 40;/* max number of bytes of the header */

    capwap802dot11MacHeaderInfo.pclKeyFormat = CHT_PCL_KEY_UNLIMITED_SIZE_E;
    capwap802dot11MacHeaderInfo.updateOnlyDiff = GT_FALSE;
    capwap802dot11MacHeaderInfo.key.unlimitedBufferPtr = egressBufferPtr;
    capwap802dot11MacHeaderInfo.devObjPtr = devObjPtr;

    numBytesOffset = 0;

    fieldIndex = CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_PROTOCOL_E;
    snetCht3TsCapwap802dot11MacHeaderFrameFieldBuildByValue(&capwap802dot11MacHeaderInfo, 0,/* fixed value */
                                    fieldIndex);

    fieldIndex = CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_TYPE_E;
    snetCht3TsCapwap802dot11MacHeaderFrameFieldBuildByValue(&capwap802dot11MacHeaderInfo, 2,/* fixed value */
                                    fieldIndex);

    fieldIndex = CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_SUBTYPE_E;
    tmpValue = descrPtr->capwap.egressInfo.tsOrDa802dot11eEgressEnable == GT_TRUE ? 8 : 0;
    snetCht3TsCapwap802dot11MacHeaderFrameFieldBuildByValue(&capwap802dot11MacHeaderInfo, tmpValue,
                                    fieldIndex);

    fieldIndex = CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_TO_DS_E;
    snetCht3TsCapwap802dot11MacHeaderFrameFieldBuildByValue(&capwap802dot11MacHeaderInfo, tsCapwapPtr->enable802dot11wds,
                                    fieldIndex);

    fieldIndex = CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_FROM_DS_E;
    snetCht3TsCapwap802dot11MacHeaderFrameFieldBuildByValue(&capwap802dot11MacHeaderInfo, 1, /* fixed value */
                                    fieldIndex);

    fieldIndex = CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_MORE_FRAG_E;
    snetCht3TsCapwap802dot11MacHeaderFrameFieldBuildByValue(&capwap802dot11MacHeaderInfo, 0, /* fixed value */
                                    fieldIndex);

    fieldIndex = CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_RETRY_E;
    snetCht3TsCapwap802dot11MacHeaderFrameFieldBuildByValue(&capwap802dot11MacHeaderInfo, 0, /* fixed value */
                                    fieldIndex);

    fieldIndex = CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_PWR_MGT_E;
    snetCht3TsCapwap802dot11MacHeaderFrameFieldBuildByValue(&capwap802dot11MacHeaderInfo, 0, /* fixed value */
                                    fieldIndex);

    fieldIndex = CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_MORE_DATA_E;
    snetCht3TsCapwap802dot11MacHeaderFrameFieldBuildByValue(&capwap802dot11MacHeaderInfo, 0, /* fixed value */
                                    fieldIndex);

    fieldIndex = CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_WEP_E;
    snetCht3TsCapwap802dot11MacHeaderFrameFieldBuildByValue(&capwap802dot11MacHeaderInfo, 0, /* fixed value */
                                    fieldIndex);

    fieldIndex = CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_ORDER_E;
    snetCht3TsCapwap802dot11MacHeaderFrameFieldBuildByValue(&capwap802dot11MacHeaderInfo, 0, /* fixed value */
                                    fieldIndex);

    fieldIndex = CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_DURATION_E;
    snetCht3TsCapwap802dot11MacHeaderFrameFieldBuildByValue(&capwap802dot11MacHeaderInfo, 0, /* fixed value */
                                    fieldIndex);

    fieldIndex = CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_ADDRESS_1_E;
    snetCht3TsCapwap802dot11MacHeaderFrameFieldBuildByPointer(&capwap802dot11MacHeaderInfo, descrPtr->capwap.egressInfo.address1Ptr,
                                    fieldIndex);

    fieldIndex = CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_ADDRESS_2_E;
    snetCht3TsCapwap802dot11MacHeaderFrameFieldBuildByPointer(&capwap802dot11MacHeaderInfo, descrPtr->capwap.egressInfo.address2Ptr,
                                    fieldIndex);

    fieldIndex = CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_ADDRESS_3_E;
    snetCht3TsCapwap802dot11MacHeaderFrameFieldBuildByPointer(&capwap802dot11MacHeaderInfo, descrPtr->capwap.egressInfo.address3Ptr,
                                    fieldIndex);

    fieldIndex = CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_SEQUENCE_CONTROL_E;
    snetCht3TsCapwap802dot11MacHeaderFrameFieldBuildByValue(&capwap802dot11MacHeaderInfo, 0,/* fixed value */
                                    fieldIndex);

    if(descrPtr->capwap.egressInfo.address4Ptr)
    {
        fieldIndex = CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_ADDRESS_4_E;
        snetCht3TsCapwap802dot11MacHeaderFrameFieldBuildByPointer(&capwap802dot11MacHeaderInfo, descrPtr->capwap.egressInfo.address4Ptr,
                                        fieldIndex);

        numBytesOffset = 0;
    }
    else
    {
        numBytesOffset = 6;/* 6 bytes of the mac address 4 */
    }

    /****************************************************************/
    /* update pointer to overcome the use/un-use of optional fields */
    /****************************************************************/
    capwap802dot11MacHeaderInfo.key.unlimitedBufferPtr = egressBufferPtr - numBytesOffset;

    if(descrPtr->capwap.egressInfo.tsOrDa802dot11eEgressEnable == GT_TRUE)
    {
        fieldIndex = CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_QOS_CONTROL_0_3_E;
        snetCht3TsCapwap802dot11MacHeaderFrameFieldBuildByValue(&capwap802dot11MacHeaderInfo, descrPtr->capwap.egressInfo.tid,
                                        fieldIndex);

        fieldIndex = CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_QOS_CONTROL_4_E;
        snetCht3TsCapwap802dot11MacHeaderFrameFieldBuildByValue(&capwap802dot11MacHeaderInfo, 0,/* fixed value */
                                        fieldIndex);

        fieldIndex = CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_QOS_CONTROL_5_6_E;
        snetCht3TsCapwap802dot11MacHeaderFrameFieldBuildByValue(&capwap802dot11MacHeaderInfo, descrPtr->capwap.egressInfo.ackPolicy,
                                        fieldIndex);

        fieldIndex = CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_QOS_CONTROL_7_15_E;
        snetCht3TsCapwap802dot11MacHeaderFrameFieldBuildByValue(&capwap802dot11MacHeaderInfo, 0,/* fixed value */
                                        fieldIndex);

        numBytesOffset += 0;
    }
    else
    {
        numBytesOffset += 2;/* 2 bytes of the 802.113 QoS control info */
    }

    /****************************************************************/
    /* update pointer to overcome the use/un-use of optional fields */
    /****************************************************************/
    capwap802dot11MacHeaderInfo.key.unlimitedBufferPtr = egressBufferPtr - numBytesOffset;

    fieldIndex = CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_LLC_DSAP_SSAP_CONTROL_E;
    snetCht3TsCapwap802dot11MacHeaderFrameFieldBuildByPointer(&capwap802dot11MacHeaderInfo, llcHeader,
                                    fieldIndex);

    fieldIndex = CHT3_TS_CAPWAP_802_11_MAC_HEADER_FIELD_LLC_ETHER_TYPE_E;
    snetCht3TsCapwap802dot11MacHeaderFrameFieldBuildByValue(&capwap802dot11MacHeaderInfo, descrPtr->etherTypeOrSsapDsap,
                                    fieldIndex);

    /* return the pointer to the next byte where to put the payload of the 802.11 frame */
    __LOG(("return the pointer to the next byte where to put the payload of the 802.11 frame"));
    return &egressBufferPtr[maxBytesToWrite - numBytesOffset];
}
/*******************************************************************************
*   snetCht3TsCapwapHeaderFieldBuildByValue
*
* DESCRIPTION:
*       function insert data of the field to the frame header.
*
* INPUTS:
*       capwapHeaderPtr - (pointer to) the CAPWAP  header
*       fieldVal - data of field to insert to header
*       fieldId -- field id
*
* OUTPUTS:
*       capwapHeaderPtr - (pointer to) the updated CAPWAP header
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID snetCht3TsCapwapHeaderFieldBuildByValue
(
    INOUT SNET_CHT_POLICY_KEY_STC           *capwapHeaderPtr,
    IN GT_U32                               fieldVal,
    IN CHT3_TS_CAPWAP_HEADER_ENT            fieldId
)
{
    CHT_PCL_KEY_FIELDS_INFO_STC *fieldInfoPtr = &cht3TsCapwapHeaderFields[fieldId];

    snetChtPclSrvKeyFieldBuildByValueNetworkOrder(capwapHeaderPtr, fieldVal, fieldInfoPtr);

    return;
}
/*******************************************************************************
*   snetCht3TsCapwapHeaderGenerate
*
* DESCRIPTION:
*   CAPWAP header is generated for packets forwarded over a CAPWAP tunnel.
*   Note that the Tunnel-start (TS) entry determines some of the CAPWAP headers
*   are generated.
*
* INPUTS:
*        tsAddr - TS CAPWAP memory address.
*        descrPtr     - pointer to the frame's descriptor.
*        tsCapwapPtr  - pointer to the TS CAPWAP entry.
*        egressBufferPtr - egress buffer to put the header into
* OUTPUTS:
*       descrPtr     - pointer to updated frame data buffer Id
*
*   RETURNS:
*       pointer to first byte in the buffer that was not modified.
*
*******************************************************************************/
static GT_U8* snetCht3TsCapwapHeaderGenerate
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    OUT SNET_CHT3_TS_CAPWAP_STC *tsCapwapPtr,
    IN GT_U8*      egressBufferPtr
)
{
    CHT3_TS_CAPWAP_HEADER_ENT fieldIndex;/* index of current field */
    SNET_CHT_POLICY_KEY_STC capwapHeaderInfo;
    GT_U32      numBytesOffset;
    GT_U32      tmpValue;
    GT_U32      maxBytesToWrite = 20;/* max number of bytes of the header */

    capwapHeaderInfo.pclKeyFormat = CHT_PCL_KEY_UNLIMITED_SIZE_E;
    capwapHeaderInfo.updateOnlyDiff = GT_FALSE;
    capwapHeaderInfo.key.unlimitedBufferPtr = egressBufferPtr;
    capwapHeaderInfo.devObjPtr = devObjPtr;


    fieldIndex = CHT3_TS_CAPWAP_HEADER_FIELD_PREAMBLE_VERSION_E;
    snetCht3TsCapwapHeaderFieldBuildByValue(&capwapHeaderInfo, tsCapwapPtr->capwap.version,
                                    fieldIndex);

    fieldIndex = CHT3_TS_CAPWAP_HEADER_FIELD_PREAMBLE_TYPE_E;
    snetCht3TsCapwapHeaderFieldBuildByValue(&capwapHeaderInfo, 0,/* Fixed value -- non encrypted data */
                                    fieldIndex);

    fieldIndex = CHT3_TS_CAPWAP_HEADER_FIELD_PREAMBLE_RESERVED_E;
    snetCht3TsCapwapHeaderFieldBuildByValue(&capwapHeaderInfo, 0,/* Fixed value */
                                    fieldIndex);

    fieldIndex = CHT3_TS_CAPWAP_HEADER_FIELD_VERSION_E;
    snetCht3TsCapwapHeaderFieldBuildByValue(&capwapHeaderInfo, tsCapwapPtr->capwap.version,/* Fixed value ?? */
                                    fieldIndex);

    fieldIndex = CHT3_TS_CAPWAP_HEADER_FIELD_RID_E;
    snetCht3TsCapwapHeaderFieldBuildByValue(&capwapHeaderInfo, tsCapwapPtr->capwap.rid,
                                    fieldIndex);

    fieldIndex = CHT3_TS_CAPWAP_HEADER_FIELD_HLEN_E;
    if(tsCapwapPtr->capwap.wBit)
    {
        tmpValue = 4;/* 4 words length */
    }
    else
    {
        tmpValue = 2;/* 2 words length */
    }
    snetCht3TsCapwapHeaderFieldBuildByValue(&capwapHeaderInfo, tmpValue,
                                    fieldIndex);


    fieldIndex = CHT3_TS_CAPWAP_HEADER_FIELD_WBID_E;
    snetCht3TsCapwapHeaderFieldBuildByValue(&capwapHeaderInfo, tsCapwapPtr->capwap.wbid,
                                    fieldIndex);

    fieldIndex = CHT3_TS_CAPWAP_HEADER_FIELD_T_BIT_E;
    snetCht3TsCapwapHeaderFieldBuildByValue(&capwapHeaderInfo, tsCapwapPtr->capwap.tBit,
                                    fieldIndex);

    fieldIndex = CHT3_TS_CAPWAP_HEADER_FIELD_F_BIT_E;
    snetCht3TsCapwapHeaderFieldBuildByValue(&capwapHeaderInfo, 0,/* not simulate fragments */
                                    fieldIndex);

    fieldIndex = CHT3_TS_CAPWAP_HEADER_FIELD_L_BIT_E;
    snetCht3TsCapwapHeaderFieldBuildByValue(&capwapHeaderInfo, 1,/* not simulate fragments */
                                    fieldIndex);

    fieldIndex = CHT3_TS_CAPWAP_HEADER_FIELD_W_BIT_E;
    snetCht3TsCapwapHeaderFieldBuildByValue(&capwapHeaderInfo, tsCapwapPtr->capwap.wBit,
                                    fieldIndex);

    fieldIndex = CHT3_TS_CAPWAP_HEADER_FIELD_M_BIT_E;
    snetCht3TsCapwapHeaderFieldBuildByValue(&capwapHeaderInfo, 0,/* Fixed value */
                                    fieldIndex);

    fieldIndex = CHT3_TS_CAPWAP_HEADER_FIELD_K_BIT_E;
    snetCht3TsCapwapHeaderFieldBuildByValue(&capwapHeaderInfo, 0,/* Fixed value */
                                    fieldIndex);

    fieldIndex = CHT3_TS_CAPWAP_HEADER_FIELD_FLAGS_E;
    snetCht3TsCapwapHeaderFieldBuildByValue(&capwapHeaderInfo, tsCapwapPtr->capwap.flags,
                                    fieldIndex);

    fieldIndex = CHT3_TS_CAPWAP_HEADER_FIELD_FRAGMENT_ID_E;
    snetCht3TsCapwapHeaderFieldBuildByValue(&capwapHeaderInfo, 0,/* not simulate fragments */
                                    fieldIndex);

    fieldIndex = CHT3_TS_CAPWAP_HEADER_FIELD_FRAGMENT_OFFSET_E;
    snetCht3TsCapwapHeaderFieldBuildByValue(&capwapHeaderInfo, 0,/* not simulate fragments */
                                    fieldIndex);

    fieldIndex = CHT3_TS_CAPWAP_HEADER_FIELD_RESERVED_1_E;
    snetCht3TsCapwapHeaderFieldBuildByValue(&capwapHeaderInfo, 0,/* Fixed value */
                                    fieldIndex);

    if(tsCapwapPtr->capwap.wBit)
    {
        fieldIndex = CHT3_TS_CAPWAP_HEADER_FIELD_WIRELESS_ID_E;
        snetCht3TsCapwapHeaderFieldBuildByValue(&capwapHeaderInfo, tsCapwapPtr->capwap.wbid,
                                        fieldIndex);

        fieldIndex = CHT3_TS_CAPWAP_HEADER_FIELD_LENGTH_E;
        snetCht3TsCapwapHeaderFieldBuildByValue(&capwapHeaderInfo, 4,/* Fixed value */
                                        fieldIndex);

        fieldIndex = CHT3_TS_CAPWAP_HEADER_FIELD_WLAN_ID_BITMAP_E;
        snetCht3TsCapwapHeaderFieldBuildByValue(&capwapHeaderInfo,tsCapwapPtr->wlanIdBitmap ,
                                        fieldIndex);

        fieldIndex = CHT3_TS_CAPWAP_HEADER_FIELD_RESERVED_2_E;
        snetCht3TsCapwapHeaderFieldBuildByValue(&capwapHeaderInfo, 0,/* Fixed value */
                                        fieldIndex);

        fieldIndex = CHT3_TS_CAPWAP_HEADER_FIELD_PADDING_FOR_4_BYTES_ALIGN_E;
        snetCht3TsCapwapHeaderFieldBuildByValue(&capwapHeaderInfo, 0,/* Fixed value */
                                        fieldIndex);

        numBytesOffset = 0;
    }
    else
    {
        numBytesOffset = 8;
    }

    /* return the pointer to the next byte where to put what comes after the
       CAPWAP header */
    return &egressBufferPtr[maxBytesToWrite - numBytesOffset];
}

/*******************************************************************************
*   snetCht3TsCapwapUdpTunnelHeaderFieldBuildByValue
*
* DESCRIPTION:
*       function insert data of the field to the frame header.
*
* INPUTS:
*       capwapUdpTunnelHeaderPtr - (pointer to) the CAPWAP UDP tunnel header
*       fieldVal - data of field to insert to header
*       fieldId -- field id
*
* OUTPUTS:
*       capwapUdpTunnelHeaderPtr - (pointer to) the updated CAPWAP UDP tunnel header
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID snetCht3TsCapwapUdpTunnelHeaderFieldBuildByValue
(
    INOUT SNET_CHT_POLICY_KEY_STC           *capwapUdpTunnelHeaderPtr,
    IN GT_U32                               fieldVal,
    IN CHT3_TS_CAPWAP_UDP_TUNNEL_HEADER_ENT fieldId
)
{
    CHT_PCL_KEY_FIELDS_INFO_STC *fieldInfoPtr = &cht3TsCapwapUdpTunnelHeaderFields[fieldId];

    snetChtPclSrvKeyFieldBuildByValueNetworkOrder(capwapUdpTunnelHeaderPtr, fieldVal, fieldInfoPtr);

    return;
}
/*******************************************************************************
*   snetCht3TsCapwapUdpTunnelHeaderGenerate
*
* DESCRIPTION:
*   how the UDP header is generated for packets forwarded over a CAPWAP-over-IPv4
*   tunnel, and how the UDP-Lite is generated for packets forwarded over a
*   CAPWAP-over-IPv6 tunnel. Note that the Tunnel-start (TS) entry determines
*   some of the UDP fields are generated.
*
* INPUTS:
*        tsAddr - TS CAPWAP memory address.
*        descrPtr     - pointer to the frame's descriptor.
*        tsCapwapPtr  - pointer to the TS CAPWAP entry.
*        egressBufferPtr - egress buffer to put the header into
*        firstPhase - on first phase we set the src,dst ports
*                     on second phase we can calculate the checksum and the length.
*        udpCheckSum - used only on second phase.
*                     this is the UDP payload checksum.
*        udpLength - used only on second phase.
*                    relevant only for IPv4 frames.
*                    this is the UDP payload length. - number of bytes.
*
* OUTPUTS:
*       descrPtr     - pointer to updated frame data buffer Id
*
*   RETURNS:
*       pointer to first byte in the buffer that was not modified.
*
*******************************************************************************/
static GT_U8* snetCht3TsCapwapUdpTunnelHeaderGenerate
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    OUT SNET_CHT3_TS_CAPWAP_STC *tsCapwapPtr,
    IN GT_U8*      egressBufferPtr,
    IN HEADER_BUILD_STAGE_ENT     stage,
    IN GT_U16      udpCheckSum,
    IN GT_U16      udpLength
)
{
    DECLARE_FUNC_NAME(snetCht3TsCapwapUdpTunnelHeaderGenerate);

    CHT3_TS_CAPWAP_UDP_TUNNEL_HEADER_ENT fieldIndex;/* index of current field */
    SNET_CHT_POLICY_KEY_STC udpHeaderInfo;
    GT_U32      maxBytesToWrite = 8;/* max number of bytes of the header */
    GT_BOOL     isIpv4 = GT_FALSE;

    udpHeaderInfo.pclKeyFormat = CHT_PCL_KEY_UNLIMITED_SIZE_E;
    udpHeaderInfo.updateOnlyDiff = GT_FALSE;
    udpHeaderInfo.key.unlimitedBufferPtr = egressBufferPtr;
    udpHeaderInfo.devObjPtr = devObjPtr;

    switch(tsCapwapPtr->tunnelType)
    {
        case 2:
            isIpv4 = GT_TRUE; /* ipv4 */
            break;
        case 3:
            isIpv4 = GT_FALSE;/* ipv6 */
            break;
        default:
            skernelFatalError(" snetCht3TsCapwapUdpTunnelHeaderGenerate:should not happen \n" );
    }

    if(stage == HEADER_BUILD_STAGE_1_E)
    {
        fieldIndex = CHT3_TS_CAPWAP_UDP_TUNNEL_HEADER_FIELD_UDP_SRC_PORT_E;
        snetCht3TsCapwapUdpTunnelHeaderFieldBuildByValue(&udpHeaderInfo, tsCapwapPtr->udpSrcPort,
                                        fieldIndex);

        fieldIndex = CHT3_TS_CAPWAP_UDP_TUNNEL_HEADER_FIELD_UDP_DST_PORT_E;
        snetCht3TsCapwapUdpTunnelHeaderFieldBuildByValue(&udpHeaderInfo, tsCapwapPtr->udpDstPort,
                                        fieldIndex);
        if(isIpv4 == GT_TRUE)
        {
            fieldIndex = CHT3_TS_CAPWAP_UDP_TUNNEL_HEADER_FIELD_UDP_CHECKSUM_E;
            snetCht3TsCapwapUdpTunnelHeaderFieldBuildByValue(&udpHeaderInfo, 0,/* clear the checksum on fist stage */
                                            fieldIndex);
        }
        else
        {
            fieldIndex = CHT3_TS_CAPWAP_UDP_TUNNEL_HEADER_FIELD_UDP_LITE_CHECKSUM_E;
            snetCht3TsCapwapUdpTunnelHeaderFieldBuildByValue(&udpHeaderInfo, 0 ,/* clear the checksum on fist stage */
                                            fieldIndex);
        }
    }
    else  /* phase 2/3 */
    {
        if(isIpv4 == GT_TRUE)
        {
            if(stage == HEADER_BUILD_STAGE_2_E)
            {
                /* "UDP" header */
                __LOG(("UDP header"));
                fieldIndex = CHT3_TS_CAPWAP_UDP_TUNNEL_HEADER_FIELD_UDP_LENGTH_E;
                snetCht3TsCapwapUdpTunnelHeaderFieldBuildByValue(&udpHeaderInfo, udpLength ,
                                                fieldIndex);
            }
            else
            {
                fieldIndex = CHT3_TS_CAPWAP_UDP_TUNNEL_HEADER_FIELD_UDP_CHECKSUM_E;
                snetCht3TsCapwapUdpTunnelHeaderFieldBuildByValue(&udpHeaderInfo, udpCheckSum ,
                                                fieldIndex);
            }
        }
        else
        {
            if(stage == HEADER_BUILD_STAGE_2_E)
            {
                /* "UDP-Lite" header */
                __LOG(("UDP-Lite header"));
                fieldIndex = CHT3_TS_CAPWAP_UDP_TUNNEL_HEADER_FIELD_UDP_LITE_CHECKSUM_COVERAGE_E;
                snetCht3TsCapwapUdpTunnelHeaderFieldBuildByValue(&udpHeaderInfo, UDP_LITE_CHECKSUM_LENGTH ,/* fixed value */
                                                fieldIndex);
            }
            else
            {
                fieldIndex = CHT3_TS_CAPWAP_UDP_TUNNEL_HEADER_FIELD_UDP_LITE_CHECKSUM_E;
                snetCht3TsCapwapUdpTunnelHeaderFieldBuildByValue(&udpHeaderInfo, udpCheckSum ,
                                                fieldIndex);
            }
        }
    }

    /* return the pointer to the next byte where to put what comes after the
       UDP header */
    return &egressBufferPtr[maxBytesToWrite];
}

/*******************************************************************************
*   snetCht3TsCapwapIpv4HeaderFieldBuildByValue
*
* DESCRIPTION:
*       function insert data of the field to the frame header.
*
* INPUTS:
*       capwapIpv4TunnelHeaderPtr - (pointer to) the CAPWAP IPv4 tunnel header
*       fieldVal - data of field to insert to header
*       fieldId -- field id
*
* OUTPUTS:
*       capwapIpv4TunnelHeaderPtr - (pointer to) the updated CAPWAP IPv4 tunnel header
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID snetCht3TsCapwapIpv4HeaderFieldBuildByValue
(
    INOUT SNET_CHT_POLICY_KEY_STC           *capwapIpv4TunnelHeaderPtr,
    IN GT_U32                               fieldVal,
    IN CHT3_TS_CAPWAP_IPV4_HEADER_ENT       fieldId
)
{
    CHT_PCL_KEY_FIELDS_INFO_STC *fieldInfoPtr = &cht3TsCapwapIpv4HeaderFields[fieldId];

    snetChtPclSrvKeyFieldBuildByValueNetworkOrder(capwapIpv4TunnelHeaderPtr, fieldVal, fieldInfoPtr);

    return;
}

/*******************************************************************************
*   snetCht3TsCapwapIpv4HeaderFieldBuildByPointer
*
* DESCRIPTION:
*       function insert data of the field to the frame header.
*
* INPUTS:
*       capwapIpv4TunnelHeaderPtr - (pointer to) the CAPWAP IPv4 tunnel header
*       fieldValPtr - (pointer to) data of field to insert to header
*       fieldId -- field id
*
* OUTPUTS:
*       capwapIpv4TunnelHeaderPtr - (pointer to) the updated CAPWAP IPv4 tunnel header
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID snetCht3TsCapwapIpv4HeaderFieldBuildByPointer
(
    INOUT SNET_CHT_POLICY_KEY_STC           *capwapIpv4TunnelHeaderPtr,
    IN GT_U8                                *fieldValPtr,
    IN CHT3_TS_CAPWAP_IPV4_HEADER_ENT       fieldId
)
{
    CHT_PCL_KEY_FIELDS_INFO_STC *fieldInfoPtr = &cht3TsCapwapIpv4HeaderFields[fieldId];

    snetChtPclSrvKeyFieldBuildByPointerNetworkOrder(capwapIpv4TunnelHeaderPtr, fieldValPtr, fieldInfoPtr);

    return;
}

/*******************************************************************************
*   snetCht3TsCapwapIpv6HeaderFieldBuildByValue
*
* DESCRIPTION:
*       function insert data of the field to the frame header.
*
* INPUTS:
*       capwapIpv4TunnelHeaderPtr - (pointer to) the CAPWAP IPv6 tunnel header
*       fieldVal - data of field to insert to header
*       fieldId -- field id
*
* OUTPUTS:
*       capwapIpv4TunnelHeaderPtr - (pointer to) the updated CAPWAP IPv6 tunnel header
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID snetCht3TsCapwapIpv6HeaderFieldBuildByValue
(
    INOUT SNET_CHT_POLICY_KEY_STC           *capwapIpv6TunnelHeaderPtr,
    IN GT_U32                               fieldVal,
    IN CHT3_TS_CAPWAP_IPV6_HEADER_ENT       fieldId
)
{
    CHT_PCL_KEY_FIELDS_INFO_STC *fieldInfoPtr = &cht3TsCapwapIpv6HeaderFields[fieldId];

    snetChtPclSrvKeyFieldBuildByValueNetworkOrder(capwapIpv6TunnelHeaderPtr, fieldVal, fieldInfoPtr);

    return;
}

/*******************************************************************************
*   snetCht3TsCapwapIpv6HeaderFieldBuildByPointer
*
* DESCRIPTION:
*       function insert data of the field to the frame header.
*
* INPUTS:
*       capwapIpv4TunnelHeaderPtr - (pointer to) the CAPWAP IPv6 tunnel header
*       fieldValPtr - (pointer to) data of field to insert to header
*       fieldId -- field id
*
* OUTPUTS:
*       capwapIpv4TunnelHeaderPtr - (pointer to) the updated CAPWAP IPv6 tunnel header
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID snetCht3TsCapwapIpv6HeaderFieldBuildByPointer
(
    INOUT SNET_CHT_POLICY_KEY_STC           *capwapIpv6TunnelHeaderPtr,
    IN GT_U8                                *fieldValPtr,
    IN CHT3_TS_CAPWAP_IPV6_HEADER_ENT       fieldId
)
{
    CHT_PCL_KEY_FIELDS_INFO_STC *fieldInfoPtr = &cht3TsCapwapIpv6HeaderFields[fieldId];

    snetChtPclSrvKeyFieldBuildByPointerNetworkOrder(capwapIpv6TunnelHeaderPtr, fieldValPtr, fieldInfoPtr);

    return;
}
/*******************************************************************************
*   snetCht3TsCapwapIpHeaderGenerate
*
* DESCRIPTION:
*   how the IPv4/6 header is generated for packets forwarded over a CAPWAP tunnel.
*   Note that the Tunnel start (TS) entry determines some of the IPv4/6 fields are
*   generated.
*
* INPUTS:
*        tsAddr - TS CAPWAP memory address.
*        descrPtr     - pointer to the frame's descriptor.
*        tsCapwapPtr  - pointer to the TS CAPWAP entry.
*        egressBufferPtr - egress buffer to put the header into
*        stage - on first phase we set most of the header
*                     on second phase we can calculate the checksum and the length.
*        ipv4CheckSum - used only on second phase.
*                     only for ipv4 frames.
*        totalLengthOrPayloadLen - used only on second phase.
*                    for ipv6 this is the payload len
*                    for ipv4 this is total length - number of bytes.
*
* OUTPUTS:
*       descrPtr     - pointer to updated frame data buffer Id
*
*   RETURNS:
*       pointer to first byte in the buffer that was not modified.
*
*******************************************************************************/
static GT_U8* snetCht3TsCapwapIpHeaderGenerate
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    OUT SNET_CHT3_TS_CAPWAP_STC *tsCapwapPtr,
    IN GT_U8*      egressBufferPtr,
    IN HEADER_BUILD_STAGE_ENT     stage,
    IN GT_U16      ipv4CheckSum,
    IN GT_U16      totalLengthOrPayloadLen
)
{
    DECLARE_FUNC_NAME(snetCht3TsCapwapIpHeaderGenerate);

    CHT3_TS_CAPWAP_IPV4_HEADER_ENT fieldIndexIpv4;/* index of current field */
    CHT3_TS_CAPWAP_IPV6_HEADER_ENT fieldIndexIpv6;/* index of current field */
    SNET_CHT_POLICY_KEY_STC ipHeaderInfo;
    GT_U32      tmpValue;
    GT_U32      maxBytesToWriteIpv4 = 20;/* max number of bytes of the ipv4 header */
    GT_U32      maxBytesToWriteIpv6 = 40;/* max number of bytes of the ipv6 header */
    GT_U32      maxBytesToWrite = 0 ;/* max number of bytes of the header */
    GT_BOOL     isIpv4 =GT_FALSE;
    GT_U32      regAddr;
    GT_U32      qosProfileEntry;         /* QOS Profile entry */
    GT_U32      tmpDscp=0;/* DSCP from the QOS Profile entry */

    ipHeaderInfo.pclKeyFormat = CHT_PCL_KEY_UNLIMITED_SIZE_E;
    ipHeaderInfo.updateOnlyDiff = GT_FALSE;
    ipHeaderInfo.key.unlimitedBufferPtr = egressBufferPtr;
    ipHeaderInfo.devObjPtr = devObjPtr;

    switch(tsCapwapPtr->tunnelType)
    {
        case 2:
            isIpv4 = GT_TRUE; /* ipv4 */
            maxBytesToWrite = maxBytesToWriteIpv4;
            break;
        case 3:
            isIpv4 = GT_FALSE;/* ipv6 */
            maxBytesToWrite = maxBytesToWriteIpv6;
            break;
        default:
            skernelFatalError(" snetCht3TsCapwapIpHeaderGenerate:should not happen \n" );
    }

    if(stage == HEADER_BUILD_STAGE_1_E)
    {
        if(tsCapwapPtr->dscpMarkingMode == 0)
        {
            regAddr = SMEM_CHT_QOS_TBL_MEM(devObjPtr, descrPtr->qos.qosProfile);

            /* QoSProfile to QoS table Entry */
            __LOG(("QoSProfile to QoS table Entry"));
            smemRegGet(devObjPtr, regAddr, &qosProfileEntry);

            /* QoS Profile DSCP */
            __LOG(("QoS Profile DSCP"));
            tmpDscp = SMEM_U32_GET_FIELD(qosProfileEntry, 0, 6);
        }

        if(isIpv4 == GT_TRUE)
        {
            fieldIndexIpv4 = CHT3_TS_CAPWAP_IPV4_HEADER_FIELD_VERSION_E;
            snetCht3TsCapwapIpv4HeaderFieldBuildByValue(&ipHeaderInfo, 4,/* fixed value */
                                            fieldIndexIpv4);

            fieldIndexIpv4 = CHT3_TS_CAPWAP_IPV4_HEADER_FIELD_IHL_E;
            snetCht3TsCapwapIpv4HeaderFieldBuildByValue(&ipHeaderInfo, 5,/* fixed value */
                                            fieldIndexIpv4);

            fieldIndexIpv4 = CHT3_TS_CAPWAP_IPV4_HEADER_FIELD_TOS_E;
            tmpValue = tsCapwapPtr->dscpMarkingMode ?
                            tsCapwapPtr->dscp :
                            tmpDscp ;
            /* update the descriptor too */
/*            descrPtr->modifyDscp = 1;
            descrPtr->dscp = tmpDscp;*/

            tmpValue <<= 2;/* DSCP is msb of TOS */

            snetCht3TsCapwapIpv4HeaderFieldBuildByValue(&ipHeaderInfo, tmpValue,
                                            fieldIndexIpv4);

            fieldIndexIpv4 = CHT3_TS_CAPWAP_IPV4_HEADER_FIELD_IDENTIFICATION_E;
            snetCht3TsCapwapIpv4HeaderFieldBuildByValue(&ipHeaderInfo, 0,/* fixed value */
                                            fieldIndexIpv4);

            fieldIndexIpv4 = CHT3_TS_CAPWAP_IPV4_HEADER_FIELD_RESERVED_E;
            snetCht3TsCapwapIpv4HeaderFieldBuildByValue(&ipHeaderInfo, 0,/* fixed value */
                                            fieldIndexIpv4);

            fieldIndexIpv4 = CHT3_TS_CAPWAP_IPV4_HEADER_FIELD_DONT_FRAG_E;
            snetCht3TsCapwapIpv4HeaderFieldBuildByValue(&ipHeaderInfo, tsCapwapPtr->dontFragmentFlag,
                                            fieldIndexIpv4);

            fieldIndexIpv4 = CHT3_TS_CAPWAP_IPV4_HEADER_FIELD_MORE_FRAG_E;
            snetCht3TsCapwapIpv4HeaderFieldBuildByValue(&ipHeaderInfo, 0,/* not simulate fragments */
                                            fieldIndexIpv4);

            fieldIndexIpv4 = CHT3_TS_CAPWAP_IPV4_HEADER_FIELD_FRAG_OFFSET_E;
            snetCht3TsCapwapIpv4HeaderFieldBuildByValue(&ipHeaderInfo, 0,/* not simulate fragments */
                                            fieldIndexIpv4);

            fieldIndexIpv4 = CHT3_TS_CAPWAP_IPV4_HEADER_FIELD_TTL_E;
            snetCht3TsCapwapIpv4HeaderFieldBuildByValue(&ipHeaderInfo, tsCapwapPtr->ttlOrHotLimit,
                                            fieldIndexIpv4);

            fieldIndexIpv4 = CHT3_TS_CAPWAP_IPV4_HEADER_FIELD_PROTOCOL_E;
            snetCht3TsCapwapIpv4HeaderFieldBuildByValue(&ipHeaderInfo, SNET_UDP_PROT_E,/* fixed value */
                                            fieldIndexIpv4);

            fieldIndexIpv4 = CHT3_TS_CAPWAP_IPV4_HEADER_FIELD_SIP_E;
            snetCht3TsCapwapIpv4HeaderFieldBuildByPointer(&ipHeaderInfo, tsCapwapPtr->sip,
                                            fieldIndexIpv4);

            fieldIndexIpv4 = CHT3_TS_CAPWAP_IPV4_HEADER_FIELD_DIP_E;
            snetCht3TsCapwapIpv4HeaderFieldBuildByPointer(&ipHeaderInfo, tsCapwapPtr->dip,
                                            fieldIndexIpv4);

            fieldIndexIpv4 = CHT3_TS_CAPWAP_IPV4_HEADER_FIELD_HEADER_CHECKSUM_E;
            snetCht3TsCapwapIpv4HeaderFieldBuildByValue(&ipHeaderInfo, 0,/* clear the checksum on fist stage */
                                            fieldIndexIpv4);
        }
        else /* IPv6 */
        {
            fieldIndexIpv6 = CHT3_TS_CAPWAP_IPV6_HEADER_FIELD_VERSION_E;
            snetCht3TsCapwapIpv6HeaderFieldBuildByValue(&ipHeaderInfo, 6,/* fixed value */
                                            fieldIndexIpv6);

            fieldIndexIpv6 = CHT3_TS_CAPWAP_IPV6_HEADER_FIELD_TRAFFIC_CLASS_E;
            tmpValue = tsCapwapPtr->dscpMarkingMode ?
                            tsCapwapPtr->dscp :
                            tmpDscp ;
            /* update the descriptor too */
/*            descrPtr->modifyDscp = 1;
            descrPtr->dscp = tmpDscp;*/

            tmpValue <<= 2;/* DSCP is msb of TOS */

            snetCht3TsCapwapIpv6HeaderFieldBuildByValue(&ipHeaderInfo, tmpValue,
                                            fieldIndexIpv6);

            fieldIndexIpv6 = CHT3_TS_CAPWAP_IPV6_HEADER_FIELD_FLOW_E;
            snetCht3TsCapwapIpv6HeaderFieldBuildByValue(&ipHeaderInfo, 0,/* fixed value */
                                            fieldIndexIpv6);

            fieldIndexIpv6 = CHT3_TS_CAPWAP_IPV6_HEADER_FIELD_NEXT_HEADER_E;
            snetCht3TsCapwapIpv6HeaderFieldBuildByValue(&ipHeaderInfo, SNET_UDP_LITE_PROT_E,/* fixed value */
                                            fieldIndexIpv6);

            fieldIndexIpv6 = CHT3_TS_CAPWAP_IPV6_HEADER_FIELD_HOP_LIMIT_E;
            snetCht3TsCapwapIpv6HeaderFieldBuildByValue(&ipHeaderInfo, tsCapwapPtr->ttlOrHotLimit,
                                            fieldIndexIpv6);

            fieldIndexIpv6 = CHT3_TS_CAPWAP_IPV6_HEADER_FIELD_SIP_E;
            snetCht3TsCapwapIpv6HeaderFieldBuildByPointer(&ipHeaderInfo, tsCapwapPtr->sip,
                                            fieldIndexIpv6);

            fieldIndexIpv6 = CHT3_TS_CAPWAP_IPV6_HEADER_FIELD_DIP_E;
            snetCht3TsCapwapIpv6HeaderFieldBuildByPointer(&ipHeaderInfo, tsCapwapPtr->dip,
                                            fieldIndexIpv6);
        }
    }
    else /* phase 2,3*/
    {
        if(isIpv4 == GT_TRUE)
        {
            if(stage == HEADER_BUILD_STAGE_2_E)
            {
                fieldIndexIpv4 = CHT3_TS_CAPWAP_IPV4_HEADER_FIELD_TOTAL_LEN_E;
                snetCht3TsCapwapIpv4HeaderFieldBuildByValue(&ipHeaderInfo, totalLengthOrPayloadLen,
                                                fieldIndexIpv4);
            }
            else
            {
                fieldIndexIpv4 = CHT3_TS_CAPWAP_IPV4_HEADER_FIELD_HEADER_CHECKSUM_E;
                snetCht3TsCapwapIpv4HeaderFieldBuildByValue(&ipHeaderInfo, ipv4CheckSum,
                                                fieldIndexIpv4);
            }
        }
        else /* IPv6 */
        {
            if(stage == HEADER_BUILD_STAGE_2_E)
            {
                fieldIndexIpv6 = CHT3_TS_CAPWAP_IPV6_HEADER_FIELD_PAYLOAD_LEN_E;
                snetCht3TsCapwapIpv6HeaderFieldBuildByValue(&ipHeaderInfo, totalLengthOrPayloadLen,
                                                fieldIndexIpv6);
            }
            else
            {
                /* no need for stage 3 */
                __LOG(("no need for stage 3"));
            }

        }
    }

    /* return the pointer to the next byte where to put what comes after the
       IPv4/6 header */
    return &egressBufferPtr[maxBytesToWrite];
}

/*******************************************************************************
*   snetCht3TsCapwapEthernetHeaderFieldBuildByValue
*
* DESCRIPTION:
*       function insert data of the field to the frame header.
*
* INPUTS:
*       capwapEthernetTunnelHeaderPtr - (pointer to) the CAPWAP Ethernet tunnel header
*       fieldVal - data of field to insert to header
*       fieldId -- field id
*
* OUTPUTS:
*       capwapEthernetTunnelHeaderPtr - (pointer to) the updated CAPWAP Ethernet tunnel header
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID snetCht3TsCapwapEthernetHeaderFieldBuildByValue
(
    INOUT SNET_CHT_POLICY_KEY_STC           *capwapEthernetTunnelHeaderPtr,
    IN GT_U32                               fieldVal,
    IN CHT3_TS_CAPWAP_ETHERNET_HEADER_ENT   fieldId
)
{
    CHT_PCL_KEY_FIELDS_INFO_STC *fieldInfoPtr = &cht3TsCapwapEthernetHeaderFields[fieldId];

    snetChtPclSrvKeyFieldBuildByValueNetworkOrder(capwapEthernetTunnelHeaderPtr, fieldVal, fieldInfoPtr);

    return;
}

/*******************************************************************************
*   snetCht3TsCapwapEthernetHeaderFieldBuildByPointer
*
* DESCRIPTION:
*       function insert data of the field to the frame header.
*
* INPUTS:
*       capwapEthernetTunnelHeaderPtr - (pointer to) the CAPWAP Ethernet tunnel header
*       fieldValPtr - (pointer to) data of field to insert to header
*       fieldId -- field id
*
* OUTPUTS:
*       capwapEthernetTunnelHeaderPtr - (pointer to) the updated CAPWAP Ethernet tunnel header
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID snetCht3TsCapwapEthernetHeaderFieldBuildByPointer
(
    INOUT SNET_CHT_POLICY_KEY_STC           *capwapEthernetTunnelHeaderPtr,
    IN GT_U8                                *fieldValPtr,
    IN CHT3_TS_CAPWAP_ETHERNET_HEADER_ENT   fieldId
)
{
    CHT_PCL_KEY_FIELDS_INFO_STC *fieldInfoPtr = &cht3TsCapwapEthernetHeaderFields[fieldId];

    snetChtPclSrvKeyFieldBuildByPointerNetworkOrder(capwapEthernetTunnelHeaderPtr, fieldValPtr, fieldInfoPtr);

    return;
}

/*******************************************************************************
*   snetCht3TsCapwapEthernetHeaderGenerate
*
* DESCRIPTION:
*   how the Ethernet header is generated for packets forwarded over a CAPWAP
*   tunnel.
*   Note that the Tunnel-start (TS) entry determines some of the Ethernet fields
*   are generated.
*
* INPUTS:
*        tsAddr - TS CAPWAP memory address.
*        descrPtr     - pointer to the frame's descriptor.
*        tsCapwapPtr  - pointer to the TS CAPWAP entry.
*        egressBufferPtr - egress buffer to put the header into
*
* OUTPUTS:
*       descrPtr     - pointer to updated frame data buffer Id
*
*   RETURNS:
*       pointer to first byte in the buffer that was not modified.
*
*******************************************************************************/
static GT_U8* snetCht3TsCapwapEthernetHeaderGenerate
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    OUT SNET_CHT3_TS_CAPWAP_STC *tsCapwapPtr,
    IN GT_U8*      egressBufferPtr
)
{
    DECLARE_FUNC_NAME(snetCht3TsCapwapEthernetHeaderGenerate);

    CHT3_TS_CAPWAP_IPV6_HEADER_ENT fieldIndex;/* index of current field */
    SNET_CHT_POLICY_KEY_STC ethernetHeaderInfo;
    GT_U32      tmpValue;
    GT_U32      numBytesOffset;
    GT_U32      maxBytesToWrite = 18;/* max number of bytes of the header */
    GT_U8       macFromMe[6];
    GT_U32      vlanEtherType,protocolEtherType =0;
    GT_U32      regAddr;
    GT_U32      qosProfileEntry;         /* QOS Profile entry */

    ethernetHeaderInfo.pclKeyFormat = CHT_PCL_KEY_UNLIMITED_SIZE_E;
    ethernetHeaderInfo.updateOnlyDiff = GT_FALSE;
    ethernetHeaderInfo.key.unlimitedBufferPtr = egressBufferPtr;
    ethernetHeaderInfo.devObjPtr = devObjPtr;

    /* update the descriptor too */
/*    descrPtr->macDaPtr = &egressBufferPtr[0];
    descrPtr->macSaPtr = &egressBufferPtr[6];*/

    fieldIndex = CHT3_TS_CAPWAP_ETHERNET_HEADER_FIELD_DST_MAC_E;
    snetCht3TsCapwapEthernetHeaderFieldBuildByPointer(&ethernetHeaderInfo, tsCapwapPtr->nextHopMacDa,
                                    fieldIndex);

    snetChtHaMacFromMeBuild(devObjPtr,descrPtr,descrPtr->trgEPort,GT_TRUE,macFromMe);

    fieldIndex = CHT3_TS_CAPWAP_ETHERNET_HEADER_FIELD_SRC_MAC_E;
    snetCht3TsCapwapEthernetHeaderFieldBuildByPointer(&ethernetHeaderInfo, macFromMe,
                                    fieldIndex);

    switch(tsCapwapPtr->tunnelType)
    {
        case 2:
            protocolEtherType = SKERNEL_L3_PROT_TYPE_IPV4_E;
            break;
        case 3:
            protocolEtherType = SKERNEL_L3_PROT_TYPE_IPV6_E;
            break;
        default:
            skernelFatalError(" snetCht3TsCapwapEthernetHeaderGenerate: should not happen \n");
    }

    if(tsCapwapPtr->tagEnable == 0)
    {
        vlanEtherType = protocolEtherType;

        numBytesOffset = 4;
    }
    else  /* use Port VLAN EtherType */
    {
        __LOG(("use Port VLAN EtherType"));
        numBytesOffset = 0;

        /* Select the VLAN EtherType for tagged packets transmitted via this port */
        __LOG(("Select the VLAN EtherType for tagged packets transmitted via this port"));
        smemRegFldGet(devObjPtr, SMEM_CHT_EGR_VLAN_ETH_TYPE_SELECT_REG(devObjPtr, descrPtr->trgEPort),
                    descrPtr->trgEPort, 1, &tmpValue);

        /* Select the VLAN EtherType for tagged packets transmitted via this port */
        __LOG(("Select the VLAN EtherType for tagged packets transmitted via this port"));
        smemRegFldGet(devObjPtr, SMEM_CHT_EGR_VLAN_ETH_TYPE_REG(devObjPtr), tmpValue * 16, 16,
                    &vlanEtherType);
    }

    fieldIndex = CHT3_TS_CAPWAP_ETHERNET_HEADER_FIELD_ETHER_TYPE_E;
    tmpValue = vlanEtherType;
    snetCht3TsCapwapEthernetHeaderFieldBuildByValue(&ethernetHeaderInfo, tmpValue,
                                    fieldIndex);
    /* update the descriptor too */
/*    descrPtr->etherTypeOrSsapDsap = tmpValue;*/

    if(tsCapwapPtr->tagEnable == 1)
    {
        /*haInfoPtr->tsVlanTagInfo.vlanTagged = 1;*/

        fieldIndex = CHT3_TS_CAPWAP_ETHERNET_HEADER_FIELD_VPT_E;
        if(tsCapwapPtr->upMarkingMode)
        {
            tmpValue = tsCapwapPtr->up;
        }
        else
        {
            regAddr = SMEM_CHT_QOS_TBL_MEM(devObjPtr, descrPtr->qos.qosProfile);

            /* QoSProfile to QoS table Entry */
            __LOG(("QoSProfile to QoS table Entry"));
            smemRegGet(devObjPtr, regAddr, &qosProfileEntry);

            /* QoS Profile UP */
            __LOG(("QoS Profile UP"));
            tmpValue = SMEM_U32_GET_FIELD(qosProfileEntry, 6, 3);
        }
        snetCht3TsCapwapEthernetHeaderFieldBuildByValue(&ethernetHeaderInfo, tmpValue,
                                        fieldIndex);
        /* update the descriptor too */
        __LOG(("update the descriptor too"));
        descrPtr->up = (GT_U8)tmpValue;

        tmpValue = 0; /* fixed value*/
        fieldIndex = CHT3_TS_CAPWAP_ETHERNET_HEADER_FIELD_CFI_E;
        snetCht3TsCapwapEthernetHeaderFieldBuildByValue(&ethernetHeaderInfo, tmpValue,
                                        fieldIndex);
        /* update the descriptor too */
        descrPtr->cfidei = (GT_U8)tmpValue;

        tmpValue = tsCapwapPtr->vid;
        fieldIndex = CHT3_TS_CAPWAP_ETHERNET_HEADER_FIELD_VID_E;
        snetCht3TsCapwapEthernetHeaderFieldBuildByValue(&ethernetHeaderInfo, tmpValue,
                                        fieldIndex);
        /* update the descriptor too */
        descrPtr->eVid = (GT_U16)tmpValue;


        fieldIndex = CHT3_TS_CAPWAP_ETHERNET_HEADER_FIELD_ETHER_TYPE_2_E;
        tmpValue = protocolEtherType;
        snetCht3TsCapwapEthernetHeaderFieldBuildByValue(&ethernetHeaderInfo, tmpValue,
                                        fieldIndex);
        /* update the descriptor too */
/*        descrPtr->etherTypeOrSsapDsap = tmpValue;*/
    }

    /* return the pointer to the next byte where to put what comes after the
       L2 mac header */
    return &egressBufferPtr[maxBytesToWrite - numBytesOffset];
}

/*******************************************************************************
*   snetCht3TsCapwap802dot3MacHeaderGenerate
*
* DESCRIPTION:
*   build the Mac header of 802.3 frames that encapsulated in the CAPWAP header
*
* INPUTS:
*        tsAddr - TS CAPWAP memory address.
*        descrPtr     - pointer to the frame's descriptor.
*        tsCapwapPtr  - pointer to the TS CAPWAP entry.
*        egressBufferPtr - egress buffer to put the header into
* OUTPUTS:
*       descrPtr     - pointer to updated frame data buffer Id
*
*   RETURNS:
*       pointer to first byte in the buffer that was not modified.
*
*******************************************************************************/
static GT_U8* snetCht3TsCapwap802dot3MacHeaderGenerate
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    OUT SNET_CHT3_TS_CAPWAP_STC *tsCapwapPtr,
    IN GT_U8*      egressBufferPtr
)
{
    CHT3_TS_CAPWAP_ETHERNET_HEADER_ENT fieldIndex;/* index of current field */
    SNET_CHT_POLICY_KEY_STC capwap802dot3MacHeaderInfo;
    GT_U32      numBytesOffset;
    GT_U32      tmpValue;
    GT_U32      maxBytesToWrite = 18;/* max number of bytes of the header */

    capwap802dot3MacHeaderInfo.pclKeyFormat = CHT_PCL_KEY_UNLIMITED_SIZE_E;
    capwap802dot3MacHeaderInfo.updateOnlyDiff = GT_FALSE;
    capwap802dot3MacHeaderInfo.key.unlimitedBufferPtr = egressBufferPtr;
    capwap802dot3MacHeaderInfo.devObjPtr = devObjPtr;

    fieldIndex = CHT3_TS_CAPWAP_ETHERNET_HEADER_FIELD_DST_MAC_E;
    snetCht3TsCapwapEthernetHeaderFieldBuildByPointer(&capwap802dot3MacHeaderInfo, descrPtr->macDaPtr,
                                    fieldIndex);

    fieldIndex = CHT3_TS_CAPWAP_ETHERNET_HEADER_FIELD_SRC_MAC_E;
    snetCht3TsCapwapEthernetHeaderFieldBuildByPointer(&capwap802dot3MacHeaderInfo, descrPtr->macSaPtr,
                                    fieldIndex);

    numBytesOffset = 4;

    fieldIndex = CHT3_TS_CAPWAP_ETHERNET_HEADER_FIELD_ETHER_TYPE_E;
    if(descrPtr->trgTagged)
    {
        tmpValue = 0x8100;
    }
    else
    {
        tmpValue = descrPtr->etherTypeOrSsapDsap;
    }
    snetCht3TsCapwapEthernetHeaderFieldBuildByValue(&capwap802dot3MacHeaderInfo, tmpValue,
                                    fieldIndex);

    if(descrPtr->passengerTag)
    {
        numBytesOffset = 0;

        fieldIndex = CHT3_TS_CAPWAP_ETHERNET_HEADER_FIELD_VPT_E;
        snetCht3TsCapwapEthernetHeaderFieldBuildByValue(&capwap802dot3MacHeaderInfo, descrPtr->up,
                                        fieldIndex);

        fieldIndex = CHT3_TS_CAPWAP_ETHERNET_HEADER_FIELD_CFI_E;
        snetCht3TsCapwapEthernetHeaderFieldBuildByValue(&capwap802dot3MacHeaderInfo, 0,/* fixed value*/
                                        fieldIndex);

        fieldIndex = CHT3_TS_CAPWAP_ETHERNET_HEADER_FIELD_VID_E;
        snetCht3TsCapwapEthernetHeaderFieldBuildByValue(&capwap802dot3MacHeaderInfo, descrPtr->eVid,
                                        fieldIndex);

        fieldIndex = CHT3_TS_CAPWAP_ETHERNET_HEADER_FIELD_ETHER_TYPE_2_E;
        tmpValue = descrPtr->etherTypeOrSsapDsap;
        snetCht3TsCapwapEthernetHeaderFieldBuildByValue(&capwap802dot3MacHeaderInfo, tmpValue,
                                        fieldIndex);
    }

    /* return the pointer to the next byte where to put what comes after the
       mac header (the payload) */
    return &egressBufferPtr[maxBytesToWrite - numBytesOffset];
}

/*******************************************************************************
*   snetCht3HaTunnelStartCapwap
*
* DESCRIPTION:
*        Tunnel start for CAPWAP frames - entry point .
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - pointer to the frame's descriptor.
* OUTPUTS:
*       descrPtr     - pointer to updated frame data buffer Id
*
* RETURN:
*
*******************************************************************************/
GT_VOID snetCht3HaTunnelStartCapwap
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetCht3HaTunnelStartCapwap);

    GT_U32                  tsAddr;
    SNET_CHT3_TS_CAPWAP_STC tsCapwap;
    GT_U8                   *nextBufferPtr;
    GT_U8                   *ipHeaderPtr;
    GT_U8                   *udpHeaderPtr;
    GT_U8                   *egressPayloadPtr;
    GT_U8                   *ingressPayloadPtr;
    GT_U32                  payloadLength;
    GT_BOOL                 isIpv4;
    GT_U16                  ipLen;
    GT_U16                  udpLen = 0;
    GT_U32                  totalFrameLength;
    GT_U16                  checksum;
    GT_U16                  udpChecksumLen;
    GT_U32                  regVal;
    GT_U32                  startBit;
    GT_U32                  ii;
    GT_U32                  totalTunnelStartLength;
    GT_U8                   *origEgressPtr;
    GT_U32                  skippedLength;
/*    GT_U32                  etherTypeOrSsapDsap,tmpValue;*/


    if(descrPtr->tunnelStart == 0)
    {
        skernelFatalError(" snetCht3HaTunnelStartCapwap: tunnel start not needed \n");
    }

    tsAddr = SMEM_CHT2_TUNNEL_START_TBL_MEM(devObjPtr, descrPtr->tunnelPtr);

    /* read the Tunnel start entry */
    __LOG(("read the Tunnel start entry"));
    snetCht3TsCapwapEntryRead(devObjPtr,tsAddr,&tsCapwap);

    if(tsCapwap.enable802dot11wds == 0)
    {
        descrPtr->capwap.egressInfo.address1Ptr = descrPtr->macDaPtr;
        descrPtr->capwap.egressInfo.address2Ptr = tsCapwap.wds.bssidOrTa;
        descrPtr->capwap.egressInfo.address3Ptr = descrPtr->macSaPtr;
        descrPtr->capwap.egressInfo.address4Ptr = NULL;
    }
    else
    {
        descrPtr->capwap.egressInfo.address1Ptr = tsCapwap.wds.ra;
        descrPtr->capwap.egressInfo.address2Ptr = tsCapwap.wds.bssidOrTa;
        descrPtr->capwap.egressInfo.address3Ptr = descrPtr->macDaPtr;
        descrPtr->capwap.egressInfo.address4Ptr = descrPtr->macSaPtr;
    }


    if(descrPtr->capwap.egressInfo.daLookUpMatched == GT_FALSE)
    {
        descrPtr->capwap.egressInfo.tsOrDa802dot11eEgressEnable = tsCapwap.default802dot11eEnable;
        descrPtr->capwap.egressInfo.tsOrDa802dot11eEgressMappingTableProfile = tsCapwap._802dot11eMappingProfile;
    }

    /************************************************/
    /* start to build the TS frame header by header */
    /************************************************/

    /* start with the Ethernet header */
    __LOG(("start with the Ethernet header"));
    origEgressPtr = devObjPtr->egressBuffer1;
    nextBufferPtr = origEgressPtr;

    /* save the etherType */
/*    etherTypeOrSsapDsap = descrPtr->etherTypeOrSsapDsap;*/

    descrPtr->tunnelStartMacInfoPtr = origEgressPtr;
    descrPtr->tunnelStartMacInfoLen = 12;/* mac sa,da */

    nextBufferPtr = snetCht3TsCapwapEthernetHeaderGenerate(devObjPtr,descrPtr,&tsCapwap,nextBufferPtr);

    /* set point to the start of EtherType info */
    __LOG(("set point to the start of EtherType info"));
    descrPtr->tunnelStartRestOfHeaderInfoPtr = nextBufferPtr - 2;


    /* notify the caller about the egress buffer but without the L2 mac
       addresses , and vlan tag .
       because we let it build it by itself, using parameters from the descriptor
    */
    skippedLength = (nextBufferPtr - origEgressPtr) - 2;

    /* save the Ip header for next time */
    __LOG(("save the Ip header for next time"));
    ipHeaderPtr = nextBufferPtr;

    /* update the descriptor too */
/*    descrPtr->l3StartOffsetPtr = ipHeaderPtr;*/

    /* generate the IPv4/6 header */
    __LOG(("generate the IPv4/6 header"));

    /* call it for the first time -- no checksum,length done here */
    __LOG(("call it for the first time -- no checksum,length done here"));
    nextBufferPtr = snetCht3TsCapwapIpHeaderGenerate(devObjPtr,descrPtr,&tsCapwap,nextBufferPtr,
                        HEADER_BUILD_STAGE_1_E,0,0);


    /* save the UDP header for next time */
    __LOG(("save the UDP header for next time"));
    udpHeaderPtr = nextBufferPtr;

    /* update the descriptor too */
/*    descrPtr->l4StartOffsetPtr = ipHeaderPtr;*/

    /* generate the UDP/UDP-lite header */

    /* call it for the first time -- no checksum,length done here */
    nextBufferPtr = snetCht3TsCapwapUdpTunnelHeaderGenerate(devObjPtr,descrPtr,&tsCapwap,nextBufferPtr,
                        HEADER_BUILD_STAGE_1_E,0,0);

    /********************************/
    /* generate the CAPWAP header   */
    /* (include the CAPWAP preamble)*/
    /********************************/
    nextBufferPtr = snetCht3TsCapwapHeaderGenerate(devObjPtr,descrPtr,&tsCapwap,nextBufferPtr);

/*    tmpValue = descrPtr->etherTypeOrSsapDsap;
    descrPtr->etherTypeOrSsapDsap = etherTypeOrSsapDsap;*/
    if(tsCapwap.capwap.tBit)
    {
        /*************************/
        /* generate 802.11 frame */
        /*************************/
        if(descrPtr->capwap.egressInfo.tsOrDa802dot11eEgressEnable == GT_TRUE)
        {
            snetCht3Egress802dot11eFrame(devObjPtr,descrPtr);
        }

        nextBufferPtr = snetCht3TsCapwap802dot11MacHeaderGenerate(devObjPtr,descrPtr,&tsCapwap,nextBufferPtr);
        egressPayloadPtr = nextBufferPtr;
    }
    else
    {
        /************************/
        /* generate 802.3 frame */
        /************************/
        nextBufferPtr = snetCht3TsCapwap802dot3MacHeaderGenerate(devObjPtr,descrPtr,&tsCapwap,nextBufferPtr);
        egressPayloadPtr = nextBufferPtr;
    }
/*    descrPtr->etherTypeOrSsapDsap = tmpValue;*/

    ingressPayloadPtr = descrPtr->payloadPtr;

    /******************************/
    /* copy the rest of the frame */
    /******************************/
    payloadLength = descrPtr->payloadLength;
    memcpy(egressPayloadPtr,ingressPayloadPtr,payloadLength);

    totalTunnelStartLength = (egressPayloadPtr - origEgressPtr);

    /* calculate the different header length */
    __LOG(("calculate the different header length"));
    totalFrameLength = totalTunnelStartLength + payloadLength;

    switch(tsCapwap.tunnelType)
    {
        case 2:
            descrPtr->tunnelStartType = SKERNEL_FRAME_TUNNEL_START_TYPE_CAPWAP_E;
            isIpv4 = GT_TRUE; /* ipv4 */
            /* the length include the ipv4 header */
            ipLen = totalFrameLength - (ipHeaderPtr - origEgressPtr);
            /* the length include the UDP header */
            udpLen = totalFrameLength - (udpHeaderPtr - origEgressPtr);
            udpChecksumLen = udpLen;/* check sum on UDP header + it's data + pseudo IP header !! */

            if(udpChecksumLen & 1)
            {
                /* from RFC 791 */
                /* Checksum is the 16-bit one's complement of the one's
                   complement sum of a pseudo header of information from the IP
                   header, the UDP header, and the data,  padded  with zero octets
                   at the end (if  necessary)  to  make  a multiple of two octets. */

                udpHeaderPtr[udpChecksumLen++] = 0;

            }

            /* from RFC 791 */
            /* total of 12 bytes of the pseudo header */
            /*
                  0      7 8     15 16    23 24    31
                 +--------+--------+--------+--------+
                 |          source address           |
                 +--------+--------+--------+--------+
                 |        destination address        |
                 +--------+--------+--------+--------+
                 |  zero  |protocol|   UDP length    |
                 +--------+--------+--------+--------+
            */

            /* sip,dip */
            startBit = 12;
            for(ii=0 ; ii < 8 ; ii++,udpChecksumLen++,startBit++)
            {
                udpHeaderPtr[udpChecksumLen] = ipHeaderPtr[startBit];
            }

            udpHeaderPtr[udpChecksumLen++] = 0;
            udpHeaderPtr[udpChecksumLen++] = (GT_U8)SNET_UDP_PROT_E;
            udpHeaderPtr[udpChecksumLen++] = (GT_U8)(udpLen >> 8);
            udpHeaderPtr[udpChecksumLen++] = (GT_U8)(udpLen);

            /* total of 12 bytes of the pseudo header */
            __LOG(("total of 12 bytes of the pseudo header"));


            break;
        case 3:
            descrPtr->tunnelStartType = SKERNEL_FRAME_TUNNEL_START_TYPE_CAPWAP_E;
            isIpv4 = GT_FALSE;/* ipv6 */
            /* the length NOT include the ipv6 header */
            __LOG(("the length NOT include the ipv6 header"));
            ipLen = totalFrameLength - (udpHeaderPtr - origEgressPtr);
            udpChecksumLen = UDP_LITE_CHECKSUM_LENGTH;
            break;
        default:
            skernelFatalError(" snetCht3HaTunnelStartCapwap:should not happen \n" );
            return;
    }

    /* call it for the second time -- now we do length */
    __LOG(("call it for the second time -- now we do length"));
    snetCht3TsCapwapIpHeaderGenerate(devObjPtr,descrPtr,&tsCapwap,ipHeaderPtr,
                        HEADER_BUILD_STAGE_2_E,0,ipLen);

    /* call it for the second time -- now we do length */
    __LOG(("call it for the second time -- now we do length"));
    snetCht3TsCapwapUdpTunnelHeaderGenerate(devObjPtr,descrPtr,&tsCapwap,udpHeaderPtr,
                        HEADER_BUILD_STAGE_2_E,0,udpLen);

    checksum = checkSumCalc(udpHeaderPtr ,udpChecksumLen);
    /* call it for the third time -- now we do checksum */
    __LOG(("call it for the third time -- now we do checksum"));
    snetCht3TsCapwapUdpTunnelHeaderGenerate(devObjPtr,descrPtr,&tsCapwap,udpHeaderPtr,
                        HEADER_BUILD_STAGE_3_E,checksum,0);

    if(isIpv4 == GT_TRUE)
    {
        checksum = checkSumCalc(ipHeaderPtr,ipLen);
        /* call it for the third time -- now we do checksum */
        __LOG(("call it for the third time -- now we do checksum"));
        snetCht3TsCapwapIpHeaderGenerate(devObjPtr,descrPtr,&tsCapwap,ipHeaderPtr,
                            HEADER_BUILD_STAGE_3_E,checksum,0);
    }

    if(tsCapwap.egressOsmRedirect)
    {
        /* set if we need to send the frame to the CPU , and not to the specified port */
        descrPtr->egressOsmRedirect = 1;
        /* we need to trap it to the CPU */
        descrPtr->packetCmd = SKERNEL_EXT_PKT_CMD_TRAP_TO_CPU_E;

        /* get the <OSM CPU CODE> */
        smemRegGet(devObjPtr,SMEM_CHT3_OSM_CPU_CODE_TBL_MEM,&regVal);

        descrPtr->cpuCode   = SMEM_U32_GET_FIELD(regVal,0,8);
    }

    descrPtr->tunnelStartRestOfHeaderInfoLen = totalFrameLength - skippedLength;

    return;
}

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
)
{
    DECLARE_FUNC_NAME(snetCht3HaTunnelStart);

    SNET_CHT3_TS_CAPWAP_FIELDS_ENT    index;
    GT_U32                  tsAddr;
    GT_U32                  tunnelType;
    GT_U32                  tmpVal;
    GT_U32                  *tsCapwapEntryPtr;

    tsAddr = SMEM_CHT2_TUNNEL_START_TBL_MEM(devObjPtr, descrPtr->tunnelPtr);

    tsCapwapEntryPtr = smemMemGet(devObjPtr, tsAddr);

    index = SNET_CHT3_TS_CAPWAP_FIELD_TUNNEL_TYPE_E;
    tmpVal = snetFieldValueGet(tsCapwapEntryPtr,
        snetCh3TsCapwapEntry[index].startBit,
        (snetCh3TsCapwapEntry[index].lastBit - snetCh3TsCapwapEntry[index].startBit + 1));
    tunnelType = tmpVal;

    /* function not supported -- don't use it */
    __LOG(("function not supported -- don't use it"));


    switch(tunnelType)
    {
        case 2:
                snetCht3HaTunnelStartCapwap(devObjPtr,descrPtr);
            break;
        case 3:
            snetCht3HaTunnelStartCapwap(devObjPtr,descrPtr);
            break;
        default:
            skernelFatalError(" snetCht3HaTunnelStart: unknown TS type [%d]",tunnelType );
    }
}


