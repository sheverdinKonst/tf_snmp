/*
 *  FIPS-46-3 compliant Triple-DES implementation
 *
 *  Copyright (C) 2006-2010, Brainspark B.V.
 *
 *  This file is part of PolarSSL (http://www.polarssl.org)
 *  Lead Maintainer: Paul Bakker <polarssl_maintainer at polarssl.org>
 *
 *  All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 *  DES, on which TDES is based, was originally designed by Horst Feistel
 *  at IBM in 1974, and was adopted as a standard by NIST (formerly NBS).
 *
 *  http://csrc.nist.gov/publications/fips/fips46-3/fips46-3.pdf
 */

/* Note: This file has been modified by ST's MCD Application Team, to support 
   the hardware crypto engine embedded in STM32F417xx */

#include "../../config.h"

#if defined(POLARSSL_DES_C)
//#include "main.h"
#include "../include/polarssl/des.h"

#ifdef USE_STM32F4XX_HW_CRYPTO /* use HW Crypto */ 
CRYP_InitTypeDef DES3_CRYP_InitStructure;
CRYP_KeyInitTypeDef DES3_CRYP_KeyInitStructure;
CRYP_IVInitTypeDef DES3_CRYP_IVInitStructure;
#endif /* USE_STM32F4XX_HW_CRYPTO */

/*
 * 32-bit integer manipulation macros (big endian)
 */
#ifndef GET_ULONG_BE
#define GET_ULONG_BE(n,b,i)                             \
{                                                       \
    (n) = ( (unsigned long) (b)[(i)    ] << 24 )        \
        | ( (unsigned long) (b)[(i) + 1] << 16 )        \
        | ( (unsigned long) (b)[(i) + 2] <<  8 )        \
        | ( (unsigned long) (b)[(i) + 3]       );       \
}
#endif

#ifndef PUT_ULONG_BE
#define PUT_ULONG_BE(n,b,i)                             \
{                                                       \
    (b)[(i)    ] = (unsigned char) ( (n) >> 24 );       \
    (b)[(i) + 1] = (unsigned char) ( (n) >> 16 );       \
    (b)[(i) + 2] = (unsigned char) ( (n) >>  8 );       \
    (b)[(i) + 3] = (unsigned char) ( (n)       );       \
}
#endif

/*
 * Expanded DES S-boxes
 */
static const unsigned long SB1[64] =
{
    0x01010400, 0x00000000, 0x00010000, 0x01010404,
    0x01010004, 0x00010404, 0x00000004, 0x00010000,
    0x00000400, 0x01010400, 0x01010404, 0x00000400,
    0x01000404, 0x01010004, 0x01000000, 0x00000004,
    0x00000404, 0x01000400, 0x01000400, 0x00010400,
    0x00010400, 0x01010000, 0x01010000, 0x01000404,
    0x00010004, 0x01000004, 0x01000004, 0x00010004,
    0x00000000, 0x00000404, 0x00010404, 0x01000000,
    0x00010000, 0x01010404, 0x00000004, 0x01010000,
    0x01010400, 0x01000000, 0x01000000, 0x00000400,
    0x01010004, 0x00010000, 0x00010400, 0x01000004,
    0x00000400, 0x00000004, 0x01000404, 0x00010404,
    0x01010404, 0x00010004, 0x01010000, 0x01000404,
    0x01000004, 0x00000404, 0x00010404, 0x01010400,
    0x00000404, 0x01000400, 0x01000400, 0x00000000,
    0x00010004, 0x00010400, 0x00000000, 0x01010004
};

static const unsigned long SB2[64] =
{
    0x80108020, 0x80008000, 0x00008000, 0x00108020,
    0x00100000, 0x00000020, 0x80100020, 0x80008020,
    0x80000020, 0x80108020, 0x80108000, 0x80000000,
    0x80008000, 0x00100000, 0x00000020, 0x80100020,
    0x00108000, 0x00100020, 0x80008020, 0x00000000,
    0x80000000, 0x00008000, 0x00108020, 0x80100000,
    0x00100020, 0x80000020, 0x00000000, 0x00108000,
    0x00008020, 0x80108000, 0x80100000, 0x00008020,
    0x00000000, 0x00108020, 0x80100020, 0x00100000,
    0x80008020, 0x80100000, 0x80108000, 0x00008000,
    0x80100000, 0x80008000, 0x00000020, 0x80108020,
    0x00108020, 0x00000020, 0x00008000, 0x80000000,
    0x00008020, 0x80108000, 0x00100000, 0x80000020,
    0x00100020, 0x80008020, 0x80000020, 0x00100020,
    0x00108000, 0x00000000, 0x80008000, 0x00008020,
    0x80000000, 0x80100020, 0x80108020, 0x00108000
};

static const unsigned long SB3[64] =
{
    0x00000208, 0x08020200, 0x00000000, 0x08020008,
    0x08000200, 0x00000000, 0x00020208, 0x08000200,
    0x00020008, 0x08000008, 0x08000008, 0x00020000,
    0x08020208, 0x00020008, 0x08020000, 0x00000208,
    0x08000000, 0x00000008, 0x08020200, 0x00000200,
    0x00020200, 0x08020000, 0x08020008, 0x00020208,
    0x08000208, 0x00020200, 0x00020000, 0x08000208,
    0x00000008, 0x08020208, 0x00000200, 0x08000000,
    0x08020200, 0x08000000, 0x00020008, 0x00000208,
    0x00020000, 0x08020200, 0x08000200, 0x00000000,
    0x00000200, 0x00020008, 0x08020208, 0x08000200,
    0x08000008, 0x00000200, 0x00000000, 0x08020008,
    0x08000208, 0x00020000, 0x08000000, 0x08020208,
    0x00000008, 0x00020208, 0x00020200, 0x08000008,
    0x08020000, 0x08000208, 0x00000208, 0x08020000,
    0x00020208, 0x00000008, 0x08020008, 0x00020200
};

