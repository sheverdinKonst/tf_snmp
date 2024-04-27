/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetCheetahPcl.c
*
* DESCRIPTION:
*       (Cheetah) Ingress Policy Engine processing for frame -- simulation
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 24 $
*
*******************************************************************************/

#include <asicSimulation/SKernel/smem/smemCheetah.h>
#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SKernel/suserframes/snet.h>
#include <common/Utils/FrameInfo/sframeInfoAddr.h>
#include <asicSimulation/SKernel/cheetahCommon/sregCheetah.h>
#include <asicSimulation/SKernel/suserframes/snetCheetahPcl.h>
#include <asicSimulation/SKernel/suserframes/snetCheetahPclSrv.h>
#include <asicSimulation/SLog/simLog.h>

/* size in word of the tcam entry */
#define CHT_PCL_TCAM_ENTRY_WORDS_WIDTH_CNS  8
#define CHT_PCL_TCAM_OFFSET_TO_SECOND_HALF_TABLE_IN_WORD_CNS  (8*512)

#define SNET_CHT_PCL_KEY_WORDS(pcl_key) \
    ((pcl_key->pclKeyFormat) == CHT_PCL_KEY_REGULAR_E)  ?  6 : \
    ((pcl_key->pclKeyFormat) == CHT_PCL_KEY_EXTENDED_E) ? 12 : 18
/*
    enum :  CHT_PCL_KEY_TYPE_ENT

    purpose : to enum the policy key search types

*/
typedef enum{
    CHT_PCL_KEY_TYPE_L2_KEY_SHORT_E              ,
    CHT_PCL_KEY_TYPE_L2_AND_L3_KEY_SHORT_E       ,
    CHT_PCL_KEY_TYPE_L3_AND_L4_KEY_SHORT_E       ,
    CHT_PCL_KEY_TYPE_L2_AND_L3_AND_L4_KEY_LONG_E ,
    CHT_PCL_KEY_TYPE_L2_AND_L3_KEY_LONG_E        ,
    CHT_PCL_KEY_TYPE_L3_AND_L4_KEY_LONG_E        ,
    /* IPV6 DIP short search key */
    CHT_PCL_KEY_TYPE_L3_KEY_SHORT_E              ,
    CHT_PCL_KEY_TYPE_LAST_E
}CHT_PCL_KEY_TYPE_ENT;

/*
    enum :  CHT_PCL_KEY_FIELDS_ID_ENT

    purpose : to enum the policy key fields

*/
typedef enum{
    CHT_PCL_KEY_FIELDS_ID_VALID_E,
    CHT_PCL_KEY_FIELDS_ID_PCL_ID_E,
    CHT_PCL_KEY_FIELDS_ID_SRC_PORT_E,
    CHT_PCL_KEY_FIELDS_ID_IS_TAGGET_E,
    CHT_PCL_KEY_FIELDS_ID_VID_E,

    CHT_PCL_KEY_FIELDS_ID_UP_E,
    CHT_PCL_KEY_FIELDS_ID_QOS_PROFILE_E,
    CHT_PCL_KEY_FIELDS_ID_IS_IPV4_E,
    CHT_PCL_KEY_FIELDS_ID_IS_IP_E,
    CHT_PCL_KEY_FIELDS_ID_RESERVED_42_E,
    CHT_PCL_KEY_FIELDS_ID_ETHER_TYPE_DSAP_SSAP_E,
    CHT_PCL_KEY_FIELDS_ID_USER_DEFINED_BYTE2_SHORT_E,
    CHT_PCL_KEY_FIELDS_ID_USER_DEFINED_BYTE3_SHORT_E,
    CHT_PCL_KEY_FIELDS_ID_USER_DEFINED_BYTE0_SHORT_E,
    CHT_PCL_KEY_FIELDS_ID_USER_DEFINED_BYTE1_SHORT_E,
    CHT_PCL_KEY_FIELDS_ID_IS_ETH_TYPE_VALID_E,
    CHT_PCL_KEY_FIELDS_ID_MAC_SA_E,
    CHT_PCL_KEY_FIELDS_ID_MAC_DA_E,
    CHT_PCL_KEY_FIELDS_ID_USER_DEFINED_VALID_E,
    CHT_PCL_KEY_FIELDS_ID_RESERVED_189_191_E,

    CHT_PCL_KEY_FIELDS_ID_IP_PROTOCOL_E,
    CHT_PCL_KEY_FIELDS_ID_PACKET_DSCP_E,
    CHT_PCL_KEY_FIELDS_ID_IS_L4_VALID_E,
    CHT_PCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_2_3_E,
    CHT_PCL_KEY_FIELDS_ID_IS_ARP_E,
    CHT_PCL_KEY_FIELDS_ID_IS_IPV6_EH_EXISTS_E,
    CHT_PCL_KEY_FIELDS_ID_IS_BC_E,

    CHT_PCL_KEY_FIELDS_ID_IS_IPV6_EH_HOP_BY_HOP_E,
    CHT_PCL_KEY_FIELDS_ID_RESERVED_189_E,
    CHT_PCL_KEY_FIELDS_ID_IPV4_FRAGMENT_E,
    CHT_PCL_KEY_FIELDS_ID_HEADER_OK_E,

    CHT_PCL_KEY_FIELDS_ID_USER_DEFINED_BYTE2_L3_AND_L4_KEY_SHORT_E,
    CHT_PCL_KEY_FIELDS_ID_SIP_31_0_E,
    CHT_PCL_KEY_FIELDS_ID_DIP_31_0_E,
    CHT_PCL_KEY_FIELDS_ID_L4_BYTES_OFFSETS_0_1_13_E,
    CHT_PCL_KEY_FIELDS_ID_RESERVED_187_E,

    CHT_PCL_KEY_FIELDS_ID_LONG_L4_BYTES_OFFSETS_0_1_13_E,
    CHT_PCL_KEY_FIELDS_ID_LONG_SIP_31_0_E,
    CHT_PCL_KEY_FIELDS_ID_LONG_DIP_31_0_E,
    CHT_PCL_KEY_FIELDS_ID_LONG_IS_ETH_TYPE_VALID_E,
    CHT_PCL_KEY_FIELDS_ID_LONG_ETHER_TYPE_DSAP_SSAP_E,
    CHT_PCL_KEY_FIELDS_ID_LONG_IPV4_FRAGMENT_E,
    CHT_PCL_KEY_FIELDS_ID_LONG_RESERVED_179_234_E,
    CHT_PCL_KEY_FIELDS_ID_LONG_MAC_SA_E,
    CHT_PCL_KEY_FIELDS_ID_LONG_MAC_DA_E,
    CHT_PCL_KEY_FIELDS_ID_LONG_USER_DEFINED_BYTE3_E,
    CHT_PCL_KEY_FIELDS_ID_LONG_USER_DEFINED_BYTE4_E,
    CHT_PCL_KEY_FIELDS_ID_LONG_USER_DEFINED_BYTE5_E,
    CHT_PCL_KEY_FIELDS_ID_LONG_USER_DEFINED_BYTE0_E,
    CHT_PCL_KEY_FIELDS_ID_LONG_USER_DEFINED_BYTE1_E,
    CHT_PCL_KEY_FIELDS_ID_LONG_USER_DEFINED_BYTE2_E,
    CHT_PCL_KEY_FIELDS_ID_LONG_RESERVED_379_381_E,
    CHT_PCL_KEY_FIELDS_ID_LONG_USER_DEFINED_VALID_E,
    CHT_PCL_KEY_FIELDS_ID_LONG_HEADER_OK_E,

    CHT_PCL_KEY_FIELDS_ID_LONG_SIP_63_32_E,
    CHT_PCL_KEY_FIELDS_ID_LONG_SIP_95_64_E,
    CHT_PCL_KEY_FIELDS_ID_LONG_SIP_127_96_E,
    CHT_PCL_KEY_FIELDS_ID_LONG_IS_IPV6_EH_EXISTS_E,
    CHT_PCL_KEY_FIELDS_ID_LONG_IS_IPV6_EH_HOP_BY_HOP_E,
    CHT_PCL_KEY_FIELDS_ID_LONG_DIP_127_120_E,


    CHT_PCL_KEY_FIELDS_ID_LONG_DIP_63_32_E,
    CHT_PCL_KEY_FIELDS_ID_LONG_DIP_95_64_E,
    CHT_PCL_KEY_FIELDS_ID_LONG_DIP_119_96_E,
    CHT_PCL_KEY_FIELDS_ID_LONG_DIP_31_0_L3_AND_L4_KEY_LONG_E,

    CHT_PCL_KEY_FIELDS_ID_IS_IPV6_E,

    /* UC routing specific fields */
    CHT_PCL_KEY_FIELDS_ID_DIP_15_0_E,
    CHT_PCL_KEY_FIELDS_ID_DIP_31_16_E,
    CHT_PCL_KEY_FIELDS_ID_RESERVED_91_E,
    CHT_PCL_KEY_FIELDS_ID_DIP_127_32_E,
    CHT_PCL_KEY_FIELDS_ID_RESERVED_188_E,
    /* last -- for internal use only -- not a field */
    CHT_PCL_KEY_FIELDS_ID_LAST_E
}CHT_PCL_KEY_FIELDS_ID_ENT;

static CHT_PCL_USER_DEFINED_CONFIG_REG_STC chtPclUserDefinedConfigInfoShortL2KeyArray[4] =
{
    {0x14/4 ,0 ,7 },
    {0x14/4 ,11,18},
    {0x14/4 ,22,29},
    {0x18/4 ,0 ,7 }
};

static CHT_PCL_USER_DEFINED_CONFIG_REG_STC chtPclUserDefinedConfigInfoShortL3KeyArray[2] =
{
    {0x18/4 ,11,18},
    {0x18/4 ,22,29}
};

static CHT_PCL_USER_DEFINED_CONFIG_REG_STC chtPclUserDefinedConfigInfoShortL4KeyArray[3] =
{
    {0x1c/4 ,0 ,7 },
    {0x1c/4 ,11,18},
    {0x1c/4 ,22,29}
};

static CHT_PCL_USER_DEFINED_CONFIG_REG_STC chtPclUserDefinedConfigInfoLongL2KeyArray[6] =
{
    {0x0/4 ,0 ,7 },
    {0x0/4 ,11,18},
    {0x0/4 ,22,29},
    {0x4/4 ,0 ,7 },
    {0x4/4 ,11,18},
    {0x4/4 ,22,29}
};

static CHT_PCL_USER_DEFINED_CONFIG_REG_STC chtPclUserDefinedConfigInfoLongL3KeyArray[6] =
{
    {0x8/4 ,0 ,7 },
    {0x8/4 ,11,18},
    {0x8/4 ,22,29},
    {0xc/4 ,0 ,7 },
    {0xc/4 ,11,18},
    {0xc/4 ,22,29}
};

static CHT_PCL_USER_DEFINED_CONFIG_REG_STC chtPclUserDefinedConfigInfoLongL4KeyArray[3] =
{
    {0x10/4 ,0 ,7 },
    {0x10/4 ,11,18},
    {0x10/4 ,22,29},
};

/* array that holds the info about the fields */
static CHT_PCL_KEY_FIELDS_INFO_STC chtPclKeyFieldsAllArray[CHT_PCL_KEY_FIELDS_ID_LAST_E]=
{
    {0  ,0  ,GT_FALSE   ," CHT_PCL_KEY_FIELDS_ID_VALID_E "  },
    {1  ,10 ,GT_TRUE    ," CHT_PCL_KEY_FIELDS_ID_PCL_ID_E "  },
    {11 ,16 ,GT_TRUE    ," CHT_PCL_KEY_FIELDS_ID_SRC_PORT_E "  },
    {17 ,17 ,GT_FALSE   ," CHT_PCL_KEY_FIELDS_ID_IS_TAGGET_E "  },
    {18 ,29 ,GT_TRUE    ," CHT_PCL_KEY_FIELDS_ID_VID_E "  },
    {30 ,32 ,GT_TRUE    ," CHT_PCL_KEY_FIELDS_ID_UP_E "  },
    {33 ,39 ,GT_TRUE    ," CHT_PCL_KEY_FIELDS_ID_QOS_PROFILE_E "  },

    {40 ,40 ,GT_FALSE   ," CHT_PCL_KEY_FIELDS_ID_IS_IPV4_E "  },
    {41 ,41 ,GT_FALSE   ," CHT_PCL_KEY_FIELDS_ID_IS_IP_E "  },
    {42 ,42 ,GT_FALSE   ," CHT_PCL_KEY_FIELDS_ID_RESERVED_42_E "  },
    {43 ,58 ,GT_FALSE   ," CHT_PCL_KEY_FIELDS_ID_ETHER_TYPE_DSAP_SSAP_E "  },
    {59 ,66 ,GT_FALSE   ," CHT_PCL_KEY_FIELDS_ID_USER_DEFINED_BYTE2_SHORT_E "  },
    {67 ,74 ,GT_FALSE   ," CHT_PCL_KEY_FIELDS_ID_USER_DEFINED_BYTE3_SHORT_E "  },
    {75 ,82 ,GT_FALSE   ," CHT_PCL_KEY_FIELDS_ID_USER_DEFINED_BYTE0_SHORT_E "  },
    {83 ,90 ,GT_FALSE   ," CHT_PCL_KEY_FIELDS_ID_USER_DEFINED_BYTE1_SHORT_E "  },
    {91 ,91 ,GT_FALSE   ," CHT_PCL_KEY_FIELDS_ID_IS_ETH_TYPE_VALID_E "  },
    {92 ,139,GT_FALSE   ," CHT_PCL_KEY_FIELDS_ID_MAC_SA_E "  },
    {140,187,GT_FALSE   ," CHT_PCL_KEY_FIELDS_ID_MAC_DA_E "  },
    {188,188,GT_FALSE   ," CHT_PCL_KEY_FIELDS_ID_USER_DEFINED_VALID_E "  },
    {189,191,GT_FALSE   ," CHT_PCL_KEY_FIELDS_ID_RESERVED_189_191_E "  },



    {42 ,49 ,GT_FALSE   ," CHT_PCL_KEY_FIELDS_ID_IP_PROTOCOL_E "  },
    {50 ,55 ,GT_FALSE   ," CHT_PCL_KEY_FIELDS_ID_PACKET_DSCP_E "  },
    {56 ,56 ,GT_FALSE   ," CHT_PCL_KEY_FIELDS_ID_IS_L4_VALID_E "  },
    {57 ,72 ,GT_FALSE   ," CHT_PCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_2_3_E "  },
    {73 ,73 ,GT_FALSE   ," CHT_PCL_KEY_FIELDS_ID_IS_ARP_E "  },
    {74 ,74 ,GT_FALSE   ," CHT_PCL_KEY_FIELDS_ID_IS_IPV6_EH_EXISTS_E "  },

    {74 ,74 ,GT_FALSE   ," CHT_PCL_KEY_FIELDS_ID_IS_BC_E "  },

    {91 ,91 ,GT_FALSE   ," CHT_PCL_KEY_FIELDS_ID_IS_IPV6_EH_HOP_BY_HOP_E "  },
    {189,189,GT_FALSE   ," CHT_PCL_KEY_FIELDS_ID_RESERVED_189_E "  },
    {190,190,GT_FALSE   ," CHT_PCL_KEY_FIELDS_ID_IPV4_FRAGMENT_E "  },
    {191,191,GT_FALSE   ," CHT_PCL_KEY_FIELDS_ID_HEADER_OK_E "  },

    {91 ,98 ,GT_FALSE   ," CHT_PCL_KEY_FIELDS_ID_USER_DEFINED_BYTE2_L3_AND_L4_KEY_SHORT_E "  },
    {99 ,130,GT_FALSE   ," CHT_PCL_KEY_FIELDS_ID_SIP_31_0_E "  },
    {131,162,GT_FALSE   ," CHT_PCL_KEY_FIELDS_ID_DIP_31_0_E "  },
    {163,186,GT_FALSE   ," CHT_PCL_KEY_FIELDS_ID_L4_BYTES_OFFSETS_0_1_13_E "  },
    {187,187,GT_FALSE   ," CHT_PCL_KEY_FIELDS_ID_RESERVED_187_E "  },

    /* start difference for long keys */

    {73 ,96 ,GT_FALSE   ," CHT_PCL_KEY_FIELDS_ID_LONG_L4_BYTES_OFFSETS_0_1_13_E "  },
    {97 ,128,GT_FALSE   ," CHT_PCL_KEY_FIELDS_ID_LONG_SIP_31_0_E "  },
    {129,160,GT_FALSE   ," CHT_PCL_KEY_FIELDS_ID_LONG_DIP_31_0_E "  },
    {161,161,GT_FALSE   ," CHT_PCL_KEY_FIELDS_ID_LONG_IS_ETH_TYPE_VALID_E "  },
    {162,177,GT_FALSE   ," CHT_PCL_KEY_FIELDS_ID_LONG_ETHER_TYPE_DSAP_SSAP_E "  },
    {178,178,GT_FALSE   ," CHT_PCL_KEY_FIELDS_ID_LONG_IPV4_FRAGMENT_E "  },
    {179,234,GT_FALSE   ," CHT_PCL_KEY_FIELDS_ID_LONG_RESERVED_179_234_E "  },

    {235,282,GT_FALSE   ," CHT_PCL_KEY_FIELDS_ID_LONG_MAC_SA_E "  },
    {283,330,GT_FALSE   ," CHT_PCL_KEY_FIELDS_ID_LONG_MAC_DA_E "  },

    {331,338,GT_FALSE   ," CHT_PCL_KEY_FIELDS_ID_LONG_USER_DEFINED_BYTE3_E "  },
    {339,346,GT_FALSE   ," CHT_PCL_KEY_FIELDS_ID_LONG_USER_DEFINED_BYTE4_E "  },
    {347,354,GT_FALSE   ," CHT_PCL_KEY_FIELDS_ID_LONG_USER_DEFINED_BYTE5_E "  },
    {355,362,GT_FALSE   ," CHT_PCL_KEY_FIELDS_ID_LONG_USER_DEFINED_BYTE0_E "  },
    {363,370,GT_FALSE   ," CHT_PCL_KEY_FIELDS_ID_LONG_USER_DEFINED_BYTE1_E "  },
    {371,378,GT_FALSE   ," CHT_PCL_KEY_FIELDS_ID_LONG_USER_DEFINED_BYTE2_E "  },
    {379,381,GT_FALSE   ," CHT_PCL_KEY_FIELDS_ID_LONG_RESERVED_379_381_E "  },
    {382,382,GT_FALSE   ," CHT_PCL_KEY_FIELDS_ID_LONG_USER_DEFINED_VALID_E "  },
    {383,383,GT_FALSE   ," CHT_PCL_KEY_FIELDS_ID_LONG_HEADER_OK_E "  },


    {129,160,GT_FALSE   ," CHT_PCL_KEY_FIELDS_ID_LONG_SIP_63_32_E "  },
    {161,192,GT_FALSE   ," CHT_PCL_KEY_FIELDS_ID_LONG_SIP_64_95_E "  },
    {193,224,GT_FALSE   ," CHT_PCL_KEY_FIELDS_ID_LONG_SIP_96_127_E "  },
    {225,225,GT_FALSE   ," CHT_PCL_KEY_FIELDS_ID_LONG_IS_IPV6_EH_EXISTS_E "  },
    {226,226,GT_FALSE   ," CHT_PCL_KEY_FIELDS_ID_LONG_IS_IPV6_EH_HOP_BY_HOP_E "  },
    {227,234,GT_FALSE   ," CHT_PCL_KEY_FIELDS_ID_LONG_DIP_127_120_E "  },

    {235,266,GT_FALSE   ," CHT_PCL_KEY_FIELDS_ID_LONG_DIP_63_32_E "  },
    {267,298,GT_FALSE   ," CHT_PCL_KEY_FIELDS_ID_LONG_DIP_95_64_E "  },
    {299,322,GT_FALSE   ," CHT_PCL_KEY_FIELDS_ID_LONG_DIP_119_96_E " },
    {323,354,GT_FALSE   ," CHT_PCL_KEY_FIELDS_ID_LONG_DIP_31_0_L3_AND_L4_KEY_LONG_E "  },

    {40 ,40 ,GT_FALSE   ," CHT_PCL_KEY_FIELDS_ID_IS_IPV6_E "  },

    /* IPV6 DIP Key specific fields */
    {57, 72 ,GT_TRUE    ," CHT_PCL_KEY_FIELDS_ID_DIP_15_0_E " },
    {75, 90 ,GT_TRUE    ," CHT_PCL_KEY_FIELDS_ID_DIP_31_16_E " },
    {91, 91 ,GT_FALSE   ," CHT_PCL_KEY_FIELDS_ID_RESERVED_91_E " },
    {92 ,187,GT_FALSE   ," CHT_PCL_KEY_FIELDS_ID_DIP_127_32_E " }
};

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
static GT_VOID snetChtPclKeyFieldBuildByPointer
(
    INOUT SNET_CHT_POLICY_KEY_STC *pclKeyPtr,
    IN GT_U8                      *fieldValPtr,
    IN CHT_PCL_KEY_FIELDS_ID_ENT  fieldId,
    IN GT_U32                     cycleNum
)
{
    CHT_PCL_KEY_FIELDS_INFO_STC *fieldInfoPtr = &chtPclKeyFieldsAllArray[fieldId] ;

    snetChtPclSrvKeyFieldBuildByPointer (pclKeyPtr, fieldValPtr, fieldInfoPtr);

    return;
}

