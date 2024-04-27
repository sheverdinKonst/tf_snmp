/* sysLib.c - ARM Integrator system-dependent routines */

/* Copyright 1999-2003 ARM Limited */
/* Copyright 1999-2004 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01x,26apr04,dmh  add 926EJ support
01w,25feb03,jb   Enabling access to Private SDRAM
01v,04feb03,jb   Adding ARM10 Support
01r,21jan03,jb   Resolving SPR 81285
01q,28may02,m_h  windML not UGL
01p,31oct01,rec  use generic driver for amba timer
01o,09oct01,m_h  configure keyboard if windML is configured
01n,09oct01,jpd  correct sysPhysMemDesc entres for 946es.
01m,03oct01,jpd  tidied slightly
01l,28sep01,pr   added support for ARM946E.
01k,12sep01,m_h  WindML support
01j,27aug01,jb   Adding USB support
01i,21feb01,h_k  added support for ARM966ES and ARM966ES_T.
01h,01dec00,rec  fix typo in INCLUDE_FEI82557END
01g,20nov00,jpd  added support for Intel Ethernet driver.
01f,14jun00,pr   fixed Flash enable/disable with recent versions of FPGA.
01e,18feb00,jpd  minor tidying. Added Core Module Header sysPhysMemDesc entry.
01d,07feb00,jpd  added support for ARM720T, ARM920T.
01c,13jan00,pr	 added support for ARM740T.
01b,07dec99,pr	 added support for PCI.
01a,15nov99,ajb  copied from pid7t version 01o.
*/

/*
DESCRIPTION
This library provides board-specific routines for the ARM Integrator
Development Board BSP.


SEE ALSO:
.pG "Configuration"
.I "ARM Architecture Reference Manual,"
.I "ARM 7TDMI Data Sheet,"
.I "ARM 720T Data Sheet,"
.I "ARM 740T Data Sheet,"
.I "ARM 920T Technical Reference Manual",
.I "ARM 926EJ-S Technical Reference Manual",
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
.I "ARM Integrator/CM926EJ-S, CM946E-S, CM966E-S, User Guide",
.I "ARM Integrator/CM940T User Guide",
.I "ARM Integrator/CM946E User Guide",
.I "ARM Integrator/CM9x6ES Datasheet".
.I "ARM Integrator/CM10200 User Guide",
*/

/* includes */

#include "vxWorks.h"
#include "config.h"

#include "sysLib.h"
#include "string.h"
#include "intLib.h"
#include "taskLib.h"
#include "vxLib.h"
#include "muxLib.h"
#include "cacheLib.h"
#include "cpu/mvCpu.h"
#include "symbol.h"

#include "mvBoardEnvLib.h"
#include "mvGpp.h"
#include "mvPrestera.h"

#if !defined(BOOTROM) && !defined (VIPS_APP)
#include "pssBspApis.h"
#endif

#if defined(CPU_720T)  || defined(CPU_720T_T) || \
    defined(CPU_740T)  || defined(CPU_740T_T) || \
    defined(CPU_920T)  || defined(CPU_920T_T) || \
    defined(CPU_926EJ)  || defined(CPU_926EJ_T) || \
    defined(CPU_940T)  || defined(CPU_940T_T) || \
    defined(CPU_946ES) || defined(CPU_946ES_T) || \
    defined(CPU_1020E) || defined(CPU_1022E)
#include "arch/arm/mmuArmLib.h"

#if   defined(CPU_926EJ) || defined(CPU_926EJ_T)
#if (_WRS_VXWORKS_MAJOR == 6) && (_WRS_VXWORKS_MINOR > 2)
IMPORT void	cacheArmFeroceonLibInstall (VIRT_ADDR(physToVirt) (PHYS_ADDR),
                                        PHYS_ADDR(virtToPhys) (VIRT_ADDR));
#endif
#endif
#include "private/vmLibP.h"
#include "dllLib.h"
#endif /* defined(720T/740T/920T/940T/946ES/1020E/1022E) */

#include "mvDramIf.h"
/* #include "vxAhb2MbusIntCtrl.h" */
#include "mvCpuIf.h"
#include "mvCtrlEnvSpec.h"
#include "mvCntmrRegs.h"
#include "mvCntmr.h"
#include "device/mvDevice.h"

#ifdef INCLUDE_PCI
#undef  PCI_AUTO_DEBUG
#include "drv/pci/pciIntLib.h"
#include "drv/pci/pciConfigLib.h"
#include "drv/pci/pciAutoConfigLib.h"
#include "drv/pci/pciConfigShow.h"

#include "pci/pciConfigLib.c"
#include "pci/pciAutoConfigLib.c"
#include "pci/pciConfigShow.c"       /* provides PCI_AUTO_DEBUG_MSG macro */
#include "pci/pciIntLib.c"

#include "sysBusPci.c"

#endif /* #ifdef INCLUDE_PCI */

#ifdef MV_INCLUDE_SPI
    #include "sflash/mvSFlash.h"
    MV_SFLASH_INFO G_spiFlashInfo;
#endif

#ifdef MV_INCLUDE_NAND
    #include "sysNflash.c"
    MV_NFLASH_INFO *G_nandFlashInfoP;
#endif

/* imports */
MV_STATUS mvSwitchGenBootCfg();

extern void usrBootLineInit (int startType);
extern void          *hsuBase;
extern unsigned long  hsuLen;

IMPORT char end [];    /* end of system, created by ld */
IMPORT VOIDFUNCPTR _func_armIntStackSplit;  /* ptr to fn to split stack */

IMPORT int  ambaIntDevInit (void);
IMPORT void sysIntStackSplit (char *, long);

IMPORT void      mvDebugInit(void);
IMPORT MV_STATUS mvCtrlInit(MV_VOID);
IMPORT MV_STATUS mvCtrlInit2(MV_VOID);

IMPORT STATUS sys543PciInit (void);
IMPORT void   sys557PciInit (void);
IMPORT STATUS sysSkGePciInit (void);
IMPORT UINT32 sysTimeBaseLGet (void);

IMPORT int	consoleFd;		/* fd of initial console device */
IMPORT int  sysStartType;	/* BOOT_CLEAR, BOOT_NO_AUTOBOOT, ... */

IMPORT MV_U32 gBoardId;     /* 88F6XXX board ID global variable*/

IMPORT END_TBL_ENTRY    endDevTbl[];    /* end device table */
IMPORT END_OBJ* mgiEndLoad(char *, void*);

IMPORT MV_U32 PRESTERA_DEV_ID_REG_OFFSET_extern;
IMPORT MV_U32 XCAT_INTERNAL_PP_BASE_ADDR_extern;

IMPORT MV_U32 bspReadRegisterInternal(MV_U32 address);
void sysOpenMemWinToSwitch(void);
void mvSetNonCacheable(void *addr, int len);

#if !defined(INCLUDE_MMU) && \
    (defined(INCLUDE_CACHE_SUPPORT) || defined(INCLUDE_MMU_BASIC) || \
     defined(INCLUDE_MMU_FULL) || defined(INCLUDE_MMU_MPU))
#define INCLUDE_MMU
#endif

#ifdef MV_MMU_DISABLE
#undef INCLUDE_MMU_BASIC
#undef INCLUDE_MMU_FULL
#undef INCLUDE_MMU_MPU
#undef INCLUDE_MMU
#endif

#if defined(INCLUDE_CACHE_SUPPORT)
#if defined(CPU_7TDMI) || defined(CPU_7TDMI_T) || \
    defined(CPU_966ES) || defined(CPU_966ES_T)
       FUNCPTR sysCacheLibInit = NULL;
#endif /* defined(CPU_7TDMI/7TDMI_T) */

#if	defined(CPU_940T) || defined(CPU_940T_T) || \
        defined(CPU_926EJ) || defined(CPU_926EJ_T)
UINT32 * sysCacheUncachedAdrs = (UINT32 *)SYS_CACHE_UNCACHED_ADRS;
#endif /* defined(CPU_940T/940T_T) */
#endif /* defined(INCLUDE_CACHE_SUPPORT) */


/* globals */
MV_U32 G_mvIsPexInited = 0;
BOOT_PARAMS bootParams;
UINT32 sysClockRate;				/* System clock variable				*/
UINT32 sdramSize = LOCAL_MEM_SIZE;  /* SDRAM auto configurating result  for sysPhysMemTop */

#if defined(INCLUDE_MMU)
#if (_WRS_VXWORKS_MAJOR == 6) && (_WRS_VXWORKS_MINOR > 2)
#else
#ifndef _TYPE_VIRT_ADDR
#define _TYPE_VIRT_ADDR typedef UINT32 *  VIRT_ADDR
#endif
_TYPE_VIRT_ADDR;
#ifndef _TYPE_PHYS_ADDR
#define _TYPE_PHYS_ADDR typedef UINT32 * PHYS_ADDR
#endif
_TYPE_PHYS_ADDR;
#endif
#if defined(CPU_720T)  || defined(CPU_720T_T)  || \
    defined(CPU_740T)  || defined(CPU_740T_T)  || \
    defined(CPU_920T)  || defined(CPU_920T_T)  || \
    defined(CPU_926EJ) || defined(CPU_926EJ_T) || \
    defined(CPU_940T)  || defined(CPU_940T_T)  || \
    defined(CPU_946ES) || defined(CPU_946ES_T) || \
    defined(CPU_1020E) || defined(CPU_1022E)