static const unsigned long SB4[64] =
{
    0x00802001, 0x00002081, 0x00002081, 0x00000080,
    0x00802080, 0x00800081, 0x00800001, 0x00002001,
    0x00000000, 0x00802000, 0x00802000, 0x00802081,
    0x00000081, 0x00000000, 0x00800080, 0x00800001,
    0x00000001, 0x00002000, 0x00800000, 0x00802001,
    0x00000080, 0x00800000, 0x00002001, 0x00002080,
    0x00800081, 0x00000001, 0x00002080, 0x00800080,
    0x00002000, 0x00802080, 0x00802081, 0x00000081,
    0x00800080, 0x00800001, 0x00802000, 0x00802081,
    0x00000081, 0x00000000, 0x00000000, 0x00802000,
    0x00002080, 0x00800080, 0x00800081, 0x00000001,
    0x00802001, 0x00002081, 0x00002081, 0x00000080,
    0x00802081, 0x00000081, 0x00000001, 0x00002000,
    0x00800001, 0x00002001, 0x00802080, 0x00800081,
    0x00002001, 0x00002080, 0x00800000, 0x00802001,
    0x00000080, 0x00800000, 0x00002000, 0x00802080
};

static const unsigned long SB5[64] =
{
    0x00000100, 0x02080100, 0x02080000, 0x42000100,
    0x00080000, 0x00000100, 0x40000000, 0x02080000,
    0x40080100, 0x00080000, 0x02000100, 0x40080100,
    0x42000100, 0x42080000, 0x00080100, 0x40000000,
    0x02000000, 0x40080000, 0x40080000, 0x00000000,
    0x40000100, 0x42080100, 0x42080100, 0x02000100,
    0x42080000, 0x40000100, 0x00000000, 0x42000000,
    0x02080100, 0x02000000, 0x42000000, 0x00080100,
    0x00080000, 0x42000100, 0x00000100, 0x02000000,
    0x40000000, 0x02080000, 0x42000100, 0x40080100,
    0x02000100, 0x40000000, 0x42080000, 0x02080100,
    0x40080100, 0x00000100, 0x02000000, 0x42080000,
    0x42080100, 0x00080100, 0x42000000, 0x42080100,
    0x02080000, 0x00000000, 0x40080000, 0x42000000,
    0x00080100, 0x02000100, 0x40000100, 0x00080000,
    0x00000000, 0x40080000, 0x02080100, 0x40000100
};

static const unsigned long SB6[64] =
{
    0x20000010, 0x20400000, 0x00004000, 0x20404010,
    0x20400000, 0x00000010, 0x20404010, 0x00400000,
    0x20004000, 0x00404010, 0x00400000, 0x20000010,
    0x00400010, 0x20004000, 0x20000000, 0x00004010,
    0x00000000, 0x00400010, 0x20004010, 0x00004000,
    0x00404000, 0x20004010, 0x00000010, 0x20400010,
    0x20400010, 0x00000000, 0x00404010, 0x20404000,
    0x00004010, 0x00404000, 0x20404000, 0x20000000,
    0x20004000, 0x00000010, 0x20400010, 0x00404000,
    0x20404010, 0x00400000, 0x00004010, 0x20000010,
    0x00400000, 0x20004000, 0x20000000, 0x00004010,
    0x20000010, 0x20404010, 0x00404000, 0x20400000,
    0x00404010, 0x20404000, 0x00000000, 0x20400010,
    0x00000010, 0x00004000, 0x20400000, 0x00404010,
    0x00004000, 0x00400010, 0x20004010, 0x00000000,
    0x20404000, 0x20000000, 0x00400010, 0x20004010
};

static const unsigned long SB7[64] =
{
    0x00200000, 0x04200002, 0x04000802, 0x00000000,
    0x00000800, 0x04000802, 0x00200802, 0x04200800,
    0x04200802, 0x00200000, 0x00000000, 0x04000002,
    0x00000002, 0x04000000, 0x04200002, 0x00000802,
    0x04000800, 0x00200802, 0x00200002, 0x04000800,
    0x04000002, 0x04200000, 0x04200800, 0x00200002,
    0x04200000, 0x00000800, 0x00000802, 0x04200802,
    0x00200800, 0x00000002, 0x04000000, 0x00200800,
    0x04000000, 0x00200800, 0x00200000, 0x04000802,
    0x04000802, 0x04200002, 0x04200002, 0x00000002,
    0x00200002, 0x04000000, 0x04000800, 0x00200000,
    0x04200800, 0x00000802, 0x00200802, 0x04200800,
    0x00000802, 0x04000002, 0x04200802, 0x04200000,
    0x00200800, 0x00000000, 0x00000002, 0x04200802,
    0x00000000, 0x00200802, 0x04200000, 0x00000800,
    0x04000002, 0x04000800, 0x00000800, 0x00200002
};

static const unsigned long SB8[64] =
{
    0x10001040, 0x00001000, 0x00040000, 0x10041040,
    0x10000000, 0x10001040, 0x00000040, 0x10000000,
    0x00040040, 0x10040000, 0x10041040, 0x00041000,
    0x10041000, 0x00041040, 0x00001000, 0x00000040,
    0x10040000, 0x10000040, 0x10001000, 0x00001040,
    0x00041000, 0x00040040, 0x10040040, 0x10041000,
    0x00001040, 0x00000000, 0x00000000, 0x10040040,
    0x10000040, 0x10001000, 0x00041040, 0x00040000,
    0x00041040, 0x00040000, 0x10041000, 0x00001000,
    0x00000040, 0x10040040, 0x00001000, 0x00041040,
    0x10001000, 0x00000040, 0x10000040, 0x10040000,
    0x10040040, 0x10000000, 0x00040000, 0x10001040,
    0x00000000, 0x10041040, 0x00040040, 0x10000040,
    0x10040000, 0x10001000, 0x10001040, 0x00000000,
    0x10041040, 0x00041000, 0x00041000, 0x00001040,
    0x00001040, 0x00040040, 0x10000000, 0x10041000
};

/*
 * PC1: left and right halves bit-swap
 */
static const unsigned long LHs[16] =
{
    0x00000000, 0x00000001, 0x00000100, 0x00000101,
    0x00010000, 0x00010001, 0x00010100, 0x00010101,
    0x01000000, 0x01000001, 0x01000100, 0x01000101,
    0x01010000, 0x01010001, 0x01010100, 0x01010101
};

static const unsigned long RHs[16] =
{
    0x00000000, 0x01000000, 0x00010000, 0x01010000,
    0x00000100, 0x01000100, 0x00010100, 0x01010100,
    0x00000001, 0x01000001, 0x00010001, 0x01010001,
    0x00000101, 0x01000101, 0x00010101, 0x01010101,
};

/*
 * Initial Permutation macro
 */
