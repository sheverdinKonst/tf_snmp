/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetCheetahPclSrv.h
*
* DESCRIPTION:
*       Cheetah Asic Simulation .
*       Policy Engine Service Routine.
*       header file.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 20 $
*
*******************************************************************************/
#ifndef __snetCheetahPclSrvh
#define __snetCheetahPclSrvh

#include <asicSimulation/SKernel/suserframes/snetCheetahPcl.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* the max num of bytes from start of packet that can be taken by user defined
fields into the search key of the pcl */
#define SNET_CHT_PCL_MAX_BYTE_INDEX_CNS                     (128)

/* extended -- the max num of bytes from start of packet that can be taken by user defined
fields into the search key of the pcl */
#define SNET_CHT_PCL_MAX_BYTE_INDEX_EXTENDED_CNS                     (192)

/* regular key size (in bytes) 24 bytes used for ch2,ch3,xCat, Lion.  26 - for xCat2 and above. lion3 - 30*/
#define SNET_CHT_POLICY_KEY_SIZE_BYTES_REGULAR_CNS          (30)

/* extended key size (in bytes) - length of 2 regular keys */   /*ch1 - 48 , xcat2 - 52 , lion3 - 60 */
#define SNET_CHT_POLICY_KEY_SIZE_BYTES_EXTENDED_CNS         (60)

/* triple key size (in bytes)  length of 3 regular keys  */
#define SNET_CHT_POLICY_KEY_SIZE_BYTES_TRIPLE_CNS           (80)/*ch1 - 72 , xcat2 - 78 , lion3 - 80 */

#define SNET_CHT_TCP_UDP_PORT_COMP_RANGE_CNS                (8)

/**
 * Enum: CHT_PCL_KEY_TYPE_ENT
 *
 * DESCRIPTION:
 *  PCL key type enumerator:
 *      CHT_PCL_KEY_REGULAR_E  - 24(26 xCat2 and above) bytes key size
 *      CHT_PCL_KEY_EXTENDED_E - 48(52 xCat2 and above) bytes key size
 *      CHT_PCL_KEY_TRIPLE_E   - 72(78 xCat2 and above) bytes key size
 *
 *
 *      CHT_PCL_KEY_UNLIMITED_SIZE_E - unlimited size , on any buffer
 **/
typedef enum {
    CHT_PCL_KEY_REGULAR_E = 0,
    CHT_PCL_KEY_EXTENDED_E,
    CHT_PCL_KEY_TRIPLE_E,

    CHT_PCL_KEY_10B_E,
    CHT_PCL_KEY_20B_E,
    CHT_PCL_KEY_30B_E,
    CHT_PCL_KEY_40B_E,
    CHT_PCL_KEY_50B_E,
    CHT_PCL_KEY_60B_E,
    CHT_PCL_KEY_80B_E,

    CHT_PCL_KEY_UNLIMITED_SIZE_E
} CHT_PCL_KEY_FORMAT_ENT;

#define SNET_CHT_PCL_KEY_PTR(pcl_key) \
    (((pcl_key->pclKeyFormat) != CHT_PCL_KEY_UNLIMITED_SIZE_E) ? (GT_U8*)(&pcl_key->key) :  \
                                              (pcl_key->key.unlimitedBufferPtr))

/*
    union : SNET_CHT_POLICY_KEY_UNT

    purpose: info about the policy search key

    regular - key in the regular format
    extended - key in the extended format
    triple -  key in the triple format

    unlimitedBufferPtr - pointer to unlimited buffer size
    data               - need to treat union as 32 bits array
*/
typedef union{
    GT_U8   regular[SNET_CHT_POLICY_KEY_SIZE_BYTES_REGULAR_CNS];
    GT_U8   extended[SNET_CHT_POLICY_KEY_SIZE_BYTES_EXTENDED_CNS];
    GT_U8   triple[SNET_CHT_POLICY_KEY_SIZE_BYTES_TRIPLE_CNS];

    GT_U8   *unlimitedBufferPtr;
    GT_U32  data[(SNET_CHT_POLICY_KEY_SIZE_BYTES_TRIPLE_CNS+3)/4];
}SNET_CHT_POLICY_KEY_UNT;


