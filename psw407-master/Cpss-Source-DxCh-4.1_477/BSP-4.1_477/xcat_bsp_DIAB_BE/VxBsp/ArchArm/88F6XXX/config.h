/* config.h - ARM Integrator configuration header */

/* Copyright 1999-2003 ARM Limited */
/* Copyright 1999-2004 Wind River Systems, Inc. */

/*
modification history
--------------------
01r,26apr04,dmh  support for 926EJ
01q,07feb03,jb   Removing unwanted define
01p,04feb03,jb   Adding ARM10 support
01o,15jul02,m_h  WindML support, C++ protection
01n,22may02,m_h  Reduce ROM_SIZE for boards with 32 meg RAM (77901)
01m,15may02,m_h  INCLUDE_SHELL, etc are for BSP validation (75760, 75904)
01l,09oct01,jpd  corrected RAM_HIGH_ADRS and LOCAL_MEM_SIZE for integrator946.
                 bump revision number to /5
01k,03oct01,jpd  added support for Integrator 946es/946es_t.
01j,02may01,rec  bump revision number, fix 559 initialization problem
01m,01nov01,t_m  merge in 946 updates
01l,22oct01,jb  Setting MMU_BASIC as default for builds of cpus with MMU
01k,15oct01,jb  New assembly macros are in h/arch/arm/arm.h
01l,09oct01,jpd  corrected RAM_HIGH_ADRS and LOCAL_MEM_SIZE for integrator946.
01k,03oct01,jpd  added support for Integrator 946es/946es_t.
01j,02may01,rec  bump revision number, fix 559 initialization problem
01i,27apr01,rec  add support for 966
01h,25jan01,jmb  remove INCLUDE_MIILIB
01g,15dec00,rec  change RAM_HIGH_ADRS
01f,21nov00,jpd  added support for Intel Ethernet driver.
01e,17feb00,jpd  added define of INCLUDE_FLASH_SIB_FOOTER; raised RAM_HIGH_ADRS.
01d,07feb00,jpd  added support for ARM720T and ARM920T.
01c,13jan00,pr   add support for Integrator 740T/740T_T.
01b,07dec99,pr   add DEC and PCI support.
01a,05nov99,ajb  copied from PID BSP version 01p.
*/

/*
This module contains the configuration parameters for the ARM Integrator BSP.
*/

#ifndef INCconfigh
#define INCconfigh

