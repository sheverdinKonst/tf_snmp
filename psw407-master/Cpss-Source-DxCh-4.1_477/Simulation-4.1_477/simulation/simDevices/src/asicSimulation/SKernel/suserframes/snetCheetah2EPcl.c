/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetCheetah2EPcl.c
*
* DESCRIPTION:
*       Cheetah2 Asic Simulation .
*       Egress Policy Engine processing for outgoing frame.
*       Source Code file.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 22 $
*
*******************************************************************************/

#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SKernel/smem/smemCheetah.h>
#include <common/Utils/FrameInfo/sframeInfoAddr.h>
#include <asicSimulation/SKernel/cheetahCommon/sregCheetah.h>
#include <asicSimulation/SKernel/suserframes/snet.h>
#include <asicSimulation/SKernel/suserframes/snetCheetah2EPcl.h>
#include <asicSimulation/SLog/simLog.h>

/* number of compares to be done on Layer 4 ports */
#define SNET_CHT2_L4_TCP_PORT_COMP_RANGE_INDEX_CNS      (0x8)

 /* not in use

static CHT_PCL_USER_DEFINED_CONFIG_REG_STC cht2PclUserDefinedConfigInfoShortL2KeyArray[3] =
{
    {0 ,7 },
    {11,18},
    {22,29}
};

static CHT_PCL_USER_DEFINED_CONFIG_REG_STC cht2PclUserDefinedConfigInfoShortL2L3KeyArray[2] =
{
    {11,18},
    {22,29}
}; */

static CHT_PCL_USER_DEFINED_CONFIG_REG_STC cht2PclUserDefinedConfigInfoShortL3L4KeyArray[3] =
{
    {0,7},
    {11,18},
    {22,29}
};

static CHT_PCL_USER_DEFINED_CONFIG_REG_STC cht2PclUserDefinedConfigInfoLongL2L3L4KeyArray[6] =
{
    {0 , 7},
    {11 ,18},
    {22 , 29},
    {0 , 7},
    {11,18},
    {22,29}
};

static CHT_PCL_USER_DEFINED_CONFIG_REG_STC cht2PclUserDefinedConfigInfoLongL2Ipv6KeyArray[6] =
{
    {0 , 7},
    {11 ,18},
    {22 , 29},
    {0 , 7},
    {11,18},
    {22,29}
};

static CHT_PCL_USER_DEFINED_CONFIG_REG_STC cht2PclUserDefinedConfigInfoLongL4Ipv6KeyArray[3] =
{
    {0 ,7 },
    {11 ,18 },
    {22 , 29},
};