/**
 *  Structure : SNET_CHT_POLICY_KEY_STC
 *
 *  DESCRIPTION:
 *      Policy search key info
 *
 *      pclKeyFormat        -  PCL key format regular/extended/triple
 *
 *      updateOnlyDiff      -
 *              GT_FALSE -  the key need to be build from scratch (rebuild all)
 *                          (may be needed in cycle 1 when changing from
 *                          regular<-->extended key)
 *              GT_TRUE -   the key need to be updated only in some fields
 *                          that may be changed from cycle 0 to cycle 1
 *
 *      key                 - PCL key in the regular/extended/triple format
 *      devObjPtr           - (pointer to) device object (key is build/used for this device).
 *
**/
typedef struct SNET_CHT_POLICY_KEY_STCT{
    CHT_PCL_KEY_FORMAT_ENT  pclKeyFormat;
    GT_BOOL                 updateOnlyDiff;
    SNET_CHT_POLICY_KEY_UNT key;
    SKERNEL_DEVICE_OBJECT   *devObjPtr;
}SNET_CHT_POLICY_KEY_STC;

/*
    struct : CHT_PCL_KEY_FIELDS_INFO_STC

    purpose : hold info about a field that is part of the key

    startBitInKey - the starting bit of the field in the key (include this bit)
    endBitInKey   - the end bit of the field in the key (include this bit)
    updateOnSecondCycle - relevant only when pcl is in second cycle.
                          GT_FALSE - do not update the filed
                          GT_TRUE - update the filed with info from descriptor
    debugName - used for debugging --- internal use
*/
typedef struct{
    GT_U32                      startBitInKey;
    GT_U32                      endBitInKey;
    GT_BOOL                     updateOnSecondCycle;
    char*                       debugName;
}CHT_PCL_KEY_FIELDS_INFO_STC;

/*
    struct : CHT_PCL_EXTRA_PACKET_INFO_STC

    purpose : hold info info from the packet that is needed to pcl key build
              fields that not exists in the descriptor
*/
typedef struct CHT_PCL_EXTRA_PACKET_INFO_STCT{
    GT_BOOL         isIpV6EhExists;
    GT_BOOL         isIpHeaderOk;
    GT_BOOL         isIpV6EhHopByHop;
    GT_BOOL         isL4Valid;
    GT_BOOL         isIpv4Fragment;
    GT_U32          ipv4FragmentOffset;
    GT_BOOL         isL2Valid;
    GT_U32          ipv4HeaderInfo;
}CHT_PCL_EXTRA_PACKET_INFO_STC;


/*
    enum :  CHT_PCL_TCP_PORT_COMPARE_SIGN_ENT

    description : enum of the ingress PCL TCP/UDP port comparison OPERATOR

*/
typedef enum{
      CHT_PCL_IP_PORT_OPERATOR_INVALID_E = 0,
      CHT_PCL_IP_PORT_OPERATOR_GREATER_OR_EQUAL_E,  /* >= */
      CHT_PCL_IP_PORT_OPERATOR_LESS_OR_EQUAL_E,     /* <= */
      CHT_PCL_IP_PORT_OPERATOR_NOT_EQUAL_E,         /* != */
      CHT_PCL_IP_PORT_OPERATOR_LAST_E
}CHT_PCL_TCP_PORT_COMPARE_SIGN_ENT;

/**
 * CHT_PCL_LOOKUP_PCKT_TYPE_ENT
 *
 * DESCRIPTION:
 *
 *  Packet type involved in PCL lookup engine.
 *
 *  CHT_PCL_LOOKUP_NON_IP_TYPE_E        - The key type used for Non IP Packets in this lookup
 *  CHT_PCL_LOOKUP_IPV4_ARP_TYPE_E      - The key type used for IPv4 and ARP traffic in this lookup.
 *  CHT_PCL_LOOKUP_IPV6_TYPE_E          - The key type used for IPv6 in this lookup
 *
 **/