#define DES_IP(X,Y)                                             \
{                                                               \
    T = ((X >>  4) ^ Y) & 0x0F0F0F0F; Y ^= T; X ^= (T <<  4);   \
    T = ((X >> 16) ^ Y) & 0x0000FFFF; Y ^= T; X ^= (T << 16);   \
    T = ((Y >>  2) ^ X) & 0x33333333; X ^= T; Y ^= (T <<  2);   \
    T = ((Y >>  8) ^ X) & 0x00FF00FF; X ^= T; Y ^= (T <<  8);   \
    Y = ((Y << 1) | (Y >> 31)) & 0xFFFFFFFF;                    \
    T = (X ^ Y) & 0xAAAAAAAA; Y ^= T; X ^= T;                   \
    X = ((X << 1) | (X >> 31)) & 0xFFFFFFFF;                    \
}

/*
 * Final Permutation macro
 */
#define DES_FP(X,Y)                                             \
{                                                               \
    X = ((X << 31) | (X >> 1)) & 0xFFFFFFFF;                    \
    T = (X ^ Y) & 0xAAAAAAAA; X ^= T; Y ^= T;                   \
    Y = ((Y << 31) | (Y >> 1)) & 0xFFFFFFFF;                    \
    T = ((Y >>  8) ^ X) & 0x00FF00FF; X ^= T; Y ^= (T <<  8);   \
    T = ((Y >>  2) ^ X) & 0x33333333; X ^= T; Y ^= (T <<  2);   \
    T = ((X >> 16) ^ Y) & 0x0000FFFF; Y ^= T; X ^= (T << 16);   \
    T = ((X >>  4) ^ Y) & 0x0F0F0F0F; Y ^= T; X ^= (T <<  4);   \
}

/*
 * DES round macro
 */
#define DES_ROUND(X,Y)                          \
{                                               \
    T = *SK++ ^ X;                              \
    Y ^= SB8[ (T      ) & 0x3F ] ^              \
         SB6[ (T >>  8) & 0x3F ] ^              \
         SB4[ (T >> 16) & 0x3F ] ^              \
         SB2[ (T >> 24) & 0x3F ];               \
                                                \
    T = *SK++ ^ ((X << 28) | (X >> 4));         \
    Y ^= SB7[ (T      ) & 0x3F ] ^              \
         SB5[ (T >>  8) & 0x3F ] ^              \
         SB3[ (T >> 16) & 0x3F ] ^              \
         SB1[ (T >> 24) & 0x3F ];               \
}

#define SWAP(a,b) { unsigned long t = a; a = b; b = t; t = 0; }

static const unsigned char odd_parity_table[128] = { 1,  2,  4,  7,  8,
        11, 13, 14, 16, 19, 21, 22, 25, 26, 28, 31, 32, 35, 37, 38, 41, 42, 44,
        47, 49, 50, 52, 55, 56, 59, 61, 62, 64, 67, 69, 70, 73, 74, 76, 79, 81,
        82, 84, 87, 88, 91, 93, 94, 97, 98, 100, 103, 104, 107, 109, 110, 112,
        115, 117, 118, 121, 122, 124, 127, 128, 131, 133, 134, 137, 138, 140,
        143, 145, 146, 148, 151, 152, 155, 157, 158, 161, 162, 164, 167, 168,
        171, 173, 174, 176, 179, 181, 182, 185, 186, 188, 191, 193, 194, 196,
        199, 200, 203, 205, 206, 208, 211, 213, 214, 217, 218, 220, 223, 224,
        227, 229, 230, 233, 234, 236, 239, 241, 242, 244, 247, 248, 251, 253,
        254 };

void des_key_set_parity( unsigned char key[DES_KEY_SIZE] )
{
    int i;

    for( i = 0; i < DES_KEY_SIZE; i++ )
        key[i] = odd_parity_table[key[i] / 2];
}

/*
 * Check the given key's parity, returns 1 on failure, 0 on SUCCESS
 */
int des_key_check_key_parity( const unsigned char key[DES_KEY_SIZE] )
{
    int i;

    for( i = 0; i < DES_KEY_SIZE; i++ )
        if ( key[i] != odd_parity_table[key[i] / 2] )
            return( 1 );

    return( 0 );
}

/*
 * Table of weak and semi-weak keys
 *
 * Source: http://en.wikipedia.org/wiki/Weak_key
 *
 * Weak:
 * Alternating ones + zeros (0x0101010101010101)
 * Alternating 'F' + 'E' (0xFEFEFEFEFEFEFEFE)
 * '0xE0E0E0E0F1F1F1F1'
 * '0x1F1F1F1F0E0E0E0E'
 *
 * Semi-weak:
 * 0x011F011F010E010E and 0x1F011F010E010E01
 * 0x01E001E001F101F1 and 0xE001E001F101F101
 * 0x01FE01FE01FE01FE and 0xFE01FE01FE01FE01
 * 0x1FE01FE00EF10EF1 and 0xE01FE01FF10EF10E
 * 0x1FFE1FFE0EFE0EFE and 0xFE1FFE1FFE0EFE0E
 * 0xE0FEE0FEF1FEF1FE and 0xFEE0FEE0FEF1FEF1
 *
 */

#define WEAK_KEY_COUNT 16

static const unsigned char weak_key_table[WEAK_KEY_COUNT][DES_KEY_SIZE] =
{
    { 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 },
    { 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE },
    { 0x1F, 0x1F, 0x1F, 0x1F, 0x0E, 0x0E, 0x0E, 0x0E },
    { 0xE0, 0xE0, 0xE0, 0xE0, 0xF1, 0xF1, 0xF1, 0xF1 },

    { 0x01, 0x1F, 0x01, 0x1F, 0x01, 0x0E, 0x01, 0x0E },
    { 0x1F, 0x01, 0x1F, 0x01, 0x0E, 0x01, 0x0E, 0x01 },
    { 0x01, 0xE0, 0x01, 0xE0, 0x01, 0xF1, 0x01, 0xF1 },
    { 0xE0, 0x01, 0xE0, 0x01, 0xF1, 0x01, 0xF1, 0x01 },
    { 0x01, 0xFE, 0x01, 0xFE, 0x01, 0xFE, 0x01, 0xFE },
    { 0xFE, 0x01, 0xFE, 0x01, 0xFE, 0x01, 0xFE, 0x01 },
    { 0x1F, 0xE0, 0x1F, 0xE0, 0x0E, 0xF1, 0x0E, 0xF1 },
    { 0xE0, 0x1F, 0xE0, 0x1F, 0xF1, 0x0E, 0xF1, 0x0E },
    { 0x1F, 0xFE, 0x1F, 0xFE, 0x0E, 0xFE, 0x0E, 0xFE },
    { 0xFE, 0x1F, 0xFE, 0x1F, 0xFE, 0x0E, 0xFE, 0x0E },
    { 0xE0, 0xFE, 0xE0, 0xFE, 0xF1, 0xFE, 0xF1, 0xFE },
    { 0xFE, 0xE0, 0xFE, 0xE0, 0xFE, 0xF1, 0xFE, 0xF1 }
};