#ifdef __cplusplus
extern "C" {
#endif

/* BSP version/revision identification, before configAll.h */

#ifdef MV_MMU_DISABLE
#undef INCLUDE_MMU_BASIC
#undef INCLUDE_MMU_FULL
#undef INCLUDE_MMU_MPU
#undef INCLUDE_MMU
#endif

#define BSP_VER_1_1     1       /* 1.2 is backwards compatible with 1.1 */
#define BSP_VER_1_2     1
#if defined (WORKBENCH)
    #define BSP_VER	 6
    #define BSP_VERSION     "2.0"
#if (_WRS_VXWORKS_MAJOR == 6) && (_WRS_VXWORKS_MINOR > 2)
#undef SUPPORT_VXW_DOS_FS_B1_0_API
#else
#define SUPPORT_VXW_DOS_FS_B1_0_API
#endif /* _WRS_VXWORKS_ <6.4 */

#else
#define SUPPORT_VXW_DOS_FS_B1_0_API
    #define BSP_VER	 5
    #define BSP_VERSION "1.2"     /* 1.2 for Tornado 2.x                 */
#endif /* WORKBENCH */


#if defined (TORNADO_PID)
#define BSP_REV         "/5.4.0.0017"
#elif defined (TORNADO)
#define BSP_REV         "/5.4.0.0017"
#elif defined (WORKBENCH)
#define BSP_REV         "/5.4.0.0017"
#endif /* TORNADO_PID*/

#define BOOT_CMD_STACK_SIZE	0x4000

#include "configAll.h"        /* Set the VxWorks default configuration */

#ifdef ROOT_STACK_SIZE
#undef ROOT_STACK_SIZE
#define ROOT_STACK_SIZE         0x4000  /* size of root's stack, in bytes */
#endif

#if (_WRS_VXWORKS_MAJOR == 6)
/*#define INCLUDE_DEBUG */
#define INCLUDE_PAGE_SIZE_OPTIMIZATION

#if defined(INCLUDE_USER_APPL)
#define INCLUDE_UNLOADER
#define INCLUDE_NET_INIT
#undef  INCLUDE_FASTUDP
#define INCLUDE_ZBUF_SOCK
#define INCLUDE_SHOW_ROUTINES
#define INCLUDE_NET_SHOW

#define INCLUDE_RTP
#define INCLUDE_RTP_HOOKS
#define INCLUDE_SYSCALL_HOOKS
#define INCLUDE_SHARED_DATA
#define INCLUDE_SC_SYSCTL
#define INCLUDE_SYSCTL
#endif
#endif

/* Enable simulation of unaligned access in exception */
#define INCLUDE_UNALIGNED_ACCESS
#undef  INCLUDE_UNALIGNED_ACCESS

#ifdef INCLUDE_UNALIGNED_ACCESS
    #define UNALIGNED_ACCESS_PC_LOG_ENABLE
    #define UNALIGNED_ACCESS_PC_LOG_LEN 1024 /* should be power of 2 */
#endif


#define INCLUDE_CACHE_SUPPORT

#include "db88F6281.h"
#ifdef BOOT_LINE_OFFSET
#undef BOOT_LINE_OFFSET
#define BOOT_LINE_OFFSET        0x700
#endif
#define DEFAULT_BOOT_LINE                \
       "mgi(0,0) host:VxWorks.st "      \
       "h=10.4.50.105 "                  \
       "e=10.4.50.100:FFFF0000 "         \
       "u=anonymous "                    \
       "pw=target "                      \
       "f=0x0 "

/* memory configuration */

#define LOCAL_MEM_LOCAL_ADRS SDRAM_CS0_BASE /* Start of SDRAM is zero */
#define LOCAL_MEM_AUTOSIZE                  /* run-time memory sizing */
#define LOCAL_MEM_SIZE       0x08000000     /* Total of Target memory */
#define USER_RESERVED_MEM    0              /* see sysMemTop()        */


/*
 * Boot ROM is an image written into Flash. Part of the Flash can be
 * reserved for boot parameters etc. (see the Flash section below).
 *
 * The following parameters are defined here and in the Makefile.
 * They must be kept synchronized; effectively config.h depends on Makefile.
 * Any changes made here must be made in the Makefile and vice versa.
 *
 * ROM_BASE_ADRS is the base of the Flash ROM/EPROM.
 * ROM_TEXT_ADRS is the entry point of the VxWorks image
 * ROM_SIZE is the size of the part of the Flash ROM/EPROM allocated to
 *      the VxWorks image (block size - size of headers)
 *
 * Two other constants are used:
 * ROM_COPY_SIZE is the size of the part of the ROM to be copied into RAM
 *       (e.g. in uncompressed boot ROM)
 * ROM_SIZE_TOTAL is the size of the entire Flash ROM (used in sysPhysMemDesc)
 *
 * The values are given as literals here to make it easier to ensure
 * that they are the same as those in the Makefile.
 */
#if defined(INCLUDE_USER_APPL)
#define INCLUDE_XOR_COPY
#define INCLUDE_I_AND_D_CACHE
#endif

#ifdef MV_INCLUDE_SAGE
#if defined(INCLUDE_USER_APPL)
#define ROM_BASE_ADRS   0xf8240000              /* base address of ROM     */
#define ROM_TEXT_ADRS   0xf8240000 				/* with PC & SP            */
#define ROM_SIZE        0x00300000              /* > 2MB ROM space         */
#else
#define ROM_BASE_ADRS   0xf8140000              /* base address of ROM     */
#define ROM_TEXT_ADRS   0xf8140000 				/* with PC & SP            */
#define ROM_SIZE        0x00070004              /* 512KB ROM space         */
#endif /* INCLUDE_USER_APPL */
#else
#ifdef VIPS_APP
#define ROM_BASE_ADRS   0xf8240000              /* base address of ROM     */
#define ROM_TEXT_ADRS   0xf8240000 				/* with PC & SP            */
#define ROM_SIZE        0x00300000              /* > 2MB ROM space         */
#else
#define ROM_BASE_ADRS   0xf8140000              /* base address of ROM     */
#define ROM_TEXT_ADRS   0xf8140000 				/* with PC & SP            */
#define ROM_SIZE        0x00070004              /* 512KB ROM space         */
#endif
#endif  /* MV_INCLUDE_SAGE */
#define ROM_RESET		(ROM_BASE_ADRS)
#define ROM_WARM_ADRS   (ROM_RESET + 0x004)		/* warm entry              */
#define ROM_CONFIG_ADRS (ROM_RESET + 0x008) 	/* warm entry              */
#define RAM_LOW_ADRS    0x00010000              /* RAM address for vxWorks */
#define RAM_HIGH_ADRS   0x03000000              /* RAM address for bootrom */

#define PCI_MSTR_MEMIO_LOCAL    (PCI_IF0_MEM_BASE + LOCAL_MEM_LOCAL_ADRS)
#define PCI_MSTR_IO_LOCAL       (PCI_IF0_IO_BASE  + LOCAL_MEM_LOCAL_ADRS)
#define PCI_SLV_MEM_LOCAL       (SDRAM_CS0_BASE + LOCAL_MEM_LOCAL_ADRS)

/*
 * Flash/NVRAM memory configuration
 *
 * A block of the Flash memory (FLASH_SIZE bytes at FLASH_ADRS) is
 * reserved for non-volatile storage of data.
 *
 */

/* HW floating point support */
#if defined(INCLUDE_USER_APPL)

#ifdef MV_VFP
#define INCLUDE_VFP
#if defined(MV_CPU_LE)

#   undef INCLUDE_SW_FP
#   define INCLUDE_HW_FP
#elif defined(MV_CPU_BE)
#   define INCLUDE_SW_FP
#   undef INCLUDE_HW_FP
#endif
#endif

#endif /* INCLUDE_USER_APPL  */


/* Flash support */
#define INCLUDE_FLASH

#ifdef INCLUDE_FLASH

#ifdef MV_SPI_BOOT
#define BOOT_LINE_ON_SPI_FLASH
#endif

#ifdef MV_NAND_BOOT
#define BOOT_LINE_ON_NAND_FLASH
#endif

#ifdef MV_NOR_BOOT
#define BOOT_LINE_ON_NOR32_FLASH
#endif

/*
 * to early convert VxWorks boot string to BOOT_PARAMS struct
 * (not for bootrom.bin)
 */
#if defined (BOOT_LINE_ON_SPI_FLASH) && defined (INCLUDE_USER_APPL)
#define MV_VXWORKS_ST
#endif

#   define NV_RAM_SIZE      0x100       /* how much we use as NVRAM */
#   undef  NV_BOOT_OFFSET
#   define NV_BOOT_OFFSET   0x100000    /* bootline at start of NVRAM */
#   define NV_BOOT_SECTOR   4           /* bootline at start of NVRAM */
#   define INCLUDE_FLASH_BOOT_LINE
#else
#define NV_RAM_SIZE     NONE
#endif  /* INCLUDE_FLASH */

/* Serial port configuration */
#define INCLUDE_SERIAL

#ifdef INCLUDE_SERIAL
#undef  NUM_TTY
#define NUM_TTY      N_SIO_CHANNELS
#undef  CONSOLE_TTY
#define CONSOLE_TTY  0              /* console channel */

/* Console baud rate reconfoguration. */
#undef  CONSOLE_BAUD_RATE
#define CONSOLE_BAUD_RATE 115200   /* Reconfigure default baud rate */
#endif

/* Modify system clock tick to 1ms to get better timing resolution */
/*
#undef  SYS_CLK_RATE
#define SYS_CLK_RATE    1000
*/

/* Define SERIAL_DEBUG to enable debugging via the serial ports */
#undef SERIAL_DEBUG
#ifdef SERIAL_DEBUG

#   undef WDB_COMM_TYPE
#   undef WDB_TTY_BAUD
#   undef WDB_TTY_CHANNEL

#   define WDB_COMM_TYPE       WDB_COMM_SERIAL /* WDB in Serial mode */
#   define WDB_TTY_BAUD        115200          /* Baud rate for WDB Connection*/
#   define WDB_TTY_CHANNEL     1               /* COM PORT #2 */
#endif /* SERIAL_DEBUG */

/*
 * Cache/MMU configuration
 *
 * Note that when MMU is enabled, cache modes are controlled by
 * the MMU table entries in sysPhysMemDesc[], not the cache mode
 * macros defined here.
 */

/*
 * sysPhysMemDesc[] dummy entries:
 * these create space for updating sysPhysMemDesc table at a later stage
 * mainly to provide plug and play
 */


/*
 * We use the generic architecture libraries, with caches/MMUs present. A
 * call to sysHwInit0() is needed from within usrInit before
 * cacheLibInit() is called.
 */

#ifndef _ASMLANGUAGE
IMPORT void sysHwInit0 (void);
#endif
#define INCLUDE_SYS_HW_INIT_0
#define SYS_HW_INIT_0()         sysHwInit0 ()

/*
 * 920T/940T/946ES I-cache mode is a bit of an inappropriate concept,
 * but use this.
 * */

#define USER_I_CACHE_ENABLE                /* Enable ICACHE */
#undef  USER_I_CACHE_MODE
#define USER_I_CACHE_MODE       CACHE_WRITETHROUGH

/* 920T/926EJ/940T/946ES has to be this. */

#define USER_D_CACHE_ENABLE                /* Enable DCACHE */
#undef  USER_D_CACHE_MODE
#define USER_D_CACHE_MODE       CACHE_COPYBACK


#if defined(CPU_940T) || defined(CPU_940T_T) || \
    defined(CPU_926EJ) || defined(CPU_926EJ_T)
/*
 * All ARM 940T and 926EJ BSPs must define a variable sysCacheUncachedAdrs:
 * a pointer to a word that is uncached and is safe to read (i.e. has no
 * side effects).  This is used by the cacheLib code to perform a read
 * (only) to drain the write-buffer. Clearly this address must be present
 * within one of the regions created within sysPhysMemDesc, where it must
 * be marked as non-cacheable. There are many such addresses we could use
 * on the board, but we choose to use an address here that will be
 * mapped in on just about all configurations: a safe address within the
 * interrupt controller: the IRQ Enabled status register. This saves us
 * from having to define a region just for this pointer. This constant
 * defined here is used to initialise sysCacheUncachedAdrs in sysLib.c
 * and is also used by the startup code in sysALib.s and romInit.s in
 * draining the write-buffer.
 */

#define SYS_CACHE_UNCACHED_ADRS     (INTER_REGS_BASE + 0x20200)
#endif /* defined(CPU_940T/940T_T/926EJ/926EJ_T) */



/*
 * Network driver configuration.
 *
 * De-select unused (default) network drivers selected in configAll.h.
 */
#define INCLUDE_GTF_TIMER_START
#define INCLUDE_GTF
#define INCLUDE_INETLIB          /* inetLib */
#define INCLUDE_IPATTACH
#define INCLUDE_IPPING_CMD         /* ping */
#define INCLUDE_NET_BOOT_CONFIG  /* network boot device configuration */
#define  INCLUDE_NET_SYSCTL      /* Network sysctl tree support */
#define INCLUDE_IPWRAP_PING
#define INCLUDE_XDR

#define INCLUDE_IFCONFIG      /* old coreip stack ifconfig command line/API */
#define INCLUDE_ROUTECMD
#define INCLUDE_NETWORK
#define INCLUDE_NET_DRV
#define INCLUDE_NET_LIB
#define INCLUDE_NET_REM_IO
#define INCLUDE_NET_SYM_TBL

#if defined(INCLUDE_USER_APPL)

#define INCLUDE_NET_SHOW
#define INCLUDE_NET_HOST_SHOW    /* Host show routines */
#define INCLUDE_NET_ROUTE_SHOW   /* Routing show routines */
#define INCLUDE_ICMP_SHOW       /* ICMP show routines */
#define INCLUDE_IGMP_SHOW       /* IGMP show routines */
#define INCLUDE_TCP_SHOW        /* TCP show routines */
#define INCLUDE_UDP_SHOW        /* UDP show routines */

#undef  INCLUDE_ENP     /* include CMC Ethernet interface*/
#undef  INCLUDE_EX      /* include Excelan Ethernet interface */
#undef  INCLUDE_SM_NET      /* include backplane net interface */
#undef  INCLUDE_SM_SEQ_ADDR /* shared memory network auto address setup */
#endif /* INCLUDE_USER_APPL */

/* Enhanced Network Driver (END) Support */

#define INCLUDE_END           /* Enhanced Network Driver (see configNet.h) */
#undef  INCLUDE_BSD           /* BSD 4.4 drivers - Not supported */

#ifdef INCLUDE_END
#   define INCLUDE_MGI_END
#   define MGI_TASK_LOCK
#	undef  MGI_SEMAPHORE

#   define SYS_FEI_OFFSET_VALUE         2 /* ARM have alignment  restriction     */
#	define FEI_MAX_UNITS				4
#   define GEI82543_OFFSET_VALUE        2 /* ARM have alignment  restriction     */
#   define GEI82543_USER_SHARED_MEM
#   define GEI_MAX_UNITS                4

#define SWITCH_END_PP_IFACE_SDMA        0
#define SWITCH_END_PP_IFACE_MII         1

#if defined(MV_SWITCH_PORTS) && !defined(VIPS_APP) && !defined(PSS_MODE)
#define MUX_MAX_BINDS 32
#define INCLUDE_SWITCH_END
#define MV_USE_END_SWITCH_CPU           0x10000
#define MV_USE_END_SWITCH_SDMA          0x30000
#define MV_USE_END_SWITCH_TX_MODE       0x40000
#if 1
#   define SWITCH_TASK_LOCK
#else
#	define SWITCH_SEMAPHORE
#endif
#endif


#   define SYS_SGI_OFFSET_VALUE         2
#   define SGI_MAX_UNITS                4 /* Maximum device units */

#if defined(INCLUDE_USER_APPL)
#   ifndef SERIAL_DEBUG
#       undef  WDB_COMM_TYPE        /* WDB agent communication path is END */
#       define WDB_COMM_TYPE    WDB_COMM_END
#       define WBD_AGENT_END
#       define INCLUDE_WDB
#   else
#       undef WBD_AGENT_END
#   endif /* SERIAL_DEBUG */
#endif
#endif  /* INCLUDE_END */

/* Optional drivers support */
#define INCLUDE_PCI

#if defined(INCLUDE_USER_APPL)
/* PCI driver - see sysBusPci.c for more information */

/* timestamp support for WindView - undefined by default */
#define  INCLUDE_TIMESTAMP
#define  INCLUDE_USER_TIMESTAMP
/* Optional usage of Auxiliary clock */
#define INCLUDE_AUXCLK

#   define INCLUDE_PING
#   define INCLUDE_TELNET
#   define INCLUDE_ARP_API
#   define INCLUDE_NET_SHOW        /* IP show routines */
#   define INCLUDE_NET_IF_SHOW     /* Interface show routines */
#   define BSP_VTS

#endif

/*
 * Interrupt mode - interrupts can be in either preemptive or non-preemptive
 * mode.  For non-preemptive mode, change INT_MODE to INT_NON_PREEMPT_MODEL
 */

#define INT_MODE    INT_PREEMPT_MODEL

/*
 * Enable BSP-configurable interrupt priorities: order of servicing and
 * masking of interrupts will be determined by ambaIntLvlPriMap[] in
 * sysLib.c.  If AMBA_INT_PRIORITY_MAP is not defined, priority of
 * interrupts will be least-significant bit first.
 */

#define AMBA_INT_PRIORITY_MAP   /* BSP-configurable interrupt priorities */


/*
 * miscellaneous definitions
 * Note: ISR_STACK_SIZE is defined here rather than in ../all/configAll.h
 * (as is more usual) because the stack size depends on the interrupt
 * structure of the BSP.
 */

#define ISR_STACK_SIZE  0x800   /* size of ISR stack, in bytes */


/*
 * If we are building a ".st" image (vxWorks.st, vxWorks.st_rom, etc)
 * then STANDALONE is defined, and if WDB is using a network device
 * like netif or END then we need to define STANDALONE_NET to start
 * the network. SPR#23450.  Not needed in T2.
 */

#if defined (STANDALONE)
#   define STANDALONE_NET
#endif /* STANDALONE */


/*
 * User application initialization
 *
 * USER_APPL_INIT must be a valid C statement or block. It is included
 * in the usrRoot() routine only if INCLUDE_USER_APPL is defined.
 * The code for USER_APPL_INIT is only an example.
 * The user is expected to change it as needed.
 * The use of taskSpawn is recommended over direct execution of the user
 * routine.
 */

#if (_WRS_VXWORKS_MAJOR == 6)
    #define USER_APPL_INIT          \
    {                               \
        IMPORT int mvUserAppInit(); \
        mvUserAppInit();            \
    }
#elif defined (VIPS_APP)
    #define USER_APPL_INIT     \
    {                          \
       IMPORT int mvAppInit(); \
       mvAppInit();            \
    }
#elif defined (PSS_MODE)
    #define USER_APPL_INIT        \
    {                             \
        IMPORT int userAppInit(); \
        userAppInit();            \
    }
#else
    #define USER_APPL_INIT        \
    {                             \
        IMPORT int userAppInit(); \
        userAppInit();            \
        /* taskSpawn ("userApp", 30, 0, 5120, userAppInit, 0x0, 0x1); */ \
    }
#endif

#define INCLUDE_SHELL
#define INCLUDE_NET_SYM_TBL
#define INCLUDE_STAT_SYM_TBL
#define INCLUDE_LOADER



#ifdef BSP_VTS

/***************************************************
 * Add these defines for the Validation Test Suite *
 ***************************************************/
#define INCLUDE_RLOGIN
#define INCLUDE_DEBUG

#define INCLUDE_SHOW_ROUTINES

#define INCLUDE_NET_HOST_SHOW    /* Host show routines */
#define INCLUDE_NET_ROUTE_SHOW   /* Routing show routines */
#define INCLUDE_ICMP_SHOW       /* ICMP show routines */
#define INCLUDE_IGMP_SHOW       /* IGMP show routines */
#define INCLUDE_TCP_SHOW        /* TCP show routines */
#define INCLUDE_UDP_SHOW        /* UDP show routines */

#define INCLUDE_TIMESTAMP   /* define to include timestamp driver */

#endif /*BSP_VTS*/



#ifndef _ASMLANGUAGE

/* This structure describes a user ISR entry for  the various interrupt     */
/* controllers drivers throughout the Marvell controller device             */

typedef struct _mvIsrEntry
{
    UINT32      causeBit;  /* A specific cause bit in the cause register.*/
    VOIDFUNCPTR userISR;   /* A user ISR pointer.*/
    UINT32      arg1;      /* First argument to interrupt ISR. */
    UINT32      arg2;      /* Second argument to interrupt ISR. */
    UINT32      prio;      /* An interrupt priority. */
} MV_ISR_ENTRY;
#endif /* _ASMLANGUAGE */


#ifdef __cplusplus
}
#endif
#endif  /* INCconfigh */

