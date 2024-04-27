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
  01c,13jan00,pr   added support for ARM740T.
  01b,07dec99,pr   added support for PCI.
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

#include <vxWorks.h>
#include "config.h"

#include <memPartLib.h> /* for KMEM macros */
#include <sysLib.h>
#include <string.h>
#include <intLib.h>
#include <taskLib.h>
#include <vxLib.h>
#include <muxLib.h>
#include <cacheLib.h>
#include <usrConfig.h>

#include "mvCommon.h"
#include "cpu\mvCpu.h"
#include "mvPrestera.h"
#if defined (MV78200)
	#include "mv78200\mvSocUnitMap.h"
	#include "mv78200\mvSemaphore.h"
#endif

#if defined(CPU_720T)  || defined(CPU_720T_T) || \
    defined(CPU_920T)  || defined(CPU_920T_T) || \
    defined(CPU_926EJ)  || defined(CPU_926EJ_T) || \
    defined(CPU_1020E) || defined(CPU_1022E)
    #include <arch/arm/mmuArmLib.h>
    #include <private/vmLibP.h>
    #include <dllLib.h>
#elif defined(CPU_740T)  || defined(CPU_740T_T) || \
    defined(CPU_940T)  || defined(CPU_940T_T) || \
    defined(CPU_946ES) || defined(CPU_946ES_T)
    #include <private/vmLibP.h>
    #include <dllLib.h>
    #include <arch/arm/mmuArmLib.h>
#if (_WRS_VXWORKS_MAJOR == 6) && (_WRS_VXWORKS_MINOR > 5) 
    #include <arch/arm/mpuGlobalMap.h>
#endif /* (_WRS_VXWORKS_MAJOR == 6) && (_WRS_VXWORKS_MINOR > 5)  */
#endif /* defined(720T/740T/920T/940T/946ES/1020E/1022E) */

#if   defined(CPU_926EJ) || defined(CPU_926EJ_T)
#if (_WRS_VXWORKS_MAJOR == 6) && (_WRS_VXWORKS_MINOR > 2) 
IMPORT void	cacheArmFeroceonLibInstall (VIRT_ADDR(physToVirt) (PHYS_ADDR),
                                        PHYS_ADDR(virtToPhys) (VIRT_ADDR));
#endif 
#endif 

#include "ddr2/mvDramIf.h"

#include "ctrlEnv/sys/mvCpuIf.h"
#include "ctrlEnv/mvCtrlEnvSpec.h"
#include "cntmr/mvCntmrRegs.h"
#include "cntmr/mvCntmr.h"
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

#ifdef  INCLUDE_FLASH
    #include "norflash/mvFlash.h"

#   undef  NV_BOOT_OFFSET
#   define NV_BOOT_OFFSET 	(whoAmI() ? _512K : 0)             /* bootline at start of NVRAM */

#ifdef MV_INCLUDE_NOR
	MV_FLASH_INFO nor8FlashInfo;    /* Boot flash identifer */
	MV_FLASH_INFO nor32FlashInfo;    /* Main flash identifer */
#endif

#ifdef MV_INCLUDE_SPI
    #include "sflash/mvSFlash.h"
	MV_SFLASH_INFO spiFlashInfo;    /* SPI flash identifer */
#endif /* MV_INCLUDE_SPI  */
#if defined(MV_INCLUDE_NAND)
	MV_SFLASH_INFO nandFlashInfo;    /* Nand flash identifer */
#endif /* MV_INCLUDE_NAND  */

#endif  /* INCLUDE_FLASH */

#ifdef MV_INCLUDE_RTC 
	#include "VxBsp/vxRtc.h"
#endif /* MV_INCLUDE_RTC */

/* imports */
IMPORT void mv_fix_pex(void);
IMPORT void usrBootLineInit (int startType);
IMPORT char end [];         /* end of system, created by ld */
IMPORT VOIDFUNCPTR _func_armIntStackSplit;  /* ptr to fn to split stack */

IMPORT int  ambaIntDevInit (void);
IMPORT void sysIntStackSplit (char *, long);

IMPORT void      mvDebugInit(void);
IMPORT MV_STATUS mvCtrlInit(MV_VOID);
IMPORT MV_STATUS mvCtrlInit0(MV_VOID);
IMPORT MV_STATUS mvCtrlInit2(MV_VOID);

IMPORT STATUS sys543PciInit (void);
IMPORT void   sys557PciInit (void);
IMPORT STATUS sysSkGePciInit (void);
IMPORT UINT32 sysTimeBaseLGet (void);

IMPORT int  consoleFd;    /* fd of initial console device */
IMPORT int  sysStartType; /* BOOT_CLEAR, BOOT_NO_AUTOBOOT, ... */

IMPORT END_TBL_ENTRY    endDevTbl[];    /* end device table */
IMPORT END_OBJ* mgiEndLoad(char *, void*);

#if !defined(INCLUDE_MMU) &&                                        \
  (defined(INCLUDE_CACHE_SUPPORT) || defined(INCLUDE_MMU_BASIC) ||  \
   defined(INCLUDE_MMU_FULL) || defined(INCLUDE_MMU_MPU))
#define INCLUDE_MMU
#endif

#if defined(INCLUDE_CACHE_SUPPORT)
#if defined(CPU_7TDMI) || defined(CPU_7TDMI_T) || \
  defined(CPU_966ES) || defined(CPU_966ES_T)
FUNCPTR sysCacheLibInit = NULL;
#endif /* defined(CPU_7TDMI/7TDMI_T) */

#if defined(CPU_940T) || defined(CPU_940T_T) || \
  defined(CPU_926EJ) || defined(CPU_926EJ_T)
UINT32 * sysCacheUncachedAdrs = (UINT32 *)SYS_CACHE_UNCACHED_ADRS;
#endif /* defined(CPU_940T/940T_T) */
#endif /* defined(INCLUDE_CACHE_SUPPORT) */


/* globals */
UINT32 sysClockRate;        /* System clock variable        */
UINT32 sdramSize = LOCAL_MEM_SIZE;  /* SDRAM auto configurating result  for sysPhysMemTop */
char BspVersion[]=BSP_VERSION;		/* VxWorks BSP version  */
char BspRev[]=BSP_REV;				/* Marvell BSP version  */

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
#if defined(CPU_720T)  || defined(CPU_720T_T) ||  \
  defined(CPU_740T)  || defined(CPU_740T_T) ||    \
  defined(CPU_920T)  || defined(CPU_920T_T) ||    \
  defined(CPU_926EJ)  || defined(CPU_926EJ_T) ||  \
  defined(CPU_940T)  || defined(CPU_940T_T) ||    \
  defined(CPU_946ES) || defined(CPU_946ES_T) ||   \
  defined(CPU_1020E) || defined(CPU_1022E)
  