int des_key_check_weak( const unsigned char key[DES_KEY_SIZE] )
{
    int i;

    for( i = 0; i < WEAK_KEY_COUNT; i++ )
        if( memcmp( weak_key_table[i], key, DES_KEY_SIZE) == 0)
            return( 1 );

    return( 0 );
}

static void des_setkey( unsigned long SK[32], const unsigned char key[DES_KEY_SIZE] )
{
    int i;
    unsigned long X, Y, T;

    GET_ULONG_BE( X, key, 0 );
    GET_ULONG_BE( Y, key, 4 );

    /*
     * Permuted Choice 1
     */
    T =  ((Y >>  4) ^ X) & 0x0F0F0F0F;  X ^= T; Y ^= (T <<  4);
    T =  ((Y      ) ^ X) & 0x10101010;  X ^= T; Y ^= (T      );

    X =   (LHs[ (X      ) & 0xF] << 3) | (LHs[ (X >>  8) & 0xF ] << 2)
        | (LHs[ (X >> 16) & 0xF] << 1) | (LHs[ (X >> 24) & 0xF ]     )
        | (LHs[ (X >>  5) & 0xF] << 7) | (LHs[ (X >> 13) & 0xF ] << 6)
        | (LHs[ (X >> 21) & 0xF] << 5) | (LHs[ (X >> 29) & 0xF ] << 4);

    Y =   (RHs[ (Y >>  1) & 0xF] << 3) | (RHs[ (Y >>  9) & 0xF ] << 2)
        | (RHs[ (Y >> 17) & 0xF] << 1) | (RHs[ (Y >> 25) & 0xF ]     )
        | (RHs[ (Y >>  4) & 0xF] << 7) | (RHs[ (Y >> 12) & 0xF ] << 6)
        | (RHs[ (Y >> 20) & 0xF] << 5) | (RHs[ (Y >> 28) & 0xF ] << 4);

    X &= 0x0FFFFFFF;
    Y &= 0x0FFFFFFF;

    /*
     * calculate subkeys
     */
    for( i = 0; i < 16; i++ )
    {
        if( i < 2 || i == 8 || i == 15 )
        {
            X = ((X <<  1) | (X >> 27)) & 0x0FFFFFFF;
            Y = ((Y <<  1) | (Y >> 27)) & 0x0FFFFFFF;
        }
        else
        {
            X = ((X <<  2) | (X >> 26)) & 0x0FFFFFFF;
            Y = ((Y <<  2) | (Y >> 26)) & 0x0FFFFFFF;
        }

        *SK++ =   ((X <<  4) & 0x24000000) | ((X << 28) & 0x10000000)
                | ((X << 14) & 0x08000000) | ((X << 18) & 0x02080000)
                | ((X <<  6) & 0x01000000) | ((X <<  9) & 0x00200000)
                | ((X >>  1) & 0x00100000) | ((X << 10) & 0x00040000)
                | ((X <<  2) & 0x00020000) | ((X >> 10) & 0x00010000)
                | ((Y >> 13) & 0x00002000) | ((Y >>  4) & 0x00001000)
                | ((Y <<  6) & 0x00000800) | ((Y >>  1) & 0x00000400)
                | ((Y >> 14) & 0x00000200) | ((Y      ) & 0x00000100)
                | ((Y >>  5) & 0x00000020) | ((Y >> 10) & 0x00000010)
                | ((Y >>  3) & 0x00000008) | ((Y >> 18) & 0x00000004)
                | ((Y >> 26) & 0x00000002) | ((Y >> 24) & 0x00000001);

        *SK++ =   ((X << 15) & 0x20000000) | ((X << 17) & 0x10000000)
                | ((X << 10) & 0x08000000) | ((X << 22) & 0x04000000)
                | ((X >>  2) & 0x02000000) | ((X <<  1) & 0x01000000)
                | ((X << 16) & 0x00200000) | ((X << 11) & 0x00100000)
                | ((X <<  3) & 0x00080000) | ((X >>  6) & 0x00040000)
                | ((X << 15) & 0x00020000) | ((X >>  4) & 0x00010000)
                | ((Y >>  2) & 0x00002000) | ((Y <<  8) & 0x00001000)
                | ((Y >> 14) & 0x00000808) | ((Y >>  9) & 0x00000400)
                | ((Y      ) & 0x00000200) | ((Y <<  7) & 0x00000100)
                | ((Y >>  7) & 0x00000020) | ((Y >>  3) & 0x00000011)
                | ((Y <<  2) & 0x00000004) | ((Y >> 21) & 0x00000002);
    }
}

/*
 * DES key schedule (56-bit, encryption)
 */
int des_setkey_enc( des_context *ctx, const unsigned char key[DES_KEY_SIZE] )
{
    des_setkey( ctx->sk, key );

    return( 0 );
}

/*
 * DES key schedule (56-bit, decryption)
 */
int des_setkey_dec( des_context *ctx, const unsigned char key[DES_KEY_SIZE] )
{
    int i;

    des_setkey( ctx->sk, key );

    for( i = 0; i < 16; i += 2 )
    {
        SWAP( ctx->sk[i    ], ctx->sk[30 - i] );
        SWAP( ctx->sk[i + 1], ctx->sk[31 - i] );
    }

    return( 0 );
}

static void des3_set2key( unsigned long esk[96],
                          unsigned long dsk[96],
                          const unsigned char key[DES_KEY_SIZE*2] )
{
    int i;

    des_setkey( esk, key );
    des_setkey( dsk + 32, key + 8 );

    for( i = 0; i < 32; i += 2 )
    {
        dsk[i     ] = esk[30 - i];
        dsk[i +  1] = esk[31 - i];

        esk[i + 32] = dsk[62 - i];
        esk[i + 33] = dsk[63 - i];

        esk[i + 64] = esk[i    ];
        esk[i + 65] = esk[i + 1];

        dsk[i + 64] = dsk[i    ];
        dsk[i + 65] = dsk[i + 1];
    }
}

/*
 * Triple-DES key schedule (112-bit, encryption)
 */
int des3_set2key_enc( des3_context *ctx, const unsigned char key[DES_KEY_SIZE * 2] )
{
    unsigned long sk[96];

    des3_set2key( ctx->sk, sk, key );
    memset( sk,  0, sizeof( sk ) );

    return( 0 );
}

