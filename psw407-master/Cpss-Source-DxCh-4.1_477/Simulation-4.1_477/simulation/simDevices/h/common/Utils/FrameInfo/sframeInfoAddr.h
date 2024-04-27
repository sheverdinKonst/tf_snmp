/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* sframeInfoAddr.h
*
* DESCRIPTION:
*
*       Information and macros for frame
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 17 $
*
*******************************************************************************/
#ifndef __sframeinfoh
#define __sframeinfoh

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef __GNUC__
typedef struct __attribute__ ((__packed__)) {
    GT_U32     word1;
    GT_U16     hword2;
} SGT_MAC_ADDR_WORDS_STC;

typedef union __attribute__ ((__packed__)) {
    GT_U8                   bytes[6];
    SGT_MAC_ADDR_WORDS_STC  w;
} SGT_MAC_ADDR_UNT;

#else
typedef struct {
    GT_U32     word1;
    GT_U16     hword2;
} SGT_MAC_ADDR_WORDS_STC;

typedef union {
    GT_U8                   bytes[6];
    SGT_MAC_ADDR_WORDS_STC  w;
} SGT_MAC_ADDR_UNT;
#endif

typedef struct {
    GT_U32     word1;
    GT_U32     word2;
    GT_U32     word3;
} SGT_IP_HEADER_TOP_STC;

typedef struct {
    GT_U32     headerChecksum  :16;
    GT_U32     protocol        :8;
    GT_U32     timeToLive      :8;
} SGT_IP_HEADER_WORD_STC;

#define SGT_MAC_ADDR_TYP    SGT_MAC_ADDR_UNT

#if !defined(BE_HOST) /* i.e. simulation running on little endian CPU - LE_HOST */

/* swap bytes inside a half word (GT_U16) */
#define SGT_LIB_SWAP_BYTES_MAC(x)                                   \
    ( (GT_U16)(((GT_U16)(x) << 8 ) + ((GT_U16)(x) >> 8 )) )

#define SGT_LIB_SWAP16_MAC(val)   \
    (GT_U16)(((GT_U16)(val) << 8) + ((GT_U16)(val) >> 8))

/* swap half words (GT_U16) inside a word (UINT_32 ) */
#define SGT_LIB_SWAP_WORDS_MAC(x)                                   \
    ( (GT_U32)(((GT_U32)(x) << 16 ) + ((GT_U32)(x) >> 16 )) )

/* swap bytes and half word inside a word */
#define SGT_LIB_SWAP_BYTES_AND_WORDS_MAC(x)                         \
       ((((x) >>  0) & 0xFF) << 24) |                               \
       ((((x) >>  8) & 0xFF) << 16) |                               \
       ((((x) >> 16) & 0xFF) <<  8) |                               \
       ((((x) >> 24) & 0xFF) <<  0)
/*    ( (GT_U32)(((GT_U32)(SGT_LIB_SWAP_BYTES_MAC(x)) << 16 ) +     \
    (SGT_LIB_SWAP_BYTES_MAC((GT_U32)(x) >> 16 ))) )*/

#else /* BE_HOST */

#define SGT_LIB_SWAP_BYTES_MAC(x)   (x)

#define SGT_LIB_SWAP16_MAC(val)     (val)

#define SGT_LIB_SWAP_WORDS_MAC(x)   (x)

#define SGT_LIB_SWAP_BYTES_AND_WORDS_MAC(x) (x)

#endif

/* Convert byte sequence of mac to SGT_MAC_ADDR_TYP type */
#define SGT_MAC_ADDR_GET_FROM_STRING(addr_PTR, str_PTR) \
{ \
    GT_U8 * strPtr = (GT_U8 *)str_PTR;  \
    GT_U16 hWord = *((GT_U16 *)(strPtr));  \
    GT_U32 lWord = *((GT_U32 *)(strPtr + 2));  \
    ((SGT_MAC_ADDR_TYP *)(addr_PTR))->w.hword2 = \
            SGT_LIB_SWAP16_MAC(hWord);  \
    ((SGT_MAC_ADDR_TYP *)(addr_PTR))->w.word1 = \
            SGT_LIB_SWAP_BYTES_AND_WORDS_MAC(lWord); \
}

#define SGT_MAC_ADDR_ARE_EQUAL(addr1_PTR, addr2_PTR )               \
    ( (((SGT_MAC_ADDR_TYP *)(addr1_PTR))->w.word1 ==                    \
                        ((SGT_MAC_ADDR_TYP *)(addr2_PTR))->w.word1) &&  \
      (((SGT_MAC_ADDR_TYP *)(addr1_PTR))->w.hword2 ==                   \
                        ((SGT_MAC_ADDR_TYP *)(addr2_PTR))->w.hword2) )

