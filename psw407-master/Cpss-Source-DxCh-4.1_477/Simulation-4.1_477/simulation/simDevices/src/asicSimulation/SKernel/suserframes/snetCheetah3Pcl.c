/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetCheetah3Pcl.c
*
* DESCRIPTION:
*
*       Ingress Policy Engine processing for incoming frame.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 51 $
*
*******************************************************************************/

#include <asicSimulation/SKernel/suserframes/snet.h>
#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SKernel/smem/smemCheetah.h>
#include <asicSimulation/SKernel/cheetahCommon/sregCheetah.h>
#include <asicSimulation/SKernel/suserframes/snet.h>
#include <asicSimulation/SKernel/suserframes/snetCheetah3Pcl.h>
#include <asicSimulation/SLog/simLog.h>

/* user defined bytes address for short l2-l3 pcl search key*/
#define SMEM_CHT3_KEY0_USERDEFINED_BYTES_SHORT_L2_L3_KEY_REG        (0x0B800038)

#define SMEM_CHT3_KEY3_USERDEFINED_BYTES_KEY_REG                    (0x0B800034)

/* user defined bytes address for long l2-l3-l4 pcl search key*/
#define SMEM_CHT3_KEY5_0_USERDEFINED_BYTES_LONG_L3_L4_KEY_REG       (0x0B800024)

/* user defined bytes address for long l2-l3-l4 pcl search key*/
#define SMEM_CHT3_KEY5_1_USERDEFINED_BYTES_LONG_L3_L4_KEY_REG       (0x0B800020)

/* user defined bytes address for standard l3-l4 pcl search key*/
#define SMEM_CHT3_KEY5_USERDEFINED_BYTES_SHORT_L3_L4_KEY_REG        (0x0B80003C)

/* user defined bytes address for extended l2-ipv6 pcl search key*/
#define SMEM_CHT3_KEY1_0_USERDEFINED_BYTES_LONG_L2_IPv6_KEY_REG     (0x0B800028)

/* user defined bytes address for extended l2-ipv6 pcl search key*/
#define SMEM_CHT3_KEY1_1_USERDEFINED_BYTES_LONG_L2_IPv6_KEY_REG     (0x0B80002C)

/* user defined bytes address for extended l3-ipv6 pcl search key*/
#define SMEM_CHT3_KEY5_USERDEFINED_BYTES_LONG_L4_IPv6_KEY_REG       (0x0B800030)

/* Toggle PCL_ID LSB to differentiate between lookup_0_0 and lookup_0_1 */
#define SNET_CHT3_TOGGLE_PCL_LSB(pcl_key_ptr)  (pcl_key_ptr->key.regular[0]^=0x01)


/* macro to return result of which bits caused the 'NO MATCH' */
#define X_Y_K_FIND_NO_MATCH(x,y,k,mask)  \
    ((~(((~x) & (~k)) | ((~y) & (k)))) & mask )


static GT_BOOL snetCht3DataMaskKeyCompare
(
    IN GT_U8 * dataPtr,
    IN GT_U8 * maskPtr,
    IN GT_U8 * keyPtr,
    IN GT_U32 compBits,

    IN GT_U32   indexInTcam,    /* for log info */
    IN GT_U32   bankIndex,      /* for log info */
    IN GT_U32   wordIndex      /* for log info */
);

CHT_PCL_TCAM_COMMON_DATA_STC cht3GlobalPclData =
           {CHT3_PCL_TCAM_BANK_NUMBERS,
            CHT3_PCL_TCAM_BANK_ENTRIES,     CHT3_PCL_TCAM_BANK_ENTRIES_STEP,
            CHT3_PCL_TCAM_BANK_ENTRY_WORDS, CHT3_PCL_TCAM_BANK_ENTRY_WORDS_STEP,
            CHT3_PCL_TCAM_BANK_DATA_BITS,   CHT3_PCL_TCAM_BANK_CTRL_BITS,
            snetCht3DataMaskKeyCompare};

CHT_PCL_TCAM_COMMON_DATA_STC xcatGlobalPclData =
           {CHT3_PCL_TCAM_BANK_NUMBERS,
            XCAT_PCL_TCAM_BANK_ENTRIES,     CHT3_PCL_TCAM_BANK_ENTRIES_STEP,
            CHT3_PCL_TCAM_BANK_ENTRY_WORDS, CHT3_PCL_TCAM_BANK_ENTRY_WORDS_STEP,
            CHT3_PCL_TCAM_BANK_DATA_BITS,   CHT3_PCL_TCAM_BANK_CTRL_BITS,
            snetCht3DataMaskKeyCompare};

CHT_PCL_TCAM_COMMON_DATA_STC lionPortGroupGlobalPclData =
           {CHT3_PCL_TCAM_BANK_NUMBERS,
            XCAT_PCL_TCAM_BANK_ENTRIES,     CHT3_PCL_TCAM_BANK_ENTRIES_STEP,
            CHT3_PCL_TCAM_BANK_ENTRY_WORDS, CHT3_PCL_TCAM_BANK_ENTRY_WORDS_STEP,
            CHT3_PCL_TCAM_BANK_DATA_BITS,   CHT3_PCL_TCAM_BANK_CTRL_BITS,
            snetCht3DataMaskKeyCompare};

CHT_PCL_TCAM_COMMON_DATA_STC xcat2GlobalPclData =
           {CHT3_PCL_TCAM_BANK_NUMBERS,
            XCAT2_PCL_TCAM_BANK_ENTRIES,    CHT3_PCL_TCAM_BANK_ENTRIES_STEP,
            CHT3_PCL_TCAM_BANK_ENTRY_WORDS, CHT3_PCL_TCAM_BANK_ENTRY_WORDS_STEP,
            XCAT2_PCL_TCAM_BANK_DATA_BITS,  XCAT2_PCL_TCAM_BANK_CTRL_BITS,
            snetCht3DataMaskKeyCompare};

CHT_PCL_TCAM_COMMON_DATA_STC lion2PortGroupGlobalPclData =
           {CHT3_PCL_TCAM_BANK_NUMBERS,
            LION2_PCL_TCAM_BANK_ENTRIES,    CHT3_PCL_TCAM_BANK_ENTRIES_STEP,
            CHT3_PCL_TCAM_BANK_ENTRY_WORDS, CHT3_PCL_TCAM_BANK_ENTRY_WORDS_STEP,
            XCAT2_PCL_TCAM_BANK_DATA_BITS,  XCAT2_PCL_TCAM_BANK_CTRL_BITS,
            snetCht3DataMaskKeyCompare};

#define CHT3_UNDEFINED_FIELD            0x55555555

#define SNET_CHT3_PCL_NON_IP_KEY_TYPE(lookup_key) \
    ((lookup_key)->ingrlookUpKey)   ?            \
       (((lookup_key)->pclNonIpTypeCfg == 0) ? CHT3_PCL_KEY_TYPE_L2_KEY_SHORT_E :            \
        ((lookup_key)->pclNonIpTypeCfg == 2) ? CHT3_PCL_KEY_TYPE_L2_L3_L4_KEY_LONG_E :       \
        ((lookup_key)->pclNonIpTypeCfg == 5) ? CHT3_PCL_KEY_TYPE_IPCL0_IPV4_KEY_LONG_E :    \
        ((lookup_key)->pclNonIpTypeCfg == 6) ? CHT3_PCL_KEY_TYPE_IPCL0_IPV6_KEY_TRIPLE_E :   \
                      CHT3_PCL_KEY_TYPE_LAST_E)                      \
                                    : \
       (((lookup_key)->pclNonIpTypeCfg == 0) ? CHT3_EPCL_KEY_TYPE_L2_KEY_SHORT_E :           \
        ((lookup_key)->pclNonIpTypeCfg == 1) ? CHT3_EPCL_KEY_TYPE_L2_L3_L4_KEY_LONG_E :       \
        ((lookup_key)->pclNonIpTypeCfg == 2) ? CHT3_EPCL_KEY_TYPE_L4_IPV4_NON_IP_KEY_LONG_E : \
                      CHT3_EPCL_KEY_TYPE_LAST_E)

#define SNET_CHT3_PCL_IPV4_ARP_KEY_TYPE(lookup_key)\
    ((lookup_key)->ingrlookUpKey)   ?           \
       (((lookup_key)->pclIpV4ArpTypeCfg == 0) ? CHT3_PCL_KEY_TYPE_L2_L3_KEY_SHORT_E :         \
        ((lookup_key)->pclIpV4ArpTypeCfg == 1) ? CHT3_PCL_KEY_TYPE_L3_L4_KEY_SHORT_E :         \
        ((lookup_key)->pclIpV4ArpTypeCfg == 2) ? CHT3_PCL_KEY_TYPE_L2_L3_L4_KEY_LONG_E :       \
        ((lookup_key)->pclIpV4ArpTypeCfg == 5) ? CHT3_PCL_KEY_TYPE_IPCL0_IPV4_KEY_LONG_E :     \
        ((lookup_key)->pclIpV4ArpTypeCfg == 6) ? CHT3_PCL_KEY_TYPE_IPCL1_IPV4_KEY_SHORT_E :    \
                      CHT3_PCL_KEY_TYPE_LAST_E)                      \
                                    :           \
       (((lookup_key)->pclIpV4ArpTypeCfg == 0) ? CHT3_EPCL_KEY_TYPE_L2_L3_KEY_SHORT_E :        \
        ((lookup_key)->pclIpV4ArpTypeCfg == 1) ? CHT3_EPCL_KEY_TYPE_L3_L4_KEY_SHORT_E :        \
        ((lookup_key)->pclIpV4ArpTypeCfg == 2) ? CHT3_EPCL_KEY_TYPE_L2_L3_L4_KEY_LONG_E :      \
        ((lookup_key)->pclIpV4ArpTypeCfg == 3) ? CHT3_EPCL_KEY_TYPE_L4_IPV4_NON_IP_KEY_LONG_E : \
                      CHT3_PCL_KEY_TYPE_LAST_E)

#define SNET_CHT3_PCL_IPV6_KEY_TYPE(lookup_key)\
    ((lookup_key)->ingrlookUpKey)   ?           \
       (((lookup_key)->pclIpV6TypeCfg == 0) ? CHT3_PCL_KEY_TYPE_L2_L3_KEY_SHORT_E :         \
        ((lookup_key)->pclIpV6TypeCfg == 1) ? CHT3_PCL_KEY_TYPE_IPV6_DIP_KEY_SHORT_E  :     \
        ((lookup_key)->pclIpV6TypeCfg == 2) ? CHT3_PCL_KEY_TYPE_L2_IPV6_KEY_LONG_E :        \
        ((lookup_key)->pclIpV6TypeCfg == 3) ? CHT3_PCL_KEY_TYPE_L4_IPV6_KEY_LONG_E :        \
        ((lookup_key)->pclIpV6TypeCfg == 5) ? CHT3_PCL_KEY_TYPE_IPCL0_IPV6_KEY_TRIPLE_E :   \
        ((lookup_key)->pclIpV6TypeCfg == 6) ? CHT3_PCL_KEY_TYPE_IPCL1_IPV6_KEY_TRIPLE_E :   \
                      CHT3_PCL_KEY_TYPE_LAST_E)                                    \
                                    :           \
       (((lookup_key)->pclIpV6TypeCfg == 0) ? CHT3_EPCL_KEY_TYPE_L2_L3_KEY_SHORT_E :        \
        ((lookup_key)->pclIpV6TypeCfg == 1) ? CHT3_EPCL_KEY_TYPE_L4_IPV6_KEY_TRIPLE_E :     \
        ((lookup_key)->pclIpV6TypeCfg == 2) ? CHT3_EPCL_KEY_TYPE_L2_IPV6_KEY_LONG_E :       \
        ((lookup_key)->pclIpV6TypeCfg == 3) ? CHT3_EPCL_KEY_TYPE_L4_IPV6_KEY_LONG_E :       \
                      CHT3_EPCL_KEY_TYPE_LAST_E)

#define SNET_CHT3_PCL_KEY_FORMAT(key) \
    (key == CHT3_PCL_KEY_TYPE_L2_KEY_SHORT_E) ? CHT_PCL_KEY_REGULAR_E :        \
    (key == CHT3_PCL_KEY_TYPE_L2_L3_KEY_SHORT_E) ? CHT_PCL_KEY_REGULAR_E :     \
    (key == CHT3_PCL_KEY_TYPE_L3_L4_KEY_SHORT_E) ? CHT_PCL_KEY_REGULAR_E :     \
    (key == CHT3_PCL_KEY_TYPE_IPCL1_IPV4_KEY_SHORT_E) ? CHT_PCL_KEY_REGULAR_E :\
    (key == CHT3_PCL_KEY_TYPE_IPV6_DIP_KEY_SHORT_E) ? CHT_PCL_KEY_REGULAR_E :  \
    (key == CHT3_PCL_KEY_TYPE_L2_L3_L4_KEY_LONG_E) ? CHT_PCL_KEY_EXTENDED_E :  \
    (key == CHT3_PCL_KEY_TYPE_L2_IPV6_KEY_LONG_E) ? CHT_PCL_KEY_EXTENDED_E :   \
    (key == CHT3_PCL_KEY_TYPE_L4_IPV6_KEY_LONG_E) ? CHT_PCL_KEY_EXTENDED_E :   \
    (key == CHT3_PCL_KEY_TYPE_IPCL0_IPV4_KEY_LONG_E) ?  CHT_PCL_KEY_EXTENDED_E : \
                                                    CHT_PCL_KEY_TRIPLE_E

#define SNET_CHT3_CNC_PCL_LOOKUP_GET(index)   \
    (((index) == 0) ? \
        SNET_CNC_CLIENT_IPCL_LOOKUP_0_ACTION_0_E : \
     ((index) == 1) ? \
        SNET_CNC_CLIENT_IPCL_LOOKUP_1_ACTION_0_E : \
        SNET_CNC_CLIENT_IPCL_LOOKUP_2_ACTION_0_E)

/* Key 10 UDB */
static CHT_PCL_USER_DEFINED_CONFIG_REG_STC userDefByteKey10Array[] =
{
    {SMEM_CHT3_KEY5_1_USERDEFINED_BYTES_LONG_L3_L4_KEY_REG,  0,  7},
    {SMEM_CHT3_KEY5_1_USERDEFINED_BYTES_LONG_L3_L4_KEY_REG, 11, 18},
    {SMEM_CHT3_KEY5_1_USERDEFINED_BYTES_LONG_L3_L4_KEY_REG, 22, 29},
    {SMEM_CHT3_KEY5_0_USERDEFINED_BYTES_LONG_L3_L4_KEY_REG,  0,  7},
    {SMEM_CHT3_KEY5_0_USERDEFINED_BYTES_LONG_L3_L4_KEY_REG, 11, 18},
    {SMEM_CHT3_KEY5_1_USERDEFINED_BYTES_LONG_L3_L4_KEY_REG, 22, 29}
};
/* Key 11 UDB */
static CHT_PCL_USER_DEFINED_CONFIG_REG_STC userDefByteKey11Array[] =
{
    {SMEM_CHT3_KEY1_0_USERDEFINED_BYTES_LONG_L2_IPv6_KEY_REG,  0,  7},
    {SMEM_CHT3_KEY1_0_USERDEFINED_BYTES_LONG_L2_IPv6_KEY_REG, 11, 18},
    {SMEM_CHT3_KEY1_0_USERDEFINED_BYTES_LONG_L2_IPv6_KEY_REG, 22, 29},
    {SMEM_CHT3_KEY1_1_USERDEFINED_BYTES_LONG_L2_IPv6_KEY_REG,  0,  7},
    {SMEM_CHT3_KEY1_1_USERDEFINED_BYTES_LONG_L2_IPv6_KEY_REG, 11, 18},
    {SMEM_CHT3_KEY1_1_USERDEFINED_BYTES_LONG_L2_IPv6_KEY_REG, 22, 29}
};
/* Key 12 UDB */
static CHT_PCL_USER_DEFINED_CONFIG_REG_STC userDefByteKey12Array[] =
{
    {SMEM_CHT3_KEY5_USERDEFINED_BYTES_LONG_L4_IPv6_KEY_REG,  0,  7},
    {SMEM_CHT3_KEY5_USERDEFINED_BYTES_LONG_L4_IPv6_KEY_REG, 11, 18},
    {SMEM_CHT3_KEY5_USERDEFINED_BYTES_LONG_L4_IPv6_KEY_REG, 22, 29}
};
/* Key 0 UDB */
static CHT_PCL_USER_DEFINED_CONFIG_REG_STC userDefByteKey0Array[] =
{
    {SMEM_CHT3_KEY3_USERDEFINED_BYTES_KEY_REG,  0,  7},
    {SMEM_CHT3_KEY3_USERDEFINED_BYTES_KEY_REG, 11, 18},
    {SMEM_CHT3_KEY3_USERDEFINED_BYTES_KEY_REG, 22, 29},
    {SMEM_CHT3_KEY0_USERDEFINED_BYTES_SHORT_L2_L3_KEY_REG,  0,  7}
};
/* Key 1 UDB */
static CHT_PCL_USER_DEFINED_CONFIG_REG_STC userDefByteKey1Array[] =
{
    {SMEM_CHT3_KEY0_USERDEFINED_BYTES_SHORT_L2_L3_KEY_REG, 11, 18},
    {SMEM_CHT3_KEY0_USERDEFINED_BYTES_SHORT_L2_L3_KEY_REG, 22, 29}
};
/* Key 2 UDB */
static CHT_PCL_USER_DEFINED_CONFIG_REG_STC userDefByteKey2Array[] =
{
    {SMEM_CHT3_KEY5_USERDEFINED_BYTES_SHORT_L3_L4_KEY_REG,  0,  7},
    {SMEM_CHT3_KEY5_USERDEFINED_BYTES_SHORT_L3_L4_KEY_REG, 11, 18},
    {SMEM_CHT3_KEY5_USERDEFINED_BYTES_SHORT_L3_L4_KEY_REG, 22, 29}
};

