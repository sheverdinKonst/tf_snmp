/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetCheetah2Pcl.c
*
* DESCRIPTION:
*       Cheetah2 Asic Simulation .
*       Ingress Policy Engine processing for incoming frame.
*       Source Code file.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 31 $
*
*******************************************************************************/

#include <asicSimulation/SKernel/suserframes/snet.h>
#include <asicSimulation/SKernel/smem/smemCheetah.h>
#include <asicSimulation/SKernel/cheetahCommon/sregCheetah.h>
#include <asicSimulation/SKernel/suserframes/snetCheetah2Pcl.h>
#include <asicSimulation/SKernel/suserframes/snetXCat.h>
#include <asicSimulation/SLog/simLog.h>

static CHT2_PCL_USER_DEFINED_CONFIG_REG_STC cht2PclUserDefinedConfigInfoShortL2KeyArray[3] =
{
    {0 ,7 },
    {11,18},
    {22,29}
};

static CHT2_PCL_USER_DEFINED_CONFIG_REG_STC cht2PclUserDefinedConfigInfoShortL2L3KeyArray[2] =
{
    {11,18},
    {22,29}
};

static CHT2_PCL_USER_DEFINED_CONFIG_REG_STC cht2PclUserDefinedConfigInfoShortL3L4KeyArray[3] =
{
    {0,7},
    {11,18},
    {22,29}
};

static CHT2_PCL_USER_DEFINED_CONFIG_REG_STC cht2PclUserDefinedConfigInfoLongL2L3L4KeyArray[6] =
{
    {0 , 7},
    {11 ,18},
    {22 , 29},
    {0 , 7},
    {11,18},
    {22,29}
};

static CHT2_PCL_USER_DEFINED_CONFIG_REG_STC cht2PclUserDefinedConfigInfoLongL2Ipv6KeyArray[6] =
{
    {0 , 7},
    {11 ,18},
    {22 , 29},
    {0 , 7},
    {11,18},
    {22,29}
};

static CHT2_PCL_USER_DEFINED_CONFIG_REG_STC cht2PclUserDefinedConfigInfoLongL4Ipv6KeyArray[3] =
{
    {0 ,7 },
    {11 ,18 },
    {22 , 29},
};

/* array that holds the info about the tcam key fields */
static CHT_PCL_KEY_FIELDS_INFO_STC cht2PclKeyFieldsData[CHT2_PCL_KEY_FIELDS_ID_LAST_E]=
{
    {0  ,0  ,GT_TRUE," CHT2_PCL_KEY_FIELDS_ID_VALID_E "  },
    {1  ,10 ,GT_TRUE ," CHT2_PCL_KEY_FIELDS_ID_PCL_ID_E "  },
    {11 ,16 ,GT_TRUE ," CHT2_PCL_KEY_FIELDS_ID_SRC_PORT_E "  },
    {17 ,17 ,GT_TRUE," CHT2_PCL_KEY_FIELDS_ID_IS_TAGGED_E "  },
    {18 ,29 ,GT_TRUE ," CHT2_PCL_KEY_FIELDS_ID_VID_E "  },
    {30 ,32 ,GT_TRUE ," CHT2_PCL_KEY_FIELDS_ID_UP_E "  },
    {33 ,39 ,GT_TRUE ," CHT2_PCL_KEY_FIELDS_ID_QOS_PROFILE_E "  },
    {40 ,40 ,GT_TRUE," CHT2_PCL_KEY_FIELDS_ID_IS_IPV4_E "  },
    {41 ,41 ,GT_TRUE," CHT2_PCL_KEY_FIELDS_ID_IS_IP_E "  },
    {42 ,42 ,GT_TRUE," CHT2_PCL_KEY_FIELDS_ID_RESERVED_42_E "  },
    {43 ,58 ,GT_TRUE," CHT2_PCL_KEY_FIELDS_ID_ETHER_TYPE_DSAP_SSAP_E "  },
    {59 ,66 ,GT_TRUE," CHT2_PCL_KEY_FIELDS_ID_USER_DEFINED_BYTE2_66_59_SHORT_E "},
    {67 ,72 ,GT_TRUE," CHT2_PCL_KEY_FIELDS_ID_RESERVED_72_67_E "  },
    {73 ,73 ,GT_TRUE," CHT2_PCL_KEY_FIELDS_ID_IS_ARP_E "  },
    {74 ,74 ,GT_TRUE," CHT2_PCL_KEY_FIELDS_ID_RESERVED_74_E "  },
    {75 ,82 ,GT_TRUE," CHT2_PCL_KEY_FIELDS_ID_USER_DEFINED_BYTE0_82_75_SHORT_E "},
    {83 ,90 ,GT_TRUE," CHT2_PCL_KEY_FIELDS_ID_USER_DEFINED_BYTE1_90_83_SHORT_E "},
    {91 ,91 ,GT_TRUE," CHT2_PCL_KEY_FIELDS_ID_ENCAP_TYPE_SHORT_E "  },
    {92 ,139,GT_TRUE," CHT2_PCL_KEY_FIELDS_ID_MAC_SA_SHORT_E "  },
    {140,187,GT_TRUE," CHT2_PCL_KEY_FIELDS_ID_MAC_DA_SHORT_E "  },
    {189,189,GT_TRUE," CHT2_PCL_KEY_FIELDS_ID_IS_L2_VALID_SHORT_E "  },
    {188,188,GT_TRUE," CHT2_PCL_KEY_FIELDS_ID_USER_DEFINED_VALID_SHORT_E " },
    {190,191,GT_TRUE," CHT2_PCL_KEY_FIELDS_ID_RESERVED_190_191_E "  },
    {42 ,49 ,GT_TRUE," CHT2_PCL_KEY_FIELDS_ID_IP_PROTOCOL_E "},
    {50 ,55 ,GT_TRUE," CHT2_PCL_KEY_FIELDS_ID_DSCP_E "} ,
    {56 ,56 ,GT_TRUE," CHT2_PCL_KEY_FIELDS_ID_IS_L4_VALID_E "},
    {57 ,72 ,GT_TRUE," CHT2_PCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_2_3_E "},
    {74 ,74 ,GT_TRUE," CHT2_PCL_KEY_FIELDS_ID_IS_IPV6_EH_EXISTS_SHORT_E "},
    {75 ,82 ,GT_TRUE," CHT2_PCL_KEY_FIELDS_ID_TCP_UDP_PORT_COMPS_SHORT_E "},
    {91 ,91 ,GT_TRUE," CHT2_PCL_KEY_FIELDS_ID_LONG_IS_IPV6_EH_HOP_BY_HOP_E, "},
    {190,190,GT_TRUE," CHT2_PCL_KEY_FIELDS_ID_IPV4_FRAGMENTED_SHORT_E "}   ,
    {74 ,74 ,GT_TRUE," CHT2_PCL_KEY_FIELDS_ID_IS_BC_E "},
    {91 ,98 ,GT_TRUE," CHT2_PCL_KEY_FIELDS_ID_USER_DEFINED_BYTE2_98_91_SHORT_E "}   ,
    {99 ,130,GT_TRUE," CHT2_PCL_KEY_FIELDS_ID_SIP_31_0_E "}   ,
    {131,162,GT_TRUE," CHT2_PCL_KEY_FIELDS_ID_DIP_31_0_E "},
    {163,186,GT_TRUE," CHT2_PCL_KEY_FIELDS_ID_L4_BYTES_OFFSETS_0_1_13_SHORT_E "} ,
    {187,187,GT_TRUE," CHT2_PCL_KEY_FIELDS_ID_RESERVED_187_E "},
    {191,191,GT_TRUE," CHT2_PCL_KEY_FIELDS_ID_IP_HEADER_OK_SHORT_E "},
    {40 ,40 ,GT_TRUE," CHT2_PCL_KEY_FIELDS_ID_IS_IPV6_E "}      ,
    {73 ,96 ,GT_TRUE," CHT2_PCL_KEY_FIELDS_ID_L4_BYTES_OFFSETS_0_1_13_LONG_E "},
    {97 ,128,GT_TRUE," CHT2_PCL_KEY_FIELDS_ID_SIP_31_0_LONG_E " },
    {129,160,GT_TRUE," CHT2_PCL_KEY_FIELDS_ID_DIP_31_0_LONG_E "},
    {161,161,GT_TRUE," CHT2_PCL_KEY_FIELDS_ID_ENCAP_TYPE_LONG_VALID_E "},
    {162,177,GT_TRUE," CHT2_PCL_KEY_FIELDS_ID_ETHER_TYPE_DSAP_SSAP_LONG_E "  },
    {178,178,GT_TRUE," CHT_PCL_KEY_FIELDS_ID_LONG_IPV4_FRAGMENT_E "} ,
    {179,234,GT_TRUE," CHT2_PCL_KEY_FIELDS_ID_LONG_RESERVED_179_234_E "},
    {235,282,GT_TRUE," CHT2_PCL_KEY_FIELDS_ID_MAC_SA_LONG_E "},
    {283,330,GT_TRUE," CHT2_PCL_KEY_FIELDS_ID_MAC_DA_LONG_E "},
    {331,338,GT_TRUE," CHT2_PCL_KEY_FIELDS_ID_USER_DEFINED_BYTE3_338_331_LONG_E "},
    {339,346,GT_TRUE," CHT2_PCL_KEY_FIELDS_ID_USER_DEFINED_BYTE4_346_339_LONG_E "},
    {347,354,GT_TRUE," CHT2_PCL_KEY_FIELDS_ID_USER_DEFINED_BYTE5_354_347_LONG_E "},
    {355,362,GT_TRUE," CHT2_PCL_KEY_FIELDS_ID_USER_DEFINED_BYTE0_362_355_LONG_E "},
    {363,370,GT_TRUE," CHT2_PCL_KEY_FIELDS_ID_USER_DEFINED_BYTE1_370_363_LONG_E "},
    {371,378,GT_TRUE," CHT2_PCL_KEY_FIELDS_ID_USER_DEFINED_BYTE2_378_371_LONG_E "},
    {379,380,GT_TRUE," CHT2_PCL_KEY_FIELDS_ID_RESERVED_380_379_LONG_E "},
    {381,381,GT_TRUE," CHT2_PCL_KEY_FIELDS_ID_IS_L2_VALID_LONG_E "} ,
    {382,382,GT_TRUE," CHT2_PCL_KEY_FIELDS_ID_USER_DEFINED_VALID_LONG_E "},
    {129,224,GT_TRUE," CHT2_PCL_KEY_FIELDS_ID_SIP_127_32_LONG_E "},
    {225,225,GT_TRUE," CHT2_PCL_KEY_FIELDS_ID_IS_IPV6_EH_EXISTS_LONG_E "},
    {226,226,GT_TRUE," CHT2_PCL_KEY_FIELDS_ID_IS_IPV6_EH_HOP_BY_HOP_LONG_E "},
    {120,127,GT_TRUE," CHT2_PCL_KEY_FIELDS_ID_DIP_127_120_LONG_E "} ,
    {235,322,GT_TRUE," CHT2_PCL_KEY_FIELDS_ID_DIP_119_32_LONG_E "},
    {323,354,GT_TRUE," CHT2_PCL_KEY_FIELDS_ID_DIP_0_31_IPV6_LONG_E "},
    {57 ,72 ,GT_TRUE," CHT2_PCL_KEY_FIELDS_ID_DIP_0_15_IPV6_SHORT_E "},
    {75 ,90 ,GT_TRUE," CHT2_PCL_KEY_FIELDS_ID_DIP_16_31_IPV6_SHORT_E "},
    {92 ,187,GT_TRUE," CHT2_PCL_KEY_FIELDS_ID_DIP_127_32_SHORT_E "},
    {188,188,GT_TRUE," CHT2_PCL_KEY_FIELDS_ID_RESERVED_188_E "} ,
    {190,190,GT_TRUE," CHT2_PCL_KEY_FIELDS_ID_RESERVED_190_E "} ,
    {383,383,GT_TRUE," CHT2_PCL_KEY_FIELDS_ID_IP_HEADER_OK_LONG_E "},
    {371,378,GT_TRUE," CHT2_PCL_KEY_FIELDS_ID_TCP_UDP_PORT_COMPS_LONG_E "},
    {57 ,72 ,GT_TRUE," CHT2_PCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_4_5_E "}
};

static GT_VOID snetCht2IPclConfigPtrGet
(
    IN SKERNEL_DEVICE_OBJECT                * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC      * descrPtr       ,
    OUT SNET_CHT2_IPOLICY_LOOKUP_CONFIG_STC * currentLookupConfPtr
);

/*******************************************************************************
*   snetChtPclKeyFieldBuildByPointer
*
* DESCRIPTION:
*       function insert data of field to the search key in specific place in key
*
* INPUTS:
*       pclKeyPtr - (pointer to) current pcl key
*       fieldValPtr - (pointer to) data of field to insert to key
*       fieldId -- field id
*       cycleNum -  the number of current cycle
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
static GT_VOID snetCht2IPclKeyFieldBuildByValue
(
    SNET_CHT_POLICY_KEY_STC        * pclKeyPtr,
    GT_U32                          fieldVal,
    CHT2_PCL_KEY_FIELDS_ID_ENT      fieldId
)
{
    CHT_PCL_KEY_FIELDS_INFO_STC *fieldInfoPtr = &cht2PclKeyFieldsData[fieldId] ;

    snetChtPclSrvKeyFieldBuildByValue(pclKeyPtr, fieldVal,  fieldInfoPtr);

    return ;
}

/*******************************************************************************
*   snetChtIPclKeyFieldBuildByPointer
*
* DESCRIPTION:
*       function insert data of field to the search key in specific place in key
*
* INPUTS:
*       pclKeyPtr   - (pointer to) current pcl key
*       fieldValPtr - (pointer to) data of field to insert to key
*       fieldId     - field id
*       cycleNum    - The number of current cycle
*
* OUTPUTS:
*       pclKeyPtr   - (pointer to) current pcl key
*
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID snetCht2IPclKeyFieldBuildByPointer
(
    INOUT SNET_CHT_POLICY_KEY_STC *pclKeyPtr,
    IN GT_U8                      *fieldValPtr,
    IN CHT2_PCL_KEY_FIELDS_ID_ENT  fieldId
)
{
    CHT_PCL_KEY_FIELDS_INFO_STC *fieldInfoPtr = &cht2PclKeyFieldsData[fieldId] ;

    snetChtPclSrvKeyFieldBuildByPointer(pclKeyPtr, fieldValPtr, fieldInfoPtr);

    return;
}

/*******************************************************************************
*   snetCht2IPclKeyBuildL2IPv6StandardKey
*
* DESCRIPTION:
*        Build IPv6 DIP standard PCL search key.
*
* INPUTS:
*       devObjPtr            - pointer to device object.
*       descrPtr             - pointer to frame descriptor.
*       currentLookupConfPtr - pointer to the current PCL configuration entry.
*       pclExtraDataPtr      - pointer to extra data needed for the I-PCL engine.
*       cycleNum             - cycle number
*
* OUTPUTS:
*       pclKeyPtr    - pointer to PCL search key structure.
*
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID snetCht2IPclKeyBuildL2IPv6StandardKey
(
    IN SKERNEL_DEVICE_OBJECT                       * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC             * descrPtr   ,
    IN SNET_CHT2_POLICY_SPECIFIC_LOOKUP_CONFIG_STC * currentLookupConfPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC               * pclExtraDataPtr       ,
    OUT SNET_CHT_POLICY_KEY_STC                    * pclKeyPtr
)
{
    GT_U32  fieldVal = 1;
    GT_U32  resrvdVal =0;

    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, fieldVal, /* bits[0] Valid */
                            CHT2_PCL_KEY_FIELDS_ID_VALID_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, currentLookupConfPtr->pclID ,
                            CHT2_PCL_KEY_FIELDS_ID_PCL_ID_E);/*[1:10] PCL-ID */
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->localDevSrcPort ,
                            CHT2_PCL_KEY_FIELDS_ID_SRC_PORT_E);/* [11:16] src port */
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->origSrcTagged ,/* [17] is tagged */
                            CHT2_PCL_KEY_FIELDS_ID_IS_TAGGED_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->eVid ,/* [18:29] eVid */
                            CHT2_PCL_KEY_FIELDS_ID_VID_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->up  ,/* [30:32] UP */
                            CHT2_PCL_KEY_FIELDS_ID_UP_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->qos.qosProfile,/* [33:39] QOS profile */
                            CHT2_PCL_KEY_FIELDS_ID_QOS_PROFILE_E);   /* [40] is IPv6 */
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->isIPv4 , /* [40] is IPv4 */
                            CHT2_PCL_KEY_FIELDS_ID_IS_IPV4_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->isIp,/* [41] is IP */
                            CHT2_PCL_KEY_FIELDS_ID_IS_IP_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->ipProt,/* [42:49] IP Protocol*/
                            CHT2_PCL_KEY_FIELDS_ID_IP_PROTOCOL_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dscp,/* [50:55] dscp */
                            CHT2_PCL_KEY_FIELDS_ID_DSCP_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL4Valid, /* [56] isL4Valid*/
                            CHT2_PCL_KEY_FIELDS_ID_IS_L4_VALID_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[3],  /* [57:72] DIP */
                            CHT2_PCL_KEY_FIELDS_ID_DIP_0_15_IPV6_SHORT_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->arp ,/* [73] isArp */
                            CHT2_PCL_KEY_FIELDS_ID_IS_ARP_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpV6EhExists,/* [74] ipv6_EH_exist */
                            CHT2_PCL_KEY_FIELDS_ID_IS_IPV6_EH_EXISTS_SHORT_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpV6EhHopByHop,/* [91] is IPv6 hopbyhop */
                            CHT2_PCL_KEY_FIELDS_ID_LONG_IS_IPV6_EH_HOP_BY_HOP_E);
    snetCht2IPclKeyFieldBuildByPointer(pclKeyPtr,(GT_U8*)&descrPtr->sip[0],/* [224:129] SIP*/
                                       CHT2_PCL_KEY_FIELDS_ID_SIP_127_32_LONG_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, resrvdVal,/* [188] reserved */
                            CHT2_PCL_KEY_FIELDS_ID_RESERVED_188_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL2Valid ,/* [189] isL2Valid  */
                            CHT2_PCL_KEY_FIELDS_ID_IS_L2_VALID_SHORT_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, resrvdVal,/* [190] reserved */
                            CHT2_PCL_KEY_FIELDS_ID_RESERVED_190_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpHeaderOk ,/* [191] ip header  */
                            CHT2_PCL_KEY_FIELDS_ID_IP_HEADER_OK_SHORT_E);

    return  ;
}

