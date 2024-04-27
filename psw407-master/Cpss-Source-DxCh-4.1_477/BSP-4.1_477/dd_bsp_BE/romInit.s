/* romInit.s - ARM Integrator ROM initialization module */

/* Copyright 1999-2001 ARM Limited */
/* Copyright 1999-2004 Wind River Systems, Inc. */

/*
modification history
--------------------
01n,26may04,dmh  support 926ej
01m,25jan02,m_h  sdata needs "_" for bootrom_res
01l,09oct01,jpd  added clock speed setting for 946ES.
01k,03oct01,jpd  tidied slightly.
01j,28sep01,pr   added support for ARM946ES.
01i,04jun01,rec  memory clock rate changes for 740t
01h,21feb01,h_k  added support for ARM966ES and ARM966ES_T.
01g,20nov00,jpd  change speeds on 920T and add conditional early
		 enabling of I-cache on 920T.
01f,18sep00,rec  Add delay during power up
01e,23feb00,jpd  comments changes.
01d,22feb00,jpd  changed copyright string.
01c,20jan00,jpd  added support for ARM720T/ARM920T.
01b,13jan00,pr	 added support for ARM740T.
01a,30nov99,ajb  created, based on PID version 01i.
*/

/*
DESCRIPTION
This module contains the entry code for VxWorks images that start
running from ROM, such as 'bootrom' and 'vxWorks_rom'.  The entry
point, romInit(), is the first code executed on power-up.  It performs
the minimal setup needed to call the generic C routine romStart() with
parameter BOOT_COLD.

romInit() masks interrupts in the processor and the interrupt
controller and sets the initial stack pointer (to STACK_ADRS which is
defined in configAll.h).  Other hardware and device initialisation is
performed later in the sysHwInit routine in sysLib.c.

The routine sysToMonitor() jumps to a location after the beginning of
romInit, (defined by ROM_WARM_ADRS) to perform a "warm boot".  This
entry point allows a parameter to be passed to romStart().

The routines in this module don't use the "C" frame pointer %r11@ ! or
establish a stack frame.

SEE ALSO:
.I "ARM Architecture Reference Manual,"
*/

#define	_ASMLANGUAGE
#define MV_ASMLANGUAGE
#include "vxWorks.h"
#include "sysLib.h"
#include "asm.h"
#include "regs.h"	
#include "config.h"
#include "arch/arm/mmuArmLib.h"
#include "mvOsAsm.h"
#include "mvCtrlEnvSpec.h"
#include "mvCpuIfRegs.h"
#include "mvPexRegs.h"
#include "mvAhbToMbusRegs.h"
#include "mvCommon.h"
#include "mvDeviceId.h"
#include "mvSpiSpec.h"
#include "sys/mvCpuIfRegs.h"
#define FPEXC_EN_BIT       (1<<30) /* global enable bit */
#define MMU_INIT_VALUE_ENABLE (MMUCR_I_ENABLE | MMUCR_PROG32)

        .data
        .globl   VAR(copyright_wind_river)
		.globl   VAR(L$_rConfigDramInRom)
        .long    VAR(copyright_wind_river)

/* internals */

	.globl	FUNC(romInit)		/* start of system code */
	.globl	FUNC(resetVector )		/* start of system code */
	.globl	FUNC(resetEntry  )		/* start of system code */
	.globl	VAR(sdata)		/* start of data */
        .globl  _sdata


/* externals */

	.extern	FUNC(romStart)	/* system initialization routine */
	.extern	FUNC(_mvDramIfStaticInit)	/* DRAM initialization routine */
	.extern	FUNC(_mvDramIfConfig)	/* DRAM initialization routine */

_sdata:
VAR_LABEL(sdata)
	.asciz	"start of data"
	.balign	4

/* variables */

	.data
	.text
	.balign 4

/*******************************************************************************
*
* romInit - entry point for VxWorks in ROM
*

* romInit
*     (
*     int startType	/@ only used by 2nd entry point @/
*     )

* INTERNAL
* sysToMonitor examines the ROM for the first instruction and the string
* "Copy" in the third word so if this changes, sysToMonitor must be updated.
*/

_ARM_FUNCTION(romInit)
_romInit:
cold:
	MOV		r0, #BOOT_COLD	/* fall through to warm boot entry */
	B		start
warm:
	MV_REG_READ_ASM r6, r1, CPU_CTRL_STAT_REG(0)
	ORR     r6, r6, #(CCSR_ARM_RESET)
	MV_REG_WRITE_ASM r6, r1,CPU_CTRL_STAT_REG(0)
	B		start