/* array that holds the info about the fields */
static CHT_PCL_KEY_FIELDS_INFO_STC cht2ePclKeyFieldsData[CHT2_EPCL_KEY_FIELDS_ID_LAST_E]=
{
    {0  ,0  ,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_VALID_E "  },
    {1  ,10 ,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_PCL_ID_E "  },
    {11 ,16 ,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_SRC_PORT_E "  },
    {17 ,17 ,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_IS_TAGGED_E "  },
    {18 ,29 ,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_VID_E "  },
    {30 ,32 ,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_UP_E "  },
    {33 ,39 ,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_QOS_PROFILE_E "  },
    {40 ,40 ,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_IS_IPV4_E "  },
    {41 ,41 ,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_IS_IP_E "  },
    {42 ,42 ,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_RESERVED_42_E "  },
    {43 ,58 ,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_ETHER_TYPE_DSAP_SSAP_E "  },
    {59 ,66 ,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_RESERVED_66_59_SHORT_E "  },
    {67 ,72 ,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_RESERVED_72_67_E "  },
    {73 ,73 ,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_IS_ARP_E "  },
    {74 ,74 ,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_RESERVED_74_E "  },
    {82 ,82 ,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_SRC_IS_TRUNK_SHORT_E "},
    {88 ,88 ,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_IS_ROUTED "},
    {90 ,90 ,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_IS_L2_VALID_SHORT_E "  },
    {91 ,91 ,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_ENCAP_TYPE_SHORT_E "  },
    {92 ,139,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_MAC_SA_SHORT_E "  },
    {140,187,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_MAC_DA_SHORT_E "  },
    {188,189,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_PACKET_TYPE_SHORT_E "},
    {190,190,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_RESERVED_190_E "} ,
    {191,191,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_IS_VIDX_SHORT_E "},
    {42 ,49 ,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_IP_PROTOCOL_E "},
    {50 ,55 ,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_DSCP_E "} ,
    {56 ,56 ,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_IS_L4_VALID_E "},
    {57 ,72 ,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_2_3_E "},
    {57 ,72 ,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_4_5_E "},
    {91 ,98 ,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_TCP_UDP_PORT_COMPS_SHORT_E "},
    {99 ,130,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_DIP_31_0_E "},
    {131,138,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_L4_BYTES_OFFSETS_13_SHORT_E "},
    {139,139,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_RESERVED_139_SHORT_E "},
    {190,190,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_IPV4_FRAGMENTED_SHORT_E "}   ,
    {74 ,74 ,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_IS_BC_E "},
    {99 ,130,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_SIP_31_0_SHORT_E "}   ,
    {131,162,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_DIP_31_0_IPV4_L4_SHORT_E "},
    {163,186,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_L4_BYTES_OFFSETS_0_1_13_SHORT_E "} ,
    {187,187,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_RESERVED_187_E "},
    {40 ,40 ,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_IS_IPV6_E "}      ,
    {73 ,96 ,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_L4_BYTES_OFFSETS_0_1_13_LONG_E "},
    {97 ,128,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_SIP_31_0_LONG_E " },
    {129,160,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_DIP_31_0_LONG_E "},
    {161,161,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_ENCAP_TYPE_LONG_VALID_E "},
    {162,177,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_ETHER_TYPE_DSAP_SSAP_LONG_E "},
    {179,234,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_LONG_RESERVED_179_234_E "},
    {331,338,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_USER_DEFINED_BYTE3_338_331_LONG_E "},
    {339,346,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_USER_DEFINED_BYTE4_346_339_LONG_E "},
    {347,354,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_USER_DEFINED_BYTE5_354_347_LONG_E "},
    {355,362,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_USER_DEFINED_BYTE0_362_355_LONG_E "},
    {363,370,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_USER_DEFINED_BYTE1_370_363_LONG_E "},
    {371,378,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_USER_DEFINED_BYTE2_378_371_LONG_E "},
    {371,378,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_TCP_UDP_PORT_COMPS_LONG_E "},
    {129,224,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_SIP_127_32_LONG_E "},
    {226,226,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_IS_IPV6_EH_HOP_BY_HOP_LONG_E "},
    {120,127,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_DIP_127_120_LONG_E "} ,
    {235,282,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_MAC_SA_LONG_E "},
    {283,330,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_MAC_DA_LONG_E "},
    {379,380,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_RESERVED_380_379_LONG_E "},
    {225,225,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_IS_IPV6_EH_EXISTS_LONG_E "},
    {235,322,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_DIP_119_32_LONG_E "},
    {323,354,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_DIP_0_31_IPV6_LONG_E "},
    {381,381,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_IS_L2_VALID_LONG_E "} ,
    {382,382,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_USER_DEFINED_VALID_LONG_E "},
    {383,383,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_IP_HEADER_OK_LONG_E "},
    {33 , 39,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_CPU_CODE_0_6_STANDARD_E "},
    {33 , 39,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_RESERVED_39_33_E "},
    {75 , 81,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_TRUNK_ID "},
    {75 , 81,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_SRCDEV_ID "},
    {83 , 87,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_SST_ID_E " },
    {89 , 89,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_IS_UNKNOWN_UC_E " },
    {89 , 89,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_CPU_CODE_7_STANDARD_E "},
    {178,178,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_LONG_IPV4_FRAGMENT_E "},
    {89 , 89,GT_TRUE," CHT2_EPCL_KEY_FIELDS_ID_RESERVED_89_E "}
};

#define SNET_CHT2_EPCL_KEY_FORMAT(key) \
    (key == CHT2_EPCL_KEY_TYPE_L2_KEY_SHORT_E) ? CHT_PCL_KEY_REGULAR_E :              \
    (key == CHT2_EPCL_KEY_TYPE_L2_L3_KEY_SHORT_E) ? CHT_PCL_KEY_REGULAR_E :           \
    (key == CHT2_EPCL_KEY_TYPE_L3_L4_KEY_SHORT_E) ? CHT_PCL_KEY_REGULAR_E :           \
    (key == CHT2_EPCL_KEY_TYPE_L2_L3_L4_KEY_LONG_E) ? CHT_PCL_KEY_EXTENDED_E :        \
    (key == CHT2_EPCL_KEY_TYPE_L2_IPV6_KEY_LONG_E) ? CHT_PCL_KEY_EXTENDED_E :         \
    (key == CHT2_EPCL_KEY_TYPE_L4_IPV6_KEY_LONG_E) ? CHT_PCL_KEY_EXTENDED_E :         \
                                                     CHT_PCL_KEY_TRIPLE_E

static GT_VOID snetCht2EPclConfigPtrGet
(
    IN SKERNEL_DEVICE_OBJECT                * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC      * descrPtr ,
    IN GT_U32                                 port ,
    OUT SNET_CHT2_EPCL_LOOKUP_CONFIG_STC    * lookupConfPtr
);

/*  not in use static GT_VOID snetCht2EPclParseExtraData
(
    IN SKERNEL_DEVICE_OBJECT                * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC      * descrPtr,
    OUT CHT_PCL_EXTRA_PACKET_INFO_STC       * pclExtraDataPtr
); */


/*******************************************************************************
*   snetCht2EPclKeyFieldBuildByPointer
*
* DESCRIPTION:
*       function insert data of field to the search key in specific place in key
*
* INPUTS:
*       pclKeyPtr   - (pointer to) current pcl key
*       fieldValPtr - (pointer to) data of field to insert to key
*       fieldId     - field id
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
static GT_VOID snetCht2EPclKeyFieldBuildByPointer
(
    INOUT SNET_CHT_POLICY_KEY_STC       *pclKeyPtr,
    IN GT_U8                            *fieldValPtr,
    IN CHT2_EPCL_KEY_FIELDS_ID_ENT      fieldId
)
{
    CHT_PCL_KEY_FIELDS_INFO_STC *fieldInfoPtr = &cht2ePclKeyFieldsData[fieldId];

    snetChtPclSrvKeyFieldBuildByPointer(pclKeyPtr, fieldValPtr, fieldInfoPtr);

    return;
}

/*******************************************************************************
*   snetCht2EPclKeyFieldBuildByValue
*
* DESCRIPTION:
*       function insert data of the field to the search key
*       in specific place in epcl search key
*
* INPUTS:
*       pclKeyPtr - (pointer to) current epcl key
*       fieldVal - data of field to insert to key
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
static GT_VOID snetCht2EPclKeyFieldBuildByValue
(
    INOUT SNET_CHT_POLICY_KEY_STC           *ePclKeyPtr,
    IN GT_U32                               fieldVal,
    IN CHT2_EPCL_KEY_FIELDS_ID_ENT          fieldId
)
{
    CHT_PCL_KEY_FIELDS_INFO_STC *fieldInfoPtr = &cht2ePclKeyFieldsData[fieldId];

    snetChtPclSrvKeyFieldBuildByValue(ePclKeyPtr, fieldVal, fieldInfoPtr);

    return;
}

/*******************************************************************************
*   snetCht2EPclKeyBuildL4IPv6ExtendedKey
*
* DESCRIPTION:
*        Build Layer4+IPv6 extended ePCL search key.
*
* INPUTS:
*       devObjPtr            - pointer to device object.
*       descrPtr             - pointer to frame data buffer Id.
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
static GT_VOID snetCht2EPclKeyBuildL4IPv6ExtendedKey
(
    IN SKERNEL_DEVICE_OBJECT                * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC      * descrPtr      ,
    IN SNET_CHT2_EPCL_LOOKUP_CONFIG_STC     * lookupConfPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC        * pclExtraDataPtr  ,
    OUT SNET_CHT_POLICY_KEY_STC             * pclKeyPtr
)
{
    GT_U32          fieldVal = 1; /* valid bit value */
    GT_U32          resrvdVal = 0 ; /* reserved bit value */
    GT_U32          userDefdata0 = 0;
    CHT_PCL_USER_DEFINED_CONFIG_REG_STC *userDefinedArrayPtr;
    GT_U32          userDefinedAnchor; /* user defined byte Anchor */
    GT_U32          userDefinedOffset; /* user defined byte offset from Anchor */
    GT_U8           byteValue;/* value of user defined byte from the packets or tcp/udp compare */
    GT_U32          userDefinedError = 0;/* is there user defined byte error */
    GT_STATUS       rc;/* return code from function */
    GT_U32          tmpVal; /* temp val */
    GT_U8           tmpFieldVal[4];

    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, fieldVal, /* bits[0] Valid */
                                        CHT2_EPCL_KEY_FIELDS_ID_VALID_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, lookupConfPtr->lookupConfig.pclID ,
                                        CHT2_EPCL_KEY_FIELDS_ID_PCL_ID_E);/*[10:1] ePCL-ID */
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->localDevSrcPort ,
                                        CHT2_EPCL_KEY_FIELDS_ID_SRC_PORT_E);/* [16:11] src port */
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->egressTagged ,
                                        CHT2_EPCL_KEY_FIELDS_ID_IS_TAGGED_E);/* [17] is tagged */
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->eVid ,
                                        CHT2_EPCL_KEY_FIELDS_ID_VID_E);/* [29:18] eVid */
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->up  ,
                                        CHT2_EPCL_KEY_FIELDS_ID_UP_E); /* [32:30] UP */
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->qos.qosProfile,/* [39:33] QOS profile */
                                        CHT2_EPCL_KEY_FIELDS_ID_QOS_PROFILE_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, (descrPtr->isIPv4==0 && descrPtr->isIp),
                                        CHT2_EPCL_KEY_FIELDS_ID_IS_IPV6_E);/* [40] is IPv6 */
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->isIp,/* [41] is IP */
                                        CHT2_EPCL_KEY_FIELDS_ID_IS_IP_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->ipProt,/* [49:42] IP Protocol*/
                                        CHT2_EPCL_KEY_FIELDS_ID_IP_PROTOCOL_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dscp,/* [55:50] dscp */
                                        CHT2_EPCL_KEY_FIELDS_ID_DSCP_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL4Valid, /* [56] isL4Valid*/
                                        CHT2_EPCL_KEY_FIELDS_ID_IS_L4_VALID_E);
    if (descrPtr->l4StartOffsetPtr != NULL)
    {
        if (descrPtr->ipv6Icmp || descrPtr->ipv4Icmp)
        {/* [72:57] L4 Byte Offsets 5:4 */
            snetCht2EPclKeyFieldBuildByPointer(
                pclKeyPtr,(GT_U8*)&(descrPtr->l4StartOffsetPtr[4]),
                                        CHT2_EPCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_4_5_E);
        }
        else
        {/* [72:57] L4 Byte Offsets 3:2 */
            snetCht2EPclKeyFieldBuildByPointer(
                pclKeyPtr,(GT_U8*)&(descrPtr->l4StartOffsetPtr[2]),
                                        CHT2_EPCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_2_3_E);
        }
        tmpFieldVal[0] = descrPtr->l4StartOffsetPtr[0];
        tmpFieldVal[1] = descrPtr->l4StartOffsetPtr[1];
        tmpFieldVal[2] = descrPtr->l4StartOffsetPtr[13];
        snetCht2EPclKeyFieldBuildByPointer(
            pclKeyPtr,(GT_U8*)&tmpFieldVal[0],/* [96:73] L4 Byte Offsets[1:0],[13] */
                    CHT2_EPCL_KEY_FIELDS_ID_L4_BYTES_OFFSETS_0_1_13_LONG_E);
    }

    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[3], /* [128:97] SIP*/
                                    CHT2_EPCL_KEY_FIELDS_ID_SIP_31_0_LONG_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[0], /* [224:129] SIP*/
                                    CHT2_EPCL_KEY_FIELDS_ID_SIP_127_32_LONG_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpV6EhExists, /* [225] ipv6_eh_exist */
                            CHT2_EPCL_KEY_FIELDS_ID_IS_IPV6_EH_EXISTS_LONG_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpV6EhHopByHop, /* [226] ipv6_eh_hopbyhop */
                            CHT2_EPCL_KEY_FIELDS_ID_IS_IPV6_EH_HOP_BY_HOP_LONG_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, (descrPtr->dip[0] >> 24),/* [234:227] DIP */
                                    CHT2_EPCL_KEY_FIELDS_ID_DIP_127_120_LONG_E);
    snetCht2EPclKeyFieldBuildByPointer(pclKeyPtr,(GT_U8*)&descrPtr->dip[0],/* [322:235] DIP */
                                    CHT2_EPCL_KEY_FIELDS_ID_DIP_119_32_LONG_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->dip[3], /* [354:323] DIP */
                                    CHT2_EPCL_KEY_FIELDS_ID_DIP_0_31_IPV6_LONG_E);

    smemRegGet(devObjPtr,SMEM_CHT2_KEY5_USERDEFINED_BYTES_LONG_L4_IPv6_KEY_REG(devObjPtr),&userDefdata0);
    /* Key2 User Define Byte0  */
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

    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [362:355] UserDefinedByte0 */
                        CHT2_EPCL_KEY_FIELDS_ID_USER_DEFINED_BYTE0_362_355_LONG_E);

    /* Key2 User Define Byte1  */
    userDefinedAnchor = SMEM_U32_GET_FIELD(userDefdata0,userDefinedArrayPtr[1].startBitAnchor,2);
    userDefinedOffset = SMEM_U32_GET_FIELD(userDefdata0,userDefinedArrayPtr[1].startBitOffset,7);

    rc = snetChtPclUserDefinedByteGet(devObjPtr,descrPtr,userDefinedAnchor,
                                userDefinedOffset,&byteValue);
    if(rc != GT_OK)
    {
        userDefinedError = GT_TRUE;
        byteValue = 0;
    }

    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [370:363] UserDefinedByte1 */
                        CHT2_EPCL_KEY_FIELDS_ID_USER_DEFINED_BYTE1_370_363_LONG_E);

    if (userDefdata0 & (1 << 0x1F))
    {/* TCP/UDP Range Comparators is enable */
        rc = snetCht2EPclTcpUdpPortRangeCompareGet(devObjPtr,descrPtr,&byteValue);
        byteValue = (rc != GT_OK) ? 0 : byteValue ;
        snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [378:371] TCP/UDP comparator */
                            CHT2_EPCL_KEY_FIELDS_ID_TCP_UDP_PORT_COMPS_LONG_E);
    }
    else
    {   /* Key2 User Define Byte2  */
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

        snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [378:371] UserDefinedByte2 */
                            CHT2_EPCL_KEY_FIELDS_ID_USER_DEFINED_BYTE2_378_371_LONG_E);
    }

    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, resrvdVal,/* Reserved [380:379] */
                                CHT2_EPCL_KEY_FIELDS_ID_RESERVED_380_379_LONG_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr,pclExtraDataPtr->isL2Valid ,/* is L2 valid [381] */
                                    CHT2_EPCL_KEY_FIELDS_ID_IS_L2_VALID_LONG_E);
    tmpVal = (userDefinedError == GT_TRUE) ? 1 : 0;
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, tmpVal,/* [382] UserDefineValid */
                        CHT2_EPCL_KEY_FIELDS_ID_USER_DEFINED_VALID_LONG_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpHeaderOk ,
                            CHT2_EPCL_KEY_FIELDS_ID_IP_HEADER_OK_LONG_E);/* [383] ip header ok */

    return  ;
}

/*******************************************************************************
*   snetCht2EPclKeyBuildL2IPv6ExtendedKey
*
* DESCRIPTION:
*        Build Layer2+IPv6 extended ePCL search key.
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       descrPtr            - pointer to frame data buffer Id.
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
static GT_VOID snetCht2EPclKeyBuildL2IPv6ExtendedKey
(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr   ,
    IN SNET_CHT2_EPCL_LOOKUP_CONFIG_STC * lookupConfPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC    * pclExtraDataPtr ,
    OUT SNET_CHT_POLICY_KEY_STC         * pclKeyPtr
)
{
    GT_U32              fieldVal; /* valid bit value */
    GT_U32              resrvdVal; /* reserved bit value */
    GT_U32              userDefdata0;
    CHT_PCL_USER_DEFINED_CONFIG_REG_STC *userDefinedArrayPtr;
    GT_U32              userDefinedAnchor; /* user defined byte Anchor */
    GT_U32              userDefinedOffset;/* user defined byte offset from Anchor */
    GT_U8               byteValue;/* value of user defined byte from the packets or tcp/udp compare */
    GT_U32              userDefinedError = 0;/* is there user defined byte error */
    GT_STATUS           rc;/* return code from function */
    GT_U32              tmpVal;/* temp val */
    GT_U8               tmpFieldVal[4];

    fieldVal = 1;
    resrvdVal = 0;

    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, fieldVal, /* bits[0] Valid */
                                        CHT2_EPCL_KEY_FIELDS_ID_VALID_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, lookupConfPtr->lookupConfig.pclID ,
                                        CHT2_EPCL_KEY_FIELDS_ID_PCL_ID_E);/*[10:1] ePCL-ID */
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->localDevSrcPort ,
                                        CHT2_EPCL_KEY_FIELDS_ID_SRC_PORT_E);/* [16:11] src port */
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->egressTagged ,/* [17] is tagged */
                                        CHT2_EPCL_KEY_FIELDS_ID_IS_TAGGED_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->eVid ,/* [29:18] eVid */
                                        CHT2_EPCL_KEY_FIELDS_ID_VID_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->up  ,/* [32:30] UP */
                                        CHT2_EPCL_KEY_FIELDS_ID_UP_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->qos.qosProfile,/* [39:33] QOS profile */
                                        CHT2_EPCL_KEY_FIELDS_ID_QOS_PROFILE_E);   /* [40] is IPv6 */
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, (descrPtr->isIPv4==0 && descrPtr->isIp),
                                        CHT2_EPCL_KEY_FIELDS_ID_IS_IPV6_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->isIp,/* [41] is IP */
                                        CHT2_EPCL_KEY_FIELDS_ID_IS_IP_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->ipProt,/* [49:42] IP Protocol*/
                                        CHT2_EPCL_KEY_FIELDS_ID_IP_PROTOCOL_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dscp,/* [55:50] dscp */
                                        CHT2_EPCL_KEY_FIELDS_ID_DSCP_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL4Valid, /* [56] isL4Valid*/
                                        CHT2_EPCL_KEY_FIELDS_ID_IS_L4_VALID_E);
    if (descrPtr->l4StartOffsetPtr != NULL)
    {
        if (descrPtr->ipv6Icmp || descrPtr->ipv4Icmp)
        {/* [72:57] L4 Byte Offsets 5:4 */
            snetCht2EPclKeyFieldBuildByPointer(
                pclKeyPtr,(GT_U8*)&(descrPtr->l4StartOffsetPtr[4]),
                        CHT2_EPCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_4_5_E);
        }
        else
        {/* [72:57] L4 Byte Offsets 3:2 */
            snetCht2EPclKeyFieldBuildByPointer(
                pclKeyPtr,(GT_U8*)&(descrPtr->l4StartOffsetPtr[2]),
                        CHT2_EPCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_2_3_E);
        }
        tmpFieldVal[0] = descrPtr->l4StartOffsetPtr[0];
        tmpFieldVal[1] = descrPtr->l4StartOffsetPtr[1];
        tmpFieldVal[2] = descrPtr->l4StartOffsetPtr[13];
        snetCht2EPclKeyFieldBuildByPointer(
            pclKeyPtr,(GT_U8*)&tmpFieldVal[0],/* [73:96] L4 Byte Offsets[1:0],[13] */
                CHT2_EPCL_KEY_FIELDS_ID_L4_BYTES_OFFSETS_0_1_13_LONG_E);
    }
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[3], /* [128:97] SIP*/
                                    CHT2_EPCL_KEY_FIELDS_ID_SIP_31_0_LONG_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[0], /* [224:129] SIP*/
                                    CHT2_EPCL_KEY_FIELDS_ID_SIP_127_32_LONG_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpV6EhExists, /* [225] ipv6_eh_exist */
                                    CHT2_EPCL_KEY_FIELDS_ID_IS_IPV6_EH_EXISTS_LONG_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpV6EhHopByHop, /* [226] ipv6_eh_hopbyhop */
                                    CHT2_EPCL_KEY_FIELDS_ID_IS_IPV6_EH_HOP_BY_HOP_LONG_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, (descrPtr->dip[0] >> 24),  /* [234:227] DIP */
                                    CHT2_EPCL_KEY_FIELDS_ID_DIP_127_120_LONG_E);
    snetCht2EPclKeyFieldBuildByPointer(pclKeyPtr, descrPtr->macSaPtr,/* [282:235] MAC SA*/
                                    CHT2_EPCL_KEY_FIELDS_ID_MAC_SA_LONG_E);
    snetCht2EPclKeyFieldBuildByPointer(pclKeyPtr, descrPtr->macDaPtr,/* [330:283] MAC DA*/
                                    CHT2_EPCL_KEY_FIELDS_ID_MAC_DA_LONG_E);

    smemRegGet(devObjPtr,SMEM_CHT2_KEY1_0_USERDEFINED_BYTES_LONG_L2_IPv6_KEY_REG(devObjPtr),&userDefdata0);
    /* Key0 User Define Byte3  */
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

    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [338:331] UserDefinedByte3 */
                        CHT2_EPCL_KEY_FIELDS_ID_USER_DEFINED_BYTE3_338_331_LONG_E);

    /* Key0 User Define Byte4  */
    userDefinedAnchor = SMEM_U32_GET_FIELD(userDefdata0,userDefinedArrayPtr[4].startBitAnchor,2);
    userDefinedOffset = SMEM_U32_GET_FIELD(userDefdata0,userDefinedArrayPtr[4].startBitOffset,7);

    rc = snetChtPclUserDefinedByteGet(devObjPtr,descrPtr,userDefinedAnchor,
                                            userDefinedOffset,&byteValue);
    if(rc != GT_OK)
    {
        userDefinedError = GT_TRUE;
        byteValue = 0;
    }

    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [346:339] UserDefinedByte4 */
                        CHT2_EPCL_KEY_FIELDS_ID_USER_DEFINED_BYTE4_346_339_LONG_E);

    /* Key0 User Define Byte5  */
    userDefinedAnchor = SMEM_U32_GET_FIELD(userDefdata0,userDefinedArrayPtr[5].startBitAnchor,2);
    userDefinedOffset = SMEM_U32_GET_FIELD(userDefdata0,userDefinedArrayPtr[5].startBitOffset,7);

    rc = snetChtPclUserDefinedByteGet(devObjPtr,descrPtr,userDefinedAnchor,
                                            userDefinedOffset,&byteValue);
    if(rc != GT_OK)
    {
        userDefinedError = GT_TRUE;
        byteValue = 0;
    }

    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [354:347] UserDefinedByte5 */
                        CHT2_EPCL_KEY_FIELDS_ID_USER_DEFINED_BYTE5_354_347_LONG_E);

    /* Key0 User Define Byte0  */
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

    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [355:362] UserDefinedByte0 */
                        CHT2_EPCL_KEY_FIELDS_ID_USER_DEFINED_BYTE0_362_355_LONG_E);

    /* Key0 User Define Byte1  */
    userDefinedAnchor = SMEM_U32_GET_FIELD(userDefdata0,userDefinedArrayPtr[1].startBitAnchor,2);
    userDefinedOffset = SMEM_U32_GET_FIELD(userDefdata0,userDefinedArrayPtr[1].startBitOffset,7);

    rc = snetChtPclUserDefinedByteGet(devObjPtr,descrPtr,userDefinedAnchor,
                                        userDefinedOffset,&byteValue);
    if(rc != GT_OK)
    {
        userDefinedError = GT_TRUE;
        byteValue = 0;
    }

    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [370:363] UserDefinedByte1 */
                        CHT2_EPCL_KEY_FIELDS_ID_USER_DEFINED_BYTE1_370_363_LONG_E);
    if (userDefdata0 & (1 << 0x1F))
    {/* TCP/UDP Range Comparators is enable */
        rc = snetCht2EPclTcpUdpPortRangeCompareGet(devObjPtr,descrPtr,&byteValue);
        byteValue = (rc != GT_OK) ? 0 : byteValue ;
        snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [378:371] TCP/UDP comparator */
                            CHT2_EPCL_KEY_FIELDS_ID_TCP_UDP_PORT_COMPS_LONG_E);
    }
    else
    {   /* Key5 User Define Byte2  */
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

        snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [371:378] UserDefinedByte2 */
                                CHT2_EPCL_KEY_FIELDS_ID_USER_DEFINED_BYTE2_378_371_LONG_E);
    }

    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, resrvdVal,/* Reserved [379:380] */
                            CHT2_EPCL_KEY_FIELDS_ID_RESERVED_380_379_LONG_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, resrvdVal,/* is L2 valid [381] */
                            CHT2_EPCL_KEY_FIELDS_ID_IS_L2_VALID_LONG_E);
    tmpVal = (userDefinedError == GT_TRUE) ? 1 : 0;
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, tmpVal,/* [382] UserDefineValid */
                        CHT2_EPCL_KEY_FIELDS_ID_USER_DEFINED_VALID_LONG_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpHeaderOk ,
                            CHT2_EPCL_KEY_FIELDS_ID_IP_HEADER_OK_LONG_E);/* [383] ip header  */

    return  ;
}

/*******************************************************************************
*   snetCht2EPclBuildL2L3L4ExtendedKey
*
* DESCRIPTION:
*        Build layer2+IPv4+layer4 extended epcl search tcam key.
*
* INPUTS:
*       devObjPtr            - pointer to device object.
*       descrPtr             - pointer to frame data buffer Id.
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
static GT_VOID snetCht2EPclBuildL2L3L4ExtendedKey
(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr    ,
    IN SNET_CHT2_EPCL_LOOKUP_CONFIG_STC * lookupConfPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC    * pclExtraDataPtr  ,
    OUT SNET_CHT_POLICY_KEY_STC         * pclKeyPtr
)
{
    DECLARE_FUNC_NAME(snetCht2EPclBuildL2L3L4ExtendedKey);

    GT_U32              fieldVal; /* valid bit value */
    GT_U32              resrvdVal;  /* reserved bit value */
    GT_U32              userDefdata0;
    CHT_PCL_USER_DEFINED_CONFIG_REG_STC *userDefinedArrayPtr;
    GT_U32              userDefinedAnchor; /* user defined byte Anchor */
    GT_U32              userDefinedOffset; /* user defined byte offset from Anchor */
    GT_U8               byteValue;/* value of user defined byte from the packets or tcp/udp compare */
    GT_U32              userDefinedError = 0;/* is there user defined byte error */
    GT_STATUS           rc;/* return code from function */
    GT_U32              tmpVal; /* temp val */
    GT_U8               tmpFieldVal[4];

    fieldVal = 1;
    resrvdVal = 0;

    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, fieldVal,
                            CHT2_EPCL_KEY_FIELDS_ID_VALID_E);/* bits[0] Valid */
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, lookupConfPtr->lookupConfig.pclID ,
                            CHT2_EPCL_KEY_FIELDS_ID_PCL_ID_E);/* [10:1] ePCL-ID */
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->localDevSrcPort ,
                            CHT2_EPCL_KEY_FIELDS_ID_SRC_PORT_E);/* [16:11] src port */
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->egressTagged ,
                            CHT2_EPCL_KEY_FIELDS_ID_IS_TAGGED_E);/* [17] is tagged */
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->eVid ,
                            CHT2_EPCL_KEY_FIELDS_ID_VID_E);/* [29:18] eVid */
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->up  ,
                            CHT2_EPCL_KEY_FIELDS_ID_UP_E);/* [32:30] UP */
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->qos.qosProfile,
                            CHT2_EPCL_KEY_FIELDS_ID_QOS_PROFILE_E); /* [39:33] QOS profile */
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, (descrPtr->isIPv4==0 && descrPtr->isIp)  ,
                            CHT2_EPCL_KEY_FIELDS_ID_IS_IPV6_E); /* [40] is IPv6 */
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->isIp,
                            CHT2_EPCL_KEY_FIELDS_ID_IS_IP_E); /* [41] is IP */
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->ipProt,
                            CHT2_EPCL_KEY_FIELDS_ID_IP_PROTOCOL_E);/* [49:42] IP Protocol*/
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dscp,
                            CHT2_EPCL_KEY_FIELDS_ID_DSCP_E);/* [55:50] dscp */
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL4Valid,
                            CHT2_EPCL_KEY_FIELDS_ID_IS_L4_VALID_E);/* [56] isL4Valid*/
    if (descrPtr->l4StartOffsetPtr != NULL)
    {
        if (descrPtr->ipv6Icmp || descrPtr->ipv4Icmp)
        {
            snetCht2EPclKeyFieldBuildByPointer(
                pclKeyPtr,(GT_U8*)&(descrPtr->l4StartOffsetPtr[4]),
                    CHT2_EPCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_4_5_E);/* [72:57] L4 Byte Offsets 5:4 */
        }
        else
        {
            snetCht2EPclKeyFieldBuildByPointer(
                pclKeyPtr,(GT_U8*)&(descrPtr->l4StartOffsetPtr[2]),
                    CHT2_EPCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_2_3_E);/* [72:57] L4 Byte Offsets 3:2 */
        }
        tmpFieldVal[0] = descrPtr->l4StartOffsetPtr[0];
        tmpFieldVal[1] = descrPtr->l4StartOffsetPtr[1];
        tmpFieldVal[2] = descrPtr->l4StartOffsetPtr[13];
        snetCht2EPclKeyFieldBuildByPointer(
            pclKeyPtr,(GT_U8*)&tmpFieldVal[0],/* [96:73] L4 Byte Offsets[1:0],[13] */
                                CHT2_EPCL_KEY_FIELDS_ID_L4_BYTES_OFFSETS_0_1_13_LONG_E);
    }

    /* for IPv4 packets only the first bye sip[0] or dip[0] is relevant */
    __LOG(("for IPv4 packets only the first bye sip[0] or dip[0] is relevant"));
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[0], /* [128:97] SIP*/
                                    CHT2_EPCL_KEY_FIELDS_ID_SIP_31_0_LONG_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[0], /* [160:129] DIP*/
                                    CHT2_EPCL_KEY_FIELDS_ID_DIP_31_0_LONG_E);
    tmpVal = (descrPtr->l2Encaps == SKERNEL_ETHERNET_II_E) ? 1 : 0;
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, tmpVal,/* [161] Encap Type */
                        CHT2_EPCL_KEY_FIELDS_ID_ENCAP_TYPE_LONG_VALID_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->etherTypeOrSsapDsap,/* [177:162] */
                        CHT2_EPCL_KEY_FIELDS_ID_ETHER_TYPE_DSAP_SSAP_LONG_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpv4Fragment,/* [178] */
                        CHT2_EPCL_KEY_FIELDS_ID_LONG_IPV4_FRAGMENT_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, resrvdVal,/* [234:179] reserved */
                        CHT2_EPCL_KEY_FIELDS_ID_LONG_RESERVED_179_234_E);
    snetCht2EPclKeyFieldBuildByPointer(pclKeyPtr, descrPtr->macSaPtr,/* [282:235] MAC SA*/
                            CHT2_EPCL_KEY_FIELDS_ID_MAC_SA_LONG_E);
    snetCht2EPclKeyFieldBuildByPointer(pclKeyPtr, descrPtr->macDaPtr,/* [330:283] MAC DA*/
                            CHT2_EPCL_KEY_FIELDS_ID_MAC_DA_LONG_E);

    smemRegGet(devObjPtr,SMEM_CHT2_KEY5_0_USERDEFINED_BYTES_LONG_L3_L4_KEY_REG(devObjPtr),&userDefdata0);
    /* Key0 User Define Byte3  */
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

    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [338:331] UserDefinedByte3 */
                        CHT2_EPCL_KEY_FIELDS_ID_USER_DEFINED_BYTE3_338_331_LONG_E);

    /* Key0 User Define Byte4  */
    userDefinedAnchor = SMEM_U32_GET_FIELD(userDefdata0,userDefinedArrayPtr[4].startBitAnchor,2);
    userDefinedOffset = SMEM_U32_GET_FIELD(userDefdata0,userDefinedArrayPtr[4].startBitOffset,7);

    rc = snetChtPclUserDefinedByteGet(devObjPtr,descrPtr,userDefinedAnchor,
                                        userDefinedOffset,&byteValue);
    if(rc != GT_OK)
    {
        userDefinedError = GT_TRUE;
        byteValue = 0;
    }

    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [346:339] UserDefinedByte4 */
                        CHT2_EPCL_KEY_FIELDS_ID_USER_DEFINED_BYTE4_346_339_LONG_E);

    /* Key0 User Define Byte5  */
    userDefinedAnchor = SMEM_U32_GET_FIELD(userDefdata0,userDefinedArrayPtr[5].startBitAnchor,2);
    userDefinedOffset = SMEM_U32_GET_FIELD(userDefdata0,userDefinedArrayPtr[5].startBitOffset,7);

    rc = snetChtPclUserDefinedByteGet(devObjPtr,descrPtr,userDefinedAnchor,
                                        userDefinedOffset,&byteValue);
    if(rc != GT_OK)
    {
        userDefinedError = GT_TRUE;
        byteValue = 0;
    }

    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [354:347] UserDefinedByte5 */
                        CHT2_EPCL_KEY_FIELDS_ID_USER_DEFINED_BYTE5_354_347_LONG_E);

    /* Key0 User Define Byte0  */
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

    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [362:355] UserDefinedByte0 */
                        CHT2_EPCL_KEY_FIELDS_ID_USER_DEFINED_BYTE0_362_355_LONG_E);

    /* Key0 User Define Byte1  */
    userDefinedAnchor = SMEM_U32_GET_FIELD(userDefdata0,userDefinedArrayPtr[1].startBitAnchor,2);
    userDefinedOffset = SMEM_U32_GET_FIELD(userDefdata0,userDefinedArrayPtr[1].startBitOffset,7);

    rc = snetChtPclUserDefinedByteGet(devObjPtr,descrPtr,userDefinedAnchor,
                                        userDefinedOffset,&byteValue);
    if(rc != GT_OK)
    {
        userDefinedError = GT_TRUE;
        byteValue = 0;
    }

    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [370:363] UserDefinedByte1 */
                        CHT2_EPCL_KEY_FIELDS_ID_USER_DEFINED_BYTE1_370_363_LONG_E);
    if (userDefdata0 & (1 << 0x1F))
    {/* TCP/UDP Range Comparators is enable */
        rc = snetCht2EPclTcpUdpPortRangeCompareGet(devObjPtr,descrPtr,&byteValue);
        byteValue = (rc != GT_OK) ? 0 : byteValue ;
        snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [378:371] TCP/UDP comparator */
                            CHT2_EPCL_KEY_FIELDS_ID_TCP_UDP_PORT_COMPS_LONG_E);
    }
    else
    {   /* Key5 User Define Byte2  */
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

        snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [378:371] UserDefinedByte0 */
                            CHT2_EPCL_KEY_FIELDS_ID_USER_DEFINED_BYTE2_378_371_LONG_E);
    }

    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, resrvdVal,/* Reserved [380:379] */
                            CHT2_EPCL_KEY_FIELDS_ID_RESERVED_380_379_LONG_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, resrvdVal,/* is L2 valid [381] */
                            CHT2_EPCL_KEY_FIELDS_ID_IS_L2_VALID_LONG_E);
    tmpVal = (userDefinedError == GT_TRUE) ? 1 : 0;
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, tmpVal,/* [382] UserDefineValid */
                        CHT2_EPCL_KEY_FIELDS_ID_USER_DEFINED_VALID_LONG_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpHeaderOk ,
                            CHT2_EPCL_KEY_FIELDS_ID_IP_HEADER_OK_LONG_E);/* [383] ip header ok */

    return ;
}

/*******************************************************************************
*   snetCht2EPclBuildL3L4StandardKey
*
* DESCRIPTION:
*        Build IPv4+Layer4 standard epcl search key.
*
* INPUTS:
*       devObjPtr            - pointer to device object.
*       descrPtr             - pointer to frame data buffer Id.
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
static GT_VOID snetCht2EPclBuildL3L4StandardKey
(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr     ,
    IN SNET_CHT2_EPCL_LOOKUP_CONFIG_STC * lookupConfPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC    * pclExtraDataPtr  ,
    OUT SNET_CHT_POLICY_KEY_STC         * pclKeyPtr
)
{
    DECLARE_FUNC_NAME(snetCht2EPclBuildL3L4StandardKey);

    GT_U32              fieldVal;   /* value for valid bit */
    GT_U32              resrvdVal;   /* reserved value */
    GT_U8               byteValue;    /* value of user defined byte from
                                         the packets or tcp/udp compare */
    GT_U32              tmpVal;  /* temp val */
    GT_U8               tmpFieldVal[4];
    GT_STATUS           rc; /* return code */

    fieldVal = 1;
    resrvdVal = 0;
    byteValue = 0;

    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, fieldVal, /* bits[0] Valid */
                                        CHT2_EPCL_KEY_FIELDS_ID_VALID_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, lookupConfPtr->lookupConfig.pclID ,
                                        CHT2_EPCL_KEY_FIELDS_ID_PCL_ID_E);/* [10:1] PCL-ID */
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->localDevSrcPort ,
                                        CHT2_EPCL_KEY_FIELDS_ID_SRC_PORT_E);/* [16:11] src port */
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->egressTagged ,/* [17] is tagged */
                                        CHT2_EPCL_KEY_FIELDS_ID_IS_TAGGED_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->eVid ,/* [29:18] eVid */
                                        CHT2_EPCL_KEY_FIELDS_ID_VID_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->up  ,/* [32:30] UP */
                                        CHT2_EPCL_KEY_FIELDS_ID_UP_E);

    /* filling bits [39:33] depends on the marvell tag command */
    __LOG(("filling bits [39:33] depends on the marvell tag command"));
    switch (descrPtr->outGoingMtagCmd)
    {
    case SKERNEL_MTAG_CMD_FORWARD_E :
        snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->qos.qosProfile,
                                        CHT2_EPCL_KEY_FIELDS_ID_QOS_PROFILE_E);
        break;
    case SKERNEL_MTAG_CMD_TO_CPU_E:
        snetCht2EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->cpuCode,/* [39:33] CPU_CODE[6:0]*/
                                CHT2_EPCL_KEY_FIELDS_ID_CPU_CODE_0_6_STANDARD_E);
        break;
    case SKERNEL_MTAG_CMD_FROM_CPU_E:
        tmpVal = 0x0;
        tmpVal = SMEM_U32_GET_FIELD(descrPtr->qos.fromCpuQos.fromCpuDp ,0,2);
        tmpVal |= (SMEM_U32_GET_FIELD(descrPtr->qos.fromCpuQos.fromCpuTc ,0,3) << 2);
        snetCht2EPclKeyFieldBuildByValue(pclKeyPtr,tmpVal,/* [39:33] 2'b0,TC[2:0],DP[1:0] */
                                        CHT2_EPCL_KEY_FIELDS_ID_QOS_PROFILE_E);
        break;
    default :
        snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, resrvdVal ,/* [39:33] reserved  */
                                        CHT2_EPCL_KEY_FIELDS_ID_RESERVED_39_33_E);
        break;
    }

    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->isIPv4 , /* [40] is IPv4 */
                                        CHT2_EPCL_KEY_FIELDS_ID_IS_IPV4_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->isIp,/* [41] is IP */
                                        CHT2_EPCL_KEY_FIELDS_ID_IS_IP_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->ipProt,/* [49:42] IP Protocol*/
                                        CHT2_EPCL_KEY_FIELDS_ID_IP_PROTOCOL_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dscp,/* [55:50] dscp */
                                        CHT2_EPCL_KEY_FIELDS_ID_DSCP_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL4Valid, /* [56] isL4Valid*/
                                        CHT2_EPCL_KEY_FIELDS_ID_IS_L4_VALID_E);
    if (descrPtr->l4StartOffsetPtr != NULL)
    {
        if (descrPtr->ipv6Icmp || descrPtr->ipv4Icmp)
        {
            snetCht2EPclKeyFieldBuildByPointer(
                pclKeyPtr,(GT_U8*)&(descrPtr->l4StartOffsetPtr[4]),
                    CHT2_EPCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_4_5_E);/* [72:57] L4 Byte Offsets 5:4 */
        }
        else
        {
            snetCht2EPclKeyFieldBuildByPointer(
                pclKeyPtr,(GT_U8*)&(descrPtr->l4StartOffsetPtr[2]),
                    CHT2_EPCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_2_3_E);/* [72:57] L4 Byte Offsets 3:2 */
        }
    }

    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->arp ,/* [73] isArp */
                                    CHT2_EPCL_KEY_FIELDS_ID_IS_ARP_E);
    tmpVal = (descrPtr->macDaType == SKERNEL_BROADCAST_MAC_E ||
              descrPtr->macDaType == SKERNEL_BROADCAST_ARP_E) ? 1 : 0 ;
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr,  tmpVal,/* [74] IS bc   */
                                    CHT2_EPCL_KEY_FIELDS_ID_IS_BC_E);

    if (descrPtr->origIsTrunk)
    {
        snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->origSrcEPortOrTrnk ,
                                        CHT2_EPCL_KEY_FIELDS_ID_TRUNK_ID);/* [81:75] source trunk id */
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
        if (descrPtr->outGoingMtagCmd == SKERNEL_MTAG_CMD_TO_TRG_SNIFFER_E)
        {
            byteValue |= ( descrPtr->rxSniff & 0x1 ) << 0x5;
        }
        /* [80] FROM_CPU : Desc<EgressFilterEn> */
        if (descrPtr->outGoingMtagCmd == SKERNEL_MTAG_CMD_FROM_CPU_E)
        {
            byteValue |= ( descrPtr->egressFilterEn & 0x1 ) << 0x4;
        }
        /* [79:75] source device */
        byteValue |= (descrPtr->srcDev & 0x1F) ;
        snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, byteValue ,
                                            CHT2_EPCL_KEY_FIELDS_ID_SRCDEV_ID);
    }

    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->origIsTrunk ,/* [82] source is trunk  */
                            CHT2_EPCL_KEY_FIELDS_ID_SRC_IS_TRUNK_SHORT_E);

    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sstId ,/* [87:83] Source-ID */
                                        CHT2_EPCL_KEY_FIELDS_ID_SST_ID_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->routed ,/* [88] isRouted  */
                                        CHT2_EPCL_KEY_FIELDS_ID_IS_ROUTED);

    switch (descrPtr->outGoingMtagCmd)
    {
        case SKERNEL_MTAG_CMD_TO_CPU_E: /* [89] CPU_CODE[7]  */
            tmpVal = SMEM_U32_GET_FIELD(descrPtr->cpuCode,7,1);
            snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, tmpVal ,
                                CHT2_EPCL_KEY_FIELDS_ID_CPU_CODE_7_STANDARD_E);
            break;
        case SKERNEL_MTAG_CMD_FORWARD_E : /*  isUnknown [89] */
            snetCht2EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->macDaFound ,
                                        CHT2_EPCL_KEY_FIELDS_ID_IS_UNKNOWN_UC_E);
            break;
        default : /* reserved [89] */
            snetCht2EPclKeyFieldBuildByValue(pclKeyPtr,resrvdVal ,
                                        CHT2_EPCL_KEY_FIELDS_ID_RESERVED_89_E );
    }

    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL2Valid ,/* [90] isL2Valid  */
                            CHT2_EPCL_KEY_FIELDS_ID_IS_L2_VALID_SHORT_E);
    rc = snetCht2EPclTcpUdpPortRangeCompareGet(devObjPtr,descrPtr,&byteValue);
    byteValue = (rc != GT_OK) ? 0 : byteValue ;
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [98:91] TCP/UDP comparator */
                             CHT2_EPCL_KEY_FIELDS_ID_TCP_UDP_PORT_COMPS_SHORT_E);

    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[3], /* [130:99] SIP*/
                             CHT2_EPCL_KEY_FIELDS_ID_SIP_31_0_SHORT_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[3], /* [162:131] DIP*/
                             CHT2_EPCL_KEY_FIELDS_ID_DIP_31_0_IPV4_L4_SHORT_E);
    if (descrPtr->l4StartOffsetPtr != NULL)
    {
        tmpFieldVal[0] = descrPtr->l4StartOffsetPtr[0];
        tmpFieldVal[1] = descrPtr->l4StartOffsetPtr[1];
        tmpFieldVal[2] = descrPtr->l4StartOffsetPtr[13];
        snetCht2EPclKeyFieldBuildByPointer(
            pclKeyPtr,(GT_U8*)&tmpFieldVal[0],/* [186:163] L4 Byte Offsets[1:0],[13] */
                    CHT2_EPCL_KEY_FIELDS_ID_L4_BYTES_OFFSETS_0_1_13_SHORT_E);
    }
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, resrvdVal, /* [187] Reserved */
                                        CHT2_EPCL_KEY_FIELDS_ID_RESERVED_187_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, (GT_U32)descrPtr->macDaType , /* [189:188] packet type */
                                        CHT2_EPCL_KEY_FIELDS_ID_PACKET_TYPE_SHORT_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpv4Fragment , /* [190] ipv4 fragment */
                            CHT2_EPCL_KEY_FIELDS_ID_IPV4_FRAGMENTED_SHORT_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->useVidx ,/* [191] isVidx  */
                            CHT2_EPCL_KEY_FIELDS_ID_IS_VIDX_SHORT_E);

    return  ;
}