#if defined(CPU_720T)  || defined(CPU_720T_T)  || \
    defined(CPU_920T)  || defined(CPU_920T_T)  || \
    defined(CPU_926EJ) || defined(CPU_926EJ_T) || \
    defined(CPU_1020E) || defined(CPU_1022E)

/*
 * The following structure describes the various different parts of the
 * memory map to be used only during initialisation by
 * vm(Base)GlobalMapInit() when INCLUDE_MMU_BASIC/FULL are
 * defined.
 *
 * Clearly, this structure is only needed if the CPU has an MMU!
 *
 * The following are not the smallest areas that could be allocated for a
 * working system. If the amount of memory used by the page tables is
 * critical, they could be reduced.
 */

PHYS_MEM_DESC sysPhysMemDesc [] =
{
    /* DRAM must always be the first entry */
    /* adrs and length parameters must be page-aligned (multiples of 0x1000) */
    /* DRAM - Always the first entry */
    {
    (VIRT_ADDR)  LOCAL_MEM_LOCAL_ADRS,	/* virtual address */
    (PHYS_ADDR ) LOCAL_MEM_LOCAL_ADRS,	/* physical address */
    ROUND_UP (LOCAL_MEM_SIZE, PAGE_SIZE), /* length, then initial state: */
    VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
    VM_STATE_VALID	| VM_STATE_WRITABLE	 | VM_STATE_CACHEABLE
    },

#ifdef MV_INCLUDE_SPI
    {
    (VIRT_ADDR) DEVICE_SPI_BASE,  				/* SPI Flash */
    (PHYS_ADDR) DEVICE_SPI_BASE,
    ROUND_UP (DEVICE_SPI_SIZE, PAGE_SIZE),
    VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
    VM_STATE_VALID	    | VM_STATE_WRITABLE	     | VM_STATE_CACHEABLE_NOT
    },
#endif

    {
    (VIRT_ADDR) PEX0_MEM_BASE,		/* PCI Express Memory space */
    (PHYS_ADDR) PEX0_MEM_BASE,
    ROUND_UP (PEX0_MEM_SIZE, PAGE_SIZE),
    VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
    VM_STATE_VALID	    | VM_STATE_WRITABLE	     | VM_STATE_CACHEABLE_NOT
    },

    {
    (VIRT_ADDR) PEX0_IO_BASE,		/* PCI Express IO space */
    (PHYS_ADDR) PEX0_IO_BASE,
    ROUND_UP (PEX0_IO_SIZE, PAGE_SIZE),
    VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
    VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT
    },

    {
    (VIRT_ADDR)  INTER_REGS_BASE,	/* Marvell Controller internal register */
    (PHYS_ADDR)  INTER_REGS_BASE,
    ROUND_UP (INTER_REGS_SIZE, PAGE_SIZE),
    VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
    VM_STATE_VALID	    | VM_STATE_WRITABLE	     | VM_STATE_CACHEABLE_NOT
    },

    {
    (VIRT_ADDR)  0xf2000000,        /* DTCM memory of Dragonite */
    (PHYS_ADDR)  0xf2000000,
    ROUND_UP (INTER_REGS_SIZE, PAGE_SIZE),
    VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
    VM_STATE_VALID	    | VM_STATE_WRITABLE	     | VM_STATE_CACHEABLE_NOT
    },

    {
    (VIRT_ADDR)  NFLASH_CS_BASE,				/* Large Flash */
    (PHYS_ADDR)  NFLASH_CS_BASE,
    ROUND_UP (NFLASH_CS_SIZE, PAGE_SIZE),
    VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
    VM_STATE_VALID	    | VM_STATE_WRITABLE	     | VM_STATE_CACHEABLE_NOT
    },

#ifdef MV_INCLUDE_CESA
    {
    (VIRT_ADDR) CRYPT_ENG_BASE,
    (PHYS_ADDR) CRYPT_ENG_BASE,
    CRYPT_ENG_SIZE,
    VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
    VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT
    },
#endif

#ifdef MV_INCLUDE_SAGE
    {
    (VIRT_ADDR) SAGE_UNIT_BASE,
    (PHYS_ADDR) SAGE_UNIT_BASE,
    SAGE_UNIT_SIZE,
    VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
    VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT
    }
#endif
};
#endif /* defined(CPU_720T/720T_T/920T/920T_T/926EJ/926EJ_T/1020E/1022E) */

void mvPrintsysPhysMemDesc(void);

int sysPhysMemDescNumEnt = NELEMENTS (sysPhysMemDesc);
#endif /* defined(CPU_720T/740T/920T/940T/946ES/1020E/1022E) */

#else

PHYS_MEM_DESC sysPhysMemDesc [] =
{
    /* DRAM must always be the first entry */
    /* adrs and length parameters must be page-aligned (multiples of 0x1000) */
    /* DRAM - Always the first entry */
    {
        LOCAL_MEM_LOCAL_ADRS,	/* virtual address */
        LOCAL_MEM_LOCAL_ADRS,	/* physical address */
    ROUND_UP (LOCAL_MEM_SIZE, PAGE_SIZE), /* length, then initial state: */
    VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
    VM_STATE_VALID	| VM_STATE_WRITABLE	 | VM_STATE_CACHEABLE
    }
};

int sysPhysMemDescNumEnt = NELEMENTS (sysPhysMemDesc);

#endif /* defined(INCLUDE_MMU) */

int	sysBus	    = BUS;		/* system bus type (VME_BUS, etc) */
int	sysCpu	    = CPU;		/* system CPU type (e.g. ARMARCH4/4_T)*/
char *	sysBootLine = BOOT_LINE_ADRS; 	/* address of boot line */
char *	sysExcMsg   = EXC_MSG_ADRS;	/* catastrophic message area */
int	sysProcNum;			/* processor number of this CPU */
int	sysFlags;			/* boot flags */
char	sysBootHost [BOOT_FIELD_LEN];	/* name of host from which we booted */
char	sysBootFile [BOOT_FIELD_LEN];	/* name of file from which we booted */

char sysModelStr [80] = "";				/* Name of system */

/* Timer delay globals */
static UINT32 timestamp;
static UINT32 lastdec;
static UINT32 maxClkTicks;

#ifdef AMBA_INT_PRIORITY_MAP
/*
 * List of interrupts to be serviced in order of decreasing priority.
 * Interrupts not in this list will be serviced least-significant bit
 * first at a lower priority than those in the list.
 *
 * To use lowest-bit = highest-priority, reverse the sense of the
 * condition below so that ambaIntLvlPriMap is a zero pointer.
 */

LOCAL int ambaIntLvlPriMap[] =
{
    INT_LVL_UART0,              /* console port */
    INT_LVL_UART1,              /* second serial port */
#ifdef INCLUDE_PCI
    INT_LVL_PEX00_ABCD,         /* PCI device 0: */
#endif /* INCLUDE_PCI */
    -1                          /* list terminator */
};

#else
/*
 * This array maps interrupt levels to mask patterns.  The interrupt level
 * is the index, the data is the mask value.  A mask bit enables one
 * level.  The mask value is 'and'd with the ambaIntLvlEnabled value
 * before writing to the chip.
 */
LOCAL UINT32 ambaIntLvlMask[AMBA_INT_NUM_LEVELS + 1] = /* int level mask */
{
    0x00000000, /* level 0, all disabled */
    0x00000001, 0x00000003, 0x00000007, 0x0000000f,
    0x0000001f, 0x0000003f, 0x0000007f, 0x000000ff,
    0x000001ff, 0x000003ff, 0x000007ff, 0x00000fff,
    0x00001fff, 0x00003fff, 0x00007fff, 0x0000ffff,
    0x0001ffff, 0x0003ffff, 0x0007ffff, 0x000fffff,
    0x001fffff, 0x003fffff, 0x007fffff, 0x00ffffff,
    0x01ffffff, 0x03ffffff, 0x07ffffff, 0x0fffffff,
    0x1fffffff, 0x3fffffff, 0x7fffffff, 0xffffffff,
    0x00000001, 0x00000003, 0x00000007, 0x0000000f,
    0x0000001f, 0x0000003f, 0x0000007f, 0x000000ff,
    0x000001ff, 0x000003ff, 0x000007ff, 0x00000fff,
    0x00001fff, 0x00003fff, 0x00007fff, 0x0000ffff,
    0x0001ffff, 0x0003ffff, 0x0007ffff, 0x000fffff,
    0x001fffff, 0x003fffff, 0x007fffff, 0x00ffffff,
    0x01ffffff, 0x03ffffff, 0x07ffffff, 0x0fffffff,
    0x1fffffff, 0x3fffffff, 0x7fffffff, 0xffffffff,
};

    #if AMBA_INT_NUM_LEVELS != 64
    #error ambaIntLvlMask is wrong size for number of levels
    #endif
