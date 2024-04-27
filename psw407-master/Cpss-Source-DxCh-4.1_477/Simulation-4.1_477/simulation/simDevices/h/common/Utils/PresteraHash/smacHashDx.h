/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* macHashDx.h
*
* DESCRIPTION:
*       Hash calculate for MAC address table implementation for Salsa.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 8 $
*
*******************************************************************************/
#ifndef __smacHashDxh
#define __smacHashDxh

#include <os/simTypes.h>
#include <asicSimulation/SKernel/smain/smain.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum {
    SNET_CHEETAH_FDB_ENTRY_MAC_E = 0,
    SNET_CHEETAH_FDB_ENTRY_IPV4_IPMC_BRIDGING_E,/*IPv4 Multicast address entry (IGMP snooping);*/
    SNET_CHEETAH_FDB_ENTRY_IPV6_IPMC_BRIDGING_E,/*IPv6 Multicast address entry (MLD snooping);*/

    /* SIP5 new types */
    SNET_CHEETAH_FDB_ENTRY_IPV4_UC_ROUTING_E,/*Unicast routing IPv4 address entry.*/
    SNET_CHEETAH_FDB_ENTRY_IPV6_UC_ROUTING_KEY_E,/*Unicast routing IPv6 address lookup-key entry.*/
    SNET_CHEETAH_FDB_ENTRY_IPV6_UC_ROUTING_DATA_E,/*Unicast routing IPv6 address data entry.*/
    SNET_CHEETAH_FDB_ENTRY_FCOE_UC_ROUTING_E,/*Unicast routing FCOE address entry.*/

} SNET_CHEETAH_FDB_ENTRY_ENT;

typedef enum {
    SNET_CHEETAH_FDB_CRC_HASH_UPPER_BITS_MODE_ALL_ZERO_E,/* use 16 upper bit 64..80 as 0 */
    SNET_CHEETAH_FDB_CRC_HASH_UPPER_BITS_MODE_FID_E,     /* use 16 upper bit 64..80 as FID */
    SNET_CHEETAH_FDB_CRC_HASH_UPPER_BITS_MODE_MAC_E      /* use 16 upper bit 64..80 as MAC LSBits */
}SNET_CHEETAH_FDB_CRC_HASH_UPPER_BITS_MODE_ENT;

typedef struct{
    SNET_CHEETAH_FDB_ENTRY_ENT     entryType;

    /*common to most entry types, but not relevant for UC routing */
    GT_U16  origFid;/* the fid may be modified due to fid16BitHashEn , but this value should hold original fid  */
    GT_U16  fid;/*vlanId - common to most entry types */
    GT_BIT  fdbLookupKeyMode;   /* indicates FDB lookup mode: single tag FDB lookup or double tag FDB lookup (sip5_10)*/
    GT_U32  vid1; /* vid1 - used for double tag FDB Lookup (sip5_10)*/
    GT_BIT  fid16BitHashEn;/* indication if FID used 12 or 16 bits*/
    GT_BIT  isSip5;  /* use SIP5 algorithm for HASH function calculation */

    union{
        struct{
            GT_U8       macAddr[6];/*source / destination mac address */
            SNET_CHEETAH_FDB_CRC_HASH_UPPER_BITS_MODE_ENT crcHashUpperBitsMode;/* 16 MSbits mode for of DATA into the hash function */
        }macInfo;  /*SNET_CHEETAH_FDB_ENTRY_MAC_E*/

        struct{
            GT_U32      sip;/*source IP*/
            GT_U32      dip;/*destination IP*/
        }ipmcBridge;/*SNET_CHEETAH_FDB_ENTRY_IPV4_IPMC_BRIDGING_E , SNET_CHEETAH_FDB_ENTRY_IPV6_IPMC_BRIDGING_E */

        struct{
            GT_U32  vrfId;   /*vrf Id*/
            GT_U32  dip[4];  /*destination IP, for ipv4 and fcoe dip[0] only used*/
        }ucRouting; /* SNET_CHEETAH_FDB_ENTRY_IPV4_UC_ROUTING_E
                       SNET_CHEETAH_FDB_ENTRY_IPV6_UC_ROUTING_KEY_E
                       SNET_CHEETAH_FDB_ENTRY_FCOE_UC_ROUTING_E */
    }info;
}SNET_CHEETAH_FDB_ENTRY_HASH_INFO_STC;

/* macro to get bit from the array of words */
#define _D(bit) (D[(bit)>>5] >> ((bit)&0x1f))

