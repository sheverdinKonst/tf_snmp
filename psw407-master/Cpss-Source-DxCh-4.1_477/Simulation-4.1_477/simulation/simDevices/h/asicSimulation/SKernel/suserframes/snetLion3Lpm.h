/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetLion3Lpm.h
*
* DESCRIPTION:
*       This is a external API definition for SIP5 LPM
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 6 $
*
*******************************************************************************/
#ifndef __snetLion3Lpmh
#define __snetLion3Lpmh

#include <asicSimulation/SKernel/smain/smain.h>
#include <asicSimulation/SKernel/suserframes/snetCheetah2Routing.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



typedef enum{
    LPM_BUCKET_TYPE_REGULAR_E    = 0, /* Next pointer is a pointer to another regular bucket     */
    LPM_BUCKET_TYPE_COMPRESS_1_E = 1, /* Next pointer is a pointer to one-line compressed bucket */
    LPM_BUCKET_TYPE_COMPRESS_2_E = 2, /* Next pointer is a pointer to two-line compressed bUcket */
    LPM_BUCKET_TYPE_RESERVED_E   = 3  /* Reserved bucket type*/
}LPM_BUCKET_TYPE_ENT;


typedef enum{
    LPM_POINTER_TYPE_BUCKET_E       = 0, /* Bucket       */
    LPM_POINTER_TYPE_REGULAR_LEAF_E = 1, /* Regular Leaf */
    LPM_POINTER_TYPE_ECMP_LEAF_E    = 2, /* Ecmp    Leaf */
    LPM_POINTER_TYPE_QOS_LEAF_E     = 3  /* QoS     Leaf */
}LPM_POINTER_TYPE_ENT;

/*
 * Typedef: struct SNET_LPM_MEM_BUCKET_STC
 *
 * Fields:
 *         pointToSipTrie                   : In case of (*,G) lookup, this bit indicates
 *                                            if the next bucket is a root of (S,G) lookup
 *                                            Relevant for multicast packets only
 *         bucketType                       : bucket type (see LPM_BUCKET_TYPE_ENT)
 *         twoCompressedFirstLineLastOffset : This is the last offset of the first line in
 *                                            the Two Compressed Bucket
 *                                            for deciding if to access the first or the second line.
 *                                            Relevant for compressed 2 bucket type only
*/
typedef struct {
    GT_BOOL              pointToSipTrie;
    LPM_BUCKET_TYPE_ENT  bucketType;
    GT_U8                twoCompressedFirstLineLastOffset;
} SNET_LPM_MEM_BUCKET_STC;

/*
 * Typedef: struct SNET_LPM_LTT_DATA_STC
 *
 * Fields:
 *         lttIpv6MulticastGroupScopeLevel      : Ipv6 Multicast Scope checking
 *         lttUnicastRpfCheckEnable             : If enabled the Unicast RPF CHECK performs
 *         lttUnicastSrcAddrCheckMismatchEnable : If enabled the Unicast MAC SIP/SA performs
*/
typedef struct{
    GT_U8   lttIpv6MulticastGroupScopeLevel;
    GT_BOOL lttUnicastRpfCheckEnable;
    GT_BOOL lttUnicastSrcAddrCheckMismatchEnable;
}SNET_LPM_LTT_DATA_STC;

/*
 * Typedef: struct SNET_LPM_ECMP_OR_QOS_DATA_STC
 *
 * Fields:
 *         nextHopBaseAddr : The base address of the next hops that are associated to this ECMP/QoS block
 *         numOfPaths      : Number of ECMP or QoS paths.
 *                           Require for calculating the next HOP pointer.
 *                           The real <Number Of Paths> = <Number Of Paths>+1
 *         randomEn        : Define if to do random selection of the NH
*/
typedef struct{
    GT_U16  nextHopBaseAddr;
    GT_U16  numOfPaths;
    GT_BOOL randomEn;
}SNET_LPM_ECMP_OR_QOS_DATA_STC;

/*
 * Typedef: struct SNET_LPM_MEM_LEAF_STC
 *
 * Fields:
 *         lttData      : LTT data struct
 *         activity     : activity bit is used for aging of the prefix.
 *                        It's described in SIP5 section
 *                        8.11.2 – Refreshing the prefix activity bit.
 *         ecmpOrQosData: ecmp or qos data
 *                        (relevant only if pointerType == ecmp leaf or
 *                                          pointerType == qos  leaf)
*/
typedef struct {
    SNET_LPM_LTT_DATA_STC         lttData;
    GT_BIT                        activity;
    SNET_LPM_ECMP_OR_QOS_DATA_STC ecmpOrQosData;
} SNET_LPM_MEM_LEAF_STC;

/*
 * Typedef: union SNET_LPM_MEM_ENTRY_DATA_UNT
 *
 * Fields:
 *         bucket: lpm mem entry represents bucket
 *         leaf  : lpm mem entry represents leaf
*/
typedef union {
    SNET_LPM_MEM_BUCKET_STC bucket;
    SNET_LPM_MEM_LEAF_STC   leaf;
} SNET_LPM_MEM_ENTRY_DATA_UNT;

/*
 * Typedef: struct SNET_LPM_MEM_ENTRY_STC
 *
 * Fields:
 *         pointerType: lpm mem entry pointer type (see LPM_POINTER_TYPE_ENT)
 *         nextPtr    : next pointer, used for all pointer types and bucket types
 *         data       : bucket or leaf specific data fields
*/
typedef struct {
    LPM_POINTER_TYPE_ENT        pointerType;
    GT_U32                      nextPtr;
    SNET_LPM_MEM_ENTRY_DATA_UNT data;
} SNET_LPM_MEM_ENTRY_STC;



/*******************************************************************************
*   snetLion3Lpm
*
* DESCRIPTION:
*        lion3 LPM lookup function
*
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
*
* OUTPUTS:
*        matchIndexPtr         - array to indicate dip sip match
*        ipSecurChecksInfoPtr  - routing security checks information
*
* RETURNS:
*       whether sip lookup was performed
*
*******************************************************************************/
GT_U32 snetLion3Lpm
(
    IN  SKERNEL_DEVICE_OBJECT               *devObjPtr,
    IN  SKERNEL_FRAME_CHEETAH_DESCR_STC     *descrPtr,
    OUT GT_U32                              *matchIndexPtr,
    OUT SNET_ROUTE_SECURITY_CHECKS_INFO_STC *ipSecurChecksInfoPtr
);


/*******************************************************************************
*   snetLion3LpmPrefixAdd_test
*
* DESCRIPTION:
*        lion3 LPM lookup test function
*
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
*
*******************************************************************************/
GT_VOID snetLion3LpmPrefixAdd_test
(
    IN  SKERNEL_DEVICE_OBJECT           *devObjPtr,
    IN  SKERNEL_FRAME_CHEETAH_DESCR_STC *descrPtr
);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __snetLion3Lpmh */









