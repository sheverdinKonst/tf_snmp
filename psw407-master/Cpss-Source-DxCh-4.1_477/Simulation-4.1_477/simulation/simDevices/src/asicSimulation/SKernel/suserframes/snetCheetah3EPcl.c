/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetCheetah3EPcl.c
*
* DESCRIPTION:
*       Cheetah3 Asic Simulation .
*       Egress Policy Engine processing for outgoing frame.
*       Source Code file.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 47 $
*
*******************************************************************************/

#include <asicSimulation/SKernel/skernel.h>
#include <common/Utils/FrameInfo/sframeInfoAddr.h>
#include <asicSimulation/SKernel/smem/smemCheetah.h>
#include <asicSimulation/SKernel/cheetahCommon/sregCheetah.h>
#include <asicSimulation/SKernel/suserframes/snet.h>
#include <asicSimulation/SKernel/suserframes/snetCheetahPclSrv.h>
#include <asicSimulation/SKernel/suserframes/snetCheetah3EPcl.h>
#include <asicSimulation/SKernel/suserframes/snetCheetahEgress.h>
#include <asicSimulation/SKernel/suserframes/snetXCatPcl.h>
#include <asicSimulation/SLog/simLog.h>

/* array that holds the info about the fields */
static CHT_PCL_KEY_FIELDS_INFO_STC cht3ePclKeyFieldsData[CHT3_EPCL_KEY_FIELDS_ID_LAST_E]=
{
    {0  ,0  ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_VALID_E "  },
    {1  ,10 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_PCL_ID_E "  },
    {0  ,9  ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_PCL_ID_EXT_E"    },
    {17 ,23 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_QOS_PROFILE_6_0_EXT_E"  },
    {37 ,48 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_ORIG_VID_EXT_E"  },
    {51 ,57 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_SRC_DEV_TRUNK_EXT_E"  },
    {60 ,60 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_SRC_IS_TRUNK_EXT_E"  },
    {61 ,108 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_MAC_SA_EXT_E"  },
    {109,156 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_MAC_DA_EXT_E"  },
    {157,157 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_IS_L2_VALID_EXT_E"  },
    {158,158 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_IS_IP_EXT_E"  },
    {159,166 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_IP_PROTOCOL_EXT_E"  },
    {167,172 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_DSCP_EXT_E"  },
    {173,174 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_TOS_1_0_EXT_E"  },
    {175,182 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_TTL_EXT_E"  },
    {183,183 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_IS_ARP_EXT_E"  },
    {184,215 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_SIP_31_0_EXT_E"  },
    {216,247 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_DIP_31_0_EXT_E"  },
    {248,255 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_TCP_UDP_PORT_COMPS_EXT_E"  },
    {256,279 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_L4_BYTES_OFFSETS_0_1_13_EXT_E"  },
    {280,295 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_2_3_EXT_E"  },
    {280,295 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_4_5_EXT_E"  },
    {296,296 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_IS_L4_VALID_EXT_E"  },
    {297,299 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_TC_EXT_E"  },
    {300,301 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_DP_EXT_E"  },
    {302,303 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_PACKET_TYPE_EXT_E"  },
    {304,304 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_SRC_TRG_TX_MIRR_EXT_E"  },
    {305,307 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_ASSIGNED_UP_EXT_E"  },
    {308,313 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_TRG_PORT_EXT_E"  },
    {314,314 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_RX_SNIF_EXT_E"  },
    {315,315 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_IS_ROUTED_EXT_E"  },
    {316,316 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_IP_VER_EXT_E"  },
    {317,317 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_IPV4_OPT_EXT_E"  },
/* RACL/VACL (72B) IPv6 - Egress */
    {317,317 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_IPV6_EH_TRIPLE_E"  },
    {318,318 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_IPV6_IS_ND_TRIPLE_E"  },
    {319,319 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_IPV6_EH_HOP_TRIPLE_E"  },
    {320,415 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_SIP_127_32_TRIPLE_E"  },
    {416,511 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_DIP_127_32_TRIPLE_E"  },

    {11 ,16 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_SRC_PORT_E "  },
    {17 ,17 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_IS_TAGGED_E "  },
    {18 ,29 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_VID_E "  },
    {30 ,32 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_UP_E "  },
    {33 ,39 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_QOS_PROFILE_E "  },
    {40 ,40 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_IS_IPV4_E "  },
    {41 ,41 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_IS_IP_E "  },
    {42 ,42 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_RESERVED_42_XCAT2_41_E "  },
    {43 ,58 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_ETHER_TYPE_DSAP_SSAP_E "  },
    {59 ,66 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_RESERVED_66_59_XCAT2_65_58_SHORT_E "  },
    {67 ,72 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_RESERVED_72_67_XCAT2_71_66_E "  },
    {73 ,73 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_IS_ARP_E "  },
    {74 ,74 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_RESERVED_74_XCAT2_73_E "  },
    {82 ,82 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_SRC_IS_TRUNK_SHORT_E "},
    {88 ,88 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_IS_ROUTED "},
    {90 ,90 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_IS_L2_VALID_SHORT_E "  },
    {91 ,91 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_ENCAP_TYPE_SHORT_E "  },
    {92 ,139,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_MAC_SA_SHORT_E "  },
    {140,187,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_MAC_DA_SHORT_E "  },
    {188,189,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_PACKET_TYPE_SHORT_E "},
    {190,190,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_RESERVED_190_XCAT2_189_E "} ,
    {191,191,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_IS_VIDX_SHORT_E "},
    {42 ,49 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_IP_PROTOCOL_E "},
    {50 ,55 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_DSCP_E "} ,
    {56 ,56 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_IS_L4_VALID_E "},
    {57 ,72 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_2_3_E "},
    {57 ,72 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_4_5_E "},
    {91 ,98 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_TCP_UDP_PORT_COMPS_SHORT_E "},
    {99 ,130,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_DIP_31_0_E "},
    {131,138,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_L4_BYTES_OFFSETS_13_SHORT_E "},
    {139,139,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_RESERVED_139_XCAT2_138_SHORT_E "},
    {190,190,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_IPV4_FRAGMENTED_SHORT_E "}   ,
    {74 ,74 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_IS_BC_E "},
    {99 ,130,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_SIP_31_0_SHORT_E "}   ,
    {131,162,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_DIP_31_0_IPV4_L4_SHORT_E "},
    {163,186,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_L4_BYTES_OFFSETS_0_1_13_SHORT_E "} ,
    {187,187,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_RESERVED_187_XCAT2_186_E "},
    {40 ,40 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_IS_IPV6_E "}      ,
    {73 ,96 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_L4_BYTES_OFFSETS_0_1_13_LONG_E "},
    {97 ,128,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_SIP_31_0_LONG_E " },
    {129,160,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_DIP_31_0_LONG_E "},
    {161,161,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_ENCAP_TYPE_LONG_VALID_E "},
    {162,177,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_ETHER_TYPE_DSAP_SSAP_LONG_E "},
    {371,378,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_TCP_UDP_PORT_COMPS_LONG_E "},
    {129,224,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_SIP_127_32_LONG_E "},
    {226,226,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_IS_IPV6_EH_HOP_BY_HOP_LONG_E "},
    {227,234,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_DIP_127_120_LONG_E "} ,
    {235,282,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_MAC_SA_LONG_E "},
    {283,330,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_MAC_DA_LONG_E "},
    {379,380,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_RESERVED_380_379_XCAT2_394_393_LONG_E "},
    {225,225,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_IS_IPV6_EH_EXISTS_LONG_E "},
    {235,322,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_DIP_119_32_LONG_E "},
    {323,354,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_DIP_0_31_IPV6_LONG_E "},
    {370,370,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_IS_L2_VALID_LONG_E "} ,
    {383,383,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_IS_VIDX_LONG_E "},
    {33 , 39,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_CPU_CODE_0_6_STANDARD_E "},
    {33 , 39,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_RESERVED_39_33_XCAT2_38_32_E "},
    {75 , 81,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_TRUNK_ID "},
    {75 , 81,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_SRCDEV_ID "},
    {83 , 87,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_SST_ID_E " },
    {89 , 89,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_IS_UNKNOWN_UC_E " },
    {89 , 89,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_CPU_CODE_7_STANDARD_E "},
    {178,178,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_LONG_IPV4_FRAGMENT_E "},
    {89 , 89,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_RESERVED_89_XCAT2_88_E "},
    {381,382,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_PACKET_TYPE_LONG_E "},
    {128,159,GT_TRUE," XCAT2_EPCL_KEY_FIELDS_ID_SIP_63_32_LONG_E "},
    {160,190,GT_TRUE," XCAT2_EPCL_KEY_FIELDS_ID_SIP_94_64_LONG_E "},
    {206,238,GT_TRUE," XCAT2_EPCL_KEY_FIELDS_ID_SIP_127_95_LONG_E "},
    {  1,  8,GT_TRUE," XCAT2_EPCL_KEY_FIELDS_ID_PORT_LIST_7_0_E "},
    { 33, 38,GT_TRUE," XCAT2_EPCL_KEY_FIELDS_ID_PORT_LIST_13_8_E "},
    {193,206,GT_TRUE," XCAT2_EPCL_KEY_FIELDS_ID_PORT_LIST_27_14_E "},
    { 39, 39,GT_TRUE," XCAT2_EPCL_KEY_FIELDS_ID_RESERVED_38_E "}
};

/* array that holds the info about the fields -- xcat 2 format */
static CHT_PCL_KEY_FIELDS_INFO_STC xcat2ePclKeyFieldsData[CHT3_EPCL_KEY_FIELDS_ID_LAST_E]=
{
    {0  ,0  ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_VALID_E "  },
    {0  ,9  ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_PCL_ID_E "  },
    {0  ,9  ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_PCL_ID_EXT_E"    },
    {16 ,22 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_QOS_PROFILE_6_0_EXT_E"  },
    {36 ,47 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_ORIG_VID_EXT_E"  },
    {50 ,56 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_SRC_DEV_TRUNK_EXT_E"  },
    {59 ,59 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_SRC_IS_TRUNK_EXT_E"  },
    {60 ,107 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_MAC_SA_EXT_E"  },
    {108,155 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_MAC_DA_EXT_E"  },
    {156,156 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_IS_L2_VALID_EXT_E"  },
    {157,157 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_IS_IP_EXT_E"  },
    {158,165 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_IP_PROTOCOL_EXT_E"  },
    {166,171 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_DSCP_EXT_E"  },
    {172,173 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_TOS_1_0_EXT_E"  },
    {174,181 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_TTL_EXT_E"  },
    {182,182 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_IS_ARP_EXT_E"  },
    {184,215 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_SIP_31_0_EXT_E"  },
    {216,247 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_DIP_31_0_EXT_E"  },
    {262,269 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_TCP_UDP_PORT_COMPS_EXT_E"  },
    {270,293 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_L4_BYTES_OFFSETS_0_1_13_EXT_E"  },
    {294,309 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_2_3_EXT_E"  },
    {294,309 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_4_5_EXT_E"  },
    {310,310 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_IS_L4_VALID_EXT_E"  },
    {311,313 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_TC_EXT_E"  },
    {314,315 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_DP_EXT_E"  },
    {316,317 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_PACKET_TYPE_EXT_E"  },
    {318,318 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_SRC_TRG_TX_MIRR_EXT_E"  },
    {319,321 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_ASSIGNED_UP_EXT_E"  },
    {322,327 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_TRG_PORT_EXT_E"  },
    {328,328 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_RX_SNIF_EXT_E"  },
    {329,329 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_IS_ROUTED_EXT_E"  },
    {330,330 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_IP_VER_EXT_E"  },
    {331,331 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_IPV4_OPT_EXT_E"  },
/* RACL/VACL (72B) IPv6 - Egress */
    {331,331 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_IPV6_EH_TRIPLE_E"  },
    {332,332 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_IPV6_IS_ND_TRIPLE_E"  },
    {333,333 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_IPV6_EH_HOP_TRIPLE_E"  },
    {334,429 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_SIP_127_32_TRIPLE_E"  },
    {430,525 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_DIP_127_32_TRIPLE_E"  },
    {10 ,15 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_SRC_PORT_E "  },
    {16 ,16 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_IS_TAGGED_E "  },
    {17 ,28 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_VID_E "  },
    {29 ,31 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_UP_E "  },
    {32 ,38 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_QOS_PROFILE_E "  },
    {39 ,39 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_IS_IPV4_E "  },
    {40 ,40 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_IS_IP_E "  },
    {41 ,41 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_RESERVED_42_XCAT2_41_E "  },
    {42 ,57 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_ETHER_TYPE_DSAP_SSAP_E "  },
    {58 ,65 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_RESERVED_66_59_XCAT2_65_58_SHORT_E "  },
    {66 ,71 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_RESERVED_72_67_XCAT2_71_66_E "  },
    {72 ,72 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_IS_ARP_E "  },
    {73 ,73 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_RESERVED_74_XCAT2_73_E "  },
    {81 ,81 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_SRC_IS_TRUNK_SHORT_E "},
    {87 ,87 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_IS_ROUTED "},
    {89 ,89 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_IS_L2_VALID_SHORT_E "  },
    {90 ,90 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_ENCAP_TYPE_SHORT_E "  },
    {91 ,138,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_MAC_SA_SHORT_E "  },
    {139,186,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_MAC_DA_SHORT_E "  },
    {187,188,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_PACKET_TYPE_SHORT_E "},
    {189,189,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_RESERVED_190_XCAT2_189_E "} ,
    {190,190,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_IS_VIDX_SHORT_E "},
    {41 ,48 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_IP_PROTOCOL_E "},
    {49 ,54 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_DSCP_E "} ,
    {55 ,55 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_IS_L4_VALID_E "},
    {56 ,71 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_2_3_E "},
    {56 ,71 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_4_5_E "},
    {90 ,97 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_TCP_UDP_PORT_COMPS_SHORT_E "},
    {98 ,129,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_DIP_31_0_E "},
    {130,137,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_L4_BYTES_OFFSETS_13_SHORT_E "},
    {138,138,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_RESERVED_139_XCAT2_138_SHORT_E "},
    {189,189,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_IPV4_FRAGMENTED_SHORT_E "}   ,
    {73 ,73 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_IS_BC_E "},
    {98 ,129,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_SIP_31_0_SHORT_E "}   ,
    {130,161,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_DIP_31_0_IPV4_L4_SHORT_E "},
    {162,185,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_L4_BYTES_OFFSETS_0_1_13_SHORT_E "} ,
    {186,186,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_RESERVED_187_XCAT2_186_E "},
    {39 ,39 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_IS_IPV6_E "}      ,
    {72 ,95 ,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_L4_BYTES_OFFSETS_0_1_13_LONG_E "},
    {96 ,127,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_SIP_31_0_LONG_E " },
    {128,159,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_DIP_31_0_LONG_E "},
    {160,160,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_ENCAP_TYPE_LONG_VALID_E "},
    {161,176,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_ETHER_TYPE_DSAP_SSAP_LONG_E "},
    {385,392,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_TCP_UDP_PORT_COMPS_LONG_E "},
    {129,224,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_SIP_127_32_LONG_E "},
    {240,240,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_IS_IPV6_EH_HOP_BY_HOP_LONG_E "},
    {241,248,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_DIP_127_120_LONG_E "} ,
    {249,296,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_MAC_SA_LONG_E "},
    {297,344,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_MAC_DA_LONG_E "},
    {393,394,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_RESERVED_380_379_XCAT2_394_393_LONG_E "},
    {239,239,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_IS_IPV6_EH_EXISTS_LONG_E "},
    {249,336,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_DIP_119_32_LONG_E "},
    {337,368,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_DIP_0_31_IPV6_LONG_E "},
    {384,384,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_IS_L2_VALID_LONG_E "} ,
    {397,397,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_IS_VIDX_LONG_E "},
    {32 , 38,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_CPU_CODE_0_6_STANDARD_E "},
    {32 , 38,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_RESERVED_39_33_XCAT2_38_32_E "},
    {74 , 80,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_TRUNK_ID "},
    {74 , 80,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_SRCDEV_ID "},
    {82 , 86,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_SST_ID_E " },
    {88 , 88,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_IS_UNKNOWN_UC_E " },
    {88 , 88,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_CPU_CODE_7_STANDARD_E "},
    {177,177,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_LONG_IPV4_FRAGMENT_E "},
    {88 , 88,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_RESERVED_89_XCAT2_88_E "},
    {395,396,GT_TRUE," CHT3_EPCL_KEY_FIELDS_ID_PACKET_TYPE_LONG_E "},
    {128,159,GT_TRUE," XCAT2_EPCL_KEY_FIELDS_ID_SIP_63_32_LONG_E "},
    {160,190,GT_TRUE," XCAT2_EPCL_KEY_FIELDS_ID_SIP_94_64_LONG_E "},
    {206,238,GT_TRUE," XCAT2_EPCL_KEY_FIELDS_ID_SIP_127_95_LONG_E "},
    {  0,  7,GT_TRUE," XCAT2_EPCL_KEY_FIELDS_ID_PORT_LIST_7_0_E "},
    { 32, 37,GT_TRUE," XCAT2_EPCL_KEY_FIELDS_ID_PORT_LIST_13_8_E "},
    {192,205,GT_TRUE," XCAT2_EPCL_KEY_FIELDS_ID_PORT_LIST_27_14_E "},
    { 38, 38,GT_TRUE," XCAT2_EPCL_KEY_FIELDS_ID_RESERVED_38_E "},

    /* lion2 fields */
    /* fields common to keys 0..5*/
    {191,191,GT_TRUE," LION2_EPCL_KEY_FIELDS_ID_KEY_0_5_TAG1_EXISTS_E "},

    /* fields common to keys 0..2 - standard keys */
    {204,205,GT_TRUE," LION2_EPCL_KEY_FIELDS_ID_KEY_0_2_SRC_PORT_6_7_E "},

    /* fields common to keys 3..5*/
    {393,394,GT_TRUE," LION2_EPCL_KEY_FIELDS_ID_KEY_3_5_SRC_PORT_6_7_E "},
    {398,399,GT_TRUE," LION2_EPCL_KEY_FIELDS_ID_KEY_3_5_SRC_DEV_5_6_E "},
    {400,404,GT_TRUE," LION2_EPCL_KEY_FIELDS_ID_KEY_3_5_SRC_DEV_7_11_OR_TRUNKID_7_11_E "},
    {405,411,GT_TRUE," LION2_EPCL_KEY_FIELDS_ID_KEY_3_5_SRC_ID_5_11_E "},

    /* fields common to keys 6..7*/
    { 11, 11,GT_TRUE," LION2_EPCL_KEY_FIELDS_ID_KEY_6_7_TAG1_EXISTS_E "},
    { 17, 18,GT_TRUE," LION2_EPCL_KEY_FIELDS_ID_KEY_6_7_TRG_PORT_6_7_E "},
    { 24, 35,GT_TRUE," LION2_EPCL_KEY_FIELDS_ID_KEY_6_7_SRC_ID_E "},
    { 48, 49,GT_TRUE," LION2_EPCL_KEY_FIELDS_ID_KEY_6_7_SRC_PORT_6_7_E "},
    { 50, 58,GT_TRUE," LION2_EPCL_KEY_FIELDS_ID_KEY_6_7_SRC_DEV_0_8_OR_SRC_TRUNKID_0_8_E "},

    { 40, 40,GT_TRUE," LION2_EPCL_KEY_FIELDS_ID_KEY0_UP1_2_E "},
    { 58, 69,GT_TRUE," LION2_EPCL_KEY_FIELDS_ID_KEY0_TAG1_VID_E "},
    { 70, 71,GT_TRUE," LION2_EPCL_KEY_FIELDS_ID_KEY0_UP1_0_1_E "},
    {189,189,GT_TRUE," LION2_EPCL_KEY_FIELDS_ID_KEY0_TAG1_CFI_E "},

    {206,217,GT_TRUE," LION2_EPCL_KEY_FIELDS_ID_KEY3_TAG1_VID_E "},
    {218,218,GT_TRUE," LION2_EPCL_KEY_FIELDS_ID_KEY3_TAG1_CFI_E "},
    {219,221,GT_TRUE," LION2_EPCL_KEY_FIELDS_ID_KEY3_UP1_OR_EXP_E "},
    {222,223,GT_TRUE," LION2_EPCL_KEY_FIELDS_ID_KEY3_NUMBER_OF_MPLS_LABELS_E "},
    {224,225,GT_TRUE," LION2_EPCL_KEY_FIELDS_ID_KEY3_PROTOCOL_AFTER_MPLS_E "},
    {226,245,GT_TRUE," LION2_EPCL_KEY_FIELDS_ID_KEY3_MPLS_LABEL1_E "},
    {246,248,GT_TRUE," LION2_EPCL_KEY_FIELDS_ID_KEY3_MPLS_EXP1_E "},
    {345,345,GT_TRUE," LION2_EPCL_KEY_FIELDS_ID_KEY3_IS_MPLS_E "},
    {346,365,GT_TRUE," LION2_EPCL_KEY_FIELDS_ID_KEY3_MPLS_LABEL0_E "},
    {366,368,GT_TRUE," LION2_EPCL_KEY_FIELDS_ID_KEY3_MPLS_EXP0_E "},

    {345,356,GT_TRUE," LION2_EPCL_KEY_FIELDS_ID_KEY4_TAG1_VID_E "},
    {357,357,GT_TRUE," LION2_EPCL_KEY_FIELDS_ID_KEY4_TAG1_CFI_E "},
    {358,360,GT_TRUE," LION2_EPCL_KEY_FIELDS_ID_KEY4_UP1_OR_EXP_E "},

    {318,329,GT_TRUE," LION2_EPCL_KEY_FIELDS_ID_KEY6_TAG1_VID_E "},
    {330,330,GT_TRUE," LION2_EPCL_KEY_FIELDS_ID_KEY6_TAG1_CFI_E "},
    {331,333,GT_TRUE," LION2_EPCL_KEY_FIELDS_ID_KEY6_UP1_OR_EXP_E "},
    {334,336,GT_TRUE," LION2_EPCL_KEY_FIELDS_ID_KEY6_SRC_DEV_9_11_OR_SRC_TRUNKID_9_11_E "},

    {512,523,GT_TRUE," LION2_EPCL_KEY_FIELDS_ID_KEY7_TAG1_VID_E "},
    {524,524,GT_TRUE," LION2_EPCL_KEY_FIELDS_ID_KEY7_TAG1_CFI_E "},
    {525,527,GT_TRUE," LION2_EPCL_KEY_FIELDS_ID_KEY7_UP1_OR_EXP_E "},
    {527,530,GT_TRUE," LION2_EPCL_KEY_FIELDS_ID_KEY7_SRC_DEV_9_11_OR_SRC_TRUNKID_9_11_E "},

    {532,551,GT_TRUE," LION2_EPCL_KEY_FIELDS_ID_KEY7_SRC_EPORT_E "},
    {552,571,GT_TRUE," LION2_EPCL_KEY_FIELDS_ID_KEY7_TRG_EPORT_E "},
    {572,575,GT_TRUE," LION2_EPCL_KEY_FIELDS_ID_KEY7_E_VID_12_15_E "},

};


/*******************************************************************************
*   snetCht3EPclKeyFieldBuildByPointer
*
* DESCRIPTION:
*       function insert data of field to the search key in specific place in key
*
* INPUTS:
*       pclKeyPtr - (pointer to) current pcl key
*       fieldValPtr - (pointer to) data of field to insert to key
*       fieldId -- field id
*
* OUTPUTS:
*       pclKeyPtr - (pointer to) current pcl key
*
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID snetCht3EPclKeyFieldBuildByPointer
(
    INOUT SNET_CHT_POLICY_KEY_STC       *pclKeyPtr,
    IN GT_U8                            *fieldValPtr,
    IN CHT3_EPCL_KEY_FIELDS_ID_ENT      fieldId
)
{
    CHT_PCL_KEY_FIELDS_INFO_STC *fieldInfoPtr;

    if (pclKeyPtr->devObjPtr->ePclKeyFormatVersion)
    {
        fieldInfoPtr = &xcat2ePclKeyFieldsData[fieldId];
    }
    else
    {
        fieldInfoPtr = &cht3ePclKeyFieldsData[fieldId];
    }

    snetChtPclSrvKeyFieldBuildByPointer(pclKeyPtr, fieldValPtr, fieldInfoPtr);

    return;
}

/*******************************************************************************
*   snetCht3EPclKeyFieldBuildByU32Pointer
*
* DESCRIPTION:
*       function insert data of field to the search key in specific place in key
*
* INPUTS:
*       ePclKeyPtr - (pointer to) current epcl key
*       fieldValPtr - (pointer to) data of field to insert to key
*       fieldId -- field id
*
* OUTPUTS:
*       ePclKeyPtr - (pointer to) current epcl key
*
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID snetCht3EPclKeyFieldBuildByU32Pointer
(
    INOUT SNET_CHT_POLICY_KEY_STC       *ePclKeyPtr,
    IN GT_U32                           *fieldValPtr,
    IN CHT3_EPCL_KEY_FIELDS_ID_ENT      fieldId
)
{
    CHT_PCL_KEY_FIELDS_INFO_STC *fieldInfoPtr;

    if (ePclKeyPtr->devObjPtr->ePclKeyFormatVersion)
    {
        fieldInfoPtr = &xcat2ePclKeyFieldsData[fieldId];
    }
    else
    {
        fieldInfoPtr = &cht3ePclKeyFieldsData[fieldId];
    }

    snetChtPclSrvKeyFieldBuildByU32Pointer(ePclKeyPtr, fieldValPtr, fieldInfoPtr);

    return;
}

/*******************************************************************************
*   snetCht3EPclKeyFieldBuildByValue
*
* DESCRIPTION:
*       function insert data of the field to the search key
*       in specific place in epcl search key
*
* INPUTS:
*       ePclKeyPtr - (pointer to) current epcl key
*       fieldVal - data of field to insert to key
*       fieldId -- field id
*
* OUTPUTS:
*       ePclKeyPtr - (pointer to) current epcl key
*
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID snetCht3EPclKeyFieldBuildByValue
(
    INOUT SNET_CHT_POLICY_KEY_STC           *ePclKeyPtr,
    IN GT_U32                               fieldVal,
    IN CHT3_EPCL_KEY_FIELDS_ID_ENT          fieldId
)
{
    CHT_PCL_KEY_FIELDS_INFO_STC *fieldInfoPtr;

    if (ePclKeyPtr->devObjPtr->ePclKeyFormatVersion)
    {
        fieldInfoPtr = &xcat2ePclKeyFieldsData[fieldId];
    }
    else
    {
        fieldInfoPtr = &cht3ePclKeyFieldsData[fieldId];
    }

    snetChtPclSrvKeyFieldBuildByValue(ePclKeyPtr, fieldVal, fieldInfoPtr);

    return;
}


/*******************************************************************************
*   snetCht3EPclPortListBmpBuild
*
* DESCRIPTION:
*        Build EPCL Port List bitmap, separated on 3 groups of bits:
*        bits 7-0, 13-8, 27-14.
*        Used in EPCL keys.
*
* INPUTS:
*       devObjPtr             - pointer to device object of the egress core
*       descrPtr             - pointer to frame data buffer Id.
*       egressPort            - egress port.
*
* OUTPUTS:
*
*       portListBits7_0Ptr     - pointer to value of bits 7-0.
*       portListBits13_8Ptr    - pointer to value of bits 13-8.
*       portListBits27_14Ptr   - pointer to value of bits 27-14.
*
* RETURN:
*
* COMMENTS:
*    port List bitmap - only one bit is "1", corresponding to source port  (1 << srcPort)
*    For CPU port (63) - all bits are "0"
*******************************************************************************/
GT_VOID snetCht3EPclPortListBmpBuild
(
    IN SKERNEL_DEVICE_OBJECT                * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr,
    IN  GT_U32            egressPort,
    OUT GT_U32          * portListBits7_0Ptr,
    OUT GT_U32          * portListBits13_8Ptr,
    OUT GT_U32          * portListBits27_14Ptr
)
{
    GT_U32 bitIndex; /* bit index that represent the port */

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        /* the port number is mapped to a bit index */
        bitIndex =
            SMEM_LION3_HA_PHYSICAL_PORT_1_ENTRY_FIELD_AUTO_GET(devObjPtr,
                descrPtr,
                SMEM_LION3_HA_PHYSICAL_PORT_TABLE_1_FIELDS_PORT_LIST_BIT_VECTOR_OFFSET);
    }
    else
    {
        /* the port number is the bit index */
        bitIndex = egressPort;
    }

    *portListBits7_0Ptr   = 0;
    *portListBits13_8Ptr  = 0;
    *portListBits27_14Ptr = 0;

    if (bitIndex <= 7)
    {
        *portListBits7_0Ptr = 1 << bitIndex;
    }
    else
    if (bitIndex <= 13)
    {
        *portListBits13_8Ptr = 1 << (bitIndex - 8);
    }
    else
    if (bitIndex <= 27)
    {
        *portListBits27_14Ptr = 1 << (bitIndex - 14);
    }
}

/*******************************************************************************
*   snetCht3EPclKeyBuildL4IPv6ExtendedKey
*
* DESCRIPTION:
*        Build Layer4+IPv6 extended ePCL search key.
*
* INPUTS:
*       devObjPtr            - pointer to device object.
*       descrPtr             - pointer to frame data buffer Id.
*        egressPort     - local egress port (not global).
*       lookupConfPtr        - pointer to the ePCL configuration entry.
*       pclExtraDataPtr      - pointer to extra data needed for the EPCL engine.
*
* OUTPUTS:
*       pclKeyPtr    - pointer to PCL key structure.
*
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID snetCht3EPclKeyBuildL4IPv6ExtendedKey
(
    IN SKERNEL_DEVICE_OBJECT                * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr,
    IN  GT_U32                                egressPort,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC  * lookupConfPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC    * pclExtraDataPtr,
    OUT SNET_CHT_POLICY_KEY_STC             * pclKeyPtr
)
{
    DECLARE_FUNC_NAME(snetCht3EPclKeyBuildL4IPv6ExtendedKey);

    GT_U32          fieldVal = 1; /* valid bit value */
    GT_U32          resrvdVal = 0 ; /* reserved bit value */
    GT_U8           byteValue;/* value of tcp/udp compare */
    GT_STATUS       rc;/* return code from function */
    GT_U8           tmpFieldVal[4];
    GT_U32  srcPort = EPCL_SRC_PORT_MAC(descrPtr);
    GT_U8           ipv6Bytes[16];/*16 bytes for internal use of ipv6 bytes*/
    GT_U32          ii,word;

    if (devObjPtr->ePclKeyFormatVersion == 0)
    {
        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, fieldVal, /* bits[0] Valid */
                                            CHT3_EPCL_KEY_FIELDS_ID_VALID_E);
    }

    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, lookupConfPtr->pclID,
                                        CHT3_EPCL_KEY_FIELDS_ID_PCL_ID_E);/*[10:1] ePCL-ID */
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, srcPort ,
                                        CHT3_EPCL_KEY_FIELDS_ID_SRC_PORT_E);/* [16:11] src port */
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->egressTagged,
                                        CHT3_EPCL_KEY_FIELDS_ID_IS_TAGGED_E);/* [17] is tagged */
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->eVid,
                                        CHT3_EPCL_KEY_FIELDS_ID_VID_E);/* [29:18] eVid */
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->up,
                                        CHT3_EPCL_KEY_FIELDS_ID_UP_E); /* [32:30] UP */
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->qos.qosProfile,/* [39:33] QOS profile */
                                        CHT3_EPCL_KEY_FIELDS_ID_QOS_PROFILE_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, (descrPtr->isIPv4==0 && descrPtr->isIp),
                                        CHT3_EPCL_KEY_FIELDS_ID_IS_IPV6_E);/* [40] is IPv6 */
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->isIp,/* [41] is IP */
                                        CHT3_EPCL_KEY_FIELDS_ID_IS_IP_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->ipProt,/* [49:42] IP Protocol*/
                                        CHT3_EPCL_KEY_FIELDS_ID_IP_PROTOCOL_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dscp,/* [55:50] dscp */
                                        CHT3_EPCL_KEY_FIELDS_ID_DSCP_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL4Valid, /* [56] isL4Valid*/
                                        CHT3_EPCL_KEY_FIELDS_ID_IS_L4_VALID_E);
    if (descrPtr->l4StartOffsetPtr != NULL)
    {
        if (descrPtr->ipv6Icmp || descrPtr->ipv4Icmp)
        {/* [72:57] L4 Byte Offsets 5:4 */
            snetCht3EPclKeyFieldBuildByPointer(
                pclKeyPtr,(GT_U8*)&(descrPtr->l4StartOffsetPtr[4]),
                                        CHT3_EPCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_4_5_E);
        }
        else
        {/* [72:57] L4 Byte Offsets 3:2 */
            snetCht3EPclKeyFieldBuildByPointer(
                pclKeyPtr,(GT_U8*)&(descrPtr->l4StartOffsetPtr[2]),
                                        CHT3_EPCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_2_3_E);
        }
        tmpFieldVal[0] = descrPtr->l4StartOffsetPtr[0];
        tmpFieldVal[1] = descrPtr->l4StartOffsetPtr[1];
        tmpFieldVal[2] = descrPtr->l4StartOffsetPtr[13];
        snetCht3EPclKeyFieldBuildByPointer(
            pclKeyPtr,(GT_U8*)&tmpFieldVal[0],/* [96:73] L4 Byte Offsets[1:0],[13] */
                    CHT3_EPCL_KEY_FIELDS_ID_L4_BYTES_OFFSETS_0_1_13_LONG_E);
    }

    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[3], /* [128:97] SIP*/
                                    CHT3_EPCL_KEY_FIELDS_ID_SIP_31_0_LONG_E);

    if (devObjPtr->ePclKeyFormatVersion)
    {
        /* xCat2 EPCL Key format */
        __LOG(("xCat2 EPCL Key format"));
        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[2], /* [159:128] SIP*/
                                        XCAT2_EPCL_KEY_FIELDS_ID_SIP_63_32_LONG_E);
        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[1], /* [190:160]  SIP*/
                                        XCAT2_EPCL_KEY_FIELDS_ID_SIP_94_64_LONG_E);
        snetCht3EPclKeyFieldBuildByU32Pointer(pclKeyPtr, descrPtr->sip, /* [238:206] SIP*/
                                        XCAT2_EPCL_KEY_FIELDS_ID_SIP_127_95_LONG_E);
    }
    else
    {
        /* Ch3, xCat, Lion EPCL Key format */
        __LOG(("Ch3, xCat, Lion EPCL Key format"));
        snetCht3EPclKeyFieldBuildByU32Pointer(pclKeyPtr, descrPtr->sip, /* [224:129] SIP*/
                                        CHT3_EPCL_KEY_FIELDS_ID_SIP_127_32_LONG_E);
    }

    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpV6EhExists, /* [225] ipv6_eh_exist */
                            CHT3_EPCL_KEY_FIELDS_ID_IS_IPV6_EH_EXISTS_LONG_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpV6EhHopByHop, /* [226] ipv6_eh_hopbyhop */
                            CHT3_EPCL_KEY_FIELDS_ID_IS_IPV6_EH_HOP_BY_HOP_LONG_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, (descrPtr->dip[0] >> 24),/* [234:227] DIP */
                                    CHT3_EPCL_KEY_FIELDS_ID_DIP_127_120_LONG_E);

    /* dip 119.. 96 */
    word = 0;
    ii = 0;
    ipv6Bytes[ii++] = (GT_U8)SMEM_U32_GET_FIELD(descrPtr->dip[word],16,8);
    ipv6Bytes[ii++] = (GT_U8)SMEM_U32_GET_FIELD(descrPtr->dip[word], 8,8);
    ipv6Bytes[ii++] = (GT_U8)SMEM_U32_GET_FIELD(descrPtr->dip[word], 0,8);
    /* dip 95..64 */
    word++;
    ipv6Bytes[ii++] = (GT_U8)SMEM_U32_GET_FIELD(descrPtr->dip[word],24,8);
    ipv6Bytes[ii++] = (GT_U8)SMEM_U32_GET_FIELD(descrPtr->dip[word],16,8);
    ipv6Bytes[ii++] = (GT_U8)SMEM_U32_GET_FIELD(descrPtr->dip[word], 8,8);
    ipv6Bytes[ii++] = (GT_U8)SMEM_U32_GET_FIELD(descrPtr->dip[word], 0,8);
    /* dip 63..32 */
    word++;
    ipv6Bytes[ii++] = (GT_U8)SMEM_U32_GET_FIELD(descrPtr->dip[word],24,8);
    ipv6Bytes[ii++] = (GT_U8)SMEM_U32_GET_FIELD(descrPtr->dip[word],16,8);
    ipv6Bytes[ii++] = (GT_U8)SMEM_U32_GET_FIELD(descrPtr->dip[word], 8,8);
    ipv6Bytes[ii++] = (GT_U8)SMEM_U32_GET_FIELD(descrPtr->dip[word], 0,8);

    snetCht3EPclKeyFieldBuildByPointer(pclKeyPtr,ipv6Bytes,/* [322:235] DIP */
                                    CHT3_EPCL_KEY_FIELDS_ID_DIP_119_32_LONG_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->dip[3], /* [354:323] DIP */
                                    CHT3_EPCL_KEY_FIELDS_ID_DIP_0_31_IPV6_LONG_E);

    /* TCP/UDP Range Comparators is enable */
    __LOG(("TCP/UDP Range Comparators is enable"));
    rc = snetCht2EPclTcpUdpPortRangeCompareGet(devObjPtr,descrPtr,&byteValue);
    byteValue = (rc != GT_OK) ? 0 : byteValue ;
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [378:371] TCP/UDP comparator */
                        CHT3_EPCL_KEY_FIELDS_ID_TCP_UDP_PORT_COMPS_LONG_E);

    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, resrvdVal,/* Reserved [380:379] */
                                CHT3_EPCL_KEY_FIELDS_ID_RESERVED_380_379_XCAT2_394_393_LONG_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr,pclExtraDataPtr->isL2Valid ,/* is L2 valid [381] */
                                    CHT3_EPCL_KEY_FIELDS_ID_IS_L2_VALID_LONG_E);
    SNET_CHT3_EPCL_KEY_EGT_PKT_TYPE(descrPtr, fieldVal);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, fieldVal, /* [381:382] packet type */
                                CHT3_EPCL_KEY_FIELDS_ID_PACKET_TYPE_LONG_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->useVidx ,
                            CHT3_EPCL_KEY_FIELDS_ID_IS_VIDX_LONG_E);/* [383] isVidx */


    if (devObjPtr->ePclKeyFormatVersion >= 2)
    {
        /* fields common to keys 0..5*/
        __LOG(("fields common to keys 0..5"));
        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, TAG1_EXIST_MAC(descrPtr) ,
                                    LION2_EPCL_KEY_FIELDS_ID_KEY_0_5_TAG1_EXISTS_E);

        /* fields common to keys 3..5*/
        __LOG(("fields common to keys 3..5"));
        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr,srcPort >> 6,
                                    LION2_EPCL_KEY_FIELDS_ID_KEY_3_5_SRC_PORT_6_7_E);
        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->srcDev >> 5 ,
                                    LION2_EPCL_KEY_FIELDS_ID_KEY_3_5_SRC_DEV_5_6_E);

        if(descrPtr->outGoingMtagCmd == SKERNEL_MTAG_CMD_FORWARD_E && descrPtr->origIsTrunk)
        {
            snetCht3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->origSrcEPortOrTrnk >> 7,
                LION2_EPCL_KEY_FIELDS_ID_KEY_3_5_SRC_DEV_7_11_OR_TRUNKID_7_11_E);
        }
        else
        {
            snetCht3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->srcDev >> 7,
                LION2_EPCL_KEY_FIELDS_ID_KEY_3_5_SRC_DEV_7_11_OR_TRUNKID_7_11_E);
        }

    }


    return  ;
}

/*******************************************************************************
*   snetCht3EPclKeyBuildL4IPv4NonIpExtendedKey
*
* DESCRIPTION:
*        Build  RACL/VACL (48B) IPv4 - Egress NonIP or IPv4/ARP search key.
*
* INPUTS:
*       devObjPtr            - pointer to device object.
*       descrPtr             - pointer to frame data buffer Id.
*        egressPort     - local egress port (not global).
*       lookupConfPtr        - pointer to the ePCL configuration entry.
*       pclExtraDataPtr      - pointer to extra data needed for the EPCL engine.
*
* OUTPUTS:
*       pclKeyPtr    - pointer to PCL key structure.
*
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID snetCht3EPclKeyBuildL4IPv4NonIpExtendedKey
(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr,
    IN  GT_U32                                egressPort,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC  * lookupConfPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC    * pclExtraDataPtr,
    OUT SNET_CHT_POLICY_KEY_STC         * pclKeyPtr
)
{
    DECLARE_FUNC_NAME(snetCht3EPclKeyBuildL4IPv4NonIpExtendedKey);

    GT_U32      fieldVal;       /* 32 bit field value */
    GT_U8       byteValue;      /* value of cp/udp compare */
    GT_STATUS   rc; /* status from called function */
    GT_U8       tmpFieldVal[4];
    GT_U8     * l3HdrStartPtr;  /* Layer 3 offset start pointer */
    GT_U32  srcPort = EPCL_SRC_PORT_MAC(descrPtr);
    GT_U32  trgPort = EPCL_TRG_PORT_MAC(devObjPtr,descrPtr,egressPort);

    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, lookupConfPtr->pclID ,
                            CHT3_EPCL_KEY_FIELDS_ID_PCL_ID_E);  /*[10:1] PCL-ID */
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, srcPort ,
                            CHT3_EPCL_KEY_FIELDS_ID_SRC_PORT_E);/* [16:11] src port */
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->qos.qosProfile, /* [23:17] QOS profile */
                            CHT3_EPCL_KEY_FIELDS_ID_QOS_PROFILE_6_0_EXT_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->vid0Or1AfterTti ,/* [48:37] orig vid */
                            CHT3_EPCL_KEY_FIELDS_ID_ORIG_VID_EXT_E);
    if (descrPtr->origIsTrunk)
    {
        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->origSrcEPortOrTrnk ,
                            CHT3_EPCL_KEY_FIELDS_ID_SRC_DEV_TRUNK_EXT_E); /* [57:51] source trunk id */
    }
    else
    {
        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->srcDev,
                            CHT3_EPCL_KEY_FIELDS_ID_SRC_DEV_TRUNK_EXT_E); /* [57:51] source device */
    }

    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->origIsTrunk ,/* [60] source is trunk  */
                            CHT3_EPCL_KEY_FIELDS_ID_SRC_IS_TRUNK_EXT_E);

    /* the EPCL need to use the actual mac addresses of egress and not those of ingress */
    /* For non-tunnel terminated packets:
            Transmitted packet MACs after packet modification (for example, due to routing or Tunnel Termination)
        For Ethernet-Over-xxx Tunnel-Start packets:
            MACs of the passenger packet
    */
    if(descrPtr->haToEpclInfo.macDaSaPtr)
    {
        snetCht3EPclKeyFieldBuildByPointer(pclKeyPtr, &descrPtr->haToEpclInfo.macDaSaPtr[6]/* mac SA */, /* [108:61] MAC SA*/
                                CHT3_EPCL_KEY_FIELDS_ID_MAC_SA_EXT_E);
        snetCht3EPclKeyFieldBuildByPointer(pclKeyPtr, descrPtr->haToEpclInfo.macDaSaPtr,/* [156:109] MAC DA*/
                                CHT3_EPCL_KEY_FIELDS_ID_MAC_DA_EXT_E);
    }

    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL2Valid ,/* [157] isL2Valid  */
                            CHT3_EPCL_KEY_FIELDS_ID_IS_L2_VALID_EXT_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->isIp,/* [158] is IP */
                            CHT3_EPCL_KEY_FIELDS_ID_IS_IP_EXT_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->ipProt,/* [166:159] IP Protocol*/
                            CHT3_EPCL_KEY_FIELDS_ID_IP_PROTOCOL_EXT_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dscp,/* [172:167] dscp */
                            CHT3_EPCL_KEY_FIELDS_ID_DSCP_EXT_E);

    l3HdrStartPtr = SNET_CHT3_PCKT_TUNNEL_START_PSGR(descrPtr);

    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, l3HdrStartPtr[1] & 0x3, /* [174:173] TOS[1:0] */
                            CHT3_EPCL_KEY_FIELDS_ID_TOS_1_0_EXT_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->ttl,          /* [182-175] Packet TTL */
                            CHT3_EPCL_KEY_FIELDS_ID_TTL_EXT_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->arp ,         /* [183] isArp */
                            CHT3_EPCL_KEY_FIELDS_ID_IS_ARP_EXT_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[0],       /* [215:184] SIP*/
                            CHT3_EPCL_KEY_FIELDS_ID_SIP_31_0_EXT_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[0],       /* [247:216] DIP*/
                            CHT3_EPCL_KEY_FIELDS_ID_DIP_31_0_EXT_E);

    rc = snetChtIPclTcpUdpPortRangeCompareGet(devObjPtr, descrPtr, &byteValue);
    byteValue = (rc != GT_OK) ? 0 : byteValue ;
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, byteValue,              /* [255:248] TCP/UDP comparator */
                            CHT3_EPCL_KEY_FIELDS_ID_TCP_UDP_PORT_COMPS_EXT_E);

    if (descrPtr->l4StartOffsetPtr != NULL)
    {
        tmpFieldVal[0] = descrPtr->l4StartOffsetPtr[0];
        tmpFieldVal[1] = descrPtr->l4StartOffsetPtr[1];
        tmpFieldVal[2] = descrPtr->l4StartOffsetPtr[13];
        snetCht3EPclKeyFieldBuildByPointer(
            pclKeyPtr,(GT_U8*)&tmpFieldVal[0],                          /* [279:256] L4 Byte Offsets[1:0],[13] */
                            CHT3_EPCL_KEY_FIELDS_ID_L4_BYTES_OFFSETS_0_1_13_EXT_E);

        if (descrPtr->ipv6Icmp || descrPtr->ipv4Icmp)
        {/* [295:280] L4 Byte Offsets 5:4 */
            snetCht3EPclKeyFieldBuildByPointer(
                pclKeyPtr,(GT_U8*)&(descrPtr->l4StartOffsetPtr[4]),
                            CHT3_EPCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_4_5_EXT_E);
        }
        else
        {/* [295:280] L4 Byte Offsets 3:2 */
            snetCht3EPclKeyFieldBuildByPointer(
                pclKeyPtr,(GT_U8*)&(descrPtr->l4StartOffsetPtr[2]),
                            CHT3_EPCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_2_3_EXT_E);
        }
    }

    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL4Valid,
                            CHT3_EPCL_KEY_FIELDS_ID_IS_L4_VALID_EXT_E);/* [296] isL4Valid*/

    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->tc,           /* [299:297] TC */
                            CHT3_EPCL_KEY_FIELDS_ID_TC_EXT_E);

    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dp,           /* [299:297] DP */
                            CHT3_EPCL_KEY_FIELDS_ID_DP_EXT_E);

    SNET_CHT3_EPCL_KEY_EGT_PKT_TYPE(descrPtr, fieldVal);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, fieldVal,   /* [303:302] Packet type */
                            CHT3_EPCL_KEY_FIELDS_ID_PACKET_TYPE_EXT_E);
    if (descrPtr->outGoingMtagCmd == SKERNEL_MTAG_CMD_TO_CPU_E)
    {
        fieldVal = (descrPtr->srcTrg & 0x1);
    }
    else
    if (descrPtr->outGoingMtagCmd == SKERNEL_MTAG_CMD_TO_TRG_SNIFFER_E)
    {
        fieldVal = ~(descrPtr->rxSniff & 0x1);
    }
    else
    {
        fieldVal = 0;
    }

    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, fieldVal, /* [304:304] SrcTrg/TxMirror */
                            CHT3_EPCL_KEY_FIELDS_ID_SRC_TRG_TX_MIRR_EXT_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->up,           /* [307:305] UP */
                            CHT3_EPCL_KEY_FIELDS_ID_ASSIGNED_UP_EXT_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, trgPort,           /* [313:308] TrgPort */
                            CHT3_EPCL_KEY_FIELDS_ID_TRG_PORT_EXT_E);
    fieldVal = (descrPtr->rxSniff) ? 1 : 0;
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, fieldVal,              /* [314] TrgPort */
                            CHT3_EPCL_KEY_FIELDS_ID_RX_SNIF_EXT_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr,      /* [315] isRouted  */
                            descrPtr->routed,
                            CHT3_EPCL_KEY_FIELDS_ID_IS_ROUTED_EXT_E);

    fieldVal = (descrPtr->isIp && descrPtr->isIPv4) ? 1 : 0;

    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, fieldVal,      /* [316] IPVersion  */
                            CHT3_EPCL_KEY_FIELDS_ID_IP_VER_EXT_E);

    if (descrPtr->isIp && ((l3HdrStartPtr[16] >> 2) & 0x1f))
    {
        fieldVal = 1;
    }
    else
    {
        fieldVal = 0;
    }

    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, fieldVal,      /* [317] Ipv4Options  */
                            CHT3_EPCL_KEY_FIELDS_ID_IPV4_OPT_EXT_E);

    if (devObjPtr->ePclKeyFormatVersion >= 2)
    {
        /* fields common to keys 6..7*/
        __LOG(("fields common to keys 6..7"));
        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, TAG1_EXIST_MAC(descrPtr) ,
                                    LION2_EPCL_KEY_FIELDS_ID_KEY_6_7_TAG1_EXISTS_E);
        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, trgPort >> 6 ,
                                    LION2_EPCL_KEY_FIELDS_ID_KEY_6_7_TRG_PORT_6_7_E);
        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sstId ,
                                    LION2_EPCL_KEY_FIELDS_ID_KEY_6_7_SRC_ID_E);
        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr,srcPort >> 6,
                                    LION2_EPCL_KEY_FIELDS_ID_KEY_6_7_SRC_PORT_6_7_E);
        if(descrPtr->outGoingMtagCmd == SKERNEL_MTAG_CMD_FORWARD_E && descrPtr->origIsTrunk)
        {
            snetCht3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->origSrcEPortOrTrnk,
                LION2_EPCL_KEY_FIELDS_ID_KEY_6_7_SRC_DEV_0_8_OR_SRC_TRUNKID_0_8_E);
        }
        else
        {
            snetCht3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->srcDev,
                LION2_EPCL_KEY_FIELDS_ID_KEY_6_7_SRC_DEV_0_8_OR_SRC_TRUNKID_0_8_E);
        }

        /* KEY 6 specifics */
        __LOG(("KEY 6 specifics"));
        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->vid1 ,
                                    LION2_EPCL_KEY_FIELDS_ID_KEY6_TAG1_VID_E);
        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->cfidei1 ,
                                    LION2_EPCL_KEY_FIELDS_ID_KEY6_TAG1_CFI_E);
        if(descrPtr->mpls)
        {
            snetCht3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->exp1,
                LION2_EPCL_KEY_FIELDS_ID_KEY6_UP1_OR_EXP_E);
        }
        else
        {
            snetCht3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->up1,
                LION2_EPCL_KEY_FIELDS_ID_KEY6_UP1_OR_EXP_E);
        }

        if(descrPtr->outGoingMtagCmd == SKERNEL_MTAG_CMD_FORWARD_E && descrPtr->origIsTrunk)
        {
            snetCht3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->origSrcEPortOrTrnk >> 9,
                LION2_EPCL_KEY_FIELDS_ID_KEY6_SRC_DEV_9_11_OR_SRC_TRUNKID_9_11_E);
        }
        else
        {
            snetCht3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->srcDev >> 9,
                LION2_EPCL_KEY_FIELDS_ID_KEY6_SRC_DEV_9_11_OR_SRC_TRUNKID_9_11_E);
        }
    }

    return;
}

/*******************************************************************************
*   snetCht3EPclKeyBuildL4IPv6TripleKey
*
* DESCRIPTION:
*        Build  RACL/VACL (72B) IPv6 - Egress search key.
*
* INPUTS:
*       devObjPtr            - pointer to device object.
*       descrPtr             - pointer to frame data buffer Id.
*        egressPort     - local egress port (not global).
*       lookupConfPtr        - pointer to the ePCL configuration entry.
*       pclExtraDataPtr      - pointer to extra data needed for the EPCL engine.
*
* OUTPUTS:
*       pclKeyPtr    - pointer to PCL key structure.
*
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID snetCht3EPclKeyBuildL4IPv6TripleKey
(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr,
    IN  GT_U32                                egressPort,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC  * lookupConfPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC    * pclExtraDataPtr,
    OUT SNET_CHT_POLICY_KEY_STC         * pclKeyPtr
)
{
    DECLARE_FUNC_NAME(snetCht3EPclKeyBuildL4IPv6TripleKey);

    GT_U32      fieldVal;       /* 32 bit field value */
    GT_U8       byteValue;      /* value of tcp/udp compare */
    GT_STATUS   rc; /* status from called function */
    GT_U8       tmpFieldVal[4];
    GT_U8     * l3HdrStartPtr;  /* Layer 3 offset start pointer */
    GT_U32  srcPort = EPCL_SRC_PORT_MAC(descrPtr);
    GT_U32  trgPort = EPCL_TRG_PORT_MAC(devObjPtr,descrPtr,egressPort);

    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, lookupConfPtr->pclID ,
                            CHT3_EPCL_KEY_FIELDS_ID_PCL_ID_E);  /*[10:1] PCL-ID */
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, srcPort ,
                            CHT3_EPCL_KEY_FIELDS_ID_SRC_PORT_E);/* [16:11] src port */
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->qos.qosProfile, /* [23:17] QOS profile */
                            CHT3_EPCL_KEY_FIELDS_ID_QOS_PROFILE_6_0_EXT_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->vid0Or1AfterTti ,/* [48:37] origVid */
                            CHT3_EPCL_KEY_FIELDS_ID_ORIG_VID_EXT_E);
    if (descrPtr->origIsTrunk)
    {
        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->origSrcEPortOrTrnk ,
                            CHT3_EPCL_KEY_FIELDS_ID_SRC_DEV_TRUNK_EXT_E); /* [57:51] source trunk id */
    }
    else
    {
        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->srcDev,
                            CHT3_EPCL_KEY_FIELDS_ID_SRC_DEV_TRUNK_EXT_E); /* [57:51] source device */
    }

    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->origIsTrunk ,/* [60] source is trunk  */
                            CHT3_EPCL_KEY_FIELDS_ID_SRC_IS_TRUNK_EXT_E);

    /* the EPCL need to use the actual mac addresses of egress and not those of ingress */
    /* For non-tunnel terminated packets:
            Transmitted packet MACs after packet modification (for example, due to routing or Tunnel Termination)
        For Ethernet-Over-xxx Tunnel-Start packets:
            MACs of the passenger packet
    */
    if(descrPtr->haToEpclInfo.macDaSaPtr)
    {
        snetCht3EPclKeyFieldBuildByPointer(pclKeyPtr, &descrPtr->haToEpclInfo.macDaSaPtr[6]/* mac SA */, /* [108:61] MAC SA*/
                                CHT3_EPCL_KEY_FIELDS_ID_MAC_SA_EXT_E);
        snetCht3EPclKeyFieldBuildByPointer(pclKeyPtr, descrPtr->haToEpclInfo.macDaSaPtr,/* [156:109] MAC DA*/
                                CHT3_EPCL_KEY_FIELDS_ID_MAC_DA_EXT_E);
    }

    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL2Valid ,/* [157] isL2Valid  */
                            CHT3_EPCL_KEY_FIELDS_ID_IS_L2_VALID_EXT_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->isIp,/* [158] is IP */
                            CHT3_EPCL_KEY_FIELDS_ID_IS_IP_EXT_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->ipProt,/* [166:159] IP Protocol*/
                            CHT3_EPCL_KEY_FIELDS_ID_IP_PROTOCOL_EXT_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dscp,/* [172:167] dscp */
                            CHT3_EPCL_KEY_FIELDS_ID_DSCP_EXT_E);

    l3HdrStartPtr = SNET_CHT3_PCKT_TUNNEL_START_PSGR(descrPtr);

    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, l3HdrStartPtr[1] & 0x3, /* [174:173] TOS[1:0] */
                            CHT3_EPCL_KEY_FIELDS_ID_TOS_1_0_EXT_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->ttl,          /* [182-175] Packet TTL */
                            CHT3_EPCL_KEY_FIELDS_ID_TTL_EXT_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->arp ,         /* [183] isArp */
                            CHT3_EPCL_KEY_FIELDS_ID_IS_ARP_EXT_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[3],       /* [215:184] SIP*/
                            CHT3_EPCL_KEY_FIELDS_ID_SIP_31_0_EXT_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[3],       /* [247:216] DIP*/
                            CHT3_EPCL_KEY_FIELDS_ID_DIP_31_0_EXT_E);

    rc = snetChtIPclTcpUdpPortRangeCompareGet(devObjPtr, descrPtr, &byteValue);
    byteValue = (rc != GT_OK) ? 0 : byteValue ;
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, byteValue,              /* [255:248] TCP/UDP comparator */
                            CHT3_EPCL_KEY_FIELDS_ID_TCP_UDP_PORT_COMPS_EXT_E);

    if (descrPtr->l4StartOffsetPtr != NULL)
    {
        tmpFieldVal[0] = descrPtr->l4StartOffsetPtr[0];
        tmpFieldVal[1] = descrPtr->l4StartOffsetPtr[1];
        tmpFieldVal[2] = descrPtr->l4StartOffsetPtr[13];
        snetCht3EPclKeyFieldBuildByPointer(
            pclKeyPtr,(GT_U8*)&tmpFieldVal[0],                          /* [279:256] L4 Byte Offsets[1:0],[13] */
                            CHT3_EPCL_KEY_FIELDS_ID_L4_BYTES_OFFSETS_0_1_13_EXT_E);

        if (descrPtr->ipv6Icmp || descrPtr->ipv4Icmp)
        {/* [295:280] L4 Byte Offsets 5:4 */
            snetCht3EPclKeyFieldBuildByPointer(
                pclKeyPtr,(GT_U8*)&(descrPtr->l4StartOffsetPtr[4]),
                            CHT3_EPCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_4_5_EXT_E);
        }
        else
        {/* [295:280] L4 Byte Offsets 3:2 */
            snetCht3EPclKeyFieldBuildByPointer(
                pclKeyPtr,(GT_U8*)&(descrPtr->l4StartOffsetPtr[2]),
                            CHT3_EPCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_2_3_EXT_E);
        }
    }

    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL4Valid,
                            CHT3_EPCL_KEY_FIELDS_ID_IS_L4_VALID_EXT_E);/* [296] isL4Valid*/

    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->tc,           /* [299:297] TC */
                            CHT3_EPCL_KEY_FIELDS_ID_TC_EXT_E);

    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dp,           /* [299:297] DP */
                            CHT3_EPCL_KEY_FIELDS_ID_DP_EXT_E);

    SNET_CHT3_EPCL_KEY_EGT_PKT_TYPE(descrPtr, fieldVal);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, fieldVal, /* [303:302] Packet type */
                            CHT3_EPCL_KEY_FIELDS_ID_PACKET_TYPE_EXT_E);
    if (descrPtr->outGoingMtagCmd == SKERNEL_MTAG_CMD_TO_CPU_E)
    {
        fieldVal = (descrPtr->srcTrg & 0x1);
    }
    else
    if (descrPtr->outGoingMtagCmd == SKERNEL_MTAG_CMD_TO_TRG_SNIFFER_E)
    {
        fieldVal = ~(descrPtr->rxSniff & 0x1);
    }
    else
    {
        fieldVal = 0;
    }

    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, fieldVal, /* [304:304] SrcTrg/TxMirror */
                            CHT3_EPCL_KEY_FIELDS_ID_SRC_TRG_TX_MIRR_EXT_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->up,           /* [307:305] UP */
                            CHT3_EPCL_KEY_FIELDS_ID_ASSIGNED_UP_EXT_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, trgPort,           /* [313:308] TrgPort */
                            CHT3_EPCL_KEY_FIELDS_ID_TRG_PORT_EXT_E);
    fieldVal = (descrPtr->rxSniff) ? 1 : 0;
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, fieldVal,              /* [314] TrgPort */
                            CHT3_EPCL_KEY_FIELDS_ID_RX_SNIF_EXT_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr,      /* [315] isRouted  */
                            descrPtr->routed,
                            CHT3_EPCL_KEY_FIELDS_ID_IS_ROUTED_EXT_E);

    fieldVal = (descrPtr->isIp && descrPtr->isIPv4) ? 1 : 0;
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, fieldVal,                           /* [316] IPVersion  */
                            CHT3_EPCL_KEY_FIELDS_ID_IP_VER_EXT_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpV6EhExists,    /* [317] ipv6_eh_exist */
                            CHT3_EPCL_KEY_FIELDS_ID_IPV6_EH_TRIPLE_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->solicitationMcastMsg,     /* [318] IPv6_IsND */
                            CHT3_EPCL_KEY_FIELDS_ID_IPV6_IS_ND_TRIPLE_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpV6EhHopByHop,  /* [319] ipv6_eh_hopbyhop */
                            CHT3_EPCL_KEY_FIELDS_ID_IPV6_EH_HOP_TRIPLE_E);

    snetCht3EPclKeyFieldBuildByU32Pointer(pclKeyPtr, descrPtr->sip,        /* [415:320] SIP*/
                            CHT3_EPCL_KEY_FIELDS_ID_SIP_127_32_TRIPLE_E);
    snetCht3EPclKeyFieldBuildByU32Pointer(pclKeyPtr, descrPtr->dip,       /* [511:416] DIP[127-32]*/
                            CHT3_EPCL_KEY_FIELDS_ID_DIP_127_32_TRIPLE_E);
    if (devObjPtr->ePclKeyFormatVersion >= 2)
    {
        /* fields common to keys 6..7*/
        __LOG(("fields common to keys 6..7"));
        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, TAG1_EXIST_MAC(descrPtr) ,
                                    LION2_EPCL_KEY_FIELDS_ID_KEY_6_7_TAG1_EXISTS_E);
        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, trgPort >> 6 ,
                                    LION2_EPCL_KEY_FIELDS_ID_KEY_6_7_TRG_PORT_6_7_E);
        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sstId ,
                                    LION2_EPCL_KEY_FIELDS_ID_KEY_6_7_SRC_ID_E);
        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr,srcPort >> 6,
                                    LION2_EPCL_KEY_FIELDS_ID_KEY_6_7_SRC_PORT_6_7_E);
        if(descrPtr->outGoingMtagCmd == SKERNEL_MTAG_CMD_FORWARD_E && descrPtr->origIsTrunk)
        {
            snetCht3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->origSrcEPortOrTrnk,
                LION2_EPCL_KEY_FIELDS_ID_KEY_6_7_SRC_DEV_0_8_OR_SRC_TRUNKID_0_8_E);
        }
        else
        {
            snetCht3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->srcDev,
                LION2_EPCL_KEY_FIELDS_ID_KEY_6_7_SRC_DEV_0_8_OR_SRC_TRUNKID_0_8_E);
        }

        /* KEY 7 specifics */
        __LOG(("KEY 7 specifics"));
        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->vid1 ,
                                    LION2_EPCL_KEY_FIELDS_ID_KEY7_TAG1_VID_E);
        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->cfidei1 ,
                                    LION2_EPCL_KEY_FIELDS_ID_KEY7_TAG1_CFI_E);
        if(descrPtr->mpls)
        {
            snetCht3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->exp1,
                LION2_EPCL_KEY_FIELDS_ID_KEY7_UP1_OR_EXP_E);
        }
        else
        {
            snetCht3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->up1,
                LION2_EPCL_KEY_FIELDS_ID_KEY7_UP1_OR_EXP_E);
        }

        if(descrPtr->outGoingMtagCmd == SKERNEL_MTAG_CMD_FORWARD_E && descrPtr->origIsTrunk)
        {
            snetCht3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->origSrcEPortOrTrnk >> 9,
                LION2_EPCL_KEY_FIELDS_ID_KEY7_SRC_DEV_9_11_OR_SRC_TRUNKID_9_11_E);
        }
        else
        {
            snetCht3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->srcDev >> 9,
                LION2_EPCL_KEY_FIELDS_ID_KEY7_SRC_DEV_9_11_OR_SRC_TRUNKID_9_11_E);
        }

        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr,
            descrPtr->marvellTagged ?
                descrPtr->origSrcEPortOrTrnk :
                descrPtr->eArchExtInfo.localDevSrcEPort,
            LION2_EPCL_KEY_FIELDS_ID_KEY7_SRC_EPORT_E);

        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr,
            descrPtr->trgEPort,
            LION2_EPCL_KEY_FIELDS_ID_KEY7_TRG_EPORT_E);

        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr,
            descrPtr->eVid >> 12,
            LION2_EPCL_KEY_FIELDS_ID_KEY7_E_VID_12_15_E);

    }
    return;
}

/*******************************************************************************
*   snetCht3EPclKeyBuildL2IPv6ExtendedKey
*
* DESCRIPTION:
*        Build Layer2+IPv6 extended ePCL search key.
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       descrPtr            - pointer to frame data buffer Id.
*        egressPort     - local egress port (not global).
*       lookupConfPtr       - pointer to the ePCL configuration entry.
*       pclExtraDataPtr     - pointer to extra data needed for the ePCL engine.
*
* OUTPUTS:
*       pclKeyPtr    - pointer to ePCL key structure.
*
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID snetCht3EPclKeyBuildL2IPv6ExtendedKey
(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr   ,
    IN  GT_U32                                egressPort,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC  * lookupConfPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC    * pclExtraDataPtr ,
    OUT SNET_CHT_POLICY_KEY_STC         * pclKeyPtr
)
{
    DECLARE_FUNC_NAME(snetCht3EPclKeyBuildL2IPv6ExtendedKey);

    GT_U32              fieldVal; /* valid bit value */
    GT_U32              resrvdVal; /* reserved bit value */
    GT_U8               byteValue;/* value of tcp/udp compare */
    GT_STATUS           rc;/* return code from function */
    GT_U8               tmpFieldVal[4];
    GT_U32  srcPort = EPCL_SRC_PORT_MAC(descrPtr);

    fieldVal = 1;
    resrvdVal = 0;

    if (devObjPtr->ePclKeyFormatVersion == 0)
    {
        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, fieldVal, /* bits[0] Valid */
                                            CHT3_EPCL_KEY_FIELDS_ID_VALID_E);
    }

    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, lookupConfPtr->pclID ,
                                        CHT3_EPCL_KEY_FIELDS_ID_PCL_ID_E);/*[10:1] ePCL-ID */
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, srcPort ,
                                        CHT3_EPCL_KEY_FIELDS_ID_SRC_PORT_E);/* [16:11] src port */
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->egressTagged ,/* [17] is tagged */
                                        CHT3_EPCL_KEY_FIELDS_ID_IS_TAGGED_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->eVid ,/* [29:18] eVid */
                                        CHT3_EPCL_KEY_FIELDS_ID_VID_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->up  ,/* [32:30] UP */
                                        CHT3_EPCL_KEY_FIELDS_ID_UP_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->qos.qosProfile,/* [39:33] QOS profile */
                                        CHT3_EPCL_KEY_FIELDS_ID_QOS_PROFILE_E);   /* [40] is IPv6 */
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, (descrPtr->isIPv4==0 && descrPtr->isIp),
                                        CHT3_EPCL_KEY_FIELDS_ID_IS_IPV6_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->isIp,/* [41] is IP */
                                        CHT3_EPCL_KEY_FIELDS_ID_IS_IP_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->ipProt,/* [49:42] IP Protocol*/
                                        CHT3_EPCL_KEY_FIELDS_ID_IP_PROTOCOL_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dscp,/* [55:50] dscp */
                                        CHT3_EPCL_KEY_FIELDS_ID_DSCP_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL4Valid, /* [56] isL4Valid*/
                                        CHT3_EPCL_KEY_FIELDS_ID_IS_L4_VALID_E);
    if (descrPtr->l4StartOffsetPtr != NULL)
    {
        if (descrPtr->ipv6Icmp || descrPtr->ipv4Icmp)
        {/* [72:57] L4 Byte Offsets 5:4 */
            snetCht3EPclKeyFieldBuildByPointer(
                pclKeyPtr,(GT_U8*)&(descrPtr->l4StartOffsetPtr[4]),
                        CHT3_EPCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_4_5_E);
        }
        else
        {/* [72:57] L4 Byte Offsets 3:2 */
            snetCht3EPclKeyFieldBuildByPointer(
                pclKeyPtr,(GT_U8*)&(descrPtr->l4StartOffsetPtr[2]),
                        CHT3_EPCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_2_3_E);
        }
        tmpFieldVal[0] = descrPtr->l4StartOffsetPtr[0];
        tmpFieldVal[1] = descrPtr->l4StartOffsetPtr[1];
        tmpFieldVal[2] = descrPtr->l4StartOffsetPtr[13];
        snetCht3EPclKeyFieldBuildByPointer(
            pclKeyPtr,(GT_U8*)&tmpFieldVal[0],/* [73:96] L4 Byte Offsets[1:0],[13] */
                CHT3_EPCL_KEY_FIELDS_ID_L4_BYTES_OFFSETS_0_1_13_LONG_E);
    }
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[3], /* [128:97] SIP*/
                                    CHT3_EPCL_KEY_FIELDS_ID_SIP_31_0_LONG_E);

    if (devObjPtr->ePclKeyFormatVersion)
    {
        /* xCat2 EPCL Key format */
        __LOG(("xCat2 EPCL Key format"));
        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[2], /* [159:128] SIP*/
                                        XCAT2_EPCL_KEY_FIELDS_ID_SIP_63_32_LONG_E);
        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[1], /* [190:160]  SIP*/
                                        XCAT2_EPCL_KEY_FIELDS_ID_SIP_94_64_LONG_E);
        snetCht3EPclKeyFieldBuildByU32Pointer(pclKeyPtr, descrPtr->sip, /* [238:206] SIP*/
                                        XCAT2_EPCL_KEY_FIELDS_ID_SIP_127_95_LONG_E);
    }
    else
    {
        /* Ch3, xCat, Lion EPCL Key format */
        __LOG(("Ch3, xCat, Lion EPCL Key format"));
        snetCht3EPclKeyFieldBuildByU32Pointer(pclKeyPtr, descrPtr->sip, /* [224:129] SIP*/
                                        CHT3_EPCL_KEY_FIELDS_ID_SIP_127_32_LONG_E);
    }

    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpV6EhExists, /* [225] ipv6_eh_exist */
                                    CHT3_EPCL_KEY_FIELDS_ID_IS_IPV6_EH_EXISTS_LONG_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpV6EhHopByHop, /* [226] ipv6_eh_hopbyhop */
                                    CHT3_EPCL_KEY_FIELDS_ID_IS_IPV6_EH_HOP_BY_HOP_LONG_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, (descrPtr->dip[0] >> 24),  /* [234:227] DIP */
                                    CHT3_EPCL_KEY_FIELDS_ID_DIP_127_120_LONG_E);
    /* the EPCL need to use the actual mac addresses of egress and not those of ingress */
    /* For non-tunnel terminated packets:
            Transmitted packet MACs after packet modification (for example, due to routing or Tunnel Termination)
        For Ethernet-Over-xxx Tunnel-Start packets:
            MACs of the passenger packet
    */
    if(descrPtr->haToEpclInfo.macDaSaPtr)
    {
        snetCht3EPclKeyFieldBuildByPointer(pclKeyPtr, &descrPtr->haToEpclInfo.macDaSaPtr[6]/* mac SA */,/* [282:235] MAC SA*/
                                        CHT3_EPCL_KEY_FIELDS_ID_MAC_SA_LONG_E);
        snetCht3EPclKeyFieldBuildByPointer(pclKeyPtr, descrPtr->haToEpclInfo.macDaSaPtr,/* [330:283] MAC DA*/
                                        CHT3_EPCL_KEY_FIELDS_ID_MAC_DA_LONG_E);
    }

    /* TCP/UDP Range Comparators is enable */
    __LOG(("TCP/UDP Range Comparators is enable"));
    rc = snetCht2EPclTcpUdpPortRangeCompareGet(devObjPtr,descrPtr,&byteValue);
    byteValue = (rc != GT_OK) ? 0 : byteValue ;
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [378:371] TCP/UDP comparator */
                        CHT3_EPCL_KEY_FIELDS_ID_TCP_UDP_PORT_COMPS_LONG_E);

    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, resrvdVal,/* Reserved [379:380] */
                            CHT3_EPCL_KEY_FIELDS_ID_RESERVED_380_379_XCAT2_394_393_LONG_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, resrvdVal,/* is L2 valid [370] */
                            CHT3_EPCL_KEY_FIELDS_ID_IS_L2_VALID_LONG_E);
    SNET_CHT3_EPCL_KEY_EGT_PKT_TYPE(descrPtr, fieldVal);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, fieldVal, /* [381:382] packet type */
                                CHT3_EPCL_KEY_FIELDS_ID_PACKET_TYPE_LONG_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->useVidx,
                            CHT3_EPCL_KEY_FIELDS_ID_IS_VIDX_LONG_E);/* [383] isVidx  */

    if (devObjPtr->ePclKeyFormatVersion >= 2)
    {
        /* fields common to keys 0..5*/
        __LOG(("fields common to keys 0..5"));
        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, TAG1_EXIST_MAC(descrPtr) ,
                                    LION2_EPCL_KEY_FIELDS_ID_KEY_0_5_TAG1_EXISTS_E);


        /* fields common to keys 3..5*/
        __LOG(("fields common to keys 3..5"));
        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr,srcPort >> 6,
                                    LION2_EPCL_KEY_FIELDS_ID_KEY_3_5_SRC_PORT_6_7_E);
        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->srcDev >> 5 ,
                                    LION2_EPCL_KEY_FIELDS_ID_KEY_3_5_SRC_DEV_5_6_E);

        if(descrPtr->outGoingMtagCmd == SKERNEL_MTAG_CMD_FORWARD_E && descrPtr->origIsTrunk)
        {
            snetCht3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->origSrcEPortOrTrnk >> 7,
                LION2_EPCL_KEY_FIELDS_ID_KEY_3_5_SRC_DEV_7_11_OR_TRUNKID_7_11_E);
        }
        else
        {
            snetCht3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->srcDev >> 7,
                LION2_EPCL_KEY_FIELDS_ID_KEY_3_5_SRC_DEV_7_11_OR_TRUNKID_7_11_E);
        }

        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sstId >> 5 ,
                                    LION2_EPCL_KEY_FIELDS_ID_KEY_3_5_SRC_ID_5_11_E);

        /* KEY 4 specifics */
        __LOG(("KEY 4 specifics"));
        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->vid1 ,
                                    LION2_EPCL_KEY_FIELDS_ID_KEY4_TAG1_VID_E);
        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->cfidei1 ,
                                    LION2_EPCL_KEY_FIELDS_ID_KEY4_TAG1_CFI_E);
        if(descrPtr->mpls)
        {
            snetCht3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->exp1,
                LION2_EPCL_KEY_FIELDS_ID_KEY4_UP1_OR_EXP_E);
        }
        else
        {
            snetCht3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->up1,
                LION2_EPCL_KEY_FIELDS_ID_KEY4_UP1_OR_EXP_E);
        }

    }

    return  ;
}

/*******************************************************************************
*   snetCht3EPclBuildL2L3L4ExtendedKey
*
* DESCRIPTION:
*        Build layer2+IPv4+layer4 extended epcl search tcam key.
*
* INPUTS:
*       devObjPtr            - pointer to device object.
*       descrPtr             - pointer to frame data buffer Id.
*        egressPort     - local egress port (not global).
*       lookupConfPtr        - pointer to the ePCL configuration entry.
*       pclExtraDataPtr      - pointer to extra data needed for the ePCL engine.
*
* OUTPUTS:
*       pclKeyPtr    - pointer to PCL key structure.
*
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID snetCht3EPclBuildL2L3L4ExtendedKey
(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr    ,
    IN  GT_U32                                egressPort,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC  * lookupConfPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC    * pclExtraDataPtr  ,
    OUT SNET_CHT_POLICY_KEY_STC         * pclKeyPtr
)
{
    DECLARE_FUNC_NAME(snetCht3EPclBuildL2L3L4ExtendedKey);

    GT_U32              fieldVal; /* valid bit value */
    GT_U32              resrvdVal;  /* reserved bit value */
    GT_U8               byteValue;/* value tcp/udp compare */
    GT_STATUS           rc;/* return code from function */
    GT_U32              tmpVal; /* temp val */
    GT_U8               tmpFieldVal[4];
    GT_U32  srcPort = EPCL_SRC_PORT_MAC(descrPtr);

    fieldVal = 1;
    resrvdVal = 0;

    if (devObjPtr->ePclKeyFormatVersion == 0)
    {
        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, fieldVal,
                                CHT3_EPCL_KEY_FIELDS_ID_VALID_E);/* bits[0] Valid */
    }

    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, lookupConfPtr->pclID ,
                            CHT3_EPCL_KEY_FIELDS_ID_PCL_ID_E);/* [10:1] ePCL-ID */
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, srcPort ,
                            CHT3_EPCL_KEY_FIELDS_ID_SRC_PORT_E);/* [16:11] src port */
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->egressTagged ,
                            CHT3_EPCL_KEY_FIELDS_ID_IS_TAGGED_E);/* [17] is tagged */
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->eVid ,
                            CHT3_EPCL_KEY_FIELDS_ID_VID_E);/* [29:18] eVid */
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->up  ,
                            CHT3_EPCL_KEY_FIELDS_ID_UP_E);/* [32:30] UP */
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->qos.qosProfile,
                            CHT3_EPCL_KEY_FIELDS_ID_QOS_PROFILE_E); /* [39:33] QOS profile */
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, (descrPtr->isIPv4==0 && descrPtr->isIp)  ,
                            CHT3_EPCL_KEY_FIELDS_ID_IS_IPV6_E); /* [40] is IPv6 */
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->isIp,
                            CHT3_EPCL_KEY_FIELDS_ID_IS_IP_E); /* [41] is IP */
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->ipProt,
                            CHT3_EPCL_KEY_FIELDS_ID_IP_PROTOCOL_E);/* [49:42] IP Protocol*/
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dscp,
                            CHT3_EPCL_KEY_FIELDS_ID_DSCP_E);/* [55:50] dscp */
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL4Valid,
                            CHT3_EPCL_KEY_FIELDS_ID_IS_L4_VALID_E);/* [56] isL4Valid*/
    if (descrPtr->l4StartOffsetPtr != NULL)
    {
        if (descrPtr->ipv6Icmp || descrPtr->ipv4Icmp)
        {
            snetCht3EPclKeyFieldBuildByPointer(
                pclKeyPtr,(GT_U8*)&(descrPtr->l4StartOffsetPtr[4]),
                    CHT3_EPCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_4_5_E);/* [72:57] L4 Byte Offsets 5:4 */
        }
        else
        {
            snetCht3EPclKeyFieldBuildByPointer(
                pclKeyPtr,(GT_U8*)&(descrPtr->l4StartOffsetPtr[2]),
                    CHT3_EPCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_2_3_E);/* [72:57] L4 Byte Offsets 3:2 */
        }
        tmpFieldVal[0] = descrPtr->l4StartOffsetPtr[0];
        tmpFieldVal[1] = descrPtr->l4StartOffsetPtr[1];
        tmpFieldVal[2] = descrPtr->l4StartOffsetPtr[13];
        snetCht3EPclKeyFieldBuildByPointer(
            pclKeyPtr,(GT_U8*)&tmpFieldVal[0],/* [96:73] L4 Byte Offsets[1:0],[13] */
                                CHT3_EPCL_KEY_FIELDS_ID_L4_BYTES_OFFSETS_0_1_13_LONG_E);
    }

    /* for IPv4 packets only the first bye sip[0] or dip[0] is relevant */
    __LOG(("for IPv4 packets only the first bye sip[0] or dip[0] is relevant"));
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[0], /* [128:97] SIP*/
                                    CHT3_EPCL_KEY_FIELDS_ID_SIP_31_0_LONG_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[0], /* [160:129] DIP*/
                                    CHT3_EPCL_KEY_FIELDS_ID_DIP_31_0_LONG_E);
    tmpVal = (descrPtr->l2Encaps == SKERNEL_ETHERNET_II_E ||
                       descrPtr->l2Encaps == SKERNEL_LLC_SNAP_E) ? 1 : 0;
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, tmpVal,/* [161] Encap Type */
                        CHT3_EPCL_KEY_FIELDS_ID_ENCAP_TYPE_LONG_VALID_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->etherTypeOrSsapDsap,/* [177:162] */
                        CHT3_EPCL_KEY_FIELDS_ID_ETHER_TYPE_DSAP_SSAP_LONG_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpv4Fragment,/* [178] */
                        CHT3_EPCL_KEY_FIELDS_ID_LONG_IPV4_FRAGMENT_E);
    /* the EPCL need to use the actual mac addresses of egress and not those of ingress */
    /* For non-tunnel terminated packets:
            Transmitted packet MACs after packet modification (for example, due to routing or Tunnel Termination)
        For Ethernet-Over-xxx Tunnel-Start packets:
            MACs of the passenger packet
    */
    if(descrPtr->haToEpclInfo.macDaSaPtr)
    {
        snetCht3EPclKeyFieldBuildByPointer(pclKeyPtr, &descrPtr->haToEpclInfo.macDaSaPtr[6]/* mac SA */,/* [282:235] MAC SA*/
                                CHT3_EPCL_KEY_FIELDS_ID_MAC_SA_LONG_E);
        snetCht3EPclKeyFieldBuildByPointer(pclKeyPtr, descrPtr->haToEpclInfo.macDaSaPtr,/* [330:283] MAC DA*/
                                CHT3_EPCL_KEY_FIELDS_ID_MAC_DA_LONG_E);
    }

    /* TCP/UDP Range Comparators is enable */
    __LOG(("TCP/UDP Range Comparators is enable"));
    rc = snetCht2EPclTcpUdpPortRangeCompareGet(devObjPtr,descrPtr,&byteValue);
    byteValue = (rc != GT_OK) ? 0 : byteValue ;
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [378:371] TCP/UDP comparator */
                        CHT3_EPCL_KEY_FIELDS_ID_TCP_UDP_PORT_COMPS_LONG_E);

    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, resrvdVal,/* Reserved [380:379] */
                            CHT3_EPCL_KEY_FIELDS_ID_RESERVED_380_379_XCAT2_394_393_LONG_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, resrvdVal,/* is L2 valid [370] */
                            CHT3_EPCL_KEY_FIELDS_ID_IS_L2_VALID_LONG_E);
    SNET_CHT3_EPCL_KEY_EGT_PKT_TYPE(descrPtr, fieldVal);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, fieldVal, /* [381:382] packet type */
                                CHT3_EPCL_KEY_FIELDS_ID_PACKET_TYPE_LONG_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->useVidx,
                            CHT3_EPCL_KEY_FIELDS_ID_IS_VIDX_LONG_E);/* [383] isVidx */


    if (devObjPtr->ePclKeyFormatVersion >= 2)
    {
        /* fields common to keys 0..5*/
        __LOG(("fields common to keys 0..5"));
        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, TAG1_EXIST_MAC(descrPtr) ,
                                    LION2_EPCL_KEY_FIELDS_ID_KEY_0_5_TAG1_EXISTS_E);


        /* fields common to keys 3..5*/
        __LOG(("fields common to keys 3..5"));
        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr,srcPort >> 6,
                                    LION2_EPCL_KEY_FIELDS_ID_KEY_3_5_SRC_PORT_6_7_E);
        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->srcDev >> 5 ,
                                    LION2_EPCL_KEY_FIELDS_ID_KEY_3_5_SRC_DEV_5_6_E);

        if(descrPtr->outGoingMtagCmd == SKERNEL_MTAG_CMD_FORWARD_E && descrPtr->origIsTrunk)
        {
            snetCht3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->origSrcEPortOrTrnk >> 7,
                LION2_EPCL_KEY_FIELDS_ID_KEY_3_5_SRC_DEV_7_11_OR_TRUNKID_7_11_E);
        }
        else
        {
            snetCht3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->srcDev >> 7,
                LION2_EPCL_KEY_FIELDS_ID_KEY_3_5_SRC_DEV_7_11_OR_TRUNKID_7_11_E);
        }

        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sstId >> 5 ,
                                    LION2_EPCL_KEY_FIELDS_ID_KEY_3_5_SRC_ID_5_11_E);

        /* KEY 3 specifics */
        __LOG(("KEY 3 specifics"));

        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->vid1 ,
                                    LION2_EPCL_KEY_FIELDS_ID_KEY3_TAG1_VID_E);
        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->cfidei1 ,
                                    LION2_EPCL_KEY_FIELDS_ID_KEY3_TAG1_CFI_E);
        if(descrPtr->mpls)
        {
            snetCht3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->exp1,
                LION2_EPCL_KEY_FIELDS_ID_KEY3_UP1_OR_EXP_E);
        }
        else
        {
            snetCht3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->up1,
                LION2_EPCL_KEY_FIELDS_ID_KEY3_UP1_OR_EXP_E);
        }
        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->numOfLabels,
            LION2_EPCL_KEY_FIELDS_ID_KEY3_NUMBER_OF_MPLS_LABELS_E);
        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->protOverMpls,
            LION2_EPCL_KEY_FIELDS_ID_KEY3_PROTOCOL_AFTER_MPLS_E);
        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->label2,
            LION2_EPCL_KEY_FIELDS_ID_KEY3_MPLS_LABEL1_E);
        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->exp2,
            LION2_EPCL_KEY_FIELDS_ID_KEY3_MPLS_EXP1_E);
        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->mpls,
            LION2_EPCL_KEY_FIELDS_ID_KEY3_IS_MPLS_E);
        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->label1,
            LION2_EPCL_KEY_FIELDS_ID_KEY3_MPLS_LABEL0_E);
        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->exp1,
            LION2_EPCL_KEY_FIELDS_ID_KEY3_MPLS_EXP0_E);
    }


    return ;
}

/*******************************************************************************
*   snetCht3EPclBuildL3L4StandardKey
*
* DESCRIPTION:
*        Build IPv4+Layer4 standard epcl search key.
*
* INPUTS:
*       devObjPtr            - pointer to device object.
*       descrPtr             - pointer to frame data buffer Id.
*        egressPort     - local egress port (not global).
*       lookupConfPtr        - pointer to the current PCL configuration entry.
*       pclExtraDataPtr      - pointer to extra data needed for the ePCL engine.
*
* OUTPUTS:
*       pclKeyPtr    - pointer to PCL key structure.
*
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID snetCht3EPclBuildL3L4StandardKey
(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr     ,
    IN  GT_U32                                egressPort,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC  * lookupConfPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC    * pclExtraDataPtr  ,
    OUT SNET_CHT_POLICY_KEY_STC         * pclKeyPtr
)
{
    DECLARE_FUNC_NAME(snetCht3EPclBuildL3L4StandardKey);

    GT_U32              fieldVal;   /* value for valid bit */
    GT_U32              resrvdVal;   /* reserved value */
    GT_U8               byteValue;    /* value of tcp/udp compare */
    GT_U32              tmpVal;  /* temp val */
    GT_U8               tmpFieldVal[4];
    GT_STATUS           rc; /* return code */
    GT_U32  srcPort = EPCL_SRC_PORT_MAC(descrPtr);

    fieldVal = 1;
    resrvdVal = 0;
    byteValue = 0;

    if (devObjPtr->ePclKeyFormatVersion == 0)
    {
        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, fieldVal, /* bits[0] Valid */
                                            CHT3_EPCL_KEY_FIELDS_ID_VALID_E);
    }

    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, lookupConfPtr->pclID,
                                        CHT3_EPCL_KEY_FIELDS_ID_PCL_ID_E);/* [10:1] PCL-ID */
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, srcPort ,
                                        CHT3_EPCL_KEY_FIELDS_ID_SRC_PORT_E);/* [16:11] src port */
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->egressTagged ,/* [17] is tagged */
                                        CHT3_EPCL_KEY_FIELDS_ID_IS_TAGGED_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->eVid ,/* [29:18] eVid */
                                        CHT3_EPCL_KEY_FIELDS_ID_VID_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->up  ,/* [32:30] UP */
                                        CHT3_EPCL_KEY_FIELDS_ID_UP_E);

    /* filling bits [39:33] depends on the marvell tag command */
    __LOG(("filling bits [39:33] depends on the marvell tag command"));
    switch (descrPtr->outGoingMtagCmd)
    {
    case SKERNEL_MTAG_CMD_FORWARD_E :
        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->qos.qosProfile,
                                        CHT3_EPCL_KEY_FIELDS_ID_QOS_PROFILE_E);
        break;
    case SKERNEL_MTAG_CMD_TO_CPU_E:
        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->cpuCode,/* [39:33] CPU_CODE[6:0]*/
                                CHT3_EPCL_KEY_FIELDS_ID_CPU_CODE_0_6_STANDARD_E);
        break;
    case SKERNEL_MTAG_CMD_FROM_CPU_E:
        tmpVal = 0x0;
        tmpVal = SMEM_U32_GET_FIELD(descrPtr->qos.fromCpuQos.fromCpuDp ,0,2);
        tmpVal |= (SMEM_U32_GET_FIELD(descrPtr->qos.fromCpuQos.fromCpuTc ,0,3) << 2);
        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr,tmpVal,/* [39:33] 2'b0,TC[2:0],DP[1:0] */
                                        CHT3_EPCL_KEY_FIELDS_ID_QOS_PROFILE_E);
        break;
    default :
        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, resrvdVal ,/* [39:33] reserved  */
                                        CHT3_EPCL_KEY_FIELDS_ID_RESERVED_39_33_XCAT2_38_32_E);
        break;
    }

    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->isIPv4 , /* [40] is IPv4 */
                                        CHT3_EPCL_KEY_FIELDS_ID_IS_IPV4_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->isIp,/* [41] is IP */
                                        CHT3_EPCL_KEY_FIELDS_ID_IS_IP_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->ipProt,/* [49:42] IP Protocol*/
                                        CHT3_EPCL_KEY_FIELDS_ID_IP_PROTOCOL_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dscp,/* [55:50] dscp */
                                        CHT3_EPCL_KEY_FIELDS_ID_DSCP_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL4Valid, /* [56] isL4Valid*/
                                        CHT3_EPCL_KEY_FIELDS_ID_IS_L4_VALID_E);
    if (descrPtr->l4StartOffsetPtr != NULL)
    {
        if (descrPtr->ipv6Icmp || descrPtr->ipv4Icmp)
        {
            snetCht3EPclKeyFieldBuildByPointer(
                pclKeyPtr,(GT_U8*)&(descrPtr->l4StartOffsetPtr[4]),
                    CHT3_EPCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_4_5_E);/* [72:57] L4 Byte Offsets 5:4 */
        }
        else
        {
            snetCht3EPclKeyFieldBuildByPointer(
                pclKeyPtr,(GT_U8*)&(descrPtr->l4StartOffsetPtr[2]),
                    CHT3_EPCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_2_3_E);/* [72:57] L4 Byte Offsets 3:2 */
        }
    }

    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->arp ,/* [73] isArp */
                                    CHT3_EPCL_KEY_FIELDS_ID_IS_ARP_E);
    tmpVal = (descrPtr->macDaType == SKERNEL_BROADCAST_MAC_E ||
              descrPtr->macDaType == SKERNEL_BROADCAST_ARP_E) ? 1 : 0 ;
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr,  tmpVal,/* [74] IS bc   */
                                    CHT3_EPCL_KEY_FIELDS_ID_IS_BC_E);

    if (descrPtr->origIsTrunk)
    {
        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->origSrcEPortOrTrnk ,
                                        CHT3_EPCL_KEY_FIELDS_ID_TRUNK_ID);/* [81:75] source trunk id */
    }
    else
    {
        byteValue = 0;
        /* [81] TO_CPU : Desc<SrcTrg>
           [81] TO_ANALYSER : Desc<rx_sniff> */
        if (descrPtr->outGoingMtagCmd == SKERNEL_MTAG_CMD_TO_CPU_E)
        {
            byteValue |= ( descrPtr->srcTrg & 0x1 ) << 0x5;
        }
        else
        if (descrPtr->outGoingMtagCmd == SKERNEL_MTAG_CMD_TO_TRG_SNIFFER_E)
        {
            byteValue |= ( descrPtr->rxSniff & 0x1 ) << 0x5;
        }
        else
        /* [80] FROM_CPU : Desc<EgressFilterEn> */
        if (descrPtr->outGoingMtagCmd == SKERNEL_MTAG_CMD_FROM_CPU_E)
        {
            byteValue |= ( descrPtr->egressFilterEn & 0x1 ) << 0x4;
        }
        /* [79:75] source device */
        byteValue |= (descrPtr->srcDev & 0x1F) ;
        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, byteValue ,
                                            CHT3_EPCL_KEY_FIELDS_ID_SRCDEV_ID);
    }

    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->origIsTrunk ,/* [82] source is trunk  */
                            CHT3_EPCL_KEY_FIELDS_ID_SRC_IS_TRUNK_SHORT_E);

    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sstId ,/* [87:83] Source-ID */
                                        CHT3_EPCL_KEY_FIELDS_ID_SST_ID_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->routed ,/* [88] isRouted  */
                                        CHT3_EPCL_KEY_FIELDS_ID_IS_ROUTED);

    switch (descrPtr->outGoingMtagCmd)
    {
        case SKERNEL_MTAG_CMD_TO_CPU_E: /* [89] CPU_CODE[7]  */
            tmpVal = SMEM_U32_GET_FIELD(descrPtr->cpuCode,7,1);
            snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, tmpVal ,
                                CHT3_EPCL_KEY_FIELDS_ID_CPU_CODE_7_STANDARD_E);
            break;
        case SKERNEL_MTAG_CMD_FORWARD_E : /*  isUnknown [89] */
            snetCht3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->macDaFound ,
                                        CHT3_EPCL_KEY_FIELDS_ID_IS_UNKNOWN_UC_E);
            break;
        default : /* reserved [89] */
            snetCht3EPclKeyFieldBuildByValue(pclKeyPtr,resrvdVal ,
                                        CHT3_EPCL_KEY_FIELDS_ID_RESERVED_89_XCAT2_88_E );
    }

    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL2Valid ,/* [90] isL2Valid  */
                            CHT3_EPCL_KEY_FIELDS_ID_IS_L2_VALID_SHORT_E);
    rc = snetCht2EPclTcpUdpPortRangeCompareGet(devObjPtr,descrPtr,&byteValue);
    byteValue = (rc != GT_OK) ? 0 : byteValue ;
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [98:91] TCP/UDP comparator */
                             CHT3_EPCL_KEY_FIELDS_ID_TCP_UDP_PORT_COMPS_SHORT_E);

    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[0], /* [130:99] SIP*/
                             CHT3_EPCL_KEY_FIELDS_ID_SIP_31_0_SHORT_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[0], /* [162:131] DIP*/
                             CHT3_EPCL_KEY_FIELDS_ID_DIP_31_0_IPV4_L4_SHORT_E);
    if (descrPtr->l4StartOffsetPtr != NULL)
    {
        tmpFieldVal[0] = descrPtr->l4StartOffsetPtr[0];
        tmpFieldVal[1] = descrPtr->l4StartOffsetPtr[1];
        tmpFieldVal[2] = descrPtr->l4StartOffsetPtr[13];
        snetCht3EPclKeyFieldBuildByPointer(
            pclKeyPtr,(GT_U8*)&tmpFieldVal[0],/* [186:163] L4 Byte Offsets[1:0],[13] */
                    CHT3_EPCL_KEY_FIELDS_ID_L4_BYTES_OFFSETS_0_1_13_SHORT_E);
    }
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, resrvdVal, /* [187] Reserved */
                                        CHT3_EPCL_KEY_FIELDS_ID_RESERVED_187_XCAT2_186_E);
    SNET_CHT3_EPCL_KEY_EGT_PKT_TYPE(descrPtr, fieldVal);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, fieldVal, /* [189:188] packet type */
                                        CHT3_EPCL_KEY_FIELDS_ID_PACKET_TYPE_SHORT_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpv4Fragment , /* [190] ipv4 fragment */
                            CHT3_EPCL_KEY_FIELDS_ID_IPV4_FRAGMENTED_SHORT_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->useVidx ,/* [191] isVidx  */
                            CHT3_EPCL_KEY_FIELDS_ID_IS_VIDX_SHORT_E);

    if (devObjPtr->ePclKeyFormatVersion >= 2)
    {
        /* fields common to keys 0..5*/
        __LOG(("fields common to keys 0..5"));
        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, TAG1_EXIST_MAC(descrPtr) ,
                                    LION2_EPCL_KEY_FIELDS_ID_KEY_0_5_TAG1_EXISTS_E);
    }


    return  ;
}

/*******************************************************************************
*   snetCht3EPclBuildL2L3StandardKey
*
* DESCRIPTION:
*        Build Layer2+IPv4\6+QoS standard epcl search key.
*
* INPUTS:
*       devObjPtr            - pointer to device object.
*       descrPtr             - pointer to frame data buffer Id.
*        egressPort     - local egress port (not global).
*       currentLookupConfPtr - pointer to the current PCL configuration entry.
*       pclExtraDataPtr      - pointer to extra data needed for the ePCL engine.
*
* OUTPUTS:
*       pclKeyPtr    - pointer to PCL key structure.
*
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID snetCht3EPclBuildL2L3StandardKey
(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr,
    IN  GT_U32                                egressPort,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC  * lookupConfPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC    * pclExtraDataPtr  ,
    OUT SNET_CHT_POLICY_KEY_STC         * pclKeyPtr
)
{
    DECLARE_FUNC_NAME(snetCht3EPclBuildL2L3StandardKey);

    GT_U32          fieldVal;
    GT_U32          resrvdVal;
    GT_U8           byteValue;
    GT_U8           tmpFieldVal[4];
    GT_U32          tmpVal;
    GT_STATUS       rc;
    GT_U32  srcPort = EPCL_SRC_PORT_MAC(descrPtr);

    fieldVal = 1;
    resrvdVal = 0;

    if (devObjPtr->ePclKeyFormatVersion == 0)
    {
        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, fieldVal, /* bits[0] Valid */
                                CHT3_EPCL_KEY_FIELDS_ID_VALID_E);
    }

    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, lookupConfPtr->pclID,
                            CHT3_EPCL_KEY_FIELDS_ID_PCL_ID_E);/* [10:1] ePCL-ID */
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, srcPort ,
                            CHT3_EPCL_KEY_FIELDS_ID_SRC_PORT_E);/* [16:11] src port */
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->egressTagged ,/* [17] is tagged */
                            CHT3_EPCL_KEY_FIELDS_ID_IS_TAGGED_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->eVid ,/* [29:18] eVid */
                            CHT3_EPCL_KEY_FIELDS_ID_VID_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->up  ,/* [32:30] UP */
                            CHT3_EPCL_KEY_FIELDS_ID_UP_E);

    switch (descrPtr->outGoingMtagCmd)
    {
        case SKERNEL_MTAG_CMD_FORWARD_E : /* [39:33] QOS profile */
            snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->qos.qosProfile,
                                        CHT3_EPCL_KEY_FIELDS_ID_QOS_PROFILE_E);
        break;
        case SKERNEL_MTAG_CMD_TO_CPU_E: /* [39:33] CPU_CODE[6:0]*/
            snetCht3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->cpuCode,
                            CHT3_EPCL_KEY_FIELDS_ID_CPU_CODE_0_6_STANDARD_E);
        break;
        case SKERNEL_MTAG_CMD_FROM_CPU_E: /* [39:33] 2'b0,TC[2:0],DP[1:0] */
            tmpVal = 0x0;
            tmpVal = SMEM_U32_GET_FIELD(descrPtr->qos.fromCpuQos.fromCpuDp ,0,2);
            tmpVal |= (SMEM_U32_GET_FIELD(descrPtr->qos.fromCpuQos.fromCpuTc ,0,3) << 2);
            snetCht3EPclKeyFieldBuildByValue(pclKeyPtr,tmpVal,
                                CHT3_EPCL_KEY_FIELDS_ID_QOS_PROFILE_E);
        break;
        default : /* [39:33] reserved  */
            snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, resrvdVal ,
                                    CHT3_EPCL_KEY_FIELDS_ID_RESERVED_39_33_XCAT2_38_32_E);
    }

    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->isIPv4 , /* [40] is IPv4 */
                                            CHT3_EPCL_KEY_FIELDS_ID_IS_IPV4_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->isIp,/* [41] is IP */
                                            CHT3_EPCL_KEY_FIELDS_ID_IS_IP_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->ipProt,/* [49:42] IP Protocol*/
                                        CHT3_EPCL_KEY_FIELDS_ID_IP_PROTOCOL_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dscp,/* [55:50] dscp */
                                        CHT3_EPCL_KEY_FIELDS_ID_DSCP_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL4Valid, /* [56] isL4Valid*/
                                        CHT3_EPCL_KEY_FIELDS_ID_IS_L4_VALID_E);

    if (descrPtr->l4StartOffsetPtr != NULL)
    {
        if (descrPtr->ipv6Icmp || descrPtr->ipv4Icmp)
        {
            snetCht3EPclKeyFieldBuildByPointer(/* [72:57] L4 Byte Offsets 5:4 */
                pclKeyPtr,(GT_U8*)&(descrPtr->l4StartOffsetPtr[4]),
                                CHT3_EPCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_4_5_E);
        }
        else
        {
            snetCht3EPclKeyFieldBuildByPointer(/* [72:57] L4 Byte Offsets 3:2 */
                pclKeyPtr,(GT_U8*)&(descrPtr->l4StartOffsetPtr[2]),
                                CHT3_EPCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_2_3_E);
        }
    }

    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->arp ,/* [73] isArp */
                                        CHT3_EPCL_KEY_FIELDS_ID_IS_ARP_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, resrvdVal ,/* [74] reserved  */
                                        CHT3_EPCL_KEY_FIELDS_ID_RESERVED_74_XCAT2_73_E);
    if (descrPtr->origIsTrunk)
    {
        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->origSrcEPortOrTrnk ,
                CHT3_EPCL_KEY_FIELDS_ID_TRUNK_ID);/* [81:75] source trunk id */
    }
    else
    {
        byteValue = 0;
        /* [81] TO_CPU : Desc<SrcTrg>
           [81] TO_ANALYSER : Desc<rx_sniff> */
        if (descrPtr->outGoingMtagCmd == SKERNEL_MTAG_CMD_TO_CPU_E)
        {
                byteValue |= ( descrPtr->srcTrg & 0x1 ) << 0x5;
        }
        else
        if (descrPtr->outGoingMtagCmd == SKERNEL_MTAG_CMD_TO_TRG_SNIFFER_E)
        {
                byteValue |= ( descrPtr->rxSniff & 0x1 ) << 0x5;
        }
        else
        /* [80] FROM_CPU : Desc<EgressFilterEn> */
        if (descrPtr->outGoingMtagCmd == SKERNEL_MTAG_CMD_FROM_CPU_E)
        {
                byteValue |= ( descrPtr->egressFilterEn & 0x1 ) << 0x4;
        }
        /* [79:75] source trunk id */
        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->srcDev ,
                                            CHT3_EPCL_KEY_FIELDS_ID_SRCDEV_ID);
    }

    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->origIsTrunk ,/* [82] source is trunk  */
                            CHT3_EPCL_KEY_FIELDS_ID_SRC_IS_TRUNK_SHORT_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sstId ,/* [87:83] Source-ID */
                            CHT3_EPCL_KEY_FIELDS_ID_SST_ID_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->routed ,/* [88] isRouted  */
                            CHT3_EPCL_KEY_FIELDS_ID_IS_ROUTED);

    if (descrPtr->outGoingMtagCmd == SKERNEL_MTAG_CMD_TO_CPU_E)
    {
        tmpVal = SMEM_U32_GET_FIELD(descrPtr->cpuCode,7,1);
        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, tmpVal ,/* [89] CPU_CODE[7]  */
                        CHT3_EPCL_KEY_FIELDS_ID_CPU_CODE_7_STANDARD_E);
    }
    else if (descrPtr->outGoingMtagCmd == SKERNEL_MTAG_CMD_FORWARD_E)
    {/*  isUnknown [89] */
        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->macDaFound ,
                                        CHT3_EPCL_KEY_FIELDS_ID_IS_UNKNOWN_UC_E);
    }
    else
    {   /* reserved [89] */
        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr,resrvdVal ,
                                CHT3_EPCL_KEY_FIELDS_ID_RESERVED_89_XCAT2_88_E);
    }

    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL2Valid ,/* [90] isL2Valid  */
                            CHT3_EPCL_KEY_FIELDS_ID_IS_L2_VALID_SHORT_E);
    rc = snetCht2EPclTcpUdpPortRangeCompareGet(devObjPtr,descrPtr,&byteValue);
    byteValue = (rc != GT_OK) ? 0 : byteValue ;
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [98:91] TCP/UDP comparator */
                             CHT3_EPCL_KEY_FIELDS_ID_TCP_UDP_PORT_COMPS_SHORT_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[0], /* [130:99] DIP[31:0] */
                                        CHT3_EPCL_KEY_FIELDS_ID_DIP_31_0_E);
    if (descrPtr->l4StartOffsetPtr != NULL)
    {
        tmpFieldVal[0] = descrPtr->l4StartOffsetPtr[13];
        snetCht3EPclKeyFieldBuildByPointer(
            pclKeyPtr,(GT_U8*)&tmpFieldVal[0],/* [138:131] L4 Byte Offsets[13] */
                        CHT3_EPCL_KEY_FIELDS_ID_L4_BYTES_OFFSETS_13_SHORT_E);
    }

    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, resrvdVal ,/* [139] reserved  */
                            CHT3_EPCL_KEY_FIELDS_ID_RESERVED_139_XCAT2_138_SHORT_E);
    /* the EPCL need to use the actual mac addresses of egress and not those of ingress */
    /* For non-tunnel terminated packets:
            Transmitted packet MACs after packet modification (for example, due to routing or Tunnel Termination)
        For Ethernet-Over-xxx Tunnel-Start packets:
            MACs of the passenger packet
    */
    if(descrPtr->haToEpclInfo.macDaSaPtr)
    {
        snetCht3EPclKeyFieldBuildByPointer(pclKeyPtr, descrPtr->haToEpclInfo.macDaSaPtr,/* [187:140] MAC DA*/
                                CHT3_EPCL_KEY_FIELDS_ID_MAC_DA_SHORT_E);
    }

    SNET_CHT3_EPCL_KEY_EGT_PKT_TYPE(descrPtr, fieldVal);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, fieldVal, /* [189:188] packet type */
                            CHT3_EPCL_KEY_FIELDS_ID_PACKET_TYPE_SHORT_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, (GT_U32)pclExtraDataPtr->isIpv4Fragment , /* [190] ipv4 fragment */
                            CHT3_EPCL_KEY_FIELDS_ID_IPV4_FRAGMENTED_SHORT_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->useVidx ,/* [191] isVidx  */
                                    CHT3_EPCL_KEY_FIELDS_ID_IS_VIDX_SHORT_E);

    if (devObjPtr->ePclKeyFormatVersion >= 2)
    {
        /* fields common to keys 0..5*/
        __LOG(("fields common to keys 0..5"));
        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, TAG1_EXIST_MAC(descrPtr) ,
                                    LION2_EPCL_KEY_FIELDS_ID_KEY_0_5_TAG1_EXISTS_E);
    }

    return ;
}

/*******************************************************************************
*   snetCht3EPclBuildL2StandardKey
*
* DESCRIPTION:
*        Build layer2 standard epcl search key.
*
* INPUTS:
*       devObjPtr            - pointer to device object.
*       descrPtr             - pointer to frame data buffer Id.
*        egressPort     - local egress port (not global).
*       lookupConfPtr        - pointer to EPCL configuration entry.
*       pclExtraDataPtr      - pointer to extra data needed for the EPCL engine.
*
* OUTPUTS:
*
*       pclKeyPtr            - pointer to EPCL key structure.
*
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID snetCht3EPclBuildL2StandardKey
(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr,
    IN  GT_U32                                egressPort,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC  * lookupConfPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC    * pclExtraDataPtr,
    OUT SNET_CHT_POLICY_KEY_STC         * pclKeyPtr
)
{
    DECLARE_FUNC_NAME(snetCht3EPclBuildL2StandardKey);

    GT_U32  fieldVal = 1;
    GT_U32  resrvdVal = 0;
    GT_U32  tmpVal;
    GT_U32  byteVal = 0 ;
    GT_U32  srcPort = EPCL_SRC_PORT_MAC(descrPtr);

    if (devObjPtr->ePclKeyFormatVersion == 0)
    {
        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, fieldVal, /* bits[0] Valid */
                                            CHT3_EPCL_KEY_FIELDS_ID_VALID_E);
    }

    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, lookupConfPtr->pclID ,
                                        CHT3_EPCL_KEY_FIELDS_ID_PCL_ID_E);/*[10:1] PCL-ID */
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, srcPort ,
                                        CHT3_EPCL_KEY_FIELDS_ID_SRC_PORT_E);/* [16:11] src port */
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->egressTagged ,/* [17] is tagged */
                                        CHT3_EPCL_KEY_FIELDS_ID_IS_TAGGED_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->eVid ,/* [29:18] eVid */
                                        CHT3_EPCL_KEY_FIELDS_ID_VID_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->up  ,/* [32:30] UP */
                                        CHT3_EPCL_KEY_FIELDS_ID_UP_E);

    switch (descrPtr->outGoingMtagCmd)
    {
        case SKERNEL_MTAG_CMD_FORWARD_E:/* [39:33] QOS profile */
            snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->qos.qosProfile,
                                        CHT3_EPCL_KEY_FIELDS_ID_QOS_PROFILE_E);
        break;
        case SKERNEL_MTAG_CMD_TO_CPU_E: /* [39:33] CPU_CODE[6:0]*/
            tmpVal = SMEM_U32_GET_FIELD(descrPtr->cpuCode,0,6);
            snetCht3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->cpuCode,
                                CHT3_EPCL_KEY_FIELDS_ID_CPU_CODE_0_6_STANDARD_E);
        break;
        case SKERNEL_MTAG_CMD_FROM_CPU_E:/* [39:33] 2'b0,TC[2:0],DP[1:0] */
            tmpVal = 0;
            tmpVal = SMEM_U32_GET_FIELD(descrPtr->qos.fromCpuQos.fromCpuDp ,0,2);
            tmpVal |= (SMEM_U32_GET_FIELD(descrPtr->qos.fromCpuQos.fromCpuTc ,0,3) << 2);
            snetCht3EPclKeyFieldBuildByValue(pclKeyPtr,tmpVal,
                                         CHT3_EPCL_KEY_FIELDS_ID_QOS_PROFILE_E);
        break;
        default :/* [39:33] reserved  */
            snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, resrvdVal ,
                                    CHT3_EPCL_KEY_FIELDS_ID_RESERVED_39_33_XCAT2_38_32_E);
    }

    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->isIPv4 ,/* [40] is IPv4 */
                                        CHT3_EPCL_KEY_FIELDS_ID_IS_IPV4_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->isIp,/* [41] is IP */
                                        CHT3_EPCL_KEY_FIELDS_ID_IS_IP_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, resrvdVal,/*[42] reserved field*/
                                        CHT3_EPCL_KEY_FIELDS_ID_RESERVED_42_XCAT2_41_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->etherTypeOrSsapDsap,/* [58:43] */
                            CHT3_EPCL_KEY_FIELDS_ID_ETHER_TYPE_DSAP_SSAP_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, resrvdVal,/* [66:59] reserved */
                            CHT3_EPCL_KEY_FIELDS_ID_RESERVED_66_59_XCAT2_65_58_SHORT_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, resrvdVal,/* [72:67] reserved  */
                                    CHT3_EPCL_KEY_FIELDS_ID_RESERVED_72_67_XCAT2_71_66_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->arp ,/* [73] isArp */
                                        CHT3_EPCL_KEY_FIELDS_ID_IS_ARP_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, resrvdVal ,/* [74] reserved  */
                                        CHT3_EPCL_KEY_FIELDS_ID_RESERVED_74_XCAT2_73_E);

    if (descrPtr->origIsTrunk)
    {/* [81:75] source trunk id */
        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->origSrcEPortOrTrnk ,
                                            CHT3_EPCL_KEY_FIELDS_ID_TRUNK_ID);
    }
    else
    {
        byteVal = 0;
        /* [81] TO_CPU : Desc<SrcTrg>
           [81] TO_ANALYSER : Desc<rx_sniff> */
        if (descrPtr->outGoingMtagCmd == SKERNEL_MTAG_CMD_TO_CPU_E)
        {
                byteVal |= ( descrPtr->srcTrg & 0x1 ) << 0x5;
        }
        else
        if (descrPtr->outGoingMtagCmd == SKERNEL_MTAG_CMD_TO_TRG_SNIFFER_E)
        {
                byteVal |= ( descrPtr->rxSniff & 0x1 ) << 0x5;
        }
        else
        /* [80] FROM_CPU : Desc<EgressFilterEn> */
        if (descrPtr->outGoingMtagCmd == SKERNEL_MTAG_CMD_FROM_CPU_E)
        {
                byteVal |= ( descrPtr->egressFilterEn & 0x1 ) << 0x4;
        }
        /* [79:75] source device */
        byteVal |= SMEM_U32_GET_FIELD(descrPtr->srcDev , 0x0 , 0x5);
        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, byteVal ,
                                        CHT3_EPCL_KEY_FIELDS_ID_SRCDEV_ID);
    }

    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->origIsTrunk ,/* [82] source is trunk  */
                            CHT3_EPCL_KEY_FIELDS_ID_SRC_IS_TRUNK_SHORT_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sstId ,/* [87:83] Source-ID */
                                        CHT3_EPCL_KEY_FIELDS_ID_SST_ID_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->routed ,/* [88] isRouted  */
                                        CHT3_EPCL_KEY_FIELDS_ID_IS_ROUTED);
    switch (descrPtr->outGoingMtagCmd)
    {
        case SKERNEL_MTAG_CMD_TO_CPU_E : /* [89] CPU_CODE[7]  */
            tmpVal = SMEM_U32_GET_FIELD(descrPtr->cpuCode,7,1);
            snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, tmpVal ,
                            CHT3_EPCL_KEY_FIELDS_ID_CPU_CODE_7_STANDARD_E);
        break;
        case SKERNEL_MTAG_CMD_FORWARD_E :     /*  isUnknown [89] */
            snetCht3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->macDaFound ,
                                       CHT3_EPCL_KEY_FIELDS_ID_IS_UNKNOWN_UC_E);
        break;
        default :        /* reserved [89] */
            snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, resrvdVal ,
                                        CHT3_EPCL_KEY_FIELDS_ID_RESERVED_89_XCAT2_88_E );
    }

    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL2Valid ,/* [90] isL2Valid  */
                                CHT3_EPCL_KEY_FIELDS_ID_IS_L2_VALID_SHORT_E);
    tmpVal = (descrPtr->l2Encaps == SKERNEL_ETHERNET_II_E ||
                       descrPtr->l2Encaps == SKERNEL_LLC_SNAP_E) ? 1 : 0;
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, tmpVal,/* [91] Encap Type */
                                CHT3_EPCL_KEY_FIELDS_ID_ENCAP_TYPE_SHORT_E);
    /* the EPCL need to use the actual mac addresses of egress and not those of ingress */
    /* For non-tunnel terminated packets:
            Transmitted packet MACs after packet modification (for example, due to routing or Tunnel Termination)
        For Ethernet-Over-xxx Tunnel-Start packets:
            MACs of the passenger packet
    */
    if(descrPtr->haToEpclInfo.macDaSaPtr)
    {
        snetCht3EPclKeyFieldBuildByPointer(pclKeyPtr, &descrPtr->haToEpclInfo.macDaSaPtr[6]/* mac SA */,/* [139:92] MAC SA*/
                                    CHT3_EPCL_KEY_FIELDS_ID_MAC_SA_SHORT_E);
        snetCht3EPclKeyFieldBuildByPointer(pclKeyPtr, descrPtr->haToEpclInfo.macDaSaPtr,/* [187:140] MAC DA*/
                                    CHT3_EPCL_KEY_FIELDS_ID_MAC_DA_SHORT_E);
    }
    SNET_CHT3_EPCL_KEY_EGT_PKT_TYPE(descrPtr, fieldVal);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, fieldVal, /* [189:188] packet type */
                                CHT3_EPCL_KEY_FIELDS_ID_PACKET_TYPE_SHORT_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, resrvdVal ,/* [190] Reserved  */
                                CHT3_EPCL_KEY_FIELDS_ID_IS_L2_VALID_SHORT_E);
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->useVidx ,/* [191] isVidx  */
                                CHT3_EPCL_KEY_FIELDS_ID_IS_VIDX_SHORT_E);


    if (devObjPtr->ePclKeyFormatVersion >= 2)
    {
        /* fields common to keys 0..5*/
        __LOG(("fields common to keys 0..5"));
        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, TAG1_EXIST_MAC(descrPtr) ,
                                    LION2_EPCL_KEY_FIELDS_ID_KEY_0_5_TAG1_EXISTS_E);

        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->up1,
            LION2_EPCL_KEY_FIELDS_ID_KEY0_UP1_0_1_E);

        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->up1>>2,
            LION2_EPCL_KEY_FIELDS_ID_KEY0_UP1_2_E);

        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->vid1,
            LION2_EPCL_KEY_FIELDS_ID_KEY0_TAG1_VID_E);

        snetCht3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->cfidei1,
            LION2_EPCL_KEY_FIELDS_ID_KEY0_TAG1_CFI_E);
    }

    return ;
}