#endif /* ifdef AMBA_INT_PRIORITY_MAP */
/* defines */

/* externals */

/* globals */

/* forward LOCAL functions declarations */

/* forward declarations */

char *sysPhysMemTop    (void);
int   sysLogPoll(UCHAR* s,UINT a,UINT b,UINT c,UINT d,UINT e,UINT f);
void  sysPrintPollInit (void); /* Initialize the UART in poll mode */
void  sysPrintfPoll    (char *pString, int nchars, int  outarg);
void  sysCacheConfig   (void);
void flashBootSectorInit(void);

#ifdef INCLUDE_NETWORK
#   ifdef INCLUDE_GEI_END
#      include "./sysGei82543End.c"
#   endif

#   ifdef INCLUDE_FEI_END
#      include "./sysFei82557End.c"
#   endif
#endif /* INCLUDE_NETWORK */

/* included source files */
#include "sysSerial.c"

#include "mv88fxx81IntCtrl.c"

#include "mv88fxx81Timer.c"

#include "mvSysHwConfig.h"

extern MV_U32 _G_xCatIsFEBoardType;
extern MV_U32 _G_xCatRevision;

/*******************************************************************************
* mvSwitchSetChipType - sets FE/GE chip type by reading deviceId register
*
* DESCRIPTION:
*   If bit 5 of deviceId resister if 1, then the Prestera chip is FE.
*   Otherwise, it is GE.
*   4 LSB bits of deviceId register designate chip revision:
*       if >= 3 then xCat-A2 chip revision
*       if <3   then xCat-A1 chip revision
*
* RETURN:
*   MV_OK   - configuration succeeded.
*   MV_FAIL - configuration failed.
*
*******************************************************************************/
MV_STATUS mvSwitchSetChipType()
{
    MV_U32 devIdReg = 0;

    /* Read deviceId register */
    devIdReg = *(MV_U32*)(PRESTERA_DEV0_BASE + 0x4C);
    devIdReg = MV_32BIT_LE(devIdReg);

    _G_xCatIsFEBoardType   = devIdReg;
    _G_xCatIsFEBoardType  &= 0x10;
    _G_xCatIsFEBoardType >>= 4;

    _G_xCatRevision      = (devIdReg & 0xF);

    return MV_OK;
}

/*******************************************************************************
*
* sysModel - return the model name of the CPU board
*
* This routine returns the model name of the CPU board.
*
* NOTE
* This routine does not include all of the possible variants, and the
* inclusion of a variant in here does not mean that it is supported.
*
* RETURNS: A pointer to a string identifying the board and CPU.
*/

char *sysModel(void)
{
    mvBoardNameGet(sysModelStr);

    /* if (MV_REG_READ(CPU_CONFIG_REG) & CCR_ENDIAN_INIT_BIG) */
    if (mvCpuIsLE() == MV_FALSE)
    {
        strcat (sysModelStr, " BE");
    }
    else
    {
        strcat (sysModelStr, " LE");
    }

    strcat (sysModelStr, " MMU ARCH 5");

#ifdef MV_VFP
    strcat (sysModelStr, " VFP");
#endif

    return sysModelStr;
}

/*******************************************************************************
*
* sysBspRev - return the BSP version with the revision eg 1.2/<x>
*
* This function returns a pointer to a BSP version with the revision.
* e.g. 1.2/<x>. BSP_REV is concatenated to BSP_VERSION to form the
* BSP identification string.
*
* RETURNS: A pointer to the BSP version/revision string.
*/
char *sysBspRev (void)
{
    return BSP_VERSION BSP_REV;
}

#if defined(CPU_720T)  || defined(CPU_720T_T) || \
    defined(CPU_740T)  || defined(CPU_740T_T) || \
    defined(CPU_920T)  || defined(CPU_920T_T) || \
    defined(CPU_926EJ) || defined(CPU_926EJ_T)|| \
    defined(CPU_940T)  || defined(CPU_940T_T) || \
    defined(CPU_946ES) || defined(CPU_946ES_T)|| \
    defined(CPU_1020E) || defined(CPU_1022E)

/*******************************************************************************
*
* sysHwInit0 - perform early BSP-specific initialisation
*
* This routine performs such BSP-specific initialisation as is necessary before
* the architecture-independent cacheLibInit can be called. It is called
* from usrInit() before cacheLibInit(), before sysHwInit() and before BSS
* has been cleared.
*
* RETURNS: N/A
*/

#ifdef _DIAB_TOOL
/* use macros */

asm void  InvUnlockL1ICacheDiab(volatile unsigned int temp)
{
% reg temp;

  mcr    p15, 1, temp, c7, c5, 0;
  nop ;  nop ;
  mcr    p15, 1, temp, c9, c0, 1;
  nop ;  nop ;
}

asm volatile MV_U32 readMarvellEFRegDiab(void)
{
  mrc	p15, 1, r0, c15, c1, 0; /* Read Marvell extra features register */
/* return in r0 */
}

#endif

void sysHwInit0 (void)
{
    MV_CNTMR_CTRL cntmrCtrl;

    /*
     * unlock cache for xCat-A1 only, for xCat-A2 it is already unlocked;
     * see romStart();
     */
    if (mvCtrlModelGet() == MV_6281_DEV_ID)
    {
        volatile unsigned int temp = 0;
        /* Invalidate and Unlock CPU L1 I-cache*/
#ifdef _DIAB_TOOL
        InvUnlockL1ICacheDiab(temp);
#else
        __asm volatile ("mcr    p15, 1, %0, c7, c5, 0" : "=r" (temp));
        __asm volatile ("nop");
        __asm volatile ("nop");
        __asm volatile ("mcr    p15, 1, %0, c9, c0, 1" : "=r" (temp));
        __asm volatile ("nop");
        __asm volatile ("nop");
#endif
    }

    cntmrCtrl.enable  = MV_TRUE;
    cntmrCtrl.autoEnable = MV_TRUE; /* Work in auto mode - Periodic */

    /* In this phase, the system timer was not initialized yet. */
    /* start the counter to enable usage of system delay routine */
    /* The OS timer will reinitialize the timer later */
    mvCntmrStart(SYS_TIMER_NUM, TVR_ARM_TIMER_MAX, &cntmrCtrl);

#ifdef INCLUDE_CACHE_SUPPORT
    /*
     * Install the appropriate cache library, no address translation
     * routines are required for this BSP, as the default memory map has
     * virtual and physical addresses the same.
     */

#	if	defined(CPU_720T) || defined(CPU_720T_T)
           cacheArm720tLibInstall (NULL, NULL);
#	elif   defined(CPU_740T) || defined(CPU_740T_T)
           cacheArm740tLibInstall (NULL, NULL);
#	elif   defined(CPU_920T) || defined(CPU_920T_T)
           cacheArm920tLibInstall (NULL, NULL);
#	elif   defined(CPU_926EJ) || defined(CPU_926EJ_T)

#if (_WRS_VXWORKS_MAJOR == 6) && (_WRS_VXWORKS_MINOR > 2)
     cacheArmFeroceonLibInstall (NULL, NULL);
#else
     cacheArm926eLibInstall (NULL, NULL);
#endif /* (_WRS_VXWORKS_MAJOR == 6) && (_WRS_VXWORKS_MINOR > 2)  */

#	elif   defined(CPU_940T) || defined(CPU_940T_T)
           cacheArm940tLibInstall (NULL, NULL);
#	elif   defined(CPU_946ES) || defined(CPU_946ES_T)
           cacheArm946eLibInstall (NULL, NULL);
#	elif   defined(CPU_1020E)
           cacheArm1020eLibInstall (NULL, NULL);
#	elif   defined(CPU_1022E)
           cacheArm1022eLibInstall (NULL, NULL);
#	endif

#endif /* INCLUDE_CACHE_SUPPORT */


#if defined(INCLUDE_MMU)


/* Install the appropriate MMU library and translation routines */
#	if	defined(CPU_720T) || defined(CPU_720T_T)
           mmuArm720tLibInstall (NULL, NULL);
#	elif   defined(CPU_740T) || defined(CPU_740T_T)
           mmuArm740tLibInstall (NULL, NULL);
#	elif   defined(CPU_920T) || defined(CPU_920T_T)
           mmuArm920tLibInstall (NULL, NULL);
#	elif   defined(CPU_926EJ) || defined(CPU_926EJ_T)
           mmuArm926eLibInstall (NULL, NULL);
#	elif   defined(CPU_940T) || defined(CPU_940T_T)
           mmuArm940tLibInstall (NULL, NULL);
#	elif   defined(CPU_946ES) || defined(CPU_946ES_T)
           mmuArm946eLibInstall (NULL, NULL);
#	elif   defined(CPU_1020E)
           mmuArm1020eLibInstall (NULL, NULL);
#	elif   defined(CPU_1022E)
           mmuArm1022eLibInstall (NULL, NULL);
#	endif

#endif /* defined(INCLUDE_MMU) */

    return;
}
#endif /* defined(720T/740T/920T/926EJ/940T/946ES) */