#if defined(CPU_720T) || defined(CPU_720T_T) ||   \
  defined(CPU_920T) || defined(CPU_920T_T) ||     \
  defined(CPU_926EJ)  || defined(CPU_926EJ_T) ||  \
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

      (VIRT_ADDR)  LOCAL_MEM_LOCAL_ADRS,  /* virtual address */
      (PHYS_ADDR ) LOCAL_MEM_LOCAL_ADRS,  /* physical address */
      ROUND_UP (LOCAL_MEM_SIZE, PAGE_SIZE), /* length, then initial state: */
      VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
      VM_STATE_VALID  | VM_STATE_WRITABLE  | VM_STATE_CACHEABLE
    },

#ifdef MV_INCLUDE_SPI
    {
      (VIRT_ADDR) DEVICE_SPI_BASE,          /* SPI Flash */
      (PHYS_ADDR) DEVICE_SPI_BASE,
      ROUND_UP (DEVICE_SPI_SIZE, PAGE_SIZE),
      VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
      VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT
    },
#endif
    /*
     * ROM is normally marked as uncacheable by VxWorks. We leave it like that
     * for the time being, even though this has a severe impact on execution
     * speed from ROM.
     */
    {
      (VIRT_ADDR)  (BOOTDEV_CS_BASE),       /* BOOT  Flash */ 
      (PHYS_ADDR)  (BOOTDEV_CS_BASE),
#ifdef DB_MV78200_A_AMC
      ROUND_UP (0x800000, PAGE_SIZE),
#else
      ROUND_UP (0x1000000, PAGE_SIZE),
#endif
      VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
      VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT
    },
#if DEVICE_CS0_SIZE_xxx
    {
      (VIRT_ADDR)  DEVICE_CS0_BASE,       /* Large Flash */
      (PHYS_ADDR)  DEVICE_CS0_BASE,
      ROUND_UP (DEVICE_CS0_SIZE, PAGE_SIZE),
      VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
      VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT
    },
#endif
#if DEVICE_CS1_SIZE 
    {
      (VIRT_ADDR) DEVICE_CS1_BASE,        /* 7-Segment */
      (PHYS_ADDR) DEVICE_CS1_BASE,
      ROUND_UP (DEVICE_CS1_SIZE, PAGE_SIZE),
      VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
      VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT
    },
#endif
#if DEVICE_CS2_SIZE 
    {
      (VIRT_ADDR) DEVICE_CS2_BASE,          /* Real Time Clock/Nand Flash */
      (PHYS_ADDR) DEVICE_CS2_BASE,
      ROUND_UP (DEVICE_CS2_SIZE, PAGE_SIZE),
      VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
      VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT
    },
#endif
#if DEVICE_CS3_SIZE 
    {
      (VIRT_ADDR) DEVICE_CS3_BASE,          /* Real Time Clock/Nand Flash */
      (PHYS_ADDR) DEVICE_CS3_BASE,
      ROUND_UP (DEVICE_CS3_SIZE, PAGE_SIZE),
      VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
      VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT
    },
#endif
    {
      (VIRT_ADDR)  INTER_REGS_BASE, /* Marvell Controller internal register */
      (PHYS_ADDR)  INTER_REGS_BASE,
      ROUND_UP (INTER_REGS_SIZE, PAGE_SIZE),
      VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
      VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT
    },

#ifdef INCLUDE_PCI
    {
      (VIRT_ADDR) PCI0_MEM0_BASE,   /* PCI Express Memory space */
      (PHYS_ADDR) PCI0_MEM0_BASE,
      ROUND_UP (PCI0_MEM0_SIZE, PAGE_SIZE),
      VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
      VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT
    },

    
    {
      (VIRT_ADDR) PCI1_MEM0_BASE,   /* PCI Express Memory space */
      (PHYS_ADDR) PCI1_MEM0_BASE,
      ROUND_UP (PCI1_MEM0_SIZE, PAGE_SIZE),
      VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
      VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT
    },


#if PCI2_MEM0_SIZE > 0
    {
      (VIRT_ADDR) PCI2_MEM0_BASE,   /* PCI Express Memory space */
      (PHYS_ADDR) PCI2_MEM0_BASE,
      ROUND_UP (PCI2_MEM0_SIZE, PAGE_SIZE),
      VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
      VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT
    },
#endif
#if PCI3_MEM0_SIZE > 0
    {
      (VIRT_ADDR) PCI3_MEM0_BASE,   /* PCI Express Memory space */
      (PHYS_ADDR) PCI3_MEM0_BASE,
      ROUND_UP (PCI3_MEM0_SIZE, PAGE_SIZE),
      VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
      VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT
    },
#endif
#if PCI4_MEM0_SIZE > 0
    {
      (VIRT_ADDR) PCI4_MEM0_BASE,   /* PCI Express Memory space */
      (PHYS_ADDR) PCI4_MEM0_BASE,
      ROUND_UP (PCI4_MEM0_SIZE, PAGE_SIZE),
      VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
      VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT
    },
#endif /* PCI4_MEM0_SIZE */

#if PCI0_IO_SIZE > 0
    {
      (VIRT_ADDR) PCI0_IO_BASE,   /* PCI Express IO space */
      (PHYS_ADDR) PCI0_IO_BASE,
      ROUND_UP (PCI0_IO_SIZE, PAGE_SIZE),
      VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
      VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT
    },
#endif /* PCI0_IO_BASE */

#if PCI1_IO_SIZE > 0
    {
      (VIRT_ADDR) PCI1_IO_BASE,   /* PCI Express IO space */
      (PHYS_ADDR) PCI1_IO_BASE,
      ROUND_UP (PCI1_IO_SIZE, PAGE_SIZE),
      VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
      VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT
    },
#endif /* PCI1_IO_BASE */

#if PCI2_IO_SIZE > 0
    {
      (VIRT_ADDR) PCI2_IO_BASE,   /* PCI Express IO space */
      (PHYS_ADDR) PCI2_IO_BASE,
      ROUND_UP (PCI2_IO_SIZE, PAGE_SIZE),
      VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
      VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT
    },
#endif /* PCI2_IO_BASE */

#if PCI3_IO_SIZE > 0
    {
      (VIRT_ADDR) PCI3_IO_BASE,   /* PCI Express IO space */
      (PHYS_ADDR) PCI3_IO_BASE,
      ROUND_UP (PCI3_IO_SIZE, PAGE_SIZE),
      VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
      VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT
    }
#endif /* PCI0_IO_BASE */

#if PCI4_IO_SIZE > 0
    ,
    {
      (VIRT_ADDR) PCI4_IO_BASE,   /* PCI Express IO space */
      (PHYS_ADDR) PCI4_IO_BASE,
      ROUND_UP (PCI4_IO_SIZE, PAGE_SIZE),
      VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
      VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT
    }
#endif /* PCI4_IO_SIZE */
#endif /* INCLUDE_PCI */
#ifdef MV_INCLUDE_CESA
    {
      (VIRT_ADDR) CRYPTO_BASE,
      (PHYS_ADDR) CRYPTO_BASE,
      CRYPTO_SIZE,
      VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
      VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT
    }
#endif
  };