/*******************************************************************************
* snetCht3EPclConfigPtrGet
*
* DESCRIPTION:
*       Get data from the egress PCL configuration table.
*
* INPUTS:
*       devObjPtr            - pointer to device object.
*       descrPtr             - pointer to frame descriptor object.
*        egressPort     - local egress port (not global).
* OUTPUTS:
*       lookupConfPtr - pointer to epcl configuration table entry.
*
* RETURN:
*
* COMMENTS:
*       C.12.17.1 Egress PCL Configuration Table
*                 The EPCL configuration table is a 4160 line table.
*                 Each entry is 32-bits.
*
*******************************************************************************/
static GT_VOID snetCht3EPclConfigPtrGet
(
    IN SKERNEL_DEVICE_OBJECT                * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC      * descrPtr ,
    IN GT_U32                                 egressPort,
    OUT SNET_CHT3_PCL_LOOKUP_CONFIG_STC     * lookupConfPtr
)
{
    DECLARE_FUNC_NAME(snetCht3EPclConfigPtrGet);

    GT_U32  regAddrEPclGlobTbl;
    GT_U32  maxPortNo;
    GT_U32  index = 0;
    GT_U32  pclCfgMode = 0;
    GT_U32  egressPclCfgMode;
    GT_U32  regAddr;
    GT_U32  portIfPCLBlock;
    GT_U32  epclCfgEntry;
    GT_U32  portListEn;
    GT_U32  outputPortBit;  /* the bit index for the egress port  */
    GT_BOOL isSecondRegister;/* is the per port config is in second register -- for bmp of ports */
    GT_U32  tsNormalMode;/*tunnel start normal mode or vlan mode */
    GT_BIT  forceVlanMode = 0;


    /* convert local port to register offset and to bit offset */
    __LOG(("convert local port to register offset and to bit offset"));
    snetChtHaPerPortInfoGet(devObjPtr,egressPort,&isSecondRegister,&outputPortBit);

    /* Egress Policy Configuration Table Access mode configuration */
    __LOG(("Egress Policy Configuration Table Access mode configuration"));
    regAddr = SMEM_CHT3_EPCL_TBL_ACCESS_MODE_CONFIG_REG(devObjPtr);
    if(isSecondRegister == GT_TRUE)
    {
        regAddr +=4;
    }

    if (devObjPtr->pclSupport.pclSupportPortList)
    {
        /* Port list mode - enabled or not */
        smemRegFldGet(devObjPtr, SMEM_CHT3_EPCL_GLOBAL_REG(devObjPtr), 27, 1, &portListEn);
        __LOG(("Port list mode = [%d] \n",
            portListEn));
    }
    else
    {
        portListEn = 0;
    }

    if(descrPtr->tunnelStart)
    {
        /*EPCL TS Lookup CFG*/
        smemRegFldGet(devObjPtr, SMEM_CHT3_EPCL_GLOBAL_REG(devObjPtr), 26, 1, &tsNormalMode);

        if(tsNormalMode == 0)
        {
            __LOG(("EPCL TS Lookup CFG = 0 --> meaning 'FORCE Vlan mode' \n",
                tsNormalMode));

            forceVlanMode = 1;
        }
        else
        {
            __LOG(("EPCL TS Lookup CFG = 1 --> meaning 'Normal mode' \n",
                tsNormalMode));
        }
    }

    if(portListEn == 0)
    {
        if(forceVlanMode)
        {
            /* vlan mode is forced */
            pclCfgMode = 1;
        }
        else
        {
    /* bit per port */
    __LOG(("bit per port"));
    smemRegFldGet(devObjPtr, regAddr, outputPortBit, 1, &pclCfgMode);
        }
    }
    else
    {
        /*this is actual device behavior */
        __LOG(("Force access to the EPCL configuration table with 'PORT mode' due to <Port list mode> = 1 \n"));
        pclCfgMode = 0;/* port MODE */
    }

    if (pclCfgMode == 1)
    {
        __LOG(("Access to the EPCL configuration table with 'VLAN mode' \n"));
        index = descrPtr->eVid & 0xFFF; /* Access ePCL-id config. table with vlan id  */
    }
    else
    {
        __LOG(("Access to the EPCL configuration table with 'PORT mode' \n"));
        regAddrEPclGlobTbl = SMEM_CHT3_EPCL_GLOBAL_REG(devObjPtr);
        smemRegFldGet(devObjPtr, regAddrEPclGlobTbl, 1, 1, &egressPclCfgMode);
        if (egressPclCfgMode == 0)
        {/* EPCL configuration table index = Local Device Source Port# + 4K */
            __LOG(("EPCL configuration table index = Local Device Source Port# + 4K"));
            index = (1 << 12) | egressPort;
        }
        else
        {
            /* read the max port per device and the ePCL interface block */
            __LOG(("read the max port per device and the ePCL interface block"));
            smemRegFldGet(devObjPtr, regAddrEPclGlobTbl, 18, 1, &maxPortNo);
            smemRegFldGet(devObjPtr, regAddrEPclGlobTbl ,16, 2, &portIfPCLBlock);
            if (maxPortNo == 0)
            {   /* Max port per device = 0 , up to 1K ports in system */
                __LOG(("Max port per device = 0 , up to 1K ports in system"));
                index = ((portIfPCLBlock & 0x3)<< 10) |
                        ((descrPtr->trgDev & 0x1f)<< 5) |
                        ((descrPtr->trgEPort & 0x1f));
            }
            else
            {   /* Max port per device = 1 , up to 2K ports in system */
                __LOG(("Max port per device = 1 , up to 2K ports in system"));
                index = ((portIfPCLBlock & 0x1)<< 11) |
                        ((descrPtr->trgDev & 0x1f)<< 6) |
                        ((descrPtr->trgEPort & 0x3f));
            }
        }
    }

    /* calculate the address of the EPCL configuration table */
    __LOG(("calculate the address of the EPCL configuration table"));
    smemRegGet(devObjPtr, SMEM_CHT3_EPCL_CONFIG_TBL_MEM(devObjPtr, index),
               &epclCfgEntry);

    /* The Egress PCL configuration Table */
    __LOG(("The Egress PCL configuration Table"));
    lookupConfPtr->lookUpEn =
        SMEM_U32_GET_FIELD(epclCfgEntry,0,1);
    lookupConfPtr->pclNonIpTypeCfg =
        SMEM_U32_GET_FIELD(epclCfgEntry, 1, 2);
    lookupConfPtr->pclIpV4ArpTypeCfg =
        SMEM_U32_GET_FIELD(epclCfgEntry, 13, 2);
    lookupConfPtr->pclIpV6TypeCfg =
        SMEM_U32_GET_FIELD(epclCfgEntry, 15, 2);
    lookupConfPtr->pclID =
        SMEM_U32_GET_FIELD(epclCfgEntry, 3, 10);

    /* Note: For Egress PCL, Port-List method is applied only to packets with command = Forward */
    __LOG(("Note: For Egress PCL, Port-List method is applied only to packets with command = Forward"));
    if ( (devObjPtr->pclSupport.pclSupportPortList) &&
         (descrPtr->outGoingMtagCmd == SKERNEL_MTAG_CMD_FORWARD_E) )
    {
        /* Port list mode - enabled or not */
        lookupConfPtr->pclPortListModeEn = portListEn ? GT_TRUE : GT_FALSE;
    }
    else
    {
        lookupConfPtr->pclPortListModeEn = GT_FALSE;
    }

    lookupConfPtr->ingrlookUpKey = GT_FALSE;

    return ;
}