/*******************************************************************************
*   snetCht2IPclKeyBuildL4IPv6ExtendedKey
*
* DESCRIPTION:
*        Build Layer4+IPv6 extended PCL search key.
*
* INPUTS:
*       devObjPtr            - pointer to device object.
*       descrPtr             - pointer to frame data buffer Id.
*       currentLookupConfPtr - pointer to the curent PCL configuration entry.
*       pclExtraDataPtr      - pointer to extra data needed for the IPCL engine.
*       cycleNum             - cycle number.
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
static GT_VOID snetCht2IPclKeyBuildL4IPv6ExtendedKey
(
    IN SKERNEL_DEVICE_OBJECT                        * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC              * descrPtr      ,
    IN SNET_CHT2_POLICY_SPECIFIC_LOOKUP_CONFIG_STC  * currentLookupConfPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC                * pclExtraDataPtr  ,
    OUT SNET_CHT_POLICY_KEY_STC                     * pclKeyPtr
)
{
    DECLARE_FUNC_NAME(snetCht2IPclKeyBuildL4IPv6ExtendedKey);

    GT_U32      fieldVal;       /* valid bit value */
    GT_U32      resrvdVal;       /* reserved bit value */
    GT_U32      userDefdata0;
    CHT2_PCL_USER_DEFINED_CONFIG_REG_STC *userDefinedArrayPtr;
    GT_U32      userDefinedAnchor;/* user defined byte Anchor */
    GT_U32      userDefinedOffset;/* user defined byte offset from Anchor */
    GT_U8       byteValue;/* value of user defined byte from the packets or tcp/udp compare */
    GT_U32      userDefinedError = GT_FALSE;/* is there user defined byte error */
    GT_STATUS   rc; /* return code from function */
    GT_U32      tmpVal; /* temp val */
    GT_U8       tmpFieldVal[4];

    fieldVal = 1;
    resrvdVal = 0;
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, fieldVal, /* bits[0] Valid */
                            CHT2_PCL_KEY_FIELDS_ID_VALID_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, currentLookupConfPtr->pclID ,
                            CHT2_PCL_KEY_FIELDS_ID_PCL_ID_E);/*[10:1] PCL-ID */
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->localDevSrcPort ,
                            CHT2_PCL_KEY_FIELDS_ID_SRC_PORT_E);/* [16:11] src port */
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->origSrcTagged ,/* [17] is tagged */
                            CHT2_PCL_KEY_FIELDS_ID_IS_TAGGED_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->eVid ,/* [29:18] eVid */
                            CHT2_PCL_KEY_FIELDS_ID_VID_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->up  ,/* [32:30] UP */
                            CHT2_PCL_KEY_FIELDS_ID_UP_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->qos.qosProfile,/* [39:33] QOS profile */
                            CHT2_PCL_KEY_FIELDS_ID_QOS_PROFILE_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, (descrPtr->isIPv4==0 && descrPtr->isIp),
                            CHT2_PCL_KEY_FIELDS_ID_IS_IPV6_E); /* [40] is IPv6 */
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->isIp,/* [41] is IP */
                            CHT2_PCL_KEY_FIELDS_ID_IS_IP_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->ipProt,/* [49:42] IP Protocol*/
                            CHT2_PCL_KEY_FIELDS_ID_IP_PROTOCOL_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dscp,/* [55:50] dscp */
                            CHT2_PCL_KEY_FIELDS_ID_DSCP_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL4Valid, /* [56] isL4Valid*/
                            CHT2_PCL_KEY_FIELDS_ID_IS_L4_VALID_E);
    if (descrPtr->l4StartOffsetPtr != NULL)
    {
        if (descrPtr->ipv6Icmp || descrPtr->ipv4Icmp)
        {
            snetCht2IPclKeyFieldBuildByPointer(/* [72:57] L4 Byte Offsets 5:4 */
                pclKeyPtr,(GT_U8*)&(descrPtr->l4StartOffsetPtr[4]),
                CHT2_PCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_4_5_E);
        }
        else
        {
            snetCht2IPclKeyFieldBuildByPointer(/* [72:57] L4 Byte Offsets 3:2 */
                pclKeyPtr,(GT_U8*)&(descrPtr->l4StartOffsetPtr[2]),
                CHT2_PCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_2_3_E);
        }

        tmpFieldVal[0] = descrPtr->l4StartOffsetPtr[0];
        tmpFieldVal[1] = descrPtr->l4StartOffsetPtr[1];
        tmpFieldVal[2] = descrPtr->l4StartOffsetPtr[13];
        snetCht2IPclKeyFieldBuildByPointer(
            pclKeyPtr,(GT_U8*)&tmpFieldVal[0],/* [96:73] L4 Byte Offsets[1:0],[13] */
            CHT2_PCL_KEY_FIELDS_ID_L4_BYTES_OFFSETS_0_1_13_LONG_E);
    }
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[3], /* [128:97] SIP*/
                            CHT2_PCL_KEY_FIELDS_ID_SIP_31_0_LONG_E);
    snetCht2IPclKeyFieldBuildByPointer(pclKeyPtr,(GT_U8*)&descrPtr->sip[0],/* [224:129] SIP*/
                                       CHT2_PCL_KEY_FIELDS_ID_SIP_127_32_LONG_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpV6EhExists, /* [225] ipv6_eh_exist */
                            CHT2_PCL_KEY_FIELDS_ID_IS_IPV6_EH_EXISTS_LONG_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpV6EhHopByHop, /* [226] ipv6_eh_hopbyhop */
                            CHT2_PCL_KEY_FIELDS_ID_IS_IPV6_EH_HOP_BY_HOP_LONG_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, (descrPtr->dip[0] >> 24),  /* [234:227] DIP */
                            CHT2_PCL_KEY_FIELDS_ID_DIP_127_120_LONG_E);
    snetCht2IPclKeyFieldBuildByPointer(pclKeyPtr,(GT_U8*)&descrPtr->dip[0], /* [322:235] DIP */
                            CHT2_PCL_KEY_FIELDS_ID_DIP_119_32_LONG_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->dip[3], /* [354:323] DIP */
                            CHT2_PCL_KEY_FIELDS_ID_DIP_0_31_IPV6_LONG_E);

    smemRegGet(devObjPtr,SMEM_CHT2_KEY5_USERDEFINED_BYTES_LONG_L4_IPv6_KEY_REG(devObjPtr),&userDefdata0);
    /* Key2 User Define Byte0  */
    __LOG(("Key2 User Define Byte0"));
    userDefinedArrayPtr = cht2PclUserDefinedConfigInfoLongL4Ipv6KeyArray;
    userDefinedAnchor = SMEM_U32_GET_FIELD(userDefdata0,userDefinedArrayPtr[0].startBitAnchor,2);
    userDefinedOffset = SMEM_U32_GET_FIELD(userDefdata0,userDefinedArrayPtr[0].startBitOffset,7);

    rc = snetChtPclUserDefinedByteGet(devObjPtr,descrPtr,userDefinedAnchor,
                                userDefinedOffset,&byteValue);
    if(rc != GT_OK)
    {
        userDefinedError = GT_TRUE;
        byteValue = 0;
    }

    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [362:355] UserDefinedByte0 */
                        CHT2_PCL_KEY_FIELDS_ID_USER_DEFINED_BYTE0_362_355_LONG_E);

    /* Key2 User Define Byte1  */
    __LOG(("Key2 User Define Byte1"));
    userDefinedAnchor = SMEM_U32_GET_FIELD(userDefdata0,userDefinedArrayPtr[1].startBitAnchor,2);
    userDefinedOffset = SMEM_U32_GET_FIELD(userDefdata0,userDefinedArrayPtr[1].startBitOffset,7);

    rc = snetChtPclUserDefinedByteGet(devObjPtr,descrPtr,userDefinedAnchor,
                                userDefinedOffset,&byteValue);
    if(rc != GT_OK)
    {
        userDefinedError = GT_TRUE;
        byteValue = 0;
    }

    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [370:363] UserDefinedByte1 */
                        CHT2_PCL_KEY_FIELDS_ID_USER_DEFINED_BYTE1_370_363_LONG_E);

    if (userDefdata0 & (1 << 0x1F))
    {/* TCP/UDP Range Comparators is enable */
        rc = snetChtIPclTcpUdpPortRangeCompareGet(devObjPtr,descrPtr,&byteValue);
        byteValue = (rc != GT_OK) ? 0 : byteValue ;
        snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [378:371] TCP/UDP comperator */
                            CHT2_PCL_KEY_FIELDS_ID_TCP_UDP_PORT_COMPS_LONG_E);
    }
    else
    {   /* Key2 User Define Byte2  */
        __LOG(("Key2 User Define Byte2"));
        userDefinedArrayPtr = cht2PclUserDefinedConfigInfoShortL3L4KeyArray;
        userDefinedAnchor = SMEM_U32_GET_FIELD(userDefdata0,userDefinedArrayPtr[2].startBitAnchor,2);
        userDefinedOffset = SMEM_U32_GET_FIELD(userDefdata0,userDefinedArrayPtr[2].startBitOffset,7);

        rc = snetChtPclUserDefinedByteGet(devObjPtr,descrPtr,userDefinedAnchor,
                                    userDefinedOffset,&byteValue);
        if(rc != GT_OK)
        {
            userDefinedError = GT_TRUE;
            byteValue = 0;
        }

        snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [378:371] UserDefinedByte2 */
                            CHT2_PCL_KEY_FIELDS_ID_USER_DEFINED_BYTE2_378_371_LONG_E);
    }

    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, resrvdVal,/* Reserved [380:379] */
                            CHT2_PCL_KEY_FIELDS_ID_RESERVED_380_379_LONG_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr,pclExtraDataPtr->isL2Valid ,/* is L2 valid [381] */
                            CHT2_PCL_KEY_FIELDS_ID_IS_L2_VALID_LONG_E);
    tmpVal = (userDefinedError == GT_TRUE) ? 0 : 1;
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, tmpVal,/* [382] UserDefineValid */
                            CHT2_PCL_KEY_FIELDS_ID_USER_DEFINED_VALID_LONG_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpHeaderOk ,
                            CHT2_PCL_KEY_FIELDS_ID_IP_HEADER_OK_LONG_E);/* [383] ip header ok */

    return  ;
}

