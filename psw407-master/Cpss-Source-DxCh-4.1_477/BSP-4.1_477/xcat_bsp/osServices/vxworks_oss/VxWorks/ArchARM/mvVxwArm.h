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
#ifndef __INCmvLinuxPpch
#define __INCmvLinuxPpch

#include "vxWorks.h"
#include "mvCommon.h"

#define CPU_PHY_MEM(x)			    (MV_U32)x
#define CPU_MEMIO_CACHED_ADDR(x)    (void*)x
#define CPU_MEMIO_UNCACHED_ADDR(x)	(void*)x

/* CPU architecture dependent 32, 16, 8 bit read/write IO addresses */
#define MV_MEMIO32_WRITE(addr, data)	\
    ((*((volatile unsigned int*)(addr))) = ((unsigned int)(data)))

#define MV_MEMIO32_READ(addr)       	\
    ((*((volatile unsigned int*)(addr))))

#define MV_MEMIO16_WRITE(addr, data)	\
    ((*((volatile unsigned short*)(addr))) = ((unsigned short)(data)))

#define MV_MEMIO16_READ(addr)       	\
    ((*((volatile unsigned short*)(addr))))

#define MV_MEMIO8_WRITE(addr, data) 	\
    ((*((volatile unsigned char*)(addr))) = ((unsigned char)(data)))

#define MV_MEMIO8_READ(addr)        	\
    ((*((volatile unsigned char*)(addr))))


#define MV_MEMIO8_NFLASH_WRITE(addr, data) 	                                    \
    {  (*((volatile unsigned char*)(addr))) = ((unsigned char)(data));     }
    
#define MV_MEMIO16_NFLASH_WRITE(addr, data)	                                    \
    {  (*((volatile unsigned short*)(addr))) = ((unsigned short)(data));  }

/* No Fast Swap implementation (in assembler) for ARM */
#define MV_32BIT_LE_FAST(val)            MV_32BIT_LE(val)
#define MV_16BIT_LE_FAST(val)            MV_16BIT_LE(val)
#define MV_32BIT_BE_FAST(val)            MV_32BIT_BE(val)
#define MV_16BIT_BE_FAST(val)            MV_16BIT_BE(val)
    
/* 32 and 16 bit read/write in big/little endian mode */

/* 16bit write in little endian mode */
#define MV_MEMIO_LE16_WRITE(addr, data) \
        MV_MEMIO16_WRITE(addr, MV_16BIT_LE_FAST(data))

/* 16bit read in little endian mode */
static __inline MV_U16 MV_MEMIO_LE16_READ(MV_U32 addr)
{
	MV_U16 data;
	data = (MV_U16)MV_MEMIO16_READ(addr);
	data = (MV_U16)MV_16BIT_LE_FAST(data);
    return data;
}

/* 32bit write in little endian mode */
#define MV_MEMIO_LE32_WRITE(addr, data) \
        MV_MEMIO32_WRITE(addr, MV_32BIT_LE_FAST(data))

/* 32bit read in little endian mode */
static __inline MV_U32 MV_MEMIO_LE32_READ(MV_U32 addr)
{
	MV_U32 data;
	data = (MV_U32)MV_MEMIO32_READ(addr);
	data = (MV_U32)MV_32BIT_LE_FAST(data);
    return data;
}
#define MV_READCHAR(address)                                                   \
        ((*((volatile unsigned char *)(address))))

#define MV_READSHORT(address)                                                  \
        ((*((volatile unsigned short *)(address))))

#define MV_READWORD(address)                                                   \
        ((*((volatile unsigned int *)(address))))

#define	MV_REG_READ_BYTE(base, offset)	MV_READCHAR(base + offset)
#define	MV_REG_READ_WORD(base, offset)  mvSwapShort(MV_READSHORT(base + offset))
#define	MV_REG_READ_DWORD(base, offset)	mvSwapWord(MV_READWORD(base + offset))