/*
 * Triple-DES key schedule (112-bit, decryption)
 */
int des3_set2key_dec( des3_context *ctx, const unsigned char key[DES_KEY_SIZE * 2] )
{
    unsigned long sk[96];

    des3_set2key( sk, ctx->sk, key );
    memset( sk,  0, sizeof( sk ) );

    return( 0 );
}

#ifndef USE_STM32F4XX_HW_CRYPTO /* use SW Crypto */
static void des3_set3key( unsigned long esk[96],
                          unsigned long dsk[96],
                          const unsigned char key[24] )
{
    int i;

    des_setkey( esk, key );
    des_setkey( dsk + 32, key +  8 );
    des_setkey( esk + 64, key + 16 );

    for( i = 0; i < 32; i += 2 )
    {
        dsk[i     ] = esk[94 - i];
        dsk[i +  1] = esk[95 - i];

        esk[i + 32] = dsk[62 - i];
        esk[i + 33] = dsk[63 - i];

        dsk[i + 64] = esk[30 - i];
        dsk[i + 65] = esk[31 - i];
    }
}
#endif /* USE_STM32F4XX_HW_CRYPTO */

/*
 * Triple-DES key schedule (168-bit, encryption)
 */
int des3_set3key_enc( des3_context *ctx, const unsigned char key[DES_KEY_SIZE * 3] )
{
#ifdef USE_STM32F4XX_HW_CRYPTO /* use HW Crypto */
  memcpy(ctx->tdes_enc_key, key, 24);

#else /* use SW Crypto */
    unsigned long sk[96];

    des3_set3key( ctx->sk, sk, key );
    memset( sk, 0, sizeof( sk ) );

#endif /* USE_STM32F4XX_HW_CRYPTO */
    return( 0 );
}

/*
 * Triple-DES key schedule (168-bit, decryption)
 */
int des3_set3key_dec( des3_context *ctx, const unsigned char key[DES_KEY_SIZE * 3] )
{
#ifdef USE_STM32F4XX_HW_CRYPTO /* use HW Crypto */
  memcpy(ctx->tdes_dec_key, key, 24);

#else /* use SW Crypto */
    unsigned long sk[96];

    des3_set3key( sk, ctx->sk, key );
    memset( sk, 0, sizeof( sk ) );

#endif /* USE_STM32F4XX_HW_CRYPTO */
    return( 0 );
}

/*
 * DES-ECB block encryption/decryption
 */
int des_crypt_ecb( des_context *ctx,
                    const unsigned char input[8],
                    unsigned char output[8] )
{
    int i;
    unsigned long X, Y, T, *SK;

    SK = ctx->sk;

    GET_ULONG_BE( X, input, 0 );
    GET_ULONG_BE( Y, input, 4 );

    DES_IP( X, Y );

    for( i = 0; i < 8; i++ )
    {
        DES_ROUND( Y, X );
        DES_ROUND( X, Y );
    }

    DES_FP( Y, X );

    PUT_ULONG_BE( Y, output, 0 );
    PUT_ULONG_BE( X, output, 4 );

    return( 0 );
}

/*
 * DES-CBC buffer encryption/decryption
 */
int des_crypt_cbc( des_context *ctx,
                    int mode,
                    size_t length,
                    unsigned char iv[8],
                    const unsigned char *input,
                    unsigned char *output )
{
    int i;
    unsigned char temp[8];

    if( length % 8 )
        return( POLARSSL_ERR_DES_INVALID_INPUT_LENGTH );

    if( mode == DES_ENCRYPT )
    {
        while( length > 0 )
        {
            for( i = 0; i < 8; i++ )
                output[i] = (unsigned char)( input[i] ^ iv[i] );

            des_crypt_ecb( ctx, output, output );
            memcpy( iv, output, 8 );

            input  += 8;
            output += 8;
            length -= 8;
        }
    }
    else /* DES_DECRYPT */
    {
        while( length > 0 )
        {
            memcpy( temp, input, 8 );
            des_crypt_ecb( ctx, input, output );

            for( i = 0; i < 8; i++ )
                output[i] = (unsigned char)( output[i] ^ iv[i] );

            memcpy( iv, temp, 8 );

            input  += 8;
            output += 8;
            length -= 8;
        }
    }

    return( 0 );
}

/*
 * 3DES-ECB block encryption/decryption
 */