/* Array that holds the info about the tcam key fields */
static CHT_PCL_KEY_FIELDS_INFO_STC cht3PclKeyFieldsData[]=
{
    { 0, 9,      GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_PCL_ID_E                            " },
    { 10,10,     GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_MAC_TO_ME_E                         " },
    { 11,16,     GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_SRC_PORT_E                          " },
    { 17,17,     GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_IS_TAGGED_E                         " },
    { 18,29,     GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_VID_E                               " },
    { 30,32,     GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_UP_E                                " },
    { 33,39,     GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_QOS_PROFILE_E                       " },
    { 40,40,     GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_IS_IPV4_E                           " },
    { 41,41,     GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_IS_IP_E                             " },
    { 42,42,     GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_RESERVED_42_E                       " },
    { 43,58,     GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_ETHER_TYPE_DSAP_SSAP_E              " },
    { 59,66,     GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_UDB2_66_59_SHORT_E                  " },
    { 67,72,     GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_RESERVED_72_67_E                    " },
    { 73,73,     GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_IS_ARP_E                            " },
    { 74,74,     GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_RESERVED_74_E                       " },
    { 75,82,     GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_UDB0_82_75_SHORT_E                  " },
    { 83,90,     GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_UDB1_90_83_SHORT_E                  " },
    { 91,91,     GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_ENCAP_TYPE_SHORT_E                  " },
    { 92,139,    GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_MAC_SA_SHORT_E                      " },
    { 140,187,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_MAC_DA_SHORT_E                      " },
    { 188,188,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_USER_DEFINED_VALID_SHORT_E          " },
    { 189,189,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_IS_L2_VALID_SHORT_E                 " },
    { 190,191,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_RESERVED_190_191_E                  " },
                /* Standard (24B) L2+IPv4/v6 QoS - Ingress */
    { 42,49,     GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_IP_PROTOCOL_E                       " },
    { 50,55,     GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_DSCP_E                              " },
    { 56,56,     GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_IS_L4_VALID_E                       " },
    { 57,72,     GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_E                    " },
    { 74,74,     GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_IS_IPV6_EH_EXISTS_SHORT_E           " },
    { 75,82,     GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_TCP_UDP_PORT_COMPS_SHORT_E          " },
    { 91,91,     GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_LONG_IS_IPV6_EH_HOP_BY_HOP_E        " },
    { 190,190,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_IPV4_FRAGMENTED_SHORT_E             " },
    { 191,191,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_IP_HEADER_OK_SHORT_E                " },
                /* Standard (24B) IPv4+L4 - Ingress */
    { 74,74,     GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_IS_BC_SHORT_E                       " },
    { 91,98,     GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_UDB2_98_91_SHORT_E    " },
    { 99,130,    GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_SIP_31_0_E                          " },
    { 131,162,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_DIP_31_0_SHORT_E                    " },
    { 163,186,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_L4_BYTES_OFFSETS_0_1_13_SHORT_E     " },
    { 187,187,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_RESERVED_187_E                      " },
               /* IPCL1 IPv4 (24B) Key - Ingress */
    { 33,38,     GT_TRUE,       "CHT3_PCL_KEY_FIELDS_PCKT_TOS_0_5_SHORT_E                   " },
    { 39,41,     GT_TRUE,       "CHT3_PCL_KEY_FIELDS_PCKT_TYPE_E                            " },
    { 50,51,     GT_TRUE,       "CHT3_PCL_KEY_FIELDS_PCKT_TOS_6_7_SHORT_E                   " },
    { 52,53,     GT_TRUE,       "CHT3_PCL_KEY_FIELDS_IP_FRAGMENTED_SHORT_E                  " },
    { 54,55,     GT_TRUE,       "CHT3_PCL_KEY_FIELDS_IP_HEADER_INFO_SHORT_E                 " },
    { 73,73,     GT_TRUE,       "CHT3_PCL_KEY_FIELDS_IP_PCKT_LEN_13_E                       " },
    { 83,95,     GT_TRUE,       "CHT3_PCL_KEY_FIELDS_IP_PCKT_LEN_0_12_E                     " },
    { 96,98,     GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_TTL_0_2_E                           " },
    { 187,191,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_TTL_3_7_E                           " },
               /* Standard (24B) IPv6 DIP - Ingress */
    { 57,72,     GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_DIP_15_0_E                          " },
    { 75,90,     GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_DIP_31_16_E                         " },
    { 91,91,     GT_TRUE,       "CHT3_PCL_KEY_FIELDS_IS_IPV6_HOP_BY_HOP_E                   " },
    /*{ 92,187,    GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_DIP_127_32_E                     " },*/
    { 92,123,    GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_DIP_63_32_LONG_E                     " },
    { 124,155,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_DIP_95_64_LONG_E                     " },
    { 156,187,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_DIP_127_96_LONG_E                    " },
    { 244,275,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_DIP_31_0_E                          " },
    { 188,188,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_RESERVED_188_E                      " },
    { 190,190,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_RESERVED_190_E                      " },
                /* Extended (48B) L2+IPv4 + L4 - Ingress */
    { 73,96,     GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_L4_BYTES_OFFSETS_0_1_13_LONG_E      " },
    { 97,128,    GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_SIP_31_0_LONG_E                     " },
    { 129,160,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_DIP_31_0_LONG_E                     " },
    { 161,161,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_ENCAP_TYPE_LONG_E                   " },
    { 162,177,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_ETHER_TYPE_DSAP_SSAP_LONG_E         " },
    { 178,178,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_LONG_IPV4_FRAGMENT_E                " },
    { 235,282,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_MAC_SA_LONG_E                       " },
    { 283,330,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_MAC_DA_LONG_E                       " },
    /*{ 331,378,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_UDB_KEY10_LONG_E                 " },*/
    { 355,378,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_UDB_EXT_B012_E                      " },
    { 331,354,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_UDB_EXT_B345_E                      " },
    { 379,380,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_RESERVED_380_379_LONG_E             " },
    { 381,381,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_IS_L2_VALID_LONG_E                  " },
    { 382,382,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_USER_DEFINED_VALID_LONG_E           " },
    { 383,383,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_IP_HEADER_OK_LONG_E                 " },
                /*  Extended (48B) L2+IPv6  - Ingress */
    /*{ 129,224,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_SIP_127_32_LONG_E                " },*/
    { 129,160,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_SIP_63_32_LONG_E                     " },
    { 161,192,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_SIP_95_64_LONG_E                     " },
    { 193,224,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_SIP_127_96_LONG_E                    " },
    { 225,225,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_IS_IPV6_EH_EXISTS_LONG_E            " },
    { 226,226,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_IS_IPV6_EH_HOP_BY_HOP_LONG_E        " },
    { 227,234,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_DIP_127_120_LONG_E                  " },
                /* Extended (48B) L4+IPv6  - Ingress */
    /*{ 235,322,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_DIP_119_32_LONG_E                " },*/
    { 235,266,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_DIP_63_32_LONG1_E                   " },
    { 267,298,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_DIP_95_64_LONG1_E                   " },
    { 299,322,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_DIP_119_96_LONG1_E                  " },
    { 323,354,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_DIP_0_31_IPV6_LONG_E                " },
                /* IPCL0 IPv4 - Ingress*/
    { 39,39,     GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_ENCAP_TYPE_IPCL0_LONG_E             " },
    { 40,40,     GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_IS_BC_LONG_E                        " },
    { 52,57,     GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_TCP_FLAGS_IPCL0_LONG_E              " },
    { 58,73,     GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_ETHER_TYPE_DSAP_SSAP_IPCL0_LONG_E   " },
    { 74,89,     GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_IPCL0_E              " },
    { 90,91,     GT_TRUE,       "CHT3_PCL_KEY_FIELDS_IP_HEADER_INFO_IPCL0_LONG_E            " },
    { 188,211,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_L4_BYTES_OFFSETS_13_1_0_IPCL0_LONG_E" },
    { 212,243,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_SIP_31_0_IPCL0_LONG_E               " },
    { 244,275,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_DIP_31_0_IPCL0_LONG_E               " },
    { 276,277,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_IP_FRAGMENTED_IPCL0_LONG_E             " },
    { 278,278,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_IS_L2_VALID_IPCL0_LONG_E            " },
    { 279,279,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_IS_ARP_IPCL0_LONG_E                 " },
    { 280,280,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_USER_DEFINED_VALID_IPCL0_E          " },
    { 281,288,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_UDB3_KEY11_E                        " },
    { 289,312,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_UDB0_1_2_KEY12_E                    " },
    { 313,319,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_RESERVED_319_313_E                  " },
    { 320,335,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_UDB0_1_KEY1_E                       " },
    { 336,359,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_UDB0_1_2_KEY2_E                     " },
    { 360,383,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_UDB0_1_2_KEY11_E                    " },
                /* IPCL0 IPv6 Key */
    { 39,39,     GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_IS_IPV6_EH_HOP_BY_HOP_IPCL0_TRIPLE_E" },
    { 40,40,     GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_IS_IPV6_EH_EXISTS_IPCL0_TRIPLE_E    " },
    { 90,90,     GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_IS_IS_ND_E                          " },
    { 91,91,     GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_IS_BC_TRIPLE_E                      " },
    /*{ 276,371,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_SIP_127_32_E                     " },*/
    { 276,307,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_SIP_63_32_EXT_1_E                    " },
    { 308,339,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_SIP_95_64_EXT_1_E                    " },
    { 340,371,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_SIP_127_96_EXT_1_E                   " },
    /*{ 372,467,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_DIP_127_32_TRIPLE_E              " },*/
    { 372,403,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_DIP_63_32_EXT_1_E                    " },
    { 404,435,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_DIP_95_64_EXT_1_E                    " },
    { 436,467,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_DIP_127_96_EXT_1_E                   " },
    { 468,475,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_UDB3_475_468_E                      " },
    { 476,476,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_ENCAP_TYPE_VALID_TRIPLE_E           " },
    { 477,477,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_IS_L2_VALID_IPCL0_TRIPLE_E          " },
    { 478,478,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_USER_DEFINED_VALID_IPCL0_TRIPLE_E   " },
    { 479,479,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_IP_HEADER_OK_IPCL0_TRIPLE_E            " },
    { 480,503,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_UDB1_2_3_KEY10_E                    " },
    { 504,543,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_UDB0_1_2_3_4_KEY11_E                " },
    { 544,567,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_UDB0_1_2_KEY12_E                    " },
    { 568,575,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_UDB18_E                             " },
                /* IPCL0 IPv6 Key */
    { 52,64,     GT_TRUE,       "CHT3_PCL_KEY_FIELDS_IP_PCKT_LEN_0_12_TRIPLE_E              " },
    { 65,72,     GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_UDB2_KEY11_E                        " },
    { 73,73,     GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_RESERVED_73_E                       " },
    { 91,91,     GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_IS_L4_VALID_TRIPLE_E                " },
    { 92,111,    GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_IPV6_HEADER_FLOW_LBL_E              " },
    { 112,131,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_MPLS_OUTER_LBL_E                    " },
    { 132,147,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_UDB0_1_KEY11_E                      " },
    { 148,171,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_UDB0_1_2_KEY12_E                    " },
    { 172,172,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_IS_IPV6_LINK_LOCAL_E                " },
    { 173,175,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_MPLS_OUTER_LBL_EXP_E                " },
    { 176,176,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_MPLS_OUTER_LBL_S_BIT_E              " },
    { 177,179,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_IP_PCKT_LEN_15_13_E                    " },
    { 180,187,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_TTL_E                               " },
    { 468,468,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_RESERVED_468_E                      " },
    { 469,469,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_IS_IPV6_MLD_E                       " },
    { 470,470,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_IS_IPV6_EH_HOP_BY_HOP_IPCL1_TRIPLE_E" },
    { 471,471,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_IS_IPV6_EH_EXISTS_IPCL1_TRIPLE_E    " },
    { 472,477,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_RESERVED_477_472_E                  " },
    { 480,495,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_UDB0_1_KEY0_E                       " },
    { 496,511,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_UDB0_1_KEY1_E                       " },
    { 512,519,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_UDB18_TRIPLE_E                      " },
    { 520,543,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_UDB0_1_2_KEY2_E                     " },
    { 544,575,   GT_TRUE,       "CHT3_PCL_KEY_FIELDS_ID_UDB0_1_2_3_KEY10_E                  " }
};



/*******************************************************************************
*   snetCht3IPclKeyFieldBuildByValue
*
* DESCRIPTION:
*       Function insert data of field to the search key in specific place in PCL key
*
* INPUTS:
*       pclKeyPtr - pointer to current pcl key
*       fieldVal - data of field to insert to key
*       fieldId -- field id
*
* OUTPUTS:
*
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID snetCht3IPclKeyFieldBuildByValue
(
    SNET_CHT_POLICY_KEY_STC        * pclKeyPtr,
    GT_U32                          fieldVal,
    CHT3_PCL_KEY_FIELDS_ID_ENT      fieldId
)
{
    CHT_PCL_KEY_FIELDS_INFO_STC *fieldInfoPtr = &cht3PclKeyFieldsData[fieldId] ;

    snetChtPclSrvKeyFieldBuildByValue(pclKeyPtr, fieldVal,  fieldInfoPtr);

    return ;
}

/*******************************************************************************
*   snetChtIPclKeyFieldBuildByPointer
*
* DESCRIPTION:
*       Function insert data of field to the search key in specific place in key
*
* INPUTS:
*       pclKeyPtr   - pointer to current pcl key
*       fieldValPtr - pointer to data of field to insert to key
*       fieldId     - field id
*
* OUTPUTS:
*
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID snetCht3IPclKeyFieldBuildByPointer
(
    INOUT SNET_CHT_POLICY_KEY_STC *pclKeyPtr,
    IN GT_U8                      *fieldValPtr,
    IN CHT3_PCL_KEY_FIELDS_ID_ENT  fieldId
)
{
    CHT_PCL_KEY_FIELDS_INFO_STC *fieldInfoPtr = &cht3PclKeyFieldsData[fieldId] ;

    snetChtPclSrvKeyFieldBuildByPointer(pclKeyPtr, fieldValPtr, fieldInfoPtr);

    return;
}

/*******************************************************************************
*   snetCht3IPclKeyBuildL4IPv6ExtendedKey
*
* DESCRIPTION:
*        Build Layer4+IPv6 extended PCL search key.
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       descrPtr            - pointer to frame data buffer Id.
*       curLookUpCfgPtr     - pointer to the current PCL configuration entry.
*       pclExtraDataPtr     - pointer to extra data needed for the IPCL engine.
*
* OUTPUTS:
*       pclKeyPtr           - pointer to PCL key structure.
*
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID snetCht3IPclKeyBuildL4IPv6ExtendedKey
(
    IN SKERNEL_DEVICE_OBJECT                        * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC              * descrPtr,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC             * curLookUpCfgPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC                * pclExtraDataPtr,
    OUT SNET_CHT_POLICY_KEY_STC                     * pclKeyPtr
)
{
    DECLARE_FUNC_NAME(snetCht3IPclKeyBuildL4IPv6ExtendedKey);

    GT_U32      fieldVal;           /* valid bit value */
    GT_U32      resrvdVal;          /* reserved bit value */
    GT_U8       byteValue;          /* value of user defined byte from the packets or tcp/udp compare */
    GT_U32      userDefError = 0;   /* is there user defined byte error */
    GT_STATUS   rc;                 /* return code from function */
    GT_U8       tmpFieldVal[6];     /* byte array */
    GT_U32      byte;               /* byte array index */

    resrvdVal = 0;

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, curLookUpCfgPtr->pclID,
                            CHT3_PCL_KEY_FIELDS_ID_PCL_ID_E);/* [9:0] PCL-ID */
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->mac2me,
                            CHT3_PCL_KEY_FIELDS_ID_MAC_TO_ME_E);/*[10:10] mac2me */
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->localDevSrcPort ,
                            CHT3_PCL_KEY_FIELDS_ID_SRC_PORT_E);/* [16:11] src port */
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->origSrcTagged ,/* [17] is tagged */
                            CHT3_PCL_KEY_FIELDS_ID_IS_TAGGED_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->eVid ,/* [29:18] eVid */
                            CHT3_PCL_KEY_FIELDS_ID_VID_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->up  ,/* [32:30] UP */
                            CHT3_PCL_KEY_FIELDS_ID_UP_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->qos.qosProfile,/* [39:33] QOS profile */
                            CHT3_PCL_KEY_FIELDS_ID_QOS_PROFILE_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, (descrPtr->isIPv4==0 && descrPtr->isIp),
                            CHT3_PCL_KEY_FIELDS_ID_IS_IPV4_E); /* [40] is IPv4 */
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->isIp,/* [41] is IP */
                            CHT3_PCL_KEY_FIELDS_ID_IS_IP_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->ipProt,/* [49:42] IP Protocol*/
                            CHT3_PCL_KEY_FIELDS_ID_IP_PROTOCOL_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dscp,/* [55:50] dscp */
                            CHT3_PCL_KEY_FIELDS_ID_DSCP_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL4Valid, /* [56] isL4Valid*/
                            CHT3_PCL_KEY_FIELDS_ID_IS_L4_VALID_E);
    if (descrPtr->l4StartOffsetPtr != NULL)
    {
        if (descrPtr->ipv6Icmp || descrPtr->ipv4Icmp)
        {
            snetCht3IPclKeyFieldBuildByPointer(/* [72:57] L4 Byte Offsets 5:4 */
                pclKeyPtr,(GT_U8*)&(descrPtr->l4StartOffsetPtr[4]),
                CHT3_PCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_E);
        }
        else
        {
            snetCht3IPclKeyFieldBuildByPointer(/* [72:57] L4 Byte Offsets 3:2 */
                pclKeyPtr,(GT_U8*)&(descrPtr->l4StartOffsetPtr[2]),
                CHT3_PCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_E);
        }

        tmpFieldVal[0] = descrPtr->l4StartOffsetPtr[0];
        tmpFieldVal[1] = descrPtr->l4StartOffsetPtr[1];
        tmpFieldVal[2] = descrPtr->l4StartOffsetPtr[13];
        snetCht3IPclKeyFieldBuildByPointer(
            pclKeyPtr, (GT_U8*)&tmpFieldVal[0],/* [96:73] L4 Byte Offsets[1:0],[13] */
            CHT3_PCL_KEY_FIELDS_ID_L4_BYTES_OFFSETS_0_1_13_LONG_E);
    }
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[3], /* [128:97] SIP*/
                            CHT3_PCL_KEY_FIELDS_ID_SIP_31_0_LONG_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[2], /* [160:129] SIP*/
                            CHT3_PCL_KEY_FIELDS_ID_SIP_63_32_LONG_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[1], /* [192:161] SIP*/
                            CHT3_PCL_KEY_FIELDS_ID_SIP_95_64_LONG_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[0], /* [224:192] SIP*/
                            CHT3_PCL_KEY_FIELDS_ID_SIP_127_96_LONG_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpV6EhExists, /* [225] ipv6_eh_exist */
                            CHT3_PCL_KEY_FIELDS_ID_IS_IPV6_EH_EXISTS_LONG_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpV6EhHopByHop, /* [226] ipv6_eh_hopbyhop */
                            CHT3_PCL_KEY_FIELDS_ID_IS_IPV6_EH_HOP_BY_HOP_LONG_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[3], /* [354:323] DIP */
                            CHT3_PCL_KEY_FIELDS_ID_DIP_0_31_IPV6_LONG_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[2], /* [266:235] DIP */
                            CHT3_PCL_KEY_FIELDS_ID_DIP_63_32_LONG1_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[1], /* [298:267] DIP */
                            CHT3_PCL_KEY_FIELDS_ID_DIP_95_64_LONG1_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[0], /* [322:299] DIP */
                            CHT3_PCL_KEY_FIELDS_ID_DIP_119_96_LONG1_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, (descrPtr->dip[0] >> 24),  /* [234:227] DIP */
                            CHT3_PCL_KEY_FIELDS_ID_DIP_127_120_LONG_E);

    /* Key12 User Define Byte 2:0 in net order */
    __LOG(("Key12 User Define Byte 2:0 in net order"));
    for (byte = 0; byte < 2; byte++)
    {
        userDefError |= snetChtPclUdbKeyValueGet(devObjPtr,
                                                descrPtr,
                                                userDefByteKey12Array,
                                                byte,
                                                &tmpFieldVal[2 - byte]);
    }

    smemRegFldGet(devObjPtr,
                  userDefByteKey12Array[0].regOffsetAddress, 31, 1, &fieldVal);
    if (fieldVal)
    {/* TCP/UDP Range Comparators is enable */
        __LOG(("TCP/UDP Range Comparators is enable"));
        rc = snetChtIPclTcpUdpPortRangeCompareGet(devObjPtr, descrPtr, &byteValue);
        tmpFieldVal[0] = (rc != GT_OK) ? 0 : byteValue ;
    }
    else
    {   /* Key12 User Define Byte2  */
        __LOG(("Key12 User Define Byte2"));
        userDefError |= snetChtPclUdbKeyValueGet(devObjPtr,
                                                 descrPtr,
                                                 userDefByteKey12Array,
                                                 2,
                                                 &(tmpFieldVal[0]));
    }

    snetCht3IPclKeyFieldBuildByPointer(pclKeyPtr, tmpFieldVal,
                                       CHT3_PCL_KEY_FIELDS_ID_UDB_EXT_B012_E);

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, resrvdVal,/* Reserved [380:379] */
                            CHT3_PCL_KEY_FIELDS_ID_RESERVED_380_379_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr,pclExtraDataPtr->isL2Valid ,/* is L2 valid [381] */
                            CHT3_PCL_KEY_FIELDS_ID_IS_L2_VALID_LONG_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, CH3_UDB_VALID_MAC(userDefError),/* [382] UserDefineValid */
                            CHT3_PCL_KEY_FIELDS_ID_USER_DEFINED_VALID_LONG_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpHeaderOk ,
                            CHT3_PCL_KEY_FIELDS_ID_IP_HEADER_OK_LONG_E);/* [383] ip header ok */

    return  ;
}

/*******************************************************************************
*   snetCht3IPclKeyBuildL2IPv6ExtendedKey
*
* DESCRIPTION:
*        Build Layer2+IPv6 extended PCL search key.
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       descrPtr            - pointer to frame data buffer Id.
*       curLookUpCfgPtr     - pointer to the current PCL configuration entry.
*       pclExtraDataPtr     - pointer to extra data needed for the IPCL engine.
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
static GT_VOID snetCht3IPclKeyBuildL2IPv6ExtendedKey
(
    IN SKERNEL_DEVICE_OBJECT                        * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC              * descrPtr,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC             * curLookUpCfgPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC                * pclExtraDataPtr,
    OUT SNET_CHT_POLICY_KEY_STC                     * pclKeyPtr
)
{
    DECLARE_FUNC_NAME(snetCht3IPclKeyBuildL2IPv6ExtendedKey);

    GT_U32      fieldVal;           /* valid bit value */
    GT_U32      resrvdVal;          /* reserved bit value */
    GT_U8       byteValue;          /* value of user defined byte from the packets or tcp/udp compare */
    GT_U32      userDefError = 0;   /* is there user defined byte error */
    GT_STATUS   rc;                 /* return code from function */
    GT_U8       tmpFieldVal[6];
    GT_U32      byte;               /* byte array index */

    resrvdVal = 0;
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, curLookUpCfgPtr->pclID,
                            CHT3_PCL_KEY_FIELDS_ID_PCL_ID_E);/* [9:0] PCL-ID */
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->mac2me,
                            CHT3_PCL_KEY_FIELDS_ID_MAC_TO_ME_E);/*[10:10] mac2me */
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->localDevSrcPort ,
                            CHT3_PCL_KEY_FIELDS_ID_SRC_PORT_E);/* [16:11] src port */
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->origSrcTagged,/* [17] is tagged */
                            CHT3_PCL_KEY_FIELDS_ID_IS_TAGGED_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->eVid,/* [29:18] eVid */
                            CHT3_PCL_KEY_FIELDS_ID_VID_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->up,/* [32:30] UP */
                            CHT3_PCL_KEY_FIELDS_ID_UP_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->qos.qosProfile,/* [39:33] QOS profile */
                            CHT3_PCL_KEY_FIELDS_ID_QOS_PROFILE_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, (descrPtr->isIPv4==0 && descrPtr->isIp),
                            CHT3_PCL_KEY_FIELDS_ID_IS_IPV4_E);  /* [40] is IPv6 */
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->isIp, /* [41] is IP */
                            CHT3_PCL_KEY_FIELDS_ID_IS_IP_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->ipProt,/* [49:42] IP Protocol*/
                            CHT3_PCL_KEY_FIELDS_ID_IP_PROTOCOL_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dscp,/* [55:50] dscp */
                            CHT3_PCL_KEY_FIELDS_ID_DSCP_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL4Valid, /* [56] isL4Valid*/
                            CHT3_PCL_KEY_FIELDS_ID_IS_L4_VALID_E);
    if (descrPtr->l4StartOffsetPtr != NULL)
    {
        if (descrPtr->ipv6Icmp || descrPtr->ipv4Icmp)
        {
            snetCht3IPclKeyFieldBuildByPointer(/* [72:57] L4 Byte Offsets 5:4 */
                    pclKeyPtr,(GT_U8*)&(descrPtr->l4StartOffsetPtr[4]),
                    CHT3_PCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_E);
        }
        else
        {
            snetCht3IPclKeyFieldBuildByPointer(/* [72:57] L4 Byte Offsets 3:2 */
                    pclKeyPtr,(GT_U8*)&(descrPtr->l4StartOffsetPtr[2]),
                    CHT3_PCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_E);
        }
        tmpFieldVal[0] = descrPtr->l4StartOffsetPtr[0];
        tmpFieldVal[1] = descrPtr->l4StartOffsetPtr[1];
        tmpFieldVal[2] = descrPtr->l4StartOffsetPtr[13];
        snetCht3IPclKeyFieldBuildByPointer(
                pclKeyPtr, (GT_U8*)&tmpFieldVal[0],/* [73:96] L4 Byte Offsets[1:0],[13] */
                CHT3_PCL_KEY_FIELDS_ID_L4_BYTES_OFFSETS_0_1_13_LONG_E);
    }

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[3], /* [128:97] SIP*/
                            CHT3_PCL_KEY_FIELDS_ID_SIP_31_0_LONG_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[2], /* [160:129] SIP*/
                            CHT3_PCL_KEY_FIELDS_ID_SIP_63_32_LONG_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[1], /* [192:161] SIP*/
                            CHT3_PCL_KEY_FIELDS_ID_SIP_95_64_LONG_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[0], /* [224:192] SIP*/
                            CHT3_PCL_KEY_FIELDS_ID_SIP_127_96_LONG_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpV6EhExists, /* [225] ipv6_eh_exist */
                            CHT3_PCL_KEY_FIELDS_ID_IS_IPV6_EH_EXISTS_LONG_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpV6EhHopByHop, /* [226] ipv6_eh_hopbyhop */
                            CHT3_PCL_KEY_FIELDS_ID_IS_IPV6_EH_HOP_BY_HOP_LONG_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, (descrPtr->dip[0] >> 24),  /* [234:227] DIP */
                            CHT3_PCL_KEY_FIELDS_ID_DIP_127_120_LONG_E);
    snetCht3IPclKeyFieldBuildByPointer(pclKeyPtr, descrPtr->macSaPtr,/* [282:235] MAC SA*/
                            CHT3_PCL_KEY_FIELDS_ID_MAC_SA_LONG_E);
    snetCht3IPclKeyFieldBuildByPointer(pclKeyPtr, descrPtr->macDaPtr,/* [330:283] MAC DA*/
                            CHT3_PCL_KEY_FIELDS_ID_MAC_DA_LONG_E);

    /* User Define Byte 5-3 and 1-0 (network order) */
    __LOG(("User Define Byte 5-3 and 1-0 (network order)"));
    for (byte = 0; byte < 6; byte++)
    {
        /* maybe TCP/UDB port comparator */
        __LOG(("maybe TCP/UDB port comparator"));
        if (byte == 2)
            continue;

        userDefError |= snetChtPclUdbKeyValueGet(devObjPtr, descrPtr,
                                                 userDefByteKey11Array,
                                                 byte, &tmpFieldVal[5 - byte]);
    }

    /* check UDB 2 TCP/UDP Range Comparators enable bit */
    __LOG(("check UDB 2 TCP/UDP Range Comparators enable bit"));
    smemRegFldGet(devObjPtr,
                  userDefByteKey11Array[2].regOffsetAddress, 31, 1, &fieldVal);
    if (fieldVal)
    {/* TCP/UDP Range Comparators is enable */
        __LOG(("TCP/UDP Range Comparators is enable"));
        rc = snetChtIPclTcpUdpPortRangeCompareGet(devObjPtr,descrPtr,&byteValue);
        tmpFieldVal[5 - 2] = (rc != GT_OK) ? 0 : byteValue;
    }
    else
    {   /* User Define Byte2  */
        __LOG(("User Define Byte2"));
        userDefError |= snetChtPclUdbKeyValueGet(devObjPtr, descrPtr,
                                                 userDefByteKey11Array,
                                                 2, &tmpFieldVal[5 - 2]);

    }

    snetCht3IPclKeyFieldBuildByPointer(pclKeyPtr, &(tmpFieldVal[5 - 2]),
                                       CHT3_PCL_KEY_FIELDS_ID_UDB_EXT_B012_E);
    snetCht3IPclKeyFieldBuildByPointer(pclKeyPtr, &(tmpFieldVal[5 - 0]),
                                       CHT3_PCL_KEY_FIELDS_ID_UDB_EXT_B345_E);

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, resrvdVal,/* Reserved [379:380] */
                            CHT3_PCL_KEY_FIELDS_ID_RESERVED_380_379_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL2Valid,/* is L2 valid [381] */
                            CHT3_PCL_KEY_FIELDS_ID_IS_L2_VALID_LONG_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, CH3_UDB_VALID_MAC(userDefError),/* [382] UserDefineValid */
                            CHT3_PCL_KEY_FIELDS_ID_USER_DEFINED_VALID_LONG_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpHeaderOk ,
                            CHT3_PCL_KEY_FIELDS_ID_IP_HEADER_OK_LONG_E);/* [383] ip header  */

    return;
}

/*******************************************************************************
*   snetCht3IPclBuildL2L3L4ExtendedKey
*
* DESCRIPTION:
*        Build layer2+IPv4+layer4 extended PCL search key.
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       descrPtr            - pointer to frame data buffer Id.
*       curLookUpCfgPtr     - pointer to the current PCL configuration entry.
*       pclExtraDataPtr     - pointer to extra data needed for the IPCL engine.
*
* OUTPUTS:
*       pclKeyPtr           - pointer to PCL key structure.
*
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID snetCht3IPclBuildL2L3L4ExtendedKey
(
    IN SKERNEL_DEVICE_OBJECT                        * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC              * descrPtr,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC             * curLookUpCfgPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC                * pclExtraDataPtr,
    OUT SNET_CHT_POLICY_KEY_STC                     * pclKeyPtr
)
{
    DECLARE_FUNC_NAME(snetCht3IPclBuildL2L3L4ExtendedKey);

    GT_U32      resrvdVal;          /* reserved bit value */
    GT_U8       byteValue;          /* value of user defined byte from the packets or tcp/udp compare */
    GT_U32      userDefError = 0;   /* is there user defined byte error */
    GT_STATUS   rc;                 /* status from called function */
    GT_U32      fieldVal;           /* field value */
    GT_U8       tmpFieldVal[6];
    GT_U32      byte;               /* byte array index */

    resrvdVal = 0;
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, curLookUpCfgPtr->pclID,
                            CHT3_PCL_KEY_FIELDS_ID_PCL_ID_E);/* [9:0] PCL-ID */
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->mac2me,
                            CHT3_PCL_KEY_FIELDS_ID_MAC_TO_ME_E);/*[10:10] mac2me */
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->localDevSrcPort ,
                            CHT3_PCL_KEY_FIELDS_ID_SRC_PORT_E);/* [16:11] src port */
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->origSrcTagged ,/* [17] is tagged */
                            CHT3_PCL_KEY_FIELDS_ID_IS_TAGGED_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->eVid ,/* [29:18] eVid */
                            CHT3_PCL_KEY_FIELDS_ID_VID_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->up  ,/* [32:30] UP */
                            CHT3_PCL_KEY_FIELDS_ID_UP_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->qos.qosProfile,/* [39:33] QOS profile */
                            CHT3_PCL_KEY_FIELDS_ID_QOS_PROFILE_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, (descrPtr->isIPv4==0 && descrPtr->isIp),
                            CHT3_PCL_KEY_FIELDS_ID_IS_IPV4_E); /* [40] is IPv6 */
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->isIp,/* [41] is IP */
                            CHT3_PCL_KEY_FIELDS_ID_IS_IP_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->ipProt,/* [49:42] IP Protocol*/
                            CHT3_PCL_KEY_FIELDS_ID_IP_PROTOCOL_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dscp,/* [55:50] dscp */
                            CHT3_PCL_KEY_FIELDS_ID_DSCP_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL4Valid, /* [56] isL4Valid*/
                            CHT3_PCL_KEY_FIELDS_ID_IS_L4_VALID_E);
    if (descrPtr->l4StartOffsetPtr != NULL)
    {
        if (descrPtr->ipv6Icmp || descrPtr->ipv4Icmp)
        {
            snetCht3IPclKeyFieldBuildByPointer(/* [72:57] L4 Byte Offsets 5:4 */
                pclKeyPtr,(GT_U8*)&(descrPtr->l4StartOffsetPtr[4]),
                        CHT3_PCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_E);
        }
        else
        {
            snetCht3IPclKeyFieldBuildByPointer(/* [72:57] L4 Byte Offsets 3:2 */
                pclKeyPtr,(GT_U8*)&(descrPtr->l4StartOffsetPtr[2]),
                        CHT3_PCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_E);
        }
        tmpFieldVal[0] = descrPtr->l4StartOffsetPtr[0];
        tmpFieldVal[1] = descrPtr->l4StartOffsetPtr[1];
        tmpFieldVal[2] = descrPtr->l4StartOffsetPtr[13];
        snetCht3IPclKeyFieldBuildByPointer(
            pclKeyPtr,(GT_U8*)&tmpFieldVal[0],/* [96:73] L4 Byte Offsets[1:0],[13] */
                CHT3_PCL_KEY_FIELDS_ID_L4_BYTES_OFFSETS_0_1_13_LONG_E);
    }

    /* for IPv4 packets only the first byte sip[0] or dip[0] is relevant */
    __LOG(("for IPv4 packets only the first byte sip[0] or dip[0] is relevant"));
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[0], /* [128:97] SIP*/
                        CHT3_PCL_KEY_FIELDS_ID_SIP_31_0_LONG_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[0], /* [160:129] DIP*/
                        CHT3_PCL_KEY_FIELDS_ID_DIP_31_0_LONG_E);
    fieldVal = (descrPtr->l2Encaps == SKERNEL_ETHERNET_II_E ||
                       descrPtr->l2Encaps == SKERNEL_LLC_SNAP_E) ? 1 : 0;
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, fieldVal,/* [161] Encap Type */
                        CHT3_PCL_KEY_FIELDS_ID_ENCAP_TYPE_LONG_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->etherTypeOrSsapDsap,/* [177:162] */
                        CHT3_PCL_KEY_FIELDS_ID_ETHER_TYPE_DSAP_SSAP_LONG_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpv4Fragment,/* [178] */
                        CHT3_PCL_KEY_FIELDS_ID_LONG_IPV4_FRAGMENT_E);
    snetCht3IPclKeyFieldBuildByPointer(pclKeyPtr, descrPtr->macSaPtr,/* [235:282] MAC SA*/
                        CHT3_PCL_KEY_FIELDS_ID_MAC_SA_LONG_E);
    snetCht3IPclKeyFieldBuildByPointer(pclKeyPtr, descrPtr->macDaPtr,/* [330:283] MAC DA*/
                        CHT3_PCL_KEY_FIELDS_ID_MAC_DA_LONG_E);

    /* User Define Byte 5-3 and 1-0 (network order) */
    __LOG(("User Define Byte 5-3 and 1-0 (network order)"));
    for (byte = 0; byte < 6; byte++)
    {
        /* maybe TCP/UDB port comparator */
        __LOG(("maybe TCP/UDB port comparator"));
        if (byte == 2)
            continue;

        userDefError |= snetChtPclUdbKeyValueGet(devObjPtr, descrPtr,
                                                 userDefByteKey10Array,
                                                 byte, &tmpFieldVal[5 - byte]);
    }


    smemRegFldGet(devObjPtr,
                  userDefByteKey10Array[2].regOffsetAddress, 31, 1, &fieldVal);
    if (fieldVal)
    {/* TCP/UDP Range Comparators is enable */
        __LOG(("TCP/UDP Range Comparators is enable"));
        rc = snetChtIPclTcpUdpPortRangeCompareGet(devObjPtr,descrPtr,&byteValue);
        tmpFieldVal[5 - 2] = (rc != GT_OK) ? 0 : byteValue;
    }
    else
    {   /* User Define Byte2  */
        __LOG(("User Define Byte2"));
        userDefError |= snetChtPclUdbKeyValueGet(devObjPtr, descrPtr,
                                                 userDefByteKey10Array,
                                                 2, &tmpFieldVal[5 - 2]);

    }

    snetCht3IPclKeyFieldBuildByPointer(pclKeyPtr, &(tmpFieldVal[5 - 2]),
                                       CHT3_PCL_KEY_FIELDS_ID_UDB_EXT_B012_E);
    snetCht3IPclKeyFieldBuildByPointer(pclKeyPtr, &(tmpFieldVal[5 - 0]),
                                       CHT3_PCL_KEY_FIELDS_ID_UDB_EXT_B345_E);

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, resrvdVal,/* Reserved [380:379] */
                        CHT3_PCL_KEY_FIELDS_ID_RESERVED_380_379_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL2Valid,/* is L2 valid [381] */
                        CHT3_PCL_KEY_FIELDS_ID_IS_L2_VALID_LONG_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, CH3_UDB_VALID_MAC(userDefError),/* [382] UserDefineValid */
                        CHT3_PCL_KEY_FIELDS_ID_USER_DEFINED_VALID_LONG_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpHeaderOk ,
                        CHT3_PCL_KEY_FIELDS_ID_IP_HEADER_OK_LONG_E);/* [383] ip header ok */

    return ;
}

/*******************************************************************************
*   snetCht3IPclBuildL3L4StandardKey
*
* DESCRIPTION:
*       Build IPv4+Layer4 standard PCL search key.
*
* INPUTS:
*       devObjPtr            - pointer to device object.
*       descrPtr             - pointer to frame data buffer Id.
*       curLookUpCfgPtr - pointer to the current PCL configuration entry.
*       pclExtraDataPtr      - pointer to extra data needed for the IPCL engine.
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
static GT_VOID snetCht3IPclBuildL3L4StandardKey
(
    IN SKERNEL_DEVICE_OBJECT                        * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC              * descrPtr,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC             * curLookUpCfgPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC                * pclExtraDataPtr,
    OUT SNET_CHT_POLICY_KEY_STC                     * pclKeyPtr
)
{
    DECLARE_FUNC_NAME(snetCht3IPclBuildL3L4StandardKey);

    GT_U32                  resrvdVal;      /* reserved bit value */
    GT_U8                   byteValue;      /* value of user defined byte from the
                                               packets or tcp/udp compare */
    GT_U32                  userDefError = 0;
    GT_STATUS               rc;             /* status from called function */
    GT_U32                  fieldVal;       /* field value */
    GT_U8                   tmpFieldVal[4];


    resrvdVal = 0;

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, curLookUpCfgPtr->pclID,
                            CHT3_PCL_KEY_FIELDS_ID_PCL_ID_E);/* [9:0] PCL-ID */
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->mac2me,
                            CHT3_PCL_KEY_FIELDS_ID_MAC_TO_ME_E);/*[10:10] mac2me */
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->localDevSrcPort ,
                            CHT3_PCL_KEY_FIELDS_ID_SRC_PORT_E);/* [16:11] src port */
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->origSrcTagged ,/* [17] is tagged */
                            CHT3_PCL_KEY_FIELDS_ID_IS_TAGGED_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->eVid ,/* [29:18] eVid */
                            CHT3_PCL_KEY_FIELDS_ID_VID_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->up  ,/* [32:30] UP */
                            CHT3_PCL_KEY_FIELDS_ID_UP_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->qos.qosProfile,/* [39:33] QOS profile */
                            CHT3_PCL_KEY_FIELDS_ID_QOS_PROFILE_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->isIPv4 , /* [40] is IPv4 */
                            CHT3_PCL_KEY_FIELDS_ID_IS_IPV4_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->isIp,/* [41] is IP */
                            CHT3_PCL_KEY_FIELDS_ID_IS_IP_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->ipProt,/* [49:42] IP Protocol*/
                            CHT3_PCL_KEY_FIELDS_ID_IP_PROTOCOL_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dscp,/* [55:50] dscp */
                            CHT3_PCL_KEY_FIELDS_ID_DSCP_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL4Valid, /* [56] isL4Valid*/
                            CHT3_PCL_KEY_FIELDS_ID_IS_L4_VALID_E);
    if (descrPtr->l4StartOffsetPtr != NULL)
    {
        if (descrPtr->ipv6Icmp || descrPtr->ipv4Icmp)
        { /* [72:57] L4 Byte Offsets 5:4 */
            snetCht3IPclKeyFieldBuildByPointer(pclKeyPtr,
                    (GT_U8*)&(descrPtr->l4StartOffsetPtr[4]),
                    CHT3_PCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_E);
        }
        else
        {/* [72:57] L4 Byte Offsets 3:2 */
            snetCht3IPclKeyFieldBuildByPointer(pclKeyPtr,
                    (GT_U8*)&(descrPtr->l4StartOffsetPtr[2]),
                    CHT3_PCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_E);
        }
    }

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->arp ,/* [73] isArp */
                            CHT3_PCL_KEY_FIELDS_ID_IS_ARP_E);

    fieldVal = (descrPtr->macDaType == SKERNEL_BROADCAST_MAC_E ||
              descrPtr->macDaType == SKERNEL_BROADCAST_ARP_E) ? 1 : 0;

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, fieldVal ,/* [74] is bc */
                            CHT3_PCL_KEY_FIELDS_ID_IS_BC_SHORT_E);

    smemRegFldGet(devObjPtr,
                  userDefByteKey2Array[0].regOffsetAddress, 9, 1, &fieldVal);
    if (fieldVal)
    {/* TCP/UDP Range Comparators is enable */
        __LOG(("TCP/UDP Range Comparators is enable"));
        rc = snetChtIPclTcpUdpPortRangeCompareGet(devObjPtr, descrPtr, &byteValue);
        byteValue = (rc != GT_OK) ? 0 : byteValue ;
        snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [82:75] TCP/UDP comparator */
                            CHT3_PCL_KEY_FIELDS_ID_TCP_UDP_PORT_COMPS_SHORT_E);
    }
    else
    {   /* Key2 User Define Byte0  */
        __LOG(("Key2 User Define Byte0"));
        userDefError |= snetChtPclUdbKeyValueGet(devObjPtr, descrPtr,
                                                 userDefByteKey2Array,
                                                 0, &byteValue);

        snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [82:75] UserDefinedByte0 */
                            CHT3_PCL_KEY_FIELDS_ID_UDB0_82_75_SHORT_E);
    }

    /* Key2 User Define Byte1  */
    __LOG(("Key2 User Define Byte1"));
    userDefError |= snetChtPclUdbKeyValueGet(devObjPtr, descrPtr,
                                             userDefByteKey2Array,
                                             1, &byteValue);

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [83:90] UserDefinedByte1 */
                             CHT3_PCL_KEY_FIELDS_ID_UDB1_90_83_SHORT_E);

    /* Key2 User Define Byte2  */
    __LOG(("Key2 User Define Byte2"));
    userDefError |= snetChtPclUdbKeyValueGet(devObjPtr, descrPtr,
                                             userDefByteKey2Array,
                                             2, &byteValue);

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [98:91] UserDefinedByte2 */
                             CHT3_PCL_KEY_FIELDS_ID_UDB2_98_91_SHORT_E);
    /* for IPv4 packets only the first index sip[0] or dip[0] is relevant */
    __LOG(("for IPv4 packets only the first index sip[0] or dip[0] is relevant"));
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[0], /* [130:99] SIP*/
                             CHT3_PCL_KEY_FIELDS_ID_SIP_31_0_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[0], /* [162:131] DIP*/
                             CHT3_PCL_KEY_FIELDS_ID_DIP_31_0_SHORT_E);
    if (descrPtr->l4StartOffsetPtr != NULL)
    {
        tmpFieldVal[0] = descrPtr->l4StartOffsetPtr[0];
        tmpFieldVal[1] = descrPtr->l4StartOffsetPtr[1];
        tmpFieldVal[2] = descrPtr->l4StartOffsetPtr[13];
        snetCht3IPclKeyFieldBuildByPointer(
            pclKeyPtr,(GT_U8*)&tmpFieldVal[0], /* [186:163] L4 byte offsets */
                    CHT3_PCL_KEY_FIELDS_ID_L4_BYTES_OFFSETS_0_1_13_SHORT_E);
    }
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, resrvdVal, /* [187] reserved */
                            CHT3_PCL_KEY_FIELDS_ID_RESERVED_187_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, CH3_UDB_VALID_MAC(userDefError),/* [188] user defined valid */
                            CHT3_PCL_KEY_FIELDS_ID_USER_DEFINED_VALID_SHORT_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL2Valid ,/* [189] isL2Valid  */
                            CHT3_PCL_KEY_FIELDS_ID_IS_L2_VALID_SHORT_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpv4Fragment ,/* [190] ipv4 fragmented  */
                            CHT3_PCL_KEY_FIELDS_ID_IPV4_FRAGMENTED_SHORT_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpHeaderOk ,/* [191] ip header  */
                            CHT3_PCL_KEY_FIELDS_ID_IP_HEADER_OK_SHORT_E);

    return  ;
}

/*******************************************************************************
*   snetCht3IPclBuildL2L3StandardKey
*
* DESCRIPTION:
*        Build Layer2+IPv4\6+QoS standard PCL search key.
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       descrPtr            - pointer to frame data buffer Id.
*       curLookUpCfgPtr     - pointer to the current PCL configuration entry.
*       pclExtraDataPtr     - pointer to extra data needed for the IPCL engine.
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
static GT_VOID snetCht3IPclBuildL2L3StandardKey
(
    IN SKERNEL_DEVICE_OBJECT                        * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC              * descrPtr,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC             * curLookUpCfgPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC                * pclExtraDataPtr,
    OUT SNET_CHT_POLICY_KEY_STC                     * pclKeyPtr
)
{
    DECLARE_FUNC_NAME(snetCht3IPclBuildL2L3StandardKey);

    GT_U8       byteValue;          /* value of user defined byte from the
                                       packets or tcp/udp compare */
    GT_U32      userDefError = 0;   /* is there user defined byte error */
    GT_STATUS   rc;                 /* status from called function */
    GT_U32      fieldVal;           /* temp val */

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, curLookUpCfgPtr->pclID,
                            CHT3_PCL_KEY_FIELDS_ID_PCL_ID_E);/* [9:0] PCL-ID */
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->mac2me,
                            CHT3_PCL_KEY_FIELDS_ID_MAC_TO_ME_E);/*[10:10] mac2me */
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->localDevSrcPort ,
                            CHT3_PCL_KEY_FIELDS_ID_SRC_PORT_E);/* [16:11] src port */
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->origSrcTagged ,/* [17] is tagged */
                            CHT3_PCL_KEY_FIELDS_ID_IS_TAGGED_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->eVid ,/* [29:18] eVid */
                            CHT3_PCL_KEY_FIELDS_ID_VID_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->up  ,/* [32:30] UP */
                            CHT3_PCL_KEY_FIELDS_ID_UP_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->qos.qosProfile,/* [39:33] QOS profile */
                            CHT3_PCL_KEY_FIELDS_ID_QOS_PROFILE_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->isIPv4 , /* [40] is IPv4 */
                            CHT3_PCL_KEY_FIELDS_ID_IS_IPV4_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->isIp,/* [41] is IP */
                            CHT3_PCL_KEY_FIELDS_ID_IS_IP_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->ipProt,/* [49:42] IP Protocol*/
                            CHT3_PCL_KEY_FIELDS_ID_IP_PROTOCOL_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dscp,/* [55:50] dscp */
                            CHT3_PCL_KEY_FIELDS_ID_DSCP_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL4Valid, /* [56] isL4Valid*/
                            CHT3_PCL_KEY_FIELDS_ID_IS_L4_VALID_E);
    if (descrPtr->l4StartOffsetPtr != NULL)
    {
        if (descrPtr->ipv6Icmp || descrPtr->ipv4Icmp)
        {
            snetCht3IPclKeyFieldBuildByPointer(
                pclKeyPtr,(GT_U8*)&(descrPtr->l4StartOffsetPtr[4]),
                CHT3_PCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_E);/*     [72:57] L4 Byte Offsets 5:4 */
        }
        else
        {
            snetCht3IPclKeyFieldBuildByPointer(
                pclKeyPtr,(GT_U8*)&(descrPtr->l4StartOffsetPtr[2]),
                CHT3_PCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_E);/* [72:57] L4 Byte Offsets 3:2 */
        }
    }
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->arp ,/* [73] isArp */
                                     CHT3_PCL_KEY_FIELDS_ID_IS_ARP_E);

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpV6EhExists,/* [74] ipv6_EH_exist */
                                     CHT3_PCL_KEY_FIELDS_ID_IS_IPV6_EH_EXISTS_SHORT_E);

    smemRegFldGet(devObjPtr,
                  userDefByteKey1Array[0].regOffsetAddress, 20, 1, &fieldVal);
    if (fieldVal)
    {/* TCP/UDP Range Comparators is enable */
        __LOG(("TCP/UDP Range Comparators is enable"));
        rc = snetChtIPclTcpUdpPortRangeCompareGet(devObjPtr, descrPtr, &byteValue);
        byteValue = (rc != GT_OK) ? 0 : byteValue ;
        snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [82:75] TCP/UDP comparator */
                                         CHT3_PCL_KEY_FIELDS_ID_TCP_UDP_PORT_COMPS_SHORT_E);
    }
    else
    {   /* Get User Define Byte0 for Key 1 */
        __LOG(("Get User Define Byte0 for Key 1"));
        userDefError |= snetChtPclUdbKeyValueGet(devObjPtr, descrPtr,
                                                 userDefByteKey1Array,
                                                 0, &byteValue);

        snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [82:75] UserDefinedByte0 */
                                         CHT3_PCL_KEY_FIELDS_ID_UDB0_82_75_SHORT_E);
    }

    /* Get User Define Byte1 for Key 1 */
    __LOG(("Get User Define Byte1 for Key 1"));
    userDefError |= snetChtPclUdbKeyValueGet(devObjPtr, descrPtr,
                                            userDefByteKey1Array,
                                            1, &byteValue);

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [90:83] UserDefinedByte1 */
                            CHT3_PCL_KEY_FIELDS_ID_UDB1_90_83_SHORT_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpV6EhHopByHop,/* [91] is IPv6 hopbyhop */
                            CHT3_PCL_KEY_FIELDS_ID_LONG_IS_IPV6_EH_HOP_BY_HOP_E);
    snetCht3IPclKeyFieldBuildByPointer(pclKeyPtr, descrPtr->macSaPtr,/* [139:92] MAC SA*/
                            CHT3_PCL_KEY_FIELDS_ID_MAC_SA_SHORT_E);
    snetCht3IPclKeyFieldBuildByPointer(pclKeyPtr, descrPtr->macDaPtr,/* [187:140] MAC DA*/
                            CHT3_PCL_KEY_FIELDS_ID_MAC_DA_SHORT_E);

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, CH3_UDB_VALID_MAC(userDefError),              /* [188] user defined valid */
                        CHT3_PCL_KEY_FIELDS_ID_USER_DEFINED_VALID_SHORT_E);

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL2Valid ,    /* [189] isL2Valid  */
                            CHT3_PCL_KEY_FIELDS_ID_IS_L2_VALID_SHORT_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpv4Fragment ,/* [190] ipv4 fragmented  */
                            CHT3_PCL_KEY_FIELDS_ID_IPV4_FRAGMENTED_SHORT_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpHeaderOk , /* [191] ip header ok */
                            CHT3_PCL_KEY_FIELDS_ID_IP_HEADER_OK_SHORT_E);

    return ;
}

/*******************************************************************************
*   snetCht3IPclBuildL2StandardKey
*
* DESCRIPTION:
*        Build layer2 standard pcl search key.
*
* INPUTS:
*       devObjPtr            - pointer to device object.
*       descrPtr             - pointer to frame data buffer Id.
*       curLookUpCfgPtr      - pointer to the current PCL configuration entry.
*       pclExtraDataPtr      - pointer to extra data needed for the IPCL engine.
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
static GT_VOID snetCht3IPclBuildL2StandardKey
(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC * curLookUpCfgPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC    * pclExtraDataPtr,
    OUT SNET_CHT_POLICY_KEY_STC         * pclKeyPtr
)
{
    DECLARE_FUNC_NAME(snetCht3IPclBuildL2StandardKey);

    GT_U32 resrvdVal;           /* reserved bit value */
    GT_U32 fieldVal;            /* field value */
    GT_U8  byteValue;           /* value of user defined byte from the packets or tcp/udp compare */
    GT_U32 userDefError = 0;    /* is there user defined byte error */

    resrvdVal = 0;
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, curLookUpCfgPtr->pclID ,
                            CHT3_PCL_KEY_FIELDS_ID_PCL_ID_E);/*[9:0] PCL-ID */
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->mac2me,
                            CHT3_PCL_KEY_FIELDS_ID_MAC_TO_ME_E);/*[10:10] mac2me */
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->localDevSrcPort ,
                            CHT3_PCL_KEY_FIELDS_ID_SRC_PORT_E);/* [16:11] src port */
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->origSrcTagged,/* [17] is tagged */
                            CHT3_PCL_KEY_FIELDS_ID_IS_TAGGED_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->eVid ,/* [29:18] eVid */
                            CHT3_PCL_KEY_FIELDS_ID_VID_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->up  ,/* [32:30] UP */
                            CHT3_PCL_KEY_FIELDS_ID_UP_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->qos.qosProfile,/* [39:33] QOS profile */
                            CHT3_PCL_KEY_FIELDS_ID_QOS_PROFILE_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->isIPv4 , /* [40] is IPv4 */
                            CHT3_PCL_KEY_FIELDS_ID_IS_IPV4_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->isIp,/* [41] is IP */
                            CHT3_PCL_KEY_FIELDS_ID_IS_IP_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, resrvdVal,/*[42] reserved field*/
                            CHT3_PCL_KEY_FIELDS_ID_RESERVED_42_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->etherTypeOrSsapDsap,/* [58:43] */
                            CHT3_PCL_KEY_FIELDS_ID_ETHER_TYPE_DSAP_SSAP_E);

    /* Key0 User Defined Byte2*/
    __LOG(("Key0 User Defined Byte2"));
    if (descrPtr->cfm)
    {
        byteValue = descrPtr->udb[2];
    }

    else
    {
        userDefError |= snetChtPclUdbKeyValueGet(devObjPtr, descrPtr,
                                            userDefByteKey0Array,
                                            2, &byteValue);
    }

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [66:59] UserDefinedByte2 */
                            CHT3_PCL_KEY_FIELDS_ID_UDB2_66_59_SHORT_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, resrvdVal,/* [72:67] reserved  */
                            CHT3_PCL_KEY_FIELDS_ID_RESERVED_72_67_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->arp ,/* [73] isArp */
                            CHT3_PCL_KEY_FIELDS_ID_IS_ARP_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, resrvdVal ,/* [74] reserved  */
                            CHT3_PCL_KEY_FIELDS_ID_RESERVED_74_E);
    if (descrPtr->cfm)
    {
        byteValue = descrPtr->udb[0];
    }
    else
    {
        userDefError |= snetChtPclUdbKeyValueGet(devObjPtr, descrPtr,
                                            userDefByteKey0Array,
                                            0, &byteValue);
    }

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [82:75] UserDefinedByte0 */
                            CHT3_PCL_KEY_FIELDS_ID_UDB0_82_75_SHORT_E);

    if (descrPtr->cfm)
    {
        byteValue = descrPtr->udb[1];
    }
    else
    {

        userDefError |= snetChtPclUdbKeyValueGet(devObjPtr, descrPtr,
                                            userDefByteKey0Array,
                                             1, &byteValue);
    }

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [90:83] UserDefinedByte1 */
                            CHT3_PCL_KEY_FIELDS_ID_UDB1_90_83_SHORT_E);

    fieldVal = (descrPtr->l2Encaps == SKERNEL_ETHERNET_II_E ||
                       descrPtr->l2Encaps == SKERNEL_LLC_SNAP_E) ? 1 : 0;
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, fieldVal,/* [91] Encap Type */
                        CHT3_PCL_KEY_FIELDS_ID_ENCAP_TYPE_SHORT_E);

    snetCht3IPclKeyFieldBuildByPointer(pclKeyPtr, descrPtr->macSaPtr,/* [139:92] MAC SA*/
                            CHT3_PCL_KEY_FIELDS_ID_MAC_SA_SHORT_E);
    snetCht3IPclKeyFieldBuildByPointer(pclKeyPtr, descrPtr->macDaPtr,/* [187:140] MAC DA*/
                            CHT3_PCL_KEY_FIELDS_ID_MAC_DA_SHORT_E);

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, CH3_UDB_VALID_MAC(userDefError),
                        CHT3_PCL_KEY_FIELDS_ID_USER_DEFINED_VALID_SHORT_E);

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL2Valid ,/* [189] isL2Valid  */
                                    CHT3_PCL_KEY_FIELDS_ID_IS_L2_VALID_SHORT_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, resrvdVal ,/* [191:190] reserved  */
                                    CHT3_PCL_KEY_FIELDS_ID_RESERVED_190_191_E);

    return ;
}

/*******************************************************************************
*   snetCht3IPclKeyBuildIpcl0IPv4ExtendedKey
*
* DESCRIPTION:
*        Build IPCL0 IPv4 extended PCL search key.
*
* INPUTS:
*       devObjPtr            - pointer to device object.
*       descrPtr             - pointer to frame data buffer Id.
*       curLookUpCfgPtr - pointer to the current PCL configuration entry.
*       pclExtraDataPtr      - pointer to extra data needed for the IPCL engine.
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
static GT_VOID snetCht3IPclKeyBuildIpcl0IPv4ExtendedKey
(
    IN SKERNEL_DEVICE_OBJECT                        * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC              * descrPtr,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC             * curLookUpCfgPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC                * pclExtraDataPtr,
    OUT SNET_CHT_POLICY_KEY_STC                     * pclKeyPtr
)
{
    DECLARE_FUNC_NAME(snetCht3IPclKeyBuildIpcl0IPv4ExtendedKey);

    GT_U32      fieldVal;           /* valid bit value */
    GT_U32      resrvdVal;          /* reserved bit value */
    GT_U8       byteValue;          /* value of user defined byte from the packets or tcp/udp compare */
    GT_U32      userDefError = 0;   /* is there user defined byte error */
    GT_U8       tmpFieldVal[6];     /* byte array */
    GT_U32      byte;               /* byte array index */

    resrvdVal = 0;

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, curLookUpCfgPtr->pclID,
                            CHT3_PCL_KEY_FIELDS_ID_PCL_ID_E);/* [9:0] PCL-ID */
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->mac2me,
                            CHT3_PCL_KEY_FIELDS_ID_MAC_TO_ME_E);/*[10:10] mac2me */
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->localDevSrcPort ,
                            CHT3_PCL_KEY_FIELDS_ID_SRC_PORT_E);/* [16:11] src port */
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->origSrcTagged ,/* [17] is tagged */
                            CHT3_PCL_KEY_FIELDS_ID_IS_TAGGED_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->eVid ,/* [29:18] eVid */
                            CHT3_PCL_KEY_FIELDS_ID_VID_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->up  ,/* [32:30] UP */
                            CHT3_PCL_KEY_FIELDS_ID_UP_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, (descrPtr->l3StartOffsetPtr[1] & 0x3f),/* [38:33] TOS[5:0] */
                            CHT3_PCL_KEY_FIELDS_PCKT_TOS_0_5_SHORT_E);
    fieldVal = (descrPtr->l2Encaps == SKERNEL_ETHERNET_II_E ||
                       descrPtr->l2Encaps == SKERNEL_LLC_SNAP_E) ? 1 : 0;
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, fieldVal,     /* [39] L2 encap type */
                            CHT3_PCL_KEY_FIELDS_ID_ENCAP_TYPE_IPCL0_LONG_E);

    fieldVal = (descrPtr->macDaType == SKERNEL_BROADCAST_MAC_E ||
              descrPtr->macDaType == SKERNEL_BROADCAST_ARP_E) ? 1 : 0;

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, fieldVal,    /* [40] is bc */
                            CHT3_PCL_KEY_FIELDS_ID_IS_BC_LONG_E);

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->isIp,/* [41] is IP */
                            CHT3_PCL_KEY_FIELDS_ID_IS_IP_E);

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->ipProt,/* [49:42] IP Protocol*/
                            CHT3_PCL_KEY_FIELDS_ID_IP_PROTOCOL_E);

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, ((descrPtr->l3StartOffsetPtr[1] >> 6) & 0x3),/* [51:50] TOS[7:6] */
                            CHT3_PCL_KEY_FIELDS_PCKT_TOS_6_7_SHORT_E);


    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, CHT3_UNDEFINED_FIELD,/* [57:52] TOS[7:6] */
                            CHT3_PCL_KEY_FIELDS_ID_TCP_FLAGS_IPCL0_LONG_E);

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->etherTypeOrSsapDsap,/* [58:43] */
                            CHT3_PCL_KEY_FIELDS_ID_ETHER_TYPE_DSAP_SSAP_IPCL0_LONG_E);

    if (descrPtr->l4StartOffsetPtr != NULL)
    {
        if (descrPtr->ipv6Icmp || descrPtr->ipv4Icmp)
        {
            snetCht3IPclKeyFieldBuildByPointer(/* [89:74] L4 Byte Offsets 5:4 */
                        pclKeyPtr,(GT_U8*)&(descrPtr->l4StartOffsetPtr[4]),
                        CHT3_PCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_IPCL0_E);
        }
        else
        {
            snetCht3IPclKeyFieldBuildByPointer(/* [89:74] L4 Byte Offsets 3:2 */
                        pclKeyPtr,(GT_U8*)&(descrPtr->l4StartOffsetPtr[2]),
                        CHT3_PCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_IPCL0_E);
        }
    }

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->ipv4HeaderInfo,/* [91:90] IP header info */
                            CHT3_PCL_KEY_FIELDS_IP_HEADER_INFO_IPCL0_LONG_E);

    snetCht3IPclKeyFieldBuildByPointer(pclKeyPtr, descrPtr->macSaPtr,/* [139:92] MAC SA*/
                            CHT3_PCL_KEY_FIELDS_ID_MAC_SA_SHORT_E);

    snetCht3IPclKeyFieldBuildByPointer(pclKeyPtr, descrPtr->macDaPtr,/* [187:140] MAC DA*/
                            CHT3_PCL_KEY_FIELDS_ID_MAC_DA_SHORT_E);

    if (descrPtr->l4StartOffsetPtr != NULL)
    {
        tmpFieldVal[0] = descrPtr->l4StartOffsetPtr[0];
        tmpFieldVal[1] = descrPtr->l4StartOffsetPtr[1];
        tmpFieldVal[2] = descrPtr->l4StartOffsetPtr[13];
    }

    snetCht3IPclKeyFieldBuildByPointer(
        pclKeyPtr,(GT_U8*)&tmpFieldVal[0],/* [211:188] L4 Byte Offsets[1:0],[13] */
            CHT3_PCL_KEY_FIELDS_ID_L4_BYTES_OFFSETS_13_1_0_IPCL0_LONG_E);

    /* for IPv4 packets only the first index sip[0] or dip[0] is relevant */
    __LOG(("for IPv4 packets only the first index sip[0] or dip[0] is relevant"));
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[0], /* [243:212] SIP*/
                             CHT3_PCL_KEY_FIELDS_ID_SIP_31_0_IPCL0_LONG_E);

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[0], /* [275:244] DIP*/
                             CHT3_PCL_KEY_FIELDS_ID_DIP_31_0_IPCL0_LONG_E);

    if (pclExtraDataPtr->isIpv4Fragment == 0)
    {
        fieldVal = 0;
    }
    else if (pclExtraDataPtr->ipv4FragmentOffset == 0)
    {
        fieldVal = 1;
    }
    else
    {
        fieldVal = 2;
    }

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, fieldVal, /* [277:276] IP fragmented */
                            CHT3_PCL_KEY_FIELDS_IP_FRAGMENTED_IPCL0_LONG_E);

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL2Valid ,    /* [278] isL2Valid  */
                            CHT3_PCL_KEY_FIELDS_ID_IS_L2_VALID_IPCL0_LONG_E);

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->arp ,/* [279] isArp */
                            CHT3_PCL_KEY_FIELDS_ID_IS_ARP_IPCL0_LONG_E);

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, CH3_UDB_VALID_MAC(userDefError),/* [280] UserDefineValid */
                        CHT3_PCL_KEY_FIELDS_ID_USER_DEFINED_VALID_IPCL0_E);

    /* Key11 User Define Byte3  */
    __LOG(("Key11 User Define Byte3"));
    userDefError |= snetChtPclUdbKeyValueGet(devObjPtr, descrPtr,
                                             userDefByteKey11Array,
                                             3, &byteValue);

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [312-289] UserDefineByte3 Key11 */
                        CHT3_PCL_KEY_FIELDS_ID_UDB3_KEY11_E);

    /* Key12 User Define Byte0-1-2 */
    __LOG(("Key12 User Define Byte0-1-2"));
    for (byte = 0; byte < 3; byte++)
    {
        userDefError |= snetChtPclUdbKeyValueGet(devObjPtr, descrPtr,
                                                 userDefByteKey12Array,
                                                 byte, &tmpFieldVal[2-byte]);
    }

    snetCht3IPclKeyFieldBuildByPointer(pclKeyPtr, tmpFieldVal, /* [312-289] UserDefineByte0-1-2 Key12 */
                        CHT3_PCL_KEY_FIELDS_ID_UDB0_1_2_KEY12_E);

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, resrvdVal,/* [319-313] Reserved */
                        CHT3_PCL_KEY_FIELDS_ID_RESERVED_319_313_E);

    /* Key0 User Define Byte0-1 */
    __LOG(("Key0 User Define Byte0-1"));
    for (byte = 0; byte < 2; byte++)
    {
        userDefError |= snetChtPclUdbKeyValueGet(devObjPtr, descrPtr,
                                                 userDefByteKey1Array,
                                                 byte, &tmpFieldVal[1-byte]);
    }

    snetCht3IPclKeyFieldBuildByPointer(pclKeyPtr, tmpFieldVal, /* [312-289] UserDefineByte0-1 Key0 */
                        CHT3_PCL_KEY_FIELDS_ID_UDB0_1_KEY1_E);

    /* Key2 User Define Byte0-1-2 */
    __LOG(("Key2 User Define Byte0-1-2"));
    for (byte = 0; byte < 3; byte++)
    {
        userDefError |= snetChtPclUdbKeyValueGet(devObjPtr, descrPtr,
                                                 userDefByteKey2Array,
                                                 byte, &tmpFieldVal[2-byte]);
    }

    snetCht3IPclKeyFieldBuildByPointer(pclKeyPtr, tmpFieldVal, /* [359-336] UserDefineByte0-1-2 Key2 */
                        CHT3_PCL_KEY_FIELDS_ID_UDB0_1_2_KEY2_E);

    /* Key11 User Define Byte0-1-2 */
    __LOG(("Key11 User Define Byte0-1-2"));
    for (byte = 0; byte < 3; byte++)
    {
        userDefError |= snetChtPclUdbKeyValueGet(devObjPtr, descrPtr,
                                                userDefByteKey11Array,
                                                byte, &tmpFieldVal[2-byte]);
    }

    snetCht3IPclKeyFieldBuildByPointer(pclKeyPtr, tmpFieldVal, /* [383-360] UserDefineByte0-1-2 Key2 */
                        CHT3_PCL_KEY_FIELDS_ID_UDB0_1_2_KEY11_E);


    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, CH3_UDB_VALID_MAC(userDefError),/* [280] Reserved */
                        CHT3_PCL_KEY_FIELDS_ID_USER_DEFINED_VALID_IPCL0_E);

    return;
}

/*******************************************************************************
*   snetCht3IPclKeyBuildIpcl1IPv4ShortKey
*
* DESCRIPTION:
*        Build IPCL1 IPv4 standard PCL search key.
*
* INPUTS:
*       devObjPtr            - pointer to device object.
*       descrPtr             - pointer to frame data buffer Id.
*       curLookUpCfgPtr - pointer to the current PCL configuration entry.
*       pclExtraDataPtr      - pointer to extra data needed for the IPCL engine.
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
static GT_VOID snetCht3IPclKeyBuildIpcl1IPv4ShortKey
(
    IN SKERNEL_DEVICE_OBJECT                        * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC              * descrPtr,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC             * curLookUpCfgPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC                * pclExtraDataPtr,
    OUT SNET_CHT_POLICY_KEY_STC                     * pclKeyPtr
)
{
    DECLARE_FUNC_NAME(snetCht3IPclKeyBuildIpcl1IPv4ShortKey);

    GT_U32      fieldVal;           /* field value */
    GT_U8       byteValue;          /* value of user defined byte from the packets or tcp/udp compare */
    GT_U8       tmpFieldVal[4];     /* byte array */
    GT_U32      l3TotalLength;      /* Layer 3 total length */
    GT_U32      pcktType;           /* Packet type  */
    GT_STATUS   rc;                 /* status from called function */

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, curLookUpCfgPtr->pclID,
                            CHT3_PCL_KEY_FIELDS_ID_PCL_ID_E);/* [9:0] PCL-ID */
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->mac2me,
                            CHT3_PCL_KEY_FIELDS_ID_MAC_TO_ME_E);/*[10:10] mac2me */
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->localDevSrcPort ,
                            CHT3_PCL_KEY_FIELDS_ID_SRC_PORT_E);/* [16:11] src port */
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->origSrcTagged ,/* [17] is tagged */
                            CHT3_PCL_KEY_FIELDS_ID_IS_TAGGED_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->eVid ,/* [29:18] eVid */
                            CHT3_PCL_KEY_FIELDS_ID_VID_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->up  ,/* [32:30] UP */
                            CHT3_PCL_KEY_FIELDS_ID_UP_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, (descrPtr->l3StartOffsetPtr[1] & 0x3f), /* [38:33] TOS[5:0] */
                            CHT3_PCL_KEY_FIELDS_PCKT_TOS_0_5_SHORT_E);
    pcktType = (descrPtr->arp) ? 1 :
               (descrPtr->mpls) ? 4 :
               (descrPtr->isIp && descrPtr->isIPv4 == 0) ? 6 :
               (descrPtr->isIp && descrPtr->isIPv4) ? 7  : 0;

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, pcktType,/* [41:39] Packet type */
                           CHT3_PCL_KEY_FIELDS_PCKT_TYPE_E);

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->ipProt,/* [49:42] IP Protocol*/
                            CHT3_PCL_KEY_FIELDS_ID_IP_PROTOCOL_E);

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, ((descrPtr->l3StartOffsetPtr[1] >> 6) & 0x3),/* [51:50] TOS[7:6] */
                            CHT3_PCL_KEY_FIELDS_PCKT_TOS_6_7_SHORT_E);

    if (pclExtraDataPtr->isIpv4Fragment == 0)
    {
        fieldVal = 0;
    }
    else if (pclExtraDataPtr->ipv4FragmentOffset == 0)
    {
        fieldVal = 1;
    }
    else
    {
        fieldVal = 2;
    }

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, fieldVal, /* [53:52] IP fragmented */
                            CHT3_PCL_KEY_FIELDS_IP_FRAGMENTED_SHORT_E);

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->ipv4HeaderInfo,/* [55:54] IP header info */
                            CHT3_PCL_KEY_FIELDS_IP_HEADER_INFO_SHORT_E);

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL4Valid, /* [56] isL4Valid*/
                            CHT3_PCL_KEY_FIELDS_ID_IS_L4_VALID_E);
    if (descrPtr->l4StartOffsetPtr != NULL)
    {
        if (descrPtr->ipv6Icmp || descrPtr->ipv4Icmp)
        {
            snetCht3IPclKeyFieldBuildByPointer(/* [72:57] L4 Byte Offsets 5:4 */
                pclKeyPtr,(GT_U8*)&(descrPtr->l4StartOffsetPtr[4]),
                CHT3_PCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_E);
        }
        else
        {
            snetCht3IPclKeyFieldBuildByPointer(/* [72:57] L4 Byte Offsets 3:2 */
                pclKeyPtr,(GT_U8*)&(descrPtr->l4StartOffsetPtr[2]),
                CHT3_PCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_E);
        }
    }

    l3TotalLength = (descrPtr->l3StartOffsetPtr[2] << 8) |
                     descrPtr->l3StartOffsetPtr[3];

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, ((l3TotalLength >> 13) & 0x1), /* [73] IP packet length[13]*/
                            CHT3_PCL_KEY_FIELDS_IP_PCKT_LEN_13_E);

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, 0,/* [74] reserved  */
                            CHT3_PCL_KEY_FIELDS_ID_RESERVED_74_E);

    rc = snetChtIPclTcpUdpPortRangeCompareGet(devObjPtr, descrPtr, &byteValue);
    byteValue = (rc != GT_OK) ? 0 : byteValue ;

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [82:75] TCP/UDP comparator */
                            CHT3_PCL_KEY_FIELDS_ID_TCP_UDP_PORT_COMPS_SHORT_E);

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, (l3TotalLength & 0X3FFF), /* [95-83] IP packet length[12:0]*/
                            CHT3_PCL_KEY_FIELDS_IP_PCKT_LEN_0_12_E);

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, (descrPtr->ttl & 0x7), /* [98-96] Packet TTL [2:0]*/
                            CHT3_PCL_KEY_FIELDS_ID_TTL_0_2_E);
    /* for IPv4 packets only the first index sip[0] or dip[0] is relevant */
    __LOG(("for IPv4 packets only the first index sip[0] or dip[0] is relevant"));
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[0], /* [130:99] SIP*/
                            CHT3_PCL_KEY_FIELDS_ID_SIP_31_0_E);

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[0], /* [162:131] DIP*/
                            CHT3_PCL_KEY_FIELDS_ID_DIP_31_0_SHORT_E);

    if (descrPtr->l4StartOffsetPtr != NULL)
    {
        tmpFieldVal[0] = descrPtr->l4StartOffsetPtr[13];
        tmpFieldVal[1] = descrPtr->l4StartOffsetPtr[1];
        tmpFieldVal[2] = descrPtr->l4StartOffsetPtr[0];
        snetCht3IPclKeyFieldBuildByPointer(
            pclKeyPtr,(GT_U8*)&tmpFieldVal[0], /* [186:163] L4 byte offsets */
                    CHT3_PCL_KEY_FIELDS_ID_L4_BYTES_OFFSETS_0_1_13_SHORT_E);
    }

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, ((descrPtr->ttl >> 3) & 0x1f), /* [191-187] Packet TTL [7:3] */
                            CHT3_PCL_KEY_FIELDS_ID_TTL_3_7_E);
    return;
}

/*******************************************************************************
*   snetCht3IPclKeyBuildIpcl0IPv6TripleKey
*
* DESCRIPTION:
*        Build IPCL0 IPv6 triple PCL search key.
*
* INPUTS:
*       devObjPtr            - pointer to device object.
*       descrPtr             - pointer to frame data buffer Id.
*       curLookUpCfgPtr - pointer to the current PCL configuration entry.
*       pclExtraDataPtr      - pointer to extra data needed for the IPCL engine.
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
static GT_VOID snetCht3IPclKeyBuildIpcl0IPv6TripleKey
(
    IN SKERNEL_DEVICE_OBJECT                        * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC              * descrPtr,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC             * curLookUpCfgPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC                * pclExtraDataPtr,
    OUT SNET_CHT_POLICY_KEY_STC                     * pclKeyPtr
)
{
    DECLARE_FUNC_NAME(snetCht3IPclKeyBuildIpcl0IPv6TripleKey);

    GT_U32      fieldVal;           /* field value */
    GT_U8       byteValue;          /* value of user defined byte from the packets or tcp/udp compare */
    GT_U8       tmpFieldVal[6];     /* byte array */
    GT_U32      userDefError = 0;
    GT_U32      byte;               /* byte array index */

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, curLookUpCfgPtr->pclID,
                            CHT3_PCL_KEY_FIELDS_ID_PCL_ID_E);/* [9:0] PCL-ID */
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->mac2me,
                            CHT3_PCL_KEY_FIELDS_ID_MAC_TO_ME_E);/*[10:10] mac2me */
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->localDevSrcPort ,
                            CHT3_PCL_KEY_FIELDS_ID_SRC_PORT_E);/* [16:11] src port */
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->origSrcTagged ,/* [17] is tagged */
                            CHT3_PCL_KEY_FIELDS_ID_IS_TAGGED_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->eVid ,/* [29:18] eVid */
                            CHT3_PCL_KEY_FIELDS_ID_VID_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->up  ,/* [32:30] UP */
                            CHT3_PCL_KEY_FIELDS_ID_UP_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, /* [38:33] TOS[5:0] */
        (((descrPtr->l3StartOffsetPtr[0] & 3) << 4)
         | ((descrPtr->l3StartOffsetPtr[1] >> 4) & 0x0F)),
        CHT3_PCL_KEY_FIELDS_PCKT_TOS_0_5_SHORT_E);

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpV6EhHopByHop, /* [39] ipv6_eh_hopbyhop */
                            CHT3_PCL_KEY_FIELDS_ID_IS_IPV6_EH_HOP_BY_HOP_IPCL0_TRIPLE_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpV6EhExists, /* [40] ipv6_eh_exist */
                            CHT3_PCL_KEY_FIELDS_ID_IS_IPV6_EH_EXISTS_IPCL0_TRIPLE_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->isIp,/* [41] is IP */
                            CHT3_PCL_KEY_FIELDS_ID_IS_IP_E);

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, /* [51:50] TOS[7:6] */
        ((descrPtr->l3StartOffsetPtr[0] >> 2) & 0x3),
        CHT3_PCL_KEY_FIELDS_PCKT_TOS_6_7_SHORT_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, CHT3_UNDEFINED_FIELD,/* [57:52] TOS[7:6] */
                            CHT3_PCL_KEY_FIELDS_ID_TCP_FLAGS_IPCL0_LONG_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->etherTypeOrSsapDsap,/* [58:43] */
                            CHT3_PCL_KEY_FIELDS_ID_ETHER_TYPE_DSAP_SSAP_IPCL0_LONG_E);
    if (descrPtr->l4StartOffsetPtr != NULL)
    {
        if (descrPtr->ipv6Icmp || descrPtr->ipv4Icmp)
        {
            snetCht3IPclKeyFieldBuildByPointer(/* [89:74] L4 Byte Offsets 5:4 */
                        pclKeyPtr,(GT_U8*)&(descrPtr->l4StartOffsetPtr[4]),
                        CHT3_PCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_IPCL0_E);
        }
        else
        {
            snetCht3IPclKeyFieldBuildByPointer(/* [89:74] L4 Byte Offsets 3:2 */
                        pclKeyPtr,(GT_U8*)&(descrPtr->l4StartOffsetPtr[2]),
                        CHT3_PCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_IPCL0_E);
        }
    }

    snetCht3IPclKeyFieldBuildByValue(/* [90] Is ND */
        pclKeyPtr,
        (CH3_IPV6_IS_NET_DISCOVERY_DIP_MAC(descrPtr->dip) ? 1 : 0),
        CHT3_PCL_KEY_FIELDS_ID_IS_IS_ND_E);

    fieldVal = (descrPtr->macDaType == SKERNEL_BROADCAST_MAC_E ||
              descrPtr->macDaType == SKERNEL_BROADCAST_ARP_E) ? 1 : 0;

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, fieldVal, /* [91] Is BC */
                            CHT3_PCL_KEY_FIELDS_ID_IS_BC_TRIPLE_E);

    snetCht3IPclKeyFieldBuildByPointer(pclKeyPtr, descrPtr->macSaPtr,/* [139:92] MAC SA*/
                            CHT3_PCL_KEY_FIELDS_ID_MAC_SA_SHORT_E);
    snetCht3IPclKeyFieldBuildByPointer(pclKeyPtr, descrPtr->macDaPtr,/* [187:140] MAC DA*/
                            CHT3_PCL_KEY_FIELDS_ID_MAC_DA_SHORT_E);
    if (descrPtr->l4StartOffsetPtr != NULL)
    {
        tmpFieldVal[0] = descrPtr->l4StartOffsetPtr[0];
        tmpFieldVal[1] = descrPtr->l4StartOffsetPtr[1];
        tmpFieldVal[2] = descrPtr->l4StartOffsetPtr[13];
        snetCht3IPclKeyFieldBuildByPointer(
            pclKeyPtr,(GT_U8*)&tmpFieldVal[0], /* [211:188] L4 byte offsets */
                    CHT3_PCL_KEY_FIELDS_ID_L4_BYTES_OFFSETS_13_1_0_IPCL0_LONG_E);
    }

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[3], /* [243:212] SIP[31-0] */
                            CHT3_PCL_KEY_FIELDS_ID_SIP_31_0_IPCL0_LONG_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[2], /* [307:276] SIP[63-32] */
                            CHT3_PCL_KEY_FIELDS_ID_SIP_63_32_EXT_1_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[1], /* [339:308] SIP[95-64] */
                            CHT3_PCL_KEY_FIELDS_ID_SIP_95_64_EXT_1_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[0], /* [243:340] SIP[127-96] */
                            CHT3_PCL_KEY_FIELDS_ID_SIP_127_96_EXT_1_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[3], /* [275:244] DIP[31-0] */
                            CHT3_PCL_KEY_FIELDS_ID_DIP_31_0_IPCL0_LONG_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[2], /* [403:372] DIP[127:32] */
                            CHT3_PCL_KEY_FIELDS_ID_DIP_63_32_EXT_1_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[1], /* [435:404] DIP[127:32] */
                            CHT3_PCL_KEY_FIELDS_ID_DIP_95_64_EXT_1_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[0], /* [467:436] DIP[127:32] */
                            CHT3_PCL_KEY_FIELDS_ID_DIP_127_96_EXT_1_E);

    userDefError = snetChtPclUdbKeyValueGet(devObjPtr, descrPtr,
                                             userDefByteKey10Array,
                                             0, &byteValue);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [475:468] UDB0 Key 10 */
                            CHT3_PCL_KEY_FIELDS_ID_UDB3_475_468_E);

    fieldVal = (descrPtr->l2Encaps == SKERNEL_ETHERNET_II_E ||
                       descrPtr->l2Encaps == SKERNEL_LLC_SNAP_E) ? 1 : 0;
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, fieldVal,/* [476] Encap Type */
                            CHT3_PCL_KEY_FIELDS_ID_ENCAP_TYPE_VALID_TRIPLE_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL2Valid, /* [477] isL2Valid  */
                            CHT3_PCL_KEY_FIELDS_ID_USER_DEFINED_VALID_IPCL0_TRIPLE_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpHeaderOk , /* [478] ip header ok */
                            CHT3_PCL_KEY_FIELDS_IP_HEADER_OK_IPCL0_TRIPLE_E);


     /* Key10 User Define Byte1-2-3 */
    __LOG(("Key10 User Define Byte1-2-3"));
    for (byte = 1; byte < 4; byte++)
    {
        userDefError |= snetChtPclUdbKeyValueGet(devObjPtr, descrPtr,
                                                 userDefByteKey10Array,
                                                 byte, &tmpFieldVal[byte-1]);
    }

    snetCht3IPclKeyFieldBuildByPointer(pclKeyPtr,   /* [503-480] UserDefinedBytes 1-2-3 Key 10 */
                                       tmpFieldVal,
                                       CHT3_PCL_KEY_FIELDS_ID_UDB1_2_3_KEY10_E);

    /* Key11 User Define Byte0-1-2-3-4-5 */
    __LOG(("Key11 User Define Byte0-1-2-3-4-5"));
    for (byte = 0; byte < 5; byte++)
    {
        userDefError |= snetChtPclUdbKeyValueGet(devObjPtr, descrPtr,
                                                 userDefByteKey11Array,
                                                 byte, &tmpFieldVal[4-byte]);
    }

    snetCht3IPclKeyFieldBuildByPointer(pclKeyPtr,   /* [543-504] UserDefinedBytes 0-1-2-3-4-5 Key 11 */
                                       tmpFieldVal,
                                       CHT3_PCL_KEY_FIELDS_ID_UDB0_1_2_3_4_KEY11_E);
    /* Key12 User Define Byte0-1-2 */
    __LOG(("Key12 User Define Byte0-1-2"));
    for (byte = 0; byte < 3; byte++)
    {
        userDefError |= snetChtPclUdbKeyValueGet(devObjPtr, descrPtr,
                                                 userDefByteKey12Array,
                                                 byte, &tmpFieldVal[2-byte]);
    }

    snetCht3IPclKeyFieldBuildByPointer(pclKeyPtr,   /* [567-544] UserDefinedBytes 0-1-2 Key 12 */
                                       tmpFieldVal,
                                       CHT3_PCL_KEY_FIELDS_ID_UDB0_1_2_KEY12_TRIPLE_E);

    userDefError |= snetChtPclUdbKeyValueGet(devObjPtr, descrPtr,
                                             userDefByteKey0Array,
                                             3, &byteValue);

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr,   /* [568-575] UserDefinedBytes 3 Key 0 (UDB18) */
                                     byteValue,
                                     CHT3_PCL_KEY_FIELDS_ID_UDB18_E);

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL2Valid,/* [477] UserDefineValid */
                            CHT3_PCL_KEY_FIELDS_ID_IS_L2_VALID_IPCL0_TRIPLE_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, CH3_UDB_VALID_MAC(userDefError),/* [478] UserDefineValid */
                            CHT3_PCL_KEY_FIELDS_ID_USER_DEFINED_VALID_IPCL0_TRIPLE_E);

    return;
}

/*******************************************************************************
*   snetCht3IPclKeyBuildIpcl1IPv6TripleKey
*
* DESCRIPTION:
*        Build IPCL1 IPv6 triple PCL search key.
*
* INPUTS:
*       devObjPtr            - pointer to device object.
*       descrPtr             - pointer to frame data buffer Id.
*       curLookUpCfgPtr - pointer to the current PCL configuration entry.
*       pclExtraDataPtr      - pointer to extra data needed for the IPCL engine.
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
static GT_VOID snetCht3IPclKeyBuildIpcl1IPv6TripleKey
(
    IN SKERNEL_DEVICE_OBJECT                        * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC              * descrPtr,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC             * curLookUpCfgPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC                * pclExtraDataPtr,
    OUT SNET_CHT_POLICY_KEY_STC                     * pclKeyPtr
)
{
    GT_U8       byteValue;          /* value of user defined byte from the packets or tcp/udp compare */
    GT_U8       tmpFieldVal[6];     /* byte array */
    GT_U32      l3TotalLength;      /* Layer 3 total length */
    GT_U32      userDefError = 0;
    GT_U32      pcktType;           /* Packet type  */
    GT_U32      byte;               /* byte array index */
    GT_U32      isMld;

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, curLookUpCfgPtr->pclID,
                            CHT3_PCL_KEY_FIELDS_ID_PCL_ID_E);/* [9:0] PCL-ID */
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->mac2me,
                            CHT3_PCL_KEY_FIELDS_ID_MAC_TO_ME_E);/*[10:10] mac2me */
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->localDevSrcPort ,
                            CHT3_PCL_KEY_FIELDS_ID_SRC_PORT_E);/* [16:11] src port */
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->origSrcTagged ,/* [17] is tagged */
                            CHT3_PCL_KEY_FIELDS_ID_IS_TAGGED_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->eVid ,/* [29:18] eVid */
                            CHT3_PCL_KEY_FIELDS_ID_VID_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->up  ,/* [32:30] UP */
                            CHT3_PCL_KEY_FIELDS_ID_UP_E);

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, /* [38:33] TOS[5:0] */
        (((descrPtr->l3StartOffsetPtr[0] & 3) << 4)
         | ((descrPtr->l3StartOffsetPtr[1] >> 4) & 0x0F)),
        CHT3_PCL_KEY_FIELDS_PCKT_TOS_0_5_SHORT_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, /* [51:50] TOS[7:6] */
        ((descrPtr->l3StartOffsetPtr[0] >> 2) & 0x3),
        CHT3_PCL_KEY_FIELDS_PCKT_TOS_6_7_SHORT_E);

    pcktType = (descrPtr->arp) ? 1 :
               (descrPtr->mpls) ? 4 :
               (descrPtr->isIp && descrPtr->isIPv4 == 0) ? 6 :
               (descrPtr->isIp && descrPtr->isIPv4) ? 7  : 0;

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, pcktType,/* [41:39] Packet type */
                           CHT3_PCL_KEY_FIELDS_PCKT_TYPE_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->ipProt,/* [49:42] IP Protocol*/
                           CHT3_PCL_KEY_FIELDS_ID_IP_PROTOCOL_E);

    l3TotalLength = (descrPtr->l3StartOffsetPtr[4] << 8) |
                     descrPtr->l3StartOffsetPtr[5];

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, (l3TotalLength & 0x1FFF), /* [64:52] IP packet length[12:0]*/
                            CHT3_PCL_KEY_FIELDS_IP_PCKT_LEN_0_12_TRIPLE_E);

    userDefError = snetChtPclUdbKeyValueGet(devObjPtr, descrPtr,
                                             userDefByteKey11Array,
                                             2, &byteValue);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, byteValue,  /* [72-65] UserDefinedBytes 2 Key 11 */
                            CHT3_PCL_KEY_FIELDS_ID_UDB2_KEY11_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, 0,  /* [73] Reserved */
                                     CHT3_PCL_KEY_FIELDS_ID_RESERVED_73_E);
    if (descrPtr->l4StartOffsetPtr != NULL)
    {
        if (descrPtr->ipv6Icmp || descrPtr->ipv4Icmp)
        {
            snetCht3IPclKeyFieldBuildByPointer(/* [89:74] L4 Byte Offsets 5:4 */
                pclKeyPtr,(GT_U8*)&(descrPtr->l4StartOffsetPtr[4]),
                CHT3_PCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_IPCL0_E);
        }
        else
        {
            snetCht3IPclKeyFieldBuildByPointer(/* [89:74] L4 Byte Offsets 3:2 */
                pclKeyPtr,(GT_U8*)&(descrPtr->l4StartOffsetPtr[2]),
                CHT3_PCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_IPCL0_E);
        }
    }
    snetCht3IPclKeyFieldBuildByValue(/* [90] Is ND */
        pclKeyPtr,
        (CH3_IPV6_IS_NET_DISCOVERY_DIP_MAC(descrPtr->dip) ? 1 : 0),
        CHT3_PCL_KEY_FIELDS_ID_IS_IS_ND_E);

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL4Valid, /* [91] isL4Valid*/
                            CHT3_PCL_KEY_FIELDS_ID_IS_L4_VALID_TRIPLE_E);

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->flowLabel, /* [111-92] IPV6 flow label */
                            CHT3_PCL_KEY_FIELDS_ID_IPV6_HEADER_FLOW_LBL_E);

#if 0
    if (descrPtr->macDaType == SKERNEL_UNICAST_MAC_E ||
        descrPtr->macDaType == SKERNEL_MULTICAST_MAC_E)
    {

        snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->mplsLabels[0].label, /* [131-112] MPLS outer label */
                                CHT3_PCL_KEY_FIELDS_ID_MPLS_OUTER_LBL_E);
        snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->mplsLabels[0].exp, /* [175-173]  MPLS Outer Label EXP */
                                CHT3_PCL_KEY_FIELDS_ID_MPLS_OUTER_LBL_EXP_E);
        snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->mplsLabels[0].sbit, /* [176] MPLS Outer Label S-Bit */
                                CHT3_PCL_KEY_FIELDS_ID_MPLS_OUTER_LBL_S_BIT_E);
    }
#endif
    for (byte = 0; byte < 2; byte++)
    {
        userDefError |= snetChtPclUdbKeyValueGet(devObjPtr, descrPtr,
                                                 userDefByteKey11Array,
                                                 byte, &tmpFieldVal[1-byte]);
    }

    snetCht3IPclKeyFieldBuildByPointer(pclKeyPtr, tmpFieldVal,                 /* [147:132] User Defined Bytes0-1 Key11 */
                                       CHT3_PCL_KEY_FIELDS_ID_UDB0_1_KEY11_E);

    for (byte = 0; byte < 3; byte++)
    {
        userDefError |= snetChtPclUdbKeyValueGet(devObjPtr, descrPtr,
                                                 userDefByteKey12Array,
                                                 byte, &tmpFieldVal[2-byte]);
    }

    snetCht3IPclKeyFieldBuildByPointer(pclKeyPtr, tmpFieldVal,  /* [171:148] User Defined Bytes0-1-2 Key12 */
                            CHT3_PCL_KEY_FIELDS_ID_UDB0_1_2_KEY12_TRIPLE1_E);

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->ipXMcLinkLocalProt, /* [172] IPv6 Link Local Protocol */
                            CHT3_PCL_KEY_FIELDS_ID_IS_IPV6_LINK_LOCAL_E);

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, ((l3TotalLength >> 13) & 0x7), /* [179:177] IP packet length[15-13]*/
                            CHT3_PCL_KEY_FIELDS_IP_PCKT_LEN_15_13_E);

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->ttl, /* [187-180] Packet TTL */
                            CHT3_PCL_KEY_FIELDS_ID_TTL_E);
    if (descrPtr->l4StartOffsetPtr != NULL)
    {
        tmpFieldVal[0] = descrPtr->l4StartOffsetPtr[0];
        tmpFieldVal[1] = descrPtr->l4StartOffsetPtr[1];
        tmpFieldVal[2] = descrPtr->l4StartOffsetPtr[13];
        snetCht3IPclKeyFieldBuildByPointer(pclKeyPtr,(GT_U8*)&tmpFieldVal[0], /* [211:188] L4 byte offsets */
                                CHT3_PCL_KEY_FIELDS_ID_L4_BYTES_OFFSETS_13_1_0_IPCL0_LONG_E);
    }

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[3], /* [243:212] SIP[31:0] */
                            CHT3_PCL_KEY_FIELDS_ID_SIP_31_0_IPCL0_LONG_E);

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[2], /* [307:276] SIP[63:32] */
                            CHT3_PCL_KEY_FIELDS_ID_SIP_63_32_EXT_1_E);

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[1], /* [339:308] SIP[95:64] */
                            CHT3_PCL_KEY_FIELDS_ID_SIP_95_64_EXT_1_E);

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[0], /* [371:340] SIP[127:96] */
                            CHT3_PCL_KEY_FIELDS_ID_SIP_127_96_EXT_1_E);

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[3], /* [275:244] DIP[31:0] */
                            CHT3_PCL_KEY_FIELDS_ID_DIP_31_0_IPCL0_LONG_E);

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[2], /* [403:372] DIP[63-32]*/
                            CHT3_PCL_KEY_FIELDS_ID_DIP_63_32_EXT_1_E);

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[1], /* [435:404] DIP[95-64]*/
                            CHT3_PCL_KEY_FIELDS_ID_DIP_95_64_EXT_1_E);

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[0], /* [467:436] DIP[127-96]*/
                            CHT3_PCL_KEY_FIELDS_ID_DIP_127_96_EXT_1_E);

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, 0, /* [468] Reserved */
                            CHT3_PCL_KEY_FIELDS_ID_RESERVED_468_E);

    isMld = 0;
    if ((descrPtr->l4StartOffsetPtr != NULL) && (descrPtr->ipv6Icmp != 0))
    {
        GT_U8 icmpMsgType = descrPtr->l4StartOffsetPtr[0];

        if (pclExtraDataPtr->isIpV6EhExists)
        {
            /* skip extended header */
            GT_U32 payloadSize =
                ((GT_U32)descrPtr->l3StartOffsetPtr[4] << 8)
                + descrPtr->l3StartOffsetPtr[5];
            icmpMsgType = descrPtr->l3StartOffsetPtr[payloadSize];
        }

        switch (icmpMsgType)
        {
            case 131: /* Group Membership Report */
            case 132: /* Group Membership Reduction */
            case 133: /* Router Solicitation */
            case 143: /* MLDv2 Multicast Listener Report */
                isMld = 1;
                break;
            default: break;
        }
    }

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, isMld, /* [469] isMld */
                            CHT3_PCL_KEY_FIELDS_ID_IS_IPV6_MLD_E);

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpV6EhHopByHop, /* [470] hopByHop */
                            CHT3_PCL_KEY_FIELDS_ID_IS_IPV6_EH_HOP_BY_HOP_IPCL1_TRIPLE_E);

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpV6EhExists, /* [471] isIpv6EhExists*/
                            CHT3_PCL_KEY_FIELDS_ID_IS_IPV6_EH_EXISTS_IPCL1_TRIPLE_E);

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpHeaderOk ,/* [479] ip header OK */
                            CHT3_PCL_KEY_FIELDS_IP_HEADER_OK_IPCL0_TRIPLE_E);

    for (byte = 0; byte < 2; byte++)
    {
        userDefError |= snetChtPclUdbKeyValueGet(devObjPtr, descrPtr,
                                                 userDefByteKey0Array,
                                                 byte, &tmpFieldVal[1-byte]);
    }

    snetCht3IPclKeyFieldBuildByPointer(pclKeyPtr, tmpFieldVal,  /* [495:480] User Defined Bytes0-1 Key0 */
                            CHT3_PCL_KEY_FIELDS_ID_UDB0_1_KEY0_E);

    for (byte = 0; byte < 2; byte++)
    {
        userDefError |= snetChtPclUdbKeyValueGet(devObjPtr, descrPtr,
                                                 userDefByteKey1Array,
                                                 byte, &tmpFieldVal[1-byte]);
    }

    snetCht3IPclKeyFieldBuildByPointer(pclKeyPtr, tmpFieldVal,  /* [511:496] User Defined Bytes0-1 Key1 */
                            CHT3_PCL_KEY_FIELDS_ID_UDB0_1_KEY1_TRIPLE_E);

    userDefError |= snetChtPclUdbKeyValueGet(devObjPtr, descrPtr,
                            userDefByteKey0Array,
                            3, &byteValue);

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr,   /* [519-512] UserDefinedBytes 3 Key 0 (UDB18)*/
                            byteValue,
                            CHT3_PCL_KEY_FIELDS_ID_UDB18_TRIPLE_E);
    for (byte = 0; byte < 3; byte++)
    {
        userDefError |= snetChtPclUdbKeyValueGet(devObjPtr, descrPtr,
                                                 userDefByteKey2Array,
                                                 byte, &tmpFieldVal[2-byte]);
    }

    snetCht3IPclKeyFieldBuildByPointer(pclKeyPtr, tmpFieldVal,  /* [543:520] User Defined Bytes0-1-2 Key12 */
                            CHT3_PCL_KEY_FIELDS_ID_UDB0_1_2_KEY2_TRIPLE_E);

    for (byte = 0; byte < 4; byte++)
    {
        userDefError |= snetChtPclUdbKeyValueGet(devObjPtr, descrPtr,
                            userDefByteKey10Array,
                            byte, &tmpFieldVal[3-byte]);
    }

    snetCht3IPclKeyFieldBuildByPointer(pclKeyPtr, tmpFieldVal,  /* [575:544] User Defined Bytes0-1-2-3 Key10 */
                            CHT3_PCL_KEY_FIELDS_ID_UDB0_1_2_3_KEY10_E);

    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, CH3_UDB_VALID_MAC(userDefError),/* [478] User Defined Valid */
                            CHT3_PCL_KEY_FIELDS_ID_USER_DEFINED_VALID_IPCL0_TRIPLE_E);

    return;
}