#if defined(PRJ_BUILD)
#include "prjParams.h"
#endif

#if defined(INCLUDE_USER_APPL) /*	not for bootrom */

#   if NUM_DAT_64 < 256
#       undef NUM_DAT_64
#       define NUM_DAT_64  256
#   endif

#   if NUM_DAT_128 < 1024
#       undef NUM_DAT_128
#       define NUM_DAT_128  1024
#   endif

        /* The following components are required */
        #define INCLUDE_DOSFS           /* usrDosFsOld.c wrapper layer      */
        #define INCLUDE_DOSFS_MAIN      /* dosFsLib (2)                     */
        #define INCLUDE_DOSFS_FAT       /* dosFs FAT12/16/32 FAT handler    */
        #define INCLUDE_FS_MONITOR

        #define INCLUDE_CBIO            /* CBIO API module                  */
        #define INCLUDE_DOSFS_DIR_VFAT  /* Microsoft VFAT direct handler    */
        #define INCLUDE_DOSFS_DIR_FIXED /* Strict 8.3 & VxLongNames directory handler */

        /* Optional dosFs components */
        #define INCLUDE_DOSFS_FMT       /* dosFs2 file system formatting module */
        #define INCLUDE_DOSFS_CHKDSK    /* file system integrity checking   */
        #define INCLUDE_DOSFS_CACHE

        /* standard file system operations: ls, cd, mkdir, xcopy and so on */
        #define INCLUDE_DISK_UTIL

        /* Optional CBIO Components */
        #define INCLUDE_DISK_CACHE      /* CBIO API disk caching layer      */
        #define INCLUDE_DISK_PART       /* disk partition handling code     */
        #define INCLUDE_RAM_DISK        /* CBIO API RAM disk driver         */
        /* Optional XBD Components  */
        #define INCLUDE_XBD_BLK_DEV
        #ifndef SUPPORT_VXW_DOS_FS_B1_0_API
                #define INCLUDE_SCSI_DOSFS
                #define INCLUDE_XBD 			/* XBD component                    */
        #endif
        #define INCLUDE_XBD_PART_LIB 	/* disk partitioning facilities 	*/
        #define INCLUDE_XBD_BLK_DEV 	/* XBD wrapper component for device drivers that have not been ported to XBD */
        #define INCLUDE_RAWFS
        #ifdef  NUM_RAWFS_FILES
                #undef NUM_RAWFS_FILES
                #define NUM_RAWFS_FILES 10
        #endif