/*******************************************************************************
* snetCht3EPclActionApply
*
* DESCRIPTION:
*        Apply the action entry from the policy action table.
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - pointer to frame descriptor.
*       actionInfoPtr   - pointer to the ePCL action entry.
*
* OUTPUTS:
*
* RETURN:
*
* COMMENTS:
*       C.12.14  - Egress PCL action table and Policy TCAM access control
*                   Registers
*
*******************************************************************************/
GT_VOID snetCht3EPclActionApply
(
    IN SKERNEL_DEVICE_OBJECT                * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC   * descrPtr  ,
    IN SNET_CHT3_EPCL_ACTION_STC            * actionDataPtr
)
{
    SNET_CHT2_EPCL_ACTION_STC  * actionCht2DataPtr;

    actionCht2DataPtr = (SNET_CHT2_EPCL_ACTION_STC *)actionDataPtr;

    snetCht2EPclActionApply(devObjPtr, descrPtr, actionCht2DataPtr);

    return ;
}

/*******************************************************************************
* snetCht3EPclActionGet
*
* DESCRIPTION:
*       Get the action entry from the policy action table.
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - pointer to frame descriptor.
*       matchIndex      - index to the action table .
*
* OUTPUTS:
*       actionInfoPtr   - pointer to the PCL action.
*
* RETURN:
*
* COMMENTS:
*       C.12.12 - The policy Engine maintains an 1024 entries table, corresponding
*                 to the 1024 rules that may be defined in the TCAM lookup
*                 structure.
*
*******************************************************************************/
GT_VOID snetCht3EPclActionGet
(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr  ,
    IN GT_U32                             matchIndex,
    OUT SNET_CHT3_EPCL_ACTION_STC       * actionDataPtr
)
{
    SNET_CHT2_EPCL_ACTION_STC  * actionCht2DataPtr;
    GT_U32  tblAddr;
    GT_U32  * actionEntryDataPtr;
    GT_U32 fieldVal;

    actionCht2DataPtr = (SNET_CHT2_EPCL_ACTION_STC *)actionDataPtr;

    snetCht2EPclActionGet(devObjPtr, descrPtr, matchIndex, actionCht2DataPtr);

    tblAddr = SMEM_CHT_PCL_ACTION_TBL_MEM(devObjPtr, matchIndex);
    actionEntryDataPtr = smemMemGet(devObjPtr, tblAddr);

    fieldVal = SMEM_U32_GET_FIELD(actionEntryDataPtr[3], 2, 8);
    actionDataPtr->matchCounterIndex =
        fieldVal << 6 | actionDataPtr->matchCounterIndex;

    return ;
}