/*******************************************************************************
*   snetCht2EPclBuildL2L3StandardKey
*
* DESCRIPTION:
*        Build Layer2+IPv4\6+QoS standard epcl search key.
*
* INPUTS:
*       devObjPtr            - pointer to device object.
*       descrPtr             - pointer to frame data buffer Id.
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
static GT_VOID snetCht2EPclBuildL2L3StandardKey
(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr      ,
    IN SNET_CHT2_EPCL_LOOKUP_CONFIG_STC * lookupConfPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC    * pclExtraDataPtr  ,
    OUT SNET_CHT_POLICY_KEY_STC         * pclKeyPtr
)
{
    GT_U32          fieldVal;
    GT_U32          resrvdVal;
    GT_U8           byteValue;
    GT_U8           tmpFieldVal[4];
    GT_U32          tmpVal;
    GT_STATUS       rc;

    fieldVal = 1;
    resrvdVal = 0;

    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, fieldVal, /* bits[0] Valid */
                            CHT2_EPCL_KEY_FIELDS_ID_VALID_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, lookupConfPtr->lookupConfig.pclID ,
                            CHT2_EPCL_KEY_FIELDS_ID_PCL_ID_E);/* [10:1] ePCL-ID */
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->localDevSrcPort ,
                            CHT2_EPCL_KEY_FIELDS_ID_SRC_PORT_E);/* [16:11] src port */
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->egressTagged ,/* [17] is tagged */
                            CHT2_EPCL_KEY_FIELDS_ID_IS_TAGGED_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->eVid ,/* [29:18] eVid */
                            CHT2_EPCL_KEY_FIELDS_ID_VID_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->up  ,/* [32:30] UP */
                            CHT2_EPCL_KEY_FIELDS_ID_UP_E);

    switch (descrPtr->outGoingMtagCmd)
    {
        case SKERNEL_MTAG_CMD_FORWARD_E : /* [39:33] QOS profile */
            snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->qos.qosProfile,
                                        CHT2_EPCL_KEY_FIELDS_ID_QOS_PROFILE_E);
        break;
        case SKERNEL_MTAG_CMD_TO_CPU_E: /* [39:33] CPU_CODE[6:0]*/
            snetCht2EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->cpuCode,
                            CHT2_EPCL_KEY_FIELDS_ID_CPU_CODE_0_6_STANDARD_E);
        break;
        case SKERNEL_MTAG_CMD_FROM_CPU_E: /* [39:33] 2'b0,TC[2:0],DP[1:0] */
            tmpVal = 0x0;
            tmpVal = SMEM_U32_GET_FIELD(descrPtr->qos.fromCpuQos.fromCpuDp ,0,2);
            tmpVal |= (SMEM_U32_GET_FIELD(descrPtr->qos.fromCpuQos.fromCpuTc ,0,3) << 2);
            snetCht2EPclKeyFieldBuildByValue(pclKeyPtr,tmpVal,
                                CHT2_EPCL_KEY_FIELDS_ID_QOS_PROFILE_E);
        break;
        default : /* [39:33] reserved  */
            snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, resrvdVal ,
                                    CHT2_EPCL_KEY_FIELDS_ID_RESERVED_39_33_E);
    }

    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->isIPv4 , /* [40] is IPv4 */
                                            CHT2_EPCL_KEY_FIELDS_ID_IS_IPV4_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->isIp,/* [41] is IP */
                                            CHT2_EPCL_KEY_FIELDS_ID_IS_IP_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->ipProt,/* [49:42] IP Protocol*/
                                        CHT2_EPCL_KEY_FIELDS_ID_IP_PROTOCOL_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dscp,/* [55:50] dscp */
                                        CHT2_EPCL_KEY_FIELDS_ID_DSCP_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL4Valid, /* [56] isL4Valid*/
                                        CHT2_EPCL_KEY_FIELDS_ID_IS_L4_VALID_E);

    if (descrPtr->l4StartOffsetPtr != NULL)
    {
        if (descrPtr->ipv6Icmp || descrPtr->ipv4Icmp)
        {
            snetCht2EPclKeyFieldBuildByPointer(/* [72:57] L4 Byte Offsets 5:4 */
                pclKeyPtr,(GT_U8*)&(descrPtr->l4StartOffsetPtr[4]),
                                CHT2_EPCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_4_5_E);
        }
        else
        {
            snetCht2EPclKeyFieldBuildByPointer(/* [72:57] L4 Byte Offsets 3:2 */
                pclKeyPtr,(GT_U8*)&(descrPtr->l4StartOffsetPtr[2]),
                                CHT2_EPCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_2_3_E);
        }
    }

    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->arp ,/* [73] isArp */
                                        CHT2_EPCL_KEY_FIELDS_ID_IS_ARP_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, resrvdVal ,/* [74] reserved  */
                                        CHT2_EPCL_KEY_FIELDS_ID_RESERVED_74_E);
    if (descrPtr->origIsTrunk)
    {
        snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->origSrcEPortOrTrnk ,
                CHT2_EPCL_KEY_FIELDS_ID_TRUNK_ID);/* [81:75] source trunk id */
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
        if (descrPtr->outGoingMtagCmd == SKERNEL_MTAG_CMD_TO_TRG_SNIFFER_E)
        {
                byteValue |= ( descrPtr->rxSniff & 0x1 ) << 0x5;
        }
        /* [80] FROM_CPU : Desc<EgressFilterEn> */
        if (descrPtr->outGoingMtagCmd == SKERNEL_MTAG_CMD_FROM_CPU_E)
        {
                byteValue |= ( descrPtr->egressFilterEn & 0x1 ) << 0x4;
        }
        /* [79:75] source trunk id */
        snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->srcDev ,
                                            CHT2_EPCL_KEY_FIELDS_ID_SRCDEV_ID);
    }

    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->origIsTrunk ,/* [82] source is trunk  */
                            CHT2_EPCL_KEY_FIELDS_ID_SRC_IS_TRUNK_SHORT_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sstId ,/* [87:83] Source-ID */
                            CHT2_EPCL_KEY_FIELDS_ID_SST_ID_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->routed ,/* [88] isRouted  */
                            CHT2_EPCL_KEY_FIELDS_ID_IS_ROUTED);

    if (descrPtr->outGoingMtagCmd == SKERNEL_MTAG_CMD_TO_CPU_E)
    {
        tmpVal = SMEM_U32_GET_FIELD(descrPtr->cpuCode,7,1);
        snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, tmpVal ,/* [89] CPU_CODE[7]  */
                        CHT2_EPCL_KEY_FIELDS_ID_CPU_CODE_7_STANDARD_E);
    }
    else if (descrPtr->outGoingMtagCmd == SKERNEL_MTAG_CMD_FORWARD_E)
    {/*  isUnknown [89] */
        snetCht2EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->macDaFound ,
                                        CHT2_EPCL_KEY_FIELDS_ID_IS_UNKNOWN_UC_E);
    }
    else
    {   /* reserved [89] */
        snetCht2EPclKeyFieldBuildByValue(pclKeyPtr,resrvdVal ,
                                CHT2_EPCL_KEY_FIELDS_ID_RESERVED_89_E);
    }

    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL2Valid ,/* [90] isL2Valid  */
                            CHT2_EPCL_KEY_FIELDS_ID_IS_L2_VALID_SHORT_E);
    rc = snetCht2EPclTcpUdpPortRangeCompareGet(devObjPtr,descrPtr,&byteValue);
    byteValue = (rc != GT_OK) ? 0 : byteValue ;
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [98:91] TCP/UDP comparator */
                             CHT2_EPCL_KEY_FIELDS_ID_TCP_UDP_PORT_COMPS_SHORT_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[0], /* [130:99] DIP[31:0] */
                                        CHT2_EPCL_KEY_FIELDS_ID_DIP_31_0_E);
    if (descrPtr->l4StartOffsetPtr != NULL)
    {
        tmpFieldVal[0] = descrPtr->l4StartOffsetPtr[13];
        snetCht2EPclKeyFieldBuildByPointer(
            pclKeyPtr,(GT_U8*)&tmpFieldVal[0],/* [138:131] L4 Byte Offsets[13] */
                        CHT2_EPCL_KEY_FIELDS_ID_L4_BYTES_OFFSETS_13_SHORT_E);
    }

    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, resrvdVal ,/* [139] reserved  */
                            CHT2_EPCL_KEY_FIELDS_ID_RESERVED_139_SHORT_E);
    snetCht2EPclKeyFieldBuildByPointer(pclKeyPtr, descrPtr->macDaPtr,/* [187:140] MAC DA*/
                            CHT2_EPCL_KEY_FIELDS_ID_MAC_DA_SHORT_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, (GT_U32)descrPtr->macDaType , /* [189:188] packet type */
                            CHT2_EPCL_KEY_FIELDS_ID_PACKET_TYPE_SHORT_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, (GT_U32)pclExtraDataPtr->isIpv4Fragment , /* [190] ipv4 fragment */
                            CHT2_EPCL_KEY_FIELDS_ID_IPV4_FRAGMENTED_SHORT_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->useVidx ,/* [191] isVidx  */
                                    CHT2_EPCL_KEY_FIELDS_ID_IS_VIDX_SHORT_E);

    return ;
}

