/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetCheetah3Pcl.h
*
* DESCRIPTION:
*       Cheetah3 Asic Simulation .
*       Ingress Policy Engine processing for incoming frame.
*       header file.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 25 $
*
*******************************************************************************/
#ifndef __snetCheetah3Pclh
#define __snetCheetah3Pclh

#include <asicSimulation/SKernel/suserframes/snetCheetah2Pcl.h>
#include <asicSimulation/SKernel/suserframes/snetCheetah3CentralizedCnt.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define CHT3_PCL_TCAM_BANK_NUMBERS              4
#define CHT3_PCL_TCAM_BANK_ENTRIES              (14*1024 / CHT3_PCL_TCAM_BANK_NUMBERS) /*3.5K*/
#define XCAT_PCL_TCAM_BANK_ENTRIES              ( 3*1024 / CHT3_PCL_TCAM_BANK_NUMBERS) /*0.75K*/
#define XCAT2_PCL_TCAM_BANK_ENTRIES             ( 1024 / CHT3_PCL_TCAM_BANK_NUMBERS) /*0.25K*/
#define LION2_PCL_TCAM_BANK_ENTRIES             ( 2*1024 / CHT3_PCL_TCAM_BANK_NUMBERS) /*0.5K*/
#define CHT3_PCL_TCAM_BANK_ENTRIES_STEP         16
#define CHT3_PCL_TCAM_BANK_ENTRY_WORDS          4
#define CHT3_PCL_TCAM_BANK_ENTRY_WORDS_STEP     2
#define CHT3_PCL_TCAM_BANK_DATA_BITS            48
#define CHT3_PCL_TCAM_BANK_CTRL_BITS            4
#define XCAT2_PCL_TCAM_BANK_DATA_BITS           52
/* the xcat2 uses the TCAM with key that already hold control bits so it is part of the data */
#define XCAT2_PCL_TCAM_BANK_CTRL_BITS           0

#define SNET_CHT3_ACTION_STOP_CHECK_MAC(action_ptr)   \
    if ((action_ptr)->actionStop)          \
        break;

/* calc result from x,y,k
 x - X value
 y - Y value
 k - key value
 mask - mask that represent the number of bits involved

 return value
 1 - match
 0 - no match
*/
#define CH3_TCAM_X_Y_K_MATCH(x,y,k,mask) \
    (((mask) & (((~x) & (~k)) | ((~y) & (k))) ) == (mask))


/******************************************************************************
 *  CHT3_PCL_KEY_TYPE_ENT
 *
 *  DESCRIPTION : The key type used for PCL lookup
 *
********************************************************************************/
typedef enum{
    CHT3_PCL_KEY_TYPE_L2_KEY_SHORT_E,               /* L2  ingress, standard                    */
    CHT3_PCL_KEY_TYPE_L2_L3_KEY_SHORT_E,            /* L2+IPv4/6+QOS - ingress, standard         */
    CHT3_PCL_KEY_TYPE_L3_L4_KEY_SHORT_E,            /* IPv4+L4 - ingress , standard              */
    CHT3_PCL_KEY_TYPE_L2_L3_L4_KEY_LONG_E,          /* L2+IPv4+L4 - ingress, extended            */
    CHT3_PCL_KEY_TYPE_L2_IPV6_KEY_LONG_E,           /* L2+IPv6 - ingress, extended               */
    CHT3_PCL_KEY_TYPE_L4_IPV6_KEY_LONG_E,           /* L4+IPv6 - ingress, extended               */
    CHT3_PCL_KEY_TYPE_IPCL0_IPV4_KEY_LONG_E,        /* Port/VLAN + QoS - ingress, PV-ACL extended*/
    CHT3_PCL_KEY_TYPE_IPCL1_IPV4_KEY_SHORT_E,       /* ACL + QoS - ingress, R_ACL standard       */
    CHT3_PCL_KEY_TYPE_IPCL0_IPV6_KEY_TRIPLE_E,      /* ACL + QoS - ingress, R_ACL triple         */
    CHT3_PCL_KEY_TYPE_IPCL1_IPV6_KEY_TRIPLE_E,      /* ACL + QoS - ingress, R_ACL triple         */
    CHT3_PCL_KEY_TYPE_IPV6_DIP_KEY_SHORT_E,         /* lookup for implementing 98DX253/263/273 (Cheetah+) */
    CHT3_PCL_KEY_TYPE_LAST_E
}SNET_CHT3_PCL_KEY_TYPE_ENT;