#endif /* defined(CPU_720T/720T_T/920T/920T_T/926EJ/926EJ_T/1020E/1022E) */


#endif /* defined(CPU_720T/740T/920T/940T/946ES/1020E/1022E) */
#if defined(CPU_740T)  || defined(CPU_740T_T) ||  \
  defined(CPU_940T)  || defined(CPU_940T_T) ||    \
  defined(CPU_946ES) || defined(CPU_946ES_T)

/*
 * The following structure describes the various different regions of the
 * memory map to be used only during initialisation by
 * vmMpuGlobalMapInit() when INCLUDE_MMU_MPU is defined.
 *
 * On the MPUs, the virtual and physical addresses must be
 * the same.  In addition, the regions must have an alignment equal to
 * their size, with a minimum size of 4K.  This restriction is very
 * important in understanding the region definitions.  Regions cannot be
 * arbitrarily moved or their size changed without considering
 * alignment.  There is no page-table RAM overhead to mapping in large
 * areas of the memory map, but we can only define 8 memory regions in
 * total. Regions cannot be marked as read-only in VxWorks.
 *
 * Here, we (arbitrarily) choose to leave as many regions in the MPU
 * unused as possible, so that they are available for later use (e.g. to
 * mark areas of RAM as non-cacheable).  This means that large areas are
 * mapped in as valid where no memory or I/O devices are actually
 * present.  If this is not desired, larger numbers of smaller regions
 * could be defined which more closely match what is actually present in
 * the memory map (paying close attention to the alignment requirements
 * mentioned above).  Spurious accesses outside those defined regions
 * would then cause access violation exceptions when the MPU is switched
 * on.
 *
 * Note that potentially important areas of memory space are currently
 * unmapped.  Core module alias areas, and the EBI space
 * (Boot ROM and SSRAM) are not defined, and will therefore cause
 * access violations.
 */
PHYS_MEM_DESC sysPhysMemDesc [] =
  {

    /* DRAM must always be the first entry */
    /* adrs and length parameters must be page-aligned (multiples of 0x1000) */
    /* DRAM - Always the first entry */
    {

      (VIRT_ADDR)  LOCAL_MEM_LOCAL_ADRS,  /* virtual address */
      (PHYS_ADDR ) LOCAL_MEM_LOCAL_ADRS,  /* physical address */
      ROUND_UP (LOCAL_MEM_SIZE, PAGE_SIZE), /* length, then initial state: */
      VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
      VM_STATE_VALID  | VM_STATE_WRITABLE  | VM_STATE_CACHEABLE
    }

    /*
     * ROM is normally marked as uncacheable by VxWorks. We leave it like that
     * for the time being, even though this has a severe impact on execution
     * speed from ROM.
     */
#ifdef INCLUDE_PCI
    ,
    {
      (VIRT_ADDR) PCI0_MEM0_BASE,   /* PCI Express Memory space */
      (PHYS_ADDR) PCI0_MEM0_BASE,
      ROUND_UP (0x10000000, PAGE_SIZE),
      /*    ROUND_UP (ALL_PCI_MEM_SIZE, PAGE_SIZE), */
      VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
      VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT
    }
#endif
    ,
    /*
     * The following segment defines all DEVICE space.
     */
    {
      (VIRT_ADDR) PCI0_IO_BASE,
      (PHYS_ADDR) PCI0_IO_BASE,
      ROUND_UP (0x10000000, PAGE_SIZE),
      VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
      VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT
    }
  };
#endif /* defined(740T/940T/946ES) */

void mvPrintsysPhysMemDesc(void);
int sysPhysMemDescNumEnt = NELEMENTS (sysPhysMemDesc);

#endif /* defined(INCLUDE_MMU) */

int sysBus      = BUS;    /* system bus type (VME_BUS, etc) */
int sysCpu      = CPU;    /* system CPU type (e.g. ARMARCH4/4_T)*/
char *  sysBootLine = BOOT_LINE_ADRS;   /* address of boot line */
char *  sysExcMsg   = EXC_MSG_ADRS; /* catastrophic message area */
int sysProcNum;     /* processor number of this CPU */
int sysFlags;     /* boot flags */
char  sysBootHost [BOOT_FIELD_LEN]; /* name of host from which we booted */
char  sysBootFile [BOOT_FIELD_LEN]; /* name of file from which we booted */

char sysModelStr [80] = "";       /* Name of system */

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
    INT_LVL_UART0,    /* console port     */
    INT_LVL_UART1,    /* second serial port   */
# ifdef INCLUDE_PCI
    INT_LVL_PEX00_ABCD,   /* PCI device 0: */
    INT_LVL_PEX10_ABCD,   /* PCI device 1: */
# endif /* INCLUDE_PCI */
    -1        /* list terminator */
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
    0x00000000,     /* level 0, all disabled */
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
    0x00000001, 0x00000003, 0x00000007, 0x0000000f,
    0x0000001f, 0x0000003f, 0x0000007f, 0x000000ff,
    0x000001ff, 0x000003ff, 0x000007ff, 0x00000fff,
    0x00001fff, 0x00003fff, 0x00007fff, 0x0000ffff,
    0x0001ffff, 0x0003ffff, 0x0007ffff, 0x000fffff,
    0x001fffff, 0x003fffff, 0x007fffff, 0x00ffffff,  
    0x01ffffff, 0x03ffffff, 0x07ffffff, 0x0fffffff,  
    0x1fffffff, 0x3fffffff, 0x7fffffff, 0xffffffff,     
  };

# if AMBA_INT_NUM_LEVELS != 96
#     error ambaIntLvlMask is wrong size for number of levels
# endif
#endif  /* ifdef AMBA_INT_PRIORITY_MAP */

/* defines */

/* externals */

/* globals */

/* forward LOCAL functions declarations */

/* forward declarations */

char *  sysPhysMemTop (void);
int   sysLogPoll(UCHAR* s,UINT a,UINT b,UINT c,UINT d,UINT e,UINT f);
void  sysPrintPollInit(void);      /* Initialize the UART in poll mode */
void  sysPrintfPoll   (char *pString, int nchars, int  outarg);
void  sysCacheConfig(void);
void flashBootSectorInit(void);


#ifdef INCLUDE_NETWORK
# ifdef INCLUDE_GEI_END
#      include "./sysGei82543End.c"
#   endif

# ifdef INCLUDE_FEI_END
#      include "./sysFei82557End.c"
#   endif
#endif /* INCLUDE_NETWORK */

/* included source files */
#include "sysSerial.c"
#ifdef MV_INCLUDE_NAND
#include "sysNflash.c"
#endif

#include "mv88fxx81IntCtrl.c"

#include "mv88fxx81Timer.c" 

#ifdef _DIAB_TOOL
/* use macros */

asm volatile MV_U32 readMarvellEFRegDiab(void)
{
  mrc	p15, 1, r0, c15, c1, 0; /* Read Marvell extra features register */
/* return in r0 */
}

#endif


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