/* Flash APIs */
#define MV_FL_8_READ            MV_MEMIO8_READ
#define MV_FL_16_READ           MV_MEMIO_LE16_READ
#define MV_FL_32_READ           MV_MEMIO_LE32_READ
#define MV_FL_8_DATA_READ       MV_MEMIO8_READ
#define MV_FL_16_DATA_READ      MV_MEMIO16_READ
#define MV_FL_32_DATA_READ      MV_MEMIO32_READ
#define MV_FL_8_WRITE           MV_MEMIO8_WRITE
#define MV_FL_16_WRITE          MV_MEMIO_LE16_WRITE
#define MV_FL_32_WRITE          MV_MEMIO_LE32_WRITE
#define MV_FL_8_DATA_WRITE      MV_MEMIO8_WRITE
#define MV_FL_16_DATA_WRITE     MV_MEMIO16_WRITE
#define MV_FL_32_DATA_WRITE     MV_MEMIO32_WRITE

/* CPU cache information */
#define CPU_I_CACHE_LINE_SIZE   32    /* 2do: replace 32 with linux core macro */
#define CPU_D_CACHE_LINE_SIZE   32    /* 2do: replace 32 with linux core macro */

#ifdef _DIAB_TOOL
/* use macros */
asm void  mvOsCacheLineFlushInvDiab(volatile MV_U32 virtAddr)
{
% reg virtAddr;

 mcr p15, 0, virtAddr, c7, c14, 1; 
 /* DCache clean and invalidate single line by MVA. */ 
 nop ;  nop ;  nop ;  nop ;

 mcr p15, 1, virtAddr, c15, c10, 1; /* Clean and invalidate L2 line by MVA. */ 
 nop ;  nop ;  nop ;  nop ;

 mcr p15, 0, virtAddr, c7, c10, 4;  /* Drain Write buffer. */
 nop ;  nop ;  nop ;  nop ;
}

asm void mvOsCacheLineInvDiab(volatile MV_U32 virtAddr)
{
% reg virtAddr;

 mcr p15, 0, virtAddr, c7, c6, 1;    /* DCache invalidate single line by MVA. */
 nop ;  nop ;  nop ;  nop ;
 mcr p15, 1, virtAddr, c15, c11, 1;  /* Invalidate L2 line by MVA. */
 nop ;  nop ;  nop ;  nop ;
}
#endif

static __inline void mvOsCacheLineFlushInv(void *handle, MV_U32 virtAddr)
{
#ifdef _DIAB_TOOL
  volatile MV_U32 virtAddrDiab = virtAddr;
  mvOsCacheLineFlushInvDiab(virtAddrDiab);
#else
    /* DCache clean and invalidate single line by MVA. */
    _WRS_ASM ("mcr p15, 0, %0, c7, c14, 1" : : "r" (virtAddr));
    /* Clean and invalidate L2 line by MVA. */
    _WRS_ASM ("mcr p15, 1, %0, c15, c10, 1" : : "r" (virtAddr));
    /* Drain Write buffer. */
    _WRS_ASM ("mcr p15, 0, %0, c7, c10, 4" : : "r" (virtAddr));
#endif
}

static __inline void mvOsCacheLineInv(void *handle, MV_U32 virtAddr)
{
#ifdef _DIAB_TOOL
  volatile MV_U32 virtAddrDiab = virtAddr;
  mvOsCacheLineInvDiab(virtAddrDiab);
#else
    /* DCache invalidate single line by MVA. */
    _WRS_ASM ("mcr p15, 0, %0, c7, c6, 1" : : "r" (virtAddr));
    /* Invalidate L2 line by MVA. */
    _WRS_ASM ("mcr p15, 1, %0, c15, c11, 1" : : "r" (virtAddr));
#endif
}

/* Flush CPU pipe */
#define CPU_PIPE_FLUSH

/* ARM architecture APIs */
MV_U32  mvOsCpuRevGet (MV_VOID);
MV_U32  mvOsCpuPartGet (MV_VOID);
MV_U32  mvOsCpuArchGet (MV_VOID);
MV_U32  mvOsCpuVarGet (MV_VOID);
MV_U32  mvOsCpuAsciiGet (MV_VOID);

#endif /* __INCmvLinuxPpch */