#endif  /* INCLUDE_USER_APPL) not for bootrom */

#define NUMBER_OF_IP_PORTS  IP_MAX_UNITS


#ifdef MV_INCLUDE_CESA

#define SADB_TASK_PRIORITY		40
#define IKE_TASK_PRIORITY 		41
#undef  NUM_DAT_64
#undef  NUM_DAT_128
#undef  NUM_DAT_256
#undef  NUM_DAT_512
#undef  NUM_DAT_1024

#undef  NUM_DAT_2048
#undef  NUM_DAT_8192
#undef  NUM_DAT_16384
#undef  NUM_DAT_32768
#undef  NUM_DAT_65536


#define NUM_DAT_64              256
#define NUM_DAT_128            1024
#define NUM_DAT_256             256
#define NUM_DAT_512             256
#define NUM_DAT_1024            100

#define NUM_DAT_2048           1024
#define NUM_DAT_8192            512
#define NUM_DAT_16384		    512
#define NUM_DAT_32768		    256
#define NUM_DAT_65536		    100

#undef NET_JOB_NUM_CFG
#define NET_JOB_NUM_CFG     180

 #if defined(INCLUDE_USER_APPL) /*	not for bootrom */
#if defined (TORNADO_PID) || defined(MV_USB_STACK_VXW)

