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
#ifdef _DIAB_TOOL
volatile static void empty_func(void)
{
}
#else
unsigned int hweight16(unsigned int w)
{
	unsigned int res = w - ((w >> 1) & 0x5555);
	res = (res & 0x3333) + ((res >> 2) & 0x3333);
	res = (res + (res >> 4)) & 0x0F0F;
	return (res + (res >> 8)) & 0x00FF;
}

#include "VxWorks.h"
#include <stdlib.h>
#include "config.h"
#include "mvCtrlEnvRegs.h"
#include "mvSysHwConfig.h"
#include "mvTypes.h"
#include "mvNflash.h"

#include "arch/arm/regsArm.h"
#include "arch/arm/esfArm.h"
#include "arch/arm/excArmLib.h"
#include "intLib.h"

#include "mvOsVxw.h"

extern FUNCPTR _func_excBaseHook;
FUNCPTR G_prev_val_func_excBaseHook;

MV_U32 G_unalignExcHookCounter = 0;
MV_U32 G_unalignExc_DFSR = 0;
MV_U32 G_unalignExc_FAR = 0;

#ifdef UNALIGNED_ACCESS_PC_LOG_ENABLE
    int G_unalignedAccessPcRegLog[UNALIGNED_ACCESS_PC_LOG_LEN];
    int G_unalignedAccessPcRegLog_idx = 0;
#endif

/*
 * PSR bits
 */
#define USR26_MODE      0x00000000
#define FIQ26_MODE      0x00000001
#define IRQ26_MODE      0x00000002
#define SVC26_MODE      0x00000003
#define USR_MODE        0x00000010
#define FIQ_MODE        0x00000011
#define IRQ_MODE        0x00000012
#define SVC_MODE        0x00000013
#define ABT_MODE        0x00000017
#define UND_MODE        0x0000001b
#define SYSTEM_MODE     0x0000001f
#define MODE32_BIT      0x00000010
#define MODE_MASK       0x0000001f
#define PSR_T_BIT       0x00000020
#define PSR_F_BIT       0x00000040
#define PSR_I_BIT       0x00000080
#define PSR_A_BIT       0x00000100
#define PSR_J_BIT       0x01000000
#define PSR_Q_BIT       0x08000000
#define PSR_V_BIT       0x10000000
#define PSR_C_BIT       0x20000000
#define PSR_Z_BIT       0x40000000
#define PSR_N_BIT       0x80000000

#define user_mode(regs) \
        (((regs)->cpsr & 0xf) == 0)

#define thumb_mode(regs) \
        (((regs)->cpsr & PSR_T_BIT))

int do_alignment(unsigned long addr, ESF *pEsf, REG_SET *regs, EXC_INFO *pExcInfo);

/*
 * Interrupts are enabled at this point. See excEnterCommon in excALib.s file.
 */
STATUS unalignExcHook(int vec, ESF *pEsf, REG_SET *pRegs, EXC_INFO *pExcInfo)
{
    MV_U32 int_save;

    G_unalignExcHookCounter++; /* for debug */

    /*
     * If the exception is not unaligned access,
     * continue regular interrupt handling.
     */
    int_save = intLock();
    /* Data Fault Status Register */
    __asm volatile ("mrc p15, 0, %0, c5, c0, 0" : "=r" (G_unalignExc_DFSR));

    if ((G_unalignExc_DFSR & 0xF) != 0x1) /* if it's not alignment fault, exit*/
    {
        intUnlock(int_save);
        return OK; /* enables to continue regular interrupt handling */
    }
    intUnlock(int_save);

    /* Fault Address Register */
    __asm volatile ("mrc p15, 0, %0, c6, c0, 0" : "=r" (G_unalignExc_FAR));

    do_alignment(G_unalignExc_FAR, pEsf, pRegs, pExcInfo);

    /*
     * return to the next-after-faulted instruction
     */
     pEsf->pc += 1;

    /*
     * call previously registered hook if needed
     */
    /*
     * if (G_prev_val_func_excBaseHook)
     *     return G_prev_val_func_excBaseHook(vec, pEsf, pRegs, pExcInfo);
     * else
     *     return OK;
     */

    return 1; /* non-null val should be returned; see excExcHandle() */

    /*
     * return other than OK, to continue regular exception handling
     */
    /* return ERROR; */
}

void enableAlignmentFaultChecking_CP15_regR1_silent();