/*******************************************************************************
*   snetChtPclKeyFieldBuildByValue
*
* DESCRIPTION:
*       function inserts data of the field to the search key
*       in specific place in key
*       used for "numeric" fields - up to GT_U32
*
* INPUTS:
*       pclKeyPtr - (pointer to) current pcl key
*       fieldVal - data of field to insert to key
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
static GT_VOID snetChtPclKeyFieldBuildByValue
(
    INOUT SNET_CHT_POLICY_KEY_STC *pclKeyPtr,
    IN GT_U32                     fieldVal,
    IN CHT_PCL_KEY_FIELDS_ID_ENT  fieldId,
    IN GT_U32                     cycleNum
)
{
    CHT_PCL_KEY_FIELDS_INFO_STC *fieldInfoPtr = &chtPclKeyFieldsAllArray[fieldId] ;

    snetChtPclSrvKeyFieldBuildByValue(pclKeyPtr, fieldVal, fieldInfoPtr);

    return;
}

/*******************************************************************************
*   snetChtPclKeyShortL2Build
*
* DESCRIPTION:
*       function build key for CHT_PCL_KEY_TYPE_L2_KEY_SHORT_E
*
* INPUTS:
*       devObjPtr    - (pointer to) device object.
*       descrPtr     - (pointer to) frame data buffer Id
*       cycleNum -  the number of current cycle
*       currPclLookupConfigPtr - info (structure) on the lookup config on
*                                the current pclId (current cycle)
*       pclKeyPtr - (pointer to) last pcl key
*       pclCookiePtr - (pointer to) pcl extra info
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
static GT_VOID snetChtPclKeyShortL2Build
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_U32  cycleNum,
    IN SNET_CHT_POLICY_SPECIFIC_LOOKUP_CONFIG_STC *currPclLookupConfigPtr,
    INOUT SNET_CHT_POLICY_KEY_STC *pclKeyPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC *pclCookiePtr
)
{
    GT_STATUS   rc;/* status from called function */
    GT_U32  userDefinedAnchor;/* user defined byte Anchor */
    GT_U32  userDefinedOffset;/* user defined byte offset from Anchor */
    GT_U8   byteValue;/* value of user defined byte from the packets */
    GT_BOOL didUserDefinedError = GT_FALSE;/* is there user defined byte error */
    GT_U32  *userDefinedInfoMemPtr; /*pointer to first register of info about the
                                      user defined bytes of the key */
    GT_U8   tmpFieldVal[4];
    GT_U8   value1 = 1;
    GT_U8   value0 = 0;
    CHT_PCL_USER_DEFINED_CONFIG_REG_STC *pclUserDefConfigPtr;

    /* get pointer to user defined bytes info */
    userDefinedInfoMemPtr = smemMemGet(devObjPtr,0x0B800020);

    snetChtPclKeyFieldBuildByValue(pclKeyPtr, value1,
                            CHT_PCL_KEY_FIELDS_ID_VALID_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, currPclLookupConfigPtr->pclId,
                            CHT_PCL_KEY_FIELDS_ID_PCL_ID_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->localDevSrcPort,
                            CHT_PCL_KEY_FIELDS_ID_SRC_PORT_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->origSrcTagged,
                            CHT_PCL_KEY_FIELDS_ID_IS_TAGGET_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->eVid,
                            CHT_PCL_KEY_FIELDS_ID_VID_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->up,
                            CHT_PCL_KEY_FIELDS_ID_UP_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->qos.qosProfile,
                            CHT_PCL_KEY_FIELDS_ID_QOS_PROFILE_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->isIPv4,
                            CHT_PCL_KEY_FIELDS_ID_IS_IPV4_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->isIp,
                            CHT_PCL_KEY_FIELDS_ID_IS_IP_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, value0,
                            CHT_PCL_KEY_FIELDS_ID_RESERVED_42_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->etherTypeOrSsapDsap,
                            CHT_PCL_KEY_FIELDS_ID_ETHER_TYPE_DSAP_SSAP_E,cycleNum);

    /* user defined byte 2 */
    pclUserDefConfigPtr = &chtPclUserDefinedConfigInfoShortL2KeyArray[2];
    userDefinedAnchor = SMEM_U32_GET_FIELD(userDefinedInfoMemPtr[pclUserDefConfigPtr->regOffsetAddress],
                                         pclUserDefConfigPtr->startBitAnchor,2);
    userDefinedOffset = SMEM_U32_GET_FIELD(userDefinedInfoMemPtr[pclUserDefConfigPtr->regOffsetAddress],
                                         pclUserDefConfigPtr->startBitOffset,7);

    rc = snetChtPclUserDefinedByteGet(devObjPtr,descrPtr,userDefinedAnchor,
                                userDefinedOffset,&byteValue);
    if(rc != GT_OK)
    {
        didUserDefinedError = GT_TRUE;
        byteValue = 0;
    }

    snetChtPclKeyFieldBuildByValue(pclKeyPtr, byteValue,
                        CHT_PCL_KEY_FIELDS_ID_USER_DEFINED_BYTE2_SHORT_E,cycleNum);


    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->arp,
                            CHT_PCL_KEY_FIELDS_ID_IS_ARP_E,cycleNum);


    /* user defined byte 0 */
    pclUserDefConfigPtr = &chtPclUserDefinedConfigInfoShortL2KeyArray[0];
    userDefinedAnchor = SMEM_U32_GET_FIELD(userDefinedInfoMemPtr[pclUserDefConfigPtr->regOffsetAddress],
                                         pclUserDefConfigPtr->startBitAnchor,2);
    userDefinedOffset = SMEM_U32_GET_FIELD(userDefinedInfoMemPtr[pclUserDefConfigPtr->regOffsetAddress],
                                         pclUserDefConfigPtr->startBitOffset,7);

    rc = snetChtPclUserDefinedByteGet(devObjPtr,descrPtr,userDefinedAnchor,
                                userDefinedOffset,&byteValue);
    if(rc != GT_OK)
    {
        didUserDefinedError = GT_TRUE;
        byteValue = 0;
    }

    snetChtPclKeyFieldBuildByValue(pclKeyPtr, byteValue,
                        CHT_PCL_KEY_FIELDS_ID_USER_DEFINED_BYTE0_SHORT_E,cycleNum);

    /* user defined byte 1 */
    pclUserDefConfigPtr = &chtPclUserDefinedConfigInfoShortL2KeyArray[1];
    userDefinedAnchor = SMEM_U32_GET_FIELD(userDefinedInfoMemPtr[pclUserDefConfigPtr->regOffsetAddress],
                                         pclUserDefConfigPtr->startBitAnchor,2);
    userDefinedOffset = SMEM_U32_GET_FIELD(userDefinedInfoMemPtr[pclUserDefConfigPtr->regOffsetAddress],
                                         pclUserDefConfigPtr->startBitOffset,7);

    rc = snetChtPclUserDefinedByteGet(devObjPtr,descrPtr,userDefinedAnchor,
                                userDefinedOffset,&byteValue);
    if(rc != GT_OK)
    {
        didUserDefinedError = GT_TRUE;
        byteValue = 0;
    }

    snetChtPclKeyFieldBuildByValue(pclKeyPtr, byteValue,
                        CHT_PCL_KEY_FIELDS_ID_USER_DEFINED_BYTE1_SHORT_E,cycleNum);

    /* end of user defined bytes */

    tmpFieldVal[0] = (descrPtr->l2Encaps == SKERNEL_ETHERNET_II_E) ? 1 : 0;
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, tmpFieldVal[0],
                        CHT_PCL_KEY_FIELDS_ID_IS_ETH_TYPE_VALID_E,cycleNum);


    snetChtPclKeyFieldBuildByPointer(pclKeyPtr,descrPtr->macSaPtr,
                        CHT_PCL_KEY_FIELDS_ID_MAC_SA_E,cycleNum);
    snetChtPclKeyFieldBuildByPointer(pclKeyPtr,descrPtr->macDaPtr,
                        CHT_PCL_KEY_FIELDS_ID_MAC_DA_E,cycleNum);

    tmpFieldVal[0] = (didUserDefinedError == GT_TRUE) ? 0 : 1;
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, tmpFieldVal[0],
                        CHT_PCL_KEY_FIELDS_ID_USER_DEFINED_VALID_E,cycleNum);

    snetChtPclKeyFieldBuildByValue(pclKeyPtr, value0,
                        CHT_PCL_KEY_FIELDS_ID_RESERVED_189_191_E,cycleNum);

    return;
}

