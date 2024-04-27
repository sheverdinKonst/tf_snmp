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
#define FPEXC_EN_BIT       (1<<30) /* global enable bit */

#if defined(INCLUDE_I_AND_D_CACHE)
#define CRYPTO_SRAM_BASE           (0xFB000000)
#define DUP_CHUNK_SIZE             0x800
#define NON_CACHEABLE_DESC         (0x812)
#define CACHEABLE_DESC             (0x81E)
#endif

    .data
    .globl   VAR(copyright_wind_river)
    .globl  VAR(L$_rConfigDramInRom)
    .long    VAR(copyright_wind_river)

    /* internals */
    .globl  FUNC(romInit)		/* start of system code */
    .globl  FUNC(resetVector )		/* start of system code */
    .globl  FUNC(resetEntry  )		/* start of system code */
    .globl  VAR(sdata)		/* start of data */
    .globl  _sdata


    /* externals */

    .extern FUNC(romStart)	/* system initialization routine */
    .extern FUNC(_mvDramIfStaticInit)	/* DRAM initialization routine */
    .extern FUNC(_mvDramIfConfig)	/* DRAM initialization routine */

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
#if defined(MV_CPU_BE)
    /* convert CPU to big endian - if already uBoot is BE - andne/i
    commands - meaningless commands */
    .word 0x100f11ee /* mrc p15, 0, r0, c1, c0 */
    .word 0x800080e3 /* orr r0, r0, #0x80 */
    .word 0x100f01ee /* mcr p15, 0, r0, c1, c0 */
    nop;nop;nop;nop;
    nop;nop;nop;nop;
    nop;
#else
    /* convert CPU to little endian - if already uBoot is LE -
    andne/i commands - meaningless commands */
    .word 0x100f11ee /* mrc p15, 0, r0, c1, c0 */
    .word 0x8000c0e3 /* orr r0, r0, #0x80 */
    .word 0x100f01ee /* mcr p15, 0, r0, c1, c0 */

    nop;nop;nop;nop;
    nop;nop;nop;nop;
#endif

cold:
    MOV    r0, #BOOT_COLD	/* fall through to warm boot entry */
warm:
    B start
L$_rConfigDramInRom:
    .long ROM_TEXT_ADRS + FUNC(_mvDramIfConfig) - FUNC(romInit)

    /* copyright notice appears at beginning of ROM (in TEXT segment) */

    .ascii   "Copyright 1999-2001 ARM Limited"
    .ascii   "\nCopyright 1999-2001 Wind River Systems, Inc."
    .balign 4

start:
    MOV  r12,r0                    /* save boot type */
    MOV  sp, #0x0                  /* clear stack pointer*/
    MOV  r1, #MMU_INIT_VALUE       /* Defined in mmuArmLib.h */
    MCR  CP_MMU, 0, r1, c1, c0, 0  /* Write to MMU CR */

    /*
     * Code for running with no uBoot: change reg base to 0xf1000000
     */
    ldr    r4, =INTER_REGS_BASE
    MV_DV_REG_WRITE_ASM r4, r1, 0x20080

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

    /* ascertain chip type */
    ldr    r2, =0xF400004C
    ldr    r2, [r2]
    mov    r2, r2, lsr #10      /* >>= 10 */
    mov    r2, r2, lsr #4       /* >>= 4  */
    and    r2, r2, #63          /* &= 0x3F */
    ldr    r3, =_G_xCatChipType
    str    r2, [r3]

    /* Check is SPI WA is needed. */
    ldr    r3, =_G_xCatChipType
    ldr    r3, [r3]
    cmp    r3, #55              /* xCat stands for 0x37 (0x37 == #55) */
    bne    afterSpiWa

    /* It's xCat chip, but only if it is xCat-A1 revision, do SPI WA. */
    ldr    r3, =_G_xCatRevision
    ldr    r3, [r3]
    cmp    r3, #2               /* #2 stands for xCat-A1 chip revision ID */
    bne    afterSpiWa

    /*************************************************************************/
    /* Ref # RES-SPI-20 (Chip Select (SPI_CSn) Deselect Timing) */
    /* Load code to I-cache */
    adr    r4, _romInit /* r4 <- current position of code   */
    mvn    r5, #0xff
    and    r4, r4, r5

    /* Add for load code into I cache */
    /*
     * flush v4 I/D caches
     */
    mov    r0, #0
    mcr    p15, 0, r0, c7, c7, 0	/* flush v3/v4 cache */
    mcr    p15, 0, r0, c8, c7, 0	/* flush v4 TLB */

    /* Load source code from 0xfff80000-0xfff83fff  */
    mov    r8, r4      /* bootrom start code on flash */
    mov    r2, #0x2000 /* bootrom size of unzip code  */

    /*
     * disable MMU stuff and caches
     */
    mrc    p15, 0, r0, c1, c0, 0
    bic    r0, r0, #0x00000007	/* 2:0 (CAM) */
    orr    r0, r0, #0x00000002	/* set bit 2 (A) Align */
    orr    r0, r0, #0x00001000	/* Enabled I-cache */
    mcr    p15, 0, r0, c1, c0, 0
    nop; nop; nop; nop;

    /* Lock I-cache */
    mrc    p15, 0, r0, c9, c0, 1
    orr    r0, r0, #0xe
    mcr    p15, 0, r0, c9, c0, 1

    /* Load code into I Cache */