/*******************************************************************************
*   snetCht2EPclBuildL2StandardKey
*
* DESCRIPTION:
*        Build layer2 standard epcl search key.
*
* INPUTS:
*       devObjPtr            - pointer to device object.
*       descrPtr             - pointer to frame data buffer Id.
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
static GT_VOID snetCht2EPclBuildL2StandardKey
(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr      ,
    IN SNET_CHT2_EPCL_LOOKUP_CONFIG_STC * lookupConfPtr   ,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC    * pclExtraDataPtr  ,
    OUT SNET_CHT_POLICY_KEY_STC         * pclKeyPtr
)
{
    GT_U32  fieldVal = 1;
    GT_U32  resrvdVal = 0;
    GT_U32  tmpVal;
    GT_U32  byteVal = 0 ;

    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, fieldVal, /* bits[0] Valid */
                                        CHT2_EPCL_KEY_FIELDS_ID_VALID_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, lookupConfPtr->lookupConfig.pclID ,
                                        CHT2_EPCL_KEY_FIELDS_ID_PCL_ID_E);/*[10:1] PCL-ID */
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->localDevSrcPort ,
                                        CHT2_EPCL_KEY_FIELDS_ID_SRC_PORT_E);/* [16:11] src port */
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->egressTagged ,/* [17] is tagged */
                                        CHT2_EPCL_KEY_FIELDS_ID_IS_TAGGED_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->eVid ,/* [29:18] eVid */
                                        CHT2_EPCL_KEY_FIELDS_ID_VID_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->up  ,/* [32:30] UP */
                                        CHT2_EPCL_KEY_FIELDS_ID_UP_E);

    switch (descrPtr->outGoingMtagCmd)
    {
        case SKERNEL_MTAG_CMD_FORWARD_E:/* [39:33] QOS profile */
            snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->qos.qosProfile,
                                        CHT2_EPCL_KEY_FIELDS_ID_QOS_PROFILE_E);
        break;
        case SKERNEL_MTAG_CMD_TO_CPU_E: /* [39:33] CPU_CODE[6:0]*/
            tmpVal = SMEM_U32_GET_FIELD(descrPtr->cpuCode,0,6);
            snetCht2EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->cpuCode,
                                CHT2_EPCL_KEY_FIELDS_ID_CPU_CODE_0_6_STANDARD_E);
        break;
        case SKERNEL_MTAG_CMD_FROM_CPU_E:/* [39:33] 2'b0,TC[2:0],DP[1:0] */
            tmpVal = 0;
            tmpVal = SMEM_U32_GET_FIELD(descrPtr->qos.fromCpuQos.fromCpuDp ,0,2);
            tmpVal |= (SMEM_U32_GET_FIELD(descrPtr->qos.fromCpuQos.fromCpuTc ,0,3) << 2);
            snetCht2EPclKeyFieldBuildByValue(pclKeyPtr,tmpVal,
                                         CHT2_EPCL_KEY_FIELDS_ID_QOS_PROFILE_E);
        break;
        default :/* [39:33] reserved  */
            snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, resrvdVal ,
                                    CHT2_EPCL_KEY_FIELDS_ID_RESERVED_39_33_E);
    }

    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->isIPv4 ,/* [40] is IPv4 */
                                        CHT2_EPCL_KEY_FIELDS_ID_IS_IPV4_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->isIp,/* [41] is IP */
                                        CHT2_EPCL_KEY_FIELDS_ID_IS_IP_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, resrvdVal,/*[42] reserved field*/
                                        CHT2_EPCL_KEY_FIELDS_ID_RESERVED_42_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->etherTypeOrSsapDsap,/* [58:43] */
                            CHT2_EPCL_KEY_FIELDS_ID_ETHER_TYPE_DSAP_SSAP_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, resrvdVal,/* [66:59] reserved */
                            CHT2_EPCL_KEY_FIELDS_ID_RESERVED_66_59_SHORT_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, resrvdVal,/* [72:67] reserved  */
                                    CHT2_EPCL_KEY_FIELDS_ID_RESERVED_72_67_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->arp ,/* [73] isArp */
                                        CHT2_EPCL_KEY_FIELDS_ID_IS_ARP_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, resrvdVal ,/* [74] reserved  */
                                        CHT2_EPCL_KEY_FIELDS_ID_RESERVED_74_E);

    if (descrPtr->origIsTrunk)
    {/* [81:75] source trunk id */
        snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->origSrcEPortOrTrnk ,
                                            CHT2_EPCL_KEY_FIELDS_ID_TRUNK_ID);
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
        if (descrPtr->outGoingMtagCmd == SKERNEL_MTAG_CMD_TO_TRG_SNIFFER_E)
        {
                byteVal |= ( descrPtr->rxSniff & 0x1 ) << 0x5;
        }
        /* [80] FROM_CPU : Desc<EgressFilterEn> */
        if (descrPtr->outGoingMtagCmd == SKERNEL_MTAG_CMD_FROM_CPU_E)
        {
                byteVal |= ( descrPtr->egressFilterEn & 0x1 ) << 0x4;
        }
        /* [79:75] source device */
        byteVal |= SMEM_U32_GET_FIELD(descrPtr->srcDev , 0x0 , 0x5);
        snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, byteVal ,
                                        CHT2_EPCL_KEY_FIELDS_ID_SRCDEV_ID);
    }

    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->origIsTrunk ,/* [82] source is trunk  */
                            CHT2_EPCL_KEY_FIELDS_ID_SRC_IS_TRUNK_SHORT_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sstId ,/* [87:83] Source-ID */
                                        CHT2_EPCL_KEY_FIELDS_ID_SST_ID_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->routed ,/* [88] isRouted  */
                                        CHT2_EPCL_KEY_FIELDS_ID_IS_ROUTED);
    switch (descrPtr->outGoingMtagCmd)
    {
        case SKERNEL_MTAG_CMD_TO_CPU_E : /* [89] CPU_CODE[7]  */
            tmpVal = SMEM_U32_GET_FIELD(descrPtr->cpuCode,7,1);
            snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, tmpVal ,
                            CHT2_EPCL_KEY_FIELDS_ID_CPU_CODE_7_STANDARD_E);
        break;
        case SKERNEL_MTAG_CMD_FORWARD_E :     /*  isUnknown [89] */
            snetCht2EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->macDaFound ,
                                       CHT2_EPCL_KEY_FIELDS_ID_IS_UNKNOWN_UC_E);
        break;
        default :        /* reserved [89] */
            snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, resrvdVal ,
                                        CHT2_EPCL_KEY_FIELDS_ID_RESERVED_89_E );
    }

    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL2Valid ,/* [90] isL2Valid  */
                                CHT2_EPCL_KEY_FIELDS_ID_IS_L2_VALID_SHORT_E);
    tmpVal = (descrPtr->l2Encaps == SKERNEL_ETHERNET_II_E) ? 1 : 0;
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, tmpVal,/* [91] Encap Type */
                                CHT2_EPCL_KEY_FIELDS_ID_ENCAP_TYPE_SHORT_E);
    snetCht2EPclKeyFieldBuildByPointer(pclKeyPtr, descrPtr->macSaPtr,/* [139:92] MAC SA*/
                                CHT2_EPCL_KEY_FIELDS_ID_MAC_SA_SHORT_E);
    snetCht2EPclKeyFieldBuildByPointer(pclKeyPtr, descrPtr->macDaPtr,/* [187:140] MAC DA*/
                                CHT2_EPCL_KEY_FIELDS_ID_MAC_DA_SHORT_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, (GT_U32)descrPtr->macDaType , /* [189:188] packet type */
                                CHT2_EPCL_KEY_FIELDS_ID_PACKET_TYPE_SHORT_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, resrvdVal ,/* [190] Reserved  */
                                CHT2_EPCL_KEY_FIELDS_ID_IS_L2_VALID_SHORT_E);
    snetCht2EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->useVidx ,/* [191] isVidx  */
                                CHT2_EPCL_KEY_FIELDS_ID_IS_VIDX_SHORT_E);

    return ;
}