L$_rConfigDramInRom:
	.long	ROM_TEXT_ADRS + FUNC(_mvDramIfConfig) - FUNC(romInit)

	/* copyright notice appears at beginning of ROM (in TEXT segment) */

	.ascii   "Copyright 1999-2001 ARM Limited"
	.ascii   "\nCopyright 1999-2001 Wind River Systems, Inc."
	.balign 4

start:
		mov		r12,r0			/* save boot type */
		MOV    	sp, #0x0		/* clear stack pointer*/
		MOV		r1, #MMU_INIT_VALUE		/* Defined in mmuArmLib.h */
		MCR		CP_MMU, 0, r1, c1, c0, 0	/* Write to MMU CR */
/*************************************************************************/
/* Load code to I-cache */
		adr	r4, _romInit		/* r4 <- current position of code   */
		mvn	r5, #0xff
		and	r4, r4, r5
/* Add for load code into I cache */
	/*
	 * flush v4 I/D caches
	 */
		mov	r0, #0
		mcr	p15, 0, r0, c7, c7, 0	/* flush v3/v4 cache */
		mcr	p15, 0, r0, c8, c7, 0	/* flush v4 TLB */


	/* Load source code from 0xfff80000-0xfff83fff */
        mov   r8, r4		/* bootrom start code on flash */
        mov   r2, #0x2000	/* bootrom size of unzip code  */

	/*
	 * disable MMU stuff and caches
	 */
		mrc	p15, 0, r0, c1, c0, 0
		bic	r0, r0, #0x00000007	/* 2:0 (CAM) */
		orr	r0, r0, #0x00000002	/* set bit 2 (A) Align */
		orr	r0, r0, #0x00001000	/* Enabled I-cache */	
		mcr	p15, 0, r0, c1, c0, 0
		nop;nop;nop;nop;
	/* Lock I-cache */
		mrc	p15, 0, r0, c9, c0, 1
		orr	r0, r0, #0xe
		mcr	p15, 0, r0, c9, c0, 1


	/* Load code into I Cache */
load_loop:
		mcr   p15, 0, r8, c7, c13, 1
        add   r8, r8, #32 	/* 8 dwords * 4 bytes */
        sub   r2, r2, #32 	/* 8 dwords * 4 bytes */
        cmp   r2, #0 /* check if we have read a full Page */
        bne   load_loop

/*  start loop 2 */
		mov   r2, #0x2000	/* bootrom size of unzip code  */

	/* Lock I-cache */
		mrc	p15, 0, r0, c9, c0, 1
		orr	r0, r0, #0xd
		mcr	p15, 0, r0, c9, c0, 1


	/* Load code into I Cache */
load_loop2:
		mcr   p15, 0, r8, c7, c13, 1
        add   r8, r8, #32 	/* 8 dwords * 4 bytes */
        sub   r2, r2, #32 	/* 8 dwords * 4 bytes */
        cmp   r2, #0 /* check if we have read a full Page */
        bne   load_loop2


/* Lock I-cache */
		mrc	p15, 0, r0, c9, c0, 1
		orr	r0, r0, #0x3
		mcr	p15, 0, r0, c9, c0, 1

/* End of code load */

#if defined (MV78XX0)
        ldr     r1, =(INTERNAL_REG_BASE_DEFAULT + (AHB_TO_MBUS_WIN_INTEREG_REG(0)))
#elif defined (MV88F6XXX) 
	ldr     r1, =(INTERNAL_REG_BASE_DEFAULT + AHB_TO_MBUS_WIN_INTEREG_REG)
#endif
       	ldr     r4, =htoll(INTER_REGS_BASE)
        str     r4, [r1]

#if defined(MV_INC_BOARD_SPI_FLASH)
	/* configure the Prescale of SPI clk Tclk = 200MHz */
	MV_REG_READ_ASM r6, r1, MV_SPI_IF_CONFIG_REG
	and	r6, r6, #~MV_SPI_CLK_PRESCALE_MASK
	orr	r6, r6, #0x15
	MV_REG_WRITE_ASM r6, r1, MV_SPI_IF_CONFIG_REG
#endif
#if defined (MV78XX0)

        MOV     r0, #0                /* return OK */

        MRC     p15, 0, r1, c1, c0, 2 /* r1 = Access Control Register      */
        ORR     r1, r1, #(0xf << 20)  /* enable full access for p10,11     */
        MCR     p15, 0, r1, c1, c0, 2 /* Access Control Register = r1      */
        MOV     r1, #0
        MCR     p15, 0, r1, c7, c5, 4 /* flush prefetch buffer because of  */
                                      /* of FMXR below and CP 10 & 11 were */
                                      /* only just enabled                 */