#undef IPFORWARDING_CFG
#define IPFORWARDING_CFG    TRUE

#define INCLUDE_FASTPATH
#undef  FASTFORWARDING_CFG
#define FASTFORWARDING_CFG TRUE

#define FF_IPV4_INIT_RTN        ipFFInit
#define FF_IPV4_FIB_DISP_TBL    ptRibDispatchTable

#endif /* TORNADO_PID  or vxWorks_major == 6 */
#endif /* defined(INCLUDE_USER_APPL) 	not for bootrom */
#endif /* MV_SATA_SUPPORT */


 #if defined(INCLUDE_USER_APPL) /*	not for bootrom */
/* 0 - Device, 1 - Host */
#define USB_DEVICE	0
#define USB_HOST	1

#ifdef MV_USB_STACK_VXW

#ifndef _ASMLANGUAGE

void    usbPciByteSet (UINT8 busNo, UINT8 deviceNo, UINT8 funcNo,
                        UINT16 regOffset, UINT8 data);

void    usbPciWordSet (UINT8 busNo, UINT8 deviceNo, UINT8 funcNo,
                        UINT16 regOffset, UINT16 data);

void    usbPciDwordSet (UINT8 busNo, UINT8 deviceNo, UINT8 funcNo,
                        UINT16 regOffset, UINT32 data);