int des3_crypt_ecb( des3_context *ctx,
                     const unsigned char input[8],
                     unsigned char output[8] )
{
#ifdef USE_STM32F4XX_HW_CRYPTO /* use HW Crypto */
  unsigned char *tdeskey; /* 3DES encryption/decryption key */

  if( ctx->mode == DES_ENCRYPT ) /* 3DES encryption */
  {
    /* Encryption key */
    tdeskey = ctx->tdes_enc_key;
  
    /* Deinitializes the CRYP peripheral */  
    CRYP_DeInit();
 
    /* Crypto structures initialisation*/
    CRYP_StructInit(&DES3_CRYP_InitStructure);  
    CRYP_KeyStructInit(&DES3_CRYP_KeyInitStructure);

    /* Crypto Init for Encryption process */      
    DES3_CRYP_InitStructure.CRYP_AlgoDir  = CRYP_AlgoDir_Encrypt;
    DES3_CRYP_InitStructure.CRYP_AlgoMode = CRYP_AlgoMode_TDES_ECB;
    DES3_CRYP_InitStructure.CRYP_DataType = CRYP_DataType_8b;
    CRYP_Init(&DES3_CRYP_InitStructure);

    /* Key Initialisation */
    DES3_CRYP_KeyInitStructure.CRYP_Key1Left = __REV(*(uint32_t*)&tdeskey[0]);
    DES3_CRYP_KeyInitStructure.CRYP_Key1Right= __REV(*(uint32_t*)&tdeskey[4]);
    DES3_CRYP_KeyInitStructure.CRYP_Key2Left = __REV(*(uint32_t*)&tdeskey[8]);
    DES3_CRYP_KeyInitStructure.CRYP_Key2Right= __REV(*(uint32_t*)&tdeskey[12]);
    DES3_CRYP_KeyInitStructure.CRYP_Key3Left = __REV(*(uint32_t*)&tdeskey[16]);
    DES3_CRYP_KeyInitStructure.CRYP_Key3Right= __REV(*(uint32_t*)&tdeskey[20]);
    CRYP_KeyInit(& DES3_CRYP_KeyInitStructure);

    /* Flush IN/OUT FIFO */
    CRYP_FIFOFlush();

    /* Write the input block in the input FIFO */
    CRYP_DataIn(*(uint32_t*)&input[0]);
    CRYP_DataIn(*(uint32_t*)&input[4]);

    /* Enable Crypto processor */
    CRYP_Cmd(ENABLE);

    /* Wait until the complete message has been processed */ 
    while(CRYP_GetFlagStatus(CRYP_FLAG_BUSY)!= RESET);
   
    /* Read the output block from the output FIFO */
    *(uint32_t*)&output[0] = CRYP_DataOut();
    *(uint32_t*)&output[4] = CRYP_DataOut();
  
    /* Disable Crypto */
    CRYP_Cmd(DISABLE);
  }
  else if( ctx->mode == DES_DECRYPT ) /* 3DES decryption */
  {
    /* Decryption key */
    tdeskey = ctx->tdes_dec_key;
  
    /* Deinitializes the CRYP peripheral */  
    CRYP_DeInit();
 
    /* Crypto structures initialisation*/
    CRYP_StructInit(&DES3_CRYP_InitStructure);  
    CRYP_KeyStructInit(&DES3_CRYP_KeyInitStructure);

    /* Crypto Init for Encryption process */      
    DES3_CRYP_InitStructure.CRYP_AlgoDir  = CRYP_AlgoDir_Decrypt;
    DES3_CRYP_InitStructure.CRYP_AlgoMode = CRYP_AlgoMode_TDES_ECB;
    DES3_CRYP_InitStructure.CRYP_DataType = CRYP_DataType_8b;
    CRYP_Init(&DES3_CRYP_InitStructure);

    /* Key Initialisation */
    DES3_CRYP_KeyInitStructure.CRYP_Key1Left = __REV(*(uint32_t*)&tdeskey[0]);
    DES3_CRYP_KeyInitStructure.CRYP_Key1Right= __REV(*(uint32_t*)&tdeskey[4]);
    DES3_CRYP_KeyInitStructure.CRYP_Key2Left = __REV(*(uint32_t*)&tdeskey[8]);
    DES3_CRYP_KeyInitStructure.CRYP_Key2Right= __REV(*(uint32_t*)&tdeskey[12]);
    DES3_CRYP_KeyInitStructure.CRYP_Key3Left = __REV(*(uint32_t*)&tdeskey[16]);
    DES3_CRYP_KeyInitStructure.CRYP_Key3Right= __REV(*(uint32_t*)&tdeskey[20]);
    CRYP_KeyInit(& DES3_CRYP_KeyInitStructure);

    /* Flush IN/OUT FIFO */
    CRYP_FIFOFlush();

    /* Write the input block in the input FIFO */
    CRYP_DataIn(*(uint32_t*)&input[0]);
    CRYP_DataIn(*(uint32_t*)&input[4]);

    /* Enable Crypto processor */
    CRYP_Cmd(ENABLE);

    /* Wait until the complete message has been processed */ 
    while(CRYP_GetFlagStatus(CRYP_FLAG_BUSY)!= RESET);
   
    /* Read the output block from the output FIFO */
    *(uint32_t*)&output[0] = CRYP_DataOut();
    *(uint32_t*)&output[4] = CRYP_DataOut();
  
    /* Disable Crypto */
    CRYP_Cmd(DISABLE);
  }

#else /* use SW Crypto */
    int i;
    unsigned long X, Y, T, *SK;

    SK = ctx->sk;

    GET_ULONG_BE( X, input, 0 );
    GET_ULONG_BE( Y, input, 4 );

    DES_IP( X, Y );

    for( i = 0; i < 8; i++ )
    {
        DES_ROUND( Y, X );
        DES_ROUND( X, Y );
    }

    for( i = 0; i < 8; i++ )
    {
        DES_ROUND( X, Y );
        DES_ROUND( Y, X );
    }

    for( i = 0; i < 8; i++ )
    {
        DES_ROUND( Y, X );
        DES_ROUND( X, Y );
    }

    DES_FP( Y, X );

    PUT_ULONG_BE( Y, output, 0 );
    PUT_ULONG_BE( X, output, 4 );

#endif /* USE_STM32F4XX_HW_CRYPTO */
    return( 0 );
}

/*
 * 3DES-CBC buffer encryption/decryption
 */
