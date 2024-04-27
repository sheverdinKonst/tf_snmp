/*******************************************************************************
   Copyright (C) Marvell International Ltd. and its affiliates

   This software file (the "File") is owned and distributed by Marvell
   International Ltd. and/or its affiliates ("Marvell") under the following
   alternative licensing terms.  Once you have made an election to distribute the
   File under one of the following license alternatives, please (i) delete this
   introductory statement regarding license alternatives, (ii) delete the two
   license alternatives that you have not elected to use and (iii) preserve the
   Marvell copyright notice above.

********************************************************************************
   Marvell Commercial License Option

   If you received this File from Marvell and you have entered into a commercial
   license agreement (a "Commercial License") with Marvell, the File is licensed
   to you under the terms of the applicable Commercial License.

********************************************************************************
   Marvell GPL License Option

   If you received this File from Marvell, you may opt to use, redistribute and/or
   modify this File in accordance with the terms and conditions of the General
   Public License Version 2, June 1991 (the "GPL License"), a copy of which is
   available along with the File in the license.txt file or by writing to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or
   on the worldwide web at http://www.gnu.org/licenses/gpl.txt.

   THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED
   WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY
   DISCLAIMED.  The GPL License provides additional details about this warranty
   disclaimer.
********************************************************************************
   Marvell BSD License Option

   If you received this File from Marvell, you may opt to use, redistribute and/or
   modify this File under the following licensing terms.
   Redistribution and use in source and binary forms, with or without modification,
   are permitted provided that the following conditions are met:

*   Redistributions of source code must retain the above copyright notice,
        this list of conditions and the following disclaimer.

*   Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.

*   Neither the name of Marvell nor the names of its contributors may be
        used to endorse or promote products derived from this software without
        specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
   ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
   ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
   ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/

#ifndef __INCmvCommonh
#define __INCmvCommonh

#include "mvTypes.h"

/* Swap tool */

/* 16bit nibble swap. For example 0x1234 -> 0x2143                          */
#define MV_NIBBLE_SWAP_16BIT(X)    (((X & 0xf) << 4) |     \
                                    ((X & 0xf0) >> 4) |    \
                                    ((X & 0xf00) << 4) |   \
                                    ((X & 0xf000) >> 4))

/* 32bit nibble swap. For example 0x12345678 -> 0x21436587                  */
#define MV_NIBBLE_SWAP_32BIT(X)    (((X & 0xf) << 4) |       \
                                    ((X & 0xf0) >> 4) |      \
                                    ((X & 0xf00) << 4) |     \
                                    ((X & 0xf000) >> 4) |    \
                                    ((X & 0xf0000) << 4) |   \
                                    ((X & 0xf00000) >> 4) |  \
                                    ((X & 0xf000000) << 4) | \
                                    ((X & 0xf0000000) >> 4))

/* 16bit byte swap. For example 0x1122 -> 0x2211                            */
#define MV_BYTE_SWAP_16BIT(X) ((((X) & 0xff) << 8) | (((X) & 0xff00) >> 8))

/* 32bit byte swap. For example 0x11223344 -> 0x44332211                    */
#define MV_BYTE_SWAP_32BIT(X) ((((X) & 0xff) << 24) |                       \
                               (((X) & 0xff00) << 8) |                      \
                               (((X) & 0xff0000) >> 8) |                    \
                               (((X) & 0xff000000) >> 24))

/* 64bit byte swap. For example 0x11223344.55667788 -> 0x88776655.44332211  */
#define MV_BYTE_SWAP_64BIT(X) ((l64) ((((X) & 0xffULL) << 56) |             \
                                      (((X) & 0xff00ULL) << 40) |           \
                                      (((X) & 0xff0000ULL) << 24) |         \
                                      (((X) & 0xff000000ULL) << 8) |        \
                                      (((X) & 0xff00000000ULL) >> 8) |      \
                                      (((X) & 0xff0000000000ULL) >> 24) |   \
                                      (((X) & 0xff000000000000ULL) >> 40) | \
                                      (((X) & 0xff00000000000000ULL) >> 56)))

/* Endianess macros.                                                        */
#if defined(MV_CPU_LE)
    #define MV_16BIT_LE(X)  (X)
    #define MV_32BIT_LE(X)  (X)
    #define MV_64BIT_LE(X)  (X)
    #define MV_16BIT_BE(X)  MV_BYTE_SWAP_16BIT (X)
    #define MV_32BIT_BE(X)  MV_BYTE_SWAP_32BIT (X)
    #define MV_64BIT_BE(X)  MV_BYTE_SWAP_64BIT (X)
#elif defined(MV_CPU_BE)
    #define MV_16BIT_LE(X)  MV_BYTE_SWAP_16BIT (X)
    #define MV_32BIT_LE(X)  MV_BYTE_SWAP_32BIT (X)
    #define MV_64BIT_LE(X)  MV_BYTE_SWAP_64BIT (X)
    #define MV_16BIT_BE(X)  (X)
    #define MV_32BIT_BE(X)  (X)
    #define MV_64BIT_BE(X)  (X)