typedef enum {
    CHT_PCL_LOOKUP_NON_IP_TYPE_E,
    CHT_PCL_LOOKUP_IPV4_ARP_TYPE_E,
    CHT_PCL_LOOKUP_IPV6_TYPE_E,
}CHT_PCL_LOOKUP_PCKT_TYPE_ENT;

/**
 * CHT_PCL_TCAM_DATA_TYPE_ENT
 *
 * DESCRIPTION:
 *
 *  TCAM data type
 *
 *  CHT_PCL_TCAM_PATTERN_E      - TCAM data is pattern
 *  CHT_PCL_TCAM_MASK_E         - TCAM data is mask
 *
 **/
typedef enum {
    CHT_PCL_TCAM_PATTERN_E,
    CHT_PCL_TCAM_MASK_E
}CHT_PCL_TCAM_DATA_TYPE_ENT;

/*******************************************************************************
*   COMPARE_TCAM_KEY_FUNC
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
typedef GT_BOOL(* COMPARE_TCAM_KEY_FUNC)
(
    IN GT_U8 * dataPtr,
    IN GT_U8 * maskPtr,
    IN GT_U8 * keyPtr,
    IN GT_U32 compBits,

    IN GT_U32   indexInTcam,    /* for log info */
    IN GT_U32   bankIndex,      /* for log info */
    IN GT_U32   wordIndex       /* for log info */
);

/**
 * DESCRIPTION
 *
 *  TCAM entry structure
 *  Holds pointers to TCAM entry words.
 *
 *  entryPatternPtr         pointer to policy TCAM pattern word
 *  entryPatternCtrlPtr     pointer to policy TCAM pattern control word
 *  entryMaskPtr            pointer to policy TCAM pattern mask word
 *  entryMaskCtrlPtr        pointer to policy TCAM  mask control word
 *
 **/
typedef struct {
    GT_U32 * entryPatternPtr;
    GT_U32 * entryPatternCtrlPtr;
    GT_U32 * entryMaskPtr;
    GT_U32 * entryMaskCtrlPtr;
} TCAM_ENTRY_MEM_STC;

/**
 * CHT_PCL_TCAM_COMMON_DATA_STC
 *
 * DESCRIPTION:
 *
 *  TCAM common data structure.
 *
 *  bankNumbers             - bank numbers
 *  bankEntriesNum          - number of entries in bank
 *  bankEntriesStep         - step between entries in TCAM in words
 *  bankWordsNum            - bank data words number
 *  bankWordsStep           - step between words in TCAM in words
 *  bankWordDataBits        - entry word data width (TCAM words may be 32 bits or longer)
 *  bankWordCtrlBits        - entry word control width
 *  tcamCompareFuncPtr      - TCAM compare function pointer
 *  tcamEntryMemory         - TCAM entry structure
 *
 **/
typedef struct CHT_PCL_TCAM_COMMON_DATA_STCT{
    GT_U32 bankNumbers;
    GT_U32 bankEntriesNum;
    GT_U32 bankEntriesStep;
    GT_U32 bankWordsNum;
    GT_U32 bankWordsStep;
    GT_U32 bankWordDataBits;
    GT_U32 bankWordCtrlBits;
    COMPARE_TCAM_KEY_FUNC tcamCompareFuncPtr;
    TCAM_ENTRY_MEM_STC tcamEntryMemory;
}CHT_PCL_TCAM_COMMON_DATA_STC;

