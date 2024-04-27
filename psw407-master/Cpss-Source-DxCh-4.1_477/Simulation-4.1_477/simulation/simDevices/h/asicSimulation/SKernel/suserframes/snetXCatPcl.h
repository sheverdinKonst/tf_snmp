/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetXCatPcl.h
*
* DESCRIPTION:
*       XCat Asic Simulation.
*       Ingress Policy Engine processing for incoming frame.
*       header file.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 24 $
*
*******************************************************************************/
#ifndef __snetXCatPclh
#define __snetXCatPclh

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <asicSimulation/SKernel/suserframes/snetCheetah3EPcl.h>


typedef enum{
    SNET_UDB_CLIENT_TTI_E,
    SNET_UDB_CLIENT_IPCL_E,
    SNET_UDB_CLIENT_EPCL_E,

    SNET_UDB_CLIENT___LAST___E/* must be last*/

}SNET_UDB_CLIENT_ENT;


/******************************************************************************
 *  XCAT_IPCL_KEY_TYPE_ENT
 *
 *  DESCRIPTION : The key type used for PCL lookup
 *
********************************************************************************/
typedef enum
{
    /* L2  ingress, standard                    */
    XCAT_IPCL_KEY_TYPE_L2_KEY_SHORT_E = CHT3_PCL_KEY_TYPE_L2_KEY_SHORT_E,
    /* L2+IPv4/6+QOS - ingress, standard         */
    XCAT_IPCL_KEY_TYPE_L2_L3_KEY_SHORT_E = CHT3_PCL_KEY_TYPE_L2_L3_KEY_SHORT_E,
    /* IPv4+L4 - ingress , standard              */
    XCAT_IPCL_KEY_TYPE_L3_L4_KEY_SHORT_E = CHT3_PCL_KEY_TYPE_L3_L4_KEY_SHORT_E,
    /* L2+IPv4+L4 - ingress, extended            */
    XCAT_IPCL_KEY_TYPE_L2_L3_L4_KEY_LONG_E = CHT3_PCL_KEY_TYPE_L2_L3_L4_KEY_LONG_E,
    /* L2+IPv6 - ingress, extended               */
    XCAT_IPCL_KEY_TYPE_L2_IPV6_KEY_LONG_E = CHT3_PCL_KEY_TYPE_L2_IPV6_KEY_LONG_E,
    /* L4+IPv6 - ingress, extended               */
    XCAT_IPCL_KEY_TYPE_L4_IPV6_KEY_LONG_E = CHT3_PCL_KEY_TYPE_L4_IPV6_KEY_LONG_E,
    /* Port/VLAN + QoS - ingress, PV-ACL extended*/
    XCAT_IPCL_KEY_TYPE_IPCL0_IPV4_KEY_LONG_E = CHT3_PCL_KEY_TYPE_IPCL0_IPV4_KEY_LONG_E,
    /* ACL + QoS - ingress, R_ACL standard       */
    XCAT_IPCL_KEY_TYPE_IPCL1_IPV4_KEY_SHORT_E = CHT3_PCL_KEY_TYPE_IPCL1_IPV4_KEY_SHORT_E,
    /* ACL + QoS - ingress, R_ACL triple         */
    XCAT_IPCL_KEY_TYPE_IPCL0_IPV6_KEY_TRIPLE_E = CHT3_PCL_KEY_TYPE_IPCL0_IPV6_KEY_TRIPLE_E,
    /* ACL + QoS - ingress, R_ACL triple         */
    XCAT_IPCL_KEY_TYPE_IPCL1_IPV6_KEY_TRIPLE_E = CHT3_PCL_KEY_TYPE_IPCL1_IPV6_KEY_TRIPLE_E,
    /* lookup for implementing 98DX253/263/273 (Cheetah+) */
    XCAT_IPCL_KEY_TYPE_IPV6_DIP_KEY_SHORT_E = CHT3_PCL_KEY_TYPE_IPV6_DIP_KEY_SHORT_E,
    /* standard UDB styled key */
    XCAT_IPCL_KEY_TYPE_UDB_KEY_SHORT_E,
    /* extended UDB styled key */
    XCAT_IPCL_KEY_TYPE_UDB_KEY_LONG_E,

    XCAT_IPCL_KEY_TYPE_LAST_E,   /* xcat last */

    LION3_IPCL_KEY_TYPE_UBB_10B_E,
    LION3_IPCL_KEY_TYPE_UBB_20B_E,
    LION3_IPCL_KEY_TYPE_UBB_30B_E,
    LION3_IPCL_KEY_TYPE_UBB_40B_E,
    LION3_IPCL_KEY_TYPE_UBB_50B_E,
    LION3_IPCL_KEY_TYPE_UBB_60B_E,

    LION3_IPCL_KEY_TYPE_LAST_E /* lion3 last */

} SNET_XCAT_IPCL_KEY_TYPE_ENT;

/* check if ipcl_key_type is 'udb only'
 where : ipcl_key_type of type SNET_XCAT_IPCL_KEY_TYPE_ENT*/
#define LION3_IPCL_KEY_IS_UDB_MAC(dev,ipcl_key_type)                 \
    ((SMEM_CHT_IS_SIP5_GET(dev) &&                                   \
      (ipcl_key_type >= LION3_IPCL_KEY_TYPE_UBB_10B_E &&             \
       ipcl_key_type <= LION3_IPCL_KEY_TYPE_UBB_60B_E)) ? 1 : 0)

/******************************************************************************
 *  DXCH_IPCL_KEY_ENT
 *
 *  DESCRIPTION : The key # (numbers) for the IPCL lookup
 *
********************************************************************************/
typedef enum                                                             /*SIP4.0 Description                    |   SIP5.0 Description                */
{                                                                        /*============================================================================*/
    DXCH_IPCL_KEY_0_E,   /*XCAT_IPCL_KEY_TYPE_L2_KEY_SHORT_E*/           /*Standard (26B) L2                     |   Standard (30B) L2                 */
    DXCH_IPCL_KEY_1_E,   /*XCAT_IPCL_KEY_TYPE_L2_L3_KEY_SHORT_E*/        /*Standard (26B) L2+IPv4/v6 QoS         |   Standard (30B) L2+IPv4/v6 QoS     */
    DXCH_IPCL_KEY_2_E,   /*XCAT_IPCL_KEY_TYPE_L3_L4_KEY_SHORT_E*/        /*Standard (26B) IPv4+L4                |   Standard (30B) IPv4+L4            */
    DXCH_IPCL_KEY_3_E,   /*XCAT_IPCL_KEY_TYPE_UDB_KEY_SHORT_E*/          /*Standard (26B) 16 UDBs                |   Standard (30B) 16 UDBs            */
    DXCH_IPCL_KEY_4_E,   /*XCAT_IPCL_KEY_TYPE_L2_L3_L4_KEY_LONG_E*/      /*Extended (52B) L2+IPv4 + L4           |   Extended (60B) L2+IPv4 + L4       */
    DXCH_IPCL_KEY_5_E,   /*XCAT_IPCL_KEY_TYPE_L2_IPV6_KEY_LONG_E*/       /*Extended (52B) L2+IPv6                |   Extended (60B) L2+IPv6            */
    DXCH_IPCL_KEY_6_E,   /*XCAT_IPCL_KEY_TYPE_L4_IPV6_KEY_LONG_E*/       /*Extended (52B) L4+IPv6                |   Extended (60B) L4+IPv6            */
    DXCH_IPCL_KEY_7_E,   /*XCAT_IPCL_KEY_TYPE_UDB_KEY_LONG_E*/           /*Extended (52B) 16 UDBs                |   Extended (60B) 16 UDBs            */
    DXCH_IPCL_KEY_8_E,   /*XCAT_IPCL_KEY_TYPE_IPCL0_IPV4_KEY_LONG_E*/    /*Extended (52B) Port/VLAN+QoS+IPv4     |   Extended (60B) Port/VLAN+QoS+IPv4 */
    DXCH_IPCL_KEY_9_E,   /*XCAT_IPCL_KEY_TYPE_IPCL0_IPV6_KEY_TRIPLE_E*/  /*Ultra (78B) Port/VLAN+QoS+IPv6        |   Ultra (80B) Port/VLAN+QoS+IPv6    */
    DXCH_IPCL_KEY_10_E,  /*XCAT_IPCL_KEY_TYPE_IPCL1_IPV4_KEY_SHORT_E*/   /*Standard (26B) Routed ACL+QoS+IPv4    |   Standard (30B) Routed ACL+QoS+IPv4*/
    DXCH_IPCL_KEY_11_E,  /*XCAT_IPCL_KEY_TYPE_IPCL1_IPV6_KEY_TRIPLE_E*/  /*Ultra (78B) Routed ACL+QoS+IPv6       |   Ultra (80B) Routed ACL+QoS+IPv6   */
    DXCH_IPCL_KEY_12_E,  /*XCAT_IPCL_KEY_TYPE_IPV6_DIP_KEY_SHORT_E*/     /*Standard (26B) IPv6 DIP               |   Standard (30B) IPv6 DIP           */

    DXCH_IPCL_KEY_LAST_E/*must be last*/
} DXCH_IPCL_KEY_ENT;

/******************************************************************************
 *  DXCH_EPCL_KEY_ENT
 *
 *  DESCRIPTION : The key # (numbers) for the EPCL lookup
 *
********************************************************************************/
typedef enum                                                              /*SIP4.0 Description                   |   SIP5.0 Description                */
{                                                                         /*===========================================================================*/
    DXCH_EPCL_KEY_0_E,   /*CHT3_EPCL_KEY_TYPE_L2_KEY_SHORT_E,    */       /*  Standard (26B) L2                  | Standard (30B) L2                   */
    DXCH_EPCL_KEY_1_E,   /*CHT3_EPCL_KEY_TYPE_L2_L3_KEY_SHORT_E, */       /*  Standard (26B) L2+IPv4/v6 QoS      | Standard (30B) L2+IPv4/v6 QoS       */
    DXCH_EPCL_KEY_2_E,   /*CHT3_EPCL_KEY_TYPE_L3_L4_KEY_SHORT_E ,*/       /*  Standard (26B) IPv4+L4             | Standard (30B) IPv4+L4              */
    DXCH_EPCL_KEY_3_E,   /*CHT3_EPCL_KEY_TYPE_L2_L3_L4_KEY_LONG_E,*/      /*  Extended (52B) L2+IPv4+L4          | Extended (60B) L2+IPv4+L4           */
    DXCH_EPCL_KEY_4_E,   /*CHT3_EPCL_KEY_TYPE_L2_IPV6_KEY_LONG_E ,*/      /*  Extended (52B) L2+IPv6             | Extended (60B) L2+IPv6              */
    DXCH_EPCL_KEY_5_E,   /*CHT3_EPCL_KEY_TYPE_L4_IPV6_KEY_LONG_E ,*/      /*  Extended (52B) L4+IPv6             | Extended (60B) L4+IPv6              */
    DXCH_EPCL_KEY_6_E,   /*CHT3_EPCL_KEY_TYPE_L4_IPV4_NON_IP_KEY_LONG_E*/ /*  Extended (52B) RACL/VACL IPv4      | Extended (60B) RACL/VACL IPv4       */
    DXCH_EPCL_KEY_7_E,   /*CHT3_EPCL_KEY_TYPE_L4_IPV6_KEY_TRIPLE_E,*/     /*  Ultra (78B) RACL/VACL IPv6         | Ultra (80B) RACL/VACL IPv6          */

    DXCH_EPCL_KEY_LAST_E,/* xcat last*/

    LION3_EPCL_KEY_TYPE_UBB_10B_E,
    LION3_EPCL_KEY_TYPE_UBB_20B_E,
    LION3_EPCL_KEY_TYPE_UBB_30B_E,
    LION3_EPCL_KEY_TYPE_UBB_40B_E,
    LION3_EPCL_KEY_TYPE_UBB_50B_E,
    LION3_EPCL_KEY_TYPE_UBB_60B_E,

    LION3_EPCL_KEY_TYPE_LAST_E /* lion3 last */

} DXCH_EPCL_KEY_ENT;

/* check if epcl_key_type is 'udb only'
 where : epcl_key_type of type DXCH_EPCL_KEY_ENT*/
#define LION3_EPCL_KEY_IS_UDB_MAC(dev,epcl_key_type)                 \
    ((SMEM_CHT_IS_SIP5_GET(dev) &&                                   \
      (epcl_key_type >= LION3_EPCL_KEY_TYPE_UBB_10B_E &&             \
       epcl_key_type <= LION3_EPCL_KEY_TYPE_UBB_60B_E)) ? 1 : 0)