#else
    #error "CPU endianess isn't defined!\n"
#endif

/* Bit field definitions */
#define NO_BIT      0x00000000
#define BIT0        0x00000001
#define BIT1        0x00000002
#define BIT2        0x00000004
#define BIT3        0x00000008
#define BIT4        0x00000010
#define BIT5        0x00000020
#define BIT6        0x00000040
#define BIT7        0x00000080
#define BIT8        0x00000100
#define BIT9        0x00000200
#define BIT10       0x00000400
#define BIT11       0x00000800
#define BIT12       0x00001000
#define BIT13       0x00002000
#define BIT14       0x00004000
#define BIT15       0x00008000
#define BIT16       0x00010000
#define BIT17       0x00020000
#define BIT18       0x00040000
#define BIT19       0x00080000
#define BIT20       0x00100000
#define BIT21       0x00200000
#define BIT22       0x00400000
#define BIT23       0x00800000
#define BIT24       0x01000000
#define BIT25       0x02000000
#define BIT26       0x04000000
#define BIT27       0x08000000
#define BIT28       0x10000000
#define BIT29       0x20000000
#define BIT30       0x40000000
#define BIT31       0x80000000

#define BIT_0           (0x1)
#define BIT_1           (0x2)
#define BIT_2           (0x4)
#define BIT_3           (0x8)
#define BIT_4           (0x10)
#define BIT_5           (0x20)
#define BIT_6           (0x40)
#define BIT_7           (0x80)
#define BIT_8           (0x100)
#define BIT_9           (0x200)
#define BIT_10          (0x400)
#define BIT_11          (0x800)
#define BIT_12          (0x1000)
#define BIT_13          (0x2000)
#define BIT_14          (0x4000)
#define BIT_15          (0x8000)
#define BIT_16          (0x10000)
#define BIT_17          (0x20000)
#define BIT_18          (0x40000)
#define BIT_19          (0x80000)
#define BIT_20          (0x100000)
#define BIT_21          (0x200000)
#define BIT_22          (0x400000)
#define BIT_23          (0x800000)
#define BIT_24          (0x1000000)
#define BIT_25          (0x2000000)
#define BIT_26          (0x4000000)
#define BIT_27          (0x8000000)
#define BIT_28          (0x10000000)
#define BIT_29          (0x20000000)
#define BIT_30          (0x40000000)
#define BIT_31          (0x80000000)

/* Handy sizes */
#ifndef MV_HANDY_SIZES
#define MV_HANDY_SIZES

#define _1K         0x00000400
#define _2K         0x00000800
#define _4K         0x00001000
#define _8K         0x00002000
#define _16K        0x00004000
#define _32K        0x00008000
#define _64K        0x00010000
#define _128K       0x00020000
#define _256K       0x00040000
#define _512K       0x00080000
#define _1M         0x00100000
#define _2M         0x00200000
#define _4M         0x00400000
#define _8M         0x00800000
#define _16M        0x01000000
#define _32M        0x02000000
#define _64M        0x04000000
#define _128M       0x08000000
#define _256M       0x10000000
#define _512M       0x20000000
#define _1G         0x40000000
#define _2G         0x80000000

#define _1KB        0x00000400
#define _2KB        0x00000800
#define _4KB        0x00001000
#define _8KB        0x00002000
#define _10KB       0x00002800
#define _16KB       0x00004000
#define _32KB       0x00008000
#define _64KB       0x00010000
#define _128KB      0x00020000
#define _256KB      0x00040000
#define _512KB      0x00080000
#define _1MB        0x00100000
#define _2MB        0x00200000
#define _4MB        0x00400000
#define _8MB        0x00800000
#define _16MB       0x01000000
#define _32MB       0x02000000
#define _64MB       0x04000000
#define _128MB      0x08000000
#define _256MB      0x10000000
#define _512MB      0x20000000
#define _1GB        0x40000000
#define _2GB        0x80000000

#endif /* MV_HANDY_SIZES */

/* Tclock and Sys clock define */
#define _100MHz     100000000
#define _125MHz     125000000
#define _133MHz     133333334
#define _150MHz     150000000
#define _160MHz     160000000
#define _166MHz     166666667
#define _175MHz     175000000
#define _178MHz     178000000
#define _183MHz     183333334
#define _187MHz     187000000
#define _192MHz     192000000
#define _194MHz     194000000
#define _200MHz     200000000
#define _233MHz     233333334
#define _250MHz     250000000
#define _266MHz     266666667
#define _300MHz     300000000

/* Network stuff */
#define MV_NET_1GBS             1000000000      /* bits per sec */
#define MV_NET_100MBS           100000000       /* bits per sec */
#define MV_NET_10MBS            10000000        /* bits per sec */

/* For better address window table readability */
#define EN          MV_TRUE
#define DIS         MV_FALSE
#define N_A         -1          /* Not applicable */

/* Cache configuration options for memory (DRAM, SRAM, ... ) */

