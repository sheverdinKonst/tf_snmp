/*******************************************************************************
*                Copyright 2004, MARVELL SEMICONDUCTOR, LTD.                   *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL.                      *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
*                                                                              *
* MARVELL COMPRISES MARVELL TECHNOLOGY GROUP LTD. (MTGL) AND ITS SUBSIDIARIES, *
* MARVELL INTERNATIONAL LTD. (MIL), MARVELL TECHNOLOGY, INC. (MTI), MARVELL    *
* SEMICONDUCTOR, INC. (MSI), MARVELL ASIA PTE LTD. (MAPL), MARVELL JAPAN K.K.  *
* (MJKK), MARVELL ISRAEL LTD. (MSIL).                                          *
*******************************************************************************/

#ifndef __INCmvOsVxwAsmh
#define __INCmvOsVxwAsmh

#define _ASMLANGUAGE

#include "VxWorks.h"
           
#include "asm.h"
#include "mvSysHwConfig.h"

#if defined (MV_PPC)
 /* BE/ LE swap for Asm */
#if defined(MV_CPU_LE)
#define LD32_LE(reg, addr)   lwz    reg, 0(addr)
#define ST32_LE(reg, addr)   stw    reg, 0(addr)
#elif defined(MV_CPU_BE)
#define LD32_LE(reg, addr)   lwbrx  reg, 0, addr
#define ST32_LE(reg, addr)   stwbrx reg, 0, addr
#endif
#define MV_GPR_LOAD(toReg, value32)     \
		LOADPTR(toReg, (value32))

#define MV_REG_READ_ASM(toReg, tmpReg, regOffs)		    \
	    LOADPTR (tmpReg, ((regOffs) + INTER_REGS_BASE)) \
	    LD32_LE (toReg, tmpReg)
	    sync

#define MV_REG_WRITE_ASM(fromReg, tmpReg, regOffs)      \
	    LOADPTR (tmpReg, ((regOffs) + INTER_REGS_BASE)) \
	    ST32_LE (fromReg, tmpReg)		                \
        sync

#elif defined (MV_ARM)
/* BE/ LE swap for Asm */
#if defined(MV_CPU_LE)

#define htoll(x)    x
#define HTOLL(sr,tr)
HTOLL: .macro sr,temp
       .endm

#elif defined(MV_CPU_BE)

#define htoll(x) ((((x) & 0x00ff) << 24) | \
                  (((x) & 0xff00) <<  8) | \
                  (((x) >> 8)  & 0xff00) | \
                  (((x) >> 24) & 0x00ff))
				  
#ifdef _DIAB_TOOL
HTOLL: .macro sr,temp                  /*sr   = A  ,B  ,C  ,D    */
          eor temp, sr, sr, ROR $16     /*temp = A^C,B^D,C^A,D^B  */
          bic temp, temp, $0xFF0000     /*temp = A^C,0  ,C^A,D^B  */
          mov sr, sr, ROR $8            /*sr   = D  ,A  ,B  ,C    */
          eor sr, sr, temp, LSR $8      /*sr   = D  ,C  ,B  ,A    */
       .endm

#else

#define N16 #16
#define N8 #8 
#define NFF0000 #0xFF0000
#define HTOLL(sr,temp)                  /*sr   = A  ,B  ,C  ,D    */\
        eor temp, sr, sr, ROR  N16 ;     /*temp = A^C,B^D,C^A,D^B  */\
        bic temp, temp,  NFF0000 ;     /*temp = A^C,0  ,C^A,D^B  */\
        mov sr, sr, ROR  N8 ;            /*sr   = D  ,A  ,B  ,C    */\
        eor sr, sr, temp, LSR N8        /*sr   = D  ,C  ,B  ,A    */
#endif

#endif


#define CFG_DFL_MV_REGS 0xD0000000        /* boot time MV_REGS */

MV_REG_READ_ASM: .macro toReg, tmpReg, regOffs
#ifdef _DIAB_TOOL
                        ldr     tmpReg, =(INTER_REGS_BASE + regOffs)
                        ldr     toReg, [tmpReg]
#else /*_DIAB_TOOL*/
                        ldr     \tmpReg, =(INTER_REGS_BASE + \regOffs)
                        ldr     \toReg, [\tmpReg]
						HTOLL(\toReg,\tmpReg)                          /* */
#endif /*_DIAB_TOOL*/
                 .endm

MV_REG_WRITE_ASM: .macro fromReg,tmpReg,regOffs
#ifdef _DIAB_TOOL
                         ldr     tmpReg, =(INTER_REGS_BASE + regOffs)
                         str     fromReg, [tmpReg]
#else /*_DIAB_TOOL*/
						HTOLL(\fromReg,\tmpReg)                        /* */ 
                         ldr     \tmpReg, =(INTER_REGS_BASE + \regOffs)
                         str     \fromReg, [\tmpReg]
#endif /*_DIAB_TOOL*/
                 .endm
                 
MV_DV_REG_READ_ASM: .macro toReg, tmpReg, regOffs
#ifdef _DIAB_TOOL
                        ldr     tmpReg, =(CFG_DFL_MV_REGS + regOffs)
                        ldr     toReg, [tmpReg]
#else /*_DIAB_TOOL*/
                        ldr     \tmpReg, =(CFG_DFL_MV_REGS + \regOffs)
                        ldr     \toReg, [\tmpReg]
                         HTOLL(\toReg,\tmpReg)
#endif /*_DIAB_TOOL*/
                 .endm

MV_DV_REG_WRITE_ASM: .macro fromReg,tmpReg,regOffs
#ifdef _DIAB_TOOL
                         ldr     tmpReg, =(CFG_DFL_MV_REGS + regOffs)
                         str     fromReg, [tmpReg]
#else /*_DIAB_TOOL*/
						 HTOLL(\fromReg,\tmpReg) 
                         ldr     \tmpReg, =(CFG_DFL_MV_REGS + \regOffs)
                         str     \fromReg, [\tmpReg]
#endif /*_DIAB_TOOL*/               
                 .endm
#else
 #error "CPU type not selected"
#endif
#endif /* __INCmvOsVxwAsmh */