/*******************************************************************************
*   snetChtPclKeyShortL3Build
*
* DESCRIPTION:
*       function build key for CHT_PCL_KEY_TYPE_L3_KEY_SHORT_E
*
* INPUTS:
*       devObjPtr    - (pointer to) device object.
*       descrPtr     - (pointer to) frame data buffer Id
*       cycleNum -  the number of current cycle
*       currPclLookupConfigPtr - info (structure) on the lookup config on
*                                the current pclId (current cycle)
*       pclKeyPtr - (pointer to) last pcl key
*       pclCookiePtr - (pointer to) pcl extra info
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
static GT_VOID snetChtPclKeyShortL3Build
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_U32  cycleNum,
    IN SNET_CHT_POLICY_SPECIFIC_LOOKUP_CONFIG_STC *currPclLookupConfigPtr,
    INOUT SNET_CHT_POLICY_KEY_STC *pclKeyPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC *pclCookiePtr
)
{
    GT_U8   tmpFieldVal[4];
    GT_U32  tmpDip[4];          /* Destination IP address in the CPU order */

    snetChtPclKeyFieldBuildByValue(pclKeyPtr, 1,
                            CHT_PCL_KEY_FIELDS_ID_VALID_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, currPclLookupConfigPtr->pclId,
                            CHT_PCL_KEY_FIELDS_ID_PCL_ID_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->localDevSrcPort,
                            CHT_PCL_KEY_FIELDS_ID_SRC_PORT_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->origSrcTagged,
                            CHT_PCL_KEY_FIELDS_ID_IS_TAGGET_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->eVid,
                            CHT_PCL_KEY_FIELDS_ID_VID_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->up,
                            CHT_PCL_KEY_FIELDS_ID_UP_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->qos.qosProfile,
                            CHT_PCL_KEY_FIELDS_ID_QOS_PROFILE_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->isIPv4,
                            CHT_PCL_KEY_FIELDS_ID_IS_IPV4_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->isIp,
                            CHT_PCL_KEY_FIELDS_ID_IS_IP_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->ipProt,
                            CHT_PCL_KEY_FIELDS_ID_IP_PROTOCOL_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dscp,
                            CHT_PCL_KEY_FIELDS_ID_PACKET_DSCP_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, pclCookiePtr->isL4Valid,
                            CHT_PCL_KEY_FIELDS_ID_IS_L4_VALID_E,cycleNum);

    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[3],
                            CHT_PCL_KEY_FIELDS_ID_DIP_15_0_E,cycleNum);

    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->arp,
                            CHT_PCL_KEY_FIELDS_ID_IS_ARP_E,cycleNum);

    tmpFieldVal[0] = (descrPtr->macDaType == SKERNEL_BROADCAST_MAC_E ||
                      descrPtr->macDaType == SKERNEL_BROADCAST_ARP_E) ? 1 : 0 ;
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, tmpFieldVal[0],
                            CHT_PCL_KEY_FIELDS_ID_IS_BC_E,cycleNum);

    snetChtPclKeyFieldBuildByValue(pclKeyPtr, (descrPtr->dip[3] >> 16),
                            CHT_PCL_KEY_FIELDS_ID_DIP_31_16_E,cycleNum);

    snetChtPclKeyFieldBuildByValue(pclKeyPtr, 0,
                        CHT_PCL_KEY_FIELDS_ID_RESERVED_91_E,cycleNum);

    /* Swap DIP from HOST to CPU order to be used by snetChtPclKeyFieldBuildByPointer */
    tmpDip[0] = SGT_LIB_SWAP_BYTES_AND_WORDS_MAC(descrPtr->dip[0]);
    tmpDip[1] = SGT_LIB_SWAP_BYTES_AND_WORDS_MAC(descrPtr->dip[1]);
    tmpDip[2] = SGT_LIB_SWAP_BYTES_AND_WORDS_MAC(descrPtr->dip[2]);
    tmpDip[3] = SGT_LIB_SWAP_BYTES_AND_WORDS_MAC(descrPtr->dip[3]);
    snetChtPclKeyFieldBuildByPointer(pclKeyPtr,(GT_U8*)&tmpDip[0],
                        CHT_PCL_KEY_FIELDS_ID_DIP_127_32_E,cycleNum);

    snetChtPclKeyFieldBuildByValue(pclKeyPtr, 0,
                        CHT_PCL_KEY_FIELDS_ID_RESERVED_188_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, 0,
                        CHT_PCL_KEY_FIELDS_ID_RESERVED_189_E,cycleNum);

    snetChtPclKeyFieldBuildByValue(pclKeyPtr, pclCookiePtr->isIpv4Fragment,
                        CHT_PCL_KEY_FIELDS_ID_IPV4_FRAGMENT_E,cycleNum);

    snetChtPclKeyFieldBuildByValue(pclKeyPtr, pclCookiePtr->isIpHeaderOk,
                        CHT_PCL_KEY_FIELDS_ID_HEADER_OK_E,cycleNum);

    return;
}
/*******************************************************************************
*   snetChtPclKeyShortL2L3Build
*
* DESCRIPTION:
*       function build key for CHT_PCL_KEY_TYPE_L2_AND_L3_KEY_SHORT_E
*
* INPUTS:
*       devObjPtr    - (pointer to) device object.
*       descrPtr     - (pointer to) frame data buffer Id
*       cycleNum -  the number of current cycle
*       currPclLookupConfigPtr - info (structure) on the lookup config on
*                                the current pclId (current cycle)
*       pclKeyPtr - (pointer to) last pcl key
*       pclCookiePtr - (pointer to) pcl extra info
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
static GT_VOID snetChtPclKeyShortL2L3Build
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_U32  cycleNum,
    IN SNET_CHT_POLICY_SPECIFIC_LOOKUP_CONFIG_STC *currPclLookupConfigPtr,
    INOUT SNET_CHT_POLICY_KEY_STC *pclKeyPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC *pclCookiePtr
)
{
    GT_STATUS   rc;/* status from called function */
    GT_U32  userDefinedAnchor;/* user defined byte Anchor */
    GT_U32  userDefinedOffset;/* user defined byte offset from Anchor */
    GT_U8   byteValue;/* value of user defined byte from the packets */
    GT_BOOL didUserDefinedError = GT_FALSE;/* is there user defined byte error */
    GT_U32  *userDefinedInfoMemPtr; /*pointer to first register of info about the
                                      user defined bytes of the key */
    GT_U8   tmpFieldVal[4];
    GT_U8   value1 = 1;
    GT_U8   value0 = 0;
    CHT_PCL_USER_DEFINED_CONFIG_REG_STC *pclUserDefConfigPtr;


    /* get pointer to user defined bytes info */
    userDefinedInfoMemPtr = smemMemGet(devObjPtr,0x0B800020);

    snetChtPclKeyFieldBuildByValue(pclKeyPtr, value1,
                            CHT_PCL_KEY_FIELDS_ID_VALID_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, currPclLookupConfigPtr->pclId,
                            CHT_PCL_KEY_FIELDS_ID_PCL_ID_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->localDevSrcPort,
                            CHT_PCL_KEY_FIELDS_ID_SRC_PORT_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->origSrcTagged,
                            CHT_PCL_KEY_FIELDS_ID_IS_TAGGET_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->eVid,
                            CHT_PCL_KEY_FIELDS_ID_VID_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->up,
                            CHT_PCL_KEY_FIELDS_ID_UP_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->qos.qosProfile,
                            CHT_PCL_KEY_FIELDS_ID_QOS_PROFILE_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->isIPv4,
                            CHT_PCL_KEY_FIELDS_ID_IS_IPV4_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->isIp,
                            CHT_PCL_KEY_FIELDS_ID_IS_IP_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->ipProt,
                            CHT_PCL_KEY_FIELDS_ID_IP_PROTOCOL_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dscp,
                            CHT_PCL_KEY_FIELDS_ID_PACKET_DSCP_E,cycleNum);

    snetChtPclKeyFieldBuildByValue(pclKeyPtr, pclCookiePtr->isL4Valid,
                            CHT_PCL_KEY_FIELDS_ID_IS_L4_VALID_E,cycleNum);

    if (descrPtr->l4StartOffsetPtr != NULL)
    {
        snetChtPclKeyFieldBuildByPointer(
            pclKeyPtr,(GT_U8*)&(descrPtr->l4StartOffsetPtr[2]),
            CHT_PCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_2_3_E,cycleNum);
    }
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->arp,
                            CHT_PCL_KEY_FIELDS_ID_IS_ARP_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, pclCookiePtr->isIpV6EhExists,
                            CHT_PCL_KEY_FIELDS_ID_IS_IPV6_EH_EXISTS_E,cycleNum);


    /* user defined byte 0 */
    pclUserDefConfigPtr = &chtPclUserDefinedConfigInfoShortL3KeyArray[0];
    userDefinedAnchor = SMEM_U32_GET_FIELD(userDefinedInfoMemPtr[pclUserDefConfigPtr->regOffsetAddress],
                                         pclUserDefConfigPtr->startBitAnchor,2);
    userDefinedOffset = SMEM_U32_GET_FIELD(userDefinedInfoMemPtr[pclUserDefConfigPtr->regOffsetAddress],
                                         pclUserDefConfigPtr->startBitOffset,7);

    rc = snetChtPclUserDefinedByteGet(devObjPtr,descrPtr,userDefinedAnchor,
                                userDefinedOffset,&byteValue);
    if(rc != GT_OK)
    {
        didUserDefinedError = GT_TRUE;
        byteValue = 0;
    }

    snetChtPclKeyFieldBuildByValue(pclKeyPtr, byteValue,
                        CHT_PCL_KEY_FIELDS_ID_USER_DEFINED_BYTE0_SHORT_E,cycleNum);


    /* user defined byte 1 */
    pclUserDefConfigPtr = &chtPclUserDefinedConfigInfoShortL3KeyArray[1];
    userDefinedAnchor = SMEM_U32_GET_FIELD(userDefinedInfoMemPtr[pclUserDefConfigPtr->regOffsetAddress],
                                         pclUserDefConfigPtr->startBitAnchor,2);
    userDefinedOffset = SMEM_U32_GET_FIELD(userDefinedInfoMemPtr[pclUserDefConfigPtr->regOffsetAddress],
                                         pclUserDefConfigPtr->startBitOffset,7);

    rc = snetChtPclUserDefinedByteGet(devObjPtr,descrPtr,userDefinedAnchor,
                                userDefinedOffset,&byteValue);
    if(rc != GT_OK)
    {
        didUserDefinedError = GT_TRUE;
        byteValue = 0;
    }

    snetChtPclKeyFieldBuildByValue(pclKeyPtr, byteValue,
                        CHT_PCL_KEY_FIELDS_ID_USER_DEFINED_BYTE1_SHORT_E,cycleNum);

    /* end of user defined bytes */

    snetChtPclKeyFieldBuildByValue(pclKeyPtr, pclCookiePtr->isIpV6EhHopByHop,
                            CHT_PCL_KEY_FIELDS_ID_IS_IPV6_EH_HOP_BY_HOP_E,cycleNum);

    snetChtPclKeyFieldBuildByPointer(pclKeyPtr,descrPtr->macSaPtr,
                        CHT_PCL_KEY_FIELDS_ID_MAC_SA_E,cycleNum);
    snetChtPclKeyFieldBuildByPointer(pclKeyPtr,descrPtr->macDaPtr,
                        CHT_PCL_KEY_FIELDS_ID_MAC_DA_E,cycleNum);

    tmpFieldVal[0] = (didUserDefinedError == GT_TRUE) ? 0 : 1;
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, tmpFieldVal[0],
                        CHT_PCL_KEY_FIELDS_ID_USER_DEFINED_VALID_E,cycleNum);

    snetChtPclKeyFieldBuildByValue(pclKeyPtr, value0,
                        CHT_PCL_KEY_FIELDS_ID_RESERVED_189_E,cycleNum);

    snetChtPclKeyFieldBuildByValue(pclKeyPtr, pclCookiePtr->isIpv4Fragment,
                        CHT_PCL_KEY_FIELDS_ID_IPV4_FRAGMENT_E,cycleNum);

    snetChtPclKeyFieldBuildByValue(pclKeyPtr, pclCookiePtr->isIpHeaderOk,
                        CHT_PCL_KEY_FIELDS_ID_HEADER_OK_E,cycleNum);

    return;
}

/*******************************************************************************
*   snetChtPclKeyShortL3L4Build
*
* DESCRIPTION:
*       function build key for CHT_PCL_KEY_TYPE_L3_AND_L4_KEY_SHORT_E
*
* INPUTS:
*       devObjPtr    - (pointer to) device object.
*       descrPtr     - (pointer to) frame data buffer Id
*       cycleNum -  the number of current cycle
*       currPclLookupConfigPtr - info (structure) on the lookup config on
*                                the current pclId (current cycle)
*       pclKeyPtr - (pointer to) last pcl key
*       pclCookiePtr - (pointer to) pcl extra info
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
static GT_VOID snetChtPclKeyShortL3L4Build
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_U32  cycleNum,
    IN SNET_CHT_POLICY_SPECIFIC_LOOKUP_CONFIG_STC *currPclLookupConfigPtr,
    INOUT SNET_CHT_POLICY_KEY_STC *pclKeyPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC *pclCookiePtr
)
{
    GT_STATUS   rc;/* status from called function */
    GT_U32  userDefinedAnchor;/* user defined byte Anchor */
    GT_U32  userDefinedOffset;/* user defined byte offset from Anchor */
    GT_U8   byteValue;/* value of user defined byte from the packets */
    GT_BOOL didUserDefinedError = GT_FALSE;/* is there user defined byte error */
    GT_U32  *userDefinedInfoMemPtr; /*pointer to first register of info about the
                                      user defined bytes of the key */
    GT_U8   tmpFieldVal[4];
    GT_U8   value1 = 1;
    GT_U8   value0 = 0;
    CHT_PCL_USER_DEFINED_CONFIG_REG_STC *pclUserDefConfigPtr;

    /* get pointer to user defined bytes info */
    userDefinedInfoMemPtr = smemMemGet(devObjPtr,0x0B800020);

    snetChtPclKeyFieldBuildByValue(pclKeyPtr, value1,
                            CHT_PCL_KEY_FIELDS_ID_VALID_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, currPclLookupConfigPtr->pclId,
                            CHT_PCL_KEY_FIELDS_ID_PCL_ID_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->localDevSrcPort,
                            CHT_PCL_KEY_FIELDS_ID_SRC_PORT_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->origSrcTagged,
                            CHT_PCL_KEY_FIELDS_ID_IS_TAGGET_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->eVid,
                            CHT_PCL_KEY_FIELDS_ID_VID_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->up,
                            CHT_PCL_KEY_FIELDS_ID_UP_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->qos.qosProfile,
                            CHT_PCL_KEY_FIELDS_ID_QOS_PROFILE_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->isIPv4,
                            CHT_PCL_KEY_FIELDS_ID_IS_IPV4_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->isIp,
                            CHT_PCL_KEY_FIELDS_ID_IS_IP_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->ipProt,
                            CHT_PCL_KEY_FIELDS_ID_IP_PROTOCOL_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dscp,
                            CHT_PCL_KEY_FIELDS_ID_PACKET_DSCP_E,cycleNum);

    snetChtPclKeyFieldBuildByValue(pclKeyPtr, pclCookiePtr->isL4Valid,
                            CHT_PCL_KEY_FIELDS_ID_IS_L4_VALID_E,cycleNum);

    if (descrPtr->l4StartOffsetPtr != NULL)
    {
        snetChtPclKeyFieldBuildByPointer(
            pclKeyPtr,(GT_U8*)&(descrPtr->l4StartOffsetPtr[2]),
            CHT_PCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_2_3_E,cycleNum);
    }
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->arp,
                            CHT_PCL_KEY_FIELDS_ID_IS_ARP_E,cycleNum);
    tmpFieldVal[0] = (descrPtr->macDaType == SKERNEL_BROADCAST_MAC_E ||
                      descrPtr->macDaType == SKERNEL_BROADCAST_ARP_E) ? 1 : 0 ;
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, tmpFieldVal[0],
                            CHT_PCL_KEY_FIELDS_ID_IS_BC_E,cycleNum);


    /* user defined byte 0 */
    pclUserDefConfigPtr = &chtPclUserDefinedConfigInfoShortL4KeyArray[0];
    userDefinedAnchor = SMEM_U32_GET_FIELD(userDefinedInfoMemPtr[pclUserDefConfigPtr->regOffsetAddress],
                                         pclUserDefConfigPtr->startBitAnchor,2);
    userDefinedOffset = SMEM_U32_GET_FIELD(userDefinedInfoMemPtr[pclUserDefConfigPtr->regOffsetAddress],
                                         pclUserDefConfigPtr->startBitOffset,7);

    rc = snetChtPclUserDefinedByteGet(devObjPtr,descrPtr,userDefinedAnchor,
                                userDefinedOffset,&byteValue);
    if(rc != GT_OK)
    {
        didUserDefinedError = GT_TRUE;
        byteValue = 0;
    }

    snetChtPclKeyFieldBuildByValue(pclKeyPtr, byteValue,
                        CHT_PCL_KEY_FIELDS_ID_USER_DEFINED_BYTE0_SHORT_E,cycleNum);


    /* user defined byte 1 */
    pclUserDefConfigPtr = &chtPclUserDefinedConfigInfoShortL4KeyArray[1];
    userDefinedAnchor = SMEM_U32_GET_FIELD(userDefinedInfoMemPtr[pclUserDefConfigPtr->regOffsetAddress],
                                         pclUserDefConfigPtr->startBitAnchor,2);
    userDefinedOffset = SMEM_U32_GET_FIELD(userDefinedInfoMemPtr[pclUserDefConfigPtr->regOffsetAddress],
                                         pclUserDefConfigPtr->startBitOffset,7);

    rc = snetChtPclUserDefinedByteGet(devObjPtr,descrPtr,userDefinedAnchor,
                                userDefinedOffset,&byteValue);
    if(rc != GT_OK)
    {
        didUserDefinedError = GT_TRUE;
        byteValue = 0;
    }

    snetChtPclKeyFieldBuildByValue(pclKeyPtr, byteValue,
                        CHT_PCL_KEY_FIELDS_ID_USER_DEFINED_BYTE1_SHORT_E,cycleNum);

    /* user defined byte 2 */
    pclUserDefConfigPtr = &chtPclUserDefinedConfigInfoShortL4KeyArray[2];
    userDefinedAnchor = SMEM_U32_GET_FIELD(userDefinedInfoMemPtr[pclUserDefConfigPtr->regOffsetAddress],
                                         pclUserDefConfigPtr->startBitAnchor,2);
    userDefinedOffset = SMEM_U32_GET_FIELD(userDefinedInfoMemPtr[pclUserDefConfigPtr->regOffsetAddress],
                                         pclUserDefConfigPtr->startBitOffset,7);

    rc = snetChtPclUserDefinedByteGet(devObjPtr,descrPtr,userDefinedAnchor,
                                userDefinedOffset,&byteValue);
    if(rc != GT_OK)
    {
        didUserDefinedError = GT_TRUE;
        byteValue = 0;
    }

    snetChtPclKeyFieldBuildByValue(pclKeyPtr, byteValue,
                        CHT_PCL_KEY_FIELDS_ID_USER_DEFINED_BYTE2_L3_AND_L4_KEY_SHORT_E,cycleNum);

    /* end of user defined bytes */

    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[0],
                            CHT_PCL_KEY_FIELDS_ID_SIP_31_0_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[0],
                            CHT_PCL_KEY_FIELDS_ID_DIP_31_0_E,cycleNum);

    if (descrPtr->l4StartOffsetPtr != NULL)
    {
        tmpFieldVal[0] = descrPtr->l4StartOffsetPtr[0];
        tmpFieldVal[1] = descrPtr->l4StartOffsetPtr[1];
        tmpFieldVal[2] = descrPtr->l4StartOffsetPtr[13];
        snetChtPclKeyFieldBuildByPointer(
            pclKeyPtr,(GT_U8*)&tmpFieldVal[0],
            CHT_PCL_KEY_FIELDS_ID_L4_BYTES_OFFSETS_0_1_13_E,cycleNum);
    }

    snetChtPclKeyFieldBuildByValue(pclKeyPtr, value0,
                        CHT_PCL_KEY_FIELDS_ID_RESERVED_187_E,cycleNum);

    tmpFieldVal[0] = (didUserDefinedError == GT_TRUE) ? 0 : 1;
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, tmpFieldVal[0],
                        CHT_PCL_KEY_FIELDS_ID_USER_DEFINED_VALID_E,cycleNum);

    snetChtPclKeyFieldBuildByValue(pclKeyPtr, value0,
                        CHT_PCL_KEY_FIELDS_ID_RESERVED_189_E,cycleNum);

    snetChtPclKeyFieldBuildByValue(pclKeyPtr, pclCookiePtr->isIpv4Fragment,
                        CHT_PCL_KEY_FIELDS_ID_IPV4_FRAGMENT_E,cycleNum);

    snetChtPclKeyFieldBuildByValue(pclKeyPtr, pclCookiePtr->isIpHeaderOk,
                        CHT_PCL_KEY_FIELDS_ID_HEADER_OK_E,cycleNum);

    return;
}