/*******************************************************************************
* snetCht2EPclConfigPtrGet
*
* DESCRIPTION:
*       Get data from the egress PCL configuration table.
*
* INPUTS:
*       devObjPtr     - pointer to device object.
*       descrPtr      - pointer to frame descriptor object.
*       port          - egress port number
*
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
static GT_VOID snetCht2EPclConfigPtrGet
(
    IN SKERNEL_DEVICE_OBJECT                * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC      * descrPtr ,
    IN GT_U32                                 port,
    OUT SNET_CHT2_EPCL_LOOKUP_CONFIG_STC    * lookupConfPtr
)
{
    DECLARE_FUNC_NAME(snetCht2EPclConfigPtrGet);

    GT_U32  regAddrEPclGlobTbl;
    GT_U32  maxPortNo;
    GT_U32  index;
    GT_U32  pclCfgMode;
    GT_U32  egressPclCfgMode;
    GT_U32  regAddr;
    GT_U32  portIfPCLBlock;
    GT_U32  epclCfgEntry;

    /* Egress Policy Configuration Table Access mode configuration */
    regAddr = SMEM_CHT2_EPCL_TBL_ACCESS_MODE_CONFIG_REG(devObjPtr);
    smemRegFldGet(devObjPtr, regAddr, 1, 1, &pclCfgMode);
    if (pclCfgMode == 1)
    {
        index = descrPtr->eVid; /* Access ePCL-id config. table with vlan id  */
    }
    else
    {
        regAddrEPclGlobTbl = SMEM_CHT2_EPCL_GLOBAL_REG(devObjPtr);
        smemRegFldGet(devObjPtr, regAddrEPclGlobTbl, 1, 1, &egressPclCfgMode);
        if (egressPclCfgMode == 0)
        {/* EPCL configuration table index = Local Device Source Port# + 4K */
            index = (1 << 12) | port;
        }
        else
        {
            /* read the max port per device and the ePCL interface block */
            __LOG(("read the max port per device and the ePCL interface block"));
            smemRegFldGet(devObjPtr, regAddrEPclGlobTbl, 18, 1, &maxPortNo);
            smemRegFldGet(devObjPtr, regAddrEPclGlobTbl ,16, 2, &portIfPCLBlock);
            if (maxPortNo == 0)
            {   /* Max port per device = 0 , up to 1K ports in system */
                index = ((portIfPCLBlock & 0x3)<< 10) |
                        ((descrPtr->trgDev & 0x1f)<< 5) |
                        ((descrPtr->trgEPort & 0x1f));
            }
            else
            {   /* Max port per device = 1 , up to 2K ports in system */
                index = ((portIfPCLBlock & 0x1)<< 11) |
                        ((descrPtr->trgDev & 0x1f)<< 6) |
                        ((descrPtr->trgEPort & 0x3f));
            }
        }
    }

    /* calculate the address of the EPCL configuration table */
    __LOG(("calculate the address of the EPCL configuration table"));
    smemRegGet(devObjPtr,SMEM_CHT2_EPCL_CONFIG_TBL_MEM(index),&epclCfgEntry);

    /* The Egress PCL configuration Table
        enables the following configurations
        for the Egress policy TCAM lookup         */
    lookupConfPtr->lookupConfig.enabled = SMEM_U32_GET_FIELD(epclCfgEntry,0,1);
    lookupConfPtr->lookupConfig.nonIpKeyType = SMEM_U32_GET_FIELD(epclCfgEntry,1,1);
    lookupConfPtr->lookupConfig.ipv4ArpKeyType = SMEM_U32_GET_FIELD(epclCfgEntry,12,2);
    lookupConfPtr->lookupConfig.ipv6KeyType = SMEM_U32_GET_FIELD(epclCfgEntry,14,2);
    lookupConfPtr->lookupConfig.pclID = SMEM_U32_GET_FIELD(epclCfgEntry,2,10);

    return ;
}