/*
    enum :  CHT3_EPCL_KEY_TYPE_ENT

    description : enum of the egress PCL TCAM KEY fields

*/
typedef enum{
/*Key#
   0*/CHT3_EPCL_KEY_TYPE_L2_KEY_SHORT_E,     /* L2  egress , standard non-ip*/
/* 1*/CHT3_EPCL_KEY_TYPE_L2_L3_KEY_SHORT_E,  /* L2+IPv4+QOS , standard L2+Ipv4\6*/
/* 2*/CHT3_EPCL_KEY_TYPE_L3_L4_KEY_SHORT_E , /* IPv4 + L4 , standard ipv4+L4*/
/* 3*/CHT3_EPCL_KEY_TYPE_L2_L3_L4_KEY_LONG_E,/* L2+IPv4+L4 , extended , non-ip+IPv4*/
/* 4*/CHT3_EPCL_KEY_TYPE_L2_IPV6_KEY_LONG_E ,/* L2+IPv6 , Extended L2+Iipv6*/
/* 5*/CHT3_EPCL_KEY_TYPE_L4_IPV6_KEY_LONG_E ,/* L4+IPv6 , extended L4+ipv6*/
/* 6*/CHT3_EPCL_KEY_TYPE_L4_IPV4_NON_IP_KEY_LONG_E,/* IPv4 - Egress NonIP or IPv4/ARP */
/* 7*/CHT3_EPCL_KEY_TYPE_L4_IPV6_KEY_TRIPLE_E,/* RACL/VACL (72B) IPv6 - Egress */
      CHT3_EPCL_KEY_TYPE_LAST_E
}CHT3_EPCL_KEY_TYPE_ENT;

#define SNET_CHT3_EPCL_KEY_FORMAT(key) \
    (key == CHT3_EPCL_KEY_TYPE_L2_KEY_SHORT_E) ? CHT_PCL_KEY_REGULAR_E :              \
    (key == CHT3_EPCL_KEY_TYPE_L2_L3_KEY_SHORT_E) ? CHT_PCL_KEY_REGULAR_E :           \
    (key == CHT3_EPCL_KEY_TYPE_L3_L4_KEY_SHORT_E) ? CHT_PCL_KEY_REGULAR_E :           \
    (key == CHT3_EPCL_KEY_TYPE_L2_L3_L4_KEY_LONG_E) ? CHT_PCL_KEY_EXTENDED_E :        \
    (key == CHT3_EPCL_KEY_TYPE_L2_IPV6_KEY_LONG_E) ? CHT_PCL_KEY_EXTENDED_E :         \
    (key == CHT3_EPCL_KEY_TYPE_L4_IPV6_KEY_LONG_E) ? CHT_PCL_KEY_EXTENDED_E :         \
    (key == CHT3_EPCL_KEY_TYPE_L4_IPV4_NON_IP_KEY_LONG_E) ?  CHT_PCL_KEY_EXTENDED_E : \
                                                    CHT_PCL_KEY_TRIPLE_E

#define EPCL_SRC_PORT_MAC(descrPtr)                             \
            (descrPtr->origIsTrunk ? 0 : /*reserved*/           \
            (descrPtr->marvellTagged ?                          \
                descrPtr->origSrcEPortOrTrnk :                  \
                descrPtr->localDevSrcPort))

#define EPCL_TRG_PORT_MAC(dev,descrPtr,egressPort)                                                  \
    ((descrPtr->outGoingMtagCmd == SKERNEL_MTAG_CMD_TO_TRG_SNIFFER_E ||                              \
       descrPtr->outGoingMtagCmd == SKERNEL_MTAG_CMD_TO_CPU_E) ? descrPtr->srcTrgPhysicalPort :     \
       SKERNEL_IS_LION2_DEV(dev) ?  SMEM_CHT_GLOBAL_PORT_FROM_LOCAL_PORT_MAC(dev,egressPort)/*global port*/ :  \
        egressPort /*local port*/)


#define SNET_CHT3_PCKT_TUNNEL_START_PSGR(descrPtr)  \
    (descrPtr->tunnelStart) ?                         \
    (descrPtr->l3StartOffsetPtr + 4) : descrPtr->l3StartOffsetPtr


#define SNET_CHT3_EPCL_KEY_EGT_PKT_TYPE(_descrPtr, _fieldVal)     \
    switch (_descrPtr->outGoingMtagCmd)                           \
    {                                                             \
        case SKERNEL_MTAG_CMD_TO_CPU_E        :                   \
            _fieldVal = 0; break;                                 \
        case SKERNEL_MTAG_CMD_FROM_CPU_E:                         \
            _fieldVal = 1; break;                                 \
        case SKERNEL_MTAG_CMD_TO_TRG_SNIFFER_E:                   \
            _fieldVal = 2; break;                                 \
        case SKERNEL_MTAG_CMD_FORWARD_E:                          \
        default:                                                  \
            _fieldVal = 3; break; /* forward */                   \
    }