/*******************************************************************************
* snetXcat2EpclKeyPortsBmpBuild
*
* DESCRIPTION:
*        Build the 'ports bmp' in the EPCL search key.
*       supported from xcat2
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - pointer to frame data buffer Id.
*       egressPort      - local egress port (not global).
*
* OUTPUT:
*       pclKeyPtr    - pointer to PCL key structure.
*
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID snetXcat2EpclKeyPortsBmpBuild
(
    IN SKERNEL_DEVICE_OBJECT                * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC   * descrPtr,
    IN  GT_U32                                egressPort,
    OUT SNET_CHT_POLICY_KEY_STC             * pclKeyPtr
)
{
/*    DECLARE_FUNC_NAME(snetXcat2EpclKeyPortsBmpBuild); */

    /* PortList bitmap 0-27, separated on 3 parts */
    GT_U32  portListBits7_0   = 0;
    GT_U32  portListBits13_8  = 0;
    GT_U32  portListBits27_14 = 0;

    snetCht3EPclPortListBmpBuild(devObjPtr, descrPtr , egressPort, &portListBits7_0,
                                 &portListBits13_8, &portListBits27_14);

    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, portListBits7_0 ,
            XCAT2_EPCL_KEY_FIELDS_ID_PORT_LIST_7_0_E);/*[7:0] port_list[7:0]  */
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, portListBits13_8 ,
            XCAT2_EPCL_KEY_FIELDS_ID_PORT_LIST_13_8_E);/* xCat2 [37:32] port_list[13:8]  */
    snetCht3EPclKeyFieldBuildByValue(pclKeyPtr, portListBits27_14 ,
            XCAT2_EPCL_KEY_FIELDS_ID_PORT_LIST_27_14_E);/* xCat2 [205:192] port_list[27:14]  */

}