#define USB_PCI_CFG_BYTE_OUT(busNo, deviceNo, funcNo, offset, data) \
            usbPciByteSet(busNo, deviceNo, funcNo, offset, data)

#define USB_PCI_CFG_WORD_OUT(busNo, deviceNo, funcNo, offset, data) \
            usbPciWordSet(busNo, deviceNo, funcNo, offset, data)

#define USB_PCI_CFG_DWORD_OUT(busNo, deviceNo, funcNo, offset, data) \
            usbPciDwordSet(busNo, deviceNo, funcNo, offset, data)

#endif /* _ASMLANGUAGE */

#define INCLUDE_USB
#define INCLUDE_USB_INIT

#define BULK_MAX_DEVS 6

#define INCLUDE_EHCI
#define INCLUDE_EHCI_INIT

#define INCLUDE_UHCI
#define INCLUDE_UHCI_INIT

#define INCLUDE_OHCI
#define INCLUDE_OHCI_INIT

#define INCLUDE_USB_KEYBOARD
#define INCLUDE_USB_KEYBOARD_INIT

#define INCLUDE_USB_MOUSE
#define INCLUDE_USB_MOUSE_INIT

#define INCLUDE_USB_MS_BULKONLY
#define INCLUDE_USB_MS_BULKONLY_INIT
/*
#define INCLUDE_USB_MS_CBI
#define INCLUDE_USB_MS_CBI_INIT
*/
#define INCLUDE_USB_PEGASUS_END
#define INCLUDE_USB_PEGASUS_END_INIT