/*******************************************************************************
* snetCht2EPclGetKeyType
*
* DESCRIPTION:
*       Get the key type from the EPCL configuration table.
*
* INPUTS:
*       descrPtr       - pointer to frame descriptor object.
*       lookupConfPtr  - pointer to the EPCL configuration table.
*
* OUTPUTS:
*       keyTypePtr     - pointer to the relevant key type.
*
* RETURN:
*
* COMMENTS:
*       C.12.17 Egress PCL Configuration Table
*                 The PCL table is a 4160 line table . each entry is 32-bits.
*
*******************************************************************************/
static GT_VOID snetCht2EPclGetKeyType
(
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr  ,
    IN SNET_CHT2_EPCL_LOOKUP_CONFIG_STC * lookupConfPtr,
    OUT CHT2_EPCL_KEY_TYPE_ENT          * keyTypePtr
)
{
    *keyTypePtr = CHT2_EPCL_KEY_TYPE_UNKNOWN_E;

    if (descrPtr->isIp == 0) /* bridge packet */
    {
        if (lookupConfPtr->lookupConfig.nonIpKeyType == 0)
        {
            *keyTypePtr = CHT2_EPCL_KEY_TYPE_L2_KEY_SHORT_E;
        }
        else
        {
            *keyTypePtr = CHT2_EPCL_KEY_TYPE_L2_L3_KEY_SHORT_E;
        }
    }
    else if (descrPtr->isIPv4 == 1) /* ipv4 packet */
    {
        if (lookupConfPtr->lookupConfig.ipv4ArpKeyType == 0)
        {
            *keyTypePtr = CHT2_EPCL_KEY_TYPE_L2_L3_KEY_SHORT_E;
        }
        else if (lookupConfPtr->lookupConfig.ipv4ArpKeyType == 1)
        {
            *keyTypePtr = CHT2_EPCL_KEY_TYPE_L3_L4_KEY_SHORT_E;
        }
        else if (lookupConfPtr->lookupConfig.ipv4ArpKeyType == 2)
        {
            *keyTypePtr = CHT2_EPCL_KEY_TYPE_L2_L3_L4_KEY_LONG_E;
        }
    }
    else /* ipv6 packet */
    {
        if (lookupConfPtr->lookupConfig.ipv6KeyType == 0)
        {
            *keyTypePtr = CHT2_EPCL_KEY_TYPE_L2_L3_KEY_SHORT_E;
        }
        else if (lookupConfPtr->lookupConfig.ipv6KeyType == 2)
        {
            *keyTypePtr = CHT2_EPCL_KEY_TYPE_L2_IPV6_KEY_LONG_E;
        }
        else if (lookupConfPtr->lookupConfig.ipv6KeyType == 3)
        {
            *keyTypePtr = CHT2_EPCL_KEY_TYPE_L4_IPV6_KEY_LONG_E;
        }
    }

    lookupConfPtr->lookupConfig.keySize = (*keyTypePtr <= CHT2_EPCL_KEY_TYPE_L3_L4_KEY_SHORT_E) ?
                                        SNET_CHT_PCL_STANDARD_KEY :
                                        SNET_CHT_PCL_EXTENDED_KEY ;

    return ;
}