/******************************************************************************
 *  SNET_LION3_TCAM_SEGMENT_MODE_ENT
 *
 *  DESCRIPTION : Defines whether the 4 TCAM segments assigned to the PCL are
 *                regarded as one logical TCAM returning a single reply, two
 *                separate TCAM segments returning two replies, or four
 *                separate TCAM segments returning four replies.
 *
 *  SNET_LION3_TCAM_SEGMENT_MODE_1_TCAM_E - 1 TCAM, all TCAM segments are one logical TCAM.
 *  SNET_LION3_TCAM_SEGMENT_MODE_2_TCAM_DIV_2_2_E - 2 TCAM segments divided 2 and 2,
 *                  segmentation into 2 lists, logically distributed 1/2 and 1/2.
 *  SNET_LION3_TCAM_SEGMENT_MODE_2_TCAM_DIV_1_3_E - 2 TCAM segments divided 1 and 3,
 *                  segmentation into 2 lists, logically distributed 1/4 [first] and 3/4 [second].
 *  SNET_LION3_TCAM_SEGMENT_MODE_2_TCAM_DIV_3_1_E - 2 TCAM segments divided 3 and 1,
 *                  segmentation into 2 lists, logically distributed 3/4 [first] and 1/4 [second].
 *  SNET_LION3_TCAM_SEGMENT_MODE_4_TCAM_DIV_E - 4 TCAM segments, segmentation into 4 lists.
 *
********************************************************************************/
typedef enum{
    SNET_LION3_TCAM_SEGMENT_MODE_1_TCAM_E,
    SNET_LION3_TCAM_SEGMENT_MODE_2_TCAM_DIV_2_2_E,
    SNET_LION3_TCAM_SEGMENT_MODE_2_TCAM_DIV_1_3_E,
    SNET_LION3_TCAM_SEGMENT_MODE_2_TCAM_DIV_3_1_E,
    SNET_LION3_TCAM_SEGMENT_MODE_4_TCAM_DIV_E,
}SNET_LION3_TCAM_SEGMENT_MODE_ENT;

/*******************************************************************************
 * SNET_CHT3_IPCL_LOOKUP_CONFIG_STC
 *
 * DESCRIPTION :
 *
 *  Ingress PCL configuration Table<n> (0<=n<4224))
 *
 *  lookUpEn        - enables the access to the policy TCAM table.
 *  keySize         - standard/extended/triple (24/48/72 bytes).  (for sip5 also UDBs 10..60)
 *  pclID           - PCL-ID for this lookup.
 *  pclNonIpTypeCfg -
 *  pclIpV4ArpTypeCfg -
 *  pclIpV6TypeCfg  - Key type used for different packet type
 *  ingrlookUpKey   - ingress/egress PCL key expected
 *  pclPortListModeEn  - PCL Port List mode enabled - for xCat2 device
 *                       (always GT_FALSE if not supported by device)
 *  udbKeyBitmapEnable - SIP5 : Enables replacing 32 UDBs in a UDB key (minimal key size of 50B) with a
                                bitmap vector of 256 bits that has only one bit set, representing the value of UDB49.
                                That is: UDB Bitmap = 2^UDB49
 *  tcamSegmentMode - SIP5 : TCAM segments assignment to PCL.
 ******************************************************************************/
typedef struct {
    GT_BIT                              lookUpEn;
    CHT_PCL_KEY_FORMAT_ENT              keySize;
    GT_U32                              pclID;
    GT_U32                              pclNonIpTypeCfg;
    GT_U32                              pclIpV4ArpTypeCfg;
    GT_U32                              pclIpV6TypeCfg;
    GT_BOOL                             ingrlookUpKey;
    GT_BOOL                             pclPortListModeEn;

    GT_BIT                              udbKeyBitmapEnable;
    SNET_LION3_TCAM_SEGMENT_MODE_ENT    tcamSegmentMode;

}SNET_CHT3_PCL_LOOKUP_CONFIG_STC;