#ifdef MV_VFP
        /* Enable VFP itself */
        MOV     r1, #FPEXC_EN_BIT
        FMXR    FPEXC, r1             /* FPEXC = r1 */
#endif  /* MV_VFP */
/* Setting the PEX header device ID for MV78200 */
    	MV_REG_READ_ASM r6, r1, PEX_CFG_DIRECT_ACCESS(0,PEX_DEVICE_AND_VENDOR_ID)
#if defined (MV78100)
		ldr   r1, =MV_78100_DEV_ID    
#elif defined (MV78200)
		ldr   r1, =MV_78200_DEV_ID    
#elif
error " CHIP is not define MV78100 or MV78200"
#endif
    	ldr   r2, =0xffff
    	and   r6, r6, r2
    	orr   r6, r6, r1, LSL #PXDAVI_DEV_ID_OFFS
    	MV_REG_WRITE_ASM r6, r1, PEX_CFG_DIRECT_ACCESS(0,PEX_DEVICE_AND_VENDOR_ID)
 
#endif  /* MV78XX0 */
 

#if defined (MV88F6XXX) 

		LDR	r2, =(INTER_REGS_BASE+ CPU_L2_CONFIG_REG)
		ldr r1,[r2]
		orr	r1,r1,#BIT4
		str r1,[r2]
#endif
		
		
#if defined(CPU_720T)  || defined(CPU_720T_T) || \
    defined(CPU_740T)  || defined(CPU_740T_T) || \
    defined(CPU_920T)  || defined(CPU_920T_T) || \
    defined(CPU_940T)  || defined(CPU_940T_T) || \
    defined(CPU_926EJ) || defined(CPU_926EJ_T) || \
    defined(CPU_946ES) || defined(CPU_946ES_T) || \
    defined(CPU_1020E) || defined(CPU_1022E)

	/*
	 * Set processor and MMU to known state as follows (we may have not
	 * been entered from a reset). We must do this before setting the CPU
	 * mode as we must set PROG32/DATA32.
	 *
	 * MMU Control Register layout.
	 *
	 * bit
	 *  0 M 0 MMU disabled
	 *  1 A 0 Address alignment fault disabled, initially
	 *  2 C 0 Data cache disabled
	 *  3 W 0 Write Buffer disabled
	 *  4 P 1 PROG32
	 *  5 D 1 DATA32
	 *  6 L 1 Should Be One (Late abort on earlier CPUs)
	 *  7 B ? Endianness (1 => big)
	 *  8 S 0 System bit to zero } Modifies MMU protections, not really
	 *  9 R 1 ROM bit to one     } relevant until MMU switched on later.
	 * 10 F 0 Should Be Zero
	 * 11 Z 0 Should Be Zero (Branch prediction control on 810)
	 * 12 I 0 Instruction cache control
	 */

	/* Setup MMU Control Register */

	MOV	r1, #MMU_INIT_VALUE		/* Defined in mmuArmLib.h */
	ORR	r1, r1, #MMUCR_I_ENABLE		/* conditionally enable Icache*/
	MCR	CP_MMU, 0, r1, c1, c0, 0	/* Write to MMU CR */
#if defined(CPU_1020E)
	NOP
	NOP
	NOP
#endif /* defined(CPU_1020E) */

	/*
	 * If MMU was on before this, then we'd better hope it was set
	 * up for flat translation or there will be problems. The next
	 * 2/3 instructions will be fetched "translated" (number depends
	 * on CPU).
	 *
	 * We would like to discard the contents of the Write-Buffer
	 * altogether, but there is no facility to do this. Failing that,
	 * we do not want any pending writes to happen at a later stage,
	 * so drain the Write-Buffer, i.e. force any pending writes to
	 * happen now.
	 */


#if defined(CPU_720T)  || defined(CPU_720T_T) || \
    defined(CPU_920T)  || defined(CPU_920T_T) || \
    defined(CPU_1020E) || defined(CPU_1022E)
        /*
	 * Set Process ID Register to zero, this effectively disables
	 * the process ID remapping feature.
	 */

	MOV	r1, #0
	MCR	CP_MMU, 0, r1, c13, c0, 0
#if defined(CPU_1020E)
        NOP
        NOP
        NOP
#endif /* defined(CPU_1020E) */

#endif /* defined(CPU_720T,920T,1020E, 1022E) */

#endif /* defined(CPU_720T,740T,920T,940T,946ES,1020E,1022E) */
_mmuDisable:
        /* Set CPU Control and Status Registers MMU disabled bit */
        /* according to selected memory managment method         */
#if defined (MV78XX0)
        ldr     r1, =(INTER_REGS_BASE + CPU_CONFIG_REG(0))
        ldr     r4, [r1]