/*  USB Parameters */
#define BULK_DRIVE_NAME "/bd"                /* Bulk Drive Name */
#define CBI_DRIVE_NAME "/cbid"               /* CBI Drive Name */

#define PEGASUS_IP_ADDRESS          {"10.4.50.205"}     /* Pegasus IP Address */
#define PEGASUS_DESTINATION_ADDRESS {"10.4.50.200"}     /* Pegasus Destination Address */
#define PEGASUS_NET_MASK            {0xffffff00}          /* Pegasus Net Mask */
#define PEGASUS_TARGET_NAME         "host"              /* Pegasus Target Name */

#define INCLUDE_SIO

#endif /*MV_USB_STACK_VXW*/

#ifdef MV_SATA_SUPPORT
#define TOTAL_SATA_DEVICE 	6
#define TOTAL_SCSI_CTRL 	6

/* Scsi2 Support */

#ifndef INCLUDE_SCSI
#define INCLUDE_SCSI
#endif
#ifndef INCLUDE_SCSI2
#define INCLUDE_SCSI2
#endif
#ifndef INCLUDE_SCSI_DMA
#define INCLUDE_SCSI_DMA
#endif

/* NFS support */
#if defined(TORNADO_PID) || defined (WORKBENCH)
#   define INCLUDE_NFS_SERVER_ALL
#else
#   define INCLUDE_NFS_SERVER
#endif /* TORNADO_PID */

#define INCLUDE_HOST_TBL
#define INCLUDE_NFS

#ifndef NFS_USER_ID
#define NFS_USER_ID			500
#endif

#ifndef NFS_GROUP_ID
#define NFS_GROUP_ID		500
#endif

#ifndef NFS_MAXPATH
#define NFS_MAXPATH			255
#endif

#ifndef NFS_MAXFILENAME
#define NFS_MAXFILENAME 	NAME_MAX
#endif

#endif /* MV_SATA_SUPPORT */

#endif /* defined(INCLUDE_USER_APPL) -	not for bootrom */



