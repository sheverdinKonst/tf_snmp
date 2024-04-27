/* sysALib.s - ARM Integrator system-dependent routines */
/* Copyright 1999-2003 ARM Limited */
/* Copyright 1999-2004 Wind River Systems, Inc. */
/*
modification history
--------------------
01j,26may04,dmh  support 926ej
01i,04feb03,jb   Adding ARM10 Support
01h,09oct01,jpd  added clock speed setting for 946ES.
01g,03oct01,jpd  tidied slightly.
01f,28sep01,pr   added support for ARM946E.
01g,04jun01,rec  memory clock rate changes for 740t
01f,21feb01,h_k  added support for ARM966ES and ARM966ES_T.
01e,23oct00,jpd  changed speeds on 920T; added conditional early
		 enabling of instruction cache on 920T.
01d,21feb00,jpd  added further initialisation code.
01c,07feb00,jpd  added support for ARM720T and ARM920T.
01b,13jan00,pr	 added support for ARM740T.
01a,15nov99,ajb  copied from pid940t version 01h.
*/

/*
DESCRIPTION
This module contains system-dependent routines written in assembly
language.  It contains the entry code, sysInit(), for VxWorks images
that start running from RAM, such as 'vxWorks'.  These images are
loaded into memory by some external program (e.g., a boot ROM) and then
started.  The routine sysInit() must come first in the text segment.
Its job is to perform the minimal setup needed to call the generic C
routine usrInit().

sysInit() masks interrupts in the processor and the interrupt
controller and sets the initial stack pointer.  Other hardware and
device initialisation is performed later in the sysHwInit routine in
sysLib.c.

NOTE
The routines in this module don't use the "C" frame pointer %r11@ ! or
establish a stack frame.

SEE ALSO:
.I "ARM Architecture Reference Manual,"
.I "ARM 7TDMI Data Sheet,"
.I "ARM 720T Data Sheet,"
.I "ARM 740T Data Sheet,"
.I "ARM 920T Technical Reference Manual",
.I "ARM 940T Technical Reference Manual",
.I "ARM 946E-S Technical Reference Manual",
.I "ARM 966E-S Technical Reference Manual",
.I "ARM 1020E Technical Reference Manual",
.I "ARM 1022E Technical Reference Manual",
.I "ARM Reference Peripherals Specification,"
.I "ARM Integrator/AP User Guide",
.I "ARM Integrator/CM7TDMI User Guide",
.I "ARM Integrator/CM720T User Guide",
.I "ARM Integrator/CM740T User Guide",
.I "ARM Integrator/CM920T User Guide",
.I "ARM Integrator/CM940T User Guide",
.I "ARM Integrator/CM946E User Guide",
.I "ARM Integrator/CM9x6ES Datasheet".
.I "ARM Integrator/CM10200 User Guide",
*/


#define _ASMLANGUAGE
#define MV_ASMLANGUAGE
#include "vxWorks.h"
#include "asm.h"
#include "regs.h"
#include "sysLib.h"
#include "config.h"
#include "arch/arm/mmuArmLib.h"
#include "mvCpuIfRegs.h"
#include "mvOsAsm.h"

    .data
    .globl VAR(copyright_wind_river)
    .long  VAR(copyright_wind_river)

    /* internals */
    .globl FUNC(sysInit)		/* start of system code */
    .globl FUNC(sysIntStackSplit)	/* routine to split interrupt stack */

/* externals */

    .extern FUNC(usrInit)		/* system initialization routine */

    .extern FUNC(vxSvcIntStackBase) /* base of SVC-mode interrupt stack */
    .extern FUNC(vxSvcIntStackEnd)	/* end of SVC-mode interrupt stack */
    .extern FUNC(vxIrqIntStackBase) /* base of IRQ-mode interrupt stack */
    .extern FUNC(vxIrqIntStackEnd)	/* end of IRQ-mode interrupt stack */