/*******************************************************************************
*   snetChtPclKeyLongL2L3L4Build
*
* DESCRIPTION:
*       function build key for CHT_PCL_KEY_TYPE_L2_AND_L3_AND_L4_KEY_LONG_E
*
* INPUTS:
*       devObjPtr    - (pointer to) device object.
*       descrPtr     - (pointer to) frame data buffer Id
*       cycleNum -  the number of current cycle
*       currPclLookupConfigPtr - info (structure) on the lookup config on
*                                the current pclId (current cycle)
*       pclKeyPtr - (pointer to) last pcl key
*       pclCookiePtr - (pointer to) pcl extra info
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
static GT_VOID snetChtPclKeyLongL2L3L4Build
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_U32  cycleNum,
    IN SNET_CHT_POLICY_SPECIFIC_LOOKUP_CONFIG_STC *currPclLookupConfigPtr,
    INOUT SNET_CHT_POLICY_KEY_STC *pclKeyPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC *pclCookiePtr
)
{
    DECLARE_FUNC_NAME(snetChtPclKeyLongL2L3L4Build);

    GT_STATUS   rc;/* status from called function */
    GT_U32  userDefinedAnchor;/* user defined byte Anchor */
    GT_U32  userDefinedOffset;/* user defined byte offset from Anchor */
    GT_U8   byteValue;/* value of user defined byte from the packets */
    GT_BOOL didUserDefinedError = GT_FALSE;/* is there user defined byte error */
    GT_U32  *userDefinedInfoMemPtr; /*pointer to first register of info about the
                                      user defined bytes of the key */
    GT_U8   tmpFieldVal[7];
    GT_U8   value1 = 1;
    GT_U8   value0 = 0;
    CHT_PCL_USER_DEFINED_CONFIG_REG_STC *pclUserDefConfigPtr;

    /* get pointer to user defined bytes info */
    userDefinedInfoMemPtr = smemMemGet(devObjPtr,0x0B800020);

    snetChtPclKeyFieldBuildByValue(pclKeyPtr, value1,
                            CHT_PCL_KEY_FIELDS_ID_VALID_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, currPclLookupConfigPtr->pclId,
                            CHT_PCL_KEY_FIELDS_ID_PCL_ID_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->localDevSrcPort,
                            CHT_PCL_KEY_FIELDS_ID_SRC_PORT_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->origSrcTagged,
                            CHT_PCL_KEY_FIELDS_ID_IS_TAGGET_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->eVid,
                            CHT_PCL_KEY_FIELDS_ID_VID_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->up,
                            CHT_PCL_KEY_FIELDS_ID_UP_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->qos.qosProfile,
                            CHT_PCL_KEY_FIELDS_ID_QOS_PROFILE_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, (descrPtr->isIPv4==0 && descrPtr->isIp),
                            CHT_PCL_KEY_FIELDS_ID_IS_IPV6_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->isIp,
                            CHT_PCL_KEY_FIELDS_ID_IS_IP_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->ipProt,
                            CHT_PCL_KEY_FIELDS_ID_IP_PROTOCOL_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dscp,
                            CHT_PCL_KEY_FIELDS_ID_PACKET_DSCP_E,cycleNum);

    snetChtPclKeyFieldBuildByValue(pclKeyPtr, pclCookiePtr->isL4Valid,
                            CHT_PCL_KEY_FIELDS_ID_IS_L4_VALID_E,cycleNum);

    if (descrPtr->l4StartOffsetPtr != NULL)
    {
        snetChtPclKeyFieldBuildByPointer(
            pclKeyPtr,(GT_U8*)&(descrPtr->l4StartOffsetPtr[2]),
            CHT_PCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_2_3_E,cycleNum);

        tmpFieldVal[0] = descrPtr->l4StartOffsetPtr[0];
        tmpFieldVal[1] = descrPtr->l4StartOffsetPtr[1];
        tmpFieldVal[2] = descrPtr->l4StartOffsetPtr[13];
        snetChtPclKeyFieldBuildByPointer(
            pclKeyPtr,(GT_U8*)&tmpFieldVal[0],
            CHT_PCL_KEY_FIELDS_ID_LONG_L4_BYTES_OFFSETS_0_1_13_E,cycleNum);
    }

    if (descrPtr->isIPv4 != 0)
    {
        /* IPv4 */
        __LOG(("IPv4"));
        snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[0],
            CHT_PCL_KEY_FIELDS_ID_LONG_SIP_31_0_E,cycleNum);
        snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[0],
            CHT_PCL_KEY_FIELDS_ID_LONG_DIP_31_0_E,cycleNum);
    }
    else
    {
        /* IPv6 and ETH */
        __LOG(("IPv6 and ETH"));
        snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[3],
            CHT_PCL_KEY_FIELDS_ID_LONG_SIP_31_0_E,cycleNum);
        snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[3],
            CHT_PCL_KEY_FIELDS_ID_LONG_DIP_31_0_E,cycleNum);
    }

    tmpFieldVal[0] = (descrPtr->l2Encaps == SKERNEL_ETHERNET_II_E) ? 1 : 0;
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, tmpFieldVal[0],
                        CHT_PCL_KEY_FIELDS_ID_LONG_IS_ETH_TYPE_VALID_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->etherTypeOrSsapDsap,
                        CHT_PCL_KEY_FIELDS_ID_LONG_ETHER_TYPE_DSAP_SSAP_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, pclCookiePtr->isIpv4Fragment,
                        CHT_PCL_KEY_FIELDS_ID_LONG_IPV4_FRAGMENT_E,cycleNum);

    /* 7 bytes reserved */
    tmpFieldVal[0] = tmpFieldVal[1] = tmpFieldVal[2] = tmpFieldVal[3] =
                     tmpFieldVal[4] = tmpFieldVal[5] = tmpFieldVal[6] = 0 ;

    snetChtPclKeyFieldBuildByPointer(pclKeyPtr,descrPtr->macSaPtr,
                        CHT_PCL_KEY_FIELDS_ID_LONG_MAC_SA_E,cycleNum);
    snetChtPclKeyFieldBuildByPointer(pclKeyPtr,descrPtr->macDaPtr,
                        CHT_PCL_KEY_FIELDS_ID_LONG_MAC_DA_E,cycleNum);

    /* user defined byte 3 */
    pclUserDefConfigPtr = &chtPclUserDefinedConfigInfoLongL2KeyArray[3];
    userDefinedAnchor = SMEM_U32_GET_FIELD(userDefinedInfoMemPtr[pclUserDefConfigPtr->regOffsetAddress],
                                         pclUserDefConfigPtr->startBitAnchor,2);
    userDefinedOffset = SMEM_U32_GET_FIELD(userDefinedInfoMemPtr[pclUserDefConfigPtr->regOffsetAddress],
                                         pclUserDefConfigPtr->startBitOffset,7);

    rc = snetChtPclUserDefinedByteGet(devObjPtr,descrPtr,userDefinedAnchor,
                                userDefinedOffset,&byteValue);
    if(rc != GT_OK)
    {
        didUserDefinedError = GT_TRUE;
        byteValue = 0;
    }

    snetChtPclKeyFieldBuildByValue(pclKeyPtr, byteValue,
                        CHT_PCL_KEY_FIELDS_ID_LONG_USER_DEFINED_BYTE3_E,cycleNum);

    /* user defined byte 4 */
    pclUserDefConfigPtr = &chtPclUserDefinedConfigInfoLongL2KeyArray[4];
    userDefinedAnchor = SMEM_U32_GET_FIELD(userDefinedInfoMemPtr[pclUserDefConfigPtr->regOffsetAddress],
                                         pclUserDefConfigPtr->startBitAnchor,2);
    userDefinedOffset = SMEM_U32_GET_FIELD(userDefinedInfoMemPtr[pclUserDefConfigPtr->regOffsetAddress],
                                         pclUserDefConfigPtr->startBitOffset,7);

    rc = snetChtPclUserDefinedByteGet(devObjPtr,descrPtr,userDefinedAnchor,
                                userDefinedOffset,&byteValue);
    if(rc != GT_OK)
    {
        didUserDefinedError = GT_TRUE;
        byteValue = 0;
    }

    snetChtPclKeyFieldBuildByValue(pclKeyPtr, byteValue,
                        CHT_PCL_KEY_FIELDS_ID_LONG_USER_DEFINED_BYTE4_E,cycleNum);


    /* user defined byte 5 */
    pclUserDefConfigPtr = &chtPclUserDefinedConfigInfoLongL2KeyArray[5];
    userDefinedAnchor = SMEM_U32_GET_FIELD(userDefinedInfoMemPtr[pclUserDefConfigPtr->regOffsetAddress],
                                         pclUserDefConfigPtr->startBitAnchor,2);
    userDefinedOffset = SMEM_U32_GET_FIELD(userDefinedInfoMemPtr[pclUserDefConfigPtr->regOffsetAddress],
                                         pclUserDefConfigPtr->startBitOffset,7);

    rc = snetChtPclUserDefinedByteGet(devObjPtr,descrPtr,userDefinedAnchor,
                                userDefinedOffset,&byteValue);
    if(rc != GT_OK)
    {
        didUserDefinedError = GT_TRUE;
        byteValue = 0;
    }

    snetChtPclKeyFieldBuildByValue(pclKeyPtr, byteValue,
                        CHT_PCL_KEY_FIELDS_ID_LONG_USER_DEFINED_BYTE5_E,cycleNum);


    /* user defined byte 0 */
    pclUserDefConfigPtr = &chtPclUserDefinedConfigInfoLongL2KeyArray[0];
    userDefinedAnchor = SMEM_U32_GET_FIELD(userDefinedInfoMemPtr[pclUserDefConfigPtr->regOffsetAddress],
                                         pclUserDefConfigPtr->startBitAnchor,2);
    userDefinedOffset = SMEM_U32_GET_FIELD(userDefinedInfoMemPtr[pclUserDefConfigPtr->regOffsetAddress],
                                         pclUserDefConfigPtr->startBitOffset,7);

    rc = snetChtPclUserDefinedByteGet(devObjPtr,descrPtr,userDefinedAnchor,
                                userDefinedOffset,&byteValue);
    if(rc != GT_OK)
    {
        didUserDefinedError = GT_TRUE;
        byteValue = 0;
    }

    snetChtPclKeyFieldBuildByValue(pclKeyPtr, byteValue,
                        CHT_PCL_KEY_FIELDS_ID_LONG_USER_DEFINED_BYTE0_E,cycleNum);


    /* user defined byte 1 */
    pclUserDefConfigPtr = &chtPclUserDefinedConfigInfoLongL2KeyArray[1];
    userDefinedAnchor = SMEM_U32_GET_FIELD(userDefinedInfoMemPtr[pclUserDefConfigPtr->regOffsetAddress],
                                         pclUserDefConfigPtr->startBitAnchor,2);
    userDefinedOffset = SMEM_U32_GET_FIELD(userDefinedInfoMemPtr[pclUserDefConfigPtr->regOffsetAddress],
                                         pclUserDefConfigPtr->startBitOffset,7);

    rc = snetChtPclUserDefinedByteGet(devObjPtr,descrPtr,userDefinedAnchor,
                                userDefinedOffset,&byteValue);
    if(rc != GT_OK)
    {
        didUserDefinedError = GT_TRUE;
        byteValue = 0;
    }

    snetChtPclKeyFieldBuildByValue(pclKeyPtr, byteValue,
                        CHT_PCL_KEY_FIELDS_ID_LONG_USER_DEFINED_BYTE1_E,cycleNum);

    /* user defined byte 2 */
    pclUserDefConfigPtr = &chtPclUserDefinedConfigInfoLongL2KeyArray[2];
    userDefinedAnchor = SMEM_U32_GET_FIELD(userDefinedInfoMemPtr[pclUserDefConfigPtr->regOffsetAddress],
                                         pclUserDefConfigPtr->startBitAnchor,2);
    userDefinedOffset = SMEM_U32_GET_FIELD(userDefinedInfoMemPtr[pclUserDefConfigPtr->regOffsetAddress],
                                         pclUserDefConfigPtr->startBitOffset,7);

    rc = snetChtPclUserDefinedByteGet(devObjPtr,descrPtr,userDefinedAnchor,
                                userDefinedOffset,&byteValue);
    if(rc != GT_OK)
    {
        didUserDefinedError = GT_TRUE;
        byteValue = 0;
    }

    snetChtPclKeyFieldBuildByValue(pclKeyPtr, byteValue,
                        CHT_PCL_KEY_FIELDS_ID_LONG_USER_DEFINED_BYTE2_E,cycleNum);

    /* end of user defined bytes */


    snetChtPclKeyFieldBuildByValue(pclKeyPtr, value0,
                        CHT_PCL_KEY_FIELDS_ID_LONG_RESERVED_379_381_E,cycleNum);

    tmpFieldVal[0] = (didUserDefinedError == GT_TRUE) ? 0 : 1;
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, tmpFieldVal[0],
                        CHT_PCL_KEY_FIELDS_ID_LONG_USER_DEFINED_VALID_E,cycleNum);

    snetChtPclKeyFieldBuildByValue(pclKeyPtr, pclCookiePtr->isIpHeaderOk,
                        CHT_PCL_KEY_FIELDS_ID_LONG_HEADER_OK_E,cycleNum);

    return;
}