/*******************************************************************************
*   snetChtPclKeyBuildL2IPv6ExtendedKey
*
* DESCRIPTION:
*        Build Layer2+IPv6 extended PCL search key.
*
* INPUTS:
*       devObjPtr            - pointer to device object.
*       descrPtr             - pointer to frame data buffer Id.
*       currentLookupConfPtr - pointer to the curent PCL configuration entry.
*       pclExtraDataPtr      - pointer to extra data needed for the IPCL engine.
*       cycleNum             - cycle number
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
static GT_VOID snetCht2IPclKeyBuildL2IPv6ExtendedKey
(
    IN SKERNEL_DEVICE_OBJECT                        * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC              * descrPtr   ,
    IN SNET_CHT2_POLICY_SPECIFIC_LOOKUP_CONFIG_STC  * currentLookupConfPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC                * pclExtraDataPtr ,
    OUT SNET_CHT_POLICY_KEY_STC                     * pclKeyPtr
)
{
    DECLARE_FUNC_NAME(snetCht2IPclKeyBuildL2IPv6ExtendedKey);

    GT_U32      fieldVal;          /* valid bit value */
    GT_U32      resrvdVal;         /* reserved bit value */
    GT_U32      userDefdata0;      /* user defined data */
    CHT2_PCL_USER_DEFINED_CONFIG_REG_STC *userDefinedArrayPtr;
    GT_U32      userDefinedAnchor; /* user defined byte Anchor */
    GT_U32      userDefinedOffset; /* user defined byte offset from Anchor */
    GT_U8       byteValue;         /* value of user defined byte from the packets or tcp/udp compare */
    GT_U32      userDefinedError =GT_FALSE;/* is there user defined byte error */
    GT_STATUS   rc;                /* return code from function */
    GT_U32      tmpVal;
    GT_U8       tmpFieldVal[4];


    fieldVal = 1;
    resrvdVal = 0;
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, fieldVal, /* bits[0] Valid */
                            CHT2_PCL_KEY_FIELDS_ID_VALID_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, currentLookupConfPtr->pclID ,
                            CHT2_PCL_KEY_FIELDS_ID_PCL_ID_E);/* [10:1] PCL-ID */
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->localDevSrcPort ,
                            CHT2_PCL_KEY_FIELDS_ID_SRC_PORT_E);/* [16:11] src port */
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->origSrcTagged ,/* [17] is tagged */
                            CHT2_PCL_KEY_FIELDS_ID_IS_TAGGED_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->eVid ,/* [29:18] eVid */
                            CHT2_PCL_KEY_FIELDS_ID_VID_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->up  ,/* [32:30] UP */
                            CHT2_PCL_KEY_FIELDS_ID_UP_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->qos.qosProfile,/* [39:33] QOS profile */
                            CHT2_PCL_KEY_FIELDS_ID_QOS_PROFILE_E);   /* [40] is IPv6 */
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, (descrPtr->isIPv4==0 && descrPtr->isIp),
                            CHT2_PCL_KEY_FIELDS_ID_IS_IPV6_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->isIp,/* [41] is IP */
                            CHT2_PCL_KEY_FIELDS_ID_IS_IP_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->ipProt,/* [49:42] IP Protocol*/
                            CHT2_PCL_KEY_FIELDS_ID_IP_PROTOCOL_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dscp,/* [55:50] dscp */
                            CHT2_PCL_KEY_FIELDS_ID_DSCP_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL4Valid, /* [56] isL4Valid*/
                            CHT2_PCL_KEY_FIELDS_ID_IS_L4_VALID_E);
    if (descrPtr->l4StartOffsetPtr != NULL)
    {
        if (descrPtr->ipv6Icmp || descrPtr->ipv4Icmp)
        {
            snetCht2IPclKeyFieldBuildByPointer(/* [72:57] L4 Byte Offsets 5:4 */
                pclKeyPtr,(GT_U8*)&(descrPtr->l4StartOffsetPtr[4]),
                    CHT2_PCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_4_5_E);
        }
        else
        {
            snetCht2IPclKeyFieldBuildByPointer(/* [72:57] L4 Byte Offsets 3:2 */
                pclKeyPtr,(GT_U8*)&(descrPtr->l4StartOffsetPtr[2]),
                    CHT2_PCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_2_3_E);
        }
        tmpFieldVal[0] = descrPtr->l4StartOffsetPtr[0];
        tmpFieldVal[1] = descrPtr->l4StartOffsetPtr[1];
        tmpFieldVal[2] = descrPtr->l4StartOffsetPtr[13];
        snetCht2IPclKeyFieldBuildByPointer(
            pclKeyPtr,(GT_U8*)&tmpFieldVal[0],/* [73:96] L4 Byte Offsets[1:0],[13] */
                CHT2_PCL_KEY_FIELDS_ID_L4_BYTES_OFFSETS_0_1_13_LONG_E);
    }

    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[3], /* [128:97] SIP*/
                            CHT2_PCL_KEY_FIELDS_ID_SIP_31_0_LONG_E);
    snetCht2IPclKeyFieldBuildByPointer(pclKeyPtr,(GT_U8*)&descrPtr->sip[0],/* [224:129] SIP*/
                                       CHT2_PCL_KEY_FIELDS_ID_SIP_127_32_LONG_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpV6EhExists, /* [225] ipv6_eh_exist */
                            CHT2_PCL_KEY_FIELDS_ID_IS_IPV6_EH_EXISTS_LONG_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpV6EhHopByHop, /* [226] ipv6_eh_hopbyhop */
                            CHT2_PCL_KEY_FIELDS_ID_IS_IPV6_EH_HOP_BY_HOP_LONG_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, (descrPtr->dip[0] >> 24),  /* [234:227] DIP */
                            CHT2_PCL_KEY_FIELDS_ID_DIP_127_120_LONG_E);
    snetCht2IPclKeyFieldBuildByPointer(pclKeyPtr, descrPtr->macSaPtr,/* [282:235] MAC SA*/
                            CHT2_PCL_KEY_FIELDS_ID_MAC_SA_LONG_E);
    snetCht2IPclKeyFieldBuildByPointer(pclKeyPtr, descrPtr->macDaPtr,/* [330:283] MAC DA*/
                            CHT2_PCL_KEY_FIELDS_ID_MAC_DA_LONG_E);

    smemRegGet(devObjPtr,SMEM_CHT2_KEY1_0_USERDEFINED_BYTES_LONG_L2_IPv6_KEY_REG(devObjPtr),&userDefdata0);
    /* Key0 User Define Byte3  */
    __LOG(("Key0 User Define Byte3"));
    userDefinedArrayPtr = cht2PclUserDefinedConfigInfoLongL2Ipv6KeyArray;
    userDefinedAnchor = SMEM_U32_GET_FIELD(userDefdata0,userDefinedArrayPtr[3].startBitAnchor,2);
    userDefinedOffset = SMEM_U32_GET_FIELD(userDefdata0,userDefinedArrayPtr[3].startBitOffset,7);

    rc = snetChtPclUserDefinedByteGet(devObjPtr,descrPtr,userDefinedAnchor,
                                userDefinedOffset,&byteValue);
    if(rc != GT_OK)
    {
        userDefinedError = GT_TRUE;
        byteValue = 0;
    }

    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [338:331] UserDefinedByte3 */
                        CHT2_PCL_KEY_FIELDS_ID_USER_DEFINED_BYTE3_338_331_LONG_E);

    /* Key0 User Define Byte4  */
    __LOG(("Key0 User Define Byte4"));
    userDefinedAnchor = SMEM_U32_GET_FIELD(userDefdata0,userDefinedArrayPtr[4].startBitAnchor,2);
    userDefinedOffset = SMEM_U32_GET_FIELD(userDefdata0,userDefinedArrayPtr[4].startBitOffset,7);

    rc = snetChtPclUserDefinedByteGet(devObjPtr,descrPtr,userDefinedAnchor,
                                userDefinedOffset,&byteValue);
    if(rc != GT_OK)
    {
        userDefinedError = GT_TRUE;
        byteValue = 0;
    }

    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [346:339] UserDefinedByte4 */
                        CHT2_PCL_KEY_FIELDS_ID_USER_DEFINED_BYTE4_346_339_LONG_E);

    /* Key0 User Define Byte5  */
    __LOG(("Key0 User Define Byte5"));
    userDefinedAnchor = SMEM_U32_GET_FIELD(userDefdata0,userDefinedArrayPtr[5].startBitAnchor,2);
    userDefinedOffset = SMEM_U32_GET_FIELD(userDefdata0,userDefinedArrayPtr[5].startBitOffset,7);

    rc = snetChtPclUserDefinedByteGet(devObjPtr,descrPtr,userDefinedAnchor,
                                userDefinedOffset,&byteValue);
    if(rc != GT_OK)
    {
        userDefinedError = GT_TRUE;
        byteValue = 0;
    }

    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [354:347] UserDefinedByte5 */
                        CHT2_PCL_KEY_FIELDS_ID_USER_DEFINED_BYTE5_354_347_LONG_E);

    /* Key0 User Define Byte0  */
    __LOG(("Key0 User Define Byte0"));
    smemRegGet(devObjPtr,SMEM_CHT2_KEY1_1_USERDEFINED_BYTES_LONG_L2_IPv6_KEY_REG(devObjPtr),&userDefdata0);
    userDefinedAnchor = SMEM_U32_GET_FIELD(userDefdata0,userDefinedArrayPtr[0].startBitAnchor,2);
    userDefinedOffset = SMEM_U32_GET_FIELD(userDefdata0,userDefinedArrayPtr[0].startBitOffset,7);

    rc = snetChtPclUserDefinedByteGet(devObjPtr,descrPtr,userDefinedAnchor,
                                userDefinedOffset,&byteValue);
    if(rc != GT_OK)
    {
        userDefinedError = GT_TRUE;
        byteValue = 0;
    }

    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [355:362] UserDefinedByte0 */
                        CHT2_PCL_KEY_FIELDS_ID_USER_DEFINED_BYTE0_362_355_LONG_E);

    /* Key0 User Define Byte1  */
    __LOG(("Key0 User Define Byte1"));
    userDefinedAnchor = SMEM_U32_GET_FIELD(userDefdata0,userDefinedArrayPtr[1].startBitAnchor,2);
    userDefinedOffset = SMEM_U32_GET_FIELD(userDefdata0,userDefinedArrayPtr[1].startBitOffset,7);

    rc = snetChtPclUserDefinedByteGet(devObjPtr,descrPtr,userDefinedAnchor,
                                userDefinedOffset,&byteValue);
    if(rc != GT_OK)
    {
        userDefinedError = GT_TRUE;
        byteValue = 0;
    }

    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [370:363] UserDefinedByte1 */
                        CHT2_PCL_KEY_FIELDS_ID_USER_DEFINED_BYTE1_370_363_LONG_E);
    if (userDefdata0 & (1 << 0x1F))
    {/* TCP/UDP Range Comparators is enable */
        __LOG(("TCP/UDP Range Comparators is enable"));
        rc = snetChtIPclTcpUdpPortRangeCompareGet(devObjPtr,descrPtr,&byteValue);
        byteValue = (rc != GT_OK) ? 0 : byteValue ;
        snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [378:371] TCP/UDP comperator */
                            CHT2_PCL_KEY_FIELDS_ID_TCP_UDP_PORT_COMPS_LONG_E);
    }
    else
    {   /* Key5 User Define Byte2  */
        __LOG(("Key5 User Define Byte2"));
        userDefinedArrayPtr = cht2PclUserDefinedConfigInfoShortL3L4KeyArray;
        userDefinedAnchor = SMEM_U32_GET_FIELD(userDefdata0,userDefinedArrayPtr[2].startBitAnchor,2);
        userDefinedOffset = SMEM_U32_GET_FIELD(userDefdata0,userDefinedArrayPtr[2].startBitOffset,7);

        rc = snetChtPclUserDefinedByteGet(devObjPtr,descrPtr,userDefinedAnchor,
                                    userDefinedOffset,&byteValue);
        if(rc != GT_OK)
        {
            userDefinedError = GT_TRUE;
            byteValue = 0;
        }
        snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [371:378] UserDefinedByte2 */
                            CHT2_PCL_KEY_FIELDS_ID_USER_DEFINED_BYTE2_378_371_LONG_E);
    }

    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, resrvdVal,/* Reserved [379:380] */
                            CHT2_PCL_KEY_FIELDS_ID_RESERVED_380_379_LONG_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, resrvdVal,/* is L2 valid [381] */
                            CHT2_PCL_KEY_FIELDS_ID_IS_L2_VALID_LONG_E);
    tmpVal = (userDefinedError == GT_TRUE) ? 0 : 1;
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, tmpVal,/* [382] UserDefineValid */
                            CHT2_PCL_KEY_FIELDS_ID_USER_DEFINED_VALID_LONG_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpHeaderOk ,
                            CHT2_PCL_KEY_FIELDS_ID_IP_HEADER_OK_LONG_E);/* [383] ip header  */

    return  ;
}

/*******************************************************************************
*   snetCht2IPclBuildL2L3L4ExtendedKey
*
* DESCRIPTION:
*        Build layer2+IPv4+layer4 extended PCL search key.
*
* INPUTS:
*       devObjPtr            - pointer to device object.
*       descrPtr             - pointer to frame data buffer Id.
*       currentLookupConfPtr - pointer to the curent PCL configuration entry.
*       pclExtraDataPtr      - pointer to extra data needed for the IPCL engine.
*       cycleNum             - cycle number
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
static GT_VOID snetCht2IPclBuildL2L3L4ExtendedKey
(
    IN SKERNEL_DEVICE_OBJECT                        * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC              * descrPtr    ,
    IN SNET_CHT2_POLICY_SPECIFIC_LOOKUP_CONFIG_STC  * currentLookupConfPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC                * pclExtraDataPtr  ,
    OUT SNET_CHT_POLICY_KEY_STC                     * pclKeyPtr
)
{
    DECLARE_FUNC_NAME(snetCht2IPclBuildL2L3L4ExtendedKey);

    GT_U32              fieldVal;    /* valid bit value */
    GT_U32              resrvdVal;   /* reserved bit value */
    GT_U32              userDefdata0;
    CHT2_PCL_USER_DEFINED_CONFIG_REG_STC *userDefinedArrayPtr;
    GT_U32              userDefinedAnchor; /* user defined byte Anchor */
    GT_U32              userDefinedOffset; /* user defined byte offset from Anchor */
    GT_U8               byteValue; /* value of user defined byte from the packets or tcp/udp compare */
    GT_U32              userDefinedError =GT_FALSE; /* is there user defined byte error */
    GT_STATUS           rc; /* status from called function */
    GT_U32              tmpVal; /* temp val */
    GT_U8               tmpFieldVal[4];


    fieldVal = 1;
    resrvdVal = 0;
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, fieldVal, /* bits[0] Valid */
                            CHT2_PCL_KEY_FIELDS_ID_VALID_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, currentLookupConfPtr->pclID ,
                            CHT2_PCL_KEY_FIELDS_ID_PCL_ID_E);/*[10:1] PCL-ID */
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->localDevSrcPort ,
                            CHT2_PCL_KEY_FIELDS_ID_SRC_PORT_E);/* [16:11] src port */
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->origSrcTagged ,/* [17] is tagged */
                            CHT2_PCL_KEY_FIELDS_ID_IS_TAGGED_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->eVid ,/* [29:18] eVid */
                            CHT2_PCL_KEY_FIELDS_ID_VID_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->up  ,/* [32:30] UP */
                            CHT2_PCL_KEY_FIELDS_ID_UP_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->qos.qosProfile,/* [39:33] QOS profile */
                            CHT2_PCL_KEY_FIELDS_ID_QOS_PROFILE_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, (descrPtr->isIPv4==0 && descrPtr->isIp),
                            CHT2_PCL_KEY_FIELDS_ID_IS_IPV6_E); /* [40] is IPv6 */
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->isIp,/* [41] is IP */
                            CHT2_PCL_KEY_FIELDS_ID_IS_IP_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->ipProt,/* [49:42] IP Protocol*/
                            CHT2_PCL_KEY_FIELDS_ID_IP_PROTOCOL_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dscp,/* [55:50] dscp */
                            CHT2_PCL_KEY_FIELDS_ID_DSCP_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL4Valid, /* [56] isL4Valid*/
                            CHT2_PCL_KEY_FIELDS_ID_IS_L4_VALID_E);
    if (descrPtr->l4StartOffsetPtr != NULL)
    {
        if (descrPtr->ipv6Icmp || descrPtr->ipv4Icmp)
        {
            snetCht2IPclKeyFieldBuildByPointer(/* [72:57] L4 Byte Offsets 5:4 */
                pclKeyPtr,(GT_U8*)&(descrPtr->l4StartOffsetPtr[4]),
                        CHT2_PCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_4_5_E);
        }
        else
        {
            snetCht2IPclKeyFieldBuildByPointer(/* [72:57] L4 Byte Offsets 3:2 */
                pclKeyPtr,(GT_U8*)&(descrPtr->l4StartOffsetPtr[2]),
                        CHT2_PCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_2_3_E);
        }
        tmpFieldVal[0] = descrPtr->l4StartOffsetPtr[0];
        tmpFieldVal[1] = descrPtr->l4StartOffsetPtr[1];
        tmpFieldVal[2] = descrPtr->l4StartOffsetPtr[13];
        snetCht2IPclKeyFieldBuildByPointer(
            pclKeyPtr,(GT_U8*)&tmpFieldVal[0],/* [96:73] L4 Byte Offsets[1:0],[13] */
                CHT2_PCL_KEY_FIELDS_ID_L4_BYTES_OFFSETS_0_1_13_LONG_E);
    }

    /* for IPv4 packets only the first bye sip[0] or dip[0] is relevant */
    __LOG(("for IPv4 packets only the first bye sip[0] or dip[0] is relevant"));
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[0], /* [128:97] SIP*/
                            CHT2_PCL_KEY_FIELDS_ID_SIP_31_0_LONG_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[0], /* [160:129] DIP*/
                            CHT2_PCL_KEY_FIELDS_ID_DIP_31_0_LONG_E);
    tmpVal = (descrPtr->l2Encaps == SKERNEL_ETHERNET_II_E) ? 1 : 0;
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, tmpVal,/* [161] Encap Type */
                        CHT2_PCL_KEY_FIELDS_ID_ENCAP_TYPE_LONG_VALID_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->etherTypeOrSsapDsap,/* [177:162] */
                        CHT2_PCL_KEY_FIELDS_ID_ETHER_TYPE_DSAP_SSAP_LONG_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpv4Fragment,/* [178] */
                        CHT2_PCL_KEY_FIELDS_ID_LONG_IPV4_FRAGMENT_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, resrvdVal,/* [234:179] reserved */
                        CHT2_PCL_KEY_FIELDS_ID_LONG_RESERVED_179_234_E);
    snetCht2IPclKeyFieldBuildByPointer(pclKeyPtr, descrPtr->macSaPtr,/* [235:282] MAC SA*/
                            CHT2_PCL_KEY_FIELDS_ID_MAC_SA_LONG_E);
    snetCht2IPclKeyFieldBuildByPointer(pclKeyPtr, descrPtr->macDaPtr,/* [330:283] MAC DA*/
                            CHT2_PCL_KEY_FIELDS_ID_MAC_DA_LONG_E);

    smemRegGet(devObjPtr,SMEM_CHT2_KEY5_0_USERDEFINED_BYTES_LONG_L3_L4_KEY_REG(devObjPtr),&userDefdata0);
    /* Key0 User Define Byte3  */
    __LOG(("Key0 User Define Byte3"));
    userDefinedArrayPtr = cht2PclUserDefinedConfigInfoLongL2L3L4KeyArray;
    userDefinedAnchor = SMEM_U32_GET_FIELD(userDefdata0,userDefinedArrayPtr[3].startBitAnchor,2);
    userDefinedOffset = SMEM_U32_GET_FIELD(userDefdata0,userDefinedArrayPtr[3].startBitOffset,7);

    rc = snetChtPclUserDefinedByteGet(devObjPtr,descrPtr,userDefinedAnchor,
                                         userDefinedOffset,&byteValue);
    if(rc != GT_OK)
    {
        userDefinedError = GT_TRUE;
        byteValue = 0;
    }

    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [338:331] UserDefinedByte3 */
                        CHT2_PCL_KEY_FIELDS_ID_USER_DEFINED_BYTE3_338_331_LONG_E);

    /* Key0 User Define Byte4  */
    __LOG(("Key0 User Define Byte4"));
    userDefinedAnchor = SMEM_U32_GET_FIELD(userDefdata0,userDefinedArrayPtr[4].startBitAnchor,2);
    userDefinedOffset = SMEM_U32_GET_FIELD(userDefdata0,userDefinedArrayPtr[4].startBitOffset,7);

    rc = snetChtPclUserDefinedByteGet(devObjPtr,descrPtr,userDefinedAnchor,
                                userDefinedOffset,&byteValue);
    if(rc != GT_OK)
    {
        userDefinedError = GT_TRUE;
        byteValue = 0;
    }

    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [346:339] UserDefinedByte4 */
                        CHT2_PCL_KEY_FIELDS_ID_USER_DEFINED_BYTE4_346_339_LONG_E);

    /* Key0 User Define Byte5  */
    __LOG(("Key0 User Define Byte5"));
    userDefinedAnchor = SMEM_U32_GET_FIELD(userDefdata0,userDefinedArrayPtr[5].startBitAnchor,2);
    userDefinedOffset = SMEM_U32_GET_FIELD(userDefdata0,userDefinedArrayPtr[5].startBitOffset,7);

    rc = snetChtPclUserDefinedByteGet(devObjPtr,descrPtr,userDefinedAnchor,
                                            userDefinedOffset,&byteValue);
    if(rc != GT_OK)
    {
        userDefinedError = GT_TRUE;
        byteValue = 0;
    }

    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [354:347] UserDefinedByte5 */
                        CHT2_PCL_KEY_FIELDS_ID_USER_DEFINED_BYTE5_354_347_LONG_E);

    /* Key0 User Define Byte0  */
    __LOG(("Key0 User Define Byte0"));
    smemRegGet(devObjPtr,SMEM_CHT2_KEY5_1_USERDEFINED_BYTES_LONG_L3_L4_KEY_REG(devObjPtr),&userDefdata0);
    userDefinedAnchor = SMEM_U32_GET_FIELD(userDefdata0,userDefinedArrayPtr[0].startBitAnchor,2);
    userDefinedOffset = SMEM_U32_GET_FIELD(userDefdata0,userDefinedArrayPtr[0].startBitOffset,7);

    rc = snetChtPclUserDefinedByteGet(devObjPtr,descrPtr,userDefinedAnchor,
                                        userDefinedOffset,&byteValue);
    if(rc != GT_OK)
    {
        userDefinedError = GT_TRUE;
        byteValue = 0;
    }

    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [362:355] UserDefinedByte0 */
                        CHT2_PCL_KEY_FIELDS_ID_USER_DEFINED_BYTE0_362_355_LONG_E);

    /* Key0 User Define Byte1  */
    __LOG(("Key0 User Define Byte1"));
    userDefinedAnchor = SMEM_U32_GET_FIELD(userDefdata0,userDefinedArrayPtr[1].startBitAnchor,2);
    userDefinedOffset = SMEM_U32_GET_FIELD(userDefdata0,userDefinedArrayPtr[1].startBitOffset,7);

    rc = snetChtPclUserDefinedByteGet(devObjPtr,descrPtr,userDefinedAnchor,
                                            userDefinedOffset,&byteValue);
    if(rc != GT_OK)
    {
        userDefinedError = GT_TRUE;
        byteValue = 0;
    }

    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [370:363] UserDefinedByte1 */
                        CHT2_PCL_KEY_FIELDS_ID_USER_DEFINED_BYTE1_370_363_LONG_E);
    if (userDefdata0 & (1 << 0x1F))
    {/* TCP/UDP Range Comparators is enable */
        __LOG(("TCP/UDP Range Comparators is enable"));
        rc = snetChtIPclTcpUdpPortRangeCompareGet(devObjPtr,descrPtr,&byteValue);
        byteValue = (rc != GT_OK) ? 0 : byteValue ;
        snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [378:371] TCP/UDP comperator */
                            CHT2_PCL_KEY_FIELDS_ID_TCP_UDP_PORT_COMPS_LONG_E);
    }
    else
    {   /* Key5 User Define Byte2  */
        __LOG(("Key5 User Define Byte2"));
        userDefinedArrayPtr = cht2PclUserDefinedConfigInfoShortL3L4KeyArray;
        userDefinedAnchor = SMEM_U32_GET_FIELD(userDefdata0,userDefinedArrayPtr[2].startBitAnchor,2);
        userDefinedOffset = SMEM_U32_GET_FIELD(userDefdata0,userDefinedArrayPtr[2].startBitOffset,7);

        rc = snetChtPclUserDefinedByteGet(devObjPtr,descrPtr,userDefinedAnchor,
                                            userDefinedOffset,&byteValue);
        if(rc != GT_OK)
        {
            userDefinedError = GT_TRUE;
            byteValue = 0;
        }

        snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [378:371] UserDefinedByte0 */
                            CHT2_PCL_KEY_FIELDS_ID_USER_DEFINED_BYTE2_378_371_LONG_E);
    }

    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, resrvdVal,/* Reserved [380:379] */
                            CHT2_PCL_KEY_FIELDS_ID_RESERVED_380_379_LONG_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, resrvdVal,/* is L2 valid [381] */
                            CHT2_PCL_KEY_FIELDS_ID_IS_L2_VALID_LONG_E);
    tmpVal = (userDefinedError == GT_TRUE) ? 0 : 1;
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, tmpVal,/* [382] UserDefineValid */
                        CHT2_PCL_KEY_FIELDS_ID_USER_DEFINED_VALID_LONG_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpHeaderOk ,
                            CHT2_PCL_KEY_FIELDS_ID_IP_HEADER_OK_LONG_E);/* [383] ip header ok */

    return ;
}