#if defined(CPU_720T) || defined(CPU_720T_T) || \
    defined(CPU_740T) || defined(CPU_740T_T)

/* variables */

    .data
    .balign	1			/* no alignment necessary */

    /* variable used with a SWPB instruction to drain the write-buffer */

sysCacheSwapVar:
    .byte	0
    .balign	4

#endif /* defined(720T/720T_T740T/740T_T) */

    .text
    .balign 4

/*******************************************************************************
*
* sysInit - start after boot
*
* This routine is the system start-up entry point for VxWorks in RAM, the
* first code executed after booting.  It disables interrupts, sets up
* the stack, and jumps to the C routine usrInit() in usrConfig.c.
*
* The initial stack is set to grow down from the address of sysInit().  This
* stack is used only by usrInit() and is never used again.  Memory for the
* stack must be accounted for when determining the system load address.
*
* NOTE: This routine should not be called by the user.
*
* RETURNS: N/A
* sysInit ()              /@ THIS IS NOT A CALLABLE ROUTINE @/
*/

_ARM_FUNCTION(sysInit)
#if defined(MV_CPU_BE)
        /* convert CPU to big endian - if already uBoot is BE - andne/i
        commands - meaningless commands */
        .word 0x100f11ee /* mrc p15, 0, r0, c1, c0 */
        .word 0x800080e3 /* orr r0, r0, #0x80 */
        .word 0x100f01ee /* mcr p15, 0, r0, c1, c0 */
        nop ; nop ; nop ; nop ; nop ; nop ; nop ; nop
#else
        /* convert CPU to little endian - if already uBoot is LE -
        andne/i commands - meaningless commands */
        .word 0x100f11ee /* mrc p15, 0, r0, c1, c0 */
        .word 0x8000c0e3 /* orr r0, r0, #0xc0 */
        .word 0x100f01ee /* mcr p15, 0, r0, c1, c0 */

        nop ; nop ; nop ; nop ; nop ; nop ; nop ; nop
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

    LDR     r1, =MMU_INIT_VALUE             /* Defined in mmuArmLib.h */

#if defined(CPU_920T) || defined(CPU_920T_T) || \
    defined(CPU_1020E) || defined(CPU_1022E)
#if defined(INTEGRATOR_EARLY_I_CACHE_ENABLE)
        ORR     r1, r1, #MMUCR_I_ENABLE         /* conditionally enable Icache*/
#endif /* defined(INTEGRATOR_EARLY_I_CACHE_ENABLE) */
#endif /* defined(CPU_920T/920T_T/1020E/1022E) */

    /* Ref # FE-22040
     * (Incorrect prediction resulting in incorrect command execution)
     */ 
    /* Enable BPU (Branch prediction unit) */
    ORR    r1, r1, #(1<<11)
    MCR    CP_MMU, 0, r1, c1, c0, 0

    /* Ref # RES-CPU-150 (5 NOPs after enabling BPU) */
    /* Ref # FE-22040    (5 NOPs after enabling BPU) */
    NOP
    NOP
    NOP
    NOP
    NOP

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

    bl     sysOpenMemWinToSwitch

    /* ascertain xCat chip revision */
    ldr    r2, =0xF400004C
    ldr    r2, [r2]
    bic    r2, r2, #~0xF /* mask for revision bits */
    ldr    r3, =_G_xCatRevision
    str    r2, [r3]

    /* ascertain board type: GE or FE */
    ldr    r2, =0xF400004C
    ldr    r2, [r2]
    bic    r2, r2, #~0x10 /* mask for board type */
    mov    r2, r2, LSR #4
    ldr    r3, =_G_xCatIsFEBoardType
    str    r2, [r3]