/********************************************************************************
 *  CHT3_PCL_KEY_FIELDS_ID_ENT
 *
 *  DESCRIPTION : Enum of the ingress PCL TCAM fields
 *
********************************************************************************/
typedef enum{
    /* Standard (24B) L2 - Ingress */
    CHT3_PCL_KEY_FIELDS_ID_PCL_ID_E,                            /* Bit(s) 0-9 */
    CHT3_PCL_KEY_FIELDS_ID_MAC_TO_ME_E,                         /* Bit(s) 10 */
    CHT3_PCL_KEY_FIELDS_ID_SRC_PORT_E,                          /* Bit(s) 11-16 */
    CHT3_PCL_KEY_FIELDS_ID_IS_TAGGED_E,                         /* Bit(s) 17 */
    CHT3_PCL_KEY_FIELDS_ID_VID_E,                               /* Bit(s) 18-29 */
    CHT3_PCL_KEY_FIELDS_ID_UP_E,                                /* Bit(s) 30-32 */
    CHT3_PCL_KEY_FIELDS_ID_QOS_PROFILE_E,                       /* Bit(s) 33-39 */
    CHT3_PCL_KEY_FIELDS_ID_IS_IPV4_E,                           /* Bit(s) 40 */
    CHT3_PCL_KEY_FIELDS_ID_IS_IP_E,                             /* Bit(s) 41 */
    CHT3_PCL_KEY_FIELDS_ID_RESERVED_42_E,                       /* Bit(s) 42 */
    CHT3_PCL_KEY_FIELDS_ID_ETHER_TYPE_DSAP_SSAP_E,              /* Bit(s) 43-58 */
    CHT3_PCL_KEY_FIELDS_ID_UDB2_66_59_SHORT_E,                  /* Bit(s) 59-66 */
    CHT3_PCL_KEY_FIELDS_ID_RESERVED_72_67_E,                    /* Bit(s) 67-72 */
    CHT3_PCL_KEY_FIELDS_ID_IS_ARP_E,                            /* Bit(s) 73 */
    CHT3_PCL_KEY_FIELDS_ID_RESERVED_74_E,                       /* Bit(s) 74 */
    CHT3_PCL_KEY_FIELDS_ID_UDB0_82_75_SHORT_E,                  /* Bit(s) 75-82 */
    CHT3_PCL_KEY_FIELDS_ID_UDB1_90_83_SHORT_E,                  /* Bit(s) 83-90 */
    CHT3_PCL_KEY_FIELDS_ID_ENCAP_TYPE_SHORT_E,                  /* Bit(s) 91 */
    CHT3_PCL_KEY_FIELDS_ID_MAC_SA_SHORT_E,                      /* Bit(s) 92-139 */
    CHT3_PCL_KEY_FIELDS_ID_MAC_DA_SHORT_E,                      /* Bit(s) 140-187 */
    CHT3_PCL_KEY_FIELDS_ID_USER_DEFINED_VALID_SHORT_E,          /* Bit(s) 188 */
    CHT3_PCL_KEY_FIELDS_ID_IS_L2_VALID_SHORT_E,                 /* Bit(s) 189 */
    CHT3_PCL_KEY_FIELDS_ID_RESERVED_190_191_E,                  /* Bit(s) 190-191 */
    /* Standard (24B) L2+IPv4/v6 QoS - Ingress */
    CHT3_PCL_KEY_FIELDS_ID_IP_PROTOCOL_E,                       /* Bit(s) 42-49 */
    CHT3_PCL_KEY_FIELDS_ID_DSCP_E,                              /* Bit(s) 50-55 */
    CHT3_PCL_KEY_FIELDS_ID_IS_L4_VALID_E,                       /* Bit(s) 56 */
    CHT3_PCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_E,                    /* Bit(s) 57-72 */
    CHT3_PCL_KEY_FIELDS_ID_IS_IPV6_EH_EXISTS_SHORT_E,           /* Bit(s) 74 */
    CHT3_PCL_KEY_FIELDS_ID_TCP_UDP_PORT_COMPS_SHORT_E,          /* Bit(s) 75-82 */
    CHT3_PCL_KEY_FIELDS_ID_LONG_IS_IPV6_EH_HOP_BY_HOP_E,        /* Bit(s) 91 */
    CHT3_PCL_KEY_FIELDS_ID_IPV4_FRAGMENTED_SHORT_E,             /* Bit(s) 190 */
    CHT3_PCL_KEY_FIELDS_ID_IP_HEADER_OK_SHORT_E,                /* Bit(s) 191 */
    /* Standard (24B) IPv4+L4 - Ingress */
    CHT3_PCL_KEY_FIELDS_ID_IS_BC_SHORT_E,                       /* Bit(s) 74 */
    CHT3_PCL_KEY_FIELDS_ID_UDB2_98_91_SHORT_E,                  /* Bit(s) 91-98 */
    CHT3_PCL_KEY_FIELDS_ID_SIP_31_0_E,                          /* Bit(s) 99-130 */
    CHT3_PCL_KEY_FIELDS_ID_DIP_31_0_SHORT_E,                    /* Bit(s) 131-162 */
    CHT3_PCL_KEY_FIELDS_ID_L4_BYTES_OFFSETS_0_1_13_SHORT_E,     /* Bit(s) 163-186 */
    CHT3_PCL_KEY_FIELDS_ID_RESERVED_187_E,                      /* Bit(s) 187 */
    /* IPCL1 IPv4 (24B) - Ingress */
    CHT3_PCL_KEY_FIELDS_PCKT_TOS_0_5_SHORT_E,                   /* Bit(s) 33-38 */
    CHT3_PCL_KEY_FIELDS_PCKT_TYPE_E,                            /* Bit(s) 39-41 */
    CHT3_PCL_KEY_FIELDS_PCKT_TOS_6_7_SHORT_E,                   /* Bit(s) 50-51 */
    CHT3_PCL_KEY_FIELDS_IP_FRAGMENTED_SHORT_E,                  /* Bit(s) 52-53 */
    CHT3_PCL_KEY_FIELDS_IP_HEADER_INFO_SHORT_E,                 /* Bit(s) 54-55 */
    CHT3_PCL_KEY_FIELDS_IP_PCKT_LEN_13_E,                       /* Bit(s) 73 */
    CHT3_PCL_KEY_FIELDS_IP_PCKT_LEN_0_12_E,                     /* Bit(s) 83-95 */
    CHT3_PCL_KEY_FIELDS_ID_TTL_0_2_E,                           /* Bit(s) 96-98 */
    CHT3_PCL_KEY_FIELDS_ID_TTL_3_7_E,                           /* Bit(s) 187-191 */
    /* Standard (24B) IPv6 DIP - Ingress */
    CHT3_PCL_KEY_FIELDS_ID_DIP_15_0_E,                          /* Bit(s) 57-72 */
    CHT3_PCL_KEY_FIELDS_ID_DIP_31_16_E,                         /* Bit(s) 75-90 */
    CHT3_PCL_KEY_FIELDS_IS_IPV6_HOP_BY_HOP_E,                   /* Bit(s) 91 */
    /*CHT3_PCL_KEY_FIELDS_ID_DIP_127_32_E,                         Bit(s) 92-187 */
    CHT3_PCL_KEY_FIELDS_ID_DIP_63_32_LONG_E,                     /* Bit(s)  92-123 */
    CHT3_PCL_KEY_FIELDS_ID_DIP_95_64_LONG_E,                     /* Bit(s) 124-155 */
    CHT3_PCL_KEY_FIELDS_ID_DIP_127_96_LONG_E,                    /* Bit(s) 156-187 */
    CHT3_PCL_KEY_FIELDS_ID_DIP_31_0_E,                          /* Bit(s) 244-275 */
    CHT3_PCL_KEY_FIELDS_ID_RESERVED_188_E,                      /* Bit(s) 188 */
    CHT3_PCL_KEY_FIELDS_ID_RESERVED_190_E,                      /* Bit(s) 190 */
    /* Extended (48B) L2+IPv4 + L4 - Ingress */
    CHT3_PCL_KEY_FIELDS_ID_L4_BYTES_OFFSETS_0_1_13_LONG_E,      /* Bit(s) 73-96 */
    CHT3_PCL_KEY_FIELDS_ID_SIP_31_0_LONG_E,                     /* Bit(s) 97-128 */
    CHT3_PCL_KEY_FIELDS_ID_DIP_31_0_LONG_E,                     /* Bit(s) 129-160 */
    CHT3_PCL_KEY_FIELDS_ID_ENCAP_TYPE_LONG_E,                   /* Bit(s) 161 */
    CHT3_PCL_KEY_FIELDS_ID_ETHER_TYPE_DSAP_SSAP_LONG_E,         /* Bit(s) 162-177 */
    CHT3_PCL_KEY_FIELDS_ID_LONG_IPV4_FRAGMENT_E,                /* Bit(s) 178 */
    CHT3_PCL_KEY_FIELDS_ID_MAC_SA_LONG_E,                       /* Bit(s) 235-282 */
    CHT3_PCL_KEY_FIELDS_ID_MAC_DA_LONG_E,                       /* Bit(s) 283-330 */
    /*CHT3_PCL_KEY_FIELDS_ID_UDB_KEY10_LONG_E,                    Bit(s) 331-378 */
    CHT3_PCL_KEY_FIELDS_ID_UDB_EXT_B012_E,                      /* Bit(s) 355-378 */
    CHT3_PCL_KEY_FIELDS_ID_UDB_EXT_B345_E,                      /* Bit(s) 331-354 */
    CHT3_PCL_KEY_FIELDS_ID_RESERVED_380_379_E,                  /* Bit(s) 379-380 */
    CHT3_PCL_KEY_FIELDS_ID_IS_L2_VALID_LONG_E,                  /* Bit(s) 381 */
    CHT3_PCL_KEY_FIELDS_ID_USER_DEFINED_VALID_LONG_E,           /* Bit(s) 382 */
    CHT3_PCL_KEY_FIELDS_ID_IP_HEADER_OK_LONG_E,                 /* Bit(s) 383 */
    /*  Extended (48B) L2+IPv6  - Ingress */
    /*CHT3_PCL_KEY_FIELDS_ID_SIP_127_32_LONG_E,                 Bit(s) 129-224 */
    CHT3_PCL_KEY_FIELDS_ID_SIP_63_32_LONG_E,                     /* Bit(s) 129-160 */
    CHT3_PCL_KEY_FIELDS_ID_SIP_95_64_LONG_E,                     /* Bit(s) 161-192 */
    CHT3_PCL_KEY_FIELDS_ID_SIP_127_96_LONG_E,                    /* Bit(s) 193-224 */
    CHT3_PCL_KEY_FIELDS_ID_IS_IPV6_EH_EXISTS_LONG_E,            /* Bit(s) 225 */
    CHT3_PCL_KEY_FIELDS_ID_IS_IPV6_EH_HOP_BY_HOP_LONG_E,        /* Bit(s) 226 */
    CHT3_PCL_KEY_FIELDS_ID_DIP_127_120_LONG_E,                  /* Bit(s) 227-234 */
    /* Extended (48B) L4+IPv6  - Ingress */
    /*CHT3_PCL_KEY_FIELDS_ID_DIP_119_32_LONG_E,                    Bit(s) 235-322 */
    CHT3_PCL_KEY_FIELDS_ID_DIP_63_32_LONG1_E,                   /* Bit(s) 235-266 */
    CHT3_PCL_KEY_FIELDS_ID_DIP_95_64_LONG1_E,                   /* Bit(s) 267-298 */
    CHT3_PCL_KEY_FIELDS_ID_DIP_119_96_LONG1_E,                  /* Bit(s) 299-322 */
    CHT3_PCL_KEY_FIELDS_ID_DIP_0_31_IPV6_LONG_E,                /* Bit(s) 323-354 */
    /* IPCL0 IPv4 - Ingress*/
    CHT3_PCL_KEY_FIELDS_ID_ENCAP_TYPE_IPCL0_LONG_E,             /* Bit(s) 39 */
    CHT3_PCL_KEY_FIELDS_ID_IS_BC_LONG_E,                        /* Bit(s) 40 */
    CHT3_PCL_KEY_FIELDS_ID_TCP_FLAGS_IPCL0_LONG_E,              /* Bit(s) 52-57 */
    CHT3_PCL_KEY_FIELDS_ID_ETHER_TYPE_DSAP_SSAP_IPCL0_LONG_E,   /* Bit(s) 58-73 */
    CHT3_PCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_IPCL0_E,              /* Bit(s) 74-89 */
    CHT3_PCL_KEY_FIELDS_IP_HEADER_INFO_IPCL0_LONG_E,            /* Bit(s) 90-91 */
    CHT3_PCL_KEY_FIELDS_ID_L4_BYTES_OFFSETS_13_1_0_IPCL0_LONG_E,/* Bit(s) 188-211 */
    CHT3_PCL_KEY_FIELDS_ID_SIP_31_0_IPCL0_LONG_E,               /* Bit(s) 212-243 */
    CHT3_PCL_KEY_FIELDS_ID_DIP_31_0_IPCL0_LONG_E,               /* Bit(s) 244-275 */
    CHT3_PCL_KEY_FIELDS_IP_FRAGMENTED_IPCL0_LONG_E,             /* Bit(s) 276-277 */
    CHT3_PCL_KEY_FIELDS_ID_IS_L2_VALID_IPCL0_LONG_E,            /* Bit(s) 278 */
    CHT3_PCL_KEY_FIELDS_ID_IS_ARP_IPCL0_LONG_E,                 /* Bit(s) 279 */
    CHT3_PCL_KEY_FIELDS_ID_USER_DEFINED_VALID_IPCL0_E,          /* Bit(s) 280 */
    CHT3_PCL_KEY_FIELDS_ID_UDB3_KEY11_E,                        /* Bit(s) 281-288 */
    CHT3_PCL_KEY_FIELDS_ID_UDB0_1_2_KEY12_E,                    /* Bit(s) 289-312 */
    CHT3_PCL_KEY_FIELDS_ID_RESERVED_319_313_E,                  /* Bit(s) 313-319 */
    CHT3_PCL_KEY_FIELDS_ID_UDB0_1_KEY1_E,                       /* Bit(s) 320-335 */
    CHT3_PCL_KEY_FIELDS_ID_UDB0_1_2_KEY2_E,                     /* Bit(s) 336-359 */
    CHT3_PCL_KEY_FIELDS_ID_UDB0_1_2_KEY11_E,                    /* Bit(s) 360-383 */
    /* IPCL0 IPv6 Key */
    CHT3_PCL_KEY_FIELDS_ID_IS_IPV6_EH_HOP_BY_HOP_IPCL0_TRIPLE_E,/* Bit(s) 39 */
    CHT3_PCL_KEY_FIELDS_ID_IS_IPV6_EH_EXISTS_IPCL0_TRIPLE_E,    /* Bit(s) 40 */
    CHT3_PCL_KEY_FIELDS_ID_IS_IS_ND_E,                          /* Bit(s) 90 */
    CHT3_PCL_KEY_FIELDS_ID_IS_BC_TRIPLE_E,                      /* Bit(s) 91 */
    /*CHT3_PCL_KEY_FIELDS_ID_SIP_127_32_E,                         Bit(s) 276-371 */
    CHT3_PCL_KEY_FIELDS_ID_SIP_63_32_EXT_1_E,                    /* Bit(s) 276-307 */
    CHT3_PCL_KEY_FIELDS_ID_SIP_95_64_EXT_1_E,                    /* Bit(s) 308-339 */
    CHT3_PCL_KEY_FIELDS_ID_SIP_127_96_EXT_1_E,                   /* Bit(s) 340-371 */
    /*CHT3_PCL_KEY_FIELDS_ID_DIP_127_32_TRIPLE_E,                  Bit(s) 372-467 */
    CHT3_PCL_KEY_FIELDS_ID_DIP_63_32_EXT_1_E,                    /* Bit(s) 372-403 */
    CHT3_PCL_KEY_FIELDS_ID_DIP_95_64_EXT_1_E,                    /* Bit(s) 404-435 */
    CHT3_PCL_KEY_FIELDS_ID_DIP_127_96_EXT_1_E,                   /* Bit(s) 436-467 */
    CHT3_PCL_KEY_FIELDS_ID_UDB3_475_468_E,                      /* Bit(s) 468-475 */
    CHT3_PCL_KEY_FIELDS_ID_ENCAP_TYPE_VALID_TRIPLE_E,           /* Bit(s) 476 */
    CHT3_PCL_KEY_FIELDS_ID_IS_L2_VALID_IPCL0_TRIPLE_E,          /* Bit(s) 477 */
    CHT3_PCL_KEY_FIELDS_ID_USER_DEFINED_VALID_IPCL0_TRIPLE_E,   /* Bit(s) 478 */
    CHT3_PCL_KEY_FIELDS_IP_HEADER_OK_IPCL0_TRIPLE_E,            /* Bit(s) 479 */
    CHT3_PCL_KEY_FIELDS_ID_UDB1_2_3_KEY10_E,                    /* Bit(s) 480-503 */
    CHT3_PCL_KEY_FIELDS_ID_UDB0_1_2_3_4_KEY11_E,                /* Bit(s) 504-543 */
    CHT3_PCL_KEY_FIELDS_ID_UDB0_1_2_KEY12_TRIPLE_E,             /* Bit(s) 544-567 */
    CHT3_PCL_KEY_FIELDS_ID_UDB18_E,                             /* Bit(s) 568-575 */
    /* IPCL0 IPv6 Key */
    CHT3_PCL_KEY_FIELDS_IP_PCKT_LEN_0_12_TRIPLE_E,              /* Bit(s) 52-64 */
    CHT3_PCL_KEY_FIELDS_ID_UDB2_KEY11_E,                        /* Bit(s) 65-72 */
    CHT3_PCL_KEY_FIELDS_ID_RESERVED_73_E,                       /* Bit(s) 73 */
    CHT3_PCL_KEY_FIELDS_ID_IS_L4_VALID_TRIPLE_E,                /* Bit(s) 91 */
    CHT3_PCL_KEY_FIELDS_ID_IPV6_HEADER_FLOW_LBL_E,              /* Bit(s) 92-111 */
    CHT3_PCL_KEY_FIELDS_ID_MPLS_OUTER_LBL_E,                    /* Bit(s) 112-131 */
    CHT3_PCL_KEY_FIELDS_ID_UDB0_1_KEY11_E,                      /* Bit(s) 132-147 */
    CHT3_PCL_KEY_FIELDS_ID_UDB0_1_2_KEY12_TRIPLE1_E,            /* Bit(s) 148-171 */
    CHT3_PCL_KEY_FIELDS_ID_IS_IPV6_LINK_LOCAL_E,                /* Bit(s) 172 */
    CHT3_PCL_KEY_FIELDS_ID_MPLS_OUTER_LBL_EXP_E,                /* Bit(s) 173-175 */
    CHT3_PCL_KEY_FIELDS_ID_MPLS_OUTER_LBL_S_BIT_E,              /* Bit(s) 176 */
    CHT3_PCL_KEY_FIELDS_IP_PCKT_LEN_15_13_E,                    /* Bit(s) 177-179 */
    CHT3_PCL_KEY_FIELDS_ID_TTL_E,                               /* Bit(s) 180-187 */
    CHT3_PCL_KEY_FIELDS_ID_RESERVED_468_E,                      /* Bit(s) 468 */
    CHT3_PCL_KEY_FIELDS_ID_IS_IPV6_MLD_E,                       /* Bit(s) 469 */
    CHT3_PCL_KEY_FIELDS_ID_IS_IPV6_EH_HOP_BY_HOP_IPCL1_TRIPLE_E,/* Bit(s) 470 */
    CHT3_PCL_KEY_FIELDS_ID_IS_IPV6_EH_EXISTS_IPCL1_TRIPLE_E,    /* Bit(s) 471 */
    CHT3_PCL_KEY_FIELDS_ID_RESERVED_477_472_E,                  /* Bit(s) 472-477 */
    CHT3_PCL_KEY_FIELDS_ID_UDB0_1_KEY0_E,                       /* Bit(s) 480-495 */
    CHT3_PCL_KEY_FIELDS_ID_UDB0_1_KEY1_TRIPLE_E,                /* Bit(s) 496-511 */
    CHT3_PCL_KEY_FIELDS_ID_UDB18_TRIPLE_E,                      /* Bit(s) 512-519 */
    CHT3_PCL_KEY_FIELDS_ID_UDB0_1_2_KEY2_TRIPLE_E,              /* Bit(s) 520-543 */
    CHT3_PCL_KEY_FIELDS_ID_UDB0_1_2_3_KEY10_E,                  /* Bit(s) 544-575 */
    CHT3_PCL_KEY_FIELDS_ID_LAST_E
}CHT3_PCL_KEY_FIELDS_ID_ENT;