/********************************************************************************
 *  XCAT_PCL_KEY_FIELDS_ID_ENT
 *
 *  DESCRIPTION : Enum of the ingress PCL TCAM fields
 *
********************************************************************************/
typedef enum
{
    /* common fields */
    XCAT_PCL_KEY_FIELDS_ID_PCL_ID_E                             ,   /* Bit(s) 0-9    */
    XCAT_PCL_KEY_FIELDS_ID_PORT_LIST_7_0_E                      ,   /* Bit(s) 0-7    */
    XCAT_PCL_KEY_FIELDS_ID_MAC_TO_ME_E                          ,   /* Bit(s) 10-10   */
    XCAT_PCL_KEY_FIELDS_ID_SRC_PORT_E                           ,   /* Bit(s) 11-16   */
    XCAT_PCL_KEY_FIELDS_ID_PORT_LIST_13_8_E                     ,   /* Bit(s) 11-16   */
    XCAT_PCL_KEY_FIELDS_ID_IS_TAGGED_E                          ,   /* Bit(s) 17-17   */
    XCAT_PCL_KEY_FIELDS_ID_VID_E                                ,   /* Bit(s) 18-29   */
    XCAT_PCL_KEY_FIELDS_ID_UP_E                                 ,   /* Bit(s) 30-32   */
    XCAT_PCL_KEY_FIELDS_ID_QOS_PROFILE_E                        ,   /* Bit(s) 33-39   */
    XCAT_PCL_KEY_FIELDS_ID_IS_IPV4_E                            ,   /* Bit(s) 40-40   */
    XCAT_PCL_KEY_FIELDS_ID_IS_IP_E                              ,   /* Bit(s) 41-41   */
    XCAT_PCL_KEY_FIELDS_ID_DSCP_E                               ,   /* Bit(s) 42-47   */
    XCAT_PCL_KEY_FIELDS_ID_IP_PROTOCOL_E                        ,   /* Bit(s) 48-55   */
    XCAT_PCL_KEY_FIELDS_ID_STD_KEY_MAC_SA_23_0_E                ,   /* Bit(s) 80-103  */
    XCAT_PCL_KEY_FIELDS_ID_STD_KEY_IS_ARP_E                     ,   /* Bit(s) 105-105 */
    XCAT_PCL_KEY_FIELDS_ID_USER_DEFINED_VALID_E                 ,   /* Bit(s) 106-106 */
    XCAT_PCL_KEY_FIELDS_ID_IS_L2_VALID_E                        ,   /* Bit(s) 107-107 */
    XCAT_PCL_KEY_FIELDS_ID_STD_KEY_MAC_SA_47_24_E               ,   /* Bit(s) 120-143 */
    XCAT_PCL_KEY_FIELDS_ID_STD_KEY_MAC_DA_E                     ,   /* Bit(s) 144-191 */
    XCAT_PCL_KEY_FIELDS_ID_PORT_LIST_27_14_E                    ,   /* Bit(s) 192-205 */

    /* reserved fields */
    XCAT_PCL_KEY_FIELDS_ID_RESERVED_55_E                        ,   /* Bit(s) 55-55   */
    XCAT_PCL_KEY_FIELDS_ID_RESERVED_58_63_E                     ,   /* Bit(s) 58-63   */
    XCAT_PCL_KEY_FIELDS_ID_RESERVED_59_63_E                     ,   /* Bit(s) 59-63   */
    XCAT_PCL_KEY_FIELDS_ID_RESERVED_104_E                       ,   /* Bit(s) 104-104 */
    XCAT_PCL_KEY_FIELDS_ID_RESERVED_109_111_XCAT2_110_111_E     ,   /* Bit(s) 109-111 */
    XCAT_PCL_KEY_FIELDS_ID_RESERVED_110_111_E                   ,   /* Bit(s) 110-111 */
    XCAT_PCL_KEY_FIELDS_ID_RESERVED_111_E                       ,   /* Bit(s) 111-111 */
    XCAT_PCL_KEY_FIELDS_ID_RESERVED_188_E                       ,   /* Bit(s) 188-188 */
    XCAT_PCL_KEY_FIELDS_ID_RESERVED_190_E                       ,   /* Bit(s) 190-190 */
    XCAT_PCL_KEY_FIELDS_ID_RESERVED_192_247_E                   ,   /* Bit(s) 192-247 */
    XCAT_PCL_KEY_FIELDS_ID_RESERVED_313_319_E                   ,   /* Bit(s) 313-319 */
    XCAT_PCL_KEY_FIELDS_ID_RESERVED_475_E                       ,   /* Bit(s) 475-475 */

    /* Key#0 Standard (24B) L2 - Ingress */
    XCAT_PCL_KEY_FIELDS_ID_KEY0_ETHER_TYPE_DSAP_SSAP_E          ,   /* Bit(s) 42-57   */
    XCAT_PCL_KEY_FIELDS_ID_KEY0_UDB_15_E                        ,   /* Bit(s) 64-71   */
    XCAT_PCL_KEY_FIELDS_ID_KEY0_UDB_16_E                        ,   /* Bit(s) 72-79   */
    XCAT_PCL_KEY_FIELDS_ID_KEY0_ENCAP_TYPE_E                    ,   /* Bit(s) 108-108, 108-109 xCat2 */
    XCAT_PCL_KEY_FIELDS_ID_KEY0_UDB_17_E                        ,   /* Bit(s) 112-119 */

    /* Key#1 Standard (24B) L2+IPv4/v6 QoS - Ingress */
    XCAT_PCL_KEY_FIELDS_ID_UDB_18_E                             ,   /* Bit(s) 56-63   */
    XCAT_PCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_3_2_E                 ,   /* Bit(s) 64-79   */
    XCAT_PCL_KEY_FIELDS_ID_IS_L4_VALID_E                        ,   /* Bit(s) 104-104 */
    XCAT_PCL_KEY_FIELDS_ID_IPV4_FRAGMENT_E                      ,   /* Bit(s) 108-108 */
    XCAT_PCL_KEY_FIELDS_ID_IP_HEADER_OK_E                       ,   /* Bit(s) 109-109 */
    XCAT_PCL_KEY_FIELDS_ID_IS_IPV6_EH_EXISTS_E                  ,   /* Bit(s) 110-110 */
    XCAT_PCL_KEY_FIELDS_ID_LONG_IS_IPV6_EH_HOP_BY_HOP_E         ,   /* Bit(s) 111-111 */
    XCAT_PCL_KEY_FIELDS_ID_UDB_19_E                             ,   /* Bit(s) 112-119 */

    /* Key#2 Standard (24B) IPv4+L4 - Ingress */
    XCAT_PCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_13_1_0_E              ,   /* Bit(s) 80-103   */
    XCAT_PCL_KEY_FIELDS_ID_IS_BC_E                              ,   /* Bit(s) 110-110 */
    XCAT_PCL_KEY_FIELDS_ID_SIP_31_0_E                           ,   /* Bit(s) 112-143 */
    XCAT_PCL_KEY_FIELDS_ID_DIP_31_0_E                           ,   /* Bit(s) 144-175 */
    XCAT_PCL_KEY_FIELDS_ID_UDB_21_KEY2_E                       ,   /* Bit(s) 176-183 */
    XCAT_PCL_KEY_FIELDS_ID_UDB_22_KEY2_E                       ,   /* Bit(s) 184-191 */

    /* Key#3 Key3 - Standard (24B) 16 UDBs - Ingress */
    XCAT_PCL_KEY_FIELDS_ID_IS_PRIORITY_TAGGED_E                 ,   /* Bit(s) 33-33   */
    XCAT_PCL_KEY_FIELDS_ID_L3_OFFSET_INVALID_E                  ,   /* Bit(s) 34-34   */
    XCAT_PCL_KEY_FIELDS_ID_L4_PROTOCOL_OR_VALID_E               ,   /* Bit(s) 35-36   */
    XCAT_PCL_KEY_FIELDS_ID_APPLICABLE_FLOW_SUB_TEMPLATE_1_0_E   ,   /* Bit(s) 37-38   */
    XCAT_PCL_KEY_FIELDS_ID_IP_HEADER_OK_KEY3_E                 ,   /* Bit(s) 39-39   */
    XCAT_PCL_KEY_FIELDS_ID_UDB_0_2_5_KEY3_E                    ,   /* Bit(s) 48-79   */
    XCAT_PCL_KEY_FIELDS_ID_MAC_DA_TYPE_E                        ,   /* Bit(s) 80-81   */
    XCAT_PCL_KEY_FIELDS_ID_L4_OFFSET_INVALID_E                  ,   /* Bit(s) 82-82   */
    XCAT_PCL_KEY_FIELDS_ID_APPLICABLE_FLOW_SUB_TEMPLATE_7_2_E   ,   /* Bit(s) 83-88   */
    XCAT_PCL_KEY_FIELDS_ID_L2_ENCAP_0_KEY3_E                   ,   /* Bit(s) 89-90   */
    XCAT_PCL_KEY_FIELDS_ID_IS_IPV6_EH_HOP_BY_HOP_KEY3_E        ,   /* Bit(s) 91-91   */
    XCAT_PCL_KEY_FIELDS_ID_IS_IPV6_EH_EXISTS_KEY3_E            ,   /* Bit(s) 92-92   */
    XCAT_PCL_KEY_FIELDS_ID_USER_DEFINED_VALID_KEY3_E           ,   /* Bit(s) 94-94   */
    XCAT_PCL_KEY_FIELDS_ID_IS_L2_VALID_KEY3_E                  ,   /* Bit(s) 95-95   */
    XCAT_PCL_KEY_FIELDS_ID_KEY3_UDB_3_4_6_15_E                 ,   /* Bit(s) 96-191  */

    /* Key#4 Extended (48B) L2+IPv4+L4 - Ingress */
    XCAT_PCL_KEY_FIELDS_ID_IS_IPV6_E                            ,   /* Bit(s) 40-40   */
    XCAT_PCL_KEY_FIELDS_ID_UDB_5_E                              ,   /* Bit(s) 56-63   */
    XCAT_PCL_KEY_FIELDS_ID_ENCAP_TYPE_LONG_E                    ,   /* Bit(s) 105-105 */
    XCAT_PCL_KEY_FIELDS_ID_ETHERTYPE_E                          ,   /* Bit(s) 176-191 */
    XCAT_PCL_KEY_FIELDS_ID_MAC_SA_E                             ,   /* Bit(s) 248-295 */
    XCAT_PCL_KEY_FIELDS_ID_MAC_DA_KEY4_E                       ,   /* Bit(s) 296-343 */
    XCAT_PCL_KEY_FIELDS_ID_UDB_1_E                              ,   /* Bit(s) 344-351 */
    XCAT_PCL_KEY_FIELDS_ID_UDB_0_4_3_2_E                        ,   /* Bit(s) 352-383 */
        /* xCat2 modified fields - Key#4 */
    XCAT2_PCL_KEY_FIELDS_ID_ENCAP_1_KEY4_E                     ,   /* Bit(s) 110-110 xCat2 */

    /* Key#5 Extended (48B) L2+IPv6  - Ingress */
    XCAT_PCL_KEY_FIELDS_ID_UDB_11_E                             ,   /* Bit(s) 56-63 */
    XCAT_PCL_KEY_FIELDS_ID_IS_IPV6_EH_EXISTS_LONG_E             ,   /* Bit(s) 105-105 */
    XCAT_PCL_KEY_FIELDS_ID_LONG_IS_IPV6_EH_HOP_BY_HOP_KEY5_E   ,   /* Bit(s) 108-108 */
    XCAT_PCL_KEY_FIELDS_ID_SIP_31_0_KEY5_E                      ,   /* Bit(s) 112-143 */
    XCAT_PCL_KEY_FIELDS_ID_SIP_63_32_KEY5_E                     ,   /* Bit(s) 144-175 */
    XCAT_PCL_KEY_FIELDS_ID_SIP_95_64_KEY5_E                     ,   /* Bit(s) 176-207 */
    XCAT_PCL_KEY_FIELDS_ID_SIP_127_96_KEY5_E                    ,   /* Bit(s) 208-239 */
    XCAT_PCL_KEY_FIELDS_ID_DIP_127_120_E                        ,   /* Bit(s) 240-247 */
    XCAT_PCL_KEY_FIELDS_ID_UDB_6_E                              ,   /* Bit(s) 344-351 */
    XCAT_PCL_KEY_FIELDS_ID_UDB_10_9_8_7_E                       ,   /* Bit(s) 352-383 */
    /* xCat2 modified fields - Key#5 */
    XCAT2_PCL_KEY_FIELDS_ID_SIP_79_64_KEY5_E                     ,   /* Bit(s) 176-191 xCat2 */
    XCAT2_PCL_KEY_FIELDS_ID_SIP_95_80_KEY5_E                     ,   /* Bit(s) 206-221 xCat2 */

    /* Key#6 Extended (48B) L4+IPv6  - Ingress */
    XCAT_PCL_KEY_FIELDS_ID_UDB_14_E                             ,   /* Bit(s) 56-63 */
    XCAT_PCL_KEY_FIELDS_ID_SIP_63_32_KEY6_E                    ,   /* Bit(s) 176-207 */
    XCAT_PCL_KEY_FIELDS_ID_SIP_95_64_KEY6_E                    ,   /* Bit(s) 208-239 */
    XCAT_PCL_KEY_FIELDS_ID_SIP_127_96_KEY6_E                   ,   /* Bit(s) 240-271 */
    XCAT_PCL_KEY_FIELDS_ID_DIP_127_120_KEY6_E                  ,   /* Bit(s) 272-279 */
    XCAT_PCL_KEY_FIELDS_ID_DIP_63_32_E                          ,   /* Bit(s) 280-311 */
    XCAT_PCL_KEY_FIELDS_ID_DIP_95_64_E                          ,   /* Bit(s) 312-343 */
    XCAT_PCL_KEY_FIELDS_ID_DIP_119_96_E                         ,   /* Bit(s) 344-367 */
    XCAT_PCL_KEY_FIELDS_ID_UDB_12_KEY6_E                       ,   /* Bit(s) 368-375 */
    XCAT_PCL_KEY_FIELDS_ID_UDB_13_E                             ,   /* Bit(s) 376-383 */
            /* xCat2 modified fields - Key#6 */
    XCAT2_PCL_KEY_FIELDS_ID_SIP_47_32_KEY6_E                   ,   /* Bit(s) 176-191 */
    XCAT2_PCL_KEY_FIELDS_ID_SIP_63_48_KEY6_E                   ,   /* Bit(s) 206-221 */

    /* Key#7    IPCL Key7 - Extended (48B) 16 UDBs - Ingress */
    XCAT_PCL_KEY_FIELDS_ID_UDB_1_2_5_KEY7_E                    ,   /* Bit(s) 56-79   */
    XCAT_PCL_KEY_FIELDS_ID_UDB_3_4_6_9_KEY7_E                  ,   /* Bit(s) 96-143  */
    XCAT_PCL_KEY_FIELDS_ID_SIP_31_0_KEY7_E                     ,   /* Bit(s) 144-175 */
    XCAT_PCL_KEY_FIELDS_ID_SIP_63_32_KEY7_E                    ,   /* Bit(s) 176-207 */
    XCAT_PCL_KEY_FIELDS_ID_SIP_95_64_KEY7_E                    ,   /* Bit(s) 208-239 */
    XCAT_PCL_KEY_FIELDS_ID_SIP_127_96_KEY7_E                   ,   /* Bit(s) 240-271 */
    XCAT_PCL_KEY_FIELDS_ID_MAC_SA_KEY7_E                       ,   /* Bit(s) 176-223 */
    XCAT_PCL_KEY_FIELDS_ID_MAC_DA_KEY7_E                       ,   /* Bit(s) 224-271 */
    XCAT_PCL_KEY_FIELDS_ID_DIP_127_112_KEY7_E                  ,   /* Bit(s) 272-287 */
    XCAT_PCL_KEY_FIELDS_ID_DIP_31_0_KEY7_E                     ,   /* Bit(s) 288-319 */
    XCAT_PCL_KEY_FIELDS_ID_UDB_0_15__10_KEY7_E                 ,   /* Bit(s) 320-375 */
            /* xCat2 modified fields - Key#7 */
    XCAT2_PCL_KEY_FIELDS_ID_MAC_SA_15_0_KEY7_E                 ,   /* Bit(s) 176-191 */
    XCAT2_PCL_KEY_FIELDS_ID_MAC_SA_47_16_KEY7_E                ,   /* Bit(s) 206-237 */
    XCAT2_PCL_KEY_FIELDS_ID_SIP_47_32_KEY7_E                   ,   /* Bit(s) 176-191 */
    XCAT2_PCL_KEY_FIELDS_ID_SIP_63_48_KEY7_E                   ,   /* Bit(s) 206-221 */

    /* Key#8 Extended (48B) Port/VLAN+QoS+IPv4  - Ingress */
    XCAT_PCL_KEY_FIELDS_PCKT_TOS_2_7_E                          ,   /* Bit(s) 33-38   */
    XCAT_PCL_KEY_FIELDS_ID_IS_ARP_KEY8_E                       ,   /* Bit(s) 39-39   */
    XCAT_PCL_KEY_FIELDS_ID_IS_BC_KEY8_E                        ,   /* Bit(s) 40-40   */
    XCAT_PCL_KEY_FIELDS_ID_IP_PROTOCOL_KEY8_E                  ,   /* Bit(s) 42-49   */
    XCAT_PCL_KEY_FIELDS_PCKT_TOS_0_E                            ,   /* Bit(s) 50-50   */
    XCAT_PCL_KEY_FIELDS_IP_FRAGMENTED_E                         ,   /* Bit(s) 51-52   */
    XCAT_PCL_KEY_FIELDS_IP_HEADER_INFO_E                        ,   /* Bit(s) 53-54   */
    XCAT_PCL_KEY_FIELDS_ID_IS_L2_VALID_KEY8_E                  ,   /* Bit(s) 55-55   */
    XCAT_PCL_KEY_FIELDS_ID_IS_L4_VALID_KEY8_E                  ,   /* Bit(s) 56-56   */
    XCAT_PCL_KEY_FIELDS_ID_USER_DEFINED_VALID_KEY8_E           ,   /* Bit(s) 57-57   */
    XCAT_PCL_KEY_FIELDS_PCKT_TOS_1_E                            ,   /* Bit(s) 58-58   */
    XCAT_PCL_KEY_FIELDS_ID_UDB_13_KEY8_E                       ,   /* Bit(s) 104-111 */
    XCAT_PCL_KEY_FIELDS_ID_MAC_SA_KEY8_E                       ,   /* Bit(s) 176-223 */
    XCAT_PCL_KEY_FIELDS_ID_MAC_DA_KEY8_E                       ,   /* Bit(s) 224-271 */
    XCAT_PCL_KEY_FIELDS_ID_ETHERTYPE_KEY8_E                    ,   /* Bit(s) 272-287 */
    XCAT_PCL_KEY_FIELDS_ID_L2_ENCAP_0_E                         ,   /* Bit(s) 288-288 */
    XCAT_PCL_KEY_FIELDS_ID_UDB14_E                              ,   /* Bit(s) 289-296 */
    XCAT_PCL_KEY_FIELDS_ID_UDB_16_15_E                          ,   /* Bit(s) 297-312 */

    /* Key#9 Ultra (72B) Port/VLAN+QoS+IPv6 */
    XCAT_PCL_KEY_FIELDS_ID_L2_ENCAP_0_KEY9_E                   ,   /* Bit(s) 39-39   */
    XCAT_PCL_KEY_FIELDS_ID_IS_IPV6_EH_HOP_BY_HOP_E              ,   /* Bit(s) 51-51   */
    XCAT_PCL_KEY_FIELDS_ID_IS_IPV6_EH_EXISTS_KEY9_E            ,   /* Bit(s) 52-52   */
    XCAT_PCL_KEY_FIELDS_ID_IS_ND_E                              ,   /* Bit(s) 53-53   */
    XCAT_PCL_KEY_FIELDS_IP_HEADER_OK_E                          ,   /* Bit(s) 54-54   */
    XCAT_PCL_KEY_FIELDS_ID_UDB_12_E                             ,   /* Bit(s) 104-111 */
    XCAT_PCL_KEY_FIELDS_ID_SIP_63_32_KEY9_E                    ,   /* Bit(s) 288-319 */
    XCAT_PCL_KEY_FIELDS_ID_SIP_95_64_KEY9_E                    ,   /* Bit(s) 320-351 */
    XCAT_PCL_KEY_FIELDS_ID_SIP_127_96_KEY9_E                   ,   /* Bit(s) 352-383 */
    XCAT_PCL_KEY_FIELDS_ID_DIP_63_32_KEY9_E                    ,   /* Bit(s) 384-415 */
    XCAT_PCL_KEY_FIELDS_ID_DIP_95_64_KEY9_E                    ,   /* Bit(s) 416-447 */
    XCAT_PCL_KEY_FIELDS_ID_DIP_127_96_KEY9_E                   ,   /* Bit(s) 448-479 */

    /* Key#10 Standard (24B) Routed ACL+QoS+IPv4 */
    XCAT_PCL_KEY_FIELDS_PCKT_TYPE_E                             ,   /* Bit(s) 39-41   */
    XCAT_PCL_KEY_FIELDS_IP_PCKT_LEN_5_0_E                       ,   /* Bit(s) 57-62   */
    XCAT_PCL_KEY_FIELDS_PCKT_TOS_1_KEY10_E                     ,   /* Bit(s) 63-63   */
    XCAT_PCL_KEY_FIELDS_ID_UDB_5_KEY10_E                       ,   /* Bit(s) 104-111 */
    XCAT_PCL_KEY_FIELDS_IP_PCKT_LEN_13_6_E                      ,   /* Bit(s) 176-183 */
    XCAT_PCL_KEY_FIELDS_ID_TTL_0_2_E                            ,   /* Bit(s) 184-186 */
    XCAT_PCL_KEY_FIELDS_ID_TTL_3_7_E                            ,   /* Bit(s) 187-191 */

    /* Key#11 Ultra (72B) Routed ACL+QoS+IPv6 */
    XCAT_PCL_KEY_FIELDS_IP_PCKT_LEN_3_0_E                       ,   /* Bit(s) 51-54   */
    XCAT_PCL_KEY_FIELDS_ID_UDB_18_KEY11_E                      ,   /* Bit(s) 104-111 */
    XCAT_PCL_KEY_FIELDS_ID_UDB_19_KEY11_E                      ,   /* Bit(s) 176-183 */
    XCAT_PCL_KEY_FIELDS_ID_UDB_17_22_21_20_E                    ,   /* Bit(s) 184-215 */
    XCAT_PCL_KEY_FIELDS_PCKT_TOS_1_KEY11_E                     ,   /* Bit(s) 216-216 */
    XCAT_PCL_KEY_FIELDS_ID_IS_ND_KEY11_E                       ,   /* Bit(s) 217-217 */
    XCAT_PCL_KEY_FIELDS_ID_IPV6_HEADER_FLOW_LBL_E               ,   /* Bit(s) 218-237 */
    XCAT_PCL_KEY_FIELDS_ID_IS_IPV6_LINK_LOCAL_E                 ,   /* Bit(s) 238-238 */
    XCAT_PCL_KEY_FIELDS_ID_MPLS_OUTER_LBL_E                     ,   /* Bit(s) 239-258 */
    XCAT_PCL_KEY_FIELDS_ID_MPLS_OUTER_LBL_EXP_E                 ,   /* Bit(s) 259-261 */
    XCAT_PCL_KEY_FIELDS_ID_MPLS_OUTER_LBL_S_BIT_E               ,   /* Bit(s) 262-262 */
    XCAT_PCL_KEY_FIELDS_IP_PCKT_LEN_11_4_E                      ,   /* Bit(s) 263-270 */
    XCAT_PCL_KEY_FIELDS_ID_TTL_E                                ,   /* Bit(s) 271-278 */
    XCAT_PCL_KEY_FIELDS_ID_SIP_63_32_KEY11_E                   ,   /* Bit(s) 279-310 */
    XCAT_PCL_KEY_FIELDS_ID_SIP_95_64_KEY11_E                   ,   /* Bit(s) 311-342 */
    XCAT_PCL_KEY_FIELDS_ID_SIP_127_96_KEY11_E                  ,   /* Bit(s) 343-374 */
    XCAT_PCL_KEY_FIELDS_ID_DIP_63_32_KEY11_E                   ,   /* Bit(s) 375-406 */
    XCAT_PCL_KEY_FIELDS_ID_DIP_95_64_KEY11_E                   ,   /* Bit(s) 407-438 */
    XCAT_PCL_KEY_FIELDS_ID_DIP_127_96_KEY11_E                  ,   /* Bit(s) 439-470 */
    XCAT_PCL_KEY_FIELDS_IP_PCKT_LEN_15_12_E                     ,   /* Bit(s) 471-474 */
    XCAT_PCL_KEY_FIELDS_ID_IS_IPV6_MLD_E                        ,   /* Bit(s) 476-476 */
    XCAT_PCL_KEY_FIELDS_ID_IS_IPV6_EH_HOP_BY_HOP_KEY11_E       ,   /* Bit(s) 477-477 */
    XCAT_PCL_KEY_FIELDS_ID_IS_IPV6_EH_EXISTS_KEY11_E           ,   /* Bit(s) 478-478 */
    XCAT_PCL_KEY_FIELDS_ID_IP_HEADER_OK_KEY11_E                ,   /* Bit(s) 479-479 */

    /* Key#12 Standard (24B) IPv6 DIP */
    XCAT_PCL_KEY_FIELDS_PCKT_DSCP_E                             ,   /* Bit(s) 50-50   */
    XCAT_PCL_KEY_FIELDS_ID_DIP_15_0_E                           ,   /* Bit(s) 57-72   */
    XCAT_PCL_KEY_FIELDS_ID_IS_ARP_KEY12_E                      ,   /* Bit(s) 73-73   */
    XCAT_PCL_KEY_FIELDS_ID_IS_IPV6_EH_EXISTS_KEY12_E           ,   /* Bit(s) 74-74   */
    XCAT_PCL_KEY_FIELDS_ID_DIP_31_16_E                          ,   /* Bit(s) 75-90   */
    XCAT_PCL_KEY_FIELDS_ID_LONG_IS_IPV6_EH_HOP_BY_HOP_KEY12_E  ,   /* Bit(s) 91-91   */
    XCAT_PCL_KEY_FIELDS_ID_DIP_63_32_KEY12_E                   ,   /* Bit(s) 92-123 */
    XCAT_PCL_KEY_FIELDS_ID_DIP_95_64_KEY12_E                   ,   /* Bit(s) 124-155 */
    XCAT_PCL_KEY_FIELDS_ID_DIP_127_96_KEY12_E                  ,   /* Bit(s) 156-187 */
    XCAT_PCL_KEY_FIELDS_ID_IS_L2_VALID_KEY12_E                 ,   /* Bit(s) 189-189 */
    XCAT_PCL_KEY_FIELDS_IP_HEADER_OK_KEY12_E                   ,   /* Bit(s) 191-191 */

}XCAT_PCL_KEY_FIELDS_ID_ENT;