int des3_crypt_cbc( des3_context *ctx,
                     int mode,
                     size_t length,
                     unsigned char iv[8],
                     const unsigned char *input,
                     unsigned char *output )
{
#ifdef USE_STM32F4XX_HW_CRYPTO /* use HW Crypto */
  unsigned char *tdeskey; /* 3DES encryption/decryption key */

  if( mode == DES_ENCRYPT ) /* 3DES encryption */
  {
    ctx->mode = DES_ENCRYPT; 

    /* Encryption key */
    tdeskey = ctx->tdes_enc_key;
  
    /* Deinitializes the CRYP peripheral */  
    CRYP_DeInit();
 
    /* Crypto structures initialisation*/
    CRYP_StructInit(&DES3_CRYP_InitStructure);  
    CRYP_KeyStructInit(&DES3_CRYP_KeyInitStructure);
    CRYP_IVStructInit(&DES3_CRYP_IVInitStructure);

    /* Crypto Init for Encryption process */      
    DES3_CRYP_InitStructure.CRYP_AlgoDir  = CRYP_AlgoDir_Encrypt;
    DES3_CRYP_InitStructure.CRYP_AlgoMode = CRYP_AlgoMode_TDES_CBC;
    DES3_CRYP_InitStructure.CRYP_DataType = CRYP_DataType_8b;
    CRYP_Init(&DES3_CRYP_InitStructure);

    /* Key Initialisation */
    DES3_CRYP_KeyInitStructure.CRYP_Key1Left = __REV(*(uint32_t*)&tdeskey[0]);
    DES3_CRYP_KeyInitStructure.CRYP_Key1Right= __REV(*(uint32_t*)&tdeskey[4]);
    DES3_CRYP_KeyInitStructure.CRYP_Key2Left = __REV(*(uint32_t*)&tdeskey[8]);
    DES3_CRYP_KeyInitStructure.CRYP_Key2Right= __REV(*(uint32_t*)&tdeskey[12]);
    DES3_CRYP_KeyInitStructure.CRYP_Key3Left = __REV(*(uint32_t*)&tdeskey[16]);
    DES3_CRYP_KeyInitStructure.CRYP_Key3Right= __REV(*(uint32_t*)&tdeskey[20]);
    CRYP_KeyInit(& DES3_CRYP_KeyInitStructure);

    /* Initialization Vectors */
    DES3_CRYP_IVInitStructure.CRYP_IV0Left = __REV(*(uint32_t*)&iv[0]);
    DES3_CRYP_IVInitStructure.CRYP_IV0Right= __REV(*(uint32_t*)&iv[4]);
    CRYP_IVInit(&DES3_CRYP_IVInitStructure);

    while(length > 0)
    {
      /* Flush IN/OUT FIFO */
      CRYP_FIFOFlush();

      /* Write the input block in the input FIFO */
      CRYP_DataIn(*(uint32_t*)&input[0]);
      CRYP_DataIn(*(uint32_t*)&input[4]);

      /* Enable Crypto processor */
      CRYP_Cmd(ENABLE);

      /* Wait until the complete message has been processed */ 
      while(CRYP_GetFlagStatus(CRYP_FLAG_BUSY)!= RESET);
   
      /* Read the output block from the output FIFO */
      *(uint32_t*)&output[0] = CRYP_DataOut();
      *(uint32_t*)&output[4] = CRYP_DataOut();
      
      /* Update the initialization Vector */
      memcpy(iv, output, 8);
      
      input  += 8;
      output += 8;
      length -= 8; 
    }
    /* Disable Crypto */
    CRYP_Cmd(DISABLE);
  }
  else /* 3DES decryption */
  {
    ctx->mode = DES_DECRYPT; 
      
    /* Decryption key */
    tdeskey = ctx->tdes_dec_key;
  
    /* Deinitializes the CRYP peripheral */  
    CRYP_DeInit();
 
    /* Crypto structures initialisation*/
    CRYP_StructInit(&DES3_CRYP_InitStructure);  
    CRYP_KeyStructInit(&DES3_CRYP_KeyInitStructure);
    CRYP_IVStructInit(&DES3_CRYP_IVInitStructure);

    /* Crypto Init for Encryption process */      
    DES3_CRYP_InitStructure.CRYP_AlgoDir  = CRYP_AlgoDir_Decrypt;
    DES3_CRYP_InitStructure.CRYP_AlgoMode = CRYP_AlgoMode_TDES_CBC;
    DES3_CRYP_InitStructure.CRYP_DataType = CRYP_DataType_8b;
    CRYP_Init(&DES3_CRYP_InitStructure);

    /* Key Initialisation */
    DES3_CRYP_KeyInitStructure.CRYP_Key1Left = __REV(*(uint32_t*)&tdeskey[0]);
    DES3_CRYP_KeyInitStructure.CRYP_Key1Right= __REV(*(uint32_t*)&tdeskey[4]);
    DES3_CRYP_KeyInitStructure.CRYP_Key2Left = __REV(*(uint32_t*)&tdeskey[8]);
    DES3_CRYP_KeyInitStructure.CRYP_Key2Right= __REV(*(uint32_t*)&tdeskey[12]);
    DES3_CRYP_KeyInitStructure.CRYP_Key3Left = __REV(*(uint32_t*)&tdeskey[16]);
    DES3_CRYP_KeyInitStructure.CRYP_Key3Right= __REV(*(uint32_t*)&tdeskey[20]);
    CRYP_KeyInit(& DES3_CRYP_KeyInitStructure);

    /* Initialization Vectors */
    DES3_CRYP_IVInitStructure.CRYP_IV0Left = __REV(*(uint32_t*)&iv[0]);
    DES3_CRYP_IVInitStructure.CRYP_IV0Right= __REV(*(uint32_t*)&iv[4]);
    CRYP_IVInit(&DES3_CRYP_IVInitStructure);

    while(length > 0)
    {
      /* Update the initialization Vector */
      memcpy(iv, input, 8);
      
      /* Flush IN/OUT FIFO */
      CRYP_FIFOFlush();

      /* Write the input block in the input FIFO */
      CRYP_DataIn(*(uint32_t*)&input[0]);
      CRYP_DataIn(*(uint32_t*)&input[4]);

      /* Enable Crypto processor */
      CRYP_Cmd(ENABLE);

      /* Wait until the complete message has been processed */ 
      while(CRYP_GetFlagStatus(CRYP_FLAG_BUSY)!= RESET);
   
      /* Read the output block from the output FIFO */
      *(uint32_t*)&output[0] = CRYP_DataOut();
      *(uint32_t*)&output[4] = CRYP_DataOut();

      input  += 8;
      output += 8;
      length -= 8; 
    }
    /* Disable Crypto */
    CRYP_Cmd(DISABLE);
  }

#else /* use SW Crypto */
    int i;
    unsigned char temp[8];

    if( length % 8 )
        return( POLARSSL_ERR_DES_INVALID_INPUT_LENGTH );

    if( mode == DES_ENCRYPT )
    {
        ctx->mode = DES_ENCRYPT; /* Added by ST's MCD Application Team */    
        while( length > 0 )
        {
            for( i = 0; i < 8; i++ )
                output[i] = (unsigned char)( input[i] ^ iv[i] );

            des3_crypt_ecb( ctx, output, output );
            memcpy( iv, output, 8 );

            input  += 8;
            output += 8;
            length -= 8;
        }
    }
    else /* DES_DECRYPT */
    {
        ctx->mode = DES_DECRYPT; /* Added by ST's MCD Application Team */
        while( length > 0 )
        {
            memcpy( temp, input, 8 );
            des3_crypt_ecb( ctx, input, output );

            for( i = 0; i < 8; i++ )
                output[i] = (unsigned char)( output[i] ^ iv[i] );

            memcpy( iv, temp, 8 );

            input  += 8;
            output += 8;
            length -= 8;
        }
    }

#endif /* USE_STM32F4XX_HW_CRYPTO */
    return( 0 );
}

#if defined(POLARSSL_SELF_TEST)

#include <stdio.h>

/*
 * DES and 3DES test vectors from:
 *
 * http://csrc.nist.gov/groups/STM/cavp/documents/des/tripledes-vectors.zip
 */
static const unsigned char des3_test_keys[24] =
{
    0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
    0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, 0x01,
    0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, 0x01, 0x23
};

static const unsigned char des3_test_iv[8] =
{
    0x12, 0x34, 0x56, 0x78, 0x90, 0xAB, 0xCD, 0xEF,
};

static const unsigned char des3_test_buf[8] =
{
    0x4E, 0x6F, 0x77, 0x20, 0x69, 0x73, 0x20, 0x74
};