/*******************************************************************************
* snetCht2EPclActionApply
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
GT_VOID snetCht2EPclActionApply
(
    IN SKERNEL_DEVICE_OBJECT                * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC   * descrPtr  ,
    IN SNET_CHT2_EPCL_ACTION_STC            * actionDataPtr
)
{
    DECLARE_FUNC_NAME(snetCht2EPclActionApply);

    GT_U32  address;
    GT_U32  regValue;

    /* For Ch3 matching counter update is done as part of the CNC code */
    /* and not here */
    if (SKERNEL_IS_CHEETAH3_DEV(devObjPtr) == 0)
    {
        /* update the matching counter if enabled */
        __LOG(("update the matching counter if enabled"));
        if (actionDataPtr->matchCounterEn)
        {
            address = SMEM_CHT2_PCL_RULE_MATCH_REG(devObjPtr) +
                        (actionDataPtr->matchCounterIndex * 0x4);
            smemRegGet(devObjPtr, address, &regValue);
            ++regValue;
            smemRegSet(devObjPtr, address, regValue);
        }
    }

    /* Remarking the QOS profile .                                       */
    /* Enables the modification of the packet 802.1q User Priority field */
    descrPtr->modifyUp = actionDataPtr->pceModifyUp;
    if (actionDataPtr->pceModifyUp == EPCL_ACTION_MODIFY_OUTER_TAG_E) /* 0 - no modification */
    {                                    /* 2 ,3 are reserved values */
       /*ModifyOuterTag : EPCL Action entry <Tag0 VID> is assigned to the outer tag of the packet.
          If the packet is tunneled, this refers to the tunnel header tag.*/

       descrPtr->up = actionDataPtr->epceUp;
    }

    /* Enables the modification of the packet 802.1q DSCP field */
    if (actionDataPtr->pceModifyDscp == 1) /* 0 - no modification */
    {                                      /* 2 ,3 are reserved values */
       descrPtr->dscp = actionDataPtr->epceDscp;
       descrPtr->modifyDscp = 1;
    }

    descrPtr->epclAction.modifyUp = actionDataPtr->pceModifyUp;
    descrPtr->epclAction.modifyDscp = actionDataPtr->pceModifyDscp;
    descrPtr->epclAction.up = actionDataPtr->epceUp;
    descrPtr->epclAction.dscp = actionDataPtr->epceDscp;
    descrPtr->epclAction.drop = actionDataPtr->fwdCmd;
    descrPtr->epclAction.modifyVid = actionDataPtr->epceVlanCmd;
    descrPtr->epclAction.vid = actionDataPtr->epceVid;

    if(devObjPtr->ePclKeyFormatVersion >= 2)
    {
        /*Tag1 VLAN Command*/
        descrPtr->epclAction.vlan1Cmd = actionDataPtr->vlan1Cmd;
        /*Enable Modify UP1*/
        descrPtr->epclAction.modifyUp1 = actionDataPtr->modifyUp1;
        /*Tag1 VID*/
        descrPtr->epclAction.vid1 = actionDataPtr->vid1;
        /*UP1*/
        descrPtr->epclAction.up1 =  actionDataPtr->up1;
    }



    return ;
}

/*******************************************************************************
* snetCht2EPclActionGet
*
* DESCRIPTION:
*        Get the action entry from the policy action table.
*        The function used for CH3 and above.
*        For CH2  used another function due to FEr#1087.
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
GT_VOID snetCht2EPclActionGet
(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr  ,
    IN GT_U32                             matchIndex,
    OUT SNET_CHT2_EPCL_ACTION_STC       * actionDataPtr
)
{
    DECLARE_FUNC_NAME(snetCht2EPclActionGet);

    GT_U32  tblAddr;
    GT_U32  * actionEntryDataPtr;

    tblAddr = SMEM_CHT_PCL_ACTION_TBL_MEM(devObjPtr, matchIndex) ;
    actionEntryDataPtr = smemMemGet(devObjPtr,tblAddr);

    /* Read word 0 from the action table entry */
    __LOG(("Read word 0 from the action table entry"));
    actionDataPtr->matchCounterIndex = SMEM_U32_GET_FIELD(actionEntryDataPtr[0],26,6);
    actionDataPtr->matchCounterEn = SMEM_U32_GET_FIELD(actionEntryDataPtr[0],25,1);
    actionDataPtr->pceModifyUp = SMEM_U32_GET_FIELD(actionEntryDataPtr[0],23,2);
    actionDataPtr->pceModifyDscp = SMEM_U32_GET_FIELD(actionEntryDataPtr[0],21,2);
    actionDataPtr->epceUp = SMEM_U32_GET_FIELD(actionEntryDataPtr[0],18,3);
    actionDataPtr->epceDscp = SMEM_U32_GET_FIELD(actionEntryDataPtr[0],12,6);
    actionDataPtr->fwdCmd = SMEM_U32_GET_FIELD(actionEntryDataPtr[0],0,3);

    /* Read word 1 from the action table entry */
    __LOG(("Read word 1 from the action table entry"));
    actionDataPtr->epceVid = SMEM_U32_GET_FIELD(actionEntryDataPtr[1],19,12);
    actionDataPtr->epceVlanCmd = SMEM_U32_GET_FIELD(actionEntryDataPtr[1],17,2);

    return ;
}

/*******************************************************************************
* snetCht2EPclActionGetWithCh2ErrataVlanAndConter
*
* DESCRIPTION:
*        Get the action entry from the policy action table.
*        Supports FEr#1087 - CH2 only
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
*   Wrong HW format described below:
*     Policy action entry world0:
*     Bit[30], MatchCounterEn
*     Bits[29:25], MatchCounterIndex
*     Policy action entry world1:
*     Bits[30:20], PCE_VID[10:0]
*     Bits[19:18], PCE_VLANCmd
*     Policy action entry world2:
*     Bit[0], PCE_VID[11]
*
*******************************************************************************/
GT_VOID snetCht2EPclActionGetWithCh2ErrataVlanAndConter
(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr  ,
    IN GT_U32                             matchIndex,
    OUT SNET_CHT2_EPCL_ACTION_STC       * actionDataPtr
)
{
    DECLARE_FUNC_NAME(snetCht2EPclActionGetWithCh2ErrataVlanAndConter);

    GT_U32  tblAddr;
    GT_U32  * actionEntryDataPtr;

    tblAddr = SMEM_CHT_PCL_ACTION_TBL_MEM(devObjPtr, matchIndex) ;
    actionEntryDataPtr = smemMemGet(devObjPtr,tblAddr);

    /* Read word 0 from the action table entry */
    __LOG(("Read word 0 from the action table entry"));
    actionDataPtr->matchCounterIndex = SMEM_U32_GET_FIELD(actionEntryDataPtr[0],25,5);
    actionDataPtr->matchCounterEn = SMEM_U32_GET_FIELD(actionEntryDataPtr[0],30,1);
    actionDataPtr->pceModifyUp = SMEM_U32_GET_FIELD(actionEntryDataPtr[0],23,2);
    actionDataPtr->pceModifyDscp = SMEM_U32_GET_FIELD(actionEntryDataPtr[0],21,2);
    actionDataPtr->epceUp = SMEM_U32_GET_FIELD(actionEntryDataPtr[0],18,3);
    actionDataPtr->epceDscp = SMEM_U32_GET_FIELD(actionEntryDataPtr[0],12,6);
    actionDataPtr->fwdCmd = SMEM_U32_GET_FIELD(actionEntryDataPtr[0],0,3);

    /* Read word 1 from the action table entry */
    __LOG(("Read word 1 from the action table entry"));
    actionDataPtr->epceVid =
        SMEM_U32_GET_FIELD(actionEntryDataPtr[1],20,11)
        | ((SMEM_U32_GET_FIELD(actionEntryDataPtr[2],1,1)) << 11);
    actionDataPtr->epceVlanCmd = SMEM_U32_GET_FIELD(actionEntryDataPtr[1],18,2);

    return ;
}