/*******************************************************************************
*   snetChtPclKeyLongL2L3Build
*
* DESCRIPTION:
*       function build key for CHT_PCL_KEY_TYPE_L2_AND_L3_KEY_LONG_E
*
* INPUTS:
*       devObjPtr    - (pointer to) device object.
*       descrPtr     - (pointer to) frame data buffer Id
*       cycleNum -  the number of current cycle
*       currPclLookupConfigPtr - info (structure) on the lookup config on
*                                the current pclId (current cycle)
*       pclKeyPtr - (pointer to) last pcl key
*       pclCookiePtr - (pointer to) pcl extra info
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
static GT_VOID snetChtPclKeyLongL2L3Build
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_U32  cycleNum,
    IN SNET_CHT_POLICY_SPECIFIC_LOOKUP_CONFIG_STC *currPclLookupConfigPtr,
    INOUT SNET_CHT_POLICY_KEY_STC *pclKeyPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC *pclCookiePtr
)
{
    GT_STATUS   rc;/* status from called function */
    GT_U32  userDefinedAnchor;/* user defined byte Anchor */
    GT_U32  userDefinedOffset;/* user defined byte offset from Anchor */
    GT_U8   byteValue;/* value of user defined byte from the packets */
    GT_BOOL didUserDefinedError = GT_FALSE;/* is there user defined byte error */
    GT_U32  *userDefinedInfoMemPtr; /*pointer to first register of info about the
                                      user defined bytes of the key */
    GT_U8   tmpFieldVal[7];
    GT_U8   value1 = 1;
    GT_U8   value0 = 0;
    CHT_PCL_USER_DEFINED_CONFIG_REG_STC *pclUserDefConfigPtr;

    /* get pointer to user defined bytes info */
    userDefinedInfoMemPtr = smemMemGet(devObjPtr,0x0B800020);

    snetChtPclKeyFieldBuildByValue(pclKeyPtr, value1,
                            CHT_PCL_KEY_FIELDS_ID_VALID_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, currPclLookupConfigPtr->pclId,
                            CHT_PCL_KEY_FIELDS_ID_PCL_ID_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->localDevSrcPort,
                            CHT_PCL_KEY_FIELDS_ID_SRC_PORT_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->origSrcTagged,
                            CHT_PCL_KEY_FIELDS_ID_IS_TAGGET_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->eVid,
                            CHT_PCL_KEY_FIELDS_ID_VID_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->up,
                            CHT_PCL_KEY_FIELDS_ID_UP_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->qos.qosProfile,
                            CHT_PCL_KEY_FIELDS_ID_QOS_PROFILE_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, (descrPtr->isIPv4==0 && descrPtr->isIp),
                            CHT_PCL_KEY_FIELDS_ID_IS_IPV6_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->isIp,
                            CHT_PCL_KEY_FIELDS_ID_IS_IP_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->ipProt,
                            CHT_PCL_KEY_FIELDS_ID_IP_PROTOCOL_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dscp,
                            CHT_PCL_KEY_FIELDS_ID_PACKET_DSCP_E,cycleNum);

    snetChtPclKeyFieldBuildByValue(pclKeyPtr, pclCookiePtr->isL4Valid,
                            CHT_PCL_KEY_FIELDS_ID_IS_L4_VALID_E,cycleNum);

    if (descrPtr->l4StartOffsetPtr != NULL)
    {
        snetChtPclKeyFieldBuildByPointer(
            pclKeyPtr,(GT_U8*)&(descrPtr->l4StartOffsetPtr[2]),
            CHT_PCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_2_3_E,cycleNum);

        tmpFieldVal[0] = descrPtr->l4StartOffsetPtr[0];
        tmpFieldVal[1] = descrPtr->l4StartOffsetPtr[1];
        tmpFieldVal[2] = descrPtr->l4StartOffsetPtr[13];
        snetChtPclKeyFieldBuildByPointer(
            pclKeyPtr,(GT_U8*)&tmpFieldVal[0],
            CHT_PCL_KEY_FIELDS_ID_LONG_L4_BYTES_OFFSETS_0_1_13_E,cycleNum);
    }


    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[3],
                            CHT_PCL_KEY_FIELDS_ID_LONG_SIP_31_0_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[2],
                            CHT_PCL_KEY_FIELDS_ID_LONG_SIP_63_32_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[1],
                            CHT_PCL_KEY_FIELDS_ID_LONG_SIP_95_64_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[0],
                            CHT_PCL_KEY_FIELDS_ID_LONG_SIP_127_96_E,cycleNum);

    snetChtPclKeyFieldBuildByValue(pclKeyPtr, pclCookiePtr->isIpV6EhExists,
                            CHT_PCL_KEY_FIELDS_ID_IS_IPV6_EH_EXISTS_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, pclCookiePtr->isIpV6EhHopByHop,
        CHT_PCL_KEY_FIELDS_ID_LONG_IS_IPV6_EH_HOP_BY_HOP_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, (descrPtr->dip[0] >> 24),
                            CHT_PCL_KEY_FIELDS_ID_LONG_DIP_127_120_E,cycleNum);

    snetChtPclKeyFieldBuildByPointer(pclKeyPtr,descrPtr->macSaPtr,
                        CHT_PCL_KEY_FIELDS_ID_LONG_MAC_SA_E,cycleNum);
    snetChtPclKeyFieldBuildByPointer(pclKeyPtr,descrPtr->macDaPtr,
                        CHT_PCL_KEY_FIELDS_ID_LONG_MAC_DA_E,cycleNum);

    /* user defined byte 3 */
    pclUserDefConfigPtr = &chtPclUserDefinedConfigInfoLongL3KeyArray[3];
    userDefinedAnchor = SMEM_U32_GET_FIELD(userDefinedInfoMemPtr[pclUserDefConfigPtr->regOffsetAddress],
                                         pclUserDefConfigPtr->startBitAnchor,2);
    userDefinedOffset = SMEM_U32_GET_FIELD(userDefinedInfoMemPtr[pclUserDefConfigPtr->regOffsetAddress],
                                         pclUserDefConfigPtr->startBitOffset,7);

    rc = snetChtPclUserDefinedByteGet(devObjPtr,descrPtr,userDefinedAnchor,
                                userDefinedOffset,&byteValue);
    if(rc != GT_OK)
    {
        didUserDefinedError = GT_TRUE;
        byteValue = 0;
    }

    snetChtPclKeyFieldBuildByValue(pclKeyPtr, byteValue,
                        CHT_PCL_KEY_FIELDS_ID_LONG_USER_DEFINED_BYTE3_E,cycleNum);

    /* user defined byte 4 */
    pclUserDefConfigPtr = &chtPclUserDefinedConfigInfoLongL3KeyArray[4];
    userDefinedAnchor = SMEM_U32_GET_FIELD(userDefinedInfoMemPtr[pclUserDefConfigPtr->regOffsetAddress],
                                         pclUserDefConfigPtr->startBitAnchor,2);
    userDefinedOffset = SMEM_U32_GET_FIELD(userDefinedInfoMemPtr[pclUserDefConfigPtr->regOffsetAddress],
                                         pclUserDefConfigPtr->startBitOffset,7);

    rc = snetChtPclUserDefinedByteGet(devObjPtr,descrPtr,userDefinedAnchor,
                                userDefinedOffset,&byteValue);
    if(rc != GT_OK)
    {
        didUserDefinedError = GT_TRUE;
        byteValue = 0;
    }

    snetChtPclKeyFieldBuildByValue(pclKeyPtr, byteValue,
                        CHT_PCL_KEY_FIELDS_ID_LONG_USER_DEFINED_BYTE4_E,cycleNum);


    /* user defined byte 5 */
    pclUserDefConfigPtr = &chtPclUserDefinedConfigInfoLongL3KeyArray[5];
    userDefinedAnchor = SMEM_U32_GET_FIELD(userDefinedInfoMemPtr[pclUserDefConfigPtr->regOffsetAddress],
                                         pclUserDefConfigPtr->startBitAnchor,2);
    userDefinedOffset = SMEM_U32_GET_FIELD(userDefinedInfoMemPtr[pclUserDefConfigPtr->regOffsetAddress],
                                         pclUserDefConfigPtr->startBitOffset,7);

    rc = snetChtPclUserDefinedByteGet(devObjPtr,descrPtr,userDefinedAnchor,
                                userDefinedOffset,&byteValue);
    if(rc != GT_OK)
    {
        didUserDefinedError = GT_TRUE;
        byteValue = 0;
    }

    snetChtPclKeyFieldBuildByValue(pclKeyPtr, byteValue,
                        CHT_PCL_KEY_FIELDS_ID_LONG_USER_DEFINED_BYTE5_E,cycleNum);


    /* user defined byte 0 */
    pclUserDefConfigPtr = &chtPclUserDefinedConfigInfoLongL3KeyArray[0];
    userDefinedAnchor = SMEM_U32_GET_FIELD(userDefinedInfoMemPtr[pclUserDefConfigPtr->regOffsetAddress],
                                         pclUserDefConfigPtr->startBitAnchor,2);
    userDefinedOffset = SMEM_U32_GET_FIELD(userDefinedInfoMemPtr[pclUserDefConfigPtr->regOffsetAddress],
                                         pclUserDefConfigPtr->startBitOffset,7);

    rc = snetChtPclUserDefinedByteGet(devObjPtr,descrPtr,userDefinedAnchor,
                                userDefinedOffset,&byteValue);
    if(rc != GT_OK)
    {
        didUserDefinedError = GT_TRUE;
        byteValue = 0;
    }

    snetChtPclKeyFieldBuildByValue(pclKeyPtr, byteValue,
                        CHT_PCL_KEY_FIELDS_ID_LONG_USER_DEFINED_BYTE0_E,cycleNum);


    /* user defined byte 1 */
    pclUserDefConfigPtr = &chtPclUserDefinedConfigInfoLongL3KeyArray[1];
    userDefinedAnchor = SMEM_U32_GET_FIELD(userDefinedInfoMemPtr[pclUserDefConfigPtr->regOffsetAddress],
                                         pclUserDefConfigPtr->startBitAnchor,2);
    userDefinedOffset = SMEM_U32_GET_FIELD(userDefinedInfoMemPtr[pclUserDefConfigPtr->regOffsetAddress],
                                         pclUserDefConfigPtr->startBitOffset,7);

    rc = snetChtPclUserDefinedByteGet(devObjPtr,descrPtr,userDefinedAnchor,
                                userDefinedOffset,&byteValue);
    if(rc != GT_OK)
    {
        didUserDefinedError = GT_TRUE;
        byteValue = 0;
    }

    snetChtPclKeyFieldBuildByValue(pclKeyPtr, byteValue,
                        CHT_PCL_KEY_FIELDS_ID_LONG_USER_DEFINED_BYTE1_E,cycleNum);

    /* user defined byte 2 */
    pclUserDefConfigPtr = &chtPclUserDefinedConfigInfoLongL3KeyArray[2];
    userDefinedAnchor = SMEM_U32_GET_FIELD(userDefinedInfoMemPtr[pclUserDefConfigPtr->regOffsetAddress],
                                         pclUserDefConfigPtr->startBitAnchor,2);
    userDefinedOffset = SMEM_U32_GET_FIELD(userDefinedInfoMemPtr[pclUserDefConfigPtr->regOffsetAddress],
                                         pclUserDefConfigPtr->startBitOffset,7);

    rc = snetChtPclUserDefinedByteGet(devObjPtr,descrPtr,userDefinedAnchor,
                                userDefinedOffset,&byteValue);
    if(rc != GT_OK)
    {
        didUserDefinedError = GT_TRUE;
        byteValue = 0;
    }

    snetChtPclKeyFieldBuildByValue(pclKeyPtr, byteValue,
                        CHT_PCL_KEY_FIELDS_ID_LONG_USER_DEFINED_BYTE2_E,cycleNum);

    /* end of user defined bytes */


    snetChtPclKeyFieldBuildByValue(pclKeyPtr, value0,
                        CHT_PCL_KEY_FIELDS_ID_LONG_RESERVED_379_381_E,cycleNum);

    tmpFieldVal[0] = (didUserDefinedError == GT_TRUE) ? 0 : 1;
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, tmpFieldVal[0],
                        CHT_PCL_KEY_FIELDS_ID_LONG_USER_DEFINED_VALID_E,cycleNum);

    snetChtPclKeyFieldBuildByValue(pclKeyPtr, pclCookiePtr->isIpHeaderOk,
                        CHT_PCL_KEY_FIELDS_ID_LONG_HEADER_OK_E,cycleNum);

    return;
}


/*******************************************************************************
*   snetChtPclKeyLongL3L4Build
*
* DESCRIPTION:
*       function build key for CHT_PCL_KEY_TYPE_L3_AND_L4_KEY_LONG_E
*
* INPUTS:
*       devObjPtr    - (pointer to) device object.
*       descrPtr     - (pointer to) frame data buffer Id
*       cycleNum -  the number of current cycle
*       currPclLookupConfigPtr - info (structure) on the lookup config on
*                                the current pclId (current cycle)
*       pclKeyPtr - (pointer to) last pcl key
*       pclCookiePtr - (pointer to) pcl extra info
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
static GT_VOID snetChtPclKeyLongL3L4Build
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_U32  cycleNum,
    IN SNET_CHT_POLICY_SPECIFIC_LOOKUP_CONFIG_STC *currPclLookupConfigPtr,
    INOUT SNET_CHT_POLICY_KEY_STC *pclKeyPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC *pclCookiePtr
)
{
    GT_STATUS   rc;/* status from called function */
    GT_U32  userDefinedAnchor;/* user defined byte Anchor */
    GT_U32  userDefinedOffset;/* user defined byte offset from Anchor */
    GT_U8   byteValue;/* value of user defined byte from the packets */
    GT_BOOL didUserDefinedError = GT_FALSE;/* is there user defined byte error */
    GT_U32  *userDefinedInfoMemPtr; /*pointer to first register of info about the
                                      user defined bytes of the key */
    GT_U8   tmpFieldVal[7];
    GT_U8   value1 = 1;
    GT_U8   value0 = 0;
    CHT_PCL_USER_DEFINED_CONFIG_REG_STC *pclUserDefConfigPtr;

    /* get pointer to user defined bytes info */
    userDefinedInfoMemPtr = smemMemGet(devObjPtr,0x0B800020);

    snetChtPclKeyFieldBuildByValue(pclKeyPtr, value1,
                            CHT_PCL_KEY_FIELDS_ID_VALID_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, currPclLookupConfigPtr->pclId,
                            CHT_PCL_KEY_FIELDS_ID_PCL_ID_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->localDevSrcPort,
                            CHT_PCL_KEY_FIELDS_ID_SRC_PORT_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->origSrcTagged,
                            CHT_PCL_KEY_FIELDS_ID_IS_TAGGET_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->eVid,
                            CHT_PCL_KEY_FIELDS_ID_VID_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->up,
                            CHT_PCL_KEY_FIELDS_ID_UP_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->qos.qosProfile,
                            CHT_PCL_KEY_FIELDS_ID_QOS_PROFILE_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, (descrPtr->isIPv4==0 && descrPtr->isIp),
                            CHT_PCL_KEY_FIELDS_ID_IS_IPV6_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->isIp,
                            CHT_PCL_KEY_FIELDS_ID_IS_IP_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->ipProt,
                            CHT_PCL_KEY_FIELDS_ID_IP_PROTOCOL_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dscp,
                            CHT_PCL_KEY_FIELDS_ID_PACKET_DSCP_E,cycleNum);

    snetChtPclKeyFieldBuildByValue(pclKeyPtr, pclCookiePtr->isL4Valid,
                            CHT_PCL_KEY_FIELDS_ID_IS_L4_VALID_E,cycleNum);

    if (descrPtr->l4StartOffsetPtr != NULL)
    {
        snetChtPclKeyFieldBuildByPointer(
            pclKeyPtr,(GT_U8*)&(descrPtr->l4StartOffsetPtr[2]),
            CHT_PCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_2_3_E,cycleNum);

        tmpFieldVal[0] = descrPtr->l4StartOffsetPtr[0];
        tmpFieldVal[1] = descrPtr->l4StartOffsetPtr[1];
        tmpFieldVal[2] = descrPtr->l4StartOffsetPtr[13];
        snetChtPclKeyFieldBuildByPointer(
            pclKeyPtr, (GT_U8*)(&tmpFieldVal[0]),
            CHT_PCL_KEY_FIELDS_ID_LONG_L4_BYTES_OFFSETS_0_1_13_E,cycleNum);
    }

    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[3],
                            CHT_PCL_KEY_FIELDS_ID_LONG_SIP_31_0_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[2],
                            CHT_PCL_KEY_FIELDS_ID_LONG_SIP_63_32_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[1],
                            CHT_PCL_KEY_FIELDS_ID_LONG_SIP_95_64_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[0],
                            CHT_PCL_KEY_FIELDS_ID_LONG_SIP_127_96_E,cycleNum);

    snetChtPclKeyFieldBuildByValue(pclKeyPtr, pclCookiePtr->isIpV6EhExists,
                            CHT_PCL_KEY_FIELDS_ID_IS_IPV6_EH_EXISTS_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, pclCookiePtr->isIpV6EhHopByHop,
        CHT_PCL_KEY_FIELDS_ID_LONG_IS_IPV6_EH_HOP_BY_HOP_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, (descrPtr->dip[0] >> 24),
                            CHT_PCL_KEY_FIELDS_ID_LONG_DIP_127_120_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[2],
                            CHT_PCL_KEY_FIELDS_ID_LONG_DIP_63_32_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[1],
                            CHT_PCL_KEY_FIELDS_ID_LONG_DIP_95_64_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[0],
                            CHT_PCL_KEY_FIELDS_ID_LONG_DIP_119_96_E,cycleNum);
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[3],
                            CHT_PCL_KEY_FIELDS_ID_LONG_DIP_31_0_L3_AND_L4_KEY_LONG_E,cycleNum);

    /* user defined byte 0 */
    pclUserDefConfigPtr = &chtPclUserDefinedConfigInfoLongL4KeyArray[0];
    userDefinedAnchor = SMEM_U32_GET_FIELD(userDefinedInfoMemPtr[pclUserDefConfigPtr->regOffsetAddress],
                                         pclUserDefConfigPtr->startBitAnchor,2);
    userDefinedOffset = SMEM_U32_GET_FIELD(userDefinedInfoMemPtr[pclUserDefConfigPtr->regOffsetAddress],
                                         pclUserDefConfigPtr->startBitOffset,7);

    rc = snetChtPclUserDefinedByteGet(devObjPtr,descrPtr,userDefinedAnchor,
                                userDefinedOffset,&byteValue);
    if(rc != GT_OK)
    {
        didUserDefinedError = GT_TRUE;
        byteValue = 0;
    }

    snetChtPclKeyFieldBuildByValue(pclKeyPtr, byteValue,
                        CHT_PCL_KEY_FIELDS_ID_LONG_USER_DEFINED_BYTE0_E,cycleNum);


    /* user defined byte 1 */
    pclUserDefConfigPtr = &chtPclUserDefinedConfigInfoLongL4KeyArray[1];
    userDefinedAnchor = SMEM_U32_GET_FIELD(userDefinedInfoMemPtr[pclUserDefConfigPtr->regOffsetAddress],
                                         pclUserDefConfigPtr->startBitAnchor,2);
    userDefinedOffset = SMEM_U32_GET_FIELD(userDefinedInfoMemPtr[pclUserDefConfigPtr->regOffsetAddress],
                                         pclUserDefConfigPtr->startBitOffset,7);

    rc = snetChtPclUserDefinedByteGet(devObjPtr,descrPtr,userDefinedAnchor,
                                userDefinedOffset,&byteValue);
    if(rc != GT_OK)
    {
        didUserDefinedError = GT_TRUE;
        byteValue = 0;
    }

    snetChtPclKeyFieldBuildByValue(pclKeyPtr, byteValue,
                        CHT_PCL_KEY_FIELDS_ID_LONG_USER_DEFINED_BYTE1_E,cycleNum);

    /* user defined byte 2 */
    pclUserDefConfigPtr = &chtPclUserDefinedConfigInfoLongL4KeyArray[2];
    userDefinedAnchor = SMEM_U32_GET_FIELD(userDefinedInfoMemPtr[pclUserDefConfigPtr->regOffsetAddress],
                                         pclUserDefConfigPtr->startBitAnchor,2);
    userDefinedOffset = SMEM_U32_GET_FIELD(userDefinedInfoMemPtr[pclUserDefConfigPtr->regOffsetAddress],
                                         pclUserDefConfigPtr->startBitOffset,7);

    rc = snetChtPclUserDefinedByteGet(devObjPtr,descrPtr,userDefinedAnchor,
                                userDefinedOffset,&byteValue);
    if(rc != GT_OK)
    {
        didUserDefinedError = GT_TRUE;
        byteValue = 0;
    }

    snetChtPclKeyFieldBuildByValue(pclKeyPtr, byteValue,
                        CHT_PCL_KEY_FIELDS_ID_LONG_USER_DEFINED_BYTE2_E,cycleNum);

    /* end of user defined bytes */


    snetChtPclKeyFieldBuildByValue(pclKeyPtr, value0,
                        CHT_PCL_KEY_FIELDS_ID_LONG_RESERVED_379_381_E,cycleNum);

    tmpFieldVal[0] = (didUserDefinedError == GT_TRUE) ? 0 : 1;
    snetChtPclKeyFieldBuildByValue(pclKeyPtr, tmpFieldVal[0],
                        CHT_PCL_KEY_FIELDS_ID_LONG_USER_DEFINED_VALID_E,cycleNum);

    snetChtPclKeyFieldBuildByValue(pclKeyPtr, pclCookiePtr->isIpHeaderOk,
                        CHT_PCL_KEY_FIELDS_ID_LONG_HEADER_OK_E,cycleNum);

    return;
}