/*******************************************************************************
* mvSysFlashInitNand
*
* DESCRIPTION:
*       Initializes NAND flash.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_TRUE     - success.
*       MV_FALSE    - failure.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_STATUS mvSysFlashInitNand(void)
{
#if defined(MV_INCLUDE_NAND)
    MV_U32          flashSize = 0;

    if (sysNflashHwInit() != MV_OK)
    {
        mvOsPrintf("%s sysNflashHwInit failed.\n", __func__);
        return MV_FAIL;
    }

    G_nandFlashInfoP = sysNflashDevGet(0);
    if (G_nandFlashInfoP == NULL)
    {
        mvOsPrintf("%s: sysNflashDevGet returned NULL.\n", __func__);
        return MV_FAIL;
    }

    flashSize = G_nandFlashInfoP->pNflashStruct->size;

    /* Update NAND Flash memory size in sysPhyMemDesc table 	*/
    #if defined(INCLUDE_MMU) && defined(CPU_926EJ)
        sysPhysMemDesc[4].len = ROUND_UP (flashSize, PAGE_SIZE);
    #endif

    if (flashSize < _1M)
    {
        mvOsPrintf("NAND boot Flash %dKB detected\n", flashSize / _1K);
    }
    else
    {
        mvOsPrintf("NAND boot Flash %dMB detected\n", flashSize / _1M);
    }

    return MV_OK;

#else

    return MV_OK;

#endif /* MV_INCLUDE_NAND */
}

/*******************************************************************************
* mvSysFlashInitSpi
*
* DESCRIPTION:
*       Initializes SPI flash.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_TRUE     - success.
*       MV_FALSE    - failure.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_STATUS mvSysFlashInitSpi(void)
{
#ifdef MV_INCLUDE_SPI
    MV_CHIP_FEATURES *featuresP = mvChipFeaturesGet();
    MV_SFLASH_INIT    sFlashInit;
    MV_CPU_DEC_WIN    addrDecWin;
    MV_U32            flashSize = 0;

    if (mvCpuIfTargetWinGet(SPI_CS, &addrDecWin) != MV_OK)
    {
        mvOsPrintf("%s: mvCpuIfTargetWinGet failed.\n", __func__);
    }

    /*
     * Prepare to init SPI flash.
     */
    G_spiFlashInfo.baseAddr         = addrDecWin.addrWin.baseLow;
    /* Other fields will be detected in init. */
    G_spiFlashInfo.manufacturerId   = 0;
    G_spiFlashInfo.deviceId         = 0;
    G_spiFlashInfo.sectorSize       = 0;
    G_spiFlashInfo.sectorNumber     = 0;
    G_spiFlashInfo.pageSize         = 0;
    G_spiFlashInfo.index            = MV_INVALID_DEVICE_NUMBER;

    sFlashInit.numOfSpiCS = featuresP->numOfSpiCS;
    if (mvSFlashInit(&G_spiFlashInfo, &sFlashInit) != MV_OK)
    {
        mvOsPrintf("%s mvSFlashInit failed.\n", __func__);
    }

    /*
     * Test flash sanity after init.
     */
    flashSize = mvSFlashSizeGet(&G_spiFlashInfo);
    if (flashSize == 0)
    {
        mvOsPrintf("%s: mvSFlashSizeGet returned 0.\n", __func__);
        return MV_FAIL;
    }

    /* Update SPI Flash memory size in sysPhyMemDesc table */
    #if defined(INCLUDE_MMU) && defined(CPU_926EJ)
        sysPhysMemDesc[1].len = ROUND_UP (flashSize, PAGE_SIZE);
    #endif
    #if defined(INCLUDE_FLASH_BOOTLINE)
        flashBootSectorInit();
    #endif

    if (flashSize < _1M)
    {
        mvOsPrintf("SPI boot Flash %dKB detected\n", flashSize / _1K);
    }
    else
    {
        mvOsPrintf("SPI boot Flash %dMB detected\n", flashSize / _1M);
    }

    return MV_OK;

#else

    return MV_OK;

#endif /* MV_INCLUDE_SPI */
}

/*******************************************************************************
* mvSysFlashInit
*
* DESCRIPTION:
*       Initializes flash (SPI or NAND).
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_TRUE     - success.
*       MV_FALSE    - failure.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_STATUS mvSysFlashInit(void)
{
    if (mvSysFlashInitSpi() != MV_OK)
    {
        mvOsPrintf("%s: mvSysFlashInitSpi failed.\n", __func__);
        return MV_FAIL;
    }

    if (mvSysFlashInitNand() != MV_OK)
    {
        mvOsPrintf("%s: mvSysFlashInitNand failed.\n", __func__);
        return MV_FAIL;
    }

    return MV_OK;
}

/*******************************************************************************
*
* sysHwInit - initialize the CPU board hardware
*
* This routine initializes various features of the hardware.
* Normally, it is called from usrInit() in usrConfig.c.
*
* NOTE: This routine should not be called directly by the user.
*
* RETURNS: N/A
*/

void sysHwInit (void)
{
    char boardName[132];
#ifndef BOOTROM
  unsigned long prestera_device_id;
  int xcat_revision;
#endif

    sysPrintPollInit(); /* For debug messages */
    mvOsPrintf("\n");

    /*
     * The next init (partial) order should be mantained:
     *     Init memory windows from CPU to Switch
     *     Read deviceId register of the Switch to ascertain the chip type
     *         (After this step mvBoardIdGet() returns the right ID)
     */
    sysOpenMemWinToSwitch();
#ifdef BSP_MODE
    if (mvSwitchSetChipType() != MV_OK)
    {
        mvOsPrintf("%s: mvSwitchSetChipType() failed.\n", __func__);
    }
#endif

    mvBoardEnvInit();
    mvDebugInit();

    mvBoardNameGet(boardName);
    mvOsPrintf("Detected board %s\n", boardName);
#if defined(CPU_946ES) || defined(CPU_946ES_T)
    mvOsPrintf("\n\nCPU MODE 946\n\n");
#endif

    /* Ref # GL-MISL-70 (Speculative Instruction Prefetch) */
    sysCacheConfig();

    /* install the IRQ/SVC interrupt stack splitting routine */
    _func_armIntStackSplit = sysIntStackSplit;

    /*
     * Init various Marvell system controller interfaces
     */

    /* Get SysClock */
    sysClockRate = mvBoardSysClkGet();

    if (mvCtrlInit() != MV_OK)
    {
        mvOsPrintf("Marvell Controller initialization failed\n");
        sysToMonitor (BOOT_NO_AUTOBOOT);
    }

    sdramSize = mvDramIfSizeGet();
#if defined(INCLUDE_MMU)
           /* Make sure sysPhysMemDesc isn't wrong */
#if defined(CPU_720T) || defined(CPU_720T_T) || defined(CPU_920T)   || \
                         defined(CPU_920T_T) || defined(CPU_926EJ)  || \
                         defined(CPU_926EJ_T)|| defined(CPU_1020E)  || \
                         defined(CPU_1022E)
        sysPhysMemDesc[0].len = ROUND_UP (sdramSize, PAGE_SIZE);
#endif

#if defined(CPU_740T) || defined(CPU_740T_T) || defined(CPU_940T)   || \
                         defined(CPU_940T_T) || defined(CPU_946ES)  || \
                         defined(CPU_946ES_T)
        if (sdramSize > _2G)
                sysPhysMemDesc[0].len = 0;
        else
                sysPhysMemDesc[0].len = ctrlSizeRegRoundUp (sdramSize, PAGE_SIZE);
#endif
#endif /* INCLUDE_MMU */

    mvOsPrintf("Detected DRAM ");
    mvSizePrint(sdramSize);
    mvOsPrintf("\n");
    mvOsPrintf("Detect SysClk  %dMHz\n", sysClockRate/1000000);
    mvOsPrintf("Detect Tclk    %dMHz\n", mvBoardTclkGet()/1000000);
    mvOsPrintf("Marvell Controller initialization Done.\n");

#ifdef INCLUDE_SERIAL
    sysSerialHwInit ();
#endif
    mvOsPrintf("\r"); /* After sysSerialHwInit */

#if defined(INCLUDE_MMU)
    mvPrintsysPhysMemDesc();
#endif
  /*
   * detect xCat version
   * (at this point we can access the internal xcat prestera id register)
   */
#ifndef BOOTROM
  prestera_device_id = bspReadRegisterInternal(PRESTERA_DEV_ID_REG_OFFSET_extern +
                     XCAT_INTERNAL_PP_BASE_ADDR_extern);
  mvOsPrintf("Internal prestera id register = 0x%08lx\n",
         prestera_device_id);
  xcat_revision = prestera_device_id & 0x0000000f; /* first 4 bits  */
  if (xcat_revision <= 1)
    xcat_revision = 1; /* if revision <=1 this is A0 */
  else
    xcat_revision -=1; /* revision 2 -> A1, revision 3 -> A2 */

  mvOsPrintf("xCat revision = %d\n", xcat_revision); /* no action - just print */
#endif
}