/*******************************************************************************
*   snetCht3IPclKeyBuildIpclIPv6DipShortKey
*
* DESCRIPTION:
*        Build Key Type:  Standard (24B) IPv6 DIP - Ingress PCL search key.
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       descrPtr            - pointer to frame data buffer Id.
*       curLookUpCfgPtr     - pointer to the current PCL configuration entry.
*       pclExtraDataPtr     - pointer to extra data needed for the IPCL engine.
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
static GT_VOID snetCht3IPclKeyBuildIpclIPv6DipShortKey
(
    IN SKERNEL_DEVICE_OBJECT                        * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC              * descrPtr,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC             * curLookUpCfgPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC                * pclExtraDataPtr,
    OUT SNET_CHT_POLICY_KEY_STC                     * pclKeyPtr
)
{
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, curLookUpCfgPtr->pclID,
                            CHT3_PCL_KEY_FIELDS_ID_PCL_ID_E);/* [9:0] PCL-ID */
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->mac2me,
                            CHT3_PCL_KEY_FIELDS_ID_MAC_TO_ME_E);/*[10:10] mac2me */
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->localDevSrcPort ,
                            CHT3_PCL_KEY_FIELDS_ID_SRC_PORT_E);/* [16:11] src port */
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->origSrcTagged ,/* [17] is tagged */
                            CHT3_PCL_KEY_FIELDS_ID_IS_TAGGED_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->eVid ,/* [29:18] eVid */
                            CHT3_PCL_KEY_FIELDS_ID_VID_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->up  ,/* [32:30] UP */
                            CHT3_PCL_KEY_FIELDS_ID_UP_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->qos.qosProfile,/* [39:33] QOS profile */
                            CHT3_PCL_KEY_FIELDS_ID_QOS_PROFILE_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->isIPv4 , /* [40] is IPv4 */
                            CHT3_PCL_KEY_FIELDS_ID_IS_IPV4_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->isIp,/* [41] is IP */
                            CHT3_PCL_KEY_FIELDS_ID_IS_IP_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->ipProt,/* [49:42] IP Protocol*/
                            CHT3_PCL_KEY_FIELDS_ID_IP_PROTOCOL_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dscp,/* [55:50] dscp */
                            CHT3_PCL_KEY_FIELDS_ID_DSCP_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL4Valid, /* [56] isL4Valid*/
                            CHT3_PCL_KEY_FIELDS_ID_IS_L4_VALID_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, (descrPtr->dip[3] & 0xffff), /* [72:57] DIP[15:0] */
                            CHT3_PCL_KEY_FIELDS_ID_DIP_15_0_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->arp ,/* [73] isArp */
                            CHT3_PCL_KEY_FIELDS_ID_IS_ARP_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpV6EhExists, /* [74] ipv6_eh_exist */
                            CHT3_PCL_KEY_FIELDS_ID_IS_IPV6_EH_EXISTS_SHORT_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, (descrPtr->dip[3] >> 16), /* [90:75] DIP[31:16] */
                            CHT3_PCL_KEY_FIELDS_ID_DIP_31_16_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpV6EhHopByHop,/* [91] is IPv6 hopbyhop */
                            CHT3_PCL_KEY_FIELDS_IS_IPV6_HOP_BY_HOP_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[2], /* [123:92] DIP[63:32] */
                            CHT3_PCL_KEY_FIELDS_ID_DIP_63_32_LONG_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[1], /* [155:124] DIP[95:64] */
                            CHT3_PCL_KEY_FIELDS_ID_DIP_95_64_LONG_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[0], /* [187:156] DIP[127:96] */
                            CHT3_PCL_KEY_FIELDS_ID_DIP_127_96_LONG_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, 0,              /* [188] reserved */
                            CHT3_PCL_KEY_FIELDS_ID_RESERVED_188_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL2Valid ,/* [189] isL2Valid  */
                            CHT3_PCL_KEY_FIELDS_ID_IS_L2_VALID_SHORT_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, 0,                  /* [190] reserved */
                            CHT3_PCL_KEY_FIELDS_ID_RESERVED_190_E);
    snetCht3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpHeaderOk ,/* [191] ip header  */
                            CHT3_PCL_KEY_FIELDS_ID_IP_HEADER_OK_SHORT_E);

    return  ;
}