/* build unique name for field in key:
    device : lion3
    key# : 5
    field name : UP0

    will generate name : lion3_key5_UP0_E
*/
#define KEY_FIELD_NAME_BUILD(devName , keyId , startBit,endBit,fieldName)  \
    devName##_key##keyId##_Bits_##startBit##_##endBit##_##fieldName##_E

/* field names for many keys for lion3*/
#define LION3_KEY_FIELD_NAME_BUILD(startBit,endBit,fieldName)  \
    KEY_FIELD_NAME_BUILD(LION3,_Common,startBit,endBit,fieldName)

/* field names for key 0 for lion3*/
#define LION3_KEY0_FIELD_NAME_BUILD(startBit,endBit,fieldName)  \
    KEY_FIELD_NAME_BUILD(LION3,0,startBit,endBit,fieldName)
/* field names for key 1 for lion3*/
#define LION3_KEY1_FIELD_NAME_BUILD(startBit,endBit,fieldName)  \
    KEY_FIELD_NAME_BUILD(LION3,1,startBit,endBit,fieldName)
/* field names for key 2 for lion3*/
#define LION3_KEY2_FIELD_NAME_BUILD(startBit,endBit,fieldName)  \
    KEY_FIELD_NAME_BUILD(LION3,2,startBit,endBit,fieldName)
/* field names for key 3 for lion3*/
#define LION3_KEY3_FIELD_NAME_BUILD(startBit,endBit,fieldName)  \
    KEY_FIELD_NAME_BUILD(LION3,3,startBit,endBit,fieldName)
/* field names for key 4 for lion3*/
#define LION3_KEY4_FIELD_NAME_BUILD(startBit,endBit,fieldName)  \
    KEY_FIELD_NAME_BUILD(LION3,4,startBit,endBit,fieldName)
/* field names for key 5 for lion3*/
#define LION3_KEY5_FIELD_NAME_BUILD(startBit,endBit,fieldName)  \
    KEY_FIELD_NAME_BUILD(LION3,5,startBit,endBit,fieldName)
/* field names for key 6 for lion3*/
#define LION3_KEY6_FIELD_NAME_BUILD(startBit,endBit,fieldName)  \
    KEY_FIELD_NAME_BUILD(LION3,6,startBit,endBit,fieldName)
/* field names for key 7 for lion3*/
#define LION3_KEY7_FIELD_NAME_BUILD(startBit,endBit,fieldName)  \
    KEY_FIELD_NAME_BUILD(LION3,7,startBit,endBit,fieldName)
/* field names for key 8 for lion3*/
#define LION3_KEY8_FIELD_NAME_BUILD(startBit,endBit,fieldName)  \
    KEY_FIELD_NAME_BUILD(LION3,8,startBit,endBit,fieldName)