#define SGT_MAC_ADDR_APPLY_MASK(addr1_PTR, addr2_PTR )               \
     (((SGT_MAC_ADDR_TYP *)(addr1_PTR))->w.word1 &=                    \
                        ((SGT_MAC_ADDR_TYP *)(addr2_PTR))->w.word1);  \
      (((SGT_MAC_ADDR_TYP *)(addr1_PTR))->w.hword2 &=                   \
                        ((SGT_MAC_ADDR_TYP *)(addr2_PTR))->w.hword2);

#define SGT_MAC_ADDR_IS_NULL(addr_PTR)                              \
    ( (((SGT_MAC_ADDR_TYP *)(addr_PTR))->w.word1 ==  0) &&              \
      (((SGT_MAC_ADDR_TYP *)(addr_PTR))->w.hword2 == 0) )

#define SGT_MAC_ADDR_IS_BCST(addr_PTR)                            \
    ( (((SGT_MAC_ADDR_TYP *)(addr_PTR))->w.word1 ==  0xFFFFFFFF) &&     \
      (((SGT_MAC_ADDR_TYP *)(addr_PTR))->w.hword2 == 0xFFFF) )

#define SGT_MAC_ADDR_IS_MCST(addr_PTR)                            \
    ( ((SGT_MAC_ADDR_TYP *)(addr_PTR))->bytes[0] & 0x01 )

#define SGT_MAC_ADDR_IS_MCST_BY_FIRST_BYTE(byte)  \
                            ( byte & 0x01 )

#define SGT_MAC_ADDR_ADD_MAC(addr_PTR,n)                                \
    ( ((RSG_mac_addr_TYP *)(addr_PTR))->w.hword2  =                     \
        SGT_LIB_SWAP16_MAC(                                                 \
            SGT_LIB_SWAP16_MAC(((SGT_MAC_ADDR_TYP *)(addr_PTR))->w.hword2)+n) )

#define SGT_MAC_ADDR_SUB_MAC(addr1_PTR,addr2_PTR)                       \
    ( SGT_LIB_SWAP16_MAC(((SGT_MAC_ADDR_TYP *)(addr1_PTR))->w.hword2) -     \
        SGT_LIB_SWAP16_MAC(((SGT_MAC_ADDR_TYP *)(addr2_PTR))->w.hword2) )

#define SGT_MAC_ADDR_COPY_MAC(addr1_PTR,addr2_PTR)                      \
     *(SGT_MAC_ADDR_TYP *)(addr2_PTR) = *(SGT_MAC_ADDR_TYP *)(addr1_PTR)

#define SGT_MAC_ADDR_DIFF_MAC(addr1_PTR,addr2_PTR)                      \
    ( ( ((SGT_MAC_ADDR_TYP *)(addr1_PTR))->w.word1 ==                   \
        ((SGT_MAC_ADDR_TYP *)(addr2_PTR))->w.word1 ) ?                  \
      ( SGT_LIB_SWAP16_MAC(((SGT_MAC_ADDR_TYP *)(addr1_PTR))->w.hword2) -   \
        SGT_LIB_SWAP16_MAC(((SGT_MAC_ADDR_TYP *)(addr2_PTR))->w.hword2) ) : \
        MAX_UINT_16 )

#define SGT_MAC_ADDR_GET_LSB_OCTET(addr1_PTR)                      \
    ( ((SGT_MAC_ADDR_TYP *)(addr1_PTR))->bytes[5] )


#define SGT_MAC_ADDR_IS_GREATER(addr1_PTR, addr2_PTR)           \
    ( (   ((SGT_MAC_ADDR_TYP *)(addr1_PTR))->w.word1 >              \
          ((SGT_MAC_ADDR_TYP *)(addr2_PTR))->w.word1     ) ||       \
      ( ( ((SGT_MAC_ADDR_TYP *)(addr1_PTR))->w.word1 ==             \
          ((SGT_MAC_ADDR_TYP *)(addr2_PTR))->w.word1 ) &&           \
        ( ((SGT_MAC_ADDR_TYP *)(addr1_PTR))->w.hword2 >              \
          ((SGT_MAC_ADDR_TYP *)(addr2_PTR))->w.hword2 )   )    )