load_loop:
    mcr    p15, 0, r8, c7, c13, 1
    add    r8, r8, #32 	/* 8 dwords * 4 bytes */
    sub    r2, r2, #32 	/* 8 dwords * 4 bytes */
    cmp    r2, #0 /* check if we have read a full Page */
    bne    load_loop

    /*  start loop 2 */
    mov    r2, #0x2000	/* bootrom size of unzip code  */

    /* Lock I-cache */
    mrc    p15, 0, r0, c9, c0, 1
    orr    r0, r0, #0xd
    mcr    p15, 0, r0, c9, c0, 1

    /* Load code into I Cache */
load_loop2:
    mcr    p15, 0, r8, c7, c13, 1
    add    r8, r8, #32 	/* 8 dwords * 4 bytes */
    sub    r2, r2, #32 	/* 8 dwords * 4 bytes */
    cmp    r2, #0 /* check if we have read a full Page */
    bne    load_loop2

    /* Lock I-cache */
    mrc    p15, 0, r0, c9, c0, 1
    orr    r0, r0, #0x3
    mcr    p15, 0, r0, c9, c0, 1

    /* End of code load */

    ldr    r1, =(INTER_REGS_BASE + (AHB_TO_MBUS_WIN_INTEREG_REG))
    ldr    r4, =htoll(INTER_REGS_BASE)
    str    r4, [r1]

    ldr    r3, =_isSpiWaDone
    mov    r2, #1                   /* 1 = SPI WA was done */
    str    r2, [r3]
    /*
     * End of SPI WA.
     *************************************************************************/

afterSpiWa:
    /*
     * Configures L2 cache to WriteBack (WB) mode.
     * Note: WriteThrough (WT) is the default mode.
     */
    LDR     r2, =(INTER_REGS_BASE + CPU_L2_CONFIG_REG)
    ldr     r1, [r2]
    bic     r1, r1, #BIT4
    str     r1, [r2]

    /*
     * Configure L2 Cache to WriteBack (WB) mode with Prefetch enabled.
     */
    mrc    p15, 1, r0, c15, c1, 0
    orr    r0, r0, #0x400000        /* set bit 22 - enables L2 Cache */
    bic    r0, r0, #0x1000000       /* clear bit 24 - enables L2 Prefetch */
    mcr    p15, 1, r0, c15, c1, 0

    /***************************************************************************
     * Configure MMU, create Page Table in SRAM, enable ICache and DCache.
     */
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

    MOV     r1, #MMU_INIT_VALUE      /* Defined in mmuArmLib.h */

    MCR     CP_MMU, 0, r1, c1, c0, 0 /* Write to MMU CR */

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