#if defined(CPU_720T) || defined(CPU_720T_T) || \
    defined(CPU_740T) || defined(CPU_740T_T)
    LDR    r2, L$_sysCacheSwapVar	/* R2 -> sysCacheSwapVar */
    SWPB   r1, r1, [r2]

    /* Flush, (i.e. invalidate) all entries in the ID-cache */

    MCR    CP_MMU, 0, r1, c7, c0, 0	/* Flush (inval) all ID-cache */
#endif /* defined(CPU_720T,740T) */

#if defined(CPU_920T)  || defined(CPU_920T_T) || \
    defined(CPU_926EJ) || defined(CPU_926EJ_T) || \
    defined(CPU_946ES) || defined(CPU_946ES_T) || \
    defined(CPU_1020E) || defined(CPU_1022E)
    MOV    r1, #0				/* data SBZ */
    MCR    CP_MMU, 0, r1, c7, c10, 4	/* drain write-buffer */
#if defined(CPU_1020E)
    NOP
    NOP
    NOP
#endif /* defined(CPU_1020E) */

    /* Flush (invalidate) both I and D caches */

    MCR    CP_MMU, 0, r1, c7, c7, 0	/* R1 = 0 from above, data SBZ*/
#if defined(CPU_1020E)
    NOP
    NOP
    NOP
#endif /* defined(CPU_1020E) */
#endif /* defined(CPU_920T,946ES,1020E,1022E) */

#if defined(CPU_940T) || defined(CPU_940T_T)
    LDR    r1, L$_sysCacheUncachedAdrs	/* R1 -> uncached area */
    LDR    r1, [r1]			/* drain write-buffer */

    /* Flush (invalidate) both caches */

    MOV    r1, #0				/* data SBZ */
    MCR    CP_MMU, 0, r1, c7, c5, 0	/* Flush (inval) all I-cache */
    MCR    CP_MMU, 0, r1, c7, c6, 0	/* Flush (inval) all D-cache */
#endif /* defined(CPU_940T,940T_T) */

#if defined(CPU_720T)  || defined(CPU_720T_T) || \
    defined(CPU_920T)  || defined(CPU_920T_T) || \
    defined(CPU_1020E) || defined(CPU_1022E)
    /*
     * Set Process ID Register to zero, this effectively disables
     * the process ID remapping feature.
     */

    MOV     r1, #0
    MCR     CP_MMU, 0, r1, c13, c0, 0
#if defined(CPU_1020E)
    NOP
    NOP
    NOP
#endif /* defined(CPU_1020E) */
#endif /* defined(CPU_720T,920T,1020E, 1022E) */

#endif /* defined(CPU_720T,740T,920T,940T,946ES,1020E,1022E) */

    /* Set CPU Control and Status Registers MMU disabled bit */
    /* according to selected memory managment method         */
#if defined (MV78XX0)
    ldr    r1, =(INTER_REGS_BASE + CPU_CONFIG_REG(0))
    ldr    r4, [r1]
#if defined(CPU_946ES) || defined(CPU_946ES_T)
    orr    r4, r4, #htoll(CCR_MMU_DISABLED_MASK)
#else
    bic    r4, r4, #htoll(CCR_MMU_DISABLED_MASK)
#endif
#elif defined (MV88F6XXX)
    ldr    r1, =(INTER_REGS_BASE + CPU_CONFIG_REG)
    ldr    r4, [r1]
#if defined(CPU_946ES) || defined(CPU_946ES_T)
    orr    r4, r4, #htoll(CCR_NCB_BLOCKING_MASK)
#else
    bic    r4, r4, #htoll(CCR_NCB_BLOCKING_MASK)
#endif
#endif

    str    r4, [r1]

    /* disable interrupts in CPU and switch to SVC32 mode */

    MRS    r1, cpsr
    BIC    r1, r1, #MASK_MODE
    ORR    r1, r1, #MODE_SVC32 | I_BIT | F_BIT
    MSR    cpsr, r1

    /*
     * CPU INTERRUPTS DISABLED
     *
     * disable individual interrupts in the interrupt controller
     */

    MOV    r1, #0