/* field names for key 9 for lion3*/
#define LION3_KEY9_FIELD_NAME_BUILD(startBit,endBit,fieldName)  \
    KEY_FIELD_NAME_BUILD(LION3,9,startBit,endBit,fieldName)
/* field names for key 10 for lion3*/
#define LION3_KEY10_FIELD_NAME_BUILD(startBit,endBit,fieldName)  \
    KEY_FIELD_NAME_BUILD(LION3,10,startBit,endBit,fieldName)
/* field names for key 11 for lion3*/
#define LION3_KEY11_FIELD_NAME_BUILD(startBit,endBit,fieldName)  \
    KEY_FIELD_NAME_BUILD(LION3,11,startBit,endBit,fieldName)
/* field names for key 12 for lion3*/
#define LION3_KEY12_FIELD_NAME_BUILD(startBit,endBit,fieldName)  \
    KEY_FIELD_NAME_BUILD(LION3,12,startBit,endBit,fieldName)

/* field names for 60B Key for extra 10 bytes for lion3*/
#define LION3_KEY_60B_EXTRA_10B_FIELD_NAME_BUILD(startBit,endBit,fieldName)  \
    KEY_FIELD_NAME_BUILD(LION3,60B_EXTRA_10B,startBit,endBit,fieldName)

/********************************************************************************
 *  LION3_PCL_KEY_FIELDS_ID_ENT
 *
 *  DESCRIPTION : Lion3 : Enum of the ingress PCL TCAM fields
 *
********************************************************************************/
typedef enum
{
    /* common to keys (without UDBs) */
     LION3_KEY_FIELD_NAME_BUILD( 0   ,   7   ,PCL_ID_7_0_OR_port_list_7_0      )
    ,LION3_KEY_FIELD_NAME_BUILD( 8   ,   9   ,PCL_ID_9_8                       )
    ,LION3_KEY_FIELD_NAME_BUILD( 10  ,  10   ,MAC2ME                           )
    ,LION3_KEY_FIELD_NAME_BUILD( 11  ,  16   ,SrcPort_5_0_OR_port_list_13_8    )
    ,LION3_KEY_FIELD_NAME_BUILD( 17  ,  17   ,IsTagged                         )
    ,LION3_KEY_FIELD_NAME_BUILD( 18  ,  29   ,VID0_OR_eVLAN_11_0               )
    ,LION3_KEY_FIELD_NAME_BUILD( 30  ,  32   ,UP0                              )
    ,LION3_KEY_FIELD_NAME_BUILD( 33  ,  39   ,QoS_Profile                      )
    ,LION3_KEY_FIELD_NAME_BUILD( 40  ,  40   ,IsIPv4_OR_IsFCoE                 )
    ,LION3_KEY_FIELD_NAME_BUILD( 41  ,  41   ,IsIP                             )
    ,LION3_KEY_FIELD_NAME_BUILD( 80  , 103   ,MAC_SA_23_0                      )
    ,LION3_KEY_FIELD_NAME_BUILD( 105 , 105   ,IsARP                            )
    ,LION3_KEY_FIELD_NAME_BUILD( 107 , 107   ,Is_L2_Valid                      )
    ,LION3_KEY_FIELD_NAME_BUILD( 120 , 143   ,MAC_SA_47_24                     )
    ,LION3_KEY_FIELD_NAME_BUILD( 144 , 191   ,MAC_DA                           )
    ,LION3_KEY_FIELD_NAME_BUILD( 192 , 193   ,port_list_15_14_OR_SrcPort_7_6   )
    ,LION3_KEY_FIELD_NAME_BUILD( 194 , 205   ,port_list_27_16_OR_2b0_SrcDev_9_0)
    ,LION3_KEY_FIELD_NAME_BUILD( 42  ,  47   ,Packet_DSCP                      )
    ,LION3_KEY_FIELD_NAME_BUILD( 48  ,  55   ,IpProtocol                       )
    ,LION3_KEY_FIELD_NAME_BUILD( 64  ,  79   ,L4_Byte_Offset_3_2_OR_L4_Byte_Offset_5_4)
    ,LION3_KEY_FIELD_NAME_BUILD( 104 , 104   ,Is_L4_Valid                      )
    ,LION3_KEY_FIELD_NAME_BUILD( 108 , 108   ,IPv4_fragmented                  )
    ,LION3_KEY_FIELD_NAME_BUILD( 109 , 109   ,IP_Hdr_OK                        )
    /* key0 extra (without UDBs) */
    ,LION3_KEY0_FIELD_NAME_BUILD(42  ,  57   ,Ethertype                        )
    ,LION3_KEY0_FIELD_NAME_BUILD(58  ,  63   ,Tag1_VID_5_0                     )
    ,LION3_KEY0_FIELD_NAME_BUILD(104 , 104   ,UP1_0                            )
    ,LION3_KEY0_FIELD_NAME_BUILD(108 , 108   ,L2_Encapsulation_0               )
    ,LION3_KEY0_FIELD_NAME_BUILD(109 , 109   ,L2_Encapsulation_1               )
    ,LION3_KEY0_FIELD_NAME_BUILD(110 , 111   ,UP1_2_1                          )
    /* key1 extra (without UDBs) */
    ,LION3_KEY1_FIELD_NAME_BUILD(110 , 110   ,Is_IPv6_EH_Exist                 )
    ,LION3_KEY1_FIELD_NAME_BUILD(111 , 111   ,Is_IPv6_EH_Hop_By_Hop            )
    /* key2 extra (without UDBs) */
    ,LION3_KEY2_FIELD_NAME_BUILD(80  ,  87   ,L4_Byte_Offsets_0                )
    ,LION3_KEY2_FIELD_NAME_BUILD(88  ,  95   ,L4_Byte_Offsets_1                )
    ,LION3_KEY2_FIELD_NAME_BUILD(96  , 103   ,L4_Byte_Offsets_13               )
    ,LION3_KEY2_FIELD_NAME_BUILD(110 , 110   ,Is_BC                            )
    ,LION3_KEY2_FIELD_NAME_BUILD(112 , 143   ,SIP_31_0_OR_S_ID                 )
    ,LION3_KEY2_FIELD_NAME_BUILD(144 , 175   ,DIP_31_0_OR_D_ID                 )
    /* key3 extra (without UDBs) */
    ,LION3_KEY3_FIELD_NAME_BUILD(33  , 33    ,Packet_Tagging_1                 )
    ,LION3_KEY3_FIELD_NAME_BUILD(34  , 34    ,L3_Offset_Invalid                )
    ,LION3_KEY3_FIELD_NAME_BUILD(35  , 36    ,L4_Protocol_or_Valid             )
    ,LION3_KEY3_FIELD_NAME_BUILD(37  , 38    ,Applicable_Flow_Sub_Template_1_0 )
    ,LION3_KEY3_FIELD_NAME_BUILD(39  , 39    ,IP_Hdr_OK                        )
    ,LION3_KEY3_FIELD_NAME_BUILD(80  , 81    ,MacDaType                        )
    ,LION3_KEY3_FIELD_NAME_BUILD(82  , 82    ,L4_Offset_Invalid                )
    ,LION3_KEY3_FIELD_NAME_BUILD(83  , 88    ,Applicable_Flow_Sub_Template_7_2 )
    ,LION3_KEY3_FIELD_NAME_BUILD(89  , 90    ,L2_Encapsulation                 )
    ,LION3_KEY3_FIELD_NAME_BUILD(91  , 91    ,Is_IPv6_EH_Hop_By_Hop            )
    ,LION3_KEY3_FIELD_NAME_BUILD(92  , 92    ,Is_IPv6_EH_Exist                 )
    ,LION3_KEY3_FIELD_NAME_BUILD(95  , 95    ,Is_L2_Valid                      )
    /* key4 extra (without UDBs) */
    ,LION3_KEY4_FIELD_NAME_BUILD(40  ,  40   ,IsIPv6_OR_IsFCoE                 )
    ,LION3_KEY4_FIELD_NAME_BUILD(64  ,  79   ,L4_Byte_Offset_3_2               )
    ,LION3_KEY4_FIELD_NAME_BUILD(105 , 105   ,L2_Encapsulation_0               )
    ,LION3_KEY4_FIELD_NAME_BUILD(110 , 110   ,L2_Encapsulation_1               )
    ,LION3_KEY4_FIELD_NAME_BUILD(176 , 191   ,Ethertype                        )
    ,LION3_KEY4_FIELD_NAME_BUILD(208 , 210   ,UP1                              )
    ,LION3_KEY4_FIELD_NAME_BUILD(264 , 311   ,MAC_SA                           )
    ,LION3_KEY4_FIELD_NAME_BUILD(312 , 359   ,MAC_DA                           )
    ,LION3_KEY4_FIELD_NAME_BUILD(400 , 400   ,Tag1_Exist                       )
    ,LION3_KEY4_FIELD_NAME_BUILD(401 , 412   ,Tag1_VID                         )
    ,LION3_KEY4_FIELD_NAME_BUILD(413 , 413   ,Tag1_CFI                         )
    /* key5 extra (without UDBs) */
    ,LION3_KEY5_FIELD_NAME_BUILD(105 , 105   ,Is_IPv6_EH_Exist                 )
    ,LION3_KEY5_FIELD_NAME_BUILD(108 , 108   ,Is_IPv6_EH_Hop_By_Hop            )
    ,LION3_KEY5_FIELD_NAME_BUILD(110 , 111   ,UP1_1_0                          )
    ,LION3_KEY5_FIELD_NAME_BUILD(144 , 175   ,SIP_63_32                        )
    ,LION3_KEY5_FIELD_NAME_BUILD(176 , 191   ,SIP_79_64                        )
    ,LION3_KEY5_FIELD_NAME_BUILD(208 , 223   ,SIP_95_80                        )
    ,LION3_KEY5_FIELD_NAME_BUILD(224 , 255   ,SIP_127_96                       )
    ,LION3_KEY5_FIELD_NAME_BUILD(256 , 263   ,DIP_127_120                      )
    ,LION3_KEY5_FIELD_NAME_BUILD(413 , 413   ,UP1_2                            )
    /* key6 extra (without UDBs) */
    ,LION3_KEY6_FIELD_NAME_BUILD(176 ,  191  ,SIP_47_32                        )
    ,LION3_KEY6_FIELD_NAME_BUILD(208 ,  223  ,SIP_63_48                        )
    ,LION3_KEY6_FIELD_NAME_BUILD(224 ,  255  ,SIP_95_64                        )
    ,LION3_KEY6_FIELD_NAME_BUILD(256 ,  287  ,SIP_127_96                       )
    ,LION3_KEY6_FIELD_NAME_BUILD(288 ,  295  ,DIP_127_120                      )
    ,LION3_KEY6_FIELD_NAME_BUILD(296 ,  327  ,DIP_63_32                        )
    ,LION3_KEY6_FIELD_NAME_BUILD(328 ,  359  ,DIP_95_64                        )
    ,LION3_KEY6_FIELD_NAME_BUILD(360 ,  383  ,DIP_119_96                       )
    /* key7 extra (without UDBs) */
    ,LION3_KEY7_FIELD_NAME_BUILD(17  ,  17   ,Packet_Tagging_0                 )
    ,LION3_KEY7_FIELD_NAME_BUILD(144 , 175   ,SIP_31_0_OR_S_ID                 )
    ,LION3_KEY7_FIELD_NAME_BUILD(176 , 191   ,MAC_SA_15_0                      )
    ,LION3_KEY7_FIELD_NAME_BUILD(208 , 239   ,MAC_SA_47_32                     )
    ,LION3_KEY7_FIELD_NAME_BUILD(240 , 287   ,MAC_DA                           )
    ,LION3_KEY7_FIELD_NAME_BUILD(288 , 303   ,DIP_127_112                      )
    ,LION3_KEY7_FIELD_NAME_BUILD(304 , 335   ,DIP_31_0_OR_D_ID                 )
    ,LION3_KEY7_FIELD_NAME_BUILD(397 , 399   ,UP1                              )
    /* key8 extra (without UDBs) */
    ,LION3_KEY8_FIELD_NAME_BUILD(33  ,  38   ,PacketTOS_OR_DSCP_7_2            )
    ,LION3_KEY8_FIELD_NAME_BUILD(39  ,  39   ,Is_ARP                           )
    ,LION3_KEY8_FIELD_NAME_BUILD(40  ,  40   ,Is_BC                            )
    ,LION3_KEY8_FIELD_NAME_BUILD(42  ,  49   ,IpProtocol                       )
    ,LION3_KEY8_FIELD_NAME_BUILD(50  ,  50   ,PacketTOS_OR_DSCP_0              )
    ,LION3_KEY8_FIELD_NAME_BUILD(51  ,  52   ,IP_Fragmented                    )
    ,LION3_KEY8_FIELD_NAME_BUILD(53  ,  54   ,IP_Header_Info                   )
    ,LION3_KEY8_FIELD_NAME_BUILD(55  ,  55   ,Is_L2_Valid                      )
    ,LION3_KEY8_FIELD_NAME_BUILD(56  ,  56   ,Is_L4_Valid                      )
    ,LION3_KEY8_FIELD_NAME_BUILD(58  ,  58   ,PacketTOS_DSCP_1                 )
    ,LION3_KEY8_FIELD_NAME_BUILD(288 , 303   ,Ethertype                        )
    ,LION3_KEY8_FIELD_NAME_BUILD(304 , 304   ,L2_Encapsulation_0               )
    ,LION3_KEY8_FIELD_NAME_BUILD(329 , 329   ,L2_Encapsulation_1               )
    ,LION3_KEY8_FIELD_NAME_BUILD(333 , 335   ,UP1                              )
    ,LION3_KEY8_FIELD_NAME_BUILD(336 , 336   ,Tag1_Exist                       )
    ,LION3_KEY8_FIELD_NAME_BUILD(337 , 348   ,Tag1_VID                         )
    ,LION3_KEY8_FIELD_NAME_BUILD(349 , 349   ,Tag1_CFI                         )
    /* key9 extra (without UDBs) */
    ,LION3_KEY9_FIELD_NAME_BUILD(39  ,  39   ,L2_Encapsulation_0               )
    ,LION3_KEY9_FIELD_NAME_BUILD(51  ,  51   ,Is_IPv6_EH_Hop_By_Hop            )
    ,LION3_KEY9_FIELD_NAME_BUILD(52  ,  52   ,Is_IPv6_EH_Exist                 )
    ,LION3_KEY9_FIELD_NAME_BUILD(53  ,  53   ,Is_ND                            )
    ,LION3_KEY9_FIELD_NAME_BUILD(54  ,  54   ,IP_Hdr_OK                        )
    ,LION3_KEY9_FIELD_NAME_BUILD(59  ,  59   ,L2_Encapsulation_1               )
    ,LION3_KEY9_FIELD_NAME_BUILD(60  ,  63   ,SRCePort_OR_Trunk_3_0            )
    ,LION3_KEY9_FIELD_NAME_BUILD(304 , 335   ,SIP_63_32                        )
    ,LION3_KEY9_FIELD_NAME_BUILD(336 , 367   ,SIP_95_64                        )
    ,LION3_KEY9_FIELD_NAME_BUILD(368 , 399   ,SIP_127_96                       )
    ,LION3_KEY9_FIELD_NAME_BUILD(400 , 407   ,SRCePort_OR_Trunk_11_4           )
    ,LION3_KEY9_FIELD_NAME_BUILD(408 , 408   ,OrigSrcIsTrunk                   )
    ,LION3_KEY9_FIELD_NAME_BUILD(409 , 409   ,eVLAN_12                         )
    ,LION3_KEY9_FIELD_NAME_BUILD(411 , 413   ,UP1                              )
    ,LION3_KEY9_FIELD_NAME_BUILD(416 , 447   ,DIP_63_32                        )
    ,LION3_KEY9_FIELD_NAME_BUILD(448 , 479   ,DIP_95_64                        )
    ,LION3_KEY9_FIELD_NAME_BUILD(480 , 511   ,DIP_127_96                       )
    ,LION3_KEY9_FIELD_NAME_BUILD(608 , 608   ,Tag1_Exist                       )
    ,LION3_KEY9_FIELD_NAME_BUILD(609 , 620   ,Tag1_VID                         )
    ,LION3_KEY9_FIELD_NAME_BUILD(621 , 621   ,Tag1_CFI                         )
    /* key10 extra (without UDBs) */
    ,LION3_KEY10_FIELD_NAME_BUILD(30  ,  32   ,UP0_OR_EXP0                     )
    ,LION3_KEY10_FIELD_NAME_BUILD(39  ,  41   ,Pkt_type                        )
    ,LION3_KEY10_FIELD_NAME_BUILD(57  ,  62   ,IP_Packet_Length_5_0            )
    ,LION3_KEY10_FIELD_NAME_BUILD(63  ,  63   ,PacketTOS_DSCP_1                )
    ,LION3_KEY10_FIELD_NAME_BUILD(176 , 183   ,IP_Packet_Length_13_6           )
    ,LION3_KEY10_FIELD_NAME_BUILD(184 , 191   ,TTL                             )
    /* key11 extra (without UDBs) */
    ,LION3_KEY11_FIELD_NAME_BUILD(51  ,  54   ,IP_Packet_Length_3_0            )
    ,LION3_KEY11_FIELD_NAME_BUILD(58  ,  63   ,SRCePort_OR_Trunk_5_0           )
    ,LION3_KEY11_FIELD_NAME_BUILD(232 , 232   ,PacketTOS_DSCP_1                )
    ,LION3_KEY11_FIELD_NAME_BUILD(233 , 233   ,Is_ND                           )
    ,LION3_KEY11_FIELD_NAME_BUILD(234 , 253   ,IPv6_Flow_Label                 )
    ,LION3_KEY11_FIELD_NAME_BUILD(254 , 254   ,Is_IPv6_Link_local              )
    ,LION3_KEY11_FIELD_NAME_BUILD(255 , 274   ,MPLS_Outer_Label                )
    ,LION3_KEY11_FIELD_NAME_BUILD(275 , 277   ,MPLS_Outer_Label_EXP            )
    ,LION3_KEY11_FIELD_NAME_BUILD(278 , 278   ,MPLS_Outer_Label_S_Bit          )
    ,LION3_KEY11_FIELD_NAME_BUILD(279 , 286   ,IP_Packet_Length_11_4           )
    ,LION3_KEY11_FIELD_NAME_BUILD(287 , 294   ,TTL                             )
    ,LION3_KEY11_FIELD_NAME_BUILD(295 , 326   ,SIP_63_32                       )
    ,LION3_KEY11_FIELD_NAME_BUILD(327 , 358   ,SIP_95_64                       )
    ,LION3_KEY11_FIELD_NAME_BUILD(359 , 390   ,SIP_127_96                      )
    ,LION3_KEY11_FIELD_NAME_BUILD(391 , 398   ,DIP_39_32                       )
    ,LION3_KEY11_FIELD_NAME_BUILD(399 , 405   ,SRCePort_OR_Trunk_12_6          )
    ,LION3_KEY11_FIELD_NAME_BUILD(407 , 407   ,OrigSrcIsTrunk                  )
    ,LION3_KEY11_FIELD_NAME_BUILD(408 , 408   ,eVLAN_12                        )
    ,LION3_KEY11_FIELD_NAME_BUILD(409 , 409   ,Tag1_Src_Tagged                 )
    ,LION3_KEY11_FIELD_NAME_BUILD(410 , 412   ,UP1                             )
    ,LION3_KEY11_FIELD_NAME_BUILD(416 , 447   ,DIP_63_40                       )
    ,LION3_KEY11_FIELD_NAME_BUILD(448 , 479   ,DIP_95_64                       )
    ,LION3_KEY11_FIELD_NAME_BUILD(480 , 503   ,DIP_127_96                      )
    ,LION3_KEY11_FIELD_NAME_BUILD(504 , 507   ,IP_Packet_Length_15_12          )
    ,LION3_KEY11_FIELD_NAME_BUILD(509 , 509   ,Is_IPv6_MLD                     )
    ,LION3_KEY11_FIELD_NAME_BUILD(510 , 510   ,Is_IPv6_EH_Hop_By_Hop           )
    ,LION3_KEY11_FIELD_NAME_BUILD(511 , 511   ,Is_IPv6_EH_Exist                )
    ,LION3_KEY11_FIELD_NAME_BUILD(512 , 512   ,IP_Hdr_OK                       )
    /* key12 extra (without UDBs) */
    ,LION3_KEY12_FIELD_NAME_BUILD(50  ,  55   ,DSCP                            )
    ,LION3_KEY12_FIELD_NAME_BUILD(57  ,  72   ,DIP_15_0                        )
    ,LION3_KEY12_FIELD_NAME_BUILD(73  ,  73   ,IsARP                           )
    ,LION3_KEY12_FIELD_NAME_BUILD(74  ,  74   ,Ipv6_EH_exist                   )
    ,LION3_KEY12_FIELD_NAME_BUILD(75  ,  90   ,DIP_31_16                       )
    ,LION3_KEY12_FIELD_NAME_BUILD(92  , 123   ,DIP_63_32                       )
    ,LION3_KEY12_FIELD_NAME_BUILD(124 , 155   ,DIP_95_64                       )
    ,LION3_KEY12_FIELD_NAME_BUILD(156 , 187   ,DIP_127_96                      )
    ,LION3_KEY12_FIELD_NAME_BUILD(189 , 189   ,isL2Valid                       )
    ,LION3_KEY12_FIELD_NAME_BUILD(191 , 191   ,IP_Header_OK                    )

    /* field names for 60B Key for extra 10 bytes for lion3*/
    ,LION3_KEY_60B_EXTRA_10B_FIELD_NAME_BUILD(400 , 400 , UDB_Valid     )
    ,LION3_KEY_60B_EXTRA_10B_FIELD_NAME_BUILD(401 , 410 , PCL_ID        )
    ,LION3_KEY_60B_EXTRA_10B_FIELD_NAME_BUILD(411 , 423 , eVLAN         )
    ,LION3_KEY_60B_EXTRA_10B_FIELD_NAME_BUILD(424 , 436 , Source_ePort_OR_Trunk_id  )
    ,LION3_KEY_60B_EXTRA_10B_FIELD_NAME_BUILD(437 , 437 , Src_Dev_Is_Own)
    ,LION3_KEY_60B_EXTRA_10B_FIELD_NAME_BUILD(438 , 449 , Tag1_VID      )
    ,LION3_KEY_60B_EXTRA_10B_FIELD_NAME_BUILD(450 , 452 , Tag1_UP       )
    ,LION3_KEY_60B_EXTRA_10B_FIELD_NAME_BUILD(453 , 453 , Mac_To_Me     )
    ,LION3_KEY_60B_EXTRA_10B_FIELD_NAME_BUILD(454 , 463 , QoS_Profile   )
    ,LION3_KEY_60B_EXTRA_10B_FIELD_NAME_BUILD(464 , 475 , Flow_ID       )
    ,LION3_KEY_60B_EXTRA_10B_FIELD_NAME_BUILD(476 , 479 , Reserved      )

    ,LION3_PCL_KEY_FIELDS_ID___LAST__E/*must be last*/
}LION3_PCL_KEY_FIELDS_ID_ENT;