/*******************************************************************************
* snetCht3EPclCreateKey
*
* DESCRIPTION:
*        Create Egress PCL search tram key.
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - pointer to frame data buffer Id.
*       egressPort      - local egress port (not global).
*       lookupConfigPtr - pointer to the EPCL configuration table entry.
*       pclExtraDataPtr - pointer to extra data needed for the EPCL engine.
*       keyType         - egress pcl tcam key type.
*
* OUTPUT:
*       pclKeyPtr    - pointer to EPCL key structure.
*
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
GT_VOID snetCht3EPclCreateKey
(
    IN SKERNEL_DEVICE_OBJECT                * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC   * descrPtr,
    IN  GT_U32                                egressPort,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC      * lookupConfPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC        * pclExtraDataPtr,
    IN GT_U32                                 keyType,
    OUT SNET_CHT_POLICY_KEY_STC             * pclKeyPtr
)
{
    DECLARE_FUNC_NAME(snetCht3EPclCreateKey);

    GT_U32 fixedValues = 0;
    GT_U32  origIsTrunk = descrPtr->origIsTrunk;
    GT_U32  origSrcEPortOrTrnk = descrPtr->origSrcEPortOrTrnk;
    CHT3_EPCL_KEY_TYPE_ENT egressPclkeyType = (CHT3_EPCL_KEY_TYPE_ENT)keyType;
    GT_BOOL replaceTag0 = GT_FALSE;     /* If Tag0 in descriptor is temporary replaced
                                           by Tag1 for "use tag1 In EPCL fields" feature */
    GT_U32 storeVid0 = 0;  /* tag0 eVid saved value */
    GT_U32 storeUp0  = 0;  /* tag0 up  saved value */
    GT_U32 regValue;


    memset(&pclKeyPtr->key, 0, sizeof(pclKeyPtr->key));

    pclKeyPtr->devObjPtr = devObjPtr;
    pclKeyPtr->updateOnlyDiff = GT_FALSE;
    pclKeyPtr->pclKeyFormat = SNET_CHT3_EPCL_KEY_FORMAT(egressPclkeyType);


    /* patch for the 'TO_CPU' traffic */
    __LOG(("patch for the 'TO_CPU' traffic"));
    if (descrPtr->outGoingMtagCmd == SKERNEL_MTAG_CMD_TO_CPU_E &&
        descrPtr->packetCmd != SKERNEL_EXT_PKT_CMD_FROM_CSCD_TO_CPU_E)/* not from cascade */
    {
        fixedValues = 1;
        descrPtr->origIsTrunk = 0;
        descrPtr->origSrcEPortOrTrnk = descrPtr->localDevSrcPort;
    }

    /* "use tag1 In EPCL fields" feature */
    if (devObjPtr->pclSupport.ePclSupportVidUpTag1)
    {
        if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            /* Due to the SIP5.0 full support of lookup using Tag0 and/or Tag1 fields,
               the existing (SIP 4) global configuration <Use Tag1 in EPCL> is obsolete */
            regValue = 0;
        }
        else
        {
            /* read "Use Tag1 In EPCL" field */
            smemRegFldGet(devObjPtr, SMEM_CHT3_ROUTE_HA_GLB_CNF_REG(devObjPtr),
                          21, 1, &regValue);
        }

        if ((regValue) &&
            (descrPtr->ingressVlanTag1Type != SKERNEL_FRAME_TR101_VLAN_TAG_NOT_FOUND_E))
        {
            /* feature enabled and Tag1 exists */
            __LOG(("feature enabled and Tag1 exists"));
            storeVid0 = descrPtr->eVid;
            storeUp0 = descrPtr->up;
            replaceTag0 = GT_TRUE;
            /* replace Tag0 eVid, up by Tag1 values */
            __LOG(("replace Tag0 eVid, up by Tag1 values"));
            descrPtr->eVid = descrPtr->vid1;
            descrPtr->up = descrPtr->up1;
        }
    }

    switch (egressPclkeyType)
    {
        case CHT3_EPCL_KEY_TYPE_L2_KEY_SHORT_E:
            snetCht3EPclBuildL2StandardKey(devObjPtr, descrPtr, egressPort,
                                           lookupConfPtr, pclExtraDataPtr,
                                           pclKeyPtr);
            break;
        case CHT3_EPCL_KEY_TYPE_L2_L3_KEY_SHORT_E:
            snetCht3EPclBuildL2L3StandardKey(devObjPtr , descrPtr, egressPort, lookupConfPtr ,
                                             pclExtraDataPtr, pclKeyPtr);
            break;
        case CHT3_EPCL_KEY_TYPE_L3_L4_KEY_SHORT_E:
            snetCht3EPclBuildL3L4StandardKey(devObjPtr ,descrPtr, egressPort, lookupConfPtr ,
                                             pclExtraDataPtr, pclKeyPtr);
            break;
        case CHT3_EPCL_KEY_TYPE_L2_L3_L4_KEY_LONG_E:
            snetCht3EPclBuildL2L3L4ExtendedKey(devObjPtr, descrPtr, egressPort, lookupConfPtr ,
                                                 pclExtraDataPtr, pclKeyPtr);
            break;
        case CHT3_EPCL_KEY_TYPE_L2_IPV6_KEY_LONG_E:
            snetCht3EPclKeyBuildL2IPv6ExtendedKey(devObjPtr, descrPtr, egressPort,
                                                    lookupConfPtr ,
                                                    pclExtraDataPtr, pclKeyPtr);
            break;
        case CHT3_EPCL_KEY_TYPE_L4_IPV6_KEY_LONG_E:
            snetCht3EPclKeyBuildL4IPv6ExtendedKey(devObjPtr , descrPtr, egressPort ,
                                                  lookupConfPtr,pclExtraDataPtr,
                                                  pclKeyPtr);
            break;
        case CHT3_EPCL_KEY_TYPE_L4_IPV4_NON_IP_KEY_LONG_E:
            snetCht3EPclKeyBuildL4IPv4NonIpExtendedKey(devObjPtr, descrPtr, egressPort,
                                                  lookupConfPtr,pclExtraDataPtr,
                                                  pclKeyPtr);

            break;
        case CHT3_EPCL_KEY_TYPE_L4_IPV6_KEY_TRIPLE_E:
            snetCht3EPclKeyBuildL4IPv6TripleKey(devObjPtr, descrPtr, egressPort,
                                                  lookupConfPtr,pclExtraDataPtr,
                                                  pclKeyPtr);
            break;

        default:
            skernelFatalError("egressPclkeyType: not valid mode[%d]",egressPclkeyType);
            break;
    }

    /* Port-List mode */
    __LOG(("Port-List mode"));
    if (lookupConfPtr->pclPortListModeEn &&
        egressPclkeyType != CHT3_EPCL_KEY_TYPE_L4_IPV4_NON_IP_KEY_LONG_E &&
        egressPclkeyType != CHT3_EPCL_KEY_TYPE_L4_IPV6_KEY_TRIPLE_E)
    {
        snetXcat2EpclKeyPortsBmpBuild(devObjPtr, descrPtr, egressPort, pclKeyPtr);
    }

    if(fixedValues)
    {
        descrPtr->origIsTrunk = origIsTrunk;
        descrPtr->origSrcEPortOrTrnk = origSrcEPortOrTrnk;
    }

    if(replaceTag0)
    {
        descrPtr->eVid = storeVid0;
        descrPtr->up = storeUp0;
    }

    return ;
}
/*******************************************************************************
* snetCht3EPclTriggeringCheck
*
* DESCRIPTION:
*        Check the enabling status of egress PCL for the outgoing frame.
*        Get the epcl-id configuration setup.
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - pointer to frame data buffer Id
*        egressPort     - local egress port (not global).
*
* OUTPUTS:
*       doLookupPtr     - pointer to enabling the PCL engine.
*       lookupConfPtr   - pointer to lookup configuration.
*
* RETURN:
*
* COMMENTS:
*         1. Check is EPCL globally enabled.
*         2. Check is EPCL enabled per port .
*         3. Get Egress PCL configuration entry.
*
*******************************************************************************/
GT_VOID snetCht3EPclTriggeringCheck
(
    IN SKERNEL_DEVICE_OBJECT             *  devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC   *  descrPtr,
    IN GT_U32                               egressPort,
    OUT GT_U8                            *  doLookupPtr,
    OUT SNET_CHT3_PCL_LOOKUP_CONFIG_STC  *  lookupConfPtr
)
{
    DECLARE_FUNC_NAME(snetCht3EPclTriggeringCheck);

    GT_U32  regAddress;
    GT_U32  * regPtr;
    GT_U32  ePclPerPortState = 0;
    GT_U32         outPort;  /* the bit index for the egress port */
    GT_BOOL        isSecondRegister;/* is the per port config is in second register -- for bmp of ports */
    GT_BIT epclEnabled;

    /* initialize the dolookupPtr */
    *doLookupPtr = 0;

    snetChtHaPerPortInfoGet(devObjPtr,egressPort,&isSecondRegister,&outPort);

    regAddress = SMEM_CHT3_EPCL_GLOBAL_REG(devObjPtr);
    regPtr = smemMemGet(devObjPtr, regAddress);

    /* Enable Egress Policy check */
    __LOG(("Enable Egress Policy check"));
    epclEnabled = SMEM_U32_GET_FIELD(*regPtr, 7, 1);
    if (SKERNEL_IS_XCAT_REVISON_A1_DEV(devObjPtr))
    {
        if(epclEnabled == 0) /* logic of enabled */
            return;
    }
    else
    {
        if(epclEnabled == 1) /* logic of disabled ! */
            return;
    }

    /* check if a bit per Egress port to Enable/Disable Egress Policy
       on packets forwarded to the CPU (local or remote) */
    if (descrPtr->outGoingMtagCmd == SKERNEL_MTAG_CMD_TO_CPU_E)
    {
        regAddress = SMEM_CHT3_EPCL_TO_CPU_CONFIG_REG(devObjPtr);
        if(isSecondRegister == GT_TRUE)
        {
            regAddress +=4;
        }
        smemRegFldGet(devObjPtr, regAddress, outPort, 1, &ePclPerPortState);
        if (ePclPerPortState == 0)
        {
            return ;
        }
    }

    /* check if a bit per Egress port to Enable/Disable Egress Policy
       on data packets from CPU .*/
    if (descrPtr->outGoingMtagCmd == SKERNEL_MTAG_CMD_FROM_CPU_E)
    {
        if(descrPtr->egressFilterEn)/* may be filtered --> data */
        {
            regAddress = SMEM_CHT3_EPCL_FROM_CPU_DATA_CONFIG_REG(devObjPtr);
        }
        else /* can not be filtered --> control */
        {
            regAddress = SMEM_CHT3_EPCL_FROM_CPU_CONFIG_REG(devObjPtr);
        }

        if(isSecondRegister == GT_TRUE)
        {
            regAddress +=4;
        }
        smemRegFldGet(devObjPtr, regAddress, outPort, 1, &ePclPerPortState);
        if (ePclPerPortState == 0)
        {
            return ;
        }
    }

    /* check if a bit per Egress port to Enable/Disable Egress Policy
       on data packets from CPU . packetCmd saves the marvell tag command
       received on the incoming frame */
    if ((descrPtr->localDevSrcPort == SNET_CHT_CPU_PORT_CNS) ||
       (descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_FROM_CPU_E &&
        descrPtr->egressFilterEn))
    {
        regAddress = SMEM_CHT3_EPCL_FROM_CPU_CONFIG_REG(devObjPtr);
        smemRegFldGet(devObjPtr, regAddress, outPort, 1, &ePclPerPortState);
        if (ePclPerPortState == 0)
        {
            return ;
        }
    }

    /* check if a bit per Egress port to Enable/Disable Egress Policy on
        TO_ANALYSER packets e.g. packets forwarded to the ingress or egress
        analyzer port */
    if (descrPtr->outGoingMtagCmd == SKERNEL_MTAG_CMD_TO_TRG_SNIFFER_E)
    {
        regAddress = SMEM_CHT3_EPCL_TO_ANALYZER_CONFIG_REG(devObjPtr);
        if(isSecondRegister == GT_TRUE)
        {
            regAddress +=4;
        }
        smemRegFldGet(devObjPtr, regAddress, outPort, 1, &ePclPerPortState);
        if (ePclPerPortState == 0)
        {
            return ;
        }
    }

    /* check if a bit per Egress port to Enable/Disable Egress Policy
       on data packets which are NOT tunneled in this device */
    if ((descrPtr->outGoingMtagCmd == SKERNEL_MTAG_CMD_FORWARD_E)
        && (descrPtr->tunnelStart == 0))
    {
        regAddress = SMEM_CHT3_EPCL_NTS_FORWARD_CONFIG_REG(devObjPtr);
        if(isSecondRegister == GT_TRUE)
        {
            regAddress +=4;
        }
        smemRegFldGet(devObjPtr, regAddress, outPort, 1, &ePclPerPortState);
        if (ePclPerPortState == 0)
        {
            return ;
        }
    }

    /* check if a bit per Egress port to Enable/Disable Egress Policy
       on data packets which are tunneled in this device */
    if ((descrPtr->outGoingMtagCmd == SKERNEL_MTAG_CMD_FORWARD_E)
        && (descrPtr->tunnelStart != 0))
    {
        regAddress = SMEM_CHT3_EPCL_TS_FORWARD_CONFIG_REG(devObjPtr);
        if(isSecondRegister == GT_TRUE)
        {
            regAddress +=4;
        }
        smemRegFldGet(devObjPtr, regAddress, outPort, 1, &ePclPerPortState);
        if (ePclPerPortState == 0)
        {
            return ;
        }
    }

    /* get EPCL configuration entry */
    __LOG(("get EPCL configuration entry"));
    memset(lookupConfPtr, 0, sizeof(SNET_CHT3_PCL_LOOKUP_CONFIG_STC));
    snetCht3EPclConfigPtrGet(devObjPtr,descrPtr,egressPort,lookupConfPtr);

    *doLookupPtr = (lookupConfPtr->lookUpEn == 0) ? 0 : 1;

    return ;
}