/*********************************************************************************
*   SNET_CHT_SRV_TCAM_MEM_INIT
*
* DESCRIPTION:
*       Initialize  memory pointers for TCAM structure
*
* INPUTS:
*       dev     - pointer to device
*       tcam    - pointer to TCAM common data structure
*
* OUTPUTS:
*       tcam    - pointer to updated TCAM common data structure
*
*
* RETURN:
*
* COMMENTS:
*       Memory pointers get their initial pointers at the beginning of TCAM lookup
*       So all pointers will point to the first word of the first entry
*
*******************************************************************************/
#define SNET_CHT_SRV_TCAM_MEM_INIT(dev, tcam) \
{ \
    (tcam)->tcamEntryMemory.entryPatternPtr = \
        smemMemGet(dev, SMEM_CHT_PCL_TCAM_TBL_MEM(dev, 0, 0)); \
    (tcam)->tcamEntryMemory.entryPatternCtrlPtr = \
        smemMemGet(dev, SMEM_CHT_PCL_TCAM_CTRL_TBL_MEM(dev, 0, 0)); \
    (tcam)->tcamEntryMemory.entryMaskPtr = \
        smemMemGet(dev, SMEM_CHT_PCL_TCAM_MASK_TBL_MEM(dev, 0, 0)); \
    (tcam)->tcamEntryMemory.entryMaskCtrlPtr = \
        smemMemGet(dev, SMEM_CHT_PCL_TCAM_CTRL_MASK_TBL_MEM(dev, 0, 0)); \
}

/*********************************************************************************
*   SNET_CHT_SRV_TCAM_MEM_GET
*
* DESCRIPTION:
*       Get TCAM memory data and control words by entry and word
*
* INPUTS:
*       tcam    - pointer to TCAM common data structure
*       type    - TCAM memory words type CHT_PCL_TCAM_PATTERN_E/CHT_PCL_TCAM_MASK_E
*       entry   - TCAM entry index
*       word    - TCAM word index
*
* OUTPUTS:
*       data    - TCAM entry data word
*       control - TCAM entry control word
*
* RETURN:
*
* COMMENTS:
*       Memory pointers get their initial pointers at the beginning of TCAM lookup
*       So all pointers will point to the first word of the first entry
*
*******************************************************************************/
#define SNET_CHT_SRV_TCAM_MEM_GET(tcam, type, entry, word, data, control) \
{ \
    GT_U32 __words_offset = ((tcam)->bankEntriesStep) * entry + (tcam)->bankWordsStep * word;\
    if ((type) == CHT_PCL_TCAM_PATTERN_E) \
    { \
        data    = (tcam)->tcamEntryMemory.entryPatternPtr[__words_offset]; \
        control = (tcam)->tcamEntryMemory.entryPatternCtrlPtr[__words_offset]; \
    } \
    else if ((type) == CHT_PCL_TCAM_MASK_E)\
    { \
        data    = (tcam)->tcamEntryMemory.entryMaskPtr[__words_offset]; \
        control = (tcam)->tcamEntryMemory.entryMaskCtrlPtr[__words_offset]; \
    } \
    else \
    { \
        return GT_FAIL; \
    } \
}

/*******************************************************************************
*   SNET_CHT_SRV_TCAM_PATTERN_MASK_INIT
*
* DESCRIPTION:
*       Set initial pattern and mask values for TCAM lookup
*
* INPUTS:
*       key     - PCL search key
*       bank    - entry bank index
*       pattern - entry data pattern
*       mask    - entry data mask
*
* OUTPUTS:
*       pattern - pattern bitmap
*       mask    - mask bitmap
*
*
* RETURN:
*
* COMMENTS:
*       Each bit of the mask corresponds to set of banks for specific PCL search
*       rule format
*
*******************************************************************************/
#define  SNET_CHT_SRV_TCAM_PATTERN_MASK_INIT(key, bank, pattern, mask) \
{ \
    if ((key)->pclKeyFormat == CHT_PCL_KEY_REGULAR_E) \
    { \
        pattern = 0; \
        mask    = (1 << (bank)); \
    } \
    else if (((key)->pclKeyFormat == CHT_PCL_KEY_EXTENDED_E) \
             && ((bank & 1) == 0)) \
    { \
        pattern = 0; \
        mask    = (3 << (bank)); \
    } \
    else if (((key)->pclKeyFormat == CHT_PCL_KEY_TRIPLE_E) \
             && (bank == 0)) \
    { \
        pattern = 0; \
        mask    = 7; \
    } \
}
                    /* Set bank pattern */