static const unsigned char des3_test_ecb_dec[3][8] =
{
    { 0xCD, 0xD6, 0x4F, 0x2F, 0x94, 0x27, 0xC1, 0x5D },
    { 0x69, 0x96, 0xC8, 0xFA, 0x47, 0xA2, 0xAB, 0xEB },
    { 0x83, 0x25, 0x39, 0x76, 0x44, 0x09, 0x1A, 0x0A }
};

static const unsigned char des3_test_ecb_enc[3][8] =
{
    { 0x6A, 0x2A, 0x19, 0xF4, 0x1E, 0xCA, 0x85, 0x4B },
    { 0x03, 0xE6, 0x9F, 0x5B, 0xFA, 0x58, 0xEB, 0x42 },
    { 0xDD, 0x17, 0xE8, 0xB8, 0xB4, 0x37, 0xD2, 0x32 }
};

static const unsigned char des3_test_cbc_dec[3][8] =
{
    { 0x12, 0x9F, 0x40, 0xB9, 0xD2, 0x00, 0x56, 0xB3 },
    { 0x47, 0x0E, 0xFC, 0x9A, 0x6B, 0x8E, 0xE3, 0x93 },
    { 0xC5, 0xCE, 0xCF, 0x63, 0xEC, 0xEC, 0x51, 0x4C }
};

static const unsigned char des3_test_cbc_enc[3][8] =
{
    { 0x54, 0xF1, 0x5A, 0xF6, 0xEB, 0xE3, 0xA4, 0xB4 },
    { 0x35, 0x76, 0x11, 0x56, 0x5F, 0xA1, 0x8E, 0x4D },
    { 0xCB, 0x19, 0x1F, 0x85, 0xD1, 0xED, 0x84, 0x39 }
};

/*
 * Checkup routine
 */
int des_self_test( int verbose )
{
    int i, j, u, v;
    des_context ctx;
    des3_context ctx3;
    unsigned char key[24];
    unsigned char buf[8];
    unsigned char prv[8];
    unsigned char iv[8];

    memset( key, 0, 24 );

    /*
     * ECB mode
     */
    for( i = 0; i < 6; i++ )
    {
        u = i >> 1;
        v = i  & 1;

        if( verbose != 0 )
            printf( "  DES%c-ECB-%3d (%s): ",
                    ( u == 0 ) ? ' ' : '3', 56 + u * 56,
                    ( v == DES_DECRYPT ) ? "dec" : "enc" );

        memcpy( buf, des3_test_buf, 8 );

        switch( i )
        {
        case 0:
            des_setkey_dec( &ctx, (unsigned char *) des3_test_keys );
            break;

        case 1:
            des_setkey_enc( &ctx, (unsigned char *) des3_test_keys );
            break;

        case 2:
            des3_set2key_dec( &ctx3, (unsigned char *) des3_test_keys );
            break;

        case 3:
            des3_set2key_enc( &ctx3, (unsigned char *) des3_test_keys );
            break;

        case 4:
            des3_set3key_dec( &ctx3, (unsigned char *) des3_test_keys );
            break;

        case 5:
            des3_set3key_enc( &ctx3, (unsigned char *) des3_test_keys );
            break;

        default:
            return( 1 );
        }

        for( j = 0; j < 10000; j++ )
        {
            if( u == 0 )
                des_crypt_ecb( &ctx, buf, buf );
            else
                des3_crypt_ecb( &ctx3, buf, buf );
        }

        if( ( v == DES_DECRYPT &&
                memcmp( buf, des3_test_ecb_dec[u], 8 ) != 0 ) ||
            ( v != DES_DECRYPT &&
                memcmp( buf, des3_test_ecb_enc[u], 8 ) != 0 ) )
        {
            if( verbose != 0 )
                printf( "failed\n" );

            return( 1 );
        }

        if( verbose != 0 )
            printf( "passed\n" );
    }

    if( verbose != 0 )
        printf( "\n" );

    /*
     * CBC mode
     */
    for( i = 0; i < 6; i++ )
    {
        u = i >> 1;
        v = i  & 1;

        if( verbose != 0 )
            printf( "  DES%c-CBC-%3d (%s): ",
                    ( u == 0 ) ? ' ' : '3', 56 + u * 56,
                    ( v == DES_DECRYPT ) ? "dec" : "enc" );

        memcpy( iv,  des3_test_iv,  8 );
        memcpy( prv, des3_test_iv,  8 );
        memcpy( buf, des3_test_buf, 8 );

        switch( i )
        {
        case 0:
            des_setkey_dec( &ctx, (unsigned char *) des3_test_keys );
            break;

        case 1:
            des_setkey_enc( &ctx, (unsigned char *) des3_test_keys );
            break;

        case 2:
            des3_set2key_dec( &ctx3, (unsigned char *) des3_test_keys );
            break;

        case 3:
            des3_set2key_enc( &ctx3, (unsigned char *) des3_test_keys );
            break;

        case 4:
            des3_set3key_dec( &ctx3, (unsigned char *) des3_test_keys );
            break;

        case 5:
            des3_set3key_enc( &ctx3, (unsigned char *) des3_test_keys );
            break;

        default:
            return( 1 );
        }

        if( v == DES_DECRYPT )
        {
            for( j = 0; j < 10000; j++ )
            {
                if( u == 0 )
                    des_crypt_cbc( &ctx, v, 8, iv, buf, buf );
                else
                    des3_crypt_cbc( &ctx3, v, 8, iv, buf, buf );
            }
        }
        else
        {
            for( j = 0; j < 10000; j++ )
            {
                unsigned char tmp[8];

                if( u == 0 )
                    des_crypt_cbc( &ctx, v, 8, iv, buf, buf );
                else
                    des3_crypt_cbc( &ctx3, v, 8, iv, buf, buf );

                memcpy( tmp, prv, 8 );
                memcpy( prv, buf, 8 );
                memcpy( buf, tmp, 8 );
            }

            memcpy( buf, prv, 8 );
        }

        if( ( v == DES_DECRYPT &&
                memcmp( buf, des3_test_cbc_dec[u], 8 ) != 0 ) ||
            ( v != DES_DECRYPT &&
                memcmp( buf, des3_test_cbc_enc[u], 8 ) != 0 ) )
        {
            if( verbose != 0 )
                printf( "failed\n" );

            return( 1 );
        }

        if( verbose != 0 )
            printf( "passed\n" );
    }

    if( verbose != 0 )
        printf( "\n" );

    return( 0 );
}

#endif

#endif
