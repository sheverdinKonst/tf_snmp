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
* smacHashCheetah.c
*
* DESCRIPTION:
*       Hash calculate for MAC address table implementation for Cheetah.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 20 $
*
*******************************************************************************/
#include <common/Utils/PresteraHash/smacHash.h>
#include <common/Utils/PresteraHash/smacHashDx.h>
#include <common/Utils/PresteraHash/crc_from_verilog.h>
#include <asicSimulation/SInit/sinit.h>
#include <asicSimulation/SKernel/smem/smem.h>
#include <asicSimulation/SKernel/suserframes/snet.h>
#include <asicSimulation/SKernel/suserframes/snetCheetahL2.h>

#define GT_HW_SIM_MAC_LOW32(macAddr)                \
                (macAddr[5] |          \
                (macAddr[4] << 8) |    \
                (macAddr[3] << 16) |   \
                (macAddr[2] << 24))

#define GT_HW_SIM_MAC_HIGH16(macAddr)           \
        (macAddr[1] | (macAddr[0] << 8))

#define getbit(word,bitNum) ((word) >> (bitNum))

/* typedef for all functions like:
GT_VOID crc_X_d36(GT_U32 *D, GT_U32 *c, GT_U32 *h);

where X is one of : 16a,16b,16c,16d,32a,32b,32k,32q,64

*/
typedef GT_VOID (*CRC_FUNC_TYPE)(GT_U32 *D, GT_U32 *c, GT_U32 *h);
/* typedef for actual functions that the SIP5 uses for calculation of up to 140 bits data*/
typedef struct{
    CRC_FUNC_TYPE   calc36BitsData;
    CRC_FUNC_TYPE   calc44BitsData;
    CRC_FUNC_TYPE   calc60BitsData;
}CRC_FUNC_STC;

/* typedef for hash functions that the SIP5 uses */
typedef enum{
    CRC_FUNC_16A_E,
    CRC_FUNC_16B_E,
    CRC_FUNC_16C_E,
    CRC_FUNC_16D_E,
    CRC_FUNC_32A_E,
    CRC_FUNC_32B_E,
    CRC_FUNC_32K_E,
    CRC_FUNC_32Q_E,
    CRC_FUNC_64_E,

    CRC_FUNC___DUMMY__E
}CRC_FUNC_ENT;

/* array of actual functions that the SIP5 uses for calculation of up to 140 bits data*/
/* relevant only to <Multi Hash Enable> = 1 */
static CRC_FUNC_STC crcFuncArray[] =
{
    /*CRC_FUNC_16A_E*/     {crc_16a_d36,   crc_16a_d44,    crc_16a_d60}
    /*CRC_FUNC_16B_E*/    ,{crc_16b_d36,   crc_16b_d44,    crc_16b_d60}
    /*CRC_FUNC_16C_E*/    ,{crc_16c_d36,   crc_16c_d44,    crc_16c_d60}
    /*CRC_FUNC_16D_E*/    ,{crc_16d_d36,   crc_16d_d44,    crc_16d_d60}
    /*CRC_FUNC_32A_E*/    ,{crc_32a_d36,   crc_32a_d44,    crc_32a_d60}
    /*CRC_FUNC_32B_E*/    ,{crc_32b_d36,   crc_32b_d44,    crc_32b_d60}
    /*CRC_FUNC_32K_E*/    ,{crc_32k_d36,   crc_32k_d44,    crc_32k_d60}
    /*CRC_FUNC_32Q_E*/    ,{crc_32q_d36,   crc_32q_d44,    crc_32q_d60}
    /*CRC_FUNC_64_E */    ,{crc_64_d36 ,   crc_64_d44 ,    crc_64_d60 }
};

#define BIT_AS_END_BIT_CNS  (0x00010000)

typedef struct{
    CRC_FUNC_ENT    funcType;
    GT_U32          startBit;
}CRC_FUNC_BITS_USED_INFO_STC;

/* relevant only to <Multi Hash Enable> = 1 */
static CRC_FUNC_BITS_USED_INFO_STC crcFuncBitsUsedArr[FDB_MAX_BANKS_CNS] =
{
    {CRC_FUNC_16A_E       ,   0}, /*[X-1:0]*/

    {CRC_FUNC_16B_E       ,   0}, /*[X-1:0]*/

    {CRC_FUNC_16C_E       ,   0}, /*[X-1:0]*/

    {CRC_FUNC_16D_E       ,   0}, /*[X-1:0]*/

    {CRC_FUNC_32A_E       ,   0}, /*[X-1:0]*/
    {CRC_FUNC___DUMMY__E  ,  16}, /*[16+x-1:16]*/

    {CRC_FUNC_32B_E       ,   0}, /*[X-1:0]*/
    {CRC_FUNC___DUMMY__E  ,  16}, /*[16+x-1:16]*/

    {CRC_FUNC_32K_E       ,   0}, /*[X-1:0]*/
#if 0
    {CRC_FUNC___DUMMY__E  ,  32 | BIT_AS_END_BIT_CNS}, /*[31:31-X+1]*/
#else
    {CRC_FUNC___DUMMY__E  ,  16}, /*[16+x-1:16]*/
#endif /*0*/

    {CRC_FUNC_32Q_E       ,   0}, /*[X-1:0]*/
#if 0
    {CRC_FUNC___DUMMY__E  ,  32 | BIT_AS_END_BIT_CNS}, /*[31:31-X+1]*/
#else
    {CRC_FUNC___DUMMY__E  ,  16}, /*[16+x-1:16]*/
#endif /*0*/

    {CRC_FUNC_64_E        ,   0}, /*[X-1:0]*/
    {CRC_FUNC___DUMMY__E  ,  16}, /*[16+x-1:16]*/
    {CRC_FUNC___DUMMY__E  ,  32}, /*[32+X-1:32]*/
    {CRC_FUNC___DUMMY__E  ,  48}  /*[48+x-1:48]*/

};

static GT_VOID hashCalcFillDipSipVidDataCrc
(
    IN  GT_BIT          fid16BitHashEn,
    IN  GT_U32          destIpAddr,
    IN  GT_U32          sourceIpAddr,
    IN  GT_U32          vlanId,
    OUT GT_U32          *dataPtr
);

/*******************************************************************************
* hashCalcDataXor
*
* DESCRIPTION:
*       This function calculates the hash index for the
*       mac address table using XOR algorithm.
*
* INPUTS:
*       dataPtr     - array of 32bit words, contain data.
*       numOfWords  - amount of words of data
* OUTPUTS:
*       hashPtr - the hash index.
*
* RETURNS:
*       none
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static GT_VOID hashCalcDataXor
(
    IN  GT_U32          *dataPtr,
    IN  GT_U32          numOfWords,
    OUT GT_U32          *hashPtr
)
{
    GT_U32  hash ;
    GT_U32  idx ;

    /* compute the transposed hash */
    hash = dataPtr[0];
    for (idx = 1; idx < numOfWords; idx++)
    {
        hash ^= dataPtr[idx];
    }

    *hashPtr = hash * 4 ;

    return ;
}