/* Memory uncached, HW or SW cache coherency is not needed */
#define MV_UNCACHED             0
/* Memory cached, HW cache coherency supported in WriteThrough mode */
#define MV_CACHE_COHER_HW_WT    1
/* Memory cached, HW cache coherency supported in WriteBack mode */
#define MV_CACHE_COHER_HW_WB    2
/* Memory cached, No HW cache coherency, Cache coherency must be in SW */
#define MV_CACHE_COHER_SW       3

/* Macro for testing aligment. Positive if number is NOT aligned   */
#define MV_IS_NOT_ALIGN(number, align)      ((number) & ((align) - 1))

/* Macro for alignment up. For example, MV_ALIGN_UP(0x0330, 0x20) = 0x0340   */
#define MV_ALIGN_UP(number, align)                                          \
    (((number) & ((align) - 1)) ? (((number) + (align)) & ~((align) - 1)) : (number))

/* Macro for alignment down. For example, MV_ALIGN_UP(0x0330, 0x20) = 0x0320 */
#define MV_ALIGN_DOWN(number, align) ((number) & ~((align) - 1))

/* This macro returns absolute value                                        */
#define MV_ABS(number)  (((int)(number) < 0) ? -(int)(number) : (int)(number))

/* Bit fields manipulation macros                                           */

/* An integer word which its 'x' bit is set                                 */
#define MV_BIT_MASK(bitNum)         (1 << (bitNum))

/* Checks wheter bit 'x' in integer word is set                             */
#define MV_BIT_CHECK(word, bitNum)  ((word) & MV_BIT_MASK (bitNum))

/* Clear (reset) bit 'x' in integer word (RMW - Read-Modify-Write)          */
#define MV_BIT_CLEAR(word, bitNum)  ((word) &= ~(MV_BIT_MASK (bitNum)))

/* Set bit 'x' in integer word (RMW)                                        */
#define MV_BIT_SET(word, bitNum)    ((word) |= MV_BIT_MASK (bitNum))

/* Invert bit 'x' in integer word (RMW)                                     */
#define MV_BIT_INV(word, bitNum)    ((word) ^= MV_BIT_MASK (bitNum))

/* Get the min between 'a' or 'b'                                           */
#define MV_MIN(a, b)    (((a) < (b)) ? (a) : (b))

/* Get the max between 'a' or 'b'                                           */
#define MV_MAX(a, b)    (((a) < (b)) ? (b) : (a))

/* Temporary */
#ifdef _DIAB_TOOL
#ifndef MV_ASMLANGUAGE
__inline__ int  mvOsDivide(num, div)
{
  return num/div;
}
#endif
#else
#define mvOsDivide(num, div)        \
({                                  \
    int i=0, rem=(num);             \
                                    \
    while(rem >= (div))             \
    {                               \
        rem -= (div);               \
        i++;                        \
    }                               \
    (i);                            \
})
#endif

/* Temporary */
#ifdef _DIAB_TOOL
#ifndef MV_ASMLANGUAGE
__inline__ int mvOsReminder(num, div)
{
  return num%div;
}
#endif
#else
#define mvOsReminder(num, div)      \
({                                  \
    int rem = (num);                \
                                    \
    while(rem >= (div))             \
        rem -= (div);               \
    (rem);                          \
})
#endif

#define MV_IP_QUAD(ipAddr)    ((ipAddr >> 24) & 0xFF), ((ipAddr >> 16) & 0xFF), \
    ((ipAddr >> 8) & 0xFF), ((ipAddr >> 0) & 0xFF)

#define MV_IS_POWER_OF_2(num) ((num != 0) && ((num & (num - 1)) == 0))

#ifndef MV_ASMLANGUAGE
/* mvCommon API list */

MV_VOID     mvHexToBin(const char *pHexStr, MV_U8 *pBin, int size);
void        mvAsciiToHex(const char *asciiStr, char *hexStr);
void        mvBinToHex(const MV_U8 *bin, char *hexStr, int size);
void        mvBinToAscii(const MV_U8 *bin, char *asciiStr, int size);

MV_STATUS   mvMacStrToHex(const char *macStr, MV_U8 *macHex);
MV_STATUS   mvMacHexToStr(MV_U8 *macHex, char *macStr);
void        mvSizePrint (MV_U32);

MV_VOID     mvPrintGndPktInfo(MV_GND_PKT_INFO *pktInfoP);
MV_VOID     mvPrintGndBufInfo(MV_GND_BUF_INFO *bufInfoP);
MV_VOID     mvPrintExtDsa    (MV_EXT_DSA *extDsaP);
MV_VOID     mvBuffsToPktInfo (MV_U8            * buffs[],
                              MV_U32            buffsLen[],
                              MV_U32            buffNum,
                              MV_GND_PKT_INFO  * pktInfoP);
MV_VOID     mvEthInitBuffInfo(MV_GND_BUF_INFO  *bufInfoP,
                              MV_U8            *buffP,
                              MV_U32            buffLen);

MV_STATUS   mvWinOverlapTest(MV_ADDR_WIN *pAddrWin1, MV_ADDR_WIN *pAddrWin2);

MV_U32 mvLog2(MV_U32 num);

#define NAMESIZE 16

#endif /* MV_ASMLANGUAGE */

#endif  /* __INCmvCommonh */