/*******************************************************************************
* snetCht3IPclConfigPtrGet
*
* DESCRIPTION:
*       Get data from the ingress PCL configuration table.
*
* INPUTS:
*       devObjPtr            - pointer to device object.
*       descrPtr             - pointer to frame descriptor object.
*
* OUTPUTS:
*       lookupConfPtr - pointer to extra data needed for the PCL engine.
*
* RETURN:
*
* COMMENTS:
*
*       Ingress PCL Configuration Table
*       The PCL configuration table is a 4224 line table each entry is 32-bits.
*
*******************************************************************************/
static GT_VOID snetCht3IPclConfigPtrGet
(
    IN SKERNEL_DEVICE_OBJECT                * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC      * descrPtr,
    OUT SNET_CHT3_PCL_LOOKUP_CONFIG_STC     * lookupConfPtr
)
{
    DECLARE_FUNC_NAME(snetCht3IPclConfigPtrGet);

    GT_U32 *pclGlobCfgWordPtr;  /* pointer to PCL global configuration register */
    GT_U32  maxDevPort;         /* number of ports in device 0 - 1( 32/64 ports) */
    GT_U32  index[2] = {0, 0};  /* PCL configuration entry 2 cycles key */
    GT_U32  pclCfgMode;         /* PCL configuration mode */
    GT_U32  ingressPclCfgMode;  /* IPCL configuration mode */
    GT_U32  regAddr;            /* register address */
    GT_U32  portIfPclBlock;     /* location of the Ports interface block in the PCL configuration table */
    GT_U32  pclCfgEntry[2];     /* PCL configuration entry */
    GT_U32  i;                  /* 0..1 loop index */
    GT_U32  pclCfgModeBit;      /* pcl cfg mode bit */
    SNET_IPCL_LOOKUP_ENT  pclLookUpCycle;       /* pcl lookup cycle */

    pclGlobCfgWordPtr =
        smemMemGet(devObjPtr, SMEM_CHT_PCL_GLOBAL_REG(devObjPtr));

    regAddr = SMEM_CHT_PORT_VLAN_QOS_CONFIG_TBL_MEM(devObjPtr, descrPtr->localDevSrcPort);

    for (i = 0; i < 2; i++)
    {

        pclCfgModeBit = (i == 0) ? 20 : 16;
        smemRegFldGet(devObjPtr, regAddr, pclCfgModeBit, 1, &pclCfgMode);
        if (pclCfgMode == 1)
        {
            /* Access PCL-ID config. table with vlan id  */
            __LOG(("Access PCL-ID config. table with vlan id"));
            index[i] = descrPtr->eVid;
        }
        else
        {
            ingressPclCfgMode = SMEM_U32_GET_FIELD(*pclGlobCfgWordPtr, 1, 1);
            if (ingressPclCfgMode == 0)
            {
                /* PCL configuration table index = Local Device Source Port# + 4K */
                __LOG(("PCL configuration table index = Local Device Source Port# + 4K"));
                SMEM_U32_SET_FIELD(index[i], 12, 1, 1);
                SMEM_U32_SET_FIELD(index[i], 0, 12, descrPtr->localDevSrcPort);
            }
            else
            if (descrPtr->origIsTrunk == 0)
            {
                /* Read the max port per device and the PCL interface block */
                __LOG(("Read the max port per device and the PCL interface block"));
                maxDevPort =
                    SMEM_U32_GET_FIELD(*pclGlobCfgWordPtr, 18, 1);
                portIfPclBlock =
                    SMEM_U32_GET_FIELD(*pclGlobCfgWordPtr, 16, 2);

                if (maxDevPort == 0)
                {
                    /* Max port per device = 0 , up to 1K ports in system */
                    __LOG(("Max port per device = 0 , up to 1K ports in system"));
                    SMEM_U32_SET_FIELD(index[i], 10, 2, portIfPclBlock);
                    SMEM_U32_SET_FIELD(index[i],  5, 5, descrPtr->srcDev);
                    SMEM_U32_SET_FIELD(index[i],  0, 5, descrPtr->origSrcEPortOrTrnk);
                }
                else
                {
                    /* Max port per device = 1 , up to 2K ports in system */
                    __LOG(("Max port per device = 1 , up to 2K ports in system"));
                    SMEM_U32_SET_FIELD(index[i], 11, 1, portIfPclBlock);
                    SMEM_U32_SET_FIELD(index[i],  6, 5, descrPtr->srcDev);
                    SMEM_U32_SET_FIELD(index[i],  0, 7, descrPtr->origSrcEPortOrTrnk);
                }
            }
            else
            {
                /* Access PCL-ID config. table with ingress trunk  */
                __LOG(("Access PCL-ID config. table with ingress trunk"));
                SMEM_U32_SET_FIELD(index[i],  12, 1, 1);
                SMEM_U32_SET_FIELD(index[i],  0, 7, descrPtr->origSrcEPortOrTrnk);
            }
        }
    } /* i -> 0..1 */

    /* Calculate the address of the PCL configuration table for the 1st lookup*/
    __LOG(("Calculate the address of the PCL configuration table for the 1st lookup"));
    pclLookUpCycle = SNET_IPCL_LOOKUP_0_0_E;
    regAddr = SMEM_CHT_PCL_CONFIG_TBL_MEM(devObjPtr, index[0], pclLookUpCycle);
    smemRegGet(devObjPtr, regAddr, &(pclCfgEntry[0]));
    smemRegGet(devObjPtr, (regAddr + 4), &(pclCfgEntry[1]));

    /* Ingress PCL configuration Table Entry , 1st lookup */
    __LOG(("Ingress PCL configuration Table Entry , 1st lookup"));
    lookupConfPtr[0].lookUpEn =
        SMEM_U32_GET_FIELD(pclCfgEntry[0], 0, 1);
    lookupConfPtr[0].pclNonIpTypeCfg =
        SMEM_U32_GET_FIELD(pclCfgEntry[0], 1, 3);
    lookupConfPtr[0].pclIpV4ArpTypeCfg =
        SMEM_U32_GET_FIELD(pclCfgEntry[0], 4, 3);
    lookupConfPtr[0].pclIpV6TypeCfg =
        SMEM_U32_GET_FIELD(pclCfgEntry[0], 7, 3);
    lookupConfPtr[0].pclID =
        SMEM_U32_GET_FIELD(pclCfgEntry[0], 11, 10);

    /* lookup0_1 enable when lookup0_0 enable and dooble-lookup0 bit is on */
    __LOG(("lookup0_1 enable when lookup0_0 enable and dooble-lookup0 bit is on"));
    lookupConfPtr[1].lookUpEn =
        lookupConfPtr[0].lookUpEn
        & SMEM_U32_GET_FIELD(pclCfgEntry[0], 10, 1);
    lookupConfPtr[1].pclNonIpTypeCfg =
        lookupConfPtr[0].pclNonIpTypeCfg;
    lookupConfPtr[1].pclIpV4ArpTypeCfg =
        lookupConfPtr[0].pclIpV4ArpTypeCfg;
    lookupConfPtr[1].pclIpV6TypeCfg =
        lookupConfPtr[0].pclIpV6TypeCfg;
    /* toggle lookup0_0 PCL id LSB */
    __LOG(("toggle lookup0_0 PCL id LSB"));
    lookupConfPtr[1].pclID = lookupConfPtr[0].pclID ^ 1;

    /* Calculate the address of the PCL configuration table for the 3nd lookup*/
    __LOG(("Calculate the address of the PCL configuration table for the 3nd lookup"));
    pclLookUpCycle = SNET_IPCL_LOOKUP_1_E;
    regAddr = SMEM_CHT_PCL_CONFIG_TBL_MEM(devObjPtr, index[1], pclLookUpCycle);
    smemRegGet(devObjPtr, regAddr, &(pclCfgEntry[0]));
    smemRegGet(devObjPtr, (regAddr + 4), &(pclCfgEntry[1]));

    /* Ingress PCL configuration Table Entry , 3nd lookup */
    __LOG(("Ingress PCL configuration Table Entry , 3nd lookup"));
    lookupConfPtr[2].lookUpEn =
        SMEM_U32_GET_FIELD(pclCfgEntry[0], 21, 1);
    lookupConfPtr[2].pclNonIpTypeCfg =
        SMEM_U32_GET_FIELD(pclCfgEntry[0], 22, 3);
    lookupConfPtr[2].pclIpV4ArpTypeCfg =
        SMEM_U32_GET_FIELD(pclCfgEntry[0], 25, 3);
    lookupConfPtr[2].pclIpV6TypeCfg =
        SMEM_U32_GET_FIELD(pclCfgEntry[0], 28, 3);
    lookupConfPtr[2].pclID =
        SMEM_U32_GET_FIELD(pclCfgEntry[1], 0, 10);

    lookupConfPtr[0].ingrlookUpKey = GT_TRUE;
    lookupConfPtr[1].ingrlookUpKey = GT_TRUE;
    lookupConfPtr[2].ingrlookUpKey = GT_TRUE;

    return;
}


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
)
{
    if ((descrPtr->isIPv4 == 1) || (descrPtr->arp == 1)) /* ipv4 or arp packet */
    {
        *keyTypePtr = SNET_CHT3_PCL_IPV4_ARP_KEY_TYPE(curLookupCfgPtr);
    }
    else if (descrPtr->isIp == 0) /* non ip, non arp packet */
    {
        *keyTypePtr = SNET_CHT3_PCL_NON_IP_KEY_TYPE(curLookupCfgPtr);
    }
    else /* ipv6 packet */
    {
        *keyTypePtr = SNET_CHT3_PCL_IPV6_KEY_TYPE(curLookupCfgPtr);
    }

    curLookupCfgPtr->keySize = SNET_CHT3_PCL_KEY_FORMAT(*keyTypePtr);

    return ;
}

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
)
{
    DECLARE_FUNC_NAME(snetCht3IPclActionApply);

    SNET_CHT2_PCL_ACTION_STC  * actionCht2DataPtr;
    GT_BOOL saveCntrEn; /* save counter enable status */
    GT_U32  origSstId;/* sstId of the incoming descriptor*/

    actionCht2DataPtr = (SNET_CHT2_PCL_ACTION_STC  *)actionDataPtr;

    if (actionCht2DataPtr->pceRedirectCmd == PCL_TTI_ACTION_REDIRECT_CMD_PBR_OR_VPLS_E)
    {
        /* Route action will be applied later in UC route engine */
        __LOG(("Route action will be applied later in UC route engine"));
        return;
    }

    /* prevent snetCht2IPclActionApply count processing */
    __LOG(("prevent snetCht2IPclActionApply count processing"));
    saveCntrEn = actionDataPtr->baseAction.matchCounterEn;
    actionDataPtr->baseAction.matchCounterEn = GT_FALSE;

    snetCht2IPclActionApply(devObjPtr, descrPtr, actionCht2DataPtr,isFirst);

    /* restore counter-enable state */
    __LOG(("restore counter-enable state"));
    actionDataPtr->baseAction.matchCounterEn = saveCntrEn;

    if(SNET_GET_DEST_INTERFACE_TYPE(actionDataPtr->pceExtEgressIf,
                                    SNET_DST_INTERFACE_LLT_INDEX_E))
    {
        descrPtr->pceRoutLttIdx =
            actionDataPtr->pceExtEgressIf.interfaceInfo.routerLLTIndex;
    }
    else
    if(SNET_GET_DEST_INTERFACE_TYPE(actionDataPtr->pceExtEgressIf,
                                    SNET_DST_INTERFACE_VIRTUAL_ROUTER_E))
    {
        descrPtr->vrfId =
            actionDataPtr->pceExtEgressIf.interfaceInfo.virtualRouterId;
    }

    if (actionDataPtr->srcIdSetEn)
    {
        origSstId = descrPtr->sstId;
        if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            /* remove the bits that should be set by this logic */
            descrPtr->sstId &= ~descrPtr->pclSrcIdMask;
            /* set the bits that should be set by this logic */
            descrPtr->sstId |= (actionDataPtr->srcId & descrPtr->pclSrcIdMask);

            descrPtr->pclAssignedSstId |= descrPtr->pclSrcIdMask;
            descrPtr->sstIdPrecedence = SKERNEL_PRECEDENCE_ORDER_HARD;/*just for legacy*/
        }
        else
        {
            /* ch3 not override DSA tag assignment - when 'Precedence hard' */
            if(descrPtr->sstIdPrecedence == SKERNEL_PRECEDENCE_ORDER_SOFT ||
                SKERNEL_IS_XCAT_REVISON_A1_DEV(devObjPtr))
            {
                descrPtr->sstId = actionDataPtr->srcId;
                descrPtr->sstIdPrecedence = SKERNEL_PRECEDENCE_ORDER_HARD;
            }
        }

        if(origSstId != descrPtr->sstId)
        {
            __LOG(("SST assignment : sstId changed from [0x%x] to [0x%x] \n",
                origSstId,descrPtr->sstId));
        }

    }

    return ;
}