/*******************************************************************************
*   snetChtPclKeyCreate
*
* DESCRIPTION:
*       create current pcl search key
*
* INPUTS:
*       devObjPtr    - (pointer to) device object.
*       descrPtr     - (pointer to) frame data buffer Id
*       cycleNum -  the number of current cycle
*       pclIdMode    - pcl id mode  0 per port/trunk  , 1 per vlan
*       currPclLookupConfigPtr - info (structure) on the lookup config on
*                                the current pclId (current cycle)
*       pclCookiePtr - (pointer to) pcl extra info
* OUTPUTS:
*       pclKeyPtr - (pointer to) current pcl key
*       userDefinedParseErrorPtr - (pointer to) did error in key build due
*                                   to user defined bytes
*
* RETURN:
*
* COMMENTS:
*    [1] Policy Search Key (page 90)
*
*    [1]  C.12.12 Search Key User Defined Bytes -- page 487
*
*******************************************************************************/
static GT_VOID snetChtPclKeyCreate
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_U32  cycleNum,
    IN SNET_CHT_POLICY_SPECIFIC_LOOKUP_CONFIG_STC *currPclLookupConfigPtr,
    OUT SNET_CHT_POLICY_KEY_STC *pclKeyPtr,
    OUT SKERNEL_EXT_PACKET_CMD_ENT *userDefinedParseErrorPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC *pclCookiePtr
)
{
    DECLARE_FUNC_NAME(snetChtPclKeyCreate);

    CHT_PCL_KEY_TYPE_ENT    keyType;/* search key type */

    memset(&pclKeyPtr->key, 0, sizeof(pclKeyPtr->key));
    pclKeyPtr->devObjPtr = devObjPtr;
    pclKeyPtr->updateOnlyDiff = GT_FALSE;

    /* init with no error */
    *userDefinedParseErrorPtr = SKERNEL_EXT_PKT_CMD_FORWARD_E;

    if(pclKeyPtr->pclKeyFormat == CHT_PCL_KEY_EXTENDED_E)
    {
        /* 48 bytes key size */
        if(descrPtr->isIp == 1 && descrPtr->isIPv4 == 0)
        {
            /* ip v6*/
            __LOG(("ip v6"));
            if(currPclLookupConfigPtr->ipKeyType == 1)
            {
                keyType = CHT_PCL_KEY_TYPE_L3_AND_L4_KEY_LONG_E;
            }
            else
            {
                keyType = CHT_PCL_KEY_TYPE_L2_AND_L3_KEY_LONG_E;
            }
        }
        else
        {
            /* ip v4 or non-ip packets */
            __LOG(("ip v4 or non-ip packets"));
            keyType = CHT_PCL_KEY_TYPE_L2_AND_L3_AND_L4_KEY_LONG_E;
        }
    }
    else
    {
        /* 24 bytes key size */
        __LOG(("24 bytes key size"));
        if(descrPtr->isIp == 1)
        {
            if(descrPtr->isIPv4 == 1)
            {
                /* ip v4*/
                if(currPclLookupConfigPtr->ipKeyType == 1)
                {
                    keyType = CHT_PCL_KEY_TYPE_L3_AND_L4_KEY_SHORT_E;
                }
                else
                {
                    keyType = CHT_PCL_KEY_TYPE_L2_AND_L3_KEY_SHORT_E;
                }
            }
            else
            {
                if (cycleNum)
                {
                    if (currPclLookupConfigPtr->lookUp1IpV6KeyType == 1)
                    {
                        /*ipv6 dip key*/
                        __LOG(("ipv6 dip key"));
                        keyType = CHT_PCL_KEY_TYPE_L3_KEY_SHORT_E;
                    }
                    else
                    {
                        /* ip v6 */
                        keyType = CHT_PCL_KEY_TYPE_L2_AND_L3_KEY_SHORT_E;
                    }
                }
                else
                {
                    /* ip v6 */
                    keyType = CHT_PCL_KEY_TYPE_L2_AND_L3_KEY_SHORT_E;
                }
            }
        }
        else if(descrPtr->arp)
        {
            /* like Ipv4 packet */
            __LOG(("like Ipv4 packet"));
            if(currPclLookupConfigPtr->ipKeyType == 1)
            {
                keyType = CHT_PCL_KEY_TYPE_L3_AND_L4_KEY_SHORT_E;
            }
            else
            {
                keyType = CHT_PCL_KEY_TYPE_L2_AND_L3_KEY_SHORT_E;
            }
        }
        else
        {
            /* non ip packets */
            __LOG(("non ip packets"));
            keyType = CHT_PCL_KEY_TYPE_L2_KEY_SHORT_E;
        }
    }

    switch(keyType)
    {
        case CHT_PCL_KEY_TYPE_L2_KEY_SHORT_E:
            snetChtPclKeyShortL2Build(devObjPtr,descrPtr,cycleNum,currPclLookupConfigPtr,pclKeyPtr,pclCookiePtr);
            break;
        case CHT_PCL_KEY_TYPE_L2_AND_L3_KEY_SHORT_E:
            snetChtPclKeyShortL2L3Build(devObjPtr,descrPtr,cycleNum,currPclLookupConfigPtr,pclKeyPtr,pclCookiePtr);
            break;
        case CHT_PCL_KEY_TYPE_L3_AND_L4_KEY_SHORT_E:
            snetChtPclKeyShortL3L4Build(devObjPtr,descrPtr,cycleNum,currPclLookupConfigPtr,pclKeyPtr,pclCookiePtr);
            break;
        case CHT_PCL_KEY_TYPE_L2_AND_L3_AND_L4_KEY_LONG_E:
            snetChtPclKeyLongL2L3L4Build(devObjPtr,descrPtr,cycleNum,currPclLookupConfigPtr,pclKeyPtr,pclCookiePtr);
            break;
        case CHT_PCL_KEY_TYPE_L2_AND_L3_KEY_LONG_E:
            snetChtPclKeyLongL2L3Build(devObjPtr,descrPtr,cycleNum,currPclLookupConfigPtr,pclKeyPtr,pclCookiePtr);
            break;
        case CHT_PCL_KEY_TYPE_L3_AND_L4_KEY_LONG_E:
            snetChtPclKeyLongL3L4Build(devObjPtr,descrPtr,cycleNum,currPclLookupConfigPtr,pclKeyPtr,pclCookiePtr);
            break;
        case CHT_PCL_KEY_TYPE_L3_KEY_SHORT_E:
            snetChtPclKeyShortL3Build(devObjPtr,descrPtr,cycleNum,currPclLookupConfigPtr,pclKeyPtr,pclCookiePtr);
            break;
        default:
            skernelFatalError(" snetChtPclKeyCreate: not valid mode[%d]",keyType);
            break;

    }

    return;
}

/*******************************************************************************
*   snetChtPclLookupInfoGet
*
* DESCRIPTION:
*       Get the lookup info for the specific cycle
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - pointer to frame data buffer Id
*       cycleNum        - the number of current cycle
*       lookupConfigPtr - pointer to the lookup config entry
*       tcamModeSelect  - tcam select mode  (was taken from 0x0B800000 bit 8)
*       tcamModePtr     - pointer to tcam mode (was taken from 0x0B800000 bit 0)
*
* OUTPUTS:
*       extendedLookupModePtr - pointer to extended lookup mode (48 bytes)
*                               or regular (24 bytes)
*       currPclLookupConfigPtr - info (structure) on the lookup config on
*                                the current pclId (current cycle)
*
* RETURN:
*
* COMMENTS:
*    Policy Lookup configuration (page 86)
*
*******************************************************************************/
static GT_VOID snetChtPclLookupInfoGet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_U32  cycleNum,
    IN SNET_CHT_POLICY_LOOKUP_CONFIG_STC *lookupConfigPtr,
    IN GT_BIT  tcamModeSelect,
    IN GT_BIT  tcamMode,
    OUT GT_BOOL *extendedLookupModePtr,
    OUT SNET_CHT_POLICY_SPECIFIC_LOOKUP_CONFIG_STC *currPclLookupConfigPtr
)
{
    SNET_CHT_POLICY_SPECIFIC_LOOKUP_CONFIG_STC *tmpPclLookupConfigPtr;

    if(cycleNum == 0)
    {
        tmpPclLookupConfigPtr = &lookupConfigPtr->lkupConf_0;
    }
    else
    {
        tmpPclLookupConfigPtr = &lookupConfigPtr->lkupConf_1;
    }

    /* fill info for the specific cycle number */
    memcpy(currPclLookupConfigPtr,tmpPclLookupConfigPtr,
            sizeof(SNET_CHT_POLICY_SPECIFIC_LOOKUP_CONFIG_STC));


    if(tcamModeSelect == 0)
    {
        *extendedLookupModePtr = tcamMode ;
    }
    else
    {
        *extendedLookupModePtr =  tmpPclLookupConfigPtr->keySize ;
    }

    return;
}


/*******************************************************************************
*   snetChtPclConfigInfoGet
*
* DESCRIPTION:
*       get info on the pcl lookup configuration entry (on 2 lookups)
*
* INPUTS:
*       devObjPtr    - (pointer to) device object.
*       descrPtr     - (pointer to) frame data buffer Id
*       pclIdMode    - pcl id mode  0 per port/trunk  , 1 per vlan
* OUTPUTS:
*       lookupConfigPtr  (pointer to) the lookup config entry
* RETURN:
*
* COMMENTS:
*    [1] c.10.12 PCL-ID configuration Table (page 483)
*
*******************************************************************************/
static GT_VOID snetChtPclConfigInfoGet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_BIT   pclIdMode,
    OUT SNET_CHT_POLICY_LOOKUP_CONFIG_STC *lookupConfigPtr
)
{
    GT_BOOL     doPerPort;/* use "per port" info -- 1 per port  , 0 per vlan */
    GT_U32      index;/* index in the lookup configuration table */
    GT_U32      regVal;/*register value*/

    if(pclIdMode == 0 || descrPtr->eVid >= 1024)
    {
        doPerPort  = GT_TRUE ;
    }
    else
    {
        doPerPort = (descrPtr->pclLookUpMode[0] == SKERNEL_PCL_LOOKUP_MODE_PORT_E) ? GT_TRUE : GT_FALSE;
    }

    if(doPerPort == GT_TRUE)
    {
        if(pclIdMode == 1)/* Access PCL-ID config. table with local devId */
        {
            index = (1<<10) | descrPtr->localDevSrcPort;/*port 63 for cpu*/
        }
        else if(descrPtr->origIsTrunk == 0)
        {   /* Access PCL-ID config. table with ingress port */
            index =  (descrPtr->srcDev << 5) | descrPtr->origSrcEPortOrTrnk;
        }
        else
        {/* Access PCL-ID config. table with ingress trunk  */
            index = (1<<10) | descrPtr->origSrcEPortOrTrnk;
        }
    }
    else
    {/* Access PCL-ID config. table with vid  */
        index = descrPtr->eVid;
    }

    /* access the PCL-ID configuration Table (table 330 page 484)
       by the calculated index */

    /* fill lookupConfigPtr */
    smemRegGet(devObjPtr, SMEM_CHT_PCL_ID_TBL_MEM(devObjPtr, index) ,&regVal);

    lookupConfigPtr->valid = SMEM_U32_GET_FIELD(regVal,28,1);
    lookupConfigPtr->lkupConf_0.enabled  = SMEM_U32_GET_FIELD(regVal,0,1);
    lookupConfigPtr->lkupConf_0.keySize  = SMEM_U32_GET_FIELD(regVal,12,1);
    lookupConfigPtr->lkupConf_0.pclId  = SMEM_U32_GET_FIELD(regVal,2,10);
    lookupConfigPtr->lkupConf_0.ipKeyType = SMEM_U32_GET_FIELD(regVal,1,1);

    lookupConfigPtr->lkupConf_1.enabled  = SMEM_U32_GET_FIELD(regVal,14,1);
    lookupConfigPtr->lkupConf_1.keySize  = SMEM_U32_GET_FIELD(regVal,26,1);
    lookupConfigPtr->lkupConf_1.pclId  = SMEM_U32_GET_FIELD(regVal,16,10);
    lookupConfigPtr->lkupConf_1.ipKeyType = SMEM_U32_GET_FIELD(regVal,15,1);

    /* This field determines the short Key Type used in the 2nd lookup for IPv6
    In Cheetah A0, this was a reserved bit */
    lookupConfigPtr->lkupConf_1.lookUp1IpV6KeyType =
                                        SMEM_U32_GET_FIELD(regVal,27,1);

    return;
}