/*  EPCL
    build unique name for field in key:
    device : lion3
    key# : 5
    field name : UP0

    will generate name : lion3_EPCL_key5_UP0_E
*/
#define EPCL_KEY_FIELD_NAME_BUILD(devName , keyId , startBit,endBit,fieldName)  \
    devName##_EPCL_key##keyId##_Bits_##startBit##_##endBit##_##fieldName##_E

/* field names for many keys for lion3*/
#define LION3_EPCL_KEY_FIELD_NAME_BUILD(startBit,endBit,fieldName)  \
    EPCL_KEY_FIELD_NAME_BUILD(LION3,_Common,startBit,endBit,fieldName)

/* field names for key 0 for lion3*/
#define LION3_EPCL_KEY0_FIELD_NAME_BUILD(startBit,endBit,fieldName)  \
    EPCL_KEY_FIELD_NAME_BUILD(LION3,0,startBit,endBit,fieldName)
/* field names for key 1 for lion3*/
#define LION3_EPCL_KEY1_FIELD_NAME_BUILD(startBit,endBit,fieldName)  \
    EPCL_KEY_FIELD_NAME_BUILD(LION3,1,startBit,endBit,fieldName)
/* field names for key 2 for lion3*/
#define LION3_EPCL_KEY2_FIELD_NAME_BUILD(startBit,endBit,fieldName)  \
    EPCL_KEY_FIELD_NAME_BUILD(LION3,2,startBit,endBit,fieldName)
/* field names for key 3 for lion3*/
#define LION3_EPCL_KEY3_FIELD_NAME_BUILD(startBit,endBit,fieldName)  \
    EPCL_KEY_FIELD_NAME_BUILD(LION3,3,startBit,endBit,fieldName)
/* field names for key 4 for lion3*/
#define LION3_EPCL_KEY4_FIELD_NAME_BUILD(startBit,endBit,fieldName)  \
    EPCL_KEY_FIELD_NAME_BUILD(LION3,4,startBit,endBit,fieldName)
/* field names for key 5 for lion3*/
#define LION3_EPCL_KEY5_FIELD_NAME_BUILD(startBit,endBit,fieldName)  \
    EPCL_KEY_FIELD_NAME_BUILD(LION3,5,startBit,endBit,fieldName)
/* field names for key 6 for lion3*/
#define LION3_EPCL_KEY6_FIELD_NAME_BUILD(startBit,endBit,fieldName)  \
    EPCL_KEY_FIELD_NAME_BUILD(LION3,6,startBit,endBit,fieldName)
/* field names for key 7 for lion3*/
#define LION3_EPCL_KEY7_FIELD_NAME_BUILD(startBit,endBit,fieldName)  \
    EPCL_KEY_FIELD_NAME_BUILD(LION3,7,startBit,endBit,fieldName)

/* field names for 60B Key for extra 10 bytes for lion3*/
#define LION3_EPCL_KEY_60B_EXTRA_10B_FIELD_NAME_BUILD(startBit,endBit,fieldName)  \
    EPCL_KEY_FIELD_NAME_BUILD(LION3,60B_EXTRA_10B,startBit,endBit,fieldName)