/*******************************************************************************
*   SNET_CHT_SRV_TCAM_PATTERN_MASK_CHECK
*
* DESCRIPTION:
*       Set bank pattern value and compare with mask
*
* INPUTS:
*       bank    - entry bank index
*       pattern - entry data pattern
*       mask    - entry data mask
*
* OUTPUTS:
*       pattern - pattern bitmap
*       status  - GT_OK when full match
*
* RETURN:
*
* COMMENTS:
*       Each bit of the mask corresponds to set of banks for specific PCL search
*       rule format
*
*******************************************************************************/
#define  SNET_CHT_SRV_TCAM_PATTERN_MASK_CHECK(bank, pattern, mask,status) \
{ \
    SMEM_U32_SET_FIELD(pattern, bank, 1, 1); \
    if (pattern == mask) \
        status = GT_OK; \
    else status = GT_FAIL;\
}


#define SNET_CHT_SRV_TCAM_CTRL_COMP_MODE(pclKeyFormat) \
    (pclKeyFormat == CHT_PCL_KEY_REGULAR_E) ? 1 : \
    (pclKeyFormat == CHT_PCL_KEY_EXTENDED_E) ? 2 : \
    (pclKeyFormat == CHT_PCL_KEY_TRIPLE_E) ? 3 : 0


/* DXCH3 devices keeps X/Y tcam data format */
/* (0,0) and (1,1) mean "match any value", (0,1) - "match 0", (1,0) - "match 1" */
/* such conversions will be provided for one-to-one mapping */
/* mask | pattern | X | Y | */
/*  0   | 0       | 0 | 0 | */
/*  0   | 1       | 1 | 1 | */

/*  1   | 0       | 0 | 1 | */
/*  1   | 1       | 1 | 0 | */
/* macros taken from <cpss\dxCh\dxChxGen\pcl\private\prvCpssDxChPcl.h>*/

/* mask and pattern -> X */
#define SNET_CHT_SRV_TCAM_MP_TO_X(_m, _p) (_p)

/* mask and pattern -> Y */
#define SNET_CHT_SRV_TCAM_MP_TO_Y(_m, _p) (_m ^ _p)

/* XY-> mask */
#define SNET_CHT_SRV_TCAM_XY_TO_M(_x, _y) (_x ^ _y)

/* XY-> pattern */
#define SNET_CHT_SRV_TCAM_XY_TO_P(_x, _y) (_x)


/***************************************************************************
* snetChtPclSrvTcamLookUp
*
* DESCRIPTION:
*        TCAM lookup
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       tcamCommonDataPtr   - pointer to TCAM common data structure.
*       pclKeyPtr           - pointer to PCL key.
*
* OUTPUTS:
*       matchIndexPtr   - pointer to the matching index.
*
* RETURN:
*       GT_OK   - operation successful
*       GT_FAIL - operation failed
*
* COMMENTS:
*       Policy Tcam Table :
*       The policy TCAM holds rules data + mask and used for ingress and egress PCL.
*
*
***************************************************************************/
extern GT_STATUS snetChtPclSrvTcamLookUp
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN CHT_PCL_TCAM_COMMON_DATA_STC * tcamCommonDataPtr,
    IN SNET_CHT_POLICY_KEY_STC * pclKeyPtr,
    OUT GT_U32 * matchIndexPtr
);