/*******************************************************************************
* snetCht3IPclActionGet
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
*
*******************************************************************************/
static GT_VOID snetCht3IPclActionGet
(
    IN SKERNEL_DEVICE_OBJECT            *   devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  *   descrPtr  ,
    IN GT_U32                               matchIndex,
    OUT SNET_CHT3_PCL_ACTION_STC        *   actionDataPtr
)
{
    DECLARE_FUNC_NAME(snetCht3IPclActionGet);

    SNET_CHT2_PCL_ACTION_STC  * actionCht2DataPtr;
    GT_U32  * actionEntryDataPtr;
    GT_U32  tblAddr;
    GT_U32  fieldVal;
    GT_U32  tmpVal = 1;

    actionCht2DataPtr = (SNET_CHT2_PCL_ACTION_STC *)actionDataPtr;

    snetCht2IPclActionGet(devObjPtr, descrPtr, matchIndex, actionCht2DataPtr);

    /* Get relevant fields of PCL action for Cheetah3  */
    __LOG(("Get relevant fields of PCL action for Cheetah3"));
    tblAddr = SMEM_CHT_PCL_ACTION_TBL_MEM(devObjPtr, matchIndex);
    actionEntryDataPtr = smemMemGet(devObjPtr, tblAddr);

    /* pceRedirectCmd bit2 in bit119 */
    __LOG(("pceRedirectCmd bit2 in bit119"));
    actionCht2DataPtr->pceRedirectCmd |= (((actionEntryDataPtr[3] >> 23) & 1) << 2);

    /* Egress interface */
    __LOG(("Egress interface"));
    switch(actionCht2DataPtr->pceRedirectCmd)
    {
        case PCL_TTI_ACTION_REDIRECT_CMD_NONE_E:
            break;
        case PCL_TTI_ACTION_REDIRECT_CMD_OUT_IF_E:
        SNET_SET_DEST_INTERFACE_TYPE(actionDataPtr->pceExtEgressIf,
                                         SNET_DST_INTERFACE_EGRESS_E, tmpVal);

        SNET_SET_EXT_EGRESS_DEST_INTERFACE(actionDataPtr,
                                           actionCht2DataPtr->pceEgressIf.interfaceInfo);
            break;
        case PCL_TTI_ACTION_REDIRECT_CMD_LTT_ROUTER_E:
            SNET_SET_DEST_INTERFACE_TYPE(actionDataPtr->pceExtEgressIf,
                                         SNET_DST_INTERFACE_LLT_INDEX_E, tmpVal);

        /* PCERouter LTTIndex */
        __LOG(("PCERouter LTTIndex"));
        fieldVal = SMEM_U32_GET_FIELD(actionEntryDataPtr[1], 2, 13) | /* PCERouter LTTIndex[12:0] */
                  (SMEM_U32_GET_FIELD(actionEntryDataPtr[3], 24, 2) << 13); /* PCERouter LTTIndex[14:13] */

        SNET_SET_EXT_LLT_DEST_INTERFACE(actionDataPtr, fieldVal);
            break;
        case PCL_TTI_ACTION_REDIRECT_CMD_PBR_OR_VPLS_E:
            /* Action type is routed entry - ch+ backwards compatibility  */
            __LOG(("Action type is routed entry - ch+ backwards compatibility"));
            return;
        case PCL_TTI_ACTION_REDIRECT_CMD_VIRT_ROUTER_E:
            SNET_SET_DEST_INTERFACE_TYPE(actionDataPtr->pceExtEgressIf,
                                         SNET_DST_INTERFACE_VIRTUAL_ROUTER_E, tmpVal);

        fieldVal = SMEM_U32_GET_FIELD(actionEntryDataPtr[1], 2, 12);

        /* Virtual router */
        __LOG(("Virtual router"));
        SNET_SET_EXT_VR_DEST_INTERFACE(actionDataPtr, fieldVal);
            break;
        default:
            /* unsupported format */
            __LOG(("unsupported format"));

            /* invalid action --> apply no extra action */
            __LOG(("invalid action --> apply no extra action"));
            memset(actionDataPtr,0,sizeof(*actionDataPtr));
            return;
    }

    actionDataPtr->actionStop = SMEM_U32_GET_FIELD(actionEntryDataPtr[3], 1, 1);

    fieldVal = SMEM_U32_GET_FIELD(actionEntryDataPtr[3], 2, 8);
    /* CounterIndex[5] */
    __LOG(("CounterIndex[5]"));
    actionCht2DataPtr->matchCounterIndex |= ((actionEntryDataPtr[0] >> 31) << 5);
    actionCht2DataPtr->matchCounterIndex |=  fieldVal << 6;


    fieldVal = SMEM_U32_GET_FIELD(actionEntryDataPtr[3], 10, 4);
    actionCht2DataPtr->policerIndex =
        fieldVal << 8 | actionCht2DataPtr->policerIndex;

    fieldVal = SMEM_U32_GET_FIELD(actionEntryDataPtr[3], 14, 3);
    actionCht2DataPtr->tunnelPtr =
        fieldVal << 10 | actionCht2DataPtr->tunnelPtr;

    actionDataPtr->srcIdSetEn =
        SMEM_U32_GET_FIELD(actionEntryDataPtr[3], 17, 1);

    actionDataPtr->srcId =
        SMEM_U32_GET_FIELD(actionEntryDataPtr[3], 18, 5);


    return ;
}