/********************************************************************************
 *  LION3_EPCL_KEY_FIELDS_ID_ENT
 *
 *  DESCRIPTION : Lion3 : Enum of the EPCL TCAM fields
 *
********************************************************************************/
typedef enum
{
    /* common to keys (without UDBs) */
     LION3_EPCL_KEY_FIELD_NAME_BUILD( 0 ,  7 ,PCL_ID_7_0_OR_port_list_7_0                )
    ,LION3_EPCL_KEY_FIELD_NAME_BUILD( 8 ,  9 ,PCL_ID_9_8                                 )
    ,LION3_EPCL_KEY_FIELD_NAME_BUILD(10 , 15 ,Src_Port_5_0                               )
    ,LION3_EPCL_KEY_FIELD_NAME_BUILD(16 , 16 ,Is_Tagged                                  )
    ,LION3_EPCL_KEY_FIELD_NAME_BUILD(17 , 28 ,VID0_eVLAN_11_0                            )
    ,LION3_EPCL_KEY_FIELD_NAME_BUILD(29 , 31 ,UP0                                        )
    ,LION3_EPCL_KEY_FIELD_NAME_BUILD(32 , 38 ,Data_Pkt_QoS_Profile_6_0_OR_port_list_13_8 )
    ,LION3_EPCL_KEY_FIELD_NAME_BUILD(32 , 38 ,Ctrl_To_CPU_CPU_CODE_6_0                   )
    ,LION3_EPCL_KEY_FIELD_NAME_BUILD(32 , 38 ,Ctrl_From_CPU_TC_2_0_DP_1_0                )
    ,LION3_EPCL_KEY_FIELD_NAME_BUILD(39 , 39 ,Is_IPv4                                    )
    ,LION3_EPCL_KEY_FIELD_NAME_BUILD(39 , 39 ,Is_IPv6                                    )
    ,LION3_EPCL_KEY_FIELD_NAME_BUILD(40 , 40 ,Is_IP                                      )
    ,LION3_EPCL_KEY_FIELD_NAME_BUILD(41 , 48 ,Ip_Protocol                                )
    ,LION3_EPCL_KEY_FIELD_NAME_BUILD(49 , 54 ,Packet_Dscp                                )
    ,LION3_EPCL_KEY_FIELD_NAME_BUILD(55 , 55 ,Is_L4_Valid                                )
    ,LION3_EPCL_KEY_FIELD_NAME_BUILD(56 , 63 ,L4_Header_Byte_3_OR_5                      )
    ,LION3_EPCL_KEY_FIELD_NAME_BUILD(64 , 71 ,L4_Header_Byte_2_OR_4                      )
    ,LION3_EPCL_KEY_FIELD_NAME_BUILD(72 , 72 ,Is_ARP                                     )
    ,LION3_EPCL_KEY_FIELD_NAME_BUILD(72 , 79 ,L4_Header_Byte_13                          )
    ,LION3_EPCL_KEY_FIELD_NAME_BUILD(80 , 87 ,L4_Header_Byte_1                           )
    ,LION3_EPCL_KEY_FIELD_NAME_BUILD(88 , 95 ,L4_Header_Byte_0                           )
    ,LION3_EPCL_KEY_FIELD_NAME_BUILD(96 ,127 ,SIP_31_0                                   )

    ,LION3_EPCL_KEY_FIELD_NAME_BUILD(74 , 80 ,Data_Pkt_OR_CtrlToCPU_Src_Trunk_ID_6_0     )
    ,LION3_EPCL_KEY_FIELD_NAME_BUILD(74 , 80 ,Data_Pkt_Src_Dev_4_0                       )
    ,LION3_EPCL_KEY_FIELD_NAME_BUILD(74 , 80 ,Ctrl_To_CPU_Src_Trg_Src_Dev_4_0            )
    ,LION3_EPCL_KEY_FIELD_NAME_BUILD(74 , 80 ,From_CPU_Egress_Filter_En_Src_Dev_4_0      )
    ,LION3_EPCL_KEY_FIELD_NAME_BUILD(74 , 80 ,To_Analyzer_Rx_Sniff_Src_Dev_4_0           )
    ,LION3_EPCL_KEY_FIELD_NAME_BUILD(81 , 81 ,Data_Pkt_OR_Ctrl_To_CPU_Src_Is_Trunk       )
    ,LION3_EPCL_KEY_FIELD_NAME_BUILD(82 , 86 ,Source_ID_4_0                              )
    ,LION3_EPCL_KEY_FIELD_NAME_BUILD(87 , 87 ,Is_Routed                                  )
    ,LION3_EPCL_KEY_FIELD_NAME_BUILD(88 , 88 ,Data_Pkt_Is_Unknown                        )
    ,LION3_EPCL_KEY_FIELD_NAME_BUILD(88 , 88 ,Ctrl_To_CPU_CPU_CODE_7                     )
    ,LION3_EPCL_KEY_FIELD_NAME_BUILD(88 , 88 ,From_CPU_Is_Unknown                        )
    ,LION3_EPCL_KEY_FIELD_NAME_BUILD(89 , 89 ,Is_L2_Valid                                )

    ,LION3_EPCL_KEY_FIELD_NAME_BUILD(187,188 ,Packet_Type_1_0                            )
    ,LION3_EPCL_KEY_FIELD_NAME_BUILD(190,190 ,is_VIDX                                    )
    ,LION3_EPCL_KEY_FIELD_NAME_BUILD(191,191 ,Tag_1_Exist                                )
    ,LION3_EPCL_KEY_FIELD_NAME_BUILD(192,205 ,port_list_27_14                            )

    /* key0 fields */
    ,LION3_EPCL_KEY0_FIELD_NAME_BUILD(41 , 41 ,UP1_2                    )
    ,LION3_EPCL_KEY0_FIELD_NAME_BUILD(42 , 57 ,Ether_Type_or_Dsap_Ssap  )
    ,LION3_EPCL_KEY0_FIELD_NAME_BUILD(58 , 69 ,Tag1_VID                 )
    ,LION3_EPCL_KEY0_FIELD_NAME_BUILD(70 , 71 ,UP1_1_0                  )
    ,LION3_EPCL_KEY0_FIELD_NAME_BUILD(90 , 90 ,L2_Encap_Type            )
    ,LION3_EPCL_KEY0_FIELD_NAME_BUILD(91 ,138 ,MAC_SA                   )
    ,LION3_EPCL_KEY0_FIELD_NAME_BUILD(139,186 ,MAC_DA                   )
    ,LION3_EPCL_KEY0_FIELD_NAME_BUILD(189,189 ,Tag_1_CFI                )
    ,LION3_EPCL_KEY0_FIELD_NAME_BUILD(206,206 ,User_Defined_Valid       )

    /* key1 fields */
    ,LION3_EPCL_KEY1_FIELD_NAME_BUILD( 90, 97 ,TCP_UDP_Port_Comparators )
    ,LION3_EPCL_KEY1_FIELD_NAME_BUILD( 98,129 ,DIP_31_0                 )
    ,LION3_EPCL_KEY1_FIELD_NAME_BUILD(130,137 ,L4_Header_Byte_13        )
    ,LION3_EPCL_KEY1_FIELD_NAME_BUILD(189,189 ,IPv4_fragmented          )

    /* key2 fields */
    ,LION3_EPCL_KEY2_FIELD_NAME_BUILD( 73, 73 ,Is_BC                    )
    ,LION3_EPCL_KEY2_FIELD_NAME_BUILD( 98,129 ,SIP_31_0                 )
    ,LION3_EPCL_KEY2_FIELD_NAME_BUILD(130,161 ,DIP_31_0                 )
    ,LION3_EPCL_KEY2_FIELD_NAME_BUILD(162,169 ,L4_Header_Byte_13        )
    ,LION3_EPCL_KEY2_FIELD_NAME_BUILD(170,177 ,L4_Header_Byte_1         )
    ,LION3_EPCL_KEY2_FIELD_NAME_BUILD(178,185 ,L4_Header_Byte_0         )

    /* key3 fields */
    ,LION3_EPCL_KEY3_FIELD_NAME_BUILD(128,159 ,DIP_31_0                 )
    ,LION3_EPCL_KEY3_FIELD_NAME_BUILD(160,160 ,L2_Encap_Type            )
    ,LION3_EPCL_KEY3_FIELD_NAME_BUILD(161,176 ,Ether_Type_OR_Dsap_Ssap  )
    ,LION3_EPCL_KEY3_FIELD_NAME_BUILD(177,177 ,IPv4_fragmented          )
    ,LION3_EPCL_KEY3_FIELD_NAME_BUILD(208,219 ,Tag1_VID                 )
    ,LION3_EPCL_KEY3_FIELD_NAME_BUILD(220,220 ,Tag1_CFI                 )
    ,LION3_EPCL_KEY3_FIELD_NAME_BUILD(221,223 ,UP1                      )
    ,LION3_EPCL_KEY3_FIELD_NAME_BUILD(224,225 ,Number_of_MPLS_Labels    )
    ,LION3_EPCL_KEY3_FIELD_NAME_BUILD(226,227 ,Protocol_After_MPLS      )
    ,LION3_EPCL_KEY3_FIELD_NAME_BUILD(228,247 ,MPLS_Label_1             )
    ,LION3_EPCL_KEY3_FIELD_NAME_BUILD(248,250 ,MPLS_EXP_1               )
    ,LION3_EPCL_KEY3_FIELD_NAME_BUILD(251,298 ,MAC_SA                   )
    ,LION3_EPCL_KEY3_FIELD_NAME_BUILD(299,346 ,MAC_DA                   )
    ,LION3_EPCL_KEY3_FIELD_NAME_BUILD(347,347 ,Is_MPLS                  )
    ,LION3_EPCL_KEY3_FIELD_NAME_BUILD(348,367 ,MPLS_Label_0             )
    ,LION3_EPCL_KEY3_FIELD_NAME_BUILD(368,370 ,MPLS_EXP_0               )
    ,LION3_EPCL_KEY3_FIELD_NAME_BUILD(371,377 ,Data_Pkt_OR_CtrlToCPU_Src_Trunk_ID_6_0     )
    ,LION3_EPCL_KEY3_FIELD_NAME_BUILD(371,377 ,Data_Pkt_Src_Dev_4_0                       )
    ,LION3_EPCL_KEY3_FIELD_NAME_BUILD(371,377 ,Ctrl_To_CPU_Src_Trg_Src_Dev_4_0            )
    ,LION3_EPCL_KEY3_FIELD_NAME_BUILD(371,377 ,From_CPU_Egress_Filter_En_Src_Dev_4_0      )
    ,LION3_EPCL_KEY3_FIELD_NAME_BUILD(371,377 ,To_Analyzer_Rx_Sniff_Src_Dev_4_0           )
    ,LION3_EPCL_KEY3_FIELD_NAME_BUILD(378,378 ,Data_Pkt_OR_Ctrl_To_CPU_Src_Is_Trunk       )
    ,LION3_EPCL_KEY3_FIELD_NAME_BUILD(379,383 ,Source_ID_4_0                              )
    ,LION3_EPCL_KEY3_FIELD_NAME_BUILD(384,384 ,Is_Routed                                  )
    ,LION3_EPCL_KEY3_FIELD_NAME_BUILD(385,385 ,Data_Pkt_Is_Unknown                        )
    ,LION3_EPCL_KEY3_FIELD_NAME_BUILD(385,385 ,Ctrl_To_CPU_CPU_CODE_7                     )
    ,LION3_EPCL_KEY3_FIELD_NAME_BUILD(385,385 ,From_CPU_Is_Unknown                        )
    ,LION3_EPCL_KEY3_FIELD_NAME_BUILD(386,386 ,Is_L2_Valid                                )
    ,LION3_EPCL_KEY3_FIELD_NAME_BUILD(387,394 ,TCP_UDP_Port_Comparators                   )
    ,LION3_EPCL_KEY3_FIELD_NAME_BUILD(395,396 ,Src_Port_7_6                               )
    ,LION3_EPCL_KEY3_FIELD_NAME_BUILD(397,398 ,Packet_Type_1_0                            )
    ,LION3_EPCL_KEY3_FIELD_NAME_BUILD(399,399 ,is_VIDX                                    )
    ,LION3_EPCL_KEY3_FIELD_NAME_BUILD(400,401 ,Source_Dev_6_5                             )
    ,LION3_EPCL_KEY3_FIELD_NAME_BUILD(402,406 ,Source_Dev_11_7_OR_Trunk_ID_11_7           )
    ,LION3_EPCL_KEY3_FIELD_NAME_BUILD(407,413 ,Source_ID_11_5                             )
    ,LION3_EPCL_KEY3_FIELD_NAME_BUILD(414,414 ,User_Defined_Valid                         )

    /* key4 fields */
    ,LION3_EPCL_KEY4_FIELD_NAME_BUILD(128,159 ,SIP_63_32                )
    ,LION3_EPCL_KEY4_FIELD_NAME_BUILD(160,190 ,SIP_94_64                )
    ,LION3_EPCL_KEY4_FIELD_NAME_BUILD(208,208 ,SIP_95_95                )
    ,LION3_EPCL_KEY4_FIELD_NAME_BUILD(209,240 ,SIP_127_96               )
    ,LION3_EPCL_KEY4_FIELD_NAME_BUILD(241,241 ,Ipv6_EH_exist            )
    ,LION3_EPCL_KEY4_FIELD_NAME_BUILD(242,242 ,Is_IPv6_EH_Hop_By_Hop    )
    ,LION3_EPCL_KEY4_FIELD_NAME_BUILD(243,250 ,DIP_127_120              )
    ,LION3_EPCL_KEY4_FIELD_NAME_BUILD(347,358 , Tag1_VID                )
    ,LION3_EPCL_KEY4_FIELD_NAME_BUILD(359,359 , Tag1_CFI                )
    ,LION3_EPCL_KEY4_FIELD_NAME_BUILD(360,362 , UP1                     )

    /* key5 fields */
    ,LION3_EPCL_KEY5_FIELD_NAME_BUILD(251,282 ,DIP_63_32                )
    ,LION3_EPCL_KEY5_FIELD_NAME_BUILD(283,314 ,DIP_95_64                )
    ,LION3_EPCL_KEY5_FIELD_NAME_BUILD(315,338 ,DIP_119_96               )
    ,LION3_EPCL_KEY5_FIELD_NAME_BUILD(339,370 ,DIP_31_0                 )


    /* key6 and key 7 common fields */
    ,LION3_EPCL_KEY6_FIELD_NAME_BUILD(0  ,  9 ,PCL_ID_9_0                )
    ,LION3_EPCL_KEY6_FIELD_NAME_BUILD(11 , 16 ,Src_Port_5_0              )
    ,LION3_EPCL_KEY6_FIELD_NAME_BUILD(17 , 23 ,Data_Pkt_QoS_Profile_6_0  )
    ,LION3_EPCL_KEY6_FIELD_NAME_BUILD(17 , 23 ,Ctrl_To_CPU_CPU_CODE_6_0  )
    ,LION3_EPCL_KEY6_FIELD_NAME_BUILD(24 , 24 ,Tag1_Exist                )
    ,LION3_EPCL_KEY6_FIELD_NAME_BUILD(25 , 36 ,Source_ID_11_0            )
    ,LION3_EPCL_KEY6_FIELD_NAME_BUILD(37 , 48 ,VID0                      )
    ,LION3_EPCL_KEY6_FIELD_NAME_BUILD(49 , 50 ,Src_Port_7_6              )
    ,LION3_EPCL_KEY6_FIELD_NAME_BUILD(51 , 57 ,Data_Pkt_OR_CtrlToCPU_Src_Trunk_ID_6_0 )
    ,LION3_EPCL_KEY6_FIELD_NAME_BUILD(51 , 57 ,Data_Pkt_Src_Dev_4_0                   )
    ,LION3_EPCL_KEY6_FIELD_NAME_BUILD(51 , 57 ,Ctrl_To_CPU_Src_Trg_Src_Dev_4_0        )
    ,LION3_EPCL_KEY6_FIELD_NAME_BUILD(51 , 57 ,From_CPU_Egress_Filter_En_Src_Dev_4_0  )
    ,LION3_EPCL_KEY6_FIELD_NAME_BUILD(51 , 57 ,To_Analyzer_Rx_Sniff_Src_Dev_4_0       )
    ,LION3_EPCL_KEY6_FIELD_NAME_BUILD(58 , 59 ,Source_Dev_6_5            )
    ,LION3_EPCL_KEY6_FIELD_NAME_BUILD(60 , 60 ,Src_Is_Trunk              )
    ,LION3_EPCL_KEY6_FIELD_NAME_BUILD(61 ,108 ,MAC_SA                    )
    ,LION3_EPCL_KEY6_FIELD_NAME_BUILD(109,156 ,MAC_DA                    )
    ,LION3_EPCL_KEY6_FIELD_NAME_BUILD(157,157 ,Is_L2_Valid               )
    ,LION3_EPCL_KEY6_FIELD_NAME_BUILD(158,158 ,Is_IP                     )
    ,LION3_EPCL_KEY6_FIELD_NAME_BUILD(159,166 ,Ip_Protocol               )
    ,LION3_EPCL_KEY6_FIELD_NAME_BUILD(167,172 ,Packet_DSCP               )
    ,LION3_EPCL_KEY6_FIELD_NAME_BUILD(173,174 ,TOS_1_0                   )
    ,LION3_EPCL_KEY6_FIELD_NAME_BUILD(175,182 ,TTL                       )
    ,LION3_EPCL_KEY6_FIELD_NAME_BUILD(183,183 ,Is_ARP                    )
    ,LION3_EPCL_KEY6_FIELD_NAME_BUILD(184,191 ,SIP_7_0                   )
    ,LION3_EPCL_KEY6_FIELD_NAME_BUILD(208,231 ,SIP_31_8                  )
    ,LION3_EPCL_KEY6_FIELD_NAME_BUILD(232,263 ,DIP_31_0                  )
    ,LION3_EPCL_KEY6_FIELD_NAME_BUILD(264,271 ,TCP_UDP_Port_Comparators  )
    ,LION3_EPCL_KEY6_FIELD_NAME_BUILD(272,279 ,L4_Header_Byte_13         )
    ,LION3_EPCL_KEY6_FIELD_NAME_BUILD(280,287 ,L4_Header_Byte_1          )
    ,LION3_EPCL_KEY6_FIELD_NAME_BUILD(288,295 ,L4_Header_Byte_0          )
    ,LION3_EPCL_KEY6_FIELD_NAME_BUILD(296,303 ,L4_Header_Byte_3_OR_5     )
    ,LION3_EPCL_KEY6_FIELD_NAME_BUILD(304,311 ,L4_Header_Byte_2_OR_4     )
    ,LION3_EPCL_KEY6_FIELD_NAME_BUILD(312,312 ,Is_L4_Valid               )
    ,LION3_EPCL_KEY6_FIELD_NAME_BUILD(313,315 ,From_CPU_TC               )
    ,LION3_EPCL_KEY6_FIELD_NAME_BUILD(316,317 ,From_CPU_DP               )
    ,LION3_EPCL_KEY6_FIELD_NAME_BUILD(318,319 ,Packet_Type               )
    ,LION3_EPCL_KEY6_FIELD_NAME_BUILD(320,320 ,Src_Trg_OR_Tx_Mirror      )
    ,LION3_EPCL_KEY6_FIELD_NAME_BUILD(321,323 ,Assigned_UP               )
    ,LION3_EPCL_KEY6_FIELD_NAME_BUILD(324,329 ,Trg_Port_5_0              )
    ,LION3_EPCL_KEY6_FIELD_NAME_BUILD(330,330 ,Rx_Sniff                  )
    ,LION3_EPCL_KEY6_FIELD_NAME_BUILD(331,331 ,Is_Routed                 )
    ,LION3_EPCL_KEY6_FIELD_NAME_BUILD(332,332 ,Is_IPv6                   )

    /* key6 only fields */
    ,LION3_EPCL_KEY6_FIELD_NAME_BUILD(333,333 ,Ipv4_Options              )
    ,LION3_EPCL_KEY6_FIELD_NAME_BUILD(334,334 ,is_VIDX                   )
    ,LION3_EPCL_KEY6_FIELD_NAME_BUILD(335,346 ,Tag1_VID                  )
    ,LION3_EPCL_KEY6_FIELD_NAME_BUILD(347,347 ,Tag1_CFI                  )
    ,LION3_EPCL_KEY6_FIELD_NAME_BUILD(348,350 ,UP1                       )
    ,LION3_EPCL_KEY6_FIELD_NAME_BUILD(351,355 ,Source_Dev_OR_Trunk_11_7  )
    ,LION3_EPCL_KEY6_FIELD_NAME_BUILD(356,357 ,Trg_Port_7_6              )
    ,LION3_EPCL_KEY6_FIELD_NAME_BUILD(358,358 ,User_Defined_Valid        )

    /* key7 only fields */
    ,LION3_EPCL_KEY7_FIELD_NAME_BUILD(333,333 ,IPv6_EH_exist             )
    ,LION3_EPCL_KEY7_FIELD_NAME_BUILD(334,334 ,IPv6_Is_ND                )
    ,LION3_EPCL_KEY7_FIELD_NAME_BUILD(335,335 ,Is_IPv6_EH_Hop_By_Hop     )
    ,LION3_EPCL_KEY7_FIELD_NAME_BUILD(336,367 ,SIP_63_32                 )
    ,LION3_EPCL_KEY7_FIELD_NAME_BUILD(368,399 ,SIP_95_64                 )
    ,LION3_EPCL_KEY7_FIELD_NAME_BUILD(416,447 ,SIP_127_96                )
    ,LION3_EPCL_KEY7_FIELD_NAME_BUILD(448,479 ,DIP_63_32                 )
    ,LION3_EPCL_KEY7_FIELD_NAME_BUILD(480,511 ,DIP_95_64                 )
    ,LION3_EPCL_KEY7_FIELD_NAME_BUILD(512,543 ,DIP_127_96                )
    ,LION3_EPCL_KEY7_FIELD_NAME_BUILD(544,544 ,is_VIDX                   )
    ,LION3_EPCL_KEY7_FIELD_NAME_BUILD(545,556 ,Tag1_VID                  )
    ,LION3_EPCL_KEY7_FIELD_NAME_BUILD(557,557 ,Tag1_CFI                  )
    ,LION3_EPCL_KEY7_FIELD_NAME_BUILD(558,560 ,UP1                       )
    ,LION3_EPCL_KEY7_FIELD_NAME_BUILD(561,565 ,Source_Dev_OR_Trunk_11_7  )
    ,LION3_EPCL_KEY7_FIELD_NAME_BUILD(566,578 ,SRC_ePort_OR_Trunk_ID     )
    ,LION3_EPCL_KEY7_FIELD_NAME_BUILD(583,595 ,TRG_ePort                 )
    ,LION3_EPCL_KEY7_FIELD_NAME_BUILD(600,600 ,eVLAN_12                  )
    ,LION3_EPCL_KEY7_FIELD_NAME_BUILD(604,605 ,Trg_Port_7_6              )
    ,LION3_EPCL_KEY7_FIELD_NAME_BUILD(606,606 ,User_Defined_Valid        )

    /* EPCL : holds the info about the extra 10 bytes of the 60B UBD key fields - lion3 format */
    ,LION3_EPCL_KEY_60B_EXTRA_10B_FIELD_NAME_BUILD(400,  400 , UDB_Valid                  )
    ,LION3_EPCL_KEY_60B_EXTRA_10B_FIELD_NAME_BUILD(401,  410 , PCL_ID                     )
    ,LION3_EPCL_KEY_60B_EXTRA_10B_FIELD_NAME_BUILD(411,  423 , eVLAN                      )
    ,LION3_EPCL_KEY_60B_EXTRA_10B_FIELD_NAME_BUILD(424,  436 , Source_ePort_or_Trunk_ID   )
    ,LION3_EPCL_KEY_60B_EXTRA_10B_FIELD_NAME_BUILD(437,  449 , Trg_ePort                  )
    ,LION3_EPCL_KEY_60B_EXTRA_10B_FIELD_NAME_BUILD(450,  459 , Src_Dev                    )
    ,LION3_EPCL_KEY_60B_EXTRA_10B_FIELD_NAME_BUILD(460,  469 , Trg_Dev                    )
    ,LION3_EPCL_KEY_60B_EXTRA_10B_FIELD_NAME_BUILD(470,  477 , Local_Dev_Trg_Phy_Port     )
    ,LION3_EPCL_KEY_60B_EXTRA_10B_FIELD_NAME_BUILD(470,  470 , Orig_Src_Is_Trunk          )
    ,LION3_EPCL_KEY_60B_EXTRA_10B_FIELD_NAME_BUILD(478,  479 , Reserved                   )


    ,LION3_EPCL_KEY_FIELDS_ID___LAST__E/*must be last*/
}LION3_EPCL_KEY_FIELDS_ID_ENT;