#endif /* defined(CPU_720T,740T,920T,940T,946ES,1020E,1022E) */
_mmuDisable:
    /* Set CPU Control and Status Registers MMU disabled bit */
    /* according to selected memory managment method         */
    ldr     r1, =(INTER_REGS_BASE + CPU_CONFIG_REG)
    ldr     r4, [r1]
    bic     r4, r4, #htoll(CCR_NCB_BLOCKING_MASK)
    str     r4, [r1]

    /* disable interrupts in CPU and switch to SVC32 mode */
_disable_interrupts:
    MRS     r1, cpsr
    BIC     r1, r1, #MASK_MODE
    ORR     r1, r1, #MODE_SVC32 | I_BIT | F_BIT
    MSR     cpsr, r1

    /*
     * CPU INTERRUPTS DISABLED
     *
     * disable individual interrupts in the interrupt controller
     */
_disableCtrlInterrupts:
    MOV     r1, #0
    LDR     r2, =(INTER_REGS_BASE + CPU_MAIN_IRQ_MASK_HIGH_REG)
    STR     r1, [r2]        /* disable all main high cause sources */
    LDR     r2, =(INTER_REGS_BASE + CPU_MAIN_IRQ_MASK_REG)
    STR     r1, [r2]        /* disable all main low cause sources */

    /*
     * If SPI WA was done (code is locked in ICache),
     * don't early enable ICache and DCache.
     */
    ldr     r3, =_isSpiWaDone
    ldr     r3, [r3]
    cmp     r3, #1
    beq     afterEarlyIDcacheEnable

    /*
     * Enable MMU, ICACHE and DCACHE and build page table in SRAM.
     */
#if defined(INCLUDE_I_AND_D_CACHE)
    /* Change mode to supervisor and disable interrupts*/
    MRS     r5 , CPSR
    bic     r5,r5,#0x1f         /* Supervisor mode*/
    orr     r5,r5,#0xd3         /* Disable FIQ and IRQ*/
    MSR     CPSR_c , r5

    /* We start as 926 , so we will use 926 instructions first */
    /* Invalidate I/D caches and TLB */
    mov     r5, #0
    mcr     p15, 0, r5, c7, c7, 0       /* invalidate v3/v4 cache */
    mcr     p15, 0, r5, c7, c6, 0       /* invalidate dcache */
    mcr     p15, 0, r5, c7, c5, 0       /* invalidate icache */
    mcr     p15, 0, r5, c8, c7, 0       /* flush v4 TLB */

    /* Load page table base address to TTBR */
    ldr     r4, =0x3
    mcr     p15, 0, r4, c3, c0, 0	/* load domain access register*/
    ldr     r4, =CRYPTO_SRAM_BASE
    mcr     p15, 0, r4, c2, c0, 0	/* load TTBR*/
    /* Set the Translation Table in the SRAM	*/
    ldr     r3, =0x100000		/* Increment for section	*/

    /* First 272MB => 272 descriptor are non-cacheable for DDR	*/
    ldr     r2, =0x0		/* Starting Phys Address */
    ldr     r1, =0x110		/* 272 Descriptors for DDR*/
    ldr     r0, =CACHEABLE_DESC	/* descriptor flags*/

ddr_loop:
    orr     r5, r2, r0		/* build descriptor */
    str     r5, [r4]
    add     r4, r4, #4		/* increment SRAM address */
    add     r2, r2, r3		/* increment phys address */
    sub     r1, r1, #1
    cmp     r1, #0
    bne     ddr_loop

    /* 1MB => 1 descriptor non-cacheable for Internal Regs*/
    ldr     r2, =0xF1000000		/* Starting Phys Address*/
    ldr     r0, =NON_CACHEABLE_DESC	/* descriptor flags*/
    orr     r5, r2, r0		/* build descriptor*/
    str     r5, [r4]
    add     r4, r4, #4		/* increment SRAM address*/

    ldr     r1, =0x6f		/* clear 0x6f descriptors */
zero_loop1:
    ldr     r5, =0		/* build descriptor */
    str     r5, [r4]
    add     r4, r4, #4		/* increment SRAM address */
    sub     r1, r1, #1
    cmp     r1, #0
    bne     zero_loop1

    /* 16MB => 16 descriptors non-cacheable for SPI space */
    ldr     r2, =0xF8000000		/* Starting Phys Address */
    ldr     r1, =0x10		/* 16 Descriptor for SPI */
    ldr     r0, =NON_CACHEABLE_DESC	/* descriptor flags */