/*******************************************************************************
*   snetChtPclKeyFieldBuildByValue
*
* DESCRIPTION:
*       function inserts data of the field to the search key
*       in specific place in key
*       used for "numeric" fields - up to GT_U32
*       in Little Endian order (PP order)
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
extern GT_VOID snetChtPclSrvKeyFieldBuildByValue
(
    INOUT SNET_CHT_POLICY_KEY_STC           *pclKeyPtr,
    IN GT_U32                                fieldVal,
    IN CHT_PCL_KEY_FIELDS_INFO_STC          *fieldInfoPtr
);


/*******************************************************************************
*   snetChtPclSrvKeyFieldBuildByPointer
*
* DESCRIPTION:
*       function insert data of field to the search key in specific place in key
*       in Little Endian order (PP order)
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
extern GT_VOID snetChtPclSrvKeyFieldBuildByPointer
(
    INOUT SNET_CHT_POLICY_KEY_STC           *pclKeyPtr,
    IN GT_U8                                *fieldValPtr,
    IN CHT_PCL_KEY_FIELDS_INFO_STC          *fieldInfoPtr
);

/*******************************************************************************
*   snetChtPclSrvKeyFieldBuildByU32Pointer
*
* DESCRIPTION:
*       function insert data of field to the search key in specific place in key
*       in Little Endian order (PP order)
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
extern GT_VOID snetChtPclSrvKeyFieldBuildByU32Pointer
(
    INOUT SNET_CHT_POLICY_KEY_STC           *pclKeyPtr,
    IN GT_U32                               *fieldValPtr,
    IN CHT_PCL_KEY_FIELDS_INFO_STC          *fieldInfoPtr
);

/*******************************************************************************
*   snetChtPclParseExtraData
*
* DESCRIPTION:
*        Parse the packet for extra data needed for PCL key creation
*
* INPUTS:
*       devObjPtr    - (pointer to) device object.
*       descrPtr     - (pointer to) frame data buffer Id
*
* OUTPUTS:
*       pclExtraDataPtr - (pointer to) pcl extra info (cookie)
*
* RETURN:
*
* COMMENTS:
*         isL2Valid is a field added for cheetah2 . It is relevant with
*           LTT packets.
*
*******************************************************************************/
extern GT_VOID snetChtPclSrvParseExtraData
(
    IN SKERNEL_DEVICE_OBJECT                *devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC      *descrPtr,
    OUT CHT_PCL_EXTRA_PACKET_INFO_STC       *pclExtraDataPtr
);

