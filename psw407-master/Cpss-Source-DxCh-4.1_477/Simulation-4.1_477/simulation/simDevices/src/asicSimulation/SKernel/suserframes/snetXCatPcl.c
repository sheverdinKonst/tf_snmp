/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetXCatPcl.c
*
* DESCRIPTION:
*
*       Ingress Policy Engine processing for incoming frame.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 139 $
*
*******************************************************************************/

#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SKernel/smem/smemCheetah.h>
#include <asicSimulation/SKernel/cheetahCommon/sregCheetah.h>
#include <asicSimulation/SKernel/suserframes/snet.h>
#include <asicSimulation/SKernel/suserframes/snetXCatPcl.h>
#include <asicSimulation/SKernel/suserframes/snetLion.h>
#include <asicSimulation/SKernel/suserframes/snetCheetahEq.h>
#include <asicSimulation/SKernel/suserframes/snetLion2Oam.h>
#include <asicSimulation/SLog/simLog.h>
#include <asicSimulation/SKernel/suserframes/snetXCat.h>
#include <asicSimulation/SKernel/suserframes/snetLion3Tcam.h>

#define CNV_CHT2_ACTION_MAC(action_ptr) \
    ((SNET_CHT2_PCL_ACTION_STC *)action_ptr)

#define CNV_CHT3_ACTION_MAC(action_ptr) \
    ((SNET_CHT3_PCL_ACTION_STC *)action_ptr)

#define CNV_LION_ACTION_MAC(action_ptr) \
    ((SNET_LION_PCL_ACTION_STC *)action_ptr)

/* EPCL relevant macro */
#define CNV_CHT3_EPCL_ACTION_MAC(action_ptr) \
    ((SNET_CHT3_EPCL_ACTION_STC *)action_ptr)

/* Bit mask for TCP flags FIN (bit 0) and RST (bit 2) */
#define SNET_XCAT_TCP_FIN_RST_MASK_CNS  5

#define CONVERT_DESCRIPTOR_L2_ENCAPS_TO_KEY_L2_ENCAPSULATION_MAC(descrPtr_l2Encaps) \
    ((descrPtr_l2Encaps ==SKERNEL_LLC_E)         ? 0 : \
     (descrPtr_l2Encaps ==SKERNEL_ETHERNET_II_E) ? 1 : \
                                                   3)
#define SNET_LION3_CNC_IPCL_LOOKUP_GET(_cycleNum,_parallelLookupInst)   \
    ((_parallelLookupInst) +                                             \
        (((_cycleNum) == 0) ? SNET_CNC_CLIENT_IPCL_LOOKUP_0_ACTION_0_E : \
         ((_cycleNum) == 1) ? SNET_CNC_CLIENT_IPCL_LOOKUP_1_ACTION_0_E : \
                              SNET_CNC_CLIENT_IPCL_LOOKUP_2_ACTION_0_E))

#define SNET_LION3_CNC_EPCL_LOOKUP_GET(_parallelLookupInst)   \
    ((_parallelLookupInst) + SNET_CNC_CLIENT_EPCL_ACTION_0_E)



#define VALUE_IPV4_FRAGMENT_MAC(pclExtraDataPtr,value) \
    if (pclExtraDataPtr->isIpv4Fragment == 0)          \
    {                                                  \
        value = 0;                                     \
    }                                                  \
    else if (pclExtraDataPtr->ipv4FragmentOffset == 0) \
    {                                                  \
        value = 1;                                     \
    }                                                  \
    else                                               \
    {                                                  \
        value = 2;                                     \
    }

/* DxChXCat IPCL LOOKUP NOT IP */
static SNET_XCAT_IPCL_KEY_TYPE_ENT type_IPCL_NOT_IP[8] =
{
/* 0 */ XCAT_IPCL_KEY_TYPE_L2_KEY_SHORT_E,
/* 1 */ XCAT_IPCL_KEY_TYPE_LAST_E,
/* 2 */ XCAT_IPCL_KEY_TYPE_L2_L3_L4_KEY_LONG_E,
/* 3 */ XCAT_IPCL_KEY_TYPE_UDB_KEY_SHORT_E,
/* 4 */ XCAT_IPCL_KEY_TYPE_UDB_KEY_LONG_E,
/* 5 */ XCAT_IPCL_KEY_TYPE_IPCL0_IPV4_KEY_LONG_E,
/* 6 */ XCAT_IPCL_KEY_TYPE_IPCL0_IPV6_KEY_TRIPLE_E,
/* 7 */ XCAT_IPCL_KEY_TYPE_LAST_E
};

/* DxChXCat IPCL LOOKUP IPV4 */
static SNET_XCAT_IPCL_KEY_TYPE_ENT type_IPCL_IPV4[8] =
{
/* 0 */ XCAT_IPCL_KEY_TYPE_L2_L3_KEY_SHORT_E,
/* 1 */ XCAT_IPCL_KEY_TYPE_L3_L4_KEY_SHORT_E,
/* 2 */ XCAT_IPCL_KEY_TYPE_L2_L3_L4_KEY_LONG_E,
/* 3 */ XCAT_IPCL_KEY_TYPE_UDB_KEY_SHORT_E,
/* 4 */ XCAT_IPCL_KEY_TYPE_UDB_KEY_LONG_E,
/* 5 */ XCAT_IPCL_KEY_TYPE_IPCL0_IPV4_KEY_LONG_E,
/* 6 */ XCAT_IPCL_KEY_TYPE_IPCL1_IPV4_KEY_SHORT_E,
/* 7 */ XCAT_IPCL_KEY_TYPE_LAST_E
};

/* DxChXCat IPCL LOOKUP IPV6 */
static SNET_XCAT_IPCL_KEY_TYPE_ENT type_IPCL_IPV6[8] =
{
/* 0 */ XCAT_IPCL_KEY_TYPE_L2_L3_KEY_SHORT_E,
/* 1 */ XCAT_IPCL_KEY_TYPE_L2_IPV6_KEY_LONG_E,
/* 2 */ XCAT_IPCL_KEY_TYPE_L4_IPV6_KEY_LONG_E,
/* 3 */ XCAT_IPCL_KEY_TYPE_UDB_KEY_SHORT_E,
/* 4 */ XCAT_IPCL_KEY_TYPE_UDB_KEY_LONG_E,
/* 5 */ XCAT_IPCL_KEY_TYPE_IPCL0_IPV6_KEY_TRIPLE_E,
/* 6 */ XCAT_IPCL_KEY_TYPE_IPCL1_IPV6_KEY_TRIPLE_E,
/* 7 */ XCAT_IPCL_KEY_TYPE_IPV6_DIP_KEY_SHORT_E
};

/* DxChXCat EPCL LOOKUP NOT IP */
static CHT3_EPCL_KEY_TYPE_ENT type_EPCL_NOT_IP[8] =
{
/* EPCL LOOKUP NOT IP */
/* 0 */ CHT3_EPCL_KEY_TYPE_L2_KEY_SHORT_E,
/* 1 */ CHT3_EPCL_KEY_TYPE_L2_L3_L4_KEY_LONG_E,
/* 2 */ CHT3_EPCL_KEY_TYPE_L4_IPV4_NON_IP_KEY_LONG_E,
/* 3 */ CHT3_EPCL_KEY_TYPE_LAST_E,
/* 4 */ CHT3_EPCL_KEY_TYPE_LAST_E,
/* 5 */ CHT3_EPCL_KEY_TYPE_LAST_E,
/* 6 */ CHT3_EPCL_KEY_TYPE_LAST_E,
/* 7 */ CHT3_EPCL_KEY_TYPE_LAST_E
};

/* DxChXCat EPCL LOOKUP IPV4 */
static CHT3_EPCL_KEY_TYPE_ENT type_EPCL_IPV4[8] =
{
/* 0 */ CHT3_EPCL_KEY_TYPE_L2_L3_KEY_SHORT_E,
/* 1 */ CHT3_EPCL_KEY_TYPE_L3_L4_KEY_SHORT_E,
/* 2 */ CHT3_EPCL_KEY_TYPE_L2_L3_L4_KEY_LONG_E,
/* 3 */ CHT3_EPCL_KEY_TYPE_L4_IPV4_NON_IP_KEY_LONG_E,
/* 4 */ CHT3_EPCL_KEY_TYPE_LAST_E,
/* 5 */ CHT3_EPCL_KEY_TYPE_LAST_E,
/* 6 */ CHT3_EPCL_KEY_TYPE_LAST_E,
/* 7 */ CHT3_EPCL_KEY_TYPE_LAST_E
};

/* DxChXCat EPCL LOOKUP IPV6 */
static CHT3_EPCL_KEY_TYPE_ENT type_EPCL_IPV6[8] =
{
/* 0 */ CHT3_EPCL_KEY_TYPE_L2_L3_KEY_SHORT_E,
/* 1 */ CHT3_EPCL_KEY_TYPE_L4_IPV6_KEY_TRIPLE_E,
/* 2 */ CHT3_EPCL_KEY_TYPE_L2_IPV6_KEY_LONG_E,
/* 3 */ CHT3_EPCL_KEY_TYPE_L4_IPV6_KEY_LONG_E,
/* 4 */ CHT3_EPCL_KEY_TYPE_LAST_E,
/* 5 */ CHT3_EPCL_KEY_TYPE_LAST_E,
/* 6 */ CHT3_EPCL_KEY_TYPE_LAST_E,
/* 7 */ CHT3_EPCL_KEY_TYPE_LAST_E
};

static GT_STATUS snetXCatPclUdbKeyValueGet
(
    IN SKERNEL_DEVICE_OBJECT                        * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC              * descrPtr,
    IN SNET_XCAT_IPCL_KEY_TYPE_ENT                    keyType,
    IN GT_U32                                         udbIdx,
    OUT GT_U8                                       * byteValuePtr
);

static GT_VOID snetXcat2IpclKeyPortsBmpBuild
(
    IN SKERNEL_DEVICE_OBJECT                * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC   * descrPtr,
    OUT SNET_CHT_POLICY_KEY_STC             * pclKeyPtr,
    OUT           GT_U32                  * portListBmpPtr
);

static void snetLion3EpclUdbMetaDataBuild
(
    IN  SKERNEL_DEVICE_OBJECT           *devObjPtr,
    INOUT  SKERNEL_FRAME_CHEETAH_DESCR_STC *descrPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC    *pclExtraDataPtr,
    IN  GT_U32                                egressPort
);

static GT_U32   snetIpclGetProperFieldValue(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    IN GT_U32                           *memPtr,
    IN GT_U32                           matchIndex,
    IN SMEM_LION3_IPCL_ACTION_TABLE_FIELDS  fieldName
);

/* macro to shorten the calling code of ipcl action entry table fields */
#define SMEM_LION3_IPCL_ACTION_ENTRY_FIELD_GET(fieldName)       \
    snetIpclGetProperFieldValue(devObjPtr,memPtr,matchIndex,fieldName)


/* macro to shorten the calling code of sip5_20ipcl action entry table fields */
#define SMEM_SIP5_20_IPCL_ACTION_ENTRY_FIELD_GET(fieldName)       \
snetFieldFromEntry_GT_U32_Get(devObjPtr,                           \
                memPtr,                                            \
                devObjPtr->tableFormatInfo[SKERNEL_TABLE_FORMAT_IPCL_ACTION_E].formatNamePtr,\
                matchIndex,                                                                  \
                devObjPtr->tableFormatInfo[SKERNEL_TABLE_FORMAT_IPCL_ACTION_E].fieldsInfoPtr,\
                devObjPtr->tableFormatInfo[SKERNEL_TABLE_FORMAT_IPCL_ACTION_E].fieldsNamePtr,\
                fieldName)


/* macro to shorten the calling code of epcl action entry table fields */
#define SMEM_LION3_EPCL_ACTION_ENTRY_FIELD_GET(fieldName)       \
    snetFieldFromEntry_GT_U32_Get(devObjPtr,                    \
        memPtr,                                                 \
        devObjPtr->tableFormatInfo[SKERNEL_TABLE_FORMAT_EPCL_ACTION_E].formatNamePtr,                  \
        matchIndex,                                             \
        devObjPtr->tableFormatInfo[SKERNEL_TABLE_FORMAT_EPCL_ACTION_E].fieldsInfoPtr, \
        devObjPtr->tableFormatInfo[SKERNEL_TABLE_FORMAT_EPCL_ACTION_E].fieldsNamePtr, \
        fieldName)

/* maximum number of MPLS labels analyzed for EPCL */
#define LION3_EPCL_MAX_MPLS_LABELS      4

/* fields that are not implemented in yet in build of KEY for the simulation */
SIM_TBD_BOOKMARK
#define EPCL_KEY_FIELD_NOT_IMPLEMENTED_MAC(fieldName,defaultValue)          \
    __LOG(("simulation limitation : %s is not implemented \n",#fieldName)); \
    fieldName = defaultValue

typedef GT_VOID (*LION3_IPCL_BUILD_KEY_FUN)
(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC  * curLookUpCfgPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC    * pclExtraDataPtr,
    IN DXCH_IPCL_KEY_ENT                  keyIndex,
    OUT SNET_CHT_POLICY_KEY_STC         * pclKeyPtr
);

typedef struct{
    LION3_IPCL_BUILD_KEY_FUN    buildKeyFunc;
    GT_U8                       nameString[50];
}LION3_IPCL_KEY_INFO_FUN;

#define  SNET_LION3_IPCL_BUILD_KEY_FUNC_MAC(funcName)           \
    static GT_VOID funcName                                     \
    (                                                           \
        IN SKERNEL_DEVICE_OBJECT            * devObjPtr,        \
        IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr,         \
        IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC  * curLookUpCfgPtr,  \
        IN CHT_PCL_EXTRA_PACKET_INFO_STC    * pclExtraDataPtr,  \
        IN DXCH_IPCL_KEY_ENT                  keyIndex,         \
        OUT SNET_CHT_POLICY_KEY_STC         * pclKeyPtr         \
    )

SNET_LION3_IPCL_BUILD_KEY_FUNC_MAC(lion3IPclBuildKey0 );
SNET_LION3_IPCL_BUILD_KEY_FUNC_MAC(lion3IPclBuildKey1 );
SNET_LION3_IPCL_BUILD_KEY_FUNC_MAC(lion3IPclBuildKey2 );
SNET_LION3_IPCL_BUILD_KEY_FUNC_MAC(lion3IPclBuildKey3 );
SNET_LION3_IPCL_BUILD_KEY_FUNC_MAC(lion3IPclBuildKey4 );
SNET_LION3_IPCL_BUILD_KEY_FUNC_MAC(lion3IPclBuildKey5 );
SNET_LION3_IPCL_BUILD_KEY_FUNC_MAC(lion3IPclBuildKey6 );
SNET_LION3_IPCL_BUILD_KEY_FUNC_MAC(lion3IPclBuildKey7 );
SNET_LION3_IPCL_BUILD_KEY_FUNC_MAC(lion3IPclBuildKey8 );
SNET_LION3_IPCL_BUILD_KEY_FUNC_MAC(lion3IPclBuildKey9 );
SNET_LION3_IPCL_BUILD_KEY_FUNC_MAC(lion3IPclBuildKey10);
SNET_LION3_IPCL_BUILD_KEY_FUNC_MAC(lion3IPclBuildKey11);
SNET_LION3_IPCL_BUILD_KEY_FUNC_MAC(lion3IPclBuildKey12);

static LION3_IPCL_KEY_INFO_FUN lion3IpclKeyInfoArr[DXCH_IPCL_KEY_LAST_E] =
{
     {lion3IPclBuildKey0 , "Standard (30B) L2"                  }
    ,{lion3IPclBuildKey1 , "Standard (30B) L2+IPv4/v6 QoS"      }
    ,{lion3IPclBuildKey2 , "Standard (30B) IPv4+L4"             }
    ,{lion3IPclBuildKey3 , "Standard (30B) 16 UDBs"             }
    ,{lion3IPclBuildKey4 , "Extended (60B) L2+IPv4 + L4"        }
    ,{lion3IPclBuildKey5 , "Extended (60B) L2+IPv6"             }
    ,{lion3IPclBuildKey6 , "Extended (60B) L4+IPv6"             }
    ,{lion3IPclBuildKey7 , "Extended (60B) 16 UDBs"             }
    ,{lion3IPclBuildKey8 , "Extended (60B) Port/VLAN+QoS+IPv4"  }
    ,{lion3IPclBuildKey9 , "Ultra (80B) Port/VLAN+QoS+IPv6"     }
    ,{lion3IPclBuildKey10, "Standard (30B) Routed ACL+QoS+IPv4" }
    ,{lion3IPclBuildKey11, "Ultra (80B) Routed ACL+QoS+IPv6"    }
    ,{lion3IPclBuildKey12, "Standard (30B) IPv6 DIP"            }
};

typedef GT_VOID (*XCAT_IPCL_BUILD_KEY_FUN)
(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC  * curLookUpCfgPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC    * pclExtraDataPtr,
    IN SNET_XCAT_IPCL_KEY_TYPE_ENT        keyType,
    OUT SNET_CHT_POLICY_KEY_STC         * pclKeyPtr
);

typedef struct{
    XCAT_IPCL_BUILD_KEY_FUN    buildKeyFunc;
    DXCH_IPCL_KEY_ENT           keyIndex;
    GT_U8                       nameString[50];
}XCAT_IPCL_KEY_INFO_FUN;

#define  SNET_XCAT_IPCL_BUILD_KEY_FUNC_MAC(funcName)           \
    static GT_VOID funcName                                    \
    (                                                          \
        IN SKERNEL_DEVICE_OBJECT            * devObjPtr,       \
        IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr,        \
        IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC  * curLookUpCfgPtr, \
        IN CHT_PCL_EXTRA_PACKET_INFO_STC    * pclExtraDataPtr, \
        IN SNET_XCAT_IPCL_KEY_TYPE_ENT        keyType,         \
        OUT SNET_CHT_POLICY_KEY_STC         * pclKeyPtr        \
    )

SNET_XCAT_IPCL_BUILD_KEY_FUNC_MAC(snetXCatIPclBuildL2StandardKey                  );
SNET_XCAT_IPCL_BUILD_KEY_FUNC_MAC(snetXCatIPclBuildL2L3StandardKey                );
SNET_XCAT_IPCL_BUILD_KEY_FUNC_MAC(snetXCatIPclBuildL3L4StandardKey                );
SNET_XCAT_IPCL_BUILD_KEY_FUNC_MAC(snetXCatIPclBuildL2L3L4ExtendedKey              );
SNET_XCAT_IPCL_BUILD_KEY_FUNC_MAC(snetXCatIPclKeyBuildL2IPv6ExtendedKey           );
SNET_XCAT_IPCL_BUILD_KEY_FUNC_MAC(snetXCatIPclKeyBuildL4IPv6ExtendedKey           );
SNET_XCAT_IPCL_BUILD_KEY_FUNC_MAC(snetXCatIPclKeyBuildIPortVlanQosIPv4ExtendedKey );
SNET_XCAT_IPCL_BUILD_KEY_FUNC_MAC(snetXCatIPclKeyBuildAclQosIPv4ShortKey          );
SNET_XCAT_IPCL_BUILD_KEY_FUNC_MAC(snetXCatIPclKeyBuildIPortVlanQosIPv6TripleKey   );
SNET_XCAT_IPCL_BUILD_KEY_FUNC_MAC(snetXCatIPclKeyBuildIAclQosIPv6TripleKey        );
SNET_XCAT_IPCL_BUILD_KEY_FUNC_MAC(snetXCatIPclKeyBuildIPv6DipShortKey             );
SNET_XCAT_IPCL_BUILD_KEY_FUNC_MAC(snetXCatIPclKeyBuildUdbShortKey                 );
SNET_XCAT_IPCL_BUILD_KEY_FUNC_MAC(snetXCatIPclKeyBuildUdbLongKey                  );

static XCAT_IPCL_KEY_INFO_FUN xcatIpclKeyInfoArr[XCAT_IPCL_KEY_TYPE_LAST_E] =
{
     {snetXCatIPclBuildL2StandardKey                   ,DXCH_IPCL_KEY_0_E ,"Standard (24B/26B) L2"                  }
    ,{snetXCatIPclBuildL2L3StandardKey                 ,DXCH_IPCL_KEY_1_E ,"Standard (24B/26B) L2+IPv4/v6 QoS"      }
    ,{snetXCatIPclBuildL3L4StandardKey                 ,DXCH_IPCL_KEY_2_E ,"Standard (24B/26B) IPv4+L4"             }
    ,{snetXCatIPclBuildL2L3L4ExtendedKey               ,DXCH_IPCL_KEY_4_E ,"Extended (48B/52B) L2+IPv4 + L4"        }
    ,{snetXCatIPclKeyBuildL2IPv6ExtendedKey            ,DXCH_IPCL_KEY_5_E ,"Extended (48B/52B) L2+IPv6"             }
    ,{snetXCatIPclKeyBuildL4IPv6ExtendedKey            ,DXCH_IPCL_KEY_6_E ,"Extended (48B/52B) L4+IPv6"             }
    ,{snetXCatIPclKeyBuildIPortVlanQosIPv4ExtendedKey  ,DXCH_IPCL_KEY_8_E ,"Extended (48B/52B) Port/VLAN+QoS+IPv4"  }
    ,{snetXCatIPclKeyBuildAclQosIPv4ShortKey           ,DXCH_IPCL_KEY_10_E,"Standard (24B/26B) Routed ACL+QoS+IPv4" }
    ,{snetXCatIPclKeyBuildIPortVlanQosIPv6TripleKey    ,DXCH_IPCL_KEY_9_E ,"Ultra (72B/78B) Port/VLAN+QoS+IPv6"     }
    ,{snetXCatIPclKeyBuildIAclQosIPv6TripleKey         ,DXCH_IPCL_KEY_11_E,"Ultra (72B/78B) Routed ACL+QoS+IPv6"    }
    ,{snetXCatIPclKeyBuildIPv6DipShortKey              ,DXCH_IPCL_KEY_12_E,"Standard (24B/26B) IPv6 DIP"            }
    ,{snetXCatIPclKeyBuildUdbShortKey                  ,DXCH_IPCL_KEY_3_E ,"Standard (48B/52B) 16 UDBs"             }
    ,{snetXCatIPclKeyBuildUdbLongKey                   ,DXCH_IPCL_KEY_7_E ,"Extended (48B/52B) 16 UDBs"             }
};

#define  SNET_LION3_EPCL_BUILD_KEY_FUNC_MAC(funcName)           \
    static GT_VOID funcName                                     \
    (                                                           \
        IN SKERNEL_DEVICE_OBJECT            * devObjPtr,        \
        IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr,         \
        IN  GT_U32                                egressPort,   \
        IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC  * lookupConfPtr,    \
        IN CHT_PCL_EXTRA_PACKET_INFO_STC    * pclExtraDataPtr,  \
        IN DXCH_EPCL_KEY_ENT                  keyIndex,         \
        OUT SNET_CHT_POLICY_KEY_STC         * pclKeyPtr         \
    )

typedef GT_VOID (*LION3_EPCL_BUILD_KEY_FUN)
(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr,
    IN  GT_U32                                egressPort,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC  * lookupConfPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC    * pclExtraDataPtr,
    IN DXCH_EPCL_KEY_ENT                  keyIndex,
    OUT SNET_CHT_POLICY_KEY_STC         * pclKeyPtr
);

typedef struct{
    LION3_EPCL_BUILD_KEY_FUN    buildKeyFunc;
    GT_U8                       nameString[50];
}LION3_EPCL_KEY_INFO_FUN;


SNET_LION3_EPCL_BUILD_KEY_FUNC_MAC(lion3EPclBuildKey0 );
SNET_LION3_EPCL_BUILD_KEY_FUNC_MAC(lion3EPclBuildKey1 );
SNET_LION3_EPCL_BUILD_KEY_FUNC_MAC(lion3EPclBuildKey2 );
SNET_LION3_EPCL_BUILD_KEY_FUNC_MAC(lion3EPclBuildKey3 );
SNET_LION3_EPCL_BUILD_KEY_FUNC_MAC(lion3EPclBuildKey4 );
SNET_LION3_EPCL_BUILD_KEY_FUNC_MAC(lion3EPclBuildKey5 );
SNET_LION3_EPCL_BUILD_KEY_FUNC_MAC(lion3EPclBuildKey6 );
SNET_LION3_EPCL_BUILD_KEY_FUNC_MAC(lion3EPclBuildKey7 );

static LION3_EPCL_KEY_INFO_FUN lion3EpclKeyInfoArr[CHT3_EPCL_KEY_TYPE_LAST_E] =
{
     {lion3EPclBuildKey0, "Standard (30B) L2             "}
    ,{lion3EPclBuildKey1, "Standard (30B) L2+IPv4/v6 QoS "}
    ,{lion3EPclBuildKey2, "Standard (30B) IPv4+L4        "}
    ,{lion3EPclBuildKey3, "Extended (60B) L2+IPv4+L4     "}
    ,{lion3EPclBuildKey4, "Extended (60B) L2+IPv6        "}
    ,{lion3EPclBuildKey5, "Extended (60B) L4+IPv6        "}
    ,{lion3EPclBuildKey6, "Extended (60B) RACL/VACL IPv4 "}
    ,{lion3EPclBuildKey7, "Ultra (80B) RACL/VACL IPv6    "}
};


/* Array that holds the info about the tcam key fields */
static CHT_PCL_KEY_FIELDS_INFO_STC xcatPclKeyFieldsData[]=
{
    /* common fields */
    { 0, 9,      GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_PCL_ID_E                 " },
    { 0, 7,      GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_PORT_LIST_7_0_E          " },
    { 10,10,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_MAC_TO_ME_E              " },
    { 11,16,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_SRC_PORT_E               " },
    { 11,16,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_PORT_LIST_13_8_E         " },
    { 17,17,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IS_TAGGED_E              " },
    { 18,29,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_VID_E                    " },
    { 30,32,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_UP_E                     " },
    { 33,39,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_QOS_PROFILE_E            " },
    { 40,40,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IS_IPV4_E                " },
    { 41,41,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IS_IP_E                  " },
    { 42,47,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_DSCP_E                   " },
    { 48,55,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IP_PROTOCOL_E            " },
    { 80,103,    GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_STD_KEY_MAC_SA_23_0_E    " },
    { 105,105,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_STD_KEY_IS_ARP_E         " },
    { 106,106,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_USER_DEFINED_VALID_E     " },
    { 107,107,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IS_L2_VALID_E            " },
    { 120,143,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_STD_KEY_MAC_SA_47_24_E   " },
    { 144,191,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_STD_KEY_MAC_DA_E         " },
    { 192,205,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_PORT_LIST_27_14_E        " },

    /* reserved fields */
    { 55,55,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_RESERVED_55_E            " },
    { 58,63,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_RESERVED_58_63_E         " },
    { 59,63,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_RESERVED_59_63_E         " },
    { 104,104,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_RESERVED_104_E           " },
    { 109,111,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_RESERVED_109_111_XCAT2_110_111_E       " },
    { 110,111,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_RESERVED_110_111_E       " },
    { 111,111,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_RESERVED_111_E           " },
    { 188,188,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_RESERVED_188_E           " },
    { 190,190,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_RESERVED_190_E           " },
    { 192,247,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_RESERVED_192_247_E       " },
    { 313,319,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_RESERVED_313_319_E       " },
    { 475,475,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_RESERVED_475_E           " },


    /* Key#0 Standard (24B) L2 - Ingress */
    { 42,57,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_KEY0_ETHER_TYPE_DSAP_SSAP_E"},
    { 64,71,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_KEY0_UDB_15_E            " },
    { 72,79,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_KEY0_UDB_16_E            " },
    { 108,108,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_KEY0_ENCAP_TYPE_E        " },
    { 112,119,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_KEY0_UDB_17_E            " },

                /* Key#1 Standard (24B) L2+IPv4/v6 QoS - Ingress */
    { 56,63,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_UDB_18_E                 " },
    { 64,79,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_3_2_E     " },
    { 104,104,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IS_L4_VALID_E            " },
    { 108,108,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IPV4_FRAGMENT_E          " },
    { 109,109,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IP_HEADER_OK_E           " },
    { 110,110,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IS_IPV6_EH_EXISTS_E      " },
    { 111,111,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_LONG_IS_IPV6_EH_HOP_BY_HOP_E " },
    { 112,119,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_UDB_19_E                 " },

                /* Key#2 Standard (24B) IPv4+L4 - Ingress */
    { 80,103,    GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_13_1_0_E  " },
    { 110,110,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IS_BC_E                  " },
    { 112,143,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_SIP_31_0_E               " },
    { 144,175,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_DIP_31_0_E               " },
    { 176,183,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_UDB_21_KEY2_E           " },
    { 184,191,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_UDB_22_KEY2_E           " },

                /* Key#3 Standard (24B) 16 UDBs - Ingress */
    { 33, 33,    GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IS_PRIORITY_TAGGED_E                " },
    { 34, 34,    GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_L3_OFFSET_INVALID_E                 " },
    { 35, 36,    GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_L4_PROTOCOL_OR_VALID_E              " },
    { 37, 38,    GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_APPLICABLE_FLOW_SUB_TEMPLATE_1_0_E  " },
    { 39, 39,    GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IP_HEADER_OK_KEY3_E                " },
    { 48, 79,    GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_UDB_0_2_5_KEY3_E                     " },
    { 80, 81,    GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_MAC_DA_TYPE_E                       " },
    { 82, 82,    GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_L4_OFFSET_INVALID_E                 " },
    { 83, 88,    GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_APPLICABLE_FLOW_SUB_TEMPLATE_7_2_E  " },
    { 89, 90,    GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_L2_ENCAP_0_KEY3_E                  " },
    { 91, 91,    GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IS_IPV6_EH_HOP_BY_HOP_KEY3_E       " },
    { 92, 92,    GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IS_IPV6_EH_EXISTS_KEY3_E           " },
    { 94, 94,    GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_USER_DEFINED_VALID_KEY3_E          " },
    { 95, 95,    GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IS_L2_VALID_KEY3_E                 " },
    { 96, 191,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_KEY3_UDB_3_4_6_15_E                 " },

                /* Key#4 Extended (48B) L2+IPv4+L4 - Ingress */
    { 40,40,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IS_IPV6_E                " },
    { 56,63,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_UDB_5_E                  " },
    { 105,105,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_ENCAP_TYPE_LONG_E        " },
    { 176,191,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_ETHERTYPE_E              " },
    { 248,295,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_MAC_SA_E                 " },
    { 296,343,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_MAC_DA_KEY4_E           " },
    { 344,351,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_UDB_1_E                  " },
    { 352,383,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_UDB_0_4_3_2_E            " },
                /* xCat2 modified fields - Key#4 */
    { 110,110,   GT_TRUE,       "XCAT2_PCL_KEY_FIELDS_ID_ENCAP_1_KEY4_E         " },

                /* Key#5 Extended (48B) L2+IPv6  - Ingress */
    { 56 ,63,    GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_UDB_11_E                 " },
    { 105,105,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IS_IPV6_EH_EXISTS_LONG_E " },
    { 108,108,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_LONG_IS_IPV6_EH_HOP_BY_HOP_KEY5_E  " },
    { 112,143,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_SIP_31_0_KEY5_E          " },
    { 144,175,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_SIP_63_32_KEY5_E         " },
    { 176,207,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_SIP_95_64_KEY5_E         " },
    { 208,239,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_SIP_127_96_KEY5_E        " },
    { 240,247,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_DIP_127_120_E            " },
    { 344,351,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_UDB_6_E                  " },
    { 352,383,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_UDB_10_9_8_7_E           " },
                /* xCat2 modified fields - Key#5 */
    {   0,999,   GT_TRUE,       "XCAT2_PCL_KEY_FIELDS_ID_SIP_79_64_KEY5_E        " }, /* xCat2 only */
    {   0,999,   GT_TRUE,       "XCAT2_PCL_KEY_FIELDS_ID_SIP_95_80_KEY5_E        " }, /* xCat2 only */

                /* Key#6 Extended (48B) L4+IPv6  - Ingress */
    { 56 ,63,    GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_UDB_14_E                 " },
    { 176,207,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_SIP_63_32_KEY6_E        " },
    { 208,239,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_SIP_95_64_KEY6_E        " },
    { 240,271,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_SIP_127_96_KEY6_E       " },
    { 272,279,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_DIP_127_120_KEY6_E      " },
    { 280,311,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_DIP_63_32_E              " },
    { 312,343,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_DIP_95_64_E              " },
    { 344,367,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_DIP_119_96_E             " },
    { 368,375,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_UDB_12_KEY6_E           " },
    { 376,383,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_UDB_13_E                 " },
                /* xCat2 modified fields - Key#6 */
    { 176,191,   GT_TRUE,       "XCAT2_PCL_KEY_FIELDS_ID_SIP_47_32_KEY6_E       " },
    { 206,221,   GT_TRUE,       "XCAT2_PCL_KEY_FIELDS_ID_SIP_63_48_KEY6_E       " },

                /* Key#7    Extended (48B) 16 UDBs - Ingress */
    { 56, 79,    GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_UDB_1_2_5_KEY7_E        " },
    { 96, 143,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_UDB_3_4_6_9_KEY7_E      " },
    { 144,175,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_SIP_31_0_KEY7_E         " },
    { 176,207,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_SIP_63_32_KEY7_E        " },
    { 208,239,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_SIP_95_64_KEY7_E        " },
    { 240,271,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_SIP_127_96_KEY7_E       " },
    { 176,223,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_MAC_SA_KEY7_E           " },
    { 224,271,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_MAC_DA_KEY7_E           " },
    { 272,287,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_DIP_127_112_KEY7_E      " },
    { 288,319,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_DIP_31_0_KEY7_E         " },
    { 320,375,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_UDB_0_15__10_KEY7_E     " },
                /* xCat2 modified fields - Key#7 */
    { 176,191,   GT_TRUE,       "XCAT2_PCL_KEY_FIELDS_ID_MAC_SA_15_0_KEY7_E     " },
    { 206,237,   GT_TRUE,       "XCAT2_PCL_KEY_FIELDS_ID_MAC_SA_47_16_KEY7_E    " },
    { 176,191,   GT_TRUE,       "XCAT2_PCL_KEY_FIELDS_ID_SIP_47_32_KEY7_E       " },
    { 206,221,   GT_TRUE,       "XCAT2_PCL_KEY_FIELDS_ID_SIP_63_48_KEY7_E       " },

    /* Key#8 Extended (48B) Port/VLAN+QoS+IPv4  - Ingress */
    { 33,38,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_PCKT_TOS_2_7_E              " },
    { 39,39,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IS_ARP_KEY8_E           " },
    { 40,40,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IS_BC_KEY8_E            " },
    { 42,49,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IP_PROTOCOL_KEY8_E      " },
    { 50,50,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_PCKT_TOS_0_E                " },
    { 51,52,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_IP_FRAGMENTED_E             " },
    { 53,54,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_IP_HEADER_INFO_E            " },
    { 55,55,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IS_L2_VALID_KEY8_E      " },
    { 56,56,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IS_L4_VALID_KEY8_E      " },
    { 57,57,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_USER_DEFINED_VALID_KEY8_E " },
    { 58,58,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_PCKT_TOS_1_E                " },
    { 104,111,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_UDB_13_KEY8_E           " },
    { 176,223,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_MAC_SA_KEY8_E           " },
    { 224,271,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_MAC_DA_KEY8_E           " },
    { 272,287,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_ETHERTYPE_KEY8_E        " },
    { 288,288,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_L2_ENCAP_0_E             " },
    { 289,296,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_UDB14_E                  " },
    { 297,312,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_UDB_16_15_E              " },

                /* Key#9 Ultra (72B) Port/VLAN+QoS+IPv6 */
    { 39,39,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_L2_ENCAP_0_KEY9_E       " },
    { 51,51,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IS_IPV6_EH_HOP_BY_HOP_E  " },
    { 52,52,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IS_IPV6_EH_EXISTS_KEY9_E" },
    { 53,53,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IS_ND_E                  " },
    { 54,54,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_IP_HEADER_OK_E              " },
    { 104,111,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_UDB_12_E                 " },
    { 288,319,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_SIP_63_32_KEY9_E        " },
    { 320,351,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_SIP_95_64_KEY9_E        " },
    { 352,383,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_SIP_127_96_KEY9_E       " },
    { 384,415,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_DIP_63_32_KEY9_E        " },
    { 416,447,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_DIP_95_64_KEY9_E        " },
    { 448,479,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_DIP_127_96_KEY9_E       " },

                /* Key#10 Standard (24B) Routed ACL+QoS+IPv4 */
    { 39,41,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_PCKT_TYPE_E                 " },
    { 57,62,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_IP_PCKT_LEN_5_0_E           " },
    { 63,63,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_PCKT_TOS_1_KEY10_E         " },
    { 104,111,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_UDB_5_KEY10_E           " },
    { 176,183,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_IP_PCKT_LEN_13_6_E          " },
    { 184,186,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_TTL_0_2_E                " },
    { 187,191,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_TTL_3_7_E                " },

                /* Key#11 Ultra (72B) Routed ACL+QoS+IPv6 */
    { 51,54,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_IP_PCKT_LEN_3_0_E           " },
    { 104,111,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_UDB_18_KEY11_E          " },
    { 176,183,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_UDB_19_KEY11_E          " },
    { 184,215,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_UDB_17_22_21_20_E        " },
    { 216,216,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_PCKT_TOS_1_KEY11_E         " },
    { 217,217,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IS_ND_KEY11_E           " },
    { 218,237,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IPV6_HEADER_FLOW_LBL_E   " },
    { 238,238,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IS_IPV6_LINK_LOCAL_E     " },
    { 239,258,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_MPLS_OUTER_LBL_E         " },
    { 259,261,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_MPLS_OUTER_LBL_EXP_E     " },
    { 262,262,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_MPLS_OUTER_LBL_S_BIT_E   " },
    { 263,270,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_IP_PCKT_LEN_11_4_E          " },
    { 271,278,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_TTL_E                    " },
    { 279,310,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_SIP_63_32_KEY11_E       " },
    { 311,342,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_SIP_95_64_KEY11_E       " },
    { 343,374,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_SIP_127_96_KEY11_E      " },
    { 375,406,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_DIP_63_32_KEY11_E       " },
    { 407,438,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_DIP_95_64_KEY11_E       " },
    { 439,470,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_DIP_127_96_KEY11_E      " },
    { 471,474,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_IP_PCKT_LEN_15_12_E         " },
    { 476,476,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IS_IPV6_MLD_E            " },
    { 477,477,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IS_IPV6_EH_HOP_BY_HOP_KEY11_E" },
    { 478,478,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IS_IPV6_EH_EXISTS_KEY11_E" },
    { 479,479,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IP_HEADER_OK_KEY11_E    " },

                /* Key#12 Standard (24B) IPv6 DIP */
    { 50,55,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_PCKT_DSCP_E                 " },
    { 57,72,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_DIP_15_0_E               " },
    { 73,73,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IS_ARP_KEY12_E          " },
    { 74,74,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IS_IPV6_EH_EXISTS_KEY12_E  " },
    { 75,90,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_DIP_31_16_E               " },
    { 91,91,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_LONG_IS_IPV6_EH_HOP_BY_HOP_KEY12_E " },
    { 92,123,    GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_DIP_63_32_KEY12_E       " },
    { 124,155,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_DIP_95_64_KEY12_E       " },
    { 156,187,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_DIP_127_96_KEY12_E      " },
    { 189,189,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IS_L2_VALID_KEY12_E     " },
    { 191,191,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_IP_HEADER_OK_KEY12_E       " }
};

/* Array that holds the info about the tcam key fields - xCat2 format */
static CHT_PCL_KEY_FIELDS_INFO_STC xcat2PclKeyFieldsData[]=
{
    /* common fields */
    { 0, 9,      GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_PCL_ID_E                 " },
    { 0, 7,      GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_PORT_LIST_7_0_E          " },
    { 10,10,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_MAC_TO_ME_E              " },
    { 11,16,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_SRC_PORT_E               " },
    { 11,16,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_PORT_LIST_13_8_E         " },
    { 17,17,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IS_TAGGED_E              " },
    { 18,29,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_VID_E                    " },
    { 30,32,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_UP_E                     " },
    { 33,39,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_QOS_PROFILE_E            " },
    { 40,40,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IS_IPV4_E                " },
    { 41,41,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IS_IP_E                  " },
    { 42,47,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_DSCP_E                   " },
    { 48,55,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IP_PROTOCOL_E            " },
    { 80,103,    GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_STD_KEY_MAC_SA_23_0_E    " },
    { 105,105,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_STD_KEY_IS_ARP_E         " },
    { 106,106,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_USER_DEFINED_VALID_E     " },
    { 107,107,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IS_L2_VALID_E            " },
    { 120,143,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_STD_KEY_MAC_SA_47_24_E   " },
    { 144,191,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_STD_KEY_MAC_DA_E         " },
    { 192,205,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_PORT_LIST_27_14_E        " },

    /* reserved fields */
    { 55,55,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_RESERVED_55_E            " },
    { 58,63,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_RESERVED_58_63_E         " },
    { 59,63,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_RESERVED_59_63_E         " },
    { 104,104,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_RESERVED_104_E           " },
    { 109,111,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_RESERVED_109_111_XCAT2_110_111_E       " },
    { 110,111,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_RESERVED_110_111_E       " },
    { 111,111,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_RESERVED_111_E           " },
    { 188,188,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_RESERVED_188_E           " },
    { 190,190,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_RESERVED_190_E           " },
    { 192,247,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_RESERVED_192_247_E       " },
    { 313,319,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_RESERVED_313_319_E       " },
    { 475,475,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_RESERVED_475_E           " },


    /* Key#0 Standard (24B) L2 - Ingress */
    { 42,57,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_KEY0_ETHER_TYPE_DSAP_SSAP_E"},
    { 64,71,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_KEY0_UDB_15_E            " },
    { 72,79,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_KEY0_UDB_16_E            " },
    { 108,109,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_KEY0_ENCAP_TYPE_E        " },
    { 112,119,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_KEY0_UDB_17_E            " },

                /* Key#1 Standard (24B) L2+IPv4/v6 QoS - Ingress */
    { 56,63,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_UDB_18_E                 " },
    { 64,79,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_3_2_E     " },
    { 104,104,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IS_L4_VALID_E            " },
    { 108,108,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IPV4_FRAGMENT_E          " },
    { 109,109,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IP_HEADER_OK_E           " },
    { 110,110,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IS_IPV6_EH_EXISTS_E      " },
    { 111,111,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_LONG_IS_IPV6_EH_HOP_BY_HOP_E " },
    { 112,119,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_UDB_19_E                 " },

                /* Key#2 Standard (24B) IPv4+L4 - Ingress */
    { 80,103,    GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_13_1_0_E  " },
    { 110,110,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IS_BC_E                  " },
    { 112,143,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_SIP_31_0_E               " },
    { 144,175,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_DIP_31_0_E               " },
    { 176,183,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_UDB_21_KEY2_E           " },
    { 184,191,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_UDB_22_KEY2_E           " },

                /* Key#3 Standard (24B) 16 UDBs - Ingress */
    { 33, 33,    GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IS_PRIORITY_TAGGED_E                " },
    { 34, 34,    GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_L3_OFFSET_INVALID_E                 " },
    { 35, 36,    GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_L4_PROTOCOL_OR_VALID_E              " },
    { 37, 38,    GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_APPLICABLE_FLOW_SUB_TEMPLATE_1_0_E  " },
    { 39, 39,    GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IP_HEADER_OK_KEY3_E                " },
    { 48, 79,    GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_UDB_0_2_5_KEY3_E                     " },
    { 80, 81,    GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_MAC_DA_TYPE_E                       " },
    { 82, 82,    GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_L4_OFFSET_INVALID_E                 " },
    { 83, 88,    GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_APPLICABLE_FLOW_SUB_TEMPLATE_7_2_E  " },
    { 89, 90,    GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_L2_ENCAP_0_KEY3_E                  " },
    { 91, 91,    GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IS_IPV6_EH_HOP_BY_HOP_KEY3_E       " },
    { 92, 92,    GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IS_IPV6_EH_EXISTS_KEY3_E           " },
    { 94, 94,    GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_USER_DEFINED_VALID_KEY3_E          " },
    { 95, 95,    GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IS_L2_VALID_KEY3_E                 " },
    { 96, 191,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_KEY3_UDB_3_4_6_15_E                 " },

                /* Key#4 Extended (48B) L2+IPv4+L4 - Ingress */
    { 40,40,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IS_IPV6_E                " },
    { 56,63,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_UDB_5_E                  " },
    { 105,105,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_ENCAP_TYPE_LONG_E        " },
    { 176,191,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_ETHERTYPE_E              " },
    { 262,309,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_MAC_SA_E                 " },
    { 310,357,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_MAC_DA_KEY4_E           " },
    { 358,365,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_UDB_1_E                  " },
    { 366,397,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_UDB_0_4_3_2_E            " },
                /* xCat2 modified fields - Key#4 */
    { 110,110,   GT_TRUE,       "XCAT2_PCL_KEY_FIELDS_ID_ENCAP_1_KEY4_E         " },

                /* Key#5 Extended (48B) L2+IPv6  - Ingress */
    { 56 ,63,    GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_UDB_11_E                 " },
    { 105,105,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IS_IPV6_EH_EXISTS_LONG_E " },
    { 108,108,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_LONG_IS_IPV6_EH_HOP_BY_HOP_KEY5_E  " },
    { 112,143,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_SIP_31_0_KEY5_E          " },
    { 144,175,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_SIP_63_32_KEY5_E         " },
    {   0,999,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_SIP_95_64_KEY5_E         " },  /* not used */
    { 222,253,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_SIP_127_96_KEY5_E        " },
    { 254,261,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_DIP_127_120_E            " },
    { 358,365,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_UDB_6_E                  " },
    { 366,397,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_UDB_10_9_8_7_E           " },
                /* xCat2 modified fields - Key#5 */
    { 176,191,   GT_TRUE,       "XCAT2_PCL_KEY_FIELDS_ID_SIP_79_64_KEY5_E        " }, /* xCat2 only */
    { 206,221,   GT_TRUE,       "XCAT2_PCL_KEY_FIELDS_ID_SIP_95_80_KEY5_E        " }, /* xCat2 only */

                /* Key#6 Extended (48B) L4+IPv6  - Ingress */
    { 56 ,63,    GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_UDB_14_E                 " },
    { 176,207,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_SIP_63_32_KEY6_E        " },
    { 222,253,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_SIP_95_64_KEY6_E        " },
    { 254,285,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_SIP_127_96_KEY6_E       " },
    { 286,293,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_DIP_127_120_KEY6_E      " },
    { 294,325,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_DIP_63_32_E              " },
    { 326,357,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_DIP_95_64_E              " },
    { 358,381,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_DIP_119_96_E             " },
    { 382,389,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_UDB_12_KEY6_E           " },
    { 390,397,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_UDB_13_E                 " },
                /* xCat2 modified fields - Key#6 */
    { 176,191,   GT_TRUE,       "XCAT2_PCL_KEY_FIELDS_ID_SIP_47_32_KEY6_E       " },
    { 206,221,   GT_TRUE,       "XCAT2_PCL_KEY_FIELDS_ID_SIP_63_48_KEY6_E       " },

                /* Key#7    Extended (48B) 16 UDBs - Ingress */
    { 56, 79,    GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_UDB_1_2_5_KEY7_E        " },
    { 96, 143,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_UDB_3_4_6_9_KEY7_E      " },
    { 144,175,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_SIP_31_0_KEY7_E         " },
    { 176,207,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_SIP_63_32_KEY7_E        " },
    { 222,253,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_SIP_95_64_KEY7_E        " },
    { 254,285,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_SIP_127_96_KEY7_E       " },
    { 176,223,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_MAC_SA_KEY7_E           " },
    { 238,285,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_MAC_DA_KEY7_E           " },
    { 286,301,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_DIP_127_112_KEY7_E      " },
    { 302,333,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_DIP_31_0_KEY7_E         " },
    { 334,389,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_UDB_0_15__10_KEY7_E     " },
                /* xCat2 modified fields - Key#7 */
    { 176,191,   GT_TRUE,       "XCAT2_PCL_KEY_FIELDS_ID_MAC_SA_15_0_KEY7_E     " },
    { 206,237,   GT_TRUE,       "XCAT2_PCL_KEY_FIELDS_ID_MAC_SA_47_16_KEY7_E    " },
    { 176,191,   GT_TRUE,       "XCAT2_PCL_KEY_FIELDS_ID_SIP_47_32_KEY7_E       " },
    { 206,221,   GT_TRUE,       "XCAT2_PCL_KEY_FIELDS_ID_SIP_63_48_KEY7_E       " },

    /* Key#8 Extended (48B) Port/VLAN+QoS+IPv4  - Ingress */
    { 33,38,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_PCKT_TOS_2_7_E              " },
    { 39,39,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IS_ARP_KEY8_E           " },
    { 40,40,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IS_BC_KEY8_E            " },
    { 42,49,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IP_PROTOCOL_KEY8_E      " },
    { 50,50,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_PCKT_TOS_0_E                " },
    { 51,52,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_IP_FRAGMENTED_E             " },
    { 53,54,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_IP_HEADER_INFO_E            " },
    { 55,55,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IS_L2_VALID_KEY8_E      " },
    { 56,56,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IS_L4_VALID_KEY8_E      " },
    { 57,57,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_USER_DEFINED_VALID_KEY8_E " },
    { 58,58,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_PCKT_TOS_1_E                " },
    { 104,111,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_UDB_13_KEY8_E           " },
    { 176,223,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_MAC_SA_KEY8_E           " },
    { 224,271,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_MAC_DA_KEY8_E           " },
    { 272,287,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_ETHERTYPE_KEY8_E        " },
    { 288,288,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_L2_ENCAP_0_E             " },
    { 289,296,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_UDB14_E                  " },
    { 297,312,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_UDB_16_15_E              " },

                /* Key#9 Ultra (72B) Port/VLAN+QoS+IPv6 */
    { 39,39,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_L2_ENCAP_0_KEY9_E       " },
    { 51,51,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IS_IPV6_EH_HOP_BY_HOP_E  " },
    { 52,52,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IS_IPV6_EH_EXISTS_KEY9_E" },
    { 53,53,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IS_ND_E                  " },
    { 54,54,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_IP_HEADER_OK_E              " },
    { 104,111,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_UDB_12_E                 " },
    { 288,319,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_SIP_63_32_KEY9_E        " },
    { 320,351,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_SIP_95_64_KEY9_E        " },
    { 352,383,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_SIP_127_96_KEY9_E       " },
    { 384,415,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_DIP_63_32_KEY9_E        " },
    { 416,447,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_DIP_95_64_KEY9_E        " },
    { 448,479,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_DIP_127_96_KEY9_E       " },

                /* Key#10 Standard (24B) Routed ACL+QoS+IPv4 */
    { 39,41,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_PCKT_TYPE_E                 " },
    { 57,62,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_IP_PCKT_LEN_5_0_E           " },
    { 63,63,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_PCKT_TOS_1_KEY10_E         " },
    { 104,111,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_UDB_5_KEY10_E           " },
    { 176,183,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_IP_PCKT_LEN_13_6_E          " },
    { 184,186,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_TTL_0_2_E                " },
    { 187,191,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_TTL_3_7_E                " },

                /* Key#11 Ultra (72B) Routed ACL+QoS+IPv6 */
    { 51,54,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_IP_PCKT_LEN_3_0_E           " },
    { 104,111,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_UDB_18_KEY11_E          " },
    { 176,183,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_UDB_19_KEY11_E          " },
    { 184,215,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_UDB_17_22_21_20_E        " },
    { 216,216,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_PCKT_TOS_1_KEY11_E         " },
    { 217,217,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IS_ND_KEY11_E           " },
    { 218,237,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IPV6_HEADER_FLOW_LBL_E   " },
    { 238,238,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IS_IPV6_LINK_LOCAL_E     " },
    { 239,258,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_MPLS_OUTER_LBL_E         " },
    { 259,261,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_MPLS_OUTER_LBL_EXP_E     " },
    { 262,262,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_MPLS_OUTER_LBL_S_BIT_E   " },
    { 263,270,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_IP_PCKT_LEN_11_4_E          " },
    { 271,278,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_TTL_E                    " },
    { 279,310,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_SIP_63_32_KEY11_E       " },
    { 311,342,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_SIP_95_64_KEY11_E       " },
    { 343,374,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_SIP_127_96_KEY11_E      " },
    { 375,406,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_DIP_63_32_KEY11_E       " },
    { 407,438,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_DIP_95_64_KEY11_E       " },
    { 439,470,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_DIP_127_96_KEY11_E      " },
    { 471,474,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_IP_PCKT_LEN_15_12_E         " },
    { 476,476,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IS_IPV6_MLD_E            " },
    { 477,477,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IS_IPV6_EH_HOP_BY_HOP_KEY11_E" },
    { 478,478,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IS_IPV6_EH_EXISTS_KEY11_E" },
    { 479,479,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IP_HEADER_OK_KEY11_E    " },

                /* Key#12 Standard (24B) IPv6 DIP */
    { 50,55,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_PCKT_DSCP_E                 " },
    { 57,72,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_DIP_15_0_E               " },
    { 73,73,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IS_ARP_KEY12_E          " },
    { 74,74,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IS_IPV6_EH_EXISTS_KEY12_E  " },
    { 75,90,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_DIP_31_16_E               " },
    { 91,91,     GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_LONG_IS_IPV6_EH_HOP_BY_HOP_KEY12_E " },
    { 92,123,    GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_DIP_63_32_KEY12_E       " },
    { 124,155,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_DIP_95_64_KEY12_E       " },
    { 156,187,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_DIP_127_96_KEY12_E      " },
    { 189,189,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_ID_IS_L2_VALID_KEY12_E     " },
    { 191,191,   GT_TRUE,       "XCAT_PCL_KEY_FIELDS_IP_HEADER_OK_KEY12_E       " },

};

/* Array that holds the info about the tcam UDB key that may be override by the 'trunk hash' */
static CHT_PCL_KEY_FIELDS_INFO_STC lionPclUdbKeyTrunkHashFields[DXCH_IPCL_KEY_LAST_E]=
{
        {112, 119,  GT_TRUE,  "trunk hash in KEY0_UDB_17_E        " },
        {SMAIN_NOT_VALID_CNS,   0,  GT_TRUE,  "trunk hash in KEY1_UDB_RESERVED_E  " },
        {184, 191,  GT_TRUE,  "trunk hash in KEY2_UDB_22_E        " },
        { 96, 103,  GT_TRUE,  "trunk hash in KEY3_UDB_3_E         " },
        {360, 367,  GT_TRUE,  "trunk hash in KEY4_UDB_3_E         " },
        {352, 359,  GT_TRUE,  "trunk hash in KEY5_UDB_7_E         " },
        {376, 383,  GT_TRUE,  "trunk hash in KEY6_UDB_13_E        " },
        { 96, 103,  GT_TRUE,  "trunk hash in KEY7_UDB_3_E         " },
        {297, 304,  GT_TRUE,  "trunk hash in KEY8_UDB_15_E        " },
        {488, 495,  GT_TRUE,  "trunk hash in KEY9_UDB_1_E         " },
        {104, 111,  GT_TRUE,  "trunk hash in KEY10_UDB_5_E        " },
        {184, 191,  GT_TRUE,  "trunk hash in KEY11_UDB_20_E       " },
        {SMAIN_NOT_VALID_CNS,   0,  GT_TRUE,  "trunk hash in KEY12_UDB_RESERVED_E  " }
};

/* Array that holds the info about the tcam UDB key that may be override by the 'trunk hash' */
static GT_U32 lion3PclUdbKeyTrunkHashFieldsUdbIndex[DXCH_IPCL_KEY_LAST_E]=
{
        17,                 /*udb 17 in key 0 */
        SMAIN_NOT_VALID_CNS,/*no udb in key1 for trunk hash*/
        22,                 /*udb 22 in key 2 */
        3,                  /*udb 3 in key 3 */
        3,                  /*udb 3 in key 4 */
        7,                  /*udb 7 in key 5 */
        13,                 /*udb 13 in key 6 */
        3,                  /*udb 3 in key 7 */
        15,                 /*udb 15 in key 8 */
        1,                  /*udb 1 in key 9 */
        5,                  /*udb 5 in key 10 */
        20,                 /*udb 20 in key 11 */
        SMAIN_NOT_VALID_CNS /*no udb in key12 for trunk hash*/
};

/* Array that holds the info about the tcam key fields - lion3 format */
static CHT_PCL_KEY_FIELDS_INFO_STC lion3PclKeyFieldsData[]=
{
    /* NOTE the UDBs are not exists in this table */

    /* common field */
    /*common from key 0*/
     {0   ,   7   ,GT_TRUE,      "LION3_KEY_FIELD_NAME_BUILD( 0   ,   7   ,PCL_ID_7_0_OR_port_list_7_0      )" }
    ,{8   ,   9   ,GT_TRUE,      "LION3_KEY_FIELD_NAME_BUILD( 8   ,   9   ,PCL_ID_9_8                       )" }
    ,{10  ,  10   ,GT_TRUE,      "LION3_KEY_FIELD_NAME_BUILD( 10  ,  10   ,MAC2ME                           )" }
    ,{11  ,  16   ,GT_TRUE,      "LION3_KEY_FIELD_NAME_BUILD( 11  ,  16   ,SrcPort_5_0_OR_port_list_13_8    )" }
    ,{17  ,  17   ,GT_TRUE,      "LION3_KEY_FIELD_NAME_BUILD( 17  ,  17   ,IsTagged                         )" }
    ,{18  ,  29   ,GT_TRUE,      "LION3_KEY_FIELD_NAME_BUILD( 18  ,  29   ,VID0_OR_eVLAN_11_0               )" }
    ,{30  ,  32   ,GT_TRUE,      "LION3_KEY_FIELD_NAME_BUILD( 30  ,  32   ,UP0                              )" }
    ,{33  ,  39   ,GT_TRUE,      "LION3_KEY_FIELD_NAME_BUILD( 33  ,  39   ,QoS_Profile                      )" }
    ,{40  ,  40   ,GT_TRUE,      "LION3_KEY_FIELD_NAME_BUILD( 40  ,  40   ,IsIPv4_OR_IsFCoE                 )" }
    ,{41  ,  41   ,GT_TRUE,      "LION3_KEY_FIELD_NAME_BUILD( 41  ,  41   ,IsIP                             )" }
    ,{80  , 103   ,GT_TRUE,      "LION3_KEY_FIELD_NAME_BUILD( 80  , 103   ,MAC_SA_23_0                      )" }
    ,{105 , 105   ,GT_TRUE,      "LION3_KEY_FIELD_NAME_BUILD( 105 , 105   ,IsARP                            )" }
    ,{107 , 107   ,GT_TRUE,      "LION3_KEY_FIELD_NAME_BUILD( 107 , 107   ,Is_L2_Valid                      )" }
    ,{120 , 143   ,GT_TRUE,      "LION3_KEY_FIELD_NAME_BUILD( 120 , 143   ,MAC_SA_47_24                     )" }
    ,{144 , 191   ,GT_TRUE,      "LION3_KEY_FIELD_NAME_BUILD( 144 , 191   ,MAC_DA                           )" }
    ,{192 , 193   ,GT_TRUE,      "LION3_KEY_FIELD_NAME_BUILD( 192 , 193   ,port_list_15_14_OR_SrcPort_7_6   )" }
    ,{194 , 205   ,GT_TRUE,      "LION3_KEY_FIELD_NAME_BUILD( 194 , 205   ,port_list_27_16_OR_2b0_SrcDev_9_0)" }
    ,{42  ,  47   ,GT_TRUE,      "LION3_KEY_FIELD_NAME_BUILD( 42  ,  47   ,Packet_DSCP                      )" }
    ,{48  ,  55   ,GT_TRUE,      "LION3_KEY_FIELD_NAME_BUILD( 48  ,  55   ,IpProtocol                       )" }
    ,{64  ,  79   ,GT_TRUE,      "LION3_KEY_FIELD_NAME_BUILD( 64  ,  79   ,L4_Byte_Offset_3_2_OR_L4_Byte_Offset_5_4)" }
    ,{104 , 104   ,GT_TRUE,      "LION3_KEY_FIELD_NAME_BUILD( 104 , 104   ,Is_L4_Valid                      )" }
    ,{108 , 108   ,GT_TRUE,      "LION3_KEY_FIELD_NAME_BUILD( 108 , 108   ,IPv4_fragmented                  )" }
    ,{109 , 109   ,GT_TRUE,      "LION3_KEY_FIELD_NAME_BUILD( 109 , 109   ,IP_Hdr_OK                        )" }
    /* key0 (without UDBs) */
    ,{42  ,  57   ,GT_TRUE,      "LION3_KEY0_FIELD_NAME_BUILD(42  ,  57   ,Ethertype                        )" }
    ,{58  ,  63   ,GT_TRUE,      "LION3_KEY0_FIELD_NAME_BUILD(58  ,  63   ,Tag1_VID_5_0                     )" }
    ,{104 , 104   ,GT_TRUE,      "LION3_KEY0_FIELD_NAME_BUILD(104 , 104   ,UP1_0                            )" }
    ,{108 , 108   ,GT_TRUE,      "LION3_KEY0_FIELD_NAME_BUILD(108 , 108   ,L2_Encapsulation_0               )" }
    ,{109 , 109   ,GT_TRUE,      "LION3_KEY0_FIELD_NAME_BUILD(109 , 109   ,L2_Encapsulation_1               )" }
    ,{110 , 111   ,GT_TRUE,      "LION3_KEY0_FIELD_NAME_BUILD(110 , 111   ,UP1_2_1                          )" }
    /* key1 (without UDBs) */
    ,{110 , 110   ,GT_TRUE,      "LION3_KEY1_FIELD_NAME_BUILD(110 , 110   ,Is_IPv6_EH_Exist                 )" }
    ,{111 , 111   ,GT_TRUE,      "LION3_KEY1_FIELD_NAME_BUILD(111 , 111   ,Is_IPv6_EH_Hop_By_Hop            )" }
    /* key2 (without UDBs) */
    ,{80  ,  87   ,GT_TRUE,      "LION3_KEY2_FIELD_NAME_BUILD(80  ,  87   ,L4_Byte_Offsets_0                )" }
    ,{88  ,  95   ,GT_TRUE,      "LION3_KEY2_FIELD_NAME_BUILD(88  ,  95   ,L4_Byte_Offsets_1                )" }
    ,{96  , 103   ,GT_TRUE,      "LION3_KEY2_FIELD_NAME_BUILD(96  , 103   ,L4_Byte_Offsets_13               )" }
    ,{110 , 110   ,GT_TRUE,      "LION3_KEY2_FIELD_NAME_BUILD(110 , 110   ,Is_BC                            )" }
    ,{112 , 143   ,GT_TRUE,      "LION3_KEY2_FIELD_NAME_BUILD(112 , 143   ,SIP_31_0_OR_S_ID                 )" }
    ,{144 , 175   ,GT_TRUE,      "LION3_KEY2_FIELD_NAME_BUILD(144 , 175   ,DIP_31_0_OR_D_ID                 )" }
    /* key3 (without UDBs) */
    ,{33  , 33    ,GT_TRUE,      "LION3_KEY3_FIELD_NAME_BUILD(33  , 33    ,Packet_Tagging_1                 )" }
    ,{34  , 34    ,GT_TRUE,      "LION3_KEY3_FIELD_NAME_BUILD(34  , 34    ,L3_Offset_Invalid                )" }
    ,{35  , 36    ,GT_TRUE,      "LION3_KEY3_FIELD_NAME_BUILD(35  , 36    ,L4_Protocol_or_Valid             )" }
    ,{37  , 38    ,GT_TRUE,      "LION3_KEY3_FIELD_NAME_BUILD(37  , 38    ,Applicable_Flow_Sub_Template_1_0 )" }
    ,{39  , 39    ,GT_TRUE,      "LION3_KEY3_FIELD_NAME_BUILD(39  , 39    ,IP_Hdr_OK                        )" }
    ,{80  , 81    ,GT_TRUE,      "LION3_KEY3_FIELD_NAME_BUILD(80  , 81    ,MacDaType                        )" }
    ,{82  , 82    ,GT_TRUE,      "LION3_KEY3_FIELD_NAME_BUILD(82  , 82    ,L4_Offset_Invalid                )" }
    ,{83  , 88    ,GT_TRUE,      "LION3_KEY3_FIELD_NAME_BUILD(83  , 88    ,Applicable_Flow_Sub_Template_7_2 )" }
    ,{89  , 90    ,GT_TRUE,      "LION3_KEY3_FIELD_NAME_BUILD(89  , 90    ,L2_Encapsulation                 )" }
    ,{91  , 91    ,GT_TRUE,      "LION3_KEY3_FIELD_NAME_BUILD(91  , 91    ,Is_IPv6_EH_Hop_By_Hop            )" }
    ,{92  , 92    ,GT_TRUE,      "LION3_KEY3_FIELD_NAME_BUILD(92  , 92    ,Is_IPv6_EH_Exist                 )" }
    ,{95  , 95    ,GT_TRUE,      "LION3_KEY3_FIELD_NAME_BUILD(95  , 95    ,Is_L2_Valid                      )" }
    /* key4 (without UDBs) */
    ,{40  ,  40   ,GT_TRUE,      "LION3_KEY4_FIELD_NAME_BUILD(40  ,  40   ,IsIPv6_OR_IsFCoE                 )" }
    ,{64  ,  79   ,GT_TRUE,      "LION3_KEY4_FIELD_NAME_BUILD(64  ,  79   ,L4_Byte_Offset_3_2               )" }
    ,{105 , 105   ,GT_TRUE,      "LION3_KEY4_FIELD_NAME_BUILD(105 , 105   ,L2_Encapsulation_0               )" }
    ,{110 , 110   ,GT_TRUE,      "LION3_KEY4_FIELD_NAME_BUILD(110 , 110   ,L2_Encapsulation_1               )" }
    ,{176 , 191   ,GT_TRUE,      "LION3_KEY4_FIELD_NAME_BUILD(176 , 191   ,Ethertype                        )" }
    ,{208 , 210   ,GT_TRUE,      "LION3_KEY4_FIELD_NAME_BUILD(208 , 210   ,UP1                              )" }
    ,{264 , 311   ,GT_TRUE,      "LION3_KEY4_FIELD_NAME_BUILD(264 , 311   ,MAC_SA                           )" }
    ,{312 , 359   ,GT_TRUE,      "LION3_KEY4_FIELD_NAME_BUILD(312 , 359   ,MAC_DA                           )" }
    ,{400 , 400   ,GT_TRUE,      "LION3_KEY4_FIELD_NAME_BUILD(400 , 400   ,Tag1_Exist                       )" }
    ,{401 , 412   ,GT_TRUE,      "LION3_KEY4_FIELD_NAME_BUILD(401 , 412   ,Tag1_VID                         )" }
    ,{413 , 413   ,GT_TRUE,      "LION3_KEY4_FIELD_NAME_BUILD(413 , 413   ,Tag1_CFI                         )" }
    /* key5 (without UDBs) */
    ,{105 , 105   ,GT_TRUE,      "LION3_KEY5_FIELD_NAME_BUILD(105 , 105   ,Is_IPv6_EH_Exist                 )" }
    ,{108 , 108   ,GT_TRUE,      "LION3_KEY5_FIELD_NAME_BUILD(108 , 108   ,Is_IPv6_EH_Hop_By_Hop            )" }
    ,{110 , 111   ,GT_TRUE,      "LION3_KEY5_FIELD_NAME_BUILD(110 , 111   ,UP1_1_0                          )" }
    ,{144 , 175   ,GT_TRUE,      "LION3_KEY5_FIELD_NAME_BUILD(144 , 175   ,SIP_63_32                        )" }
    ,{176 , 191   ,GT_TRUE,      "LION3_KEY5_FIELD_NAME_BUILD(176 , 191   ,SIP_79_64                        )" }
    ,{208 , 223   ,GT_TRUE,      "LION3_KEY5_FIELD_NAME_BUILD(208 , 223   ,SIP_95_80                        )" }
    ,{224 , 255   ,GT_TRUE,      "LION3_KEY5_FIELD_NAME_BUILD(224 , 255   ,SIP_127_96                       )" }
    ,{256 , 263   ,GT_TRUE,      "LION3_KEY5_FIELD_NAME_BUILD(256 , 263   ,DIP_127_120                      )" }
    ,{413 , 413   ,GT_TRUE,      "LION3_KEY5_FIELD_NAME_BUILD(413 , 413   ,UP1_2                            )" }
    /* key6 (without UDBs) */
    ,{176 , 191   ,GT_TRUE,      "LION3_KEY6_FIELD_NAME_BUILD(176 ,  191  ,SIP_47_32                        )" }
    ,{208 , 223   ,GT_TRUE,      "LION3_KEY6_FIELD_NAME_BUILD(208 ,  223  ,SIP_63_48                        )" }
    ,{224 , 255   ,GT_TRUE,      "LION3_KEY6_FIELD_NAME_BUILD(224 ,  255  ,SIP_95_64                        )" }
    ,{256 , 287   ,GT_TRUE,      "LION3_KEY6_FIELD_NAME_BUILD(256 ,  287  ,SIP_127_96                       )" }
    ,{288 , 295   ,GT_TRUE,      "LION3_KEY6_FIELD_NAME_BUILD(288 ,  295  ,DIP_127_120                      )" }
    ,{296 , 327   ,GT_TRUE,      "LION3_KEY6_FIELD_NAME_BUILD(296 ,  327  ,DIP_63_32                        )" }
    ,{328 , 359   ,GT_TRUE,      "LION3_KEY6_FIELD_NAME_BUILD(328 ,  359  ,DIP_95_64                        )" }
    ,{360 , 383   ,GT_TRUE,      "LION3_KEY6_FIELD_NAME_BUILD(360 ,  383  ,DIP_119_96                       )" }
    /* key7 (without UDBs) */
    ,{17  ,  17   ,GT_TRUE,      "LION3_KEY7_FIELD_NAME_BUILD(17  ,  17   ,Packet_Tagging_0                 )" }
    ,{144 , 175   ,GT_TRUE,      "LION3_KEY7_FIELD_NAME_BUILD(144 , 175   ,SIP_31_0_OR_S_ID                 )" }
    ,{176 , 191   ,GT_TRUE,      "LION3_KEY7_FIELD_NAME_BUILD(176 , 191   ,MAC_SA_15_0                      )" }
    ,{208 , 239   ,GT_TRUE,      "LION3_KEY7_FIELD_NAME_BUILD(208 , 239   ,MAC_SA_47_32                     )" }
    ,{240 , 287   ,GT_TRUE,      "LION3_KEY7_FIELD_NAME_BUILD(240 , 287   ,MAC_DA                           )" }
    ,{288 , 303   ,GT_TRUE,      "LION3_KEY7_FIELD_NAME_BUILD(288 , 303   ,DIP_127_112                      )" }
    ,{304 , 335   ,GT_TRUE,      "LION3_KEY7_FIELD_NAME_BUILD(304 , 335   ,DIP_31_0_OR_D_ID                 )" }
    ,{397 , 399   ,GT_TRUE,      "LION3_KEY7_FIELD_NAME_BUILD(397 , 399   ,UP1                              )" }
    /* key8 (without UDBs) */
    ,{33  ,  38   ,GT_TRUE,      "LION3_KEY8_FIELD_NAME_BUILD(33  ,  38   ,PacketTOS_OR_DSCP_7_2            )" }
    ,{39  ,  39   ,GT_TRUE,      "LION3_KEY8_FIELD_NAME_BUILD(39  ,  39   ,Is_ARP                           )" }
    ,{40  ,  40   ,GT_TRUE,      "LION3_KEY8_FIELD_NAME_BUILD(40  ,  40   ,Is_BC                            )" }
    ,{42  ,  49   ,GT_TRUE,      "LION3_KEY8_FIELD_NAME_BUILD(42  ,  49   ,IpProtocol                       )" }
    ,{50  ,  50   ,GT_TRUE,      "LION3_KEY8_FIELD_NAME_BUILD(50  ,  50   ,PacketTOS_OR_DSCP_0              )" }
    ,{51  ,  52   ,GT_TRUE,      "LION3_KEY8_FIELD_NAME_BUILD(51  ,  52   ,IP_Fragmented                    )" }
    ,{53  ,  54   ,GT_TRUE,      "LION3_KEY8_FIELD_NAME_BUILD(53  ,  54   ,IP_Header_Info                   )" }
    ,{55  ,  55   ,GT_TRUE,      "LION3_KEY8_FIELD_NAME_BUILD(55  ,  55   ,Is_L2_Valid                      )" }
    ,{56  ,  56   ,GT_TRUE,      "LION3_KEY8_FIELD_NAME_BUILD(56  ,  56   ,Is_L4_Valid                      )" }
    ,{58  ,  58   ,GT_TRUE,      "LION3_KEY8_FIELD_NAME_BUILD(58  ,  58   ,PacketTOS_DSCP_1                 )" }
    ,{288 , 303   ,GT_TRUE,      "LION3_KEY8_FIELD_NAME_BUILD(288 , 303   ,Ethertype                        )" }
    ,{304 , 304   ,GT_TRUE,      "LION3_KEY8_FIELD_NAME_BUILD(304 , 304   ,L2_Encapsulation_0               )" }
    ,{329 , 329   ,GT_TRUE,      "LION3_KEY8_FIELD_NAME_BUILD(329 , 329   ,L2_Encapsulation_1               )" }
    ,{333 , 335   ,GT_TRUE,      "LION3_KEY8_FIELD_NAME_BUILD(333 , 335   ,UP1                              )" }
    ,{336 , 336   ,GT_TRUE,      "LION3_KEY8_FIELD_NAME_BUILD(336 , 336   ,Tag1_Exist                       )" }
    ,{337 , 348   ,GT_TRUE,      "LION3_KEY8_FIELD_NAME_BUILD(337 , 348   ,Tag1_VID                         )" }
    ,{349 , 349   ,GT_TRUE,      "LION3_KEY8_FIELD_NAME_BUILD(349 , 349   ,Tag1_CFI                         )" }
    /* key9 (without UDBs) */
    ,{39  ,  39   ,GT_TRUE,      "LION3_KEY9_FIELD_NAME_BUILD(39  ,  39   ,L2_Encapsulation_0               )" }
    ,{51  ,  51   ,GT_TRUE,      "LION3_KEY9_FIELD_NAME_BUILD(51  ,  51   ,Is_IPv6_EH_Hop_By_Hop            )" }
    ,{52  ,  52   ,GT_TRUE,      "LION3_KEY9_FIELD_NAME_BUILD(52  ,  52   ,Is_IPv6_EH_Exist                 )" }
    ,{53  ,  53   ,GT_TRUE,      "LION3_KEY9_FIELD_NAME_BUILD(53  ,  53   ,Is_ND                            )" }
    ,{54  ,  54   ,GT_TRUE,      "LION3_KEY9_FIELD_NAME_BUILD(54  ,  54   ,IP_Hdr_OK                        )" }
    ,{59  ,  59   ,GT_TRUE,      "LION3_KEY9_FIELD_NAME_BUILD(59  ,  59   ,L2_Encapsulation_1               )" }
    ,{60  ,  63   ,GT_TRUE,      "LION3_KEY9_FIELD_NAME_BUILD(60  ,  63   ,SRCePort_OR_Trunk_3_0            )" }
    ,{304 , 335   ,GT_TRUE,      "LION3_KEY9_FIELD_NAME_BUILD(304 , 335   ,SIP_63_32                        )" }
    ,{336 , 367   ,GT_TRUE,      "LION3_KEY9_FIELD_NAME_BUILD(336 , 367   ,SIP_95_64                        )" }
    ,{368 , 399   ,GT_TRUE,      "LION3_KEY9_FIELD_NAME_BUILD(368 , 399   ,SIP_127_96                       )" }
    ,{400 , 407   ,GT_TRUE,      "LION3_KEY9_FIELD_NAME_BUILD(400 , 407   ,SRCePort_OR_Trunk_11_4           )" }
    ,{408 , 408   ,GT_TRUE,      "LION3_KEY9_FIELD_NAME_BUILD(408 , 408   ,OrigSrcIsTrunk                   )" }
    ,{409 , 409   ,GT_TRUE,      "LION3_KEY9_FIELD_NAME_BUILD(409 , 409   ,eVLAN_12                         )" }
    ,{411 , 413   ,GT_TRUE,      "LION3_KEY9_FIELD_NAME_BUILD(411 , 413   ,UP1                              )" }
    ,{416 , 447   ,GT_TRUE,      "LION3_KEY9_FIELD_NAME_BUILD(416 , 447   ,DIP_63_32                        )" }
    ,{448 , 479   ,GT_TRUE,      "LION3_KEY9_FIELD_NAME_BUILD(448 , 479   ,DIP_95_64                        )" }
    ,{480 , 511   ,GT_TRUE,      "LION3_KEY9_FIELD_NAME_BUILD(480 , 511   ,DIP_127_96                       )" }
    ,{608 , 608   ,GT_TRUE,      "LION3_KEY9_FIELD_NAME_BUILD(608 , 608   ,Tag1_Exist                       )" }
    ,{609 , 620   ,GT_TRUE,      "LION3_KEY9_FIELD_NAME_BUILD(609 , 620   ,Tag1_VID                         )" }
    ,{621 , 621   ,GT_TRUE,      "LION3_KEY9_FIELD_NAME_BUILD(621 , 621   ,Tag1_CFI                         )" }
    /* key10 (without UDBs) */
    ,{30  ,  32   ,GT_TRUE,      "LION3_KEY10_FIELD_NAME_BUILD(30  ,  32   ,UP0_OR_EXP0                     )" }
    ,{39  ,  41   ,GT_TRUE,      "LION3_KEY10_FIELD_NAME_BUILD(39  ,  41   ,Pkt_type                        )" }
    ,{57  ,  62   ,GT_TRUE,      "LION3_KEY10_FIELD_NAME_BUILD(57  ,  62   ,IP_Packet_Length_5_0            )" }
    ,{63  ,  63   ,GT_TRUE,      "LION3_KEY10_FIELD_NAME_BUILD(63  ,  63   ,PacketTOS_DSCP_1                )" }
    ,{176 , 183   ,GT_TRUE,      "LION3_KEY10_FIELD_NAME_BUILD(176 , 183   ,IP_Packet_Length_13_6           )" }
    ,{184 , 191   ,GT_TRUE,      "LION3_KEY10_FIELD_NAME_BUILD(184 , 191   ,TTL                             )" }
    /* key11 (without UDBs) */
    ,{51  ,  54   ,GT_TRUE,      "LION3_KEY11_FIELD_NAME_BUILD(51  ,  54   ,IP_Packet_Length_3_0            )" }
    ,{58  ,  63   ,GT_TRUE,      "LION3_KEY11_FIELD_NAME_BUILD(58  ,  63   ,SRCePort_OR_Trunk_5_0           )" }
    ,{232 , 232   ,GT_TRUE,      "LION3_KEY11_FIELD_NAME_BUILD(232 , 232   ,PacketTOS_DSCP_1                )" }
    ,{233 , 233   ,GT_TRUE,      "LION3_KEY11_FIELD_NAME_BUILD(233 , 233   ,Is_ND                           )" }
    ,{234 , 253   ,GT_TRUE,      "LION3_KEY11_FIELD_NAME_BUILD(234 , 253   ,IPv6_Flow_Label                 )" }
    ,{254 , 254   ,GT_TRUE,      "LION3_KEY11_FIELD_NAME_BUILD(254 , 254   ,Is_IPv6_Link_local              )" }
    ,{255 , 274   ,GT_TRUE,      "LION3_KEY11_FIELD_NAME_BUILD(255 , 274   ,MPLS_Outer_Label                )" }
    ,{275 , 277   ,GT_TRUE,      "LION3_KEY11_FIELD_NAME_BUILD(275 , 277   ,MPLS_Outer_Label_EXP            )" }
    ,{278 , 278   ,GT_TRUE,      "LION3_KEY11_FIELD_NAME_BUILD(278 , 278   ,MPLS_Outer_Label_S_Bit          )" }
    ,{279 , 286   ,GT_TRUE,      "LION3_KEY11_FIELD_NAME_BUILD(279 , 286   ,IP_Packet_Length_11_4           )" }
    ,{287 , 294   ,GT_TRUE,      "LION3_KEY11_FIELD_NAME_BUILD(287 , 294   ,TTL                             )" }
    ,{295 , 326   ,GT_TRUE,      "LION3_KEY11_FIELD_NAME_BUILD(295 , 326   ,SIP_63_32                       )" }
    ,{327 , 358   ,GT_TRUE,      "LION3_KEY11_FIELD_NAME_BUILD(327 , 358   ,SIP_95_64                       )" }
    ,{359 , 390   ,GT_TRUE,      "LION3_KEY11_FIELD_NAME_BUILD(359 , 390   ,SIP_127_96                      )" }
    ,{391 , 398   ,GT_TRUE,      "LION3_KEY11_FIELD_NAME_BUILD(391 , 398   ,DIP_39_32                       )" }
    ,{399 , 405   ,GT_TRUE,      "LION3_KEY11_FIELD_NAME_BUILD(399 , 405   ,SRCePort_OR_Trunk_12_6          )" }
    ,{407 , 407   ,GT_TRUE,      "LION3_KEY11_FIELD_NAME_BUILD(407 , 407   ,OrigSrcIsTrunk                  )" }
    ,{408 , 408   ,GT_TRUE,      "LION3_KEY11_FIELD_NAME_BUILD(408 , 408   ,eVLAN_12                        )" }
    ,{409 , 409   ,GT_TRUE,      "LION3_KEY11_FIELD_NAME_BUILD(409 , 409   ,Tag1_Src_Tagged                 )" }
    ,{410 , 412   ,GT_TRUE,      "LION3_KEY11_FIELD_NAME_BUILD(410 , 412   ,UP1                             )" }
    ,{416 , 447   ,GT_TRUE,      "LION3_KEY11_FIELD_NAME_BUILD(416 , 447   ,DIP_63_40                       )" }
    ,{448 , 479   ,GT_TRUE,      "LION3_KEY11_FIELD_NAME_BUILD(448 , 479   ,DIP_95_64                       )" }
    ,{480 , 503   ,GT_TRUE,      "LION3_KEY11_FIELD_NAME_BUILD(480 , 503   ,DIP_127_96                      )" }
    ,{504 , 507   ,GT_TRUE,      "LION3_KEY11_FIELD_NAME_BUILD(504 , 507   ,IP_Packet_Length_15_12          )" }
    ,{509 , 509   ,GT_TRUE,      "LION3_KEY11_FIELD_NAME_BUILD(509 , 509   ,Is_IPv6_MLD                     )" }
    ,{510 , 510   ,GT_TRUE,      "LION3_KEY11_FIELD_NAME_BUILD(510 , 510   ,Is_IPv6_EH_Hop_By_Hop           )" }
    ,{511 , 511   ,GT_TRUE,      "LION3_KEY11_FIELD_NAME_BUILD(511 , 511   ,Is_IPv6_EH_Exist                )" }
    ,{512 , 512   ,GT_TRUE,      "LION3_KEY11_FIELD_NAME_BUILD(512 , 512   ,IP_Hdr_OK                       )" }
    /* key12 (without UDBs) */
    ,{50  ,  55   ,GT_TRUE,      "LION3_KEY12_FIELD_NAME_BUILD(50  ,  55   ,DSCP                            )" }
    ,{57  ,  72   ,GT_TRUE,      "LION3_KEY12_FIELD_NAME_BUILD(57  ,  72   ,DIP_15_0                        )" }
    ,{73  ,  73   ,GT_TRUE,      "LION3_KEY12_FIELD_NAME_BUILD(73  ,  73   ,IsARP                           )" }
    ,{74  ,  74   ,GT_TRUE,      "LION3_KEY12_FIELD_NAME_BUILD(74  ,  74   ,Ipv6_EH_exist                   )" }
    ,{75  ,  90   ,GT_TRUE,      "LION3_KEY12_FIELD_NAME_BUILD(75  ,  90   ,DIP_31_16                       )" }
    ,{92  , 123   ,GT_TRUE,      "LION3_KEY12_FIELD_NAME_BUILD(92  , 123   ,DIP_63_32                       )" }
    ,{124 , 155   ,GT_TRUE,      "LION3_KEY12_FIELD_NAME_BUILD(124 , 155   ,DIP_95_64                       )" }
    ,{156 , 187   ,GT_TRUE,      "LION3_KEY12_FIELD_NAME_BUILD(156 , 187   ,DIP_127_96                      )" }
    ,{189 , 189   ,GT_TRUE,      "LION3_KEY12_FIELD_NAME_BUILD(189 , 189   ,isL2Valid                       )" }
    ,{191 , 191   ,GT_TRUE,      "LION3_KEY12_FIELD_NAME_BUILD(191 , 191   ,IP_Header_OK                    )" }


    /* holds the info about the extra 10 bytes of the 60B UBD key fields - lion3 format */
    ,{400 , 400   ,GT_TRUE,      "LION3_KEY_60B_EXTRA_10B_FIELD_NAME_BUILD(400 , 400 , UDB_Valid     )" }
    ,{401 , 410   ,GT_TRUE,      "LION3_KEY_60B_EXTRA_10B_FIELD_NAME_BUILD(401 , 410 , PCL_ID        )" }
    ,{411 , 423   ,GT_TRUE,      "LION3_KEY_60B_EXTRA_10B_FIELD_NAME_BUILD(411 , 423 , eVLAN         )" }
    ,{424 , 436   ,GT_TRUE,      "LION3_KEY_60B_EXTRA_10B_FIELD_NAME_BUILD(424 , 436 , Source_ePort_OR_Trunk_id  )" }
    ,{437 , 437   ,GT_TRUE,      "LION3_KEY_60B_EXTRA_10B_FIELD_NAME_BUILD(437 , 437 , Src_Dev_Is_Own)" }
    ,{438 , 449   ,GT_TRUE,      "LION3_KEY_60B_EXTRA_10B_FIELD_NAME_BUILD(438 , 449 , Tag1_VID      )" }
    ,{450 , 452   ,GT_TRUE,      "LION3_KEY_60B_EXTRA_10B_FIELD_NAME_BUILD(450 , 452 , Tag1_UP       )" }
    ,{453 , 453   ,GT_TRUE,      "LION3_KEY_60B_EXTRA_10B_FIELD_NAME_BUILD(453 , 453 , Mac_To_Me     )" }
    ,{454 , 463   ,GT_TRUE,      "LION3_KEY_60B_EXTRA_10B_FIELD_NAME_BUILD(454 , 463 , QoS_Profile   )" }
    ,{464 , 475   ,GT_TRUE,      "LION3_KEY_60B_EXTRA_10B_FIELD_NAME_BUILD(464 , 475 , Flow_ID       )" }
    ,{476 , 479   ,GT_TRUE,      "LION3_KEY_60B_EXTRA_10B_FIELD_NAME_BUILD(476 , 479 , Reserved      )" }

};


/* udb index + start bit in relevant key*/
typedef struct{
    GT_U32  udbIndex;
    GT_U32  startBitInKey;
}UDB_INFO_STC;

/*Lion3 : udbs in key 0*/
static UDB_INFO_STC lion3Key0UdbArr[] =
{
    { 15,  64},
    { 16,  72},
    { 17, 112},

    { 23, 208},
    { 24, 216},
    { 25, 224},
    { 26, 232},
    {SMAIN_NOT_VALID_CNS , 106   }/*user defined valid*/
};

/*Lion3 : udbs in key 1*/
static UDB_INFO_STC lion3Key1UdbArr[] =
{
    { 18,  56},
    { 19, 112},

    { 27, 208},
    { 28, 216},
    { 29, 224},
    { 30, 232},
    {SMAIN_NOT_VALID_CNS , 106   }/*user defined valid*/
};

/*Lion3 : udbs in key 2*/
static UDB_INFO_STC lion3Key2UdbArr[] =
{
    { 20,  56},
    { 21, 176},
    { 22, 184},

    { 31, 208},
    { 32, 216},
    { 33, 224},
    { 34, 232},
    {SMAIN_NOT_VALID_CNS , 106   }/*user defined valid*/
};

/*Lion3 : udbs in key 3*/
static UDB_INFO_STC lion3Key3UdbArr[] =
{
    { 0 ,  48},
    { 1 ,  56},
    { 2 ,  64},
    { 3 ,  96},
    { 4 , 104},
    { 5 ,  72},
    { 6 , 112},
    { 7 , 120},
    { 8 , 128},
    { 9 , 136},
    { 10, 144},
    { 11, 152},
    { 12, 160},
    { 13, 168},
    { 14, 176},
    { 15, 184},

    { 35, 208},
    { 36, 216},
    { 37, 224},
    { 38, 232},
    {SMAIN_NOT_VALID_CNS , 94   }/*user defined valid*/
};

/*Lion3 : udbs in key 4*/
static UDB_INFO_STC lion3Key4UdbArr[] =
{
    { 0 , 392},
    { 1 , 360},
    { 2 , 368},
    { 3 , 376},
    { 4 , 384},
    { 5 ,  56},

    { 39, 416},
    { 40, 424},
    { 41, 432},
    { 42, 440},
    { 43, 448},
    { 44, 456},
    { 45, 464},
    { 46, 472},
    {SMAIN_NOT_VALID_CNS , 106   }/*user defined valid*/
};

/*Lion3 : udbs in key 5*/
static UDB_INFO_STC lion3Key5UdbArr[] =
{
    { 6 , 360},
    { 7 , 368},
    { 8 , 376},
    { 9 , 384},
    { 10, 392},
    { 11,  56},

    { 47, 416},
    { 48, 424},
    { 49, 432},
    { 0 , 440},
    { 1 , 448},
    { 2 , 456},
    { 3 , 464},
    { 4 , 472},
    {SMAIN_NOT_VALID_CNS , 106   }/*user defined valid*/
};

/*Lion3 : udbs in key 6*/
static UDB_INFO_STC lion3Key6UdbArr[] =
{
    { 12, 384},
    { 13, 392},
    { 14,  56},

    { 15, 416},
    { 16, 424},
    { 17, 432},
    { 18, 440},
    { 19, 448},
    { 20, 456},
    { 21, 464},
    { 22, 472},
    {SMAIN_NOT_VALID_CNS , 106   }/*user defined valid*/
};

/*Lion3 : udbs in key 7*/
static UDB_INFO_STC lion3Key7UdbArr[] =
{
    { 0 , 384},
    { 1 ,  56},
    { 2 ,  64},
    { 3 ,  96},
    { 4 , 104},
    { 5 ,  72},
    { 6 , 112},
    { 7 , 120},
    { 8 , 128},
    { 9 , 136},
    { 10, 336},
    { 11, 344},
    { 12, 352},
    { 13, 360},
    { 14, 368},
    { 15, 376},

    { 23, 416},
    { 24, 424},
    { 25, 432},
    { 26, 440},
    { 27, 448},
    { 28, 456},
    { 29, 464},
    { 30, 472},
    {SMAIN_NOT_VALID_CNS , 94   }/*user defined valid*/
};

/*Lion3 : udbs in key 8*/
static UDB_INFO_STC lion3Key8UdbArr[] =
{
    {  5, 406},
    {  6, 350},
    {  7, 358},
    {  8, 366},
    {  9, 374},
    { 10, 382},
    { 11, 390},
    { 12, 398},
    { 13, 104},
    { 14, 305},
    { 15, 313},
    { 16, 321},

    { 31, 416},
    { 32, 424},
    { 33, 432},
    { 34, 440},
    { 35, 448},
    { 36, 456},
    { 37, 464},
    { 38, 472},
    {SMAIN_NOT_VALID_CNS , 57   }/*user defined valid*/
};

/*Lion3 : udbs in key 9*/
static UDB_INFO_STC lion3Key9UdbArr[] =
{
    {  0, 512},
    {  1, 520},
    {  2, 528},
    {  3, 536},
    {  4, 544},
    {  5, 552},
    {  6, 560},
    {  7, 568},
    {  8, 576},
    {  9, 584},
    { 10, 592},
    { 11, 600},
    { 12, 104},

    { 39, 624},
    { 40, 632},
    {SMAIN_NOT_VALID_CNS , 57   }/*user defined valid*/
};

/*Lion3 : udbs in key 10*/
static UDB_INFO_STC lion3Key10UdbArr[] =
{
    {  5, 104},

    { 41, 208},
    { 42, 216},
    { 43, 224},
    { 44, 232},
    {SMAIN_NOT_VALID_CNS , 206   }/*user defined valid*/
};

/*Lion3 : udbs in key 11*/
static UDB_INFO_STC lion3Key11UdbArr[] =
{
    {  0, 513},
    {  1, 521},
    {  2, 529},
    {  3, 537},
    {  4, 545},
    {  5, 553},
    {  6, 561},
    {  7, 569},
    {  8, 577},
    {  9, 585},
    { 10, 593},
    { 11, 601},
    { 17, 224},
    { 18, 104},
    { 19, 176},
    { 20, 184},
    { 21, 208},
    { 22, 216},

    { 45, 624},
    { 46, 632},
    {SMAIN_NOT_VALID_CNS , 57   }/*user defined valid*/
};

/*Lion3 : udbs in key 12*/
static UDB_INFO_STC lion3Key12UdbArr[] =
{
    { 47, 208},
    { 48, 216},
    { 49, 224},
    {  0, 232},
    {SMAIN_NOT_VALID_CNS , 206   }/*user defined valid*/
};

/* Lion3 : array to hold the UDBs info of all keys*/
static UDB_INFO_STC*    lion3KeysUdbArr[] =
{
    lion3Key0UdbArr,
    lion3Key1UdbArr,
    lion3Key2UdbArr,
    lion3Key3UdbArr,
    lion3Key4UdbArr,
    lion3Key5UdbArr,
    lion3Key6UdbArr,
    lion3Key7UdbArr,
    lion3Key8UdbArr,
    lion3Key9UdbArr,
    lion3Key10UdbArr,
    lion3Key11UdbArr,
    lion3Key12UdbArr
};

#define SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_NAME                                \
     STR(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_COMMAND                                 )\
    ,STR(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_CPU_CODE                                )\
    ,STR(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_MIRROR_TO_ANALYZER_PORT_0               )\
    ,STR(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_REDIRECT_COMMAND                        )\
    ,STR(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_RESERVED_46_15                          )\
    ,STR(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_POLICY1_LOOKUP_MODE                     )\
    ,STR(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_POLICY2_LOOKUP_MODE                     )\
    ,STR(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_IPCL_PROFILE_INDEX                      )\
    ,STR(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_RESERVED_17_15                          )\
    ,STR(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_VIDX                                    )\
    ,STR(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_IS_TRUNK                                )\
    ,STR(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_TRG_PORT                                )\
    ,STR(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_TARGET_DEVICE                           )\
    ,STR(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_TRUNK_ID                                )\
    ,STR(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_USE_VIDX                                )\
    ,STR(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_VNT_L2_ECHO                             )\
    ,STR(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_TUNNEL_START                            )\
    ,STR(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_ARP_POINTER                             )\
    ,STR(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_TUNNEL_POINTER                          )\
    ,STR(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_TUNNEL_START_PASSENGER_TYPE             )\
    ,STR(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_IP_NEXT_HOP_ENTRY_INDEX                 )\
    ,STR(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_VRF_ID                                  )\
    ,STR(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_MAC_SA_29_0                             )\
    ,STR(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_BIND_TO_CNC_COUNTER                     )\
    ,STR(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_CNC_COUNTER_INDEX                       )\
    ,STR(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_MAC_SA_47_30                            )\
    ,STR(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_ACTIVATE_METER                          )\
    ,STR(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_BIND_TO_POLICER_COUNTER                 )\
    ,STR(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_POLICER_PTR                             )\
    ,STR(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_SOURCE_ID_SET_ENABLE                    )\
    ,STR(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_SOURCE_ID                               )\
    ,STR(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_ACTION_STOP                             )\
    ,STR(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_BRIDGE_BYPASS                           )\
    ,STR(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_BYPASS_INGRESS_PIPE                     )\
    ,STR(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_VLAN_PRECEDENCE                         )\
    ,STR(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_EN_NESTED_VLAN                          )\
    ,STR(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_VLAN_COMMAND                            )\
    ,STR(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_VID0                                    )\
    ,STR(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_QOS_PROFILE_MARKING_ENABLE              )\
    ,STR(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_QOS_PRECEDENCE                          )\
    ,STR(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_QOS_PROFILE                             )\
    ,STR(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_MODIFY_DSCP                             )\
    ,STR(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_MODIFY_UP                               )\
    ,STR(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_ENABLE_MIRROR_TCP_RST_OR_FIN            )\
    ,STR(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_MIRROR_TO_ANALYZER_PORT_2_1             )\
    ,STR(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_MODIFY_MAC_DA                           )\
    ,STR(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_MODIFY_MAC_SA                           )\
    ,STR(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_SET_EGRESS_FILTER_REGISTERED            )\
    ,STR(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_VLAN1_CMD                               )\
    ,STR(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_UP1_CMD                                 )\
    ,STR(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_VID1                                    )\
    ,STR(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_UP1                                     )\
    ,STR(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_FLOW_ID                                 )\
    ,STR(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_IPCL_RESERVED_EN                        )\
    ,STR(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_IPCL_TO_TXQ_RESERVED                    )\
    ,STR(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_SET_MAC2ME                              )\
    ,STR(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_TIMESTAMP_EN                            )\
    ,STR(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_OFFSET_INDEX                            )\
    ,STR(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_OAM_PROCESSING_EN                       )\
    ,STR(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_OAM_PROFILE                             )\
    ,STR(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_ASSIGN_SRC_EPORT_EN                     )\
    ,STR(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_SRC_EPORT                               )\
    ,STR(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_USER_AC_ENABLE                          )

static char * lion3IpclActionFieldsTableNames[
    SMEM_LION3_IPCL_ACTION_TABLE_FIELDS___LAST_VALUE___E] =
    {SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_NAME};

static SNET_ENTRY_FORMAT_TABLE_STC lion3IpclActionTableFieldsFormat[
    SMEM_LION3_IPCL_ACTION_TABLE_FIELDS___LAST_VALUE___E] =
{
/*SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_COMMAND                               */
    STANDARD_FIELD_MAC(3),
/*SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_CPU_CODE                              */
    STANDARD_FIELD_MAC(8),
/*SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_MIRROR_TO_ANALYZER_PORT_0             */
    STANDARD_FIELD_MAC(1),
/*SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_REDIRECT_COMMAND                      */
    STANDARD_FIELD_MAC(3),

    /*If <Redirect Command> = No Redirect (0)*/
    /*SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_RESERVED_46_15                        */
        {FIELD_SET_IN_RUNTIME_CNS,
         32,
         SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_REDIRECT_COMMAND},
    /*SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_POLICY1_LOOKUP_MODE                   */
        STANDARD_FIELD_MAC(1),
    /*SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_POLICY2_LOOKUP_MODE                   */
        STANDARD_FIELD_MAC(1),
    /*SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_IPCL_PROFILE_INDEX                    */
        STANDARD_FIELD_MAC(13),

    /* If <Redirect Command> = 1 (Redirect to egress interface)*/
    /*SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_RESERVED_17_15                        */
        {FIELD_SET_IN_RUNTIME_CNS,
         3,
         SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_REDIRECT_COMMAND},

         /*<UseVIDX> = 1*/
        /*SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_VIDX                                  */
        {FIELD_SET_IN_RUNTIME_CNS,
         16,
         SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_RESERVED_17_15},

         /*if <UseVIDX> = 0*/
        /*SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_IS_TRUNK                              */
        {FIELD_SET_IN_RUNTIME_CNS,
         1,
         SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_RESERVED_17_15},

            /*if <UseVIDX> = 0 & <IsTrunk> = 0*/
            /*SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_TRG_PORT                              */
            {FIELD_SET_IN_RUNTIME_CNS,
             13,
             SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_IS_TRUNK},

            /*SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_TARGET_DEVICE                         */
            STANDARD_FIELD_MAC(10),

            /*if <UseVIDX> = 0 & <IsTrunk> = 1*/
            /*SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_TRUNK_ID                              */
            {FIELD_SET_IN_RUNTIME_CNS,
             12,
             SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_IS_TRUNK},


        /*SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_USE_VIDX                              */
            {FIELD_SET_IN_RUNTIME_CNS,
             1,
             SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_TARGET_DEVICE},

        /*SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_VNT_L2_ECHO                           */
            STANDARD_FIELD_MAC(1),
        /*SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_TUNNEL_START                          */
            STANDARD_FIELD_MAC(1),

            /*if <Tunnel Start> == 0*/
            /*SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_ARP_POINTER                           */
                {FIELD_SET_IN_RUNTIME_CNS,
                 17,
                 SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_TUNNEL_START},

            /*if <Tunnel Start> == 1*/
            /*SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_TUNNEL_POINTER                        */
                {FIELD_SET_IN_RUNTIME_CNS,
                 15,
                 SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_TUNNEL_START},
            /*SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_TUNNEL_START_PASSENGER_TYPE           */
                STANDARD_FIELD_MAC(1),

    /*If <Redirect Command> = 2 (Redirect to NHE)*/
    /*SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_IP_NEXT_HOP_ENTRY_INDEX               */
        {FIELD_SET_IN_RUNTIME_CNS,
         16,
         SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_REDIRECT_COMMAND},

    /*if <Redirect Command> = 4 (Assign VRF_ID)*/
    /*SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_VRF_ID                                */
        {FIELD_SET_IN_RUNTIME_CNS,
         12,
         SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_REDIRECT_COMMAND},


    /* if (<Redirect Command> = 6 - Replace MAC SA)*/
    /*SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_MAC_SA_29_0                           */
        {FIELD_SET_IN_RUNTIME_CNS,
         30,
         SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_REDIRECT_COMMAND},

/*SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_BIND_TO_CNC_COUNTER                   */
    {FIELD_SET_IN_RUNTIME_CNS,
     1,
     SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_ARP_POINTER},
/*SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_CNC_COUNTER_INDEX                      */
    STANDARD_FIELD_MAC(14),

    /* if (<Redirect Command> = 6 - Replace MAC SA)*/
    /*SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_MAC_SA_47_30                          */
        {FIELD_SET_IN_RUNTIME_CNS,
         18,
         SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_CNC_COUNTER_INDEX},


     /*if (<Redirect Command> != 6 - Replace MAC SA)*/
    /*SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_ACTIVATE_METER                        */
        {FIELD_SET_IN_RUNTIME_CNS,
         1,
         SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_CNC_COUNTER_INDEX},
    /*SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_BIND_TO_POLICER_COUNTER               */
        STANDARD_FIELD_MAC(1),
    /*SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_POLICER_PTR                           */
        STANDARD_FIELD_MAC(16),

/*SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_SOURCE_ID_SET_ENABLE                  */
    {FIELD_SET_IN_RUNTIME_CNS,
     1,
     SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_MAC_SA_47_30},
/*SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_SOURCE_ID                             */
    STANDARD_FIELD_MAC(12),
/*SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_ACTION_STOP                           */
    STANDARD_FIELD_MAC(1),
/*SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_BRIDGE_BYPASS                         */
    STANDARD_FIELD_MAC(1),
/*SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_BYPASS_INGRESS_PIPE                   */
    STANDARD_FIELD_MAC(1),
/*SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_VLAN_PRECEDENCE                       */
    STANDARD_FIELD_MAC(1),
/*SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_EN_NESTED_VLAN                        */
    STANDARD_FIELD_MAC(1),
/*SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_VLAN_COMMAND                          */
    STANDARD_FIELD_MAC(2),
/*SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_VID0                                  */
    STANDARD_FIELD_MAC(13),
/*SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_QOS_PROFILE_MARKING_ENABLE            */
    STANDARD_FIELD_MAC(1),
/*SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_QOS_PRECEDENCE                        */
    STANDARD_FIELD_MAC(1),
/*SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_QOS_PROFILE                           */
    STANDARD_FIELD_MAC(10),
/*SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_MODIFY_DSCP                           */
    STANDARD_FIELD_MAC(2),
/*SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_MODIFY_UP                             */
    STANDARD_FIELD_MAC(2),
/*SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_ENABLE_MIRROR_TCP_RST_OR_FIN          */
    STANDARD_FIELD_MAC(1),
/*SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_MIRROR_TO_ANALYZER_PORT_2_1           */
    STANDARD_FIELD_MAC(2),
/*SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_MODIFY_MAC_DA                         */
    STANDARD_FIELD_MAC(1),

    /*If <Redirect Command> = 1 (Redirect to egress interface)*/
    /*SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_MODIFY_MAC_SA                         */
        {FIELD_SET_IN_RUNTIME_CNS,
         1,
         SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_MODIFY_MAC_DA},

    /* If <Redirect Command> != 1 (Redirect to egress interface)*/
    /*SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_SET_EGRESS_FILTER_REGISTERED          */
        {FIELD_SET_IN_RUNTIME_CNS,
         1,
         SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_MODIFY_MAC_DA},

/*SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_VLAN1_CMD                             */
    {FIELD_SET_IN_RUNTIME_CNS,
     2,
     SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_SET_EGRESS_FILTER_REGISTERED},

/*SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_UP1_CMD                               */
    STANDARD_FIELD_MAC(2),
/*SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_VID1                                  */
    STANDARD_FIELD_MAC(12),
/*SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_UP1                                   */
    STANDARD_FIELD_MAC(3),
/*SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_FLOW_ID                               */
    STANDARD_FIELD_MAC(12),
/*SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_IPCL_RESERVED_EN                      */
    STANDARD_FIELD_MAC(1),
/*SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_IPCL_TO_TXQ_RESERVED                  */
    STANDARD_FIELD_MAC(20),
/*SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_SET_MAC2ME                            */
    STANDARD_FIELD_MAC(1),
/*SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_TIMESTAMP_EN                          */
    STANDARD_FIELD_MAC(1),
/*SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_OFFSET_INDEX                          */
    STANDARD_FIELD_MAC(7),
/*SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_OAM_PROCESSING_EN                     */
    STANDARD_FIELD_MAC(1),
/*SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_OAM_PROFILE                           */
    STANDARD_FIELD_MAC(1),
/*SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_ASSIGN_SRC_EPORT_EN                   */
    STANDARD_FIELD_MAC(1),
/*SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_SRC_EPORT                             */
    STANDARD_FIELD_MAC(13),
/*SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_USER_AC_ENABLE                        */
    STANDARD_FIELD_MAC(1)
};
#define SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_NAME                             \
      STR(SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_CPU_CODE                      )\
     ,STR(SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_COMMAND                       )\
     ,STR(SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_REDIRECT_COMMAND              )\
     ,STR(SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_ACTION_STOP                   )\
     ,STR(SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_SET_MAC2ME                    )\
     ,STR(SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_POLICY1_LOOKUP_MODE           )\
     ,STR(SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_POLICY2_LOOKUP_MODE           )\
     ,STR(SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_IPCL_PROFILE_INDEX            )\
     ,STR(SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_USE_VIDX                      )\
     ,STR(SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_VIDX                          )\
     ,STR(SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_IS_TRUNK                      )\
     ,STR(SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_TRG_PORT                      )\
     ,STR(SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_TARGET_DEVICE                 )\
     ,STR(SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_TRUNK_ID                      )\
     ,STR(SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_VNT_L2_ECHO                   )\
     ,STR(SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_TUNNEL_START                  )\
     ,STR(SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_ARP_POINTER                   )\
     ,STR(SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_TUNNEL_POINTER                )\
     ,STR(SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_TUNNEL_START_PASSENGER_TYPE   )\
     ,STR(SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_PBR_MODE                      )\
     ,STR(SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_IP_NEXT_HOP_ENTRY_INDEX       )\
     ,STR(SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_VRF_ID                        )\
     ,STR(SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_MAC_SA_29_0                   )\
     ,STR(SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_MAC_SA_47_30                  )\
     ,STR(SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_ACTIVATE_METER                )\
     ,STR(SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_BIND_TO_POLICER_COUNTER       )\
     ,STR(SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_POLICER_PTR                   )\
     ,STR(SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_RESERVED                      )\
     ,STR(SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_ENABLE_MIRROR_TCP_RST_OR_FIN  )\
     ,STR(SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_MIRROR_TO_ANALYZER_PORT       )\
     ,STR(SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_MODIFY_MAC_DA                 )\
     ,STR(SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_MODIFY_MAC_SA                 )\
     ,STR(SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_SET_EGRESS_FILTER_REGISTERED  )\
     ,STR(SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_BRIDGE_BYPASS                 )\
     ,STR(SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_BYPASS_INGRESS_PIPE           )\
     ,STR(SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_EN_NESTED_VLAN                )\
     ,STR(SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_SET_SST_ID                    )\
     ,STR(SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_SST_ID                        )\
     ,STR(SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_USER_AC_ENABLE                )\
     ,STR(SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_ACTIVATE_COUNTER              )\
     ,STR(SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_COUNTER_INDEX                 )\
     ,STR(SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_VLAN_PRECEDENCE               )\
     ,STR(SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_VLAN_COMMAND                  )\
     ,STR(SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_VID0                          )\
     ,STR(SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_QOS_PROFILE_MARKING_ENABLE    )\
     ,STR(SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_QOS_PRECEDENCE                )\
     ,STR(SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_QOS_PROFILE                   )\
     ,STR(SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_MODIFY_DSCP                   )\
     ,STR(SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_MODIFY_UP                     )\
     ,STR(SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_VLAN1_CMD                     )\
     ,STR(SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_UP1_CMD                       )\
     ,STR(SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_VID1                          )\
     ,STR(SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_UP1                           )\
     ,STR(SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_IPCL_RESERVED_EN              )\
     ,STR(SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_IPCL_TO_TXQ_RESERVED          )\
     ,STR(SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_TIMESTAMP_EN                  )\
     ,STR(SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_OFFSET_INDEX                  )\
     ,STR(SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_OAM_PROCESSING_EN             )\
     ,STR(SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_OAM_PROFILE                   )\
     ,STR(SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_FLOW_ID                       )\
     ,STR(SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_RESERVED_1                    )\
     ,STR(SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_ASSIGN_SRC_EPORT_EN           )\
     ,STR(SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_SRC_EPORT                     )

static char * sip5_20IpclActionFieldsTableNames[
    SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS___LAST_VALUE___E] =
    {SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_NAME};

static SNET_ENTRY_FORMAT_TABLE_STC sip5_20IpclActionTableFieldsFormat[
    SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS___LAST_VALUE___E] =
{
/*SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_CPU_CODE*/
STANDARD_FIELD_MAC(8),
/*SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_COMMAND*/
STANDARD_FIELD_MAC(3),
/*SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_REDIRECT_COMMAND*/
STANDARD_FIELD_MAC(3),
/*SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_ACTION_STOP*/
STANDARD_FIELD_MAC(1),
/*SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_SET_MAC2ME*/
STANDARD_FIELD_MAC(1),
/*==================*/
/*If <Redirect Command> = No Redirect (0)*/
    /*SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_POLICY1_LOOKUP_MODE*/
    {FIELD_SET_IN_RUNTIME_CNS,
     1,
     SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_SET_MAC2ME},
    /*SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_POLICY2_LOOKUP_MODE*/
            STANDARD_FIELD_MAC(1),
    /*SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_IPCL_PROFILE_INDEX*/
            STANDARD_FIELD_MAC(13),
/*==================*/
/* If <Redirect Command> = 1 (Redirect to egress interface)*/
    /*SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_USE_VIDX*/
    {FIELD_SET_IN_RUNTIME_CNS,
     1,
     SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_SET_MAC2ME},
     /*<UseVIDX> = 1*/
    /*SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_VIDX*/
    STANDARD_FIELD_MAC(16),
     /*if <UseVIDX> = 0*/
    /*SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_IS_TRUNK*/
        {FIELD_SET_IN_RUNTIME_CNS,
         1,
         SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_USE_VIDX},
        /*if <UseVIDX> = 0 & <IsTrunk> = 0*/
        /*SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_TRG_PORT*/
            {FIELD_SET_IN_RUNTIME_CNS,
             14,
             SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_IS_TRUNK},
        /*SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_TARGET_DEVICE*/
        STANDARD_FIELD_MAC(10),
        /*if <UseVIDX> = 0 & <IsTrunk> = 1*/
        /*SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_TRUNK_ID*/
        {FIELD_SET_IN_RUNTIME_CNS,
         14,
         SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_IS_TRUNK},
    /*SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_VNT_L2_ECHO*/
    {FIELD_SET_IN_RUNTIME_CNS,
     1,
     SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_TARGET_DEVICE},
    /*SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_TUNNEL_START*/
    STANDARD_FIELD_MAC(1),
        /*if <Tunnel Start> == 0*/
        /*EM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_ARP_POINTER*/
        {FIELD_SET_IN_RUNTIME_CNS,
         18,
         SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_TUNNEL_START},
        /*if <Tunnel Start> == 1*/
        /*EM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_TUNNEL_POINTER*/
        {FIELD_SET_IN_RUNTIME_CNS,
         16,
         SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_TUNNEL_START},
        /*EM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_TUNNEL_START_PASSENGER_TYPE*/
        STANDARD_FIELD_MAC(1),
/*==================*/
/*If <Redirect Command> = 2 (Redirect to NHE)*/
    /*EM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_PBR_MODE*/
    EXPLICIT_FIELD_MAC(32,1),
    /*EM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_IP_NEXT_HOP_ENTRY_INDEX*/
    STANDARD_FIELD_MAC(18),
/*==================*/
/*else if <Redirect Command> = 4 (Assign VRF_ID)*/
    /*EM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_VRF_ID*/
    EXPLICIT_FIELD_MAC(52,12),
/*==================*/
/*else if (<Redirect Command> = 6 - Replace MAC SA)*/
/* 43 16 Mac Sa[27:0]
Mac_Sa
 81 62 Mac Sa [47:28]
Mac_Sa_47_28*/
    /*SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_MAC_SA_29_0*/
    {FIELD_SET_IN_RUNTIME_CNS,
     28,
     SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_SET_MAC2ME},
    /*SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_MAC_SA_47_30*/
    EXPLICIT_FIELD_MAC(62,18),
/*==================*/

/*else if (<Redirect Command> = !6 - Replace MAC SA)*/
    /*SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_ACTIVATE_METER*/
    EXPLICIT_FIELD_MAC(64,1),
    /*SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_BIND_TO_POLICER_COUNTER*/
    STANDARD_FIELD_MAC(1),
    /*SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_POLICER_PTR*/
    STANDARD_FIELD_MAC(14),
    /*SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_RESERVED*/
    STANDARD_FIELD_MAC(2),
/*==================*/
/*SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_ENABLE_MIRROR_TCP_RST_OR_FIN*/
STANDARD_FIELD_MAC(1),
/*SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_MIRROR_TO_ANALYZER_PORT*/
STANDARD_FIELD_MAC(3),
/*SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_MODIFY_MAC_DA*/
STANDARD_FIELD_MAC(1),
    /*If <Redirect Command> = 1 (Redirect to egress interface)*/
    /*SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_MODIFY_MAC_SA*/
    {FIELD_SET_IN_RUNTIME_CNS,
     1,
     SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_MODIFY_MAC_DA},
    /* If <Redirect Command> != 1 (Redirect to egress interface)*/
    /*SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_SET_EGRESS_FILTER_REGISTERED*/
    {FIELD_SET_IN_RUNTIME_CNS,
     1,
     SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_MODIFY_MAC_DA},
/*SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_BRIDGE_BYPASS*/
STANDARD_FIELD_MAC(1),
/*SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_BYPASS_INGRESS_PIPE*/
STANDARD_FIELD_MAC(1),
/*SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_EN_NESTED_VLAN*/
STANDARD_FIELD_MAC(1),
/*SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_SET_SST_ID*/
STANDARD_FIELD_MAC(1),
/*SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_SST_ID*/
STANDARD_FIELD_MAC(12),
/*SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_USER_AC_ENABLE*/
STANDARD_FIELD_MAC(1),
/*SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_ACTIVATE_COUNTER*/
STANDARD_FIELD_MAC(1),
/*SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_COUNTER_INDEX*/
STANDARD_FIELD_MAC(14),
/*SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_VLAN_PRECEDENCE*/
STANDARD_FIELD_MAC(1),
/*SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_VLAN_COMMAND*/
STANDARD_FIELD_MAC(2),
/*SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_VID0*/
STANDARD_FIELD_MAC(13),
/*SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_QOS_PROFILE_MARKING_ENABLE*/
STANDARD_FIELD_MAC(1),
/*SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_QOS_PRECEDENCE*/
STANDARD_FIELD_MAC(1),
/*SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_QOS_PROFILE*/
STANDARD_FIELD_MAC(10),
/*SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_MODIFY_DSCP*/
STANDARD_FIELD_MAC(2),
/*SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_MODIFY_UP*/
STANDARD_FIELD_MAC(2),
/*SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_VLAN1_CMD*/
STANDARD_FIELD_MAC(2),
/*SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_UP1_CMD*/
STANDARD_FIELD_MAC(2),
/*SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_VID1*/
STANDARD_FIELD_MAC(12),
/*SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_UP1*/
STANDARD_FIELD_MAC(3),
/*SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_IPCL_RESERVED_EN*/
STANDARD_FIELD_MAC(1),
/*SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_IPCL_TO_TXQ_RESERVED*/
STANDARD_FIELD_MAC(20),
/*SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_TIMESTAMP_EN*/
STANDARD_FIELD_MAC(1),
/*SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_OFFSET_INDEX*/
STANDARD_FIELD_MAC(7),
/*SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_OAM_PROCESSING_EN*/
STANDARD_FIELD_MAC(1),
/*SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_OAM_PROFILE*/
STANDARD_FIELD_MAC(1),
/*SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_FLOW_ID*/
STANDARD_FIELD_MAC(13),
/*SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_RESERVED_1*/
STANDARD_FIELD_MAC(1),
/*SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_ASSIGN_SRC_EPORT_EN*/
STANDARD_FIELD_MAC(1),
/*SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_SRC_EPORT*/
STANDARD_FIELD_MAC(14)
};


#define SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_NAME                                \
     STR(SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_COMMAND                           )\
    ,STR(SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_DSCP_EXP                          )\
    ,STR(SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_UP0                               )\
    ,STR(SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_MODIFY_DSCP_EXP                   )\
    ,STR(SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_ENABLE_MODIFY_UP0                 )\
    ,STR(SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_BIND_TO_CNC_COUNTER               )\
    ,STR(SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_CNC_COUNTER_INDEX                 )\
    ,STR(SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_TAG1_VID_CMD                      )\
    ,STR(SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_ENABLE_MODIFY_UP1                 )\
    ,STR(SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_TAG1_VID                          )\
    ,STR(SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_UP1                               )\
    ,STR(SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_TAG0_VLAN_CMD                     )\
    ,STR(SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_TAG0_VID                          )\
    ,STR(SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_BIND_TO_POLICER_COUNTER           )\
    ,STR(SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_BIND_TO_POLICER_METER             )\
    ,STR(SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_POLICER_INDEX                     )\
    ,STR(SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_FLOW_ID                           )\
    ,STR(SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_OAM_PROCESSING_ENABLE             )\
    ,STR(SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_OAM_PROFILE                       )\
    ,STR(SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_TIME_STAMP_ENABLE                 )\
    ,STR(SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_TIME_STAMP_OFFSET_INDEX           )\
    ,STR(SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_CHANNEL_TYPE_TO_OPCODE_MAPPING_EN )\
    ,STR(SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_EPCL_RESERVED_ENABLE              )\
    ,STR(SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_EPCL_RESERVED                     )\
    ,STR(SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_TM_QUEUE_ID                       )

static char * lion3EpclActionFieldsTableNames[
    SMEM_LION3_EPCL_ACTION_TABLE_FIELDS___LAST_VALUE___E] =
    {SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_NAME};

static SNET_ENTRY_FORMAT_TABLE_STC lion3EpclActionTableFieldsFormat[
    SMEM_LION3_EPCL_ACTION_TABLE_FIELDS___LAST_VALUE___E] =
{
/*SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_COMMAND                             */
     STANDARD_FIELD_MAC(3 )
/*SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_DSCP_EXP                            */
    ,STANDARD_FIELD_MAC(6 )
/*SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_UP0                                 */
    ,STANDARD_FIELD_MAC(3 )
/*SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_MODIFY_DSCP_EXP                     */
    ,STANDARD_FIELD_MAC(2 )
/*SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_ENABLE_MODIFY_UP0                   */
    ,STANDARD_FIELD_MAC(2 )
/*SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_BIND_TO_CNC_COUNTER                 */
    ,STANDARD_FIELD_MAC(1 )
/*SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_CNC_COUNTER_INDEX                   */
    ,STANDARD_FIELD_MAC(14)
/*SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_TAG1_VID_CMD                        */
    ,STANDARD_FIELD_MAC(1 )
/*SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_ENABLE_MODIFY_UP1                   */
    ,STANDARD_FIELD_MAC(1 )
/*SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_TAG1_VID                            */
    ,STANDARD_FIELD_MAC(12)
/*SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_UP1                                 */
    ,STANDARD_FIELD_MAC(3 )
/*SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_TAG0_VLAN_CMD                       */
    ,STANDARD_FIELD_MAC(2 )
/*SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_TAG0_VID                            */
    ,STANDARD_FIELD_MAC(12)
/*SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_BIND_TO_POLICER_COUNTER             */
    ,STANDARD_FIELD_MAC(1 )
/*SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_BIND_TO_POLICER_METER               */
    ,STANDARD_FIELD_MAC(1 )
/*SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_POLICER_INDEX                       */
    ,STANDARD_FIELD_MAC(16)
/*SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_FLOW_ID                             */
    ,STANDARD_FIELD_MAC(12)
/*SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_OAM_PROCESSING_ENABLE               */
    ,STANDARD_FIELD_MAC(1 )
/*SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_OAM_PROFILE                         */
    ,STANDARD_FIELD_MAC(1 )
/*SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_TIME_STAMP_ENABLE                   */
    ,STANDARD_FIELD_MAC(1 )
/*SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_TIME_STAMP_OFFSET_INDEX             */
    ,STANDARD_FIELD_MAC(7 )
/*SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_CHANNEL_TYPE_TO_OPCODE_MAPPING_EN   */
    ,STANDARD_FIELD_MAC(1 )
/*SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_EPCL_RESERVED_ENABLE                */
    ,STANDARD_FIELD_MAC(1 )
/*SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_EPCL_RESERVED                       */
    ,STANDARD_FIELD_MAC(20)
/*SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_TM_QUEUE_ID                         */
    ,STANDARD_FIELD_MAC(14)
};

static SNET_ENTRY_FORMAT_TABLE_STC sip5_20EpclActionTableFieldsFormat[
    SMEM_LION3_EPCL_ACTION_TABLE_FIELDS___LAST_VALUE___E] =
{
/*SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_COMMAND                             */
     STANDARD_FIELD_MAC(3 )
/*SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_DSCP_EXP                            */
    ,STANDARD_FIELD_MAC(6 )
/*SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_UP0                                 */
    ,STANDARD_FIELD_MAC(3 )
/*SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_MODIFY_DSCP_EXP                     */
    ,STANDARD_FIELD_MAC(2 )
/*SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_ENABLE_MODIFY_UP0                   */
    ,STANDARD_FIELD_MAC(2 )
/*SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_BIND_TO_CNC_COUNTER                 */
    ,STANDARD_FIELD_MAC(1 )
/*SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_CNC_COUNTER_INDEX                   */
    ,STANDARD_FIELD_MAC(14)
/*SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_TAG1_VID_CMD                        */
    ,STANDARD_FIELD_MAC(1 )
/*SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_ENABLE_MODIFY_UP1                   */
    ,STANDARD_FIELD_MAC(1 )
/*SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_TAG1_VID                            */
    ,STANDARD_FIELD_MAC(12)
/*SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_UP1                                 */
    ,STANDARD_FIELD_MAC(3 )
/*SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_TAG0_VLAN_CMD                       */
    ,STANDARD_FIELD_MAC(2 )
/*SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_TAG0_VID                            */
    ,STANDARD_FIELD_MAC(12)
/*SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_BIND_TO_POLICER_COUNTER             */
    ,STANDARD_FIELD_MAC(1 )
/*SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_BIND_TO_POLICER_METER               */
    ,STANDARD_FIELD_MAC(1 )
/*SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_POLICER_INDEX                       */
    ,STANDARD_FIELD_MAC(14)/*was 16*/
/*SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_FLOW_ID                             */
    ,STANDARD_FIELD_MAC(13)/*increase by 1*/
/*SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_OAM_PROCESSING_ENABLE               */
    ,STANDARD_FIELD_MAC(1 )
/*SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_OAM_PROFILE                         */
    ,STANDARD_FIELD_MAC(1 )
/*SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_TIME_STAMP_ENABLE                   */
    ,STANDARD_FIELD_MAC(1 )
/*SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_TIME_STAMP_OFFSET_INDEX             */
    ,STANDARD_FIELD_MAC(7 )
/*SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_CHANNEL_TYPE_TO_OPCODE_MAPPING_EN   */
    ,STANDARD_FIELD_MAC(1 )
/*SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_EPCL_RESERVED_ENABLE                */
    ,STANDARD_FIELD_MAC(1 )
/*SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_EPCL_RESERVED                       */
    ,STANDARD_FIELD_MAC(20)
/*SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_TM_QUEUE_ID                         */
    ,STANDARD_FIELD_MAC(0)/*field removed*/
};


#define SMEM_LION3_IPCL_META_DATA_FIELDS_NAME                                    \
 STR(SMEM_LION3_IPCL_META_DATA_FIELDS_PORT_LIST_SRC_E                           )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_LOCAL_DEV_SRC_TRUNK_ID_E                  )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_LOCAL_DEV_SRC_EPORT_E                     )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_SRC_DEV_IS_OWN_E                          )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_RESERVED_1_E                              )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_ORIG_SRC_DEV_E                            )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_ORIG_SRC_IS_TRUNK_E                       )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_ORIG_SRC_EPORT_OR_TRUNK_ID_E              )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_LOCAL_DEV_SRC_PORT_E                      )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_USE_VIDX_E                                )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_EVIDX_E                                   )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_INGRESS_UDB_PACKET_TYPE_E                 )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_IP_LEGAL_OR_FCOE_LEGAL_DUP_E              )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_L2_VALID_DUP_E                            )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_IS_ARP_DUP_E                              )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_TRG_DEV_E                                 )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_TRG_IS_TRUNK_E                            )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_TRG_EPORT_OR_TRG_TRUNK_ID_E               )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_TRG_PHY_PORT_E                            )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_IS_TRG_PHY_PORT_VALID_E                   )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_EGRESS_UDB_PACKET_TYPE_E                  )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_RESERVED_2_E                              )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_SRC_ID_E                                  )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_EGRESS_FILTER_REGISTERED_E                )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_PACKET_IS_LOOPED_E                        )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_DROP_ON_SOURCE_E                          )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_RESERVED_3_E                              )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_PHY_SRC_MC_FILTER_EN_E                    )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_ORIG_SRC_PHY_IS_TRUNK_E                   )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_ORIG_SRC_PHY_PORT_OR_TRUNK_ID_E           )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_RESERVED_4_E                              )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_L2_VALID_E                                )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_L2_ENCAPSULATION_E                        )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_BYPASS_BRIDGE_E                           )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_MAC_DA_TYPE_E                             )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_OUTER_IS_LLC_NON_SNAP_E                   )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_INNER_IS_LLC_NON_SNAP_E                   )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_ETHERTYPEORDSAPSSAP_E                     )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_TAG0_SRC_TAGGED_E                         )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_TAG1_SRC_TAGGED_E                         )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_SRC_TAG0_IS_OUTER_TAG_E                   )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_OUTER_SRC_TAG_E                           )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_TAG1_LOCAL_DEV_SRC_TAGGED_E               )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_NESTED_VLAN_EN_E                          )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_TAG0_PRIO_TAGGED_E                        )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_TAG1_PRIO_TAGGED_E                        )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_ORIG_VID_E                                )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_OVERRIDE_EVLAN_WITH_ORIGVID_E             )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_TRG_TAGGED_E                              )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_EVLAN_PRECEDENCE_E                        )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_RESERVED_5_E                              )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_TAG0_TPID_INDEX_E                         )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_TAG1_TPID_INDEX_E                         )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_NUM_OF_TAG_WORDS_TO_POP_E                 )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_IPX_PROTOCOL_E                            )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_IPV4_SIP_OR_ARP_SIP_OR_FCOE_S_ID_E        )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_IPV4_DIP_OR_ARP_DIP_OR_FCOE_D_ID_E        )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_POLICY_RTT_INDEX_E                        )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_L3_OFFSET_INVALID_E                       )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_IS_IP_E                                   )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_IS_IPV4_E                                 )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_IS_IPV6_E                                 )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_IS_FCOE_E                                 )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_IS_ARP_E                                  )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_IP_LEGAL_OR_FCOE_LEGAL_E                  )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_IPM_E                                     )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_IP_HEADER_INFO_E                          )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_IP_FRAGMENTED_E                           )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_FRAGMENTED_E                              )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_ROUTED_E                                  )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_DO_ROUTE_HA_E                             )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_MAC_SA_ARP_SA_MISMATCH_E                  )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_IPX_HEADER_LENGTH_E                       )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_RESERVED_6_E                              )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_IS_IP_1_E                                   )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_IS_IPV6_1_E                                 )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_IS_ND_E                                   )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_IS_IPV6_LINK_LOCAL_E                      )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_IS_IPV6_MLD_E                             )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_IPV6_HBH_EXT_E                            )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_IPV6_EH_E                                 )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_SOLICITATION_MULTICAST_MESSAGE_E          )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_IS_IP_2_E                                   )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_IS_IPV6_2_E                                 )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_IPV6_FLOW_LABEL_E                         )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_RESERVED_7_E                              )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_TCP_UDP_DEST_PORT_E                       )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_TCPORUDP_PORT_COMPARATORS_E               )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_L4_OFFSET_INVALID_E                       )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_L4_VALID_E                                )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_IS_SYN_E                                  )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_SYN_WITH_DATA_E                           )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_RESERVED_8_E                              )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_L2_ECHO_E                                 )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_CFM_PACKET_E                              )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_TIMESTAMP_EN_E                            )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_TIMESTAMP_TAGGED_E                        )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_RESERVED_9_E                              )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_TIMESTAMP_E                               )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_IS_PTP_E                                  )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_PTP_DOMAIN_E                              )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_PTP_U_FIELD_E                             )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_PTP_TAI_SELECT_E                          )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_PTP_TRIGGER_TYPE_E                        )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_OAM_PROCESSING_EN_E                       )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_OAM_PTP_OFFSET_INDEX_OR_PTP_OFFSET_E      )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_RX_SNIFF_OR_SRC_TRG_E                     )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_SRC_TRG_EPORT_E                           )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_SRC_TRG_DEV_E                             )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_ANALYZER_INDEX_E                          )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_RESERVED_10_E                             )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_SRC_TRG_PHY_PORT_E                        )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_IS_MPLS_E                                 )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_MPLS_CMD_E                                )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_RESERVED_11_E                             )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_ECN_CAPABLE_E                             )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_QCN_RX_E                                  )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_ORIG_RX_QCN_PRIO_E                        )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_RESERVED_12_E                             )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_TUNNEL_TERMINATED_E                       )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_INNER_PACKET_TYPE_E                       )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_TUNNEL_START_E                            )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_TUNNEL_START_PASSENGER_TYPE_E             )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_ARP_PTR_OR_TUNNEL_PTR_E                   )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_RESERVED_13_E                             )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_QOS_PROFILE_PRECEDENCE_E                  )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_QOS_PROFILE_E                             )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_MODIFY_UP_E                               )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_MODIFY_DSCP_EXP_E                         )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_RESERVED_14_E                             )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_BYTE_COUNT_E                              )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_RECALC_CRC_E                              )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_RESERVED_15_E                             )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_BYPASS_INGRESS_PIPE_E                     )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_MARVELL_TAGGED_E                          )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_MARVELL_TAGGED_EXTENDED_E                 )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_RESERVED_16_E                             )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_RECEIVED_DSA_TAG_WORD_0_E                 )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_RECEIVED_DSA_TAG_WORD_1_E                 )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_RECEIVED_DSA_TAG_WORD_2_E                 )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_RECEIVED_DSA_TAG_WORD_3_E                 )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_IS_TRILL_E                                )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_REP_E                                     )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_REP_LAST_E                                )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_PACKET_TYPE_E                             )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_RESERVED_17_E                             )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_APPLICABLE_FLOW_SUB_TEMPLATE_E            )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_METERING_EN_E                             )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_BILLING_EN_E                              )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_RESERVED_18_E                             )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_POLICER_PTR_E                             )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_INGRESS_CORE_ID_E                         )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_RX_IS_PROTECTION_PATH_E                   )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_RX_PROTECTION_SWITCH_EN_E                 )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_RESERVED_19_E                             )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_COPY_RESERVED_E                           )\
,STR(SMEM_LION3_IPCL_META_DATA_FIELDS_RESERVED_20_E                             )

static char * lion3IpclMetadataFieldsTableNames[
    SMEM_LION3_IPCL_META_DATA_FIELDS___LAST_VALUE___E] =
    {SMEM_LION3_IPCL_META_DATA_FIELDS_NAME};

static SNET_ENTRY_FORMAT_TABLE_STC lion3IpclMetadataTableFieldsFormat[
    SMEM_LION3_IPCL_META_DATA_FIELDS___LAST_VALUE___E] =
{
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_PORT_LIST_SRC_E*/
    STANDARD_FIELD_MAC(28)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_LOCAL_DEV_SRC_TRUNK_ID_E*/
    ,STANDARD_FIELD_MAC(12)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_LOCAL_DEV_SRC_EPORT_E*/
    ,STANDARD_FIELD_MAC(13)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_SRC_DEV_IS_OWN_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_RESERVED_1_E*/
    ,STANDARD_FIELD_MAC(2)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_ORIG_SRC_DEV_E*/
    ,STANDARD_FIELD_MAC(10)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_ORIG_SRC_IS_TRUNK_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_ORIG_SRC_EPORT_OR_TRUNK_ID_E*/
    ,STANDARD_FIELD_MAC(13)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_LOCAL_DEV_SRC_PORT_E*/
    ,STANDARD_FIELD_MAC(8)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_USE_VIDX_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_EVIDX_E*/
    ,STANDARD_FIELD_MAC(16)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_INGRESS_UDB_PACKET_TYPE_E*/
    ,STANDARD_FIELD_MAC(4)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_IP_LEGAL_OR_FCOE_LEGAL_DUP_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_L2_VALID_DUP_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_IS_ARP_DUP_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_TRG_DEV_E*/
    ,STANDARD_FIELD_MAC(10)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_TRG_IS_TRUNK_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_TRG_EPORT_OR_TRG_TRUNK_ID_E*/
    ,STANDARD_FIELD_MAC(13)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_TRG_PHY_PORT_E*/
    ,STANDARD_FIELD_MAC(8)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_IS_TRG_PHY_PORT_VALID_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_EGRESS_UDB_PACKET_TYPE_E*/
    ,STANDARD_FIELD_MAC(4)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_RESERVED_2_E*/
    ,STANDARD_FIELD_MAC(3)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_SRC_ID_E*/
    ,STANDARD_FIELD_MAC(12)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_EGRESS_FILTER_REGISTERED_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_PACKET_IS_LOOPED_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_DROP_ON_SOURCE_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_RESERVED_3_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_PHY_SRC_MC_FILTER_EN_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_ORIG_SRC_PHY_IS_TRUNK_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_ORIG_SRC_PHY_PORT_OR_TRUNK_ID_E*/
    ,STANDARD_FIELD_MAC(12)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_RESERVED_4_E*/
    ,STANDARD_FIELD_MAC(2)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_L2_VALID_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_L2_ENCAPSULATION_E*/
    ,STANDARD_FIELD_MAC(2)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_BYPASS_BRIDGE_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_MAC_DA_TYPE_E*/
    ,STANDARD_FIELD_MAC(2)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_OUTER_IS_LLC_NON_SNAP_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_INNER_IS_LLC_NON_SNAP_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_ETHERTYPEORDSAPSSAP_E*/
    ,STANDARD_FIELD_MAC(16)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_TAG0_SRC_TAGGED_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_TAG1_SRC_TAGGED_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_SRC_TAG0_IS_OUTER_TAG_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_OUTER_SRC_TAG_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_TAG1_LOCAL_DEV_SRC_TAGGED_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_NESTED_VLAN_EN_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_TAG0_PRIO_TAGGED_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_TAG1_PRIO_TAGGED_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_ORIG_VID_E*/
    ,STANDARD_FIELD_MAC(12)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_OVERRIDE_EVLAN_WITH_ORIGVID_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_TRG_TAGGED_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_EVLAN_PRECEDENCE_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_RESERVED_5_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_TAG0_TPID_INDEX_E*/
    ,STANDARD_FIELD_MAC(3)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_TAG1_TPID_INDEX_E*/
    ,STANDARD_FIELD_MAC(3)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_NUM_OF_TAG_WORDS_TO_POP_E*/
    ,STANDARD_FIELD_MAC(2)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_IPX_PROTOCOL_E*/
    ,STANDARD_FIELD_MAC(8)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_IPV4_SIP_OR_ARP_SIP_OR_FCOE_S_ID_E*/
    ,STANDARD_FIELD_MAC(32)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_IPV4_DIP_OR_ARP_DIP_OR_FCOE_D_ID_E*/
    ,STANDARD_FIELD_MAC(32)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_POLICY_RTT_INDEX_E*/
    ,STANDARD_FIELD_MAC(16)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_L3_OFFSET_INVALID_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_IS_IP_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_IS_IPV4_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_IS_IPV6_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_IS_FCOE_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_IS_ARP_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_IP_LEGAL_OR_FCOE_LEGAL_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_IPM_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_IP_HEADER_INFO_E*/
    ,STANDARD_FIELD_MAC(2)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_IP_FRAGMENTED_E*/
    ,STANDARD_FIELD_MAC(2)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_FRAGMENTED_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_ROUTED_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_DO_ROUTE_HA_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_MAC_SA_ARP_SA_MISMATCH_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_IPX_HEADER_LENGTH_E*/
    ,STANDARD_FIELD_MAC(5)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_RESERVED_6_E*/
    ,STANDARD_FIELD_MAC(3)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_IS_IP_1_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_IS_IPV6_1_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_IS_ND_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_IS_IPV6_LINK_LOCAL_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_IS_IPV6_MLD_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_IPV6_HBH_EXT_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_IPV6_EH_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_SOLICITATION_MULTICAST_MESSAGE_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_IS_IP_2_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_IS_IPV6_2_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_IPV6_FLOW_LABEL_E*/
    ,STANDARD_FIELD_MAC(20)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_RESERVED_7_E*/
    ,STANDARD_FIELD_MAC(2)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_TCP_UDP_DEST_PORT_E*/
    ,STANDARD_FIELD_MAC(16)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_TCPORUDP_PORT_COMPARATORS_E*/
    ,STANDARD_FIELD_MAC(8)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_L4_OFFSET_INVALID_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_L4_VALID_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_IS_SYN_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_SYN_WITH_DATA_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_RESERVED_8_E*/
    ,STANDARD_FIELD_MAC(4)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_L2_ECHO_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_CFM_PACKET_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_TIMESTAMP_EN_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_TIMESTAMP_TAGGED_E*/
    ,STANDARD_FIELD_MAC(3)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_RESERVED_9_E*/
    ,STANDARD_FIELD_MAC(2)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_TIMESTAMP_E*/
    ,STANDARD_FIELD_MAC(32)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_IS_PTP_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_PTP_DOMAIN_E*/
    ,STANDARD_FIELD_MAC(3)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_PTP_U_FIELD_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_PTP_TAI_SELECT_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_PTP_TRIGGER_TYPE_E*/
    ,STANDARD_FIELD_MAC(2)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_OAM_PROCESSING_EN_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_OAM_PTP_OFFSET_INDEX_OR_PTP_OFFSET_E*/
    ,STANDARD_FIELD_MAC(7)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_RX_SNIFF_OR_SRC_TRG_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_SRC_TRG_EPORT_E*/
    ,STANDARD_FIELD_MAC(13)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_SRC_TRG_DEV_E*/
    ,STANDARD_FIELD_MAC(10)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_ANALYZER_INDEX_E*/
    ,STANDARD_FIELD_MAC(3)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_RESERVED_10_E*/
    ,STANDARD_FIELD_MAC(5)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_SRC_TRG_PHY_PORT_E*/
    ,STANDARD_FIELD_MAC(8)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_IS_MPLS_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_MPLS_CMD_E*/
    ,STANDARD_FIELD_MAC(3)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_RESERVED_11_E*/
    ,STANDARD_FIELD_MAC(4)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_ECN_CAPABLE_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_QCN_RX_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_ORIG_RX_QCN_PRIO_E*/
    ,STANDARD_FIELD_MAC(3)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_RESERVED_12_E*/
    ,STANDARD_FIELD_MAC(3)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_TUNNEL_TERMINATED_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_INNER_PACKET_TYPE_E*/
    ,STANDARD_FIELD_MAC(3)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_TUNNEL_START_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_TUNNEL_START_PASSENGER_TYPE_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_ARP_PTR_OR_TUNNEL_PTR_E*/
    ,STANDARD_FIELD_MAC(17)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_RESERVED_13_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_QOS_PROFILE_PRECEDENCE_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_QOS_PROFILE_E*/
    ,STANDARD_FIELD_MAC(10)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_MODIFY_UP_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_MODIFY_DSCP_EXP_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_RESERVED_14_E*/
    ,STANDARD_FIELD_MAC(3)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_BYTE_COUNT_E*/
    ,STANDARD_FIELD_MAC(14)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_RECALC_CRC_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_RESERVED_15_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_BYPASS_INGRESS_PIPE_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_MARVELL_TAGGED_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_MARVELL_TAGGED_EXTENDED_E*/
    ,STANDARD_FIELD_MAC(2)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_RESERVED_16_E*/
    ,STANDARD_FIELD_MAC(4)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_RECEIVED_DSA_TAG_WORD_0_E*/
    ,STANDARD_FIELD_MAC(32)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_RECEIVED_DSA_TAG_WORD_1_E*/
    ,STANDARD_FIELD_MAC(32)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_RECEIVED_DSA_TAG_WORD_2_E*/
    ,STANDARD_FIELD_MAC(32)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_RECEIVED_DSA_TAG_WORD_3_E*/
    ,STANDARD_FIELD_MAC(32)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_IS_TRILL_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_REP_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_REP_LAST_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_PACKET_TYPE_E*/
    ,STANDARD_FIELD_MAC(3)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_RESERVED_17_E*/
    ,STANDARD_FIELD_MAC(2)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_APPLICABLE_FLOW_SUB_TEMPLATE_E*/
    ,STANDARD_FIELD_MAC(8)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_METERING_EN_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_BILLING_EN_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_RESERVED_18_E*/
    ,STANDARD_FIELD_MAC(6)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_POLICER_PTR_E*/
    ,STANDARD_FIELD_MAC(16)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_INGRESS_CORE_ID_E*/
    ,STANDARD_FIELD_MAC(4)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_RX_IS_PROTECTION_PATH_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_RX_PROTECTION_SWITCH_EN_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_RESERVED_19_E*/
    ,STANDARD_FIELD_MAC(2)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_COPY_RESERVED_E*/
    ,STANDARD_FIELD_MAC(20)
    /*SMEM_LION3_IPCL_META_DATA_FIELDS_RESERVED_20_E*/
    ,STANDARD_FIELD_MAC(4)
};

/* macro to shorten the calling code of 'ipcl metadata' fields */
#define SMEM_LION3_IPCL_METADATA_FIELD_SET(_devObjPtr,_descrPtr,fieldName,value) \
    snetFieldFromEntry_GT_U32_Set(_devObjPtr,                       \
        _descrPtr->ipclMetadataInfo,                                 \
        _devObjPtr->tableFormatInfo[SKERNEL_TABLE_FORMAT_IPCL_META_DATA_E].formatNamePtr, \
        0, /* not a table */                                        \
        _devObjPtr->tableFormatInfo[SKERNEL_TABLE_FORMAT_IPCL_META_DATA_E].fieldsInfoPtr, \
        _devObjPtr->tableFormatInfo[SKERNEL_TABLE_FORMAT_IPCL_META_DATA_E].fieldsNamePtr,   \
        fieldName,                                                  \
        value);

#define SMEM_LION3_EPCL_META_DATA_FIELDS_NAME                                   \
 STR(SMEM_LION3_EPCL_META_DATA_FIELDS_PORT_LIST_TRG_E                          )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_LOCAL_DEV_SRC_TRUNK_ID_E                 )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_LOCAL_DEV_SRC_EPORT_E                    )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_RESERVED_1_E                             )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_ORIG_SRC_DEV_E                           )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_ORIG_SRC_IS_TRUNK_E                      )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_ORIG_SRC_EPORT_OR_TRUNK_ID_E             )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_TRG_PORT_E                               )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_IS_TRG_PHY_PORT_VALID_E                  )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_RESERVED_2_E                             )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_LOCAL_DEV_TRG_PHY_PORT_E                 )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_TRG_DEV_E                                )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_TRG_EPORT_E                              )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_ASSIGN_TRG_EPORT_ATTRIBUTES_LOCALLY_E    )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_SRC_PORT_E                               )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_USE_VIDX_E                               )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_EVIDX_E                                  )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_EGRESS_UDB_PACKET_TYPE_E                 )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_IP_LEGAL_E                               )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_L2_VALID_E                               )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_IS_ARP_E                                 )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_SRC_ID_E                                 )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_EGRESS_FILTER_EN_E                       )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_EGRESS_FILTER_REGISTERED_E               )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_INCOMING_EGRESS_FILTER_EN_E              )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_RESERVED_4_E                             )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_ETHERTYPE_OR_DSAPSSAP_E                  )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_IS_UNKNOWN_E                             )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_L2_VALID_1_E                               )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_L2_ENCAPSULATION_TYPE_E                  )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_IS_BC_E                                  )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_ORIG_VID_E                               )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_EVLAN_E                                  )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_TAG0_EXIST_E                             )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_TAG1_EXIST_E                             )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_TAG0_IS_OUTER_TAG_E                      )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_TAG0_UP_E                                )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_TAG0_CFI_E                               )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_TAG1_UP_E                                )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_TAG1_CFI_E                               )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_TAG1_VID_E                               )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_EVB_OR_BPE_OR_RSPAN_TAG_SIZE_E           )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_RESERVED_5_E                             )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_IP_PROTOCOL_E                            )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_IPV4_SIP_OR_ARP_SIP_E                    )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_IPV4_DIP_OR_ARP_DIP_E                    )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_ROUTED_E                                 )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_IS_IP_E                                  )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_IS_IPV4_E                                )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_IS_IPV6_E                                )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_IP_LEGAL_1_E                               )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_IS_ARP_1_E                                 )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_IPV4_FRAGMENTED_E                        )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_IPV4_OPTIONS_E                           )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_IS_IP_1_E                                  )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_IS_IPV4_1_E                                )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_IS_IPV6_1_E                                )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_IP_LEGAL_2_E                               )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_IPV6_EH_EXIST_E                          )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_IPV6_EH_HOP_BY_HOP_E                     )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_IPV6_IS_ND_E                             )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_L4_VALID_E                               )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_TCP_OR_UDP_PORT_COMPARATORS_E            )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_IS_PTP_E                                 )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_TIMESTAMP_EN_E                           )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_INGRESS_TIMESTAMP_TAGGED_E               )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_TIMESTAMP_TAGGED_E                       )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_LM_COUNTER_OR_TIMESTAMP_E                )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_LM_COUNTER_INSERT_EN_E                   )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_PTP_U_FIELD_E                            )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_PTP_TAI_SELECT_E                         )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_PTP_DOMAIN_E                             )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_PTP_TRIGGER_TYPE_E                       )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_PTP_MESSAGE_TYPE_E                       )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_CONTROL_WORD_CHANNEL_TYPE_PROFILE_E      )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_PTP_VERSION_E                            )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_OAM_PTP_OFFSET_INDEX_OR_PTP_OFFSET_E     )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_RESERVED_LABEL_EXISTS_E                  )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_RESERVED_LABEL_VALUE_E                   )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_RESERVED_6_E                             )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_DATA_AFTER_INNER_LABEL_E                 )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_RESERVED_7_E                             )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_SRC_TRG_OR_RX_SNIFF_E                    )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_ANALYZER_TRG_EPORT_E                     )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_ANALYZER_TRG_DEV_E                       )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_ANALYZER_TRG_PHY_PORT_E                  )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_SRC_TRG_TAG0_TAGGED_E                    )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_ANALYZER_IS_TRG_PHY_PORT_VALID_E         )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_RESERVED_8_E                             )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_IS_MPLS_E                                )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_PROTOCOL_AFTER_MPLS_E                    )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_NUMBER_OF_MPLS_LABELS_E                  )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_IS_TUNNELED_E                            )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_TUNNEL_START_PASSENGER_TYPE_E            )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_RESERVED_9_E                             )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_QOS_PROFILE_E                            )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_QOS_MAPPING_TABLE_INDEX_E                )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_FROM_CPU_DP_E                            )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_PACKET_TC_E                              )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_EGRESS_UP_MAP_EN_E                       )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_EGRESS_DSCP_MAP_EN_E                     )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_EGRESS_EXP_MAP_EN_E                      )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_EGRESS_TC_DP_MAP_EN_E                    )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_EGRESS_DP2CFI_MAP_EN_E                   )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_INGRESS_BYTE_COUNT_E                     )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_INCOMING_MTAG_CMD_E                      )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_EGRESS_MTAG_CMD_E                        )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_EGRESS_MARVELL_TAGGED_E                  )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_EGRESS_MARVELL_TAGGED_EXTENDED_E         )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_ADD_CRC_E                                )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_NUM_OF_CRC_IN_EGRESS_BYTE_COUNT_E        )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_INGRESS_MARVELL_TAGGED_E                 )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_RESERVED_10_E                            )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_TRANSMITTED_DSA_TAG_WORD_0_E             )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_TRANSMITTED_DSA_TAG_WORD_1_E             )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_TRANSMITTED_DSA_TAG_WORD_2_E             )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_TRANSMITTED_DSA_TAG_WORD_3_E             )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_FLOW_ID_E                                )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_OUTER_PACKET_TYPE_E                      )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_INNER_PACKET_TYPE_E                      )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_PACKET_CMD_DROP_OR_FORWARD_E             )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_TXQ_QUEUE_ID_E                           )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_PACKET_HASH_E                            )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_CPU_CODE_E                               )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_TWO_BYTE_HEADER_ADDED_E                  )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_COPY_RESERVED_E                          )\
,STR(SMEM_LION3_EPCL_META_DATA_FIELDS_RESERVED_11_E                            )


static char * lion3EpclMetadataFieldsTableNames[
    SMEM_LION3_EPCL_META_DATA_FIELDS___LAST_VALUE___E] =
    {SMEM_LION3_EPCL_META_DATA_FIELDS_NAME};

static SNET_ENTRY_FORMAT_TABLE_STC lion3EpclMetadataTableFieldsFormat[
    SMEM_LION3_EPCL_META_DATA_FIELDS___LAST_VALUE___E] =
{
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_PORT_LIST_TRG_E*/
    STANDARD_FIELD_MAC(28)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_LOCAL_DEV_SRC_TRUNK_ID_E*/
    ,STANDARD_FIELD_MAC(12)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_LOCAL_DEV_SRC_EPORT_E*/
    ,STANDARD_FIELD_MAC(13)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_RESERVED_1_E*/
    ,STANDARD_FIELD_MAC(3)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_ORIG_SRC_DEV_E*/
    ,STANDARD_FIELD_MAC(10)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_ORIG_SRC_IS_TRUNK_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_ORIG_SRC_EPORT_OR_TRUNK_ID_E*/
    ,STANDARD_FIELD_MAC(13)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_TRG_PORT_E*/
    ,STANDARD_FIELD_MAC(8)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_IS_TRG_PHY_PORT_VALID_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_RESERVED_2_E*/
    ,STANDARD_FIELD_MAC(7)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_LOCAL_DEV_TRG_PHY_PORT_E*/
    ,STANDARD_FIELD_MAC(8)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_TRG_DEV_E*/
    ,STANDARD_FIELD_MAC(10)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_TRG_EPORT_E*/
    ,STANDARD_FIELD_MAC(13)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_ASSIGN_TRG_EPORT_ATTRIBUTES_LOCALLY_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_SRC_PORT_E*/
    ,STANDARD_FIELD_MAC(8)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_USE_VIDX_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_EVIDX_E*/
    ,STANDARD_FIELD_MAC(16)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_EGRESS_UDB_PACKET_TYPE_E*/
    ,STANDARD_FIELD_MAC(4)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_IP_LEGAL_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_L2_VALID_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_IS_ARP_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_SRC_ID_E*/
    ,STANDARD_FIELD_MAC(12)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_EGRESS_FILTER_EN_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_EGRESS_FILTER_REGISTERED_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_INCOMING_EGRESS_FILTER_EN_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_RESERVED_4_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_ETHERTYPE_OR_DSAPSSAP_E*/
    ,STANDARD_FIELD_MAC(16)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_IS_UNKNOWN_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_L2_VALID_1_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_L2_ENCAPSULATION_TYPE_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_IS_BC_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_ORIG_VID_E*/
    ,STANDARD_FIELD_MAC(12)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_EVLAN_E*/
    ,STANDARD_FIELD_MAC(13)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_TAG0_EXIST_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_TAG1_EXIST_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_TAG0_IS_OUTER_TAG_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_TAG0_UP_E*/
    ,STANDARD_FIELD_MAC(3)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_TAG0_CFI_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_TAG1_UP_E*/
    ,STANDARD_FIELD_MAC(3)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_TAG1_CFI_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_TAG1_VID_E*/
    ,STANDARD_FIELD_MAC(12)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_EVB_OR_BPE_OR_RSPAN_TAG_SIZE_E*/
    ,STANDARD_FIELD_MAC(2)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_RESERVED_5_E*/
    ,STANDARD_FIELD_MAC(2)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_IP_PROTOCOL_E*/
    ,STANDARD_FIELD_MAC(8)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_IPV4_SIP_OR_ARP_SIP_E*/
    ,STANDARD_FIELD_MAC(32)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_IPV4_DIP_OR_ARP_DIP_E*/
    ,STANDARD_FIELD_MAC(32)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_ROUTED_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_IS_IP_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_IS_IPV4_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_IS_IPV6_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_IP_LEGAL_1_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_IS_ARP_1_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_IPV4_FRAGMENTED_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_IPV4_OPTIONS_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_IS_IP_1_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_IS_IPV4_1_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_IS_IPV6_1_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_IP_LEGAL_2_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_IPV6_EH_EXIST_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_IPV6_EH_HOP_BY_HOP_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_IPV6_IS_ND_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_L4_VALID_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_TCP_OR_UDP_PORT_COMPARATORS_E*/
    ,STANDARD_FIELD_MAC(8)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_IS_PTP_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_TIMESTAMP_EN_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_INGRESS_TIMESTAMP_TAGGED_E*/
    ,STANDARD_FIELD_MAC(3)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_TIMESTAMP_TAGGED_E*/
    ,STANDARD_FIELD_MAC(3)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_LM_COUNTER_OR_TIMESTAMP_E*/
    ,STANDARD_FIELD_MAC(32)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_LM_COUNTER_INSERT_EN_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_PTP_U_FIELD_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_PTP_TAI_SELECT_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_PTP_DOMAIN_E*/
    ,STANDARD_FIELD_MAC(3)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_PTP_TRIGGER_TYPE_E*/
    ,STANDARD_FIELD_MAC(2)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_PTP_MESSAGE_TYPE_E*/
    ,STANDARD_FIELD_MAC(4)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_CONTROL_WORD_CHANNEL_TYPE_PROFILE_E*/
    ,STANDARD_FIELD_MAC(4)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_PTP_VERSION_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_OAM_PTP_OFFSET_INDEX_OR_PTP_OFFSET_E*/
    ,STANDARD_FIELD_MAC(7)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_RESERVED_LABEL_EXISTS_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_RESERVED_LABEL_VALUE_E*/
    ,STANDARD_FIELD_MAC(4)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_RESERVED_6_E*/
    ,STANDARD_FIELD_MAC(3)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_DATA_AFTER_INNER_LABEL_E*/
    ,STANDARD_FIELD_MAC(5)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_RESERVED_7_E*/
    ,STANDARD_FIELD_MAC(3)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_SRC_TRG_OR_RX_SNIFF_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_ANALYZER_TRG_EPORT_E*/
    ,STANDARD_FIELD_MAC(13)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_ANALYZER_TRG_DEV_E*/
    ,STANDARD_FIELD_MAC(10)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_ANALYZER_TRG_PHY_PORT_E*/
    ,STANDARD_FIELD_MAC(8)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_SRC_TRG_TAG0_TAGGED_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_ANALYZER_IS_TRG_PHY_PORT_VALID_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_RESERVED_8_E*/
    ,STANDARD_FIELD_MAC(6)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_IS_MPLS_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_PROTOCOL_AFTER_MPLS_E*/
    ,STANDARD_FIELD_MAC(2)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_NUMBER_OF_MPLS_LABELS_E*/
    ,STANDARD_FIELD_MAC(2)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_IS_TUNNELED_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_TUNNEL_START_PASSENGER_TYPE_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_RESERVED_9_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_QOS_PROFILE_E*/
    ,STANDARD_FIELD_MAC(10)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_QOS_MAPPING_TABLE_INDEX_E*/
    ,STANDARD_FIELD_MAC(4)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_FROM_CPU_DP_E*/
    ,STANDARD_FIELD_MAC(2)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_PACKET_TC_E*/
    ,STANDARD_FIELD_MAC(3)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_EGRESS_UP_MAP_EN_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_EGRESS_DSCP_MAP_EN_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_EGRESS_EXP_MAP_EN_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_EGRESS_TC_DP_MAP_EN_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_EGRESS_DP2CFI_MAP_EN_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_INGRESS_BYTE_COUNT_E*/
    ,STANDARD_FIELD_MAC(14)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_INCOMING_MTAG_CMD_E*/
    ,STANDARD_FIELD_MAC(2)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_EGRESS_MTAG_CMD_E*/
    ,STANDARD_FIELD_MAC(2)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_EGRESS_MARVELL_TAGGED_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_EGRESS_MARVELL_TAGGED_EXTENDED_E*/
    ,STANDARD_FIELD_MAC(2)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_ADD_CRC_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_NUM_OF_CRC_IN_EGRESS_BYTE_COUNT_E*/
    ,STANDARD_FIELD_MAC(2)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_INGRESS_MARVELL_TAGGED_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_RESERVED_10_E*/
    ,STANDARD_FIELD_MAC(7)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_TRANSMITTED_DSA_TAG_WORD_0_E*/
    ,STANDARD_FIELD_MAC(32)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_TRANSMITTED_DSA_TAG_WORD_1_E*/
    ,STANDARD_FIELD_MAC(32)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_TRANSMITTED_DSA_TAG_WORD_2_E*/
    ,STANDARD_FIELD_MAC(32)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_TRANSMITTED_DSA_TAG_WORD_3_E*/
    ,STANDARD_FIELD_MAC(32)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_FLOW_ID_E*/
    ,STANDARD_FIELD_MAC(12)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_OUTER_PACKET_TYPE_E*/
    ,STANDARD_FIELD_MAC(2)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_INNER_PACKET_TYPE_E*/
    ,STANDARD_FIELD_MAC(2)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_PACKET_CMD_DROP_OR_FORWARD_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_TXQ_QUEUE_ID_E*/
    ,STANDARD_FIELD_MAC(11)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_PACKET_HASH_E*/
    ,STANDARD_FIELD_MAC(12)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_CPU_CODE_E*/
    ,STANDARD_FIELD_MAC(8)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_TWO_BYTE_HEADER_ADDED_E*/
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_COPY_RESERVED_E*/
    ,STANDARD_FIELD_MAC(20)
    /*SMEM_LION3_EPCL_META_DATA_FIELDS_RESERVED_11_E*/
    ,STANDARD_FIELD_MAC(3)
};

/* macro to shorten the calling code of 'Epcl metadata' fields */
#define SMEM_LION3_EPCL_METADATA_FIELD_SET(_devObjPtr,_descrPtr,fieldName,value) \
    snetFieldFromEntry_GT_U32_Set(_devObjPtr,                       \
        _descrPtr->epclMetadataInfo,                                 \
        _devObjPtr->tableFormatInfo[SKERNEL_TABLE_FORMAT_EPCL_META_DATA_E].formatNamePtr, \
        0, /* not a table */                                        \
        _devObjPtr->tableFormatInfo[SKERNEL_TABLE_FORMAT_EPCL_META_DATA_E].fieldsInfoPtr, \
        _devObjPtr->tableFormatInfo[SKERNEL_TABLE_FORMAT_EPCL_META_DATA_E].fieldsNamePtr, \
        fieldName,                                                  \
        value);


/* Array that holds the info about the tcam key fields - lion3 format */
static CHT_PCL_KEY_FIELDS_INFO_STC lion3EPclKeyFieldsData[]=
{
    /* NOTE the UDBs are not exists in this table */

    /* common field */
     { 0 ,  7 ,  GT_TRUE,      "LION3_EPCL_KEY_FIELD_NAME_BUILD( 0 ,  7 ,PCL_ID_7_0_OR_port_list_7_0                )" }
    ,{ 8 ,  9 ,  GT_TRUE,      "LION3_EPCL_KEY_FIELD_NAME_BUILD( 8 ,  9 ,PCL_ID_9_8                                 )" }
    ,{10 , 15 ,  GT_TRUE,      "LION3_EPCL_KEY_FIELD_NAME_BUILD(10 , 15 ,Src_Port_5_0                               )" }
    ,{16 , 16 ,  GT_TRUE,      "LION3_EPCL_KEY_FIELD_NAME_BUILD(16 , 16 ,Is_Tagged                                  )" }
    ,{17 , 28 ,  GT_TRUE,      "LION3_EPCL_KEY_FIELD_NAME_BUILD(17 , 28 ,VID0_eVLAN_11_0                            )" }
    ,{29 , 31 ,  GT_TRUE,      "LION3_EPCL_KEY_FIELD_NAME_BUILD(29 , 31 ,UP0                                        )" }
    ,{32 , 38 ,  GT_TRUE,      "LION3_EPCL_KEY_FIELD_NAME_BUILD(32 , 38 ,Data_Pkt_QoS_Profile_6_0_OR_port_list_13_8 )" }
    ,{32 , 38 ,  GT_TRUE,      "LION3_EPCL_KEY_FIELD_NAME_BUILD(32 , 38 ,Ctrl_To_CPU_CPU_CODE_6_0                   )" }
    ,{32 , 38 ,  GT_TRUE,      "LION3_EPCL_KEY_FIELD_NAME_BUILD(32 , 38 ,Ctrl_From_CPU_TC_2_0_DP_1_0                )" }
    ,{39 , 39 ,  GT_TRUE,      "LION3_EPCL_KEY_FIELD_NAME_BUILD(39 , 39 ,Is_IPv4                                    )" }
    ,{39 , 39 ,  GT_TRUE,      "LION3_EPCL_KEY_FIELD_NAME_BUILD(39 , 39 ,Is_IPv6                                    )" }
    ,{40 , 40 ,  GT_TRUE,      "LION3_EPCL_KEY_FIELD_NAME_BUILD(40 , 40 ,Is_IP                                      )" }
    ,{41 , 48 ,  GT_TRUE,      "LION3_EPCL_KEY_FIELD_NAME_BUILD(41 , 48 ,Ip_Protocol                                )" }
    ,{49 , 54 ,  GT_TRUE,      "LION3_EPCL_KEY_FIELD_NAME_BUILD(49 , 54 ,Packet_Dscp                                )" }
    ,{55 , 55 ,  GT_TRUE,      "LION3_EPCL_KEY_FIELD_NAME_BUILD(55 , 55 ,Is_L4_Valid                                )" }
    ,{56 , 63 ,  GT_TRUE,      "LION3_EPCL_KEY_FIELD_NAME_BUILD(56 , 63 ,L4_Header_Byte_3_OR_5                      )" }
    ,{64 , 71 ,  GT_TRUE,      "LION3_EPCL_KEY_FIELD_NAME_BUILD(64 , 71 ,L4_Header_Byte_2_OR_4                      )" }
    ,{72 , 72 ,  GT_TRUE,      "LION3_EPCL_KEY_FIELD_NAME_BUILD(72 , 72 ,Is_ARP                                     )" }
    ,{72 , 79 ,  GT_TRUE,      "LION3_EPCL_KEY_FIELD_NAME_BUILD(72 , 79 ,L4_Header_Byte_13                          )" }
    ,{80 , 87 ,  GT_TRUE,      "LION3_EPCL_KEY_FIELD_NAME_BUILD(80 , 87 ,L4_Header_Byte_1                           )" }
    ,{88 , 95 ,  GT_TRUE,      "LION3_EPCL_KEY_FIELD_NAME_BUILD(88 , 95 ,L4_Header_Byte_0                           )" }
    ,{96 ,127 ,  GT_TRUE,      "LION3_EPCL_KEY_FIELD_NAME_BUILD(96 ,127 ,SIP_31_0                                   )" }

    ,{74 , 80 ,  GT_TRUE,      "LION3_EPCL_KEY_FIELD_NAME_BUILD(74 , 80 ,Data_Pkt_OR_CtrlToCPU_Src_Trunk_ID_6_0     )" }
    ,{74 , 80 ,  GT_TRUE,      "LION3_EPCL_KEY_FIELD_NAME_BUILD(74 , 80 ,Data_Pkt_Src_Dev_4_0                       )" }
    ,{74 , 80 ,  GT_TRUE,      "LION3_EPCL_KEY_FIELD_NAME_BUILD(74 , 80 ,Ctrl_To_CPU_Src_Trg_Src_Dev_4_0            )" }
    ,{74 , 80 ,  GT_TRUE,      "LION3_EPCL_KEY_FIELD_NAME_BUILD(74 , 80 ,From_CPU_Egress_Filter_En_Src_Dev_4_0      )" }
    ,{74 , 80 ,  GT_TRUE,      "LION3_EPCL_KEY_FIELD_NAME_BUILD(74 , 80 ,To_Analyzer_Rx_Sniff_Src_Dev_4_0           )" }
    ,{81 , 81 ,  GT_TRUE,      "LION3_EPCL_KEY_FIELD_NAME_BUILD(81 , 81 ,Data_Pkt_OR_Ctrl_To_CPU_Src_Is_Trunk       )" }
    ,{82 , 86 ,  GT_TRUE,      "LION3_EPCL_KEY_FIELD_NAME_BUILD(82 , 86 ,Source_ID_4_0                              )" }
    ,{87 , 87 ,  GT_TRUE,      "LION3_EPCL_KEY_FIELD_NAME_BUILD(87 , 87 ,Is_Routed                                  )" }
    ,{88 , 88 ,  GT_TRUE,      "LION3_EPCL_KEY_FIELD_NAME_BUILD(88 , 88 ,Data_Pkt_Is_Unknown                        )" }
    ,{88 , 88 ,  GT_TRUE,      "LION3_EPCL_KEY_FIELD_NAME_BUILD(88 , 88 ,Ctrl_To_CPU_CPU_CODE_7                     )" }
    ,{88 , 88 ,  GT_TRUE,      "LION3_EPCL_KEY_FIELD_NAME_BUILD(88 , 88 ,From_CPU_Is_Unknown                        )" }
    ,{89 , 89 ,  GT_TRUE,      "LION3_EPCL_KEY_FIELD_NAME_BUILD(89 , 89 ,Is_L2_Valid                                )" }

    ,{187,188 ,  GT_TRUE,      "LION3_EPCL_KEY_FIELD_NAME_BUILD(187,188 ,Packet_Type_1_0                            )" }
    ,{190,190 ,  GT_TRUE,      "LION3_EPCL_KEY_FIELD_NAME_BUILD(190,190 ,is_VIDX                                    )" }
    ,{191,191 ,  GT_TRUE,      "LION3_EPCL_KEY_FIELD_NAME_BUILD(191,191 ,Tag_1_Exist                                )" }
    ,{192,205 ,  GT_TRUE,      "LION3_EPCL_KEY_FIELD_NAME_BUILD(192,205 ,port_list_27_14                            )" }

    /* key0 fields */
    ,{41 , 41 ,  GT_TRUE,      "LION3_EPCL_KEY0_FIELD_NAME_BUILD(41 , 41 ,UP1_2                    )" }
    ,{42 , 57 ,  GT_TRUE,      "LION3_EPCL_KEY0_FIELD_NAME_BUILD(42 , 57 ,Ether_Type_or_Dsap_Ssap  )" }
    ,{58 , 69 ,  GT_TRUE,      "LION3_EPCL_KEY0_FIELD_NAME_BUILD(58 , 69 ,Tag1_VID                 )" }
    ,{70 , 71 ,  GT_TRUE,      "LION3_EPCL_KEY0_FIELD_NAME_BUILD(70 , 71 ,UP1_1_0                  )" }
    ,{90 , 90 ,  GT_TRUE,      "LION3_EPCL_KEY0_FIELD_NAME_BUILD(90 , 90 ,L2_Encap_Type            )" }
    ,{91 ,138 ,  GT_TRUE,      "LION3_EPCL_KEY0_FIELD_NAME_BUILD(91 ,138 ,MAC_SA                   )" }
    ,{139,186 ,  GT_TRUE,      "LION3_EPCL_KEY0_FIELD_NAME_BUILD(139,186 ,MAC_DA                   )" }
    ,{189,189 ,  GT_TRUE,      "LION3_EPCL_KEY0_FIELD_NAME_BUILD(189,189 ,Tag_1_CFI                )" }
    ,{206,206 ,  GT_TRUE,      "LION3_EPCL_KEY0_FIELD_NAME_BUILD(206,206 ,User_Defined_Valid       )" }

    /* key1 fields */
    ,{ 90, 97 ,  GT_TRUE,      "LION3_EPCL_KEY1_FIELD_NAME_BUILD( 90, 97 ,TCP_UDP_Port_Comparators )" }
    ,{ 98,129 ,  GT_TRUE,      "LION3_EPCL_KEY1_FIELD_NAME_BUILD( 98,129 ,DIP_31_0                 )" }
    ,{130,137 ,  GT_TRUE,      "LION3_EPCL_KEY1_FIELD_NAME_BUILD(130,137 ,L4_Header_Byte_13        )" }
    ,{189,189 ,  GT_TRUE,      "LION3_EPCL_KEY1_FIELD_NAME_BUILD(189,189 ,IPv4_fragmented          )" }

    /* key2 fields */
    ,{ 73, 73 ,  GT_TRUE,      "LION3_EPCL_KEY2_FIELD_NAME_BUILD( 73, 73 ,Is_BC                    )" }
    ,{ 98,129 ,  GT_TRUE,      "LION3_EPCL_KEY2_FIELD_NAME_BUILD( 98,129 ,SIP_31_0                 )" }
    ,{130,161 ,  GT_TRUE,      "LION3_EPCL_KEY2_FIELD_NAME_BUILD(130,161 ,DIP_31_0                 )" }
    ,{162,169 ,  GT_TRUE,      "LION3_EPCL_KEY2_FIELD_NAME_BUILD(162,169 ,L4_Header_Byte_13        )" }
    ,{170,177 ,  GT_TRUE,      "LION3_EPCL_KEY2_FIELD_NAME_BUILD(170,177 ,L4_Header_Byte_1         )" }
    ,{178,185 ,  GT_TRUE,      "LION3_EPCL_KEY2_FIELD_NAME_BUILD(178,185 ,L4_Header_Byte_0         )" }

    /* key3 fields */
    ,{128,159 ,  GT_TRUE,      "LION3_EPCL_KEY3_FIELD_NAME_BUILD(128,159 ,DIP_31_0                 )" }
    ,{160,160 ,  GT_TRUE,      "LION3_EPCL_KEY3_FIELD_NAME_BUILD(160,160 ,L2_Encap_Type            )" }
    ,{161,176 ,  GT_TRUE,      "LION3_EPCL_KEY3_FIELD_NAME_BUILD(161,176 ,Ether_Type_OR_Dsap_Ssap  )" }
    ,{177,177 ,  GT_TRUE,      "LION3_EPCL_KEY3_FIELD_NAME_BUILD(177,177 ,IPv4_fragmented          )" }
    ,{208,219 ,  GT_TRUE,      "LION3_EPCL_KEY3_FIELD_NAME_BUILD(208,219 ,Tag1_VID                 )" }
    ,{220,220 ,  GT_TRUE,      "LION3_EPCL_KEY3_FIELD_NAME_BUILD(220,220 ,Tag1_CFI                 )" }
    ,{221,223 ,  GT_TRUE,      "LION3_EPCL_KEY3_FIELD_NAME_BUILD(221,223 ,UP1                      )" }
    ,{224,225 ,  GT_TRUE,      "LION3_EPCL_KEY3_FIELD_NAME_BUILD(224,225 ,Number_of_MPLS_Labels    )" }
    ,{226,227 ,  GT_TRUE,      "LION3_EPCL_KEY3_FIELD_NAME_BUILD(226,227 ,Protocol_After_MPLS      )" }
    ,{228,247 ,  GT_TRUE,      "LION3_EPCL_KEY3_FIELD_NAME_BUILD(228,247 ,MPLS_Label_1             )" }
    ,{248,250 ,  GT_TRUE,      "LION3_EPCL_KEY3_FIELD_NAME_BUILD(248,250 ,MPLS_EXP_1               )" }
    ,{251,298 ,  GT_TRUE,      "LION3_EPCL_KEY3_FIELD_NAME_BUILD(251,298 ,MAC_SA                   )" }
    ,{299,346 ,  GT_TRUE,      "LION3_EPCL_KEY3_FIELD_NAME_BUILD(299,346 ,MAC_DA                   )" }
    ,{347,347 ,  GT_TRUE,      "LION3_EPCL_KEY3_FIELD_NAME_BUILD(347,347 ,Is_MPLS                  )" }
    ,{348,367 ,  GT_TRUE,      "LION3_EPCL_KEY3_FIELD_NAME_BUILD(348,367 ,MPLS_Label_0             )" }
    ,{368,370 ,  GT_TRUE,      "LION3_EPCL_KEY3_FIELD_NAME_BUILD(368,370 ,MPLS_EXP_0               )" }
    ,{371,377 ,  GT_TRUE,      "LION3_EPCL_KEY3_FIELD_NAME_BUILD(371,377 ,Data_Pkt_OR_CtrlToCPU_Src_Trunk_ID_6_0     )" }
    ,{371,377 ,  GT_TRUE,      "LION3_EPCL_KEY3_FIELD_NAME_BUILD(371,377 ,Data_Pkt_Src_Dev_4_0                       )" }
    ,{371,377 ,  GT_TRUE,      "LION3_EPCL_KEY3_FIELD_NAME_BUILD(371,377 ,Ctrl_To_CPU_Src_Trg_Src_Dev_4_0            )" }
    ,{371,377 ,  GT_TRUE,      "LION3_EPCL_KEY3_FIELD_NAME_BUILD(371,377 ,From_CPU_Egress_Filter_En_Src_Dev_4_0      )" }
    ,{371,377 ,  GT_TRUE,      "LION3_EPCL_KEY3_FIELD_NAME_BUILD(371,377 ,To_Analyzer_Rx_Sniff_Src_Dev_4_0           )" }
    ,{378,378 ,  GT_TRUE,      "LION3_EPCL_KEY3_FIELD_NAME_BUILD(378,378 ,Data_Pkt_OR_Ctrl_To_CPU_Src_Is_Trunk       )" }
    ,{379,383 ,  GT_TRUE,      "LION3_EPCL_KEY3_FIELD_NAME_BUILD(379,383 ,Source_ID_4_0                              )" }
    ,{384,384 ,  GT_TRUE,      "LION3_EPCL_KEY3_FIELD_NAME_BUILD(384,384 ,Is_Routed                                  )" }
    ,{385,385 ,  GT_TRUE,      "LION3_EPCL_KEY3_FIELD_NAME_BUILD(385,385 ,Data_Pkt_Is_Unknown                        )" }
    ,{385,385 ,  GT_TRUE,      "LION3_EPCL_KEY3_FIELD_NAME_BUILD(385,385 ,Ctrl_To_CPU_CPU_CODE_7                     )" }
    ,{385,385 ,  GT_TRUE,      "LION3_EPCL_KEY3_FIELD_NAME_BUILD(385,385 ,From_CPU_Is_Unknown                        )" }
    ,{386,386 ,  GT_TRUE,      "LION3_EPCL_KEY3_FIELD_NAME_BUILD(386,386 ,Is_L2_Valid                                )" }
    ,{387,394 ,  GT_TRUE,      "LION3_EPCL_KEY3_FIELD_NAME_BUILD(387,394 ,TCP_UDP_Port_Comparators                   )" }
    ,{395,396 ,  GT_TRUE,      "LION3_EPCL_KEY3_FIELD_NAME_BUILD(395,396 ,Src_Port_7_6                               )" }
    ,{397,398 ,  GT_TRUE,      "LION3_EPCL_KEY3_FIELD_NAME_BUILD(397,398 ,Packet_Type_1_0                            )" }
    ,{399,399 ,  GT_TRUE,      "LION3_EPCL_KEY3_FIELD_NAME_BUILD(399,399 ,is_VIDX                                    )" }
    ,{400,401 ,  GT_TRUE,      "LION3_EPCL_KEY3_FIELD_NAME_BUILD(400,401 ,Source_Dev_6_5                             )" }
    ,{402,406 ,  GT_TRUE,      "LION3_EPCL_KEY3_FIELD_NAME_BUILD(402,406 ,Source_Dev_11_7_OR_Trunk_ID_11_7           )" }
    ,{407,413 ,  GT_TRUE,      "LION3_EPCL_KEY3_FIELD_NAME_BUILD(407,413 ,Source_ID_11_5                             )" }
    ,{414,414 ,  GT_TRUE,      "LION3_EPCL_KEY3_FIELD_NAME_BUILD(414,414 ,User_Defined_Valid                         )" }

    /* key4 fields */
    ,{128,159 ,  GT_TRUE,      "LION3_EPCL_KEY4_FIELD_NAME_BUILD(128,159 ,SIP_63_32                )" }
    ,{160,190 ,  GT_TRUE,      "LION3_EPCL_KEY4_FIELD_NAME_BUILD(160,190 ,SIP_94_64                )" }
    ,{208,208 ,  GT_TRUE,      "LION3_EPCL_KEY4_FIELD_NAME_BUILD(208,208 ,SIP_95_95                )" }
    ,{209,240 ,  GT_TRUE,      "LION3_EPCL_KEY4_FIELD_NAME_BUILD(209,240 ,SIP_127_96               )" }
    ,{241,241 ,  GT_TRUE,      "LION3_EPCL_KEY4_FIELD_NAME_BUILD(241,241 ,Ipv6_EH_exist            )" }
    ,{242,242 ,  GT_TRUE,      "LION3_EPCL_KEY4_FIELD_NAME_BUILD(242,242 ,Is_IPv6_EH_Hop_By_Hop    )" }
    ,{243,250 ,  GT_TRUE,      "LION3_EPCL_KEY4_FIELD_NAME_BUILD(243,250 ,DIP_127_120              )" }
    ,{347,358 ,  GT_TRUE,      "LION3_EPCL_KEY4_FIELD_NAME_BUILD(347,358 , Tag1_VID                )" }
    ,{359,359 ,  GT_TRUE,      "LION3_EPCL_KEY4_FIELD_NAME_BUILD(359,359 , Tag1_CFI                )" }
    ,{360,362 ,  GT_TRUE,      "LION3_EPCL_KEY4_FIELD_NAME_BUILD(360,362 , UP1                     )" }

    /* key5 fields */
    ,{251,282 ,  GT_TRUE,      "LION3_EPCL_KEY5_FIELD_NAME_BUILD(251,282 ,DIP_63_32                )" }
    ,{283,314 ,  GT_TRUE,      "LION3_EPCL_KEY5_FIELD_NAME_BUILD(283,314 ,DIP_95_64                )" }
    ,{315,338 ,  GT_TRUE,      "LION3_EPCL_KEY5_FIELD_NAME_BUILD(315,338 ,DIP_119_96               )" }
    ,{339,370 ,  GT_TRUE,      "LION3_EPCL_KEY5_FIELD_NAME_BUILD(339,370 ,DIP_31_0                 )" }


    /* key6 and key 7 common fields */
    ,{0  ,  9 ,  GT_TRUE,      "LION3_EPCL_KEY6_FIELD_NAME_BUILD(0  ,  9 ,PCL_ID_9_0                )" }
    ,{11 , 16 ,  GT_TRUE,      "LION3_EPCL_KEY6_FIELD_NAME_BUILD(11 , 16 ,Src_Port_5_0              )" }
    ,{17 , 23 ,  GT_TRUE,      "LION3_EPCL_KEY6_FIELD_NAME_BUILD(17 , 23 ,Data_Pkt_QoS_Profile_6_0  )" }
    ,{17 , 23 ,  GT_TRUE,      "LION3_EPCL_KEY6_FIELD_NAME_BUILD(17 , 23 ,Ctrl_To_CPU_CPU_CODE_6_0  )" }
    ,{24 , 24 ,  GT_TRUE,      "LION3_EPCL_KEY6_FIELD_NAME_BUILD(24 , 24 ,Tag1_Exist                )" }
    ,{25 , 36 ,  GT_TRUE,      "LION3_EPCL_KEY6_FIELD_NAME_BUILD(25 , 36 ,Source_ID_11_0            )" }
    ,{37 , 48 ,  GT_TRUE,      "LION3_EPCL_KEY6_FIELD_NAME_BUILD(37 , 48 ,VID0                      )" }
    ,{49 , 50 ,  GT_TRUE,      "LION3_EPCL_KEY6_FIELD_NAME_BUILD(49 , 50 ,Src_Port_7_6              )" }
    ,{51 , 57 ,  GT_TRUE,      "LION3_EPCL_KEY6_FIELD_NAME_BUILD(51 , 57 ,Data_Pkt_OR_CtrlToCPU_Src_Trunk_ID_6_0 )" }
    ,{51 , 57 ,  GT_TRUE,      "LION3_EPCL_KEY6_FIELD_NAME_BUILD(51 , 57 ,Data_Pkt_Src_Dev_4_0                   )" }
    ,{51 , 57 ,  GT_TRUE,      "LION3_EPCL_KEY6_FIELD_NAME_BUILD(51 , 57 ,Ctrl_To_CPU_Src_Trg_Src_Dev_4_0        )" }
    ,{51 , 57 ,  GT_TRUE,      "LION3_EPCL_KEY6_FIELD_NAME_BUILD(51 , 57 ,From_CPU_Egress_Filter_En_Src_Dev_4_0  )" }
    ,{51 , 57 ,  GT_TRUE,      "LION3_EPCL_KEY6_FIELD_NAME_BUILD(51 , 57 ,To_Analyzer_Rx_Sniff_Src_Dev_4_0       )" }
    ,{58 , 59 ,  GT_TRUE,      "LION3_EPCL_KEY6_FIELD_NAME_BUILD(58 , 59 ,Source_Dev_6_5            )" }
    ,{60 , 60 ,  GT_TRUE,      "LION3_EPCL_KEY6_FIELD_NAME_BUILD(60 , 60 ,Src_Is_Trunk              )" }
    ,{61 ,108 ,  GT_TRUE,      "LION3_EPCL_KEY6_FIELD_NAME_BUILD(61 ,108 ,MAC_SA                    )" }
    ,{109,156 ,  GT_TRUE,      "LION3_EPCL_KEY6_FIELD_NAME_BUILD(109,156 ,MAC_DA                    )" }
    ,{157,157 ,  GT_TRUE,      "LION3_EPCL_KEY6_FIELD_NAME_BUILD(157,157 ,Is_L2_Valid               )" }
    ,{158,158 ,  GT_TRUE,      "LION3_EPCL_KEY6_FIELD_NAME_BUILD(158,158 ,Is_IP                     )" }
    ,{159,166 ,  GT_TRUE,      "LION3_EPCL_KEY6_FIELD_NAME_BUILD(159,166 ,Ip_Protocol               )" }
    ,{167,172 ,  GT_TRUE,      "LION3_EPCL_KEY6_FIELD_NAME_BUILD(167,172 ,Packet_DSCP               )" }
    ,{173,174 ,  GT_TRUE,      "LION3_EPCL_KEY6_FIELD_NAME_BUILD(173,174 ,TOS_1_0                   )" }
    ,{175,182 ,  GT_TRUE,      "LION3_EPCL_KEY6_FIELD_NAME_BUILD(175,182 ,TTL                       )" }
    ,{183,183 ,  GT_TRUE,      "LION3_EPCL_KEY6_FIELD_NAME_BUILD(183,183 ,Is_ARP                    )" }
    ,{184,191 ,  GT_TRUE,      "LION3_EPCL_KEY6_FIELD_NAME_BUILD(184,191 ,SIP_7_0                   )" }
    ,{208,231 ,  GT_TRUE,      "LION3_EPCL_KEY6_FIELD_NAME_BUILD(208,231 ,SIP_31_8                  )" }
    ,{232,263 ,  GT_TRUE,      "LION3_EPCL_KEY6_FIELD_NAME_BUILD(232,263 ,DIP_31_0                  )" }
    ,{264,271 ,  GT_TRUE,      "LION3_EPCL_KEY6_FIELD_NAME_BUILD(264,271 ,TCP_UDP_Port_Comparators  )" }
    ,{272,279 ,  GT_TRUE,      "LION3_EPCL_KEY6_FIELD_NAME_BUILD(272,279 ,L4_Header_Byte_13         )" }
    ,{280,287 ,  GT_TRUE,      "LION3_EPCL_KEY6_FIELD_NAME_BUILD(280,287 ,L4_Header_Byte_1          )" }
    ,{288,295 ,  GT_TRUE,      "LION3_EPCL_KEY6_FIELD_NAME_BUILD(288,295 ,L4_Header_Byte_0          )" }
    ,{296,303 ,  GT_TRUE,      "LION3_EPCL_KEY6_FIELD_NAME_BUILD(296,303 ,L4_Header_Byte_3_OR_5     )" }
    ,{304,311 ,  GT_TRUE,      "LION3_EPCL_KEY6_FIELD_NAME_BUILD(304,311 ,L4_Header_Byte_2_OR_4     )" }
    ,{312,312 ,  GT_TRUE,      "LION3_EPCL_KEY6_FIELD_NAME_BUILD(312,312 ,Is_L4_Valid               )" }
    ,{313,315 ,  GT_TRUE,      "LION3_EPCL_KEY6_FIELD_NAME_BUILD(313,315 ,From_CPU_TC               )" }
    ,{316,317 ,  GT_TRUE,      "LION3_EPCL_KEY6_FIELD_NAME_BUILD(316,317 ,From_CPU_DP               )" }
    ,{318,319 ,  GT_TRUE,      "LION3_EPCL_KEY6_FIELD_NAME_BUILD(318,319 ,Packet_Type               )" }
    ,{320,320 ,  GT_TRUE,      "LION3_EPCL_KEY6_FIELD_NAME_BUILD(320,320 ,Src_Trg_OR_Tx_Mirror      )" }
    ,{321,323 ,  GT_TRUE,      "LION3_EPCL_KEY6_FIELD_NAME_BUILD(321,323 ,Assigned_UP               )" }
    ,{324,329 ,  GT_TRUE,      "LION3_EPCL_KEY6_FIELD_NAME_BUILD(324,329 ,Trg_Port_5_0              )" }
    ,{330,330 ,  GT_TRUE,      "LION3_EPCL_KEY6_FIELD_NAME_BUILD(330,330 ,Rx_Sniff                  )" }
    ,{331,331 ,  GT_TRUE,      "LION3_EPCL_KEY6_FIELD_NAME_BUILD(331,331 ,Is_Routed                 )" }
    ,{332,332 ,  GT_TRUE,      "LION3_EPCL_KEY6_FIELD_NAME_BUILD(332,332 ,Is_IPv6                   )" }

    /* key6 only fields */
    ,{333,333 ,  GT_TRUE,      "LION3_EPCL_KEY6_FIELD_NAME_BUILD(333,333 ,Ipv4_Options              )" }
    ,{334,334 ,  GT_TRUE,      "LION3_EPCL_KEY6_FIELD_NAME_BUILD(334,334 ,is_VIDX                   )" }
    ,{335,346 ,  GT_TRUE,      "LION3_EPCL_KEY6_FIELD_NAME_BUILD(335,346 ,Tag1_VID                  )" }
    ,{347,347 ,  GT_TRUE,      "LION3_EPCL_KEY6_FIELD_NAME_BUILD(347,347 ,Tag1_CFI                  )" }
    ,{348,350 ,  GT_TRUE,      "LION3_EPCL_KEY6_FIELD_NAME_BUILD(348,350 ,UP1                       )" }
    ,{351,355 ,  GT_TRUE,      "LION3_EPCL_KEY6_FIELD_NAME_BUILD(351,355 ,Source_Dev_OR_Trunk_11_7  )" }
    ,{356,357 ,  GT_TRUE,      "LION3_EPCL_KEY6_FIELD_NAME_BUILD(356,357 ,Trg_Port_7_6              )" }
    ,{358,358 ,  GT_TRUE,      "LION3_EPCL_KEY6_FIELD_NAME_BUILD(358,358 ,User_Defined_Valid        )" }

    /* key7 only fields */
    ,{333,333 ,  GT_TRUE,      "LION3_EPCL_KEY7_FIELD_NAME_BUILD(333,333 ,IPv6_EH_exist             )" }
    ,{334,334 ,  GT_TRUE,      "LION3_EPCL_KEY7_FIELD_NAME_BUILD(334,334 ,IPv6_Is_ND                )" }
    ,{335,335 ,  GT_TRUE,      "LION3_EPCL_KEY7_FIELD_NAME_BUILD(335,335 ,Is_IPv6_EH_Hop_By_Hop     )" }
    ,{336,367 ,  GT_TRUE,      "LION3_EPCL_KEY7_FIELD_NAME_BUILD(336,367 ,SIP_63_32                 )" }
    ,{368,399 ,  GT_TRUE,      "LION3_EPCL_KEY7_FIELD_NAME_BUILD(368,399 ,SIP_95_64                 )" }
    ,{416,447 ,  GT_TRUE,      "LION3_EPCL_KEY7_FIELD_NAME_BUILD(416,447 ,SIP_127_96                )" }
    ,{448,479 ,  GT_TRUE,      "LION3_EPCL_KEY7_FIELD_NAME_BUILD(448,479 ,DIP_63_32                 )" }
    ,{480,511 ,  GT_TRUE,      "LION3_EPCL_KEY7_FIELD_NAME_BUILD(480,511 ,DIP_95_64                 )" }
    ,{512,543 ,  GT_TRUE,      "LION3_EPCL_KEY7_FIELD_NAME_BUILD(512,543 ,DIP_127_96                )" }
    ,{544,544 ,  GT_TRUE,      "LION3_EPCL_KEY7_FIELD_NAME_BUILD(544,544 ,is_VIDX                   )" }
    ,{545,556 ,  GT_TRUE,      "LION3_EPCL_KEY7_FIELD_NAME_BUILD(545,556 ,Tag1_VID                  )" }
    ,{557,557 ,  GT_TRUE,      "LION3_EPCL_KEY7_FIELD_NAME_BUILD(557,557 ,Tag1_CFI                  )" }
    ,{558,560 ,  GT_TRUE,      "LION3_EPCL_KEY7_FIELD_NAME_BUILD(558,560 ,UP1                       )" }
    ,{561,565 ,  GT_TRUE,      "LION3_EPCL_KEY7_FIELD_NAME_BUILD(561,565 ,Source_Dev_OR_Trunk_11_7  )" }
    ,{566,578 ,  GT_TRUE,      "LION3_EPCL_KEY7_FIELD_NAME_BUILD(566,578 ,SRC_ePort_OR_Trunk_ID     )" }
    ,{583,595 ,  GT_TRUE,      "LION3_EPCL_KEY7_FIELD_NAME_BUILD(583,595 ,TRG_ePort                 )" }
    ,{600,600 ,  GT_TRUE,      "LION3_EPCL_KEY7_FIELD_NAME_BUILD(600,600 ,eVLAN_12                  )" }
    ,{604,605 ,  GT_TRUE,      "LION3_EPCL_KEY7_FIELD_NAME_BUILD(604,605 ,Trg_Port_7_6              )" }
    ,{606,606 ,  GT_TRUE,      "LION3_EPCL_KEY7_FIELD_NAME_BUILD(606,606 ,User_Defined_Valid        )" }

    /* EPCL : holds the info about the extra 10 bytes of the 60B UBD key fields - lion3 format */
    ,{400,400 ,  GT_TRUE,      "LION3_EPCL_KEY_60B_EXTRA_10B_FIELD_NAME_BUILD(400,  400 , UDB_Valid                  )" }
    ,{401,410 ,  GT_TRUE,      "LION3_EPCL_KEY_60B_EXTRA_10B_FIELD_NAME_BUILD(401,  410 , PCL_ID                     )" }
    ,{411,423 ,  GT_TRUE,      "LION3_EPCL_KEY_60B_EXTRA_10B_FIELD_NAME_BUILD(411,  423 , eVLAN                      )" }
    ,{424,436 ,  GT_TRUE,      "LION3_EPCL_KEY_60B_EXTRA_10B_FIELD_NAME_BUILD(424,  436 , Source_ePort_or_Trunk_ID   )" }
    ,{437,449 ,  GT_TRUE,      "LION3_EPCL_KEY_60B_EXTRA_10B_FIELD_NAME_BUILD(437,  449 , Trg_ePort                  )" }
    ,{450,459 ,  GT_TRUE,      "LION3_EPCL_KEY_60B_EXTRA_10B_FIELD_NAME_BUILD(450,  459 , Src_Dev                    )" }
    ,{460,469 ,  GT_TRUE,      "LION3_EPCL_KEY_60B_EXTRA_10B_FIELD_NAME_BUILD(460,  469 , Trg_Dev                    )" }
    ,{470,477 ,  GT_TRUE,      "LION3_EPCL_KEY_60B_EXTRA_10B_FIELD_NAME_BUILD(470,  477 , Local_Dev_Trg_Phy_Port     )" }
    ,{470,470 ,  GT_TRUE,      "LION3_EPCL_KEY_60B_EXTRA_10B_FIELD_NAME_BUILD(470,  470 , Orig_Src_Is_Trunk          )" }
    ,{478,479 ,  GT_TRUE,      "LION3_EPCL_KEY_60B_EXTRA_10B_FIELD_NAME_BUILD(478,  479 , Reserved                   )" }


};

typedef struct{
    GT_U32  Data_Pkt_OR_CtrlToCPU_Src_Trunk_ID_6_0 ;
    GT_U32  Data_Pkt_Src_Dev_4_0                   ;
    GT_U32  Ctrl_To_CPU_Src_Trg_Src_Dev_4_0        ;
    GT_U32  From_CPU_Egress_Filter_En_Src_Dev_4_0  ;
    GT_U32  To_Analyzer_Rx_Sniff_Src_Dev_4_0       ;
    GT_U32  Data_Pkt_OR_Ctrl_To_CPU_Src_Is_Trunk   ;

    GT_U32  Data_Pkt_Is_Unknown                    ;
    GT_U32  Ctrl_To_CPU_CPU_CODE_7                 ;
    GT_U32  From_CPU_Is_Unknown                    ;

    GT_U32  Data_Pkt_QoS_Profile_6_0_OR_port_list_13_8;
    GT_U32  Ctrl_To_CPU_CPU_CODE_6_0               ;
    GT_U32  Ctrl_From_CPU_TC_2_0_DP_1_0            ;
}EPCL_MUXED_TRAFFIC_TYPE_INFO;

static EPCL_MUXED_TRAFFIC_TYPE_INFO muxedTrafficTypeInfo[] =
{
    /* keys 0..2 */
    {
         LION3_EPCL_KEY_FIELD_NAME_BUILD(74 , 80 ,Data_Pkt_OR_CtrlToCPU_Src_Trunk_ID_6_0     )
        ,LION3_EPCL_KEY_FIELD_NAME_BUILD(74 , 80 ,Data_Pkt_Src_Dev_4_0                       )
        ,LION3_EPCL_KEY_FIELD_NAME_BUILD(74 , 80 ,Ctrl_To_CPU_Src_Trg_Src_Dev_4_0            )
        ,LION3_EPCL_KEY_FIELD_NAME_BUILD(74 , 80 ,From_CPU_Egress_Filter_En_Src_Dev_4_0      )
        ,LION3_EPCL_KEY_FIELD_NAME_BUILD(74 , 80 ,To_Analyzer_Rx_Sniff_Src_Dev_4_0           )
        ,LION3_EPCL_KEY_FIELD_NAME_BUILD(81 , 81 ,Data_Pkt_OR_Ctrl_To_CPU_Src_Is_Trunk       )

        ,LION3_EPCL_KEY_FIELD_NAME_BUILD(88 , 88 ,Data_Pkt_Is_Unknown                        )
        ,LION3_EPCL_KEY_FIELD_NAME_BUILD(88 , 88 ,Ctrl_To_CPU_CPU_CODE_7                     )
        ,LION3_EPCL_KEY_FIELD_NAME_BUILD(88 , 88 ,From_CPU_Is_Unknown                        )


        ,LION3_EPCL_KEY_FIELD_NAME_BUILD(32 , 38 ,Data_Pkt_QoS_Profile_6_0_OR_port_list_13_8 )
        ,LION3_EPCL_KEY_FIELD_NAME_BUILD(32 , 38 ,Ctrl_To_CPU_CPU_CODE_6_0                   )
        ,LION3_EPCL_KEY_FIELD_NAME_BUILD(32 , 38 ,Ctrl_From_CPU_TC_2_0_DP_1_0                )

    }
    ,
    /* keys 3..5 */
    {
         LION3_EPCL_KEY3_FIELD_NAME_BUILD(371,377 ,Data_Pkt_OR_CtrlToCPU_Src_Trunk_ID_6_0     )
        ,LION3_EPCL_KEY3_FIELD_NAME_BUILD(371,377 ,Data_Pkt_Src_Dev_4_0                       )
        ,LION3_EPCL_KEY3_FIELD_NAME_BUILD(371,377 ,Ctrl_To_CPU_Src_Trg_Src_Dev_4_0            )
        ,LION3_EPCL_KEY3_FIELD_NAME_BUILD(371,377 ,From_CPU_Egress_Filter_En_Src_Dev_4_0      )
        ,LION3_EPCL_KEY3_FIELD_NAME_BUILD(371,377 ,To_Analyzer_Rx_Sniff_Src_Dev_4_0           )
        ,LION3_EPCL_KEY3_FIELD_NAME_BUILD(378,378 ,Data_Pkt_OR_Ctrl_To_CPU_Src_Is_Trunk       )

        ,LION3_EPCL_KEY3_FIELD_NAME_BUILD(385,385 ,Data_Pkt_Is_Unknown                        )
        ,LION3_EPCL_KEY3_FIELD_NAME_BUILD(385,385 ,Ctrl_To_CPU_CPU_CODE_7                     )
        ,LION3_EPCL_KEY3_FIELD_NAME_BUILD(385,385 ,From_CPU_Is_Unknown                        )

        ,LION3_EPCL_KEY_FIELD_NAME_BUILD(32 , 38 ,Data_Pkt_QoS_Profile_6_0_OR_port_list_13_8 )
        ,LION3_EPCL_KEY_FIELD_NAME_BUILD(32 , 38 ,Ctrl_To_CPU_CPU_CODE_6_0                   )
        ,LION3_EPCL_KEY_FIELD_NAME_BUILD(32 , 38 ,Ctrl_From_CPU_TC_2_0_DP_1_0                )
    }
    ,
    /* keys 6..7 */
    {
         LION3_EPCL_KEY6_FIELD_NAME_BUILD( 51, 57 ,Data_Pkt_OR_CtrlToCPU_Src_Trunk_ID_6_0     )
        ,LION3_EPCL_KEY6_FIELD_NAME_BUILD( 51, 57 ,Data_Pkt_Src_Dev_4_0                       )
        ,LION3_EPCL_KEY6_FIELD_NAME_BUILD( 51, 57 ,Ctrl_To_CPU_Src_Trg_Src_Dev_4_0            )
        ,LION3_EPCL_KEY6_FIELD_NAME_BUILD( 51, 57 ,From_CPU_Egress_Filter_En_Src_Dev_4_0      )
        ,LION3_EPCL_KEY6_FIELD_NAME_BUILD( 51, 57 ,To_Analyzer_Rx_Sniff_Src_Dev_4_0           )
        ,SMAIN_NOT_VALID_CNS/*exists for all types*/

        ,SMAIN_NOT_VALID_CNS/*not exists*/
        ,SMAIN_NOT_VALID_CNS/*not exists*/
        ,SMAIN_NOT_VALID_CNS/*not exists*/

        ,LION3_EPCL_KEY6_FIELD_NAME_BUILD(17 , 23 ,Data_Pkt_QoS_Profile_6_0                   )
        ,LION3_EPCL_KEY6_FIELD_NAME_BUILD(17 , 23 ,Ctrl_To_CPU_CPU_CODE_6_0                   )
        ,SMAIN_NOT_VALID_CNS/*not exists*/
    }
};

/*Lion3 : udbs in EPCL key 0*/
static UDB_INFO_STC lion3EpclKey0UdbArr[] =
{
    { 0, 208},
    { 1, 216},
    { 2, 224},
    { 3, 232},
    {SMAIN_NOT_VALID_CNS , 206   }/*user defined valid*/
};
/*Lion3 : udbs in EPCL key 1*/
static UDB_INFO_STC lion3EpclKey1UdbArr[] =
{
    { 4, 208},
    { 5, 216},
    { 6, 224},
    { 7, 232},
    {SMAIN_NOT_VALID_CNS , 206   }/*user defined valid*/
};

/*Lion3 : udbs in EPCL key 2*/
static UDB_INFO_STC lion3EpclKey2UdbArr[] =
{
    { 8, 208},
    { 9, 216},
    {10, 224},
    {11, 232},
    {SMAIN_NOT_VALID_CNS , 206   }/*user defined valid*/
};

/*Lion3 : udbs in EPCL key 3*/
static UDB_INFO_STC lion3EpclKey3UdbArr[] =
{
    {12 ,416},
    {13 ,424},
    {14 ,432},
    {15 ,440},
    {16 ,448},
    {17 ,456},
    {18 ,464},
    {19 ,472},
    {SMAIN_NOT_VALID_CNS , 414   }/*user defined valid*/
};

/*Lion3 : udbs in EPCL key 4*/
static UDB_INFO_STC lion3EpclKey4UdbArr[] =
{
    {20 ,416},
    {21 ,424},
    {22 ,432},
    {23 ,440},
    {24 ,448},
    {25 ,456},
    {26 ,464},
    {27 ,472},
    {SMAIN_NOT_VALID_CNS , 414   }/*user defined valid*/
};
/*Lion3 : udbs in EPCL key 5*/
static UDB_INFO_STC lion3EpclKey5UdbArr[] =
{
    {28 ,416},
    {29 ,424},
    {30 ,432},
    {31 ,440},
    {32 ,448},
    {33 ,456},
    {34 ,464},
    {35 ,472},
    {SMAIN_NOT_VALID_CNS , 414   }/*user defined valid*/
};
/*Lion3 : udbs in EPCL key 6*/
static UDB_INFO_STC lion3EpclKey6UdbArr[] =
{
    {36, 360},
    {37, 368},
    {38, 376},
    {39, 384},
    {40, 392},
    {41, 400},
    {42, 408},
    {43, 416},
    {44, 424},
    {45, 432},
    {46, 440},
    {47, 448},
    {48, 456},
    {49, 464},
    {0 , 472},
    {SMAIN_NOT_VALID_CNS , 358   }/*user defined valid*/
};

/*Lion3 : udbs in EPCL key 7*/
static UDB_INFO_STC lion3EpclKey7UdbArr[] =
{
    {1, 608},
    {2, 616},
    {3, 624},
    {4, 632},
    {SMAIN_NOT_VALID_CNS , 606   }/*user defined valid*/
};

/* Lion3 : array to hold the UDBs info of all EPCL keys*/
static UDB_INFO_STC*    lion3EpclKeysUdbArr[] =
{
    lion3EpclKey0UdbArr,
    lion3EpclKey1UdbArr,
    lion3EpclKey2UdbArr,
    lion3EpclKey3UdbArr,
    lion3EpclKey4UdbArr,
    lion3EpclKey5UdbArr,
    lion3EpclKey6UdbArr,
    lion3EpclKey7UdbArr
};

/* build UDBs for the given keyIndex . and the <User Defined Valid>*/
static void  lion3PclBuildAllUdbsForKey(
    IN SKERNEL_DEVICE_OBJECT                *devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC      *descrPtr,
    IN SNET_CHT_POLICY_KEY_STC              *pclKeyPtr,
    IN SNET_XCAT_IPCL_KEY_TYPE_ENT          keyType,
    IN DXCH_IPCL_KEY_ENT                    keyIndex
)
{
    GT_STATUS       rc;
    UDB_INFO_STC* udbInfoArr;
    GT_U8         byteValue;
    GT_U32        udbIndex;
    GT_U32        ii;
    GT_U32        udbValid;
    CHT_PCL_KEY_FIELDS_INFO_STC fieldInfo;
    char            udbNameStr[32];

    if(keyIndex >= DXCH_IPCL_KEY_LAST_E)
    {
        skernelFatalError("lion3PclBuildAllUdbsForKey: the keyIndex [%d] in not valid \n",keyIndex);
        return ;
    }

    udbValid = 1;
    udbInfoArr = lion3KeysUdbArr[keyIndex];
    fieldInfo.updateOnSecondCycle = GT_FALSE;
    fieldInfo.debugName = udbNameStr;

    for(ii = 0 ; udbInfoArr[ii].udbIndex != SMAIN_NOT_VALID_CNS ; ii++)
    {
        fieldInfo.startBitInKey = udbInfoArr[ii].startBitInKey;
        fieldInfo.endBitInKey = fieldInfo.startBitInKey + 7;/*8 bit field*/

        udbIndex = udbInfoArr[ii].udbIndex;
        sprintf(udbNameStr,"key[%d] UDB[%d]",keyIndex,udbIndex);

        rc = snetXCatPclUdbKeyValueGet(devObjPtr, descrPtr,
                                       keyType,
                                       udbIndex, &byteValue);
        if(rc == GT_FAIL)
        {
            udbValid = 0;
        }

        snetChtPclSrvKeyFieldBuildByValue(pclKeyPtr, byteValue,  &fieldInfo);
    }

    /* the array hold the last entry for the <User Defined Valid> */
    sprintf(udbNameStr,"key[%d] User Defined Valid",keyIndex);
    fieldInfo.startBitInKey = udbInfoArr[ii].startBitInKey;
    fieldInfo.endBitInKey = fieldInfo.startBitInKey;/*single bit field*/
    snetChtPclSrvKeyFieldBuildByValue(pclKeyPtr, udbValid,  &fieldInfo);

    return ;
}



/*******************************************************************************
*   snetXcatPclTemplateToKeyConvert
*
* DESCRIPTION:
*       Function converts template index (packet type) to value of the UDB key.
*
* INPUTS:
*       devObjPtr - (pointer to) device object.
*       descrPtr     - (pointer to) frame data buffer Id
*
* OUTPUTS:
*       udbKeyPtr - (pointer to) the converted UDB key.
*
* RETURN:
*       None.
*
* COMMENTS:
*
*       In the UDB key the flow sub-template is one of the following:
*           1 = IPv4_TCP
*           2 = IPv4_UDP
*           4 = MPLS
*           8 = IPv4_FRAGMENT
*           16 = IPv4_OTHER
*           32 = ETHERNET_OTHER
*           64 = IPv6 (TCP,UDP,all)
*           128 = UDE0..6
*
*******************************************************************************/
static GT_VOID snetXcatPclTemplateToKeyConvert
(
    IN SKERNEL_DEVICE_OBJECT              *devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC    *descrPtr,
    OUT GT_U32                            *udbKeyPtr
)
{
    DECLARE_FUNC_NAME(snetXcatPclTemplateToKeyConvert);

    static GT_U32   bitIndexArr[12] = {
/*IPV4_TCP_E   */  0,
/*IPV4_UDP_E   */  1,
/*MPLS_E       */  2,
/*IPV4_FRAG_E  */  3,
/*IPV4_OTHER_E */  4,
/*ETH_OTHER_E  */  5,
/*UDE0_E       */  7,
/*IPV6_E       */  6,
/*UDE1_E       */  7,
/*UDE2_E       */  7,
/*UDE3_E       */  7,
/*UDE4_E       */  7
    };
    static GT_U32   lion3BitIndexArr[16] = {
/*IPV4_TCP_E   */  0,
/*IPV4_UDP_E   */  1,
/*MPLS_E       */  2,
/*IPV4_FRAG_E  */  3,
/*IPV4_OTHER_E */  4,
/*ETH_OTHER_E  */  5,
/*IPV6_E       */  6,
/*IPV6_TCP_E   */  6,
/*IPV6_UDP_E   */  6,
/*UDE0_E       */  7,
/*UDE1_E       */  7,
/*UDE2_E       */  7,
/*UDE3_E       */  7,
/*UDE4_E       */  7,
/*UDE5_E       */  7,
/*UDE6_E       */  7
    };

    GT_U32  flowSubTemplateBmp;

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        SIP5_PACKET_CLASSIFICATION_TYPE_ENT pcl_pcktType_sip5;
        /* fix CQ#153167 */
        if(descrPtr->ingressTunnelInfo.origDescrPtr &&
           descrPtr != descrPtr->ingressTunnelInfo.origDescrPtr &&
           descrPtr->tunnelTerminated == 0)
        {
            __LOG(("use Flow sub-template according to 'tunnel header' (and not passenger) \n"));
            pcl_pcktType_sip5 = descrPtr->ingressTunnelInfo.origDescrPtr->tti_pcktType_sip5;
        }
        else
        {
            if(descrPtr->tunnelTerminated)
            {
                __LOG(("Flow sub-template according to passenger header \n"));
            }
            else
            {
                __LOG(("Flow sub-template according to tunnel header \n"));
            }

            pcl_pcktType_sip5 = descrPtr->pcl_pcktType_sip5;
        }

        flowSubTemplateBmp = 1<< lion3BitIndexArr[pcl_pcktType_sip5];
    }
    else
    {
        flowSubTemplateBmp = 1<< bitIndexArr[descrPtr->pcktType];
    }

    __LOG_PARAM(flowSubTemplateBmp);

    *udbKeyPtr = (GT_U8)flowSubTemplateBmp;

}

/*******************************************************************************
*   snetXcatPclKeyPacketTypeGet
*
* DESCRIPTION:
*       get 'packet type' as field in the key
*       0=Other
*       1=ARP
*       2=IPv6overMPLS
*       3=IPv4overMPLS
*       4=MPLS
*       5=Reserved
*       6=IPv6
*       7=IPv4
*
* INPUTS:
*       devObjPtr - (pointer to) device object.
*       descrPtr     - (pointer to) frame data buffer Id
*
* OUTPUTS:
*       keyPacketTypePtr - (pointer to) the packet type field for the key.
*       useTunnelInfo - (sip5 only) the packet type of the tunnel or the passenger
* RETURN:
*       None.
*
* COMMENTS:
*
*******************************************************************************/
static GT_VOID snetXcatPclKeyPacketTypeGet
(
    IN SKERNEL_DEVICE_OBJECT              *devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC    *descrPtr,
    OUT GT_U32                            *keyPacketTypePtr,
    IN GT_BOOL                             useTunnelInfo
)
{
    GT_BIT arp,mpls,isIp,isIPv4;

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        if(useTunnelInfo == GT_TRUE &&
            descrPtr->tunnelTerminated == 0/*CQ#153167*/ &&
            descrPtr->ingressTunnelInfo.transitType == SNET_INGRESS_TUNNEL_TRANSIT_TYPE__NONE__E/*CQ#00153167*/)
        {
            /* use info from the tunnel header */
            arp = descrPtr->origInfoBeforeTunnelTermination.arp;
            mpls = descrPtr->origInfoBeforeTunnelTermination.mpls;
            isIp = descrPtr->origInfoBeforeTunnelTermination.isIp;
            isIPv4 = descrPtr->origInfoBeforeTunnelTermination.isIPv4;
        }
        else
        {
            /* for TT or even for non-TT that do inner parsing we take info from the inner packet */

            SKERNEL_CHT_DESC_INNER_FRAME_FIELD_GET_MAC(descrPtr,arp);
            SKERNEL_CHT_DESC_INNER_FRAME_FIELD_GET_MAC(descrPtr,mpls);
            SKERNEL_CHT_DESC_INNER_FRAME_FIELD_GET_MAC(descrPtr,isIp);
            SKERNEL_CHT_DESC_INNER_FRAME_FIELD_GET_MAC(descrPtr,isIPv4);
        }

        *keyPacketTypePtr = 0;
        if(arp)
        {
            *keyPacketTypePtr = 1;
        }
        else
        if(mpls)
        {
            *keyPacketTypePtr = 4;
        }
        else
        if(descrPtr->tti_pcktType_sip5 == SIP5_PACKET_CLASSIFICATION_TYPE_MPLS_E &&
           descrPtr->tunnelTerminated == 0/*CQ#153167*/)
        {
            /* tunnel is MPLS but passenger is NOT */
            if(descrPtr->isIp && descrPtr->isIPv4 == 0)
            {
                /*ipv6 over MPLS*/
                *keyPacketTypePtr = 2;
            }
            else
            if(descrPtr->isIp && descrPtr->isIPv4 == 1)
            {
                /*ipv4 over MPLS*/
                *keyPacketTypePtr = 3;
            }
        }
        else
        {
            if(isIp && isIPv4 == 0)
            {
                /*ipv6 */
                *keyPacketTypePtr = 6;
            }
            else
            if(isIp && isIPv4 == 1)
            {
                /*ipv4 */
                *keyPacketTypePtr = 7;
            }
        }
    }
    else
    {
/*  don't know to get those in current implementation
*       2=IPv6overMPLS
*       3=IPv4overMPLS
*/

        *keyPacketTypePtr = (descrPtr->arp) ? 1 :
               (descrPtr->mpls) ? 4 :
               (descrPtr->isIp && descrPtr->isIPv4 == 0) ? 6 :
               (descrPtr->isIp && descrPtr->isIPv4) ? 7  : 0;
    }
}


/*******************************************************************************
*   snetLion3IpclUdbMetaDataBuild
*
* DESCRIPTION:
*       build the metadata byte for the IPCL from the packet descriptor fields
*               (relevant for SIP5 only)
*
* INPUTS:
*       devObjPtr - (pointer to) device object.
*       descrPtr  - (pointer to) frame data buffer Id
*       pclExtraDataPtr - (pointer to) extra data needed for the IPCL engine.
*
* OUTPUTS:
*       descrPtr->ipclMetadataInfo[] - the place to hold the info of the metadata
*
* RETURN:
*       None
*
* COMMENTS:
*
*******************************************************************************/
static void snetLion3IpclUdbMetaDataBuild
(
    IN  SKERNEL_DEVICE_OBJECT           *devObjPtr,
    INOUT  SKERNEL_FRAME_CHEETAH_DESCR_STC *descrPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC    *pclExtraDataPtr
)
{
    DECLARE_FUNC_NAME(snetLion3IpclUdbMetaDataBuild);

    GT_STATUS   rc;
    GT_U32  portListBmp;
    GT_U32 l2Encapsulation,outer_is_llc_non_snap,inner_is_llc_non_snap;
    GT_U32 ip_fragmented,mac_sa_arp_sa_mismatch;
    GT_U32 isIp,isIPv4,isIPv6,ipHeaderError,ipm,ip_legal_or_fcoe_legal;
    GT_U8  tcporudp_port_comparators;
    GT_U32  rx_sniff_or_src_trg,src_trg_eport,src_trg_dev,src_trg_phy_port;
    GT_U32 passengerKeyPacketType,tunnelKeyPacketType; /* passenger , Tunnel Packet type for the key */
    GT_U32 applicableFlowSubTemplate;
    GT_U32 numOfTagsToPop;

    descrPtr->ipclMetadataReady = 1;

    __LOG(("build IPCL metadata fields so the key can use fields from it \n"));

    /* get the portListBmp */
    snetXcat2IpclKeyPortsBmpBuild(devObjPtr, descrPtr, NULL,&portListBmp);

    l2Encapsulation =
        CONVERT_DESCRIPTOR_L2_ENCAPS_TO_KEY_L2_ENCAPSULATION_MAC(descrPtr->l2Encaps);

    if(descrPtr->tunnelTerminated)
    {
        outer_is_llc_non_snap = (descrPtr->origInfoBeforeTunnelTermination.l2Encaps == SKERNEL_ETHERNET_II_E) ? 1 : 0;
        inner_is_llc_non_snap = (descrPtr->l2Encaps == SKERNEL_ETHERNET_II_E) ? 1 : 0;
    }
    else
    {
        outer_is_llc_non_snap = (descrPtr->l2Encaps == SKERNEL_ETHERNET_II_E) ? 1 : 0;
        inner_is_llc_non_snap = outer_is_llc_non_snap;
    }

    if(descrPtr->arp && descrPtr->l2Encaps == SKERNEL_ETHERNET_II_E && descrPtr->l2Valid &&
       ((descrPtr->macDaPtr[0+0] != descrPtr->l3StartOffsetPtr[8+0]) ||
        (descrPtr->macDaPtr[0+1] != descrPtr->l3StartOffsetPtr[8+1]) ||
        (descrPtr->macDaPtr[0+2] != descrPtr->l3StartOffsetPtr[8+2]) ||
        (descrPtr->macDaPtr[0+3] != descrPtr->l3StartOffsetPtr[8+3]) ||
        (descrPtr->macDaPtr[0+4] != descrPtr->l3StartOffsetPtr[8+4]) ||
        (descrPtr->macDaPtr[0+5] != descrPtr->l3StartOffsetPtr[8+5]) ))
    {
        mac_sa_arp_sa_mismatch = 1;
    }
    else
    {
        mac_sa_arp_sa_mismatch = 0;
    }

    /* even for non-TT that do inner parsing we take info from the inner packet */
    SKERNEL_CHT_DESC_INNER_FRAME_FIELD_GET_MAC(descrPtr,isIp);
    SKERNEL_CHT_DESC_INNER_FRAME_FIELD_GET_MAC(descrPtr,isIPv4);
    isIPv6  = isIp && (isIPv4==0);
    SKERNEL_CHT_DESC_INNER_FRAME_FIELD_GET_MAC(descrPtr,ipHeaderError);
    SKERNEL_CHT_DESC_INNER_FRAME_FIELD_GET_MAC(descrPtr,ipm);

    ip_legal_or_fcoe_legal = descrPtr->isFcoe ?
        descrPtr->fcoeInfo.fcoeLegal :
        (ipHeaderError == 0);/*Ip legal*/

    VALUE_IPV4_FRAGMENT_MAC(pclExtraDataPtr,ip_fragmented);

    rc = snetChtIPclTcpUdpPortRangeCompareGet(devObjPtr, descrPtr,
                                              &tcporudp_port_comparators);
    if(rc != GT_OK)
    {
        tcporudp_port_comparators = 0;
    }

    if(descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_TO_TRG_SNIFFER_E)
    {
        rx_sniff_or_src_trg = descrPtr->rxSniff;
        src_trg_eport = descrPtr->eArchExtInfo.srcTrgEPort;
        src_trg_dev = descrPtr->srcTrgDev;
        src_trg_phy_port = descrPtr->srcTrgPhysicalPort;
    }
    else
    if(descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_FROM_CSCD_TO_CPU_E)
    {
        rx_sniff_or_src_trg = descrPtr->srcTrg;
        src_trg_eport = (descrPtr->srcTrg == 0/*src*/ && descrPtr->origIsTrunk) ?
            descrPtr->origSrcEPortOrTrnk :
            descrPtr->eArchExtInfo.srcTrgEPort;
        src_trg_dev = descrPtr->srcTrgDev;
        src_trg_phy_port = descrPtr->srcTrgPhysicalPort;
    }
    else
    {
        rx_sniff_or_src_trg = 0;
        src_trg_eport = 0;
        src_trg_dev = 0;
        src_trg_phy_port = 0;
    }

    /* build value for the metadata 'numOfTagsToPop' */
    numOfTagsToPop = (descrPtr->numOfBytesToPop == 6) ? 3 :
                     (descrPtr->numOfBytesToPop / 4);


    /* get Packet type of the passenger for the key */
    snetXcatPclKeyPacketTypeGet(devObjPtr, descrPtr, &passengerKeyPacketType,GT_FALSE/*passenger*/);
    __LOG_PARAM(passengerKeyPacketType);
    /* get Packet type of the tunnel for the key */
    snetXcatPclKeyPacketTypeGet(devObjPtr, descrPtr, &tunnelKeyPacketType,GT_TRUE/*tunnel*/);
    __LOG_PARAM(tunnelKeyPacketType);

    /* Applicable Flow Sub Template */
    snetXcatPclTemplateToKeyConvert(devObjPtr, descrPtr, &applicableFlowSubTemplate);

    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_PORT_LIST_SRC_E                      ,portListBmp);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_LOCAL_DEV_SRC_TRUNK_ID_E             ,descrPtr->localDevSrcTrunkId);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_LOCAL_DEV_SRC_EPORT_E                ,descrPtr->eArchExtInfo.localDevSrcEPort);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_SRC_DEV_IS_OWN_E                     ,descrPtr->srcDevIsOwn);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_RESERVED_1_E                         ,0);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_ORIG_SRC_DEV_E                       ,descrPtr->srcDev);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_ORIG_SRC_IS_TRUNK_E                  ,descrPtr->origIsTrunk);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_ORIG_SRC_EPORT_OR_TRUNK_ID_E         ,descrPtr->origSrcEPortOrTrnk);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_LOCAL_DEV_SRC_PORT_E                 ,descrPtr->localDevSrcPort);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_USE_VIDX_E                           ,descrPtr->useVidx);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_EVIDX_E                              ,descrPtr->eVidx);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_INGRESS_UDB_PACKET_TYPE_E            ,descrPtr->pcl_pcktType_sip5);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_IP_LEGAL_OR_FCOE_LEGAL_DUP_E         ,ip_legal_or_fcoe_legal);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_L2_VALID_DUP_E                       ,descrPtr->l2Valid);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_IS_ARP_DUP_E                         ,descrPtr->arp);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_TRG_DEV_E                            ,descrPtr->trgDev);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_TRG_IS_TRUNK_E                       ,descrPtr->targetIsTrunk);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_TRG_EPORT_OR_TRG_TRUNK_ID_E          ,(descrPtr->targetIsTrunk ? descrPtr->trgTrunkId : descrPtr->trgEPort));
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_TRG_PHY_PORT_E                       ,descrPtr->eArchExtInfo.trgPhyPort);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_IS_TRG_PHY_PORT_VALID_E              ,descrPtr->eArchExtInfo.isTrgPhyPortValid);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_EGRESS_UDB_PACKET_TYPE_E             ,descrPtr->epcl_pcktType_sip5);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_RESERVED_2_E                         ,0);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_SRC_ID_E                             ,descrPtr->sstId);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_EGRESS_FILTER_REGISTERED_E           ,descrPtr->egressFilterRegistered);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_PACKET_IS_LOOPED_E                   ,descrPtr->pktIsLooped);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_DROP_ON_SOURCE_E                     ,0);/*DropOnSource not implemented in WM*/
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_RESERVED_3_E                         ,0);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_PHY_SRC_MC_FILTER_EN_E               ,descrPtr->eArchExtInfo.phySrcMcFilterEn);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_ORIG_SRC_PHY_IS_TRUNK_E              ,descrPtr->origIsTrunk);/* ORIG_SRC_PHY not implemented yet */
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_ORIG_SRC_PHY_PORT_OR_TRUNK_ID_E      ,descrPtr->origSrcEPortOrTrnk);/* ORIG_SRC_PHY not implemented yet */
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_RESERVED_4_E                         ,0);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_L2_VALID_E                           ,descrPtr->l2Valid);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_L2_ENCAPSULATION_E                   ,l2Encapsulation);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_BYPASS_BRIDGE_E                      ,descrPtr->bypassBridge);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_MAC_DA_TYPE_E                        ,descrPtr->macDaType);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_OUTER_IS_LLC_NON_SNAP_E              ,outer_is_llc_non_snap);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_INNER_IS_LLC_NON_SNAP_E              ,inner_is_llc_non_snap);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_ETHERTYPEORDSAPSSAP_E                ,descrPtr->etherTypeOrSsapDsap);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_TAG0_SRC_TAGGED_E                    ,TAG0_EXIST_MAC(descrPtr));
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_TAG1_SRC_TAGGED_E                    ,TAG1_EXIST_MAC(descrPtr));
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_SRC_TAG0_IS_OUTER_TAG_E              ,descrPtr->tag0IsOuterTag);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_OUTER_SRC_TAG_E                      ,descrPtr->origSrcTagState);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_TAG1_LOCAL_DEV_SRC_TAGGED_E          ,descrPtr->tag1LocalDevSrcTagged);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_NESTED_VLAN_EN_E                     ,descrPtr->nestedVlanAccessPort);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_TAG0_PRIO_TAGGED_E                   ,descrPtr->srcPriorityTagged );
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_TAG1_PRIO_TAGGED_E                   ,(descrPtr->tag1LocalDevSrcTagged && descrPtr->vid1 == 0) ? 1 : 0);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_ORIG_VID_E                           ,descrPtr->vid0Or1AfterTti);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_OVERRIDE_EVLAN_WITH_ORIGVID_E        ,descrPtr->overrideVid0WithOrigVid);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_TRG_TAGGED_E                         ,descrPtr->trgTagged);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_EVLAN_PRECEDENCE_E                   ,descrPtr->preserveVid);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_RESERVED_5_E                         ,0);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_TAG0_TPID_INDEX_E                    ,descrPtr->tpidIndex[SNET_CHT_TAG_0_INDEX_CNS]);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_TAG1_TPID_INDEX_E                    ,descrPtr->tpidIndex[SNET_CHT_TAG_1_INDEX_CNS]);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_NUM_OF_TAG_WORDS_TO_POP_E            ,numOfTagsToPop);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_IPX_PROTOCOL_E                       ,descrPtr->ipProt);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_IPV4_SIP_OR_ARP_SIP_OR_FCOE_S_ID_E   ,descrPtr->sip[0]);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_IPV4_DIP_OR_ARP_DIP_OR_FCOE_D_ID_E   ,descrPtr->dip[0]);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_POLICY_RTT_INDEX_E                   ,descrPtr->ttRouterLTT);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_L3_OFFSET_INVALID_E                  ,descrPtr->l3NotValid);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_IS_IP_E                              ,isIp);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_IS_IPV4_E                            ,isIPv4);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_IS_IPV6_E                            ,isIPv6);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_IS_FCOE_E                            ,descrPtr->isFcoe);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_IS_ARP_E                             ,descrPtr->arp);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_IP_LEGAL_OR_FCOE_LEGAL_E             ,ip_legal_or_fcoe_legal);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_IPM_E                                ,ipm);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_IP_HEADER_INFO_E                     ,pclExtraDataPtr->ipv4HeaderInfo);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_IP_FRAGMENTED_E                      ,ip_fragmented);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_FRAGMENTED_E                         ,pclExtraDataPtr->isIpv4Fragment);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_ROUTED_E                             ,descrPtr->routed);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_DO_ROUTE_HA_E                        ,descrPtr->doRouterHa);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_MAC_SA_ARP_SA_MISMATCH_E             ,mac_sa_arp_sa_mismatch);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_IPX_HEADER_LENGTH_E                  ,descrPtr->ipxHeaderLength);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_RESERVED_6_E                         ,0);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_IS_IP_1_E                            ,isIp);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_IS_IPV6_1_E                          ,isIPv6);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_IS_ND_E                              ,descrPtr->solicitationMcastMsg);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_IS_IPV6_LINK_LOCAL_E                 ,descrPtr->ipXMcLinkLocalProt);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_IS_IPV6_MLD_E                        ,descrPtr->isIpv6Mld);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_IPV6_HBH_EXT_E                       ,pclExtraDataPtr->isIpV6EhHopByHop);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_IPV6_EH_E                            ,pclExtraDataPtr->isIpV6EhExists);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_SOLICITATION_MULTICAST_MESSAGE_E     ,descrPtr->solicitationMcastMsg);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_IS_IP_2_E                            ,isIp);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_IS_IPV6_2_E                          ,isIPv6);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_IPV6_FLOW_LABEL_E                    ,descrPtr->flowLabel);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_RESERVED_7_E                         ,0);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_TCP_UDP_DEST_PORT_E                  ,descrPtr->l4DstPort);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_TCPORUDP_PORT_COMPARATORS_E          ,tcporudp_port_comparators);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_L4_OFFSET_INVALID_E                  ,pclExtraDataPtr->isL4Valid);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_L4_VALID_E                           ,pclExtraDataPtr->isL4Valid);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_IS_SYN_E                             ,descrPtr->tcpSyn);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_SYN_WITH_DATA_E                      ,descrPtr->tcpSynWithData);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_RESERVED_8_E                         ,0);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_L2_ECHO_E                            ,descrPtr->VntL2Echo);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_CFM_PACKET_E                         ,descrPtr->cfm);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_TIMESTAMP_EN_E                       ,descrPtr->oamInfo.timeStampEnable);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_TIMESTAMP_TAGGED_E                   ,descrPtr->timestampTagged[SMAIN_DIRECTION_INGRESS_E]);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_RESERVED_9_E                         ,0);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_TIMESTAMP_E                          ,descrPtr->timestampTagInfo[SMAIN_DIRECTION_INGRESS_E].timestamp.nanoSecondTimer);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_IS_PTP_E                             ,descrPtr->isPtp);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_PTP_DOMAIN_E                         ,descrPtr->ptpDomain);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_PTP_U_FIELD_E                        ,descrPtr->ptpUField);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_PTP_TAI_SELECT_E                     ,descrPtr->ptpTaiSelect);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_PTP_TRIGGER_TYPE_E                   ,descrPtr->ptpTriggerType);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_OAM_PROCESSING_EN_E                  ,descrPtr->oamInfo.oamProcessEnable);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_OAM_PTP_OFFSET_INDEX_OR_PTP_OFFSET_E ,descrPtr->oamInfo.offsetIndex);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_RX_SNIFF_OR_SRC_TRG_E                ,rx_sniff_or_src_trg);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_SRC_TRG_EPORT_E                      ,src_trg_eport);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_SRC_TRG_DEV_E                        ,src_trg_dev);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_ANALYZER_INDEX_E                     ,descrPtr->analyzerIndex);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_RESERVED_10_E                        ,0);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_SRC_TRG_PHY_PORT_E                   ,src_trg_phy_port);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_IS_MPLS_E                            ,descrPtr->mpls);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_MPLS_CMD_E                           ,descrPtr->mplsCommand);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_RESERVED_11_E                        ,0);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_ECN_CAPABLE_E                        ,descrPtr->ecnCapable);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_QCN_RX_E                             ,descrPtr->qcnRx);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_ORIG_RX_QCN_PRIO_E                   ,descrPtr->origRxQcnPrio);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_RESERVED_12_E                        ,0);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_TUNNEL_TERMINATED_E                  ,descrPtr->tunnelTerminated);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_INNER_PACKET_TYPE_E                  ,passengerKeyPacketType);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_TUNNEL_START_E                       ,descrPtr->tunnelStart);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_TUNNEL_START_PASSENGER_TYPE_E        ,descrPtr->tunnelStartPassengerType);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_ARP_PTR_OR_TUNNEL_PTR_E              ,(descrPtr->tunnelStart ? descrPtr->tunnelPtr : descrPtr->arpPtr));
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_RESERVED_13_E                        ,0);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_QOS_PROFILE_PRECEDENCE_E             ,descrPtr->qosProfilePrecedence);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_QOS_PROFILE_E                        ,descrPtr->qos.qosProfile);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_MODIFY_UP_E                          ,descrPtr->modifyUp);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_MODIFY_DSCP_EXP_E                    ,descrPtr->modifyDscp);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_RESERVED_14_E                        ,0);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_BYTE_COUNT_E                         ,descrPtr->origByteCount);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_RECALC_CRC_E                         ,0);/* valid CRC */
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_RESERVED_15_E                        ,0);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_BYPASS_INGRESS_PIPE_E                ,descrPtr->bypassIngressPipe);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_MARVELL_TAGGED_E                     ,descrPtr->marvellTagged);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_MARVELL_TAGGED_EXTENDED_E            ,descrPtr->marvellTaggedExtended);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_RESERVED_16_E                        ,0);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_RECEIVED_DSA_TAG_WORD_0_E            ,descrPtr->ingressDsa.dsaWords[0]);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_RECEIVED_DSA_TAG_WORD_1_E            ,descrPtr->ingressDsa.dsaWords[1]);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_RECEIVED_DSA_TAG_WORD_2_E            ,descrPtr->ingressDsa.dsaWords[2]);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_RECEIVED_DSA_TAG_WORD_3_E            ,descrPtr->ingressDsa.dsaWords[3]);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_IS_TRILL_E                           ,descrPtr->isTrillEtherType);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_REP_E                                ,descrPtr->isMultiTargetReplication);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_REP_LAST_E                           ,descrPtr->isMultiTarget/*0 is first , 1 is the last*/);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_PACKET_TYPE_E                        ,tunnelKeyPacketType);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_RESERVED_17_E                        ,0);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_APPLICABLE_FLOW_SUB_TEMPLATE_E       ,applicableFlowSubTemplate);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_METERING_EN_E                        ,descrPtr->policerEn);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_BILLING_EN_E                         ,descrPtr->policerCounterEn);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_RESERVED_18_E                        ,0);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_POLICER_PTR_E                        ,descrPtr->policerPtr);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_INGRESS_CORE_ID_E                    ,descrPtr->srcCoreId);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_RX_IS_PROTECTION_PATH_E              ,descrPtr->rxIsProtectionPath);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_RX_PROTECTION_SWITCH_EN_E            ,descrPtr->rxEnableProtectionSwitching);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_RESERVED_19_E                        ,0);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_COPY_RESERVED_E                      ,0);
    SMEM_LION3_IPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_IPCL_META_DATA_FIELDS_RESERVED_20_E                        ,0);
}

/*******************************************************************************
*   snetLion3IPclUdbMetaDataGet
*
* DESCRIPTION:
*       IPCL get metadata byte from the packet descriptor fields
*               (relevant for SIP5 only)
*
* INPUTS:
*       devObjPtr - (pointer to) device object.
*       descrPtr  - (pointer to) frame data buffer Id
*       udbIdx    - user defined byte index
*
* OUTPUTS:
*       udbValPtr - (pointer to) the user defined byte value
*
* RETURN:
*       GT_OK     -  Operation succeeded
*       GT_FAIL   -  Operation failed
*
* COMMENTS:
*
*******************************************************************************/
static GT_STATUS snetLion3IPclUdbMetaDataGet
(
    IN  SKERNEL_DEVICE_OBJECT           *devObjPtr,
    IN  SKERNEL_FRAME_CHEETAH_DESCR_STC *descrPtr,
    IN  GT_U32                           udbIdx,
    OUT GT_U8                           *udbValPtr
)
{
    DECLARE_FUNC_NAME(snetLion3IPclUdbMetaDataGet);

    if(udbIdx >= (sizeof(descrPtr->ipclMetadataInfo) / sizeof(GT_U8)))
    {
        __LOG(("configuration ERROR : try to get udbIdx[%d] out of the supported range \n",
            udbIdx));

        /* out of the supported range */
        *udbValPtr = 0;
        return GT_FAIL;
    }

    /* get the needed 8 bits from the 'metadata' fields */
    *udbValPtr = (GT_U8)snetFieldValueGet(descrPtr->ipclMetadataInfo, 8*udbIdx ,8);

    return GT_OK;
}

/*******************************************************************************
*   snetLion3EPclUdbMetaDataGet
*
* DESCRIPTION:
*       EPCL get metadata byte from the packet descriptor fields
*               (relevant for SIP5 only)
*
* INPUTS:
*       devObjPtr - (pointer to) device object.
*       descrPtr  - (pointer to) frame data buffer Id
*       udbIdx    - user defined byte index
*
* OUTPUTS:
*       udbValPtr - (pointer to) the user defined byte value
*
* RETURN:
*       GT_OK     -  Operation succeeded
*       GT_FAIL   -  Operation failed
*
* COMMENTS:
*
*******************************************************************************/
static GT_STATUS snetLion3EPclUdbMetaDataGet
(
    IN  SKERNEL_DEVICE_OBJECT           *devObjPtr,
    IN  SKERNEL_FRAME_CHEETAH_DESCR_STC *descrPtr,
    IN  GT_U32                           udbIdx,
    OUT GT_U8                           *udbValPtr
)
{
    DECLARE_FUNC_NAME(snetLion3EPclUdbMetaDataGet);

    if(udbIdx >= (sizeof(descrPtr->epclMetadataInfo) / sizeof(GT_U8)))
    {
        __LOG(("configuration ERROR : try to get udbIdx[%d] out of the supported range \n",
            udbIdx));

        /* out of the supported range */
        *udbValPtr = 0;
        return GT_FAIL;
    }

    /* get the needed 8 bits from the 'metadata' fields */
    *udbValPtr = (GT_U8)snetFieldValueGet(descrPtr->epclMetadataInfo, 8*udbIdx ,8);

    return GT_OK;
}

/*******************************************************************************
*   snetLion3TtiUdbMetaDataGet
*
* DESCRIPTION:
*       get metadata byte from the packet descriptor fields
*               (relevant for SIP5 only)
*
* INPUTS:
*       devObjPtr - (pointer to) device object.
*       descrPtr  - (pointer to) frame data buffer Id
*       udbIdx    - user defined byte index
*
* OUTPUTS:
*       udbValPtr - (pointer to) the user defined byte value
*
* RETURN:
*       GT_OK     -  Operation succeeded
*       GT_FAIL   -  Operation failed
*
* COMMENTS:
*
*******************************************************************************/
static GT_STATUS snetLion3TtiUdbMetaDataGet
(
    IN  SKERNEL_DEVICE_OBJECT           *devObjPtr,
    IN  SKERNEL_FRAME_CHEETAH_DESCR_STC *descrPtr,
    IN  GT_U32                           udbIdx,
    OUT GT_U8                           *udbValPtr
)
{
    DECLARE_FUNC_NAME(snetLion3TtiUdbMetaDataGet);
    GT_U32  udbVal;

    ASSERT_PTR(udbValPtr);

    if(udbIdx >= (sizeof(descrPtr->ttiMetadataInfo) / sizeof(GT_U8)))
    {
        __LOG(("try to get udbIdx[%d] out of the supported range \n",
            udbIdx));

        /* out of the supported range */
        *udbValPtr = 0;
        return GT_OK;
    }

    /* get the needed 8 bits from the 'metadata' fields */
    udbVal = snetFieldValueGet(descrPtr->ttiMetadataInfo, 8*udbIdx ,8);

    *udbValPtr = (GT_U8)udbVal;

    return GT_OK;
}


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
)
{
    DECLARE_FUNC_NAME(snetXCatPclUserDefinedByteGet);
    static char* anchorStr[8]={"L2",        "MPLS - 2",     "L3 - 2",       "L4",
                        "Tunnel_L2",      "Tunnel_L3 - 2",  "metadata",     "not supported"};

    GT_U8   *tmpAnchorBytePacketPtr = NULL;/* pointer to anchor byte in the packet */
    GT_U32  indexInPacket = 0,packetLength , maxOffsetSupported;
    GT_STATUS rc = GT_OK;
    GT_BOOL isMetaData = GT_FALSE;
    static GT_BOOL supportedAnchorsArr[SNET_UDB_CLIENT___LAST___E][8] =
        {                            /*0*/    /*1*/   /*2*/  /*3*/   /*4*/     /*5*/    /*6*/  /*7*/
         /*SNET_UDB_CLIENT_TTI_E*/ {GT_TRUE,GT_TRUE,GT_TRUE,GT_TRUE,GT_FALSE,GT_FALSE,GT_TRUE,GT_FALSE}
        ,/*SNET_UDB_CLIENT_IPCL_E*/{GT_TRUE,GT_TRUE,GT_TRUE,GT_TRUE,GT_TRUE ,GT_TRUE ,GT_TRUE,GT_FALSE}
        ,/*SNET_UDB_CLIENT_EPCL_E*/{GT_TRUE,GT_TRUE,GT_TRUE,GT_TRUE,GT_TRUE ,GT_TRUE ,GT_TRUE,GT_FALSE}
        };
    SNET_INGRESS_TUNNEL_TRANSIT_TYPE_ENT transitType;
    SKERNEL_FRAME_CHEETAH_DESCR_STC  * origDescrPtr;/* pointer to the original descriptor */
    SKERNEL_FRAME_CHEETAH_DESCR_STC  * passengerDescrPtr;/* pointer to the transit descriptor */

    SKERNEL_FRAME_CHEETAH_DESCR_STC  *l2AnchorDescPtr,*l3AnchorDescPtr,*l4AnchorDescPtr,*mplsAnchorDescPtr;
    SKERNEL_FRAME_CHEETAH_DESCR_STC  *tunnelL2AnchorDescPtr,*tunnelL3AnchorDescPtr;
    SKERNEL_FRAME_CHEETAH_DESCR_STC  *metadataAnchorDescPtr;
    GT_BIT      supportTunnel = 0;
    static GT_U8       l3Minus2DummyBytes[2] = {0,0};/* dummy 2 bytes to be used for 'L3-2' offsets 0,1 */
    GT_BIT  use_l3Minus2DummyBytes = 0;
    GT_BIT  offsetInIngressBuffer = 0;


    *userDefinedByteValuePtr = 0;

    if(userDefinedAnchor >= 8 || udbClient >= SNET_UDB_CLIENT___LAST___E)
    {
        return GT_FAIL;
    }
    else if(supportedAnchorsArr[udbClient][userDefinedAnchor] == GT_FALSE)
    {
        __LOG((" the client not support this type of anchor[%d] \n",
            userDefinedAnchor));
        return GT_FAIL;
    }

    metadataAnchorDescPtr = descrPtr;

    if(udbClient == SNET_UDB_CLIENT_IPCL_E)
    {
        if(descrPtr->ingressTunnelInfo.origDescrPtr == NULL)
        {
            skernelFatalError("snetXCatPclUserDefinedByteGet: origDescrPtr = NULL \n");
        }

        transitType       = descrPtr->ingressTunnelInfo.origDescrPtr->ingressTunnelInfo.transitType;
        passengerDescrPtr = descrPtr->ingressTunnelInfo.origDescrPtr->ingressTunnelInfo.passengerDescrPtr;
        origDescrPtr      = descrPtr->ingressTunnelInfo.origDescrPtr->ingressTunnelInfo.origDescrPtr;

        if(origDescrPtr == NULL || passengerDescrPtr == NULL)
        {
            skernelFatalError("snetXCatPclUserDefinedByteGet: origDescrPtr == NULL || passengerDescrPtr == NULL \n");
        }

        if(transitType != SNET_INGRESS_TUNNEL_TRANSIT_TYPE__NONE__E)
        {
            if(transitType == SNET_INGRESS_TUNNEL_TRANSIT_TYPE_LEGACY_IP_OVER_MPLS_TUNNEL_E)
            {
                l2AnchorDescPtr          = origDescrPtr;
                mplsAnchorDescPtr        = origDescrPtr;
            }
            else
            {
                supportTunnel            = 1;
                l2AnchorDescPtr          = passengerDescrPtr;
                mplsAnchorDescPtr        = passengerDescrPtr;
            }
            l3AnchorDescPtr          = passengerDescrPtr;
            l4AnchorDescPtr          = passengerDescrPtr;
            tunnelL2AnchorDescPtr    = origDescrPtr;
            tunnelL3AnchorDescPtr   = origDescrPtr;
        }
        else
        {
            l2AnchorDescPtr          = descrPtr;
            mplsAnchorDescPtr        = descrPtr;
            l3AnchorDescPtr          = descrPtr;
            l4AnchorDescPtr          = descrPtr;
            tunnelL2AnchorDescPtr    = descrPtr;
            tunnelL3AnchorDescPtr   = descrPtr;
        }
    }
    else
    {
        l2AnchorDescPtr          = descrPtr;
        l3AnchorDescPtr          = descrPtr;
        l4AnchorDescPtr          = descrPtr;
        mplsAnchorDescPtr        = descrPtr;
        tunnelL2AnchorDescPtr    = descrPtr;
        tunnelL3AnchorDescPtr   = descrPtr;

        origDescrPtr = descrPtr;
    }

    switch (userDefinedAnchor)
    {
        case 0: /* L2 */
            if(udbClient != SNET_UDB_CLIENT_EPCL_E)
            {
                if(l2AnchorDescPtr->l2Valid == 0)
                {
                    __LOG(("the anchor[%s] byte offset[%d] not supported for non-ethernet passenger packet \n",
                                  anchorStr[userDefinedAnchor],
                                  userDefinedOffset));
                    /*IP-over-X Tunnel terminated packet*/
                    return GT_OK;
                }
                tmpAnchorBytePacketPtr = l2AnchorDescPtr->macDaPtr;
            }
            else /*epcl*/
            {
                if(l2AnchorDescPtr->haToEpclInfo.macDaSaPtr == NULL)
                {
                    __LOG(("the anchor[%s] byte offset[%d] not supported for non-ethernet passenger packet \n",
                                  anchorStr[userDefinedAnchor],
                                  userDefinedOffset));
                    /*IP-over-X Tunnel terminated packet*/
                    return GT_OK;
                }


                tmpAnchorBytePacketPtr =  l2AnchorDescPtr->haToEpclInfo.macDaSaPtr;
            }
            break;
        case 1: /* MPLS - 2 */
            if(udbClient != SNET_UDB_CLIENT_EPCL_E)
            {
                if(mplsAnchorDescPtr->mpls == 0)
                {
                    __LOG(("the anchor[%s] byte offset[%d] not supported for Non-mpls packet \n",
                                  anchorStr[userDefinedAnchor],
                                  userDefinedOffset));
                    /*Any non-MPLS packet*/
                    return GT_OK;
                }
                else
                {
                    tmpAnchorBytePacketPtr = mplsAnchorDescPtr->origInfoBeforeTunnelTermination.originalL3Ptr - 2;
                }
            }
            else  /*EPCL*/
            {
                if(mplsAnchorDescPtr->mpls == 1 ||
                   (mplsAnchorDescPtr->tunnelStart &&
                    mplsAnchorDescPtr->tunnelStartType == SKERNEL_FRAME_TUNNEL_START_TYPE_MPLS_E))
                {
                    tmpAnchorBytePacketPtr = mplsAnchorDescPtr->haToEpclInfo.l3StartOffsetPtr - 2;
                }
                else
                {
                    __LOG(("the anchor[%s] byte offset[%d] not supported for Non-mpls packet \n",
                                  anchorStr[userDefinedAnchor],
                                  userDefinedOffset));
                    /*Any non-MPLS packet*/
                    return GT_OK;
                }
            }

            break;
        case 2: /* L3 - 2 */
            if (udbClient == SNET_UDB_CLIENT_TTI_E)
            {
                if(l3AnchorDescPtr->l2Encaps == SKERNEL_LLC_E && l3AnchorDescPtr->l2Valid)
                {
                    __LOG(("Anchor 'L3-2' : IEEE 802.3 (LLC Encapsulation) (non-SNAP) start after 'DSAP' so first bytes : ‘SSAP,CTRL' bytes \n"));
                    /* current simulation sets l3AnchorDescPtr->l3StartOffsetPtr after the DSAP ,SSAP */
                    /* but the L3-2 is after the 'DSAP’ byte , so need to reduce 1 byte */
                    tmpAnchorBytePacketPtr = l3AnchorDescPtr->l3StartOffsetPtr - 1;
                }
                else
                {
                    tmpAnchorBytePacketPtr = l3AnchorDescPtr->mpls ?
                                                 l3AnchorDescPtr->afterMplsLabelsPtr - 2 :
                                                 l3AnchorDescPtr->l3StartOffsetPtr   - 2 ;
                }
            }
            else if (udbClient == SNET_UDB_CLIENT_IPCL_E)
            {
                if(l3AnchorDescPtr->l2Valid == 0)
                {
                    if(userDefinedOffset == 0 || userDefinedOffset == 1)
                    {
                        /* the first 2 bytes are not really valid as those are part of the 'L2 part' (ethertype) --> use dummy value '0' */
                        __LOG(("Anchor 'L3-2' : the first 2 bytes are not really valid as those are part of the 'L2 part' (ethertype) --> use dummy value '0' \n"));
                        tmpAnchorBytePacketPtr = &l3Minus2DummyBytes[0];
                        use_l3Minus2DummyBytes = 1;
                    }
                }

                if(use_l3Minus2DummyBytes == 0)
                {
                    if ((l3AnchorDescPtr->tunnelTerminated == 0) &&
                        (l3AnchorDescPtr->mpls == 1) &&
                        (l3AnchorDescPtr->ingressTunnelInfo.transitType == SNET_INGRESS_TUNNEL_TRANSIT_TYPE__NONE__E))
                    {
                        __LOG(("Anchor 'L3-2' in MPLS with non parsed passenger \n"));
                        /* end of last MPLS label - 2             */
                        tmpAnchorBytePacketPtr = l3AnchorDescPtr->afterMplsLabelsPtr - 2;
                    }
                    else
                    if(l3AnchorDescPtr->l2Encaps == SKERNEL_LLC_E && l3AnchorDescPtr->l2Valid)
                    {
                        __LOG(("Anchor 'L3-2' : IEEE 802.3 (LLC Encapsulation) (non-SNAP) start after 'DSAP' so first bytes : ‘SSAP,CTRL' bytes \n"));
                        /* current simulation sets l3AnchorDescPtr->l3StartOffsetPtr after the DSAP ,SSAP */
                        /* but the L3-2 is after the 'DSAP’ byte , so need to reduce 1 byte */
                        tmpAnchorBytePacketPtr = l3AnchorDescPtr->l3StartOffsetPtr - 1;
                    }
                    else
                    {
                        /* default */
                        tmpAnchorBytePacketPtr = l3AnchorDescPtr->l3StartOffsetPtr - 2;
                    }
                }
            }
            else /*epcl*/
            {
                if(l2AnchorDescPtr->haToEpclInfo.macDaSaPtr == NULL)
                {
                    if(userDefinedOffset == 0 || userDefinedOffset == 1)
                    {
                        /* the first 2 bytes are not really valid as those are part of the 'L2 part' (ethertype) --> use dummy value '0' */
                        __LOG(("Anchor 'L3-2' : the first 2 bytes are not really valid as those are part of the 'L2 part' (ethertype) --> use dummy value '0' \n"));
                        tmpAnchorBytePacketPtr = &l3Minus2DummyBytes[0];
                        use_l3Minus2DummyBytes = 1;
                    }
                }

                if(use_l3Minus2DummyBytes == 0)
                {
                    /* default */
                    if((l3AnchorDescPtr->tunnelStart == 0) && l3AnchorDescPtr->mpls)
                    {
                        __LOG(("Anchor 'L3-2' in MPLS with non parsed passenger \n"));
                        /* MPLS packet from ingress pipe */
                        tmpAnchorBytePacketPtr = l3AnchorDescPtr->afterMplsLabelsPtr - 2;
                        offsetInIngressBuffer = 1;/*this is pointer in the ingress buffer */
                    }
                    else
                    if(l3AnchorDescPtr->l2Encaps == SKERNEL_LLC_E && l3AnchorDescPtr->l2Valid)
                    {
                        __LOG(("Anchor 'L3-2' : IEEE 802.3 (LLC Encapsulation) (non-SNAP) start after 'DSAP' so first bytes : ‘SSAP,CTRL' bytes \n"));
                        /* current simulation sets l3AnchorDescPtr->l3StartOffsetPtr after the DSAP ,SSAP */
                        /* but the L3-2 is after the 'DSAP’ byte , so need to reduce 1 byte */
                        tmpAnchorBytePacketPtr = l3AnchorDescPtr->haToEpclInfo.l3StartOffsetPtr - 1;
                    }
                    else
                    {
                        /* not MPLS packet */
                        tmpAnchorBytePacketPtr = l3AnchorDescPtr->haToEpclInfo.l3StartOffsetPtr - 2;
                    }
                }

            }
            break;
        case 3: /* L4 */

            if(NULL == l4AnchorDescPtr->l4StartOffsetPtr)
            {
                __LOG(("the anchor[%s] byte offset[%d] not supported for Non-L4 packet \n",
                              anchorStr[userDefinedAnchor],
                              userDefinedOffset));
                /*Any non-IP and non-FCoE packet*/
                return GT_OK;
            }

            if(udbClient != SNET_UDB_CLIENT_EPCL_E)
            {
                tmpAnchorBytePacketPtr = l4AnchorDescPtr->l4StartOffsetPtr;
            }
            else
            {
                /*calc offset in the egress buffer according to offset in the ingress buffer*/
                tmpAnchorBytePacketPtr = l4AnchorDescPtr->haToEpclInfo.l3StartOffsetPtr +
                    (l4AnchorDescPtr->l4StartOffsetPtr - l4AnchorDescPtr->l3StartOffsetPtr);
            }
            break;

        /* SIP5 only (the legacy devices support 2 bits of Anchor --> so 0..3)*/
        case 4:/* Tunnel_L2; Refers to the beginning of the original packet’s MAC header (prior to tunnel-termination).
                    If packet was not tunnel-terminated - this anchor is invalid
                    NOTE: used only in PCL UDB's;*/
            if(udbClient == SNET_UDB_CLIENT_IPCL_E)
            {
                if(supportTunnel)
                {
                    /* we consider this as parsing of 'transit' so also 'tunnel parts' are valid */
                    __LOG(("Parsing of 'transit' support 'tunnel L2' as valid \n"));
                }
                else
                if(tunnelL2AnchorDescPtr->tunnelTerminated == 0)
                {
                    __LOG(("the anchor[%s] byte offset[%d] not supported for Non-TT packet \n",
                                  anchorStr[userDefinedAnchor],
                                  userDefinedOffset));
                    /*Non tunnel-terminated packet*/
                    return GT_OK;
                }
                tmpAnchorBytePacketPtr =  tunnelL2AnchorDescPtr->origInfoBeforeTunnelTermination.originalL2Ptr;
            }
            else
            if(udbClient == SNET_UDB_CLIENT_EPCL_E)
            {
                if(tunnelL2AnchorDescPtr->tunnelStart == 0)
                {
                    __LOG(("the anchor[%s] byte offset[%d] not supported for Non-TS packet \n",
                                  anchorStr[userDefinedAnchor],
                                  userDefinedOffset));
                    /*Non tunnel-start packet*/
                    return GT_OK;
                }

                tmpAnchorBytePacketPtr =  tunnelL2AnchorDescPtr->haToEpclInfo.tunnelStartL2StartOffsetPtr;
            }
            break;
        case 5:/*Tunnel L3; Tunnel_L3; Same as ‘L3’ anchor but of the original packet’s header (prior to tunnel-termination).
                    If packet was not tunnel-terminated - this anchor is invalid
                    NOTE: used only in PCL UDB's;*/
            if(udbClient == SNET_UDB_CLIENT_IPCL_E)
            {
                if(supportTunnel)
                {
                    /* we consider this as parsing of 'transit' so also 'tunnel parts' are valid */
                    __LOG(("the anchor[%s] byte offset[%d] : Parsing of 'transit' support 'tunnel L3' as valid \n",
                                  anchorStr[userDefinedAnchor],
                                  userDefinedOffset));
                }
                else
                if(tunnelL3AnchorDescPtr->tunnelTerminated == 0)
                {
                    __LOG(("the anchor[%s] byte offset[%d] not supported for Non-TT packet \n",
                                  anchorStr[userDefinedAnchor],
                                  userDefinedOffset));
                    /*Non tunnel-terminated packet*/
                    return GT_OK;
                }

                tmpAnchorBytePacketPtr =  tunnelL3AnchorDescPtr->origInfoBeforeTunnelTermination.originalL3Ptr - 2;/*like the regular L3 , this is L3 - 2*/
            }
            else
            if(udbClient == SNET_UDB_CLIENT_EPCL_E)
            {
                if(tunnelL3AnchorDescPtr->tunnelStart == 0)
                {
                    __LOG(("the anchor[%s] byte offset[%d] not supported for Non-TS packet \n",
                                  anchorStr[userDefinedAnchor],
                                  userDefinedOffset));
                    /*Non tunnel-start packet*/
                    return GT_OK;
                }

                tmpAnchorBytePacketPtr =  tunnelL3AnchorDescPtr->haToEpclInfo.tunnelStartL3StartOffsetPtr - 2;/*like the regular L3 , this is L3 - 2*/
            }
            break;
        case 6:/*Metadata; Metadata; Calculated fields;*/
            if(udbClient == SNET_UDB_CLIENT_TTI_E)
            {
                rc = snetLion3TtiUdbMetaDataGet(devObjPtr, metadataAnchorDescPtr, userDefinedOffset,
                            userDefinedByteValuePtr);
            }
            else
            if(udbClient == SNET_UDB_CLIENT_IPCL_E)
            {
                if(origDescrPtr->ipclMetadataReady == 0)
                {
                    /* this case can be when the CRC hash needs UDB's */

                    /* build the metadata before any changes that the PCL unit may do on the descriptor by the actions */
                    snetLion3IpclUdbMetaDataBuild(devObjPtr,
                        origDescrPtr->ingressTunnelInfo.fixedFieldsTypeDescrPtr,
                        origDescrPtr->pclExtraDataPtr);
                    origDescrPtr->ipclMetadataReady = 1;
                }


                rc = snetLion3IPclUdbMetaDataGet(devObjPtr,
                            origDescrPtr->ingressTunnelInfo.fixedFieldsTypeDescrPtr,
                            userDefinedOffset,
                            userDefinedByteValuePtr);
            }
            else /*epcl*/
            {
                if(origDescrPtr->epclMetadataReady == 0)
                {
                    /* build the Metadata only when the first time that it is actual needed */
                    snetLion3EpclUdbMetaDataBuild(devObjPtr,origDescrPtr,origDescrPtr->pclExtraDataPtr,
                        origDescrPtr->egressPhysicalPortInfo.globalPortIndex);

                    origDescrPtr->epclMetadataReady = 1;
                }

                rc = snetLion3EPclUdbMetaDataGet(devObjPtr,origDescrPtr,userDefinedOffset,
                            userDefinedByteValuePtr);
            }

            if(rc != GT_OK)
            {
                return rc;
            }
            isMetaData = GT_TRUE;
            tmpAnchorBytePacketPtr = NULL;/*not used*/
            break;

        default:
            return GT_FAIL;
    }

    if(isMetaData == GT_FALSE && tmpAnchorBytePacketPtr != NULL)
    {
        if(udbClient == SNET_UDB_CLIENT_EPCL_E)
        {
            if(use_l3Minus2DummyBytes || offsetInIngressBuffer)
            {
                indexInPacket = 0;/*dummy - to avoid fail on offset*/
            }
            else
            if(origDescrPtr->haToEpclInfo.tunnelStartL2StartOffsetPtr)
            {
                /* for TS the offsets are from start of L2 of TS */
                indexInPacket = tmpAnchorBytePacketPtr - origDescrPtr->haToEpclInfo.tunnelStartL2StartOffsetPtr;
            }
            else
            {
                indexInPacket = tmpAnchorBytePacketPtr - origDescrPtr->haToEpclInfo.macDaSaPtr;
            }
            packetLength = origDescrPtr->egressByteCount;
            maxOffsetSupported = SMEM_CHT_IS_SIP5_GET(devObjPtr) ? 256 : SNET_CHT_PCL_MAX_BYTE_INDEX_CNS;
        }
        else
        {
            if(use_l3Minus2DummyBytes)
            {
                indexInPacket = 0;/*dummy - to avoid fail on offset*/
            }
            else
            {
                indexInPacket = tmpAnchorBytePacketPtr - origDescrPtr->origInfoBeforeTunnelTermination.originalL2Ptr;
            }

            packetLength = origDescrPtr->origByteCount;
            maxOffsetSupported = SNET_CHT_PCL_MAX_BYTE_INDEX_CNS;
        }

        indexInPacket += userDefinedOffset;

        if(indexInPacket >= packetLength)
        {
            __LOG(("the UDB indexInPacket[%d] is over the packet length [%d] \n",
                indexInPacket,packetLength));
            return GT_OK;
        }

        /* check limitation from start of the packet */
        if(indexInPacket >= maxOffsetSupported)
        {
            __LOG(("the UDB indexInPacket[%d] is over the max supported parse [%d] \n",
                indexInPacket,maxOffsetSupported));
            return GT_FAIL;
        }

        *userDefinedByteValuePtr = tmpAnchorBytePacketPtr[userDefinedOffset];
    }

    if(udbClient != SNET_UDB_CLIENT_TTI_E)
    {
        /* the TTI already print this info */

        __LOG(("Got UDB : anchor[%s] byte offset[%d] with value[0x%x] \n",
                      anchorStr[userDefinedAnchor],
                      userDefinedOffset,
                      *userDefinedByteValuePtr));
    }


    return GT_OK;
}

/*******************************************************************************
* snetXCatPclGetKeyType
*
* DESCRIPTION:
*       Get the key type from the PCL configuration table.
*
* INPUTS:
*       devObjPtr       - pointer to device object.
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
extern GT_VOID snetXCatPclGetKeyType
(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC   * descrPtr,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC      * curLookupCfgPtr,
    OUT GT_U32                              * keyTypePtr
)
{
    DECLARE_FUNC_NAME(snetXCatPclGetKeyType);

    GT_BIT  isIpv4OrArp = 0;
    GT_BIT  isNonIp = 0;
    GT_BIT  isIpv6 = 0;
    GT_U32  lookupType;
    GT_U32  firstUdbKey;

    if ((descrPtr->isIPv4 == 1) || (descrPtr->arp == 1)) /* ipv4 or arp packet */
    {
        isIpv4OrArp = 1;
        lookupType = curLookupCfgPtr->pclIpV4ArpTypeCfg;
    }
    else
    if (descrPtr->isIp == 0) /* non ip, non arp packet */
    {
        isNonIp = 1;
        lookupType = curLookupCfgPtr->pclNonIpTypeCfg;
    }
    else /* ipv6 packet */
    {
        isIpv6 = 1;
        lookupType = curLookupCfgPtr->pclIpV6TypeCfg;
    }

    __LOG_PARAM(isIpv4OrArp);
    __LOG_PARAM(isNonIp);
    __LOG_PARAM(isIpv6);
    __LOG_PARAM(lookupType);


    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        firstUdbKey = (curLookupCfgPtr->ingrlookUpKey == GT_TRUE) ? 8 : 4;
        __LOG_PARAM(firstUdbKey);
        /*IPCL*/
        /*
        0x8 = 10B; _10B; UDB based key;
        0x9 = 20B; _20B; UDB based key;
        0xa = 30B; _30B; UDB based key;
        0xb = 40B; _40B; UDB based key;
        0xc = 50B; _50B; UDB based key;
        0xd = 60B; _60B; UDB based key;
        */
        /*EPCL*/
        /*
         0x4 = 10B_UDB; 10B_UDB; 10 Bytes UDB only key;
         0x5 = 20B_UDB; 20B_UDB; 20 Bytes UDB only key;
         0x6 = 30B_UDB; 30B_UDB; 30 Bytes UDB only key;
         0x7 = 40B_UDB; 40B_UDB; 40 Bytes UDB only key;
         0x8 = 50B_UDB; 50B_UDB; 50 Bytes UDB only key;
         0x9 = 60B_UDB; 60B_UDB; 60 Bytes UDB key;
        */
        if(lookupType >= firstUdbKey)
        {
            if(lookupType > (firstUdbKey + 5))
            {
                __LOG(("configuration ERROR : lookupType[%d] > [%d] , treat it as 10B key \n",
                    lookupType));
                lookupType = firstUdbKey; /*10B*/
            }

            *keyTypePtr = (curLookupCfgPtr->ingrlookUpKey == GT_TRUE) ?
                    (LION3_IPCL_KEY_TYPE_UBB_10B_E + (lookupType-firstUdbKey)):/*ipcl*/
                    (LION3_EPCL_KEY_TYPE_UBB_10B_E + (lookupType-firstUdbKey));/*epcl*/
            curLookupCfgPtr->keySize = CHT_PCL_KEY_10B_E + (lookupType-firstUdbKey);

            __LOG(("use UDB key of size[%d] bytes \n",
                ((lookupType - firstUdbKey + 1)* 10)));

            return;
        }
    }

    if (curLookupCfgPtr->ingrlookUpKey == GT_TRUE)/*IPCL*/
    {
        /* ingress keys */
        if (isIpv4OrArp) /* ipv4 or arp packet */
        {
            *keyTypePtr = type_IPCL_IPV4[lookupType];
        }
        else if (isNonIp) /* non ip, non arp packet */
        {
            *keyTypePtr = type_IPCL_NOT_IP[lookupType];
        }
        else /* ipv6 packet */
        {
            *keyTypePtr = type_IPCL_IPV6[lookupType];
        }

        switch (*keyTypePtr)
        {
            default:
            case XCAT_IPCL_KEY_TYPE_L2_KEY_SHORT_E:
            case XCAT_IPCL_KEY_TYPE_L2_L3_KEY_SHORT_E:
            case XCAT_IPCL_KEY_TYPE_L3_L4_KEY_SHORT_E:
            case XCAT_IPCL_KEY_TYPE_IPCL1_IPV4_KEY_SHORT_E:
            case XCAT_IPCL_KEY_TYPE_IPV6_DIP_KEY_SHORT_E:
            case XCAT_IPCL_KEY_TYPE_UDB_KEY_SHORT_E:
                curLookupCfgPtr->keySize = CHT_PCL_KEY_REGULAR_E;
                break;
            case XCAT_IPCL_KEY_TYPE_L2_L3_L4_KEY_LONG_E:
            case XCAT_IPCL_KEY_TYPE_L2_IPV6_KEY_LONG_E:
            case XCAT_IPCL_KEY_TYPE_L4_IPV6_KEY_LONG_E:
            case XCAT_IPCL_KEY_TYPE_UDB_KEY_LONG_E:
            case XCAT_IPCL_KEY_TYPE_IPCL0_IPV4_KEY_LONG_E:
                curLookupCfgPtr->keySize = CHT_PCL_KEY_EXTENDED_E;
                break;
            case XCAT_IPCL_KEY_TYPE_IPCL0_IPV6_KEY_TRIPLE_E:
            case XCAT_IPCL_KEY_TYPE_IPCL1_IPV6_KEY_TRIPLE_E:
                curLookupCfgPtr->keySize = CHT_PCL_KEY_TRIPLE_E;
                break;
        }
    }
    else /*EPCL*/
    {
        /* egress keys */
        if (isIpv4OrArp) /* ipv4 or arp packet */
        {
            *keyTypePtr = type_EPCL_IPV4[lookupType];
        }
        else if (isNonIp) /* non ip, non arp packet */
        {
            *keyTypePtr = type_EPCL_NOT_IP[lookupType];
        }
        else /* ipv6 packet */
        {
            *keyTypePtr = type_EPCL_IPV6[lookupType];
        }

        switch (*keyTypePtr)
        {
            default:
            case CHT3_EPCL_KEY_TYPE_L2_KEY_SHORT_E:
            case CHT3_EPCL_KEY_TYPE_L2_L3_KEY_SHORT_E:
            case CHT3_EPCL_KEY_TYPE_L3_L4_KEY_SHORT_E:
                curLookupCfgPtr->keySize = CHT_PCL_KEY_REGULAR_E;
                break;
            case CHT3_EPCL_KEY_TYPE_L2_L3_L4_KEY_LONG_E:
            case CHT3_EPCL_KEY_TYPE_L2_IPV6_KEY_LONG_E:
            case CHT3_EPCL_KEY_TYPE_L4_IPV6_KEY_LONG_E:
            case CHT3_EPCL_KEY_TYPE_L4_IPV4_NON_IP_KEY_LONG_E:
                curLookupCfgPtr->keySize = CHT_PCL_KEY_EXTENDED_E;
                break;
            case CHT3_EPCL_KEY_TYPE_L4_IPV6_KEY_TRIPLE_E:
                curLookupCfgPtr->keySize = CHT_PCL_KEY_TRIPLE_E;
                 break;
        }
    }

    return ;
}


/*******************************************************************************
*   snetLion3IPclKeyFieldBuildByValue
*
* DESCRIPTION:
*       Lion3 : Function insert data of field to the search key in specific place in PCL key
*
* INPUTS:
*       pclKeyPtr - pointer to current pcl key
*       fieldVal - data of field to insert to key
*       fieldId - field id
*
* OUTPUTS:
*
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID snetLion3IPclKeyFieldBuildByValue
(
    INOUT SNET_CHT_POLICY_KEY_STC        * pclKeyPtr,
    IN GT_U32                          fieldVal,
    IN LION3_PCL_KEY_FIELDS_ID_ENT     fieldId
)
{
    CHT_PCL_KEY_FIELDS_INFO_STC *fieldInfoPtr;

    if(fieldId > LION3_PCL_KEY_FIELDS_ID___LAST__E)
    {
        /* this is not valid field for such device */
        skernelFatalError("snetLion3IPclKeyFieldBuildByValue: not valid field[%d] \n",fieldId);
    }

    fieldInfoPtr = &lion3PclKeyFieldsData[fieldId];

    snetChtPclSrvKeyFieldBuildByValue(pclKeyPtr, fieldVal,  fieldInfoPtr);
}

/*******************************************************************************
*   snetLion3IPclKeyFieldBuildByPointer
*
* DESCRIPTION:
*       Lion3 : Function insert data of field to the search key in specific place in key
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
static GT_VOID snetLion3IPclKeyFieldBuildByPointer
(
    INOUT SNET_CHT_POLICY_KEY_STC *pclKeyPtr,
    IN GT_U8                      *fieldValPtr,
    IN LION3_PCL_KEY_FIELDS_ID_ENT fieldId
)
{
    CHT_PCL_KEY_FIELDS_INFO_STC *fieldInfoPtr;

    if(fieldId > LION3_PCL_KEY_FIELDS_ID___LAST__E)
    {
        /* this is not valid field for such device */
        skernelFatalError("snetLion3IPclKeyFieldBuildByPointer: not valid field[%d] \n",fieldId);
    }

    fieldInfoPtr = &lion3PclKeyFieldsData[fieldId];

    snetChtPclSrvKeyFieldBuildByPointer(pclKeyPtr, fieldValPtr, fieldInfoPtr);
}

/*******************************************************************************
*   snetXCatIPclKeyFieldBuildByValue
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
static GT_VOID snetXCatIPclKeyFieldBuildByValue
(
    SNET_CHT_POLICY_KEY_STC        * pclKeyPtr,
    GT_U32                          fieldVal,
    XCAT_PCL_KEY_FIELDS_ID_ENT      fieldId
)
{
    CHT_PCL_KEY_FIELDS_INFO_STC *fieldInfoPtr;

    if(fieldId > XCAT_PCL_KEY_FIELDS_IP_HEADER_OK_KEY12_E)
    {
        /* this is not valid field for such device */
        skernelFatalError("snetXCatIPclKeyFieldBuildByValue: not valid field[%d]",fieldId);
    }

    if (pclKeyPtr->devObjPtr->iPclKeyFormatVersion)
    {
        fieldInfoPtr = &xcat2PclKeyFieldsData[fieldId];
    }
    else /* version 0 */
    {
        fieldInfoPtr = &xcatPclKeyFieldsData[fieldId];
    }

    snetChtPclSrvKeyFieldBuildByValue(pclKeyPtr, fieldVal,  fieldInfoPtr);
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
static GT_VOID snetXCatIPclKeyFieldBuildByPointer
(
    INOUT SNET_CHT_POLICY_KEY_STC *pclKeyPtr,
    IN GT_U8                      *fieldValPtr,
    IN XCAT_PCL_KEY_FIELDS_ID_ENT  fieldId
)
{
    CHT_PCL_KEY_FIELDS_INFO_STC *fieldInfoPtr;

    if(fieldId > XCAT_PCL_KEY_FIELDS_IP_HEADER_OK_KEY12_E)
    {
        /* this is not valid field for such device */
        skernelFatalError("snetXCatIPclKeyFieldBuildByPointer: not valid field[%d]",fieldId);
    }

    if (pclKeyPtr->devObjPtr->iPclKeyFormatVersion)
    {
        fieldInfoPtr = &xcat2PclKeyFieldsData[fieldId];
    }
    else /* version 0 */
    {
        fieldInfoPtr = &xcatPclKeyFieldsData[fieldId];
    }

    snetChtPclSrvKeyFieldBuildByPointer(pclKeyPtr, fieldValPtr, fieldInfoPtr);
}


/*******************************************************************************
*   snetXCatPclUdbKeyValueGet
*
* DESCRIPTION:
*        Get user defined value by user defined key.
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       descrPtr            - pointer to frame data buffer Id.
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
static GT_STATUS snetXCatPclUdbKeyValueGet
(
    IN SKERNEL_DEVICE_OBJECT                        * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC              * descrPtr,
    IN SNET_XCAT_IPCL_KEY_TYPE_ENT                    keyType,
    IN GT_U32                                         udbIdx,
    OUT GT_U8                                       * byteValuePtr
)
{
    DECLARE_FUNC_NAME(snetXCatPclUdbKeyValueGet);

    GT_U32  regAddr;            /* register address */
    GT_U32  fieldValue;           /* field value */
    SNET_XCAT_PCL_UDB_MUX_MODES_ENT udbType;/* udb may be muxed with other value ... check the type of udb */
    GT_U32  udbGlobalCfg;       /* UDB global configuration */
    GT_U32  vrfId_or_tag1Info = descrPtr->vrfId;/* the vrId or the tag1 info for 2 UDBs*/
    GT_U32  keyIndex;/* key index (0..12) */

    ASSERT_PTR(byteValuePtr);

    *byteValuePtr = 0;

    /* default */
    udbType = SNET_XCAT_PCL_UDB_MUX_NONE_E;/* udb is not muxed with other value */

    regAddr = SMEM_XCAT_POLICY_UDB_CONF_REG(devObjPtr);
    smemRegGet(devObjPtr, regAddr , &udbGlobalCfg);

    if(devObjPtr->supportVpls)
    {
        /* use the when <VID1 override VRF-ID mode>==1 */
        regAddr = SMEM_CHT_PCL_GLOBAL_REG(devObjPtr);
        smemRegGet(devObjPtr, regAddr , &fieldValue);
        fieldValue = SMEM_U32_GET_FIELD(fieldValue,16,1);
        if(SMEM_U32_GET_FIELD(fieldValue,16,1))
        {
            /*
                [15] Tag1_Exists
                [14:12] UP1
                [11:0] VID1
            */
            vrfId_or_tag1Info = descrPtr->vid1 |
                               (descrPtr->up1 << 12) |
                               (TAG1_EXIST_MAC(descrPtr) << 15);
        }
    }

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr) &&
        SMEM_U32_GET_FIELD(udbGlobalCfg,26,1))/*<Enable FlowID Key Extension>*/
    {
        switch (udbIdx)
        {
            case 41:
                udbType = SNET_XCAT_PCL_UDB_MUX_FLOW_ID_BYTE_0_E;
                goto getUdbValue_lbl;
            case 42:
                udbType = SNET_XCAT_PCL_UDB_MUX_FLOW_ID_BYTE_1_E;
                goto getUdbValue_lbl;
            default:
                break;
        }
    }


    switch (keyType)
    {
        case XCAT_IPCL_KEY_TYPE_L2_KEY_SHORT_E:
            if ((udbIdx == 15) && ((udbGlobalCfg & 1) != 0))
            {
                udbType = SNET_XCAT_PCL_UDB_MUX_VRF_LSB_E;
            }
            else
            if (udbIdx == 16)
            {
                if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
                {
                    switch(SMEM_U32_GET_FIELD(udbGlobalCfg,27,2))
                    {
                        case 0: /*Disable: Key 0 bits[79:72] are used for UDB16*/
                            break;
                        case (1):/*Enable VRF-ID MSB Override: Key 0 bits[79:72] are override with VRF-ID[11:8]*/
                            udbType = SNET_XCAT_PCL_UDB_MUX_VRF_MSB_E;
                            break;
                        case (2):/*Enable Tag1 VID MSB Override: Key 0 bits[79:72] are override with {4b0,Tag1 VID[11:8]}.*/
                            udbType = SNET_XCAT_PCL_UDB_MUX_TAG1_INFO_E;
                            break;
                        case (3): /*Reserved*/
                            __LOG(("ERROR : Reserved value \n"));
                            break;
                        default:/* can't happen on  */
                            break;
                    }

                }
                else if((udbGlobalCfg & (1 << 1)) != 0)
                {
                    udbType = SNET_XCAT_PCL_UDB_MUX_VRF_MSB_E;
                }
            }
            break;
        case XCAT_IPCL_KEY_TYPE_L2_L3_KEY_SHORT_E:
            if ((udbIdx == 18) && ((udbGlobalCfg & (1 << 2)) != 0))
            {
                udbType = SNET_XCAT_PCL_UDB_MUX_VRF_LSB_E;
            }
            if ((udbIdx == 19) && ((udbGlobalCfg & (1 << 3)) != 0))
            {
                udbType = SNET_XCAT_PCL_UDB_MUX_VRF_MSB_E;
            }
            break;
        case XCAT_IPCL_KEY_TYPE_L3_L4_KEY_SHORT_E:
            if ((udbIdx == 20) && ((udbGlobalCfg & (1 << 4)) != 0))
            {
                udbType = SNET_XCAT_PCL_UDB_MUX_VRF_LSB_E;
            }
            if ((udbIdx == 21) && ((udbGlobalCfg & (1 << 5)) != 0))
            {
                udbType = SNET_XCAT_PCL_UDB_MUX_VRF_MSB_E;
            }
            break;
        case XCAT_IPCL_KEY_TYPE_L2_L3_L4_KEY_LONG_E:
            if ((udbIdx == 5) && ((udbGlobalCfg & (1 << 8)) != 0))
            {
                udbType = SNET_XCAT_PCL_UDB_MUX_VRF_LSB_E;
            }
            if ((udbIdx == 1) && ((udbGlobalCfg & (1 << 9)) != 0))
            {
                udbType = SNET_XCAT_PCL_UDB_MUX_VRF_MSB_E;
            }
            break;
        case XCAT_IPCL_KEY_TYPE_L2_IPV6_KEY_LONG_E:
            if ((udbIdx == 11) && ((udbGlobalCfg & (1 << 10)) != 0))
            {
                udbType = SNET_XCAT_PCL_UDB_MUX_VRF_LSB_E;
            }
            if ((udbIdx == 6) && ((udbGlobalCfg & (1 << 11)) != 0))
            {
                udbType = SNET_XCAT_PCL_UDB_MUX_VRF_MSB_E;
            }
            break;
        case XCAT_IPCL_KEY_TYPE_L4_IPV6_KEY_LONG_E:
            if ((udbIdx == 14) && ((udbGlobalCfg & (1 << 12)) != 0))
            {
                udbType = SNET_XCAT_PCL_UDB_MUX_VRF_LSB_E;
            }
            if ((udbIdx == 12) && ((udbGlobalCfg & (1 << 13)) != 0))
            {
                udbType = SNET_XCAT_PCL_UDB_MUX_VRF_MSB_E;
            }
            break;
        case XCAT_IPCL_KEY_TYPE_IPCL0_IPV4_KEY_LONG_E:
            if ((udbIdx == 13) && ((udbGlobalCfg & (1 << 16)) != 0))
            {
                udbType = SNET_XCAT_PCL_UDB_MUX_VRF_LSB_E;
            }
            if ((udbIdx == 14) && ((udbGlobalCfg & (1 << 17)) != 0))
            {
                udbType = SNET_XCAT_PCL_UDB_MUX_VRF_MSB_E;
            }
            break;
        case XCAT_IPCL_KEY_TYPE_IPCL1_IPV4_KEY_SHORT_E:
            if ((udbIdx == 5) && ((udbGlobalCfg & (1 << 20)) != 0))
            {
                udbType = SNET_XCAT_PCL_UDB_MUX_VRF_LSB_E;
            }
            if ((udbIdx == 1) && ((udbGlobalCfg & (1 << 21)) != 0))
            {
                udbType = SNET_XCAT_PCL_UDB_MUX_VRF_MSB_E;
            }
            break;
        case XCAT_IPCL_KEY_TYPE_IPCL0_IPV6_KEY_TRIPLE_E:
            if ((udbIdx == 12) && ((udbGlobalCfg & (1 << 18)) != 0))
            {
                udbType = SNET_XCAT_PCL_UDB_MUX_VRF_LSB_E;
            }
            if ((udbIdx == 0) && ((udbGlobalCfg & (1 << 19)) != 0))
            {
                udbType = SNET_XCAT_PCL_UDB_MUX_VRF_MSB_E;
            }
            break;
        case XCAT_IPCL_KEY_TYPE_IPCL1_IPV6_KEY_TRIPLE_E:
            if ((udbIdx == 18) && ((udbGlobalCfg & (1 << 22)) != 0))
            {
                udbType = SNET_XCAT_PCL_UDB_MUX_VRF_LSB_E;
            }
            if ((udbIdx == 19) && ((udbGlobalCfg & (1 << 23)) != 0))
            {
                udbType = SNET_XCAT_PCL_UDB_MUX_VRF_MSB_E;
            }
            break;
        case XCAT_IPCL_KEY_TYPE_IPV6_DIP_KEY_SHORT_E:
            break;
        case XCAT_IPCL_KEY_TYPE_UDB_KEY_SHORT_E:
            if ((udbIdx == 0) && ((udbGlobalCfg & (1 << 6)) != 0))
            {
                udbType = SNET_XCAT_PCL_UDB_MUX_VRF_LSB_E;
            }
            if ((udbIdx == 1) && ((udbGlobalCfg & (1 << 7)) != 0))
            {
                udbType = SNET_XCAT_PCL_UDB_MUX_VRF_MSB_E;
            }
            if ((udbIdx == 2) && ((udbGlobalCfg & (1 << 24)) != 0))
            {
                udbType = SNET_XCAT_PCL_UDB_MUX_QOS_E;
            }
            break;
        case XCAT_IPCL_KEY_TYPE_UDB_KEY_LONG_E:
            if ((udbIdx == 1) && ((udbGlobalCfg & (1 << 14)) != 0))
            {
                udbType = SNET_XCAT_PCL_UDB_MUX_VRF_LSB_E;
            }
            if ((udbIdx == 2) && ((udbGlobalCfg & (1 << 15)) != 0))
            {
                udbType = SNET_XCAT_PCL_UDB_MUX_VRF_MSB_E;
            }
            if ((udbIdx == 5) && ((udbGlobalCfg & (1 << 25)) != 0))
            {
                udbType = SNET_XCAT_PCL_UDB_MUX_QOS_E;
            }
            break;
        default:
            return GT_FAIL;
    }

getUdbValue_lbl:

    keyIndex = xcatIpclKeyInfoArr[keyType].keyIndex;

    switch(udbType)
    {
        case SNET_XCAT_PCL_UDB_MUX_VRF_LSB_E:
             __LOG(("in key#[%d] : override UDB[%d] with vrfId_or_tag1Info[7:0] \n",
                keyIndex,udbIdx));
            *byteValuePtr = vrfId_or_tag1Info & 0xff;
            return GT_OK;
        case SNET_XCAT_PCL_UDB_MUX_VRF_MSB_E:
             __LOG(("in key#[%d] : override UDB[%d] with vrfId_or_tag1Info[15:8] \n",
                keyIndex,udbIdx));
            *byteValuePtr = (vrfId_or_tag1Info >> 8) & 0xf;
            return GT_OK;
        case SNET_XCAT_PCL_UDB_MUX_QOS_E:
             __LOG(("in key#[%d] : override UDB[%d] with descrPtr->qos.qosProfile[7:0] \n",
                keyIndex,udbIdx));
            *byteValuePtr = descrPtr->qos.qosProfile;
            return GT_OK;
        case SNET_XCAT_PCL_UDB_MUX_TAG1_INFO_E:
             __LOG(("in key#[%d] : override UDB[%d] with {4b0,Tag1 VID[11:8]} \n",
                keyIndex,udbIdx));
            *byteValuePtr = ((descrPtr->vid1 >> 6) & 0x3f);

            if(TAG1_EXIST_MAC(descrPtr))
            {
                *byteValuePtr |= 1 << 7;
            }

            if(descrPtr->cfidei1)
            {
                *byteValuePtr |= 1 << 6;
            }

            return GT_OK;
        case SNET_XCAT_PCL_UDB_MUX_FLOW_ID_BYTE_0_E:
             __LOG(("in key#[%d] : override UDB[%d] with FLOW_ID[7:0] \n",
                keyIndex,udbIdx));
            *byteValuePtr = (descrPtr->flowId >> 0) & 0xff;/* byte 0 */
            return GT_OK;
        case SNET_XCAT_PCL_UDB_MUX_FLOW_ID_BYTE_1_E:
             __LOG(("in key#[%d] : override UDB[%d] with FLOW_ID[15:8] \n",
                keyIndex,udbIdx));
            *byteValuePtr = (descrPtr->flowId >> 8) & 0xff;/* byte 1 */
            return GT_OK;
        default:
            break;
    }

    return snetLionPclUdbKeyValueGet(devObjPtr,descrPtr,
                                     SMAIN_DIRECTION_INGRESS_E,
                                     udbIdx,byteValuePtr);
}

/*******************************************************************************
*   snetXCatIPclBuildBaseCommonKey
*
* DESCRIPTION:
*       Build first common 42 bits of PCL search key.
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
static GT_VOID snetXCatIPclBuildBaseCommonKey
(
    IN SKERNEL_DEVICE_OBJECT                        * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC              * descrPtr,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC              * curLookUpCfgPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC                * pclExtraDataPtr,
    IN SNET_XCAT_IPCL_KEY_TYPE_ENT            keyType,
    OUT SNET_CHT_POLICY_KEY_STC                     * pclKeyPtr
)
{


    GT_U32      srcPort = descrPtr->localDevSrcPort;

    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, curLookUpCfgPtr->pclID ,
                            XCAT_PCL_KEY_FIELDS_ID_PCL_ID_E);/*[9:0] PCL-ID */
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->mac2me,
                            XCAT_PCL_KEY_FIELDS_ID_MAC_TO_ME_E);/*[10:10] mac2me */
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, srcPort ,
                            XCAT_PCL_KEY_FIELDS_ID_SRC_PORT_E);/* [16:11] src port */
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->origSrcTagged,/* [17] is tagged */
                            XCAT_PCL_KEY_FIELDS_ID_IS_TAGGED_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->eVid ,/* [29:18] eVid */
                            XCAT_PCL_KEY_FIELDS_ID_VID_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->up  ,/* [32:30] UP */
                            XCAT_PCL_KEY_FIELDS_ID_UP_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->qos.qosProfile,/* [39:33] QOS profile */
                            XCAT_PCL_KEY_FIELDS_ID_QOS_PROFILE_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->isIPv4 , /* [40] is IPv4 */
                            XCAT_PCL_KEY_FIELDS_ID_IS_IPV4_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->isIp,/* [41] is IP */
                            XCAT_PCL_KEY_FIELDS_ID_IS_IP_E);
}

/*******************************************************************************
*   snetXCatIPclBuildBaseUdbCommonKey
*
* DESCRIPTION:
*       Build common fields of STD_UDB and EXT_UDB PCL search key.
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
static GT_VOID snetXCatIPclBuildBaseUdbCommonKey
(
    IN SKERNEL_DEVICE_OBJECT                        * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC              * descrPtr,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC              * curLookUpCfgPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC                * pclExtraDataPtr,
    OUT SNET_CHT_POLICY_KEY_STC                     * pclKeyPtr
)
{
    DECLARE_FUNC_NAME(snetXCatIPclBuildBaseUdbCommonKey);

    GT_U32 packetTagging0;
    GT_U32 packetTagging1;
    GT_U32 l3OffsetInvalid;
    GT_U32 l4ProtocolorValid;
    GT_U32 packetType;
    GT_U32 l4OffsetInvalid;
    GT_U32 l2Encapsulation;
    GT_U32      srcPort = descrPtr->localDevSrcPort;

    if (descrPtr->origSrcTagged == 0)
    {
        /* untagged packet */
        packetTagging0 = 0;
        packetTagging1 = 0;
    }
    else
    {
        if (descrPtr->srcPriorityTagged == 0)
        {
            /* regular vlan tag */
            __LOG(("regular vlan tag"));
            packetTagging0 = 1;
            packetTagging1 = 1;
        }
        else
        {
            /* user priority tag */
            __LOG(("user priority tag"));
            packetTagging0 = 0;
            packetTagging1 = 1;
        }
    }

    /* This flag indicates that the Layer 3 offset was NOT found successfully */
    l3OffsetInvalid = (descrPtr->l3StartOffsetPtr) ? 0 : 1;

    if (descrPtr->isIp && pclExtraDataPtr->isIpv4Fragment == GT_FALSE)
    {
        switch (descrPtr->ipProt)
        {
            case SNET_TCP_PROT_E:
                l4ProtocolorValid = 1;
                break;
            case SNET_IPV4_ICMP_PROT_E:
                l4ProtocolorValid = 2;
                break;
            case SNET_UDP_PROT_E:
                l4ProtocolorValid = 3;
                break;
            default:
                /* Other/Invalid */
                __LOG(("Other/Invalid"));
                l4ProtocolorValid = 0;
        }

    }
    else
    {
        /* Other/Invalid */
        l4ProtocolorValid = 0;
    }


    /* Applicable Flow Sub Template */
    __LOG(("Applicable Flow Sub Template"));
    snetXcatPclTemplateToKeyConvert(devObjPtr, descrPtr, &packetType);

    /* Layer 4 offset was not found */
    __LOG(("Layer 4 offset was not found"));
    l4OffsetInvalid = (pclExtraDataPtr->isL4Valid) ? 0 : 1;

    l2Encapsulation =
        CONVERT_DESCRIPTOR_L2_ENCAPS_TO_KEY_L2_ENCAPSULATION_MAC(descrPtr->l2Encaps);

    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, curLookUpCfgPtr->pclID ,
                            XCAT_PCL_KEY_FIELDS_ID_PCL_ID_E);/*[9:0] PCL-ID */
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->mac2me,
                            XCAT_PCL_KEY_FIELDS_ID_MAC_TO_ME_E);/*[10:10] mac2me */
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, srcPort ,
                            XCAT_PCL_KEY_FIELDS_ID_SRC_PORT_E);/* [16:11] src port */
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, packetTagging0,/* [17] is tagged */
                            XCAT_PCL_KEY_FIELDS_ID_IS_TAGGED_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->eVid ,/* [29:18] eVid */
                            XCAT_PCL_KEY_FIELDS_ID_VID_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->up  ,/* [32:30] UP */
                            XCAT_PCL_KEY_FIELDS_ID_UP_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, packetTagging1,/* [33:33] PriorityTagged, with VID = 0 */
                            XCAT_PCL_KEY_FIELDS_ID_IS_PRIORITY_TAGGED_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, l3OffsetInvalid,/* [34:34] Valid - Layer 3 offset was found */
                            XCAT_PCL_KEY_FIELDS_ID_L3_OFFSET_INVALID_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, l4ProtocolorValid,/* [35:36] L4 Protocol or Valid */
                            XCAT_PCL_KEY_FIELDS_ID_L4_PROTOCOL_OR_VALID_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpHeaderOk ,/* [39] ip header OK */
                            XCAT_PCL_KEY_FIELDS_ID_IP_HEADER_OK_KEY3_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, (packetType & 0x3),/* [37:38] Applicable Flow Sub Template[1:0] */
                            XCAT_PCL_KEY_FIELDS_ID_APPLICABLE_FLOW_SUB_TEMPLATE_1_0_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->isIp,/* [41] is IP */
                            XCAT_PCL_KEY_FIELDS_ID_IS_IP_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dscp,/* [47:42] dscp */
                            XCAT_PCL_KEY_FIELDS_ID_DSCP_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, (packetType >> 2),/* [88:83] Applicable Flow Sub Template[7:2] */
                            XCAT_PCL_KEY_FIELDS_ID_APPLICABLE_FLOW_SUB_TEMPLATE_7_2_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->macDaType,/* [80:81] MacDaType  */
                            XCAT_PCL_KEY_FIELDS_ID_MAC_DA_TYPE_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, l4OffsetInvalid,/* [82:82] Valid: Layer 4 offset was found */
                            XCAT_PCL_KEY_FIELDS_ID_L4_OFFSET_INVALID_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, l2Encapsulation,/* [89:90] L2 Encapsulation of the packet */
                            XCAT_PCL_KEY_FIELDS_ID_L2_ENCAP_0_KEY3_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpV6EhHopByHop,/* [91:91] Indicates that the IPv6 Original Extension Header is Hop-by-Hop */
                            XCAT_PCL_KEY_FIELDS_ID_IS_IPV6_EH_HOP_BY_HOP_KEY3_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpV6EhExists,/* [92:92] Indicates that an IPv6 extension exists */
                            XCAT_PCL_KEY_FIELDS_ID_IS_IPV6_EH_EXISTS_KEY3_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL2Valid,/* [95:95] Indicates that Layer 2 information in the search key is valid */
                            XCAT_PCL_KEY_FIELDS_ID_IS_L2_VALID_KEY3_E);
}

/*******************************************************************************
*   snetXCatIPclBuildL4ByteOffsetKey
*
* DESCRIPTION:
*       Build L4 bytes offset key.
*
* INPUTS:
*       devObjPtr            - pointer to device object.
*       descrPtr             - pointer to frame data buffer Id.
*       pclExtraDataPtr      - pointer to extra data needed for the IPCL engine.
*       keyIndex             - key index (0..12) - used for SIP5
* OUTPUTS:
*       pclKeyPtr    - pointer to PCL key structure.
*
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID snetXCatIPclBuildL4ByteOffsetKey
(
    IN SKERNEL_DEVICE_OBJECT                        * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC              * descrPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC                * pclExtraDataPtr,
    IN DXCH_IPCL_KEY_ENT                            keyIndex,
    OUT SNET_CHT_POLICY_KEY_STC                     * pclKeyPtr
)
{
    DECLARE_FUNC_NAME(snetXCatIPclBuildL4ByteOffsetKey);

    GT_U8 tmpFieldVal[3];
    GT_U8 *startPtr;/*pointer to L4 byte 2,3 or to 4,5*/

    if (pclExtraDataPtr->isL4Valid == 0 || descrPtr->l4StartOffsetPtr == NULL)
    {
        /* L4 Header Not Valid */
        __LOG(("L4 Header Not Valid"));
        return;
    }

    if ((descrPtr->ipv6Icmp || descrPtr->ipv4Icmp)
        && descrPtr->l4StartOffsetPtr != NULL)
    {
        startPtr = &descrPtr->l4StartOffsetPtr[4];
    }
    else
    {
        startPtr = &descrPtr->l4StartOffsetPtr[2];
    }

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        snetLion3IPclKeyFieldBuildByPointer(pclKeyPtr,startPtr,
            LION3_KEY_FIELD_NAME_BUILD( 64  ,  79   ,L4_Byte_Offset_3_2_OR_L4_Byte_Offset_5_4));
    }
    else
    {
        snetXCatIPclKeyFieldBuildByPointer(pclKeyPtr,startPtr,
            XCAT_PCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_3_2_E);/*     [79:64] L4 Byte Offsets 3:2 / 5:4 */
    }

    if(descrPtr->l4StartOffsetPtr != NULL)
    {
        if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            if(keyIndex != DXCH_IPCL_KEY_1_E)
            {
                snetLion3IPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->l4StartOffsetPtr[0],
                    LION3_KEY2_FIELD_NAME_BUILD(80  ,  87   ,L4_Byte_Offsets_0                ));
                snetLion3IPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->l4StartOffsetPtr[1],
                    LION3_KEY2_FIELD_NAME_BUILD(88  ,  95   ,L4_Byte_Offsets_1                ));
                snetLion3IPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->l4StartOffsetPtr[13],
                    LION3_KEY2_FIELD_NAME_BUILD(96  , 103   ,L4_Byte_Offsets_13               ));
            }
        }
        else
        {
            tmpFieldVal[0] = descrPtr->l4StartOffsetPtr[13];
            tmpFieldVal[1] = descrPtr->l4StartOffsetPtr[1];
            tmpFieldVal[2] = descrPtr->l4StartOffsetPtr[0];
            snetXCatIPclKeyFieldBuildByPointer(
                pclKeyPtr,(GT_U8*)&tmpFieldVal[0],  /* [103:80] L4 byte offsets */
                        XCAT_PCL_KEY_FIELDS_ID_L4_BYTE_OFFSET_13_1_0_E);
        }
    }
}


/*******************************************************************************
*   lion3IPclBuildKeyCommon
*
* DESCRIPTION:
*       Lion3 : Build first common PCL search key.
*
* INPUTS:
*       devObjPtr            - pointer to device object.
*       descrPtr             - pointer to frame data buffer Id.
*       curLookUpCfgPtr - pointer to the current PCL configuration entry.
*       pclExtraDataPtr      - pointer to extra data needed for the IPCL engine.
*       keyIndex             - key index (0..12 (not 3,7) for this key)
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
static GT_VOID lion3IPclBuildKeyCommon
(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC  * curLookUpCfgPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC    * pclExtraDataPtr,
    IN DXCH_IPCL_KEY_ENT                  keyIndex,
    OUT SNET_CHT_POLICY_KEY_STC         * pclKeyPtr
)
{


    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, curLookUpCfgPtr->pclID ,
                            LION3_KEY_FIELD_NAME_BUILD( 0   ,   7   ,PCL_ID_7_0_OR_port_list_7_0      ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, curLookUpCfgPtr->pclID >> 8 ,
                            LION3_KEY_FIELD_NAME_BUILD( 8   ,   9   ,PCL_ID_9_8                       ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->mac2me,
                            LION3_KEY_FIELD_NAME_BUILD( 10  ,  10   ,MAC2ME                           ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->localDevSrcPort ,
                            LION3_KEY_FIELD_NAME_BUILD( 11  ,  16   ,SrcPort_5_0_OR_port_list_13_8    ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->origSrcTagged,
                            LION3_KEY_FIELD_NAME_BUILD( 17  ,  17   ,IsTagged                         ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->eVid ,
                            LION3_KEY_FIELD_NAME_BUILD( 18  ,  29   ,VID0_OR_eVLAN_11_0               ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->up  ,
                            LION3_KEY_FIELD_NAME_BUILD( 30  ,  32   ,UP0                              ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->qos.qosProfile,
                            LION3_KEY_FIELD_NAME_BUILD( 33  ,  39   ,QoS_Profile                      ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->isIPv4 ,
                            LION3_KEY_FIELD_NAME_BUILD( 40  ,  40   ,IsIPv4_OR_IsFCoE                 ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->isIp,
                            LION3_KEY_FIELD_NAME_BUILD( 41  ,  41   ,IsIP                             ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->localDevSrcPort >> 6 ,
                            LION3_KEY_FIELD_NAME_BUILD( 192 , 193   ,port_list_15_14_OR_SrcPort_7_6   ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->srcDev  ,
                            LION3_KEY_FIELD_NAME_BUILD( 194 , 205   ,port_list_27_16_OR_2b0_SrcDev_9_0));
}

/*******************************************************************************
*   lion3IPclBuildBaseKey3And7
*
* DESCRIPTION:
*       Lion3 : Build common fields of keys 3,7 search key.
*
* INPUTS:
*       devObjPtr            - pointer to device object.
*       descrPtr             - pointer to frame data buffer Id.
*       curLookUpCfgPtr - pointer to the current PCL configuration entry.
*       pclExtraDataPtr      - pointer to extra data needed for the IPCL engine.
*       keyIndex             - key index (3 or 7 -  for this key)
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
static GT_VOID lion3IPclBuildBaseKey3And7
(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC  * curLookUpCfgPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC    * pclExtraDataPtr,
    IN DXCH_IPCL_KEY_ENT                  keyIndex,
    OUT SNET_CHT_POLICY_KEY_STC         * pclKeyPtr
)
{
    DECLARE_FUNC_NAME(lion3IPclBuildBaseKey3And7);

    GT_U32 packetTagging0;
    GT_U32 packetTagging1;
    GT_U32 l3OffsetInvalid;
    GT_U32 l4ProtocolorValid;
    GT_U32 packetType;
    GT_U32 l4OffsetInvalid;
    GT_U32 l2Encapsulation;

    if (descrPtr->origSrcTagged == 0)
    {
        /* untagged packet */
        __LOG(("untagged packet"));
        packetTagging0 = 0;
        packetTagging1 = 0;
    }
    else
    {
        if (descrPtr->srcPriorityTagged == 0)
        {
            /* regular vlan tag */
            __LOG(("regular vlan tag"));
            packetTagging0 = 1;
            packetTagging1 = 1;
        }
        else
        {
            /* user priority tag */
            __LOG(("user priority tag"));
            packetTagging0 = 0;
            packetTagging1 = 1;
        }
    }

    /* This flag indicates that the Layer 3 offset was NOT found successfully */
    l3OffsetInvalid = (descrPtr->l3StartOffsetPtr) ? 0 : 1;

    if (descrPtr->isIp && pclExtraDataPtr->isIpv4Fragment == GT_FALSE)
    {
        switch (descrPtr->ipProt)
        {
            case SNET_TCP_PROT_E:
                l4ProtocolorValid = 1;
                break;
            case SNET_IPV4_ICMP_PROT_E:
                l4ProtocolorValid = 2;
                break;
            case SNET_UDP_PROT_E:
                l4ProtocolorValid = 3;
                break;
            default:
                /* Other/Invalid */
                __LOG(("Other/Invalid"));
                l4ProtocolorValid = 0;
                break;
        }

    }
    else
    {
        /* Other/Invalid */
        l4ProtocolorValid = 0;
    }


    /* Applicable Flow Sub Template */
    snetXcatPclTemplateToKeyConvert(devObjPtr, descrPtr, &packetType);

    /* Layer 4 offset was not found */
    l4OffsetInvalid = (pclExtraDataPtr->isL4Valid) ? 0 : 1;

    l2Encapsulation =
        CONVERT_DESCRIPTOR_L2_ENCAPS_TO_KEY_L2_ENCAPSULATION_MAC(descrPtr->l2Encaps);

    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, curLookUpCfgPtr->pclID ,
                            LION3_KEY_FIELD_NAME_BUILD( 0   ,   7   ,PCL_ID_7_0_OR_port_list_7_0      ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, curLookUpCfgPtr->pclID >> 8 ,
                            LION3_KEY_FIELD_NAME_BUILD( 8   ,   9   ,PCL_ID_9_8                       ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->mac2me,
                            LION3_KEY_FIELD_NAME_BUILD( 10  ,  10   ,MAC2ME                           ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->localDevSrcPort ,
                            LION3_KEY_FIELD_NAME_BUILD( 11  ,  16   ,SrcPort_5_0_OR_port_list_13_8    ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, packetTagging0,
                            LION3_KEY_FIELD_NAME_BUILD( 17  ,  17   ,IsTagged                         ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->eVid ,
                            LION3_KEY_FIELD_NAME_BUILD( 18  ,  29   ,VID0_OR_eVLAN_11_0               ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->up  ,
                            LION3_KEY_FIELD_NAME_BUILD( 30  ,  32   ,UP0                              ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, packetTagging1,
                            LION3_KEY3_FIELD_NAME_BUILD(33  , 33    ,Packet_Tagging_1                 ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, l3OffsetInvalid,
                            LION3_KEY3_FIELD_NAME_BUILD(34  , 34    ,L3_Offset_Invalid                ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, l4ProtocolorValid,
                            LION3_KEY3_FIELD_NAME_BUILD(35  , 36    ,L4_Protocol_or_Valid             ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, packetType,
                            LION3_KEY3_FIELD_NAME_BUILD(37  , 38    ,Applicable_Flow_Sub_Template_1_0 ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpHeaderOk ,
                            LION3_KEY3_FIELD_NAME_BUILD(39  , 39    ,IP_Hdr_OK                        ));

    if(keyIndex == DXCH_IPCL_KEY_3_E)
    {
        snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, (descrPtr->isIp && descrPtr->isIPv4) ||descrPtr->isFcoe,
                                LION3_KEY_FIELD_NAME_BUILD( 40  ,  40   ,IsIPv4_OR_IsFCoE                 ));
    }
    else    /*DXCH_IPCL_KEY_7_E*/
    {
        snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, (descrPtr->isIp && (descrPtr->isIPv4 == 0)) ||descrPtr->isFcoe,
                                LION3_KEY4_FIELD_NAME_BUILD(40  ,  40   ,IsIPv6_OR_IsFCoE                 ));
    }

    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->isIp,
                            LION3_KEY_FIELD_NAME_BUILD( 41  ,  41   ,IsIP                             ));
    if( 4 == packetType )
    {
        /* MPLS packet type, outer EXP */
        snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->exp1,
                                LION3_KEY_FIELD_NAME_BUILD( 42  ,  47   ,Packet_DSCP                      ));
    }
    else
    {
        snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dscp,
                                LION3_KEY_FIELD_NAME_BUILD( 42  ,  47   ,Packet_DSCP                      ));
    }

    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->macDaType,
                            LION3_KEY3_FIELD_NAME_BUILD(80  , 81    ,MacDaType                        ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, l4OffsetInvalid,
                            LION3_KEY3_FIELD_NAME_BUILD(82  , 82    ,L4_Offset_Invalid                ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, (packetType >> 2),
                            LION3_KEY3_FIELD_NAME_BUILD(83  , 88    ,Applicable_Flow_Sub_Template_7_2 ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, l2Encapsulation,
                            LION3_KEY3_FIELD_NAME_BUILD(89  , 90    ,L2_Encapsulation                 ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpV6EhHopByHop,
                            LION3_KEY3_FIELD_NAME_BUILD(91  , 91    ,Is_IPv6_EH_Hop_By_Hop            ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpV6EhExists,
                            LION3_KEY3_FIELD_NAME_BUILD(92  , 92    ,Is_IPv6_EH_Exist                 ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL2Valid,
                            LION3_KEY3_FIELD_NAME_BUILD(95  , 95    ,Is_L2_Valid                      ));

    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->localDevSrcPort >> 6 ,
                            LION3_KEY_FIELD_NAME_BUILD( 192 , 193   ,port_list_15_14_OR_SrcPort_7_6   ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->srcDev  ,
                            LION3_KEY_FIELD_NAME_BUILD( 194 , 205   ,port_list_27_16_OR_2b0_SrcDev_9_0));

}


/*******************************************************************************
*   lion3IPclBuildKey0
*
* DESCRIPTION:
*        Lion3 key#0 - IPCL.
*
* INPUTS:
*       devObjPtr            - pointer to device object.
*       descrPtr             - pointer to frame data buffer Id.
*       curLookUpCfgPtr      - pointer to the current PCL configuration entry.
*       pclExtraDataPtr      - pointer to extra data needed for the IPCL engine.
*       keyIndex             - key index (0 for this key)
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
static GT_VOID lion3IPclBuildKey0
(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC  * curLookUpCfgPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC    * pclExtraDataPtr,
    IN DXCH_IPCL_KEY_ENT                  keyIndex,
    OUT SNET_CHT_POLICY_KEY_STC         * pclKeyPtr
)
{
    GT_U32 fieldVal;            /* field value */

    lion3IPclBuildKeyCommon(devObjPtr, descrPtr, curLookUpCfgPtr,
                                   pclExtraDataPtr, keyIndex,pclKeyPtr);

    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->etherTypeOrSsapDsap,
                            LION3_KEY0_FIELD_NAME_BUILD(42  ,  57   ,Ethertype                        ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->vid1,
                            LION3_KEY0_FIELD_NAME_BUILD(58  ,  63   ,Tag1_VID_5_0                     ));
    snetLion3IPclKeyFieldBuildByPointer(pclKeyPtr, &(descrPtr->macSaPtr[3]),
                            LION3_KEY_FIELD_NAME_BUILD( 80  , 103   ,MAC_SA_23_0                      ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->up1,
                            LION3_KEY0_FIELD_NAME_BUILD(104 , 104   ,UP1_0                            ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->arp ,
                            LION3_KEY_FIELD_NAME_BUILD( 105 , 105   ,IsARP                            ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL2Valid ,
                            LION3_KEY_FIELD_NAME_BUILD( 107 , 107   ,Is_L2_Valid                      ));
    fieldVal =
        CONVERT_DESCRIPTOR_L2_ENCAPS_TO_KEY_L2_ENCAPSULATION_MAC(descrPtr->l2Encaps);
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, fieldVal,
                            LION3_KEY0_FIELD_NAME_BUILD(108 , 108   ,L2_Encapsulation_0               ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, fieldVal>>1,
                            LION3_KEY0_FIELD_NAME_BUILD(109 , 109   ,L2_Encapsulation_1               ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->up1 >> 1,
                            LION3_KEY0_FIELD_NAME_BUILD(110 , 111   ,UP1_2_1                          ));
    snetLion3IPclKeyFieldBuildByPointer(pclKeyPtr, &(descrPtr->macSaPtr[0]),
                            LION3_KEY_FIELD_NAME_BUILD( 120 , 143   ,MAC_SA_47_24                     ));
    snetLion3IPclKeyFieldBuildByPointer(pclKeyPtr, descrPtr->macDaPtr,
                            LION3_KEY_FIELD_NAME_BUILD( 144 , 191   ,MAC_DA                           ));
}
/*******************************************************************************
*   lion3IPclBuildKey1
*
* DESCRIPTION:
*        Lion3 key#1 - IPCL.
*
* INPUTS:
*       devObjPtr            - pointer to device object.
*       descrPtr             - pointer to frame data buffer Id.
*       curLookUpCfgPtr      - pointer to the current PCL configuration entry.
*       pclExtraDataPtr      - pointer to extra data needed for the IPCL engine.
*       keyIndex             - key index (1 for this key)
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
static GT_VOID lion3IPclBuildKey1
(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC  * curLookUpCfgPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC    * pclExtraDataPtr,
    IN DXCH_IPCL_KEY_ENT                  keyIndex,
    OUT SNET_CHT_POLICY_KEY_STC         * pclKeyPtr
)
{


    lion3IPclBuildKeyCommon(devObjPtr, descrPtr, curLookUpCfgPtr,
                                   pclExtraDataPtr, keyIndex,pclKeyPtr);

    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dscp,
                            LION3_KEY_FIELD_NAME_BUILD( 42  ,  47   ,Packet_DSCP                      ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->ipProt,
                            LION3_KEY_FIELD_NAME_BUILD( 48  ,  55   ,IpProtocol                       ));
    /* [79:64] L4 Byte Offset 3_2 / 5_4 */
    snetXCatIPclBuildL4ByteOffsetKey(devObjPtr, descrPtr, pclExtraDataPtr, keyIndex,pclKeyPtr);

    snetLion3IPclKeyFieldBuildByPointer(pclKeyPtr, &(descrPtr->macSaPtr[3]),
                            LION3_KEY_FIELD_NAME_BUILD( 80  , 103   ,MAC_SA_23_0                      ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL4Valid,
                            LION3_KEY_FIELD_NAME_BUILD( 104 , 104   ,Is_L4_Valid                      ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->arp ,
                            LION3_KEY_FIELD_NAME_BUILD( 105 , 105   ,IsARP                            ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL2Valid ,
                            LION3_KEY_FIELD_NAME_BUILD( 107 , 107   ,Is_L2_Valid                      ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpv4Fragment ,
                            LION3_KEY_FIELD_NAME_BUILD( 108 , 108   ,IPv4_fragmented                  ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpHeaderOk ,
                            LION3_KEY_FIELD_NAME_BUILD( 109 , 109   ,IP_Hdr_OK                        ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpV6EhExists,
                            LION3_KEY1_FIELD_NAME_BUILD(110 , 110   ,Is_IPv6_EH_Exist                 ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpV6EhHopByHop,
                            LION3_KEY1_FIELD_NAME_BUILD(111 , 111   ,Is_IPv6_EH_Hop_By_Hop            ));
    snetLion3IPclKeyFieldBuildByPointer(pclKeyPtr, &(descrPtr->macSaPtr[0]),
                            LION3_KEY_FIELD_NAME_BUILD( 120 , 143   ,MAC_SA_47_24                     ));
    snetLion3IPclKeyFieldBuildByPointer(pclKeyPtr, descrPtr->macDaPtr,
                            LION3_KEY_FIELD_NAME_BUILD( 144 , 191   ,MAC_DA                           ));
}

/*******************************************************************************
*   lion3IPclBuildKey2
*
* DESCRIPTION:
*        Lion3 key#2 - IPCL.
*
* INPUTS:
*       devObjPtr            - pointer to device object.
*       descrPtr             - pointer to frame data buffer Id.
*       curLookUpCfgPtr      - pointer to the current PCL configuration entry.
*       pclExtraDataPtr      - pointer to extra data needed for the IPCL engine.
*       keyIndex             - key index (2 for this key)
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
static GT_VOID lion3IPclBuildKey2
(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC  * curLookUpCfgPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC    * pclExtraDataPtr,
    IN DXCH_IPCL_KEY_ENT                  keyIndex,
    OUT SNET_CHT_POLICY_KEY_STC         * pclKeyPtr
)
{
    GT_U32 fieldVal;            /* field value */

    lion3IPclBuildKeyCommon(devObjPtr, descrPtr, curLookUpCfgPtr,
                                   pclExtraDataPtr, keyIndex,pclKeyPtr);

    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dscp,
                            LION3_KEY_FIELD_NAME_BUILD( 42  ,  47   ,Packet_DSCP                      ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->ipProt,
                            LION3_KEY_FIELD_NAME_BUILD( 48  ,  55   ,IpProtocol                       ));

    /* [79:64] L4 Byte Offset 3_2 / 5_4 + [103:80] L4 byte offsets*/
    snetXCatIPclBuildL4ByteOffsetKey(devObjPtr, descrPtr, pclExtraDataPtr,keyIndex, pclKeyPtr);

    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL4Valid,
                            LION3_KEY_FIELD_NAME_BUILD( 104 , 104   ,Is_L4_Valid                      ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->arp ,
                            LION3_KEY_FIELD_NAME_BUILD( 105 , 105   ,IsARP                            ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL2Valid ,
                            LION3_KEY_FIELD_NAME_BUILD( 107 , 107   ,Is_L2_Valid                      ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpv4Fragment ,
                            LION3_KEY_FIELD_NAME_BUILD( 108 , 108   ,IPv4_fragmented                  ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpHeaderOk ,
                            LION3_KEY_FIELD_NAME_BUILD( 109 , 109   ,IP_Hdr_OK                        ));

    fieldVal = (descrPtr->macDaType == SKERNEL_BROADCAST_MAC_E ||
                descrPtr->macDaType == SKERNEL_BROADCAST_ARP_E) ? 1 : 0;

    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, fieldVal ,
                            LION3_KEY2_FIELD_NAME_BUILD(110 , 110   ,Is_BC                            ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[0],
                             LION3_KEY2_FIELD_NAME_BUILD(112 , 143   ,SIP_31_0_OR_S_ID                 ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[0],
                             LION3_KEY2_FIELD_NAME_BUILD(144 , 175   ,DIP_31_0_OR_D_ID                 ));
}

/*******************************************************************************
*   lion3IPclBuildKey3
*
* DESCRIPTION:
*        Lion3 key#3 - IPCL.
*
* INPUTS:
*       devObjPtr            - pointer to device object.
*       descrPtr             - pointer to frame data buffer Id.
*       curLookUpCfgPtr      - pointer to the current PCL configuration entry.
*       pclExtraDataPtr      - pointer to extra data needed for the IPCL engine.
*       keyIndex             - key index (3 for this key)
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
static GT_VOID lion3IPclBuildKey3
(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC  * curLookUpCfgPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC    * pclExtraDataPtr,
    IN DXCH_IPCL_KEY_ENT                  keyIndex,
    OUT SNET_CHT_POLICY_KEY_STC         * pclKeyPtr
)
{
    lion3IPclBuildBaseKey3And7(devObjPtr, descrPtr, curLookUpCfgPtr,
                                   pclExtraDataPtr, keyIndex, pclKeyPtr);
}

/*******************************************************************************
*   lion3IPclBuildKey4
*
* DESCRIPTION:
*        Lion3 key#4 - IPCL.
*
* INPUTS:
*       devObjPtr            - pointer to device object.
*       descrPtr             - pointer to frame data buffer Id.
*       curLookUpCfgPtr      - pointer to the current PCL configuration entry.
*       pclExtraDataPtr      - pointer to extra data needed for the IPCL engine.
*       keyIndex             - key index (4 for this key)
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
static GT_VOID lion3IPclBuildKey4
(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC  * curLookUpCfgPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC    * pclExtraDataPtr,
    IN DXCH_IPCL_KEY_ENT                  keyIndex,
    OUT SNET_CHT_POLICY_KEY_STC         * pclKeyPtr
)
{
    GT_U32 fieldVal;            /* field value */

    lion3IPclBuildKeyCommon(devObjPtr, descrPtr, curLookUpCfgPtr,
                                   pclExtraDataPtr, keyIndex,pclKeyPtr);

    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, (descrPtr->isIp && (descrPtr->isIPv4 == 0)) ||descrPtr->isFcoe,
                            LION3_KEY4_FIELD_NAME_BUILD(40  ,  40   ,IsIPv6_OR_IsFCoE                 ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dscp,
                            LION3_KEY_FIELD_NAME_BUILD( 42  ,  47   ,Packet_DSCP                      ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->ipProt,
                            LION3_KEY_FIELD_NAME_BUILD( 48  ,  55   ,IpProtocol                       ));

    /* [79:64] L4 Byte Offset 3_2 / 5_4 + [103:80] L4 byte offsets*/
    snetXCatIPclBuildL4ByteOffsetKey(devObjPtr, descrPtr, pclExtraDataPtr, keyIndex, pclKeyPtr);


    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL4Valid,
                            LION3_KEY_FIELD_NAME_BUILD( 104 , 104   ,Is_L4_Valid                      ));
    fieldVal = CONVERT_DESCRIPTOR_L2_ENCAPS_TO_KEY_L2_ENCAPSULATION_MAC(descrPtr->l2Encaps);
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, fieldVal,
                            LION3_KEY4_FIELD_NAME_BUILD(105 , 105   ,L2_Encapsulation_0               ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL2Valid,
                            LION3_KEY_FIELD_NAME_BUILD( 107 , 107   ,Is_L2_Valid                      ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpv4Fragment,
                            LION3_KEY_FIELD_NAME_BUILD( 108 , 108   ,IPv4_fragmented                  ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpHeaderOk ,
                            LION3_KEY_FIELD_NAME_BUILD( 109 , 109   ,IP_Hdr_OK                        ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, fieldVal >> 1,
                            LION3_KEY4_FIELD_NAME_BUILD(110 , 110   ,L2_Encapsulation_1               ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[0],
                            LION3_KEY2_FIELD_NAME_BUILD(112 , 143   ,SIP_31_0_OR_S_ID                 ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[0],
                            LION3_KEY2_FIELD_NAME_BUILD(144 , 175   ,DIP_31_0_OR_D_ID                 ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->etherTypeOrSsapDsap,
                            LION3_KEY4_FIELD_NAME_BUILD(176 , 191   ,Ethertype                        ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->up1,
                            LION3_KEY4_FIELD_NAME_BUILD(208 , 210   ,UP1                              ));
    snetLion3IPclKeyFieldBuildByPointer(pclKeyPtr, descrPtr->macSaPtr,
                            LION3_KEY4_FIELD_NAME_BUILD(264 , 311   ,MAC_SA                           ));
    snetLion3IPclKeyFieldBuildByPointer(pclKeyPtr, descrPtr->macDaPtr,
                            LION3_KEY4_FIELD_NAME_BUILD(312 , 359   ,MAC_DA                           ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr,TAG1_EXIST_MAC(descrPtr),
                            LION3_KEY4_FIELD_NAME_BUILD(400 , 400   ,Tag1_Exist                       ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->vid1,
                            LION3_KEY4_FIELD_NAME_BUILD(401 , 412   ,Tag1_VID                         ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->cfidei1,
                            LION3_KEY4_FIELD_NAME_BUILD(413 , 413   ,Tag1_CFI                         ));
}

/*******************************************************************************
*   lion3IPclBuildKey5
*
* DESCRIPTION:
*        Lion3 key#5 - IPCL.
*
* INPUTS:
*       devObjPtr            - pointer to device object.
*       descrPtr             - pointer to frame data buffer Id.
*       curLookUpCfgPtr      - pointer to the current PCL configuration entry.
*       pclExtraDataPtr      - pointer to extra data needed for the IPCL engine.
*       keyIndex             - key index (5 for this key)
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
static GT_VOID lion3IPclBuildKey5
(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC  * curLookUpCfgPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC    * pclExtraDataPtr,
    IN DXCH_IPCL_KEY_ENT                  keyIndex,
    OUT SNET_CHT_POLICY_KEY_STC         * pclKeyPtr
)
{


    lion3IPclBuildKeyCommon(devObjPtr, descrPtr, curLookUpCfgPtr,
                                   pclExtraDataPtr, keyIndex,pclKeyPtr);

    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, (descrPtr->isIp && (descrPtr->isIPv4 == 0)) ||descrPtr->isFcoe,
                            LION3_KEY4_FIELD_NAME_BUILD(40  ,  40   ,IsIPv6_OR_IsFCoE                 ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dscp,
                            LION3_KEY_FIELD_NAME_BUILD( 42  ,  47   ,Packet_DSCP                      ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->ipProt,
                            LION3_KEY_FIELD_NAME_BUILD( 48  ,  55   ,IpProtocol                       ));

    /* [79:64] L4 Byte Offset 3_2 / 5_4 + [103:80] L4 byte offsets*/
    snetXCatIPclBuildL4ByteOffsetKey(devObjPtr, descrPtr, pclExtraDataPtr,keyIndex, pclKeyPtr);

    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL4Valid,
                            LION3_KEY_FIELD_NAME_BUILD( 104 , 104   ,Is_L4_Valid                      ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpV6EhExists,
                            LION3_KEY5_FIELD_NAME_BUILD(105 , 105   ,Is_IPv6_EH_Exist                 ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL2Valid,
                            LION3_KEY_FIELD_NAME_BUILD( 107 , 107   ,Is_L2_Valid                      ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpV6EhHopByHop,
                            LION3_KEY5_FIELD_NAME_BUILD(108 , 108   ,Is_IPv6_EH_Hop_By_Hop            ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpHeaderOk ,
                            LION3_KEY_FIELD_NAME_BUILD( 109 , 109   ,IP_Hdr_OK                        ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[3],
                            LION3_KEY2_FIELD_NAME_BUILD(112 , 143   ,SIP_31_0_OR_S_ID                 ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[2],
                            LION3_KEY5_FIELD_NAME_BUILD(144 , 175   ,SIP_63_32                        ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[1],
                            LION3_KEY5_FIELD_NAME_BUILD(176 , 191   ,SIP_79_64                        ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[1] >> 16,
                            LION3_KEY5_FIELD_NAME_BUILD(208 , 223   ,SIP_95_80                        ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[0],
                            LION3_KEY5_FIELD_NAME_BUILD(224 , 255   ,SIP_127_96                       ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[0] >> 24,
                            LION3_KEY5_FIELD_NAME_BUILD(256 , 263   ,DIP_127_120                      ));
    snetLion3IPclKeyFieldBuildByPointer(pclKeyPtr, descrPtr->macSaPtr,
                            LION3_KEY4_FIELD_NAME_BUILD(264 , 311   ,MAC_SA                           ));
    snetLion3IPclKeyFieldBuildByPointer(pclKeyPtr, descrPtr->macDaPtr,
                            LION3_KEY4_FIELD_NAME_BUILD(312 , 359   ,MAC_DA                           ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->up1,
                            LION3_KEY5_FIELD_NAME_BUILD(110 , 111   ,UP1_1_0                          ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->up1 >> 2,
                            LION3_KEY5_FIELD_NAME_BUILD(413 , 413   ,UP1_2                            ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr,TAG1_EXIST_MAC(descrPtr),
                            LION3_KEY4_FIELD_NAME_BUILD(400 , 400   ,Tag1_Exist                       ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->vid1,
                            LION3_KEY4_FIELD_NAME_BUILD(401 , 412   ,Tag1_VID                         ));
}

/*******************************************************************************
*   lion3IPclBuildKey6
*
* DESCRIPTION:
*        Lion3 key#6 - IPCL.
*
* INPUTS:
*       devObjPtr            - pointer to device object.
*       descrPtr             - pointer to frame data buffer Id.
*       curLookUpCfgPtr      - pointer to the current PCL configuration entry.
*       pclExtraDataPtr      - pointer to extra data needed for the IPCL engine.
*       keyIndex             - key index (6 for this key)
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
static GT_VOID lion3IPclBuildKey6
(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC  * curLookUpCfgPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC    * pclExtraDataPtr,
    IN DXCH_IPCL_KEY_ENT                  keyIndex,
    OUT SNET_CHT_POLICY_KEY_STC         * pclKeyPtr
)
{


    lion3IPclBuildKeyCommon(devObjPtr, descrPtr, curLookUpCfgPtr,
                                   pclExtraDataPtr, keyIndex,pclKeyPtr);

    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, (descrPtr->isIp && (descrPtr->isIPv4 == 0)) ||descrPtr->isFcoe,
                            LION3_KEY4_FIELD_NAME_BUILD(40  ,  40   ,IsIPv6_OR_IsFCoE                 ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dscp,
                            LION3_KEY_FIELD_NAME_BUILD( 42  ,  47   ,Packet_DSCP                      ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->ipProt,
                            LION3_KEY_FIELD_NAME_BUILD( 48  ,  55   ,IpProtocol                       ));

    /* [79:64] L4 Byte Offset 3_2 / 5_4 + [103:80] L4 byte offsets*/
    snetXCatIPclBuildL4ByteOffsetKey(devObjPtr, descrPtr, pclExtraDataPtr, keyIndex, pclKeyPtr);

    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL4Valid,
                            LION3_KEY_FIELD_NAME_BUILD( 104 , 104   ,Is_L4_Valid                      ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpV6EhExists,
                            LION3_KEY5_FIELD_NAME_BUILD(105 , 105   ,Is_IPv6_EH_Exist                 ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL2Valid,
                            LION3_KEY_FIELD_NAME_BUILD( 107 , 107   ,Is_L2_Valid                      ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpV6EhHopByHop,
                            LION3_KEY5_FIELD_NAME_BUILD(108 , 108   ,Is_IPv6_EH_Hop_By_Hop            ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpHeaderOk ,
                            LION3_KEY_FIELD_NAME_BUILD( 109 , 109   ,IP_Hdr_OK                        ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[3],
                            LION3_KEY2_FIELD_NAME_BUILD(112 , 143   ,SIP_31_0_OR_S_ID                 ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[3],
                            LION3_KEY2_FIELD_NAME_BUILD(144 , 175   ,DIP_31_0_OR_D_ID                 ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[2],
                            LION3_KEY6_FIELD_NAME_BUILD(176 ,  191  ,SIP_47_32                        ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[2] >> 16,
                            LION3_KEY6_FIELD_NAME_BUILD(208 ,  223  ,SIP_63_48                        ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[1],
                            LION3_KEY6_FIELD_NAME_BUILD(224 ,  255  ,SIP_95_64                        ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[0],
                            LION3_KEY6_FIELD_NAME_BUILD(256 ,  287  ,SIP_127_96                       ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, (descrPtr->dip[0] >> 24),
                            LION3_KEY6_FIELD_NAME_BUILD(288 ,  295  ,DIP_127_120                      ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[2],
                            LION3_KEY6_FIELD_NAME_BUILD(296 ,  327  ,DIP_63_32                        ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[1],
                            LION3_KEY6_FIELD_NAME_BUILD(328 ,  359  ,DIP_95_64                        ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[0],
                            LION3_KEY6_FIELD_NAME_BUILD(360 ,  383  ,DIP_119_96                       ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->up1,
                            LION3_KEY5_FIELD_NAME_BUILD(110 , 111   ,UP1_1_0                          ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->up1 >> 2,
                            LION3_KEY5_FIELD_NAME_BUILD(413 , 413   ,UP1_2                            ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr,TAG1_EXIST_MAC(descrPtr),
                            LION3_KEY4_FIELD_NAME_BUILD(400 , 400   ,Tag1_Exist                       ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->vid1,
                            LION3_KEY4_FIELD_NAME_BUILD(401 , 412   ,Tag1_VID                         ));
}

/*******************************************************************************
*   lion3IPclBuildKey7
*
* DESCRIPTION:
*        Lion3 key#7 - IPCL.
*
* INPUTS:
*       devObjPtr            - pointer to device object.
*       descrPtr             - pointer to frame data buffer Id.
*       curLookUpCfgPtr      - pointer to the current PCL configuration entry.
*       pclExtraDataPtr      - pointer to extra data needed for the IPCL engine.
*       keyIndex             - key index (3 for this key)
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
static GT_VOID lion3IPclBuildKey7
(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC  * curLookUpCfgPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC    * pclExtraDataPtr,
    IN DXCH_IPCL_KEY_ENT                  keyIndex,
    OUT SNET_CHT_POLICY_KEY_STC         * pclKeyPtr
)
{
    lion3IPclBuildBaseKey3And7(devObjPtr, descrPtr, curLookUpCfgPtr,
                                   pclExtraDataPtr, keyIndex, pclKeyPtr);

    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->ipProt,
                            LION3_KEY_FIELD_NAME_BUILD( 48  ,  55   ,IpProtocol                       ));

    if(descrPtr->isIp && (descrPtr->isIPv4 == 0))
    {
        /* IPV6 */
        snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[3],
                                LION3_KEY7_FIELD_NAME_BUILD(144 , 175   ,SIP_31_0_OR_S_ID                 ));
        snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[2],
                                LION3_KEY6_FIELD_NAME_BUILD(176 ,  191  ,SIP_47_32                        ));
        snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[2] >> 16,
                                LION3_KEY6_FIELD_NAME_BUILD(208 ,  223  ,SIP_63_48                        ));
        snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[1],
                                LION3_KEY6_FIELD_NAME_BUILD(224 ,  255  ,SIP_95_64                        ));
        snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[0],
                                 LION3_KEY6_FIELD_NAME_BUILD(256 ,  287  ,SIP_127_96                       ));
        snetLion3IPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->dip[0] >> 16,
                                LION3_KEY7_FIELD_NAME_BUILD(288 , 303   ,DIP_127_112                      ));
        snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[3],
                                 LION3_KEY7_FIELD_NAME_BUILD(304 , 335   ,DIP_31_0_OR_D_ID                 ));
    }
    else  /*non ipv6*/
    {
        snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[0],
                                LION3_KEY7_FIELD_NAME_BUILD(144 , 175   ,SIP_31_0_OR_S_ID                 ));
        snetLion3IPclKeyFieldBuildByPointer(pclKeyPtr, &(descrPtr->macSaPtr[4]),
                                LION3_KEY7_FIELD_NAME_BUILD(176 , 191   ,MAC_SA_15_0                      ));
        snetLion3IPclKeyFieldBuildByPointer(pclKeyPtr, &(descrPtr->macSaPtr[0]),
                                LION3_KEY7_FIELD_NAME_BUILD(208 , 239   ,MAC_SA_47_32                     ));
        snetLion3IPclKeyFieldBuildByPointer(pclKeyPtr, descrPtr->macDaPtr,
                                LION3_KEY7_FIELD_NAME_BUILD(240 , 287   ,MAC_DA                           ));
        snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[0],
                                 LION3_KEY7_FIELD_NAME_BUILD(304 , 335   ,DIP_31_0_OR_D_ID                 ));
    }

    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->up1,
                            LION3_KEY7_FIELD_NAME_BUILD(397 , 399   ,UP1                              ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr,TAG1_EXIST_MAC(descrPtr),
                            LION3_KEY4_FIELD_NAME_BUILD(400 , 400   ,Tag1_Exist                       ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->vid1,
                            LION3_KEY4_FIELD_NAME_BUILD(401 , 412   ,Tag1_VID                         ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->cfidei1,
                            LION3_KEY4_FIELD_NAME_BUILD(413 , 413   ,Tag1_CFI                         ));

}

/*******************************************************************************
*   lion3IPclBuildKey8
*
* DESCRIPTION:
*        Lion3 key#8 - IPCL.
*
* INPUTS:
*       devObjPtr            - pointer to device object.
*       descrPtr             - pointer to frame data buffer Id.
*       curLookUpCfgPtr      - pointer to the current PCL configuration entry.
*       pclExtraDataPtr      - pointer to extra data needed for the IPCL engine.
*       keyIndex             - key index (8 for this key)
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
static GT_VOID lion3IPclBuildKey8
(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC  * curLookUpCfgPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC    * pclExtraDataPtr,
    IN DXCH_IPCL_KEY_ENT                  keyIndex,
    OUT SNET_CHT_POLICY_KEY_STC         * pclKeyPtr
)
{
    GT_U32 fieldVal;            /* field value */
    GT_U32 PacketTOS_OR_DSCP = descrPtr->l3StartOffsetPtr[1];

    lion3IPclBuildKeyCommon(devObjPtr, descrPtr, curLookUpCfgPtr,
                                   pclExtraDataPtr, keyIndex,pclKeyPtr);

    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, PacketTOS_OR_DSCP>>2,
                            LION3_KEY8_FIELD_NAME_BUILD(33  ,  38   ,PacketTOS_OR_DSCP_7_2            ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, PacketTOS_OR_DSCP,
                            LION3_KEY8_FIELD_NAME_BUILD(50  ,  50   ,PacketTOS_OR_DSCP_0              ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, PacketTOS_OR_DSCP >> 1,
                            LION3_KEY8_FIELD_NAME_BUILD(58  ,  58   ,PacketTOS_DSCP_1                 ));

    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->arp ,
                            LION3_KEY8_FIELD_NAME_BUILD(39  ,  39   ,Is_ARP                           ));
    fieldVal = (descrPtr->macDaType == SKERNEL_BROADCAST_MAC_E ||
              descrPtr->macDaType == SKERNEL_BROADCAST_ARP_E) ? 1 : 0;
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, fieldVal,
                            LION3_KEY8_FIELD_NAME_BUILD(40  ,  40   ,Is_BC                            ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->ipProt,
                            LION3_KEY8_FIELD_NAME_BUILD(42  ,  49   ,IpProtocol                       ));

    VALUE_IPV4_FRAGMENT_MAC(pclExtraDataPtr,fieldVal)
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, fieldVal,
                            LION3_KEY8_FIELD_NAME_BUILD(51  ,  52   ,IP_Fragmented                    ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->ipv4HeaderInfo,
                            LION3_KEY8_FIELD_NAME_BUILD(53  ,  54   ,IP_Header_Info                   ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL2Valid ,
                            LION3_KEY8_FIELD_NAME_BUILD(55  ,  55   ,Is_L2_Valid                      ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL4Valid,
                            LION3_KEY8_FIELD_NAME_BUILD(56  ,  56   ,Is_L4_Valid                      ));

    /* [79:64] L4 Byte Offset 3_2 / 5_4 + [103:80] L4 byte offsets*/
    snetXCatIPclBuildL4ByteOffsetKey(devObjPtr, descrPtr, pclExtraDataPtr, keyIndex, pclKeyPtr);

    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[0],
                            LION3_KEY2_FIELD_NAME_BUILD(112 , 143   ,SIP_31_0_OR_S_ID                 ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[0],
                            LION3_KEY2_FIELD_NAME_BUILD(144 , 175   ,DIP_31_0_OR_D_ID                 ));
    snetLion3IPclKeyFieldBuildByPointer(pclKeyPtr, &descrPtr->macSaPtr[4],
                            LION3_KEY7_FIELD_NAME_BUILD(176 , 191   ,MAC_SA_15_0                      ));
    snetLion3IPclKeyFieldBuildByPointer(pclKeyPtr, &descrPtr->macSaPtr[0],
                            LION3_KEY7_FIELD_NAME_BUILD(208 , 239   ,MAC_SA_47_32                     ));
    snetLion3IPclKeyFieldBuildByPointer(pclKeyPtr, descrPtr->macDaPtr,
                            LION3_KEY7_FIELD_NAME_BUILD(240 , 287   ,MAC_DA                           ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->etherTypeOrSsapDsap,
                            LION3_KEY8_FIELD_NAME_BUILD(288 , 303   ,Ethertype                        ));
    fieldVal = CONVERT_DESCRIPTOR_L2_ENCAPS_TO_KEY_L2_ENCAPSULATION_MAC(descrPtr->l2Encaps);
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, fieldVal,
                            LION3_KEY8_FIELD_NAME_BUILD(304 , 304   ,L2_Encapsulation_0               ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, fieldVal>>1,
                            LION3_KEY8_FIELD_NAME_BUILD(329 , 329   ,L2_Encapsulation_1               ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->up1,
                            LION3_KEY8_FIELD_NAME_BUILD(333 , 335   ,UP1                              ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr,TAG1_EXIST_MAC(descrPtr),
                            LION3_KEY8_FIELD_NAME_BUILD(336 , 336   ,Tag1_Exist                       ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->vid1,
                            LION3_KEY8_FIELD_NAME_BUILD(337 , 348   ,Tag1_VID                         ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->cfidei1,
                            LION3_KEY8_FIELD_NAME_BUILD(349 , 349   ,Tag1_CFI                         ));
}

/*******************************************************************************
*   lion3IPclBuildKey9
*
* DESCRIPTION:
*        Lion3 key#9 - IPCL.
*
* INPUTS:
*       devObjPtr            - pointer to device object.
*       descrPtr             - pointer to frame data buffer Id.
*       curLookUpCfgPtr      - pointer to the current PCL configuration entry.
*       pclExtraDataPtr      - pointer to extra data needed for the IPCL engine.
*       keyIndex             - key index (9 for this key)
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
static GT_VOID lion3IPclBuildKey9
(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC  * curLookUpCfgPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC    * pclExtraDataPtr,
    IN DXCH_IPCL_KEY_ENT                  keyIndex,
    OUT SNET_CHT_POLICY_KEY_STC         * pclKeyPtr
)
{
    GT_U32 fieldVal;            /* field value */
    GT_U32 PacketTOS_OR_DSCP = descrPtr->l3StartOffsetPtr[1];

    lion3IPclBuildKeyCommon(devObjPtr, descrPtr, curLookUpCfgPtr,
                                   pclExtraDataPtr, keyIndex,pclKeyPtr);

    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, PacketTOS_OR_DSCP>>2,
                            LION3_KEY8_FIELD_NAME_BUILD(33  ,  38   ,PacketTOS_OR_DSCP_7_2            ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, PacketTOS_OR_DSCP,
                            LION3_KEY8_FIELD_NAME_BUILD(50  ,  50   ,PacketTOS_OR_DSCP_0              ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, PacketTOS_OR_DSCP >> 1,
                            LION3_KEY8_FIELD_NAME_BUILD(58  ,  58   ,PacketTOS_DSCP_1                 ));

    fieldVal = CONVERT_DESCRIPTOR_L2_ENCAPS_TO_KEY_L2_ENCAPSULATION_MAC(descrPtr->l2Encaps);
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, fieldVal,
                            LION3_KEY9_FIELD_NAME_BUILD(39  ,  39   ,L2_Encapsulation_0               ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, fieldVal>>1,
                            LION3_KEY9_FIELD_NAME_BUILD(59  ,  59   ,L2_Encapsulation_1               ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, fieldVal,
                            LION3_KEY8_FIELD_NAME_BUILD(40  ,  40   ,Is_BC                            ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->ipProt,
                            LION3_KEY8_FIELD_NAME_BUILD(42  ,  49   ,IpProtocol                       ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpV6EhHopByHop,
                            LION3_KEY9_FIELD_NAME_BUILD(51  ,  51   ,Is_IPv6_EH_Hop_By_Hop            ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpV6EhExists,
                            LION3_KEY9_FIELD_NAME_BUILD(52  ,  52   ,Is_IPv6_EH_Exist                 ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->solicitationMcastMsg,
                            LION3_KEY9_FIELD_NAME_BUILD(53  ,  53   ,Is_ND                            ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpHeaderOk ,
                            LION3_KEY9_FIELD_NAME_BUILD(54  ,  54   ,IP_Hdr_OK                        ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL2Valid ,
                            LION3_KEY8_FIELD_NAME_BUILD(55  ,  55   ,Is_L2_Valid                      ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL4Valid,
                            LION3_KEY8_FIELD_NAME_BUILD(56  ,  56   ,Is_L4_Valid                      ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->origSrcEPortOrTrnk,
        LION3_KEY9_FIELD_NAME_BUILD(60  ,  63   ,SRCePort_OR_Trunk_3_0            ));

    /* [79:64] L4 Byte Offset 3_2 / 5_4 + [103:80] L4 byte offsets*/
    snetXCatIPclBuildL4ByteOffsetKey(devObjPtr, descrPtr, pclExtraDataPtr, keyIndex,pclKeyPtr);

    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[0],
                            LION3_KEY2_FIELD_NAME_BUILD(112 , 143   ,SIP_31_0_OR_S_ID                 ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[0],
                            LION3_KEY2_FIELD_NAME_BUILD(144 , 175   ,DIP_31_0_OR_D_ID                 ));
    snetLion3IPclKeyFieldBuildByPointer(pclKeyPtr, &descrPtr->macSaPtr[4],
                            LION3_KEY7_FIELD_NAME_BUILD(176 , 191   ,MAC_SA_15_0                      ));
    snetLion3IPclKeyFieldBuildByPointer(pclKeyPtr, &descrPtr->macSaPtr[0],
                            LION3_KEY7_FIELD_NAME_BUILD(208 , 239   ,MAC_SA_47_32                     ));
    snetLion3IPclKeyFieldBuildByPointer(pclKeyPtr, descrPtr->macDaPtr,
                            LION3_KEY7_FIELD_NAME_BUILD(240 , 287   ,MAC_DA                           ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->etherTypeOrSsapDsap,
                            LION3_KEY8_FIELD_NAME_BUILD(288 , 303   ,Ethertype                        ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[2],
                            LION3_KEY9_FIELD_NAME_BUILD(304 , 335   ,SIP_63_32                        ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[1],
                            LION3_KEY9_FIELD_NAME_BUILD(336 , 367   ,SIP_95_64                        ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[0],
                            LION3_KEY9_FIELD_NAME_BUILD(368 , 399   ,SIP_127_96                       ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->origSrcEPortOrTrnk >> 4,
                            LION3_KEY9_FIELD_NAME_BUILD(400 , 407   ,SRCePort_OR_Trunk_11_4           ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->origIsTrunk,
                            LION3_KEY9_FIELD_NAME_BUILD(408 , 408   ,OrigSrcIsTrunk                   ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->eVid >> 12,
                            LION3_KEY9_FIELD_NAME_BUILD(409 , 409   ,eVLAN_12                         ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[2],
                            LION3_KEY9_FIELD_NAME_BUILD(416 , 447   ,DIP_63_32                        ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[1],
                            LION3_KEY9_FIELD_NAME_BUILD(448 , 479   ,DIP_95_64                        ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[0],
                            LION3_KEY9_FIELD_NAME_BUILD(480 , 511   ,DIP_127_96                       ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->up1,
                            LION3_KEY9_FIELD_NAME_BUILD(411 , 413   ,UP1                              ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr,TAG1_EXIST_MAC(descrPtr),
                            LION3_KEY9_FIELD_NAME_BUILD(608 , 608   ,Tag1_Exist                       ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->vid1,
                            LION3_KEY9_FIELD_NAME_BUILD(609 , 620   ,Tag1_VID                         ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->cfidei1,
                            LION3_KEY9_FIELD_NAME_BUILD(621 , 621   ,Tag1_CFI                         ));
}

/*******************************************************************************
*   lion3IPclBuildKey10
*
* DESCRIPTION:
*        Lion3 key#10 - IPCL.
*
* INPUTS:
*       devObjPtr            - pointer to device object.
*       descrPtr             - pointer to frame data buffer Id.
*       curLookUpCfgPtr      - pointer to the current PCL configuration entry.
*       pclExtraDataPtr      - pointer to extra data needed for the IPCL engine.
*       keyIndex             - key index (10 for this key)
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
static GT_VOID lion3IPclBuildKey10
(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC  * curLookUpCfgPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC    * pclExtraDataPtr,
    IN DXCH_IPCL_KEY_ENT                  keyIndex,
    OUT SNET_CHT_POLICY_KEY_STC         * pclKeyPtr
)
{
    DECLARE_FUNC_NAME(lion3IPclBuildKey10);

    GT_U32 fieldVal;            /* field value */
    GT_U32 PacketTOS_OR_DSCP = descrPtr->l3StartOffsetPtr[1];
    GT_U32      l3TotalLength = descrPtr->ipxLength;      /* Layer 3 total length */
    GT_U32      keyPacketType;      /* Packet type for the key */

    lion3IPclBuildKeyCommon(devObjPtr, descrPtr, curLookUpCfgPtr,
                                   pclExtraDataPtr, keyIndex,pclKeyPtr);

    if(descrPtr->mpls)
    {
        snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->exp1,
                LION3_KEY10_FIELD_NAME_BUILD(30  ,  32   ,UP0_OR_EXP0                     ));
    }

    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, PacketTOS_OR_DSCP>>2,
                            LION3_KEY8_FIELD_NAME_BUILD(33  ,  38   ,PacketTOS_OR_DSCP_7_2            ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, PacketTOS_OR_DSCP,
                            LION3_KEY8_FIELD_NAME_BUILD(50  ,  50   ,PacketTOS_OR_DSCP_0              ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, PacketTOS_OR_DSCP >> 1,
                            LION3_KEY10_FIELD_NAME_BUILD(63  ,  63   ,PacketTOS_DSCP_1                ));

    /* get Packet type for the key */
    snetXcatPclKeyPacketTypeGet(devObjPtr, descrPtr, &keyPacketType,GT_FALSE/*passenger*/);
    __LOG_PARAM(keyPacketType);

    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, keyPacketType,/* [41:39] Packet type */
                            LION3_KEY10_FIELD_NAME_BUILD(39  ,  41   ,Pkt_type                        ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->ipProt,
                            LION3_KEY8_FIELD_NAME_BUILD(42  ,  49   ,IpProtocol                       ));
    VALUE_IPV4_FRAGMENT_MAC(pclExtraDataPtr,fieldVal)
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, fieldVal,
                            LION3_KEY8_FIELD_NAME_BUILD(51  ,  52   ,IP_Fragmented                    ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->ipv4HeaderInfo,
                            LION3_KEY8_FIELD_NAME_BUILD(53  ,  54   ,IP_Header_Info                   ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL4Valid,
                            LION3_KEY8_FIELD_NAME_BUILD(56  ,  56   ,Is_L4_Valid                      ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, l3TotalLength,
                            LION3_KEY10_FIELD_NAME_BUILD(57  ,  62   ,IP_Packet_Length_5_0            ));

    /* [79:64] L4 Byte Offset 3_2 / 5_4 + [103:80] L4 byte offsets*/
    snetXCatIPclBuildL4ByteOffsetKey(devObjPtr, descrPtr, pclExtraDataPtr, keyIndex, pclKeyPtr);

    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[0],
                             LION3_KEY2_FIELD_NAME_BUILD(112 , 143   ,SIP_31_0_OR_S_ID                 ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[0],
                             LION3_KEY2_FIELD_NAME_BUILD(144 , 175   ,DIP_31_0_OR_D_ID                 ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, l3TotalLength >> 6,
                            LION3_KEY10_FIELD_NAME_BUILD(176 , 183   ,IP_Packet_Length_13_6           ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->ttl,
                            LION3_KEY10_FIELD_NAME_BUILD(184 , 191   ,TTL                             ));
}

/*******************************************************************************
*   lion3IPclBuildKey11
*
* DESCRIPTION:
*        Lion3 key#11 - IPCL.
*
* INPUTS:
*       devObjPtr            - pointer to device object.
*       descrPtr             - pointer to frame data buffer Id.
*       curLookUpCfgPtr      - pointer to the current PCL configuration entry.
*       pclExtraDataPtr      - pointer to extra data needed for the IPCL engine.
*       keyIndex             - key index (11 for this key)
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
static GT_VOID lion3IPclBuildKey11
(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC  * curLookUpCfgPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC    * pclExtraDataPtr,
    IN DXCH_IPCL_KEY_ENT                  keyIndex,
    OUT SNET_CHT_POLICY_KEY_STC         * pclKeyPtr
)
{
    DECLARE_FUNC_NAME(lion3IPclBuildKey11);

    GT_U32 PacketTOS_OR_DSCP = descrPtr->l3StartOffsetPtr[1];
    GT_U32 l3TotalLength = descrPtr->ipxLength;      /* Layer 3 total length */
    GT_U32      keyPacketType;      /* Packet type for the key */

    lion3IPclBuildKeyCommon(devObjPtr, descrPtr, curLookUpCfgPtr,
                                   pclExtraDataPtr, keyIndex,pclKeyPtr);

    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, PacketTOS_OR_DSCP>>2,
                            LION3_KEY8_FIELD_NAME_BUILD(33  ,  38   ,PacketTOS_OR_DSCP_7_2            ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, PacketTOS_OR_DSCP,
                            LION3_KEY8_FIELD_NAME_BUILD(50  ,  50   ,PacketTOS_OR_DSCP_0              ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, PacketTOS_OR_DSCP>> 1,
                            LION3_KEY11_FIELD_NAME_BUILD(232 , 232   ,PacketTOS_DSCP_1                ));

    /* get Packet type for the key */
    snetXcatPclKeyPacketTypeGet(devObjPtr, descrPtr, &keyPacketType,GT_FALSE/*passenger*/);
    __LOG_PARAM(keyPacketType);

    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, keyPacketType,/* [41:39] Packet type */
                            LION3_KEY10_FIELD_NAME_BUILD(39  ,  41   ,Pkt_type                        ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->ipProt,
                            LION3_KEY8_FIELD_NAME_BUILD(42  ,  49   ,IpProtocol                       ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, l3TotalLength,
                            LION3_KEY11_FIELD_NAME_BUILD(51  ,  54   ,IP_Packet_Length_3_0            ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL4Valid,
                            LION3_KEY8_FIELD_NAME_BUILD(56  ,  56   ,Is_L4_Valid                      ));

    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->origSrcEPortOrTrnk,
                            LION3_KEY11_FIELD_NAME_BUILD(58  ,  63   ,SRCePort_OR_Trunk_5_0           ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->origSrcEPortOrTrnk>>6,
                            LION3_KEY11_FIELD_NAME_BUILD(399 , 405   ,SRCePort_OR_Trunk_12_6          ));


    /* [79:64] L4 Byte Offset 3_2 / 5_4 + [103:80] L4 byte offsets*/
    snetXCatIPclBuildL4ByteOffsetKey(devObjPtr, descrPtr, pclExtraDataPtr, keyIndex, pclKeyPtr);

    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[3],
                             LION3_KEY2_FIELD_NAME_BUILD(112 , 143   ,SIP_31_0_OR_S_ID                 ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[3],
                             LION3_KEY2_FIELD_NAME_BUILD(144 , 175   ,DIP_31_0_OR_D_ID                 ));

    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->solicitationMcastMsg,
                            LION3_KEY11_FIELD_NAME_BUILD(233 , 233   ,Is_ND                           ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->flowLabel,
                            LION3_KEY11_FIELD_NAME_BUILD(234 , 253   ,IPv6_Flow_Label                 ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->ipXMcLinkLocalProt,
                            LION3_KEY11_FIELD_NAME_BUILD(254 , 254   ,Is_IPv6_Link_local              ));

    if(descrPtr->tti_pcktType_sip5 == SIP5_PACKET_CLASSIFICATION_TYPE_MPLS_E)
    {
        /*the TTI classified as mpls tunnel */
        snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->origInfoBeforeTunnelTermination.origMplsOuterLabel,
                                LION3_KEY11_FIELD_NAME_BUILD(255 , 274   ,MPLS_Outer_Label                ));
        snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->origInfoBeforeTunnelTermination.origMplsOuterLabel>>20,
                                LION3_KEY11_FIELD_NAME_BUILD(275 , 277   ,MPLS_Outer_Label_EXP            ));
        snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->origInfoBeforeTunnelTermination.origMplsOuterLabel>>23,
                                LION3_KEY11_FIELD_NAME_BUILD(278 , 278   ,MPLS_Outer_Label_S_Bit          ));
    }

    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, l3TotalLength >> 4,
                            LION3_KEY11_FIELD_NAME_BUILD(279 , 286   ,IP_Packet_Length_11_4           ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->ttl,
                            LION3_KEY11_FIELD_NAME_BUILD(287 , 294   ,TTL                             ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[2],
                            LION3_KEY11_FIELD_NAME_BUILD(295 , 326   ,SIP_63_32                       ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[1],
                            LION3_KEY11_FIELD_NAME_BUILD(327 , 358   ,SIP_95_64                       ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[0],
                            LION3_KEY11_FIELD_NAME_BUILD(359 , 390   ,SIP_127_96                      ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[2],
                            LION3_KEY11_FIELD_NAME_BUILD(391 , 398   ,DIP_39_32                       ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->origIsTrunk,
                            LION3_KEY11_FIELD_NAME_BUILD(407 , 407   ,OrigSrcIsTrunk                  ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->eVid>>12,
                            LION3_KEY11_FIELD_NAME_BUILD(408 , 408   ,eVLAN_12                        ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr,TAG1_EXIST_MAC(descrPtr),
                            LION3_KEY11_FIELD_NAME_BUILD(409 , 409   ,Tag1_Src_Tagged                 ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->up1,
                            LION3_KEY11_FIELD_NAME_BUILD(410 , 412   ,UP1                             ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[2]>>8,
                            LION3_KEY11_FIELD_NAME_BUILD(416 , 447   ,DIP_63_40                       ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[1],
                            LION3_KEY11_FIELD_NAME_BUILD(448 , 479   ,DIP_95_64                       ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[0],
                            LION3_KEY11_FIELD_NAME_BUILD(480 , 503   ,DIP_127_96                      ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, l3TotalLength >> 12,
                            LION3_KEY11_FIELD_NAME_BUILD(504 , 507   ,IP_Packet_Length_15_12          ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->isIpv6Mld,
                            LION3_KEY11_FIELD_NAME_BUILD(509 , 509   ,Is_IPv6_MLD                     ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpV6EhHopByHop,
                            LION3_KEY11_FIELD_NAME_BUILD(510 , 510   ,Is_IPv6_EH_Hop_By_Hop           ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpV6EhExists,
                            LION3_KEY11_FIELD_NAME_BUILD(511 , 511   ,Is_IPv6_EH_Exist                ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpHeaderOk ,
                            LION3_KEY11_FIELD_NAME_BUILD(512 , 512   ,IP_Hdr_OK                       ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->vid1,
                            LION3_KEY9_FIELD_NAME_BUILD(609 , 620   ,Tag1_VID                         ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->cfidei1,
                            LION3_KEY9_FIELD_NAME_BUILD(621 , 621   ,Tag1_CFI                         ));

}
/*******************************************************************************
*   lion3IPclBuildKey12
*
* DESCRIPTION:
*        Lion3 key#12 - IPCL.
*
* INPUTS:
*       devObjPtr            - pointer to device object.
*       descrPtr             - pointer to frame data buffer Id.
*       curLookUpCfgPtr      - pointer to the current PCL configuration entry.
*       pclExtraDataPtr      - pointer to extra data needed for the IPCL engine.
*       keyIndex             - key index (12 for this key)
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
static GT_VOID lion3IPclBuildKey12
(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC  * curLookUpCfgPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC    * pclExtraDataPtr,
    IN DXCH_IPCL_KEY_ENT                  keyIndex,
    OUT SNET_CHT_POLICY_KEY_STC         * pclKeyPtr
)
{


    lion3IPclBuildKeyCommon(devObjPtr, descrPtr, curLookUpCfgPtr,
                                   pclExtraDataPtr, keyIndex,pclKeyPtr);

    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->ipProt,
                            LION3_KEY8_FIELD_NAME_BUILD(42  ,  49   ,IpProtocol                       ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dscp,
                            LION3_KEY12_FIELD_NAME_BUILD(50  ,  55   ,DSCP                            ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL4Valid,
                            LION3_KEY8_FIELD_NAME_BUILD(56  ,  56   ,Is_L4_Valid                      ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[3],
                            LION3_KEY12_FIELD_NAME_BUILD(57  ,  72   ,DIP_15_0                        ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->arp ,
                            LION3_KEY12_FIELD_NAME_BUILD(73  ,  73   ,IsARP                           ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpV6EhExists,
                            LION3_KEY12_FIELD_NAME_BUILD(74  ,  74   ,Ipv6_EH_exist                   ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[3] >> 16,
                            LION3_KEY12_FIELD_NAME_BUILD(75  ,  90   ,DIP_31_16                       ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpV6EhHopByHop,
                            LION3_KEY3_FIELD_NAME_BUILD(91  , 91    ,Is_IPv6_EH_Hop_By_Hop            ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[2],
                            LION3_KEY12_FIELD_NAME_BUILD(92  , 123   ,DIP_63_32                       ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[1],
                            LION3_KEY12_FIELD_NAME_BUILD(124 , 155   ,DIP_95_64                       ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[0],
                            LION3_KEY12_FIELD_NAME_BUILD(156 , 187   ,DIP_127_96                      ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL2Valid ,
                            LION3_KEY12_FIELD_NAME_BUILD(189 , 189   ,isL2Valid                       ));
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpHeaderOk ,
                            LION3_KEY12_FIELD_NAME_BUILD(191 , 191   ,IP_Header_OK                    ));
}

/*******************************************************************************
*   snetXCatIPclBuildL2StandardKey
*
* DESCRIPTION:
*        Build Standard (24B) L2 - Ingress standard pcl search key.
*
* INPUTS:
*       devObjPtr            - pointer to device object.
*       descrPtr             - pointer to frame data buffer Id.
*       curLookUpCfgPtr      - pointer to the current PCL configuration entry.
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
static GT_VOID snetXCatIPclBuildL2StandardKey
(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC  * curLookUpCfgPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC    * pclExtraDataPtr,
    IN SNET_XCAT_IPCL_KEY_TYPE_ENT            keyType,
    OUT SNET_CHT_POLICY_KEY_STC         * pclKeyPtr
)
{


    GT_U32 fieldVal;            /* field value */
    GT_U8  byteValue;           /* value of user defined byte from the packets or tcp/udp compare */
    GT_U32 userDefError = 0;    /* is there user defined byte error */
    GT_STATUS rc;

    snetXCatIPclBuildBaseCommonKey(devObjPtr, descrPtr, curLookUpCfgPtr,
                                   pclExtraDataPtr, keyType,pclKeyPtr);

    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->etherTypeOrSsapDsap,/* [57:42] */
                            XCAT_PCL_KEY_FIELDS_ID_KEY0_ETHER_TYPE_DSAP_SSAP_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, 0,  /*[63:58] reserved field*/
                            XCAT_PCL_KEY_FIELDS_ID_RESERVED_58_63_E);


    rc = snetXCatPclUdbKeyValueGet(devObjPtr, descrPtr,
                                   XCAT_IPCL_KEY_TYPE_L2_KEY_SHORT_E,
                                   15, &byteValue);
    if(rc == GT_FAIL)
    {
        userDefError = 1;
    }

    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [71:64] UserDefinedByte15 */
                            XCAT_PCL_KEY_FIELDS_ID_KEY0_UDB_15_E);

    rc = snetXCatPclUdbKeyValueGet(devObjPtr, descrPtr,
                                   XCAT_IPCL_KEY_TYPE_L2_KEY_SHORT_E,
                                   16, &byteValue);
    if(rc == GT_FAIL)
    {
        userDefError = 1;
    }

    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [72:79] UserDefinedByte16 */
                            XCAT_PCL_KEY_FIELDS_ID_KEY0_UDB_16_E);

    snetXCatIPclKeyFieldBuildByPointer(pclKeyPtr, &(descrPtr->macSaPtr[3]),/* [103:80] MAC SA*/
                            XCAT_PCL_KEY_FIELDS_ID_STD_KEY_MAC_SA_23_0_E);


    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->arp ,/* [105] isArp */
                            XCAT_PCL_KEY_FIELDS_ID_STD_KEY_IS_ARP_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL2Valid ,/* [107] isL2Valid  */
                            XCAT_PCL_KEY_FIELDS_ID_IS_L2_VALID_E);

    fieldVal =
        CONVERT_DESCRIPTOR_L2_ENCAPS_TO_KEY_L2_ENCAPSULATION_MAC(descrPtr->l2Encaps);

    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, fieldVal,/* [108] Encap Type */
                            XCAT_PCL_KEY_FIELDS_ID_KEY0_ENCAP_TYPE_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, 0 ,/* [111:109] reserved  */
                            XCAT_PCL_KEY_FIELDS_ID_RESERVED_109_111_XCAT2_110_111_E);
    rc = snetXCatPclUdbKeyValueGet(devObjPtr, descrPtr,
                                   XCAT_IPCL_KEY_TYPE_L2_KEY_SHORT_E,
                                   17, &byteValue);
    if(rc == GT_FAIL)
    {
        userDefError = 1;
    }

    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [119:112] UserDefinedByte16 */
                            XCAT_PCL_KEY_FIELDS_ID_KEY0_UDB_17_E);
    snetXCatIPclKeyFieldBuildByPointer(pclKeyPtr, &(descrPtr->macSaPtr[0]),/* [143:120] MAC SA*/
                            XCAT_PCL_KEY_FIELDS_ID_STD_KEY_MAC_SA_47_24_E);
    snetXCatIPclKeyFieldBuildByPointer(pclKeyPtr, descrPtr->macDaPtr,/* [191:144] MAC DA*/
                            XCAT_PCL_KEY_FIELDS_ID_STD_KEY_MAC_DA_E);

    /* must be checked after all the UDB validity checks */
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, CH3_UDB_VALID_MAC(userDefError), /* [106] userDefinedValid */
                            XCAT_PCL_KEY_FIELDS_ID_USER_DEFINED_VALID_E);
}

/*******************************************************************************
*   snetXCatIPclBuildL2L3StandardKey
*
* DESCRIPTION:
*        Build Layer2+IPv4/v6+QoS standard PCL search key.
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
static GT_VOID snetXCatIPclBuildL2L3StandardKey
(
    IN SKERNEL_DEVICE_OBJECT                        * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC              * descrPtr,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC              * curLookUpCfgPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC                * pclExtraDataPtr,
    IN SNET_XCAT_IPCL_KEY_TYPE_ENT            keyType,
    OUT SNET_CHT_POLICY_KEY_STC                     * pclKeyPtr
)
{
    GT_U8       byteValue;          /* value of user defined byte from the
                                       packets or tcp/udp compare */
    GT_U32      userDefError = 0;   /* is there user defined byte error */
    GT_STATUS   rc;                 /* status from called function */


    snetXCatIPclBuildBaseCommonKey(devObjPtr, descrPtr, curLookUpCfgPtr,
                                   pclExtraDataPtr, keyType,pclKeyPtr);

    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dscp,/* [47:42] dscp */
                            XCAT_PCL_KEY_FIELDS_ID_DSCP_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->ipProt,/* [55:48] IP Protocol*/
                            XCAT_PCL_KEY_FIELDS_ID_IP_PROTOCOL_E);

    rc = snetXCatPclUdbKeyValueGet(devObjPtr, descrPtr,
                                   XCAT_IPCL_KEY_TYPE_L2_L3_KEY_SHORT_E,
                                   18, &byteValue);
    if(rc == GT_FAIL)
    {
        userDefError = 1;
    }

    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [63:56] UserDefinedByte18 */
                            XCAT_PCL_KEY_FIELDS_ID_UDB_18_E);

    snetXCatIPclBuildL4ByteOffsetKey(devObjPtr, descrPtr, pclExtraDataPtr , 0 /*don't care*/, pclKeyPtr); /* [79:64] L4 Byte Offset 3_2 +
                                                                                          [103:80] L4 byte offsets*/

    snetXCatIPclKeyFieldBuildByPointer(pclKeyPtr, &(descrPtr->macSaPtr[3]),/* [103:80] MAC SA (overrides previously set of [103:80] L4 byte offsets) */
                            XCAT_PCL_KEY_FIELDS_ID_STD_KEY_MAC_SA_23_0_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL4Valid, /* [104] isL4Valid*/
                            XCAT_PCL_KEY_FIELDS_ID_IS_L4_VALID_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->arp , /* [105] isArp */
                            XCAT_PCL_KEY_FIELDS_ID_STD_KEY_IS_ARP_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL2Valid , /* [107] isL2Valid  */
                            XCAT_PCL_KEY_FIELDS_ID_IS_L2_VALID_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpv4Fragment , /* [108] IPv4Fragmented  */
                            XCAT_PCL_KEY_FIELDS_ID_IPV4_FRAGMENT_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpHeaderOk ,
                            XCAT_PCL_KEY_FIELDS_ID_IP_HEADER_OK_E);/* [109] ip header ok */
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpV6EhExists,/* [110] ipv6_EH_exist */
                            XCAT_PCL_KEY_FIELDS_ID_IS_IPV6_EH_EXISTS_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpV6EhHopByHop,/* [111] ipv6_EH_exist */
                            XCAT_PCL_KEY_FIELDS_ID_LONG_IS_IPV6_EH_HOP_BY_HOP_E);

    rc = snetXCatPclUdbKeyValueGet(devObjPtr, descrPtr,
                                   XCAT_IPCL_KEY_TYPE_L2_L3_KEY_SHORT_E,
                                   19, &byteValue);
    if(rc == GT_FAIL)
    {
        userDefError = 1;
    }

    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [119:112] UserDefinedByte16 */
                            XCAT_PCL_KEY_FIELDS_ID_UDB_19_E);
    snetXCatIPclKeyFieldBuildByPointer(pclKeyPtr, &(descrPtr->macSaPtr[0]),/* [143:120] MAC SA*/
                            XCAT_PCL_KEY_FIELDS_ID_STD_KEY_MAC_SA_47_24_E);
    snetXCatIPclKeyFieldBuildByPointer(pclKeyPtr, descrPtr->macDaPtr,/* [191:144] MAC DA*/
                            XCAT_PCL_KEY_FIELDS_ID_STD_KEY_MAC_DA_E);

    /* must be checked after all the UDB validity checks */
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, CH3_UDB_VALID_MAC(userDefError), /* [106] userDefinedValid */
                            XCAT_PCL_KEY_FIELDS_ID_USER_DEFINED_VALID_E);
}

/*******************************************************************************
*   snetXCatIPclBuildL3L4StandardKey
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
static GT_VOID snetXCatIPclBuildL3L4StandardKey
(
    IN SKERNEL_DEVICE_OBJECT                        * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC              * descrPtr,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC              * curLookUpCfgPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC                * pclExtraDataPtr,
    IN SNET_XCAT_IPCL_KEY_TYPE_ENT            keyType,
    OUT SNET_CHT_POLICY_KEY_STC                     * pclKeyPtr
)
{
    DECLARE_FUNC_NAME(snetXCatIPclBuildL3L4StandardKey);

    GT_U8                   byteValue;      /* value of user defined byte from the
                                               packets or tcp/udp compare */
    GT_U32                  userDefError = 0;
    GT_STATUS               rc;             /* status from called function */
    GT_U32                  fieldVal;       /* field value */

    snetXCatIPclBuildBaseCommonKey(devObjPtr, descrPtr, curLookUpCfgPtr,
                                   pclExtraDataPtr, keyType,pclKeyPtr);

    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dscp,/* [47:42] dscp */
                            XCAT_PCL_KEY_FIELDS_ID_DSCP_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->ipProt,/* [55:48] IP Protocol*/
                            XCAT_PCL_KEY_FIELDS_ID_IP_PROTOCOL_E);

    rc = snetXCatPclUdbKeyValueGet(devObjPtr, descrPtr,
                                   XCAT_IPCL_KEY_TYPE_L3_L4_KEY_SHORT_E,
                                   20, &byteValue);
    if(rc == GT_FAIL)
    {
        userDefError = 1;
    }

    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [63:56] UserDefinedByte18 */
                            XCAT_PCL_KEY_FIELDS_ID_UDB_18_E);

    snetXCatIPclBuildL4ByteOffsetKey(devObjPtr, descrPtr, pclExtraDataPtr , 0 /*don't care*/, pclKeyPtr); /* [79:64] L4 Byte Offset 3_2 +
                                                                                          [103:80] L4 byte offsets*/

    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL4Valid, /* [104] isL4Valid*/
                            XCAT_PCL_KEY_FIELDS_ID_IS_L4_VALID_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->arp ,/* [105] isArp */
                            XCAT_PCL_KEY_FIELDS_ID_STD_KEY_IS_ARP_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL2Valid ,/* [107] isL2Valid  */
                            XCAT_PCL_KEY_FIELDS_ID_IS_L2_VALID_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpv4Fragment ,/* [108] ipv4 fragmented  */
                            XCAT_PCL_KEY_FIELDS_ID_IPV4_FRAGMENT_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpHeaderOk ,/* [109] ip header  */
                            XCAT_PCL_KEY_FIELDS_ID_IP_HEADER_OK_E);

    fieldVal = (descrPtr->macDaType == SKERNEL_BROADCAST_MAC_E ||
                descrPtr->macDaType == SKERNEL_BROADCAST_ARP_E) ? 1 : 0;

    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, fieldVal ,/* [110] is bc */
                            XCAT_PCL_KEY_FIELDS_ID_IS_BC_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, 0, /* [111] reserved */
                            XCAT_PCL_KEY_FIELDS_ID_RESERVED_111_E);
    /* for IPv4 packets only the first index sip[0] or dip[0] is relevant */
    __LOG(("for IPv4 packets only the first index sip[0] or dip[0] is relevant"));
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[0], /* [143:112] SIP*/
                             XCAT_PCL_KEY_FIELDS_ID_SIP_31_0_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[0], /* [175:144] DIP*/
                             XCAT_PCL_KEY_FIELDS_ID_DIP_31_0_E);

    rc = snetXCatPclUdbKeyValueGet(devObjPtr, descrPtr,
                                   XCAT_IPCL_KEY_TYPE_L3_L4_KEY_SHORT_E,
                                   21, &byteValue);
    if(rc == GT_FAIL)
    {
        userDefError = 1;
    }

    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [183:176] UserDefinedByte21 */
                            XCAT_PCL_KEY_FIELDS_ID_UDB_21_KEY2_E);

    rc = snetXCatPclUdbKeyValueGet(devObjPtr, descrPtr,
                                   XCAT_IPCL_KEY_TYPE_L3_L4_KEY_SHORT_E,
                                   22, &byteValue);
    if(rc == GT_FAIL)
    {
        userDefError = 1;
    }

    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [191:184] UserDefinedByte22 */
                            XCAT_PCL_KEY_FIELDS_ID_UDB_22_KEY2_E);


    /* must be checked after all the UDB validity checks */
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, CH3_UDB_VALID_MAC(userDefError),/* [106] user defined valid */
                            XCAT_PCL_KEY_FIELDS_ID_USER_DEFINED_VALID_E);
}

/*******************************************************************************
*   snetXCatIPclBuildL2L3L4ExtendedKey
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
static GT_VOID snetXCatIPclBuildL2L3L4ExtendedKey
(
    IN SKERNEL_DEVICE_OBJECT                        * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC              * descrPtr,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC              * curLookUpCfgPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC                * pclExtraDataPtr,
    IN SNET_XCAT_IPCL_KEY_TYPE_ENT            keyType,
    OUT SNET_CHT_POLICY_KEY_STC                     * pclKeyPtr
)
{
    DECLARE_FUNC_NAME(snetXCatIPclBuildL2L3L4ExtendedKey);

    GT_U8       byteValue;          /* value of user defined byte from the packets or tcp/udp compare */
    GT_U32      userDefError = 0;   /* is there user defined byte error */
    GT_STATUS   rc;                 /* status from called function */
    GT_U32      fieldVal;           /* field value */
    GT_U8       tmpFieldVal[6];
    GT_U32      i;

    snetXCatIPclBuildBaseCommonKey(devObjPtr, descrPtr, curLookUpCfgPtr,
                                   pclExtraDataPtr, keyType,pclKeyPtr);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, (descrPtr->isIp && descrPtr->isIPv4 == 0), /* [40] is IPv6 */
                            XCAT_PCL_KEY_FIELDS_ID_IS_IPV6_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dscp,/* [47:42] dscp */
                            XCAT_PCL_KEY_FIELDS_ID_DSCP_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->ipProt,/* [55:48] IP Protocol*/
                            XCAT_PCL_KEY_FIELDS_ID_IP_PROTOCOL_E);

    rc = snetXCatPclUdbKeyValueGet(devObjPtr, descrPtr,
                                   XCAT_IPCL_KEY_TYPE_L2_L3_L4_KEY_LONG_E,
                                   5, &byteValue);
    if(rc == GT_FAIL)
    {
        userDefError = 1;
    }

    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [63:56] UserDefinedByte22 */
                            XCAT_PCL_KEY_FIELDS_ID_UDB_5_E);

    snetXCatIPclBuildL4ByteOffsetKey(devObjPtr, descrPtr, pclExtraDataPtr , 0 /*don't care*/, pclKeyPtr); /* [79:64] L4 Byte Offset 3_2 +
                                                                                          [103:80] L4 byte offsets*/

    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL4Valid, /* [104] isL4Valid*/
                            XCAT_PCL_KEY_FIELDS_ID_IS_L4_VALID_E);

    fieldVal = CONVERT_DESCRIPTOR_L2_ENCAPS_TO_KEY_L2_ENCAPSULATION_MAC(descrPtr->l2Encaps);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, fieldVal,/* [105] Encap Type */
                        XCAT_PCL_KEY_FIELDS_ID_ENCAP_TYPE_LONG_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL2Valid,/* is L2 valid [107] */
                        XCAT_PCL_KEY_FIELDS_ID_IS_L2_VALID_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpv4Fragment,/* [108] */
                        XCAT_PCL_KEY_FIELDS_ID_IPV4_FRAGMENT_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpHeaderOk ,
                        XCAT_PCL_KEY_FIELDS_ID_IP_HEADER_OK_E);/* [109] ip header ok */

    if (devObjPtr->iPclKeyFormatVersion)
    {
        fieldVal = (descrPtr->l2Encaps == SKERNEL_LLC_SNAP_E) ? 1 : 0;
        snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, fieldVal,/* [110:110] Encap Type - bit[1] */
                                XCAT2_PCL_KEY_FIELDS_ID_ENCAP_1_KEY4_E);
        snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, 0,/* Reserved [111:111] */
                            XCAT_PCL_KEY_FIELDS_ID_RESERVED_111_E);
    }
    else
    {
        snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, 0,/* Reserved [111:110] */
                            XCAT_PCL_KEY_FIELDS_ID_RESERVED_110_111_E);
    }

    /* for IPv4 packets only the first byte sip[0] or dip[0] is relevant */
    __LOG(("for IPv4 packets only the first byte sip[0] or dip[0] is relevant"));
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[0], /* [143:112] SIP*/
                        XCAT_PCL_KEY_FIELDS_ID_SIP_31_0_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[0], /* [175:144] DIP*/
                        XCAT_PCL_KEY_FIELDS_ID_DIP_31_0_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->etherTypeOrSsapDsap,/* [191:176] */
                        XCAT_PCL_KEY_FIELDS_ID_ETHERTYPE_E);

    snetXCatIPclKeyFieldBuildByPointer(pclKeyPtr, descrPtr->macSaPtr,/* MAC SA*/
                        XCAT_PCL_KEY_FIELDS_ID_MAC_SA_E);
    snetXCatIPclKeyFieldBuildByPointer(pclKeyPtr, descrPtr->macDaPtr,/* MAC DA*/
                        XCAT_PCL_KEY_FIELDS_ID_MAC_DA_KEY4_E);

    rc = snetXCatPclUdbKeyValueGet(devObjPtr, descrPtr,
                                   XCAT_IPCL_KEY_TYPE_L2_L3_L4_KEY_LONG_E,
                                   1, &byteValue);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/*  UserDefinedByte22 */
                        XCAT_PCL_KEY_FIELDS_ID_UDB_1_E);

    for (i = 0; i < 4; i++)
    {
        /* UDBs 0,4,3,2 in order 0,1,2,3 */
        rc = snetXCatPclUdbKeyValueGet(
            devObjPtr, descrPtr,
            XCAT_IPCL_KEY_TYPE_L2_L3_L4_KEY_LONG_E,
            ((i == 0) ? 0 : (5 - i)),
            &tmpFieldVal[i]);
    }

    snetXCatIPclKeyFieldBuildByPointer(pclKeyPtr, (GT_U8*)&tmpFieldVal[0],/* [359:383] UserDefinedByte 2,3,4,0 */
                            XCAT_PCL_KEY_FIELDS_ID_UDB_0_4_3_2_E);

    /* must be checked after all the UDB validity checks */
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, CH3_UDB_VALID_MAC(userDefError),/* [106] UserDefineValid */
                        XCAT_PCL_KEY_FIELDS_ID_USER_DEFINED_VALID_E);
}

/*******************************************************************************
*   snetXCatIPclKeyBuildL2IPv6ExtendedKey
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
static GT_VOID snetXCatIPclKeyBuildL2IPv6ExtendedKey
(
    IN SKERNEL_DEVICE_OBJECT                        * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC              * descrPtr,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC             * curLookUpCfgPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC                * pclExtraDataPtr,
    IN SNET_XCAT_IPCL_KEY_TYPE_ENT            keyType,
    OUT SNET_CHT_POLICY_KEY_STC                     * pclKeyPtr
)
{
    GT_U8       byteValue;          /* value of user defined byte from the packets or tcp/udp compare */
    GT_U32      userDefError = 0;   /* is there user defined byte error */
    GT_STATUS   rc;                 /* return code from function */
    GT_U8       tmpFieldVal[6];
    GT_U32      i;


    snetXCatIPclBuildBaseCommonKey(devObjPtr, descrPtr, curLookUpCfgPtr,
                                   pclExtraDataPtr, keyType,pclKeyPtr);

    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, (descrPtr->isIp && descrPtr->isIPv4 == 0), /* [40] is IPv6 */
                            XCAT_PCL_KEY_FIELDS_ID_IS_IPV6_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dscp,/* [47:42] dscp */
                            XCAT_PCL_KEY_FIELDS_ID_DSCP_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->ipProt,/* [55:48] IP Protocol*/
                            XCAT_PCL_KEY_FIELDS_ID_IP_PROTOCOL_E);

    rc = snetXCatPclUdbKeyValueGet(devObjPtr, descrPtr,
                                   XCAT_IPCL_KEY_TYPE_L2_IPV6_KEY_LONG_E,
                                   11, &byteValue);
    if(rc == GT_FAIL)
    {
        userDefError = 1;
    }

    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [63:56] UserDefinedByte22 */
                            XCAT_PCL_KEY_FIELDS_ID_UDB_11_E);


    snetXCatIPclBuildL4ByteOffsetKey(devObjPtr, descrPtr, pclExtraDataPtr , 0 /*don't care*/, pclKeyPtr); /* [79:64] L4 Byte Offset 3_2 +
                                                                                          [103:80] L4 byte offsets*/
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL4Valid, /* [104] isL4Valid*/
                            XCAT_PCL_KEY_FIELDS_ID_IS_L4_VALID_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpV6EhExists, /* [105] isL4Valid*/
                            XCAT_PCL_KEY_FIELDS_ID_IS_IPV6_EH_EXISTS_LONG_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL2Valid,/* is L2 valid [107] */
                            XCAT_PCL_KEY_FIELDS_ID_IS_L2_VALID_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpV6EhHopByHop, /* [108] Is IPv6 EH Hop By Hop */
                            XCAT_PCL_KEY_FIELDS_ID_LONG_IS_IPV6_EH_HOP_BY_HOP_KEY5_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpHeaderOk ,
                            XCAT_PCL_KEY_FIELDS_ID_IP_HEADER_OK_E);/* [109] ip header  */
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, 0,/* Reserved [111:110] */
                            XCAT_PCL_KEY_FIELDS_ID_RESERVED_110_111_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[3], /* [143:112] SIP*/
                            XCAT_PCL_KEY_FIELDS_ID_SIP_31_0_KEY5_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[2], /* [175:144] SIP*/
                            XCAT_PCL_KEY_FIELDS_ID_SIP_63_32_KEY5_E);

    if (devObjPtr->iPclKeyFormatVersion)
    {
        snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, (descrPtr->sip[1] & 0xFFFF), /* [191:176] SIP*/
                                XCAT2_PCL_KEY_FIELDS_ID_SIP_79_64_KEY5_E);
        snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, (descrPtr->sip[1] >> 16), /* [221:206] SIP*/
                                XCAT2_PCL_KEY_FIELDS_ID_SIP_95_80_KEY5_E);
    }
    else
    {
        snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[1], /* [207:176] SIP*/
                            XCAT_PCL_KEY_FIELDS_ID_SIP_95_64_KEY5_E);
    }

    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[0], /* [239:208] SIP*/
                            XCAT_PCL_KEY_FIELDS_ID_SIP_127_96_KEY5_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, (descrPtr->dip[0] >> 24), /* [247:240] DIP*/
                            XCAT_PCL_KEY_FIELDS_ID_DIP_127_120_E);

    snetXCatIPclKeyFieldBuildByPointer(pclKeyPtr, descrPtr->macSaPtr,/* [295:248] MAC SA*/
                            XCAT_PCL_KEY_FIELDS_ID_MAC_SA_E);
    snetXCatIPclKeyFieldBuildByPointer(pclKeyPtr, descrPtr->macDaPtr,/* [343:296] MAC DA*/
                            XCAT_PCL_KEY_FIELDS_ID_MAC_DA_KEY4_E);

    rc = snetXCatPclUdbKeyValueGet(devObjPtr, descrPtr,
                                   XCAT_IPCL_KEY_TYPE_L2_IPV6_KEY_LONG_E,
                                   6, &byteValue);
    if(rc == GT_FAIL)
    {
        userDefError = 1;
    }

    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [351:344] UserDefinedByte6 */
                            XCAT_PCL_KEY_FIELDS_ID_UDB_6_E);

    /* UDB 7,8,9,10 positions 3,2,1,0 */
    for(i = 0; i < 4; i++)
    {
        rc = snetXCatPclUdbKeyValueGet(devObjPtr, descrPtr,
                                       XCAT_IPCL_KEY_TYPE_L2_IPV6_KEY_LONG_E,
                                       i + 7, &tmpFieldVal[3 - i]);
        if(rc == GT_FAIL)
        {
            userDefError = 1;
        }
    }

    snetXCatIPclKeyFieldBuildByPointer(pclKeyPtr, (GT_U8*)&tmpFieldVal[0],/* User defined bytes 7,8,9,10  */
                            XCAT_PCL_KEY_FIELDS_ID_UDB_10_9_8_7_E);

    /* must be checked after all the UDB validity checks */
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, CH3_UDB_VALID_MAC(userDefError),/* [106] UserDefineValid */
                            XCAT_PCL_KEY_FIELDS_ID_USER_DEFINED_VALID_E);
}

/*******************************************************************************
*   snetXCatIPclKeyBuildL4IPv6ExtendedKey
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
static GT_VOID snetXCatIPclKeyBuildL4IPv6ExtendedKey
(
    IN SKERNEL_DEVICE_OBJECT                        * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC              * descrPtr,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC              * curLookUpCfgPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC                * pclExtraDataPtr,
    IN SNET_XCAT_IPCL_KEY_TYPE_ENT            keyType,
    OUT SNET_CHT_POLICY_KEY_STC                     * pclKeyPtr
)
{
    GT_U8       byteValue;          /* value of user defined byte from the packets or tcp/udp compare */
    GT_U32      userDefError = 0;   /* is there user defined byte error */
    GT_STATUS   rc;                 /* return code from function */


    snetXCatIPclBuildBaseCommonKey(devObjPtr, descrPtr, curLookUpCfgPtr,
                                   pclExtraDataPtr, keyType,pclKeyPtr);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, (descrPtr->isIp && descrPtr->isIPv4 == 0), /* [40] is IPv6 */
                            XCAT_PCL_KEY_FIELDS_ID_IS_IPV6_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dscp,/* [47:42] dscp */
                            XCAT_PCL_KEY_FIELDS_ID_DSCP_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->ipProt,/* [55:48] IP Protocol*/
                            XCAT_PCL_KEY_FIELDS_ID_IP_PROTOCOL_E);

    rc = snetXCatPclUdbKeyValueGet(devObjPtr, descrPtr,
                                   XCAT_IPCL_KEY_TYPE_L4_IPV6_KEY_LONG_E,
                                   14, &byteValue);
    if(rc == GT_FAIL)
    {
        userDefError = 1;
    }

    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [63:56] UserDefinedByte14 */
                            XCAT_PCL_KEY_FIELDS_ID_UDB_14_E);

    snetXCatIPclBuildL4ByteOffsetKey(devObjPtr, descrPtr, pclExtraDataPtr , 0 /*don't care*/, pclKeyPtr); /* [79:64] L4 Byte Offset 3_2 +
                                                                                          [103:80] L4 byte offsets*/
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL4Valid, /* [104] isL4Valid*/
                            XCAT_PCL_KEY_FIELDS_ID_IS_L4_VALID_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpV6EhExists, /* [105] isL4Valid*/
                            XCAT_PCL_KEY_FIELDS_ID_IS_IPV6_EH_EXISTS_LONG_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL2Valid,/* is L2 valid [107] */
                            XCAT_PCL_KEY_FIELDS_ID_IS_L2_VALID_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpV6EhHopByHop, /* [108] Is IPv6 EH Hop By Hop */
                            XCAT_PCL_KEY_FIELDS_ID_LONG_IS_IPV6_EH_HOP_BY_HOP_KEY5_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpHeaderOk ,
                            XCAT_PCL_KEY_FIELDS_ID_IP_HEADER_OK_E);/* [109] ip header  */
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, 0,/* Reserved [111:110] */
                            XCAT_PCL_KEY_FIELDS_ID_RESERVED_110_111_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[3], /* [143:112] SIP*/
                            XCAT_PCL_KEY_FIELDS_ID_SIP_31_0_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[3], /* [175:144] DIP */
                            XCAT_PCL_KEY_FIELDS_ID_DIP_31_0_E);

    if (devObjPtr->iPclKeyFormatVersion)
    {
        snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[2], /* [191:176] SIP*/
                                XCAT2_PCL_KEY_FIELDS_ID_SIP_47_32_KEY6_E);

        snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, (descrPtr->sip[2] >> 16), /* [221:206] SIP*/
                                XCAT2_PCL_KEY_FIELDS_ID_SIP_63_48_KEY6_E);
    }
    else
    {
        snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[2], /* [207:176] SIP*/
                            XCAT_PCL_KEY_FIELDS_ID_SIP_63_32_KEY6_E);
    }

    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[1], /* [239:208] SIP*/
                            XCAT_PCL_KEY_FIELDS_ID_SIP_95_64_KEY6_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[0], /* [271:240] SIP*/
                            XCAT_PCL_KEY_FIELDS_ID_SIP_127_96_KEY6_E);

    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, (descrPtr->dip[0] >> 24), /* [279:272] DIP*/
                            XCAT_PCL_KEY_FIELDS_ID_DIP_127_120_KEY6_E);

    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[2], /* [311:280] DIP*/
                            XCAT_PCL_KEY_FIELDS_ID_DIP_63_32_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[1], /* [343:312] DIP*/
                            XCAT_PCL_KEY_FIELDS_ID_DIP_95_64_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[0], /* [367:344] DIP*/
                            XCAT_PCL_KEY_FIELDS_ID_DIP_119_96_E);

    rc = snetXCatPclUdbKeyValueGet(devObjPtr, descrPtr,
                            XCAT_IPCL_KEY_TYPE_L4_IPV6_KEY_LONG_E,
                            12, &byteValue);
    if(rc == GT_FAIL)
    {
        userDefError = 1;
    }

    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [375:368] UserDefinedByte12 */
                            XCAT_PCL_KEY_FIELDS_ID_UDB_12_KEY6_E);

    rc = snetXCatPclUdbKeyValueGet(devObjPtr, descrPtr,
                              XCAT_IPCL_KEY_TYPE_L4_IPV6_KEY_LONG_E,
                              13, &byteValue);
    if(rc == GT_FAIL)
    {
        userDefError = 1;
    }

    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [376:383] UserDefinedByte13 */
                            XCAT_PCL_KEY_FIELDS_ID_UDB_13_E);

    /* must be checked after all the UDB validity checks */
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, CH3_UDB_VALID_MAC(userDefError),/* [106] UserDefineValid */
                            XCAT_PCL_KEY_FIELDS_ID_USER_DEFINED_VALID_E);
}

/*******************************************************************************
*   snetXCatIPclKeyBuildIPortVlanQosIPv4ExtendedKey
*
* DESCRIPTION:
*        Build Extended (48B) Port/VLAN+QoS+IPv4 PCL search key.
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
static GT_VOID snetXCatIPclKeyBuildIPortVlanQosIPv4ExtendedKey
(
    IN SKERNEL_DEVICE_OBJECT                        * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC              * descrPtr,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC              * curLookUpCfgPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC                * pclExtraDataPtr,
    IN SNET_XCAT_IPCL_KEY_TYPE_ENT            keyType,
    OUT SNET_CHT_POLICY_KEY_STC                     * pclKeyPtr
)
{
    GT_STATUS rc;
    GT_U32      fieldVal;           /* valid bit value */
    GT_U8       byteValue;          /* value of user defined byte from the packets or tcp/udp compare */
    GT_U32      userDefError = 0;   /* is there user defined byte error */
    GT_U8       tmpFieldVal[6];     /* byte array */
    GT_U32      i;

    snetXCatIPclBuildBaseCommonKey(devObjPtr, descrPtr, curLookUpCfgPtr,
                                   pclExtraDataPtr, keyType,pclKeyPtr);

    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, ((descrPtr->l3StartOffsetPtr[1] >> 2) & 0x3f),/* [38:33] TOS[7:2] */
                            XCAT_PCL_KEY_FIELDS_PCKT_TOS_2_7_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->arp ,/* [39] isArp */
                            XCAT_PCL_KEY_FIELDS_ID_IS_ARP_KEY8_E);

    fieldVal = (descrPtr->macDaType == SKERNEL_BROADCAST_MAC_E ||
              descrPtr->macDaType == SKERNEL_BROADCAST_ARP_E) ? 1 : 0;

    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, fieldVal,    /* [40] is bc */
                            XCAT_PCL_KEY_FIELDS_ID_IS_BC_KEY8_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->ipProt,/* [49:42] IP Protocol*/
                            XCAT_PCL_KEY_FIELDS_ID_IP_PROTOCOL_KEY8_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->l3StartOffsetPtr[1],/* [50] TOS[0] */
                            XCAT_PCL_KEY_FIELDS_PCKT_TOS_0_E);

    VALUE_IPV4_FRAGMENT_MAC(pclExtraDataPtr,fieldVal)
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, fieldVal, /* [52:51] IP fragmented */
                            XCAT_PCL_KEY_FIELDS_IP_FRAGMENTED_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->ipv4HeaderInfo,/* [53:54] IP header info */
                            XCAT_PCL_KEY_FIELDS_IP_HEADER_INFO_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL2Valid ,/* [55] isL2Valid  */
                            XCAT_PCL_KEY_FIELDS_ID_IS_L2_VALID_KEY8_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL4Valid, /* [56] isL4Valid*/
                            XCAT_PCL_KEY_FIELDS_ID_IS_L4_VALID_KEY8_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, (descrPtr->l3StartOffsetPtr[1] >> 1),/* [58] TOS[1] */
                            XCAT_PCL_KEY_FIELDS_PCKT_TOS_1_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, 0,/* [63:59] Reserved */
                            XCAT_PCL_KEY_FIELDS_ID_RESERVED_59_63_E);
    snetXCatIPclBuildL4ByteOffsetKey(devObjPtr, descrPtr, pclExtraDataPtr , 0 /*don't care*/, pclKeyPtr); /* [79:64] L4 Byte Offset 3_2 +
                                                                                          [103:80] L4 byte offsets*/

    rc = snetXCatPclUdbKeyValueGet(devObjPtr, descrPtr,
                            XCAT_IPCL_KEY_TYPE_IPCL0_IPV4_KEY_LONG_E,
                            13, &byteValue);
    if(rc == GT_FAIL)
    {
        userDefError = 1;
    }

    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [111:104] UserDefinedByte13 */
                            XCAT_PCL_KEY_FIELDS_ID_UDB_13_KEY8_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[0], /* [143:112] SIP*/
                            XCAT_PCL_KEY_FIELDS_ID_SIP_31_0_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[0], /* [175:144] DIP */
                            XCAT_PCL_KEY_FIELDS_ID_DIP_31_0_E);
    snetXCatIPclKeyFieldBuildByPointer(pclKeyPtr, descrPtr->macSaPtr,/* [223:176] MAC SA*/
                        XCAT_PCL_KEY_FIELDS_ID_MAC_SA_KEY8_E);
    snetXCatIPclKeyFieldBuildByPointer(pclKeyPtr, descrPtr->macDaPtr,/* [271:224] MAC DA*/
                        XCAT_PCL_KEY_FIELDS_ID_MAC_DA_KEY8_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->etherTypeOrSsapDsap,/* [287:272] */
                        XCAT_PCL_KEY_FIELDS_ID_ETHERTYPE_KEY8_E);

    fieldVal = CONVERT_DESCRIPTOR_L2_ENCAPS_TO_KEY_L2_ENCAPSULATION_MAC(descrPtr->l2Encaps);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, fieldVal,     /* [288] L2 encap type */
                            XCAT_PCL_KEY_FIELDS_ID_L2_ENCAP_0_E);
    rc = snetXCatPclUdbKeyValueGet(devObjPtr, descrPtr,
                            XCAT_IPCL_KEY_TYPE_IPCL0_IPV4_KEY_LONG_E,
                            14, &byteValue);
    if(rc == GT_FAIL)
    {
        userDefError = 1;
    }

    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [296:289] UserDefinedByte14 */
                            XCAT_PCL_KEY_FIELDS_ID_UDB14_E);


    for(i = 0; i < 2; i++)
    {
        rc = snetXCatPclUdbKeyValueGet(devObjPtr, descrPtr,
                                  XCAT_IPCL_KEY_TYPE_IPCL0_IPV4_KEY_LONG_E,
                                  i + 15, &tmpFieldVal[i]);
        if(rc == GT_FAIL)
        {
            userDefError = 1;
        }
    }

    snetXCatIPclKeyFieldBuildByPointer(pclKeyPtr, (GT_U8*)&tmpFieldVal[0],/* User defined bytes 15,16  */
                            XCAT_PCL_KEY_FIELDS_ID_UDB_16_15_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, 0,/* [319:313] Reserved */
                            XCAT_PCL_KEY_FIELDS_ID_RESERVED_313_319_E);

    /* must be checked after all the UDB validity checks */

    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, CH3_UDB_VALID_MAC(userDefError), /* [57] userDefinedValid */
                            XCAT_PCL_KEY_FIELDS_ID_USER_DEFINED_VALID_KEY8_E);
}

/*******************************************************************************
*   snetXCatIPclKeyBuildIPortVlanQosIPv6TripleKey
*
* DESCRIPTION:
*        Build Ultra (72B) Port/VLAN+QoS+IPv6 PCL search key.
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
static GT_VOID snetXCatIPclKeyBuildIPortVlanQosIPv6TripleKey
(
    IN SKERNEL_DEVICE_OBJECT                        * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC              * descrPtr,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC              * curLookUpCfgPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC                * pclExtraDataPtr,
    IN SNET_XCAT_IPCL_KEY_TYPE_ENT            keyType,
    OUT SNET_CHT_POLICY_KEY_STC                     * pclKeyPtr
)
{
    GT_STATUS   rc;
    GT_U32      fieldVal;           /* field value */
    GT_U8       byteValue;          /* value of user defined byte from the packets or tcp/udp compare */
    GT_U32      userDefError = 0;


    snetXCatIPclBuildBaseCommonKey(devObjPtr, descrPtr, curLookUpCfgPtr,
                                   pclExtraDataPtr, keyType,pclKeyPtr);

    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, ((descrPtr->l3StartOffsetPtr[1] >> 2) & 0x3f),/* [38:33] TOS[7:2] */
                            XCAT_PCL_KEY_FIELDS_PCKT_TOS_2_7_E);

    fieldVal = CONVERT_DESCRIPTOR_L2_ENCAPS_TO_KEY_L2_ENCAPSULATION_MAC(descrPtr->l2Encaps);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, fieldVal,     /* [39] L2 encap type */
                            XCAT_PCL_KEY_FIELDS_ID_L2_ENCAP_0_KEY9_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, fieldVal,    /* [40] is bc */
                            XCAT_PCL_KEY_FIELDS_ID_IS_BC_KEY8_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->ipProt,/* [49:42] IP Protocol*/
                            XCAT_PCL_KEY_FIELDS_ID_IP_PROTOCOL_KEY8_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->l3StartOffsetPtr[1],/* [50] TOS[0] */
                            XCAT_PCL_KEY_FIELDS_PCKT_TOS_0_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpV6EhHopByHop, /* [51] ipv6_eh_hopbyhop */
                            XCAT_PCL_KEY_FIELDS_ID_IS_IPV6_EH_HOP_BY_HOP_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpV6EhExists, /* [52] ipv6_eh_exist */
                            XCAT_PCL_KEY_FIELDS_ID_IS_IPV6_EH_EXISTS_KEY9_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, (CH3_IPV6_IS_NET_DISCOVERY_DIP_MAC(descrPtr->dip) ? 1 : 0),
                            XCAT_PCL_KEY_FIELDS_ID_IS_ND_E); /* [53] Is ND */
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpHeaderOk ,/* [54] ip header  */
                            XCAT_PCL_KEY_FIELDS_IP_HEADER_OK_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL2Valid ,/* [55] isL2Valid  */
                            XCAT_PCL_KEY_FIELDS_ID_IS_L2_VALID_KEY8_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL4Valid, /* [56] isL4Valid*/
                            XCAT_PCL_KEY_FIELDS_ID_IS_L4_VALID_KEY8_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, (descrPtr->l3StartOffsetPtr[1] >> 1),/* [58] TOS[1] */
                            XCAT_PCL_KEY_FIELDS_PCKT_TOS_1_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, 0,/* [63:59] Reserved */
                            XCAT_PCL_KEY_FIELDS_ID_RESERVED_59_63_E);
    snetXCatIPclBuildL4ByteOffsetKey(devObjPtr, descrPtr, pclExtraDataPtr , 0 /*don't care*/, pclKeyPtr); /* [79:64] L4 Byte Offset 3_2 +
                                                                                          [103:80] L4 byte offsets*/
    rc = snetXCatPclUdbKeyValueGet(devObjPtr, descrPtr,
                            XCAT_IPCL_KEY_TYPE_IPCL0_IPV6_KEY_TRIPLE_E,
                            12, &byteValue);
    if(rc == GT_FAIL)
    {
        userDefError = 1;
    }

    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [111:104] UserDefinedByte12 */
                            XCAT_PCL_KEY_FIELDS_ID_UDB_12_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[3], /* [143:112] SIP*/
                            XCAT_PCL_KEY_FIELDS_ID_SIP_31_0_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[3], /* [175:144] DIP */
                            XCAT_PCL_KEY_FIELDS_ID_DIP_31_0_E);
    snetXCatIPclKeyFieldBuildByPointer(pclKeyPtr, descrPtr->macSaPtr,/* [223:176] MAC SA*/
                        XCAT_PCL_KEY_FIELDS_ID_MAC_SA_KEY8_E);
    snetXCatIPclKeyFieldBuildByPointer(pclKeyPtr, descrPtr->macDaPtr,/* [271:224] MAC DA*/
                        XCAT_PCL_KEY_FIELDS_ID_MAC_DA_KEY8_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->etherTypeOrSsapDsap,/* [287:272] */
                        XCAT_PCL_KEY_FIELDS_ID_ETHERTYPE_KEY8_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[2], /* [288:319] SIP*/
                            XCAT_PCL_KEY_FIELDS_ID_SIP_63_32_KEY9_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[1], /* [320:351] SIP*/
                            XCAT_PCL_KEY_FIELDS_ID_SIP_95_64_KEY9_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[0], /* [352:383] SIP*/
                            XCAT_PCL_KEY_FIELDS_ID_SIP_127_96_KEY9_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[2], /* [384:415] DIP*/
                            XCAT_PCL_KEY_FIELDS_ID_DIP_63_32_KEY9_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[1], /* [416:447] DIP*/
                            XCAT_PCL_KEY_FIELDS_ID_DIP_95_64_KEY9_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[0], /* [448:479] DIP*/
                            XCAT_PCL_KEY_FIELDS_ID_DIP_127_96_KEY9_E);

    /* must be checked after all the UDB validity checks */
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, CH3_UDB_VALID_MAC(userDefError), /* [57] userDefinedValid */
                            XCAT_PCL_KEY_FIELDS_ID_USER_DEFINED_VALID_KEY8_E);

}

/*******************************************************************************
*   snetXCatIPclKeyBuildAclQosIPv4ShortKey
*
* DESCRIPTION:
*        Build ACL+QoS+IPv4 standard PCL search key.
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
static GT_VOID snetXCatIPclKeyBuildAclQosIPv4ShortKey
(
    IN SKERNEL_DEVICE_OBJECT                        * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC              * descrPtr,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC              * curLookUpCfgPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC                * pclExtraDataPtr,
    IN SNET_XCAT_IPCL_KEY_TYPE_ENT            keyType,
    OUT SNET_CHT_POLICY_KEY_STC                     * pclKeyPtr
)
{
    DECLARE_FUNC_NAME(snetXCatIPclKeyBuildAclQosIPv4ShortKey);

    GT_U32      fieldVal;           /* field value */
    GT_U8       byteValue;          /* value of user defined byte from the packets or tcp/udp compare */
    GT_U32      l3TotalLength;      /* Layer 3 total length */
    GT_U32      keyPacketType;      /* Packet type for the key */

    snetXCatIPclBuildBaseCommonKey(devObjPtr, descrPtr, curLookUpCfgPtr,
                                   pclExtraDataPtr, keyType,pclKeyPtr);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, ((descrPtr->l3StartOffsetPtr[1] >> 2) & 0x3f),/* [38:33] TOS[7:2] */
                            XCAT_PCL_KEY_FIELDS_PCKT_TOS_2_7_E);

    /* get Packet type for the key */
    snetXcatPclKeyPacketTypeGet(devObjPtr, descrPtr, &keyPacketType,GT_FALSE/*passenger*/);
    __LOG_PARAM(keyPacketType);

    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, keyPacketType,/* [41:39] Packet type */
                            XCAT_PCL_KEY_FIELDS_PCKT_TYPE_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->ipProt,/* [49:42] IP Protocol*/
                            XCAT_PCL_KEY_FIELDS_ID_IP_PROTOCOL_KEY8_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->l3StartOffsetPtr[1],/* [50] TOS[0] */
                            XCAT_PCL_KEY_FIELDS_PCKT_TOS_0_E);
    VALUE_IPV4_FRAGMENT_MAC(pclExtraDataPtr,fieldVal)
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, fieldVal, /* [52:51] IP fragmented */
                            XCAT_PCL_KEY_FIELDS_IP_FRAGMENTED_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->ipv4HeaderInfo,/* [53:54] IP header info */
                            XCAT_PCL_KEY_FIELDS_IP_HEADER_INFO_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, 0,/* [55] Reserved */
                            XCAT_PCL_KEY_FIELDS_ID_RESERVED_55_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL4Valid, /* [56] isL4Valid*/
                            XCAT_PCL_KEY_FIELDS_ID_IS_L4_VALID_KEY8_E);
    l3TotalLength = (descrPtr->l3StartOffsetPtr[2] << 8) |
                     descrPtr->l3StartOffsetPtr[3];
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, (l3TotalLength & 0x3F), /* [62-57] IP packet length[5:0]*/
                            XCAT_PCL_KEY_FIELDS_IP_PCKT_LEN_5_0_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->l3StartOffsetPtr[1] >> 1,/* [63] TOS[1] */
                            XCAT_PCL_KEY_FIELDS_PCKT_TOS_1_KEY10_E);

    snetXCatIPclBuildL4ByteOffsetKey(devObjPtr, descrPtr, pclExtraDataPtr , 0 /*don't care*/, pclKeyPtr); /* [79:64] L4 Byte Offset 3_2 +
                                                                                          [103:80] L4 byte offsets*/
    snetXCatPclUdbKeyValueGet(devObjPtr, descrPtr,
                            XCAT_IPCL_KEY_TYPE_IPCL1_IPV4_KEY_SHORT_E,
                            5, &byteValue);

    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [111:104] UserDefinedByte5 */
                            XCAT_PCL_KEY_FIELDS_ID_UDB_5_KEY10_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[0], /* [143:112] SIP*/
                             XCAT_PCL_KEY_FIELDS_ID_SIP_31_0_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[0], /* [175:144] DIP*/
                             XCAT_PCL_KEY_FIELDS_ID_DIP_31_0_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, (l3TotalLength >> 6) & 0xff, /* [183-176] IP packet length[13:6]*/
                            XCAT_PCL_KEY_FIELDS_IP_PCKT_LEN_13_6_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, (descrPtr->ttl & 0x7), /* [186-184] Packet TTL [2:0]*/
                            XCAT_PCL_KEY_FIELDS_ID_TTL_0_2_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, ((descrPtr->ttl >> 3) & 0x1f), /* [191-187] Packet TTL [7:3] */
                            XCAT_PCL_KEY_FIELDS_ID_TTL_3_7_E);
}

/*******************************************************************************
*   snetXCatIPclKeyBuildIAclQosIPv6TripleKey
*
* DESCRIPTION:
*        Build Ultra (72B) Routed ACL+QoS+IPv6 PCL search key.
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
* COMMENSS:
*
*
*******************************************************************************/
static GT_VOID snetXCatIPclKeyBuildIAclQosIPv6TripleKey
(
    IN SKERNEL_DEVICE_OBJECT                        * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC              * descrPtr,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC              * curLookUpCfgPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC                * pclExtraDataPtr,
    IN SNET_XCAT_IPCL_KEY_TYPE_ENT            keyType,
    OUT SNET_CHT_POLICY_KEY_STC                     * pclKeyPtr
)
{
    DECLARE_FUNC_NAME(snetXCatIPclKeyBuildIAclQosIPv6TripleKey);

    GT_U8       byteValue;          /* value of user defined byte from the packets or tcp/udp compare */
    GT_U8       tmpFieldVal[6];     /* byte array */
    GT_U32      l3TotalLength;      /* Layer 3 total length */
    GT_U32      userDefError = 0;
    GT_U32      keyPacketType;      /* Packet type for the key */
    GT_U32      i;
    GT_STATUS   rc;


    snetXCatIPclBuildBaseCommonKey(devObjPtr, descrPtr, curLookUpCfgPtr,
                                   pclExtraDataPtr, keyType,pclKeyPtr);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, ((descrPtr->l3StartOffsetPtr[1] >> 2) & 0x3f),/* [38:33] TOS[7:2] */
                            XCAT_PCL_KEY_FIELDS_PCKT_TOS_2_7_E);

    /* get Packet type for the key */
    snetXcatPclKeyPacketTypeGet(devObjPtr, descrPtr, &keyPacketType,GT_FALSE/*passenger*/);
    __LOG_PARAM(keyPacketType);

    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, keyPacketType,/* [41:39] Packet type */
                            XCAT_PCL_KEY_FIELDS_PCKT_TYPE_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->ipProt,/* [49:42] IP Protocol*/
                            XCAT_PCL_KEY_FIELDS_ID_IP_PROTOCOL_KEY8_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->l3StartOffsetPtr[1],/* [50] TOS[0] */
                            XCAT_PCL_KEY_FIELDS_PCKT_TOS_0_E);

    l3TotalLength = (descrPtr->l3StartOffsetPtr[2] << 8) |
                     descrPtr->l3StartOffsetPtr[3];
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, l3TotalLength & 0xf, /* [54:51] IP packet length[3:0]*/
                            XCAT_PCL_KEY_FIELDS_IP_PCKT_LEN_3_0_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, 0,/* [55] Reserved */
                            XCAT_PCL_KEY_FIELDS_ID_RESERVED_55_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL4Valid, /* [56] isL4Valid*/
                            XCAT_PCL_KEY_FIELDS_ID_IS_L4_VALID_KEY8_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, 0,  /*[63:58] reserved field*/
                            XCAT_PCL_KEY_FIELDS_ID_RESERVED_58_63_E);
    snetXCatIPclBuildL4ByteOffsetKey(devObjPtr, descrPtr, pclExtraDataPtr , 0 /*don't care*/, pclKeyPtr); /* [79:64] L4 Byte Offset 3_2 +
                                                                                          [103:80] L4 byte offsets*/
    rc = snetXCatPclUdbKeyValueGet(
        devObjPtr, descrPtr,
        XCAT_IPCL_KEY_TYPE_IPCL1_IPV6_KEY_TRIPLE_E,
        18, &byteValue);
    if(rc == GT_FAIL)
    {
        userDefError = 1;
    }
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [111:104] UserDefinedByte18 */
                            XCAT_PCL_KEY_FIELDS_ID_UDB_18_KEY11_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[3], /* [143:112] SIP*/
                             XCAT_PCL_KEY_FIELDS_ID_SIP_31_0_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[3], /* [175:144] DIP*/
                             XCAT_PCL_KEY_FIELDS_ID_DIP_31_0_E);
    rc = snetXCatPclUdbKeyValueGet(
        devObjPtr, descrPtr,
        XCAT_IPCL_KEY_TYPE_IPCL1_IPV6_KEY_TRIPLE_E,
        19, &byteValue);
    if(rc == GT_FAIL)
    {
        userDefError = 1;
    }

    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, byteValue,/* [183:176] UserDefinedByte19 */
                            XCAT_PCL_KEY_FIELDS_ID_UDB_19_KEY11_E);
    for(i = 0; i < 4; i++)
    {
        rc = snetXCatPclUdbKeyValueGet(devObjPtr, descrPtr,
                                       XCAT_IPCL_KEY_TYPE_IPCL1_IPV6_KEY_TRIPLE_E,
                                       (i == 3) ? 17 : (i + 20), &tmpFieldVal[i]);
        if(rc == GT_FAIL)
        {
            userDefError = 1;
        }
    }

    snetXCatIPclKeyFieldBuildByPointer(pclKeyPtr, (GT_U8*)&tmpFieldVal[0],/* User defined bytes 7,8,9,10  */
                            XCAT_PCL_KEY_FIELDS_ID_UDB_17_22_21_20_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, (descrPtr->l3StartOffsetPtr[1] >> 1),/* [216] TOS[1] */
                            XCAT_PCL_KEY_FIELDS_PCKT_TOS_1_KEY11_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, (CH3_IPV6_IS_NET_DISCOVERY_DIP_MAC(descrPtr->dip) ? 1 : 0),
                            XCAT_PCL_KEY_FIELDS_ID_IS_ND_KEY11_E); /* [217] Is ND */
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->flowLabel, /* [237:218] IPV6 flow label */
                            XCAT_PCL_KEY_FIELDS_ID_IPV6_HEADER_FLOW_LBL_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->ipXMcLinkLocalProt, /* [238] IPv6 Link Local Protocol */
                            XCAT_PCL_KEY_FIELDS_ID_IS_IPV6_LINK_LOCAL_E);

#if 0
    if (descrPtr->macDaType == SKERNEL_UNICAST_MAC_E ||
        descrPtr->macDaType == SKERNEL_MULTICAST_MAC_E)
    {

        snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->mplsLabels[0].label, /* [258-239] MPLS outer label */
                                XCAT_PCL_KEY_FIELDS_ID_MPLS_OUTER_LBL_E);
        snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->mplsLabels[0].exp, /* [261-259]  MPLS Outer Label EXP */
                                XCAT_PCL_KEY_FIELDS_ID_MPLS_OUTER_LBL_EXP_E);
        snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->mplsLabels[0].sbit, /* [262] MPLS Outer Label S-Bit */
                                XCAT_PCL_KEY_FIELDS_ID_MPLS_OUTER_LBL_S_BIT_E);
    }
#endif
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, ((l3TotalLength >> 4) & 0xff), /* [270:263] IP packet length[11-4]*/
                            XCAT_PCL_KEY_FIELDS_IP_PCKT_LEN_11_4_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->ttl, /* [278-271] Packet TTL */
                            XCAT_PCL_KEY_FIELDS_ID_TTL_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[2], /* [310:279] SIP*/
                            XCAT_PCL_KEY_FIELDS_ID_SIP_63_32_KEY11_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[1], /* [342:311] SIP*/
                            XCAT_PCL_KEY_FIELDS_ID_SIP_95_64_KEY11_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[0], /* [374:343] SIP*/
                            XCAT_PCL_KEY_FIELDS_ID_SIP_127_96_KEY11_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[2], /* [406:375] DIP*/
                            XCAT_PCL_KEY_FIELDS_ID_DIP_63_32_KEY11_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[1], /* [438:312] DIP*/
                            XCAT_PCL_KEY_FIELDS_ID_DIP_95_64_KEY11_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[0], /* [470:439] DIP*/
                            XCAT_PCL_KEY_FIELDS_ID_DIP_127_96_KEY11_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, ((l3TotalLength >> 12) & 0xf), /* [474:471] IP packet length[15-12]*/
                            XCAT_PCL_KEY_FIELDS_IP_PCKT_LEN_15_12_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, 0,/* [475] reserved  */
                            XCAT_PCL_KEY_FIELDS_ID_RESERVED_475_E);

    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->isIpv6Mld, /* [469] isMld */
                            XCAT_PCL_KEY_FIELDS_ID_IS_IPV6_MLD_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpV6EhHopByHop, /* [477] ipv6_eh_hopbyhop */
                            XCAT_PCL_KEY_FIELDS_ID_IS_IPV6_EH_HOP_BY_HOP_KEY11_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpV6EhExists, /* [478] ipv6_eh_exist */
                            XCAT_PCL_KEY_FIELDS_ID_IS_IPV6_EH_EXISTS_KEY11_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpHeaderOk ,/* [479] ip header  */
                            XCAT_PCL_KEY_FIELDS_ID_IP_HEADER_OK_KEY11_E);

    /* must be checked after all the UDB validity checks */
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, CH3_UDB_VALID_MAC(userDefError), /* [57] userDefinedValid */
                            XCAT_PCL_KEY_FIELDS_ID_USER_DEFINED_VALID_KEY8_E);
}

/*******************************************************************************
*   snetXCatIPclKeyBuildIPv6DipShortKey
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
static GT_VOID snetXCatIPclKeyBuildIPv6DipShortKey
(
    IN SKERNEL_DEVICE_OBJECT                        * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC              * descrPtr,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC             * curLookUpCfgPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC                * pclExtraDataPtr,
    IN SNET_XCAT_IPCL_KEY_TYPE_ENT            keyType,
    OUT SNET_CHT_POLICY_KEY_STC                     * pclKeyPtr
)
{
    snetXCatIPclBuildBaseCommonKey(devObjPtr, descrPtr, curLookUpCfgPtr,
                                   pclExtraDataPtr, keyType,pclKeyPtr);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->ipProt,/* [49:42] IP Protocol*/
                            XCAT_PCL_KEY_FIELDS_ID_IP_PROTOCOL_KEY8_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dscp,/* [55:50] dscp */
                            XCAT_PCL_KEY_FIELDS_PCKT_DSCP_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL4Valid, /* [56] isL4Valid*/
                            XCAT_PCL_KEY_FIELDS_ID_IS_L4_VALID_KEY8_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[3], /* [72:57] DIP*/
                            XCAT_PCL_KEY_FIELDS_ID_DIP_15_0_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->arp ,/* [73] isArp */
                            XCAT_PCL_KEY_FIELDS_ID_IS_ARP_KEY12_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpV6EhExists,/* [74] ipv6_EH_exist */
                            XCAT_PCL_KEY_FIELDS_ID_IS_IPV6_EH_EXISTS_KEY12_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[3] >> 16, /* [90:75] DIP*/
                            XCAT_PCL_KEY_FIELDS_ID_DIP_31_16_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpV6EhHopByHop,/* [91] ipv6_EH_exist */
                            XCAT_PCL_KEY_FIELDS_ID_LONG_IS_IPV6_EH_HOP_BY_HOP_KEY12_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[2], /* [123:92] DIP*/
                            XCAT_PCL_KEY_FIELDS_ID_DIP_63_32_KEY12_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[1], /* [155:124] DIP*/
                            XCAT_PCL_KEY_FIELDS_ID_DIP_95_64_KEY12_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[0], /* [187:156] DIP*/
                            XCAT_PCL_KEY_FIELDS_ID_DIP_127_96_KEY12_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, 0,/* [188] Reserved */
                            XCAT_PCL_KEY_FIELDS_ID_RESERVED_188_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL2Valid ,/* [189] isL2Valid  */
                            XCAT_PCL_KEY_FIELDS_ID_IS_L2_VALID_KEY12_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, 0,/* [190] Reserved */
                            XCAT_PCL_KEY_FIELDS_ID_RESERVED_190_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpHeaderOk ,/* [191] ip header  */
                            XCAT_PCL_KEY_FIELDS_IP_HEADER_OK_KEY12_E);
}

/*******************************************************************************
*   snetXCatIPclKeyBuildUdbShortKey
*
* DESCRIPTION:
*        Build Key Type:  Standard (24B) UDB - Ingress PCL search key.
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
static GT_VOID snetXCatIPclKeyBuildUdbShortKey
(
    IN SKERNEL_DEVICE_OBJECT                        * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC              * descrPtr,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC              * curLookUpCfgPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC                * pclExtraDataPtr,
    IN SNET_XCAT_IPCL_KEY_TYPE_ENT            keyType,
    OUT SNET_CHT_POLICY_KEY_STC                     * pclKeyPtr
)
{
    GT_U8  tmpFieldVal[13];
    GT_U32 i;
    GT_U32 userDefError = 0;
    GT_STATUS rc;

    snetXCatIPclBuildBaseUdbCommonKey(devObjPtr, descrPtr, curLookUpCfgPtr,
                                   pclExtraDataPtr, pclKeyPtr);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->isIPv4 , /* [40] is IPv4 */
                            XCAT_PCL_KEY_FIELDS_ID_IS_IPV4_E);

    /* UDB 0,1,2,5 0-3 in array */
    for(i = 0; i < 4; i++)
    {
        rc = snetXCatPclUdbKeyValueGet(devObjPtr, descrPtr,
                                       XCAT_IPCL_KEY_TYPE_UDB_KEY_SHORT_E,
                                       ((i == 3) ? 5 : i), &tmpFieldVal[3 - i]);
        if(rc == GT_FAIL)
        {
            userDefError = 1;
        }
    }

    snetXCatIPclKeyFieldBuildByPointer(pclKeyPtr, (GT_U8*)&tmpFieldVal[0],/* [48:63] UserDefinedByte 0,1,2,5 */
                            XCAT_PCL_KEY_FIELDS_ID_UDB_0_2_5_KEY3_E);


    /* UDB 3-4, 10-11 in array */
    for(i = 0; i < 2; i++)
    {
        rc = snetXCatPclUdbKeyValueGet(devObjPtr, descrPtr,
                                       XCAT_IPCL_KEY_TYPE_UDB_KEY_SHORT_E,
                                       (i + 3), &tmpFieldVal[11 - i]);
        if(rc == GT_FAIL)
        {
            userDefError = 1;
        }

    }

    /* UDB 6-15, 0-9 */
    for(i = 0; i < 10; i++)
    {
        rc = snetXCatPclUdbKeyValueGet(devObjPtr, descrPtr,
                                       XCAT_IPCL_KEY_TYPE_UDB_KEY_SHORT_E,
                                       (i + 6), &tmpFieldVal[9 - i]);
        if(rc == GT_FAIL)
        {
            userDefError = 1;
        }
    }

    snetXCatIPclKeyFieldBuildByPointer(pclKeyPtr, (GT_U8*)&tmpFieldVal[0],/* [96:191] UserDefinedByte 2,3,4,0 */
                            XCAT_PCL_KEY_FIELDS_ID_KEY3_UDB_3_4_6_15_E);

    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, CH3_UDB_VALID_MAC(userDefError),/* [94:94] User Defined Valid */
                            XCAT_PCL_KEY_FIELDS_ID_USER_DEFINED_VALID_KEY3_E);
}

/*******************************************************************************
*   snetXCatIPclKeyBuildUdbLongKey
*
* DESCRIPTION:
*        Build Extended (48B) UDB PCL search key.
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
static GT_VOID snetXCatIPclKeyBuildUdbLongKey
(
    IN SKERNEL_DEVICE_OBJECT                        * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC              * descrPtr,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC             * curLookUpCfgPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC                * pclExtraDataPtr,
    IN SNET_XCAT_IPCL_KEY_TYPE_ENT            keyType,
    OUT SNET_CHT_POLICY_KEY_STC                     * pclKeyPtr
)
{
    DECLARE_FUNC_NAME(snetXCatIPclKeyBuildUdbLongKey);

    GT_U8  tmpFieldVal[7];
    GT_U32 i;
    GT_U32 userDefError = 0;
    GT_STATUS rc;

    snetXCatIPclBuildBaseUdbCommonKey(devObjPtr, descrPtr, curLookUpCfgPtr,
                                   pclExtraDataPtr, pclKeyPtr);

    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, (descrPtr->isIp && descrPtr->isIPv4 == 0), /* [40] is IPv6 */
                            XCAT_PCL_KEY_FIELDS_ID_IS_IPV6_E);
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->ipProt,/* [55:48] IP Protocol*/
                            XCAT_PCL_KEY_FIELDS_ID_IP_PROTOCOL_E);

    for(i = 0; i < 3; i++)
    {
        rc = snetXCatPclUdbKeyValueGet(
            devObjPtr, descrPtr,
            XCAT_IPCL_KEY_TYPE_UDB_KEY_LONG_E,
            ((i == 2) ? 5 : (i + 1)), &tmpFieldVal[2 - i]);
        if(rc == GT_FAIL)
        {
            userDefError = 1;
        }
    }

    snetXCatIPclKeyFieldBuildByPointer(pclKeyPtr, (GT_U8*)&tmpFieldVal[0],/* [56:79] UserDefinedByte 1,2,5 */
                            XCAT_PCL_KEY_FIELDS_ID_UDB_1_2_5_KEY7_E);

    /* UDBs 3,4 => positions 5,4 */
    for (i = 0; (i < 2); i++)
    {
        rc = snetXCatPclUdbKeyValueGet(
            devObjPtr, descrPtr,
            XCAT_IPCL_KEY_TYPE_UDB_KEY_LONG_E,
            (i + 3), &(tmpFieldVal[5 - i]));
        if(rc == GT_FAIL)
        {
            userDefError = 1;
        }
    }

    /* UDBs 6,7,8,9 => positions 3,2,1,0 */
    for (i = 0; (i < 4); i++)
    {
        rc = snetXCatPclUdbKeyValueGet(
            devObjPtr, descrPtr,
            XCAT_IPCL_KEY_TYPE_UDB_KEY_LONG_E,
            (i + 6), &(tmpFieldVal[3 - i]));
        if(rc == GT_FAIL)
        {
            userDefError = 1;
        }
    }

    snetXCatIPclKeyFieldBuildByPointer(pclKeyPtr, (GT_U8*)&tmpFieldVal[0],/* [96:143] UserDefinedByte 3,4,6-9 */
                            XCAT_PCL_KEY_FIELDS_ID_UDB_3_4_6_9_KEY7_E);


    if(descrPtr->isIp && (descrPtr->isIPv4 == 0))
    {
        /* IPV6 */
        __LOG(("IPV6"));
        snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[3], /* [144:175] SIP*/
                                XCAT_PCL_KEY_FIELDS_ID_SIP_31_0_KEY7_E);
        if (devObjPtr->iPclKeyFormatVersion)
        {
            snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[2], /* [176:191] SIP[47:32]*/
                                    XCAT2_PCL_KEY_FIELDS_ID_SIP_47_32_KEY7_E);
            snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, ((descrPtr->sip[2] >> 16) & 0x0000FFFF), /* [206:221] SIP[63:48]*/
                                    XCAT2_PCL_KEY_FIELDS_ID_SIP_63_48_KEY7_E);
        }
        else
        {
            snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[2], /* [176:207] SIP*/
                                    XCAT_PCL_KEY_FIELDS_ID_SIP_63_32_KEY7_E);
        }

        snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[1], /* [208:239] SIP*/
                                XCAT_PCL_KEY_FIELDS_ID_SIP_95_64_KEY7_E);
        snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[0], /* [240:271] SIP*/
                                 XCAT_PCL_KEY_FIELDS_ID_SIP_127_96_KEY7_E);
        snetXCatIPclKeyFieldBuildByValue(
            pclKeyPtr,
            ((descrPtr->dip[0] >> 16) & 0x0000FFFF), /* [272:287] DIP*/
            XCAT_PCL_KEY_FIELDS_ID_DIP_127_112_KEY7_E);
        snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[3], /* [288:319] DIP*/
                                 XCAT_PCL_KEY_FIELDS_ID_DIP_31_0_KEY7_E);
    }
    else
    {
        /* IPV4 and ETH_OTHER */
        __LOG(("IPV4 and ETH_OTHER"));
        snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[0], /* [144:175] SIP*/
                                 XCAT_PCL_KEY_FIELDS_ID_SIP_31_0_KEY7_E);
        if (devObjPtr->iPclKeyFormatVersion)
        {
            snetXCatIPclKeyFieldBuildByPointer(pclKeyPtr, &(descrPtr->macSaPtr[4]),/* [176:191] MAC_SA [15:0] */
                                    XCAT2_PCL_KEY_FIELDS_ID_MAC_SA_15_0_KEY7_E);
            snetXCatIPclKeyFieldBuildByPointer(pclKeyPtr, &(descrPtr->macSaPtr[0]),/* [206:237] MAC_SA [47:16] */
                                    XCAT2_PCL_KEY_FIELDS_ID_MAC_SA_47_16_KEY7_E);
        }
        else
        {
            snetXCatIPclKeyFieldBuildByPointer(pclKeyPtr, descrPtr->macSaPtr,/* [176:223] MAC_SA */
                                    XCAT_PCL_KEY_FIELDS_ID_MAC_SA_KEY7_E);
        }
        snetXCatIPclKeyFieldBuildByPointer(pclKeyPtr, descrPtr->macDaPtr,/* [224:271] MAC_DA */
                                XCAT_PCL_KEY_FIELDS_ID_MAC_DA_KEY7_E);
        snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[0], /* [288:319] DIP*/
                                 XCAT_PCL_KEY_FIELDS_ID_DIP_31_0_KEY7_E);
    }

    /* UDBs 0 => position 0 */
    rc = snetXCatPclUdbKeyValueGet(
        devObjPtr, descrPtr,
        XCAT_IPCL_KEY_TYPE_UDB_KEY_LONG_E,
        0, &tmpFieldVal[0]);

    /* UDBs 10,11,12,13,14,15 => positions 6,5,4,3,2,1 */
    for(i = 0; (i < 6); i++)
    {
        rc = snetXCatPclUdbKeyValueGet(
            devObjPtr, descrPtr,
            XCAT_IPCL_KEY_TYPE_UDB_KEY_LONG_E,
            (i + 10), &(tmpFieldVal[6 - i]));
        if(rc == GT_FAIL)
        {
            userDefError = 1;
        }
    }

    snetXCatIPclKeyFieldBuildByPointer(pclKeyPtr, (GT_U8*)&tmpFieldVal[0],/* [320:375] UserDefinedByte 0,10-15 */
                            XCAT_PCL_KEY_FIELDS_ID_UDB_0_15__10_KEY7_E);

    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, CH3_UDB_VALID_MAC(userDefError),/* [94:94] User Defined Valid */
                            XCAT_PCL_KEY_FIELDS_ID_USER_DEFINED_VALID_KEY3_E);
}

/*******************************************************************************
* snetXCatIPclConfigPtrGet
*
* DESCRIPTION:
*       Get data from the ingress PCL configuration table.
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       descrPtr            - pointer to frame descriptor object.
*       cycle               - lookup cycle index 0,1 or 2
*       lookupConfPtr       - pointer to PCL lookup configuration
*
* OUTPUTS:
*       lookupConfPtr - pointer to lookup configuration data needed for
*       the PCL engine.
*
* RETURN:
*
* COMMENTS:
*
*       Ingress PCL Configuration Table
*       The PCL configuration table is a 4224 line table each entry is 32-bits.
*
*******************************************************************************/
static GT_VOID snetXCatIPclConfigPtrGet
(
    IN SKERNEL_DEVICE_OBJECT                * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC      * descrPtr,
    IN SNET_IPCL_LOOKUP_ENT                 cycle,
    OUT SNET_CHT3_PCL_LOOKUP_CONFIG_STC     * lookupConfPtr
)
{
    DECLARE_FUNC_NAME(snetXCatIPclConfigPtrGet);

    GT_U32 *pclGlobCfgWordPtr;  /* pointer to PCL global configuration register */
    GT_U32  maxDevPort;         /* number of ports in device 0 - 1( 32/64 ports) */
    GT_U32  portIfPclBlock;     /* location of the Ports interface block in the PCL configuration table */
    GT_U32  lookUpIndex = 0;    /* PCL configuration entry cycle index */
    GT_U32  ingressPclCfgMode;  /* IPCL configuration mode */
    GT_U32  regAddr;            /* register address */
    GT_U32  pclCfgEntry[2];     /* PCL configuration entry */
    GT_U32  fieldVal;

    pclGlobCfgWordPtr =
        smemMemGet(devObjPtr, SMEM_CHT_PCL_GLOBAL_REG(devObjPtr));

    switch(descrPtr->pclLookUpMode[cycle])
    {
        case SKERNEL_PCL_LOOKUP_MODE_VID_E:
            /* Access PCL-ID config. table with vlan id  */
            lookUpIndex = SMEM_U32_GET_FIELD(descrPtr->eVid,0,12);
            __LOG(("Access PCL-ID config. table with vlan id : index [0x%x]\n",
                lookUpIndex));
            break;
        case SKERNEL_PCL_LOOKUP_MODE_PORT_E:
            ingressPclCfgMode = SMEM_U32_GET_FIELD(*pclGlobCfgWordPtr, 2, 1);
            if (ingressPclCfgMode == 0)
            {
                /* PCL configuration table index = Local Device Source Port# + 4K */
                SMEM_U32_SET_FIELD(lookUpIndex, 12, 1, 1);
                SMEM_U32_SET_FIELD(lookUpIndex, 0,
                    devObjPtr->flexFieldNumBitsSupport.phyPort,/* 6 in legacy devices , 8 in bc2/bobk */
                    descrPtr->localDevSrcPort);

                __LOG(("PCL configuration table index = Local Device Source Port# + 4K : index [0x%x]\n",
                    lookUpIndex));
            }
            else if (descrPtr->origIsTrunk == 0)
            {
                /* Read the max port per device and the PCL interface block */
                __LOG(("Read the max port per device and the PCL interface block"));
                maxDevPort =
                    SMEM_U32_GET_FIELD(*pclGlobCfgWordPtr, 7, 1);
                portIfPclBlock =
                    SMEM_U32_GET_FIELD(*pclGlobCfgWordPtr, 3, 2);

                if (maxDevPort == 0)
                {
                    /* Max port per device = 0 , up to 1K ports in system */
                    __LOG(("Max 32 ports per device \n"));
                    SMEM_U32_SET_FIELD(lookUpIndex, 10, 2, portIfPclBlock);
                    SMEM_U32_SET_FIELD(lookUpIndex,  5, 5, descrPtr->srcDev);
                    SMEM_U32_SET_FIELD(lookUpIndex,  0, 5, descrPtr->origSrcEPortOrTrnk);
                }
                else
                {
                    /* Max port per device = 1 , up to 2K ports in system */
                    __LOG(("Max 64 ports per device \n"));
                    SMEM_U32_SET_FIELD(lookUpIndex, 11, 1, portIfPclBlock);
                    SMEM_U32_SET_FIELD(lookUpIndex,  6, 5, descrPtr->srcDev);
                    SMEM_U32_SET_FIELD(lookUpIndex,  0, 6, descrPtr->origSrcEPortOrTrnk);
                }
            }
            else
            {
                /* Access PCL-ID config. table with ingress trunk  */
                __LOG(("Access PCL-ID config. table with ingress trunk"));
                SMEM_U32_SET_FIELD(lookUpIndex, 12, 1, 1);
                SMEM_U32_SET_FIELD(lookUpIndex,  0, 7, descrPtr->origSrcEPortOrTrnk);
            }
            __LOG(("PCL configuration table index : index [0x%x]\n",
                lookUpIndex));

            break;


        case SKERNEL_PCL_LOOKUP_MODE_EPORT_E :
            /*This is a SIP5.0 new indexing mode - index with 12 lower bits of <LocalDevSRCePort>.*/
            SMEM_U32_SET_FIELD(lookUpIndex,  0, 12, descrPtr->eArchExtInfo.localDevSrcEPort);
            __LOG(("Access PCL-ID config. table in ePort mode : index [0x%x]\n",
                lookUpIndex));
            break;

        case SKERNEL_PCL_LOOKUP_MODE_DESCRIPTOR_INDEX_E:
            lookUpIndex = descrPtr->ipclProfileIndex;
            __LOG(("Access PCL-ID config. table according to index from previous TTI/PCL action : index [0x%x]",
                lookUpIndex));
            break;
        default:
            skernelFatalError("snetXCatIPclConfigPtrGet: not valid mode[%d]",descrPtr->pclLookUpMode[cycle]);
    }

    __LOG(("Access PCL-ID config. table index [0x%x] \n" , lookUpIndex));

    /* Calculate the address of the PCL configuration table */
    regAddr = SMEM_CHT_PCL_CONFIG_TBL_MEM(devObjPtr, lookUpIndex, cycle);

    smemRegGet(devObjPtr, regAddr, &(pclCfgEntry[0]));

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        lookupConfPtr->pclNonIpTypeCfg   = snetFieldValueGet(pclCfgEntry, 1, 4);
        lookupConfPtr->pclIpV4ArpTypeCfg = snetFieldValueGet(pclCfgEntry, 5, 4);
        lookupConfPtr->pclIpV6TypeCfg    = snetFieldValueGet(pclCfgEntry, 9, 4);
        lookupConfPtr->lookUpEn          = snetFieldValueGet(pclCfgEntry, 0, 1);
        lookupConfPtr->pclID             = snetFieldValueGet(pclCfgEntry, 13, 10);
        lookupConfPtr->udbKeyBitmapEnable= snetFieldValueGet(pclCfgEntry, 23, 1);
        lookupConfPtr->tcamSegmentMode   = snetFieldValueGet(pclCfgEntry, 24, 3);
    }
    else
    {
        lookupConfPtr->pclNonIpTypeCfg =
            SMEM_U32_GET_FIELD(pclCfgEntry[0], 1, 3);
        lookupConfPtr->pclIpV4ArpTypeCfg =
            SMEM_U32_GET_FIELD(pclCfgEntry[0], 4, 3);
        lookupConfPtr->pclIpV6TypeCfg =
            SMEM_U32_GET_FIELD(pclCfgEntry[0], 7, 3);

        if (devObjPtr->pclSupport.ipclSupportSeparatedIConfigTables == 0 &&
            cycle == SNET_IPCL_LOOKUP_0_1_E)
        {
            smemRegGet(devObjPtr, (regAddr + 4), &(pclCfgEntry[1]));
            lookupConfPtr->lookUpEn =
                snetFieldValueGet(pclCfgEntry, 10, 1);
            lookupConfPtr->pclID =
                snetFieldValueGet(pclCfgEntry, 23, 10);
        }
        else
        {
            lookupConfPtr->lookUpEn =
                snetFieldValueGet(pclCfgEntry, 0, 1);
            lookupConfPtr->pclID =
                snetFieldValueGet(pclCfgEntry, 13, 10);
        }
    }

    if (devObjPtr->pclSupport.pclSupportPortList)
    {
        regAddr = SMEM_XCAT_POLICY_ENGINE_CONF_REG(devObjPtr);
        /* Bits 14, 15, 16 - use PortList mode in lookup 0_0, 0_1, 1 */
        smemRegFldGet(devObjPtr, regAddr, 14 + cycle, 1, &fieldVal);
        lookupConfPtr->pclPortListModeEn = fieldVal ? GT_TRUE : GT_FALSE;
    }
    else
    {
        lookupConfPtr->pclPortListModeEn = GT_FALSE;
    }

    lookupConfPtr->ingrlookUpKey = GT_TRUE;

    if(simLogIsOpen())
    {
        __LOG_PARAM(lookupConfPtr->lookUpEn);
        __LOG_PARAM(lookupConfPtr->pclID);
        __LOG_PARAM(lookupConfPtr->pclNonIpTypeCfg);
        __LOG_PARAM(lookupConfPtr->pclIpV4ArpTypeCfg);
        __LOG_PARAM(lookupConfPtr->pclIpV6TypeCfg);
        __LOG_PARAM(lookupConfPtr->tcamSegmentMode);
        __LOG_PARAM(lookupConfPtr->udbKeyBitmapEnable);
        __LOG_PARAM(lookupConfPtr->pclPortListModeEn);
        __LOG_PARAM(lookupConfPtr->ingrlookUpKey);
    }

}

/*******************************************************************************
* snetXCatIPclTriggeringCheck
*
* DESCRIPTION:
*        Check the enabling status of ingress PCL for the incoming frame.
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - pointer to frame data buffer Id
*       cycle           - PCL Lookup cycle number
*       lookUpCfgPtr    - pointer to PCL Lookup configuration
*
*
* OUTPUTS:
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
static GT_BOOL snetXCatIPclTriggeringCheck
(
    IN SKERNEL_DEVICE_OBJECT                * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC      * descrPtr,
    IN SNET_IPCL_LOOKUP_ENT                 cycle,
    OUT SNET_CHT3_PCL_LOOKUP_CONFIG_STC     * lookUpCfgPtr
)
{
    DECLARE_FUNC_NAME(snetXCatIPclTriggeringCheck);

    GT_U32 fieldVal;
    GT_U32 *portVlanCfgEntryPtr;    /* Port VLAN table entry pointer          */
    GT_U32 regAddress;              /* Register address                       */
    GT_BOOL isRouted;               /* is Packet routed                       */

    /*  Enable Ingress Policy Engine */
    smemRegFldGet(devObjPtr, SMEM_CHT_PCL_GLOBAL_REG(devObjPtr), 1, 1, &fieldVal);
     /* 1 - enable, 0 - disable */
    if (fieldVal == 0)
    {
        __LOG(("IPCL globally Disabled \n"));
        return GT_FALSE;
    }

    if (descrPtr->policyOnPortEn == 0)
    {
        __LOG(("IPCL disabled on the ingress port \n"));
        return GT_FALSE;
    }

    snetXCatIPclConfigPtrGet(devObjPtr, descrPtr, cycle, lookUpCfgPtr);
    if(lookUpCfgPtr->lookUpEn == 0)
    {
        __LOG(("the lookup[%d] is disabled \n",
            cycle));
        return GT_FALSE;
    }

    if(devObjPtr->supportEArch && devObjPtr->unitEArchEnable.tti)
    {
        portVlanCfgEntryPtr = NULL;
    }
    else
    {
        regAddress = SMEM_CHT_PORT_VLAN_QOS_CONFIG_TBL_MEM(devObjPtr,
                                                            descrPtr->localDevSrcPort);
        portVlanCfgEntryPtr = smemMemGet(devObjPtr, regAddress);
    }

    if ((descrPtr->tunnelTerminated == 0) &&
        (descrPtr->mac2me || descrPtr->ipm))
    {
        isRouted = GT_TRUE;
    }
    else
    if ((descrPtr->tunnelTerminated == 1) && (descrPtr->isIp))
    {
        /* this is not documented in the FS , but I got it from VERIFIER bobcat2 */
        isRouted = GT_TRUE;
    }
    else
    {
        isRouted = GT_FALSE;
    }

    __LOG_PARAM(isRouted);

    /* Different routed and not_routed packets behavior support */
    if(cycle == SNET_IPCL_LOOKUP_1_E)  /* third lookup */
    {
        if (isRouted == GT_FALSE)
        {
            /* IPCL1 can be disabled for non-routed packets by setting
               the global <Enable IPCL1 Only for Routed Packets>, */
            smemRegFldGet(devObjPtr, SMEM_XCAT_POLICY_ENGINE_CONF_REG(devObjPtr),
                          8, 1, &fieldVal);
            if (fieldVal == 1)
            {
                __LOG(("Disabled: IPCL[%d] only for routed packets \n",
                    cycle));
                return GT_FALSE;
            }
        }
    }
    else  /* IPCL0-0 and IPCL0-1 (first and second lookups) */
    {
        if (isRouted)
        {
            if(descrPtr->eArchExtInfo.ttiPostTtiLookupIngressEPortTablePtr)
            {
                /*Disable IPCL0-0 and IPCL0-1 lookups for routed packets*/
                fieldVal =
                    SMEM_LION3_TTI_EPORT_ATTRIBUTES_ENTRY_FIELD_GET(devObjPtr,descrPtr,
                        SMEM_LION3_TTI_EPORT_ATTRIBUTES_TABLE_FIELDS_DIS_IPCL0_FOR_ROUTED);
            }
            else
            {
                /* IPCL0-0 and IPCL0-1 can be disabled for routed packets */
                fieldVal = SMEM_U32_GET_FIELD(portVlanCfgEntryPtr[0], 14, 1);
            }

            if (fieldVal)
            {
                __LOG(("Disabled: IPCL[%d] only for routed packets \n",
                    cycle));
                return GT_FALSE;
            }
        }
    }

    return GT_TRUE;
}

/*******************************************************************************
* snetXCatIPclPartialModeKeyCreate
*
* DESCRIPTION:
*        Create PCL search key.
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       curLookUpCfgPtr - pointer to the PCL configuration table entry.
*       cycle           - PCL Lookup cycle number
*
* OUTPUT:
*       pclKeyPtr       - pointer to PCL key structure.
*
* RETURN:
*       GT_OK           - on partial lookup key successful creation
*       GT_FAIL         - lookup doesn't match or dual lookup mode is not partial
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_STATUS snetXCatIPclPartialModeKeyCreate
(
    IN SKERNEL_DEVICE_OBJECT                * devObjPtr,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC      * curLookUpCfgPtr,
    IN SNET_IPCL_LOOKUP_ENT                 cycle,
    OUT SNET_CHT_POLICY_KEY_STC             * pclKeyPtr
)
{
    DECLARE_FUNC_NAME(snetXCatIPclPartialModeKeyCreate);

    GT_U32 fieldVal;                    /* Register field value */
    GT_U32 regAddr;                     /* Register address */

    /* Lookup cycle doesn't match  */
    if(cycle != SNET_IPCL_LOOKUP_0_1_E)
    {
        return GT_FAIL;
    }

    regAddr = SMEM_XCAT_POLICY_ENGINE_CONF_REG(devObjPtr);

    /* The additional lookup, IPCL0-1 is performed after IPCL0-0,
       in one of the following modes: full lookup, or partial lookup */
    smemRegFldGet(devObjPtr, regAddr, 12, 1, &fieldVal);
    if(fieldVal)
    {
        /* Dual lookup mode is not partial */
        __LOG(("Dual lookup mode is not partial \n"));
        return GT_FAIL;
    }

    /* PCL-ID for Lookup0-1 */
    __LOG(("WARNING : Partial Dual lookup mode !!! (only PCL-ID changed between first and second lookups) \n"));
    __LOG(("PCL-ID for Lookup0-1 [%d] \n",
        curLookUpCfgPtr->pclID));
    snetXCatIPclKeyFieldBuildByValue(pclKeyPtr,
                                     curLookUpCfgPtr->pclID,
                                     XCAT_PCL_KEY_FIELDS_ID_PCL_ID_E); /*[9:0] PCL-ID */

    return GT_OK;
}

/*******************************************************************************
* snetXcat2IpclKeyPortsBmpBuild
*
* DESCRIPTION:
*        Build the 'ports bmp' in the PCL search key.
*       supported from xcat2
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - pointer to frame data buffer Id.
*
* OUTPUT:
*       pclKeyPtr    - pointer to PCL key structure. (ignored when NULL)
*       portListBmpPtr - pointer to the bmp of ports. (ignored when NULL)
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID snetXcat2IpclKeyPortsBmpBuild
(
    IN SKERNEL_DEVICE_OBJECT                * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC   * descrPtr,
    OUT SNET_CHT_POLICY_KEY_STC             * pclKeyPtr,
    OUT           GT_U32                  * portListBmpPtr
)
{
    DECLARE_FUNC_NAME(snetXcat2IpclKeyPortsBmpBuild);

    /* PortList bitmap 0-27, separated on 3 parts */
    GT_U32 portListBits7_0   = 0;
    GT_U32 portListBits13_8  = 0;
    GT_U32 portListBits27_14 = 0;
    GT_U32 bitIndex; /* bit index that represent the port */
    GT_U32 index;
    GT_U32 regAddr;
    GT_U32 startBit;
    GT_BIT pclPortGroupEnable; /*When enabled, the 4 msbits of the <Port List> field in the PCL keys is replaced by a 4-bit <Port Group>.*/
    GT_U32 finalPortListBmp; /*final 28 bits of <port list> bmp */
    GT_U32  portGroup;


    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        /* the port number is mapped to a bit index */
        if(descrPtr->eArchExtInfo.ttiPhysicalPort2AttributePtr)
        {
            bitIndex = SMEM_SIP5_20_TTI_PHYSICAL_PORT_ENTRY_FIELD_GET(devObjPtr,descrPtr,
                SMEM_LION3_TTI_PHYSICAL_PORT_2_TABLE_FIELD_PORT_LIST_BIT_VECTOR_OFFSET);
        }
        else
        {
            /* 6 ports in register */
            index = descrPtr->localDevSrcPort / 6;
            startBit = (descrPtr->localDevSrcPort % 6) * 5;/* 5 bits per port*/
            regAddr = SMEM_LION3_TTI_PCL_PORT_LIST_MAP_REG(devObjPtr,index);

            smemRegFldGet(devObjPtr,regAddr,startBit ,5,&bitIndex);
        }

        regAddr = SMEM_LION3_TTI_UNIT_GLOBAL_CONFIG_EXT_REG(devObjPtr);
        smemRegFldGet(devObjPtr, regAddr, 28, 1, &pclPortGroupEnable);
    }
    else
    {
        /* the port number is the bit index */
        bitIndex = descrPtr->localDevSrcPort;
        /* to set generic code with sip5 */
        pclPortGroupEnable = 0;
    }

    __LOG(("the bit index [%d] for the <localDevSrcPort> [%d]\n",
        bitIndex,
        descrPtr->localDevSrcPort));

    /* port List bitmap - only one bit is "1", corresponding to source port
       For CPU port (63) - all bits are "0" */
    if (bitIndex <= 7)
    {
        portListBits7_0 = 1 << bitIndex;
    }
    else
    if (bitIndex <= 13)
    {
        portListBits13_8 = 1 << (bitIndex - 8);
    }
    else
    if (bitIndex <= 27)
    {
        portListBits27_14 = 1 << (bitIndex - 14);
    }

    finalPortListBmp = (portListBits27_14 << 14) |
                       (portListBits13_8  <<  8) |
                        portListBits7_0;

    if(pclPortGroupEnable)/*sip5 only*/
    {
        if(descrPtr->eArchExtInfo.ttiPhysicalPort2AttributePtr)
        {
            portGroup = SMEM_SIP5_20_TTI_PHYSICAL_PORT_ENTRY_FIELD_GET(devObjPtr,descrPtr,
                SMEM_LION3_TTI_PHYSICAL_PORT_2_TABLE_FIELD_PORT_GROUP);
        }
        else
        {
            index = descrPtr->localDevSrcPort / 8;
            startBit = (descrPtr->localDevSrcPort % 8) * 4;/* 4 bits per port*/

            /*When enabled, the 4 msbits of the <Port List> field in the PCL keys is replaced by a 4-bit <Port Group>.*/
            regAddr = SMEM_LION3_TTI_PCL_PORT_GROUP_MAP_REG(devObjPtr,index);
            smemRegFldGet(devObjPtr,regAddr,startBit ,4,&portGroup);
        }

        __LOG(("PCL Port Group Enabled , so use <Port Group>[%d] to override 4 msbits of the <Port List> \n",
            portGroup));

        /*When enabled, the 4 msbits of the <Port List> field in the PCL keys is replaced by a 4-bit <Port Group>.*/
        SMEM_U32_SET_FIELD(finalPortListBmp ,24 ,4 ,portGroup);
        portListBits27_14 = finalPortListBmp >> 14;
    }

    __LOG_PARAM(portListBits7_0);
    __LOG_PARAM(portListBits13_8);
    __LOG_PARAM(portListBits27_14);
    __LOG_PARAM(finalPortListBmp);

    if(portListBmpPtr)
    {
        *portListBmpPtr = finalPortListBmp;
    }

    if(pclKeyPtr)
    {
        if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, portListBits7_0 ,
                                    LION3_KEY_FIELD_NAME_BUILD( 0   ,   7   ,PCL_ID_7_0_OR_port_list_7_0      ));
            snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, portListBits13_8 ,
                                    LION3_KEY_FIELD_NAME_BUILD( 11  ,  16   ,SrcPort_5_0_OR_port_list_13_8    ));
            snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, portListBits27_14 ,
                                    LION3_KEY_FIELD_NAME_BUILD( 192 , 193   ,port_list_15_14_OR_SrcPort_7_6   ));
            snetLion3IPclKeyFieldBuildByValue(pclKeyPtr, portListBits27_14 >> 2 ,
                                    LION3_KEY_FIELD_NAME_BUILD( 194 , 205   ,port_list_27_16_OR_2b0_SrcDev_9_0));
        }
        else
        {
            snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, portListBits7_0 ,
                                    XCAT_PCL_KEY_FIELDS_ID_PORT_LIST_7_0_E);/*[7:0] port_list[7:0]  */
            snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, portListBits13_8 ,
                                    XCAT_PCL_KEY_FIELDS_ID_PORT_LIST_13_8_E);/*[16:11] port_list[13:8]  */
            snetXCatIPclKeyFieldBuildByValue(pclKeyPtr, portListBits27_14 ,
                                    XCAT_PCL_KEY_FIELDS_ID_PORT_LIST_27_14_E);/*[205:192] port_list[27:14]  */
        }
    }
}

/*******************************************************************************
* snetLionIPclOverrideUdbKeyWithPacketHash
*
* DESCRIPTION:
*        Overrides UDB byte by trunk hash in PCL search key
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - pointer to frame data buffer Id.
*       keyIndex        - the key index
*       cycle           - PCL Lookup cycle number
*       pclKeyPtr       - pointer to PCL key structure.
*
* OUTPUT:
*       pclKeyPtr       - pointer to PCL key structure.
*
* RETURN:
*
* COMMENTS:
*       Each IPCL key can  independently be configured to have one of its
*       user-defined bytes replaced by the trunk hash.
*
*******************************************************************************/
static GT_VOID snetLionIPclOverrideUdbKeyWithPacketHash
(
    IN SKERNEL_DEVICE_OBJECT                * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC   * descrPtr,
    IN DXCH_IPCL_KEY_ENT                      keyIndex,
    IN GT_U32                                 cycle,
    INOUT SNET_CHT_POLICY_KEY_STC           * pclKeyPtr
)
{
    DECLARE_FUNC_NAME(snetLionIPclOverrideUdbKeyWithPacketHash);

    GT_BIT  enabled;                           /* Register field value  */
    CHT_PCL_KEY_FIELDS_INFO_STC lion3FieldInfo;  /* Lion3 : PCL field info */
    CHT_PCL_KEY_FIELDS_INFO_STC *fieldInfoPtr;  /* PCL field info */
    UDB_INFO_STC* udbInfoArr;
    GT_U32  udbIndex;
    GT_U32  ii;
    char            udbNameStr[32];

    ASSERT_PTR(pclKeyPtr);

    /* The Trunk Hash field cannot be included in the key of the first IPCL lookup */
    if(cycle == 0)
    {
        __LOG(("The Trunk Hash field cannot be included in the key of the first IPCL lookup \n"));
        return;
    }

    /* Get Policy Engine User Defined Bytes Extended Configuration */
    smemRegFldGet(devObjPtr, SMEM_LION_PCL_UDB_EXTEND_CONF_REG(devObjPtr),
                  keyIndex, 1, &enabled);
    if(enabled == 0)
    {
        /* the replacement not enabled for the key */
        __LOG(("Key#[%d] : not enabled for trunk hash to replace UDB \n",
            keyIndex));
        return;
    }

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        /* get the udb index for this key */
        udbIndex = lion3PclUdbKeyTrunkHashFieldsUdbIndex[keyIndex];
        if (udbIndex == SMAIN_NOT_VALID_CNS)
        {
            __LOG(("Key#[%d] : does not support trunk hash to replace UDB \n",
                keyIndex));
            return;
        }

        udbInfoArr = lion3KeysUdbArr[keyIndex];

        for(ii = 0 ; udbInfoArr[ii].udbIndex != SMAIN_NOT_VALID_CNS ; ii++)
        {
            if(udbInfoArr[ii].udbIndex == udbIndex)
            {
                lion3FieldInfo.updateOnSecondCycle = GT_FALSE;
                lion3FieldInfo.startBitInKey = udbInfoArr[ii].startBitInKey;
                lion3FieldInfo.endBitInKey = lion3FieldInfo.startBitInKey + 7;/*8 bit field*/

                sprintf(udbNameStr,"trunk hash in key[%d] UDB[%d]",keyIndex,udbIndex);
                lion3FieldInfo.debugName = udbNameStr;
                break;
            }
        }

        if(udbInfoArr[ii].udbIndex == SMAIN_NOT_VALID_CNS)
        {
            /* not found the UDB in the array ... not good */
            skernelFatalError("not found the UDB index [%d] in the array for key [%d]",
                udbIndex,keyIndex);
        }

        fieldInfoPtr = &lion3FieldInfo;

    }
    else
    {
        fieldInfoPtr = &lionPclUdbKeyTrunkHashFields[keyIndex];

        /* Key 1 (LION_IPCL_KEY_TYPE_L2_L3_KEY_SHORT_E) does not support trunk hash */
        if (fieldInfoPtr->startBitInKey == SMAIN_NOT_VALID_CNS)
        {
            __LOG(("Key#[%d] : does not support trunk hash to replace UDB \n",
                keyIndex));
            return;
        }
    }

    __LOG(("key#[%d] enabled for Override UDB with trunk hash result [0x%x] \n",
        keyIndex , descrPtr->pktHashForIpcl));

    /* Override key UDB with trunk hash result */
    snetChtPclSrvKeyFieldBuildByValue(pclKeyPtr,descrPtr->pktHashForIpcl,fieldInfoPtr);
}


/*******************************************************************************
*   lion3IpclKey60UdbBuildKeyExtra10Bytes
*
* DESCRIPTION:
*        Lion3 IPCL - build the extra 10 bytes of the key (after the first 50 UDBs)
*                   of the 60B Key
*
* INPUTS:
*       devObjPtr            - pointer to device object.
*       descrPtr             - pointer to frame data buffer Id.
*       curLookUpCfgPtr      - pointer to the current PCL configuration entry.
*       pclExtraDataPtr      - pointer to extra data needed for the IPCL engine.
*       udbValid             - indication that ALL the 50 UDBs are valid
*                               (to be part of the extra 10B info)
* OUTPUTS:
*       pclKeyPtr    - pointer to PCL key structure.
*
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID lion3IpclKey60UdbBuildKeyExtra10Bytes
(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC  * curLookUpCfgPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC    * pclExtraDataPtr,
    OUT SNET_CHT_POLICY_KEY_STC         * pclKeyPtr,
    IN GT_BIT                           udbValid
)
{
    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr,    udbValid,
        LION3_KEY_60B_EXTRA_10B_FIELD_NAME_BUILD(400 , 400 , UDB_Valid     ));

    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr,    curLookUpCfgPtr->pclID,
        LION3_KEY_60B_EXTRA_10B_FIELD_NAME_BUILD(401 , 410 , PCL_ID        ));

    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr,    descrPtr->eVid,
        LION3_KEY_60B_EXTRA_10B_FIELD_NAME_BUILD(411 , 423 , eVLAN         ));

    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr,    descrPtr->origSrcEPortOrTrnk,
        LION3_KEY_60B_EXTRA_10B_FIELD_NAME_BUILD(424 , 436 , Source_ePort_OR_Trunk_id  ));

    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr,    descrPtr->srcDevIsOwn,
        LION3_KEY_60B_EXTRA_10B_FIELD_NAME_BUILD(437 , 437 , Src_Dev_Is_Own));

    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr,    descrPtr->vid1,
        LION3_KEY_60B_EXTRA_10B_FIELD_NAME_BUILD(438 , 449 , Tag1_VID      ));

    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr,    descrPtr->up1,
        LION3_KEY_60B_EXTRA_10B_FIELD_NAME_BUILD(450 , 452 , Tag1_UP       ));

    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr,    descrPtr->mac2me,
        LION3_KEY_60B_EXTRA_10B_FIELD_NAME_BUILD(453 , 453 , Mac_To_Me     ));

    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr,    descrPtr->qos.qosProfile,
        LION3_KEY_60B_EXTRA_10B_FIELD_NAME_BUILD(454 , 463 , QoS_Profile   ));

    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr,    descrPtr->flowId,
        LION3_KEY_60B_EXTRA_10B_FIELD_NAME_BUILD(464 , 475 , Flow_ID       ));

    snetLion3IPclKeyFieldBuildByValue(pclKeyPtr,    0,
        LION3_KEY_60B_EXTRA_10B_FIELD_NAME_BUILD(476 , 479 , Reserved      ));
}

/*******************************************************************************
*   lion3IpclKeyUdbBuildKey
*
* DESCRIPTION:
*        Lion3 IPCL - built UDB only keys
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - pointer to frame data buffer Id.
*       curLookUpCfgPtr - pointer to the current PCL configuration entry.
*       pclExtraDataPtr - pointer to extra data needed for the IPCL engine.
*       cycle           - PCL Lookup cycle number
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
static GT_VOID lion3IpclKeyUdbBuildKey
(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC  * curLookUpCfgPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC    * pclExtraDataPtr,
    IN GT_U32                             cycle,
    OUT SNET_CHT_POLICY_KEY_STC         * pclKeyPtr
)
{
    DECLARE_FUNC_NAME(lion3IpclKeyUdbBuildKey);

    GT_STATUS   rc;
    GT_U32  numOfUdbs = 0;
    GT_U32  udbIndexIterator;/*udb index iterator 0..(numOfUdbs-1)*/
    GT_U32  newUdbIndex; /*new udb index 0..49*/
    GT_U32  udbValid = 1;
    GT_U32  entryIndex_PCL_UDB_SELECT_TBL;/* index in ipcl udb select table */
    GT_U32  *memPtr;/*pointer to memory */
    GT_U8   udb49Value;
    GT_U8   *currKeyBytesPtr;
    SIP5_PACKET_CLASSIFICATION_TYPE_ENT pcl_pcktType_sip5;
    GT_CHAR*  udbReplaceNameArr[] =
    {
         "UDB0 - replaced with {PCL-ID[7:0]}"
        ,"UDB1 - replaced with {UDB Valid,reserved,PCL-ID[9:8]}"
        ,"UDB2 - replaced with eVLAN[7:0]"
        ,"UDB3 - replaced with eVLAN[12:8]"
        ,"UDB4 - replaced with Source-ePort[7:0]"
        ,"UDB5 - replaced with Source-ePort[12:8]"
        ,"UDB6 - replaced with Tag1-VID[7:0]"
        ,"UDB7 - replaced with MAC2ME and Tag1-{UP[2:0],VID[11:8]}"
        ,"UDB8 - replaced with QoS-Profile[7:0]"
        ,"UDB9 - replaced with QoS-Profile[9:8]"
        ,"UDB10 - replaced with Flow-ID[7:0]"
        ,"UDB11 - replaced with Flow-ID[11:8]"
    };
    GT_BIT  udbKeyBitmapEnable;
    GT_U32  numMaxUdbs    = SMEM_CHT_IS_SIP5_20_GET(devObjPtr) ? 70 : 50;
    GT_U32  numBitsPerUdb = SMEM_CHT_IS_SIP5_20_GET(devObjPtr) ?  7 : 6;
    GT_U32  startBitsAfterUdbs = (numMaxUdbs * numBitsPerUdb);

    __LOG_PARAM(numMaxUdbs);
    __LOG_PARAM(numBitsPerUdb);
    __LOG_PARAM(startBitsAfterUdbs);

    switch(pclKeyPtr->pclKeyFormat)
    {
        case CHT_PCL_KEY_10B_E:
        case CHT_PCL_KEY_20B_E:
        case CHT_PCL_KEY_30B_E:
        case CHT_PCL_KEY_40B_E:
        case CHT_PCL_KEY_50B_E:
            numOfUdbs = ((pclKeyPtr->pclKeyFormat - CHT_PCL_KEY_10B_E) + 1) * 10;
            __LOG(("UDB key[%d] bytes \n",
                numOfUdbs));
            break;
        case CHT_PCL_KEY_60B_E:
            numOfUdbs = 50;
            __LOG(("UDB key[%d] bytes \n",
                60));
            break;
        default:
            skernelFatalError("pclKeyFormat: not valid [%d]",pclKeyPtr->pclKeyFormat);
            break;
    }

    if(descrPtr->ipclMetadataReady == 0)
    {
        /* for optimization we build the metadata only when there is actual build of the first key */

        /* build the metadata before any changes that the PCL unit may do on the descriptor by the actions */
        snetLion3IpclUdbMetaDataBuild(devObjPtr,descrPtr,pclExtraDataPtr);

        descrPtr->ipclMetadataReady = 1;
    }

    pcl_pcktType_sip5 = descrPtr->ingressTunnelInfo.passengerDescrPtr->pcl_pcktType_sip5;
    __LOG_PARAM(pcl_pcktType_sip5);

    /*index = (KeySize * 16) + PacketType*/
    entryIndex_PCL_UDB_SELECT_TBL =
        ((pclKeyPtr->pclKeyFormat - CHT_PCL_KEY_10B_E) * 16) + pcl_pcktType_sip5;

    __LOG_PARAM(entryIndex_PCL_UDB_SELECT_TBL);

    memPtr = smemMemGet(devObjPtr,
        SMEM_LION3_PCL_UDB_SELECT_TBL_MEM(devObjPtr,entryIndex_PCL_UDB_SELECT_TBL,cycle));


    if((numOfUdbs == 50) && curLookUpCfgPtr->udbKeyBitmapEnable)
    {
        __LOG(("set to 0 the first 32 UDBs due to curLookUpCfgPtr->udbKeyBitmapEnable = 1 \n"));
        udbKeyBitmapEnable = 1;
    }
    else
    {
        udbKeyBitmapEnable = 0;
    }

    currKeyBytesPtr = &pclKeyPtr->key.triple[0];
    /* convert each udb index 0..(numOfUdbs-1) to the key into new udb index 0..49 */
    for(udbIndexIterator = 0 ; udbIndexIterator < numOfUdbs ; udbIndexIterator++,currKeyBytesPtr++)
    {
        if(udbIndexIterator != 0)
        {
            /* log the previous UDB info */
            __LOG(("udbIndexIterator[%d] got value[0x%x] \n",
                (udbIndexIterator-1),
                *(currKeyBytesPtr-1)));
        }

        if(udbKeyBitmapEnable && (udbIndexIterator < 32))
        {
            /*set to 0 the first 32 UDBs due to curLookUpCfgPtr->udbKeyBitmapEnable = 1*/

            /* the UDB is set only according to value of UDB 49 */
            /* we will set it at the end of the loop */
            *currKeyBytesPtr = 0;
            continue;
        }

        if(udbIndexIterator <= 11 &&
           snetFieldValueGet(memPtr,startBitsAfterUdbs + udbIndexIterator,1))
        {
            /* need to override this UDB with other value */
            switch(udbIndexIterator)
            {
                /*UDB0 - replaced with {PCL-ID[7:0]}*/
                case 0:
                    *currKeyBytesPtr = (GT_U8)curLookUpCfgPtr->pclID;
                    break;
                /*UDB1 - replaced with {UDB Valid,reserved,PCL-ID[9:8]}*/
                case 1:
                    /* NOTE: the <UDB Valid> wait for the end of all UDBs parse */
                    *currKeyBytesPtr = (GT_U8)(curLookUpCfgPtr->pclID >> 8);
                    break;
                /*UDB2 - replaced with eVLAN[7:0]*/
                case 2:
                    *currKeyBytesPtr = (GT_U8)(descrPtr->eVid);
                    break;
                /*UDB3 - replaced with eVLAN[12:8]*/
                case 3:
                    *currKeyBytesPtr = (GT_U8)(descrPtr->eVid >> 8);
                    break;
                /*UDB4 - replaced with Source-ePort[7:0]*/
                case 4:
                    *currKeyBytesPtr = (GT_U8)(descrPtr->origSrcEPortOrTrnk);
                    break;
                /*UDB5 - replaced with Source-ePort[12:8]*/
                case 5:
                    *currKeyBytesPtr = (GT_U8)(descrPtr->origSrcEPortOrTrnk >> 8);
                    break;
                /*UDB6 - replaced with Tag1-VID[7:0]*/
                case 6:
                    *currKeyBytesPtr = (GT_U8)(descrPtr->vid1);
                    break;
                /*UDB7 - replaced with MAC2ME and Tag1-{UP[2:0],VID[11:8]}*/
                case 7:
                    *currKeyBytesPtr = (GT_U8)((descrPtr->mac2me << 7) | (descrPtr->up1 << 4)  | (descrPtr->vid1 >> 8));
                    break;
                /*UDB8 - replaced with QoS-Profile[7:0]*/
                case 8:
                    *currKeyBytesPtr = (GT_U8)(descrPtr->qos.qosProfile);
                    break;
                /*UDB9 - replaced with QoS-Profile[9:8]*/
                case 9:
                    *currKeyBytesPtr = (GT_U8)(descrPtr->qos.qosProfile >> 8);
                    break;
                /*UDB10 - replaced with Flow-ID[7:0]*/
                case 10:
                    *currKeyBytesPtr = (GT_U8)(descrPtr->flowId);
                    break;
                /*UDB11 - replaced with Flow-ID[11:8]*/
                default:/*case 11:*/
                    *currKeyBytesPtr = (GT_U8)(descrPtr->flowId >> 8);
                    break;
            }

            __LOG((udbReplaceNameArr[udbIndexIterator]));

            /* ignore the UDB ... */
            continue;
        }


        newUdbIndex = snetFieldValueGet(memPtr,numBitsPerUdb*udbIndexIterator,numBitsPerUdb);

        __LOG(("udbIndexIterator[%d] mapped to newUdbIndex[%d] \n",
            udbIndexIterator,
            newUdbIndex));

        if(newUdbIndex >= numMaxUdbs)
        {
            __LOG((" newUdbIndex >= [%d] , so consider as 'UDB not valid' \n",
                numMaxUdbs));
            udbValid = 0;
            *currKeyBytesPtr = 0;

            continue;
        }

        rc = snetLionPclUdbKeyValueGet(devObjPtr,descrPtr,SMAIN_DIRECTION_INGRESS_E,
                newUdbIndex,currKeyBytesPtr);
        if(rc == GT_FAIL)
        {
            __LOG((" failed to get UDB so consider as 'UDB not valid' \n"));
            udbValid = 0;
            *currKeyBytesPtr = 0;
        }
    }

    /* log the last byte */
    __LOG(("udbIndexIterator[%d] got value[0x%x] \n",
        (udbIndexIterator-1),
        *(currKeyBytesPtr-1)));

    __LOG_PARAM(udbValid);

    if(udbKeyBitmapEnable)
    {
        /* set the needed bit according to value of UDB 49 */
        newUdbIndex = 49;
        rc = snetLionPclUdbKeyValueGet(devObjPtr,descrPtr,SMAIN_DIRECTION_INGRESS_E,
            newUdbIndex,&udb49Value);
        if(rc == GT_OK)
        {
            __LOG(("set to 1 single bit[%d] (bit index is the value of UDB 49) \n" ,
                udb49Value));
            snetFieldValueSet(pclKeyPtr->key.data , udb49Value , 1, 1);
        }
    }
    else
    if(udbValid == 1)
    {
        udbIndexIterator = 1;
        if(snetFieldValueGet(memPtr,startBitsAfterUdbs + udbIndexIterator,1))
        {
            /*UDB1 - replaced with {UDB Valid,reserved,PCL-ID[9:8]}*/
            currKeyBytesPtr = &pclKeyPtr->key.triple[udbIndexIterator];
            (*currKeyBytesPtr) |= udbValid << 7;
        }
    }


    if(pclKeyPtr->pclKeyFormat == CHT_PCL_KEY_60B_E)
    {
        /* build the extra 10 bytes of the key (after the first 50 UDBs) */
        __LOG(("build the extra 10 bytes of the key (after the first 50 UDBs)"));
        lion3IpclKey60UdbBuildKeyExtra10Bytes(devObjPtr, descrPtr, curLookUpCfgPtr,
                pclExtraDataPtr,pclKeyPtr , udbValid);
    }

    return;
}

/*******************************************************************************
* snetXCatIPclCreateKey
*
* DESCRIPTION:
*        Create PCL search key.
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - pointer to frame data buffer Id.
*       curLookUpCfgPtr - pointer to the PCL configuration table entry.
*       pclExtraDataPtr - pointer to extra data needed for the PCL engine.
*       keyType         - key type used for PCL lookup
*       cycle           - PCL Lookup cycle number
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
static GT_VOID snetXCatIPclCreateKey
(
    IN SKERNEL_DEVICE_OBJECT                * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC   * descrPtr,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC      * lookUpCfgPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC        * pclExtraDataPtr,
    IN SNET_XCAT_IPCL_KEY_TYPE_ENT            keyType,
    IN GT_U32                                 cycle,
    OUT SNET_CHT_POLICY_KEY_STC             * pclKeyPtr
)
{
    DECLARE_FUNC_NAME(snetXCatIPclCreateKey);

    GT_STATUS rc;
    DXCH_IPCL_KEY_ENT   keyIndex;

    /* Partial mode creation: relies on PCL lookup cycle and dual lookup mode */
    __LOG(("Partial mode creation: relies on PCL lookup cycle and dual lookup mode"));
    rc = snetXCatIPclPartialModeKeyCreate(devObjPtr,
                                          lookUpCfgPtr,
                                          cycle, pclKeyPtr);
    if(rc == GT_OK)
    {
        /* Partial mode key has been successfully created */
        __LOG(("Partial mode key has been successfully created"));
        return;
    }

    memset(&pclKeyPtr->key, 0, sizeof(pclKeyPtr->key));

    pclKeyPtr->devObjPtr = devObjPtr;
    pclKeyPtr->updateOnlyDiff = GT_FALSE;
    pclKeyPtr->pclKeyFormat = lookUpCfgPtr->keySize;

    if(LION3_IPCL_KEY_IS_UDB_MAC(devObjPtr,keyType))
    {
        __LOG(("Start Build UDB Key for lookup[%d] \n",cycle));

        /* UDB only keys 10..50, 50+10(60) */
        lion3IpclKeyUdbBuildKey(devObjPtr, descrPtr,
                            lookUpCfgPtr,
                            pclExtraDataPtr,
                            cycle,
                            pclKeyPtr);

        __LOG(("End of Build UDB Key for lookup[%d] \n",cycle));
        return;
    }

    if(keyType >= XCAT_IPCL_KEY_TYPE_LAST_E)
    {
        skernelFatalError("keyType: not valid [%d]",keyType);
        keyType = 0;
    }

    keyIndex = xcatIpclKeyInfoArr[keyType].keyIndex;

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        if(keyIndex >= DXCH_IPCL_KEY_LAST_E)
        {
            skernelFatalError("keyIndex: not valid [%d]",keyIndex);
            keyIndex = 0;
        }

        __LOG(("Build Key#[%d] [%s] \n",
            keyIndex,
            lion3IpclKeyInfoArr[keyIndex].nameString));

        /* build the key (without UDBs and <User Defined Valid>)*/
        lion3IpclKeyInfoArr[keyIndex].buildKeyFunc(devObjPtr,
                            descrPtr->ingressTunnelInfo.fixedFieldsTypeDescrPtr,
                            lookUpCfgPtr,
                            pclExtraDataPtr,
                            keyIndex,
                            pclKeyPtr);

        __LOG(("Add the relevant UDBs to the Key#[%d] [%s] \n",
            keyIndex,
            lion3IpclKeyInfoArr[keyIndex].nameString));
        /* build all the UDBs for this key and the <User Defined Valid> */
        lion3PclBuildAllUdbsForKey(devObjPtr, descrPtr,
            pclKeyPtr,keyType,keyIndex);
    }
    else
    {
        __LOG(("Build Key#[%d] [%s] \n",
            keyIndex,
            xcatIpclKeyInfoArr[keyType].nameString));
        /* build the key (with UDBs and <User Defined Valid>)*/
        xcatIpclKeyInfoArr[keyType].buildKeyFunc(devObjPtr,
                            descrPtr->ingressTunnelInfo.passengerDescrPtr,
                            lookUpCfgPtr,
                            pclExtraDataPtr,
                            keyType,
                            pclKeyPtr);

    }

    if(devObjPtr->pclSupport.iPclSupportTrunkHash)
    {
        __LOG(("check to override UDB with trunk hash in key#[%d] \n",
            keyIndex));
        snetLionIPclOverrideUdbKeyWithPacketHash(devObjPtr,
            descrPtr->ingressTunnelInfo.passengerDescrPtr,
            keyIndex, cycle, pclKeyPtr);
    }

    /* PCL Port-List mode - override built key with Port-list bits */
    if (lookUpCfgPtr->pclPortListModeEn)
    {
        __LOG(("Override key info with 'ports bitmap' in key#[%d] \n",
            keyIndex));
        snetXcat2IpclKeyPortsBmpBuild(devObjPtr, descrPtr,
                               pclKeyPtr,NULL);
    }
}

/*******************************************************************************
* snetXCatIPclActionEgressInterfaceSet
*
* DESCRIPTION:
*       Get the egress interface data from the policy action entry.
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       descrPtr            - pointer to frame descriptor.
*       actionEntryDataPtr  - pointer to Policy action table entry.
*
* OUTPUTS:
*       actionInfoPtr   - pointer to the PCL action.
*
* RETURN:
*
* COMMENTS:
*
*******************************************************************************/
static GT_VOID snetXCatIPclActionEgressInterfaceSet
(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr,
    IN GT_U32                           * actionEntryDataPtr,
    OUT SNET_XCAT_PCL_ACTION_STC        * actionDataPtr
)
{
    DECLARE_FUNC_NAME(snetXCatIPclActionEgressInterfaceSet);

    GT_U32  fieldVal;
    GT_U32  enable = 1;

    /* Use VIDX */
    fieldVal = SMEM_U32_GET_FIELD(actionEntryDataPtr[0], 27, 1);
    SNET_SET_DEST_INTERFACE_TYPE(
        CNV_CHT2_ACTION_MAC(actionDataPtr)->pceEgressIf,
        SNET_DST_INTERFACE_VIDX_E, fieldVal);

    if (SNET_GET_DEST_INTERFACE_TYPE(
        CNV_CHT2_ACTION_MAC(actionDataPtr)->pceEgressIf, SNET_DST_INTERFACE_VIDX_E))
    {
        CNV_CHT2_ACTION_MAC(actionDataPtr)->pceEgressIf.interfaceInfo.vidx =
            (GT_U16)SMEM_U32_GET_FIELD(actionEntryDataPtr[0], 15, 12);
    }
    else
    {
        /* Egress interface trunk */
        __LOG(("Egress interface trunk"));
        fieldVal = (GT_U8)SMEM_U32_GET_FIELD(actionEntryDataPtr[0], 15, 1);
        SNET_SET_DEST_INTERFACE_TYPE(
            CNV_CHT2_ACTION_MAC(actionDataPtr)->pceEgressIf,
            SNET_DST_INTERFACE_TRUNK_E, fieldVal);
        if (SNET_GET_DEST_INTERFACE_TYPE(
            CNV_CHT2_ACTION_MAC(actionDataPtr)->pceEgressIf,
            SNET_DST_INTERFACE_TRUNK_E))
        {
            CNV_CHT2_ACTION_MAC(actionDataPtr)->pceEgressIf.interfaceInfo.trunkId =
                    (GT_U8)SMEM_U32_GET_FIELD(actionEntryDataPtr[0], 16, 7);
        }
        else
        {
            SNET_SET_DEST_INTERFACE_TYPE(
                CNV_CHT2_ACTION_MAC(actionDataPtr)->pceEgressIf,
                SNET_DST_INTERFACE_PORT_E, enable);

            CNV_CHT2_ACTION_MAC(actionDataPtr)->pceEgressIf.interfaceInfo.devPort.port =
                    (GT_U8)SMEM_U32_GET_FIELD(actionEntryDataPtr[0], 16, 6);
            CNV_CHT2_ACTION_MAC(actionDataPtr)->pceEgressIf.interfaceInfo.devPort.devNum =
                    (GT_U8)SMEM_U32_GET_FIELD(actionEntryDataPtr[0], 22, 5);
        }
    }

    SNET_SET_EXT_EGRESS_DEST_INTERFACE(
            CNV_CHT3_ACTION_MAC(actionDataPtr),
            CNV_CHT2_ACTION_MAC(actionDataPtr)->pceEgressIf.interfaceInfo);
}

/*******************************************************************************
* snetXCatIPclActionGet
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
static GT_VOID snetXCatIPclActionGet
(
    IN SKERNEL_DEVICE_OBJECT            *   devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  *   descrPtr  ,
    IN GT_U32                               matchIndex,
    OUT SNET_XCAT_PCL_ACTION_STC        *   actionDataPtr
)
{
    DECLARE_FUNC_NAME(snetXCatIPclActionGet);

    GT_U32  *actionEntryDataPtr;
    GT_U32  tblAddr;
    GT_U32  fieldVal;
    GT_U32  enable = 1;

    tblAddr = SMEM_CHT_PCL_ACTION_TBL_MEM(devObjPtr, matchIndex);
    actionEntryDataPtr = smemMemGet(devObjPtr, tblAddr);

    CNV_CHT2_ACTION_MAC(actionDataPtr)->fwdCmd =
        snetFieldValueGet(actionEntryDataPtr, 0, 3);
    CNV_CHT2_ACTION_MAC(actionDataPtr)->pceRedirectCmd =
        snetFieldValueGet(actionEntryDataPtr, 12, 3);

    descrPtr->pclRedirectCmd = CNV_CHT2_ACTION_MAC(actionDataPtr)->pceRedirectCmd;

    if (CNV_CHT2_ACTION_MAC(actionDataPtr)->pceRedirectCmd == PCL_TTI_ACTION_REDIRECT_CMD_PBR_OR_VPLS_E)
    {
        /* special case: action as nexthop */
        __LOG(("special case: action as nexthop"));

        /* Store nexthop info index internally into descriptor  */
        descrPtr->pclUcNextHopIndex = matchIndex;

        /* we need only the match counters info got get in the PCL unit */
        CNV_CHT2_ACTION_MAC(actionDataPtr)->matchCounterIndex =
            snetFieldValueGet(actionEntryDataPtr, 29, 5);
        CNV_CHT2_ACTION_MAC(actionDataPtr)->matchCounterEn =
            snetFieldValueGet(actionEntryDataPtr, 28, 1);

        /* the router engine parse the PCL action according to
           descrPtr->pclUcNextHopIndex */

        return;
    }


    /* regular IPCL action */
    __LOG(("regular IPCL action"));

    CNV_CHT2_ACTION_MAC(actionDataPtr)->pceCpuCode =
        snetFieldValueGet(actionEntryDataPtr, 3, 8);
    CNV_CHT2_ACTION_MAC(actionDataPtr)->mirrorToIngressAnalyzerPort =
        snetFieldValueGet(actionEntryDataPtr, 11, 1);
    /* use the new action fields only when <VPLS Mode>==0      */
    /* for VPLS mode these bits are part of Unknown SA command */
    if(devObjPtr->vplsModeEnable.ipcl == 0)
    {
        CNV_CHT2_ACTION_MAC(actionDataPtr)->mirrorToIngressAnalyzerPort |=
            snetFieldValueGet(actionEntryDataPtr, 114, 2) << 1;
    }


    switch(CNV_CHT2_ACTION_MAC(actionDataPtr)->pceRedirectCmd)
    {
        case PCL_TTI_ACTION_REDIRECT_CMD_OUT_IF_E:
            SNET_SET_DEST_INTERFACE_TYPE(
                CNV_CHT3_ACTION_MAC(actionDataPtr)->pceExtEgressIf,
                SNET_DST_INTERFACE_EGRESS_E, enable);

            /* Redirect to egress interface */
            __LOG(("Redirect to egress interface"));
            snetXCatIPclActionEgressInterfaceSet(devObjPtr, descrPtr,
                                                 actionEntryDataPtr,
                                                 actionDataPtr);
            CNV_CHT2_ACTION_MAC(actionDataPtr)->tunnelStart =
                snetFieldValueGet(actionEntryDataPtr, 28, 1);
            /* Tunnel Start */
            __LOG(("Tunnel Start"));
            if(CNV_CHT2_ACTION_MAC(actionDataPtr)->tunnelStart)
            {
                CNV_CHT2_ACTION_MAC(actionDataPtr)->tunnelPtr =
                    snetFieldValueGet(actionEntryDataPtr, 29, 13);
                CNV_CHT2_ACTION_MAC(actionDataPtr)->tunnelType =
                    snetFieldValueGet(actionEntryDataPtr, 42, 1);
            }
            else
            {
                actionDataPtr->arpPointer =
                    snetFieldValueGet(actionEntryDataPtr, 29, 14);
                CNV_CHT2_ACTION_MAC(actionDataPtr)->VNTL2Echo =
                    snetFieldValueGet(actionEntryDataPtr, 43, 1);
            }

            break;
        case PCL_TTI_ACTION_REDIRECT_CMD_LTT_ROUTER_E:
            SNET_SET_DEST_INTERFACE_TYPE(
                CNV_CHT3_ACTION_MAC(actionDataPtr)->pceExtEgressIf,
                SNET_DST_INTERFACE_LLT_INDEX_E, enable);

            /* PCERouter LTTIndex[15:0] */
            fieldVal = snetFieldValueGet(actionEntryDataPtr, 15, 15);
            /* LLT router */
            __LOG(("LLT router"));
            SNET_SET_EXT_LLT_DEST_INTERFACE(
                CNV_CHT3_ACTION_MAC(actionDataPtr), fieldVal);

            break;
        case PCL_TTI_ACTION_REDIRECT_CMD_VIRT_ROUTER_E:
            SNET_SET_DEST_INTERFACE_TYPE(
                CNV_CHT3_ACTION_MAC(actionDataPtr)->pceExtEgressIf,
                SNET_DST_INTERFACE_VIRTUAL_ROUTER_E, enable);

            fieldVal = snetFieldValueGet(actionEntryDataPtr, 15, 12);
            /* Virtual router */
            __LOG(("Virtual router"));
            SNET_SET_EXT_VR_DEST_INTERFACE(
                CNV_CHT3_ACTION_MAC(actionDataPtr), fieldVal);

            break;
        case PCL_TTI_ACTION_REDIRECT_CMD_ASSIGN_SOURCE_LOGICAL_PORT_E:/* Assign Logic Port */
         /* 15        :        15        1        Reserved
            21        :        16        6        VirtualSrcPort
            26        :        22        5        VirtualSrcDev
            29        :        27        3        Reserved
            30        :        30        1        Reserved
            43        :        31        13        IPCL Profile Index */
        actionDataPtr->srcLogicPortInfo.srcIsTrunk = 0;
        actionDataPtr->srcLogicPortInfo.srcTrunkOrPortNum = snetFieldValueGet(actionEntryDataPtr,16,6);
        actionDataPtr->srcLogicPortInfo.srcDevice = snetFieldValueGet(actionEntryDataPtr,22,5);
            break;
        default:
            break;
    }
    /* Pointer to IPCL configuration entry to be used when fetching IPCL parameter */
    if(CNV_CHT2_ACTION_MAC(actionDataPtr)->pceRedirectCmd == PCL_TTI_ACTION_REDIRECT_CMD_OUT_IF_E)
    {
        actionDataPtr->ipclProfileIndex = 0;
    }
    else
    {
        actionDataPtr->ipclProfileIndex =
            snetFieldValueGet(actionEntryDataPtr, 31, 13);
    }

    /* Read words 1, 2 and 3 from the action table entry */
    CNV_CHT2_ACTION_MAC(actionDataPtr)->matchCounterEn =
        snetFieldValueGet(actionEntryDataPtr, 44, 1);
    CNV_CHT2_ACTION_MAC(actionDataPtr)->matchCounterIndex =
        snetFieldValueGet(actionEntryDataPtr, 45, 14);
    CNV_CHT2_ACTION_MAC(actionDataPtr)->policerEn =
        snetFieldValueGet(actionEntryDataPtr, 59, 1);
    CNV_CHT2_ACTION_MAC(actionDataPtr)->policerIndex =
        snetFieldValueGet(actionEntryDataPtr, 61, 12);
    CNV_CHT2_ACTION_MAC(actionDataPtr)->pceVidPrecedence =
        snetFieldValueGet(actionEntryDataPtr, 84, 1);
    CNV_CHT2_ACTION_MAC(actionDataPtr)->enNestedVlan =
        snetFieldValueGet(actionEntryDataPtr, 85, 1);
    CNV_CHT2_ACTION_MAC(actionDataPtr)->pceVlanCmd =
        snetFieldValueGet(actionEntryDataPtr, 86, 2);
    CNV_CHT2_ACTION_MAC(actionDataPtr)->pceVid =
        snetFieldValueGet(actionEntryDataPtr, 88, 12);
    CNV_CHT2_ACTION_MAC(actionDataPtr)->pceQosProfileMarkingEn =
        snetFieldValueGet(actionEntryDataPtr, 100, 1);
    CNV_CHT2_ACTION_MAC(actionDataPtr)->pceQosPrecedence =
        snetFieldValueGet(actionEntryDataPtr, 101, 1);
    CNV_CHT2_ACTION_MAC(actionDataPtr)->pceQosProfile =
        snetFieldValueGet(actionEntryDataPtr, 102, 7);
    CNV_CHT2_ACTION_MAC(actionDataPtr)->pceModifyDscp =
        snetFieldValueGet(actionEntryDataPtr, 109, 2);
    CNV_CHT2_ACTION_MAC(actionDataPtr)->pceModifyUp =
        snetFieldValueGet(actionEntryDataPtr, 111, 2);

    CNV_CHT3_ACTION_MAC(actionDataPtr)->srcIdSetEn =
        snetFieldValueGet(actionEntryDataPtr, 73, 1);
    CNV_CHT3_ACTION_MAC(actionDataPtr)->srcId =
        snetFieldValueGet(actionEntryDataPtr, 74, 5);
    CNV_CHT3_ACTION_MAC(actionDataPtr)->actionStop =
        snetFieldValueGet(actionEntryDataPtr, 79, 1);

    actionDataPtr->policerCounterEn =
        snetFieldValueGet(actionEntryDataPtr, 60, 1);
    actionDataPtr->bypassBridge =
        snetFieldValueGet(actionEntryDataPtr, 80, 1);
    actionDataPtr->bypassIngressPipe =
        snetFieldValueGet(actionEntryDataPtr, 81, 1);
    actionDataPtr->policy2LookUpMode =
        snetFieldValueGet(actionEntryDataPtr, 82, 1);
    actionDataPtr->policy1LookUpMode =
        snetFieldValueGet(actionEntryDataPtr, 83, 1);
    actionDataPtr->tcpRstFinMirrorEn =
        snetFieldValueGet(actionEntryDataPtr, 113, 1);

    if(SKERNEL_IS_LION_REVISON_B0_DEV(devObjPtr))
    {
        CNV_LION_ACTION_MAC(actionDataPtr)->modifyMacDa =
            snetFieldValueGet(actionEntryDataPtr, 116, 1);
        CNV_LION_ACTION_MAC(actionDataPtr)->modifyMacSa =
            snetFieldValueGet(actionEntryDataPtr, 117, 1);
    }

    /* use the new action fields only when <VPLS Mode>==1 */
    if(devObjPtr->vplsModeEnable.ipcl)
    {
        actionDataPtr->vplsInfo.unknownSaCmd =
            snetFieldValueGet(actionEntryDataPtr,58,1) | snetFieldValueGet(actionEntryDataPtr,114,2) << 1;

        /* flag internal for the simulation to remove the use of value '7' from the unknownSaCmd */
        if(actionDataPtr->vplsInfo.unknownSaCmd > 5)
        {
            actionDataPtr->vplsInfo.unknownSaCmdAssigned = 0;
        }
        else
        {
            actionDataPtr->vplsInfo.unknownSaCmdAssigned = 1;
        }

        /* <Unknown SA Cmd[0]> bit come instead of <CNC Counter Index> bits 13 */
        SMEM_U32_SET_FIELD(CNV_CHT2_ACTION_MAC(actionDataPtr)->matchCounterIndex,13,1,0);

        if(CNV_CHT2_ACTION_MAC(actionDataPtr)->pceRedirectCmd == PCL_TTI_ACTION_REDIRECT_CMD_ASSIGN_SOURCE_LOGICAL_PORT_E)
        {
            actionDataPtr->vplsInfo.srcMeshIdEn =
                    snetFieldValueGet(actionEntryDataPtr, 30, 1);

            actionDataPtr->vplsInfo.userTaggedAcEnable =
                snetFieldValueGet(actionEntryDataPtr, 27, 1);

            actionDataPtr->vplsInfo.srcMeshId =
                snetFieldValueGet(actionEntryDataPtr, 28, 2);
        }
    }
}

/*******************************************************************************
* snetXCatIPclActionApply
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
*
* OUTPUTS:
*
* RETURN:
*
* COMMENTS:
*       Policy Tcam Table : The policy TCAM holds 1024 lines
*       of 24 bytes or rule data used for ingress and egress PCL.
*******************************************************************************/
static GT_VOID snetXCatIPclActionApply
(
    IN SKERNEL_DEVICE_OBJECT                * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC   * descrPtr  ,
    IN SNET_XCAT_PCL_ACTION_STC             * actionDataPtr,
    IN GT_BOOL                              isFirst
)
{
    DECLARE_FUNC_NAME(snetXCatIPclActionApply);

    GT_U32  vid1 = 0,up1 = 0,cfidei1 = 0,origVid = 0;


    if (devObjPtr->vplsModeEnable.ipcl &&
        CNV_CHT2_ACTION_MAC(actionDataPtr)->pceRedirectCmd ==  PCL_TTI_ACTION_REDIRECT_CMD_ASSIGN_SOURCE_LOGICAL_PORT_E)/* Assign Logic Port */
    {
        /* must be done before calling the snetCht3IPclActionApply(...) because :
           this configuration is applied BEFORE applying the IPCL AE<VID0 Command>. */
        if(actionDataPtr->vplsInfo.userTaggedAcEnable)
        {
            if(descrPtr->tagSrcTagged[SNET_CHT_TAG_0_INDEX_CNS] == 0)
            {
                /* packet arrived Untagged. If packet arrived DSA, this is <SrcTag> from DSA*/
                __LOG(("packet arrived Untagged. If packet arrived DSA, this is <SrcTag> from DSA"));
                vid1 = 0;
                up1 = 0;
                cfidei1 = 0;
                origVid = descrPtr->eVid;/*DescOut<OrigVID> = DescIn<VID0>  This is the PVID on the original ingress port
                            Note it would be better if we could assign the default from the IPCL AE, but we dont have the
                            <VID1> field like we have in TTI AE. So we have to rely on the AC port PVID.*/
            }
            else
            {
                vid1 = descrPtr->eVid & 0xFFF;
                up1 = descrPtr->up;
                cfidei1 = descrPtr->cfidei;
                origVid = descrPtr->eVid;
            }

        }
    }



    snetCht3IPclActionApply(devObjPtr, descrPtr, CNV_CHT3_ACTION_MAC(actionDataPtr),isFirst);
    /* PCL action as Next Hop */
    if (CNV_CHT2_ACTION_MAC(actionDataPtr)->pceRedirectCmd == PCL_TTI_ACTION_REDIRECT_CMD_PBR_OR_VPLS_E)
    {
        __LOG(("PCL action as Next Hop"));
        return;
    }

    if (CNV_CHT2_ACTION_MAC(actionDataPtr)->pceRedirectCmd ==  PCL_TTI_ACTION_REDIRECT_CMD_ASSIGN_SOURCE_LOGICAL_PORT_E)/* Assign Logic Port */
    {
        descrPtr->useVidx = 0;
        descrPtr->targetIsTrunk = actionDataPtr->srcLogicPortInfo.srcIsTrunk;
        if(descrPtr->targetIsTrunk)
        {
            descrPtr->trgTrunkId = actionDataPtr->srcLogicPortInfo.srcTrunkOrPortNum;
        }
        else
        {
            descrPtr->trgDev = actionDataPtr->srcLogicPortInfo.srcDevice;
            descrPtr->trgEPort = actionDataPtr->srcLogicPortInfo.srcTrunkOrPortNum;
        }
    }



    /* Bind to Policer Counter */
    if (actionDataPtr->policerCounterEn == GT_TRUE)
    {
        descrPtr->policerCounterEn = actionDataPtr->policerCounterEn;
        /* Policer Counter assigned direct from PCL action   */
        __LOG(("Policer Counter assigned direct from PCL action"));
        if(descrPtr->policerPtr == 0)
        {
            descrPtr->policerPtr =
                ((SNET_CHT2_PCL_ACTION_STC *)actionDataPtr)->policerIndex;
        }
    }

    if(actionDataPtr->policy1LookUpMode)
    {
        descrPtr->pclLookUpMode[1] = SKERNEL_PCL_LOOKUP_MODE_DESCRIPTOR_INDEX_E;
        if (actionDataPtr->ipclProfileIndex)
        {
            descrPtr->ipclProfileIndex =   actionDataPtr->ipclProfileIndex;
        }
    }

    if(actionDataPtr->policy2LookUpMode)
    {
        descrPtr->pclLookUpMode[2] = SKERNEL_PCL_LOOKUP_MODE_DESCRIPTOR_INDEX_E;
        if (actionDataPtr->ipclProfileIndex)
        {
            descrPtr->ipclProfileIndex =   actionDataPtr->ipclProfileIndex;
        }
    }

    /* TCP FIN/RST mirror to CPU action */
    __LOG(("TCP FIN/RST mirror to CPU action"));
    if (actionDataPtr->tcpRstFinMirrorEn == GT_TRUE)
    {
        /* check bits 0 (FIN) and 2 (RST) of TCP flags */
        if (descrPtr->isIp && (descrPtr->ipProt == SNET_TCP_PROT_E) &&
            descrPtr->l4StartOffsetPtr &&
           (descrPtr->l4StartOffsetPtr[13] & SNET_XCAT_TCP_FIN_RST_MASK_CNS))
        {
            /* send packet to CPU for TCP RST or FIN.
               CPU code is already set by snetCht2IPclActionApply */
            descrPtr->packetCmd = snetChtPktCmdResolution(descrPtr->packetCmd,
                SKERNEL_EXT_PKT_CMD_MIRROR_TO_CPU_E);
        }
    }

    if(SKERNEL_IS_LION_REVISON_B0_DEV(devObjPtr))
    {
        if (CNV_CHT2_ACTION_MAC(actionDataPtr)->pceRedirectCmd == PCL_TTI_ACTION_REDIRECT_CMD_OUT_IF_E)
        {
            descrPtr->egressFilterRegistered = 1;
            if(descrPtr->modifyMacDa == 0)
            {
                descrPtr->modifyMacDa =
                    CNV_LION_ACTION_MAC(actionDataPtr)->modifyMacDa;
            }
            if(descrPtr->modifyMacSa == 0)
            {
                descrPtr->modifyMacSa =
                    CNV_LION_ACTION_MAC(actionDataPtr)->modifyMacSa;
            }

            if (CNV_CHT2_ACTION_MAC(actionDataPtr)->tunnelStart == 0 &&
                CNV_LION_ACTION_MAC(actionDataPtr)->modifyMacDa == 1)
            {
                /* Pointer to the ARP entry */
                descrPtr->arpPtr = actionDataPtr->arpPointer;
            }
        }
    }

    if(devObjPtr->vplsModeEnable.ipcl)
    {
        if(actionDataPtr->vplsInfo.unknownSaCmdAssigned)
        {
            descrPtr->vplsInfo.unknownSaCmd            = actionDataPtr->vplsInfo.unknownSaCmd;
            descrPtr->vplsInfo.unknownSaCmdAssigned    = 1;
        }
        /* check if PCL can override previous assignment */
        __LOG(("check if PCL can override previous assignment"));
        if(actionDataPtr->vplsInfo.srcMeshIdEn)
        {
            descrPtr->vplsInfo.srcMeshId               = actionDataPtr->vplsInfo.srcMeshId;
        }
        descrPtr->vplsInfo.userTaggedAcEnable     = actionDataPtr->vplsInfo.userTaggedAcEnable;

        if (CNV_CHT2_ACTION_MAC(actionDataPtr)->pceRedirectCmd ==  PCL_TTI_ACTION_REDIRECT_CMD_ASSIGN_SOURCE_LOGICAL_PORT_E)/* Assign Logic Port */
        {
            if(actionDataPtr->vplsInfo.userTaggedAcEnable)
            {
                descrPtr->vid1 = vid1;
                descrPtr->up1 = up1;
                descrPtr->cfidei1 = cfidei1;
                descrPtr->vid0Or1AfterTti = origVid & 0xFFF;
                /*DescOut<Override VID0 with OrigVID> is 1.*/
                descrPtr->overrideVid0WithOrigVid = 1;
            }
        }
    }

    if(descrPtr->bypassBridge == 0 &&
       actionDataPtr->bypassBridge)
    {
        /* bridge is bypassed for this packet */
        __LOG(("Bridge is bypassed for this packet"));
        descrPtr->bypassBridge = 1;
    }

    if(descrPtr->bypassIngressPipe == 0 &&
       actionDataPtr->bypassIngressPipe)
    {
        /* Ingress pipe is bypassed for this packet */
        __LOG(("Ingress pipe is bypassed for this packet"));
        descrPtr->bypassIngressPipe = 1;
    }
}

/*******************************************************************************
* snetLion3IPclActionGet
*
* DESCRIPTION:
*       Lion3 : Get the action entry from the policy action table.
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
static GT_VOID snetLion3IPclActionGet
(
    IN SKERNEL_DEVICE_OBJECT            *   devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  *   descrPtr  ,
    IN GT_U32                               matchIndex,
    OUT SNET_LION3_PCL_ACTION_STC       *   actionDataPtr
)
{
    GT_U32  *memPtr;
    GT_U32  value,value1;
    SNET_LION3_IPCL_ACTION_REDIRECT_TO_EGRESS_INTERFACE_STC *redirectEgressInfoPtr;

    memPtr = smemMemGet(devObjPtr, SMEM_LION3_TCAM_ACTION_TBL_MEM(devObjPtr, matchIndex));

    actionDataPtr->packetCmd = SMEM_LION3_IPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_COMMAND);
    actionDataPtr->cpuCode = SMEM_LION3_IPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_CPU_CODE);

    if(!SMEM_CHT_IS_SIP5_20_GET(devObjPtr))
    {
        actionDataPtr->mirrorToIngressAnalyzerPort =
            SMEM_LION3_IPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_MIRROR_TO_ANALYZER_PORT_0) |
            SMEM_LION3_IPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_MIRROR_TO_ANALYZER_PORT_2_1) << 1;
    }
    else
    {
        actionDataPtr->mirrorToIngressAnalyzerPort =
            SMEM_SIP5_20_IPCL_ACTION_ENTRY_FIELD_GET(SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_MIRROR_TO_ANALYZER_PORT);
    }

    actionDataPtr->redirectCmd = SMEM_LION3_IPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_REDIRECT_COMMAND);

    if(actionDataPtr->redirectCmd == PCL_TTI_ACTION_REDIRECT_CMD_NONE_E ||
       actionDataPtr->redirectCmd == PCL_TTI_ACTION_REDIRECT_CMD_LTT_ROUTER_E ||
       actionDataPtr->redirectCmd == PCL_TTI_ACTION_REDIRECT_CMD_VIRT_ROUTER_E)
    {
        actionDataPtr->ipclProfile.lookup1Mode =
            SMEM_LION3_IPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_POLICY1_LOOKUP_MODE);
        actionDataPtr->ipclProfile.lookup2Mode =
            SMEM_LION3_IPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_POLICY2_LOOKUP_MODE);
        actionDataPtr->ipclProfile.profileIndex =
            SMEM_LION3_IPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_IPCL_PROFILE_INDEX);
    }

    if(actionDataPtr->redirectCmd != PCL_TTI_ACTION_REDIRECT_CMD_OUT_IF_E)
    {
        actionDataPtr->setEgressFilterRegistered =
            SMEM_LION3_IPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_SET_EGRESS_FILTER_REGISTERED);
    }

    if(actionDataPtr->redirectCmd != PCL_TTI_ACTION_REDIRECT_CMD_REPLACE_MAC_SA_E)
    {
        actionDataPtr->bindToPolicerMeter =
            SMEM_LION3_IPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_ACTIVATE_METER);
        actionDataPtr->bindToPolicerCounter =
            SMEM_LION3_IPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_BIND_TO_POLICER_COUNTER);
        actionDataPtr->policerIndex =
            SMEM_LION3_IPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_POLICER_PTR);
    }

    switch(actionDataPtr->redirectCmd)
    {
        case PCL_TTI_ACTION_REDIRECT_CMD_OUT_IF_E:
            redirectEgressInfoPtr = &actionDataPtr->redirectEgressInfo;

            value = SMEM_LION3_IPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_USE_VIDX);

            if(value)
            {
                redirectEgressInfoPtr->egressInterface.dstInterface = SNET_DST_INTERFACE_VIDX_E;
                redirectEgressInfoPtr->egressInterface.interfaceInfo.vidx = (GT_U16)
                    SMEM_LION3_IPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_VIDX);
            }
            else
            {
                value = SMEM_LION3_IPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_IS_TRUNK);
                if(value)
                {
                    redirectEgressInfoPtr->egressInterface.dstInterface = SNET_DST_INTERFACE_TRUNK_E;
                    redirectEgressInfoPtr->egressInterface.interfaceInfo.trunkId = (GT_U16)
                        SMEM_LION3_IPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_TRUNK_ID);
                }
                else
                {
                    redirectEgressInfoPtr->egressInterface.dstInterface = SNET_DST_INTERFACE_PORT_E;
                    redirectEgressInfoPtr->egressInterface.interfaceInfo.devPort.port =
                        SMEM_LION3_IPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_TRG_PORT);
                    redirectEgressInfoPtr->egressInterface.interfaceInfo.devPort.devNum =
                        SMEM_LION3_IPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_TARGET_DEVICE);
                }
            }

            redirectEgressInfoPtr->modifyMacSaEn =
                SMEM_LION3_IPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_MODIFY_MAC_SA);
            redirectEgressInfoPtr->VNTL2Echo =
                SMEM_LION3_IPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_VNT_L2_ECHO);
            redirectEgressInfoPtr->tunnelStart =
                SMEM_LION3_IPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_TUNNEL_START);
            if(redirectEgressInfoPtr->tunnelStart)
            {
                redirectEgressInfoPtr->tsOrArpInfo.tsInfo.index =
                    SMEM_LION3_IPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_TUNNEL_POINTER);
                redirectEgressInfoPtr->tsOrArpInfo.tsInfo.passengerType =
                    SMEM_LION3_IPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_TUNNEL_START_PASSENGER_TYPE);
            }
            else
            {
                redirectEgressInfoPtr->tsOrArpInfo.arpInfo.index =
                    SMEM_LION3_IPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_ARP_POINTER);
            }

            break;

        case PCL_TTI_ACTION_REDIRECT_CMD_LTT_ROUTER_E:
            actionDataPtr->routerLLTIndex =
                SMEM_LION3_IPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_IP_NEXT_HOP_ENTRY_INDEX);
            break;
        case PCL_TTI_ACTION_REDIRECT_CMD_VIRT_ROUTER_E:
            actionDataPtr->vrfId =
                SMEM_LION3_IPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_VRF_ID);
            break;
        case PCL_TTI_ACTION_REDIRECT_CMD_REPLACE_MAC_SA_E:
            if(SMEM_CHT_IS_SIP5_20_GET(devObjPtr) == 0)
            {
                value  = SMEM_LION3_IPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_MAC_SA_29_0);
                value1 = SMEM_LION3_IPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_MAC_SA_47_30);
                actionDataPtr->macSa[5] = (GT_U8) (value >> 0);
                actionDataPtr->macSa[4] = (GT_U8) (value >> 8);
                actionDataPtr->macSa[3] = (GT_U8) (value >>16);
                actionDataPtr->macSa[2] = (GT_U8) ((value >> 24) | (value1 << 6));
                actionDataPtr->macSa[1] = (GT_U8) (value1 >> 2);
                actionDataPtr->macSa[0] = (GT_U8) (value1 >> 10);
            }
            else
            {
                value  = SMEM_SIP5_20_IPCL_ACTION_ENTRY_FIELD_GET(SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_MAC_SA_27_0);
                value1 = SMEM_SIP5_20_IPCL_ACTION_ENTRY_FIELD_GET(SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_MAC_SA_47_28);
                actionDataPtr->macSa[5] = (GT_U8) (value >> 0);
                actionDataPtr->macSa[4] = (GT_U8) (value >> 8);
                actionDataPtr->macSa[3] = (GT_U8) (value >>16);
                actionDataPtr->macSa[2] = (GT_U8) ((value >> 24) | (value1 << 4));
                actionDataPtr->macSa[1] = (GT_U8) (value1 >> 4);
                actionDataPtr->macSa[0] = (GT_U8) (value1 >> 12);
            }

            actionDataPtr->arpIndex = /*same field as for the 'egress interface' for ARP info ! */
                SMEM_LION3_IPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_ARP_POINTER);
            break;

        default:
            break;
    }


    actionDataPtr->bindToCncCounter =
        SMEM_LION3_IPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_BIND_TO_CNC_COUNTER);
    actionDataPtr->cncCounterIndex =
        SMEM_LION3_IPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_CNC_COUNTER_INDEX);
    actionDataPtr->srcIdSetEn =
        SMEM_LION3_IPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_SOURCE_ID_SET_ENABLE);
    actionDataPtr->srcId =
        SMEM_LION3_IPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_SOURCE_ID);
    actionDataPtr->actionStop =
        SMEM_LION3_IPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_ACTION_STOP);
    actionDataPtr->bypassBridge =
        SMEM_LION3_IPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_BRIDGE_BYPASS);
    actionDataPtr->bypassIngressPipe =
        SMEM_LION3_IPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_BYPASS_INGRESS_PIPE);
    actionDataPtr->vidPrecedence =
        SMEM_LION3_IPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_VLAN_PRECEDENCE);
    actionDataPtr->enNestedVlan =
        SMEM_LION3_IPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_EN_NESTED_VLAN);
    actionDataPtr->vlanCmd =
        SMEM_LION3_IPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_VLAN_COMMAND);
    actionDataPtr->eVid =
        SMEM_LION3_IPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_VID0);
    actionDataPtr->qosProfileMarkingEn =
        SMEM_LION3_IPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_QOS_PROFILE_MARKING_ENABLE);
    actionDataPtr->qosPrecedence =
        SMEM_LION3_IPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_QOS_PRECEDENCE);
    actionDataPtr->qosProfile =
        SMEM_LION3_IPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_QOS_PROFILE);
    actionDataPtr->modifyDscp =
        SMEM_LION3_IPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_MODIFY_DSCP);
    actionDataPtr->modifyUp =
        SMEM_LION3_IPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_MODIFY_UP);
    actionDataPtr->tcpRstFinMirrorEn =
        SMEM_LION3_IPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_ENABLE_MIRROR_TCP_RST_OR_FIN);
    actionDataPtr->modifyMacDaEn =
        SMEM_LION3_IPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_MODIFY_MAC_DA);

    actionDataPtr->tag1Cmd =
        SMEM_LION3_IPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_VLAN1_CMD);
    actionDataPtr->up1Cmd =
        SMEM_LION3_IPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_UP1_CMD);
    actionDataPtr->vid1 =
        SMEM_LION3_IPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_VID1);
    actionDataPtr->up1 =
        SMEM_LION3_IPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_UP1);
    actionDataPtr->flowId =
        SMEM_LION3_IPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_FLOW_ID);
    actionDataPtr->setMacToMe =
        SMEM_LION3_IPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_SET_MAC2ME);
    actionDataPtr->assignSrcEPortEnable =
        SMEM_LION3_IPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_ASSIGN_SRC_EPORT_EN);
    actionDataPtr->sourceEPort =
        SMEM_LION3_IPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_SRC_EPORT);
    actionDataPtr->userAcEnable =
        SMEM_LION3_IPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_USER_AC_ENABLE);
    actionDataPtr->oamInfo.ptpTimeStampEnable =
        SMEM_LION3_IPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_TIMESTAMP_EN);
    actionDataPtr->oamInfo.ptpTimeStampOffsetIndex =
        SMEM_LION3_IPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_OFFSET_INDEX);
    actionDataPtr->oamInfo.oamProcessEnable =
        SMEM_LION3_IPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_OAM_PROCESSING_EN);
    actionDataPtr->oamInfo.oamProfile =
        SMEM_LION3_IPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_OAM_PROFILE);
}

/*******************************************************************************
* snetLion3IPclActionApplyQos
*
* DESCRIPTION:
*        Lion3 : Apply the action entry from the policy action table : QOS part.
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - pointer to frame descriptor.
*       actionInfoPtr   - pointer to the PCL action entry.
*
*
* OUTPUTS:
*
* RETURN:
*
* COMMENTS:
*
*******************************************************************************/
static GT_VOID snetLion3IPclActionApplyQos
(
    IN SKERNEL_DEVICE_OBJECT                * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC   * descrPtr  ,
    IN SNET_LION3_PCL_ACTION_STC            * actionDataPtr
)
{
    DECLARE_FUNC_NAME(snetLion3IPclActionApplyQos);

    /* Remarking the QOS profile . relevant only is Qos precedence of the
       previous Qos Assignment mechanism is soft */
    if (descrPtr->qosProfilePrecedence != SKERNEL_QOS_PROF_PRECED_SOFT)
    {
        __LOG(("Due to QOS profile HARD precedence of previous engine "
            ", ignore action's : modifyUp,modifyDscp,qosProfile,qosProfilePrecedence \n"));

        return;
    }

    /* Enables the modification of the packet 802.1q User Priority field */
    descrPtr->modifyUp = (actionDataPtr->modifyUp == 1) ? 1 :
                         (actionDataPtr->modifyUp == 2) ? 0 : descrPtr->modifyUp;

    /* Enables the modification of the packet 802.1q dscp field */
    descrPtr->modifyDscp = (actionDataPtr->modifyDscp == 1) ? 1 :
                           (actionDataPtr->modifyDscp == 2) ? 0 : descrPtr->modifyDscp;

    /* Enables the modification of the qos profile index */
    __LOG_PARAM(actionDataPtr->qosProfileMarkingEn);
    if(actionDataPtr->qosProfileMarkingEn == GT_TRUE)
    {
        descrPtr->qos.qosProfile = actionDataPtr->qosProfile;
    }
    else
    {
        __LOG(("do not modify qosProfile when actionDataPtr->qosProfileMarkingEn = GT_FALSE \n"));
    }
    descrPtr->qosProfilePrecedence =  actionDataPtr->qosPrecedence ;

    __LOG(("QOS params updated according to action \n"));
}

/*******************************************************************************
* snetLion3IPclActionApplyTagging
*
* DESCRIPTION:
*        Lion3 : Apply the action entry from the policy action table : Tagging part.
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - pointer to frame descriptor.
*       actionInfoPtr   - pointer to the PCL action entry.
*
*
* OUTPUTS:
*
* RETURN:
*
* COMMENTS:
*
*******************************************************************************/
static GT_VOID snetLion3IPclActionApplyTagging
(
    IN SKERNEL_DEVICE_OBJECT                * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC   * descrPtr  ,
    IN SNET_LION3_PCL_ACTION_STC            * actionDataPtr
)
{
    DECLARE_FUNC_NAME(snetLion3IPclActionApplyTagging);
    GT_BIT   isTag0Tagged;/* indication to treat packet as 'tag 0 exists' */
    GT_BIT   isTag1Tagged;/* indication to treat packet as 'tag 1 exists' */
    GT_BOOL updateVid; /* update eVid flag */
    GT_U32  vid1 = 0,up1 = 0,cfidei1 = 0,origVid = 0;

    /* must be done before calling the snetCht3IPclActionApply(...) because :
       this configuration is applied BEFORE applying the IPCL AE<VID0 Command>. */
    if(actionDataPtr->userAcEnable)
    {
        if(descrPtr->tagSrcTagged[SNET_CHT_TAG_0_INDEX_CNS] == 0)
        {
            /* packet arrived Untagged. If packet arrived DSA, this is <SrcTag> from DSA*/
            __LOG(("packet arrived Untagged. If packet arrived DSA, this is <SrcTag> from DSA"));
            vid1 = 0;
            up1 = 0;
            cfidei1 = 0;
            origVid = descrPtr->eVid;/*DescOut<OrigVID> = DescIn<VID0>  This is the PVID on the original ingress port
                        Note it would be better if we could assign the default from the IPCL AE, but we dont have the
                        <VID1> field like we have in TTI AE. So we have to rely on the AC port PVID.*/
        }
        else
        {
            vid1 = descrPtr->eVid & 0xFFF;
            up1 = descrPtr->up;
            cfidei1 = descrPtr->cfidei;
            origVid = descrPtr->eVid;
        }

    }

    /* Nested Vid  */
    if(descrPtr->nestedVlanAccessPort == 0)
    {
        /*When <EnNestedVLAN> is set, this rule matching flow is defined as a Nested VLAN Access Flow.*/
        descrPtr->nestedVlanAccessPort = actionDataPtr->enNestedVlan;
        if(actionDataPtr->enNestedVlan)
        {
            /* When a packet received on an access flow is transmitted via a
               tagged port or a cascading port, a VLAN tag is added to the packet
               (on top of the existing VLAN tag, if any). The VID field is the
               VID assigned to the packet as a result of all VLAN assignment
               algorithms. */
            descrPtr->tagSrcTagged[SNET_CHT_TAG_0_INDEX_CNS] = 0;
            __LOG(("Nested vlan : set descrPtr->tagSrcTagged[SNET_CHT_TAG_0_INDEX_CNS] = 0 \n"));
        }
    }

    isTag0Tagged = (descrPtr->origSrcTagged && (descrPtr->nestedVlanAccessPort == 0)) ? 1 : 0;
    isTag1Tagged = ((TAG1_EXIST_MAC(descrPtr) == 0) || (descrPtr->nestedVlanAccessPort == 1)) ? 0 : 1;

    /*  VID re-assignment is relevant only is the VID precedence , set by the
        previous VID assignment mechanism is soft */
    if (descrPtr->preserveVid == 0)
    {
        descrPtr->preserveVid = actionDataPtr->vidPrecedence;
        /* PCE_VID is assigned for all packets */
        if (actionDataPtr->vlanCmd == 3)
        {
            __LOG(("PCE_VID is assigned for all packets"));
            updateVid = GT_TRUE;
        }
        else if ((actionDataPtr->vlanCmd == 2) &&
                 (isTag0Tagged == 1))
        {
            __LOG(("PCE_VID is assigned for tagged packets"));
            updateVid = GT_TRUE;
        }
        else if ((actionDataPtr->vlanCmd == 1) &&
                 (isTag0Tagged == 0))
        {
            __LOG(("PCE_VID is assigned for untagged packets"));
            updateVid = GT_TRUE;
        }
        else
        {
            __LOG(("no PCE_VID modifications \n "));
            updateVid = GT_FALSE;
        }

        if(updateVid == GT_TRUE)
        {
            descrPtr->eVid = actionDataPtr->eVid;
            descrPtr->vidModified = 1;
        }
    }

    if(actionDataPtr->tag1Cmd == 2 || /*All: The entrys <Tag1 VID> is assigned to all packets*/
       (actionDataPtr->tag1Cmd == 1 &&
       (isTag1Tagged == 0 || descrPtr->vid1 == 0)))
    {
        descrPtr->vid1 = actionDataPtr->vid1;
    }

    if(actionDataPtr->up1Cmd == 3 ||
       (actionDataPtr->up1Cmd == 1 && (0 == TAG1_EXIST_MAC(descrPtr))))
    {
        /*1 = Assign VLAN1 Untagged: If packet does not contain Tag1 assign
            according to action entrys <UP1>, else retain*/
        /*3 = Assign All: Assign action entrys <UP1> field to all packets.*/
        descrPtr->up1 = actionDataPtr->up1;
    }
    else if(actionDataPtr->up1Cmd == 2)
    {
        /*2 = Assign VLAN0 Untagged: If packet contains Tag0 use <UP0>, else
            use action entrys <UP1> field.*/
        descrPtr->up1 = TAG0_EXIST_MAC(descrPtr) ?
                    descrPtr->up :
                    actionDataPtr->up1;
    }

    if(actionDataPtr->userAcEnable)
    {
        descrPtr->vid1 = vid1;
        descrPtr->up1 = up1;
        descrPtr->cfidei1 = cfidei1;
        descrPtr->vid0Or1AfterTti = origVid & 0xFFF;
        /*DescOut<Override VID0 with OrigVID> is 1.*/
        descrPtr->overrideVid0WithOrigVid = 1;
    }



}
/*******************************************************************************
* snetLion3IPclActionApplyInterface
*
* DESCRIPTION:
*        Lion3 : Apply the action entry from the policy action table : interface part.
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - pointer to frame descriptor.
*       actionInfoPtr   - pointer to the PCL action entry.
*
*
* OUTPUTS:
*
* RETURN:
*
* COMMENTS:
*
*******************************************************************************/
static GT_VOID snetLion3IPclActionApplyInterface
(
    IN SKERNEL_DEVICE_OBJECT                * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC   * descrPtr  ,
    IN SNET_LION3_PCL_ACTION_STC            * actionDataPtr
)
{
    DECLARE_FUNC_NAME(snetLion3IPclActionApplyInterface);

    descrPtr->pclRedirectCmd = actionDataPtr->redirectCmd;

    if(actionDataPtr->redirectCmd != PCL_TTI_ACTION_REDIRECT_CMD_NONE_E)
    {
        /* reset the flag ,when another IPCL-AE assigns a different redirect command,
         and thus overriding previous IPCL-AE MAT decision*/
        descrPtr->useArpForMacSa = GT_FALSE;
    }

    if(actionDataPtr->redirectCmd != PCL_TTI_ACTION_REDIRECT_CMD_OUT_IF_E)
    {
        if(actionDataPtr->setEgressFilterRegistered)
        {
            /* do not set to 0 if already set to 1 */
            descrPtr->egressFilterRegistered = 1;
        }

        if(actionDataPtr->redirectCmd == PCL_TTI_ACTION_REDIRECT_CMD_LTT_ROUTER_E)
        {
            descrPtr->pceRoutLttIdx = actionDataPtr->routerLLTIndex;
        }
        else
        if(actionDataPtr->redirectCmd == PCL_TTI_ACTION_REDIRECT_CMD_VIRT_ROUTER_E)
        {
            descrPtr->vrfId = actionDataPtr->vrfId;
        }
        else
        if(actionDataPtr->redirectCmd == PCL_TTI_ACTION_REDIRECT_CMD_REPLACE_MAC_SA_E)
        {
            /* get the new mac SA */
            memcpy(descrPtr->newMacSa,
                   actionDataPtr->macSa,
                   sizeof(descrPtr->newMacSa));

            /*NOTE: final changes in the descriptor done after all actions done
                to check that this actions was not overridden by later action */
            descrPtr->useArpForMacSa = GT_TRUE;

            __LOG(("REPLACE_MAC_SA : indicate this action uses ARP pointer"));
            descrPtr->arpPtr = actionDataPtr->arpIndex; /*same field as for the 'egress interface' for ARP info ! */
        }
    }
    else /*if (actionDataPtr->redirectCmd == PCL_TTI_ACTION_REDIRECT_CMD_OUT_IF_E)*/
    {
        descrPtr->egressFilterRegistered = 1; /* implicitly set by the device for 'redirect to outlif'*/

        descrPtr->useVidx =
            (actionDataPtr->redirectEgressInfo.egressInterface.dstInterface == SNET_DST_INTERFACE_VIDX_E) ? 1 : 0;
        if(descrPtr->useVidx)
        {
            descrPtr->eVidx = actionDataPtr->redirectEgressInfo.egressInterface.interfaceInfo.vidx;
        }
        else
        {
            descrPtr->targetIsTrunk =
                (actionDataPtr->redirectEgressInfo.egressInterface.dstInterface ==  SNET_DST_INTERFACE_TRUNK_E) ? 1 : 0;

            if(descrPtr->targetIsTrunk)
            {
                descrPtr->trgTrunkId = actionDataPtr->redirectEgressInfo.egressInterface.interfaceInfo.trunkId;
            }
            else
            {
                descrPtr->trgDev = actionDataPtr->redirectEgressInfo.egressInterface.interfaceInfo.devPort.devNum;
                descrPtr->trgEPort = actionDataPtr->redirectEgressInfo.egressInterface.interfaceInfo.devPort.port;
                /* call after setting trgEPort */
                SNET_E_ARCH_CLEAR_IS_TRG_PHY_PORT_VALID_MAC(devObjPtr,descrPtr,pcl[SMAIN_DIRECTION_INGRESS_E]);
            }
        }

        if (actionDataPtr->redirectEgressInfo.tunnelStart )
        {/* indicate this action is a T.S */
            __LOG(("indicate this action is a tunnelStart pointer"));
            descrPtr->tunnelStart = GT_TRUE;
            descrPtr->tunnelPtr = actionDataPtr->redirectEgressInfo.tsOrArpInfo.tsInfo.index;
            descrPtr->tunnelStartPassengerType = actionDataPtr->redirectEgressInfo.tsOrArpInfo.tsInfo.passengerType;
        }

        descrPtr->VntL2Echo = actionDataPtr->redirectEgressInfo.VNTL2Echo;

        if(descrPtr->modifyMacDa == 0 && actionDataPtr->modifyMacDaEn)
        {
            if(actionDataPtr->redirectEgressInfo.tunnelStart)
            {
                __LOG(("ERROR : the 'TunnelStart must be 'off' when do 'modify mac DA' \n"));
                descrPtr->tunnelStart = GT_FALSE;
            }
            descrPtr->modifyMacDa = 1;
            __LOG(("modifyMacDa : indicate this action uses ARP pointer"));
            /* Pointer to the ARP entry */
            descrPtr->arpPtr = actionDataPtr->redirectEgressInfo.tsOrArpInfo.arpInfo.index;
        }

        if(descrPtr->modifyMacSa == 0)
        {
            descrPtr->modifyMacSa = actionDataPtr->redirectEgressInfo.modifyMacSaEn;
        }
    }

}

/*******************************************************************************
* snetLion3IPclActionApplySourceId
*
* DESCRIPTION:
*        Lion3 : Apply the action entry from the policy action table : sourceId part.
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - pointer to frame descriptor.
*       actionInfoPtr   - pointer to the PCL action entry.
*
*
* OUTPUTS:
*
* RETURN:
*
* COMMENTS:
*
*******************************************************************************/
static GT_VOID snetLion3IPclActionApplySourceId
(
    IN SKERNEL_DEVICE_OBJECT                * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC   * descrPtr  ,
    IN SNET_LION3_PCL_ACTION_STC            * actionDataPtr
)
{
    DECLARE_FUNC_NAME(snetLion3IPclActionApplySourceId);
    GT_U32  origSstId;/* sstId of the incoming descriptor*/

    if (actionDataPtr->srcIdSetEn == 0)
    {
        return;
    }
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

    return ;
}

/*******************************************************************************
* snetLion3IPclActionApplyPolicer
*
* DESCRIPTION:
*        Lion3 : Apply the action entry from the policy action table : policer part.
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - pointer to frame descriptor.
*       actionInfoPtr   - pointer to the PCL action entry.
*
*
* OUTPUTS:
*
* RETURN:
*
* COMMENTS:
*
*******************************************************************************/
static GT_VOID snetLion3IPclActionApplyPolicer
(
    IN SKERNEL_DEVICE_OBJECT                * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC   * descrPtr  ,
    IN SNET_LION3_PCL_ACTION_STC            * actionDataPtr
)
{


    if(actionDataPtr->redirectCmd == PCL_TTI_ACTION_REDIRECT_CMD_REPLACE_MAC_SA_E)
    {
        return;
    }


    /* when set to 1 , this rule is bound to the policer indexed    *
     *   by actionDataPtr->policerIndex                              */
    if(actionDataPtr->bindToPolicerMeter == 1)
    {
        descrPtr->policerEn = 1;
    }

    /* Bind to Policer Counter */
    if (actionDataPtr->bindToPolicerCounter == GT_TRUE)
    {
        descrPtr->policerCounterEn = 1;
    }

    if(descrPtr->policerCounterEn || descrPtr->policerEn)
    {
        descrPtr->policerPtr = actionDataPtr->policerIndex;
    }


    /* copy the redirection command */
    return ;
}

/*******************************************************************************
* snetLion3IPclActionApply
*
* DESCRIPTION:
*        Lion3 : Apply the action entry from the policy action table.
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - pointer to frame descriptor.
*       actionInfoPtr   - pointer to the PCL action entry.
*       cycleNum        - cycle number
*
*
* OUTPUTS:
*
* RETURN:
*
* COMMENTS:
*
*******************************************************************************/
static GT_VOID snetLion3IPclActionApply
(
    IN SKERNEL_DEVICE_OBJECT                * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC   * descrPtr  ,
    IN SNET_LION3_PCL_ACTION_STC            * actionDataPtr,
    IN GT_BOOL                              isFirst,
    IN SNET_IPCL_LOOKUP_ENT                 cycleNum
)
{
    DECLARE_FUNC_NAME(snetLion3IPclActionApply);

    snetLion3IPclActionApplyQos(devObjPtr,descrPtr,actionDataPtr);
    snetLion3IPclActionApplyTagging(devObjPtr,descrPtr,actionDataPtr);
    snetLion3IPclActionApplyInterface(devObjPtr,descrPtr,actionDataPtr);
    snetLion3IPclActionApplySourceId(devObjPtr,descrPtr,actionDataPtr);
    snetLion3IPclActionApplyPolicer(devObjPtr,descrPtr,actionDataPtr);

    if(actionDataPtr->setMacToMe)
    {
        descrPtr->mac2me = 1;
    }

    if(actionDataPtr->assignSrcEPortEnable)
    {
        snetLion3IngressReassignSrcEPort(devObjPtr,descrPtr,"PCL Action" ,
            actionDataPtr->sourceEPort);
    }

    /* enables the mirroring of the packet to the ingress analyzer port */
    if(actionDataPtr->mirrorToIngressAnalyzerPort)
    {
        descrPtr->rxSniff = 1;
    }

    descrPtr->flowId = actionDataPtr->flowId;

    /* Select mirror analyzer index */
    snetXcatIngressMirrorAnalyzerIndexSelect(devObjPtr, descrPtr,
        actionDataPtr->mirrorToIngressAnalyzerPort);

    /* resolve packet command and CPU code */
    snetChtIngressCommandAndCpuCodeResolution(devObjPtr,descrPtr,
                                              descrPtr->packetCmd,
                                              actionDataPtr->packetCmd,
                                              descrPtr->cpuCode,
                                              actionDataPtr->cpuCode,
                                              SNET_CHEETAH_ENGINE_UNIT_PCL_E,
                                              isFirst);

    if(actionDataPtr->redirectCmd == PCL_TTI_ACTION_REDIRECT_CMD_NONE_E ||
       actionDataPtr->redirectCmd == PCL_TTI_ACTION_REDIRECT_CMD_LTT_ROUTER_E ||
       actionDataPtr->redirectCmd == PCL_TTI_ACTION_REDIRECT_CMD_VIRT_ROUTER_E)
    {
        if(cycleNum < 2 && actionDataPtr->ipclProfile.lookup1Mode)
        {
            descrPtr->pclLookUpMode[1] = SKERNEL_PCL_LOOKUP_MODE_DESCRIPTOR_INDEX_E;
            descrPtr->ipclProfileIndex = actionDataPtr->ipclProfile.profileIndex;
        }

        if(actionDataPtr->ipclProfile.lookup2Mode)
        {
            descrPtr->pclLookUpMode[2] = SKERNEL_PCL_LOOKUP_MODE_DESCRIPTOR_INDEX_E;
            descrPtr->ipclProfileIndex = actionDataPtr->ipclProfile.profileIndex;
        }
    }

    /* TCP FIN/RST mirror to CPU action */
    if (actionDataPtr->tcpRstFinMirrorEn == GT_TRUE)
    {
        /* check bits 0 (FIN) and 2 (RST) of TCP flags */
        if (descrPtr->isIp && (descrPtr->ipProt == SNET_TCP_PROT_E) &&
            descrPtr->l4StartOffsetPtr &&
           (descrPtr->l4StartOffsetPtr[13] & SNET_XCAT_TCP_FIN_RST_MASK_CNS))
        {
            __LOG(("TCP FIN/RST mirror to CPU action \n"));

            /* send packet to CPU for TCP RST or FIN.
               CPU code is already set by snetCht2IPclActionApply */
            descrPtr->packetCmd = snetChtPktCmdResolution(descrPtr->packetCmd,
                SKERNEL_EXT_PKT_CMD_MIRROR_TO_CPU_E);
        }
    }

    if(descrPtr->bypassBridge == 0 &&
       actionDataPtr->bypassBridge)
    {
        /* bridge is bypassed for this packet */
        __LOG(("Bridge is bypassed for this packet \n"));
        descrPtr->bypassBridge = 1;
    }

    if(descrPtr->bypassIngressPipe == 0 &&
       actionDataPtr->bypassIngressPipe)
    {
        /* Ingress pipe is bypassed for this packet */
        __LOG(("Ingress pipe is bypassed for this packet \n"));
        descrPtr->bypassIngressPipe = 1;
    }
}

/*******************************************************************************
* lion3PclSrcIdMaskGet
*
* DESCRIPTION:
*        Lion3 : Get the srcIdMask for the 3 lookups.
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*
* OUTPUTS:
*       pclSrcIdMaskArr[] - array of srcIdMask for the 3 lookups
* RETURN:
*
* COMMENTS:
*
*******************************************************************************/
static GT_VOID lion3PclSrcIdMaskGet
(
    IN    SKERNEL_DEVICE_OBJECT             *devObjPtr,
    OUT   GT_U32                             pclSrcIdMaskArr[3]
)
{
    GT_U32 regAddr;
    GT_U32 regValue;
    GT_U32 ii;

    for(ii = 0 ; ii < 3 ; ii++)
    {
        regAddr = SMEM_LION3_PCL_LOOKUP_SOURCE_ID_MASK_REG(devObjPtr,ii);
        smemRegGet(devObjPtr,regAddr , &regValue);
        pclSrcIdMaskArr[ii]= SMEM_U32_GET_FIELD(regValue,0,12);
    }

    return;
}

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
)
{
    GT_U32  ii; /* loop param for parallel lookups */

    *bitmapPtr = 0x0;

    __LOG_PARAM_NO_LOCATION_META_DATA(tcamSegmentMode);

    switch(tcamSegmentMode)
    {
        case(SNET_LION3_TCAM_SEGMENT_MODE_1_TCAM_E):
            __LOG_NO_LOCATION_META_DATA__WITH_SCIB_LOCK(("segment mode: 0x0 = 1 TCAM; One_TCAM; All TCAM segments are one logical TCAM \n"));
            for (ii = 0 ; ii < SIP5_TCAM_MAX_NUM_OF_HITS_CNS ; ii++)
            {
                if(SNET_CHT_POLICY_NO_MATCH_INDEX_CNS != matchIndexArr[ii])
                {
                    *bitmapPtr = (1 << ii);
                    break;
                }
            }
            break;

        case(SNET_LION3_TCAM_SEGMENT_MODE_2_TCAM_DIV_2_2_E):
            __LOG_NO_LOCATION_META_DATA__WITH_SCIB_LOCK(("segment mode: 0x1 = 2TCAM_2and2; Two_TCAM_2and2; Segmentation into 2 lists, logically distributed 1/2 and 1/2 \n"));
            if(SNET_CHT_POLICY_NO_MATCH_INDEX_CNS != matchIndexArr[0])
            {
                *bitmapPtr = 0x1;
            }
            else if(SNET_CHT_POLICY_NO_MATCH_INDEX_CNS != matchIndexArr[1])
            {
                *bitmapPtr = 0x2;
            }

            if(SNET_CHT_POLICY_NO_MATCH_INDEX_CNS != matchIndexArr[2])
            {
                *bitmapPtr += 0x4;
            }
            else if(SNET_CHT_POLICY_NO_MATCH_INDEX_CNS != matchIndexArr[3])
            {
                *bitmapPtr += 0x8;
            }
            break;

        case(SNET_LION3_TCAM_SEGMENT_MODE_2_TCAM_DIV_1_3_E):
            __LOG_NO_LOCATION_META_DATA__WITH_SCIB_LOCK(("segment mode: 0x2 = 2TCAM_1and3; Two_TCAM_1and3; Segmentation into 2 lists, logically distributed 1/4 [first] and 3/4 [second] \n"));
            if(SNET_CHT_POLICY_NO_MATCH_INDEX_CNS != matchIndexArr[0])
            {
                *bitmapPtr = 0x1;
            }

            for (ii = 1 ; ii < SIP5_TCAM_MAX_NUM_OF_HITS_CNS ; ii++)
            {
                if(SNET_CHT_POLICY_NO_MATCH_INDEX_CNS != matchIndexArr[ii])
                {
                    *bitmapPtr += (1 << ii);
                    break;
                }
            }
            break;

        case(SNET_LION3_TCAM_SEGMENT_MODE_2_TCAM_DIV_3_1_E):
            __LOG_NO_LOCATION_META_DATA__WITH_SCIB_LOCK(("segment mode: 0x3 = 2TCAM_3and1; Two_TCAM_3and1; Segmentation into 2 lists, logically distributed 3/4 [first] and 1/4 [second] \n"));
            if(SNET_CHT_POLICY_NO_MATCH_INDEX_CNS != matchIndexArr[3])
            {
                *bitmapPtr = 0x8;
            }

            for (ii = 0 ; ii < (SIP5_TCAM_MAX_NUM_OF_HITS_CNS-1) ; ii++)
            {
                if(SNET_CHT_POLICY_NO_MATCH_INDEX_CNS != matchIndexArr[ii])
                {
                    *bitmapPtr += (1 << ii);
                    break;
                }
            }
            break;

        case(SNET_LION3_TCAM_SEGMENT_MODE_4_TCAM_DIV_E):
            __LOG_NO_LOCATION_META_DATA__WITH_SCIB_LOCK(("segment mode: 0x4 = 4 TCAM; Four_TCAM; Segmentation into 4 lists \n"));
            for (ii = 0 ; ii < SIP5_TCAM_MAX_NUM_OF_HITS_CNS ; ii++)
            {
                if(SNET_CHT_POLICY_NO_MATCH_INDEX_CNS != matchIndexArr[ii])
                {
                    *bitmapPtr += (1 << ii);
                }
            }
            break;
        default:
            break;
    }

    return;
}

#define IPCL_SET_FIELD_TO_ALL_DESCRIPTORS_MAC(_descrPtr,fieldName) \
{                                                                      \
    _descrPtr->ingressTunnelInfo.origDescrPtr->fieldName = _descrPtr->fieldName;   \
    _descrPtr->ingressTunnelInfo.origDescrPtr->ingressTunnelInfo.passengerDescrPtr->fieldName = _descrPtr->fieldName;       \
    _descrPtr->ingressTunnelInfo.origDescrPtr->ingressTunnelInfo.fixedFieldsTypeDescrPtr->fieldName = _descrPtr->fieldName; \
    _descrPtr->ingressTunnelInfo.origDescrPtr->ingressTunnelInfo.udbDescrPtr->fieldName = _descrPtr->fieldName;             \
    /* descrPtr is also one of the modified above ! */ \
    __LOG_PARAM((descrPtr->fieldName)); \
}

/*******************************************************************************
* transitPacketPrepare
*
* DESCRIPTION:
*        do preparations for handling transit packet (did not do TunnelTermination
*       but so info taken from passenger while others from the Tunnel)
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - pointer to frame data buffer Id
*
* OUTPUTS:
*       descrPtr     - pointer to updated frame data buffer Id
*
* RETURN:
*       main descriptor to work with
* COMMENTS:
*
*
*******************************************************************************/
static SKERNEL_FRAME_CHEETAH_DESCR_STC  * transitPacketPrepare
(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr
)
{
    DECLARE_FUNC_NAME(transitPacketPrepare);

    SKERNEL_FRAME_CHEETAH_DESCR_STC  * origDescrPtr;/* pointer to the original descriptor */
    SKERNEL_FRAME_CHEETAH_DESCR_STC  * transitDescrPtr;/* pointer to the transit descriptor */
    SKERNEL_FRAME_CHEETAH_DESCR_STC  * passengerDescrPtr;/* pointer to the transit descriptor */

    SKERNEL_FRAME_CHEETAH_DESCR_STC  * fixedFieldsTypeDescrPtr;/* pointer to the descriptor for fixed fields */
    SKERNEL_FRAME_CHEETAH_DESCR_STC  * udbDescrPtr;/* pointer to the descriptor for UDB anchors  */

    transitDescrPtr = descrPtr->ingressTunnelInfo.innerFrameDescrPtr;

    origDescrPtr = descrPtr;

    if(descrPtr->ingressTunnelInfo.transitType != SNET_INGRESS_TUNNEL_TRANSIT_TYPE__NONE__E)
    {
        __LOG(("start handle 'Transit packet' (non TT) \n"));

        if(transitDescrPtr == NULL)
        {
            skernelFatalError("snetXCatIPcl: transitDescrPtr = NULL \n");
        }

        if(descrPtr->ingressTunnelInfo.transitType ==
            SNET_INGRESS_TUNNEL_TRANSIT_TYPE_LEGACY_IP_OVER_MPLS_TUNNEL_E)
        {
            __LOG(("L3L4 info of the passenger -- even when not tunnel terminated \n"));

            /* L3L4 info of the passenger -- even when not tunnel terminated */
            descrPtr = snetChtEqDuplicateDescr(devObjPtr, origDescrPtr);

            /* since the time that descrPtr->ingressTunnelInfo.innerFrameDescrPtr was set with values the
               'original descriptor' added to it's info values that must be taken from
               origDescrPtr .

               so only L3L4 info need to be taken from ingressTunnelInfo.innerFrameDescrPtr
               */

            snetChtL3L4ProtCopyDesc(descrPtr,transitDescrPtr);

            __LOG(("PCL fixed key fields (non-UDB) are based on the passenger IPv4/6 header \n"));
            fixedFieldsTypeDescrPtr = descrPtr;

            __LOG(("The UDB anchors are relative to the tunnel header only, and not to the passenger header \n"));
            udbDescrPtr = origDescrPtr;

        }
        else/* sip5 : new MPLS transit or new NON-MPLS transit */
        {
            __LOG(("new MPLS transit or new NON-MPLS transit \n"));

            __LOG(("IPCL fixed key fields (non-UDB) are based on the tunnel header \n"));
            fixedFieldsTypeDescrPtr = origDescrPtr;

            __LOG(("IPCL UDB key fields can have an <Anchor Type> relative to passenger header or tunnel header \n"));
            udbDescrPtr = origDescrPtr;

            descrPtr = transitDescrPtr;
        }

        passengerDescrPtr = descrPtr;

        /* copy fields that may changed/added since the inner descriptor was created */
        passengerDescrPtr->eArchExtInfo.ttiPostTtiLookupIngressEPortTablePtr =
             origDescrPtr->eArchExtInfo.ttiPostTtiLookupIngressEPortTablePtr;

        passengerDescrPtr->ipclUdbConfigurationTableUdeIndex =
             origDescrPtr->ipclUdbConfigurationTableUdeIndex;

    }
    else  /* packet is TT or not TT and not 'transit' */
    {
        __LOG(("packet is TT or not TT and not 'transit' \n"));

        passengerDescrPtr = origDescrPtr;
        fixedFieldsTypeDescrPtr = origDescrPtr;
        udbDescrPtr = origDescrPtr;
    }

    /* save values for later use */
    origDescrPtr->ingressTunnelInfo.origDescrPtr            = origDescrPtr;
    origDescrPtr->ingressTunnelInfo.passengerDescrPtr       = passengerDescrPtr;
    origDescrPtr->ingressTunnelInfo.fixedFieldsTypeDescrPtr = fixedFieldsTypeDescrPtr;
    origDescrPtr->ingressTunnelInfo.udbDescrPtr             = udbDescrPtr;

    if(origDescrPtr != descrPtr)
    {
        descrPtr->ingressTunnelInfo.origDescrPtr            = origDescrPtr;
        descrPtr->ingressTunnelInfo.passengerDescrPtr       = passengerDescrPtr;
        descrPtr->ingressTunnelInfo.fixedFieldsTypeDescrPtr = fixedFieldsTypeDescrPtr;
        descrPtr->ingressTunnelInfo.udbDescrPtr             = udbDescrPtr;
    }

    if(origDescrPtr != passengerDescrPtr)
    {
        passengerDescrPtr->ingressTunnelInfo.origDescrPtr            = origDescrPtr;
        passengerDescrPtr->ingressTunnelInfo.passengerDescrPtr       = passengerDescrPtr;
        passengerDescrPtr->ingressTunnelInfo.fixedFieldsTypeDescrPtr = fixedFieldsTypeDescrPtr;
        passengerDescrPtr->ingressTunnelInfo.udbDescrPtr             = udbDescrPtr;
    }

    return descrPtr;
}

/*******************************************************************************
* pclCalcPacketHashForKey
*
* DESCRIPTION:
*        calc packet hash for IPCL key
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
static GT_VOID pclCalcPacketHashForKey
(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr
)
{
    DECLARE_FUNC_NAME(pclCalcPacketHashForKey);

    GT_U32 regValue;
    GT_U32 numOfBits,firstBit,lastBit;

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        smemRegGet(devObjPtr, SMEM_XCAT_POLICY_ENGINE_CONF_REG(devObjPtr),&regValue);

        /*<Hash Select First Bit>*/
        firstBit = SMEM_U32_GET_FIELD(regValue, 18, 5);
        /*<Hash Select Last Bit>*/
        lastBit = SMEM_U32_GET_FIELD(regValue, 23, 5);

        if(firstBit && ((lastBit + 1)  < firstBit))
        {
            __LOG(("ERROR : lastBit[%d] less then firstBit[%d] \n",
                lastBit ,
                firstBit));

            lastBit = firstBit - 1;
        }

        numOfBits = (lastBit + 1) - firstBit;

        descrPtr->pktHashForIpcl = SMEM_U32_GET_FIELD(descrPtr->pktHash,firstBit,numOfBits);

        __LOG(("PCL packet hash for key [0x%4.4x] after using bits[%d to %d] \n",
            descrPtr->pktHashForIpcl,firstBit,lastBit));

    }
    else
    {
        descrPtr->pktHashForIpcl = (descrPtr->pktHash & 0x3F); /* use only 6 bits !!!*/
    }

    __LOG_PARAM(descrPtr->pktHashForIpcl);
}

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
*       A1 and above
*
*******************************************************************************/
GT_VOID snetXCatIPcl
(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr
)
{
    DECLARE_FUNC_NAME(snetXCatIPcl);

    SNET_IPCL_LOOKUP_ENT            cycleNum;    /* cycle number */
    SNET_CHT3_PCL_LOOKUP_CONFIG_STC lookUpCfg;  /* lookup configuration
                                                   Ingress PCL configuration
                                                   table */

    CHT_PCL_EXTRA_PACKET_INFO_STC   pclExtraData;
    CHT_PCL_EXTRA_PACKET_INFO_STC   origPclExtraData;
    SNET_CHT_POLICY_KEY_STC         pclKey;       /* policy key structure */
    GT_U32                          keyType;      /* type of tcam search key*/
    GT_U32                          matchIndexArr[SIP5_TCAM_MAX_NUM_OF_HITS_CNS];
                                     /* array of indexes to the match actions */
    GT_U32                          parallelLookupInst; /* loop param for parallel lookups */
    GT_U32                          validHitBmp = 0;
    GT_U32                          maxNumOfParallelLookups;
    SNET_XCAT_PCL_ACTION_STC      * actionDataXcatPtr;
    SNET_LION_PCL_ACTION_STC        actionDataLion;  /* Lion action data */
    SNET_LION3_PCL_ACTION_STC       lion3Action;
    GT_U32                          lookUpClient;
    GT_BOOL                         rc;
    GT_BOOL                         isFirst = GT_TRUE;/* is first action apply */
    SKERNEL_FRAME_CHEETAH_DESCR_STC  * origDescrPtr;/* pointer to the original descriptor */
    SKERNEL_FRAME_CHEETAH_DESCR_STC  * passengerDescrPtr;/* pointer to the transit descriptor */
    SKERNEL_FRAME_CHEETAH_DESCR_STC  * fixedFieldsTypeDescrPtr;/* pointer to the descriptor for fixed fields */

    GT_U32  pclSrcIdMaskArr[3] = {0,0,0}; /*sip5 pclSrcIdMask : one per lookup*/
    PCL_TTI_ACTION_REDIRECT_CMD_ENT pceRedirectCmd;
    GT_BOOL actionStop = GT_FALSE;
    GT_BOOL matchCounterEn;
    GT_U32  matchCounterIndex;

    memset(&lookUpCfg, 0, sizeof(SNET_CHT3_PCL_LOOKUP_CONFIG_STC));
    pclKey.devObjPtr = devObjPtr;

    origDescrPtr =  descrPtr;

    /* prepare for transit issues */
    descrPtr = transitPacketPrepare(devObjPtr,descrPtr);

    passengerDescrPtr       = descrPtr->ingressTunnelInfo.passengerDescrPtr;
    fixedFieldsTypeDescrPtr = descrPtr->ingressTunnelInfo.fixedFieldsTypeDescrPtr;

    /* assign packet type for PCL usage
       must be done only after descrPtr may hold different values then origDescrPtr !!! */
    pcktTypeAssign(devObjPtr, passengerDescrPtr,PACKET_TYPE_USED_FOR_CLIENT_IPCL_E);
    IPCL_SET_FIELD_TO_ALL_DESCRIPTORS_MAC(passengerDescrPtr,pcl_pcktType_sip5);

    /* Get extra data from packet that will be used in PCL engine */
    __LOG(("Get extra data from packet that will be used in PCL engine"));
    snetChtPclSrvParseExtraData(devObjPtr, fixedFieldsTypeDescrPtr, &pclExtraData);
    fixedFieldsTypeDescrPtr->pclExtraDataPtr = &pclExtraData;

    if (origDescrPtr != fixedFieldsTypeDescrPtr)
    {
        /* Get extra data from original packet that will be used in PCL engine */
        snetChtPclSrvParseExtraData(devObjPtr, origDescrPtr, &origPclExtraData);
        origDescrPtr->pclExtraDataPtr = &origPclExtraData;
    }

    if(devObjPtr->supportCrcTrunkHashMode)
    {
        /*the HASHing is always done according to the 'passenger' */
        snetLionCrcBasedTrunkHash(devObjPtr, passengerDescrPtr);
        /* check that original descriptor is not the used descriptor ..
           if different save the needed changes */
        if(origDescrPtr != passengerDescrPtr)
        {
            /* save the calculated pktHash */
            __LOG(("save the calculated pktHash: [0x%x] \n", passengerDescrPtr->pktHash));
            IPCL_SET_FIELD_TO_ALL_DESCRIPTORS_MAC(passengerDescrPtr,pktHash);
        }
    }

    for (parallelLookupInst = 0 ;
         parallelLookupInst < SIP5_TCAM_MAX_NUM_OF_HITS_CNS ;
         parallelLookupInst++)
    {
        matchIndexArr[parallelLookupInst] = SNET_CHT_POLICY_NO_MATCH_INDEX_CNS;
    }


    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        /*set the array of pclSrcIdMaskArr*/
        lion3PclSrcIdMaskGet(devObjPtr,pclSrcIdMaskArr);
    }

    /*the HASHing is always done according to the 'passenger' */
    pclCalcPacketHashForKey(devObjPtr,passengerDescrPtr);


    if(descrPtr->oamInfo.oamProcessEnable)
    {
        SNET_LION3_PCL_ACTION_OAM_STC   oamInfo;

        oamInfo.oamProcessEnable             = 1;
        oamInfo.oamProfile                   = descrPtr->oamInfo.oamProfile;
        oamInfo.ptpTimeStampEnable           = descrPtr->oamInfo.timeStampEnable;
        oamInfo.ptpTimeStampOffsetIndex      = descrPtr->oamInfo.offsetIndex;
        oamInfo.channelTypeToOpcodeMappingEn = descrPtr->channelTypeToOpcodeMappingEn;

        __LOG(("do first OAM according to the TTI info (as if this is 'first' IPCL action info) \n"));
        __LOG(("Update relevant OAM descriptor fields. for TTI info \n"));
        snetLion2PclOamDescriptorSet(devObjPtr, origDescrPtr,
                                     SMAIN_DIRECTION_INGRESS_E,
                                     &oamInfo);
    }

    if(origDescrPtr->ActionStop)
    {
        /* the TTI AE <STOP> bit disables subsequent IPCL lookups */
        __LOG(("the TTI AE <STOP> bit disables subsequent IPCL lookups"));
        return;
    }

    /* Perform the PCL engine for lookups: PCL_0_0, PCL_0_1 and PCL_1 */
    for (cycleNum = 0; cycleNum < 3; cycleNum++)
    {
        __LOG(("start PCL lookup[%d]\n", cycleNum));

        /* Check if the packet command is hard drop */
        if (origDescrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_HARD_DROP_E)
        {
            __LOG(("the packet command is hard drop, so stop PCL lookups. cycleNum[%d]", cycleNum));
            break;
        }

        rc = snetXCatIPclTriggeringCheck(devObjPtr, origDescrPtr, cycleNum, &lookUpCfg);
        /* check lookup trigger condition */
        if(rc == GT_FALSE)
        {
            __LOG(("current lookup not triggered. cycleNum[%d]", cycleNum));
            continue;
        }

        /* PCL_ID from PCL configuration table */
        descrPtr->pclId = lookUpCfg.pclID;

        /* Get from the configuration the key type and the key-size */
        __LOG(("Get from the configuration the key type and the key-size. cycleNum[%d]", cycleNum));
        snetXCatPclGetKeyType(devObjPtr,fixedFieldsTypeDescrPtr, &lookUpCfg, &keyType);

        /* Create policy search key */
        __LOG(("Create policy search key. cycleNum[%d]", cycleNum));
        snetXCatIPclCreateKey(devObjPtr, origDescrPtr, &lookUpCfg, &pclExtraData,
                              keyType, cycleNum, &pclKey);

        /* Search key in policy Tcam */
        __LOG(("Search key in policy Tcam. cycleNum [%d]", cycleNum));

        if(devObjPtr->supportEArch)
        {
            snetSip5PclTcamLookup(devObjPtr, SIP5_TCAM_CLIENT_IPCL0_E + cycleNum,
                               pclKey.key.data, pclKey.pclKeyFormat, &matchIndexArr[0]);

            for (parallelLookupInst = 0 ;
                  parallelLookupInst < SIP5_TCAM_MAX_NUM_OF_HITS_CNS ;
                  parallelLookupInst++)
            {
                if(SNET_CHT_POLICY_NO_MATCH_INDEX_CNS != matchIndexArr[parallelLookupInst])
                {
                    break;
                }
            }

            if(SIP5_TCAM_MAX_NUM_OF_HITS_CNS == parallelLookupInst)
            {
                __LOG(("current Quad lookup with 'no match'. cycleNum [%d]", cycleNum));
                continue;
            }

            /* Match found */
            __LOG((
                   "************************************************************* \n"
                   "************************************************************* \n"
                 ));
            for (parallelLookupInst = 0 ;
                  parallelLookupInst < SIP5_TCAM_MAX_NUM_OF_HITS_CNS ;
                  parallelLookupInst++)
            {
                if(SNET_CHT_POLICY_NO_MATCH_INDEX_CNS != matchIndexArr[parallelLookupInst])
                {
                    __LOG((
                           "Match found at index[0x%x], quad lookup [%d]. cycleNum [%d]\n"
                           ,matchIndexArr[parallelLookupInst],parallelLookupInst,cycleNum));
                }
            }
            __LOG((
                   "************************************************************* \n"
                   "************************************************************* \n"
                 ));

            /* create action index valid bitmap for quad results resolution */
            lion3PclValidActionBmpGet(&matchIndexArr[0],
                                  lookUpCfg.tcamSegmentMode,
                                  &validHitBmp);
            __LOG((
                   "PCL : action index valid bitmap for quad results resolution [0x%x],"
                   "TCAM segment Mode [%d], cycleNum [%d]\n"
                   ,validHitBmp,lookUpCfg.tcamSegmentMode,cycleNum));

            maxNumOfParallelLookups = SIP5_TCAM_MAX_NUM_OF_HITS_CNS;
        }
        else
        {
            snetChtPclSrvTcamLookUp(devObjPtr, devObjPtr->pclTcamInfoPtr, &pclKey, &matchIndexArr[0]);

            if(SNET_CHT_POLICY_NO_MATCH_INDEX_CNS == matchIndexArr[0])
            {
                __LOG(("current lookup with 'no match'. cycleNum [%d]", cycleNum));
                continue;
            }

            /* Match found */
            __LOG((
                   "************************************************************* \n"
                   "************************************************************* \n"
                   "Match found at index[0x%x]. cycleNum [%d] \n"
                   "************************************************************* \n"
                   "************************************************************* \n"
                   ,matchIndexArr[0],cycleNum));

            maxNumOfParallelLookups = 1;
        }

        if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            /* use the pclSrcIdMask that relevant to current lookup */
            origDescrPtr->pclSrcIdMask = pclSrcIdMaskArr[cycleNum];
        }

        for (parallelLookupInst = 0 ;
             parallelLookupInst < maxNumOfParallelLookups ;
             parallelLookupInst++)
        {
            if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
            {
                if( 0 == (validHitBmp & (1 << parallelLookupInst)) )
                {
                    continue;
                }

                memset(&lion3Action, 0, sizeof(SNET_LION3_PCL_ACTION_STC));

                /* Read the action from the pcl action table */
                __LOG(("Read the action from the pcl action table. cycleNum [%d],"
                        "quad lookup num [%d]"
                        ,cycleNum, parallelLookupInst));
                snetLion3IPclActionGet(devObjPtr, origDescrPtr,
                                       matchIndexArr[parallelLookupInst], &lion3Action);

                /* Apply the action */
                __LOG(("Apply the action. cycleNum [%d], quad lookup num [%d]",
                       cycleNum, parallelLookupInst));
                snetLion3IPclActionApply(devObjPtr, origDescrPtr, &lion3Action, isFirst , cycleNum);

                /* Update relevant OAM descriptor fields */
                __LOG(("Update relevant OAM descriptor fields. cycleNum [%d],"
                       "quad lookup num [%d]"
                       ,cycleNum, parallelLookupInst));
                snetLion2PclOamDescriptorSet(devObjPtr, origDescrPtr,
                                             SMAIN_DIRECTION_INGRESS_E,
                                             &lion3Action.oamInfo);

                actionStop          = lion3Action.actionStop;
                pceRedirectCmd      = lion3Action.redirectCmd;
                matchCounterEn      = lion3Action.bindToCncCounter;
                matchCounterIndex   = lion3Action.cncCounterIndex;
            }
            else
            {
                memset(&actionDataLion, 0, sizeof(SNET_LION_PCL_ACTION_STC));
                actionDataXcatPtr = &actionDataLion.baseAction;

                /* Read the action from the pcl action table */
                __LOG(("Read the action from the pcl action table. cycleNum [%d]", cycleNum));
                snetXCatIPclActionGet(devObjPtr, origDescrPtr, matchIndexArr[0], actionDataXcatPtr);

                /* Apply the action */
                __LOG(("Apply the action. cycleNum [%d]", cycleNum));
                snetXCatIPclActionApply(devObjPtr, origDescrPtr, actionDataXcatPtr, isFirst);

                actionStop          = CNV_CHT3_ACTION_MAC(actionDataXcatPtr)->actionStop;
                pceRedirectCmd      = CNV_CHT2_ACTION_MAC(actionDataXcatPtr)->pceRedirectCmd;
                matchCounterEn      = CNV_CHT2_ACTION_MAC(actionDataXcatPtr)->matchCounterEn;
                matchCounterIndex   = CNV_CHT2_ACTION_MAC(actionDataXcatPtr)->matchCounterIndex;
            }
            isFirst = GT_FALSE;

            /* Ingress CNC Policy Clients */
            if(matchCounterEn == GT_TRUE)
            {
                __LOG(("Ingress CNC Policy Clients. cycleNum [%d]", cycleNum));
                if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
                {
                    __LOG((", quad lookup num [%d]", parallelLookupInst));
                }

                lookUpClient =  SNET_LION3_CNC_IPCL_LOOKUP_GET(cycleNum, parallelLookupInst);
                snetCht3CncCount(devObjPtr, origDescrPtr, lookUpClient,
                                 matchCounterIndex);
            }

            if (pceRedirectCmd == PCL_TTI_ACTION_REDIRECT_CMD_PBR_OR_VPLS_E)
            {
                /* There is no restriction that prevents from using the first
                PCL lookup for routing. However, if this is done, no further PCL
                lookups will be performed on this packet */

                if(cycleNum != 2)
                {
                    /* this behavior seems like simulating the actual HW , anyway
                    routing will not happen on this case */

                    origDescrPtr->pclRedirectCmd = PCL_TTI_ACTION_REDIRECT_CMD_NONE_E;
                    /* rest the info because the device support it only on lookup 2 */
                }
                break;
            }

            /* If set, packet will not have any searches in the IPCL mechanism */
            if (actionStop)
            {
                /* break from inner loop */
                __LOG(("'Action stopped', so stop PCL lookups . cycleNum [%d]"
                       ,cycleNum));
                if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
                {
                    __LOG((", quad lookup num [%d]", parallelLookupInst));
                }
                break;
            }
        }

        if (actionStop)
        {
            /* break from uoter loop*/
            break;
        }
    }

    if(origDescrPtr->useArpForMacSa == GT_TRUE)
    {
        /* next is done after all actions are done , because:
            1. still use the 'original mac SA' in the PCL unit.
            2. modify only if no other lookup override the flag
        */
        __LOG(("the PCL unit modified the SRC MAC addr from [%2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x] to [%2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x] \n"
            ,origDescrPtr->macSaPtr[0]
            ,origDescrPtr->macSaPtr[1]
            ,origDescrPtr->macSaPtr[2]
            ,origDescrPtr->macSaPtr[3]
            ,origDescrPtr->macSaPtr[4]
            ,origDescrPtr->macSaPtr[5]

            ,origDescrPtr->newMacSa[0]
            ,origDescrPtr->newMacSa[1]
            ,origDescrPtr->newMacSa[2]
            ,origDescrPtr->newMacSa[3]
            ,origDescrPtr->newMacSa[4]
            ,origDescrPtr->newMacSa[5]
            ));

        /* save the 'orig' mac SA (before the change)*/
        origDescrPtr->origMacSaPtr = origDescrPtr->macSaPtr;
        /* point the descriptor into the new mac SA */
        origDescrPtr->macSaPtr = &origDescrPtr->newMacSa[0];
        /*reset the tunnel start*/
        origDescrPtr->tunnelStart = GT_FALSE;
    }
}

/*******************************************************************************
*   snetLion3EPclKeyFieldBuildByValue
*
* DESCRIPTION:
*       Lion3 : Function insert data of field to the search key in specific place in EPCL key
*
* INPUTS:
*       pclKeyPtr - pointer to current pcl key
*       fieldVal - data of field to insert to key
*       fieldId - field id
*
* OUTPUTS:
*
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID snetLion3EPclKeyFieldBuildByValue
(
    INOUT SNET_CHT_POLICY_KEY_STC        * pclKeyPtr,
    IN GT_U32                          fieldVal,
    IN LION3_EPCL_KEY_FIELDS_ID_ENT     fieldId
)
{
    CHT_PCL_KEY_FIELDS_INFO_STC *fieldInfoPtr;

    if(fieldId > LION3_EPCL_KEY_FIELDS_ID___LAST__E)
    {
        /* this is not valid field for such device */
        skernelFatalError("snetLion3EPclKeyFieldBuildByValue: not valid field[%d] \n",fieldId);
    }

    fieldInfoPtr = &lion3EPclKeyFieldsData[fieldId];

    snetChtPclSrvKeyFieldBuildByValue(pclKeyPtr, fieldVal,  fieldInfoPtr);
}

/*******************************************************************************
*   snetLion3EPclKeyFieldBuildByPointer
*
* DESCRIPTION:
*       Lion3 : Function insert data of field to the search key in specific place in key - EPCL
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
static GT_VOID snetLion3EPclKeyFieldBuildByPointer
(
    INOUT SNET_CHT_POLICY_KEY_STC *pclKeyPtr,
    IN GT_U8                      *fieldValPtr,
    IN LION3_EPCL_KEY_FIELDS_ID_ENT  fieldId
)
{
    CHT_PCL_KEY_FIELDS_INFO_STC *fieldInfoPtr;

    if(fieldId > LION3_EPCL_KEY_FIELDS_ID___LAST__E)
    {
        /* this is not valid field for such device */
        skernelFatalError("snetLion3EPclKeyFieldBuildByPointer: not valid field[%d] \n",fieldId);
    }

    fieldInfoPtr = &lion3EPclKeyFieldsData[fieldId];

    snetChtPclSrvKeyFieldBuildByPointer(pclKeyPtr, fieldValPtr, fieldInfoPtr);
}


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
)
{
    DECLARE_FUNC_NAME(lion3EPclUdbKeyValueGet);

    GT_U32  regAddr;            /* register address */
    GT_U32  udbData;            /* 11-bit UDB data */
    GT_U32  fieldValue;           /* field value */
    GT_U32  * regDataPtr;       /* register data pointer */
    GT_U32  userDefinedAnchor;  /* user defined byte Anchor */
    GT_U32  userDefinedOffset;  /* user defined byte offset from Anchor */
    GT_STATUS  rc;
    GT_U32  lineIndex;/* index of line in the "IPCL User Defined Bytes Configuration" */
    GT_U32  startBit;/* start bit in the "IPCL User Defined Bytes Configuration" */
    GT_U32  numBitsForUdbIndex = 12;

    lineIndex = descrPtr->epcl_pcktType_sip5;

    *byteValuePtr = 0;

    regAddr = SMEM_LION3_HA_EPCL_UDB_CONFIGURATION_TBL_MEM(devObjPtr, lineIndex);
    regDataPtr = smemMemGet(devObjPtr, regAddr);

    startBit = udbIdx * numBitsForUdbIndex;

    /* get all 12-bit data related to the udbIdx UDB */
    udbData = snetFieldValueGet(regDataPtr, startBit, numBitsForUdbIndex);

    /* bit 0 - valid */
    fieldValue = udbData & 1; /*1 bit*/
    if (fieldValue == 0)
    {
        __LOG(("descrPtr->egressUdbPacketType[%d] , UDB[%d] - not valid (in UDB configuration table)\n",
            descrPtr->epcl_pcktType_sip5,
            udbIdx));
        return GT_OK;
    }

    /* Use UDB */
    userDefinedAnchor = (udbData >> 1) & 7;/*3bits*/
    userDefinedOffset = (udbData >> 4) ;/*8 bits*/

    rc = snetXCatPclUserDefinedByteGet(devObjPtr, descrPtr, userDefinedAnchor,
                                      userDefinedOffset, SNET_UDB_CLIENT_EPCL_E, byteValuePtr);

    __LOG(("descrPtr->egressUdbPacketType[%d] ,  UDB[%d] - value[0x%2.2x] \n",
        descrPtr->epcl_pcktType_sip5,
        udbIdx,
        *byteValuePtr));

    return rc;
}

/* build UDBs for the given keyIndex . and the <User Defined Valid>*/
static void  lion3EPclBuildAllUdbsForKey(
    IN SKERNEL_DEVICE_OBJECT                *devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC      *descrPtr,
    IN SNET_CHT_POLICY_KEY_STC              *pclKeyPtr,
    IN DXCH_EPCL_KEY_ENT                    keyIndex
)
{
    GT_STATUS       rc;
    UDB_INFO_STC* udbInfoArr;
    GT_U8         byteValue;
    GT_U32        udbIndex;
    GT_U32        ii;
    GT_U32        udbValid;
    CHT_PCL_KEY_FIELDS_INFO_STC fieldInfo;
    char            udbNameStr[32];

    if(keyIndex >= DXCH_EPCL_KEY_LAST_E)
    {
        skernelFatalError("lion3EPclBuildAllUdbsForKey: the keyIndex [%d] in not valid \n",keyIndex);
        return ;
    }

    udbValid = 1;
    udbInfoArr = lion3EpclKeysUdbArr[keyIndex];
    fieldInfo.updateOnSecondCycle = GT_FALSE;
    fieldInfo.debugName = udbNameStr;

    for(ii = 0 ; udbInfoArr[ii].udbIndex != SMAIN_NOT_VALID_CNS ; ii++)
    {
        fieldInfo.startBitInKey = udbInfoArr[ii].startBitInKey;
        fieldInfo.endBitInKey = fieldInfo.startBitInKey + 7;/*8 bit field*/

        udbIndex = udbInfoArr[ii].udbIndex;
        sprintf(udbNameStr,"key[%d] UDB[%d]",keyIndex,udbIndex);

        rc = lion3EPclUdbKeyValueGet(devObjPtr, descrPtr,
                                       udbIndex, &byteValue);
        if(rc == GT_FAIL)
        {
            udbValid = 0;
        }

        snetChtPclSrvKeyFieldBuildByValue(pclKeyPtr, byteValue,  &fieldInfo);
    }

    /* the array hold the last entry for the <User Defined Valid> */
    sprintf(udbNameStr,"key[%d] User Defined Valid",keyIndex);
    fieldInfo.startBitInKey = udbInfoArr[ii].startBitInKey;
    fieldInfo.endBitInKey = fieldInfo.startBitInKey;/*single bit field*/
    snetChtPclSrvKeyFieldBuildByValue(pclKeyPtr, udbValid,  &fieldInfo);

    return ;
}

/*******************************************************************************
* lion3EpclKeyPortsBmpBuild
*
* DESCRIPTION:
*        Build the 'ports bmp' in the EPCL search key. sip5 : for keys 0..5 , metadata
*
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - pointer to frame data buffer Id.
*       egressPort      - local egress port (not global).
*
* OUTPUT:
*       pclKeyPtr    - pointer to PCL key structure. (ignored when NULL)
*       portListBmpPtr - pointer to the bmp of ports. (ignored when NULL)
*
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID lion3EpclKeyPortsBmpBuild
(
    IN SKERNEL_DEVICE_OBJECT                * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC   * descrPtr,
    IN  GT_U32                                egressPort,
    OUT SNET_CHT_POLICY_KEY_STC             * pclKeyPtr,
    OUT           GT_U32                  * portListBmpPtr
)
{
    DECLARE_FUNC_NAME(lion3EpclKeyPortsBmpBuild);

    /* PortList bitmap 0-27, separated on 3 parts */
    GT_U32  portListBits7_0   = 0;
    GT_U32  portListBits13_8  = 0;
    GT_U32  portListBits27_14 = 0;
    GT_BIT pclPortGroupEnable; /*When enabled, the 4 msbits of the <Port List> field in the PCL keys is replaced by a 4-bit <Port Group>.*/
    GT_U32 finalPortListBmp; /*final 28 bits of <port list> bmp */
    GT_U32  portGroup;
    GT_U32  regAddr;

    __LOG_PARAM(egressPort);

    snetCht3EPclPortListBmpBuild(devObjPtr, descrPtr , egressPort, &portListBits7_0,
                                 &portListBits13_8, &portListBits27_14);

    finalPortListBmp = (portListBits27_14 << 14) |
                       (portListBits13_8  <<  8) |
                        portListBits7_0;

    regAddr = SMEM_XCAT_ROUTE_HA_GLB_CNF_REG(devObjPtr);
    smemRegFldGet(devObjPtr, regAddr, 19, 1, &pclPortGroupEnable);

    __LOG_PARAM(pclPortGroupEnable);

    if(pclPortGroupEnable)
    {
        /*When enabled, the 4 msbits of the <Port List> field in the PCL keys is replaced by a 4-bit <Port Group>.*/
        portGroup = SMEM_LION3_HA_PHYSICAL_PORT_1_ENTRY_FIELD_AUTO_GET(devObjPtr,
                descrPtr,
                SMEM_LION3_HA_PHYSICAL_PORT_TABLE_1_FIELDS_PORT_GROUP);

        __LOG(("PCL Port Group Enabled , so use <Port Group>[%d] to override 4 msbits of the <Port List> \n",
            portGroup));

        SMEM_U32_SET_FIELD(finalPortListBmp ,24 ,4 ,portGroup);
        portListBits27_14 = finalPortListBmp >> 14;
    }

    __LOG_PARAM(portListBits7_0);
    __LOG_PARAM(portListBits13_8);
    __LOG_PARAM(portListBits27_14);
    __LOG_PARAM(finalPortListBmp);

    if(portListBmpPtr)
    {
        *portListBmpPtr = finalPortListBmp;
    }

    if(pclKeyPtr)
    {
        snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, portListBits7_0 ,
            LION3_EPCL_KEY_FIELD_NAME_BUILD(0  ,  7 ,PCL_ID_7_0_OR_port_list_7_0                ));
        snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, portListBits13_8 ,
            LION3_EPCL_KEY_FIELD_NAME_BUILD(32 , 38 ,Data_Pkt_QoS_Profile_6_0_OR_port_list_13_8 ));
        snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, portListBits27_14 ,
            LION3_EPCL_KEY_FIELD_NAME_BUILD(192,205 ,port_list_27_14                            ));
    }

}

/*******************************************************************************
*   snetEpclPacketTypeGet
*
* DESCRIPTION:
*       EPCL : get egress 'packet type' (inner or outer for TS case)
*       see SKERNEL_EGRESS_PACKET_TYPE_ENT
*       0x0 - IPv4
*       0x1 - IPv6
*       0x2 - MPLS
*       0x3 - Other
*
* INPUTS:
*       devObjPtr - (pointer to) device object.
*       descrPtr     - (pointer to) frame data buffer Id
*       useOuterInfo - the packet type of the outer (TS) or the inner (passenger)
*
* OUTPUTS:
*       keyPacketTypePtr - (pointer to) the packet type field.
* RETURN:
*       None.
*
* COMMENTS:
*
*******************************************************************************/
static GT_VOID snetEpclPacketTypeGet
(
    IN SKERNEL_DEVICE_OBJECT              *devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC    *descrPtr,
    OUT SKERNEL_EGRESS_PACKET_TYPE_ENT    *keyPacketTypePtr,
    IN GT_BOOL                             useOuterInfo
)
{
    DECLARE_FUNC_NAME(snetEpclPacketTypeGet);

    GT_U32  innerEtherType;
    GT_U32  outerEtherType;
    GT_U32  etherType;
    GT_U32  regValue;
    GT_U32  mplsEtherType0,mplsEtherType1;

    if(descrPtr->tunnelStart)
    {
        /* pointer to L3 of passenger */
        innerEtherType = descrPtr->haToEpclInfo.l3StartOffsetPtr[- 2] << 8 |
                         descrPtr->haToEpclInfo.l3StartOffsetPtr[- 1] << 0;

        /* pointer to L3 of tunnel start */
        outerEtherType = descrPtr->haToEpclInfo.tunnelStartL3StartOffsetPtr[- 2] << 8 |
                         descrPtr->haToEpclInfo.tunnelStartL3StartOffsetPtr[- 1] << 0;
    }
    else
    {
        /* pointer to L3 of egress packet */
        outerEtherType = descrPtr->haToEpclInfo.l3StartOffsetPtr[- 2] << 8 |
                         descrPtr->haToEpclInfo.l3StartOffsetPtr[- 1] << 0;
        innerEtherType = outerEtherType;
    }

    etherType = useOuterInfo ? outerEtherType :  innerEtherType;

    if(useOuterInfo)
    {
        __LOG_PARAM(outerEtherType);
    }
    else
    {
        __LOG_PARAM(innerEtherType);
    }

    if(etherType == SKERNEL_L3_PROT_TYPE_IPV4_E)
    {
        *keyPacketTypePtr = SKERNEL_EGRESS_PACKET_IPV4_E;
        __LOG((" ethertype IPV4 \n"));
    }
    else if(etherType == SKERNEL_L3_PROT_TYPE_IPV6_E)
    {
        *keyPacketTypePtr = SKERNEL_EGRESS_PACKET_IPV6_E;
        __LOG((" ethertype IPV6 \n"));
    }
    else
    {
        if (SKERNEL_IS_XCAT_REVISON_A1_DEV(devObjPtr))
        {
            smemRegGet(devObjPtr,
                SMEM_XCAT_HA_MPLS_ETHERTYPES_REG(devObjPtr),
                    &regValue);
        }
        else
        {
            /* start with the ether type */
            regValue = 0x88478847;/*0x8847 every 16 bits*/
        }

        mplsEtherType0 =  regValue       & 0xFFFF;
        mplsEtherType1 = (regValue >> 16)& 0xFFFF;

        __LOG_PARAM(mplsEtherType0);
        __LOG_PARAM(mplsEtherType1);

        /* check MPLS UC,MC */
        if(mplsEtherType0 == etherType ||
           mplsEtherType1 == etherType )
        {
            /* MPLS */
            *keyPacketTypePtr = SKERNEL_EGRESS_PACKET_MPLS_E;
            __LOG((" ethertype MPLS \n"));
        }
        else
        {
            /*other*/
            *keyPacketTypePtr = SKERNEL_EGRESS_PACKET_OTHER_E;
            __LOG((" other (non mpls/ip) \n"));
        }
    }

    return;
}

/*******************************************************************************
*   snetLion3Epcl_srcPortTrgPortGet
*
* DESCRIPTION:
*       get EPCL key <SrcPort> and EPCL Key<Target port>
*               (relevant for SIP5 only)
*
* INPUTS:
*       devObjPtr - (pointer to) device object.
*       descrPtr  - (pointer to) frame data buffer Id
*       egressPort  - the egress physical port
*
* OUTPUTS:
*       srcPortPtr - pointer to EPCL key <SrcPort>
*       trgPortPtr - pointer to EPCL key <Target port>
*
* RETURN:
*       None
*
* COMMENTS:
*
*******************************************************************************/
static void snetLion3Epcl_srcPortTrgPortGet
(
    IN  SKERNEL_DEVICE_OBJECT           *devObjPtr,
    INOUT  SKERNEL_FRAME_CHEETAH_DESCR_STC *descrPtr,
    IN  GT_U32                  egressPort,
    OUT GT_U32                  *srcPortPtr,
    OUT GT_U32                  *trgPortPtr
)
{
    DECLARE_FUNC_NAME(snetLion3Epcl_srcPortTrgPortGet);

    GT_U32  regAddr,regValue;
    GT_U32  srcPort,trgPort;
    GT_BIT  keySrcPortModeForDsaTaggedPackets;/*EPCL key <SrcPort> always reflects the local device source physical
                ingress port, even for DSA-tagged packets. To maintain backward
                compatibility for DSA-tagged packets, a global mode allows working in the
                backward compatible mode where <SrcPort> is taken from the DSA tag if
                packet was received DSA tagged.
                0 = Original: Backwards compatible mode - for DSA tagged packet,
                    <SrcPort> is taken from the DSA tag
                1 = Local: For all packets (DSA tagged or not), <SrcPort> indicates the
                    local device ingress physical port*/
    GT_BIT  keyTrgPortMode;/*EPCL Key<Target port> field source selector.
                Can be set to final destination (Desc<Trg PHY Port>) or local physical port
                (Desc<Local Dev Trg PHY Port>).
                0 = Local: Local Device Physical Port
                1 = Final: Final Destination Port*/

    regAddr = SMEM_XCAT_ROUTE_HA_GLB_CNF_REG(devObjPtr);
    smemRegGet(devObjPtr, regAddr,&regValue);

    keySrcPortModeForDsaTaggedPackets = SMEM_U32_GET_FIELD(regValue,20,1);
    keyTrgPortMode = SMEM_U32_GET_FIELD(regValue,21,1);

    __LOG_PARAM(keySrcPortModeForDsaTaggedPackets);
    __LOG_PARAM(keyTrgPortMode);

    if(0 == keySrcPortModeForDsaTaggedPackets)
    {
        switch(descrPtr->outGoingMtagCmd)
        {
            case SKERNEL_MTAG_CMD_FORWARD_E:
                __LOG(("srcPort for FORAWRD \n"))
                srcPort = descrPtr->origIsTrunk ? 0 : descrPtr->origSrcEPortOrTrnk;
                break;
            case SKERNEL_MTAG_CMD_FROM_CPU_E:
                __LOG(("srcPort for FROM_CPU \n"))
                srcPort = SNET_CHT_CPU_PORT_CNS;
                break;
            default:/*TO_CPU/TO_TARGET_SNIFFER*/
                __LOG(("srcPort for TO_CPU/TO_TARGET_SNIFFER \n"))
                srcPort = descrPtr->srcTrgPhysicalPort;
                break;
        }
    }
    else
    {
        __LOG(("srcPort regardless to outGoingMtagCmd \n"))
        srcPort = descrPtr->localDevSrcPort;
    }

    if((descrPtr->srcTrgDev == descrPtr->ownDev) &&
       ((descrPtr->outGoingMtagCmd == SKERNEL_MTAG_CMD_TO_CPU_E && descrPtr->srcTrg == 0)/*Src*/ ||
        (descrPtr->outGoingMtagCmd == SKERNEL_MTAG_CMD_TO_TRG_SNIFFER_E && descrPtr->rxSniff)))
    {
        __LOG(("trgPort for TO_CPU(src)/TO_TARGET_SNIFFER(rxSniff) \n"))
        trgPort = descrPtr->srcTrgPhysicalPort;
    }
    else
    {
        if (keyTrgPortMode &&
            (descrPtr->outGoingMtagCmd != SKERNEL_MTAG_CMD_TO_CPU_E) &&
            (descrPtr->useVidx == 0) &&
            descrPtr->eArchExtInfo.isTrgPhyPortValid)
        {
            __LOG(("trgPort for keyTrgPortMode && useVidx == 0 && isTrgPhyPortValid \n"))
            trgPort = descrPtr->eArchExtInfo.trgPhyPort;
        }
        else
        {
            __LOG(("trgPort according to local_dev_trg_phy_port \n"))
            trgPort = egressPort;/*local_dev_trg_phy_port*/
        }
    }

    __LOG_PARAM(srcPort);
    __LOG_PARAM(trgPort);

    *srcPortPtr = srcPort;
    *trgPortPtr = trgPort;
    return;
}

/*******************************************************************************
*   snetLion3Epcl_isUnknownGet
*
* DESCRIPTION:
*       get EPCL key <isUnknown>
*               (relevant for SIP5 only)
*
* INPUTS:
*       devObjPtr - (pointer to) device object.
*       descrPtr  - (pointer to) frame data buffer Id
*       egressPort  - the egress physical port
*
* OUTPUTS:
*       isUnknownPtr - pointer to EPCL key <isUnknown>
*
* RETURN:
*       None
*
* COMMENTS:
*
*******************************************************************************/
static void snetLion3Epcl_isUnknownGet
(
    IN  SKERNEL_DEVICE_OBJECT           *devObjPtr,
    INOUT  SKERNEL_FRAME_CHEETAH_DESCR_STC *descrPtr,
    IN  GT_U32                  egressPort,
    OUT GT_U32                  *isUnknownPtr
)
{
    DECLARE_FUNC_NAME(snetLion3Epcl_isUnknownGet);

    GT_BIT is_unknown;

    if(descrPtr->outGoingMtagCmd == SKERNEL_MTAG_CMD_FROM_CPU_E ||
       descrPtr->outGoingMtagCmd == SKERNEL_MTAG_CMD_FORWARD_E)
    {
        __LOG(("is_unknown according to opposite of descrPtr->egressFilterRegistered \n"))
        is_unknown = descrPtr->egressFilterRegistered ? 0 : 1;
    }
    else if(descrPtr->outGoingMtagCmd == SKERNEL_MTAG_CMD_TO_CPU_E)
    {
        __LOG(("is_unknown according bit 7 of CPU code \n"))
        /* use bit 7 of the CPU code ! */
        is_unknown = SMEM_U32_GET_FIELD(descrPtr->cpuCode,7,1);
    }
    else /*TO_TARGET_SNIFFER*/
    {
        __LOG(("is_unknown is not valid for 'TO_TARGET_SNIFFER' --> set to 0 \n"))
        is_unknown = 0;
    }

    *isUnknownPtr = is_unknown;

    return;
}

/*******************************************************************************
*   snetLion3Epcl_vlanTagInfoGet
*
* DESCRIPTION:
*       get EPCL key vlan tag info (tag0,tag1)
*               (relevant for SIP5 only)
*
* INPUTS:
*       devObjPtr - (pointer to) device object.
*       descrPtr  - (pointer to) frame data buffer Id
*
* OUTPUTS:
*       tag0ExistPtr - pointer to EPCL key <tag0Exist>
*       tag1ExistPtr - pointer to EPCL key <tag0Exist>
*       tag0IsOuterTagPtr - pointer to EPCL key <tag0IsOuterTag>
*       tag0Ptr - pointer to EPCL key <tag0>
*       tag1Ptr - pointer to EPCL key <tag1>
*
* RETURN:
*       None
*
* COMMENTS:
*
*******************************************************************************/
static void snetLion3Epcl_vlanTagInfoGet
(
    IN  SKERNEL_DEVICE_OBJECT           *devObjPtr,
    INOUT  SKERNEL_FRAME_CHEETAH_DESCR_STC *descrPtr,
    OUT GT_BIT                  *tag0ExistPtr,
    OUT GT_BIT                  *tag1ExistPtr,
    OUT GT_BIT                  *tag0IsOuterTagPtr,
    OUT GT_U32                  *tag0Ptr,
    OUT GT_U32                  *tag1Ptr
)
{
    DECLARE_FUNC_NAME(snetLion3Epcl_vlanTagInfoGet);

    GT_BIT egress_tag_0_exist,egress_tag_1_exist,egress_tag0_is_outer_tag;
    GT_U32  egress_tag0 = 0,egress_tag1 = 0;

    egress_tag_0_exist = descrPtr->haToEpclInfo.vlanTag0Ptr ? 1 : 0;
    egress_tag_1_exist = descrPtr->haToEpclInfo.vlanTag1Ptr ? 1 : 0;
    if(egress_tag_0_exist && egress_tag_1_exist)
    {
        egress_tag0_is_outer_tag = (descrPtr->haToEpclInfo.vlanTag0Ptr < descrPtr->haToEpclInfo.vlanTag1Ptr) ? 1 : 0;
    }
    else
    {
        egress_tag0_is_outer_tag = egress_tag_0_exist ? 1 : 0;
    }

    if(descrPtr->haToEpclInfo.vlanTag0Ptr)
    {
        egress_tag0 = descrPtr->haToEpclInfo.vlanTag0Ptr[0] << 24 |
               descrPtr->haToEpclInfo.vlanTag0Ptr[1] << 16 |
               descrPtr->haToEpclInfo.vlanTag0Ptr[2] <<  8 |
               descrPtr->haToEpclInfo.vlanTag0Ptr[3] <<  0 ;
    }

    if(descrPtr->haToEpclInfo.vlanTag1Ptr)
    {
        egress_tag1 = descrPtr->haToEpclInfo.vlanTag1Ptr[0] << 24 |
               descrPtr->haToEpclInfo.vlanTag1Ptr[1] << 16 |
               descrPtr->haToEpclInfo.vlanTag1Ptr[2] <<  8 |
               descrPtr->haToEpclInfo.vlanTag1Ptr[3] <<  0 ;
    }

    __LOG_PARAM(egress_tag_0_exist);
    __LOG_PARAM(egress_tag_1_exist);
    __LOG_PARAM(egress_tag0_is_outer_tag);
    __LOG_PARAM(egress_tag0);
    __LOG_PARAM(egress_tag1);

    *tag0ExistPtr = egress_tag_0_exist;
    *tag1ExistPtr = egress_tag_1_exist;
    *tag0IsOuterTagPtr = egress_tag0_is_outer_tag;
    *tag0Ptr = egress_tag0;
    *tag1Ptr = egress_tag1;

    return;
}


/*******************************************************************************
*   snetLion3Epcl_mplsInfoGet
*
* DESCRIPTION:
*       get EPCL key MPLS labels info
*               (relevant for SIP5 only)
*
* INPUTS:
*       devObjPtr - (pointer to) device object.
*       descrPtr  - (pointer to) frame data buffer Id
*       egressPort  - the egress physical port
*
* OUTPUTS:
*       is_Mpls_Ptr - pointer to EPCL key <is_Mpls>
*       mplsNumOfLabels_Ptr - pointer to EPCL key <mplsNumOfLabels>
*       mplsNumOfLabels_Ptr - pointer to EPCL key <mplsNumOfLabels>
*       reserved_label_exists_Ptr - pointer to EPCL key <reserved_label_exists>
*       data_after_inner_label_Ptr - pointer to EPCL key <data_after_inner_label>
*       protocol_after_mpls_Ptr - pointer to EPCL key <protocol_after_mpls>
*
* RETURN:
*       None
*
* COMMENTS:
*
*******************************************************************************/
static void snetLion3Epcl_mplsInfoGet
(
    IN  SKERNEL_DEVICE_OBJECT           *devObjPtr,
    INOUT  SKERNEL_FRAME_CHEETAH_DESCR_STC *descrPtr,
    IN  GT_U32                  egressPort,
    OUT GT_BIT                  *is_Mpls_Ptr,
    OUT GT_BIT                  *mplsNumOfLabels_Ptr,
    OUT GT_BIT                  *reserved_label_exists_Ptr,
    OUT GT_U32                  *reserved_label_value_Ptr,
    OUT GT_U32                  *data_after_inner_label_Ptr,
    OUT GT_U32                  *protocol_after_mpls_Ptr
)
{
    DECLARE_FUNC_NAME(snetLion3Epcl_mplsInfoGet);

    GT_U32  ii;
    GT_BIT  is_Mpls;
    GT_U8   *mplsPtr;
    GT_BIT  reserved_label_exists;/*mpls related*/
    GT_U32  reserved_label_value;/*mpls related*/
    GT_U32  data_after_inner_label;/*mpls related - data after all mpls labels*/
    GT_U32  currentMplsLabel;/*current mpls label*/
    GT_U32  actualMplsNumOfLabelsValid;/* actual number of valid MPLS labels */
    GT_U32  mplsNumOfLabels;/* value to the key */
    GT_U32  protocol_after_mpls;/*protocol after MPLS*/
    GT_U32  firstWordAfterMPLS;/*first word after MPLS*/
    GT_U32  mplsLastSbit;/*value of the 'S' bit in the last label*/

    if(descrPtr->isMplsLsr) /* the LSR comes with TS */
    {
        mplsPtr = descrPtr->haToEpclInfo.tunnelStartL3StartOffsetPtr;
    }
    else if (descrPtr->mpls)
    {
        mplsPtr = descrPtr->haToEpclInfo.l3StartOffsetPtr;
    }
    else/* non MPLS labels */
    {
        mplsPtr = NULL;
    }

    mplsNumOfLabels = 0;
    reserved_label_exists = 0;
    reserved_label_value = 0;
    data_after_inner_label = 0;
    actualMplsNumOfLabelsValid = 0;/*0 means 0 , 1 means 1 label ..*/
    protocol_after_mpls = 0;
    is_Mpls = 0;

    if(mplsPtr)
    {
        is_Mpls = 1;
        mplsLastSbit = 0;
        for(ii = 0 ; ii < LION3_EPCL_MAX_MPLS_LABELS /*4*/ ; ii++)
        {
            currentMplsLabel = mplsPtr[(ii*4) + 0] << 24 |
                               mplsPtr[(ii*4) + 1] << 16 |
                               mplsPtr[(ii*4) + 2] <<  8 |
                               mplsPtr[(ii*4) + 3] <<  0 ;

            __LOG_PARAM(currentMplsLabel);

            if(reserved_label_exists == 0)
            {
                /* the 4 LSB of the 'label' field */
                reserved_label_value = SMEM_U32_GET_FIELD(currentMplsLabel,12,4);

                /* the values 0..15 are 'reserved' */
                if(SMEM_U32_GET_FIELD(currentMplsLabel,12,20) <= 15)
                {
                    reserved_label_exists = 1;
                }
            }

            actualMplsNumOfLabelsValid++;

            /* the S bit */
            mplsLastSbit = SMEM_U32_GET_FIELD(currentMplsLabel,8,1);
            if(mplsLastSbit)
            {
                break;
            }
        }

        __LOG_PARAM(actualMplsNumOfLabelsValid);

        mplsNumOfLabels = actualMplsNumOfLabelsValid;

        if(mplsNumOfLabels == LION3_EPCL_MAX_MPLS_LABELS/*4*/)
        {
            __LOG(("mplsNumOfLabels --> value 0 means more then 3 labels \n"));
            mplsNumOfLabels = 0;
        }

        __LOG_PARAM(mplsNumOfLabels);

        firstWordAfterMPLS = mplsPtr[(actualMplsNumOfLabelsValid*4) + 0] << 24 |
                             mplsPtr[(actualMplsNumOfLabelsValid*4) + 1] << 16 |
                             mplsPtr[(actualMplsNumOfLabelsValid*4) + 2] <<  8 |
                             mplsPtr[(actualMplsNumOfLabelsValid*4) + 3] <<  0 ;

        switch (SMEM_U32_GET_FIELD(firstWordAfterMPLS,28,4))/*first Nibble After MPLS*/
        {
            case 4:
                protocol_after_mpls = 0; /* protocol over MPLS is IPv4 */
                break;
            case 6:
                protocol_after_mpls = 1; /* protocol over MPLS is IPv6 */
                break;
            default:
                protocol_after_mpls = 2; /* protocol over MPLS is Other */
                break;
        }

        data_after_inner_label = mplsLastSbit ? SMEM_U32_GET_FIELD(firstWordAfterMPLS,27,5) : 0;
    }

    __LOG_PARAM(is_Mpls               );
    __LOG_PARAM(mplsNumOfLabels       );
    __LOG_PARAM(reserved_label_exists );
    __LOG_PARAM(reserved_label_value  );
    __LOG_PARAM(data_after_inner_label);
    __LOG_PARAM(protocol_after_mpls   );

    *is_Mpls_Ptr                  =  is_Mpls;
    *mplsNumOfLabels_Ptr          =  mplsNumOfLabels;
    *reserved_label_exists_Ptr    =  reserved_label_exists;
    *reserved_label_value_Ptr     =  reserved_label_value;
    *data_after_inner_label_Ptr   =  data_after_inner_label;
    *protocol_after_mpls_Ptr      =  protocol_after_mpls;

    return;
}


/*******************************************************************************
*   snetLion3EpclUdbMetaDataBuild
*
* DESCRIPTION:
*       build the metadata byte for the EPCL from the packet descriptor fields
*               (relevant for SIP5 only)
*
* INPUTS:
*       devObjPtr - (pointer to) device object.
*       descrPtr  - (pointer to) frame data buffer Id
*       pclExtraDataPtr - (pointer to) extra data needed for the EPCL engine.
*       egressPort  - the egress physical port
*
* OUTPUTS:
*       descrPtr->epclMetadataInfo[] - the place to hold the info of the metadata
*
* RETURN:
*       None
*
* COMMENTS:
*
*******************************************************************************/
static void snetLion3EpclUdbMetaDataBuild
(
    IN  SKERNEL_DEVICE_OBJECT           *devObjPtr,
    INOUT  SKERNEL_FRAME_CHEETAH_DESCR_STC *descrPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC    *pclExtraDataPtr,
    IN  GT_U32                                egressPort
)
{
    DECLARE_FUNC_NAME(snetLion3EpclUdbMetaDataBuild);

    GT_STATUS rc;
    GT_U32  portListBmp;
    GT_U32  srcPort,trgPort;
    GT_U32  isIPv6,ip_legal_or_fcoe_legal;
    GT_BIT is_unknown,l2Encaps,tag_0_exist,tag_1_exist,is_BC,tag0_is_outer_tag;
    GT_BIT incoming_egress_filter_en;
    GT_U8*  macDaPtr;/*pointer to mac DA of ethernet passenger or mac DA of the tunnel */
    GT_U32  tag0,tag1;
    GT_U8  tcporudp_port_comparators;
    GT_U32  lm_counter_or_timestamp;
    GT_BIT  lm_counter_insert_en;
    GT_BIT  is_Mpls;
    GT_BIT  reserved_label_exists;/*mpls related*/
    GT_U32  reserved_label_value;/*mpls related*/
    GT_U32  data_after_inner_label;/*mpls related - data after all mpls labels*/
    GT_U32  mplsNumOfLabels;/* value to the key */
    GT_U32  protocol_after_mpls;/*protocol after MPLS*/
    GT_BIT  dp2cfi_map_en;
    GT_BIT  add_crc;
    GT_U32  num_of_crc_in_egress_byte_count;
    GT_BIT  packet_cmd_drop_or_forward;
    GT_U32  txq_queue_id;
    SKERNEL_EGRESS_PACKET_TYPE_ENT  outer_packet_type;
    SKERNEL_EGRESS_PACKET_TYPE_ENT  inner_packet_type;
    GT_BIT  src_trg_or_rx_sniff;
    GT_U32  analyzer_trg_eport,analyzer_trg_dev,analyzer_trg_phy_port;
    GT_BIT  analyzer_is_trg_phy_port_valid;
    GT_BIT  l2Valid;

    EPCL_KEY_FIELD_NOT_IMPLEMENTED_MAC(dp2cfi_map_en,0);
    EPCL_KEY_FIELD_NOT_IMPLEMENTED_MAC(add_crc,0);
    EPCL_KEY_FIELD_NOT_IMPLEMENTED_MAC(num_of_crc_in_egress_byte_count,0);
    EPCL_KEY_FIELD_NOT_IMPLEMENTED_MAC(packet_cmd_drop_or_forward,0);
    EPCL_KEY_FIELD_NOT_IMPLEMENTED_MAC(txq_queue_id,0);

    /* get the portListBmp */
    lion3EpclKeyPortsBmpBuild(devObjPtr, descrPtr,egressPort, NULL,&portListBmp);
    snetLion3Epcl_srcPortTrgPortGet(devObjPtr, descrPtr,egressPort,&srcPort,&trgPort);

    isIPv6 = descrPtr->isIp && (descrPtr->isIPv4==0);
    ip_legal_or_fcoe_legal = descrPtr->isFcoe ?
        descrPtr->fcoeInfo.fcoeLegal :
        (descrPtr->ipHeaderError == 0);/*Ip legal*/

    snetLion3Epcl_isUnknownGet(devObjPtr, descrPtr,egressPort,&is_unknown);

    l2Encaps = (descrPtr->l2Encaps == SKERNEL_ETHERNET_II_E ||
                descrPtr->l2Encaps == SKERNEL_LLC_SNAP_E) ? 1 : 0;

    /*for TS that is not with ethernet passenger this is NULL */
    /*for TS that is     with ethernet passenger this is the mac da of the passenger */
    /*for non-TS that this is the mac da of the packet */
    macDaPtr = descrPtr->haToEpclInfo.macDaSaPtr;

    if(macDaPtr)
    {
        if (SGT_MAC_ADDR_IS_BCST(macDaPtr))
        {
            is_BC = 1;
        }
        else
        {
            is_BC = 0;
        }
    }
    else
    {
        is_BC = 0;
    }

    l2Valid = macDaPtr ? 1 : 0;

    snetLion3Epcl_vlanTagInfoGet(devObjPtr, descrPtr,
        &tag_0_exist,&tag_1_exist,&tag0_is_outer_tag,
        &tag0,&tag1);

    if(descrPtr->marvellTagged && descrPtr->incomingMtagCmd == SKERNEL_MTAG_CMD_FROM_CPU_E)
    {
        incoming_egress_filter_en = descrPtr->ingressDsa.fromCpu_egressFilterEn;
    }
    else
    {
        incoming_egress_filter_en = 0;
    }

    rc = snetCht2EPclTcpUdpPortRangeCompareGet(devObjPtr, descrPtr,
                                              &tcporudp_port_comparators);
    if(rc != GT_OK)
    {
        tcporudp_port_comparators = 0;
    }

    snetLion3Epcl_mplsInfoGet(devObjPtr, descrPtr,egressPort,
        &is_Mpls,
        &mplsNumOfLabels ,&reserved_label_exists ,
        &reserved_label_value, &data_after_inner_label,
        &protocol_after_mpls);


    snetEpclPacketTypeGet(devObjPtr, descrPtr,&outer_packet_type,GT_TRUE);
    if(descrPtr->tunnelStart)
    {
        snetEpclPacketTypeGet(devObjPtr, descrPtr,&inner_packet_type,GT_FALSE);
    }
    else
    {
        inner_packet_type = outer_packet_type;
    }

    if(descrPtr->outGoingMtagCmd == SKERNEL_MTAG_CMD_TO_CPU_E)
    {
        src_trg_or_rx_sniff = descrPtr->srcTrg;
    }
    else
    if(descrPtr->outGoingMtagCmd == SKERNEL_MTAG_CMD_TO_TRG_SNIFFER_E)
    {
        src_trg_or_rx_sniff = descrPtr->rxSniff;
    }
    else
    {
        src_trg_or_rx_sniff = 0;
    }

    /* next code aligned to VERIFIER ... */
    if(descrPtr->useVidx == 0 && descrPtr->outGoingMtagCmd != SKERNEL_MTAG_CMD_TO_CPU_E)
    {
        analyzer_trg_eport              = descrPtr->trgEPort;
        analyzer_trg_dev                = descrPtr->trgDev;
        analyzer_trg_phy_port           = descrPtr->eArchExtInfo.trgPhyPort;
        analyzer_is_trg_phy_port_valid  = descrPtr->eArchExtInfo.isTrgPhyPortValid;
    }
    else
    {
        analyzer_trg_eport    = SNET_CHT_NULL_PORT_CNS;
        analyzer_trg_phy_port = SNET_CHT_NULL_PORT_CNS;
        analyzer_trg_dev      = 0;
        analyzer_is_trg_phy_port_valid = 0;

    }

    lm_counter_insert_en = descrPtr->oamInfo.lmCounterInsertEnable;
    /* LM Counter Insert Enable */
    if(lm_counter_insert_en)
    {
        lm_counter_or_timestamp = descrPtr->oamInfo.lmCounter;
    }
    else
    {
        lm_counter_or_timestamp =
            descrPtr->timestampTagInfo[SMAIN_DIRECTION_INGRESS_E].timestamp.nanoSecondTimer;
    }

    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_PORT_LIST_TRG_E                          ,portListBmp);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_LOCAL_DEV_SRC_TRUNK_ID_E                 ,descrPtr->localDevSrcTrunkId);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_LOCAL_DEV_SRC_EPORT_E                    ,descrPtr->eArchExtInfo.localDevSrcEPort);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_RESERVED_1_E                             ,0);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_ORIG_SRC_DEV_E                           ,descrPtr->srcDev);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_ORIG_SRC_IS_TRUNK_E                      ,descrPtr->origIsTrunk);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_ORIG_SRC_EPORT_OR_TRUNK_ID_E             ,descrPtr->origSrcEPortOrTrnk);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_TRG_PORT_E                               ,trgPort);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_IS_TRG_PHY_PORT_VALID_E                  ,descrPtr->eArchExtInfo.isTrgPhyPortValid);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_RESERVED_2_E                             ,0);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_LOCAL_DEV_TRG_PHY_PORT_E                 ,egressPort);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_TRG_DEV_E                                ,descrPtr->trgDev);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_TRG_EPORT_E                              ,descrPtr->trgEPort);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_ASSIGN_TRG_EPORT_ATTRIBUTES_LOCALLY_E    ,descrPtr->eArchExtInfo.assignTrgEPortAttributesLocally);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_SRC_PORT_E                               ,srcPort);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_USE_VIDX_E                               ,descrPtr->useVidx);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_EVIDX_E                                  ,descrPtr->eVidx);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_EGRESS_UDB_PACKET_TYPE_E                 ,descrPtr->epcl_pcktType_sip5);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_IP_LEGAL_E                               ,ip_legal_or_fcoe_legal);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_L2_VALID_E                               ,l2Valid);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_IS_ARP_E                                 ,descrPtr->arp);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_SRC_ID_E                                 ,descrPtr->sstId);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_EGRESS_FILTER_EN_E                       ,descrPtr->egressFilterEn);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_EGRESS_FILTER_REGISTERED_E               ,descrPtr->egressFilterRegistered);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_INCOMING_EGRESS_FILTER_EN_E              ,incoming_egress_filter_en);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_RESERVED_4_E                             ,0);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_ETHERTYPE_OR_DSAPSSAP_E                  ,descrPtr->etherTypeOrSsapDsap);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_IS_UNKNOWN_E                             ,is_unknown);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_L2_VALID_1_E                             ,l2Valid);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_L2_ENCAPSULATION_TYPE_E                  ,l2Encaps);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_IS_BC_E                                  ,is_BC);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_ORIG_VID_E                               ,descrPtr->vid0Or1AfterTti);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_EVLAN_E                                  ,descrPtr->eVid);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_TAG0_EXIST_E                             ,tag_0_exist);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_TAG1_EXIST_E                             ,tag_1_exist);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_TAG0_IS_OUTER_TAG_E                      ,tag0_is_outer_tag);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_TAG0_UP_E                                ,tag_0_exist ? SMEM_U32_GET_FIELD(tag0,13,3) : descrPtr->up);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_TAG0_CFI_E                               ,tag_0_exist ? SMEM_U32_GET_FIELD(tag0,12,1) : descrPtr->cfidei);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_TAG1_UP_E                                ,tag_1_exist ? SMEM_U32_GET_FIELD(tag1,13,3) : descrPtr->up1);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_TAG1_CFI_E                               ,tag_1_exist ? SMEM_U32_GET_FIELD(tag1,12,1) : descrPtr->cfidei1);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_TAG1_VID_E                               ,tag_1_exist ? SMEM_U32_GET_FIELD(tag1,0,12) : descrPtr->vid1);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_EVB_OR_BPE_OR_RSPAN_TAG_SIZE_E           ,descrPtr->haToEpclInfo.evbBpeRspanTagSize / 4);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_RESERVED_5_E                             ,0);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_IP_PROTOCOL_E                            ,descrPtr->ipProt);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_IPV4_SIP_OR_ARP_SIP_E                    ,descrPtr->sip[0]);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_IPV4_DIP_OR_ARP_DIP_E                    ,descrPtr->dip[0]);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_ROUTED_E                                 ,descrPtr->routed);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_IS_IP_E                                  ,descrPtr->isIp);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_IS_IPV4_E                                ,descrPtr->isIPv4);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_IS_IPV6_E                                ,isIPv6);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_IP_LEGAL_1_E                               ,ip_legal_or_fcoe_legal);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_IS_ARP_1_E                                 ,descrPtr->arp);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_IPV4_FRAGMENTED_E                        ,pclExtraDataPtr->isIpv4Fragment);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_IPV4_OPTIONS_E                           ,descrPtr->ipv4HeaderOptionsExists);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_IS_IP_1_E                                  ,descrPtr->isIp);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_IS_IPV4_1_E                                ,descrPtr->isIPv4);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_IS_IPV6_1_E                                ,isIPv6);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_IP_LEGAL_2_E                               ,ip_legal_or_fcoe_legal);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_IPV6_EH_EXIST_E                          ,pclExtraDataPtr->isIpV6EhExists);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_IPV6_EH_HOP_BY_HOP_E                     ,pclExtraDataPtr->isIpV6EhHopByHop);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_IPV6_IS_ND_E                             ,descrPtr->solicitationMcastMsg);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_L4_VALID_E                               ,descrPtr->l4Valid);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_TCP_OR_UDP_PORT_COMPARATORS_E            ,tcporudp_port_comparators);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_IS_PTP_E                                 ,descrPtr->isPtp);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_TIMESTAMP_EN_E                           ,descrPtr->oamInfo.timeStampEnable);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_INGRESS_TIMESTAMP_TAGGED_E               ,descrPtr->timestampTagged[SMAIN_DIRECTION_INGRESS_E]);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_TIMESTAMP_TAGGED_E                       ,descrPtr->timestampTagged[SMAIN_DIRECTION_EGRESS_E]);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_LM_COUNTER_OR_TIMESTAMP_E                ,lm_counter_or_timestamp);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_LM_COUNTER_INSERT_EN_E                   ,lm_counter_insert_en   );
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_PTP_U_FIELD_E                            ,descrPtr->ptpUField);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_PTP_TAI_SELECT_E                         ,descrPtr->ptpEgressTaiSel);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_PTP_DOMAIN_E                             ,descrPtr->ptpDomain);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_PTP_TRIGGER_TYPE_E                       ,descrPtr->ptpTriggerType);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_PTP_MESSAGE_TYPE_E                       ,descrPtr->ptpGtsInfo.gtsEntry.msgType);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_CONTROL_WORD_CHANNEL_TYPE_PROFILE_E      ,descrPtr->channelTypeProfile);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_PTP_VERSION_E                            ,descrPtr->ptpGtsInfo.gtsEntry.ptpVersion);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_OAM_PTP_OFFSET_INDEX_OR_PTP_OFFSET_E     ,descrPtr->oamInfo.offsetIndex);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_RESERVED_LABEL_EXISTS_E                  ,reserved_label_exists);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_RESERVED_LABEL_VALUE_E                   ,reserved_label_value );
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_RESERVED_6_E                             ,0);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_DATA_AFTER_INNER_LABEL_E                 ,data_after_inner_label);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_RESERVED_7_E                             ,0);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_SRC_TRG_OR_RX_SNIFF_E                    ,src_trg_or_rx_sniff);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_ANALYZER_TRG_EPORT_E                     ,analyzer_trg_eport);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_ANALYZER_TRG_DEV_E                       ,analyzer_trg_dev);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_ANALYZER_TRG_PHY_PORT_E                  ,analyzer_trg_phy_port);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_SRC_TRG_TAG0_TAGGED_E                    ,descrPtr->srcTaggedTrgTagged);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_ANALYZER_IS_TRG_PHY_PORT_VALID_E         ,analyzer_is_trg_phy_port_valid);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_RESERVED_8_E                             ,0);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_IS_MPLS_E                                ,is_Mpls);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_PROTOCOL_AFTER_MPLS_E                    ,protocol_after_mpls);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_NUMBER_OF_MPLS_LABELS_E                  ,mplsNumOfLabels);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_IS_TUNNELED_E                            ,descrPtr->tunnelStart);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_TUNNEL_START_PASSENGER_TYPE_E            ,descrPtr->tunnelStartPassengerType);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_RESERVED_9_E                             ,0);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_QOS_PROFILE_E                            ,descrPtr->qos.qosProfile);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_QOS_MAPPING_TABLE_INDEX_E                ,descrPtr->haToEpclInfo.qos.qosMapTableIndex);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_FROM_CPU_DP_E                            ,descrPtr->qos.fromCpuQos.fromCpuDp);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_PACKET_TC_E                              ,descrPtr->tc);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_EGRESS_UP_MAP_EN_E                       ,descrPtr->haToEpclInfo.qos.egressUpMapEn);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_EGRESS_DSCP_MAP_EN_E                     ,descrPtr->haToEpclInfo.qos.egressDscpMapEn);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_EGRESS_EXP_MAP_EN_E                      ,descrPtr->haToEpclInfo.qos.egressExpMapEn);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_EGRESS_TC_DP_MAP_EN_E                    ,descrPtr->haToEpclInfo.qos.egressTcDpMapEn);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_EGRESS_DP2CFI_MAP_EN_E                   ,dp2cfi_map_en);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_INGRESS_BYTE_COUNT_E                     ,descrPtr->origByteCount);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_INCOMING_MTAG_CMD_E                      ,descrPtr->incomingMtagCmd);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_EGRESS_MTAG_CMD_E                        ,descrPtr->outGoingMtagCmd);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_EGRESS_MARVELL_TAGGED_E                  ,(descrPtr->egrMarvellTagType == MTAG_TYPE_NONE_E ? 0 : 1));
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_EGRESS_MARVELL_TAGGED_EXTENDED_E         ,(descrPtr->egrMarvellTagType == MTAG_TYPE_NONE_E ? 0 : descrPtr->egrMarvellTagType));
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_ADD_CRC_E                                ,add_crc);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_NUM_OF_CRC_IN_EGRESS_BYTE_COUNT_E        ,num_of_crc_in_egress_byte_count);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_INGRESS_MARVELL_TAGGED_E                 ,descrPtr->marvellTagged);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_RESERVED_10_E                            ,0);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_TRANSMITTED_DSA_TAG_WORD_0_E             ,descrPtr->egress_dsaWords[0]);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_TRANSMITTED_DSA_TAG_WORD_1_E             ,descrPtr->egress_dsaWords[1]);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_TRANSMITTED_DSA_TAG_WORD_2_E             ,descrPtr->egress_dsaWords[2]);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_TRANSMITTED_DSA_TAG_WORD_3_E             ,descrPtr->egress_dsaWords[3]);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_FLOW_ID_E                                ,descrPtr->flowId);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_OUTER_PACKET_TYPE_E                      ,outer_packet_type);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_INNER_PACKET_TYPE_E                      ,inner_packet_type);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_PACKET_CMD_DROP_OR_FORWARD_E             ,packet_cmd_drop_or_forward);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_TXQ_QUEUE_ID_E                           ,txq_queue_id);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_PACKET_HASH_E                            ,descrPtr->pktHash);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_CPU_CODE_E                               ,descrPtr->cpuCode);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_TWO_BYTE_HEADER_ADDED_E                  ,descrPtr->haToEpclInfo.prePendLength ? 1 : 0);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_COPY_RESERVED_E                          ,0);
    SMEM_LION3_EPCL_METADATA_FIELD_SET(devObjPtr,descrPtr,SMEM_LION3_EPCL_META_DATA_FIELDS_RESERVED_11_E                            ,0);

}


/*******************************************************************************
*   lion3EpclKey60UdbBuildKeyExtra10Bytes
*
* DESCRIPTION:
*        Lion3 EPCL - build the extra 10 bytes of the key (after the first 50 UDBs)
*                   of the 60B Key
*
* INPUTS:
*       devObjPtr            - pointer to device object.
*       descrPtr             - pointer to frame data buffer Id.
*       curLookUpCfgPtr      - pointer to the current PCL configuration entry.
*       pclExtraDataPtr      - pointer to extra data needed for the IPCL engine.
*       udbValid             - indication that ALL the 50 UDBs are valid
*                               (to be part of the extra 10B info)
* OUTPUTS:
*       pclKeyPtr    - pointer to PCL key structure.
*
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID lion3EpclKey60UdbBuildKeyExtra10Bytes
(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC  * curLookUpCfgPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC    * pclExtraDataPtr,
    OUT SNET_CHT_POLICY_KEY_STC         * pclKeyPtr,
    IN  GT_U32                            egressPort,
    IN GT_BIT                           udbValid
)
{
    snetLion3EPclKeyFieldBuildByValue(pclKeyPtr,    udbValid,
        LION3_EPCL_KEY_60B_EXTRA_10B_FIELD_NAME_BUILD(400,  400 , UDB_Valid                  ));
    snetLion3EPclKeyFieldBuildByValue(pclKeyPtr,    curLookUpCfgPtr->pclID,
        LION3_EPCL_KEY_60B_EXTRA_10B_FIELD_NAME_BUILD(401,  410 , PCL_ID                     ));
    snetLion3EPclKeyFieldBuildByValue(pclKeyPtr,    descrPtr->eVid,
        LION3_EPCL_KEY_60B_EXTRA_10B_FIELD_NAME_BUILD(411,  423 , eVLAN                      ));
    snetLion3EPclKeyFieldBuildByValue(pclKeyPtr,    descrPtr->origSrcEPortOrTrnk,
        LION3_EPCL_KEY_60B_EXTRA_10B_FIELD_NAME_BUILD(424,  436 , Source_ePort_or_Trunk_ID   ));
    snetLion3EPclKeyFieldBuildByValue(pclKeyPtr,    descrPtr->trgEPort,
        LION3_EPCL_KEY_60B_EXTRA_10B_FIELD_NAME_BUILD(437,  449 , Trg_ePort                  ));
    snetLion3EPclKeyFieldBuildByValue(pclKeyPtr,    descrPtr->srcDev,
        LION3_EPCL_KEY_60B_EXTRA_10B_FIELD_NAME_BUILD(450,  459 , Src_Dev                    ));
    snetLion3EPclKeyFieldBuildByValue(pclKeyPtr,    descrPtr->trgDev,
        LION3_EPCL_KEY_60B_EXTRA_10B_FIELD_NAME_BUILD(460,  469 , Trg_Dev                    ));
    snetLion3EPclKeyFieldBuildByValue(pclKeyPtr,    egressPort,
        LION3_EPCL_KEY_60B_EXTRA_10B_FIELD_NAME_BUILD(470,  477 , Local_Dev_Trg_Phy_Port     ));
    snetLion3EPclKeyFieldBuildByValue(pclKeyPtr,    descrPtr->origIsTrunk,
        LION3_EPCL_KEY_60B_EXTRA_10B_FIELD_NAME_BUILD(470,  470 , Orig_Src_Is_Trunk          ));
    snetLion3EPclKeyFieldBuildByValue(pclKeyPtr,    0,
        LION3_EPCL_KEY_60B_EXTRA_10B_FIELD_NAME_BUILD(478,  479 , Reserved                   ));
}

/*******************************************************************************
*   lion3EpclKeyUdbBuildKey
*
* DESCRIPTION:
*        Lion3 EPCL - built UDB only keys
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - pointer to frame data buffer Id.
*       curLookUpCfgPtr - pointer to the current PCL configuration entry.
*       pclExtraDataPtr - pointer to extra data needed for the IPCL engine.
*       egressPort      - the egress port
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
static GT_VOID lion3EpclKeyUdbBuildKey
(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC  * curLookUpCfgPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC    * pclExtraDataPtr,
    IN GT_U32                             egressPort,
    OUT SNET_CHT_POLICY_KEY_STC         * pclKeyPtr
)
{
    DECLARE_FUNC_NAME(lion3EpclKeyUdbBuildKey);

    GT_STATUS   rc;
    GT_U32  numOfUdbs = 0;
    GT_U32  udbIndexIterator;/*udb index iterator 0..(numOfUdbs-1)*/
    GT_U32  udbIndexIterator_start;/*first udb index iterator */
    GT_U32  newUdbIndex; /*new udb index 0..49*/
    GT_U32  udbValid = 1;
    GT_U32  entryIndex_PCL_UDB_SELECT_TBL;/* index in ipcl udb select table */
    GT_U32  *memPtr;/*pointer to memory */
    GT_U8   udb49Value;
    GT_U8   *currKeyBytesPtr;
    GT_BIT  udbKeyBitmapEnable;

    switch(pclKeyPtr->pclKeyFormat)
    {
        case CHT_PCL_KEY_10B_E:
        case CHT_PCL_KEY_20B_E:
        case CHT_PCL_KEY_30B_E:
        case CHT_PCL_KEY_40B_E:
        case CHT_PCL_KEY_50B_E:
            numOfUdbs = ((pclKeyPtr->pclKeyFormat - CHT_PCL_KEY_10B_E) + 1) * 10;
            break;
        case CHT_PCL_KEY_60B_E:
            numOfUdbs = 50;
            break;
        default:
            skernelFatalError("pclKeyFormat: not valid [%d]",pclKeyPtr->pclKeyFormat);
            break;
    }

    __LOG_PARAM(descrPtr->epcl_pcktType_sip5);

    /*index = (KeySize * 16) + PacketType*/
    entryIndex_PCL_UDB_SELECT_TBL =
        ((pclKeyPtr->pclKeyFormat - CHT_PCL_KEY_10B_E) * 16) + descrPtr->epcl_pcktType_sip5;

    __LOG_PARAM(entryIndex_PCL_UDB_SELECT_TBL);

    memPtr = smemMemGet(devObjPtr,
        SMEM_LION3_EPCL_UDB_SELECT_TBL_MEM(devObjPtr,entryIndex_PCL_UDB_SELECT_TBL));


    if((numOfUdbs == 50) && curLookUpCfgPtr->udbKeyBitmapEnable)
    {
        __LOG(("set to 0 the first 32 UDBs due to curLookUpCfgPtr->udbKeyBitmapEnable = 1 \n"));
        udbKeyBitmapEnable = 1;
    }
    else
    {
        udbKeyBitmapEnable = 0;
    }

    currKeyBytesPtr = &pclKeyPtr->key.triple[0];

    if((udbKeyBitmapEnable == 0) &&
       snetFieldValueGet(memPtr,300,1))
    {
        __LOG(("Replace UDBs 0 and 1 by PCL_ID [0x%x] \n",
            curLookUpCfgPtr->pclID));

        *currKeyBytesPtr = (GT_U8)(curLookUpCfgPtr->pclID);
        __LOG(("udbIndexIterator[%d] got value[0x%x] \n",
            0,
            *currKeyBytesPtr));
        currKeyBytesPtr++;

        *currKeyBytesPtr = (GT_U8)(curLookUpCfgPtr->pclID >> 8);
        __LOG(("udbIndexIterator[%d] got value[0x%x] \n",
            1,
            *currKeyBytesPtr));

        currKeyBytesPtr++;

        /* jump the first 2 UDBs */
        udbIndexIterator_start = 2;
    }
    else
    {
        udbIndexIterator_start = 0;
    }

    /* convert each udb index 0..(numOfUdbs-1) to the key into new udb index 0..49 */
    for(udbIndexIterator = udbIndexIterator_start ; udbIndexIterator < numOfUdbs ; udbIndexIterator++,currKeyBytesPtr++)
    {
        if(udbIndexIterator != udbIndexIterator_start)
        {
            /* log the previous UDB info */
            __LOG(("udbIndexIterator[%d] got value[0x%x] \n",
                (udbIndexIterator-1),
                *(currKeyBytesPtr-1)));
        }

        if(udbKeyBitmapEnable && (udbIndexIterator < 32))
        {
            /*set to 0 the first 32 UDBs due to curLookUpCfgPtr->udbKeyBitmapEnable = 1*/

            /* the UDB is set only according to value of UDB 49 */
            /* we will set it at the end of the loop */
            *currKeyBytesPtr = 0;
            continue;
        }

        newUdbIndex = snetFieldValueGet(memPtr,6*udbIndexIterator,6);

        __LOG(("udbIndexIterator[%d] mapped to newUdbIndex[%d] \n",
            udbIndexIterator,
            newUdbIndex));

        if(newUdbIndex >= 50)
        {
            __LOG((" newUdbIndex >= 50 , so consider as 'UDB not valid' \n"));
            udbValid = 0;
            *currKeyBytesPtr = 0;

            continue;
        }

        rc = snetLionPclUdbKeyValueGet(devObjPtr,descrPtr,SMAIN_DIRECTION_EGRESS_E,
                newUdbIndex,currKeyBytesPtr);
        if(rc == GT_FAIL)
        {
            __LOG((" failed to get UDB so consider as 'UDB not valid' \n"));
            udbValid = 0;
            *currKeyBytesPtr = 0;
        }
    }

    /* log the last byte */
    __LOG(("udbIndexIterator[%d] got value[0x%x] \n",
        (udbIndexIterator-1),
        *(currKeyBytesPtr-1)));

    __LOG_PARAM(udbValid);

    if(udbKeyBitmapEnable)
    {
        /* set the needed bit according to value of UDB 49 */
        newUdbIndex = 49;
        rc = snetLionPclUdbKeyValueGet(devObjPtr,descrPtr,SMAIN_DIRECTION_EGRESS_E,
            newUdbIndex,&udb49Value);
        if(rc == GT_OK)
        {
            __LOG(("set to 1 single bit[%d] (bit index is the value of UDB 49) \n" ,
                udb49Value));
            snetFieldValueSet(pclKeyPtr->key.data , udb49Value , 1, 1);
        }
    }
    else
    if(snetFieldValueGet(memPtr,300 + 1,1))
    {
        /*Enable the key bit 15 to be override by UDB Valid indication*/
        currKeyBytesPtr = &pclKeyPtr->key.triple[1];
        (*currKeyBytesPtr) &= 0x7F;/*clear bit 7 in byte 1*/
        (*currKeyBytesPtr) |= udbValid << 7;/*set bit 7 in byte 1 according to udbValid */
        __LOG(("Enable the key bit 15 to be override by UDB Valid indication [%d] \n" ,
            udbValid));
    }


    if(pclKeyPtr->pclKeyFormat == CHT_PCL_KEY_60B_E)
    {
        /* build the extra 10 bytes of the key (after the first 50 UDBs) */
        __LOG(("EPCL: build the extra 10 bytes of the key (after the first 50 UDBs)"));
        lion3EpclKey60UdbBuildKeyExtra10Bytes(devObjPtr, descrPtr, curLookUpCfgPtr,
                pclExtraDataPtr,pclKeyPtr , egressPort , udbValid);
    }

    return;
}

/*******************************************************************************
* lion3EPclCreateKey
*
* DESCRIPTION:
*        Create Egress PCL search tram key.
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - pointer to frame data buffer Id.
*       egressPort      - egress port (global physical port).
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
static GT_VOID lion3EPclCreateKey
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
    DECLARE_FUNC_NAME(lion3EPclCreateKey);

    GT_U32  keyIndex = keyType;
    CHT3_EPCL_KEY_TYPE_ENT egressPclkeyType = (CHT3_EPCL_KEY_TYPE_ENT)keyType;

    memset(&pclKeyPtr->key, 0, sizeof(pclKeyPtr->key));

    pclKeyPtr->devObjPtr = devObjPtr;
    pclKeyPtr->updateOnlyDiff = GT_FALSE;
    pclKeyPtr->pclKeyFormat = SNET_CHT3_EPCL_KEY_FORMAT(egressPclkeyType);

    if(LION3_EPCL_KEY_IS_UDB_MAC(devObjPtr,keyType))
    {
        pclKeyPtr->pclKeyFormat = lookupConfPtr->keySize;

        __LOG(("Start Build UDB Key for lookup \n"));

        /* UDB only keys 10..50, 50+10(60) */
        lion3EpclKeyUdbBuildKey(devObjPtr, descrPtr,
                            lookupConfPtr,
                            pclExtraDataPtr,
                            egressPort,
                            pclKeyPtr);

        __LOG(("End of Build UDB Key for lookup \n"));
        return;
    }

    if(keyIndex >= CHT3_EPCL_KEY_TYPE_LAST_E)
    {
        skernelFatalError("keyIndex: not valid [%d]",keyIndex);
        keyIndex = 0;
    }

    __LOG(("Build Key#[%d] [%s] \n",
        keyIndex,
        lion3EpclKeyInfoArr[keyIndex].nameString));
    /* build the key (without UDBs and <User Defined Valid>)*/
    lion3EpclKeyInfoArr[keyIndex].buildKeyFunc(devObjPtr, descrPtr, egressPort,
                        lookupConfPtr,
                        pclExtraDataPtr,
                        keyIndex,
                        pclKeyPtr);

    __LOG(("Add the relevant UDBs to the Key#[%d] [%s] \n",
        keyIndex,
        lion3EpclKeyInfoArr[keyIndex].nameString));
    /* build all the UDBs for this key and the <User Defined Valid> */
    lion3EPclBuildAllUdbsForKey(devObjPtr, descrPtr,
        pclKeyPtr,keyIndex);

    /* Port-List mode */
    if (lookupConfPtr->pclPortListModeEn &&
        egressPclkeyType != CHT3_EPCL_KEY_TYPE_L4_IPV4_NON_IP_KEY_LONG_E &&
        egressPclkeyType != CHT3_EPCL_KEY_TYPE_L4_IPV6_KEY_TRIPLE_E)
    {
        __LOG(("Port-List mode"));
        lion3EpclKeyPortsBmpBuild(devObjPtr, descrPtr, egressPort, pclKeyPtr , NULL);
    }
}

/*******************************************************************************
*   lion3EPclBuildKeyPacketTypeCommon
*
* DESCRIPTION:
*       Lion3 : Build first common EPCL search key. - related to packet type.
*
* INPUTS:
*       devObjPtr            - pointer to device object.
*       descrPtr             - pointer to frame data buffer Id.
*        egressPort     - local egress port (not global).
*       lookupConfPtr        - pointer to EPCL configuration entry.
*       pclExtraDataPtr      - pointer to extra data needed for the EPCL engine.
*       keyIndex             - key index (0..7) - used for SIP5
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
static GT_VOID lion3EPclBuildKeyPacketTypeCommon
(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr,
    IN  GT_U32                            egressPort,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC  * lookupConfPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC    * pclExtraDataPtr,
    IN DXCH_EPCL_KEY_ENT                  keyIndex,
    OUT SNET_CHT_POLICY_KEY_STC         * pclKeyPtr
)
{
    GT_U32  tmpVal;
    GT_U32  packetTypeIndex;

    packetTypeIndex = keyIndex <= DXCH_EPCL_KEY_2_E ? 0 :
                      keyIndex <= DXCH_EPCL_KEY_5_E ? 1 :
                      keyIndex <= DXCH_EPCL_KEY_7_E ? 2 :
                      0;/* ERROR */

    switch (descrPtr->outGoingMtagCmd)
    {
        case SKERNEL_MTAG_CMD_FORWARD_E:
            snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->qos.qosProfile,
                muxedTrafficTypeInfo[packetTypeIndex].Data_Pkt_QoS_Profile_6_0_OR_port_list_13_8);
            break;
        case SKERNEL_MTAG_CMD_TO_CPU_E:
            tmpVal = SMEM_U32_GET_FIELD(descrPtr->cpuCode,0,6);
            snetLion3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->cpuCode,
                muxedTrafficTypeInfo[packetTypeIndex].Ctrl_To_CPU_CPU_CODE_6_0);
            break;
        case SKERNEL_MTAG_CMD_FROM_CPU_E:
            if(SMAIN_NOT_VALID_CNS != muxedTrafficTypeInfo[packetTypeIndex].Ctrl_From_CPU_TC_2_0_DP_1_0)
            {
                /* 2'b0,TC[2:0],DP[1:0] */
                tmpVal = 0;
                tmpVal = SMEM_U32_GET_FIELD(descrPtr->qos.fromCpuQos.fromCpuDp ,0,2);
                tmpVal |= (SMEM_U32_GET_FIELD(descrPtr->qos.fromCpuQos.fromCpuTc ,0,3) << 2);
                snetLion3EPclKeyFieldBuildByValue(pclKeyPtr,tmpVal,
                    muxedTrafficTypeInfo[packetTypeIndex].Ctrl_From_CPU_TC_2_0_DP_1_0);
            }
            break;
        default :/* TO_ANALYZER */
            break;/*already set as 0*/
    }

    if(descrPtr->outGoingMtagCmd == SKERNEL_MTAG_CMD_FORWARD_E ||
       descrPtr->outGoingMtagCmd == SKERNEL_MTAG_CMD_TO_CPU_E)
    {
        if (descrPtr->origIsTrunk)
        {
            snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->origSrcEPortOrTrnk ,
                muxedTrafficTypeInfo[packetTypeIndex].Data_Pkt_OR_CtrlToCPU_Src_Trunk_ID_6_0);
        }
        else
        {
            if(descrPtr->outGoingMtagCmd == SKERNEL_MTAG_CMD_FORWARD_E)
            {
                /*{2'b0, SrcDev[4:0]}*/
                tmpVal =  (descrPtr->srcDev & 0x1F);
                snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, tmpVal ,
                    muxedTrafficTypeInfo[packetTypeIndex].Data_Pkt_Src_Dev_4_0);
            }
            else  /*SKERNEL_MTAG_CMD_TO_CPU_E*/
            {
                /*{SrcTrg,0,SrcDev[4:0]}*/
                tmpVal = ((descrPtr->srcTrg & 1) << 6) | (descrPtr->srcDev & 0x1F);
                snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, tmpVal ,
                    muxedTrafficTypeInfo[packetTypeIndex].Ctrl_To_CPU_Src_Trg_Src_Dev_4_0);
            }

        }

        if(SMAIN_NOT_VALID_CNS != muxedTrafficTypeInfo[packetTypeIndex].Data_Pkt_OR_Ctrl_To_CPU_Src_Is_Trunk)
        {
            snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->origIsTrunk ,
                muxedTrafficTypeInfo[packetTypeIndex].Data_Pkt_OR_Ctrl_To_CPU_Src_Is_Trunk);
        }

        if(descrPtr->outGoingMtagCmd == SKERNEL_MTAG_CMD_FORWARD_E)
        {
            if(SMAIN_NOT_VALID_CNS != muxedTrafficTypeInfo[packetTypeIndex].Data_Pkt_Is_Unknown)
            {
                snetLion3EPclKeyFieldBuildByValue(pclKeyPtr,(descrPtr->egressFilterRegistered ? 0 : 1)/*IsUnknown*/,
                    muxedTrafficTypeInfo[packetTypeIndex].Data_Pkt_Is_Unknown);
            }
        }
        else /*SKERNEL_MTAG_CMD_TO_CPU_E*/
        {
            if(SMAIN_NOT_VALID_CNS != muxedTrafficTypeInfo[packetTypeIndex].Ctrl_To_CPU_CPU_CODE_7)
            {
                snetLion3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->cpuCode >> 7,
                    muxedTrafficTypeInfo[packetTypeIndex].Ctrl_To_CPU_CPU_CODE_7);
            }
        }
    }
    else
    if(descrPtr->outGoingMtagCmd == SKERNEL_MTAG_CMD_FROM_CPU_E)
    {
        /*{1'b0,EgressFilterEn, SrcDev[4:0]}*/
        tmpVal = 1 << 6 | (descrPtr->egressFilterEn << 5) | (descrPtr->srcDev & 0x1F);
        snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, tmpVal ,
            muxedTrafficTypeInfo[packetTypeIndex].From_CPU_Egress_Filter_En_Src_Dev_4_0);

        if(SMAIN_NOT_VALID_CNS != muxedTrafficTypeInfo[packetTypeIndex].From_CPU_Is_Unknown)
        {
            snetLion3EPclKeyFieldBuildByValue(pclKeyPtr,(descrPtr->egressFilterRegistered ? 0 : 1)/*IsUnknown*/,
                muxedTrafficTypeInfo[packetTypeIndex].From_CPU_Is_Unknown);
        }
    }
    else /*SKERNEL_MTAG_CMD_TO_TRG_SNIFFER_E*/
    {
        /*{RxSniff,1'b0,SrcDev[4:0]}*/
        tmpVal = (descrPtr->rxSniff << 6) | (descrPtr->srcDev & 0x1F);
        snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, tmpVal ,
            muxedTrafficTypeInfo[packetTypeIndex].To_Analyzer_Rx_Sniff_Src_Dev_4_0);
    }
}
/*******************************************************************************
*   lion3EPclBuildKeyCommon
*
* DESCRIPTION:
*       Lion3 : Build common EPCL search key for keys 0..5 (not for 6,7)
*
* INPUTS:
*       devObjPtr            - pointer to device object.
*       descrPtr             - pointer to frame data buffer Id.
*        egressPort     - local egress port (not global).
*       lookupConfPtr        - pointer to EPCL configuration entry.
*       pclExtraDataPtr      - pointer to extra data needed for the EPCL engine.
*       keyIndex             - key index (0..7) - used for SIP5
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
static GT_VOID lion3EPclBuildKeyCommon
(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr,
    IN  GT_U32                            egressPort,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC  * lookupConfPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC    * pclExtraDataPtr,
    IN DXCH_EPCL_KEY_ENT                  keyIndex,
    OUT SNET_CHT_POLICY_KEY_STC         * pclKeyPtr
)
{
    GT_U32  tmpVal;
    GT_U32  srcPort,trgPort;
    GT_STATUS   rc;
    GT_U8   byteValue;
    GT_BIT  tag_0_exist,tag_1_exist,tag0_is_outer_tag;
    GT_U32  tag0,tag1;
    GT_U32  epclKeyVid = descrPtr->haToEpclInfo.epclKeyVid;/* NOTE: impact only 12 lower bits ! */

    if(keyIndex >= DXCH_EPCL_KEY_6_E)
    {
        /* not valid for those keys */
        return;
    }

    snetLion3Epcl_vlanTagInfoGet(devObjPtr, descrPtr,
        &tag_0_exist, &tag_1_exist, &tag0_is_outer_tag,
        &tag0,&tag1);

    snetLion3Epcl_srcPortTrgPortGet(devObjPtr, descrPtr,egressPort,
                                    &srcPort, &trgPort);

    snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, lookupConfPtr->pclID ,
        LION3_EPCL_KEY_FIELD_NAME_BUILD(0  ,  7 ,PCL_ID_7_0_OR_port_list_7_0                ));
    snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, lookupConfPtr->pclID >> 8,
        LION3_EPCL_KEY_FIELD_NAME_BUILD( 8 ,  9 ,PCL_ID_9_8                                 ));
    snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, srcPort ,
        LION3_EPCL_KEY_FIELD_NAME_BUILD(10 , 15 ,Src_Port_5_0                               ));
    snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, tag_0_exist ,
        LION3_EPCL_KEY_FIELD_NAME_BUILD(16 , 16 ,Is_Tagged                                  ));
    snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, epclKeyVid ,
        LION3_EPCL_KEY_FIELD_NAME_BUILD(17 , 28 ,VID0_eVLAN_11_0                            ));
    snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->up  ,
        LION3_EPCL_KEY_FIELD_NAME_BUILD(29 , 31 ,UP0                                        ));

    lion3EPclBuildKeyPacketTypeCommon(devObjPtr, descrPtr, egressPort, lookupConfPtr,
                                   pclExtraDataPtr, keyIndex, pclKeyPtr);

    snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->isIp,
        LION3_EPCL_KEY_FIELD_NAME_BUILD(40 , 40 ,Is_IP                                      ));

    snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, tag_1_exist ,
        LION3_EPCL_KEY_FIELD_NAME_BUILD(191,191 ,Tag_1_Exist                                ));

    if(keyIndex <= DXCH_EPCL_KEY_2_E) /*short keys*/
    {
        snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, (descrPtr->isIp && descrPtr->isIPv4) ||descrPtr->isFcoe ,
            LION3_EPCL_KEY_FIELD_NAME_BUILD(39 , 39 ,Is_IPv4                                    ));

        snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->arp,
            LION3_EPCL_KEY_FIELD_NAME_BUILD(72 , 72 ,Is_ARP                                     ));

        switch (descrPtr->outGoingMtagCmd)
        {
            case SKERNEL_MTAG_CMD_FORWARD_E:
                snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->qos.qosProfile,
                    LION3_EPCL_KEY_FIELD_NAME_BUILD(32 , 38 ,Data_Pkt_QoS_Profile_6_0_OR_port_list_13_8 ));
                break;
            case SKERNEL_MTAG_CMD_TO_CPU_E:
                tmpVal = SMEM_U32_GET_FIELD(descrPtr->cpuCode,0,6);
                snetLion3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->cpuCode,
                    LION3_EPCL_KEY_FIELD_NAME_BUILD(32 , 38 ,Ctrl_To_CPU_CPU_CODE_6_0                   ));
                break;
            case SKERNEL_MTAG_CMD_FROM_CPU_E:
                /* 2'b0,TC[2:0],DP[1:0] */
                tmpVal = 0;
                tmpVal = SMEM_U32_GET_FIELD(descrPtr->qos.fromCpuQos.fromCpuDp ,0,2);
                tmpVal |= (SMEM_U32_GET_FIELD(descrPtr->qos.fromCpuQos.fromCpuTc ,0,3) << 2);
                snetLion3EPclKeyFieldBuildByValue(pclKeyPtr,tmpVal,
                    LION3_EPCL_KEY_FIELD_NAME_BUILD(32 , 38 ,Ctrl_From_CPU_TC_2_0_DP_1_0                ));
                break;
            default :/* TO_ANALYZER */
                break;/*already set as 0*/
        }

        snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sstId ,
            LION3_EPCL_KEY_FIELD_NAME_BUILD(82 , 86 ,Source_ID_4_0                              ));

        snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->routed ,
            LION3_EPCL_KEY_FIELD_NAME_BUILD(87 , 87 ,Is_Routed                                  ));

        snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL2Valid ,
            LION3_EPCL_KEY_FIELD_NAME_BUILD(89 , 89 ,Is_L2_Valid                                ));


        SNET_CHT3_EPCL_KEY_EGT_PKT_TYPE(descrPtr, tmpVal);
        snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, tmpVal,
            LION3_EPCL_KEY_FIELD_NAME_BUILD(187,188 ,Packet_Type_1_0                            ));

        snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->useVidx ,
            LION3_EPCL_KEY_FIELD_NAME_BUILD(190,190 ,is_VIDX                                    ));

        if(keyIndex >= DXCH_EPCL_KEY_1_E)/*key 1,2*/
        {
            rc = snetCht2EPclTcpUdpPortRangeCompareGet(devObjPtr,descrPtr,&byteValue);
            if(rc != GT_OK)
            {
                byteValue = 0;
            }
            snetLion3EPclKeyFieldBuildByValue(pclKeyPtr,byteValue,
                LION3_EPCL_KEY1_FIELD_NAME_BUILD(90 , 97 ,TCP_UDP_Port_Comparators ));
        }


    }
    else /*keys 3..5 - long keys */
    {
        snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, (descrPtr->isIp && descrPtr->isIPv4 == 0),
            LION3_EPCL_KEY_FIELD_NAME_BUILD(39 , 39 ,Is_IPv6                                    ));

        if (descrPtr->l4StartOffsetPtr != NULL)
        {
            snetLion3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->l4StartOffsetPtr[13],
                LION3_EPCL_KEY_FIELD_NAME_BUILD(72 , 79 ,L4_Header_Byte_13                          ));
            snetLion3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->l4StartOffsetPtr[1],
                LION3_EPCL_KEY_FIELD_NAME_BUILD(80 , 87 ,L4_Header_Byte_1                           ));
            snetLion3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->l4StartOffsetPtr[0],
                LION3_EPCL_KEY_FIELD_NAME_BUILD(88 , 95 ,L4_Header_Byte_0                           ));
        }

        if((descrPtr->isIp && descrPtr->isIPv4 == 0))/*ipv6*/
        {
            snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[3],
                LION3_EPCL_KEY_FIELD_NAME_BUILD(96 ,127 ,SIP_31_0                                   ));
        }
        else
        {
            snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[0],
                LION3_EPCL_KEY_FIELD_NAME_BUILD(96 ,127 ,SIP_31_0                                   ));
        }

        snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sstId ,
            LION3_EPCL_KEY3_FIELD_NAME_BUILD(379,383 ,Source_ID_4_0                              ));

        snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->routed ,
            LION3_EPCL_KEY3_FIELD_NAME_BUILD(384,384 ,Is_Routed                                  ));

        snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL2Valid ,
            LION3_EPCL_KEY3_FIELD_NAME_BUILD(386,386 ,Is_L2_Valid                                ));

        rc = snetCht2EPclTcpUdpPortRangeCompareGet(devObjPtr,descrPtr,&byteValue);
        if(rc != GT_OK)
        {
            byteValue = 0;
        }
        snetLion3EPclKeyFieldBuildByValue(pclKeyPtr,byteValue,
            LION3_EPCL_KEY3_FIELD_NAME_BUILD(387,394 ,TCP_UDP_Port_Comparators                   ));

        snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, srcPort >> 6 ,
            LION3_EPCL_KEY3_FIELD_NAME_BUILD(395,396 ,Src_Port_7_6                               ));


        SNET_CHT3_EPCL_KEY_EGT_PKT_TYPE(descrPtr, tmpVal);
        snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, tmpVal,
            LION3_EPCL_KEY3_FIELD_NAME_BUILD(397,398 ,Packet_Type_1_0                            ));

        snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->useVidx ,
            LION3_EPCL_KEY3_FIELD_NAME_BUILD(399,399 ,is_VIDX                                    ));


        snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->srcDev >> 5 ,
            LION3_EPCL_KEY3_FIELD_NAME_BUILD(400,401 ,Source_Dev_6_5                             ));
        if(descrPtr->origIsTrunk)
        {
            snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->origSrcEPortOrTrnk >> 7 ,
                LION3_EPCL_KEY3_FIELD_NAME_BUILD(402,406 ,Source_Dev_11_7_OR_Trunk_ID_11_7           ));
        }
        else
        {
            snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->srcDev >> 7 ,
                LION3_EPCL_KEY3_FIELD_NAME_BUILD(402,406 ,Source_Dev_11_7_OR_Trunk_ID_11_7           ));
        }

        snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sstId >> 5 ,
            LION3_EPCL_KEY3_FIELD_NAME_BUILD(407,413 ,Source_ID_11_5                             ));

        if(keyIndex >= DXCH_EPCL_KEY_4_E)/*key 4,5*/
        {
            snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[2],
                LION3_EPCL_KEY4_FIELD_NAME_BUILD(128,159 ,SIP_63_32                ));
            snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[1],
                LION3_EPCL_KEY4_FIELD_NAME_BUILD(160,190 ,SIP_94_64                ));
            snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[1] >> 31,
                LION3_EPCL_KEY4_FIELD_NAME_BUILD(208,208 ,SIP_95_95                ));
            snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[0],
                LION3_EPCL_KEY4_FIELD_NAME_BUILD(209,240 ,SIP_127_96               ));

            snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpV6EhExists,
                LION3_EPCL_KEY4_FIELD_NAME_BUILD(241,241 ,Ipv6_EH_exist            ));
            snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpV6EhHopByHop,
                LION3_EPCL_KEY4_FIELD_NAME_BUILD(242,242 ,Is_IPv6_EH_Hop_By_Hop    ));

            snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, (descrPtr->dip[0] >> 24),
                LION3_EPCL_KEY4_FIELD_NAME_BUILD(243,250 ,DIP_127_120              ));
        }
    }

    if(keyIndex >= DXCH_EPCL_KEY_1_E)/*not key 0*/
    {
        snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->ipProt,
            LION3_EPCL_KEY_FIELD_NAME_BUILD(41 , 48 ,Ip_Protocol                                ));
        snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dscp,
            LION3_EPCL_KEY_FIELD_NAME_BUILD(49 , 54 ,Packet_Dscp                                ));
        snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL4Valid,
            LION3_EPCL_KEY_FIELD_NAME_BUILD(55 , 55 ,Is_L4_Valid                                ));

        if (descrPtr->l4StartOffsetPtr != NULL)
        {
            if (descrPtr->ipv6Icmp || descrPtr->ipv4Icmp)
            {
                snetLion3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->l4StartOffsetPtr[4],
                    LION3_EPCL_KEY_FIELD_NAME_BUILD(64 , 71 ,L4_Header_Byte_2_OR_4                      ));

                snetLion3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->l4StartOffsetPtr[5],
                    LION3_EPCL_KEY_FIELD_NAME_BUILD(56 , 63 ,L4_Header_Byte_3_OR_5                      ));
            }
            else
            {
                snetLion3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->l4StartOffsetPtr[2],
                    LION3_EPCL_KEY_FIELD_NAME_BUILD(64 , 71 ,L4_Header_Byte_2_OR_4                      ));

                snetLion3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->l4StartOffsetPtr[3],
                    LION3_EPCL_KEY_FIELD_NAME_BUILD(56 , 63 ,L4_Header_Byte_3_OR_5                      ));
            }
        }
    }




}

/*******************************************************************************
*   lion3EPclBuildKey0
*
* DESCRIPTION:
*        Build layer2 standard epcl search key. - key0
*
* INPUTS:
*       devObjPtr            - pointer to device object.
*       descrPtr             - pointer to frame data buffer Id.
*        egressPort     - local egress port (not global).
*       lookupConfPtr        - pointer to EPCL configuration entry.
*       pclExtraDataPtr      - pointer to extra data needed for the EPCL engine.
*       keyIndex             - key index (0..7) - used for SIP5
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
static GT_VOID lion3EPclBuildKey0
(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr,
    IN  GT_U32                            egressPort,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC  * lookupConfPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC    * pclExtraDataPtr,
    IN DXCH_EPCL_KEY_ENT                  keyIndex,
    OUT SNET_CHT_POLICY_KEY_STC         * pclKeyPtr
)
{
    GT_U32  tmpVal;

    lion3EPclBuildKeyCommon(devObjPtr, descrPtr, egressPort, lookupConfPtr,
                                   pclExtraDataPtr, keyIndex, pclKeyPtr);

    /* extra fields beside 'common' */
    snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->up1 >> 2,
        LION3_EPCL_KEY0_FIELD_NAME_BUILD(41 , 41 ,UP1_2                    ));
    snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->etherTypeOrSsapDsap,
        LION3_EPCL_KEY0_FIELD_NAME_BUILD(42 , 57 ,Ether_Type_or_Dsap_Ssap  ));
    snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->vid1,
        LION3_EPCL_KEY0_FIELD_NAME_BUILD(58 , 69 ,Tag1_VID                 ));
    snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->up1,
        LION3_EPCL_KEY0_FIELD_NAME_BUILD(70 , 71 ,UP1_1_0                  ));

    tmpVal = (descrPtr->l2Encaps == SKERNEL_ETHERNET_II_E ||
              descrPtr->l2Encaps == SKERNEL_LLC_SNAP_E) ? 1 : 0;
    snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, tmpVal,
        LION3_EPCL_KEY0_FIELD_NAME_BUILD(90 , 90 ,L2_Encap_Type            ));

    /* the EPCL need to use the actual mac addresses of egress and not those of ingress */
    /* For non-tunnel terminated packets:
            Transmitted packet MACs after packet modification (for example, due to routing or Tunnel Termination)
        For Ethernet-Over-xxx Tunnel-Start packets:
            MACs of the passenger packet
    */
    if(descrPtr->haToEpclInfo.macDaSaPtr)
    {
        snetLion3EPclKeyFieldBuildByPointer(pclKeyPtr, &descrPtr->haToEpclInfo.macDaSaPtr[6]/* mac SA */,
            LION3_EPCL_KEY0_FIELD_NAME_BUILD(91 ,138 ,MAC_SA                   ));
        snetLion3EPclKeyFieldBuildByPointer(pclKeyPtr, descrPtr->haToEpclInfo.macDaSaPtr,
            LION3_EPCL_KEY0_FIELD_NAME_BUILD(139,186 ,MAC_DA                   ));
    }

    snetLion3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->cfidei1,
        LION3_EPCL_KEY0_FIELD_NAME_BUILD(189,189 ,Tag_1_CFI                ));




}

/*******************************************************************************
*   lion3EPclBuildKey1
*
* DESCRIPTION:
*        Build Layer2+IPv4\6+QoS standard epcl search key. - key1
*
* INPUTS:
*       devObjPtr            - pointer to device object.
*       descrPtr             - pointer to frame data buffer Id.
*        egressPort     - local egress port (not global).
*       currentLookupConfPtr - pointer to the current PCL configuration entry.
*       pclExtraDataPtr      - pointer to extra data needed for the ePCL engine.
*       keyIndex             - key index (0..7) - used for SIP5
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
static GT_VOID lion3EPclBuildKey1
(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr,
    IN  GT_U32                            egressPort,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC  * lookupConfPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC    * pclExtraDataPtr,
    IN DXCH_EPCL_KEY_ENT                  keyIndex,
    OUT SNET_CHT_POLICY_KEY_STC         * pclKeyPtr
)
{

    lion3EPclBuildKeyCommon(devObjPtr, descrPtr, egressPort, lookupConfPtr,
                                   pclExtraDataPtr, keyIndex, pclKeyPtr);

    /* extra fields beside 'common' */
    snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[0],
        LION3_EPCL_KEY1_FIELD_NAME_BUILD( 98,129 ,DIP_31_0                 ));
    if (descrPtr->l4StartOffsetPtr != NULL)
    {
        snetLion3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->l4StartOffsetPtr[13],
            LION3_EPCL_KEY1_FIELD_NAME_BUILD(130,137 ,L4_Header_Byte_13        ));
    }

    /* the EPCL need to use the actual mac addresses of egress and not those of ingress */
    /* For non-tunnel terminated packets:
            Transmitted packet MACs after packet modification (for example, due to routing or Tunnel Termination)
        For Ethernet-Over-xxx Tunnel-Start packets:
            MACs of the passenger packet
    */
    if(descrPtr->haToEpclInfo.macDaSaPtr)
    {
        snetLion3EPclKeyFieldBuildByPointer(pclKeyPtr, descrPtr->haToEpclInfo.macDaSaPtr,
            LION3_EPCL_KEY0_FIELD_NAME_BUILD(139,186 ,MAC_DA                   ));
    }

    snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpv4Fragment,
        LION3_EPCL_KEY1_FIELD_NAME_BUILD(189,189 ,IPv4_fragmented          ));


}
/*******************************************************************************
*   lion3EPclBuildKey2
*
* DESCRIPTION:
*        Build IPv4+Layer4 standard epcl search key. - key2
*
* INPUTS:
*       devObjPtr            - pointer to device object.
*       descrPtr             - pointer to frame data buffer Id.
*        egressPort     - local egress port (not global).
*       currentLookupConfPtr - pointer to the current PCL configuration entry.
*       pclExtraDataPtr      - pointer to extra data needed for the ePCL engine.
*       keyIndex             - key index (0..7) - used for SIP5
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
static GT_VOID lion3EPclBuildKey2
(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr,
    IN  GT_U32                            egressPort,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC  * lookupConfPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC    * pclExtraDataPtr,
    IN DXCH_EPCL_KEY_ENT                  keyIndex,
    OUT SNET_CHT_POLICY_KEY_STC         * pclKeyPtr
)
{
    GT_U32  tmpVal;

    lion3EPclBuildKeyCommon(devObjPtr, descrPtr, egressPort, lookupConfPtr,
                                   pclExtraDataPtr, keyIndex, pclKeyPtr);

    /* extra fields beside 'common' */

    tmpVal = (descrPtr->macDaType == SKERNEL_BROADCAST_MAC_E ||
              descrPtr->macDaType == SKERNEL_BROADCAST_ARP_E) ? 1 : 0 ;
    snetLion3EPclKeyFieldBuildByValue(pclKeyPtr,  tmpVal,
        LION3_EPCL_KEY2_FIELD_NAME_BUILD(73 , 73 ,Is_BC                    ));

    /* extra fields beside 'common' */
    snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[0],
        LION3_EPCL_KEY2_FIELD_NAME_BUILD( 98,129 ,SIP_31_0                 ));
    snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[0],
        LION3_EPCL_KEY2_FIELD_NAME_BUILD(130,161 ,DIP_31_0                 ));

    if (descrPtr->l4StartOffsetPtr != NULL)
    {
        snetLion3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->l4StartOffsetPtr[13],
            LION3_EPCL_KEY2_FIELD_NAME_BUILD(162,169 ,L4_Header_Byte_13        ));
        snetLion3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->l4StartOffsetPtr[1],
            LION3_EPCL_KEY2_FIELD_NAME_BUILD(170,177 ,L4_Header_Byte_1         ));
        snetLion3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->l4StartOffsetPtr[0],
            LION3_EPCL_KEY2_FIELD_NAME_BUILD(178,185 ,L4_Header_Byte_0         ));
    }

    snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpv4Fragment,
        LION3_EPCL_KEY1_FIELD_NAME_BUILD(189,189 ,IPv4_fragmented          ));


}


/*******************************************************************************
*   lion3EPclBuildKey3
*
* DESCRIPTION:
*        Build layer2+IPv4+layer4 extended epcl search tcam key. - key3
*
* INPUTS:
*       devObjPtr            - pointer to device object.
*       descrPtr             - pointer to frame data buffer Id.
*        egressPort     - local egress port (not global).
*       currentLookupConfPtr - pointer to the current PCL configuration entry.
*       pclExtraDataPtr      - pointer to extra data needed for the ePCL engine.
*       keyIndex             - key index (0..7) - used for SIP5
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
static GT_VOID lion3EPclBuildKey3
(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr,
    IN  GT_U32                            egressPort,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC  * lookupConfPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC    * pclExtraDataPtr,
    IN DXCH_EPCL_KEY_ENT                  keyIndex,
    OUT SNET_CHT_POLICY_KEY_STC         * pclKeyPtr
)
{
    GT_U32  tmpVal;

    lion3EPclBuildKeyCommon(devObjPtr, descrPtr, egressPort, lookupConfPtr,
                                   pclExtraDataPtr, keyIndex, pclKeyPtr);

    /* extra fields beside 'common' */
    snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[0],
        LION3_EPCL_KEY3_FIELD_NAME_BUILD(128,159 ,DIP_31_0                 ));

    tmpVal = (descrPtr->l2Encaps == SKERNEL_ETHERNET_II_E ||
              descrPtr->l2Encaps == SKERNEL_LLC_SNAP_E) ? 1 : 0;
    snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, tmpVal,
        LION3_EPCL_KEY3_FIELD_NAME_BUILD(160,160 ,L2_Encap_Type            ));

    snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->etherTypeOrSsapDsap,
        LION3_EPCL_KEY3_FIELD_NAME_BUILD(161,176 ,Ether_Type_OR_Dsap_Ssap  ));

    snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpv4Fragment,
        LION3_EPCL_KEY3_FIELD_NAME_BUILD(177,177 ,IPv4_fragmented          ));

    snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->vid1 ,
        LION3_EPCL_KEY3_FIELD_NAME_BUILD(208,219 ,Tag1_VID                 ));

    snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->cfidei1 ,
        LION3_EPCL_KEY3_FIELD_NAME_BUILD(220,220 ,Tag1_CFI                 ));

    snetLion3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->up1,
        LION3_EPCL_KEY3_FIELD_NAME_BUILD(221,223 ,UP1                      ));


    /* the EPCL need to use the actual mac addresses of egress and not those of ingress */
    /* For non-tunnel terminated packets:
            Transmitted packet MACs after packet modification (for example, due to routing or Tunnel Termination)
        For Ethernet-Over-xxx Tunnel-Start packets:
            MACs of the passenger packet
    */
    if(descrPtr->haToEpclInfo.macDaSaPtr)
    {
        snetLion3EPclKeyFieldBuildByPointer(pclKeyPtr, &descrPtr->haToEpclInfo.macDaSaPtr[6]/* mac SA */,
            LION3_EPCL_KEY3_FIELD_NAME_BUILD(251,298 ,MAC_SA                   ));
        snetLion3EPclKeyFieldBuildByPointer(pclKeyPtr, descrPtr->haToEpclInfo.macDaSaPtr,
            LION3_EPCL_KEY3_FIELD_NAME_BUILD(299,346 ,MAC_DA                   ));
    }

    if(descrPtr->mpls)
    {
        snetLion3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->numOfLabels,
            LION3_EPCL_KEY3_FIELD_NAME_BUILD(224,225 ,Number_of_MPLS_Labels    ));

        snetLion3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->protOverMpls,
            LION3_EPCL_KEY3_FIELD_NAME_BUILD(226,227 ,Protocol_After_MPLS      ));

        snetLion3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->label2,
            LION3_EPCL_KEY3_FIELD_NAME_BUILD(228,247 ,MPLS_Label_1             ));

        snetLion3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->exp2,
            LION3_EPCL_KEY3_FIELD_NAME_BUILD(248,250 ,MPLS_EXP_1               ));

        snetLion3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->mpls,
            LION3_EPCL_KEY3_FIELD_NAME_BUILD(347,347 ,Is_MPLS                  ));

        snetLion3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->label1,
            LION3_EPCL_KEY3_FIELD_NAME_BUILD(348,367 ,MPLS_Label_0             ));

        snetLion3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->exp1,
            LION3_EPCL_KEY3_FIELD_NAME_BUILD(368,370 ,MPLS_EXP_0               ));
    }

}


/*******************************************************************************
*   lion3EPclBuildKey4
*
* DESCRIPTION:
*        Build Layer2+IPv6 extended ePCL search key. - key4
*
* INPUTS:
*       devObjPtr            - pointer to device object.
*       descrPtr             - pointer to frame data buffer Id.
*        egressPort     - local egress port (not global).
*       currentLookupConfPtr - pointer to the current PCL configuration entry.
*       pclExtraDataPtr      - pointer to extra data needed for the ePCL engine.
*       keyIndex             - key index (0..7) - used for SIP5
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
static GT_VOID lion3EPclBuildKey4
(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr,
    IN  GT_U32                            egressPort,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC  * lookupConfPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC    * pclExtraDataPtr,
    IN DXCH_EPCL_KEY_ENT                  keyIndex,
    OUT SNET_CHT_POLICY_KEY_STC         * pclKeyPtr
)
{
    lion3EPclBuildKeyCommon(devObjPtr, descrPtr, egressPort, lookupConfPtr,
                                   pclExtraDataPtr, keyIndex, pclKeyPtr);

    /* extra fields beside 'common' */

    /* the EPCL need to use the actual mac addresses of egress and not those of ingress */
    /* For non-tunnel terminated packets:
            Transmitted packet MACs after packet modification (for example, due to routing or Tunnel Termination)
        For Ethernet-Over-xxx Tunnel-Start packets:
            MACs of the passenger packet
    */
    if(descrPtr->haToEpclInfo.macDaSaPtr)
    {
        snetLion3EPclKeyFieldBuildByPointer(pclKeyPtr, &descrPtr->haToEpclInfo.macDaSaPtr[6]/* mac SA */,
            LION3_EPCL_KEY3_FIELD_NAME_BUILD(251,298 ,MAC_SA                   ));
        snetLion3EPclKeyFieldBuildByPointer(pclKeyPtr, descrPtr->haToEpclInfo.macDaSaPtr,
            LION3_EPCL_KEY3_FIELD_NAME_BUILD(299,346 ,MAC_DA                   ));
    }


    snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->vid1 ,
        LION3_EPCL_KEY4_FIELD_NAME_BUILD(347,358 , Tag1_VID                ));

    snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->cfidei1 ,
        LION3_EPCL_KEY4_FIELD_NAME_BUILD(359,359 , Tag1_CFI                ));

    snetLion3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->up1,
        LION3_EPCL_KEY4_FIELD_NAME_BUILD(360,362 , UP1                     ));





}

/*******************************************************************************
*   lion3EPclBuildKey5
*
* DESCRIPTION:
*        Build Layer4+IPv6 extended ePCL search key. - key5
*
* INPUTS:
*       devObjPtr            - pointer to device object.
*       descrPtr             - pointer to frame data buffer Id.
*        egressPort     - local egress port (not global).
*       currentLookupConfPtr - pointer to the current PCL configuration entry.
*       pclExtraDataPtr      - pointer to extra data needed for the ePCL engine.
*       keyIndex             - key index (0..7) - used for SIP5
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
static GT_VOID lion3EPclBuildKey5
(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr,
    IN  GT_U32                            egressPort,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC  * lookupConfPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC    * pclExtraDataPtr,
    IN DXCH_EPCL_KEY_ENT                  keyIndex,
    OUT SNET_CHT_POLICY_KEY_STC         * pclKeyPtr
)
{
    lion3EPclBuildKeyCommon(devObjPtr, descrPtr, egressPort, lookupConfPtr,
                                   pclExtraDataPtr, keyIndex, pclKeyPtr);

    /* extra fields beside 'common' */

    snetLion3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->dip[2],
        LION3_EPCL_KEY5_FIELD_NAME_BUILD(251,282 ,DIP_63_32                ));

    snetLion3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->dip[1],
        LION3_EPCL_KEY5_FIELD_NAME_BUILD(283,314 ,DIP_95_64                ));

    snetLion3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->dip[0],
        LION3_EPCL_KEY5_FIELD_NAME_BUILD(315,338 ,DIP_119_96               ));

    snetLion3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->dip[3],
        LION3_EPCL_KEY5_FIELD_NAME_BUILD(339,370 ,DIP_31_0                 ));

}

/*******************************************************************************
*   lion3EPclBuildKeyCommonKey6And7
*
* DESCRIPTION:
*       Lion3 : Build common EPCL search key for keys 6,7
*
* INPUTS:
*       devObjPtr            - pointer to device object.
*       descrPtr             - pointer to frame data buffer Id.
*        egressPort     - local egress port (not global).
*       lookupConfPtr        - pointer to EPCL configuration entry.
*       pclExtraDataPtr      - pointer to extra data needed for the EPCL engine.
*       keyIndex             - key index (0..7) - used for SIP5
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
static GT_VOID lion3EPclBuildKeyCommonKey6And7
(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr,
    IN  GT_U32                            egressPort,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC  * lookupConfPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC    * pclExtraDataPtr,
    IN DXCH_EPCL_KEY_ENT                  keyIndex,
    OUT SNET_CHT_POLICY_KEY_STC         * pclKeyPtr
)
{
    GT_U32  tmpVal;
    GT_STATUS   rc;
    GT_U8   byteValue;
    GT_U8   *l3HdrStartPtr;  /* Layer 3 offset start pointer */
    GT_U32  srcPort,trgPort;
    GT_U32  vid0;
    GT_BIT  tag_0_exist,tag_1_exist,tag0_is_outer_tag;
    GT_U32  tag0,tag1;

    if(keyIndex < DXCH_EPCL_KEY_6_E)
    {
        /* not valid for those keys */
        return;
    }

    snetLion3Epcl_vlanTagInfoGet(devObjPtr, descrPtr,
        &tag_0_exist, &tag_1_exist, &tag0_is_outer_tag,
        &tag0, &tag1);

    snetLion3Epcl_srcPortTrgPortGet(devObjPtr, descrPtr,egressPort,
                                    &srcPort, &trgPort);

    /*<EPCL Key Use Orig VID>*/
    smemRegFldGet(devObjPtr, SMEM_CHT3_EPCL_GLOBAL_REG(devObjPtr), 4, 1, &tmpVal);

    vid0 = tmpVal ? descrPtr->vid0Or1AfterTti : descrPtr->eVid;

    snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, lookupConfPtr->pclID ,
        LION3_EPCL_KEY6_FIELD_NAME_BUILD(0  ,  9 ,PCL_ID_9_0                ));

    snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, srcPort ,
        LION3_EPCL_KEY6_FIELD_NAME_BUILD(11 , 16 ,Src_Port_5_0              ));

    lion3EPclBuildKeyPacketTypeCommon(devObjPtr, descrPtr, egressPort, lookupConfPtr,
                                   pclExtraDataPtr, keyIndex, pclKeyPtr);

    snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, tag_1_exist ,
        LION3_EPCL_KEY6_FIELD_NAME_BUILD(24 , 24 ,Tag1_Exist                ));

    snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sstId ,
        LION3_EPCL_KEY6_FIELD_NAME_BUILD(25 , 36 ,Source_ID_11_0            ));

    snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, vid0 ,
        LION3_EPCL_KEY6_FIELD_NAME_BUILD(37 , 48 ,VID0                      ));

    snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, srcPort >> 5 ,
        LION3_EPCL_KEY6_FIELD_NAME_BUILD(49 , 50 ,Src_Port_7_6              ));

    snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->srcDev >> 5 ,
        LION3_EPCL_KEY6_FIELD_NAME_BUILD(58 ,59  ,Source_Dev_6_5            ));

    snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->origIsTrunk ,
        LION3_EPCL_KEY6_FIELD_NAME_BUILD(60 ,60  ,Src_Is_Trunk              ));

    /* the EPCL need to use the actual mac addresses of egress and not those of ingress */
    /* For non-tunnel terminated packets:
            Transmitted packet MACs after packet modification (for example, due to routing or Tunnel Termination)
        For Ethernet-Over-xxx Tunnel-Start packets:
            MACs of the passenger packet
    */
    if(descrPtr->haToEpclInfo.macDaSaPtr)
    {
        snetLion3EPclKeyFieldBuildByPointer(pclKeyPtr, &descrPtr->haToEpclInfo.macDaSaPtr[6]/* mac SA */,
            LION3_EPCL_KEY6_FIELD_NAME_BUILD(61 ,108 ,MAC_SA                    ));
        snetLion3EPclKeyFieldBuildByPointer(pclKeyPtr, descrPtr->haToEpclInfo.macDaSaPtr,
            LION3_EPCL_KEY6_FIELD_NAME_BUILD(109,156 ,MAC_DA                    ));
    }


    snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL2Valid ,
        LION3_EPCL_KEY6_FIELD_NAME_BUILD(157,157 ,Is_L2_Valid               ));

    snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->isIp,
        LION3_EPCL_KEY6_FIELD_NAME_BUILD(158,158 ,Is_IP                     ));

    snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->ipProt,
        LION3_EPCL_KEY6_FIELD_NAME_BUILD(159,166 ,Ip_Protocol               ));

    snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dscp,
        LION3_EPCL_KEY6_FIELD_NAME_BUILD(167,172 ,Packet_DSCP               ));

    l3HdrStartPtr = SNET_CHT3_PCKT_TUNNEL_START_PSGR(descrPtr);

    snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, l3HdrStartPtr[1] ,
        LION3_EPCL_KEY6_FIELD_NAME_BUILD(173,174 ,TOS_1_0                   ));

    snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->ttl,
        LION3_EPCL_KEY6_FIELD_NAME_BUILD(175,182 ,TTL                       ));

    snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->arp ,
        LION3_EPCL_KEY6_FIELD_NAME_BUILD(183,183 ,Is_ARP                    ));

    rc = snetCht2EPclTcpUdpPortRangeCompareGet(devObjPtr,descrPtr,&byteValue);
    if(rc != GT_OK)
    {
        byteValue = 0;
    }
    snetLion3EPclKeyFieldBuildByValue(pclKeyPtr,byteValue,
        LION3_EPCL_KEY6_FIELD_NAME_BUILD(264,271 ,TCP_UDP_Port_Comparators  ));


    if (descrPtr->l4StartOffsetPtr != NULL)
    {
        snetLion3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->l4StartOffsetPtr[13],
            LION3_EPCL_KEY6_FIELD_NAME_BUILD(272,279 ,L4_Header_Byte_13         ));

        snetLion3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->l4StartOffsetPtr[1],
            LION3_EPCL_KEY6_FIELD_NAME_BUILD(280,287 ,L4_Header_Byte_1          ));

        snetLion3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->l4StartOffsetPtr[0],
            LION3_EPCL_KEY6_FIELD_NAME_BUILD(288,295 ,L4_Header_Byte_0          ));

        if (descrPtr->ipv6Icmp || descrPtr->ipv4Icmp)
        {
            snetLion3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->l4StartOffsetPtr[4],
                LION3_EPCL_KEY6_FIELD_NAME_BUILD(304,311 ,L4_Header_Byte_2_OR_4     ));

            snetLion3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->l4StartOffsetPtr[5],
                LION3_EPCL_KEY6_FIELD_NAME_BUILD(296,303 ,L4_Header_Byte_3_OR_5     ));
        }
        else
        {
            snetLion3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->l4StartOffsetPtr[2],
                LION3_EPCL_KEY6_FIELD_NAME_BUILD(304,311 ,L4_Header_Byte_2_OR_4     ));

            snetLion3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->l4StartOffsetPtr[3],
                LION3_EPCL_KEY6_FIELD_NAME_BUILD(296,303 ,L4_Header_Byte_3_OR_5     ));
        }
    }

    snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isL4Valid,
        LION3_EPCL_KEY6_FIELD_NAME_BUILD(312,312 ,Is_L4_Valid               ));

    if (descrPtr->outGoingMtagCmd == SKERNEL_MTAG_CMD_FROM_CPU_E)
    {
        snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->qos.fromCpuQos.fromCpuTc,
            LION3_EPCL_KEY6_FIELD_NAME_BUILD(313,315 ,From_CPU_TC               ));

        snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->qos.fromCpuQos.fromCpuDp,
            LION3_EPCL_KEY6_FIELD_NAME_BUILD(316,317 ,From_CPU_DP               ));
    }

    SNET_CHT3_EPCL_KEY_EGT_PKT_TYPE(descrPtr, tmpVal);
    snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, tmpVal,
        LION3_EPCL_KEY6_FIELD_NAME_BUILD(318,319 ,Packet_Type               ));

    if (descrPtr->outGoingMtagCmd == SKERNEL_MTAG_CMD_TO_CPU_E)
    {
        /*Src_Trg*/
        tmpVal = (descrPtr->srcTrg & 0x1);
    }
    else
    if (descrPtr->outGoingMtagCmd == SKERNEL_MTAG_CMD_TO_TRG_SNIFFER_E)
    {
        /*Tx_Mirror*/
        tmpVal = descrPtr->rxSniff ? 0 : 1;
    }
    else
    {
        tmpVal = 0;
    }

    snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, tmpVal,
        LION3_EPCL_KEY6_FIELD_NAME_BUILD(320,320 ,Src_Trg_OR_Tx_Mirror      ));

    snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->up,
        LION3_EPCL_KEY6_FIELD_NAME_BUILD(321,323 ,Assigned_UP               ));

    snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, trgPort,
        LION3_EPCL_KEY6_FIELD_NAME_BUILD(324,329 ,Trg_Port_5_0              ));

    snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->rxSniff,
        LION3_EPCL_KEY6_FIELD_NAME_BUILD(330,330 ,Rx_Sniff                  ));

    snetLion3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->routed,
        LION3_EPCL_KEY6_FIELD_NAME_BUILD(331,331 ,Is_Routed                 ));

    tmpVal = (descrPtr->isIp && descrPtr->isIPv4 == 0) ? 1 : 0;

    snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, tmpVal,
        LION3_EPCL_KEY6_FIELD_NAME_BUILD(332,332 ,Is_IPv6                   ));

    if(keyIndex == DXCH_EPCL_KEY_6_E)
    {
        if (descrPtr->isIp && ((l3HdrStartPtr[16] >> 2) & 0x1f))
        {
            tmpVal = 1;
        }
        else
        {
            tmpVal = 0;
        }

        snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[0],
            LION3_EPCL_KEY6_FIELD_NAME_BUILD(184,191 ,SIP_7_0                   ));

        snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[0] >> 8,
            LION3_EPCL_KEY6_FIELD_NAME_BUILD(208,231 ,SIP_31_8                  ));

        snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[0],
            LION3_EPCL_KEY6_FIELD_NAME_BUILD(232,263 ,DIP_31_0                  ));

        snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, tmpVal,
            LION3_EPCL_KEY6_FIELD_NAME_BUILD(333,333 ,Ipv4_Options              ));

        snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->useVidx ,
            LION3_EPCL_KEY6_FIELD_NAME_BUILD(334,334 ,is_VIDX                   ));

        snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->vid1,
            LION3_EPCL_KEY6_FIELD_NAME_BUILD(335,346 ,Tag1_VID                  ));

        snetLion3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->cfidei1,
            LION3_EPCL_KEY6_FIELD_NAME_BUILD(347,347 ,Tag1_CFI                  ));

        snetLion3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->up1,
            LION3_EPCL_KEY6_FIELD_NAME_BUILD(348,350 ,UP1                       ));

        if(descrPtr->outGoingMtagCmd == SKERNEL_MTAG_CMD_FORWARD_E && descrPtr->origIsTrunk)
        {
            snetLion3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->origSrcEPortOrTrnk >> 7,
                LION3_EPCL_KEY6_FIELD_NAME_BUILD(351,355 ,Source_Dev_OR_Trunk_11_7  ));
        }
        else
        {
            snetLion3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->srcDev >> 7,
                LION3_EPCL_KEY6_FIELD_NAME_BUILD(351,355 ,Source_Dev_OR_Trunk_11_7  ));
        }


        snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, trgPort >> 6,
            LION3_EPCL_KEY6_FIELD_NAME_BUILD(356,357 ,Trg_Port_7_6              ));
    }
    else
    {
        /* key 7*/

        snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpV6EhExists,
            LION3_EPCL_KEY7_FIELD_NAME_BUILD(333,333 ,IPv6_EH_exist             ));

        snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->solicitationMcastMsg,
            LION3_EPCL_KEY7_FIELD_NAME_BUILD(334,334 ,IPv6_Is_ND                ));

        snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, pclExtraDataPtr->isIpV6EhHopByHop,
            LION3_EPCL_KEY7_FIELD_NAME_BUILD(335,335 ,Is_IPv6_EH_Hop_By_Hop     ));

        snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[3],
            LION3_EPCL_KEY6_FIELD_NAME_BUILD(184,191 ,SIP_7_0                   ));
        snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[3] >> 8,
            LION3_EPCL_KEY6_FIELD_NAME_BUILD(208,231 ,SIP_31_8                  ));
        snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[2],
            LION3_EPCL_KEY7_FIELD_NAME_BUILD(336,367 ,SIP_63_32                 ));
        snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[1],
            LION3_EPCL_KEY7_FIELD_NAME_BUILD(368,399 ,SIP_95_64                 ));
        snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->sip[0],
            LION3_EPCL_KEY7_FIELD_NAME_BUILD(416,447 ,SIP_127_96                ));

        snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[3],
            LION3_EPCL_KEY6_FIELD_NAME_BUILD(232,263 ,DIP_31_0                  ));
        snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[2],
            LION3_EPCL_KEY7_FIELD_NAME_BUILD(448,479 ,DIP_63_32                 ));
        snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[1],
            LION3_EPCL_KEY7_FIELD_NAME_BUILD(480,511 ,DIP_95_64                 ));
        snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->dip[0],
            LION3_EPCL_KEY7_FIELD_NAME_BUILD(512,543 ,DIP_127_96                ));

        if(descrPtr->outGoingMtagCmd == SKERNEL_MTAG_CMD_FORWARD_E && descrPtr->origIsTrunk)
        {
            snetLion3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->origSrcEPortOrTrnk >> 7,
                LION3_EPCL_KEY7_FIELD_NAME_BUILD(561,565 ,Source_Dev_OR_Trunk_11_7  ));
        }
        else
        {
            snetLion3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->srcDev >> 7,
                LION3_EPCL_KEY7_FIELD_NAME_BUILD(561,565 ,Source_Dev_OR_Trunk_11_7  ));
        }

        snetLion3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->origSrcEPortOrTrnk,
            LION3_EPCL_KEY7_FIELD_NAME_BUILD(566,578 ,SRC_ePort_OR_Trunk_ID     ));

        snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->useVidx ,
            LION3_EPCL_KEY7_FIELD_NAME_BUILD(544,544 ,is_VIDX                   ));

        snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->vid1 ,
            LION3_EPCL_KEY7_FIELD_NAME_BUILD(545,556 ,Tag1_VID                  ));

        snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, descrPtr->cfidei1 ,
            LION3_EPCL_KEY7_FIELD_NAME_BUILD(557,557 ,Tag1_CFI                  ));

        if(descrPtr->mpls)
        {
            snetLion3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->exp2,
                LION3_EPCL_KEY7_FIELD_NAME_BUILD(558,560 ,UP1                       ));
        }
        else
        {
            snetLion3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->up1,
                LION3_EPCL_KEY7_FIELD_NAME_BUILD(558,560 ,UP1                       ));
        }


        snetLion3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->trgEPort,
            LION3_EPCL_KEY7_FIELD_NAME_BUILD(583,595 ,TRG_ePort                 ));

        /* those bits taken from 'eVid' regardless to 'vid0' (mode of bit 4 in the global register) */
        snetLion3EPclKeyFieldBuildByValue(pclKeyPtr,descrPtr->eVid >> 12,
            LION3_EPCL_KEY7_FIELD_NAME_BUILD(600,600 ,eVLAN_12                  ));

        snetLion3EPclKeyFieldBuildByValue(pclKeyPtr, trgPort >> 6 ,
            LION3_EPCL_KEY7_FIELD_NAME_BUILD(604,605 ,Trg_Port_7_6              ));

    }
}

/*******************************************************************************
*   lion3EPclBuildKey6
*
* DESCRIPTION:
*        Build  RACL/VACL (48B) IPv4 - Egress NonIP or IPv4/ARP search key. - key6
*
* INPUTS:
*       devObjPtr            - pointer to device object.
*       descrPtr             - pointer to frame data buffer Id.
*        egressPort     - local egress port (not global).
*       currentLookupConfPtr - pointer to the current PCL configuration entry.
*       pclExtraDataPtr      - pointer to extra data needed for the ePCL engine.
*       keyIndex             - key index (0..7) - used for SIP5
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
static GT_VOID lion3EPclBuildKey6
(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr,
    IN  GT_U32                            egressPort,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC  * lookupConfPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC    * pclExtraDataPtr,
    IN DXCH_EPCL_KEY_ENT                  keyIndex,
    OUT SNET_CHT_POLICY_KEY_STC         * pclKeyPtr
)
{
    lion3EPclBuildKeyCommonKey6And7(devObjPtr, descrPtr, egressPort, lookupConfPtr,
                                   pclExtraDataPtr, keyIndex, pclKeyPtr);
}

/*******************************************************************************
*   lion3EPclBuildKey7
*
* DESCRIPTION:
*        Build  RACL/VACL (72B) IPv6 - Egress search key. - key7
*
* INPUTS:
*       devObjPtr            - pointer to device object.
*       descrPtr             - pointer to frame data buffer Id.
*        egressPort     - local egress port (not global).
*       currentLookupConfPtr - pointer to the current PCL configuration entry.
*       pclExtraDataPtr      - pointer to extra data needed for the ePCL engine.
*       keyIndex             - key index (0..7) - used for SIP5
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
static GT_VOID lion3EPclBuildKey7
(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr,
    IN  GT_U32                            egressPort,
    IN SNET_CHT3_PCL_LOOKUP_CONFIG_STC  * lookupConfPtr,
    IN CHT_PCL_EXTRA_PACKET_INFO_STC    * pclExtraDataPtr,
    IN DXCH_EPCL_KEY_ENT                  keyIndex,
    OUT SNET_CHT_POLICY_KEY_STC         * pclKeyPtr
)
{
    lion3EPclBuildKeyCommonKey6And7(devObjPtr, descrPtr, egressPort, lookupConfPtr,
                                   pclExtraDataPtr, keyIndex, pclKeyPtr);
}

/*******************************************************************************
* lion3EPclActionGet
*
* DESCRIPTION:
*        Get the action entry from the policy action table. - sip5.
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
static GT_VOID lion3EPclActionGet
(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr,
    IN GT_U32                             matchIndex,
    OUT SNET_LION3_EPCL_ACTION_STC       * actionDataPtr
)
{
    GT_U32  tblAddr;
    GT_U32 *memPtr;

    tblAddr = SMEM_LION3_TCAM_ACTION_TBL_MEM(devObjPtr, matchIndex);

    memPtr = smemMemGet(devObjPtr, tblAddr);

    actionDataPtr->packetCmd =
        SMEM_LION3_EPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_COMMAND);

    actionDataPtr->dscpOrExp =
        SMEM_LION3_EPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_DSCP_EXP);

    actionDataPtr->up0 =
        SMEM_LION3_EPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_UP0);

    actionDataPtr->modifyDscpOrExp =
        SMEM_LION3_EPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_MODIFY_DSCP_EXP);

    actionDataPtr->enableModifyUp0 =
        SMEM_LION3_EPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_ENABLE_MODIFY_UP0);

    actionDataPtr->bindToCncCounter =
        SMEM_LION3_EPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_BIND_TO_CNC_COUNTER);

    actionDataPtr->cncCounterIndex =
        SMEM_LION3_EPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_CNC_COUNTER_INDEX);

    actionDataPtr->tag1VidCmd =
        SMEM_LION3_EPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_TAG1_VID_CMD);

    actionDataPtr->enableModifyUp1 =
        SMEM_LION3_EPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_ENABLE_MODIFY_UP1);

    actionDataPtr->tag1Vid =
        SMEM_LION3_EPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_TAG1_VID);

    actionDataPtr->up1 =
        SMEM_LION3_EPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_UP1);

    actionDataPtr->tag0VlanCmd =
        SMEM_LION3_EPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_TAG0_VLAN_CMD);

    actionDataPtr->tag0Vid =
        SMEM_LION3_EPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_TAG0_VID);

    actionDataPtr->bindToPolicerCounter =
        SMEM_LION3_EPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_BIND_TO_POLICER_COUNTER);

    actionDataPtr->bindToPolicerMeter =
        SMEM_LION3_EPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_BIND_TO_POLICER_METER);

    actionDataPtr->policerIndex =
        SMEM_LION3_EPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_POLICER_INDEX);

    actionDataPtr->flowId =
        SMEM_LION3_EPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_FLOW_ID);

    actionDataPtr->tmQueueId =
        SMEM_LION3_EPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_TM_QUEUE_ID);

    actionDataPtr->oamInfo.oamProcessEnable =
        SMEM_LION3_EPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_OAM_PROCESSING_ENABLE);

    actionDataPtr->oamInfo.oamProfile =
        SMEM_LION3_EPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_OAM_PROFILE);

    actionDataPtr->oamInfo.ptpTimeStampEnable =
        SMEM_LION3_EPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_TIME_STAMP_ENABLE);

    actionDataPtr->oamInfo.ptpTimeStampOffsetIndex =
        SMEM_LION3_EPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_TIME_STAMP_OFFSET_INDEX);

    actionDataPtr->oamInfo.channelTypeToOpcodeMappingEn =
        SMEM_LION3_EPCL_ACTION_ENTRY_FIELD_GET(SMEM_LION3_EPCL_ACTION_TABLE_FIELDS_CHANNEL_TYPE_TO_OPCODE_MAPPING_EN);

}

/*******************************************************************************
* lion3EPclActionApply
*
* DESCRIPTION:
*        Apply the action entry from the policy action table. - Sip5.
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
*
*******************************************************************************/
GT_VOID lion3EPclActionApply
(
    IN SKERNEL_DEVICE_OBJECT                * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC   * descrPtr,
    IN SNET_LION3_EPCL_ACTION_STC           * actionDataPtr
)
{
    /* DECLARE_FUNC_NAME(lion3EPclActionApply); */

    if (actionDataPtr->enableModifyUp0 == EPCL_ACTION_MODIFY_OUTER_TAG_E) /* 0 - no modification */
    {                                    /* 2 ,3 are reserved values */
       /*ModifyOuterTag : EPCL Action entry <Tag0 VID> is assigned to the outer tag of the packet.
          If the packet is tunneled, this refers to the tunnel header tag.*/

       descrPtr->up = actionDataPtr->up0;
    }
    else if(actionDataPtr->enableModifyUp0 == EPCL_ACTION_MODIFY_TAG0_E)
    {
       /* ModifyTag0: EPCL Action entry <Tag0 VID> is assigned to VID field
         of tag 0 of the packet */
       descrPtr->up = actionDataPtr->up0;
    }

    /* Enables the modification of the packet 802.1q DSCP field */
    if (actionDataPtr->modifyDscpOrExp != 0) /* 0 - no modification */
    {                                        /* 3 is reserved value */
       descrPtr->dscp = actionDataPtr->dscpOrExp;
       descrPtr->modifyDscp = actionDataPtr->modifyDscpOrExp;

       descrPtr->epclAction.exp = actionDataPtr->dscpOrExp;
       descrPtr->epclAction.modifyExp = actionDataPtr->modifyDscpOrExp;
    }

    if (actionDataPtr->enableModifyUp0) /* 0 - no modification */
    {
        descrPtr->epclAction.modifyUp = actionDataPtr->enableModifyUp0;
        descrPtr->epclAction.up = actionDataPtr->up0;
    }

    if (actionDataPtr->modifyDscpOrExp) /* 0 - no modification */
    {
        descrPtr->epclAction.modifyDscp = actionDataPtr->modifyDscpOrExp;
        descrPtr->epclAction.dscp = actionDataPtr->dscpOrExp;
    }

    if (actionDataPtr->tag0VlanCmd) /* 0 - no modification */
    {
        descrPtr->epclAction.modifyVid = actionDataPtr->tag0VlanCmd;
        descrPtr->epclAction.vid = actionDataPtr->tag0Vid;
    }

    /*Tag1 VLAN Command*/
    if (actionDataPtr->tag1VidCmd) /* 0 - no modification */
    {
        descrPtr->epclAction.vlan1Cmd = actionDataPtr->tag1VidCmd;
        /*Tag1 VID*/
        descrPtr->epclAction.vid1 = actionDataPtr->tag1Vid;
    }

    /*Enable Modify UP1*/
    if (actionDataPtr->enableModifyUp1) /* 0 - keep previous */
    {
        descrPtr->epclAction.modifyUp1 = actionDataPtr->enableModifyUp1;
        /*UP1*/
        descrPtr->epclAction.up1 =  actionDataPtr->up1;
    }

    if (actionDataPtr->packetCmd != SKERNEL_CHT2_EPCL_CMD_FORWARD_E)
    {
        descrPtr->epclAction.drop = SKERNEL_CHT2_EPCL_CMD_HARD_DROP;
    }

    if (actionDataPtr->bindToPolicerMeter)
    {
        descrPtr->policerEgressEn = actionDataPtr->bindToPolicerMeter;
    }

    if (actionDataPtr->bindToPolicerCounter)
    {
        descrPtr->policerEgressCntEn = actionDataPtr->bindToPolicerCounter;
    }

    if (actionDataPtr->bindToPolicerMeter ||
        actionDataPtr->bindToPolicerCounter)
    {
        descrPtr->policerPtr = actionDataPtr->policerIndex;
    }

    if(actionDataPtr->flowId)
    {
        /* override only when non zero */
        descrPtr->flowId = actionDataPtr->flowId;
    }

    descrPtr->tmQueueId = actionDataPtr->tmQueueId;
}

/*******************************************************************************
* lion3EPclConfigPtrGet
*
* DESCRIPTION:
*       Get data from the egress PCL configuration table.  - sip5
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
static GT_VOID lion3EPclConfigPtrGet
(
    IN SKERNEL_DEVICE_OBJECT                * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC      * descrPtr ,
    IN GT_U32                                 egressPort,
    OUT SNET_CHT3_PCL_LOOKUP_CONFIG_STC     * lookupConfPtr
)
{
    DECLARE_FUNC_NAME(lion3EPclConfigPtrGet);

    GT_U32  epclGlobalRegValue;/* value of EPCL global register */
    GT_U32  maxPortNo;
    GT_U32  index = 0;
    GT_U32  pclCfgMode = 0;
    GT_U32  egressPclCfgMode;
    GT_U32  portIfPCLBlock;
    GT_U32  *epclCfgEntryPtr;
    GT_U32  portListEn;
    GT_U32  tsNormalMode;/*tunnel start normal mode or vlan mode */
    GT_BIT  forceVlanMode = 0;
    GT_BIT  epcl_key_use_orig_vid;/*Select VLAN field source for Lookup keys*/
    GT_BIT  epcl_key_ing_analyzer_use_orig_vid;/*If active, and ingress analyzed packet was VLAN-tagged,
                                                ingress analyzed traffic uses the VID from the incoming packet to access the EPCL Config table
                                                and for lookup keys generation*/
    GT_U32  epclVid;   /* the VID to be used in the EPCL Config table */
    GT_U32  epclKeyVid;/* the VID to be used in the EPCL build key */
    GT_BIT bOutgoingMtagToSniffer = (descrPtr->outGoingMtagCmd == SKERNEL_MTAG_CMD_TO_TRG_SNIFFER_E) ? 1 : 0;
    GT_BIT bOutgoingMtagToCpu     = (descrPtr->outGoingMtagCmd == SKERNEL_MTAG_CMD_TO_CPU_E) ? 1 : 0;

    GT_BIT m_bRxTrappedOrMonitored = ((bOutgoingMtagToSniffer && descrPtr->rxSniff) ||
                             (bOutgoingMtagToCpu && (0 == descrPtr->srcTrg))) ? 1 : 0;
    GT_BIT m_bTxTrappedOrMonitored = ((bOutgoingMtagToSniffer && (0 == descrPtr->rxSniff)) ||
                             (bOutgoingMtagToCpu && (1 == descrPtr->srcTrg))) ? 1 : 0;
    GT_U32  uTrgPort,uTrgDev;
    GT_BIT  useTxTrappedOrMonitored;

    __LOG_PARAM(m_bRxTrappedOrMonitored);
    __LOG_PARAM(m_bTxTrappedOrMonitored);


    if(m_bTxTrappedOrMonitored && (descrPtr->srcTrgDev == descrPtr->ownDev))
    {
        useTxTrappedOrMonitored = 1;
    }
    else
    {
        useTxTrappedOrMonitored = 0;
    }

    if(useTxTrappedOrMonitored)
    {
        uTrgPort  = SMEM_CHT_IS_SIP5_GET(devObjPtr) ?
                    descrPtr->eArchExtInfo.srcTrgEPort :
                    descrPtr->egressTrgPort;
        uTrgDev   = descrPtr->srcTrgDev;

        __LOG(("The Epcl use configurations of the 'orig egress physical port' [%d] and not analyzer/cpu egress port[%d] \n",
            uTrgPort,
            egressPort))

        __LOG_PARAM(uTrgPort);
        __LOG_PARAM(uTrgDev);
    }
    else
    {
        uTrgPort = egressPort;
        uTrgDev = descrPtr->trgDev;
    }

    smemRegGet(devObjPtr, SMEM_CHT3_EPCL_GLOBAL_REG(devObjPtr),&epclGlobalRegValue);

    epcl_key_use_orig_vid = SMEM_U32_GET_FIELD(epclGlobalRegValue,4,1);
    epcl_key_ing_analyzer_use_orig_vid = SMEM_U32_GET_FIELD(epclGlobalRegValue,3,1);

    if(m_bRxTrappedOrMonitored && descrPtr->tagSrcTagged[SNET_CHT_TAG_0_INDEX_CNS] &&
        epcl_key_ing_analyzer_use_orig_vid)
    {
        epclVid = descrPtr->vid0Or1AfterTti;

        __LOG(("EPCL will use 'orig vid' [0x%3.3x] for ingress_trapped/ingress_mirror tagged packets and not eVid (and originally had vlan tag) \n",
            epclVid));
    }
    else
    {
        /* this logic of 0xFFF that should be applied only on accessing to the
           configuration table impact also the EPCL keys !
           so when key needs more then 12 bits it is using descrPtr->eVid
           regardless to this logic ...
           ALSO in HW : see that eVLAN_12 using eVid and not epclKeyVid  */
        epclVid = descrPtr->eVid & 0xFFF;
        __LOG(("EPCL will use 12 LSBits from 'evlan' [0x%3.3x] \n",
            epclVid));
    }

    if(epcl_key_use_orig_vid)
    {
        epclKeyVid = descrPtr->vid0Or1AfterTti;
        __LOG(("EPCL will use 'orig vid' [0x%4.4x] --> for build key (epclKeyVid) \n",
            epclKeyVid));
    }
    else
    {
        epclKeyVid = epclVid;
        __LOG(("EPCL will use 'epclVid' [0x%4.4x] --> for build key (epclKeyVid) \n",
            epclKeyVid));
    }

    descrPtr->haToEpclInfo.epclKeyVid = epclKeyVid;

    __LOG_PARAM(descrPtr->haToEpclInfo.epclKeyVid);

    /* Port list mode - enabled or not */
    portListEn = SMEM_U32_GET_FIELD(epclGlobalRegValue,5,1);
    __LOG(("Port list mode = [%d] \n",
        portListEn));

    if(descrPtr->tunnelStart)
    {
        /*EPCL TS Lookup CFG*/
        tsNormalMode = SMEM_U32_GET_FIELD(epclGlobalRegValue,11,1);

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

    if(portListEn == 0 ||
       devObjPtr->errata.epclPortListModeForcePortMode == 0)
    {
        if(forceVlanMode)
        {
            /* vlan mode is forced */
            pclCfgMode = 1;
        }
        else
        {
            pclCfgMode =
                SMEM_LION3_HA_PHYSICAL_PORT_1_ENTRY_FIELD_AUTO_GET(devObjPtr,
                    descrPtr,
                    SMEM_LION3_HA_PHYSICAL_PORT_TABLE_1_FIELDS_EPCL_LOOKUP_CONFIGURATION_MODE);
        }
    }
    else
    {
        /*[JIRA]  [EPCL-443] 'EPort mode' is ignored when PortListMode = 1 So 'Port mode' used to access the EPCL configuration table.*/
        /*this is actual device behavior */
        __LOG(("Force access to the EPCL configuration table with 'PORT mode' due to <Port list mode> = 1 \n"));
        pclCfgMode = 0;/* port MODE */
    }


    if(pclCfgMode == 2)/* ePort mode */
    {
        __LOG(("Access to the EPCL configuration table with 'ePort mode' \n"));
        /*logic is similar to HA ePort Table 1 indexing logic*/
        /* EPCL configuration index is 12 lower bits of the Target ePort assigned to the packet */
        index =  descrPtr->eArchExtInfo.haEgressEPortAtt1Table_index & 0xFFF;
    }
    else
    if (pclCfgMode == 1)
    {
        __LOG(("Access to the EPCL configuration table with 'VLAN mode' \n"));
        index = epclVid & 0xFFF; /* Access ePCL-id config. table with vlan id  */
    }
    else /*pclCfgMode == 0*/
    if (pclCfgMode == 0)
    {
        __LOG(("Access to the EPCL configuration table with 'Port mode' \n"));
        egressPclCfgMode = SMEM_U32_GET_FIELD(epclGlobalRegValue,1,1);
        maxPortNo = SMEM_U32_GET_FIELD(epclGlobalRegValue,18,1);
        portIfPCLBlock = SMEM_U32_GET_FIELD(epclGlobalRegValue,16,2);

        __LOG_PARAM(egressPclCfgMode);
        if (egressPclCfgMode == 0)
        {/* EPCL configuration table index = Local Device Source Port# + 4K */
            __LOG(("EPCL configuration table index = Local Device Source Port# + 4K"));
            index = (1 << 12) | uTrgPort;
        }
        else
        {
            if(useTxTrappedOrMonitored == 0)/* uTrgPort already initialized */
            {
                uTrgPort = descrPtr->trgEPort;
            }

            /* read the max port per device and the ePCL interface block */
            __LOG_PARAM(maxPortNo);
            __LOG_PARAM(portIfPCLBlock);
            if (maxPortNo == 0)
            {   /* Max port per device = 0 , up to 1K ports in system */
                __LOG(("Max port per device = 0 , up to 1K ports in system"));
                index = ((portIfPCLBlock & 0x3)<< 10) |
                        ((uTrgDev & 0x1f)<< 5) |
                        ((uTrgPort & 0x1f));
            }
            else
            {   /* Max port per device = 1 , up to 2K ports in system */
                __LOG(("Max port per device = 1 , up to 2K ports in system"));
                index = ((portIfPCLBlock & 0x1)<< 11) |
                        ((uTrgDev & 0x1f)<< 6) |
                        ((uTrgPort & 0x3f));
            }
        }
    }
    else /*unknown*/
    {
        __LOG(("Configuration ERROR: unknown pclCfgMode[%d] ,use index 0 \n",
            pclCfgMode));
        index = 0;
    }

    __LOG(("final Index [0x%x] into the 'EPCL configuration table' \n",
        index));

    /* calculate the address of the EPCL configuration table */
    epclCfgEntryPtr = smemMemGet(devObjPtr, SMEM_CHT3_EPCL_CONFIG_TBL_MEM(devObjPtr, index));

    /* The Egress PCL configuration Table */
    __LOG(("The Egress PCL configuration Table"));
    lookupConfPtr->lookUpEn = snetFieldValueGet(epclCfgEntryPtr,0,1);
    lookupConfPtr->pclID = snetFieldValueGet(epclCfgEntryPtr, 1, 10);
    lookupConfPtr->pclNonIpTypeCfg = snetFieldValueGet(epclCfgEntryPtr, 11, 4);
    lookupConfPtr->pclIpV4ArpTypeCfg = snetFieldValueGet(epclCfgEntryPtr, 15, 4);
    lookupConfPtr->pclIpV6TypeCfg = snetFieldValueGet(epclCfgEntryPtr, 19, 4);
    lookupConfPtr->tcamSegmentMode = snetFieldValueGet(epclCfgEntryPtr, 23, 3);
    lookupConfPtr->udbKeyBitmapEnable = snetFieldValueGet(epclCfgEntryPtr, 26, 1);

    /* Note: For Egress PCL, Port-List method is applied only to packets with command = Forward */
    if ( (devObjPtr->pclSupport.pclSupportPortList) &&
         (descrPtr->outGoingMtagCmd == SKERNEL_MTAG_CMD_FORWARD_E) )
    {
        lookupConfPtr->pclPortListModeEn = portListEn ? GT_TRUE : GT_FALSE;
    }
    else
    {
        lookupConfPtr->pclPortListModeEn = GT_FALSE;
    }

    lookupConfPtr->ingrlookUpKey = GT_FALSE;

    if(simLogIsOpen())
    {
        __LOG_PARAM(lookupConfPtr->lookUpEn);
        __LOG_PARAM(lookupConfPtr->pclID);
        __LOG_PARAM(lookupConfPtr->pclNonIpTypeCfg);
        __LOG_PARAM(lookupConfPtr->pclIpV4ArpTypeCfg);
        __LOG_PARAM(lookupConfPtr->pclIpV6TypeCfg);
        __LOG_PARAM(lookupConfPtr->tcamSegmentMode);
        __LOG_PARAM(lookupConfPtr->udbKeyBitmapEnable);
        __LOG_PARAM(lookupConfPtr->pclPortListModeEn);
        __LOG_PARAM(lookupConfPtr->ingrlookUpKey);
    }

    return ;
}

/*******************************************************************************
* lion3EPclTriggeringCheck
*
* DESCRIPTION:
*        Check the enabling status of egress PCL for the outgoing frame.
*        Get the epcl-id configuration setup. - sip5
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
static GT_VOID lion3EPclTriggeringCheck
(
    IN SKERNEL_DEVICE_OBJECT             *  devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC   *  descrPtr,
    IN GT_U32                               egressPort,
    OUT GT_U8                            *  doLookupPtr,
    OUT SNET_CHT3_PCL_LOOKUP_CONFIG_STC  *  lookupConfPtr
)
{
    DECLARE_FUNC_NAME(lion3EPclTriggeringCheck);

    GT_U32  regAddress;
    GT_U32  * regPtr;
    GT_BIT epclEnabled,bypassEpclEnable;

    /* initialize the dolookupPtr */
    *doLookupPtr = 0;

    regAddress = SMEM_CHT3_EPCL_GLOBAL_REG(devObjPtr);
    regPtr = smemMemGet(devObjPtr, regAddress);

    /* Enable Egress Policy check */
    epclEnabled = SMEM_U32_GET_FIELD(*regPtr, 0, 1);
    if(epclEnabled == 0) /* logic of enabled */
    {
        __LOG(("EPCL : globally DISABLED \n"));
        return;
    }

    bypassEpclEnable  = SMEM_U32_GET_FIELD(*regPtr, 12, 1);
    if(bypassEpclEnable == 1)
    {
        __LOG(("EPCL : globally BYPASSED \n"));
        return;
    }

    epclEnabled = 0;

    switch (descrPtr->outGoingMtagCmd)
    {
        case SKERNEL_MTAG_CMD_TO_CPU_E:
            epclEnabled = 1;
            epclEnabled =
                SMEM_LION3_HA_PHYSICAL_PORT_2_ENTRY_FIELD_GET(devObjPtr,
                    descrPtr->eArchExtInfo.haEgressPhyPort2TablePtr,
                    egressPort,
                    SMEM_LION3_HA_PHYSICAL_PORT_TABLE_2_FIELDS_EPCL_TO_CPU_PACKETS_PORT_ENABLE);
            break;
        case SKERNEL_MTAG_CMD_FROM_CPU_E:
            if(descrPtr->egressFilterEn)/* may be filtered --> data */
            {
                epclEnabled =
                    SMEM_LION3_HA_PHYSICAL_PORT_1_ENTRY_FIELD_AUTO_GET(devObjPtr,
                        descrPtr,
                        SMEM_LION3_HA_PHYSICAL_PORT_TABLE_1_FIELDS_EPCL_FROM_CPU_DATA_PACKETS_PORT_ENABLE);
            }
            else /* can not be filtered --> control */
            {
                epclEnabled =
                    SMEM_LION3_HA_PHYSICAL_PORT_1_ENTRY_FIELD_AUTO_GET(devObjPtr,
                        descrPtr,
                        SMEM_LION3_HA_PHYSICAL_PORT_TABLE_1_FIELDS_EPCL_FROM_CPU_CONTROL_PACKETS_PORT_ENABLE);
            }
            break;
        case SKERNEL_MTAG_CMD_TO_TRG_SNIFFER_E:
            epclEnabled =
                SMEM_LION3_HA_PHYSICAL_PORT_2_ENTRY_FIELD_GET(devObjPtr,
                    descrPtr->eArchExtInfo.haEgressPhyPort2TablePtr,
                    egressPort,
                    SMEM_LION3_HA_PHYSICAL_PORT_TABLE_2_FIELDS_EPCL_TO_ANALYZER_PACKETS_PORT_ENABLE);
            break;
        case SKERNEL_MTAG_CMD_FORWARD_E:
        default:
            if(descrPtr->tunnelStart == 0)
            {
                epclEnabled =
                    SMEM_LION3_HA_PHYSICAL_PORT_1_ENTRY_FIELD_AUTO_GET(devObjPtr,
                        descrPtr,
                        SMEM_LION3_HA_PHYSICAL_PORT_TABLE_1_FIELDS_EPCL_NON_TS_DATA_PORT_ENABLE);
            }
            else
            {
                epclEnabled =
                    SMEM_LION3_HA_PHYSICAL_PORT_1_ENTRY_FIELD_AUTO_GET(devObjPtr,
                        descrPtr,
                        SMEM_LION3_HA_PHYSICAL_PORT_TABLE_1_FIELDS_EPCL_TS_DATA_PORT_ENABLE);
            }
            break;
    }

    if(epclEnabled == 0)
    {
        __LOG(("EPCL : DISABLED on the egress physical port [%d] \n",
            egressPort));
        return;
    }

    /* get EPCL configuration entry */
    __LOG(("get EPCL configuration entry"));
    memset(lookupConfPtr, 0, sizeof(SNET_CHT3_PCL_LOOKUP_CONFIG_STC));
    lion3EPclConfigPtrGet(devObjPtr,descrPtr,egressPort,lookupConfPtr);

    *doLookupPtr = (lookupConfPtr->lookUpEn == 0) ? 0 : 1;

    return ;
}


/*******************************************************************************
* snetXCatEPclActionGet
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
*
*******************************************************************************/
static GT_VOID snetXCatEPclActionGet
(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr,
    IN GT_U32                             matchIndex,
    OUT SNET_XCAT_EPCL_ACTION_STC       * actionDataPtr
)
{
    GT_U32  tblAddr;
    GT_U32 *actionEntryDataPtr;

    snetCht3EPclActionGet(devObjPtr, descrPtr, matchIndex,
                          CNV_CHT3_EPCL_ACTION_MAC(actionDataPtr));

    tblAddr = SMEM_CHT_PCL_ACTION_TBL_MEM(devObjPtr, matchIndex);

    actionEntryDataPtr = smemMemGet(devObjPtr, tblAddr);

    actionDataPtr->policerCountEn =
        SMEM_U32_GET_FIELD(actionEntryDataPtr[1], 31, 1);
    actionDataPtr->policerMeterEn =
        SMEM_U32_GET_FIELD(actionEntryDataPtr[2], 0, 1);
    actionDataPtr->policerIndex =
        SMEM_U32_GET_FIELD(actionEntryDataPtr[2], 1, 8);
    actionDataPtr->policerIndex |=
        SMEM_U32_GET_FIELD(actionEntryDataPtr[3], 10, 4) << 8;
}

/*******************************************************************************
* snetXCatEPclActionApply
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
*
*******************************************************************************/
GT_VOID snetXCatEPclActionApply
(
    IN SKERNEL_DEVICE_OBJECT                * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC   * descrPtr,
    IN SNET_XCAT_EPCL_ACTION_STC            * actionDataPtr
)
{
    DECLARE_FUNC_NAME(snetXCatEPclActionApply);

    snetCht3EPclActionApply(devObjPtr, descrPtr,
                            CNV_CHT3_EPCL_ACTION_MAC(actionDataPtr));

    descrPtr->policerEgressEn = actionDataPtr->policerMeterEn;
    /* when enable, this rule is bound to the policer indexed
       by actionDataPtr->policerIndex */
    if(descrPtr->policerEgressEn)
    {
        descrPtr->policerPtr = actionDataPtr->policerIndex;
    }
    /* Bind To Policer Counter */
    __LOG(("Bind To Policer Counter"));
    descrPtr->policerEgressCntEn = actionDataPtr->policerCountEn;
    if(descrPtr->policerEgressCntEn)
    {
        descrPtr->policerPtr = actionDataPtr->policerIndex;
    }

    return ;
}

/*******************************************************************************
* snetXCatEPclIpv6IsNdSet
*
* DESCRIPTION:
*        EPCL - set 'is ipv6 ND'
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
*******************************************************************************/
static GT_VOID snetXCatEPclIpv6IsNdSet
(
    IN SKERNEL_DEVICE_OBJECT                * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC   * descrPtr
)
{
    GT_U32 soliciteMask[4];     /* IPv6 Solicited-Node Mcst Mask */
    GT_U32 soliciteValue[4];    /* IPv6 Solicited-Node Mcst Value */
    GT_U32 *regDataPtr;         /* Registers data pointer */
    GT_U32 regNum;              /* Register index */

    /* prepare the is ipv6 ND */
    descrPtr->solicitationMcastMsg = 0;
    if(descrPtr->isIp == 0 || descrPtr->isIPv4)
    {
        /* not IPv6 */
        return;
    }

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        regDataPtr = smemMemGet(devObjPtr, SMEM_LION3_HA_EGR_POLICY_DIP_SOLICITATION_DATA_0_REG(devObjPtr));
        for(regNum = 0; regNum < 4; regNum++)
        {
            soliciteValue[3 - regNum] = regDataPtr[regNum];
        }

        regDataPtr = smemMemGet(devObjPtr, SMEM_LION3_HA_EGR_POLICY_DIP_SOLICITATION_MASK_0_REG(devObjPtr));
        for(regNum = 0; regNum < 4; regNum++)
        {
            soliciteMask[3 - regNum] = regDataPtr[regNum];
        }

        if (((descrPtr->dip[0] & soliciteMask[0]) == (soliciteMask[0] & soliciteValue[0])) &&
            ((descrPtr->dip[1] & soliciteMask[1]) == (soliciteMask[1] & soliciteValue[1])) &&
            ((descrPtr->dip[2] & soliciteMask[2]) == (soliciteMask[2] & soliciteValue[2])) &&
            ((descrPtr->dip[3] & soliciteMask[3]) == (soliciteMask[3] & soliciteValue[3])))
        {
            descrPtr->solicitationMcastMsg = 1;
        }
    }
    else
    {
        descrPtr->solicitationMcastMsg = (CH3_IPV6_IS_NET_DISCOVERY_DIP_MAC(descrPtr->dip) ? 1 : 0);
    }
}


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
*       egressPort   - local egress port (not global) - (global in sip5).
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
)
{
    DECLARE_FUNC_NAME(snetXCatEPcl);

    SNET_CHT3_PCL_LOOKUP_CONFIG_STC lookUpCfg;  /* lookup configuration table */
    GT_U8                           doLookup;   /* enable searching the TCAM */
    CHT_PCL_EXTRA_PACKET_INFO_STC   pclExtraData;
    SNET_CHT_POLICY_KEY_STC         pclKey;     /* policy key structure */
    GT_U32                          keyType;    /* tcam search key type */
    GT_U32                          matchIndexArr[SIP5_TCAM_MAX_NUM_OF_HITS_CNS];
                                     /* array of indexes to the match actions */
    GT_U32                          parallelLookupInst; /* loop param for parallel lookups */
    GT_U32                          validHitBmp = 0;
    GT_U32                          maxNumOfParallelLookups;
    SNET_XCAT_EPCL_ACTION_STC       actionData;
    SNET_LION3_EPCL_ACTION_STC      lion3Action;
    GT_BOOL matchCounterEn;
    GT_U32  matchCounterIndex;

    /* Get the EPCL configuration entry */
    __LOG(("Get the EPCL configuration entry , for egress port [%d] \n",
        egressPort));

    /* get the packet type of the actual egress packet (after TS)
       this type is NOT usually used , but some cases need it ! */
    snetEpclPacketTypeGet(devObjPtr, descrPtr,&descrPtr->outerPacketType,GT_TRUE);

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        lion3EPclTriggeringCheck(devObjPtr, descrPtr, egressPort, &doLookup, &lookUpCfg);
    }
    else
    {
        snetCht3EPclTriggeringCheck(devObjPtr, descrPtr, egressPort, &doLookup, &lookUpCfg);
    }

    if (doLookup == 0) /* = 0, means EPCL engine is disabled */
    {
        __LOG(("EPCL engine is disabled for this egress port"));
        return;
    }

    /* PCL_ID from PCL configuration table */
    descrPtr->pclId = lookUpCfg.pclID;

    /* prepare the 'is ipv6 ND' */
    snetXCatEPclIpv6IsNdSet(devObjPtr, descrPtr);

    /* Parse extra data needs by EPCL engine */
    __LOG(("Parse extra data needs by EPCL engine"));
    snetChtPclSrvParseExtraData(devObjPtr, descrPtr, &pclExtraData);
    descrPtr->pclExtraDataPtr = &pclExtraData;

    /* Get from configuration the key type and the keysize */
    __LOG(("Get from configuration the key type and the key size"));
    snetXCatPclGetKeyType(devObjPtr,descrPtr, &lookUpCfg, &keyType);
    /* Create egress policy tcam search key */
    __LOG(("Create egress policy tcam search key"));
    pclKey.devObjPtr = devObjPtr;

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        lion3EPclCreateKey(devObjPtr, descrPtr, egressPort , &lookUpCfg, &pclExtraData,
                              keyType, &pclKey);
    }
    else
    {
        snetCht3EPclCreateKey(devObjPtr, descrPtr, egressPort , &lookUpCfg, &pclExtraData,
                              keyType, &pclKey);
    }

    /* Search key in policy Tcam */
    __LOG(("Search key in policy Tcam"));

    for (parallelLookupInst = 0 ;
         parallelLookupInst < SIP5_TCAM_MAX_NUM_OF_HITS_CNS ;
         parallelLookupInst++)
    {
        matchIndexArr[parallelLookupInst] = SNET_CHT_POLICY_NO_MATCH_INDEX_CNS;
    }

    if(devObjPtr->supportEArch)
    {
        snetSip5PclTcamLookup(devObjPtr, SIP5_TCAM_CLIENT_EPCL_E,
                           pclKey.key.data, pclKey.pclKeyFormat, &matchIndexArr[0]);

        for (parallelLookupInst = 0 ;
             parallelLookupInst < SIP5_TCAM_MAX_NUM_OF_HITS_CNS ;
             parallelLookupInst++)
        {
            if(SNET_CHT_POLICY_NO_MATCH_INDEX_CNS != matchIndexArr[parallelLookupInst])
            {
                break;
            }
        }

        if(SIP5_TCAM_MAX_NUM_OF_HITS_CNS == parallelLookupInst)
        {
            __LOG(("Quad lookup no match - so no action to apply \n"));
            return;
        }

        /* Match found */
        for (parallelLookupInst = 0 ;
             parallelLookupInst < SIP5_TCAM_MAX_NUM_OF_HITS_CNS ;
             parallelLookupInst++)
        {
            if(SNET_CHT_POLICY_NO_MATCH_INDEX_CNS != matchIndexArr[parallelLookupInst])
            {
                __LOG(("Match found at index[0x%x], quad lookup[%d]\n",
                       matchIndexArr[parallelLookupInst], parallelLookupInst));
            }
        }

        /* create action index valid bitmap for quad results resolution */
        lion3PclValidActionBmpGet(&matchIndexArr[0],
                                  lookUpCfg.tcamSegmentMode,
                                  &validHitBmp);
         __LOG((
                "EPCL : Action index valid bitmap for quad results resolution [0x%x],"
                "TCAM segment Mode [%d]\n"
                ,validHitBmp,lookUpCfg.tcamSegmentMode));

         maxNumOfParallelLookups = SIP5_TCAM_MAX_NUM_OF_HITS_CNS;
    }
    else
    {
        snetChtPclSrvTcamLookUp(devObjPtr, devObjPtr->pclTcamInfoPtr, &pclKey, &matchIndexArr[0]);

        if(SNET_CHT_POLICY_NO_MATCH_INDEX_CNS == matchIndexArr[0])
        {
            __LOG(("No match - so no action to apply \n"));
            return;
        }

        /* Match found */
        __LOG(("Match found at index[0x%x]\n",matchIndexArr[0]));
        maxNumOfParallelLookups = 1;
    }

    for (parallelLookupInst = 0 ;
         parallelLookupInst < maxNumOfParallelLookups ;
         parallelLookupInst++)
    {
        if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            if( 0 == (validHitBmp & (1 << parallelLookupInst)) )
            {
                continue;
            }

            memset(&lion3Action, 0, sizeof(lion3Action));
            lion3EPclActionGet(devObjPtr, descrPtr, matchIndexArr[parallelLookupInst], &lion3Action);

            __LOG(("Apply the matching action"));
            lion3EPclActionApply(devObjPtr, descrPtr, &lion3Action);

            /* Update relevant OAM descriptor fields */
            __LOG(("Update relevant OAM descriptor fields"));
            snetLion2PclOamDescriptorSet(devObjPtr, descrPtr,
                                         SMAIN_DIRECTION_EGRESS_E,
                                         &lion3Action.oamInfo);

            matchCounterEn      = lion3Action.bindToCncCounter;
            matchCounterIndex   = lion3Action.cncCounterIndex;
        }
        else
        {
            memset(&actionData, 0, sizeof(actionData));

            snetXCatEPclActionGet(devObjPtr, descrPtr, matchIndexArr[0], &actionData);
            /* Apply the matching action */
            __LOG(("Apply the matching action"));
            snetXCatEPclActionApply(devObjPtr, descrPtr, &actionData);

            matchCounterEn      = actionData.baseAction.matchCounterEn;
            matchCounterIndex   = actionData.baseAction.matchCounterIndex;

        }

        if(matchCounterEn == GT_TRUE)
        {
            /* Egress Policy CNC */
            __LOG(("Egress Policy CNC"));
            snetCht3CncCount(devObjPtr, descrPtr, SNET_LION3_CNC_EPCL_LOOKUP_GET(parallelLookupInst),
                             matchCounterIndex);
        }
    }

}

/*******************************************************************************
* snetIpclGetProperFieldValue
*
* DESCRIPTION:
*        function to allow sip5 and sip5_20 to use the same field names although
*        the order of the fields changed.
*        for sip5 device it not do any convert of the field name.
*        but for sip5_20 device it converts the field name to one of :
*           SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS names.
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       memPtr          - pointer to memory of the IPCL action.
*       matchIndex      - index of the entry in the table            --> for dump to LOG purpose only.
*       fieldName       - the name of field as in SMEM_LION3_IPCL_ACTION_TABLE_FIELDS
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
static GT_U32   snetIpclGetProperFieldValue(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    IN GT_U32                           *memPtr,
    IN GT_U32                           matchIndex,
    IN SMEM_LION3_IPCL_ACTION_TABLE_FIELDS  fieldName
)
{
    if(!SMEM_CHT_IS_SIP5_20_GET(devObjPtr))
    {
        /* no conversion needed */
        return snetFieldFromEntry_GT_U32_Get(devObjPtr,
                memPtr,
                devObjPtr->tableFormatInfo[SKERNEL_TABLE_FORMAT_IPCL_ACTION_E].formatNamePtr,
                matchIndex,
                devObjPtr->tableFormatInfo[SKERNEL_TABLE_FORMAT_IPCL_ACTION_E].fieldsInfoPtr,
                devObjPtr->tableFormatInfo[SKERNEL_TABLE_FORMAT_IPCL_ACTION_E].fieldsNamePtr,
                fieldName);
    }
    /* for sip5_20 convert from SMEM_LION3_IPCL_ACTION_TABLE_FIELDS
                           to   SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS */
    {
        GT_U32  ii;
        SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS sip5_20_fieldName = SMAIN_NOT_VALID_CNS;
        static struct {
            SMEM_LION3_IPCL_ACTION_TABLE_FIELDS     sip5Name;   /*IN*/
            SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS   sip5_20Name;/*OUT*/
        }
        ipcl_action_convert_sip5_to_sip_5_20 [] =
        {
             {SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_COMMAND                      , SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_COMMAND                     }
            ,{SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_CPU_CODE                     , SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_CPU_CODE                    }
            ,{SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_REDIRECT_COMMAND             , SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_REDIRECT_COMMAND            }
            ,{SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_POLICY1_LOOKUP_MODE          , SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_POLICY1_LOOKUP_MODE         }
            ,{SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_POLICY2_LOOKUP_MODE          , SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_POLICY2_LOOKUP_MODE         }
            ,{SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_IPCL_PROFILE_INDEX           , SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_IPCL_PROFILE_INDEX          }
            ,{SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_VIDX                         , SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_VIDX                        }
            ,{SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_IS_TRUNK                     , SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_IS_TRUNK                    }
            ,{SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_TRG_PORT                     , SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_TRG_PORT                    }
            ,{SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_TARGET_DEVICE                , SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_TARGET_DEVICE               }
            ,{SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_TRUNK_ID                     , SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_TRUNK_ID                    }
            ,{SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_USE_VIDX                     , SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_USE_VIDX                    }
            ,{SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_VNT_L2_ECHO                  , SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_VNT_L2_ECHO                 }
            ,{SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_TUNNEL_START                 , SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_TUNNEL_START                }
            ,{SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_ARP_POINTER                  , SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_ARP_POINTER                 }
            ,{SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_TUNNEL_POINTER               , SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_TUNNEL_POINTER              }
            ,{SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_TUNNEL_START_PASSENGER_TYPE  , SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_TUNNEL_START_PASSENGER_TYPE }
            ,{SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_IP_NEXT_HOP_ENTRY_INDEX      , SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_IP_NEXT_HOP_ENTRY_INDEX     }
            ,{SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_VRF_ID                       , SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_VRF_ID                      }
            ,{SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_BIND_TO_CNC_COUNTER          , SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_ACTIVATE_COUNTER            }
            ,{SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_CNC_COUNTER_INDEX            , SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_COUNTER_INDEX               }
            ,{SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_ACTIVATE_METER               , SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_ACTIVATE_METER              }
            ,{SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_BIND_TO_POLICER_COUNTER      , SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_BIND_TO_POLICER_COUNTER     }
            ,{SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_POLICER_PTR                  , SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_POLICER_PTR                 }
            ,{SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_SOURCE_ID_SET_ENABLE         , SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_SET_SST_ID                  }
            ,{SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_SOURCE_ID                    , SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_SST_ID                      }
            ,{SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_ACTION_STOP                  , SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_ACTION_STOP                 }
            ,{SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_BRIDGE_BYPASS                , SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_BRIDGE_BYPASS               }
            ,{SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_BYPASS_INGRESS_PIPE          , SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_BYPASS_INGRESS_PIPE         }
            ,{SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_VLAN_PRECEDENCE              , SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_VLAN_PRECEDENCE             }
            ,{SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_EN_NESTED_VLAN               , SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_EN_NESTED_VLAN              }
            ,{SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_VLAN_COMMAND                 , SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_VLAN_COMMAND                }
            ,{SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_VID0                         , SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_VID0                        }
            ,{SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_QOS_PROFILE_MARKING_ENABLE   , SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_QOS_PROFILE_MARKING_ENABLE  }
            ,{SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_QOS_PRECEDENCE               , SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_QOS_PRECEDENCE              }
            ,{SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_QOS_PROFILE                  , SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_QOS_PROFILE                 }
            ,{SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_MODIFY_DSCP                  , SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_MODIFY_DSCP                 }
            ,{SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_MODIFY_UP                    , SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_MODIFY_UP                   }
            ,{SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_ENABLE_MIRROR_TCP_RST_OR_FIN , SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_ENABLE_MIRROR_TCP_RST_OR_FIN}
            ,{SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_MODIFY_MAC_DA                , SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_MODIFY_MAC_DA               }
            ,{SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_MODIFY_MAC_SA                , SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_MODIFY_MAC_SA               }
            ,{SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_SET_EGRESS_FILTER_REGISTERED , SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_SET_EGRESS_FILTER_REGISTERED}
            ,{SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_VLAN1_CMD                    , SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_VLAN1_CMD                   }
            ,{SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_UP1_CMD                      , SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_UP1_CMD                     }
            ,{SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_VID1                         , SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_VID1                        }
            ,{SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_UP1                          , SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_UP1                         }
            ,{SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_FLOW_ID                      , SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_FLOW_ID                     }
            ,{SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_IPCL_RESERVED_EN             , SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_IPCL_RESERVED_EN            }
            ,{SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_IPCL_TO_TXQ_RESERVED         , SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_IPCL_TO_TXQ_RESERVED        }
            ,{SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_SET_MAC2ME                   , SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_SET_MAC2ME                  }
            ,{SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_TIMESTAMP_EN                 , SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_TIMESTAMP_EN                }
            ,{SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_OFFSET_INDEX                 , SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_OFFSET_INDEX                }
            ,{SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_OAM_PROCESSING_EN            , SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_OAM_PROCESSING_EN           }
            ,{SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_OAM_PROFILE                  , SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_OAM_PROFILE                 }
            ,{SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_ASSIGN_SRC_EPORT_EN          , SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_ASSIGN_SRC_EPORT_EN         }
            ,{SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_SRC_EPORT                    , SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_SRC_EPORT                   }
            ,{SMEM_LION3_IPCL_ACTION_TABLE_FIELDS_USER_AC_ENABLE               , SMEM_SIP5_20_IPCL_ACTION_TABLE_FIELDS_USER_AC_ENABLE              }
            ,{SMAIN_NOT_VALID_CNS , SMAIN_NOT_VALID_CNS}
        };


        for(ii = 0 ; ipcl_action_convert_sip5_to_sip_5_20[ii].sip5Name != SMAIN_NOT_VALID_CNS ; ii++)
        {
            if(ipcl_action_convert_sip5_to_sip_5_20[ii].sip5Name == fieldName)
            {
                /* found the proper convert */
                sip5_20_fieldName = ipcl_action_convert_sip5_to_sip_5_20[ii].sip5_20Name;
                break;
            }
        }

        return snetFieldFromEntry_GT_U32_Get(devObjPtr,
                memPtr,
                devObjPtr->tableFormatInfo[SKERNEL_TABLE_FORMAT_IPCL_ACTION_E].formatNamePtr,
                matchIndex,
                devObjPtr->tableFormatInfo[SKERNEL_TABLE_FORMAT_IPCL_ACTION_E].fieldsInfoPtr,
                devObjPtr->tableFormatInfo[SKERNEL_TABLE_FORMAT_IPCL_ACTION_E].fieldsNamePtr,
                sip5_20_fieldName);

    }
}



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
)
{
    if(SMEM_CHT_IS_SIP5_20_GET(devObjPtr))
    {
        LION3_TABLES_FORMAT_INIT_MAC(
            devObjPtr, SKERNEL_TABLE_FORMAT_IPCL_ACTION_E,
            sip5_20IpclActionTableFieldsFormat, sip5_20IpclActionFieldsTableNames);

        LION3_TABLES_FORMAT_INIT_MAC(
            devObjPtr, SKERNEL_TABLE_FORMAT_EPCL_ACTION_E,
            sip5_20EpclActionTableFieldsFormat, lion3EpclActionFieldsTableNames);
    }
    else
    {
        LION3_TABLES_FORMAT_INIT_MAC(
            devObjPtr, SKERNEL_TABLE_FORMAT_IPCL_ACTION_E,
            lion3IpclActionTableFieldsFormat, lion3IpclActionFieldsTableNames);

        LION3_TABLES_FORMAT_INIT_MAC(
            devObjPtr, SKERNEL_TABLE_FORMAT_EPCL_ACTION_E,
            lion3EpclActionTableFieldsFormat, lion3EpclActionFieldsTableNames);
    }

    LION3_TABLES_FORMAT_INIT_MAC(
        devObjPtr, SKERNEL_TABLE_FORMAT_IPCL_META_DATA_E,
        lion3IpclMetadataTableFieldsFormat, lion3IpclMetadataFieldsTableNames);

    LION3_TABLES_FORMAT_INIT_MAC(
        devObjPtr, SKERNEL_TABLE_FORMAT_EPCL_META_DATA_E,
        lion3EpclMetadataTableFieldsFormat, lion3EpclMetadataFieldsTableNames);
}