spi_loop:
    orr     r5, r2, r0		/* build descriptor*/
    str     r5, [r4]
    add     r4, r4, #4		/* increment SRAM address*/
    add     r2, r2, r3		/* increment phys address*/
    sub     r1, r1, #1
    cmp     r1, #0
    bne     spi_loop

    /* To Do : Fill rest of page table (0x668 - 0x7fc) with 0 */
    /* last SPI address is 0x63c => (0x640 - 0x7fc) */
    ldr     r1, =0x6f		/* clear 0x6f descriptors */
zero_loop2:
    ldr     r5, =0		/* build descriptor */
    str     r5, [r4]
    add     r4, r4, #4		/* increment SRAM address */
    sub     r1, r1, #1
    cmp     r1, #0
    bne     zero_loop2

    /* 1MB => 1 descriptor cacheable for BootROM, Stack and ECC tables*/
    ldr     r2, =0xFFF00000		/* Starting Phys Address*/
    ldr     r0, =CACHEABLE_DESC	/* descriptor flags*/
    orr     r5, r2, r0		/* build descriptor*/
    str     r5, [r4]

    /* ==========================================================
     * KW uses 2KB SRAM for the Page Table using the overlap mechanism
     * BZ has 32KB SRAM so the 2K Page Table needs to be duplicated.
     * So it will be the size of 16KB, since the the overlap mechanism
     * cannot be used.
     */

#if 1
    ldr r3, =0x7                /* Number of duplications */
    ldr r0, =(CRYPTO_SRAM_BASE + (DUP_CHUNK_SIZE)) /* Destination */

duplicate_loop:
    ldr r1, =CRYPTO_SRAM_BASE   /* Source */
    ldr r2, =DUP_CHUNK_SIZE     /* Size   */

cpy_loop:
    ldmia   r1!, {r5-r12}
    stmia   r0!, {r5-r12}
    sub     r2, r2, #0x20
    cmp     r2, #0x0
    bne     cpy_loop

    /*
     * No need to increment r0 (chunk address) since it's being
     * incremented by stmia instruction so it's already pointing
     * to the next chunk
     */

    sub     r3, r3, #1
    cmp     r3, #0
    bne     duplicate_loop

    /* ========================================================== */
#endif

    /* Enable MMU & D-Cache*/
    ldr     r3, =0x5		/* MMU[0], $D[2] enabled*/
    mrc     p15, 0, r4, c1, c0, 0	/* Read Control  Register*/
    orr     r4, r4, r3		/* Enable MMU, Data & Instruction Cache*/
    mcr     p15, 0, r4, c1, c0, 0	/* Write Control  Register*/
    nop
    nop

    /* Enable I-Cache*/
    mrc     p15, 0, r1, c1, c0, 0	/* read control reg*/
    orr     r1, r1, #0x00001000	/* set bit 12 (I) I-Cache */
    mcr     p15, 0, r1, c1, c0, 0	/* write to Control register.*/
#endif /* INCLUDE_I_AND_D_CACHE */

afterEarlyIDcacheEnable:

    /*
     * Initialize the stack pointer to just before where the
     * uncompress code, copied from ROM to RAM, will run.
     */
    ldr    sp, =0x0
    cmp    r12, #BOOT_COLD
    bne    DramDetectEnd

    /*
     * Code for running with no uBoot: change reg base to 0xf1000000
     */
    ldr    r4, =INTER_REGS_BASE
    MV_DV_REG_WRITE_ASM r4, r1, 0x20080