/**
 *  Struct : SNET_CHT3_PCL_ACTION_STC
 *
 *  DESCRIPTION :
 *
 *      The policy engine maintains an 14K entries table ,
 *      corresponding to the 14K rules that may be defined in the
 *      TCAM lookup structure . The line index of the matching rule
 *      is used to index the policy action table and extract the
 *      action to perform.
 *
**/
typedef struct {
    SNET_CHT2_PCL_ACTION_STC baseAction;/* common with previous PP version */
    SNET_EXT_DST_INTERFACE_STC      pceExtEgressIf; /* Extended destination interface */
    GT_BOOL     actionStop;         /* Once this field is set, the Ingress PCL lookup is ended (the next lookups are not executed) */
    GT_U32      srcIdSetEn;         /* Assigning Source-ID for the Packet enable bit */
    GT_U32      srcId;              /* Assigning Source-ID for the Packet */
} SNET_CHT3_PCL_ACTION_STC;

#define SNET_SET_EXT_EGRESS_DEST_INTERFACE(action, interface) \
        (action)->pceExtEgressIf.interfaceInfo.egressInterface = (interface)

#define SNET_SET_EXT_LLT_DEST_INTERFACE(action, interface) \
        (action)->pceExtEgressIf.interfaceInfo.routerLLTIndex = (interface)