/*******************************************************************************
*   snetChtPclTriggeringCheck
*
* DESCRIPTION:
*       check if need to do Pcl , for packets from the ingress inlif
*
*
*
* INPUTS:
*       devObjPtr    - (pointer to) device object.
*       descrPtr     - (pointer to) frame data buffer Id
* OUTPUTS:
*       doLookupPtr  (pointer to) do we need to lookup in this cycle
*       lookupConfigPtr  (pointer to) the lookup config entry
*       tcamModeSelectPtr  (pointer to) tcam select mode
*                           (was taken from 0x0B800000 bit 8)
*       tcamModePtr - (pointer to) tcam mode (was taken from 0x0B800000 bit 0)

* RETURN:
*
* COMMENTS:
*    [1] 8.3 Policy Lookup configuration (page 86)
*
*******************************************************************************/
static GT_VOID snetChtPclTriggeringCheck
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    OUT GT_BOOL *doLookupPtr,
    OUT SNET_CHT_POLICY_LOOKUP_CONFIG_STC *lookupConfigPtr,
    OUT GT_BIT  *tcamModeSelectPtr,
    OUT GT_BIT  *tcamModePtr
)
{
    GT_U32  regVal;

    GT_BIT policyDisabled; /*policy disabled*/
    GT_BIT pclIdMode; /*pcl id mode  0 per port/trunk  , 1 per vlan */

    *doLookupPtr = GT_FALSE;

    if(descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_HARD_DROP_E ||
       descrPtr->policyOnPortEn == 0)
    {
        return;
    }

    if(descrPtr->marvellTagged == 1 &&
       descrPtr->packetCmd  != SKERNEL_EXT_PKT_CMD_FORWARD_E)
    {
        return;
    }

    /* read -- Policy Global Configuration Register 0x0B800000 */
    smemRegGet(devObjPtr,0x0B800000,&regVal);

    policyDisabled = SMEM_U32_GET_FIELD(regVal,7,1);

    if(policyDisabled == 1)
    {
        return;
    }

    *tcamModePtr = SMEM_U32_GET_FIELD(regVal,0,1);
    *tcamModeSelectPtr = SMEM_U32_GET_FIELD(regVal,8,1);
    pclIdMode = SMEM_U32_GET_FIELD(regVal,1,1);

    snetChtPclConfigInfoGet(devObjPtr,descrPtr,pclIdMode,lookupConfigPtr);


    *doLookupPtr = lookupConfigPtr->valid;

    return;
}

/*******************************************************************************
*   snetChtPclLookup
*
* DESCRIPTION:
*        lookup for match in the TCAM tables
*
* INPUTS:
*       devObjPtr    - (pointer to) device object.
*       descrPtr     - (pointer to) frame data buffer Id
*       pclKeyPtr - (pointer to) current pcl key
* OUTPUTS:
*       matchIndexPtr     - (pointer to) index of match on current lookup
*                           SNET_CHT_POLICY_NO_MATCH_INDEX_CNS -- if not found
* RETURN:
*
* COMMENTS:
*    [1] 8.2.2.3 Search Memory   Figure 10 (page 83)
*
*******************************************************************************/
static GT_VOID snetChtPclLookup
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_CHT_POLICY_KEY_STC *pclKeyPtr,
    OUT GT_U32  *matchIndexPtr

)
{
    DECLARE_FUNC_NAME(snetChtPclLookup);

    GT_U32  *uint32Ptr;/*pointer to start of search key (needed for match pclId )*/
    GT_U32  ii ;/*current index in the tcam table*/
    GT_U32  *tcamValuesTablePtr;/* pointer to start of data tcam table */
    GT_U32  *tcamMasksTablePtr ;/* pointer to start of mask tcam table */
    GT_U32  *tcamSecondHalfValuesTablePtr;/* pointer to middle of data tcam table */
    GT_U32  *tcamSecondHalfMasksTablePtr ;/* pointer to middle of mask tcam table */
    GT_U32  keyWordsSize;/*num of words in pclKey[] --- 6 or 12*/
    GT_U32  kk, mm;/*iterators*/
    GT_U32  iiMax;/* max val for ii*/

    ii = 0;
    *matchIndexPtr =  SNET_CHT_POLICY_NO_MATCH_INDEX_CNS;
    if(pclKeyPtr->pclKeyFormat == CHT_PCL_KEY_EXTENDED_E)
    {
        uint32Ptr = (GT_U32*)pclKeyPtr->key.extended;
        iiMax = 512;
    }
    else
    {
        uint32Ptr = (GT_U32*)pclKeyPtr->key.regular;
        iiMax = 1024;
    }

    /*set tcamValuesTablePtr to start of TCAM values table*/
    tcamValuesTablePtr = smemMemGet(devObjPtr,0x0B818000);

    /*set tcamMasksTablePtr to start of TCAM masks table*/
    tcamMasksTablePtr = smemMemGet(devObjPtr, 0x0B810000);

    tcamSecondHalfValuesTablePtr = tcamValuesTablePtr
        + CHT_PCL_TCAM_OFFSET_TO_SECOND_HALF_TABLE_IN_WORD_CNS;
    tcamSecondHalfMasksTablePtr  = tcamMasksTablePtr
        + CHT_PCL_TCAM_OFFSET_TO_SECOND_HALF_TABLE_IN_WORD_CNS;

    keyWordsSize = SNET_CHT_PCL_KEY_WORDS(pclKeyPtr);

    for(ii = 0 ; ii < iiMax ; ii++ ,
        tcamValuesTablePtr += CHT_PCL_TCAM_ENTRY_WORDS_WIDTH_CNS,
        tcamMasksTablePtr  += CHT_PCL_TCAM_ENTRY_WORDS_WIDTH_CNS,
        tcamSecondHalfValuesTablePtr += CHT_PCL_TCAM_ENTRY_WORDS_WIDTH_CNS,
        tcamSecondHalfMasksTablePtr  += CHT_PCL_TCAM_ENTRY_WORDS_WIDTH_CNS
        )
    {
        if(
            ((tcamValuesTablePtr[3] & 0x02) == 0) || /* invalid */
            ((tcamValuesTablePtr[3] & (1<<3)) != (GT_U32)(pclKeyPtr->pclKeyFormat<<3)) || /* mode */
            ((tcamValuesTablePtr[7] & 0x02) == 0) || /* invalid */
            ((tcamValuesTablePtr[7] & (1<<3)) != (GT_U32)(pclKeyPtr->pclKeyFormat<<3))/* mode */
                )
        {
                /* not valid or not matching modes  */
            __LOG(("not valid or not matching modes"));
                continue;
        }

        if(pclKeyPtr->pclKeyFormat == CHT_PCL_KEY_EXTENDED_E)
        {
            if(
                ((tcamSecondHalfValuesTablePtr[3] & 0x02)== 0x0) || /* invalid */
                ((tcamSecondHalfValuesTablePtr[3] & (1<<3)) == 0x0) || /* mode */
                ((tcamSecondHalfValuesTablePtr[7] & 0x02) == 0x0) || /* invalid */
                ((tcamSecondHalfValuesTablePtr[7] & (1<<3)) == 0x0)
                    )
            {
                /* not valid or not matching modes   */
                __LOG(("not valid or not matching modes"));
                continue;
            }

            /* do nothing */
        }

        /* start compare keys */
        for(kk = 0 ; kk < keyWordsSize ; kk++ )
        {
            /*(kk/4) means that after every 3 word of data/mask
              we have 1 word of control*/

            if(kk < 6)
            {
                mm = kk + ((kk + 1)/4) ;
            }
            else
            {
                mm = (kk - 6) + ((kk - 6 + 1)/4)  + CHT_PCL_TCAM_OFFSET_TO_SECOND_HALF_TABLE_IN_WORD_CNS;
            }

            if((uint32Ptr[kk] & tcamMasksTablePtr[mm]) != tcamValuesTablePtr[mm])
            {
                /* no match */
                __LOG(("no match"));
                break;
            }
        }

        if(kk == keyWordsSize)
        {
            /* found match */
            __LOG(("found match"));
            *matchIndexPtr = ii;
            break;
        }
    }

    return;
}

/*******************************************************************************
*   snetChtPclUcActionApply
*
* DESCRIPTION:
*       Apply Routing PCL relevant action on the frames descriptor
*
* INPUTS:
*       devObjPtr    - (pointer to) device object.
*       descrPtr     - (pointer to) frame data buffer Id
*
* OUTPUTS:
*       descrPtr     - (pointer to) frame data buffer Id
*
* RETURN:
*
* COMMENTS:
*       The Policy engine only applies the rule action configuration for the
*       <Match Counter>. The remaining Routing PCL action values are stored
*       internally but are not applied to the packets forwarding information.
*
*******************************************************************************/
static GT_VOID snetChtPclUcActionApply
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_U32 matchIndex
)
{
    DECLARE_FUNC_NAME(snetChtPclUcActionApply);

    GT_U32 * nextHopEntryMemPtr;    /* (pointer to) action entry in memory */
    GT_BOOL matchCounterEn;         /* match counter enabled/disabled */
    GT_U32 matchCounterIndex;       /* match counter index */
    GT_U32 counterVal;              /* pcl counter value */

    /* Store nexthop info index internally into descriptor  */
    descrPtr->pclUcNextHopIndex = matchIndex;

    nextHopEntryMemPtr = smemMemGet(devObjPtr,
                                SMEM_CHT_PCL_ACTION_TBL_MEM(devObjPtr, matchIndex));

    /* The match counter index to which this rule is bound */
    matchCounterIndex = SMEM_U32_GET_FIELD(nextHopEntryMemPtr[0], 26, 5);

    /* If set, the PCL rule is bound to the specified match counter index */
    matchCounterEn = SMEM_U32_GET_FIELD(nextHopEntryMemPtr[0], 25, 1);

    /* PCL counters */
    __LOG(("PCL counters"));
    if(matchCounterEn == GT_TRUE)
    {
    /* The match counter is incremented regardless of whether or not the packet
       is routed by the router engine */
        smemRegGet(devObjPtr,
                 0x0B801000 + (4 * matchCounterIndex), &counterVal);

        counterVal++;

        smemRegSet(devObjPtr,
                 0x0B801000 + (4 * matchCounterIndex), counterVal);
    }
}

/*******************************************************************************
*   snetChtPclActionGet
*
* DESCRIPTION:
*       set pcl actions into simulation format
*       Fill actionInfoPtr info from the actions table according to matchIndex
*
* INPUTS:
*       devObjPtr    - (pointer to) device object.
*       descrPtr     - (pointer to) frame data buffer Id
*       matchIndex   - index of match on current lookup
*
* OUTPUTS:
*       actionInfoPtr - (pointer to) info about the action that need to be apply
*                       on the flow
*
* RETURN:
*
* COMMENTS:
*    [1] 8.6.1 Policy Action Commands   (page 102)
*    format in [1] C.12.9.1 Policy Action Table  table 337  (page 469)
*
*******************************************************************************/
static GT_VOID snetChtPclActionGet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_U32       matchIndex,
    OUT SNET_CHT_POLICY_ACTION_STC *actionInfoPtr
)
{
    DECLARE_FUNC_NAME(snetChtPclActionGet);

    GT_U32  *actionEntryMemPtr;/* (pointer to) action entry in memory */

    actionEntryMemPtr = smemMemGet(devObjPtr,
                                   SMEM_CHT_PCL_ACTION_TBL_MEM(devObjPtr, matchIndex));


    actionInfoPtr->pceRedirectCmd = SMEM_U32_GET_FIELD(actionEntryMemPtr[1],0,2);
    if (actionInfoPtr->pceRedirectCmd == PCL_TTI_ACTION_REDIRECT_CMD_PBR_OR_VPLS_E)
    {
        /* Action type is routed entry */
        __LOG(("Action type is routed entry"));
        snetChtPclUcActionApply(devObjPtr, descrPtr, matchIndex);

        return;
    }

    /* word 0 of the action */
    actionInfoPtr->matchCounterIndex = SMEM_U32_GET_FIELD(actionEntryMemPtr[0],26,5);
    actionInfoPtr->matchCounterEn = SMEM_U32_GET_FIELD(actionEntryMemPtr[0],25,1);
    actionInfoPtr->pceModifyUp = SMEM_U32_GET_FIELD(actionEntryMemPtr[0],23,2);
    actionInfoPtr->pceModifyDscp = SMEM_U32_GET_FIELD(actionEntryMemPtr[0],21,2);
    actionInfoPtr->pceQosProfile = SMEM_U32_GET_FIELD(actionEntryMemPtr[0],14,7);
    actionInfoPtr->pceQosProfileMarkingEn = SMEM_U32_GET_FIELD(actionEntryMemPtr[0],13,1);
    actionInfoPtr->pceQosPrecedence = SMEM_U32_GET_FIELD(actionEntryMemPtr[0],12,1);
    actionInfoPtr->mirrorToIngressAnalyzerPort = SMEM_U32_GET_FIELD(actionEntryMemPtr[0],11,1);
    actionInfoPtr->pceCpuCode = SMEM_U32_GET_FIELD(actionEntryMemPtr[0],3,8);
    actionInfoPtr->fwdCmd = SMEM_U32_GET_FIELD(actionEntryMemPtr[0],0,3);

    /* word 1 of the action */
    actionInfoPtr->pceVid = SMEM_U32_GET_FIELD(actionEntryMemPtr[1],19,12);
    actionInfoPtr->pceVlanCmd = SMEM_U32_GET_FIELD(actionEntryMemPtr[1],17,2);
    actionInfoPtr->enNestedVlan = SMEM_U32_GET_FIELD(actionEntryMemPtr[1],16,1);
    actionInfoPtr->pceVidPrecedence = SMEM_U32_GET_FIELD(actionEntryMemPtr[1],15,1);

    if(actionInfoPtr->pceRedirectCmd == PCL_TTI_ACTION_REDIRECT_CMD_OUT_IF_E)
    {
        /*  Redirect the packet to the Egress interface specified
            in <PCE Egress Interface> */

        if(SMEM_U32_GET_FIELD(actionEntryMemPtr[1],14,1))/* use vidx */
        {
            /* vidx */
            __LOG(("vidx"));
            actionInfoPtr->pceEgressInterface.useVidx = GT_TRUE;
            actionInfoPtr->pceEgressInterface.vidxInfo.vidx = (GT_U16)SMEM_U32_GET_FIELD(actionEntryMemPtr[1],2,12);
        }
        else
        {
            /* port/trunk */
            __LOG(("port/trunk"));
            actionInfoPtr->pceEgressInterface.useVidx = GT_FALSE;

            if(SMEM_U32_GET_FIELD(actionEntryMemPtr[1],2,1))/* targetIsTrunk */
            {
                /* trunk */
                __LOG(("trunk"));
                actionInfoPtr->pceEgressInterface.vidxInfo.noVidx.targetIsTrunk = GT_TRUE;
                actionInfoPtr->pceEgressInterface.vidxInfo.noVidx.trunkInfo.targetTrunk =
                    (GT_U16)SMEM_U32_GET_FIELD(actionEntryMemPtr[1],3,7);
            }
            else
            {
                /* port */
                __LOG(("port"));
                actionInfoPtr->pceEgressInterface.vidxInfo.noVidx.targetIsTrunk = GT_FALSE;
                actionInfoPtr->pceEgressInterface.vidxInfo.noVidx.trunkInfo.noTrunk.targetDev =
                    (GT_U8)SMEM_U32_GET_FIELD(actionEntryMemPtr[1],9,5);
                actionInfoPtr->pceEgressInterface.vidxInfo.noVidx.trunkInfo.noTrunk.targetPort =
                    (GT_U8)SMEM_U32_GET_FIELD(actionEntryMemPtr[1],3,6);
            }
        }
    }
    else
    {
        memset(&actionInfoPtr->pceEgressInterface,0,
                sizeof(actionInfoPtr->pceEgressInterface));
    }

    /* word 2 of the action */
    actionInfoPtr->policerIndex = SMEM_U32_GET_FIELD(actionEntryMemPtr[2],1,8);
    actionInfoPtr->policerEn = SMEM_U32_GET_FIELD(actionEntryMemPtr[2],0,1);

    return;
}



