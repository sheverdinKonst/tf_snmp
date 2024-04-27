
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
* macHashDx.c
*
* DESCRIPTION:
*       Hash calculate for MAC address table implementation for Salsa.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 7 $
*
*******************************************************************************/
#include <common/Utils/PresteraHash/smacHash.h>
#include <common/Utils/PresteraHash/smacHashDx.h>

/*******************************************************************************
* salsaMacHashCalc
*
* DESCRIPTION:
*       This function calculates the hash index for the mac address table.
*       for specific mac address and VLAN id.
*
* INPUTS:
*       addr        - the mac address.
*       vid         - the VLAN id.
*       vlanMode    - 1 means IVL (vlan included to hash calculation), 
*                     0 means SVL
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
GT_STATUS salsaMacHashCalc
(
    IN  GT_ETHERADDR    *addr,
    IN  GT_U16          vid,
    IN  GT_U8           vlanMode,
    OUT GT_U32          *hash
)
{
    GT_U32 u32Vid;             /* find the vid for lookup */
    GT_U32 transposedHash;     /* contains the ttransposed hash value */
    GT_U32 u32Array[7];
    GT_U32 numOfWords, idx;
    GT_U8  *arEther;

    if(NULL == addr|| NULL == hash)
    {
        return GT_BAD_PARAM;
    }

    arEther = addr->arEther;

    /* Note bits over bit 10 will be ignored in the transpose phase */
    u32Array[0] = arEther[5] | (arEther[4] << 8);
    u32Array[1] = (arEther[4] >> 3) | (arEther[3] << 5);
    u32Array[2] = (arEther[3] >> 6) | (arEther[2] << 2) | (arEther[1] << 10);
    u32Array[3] = (arEther[1] >> 1) | (arEther[0] << 7);
    u32Array[4] = (arEther[0] >> 4);

    switch (vlanMode)
    {
        case 1:
/*            u32Vid = vid & macHashStruct.vidMask; */
            u32Vid = vid;
            u32Array[4] |= u32Vid & 0x7F0;
            u32Array[5] =  u32Vid & 0xF;
            u32Array[6] =  (u32Vid >> 11) & 1; /* Maybe vidMask was all 1s */
            numOfWords = 7;
            break;
        case 0:
            /* Shared Vlan Learning */
            numOfWords = 5;
            break;
        default:
            return GT_BAD_PARAM;
    }

    /* compute the transposed hash */
    transposedHash = u32Array[0];
    for (idx = 1; idx < numOfWords; idx++)
    {
        transposedHash ^= u32Array[idx];
    }

    /* transpose the hashed value */
    *hash  = (transposedHash & (1 <<  0)) << 10;
    *hash |= (transposedHash & (1 <<  1)) <<  8;
    *hash |= (transposedHash & (1 <<  2)) <<  6;
    *hash |= (transposedHash & (1 <<  3)) <<  4;
    *hash |= (transposedHash & (1 <<  4)) <<  2;
    *hash |= (transposedHash & (1 <<  5)) <<  0;
    *hash |= (transposedHash & (1 <<  6)) >>  2;
    *hash |= (transposedHash & (1 <<  7)) >>  4;
    *hash |= (transposedHash & (1 <<  8)) >>  6;
    *hash |= (transposedHash & (1 <<  9)) >>  8;
    *hash |= (transposedHash & (1 << 10)) >> 10;

    *hash <<= 1;
    return GT_OK ;
}
/*******************************************************************************
* salsa2MacHashCalc
*
* DESCRIPTION:
*       This function calculates the hash index for the mac address table.
*       for specific mac address and VLAN id.
*
* INPUTS:
*       addr        - the mac address.
*       vid         - the VLAN id.
*       vlanMode    - 1 means IVL (vlan included to hash calculation), 
*                     0 means SVL
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
GT_STATUS salsa2MacHashCalc
(
    IN  GT_ETHERADDR    *addr,
    IN  GT_U16          vid,
    IN  GT_U8           vlanMode,
    OUT GT_U32          *hash
)
{
    GT_U32 u32Array[5];
    GT_U32 idx;
    GT_U8  *arEther;
    GT_U32 transposedHash;     /* contains the transposed hash value */
    GT_U32 numOfWords;    
    
    if(NULL == addr|| NULL == hash)
    {
      return GT_BAD_PARAM;
    }
    
    /*       
    MAC[0:11] XOR MAC[12:23] XOR MAC[24:35] XOR MAC[36:47] XOR VID[0:11] 
    */
    memset(u32Array, 0, sizeof(u32Array));
    
    arEther = addr->arEther;
    
    u32Array[0] = (arEther[5]) | ((arEther[4] & 0x0F) << 8);      /* mac 00..11 */
    u32Array[1] = ((arEther[4] & 0xF0) >> 4) | (arEther[3] << 4); /* mac 12..23 */
    u32Array[2] = (arEther[2]) | ((arEther[1] & 0x0F) << 8);      /* mac 24..35 */
    u32Array[3] = ((arEther[1] & 0xF0) >> 4) | (arEther[0] << 4); /* mac 36..47 */
            
    switch (vlanMode)
    {
        case GT_IVL:
            /* Individual Vlan Learning */
            u32Array[4] = vid & 0xFFF;
            numOfWords = 5;
            break;
        case GT_SVL:
            /* Shared Vlan Learning */
            numOfWords = 4;
            break;
        default:
            return GT_BAD_PARAM;
    }

    /* compute the transposed hash */
    transposedHash = u32Array[0];
    for (idx = 1; idx < numOfWords; idx++)
    {
        transposedHash ^= u32Array[idx];
    }
    
    /* transpose the hashed value */
    *hash  = (transposedHash & (0x001)) << 11; /* bit 0  */
    *hash |= (transposedHash & (0x002)) <<  9; /* bit 1  */
    *hash |= (transposedHash & (0x004)) <<  7; /* bit 2  */
    *hash |= (transposedHash & (0x008)) <<  5; /* bit 3  */
    *hash |= (transposedHash & (0x010)) <<  3; /* bit 4  */
    *hash |= (transposedHash & (0x020)) <<  1; /* bit 5  */
    *hash |= (transposedHash & (0x040)) >>  1; /* bit 6  */
    *hash |= (transposedHash & (0x080)) >>  3; /* bit 7  */
    *hash |= (transposedHash & (0x100)) >>  5; /* bit 8  */
    *hash |= (transposedHash & (0x200)) >>  7; /* bit 9  */
    *hash |= (transposedHash & (0x400)) >>  9; /* bit 10 */
    *hash |= (transposedHash & (0x800)) >> 11; /* bit 11 */
    *hash *= 2;
    
    return GT_OK ;
}