/*******************************************************************************
* crc_11_12
*
* DESCRIPTION:
*       This function calculates CRC (11-12 bits)
*
* INPUTS:
*       D         - array of 32bit words, contain data
*       bitsWidth - num of bits used in hash
*       isSip5      - use SIP5 algorithm for HASH function calculation
* OUTPUTS:
*       hash_bit  - the hash bits array
*
* RETURNS:
*       None.
*
* COMMENTS:
*
*******************************************************************************/
static GT_VOID crc_11_12
(
    IN  GT_U32  *D,
    IN  GT_U32   bitsWidth,
    OUT GT_U32  *hash_bit,
    IN  GT_BIT   isSip5
)
{
    hash_bit[0] =
                 getbit(D[2],11) ^ getbit(D[2],8)  ^ getbit(D[2],6)  ^
                 getbit(D[2],5)  ^ getbit(D[2],2)  ^ getbit(D[2],0)  ^
                 getbit(D[1],31) ^ getbit(D[1],29) ^ getbit(D[1],28) ^
                 getbit(D[1],27) ^ getbit(D[1],26) ^ getbit(D[1],22) ^
                 getbit(D[1],20) ^ getbit(D[1],19) ^ getbit(D[1],18) ^
                 getbit(D[1],17) ^ getbit(D[1],16) ^ getbit(D[1],15) ^
                 getbit(D[1],12) ^ getbit(D[1],3)  ^ getbit(D[1],2)  ^
                 getbit(D[1],1)  ^ getbit(D[0],30) ^ getbit(D[0],29) ^
                 getbit(D[0],26) ^ getbit(D[0],25) ^ getbit(D[0],24) ^
                 getbit(D[0],23) ^ getbit(D[0],22) ^ getbit(D[0],17) ^
                 getbit(D[0],16) ^ getbit(D[0],15) ^ getbit(D[0],14) ^
                 getbit(D[0],13) ^ getbit(D[0],12) ^ getbit(D[0],11) ^
                 getbit(D[0],8)  ^ getbit(D[0],7)  ^ getbit(D[0],6)  ^
                 getbit(D[0],5)  ^ getbit(D[0],4)  ^ getbit(D[0],3)  ^
                 getbit(D[0],2)  ^ getbit(D[0],1)  ^ getbit(D[0],0)  ;

   hash_bit[1] =
                 getbit(D[2],11) ^ getbit(D[2],9)  ^ getbit(D[2],8)  ^
                 getbit(D[2],7)  ^ getbit(D[2],5)  ^ getbit(D[2],3)  ^
                 getbit(D[2],2)  ^ getbit(D[2],1)  ^ getbit(D[1],31) ^
                 getbit(D[1],30) ^ getbit(D[1],26) ^ getbit(D[1],23) ^
                 getbit(D[1],22) ^ getbit(D[1],21) ^ getbit(D[1],15) ^
                 getbit(D[1],13) ^ getbit(D[1],12) ^ getbit(D[1],4)  ^
                 getbit(D[1],1)  ^ getbit(D[0],31) ^ getbit(D[0],29) ^
                 getbit(D[0],27) ^ getbit(D[0],22) ^ getbit(D[0],18) ^
                 getbit(D[0],11) ^ getbit(D[0],9)  ^ getbit(D[0],0)  ;

   hash_bit[2] = getbit(D[2],11) ^ getbit(D[2],10) ^ getbit(D[2],9)  ^
                 getbit(D[2],5)  ^ getbit(D[2],4)  ^ getbit(D[2],3)  ^
                 getbit(D[1],29) ^ getbit(D[1],28) ^ getbit(D[1],26) ^
                 getbit(D[1],24) ^ getbit(D[1],23) ^ getbit(D[1],20) ^
                 getbit(D[1],19) ^ getbit(D[1],18) ^ getbit(D[1],17) ^
                 getbit(D[1],15) ^ getbit(D[1],14) ^ getbit(D[1],13) ^
                 getbit(D[1],12) ^ getbit(D[1],5)  ^ getbit(D[1],3)  ^
                 getbit(D[1],1)  ^ getbit(D[1],0)  ^ getbit(D[0],29) ^
                 getbit(D[0],28) ^ getbit(D[0],26) ^ getbit(D[0],25) ^
                 getbit(D[0],24) ^ getbit(D[0],22) ^ getbit(D[0],19) ^
                 getbit(D[0],17) ^ getbit(D[0],16) ^ getbit(D[0],15) ^
                 getbit(D[0],14) ^ getbit(D[0],13) ^ getbit(D[0],11) ^
                 getbit(D[0],10) ^ getbit(D[0],8)  ^ getbit(D[0],7)  ^
                 getbit(D[0],6)  ^ getbit(D[0],5)  ^ getbit(D[0],4)  ^
                 getbit(D[0],3)  ^ getbit(D[0],2)  ^ getbit(D[0],0)  ;

   hash_bit[3] =
                 getbit(D[2],10) ^ getbit(D[2],8) ^ getbit(D[2],4)   ^
                 getbit(D[2],2)  ^ getbit(D[2],0) ^ getbit(D[1],31) ^
                 getbit(D[1],30) ^ getbit(D[1],28) ^ getbit(D[1],26) ^
                 getbit(D[1],25) ^ getbit(D[1],24) ^ getbit(D[1],22) ^
                 getbit(D[1],21) ^ getbit(D[1],17) ^ getbit(D[1],14) ^
                 getbit(D[1],13) ^ getbit(D[1],12) ^ getbit(D[1],6)  ^
                 getbit(D[1],4)  ^ getbit(D[1],3)  ^ getbit(D[0],27) ^
                 getbit(D[0],24) ^ getbit(D[0],22) ^ getbit(D[0],20) ^
                 getbit(D[0],18) ^ getbit(D[0],13) ^ getbit(D[0],9)  ^
                 getbit(D[0],2)  ^ getbit(D[0],0);

   hash_bit[4] = getbit(D[2],11) ^ getbit(D[2],9)  ^ getbit(D[2],5)  ^
                 getbit(D[2],3)  ^ getbit(D[2],1)  ^ getbit(D[2],0)  ^
                 getbit(D[1],31) ^ getbit(D[1],29) ^ getbit(D[1],27) ^
                 getbit(D[1],26) ^ getbit(D[1],25) ^ getbit(D[1],23) ^
                 getbit(D[1],22) ^ getbit(D[1],18) ^ getbit(D[1],15) ^
                 getbit(D[1],14) ^ getbit(D[1],13) ^ getbit(D[1],7)  ^
                 getbit(D[1],5)  ^ getbit(D[1],4)  ^ getbit(D[0],28) ^
                 getbit(D[0],25) ^ getbit(D[0],23) ^ getbit(D[0],21) ^
                 getbit(D[0],19) ^ getbit(D[0],14) ^ getbit(D[0],10) ^
                 getbit(D[0],3)  ^ getbit(D[0],1)  ;

   hash_bit[5] = getbit(D[2],10) ^ getbit(D[2],6)  ^ getbit(D[2],4)  ^
                 getbit(D[2],2)  ^ getbit(D[2],1)  ^ getbit(D[2],0)  ^
                 getbit(D[1],30) ^ getbit(D[1],28) ^ getbit(D[1],27) ^
                 getbit(D[1],26) ^ getbit(D[1],24) ^ getbit(D[1],23) ^
                 getbit(D[1],19) ^ getbit(D[1],16) ^ getbit(D[1],15) ^
                 getbit(D[1],14) ^ getbit(D[1],8)  ^ getbit(D[1],6)  ^
                 getbit(D[1],5)  ^ getbit(D[0],29) ^ getbit(D[0],26) ^
                 getbit(D[0],24) ^ getbit(D[0],22) ^ getbit(D[0],20) ^
                 getbit(D[0],15) ^ getbit(D[0],11) ^ getbit(D[0],4)  ^
                 getbit(D[0],2)  ;

   hash_bit[6] =
                 getbit(D[2],11) ^ getbit(D[2],7)  ^ getbit(D[2],5)  ^
                 getbit(D[2],3)  ^ getbit(D[2],2)  ^ getbit(D[2],1)  ^
                 getbit(D[1],31) ^ getbit(D[1],29) ^ getbit(D[1],28) ^
                 getbit(D[1],27) ^ getbit(D[1],25) ^ getbit(D[1],24) ^
                 getbit(D[1],20) ^ getbit(D[1],17) ^ getbit(D[1],16) ^
                 getbit(D[1],15) ^ getbit(D[1],9)  ^ getbit(D[1],7)  ^
                 getbit(D[1],6)  ^ getbit(D[0],30) ^ getbit(D[0],27) ^
                 getbit(D[0],25) ^ getbit(D[0],23) ^ getbit(D[0],21) ^
                 getbit(D[0],16) ^ getbit(D[0],12) ^ getbit(D[0],5)  ^
                 getbit(D[0],3)  ;

   hash_bit[7] = getbit(D[2],8)  ^ getbit(D[2],6)  ^ getbit(D[2],4)  ^
                 getbit(D[2],3)  ^ getbit(D[2],2)  ^ getbit(D[2],0)  ^
                 getbit(D[1],30) ^ getbit(D[1],29) ^ getbit(D[1],28) ^
                 getbit(D[1],26) ^ getbit(D[1],25) ^ getbit(D[1],21) ^
                 getbit(D[1],18) ^ getbit(D[1],17) ^ getbit(D[1],16) ^
                 getbit(D[1],10) ^ getbit(D[1],8)  ^ getbit(D[1],7)  ^
                 getbit(D[0],31) ^ getbit(D[0],28) ^ getbit(D[0],26) ^
                 getbit(D[0],24) ^ getbit(D[0],22) ^ getbit(D[0],17) ^
                 getbit(D[0],13) ^ getbit(D[0],6)  ^ getbit(D[0],4)  ;

   hash_bit[8] = getbit(D[2],9)  ^ getbit(D[2],7)  ^ getbit(D[2],5)  ^
                 getbit(D[2],4)  ^ getbit(D[2],3)  ^ getbit(D[2],1)  ^
                 getbit(D[1],31) ^ getbit(D[1],30) ^ getbit(D[1],29) ^
                 getbit(D[1],27) ^ getbit(D[1],26) ^ getbit(D[1],22) ^
                 getbit(D[1],19) ^ getbit(D[1],18) ^ getbit(D[1],17) ^
                 getbit(D[1],11) ^ getbit(D[1],9)  ^ getbit(D[1],8)  ^
                 getbit(D[1],0)  ^ getbit(D[0],29) ^ getbit(D[0],27) ^
                 getbit(D[0],25) ^ getbit(D[0],23) ^ getbit(D[0],18) ^
                 getbit(D[0],14) ^ getbit(D[0],7)  ^ getbit(D[0],5)  ;

   hash_bit[9] = getbit(D[2],10) ^ getbit(D[2],8)  ^ getbit(D[2],6)  ^
                 getbit(D[2],5)  ^ getbit(D[2],4)  ^ getbit(D[2],2)  ^
                 getbit(D[2],0)  ^ getbit(D[1],31) ^ getbit(D[1],30) ^
                 getbit(D[1],28) ^ getbit(D[1],27) ^ getbit(D[1],23) ^
                 getbit(D[1],20) ^ getbit(D[1],19) ^ getbit(D[1],18) ^
                 getbit(D[1],12) ^ getbit(D[1],10) ^ getbit(D[1],9)  ^
                 getbit(D[1],1)  ^ getbit(D[0],30) ^ getbit(D[0],28) ^
                 getbit(D[0],26) ^ getbit(D[0],24) ^ getbit(D[0],19) ^
                 getbit(D[0],15) ^ getbit(D[0],8)  ^ getbit(D[0],6);

   hash_bit[10] = getbit(D[2],11) ^ getbit(D[2],9)  ^ getbit(D[2],7)  ^
                  getbit(D[2],6)  ^ getbit(D[2],5)  ^ getbit(D[2],3)  ^
                  getbit(D[2],1)  ^ getbit(D[2],0)  ^ getbit(D[1],31) ^
                  getbit(D[1],29) ^ getbit(D[1],28) ^ getbit(D[1],24) ^
                  getbit(D[1],21) ^ getbit(D[1],20) ^ getbit(D[1],19) ^
                  getbit(D[1],13) ^ getbit(D[1],11) ^ getbit(D[1],10) ^
                  getbit(D[1],2)  ^ getbit(D[0],31) ^ getbit(D[0],29) ^
                  getbit(D[0],27) ^ getbit(D[0],25) ^ getbit(D[0],20) ^
                  getbit(D[0],16) ^ getbit(D[0],9)  ^ getbit(D[0],7)  ;

    if (bitsWidth == 12)
    {
        hash_bit[11] = getbit(D[2],11) ^ getbit(D[2],10) ^ getbit(D[2],7)  ^
                       getbit(D[2],5)  ^ getbit(D[2],4)  ^ getbit(D[2],1)  ^
                       getbit(D[1],31) ^ getbit(D[1],30) ^ getbit(D[1],28) ^
                       getbit(D[1],27) ^ getbit(D[1],26) ^ getbit(D[1],25) ^
                       getbit(D[1],21) ^ getbit(D[1],19) ^ getbit(D[1],18) ^
                       getbit(D[1],17) ^ getbit(D[1],16) ^ getbit(D[1],15) ^
                       getbit(D[1],14) ^ getbit(D[1],11) ^ getbit(D[1],2)  ^
                       getbit(D[1],1)  ^ getbit(D[1],0)  ^ getbit(D[0],29) ^
                       getbit(D[0],28) ^ getbit(D[0],25) ^ getbit(D[0],24) ^
                       getbit(D[0],23) ^ getbit(D[0],22) ^ getbit(D[0],21) ^
                       getbit(D[0],16) ^ getbit(D[0],15) ^ getbit(D[0],14) ^
                       getbit(D[0],13) ^ getbit(D[0],12) ^ getbit(D[0],11) ^
                       getbit(D[0],10) ^ getbit(D[0],7)  ^ getbit(D[0],6)  ^
                       getbit(D[0],5)  ^ getbit(D[0],4)  ^ getbit(D[0],3)  ^
                       getbit(D[0],2)  ^ getbit(D[0],1)  ^ getbit(D[0],0)  ;
    }
    else
    {
        hash_bit[11] = 0;
    }

    /* take care of init values */
    if (!isSip5)
    {
        hash_bit[3] ^= 1;
        hash_bit[9] ^= 1;
    }
}