#if defined(CPU_946ES) || defined(CPU_946ES_T)
        orr	r4, r4, #htoll(CCR_MMU_DISABLED_MASK)
#else
		bic	r4, r4, #htoll(CCR_MMU_DISABLED_MASK) 
#endif    
#elif defined (MV88F6XXX) 
	ldr     r1, =(INTER_REGS_BASE + CPU_CONFIG_REG)
	ldr     r4, [r1]
#if defined(CPU_946ES) || defined(CPU_946ES_T)
        orr	r4, r4, #htoll(CCR_NCB_BLOCKING_MASK)
#else
		bic	r4, r4, #htoll(CCR_NCB_BLOCKING_MASK) 
#endif    
#endif    
    
        str     r4, [r1]

	/* disable interrupts in CPU and switch to SVC32 mode */
_disable_interrupts:
	MRS	r1, cpsr
	BIC	r1, r1, #MASK_MODE
	ORR	r1, r1, #MODE_SVC32 | I_BIT | F_BIT
	MSR	cpsr, r1

	/*
	 * CPU INTERRUPTS DISABLED
	 *
	 * disable individual interrupts in the interrupt controller
	 */
_disableCtrlInterrupts:

	MOV	r1, #0
#if defined(MV78XX0) 
			LDR	r2, =(INTER_REGS_BASE + CPU_INT_MASK_HIGH_REG(0))
			STR	r1, [r2]	/* disable all main high cause sources */
			LDR	r2, =(INTER_REGS_BASE + CPU_INT_MASK_LOW_REG(0))
			STR	r1, [r2]	/* disable all main low cause sources */
			LDR	r2, =(INTER_REGS_BASE + CPU_INT_MASK_ERROR_REG(0))
			STR	r1, [r2]	/* disable all main erro cause sources */
#elif defined (MV88F6XXX) 
			LDR	r2, =(INTER_REGS_BASE + CPU_MAIN_IRQ_MASK_HIGH_REG)
			STR	r1, [r2]	/* disable all main high cause sources */
			LDR	r2, =(INTER_REGS_BASE + CPU_MAIN_IRQ_MASK_REG)
			STR	r1, [r2]	/* disable all main low cause sources */
#endif


/*
 * Initialize the stack pointer to just before where the
 * uncompress code, copied from ROM to RAM, will run.
 */
#if defined(MV78XX0) 
			ldr    	sp, =0x0
			cmp    	r12, #BOOT_COLD
			bne	    DramDetectEnd
			
#if defined(MV_STATIC_DRAM_ON_BOARD) 
			bl     _mvDramIfStaticInit
#else
			bl     _mvDramIfBasicInit
#endif
			ldr    r5, =(LOCAL_MEM_LOCAL_ADRS+BOOT_LINE_OFFSET)
			ldr    r0, =0
			STR		r0,[r5]
#endif
DramDetectEnd:

			LDR	sp, L$_STACK_ADDR
			mov	r0,r12		/* restore r0 = BOOT type */
					
			MOV	fp, #0			/* zero frame pointer */
		
	/* jump to C entry point in ROM: routine - entry point + ROM base */

#if	(ARM_THUMB)
	LDR	r12, L$_rStrtInRom
	ORR	r12, r12, #1		/* force Thumb state */
	BX	r12
#else
	LDR	pc, L$_rStrtInRom
#endif	/* (ARM_THUMB) */

/******************************************************************************/

/*
 * PC-relative-addressable pointers - LDR Rn,=sym is broken
 * note "_" after "$" to stop preprocessor performing substitution
 */

	.balign	4

L$_rStrtInRom:
	.long	ROM_TEXT_ADRS + FUNC(romStart) - FUNC(romInit)

L$_STACK_ADDR:
	.long	STACK_ADRS

#if defined(CPU_940T) || defined (CPU_940T_T)

L$_sysCacheUncachedAdrs:
	.long	SYS_CACHE_UNCACHED_ADRS
#endif /* defined(CPU_940T, CPU_940T_T) */

/****************************************************************************
*
* resetVector - hardware reset vector
*
* At power-on, the PPC440 starts executing at ROM address 0xfffffffc -- the
* top of the address space -- which must be a jump to the reset entry point.
* This is defined as a separate "section" to assist the linker in locating
* it properly.
*/
#if defined(MV78XX0) 

	.section .reset, "ax", @progbits 

_ARM_FUNCTION(resetVector)
	B	cold
	B	warm
L$_rConfigDramInRom10:
	.long	ROM_TEXT_ADRS 
	.long	FUNC(_mvDramIfConfig)
	.long	FUNC(romInit) 
#endif