STATUS registerUnalignExcHook()
{
    MV_U32 int_save;

    int_save = intLock();
    if (_func_excBaseHook)
        G_prev_val_func_excBaseHook = _func_excBaseHook;

    _func_excBaseHook = (FUNCPTR)unalignExcHook;
    intUnlock(int_save);

    enableAlignmentFaultChecking_CP15_regR1_silent();

    return OK;
}

#define CODING_BITS(i)	(i & 0x0e000000)

#define LDST_I_BIT(i)	(i & (1 << 26))		/* Immediate constant	*/
#define LDST_P_BIT(i)	(i & (1 << 24))		/* Preindex		*/
#define LDST_U_BIT(i)	(i & (1 << 23))		/* Add offset		*/
#define LDST_W_BIT(i)	(i & (1 << 21))		/* Writeback		*/
#define LDST_L_BIT(i)	(i & (1 << 20))		/* Load			*/

#define LDST_P_EQ_U(i)	((((i) ^ ((i) >> 1)) & (1 << 23)) == 0)

#define LDSTHD_I_BIT(i)	(i & (1 << 22))		/* double/half-word immed */
#define LDM_S_BIT(i)	(i & (1 << 22))		/* write CPSR from SPSR	*/

#define RN_BITS(i)	((i >> 16) & 15)	/* Rn			*/
#define RD_BITS(i)	((i >> 12) & 15)	/* Rd			*/
#define RM_BITS(i)	(i & 15)		/* Rm			*/

#define REGMASK_BITS(i)	(i & 0xffff)
#define OFFSET_BITS(i)	(i & 0x0fff)

#define IS_SHIFT(i)	(i & 0x0ff0)
#define SHIFT_BITS(i)	((i >> 7) & 0x1f)
#define SHIFT_TYPE(i)	(i & 0x60)
#define SHIFT_LSL	0x00
#define SHIFT_LSR	0x20
#define SHIFT_ASR	0x40
#define SHIFT_RORRRX	0x60

unsigned long ai_user;
unsigned long ai_sys;
unsigned long ai_skipped;
unsigned long ai_half;
unsigned long ai_word;
unsigned long ai_dword;
unsigned long ai_multi;
int ai_usermode;

#ifdef CONFIG_PROC_FS
static const char *usermode_action[] = {
	"ignored",
	"warn",
	"fixup",
	"fixup+warn",
	"signal",
	"signal+warn"
};

#endif /* CONFIG_PROC_FS */

union offset_union {
	unsigned long un;
	  signed long sn;
};

#define TYPE_ERROR	0
#define TYPE_FAULT	1
#define TYPE_LDST	2
#define TYPE_DONE	3

#ifdef __ARMEB__
#define BE		1
#define FIRST_BYTE_16	"mov	%1, %1, ror #8\n"
#define FIRST_BYTE_32	"mov	%1, %1, ror #24\n"
#define NEXT_BYTE	"ror #24"
#else
#define BE		0
#define FIRST_BYTE_16
#define FIRST_BYTE_32
#define NEXT_BYTE	"lsr #8"
#endif

#define __get8_unaligned_check(ins,val,addr,err)	\
	__asm__(					\
	"1:	"ins"	%1, [%2], #1\n"			\
	"2:\n"						\
	"	.section .fixup,\"ax\"\n"		\
	"	.align	2\n"				\
	"3:	mov	%0, #1\n"			\
	"	b	2b\n"				\
	"	.previous\n"				\
	"	.section __ex_table,\"a\"\n"		\
	"	.align	3\n"				\
	"	.long	1b, 3b\n"			\
	"	.previous\n"				\
	: "=r" (err), "=&r" (val), "=r" (addr)		\
	: "0" (err), "2" (addr))

#define __get16_unaligned_check(ins,val,addr)			\
	do {							\
		unsigned int err = 0, v, a = addr;		\
		__get8_unaligned_check(ins,v,a,err);		\
		val =  v << ((BE) ? 8 : 0);			\
		__get8_unaligned_check(ins,v,a,err);		\
		val |= v << ((BE) ? 0 : 8);			\
		if (err)					\
			goto fault;				\
	} while (0)

#define get16_unaligned_check(val,addr) \
	__get16_unaligned_check("ldrb",val,addr)

#define get16t_unaligned_check(val,addr) \
	__get16_unaligned_check("ldrbt",val,addr)