#define SNET_SET_EXT_VR_DEST_INTERFACE(action, interface) \
        (action)->pceExtEgressIf.interfaceInfo.virtualRouterId = (interface)

extern CHT_PCL_TCAM_COMMON_DATA_STC cht3GlobalPclData;
extern CHT_PCL_TCAM_COMMON_DATA_STC xcatGlobalPclData;
extern CHT_PCL_TCAM_COMMON_DATA_STC lionPortGroupGlobalPclData;
extern CHT_PCL_TCAM_COMMON_DATA_STC xcat2GlobalPclData;
extern CHT_PCL_TCAM_COMMON_DATA_STC lion2PortGroupGlobalPclData;

/* Convert value of flag userDefError (0 - no errors, != 0 - some UDB errors)
   to UDB Valid bit in the TCAM search key */
#define CH3_UDB_VALID_MAC(_userDefError) (((_userDefError) == 0)? 1 : 0)

#define CH3_IPV6_IS_NET_DISCOVERY_DIP_MAC(_dip) \
    ((_dip[0] == 0xFF020000)                    \
    && (_dip[1] == 0)                           \
    && (_dip[2] == 1)                           \
    && (_dip[3] == 0xFF000000))

/*******************************************************************************
* snetCht3IPcl
*
* DESCRIPTION:
*        Ingress Policy Engine processing for incoming frame  on Cheetah3
*        asic simulation.
*        PCL processing , Pcl assignment ,key forming , 2 Lookups ,
*        actions to descriptor processing
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - pointer to frame data buffer Id
* OUTPUTS:
*       descrPtr     - pointer to updated frame data buffer Id
*
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
extern GT_VOID snetCht3IPcl
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
);

/*******************************************************************************
* snetCht3PclGetKeyType
*
* DESCRIPTION:
*       Get the key type from the PCL configuration table.
*
* INPUTS:
*       descrPtr        - pointer to frame descriptor object.
*       curLookupCfgPtr - pointer to the current PCL configuration table.
*
* OUTPUTS:
*       keyTypePtr           - pointer to the relevant key type.
*
* RETURN:
*
*
*******************************************************************************/
extern GT_VOID snetCht3PclGetKeyType
(
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC   * descrPtr,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC      * curLookupCfgPtr,
    OUT GT_U32                              * keyTypePtr
);