/*******************************************************************************
*   snetCht2IPclBuildL3L4StandardKey
*
* DESCRIPTION:
*        Build IPv4+Layer4 standard PCL search key.
*
* INPUTS:
*       devObjPtr            - pointer to device object.
*       descrPtr             - pointer to frame data buffer Id.
*       currentLookupConfPtr - pointer to the curent PCL configuration entry.
*       pclExtraDataPtr      - pointer to extra data needed for the IPCL engine.
*       cycleNum             - cycle number
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
static GT_VOID snetCht2IPclBuildL3L4StandardKey
(
    IN SKERNEL_DEVICE_OBJECT                        * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC              * descrPtr     ,
    IN SNET_CHT2_POLICY_SPECIFIC_LOOKUP_CONFIG_STC  * currentLookupConfPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC                * pclExtraDataPtr  ,
    OUT SNET_CHT_POLICY_KEY_STC                     * pclKeyPtr
)
{
    DECLARE_FUNC_NAME(snetCht2IPclBuildL3L4StandardKey);

    GT_U32                  fieldVal;       /* valid bit value */
    GT_U32                  resrvdVal;       /* reserved bit value */
    GT_U32                  userDefdata5;
    CHT2_PCL_USER_DEFINED_CONFIG_REG_STC *userDefinedArrayPtr;
    GT_U32                  userDefinedAnchor; /* user defined byte Anchor */
    GT_U32                  userDefinedOffset; /* user defined byte offset from Anchor */
    GT_U8                   byteValue; /* value of user defined byte from the
                                         packets or tcp/udp compare */
    GT_U32                  userDefinedError =GT_FALSE;
    GT_STATUS               rc;  /* status from called function */
    GT_U32                  tmpVal; /* temp val */
    GT_U8                   tmpFieldVal[4];/* temp val */

    fieldVal = 1;
    resrvdVal = 0;

    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, fieldVal, /* bits[0] Valid */
                            CHT2_PCL_KEY_FIELDS_ID_VALID_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, currentLookupConfPtr->pclID ,
                            CHT2_PCL_KEY_FIELDS_ID_PCL_ID_E);/*[10:1] PCL-ID */
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->localDevSrcPort ,
                            CHT2_PCL_KEY_FIELDS_ID_SRC_PORT_E);/* [16:11] src port */
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->origSrcTagged ,/* [17] is tagged */
                            CHT2_PCL_KEY_FIELDS_ID_IS_TAGGED_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->eVid ,/* [29:18] eVid */
                            CHT2_PCL_KEY_FIELDS_ID_VID_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->up  ,/* [32:30] UP */
                            CHT2_PCL_KEY_FIELDS_ID_UP_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->qos.qosProfile,/* [39:33] QOS profile */
                            CHT2_PCL_KEY_FIELDS_ID_QOS_PROFILE_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->isIPv4 , /* [40] is IPv4 */
                            CHT2_PCL_KEY_FIELDS_ID_IS_IPV4_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->isIp,/* [41] is IP */
                            CHT2_PCL_KEY_FIELDS_ID_IS_IP_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->ipProt,/* [49:42] IP Protocol*/
                            CHT2_PCL_KEY_FIELDS_ID_IP_PROTOCOL_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dscp,/* [55:50] dscp */
                            CHT2_PCL_KEY_FIELDS_ID_DSCP_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL4Valid, /* [56] isL4Valid*/
                            CHT2_PCL_KEY_FIELDS_ID_IS_L4_VALID_E);
    if (descrPtr->l4StartOffsetPtr != NULL)
    {
        if (descrPtr->ipv6Icmp || descrPtr->ipv4Icmp)
        { /* [72:57] L4 Byte Offsets 5:4 */
            snetCht2IPclKeyFieldBuildByPointer(
                pclKeyPtr,(GT_U8*)&(descrPtr->l4StartOffsetPtr[4]),
                    CHT2_PCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_4_5_E);
        }
        else
        {/* [72:57] L4 Byte Offsets 3:2 */
            snetCht2IPclKeyFieldBuildByPointer(
                pclKeyPtr,(GT_U8*)&(descrPtr->l4StartOffsetPtr[2]),
                    CHT2_PCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_2_3_E);
        }
    }

    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->arp ,/* [73] isArp */
                            CHT2_PCL_KEY_FIELDS_ID_IS_ARP_E);
    tmpVal = (descrPtr->macDaType == SKERNEL_BROADCAST_MAC_E ||
              descrPtr->macDaType == SKERNEL_BROADCAST_ARP_E) ? 1 : 0 ;
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, tmpVal ,/* [74] is bc */
                            CHT2_PCL_KEY_FIELDS_ID_IS_BC_E);

    smemRegGet(devObjPtr,SMEM_CHT2_KEY5_USERDEFINED_BYTES_SHORT_L3_L4_KEY_REG(devObjPtr),&userDefdata5);
    if (userDefdata5 & (1 << 0x9))
    {/* TCP/UDP Range Comparators is enable */
        __LOG(("TCP/UDP Range Comparators is enable"));
        rc = snetChtIPclTcpUdpPortRangeCompareGet(devObjPtr,descrPtr,&byteValue);
        byteValue = (rc != GT_OK) ? 0 : byteValue ;
        snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [82:75] TCP/UDP comperator */
                            CHT2_PCL_KEY_FIELDS_ID_TCP_UDP_PORT_COMPS_SHORT_E);
    }
    else
    {   /* Key5 User Define Byte0  */
        __LOG(("Key5 User Define Byte0"));
        userDefinedArrayPtr = cht2PclUserDefinedConfigInfoShortL3L4KeyArray;
        userDefinedAnchor = SMEM_U32_GET_FIELD(userDefdata5,userDefinedArrayPtr[0].startBitAnchor,2);
        userDefinedOffset = SMEM_U32_GET_FIELD(userDefdata5,userDefinedArrayPtr[0].startBitOffset,7);

        rc = snetChtPclUserDefinedByteGet(devObjPtr,descrPtr,userDefinedAnchor,
                                    userDefinedOffset,&byteValue);
        if(rc != GT_OK)
        {
            userDefinedError = GT_TRUE;
            byteValue = 0;
        }

        snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [82:75] UserDefinedByte0 */
                            CHT2_PCL_KEY_FIELDS_ID_USER_DEFINED_BYTE0_82_75_SHORT_E);
    }

    /* Key5 User Define Byte1  */
    __LOG(("Key5 User Define Byte1"));
    userDefinedArrayPtr = cht2PclUserDefinedConfigInfoShortL3L4KeyArray;
    userDefinedAnchor = SMEM_U32_GET_FIELD(userDefdata5,userDefinedArrayPtr[0].startBitAnchor,2);
    userDefinedOffset = SMEM_U32_GET_FIELD(userDefdata5,userDefinedArrayPtr[0].startBitOffset,7);

    rc = snetChtPclUserDefinedByteGet(devObjPtr,descrPtr,userDefinedAnchor,
                                userDefinedOffset,&byteValue);
    if(rc != GT_OK)
    {
        userDefinedError = GT_TRUE;
        byteValue = 0;
    }

    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [83:90] UserDefinedByte1 */
                        CHT2_PCL_KEY_FIELDS_ID_USER_DEFINED_BYTE1_90_83_SHORT_E);

    /* Key5 User Define Byte2  */
    __LOG(("Key5 User Define Byte2"));
    userDefinedArrayPtr = cht2PclUserDefinedConfigInfoShortL3L4KeyArray;
    userDefinedAnchor = SMEM_U32_GET_FIELD(userDefdata5,userDefinedArrayPtr[2].startBitAnchor,2);
    userDefinedOffset = SMEM_U32_GET_FIELD(userDefdata5,userDefinedArrayPtr[2].startBitOffset,7);

    rc = snetChtPclUserDefinedByteGet(devObjPtr,descrPtr,userDefinedAnchor,
                                userDefinedOffset,&byteValue);
    if(rc != GT_OK)
    {
        userDefinedError = GT_TRUE;
        byteValue = 0;
    }

    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [98:91] UserDefinedByte2 */
                        CHT2_PCL_KEY_FIELDS_ID_USER_DEFINED_BYTE2_98_91_SHORT_E);
    /* for IPv4 packets only the first index sip[0] or dip[0] is relevant */
    __LOG(("for IPv4 packets only the first index sip[0] or dip[0] is relevant"));
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[0], /* [130:99] SIP*/
                                        CHT2_PCL_KEY_FIELDS_ID_SIP_31_0_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[0], /* [162:131] DIP*/
                                        CHT2_PCL_KEY_FIELDS_ID_DIP_31_0_E);
    if (descrPtr->l4StartOffsetPtr != NULL)
    {
        tmpFieldVal[0] = descrPtr->l4StartOffsetPtr[0];
        tmpFieldVal[1] = descrPtr->l4StartOffsetPtr[1];
        tmpFieldVal[2] = descrPtr->l4StartOffsetPtr[13];
        snetCht2IPclKeyFieldBuildByPointer(
            pclKeyPtr,(GT_U8*)&tmpFieldVal[0], /* [186:163] L4 byte offsets */
                    CHT2_PCL_KEY_FIELDS_ID_L4_BYTES_OFFSETS_0_1_13_SHORT_E);
    }
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, resrvdVal, /* [187] reserved */
                                        CHT2_PCL_KEY_FIELDS_ID_RESERVED_187_E);
    tmpVal = (userDefinedError == GT_TRUE) ? 0 : 1;
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, tmpVal,/* [188] user defined valid */
                        CHT2_PCL_KEY_FIELDS_ID_USER_DEFINED_VALID_SHORT_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL2Valid ,/* [189] isL2Valid  */
                            CHT2_PCL_KEY_FIELDS_ID_IS_L2_VALID_SHORT_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpv4Fragment ,/* [190] ipv4 fragmented  */
                            CHT2_PCL_KEY_FIELDS_ID_IPV4_FRAGMENTED_SHORT_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpHeaderOk ,/* [191] ip header  */
                            CHT2_PCL_KEY_FIELDS_ID_IP_HEADER_OK_SHORT_E);

    return  ;
}