#if defined(MV_STATIC_DRAM_ON_BOARD)
    /* code for running with no uBoot */

    /* set DRAM registers */
    ldr    r4, =0x1b1b1b1b
    MV_REG_WRITE_ASM(r4, r1, 0x100e0)
    ldr    r4, =0xffffffff
    /* Ref # RM 30250 ( Incorrect default value for timing parameters in L2 cache RAMs) */
    MV_REG_WRITE_ASM(r4, r1, 0x20134)
    ldr    r4, =0x66666666
    MV_REG_WRITE_ASM(r4, r1, 0x20138)
    ldr    r4, =0x66666666
    MV_REG_WRITE_ASM(r4, r1, 0x20154)
    ldr    r4, =0x00000000
    MV_REG_WRITE_ASM(r4, r1, 0x2014C)
    ldr    r4, =0x00000001
    MV_REG_WRITE_ASM(r4, r1, 0x20148)
    bl     _mvDramIfStaticInit
#endif

    ldr    r5, =(LOCAL_MEM_LOCAL_ADRS+BOOT_LINE_OFFSET)
    ldr    r0, =0
    STR    r0,[r5]
DramDetectEnd:

    LDR    sp, L$_STACK_ADDR
    mov    r0,r12           /* restore r0 = BOOT type */

    MOV    fp, #0           /* zero frame pointer */

    /* jump to C entry point in ROM: routine - entry point + ROM base */

#if (ARM_THUMB)
    LDR    r12, L$_rStrtInRom
    ORR    r12, r12, #1     /* force Thumb state */
    BX     r12
#else
    adr    r4, _romInit
    ldr    r5, _startRom
    add    r4, r4, r5
    mov    lr, r4
    mov    pc, lr

    _startRom:
    .long FUNC(romStart) - FUNC(romInit)

#endif /* (ARM_THUMB) */


    .balign 4

.global _romInitAddr
_romInitAddr:
    mov     r9, LR     /* Save link register */
    adr     r0, _romInit
    mov     PC, r9         /* r9 is saved link register */

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

#if defined(MV_CPU_BE)
    ldr    r4, =0xC100FF03      /* value of  window control register */
#else
    ldr    r4, =0x03FF00C1      /* value of  window control register */
#endif
    cmp    r3, r4               /* value of  window control register */
    beq    mem_win_cfg_check_base_reg
    add    r2, r2, #0x10        /* step to the next window control reg */
    bne    mem_win_cfg_loop     /* window control register is not configured */

mem_win_cfg_check_base_reg:
    /* window control register is ok, check  window base register now */
    add    r2, r2, #0x4         /* points to window base register */
    ldr    r3, [r2]             /* value of  window base register */
#if defined(MV_CPU_BE)
    cmp    r3, #0x000000F4      /* value of  window base register */
#else
    cmp    r3, #0xF4000000      /* value of  window base register */
#endif
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
#if defined(MV_CPU_BE)
    ldr    r4, =0xC100FF03      /* value of  window control register */
#else
    ldr    r4, =0x03FF00C1      /* value of  window control register */
#endif
    str    r4, [r2]             /* configure window control register */

    /* configured window base register */
    ldr    r2, =INTER_REGS_BASE
    ldr    r3, =0x20064
    add    r2, r2, r3           /* points to window base register */
#if defined(MV_CPU_BE)
    ldr    r4, =0x000000F4      /* value of  window base register */
#else
    ldr    r4, =0xF4000000      /* value of  window base register */
#endif
    str    r4, [r2]             /* configure window base register */

mem_win_cfg_window_found:
    /* continue regular execution */

    ldmia   sp!, {r3}
    mov     pc, lr

.global _G_xCatRevision
_G_xCatRevision:
    .long   1 /* default xcat revision */

.global _G_xCatIsFEBoardType
_G_xCatIsFEBoardType:
    .long   0 /* default is GE */

.global _G_xCatChipType
_G_xCatChipType:
    .long   0

/* 1 = done, 0 = was not done. */
.global _isSpiWaDone
_isSpiWaDone:
    .long   0

/******************************************************************************/

/*
 * PC-relative-addressable pointers - LDR Rn,=sym is broken
 * note "_" after "$" to stop preprocessor performing substitution
 */

    .balign 4

L$_rStrtInRom:
    .long ROM_TEXT_ADRS + FUNC(romStart) - FUNC(romInit)

L$_STACK_ADDR:
    .long STACK_ADRS