/*******************************************************************************
* crc_13
*
* DESCRIPTION:
*       This function calculates CRC (13 bits)
*
* INPUTS:
*       D         - array of 32bit words, contain data
*       isSip5      - use SIP5 algorithm for HASH function calculation
*
* OUTPUTS:
*       hash_bit  - the hash bits array
*
* RETURNS:
*       None.
*
* COMMENTS:
*
*******************************************************************************/
static GT_VOID crc_13
(
    IN  GT_U32  *D,
    OUT GT_U32  *hash_bit,
    IN  GT_BIT   isSip5
)
{
    hash_bit[0] =
                getbit(D[2],11) ^ getbit(D[2],10) ^ getbit(D[2],9)  ^
                getbit(D[2],7)  ^ getbit(D[2],4)  ^ getbit(D[2],2)  ^
                getbit(D[2],0)  ^
                getbit(D[1],31) ^ getbit(D[1],30) ^
                getbit(D[1],29) ^ getbit(D[1],28) ^ getbit(D[1],26) ^
                getbit(D[1],25) ^ getbit(D[1],24) ^ getbit(D[1],23) ^
                getbit(D[1],20) ^ getbit(D[1],13) ^ getbit(D[1],11) ^
                getbit(D[1],10) ^ getbit(D[1],9)  ^ getbit(D[1],7)  ^
                getbit(D[1],5)  ^ getbit(D[1],3)  ^ getbit(D[1],2)  ^
                getbit(D[1],1)  ^ getbit(D[1],0)  ^ getbit(D[0],31) ^
                getbit(D[0],28) ^ getbit(D[0],26) ^ getbit(D[0],25) ^
                getbit(D[0],21) ^ getbit(D[0],19) ^ getbit(D[0],18) ^
                getbit(D[0],17) ^ getbit(D[0],14) ^ getbit(D[0],13) ^
                getbit(D[0],12) ^ getbit(D[0],11) ^ getbit(D[0],9)  ^
                getbit(D[0],7)  ^ getbit(D[0],6)  ^ getbit(D[0],4)  ^
                getbit(D[0],3)  ^ getbit(D[0],1)  ^ getbit(D[0],0);

    hash_bit[1] =
                getbit(D[2],9)  ^ getbit(D[2],8)  ^ getbit(D[2],7)  ^
                getbit(D[2],5)  ^ getbit(D[2],4)  ^ getbit(D[2],3)  ^
                getbit(D[2],2)  ^ getbit(D[2],1)  ^
                getbit(D[1],28) ^
                getbit(D[1],27) ^ getbit(D[1],23) ^ getbit(D[1],21) ^
                getbit(D[1],20) ^ getbit(D[1],14) ^ getbit(D[1],13) ^
                getbit(D[1],12) ^ getbit(D[1],9)  ^ getbit(D[1],8)  ^
                getbit(D[1],7)  ^ getbit(D[1],6)  ^ getbit(D[1],5)  ^
                getbit(D[1],4)  ^ getbit(D[0],31) ^ getbit(D[0],29) ^
                getbit(D[0],28) ^ getbit(D[0],27) ^ getbit(D[0],25) ^
                getbit(D[0],22) ^ getbit(D[0],21) ^ getbit(D[0],20) ^
                getbit(D[0],17) ^ getbit(D[0],15) ^ getbit(D[0],11) ^
                getbit(D[0],10) ^ getbit(D[0],9)  ^ getbit(D[0],8)  ^
                getbit(D[0],6)  ^ getbit(D[0],5)  ^ getbit(D[0],3)  ^
                getbit(D[0],2)  ^ getbit(D[0],0);

    hash_bit[2] =
                getbit(D[2],11) ^ getbit(D[2],8)  ^ getbit(D[2],7)  ^
                getbit(D[2],6)  ^ getbit(D[2],5)  ^ getbit(D[2],3)  ^
                getbit(D[2],0)  ^
                getbit(D[1],31) ^ getbit(D[1],30) ^
                getbit(D[1],26) ^ getbit(D[1],25) ^ getbit(D[1],23) ^
                getbit(D[1],22) ^ getbit(D[1],21) ^ getbit(D[1],20) ^
                getbit(D[1],15) ^ getbit(D[1],14) ^ getbit(D[1],11) ^
                getbit(D[1],8)  ^ getbit(D[1],6)  ^ getbit(D[1],3)  ^
                getbit(D[1],2)  ^ getbit(D[1],1)  ^ getbit(D[0],31) ^
                getbit(D[0],30) ^ getbit(D[0],29) ^ getbit(D[0],25) ^
                getbit(D[0],23) ^ getbit(D[0],22) ^ getbit(D[0],19) ^
                getbit(D[0],17) ^ getbit(D[0],16) ^ getbit(D[0],14) ^
                getbit(D[0],13) ^ getbit(D[0],10) ^ getbit(D[0],0);

    hash_bit[3] =
                getbit(D[2],11) ^ getbit(D[2],10) ^ getbit(D[2],8)  ^
                getbit(D[2],6)  ^ getbit(D[2],2)  ^ getbit(D[2],1)  ^
                getbit(D[1],30) ^ getbit(D[1],29) ^ getbit(D[1],28) ^
                getbit(D[1],27) ^ getbit(D[1],25) ^ getbit(D[1],22) ^
                getbit(D[1],21) ^ getbit(D[1],20) ^ getbit(D[1],16) ^
                getbit(D[1],15) ^ getbit(D[1],13) ^ getbit(D[1],12) ^
                getbit(D[1],11) ^ getbit(D[1],10) ^ getbit(D[1],5)  ^
                getbit(D[1],4)  ^ getbit(D[1],1)  ^ getbit(D[0],30) ^
                getbit(D[0],28) ^ getbit(D[0],25) ^ getbit(D[0],24) ^
                getbit(D[0],23) ^ getbit(D[0],21) ^ getbit(D[0],20) ^
                getbit(D[0],19) ^ getbit(D[0],15) ^ getbit(D[0],13) ^
                getbit(D[0],12) ^ getbit(D[0],9)  ^ getbit(D[0],7)  ^
                getbit(D[0],6)  ^ getbit(D[0],4)  ^ getbit(D[0],3)  ^
                getbit(D[0],0);

    hash_bit[4] =
                getbit(D[2],11) ^ getbit(D[2],9)  ^ getbit(D[2],7)  ^
                getbit(D[2],3)  ^ getbit(D[2],2)  ^
                getbit(D[1],31) ^
                getbit(D[1],30) ^ getbit(D[1],29) ^ getbit(D[1],28) ^
                getbit(D[1],26) ^ getbit(D[1],23) ^ getbit(D[1],22) ^
                getbit(D[1],21) ^ getbit(D[1],17) ^ getbit(D[1],16) ^
                getbit(D[1],14) ^ getbit(D[1],13) ^ getbit(D[1],12) ^
                getbit(D[1],11) ^ getbit(D[1],6)  ^ getbit(D[1],5)  ^
                getbit(D[1],2)  ^ getbit(D[0],31) ^ getbit(D[0],29) ^
                getbit(D[0],26) ^ getbit(D[0],25) ^ getbit(D[0],24) ^
                getbit(D[0],22) ^ getbit(D[0],21) ^ getbit(D[0],20) ^
                getbit(D[0],16) ^ getbit(D[0],14) ^ getbit(D[0],13) ^
                getbit(D[0],10) ^ getbit(D[0],8)  ^ getbit(D[0],7)  ^
                getbit(D[0],5)  ^ getbit(D[0],4)  ^ getbit(D[0],1);

    hash_bit[5] =
                getbit(D[2],10)  ^ getbit(D[2],8)  ^ getbit(D[2],4)  ^
                getbit(D[2],3)   ^ getbit(D[2],0)  ^
                getbit(D[1],31) ^
                getbit(D[1],30) ^ getbit(D[1],29) ^ getbit(D[1],27) ^
                getbit(D[1],24)  ^ getbit(D[1],23) ^ getbit(D[1],22) ^
                getbit(D[1],18)  ^ getbit(D[1],17) ^ getbit(D[1],15) ^
                getbit(D[1],14)  ^ getbit(D[1],13) ^ getbit(D[1],12) ^
                getbit(D[1],7)   ^ getbit(D[1],6)  ^ getbit(D[1],3)^
                getbit(D[1],0)   ^ getbit(D[0],30) ^ getbit(D[0],27) ^
                getbit(D[0],26)  ^ getbit(D[0],25) ^ getbit(D[0],23) ^
                getbit(D[0],22)  ^ getbit(D[0],21) ^ getbit(D[0],17) ^
                getbit(D[0],15)  ^ getbit(D[0],14) ^ getbit(D[0],11) ^
                getbit(D[0],9)   ^ getbit(D[0],8)  ^ getbit(D[0],6) ^
                getbit(D[0],5)   ^ getbit(D[0],2);

    hash_bit[6] =
                getbit(D[2],11) ^ getbit(D[2],9)  ^ getbit(D[2],5)  ^
                getbit(D[2],4)  ^ getbit(D[2],1)  ^ getbit(D[2],0)  ^
                getbit(D[1],31) ^ getbit(D[1],30) ^ getbit(D[1],28) ^
                getbit(D[1],25) ^ getbit(D[1],24) ^ getbit(D[1],23) ^
                getbit(D[1],19) ^ getbit(D[1],18) ^ getbit(D[1],16) ^
                getbit(D[1],15) ^ getbit(D[1],14) ^ getbit(D[1],13) ^
                getbit(D[1],8)  ^ getbit(D[1],7)  ^ getbit(D[1],4)  ^
                getbit(D[1],1)  ^ getbit(D[0],31) ^ getbit(D[0],28) ^
                getbit(D[0],27) ^ getbit(D[0],26) ^ getbit(D[0],24) ^
                getbit(D[0],23) ^ getbit(D[0],22) ^ getbit(D[0],18) ^
                getbit(D[0],16) ^ getbit(D[0],15) ^ getbit(D[0],12) ^
                getbit(D[0],10) ^ getbit(D[0],9)  ^ getbit(D[0],7)  ^
                getbit(D[0],6)  ^ getbit(D[0],3);

    hash_bit[7] =
                getbit(D[2],10) ^ getbit(D[2],6)  ^ getbit(D[2],5)  ^
                getbit(D[2],2)  ^ getbit(D[2],1)  ^ getbit(D[2],0)  ^
                getbit(D[1],31) ^ getbit(D[1],29) ^ getbit(D[1],26) ^
                getbit(D[1],25) ^ getbit(D[1],24) ^ getbit(D[1],20) ^
                getbit(D[1],19) ^ getbit(D[1],17) ^ getbit(D[1],16) ^
                getbit(D[1],15) ^ getbit(D[1],14) ^ getbit(D[1],9)  ^
                getbit(D[1],8)  ^ getbit(D[1],5)  ^ getbit(D[1],2)  ^
                getbit(D[1],0)  ^ getbit(D[0],29) ^ getbit(D[0],28) ^
                getbit(D[0],27) ^ getbit(D[0],25) ^ getbit(D[0],24) ^
                getbit(D[0],23) ^ getbit(D[0],19) ^ getbit(D[0],17) ^
                getbit(D[0],16) ^ getbit(D[0],13) ^ getbit(D[0],11) ^
                getbit(D[0],10) ^ getbit(D[0],8)  ^ getbit(D[0],7)  ^
                getbit(D[0],4);

    hash_bit[8] =
                getbit(D[2],11) ^ getbit(D[2],7)  ^ getbit(D[2],6)  ^
                getbit(D[2],3)  ^ getbit(D[2],2)  ^ getbit(D[2],1)  ^
                getbit(D[2],0)  ^
                getbit(D[1],30) ^ getbit(D[1],27) ^
                getbit(D[1],26) ^ getbit(D[1],25) ^ getbit(D[1],21) ^
                getbit(D[1],20) ^ getbit(D[1],18) ^ getbit(D[1],17) ^
                getbit(D[1],16) ^ getbit(D[1],15) ^ getbit(D[1],10) ^
                getbit(D[1],9)  ^ getbit(D[1],6)  ^ getbit(D[1],3)  ^
                getbit(D[1],1)  ^ getbit(D[0],30) ^ getbit(D[0],29) ^
                getbit(D[0],28) ^ getbit(D[0],26) ^ getbit(D[0],25) ^
                getbit(D[0],24) ^ getbit(D[0],20) ^ getbit(D[0],18) ^
                getbit(D[0],17) ^ getbit(D[0],14) ^ getbit(D[0],12) ^
                getbit(D[0],11) ^ getbit(D[0],9)  ^ getbit(D[0],8)  ^
                getbit(D[0],5);

    hash_bit[9] =
                getbit(D[2],8)  ^ getbit(D[2],7)  ^ getbit(D[2],4) ^
                getbit(D[2],3)  ^ getbit(D[2],2)  ^ getbit(D[2],1) ^
                getbit(D[1],31)^ getbit(D[1],28) ^ getbit(D[1],27) ^
                getbit(D[1],26)^ getbit(D[1],22) ^ getbit(D[1],21) ^
                getbit(D[1],19)^ getbit(D[1],18) ^ getbit(D[1],17) ^
                getbit(D[1],16)^ getbit(D[1],11) ^ getbit(D[1],10) ^
                getbit(D[1],7) ^ getbit(D[1],4)  ^ getbit(D[1],2)  ^
                getbit(D[0],31)^ getbit(D[0],30) ^ getbit(D[0],29) ^
                getbit(D[0],27)^ getbit(D[0],26) ^ getbit(D[0],25) ^
                getbit(D[0],21)^ getbit(D[0],19) ^ getbit(D[0],18) ^
                getbit(D[0],15)^ getbit(D[0],13) ^ getbit(D[0],12) ^
                getbit(D[0],10)^ getbit(D[0],9)  ^ getbit(D[0],6);

    hash_bit[10] =
                getbit(D[2],9)  ^ getbit(D[2],8)  ^ getbit(D[2],5)  ^
                getbit(D[2],4)  ^ getbit(D[2],3)  ^ getbit(D[2],2)  ^
                getbit(D[2],0)  ^
                getbit(D[1],29) ^ getbit(D[1],28) ^
                getbit(D[1],27) ^ getbit(D[1],23) ^ getbit(D[1],22) ^
                getbit(D[1],20) ^ getbit(D[1],19) ^ getbit(D[1],18) ^
                getbit(D[1],17) ^ getbit(D[1],12) ^ getbit(D[1],11) ^
                getbit(D[1],8)  ^ getbit(D[1],5)  ^ getbit(D[1],3)  ^
                getbit(D[1],0)  ^ getbit(D[0],31) ^ getbit(D[0],30) ^
                getbit(D[0],28) ^ getbit(D[0],27) ^ getbit(D[0],26) ^
                getbit(D[0],22) ^ getbit(D[0],20) ^ getbit(D[0],19) ^
                getbit(D[0],16) ^ getbit(D[0],14) ^ getbit(D[0],13) ^
                getbit(D[0],11) ^ getbit(D[0],10) ^ getbit(D[0],7);

    hash_bit[11] =
                getbit(D[2],11) ^ getbit(D[2],7)  ^ getbit(D[2],6)  ^
                getbit(D[2],5)  ^ getbit(D[2],3)  ^ getbit(D[2],2)  ^
                getbit(D[2],1)  ^ getbit(D[2],0)  ^
                getbit(D[1],31) ^
                getbit(D[1],26) ^ getbit(D[1],25) ^ getbit(D[1],21) ^
                getbit(D[1],19) ^ getbit(D[1],18) ^ getbit(D[1],12) ^
                getbit(D[1],11) ^ getbit(D[1],10) ^ getbit(D[1],7)  ^
                getbit(D[1],6)  ^ getbit(D[1],5)  ^ getbit(D[1],4)  ^
                getbit(D[1],3)  ^ getbit(D[1],2)  ^ getbit(D[0],29) ^
                getbit(D[0],27) ^ getbit(D[0],26) ^ getbit(D[0],25) ^
                getbit(D[0],23) ^ getbit(D[0],20) ^ getbit(D[0],19) ^
                getbit(D[0],18) ^ getbit(D[0],15) ^ getbit(D[0],13) ^
                getbit(D[0],9)  ^ getbit(D[0],8)  ^ getbit(D[0],7)  ^
                getbit(D[0],6)  ^ getbit(D[0],4)  ^ getbit(D[0],3)  ^
                getbit(D[0],1)  ^ getbit(D[0],0);

    hash_bit[12] =
                getbit(D[2],11) ^ getbit(D[2],10) ^ getbit(D[2],9)  ^
                getbit(D[2],8)  ^ getbit(D[2],6)  ^ getbit(D[2],3)  ^
                getbit(D[2],1)  ^
                getbit(D[1],31) ^ getbit(D[1],30) ^
                getbit(D[1],29) ^ getbit(D[1],28) ^ getbit(D[1],27) ^
                getbit(D[1],25) ^ getbit(D[1],24) ^ getbit(D[1],23) ^
                getbit(D[1],22) ^ getbit(D[1],19) ^ getbit(D[1],12) ^
                getbit(D[1],10) ^ getbit(D[1],9)  ^ getbit(D[1],8)  ^
                getbit(D[1],6)  ^ getbit(D[1],4)  ^ getbit(D[1],2)  ^
                getbit(D[1],1)  ^ getbit(D[1],0)  ^ getbit(D[0],31) ^
                getbit(D[0],30) ^ getbit(D[0],27) ^ getbit(D[0],25) ^
                getbit(D[0],24) ^ getbit(D[0],20) ^ getbit(D[0],18) ^
                getbit(D[0],17) ^ getbit(D[0],16) ^ getbit(D[0],13) ^
                getbit(D[0],12) ^ getbit(D[0],11) ^ getbit(D[0],10) ^
                getbit(D[0],8)  ^ getbit(D[0],6)  ^ getbit(D[0],5)  ^
                getbit(D[0],3)  ^ getbit(D[0],2)  ^ getbit(D[0],0);

    /* take care of init values */
    if (!isSip5)
    {
        hash_bit[6]  ^= 1;
        hash_bit[7]  ^= 1;
        hash_bit[8]  ^= 1;
        hash_bit[9]  ^= 1;
        hash_bit[10] ^= 1;
        hash_bit[11] ^= 1;
    }
};