/*******************************************************************************
*   snetCht2IPclBuildL2L3StandardKey
*
* DESCRIPTION:
*        Build Layer2+IPv4\6+QoS standard PCL search key.
*
* INPUTS:
*       devObjPtr            - pointer to device object.
*       descrPtr             - pointer to frame data buffer Id.
*       currentLookupConfPtr - pointer to the curent PCL configuration entry.
*       pclExtraDataPtr      - pointer to extra data needed for the IPCL engine.
*       cycleNum             - cycle number
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
static GT_VOID snetCht2IPclBuildL2L3StandardKey
(
    IN SKERNEL_DEVICE_OBJECT                        * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC              * descrPtr      ,
    IN SNET_CHT2_POLICY_SPECIFIC_LOOKUP_CONFIG_STC  * currentLookupConfPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC                * pclExtraDataPtr  ,
    OUT SNET_CHT_POLICY_KEY_STC                     * pclKeyPtr
)
{
    DECLARE_FUNC_NAME(snetCht2IPclBuildL2L3StandardKey);

    GT_U32      fieldVal;           /* valid bit value */
    GT_U32      userDefdata2;
    CHT2_PCL_USER_DEFINED_CONFIG_REG_STC *userDefinedArrayPtr;
    GT_U32      userDefinedAnchor;  /* user defined byte Anchor */
    GT_U32      userDefinedOffset;  /* user defined byte offset from Anchor */
    GT_U8       byteValue;          /* value of user defined byte from the
                                        packets or tcp/udp compare */
    GT_U32      userDefinedError =GT_FALSE;/* is there user defined byte error */
    GT_STATUS   rc;                 /* status from called function */
    GT_U32      tmpVal;             /* temp val */


    fieldVal = 1;
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, fieldVal, /* bits[0] Valid */
                            CHT2_PCL_KEY_FIELDS_ID_VALID_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, currentLookupConfPtr->pclID ,
                            CHT2_PCL_KEY_FIELDS_ID_PCL_ID_E);/* [10:1] PCL-ID */
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->localDevSrcPort ,
                            CHT2_PCL_KEY_FIELDS_ID_SRC_PORT_E);/* [16:11] src port */
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->origSrcTagged ,/* [17] is tagged */
                            CHT2_PCL_KEY_FIELDS_ID_IS_TAGGED_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->eVid ,/* [29:18] eVid */
                            CHT2_PCL_KEY_FIELDS_ID_VID_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->up  ,/* [32:30] UP */
                            CHT2_PCL_KEY_FIELDS_ID_UP_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->qos.qosProfile,/* [39:33] QOS profile */
                            CHT2_PCL_KEY_FIELDS_ID_QOS_PROFILE_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->isIPv4 , /* [40] is IPv4 */
                            CHT2_PCL_KEY_FIELDS_ID_IS_IPV4_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->isIp,/* [41] is IP */
                            CHT2_PCL_KEY_FIELDS_ID_IS_IP_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->ipProt,/* [49:42] IP Protocol*/
                            CHT2_PCL_KEY_FIELDS_ID_IP_PROTOCOL_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dscp,/* [55:50] dscp */
                            CHT2_PCL_KEY_FIELDS_ID_DSCP_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL4Valid, /* [56] isL4Valid*/
                            CHT2_PCL_KEY_FIELDS_ID_IS_L4_VALID_E);
    if (descrPtr->l4StartOffsetPtr != NULL)
    {
        if (descrPtr->ipv6Icmp || descrPtr->ipv4Icmp)
        {
            snetCht2IPclKeyFieldBuildByPointer(
                pclKeyPtr,(GT_U8*)&(descrPtr->l4StartOffsetPtr[4]),
                CHT2_PCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_4_5_E);/* [72:57] L4 Byte Offsets 5:4 */
        }
        else
        {
            snetCht2IPclKeyFieldBuildByPointer(
                pclKeyPtr,(GT_U8*)&(descrPtr->l4StartOffsetPtr[2]),
                CHT2_PCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_2_3_E);/* [72:57] L4 Byte Offsets 3:2 */
        }
    }
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->arp ,/* [73] isArp */
                                        CHT2_PCL_KEY_FIELDS_ID_IS_ARP_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpV6EhExists,/* [74] ipv6_EH_exist */
                            CHT2_PCL_KEY_FIELDS_ID_IS_IPV6_EH_EXISTS_SHORT_E);
    smemRegGet(devObjPtr,SMEM_CHT2_KEY0_USERDEFINED_BYTES_SHORT_L2_L3_KEY_REG(devObjPtr),&userDefdata2);
    if (userDefdata2 & (1 << 0x14))
    {/* TCP/UDP Range Comparators is enable */
        __LOG(("TCP/UDP Range Comparators is enable"));
        rc = snetChtIPclTcpUdpPortRangeCompareGet(devObjPtr,descrPtr,&byteValue);
        byteValue = (rc != GT_OK) ? 0 : byteValue ;
        snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [82:75] TCP/UDP comperator */
                            CHT2_PCL_KEY_FIELDS_ID_TCP_UDP_PORT_COMPS_SHORT_E);
    }
    else
    {   /* Key3 and Key 4User Define Byte0  */
        __LOG(("Key3 and Key 4User Define Byte0"));
        userDefinedArrayPtr = cht2PclUserDefinedConfigInfoShortL2L3KeyArray;
        userDefinedAnchor = SMEM_U32_GET_FIELD(userDefdata2,userDefinedArrayPtr[0].startBitAnchor,2);
        userDefinedOffset = SMEM_U32_GET_FIELD(userDefdata2,userDefinedArrayPtr[0].startBitOffset,7);

        rc = snetChtPclUserDefinedByteGet(devObjPtr,descrPtr,userDefinedAnchor,
                                                userDefinedOffset,&byteValue);
        if(rc != GT_OK)
        {
            userDefinedError = GT_TRUE;
            byteValue = 0;
        }

        snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [82:75] UserDefinedByte0 */
                            CHT2_PCL_KEY_FIELDS_ID_USER_DEFINED_BYTE0_82_75_SHORT_E);
    }
    /* Key3 and Key 4 User Define Byte1  */
    __LOG(("Key3 and Key 4 User Define Byte1"));
    userDefinedArrayPtr = cht2PclUserDefinedConfigInfoShortL2L3KeyArray;
    userDefinedAnchor = SMEM_U32_GET_FIELD(userDefdata2,userDefinedArrayPtr[1].startBitAnchor,2);
    userDefinedOffset = SMEM_U32_GET_FIELD(userDefdata2,userDefinedArrayPtr[1].startBitOffset,7);

    rc = snetChtPclUserDefinedByteGet(devObjPtr,descrPtr,userDefinedAnchor,
                                        userDefinedOffset,&byteValue);
    if(rc != GT_OK)
    {
        userDefinedError = GT_TRUE;
        byteValue = 0;
    }

    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [90:83] UserDefinedByte1 */
                            CHT2_PCL_KEY_FIELDS_ID_USER_DEFINED_BYTE1_90_83_SHORT_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpV6EhHopByHop,/* [91] is IPv6 hopbyhop */
                            CHT2_PCL_KEY_FIELDS_ID_LONG_IS_IPV6_EH_HOP_BY_HOP_E);
    snetCht2IPclKeyFieldBuildByPointer(pclKeyPtr, descrPtr->macSaPtr,/* [139:92] MAC SA*/
                            CHT2_PCL_KEY_FIELDS_ID_MAC_SA_SHORT_E);
    snetCht2IPclKeyFieldBuildByPointer(pclKeyPtr, descrPtr->macDaPtr,/* [187:140] MAC DA*/
                            CHT2_PCL_KEY_FIELDS_ID_MAC_DA_SHORT_E);
    tmpVal = (userDefinedError == GT_TRUE) ? 0 : 1;
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, tmpVal,              /* [188] user defined valid */
                        CHT2_PCL_KEY_FIELDS_ID_USER_DEFINED_VALID_SHORT_E);

    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL2Valid ,    /* [189] isL2Valid  */
                            CHT2_PCL_KEY_FIELDS_ID_IS_L2_VALID_SHORT_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpv4Fragment ,/* [190] ipv4 fragmented  */
                            CHT2_PCL_KEY_FIELDS_ID_IPV4_FRAGMENTED_SHORT_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpHeaderOk , /* [191] ip header ok */
                            CHT2_PCL_KEY_FIELDS_ID_IP_HEADER_OK_SHORT_E);

    return ;
}

/*******************************************************************************
*   snetCht2IPclBuildL2StandardKey
*
* DESCRIPTION:
*        Build layer2 standard pcl search key.
*
* INPUTS:
*       devObjPtr            - pointer to device object.
*       descrPtr             - pointer to frame data buffer Id.
*       currentLookupConfPtr - pointer to the curent PCL configuration entry.
*       pclExtraDataPtr      - pointer to extra data needed for the IPCL engine.
*
* OUTPUTS:
*       pclKeyPtr    - pointer to PCK key structure.
*
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID snetCht2IPclBuildL2StandardKey
(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr      ,
    IN SNET_CHT2_POLICY_SPECIFIC_LOOKUP_CONFIG_STC * currentLookupConfPtr   ,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC    * pclExtraDataPtr  ,
    OUT SNET_CHT_POLICY_KEY_STC         * pclKeyPtr
)
{
    DECLARE_FUNC_NAME(snetCht2IPclBuildL2StandardKey);

    GT_U32  fieldVal;           /* valid bit value */
    GT_U32  resrvdVal;          /* reserved bit value */
    GT_U32  tmpVal;             /* temp val */
    GT_U32  userDefdata;
    GT_U32  userDefinedAnchor;  /* user defined byte Anchor */
    GT_U32  userDefinedOffset;  /* user defined byte offset from Anchor */
    GT_STATUS   rc;             /* status from called function */
    GT_U8  byteValue;           /* value of user defined byte from the packets or tcp/udp compare */
    GT_BOOL userDefinedError = GT_FALSE;/* is there user defined byte error */
    CHT2_PCL_USER_DEFINED_CONFIG_REG_STC *userDefinedArrayPtr;

    fieldVal = 1;
    resrvdVal = 0;
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, fieldVal, /* bits[0] Valid */
                            CHT2_PCL_KEY_FIELDS_ID_VALID_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, currentLookupConfPtr->pclID ,
                            CHT2_PCL_KEY_FIELDS_ID_PCL_ID_E);/*[10:1] PCL-ID */
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->localDevSrcPort ,
                            CHT2_PCL_KEY_FIELDS_ID_SRC_PORT_E);/* [16:11] src port */
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->origSrcTagged ,/* [17] is tagged */
                            CHT2_PCL_KEY_FIELDS_ID_IS_TAGGED_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->eVid ,/* [29:18] eVid */
                            CHT2_PCL_KEY_FIELDS_ID_VID_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->up  ,/* [32:30] UP */
                            CHT2_PCL_KEY_FIELDS_ID_UP_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->qos.qosProfile,/* [39:33] QOS profile */
                            CHT2_PCL_KEY_FIELDS_ID_QOS_PROFILE_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->isIPv4 , /* [40] is IPv4 */
                            CHT2_PCL_KEY_FIELDS_ID_IS_IPV4_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->isIp,/* [41] is IP */
                            CHT2_PCL_KEY_FIELDS_ID_IS_IP_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, resrvdVal,/*[42] reserved field*/
                            CHT2_PCL_KEY_FIELDS_ID_RESERVED_42_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->etherTypeOrSsapDsap,/* [58:43] */
                            CHT2_PCL_KEY_FIELDS_ID_ETHER_TYPE_DSAP_SSAP_E);

    /* Get UserDefined byte 2 value */
    __LOG(("Get UserDefined byte 2 value"));
    smemRegGet(devObjPtr,SMEM_CHT2_KEY3_USERDEFINED_BYTES_SHORT_L2_KEY_REG(devObjPtr),&userDefdata);
    userDefinedArrayPtr = cht2PclUserDefinedConfigInfoShortL2KeyArray;
    userDefinedAnchor = SMEM_U32_GET_FIELD(userDefdata,userDefinedArrayPtr[2].startBitAnchor,2);
    userDefinedOffset = SMEM_U32_GET_FIELD(userDefdata,userDefinedArrayPtr[2].startBitOffset,7);

    rc = snetChtPclUserDefinedByteGet(devObjPtr,descrPtr,userDefinedAnchor,
                                userDefinedOffset,&byteValue);
    if(rc != GT_OK)
    {
        userDefinedError = GT_TRUE;
        byteValue = 0;
    }

    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [66:59] UserDefinedByte2 */
                        CHT2_PCL_KEY_FIELDS_ID_USER_DEFINED_BYTE2_66_59_SHORT_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, resrvdVal,/* [72:67] reserved  */
                            CHT2_PCL_KEY_FIELDS_ID_RESERVED_72_67_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->arp ,/* [73] isArp */
                            CHT2_PCL_KEY_FIELDS_ID_IS_ARP_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, resrvdVal ,/* [74] reserved  */
                            CHT2_PCL_KEY_FIELDS_ID_RESERVED_74_E);

    /* Get UserDefined byte 0 value */
    __LOG(("Get UserDefined byte 0 value"));
    userDefinedAnchor = SMEM_U32_GET_FIELD(userDefdata,userDefinedArrayPtr[0].startBitAnchor,2);
    userDefinedOffset = SMEM_U32_GET_FIELD(userDefdata,userDefinedArrayPtr[0].startBitOffset,7);
    rc = snetChtPclUserDefinedByteGet(devObjPtr,descrPtr,userDefinedAnchor,
                                userDefinedOffset,&byteValue);
    if(rc != GT_OK)
    {
        userDefinedError = GT_TRUE;
        byteValue = 0;
    }

    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [82:75] UserDefinedByte0 */
                            CHT2_PCL_KEY_FIELDS_ID_USER_DEFINED_BYTE0_82_75_SHORT_E);

    /* Get UserDefined byte 1 value */
    __LOG(("Get UserDefined byte 1 value"));
    userDefinedAnchor = SMEM_U32_GET_FIELD(userDefdata,userDefinedArrayPtr[1].startBitAnchor,2);
    userDefinedOffset = SMEM_U32_GET_FIELD(userDefdata,userDefinedArrayPtr[1].startBitOffset,7);
    rc = snetChtPclUserDefinedByteGet(devObjPtr,descrPtr,userDefinedAnchor,
                                userDefinedOffset,&byteValue);
    if(rc != GT_OK)
    {
        userDefinedError = GT_TRUE;
        byteValue = 0;
    }
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [90:83] UserDefinedByte1 */
                            CHT2_PCL_KEY_FIELDS_ID_USER_DEFINED_BYTE1_90_83_SHORT_E);

    tmpVal = (descrPtr->l2Encaps == SKERNEL_ETHERNET_II_E) ? 1 : 0;
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, tmpVal,/* [91] Encap Type */
                        CHT2_PCL_KEY_FIELDS_ID_ENCAP_TYPE_SHORT_E);

    snetCht2IPclKeyFieldBuildByPointer(pclKeyPtr, descrPtr->macSaPtr,/* [139:92] MAC SA*/
                            CHT2_PCL_KEY_FIELDS_ID_MAC_SA_SHORT_E);
    snetCht2IPclKeyFieldBuildByPointer(pclKeyPtr, descrPtr->macDaPtr,/* [187:140] MAC DA*/
                            CHT2_PCL_KEY_FIELDS_ID_MAC_DA_SHORT_E);
    tmpVal = (userDefinedError == GT_TRUE) ? 0 : 1;
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, tmpVal,
                        CHT2_PCL_KEY_FIELDS_ID_USER_DEFINED_VALID_SHORT_E);

    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL2Valid ,/* [189] isL2Valid  */
                                    CHT2_PCL_KEY_FIELDS_ID_IS_L2_VALID_SHORT_E);
    snetCht2IPclKeyFieldBuildByValue(pclKeyPtr, resrvdVal ,/* [191:190] reserved  */
                                    CHT2_PCL_KEY_FIELDS_ID_RESERVED_190_191_E);

    return ;
}