#define __get32_unaligned_check(ins,val,addr)			\
	do {							\
		unsigned int err = 0, v, a = addr;		\
		__get8_unaligned_check(ins,v,a,err);		\
		val =  v << ((BE) ? 24 :  0);			\
		__get8_unaligned_check(ins,v,a,err);		\
		val |= v << ((BE) ? 16 :  8);			\
		__get8_unaligned_check(ins,v,a,err);		\
		val |= v << ((BE) ?  8 : 16);			\
		__get8_unaligned_check(ins,v,a,err);		\
		val |= v << ((BE) ?  0 : 24);			\
		if (err)					\
			goto fault;				\
	} while (0)

#define get32_unaligned_check(val,addr) \
	__get32_unaligned_check("ldrb",val,addr)

#define get32t_unaligned_check(val,addr) \
	__get32_unaligned_check("ldrbt",val,addr)

#define __put16_unaligned_check(ins,val,addr)			\
	do {							\
		unsigned int err = 0, v = val, a = addr;	\
		__asm__( FIRST_BYTE_16				\
		"1:	"ins"	%1, [%2], #1\n"			\
		"	mov	%1, %1, "NEXT_BYTE"\n"		\
		"2:	"ins"	%1, [%2]\n"			\
		"3:\n"						\
		"	.section .fixup,\"ax\"\n"		\
		"	.align	2\n"				\
		"4:	mov	%0, #1\n"			\
		"	b	3b\n"				\
		"	.previous\n"				\
		"	.section __ex_table,\"a\"\n"		\
		"	.align	3\n"				\
		"	.long	1b, 4b\n"			\
		"	.long	2b, 4b\n"			\
		"	.previous\n"				\
		: "=r" (err), "=&r" (v), "=&r" (a)		\
		: "0" (err), "1" (v), "2" (a));			\
		if (err)					\
			goto fault;				\
	} while (0)

#define put16_unaligned_check(val,addr)  \
	__put16_unaligned_check("strb",val,addr)

#define put16t_unaligned_check(val,addr) \
	__put16_unaligned_check("strbt",val,addr)

#define __put32_unaligned_check(ins,val,addr)			\
	do {							\
		unsigned int err = 0, v = val, a = addr;	\
		__asm__( FIRST_BYTE_32				\
		"1:	"ins"	%1, [%2], #1\n"			\
		"	mov	%1, %1, "NEXT_BYTE"\n"		\
		"2:	"ins"	%1, [%2], #1\n"			\
		"	mov	%1, %1, "NEXT_BYTE"\n"		\
		"3:	"ins"	%1, [%2], #1\n"			\
		"	mov	%1, %1, "NEXT_BYTE"\n"		\
		"4:	"ins"	%1, [%2]\n"			\
		"5:\n"						\
		"	.section .fixup,\"ax\"\n"		\
		"	.align	2\n"				\
		"6:	mov	%0, #1\n"			\
		"	b	5b\n"				\
		"	.previous\n"				\
		"	.section __ex_table,\"a\"\n"		\
		"	.align	3\n"				\
		"	.long	1b, 6b\n"			\
		"	.long	2b, 6b\n"			\
		"	.long	3b, 6b\n"			\
		"	.long	4b, 6b\n"			\
		"	.previous\n"				\
		: "=r" (err), "=&r" (v), "=&r" (a)		\
		: "0" (err), "1" (v), "2" (a));			\
		if (err)					\
			goto fault;				\
	} while (0)

#define put32_unaligned_check(val,addr) \
	__put32_unaligned_check("strb", val, addr)

#define put32t_unaligned_check(val,addr) \
	__put32_unaligned_check("strbt", val, addr)

/* static */ void
do_alignment_finish_ldst(unsigned long addr, unsigned long instr, REG_SET *regs, union offset_union offset)
{
	if (!LDST_U_BIT(instr))
		offset.un = -offset.un;

	if (!LDST_P_BIT(instr))
		addr += offset.un;

	if (!LDST_P_BIT(instr) || LDST_W_BIT(instr))
		regs->r[RN_BITS(instr)] = addr;
}

/* static */ int
do_alignment_ldrhstrh(unsigned long addr, unsigned long instr, REG_SET *regs)
{
	unsigned int rd = RD_BITS(instr);

	ai_half += 1;

	if (user_mode(regs))
		goto user;

	if (LDST_L_BIT(instr)) {
		unsigned long val;
		get16_unaligned_check(val, addr);

		/* signed half-word? */
		if (instr & 0x40)
			val = (signed long)((signed short) val);

		regs->r[rd] = val;
	} else
		put16_unaligned_check(regs->r[rd], addr);

	return TYPE_LDST;

 user:
	if (LDST_L_BIT(instr)) {
		unsigned long val;
		get16t_unaligned_check(val, addr);

		/* signed half-word? */
		if (instr & 0x40)
			val = (signed long)((signed short) val);

		regs->r[rd] = val;
	} else
		put16t_unaligned_check(regs->r[rd], addr);

	return TYPE_LDST;

 fault:
	return TYPE_FAULT;
}