/*******************************************************************************
* sip5CrcCalc
*
* DESCRIPTION:
*       This function calculates the CRC hash according to hash function type.
*       A length of data can be 44, 80, 96, 140 only
*
* INPUTS:
*       funcType    - hash function type:
*        one of : 16a,16b,16c,16d,32a,32b,32k,32q,64
*       numOfBits   - amount of bits in words data array
*                   not more then 140 bits
*       initCrcArr[2] - the CRC polynomial pattern (crc init value)
*       dataPtr[5]  - array of 32bit words, contain data.
*                     for 140 bits data needed 5 words.
*                     for 80 bits data needed 3 words.
*                     for 36 bits data needed 2 words.
*
* OUTPUTS:
*       hashBitArr[2]  - the hash result. (16 or 32 or 64 bits)
*
* RETURNS:
*       None
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static void sip5CrcCalc
(
    IN CRC_FUNC_ENT funcType,
    IN GT_U32  numOfBits,
    IN GT_U32  initCrcArr[2],
    IN GT_U32  dataArr[5],
    OUT GT_U32 hashBitArr[2]
)
{
    GT_U32  tmpInitCrc[2];/* temp initCrc as input to the interim hash calculations */
    GT_U32  tmpHash[2] = {0};  /* temp hash result between interim hash calculations */
    GT_U32  tmpData[2] = {0};  /* temp data for the specific interim hash calculation */

    GT_32   startBit = 0;        /* start of data chunk used for CRC calculation */
    GT_U32  len      = 0;        /* length of a data chunk */
    GT_BOOL need60   = GT_FALSE; /* if needed to calculate CRC for 60 bit data */
    GT_BOOL need44   = GT_FALSE; /* if needed to calculate CRC for 44 bit data */
    GT_BOOL need36   = GT_FALSE; /* if needed to calculate CRC for 36 bit data */
    CRC_FUNC_TYPE func;          /* pointer to a CRC function */

    /* the logic is to break the hash calc to cases:
       need 140 bits: calc as 60 + 44 + 36
       need 96  bits: calc as 60 + 36
       need 80  bits: calc as 44 + 36
       need 44  bits: cals as 44
    */

    switch (numOfBits)
    {
        case 140:
            need60 = GT_TRUE;
            need44 = GT_TRUE;
            need36 = GT_TRUE;
            break;
        case 96:
            need60 = GT_TRUE;
            need36 = GT_TRUE;
            break;
        case 80:
            need44 = GT_TRUE;
            need36 = GT_TRUE;
            break;
        default: /* 44 */
            need44 = GT_TRUE;
            break;
    }

    tmpInitCrc[0] = initCrcArr[0];
    tmpInitCrc[1] = initCrcArr[1];
    startBit = (GT_32)numOfBits;       /* set at the end of data */

    while (startBit > 0)
    {
        if (GT_TRUE == need60)
        {
            need60 = GT_FALSE;
            len = 60;
            func = crcFuncArray[funcType].calc60BitsData;
        }
        else if (GT_TRUE == need44)
        {
            need44 = GT_FALSE;
            len = 44;
            func = crcFuncArray[funcType].calc44BitsData;
        }
        else if (GT_TRUE == need36)
        {
            need36 = GT_FALSE;
            len = 36;
            func = crcFuncArray[funcType].calc36BitsData;
        }
        else
        {
            skernelFatalError("sip5CrcCalc: algorithm failure\n");
            return;
        }
        startBit -= len;
        tmpData[0] = snetFieldValueGet(dataArr, startBit,    32);
        tmpData[1] = snetFieldValueGet(dataArr, startBit+32,(len-32));
        func(tmpData,  tmpInitCrc, tmpHash);
        tmpInitCrc[0] = tmpHash[0];
        tmpInitCrc[1] = tmpHash[1];
    }

    hashBitArr[0] = tmpHash[0];
    hashBitArr[1] = tmpHash[1];
}