#if defined(MV78XX0)
#if defined (MV78100)
    LDR    r2, =(INTER_REGS_BASE + CPU_INT_MASK_HIGH_REG(0))
    STR    r1, [r2]	/* disable all main high cause sources */
    LDR    r2, =(INTER_REGS_BASE + CPU_INT_MASK_LOW_REG(0))
    STR    r1, [r2]	/* disable all main low cause sources */
    LDR    r2, =(INTER_REGS_BASE + CPU_INT_MASK_ERROR_REG(0))
#else
    LDR    r2, =(INTER_REGS_BASE + CPU_INT_MASK_HIGH_REG(1))
    LDR    r2, =(INTER_REGS_BASE + CPU_INT_MASK_HIGH_REG(1))
    STR    r1, [r2]	/* disable all main high cause sources */
    LDR    r2, =(INTER_REGS_BASE + CPU_INT_MASK_LOW_REG(1))
    STR    r1, [r2]	/* disable all main low cause sources */
    LDR    r2, =(INTER_REGS_BASE + CPU_INT_MASK_ERROR_REG(1))
    STR    r1, [r2]	/* disable all main erro cause sources */
#endif
#elif defined (MV88F6XXX)
    LDR    r2, =(INTER_REGS_BASE + CPU_MAIN_IRQ_MASK_HIGH_REG)
    STR    r1, [r2] /* disable all main high cause sources */
    LDR    r2, =(INTER_REGS_BASE + CPU_MAIN_IRQ_MASK_REG)
#endif
    STR    r1, [r2]	/* disable all main erro cause sources */
    /* set initial stack pointer so stack grows down from start of code */

    ADR    sp, FUNC(sysInit)		/* initialise stack pointer */

    /* now call usrInit */
    MOV    fp, #0			/* initialise frame pointer */
    MOV    r0, #BOOT_WARM_AUTOBOOT	/* pass startType */

#if	(ARM_THUMB)
    LDR    r12, L$_usrInit
    BX     r12
#else
    B FUNC(usrInit)
#endif	/* (ARM_THUMB) */

/*********************************************************************
 *  sysCacheConfig                                                  **
 *  Ref # GL-MISL-70 (Speculative Instruction Prefetch)             **
 *********************************************************************/
.globl   FUNC(sysCacheConfig)
_ARM_FUNCTION(sysCacheConfig)
    stmdb   sp!, {r3}

#ifdef INCLUDE_L2_CACHE
    /* Enable L2 Cache */
    mrc     p15, 1, r3, c15, c1, 0
    orr     r3, r3, #BIT22          /* L2Enable bit = 1 */
    mcr     p15, 1, r3, c15, c1, 0
#else
    /* Disable L2 Cache */
    mrc     p15, 1, r3, c15, c1, 0
    bic     r3, r3, #BIT22          /* L2Enable bit = 0 (to disable) */
    mcr     p15, 1, r3, c15, c1, 0
#endif /* INCLUDE_L2_CACHE */

#if defined(USER_I_CACHE_ENABLE) && defined(USER_D_CACHE_ENABLE)
#ifdef INCLUDE_L2_CACHE

    /* Disable L1 ICache and Dcache */
    mrc     p15, 0 , r3, c1, c0, 0
    bic     r3 , r3, #BIT2          /* C=0: disable DCache */
    bic     r3 , r3, #BIT12         /* I=0: disable ICache */
    mcr     p15, 0 , r3, c1, c0, 0

    /*
     * Enable L2 cache
     */

#ifdef CONFIG_MV_SP_I_FTCH_LCK_L2_ICACHE
    /* Lock all Instructions Ways of L2 Cache
     * When all the ways are set (locked), the Instructions line fill is
     * forwarded to L1 without updating L2 cache controller
     */
    mrc     p15, 1, r3, c15, c10, 7 /* Read  L2 Cache Way Lockdown Register */
    orr     r3, r3, #0x000000FF     /* Set bits to lock all Instructions Ways       */
    mcr     p15, 1, r3, c15, c10, 7 /* Write L2 Cache Way Lockdown Register */