/*******************************************************************************
*
* sysHwInit2 - additional system configuration and initialization
*
* This routine connects system interrupts and does any additional
* configuration necessary.  Note that this is called from
* sysClkConnect() in the timer driver.
*
* RETURNS: N/A
*
*/
void sysHwInit2 (void)
{
    static BOOL initialised = FALSE;

#if defined (MV_BRIDGE_SYNC_REORDER)
    mvCpuIfBridgeReorderWAInit();
#endif
    if (initialised)
        return;
#ifdef MV_VXWORKS_ST
    usrBootLineInit (sysStartType);	/* crack the bootline */
#endif
    bootStringToStruct (BOOT_LINE_ADRS, &sysBootParams);

    /* initialise the interrupt library and interrupt driver */
    intLibInit (AMBA_INT_NUM_LEVELS, AMBA_INT_NUM_LEVELS, INT_MODE);
    ambaIntDevInit();

    /* Mask summary bit for Main Interrupt Cause High Register. */
    intDisable(INT_LVL_HIGHSUM);

    mvCtrlInit2();
    mvOsPrintf("mvCtrlInit2..... done\n");

    /* Connect bridge interrupt. */
    intConnect(INT_VEC_BRIDGE, bridgeInt, 0);
    intEnable(INT_LVL_BRIDGE);

#ifdef INCLUDE_SERIAL
    /* connect serial interrupt */
    sysSerialHwInit2();
#endif /* INCLUDE_SERIAL */

    /* Initialize Debug facilities. Used by mgiEnd.c */
    mvDebugInit();

#ifdef INCLUDE_PCI
    if (mvChipFeaturesGet()->numOfPex > 0)
    {
        FUNCPTR oldLogMsg;

        oldLogMsg =  _func_logMsg;
        _func_logMsg = sysLogPoll;

        pciConfigLibInit (PCI_MECHANISM_0, (ULONG)sysPciConfigRead,
                                          (ULONG)sysPciConfigWrite, NONE);

        sysPciAutoConfig();

        _func_logMsg = oldLogMsg;

        G_mvIsPexInited = 1; /* true */
        mvOsPrintf("PCI auto config. done\n");
    }

    mvSwitchGenBootCfg();

#ifdef INCLUDE_GEI_END
    mvOsPrintf("\"gei\" PCI init.. ");
        sys543PciInit ();
    mvOsPrintf("done\n");
#endif

#ifdef INCLUDE_FEI_END
    mvOsPrintf("\"fei\" PCI init.. ");
        sys557PciInit ();
    mvOsPrintf("done\n");
#endif

#ifdef INCLUDE_SYSKONNECT
    mvOsPrintf("\"sgi\" PCI init.. ");
    sysSkGePciInit();
    mvOsPrintf("done\n");
#endif

#endif  /* INCLUDE_PCI */

    if (mvPpHalInitHw() != MV_OK)
    {
        mvOsPrintf("%s: mvPpHalInitHw failed.\n", __func__);
    }

    initialised = TRUE;
    mvOsPrintf("Reserving non-cached hsu + rx/dx area at 0x%08x, len=0x%08x\n",
               (unsigned long)hsuBase, hsuLen);
    mvSetNonCacheable(hsuBase, hsuLen);

    mvOsPrintf("sysHwInit2...... Done\n");

    }

/*******************************************************************************
*
* sysPhysMemTop - get the address of the top of physical memory
*
* This routine returns the address of the first missing byte of memory,
* which indicates the top of memory.
*
* Normally, the user specifies the amount of physical memory with the
* macro LOCAL_MEM_SIZE in config.h.  BSPs that support run-time
* memory sizing do so only if the macro LOCAL_MEM_AUTOSIZE is defined.
* If not defined, then LOCAL_MEM_SIZE is assumed to be, and must be, the
* true size of physical memory.
*
* NOTE: Do no adjust LOCAL_MEM_SIZE to reserve memory for application
* use.  See sysMemTop() for more information on reserving memory.
*
* RETURNS: The address of the top of physical memory.
*
* SEE ALSO: sysMemTop()
*/

char * sysPhysMemTop (void)
{
    static char * physTop = NULL;

    if (physTop == NULL)
        {
#ifdef LOCAL_MEM_AUTOSIZE

        /* If auto-sizing is possible, this would be the spot.  */
#	if defined(INCLUDE_MMU)
                physTop = (char *)(LOCAL_MEM_LOCAL_ADRS + sdramSize);
/*		physTop = (char *)(LOCAL_MEM_LOCAL_ADRS + sysPhysMemDesc[0].len); */
#	else
                physTop = (char *)(LOCAL_MEM_LOCAL_ADRS + LOCAL_MEM_SIZE);
#	endif /* defined(INCLUDE_MMU) */

#else /* LOCAL_MEM_AUTOSIZE */
        /* Don't do autosizing, if size is given */

        physTop = (char *)(LOCAL_MEM_LOCAL_ADRS + LOCAL_MEM_SIZE);

#endif /* LOCAL_MEM_AUTOSIZE */
        }

    return physTop;
}

/*******************************************************************************
*
* sysMemTop - get the address of the top of VxWorks memory
*
* This routine returns a pointer to the first byte of memory not
* controlled or used by VxWorks.
*
* The user can reserve memory space by defining the macro USER_RESERVED_MEM
* in config.h.  This routine returns the address of the reserved memory
* area.  The value of USER_RESERVED_MEM is in bytes.
*
* RETURNS: The address of the top of VxWorks memory.
*/

char * sysMemTop (void)
    {
    static char * memTop = NULL;

    if (memTop == NULL)
        {
        memTop = sysPhysMemTop () - USER_RESERVED_MEM;
        }

    return memTop;
    }

/*******************************************************************************
*
* sysToMonitor - transfer control to the ROM monitor
*
* This routine transfers control to the ROM monitor.  It is usually called
* only by reboot() -- which services ^X -- and bus errors at interrupt
* level.  However, in some circumstances, the user may wish to introduce a
* new <startType> to enable special boot ROM facilities.
*
* RETURNS: Does not return.
*/

STATUS sysToMonitor (int startType)
    {
#ifdef SOFT_RESET
    FUNCPTR	pRom = (FUNCPTR)(ROM_BASE_ADRS);
#endif

#ifdef INCLUDE_SERIAL
    sysSerialReset ();	/* put serial devices into quiet state */
#endif

#if defined(CPU_720T)  || defined(CPU_720T_T) || \
    defined(CPU_740T)  || defined(CPU_740T_T) || \
    defined(CPU_920T)  || defined(CPU_920T_T) || \
    defined(CPU_926EJ) || defined(CPU_926EJ_T)|| \
    defined(CPU_940T)  || defined(CPU_940T_T) || \
    defined(CPU_946ES) || defined(CPU_946ES_T)

    VM_ENABLE(FALSE);	/* disable the MMU, cache(s) and write-buffer */
#endif

#if defined(CPU_920T) || defined(CPU_920T_T)
    /*
     * On 920T, can have the I-cache enabled once the MMU has been
     * disabled, so, unlike the other processors, disabling the MMU does
     * not disable the I-cache.  This would not be a problem, as the
     * 920T boot ROM initialisation code disables and flushes both caches.
     * However, in case we are, in fact, using a 7TDMI boot ROM,
     * disable and flush the I-cache here, or else the boot process may
     * fail.
     */
    cacheDisable (INSTRUCTION_CACHE);
#endif /* defined(CPU_920T/920T_T) */

#ifdef SOFT_RESET

#ifdef MV_INCLUDE_SAGE
    /* PP soft reset
     * set the SoftReset Trigger bit of the global coontrol register
     */
        MV_MEMIO32_WRITE(SAGE_UNIT_BASE + 0x58,BIT16);
#endif

        (*pRom)(startType);	/* jump to boot ROM */
#else /*Hardware reset*/
        MV_REG_BIT_SET(CPU_RSTOUTN_MASK_REG,BIT2);
        MV_REG_BIT_SET(CPU_SYS_SOFT_RST_REG,BIT0);
#endif

    return OK;		/* in case we ever continue from ROM monitor */
    }

/****************************************************************************
*
* sysProcNumGet - get the processor number
*
* This routine returns the processor number for the CPU board, which is
* set with sysProcNumSet().
*
* RETURNS: The processor number for the CPU board.
*
* SEE ALSO: sysProcNumSet()
*/

int sysProcNumGet (void)
    {
    return 0;
    }

/****************************************************************************
*
* sysProcNumSet - set the processor number
*
* Set the processor number for the CPU board.  Processor numbers should be
* unique on a single backplane.
*
* NOTE
* By convention, only processor 0 should dual-port its memory.
*
* RETURNS: N/A
*
* SEE ALSO: sysProcNumGet()
*/

void sysProcNumSet
    (
    int procNum		/* processor number */
    )
    {
    sysProcNum = procNum;
    }