/* types of fields that may be 'MUXED' with PCL UDB type */
typedef enum {
    SNET_XCAT_PCL_UDB_MUX_NONE_E,       /* UDB is not MUXED with other field */
    SNET_XCAT_PCL_UDB_MUX_VRF_LSB_E,    /* UDB is MUXED with 8 lsb of VRF */
    SNET_XCAT_PCL_UDB_MUX_VRF_MSB_E,    /* UDB is MUXED with 4 msb of VRF */
    SNET_XCAT_PCL_UDB_MUX_QOS_E,        /* UDB is MUXED with qosIndex */
    SNET_XCAT_PCL_UDB_MUX_TAG1_INFO_E,   /* UDB is MUXED with tag 1 info :  {Tag1 Exist, Tag1 CFI,  VID1[11:6]} */
    SNET_XCAT_PCL_UDB_MUX_FLOW_ID_BYTE_0_E,   /* UDB is MUXED with byte 0 of Desc<FlowID>. */
    SNET_XCAT_PCL_UDB_MUX_FLOW_ID_BYTE_1_E,   /* UDB is MUXED with byte 1 of Desc<FlowID>. */
}SNET_XCAT_PCL_UDB_MUX_MODES_ENT;

/**
 * SNET_LION3_PCL_ACTION_OAM_STC
 *
 * DESCRIPTION :
 *
 *  OAM related fields in IPCL/EPCL action
 *
 *  FIELDS:
 *
 *  oamProcessEnable        - When enabled the packet is bound to to an entry in the OAM
 *  Table oamProfile        - The OAM profile determines the set of UDBs where the key
 *                            attributes (opcode, MEG level, RDI, MEG level)
 *  ptpTimeStampEnable      - When enabled, it indicates that a timestamp should be inserted
 *                            into the packet
 *  ptpTimeStampOffsetIndex - When <Timestamp Enable> is enabled, this field indicates
 *                            the offset index for the timestamp offset table
 *  COMMENTS:
 *      Relevant only to eArch devices
 *
 */
typedef struct {
    GT_U32      oamProcessEnable;   /* When enabled the packet is bound to an entry in the OAM Table */
    GT_U32      oamProfile;         /* The OAM profile determines the set of UDBs
                                    where the key attributes (opcode, MEG level, RDI, MEG level) of the OAM message are taken from. */
    GT_U32      ptpTimeStampEnable; /* When enabled, it indicates that a timestamp
                                    should be inserted into the packet */
    GT_U32      ptpTimeStampOffsetIndex; /* When <Timestamp Enable> is enabled,this field indicates
                                    the offset index for the timestamp offset table. */
    GT_U32      channelTypeToOpcodeMappingEn; /*"When enabled, the MPLS G-ACh Channel Type is mapped to an OAM Opcode
                                    that is used by the OAM engine."*/

}SNET_LION3_PCL_ACTION_OAM_STC;

/*
    struct : SNET_XCAT_PCL_ACTION_STC

    description : The policy engine maintains an 1024 entries table ,
                corresponding to the 1024 rules that may be defined in the
                TCAM lookup structure. The line index of the matching rule
                is used to index the policy action table and extract the
                action to perform.
*/
typedef struct {
    SNET_CHT3_PCL_ACTION_STC baseAction;
    GT_U32      arpPointer;         /* Pointer to ARP entry */
    GT_U32      ipclProfileIndex;   /* Pointer to IPCL configuration entry to be used when fetching IPCL parameter */
    GT_BOOL     policerCounterEn;   /* Rule is bound to the policer indexed by policerIndex */
    GT_BOOL     bypassBridge;       /* Bridge engine is bypassed for this packet */
    GT_BOOL     bypassIngressPipe;  /* Ingress pipe is bypassed for this packet */
    GT_U32      policy2LookUpMode;  /* Controls the index used for IPCL2 lookup */
    GT_U32      policy1LookUpMode;  /* Controls the index used for IPCL1 lookup */
    GT_U32      tcpRstFinMirrorEn;  /* Determines what to do with a TCP RST or FIN packet */
    GT_BOOL     macHeaderModifyEn;  /* Enable/disable MAC header modification */

    SNET_XCAT_TTI_ASSIGN_LOGIC_PORT_ACTION_STC srcLogicPortInfo;
    SNET_XCAT_PCL_ACTION_VPLS_SUPPORT_STC   vplsInfo;
}SNET_XCAT_PCL_ACTION_STC;


typedef enum{
    SNET_LION3_IPCL_REDIRECT_CMD_NONE_E = 0,
    SNET_LION3_IPCL_REDIRECT_CMD_TO_EGRESS_INTERFACE_E = 1,
    SNET_LION3_IPCL_REDIRECT_CMD_TO_ROUTER_LTT_INDEX_E = 2,/*redirect to NH (actually LTT index)*/
    /* value 3 - Use IPCL AE as NHE - obsolete */
    SNET_LION3_IPCL_REDIRECT_CMD_ASSIGN_VRF_ID_E = 4,
    /* value 5 - Assign Logical Port - obsolete */
    SNET_LION3_IPCL_REDIRECT_CMD_REPLACE_MAC_SA_E = 6
}SNET_LION3_IPCL_REDIRECT_CMD_ENT;

