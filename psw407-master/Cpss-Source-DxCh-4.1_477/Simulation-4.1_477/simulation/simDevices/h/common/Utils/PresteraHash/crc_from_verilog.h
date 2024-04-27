/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* crc_from_verilog.h
*
* DESCRIPTION:
*       CRC Hash functions
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 2 $
*
*******************************************************************************/
#ifndef __crcFromVerilogh
#define __crcFromVerilogh

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


GT_VOID crc_16a_d36(GT_U32 *D, GT_U32 *c, GT_U32 *h);
GT_VOID crc_16a_d44(GT_U32 *D, GT_U32 *c, GT_U32 *h);
GT_VOID crc_16a_d60(GT_U32 *D, GT_U32 *c, GT_U32 *h);
/*GT_VOID crc_16a_d76(GT_U32 *D, GT_U32 *c, GT_U32 *h);*/
/*GT_VOID crc_16a_d80(GT_U32 *D, GT_U32 *c, GT_U32 *h);*/
/*GT_VOID crc_16a_d140(GT_U32 *D, GT_U32 *c, GT_U32 *h);*/
GT_VOID crc_16b_d36(GT_U32 *D, GT_U32 *c, GT_U32 *h);
GT_VOID crc_16b_d44(GT_U32 *D, GT_U32 *c, GT_U32 *h);
GT_VOID crc_16b_d60(GT_U32 *D, GT_U32 *c, GT_U32 *h);
/*GT_VOID crc_16b_d80(GT_U32 *D, GT_U32 *c, GT_U32 *h);*/
/*GT_VOID crc_16b_d140(GT_U32 *D, GT_U32 *c, GT_U32 *h);*/
GT_VOID crc_16c_d36(GT_U32 *D, GT_U32 *c, GT_U32 *h);
GT_VOID crc_16c_d44(GT_U32 *D, GT_U32 *c, GT_U32 *h);
GT_VOID crc_16c_d60(GT_U32 *D, GT_U32 *c, GT_U32 *h);
/*GT_VOID crc_16c_d80(GT_U32 *D, GT_U32 *c, GT_U32 *h);*/
/*GT_VOID crc_16c_d140(GT_U32 *D, GT_U32 *c, GT_U32 *h);*/
GT_VOID crc_16d_d36(GT_U32 *D, GT_U32 *c, GT_U32 *h);
GT_VOID crc_16d_d44(GT_U32 *D, GT_U32 *c, GT_U32 *h);
GT_VOID crc_16d_d60(GT_U32 *D, GT_U32 *c, GT_U32 *h);
/*GT_VOID crc_16d_d80(GT_U32 *D, GT_U32 *c, GT_U32 *h);*/
/*GT_VOID crc_16d_d140(GT_U32 *D, GT_U32 *c, GT_U32 *h);*/
GT_VOID crc_32a_d36(GT_U32 *D, GT_U32 *c, GT_U32 *h);
GT_VOID crc_32a_d44(GT_U32 *D, GT_U32 *c, GT_U32 *h);
GT_VOID crc_32a_d60(GT_U32 *D, GT_U32 *c, GT_U32 *h);
/*GT_VOID crc_32a_d76(GT_U32 *D, GT_U32 *c, GT_U32 *h);*/
/*GT_VOID crc_32a_d80(GT_U32 *D, GT_U32 *c, GT_U32 *h);*/
/*GT_VOID crc_32a_d140(GT_U32 *D, GT_U32 *c, GT_U32 *h);*/
GT_VOID crc_32b_d36(GT_U32 *D, GT_U32 *c, GT_U32 *h);
GT_VOID crc_32b_d44(GT_U32 *D, GT_U32 *c, GT_U32 *h);
GT_VOID crc_32b_d60(GT_U32 *D, GT_U32 *c, GT_U32 *h);
/*GT_VOID crc_32b_d80(GT_U32 *D, GT_U32 *c, GT_U32 *h);*/
/*GT_VOID crc_32b_d140(GT_U32 *D, GT_U32 *c, GT_U32 *h);*/
GT_VOID crc_32k_d36(GT_U32 *D, GT_U32 *c, GT_U32 *h);
GT_VOID crc_32k_d44(GT_U32 *D, GT_U32 *c, GT_U32 *h);
GT_VOID crc_32k_d60(GT_U32 *D, GT_U32 *c, GT_U32 *h);
/*GT_VOID crc_32k_d80(GT_U32 *D, GT_U32 *c, GT_U32 *h);*/
/*GT_VOID crc_32k_d140(GT_U32 *D, GT_U32 *c, GT_U32 *h);*/
GT_VOID crc_32q_d36(GT_U32 *D, GT_U32 *c, GT_U32 *h);
GT_VOID crc_32q_d44(GT_U32 *D, GT_U32 *c, GT_U32 *h);
GT_VOID crc_32q_d60(GT_U32 *D, GT_U32 *c, GT_U32 *h);
/*GT_VOID crc_32q_d80(GT_U32 *D, GT_U32 *c, GT_U32 *h);*/
/*GT_VOID crc_32q_d140(GT_U32 *D, GT_U32 *c, GT_U32 *h);*/
GT_VOID crc_64_d36(GT_U32 *D, GT_U32 *c, GT_U32 *h);
GT_VOID crc_64_d44(GT_U32 *D, GT_U32 *c, GT_U32 *h);
GT_VOID crc_64_d60(GT_U32 *D, GT_U32 *c, GT_U32 *h);
/*GT_VOID crc_64_d80(GT_U32 *D, GT_U32 *c, GT_U32 *h);*/
/*GT_VOID crc_64_d140(GT_U32 *D, GT_U32 *c, GT_U32 *h);*/

/*******************************************************************************
* crc_32a_d76_noInitValueByNumBits
*
* DESCRIPTION:
*       This function calculates CRC 32a bits according to 76 bits DATA.
*        from the CRC32 only numOfBits are calculated.
*
* INPUTS:
*       D         - array of 32bit words, contain data
*
* OUTPUTS:
*       hash_bit  - the hash bits array
*
* RETURNS:
*       None.
*
* COMMENTS: based on verilog : sip_mt_crc_32a_d76.v
*
*******************************************************************************/
GT_VOID crc_32a_d76_noInitValueByNumBits
(
    IN  GT_U32  numOfBits,
    IN  GT_U32  *D,
    OUT GT_U32  *hash_bit
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __crcFromVerilogh */