/*******************************************************************************
*   snetChtPclCpuCodeResolution
*
* DESCRIPTION:
*       resolve from old and new commands the new cpu code
*
* INPUTS:
*       devObjPtr    - (pointer to) device object.
*       prevCmd --- previous command
*       currCmd  current command
*       prevCpuCode -- previous cpu code
*       currCpuCode  current cpu code
*
*
* OUTPUTS:
*       none
*
* RETURN:
*       resolved cpu code
*
* COMMENTS:
*
*        [2] Table 6: cpu code changes conflict resolution   page 17
*
*******************************************************************************/
static GT_U32 snetChtPclCpuCodeResolution
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_EXT_PACKET_CMD_ENT prevCmd,
    IN SKERNEL_EXT_PACKET_CMD_ENT currCmd,
    IN GT_U32                     prevCpuCode,
    IN GT_U32                     currCpuCode
)
{
    DECLARE_FUNC_NAME(snetChtPclCpuCodeResolution);

    GT_U32  resolvedCpuCode = prevCpuCode;/* resolved cpu code */
    GT_U32  cpuCodePrecedence;/* cpu code precedence */

    /* read -- Policy Global Configuration Register 0x0B800000 */
    smemRegFldGet(devObjPtr,0x0B800000,5,1,&cpuCodePrecedence);

    if(prevCmd == SKERNEL_EXT_PKT_CMD_HARD_DROP_E ||
       currCmd == SKERNEL_EXT_PKT_CMD_HARD_DROP_E )
    {
        /* no change */
        __LOG(("no change"));
    }
    else if(prevCmd == SKERNEL_EXT_PKT_CMD_FORWARD_E ||
            prevCmd == SKERNEL_EXT_PKT_CMD_SOFT_DROP_E )
    {
        resolvedCpuCode = currCpuCode;
    }
    else if(currCmd == SKERNEL_EXT_PKT_CMD_FORWARD_E ||
            currCmd == SKERNEL_EXT_PKT_CMD_SOFT_DROP_E )
    {
        /* no change */
        __LOG(("no change"));
    }
    else if(prevCmd == SKERNEL_EXT_PKT_CMD_TRAP_TO_CPU_E )
    {
        if(currCmd == prevCmd)
        {
            if(cpuCodePrecedence == 1)
            {
                /* no change */
                __LOG(("no change"));
            }
            else
            {
                resolvedCpuCode = currCpuCode;
            }
        }
        else
        {
            /* no change */
            __LOG(("no change"));
        }
    }
    else if(currCmd == SKERNEL_EXT_PKT_CMD_TRAP_TO_CPU_E  )
    {
        resolvedCpuCode = currCpuCode;
    }
    else if(prevCmd == SKERNEL_EXT_PKT_CMD_MIRROR_TO_CPU_E )
    {
        if(currCmd == prevCmd)
        {
            if(cpuCodePrecedence == 1)
            {
                /* no change */
            }
            else
            {
                resolvedCpuCode = currCpuCode;
            }
        }
        else
        {
            /* no change */
        }
    }
    else if(currCmd == SKERNEL_EXT_PKT_CMD_MIRROR_TO_CPU_E  )
    {
        resolvedCpuCode = currCpuCode;
    }

    return  resolvedCpuCode;
}

/*******************************************************************************
*   snetChtPclActionApply
*
* DESCRIPTION:
*       Apply pcl actions on the frames descriptor
*
* INPUTS:
*       devObjPtr    - (pointer to) device object.
*       descrPtr     - (pointer to) frame data buffer Id
*       actionInfoPtr - (pointer to) info about the action that need to be apply
*                       on the flow
* OUTPUTS:
*       descrPtr     - (pointer to) frame data buffer Id
*
* RETURN:
*
* COMMENTS:
*       [1] 8.6.2 Applying the Action on the Packet   (page 102)
*       [2] 4.3 Action (page 15)
*
*******************************************************************************/
static GT_VOID snetChtPclActionApply
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_CHT_POLICY_ACTION_STC *actionInfoPtr
)
{
    GT_U32  counterVal;/* pcl counter value */
    GT_BOOL updateVid = GT_FALSE;/* update eVid */

    if (actionInfoPtr->pceRedirectCmd == PCL_TTI_ACTION_REDIRECT_CMD_PBR_OR_VPLS_E)
    {
        /* Route action will be applied later in UC route engine */
        return;
    }

    /* associated CPU code */
    /* need to come before calling snetCheetahPktCmdResolution(...)
       because we need the descriptorPtr->packetCmd before the update*/
    descrPtr->cpuCode = snetChtPclCpuCodeResolution(devObjPtr,descrPtr->packetCmd,
                                                    actionInfoPtr->fwdCmd,
                                                    descrPtr->cpuCode,
                                                    actionInfoPtr->pceCpuCode);

    /* Apply packet command */
    descrPtr->packetCmd = snetChtPktCmdResolution(descrPtr->packetCmd,
                                                  actionInfoPtr->fwdCmd);

    /* Ingress mirroring command */
    if(actionInfoPtr->mirrorToIngressAnalyzerPort == GT_TRUE)
    {
        descrPtr->rxSniff = 1;
    }

    /* PCL counters */
    if(actionInfoPtr->matchCounterEn == GT_TRUE)
    {
        /* Update counter in  :
        n = actionInfoPtr->counterPtr
        Table 353: Policy Rule Match Counter<n> (0<=n<32)
        Offset:  Counter0: 0x0B801000, Counter1: 0x0B801004... Counter31: 0x0B80107C
        (32 Registers in steps of 0x4) */

        smemRegGet(devObjPtr,
                   0x0B801000 + (4 * actionInfoPtr->matchCounterIndex),
                   &counterVal);

        counterVal ++;

        smemRegSet(devObjPtr,
                   0x0B801000 + (4 * actionInfoPtr->matchCounterIndex),
                   counterVal);
    }

    /* QoS Marking Command */
    if(descrPtr->qosProfilePrecedence == SKERNEL_QOS_PROF_PRECED_SOFT)
    {
        if(actionInfoPtr->pceQosPrecedence ==
               (SKERNEL_PRECEDENCE_ORDER_ENT)SKERNEL_QOS_PROF_PRECED_HARD )
        {
            descrPtr->qosProfilePrecedence = SKERNEL_QOS_PROF_PRECED_HARD;
        }

        if(actionInfoPtr->pceQosProfileMarkingEn == GT_TRUE)
        {
            descrPtr->qos.qosProfile = actionInfoPtr->pceQosProfile;
        }

        if(actionInfoPtr->pceModifyDscp == 1)
        {
            descrPtr->modifyDscp = 1;
        }
        else if(actionInfoPtr->pceModifyDscp == 2)
        {
            descrPtr->modifyDscp = 0;
        }

        if(actionInfoPtr->pceModifyUp == 1)
        {
            descrPtr->modifyUp = 1;
        }
        else if(actionInfoPtr->pceModifyUp == 2)
        {
            descrPtr->modifyUp = 0;
        }
    }

    /* Nested Vid  */
    if(descrPtr->nestedVlanAccessPort == 0)
    {
        /*When <EnNestedVLAN> is set, this rule matching flow is defined as a Nested VLAN Access Flow.*/
        descrPtr->nestedVlanAccessPort = actionInfoPtr->enNestedVlan;
        if(actionInfoPtr->enNestedVlan)
        {
            /* When a packet received on an access flow is transmitted via a
               tagged port or a cascading port, a VLAN tag is added to the packet
               (on top of the existing VLAN tag, if any). The VID field is the
               VID assigned to the packet as a result of all VLAN assignment
               algorithms. */
            descrPtr->tagSrcTagged[SNET_CHT_TAG_0_INDEX_CNS] = 0;
        }
    }

    /* Vid commands */
    if(descrPtr->preserveVid == 0)
    {
        if(actionInfoPtr->pceVidPrecedence == 1)
        {
            descrPtr->preserveVid = 1;
        }

        if(actionInfoPtr->pceVlanCmd == 3)/* all */
        {
            updateVid = GT_TRUE;
        }
        else if(actionInfoPtr->pceVlanCmd == 2)/* tagged */
        {
            if( descrPtr->tagSrcTagged[SNET_CHT_TAG_0_INDEX_CNS] == 1 &&
                descrPtr->srcPriorityTagged == 0)
            {
                updateVid = GT_TRUE;
            }
        }
        else if(actionInfoPtr->pceVlanCmd == 1)/* untagged */
        {
            if(descrPtr->srcPriorityTagged == 1 ||
               descrPtr->tagSrcTagged[SNET_CHT_TAG_0_INDEX_CNS] == 0)
            {
                updateVid = GT_TRUE;
            }
        }


        if(updateVid == GT_TRUE)
        {
            descrPtr->eVid = actionInfoPtr->pceVid;
            descrPtr->vidModified = 1;
        }
    }

    /* Policer info */
    if(descrPtr->policerEn == 0)
    {
        descrPtr->policerEn = actionInfoPtr->policerEn ;

        if(descrPtr->policerEn == 1)
        {
            descrPtr->policerPtr = actionInfoPtr->policerIndex;
        }
    }

    /* Redirect command (bypass bridge) */
    if(actionInfoPtr->pceRedirectCmd == PCL_TTI_ACTION_REDIRECT_CMD_OUT_IF_E)
    {
        descrPtr->bypassBridge = 1;

        /* The redirect info */
        descrPtr->useVidx = actionInfoPtr->pceEgressInterface.useVidx;
        descrPtr->eVidx = actionInfoPtr->pceEgressInterface.vidxInfo.vidx;
        descrPtr->targetIsTrunk = actionInfoPtr->pceEgressInterface.vidxInfo.noVidx.targetIsTrunk;
        descrPtr->trgEPort = actionInfoPtr->pceEgressInterface.vidxInfo.noVidx.trunkInfo.noTrunk.targetPort;
        descrPtr->trgDev = actionInfoPtr->pceEgressInterface.vidxInfo.noVidx.trunkInfo.noTrunk.targetDev;
        descrPtr->trgTrunkId = actionInfoPtr->pceEgressInterface.vidxInfo.noVidx.trunkInfo.targetTrunk;
    }

}
/*******************************************************************************
*   snetChtPcl
*
* DESCRIPTION:
*        (Cheetah) Ingress Policy Engine processing for frame
*        PCL processing , Pcl assignment ,key forming , 2 Lookups ,
*        actions to descriptor processing
*
* INPUTS:
*       devObjPtr    - (pointer to) device object.
*       descrPtr     - (pointer to) frame data buffer Id
* OUTPUTS:
*       descrPtr     - (pointer to) updated frame data buffer Id
*
* RETURN:
*
* COMMENTS:
*    [1] 8.2.3 Packet walk through  Figure 11 (page 85)
*
*******************************************************************************/
extern GT_VOID snetChtPcl
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetChtPcl);

    GT_U32  cycleNum;               /*the number of current cycle*/
    GT_BOOL doLookup;               /* do we need to lookup in this cycle*/
    GT_U32  matchIndex;             /* index of match on current lookup -
                                       0xffff means --- not found */
    SNET_CHT_POLICY_ACTION_STC actionInfo ; /*structure that hold info about the
                                     action that need to be apply on the flow */
    SNET_CHT_POLICY_LOOKUP_CONFIG_STC lookupConfig ;/* the lookup config entry (on 2 pcls)*/
    GT_BIT  tcamModeSelect;         /*tcam select mode (was taken from 0x0B800000 bit 8)*/
    GT_BIT  tcamMode;               /*tcam mode (was taken from 0x0B800000 bit 0)*/
    SNET_CHT_POLICY_SPECIFIC_LOOKUP_CONFIG_STC currPclLookupConfig ;
                                    /* info (structure) on the lookup config on
                                    the current pclId (current cycle)*/
    GT_BOOL extendedLookupMode;     /*extended lookup mode (48 bytes)
                                    or regular (24 bytes)*/
    GT_BOOL oldExtendedLookupMode = GT_FALSE;  /*previous extended lookup mode (48 bytes)
                                    or regular (24 bytes)*/
    SNET_CHT_POLICY_KEY_STC pclKey; /* current pcl key (48 bytes) */
    SKERNEL_EXT_PACKET_CMD_ENT userDefinedParseError;
                                    /* did error in key build due to user defined bytes */
    CHT_PCL_EXTRA_PACKET_INFO_STC   pclCookie;
                                    /* pcl extra info (cookie) */
    GT_BOOL     firstPclNotValid = GT_FALSE;
                                    /* the pcl on first cycle is invalid */

    snetChtPclTriggeringCheck(devObjPtr, descrPtr, &doLookup, &lookupConfig,
                              &tcamModeSelect, &tcamMode);

    memset(&pclKey, 0, sizeof(pclKey));
    pclKey.devObjPtr = devObjPtr;

    if(doLookup == GT_FALSE ||
       descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_HARD_DROP_E)
    {
        return ;
    }

    snetChtPclSrvParseExtraData(devObjPtr, descrPtr, &pclCookie);

    /* do 2 lookup loops */
    for(cycleNum = 0 ; cycleNum < 2 ; cycleNum++)
    {
        snetChtPclLookupInfoGet(devObjPtr, descrPtr, cycleNum, &lookupConfig,
                                tcamModeSelect, tcamMode, &extendedLookupMode,
                                &currPclLookupConfig);

        if(currPclLookupConfig.enabled == GT_FALSE)
        {
            firstPclNotValid = GT_TRUE;
            /* current pcl not enabled */
            __LOG(("current pcl not enabled"));
            continue;
        }

        pclKey.updateOnlyDiff = (cycleNum == 0) ? GT_FALSE :
                                ((firstPclNotValid == GT_TRUE) ||
                                 (oldExtendedLookupMode != extendedLookupMode))?
                                GT_FALSE :
                                GT_TRUE;

        pclKey.pclKeyFormat = (extendedLookupMode) ?
            CHT_PCL_KEY_EXTENDED_E : CHT_PCL_KEY_REGULAR_E;

        oldExtendedLookupMode = extendedLookupMode;

        snetChtPclKeyCreate(devObjPtr, descrPtr , cycleNum, &currPclLookupConfig,
                            &pclKey , &userDefinedParseError ,&pclCookie);

        if(userDefinedParseError != SKERNEL_EXT_PKT_CMD_FORWARD_E)
        {
            /* has error ?*/
            __LOG(("has error ?"));
            continue;
        }

        snetChtPclLookup(devObjPtr, descrPtr ,&pclKey , &matchIndex);

        if(matchIndex == SNET_CHT_POLICY_NO_MATCH_INDEX_CNS)
        {
            /* no match */
            __LOG(("no match"));
            continue;
        }

        /* get the actions info for this lookup */
        snetChtPclActionGet(devObjPtr, descrPtr , matchIndex , &actionInfo);

        /* apply the action on the descriptor */
        snetChtPclActionApply(devObjPtr, descrPtr , &actionInfo);
    }

    return;
}

/*******************************************************************************
*   SNET_UDB_KEY_VALUE_GET
*
* DESCRIPTION:
*        Get user defined value by user defined key.
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       descrPtr            - pointer to frame data buffer Id.
*       byteKeyArr          - user defined key array (byte offset/anchor).
*       byteIdx             - byte index in user defined key array.
*
* OUTPUTS:
*       byteValuePtr        - pointer to UDB value.
*
* RETURN:
*       0 - OK
*       1 - Not valid byte
* COMMENTS:
*
*******************************************************************************/
extern GT_U32 snetChtPclUdbKeyValueGet
(
    IN SKERNEL_DEVICE_OBJECT                        * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC              * descrPtr,
    IN CHT_PCL_USER_DEFINED_CONFIG_REG_STC          * byteKeyArr,
    IN GT_U32                                         byteIdx,
    OUT GT_U8                                       * byteValuePtr
)
{
    GT_U32  byteData;           /* user defined data */
    GT_U32  userDefinedAnchor;  /* user defined byte Anchor */
    GT_U32  userDefinedOffset;  /* user defined byte offset from Anchor */
    GT_U32  retErr = 0;
    GT_STATUS  rc;

    ASSERT_PTR(byteValuePtr);

    *byteValuePtr = 0;

    smemRegGet(devObjPtr, byteKeyArr[byteIdx].regOffsetAddress, &byteData);

    userDefinedAnchor =
        SMEM_U32_GET_FIELD(byteData, byteKeyArr[byteIdx].startBitAnchor, 2);
    userDefinedOffset =
        SMEM_U32_GET_FIELD(byteData, byteKeyArr[byteIdx].startBitOffset, 7);

    rc = snetChtPclUserDefinedByteGet(devObjPtr, descrPtr, userDefinedAnchor,
                                      userDefinedOffset, byteValuePtr);
    if(rc != GT_OK)
    {
        *byteValuePtr = 0;
        retErr = 1;
    }

    return retErr;
}