/*******************************************************************************
* hashCalcDataCrc
*
* DESCRIPTION:
*       This function calculates the hash index for the
*       mac address table using CRC algorithm.
*       for data of 76 bits.
*
* INPUTS:
*       dataPtr     - array of 32bit words, contain data.
*       numOfDataBits  - amount of bits in words data array
*       bitsWidth   - amount of used bits in the value .
*       isSip5      - use SIP5 algorithm for HASH function calculation
* OUTPUTS:
*       hashPtr - the hash index.
*
* RETURNS:
*       None
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static void hashCalcDataCrc
(
    IN  GT_U32          *D,
    IN  GT_U32          numOfDataBits,
    IN  GT_U32          bitsWidth,
    OUT GT_U32          *hashPtr,
    IN  GT_BIT          isSip5
)
{
    GT_U32    hash_bit[32] = {0};
    GT_U32    i; /* iterator */
    GT_BIT    alreadyGotHashAsBits = 0;
    GT_U32    initArr[2] = {0, 0};

    if (bitsWidth < 13) /*8K,16K*/
    {/* number of hash bits is 12 or 11 */
        crc_11_12(D, bitsWidth, hash_bit, isSip5);
    }
    else if (bitsWidth == 13)/*32K*/
    {
        crc_13(D, hash_bit, isSip5);
    }
    else/* >= 64K */
    {
        if (96 == numOfDataBits)
        {
            sip5CrcCalc(CRC_FUNC_16A_E, numOfDataBits,  initArr, D,
                        hash_bit);
        }
        else
        {
            /* CRC32a with 76bits data , without any 'init CRC value' */
            crc_32a_d76_noInitValueByNumBits(bitsWidth,D, hash_bit);
        }
        alreadyGotHashAsBits = 1;
    }

    if(alreadyGotHashAsBits == 0)
    {
        (*hashPtr) = 0;
        for(i = 0 ; i < bitsWidth; i++)
        {
            (*hashPtr) |= (hash_bit[i] & 0x01) << i;
        }
    }
    else
    {
        /* already got the hash as bits bmp (not as bit per word) */
        (*hashPtr) = hash_bit[0] & SMEM_BIT_MASK(bitsWidth);
    }

    (*hashPtr) *= 4;
}

/*******************************************************************************
* sip5CrcMultiHashCalc
*
* DESCRIPTION:
*       This function calculates the 16 CRC hashes for the 'multi hash'
*
* INPUTS:
*       dataPtr     - array of 32bit words, contain data.
*       numBitsInData   - amount of bits in words data array
*                     all 32 used in all words
*                     the bits used in the last are from MSB side
*       numBitsInHashResult - number of bits to generate into the hash results
*       initCrc     - the CRC polynomial pattern (crc init value)
*       numOfHashToCalc - the number of hash functions to generate
*                           (also it is the number of elements in calculatedHashArr[])
*
* OUTPUTS:
*       calculatedHashArr     - the hash array (16 hashes)
*
* RETURNS:
*       None
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static void sip5CrcMultiHashCalc
(
    IN  GT_U32   *dataPtr,
    IN  GT_U32   numBitsInData,
    IN  GT_U32   numBitsInHashResult,
    IN  GT_U32   initCrc,
    IN  GT_U32   numOfHashToCalc,
    OUT GT_U32   calculatedHashArr[FDB_MAX_BANKS_CNS]
)
{
    GT_U32  hashBitArr[2]={0,0};/* 2 words to support CRC64 */
    GT_U32  initCrcArr[2];/* 2 words to support CRC64 */
    GT_U32  bank; /* iterator for FDB bank - for the needed hash function to used */
    GT_U32  startBit;/* the start bit from the result of the current temp hash
                        to be taken as the calculatedHashArr[bank] */
    CRC_FUNC_ENT    funcType;/* the needed hash function to used */

    if(numOfHashToCalc > FDB_MAX_BANKS_CNS)
    {
        skernelFatalError("sip5CrcMultiHashCalc: not supported numOfHashToCalc [%d] \n",numOfHashToCalc);
    }

    /* prepare initCrc array */
    initCrcArr[0] = initCrc;
    initCrcArr[1] = initCrc;

    for(bank = 0; bank < numOfHashToCalc; bank++)
    {
        /* get values from the DB */
        funcType = crcFuncBitsUsedArr[bank].funcType;
        startBit = crcFuncBitsUsedArr[bank].startBit;

        if(startBit & BIT_AS_END_BIT_CNS)
        {
            /* this is special indication that the bit is the 'end bit+1' */
            /* remove the bit , and then decrement by numBitsInHashResult*/
            startBit = ((startBit & (~BIT_AS_END_BIT_CNS)) - numBitsInHashResult);
        }

        if(funcType != CRC_FUNC___DUMMY__E)/* the dummy are skipped */
        {
            sip5CrcCalc(funcType,numBitsInData,initCrcArr,dataPtr,hashBitArr);
        }

        /* get the needed amount of bits */
        calculatedHashArr[bank] =
            snetFieldValueGet(hashBitArr,startBit,numBitsInHashResult);
    }

    return;
}

/*******************************************************************************
* hashCalcSwapBits
*
* DESCRIPTION:
*       This function swaps bits in 32 bits word
*
* INPUTS:
*       data        - 32bit value to be bit-swapped.
*       bitsWidth   - amount of used bits in the value .
* OUTPUTS:
*       none
*
* RETURNS:
*       swapped value
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static GT_U32 hashCalcSwapBits
(
    IN  GT_U32          data,
    IN  GT_U32          bitsWidth
)
{
    GT_U32 idx;
    GT_U32 result ;

    data &= SMEM_BIT_MASK(bitsWidth);

    if (data == 0)
    {
        return 0 ;
    }

    result = 0 ;
    for (idx = 0; idx < bitsWidth; idx++)
    {
        result |= (((data >> idx) & 0x01) << (bitsWidth - idx - 1));
    }

    return result ;
}

#if 0

/*******************************************************************************
* hashCalcFillMacVidData11Xor
*
* DESCRIPTION:
*       This function prepares MAC+VID data
*       for calculating 11-bit hash using XOR algorithm
*
* INPUTS:
*       macAddrPtr  - MAC address to put to array (in network order) .
*       vlanId      - Vlan id  to put to array, for shared entry specify "0"
* OUTPUTS:
*       dataPtr     - 7-32bit-words array to receive data.
*
* RETURNS:
*       none
*
* COMMENTS:
*       MAC address bit numeration as network order:
*       00:01:02:04:08:10 - "0nes" in bits 15,22,29,36,41
*
*******************************************************************************/
static GT_VOID hashCalcFillMacVidData11Xor
(

    IN  GT_U8           *macAddrPtr,
    IN  GT_U32          vlanId,
    OUT GT_U32          *dataPtr
)
{
    GT_U32  ii=0;
    GT_U32  swappedVidBits;

    /* swap 12 bits of vid */
    swappedVidBits = hashCalcSwapBits(vlanId, 12);

    /* in all words used 11 LSB */
    dataPtr[0] = /* mac 00..10 */
        (macAddrPtr[4] << 8) | macAddrPtr[5];
    dataPtr[1] = /*mac 11..22 */
        (macAddrPtr[3] << 5) | (macAddrPtr[4] >> 3);
    dataPtr[2] = /* mac 23..33 */
        (macAddrPtr[3] >> 6) | (macAddrPtr[2] << 2) |
                 (macAddrPtr[1] << 10);
    dataPtr[3] = /* mac 34..44 */
         (macAddrPtr[1] >> 1) | (macAddrPtr[0] << 7);
    dataPtr[4] = /* mac 44..47 */
         macAddrPtr[0] >> 4;
    dataPtr[5] = (swappedVidBits & 1) << 10;   /* (VID[11]<<10) */
    dataPtr[6] = swappedVidBits >> 1;/* VID[10:0]  REVERSED !!!*/

    /* we need to swap also VID because it was manipulated to 2 indexes */
    for(ii = 0 ; ii < 7 ; ii++)
    {
        dataPtr[ii] = hashCalcSwapBits(dataPtr[ii], 11) ; /* cut value to 11 bits */
    }

}