/*******************************************************************************
* snetCht3IPclCreateKey
*
* DESCRIPTION:
*        Create PCL search key.
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - pointer to frame data buffer Id.
*       lookupConfigPtr - pointer to the PCL configuration table entry.
*       pclExtraDataPtr - pointer to extra data needed for the PCL engine.
*       cycleNo         - cycle num.
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
static GT_VOID snetCht3IPclCreateKey
(
    IN SKERNEL_DEVICE_OBJECT                * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC   * descrPtr,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC     * curLookUpCfgPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC        * pclExtraDataPtr,
    IN SNET_CHT3_PCL_KEY_TYPE_ENT             keyType,
    OUT SNET_CHT_POLICY_KEY_STC             * pclKeyPtr
)
{

    memset(&pclKeyPtr->key, 0, sizeof(pclKeyPtr->key));

    pclKeyPtr->updateOnlyDiff = GT_FALSE;
    pclKeyPtr->pclKeyFormat = curLookUpCfgPtr->keySize;
    pclKeyPtr->devObjPtr = devObjPtr;

    switch (keyType)
    {
        case CHT3_PCL_KEY_TYPE_L2_KEY_SHORT_E:
            snetCht3IPclBuildL2StandardKey(devObjPtr, descrPtr,
                                           curLookUpCfgPtr,
                                           pclExtraDataPtr,
                                           pclKeyPtr);
            break;
        case CHT3_PCL_KEY_TYPE_L2_L3_KEY_SHORT_E:
            snetCht3IPclBuildL2L3StandardKey(devObjPtr , descrPtr,
                                             curLookUpCfgPtr,
                                             pclExtraDataPtr,
                                             pclKeyPtr);
            break;
        case CHT3_PCL_KEY_TYPE_L3_L4_KEY_SHORT_E:
            snetCht3IPclBuildL3L4StandardKey(devObjPtr, descrPtr,
                                             curLookUpCfgPtr,
                                             pclExtraDataPtr,
                                             pclKeyPtr);
            break;
        case CHT3_PCL_KEY_TYPE_L2_L3_L4_KEY_LONG_E:
            snetCht3IPclBuildL2L3L4ExtendedKey(devObjPtr, descrPtr,
                                               curLookUpCfgPtr,
                                               pclExtraDataPtr,
                                               pclKeyPtr);
            break;
        case CHT3_PCL_KEY_TYPE_L2_IPV6_KEY_LONG_E:
            snetCht3IPclKeyBuildL2IPv6ExtendedKey(devObjPtr, descrPtr,
                                                  curLookUpCfgPtr,
                                                  pclExtraDataPtr,
                                                  pclKeyPtr);
            break;
        case CHT3_PCL_KEY_TYPE_L4_IPV6_KEY_LONG_E:
            snetCht3IPclKeyBuildL4IPv6ExtendedKey(devObjPtr, descrPtr,
                                                  curLookUpCfgPtr,
                                                  pclExtraDataPtr,
                                                  pclKeyPtr);
            break;

        case CHT3_PCL_KEY_TYPE_IPCL0_IPV4_KEY_LONG_E:
            snetCht3IPclKeyBuildIpcl0IPv4ExtendedKey(devObjPtr, descrPtr,
                                                  curLookUpCfgPtr,
                                                  pclExtraDataPtr,
                                                  pclKeyPtr);
            break;

        case CHT3_PCL_KEY_TYPE_IPCL1_IPV4_KEY_SHORT_E:
            snetCht3IPclKeyBuildIpcl1IPv4ShortKey(devObjPtr, descrPtr,
                                                  curLookUpCfgPtr,
                                                  pclExtraDataPtr,
                                                  pclKeyPtr);
            break;
        case CHT3_PCL_KEY_TYPE_IPCL0_IPV6_KEY_TRIPLE_E:
            snetCht3IPclKeyBuildIpcl0IPv6TripleKey(devObjPtr, descrPtr,
                                                  curLookUpCfgPtr,
                                                  pclExtraDataPtr,
                                                  pclKeyPtr);
            break;
        case CHT3_PCL_KEY_TYPE_IPCL1_IPV6_KEY_TRIPLE_E:
            snetCht3IPclKeyBuildIpcl1IPv6TripleKey(devObjPtr, descrPtr,
                                                  curLookUpCfgPtr,
                                                  pclExtraDataPtr,
                                                  pclKeyPtr);
            break;
        case CHT3_PCL_KEY_TYPE_IPV6_DIP_KEY_SHORT_E:
            snetCht3IPclKeyBuildIpclIPv6DipShortKey(devObjPtr, descrPtr,
                                                  curLookUpCfgPtr,
                                                  pclExtraDataPtr,
                                                  pclKeyPtr);
            break;
       default:
            skernelFatalError("keyType: not valid mode[%d]",keyType);
            break;
    }

    return ;
}

/*******************************************************************************
* snetCht3IPclTriggeringCheck
*
* DESCRIPTION:
*        Check the enabling status of ingress PCL for the incoming frame.
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - pointer to frame data buffer Id
*
* OUTPUTS:
*       lookpuConfPtr   - pointer to lookup configuration.
*       extendedLookupModePtr - pointer to extended lookup mode.
*
* RETURN:
*       GT_TRUE     - PCL lookup triggered
*       GT_FALSE    - No PCL lookup triggered
*
* COMMENTS:
*         1. Check PCL is enabled.
*         2. Get Ingress PCL configuration entry.
*
*******************************************************************************/
static GT_BOOL snetCht3IPclTriggeringCheck
(
    IN SKERNEL_DEVICE_OBJECT                * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC      * descrPtr,
    OUT SNET_CHT3_PCL_LOOKUP_CONFIG_STC     * lookupConfPtr
)
{
    DECLARE_FUNC_NAME(snetCht3IPclTriggeringCheck);

    GT_U32  fieldVal;
    GT_U32  *portVlanCfgEntryPtr;   /* Port VLAN table entry pointer          */
    GT_U32  regAddress;             /* Register address                       */
    GT_U32 isRouted;                /* is Packet routed                       */
    GT_U32 lk0RoutedDisable;        /* lookup0 for Routed packets Disable     */
    GT_U32 lk1NotRoutedDisable;     /* lookup1 for not Routed packets Disable */

    /* check if the packet command is drop */
    __LOG(("check if the packet command is drop"));
    if (descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_HARD_DROP_E)
    {
        return GT_FALSE;
    }

    regAddress = SMEM_CHT_PORT_VLAN_QOS_CONFIG_TBL_MEM(devObjPtr,
                                                        descrPtr->localDevSrcPort);
    portVlanCfgEntryPtr = smemMemGet(devObjPtr, regAddress);

    /*  Enable Ingress Policy Engine for this port */
    __LOG(("Enable Ingress Policy Engine for this port"));
    fieldVal = SMEM_U32_GET_FIELD(portVlanCfgEntryPtr[0], 17, 1);
    if (fieldVal == 0)
    {
        return GT_FALSE;
    }

    /*  Enable Ingress Policy Engine */
    __LOG(("Enable Ingress Policy Engine"));
    smemRegFldGet(devObjPtr, SMEM_CHT_PCL_GLOBAL_REG(devObjPtr), 7, 1, &fieldVal);
    if (fieldVal == 1)
    {
        return GT_FALSE;
    }

    /* check is the PCL is enabled on port */
    __LOG(("check is the PCL is enabled on port"));
    if (descrPtr->policyOnPortEn == 0)
    {
        return GT_FALSE;
    }

    snetCht3IPclConfigPtrGet(devObjPtr, descrPtr, lookupConfPtr);

    /* different routed and not_routed packets behavior support */
    __LOG(("different routed and not_routed packets behavior support"));

    /* may be needed to add TTI passenger routed packet support */
    __LOG(("may be needed to add TTI passenger routed packet support"));
    isRouted = 0;
    if ((descrPtr->mac2me != 0) || (descrPtr->ipm != 0))
    {
        isRouted = 1;
    }

    if (isRouted)
    {
        /* disable lookup0_0 and lookup0_1 for routed packets */
        __LOG(("disable lookup0_0 and lookup0_1 for routed packets"));
        lk0RoutedDisable = SMEM_U32_GET_FIELD(portVlanCfgEntryPtr[1], 23, 1);

        if (lk0RoutedDisable)
        {
            lookupConfPtr[0].lookUpEn = 0;
            lookupConfPtr[1].lookUpEn = 0;
        }
    }
    else
    {
        /* disable lookup1 for not routed packets */
        __LOG(("disable lookup1 for not routed packets"));
        smemRegFldGet(
            devObjPtr,
            SMEM_CHT3_PCL_LOOKUP1_NOT_ROUTED_PKTS_REG(devObjPtr),
            1, 1, &lk1NotRoutedDisable);

        if (lk1NotRoutedDisable)
        {
            lookupConfPtr[2].lookUpEn = 0;
        }
    }

    return (lookupConfPtr[0].lookUpEn
            || lookupConfPtr[1].lookUpEn
            || lookupConfPtr[2].lookUpEn);
}

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
*
* OUTPUTS:
*       descrPtr     - pointer to updated frame data buffer Id
*
* RETURN:
*
* COMMENTS:
*       There are 2 PCL lookup hardware configuration setup - for LookUp0 and LookUp1.
*       Configuration for LookUp0_1 is inherited from LookUp0, the only difference
*       is toggle of LSB in the PCL-ID[9:0] field in the key.
*       Every lookup is performed independently and if succeed, overrides decision
*       of the previous lookup.
*
*******************************************************************************/
extern GT_VOID snetCht3IPcl
(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr
)
{
    DECLARE_FUNC_NAME(snetCht3IPcl);

    GT_U8  cycleNum;                              /* cycle number */

    SNET_CHT3_PCL_LOOKUP_CONFIG_STC lookUpCfg[3]; /* lookup configuration
                                                       Ingress PCL
                                                       configuration Table    */
    GT_BOOL                         doLookup;     /* perform TCAM lookup or not */
    CHT_PCL_EXTRA_PACKET_INFO_STC   pclExtraData;
    SNET_CHT_POLICY_KEY_STC         pclKey;       /* policy key structure */
    SNET_CHT_POLICY_KEY_STC        *pclKeyPtr = 0; /* policy key structure */
    GT_U32                          keyType;      /* type of tcam search key*/
    GT_U32                          matchIndex;   /* index to the match action */
    SNET_CHT3_PCL_ACTION_STC        actionData;   /* action to be performed */
    GT_U32                          lookUpClient; /* LookUp client number */
    GT_U32                          loopIdx;    /* loop array index */
    GT_BOOL                         isFirst = GT_TRUE;/* is first action apply */


    /* Get the PCL */
    __LOG(("Get the PCL"));
    doLookup = snetCht3IPclTriggeringCheck(
        devObjPtr, descrPtr, lookUpCfg);
    if (doLookup == GT_FALSE)
    {
        return ;
    }
    /* Get extra data from packet that will be used in PCL engine */
    __LOG(("Get extra data from packet that will be used in PCL engine"));
    snetChtPclSrvParseExtraData(devObjPtr, descrPtr, &pclExtraData);

    matchIndex = SNET_CHT_POLICY_NO_MATCH_INDEX_CNS;
    pclKey.devObjPtr = devObjPtr;
    /* performs the PCL engine for 3 lookups: LookUp0, LookUp0_1 and LookUp1 */
    __LOG(("performs the PCL engine for 3 lookups: LookUp0, LookUp0_1 and LookUp1"));
    for (loopIdx = 0; loopIdx < 3; loopIdx++)
    {
        cycleNum = loopIdx;

        /* Get the current configuration from the ingress PCL config. table*/
        __LOG(("Get the current configuration from the ingress PCL config. table"));
        if (lookUpCfg[cycleNum].lookUpEn == GT_FALSE)
        {
            continue;
        }

        lookUpClient = SNET_CHT3_CNC_PCL_LOOKUP_GET(loopIdx);

        if(pclKeyPtr && (cycleNum == 1))
        {
            /* Changes the LSB of the PCL-ID[9:0] field in the key, by toggling the bit */
            __LOG(("Changes the LSB of the PCL-ID[9:0] field in the key, by toggling the bit"));
            SNET_CHT3_TOGGLE_PCL_LSB(pclKeyPtr);
        }
        else
        {
            pclKeyPtr = &pclKey;
            /* Get from the configuration the key type and the key size */
            __LOG(("Get from the configuration the key type and the key size"));
            snetCht3PclGetKeyType(descrPtr, &lookUpCfg[cycleNum], &keyType);

            /* Create policy search key */
            __LOG(("Create policy search key"));
            snetCht3IPclCreateKey(devObjPtr, descrPtr, &lookUpCfg[cycleNum],
                                  &pclExtraData, (SNET_CHT3_PCL_KEY_TYPE_ENT)keyType, pclKeyPtr);
        }

        /* Search key in policy Tcam */
        __LOG(("Search key in policy Tcam"));
        snetChtPclSrvTcamLookUp(devObjPtr, devObjPtr->pclTcamInfoPtr, pclKeyPtr, &matchIndex);

        /* read and apply policy action */
        __LOG(("read and apply policy action"));
        if (matchIndex != SNET_CHT_POLICY_NO_MATCH_INDEX_CNS)
        {
           /* read the action from the pcl action table */
            __LOG(("read the action from the pcl action table"));
            snetCht3IPclActionGet(devObjPtr, descrPtr, matchIndex, &actionData);
            /* apply the action */
            __LOG(("apply the action"));
            snetCht3IPclActionApply(devObjPtr, descrPtr, &actionData , isFirst);
            isFirst = GT_FALSE;

            if(actionData.baseAction.matchCounterEn == GT_TRUE)
            {
                /* Ingress CNC Policy Clients */
                __LOG(("Ingress CNC Policy Clients"));
                snetCht3CncCount(devObjPtr, descrPtr, lookUpClient, actionData.baseAction.matchCounterIndex);
            }

            SNET_CHT3_ACTION_STOP_CHECK_MAC(&actionData);
        }
    }

    return;
}