GT_STATUS salsaMacHashCalc
(
    IN  GT_ETHERADDR    *addr,
    IN  GT_U16          vid,
    IN  GT_U8           vlanMode,
    OUT GT_U32          *hash
);

GT_STATUS salsa2MacHashCalc
(
    IN  GT_ETHERADDR    *addr,
    IN  GT_U16          vid,
    IN  GT_U8           vlanMode,
    OUT GT_U32          *hash
);

/*******************************************************************************
* cheetahMacHashCalc
*
* DESCRIPTION:
*       This function prepares MAC+VID data for calculating 12-bit hash
*
* INPUTS:
*       devObjPtr      - (pointer to) device object
*       hashType       - 1 - CRC , or 0- XOR
*       vlanLookupMode - 0 - without VLAN,
*                        1 - vlan + mac - relevant only for MAC based HASH
*       fdbEntryType   - 0 - MAC, 1 - IPv4 IPM, 2 - IPv6 IPM
*       macAddrPtr     - MAC address
*       fid            - Forwarding id
*       sourceIpAddr   - source IP
*       destIpAddr     - destination IP
*       numBitsToUse   - number of bits used to address FDB mac entry in FDB table,
*                        depends on FDB size (11 - 8K entries, 12 - 16K entries,
*                        13 - 32K entries)
*
* OUTPUTS:
*       none
*
* RETURNS:
*       hash value
*
* COMMENTS:
*       for CRC used polinom X^12+X^11+X^3+X^2+X+1 i.e 0xF01.
*
*******************************************************************************/
GT_U32 cheetahMacHashCalc
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32  hashType,
    IN GT_U32  vlanLookupMode,
    IN GT_U32  fdbEntryType,
    IN GT_U8   *macAddrPtr,
    IN GT_U32  fid,
    IN GT_U32  sourceIpAddr,
    IN GT_U32  destIpAddr,
    IN GT_U32  numBitsToUse
);

/*******************************************************************************
* cheetahMacHashCalcByStc
*
* DESCRIPTION:
*       This function prepares MAC+VID data for calculating (info by structure).
*
* INPUTS:
*       hashType       - 1 - CRC , or 0- XOR
*       vlanLookupMode - 0 - without VLAN,
*                        1 - vlan + mac - relevant only for MAC based HASH
*       entryInfoPtr    - (pointer to) entry hash info
*       numBitsToUse   - number of bits used to address FDB mac entry in FDB table,
*                        depends on FDB size (11 - 8K entries, 12 - 16K entries,
*                        13 - 32K entries)
*
* OUTPUTS:
*       none
*
* RETURNS:
*       hash value
*
* COMMENTS:
*       for CRC used polinom X^12+X^11+X^3+X^2+X+1 i.e 0xF01.
*
*******************************************************************************/
GT_U32 cheetahMacHashCalcByStc
(
    IN GT_U32  hashType,
    IN GT_U32  vlanLookupMode,
    IN SNET_CHEETAH_FDB_ENTRY_HASH_INFO_STC *entryInfoPtr,
    IN GT_U32  numBitsToUse
);


/*******************************************************************************
* sip5MacHashCalcMultiHash
*
* DESCRIPTION:
*       This function calculate <numBitsToUse> hash , for multi hash results
*           according to  numOfHashToCalc.
*
* INPUTS:
*       vlanLookupMode - 0 - without VLAN,
*                        1 - vlan + mac - relevant only for MAC based HASH
*       fdbEntryType   - 0 - MAC, 1 - IPv4 IPM, 2 - IPv6 IPM
*       fid16BitHashEn - indication that 16 bits of FID are used
*       entryInfoPtr    - (pointer to) entry hash info
*       numBitsToUse   - number of bits used to address FDB mac entry in FDB table,
*                        depends on FDB size (11 - 8K entries, 12 - 16K entries,
*                        13 - 32K entries)
*       numOfHashToCalc - the number of hash functions to generate
*                           (also it is the number of elements in calculatedHashArr[])
* OUTPUTS:
*       calculatedHashArr[] - array of calculated hash by the different functions
*
* RETURNS:
*       None
*
* COMMENTS:
*
*
*******************************************************************************/
void sip5MacHashCalcMultiHash
(
    IN GT_U32  hashType,
    IN GT_U32  vlanLookupMode,
    IN SNET_CHEETAH_FDB_ENTRY_HASH_INFO_STC *entryInfoPtr,
    IN GT_U32  numBitsToUse,
    IN GT_U32  numOfHashToCalc,
    OUT GT_U32 calculatedHashArr[]
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __smacHashDxh */