/*******************************************************************************
* hashCalcFillDipSipVidData11Xor
*
* DESCRIPTION:
*       This function prepares MAC+VID data
*       for calculating 11-bit hash usind XOR algorithm
*
* INPUTS:
*       destIpAddr   - destination IP to put to array .
*       sourceIpAddr - source IP to put to array, if not used - specify "0"
*       vlanId       - Vlan id  to put to array, for shared entry specify "0"
* OUTPUTS:
*       dataPtr      - 7-32bit-words array to receive data.
*
* RETURNS:
*       none
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static GT_VOID hashCalcFillDipSipVidData11Xor
(
    IN  GT_U32          destIpAddr,
    IN  GT_U32          sourceIpAddr,
    IN  GT_U32          vlanId,
    OUT GT_U32          *dataPtr
)
{
    GT_U32  ii;
    GT_U32  swappedVidBits;

    /* swap 12 bits of vid */
    swappedVidBits = hashCalcSwapBits(vlanId, 12);

    dataPtr[0] = destIpAddr & 0x7FF;              /* dip[10:0] */
    dataPtr[0] ^= (swappedVidBits & 1) << 10; /* VID[11] should be
                                                xored with dip[10], dip[21]
                                                sip[10], sip[21], vid(0]*/

    dataPtr[1] = sourceIpAddr & 0x7FF;              /* sip[10:0] */
    dataPtr[2] = (destIpAddr >> 11) & 0x7FF;      /* dip[21:11] */
    dataPtr[3] = (sourceIpAddr >> 11) & 0x7FF;      /* sip[21:11] */
    dataPtr[4] = (destIpAddr >> 22) & 0x3FF;      /* dip[31:22] */
    dataPtr[5] = (sourceIpAddr >> 22) & 0x3FF;      /* sip[31:22] */
    dataPtr[6] = swappedVidBits >> 1;      /* VID[10:0]  REVERSED !!!*/

    /* we need to swap also VID because it was manipulated to 2 indexes */
    for(ii = 0 ; ii < 7 ; ii++)
    {
        dataPtr[ii] = hashCalcSwapBits(dataPtr[ii], 11) ; /* cut value to 11 bits */
    }
}
#endif

/*******************************************************************************
* hashCalcFillMacVidDataXor
*
* DESCRIPTION:
*       This function prepares MAC+VID data
*       for calculating numBits hash using XOR algorithm
*
* INPUTS:
*       fid16BitHashEn - indication if FID used 12 or 16 bits
*       macAddrPtr  - MAC address to put to array (in network order) .
*       vlanId      - Vlan id  to put to array, for shared entry specify "0"
* OUTPUTS:
*       dataPtr     - 7-32bit-words array to receive data.
*
* RETURNS:
*       none
*
* COMMENTS:
*       MAC address bit numeration as network order:
*       00:01:02:04:08:10 - "0nes" in bits 15,22,29,36,41
*
*******************************************************************************/
static GT_VOID hashCalcFillMacVidDataXor
(
    IN  GT_U32          numBits,
    IN  GT_BIT          fid16BitHashEn,
    IN  GT_U8           *macAddrPtr,
    IN  GT_U32          vlanId,
    OUT GT_U32          *dataPtr
)
{
    GT_U32  tmpWords[2];
    GT_U32  startBit = 0;
    GT_U32  maxWordSwapped = 3;
    GT_U32  ii;

    tmpWords[0] = GT_HW_SIM_MAC_LOW32(macAddrPtr);
    tmpWords[1] = GT_HW_SIM_MAC_HIGH16(macAddrPtr);

    dataPtr[0] = snetFieldValueGet(tmpWords,startBit,numBits);
    startBit += numBits;
    dataPtr[1] = snetFieldValueGet(tmpWords,startBit,numBits);
    startBit += numBits;
    dataPtr[2] = snetFieldValueGet(tmpWords,startBit,numBits);
    startBit += numBits;

    if(startBit >= 48) /* we already got 48 bits of MAC */
    {
        dataPtr[3] = 0;
        maxWordSwapped --;
    }
    else
    {
        dataPtr[3] = snetFieldValueGet(tmpWords,startBit,numBits);
    }

    dataPtr[4] = 0 ;

    if(fid16BitHashEn == 0)
    {
        dataPtr[5] = ((vlanId & SMEM_BIT_MASK(numBits-12)) << 12) |/*additional bits*/
                      (vlanId & SMEM_BIT_MASK(12));                /*12 bits*/
        dataPtr[6] = 0 ;
    }
    else
    {
        tmpWords[0] = vlanId;
        startBit = 0;
        dataPtr[5] = snetFieldValueGet(tmpWords,startBit,numBits);
        startBit += numBits;
        dataPtr[6] = snetFieldValueGet(tmpWords,startBit,numBits);
    }

    /* swap bits in MAC address (but no in vlanId) */
    /* swap bits in words 0..3 , -- not in 4 - not in 5,6 (bits of the FID)*/
    for(ii = 0 ; ii <= maxWordSwapped ; ii ++)
    {
        dataPtr[ii] = hashCalcSwapBits(dataPtr[ii], numBits);
    }
}

/*******************************************************************************
* hashCalcFillDipSipVidDataXor
*
* DESCRIPTION:
*       This function prepares DIP+SIP+VID data
*       for calculating numBits hash using XOR algorithm
*
* INPUTS:
*       fid16BitHashEn - indication if FID used 12 or 16 bits
*       destIpAddr   - destination IP to put to array .
*       sourceIpAddr - source IP to put to array, if not used - specify "0"
*       vlanId       - Vlan id  to put to array, for shared entry specify "0"
* OUTPUTS:
*       dataPtr      - 7-32bit-words array to receive data.
*
* RETURNS:
*       none
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static GT_VOID hashCalcFillDipSipVidDataXor
(
    IN  GT_U32          numBits,
    IN  GT_BIT          fid16BitHashEn,
    IN  GT_U32          destIpAddr,
    IN  GT_U32          sourceIpAddr,
    IN  GT_U32          vlanId,
    OUT GT_U32          *dataPtr
)
{
    GT_U32  startBit = 0;
    GT_U32  tmpWords[2];
    GT_U32  ii;

    /*start with DIP */
    tmpWords[0] = destIpAddr;
    tmpWords[1] = 0;

    dataPtr[0] = snetFieldValueGet(tmpWords,startBit,numBits);
    startBit += numBits;
    dataPtr[1] = snetFieldValueGet(tmpWords,startBit,numBits);
    startBit += numBits;
    if(startBit >= 32) /* we already got 32 bits of DIP */
    {
        dataPtr[2] = 0;
    }
    else
    {
        dataPtr[2] = snetFieldValueGet(tmpWords,startBit,numBits);
    }

    /*continue with DIP */
    tmpWords[0] = sourceIpAddr;
    startBit = 0;

    dataPtr[3] = snetFieldValueGet(tmpWords,startBit,numBits);
    startBit += numBits;
    dataPtr[4] = snetFieldValueGet(tmpWords,startBit,numBits);
    startBit += numBits;
    if(startBit >= 32) /* we already got 32 bits of SIP */
    {
        dataPtr[5] = 0;
    }
    else
    {
        dataPtr[5] = snetFieldValueGet(tmpWords,startBit,numBits);
    }

    if(fid16BitHashEn == 0)
    {
        dataPtr[6] = ((vlanId & SMEM_BIT_MASK(numBits-12)) << 12) |/*additional bits*/
                      (vlanId & SMEM_BIT_MASK(12));                /*12 bits*/
    }
    else
    {
        tmpWords[0] = vlanId;
        startBit = 0;
        dataPtr[6] = snetFieldValueGet(tmpWords,startBit,numBits);
        startBit += numBits;

        if(startBit >= 16) /* we already got 16 bits of FID */
        {
            dataPtr[7] = 0;
        }
        else
        {
            dataPtr[7] = snetFieldValueGet(tmpWords,startBit,numBits);
        }
    }

    /* swap bits in words 0..5  -- not in 6,7 (FID info)*/
    for(ii = 0 ; ii <= 5 ; ii ++)
    {
        dataPtr[ii] = hashCalcSwapBits(dataPtr[ii], numBits);
    }
}