/*******************************************************************************
* sysPrintPollInit - Initialize the serial channel in poll mode.
*
* DESCRIPTION:
*		Initialize the serial channel in poll mode. This function also
*       signals that poll mode is in use by setting the console file
*       descriptor to NONE.
*
* INPUT:
*		None.
*
* OUTPUT:
*		None.
*
* RETURN:
*		None.
*
*******************************************************************************/
void sysPrintPollInit(void)
{
    int ix;
    SIO_CHAN * pSioChan;        /* serial I/O channel */

    /* Signal the system that console is not ready yet. Use Poll mode   */
    consoleFd = NONE;

    sysSerialHwInit ();

    for (ix = 0; ix < NUM_TTY; ix++)
    {
        pSioChan = sysSerialChanGet (ix);
        sioIoctl (pSioChan, SIO_MODE_SET, (void *) SIO_MODE_POLL);
    }

    return;
}
/*******************************************************************************
* sysPrintfPoll - Print error message in poll mode
*
* DESCRIPTION:
*		None.
*
* INPUT:
*		None.
*
* OUTPUT:
*		None.
*
* RETURN:
*		None.
*
*******************************************************************************/
void sysPrintfPoll(char *pString, int nchars, int outarg)
{
    int msgIx;
    SIO_CHAN * pSioChan;        /* serial I/O channel */

        pSioChan = sysSerialChanGet (outarg);

    for(msgIx = 0; msgIx < nchars; msgIx++)
        {
        while(sioPollOutput (pSioChan, pString[msgIx]) == EAGAIN);

                if (pString[msgIx] == '\n')
        {
                        while(sioPollOutput (pSioChan, '\r') == EAGAIN);
        }
        }

}

/*******************************************************************************
* sysLogPoll - Log message in poll mode
*
*/

int sysLogPoll(UCHAR* s,UINT a,UINT b,UINT c,UINT d,UINT e,UINT f)
{
        mvOsPrintf(s , a,b,c,d,e,f);
        return 0;
}

/*******************************************************************************
* timerTickPassedGet - Accumulate clock ticks from the last call.
*
* DESCRIPTION:
*		This function adds to timestamp global parameter the amount of timer
*		ticks passed from the last time the function has been called.
*		The function takes into consideration a roll-over situation.
*
* INPUT:
*		None.
*
* OUTPUT:
*		None.
*
* RETURN:
*		The amount of timer ticks from the last call.
*
*******************************************************************************/
UINT32 timerTickPassedGet (void)
{
    UINT32 now = mvCntmrRead(SYS_TIMER_NUM);

    if (lastdec >= now)
    {
        /* normal mode (non roll) */
        /* move timestamp fordward with absoulte diff ticks */
        timestamp += (lastdec - now);
    }
    else
    {
        /* we have overflow of the count down timer */
        /* nts = ts + ld + (TLV - now)
         * ts=old stamp, ld=time that passed before passing through -1
         * (TLV-now) amount of time after passing though -1
         * nts = new "advancing time stamp"...it could also roll and cause problems.
         */
        timestamp += (lastdec + maxClkTicks - now);
    }

    lastdec = now;

    return timestamp;
}

/*******************************************************************************
* sysUsDelay - Use the system timer to create Micro-second delay.
*
* DESCRIPTION:
*		This function use the system timer to create Micro-second delay as
*		define by the input parameter.
*
* INPUT:
*		usec - Amount of micro-seconds to delay.
*
* OUTPUT:
*		None.
*
* RETURN:
*		None.
*
*******************************************************************************/
void sysUsDelay (UINT32 usec)
{
#if 1
    UINT32 totalTicks;

    lastdec = mvCntmrRead(SYS_TIMER_NUM);  /* capure current value time */

    /* Total number of ticks in this delay */
    totalTicks = usec * (SYS_TIMER_CLK / 1000000);

    /* Max clock ticks in case of roll-over */
    maxClkTicks = MV_REG_READ(CNTMR_RELOAD_REG(SYS_TIMER_NUM));

    /* reset time */
    timestamp = 0;         /* start "advancing" time stamp from 0 */

    while (timerTickPassedGet () < totalTicks); /* loop till event */
#else
    FAST UINT32 baselineTickCount;
    FAST UINT32 ticksToWait;

    /*
     * Get the Time Base Lower register tick count, this will be used
     * as the baseline.
     */

    baselineTickCount = sysTimeBaseLGet();

    /*
     * Convert delay time into ticks
     *
     * The Time Base register and the Decrementer count at the same rate:
     * once per 4 System Bus cycles.
     *
     * e.g., 66666666 cycles     1 tick      1 second             16 tick
     *       ---------------  *  ------   *  --------          =  ----------
     *       second              4 cycles    1000000 microsec    microsec
     */

    if ((ticksToWait = usec * ((DEC_CLOCK_FREQ / 4) / 1000000)) == 0)
        return;

    while ((sysTimeBaseLGet() - baselineTickCount) < ticksToWait);
#endif
}

/******************************************************************************
*
* sysMsDelay - delay for the specified amount of time (Mili-Seconds)
*
* This routine will delay for the specified amount of time by counting
* decrementer ticks.
*
* RETURNS: N/A
*/
void sysMsDelay
(
    UINT delay                   /* length of time in MS to delay */
)
{
    sysUsDelay(1000 * delay);
}

/*******************************************************************************
* sysUsDelayTest - Test the sysUsDelay routine.
*
* DESCRIPTION:
*		This function test the sysUsDelay routine. It uses the system Aux
*       clock as a reference clock.
*       The function prints "*" each time the diviation is bigger that the
*       given accuracy.
*
* INPUT:
*		microSec - Number of micro-seconds
*       accuracy - accuracy in precentage.
*
* OUTPUT:
*		None.
*
* RETURN:
*		None.
*
*******************************************************************************/
void sysUsDelayTest(UINT32 microSec, int accuracy)
{
    UINT32 calcTicks, actualTicks, deltaTicks;
    MV_CNTMR_CTRL cntmrCtrl; /* Marvell Timer/counter control */
    int intKey;

    /* Load Timer Reload value into Timer registers, set up in */
    /* one shot mode and enable timer */
    cntmrCtrl.enable  = MV_TRUE; /* start it  */
    cntmrCtrl.autoEnable = MV_FALSE; /* Work in auto mode - Periodic */
    mvOsPrintf("Each \"*\" stands for more than %d percent accuracy:\n", accuracy);

    /* Forever loop */
    while(1)
    {
        intKey = intLock();

        /* Initialize and starts the timer */
        mvCntmrStart(AUX_TIMER_NUM, TVR_ARM_TIMER_MAX, &cntmrCtrl);

        /* Call the tested delay routine */
        sysUsDelay(microSec);

        /* Take timestamp */
        /* timer counts down. Convert it to counting up */
        actualTicks = ~mvCntmrRead(AUX_TIMER_NUM);

        intUnlock(intKey);

        calcTicks   = microSec * (mvBoardTclkGet() / 1000000);

        deltaTicks  = ((actualTicks > calcTicks) ?
                       (actualTicks - calcTicks) :
                       (calcTicks - actualTicks));

        /* Stop the timer */
        mvCntmrDisable(AUX_TIMER_NUM);

        if (((deltaTicks * 100) / calcTicks) > accuracy)
        {
            if (actualTicks >= calcTicks)
            {
                mvOsPrintf("*");
            }
            else
            {
                mvOsPrintf("!");
            }
            /*mvOsPrintf("calcTicks = %d, actualTicks = %d\n", calcTicks, actualTicks);*/
        }

        /* Let the print go out */
        taskDelay(1);
    }
    return;
}

#if defined(INCLUDE_FLASH_BOOT_LINE)
/******************************************************************************
* flashBootSectorInit - Init Flash boot line address
* This function assigns EOS to boot line address in flash in case the Flash is
* empty (full with FFFFs). This is done because the sysNvRamGet function refer
* to EOS sign as an empty memory (the Flash is FFFF when empty).
*
* Inputs: N/A
*
* Outputs: The boot line address is EOS in case flash is empty.
*
* RETURN: N/A
*/
void flashBootSectorInit(void)
{
    int data;
#ifdef MV_INCLUDE_SPI
    if (mvSFlashBlockRd(&G_spiFlashInfo, NV_BOOT_OFFSET,(MV_U8*)&data,4) != MV_OK)
    {
        mvOsPrintf("\n failed read flashBootSectorInit");
        return;
    }

    if (data == 0xFFFFFFFF)
    {
        data = EOS;
        if (mvSFlashBlockWr(&G_spiFlashInfo, NV_BOOT_OFFSET, (MV_U8*)&data,4) != MV_OK)
        {
            mvOsPrintf("\n failed write flashBootSectorInit");
            return;
        }
    }

#elif defined(MV_INCLUDE_NAND)
    if (mvNflashPageRead(G_nandFlashInfoP, NV_BOOT_OFFSET, 4, (MV_U8*)&data) != 4)
    {
        mvOsPrintf("\n failed read flashBootSectorInit");
        return;
    }

    if (data == 0xFFFFFFFF)
    {
        data = EOS;
        if (mvNflashPageProg (G_nandFlashInfoP,
                              NV_BOOT_OFFSET , 4, (MV_U8*)&data, MV_FALSE) != 4)
        {
            mvOsPrintf("\n failed write flashBootSectorInit");
            return;
        }
    }
#endif
}
#endif /* INCLUDE_FLASH_BOOT_LINE */