/*******************************************************************************
* snetCht2EPclCreateKey
*
* DESCRIPTION:
*        Create Egress PCL search tram key.
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - pointer to frame data buffer Id.
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
*******************************************************************************/
static GT_VOID snetCht2EPclCreateKey
(
    IN SKERNEL_DEVICE_OBJECT                * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC   * descrPtr  ,
    IN SNET_CHT2_EPCL_LOOKUP_CONFIG_STC     * lookupConfPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC        * pclExtraDataPtr,
    IN CHT2_EPCL_KEY_TYPE_ENT                 keyType,
    OUT SNET_CHT_POLICY_KEY_STC             * pclKeyPtr
)
{
    memset(&pclKeyPtr->key, 0, sizeof(pclKeyPtr->key));
    pclKeyPtr->devObjPtr = devObjPtr;

    switch (keyType)
    {
        case CHT2_EPCL_KEY_TYPE_L2_KEY_SHORT_E:
            snetCht2EPclBuildL2StandardKey(devObjPtr , descrPtr ,
                                           lookupConfPtr,pclExtraDataPtr,
                                           pclKeyPtr) ;
            break;
        case CHT2_EPCL_KEY_TYPE_L2_L3_KEY_SHORT_E:
            snetCht2EPclBuildL2L3StandardKey(devObjPtr , descrPtr ,lookupConfPtr ,
                                             pclExtraDataPtr, pclKeyPtr);
            break;
        case CHT2_EPCL_KEY_TYPE_L3_L4_KEY_SHORT_E:
            snetCht2EPclBuildL3L4StandardKey(devObjPtr ,descrPtr ,lookupConfPtr ,
                                             pclExtraDataPtr, pclKeyPtr);
            break;
        case CHT2_EPCL_KEY_TYPE_L2_L3_L4_KEY_LONG_E:
            snetCht2EPclBuildL2L3L4ExtendedKey(devObjPtr, descrPtr ,lookupConfPtr ,
                                                 pclExtraDataPtr, pclKeyPtr);
            break;
        case CHT2_EPCL_KEY_TYPE_L2_IPV6_KEY_LONG_E:
            snetCht2EPclKeyBuildL2IPv6ExtendedKey(devObjPtr, descrPtr,
                                                    lookupConfPtr ,
                                                    pclExtraDataPtr, pclKeyPtr);
            break;
        case CHT2_EPCL_KEY_TYPE_L4_IPV6_KEY_LONG_E:
            snetCht2EPclKeyBuildL4IPv6ExtendedKey(devObjPtr , descrPtr ,
                                                  lookupConfPtr,pclExtraDataPtr,
                                                  pclKeyPtr);
            break;

        default:
            skernelFatalError("keyType: not valid mode[%d]", keyType);
            break;
    }

    pclKeyPtr->updateOnlyDiff = GT_FALSE;
    pclKeyPtr->pclKeyFormat = SNET_CHT2_EPCL_KEY_FORMAT(keyType);

    return ;
}

/*******************************************************************************
* snetCht2EPclTriggeringCheck
*
* DESCRIPTION:
*        Check the enabling status of egress PCL for the outgoing frame.
*           Get the epcl-id configuration setup.
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - pointer to frame data buffer Id
*
* OUTPUTS:
*       doLookupPtr     - pointer to enabling the PCL engine.
*       lookupConfPtr   - pointer to lookup configuration.
*
* RETURN:
*
* COMMENTS:
*         1. Check if EPCL is globally enabled.
*         2. Check if EPCL is enabled per port .
*         3. Get Egress PCL configuration entry.
*
*******************************************************************************/
static GT_VOID snetCht2EPclTriggeringCheck
(
    IN SKERNEL_DEVICE_OBJECT             *  devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC   *  descrPtr,
    IN GT_U32                               port,
    OUT GT_U8                            *  doLookupPtr,
    OUT SNET_CHT2_EPCL_LOOKUP_CONFIG_STC *  lookupConfPtr
)
{
    DECLARE_FUNC_NAME(snetCht2EPclTriggeringCheck);

    GT_U32  regAddress;
    GT_U32  ePclDisabled;
    GT_U32  outPort;
    GT_U32  ePclPerPortState = 0;

    /* initialize the dolookupPtr */
    *doLookupPtr = 0;

    /* check if egress PCL is globally enabled */
    __LOG(("check if egress PCL is globally enabled"));
    regAddress = SMEM_CHT2_EPCL_GLOBAL_REG(devObjPtr);
    smemRegFldGet(devObjPtr, regAddress, 7, 1, &ePclDisabled);
    if (ePclDisabled)
    {   /* egress policy is disabled - *doLookupPtr = 0 */
        __LOG(("    {egress policy is disabled - *doLookupPtr = 0"));
        return ;
    }

    outPort = (port != SNET_CHT_CPU_PORT_CNS) ? port : 0x1f;

    /* check if a bit per Egress port to Enable/Disable Egress Policy
       on packets forwarded to the CPU (local or remote) */
    if (descrPtr->outGoingMtagCmd == SKERNEL_MTAG_CMD_TO_CPU_E)
    {
        regAddress = SMEM_CHT2_EPCL_TO_CPU_CONFIG_REG(devObjPtr);
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
        regAddress = SMEM_CHT2_EPCL_FROM_CPU_CONFIG_REG(devObjPtr);
        smemRegFldGet(devObjPtr, regAddress, outPort, 1, &ePclPerPortState);
        if (ePclPerPortState == 0)
        {
            return ;
        }
    }

    /* check if a bit per Egress port to Enable/Disable Egress Policy on
        TO_ANALYSER packets e.g. packets forwarded to the ingress or egress
        analyser port */
    if ((descrPtr->rxSniff ||
        descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_MIRROR_TO_CPU_E ||
        descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_TO_TRG_SNIFFER_E))
    {
        regAddress = SMEM_CHT2_EPCL_TO_ANALYSER_CONFIG_REG(devObjPtr);
        smemRegFldGet(devObjPtr, regAddress, outPort, 1, &ePclPerPortState);
        if (ePclPerPortState == 0)
        {
            return ;
        }
    }

    /* check if a bit per Egress port to Enable/Disable Egress Policy
       on data packets which are NOT tunneled in this device */
    if (descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_FORWARD_E &&
        descrPtr->tunnelStart == 0)
    {
        regAddress = SMEM_CHT2_EPCL_NTS_FORWARD_CONFIG_REG(devObjPtr);
        smemRegFldGet(devObjPtr, regAddress, outPort, 1, &ePclPerPortState);
        if (ePclPerPortState == 0)
        {
            return ;
        }
    }

    /* check if a bit per Egress port to Enable/Disable Egress Policy
       on data packets which are tunneled in this device */
    if (descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_FORWARD_E &&
        descrPtr->tunnelStart == 1)
    {
        regAddress = SMEM_CHT2_EPCL_TS_FORWARD_CONFIG_REG(devObjPtr);
        smemRegFldGet(devObjPtr, regAddress, outPort, 1, &ePclPerPortState);
        if (ePclPerPortState == 0)
        {
            return ;
        }
    }

    /* get EPCL configuration entry */
    __LOG(("get EPCL configuration entry"));
    memset(lookupConfPtr,0,sizeof(SNET_CHT2_EPCL_LOOKUP_CONFIG_STC));
    snetCht2EPclConfigPtrGet(devObjPtr,descrPtr,port,lookupConfPtr);

    *doLookupPtr = (lookupConfPtr->lookupConfig.enabled == 0) ? 0 : 1;

    return ;
}

/*******************************************************************************
* snetCht2EPcl
*
* DESCRIPTION:
*        Egress Policy Engine processing for outgoing frame on Cheetah2
*        asic simulation.
*        ePCL processing , ePcl assignment ,key forming , 1 Lookup ,
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
*   Egress PCL has only one lookup cycle.
*   C.12.9 : Egress Policy Registers
*
*******************************************************************************/
extern GT_VOID snetCht2EPcl
(
    IN SKERNEL_DEVICE_OBJECT                * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC   * descrPtr ,
    IN  GT_U32                                port
)
{
    DECLARE_FUNC_NAME(snetCht2EPcl);

    SNET_CHT2_EPCL_LOOKUP_CONFIG_STC lookupConf; /* lookup configuration
                                                       Table 266: Ingress PCL
                                                       configuration Table    */
    GT_U8                           doLookup;  /* enable searching the TCAM */
    CHT_PCL_EXTRA_PACKET_INFO_STC   pclExtraData;
    SNET_CHT_POLICY_KEY_STC         pclKey; /* policy key structure */
    CHT2_EPCL_KEY_TYPE_ENT          keyType; /* tcam search key type */
    GT_U32                          matchIndex; /* index to the matching rule */
    SNET_CHT2_EPCL_ACTION_STC       actionData;

    /* Get the ePCL configuration entry (Table 270 : EPCL configuration Table)*/
    __LOG(("Get the ePCL configuration entry (Table 270 : EPCL configuration Table)"));
    snetCht2EPclTriggeringCheck(devObjPtr,descrPtr,port,&doLookup,&lookupConf);
    if (doLookup == 0) /* =0 , means ePCL engine is disabled */
    {
        return ;
    }

    /* parse extra data need by the epcl engine */
    __LOG(("parse extra data need by the epcl engine"));
    snetChtPclSrvParseExtraData(devObjPtr,  descrPtr,  &pclExtraData);

    /* get the tcam key type and the tcam key size , from the configuration  */
    __LOG(("get the tcam key type and the tcam key size , from the configuration"));
    snetCht2EPclGetKeyType(descrPtr, &lookupConf, &keyType);

    pclKey.devObjPtr = devObjPtr;
    /* create egress policy tcam search key */
    __LOG(("create egress policy tcam search key"));
    snetCht2EPclCreateKey(devObjPtr, descrPtr ,&lookupConf,
                            &pclExtraData, keyType , &pclKey);

    /* search key in policy Tcam */
    __LOG(("search key in policy Tcam"));
    snetCht2SrvPclTcamLookUp(devObjPtr , descrPtr , &pclKey , &matchIndex);

    /* read and apply policy action */
    __LOG(("read and apply policy action"));
    if (matchIndex != SNET_CHT_POLICY_NO_MATCH_INDEX_CNS)
    {   /* read the action from the action table */
        __LOG(("    {read the action from the action table"));
        snetCht2EPclActionGetWithCh2ErrataVlanAndConter(
            devObjPtr, descrPtr , matchIndex , &actionData);

        /* apply the matching action */
        __LOG(("apply the matching action"));
        snetCht2EPclActionApply(devObjPtr, descrPtr , &actionData);
    }

    return;
}