#define SGT_MAC_ADDR_IS_ARP(x)    ( (GT_U16)(x) == 0x0806 )

#define SGT_MAC_ADDR_IS_BPDU(addr_PTR)                           \
        (((GT_U8 *)(addr_PTR))[0]  == 0x01 &&                               \
         ((GT_U8 *)(addr_PTR))[1]  == 0x80 &&                               \
         ((GT_U8 *)(addr_PTR))[2]  == 0xc2 &&                               \
         ((GT_U8 *)(addr_PTR))[3]  == 0x00 &&                               \
         ((GT_U8 *)(addr_PTR))[4]  == 0x00 &&                               \
         ((GT_U8 *)(addr_PTR))[5]  == 0x00 )

/* check if the IP frame is of protocol IGMP */
/* from the tcp/ip illustrated,volume1 chapter 1.6 ,page 10 */
#define SGT_IP_ADDR_IS_IGMP(ipHeaderPtr, result) \
{ \
    union {GT_U32 word;  SGT_IP_HEADER_WORD_STC stc; } tmp_unt;   \
    tmp_unt.word = ((SGT_IP_HEADER_TOP_STC*)ipHeaderPtr)->word3;  \
    tmp_unt.word = SGT_LIB_SWAP_BYTES_AND_WORDS_MAC(tmp_unt.word);\
    *result =  (tmp_unt.stc.protocol == 0x02 );                   \
}

#define SGT_UDP_DEST_PORT(udp_head) \
    (((GT_U8 *)(udp_head))[2] << 8 | \
    (((GT_U8 *)(udp_head))[3]))

#define SGT_IP_BCST_ADDR_IS_RIPV1(ip_head) \
    ((((GT_U8 *)(ip_head))[9] == 0x11) && \
    (SGT_UDP_DEST_PORT((GT_U8 *)ip_head + ((ip_head[0] & 0x0f) * 4)) == 0x208))

#define SGT_MAC_ADDR_IS_MLD(addr_PTR)                           \
        (((GT_U8 *)(addr_PTR))[0]  == 0x33 &&                    \
         ((GT_U8 *)(addr_PTR))[1]  == 0x33)

#define SGT_MAC_ADDR_IS_IPMC(addr_PTR)                           \
        (((GT_U8 *)(addr_PTR))[0]  == 0x01 &&                    \
         ((GT_U8 *)(addr_PTR))[1]  == 0x00 &&                    \
         ((GT_U8 *)(addr_PTR))[2]  == 0x5E &&                    \
         ((((GT_U8 *)(addr_PTR))[3] & 0x80) == 0))

#define SGT_MAC_ADDR_IS_PAUSE(addr_PTR)                          \
        (((GT_U8 *)(addr_PTR))[0]  == 0x01 &&                    \
         ((GT_U8 *)(addr_PTR))[1]  == 0x80 &&                    \
         ((GT_U8 *)(addr_PTR))[2]  == 0xc2 &&                    \
         ((GT_U8 *)(addr_PTR))[3]  == 0x00 &&                    \
         ((GT_U8 *)(addr_PTR))[4]  == 0x00 &&                    \
         ((GT_U8 *)(addr_PTR))[5]  == 0x01)

#define SGT_MAC_ADDR_IS_MGMT(addr_PTR)                           \
        (((GT_U8 *)(addr_PTR))[0]  == 0x01 &&                    \
         ((GT_U8 *)(addr_PTR))[1]  == 0x80 &&                    \
         ((GT_U8 *)(addr_PTR))[2]  == 0xC2 &&                    \
         ((GT_U8 *)(addr_PTR))[3]  == 0x00 &&                    \
         ((GT_U8 *)(addr_PTR))[4]  == 0x00)

#define SGT_MAC_ADDR_IS_IPMV4(addr_PTR)                          \
        (((GT_U8 *)(addr_PTR))[0]  == 0x01 &&                    \
         ((GT_U8 *)(addr_PTR))[1]  == 0x00 &&                    \
         ((GT_U8 *)(addr_PTR))[2]  == 0x5e)

/* Minimum Ethernet frame that the device should send
   when frame is smaller then this value , a "0" padding should be added to frame */
#define SGT_MIN_FRAME_LEN   0x40

/* Maximal number of transmitted bytes by single port per second */
#define SGT_MAX_BYTES_PER_SECOND_CNS   (100 * 64)

/* Mac address length - 6 bytes */
#define SGT_MAC_ADDR_BYTES  6

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __sframeinfoh */

