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
* smacHashSoho.c
*
* DESCRIPTION:
*       Hash calculate for MAC address table implementation for Soho.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 3 $
*
*******************************************************************************/
#include <common/Utils/SohoHash/smacHashSoho.h>



GT_U16 hashToBucket(GT_U16 hash, GT_U16 mode)
{
        GT_U16 bucket;

        switch (mode)
        {
                case 0:
                        bucket =
                        (((hash >>  7) & 1) << 10)|                /* bit 10 */
                        (((hash >> 11) & 1) << 9) |                /* bit 9 */
                        (((hash >> 3 ) & 1) << 8) |                /* bit 8 */
                        (((hash >> 14) & 1) << 7) |                /* bit 7 */
                        (((hash >> 12) & 1) << 6) |                /* bit 6 */
                        (((hash >> 10) & 1) << 5) |                /* bit 5 */
                        (((hash >> 8 ) & 1) << 4) |                /* bit 4 */
                        (((hash >> 6 ) & 1) << 3) |                /* bit 3 */
                        (((hash >> 4 ) & 1) << 2) |                /* bit 2 */
                        (((hash >> 2 ) & 1) << 1) |                /* bit 1 */
                        (( hash >> 0 ) & 1);                         /* bit 0 */
                        break;
                case 1:
                        bucket =
                        (((hash >>  2) & 1) << 10)|                /* bit 10 */
                        (((hash >> 12) & 1) << 9) |                /* bit 9 */
                        (((hash >> 0 ) & 1) << 8) |                /* bit 8 */
                        (((hash >> 10) & 1) << 7) |                /* bit 7 */
                        (((hash >> 8 ) & 1) << 6) |                /* bit 6 */
                        (((hash >> 7 ) & 1) << 5) |                /* bit 5 */
                        (((hash >> 6 ) & 1) << 4) |                /* bit 4 */
                        (((hash >> 5 ) & 1) << 3) |                /* bit 3 */
                        (((hash >> 4 ) & 1) << 2) |                /* bit 2 */
                        (((hash >> 3 ) & 1) << 1) |                /* bit 1 */
                        (( hash >> 1 ) & 1);                         /* bit 0 */
                        break;
                case 2:
                        bucket =
                        (((hash >> 13) & 1) << 10)|                /* bit 10 */
                        (((hash >> 15) & 1) << 9) |                /* bit 9 */
                        (((hash >> 3 ) & 1) << 8) |                /* bit 8 */
                        (((hash >> 14) & 1) << 7) |                /* bit 7 */
                        (((hash >> 12) & 1) << 6) |                /* bit 6 */
                        (((hash >> 11) & 1) << 5) |                /* bit 5 */
                        (((hash >> 10) & 1) << 4) |                /* bit 4 */
                        (((hash >> 9 ) & 1) << 3) |                /* bit 3 */
                        (((hash >> 8 ) & 1) << 2) |                /* bit 2 */
                        (((hash >> 7 ) & 1) << 1) |                /* bit 1 */
                        (( hash >> 5 ) & 1);                         /* bit 0 */
                        break;
                case 3:
                        bucket =
                        (((hash >> 10) & 1) << 10)|                /* bit 10 */
                        (((hash >> 8 ) & 1) << 9) |                /* bit 9 */
                        (((hash >> 7 ) & 1) << 8) |                /* bit 8 */
                        (((hash >> 13) & 1) << 7) |                /* bit 7 */
                        (((hash >> 12) & 1) << 6) |                /* bit 6 */
                        (((hash >> 10) & 1) << 5) |                /* bit 5 */
                        (((hash >> 9 ) & 1) << 4) |                /* bit 4 */
                        (((hash >> 6 ) & 1) << 3) |                /* bit 3 */
                        (((hash >> 5 ) & 1) << 2) |                /* bit 2 */
                        (((hash >> 3 ) & 1) << 1) |                /* bit 1 */
                        (( hash >> 2 ) & 1);                         /* bit 0 */
                        break;
                default:
                        /* treat as case 0 */
                        bucket =
                        (((hash >>  7) & 1) << 10)|                /* bit 10 */
                        (((hash >> 11) & 1) << 9) |                /* bit 9 */
                        (((hash >> 3 ) & 1) << 8) |                /* bit 8 */
                        (((hash >> 14) & 1) << 7) |                /* bit 7 */
                        (((hash >> 12) & 1) << 6) |                /* bit 6 */
                        (((hash >> 10) & 1) << 5) |                /* bit 5 */
                        (((hash >> 8 ) & 1) << 4) |                /* bit 4 */
                        (((hash >> 6 ) & 1) << 3) |                /* bit 3 */
                        (((hash >> 4 ) & 1) << 2) |                /* bit 2 */
                        (((hash >> 2 ) & 1) << 1) |                /* bit 1 */
                        (( hash >> 0 ) & 1);                         /* bit 0 */
                        break;
        }
        return bucket;
}


/*******************************************************************************
* hashFunction
*
* DESCRIPTION:
*       This function calculates the hash index for the mac address table.
*       for specific mac address.
*
* INPUTS:
*       addr        - the mac address.
*
* OUTPUTS:
*
* RETURNS:
*       GT_U16  - hash index
*
* COMMENTS:
*       None.
*
*******************************************************************************/
GT_U16 hashFunction(GT_ETHERADDR * eaddr)
{
    GT_U16 crc_reg;
    GT_U8 crc_in;
    int i,j;

    crc_reg=0;

    for(i=0; i<6; i++)
    {
        crc_in = eaddr->arEther[5-i];
        for(j=0; j<8; j++)
        {
            crc_reg = ((((crc_in & 1) ^ ((crc_reg>>15) & 1)) ^
                      ((crc_reg>>14) & 1)) << 15)         |        /* bit 15 */
                      ((crc_reg & 0x3FFC) << 1)           |       /* bit 14:3 */
                      ((((crc_in & 1) ^ ((crc_reg>>15) & 1)) ^
                      ((crc_reg>>1) & 1)) << 2)           |        /* bit 2 */
                      ((crc_reg & 1) << 1)                |         /* bit 1 */
                      ((crc_in & 1) ^ ((crc_reg>>15) & 1));          /* bit 0 */

                        crc_in >>= 1;
        }
    }

    return crc_reg;
}


/*******************************************************************************
* sohoMacHashCalc
*
* DESCRIPTION:
*       This function calculates the hash index for the mac address table.
*       for specific mac address.
*
* INPUTS:
*       addr        - the mac address.
*
* OUTPUTS:
*       bucket - the hash index.
*
* RETURNS:
*       GT_OK        - on success
*       GT_FAIL      - on error
*
* COMMENTS:
*       None.
*
*******************************************************************************/
GT_STATUS sohoMacHashCalc
(
    IN  GT_ETHERADDR    *addr,
    OUT GT_U32          *bucket_PTR
)
{
      GT_U16 hash , bucket ;

      hash = hashFunction(addr);
      bucket = hashToBucket(hash,0);
      *bucket_PTR = bucket ;

      return GT_OK;
}