#endif

    /* Enable L2 Cache */
    mrc     p15, 1, r3, c15, c1, 0
    orr     r3, r3, #BIT22          /* L2Enable bit = 1 */
    mcr     p15, 1, r3, c15, c1, 0

    /* Enable L1 ICache and DCache */
    mrc     p15, 0, r3, c1, c0, 0
    orr     r3, r3, #BIT12          /* I=1: enable ICache */
    orr     r3, r3, #BIT2           /* C=1: enable DCache */
    mcr     p15, 0, r3, c1, c0, 0
#endif /* INCLUDE_L2_CACHE */

    /* Invalidate and Unlock L1 ICache */
    mov     r3, #0
    mcr     p15, 1, r3, c7, c5, 0
    nop
    nop
    mcr     p15, 1, r3, c9, c0, 1
    nop
    nop

#endif /* USER_I_CACHE_ENABLE && USER_D_CACHE_ENABLE */
    ldmia   sp!, {r3}
    mov     pc, lr

/*********************************************************************
 * void  mv_l2_inv_range(start, end)                                **
 *                                                                  **
 *  Invalidate (discard) the specified virtual address range on L2. **
 *  May not write back any entries.  If 'start' or 'end'            **
 *  are not cache line aligned, those lines must be written back.   **
 *  If start > end then behavior is unpredictable.                  **
 *  Once the End Address register is written, the range operation   **
 *  starts its process. It blocks CPU processing, and interrupts    **
 *  are not served until the operation is completed in L2.          **
 *                                                                  **
 *  - start - virtual start address                                 **
 *  - end   - virtual end address                                   **
 *********************************************************************/
#ifdef CONFIG_MV_SP_I_FTCH_DB_INV
#define PSR_F_BIT 0x40
#define PSR_I_BIT 0x80
.globl   FUNC(mv_l2_inv_range)
_ARM_FUNCTION(mv_l2_inv_range)
    stmdb   sp!, {r3}
    cmp     r1, r0
    subne   r1, r1, #1                    /* Prevent cleaning of top address  */
    mrs     r2, cpsr
    orr     r3, r2, #(PSR_F_BIT | PSR_I_BIT)
    msr     cpsr_c, r3                    /* Disable interrupts */
                                          /* cache line when top is cache line aligned */
    mcr     p15, 1, r0, c15, c11, 4       /* L2 invalidation zone start addr */
    mcr     p15, 1, r1, c15, c11, 5       /* L2 invalidation zone end addr and  */
                                          /* invalidate procedure trigger */
    msr     cpsr_c, r2                    /* Restore interrupts */
    ldmia   sp!, {r3}
    mov     pc, lr
#endif

/*******************************************************************************
 * void sysOpenMemWinToSwitch(void)
 *
 * Searches for open memory window. If not found, opens one.
 *
 */
.globl   FUNC(sysOpenMemWinToSwitch)
_ARM_FUNCTION(sysOpenMemWinToSwitch)
    stmdb   sp!, {r3}

     /*
     * Configure memory window to Prestera Switch
     * Check if memory window is configured. If not, configure it.
     */

    ldr    r2, =INTER_REGS_BASE
    ldr    r3, =0x20000
    add    r2, r2, r3           /* points to memory window */

    ldr    r5, =INTER_REGS_BASE
    ldr    r3, =0x20080
    add    r5, r5, r3           /* points to last memory window (reg space) */

mem_win_cfg_loop:
    cmp    r2, r5               /* check loop stop condition */
    bne    continue_mem_win_cfg_loop
    b      mem_win_cfg_not_found