/*******************************************************************************
*   snetCht3DataMaskKeyCompare
*
* DESCRIPTION:
*     Compare TCAM data with PCL key data
*
* INPUTS:
*    dataPtr    - TCAM data pointer
*    maskPtr    - TCAM mask pointer
*    keyPtr     - PCL key pointer
*    compBits   - number of bits to be compared
*
* OUTPUT:
*
* COMMENT:
*
*******************************************************************************/
static GT_BOOL snetCht3DataMaskKeyCompare
(
    IN GT_U8 * dataPtr,
    IN GT_U8 * maskPtr,
    IN GT_U8 * keyPtr,
    IN GT_U32 compBits,

    IN GT_U32   indexInTcam,    /* for log info */
    IN GT_U32   bankIndex,      /* for log info */
    IN GT_U32   wordIndex      /* for log info */
)
{

    GT_U32 index;                   /* Byte index */
    GT_U32 bytes = compBits / 8;    /* Number of bytes to be compared */
    GT_U32 bits = compBits % 8;     /* Number of reminded bits to be compared */
    GT_U32 result;              /* Compare result status */
    GT_U32  bitMask;                /* mask for needed bits */
    GT_U32  ii;
    GT_U32  globalBitIndexForLog;
    GT_U32  x,y,k;
    GT_U32  byteIndex;
    GT_U32  bitIndex;
    GT_BOOL noMatch = GT_FALSE;
    GT_U32  startBit = 0;
    /*GT_U32  bankWidth;*/
    GT_U32  firstIndexBits;

    if(compBits == 52)/*xcat2,lion2*/
    {
        firstIndexBits = 50;
    }
    else
    {
        firstIndexBits = compBits;
    }

    if (wordIndex == 0)
    {
        startBit = compBits - firstIndexBits;
    }
    else
    {
        startBit = 0;
    }

    /* allow first line to be with different length then other lines */
    /* bankWidth = (compBits * 3) + firstIndexBits; */ /* 206 bits */

    if(wordIndex > 0)
    {
        /* allow first line to be with different length then other lines */
        globalBitIndexForLog = (wordIndex-1) * compBits + firstIndexBits;
    }
    else
    {
        globalBitIndexForLog = 0;
    }

    /*globalBitIndexForLog += (bankIndex * bankWidth);*/


    for (index = 0; index < bytes; index++)
    {
        result = CH3_TCAM_X_Y_K_MATCH(dataPtr[index],maskPtr[index],keyPtr[index],0xff);
        if (result == 0)
        {
            noMatch = GT_TRUE;
            goto exit_func_lbl;
        }
    }

    if (bits)
    {
        bitMask = ((1 << bits) - 1);
        result = CH3_TCAM_X_Y_K_MATCH(dataPtr[index],maskPtr[index],keyPtr[index],bitMask);
        if (result == 0)
        {
            noMatch = GT_TRUE;
            goto exit_func_lbl;
        }
    }

exit_func_lbl:
    if(simLogIsOpen() && noMatch == GT_TRUE && (compBits > 8))
    {
        __LOG_NO_LOCATION_META_DATA(("indexInTcam[%d] wordIndex [%d] bankIndex[%d] \n",
            indexInTcam , wordIndex , bankIndex));

        __LOG_NO_LOCATION_META_DATA(("NOTE: in terms of CPSS the 'no match' cpssCurrentIndex [0x%x]([%d]) \n",
            indexInTcam,indexInTcam));

        /* analyze the bits that are 'NO match' */
        __LOG_NO_LOCATION_META_DATA(("analyze the GLOBAL bits that are 'NO match' \n"));
        __LOG_NO_LOCATION_META_DATA(("the global index is in terms of the FS that describes the TTI/PCL key \n"));

        /* the global index is in terms of the FS that describes the TTI/PCL key */

        __LOG_NO_LOCATION_META_DATA(("Non matched Bits:"));
        for(ii = startBit ; ii < compBits ; ii++ , globalBitIndexForLog++)
        {
            byteIndex = ii / 8;
            bitIndex = ii % 8;

            x = SMEM_U32_GET_FIELD(dataPtr[byteIndex],bitIndex,1);
            y = SMEM_U32_GET_FIELD(maskPtr[byteIndex],bitIndex,1);
            k = SMEM_U32_GET_FIELD(keyPtr[byteIndex] ,bitIndex,1);

            if(X_Y_K_FIND_NO_MATCH(x,y,k,1))
            {
                __LOG_NO_LOCATION_META_DATA(("%d,",
                    globalBitIndexForLog));
            }
        }
        __LOG_NO_LOCATION_META_DATA((". \n End of not matched Bits  \n\n"));
    }

    if (noMatch == GT_TRUE)
    {
        return GT_FALSE;
    }

    if(simLogIsOpen() && (compBits > 8))
    {
        /* explain the bits that where 'exact match' : */
        __LOG_NO_LOCATION_META_DATA(("indexInTcam[%d] wordIndex [%d] bankIndex[%d] \n",
            indexInTcam , wordIndex , bankIndex));

        __LOG_NO_LOCATION_META_DATA(("NOTE: in terms of CPSS the 'match' cpssCurrentIndex [0x%x]([%d]) \n",
            indexInTcam,indexInTcam));
        __LOG_NO_LOCATION_META_DATA(("the global index is in terms of the FS that describes the TTI/PCL key \n"));

        /* the global index is in terms of the FS that describes the TTI/PCL key */

        __LOG_NO_LOCATION_META_DATA(("exact matched Bits: (not don't care)"));
        for(ii = startBit ; ii < compBits ; ii++ , globalBitIndexForLog++)
        {
            byteIndex = ii / 8;
            bitIndex = ii % 8;

            x = SMEM_U32_GET_FIELD(dataPtr[byteIndex],bitIndex,1);
            y = SMEM_U32_GET_FIELD(maskPtr[byteIndex],bitIndex,1);

            if(x != y)/* indication that key match 0 or 1 ant not ignored ! */
            {
                __LOG_NO_LOCATION_META_DATA(("%d,",
                    globalBitIndexForLog));
            }
        }
        __LOG_NO_LOCATION_META_DATA((". \n End of matched Bits  \n\n"));

    }

    return GT_TRUE;
}