/* static */ int
do_alignment_ldrdstrd(unsigned long addr, unsigned long instr, REG_SET *regs)
{
	unsigned int rd = RD_BITS(instr);

	if (((rd & 1) == 1) || (rd == 14))
		goto bad;

	ai_dword += 1;

	if (user_mode(regs))
		goto user;

	if ((instr & 0xf0) == 0xd0) { /* LDRD */
		unsigned long val;
		get32_unaligned_check(val, addr);
		regs->r[rd] = val;
		get32_unaligned_check(val, addr + 4);
		regs->r[rd + 1] = val;
	} else { /* STRD */
		put32_unaligned_check(regs->r[rd], addr);
		put32_unaligned_check(regs->r[rd + 1], addr + 4);
	}

	return TYPE_LDST;

 user:
	if ((instr & 0xf0) == 0xd0) {
		unsigned long val;
		get32t_unaligned_check(val, addr);
		regs->r[rd] = val;
		get32t_unaligned_check(val, addr + 4);
		regs->r[rd + 1] = val;
	} else {
		put32t_unaligned_check(regs->r[rd], addr);
		put32t_unaligned_check(regs->r[rd + 1], addr + 4);
	}

	return TYPE_LDST;
 bad:
	return TYPE_ERROR;
 fault:
	return TYPE_FAULT;
}

int
do_alignment_ldrstr(unsigned long addr, unsigned long instr, REG_SET *regs)
{
	unsigned int rd = RD_BITS(instr);

	ai_word += 1;

	if ((!LDST_P_BIT(instr) && LDST_W_BIT(instr)) || user_mode(regs))
		goto trans;

	if (LDST_L_BIT(instr)) {
		unsigned int val;
		get32_unaligned_check(val, addr);
		regs->r[rd] = val;
	} else
		put32_unaligned_check(regs->r[rd], addr);
	return TYPE_LDST;

 trans:
	if (LDST_L_BIT(instr)) {
		unsigned int val;
		get32t_unaligned_check(val, addr);
		regs->r[rd] = val;
	} else
		put32t_unaligned_check(regs->r[rd], addr);
	return TYPE_LDST;

 fault:
	return TYPE_FAULT;
}

/* static */ int
do_alignment_ldmstm(unsigned long addr, unsigned long instr, REG_SET *regs)
{
	unsigned int rd, rn, correction, nr_regs, regbits;
	unsigned long eaddr, newaddr;

	if (LDM_S_BIT(instr))
		goto bad;

	correction = 4; /* processor implementation defined */
	regs->pc += correction;

	ai_multi += 1;

	/* count the number of registers in the mask to be transferred */
	nr_regs = hweight16(REGMASK_BITS(instr)) * 4;

	rn = RN_BITS(instr);
	newaddr = eaddr = regs->r[rn];

	if (!LDST_U_BIT(instr))
		nr_regs = -nr_regs;
	newaddr += nr_regs;
	if (!LDST_U_BIT(instr))
		eaddr = newaddr;

	if (LDST_P_EQ_U(instr))	/* U = P */
		eaddr += 4;

	/*
	 * For alignment faults on the ARM922T/ARM920T the MMU  makes
	 * the FSR (and hence addr) equal to the updated base address
	 * of the multiple access rather than the restored value.
	 * Switch this message off if we've got a ARM92[02], otherwise
	 * [ls]dm alignment faults are noisy!
	 */
#if !(defined CONFIG_CPU_ARM922T)  && !(defined CONFIG_CPU_ARM920T)
	/*
	 * This is a "hint" - we already have eaddr worked out by the
	 * processor for us.
	 */
	if (addr != eaddr) {
		mvOsPrintf("LDMSTM: PC = %08lx, instr = %08lx, "
			"addr = %08lx, eaddr = %08lx\n",
			 regs->pc, instr, addr, eaddr);
		/* show_regs(regs); */
	}
#endif

	if (user_mode(regs)) {
		for (regbits = REGMASK_BITS(instr), rd = 0; regbits;
		     regbits >>= 1, rd += 1)
			if (regbits & 1) {
				if (LDST_L_BIT(instr)) {
					unsigned int val;
					get32t_unaligned_check(val, eaddr);
					regs->r[rd] = val;
				} else
					put32t_unaligned_check(regs->r[rd], eaddr);
				eaddr += 4;
			}
	} else {
		for (regbits = REGMASK_BITS(instr), rd = 0; regbits;
		     regbits >>= 1, rd += 1)
			if (regbits & 1) {
				if (LDST_L_BIT(instr)) {
					unsigned int val;
					get32_unaligned_check(val, eaddr);
					regs->r[rd] = val;
				} else
					put32_unaligned_check(regs->r[rd], eaddr);
				eaddr += 4;
			}
	}

	if (LDST_W_BIT(instr))
		regs->r[rn] = newaddr;
	if (!LDST_L_BIT(instr) || !(REGMASK_BITS(instr) & (1 << 15)))
		regs->pc -= correction;
	return TYPE_DONE;

fault:
	regs->pc -= correction;
	return TYPE_FAULT;

bad:
	mvOsPrintf("Alignment trap: not handling ldm with s-bit set\n");
	return TYPE_ERROR;
}