/******************************************************************************
*
* sysNvRamGet - get the contents of non-volatile RAM
*
* This routine copies the contents of non-volatile memory into a specified
* string.  The string will be terminated with an EOS.
*
* RETURNS: OK, or ERROR if access is outside the non-volatile RAM range.
*
* SEE ALSO: sysNvRamSet()
*/
STATUS sysNvRamGet
(
    char *string,    /* where to copy non-volatile RAM           */
    int  strLen,     /* maximum number of bytes to copy          */
    int  offset      /* byte offset into non-volatile RAM        */
)
{
#ifdef INCLUDE_FLASH_BOOT_LINE
    if (offset < 0 || strLen < 0 || strLen > NV_RAM_SIZE)
        return ERROR;
    offset += NV_BOOT_OFFSET;

#ifdef MV_INCLUDE_SPI
    {
    MV_STATUS res = mvSFlashBlockRd(&G_spiFlashInfo, NV_BOOT_OFFSET, string, strLen);
    if(res != MV_OK || *BOOT_LINE_ADRS == EOS)
        return ERROR;
    }
#elif defined(MV_INCLUDE_NAND)
    {
    MV_U32 size = mvNflashPageRead(G_nandFlashInfoP, NV_BOOT_OFFSET, strLen, string);
    if(size != strLen || *BOOT_LINE_ADRS == EOS)
        return ERROR;
    }
#endif
    string [strLen] = EOS;
    return OK;

#endif /* INCLUDE_FLASH_BOOT_LINE */
}

/*******************************************************************************
*
* sysNvRamSet - write to non-volatile RAM
*
* This routine copies a specified string into non-volatile RAM.
*
* RETURNS: OK, or ERROR if access is outside the non-volatile RAM range.
*
* SEE ALSO: sysNvRamGet()
*/
STATUS sysNvRamSet
(
    char *string,     /* string to be copied into non-volatile RAM */
    int strLen,       /* maximum number of bytes to copy           */
    int offset        /* byte offset into non-volatile RAM         */
)
{
#if defined(BSP_MODE) && defined(MV_INCLUDE_PP)
    mvOsPrintf("Prestera network interface configuration options:\n");
    mvOsPrintf("PP-SDMA with Asynchronous TX operation: 0x%X\n",
            MV_USE_END_SWITCH_SDMA);
    mvOsPrintf("PP-SDMA with Synchronous  TX operation: 0x%X\n",
            MV_USE_END_SWITCH_SDMA | MV_USE_END_SWITCH_TX_MODE);
    mvOsPrintf("PP-MII  with Asynchronous TX operation: 0x%X\n",
            MV_USE_END_SWITCH_CPU);
    mvOsPrintf("PP-MII  with Synchronous  TX operation: 0x%X\n",
            MV_USE_END_SWITCH_CPU  | MV_USE_END_SWITCH_TX_MODE);
    mvOsPrintf("Write chosen value to 'flags' field of VxWorks boot line.\n");
#endif

#ifdef INCLUDE_FLASH_BOOT_LINE
    if (offset < 0 || strLen < 0 || offset + strLen > NV_RAM_SIZE)
    {
        mvOsPrintf("%s: %d: Bad parameters.\n", __func__, __LINE__);
        return ERROR;
    }
    offset += NV_BOOT_OFFSET;

#if defined (MV_INCLUDE_SPI) && defined (BOOT_LINE_ON_SPI_FLASH)
    {
    unsigned int bootLineSector;
    if (mvSFlashWpRegionSet(&G_spiFlashInfo,MV_WP_NONE) != MV_OK)
    {
        mvOsPrintf("%s: %d: mvSFlashWpRegionSet failed.\n", __func__, __LINE__);
        return ERROR;
    }

    bootLineSector = NV_BOOT_OFFSET / G_spiFlashInfo.sectorSize;
    if (mvSFlashSectorErase(&G_spiFlashInfo, bootLineSector) != MV_OK)
    {
        mvOsPrintf("%s: %d: Failed to erase flash boot sector.\n", __func__, __LINE__);
        return ERROR;
    }

    if (mvSFlashBlockWr(&G_spiFlashInfo, NV_BOOT_OFFSET, string, strLen) != MV_OK)
    {
        mvOsPrintf("%s: %d: Failed to write to flash.\n", __func__, __LINE__);
        return ERROR;
    }

    string [strLen] = EOS;
    return OK;
    }
#elif defined (MV_INCLUDE_NAND) && defined (BOOT_LINE_ON_NAND_FLASH)
    {
    unsigned int block, offset, blockSize;
    unsigned int lastBlock = G_nandFlashInfoP->pNflashStruct->blockNum;

    blockSize = G_nandFlashInfoP->pNflashStruct->pagesPerBlk *
                G_nandFlashInfoP->pNflashStruct->pageDataSize;

    for (block = 0; block < lastBlock; block++)
    {
        offset = mvNflashBlkOffsGet(G_nandFlashInfoP, block);
        if(offset <= NV_BOOT_OFFSET && offset + blockSize > NV_BOOT_OFFSET)
            break; /* found the block */
    }

    if (block == lastBlock)
    {
        mvOsPrintf("%s: %d: Invalid block.\n", __func__, __LINE__);
        return ERROR;
    }

    if (mvNflashBlockErase(G_nandFlashInfoP, block) != MV_OK)
    {
        mvOsPrintf("%s: %d: Failed to erase flash boot sector.\n", __func__, __LINE__);
        return ERROR;
    }

    if (mvNflashPageProg(G_nandFlashInfoP, NV_BOOT_OFFSET, strLen, string, MV_FALSE) != MV_OK)
    {
        mvOsPrintf("%s: %d: Failed to write to flash.\n", __func__, __LINE__);
        return ERROR;
    }

    return OK;
    }
#elif defined (MV_INCLUDE_NOR) && defined (BOOT_LINE_ON_NOR32_FLASH)
    {
    int bootLineSector;

    offset += NV_BOOT_OFFSET;   /* boot line begins at <offset> = 0 */

    mvFlashSecLockSet(&nor32FlashInfo,NV_BOOT_OFFSET,MV_FALSE);
    if (offset < 0 || strLen < 0 || offset + strLen > NV_RAM_SIZE)
    {
        mvOsPrintf("sysNvRamSet Failed Parameter checking.\n");
        return ERROR;
    }

    if((bootLineSector = mvFlashInWhichSec(&nor32FlashInfo, NV_BOOT_OFFSET)) == ERROR)
    {
        mvOsPrintf("sysNvRamSet Failed to allocate boot line sector\n");
        flashPrint(&nor32FlashInfo);
        return ERROR;
    }

    if(mvFlashSecErase(&nor32FlashInfo ,bootLineSector) == ERROR)
    {
        mvOsPrintf("sysNvRamSet Failed to erase Flash boot sector\n");
        return ERROR;
    }

    if(mvFlashBlockWr (&nor32FlashInfo, NV_BOOT_OFFSET, strLen, string) == ERROR)
    {
        mvOsPrintf("sysNvRamSet Failed to write to Flash\n");
        return ERROR;
    }

    return OK;
    }
#endif
#endif /* INCLUDE_FLASH_BOOT_LINE */
}

#if defined(INCLUDE_MMU)
void mvPrintsysPhysMemDesc(void)
{
        int	ix;

        mvOsPrintf("ix |*virtualAddr| *physicalAddr| len       |initialStateMask|initialState |\n");
        mvOsPrintf("---|------------|--------------|-----------|----------------|-------------|\n");
        for(ix = 0; ix < sysPhysMemDescNumEnt; ix++)
        {
                mvOsPrintf("%3d| %08x   | %08x     | %8x  | %8x       | %8x    |\n",
                                   ix,sysPhysMemDesc[ix].virtualAddr,
                                        sysPhysMemDesc[ix].physicalAddr,
                                        sysPhysMemDesc[ix].len,
                                        sysPhysMemDesc[ix].initialStateMask,
                                        sysPhysMemDesc[ix].initialState);
        }
        mvOsDelay(3);
}
#endif

extern int _L2cacheSupport;
void mvKWShow(void)
{
       char buf[1024];
       volatile MV_U32 regVal = 0;

        mvCpuIfPrintSystemConfig(buf,0);
        mvOsPrintf(buf);
#ifdef _DIAB_TOOL
        regVal = readMarvellEFRegDiab();
#else
    __asm volatile ("mrc	p15, 1, %0, c15, c1, 0" : "=r" (regVal)); /* Read Marvell extra features register */
#endif

        if (_L2cacheSupport)
        {
                mvOsPrintf("L2 cache library is supported\n");
                if (0 == (regVal & BIT22))
                mvOsPrintf("Warning!!! L2 is disabled and the OS Cache Library is with supported the L2 cache.\n");
        }
        else
        {
                mvOsPrintf("L2 cache Library is Not Supported\n");
                if (regVal & BIT22)
                mvOsPrintf("Warning!!! L2 is enabled and OS Cache Library not supported the L2 cache.\n");
        }
}