continue_mem_win_cfg_loop:
    ldr    r3, [r2]

    ldr    r4, =_G_MEM_WIN_2_PP_VAL
    ldr    r4, [r4]
    cmp    r3, r4               /* value of  window control register */
    beq    mem_win_cfg_check_base_reg
    add    r2, r2, #0x10        /* step to the next window control reg */
    bne    mem_win_cfg_loop     /* window control register is not configured */

mem_win_cfg_check_base_reg:
    /* window control register is ok, check  window base register now */
    add    r2, r2, #0x4         /* points to window base register */
    ldr    r3, [r2]             /* value of  window base register */
    ldr    r4, =_G_MEM_WIN_2_PP_BASE
    ldr    r4, [r4]
    cmp    r3, r4               /* value of  window base register */
    beq    mem_win_cfg_window_found
    add    r2, r2, #0xC         /* step to the next window control reg */
    bne    mem_win_cfg_loop     /* window base register is not configured */

mem_win_cfg_not_found:
    /*
     * Memory window to Prestera was not configured, ==> do it now
     */

    /* configured window control register */
    ldr    r2, =INTER_REGS_BASE
    ldr    r3, =0x20060
    add    r2, r2, r3           /* points to window control register */
    ldr    r4, =_G_MEM_WIN_2_PP_VAL /* addr of win control register value */
    ldr    r4, [r4]
    str    r4, [r2]             /* configure window control register */

    /* configured window base register */
    ldr    r2, =INTER_REGS_BASE
    ldr    r3, =0x20064
    add    r2, r2, r3           /* points to window base register */
    ldr    r4, =_G_MEM_WIN_2_PP_BASE
    ldr    r4, [r4]             /* value of  window base register */
    str    r4, [r2]             /* configure window base register */

mem_win_cfg_window_found:
    /* continue regular execution */


    ldmia   sp!, {r3}
    mov     pc, lr

/*******************************************************************************
*
* sysIntStackSplit - split interrupt stack and set interrupt stack pointers
*
* This routine is called, via a function pointer, during kernel
* initialisation.  It splits the allocated interrupt stack into IRQ and
* SVC-mode stacks and sets the processor's IRQ stack pointer. Note that
* the pointer passed points to the bottom of the stack allocated i.e.
* highest address+1.
*
* IRQ stack needs 6 words per nested interrupt;
* SVC-mode will need a good deal more for the C interrupt handlers.
* For now, use ratio 1:7 with any excess allocated to the SVC-mode stack
* at the lowest address.
*
* Note that FIQ is not handled by VxWorks so no stack is allocated for it.
*
* The stacks and the variables that describe them look like this.
* .CS
*
*         - HIGH MEMORY -
*     ------------------------ <--- vxIrqIntStackBase (r0 on entry)
*     |                      |
*     |       IRQ-mode       |
*     |    interrupt stack   |
*     |                      |
*     ------------------------ <--{ vxIrqIntStackEnd
*     |                      |    { vxSvcIntStackBase
*     |       SVC-mode       |
*     |    interrupt stack   |
*     |                      |
*     ------------------------ <--- vxSvcIntStackEnd
*         - LOW  MEMORY -
* .CE
*
* NOTE: This routine should not be called by the user.

* void sysIntStackSplit
*     (
*     char *pBotStack   /@ pointer to bottom of interrupt stack @/
*     long size		/@ size of stack @/
*     )

*/

_ARM_FUNCTION_CALLED_FROM_C(sysIntStackSplit)

	/*
	 * r0 = base of space allocated for stacks (i.e. highest address)
	 * r1 = size of space
	 */

	SUB	r2, r0, r1			/* r2->lowest usable address */
	LDR	r3, L$_vxSvcIntStackEnd
	STR	r2, [r3]			/*  == end of SVC-mode stack */
	SUB	r2, r0, r1, ASR #3		/* leave 1/8 for IRQ */
	LDR	r3, L$_vxSvcIntStackBase
	STR	r2, [r3]

	/* now allocate IRQ stack, setting irq_sp */

	LDR	r3, L$_vxIrqIntStackEnd
	STR	r2, [r3]
	LDR	r3, L$_vxIrqIntStackBase
	STR	r0, [r3]

	MRS	r2, cpsr
	BIC	r3, r2, #MASK_MODE
	ORR	r3, r3, #MODE_IRQ32 | I_BIT	/* set irq_sp */
	MSR	cpsr, r3
	MOV	sp, r0

	/* switch back to original mode and return */

	MSR	cpsr, r2