typedef struct{
    SNET_DST_INTERFACE_STC  egressInterface;/* port/trunk/vidx */
    GT_BIT  VNTL2Echo;
    GT_BIT      modifyMacSaEn; /* Enable/disable MAC SA header modification */
    GT_BIT  tunnelStart;/* indication of use TS/ARP info in tsOrArpInfo */
    union{
        struct{
            GT_U32  index;          /* the TS index (pointer)*/
            GT_U32  passengerType;  /* The type of the tunneled packet */
        }tsInfo;/*tunnel start info*/

        struct{
            GT_U32  index;          /* the ARP index (pointer)*/
        }arpInfo;
    }tsOrArpInfo;
}SNET_LION3_IPCL_ACTION_REDIRECT_TO_EGRESS_INTERFACE_STC;

typedef struct{
    GT_BIT      lookup1Mode;/*second lookup*/
    GT_BIT      lookup2Mode;/*third lookup*/
    GT_U32      profileIndex;/*profile index*/
}SNET_LION3_IPCL_PROFILE_INFO_STC;



/*
    struct : SNET_LION3_PCL_ACTION_STC

    description : Lion3 IPCL action format.
*/
typedef struct {
    SKERNEL_EXT_PACKET_CMD_ENT packetCmd; /* forwarding command */
    GT_U32      cpuCode; /* cpu code*/
    GT_U32      mirrorToIngressAnalyzerPort; /* enables the mirroring of the packet to the ingress analyser port */
    PCL_TTI_ACTION_REDIRECT_CMD_ENT redirectCmd;

    /*If <Redirect Command> = 1 (Redirect to egress interface)*/
    SNET_LION3_IPCL_ACTION_REDIRECT_TO_EGRESS_INTERFACE_STC redirectEgressInfo;

    /* If <Redirect Command> != 1 (Redirect to egress interface)*/
    GT_BIT      setEgressFilterRegistered;      /* Enable/disable MAC DA header modification */

    /*If <Redirect Command> = 2 (Redirect to NHE)*/
    GT_U32      routerLLTIndex;

    /* if <Redirect Command> = 4 (Assign VRF_ID)*/
    GT_U32      vrfId;
    /*If <Redirect Command> = 0 (No Redirect)
      If <Redirect Command> = 2 (Redirect to NHE)
      if <Redirect Command> = 4 (Assign VRF_ID)*/
    SNET_LION3_IPCL_PROFILE_INFO_STC    ipclProfile;

    /* if (<Redirect Command> != 6 - Replace MAC SA)*/
    GT_BIT      bindToPolicerMeter;
    /* if (<Redirect Command> != 6 - Replace MAC SA)*/
    GT_BIT      bindToPolicerCounter;
    /* if (<Redirect Command> != 6 - Replace MAC SA)*/
    GT_U32      policerIndex;

    /*if (<Redirect Command> = 6 - Replace MAC SA)*/
    GT_U8       macSa[6];
    /*if (<Redirect Command> = 6 - Replace MAC SA)*/
    GT_U32      arpIndex;

    GT_BIT      bindToCncCounter;
    GT_U32      cncCounterIndex;
    GT_BIT      srcIdSetEn;         /* Assigning Source-ID for the Packet enable bit */
    GT_U32      srcId;              /* Assigning Source-ID for the Packet */
    GT_BIT      actionStop;         /* Once this field is set, the Ingress PCL lookup is ended (the next lookups are not executed) */
    GT_BIT      bypassBridge;       /* Bridge engine is bypassed for this packet */
    GT_BIT      bypassIngressPipe;  /* Ingress pipe is bypassed for this packet */
    SKERNEL_PRECEDENCE_ORDER_ENT vidPrecedence;      /* The VLAN-ID precedence for the subsequent VLAN assignment mechanism */
    GT_BIT      enNestedVlan;       /* enables nested VLAN */
    GT_U32      vlanCmd;            /* the setting of the VLAN command */
    GT_U32      eVid;               /* The VLAN-ID set byu this entry */
    GT_BIT      qosProfileMarkingEn; /* enables the profile remarking */
    SKERNEL_PRECEDENCE_ORDER_ENT qosPrecedence; /* marking of the QoS precedence */
    GT_U32      qosProfile;         /* index to the QoS Table entry */
    GT_U32      modifyDscp;         /* enables the modification of the packets DSCP */
    GT_U32      modifyUp;           /* enables the modification of the packets UP */
    GT_BIT      tcpRstFinMirrorEn;  /* Determines what to do with a TCP RST or FIN packet */
    GT_BIT      modifyMacDaEn;      /* Enable/disable MAC DA header modification */
    GT_U32      tag1Cmd;/*tag 1 command .*/
    GT_U32      up1Cmd;/*UP 1 command .*/
    GT_U32      vid1;  /* vid1 */
    GT_U32      up1;   /* up1 */
    GT_U32      flowId;/* flowId */

    GT_BIT      setMacToMe;
    GT_BIT      assignSrcEPortEnable;
    GT_U32      sourceEPort;
    GT_BIT      userAcEnable;

    SNET_LION3_PCL_ACTION_OAM_STC   oamInfo; /* OAM info */


}SNET_LION3_PCL_ACTION_STC;


/*
    struct : SNET_XCAT_EPCL_ACTION_STC

*/
typedef struct {
    SNET_CHT3_EPCL_ACTION_STC baseAction;
    GT_BOOL     policerMeterEn;  /* Rule is bound to the policer metering entry
                                    indexed by policerIndex */
    GT_BOOL     policerCountEn;  /* Rule is bound to the policer counting entry
                                    indexed by policerIndex */
    GT_U32      policerIndex;    /* Policer metering index */
}SNET_XCAT_EPCL_ACTION_STC;

/*
    struct : SNET_LION3_EPCL_ACTION_STC

*/
typedef struct {
    SKERNEL_CH2_EPCL_PACKET_CMD_ENT packetCmd; /*"The entry forwarding command.
                                            FORWARD
                                            HARD DROP
                                             0x0 = Forward; Forward; ;
                                             0x3 = HardDrop; HardDrop; "
                                            */
    GT_U32      dscpOrExp; /*The DSCP/EXP field set to the transmitted packets. See <ModifyDSCP>.*/
    GT_U32      up0;/*The UP value to assign to tag0 of the packet according to <Enable Modify UP0>*/
    GT_U32      modifyDscpOrExp;/*"The EPCL action today supports the fields <Modify DSCP> and <DSCP> which are only relevant for IP packets.
                                    If the packet is MPLS and these fields are set, the outer MPLS label EXP field is modified."*/
    GT_U32      enableModifyUp0;/*"This field determines the Tag0 UP assigned to the packet matching the EPCL entry.
                                0 = DoNotModify: Do not modify Tag0 UP assigned to the packet.
                                1 = ModifyOuterTag: This is a BWC mode: EPCL Action entry <UP0> is assigned to the outer tag of the packet. If the packet is tunneled, this refers to the tunnel header tag.
                                2 = ModifyTag0: EPCL Action entry <UP0> is assigned to the UP field of tag 0 of the packet.
                                3 = Reserved.
                                <Tag0 VID> in mode 2 refers to Tag0 of the packet as was previously assigned in the device pipes and used in the EPCL key, regardless of its current location or even existance (if it was removed then no action is taken) in the packet."
                                */
    GT_BIT      bindToCncCounter;/*"Renamed from <ActivateCounter> - Enables binding of this policy action entry to a counter in the Centralized Counter (CNC) unit.
                                If this field is enabled, all packets that match this entry are counted in the counter specified by <CNC Counter Index>.
                                 0x0 = Disable;
                                 0x1 = Enable;"*/
    GT_U32      cncCounterIndex;
    GT_BIT      tag1VidCmd;/*"This field determines the Tag1 VID assigned to the packet matching the EPCL entry.
                        0 = DoNotModify: Do not modify Tag1 VID assigned to the packet.
                        1 = Modify: EPCL Action entry <Tag1 VID> is assigned to tag 1 of the packet.
                        This refers to Tag1 of the packet as was previously assigned in the device pipes end used in the EPCL key, regardless of its current location or even existance (if it was removed then no action is taken) in the packet."
                        */
    GT_BIT      enableModifyUp1;/*"This field determines the Tag1 UP assigned to the packet matching the EPCL entry.
                        0 = Keep: Keep previous Tag1 UP assigned to the packet.
                        1 = Modify: EPCL Action entry <UP1> is assigned to tag 1 UP of the packet.
                        This refers to Tag1 of the packet as was previously assigned in the device pipes end used in the EPCL key, regardless of its current location or even existance (if it was removed then no action is taken) in the packet."
                        */
    GT_U32      tag1Vid;/*The VID to assign to tag1 of the packet according to <Tag1 VLAN Command>*/
    GT_U32      up1; /*The UP1 value to assign to tag1 of the packet according to <Enable Modify UP1>*/
    GT_U32      tag0VlanCmd;/*"This field determines the Tag0 VID command assigned to the packet matching the EPCL entry.
                        0 = DoNotModify: Do not modify Tag0 VID assigned to the packet.
                        1 = ModifyOuterTag: This is a BWC mode: EPCL Action entry <Tag0 VID> is assigned to the outer tag of the packet. If the packet is tunneled, this refers to the tunnel header tag.
                        2 = ModifyTag0: EPCL Action entry <Tag0 VID> is assigned to VID field of tag 0 of the packet.
                        3 = Reserved.
                        <Tag0 VID> in mode 2 refers to Tag0 of the packet as was previously assigned in the device pipes and used in the EPCL key, regardless of its current location or even existance (if it was removed then no action is taken) in the packet.
                        In case HA egress ePort table 2 <Push VLAN Command> is enabled and an additional tag is pushed, the additional tag is referred to as the outer tag. i.e. - if this additional tag is pushed, if Tag0 VLAN command = ModifyOuterTag, the pushed tag is modified. Else if Tag0 VLAN command = ModifyTag0, the pushed tag is not modified, and Tag0 (if exist) is modified by the EPCL."
                        */
    GT_U32      tag0Vid; /*The VID to assign to tag0 of the packet according to <Tag0 VLAN Command>*/
    GT_BIT      bindToPolicerCounter; /*"Renamed from ActivateBilling - Bind to Policer Counter (Use PolicerIndex field)
                        0x0 = Disable;
                         0x1 = Enable;"*/
    GT_BIT      bindToPolicerMeter; /*"Renamed from ActivatePolicer - Bind to Policer Meter:
                        0x0 = Disable;
                         0x1 = Enable"*/
    GT_U32      policerIndex; /*Policer Index*/
    GT_U32      flowId; /*"The flow ID assigned to the packet.
                        The value 0x0 represents "do not assign Flow ID". The EPCL overrides the existing Desc<Flow ID> value if and only if this field in the EPCL action is non-zero
                        used by:
                         - Egress OAM engine.
                         - eDSA Tag - TO_CPU eDSA tagged packets, if the EPCL assigns a Flow ID it is incorporated in the eDSA tag by the HA remarking logic."
                        */

    GT_BIT      epclReservedEnable;
    GT_U32      epclReserved;
    GT_U32      tmQueueId;

    SNET_LION3_PCL_ACTION_OAM_STC   oamInfo; /* OAM info.*/

}SNET_LION3_EPCL_ACTION_STC;


/* OAM Profiles */
typedef enum {
    SNET_LION_OAM_PROFILE_OPCODE_E = 0,
    SNET_LION_OAM_PROFILE_MEG_LEVEL_E,
    SNET_LION_OAM_PROFILE_RDI_E,
    SNET_LION_OAM_PROFILE_TX_PERIOD_E,
    SNET_LION_OAM_PROFILE_R_BIT_E
} SNET_LION_OAM_PROFILES_ENT;

/* OAM Profiles UDB index */
typedef struct {
    GT_U32 oamProfileUdbIndex[2];
} SNET_LION_OAM_PROFILE_UDB_OFFSET_STC;
/*******************************************************************************
* snetXCatIPcl
*
* DESCRIPTION:
*        Ingress Policy Engine processing for incoming frame on XCat
*        ASIC simulation.
*        PCL processing, Pcl assignment, Lookup Key creation, 3 Lookups,
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
*
*
*******************************************************************************/
GT_VOID snetXCatIPcl
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr
);

/*******************************************************************************
* snetXCatEPcl
*
* DESCRIPTION:
*        Egress Policy Engine processing for outgoing frame on XCat
*        asic simulation.
*        ePCL processing, ePcl assignment, key forming, 1 Lookup,
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
*       Egress PCL has only one lookup cycle.
*
*******************************************************************************/
GT_VOID snetXCatEPcl
(
    IN SKERNEL_DEVICE_OBJECT                * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC   * descrPtr,
    IN  GT_U32                                egressPort
);

/*******************************************************************************
*   snetXCatPclUserDefinedByteGet
*
* DESCRIPTION:
*       function get from the packet the user defined byte info
*
* INPUTS:
*       devObjPtr    - (pointer to) device object.
*       descrPtr     - (pointer to) frame data buffer Id
*       userDefinedAnchor - user defined byte Anchor
*       userDefinedOffset - user defined byte offset from Anchor
*       udbClient   - UDB Client (TTI/IPCL/EPCL)
* OUTPUTS:
*       userDefinedByteValuePtr - (pointer to) the user defined byte value
*
* RETURN:
*       GT_OK              Operation succeeded
*       GT_FAIL            Operation failed
*
* COMMENTS:
*       [1] 8.2.2.2 parser -- page 81
*       [1] 8.5.2.3 User-Defined Bytes -- page 99
*
*******************************************************************************/
GT_STATUS snetXCatPclUserDefinedByteGet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_U32  userDefinedAnchor,
    IN GT_U32  userDefinedOffset,
    IN SNET_UDB_CLIENT_ENT  udbClient,
    OUT GT_U8  *userDefinedByteValuePtr
);

/*******************************************************************************
*   lion3EPclUdbKeyValueGet
*
* DESCRIPTION:
*        Get user defined value by user defined key - sip5 - EPCL
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       descrPtr            - pointer to frame data buffer Id.
*       pktType             - types of packets.
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
GT_STATUS lion3EPclUdbKeyValueGet
(
    IN SKERNEL_DEVICE_OBJECT                        * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC              * descrPtr,
    IN GT_U32                                         udbIdx,
    OUT GT_U8                                       * byteValuePtr
);


/*******************************************************************************
* snetPclTablesFormatInit
*
* DESCRIPTION:
*        init the format of PCL tables.(IPCL and EPCL)
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
void snetPclTablesFormatInit(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr
);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __snetXCatPclh */