/******************************************************************************/
MV_BOOL rdreg (UINT32 offset)
{
    UINT32 value;

        value = MV_REG_READ(offset);

    mvOsPrintf("I-Register # 0x%04x has value of 0x%08x\n", offset, value);

    return(MV_TRUE);
}

/******************************************************************************/

MV_BOOL wrreg (UINT32 offset, UINT32 value)
{
    MV_REG_WRITE(offset, value);

    mvOsPrintf("I-Write to reg# 0x%04x value of 0x%08x\n",offset,value);

    return MV_TRUE;
}

#if !defined(BOOTROM) && !defined (VIPS_APP)

IMPORT unsigned long intDemuxErrorCount;
IMPORT int standTblSize;
IMPORT SYMBOL standTbl [];
IMPORT MV_ISR_ENTRY gpp7_0Array[2][8];
IMPORT MV_ISR_ENTRY gpp15_8Array[2][8];
IMPORT MV_ISR_ENTRY gpp23_16Array[2][8];
IMPORT MV_ISR_ENTRY gpp31_24Array[2][8];

int find_symbol_by_address(unsigned long address, char *name)
{
  int  j, sym_found;

  sym_found = 0;
  for (j = 0; j < standTblSize; j++)
    if (standTbl[j].value == (void *)address)
    {
      strcpy(name, standTbl[j].name);
      sym_found = 1;
      break;
    }

  if (!sym_found)
    strcpy(name, "??");

  return sym_found;
}

static unsigned long getIntVecTable(void)
{
    unsigned long addr;
    unsigned long isr;
    static unsigned long staticIntVecTable = 0;
    int i;

    if (staticIntVecTable == 0)
    {
        staticIntVecTable = 0xffffffff;
        for (i = 4; i < 12; i += 4)
        {
            addr = (*(unsigned long *)((unsigned long)&intDemuxErrorCount + i));
            /*
            Black magic. intDemuxErrorCount is an external symbol which is 4 or 8
            prior to the hidden (static) intVecTable (depending on command_line or WB).
            do nm -n vxWorks.st
            */

            if (!addr)
            continue;
            /*  isr = *(unsigned long *)addr; */
            isr = addr;
            staticIntVecTable = addr;
            break;

            /* the other dead code here is used on other systems. Don't remove it.
                Giora */
        }
    }

    return staticIntVecTable;
}

void showirq_gpio(int arrayNum)
{
  int i;
  unsigned long isr0 = 0;
  unsigned long isr1 = 0;
  char name[128];
  char name1[128];

  printf("  gpio array %d\n", arrayNum);
  for (i = 0; i < 8; i++)
  {
    switch ((arrayNum))
    {
    case 0:
      isr0 = (unsigned long)gpp7_0Array[0][i].userISR;
      isr1 = (unsigned long)gpp7_0Array[1][i].userISR;
      break;
    case 1:
      isr0 = (unsigned long)gpp15_8Array[0][i].userISR;
      isr1 = (unsigned long)gpp15_8Array[1][i].userISR;
      break;
    case 2:
      isr0 = (unsigned long)gpp23_16Array[0][i].userISR;
      isr1 = (unsigned long)gpp23_16Array[1][i].userISR;
      break;
    case 3:
      isr0 = (unsigned long)gpp31_24Array[0][i].userISR;
      isr1 = (unsigned long)gpp31_24Array[1][i].userISR;

    }

    find_symbol_by_address(isr0, name);
    find_symbol_by_address(isr1, name1);

    if (isr0)
      printf("    group 0 entry %d: count=%d, \thandler=0x%08lx, %s\n",
             i, -1, isr0, name);

    if (isr1)
      printf("    group 1 entry=%d: count=%d, \thandler1=0x%08lx, %s\n",
             i, -1, isr1, name1);
  }
}
void showirq(void)
{
  int i;
  unsigned long intVecTable = getIntVecTable();
  unsigned long isr;
  char name[128];
  int count;

  if (intVecTable == 0 || intVecTable == 0xffffffff)
  {
    printf("inVecTable not found\n");
    return;
  }
  printf("inVecTable    is at 0x%08lx\n", intVecTable);
  printf("gpp7_0Array   is at 0x%08lx\n", (unsigned long)gpp7_0Array);
  printf("gpp15_8Array  is at 0x%08lx\n", (unsigned long)gpp15_8Array);
  printf("gpp23_16Array is at 0x%08lx\n", (unsigned long)gpp23_16Array);
  printf("gpp31_24Array is at 0x%08lx\n", (unsigned long)gpp31_24Array);
  printf("\n");


  for (i = 0; i < 150; i++ )
  {
    isr = *(unsigned long *)(intVecTable+(i*8));
    find_symbol_by_address(isr, name);

    count=-1;

    if (!(strcmp(name, "??") == 0))
      printf("irq=%i, count=%d, \thandler=0x%08lx, %s\n",
             i, count, isr, name);
    if ((strcmp(name, "gpp7_0Int") == 0))
      showirq_gpio(0);
    if ((strcmp(name, "gpp15_8Int") == 0))
      showirq_gpio(1);
    if ((strcmp(name, "gpp23_16Int") == 0))
      showirq_gpio(2);
    if ((strcmp(name, "gpp31_24Int") == 0))
      showirq_gpio(3);
  }

  printf("\n");
}

int dump_bytes = 64;
#define gc_isprint(a) ((a >=' ')&&(a <= '~'))

static void gc_dump(char *buf, int len, int offset)
{
  int offs, i;
  int j;

  for (offs = 0; offs < len; offs += 16)
  {
    j = 1;
    printf("0x%08x : ", offs + offset);
    for (i = 0; i < 16 && offs + i < len; i++)
    {
      printf("%02x", (unsigned char)buf[offs + offset + i]);
      if (!((j++)%4) && (j < 16))
        printf(",");
    }
    for (; i < 16; i++)
      printf("   ");
    printf("  ");
    for (i = 0; i < 16 && offs + i < len; i++)
      printf("%c",
             gc_isprint(buf[offs + offset + i]) ? buf[offs + offset + i] : '.');
    printf("\n");
  }
  printf("\n");
}

static STATUS pciDumpHeader(int b, int d, int f, void *pArg /* ignored */)
{
  unsigned char buff[256];
  int addr;
  int nbytes = dump_bytes;

  memset(buff,0xff,dump_bytes);

  for (addr = 0 ; addr<dump_bytes ; addr++)
  {
    if ((0xff == b) && (0xFF == d) && (addr & 0x3))
      bspPciConfigReadReg(b, d, f, addr, (UINT32 *)(buff+addr));
    else
      pciConfigInByte(b, d, f, addr, (buff+addr));
  }

  printf("Device at [%d,%d,%d]\n", b, d, f);

  gc_dump((char *)buff, nbytes, 0);
  return 0;
}

STATUS static _lspci1(int b, int d, int f, void *p /* ignored */)
{
  /* dump some of the Marvell PP's registers */
  UINT16 vendor, device;
  UINT32 bar2;
  int offset;

  pciConfigInWord (b, d, f,  PCI_CFG_VENDOR_ID, &vendor);
  pciConfigInWord (b, d, f,  PCI_CFG_DEVICE_ID, &device);

  if ((vendor == 0x11ab) &&
      (((device & 0xf000) == 0xc000) || ((device & 0xf000) == 0xd000)))
  {
    printf("Marvell Packet Processor at [%d,%d,%d], vendor=0x%04x, device=0x%04x\n",
           b, d, f, vendor, device);

    offset = PCI_CFG_BASE_ADDRESS_0 + (2 * sizeof(UINT32));
    pciConfigInLong(b, d, f, offset, &bar2);
    bar2  &= ~ 0x0f;

    printf("bar2=0x%08x\n", bar2);
    gc_dump((char *)bar2, 128, 0);
  }
  return 0;
}

static void _lspci(void)
{
  int save;

  save = dump_bytes;
  dump_bytes = 64;
  printf("\nInternal prestera device\n");
  pciDumpHeader(0xff, 0xff, 0xff, NULL);
  dump_bytes = save;

  pciConfigForeachFunc(0, TRUE, pciDumpHeader, NULL);
  pciConfigForeachFunc(0, TRUE, _lspci1, NULL);
}

void lspci(void)
{
  dump_bytes = 64;
  _lspci();
}

void lspci_l(void)
{
  dump_bytes = 256;
  _lspci();
}
void mvSetNonCacheable
(
 void *addr,  /* virtual address to modify state of */
 int  len     /* len of virtual space to modify state of */
 )
{
  vmBaseStateSet(NULL, addr, len, VM_STATE_MASK_CACHEABLE, VM_STATE_CACHEABLE_NOT); 
}
#endif