/*******************************************************************************
* snetCht2IPclConfigPtrGet
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
*       C.12.15.1 Ingress PCL Configuration Table
*                 The PCL configuration table is a 4224 line table each entry is 32-bits.
*
*******************************************************************************/
static GT_VOID snetCht2IPclConfigPtrGet
(
    IN SKERNEL_DEVICE_OBJECT                * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC      * descrPtr ,
    OUT SNET_CHT2_IPOLICY_LOOKUP_CONFIG_STC * lookupConfPtr
)
{
    DECLARE_FUNC_NAME(snetCht2IPclConfigPtrGet);

    GT_U32  regAddrPclGlobTbl; /* register address for PCL global config */
    GT_U32  fldValue;
    GT_U32  index[2]; /* the pcl config. entry can be different between the 2 cycles */
    GT_U32  pclCfgMode; /* cfg mode */
    GT_U32  ingressPclCfgMode;
    GT_U32  regAddr;
    GT_U32  portIfPCLBlock;
    GT_U32  pclCfgEntry;  /* pointer to the PCL configuration mode */
    GT_U32  ii=0; /* 0..1 loop index */
    GT_U32  pclCfgModeBit ; /* pcl cfg mode bit */
    SNET_IPCL_LOOKUP_ENT  pclLookUpCycle;   /* pcl lookup cycle */

    regAddrPclGlobTbl = SMEM_CHT_PCL_GLOBAL_REG(devObjPtr); /* PCL global configuration */

    for (ii = 0; ii < 2 ; ++ii)
    {
        regAddr =  SMEM_CHT_PORT_VLAN_QOS_CONFIG_TBL_MEM(devObjPtr,
                                                          descrPtr->localDevSrcPort);

        pclCfgModeBit = (ii == 0) ? 20 : 16;
        smemRegFldGet(devObjPtr, regAddr, pclCfgModeBit, 1, &pclCfgMode);
        if (pclCfgMode == 1)
        {/* Access PCL-ID config. table with vlan id  */
            __LOG(("Access PCL-ID config. table with vlan id"));
            index[ii] = descrPtr->eVid;
        }
        else
        {
            smemRegFldGet(devObjPtr, regAddrPclGlobTbl, 1, 1, &ingressPclCfgMode);
            if (ingressPclCfgMode == 0)
            {/* PCL configuration table index = Local Device Source Port# + 4K */
                __LOG(("PCL configuration table index = Local Device Source Port# + 4K"));
                index[ii] = (1<<12) | descrPtr->localDevSrcPort;
            }
            else
            {
                if (descrPtr->origIsTrunk == 0)
                {   /* read the max port per device and the PCL interface block */
                    __LOG(("read the max port per device and the PCL interface block"));
                    smemRegFldGet(devObjPtr, regAddrPclGlobTbl, 18, 1, &fldValue);
                    smemRegFldGet(devObjPtr, regAddrPclGlobTbl,16, 2, &portIfPCLBlock);
                    if (fldValue == 0)
                    {   /* Max port per device = 0 , up to 1K ports in system */
                        __LOG(("Max port per device = 0 , up to 1K ports in system"));
                        index[ii] = ((portIfPCLBlock & 0x3)<< 10) |
                                    ((descrPtr->srcDev & 0x1f)<< 5) |
                                    ((descrPtr->origSrcEPortOrTrnk & 0x1f));
                    }
                    else
                    {   /* Max port per device = 1 , up to 2K ports in system */
                        __LOG(("Max port per device = 1 , up to 2K ports in system"));
                        index[ii] = ((portIfPCLBlock & 0x1)<< 11) |
                                    ((descrPtr->srcDev & 0x1f)<< 6) |
                                    ((descrPtr->origSrcEPortOrTrnk & 0x3f));
                    }
                }
                else
                {   /* Access PCL-ID config. table with ingress trunk  */
                    __LOG(("Access PCL-ID config. table with ingress trunk"));
                    index[ii] = (1<<12) | descrPtr->origSrcEPortOrTrnk;
                }
            }
        }
    } /* ii -> 0..1 */

    /* calculate the address of the PCL configuration table for the 1st lookup*/
    __LOG(("calculate the address of the PCL configuration table for the 1st lookup"));
    pclLookUpCycle = SNET_IPCL_LOOKUP_0_0_E;
    regAddr = SMEM_CHT_PCL_CONFIG_TBL_MEM(devObjPtr, index[0], pclLookUpCycle);
    smemRegGet(devObjPtr, regAddr, &pclCfgEntry);
    /* Ingress PCL configuration Table Entry , 1st lookup */
    __LOG(("Ingress PCL configuration Table Entry , 1st lookup"));
    lookupConfPtr->lookup0Config.enabled = SMEM_U32_GET_FIELD(pclCfgEntry,0,1);
    lookupConfPtr->lookup0Config.nonIpKeyType = SMEM_U32_GET_FIELD(pclCfgEntry,1,1);
    lookupConfPtr->lookup0Config.ipv4ArpKeyType = SMEM_U32_GET_FIELD(pclCfgEntry,12,2);
    lookupConfPtr->lookup0Config.ipv6KeyType = SMEM_U32_GET_FIELD(pclCfgEntry,14,2);
    lookupConfPtr->lookup0Config.pclID = SMEM_U32_GET_FIELD(pclCfgEntry,2,10);

    /* calculate the address of the PCL configuration table for the 2nd lookup*/
    __LOG(("calculate the address of the PCL configuration table for the 2nd lookup"));
    pclLookUpCycle = SNET_IPCL_LOOKUP_1_E;
    regAddr = SMEM_CHT_PCL_CONFIG_TBL_MEM(devObjPtr, index[1], pclLookUpCycle);
    smemRegGet(devObjPtr, regAddr, &pclCfgEntry);
    /* Ingress PCL configuration Table Entry , 2nd lookup */
    __LOG(("Ingress PCL configuration Table Entry , 2nd lookup"));
    lookupConfPtr->lookup1Config.enabled = SMEM_U32_GET_FIELD(pclCfgEntry,16,1);
    lookupConfPtr->lookup1Config.nonIpKeyType = SMEM_U32_GET_FIELD(pclCfgEntry,17,1);
    lookupConfPtr->lookup1Config.ipv4ArpKeyType = SMEM_U32_GET_FIELD(pclCfgEntry,28,2);
    lookupConfPtr->lookup1Config.ipv6KeyType = SMEM_U32_GET_FIELD(pclCfgEntry,30,2);
    lookupConfPtr->lookup1Config.pclID = SMEM_U32_GET_FIELD(pclCfgEntry,18,10);

    return ;
}


/*******************************************************************************
* snetCht2IPclGetKeyType
*
* DESCRIPTION:
*       Get the key type from the PCL configuration table.
*
* INPUTS:
*       descrPtr             - pointer to frame descriptor object.
*       currentLookupConfPtr - pointer to the current PCL configuration table.
*
* OUTPUTS:
*       keyTypePtr           - pointer to the relevant key type.
*       extendedLookupModePtr- extended or short lookup mode.
* RETURN:
*
* COMMENTS:
*       C.12.15.1 Ingress PCL Configuration Table
*                 The PCL table is a 4224 line table each entry is 32-bits.
*
*******************************************************************************/
static GT_VOID snetCht2IPclGetKeyType
(
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC           * descrPtr  ,
    IN SNET_CHT2_POLICY_SPECIFIC_LOOKUP_CONFIG_STC  * currentLookupConfPtr,
    OUT CHT2_PCL_KEY_TYPE_ENT                       * keyTypePtr,
    IN SNET_CHT_POLICY_KEY_STC                      * pclKeyPtr
)
{
    if ((descrPtr->isIPv4 == 1) || (descrPtr->arp == 1)) /* ipv4 or arp packet */
    {
        if (currentLookupConfPtr->ipv4ArpKeyType == 0)
        {
            *keyTypePtr = CHT2_PCL_KEY_TYPE_L2_L3_KEY_SHORT_E;
        }
        else if (currentLookupConfPtr->ipv4ArpKeyType == 1)
        {
            *keyTypePtr = CHT2_PCL_KEY_TYPE_L3_L4_KEY_SHORT_E;
        }
        else
        {
            *keyTypePtr = CHT2_PCL_KEY_TYPE_L2_L3_L4_KEY_LONG_E;
        }
    }
    else if (descrPtr->isIp == 0) /* non ip or non arp packet */
    {
        if (currentLookupConfPtr->nonIpKeyType == 0)
        {
            *keyTypePtr = CHT2_PCL_KEY_TYPE_L2_KEY_SHORT_E;
        }
        else
        {
            *keyTypePtr = CHT2_PCL_KEY_TYPE_L2_L3_KEY_SHORT_E;
        }
    }
    else /* ipv6 packet */
    {
        if (currentLookupConfPtr->ipv6KeyType == 0)
        {
            *keyTypePtr = CHT2_PCL_KEY_TYPE_L2_L3_KEY_SHORT_E;
        }
        else if (currentLookupConfPtr->ipv6KeyType == 1)
        {
            *keyTypePtr = CHT2_PCL_KEY_TYPE_L2_IPV6_KEY_SHORT_E;
        }
        else if (currentLookupConfPtr->ipv6KeyType == 2)
        {
            *keyTypePtr = CHT2_PCL_KEY_TYPE_L2_IPV6_KEY_LONG_E;
        }
        else
        {
            *keyTypePtr = CHT2_PCL_KEY_TYPE_L4_IPV6_KEY_LONG_E;
        }
    }

    currentLookupConfPtr->keySize = (*keyTypePtr <= CHT2_PCL_KEY_TYPE_L2_IPV6_KEY_SHORT_E) ?
                                        SNET_CHT_PCL_STANDARD_KEY :
                                        SNET_CHT_PCL_EXTENDED_KEY ;

    pclKeyPtr->pclKeyFormat  =
        (currentLookupConfPtr->keySize == SNET_CHT_PCL_EXTENDED_KEY) ?
        CHT_PCL_KEY_EXTENDED_E : CHT_PCL_KEY_REGULAR_E;

    return ;
}

/*******************************************************************************
* snetCht2IPclActionApply
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
*       C.12.13     Policy Tcam Table : The policy TCAM holds 1024 lines
*                 of 24 bytes or rule data used for ingress and egress PCL.
*******************************************************************************/
GT_VOID snetCht2IPclActionApply
(
    IN SKERNEL_DEVICE_OBJECT                * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC   * descrPtr  ,
    IN SNET_CHT2_PCL_ACTION_STC             * actionDataPtr,
    IN GT_BOOL                              isFirst
)
{
    DECLARE_FUNC_NAME(snetCht2IPclActionApply);

    GT_U32  address;  /* counter address */
    GT_U32  regValue; /* counter value */
    GT_BOOL updateVid = GT_FALSE; /* update eVid flag */

    if (actionDataPtr->pceRedirectCmd == PCL_TTI_ACTION_REDIRECT_CMD_PBR_OR_VPLS_E)
    {
        /* Route action will be applied later in UC route engine */
        __LOG(("Route action will be applied later in UC route engine"));
        return;
    }

    /* update the matching counter if enabled */
    __LOG(("update the matching counter if enabled"));
    if (actionDataPtr->matchCounterEn && devObjPtr->cncBlocksNum == 0)
    {
        address = SMEM_CHT2_PCL_RULE_MATCH_REG(devObjPtr) + (actionDataPtr->matchCounterIndex * 0x4);
        smemRegGet(devObjPtr, address, &regValue);
        ++regValue;
        smemRegSet(devObjPtr, address, regValue);
    }

    /* Remarking the QOS profile . relevant only is Qos precedence of the
       previous Qos Assignment mechanism is soft */
    if (descrPtr->qosProfilePrecedence == SKERNEL_QOS_PROF_PRECED_SOFT)
    {
        /* Enables the modification of the packet 802.1q User Priority field */
        __LOG(("Enables the modification of the packet 802.1q User Priority field"));
        descrPtr->modifyUp = (actionDataPtr->pceModifyUp == 1) ? 1 :
                             (actionDataPtr->pceModifyUp == 2) ? 0 : descrPtr->modifyUp;

        /* Enables the modification of the packet 802.1q dscp field */
        __LOG(("Enables the modification of the packet 802.1q dscp field"));
        descrPtr->modifyDscp = (actionDataPtr->pceModifyDscp == 1) ? 1 :
                               (actionDataPtr->pceModifyDscp == 2) ? 0 : descrPtr->modifyDscp;

        /* Enables the modification of the qos profile index */
        __LOG(("Enables the modification of the qos profile index"));
        if(actionDataPtr->pceQosProfileMarkingEn == GT_TRUE)
        {
            descrPtr->qos.qosProfile = actionDataPtr->pceQosProfile;
        }
        descrPtr->qosProfilePrecedence =  actionDataPtr->pceQosPrecedence ;
    }

    /* enables the mirroring of the packet to the ingress analyzer port */
    __LOG(("enables the mirroring of the packet to the ingress analyzer port"));
    if(actionDataPtr->mirrorToIngressAnalyzerPort)
    {
        descrPtr->rxSniff = 1;
    }

    /* Select mirror analyzer index */
    __LOG(("Select mirror analyzer index"));
    snetXcatIngressMirrorAnalyzerIndexSelect(devObjPtr, descrPtr,
        actionDataPtr->mirrorToIngressAnalyzerPort);


    /* resolve packet command and CPU code */
    __LOG(("resolve packet command and CPU code"));
    snetChtIngressCommandAndCpuCodeResolution(devObjPtr,descrPtr,
                                              descrPtr->packetCmd,
                                              actionDataPtr->fwdCmd,
                                              descrPtr->cpuCode,
                                              actionDataPtr->pceCpuCode,
                                              SNET_CHEETAH_ENGINE_UNIT_PCL_E,
                                              isFirst);


    /*  VID re-assignment is relevant only is the VID precedence , set by the
        previous VID assignment mechanism is soft */
    if  (descrPtr->preserveVid == 0)
    {
        descrPtr->preserveVid = actionDataPtr->pceVidPrecedence;
        /* PCE_VID is assigned for all packets */
        __LOG(("PCE_VID is assigned for all packets"));
        if (actionDataPtr->pceVlanCmd == 3)
        {
            updateVid = GT_TRUE;
        }
        else if ((actionDataPtr->pceVlanCmd == 2) &&
                 (descrPtr->origSrcTagged == 1))
        {
            updateVid = GT_TRUE;
        }
        else if ((actionDataPtr->pceVlanCmd == 1) &&
                 (descrPtr->origSrcTagged == 0))
        {
            updateVid = GT_TRUE;
        }
        if(updateVid == GT_TRUE)
        {
            descrPtr->eVid = actionDataPtr->pceVid;
            descrPtr->vidModified = 1;
        }
    }

    /* Nested Vid  */
    if(descrPtr->nestedVlanAccessPort == 0)
    {
        /*When <EnNestedVLAN> is set, this rule matching flow is defined as a Nested VLAN Access Flow.*/
        __LOG(("When <EnNestedVLAN> is set, this rule matching flow is defined as a Nested VLAN Access Flow."));
        descrPtr->nestedVlanAccessPort = actionDataPtr->enNestedVlan;
        if(actionDataPtr->enNestedVlan)
        {
            /* When a packet received on an access flow is transmitted via a
               tagged port or a cascading port, a VLAN tag is added to the packet
               (on top of the existing VLAN tag, if any). The VID field is the
               VID assigned to the packet as a result of all VLAN assignment
               algorithms. */
            descrPtr->tagSrcTagged[SNET_CHT_TAG_0_INDEX_CNS] = 0;
        }
    }


    /* when set to 1 , this rule is bound to the policer indexed    *
     *   by actionDataPtr->policerIndex                              */
    descrPtr->policerEn = actionDataPtr->policerEn ;

    if(descrPtr->policerEn == 1)
    {
        descrPtr->policerPtr = actionDataPtr->policerIndex;
    }

    /* copy the redirection command */
    __LOG(("copy the redirection command"));
    if (actionDataPtr->pceRedirectCmd == PCL_TTI_ACTION_REDIRECT_CMD_OUT_IF_E)
    {
        if(0 == SKERNEL_IS_XCAT_REVISON_A1_DEV(devObjPtr))
        {
            /* the legacy device did not hold explicit field in the action.
               but the xCat and above have it , so this is the application/cpss driver
               to configure it */
            descrPtr->bypassBridge = 1;
        }


        descrPtr->useVidx =
            SNET_GET_DEST_INTERFACE_TYPE(actionDataPtr->pceEgressIf, SNET_DST_INTERFACE_VIDX_E);
        if(descrPtr->useVidx)
        {
            descrPtr->eVidx = actionDataPtr->pceEgressIf.interfaceInfo.vidx;
        }
        else
        {
            descrPtr->targetIsTrunk =
                SNET_GET_DEST_INTERFACE_TYPE(actionDataPtr->pceEgressIf, SNET_DST_INTERFACE_TRUNK_E);

            if(descrPtr->targetIsTrunk)
            {
                descrPtr->trgTrunkId = actionDataPtr->pceEgressIf.interfaceInfo.trunkId;
            }
            else
            {
                descrPtr->trgDev = actionDataPtr->pceEgressIf.interfaceInfo.devPort.devNum;
                descrPtr->trgEPort = actionDataPtr->pceEgressIf.interfaceInfo.devPort.port;
                /* call after setting trgEPort */
                __LOG(("call after setting trgEPort"));
                SNET_E_ARCH_CLEAR_IS_TRG_PHY_PORT_VALID_MAC(devObjPtr,descrPtr,pcl[SMAIN_DIRECTION_INGRESS_E]);
            }
        }
        if (actionDataPtr->tunnelStart == 1)
        {/* indicate this action is a T.S */
            __LOG(("indicate this action is a T.S"));
            descrPtr->tunnelStart = GT_TRUE;
            descrPtr->tunnelPtr = actionDataPtr->tunnelPtr;
            descrPtr->tunnelStartPassengerType  = actionDataPtr->tunnelType;
        }
        descrPtr->VntL2Echo = actionDataPtr->VNTL2Echo;
    }

    return ;
}