#if	(ARM_THUMB)
	BX	lr
#else
	MOV	pc, lr
#endif	/* (ARM_THUMB) */

.global _G_xCatRevision
_G_xCatRevision:
    .long   1 /* default xcat revision */

.global _G_xCatIsFEBoardType
_G_xCatIsFEBoardType:
    .long   0 /* default is GE */

.global _G_MEM_WIN_2_PP_VAL
_G_MEM_WIN_2_PP_VAL:
#ifdef MV_CPU_LE
    .long   0x03FF00C1
#else
    .long   0xC100FF03
#endif

.global _G_MEM_WIN_2_PP_BASE
_G_MEM_WIN_2_PP_BASE:
#ifdef MV_CPU_LE
    .long   0xF4000000
#else
    .long   0x000000F4
#endif

/******************************************************************************/
    .globl fiq_entry
    .type  fiq_entry, #function

    /* here, we can use r10..r13 to our pleasure */
fiq_entry:

    ldr     r11,    fiq_handler
    movs    r11,    r11
    moveq   pc,    lr /* nobody registered, return */

    /* count the fiq:  it doesn't cost too much cpu time */
    ldr     r10,    fiqcount
    add     r10,    r10,    #1
    str     r10,    fiqcount

    /* save all registers, build sp and branch to C */
    adr     r9,    regpool
    stmia   r9,    {r0 - r13, lr}
    adr     sp,    fiq_sp
    ldr     sp,    [sp]
    mov     lr,    pc
    mov     pc,    r11
    adr     r9,    regpool

    /*
     * On FIQ: R14_fiq = address of next instruction to be executed + 4,
     * ==> use SUBS PC, R14, $4
     */
    ldmia   r9,    {r0 - r13, lr}
    subs    pc, r14, #4     /* Restores CPSR from SPSR_fiq as weel.           */

    /* data words*/
    .globl fiqcount
fiqcount: .long 0
fiq_sp:     .long fiq_stack+1020

    /* The pointer */
    .globl fiq_handler
fiq_handler:
    .long 0

regpool:
    .space 80 /* 10 registers */

#ifndef _DIAB_TOOL
.pool
#endif

    .align 5
fiq_stack:
    .space 1024

/******************************************************************************/

/*
 * PC-relative-addressable pointers - LDR Rn,=sym is broken
 * note "_" after "$" to stop preprocessor preforming substitution
 */

	.balign	4

L$_vxSvcIntStackBase:
	.long	VAR(vxSvcIntStackBase)

L$_vxSvcIntStackEnd:
	.long	VAR(vxSvcIntStackEnd)

L$_vxIrqIntStackBase:
	.long	VAR(vxIrqIntStackBase)

L$_vxIrqIntStackEnd:
	.long	VAR(vxIrqIntStackEnd)

#if	(ARM_THUMB)
L$_usrInit:
	.long	FUNC(usrInit)
#endif	/* (ARM_THUMB) */

#if defined(CPU_720T) || defined(CPU_720T_T) || \
    defined(CPU_740T) || defined(CPU_740T_T)
L$_sysCacheSwapVar:
	.long   sysCacheSwapVar
#endif
#if defined(CPU_940T) || defined(CPU_940T_T)

L$_sysCacheUncachedAdrs:
	.long   SYS_CACHE_UNCACHED_ADRS
#endif