/*******************************************************************************
* hashCalcFillMacDoubleVidDataCrc
*
* DESCRIPTION:
*       This function prepares MAC Address[47:0], FID[15:0] , vid1[11:0] data
*       for calculating hash using CRC algorithm
*       Generate data of 80 bits.
*
* INPUTS:
*       macAddrPtr  - MAC address to put to array (in network order) .
*       fid         - FID to put to array
*       vid1        - VID1  to put to array
*
* OUTPUTS:
*       dataPtr     - 3-32bit-words array to receive data.(for 80 bits)
*
* RETURNS:
*       none
*
* COMMENTS:
*
*******************************************************************************/
static GT_VOID hashCalcFillMacDoubleVidDataCrc
(
    IN  GT_U8           *macAddrPtr,
    IN  GT_U32          fid,
    IN  GT_U32          vid1,
    OUT GT_U32          *dataPtr
)
{
    GT_U32  lowWordMacAddress = GT_HW_SIM_MAC_LOW32(macAddrPtr);
                                  /* low 32 bits of mac address */

    GT_U32  hiHalfWordMacAddress = GT_HW_SIM_MAC_HIGH16(macAddrPtr);
                                  /* hi 16 bits of mac address*/
    GT_U32  tmpBitIndex;

    /* If looking for entry type = MAC then Data_to_CRC_Func = {MAC Address[47:0], FID[15:0] , vid1[11:0]} */
    tmpBitIndex = 0;
    snetFieldValueSet(dataPtr,  tmpBitIndex,             12,  vid1);
    tmpBitIndex += 12;
    snetFieldValueSet(dataPtr,  tmpBitIndex,             16,  fid);
    tmpBitIndex += 16;
    snetFieldValueSet(dataPtr,  tmpBitIndex,             32,  lowWordMacAddress);
    tmpBitIndex += 32;
    snetFieldValueSet(dataPtr,  tmpBitIndex,             16,  hiHalfWordMacAddress);
    tmpBitIndex += 16;
    /* Upper bits 76..79 */
    snetFieldValueSet(dataPtr,  tmpBitIndex,              4,  0);
}

/*******************************************************************************
* hashCalcFillDipSipDoubleVidDataCrc
*
* DESCRIPTION:
*       This function prepares SIP+DIP+FID+vid1+data
*       for calculating hash using CRC algorithm
*       Generate data of 80 bits.
*
* INPUTS:
*       destIpAddr   - destination IP to put to array .
*       sourceIpAddr - source IP to put to array, if not used - specify "0"
*       fid          - fid to put to array
*       vid1         - vid1 to put to array
* OUTPUTS:
*       dataPtr      - 3-32bit-words array to receive data.(for 96 bits)
*
* RETURNS:
*       none
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static GT_VOID hashCalcFillDipSipDoubleVidDataCrc
(
    IN  GT_U32          destIpAddr,
    IN  GT_U32          sourceIpAddr,
    IN  GT_U32          fid,
    IN GT_U32           vid1,
    OUT GT_U32          *dataPtr
)
{
    GT_U32  tmpBitIndex;

    /*  IPv4 MC or IPv6 MC then Data_to_CRC_Func = {SIP[31:0], DIP[31:0] , FID[15:0] , vid1[11:0]} */
    tmpBitIndex = 0;
    snetFieldValueSet(dataPtr,tmpBitIndex,  12, vid1);
    tmpBitIndex += 12;
    snetFieldValueSet(dataPtr,tmpBitIndex,  16, fid);
    tmpBitIndex += 16;
    snetFieldValueSet(dataPtr,tmpBitIndex,  32, destIpAddr);
    tmpBitIndex += 32;
    snetFieldValueSet(dataPtr,tmpBitIndex,  32, sourceIpAddr);
    tmpBitIndex += 32;
    /* Upper bits 92..95 */
    snetFieldValueSet(dataPtr,tmpBitIndex,  4,  0);
}

/*******************************************************************************
* hashCalcFillMacVidDataCrc
*
* DESCRIPTION:
*       This function prepares MAC+VID data
*       for calculating hash using CRC algorithm
*       Generate data of 76/80 bits.
*
* INPUTS:
*       macAddrPtr  - MAC address to put to array (in network order) .
*       vlanId      - Vlan id  to put to array, for shared entry specify "0"
*       crcHashUpperBitsMode - 16 MSbits mode for of DATA into the hash function
*                               relevant only to CRC for MAC+FID mode.
* OUTPUTS:
*       dataPtr     - 3-32bit-words array to receive data.(for 76/80 bits)
*
* RETURNS:
*       none
*
* COMMENTS:
*
*******************************************************************************/
static GT_VOID hashCalcFillMacVidDataCrc
(
    IN  GT_BIT          fid16BitHashEn,
    IN  GT_U8           *macAddrPtr,
    IN  GT_U32          vlanId,
    OUT GT_U32          *dataPtr,
    IN  SNET_CHEETAH_FDB_CRC_HASH_UPPER_BITS_MODE_ENT crcHashUpperBitsMode
)
{
    GT_U32  lowWordMacAddress = GT_HW_SIM_MAC_LOW32(macAddrPtr);
                                  /* low 32 bits of mac address */

    GT_U32  hiHalfWordMacAddress = GT_HW_SIM_MAC_HIGH16(macAddrPtr);
                                  /* hi 16 bits of mac address*/
    GT_U32  tmpBitIndex;
    GT_U32  fid;

    if(fid16BitHashEn)
    {
        fid = vlanId;
        tmpBitIndex = 16;
    }
    else
    {
        fid = vlanId & 0xFFF;
        tmpBitIndex = 12;
    }

    /* 16 zeros, 48bit MAC, 12bit VID */

    snetFieldValueSet(dataPtr,0           ,tmpBitIndex  ,fid);
    snetFieldValueSet(dataPtr,tmpBitIndex,32            ,lowWordMacAddress);
    tmpBitIndex += 32;
    snetFieldValueSet(dataPtr,tmpBitIndex,16            ,hiHalfWordMacAddress);
    tmpBitIndex += 16;

    switch(crcHashUpperBitsMode)
    {
        default:
        case SNET_CHEETAH_FDB_CRC_HASH_UPPER_BITS_MODE_ALL_ZERO_E:
            snetFieldValueSet(dataPtr,tmpBitIndex,16,0);/*64..79 or 60..75 */
            break;
        case SNET_CHEETAH_FDB_CRC_HASH_UPPER_BITS_MODE_FID_E:
            snetFieldValueSet(dataPtr,tmpBitIndex,16,fid);/*64..79 or 60..75 */
            break;
        case SNET_CHEETAH_FDB_CRC_HASH_UPPER_BITS_MODE_MAC_E:
            snetFieldValueSet(dataPtr,tmpBitIndex,16,lowWordMacAddress);/*64..79 or 60..75 */
            break;
    }

    if(fid16BitHashEn == 0)
    {
        snetFieldValueSet(dataPtr,76,4,0);/*76..79*/
    }

}

/*******************************************************************************
* hashCalcFillCommonDataCrc
*
* DESCRIPTION:
*       This function prepares common data
*       for calculating hash using CRC algorithm
*       Generate data of 80 bits.
*
* INPUTS:
*       vlanLookupMode          -  0 - without VLAN,
*                               -  1 - vlan + mac - relevant only for MAC based HASH
*       fdbHashInfoPtr     - (pointer to) entry hash info

*
* OUTPUTS:
*       hashInputDataPtr     - 3-32bit-words array to receive data.(for 76/80 bits)
*       dataNumOfBitsPtr     - data for calculating hash
*
* RETURNS:
*       none
*
* COMMENTS:
*
*******************************************************************************/
static GT_VOID hashCalcFillCommonDataCrc
(
    IN GT_U32                                   vlanLookupMode,
    IN SNET_CHEETAH_FDB_ENTRY_HASH_INFO_STC     *fdbHashInfoPtr,
    OUT GT_U32                                  *hashInputDataPtr,
    OUT GT_U32                                  *dataNumOfBitsPtr

)
{
    GT_U16  fid;

    /* The case is for sip5_10 only */
    if (fdbHashInfoPtr->fdbLookupKeyMode /* use fid+vid1 */ &&
        ( vlanLookupMode /*IVL*/ || (fdbHashInfoPtr->entryType != SNET_CHEETAH_FDB_ENTRY_MAC_E)))
    {
        /* FDB Lookup key is <MAC, FID, VID1> */
        if (fdbHashInfoPtr->entryType == SNET_CHEETAH_FDB_ENTRY_MAC_E)
        {
            /* FDB entry type: <MAC Address, FID, vid1> */
            hashCalcFillMacDoubleVidDataCrc(fdbHashInfoPtr->info.macInfo.macAddr,
                                            fdbHashInfoPtr->fid,
                                            fdbHashInfoPtr->vid1,
                                            hashInputDataPtr);
            *dataNumOfBitsPtr = 80;
        }
        else
        {
            /* FDB entry type: <SIP, DIP, FID, vid1> */
            hashCalcFillDipSipDoubleVidDataCrc(fdbHashInfoPtr->info.ipmcBridge.dip,
                                            fdbHashInfoPtr->info.ipmcBridge.sip,
                                            fdbHashInfoPtr->fid,
                                            fdbHashInfoPtr->vid1,
                                            hashInputDataPtr) ;
            *dataNumOfBitsPtr = 96;
        }
    }
    else
    {

        if (fdbHashInfoPtr->entryType == SNET_CHEETAH_FDB_ENTRY_MAC_E)
        {
            /* FDB entry type: <MAC Address, FID> */
            fid = (vlanLookupMode == 0 )/* SVL*/ ?  0 : fdbHashInfoPtr->fid;

            hashCalcFillMacVidDataCrc(fdbHashInfoPtr->fid16BitHashEn,
                fdbHashInfoPtr->info.macInfo.macAddr,
                fid,
                hashInputDataPtr,
                fdbHashInfoPtr->info.macInfo.crcHashUpperBitsMode) ;
        }
        else
        {
            /* FDB entry type: <SIP, DIP, FID> */
            hashCalcFillDipSipVidDataCrc(fdbHashInfoPtr->fid16BitHashEn,
                fdbHashInfoPtr->info.ipmcBridge.dip,
                fdbHashInfoPtr->info.ipmcBridge.sip,
                fdbHashInfoPtr->fid,
                hashInputDataPtr) ;
        }

        *dataNumOfBitsPtr = 80;
    }

}