/*******************************************************************************
*   snetCht2IPclNextHopActionApply
*
* DESCRIPTION:
*       Apply Routing PCL relevant action on the frame’s descriptor
*
* INPUTS:
*       devObjPtr    - (pointer to) device object.
*       descrPtr     - (pointer to) frame data buffer Id
*       matchIndex   - matching index
*
* OUTPUTS:
*
* RETURN:
*
* COMMENTS:
*       13.3.3 Route PCL Rule Actions - page 335.
*       The Policy engine only applies the rule action configuration for the
*       <Match Counter>. The remaining Routing PCL action values are stored
*       internally but are not applied to the packet’s forwarding information.
*
*******************************************************************************/
static GT_VOID snetCht2IPclNextHopActionApply
(
    IN SKERNEL_DEVICE_OBJECT            *   devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  *   descrPtr  ,
    IN GT_U32                               matchIndex
)
{
    DECLARE_FUNC_NAME(snetCht2IPclNextHopActionApply);

    GT_U32 *    nextHopEntryMemPtr;      /* (pointer to) action entry in memory */
    GT_BOOL     matchCounterEn;          /* match counter enabled/disabled */
    GT_U32      matchCounterIndex;       /* match counter index */
    GT_U32      counterVal;              /* pcl counter value */

    /* Store nexthop info index internally into descriptor  */
    __LOG(("Store nexthop info index internally into descriptor"));
    descrPtr->pclUcNextHopIndex = matchIndex;

    nextHopEntryMemPtr = smemMemGet(devObjPtr,
                                SMEM_CHT_PCL_ACTION_TBL_MEM(devObjPtr, matchIndex));

    /* The match counter index to which this rule is bound */
    __LOG(("The match counter index to which this rule is bound"));
    matchCounterIndex = SMEM_U32_GET_FIELD(nextHopEntryMemPtr[0], 26, 5);

    /* If set, the PCL rule is bound to the specified match counter index */
    __LOG(("If set, the PCL rule is bound to the specified match counter index"));
    matchCounterEn = SMEM_U32_GET_FIELD(nextHopEntryMemPtr[0], 25, 1);

    /* PCL counters */
    __LOG(("PCL counters"));
    if(matchCounterEn == GT_TRUE)
    {
    /* The match counter is incremented regardless of whether or not the packet
       is routed by the router engine */
        smemRegGet(devObjPtr,
                 0x0B8C0000 + (4 * matchCounterIndex), &counterVal);

        counterVal++;

        smemRegSet(devObjPtr,
                 0x0B8C0000 + (4 * matchCounterIndex), counterVal);
    }
}

/*******************************************************************************
* snetCht2IPclActionGet
*
* DESCRIPTION:
*        Get the action entry from the policy action table.
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
*       C.12.13     Policy Tcam Table : The policy TCAM holds 1024 lines
*                 of 24 bytes or rule data used for ingress and egress PCL.
*******************************************************************************/
extern GT_VOID snetCht2IPclActionGet
(
    IN SKERNEL_DEVICE_OBJECT            *   devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  *   descrPtr,
    IN GT_U32                               matchIndex,
    OUT SNET_CHT2_PCL_ACTION_STC        *   actionDataPtr
)
{
    DECLARE_FUNC_NAME(snetCht2IPclActionGet);

    GT_U32  tblAddr;
    GT_U32  * actionEntryDataPtr;
    GT_BIT enable;

    tblAddr = SMEM_CHT_PCL_ACTION_TBL_MEM(devObjPtr, matchIndex) ;
    actionEntryDataPtr = smemMemGet(devObjPtr,tblAddr);

    actionDataPtr->pceRedirectCmd = SMEM_U32_GET_FIELD(actionEntryDataPtr[1],0,2);

    descrPtr->pclRedirectCmd = actionDataPtr->pceRedirectCmd;

    if (actionDataPtr->pceRedirectCmd == PCL_TTI_ACTION_REDIRECT_CMD_PBR_OR_VPLS_E)
    {
        if(SKERNEL_IS_CHEETAH2_ONLY_DEV(devObjPtr))
        {
            /* Action type is routed entry */
            __LOG(("Action type is routed entry"));
            snetCht2IPclNextHopActionApply(devObjPtr,descrPtr,matchIndex);
        }
        return;
    }

    /* Read word 0 from the action table entry */
    __LOG(("Read word 0 from the action table entry"));
    actionDataPtr->matchCounterIndex = SMEM_U32_GET_FIELD(actionEntryDataPtr[0],26,5);
    actionDataPtr->matchCounterEn = SMEM_U32_GET_FIELD(actionEntryDataPtr[0],25,1);
    actionDataPtr->pceModifyUp = SMEM_U32_GET_FIELD(actionEntryDataPtr[0],23,2);
    actionDataPtr->pceModifyDscp = SMEM_U32_GET_FIELD(actionEntryDataPtr[0],21,2);
    actionDataPtr->pceQosProfile  = SMEM_U32_GET_FIELD(actionEntryDataPtr[0],14,7);
    actionDataPtr->pceQosProfileMarkingEn  = SMEM_U32_GET_FIELD(actionEntryDataPtr[0],13,1);
    actionDataPtr->pceQosPrecedence = SMEM_U32_GET_FIELD(actionEntryDataPtr[0],12,1);
    actionDataPtr->mirrorToIngressAnalyzerPort = SMEM_U32_GET_FIELD(actionEntryDataPtr[0],11,1);
    actionDataPtr->pceCpuCode = SMEM_U32_GET_FIELD(actionEntryDataPtr[0],3,8);
    actionDataPtr->fwdCmd = SMEM_U32_GET_FIELD(actionEntryDataPtr[0],0,3);

    /* Read word 1 from the action table entry */
    __LOG(("Read word 1 from the action table entry"));
    actionDataPtr->pceVid = SMEM_U32_GET_FIELD(actionEntryDataPtr[1],19,12);
    actionDataPtr->pceVlanCmd = SMEM_U32_GET_FIELD(actionEntryDataPtr[1],17,2);
    actionDataPtr->enNestedVlan = SMEM_U32_GET_FIELD(actionEntryDataPtr[1],16,1);
    actionDataPtr->pceVidPrecedence = SMEM_U32_GET_FIELD(actionEntryDataPtr[1],15,1);
    enable = SMEM_U32_GET_FIELD(actionEntryDataPtr[1],14,1);
    SNET_SET_DEST_INTERFACE_TYPE(actionDataPtr->pceEgressIf,
                                 SNET_DST_INTERFACE_VIDX_E, enable);
    if (SNET_GET_DEST_INTERFACE_TYPE(actionDataPtr->pceEgressIf,
                                     SNET_DST_INTERFACE_VIDX_E))
    {
        actionDataPtr->pceEgressIf.interfaceInfo.vidx =
            (GT_U16)SMEM_U32_GET_FIELD(actionEntryDataPtr[1],2,12);
    }
    else
    {
        enable = (GT_U8)SMEM_U32_GET_FIELD(actionEntryDataPtr[1],2,1);
        SNET_SET_DEST_INTERFACE_TYPE(actionDataPtr->pceEgressIf,
                                     SNET_DST_INTERFACE_TRUNK_E, enable);
        if (SNET_GET_DEST_INTERFACE_TYPE(actionDataPtr->pceEgressIf,
                                         SNET_DST_INTERFACE_TRUNK_E))
        {
            actionDataPtr->pceEgressIf.interfaceInfo.trunkId =
                    (GT_U8)SMEM_U32_GET_FIELD(actionEntryDataPtr[1],3,7);
        }
        else
        {
            GT_U32  tmpVal=1;
            SNET_SET_DEST_INTERFACE_TYPE(actionDataPtr->pceEgressIf,
                                         SNET_DST_INTERFACE_PORT_E, tmpVal);

            actionDataPtr->pceEgressIf.interfaceInfo.devPort.port =
                    (GT_U8)SMEM_U32_GET_FIELD(actionEntryDataPtr[1],3,6);
            actionDataPtr->pceEgressIf.interfaceInfo.devPort.devNum =
                    (GT_U8)SMEM_U32_GET_FIELD(actionEntryDataPtr[1],9,5);
        }
    }

    /* Read word 2 from the action table entry */
    __LOG(("Read word 2 from the action table entry"));
    actionDataPtr->tunnelType  = SMEM_U32_GET_FIELD(actionEntryDataPtr[2],21,1);
    actionDataPtr->tunnelPtr   = SMEM_U32_GET_FIELD(actionEntryDataPtr[2],11,10);
    actionDataPtr->tunnelStart = SMEM_U32_GET_FIELD(actionEntryDataPtr[2],10,1);
    actionDataPtr->VNTL2Echo   = SMEM_U32_GET_FIELD(actionEntryDataPtr[2],9,1);
    actionDataPtr->policerIndex= SMEM_U32_GET_FIELD(actionEntryDataPtr[2],1,9);
    actionDataPtr->policerEn   = SMEM_U32_GET_FIELD(actionEntryDataPtr[2],0,1);

    return ;
}

/*******************************************************************************
* snetCht2IPclCreateKey
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
* OUTPUS:
*       pclKeyPtr    - pointer to PCL key structure.
*
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID snetCht2IPclCreateKey
(
    IN SKERNEL_DEVICE_OBJECT                * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC   * descrPtr  ,
    IN SNET_CHT2_POLICY_SPECIFIC_LOOKUP_CONFIG_STC * currentLookupConfPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC        * pclExtraDataPtr    ,
    IN CHT2_PCL_KEY_TYPE_ENT                  keyType,
    OUT SNET_CHT_POLICY_KEY_STC             * pclKeyPtr
)
{

    memset(&pclKeyPtr->key,0,sizeof(pclKeyPtr->key));
    pclKeyPtr->devObjPtr = devObjPtr;
    pclKeyPtr->updateOnlyDiff = GT_FALSE;

    switch (keyType)
    {
        case CHT2_PCL_KEY_TYPE_L2_KEY_SHORT_E:
            snetCht2IPclBuildL2StandardKey(devObjPtr , descrPtr ,
                                           currentLookupConfPtr,pclExtraDataPtr,
                                           pclKeyPtr) ;
            break;
        case CHT2_PCL_KEY_TYPE_L2_L3_KEY_SHORT_E:
            snetCht2IPclBuildL2L3StandardKey(devObjPtr , descrPtr ,
                                             currentLookupConfPtr ,
                                             pclExtraDataPtr,
                                             pclKeyPtr);
            break;
        case CHT2_PCL_KEY_TYPE_L3_L4_KEY_SHORT_E:
            snetCht2IPclBuildL3L4StandardKey(devObjPtr , descrPtr ,
                                             currentLookupConfPtr ,
                                             pclExtraDataPtr,
                                             pclKeyPtr);
            break;
        case CHT2_PCL_KEY_TYPE_L2_IPV6_KEY_SHORT_E:
            snetCht2IPclKeyBuildL2IPv6StandardKey(devObjPtr , descrPtr ,
                                                  currentLookupConfPtr ,
                                                  pclExtraDataPtr,
                                                  pclKeyPtr);
            break;
        case CHT2_PCL_KEY_TYPE_L2_L3_L4_KEY_LONG_E:
            snetCht2IPclBuildL2L3L4ExtendedKey(devObjPtr , descrPtr ,
                                               currentLookupConfPtr ,
                                               pclExtraDataPtr,
                                               pclKeyPtr);
            break;
        case CHT2_PCL_KEY_TYPE_L2_IPV6_KEY_LONG_E:
            snetCht2IPclKeyBuildL2IPv6ExtendedKey(devObjPtr , descrPtr ,
                                                  currentLookupConfPtr ,
                                                  pclExtraDataPtr,
                                                  pclKeyPtr);
            break;
        case CHT2_PCL_KEY_TYPE_L4_IPV6_KEY_LONG_E:
            snetCht2IPclKeyBuildL4IPv6ExtendedKey(devObjPtr , descrPtr ,
                                                  currentLookupConfPtr ,
                                                  pclExtraDataPtr,
                                                  pclKeyPtr);
            break;
        default:

        break;
    }

    return ;
}

/*******************************************************************************
* snetCht2IPclGetCurrentConfig
*
* DESCRIPTION:
*        Get lookup data from the PCL configuration table
*
* INPUTS:
*       cycleNo         - cycle num.
*       lookupConfigPtr - pointer to the PCL configuration table entry.
*
* OUTPUTS:
*       currentLookupConfPtr    - current (cycleNum) lookup configuration.
*
* RETURN:
*
* COMMENTS:
*       Get the current configuration depends on cycle number.
*
*******************************************************************************/
static GT_VOID snetCht2IPclGetCurrentConfig
(
    IN GT_U8                                            cycleNo,
    IN SNET_CHT2_IPOLICY_LOOKUP_CONFIG_STC *            lookupConfigPtr,
    OUT SNET_CHT2_POLICY_SPECIFIC_LOOKUP_CONFIG_STC *   currentLookupConfPtr
)
{
    SNET_CHT2_POLICY_SPECIFIC_LOOKUP_CONFIG_STC *lookupConf;

    if(cycleNo == 0)
    {
        lookupConf = &lookupConfigPtr->lookup0Config;
    }
    else
    {
        lookupConf = &lookupConfigPtr->lookup1Config;
    }

     memcpy (currentLookupConfPtr , lookupConf,
                sizeof(SNET_CHT2_POLICY_SPECIFIC_LOOKUP_CONFIG_STC));
    return ;
}

/*******************************************************************************
* snetCht2IPclTriggeringCheck
*
* DESCRIPTION:
*        Check the enabling status of ingress PCL for the incoming frame.
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - pointer to frame data buffer Id
*
* OUTPUTS:
*       doLookupPtr     - pointer to enabling the PCL engine.
*       lookpuConfPtr   - pointer to lookup configuration.
*       extendedLookupModePtr - pointer to extended lookup mode.
*
* RETURN:
*
* COMMENTS:
*         1. Check PCL is enabled.
*         2. Get Ingress PCL configuration entry.
*
*******************************************************************************/
static GT_VOID snetCht2IPclTriggeringCheck
(
    IN SKERNEL_DEVICE_OBJECT                * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC      * descrPtr,
    OUT GT_U8                               * doLookupPtr,
    OUT SNET_CHT2_IPOLICY_LOOKUP_CONFIG_STC * lookupConfPtr
)
{
    DECLARE_FUNC_NAME(snetCht2IPclTriggeringCheck);

    GT_U32  regAddress;
    GT_U32  policyDisabled;

    *doLookupPtr = 0;

    /* check if the packet command is drop */
    __LOG(("check if the packet command is drop"));
    if (descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_HARD_DROP_E)
    {
        return;
    }

    /* check if PCL is globally enabled */
    __LOG(("check if PCL is globally enabled"));
    regAddress = SMEM_CHT_PCL_GLOBAL_REG(devObjPtr);
    smemRegFldGet(devObjPtr, regAddress, 7, 1, &policyDisabled);
    if (policyDisabled)
    {
        return ;
    }

    /* check is the PCL is enabled on port */
    __LOG(("check is the PCL is enabled on port"));
    if (descrPtr->policyOnPortEn == 0)
    {
        return ;
    }

    snetCht2IPclConfigPtrGet(devObjPtr,descrPtr,lookupConfPtr);

    *doLookupPtr = (lookupConfPtr->lookup0Config.enabled ||
                        lookupConfPtr->lookup1Config.enabled ) ? 1 : 0 ;

    return ;
}