char *sysModel (void)
{
  mvBoardNameGet(sysModelStr);
		if (MV_78XX0_A1_REV == mvCtrlRevGet())
		{
			strcat (sysModelStr, "(A1)");
		}
  if (MV_REG_READ(CPU_CONFIG_REG(0)) & CCR_ENDIAN_INIT_BIG)
  {
    strcat (sysModelStr, " BE");
  }
  else
  {
    strcat (sysModelStr, " LE");
  }

  if (MV_REG_READ(CPU_CONFIG_REG(0)) & CCR_MMU_DISABLED_MASK)
  {
    strcat (sysModelStr, " MPU");
  }
  else
  {
    strcat (sysModelStr, " MMU");
  }
    
  strcat (sysModelStr, " ARCH 5");
    
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

char * sysBspRev (void)
{
  return (BSP_VERSION BSP_REV);
    }

void dramAutoDetect(void)
{
	mvCtrlInit0();
	sysCacheConfig();


}


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

void sysHwInit0 (void)
{
  MV_CNTMR_CTRL cntmrCtrl;

	mvBoardDebug7Seg(1);
  cntmrCtrl.enable  = MV_TRUE;
  cntmrCtrl.autoEnable = MV_TRUE;   /* Work in auto mode - Periodic */

  /* In this phase, the system timer was not initialized yet.       */
  /* start the counter to enable usage of system delay routine      */
  /* The OS timer will reinitialize the timer later           */
  mvCntmrStart(SYS_TIMER_NUM, TVR_ARM_TIMER_MAX, &cntmrCtrl);

#if defined(BOOTROM)
    /* WA for CPU L2 cache clock ratio limited to 1:3 erratum FE-CPU-180*/
	if (MV_78XX0_A1_REV == mvCtrlRevGet())
	{
		MV_REG_BIT_RESET(CPU_TIMING_ADJUST_REG(whoAmI()), BIT28);

		while((MV_REG_READ(CPU_TIMING_ADJUST_REG(whoAmI())) & BIT28) == BIT28);
	}
#endif
#if defined (MV78200)
	mvSemaUnlock(MV_SEMA_NOR_FLASH);
	{
		UINT32	regGP1;
		regGP1 = MV_REG_READ(GENERAL_USAGE_REGISTER_0);
		mvSocUnitMapFillTableFormBitMap(regGP1);
	}
#endif /* (MV78200)  */

#if defined(CPU_720T)  || defined(CPU_720T_T) || \
    defined(CPU_740T)  || defined(CPU_740T_T) || \
    defined(CPU_920T)  || defined(CPU_920T_T) || \
    defined(CPU_926EJ) || defined(CPU_926EJ_T)|| \
    defined(CPU_940T)  || defined(CPU_940T_T) || \
    defined(CPU_946ES) || defined(CPU_946ES_T)|| \
    defined(CPU_1020E) || defined(CPU_1022E)

#ifdef INCLUDE_CACHE_SUPPORT
  /*
   * Install the appropriate cache library, no address translation
   * routines are required for this BSP, as the default memory map has
   * virtual and physical addresses the same.
   */

# if  defined(CPU_720T) || defined(CPU_720T_T)
  cacheArm720tLibInstall (NULL, NULL);
# elif   defined(CPU_740T) || defined(CPU_740T_T)
  cacheArm740tLibInstall (NULL, NULL);
# elif   defined(CPU_920T) || defined(CPU_920T_T)
  cacheArm920tLibInstall (NULL, NULL);
# elif   defined(CPU_926EJ) || defined(CPU_926EJ_T)

#if (_WRS_VXWORKS_MAJOR == 6) && (_WRS_VXWORKS_MINOR > 2)  
  cacheArmFeroceonLibInstall (NULL, NULL);
#else
  cacheArm926eLibInstall (NULL, NULL);
#endif /* (_WRS_VXWORKS_MAJOR == 6) && (_WRS_VXWORKS_MINOR > 2)  */

# elif   defined(CPU_940T) || defined(CPU_940T_T)
  cacheArm940tLibInstall (NULL, NULL);
# elif   defined(CPU_946ES) || defined(CPU_946ES_T)
  cacheArm946eLibInstall (NULL, NULL);
# elif   defined(CPU_1020E)
  cacheArm1020eLibInstall (NULL, NULL);
# elif   defined(CPU_1022E)
  cacheArm1022eLibInstall (NULL, NULL);
# endif

#endif /* INCLUDE_CACHE_SUPPORT */


#if defined(INCLUDE_MMU)

    
  /* Install the appropriate MMU library and translation routines */
# if  defined(CPU_720T) || defined(CPU_720T_T)
  mmuArm720tLibInstall (NULL, NULL);
# elif   defined(CPU_740T) || defined(CPU_740T_T)
  mmuArm740tLibInstall (NULL, NULL);
# elif   defined(CPU_920T) || defined(CPU_920T_T)
  mmuArm920tLibInstall (NULL, NULL);
# elif   defined(CPU_926EJ) || defined(CPU_926EJ_T)
  mmuArm926eLibInstall (NULL, NULL);
# elif   defined(CPU_940T) || defined(CPU_940T_T)
  mmuArm940tLibInstall (NULL, NULL);
# elif   defined(CPU_946ES) || defined(CPU_946ES_T)
  mmuArm946eLibInstall (NULL, NULL);
# elif   defined(CPU_1020E)
  mmuArm1020eLibInstall (NULL, NULL);
# elif   defined(CPU_1022E)
  mmuArm1022eLibInstall (NULL, NULL);
# endif

#endif /* defined(INCLUDE_MMU) */
#endif /* defined(720T/740T/920T/926EJ/940T/946ES) */

	   mvBoardDebug7Seg(2);
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
  
	mvBoardDebug7Seg(3);
	/* sysPrintPollInit  */
	sysPrintPollInit();
    /* Init board environment */
    mvBoardEnvInit();
	
    mvDebugInit();

    mvOsPrintf("\n");
    mvBoardNameGet(boardName);
    mvOsPrintf("Detected board %s\n", boardName);
#if   defined(CPU_946ES) || defined(CPU_946ES_T)
	mvOsPrintf("\n\nCPU MODE 946\n\n");
#endif
	dramAutoDetect();
	mvBoardDebug7Seg(4);

  /* install the IRQ/SVC interrupt stack splitting routine */
  _func_armIntStackSplit = sysIntStackSplit;
  
  
  /* Init various Marvell system controller interfaces */
  /* Get SysClock */
  sysClockRate = mvBoardSysClkGet();

  if (MV_OK != mvCtrlInit())
  {       
    mvOsPrintf("Marvell Controller initialization failed\n");
    
    sysToMonitor (BOOT_NO_AUTOBOOT);
  }
  
  
#ifdef DB_MV78200_A_AMC
  MV_REG_WRITE(SDRAM_SIZE_REG(0,0), 0x1ffffff1); /* set size to 512MB */
  MV_REG_WRITE(SDRAM_SIZE_REG(0,1), 0); 
#endif

  sdramSize = mvDramIfSizeGet();
#if defined(INCLUDE_MMU)
  /* Make sure sysPhysMemDesc isn't wrong */
#if defined(CPU_720T) || defined(CPU_720T_T) ||   \
  defined(CPU_920T) || defined(CPU_920T_T) ||     \
  defined(CPU_926EJ)  || defined(CPU_926EJ_T) ||  \
  defined(CPU_1020E) || defined(CPU_1022E)
  sysPhysMemDesc[0].len = ROUND_UP (sdramSize, PAGE_SIZE);  
#endif /* defined(CPU_720T/720T_T/920T/920T_T/926EJ/926EJ_T/1020E/1022E) */

#if defined(CPU_740T)  || defined(CPU_740T_T) ||  \
  defined(CPU_940T)  || defined(CPU_940T_T) ||    \
  defined(CPU_946ES) || defined(CPU_946ES_T)
  if (sdramSize > _2G)
  {
    sysPhysMemDesc[0].len = 0;
  }
  else
  {
    sysPhysMemDesc[0].len = ctrlSizeRegRoundUp (sdramSize, PAGE_SIZE);
  }
#endif /* defined(740T/940T/946ES) */

#endif /* INCLUDE_MMU */
  mvOsPrintf("\n\nDetected DRAM ");
  mvSizePrint(sdramSize);
  mvOsPrintf("\nDetect SysClk  %dMHz\n", sysClockRate/1000000);

  mvOsPrintf("Detect Tclk    %dMHz\n", mvBoardTclkGet()/1000000);
  mvOsPrintf("Marvell Controller initialization Done.\n");
  /* Get DRAM size (can be used only after Dunit interface initiated) */


#ifdef INCLUDE_SERIAL
  /* initialise the serial devices */

  sysSerialHwInit ();      /* initialise serial data structure */
#endif /* INCLUDE_SERIAL */
  
  /* After sysSerialHwInit */
  mvOsPrintf("\r");

#ifdef INCLUDE_FLASH
  {
#if defined(MV_INCLUDE_SPI)  || defined(MV_INCLUDE_NOR)   
    UINT32 flashSize;
    MV_CPU_DEC_WIN flashCS;
#endif

#ifdef MV_INCLUDE_SPI
    /* SPI flash Init */
    if (mvCpuIfTargetWinGet(SPI_CS, &flashCS) != MV_OK) 
    {
      mvOsPrintf("\nfailed calling mvCpuIfTargetWinGet\n");
    }
    spiFlashInfo.baseAddr         = flashCS.addrWin.baseLow;
    spiFlashInfo.manufacturerId   = 0;                        /* will be detected in init */
    spiFlashInfo.deviceId         = 0;                        /* will be detected in init */
    spiFlashInfo.sectorSize       = 0;                        /* will be detected in init */
    spiFlashInfo.sectorNumber     = 0;                        /* will be detected in init */
    spiFlashInfo.pageSize         = 0;                        /* will be detected in init */
    spiFlashInfo.index            = MV_INVALID_DEVICE_NUMBER; /* will be detected in init */
    if (mvSFlashInit(&spiFlashInfo) != MV_OK)
    {
      mvOsPrintf("%s ERROR: SFlash init falied!\n", __FUNCTION__);
    }
    /* return the size of the detected SFLash */
    flashSize = mvSFlashSizeGet(&spiFlashInfo);
    if (0 == flashSize) 
    {
      mvOsPrintf("SPI Flash initialization failed\n");
    }
    else
    {
      if (flashSize< _1M)
      {
        mvOsPrintf("SPI Flash %dKB detected\n", flashSize / _1K);
      }
      else
      {
        mvOsPrintf("SPI Flash %dMB detected\n", flashSize / _1M);
      }
    }

    /* Update SPI Flash memory size in sysPhyMemDesc table  */
#if defined(INCLUDE_MMU)
#if defined(CPU_720T) || defined(CPU_720T_T) ||   \
  defined(CPU_920T) || defined(CPU_920T_T) ||     \
  defined(CPU_926EJ)  || defined(CPU_926EJ_T) ||  \
  defined(CPU_1020E) || defined(CPU_1022E)
    sysPhysMemDesc[1].len = ROUND_UP (flashSize, PAGE_SIZE);
#endif
#endif
#endif /* MV_INCLUDE_SPI  */

#ifdef MV_INCLUDE_NOR
    /* MOR  flash Init */
#if defined (MV78100)
    nor32FlashInfo.busWidth = MV_BOARD_FLASH_BUS_WIDTH/8;   
    nor32FlashInfo.devWidth = MV_BOARD_FLASH_DEVICE_WIDTH / 8; 
    if (mvBoardIsBootFromNor())
    {
      mvCpuIfTargetWinGet(DEV_BOOCS, &flashCS);
      nor8FlashInfo.baseAddr  = flashCS.addrWin.baseLow;
      nor8FlashInfo.busWidth  = mvDevWidthGet(BOOT_CS) / 8;
      /* BOOT_CS bus width is sampled on reset. If it is 16 bit, we   */
      /* boot from large flash. Otherwise, we boot from 512KB flash   */
      if (4 != nor8FlashInfo.busWidth)
      {
        nor8FlashInfo.devWidth = 1; /* Boot from 521KB flash */
        flashSize = mvFlashInit(&nor8FlashInfo);
        if (0 == flashSize) 
        {
          mvOsPrintf("Boot Flash initialization failed\n");
        }
        else
        {
          mvOsPrintf("Boot Flash %dKB detected\n", flashSize / _1K);
        }

        /* Update boot Flash memory size in sysPhyMemDesc table   */
        /* Note that boot Flash entry MUST be 1           */
#if defined(INCLUDE_MMU)
#if defined(CPU_720T) || defined(CPU_720T_T) ||   \
  defined(CPU_920T) || defined(CPU_920T_T) ||     \
  defined(CPU_926EJ)  || defined(CPU_926EJ_T) ||  \
  defined(CPU_1020E) || defined(CPU_1022E)
        sysPhysMemDesc[2].len = ROUND_UP (flashSize, PAGE_SIZE);
#endif
#endif
        mvCpuIfTargetWinGet(DEVICE_CS0, &flashCS);

        nor32FlashInfo.baseAddr = flashCS.addrWin.baseLow;

      } else
      {  /* Boot from Large flash */ 

#if defined(INCLUDE_MMU)
#if defined(CPU_720T) || defined(CPU_720T_T) ||   \
  defined(CPU_920T) || defined(CPU_920T_T) ||     \
  defined(CPU_926EJ)  || defined(CPU_926EJ_T) ||  \
  defined(CPU_1020E) || defined(CPU_1022E)
        sysPhysMemDesc[2].physicalAddr = (PHYS_ADDR) DEVICE_CS0_BASE;
        sysPhysMemDesc[2].virtualAddr  = (VIRT_ADDR) DEVICE_CS0_BASE;
        sysPhysMemDesc[2].len = ROUND_UP (0x100000, PAGE_SIZE);
        sysPhysMemDesc[3].physicalAddr = (PHYS_ADDR) flashCS.addrWin.baseLow;
        sysPhysMemDesc[3].virtualAddr  = (VIRT_ADDR) flashCS.addrWin.baseLow;
        sysPhysMemDesc[3].len = ROUND_UP (_32M, PAGE_SIZE);
#endif      
#endif      
        nor32FlashInfo.baseAddr = flashCS.addrWin.baseLow;
      }
    }
    else
#endif
    {
#if defined (MV78100)
      mvCpuIfTargetWinGet(DEVICE_CS0, &flashCS);
#else
      nor32FlashInfo.busWidth = mvDevWidthGet(BOOT_CS) / 8;
      nor32FlashInfo.devWidth = mvBoardGetDeviceWidth(0,BOARD_DEV_NOR_FLASH) / 8; 
      mvCpuIfTargetWinGet(DEV_BOOCS, &flashCS);
#endif
      nor32FlashInfo.baseAddr = flashCS.addrWin.baseLow;
    }
    /* Main flash Init*/
    flashSize = mvFlashInit(&nor32FlashInfo);
    if (0 == flashSize)
    {
      mvOsPrintf("Main Flash initialization failed Base 0x%x\n",nor32FlashInfo.baseAddr);
    } else
    {
      mvOsPrintf("Main Flash %dMB detected\n", flashSize / _1M);
#if defined(INCLUDE_MMU)
#if defined(CPU_720T) || defined(CPU_720T_T) ||   \
  defined(CPU_920T) || defined(CPU_920T_T) ||     \
  defined(CPU_926EJ)  || defined(CPU_926EJ_T) ||  \
  defined(CPU_1020E) || defined(CPU_1022E)
      /* Must be sysPhysMemDesc [2] to allow adjustment in sysHwInit() */
#if defined (MV78100)
      sysPhysMemDesc[3].len = ROUND_UP (flashSize, PAGE_SIZE);
#else
      sysPhysMemDesc[2].len = ROUND_UP (flashSize, PAGE_SIZE);
#endif
#endif      
#endif      
    }
#endif /* MV_INCLUDE_NOR  */
    /* NAND Flash interface init */
#ifdef MV_INCLUDE_NAND
    sysNflashHwInit();
#endif
#ifdef INCLUDE_FLASH_BOOTLINE
    flashBootSectorInit();
#endif

  }
#endif /* INCLUDE_FLASH */
/*			mvPrintsysPhysMemDesc(); */
	
	mvBoardDebug7Seg(5);
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

	mvBoardDebug7Seg(6);
#if defined (MV_BRIDGE_SYNC_REORDER)
  mvCpuIfBridgeReorderWAInit();
#endif
  if (initialised)
    return;
	if (sysStartType & BOOT_CLEAR)
	{
		/* this is a cold boot so get the default boot line */

		if ((sysNvRamGet (BOOT_LINE_ADRS, BOOT_LINE_SIZE, 0) == ERROR) ||
			(*BOOT_LINE_ADRS == EOS) || (*BOOT_LINE_ADRS == (char) -1))
		{
			/* either no non-volatile RAM or empty boot line */

			strcpy (BOOT_LINE_ADRS, DEFAULT_BOOT_LINE);
		}
	}
  bootStringToStruct (BOOT_LINE_ADRS, &sysBootParams);

  /* initialise the interrupt library and interrupt driver */
  intLibInit (AMBA_INT_NUM_LEVELS, AMBA_INT_NUM_LEVELS, INT_MODE);
  ambaIntDevInit();
  intEnable((int)INT_LVL_ERRSUM); /* mask enable for error Interrupt Cause Register*/
  
  mvCtrlInit2();
  mvOsPrintf("mvCtrlInit2..... done\n");

  /* connect auxiliary interrupt */
  intConnect(INT_VEC_TIMER1 ,sysAuxClkInt, 0);
  intEnable(INT_LVL_TIMER1);

  /* connect timer interrupt */
  intConnect(INT_VEC_TIMER0 ,sysClkInt, 0);
  intEnable(INT_LVL_TIMER0);

                  

#ifdef INCLUDE_SERIAL
  /* connect serial interrupt */
  sysSerialHwInit2();
#endif /* INCLUDE_SERIAL */


  /* Initialize Debug facilities. Used by mgiEnd.c */
  mvDebugInit();

#ifdef INCLUDE_PCI
  {
    FUNCPTR oldLogMsg; 

    oldLogMsg =  _func_logMsg;
    _func_logMsg = sysLogPoll;
  
    pciConfigLibInit (PCI_MECHANISM_0, (ULONG)sysPciConfigRead,
                      (ULONG)sysPciConfigWrite, NONE); 
       
    sysPciAutoConfig();

    _func_logMsg = oldLogMsg;

    mvOsPrintf("PCI auto config. done\n");

    mv_fix_pex();
  }
#endif  /* INCLUDE_PCI */

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

    if (mvPpHalInitHw() != MV_OK)
    {
        mvOsPrintf("%s: mvPpHalInitHw failed.\n", __func__);
    }

    initialised = TRUE;

	mvBoardDebug7Seg(7);
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
# if defined(INCLUDE_MMU)
    physTop = (char *)(LOCAL_MEM_LOCAL_ADRS + sdramSize); 
    /*    physTop = (char *)(LOCAL_MEM_LOCAL_ADRS + sysPhysMemDesc[0].len); */
# else
    physTop = (char *)(LOCAL_MEM_LOCAL_ADRS + LOCAL_MEM_SIZE);
# endif /* defined(INCLUDE_MMU) */

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

STATUS sysToMonitor
(
 int startType  /* passed to ROM to tell it how to boot */
 )
{

#ifdef DB_MV78200_A_AMC
  /* GPIO and MPP stuff. Taken from discoduo uboot. gc */   
  MV_REG_WRITE(0xf1010010, 0x11112222);
  MV_REG_BIT_SET(0xf1010100, (1 << 7));
  MV_REG_BIT_RESET(0xf1010104, (1 << 7));
  while(1);
#else

  FUNCPTR pRom = (FUNCPTR)(ROM_WARM_ADRS);
#ifdef INCLUDE_SERIAL
  sysSerialReset ();  /* put serial devices into quiet state */
#endif

#if defined(CPU_720T)  || defined(CPU_720T_T) ||  \
  defined(CPU_740T)  || defined(CPU_740T_T) ||    \
  defined(CPU_920T)  || defined(CPU_920T_T) ||    \
  defined(CPU_926EJ) || defined(CPU_926EJ_T)||    \
  defined(CPU_940T)  || defined(CPU_940T_T) ||    \
  defined(CPU_946ES) || defined(CPU_946ES_T)

  VM_ENABLE(FALSE); /* disable the MMU, cache(s) and write-buffer */
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

  (*pRom)(startType); /* jump to boot ROM */

  return OK;    /* in case we ever continue from ROM monitor */
#endif /* DB_MV78200_A_AMC */

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
 int procNum    /* processor number */
 )
{
  sysProcNum = procNum;
}

/*******************************************************************************
 * sysPrintPollInit - Initialize the serial channel in poll mode.
 *
 * DESCRIPTION:
 *    Initialize the serial channel in poll mode. This function also 
 *       signals that poll mode is in use by setting the console file 
 *       descriptor to NONE. 
 *
 * INPUT:
 *    None.
 *
 * OUTPUT:
 *    None.
 *
 * RETURN:
 *    None.
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
 *    None.
 *
 * INPUT:
 *    None.
 *
 * OUTPUT:
 *    None.
 *
 * RETURN:
 *    None.
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
 *    This function adds to timestamp global parameter the amount of timer 
 *    ticks passed from the last time the function has been called. 
 *    The function takes into consideration a roll-over situation.
 *
 * INPUT:
 *    None.
 *
 * OUTPUT:
 *    None.
 *
 * RETURN:
 *    The amount of timer ticks from the last call.
 *
 *******************************************************************************/
UINT32 timerTickPassedGet (void)
{
  UINT32 now = mvCntmrRead(SYS_TIMER_NUM);  /* Get current tick value */

  if (lastdec >= now) 
  { /* normal mode (non roll) */
    /* move timestamp fordward with absoulte diff ticks */
    timestamp += (lastdec - now); 
  } 
  else 
  { /* we have overflow of the count down timer */
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
 *    This function use the system timer to create Micro-second delay as
 *    define by the input parameter.
 *
 * INPUT:
 *    usec - Amount of micro-seconds to delay.
 *
 * OUTPUT:
 *    None.
 *
 * RETURN:
 *    None.
 *
 *******************************************************************************/
void sysUsDelay (UINT32 usec)
{
  UINT32 totalTicks;
  
  lastdec = mvCntmrRead(SYS_TIMER_NUM);  /* capure current value time */

  /* Total number of ticks in this delay */
  totalTicks = usec * (SYS_TIMER_CLK / 1000000);
    
  /* Max clock ticks in case of roll-over */
  maxClkTicks = MV_REG_READ(CNTMR_RELOAD_REG(SYS_TIMER_NUM));

  /* reset time */
  timestamp = 0;                /* start "advancing" time stamp from 0 */

  while (timerTickPassedGet () < totalTicks); /* loop till event */
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
 *    This function test the sysUsDelay routine. It uses the system Aux 
 *       clock as a reference clock. 
 *       The function prints "*" each time the diviation is bigger that the 
 *       given accuracy.
 *
 * INPUT:
 *    microSec - Number of micro-seconds
 *       accuracy - accuracy in precentage.
 *
 * OUTPUT:
 *    None.
 *
 * RETURN:
 *    None.
 *
 *******************************************************************************/
void sysUsDelayTest(UINT32 microSec, int accuracy)
{
  UINT32 calcTicks, actualTicks, deltaTicks;
  MV_CNTMR_CTRL cntmrCtrl;  /* Marvell Timer/counter control */
  int intKey;        

  /* Load Timer Reload value into Timer registers, set up in      */
  /* one shot mode and enable timer                   */
  cntmrCtrl.enable  = MV_TRUE;    /* start it           */
  cntmrCtrl.autoEnable = MV_FALSE;    /* Work in auto mode - Periodic */
  mvOsPrintf("Each \"*\" stands for more than %d percent accuracy:\n", accuracy);

  /* Forever loop */
  while(1)
  {
    intKey = intLock();

    /* Initialize and starts the timer */
    mvCntmrStart(AUX_TIMER_NUM, 0xFFFFFFFF, &cntmrCtrl);   

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

/*******************************************************************************
 * mvSysTimerMeasureStart
 *
 * DESCRIPTION:
 *    Start system Aux clock as a reference clock.
 *
 * INPUT:
 *    None
 *
 * OUTPUT:
 *    None.
 *
 * RETURN:
 *    None.
 *
 *******************************************************************************/
void mvSysTimerMeasureStart
(
    void
)
{
    MV_CNTMR_CTRL cntmrCtrl;  /* Marvell Timer/counter control */
    
    /* Load Timer Reload value into Timer registers, set up in
        one shot mode and enable timer                   
    */
    cntmrCtrl.enable  = MV_TRUE;        /* start it           */
    cntmrCtrl.autoEnable = MV_FALSE;    /* don't restart counter automatically */

    /* can't close here interrupts, because it will cause unpredictable behavior 
        of delay system calls */

    /* Initialize and start the timer */
    mvCntmrStart(AUX_TIMER_NUM, 0xFFFFFFFF, &cntmrCtrl);   

    return;
}

/*******************************************************************************
 * mvSysTimerMeasureGet
 *
 * DESCRIPTION:
 *    Get time in miliseconds since timer started stop the timer.
 *
 * INPUT:
 *    None.
 *
 * OUTPUT:
 *    miliSecPtr - time in mili-seconds
 *
 * RETURN:
 *    None.
 *
 *******************************************************************************/
void mvSysTimerMeasureGet
(
    UINT32 *miliSecPtr
)
{
    UINT32 timeInTicks;

    /* Take timestamp */
    /* timer counts down. Convert it to counting up */
    timeInTicks = ~mvCntmrRead(AUX_TIMER_NUM);

    /* Stop the timer */
    mvCntmrDisable(AUX_TIMER_NUM);
    
    /* devide number of ticks by number of ticks in mili-second */
    *miliSecPtr = timeInTicks / (mvBoardTclkGet()/1000);
    /* printf("timeInTicks=%d,mvBoardTclkGet()=%d\n", timeInTicks, mvBoardTclkGet()); */

    return;
}

#if defined(INCLUDE_FLASH_BOOTLINE) 
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
#ifdef MV_INCLUDE_NOR
#ifdef BOOTLINE_ON_NOR32_FLASH
  if (mvFlash32Rd(&nor32FlashInfo, NV_BOOT_OFFSET) == 0xFFFFFFFF)
  {
    mvFlashSecLockSet(&nor32FlashInfo,NV_BOOT_OFFSET,MV_FALSE);
    mvFlash32Wr (&nor32FlashInfo, NV_BOOT_OFFSET, EOS);
  }
  return;
#endif
#endif /* MV_INCLUDE_NOR */
#ifdef MV_INCLUDE_SPI
#ifdef BOOTLINE_ON_SPI_FLASH
  int data;
  if (mvSFlashBlockRd(&spiFlashInfo, NV_BOOT_OFFSET,(MV_U8*)&data,4) != MV_OK) 
  {
    mvOsPrintf("\nflashBootSectorInit: failed read SPI mvSFlashBlockRd\n");
    return;
  }
  if (data == 0xFFFFFFFF)
  {
    data = EOS;
    if (mvSFlashBlockWr (&spiFlashInfo, NV_BOOT_OFFSET, (MV_U8*)&data,4) != MV_OK) 
    {
      mvOsPrintf("\n failed write flashBootSectorInit");
      return;
    }
  }
  return;
#endif
#endif /* MV_INCLUDE_SPI */
}
#endif /* defined(INCLUDE_FLASH_BOOTLINE)*/

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
#ifdef INCLUDE_FLASH_BOOTLINE
    int flashSize = 0;
#ifdef MV_INCLUDE_NOR
#ifdef BOOTLINE_ON_NOR32_FLASH
    int rdSize;
#endif
#endif

#ifdef MV_INCLUDE_NOR
#ifdef BOOTLINE_ON_NOR32_FLASH
	flashSize = mvFlashSizeGet(&nor32FlashInfo);
#endif
#endif

#ifdef BOOTLINE_ON_SPI_FLASH
	flashSize = mvSFlashSizeGet(&spiFlashInfo);
#endif
    offset += NV_BOOT_OFFSET;   /* boot line begins at <offset> = 0 */
    
	/* Parameter checking */
    if ((offset < 0) || (strLen < 0))
	{
        mvOsPrintf("sysNvRamGet Failed Parameter checking.(offset=%d,strLen=%d)\n", offset,strLen);
        return ERROR;
	}
    if ((offset + strLen) > flashSize)
    {
        mvOsPrintf("sysNvRamGet: Failed Parameter checking(offset(%d)+size(%d) over the flash size(%d).\n",
				   offset, strLen, flashSize);
        return ERROR;
    }

#ifdef MV_INCLUDE_NOR
#ifdef BOOTLINE_ON_NOR32_FLASH
	rdSize = 	mvFlashBlockRd(&nor32FlashInfo, offset , strLen, string);
	if(0 == rdSize)
	{
        mvOsPrintf("sysNvRamGet: read boot line from NOR flash error.\n");
        return(ERROR);
	}
	if(*BOOT_LINE_ADRS == EOS)
	{
        mvOsPrintf("sysNvRamGet: No boot line burned on NOR flash\n");
        return(ERROR);
	}
    else
	{
		string [strLen] = EOS;
		return OK;
	}
#endif /* defined(BOOTLINE_ON_NOR32_FLASH)  */
#endif /* defined(MV_INCLUDE_NOR)  */

#ifdef MV_INCLUDE_SPI
#ifdef BOOTLINE_ON_SPI_FLASH
	/* Parameter checking */
	if((mvSFlashBlockRd(&spiFlashInfo, NV_BOOT_OFFSET , string,strLen) != MV_OK) || 
	   (*BOOT_LINE_ADRS == EOS)) 
	{
        mvOsPrintf("sysNvRamGet: read boot line from SPI flash error.\n");
		return(ERROR);
	}
    else
	{
		string [strLen] = EOS;
		return OK;
	}
#endif /* (MV_INCLUDE_SPI) */
#endif /* (BOOTLINE_ON_SPI_FLASH)   */
#endif /* INCLUDE_FLASH_BOOTLINE */

#ifndef _DIAB_TOOL
  return ERROR; /* make the compiler happy */
#endif
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
#ifdef INCLUDE_FLASH_BOOTLINE
    int bootLineSector, flashSize = 0;
#ifdef MV_INCLUDE_NOR
#ifdef BOOTLINE_ON_NOR32_FLASH
	MV_STATUS bRet;
	flashSize = mvFlashSizeGet(&nor32FlashInfo);
#endif
#endif
#ifdef BOOTLINE_ON_SPI_FLASH
	flashSize = mvSFlashSizeGet(&spiFlashInfo);
#endif
    offset += NV_BOOT_OFFSET;   /* boot line begins at <offset> = 0 */
    
	/* Parameter checking */
    if ((offset < 0) || (strLen < 0))
    {
        mvOsPrintf("sysNvRamSet Failed Parameter checking.(offset=%d,strLen=%d)\n", offset,strLen);
        return ERROR;
    }
    if ((offset + strLen) > flashSize)
    {
        mvOsPrintf("sysNvRamSet: Failed Parameter checking(offset(%d)+size(%d) over the flash size(%d).\n",
				   offset, strLen, flashSize);
        return ERROR;
    }

#ifdef MV_INCLUDE_NOR
#ifdef BOOTLINE_ON_NOR32_FLASH



	while(1)
	{
		bootLineSector = mvFlashInWhichSec(&nor32FlashInfo, offset);
		if(bootLineSector == FLASH_BAD_SEC_NUM)
		{
			mvOsPrintf("sysNvRamSet Failed to allocate boot line sector\n");
			flashPrint(&nor32FlashInfo);
			bRet =  ERROR;
			break;
		}
		if (MV_OK != mvFlashSecLockSet(&nor32FlashInfo,bootLineSector,MV_FALSE))
		{
			mvOsPrintf("sysNvRamSet Failed to unlock sectore offset 0x%x\n",offset);
			bRet =  ERROR;
			break;
		}
		if(MV_OK != mvFlashSecErase(&nor32FlashInfo ,bootLineSector))
		{
			mvOsPrintf("sysNvRamSet Failed to erase Flash boot sector\n");
			bRet =  ERROR;
			break;
		}
		
		if(0 == mvFlashBlockWr (&nor32FlashInfo, offset , strLen, string))
		{
			mvOsPrintf("sysNvRamSet Failed to write to Flash\n");
			bRet =  ERROR;
			break;
		}
		bRet =  OK;
		break;
	}
    return bRet;
#endif
#endif
#ifdef MV_INCLUDE_SPI 
#ifdef BOOTLINE_ON_SPI_FLASH

	if(mvSFlashWpRegionSet(&spiFlashInfo, MV_WP_NONE) != MV_OK)
	{
		mvOsPrintf("mvSFlashWpRegionSet fail\n");
		return ERROR;
	}

	bootLineSector = offset / spiFlashInfo.sectorSize;
	if(mvSFlashSectorErase(&spiFlashInfo ,bootLineSector) != MV_OK)
	{
		mvOsPrintf("sysNvRamSet Failed to erase Flash boot sector\n");
		return ERROR;
	}

	if(mvSFlashBlockWr (&spiFlashInfo, offset, string, strLen) != MV_OK)
	{
		mvOsPrintf("sysNvRamSet Failed to write to Flash\n");
		return ERROR;
	}

	string[strLen] = EOS;
	return OK;
#endif
#endif
#ifndef _DIAB_TOOL
  return ERROR; /* make the compiler happy */
#endif
#endif /* INCLUDE_FLASH_BOOTLINE */
}
/**********************************************************************************************
* mvPrintsysPhysMemDesc print MMU Table pages
*
***********************************************************************************************/
#if defined(INCLUDE_MMU)
void mvPrintsysPhysMemDesc(void)
{
  int ix;
                
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
void mvDDShow(void)
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

MV_U32 mvConsoleTtyGet(void)
{
#if defined (MV78200)
	if (mvSocUnitIsMappedToThisCpu(UART0))
#endif
	{
		return 0;
	}
#if defined (MV78200)
	if (mvSocUnitIsMappedToThisCpu(UART1))
	{
		return 1;
	}
	return -1;
#endif
}
