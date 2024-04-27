/*******************************************************************************
*                   Copyright 2002, GALILEO TECHNOLOGY, LTD.
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL.
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.
*
* MARVELL COMPRISES MARVELL TECHNOLOGY GROUP LTD. (MTGL) AND ITS SUBSIDIARIES,
* MARVELL INTERNATIONAL LTD. (MIL), MARVELL TECHNOLOGY, INC. (MTI), MARVELL
* SEMICONDUCTOR, INC. (MSI), MARVELL ASIA PTE LTD. (MAPL), MARVELL JAPAN K.K.
* (MJKK), GALILEO TECHNOLOGY LTD. (GTL) AND GALILEO TECHNOLOGY, INC.(GTI).
********************************************************************************
* smacHash.h
*
* DESCRIPTION:
*       Hash calculate for MAC address table definition.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 6 $
*
*******************************************************************************/

#ifndef __smacHashh
#define __smacHashh

#include <os/simTypes.h>

/*
 * typedef: enum GT_MAC_TBL_SIZE
 *
 * Description: Enumeration of MAC Table size in entries
 *
 * Enumerations:
 *   GT_MAC_TBL_4K  -  4K Mac Address table entries.
 *   GT_MAC_TBL_8K  -  8K Mac Address table entries.
 *   GT_MAC_TBL_16K - 16K Mac Address table entries.
 */
typedef enum
{
    SGT_MAC_TBL_8K = 0,
    SGT_MAC_TBL_16K,
    SGT_MAC_TBL_4K
} SGT_MAC_TBL_SIZE;

/*
 * typedef: enum GT_MAC_VL
 *
 * Description: Enumeration of VLAN Learning modes
 *
 * Enumerations:
 *      GT_IVL - Independent VLAN Learning
 *      GT_SVL - Shared VLAN Learning
 */
typedef enum
{
    GT_IVL=1,
    GT_SVL
} GT_MAC_VL;


/*
 * typedef: enum GT_ADDR_LOOKUP_MODE
 *
 * Description: Enumeration of Address lookup modes
 *
 * Enumerations:
 *      GT_MAC_SQN_VLAN_SQN - Optimized for sequential MAC addresses and
 *                            sequential VLAN id's.
 *      GT_MAC_RND_VLAN_SQN - Optimized for random MAC addresses and
 *                            sequential VLAN id's. Reserved.
 *      GT_MAC_SQN_VLAN_RND - Optimized for sequential MAC addresses and
 *                            random VLAN id's.  Reserved.
 *      GT_MAC_RND_VLAN_RND - Optimized for random MAC addresses and
 *                            random VLAN id's.
 */
typedef enum
{
    GT_MAC_SQN_VLAN_SQN = 0,
    GT_MAC_RND_VLAN_SQN,
    GT_MAC_SQN_VLAN_RND,
    GT_MAC_RND_VLAN_RND

} GT_ADDR_LOOKUP_MODE;

/*
 * typedef: enum GT_MAC_HASH_KIND
 *
 * Description: Enumeration of Mac hash function kind,
 *
 * Enumerations:
 *   GT_OLD_MAC_HASH_FUNCTION - old hash function.
 *   GT_NEW_MAC_HASH_FUNCTION - improved hash function..
 */
typedef enum
{
    GT_OLD_MAC_HASH_FUNCTION = 0,
    GT_NEW_MAC_HASH_FUNCTION = 1
} GT_MAC_HASH_KIND;


/*
 * Typedef: SGT_MAC_HASH
 *
 * Description: struct contains the hardware parameters for hash mac Address
 *              calculates.
 *
 * Fields:
 *       macMask     - the MAC lookup mask - indicates the bits in the addr that
 *                     are used in the MAC table lookup.
 *                     the array length need to be 6. (48 bits)
 *       vlanMode    - the VLAN lookup mode.
 *       vidShift    - the Vid lookup cyclic shift left.
 *                     the vidShift range is 0-2.
 *       vidMask     - the Vid lookup mask - indicates the bit in the vid that
 *                     are used in the MAC table lookup.
 *                     the vidMask is 12 bit.
 *       macShift    - the Vid lookup cyclic shift left.
 *                     the macShift range is 0-5.
 *       addressMode - the address lookup mode.
 *       size        - the entries number in the hash table.
 *       macChainLen - mac hash chain length.
 */
typedef struct
{
    GT_ETHERADDR        macMask;
    GT_MAC_VL           vlanMode;
    GT_U8               vidShift;
    GT_U16              vidMask;
    GT_U8               macShift;
    GT_ADDR_LOOKUP_MODE addressMode;
    SGT_MAC_TBL_SIZE    size;
    GT_U32              macChainLen;
    GT_MAC_HASH_KIND    macHashKind;
} SGT_MAC_HASH;


/*******************************************************************************
* sgtMacHashCalc
*
* DESCRIPTION:
*       This function calculates the hash index for the mac address table.
*       for specific mac address and VLAN id.
*
* INPUTS:
*       addr        - the mac address.
*       vid         - the VLAN id.
*       macHashStructPtr - mac hash parameters
* OUTPUTS:
*       hash - the hash index.
*
* RETURNS:
*       GT_OK        - on success
*       GT_FAIL      - on error
*
* COMMENTS:
*       None.
*
*******************************************************************************/
GT_STATUS sgtMacHashCalc
(
    IN  GT_ETHERADDR   	*	addr,
    IN  GT_U16          	vid,
    IN  SGT_MAC_HASH 	*   macHashStructPtr,
    OUT GT_U32          *	hash
);

#endif /* __macHashh */