/*******************************************************************************
*   snetChtPclUserDefinedByteGet
*
* DESCRIPTION:
*       function get from the packet the user defined byte info
*
* INPUTS:
*       devObjPtr    - (pointer to) device object.
*       descrPtr     - (pointer to) frame data buffer Id
*       userDefinedAnchor - user defined byte Anchor
*       userDefinedOffset - user defined byte offset from Anchor
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
extern GT_STATUS snetChtPclUserDefinedByteGet
(
    IN SKERNEL_DEVICE_OBJECT            *devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  *descrPtr,
    IN GT_U32                           userDefinedAnchor,
    IN GT_U32                           userDefinedOffset,
    OUT GT_U8                           *userDefinedByteValuePtr
);

/*******************************************************************************
*   snetChtIPclTcpUdpPortRangeCompareGet
*
* DESCRIPTION:
*        Build TCP/UDP comparator value for PCL search key.
*
* INPUTS:
*       devObjPtr            - pointer to device object.
*       descrPtr             - pointer to frame descriptor.
*
* OUTPUTS:
*       byteCompareVal    - pointer to the TCP/UDP port comparator value.
*
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
extern GT_STATUS snetChtIPclTcpUdpPortRangeCompareGet
(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr,
    OUT GT_U8                           * byteCompareVal
);

/*******************************************************************************
*   snetCht2EPclTcpUdpPortRangeCompareGet
*
* DESCRIPTION:
*        Build TCP/UDP comparator value for EPCL search key - EPCL - from ch2
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - pointer to frame descriptor.
*
* OUTPUTS:
*       byteCompareVal    - pointer to the TCP/UDP port comparator value.
*
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
extern GT_STATUS snetCht2EPclTcpUdpPortRangeCompareGet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr,
    OUT GT_U8                           * byteCompareVal
);

/*******************************************************************************
*   snetChtPclSrvKeyFieldBuildByPointerNetworkOrder
*
* DESCRIPTION:
*       function insert data of field to the search key in specific place in key
*       in network order
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
extern GT_VOID snetChtPclSrvKeyFieldBuildByPointerNetworkOrder
(
    INOUT SNET_CHT_POLICY_KEY_STC       *pclKeyPtr,
    IN GT_U8                                *fieldValPtr,
    IN CHT_PCL_KEY_FIELDS_INFO_STC          *fieldInfoPtr
);

/*******************************************************************************
*   snetChtPclSrvKeyFieldBuildByValueNetworkOrder
*
* DESCRIPTION:
*       function inserts data of the field to the search key
*       in specific place in key
*       used for "numeric" fields - up to GT_U32
*       in network order
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
extern GT_VOID snetChtPclSrvKeyFieldBuildByValueNetworkOrder
(
    INOUT SNET_CHT_POLICY_KEY_STC           *pclKeyPtr,
    IN GT_U32                                fieldVal,
    IN CHT_PCL_KEY_FIELDS_INFO_STC          *fieldInfoPtr
);

/*******************************************************************************
*   snetChtPclSrvGenericTcamLookup
*
* DESCRIPTION:
*       generic function that do lookup in tcam for given key and return the
*       matching index.
*
* INPUTS:
*       devObjPtr - (pointer to) the device object
*       lookUpKeyPtr - (pointer to) current lookup key
*
* OUTPUTS:
*       matchIndexPtr - (pointer to) the match index
*                   if no match - return value SNET_CHT_POLICY_NO_MATCH_INDEX_CNS
*
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
void snetChtPclSrvGenericTcamLookup
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_DEVICE_TCAM_INFO_STC *tcamInfoPtr,
    IN SNET_CHT_POLICY_KEY_STC *lookUpKeyPtr,
    OUT GT_U32  *matchIndexPtr
);

/*******************************************************************************
* snetLion2TtiTrillAdjacencyLookUpKeyWordBuild
*
* DESCRIPTION:
*       call back function for tcamInfoPtr->lookUpKeyWordBuildPtr
*        Lion2 TRILL tcam build of lookup key word according to the key and the
*        word index to build
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - pointer to frame data buffer Id
*
* OUTPUTS:
*       ttiKeyTypePtr  - pointer to tti key type
*
*
* RETURN:
*       GT_BOOL TRUE DOING T.T false
*
* COMMENTS:
*
*
*******************************************************************************/
GT_U32 snetLion2TtiTrillAdjacencyLookUpKeyWordBuild
(
    IN SKERNEL_DEVICE_OBJECT        *devObjPtr,
    IN SKERNEL_DEVICE_TCAM_INFO_STC *tcamInfoPtr,
    IN SNET_CHT_POLICY_KEY_STC      *lookUpKeyPtr,
    IN GT_U32 bankIndex,
    IN GT_U32 wordIndex
);

/*******************************************************************************
*   snetChtPclKeyFieldPut
*
* DESCRIPTION:
*       function inserts data of the field to the search key
*       in specified place in key
*       used for "numeric" fields - up to GT_U32
*
* INPUTS:
*       bytesKeyPtr  - (pointer to) current pcl key
*       endBit       - field MSB position
*       startBit     - field LSB position
*       fieldVal     - data of field to insert to key
*
* OUTPUTS:
*       wordsKeyPtr - (pointer to) PCL field
*
* RETURN:
*
* COMMENTS:
*     The simulated key consists of 32 bit words that
*     compared "anded" and "compared" with TCAM read-only
*     pattern and mask direct access registers. There is no difference
*     Big or Little endian CPU executes this code, but for PC
*     the bytes will be seen swapped under debugger
*
*******************************************************************************/
GT_VOID snetChtPclKeyFieldPut
(
    IN SKERNEL_DEVICE_OBJECT   *devObjPtr,
    INOUT GT_U8                *bytesKeyPtr,
    IN GT_U32                   endBit,
    IN GT_U32                   startBit,
    IN GT_U32                   fieldVal
);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __snetCheetahPclSrvh */