/*******************************************************************************
* snetCht2Pcl
*
* DESCRIPTION:
*        Ingress Policy Engine processing for incoming frame  on Cheetah2
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
*
*
*******************************************************************************/
extern GT_VOID snetCht2IPcl
(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr
)
{
    DECLARE_FUNC_NAME(snetCht2IPcl);

    GT_U8  cycleNum; /* cycle number */
    SNET_CHT2_IPOLICY_LOOKUP_CONFIG_STC lookupConf; /* lookup configuration
                                                       Table 266: Ingress PCL
                                                       configuration Table    */
    SNET_CHT2_POLICY_SPECIFIC_LOOKUP_CONFIG_STC currentLookupConf;/* current
                                                        lookup configuration ,
                                                        for one of the cycles */

    GT_U8                           doLookup;     /* perform TCAM lookup or not */
    CHT_PCL_EXTRA_PACKET_INFO_STC   pclExtraData;
    SNET_CHT_POLICY_KEY_STC         pclKey;       /* policy key structure */
    CHT2_PCL_KEY_TYPE_ENT           keyType;      /* type of tcam search key*/
    GT_U32                          matchIndex;   /* index to the match action */
    SNET_CHT2_PCL_ACTION_STC        actionData;   /* action to be performed */
    GT_BOOL                         isFirst = GT_TRUE;/* is first action apply */

    /* Get the PCL */
    __LOG(("Get the PCL"));
    snetCht2IPclTriggeringCheck(devObjPtr,descrPtr, &doLookup,&lookupConf);

    if (doLookup == 0) /* =0 , means PCL engine is disabled */
    {
        __LOG(("PCL engine is disabled.\n"));
        return ;
    }
    /* get extra dat from packet . will be used in PCL engine */
    __LOG(("get extra dat from packet . will be used in PCL engine"));
    snetChtPclSrvParseExtraData(devObjPtr, descrPtr, &pclExtraData);

    pclKey.devObjPtr = devObjPtr;

    /* perform the PCL engine for both lookups */
    __LOG(("perform the PCL engine for both lookups"));
    for (cycleNum = 0 ; cycleNum < 2 ; ++cycleNum)
    {
        /* Get the current configuration from the ingress PCL config. table*/
        __LOG(("Get the current configuration from the ingress PCL config. table"));
        snetCht2IPclGetCurrentConfig(cycleNum,&lookupConf,&currentLookupConf);

        if (currentLookupConf.enabled == GT_FALSE)
        {
            continue ;
        }

        /* Get from the configuration the key type and the keysize */
        __LOG(("Get from the configuration the key type and the keysize"));
        snetCht2IPclGetKeyType(descrPtr, &currentLookupConf, &keyType, &pclKey);

        /* Create policy search key */
        __LOG(("Create policy search key"));
        snetCht2IPclCreateKey(devObjPtr, descrPtr ,&currentLookupConf,
                                &pclExtraData, keyType , &pclKey);
        /* search key in policy Tcam */
        __LOG(("search key in policy Tcam"));
        snetCht2SrvPclTcamLookUp(devObjPtr , descrPtr , &pclKey , &matchIndex);

        /* read and apply policy action */
        __LOG(("read and apply policy action"));
        if (matchIndex != SNET_CHT_POLICY_NO_MATCH_INDEX_CNS)
        {
           /* read the action from the pcl action table */
            __LOG(("read the action from the pcl action table"));
            snetCht2IPclActionGet(devObjPtr, descrPtr , matchIndex , &actionData);
            /* apply the action */
            __LOG(("apply the action"));
            snetCht2IPclActionApply(devObjPtr, descrPtr , &actionData, isFirst);
            isFirst = GT_FALSE;
        }
    }

    return;
}

/*******************************************************************************
* snetCht2PCLTcamLookUp
*
* DESCRIPTION:
*        Tcam lookup .
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - pointer to frame descriptor.
*       pclKeyPtr       - pointer to PCL key .
*
* OUTPUTS:
*       matchIndexPtr   - pointer to the matching index.
*
* RETURN:
*
* COMMENTS:
*       C.12.13     Policy Tcam Table : The policy TCAM holds 1024 lines
*                 of 24 bytes or rule data used for ingress and egress PCL.
*******************************************************************************/
extern GT_VOID snetCht2SrvPclTcamLookUp
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr  ,
    IN SNET_CHT_POLICY_KEY_STC    * pclKeyPtr,
    OUT GT_U32 *  matchIndexPtr
)
{
    DECLARE_FUNC_NAME(snetCht2SrvPclTcamLookUp);

    GT_U32   maxEntries;  /* 512 entries for extended PCE , 1024 for standard*/
    GT_U32   entryIndex;  /* Index to the TCAM rule */
    GT_U32   wordIndex;   /* Index to the word in the TCAM rule */
    GT_U32 * policyTcamEntryDataPtr; /* pointer to policy TCAM data entry  */
    GT_U32 * policyTcamEntryCtrlPtr; /* pointer to policy TCAM ctrl entry  */
    GT_U32 * policyTcamEntryDataMaskPtr; /* pointer to policy TCAM data mask entry */
    GT_U32 * policyTcamEntryCtrlMaskPtr; /* pointer to policy TCAM ctrl mask entry  */
    GT_U32   compModeData; /* compare mode bit in the tcam rule */
    GT_U32 * pclSearchKeyPtr; /* pointer to pcl tcam search0 key */
    GT_U32 * pclSearchKeyPtr1; /* pointer to pcl tcam search1 key */
    GT_U32   keySize;/* keysize . 5 words for standard key and 10 for extended*/
    GT_U32   policyTcamData32Bits;/* policy Tcam data 32 lower bits   */
    GT_U32   policyTcamData16Bits;/* policy Tcam data 16 upper bits   */
    GT_U32   pclSearchKey16Bits ;/* pcl tcam search pcl 32 lower bits   */
    GT_U32   pclSearchKey32Bits ; /* pcl tcam search pcl 16 lower bits   */
    GT_U32  *policyTcamEntryDataSecondHalfPtr;/* pointer to middle of data tcam table */
    GT_U32  *policyTcamEntryCtrlSecondHalfPtr;/* pointer to middle of ctrl tcam table */
    GT_U32  *policyTcamEntryDataMaskSecondHalfPtr;/* pointer to middle of data tcam mask table */
    GT_U32  *policyTcamEntryCtrlMaskSecondHalfPtr;/* pointer to middle of ctrl tcam mask table */
    GT_U32  *dataPtr;
    GT_U32  *dataMaskPtr;
    GT_U32  *ctrlPtr;
    GT_U32  *ctrlMaskPtr;
    GT_U32  memTcamAddr;

    if (pclKeyPtr->pclKeyFormat == CHT_PCL_KEY_EXTENDED_E)
    {
        maxEntries = SNET_CHT2_TCAM_1024_EXTENDED_RULES_CNS ;
        pclSearchKeyPtr1 = (GT_U32*)&(pclKeyPtr->key.extended);
        keySize = 10 ;
    }
    else
    {
        maxEntries = SNET_CHT2_TCAM_512_STANDARD_RULES_CNS  ;
        pclSearchKeyPtr1 = (GT_U32*)&(pclKeyPtr->key.regular);
        keySize = 5 ;
    }

    /* initialize the matchIndexPtr */
    *matchIndexPtr = SNET_CHT_POLICY_NO_MATCH_INDEX_CNS;

    memTcamAddr = SMEM_CHT_PCL_TCAM_TBL_MEM(devObjPtr, 0, 0);

    /*  Get pointer to Tcam data entry */
    policyTcamEntryDataPtr = smemMemGet(devObjPtr, memTcamAddr);

    /*  Get pointer to Tcam control entry */
    policyTcamEntryCtrlPtr = smemMemGet(devObjPtr, memTcamAddr + 0x4);

    /*  Get pointer to Tcam data mask entry */
    policyTcamEntryDataMaskPtr = smemMemGet(devObjPtr, memTcamAddr + 0x8);

    /*  Get pointer to Tcam control mask entry */
    policyTcamEntryCtrlMaskPtr = smemMemGet(devObjPtr, memTcamAddr + 0xc);

    /*  Get pointer to Tcam data entry second half table */
    policyTcamEntryDataSecondHalfPtr =  policyTcamEntryDataPtr +
            CHT2_PCL_TCAM_OFFSET_TO_SECOND_HALF_TABLE_IN_WORD_CNS ;

    /*  Get pointer to Tcam ctrl entry second half table */
    policyTcamEntryCtrlSecondHalfPtr = policyTcamEntryCtrlPtr +
            CHT2_PCL_TCAM_OFFSET_TO_SECOND_HALF_TABLE_IN_WORD_CNS ;

    /*  Get pointer to Tcam data mask entry second half table */
    policyTcamEntryDataMaskSecondHalfPtr =  policyTcamEntryDataMaskPtr +
            CHT2_PCL_TCAM_OFFSET_TO_SECOND_HALF_TABLE_IN_WORD_CNS ;

    /*  Get pointer to Tcam ctrl mask entry second half table */
    policyTcamEntryCtrlMaskSecondHalfPtr =  policyTcamEntryCtrlMaskPtr +
            CHT2_PCL_TCAM_OFFSET_TO_SECOND_HALF_TABLE_IN_WORD_CNS ;

    for (entryIndex = 0 ; entryIndex <  maxEntries;  ++entryIndex,
            policyTcamEntryDataPtr += CHT_PCL2_TCAM_ENTRY_WIDTH_CNS,
            policyTcamEntryCtrlPtr += CHT_PCL2_TCAM_ENTRY_WIDTH_CNS,
            policyTcamEntryDataMaskPtr += CHT_PCL2_TCAM_ENTRY_WIDTH_CNS,
            policyTcamEntryCtrlMaskPtr += CHT_PCL2_TCAM_ENTRY_WIDTH_CNS,
            policyTcamEntryDataSecondHalfPtr += CHT_PCL2_TCAM_ENTRY_WIDTH_CNS,
            policyTcamEntryCtrlSecondHalfPtr += CHT_PCL2_TCAM_ENTRY_WIDTH_CNS,
            policyTcamEntryDataMaskSecondHalfPtr += CHT_PCL2_TCAM_ENTRY_WIDTH_CNS,
            policyTcamEntryCtrlMaskSecondHalfPtr += CHT_PCL2_TCAM_ENTRY_WIDTH_CNS
        )
    {
        pclSearchKeyPtr = pclSearchKeyPtr1;
        dataPtr = policyTcamEntryDataPtr;
        ctrlPtr = policyTcamEntryCtrlPtr;
        dataMaskPtr = policyTcamEntryDataMaskPtr;
        ctrlMaskPtr = policyTcamEntryCtrlMaskPtr;

        for (wordIndex = 0 ; wordIndex <  keySize  ; ++wordIndex,
                                dataPtr += CHT_PCL2_TCAM_WORDS_WIDTH_CNS,
                                ctrlPtr += CHT_PCL2_TCAM_WORDS_WIDTH_CNS,
                                dataMaskPtr += CHT_PCL2_TCAM_WORDS_WIDTH_CNS,
                                ctrlMaskPtr += CHT_PCL2_TCAM_WORDS_WIDTH_CNS
            )
        {
            if (wordIndex == 5)
            {
                dataPtr =  policyTcamEntryDataSecondHalfPtr;
                ctrlPtr =  policyTcamEntryCtrlSecondHalfPtr;
                dataMaskPtr = policyTcamEntryDataMaskSecondHalfPtr;
                ctrlMaskPtr =  policyTcamEntryCtrlMaskSecondHalfPtr;
            }

            /* check validity of entry */
            __LOG(("check validity of entry"));
            if (( ((ctrlPtr[0] >> 0x11) & 0x1) &
                  ((ctrlMaskPtr[0] >> 0x11) & 0x1)) == 0x0)
            {
                break; /* not valid entry  - go to the next entry */
            }
            /* check the compmode */
            __LOG(("check the compmode"));
            compModeData =  ( ((ctrlPtr[0] >> 0x12) & 0x3 ) &
                               ((ctrlMaskPtr[0] >> 0x12) & 0x3) );
            if ((( compModeData == 1 ) &&  ( pclKeyPtr->pclKeyFormat == CHT_PCL_KEY_EXTENDED_E )) ||
                (( compModeData == 2 ) &&  ( pclKeyPtr->pclKeyFormat == CHT_PCL_KEY_REGULAR_E)))
            {
                break; /* compmode is not as expected - go to the next entry */
            }

            if ( (wordIndex == 1) || (wordIndex == 6) )
            {
                /* calculate 48 bits for word1 */
                __LOG(("calculate 48 bits for word1"));
                policyTcamData16Bits = (SMEM_U32_GET_FIELD(ctrlPtr[0],0,15) &
                                       SMEM_U32_GET_FIELD(ctrlMaskPtr[0],0,15));
                policyTcamData32Bits = (dataPtr[0] & dataMaskPtr[0]);

                /* calculate 48 bits for word1 from the PCL search key */
                __LOG(("calculate 48 bits for word1 from the PCL search key"));
                pclSearchKey16Bits = SMEM_U32_GET_FIELD(pclSearchKeyPtr[1],0,15);
                pclSearchKey32Bits = pclSearchKeyPtr[0];
                pclSearchKeyPtr++;

                pclSearchKey16Bits &= SMEM_U32_GET_FIELD(ctrlMaskPtr[0],0,15);
                pclSearchKey32Bits &= dataMaskPtr[0];
            }
            else if ( (wordIndex == 2) || (wordIndex ==7) )
            {
                /* calculate 48 bits for word2 */
                __LOG(("calculate 48 bits for word2"));
                policyTcamData16Bits = (SMEM_U32_GET_FIELD(ctrlPtr[0],0,15) &
                                        SMEM_U32_GET_FIELD(ctrlMaskPtr[0],0,15));
                policyTcamData32Bits = (dataPtr[0] & dataMaskPtr[0]);

                /* calculate 48 bits for word2 from the PCL search key */
                __LOG(("calculate 48 bits for word2 from the PCL search key"));
                pclSearchKey32Bits = SMEM_U32_GET_FIELD(pclSearchKeyPtr[0],16,16);
                pclSearchKey32Bits |= SMEM_U32_GET_FIELD(pclSearchKeyPtr[1],0,16) << 16;
                pclSearchKey16Bits = SMEM_U32_GET_FIELD(pclSearchKeyPtr[1],16,16);
                pclSearchKeyPtr += 2;

                pclSearchKey16Bits &= SMEM_U32_GET_FIELD(ctrlMaskPtr[0],0,15);
                pclSearchKey32Bits &= dataMaskPtr[0];
            }
            else
            {
                /* 16 extended bits for word1-2 are zero */
                __LOG(("16 extended bits for word1-2 are zero"));
                pclSearchKey16Bits = 0;
                policyTcamData16Bits = 0;

                /* calculate 32 bits for word 0 , word3 and word4 */
                __LOG(("calculate 32 bits for word 0 , word3 and word4"));
                policyTcamData32Bits = (dataPtr[0] & dataMaskPtr[0]);
                pclSearchKey32Bits = pclSearchKeyPtr[0];
                pclSearchKeyPtr++;

                pclSearchKey32Bits &= dataMaskPtr[0];
            }

            /* lookup in TCAM */
            __LOG(("lookup in TCAM"));
            if ((policyTcamData16Bits != pclSearchKey16Bits) ||
                (policyTcamData32Bits != pclSearchKey32Bits))
                break;

        }

        if (wordIndex == keySize)
        {
            *matchIndexPtr = entryIndex;
            break ;
        }
    }

    return ;
}