/*******************************************************************************
* hashCalcFillDipV6VfrIdDataCrc
*
* DESCRIPTION:
*       This function prepares DIPv6+VRFID data for calculating hash using CRC algorithm
*       Generates data of 140 bits.
*
* INPUTS:
*       destIpAddr   - destination IP to put to array .
*       vrfId        - vrfId
*
* OUTPUTS:
*       dataPtr      - 5-32bit-words array to receive data.(for 140 bits)
*
*******************************************************************************/
static GT_VOID hashCalcFillDipV6VfrIdDataCrc
(
    IN  GT_U32   destIpAddr[],
    IN  GT_U32   vrfId,
    OUT GT_U32  *dataPtr
)
{
    GT_U32  i = 0;

    /* set ipv6 DIP */
    for(i = 0; i < 4; i++)
    {
        snetFieldValueSet(dataPtr, i*32, 32, destIpAddr[3-i] );
    }

    /* set vrfId */
    snetFieldValueSet(dataPtr, 128, 12, vrfId);
}

/*******************************************************************************
* hashCalcFillDipV4VfrIdDataCrc
*
* DESCRIPTION:
*       This function prepares DIPv4+VRFID data for calculating hash using CRC algorithm
*       Generates data of 44 bits.
*
* INPUTS:
*       destIpAddr   - destination IP to put to array .
*       vrfId        - vrfId
*
* OUTPUTS:
*       dataPtr      - 2-32bit-words array to receive data.(for 44 bits)
*
*******************************************************************************/
static GT_VOID hashCalcFillDipV4VfrIdDataCrc
(
    IN  GT_U32   destIpAddr,
    IN  GT_U32   vrfId,
    OUT GT_U32  *dataPtr
)
{
    /* set ipv4 DIP/FCoE */
    snetFieldValueSet(dataPtr, 0, 32, destIpAddr);

    /* set vrfId */
    snetFieldValueSet(dataPtr, 32, 12, vrfId);
}

/*******************************************************************************
* hashCalcFillDipSipVidDataCrc
*
* DESCRIPTION:
*       This function prepares MAC+VID data
*       for calculating hash using CRC algorithm
*       Generate data of 76/80 bits.
*
* INPUTS:
*       destIpAddr   - destination IP to put to array .
*       sourceIpAddr - source IP to put to array, if not used - specify "0"
*       vlanId       - Vlan id  to put to array, for shared entry specify "0"
* OUTPUTS:
*       dataPtr      - 3-32bit-words array to receive data.(for 76/80 bits)
*
* RETURNS:
*       none
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static GT_VOID hashCalcFillDipSipVidDataCrc
(
    IN  GT_BIT          fid16BitHashEn,
    IN  GT_U32          destIpAddr,
    IN  GT_U32          sourceIpAddr,
    IN  GT_U32          vlanId,
    OUT GT_U32          *dataPtr
)
{
    GT_U32  tmpBitIndex;
    GT_U32  fid;

    if(fid16BitHashEn)
    {
        fid = vlanId;
        tmpBitIndex = 16;
    }
    else
    {
        fid = vlanId & 0xFFF;
        tmpBitIndex = 12;
    }

    snetFieldValueSet(dataPtr,0,tmpBitIndex,fid);
    snetFieldValueSet(dataPtr,tmpBitIndex,32,destIpAddr);
    tmpBitIndex += 32;
    snetFieldValueSet(dataPtr,tmpBitIndex,32,sourceIpAddr);
    tmpBitIndex += 32;
    if(fid16BitHashEn == 0)
    {
        snetFieldValueSet(dataPtr,76,4,0);/*76..79*/
    }
}

/*******************************************************************************
* cheetahMacHashCalcEnhanced
*
* DESCRIPTION:
*       This function prepares MAC+VID data for calculating 12-bit hash
*
* INPUTS:
*       hashType       - 1 - CRC , or 0- XOR
*       vlanLookupMode - 0 - without VLAN,
*                        1 - vlan + mac - relevant only for MAC based HASH
*       fdbEntryHashInfoPtr
*                        - (pointer to) FDB entry hash info
*
*       numBitsToUse   - number of bits used to address FDB mac entry in FDB table,
*                        depends on FDB size (11 - 8K entries, 12 - 16K entries,
*                        13 - 36K entries)
* OUTPUTS:
*       none
*
* RETURNS:
*       hash value
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_U32 cheetahMacHashCalcEnhanced
(
    IN GT_U32  hashType,
    IN GT_U32  vlanLookupMode,
    IN SNET_CHEETAH_FDB_ENTRY_HASH_INFO_STC *fdbEntryHashInfoPtr,
    IN GT_U32  numBitsToUse
)
{
    GT_U32 data[8] = {0};
    GT_U32 hash = 0;
    GT_U32  xor_numOfWords;/* in XOR mode - number of words used from data[]*/
    GT_U32  crc_numOfDataBits;/* in CRC mode - number of bits to used from data[]*/

    GT_U32  fid = fdbEntryHashInfoPtr->fid;
    GT_U32  dataLen;

    if(fdbEntryHashInfoPtr->fid16BitHashEn)
    {
        xor_numOfWords = 8;
        crc_numOfDataBits = 76;/*76 bits data for the CRC , although needed 80 bits ! */
    }
    else
    {
        xor_numOfWords = 7;
        crc_numOfDataBits = 76;/*76 bits data for the CRC */
    }

    switch((hashType << 8) | fdbEntryHashInfoPtr->entryType)
    {
        case 0x0000:  /* XOR + L2 MAC*/
            if (vlanLookupMode == 0)
                fid = 0 ;

            xor_numOfWords = 7;
            hashCalcFillMacVidDataXor(numBitsToUse,fdbEntryHashInfoPtr->fid16BitHashEn,
                fdbEntryHashInfoPtr->info.macInfo.macAddr, fid, &(data[0])) ;

            hashCalcDataXor(
                &(data[0]), xor_numOfWords, &hash) ;

            if(numBitsToUse == 10)
            {
                hash = hash / 2;
            }

            break;


        case 0x0001: /* XOR + IPV4 IGMP */
        case 0x0002: /* XOR + IPV6 MLD  */

            hashCalcFillDipSipVidDataXor(numBitsToUse,fdbEntryHashInfoPtr->fid16BitHashEn,
                fdbEntryHashInfoPtr->info.ipmcBridge.dip, fdbEntryHashInfoPtr->info.ipmcBridge.sip, fid, &(data[0])) ;

            hashCalcDataXor(
                &(data[0]), xor_numOfWords, &hash) ;

            if(numBitsToUse == 10)
            {
                hash = hash / 2;
            }

            break;


    case 0x0100: /* CRC + L2 MAC*/
    case 0x0101: /* CRC + IPV4 IGMP */
    case 0x0102: /* CRC + IPV6 MLD  */
        hashCalcFillCommonDataCrc(vlanLookupMode, fdbEntryHashInfoPtr, data, &dataLen);
        if (96 == dataLen)
        {
            /* case of {SIP, DIP, FID, VID1} is processed in special way */
            crc_numOfDataBits = 96;
        }
        hashCalcDataCrc(
            &(data[0]), crc_numOfDataBits, numBitsToUse, &hash, fdbEntryHashInfoPtr->isSip5) ;
        break;

    default:
        /* Not supported */
        skernelFatalError("cheetahMacHashCalcEnhanced: not supported hashType[%d] fdbEntryType[%d] \n",
                    hashType, fdbEntryHashInfoPtr->entryType);
        break;
    }

    return hash;
}

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
)
{
    SNET_CHEETAH_FDB_ENTRY_HASH_INFO_STC fdbEntryHashInfo;

    fdbEntryHashInfoGet(devObjPtr,fdbEntryType,fid,macAddrPtr,sourceIpAddr,destIpAddr, &fdbEntryHashInfo);

    return cheetahMacHashCalcEnhanced(hashType,vlanLookupMode, &fdbEntryHashInfo, numBitsToUse);
}


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
)
{
    return cheetahMacHashCalcEnhanced(hashType,vlanLookupMode,
                                      entryInfoPtr, numBitsToUse);
}

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
)
{
    GT_U32  hashInputDataArr[5] = {0};/* support 140 bits data */
    GT_U32  initCrc = 0xFFFFFFFF;/* use hard coded instead of read from internal register */
    GT_U32  inputDataNumOfBits;

    /* the caller give 'num of bits' as if using 'XOR/FDB' that need 2 bits more
       then the multi hash */
    numBitsToUse -= 2;

    switch(entryInfoPtr->entryType)
    {

        case SNET_CHEETAH_FDB_ENTRY_MAC_E:
        case SNET_CHEETAH_FDB_ENTRY_IPV4_IPMC_BRIDGING_E:
        case SNET_CHEETAH_FDB_ENTRY_IPV6_IPMC_BRIDGING_E:
            hashCalcFillCommonDataCrc(vlanLookupMode, entryInfoPtr, hashInputDataArr, &inputDataNumOfBits) ;
            break;

        case SNET_CHEETAH_FDB_ENTRY_IPV4_UC_ROUTING_E:/*Unicast routing IPv4 address entry.*/
        case SNET_CHEETAH_FDB_ENTRY_FCOE_UC_ROUTING_E:/*Unicast routing FCOE address entry.*/
            hashCalcFillDipV4VfrIdDataCrc(entryInfoPtr->info.ucRouting.dip[0],
                                          entryInfoPtr->info.ucRouting.vrfId,
                                          hashInputDataArr) ;
            inputDataNumOfBits = 44;
            break;

        case SNET_CHEETAH_FDB_ENTRY_IPV6_UC_ROUTING_KEY_E:/*Unicast routing IPv6 address lookup-key entry.*/
            hashCalcFillDipV6VfrIdDataCrc(entryInfoPtr->info.ucRouting.dip,
                                          entryInfoPtr->info.ucRouting.vrfId,
                                          hashInputDataArr) ;
            inputDataNumOfBits = 140;
            break;

        default:
            /* not supported */
            skernelFatalError("sip5MacHashCalcMultiHash: not supported entry type [%d] \n",entryInfoPtr->entryType);
            return ;
    }

    sip5CrcMultiHashCalc(hashInputDataArr,inputDataNumOfBits,numBitsToUse,initCrc,numOfHashToCalc,calculatedHashArr);

    return;
}