/*******************************************************************************
* snetCht3EPcl
*
* DESCRIPTION:
*        Egress Policy Engine processing for outgoing frame on Cheetah3
*        asic simulation.
*        ePCL processing , ePcl assignment ,key forming , 1 Lookup ,
*        actions to descriptor processing
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - pointer to frame data buffer Id
*        egressPort     - local egress port (not global).
*
* OUTPUTS:
*       descrPtr     - pointer to updated frame data buffer Id
*
* RETURN:
*
* COMMENTS:
*   Egress PCL has only one lookup cycle.
*   C.12.9 : Egress Policy Registers
*
*******************************************************************************/
GT_VOID snetCht3EPcl
(
    IN SKERNEL_DEVICE_OBJECT                * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC   * descrPtr,
    IN  GT_U32                                egressPort
)
{
    DECLARE_FUNC_NAME(snetCht3EPcl);

    SNET_CHT3_PCL_LOOKUP_CONFIG_STC lookUpCfg; /* lookup configuration table */
    GT_U8                           doLookup;  /* enable searching the TCAM */
    CHT_PCL_EXTRA_PACKET_INFO_STC   pclExtraData;
    SNET_CHT_POLICY_KEY_STC         pclKey; /* policy key structure */
    GT_U32                          keyType;    /* tcam search key type */
    GT_U32                          matchIndex; /* index to the matching rule */
    SNET_CHT3_EPCL_ACTION_STC       actionData;

    /* Get the ePCL configuration entry (Table 270 : EPCL configuration Table)*/
    __LOG(("Get the ePCL configuration entry (Table 270 : EPCL configuration Table)"));
    snetCht3EPclTriggeringCheck(devObjPtr, descrPtr, egressPort, &doLookup, &lookUpCfg);
    if (doLookup == 0) /* =0 , means ePCL engine is disabled */
    {
        return ;
    }

    /* parse extra data need by the epcl engine */
    __LOG(("parse extra data need by the epcl engine"));
    snetChtPclSrvParseExtraData(devObjPtr,  descrPtr,  &pclExtraData);

    /* Get from the configuration the key type and the key size */
    __LOG(("Get from the configuration the key type and the key size"));
    snetCht3PclGetKeyType(descrPtr, &lookUpCfg, &keyType);
    /* Create egress policy tcam search key */
    __LOG(("Create egress policy tcam search key"));
    pclKey.devObjPtr = devObjPtr;
    snetCht3EPclCreateKey(devObjPtr, descrPtr,egressPort, &lookUpCfg,
                            &pclExtraData, keyType, &pclKey);

    /* search key in policy Tcam */
    __LOG(("search key in policy Tcam"));
    snetChtPclSrvTcamLookUp(devObjPtr, devObjPtr->pclTcamInfoPtr, &pclKey , &matchIndex);

    /* read and apply policy action */
    __LOG(("read and apply policy action"));
    if (matchIndex != SNET_CHT_POLICY_NO_MATCH_INDEX_CNS)
    {   /* read the action from the action table */
        __LOG(("read the action from the action table"));
        snetCht3EPclActionGet(devObjPtr, descrPtr, matchIndex , &actionData);
        /* apply the matching action */
        __LOG(("apply the matching action"));
        snetCht3EPclActionApply(devObjPtr, descrPtr , &actionData);

        if(actionData.matchCounterEn)
        {
            /* Egress Policy CNC */
            __LOG(("Egress Policy CNC"));
            snetCht3CncCount(
                devObjPtr, descrPtr,
                SNET_CNC_CLIENT_EPCL_ACTION_0_E, actionData.matchCounterIndex);
        }
    }

    return;
}