int do_alignment(unsigned long addr,
                 ESF *pEsf,
                 REG_SET *regs,
                 EXC_INFO *pExcInfo)
{
	union offset_union offset;
	unsigned long instr = 0, instrptr;
    int (*handler)(unsigned long addr, unsigned long instr, REG_SET *regs);
	unsigned int type;

    instrptr = (unsigned int)pEsf->pc;
    instr = *(unsigned int *)instrptr;

#ifdef UNALIGNED_ACCESS_PC_LOG_ENABLE
    G_unalignedAccessPcRegLog[G_unalignedAccessPcRegLog_idx] = (int)pEsf->pc;
    G_unalignedAccessPcRegLog_idx = ((G_unalignedAccessPcRegLog_idx + 1) &
                                     (UNALIGNED_ACCESS_PC_LOG_LEN - 1));
#endif

	switch (CODING_BITS(instr)) {

	case 0x00000000:	/* 3.13.4 load/store instruction extensions */
		if (LDSTHD_I_BIT(instr))
			offset.un = (instr & 0xf00) >> 4 | (instr & 15);
		else
			offset.un = regs->r[RM_BITS(instr)];

        /* STRSH does not exists */
		if ((instr & 0x000000f0) == 0x000000b0 || /* LDRH, STRH */
		    (instr & 0x001000f0) == 0x001000f0)   /* LDRSH */
			handler = do_alignment_ldrhstrh;
		else if ((instr & 0x001000f0) == 0x000000d0 || /* LDRD */
			     (instr & 0x001000f0) == 0x000000f0)   /* STRD */
			handler = do_alignment_ldrdstrd;
		else if ((instr & 0x01f00ff0) == 0x01000090) /* SWP */
			goto swp;
		else
			goto bad; /* should not be reached */
		break;

	case 0x04000000:	/* ldr or str immediate */
		offset.un = OFFSET_BITS(instr);
		handler = do_alignment_ldrstr;
		break;

	case 0x06000000:	/* ldr or str register */
        offset.un = regs->r[RM_BITS(instr)];

		if (IS_SHIFT(instr)) {
			unsigned int shiftval = SHIFT_BITS(instr);

			switch(SHIFT_TYPE(instr)) {
			case SHIFT_LSL:
				offset.un <<= shiftval;
				break;

			case SHIFT_LSR:
				offset.un >>= shiftval;
				break;

			case SHIFT_ASR:
				offset.sn >>= shiftval;
				break;

			case SHIFT_RORRRX:
				if (shiftval == 0) {
					offset.un >>= 1;
                    if (regs->cpsr &  PSR_C_BIT)
						offset.un |= 1 << 31;
				} else
					offset.un = offset.un >> shiftval |
							  offset.un << (32 - shiftval);
				break;
			}
		}
		handler = do_alignment_ldrstr;
		break;

	case 0x08000000:	/* ldm or stm */
		handler = do_alignment_ldmstm;
		break;

	default:
		goto bad;
	}

    type = handler(addr, instr, regs);

	if (type == TYPE_ERROR || type == TYPE_FAULT)
		goto bad_or_fault;

	if (type == TYPE_LDST)
		do_alignment_finish_ldst(addr, instr, regs, offset);

	return 0;

 bad_or_fault:
	if (type == TYPE_ERROR)
		goto bad;
	return 0; /* continue regular vxWorks treatment */

 swp:
	mvOsPrintf("Alignment trap: not handling swp instruction\n");

 bad:
	ai_skipped += 1;
	return 0; /* continue regular vxWorks treatment */
}
#endif /* _DIAB_TOOL */