/*******************************************************************************
* snetCht3IPclActionApply
*
* DESCRIPTION:
*        Apply the action entry from the policy action table.
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - pointer to frame descriptor.
*       actionInfoPtr   - pointer to the PCL action entry.
*       isFirst         - is first action apply
*
* OUTPUTS:
*
* RETURN:
*
* COMMENTS:
*       Policy Tcam Table : The policy TCAM holds 1024 lines
*       of 24 bytes or rule data used for ingress and egress PCL.
*******************************************************************************/
GT_VOID snetCht3IPclActionApply
(
    IN SKERNEL_DEVICE_OBJECT                * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC   * descrPtr  ,
    IN SNET_CHT3_PCL_ACTION_STC             * actionDataPtr,
    IN GT_BOOL                              isFirst
);

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
);

/*******************************************************************************
* lion3PclValidActionBmpGet
*
* DESCRIPTION:
*        Lion3 : Get bitmap indicating which of the quad lookup results is
*                valid for action resolution.
*
* INPUTS:
*       matchIndexArr   - (pointer to) PCL match indexes..
*       tcamSegmentMode - TCAM segments mode assignment to PCL.
*
* OUTPUTS:
*       bitmapPtr - (pointer to) bitmap indicating which of the quad lookup
*                   results is valid for action resolution.
* RETURN:
*
* COMMENTS:
*
*******************************************************************************/
GT_VOID lion3PclValidActionBmpGet
(
    IN    GT_U32                             *matchIndexArr,
    IN    SNET_LION3_TCAM_SEGMENT_MODE_ENT   tcamSegmentMode,
    OUT   GT_U32                             *bitmapPtr
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __snetCheetah3Pclh */


