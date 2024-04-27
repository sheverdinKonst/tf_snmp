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
/* sysGei82543End.c - Intel 82540/82541/82543/82544/82545/82546 END support routine */
/*                    Intel Pro1000 F/T Adapter END driver support routines */

/* Copyright 1989-2001 Wind River Systems, Inc.  */
/* Copyright 2001 Motorola, Inc. All Rights Reserved */

#include "copyright_wrs.h"

/*
modification history
--------------------
01e,10jul02,dtr  SPR 79571. Slight API change to an unused function 
                 sys543IntEnable/Disable.
01d,28nov01,dtr  Tidy up.
01c,25Nov01,dtr  Modified sys543LocalToPciBus etc for new PCI configuration.
01b,31jul01,srr  Added support for PrPMC-G.
01a,08Jan01,jln  written based on sysNetif.c.
*/

/*
This module is BSP support for Intel PRO1000 F/T adaptors. 

SEE ALSO: ifLib,
.I "RS82543GC GIGABIT ETHERNET CONTROLLER NETWORKING SILICON DEVELOPER'S MANUAL"
*/

#include "vxWorks.h"
#include "taskLib.h"
#include "sysLib.h"
#include "config.h"
#include "end.h"
#include "vmLib.h"
#include "intLib.h"
#include "drv/pci/pciIntLib.h"
#include "drv/pci/pciConfigLib.h"
#include "drv/end/gei82543End.h"
#define INTEL_82543GC_FLOP     			0x103C  /* FLASH Opcode Register */
#define INTEL_82543GC_HICR      		0x08F00 /* Host Inteface Control */
#define INTEL_82543GC_EEWR     			0x102C  /* EEPROM Write Register - RW */
#define INTEL_82543GC_EERD              0x14    /* shortcut EEPROM read access */

#include "mvPciIf.h"
#include "mvCpuIf.h"
#include "mvOs.h"
#include "sysPciIntCtrl.h"

/* defines */
/*-----------------------------------------------------------------------------------------*/
/* additional definitions for Intel 82572 */
#define PRO1000_572_BOARD               0x107d /* 82572 MAC */
#define INTEL_82543GC_PBS      			0x01008  /* Packet Buffer Size */
#define INTEL_82543GC_EEMNGCTL 			0x01010  /* MNG EEprom Control */
#define INTEL_82543GC_FLASH_UPDATES 	1000
#define INTEL_82543GC_EEARBC   			0x1024  /* EEPROM Auto Read Bus Control */
#define INTEL_82543GC_FLASHT   			0x1028  /* FLASH Timer Register */
#define INTEL_82543GC_EEWR     			0x102C  /* EEPROM Write Register - RW */
#define INTEL_82543GC_FLSWCTL  			0x1030  /* FLASH control register */
#define INTEL_82543GC_FLSWDATA 			0x1034  /* FLASH data register */
#define INTEL_82543GC_FLSWCNT  			0x1038  /* FLASH Access Counter */
#define INTEL_82543GC_FLOP     			0x103C  /* FLASH Opcode Register */
#define INTEL_82543GC_ERT      			0x2008  /* Early Rx Threshold - RW */

#define INTEL_82543GC_GCR       		0x05B00 /* PCI-Ex Control */
#define INTEL_82543GC_GSCL_1    		0x05B10 /* PCI-Ex Statistic Control #1 */
#define INTEL_82543GC_GSCL_2    		0x05B14 /* PCI-Ex Statistic Control #2 */
#define INTEL_82543GC_GSCL_3    		0x05B18 /* PCI-Ex Statistic Control #3 */
#define INTEL_82543GC_GSCL_4    		0x05B1C /* PCI-Ex Statistic Control #4 */
#define INTEL_82543GC_FACTPS    		0x05B30 /* Function Active and Power State to MNG */
#define INTEL_82543GC_SWSM      		0x05B50 /* SW Semaphore */
#define INTEL_82543GC_FWSM      		0x05B54 /* FW Semaphore */
#define INTEL_82543GC_FFLT_DBG  		0x05F04 /* Debug Register */
#define INTEL_82543GC_HICR      		0x08F00 /* Host Inteface Control */
/*-----------------------------------------------------------------------------------------*/


#if (defined(INCLUDE_GEI_END) && defined (INCLUDE_NETWORK)	\
     && defined (INCLUDE_END))

#define DEBUG 

#ifdef DEBUG 
#define DEBUGFUNC(x) printf(x)
#else
#define DEBUGFUNC(x)
#endif

#ifdef I82543_DEBUG
#   undef    LOCAL
#   define    LOCAL
#endif    /* I82543_DEBUG */

/* include PCI Library */

#ifndef INCLUDE_PCI
#define INCLUDE_PCI
#endif /* INCLUDE_PCI */

/* PCI configuration type */
 
#ifndef PCI_CFG_TYPE
#define PCI_CFG_TYPE               PCI_CFG_NONE
#endif


/* Default User's flags  */
#define GEI_USR_FLAG               0 /*(GEI_END_FREE_RESOURCE_DELAY)*/

#define GEI_RXDES_NUM               128
#define GEI_TXDES_NUM               256

/* Default RX descriptor  */

#ifndef GEI_RXDES_NUM
#define GEI_RXDES_NUM              GEI_DEFAULT_RXDES_NUM
#endif

/* Default TX descriptor  */

#ifndef GEI_TXDES_NUM
#define GEI_TXDES_NUM              GEI_DEFAULT_TXDES_NUM
#endif

										 
#ifndef GEI_USR_FLAG
#define GEI_USR_FLAG               GEI_DEFAULT_USR_FLAG
#endif

/* Adding support for Intel PRO1000XT based on 82544EI */

/* PCI Device IDs */
#define DEV_ID_82542               0x1000
#define DEV_ID_82543GC_FIBER       0x1001
#define DEV_ID_82543GC_COPPER      0x1004
#define DEV_ID_82544EI_COPPER      0x1008
#define DEV_ID_82544EI_FIBER       0x1009
#define DEV_ID_82544GC_COPPER      0x100C
#define DEV_ID_82544GC_LOM         0x100D
#define DEV_ID_82540EM             0x100E
#define DEV_ID_82540EM_LOM         0x1015
#define DEV_ID_82540EP_LOM         0x1016
#define DEV_ID_82540EP             0x1017
#define DEV_ID_82540EP_LP          0x101E
#define DEV_ID_82545EM_COPPER      0x100F
#define DEV_ID_82545EM_FIBER       0x1011
#define DEV_ID_82545GM_COPPER      0x1026
#define DEV_ID_82545GM_FIBER       0x1027
#define DEV_ID_82545GM_SERDES      0x1028
#define DEV_ID_82546EB_COPPER      0x1010
#define DEV_ID_82546EB_FIBER       0x1012
#define DEV_ID_82546EB_QUAD_COPPER 0x101D
#define DEV_ID_82541EI             0x1013
#define DEV_ID_82541EI_MOBILE      0x1018
#define DEV_ID_82541ER             0x1078
#define DEV_ID_82547GI             0x1075
#define DEV_ID_82541GI             0x1076
#define DEV_ID_82541GI_MOBILE      0x1077
#define DEV_ID_82541GI_LF          0x107C
#define DEV_ID_82546GB_COPPER      0x1079
#define DEV_ID_82546GB_FIBER       0x107A
#define DEV_ID_82546GB_SERDES      0x107B

#define DEV_ID_82547EI             0x1019
#define DEV_ID_82571EB_COPPER      0x105E
#define DEV_ID_82571EB_FIBER       0x105F
#define DEV_ID_82571EB_SERDES      0x1060
#define DEV_ID_82572EI_COPPER      0x107D
#define DEV_ID_82572EI_FIBER       0x107E
#define DEV_ID_82572EI_SERDES      0x107F
#define DEV_ID_82573E              0x108B
#define DEV_ID_82573E_IAMT         0x108C
#define DEV_ID_82573L              0x109A

/* EEPROM Size definitions */
#define EEPROM_WORD_SIZE_SHIFT  6
#define EEPROM_SIZE_SHIFT       10
#define EEPROM_SIZE_MASK        0x1C00

/* EEPROM/Flash Control */
#define E1000_EECD_SK        0x00000001 /* EEPROM Clock */
#define E1000_EECD_CS        0x00000002 /* EEPROM Chip Select */
#define E1000_EECD_DI        0x00000004 /* EEPROM Data In */
#define E1000_EECD_DO        0x00000008 /* EEPROM Data Out */
#define E1000_EECD_FWE_MASK  0x00000030
#define E1000_EECD_FWE_DIS   0x00000010 /* Disable FLASH writes */
#define E1000_EECD_FWE_EN    0x00000020 /* Enable FLASH writes */
#define E1000_EECD_FWE_SHIFT 4
#define E1000_EECD_REQ       0x00000040 /* EEPROM Access Request */
#define E1000_EECD_GNT       0x00000080 /* EEPROM Access Grant */
#define E1000_EECD_PRES      0x00000100 /* EEPROM Present */
#define E1000_EECD_SIZE      0x00000200 /* EEPROM Size (0=64 word 1=256 word) */
#define E1000_EECD_ADDR_BITS 0x00000400 /* EEPROM Addressing bits based on type
                                         * (0-small, 1-large) */
#define E1000_EECD_TYPE      0x00002000 /* EEPROM Type (1-SPI, 0-Microwire) */
#ifndef E1000_EEPROM_GRANT_ATTEMPTS
#define E1000_EEPROM_GRANT_ATTEMPTS 1000 /* EEPROM # attempts to gain grant */
#endif

#define E1000_EECD_AUTO_RD          0x00000200  /* EEPROM Auto Read done */
#define E1000_EECD_SIZE_EX_MASK     0x00007800  /* EEprom Size */
#define E1000_EECD_SIZE_EX_SHIFT    11
#define E1000_EECD_NVADDS    0x00018000 /* NVM Address Size */
#define E1000_EECD_SELSHAD   0x00020000 /* Select Shadow RAM */
#define E1000_EECD_INITSRAM  0x00040000 /* Initialize Shadow RAM */
#define E1000_EECD_FLUPD     0x00080000 /* Update FLASH */
#define E1000_EECD_AUPDEN    0x00100000 /* Enable Autonomous FLASH update */
#define E1000_EECD_SHADV     0x00200000 /* Shadow RAM Data Valid */
#define E1000_EECD_SEC1VAL   0x00400000 /* Sector One Valid */
#define E1000_EECD_SECVAL_SHIFT      22
#define E1000_STM_OPCODE     0xDB00
#define E1000_HICR_FW_RESET  0xC0

/* EEPROM Read */
#define E1000_EERD_START      0x00000001 /* Start Read */
#define E1000_EERD_DONE       0x00000010 /* Read Done */
#define E1000_EERD_ADDR_SHIFT 8
#define E1000_EERD_ADDR_MASK  0x0000FF00 /* Read Address */
#define E1000_EERD_DATA_SHIFT 16
#define E1000_EERD_DATA_MASK  0xFFFF0000 /* Read Data */

/* SPI EEPROM Status Register */
#define EEPROM_STATUS_RDY_SPI  0x01
#define EEPROM_STATUS_WEN_SPI  0x02
#define EEPROM_STATUS_BP0_SPI  0x04
#define EEPROM_STATUS_BP1_SPI  0x08
#define EEPROM_STATUS_WPEN_SPI 0x80

/* EEPROM Commands - Microwire */
#define EEPROM_READ_OPCODE_MICROWIRE  0x6  /* EEPROM read opcode */
#define EEPROM_WRITE_OPCODE_MICROWIRE 0x5  /* EEPROM write opcode */
#define EEPROM_ERASE_OPCODE_MICROWIRE 0x7  /* EEPROM erase opcode */
#define EEPROM_EWEN_OPCODE_MICROWIRE  0x13 /* EEPROM erase/write enable */
#define EEPROM_EWDS_OPCODE_MICROWIRE  0x10 /* EEPROM erast/write disable */

/* EEPROM Commands - SPI */
#define EEPROM_MAX_RETRY_SPI    5000 /* Max wait of 5ms, for RDY signal */
#define EEPROM_READ_OPCODE_SPI  0x3  /* EEPROM read opcode */
#define EEPROM_WRITE_OPCODE_SPI 0x2  /* EEPROM write opcode */
#define EEPROM_A8_OPCODE_SPI    0x8  /* opcode bit-3 = address bit-8 */
#define EEPROM_WREN_OPCODE_SPI  0x6  /* EEPROM set Write Enable latch */
#define EEPROM_WRDI_OPCODE_SPI  0x4  /* EEPROM reset Write Enable latch */
#define EEPROM_RDSR_OPCODE_SPI  0x5  /* EEPROM read Status register */
#define EEPROM_WRSR_OPCODE_SPI  0x1  /* EEPROM write Status register */


#define E1000_EEPROM_SWDPIN0   0x0001   /* SWDPIN 0 EEPROM Value */
#define E1000_EEPROM_LED_LOGIC 0x0020   /* Led Logic Word */
#define E1000_EEPROM_RW_REG_DATA   16   /* Offset to data in EEPROM read/write registers */
#define E1000_EEPROM_RW_REG_DONE   2    /* Offset to READ/WRITE done bit */
#define E1000_EEPROM_RW_REG_START  1    /* First bit for telling part to start operation */
#define E1000_EEPROM_RW_ADDR_SHIFT 2    /* Shift to the address bits */
#define E1000_EEPROM_POLL_WRITE    1    /* Flag for polling for write complete */
#define E1000_EEPROM_POLL_READ     0    /* Flag for polling for read complete */

	/* EEPROM Word Offsets */
#define EEPROM_COMPAT                 0x0003
#define EEPROM_ID_LED_SETTINGS        0x0004
#define EEPROM_VERSION                0x0005
#define EEPROM_SERDES_AMPLITUDE       0x0006 /* For SERDES output amplitude adjustment. */
#define EEPROM_PHY_CLASS_WORD         0x0007
#define EEPROM_INIT_CONTROL1_REG      0x000A
#define EEPROM_INIT_CONTROL2_REG      0x000F
#define EEPROM_INIT_CONTROL3_PORT_B   0x0014
#define EEPROM_INIT_CONTROL3_PORT_A   0x0024
#define EEPROM_CFG                    0x0012
#define EEPROM_FLASH_VERSION          0x0032
#define EEPROM_CHECKSUM_REG           0x003F



#define GEI0_MEMBASE0_LOW           0xfd000000    /* mem base for CSR */
#define GEI0_MEMBASE0_HIGH          0x00000000    /* mem base for CSR */
#define GEI0_MEMSIZE0               0x20000       /* mem size - CSR,128KB */
#define GEI0_MEMBASE1               0xfd100000    /* mem base - Flash */
#define GEI0_MEMSIZE1               0x00080000    /* mem size - Flash,512KB */
#define GEI0_INT_LVL                0x0b          /* IRQ 11 */
#define GEI0_INIT_STATE_MASK        (NONE)
#define GEI0_INIT_STATE             (NONE)
#define GEI0_SHMEM_BASE             NONE
#define GEI0_SHMEM_SIZE             0
#define GEI0_RXDES_NUM              GEI_RXDES_NUM
#define GEI0_TXDES_NUM              GEI_TXDES_NUM
#define GEI0_USR_FLAG               GEI_USR_FLAG

#define GEI1_MEMBASE0_LOW           0xfd200000    /* mem base for CSR */
#define GEI1_MEMBASE0_HIGH          0x00000000    /* mem base for CSR */
#define GEI1_MEMSIZE0               0x20000       /* mem size - CSR,128KB */
#define GEI1_MEMBASE1               0xfd300000    /* mem base for Flash */
#define GEI1_MEMSIZE1               0x00080000    /* mem size - Flash,512KB */
#define GEI1_INT_LVL                0x05          /* IRQ 5 */
#define GEI1_INIT_STATE_MASK        (NONE)
#define GEI1_INIT_STATE             (NONE)
#define GEI1_SHMEM_BASE             NONE
#define GEI1_SHMEM_SIZE             0
#define GEI1_RXDES_NUM              GEI_RXDES_NUM
#define GEI1_TXDES_NUM              GEI_TXDES_NUM
#define GEI1_USR_FLAG               GEI_USR_FLAG

#define GEI2_MEMBASE0_LOW           0xfd400000    /* mem base - CSR */
#define GEI2_MEMBASE0_HIGH          0x00000000    /* mem base - CSR */
#define GEI2_MEMSIZE0               0x20000       /* mem size - CSR, 128KB */
#define GEI2_MEMBASE1               0xfd500000    /* mem base - Flash */
#define GEI2_MEMSIZE1               0x00080000    /* mem size - Flash,512KB */
#define GEI2_INT_LVL                0x0c          /* IRQ 12 */
#define GEI2_INIT_STATE_MASK        (NONE)
#define GEI2_INIT_STATE             (NONE)
#define GEI2_SHMEM_BASE             NONE
#define GEI2_SHMEM_SIZE             0
#define GEI2_RXDES_NUM              GEI_RXDES_NUM
#define GEI2_TXDES_NUM              GEI_TXDES_NUM
#define GEI2_USR_FLAG               GEI_USR_FLAG

#define GEI3_MEMBASE0_LOW           0xfd600000   /* mem base - CSR */
#define GEI3_MEMBASE0_HIGH          0x00000000   /* mem base - CSR */
#define GEI3_MEMSIZE0               0x20000      /* mem size - CSR,128KB */
#define GEI3_MEMBASE1               0xfd700000   /* mem base - Flash */
#define GEI3_MEMSIZE1               0x00080000   /* mem size - Flash,512KB */
#define GEI3_INT_LVL                0x09         /* IRQ 9 */
#define GEI3_INIT_STATE_MASK        (NONE)
#define GEI3_INIT_STATE             (NONE)
#define GEI3_SHMEM_BASE             NONE
#define GEI3_SHMEM_SIZE             0
#define GEI3_RXDES_NUM              GEI_RXDES_NUM
#define GEI3_TXDES_NUM              GEI_TXDES_NUM
#define GEI3_USR_FLAG               GEI_USR_FLAG

#define GEI82543_LOAD_FUNC          gei82543EndLoad
#define GEI_X86_OFFSET_VALUE        GEI82543_OFFSET_VALUE

/* Alaska PHY's information */

#define MARVELL_OUI_ID              0x5043
#define MARVELL_ALASKA_88E1000      0x5
#define MARVELL_ALASKA_88E1000S     0x4
#define ALASKA_PHY_SPEC_CTRL_REG        0x10
#define ALASKA_PHY_SPEC_STAT_REG        0x11
#define ALASKA_INT_ENABLE_REG           0x12
#define ALASKA_INT_STATUS_REG           0x13
#define ALASKA_EXT_PHY_SPEC_CTRL_REG    0x14
#define ALASKA_RX_ERROR_COUNTER         0x15
#define ALASKA_LED_CTRL_REG             0x18

#define ALASKA_PSCR_ASSERT_CRS_ON_TX    0x0800
#define ALASKA_EPSCR_TX_CLK_25          0x0070

#define ALASKA_PSCR_AUTO_X_1000T        0x0040
#define ALASKA_PSCR_AUTO_X_MODE         0x0060

#define ALASKA_PSSR_DPLX                0x2000
#define ALASKA_PSSR_SPEED               0xC000
#define ALASKA_PSSR_10MBS               0x0000
#define ALASKA_PSSR_100MBS              0x4000
#define ALASKA_PSSR_1000MBS             0x8000

/* assuming 1:1 mapping for virtual:physical address */

#define GEI_SYS_WRITE_REG(unit, reg, value)     							\
        (MV_MEMIO_LE32_WRITE(												\
		(PCI_MEMIO2LOCAL(geiResources[(unit)].memBaseLow) + (reg)), value))

#define GEI_SYS_READ_REG(unit, reg)             							\
        (MV_MEMIO_LE32_READ(												\
		PCI_MEMIO2LOCAL(geiResources[(unit)].memBaseLow) + (reg)))

#define GEI_SYS_WRITE_FLUSH(unit)						\
{														\
   UINT32 x;											\
   x = GEI_SYS_READ_REG(unit, INTEL_82543GC_STATUS);	\
}


/* externs */

IMPORT END_TBL_ENTRY    endDevTbl[];    /* end device table */
IMPORT void     sysUsDelay (UINT32);

/* typedefs */
#define BUFF_SIZE       2048    /* buffer size for driver */

#define EEPROM_ENABLE_BITS              9
#define EEPROM_WRITE_DIS_OPCODE         ((0x4 << 6) | (0x0 << 4))
#define EEPROM_WRITE_EN_OPCODE          ((0x4 << 6) | (0x3 << 4))
#define EEPROM_WRITE_ALL_OPCODE         ((0x4 << 6) | (0x1 << 4))
#define EEPROM_ERASE_ALL_OPCODE         ((0x4 << 6) | (0x2 << 4))


/* Enumerated types specific to the e1000 hardware */
/* Media Access Controlers */
typedef enum {
    E1000_UNDEFINED = 0,
    E1000_82542_REV2_0,
    E1000_82542_REV2_1,
    E1000_82543,
    E1000_82544,
    E1000_82540,
    E1000_82545,
    E1000_82545_REV_3,
    E1000_82546,
    E1000_82546_REV_3,
    E1000_82541,
    E1000_82541_REV_2,
    E1000_82547,
    E1000_82547_REV_2,
    E1000_82571,
    E1000_82572,
    E1000_82573,
    E1000_NUM_MACS
} E1000_MAC_TYPE;

typedef enum {
    EEPROM_UNINITIALIZED = 0,
    EEPROM_SPI,
    EEPROM_MICROWIRE,
	EEPROM_FLASH,
	EEPROM_NONE, /* No NVM support */
    NUM_EEPROM_TYPES
} EEPROM_TYPE;

typedef struct eeprom {
    EEPROM_TYPE type;
    UINT16 mac_type;
    UINT16 opcode_bits;
    UINT16 address_bits;
    UINT16 delay_usec;
    UINT16 page_size;
    UINT16 word_size;
    STATUS use_eerd;
	STATUS use_eewr;
}EEPROM;

typedef struct geiResource        /* GEI_RESOURCE */
    {
    UINT32 memBaseLow;            /* Base Address LOW */
    UINT32 memBaseHigh;           /* Base Address HIGH */
    UINT32 flashBase;             /* Base Address for FLASH */
    char   irq;                   /* Interrupt Request Level */
    BOOL   adr64;                 /* Indicator for 64-bit support */
    int    boardType;             /* type of LAN board this unit is */
    int    pciBus;                /* PCI Bus number */
    int    pciDevice;             /* PCI Device number */
    int    pciFunc;               /* PCI Function number */
    UINT   memLength;             /* required memory size */
    UINT   initialStateMask;      /* mask parameter to vmStateSet */
    UINT   initialState;          /* state parameter to vmStateSet */
    EEPROM eeprom;				  /* eeprom information structure */
	UINT16 eeprom_icw1;           /* EEPROM initialization control word 1 */
    UINT16 eeprom_icw2;           /* EEPROM initialization control word 2 */
    UCHAR  enetAddr[6];           /* MAC address for this adaptor */
    STATUS iniStatus;             /* initialization perform status */
    UINT32 shMemBase;             /* Share memory address if any */
    UINT32 shMemSize;             /* Share memory size if any */
    UINT32 rxDesNum;              /* RX descriptor for this unit */
    UINT32 txDesNum;              /* TX descriptor for this unit */
    UINT32 usrFlags;              /* user flags for this unit */
    } GEI_RESOURCE;

/* locals */ 
LOCAL UINT32 	geiUnits;     /* number of GEIs we found */

LOCAL UINT32 	sys543LocalToPciBusAdrs (int unit, UINT32 adrs);
LOCAL UINT32 	sys543PciBusToLocalAdrs (int unit, UINT32 adrs);
void 			e1000_init_eeprom_params(int unit, int deviceId);
STATUS 			e1000_read_eeprom(int unit, UINT16 offset, UINT16 words, UINT16 *data);
LOCAL STATUS 	e1000_spi_eeprom_ready(int unit);
STATUS 			e1000_set_mac_type(int unit, int deviceId);
LOCAL void 		e1000_raise_ee_clk(int unit, UINT32 *eecd);
LOCAL void 		e1000_lower_ee_clk(int unit, UINT32 *eecd);
LOCAL void 		e1000_shift_out_ee_bits(int unit, UINT16 data, UINT16 count);
LOCAL UINT16 	e1000_shift_in_ee_bits(int unit, UINT16 count);
LOCAL STATUS 	e1000_acquire_eeprom(int unit);
LOCAL void 		e1000_standby_eeprom(int unit);
LOCAL void 		e1000_release_eeprom(int unit);
LOCAL STATUS 	e1000_spi_eeprom_ready(int unit);
STATUS 			e1000_read_eeprom(int unit, UINT16 offset, UINT16 words, UINT16 *data);
void 			e1000_update_eeprom_checksum(int uint);
LOCAL STATUS 	e1000_commit_shadow_ram(int unit);
LOCAL STATUS 	e1000_is_onboard_nvm_eeprom(int unit, UINT16 mac_type);
LOCAL UINT32 	e1000_read_eeprom_eerd(int unit,UINT16 offset,UINT16 words,UINT16 *data);
LOCAL UINT32 	e1000_write_eeprom_eewr(int unit,UINT16 offset,UINT16 words,UINT16 *data);
LOCAL STATUS 	e1000_poll_eerd_eewr_done(int unit, int eerd);
UINT32 			e1000_write_eeprom(int unit,UINT16 offset,UINT16 words,UINT16 *data);
UINT32 			e1000_write_eeprom(int unit,UINT16 offset,UINT16 words,UINT16 *data);
UINT32 e1000_write_eeprom_spi(int unit,UINT16 offset,UINT16 words,UINT16 *data);
UINT32 e1000_write_eeprom_microwire(int unit,UINT16 offset,UINT16 words,UINT16 *data);

LOCAL GEI_RESOURCE geiResources [] =
    {
    {GEI0_MEMBASE0_LOW,GEI0_MEMBASE0_HIGH, GEI0_MEMBASE1, GEI0_INT_LVL, 
     UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, 
     GEI0_MEMSIZE0, GEI0_INIT_STATE_MASK, GEI0_INIT_STATE, 
	 {UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN},
	 0, 0, {UNKNOWN}, ERROR, 
	 GEI0_SHMEM_BASE, GEI0_SHMEM_SIZE, GEI0_RXDES_NUM, GEI0_TXDES_NUM, 
     GEI0_USR_FLAG},
    {GEI1_MEMBASE0_LOW, GEI1_MEMBASE0_HIGH, GEI1_MEMBASE1, GEI1_INT_LVL, 
     UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, 
     GEI1_MEMSIZE0, GEI1_INIT_STATE_MASK, GEI1_INIT_STATE, 
	 {UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN},
     0, 0, {UNKNOWN}, ERROR, 
	 GEI1_SHMEM_BASE, GEI1_SHMEM_SIZE, GEI1_RXDES_NUM, GEI1_TXDES_NUM, 
     GEI1_USR_FLAG},
    {GEI2_MEMBASE0_LOW, GEI2_MEMBASE0_HIGH, GEI2_MEMBASE1, GEI2_INT_LVL, 
     UNKNOWN,UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,
     GEI2_MEMSIZE0, GEI2_INIT_STATE_MASK, GEI2_INIT_STATE, 
	 {UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN},
     0, 0, {UNKNOWN}, ERROR,
	 GEI2_SHMEM_BASE, GEI2_SHMEM_SIZE, GEI2_RXDES_NUM, GEI2_TXDES_NUM, 
     GEI2_USR_FLAG},
    {GEI3_MEMBASE0_LOW,GEI3_MEMBASE0_HIGH, GEI3_MEMBASE1, GEI3_INT_LVL, 
     UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, 
     GEI3_MEMSIZE0, GEI3_INIT_STATE_MASK, GEI3_INIT_STATE, 
	 {UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN},
     0, 0, {UNKNOWN}, ERROR,
	 GEI3_SHMEM_BASE, GEI3_SHMEM_SIZE, GEI3_RXDES_NUM, GEI3_TXDES_NUM, 
     GEI3_USR_FLAG},
     };

/* globals */
 
/*
 * NOTE: Each GEI device will need it's own lowMemBuf.
 * Since the PrPMC-G only has one GEI, this is all that is needed for now.
 */
                                              /* i82543 buffer in low memory */


/* forward declarations */

LOCAL int       sys543IntEnable  (int unit);
LOCAL int       sys543IntDisable (int unit);
LOCAL int       sys543IntAck     (int unit);
LOCAL void      sys543LoadStrCompose (int unit);
LOCAL STATUS    sys543eepromCheckSum (int unit);
LOCAL STATUS    sys543EtherAdrGet (int unit);
LOCAL void      sys543PhySpecRegsInit(PHY_INFO *, UINT8);
LOCAL void      sys1000NsDelay (void);

LOCAL STATUS    e1000_is_onboard_nvm_eeprom(int unit, UINT16 mac_type);
STATUS 			e1000_spi_eeprom_ready(int unit);

/*****************************************************************************
*
* sys543PciInit - Initialize and get the PCI configuration for 82543 Chips
*
* This routine finds out PCI device, and maps its memory and IO address.
* It must be done prior to initializing of 82543 chips.  Also
* must be done prior to MMU initialization, usrMmuInit().
*
* RETURNS: N/A
*/

STATUS sys543PciInit (void)
    {
    GEI_RESOURCE *pReso;         /* chip resources */
    int pciBus;                  /* PCI bus number */
    int pciDevice;               /* PCI device number */
    int pciFunc;                 /* PCI function number */
    int unit;                    /* unit number */
    int gei541DevUnit=0;         /* count of Intel 82541 */
    int gei544DevUnit=0;         /* count of Intel 82544 */
    int gei545DevUnit=0;         /* count of Intel 82545 */
    BOOL duplicate;              /* BOOL flag for duplicate chip */
    UINT32 bar0;                 /* PCI BAR_0 */
    UINT32 memBaseLow;           /* mem base low */
    UINT32 memBaseHigh;          /* mem base High */
    UINT32 flashBase;            /* flash base */
    UINT16 boardId =0;           /* adaptor Id */
    char   irq;                  /* irq number */
    int    ix;                   /* index */
    
    
	for (unit = 0; unit < NELEMENTS(geiResources); unit++)
	{
		boardId = UNKNOWN;

		/* Detect possible PRO1000 MT desktop adaptors */

		if (pciFindDevice (INTEL_PCI_VENDOR_ID, DEV_ID_82541GI, gei541DevUnit, 
						   &pciBus, &pciDevice, &pciFunc) == OK)
		{
			gei541DevUnit++;
			boardId = DEV_ID_82541GI;
		}

		/* Detect possible PRO1000 XT server adaptors */

		else if (pciFindDevice (INTEL_PCI_VENDOR_ID, DEV_ID_82544EI_COPPER, 
								gei544DevUnit, &pciBus, &pciDevice, &pciFunc) 
				 == OK)
		{
			gei544DevUnit++;
			boardId = PRO1000_544_BOARD;
		}

		/* Detect possible PRO1000 MT server adaptors */

		else if (pciFindDevice (INTEL_PCI_VENDOR_ID, DEV_ID_82545GM_COPPER, 
								gei545DevUnit, &pciBus, &pciDevice, &pciFunc) 
				 == OK)
		{
			gei545DevUnit++;

			boardId = DEV_ID_82545GM_COPPER;
		}
		else if (pciFindDevice (INTEL_PCI_VENDOR_ID, DEV_ID_82541GI_LF, 
								gei545DevUnit, &pciBus, &pciDevice, &pciFunc) 
				 == OK)
		{
			gei545DevUnit++;

			boardId = DEV_ID_82541GI_LF;
		}

		
#if (_WRS_VXWORKS_MAJOR == 6) 

		else if (pciFindDevice (INTEL_PCI_VENDOR_ID, DEV_ID_82572EI_COPPER, 
								gei545DevUnit, &pciBus, &pciDevice, &pciFunc) 
				 == OK)
		{
			gei545DevUnit++;

			boardId = DEV_ID_82572EI_COPPER;
		}
#endif 
		else
		{
			break;        
		}

		/* check the duplicate */

		pReso     = &geiResources [0];
		duplicate = FALSE;

		for (ix = 0; ix < NELEMENTS(geiResources); ix++, pReso++)
		{
			if ((ix != unit) && (pReso->pciBus == pciBus) &&
				(pReso->pciDevice == pciDevice) && (pReso->pciFunc == pciFunc))
				duplicate = TRUE;
		}

		if (duplicate)
			continue;

		/* we found the right one */

		/* Allocate cacheable memory for mgi unit */

		pReso = &geiResources [unit];
		pReso->boardType = boardId;
		pReso->pciBus    = pciBus;
		pReso->pciDevice = pciDevice;
		pReso->pciFunc   = pciFunc;


		/* 
		 * BAR0: [32:17]: memory base
		 *       [16:4] : read as "0"; 
		 *       [3]    : 0 - device is not prefetchable
		 *       [2:1]  : 00b - 32-bit address space, or
		 *                01b - 64-bit address space
		 *       [0]    : 0 - memory map decoded
		 *
		 * BAR1: if BAR0[2:1] == 00b, optional flash memory base
		 *       if BAR0[2:1] == 01b, high portion of memory base 
		 *                            for 64-bit address space        
		 *
		 * BAR2: if BAR0[2:1] == 01b, optional flash memory base
		 *       if BAR0[2:1] == 00b, behaves as BAR-1 when BAR-0 is 
		 *                            a 32-bit value 
		 */

		pciConfigInLong (pReso->pciBus, pReso->pciDevice, pReso->pciFunc,
						 PCI_CFG_BASE_ADDRESS_0, &bar0);

		pReso->adr64 = ((bar0 & BAR0_64_BIT) == BAR0_64_BIT)? TRUE : FALSE;

		/* If configured, set the PCI Configuration manually */

		if (PCI_CFG_TYPE == PCI_CFG_FORCE)
		{
			pciConfigOutLong (pReso->pciBus, pReso->pciDevice, pReso->pciFunc,
							  PCI_CFG_BASE_ADDRESS_0, pReso->memBaseLow);
			if (pReso->adr64)
			{
				pciConfigOutLong (pReso->pciBus, pReso->pciDevice, 
								  pReso->pciFunc,PCI_CFG_BASE_ADDRESS_1, 
								  pReso->memBaseHigh);

				pciConfigOutLong (pReso->pciBus, pReso->pciDevice, 
								  pReso->pciFunc, PCI_CFG_BASE_ADDRESS_2, 
								  pReso->flashBase);
			} else
			{
				pciConfigOutLong (pReso->pciBus, pReso->pciDevice, 
								  pReso->pciFunc, PCI_CFG_BASE_ADDRESS_1, 
								  pReso->flashBase);

				pciConfigOutLong (pReso->pciBus, pReso->pciDevice, 
								  pReso->pciFunc, PCI_CFG_BASE_ADDRESS_2, 
								  pReso->flashBase);
			}

			pciConfigOutByte (pReso->pciBus, pReso->pciDevice, pReso->pciFunc,
							  PCI_CFG_DEV_INT_LINE, pReso->irq);
		}

		/* read back memory base address and IO base address */

		pciConfigInLong (pReso->pciBus, pReso->pciDevice, pReso->pciFunc,
						 PCI_CFG_BASE_ADDRESS_0, &memBaseLow);
		if (pReso->adr64)
		{
			pciConfigInLong (pReso->pciBus, pReso->pciDevice, pReso->pciFunc,
							 PCI_CFG_BASE_ADDRESS_1, &memBaseHigh);
			pciConfigInLong (pReso->pciBus, pReso->pciDevice, pReso->pciFunc,
							 PCI_CFG_BASE_ADDRESS_2, &flashBase);
		} else
		{
			pciConfigInLong (pReso->pciBus, pReso->pciDevice, pReso->pciFunc,
							 PCI_CFG_BASE_ADDRESS_1, &flashBase);
			memBaseHigh = 0x0;
		}

		pciConfigInByte (pReso->pciBus, pReso->pciDevice, pReso->pciFunc,
						 PCI_CFG_DEV_INT_LINE, &irq);

		memBaseLow &= PCI_MEMBASE_MASK;
		flashBase  &= PCI_MEMBASE_MASK;

		/* over write the resource table with read value */

		pReso->memBaseLow   = memBaseLow;
		pReso->memBaseHigh  = memBaseHigh;
		pReso->flashBase    = flashBase;
		pReso->irq          = irq;

		/* enable mapped memory and IO addresses */

		pciConfigOutWord (pReso->pciBus, pReso->pciDevice, pReso->pciFunc,
						  PCI_CFG_COMMAND, PCI_CMD_IO_ENABLE |
						  PCI_CMD_MEM_ENABLE | PCI_CMD_MASTER_ENABLE);

		/* Initialize the eeprom */

		e1000_init_eeprom_params(unit, boardId);

		/* compose loadString in endDevTbl for this unit */        

		sys543LoadStrCompose (unit);

		geiUnits++;
	}

    return OK;
    }

/****************************************************************************
*
* sys543LoadStrCompose - Compose the END load string for the device
*
* The END device load string formed by this routine is in the following
* following format.
* <shMemBase>:<shMemSize>:<rxDesNum>:<txDesNum>:<usrFlags>:<offset>
*
* RETURN: N/A
*/

LOCAL void sys543LoadStrCompose(int unit)
{
	END_TBL_ENTRY *     pDevTbl;

	for (pDevTbl = endDevTbl; pDevTbl->endLoadFunc != END_TBL_END;pDevTbl++)
	{
		if (((UINT32)pDevTbl->endLoadFunc == (UINT32)GEI82543_LOAD_FUNC) &&
			(pDevTbl->unit == unit))
		{
			sprintf (pDevTbl->endLoadString,
					 "0x%x:0x%x:0x%x:0x%x:0x%x:%d",

					 /* This will need to be setup in the original geiResources table. */
					 geiResources [unit].shMemBase,
					 geiResources [unit].shMemSize,
					 geiResources [unit].rxDesNum,	/* RX Descriptor Number*/
					 geiResources [unit].txDesNum,	/* TX Descriptor Number*/
					 geiResources [unit].usrFlags,	/* user's flags */
					 GEI_X86_OFFSET_VALUE			/* offset value */                 
					);

			/* Also, enable the table entry */
			pDevTbl->processed = FALSE;
			return;
		}
	}
	return;
}

/*****************************************************************************
*
* sys82543BoardInit - Adaptor initialization for 82543 chip
*
* This routine is expected to perform any adapter-specific or target-specific
* initialization that must be done prior to initializing the 82543 chip.
*
* The 82543 driver calls this routine from the driver load routine before
* any other routines.
*
*
*  unit,   unit number 
*  pBoard  board information for the GEI driver
* *
* RETURNS: OK or ERROR
*/

STATUS sys82543BoardInit(int unit, ADAPTOR_INFO *pBoard)
{
    GEI_RESOURCE *pReso = &geiResources [unit];

    /* sanity check */
    if (unit >= geiUnits)
        return (ERROR);

    if (pReso->boardType !=  DEV_ID_82541GI && 
		pReso->boardType !=  DEV_ID_82541GI_LF &&  
        pReso->boardType !=  DEV_ID_82544EI_COPPER &&
        pReso->boardType !=  DEV_ID_82572EI_COPPER &&
		pReso->boardType !=  DEV_ID_82545GM_COPPER &&
        pReso->boardType !=  PRO1000_543_BOARD &&
        pReso->boardType !=  PRO1000_544_BOARD &&
        pReso->boardType !=  PRO1000_546_BOARD)
           return ERROR;

    /* perform EEPROM checksum */

    if (sys543eepromCheckSum (unit) != OK)
    {
        mvOsPrintf ("GEI82543:unit=%d, EEPROM checksum Error!\n", unit);
    }

    /* get the Ethernet Address from eeprom */

    if (sys543EtherAdrGet (unit) != OK)
        {
        mvOsPrintf ("GEI82543:unit=%d, Invalid Ethernet Address!\n", unit);
        }       

    /* get the initialization control word 1 (ICW1) in EEPROM */

	e1000_read_eeprom(unit, EEPROM_ICW1, 1, &(geiResources[unit].eeprom_icw1));

    /* get the initialization control word 2 (ICW2) in EEPROM */

	e1000_read_eeprom(unit, EEPROM_ICW2, 1, &(geiResources[unit].eeprom_icw2));

    /* initializes the board information structure */

    pBoard->boardType     = pReso->boardType;
    pBoard->vector        = pReso->irq;

    pBoard->regBaseLow    = PCI_MEMIO2LOCAL(pReso->memBaseLow);
    pBoard->regBaseHigh   = PCI_MEMIO2LOCAL(pReso->memBaseHigh);
    pBoard->flashBase     = PCI_MEMIO2LOCAL(pReso->flashBase);
    pBoard->adr64         = pReso->adr64;

    pBoard->intEnable     = sys543IntEnable;
    pBoard->intDisable    = sys543IntDisable;
    pBoard->intAck        = sys543IntAck;
  
	/* Support only RJ45 */
	pBoard->phyType   = GEI_PHY_GMII_TYPE;      

    pBoard->phySpecInit   = sys543PhySpecRegsInit;
    pBoard->delayFunc     = (FUNCPTR) sys1000NsDelay;
    pBoard->delayUnit     = 1000;   /* sys1000NsDelay() delays 1000ns */
    pBoard->sysLocalToBus = sys543LocalToPciBusAdrs; 
    pBoard->sysBusToLocal = sys543PciBusToLocalAdrs; 

    /* specify the interrupt connect/disconnect routines to be used */

    pBoard->intConnect    = (FUNCPTR) pciIntConnect;
    pBoard->intDisConnect = (FUNCPTR) pciIntDisconnect;

    /* get the ICW1 and ICW2 */

    pBoard->eeprom_icw1   = geiResources[unit].eeprom_icw1;
    pBoard->eeprom_icw2   = geiResources[unit].eeprom_icw2;

    /* copy Ether address */

    memcpy (&pBoard->enetAddr[0], &geiResources[unit].enetAddr[0], 
            ETHER_ADDRESS_SIZE);

    /* we finished adaptor initialization */

    pReso->iniStatus = OK;
    
    return (OK);
    }

/*******************************************************************************
* e1000_set_mac_type - Set MAC type in eeprom struct.
*
* DESCRIPTION:
*		This function identify the MAC type according to the device ID.
*
* INPUT:
*		unit     - Device unit number.
*		deviceId - The device ID as defined in PCI configuration.
*
* OUTPUT:
*		None.
*
* RETURN:
*		OK if the given device ID has a match for MAC type.
*
*******************************************************************************/
STATUS e1000_set_mac_type(int unit, int deviceId)
{
    EEPROM *eeprom = &geiResources[unit].eeprom;

    switch (deviceId) {
    case DEV_ID_82543GC_FIBER:
    case DEV_ID_82543GC_COPPER:
        eeprom->mac_type = E1000_82543;
        break;
    case DEV_ID_82544EI_COPPER:
    case DEV_ID_82544EI_FIBER:
    case DEV_ID_82544GC_COPPER:
    case DEV_ID_82544GC_LOM:
        eeprom->mac_type = E1000_82544;
        break;
    case DEV_ID_82540EM:
    case DEV_ID_82540EM_LOM:
    case DEV_ID_82540EP:
    case DEV_ID_82540EP_LOM:
    case DEV_ID_82540EP_LP:
        eeprom->mac_type = E1000_82540;
        break;
    case DEV_ID_82545EM_COPPER:
    case DEV_ID_82545EM_FIBER:
        eeprom->mac_type = E1000_82545;
        break;
	case DEV_ID_82545GM_COPPER:
    case DEV_ID_82545GM_FIBER:
    case DEV_ID_82545GM_SERDES:
        eeprom->mac_type = E1000_82545_REV_3;
        break;
    case DEV_ID_82546EB_COPPER:
    case DEV_ID_82546EB_FIBER:
    case DEV_ID_82546EB_QUAD_COPPER:
        eeprom->mac_type = E1000_82546;
        break;
    case DEV_ID_82546GB_COPPER:
    case DEV_ID_82546GB_FIBER:
    case DEV_ID_82546GB_SERDES:
        eeprom->mac_type = E1000_82546_REV_3;
        break;
    case DEV_ID_82541EI:
    case DEV_ID_82541EI_MOBILE:
        eeprom->mac_type = E1000_82541;
        break;
    case DEV_ID_82541ER:
    case DEV_ID_82541GI:
    case DEV_ID_82541GI_LF:
    case DEV_ID_82541GI_MOBILE:
        eeprom->mac_type = E1000_82541_REV_2;
        break;
    case DEV_ID_82547EI:
        eeprom->mac_type = E1000_82547;
        break;
    case DEV_ID_82547GI:
        eeprom->mac_type = E1000_82547_REV_2;
        break;
    case DEV_ID_82571EB_COPPER:
    case DEV_ID_82571EB_FIBER:
    case DEV_ID_82571EB_SERDES:
        eeprom->mac_type = E1000_82571;
        break;
    case DEV_ID_82572EI_COPPER:
    case DEV_ID_82572EI_FIBER:
    case DEV_ID_82572EI_SERDES:
        eeprom->mac_type = E1000_82572;
        break;
    case DEV_ID_82573E:
    case DEV_ID_82573E_IAMT:
    case DEV_ID_82573L:
        eeprom->mac_type = E1000_82573;
        break;
    default:
        /* Should never have loaded on this device */
        return ERROR;
    }

    return OK;
}

/*******************************************************************************
* e1000_init_eeprom_params - Initialize the eeprom struct.
*
* DESCRIPTION:
*		This function initialize the eeprom struct according to the MAC type.
*
* INPUT:
*		unit     - Device unit number.
*		deviceId - The device ID as defined in PCI configuration.
*
* OUTPUT:
*		None.
*
* RETURN:
*		None.
*
*******************************************************************************/
void e1000_init_eeprom_params(int unit, int deviceId)
{
	UINT16 eeprom_size;
    EEPROM *eeprom = &geiResources[unit].eeprom;
	UINT32 eecd = GEI_SYS_READ_REG(unit, INTEL_82543GC_EECD);

	if (e1000_set_mac_type(unit, deviceId) == ERROR)
	{
		mvOsPrintf("GEI interface unknown MAC\n");
		return;
	}
	switch (eeprom->mac_type) 
	{
		case E1000_82543:
		case E1000_82544:
			eeprom->type = EEPROM_MICROWIRE;
			eeprom->word_size 	 = 64;
			eeprom->opcode_bits  = 3;
			eeprom->address_bits = 6;
			eeprom->delay_usec 	 = 50;
			eeprom->use_eerd   	 = FALSE;
			eeprom->use_eewr   	 = FALSE;
			break;
		case E1000_82540:
		case E1000_82545:
		case E1000_82545_REV_3:
		case E1000_82546:
		case E1000_82546_REV_3:
			eeprom->type = EEPROM_MICROWIRE;
			eeprom->opcode_bits = 3;
			eeprom->delay_usec = 50;
			if(eecd & E1000_EECD_SIZE) 
			{
				eeprom->word_size = 256;
				eeprom->address_bits = 8;
			} 
			else
			{
				eeprom->word_size = 64;
				eeprom->address_bits = 6;
			}
			break;
		case E1000_82541:
		case E1000_82541_REV_2:
		case E1000_82547:
		case E1000_82547_REV_2:
			if (eecd & E1000_EECD_TYPE) 
			{
				eeprom->type = EEPROM_SPI;
				eeprom->opcode_bits = 8;
				eeprom->delay_usec = 1;
				if (eecd & E1000_EECD_ADDR_BITS) 
				{
					eeprom->page_size = 32;
					eeprom->address_bits = 16;
				} 
				else 
				{
					eeprom->page_size = 8;
					eeprom->address_bits = 8;
				}
			} 
			else 
			{
				eeprom->type = EEPROM_MICROWIRE;
				eeprom->opcode_bits = 3;
				eeprom->delay_usec = 50;
				if (eecd & E1000_EECD_ADDR_BITS) 
				{
					eeprom->word_size = 256;
					eeprom->address_bits = 8;
				} 
				else 
				{
					eeprom->word_size = 64;
					eeprom->address_bits = 6;
				}
			}
			eeprom->use_eerd = FALSE;
			eeprom->use_eewr = FALSE;
			break;
		case E1000_82571:
		case E1000_82572:
			eeprom->type = EEPROM_SPI;
			eeprom->opcode_bits = 8;
			eeprom->delay_usec = 1;
			if (eecd & E1000_EECD_ADDR_BITS) 
			{
				eeprom->page_size = 32;
				eeprom->address_bits = 16;
			} 
			else 
			{
				eeprom->page_size = 8;
				eeprom->address_bits = 8;
			}
			eeprom->use_eerd = FALSE;
			eeprom->use_eewr = FALSE;
		break;
		case E1000_82573:
			eeprom->type = EEPROM_SPI;
			eeprom->opcode_bits = 8;
			eeprom->delay_usec = 1;
			if (eecd & E1000_EECD_ADDR_BITS) 
			{
				eeprom->page_size = 32;
				eeprom->address_bits = 16;
			} 
			else 
			{
				eeprom->page_size = 8;
				eeprom->address_bits = 8;
			}
			eeprom->use_eerd = TRUE;
			eeprom->use_eewr = TRUE;
			if(e1000_is_onboard_nvm_eeprom(unit, eeprom->mac_type) == FALSE) 
			{
				eeprom->type = EEPROM_FLASH;
				eeprom->word_size = 2048;
	
				/* Ensure that the Autonomous FLASH update bit is cleared due to
				 * Flash update issue on parts which use a FLASH for NVM. */
				eecd &= ~E1000_EECD_AUPDEN;
				GEI_SYS_WRITE_REG(unit, INTEL_82543GC_EECD, eecd);
			}
		break;
		default:
			break;
    }


    if (eeprom->type == EEPROM_SPI) 
	{
        /* eeprom_size will be an enum [0..8] that maps to eeprom sizes 128B to
         * 32KB (incremented by powers of 2).
         */
        if(eeprom->mac_type <= E1000_82547_REV_2) 
		{
            /* Set to default value for initial eeprom read. */
            eeprom->word_size = 64;
            if (!e1000_read_eeprom(unit, EEPROM_CFG, 1, &eeprom_size))
			{
				mvOsPrintf(" e1000_init_eeprom_params; faild in e1000_read_eeprom\n");
                return ;
			}
            eeprom_size = (eeprom_size & EEPROM_SIZE_MASK) >> EEPROM_SIZE_SHIFT;
            /* 256B eeprom size was not supported in earlier hardware, so we
             * bump eeprom_size up one to ensure that "1" (which maps to 256B)
             * is never the result used in the shifting logic below. */
            if(eeprom_size)
                eeprom_size++;
        } 
		else 
		{
            eeprom_size = (uint16_t)((eecd & E1000_EECD_SIZE_EX_MASK) >>
                          E1000_EECD_SIZE_EX_SHIFT);
        }

        eeprom->word_size = 1 << (eeprom_size + EEPROM_WORD_SIZE_SHIFT);
    }
}



/*******************************************************************************
* e1000_raise_ee_clk - Raises the EEPROM's clock input.
*
* DESCRIPTION:
*		To generate eeprom read/write, its clock input pins should be toggled. 
*		This function raises the EEPROM's clock input.
*
* INPUT:
*		unit - Device unit number.
*		eecd - The device eeprom structure.
*
* OUTPUT:
*		None.
*
* RETURN:
*		None.
*
*******************************************************************************/
LOCAL void e1000_raise_ee_clk(int unit, UINT32 *eecd)
{
    EEPROM *eeprom = &geiResources[unit].eeprom;

    /* Raise the clock input to the EEPROM (by setting the SK bit), and then
     * wait <delay> microseconds.
     */
    *eecd = *eecd | E1000_EECD_SK;
    GEI_SYS_WRITE_REG(unit, INTEL_82543GC_EECD, *eecd);
    GEI_SYS_WRITE_FLUSH(unit);
    sysUsDelay(eeprom->delay_usec);
}

/*******************************************************************************
* e1000_lower_ee_clk - Lowers the EEPROM's clock input.
*
* DESCRIPTION:
*		To generate eeprom read/write, its clock input pins should be toggled. 
*		This function lowers the EEPROM's clock input.
*
* INPUT:
*		unit - Device unit number.
*		eecd - The device eeprom structure.
*
* OUTPUT:
*		None.
*
* RETURN:
*		None.
*
*******************************************************************************/
LOCAL void e1000_lower_ee_clk(int unit, UINT32 *eecd)
{
    EEPROM *eeprom = &geiResources[unit].eeprom;

    /* Lower the clock input to the EEPROM (by clearing the SK bit), and then
     * wait 50 microseconds.
     */
    *eecd = *eecd & ~E1000_EECD_SK;
    GEI_SYS_WRITE_REG(unit, INTEL_82543GC_EECD, *eecd);
    GEI_SYS_WRITE_FLUSH(unit);
    sysUsDelay(eeprom->delay_usec);
}

/*******************************************************************************
* e1000_shift_out_ee_bits - Read data bits out to the EEPROM.
*
* DESCRIPTION:
*		To generate eeprom write, a binary value is written bit by bit. 
*       A value of "1" is written to the eeprom by setting bit "DI" to a "1",
*       and then raising and then lowering the clock.
*		A value of "0" is written to the eeprom by setting bit "DI" to a "0", 
*		and then raising and then lowering the clock.
*
* INPUT:
*		unit  - Device unit number.
*		data  - Data to write.
*		count - Number of bits to write.
*
* OUTPUT:
*		None.
*
* RETURN:
*		None.
*
*******************************************************************************/
LOCAL void e1000_shift_out_ee_bits(int unit, UINT16 data, UINT16 count)
{
    EEPROM *eeprom = &geiResources[unit].eeprom;
    UINT32 eecd;
    UINT32 mask;

    /* We need to shift "count" bits out to the EEPROM. So, value in the
     * "data" parameter will be shifted out to the EEPROM one bit at a time.
     * In order to do this, "data" must be broken down into bits.
     */
    mask = 0x01 << (count - 1);
    eecd = GEI_SYS_READ_REG(unit, INTEL_82543GC_EECD);

    if (eeprom->type == EEPROM_MICROWIRE) {
        eecd &= ~E1000_EECD_DO;
    } else if (eeprom->type == EEPROM_SPI) {
        eecd |= E1000_EECD_DO;
    }
    do {
        /* A "1" is shifted out to the EEPROM by setting bit "DI" to a "1",
         * and then raising and then lowering the clock (the SK bit controls
         * the clock input to the EEPROM).  A "0" is shifted out to the EEPROM
         * by setting "DI" to "0" and then raising and then lowering the clock.
         */
        eecd &= ~E1000_EECD_DI;

        if(data & mask)
            eecd |= E1000_EECD_DI;

        GEI_SYS_WRITE_REG(unit, INTEL_82543GC_EECD, eecd);
        GEI_SYS_WRITE_FLUSH(unit);

        sysUsDelay(eeprom->delay_usec);

        e1000_raise_ee_clk(unit, &eecd);
        e1000_lower_ee_clk(unit, &eecd);

        mask = mask >> 1;

    } while(mask);

    /* We leave the "DI" bit set to "0" when we leave this routine. */
    eecd &= ~E1000_EECD_DI;
	GEI_SYS_WRITE_REG(unit, INTEL_82543GC_EECD, eecd);
}

/*******************************************************************************
* e1000_shift_in_ee_bits - Read data bits from the EEPROM
*
* DESCRIPTION:
* 		In order to read a register from the EEPROM, we need to shift 'count'
* 		bits in from the EEPROM. Bits are "shifted in" by raising the clock
* 		input to the EEPROM, and then reading the value of the "DO" bit.  
*
* INPUT:
*		unit  - Device unit number.
*		count - Number of bits to write.
*
* OUTPUT:
*		None.
*
* RETURN:
*		Up to 16 bit read data
*
*******************************************************************************/
LOCAL UINT16 e1000_shift_in_ee_bits(int unit, UINT16 count)
{
    UINT32 eecd;
    UINT32 i;
    UINT16 data;

    /* In order to read a register from the EEPROM, we need to shift 'count'
     * bits in from the EEPROM. Bits are "shifted in" by raising the clock
     * input to the EEPROM (setting the SK bit), and then reading the value of
     * the "DO" bit.  During this "shifting in" process the "DI" bit should
     * always be clear.
     */

    eecd = GEI_SYS_READ_REG(unit, INTEL_82543GC_EECD);

    eecd &= ~(E1000_EECD_DO | E1000_EECD_DI);
    data = 0;

    for(i = 0; i < count; i++) 
	{
        data = data << 1;
        e1000_raise_ee_clk(unit, &eecd);

		eecd = GEI_SYS_READ_REG(unit, INTEL_82543GC_EECD);

        eecd &= ~(E1000_EECD_DI);
        if(eecd & E1000_EECD_DO)
            data |= 1;

        e1000_lower_ee_clk(unit, &eecd);
    }

    return data;
}

/*******************************************************************************
* e1000_acquire_eeprom - Prepares EEPROM for access
*
* DESCRIPTION:
* 		This function prepares the eeprom for accesses:
*		- Lowers EEPROM clock. 
*		- Clears input pin. 
*		- Sets the chip select pin. 
*		This function should be called before issuing a command to the EEPROM.
*
* INPUT:
*		unit  - Device unit number.
*
* OUTPUT:
*		None.
*
* RETURN:
*		None.
*
*******************************************************************************/
LOCAL STATUS e1000_acquire_eeprom(int unit)
{
    EEPROM *eeprom = &geiResources[unit].eeprom;
    UINT32 eecd, i=0;

    eecd = GEI_SYS_READ_REG(unit, INTEL_82543GC_EECD);

    if (eeprom->mac_type != E1000_82573) 
	{
		/* Request EEPROM Access */
		if(eeprom->mac_type > E1000_82544) {
			eecd |= E1000_EECD_REQ;
			GEI_SYS_WRITE_REG(unit, INTEL_82543GC_EECD, eecd);
			eecd = GEI_SYS_READ_REG(unit, INTEL_82543GC_EECD);
			while((!(eecd & E1000_EECD_GNT)) &&
				  (i < E1000_EEPROM_GRANT_ATTEMPTS)) {
				i++;
				sysUsDelay(5);
				eecd = GEI_SYS_READ_REG(unit, INTEL_82543GC_EECD);
			}
			if(!(eecd & E1000_EECD_GNT)) {
				eecd &= ~E1000_EECD_REQ;
				GEI_SYS_WRITE_REG(unit, INTEL_82543GC_EECD, eecd);
				mvOsPrintf("Could not acquire EEPROM grant\n");
				return ERROR;
			}
		}
	}
    /* Setup EEPROM for Read/Write */

    if (eeprom->type == EEPROM_MICROWIRE) {
        /* Clear SK and DI */
        eecd &= ~(E1000_EECD_DI | E1000_EECD_SK);
		GEI_SYS_WRITE_REG(unit, INTEL_82543GC_EECD, eecd);

        /* Set CS */
        eecd |= E1000_EECD_CS;
		GEI_SYS_WRITE_REG(unit, INTEL_82543GC_EECD, eecd);
    } else if (eeprom->type == EEPROM_SPI) {
        /* Clear SK and CS */
        eecd &= ~(E1000_EECD_CS | E1000_EECD_SK);
		GEI_SYS_WRITE_REG(unit, INTEL_82543GC_EECD, eecd);
        sysUsDelay(1);
    }

    return OK;
}

/*******************************************************************************
* e1000_standby_eeprom - Returns EEPROM to a "standby" state
*
* DESCRIPTION:
* 		This function returns the EEPROM to a "standby" state.
*
* INPUT:
*		unit  - Device unit number.
*
* OUTPUT:
*		None.
*
* RETURN:
*		None.
*
*******************************************************************************/
LOCAL void e1000_standby_eeprom(int unit)
{
    EEPROM *eeprom = &geiResources[unit].eeprom;
    UINT32 eecd;

	eecd = GEI_SYS_READ_REG(unit, INTEL_82543GC_EECD);

    if(eeprom->type == EEPROM_MICROWIRE) {
        eecd &= ~(E1000_EECD_CS | E1000_EECD_SK);
		GEI_SYS_WRITE_REG(unit, INTEL_82543GC_EECD, eecd);
        GEI_SYS_WRITE_FLUSH(unit);
        sysUsDelay(eeprom->delay_usec);

        /* Clock high */
        eecd |= E1000_EECD_SK;
		GEI_SYS_WRITE_REG(unit, INTEL_82543GC_EECD, eecd);
        GEI_SYS_WRITE_FLUSH(unit);
        sysUsDelay(eeprom->delay_usec);

        /* Select EEPROM */
        eecd |= E1000_EECD_CS;
		GEI_SYS_WRITE_REG(unit, INTEL_82543GC_EECD, eecd);
        GEI_SYS_WRITE_FLUSH(unit);
        sysUsDelay(eeprom->delay_usec);

        /* Clock low */
        eecd &= ~E1000_EECD_SK;
		GEI_SYS_WRITE_REG(unit, INTEL_82543GC_EECD, eecd);
        GEI_SYS_WRITE_FLUSH(unit);
        sysUsDelay(eeprom->delay_usec);
    } else if(eeprom->type == EEPROM_SPI) {
        /* Toggle CS to flush commands */
        eecd |= E1000_EECD_CS;
		GEI_SYS_WRITE_REG(unit, INTEL_82543GC_EECD, eecd);
        GEI_SYS_WRITE_FLUSH(unit);
        sysUsDelay(eeprom->delay_usec);
        
		eecd &= ~E1000_EECD_CS;
		GEI_SYS_WRITE_REG(unit, INTEL_82543GC_EECD, eecd);
        GEI_SYS_WRITE_FLUSH(unit);
        sysUsDelay(eeprom->delay_usec);
    }
}

/*******************************************************************************
* e1000_release_eeprom - Terminates a command.
*
* DESCRIPTION:
* 		This function Terminates a command by inverting the EEPROM's 
*		chip select pin.
*
* INPUT:
*		unit  - Device unit number.
*
* OUTPUT:
*		None.
*
* RETURN:
*		None.
*
*******************************************************************************/
LOCAL void e1000_release_eeprom(int unit)
{
    EEPROM *eeprom = &geiResources[unit].eeprom;
    UINT32 eecd;

    eecd = GEI_SYS_READ_REG(unit, INTEL_82543GC_EECD);

    if (eeprom->type == EEPROM_SPI) {
        eecd |= E1000_EECD_CS;  /* Pull CS high */
        eecd &= ~E1000_EECD_SK; /* Lower SCK */

		GEI_SYS_WRITE_REG(unit, INTEL_82543GC_EECD, eecd);

        sysUsDelay(eeprom->delay_usec);
    } else if(eeprom->type == EEPROM_MICROWIRE) {
        /* cleanup eeprom */

        /* CS on Microwire is active-high */
        eecd &= ~(E1000_EECD_CS | E1000_EECD_DI);

		GEI_SYS_WRITE_REG(unit, INTEL_82543GC_EECD, eecd);

        /* Rising edge of clock */
        eecd |= E1000_EECD_SK;
		GEI_SYS_WRITE_REG(unit, INTEL_82543GC_EECD, eecd);
        GEI_SYS_WRITE_FLUSH(unit);
        sysUsDelay(eeprom->delay_usec);

        /* Falling edge of clock */
        eecd &= ~E1000_EECD_SK;
		GEI_SYS_WRITE_REG(unit, INTEL_82543GC_EECD, eecd);
        GEI_SYS_WRITE_FLUSH(unit);
        sysUsDelay(eeprom->delay_usec);
    }

    /* Stop requesting EEPROM access */
    if(eeprom->mac_type > E1000_82544) {
        eecd &= ~E1000_EECD_REQ;
		GEI_SYS_WRITE_REG(unit, INTEL_82543GC_EECD, eecd);
    }
}

/*******************************************************************************
* e1000_spi_eeprom_ready - Wait for ready state
*
* DESCRIPTION:
* 		This function polls the status register for command completion.
*
* INPUT:
*		unit  - Device unit number.
*
* OUTPUT:
*		None.
*
* RETURN:
*		None.
*
*******************************************************************************/
LOCAL STATUS e1000_spi_eeprom_ready(int unit)
{
    UINT16 retry_count = 0;
    UINT8  spi_stat_reg;
    EEPROM *eeprom = &geiResources[unit].eeprom;

    /* Read "Status Register" repeatedly until the LSB is cleared.  The
     * EEPROM will signal that the command has been completed by clearing
     * bit 0 of the internal status register.  If it's not cleared within
     * 5 milliseconds, then error out.
     */
    retry_count = 0;
    do {
        e1000_shift_out_ee_bits(unit, EEPROM_RDSR_OPCODE_SPI, 
                                eeprom->opcode_bits);
        spi_stat_reg = (uint8_t)e1000_shift_in_ee_bits(unit, 8);
        if (!(spi_stat_reg & EEPROM_STATUS_RDY_SPI))
            break;

        sysUsDelay(5);
        retry_count += 5;

        e1000_standby_eeprom(unit);
    } while(retry_count < EEPROM_MAX_RETRY_SPI);

    /* ATMEL SPI write time could vary from 0-20mSec on 3.3V devices (and
     * only 0-5mSec on 5V devices)
     */
    if(retry_count >= EEPROM_MAX_RETRY_SPI) {
        mvOsPrintf("SPI EEPROM Status error\n");
        return ERROR;
    }

    return OK;
}

/******************************************************************************
 * Reads a 16 bit word from the EEPROM.
 *
 * unit  - Device unit number.
 * offset - offset of  word in the EEPROM to read
 * data - word read from the EEPROM
 * words - number of words to read
 *****************************************************************************/
/*******************************************************************************
* e1000_read_eeprom - Reads a 16 bit word from the EEPROM.
*
* DESCRIPTION:
* 		This function reads 16 bit word from the EEPROM.
*
* INPUT:
*		unit   - Device unit number.
*		offset - offset within the eeprom.
*		words  - Number of words to read.
*
* OUTPUT:
*		data   - Pointer to buffer that holds the read result.
*
* RETURN:
*		OK if reading succeded.
*
*******************************************************************************/
STATUS e1000_read_eeprom(int unit, UINT16 offset, UINT16 words, UINT16 *data)
{
    EEPROM *eeprom = &geiResources[unit].eeprom;
    UINT32 i = 0;
    STATUS ret_val;

    if((offset >= eeprom->word_size) || (words > eeprom->word_size - offset) ||
       (words == 0)) {
		mvOsPrintf("e1000_read_eeprom: \"words\" parameter out of bounds\n");
        return ERROR;
    }

    /* FLASH reads without acquiring the semaphore are safe */
    if (e1000_is_onboard_nvm_eeprom(unit,eeprom->mac_type) == TRUE && 
		eeprom->use_eerd == FALSE) 
	{
        switch (eeprom->mac_type) 
		{
			default:
				/* Prepare the EEPROM for reading  */
				if (e1000_acquire_eeprom(unit) != OK)
					return ERROR;
            break;
        }
    }

    if (eeprom->use_eerd == TRUE) 
	{
        ret_val = e1000_read_eeprom_eerd(unit, offset, words, data);
        if ((e1000_is_onboard_nvm_eeprom(unit,eeprom->mac_type) == TRUE) ||
            (eeprom->mac_type != E1000_82573))
            e1000_release_eeprom(unit);
        return ret_val;
    }

	if(eeprom->type == EEPROM_SPI) 
	{
        UINT16 word_in;
        UINT8  read_opcode = EEPROM_READ_OPCODE_SPI;

        if(e1000_spi_eeprom_ready(unit)) 
		{
            e1000_release_eeprom(unit);
            return ERROR;
        }

        e1000_standby_eeprom(unit);

        /* Some SPI eeproms use the 8th address bit embedded in the opcode */
        if((eeprom->address_bits == 8) && (offset >= 128))
            read_opcode |= EEPROM_A8_OPCODE_SPI;

        /* Send the READ command (opcode + addr)  */
        e1000_shift_out_ee_bits(unit, read_opcode, eeprom->opcode_bits);
        e1000_shift_out_ee_bits(unit, (uint16_t)(offset*2), eeprom->address_bits);

        /* Read the data.  The address of the eeprom internally increments with
         * each byte (spi) being read, saving on the overhead of eeprom setup
         * and tear-down.  The address counter will roll over if reading beyond
         * the size of the eeprom, thus allowing the entire memory to be read
         * starting from any offset. */
        for (i = 0; i < words; i++) 
		{
            word_in = e1000_shift_in_ee_bits(unit, 16);
            data[i] = (word_in >> 8) | (word_in << 8);
        }
    } 
	else if(eeprom->type == EEPROM_MICROWIRE) 
	{
			for (i = 0; i < words; i++) 
			{
				/* Send the READ command (opcode + addr)  */
				e1000_shift_out_ee_bits(unit, EEPROM_READ_OPCODE_MICROWIRE,
										eeprom->opcode_bits);
				e1000_shift_out_ee_bits(unit, (uint16_t)(offset + i),
										eeprom->address_bits);
	
				/* Read the data.  For microwire, each word requires the overhead
				 * of eeprom setup and tear-down. */
				data[i] = e1000_shift_in_ee_bits(unit, 16);
				e1000_standby_eeprom(unit);
			}
    }

    /* End this read operation */
    e1000_release_eeprom(unit);

    return OK;
}
/*************************************************************************
*
* sys543EtherAdrGet - Get Ethernet address from EEPROM
*
* This routine get an Ethernet address from EEPROM
*
* RETURNS: OK or ERROR
*/

LOCAL STATUS sys543EtherAdrGet
    (
    int unit
    )
    {
	int ix, iy;
    UINT16  adr [ETHER_ADDRESS_SIZE / sizeof(UINT16)];

	e1000_read_eeprom(unit, 
					  EEPROM_IA_ADDRESS, 
					  (ETHER_ADDRESS_SIZE / sizeof(UINT16)), 
					  (UINT16*)adr);

    /* Ethernet address represented always in LE mode 						*/
	/* Because the ethernet address is read in 16-bit fragments and the  	*/
	/* enetAddr is a char array, it is requred to make sure endiannes is 	*/
	/* kept in LE mode 													 	*/

    for (ix = 0, iy = 0; ix < 3; ix++)
    {
        geiResources[unit].enetAddr[iy++] = adr[ix] & 0xff;
        geiResources[unit].enetAddr[iy++] = (adr[ix] >> 8) & 0xff;
    }

	/* check IA is UCAST  */

    if (adr[0] & 0x1)
        return (ERROR);

    return (OK);
    }
/**************************************************************************
* 
* sys543eepromCheckSum - calculate checksum 
*
* This routine perform EEPROM checksum
*
* RETURNS: N/A
*/
LOCAL STATUS sys543eepromCheckSum (int unit)
{
    UINT16 checkSum = 0 ;
    UINT16 checkSumTmp;
    UINT16 eeprom_data;
    UINT32 ix;
    EEPROM *eeprom = &geiResources[unit].eeprom;

    if ((eeprom->mac_type == E1000_82573) && 
		(e1000_is_onboard_nvm_eeprom(unit, eeprom->mac_type) == FALSE)) 
	{
        /* Check bit 4 of word 10h.  If it is 0, firmware is done updating
         * 10h-12h.  Checksum may need to be fixed. */
        e1000_read_eeprom(unit, 0x10, 1, &eeprom_data);
        if ((eeprom_data & 0x10) == 0) 
		{
            /* Read 0x23 and check bit 15.  This bit is a 1 when the checksum
             * has already been fixed.  If the checksum is still wrong and this
             * bit is a 1, we need to return bad checksum.  Otherwise, we need
             * to set this bit to a 1 and update the checksum. */
            e1000_read_eeprom(unit, 0x23, 1, &eeprom_data);
            if ((eeprom_data & 0x8000) == 0) {
                eeprom_data |= 0x8000;
                e1000_write_eeprom(unit, 0x23, 1, &eeprom_data);
                e1000_update_eeprom_checksum(unit);
            }
        }
    }
    for (ix = 0; ix < EEPROM_WORD_SIZE; ix++) 
	{
		e1000_read_eeprom(unit, EEPROM_IA_ADDRESS + ix, 1, &checkSumTmp);
        checkSum += checkSumTmp;
	}
 
    if (checkSum == (UINT16)EEPROM_SUM)
        return OK;
 

	mvOsPrintf("Bad checksum 0x%x\n",checkSum); 
    return ERROR;
    }

/******************************************************************************
 * Calculates the EEPROM checksum and writes it to the EEPROM
 *
 * unit  - Device unit number.
 *
 * Sums the first 63 16 bit words of the EEPROM. Subtracts the sum from 0xBABA.
 * Writes the difference to word offset 63 of the EEPROM.
 *****************************************************************************/
void e1000_update_eeprom_checksum(int unit)
{
    UINT16 checksum = 0;
    UINT16 eeprom_data;
    EEPROM *eeprom = &geiResources[unit].eeprom;
    int i;

    mvOsPrintf("e1000_update_eeprom_checksum");

    for(i = 0; i < EEPROM_WORD_SIZE; i++) 
	{
        if(e1000_read_eeprom(unit, i, 1, &eeprom_data) < 0) 
		{
            mvOsPrintf("EEPROM Read Error\n");
            return ;
        }
        checksum += eeprom_data;
    }
    checksum = (UINT16) EEPROM_SUM - checksum;
    if(e1000_write_eeprom(unit, EEPROM_CHECKSUM_REG, 1, &checksum) < 0) 
	{
        mvOsPrintf("EEPROM Write Error\n");
        return ;
    } 
	else if (eeprom->type == EEPROM_FLASH) 
		{
        e1000_commit_shadow_ram(unit);
		}
}
/******************************************************************************
 * Flushes the cached eeprom to NVM. This is done by saving the modified values
 * in the eeprom cache and the non modified values in the currently active bank
 * to the new bank.
 *
 * unit  - Device unit number.
 * offset - offset of  word in the EEPROM to read
 * data - word read from the EEPROM
 * words - number of words to read
 *****************************************************************************/
LOCAL STATUS e1000_commit_shadow_ram(int unit)
{
    UINT32 attempts = 100000;
    UINT32 eecd = 0;
    UINT32 flop = 0;
    UINT32 i = 0;
    EEPROM *eeprom = &geiResources[unit].eeprom;

    /* The flop register will be used to determine if flash type is STM */
    flop = GEI_SYS_READ_REG(unit, INTEL_82543GC_FLOP);

    if (eeprom->mac_type == E1000_82573) 
	{
        for (i=0; i < attempts; i++) 
		{
            eecd = GEI_SYS_READ_REG(unit, INTEL_82543GC_EECD);
            if ((eecd & E1000_EECD_FLUPD) == 0) 
			{
                break;
            }
			sysUsDelay(5);
        }
        if (i == attempts) 
		{
            return ERROR;
        }

        /* If STM opcode located in bits 15:8 of flop, reset firmware */
        if ((flop & 0xFF00) == E1000_STM_OPCODE) 
		{
			GEI_SYS_WRITE_REG(unit, INTEL_82543GC_HICR, E1000_HICR_FW_RESET);
        }

        /* Perform the flash update */
		GEI_SYS_WRITE_REG(unit, INTEL_82543GC_EECD, eecd | E1000_EECD_FLUPD);

        for (i=0; i < attempts; i++) 
		{
            eecd = GEI_SYS_READ_REG(unit, INTEL_82543GC_EECD);
            if ((eecd & E1000_EECD_FLUPD) == 0) 
			{
                break;
            }
			sysUsDelay(5);
        }

        if (i == attempts) 
		{
            return ERROR;
        }
    }

    return OK;
}

/**************************************************************************
*
* sys543PhySpecRegsInit - Initialize PHY specific registers
*
* This routine initialize PHY specific registers
*
* RETURN: N/A
*/

LOCAL void sys543PhySpecRegsInit
    (
    PHY_INFO * pPhyInfo,    /* PHY's info structure pointer */
    UINT8     phyAddr       /* PHY's bus number */
    )
    {
    UINT16 regVal;          /* register value */
    UINT16 phyId1;          /* phy Id 1 */
    UINT16 phyId2;          /* phy ID 2 */
    UINT32 retVal;          /* return value */
    UINT32 phyOui = 0;      /* PHY's manufacture ID */
    UINT32 phyMode;         /* PHY mode number */
    
    /* Intel Pro1000T adaptor uses Alaska transceiver */

    /* read device ID to check Alaska chip available */

    MII_READ (phyAddr, MII_PHY_ID1_REG, &phyId1, retVal);
    
    MII_READ (phyAddr, MII_PHY_ID2_REG, &phyId2, retVal);
       
    phyOui =  phyId1 << 6 | phyId2 >> 10;

    phyMode = (phyId2 & MII_ID2_MODE_MASK) >> 4;

    if (phyOui == MARVELL_OUI_ID && (phyMode == MARVELL_ALASKA_88E1000 || 
                                     phyMode == MARVELL_ALASKA_88E1000S))
        {
         /* This is actually a Marvell Alaska 1000T transceiver */

         /* disable PHY's interrupt */         

         MII_READ (phyAddr, ALASKA_INT_ENABLE_REG, &regVal, retVal);
         regVal = 0;
         MII_WRITE (phyAddr, ALASKA_INT_ENABLE_REG, regVal, retVal);

         /* CRS assert on transmit */

         MII_READ (phyAddr, ALASKA_PHY_SPEC_CTRL_REG, &regVal, retVal);
         regVal |= ALASKA_PSCR_ASSERT_CRS_ON_TX;
         MII_WRITE (phyAddr, ALASKA_PHY_SPEC_CTRL_REG, regVal, retVal);

        /* set the clock rate when operate in 1000T mode */

         MII_READ (phyAddr, ALASKA_EXT_PHY_SPEC_CTRL_REG, &regVal, retVal);
         regVal |= ALASKA_EPSCR_TX_CLK_25;
         MII_WRITE (phyAddr, ALASKA_EXT_PHY_SPEC_CTRL_REG, regVal, retVal);
        }

     /* other PHYS .... */

     return;
     }

/*****************************************************************************
*
* sys543IntAck - acknowledge an 82543 interrupt
*
* This routine performs any 82543 interrupt acknowledge that may be
* required.  This typically involves an operation to some interrupt
* control hardware.
*
* This routine gets called from the 82543 driver's interrupt handler.
*
* This routine assumes that the PCI configuration information has already
* been setup.
*
* RETURNS: OK, or ERROR if the interrupt could not be acknowledged.
*/

LOCAL STATUS sys543IntAck
    (
    int    unit        /* unit number */
    )
    {
    GEI_RESOURCE *pReso = &geiResources [unit];

    switch (pReso->boardType)
    {
    case TYPE_PRO1000F_PCI:        /* handle PRO1000F/T LAN Adapter */
    case TYPE_PRO1000T_PCI:
        /* no addition work necessary for the PRO1000F/T */
        break;

    default:
        return (ERROR);
    }

    return (OK);
    }
/******************************************************************************
*
* sys543LocalToPciBusAdrs - convert a local address to a bus address
*
* This routine returns a PCIbus address for the LOCAL bus address.
*
*/
 
LOCAL UINT32 sys543LocalToPciBusAdrs
    (
    int unit,
    UINT32      adrs    /* Local Address */
    )
    {
    return (LOCAL2PCI_MEMIO(adrs));
    }
 
 
/******************************************************************************
*
* sys543PciBusToLocalAdrs - convert a bus address to a local address
*
* This routine returns a local address that is used to access the PCIbus.
* The bus address that is passed into this routine is the PCIbus address
* as it would be seen on the local bus.
*
*/
 
LOCAL UINT32 sys543PciBusToLocalAdrs
    (
    int unit,
    UINT32      adrs    /* PCI Address */
    )
    {
    return (PCI2LOCAL_MEMIO(adrs));
    }

/*****************************************************************************
*
* sys543IntEnable - enable 82543 interrupts
*
* This routine enables 82543 interrupts.  This may involve operations on
* interrupt control hardware.
*
* The 82543 driver calls this routine throughout normal operation to terminate
* critical sections of code.
*
* This routine assumes that the PCI configuration information has already
* been setup.
*
* RETURNS: OK, or ERROR if interrupts could not be enabled.
*/

LOCAL STATUS sys543IntEnable
    (
    int    unit        /* local unit number */
    )
    {
        GEI_RESOURCE *pReso;         /* chip resources */

        pReso = &geiResources [unit];

        /* enable adaptor interrupt */
        sysPciIntEnable(pReso->irq);
        
        return (OK);
    }

/*****************************************************************************
*
* sys543IntDisable - disable 82543 interrupts
*
* This routine disables 82543 interrupts.  This may involve operations on
* interrupt control hardware.
*
* The 82543 driver calls this routine throughout normal operation to enter
* critical sections of code.
*
* This routine assumes that the PCI configuration information has already
* been setup.
*
* RETURNS: OK, or ERROR if interrupts could not be disabled.
*/

LOCAL STATUS sys543IntDisable
    (
    int    unit        /* local unit number */
    )
    {
        GEI_RESOURCE *pReso;         /* chip resources */

        pReso     = &geiResources [unit];

        /* disable adaptor interrupt */
        sysPciIntDisable(pReso->irq);
        
        return (OK);
    }

/*****************************************************************************
*
* sys543Show - shows 82543 configuration 
*
* This routine shows (Intel Pro 1000F/T) adapters configuration 
*
* RETURNS: N/A
*/

void sys543Show
    (
    int    unit        /* unit number */
    )
    {
    int ix;
    GEI_RESOURCE *pReso = &geiResources [unit];

    if (unit >= geiUnits)
        {
        mvOsPrintf ("invalid unit number: %d\n", unit);
        return;
        }
 
    if ((pReso->boardType == DEV_ID_82541GI) || (pReso->boardType == DEV_ID_82541GI_LF))
        mvOsPrintf ("******** Intel PRO1000 MT Desktop Adapter (82541) ********\n");
    
    else if (pReso->boardType == DEV_ID_82544EI_COPPER)
        mvOsPrintf ("******** Intel PRO1000 XT Server Adapter (82544) *********\n");
    
    else if ((pReso->boardType == DEV_ID_82545GM_COPPER) || (pReso->boardType == PRO1000_546_BOARD))
        mvOsPrintf ("******** Intel PRO1000 MT Server Adapter (82545) *********\n");
    else if (pReso->boardType == DEV_ID_82572EI_COPPER)
        mvOsPrintf ("******** Intel PRO/1000 PT Server Adapter (82572) *********\n");
			   
    else
        mvOsPrintf ("****************** UNKNOWN Adaptor = %x********************** \n", pReso->boardType);
 
    mvOsPrintf ("  CSR PCI Membase address = 0x%x\n", pReso->memBaseLow);
 
    mvOsPrintf ("  Flash PCI Membase address = 0x%x\n", pReso->flashBase);

    mvOsPrintf ("  PCI bus no.= 0x%x, device no.= 0x%x, function no.= 0x%x\n", 
             pReso->pciBus, pReso->pciDevice, pReso->pciFunc);

    mvOsPrintf ("  IRQ = %d\n", pReso->irq);  

    if (pReso->iniStatus == ERROR)
        return;

    mvOsPrintf ("  Adapter Ethernet Address");

    for (ix = 0; ix < 6; ix ++)
        mvOsPrintf (":%2.2X", (UINT32)pReso->enetAddr[ix]);

    mvOsPrintf ("\n");

    mvOsPrintf ("  EEPROM Initialization Control Word 1 = 0x%4.4X\n", 
            pReso->eeprom_icw1);

    mvOsPrintf ("  EEPROM Initialization Control Word 2 = 0x%4.4X\n", 
            pReso->eeprom_icw2);

    mvOsPrintf ("**************************************************************\n");
    return;
    }


/*************************************************************************
*
* sys543EepromDump - Dump contents of EEPROM
*
* This routine dumps the 82543 EEPROM
*
* RETURNS: OK or ERROR
*/

STATUS sys543EepromDump
    (
    int unit,			/* device unit to be accessed. */
    char *adr			/* address of buffer to store data. */
    )
    {
    UINT32 ix;
    UINT32 checkSum = 0;

    mvOsPrintf("sys543EepromDump:\n\r");

	e1000_read_eeprom(unit, EEPROM_IA_ADDRESS, EEPROM_WORD_SIZE, (UINT16*)adr);

    for (ix = 0; ix < EEPROM_WORD_SIZE * 2; ix++) 
        {
		mvOsPrintf("%x: 0x%x\n\r", ix, adr[ix]);
        }

    mvOsPrintf("sys543EepromDump: checksum: 0x%x\n\r", checkSum);

    return (OK);
    }


/*************************************************************************
*
* sys1000NsDelay - Call sysUsDelay with a value of 1000 (ns).
*
* This routine delays for 1000 ns.
*
* RETURNS: NA
*/

void   sys1000NsDelay (void)
    {
    sysUsDelay (1);
    return;
    }



/***************************************************************************
* Description:     Determines if the onboard NVM is FLASH or EEPROM.
*
 * unit  - Device unit number.
****************************************************************************/
LOCAL STATUS e1000_is_onboard_nvm_eeprom(int unit, UINT16 mac_type)
{
    uint32_t eecd = 0;

    if(mac_type == E1000_82573) {
        eecd = GEI_SYS_READ_REG(unit, INTEL_82543GC_EECD);

        /* Isolate bits 15 & 16 */
        eecd = ((eecd >> 15) & 0x03);

        /* If both bits are set, device is Flash type */
        if(eecd == 0x03) {
            return FALSE;
        }
    }
    return TRUE;
}

/******************************************************************************
 * Reads a 16 bit word from the EEPROM using the EERD register.
 *
 * unit  - Device unit number.
 * offset - offset of  word in the EEPROM to read
 * data - word read from the EEPROM
 * words - number of words to read
 *****************************************************************************/
LOCAL UINT32 e1000_read_eeprom_eerd(int unit,UINT16 offset,UINT16 words,UINT16 *data)
{
    UINT32 i, eerd = 0;
    UINT32 error = 0;

    for (i = 0; i < words; i++) 
	{
        eerd = ((offset+i) << E1000_EEPROM_RW_ADDR_SHIFT) +
                         E1000_EEPROM_RW_REG_START;

		GEI_SYS_WRITE_REG(unit, INTEL_82543GC_EERD, eerd);
        error = e1000_poll_eerd_eewr_done(unit, E1000_EEPROM_POLL_READ);
        
        if(error) {
            break;
        }
        data[i] = (GEI_SYS_READ_REG(unit, INTEL_82543GC_EERD) >> E1000_EEPROM_RW_REG_DATA);
    }
    
    return error;
}

/******************************************************************************
 * Writes a 16 bit word from the EEPROM using the EEWR register.
 *
 * unit  - Device unit number.
 * offset - offset of  word in the EEPROM to read
 * data - word read from the EEPROM
 * words - number of words to read
 *****************************************************************************/
LOCAL UINT32 e1000_write_eeprom_eewr(int unit,UINT16 offset,UINT16 words,UINT16 *data)
{
    UINT32    register_value = 0;
    UINT32    i              = 0;
    UINT32    error          = 0;

    for (i = 0; i < words; i++) {
        register_value = (data[i] << E1000_EEPROM_RW_REG_DATA) | 
                         ((offset+i) << E1000_EEPROM_RW_ADDR_SHIFT) | 
                         E1000_EEPROM_RW_REG_START;

        error = e1000_poll_eerd_eewr_done(unit, E1000_EEPROM_POLL_WRITE);
        if(error) {
            break;
        }       

        GEI_SYS_WRITE_REG(unit, INTEL_82543GC_EEWR, register_value);
        
        error = e1000_poll_eerd_eewr_done(unit, E1000_EEPROM_POLL_WRITE);
        
        if(error != OK) 
		{
            break;
        }       
    }
    
    return error;
}

/******************************************************************************
 * Polls the status bit (bit 1) of the EERD to determine when the read is done.
 *
 * unit  - Device unit number.
 *****************************************************************************/
LOCAL STATUS e1000_poll_eerd_eewr_done(int unit, int eerd)
{
    UINT32 attempts = 100000;
    UINT32 i, reg = 0;
    STATUS done = ERROR;

    for(i = 0; i < attempts; i++) 
	{
        if(eerd == E1000_EEPROM_POLL_READ)
            reg = GEI_SYS_READ_REG(unit, INTEL_82543GC_EERD);
        else 
            reg = GEI_SYS_READ_REG(unit, INTEL_82543GC_EEWR);

        if(reg & E1000_EEPROM_RW_REG_DONE) 
		{
            done = OK;
            break;
        }
		sysUsDelay(5);
    }

    return done;
}

/******************************************************************************
 * Parent function for writing words to the different EEPROM types.
 *
 * unit  - Device unit number.
 * offset - offset within the EEPROM to be written to words
 * - number of words to write data - 16 bit word to be
 * written to the EEPROM
 *
 * If e1000_update_eeprom_checksum is not called after this function, the
 * EEPROM will most likely contain an invalid checksum.
 *****************************************************************************/
UINT32 e1000_write_eeprom(int unit,UINT16 offset,UINT16 words,UINT16 *data)
{
    EEPROM *eeprom = &geiResources[unit].eeprom;
    UINT32 status = 0;

    DEBUGFUNC("e1000_write_eeprom");

    /* A check for invalid values:  offset too large, too many words, and not
     * enough words.
     */
    if((offset >= eeprom->word_size) || 
	   (words > eeprom->word_size - offset) ||
	   (words == 0)) 
	{
        mvOsPrintf("\"words\" parameter out of bounds\n");
        return ERROR;
    }

    /* 82573 writes only through eewr */
    if(eeprom->use_eewr == TRUE)
        return e1000_write_eeprom_eewr(unit, offset, words, data);

    /* Prepare the EEPROM for writing  */
    if (e1000_acquire_eeprom(unit) != OK)
        return ERROR;

    if(eeprom->type == EEPROM_MICROWIRE) 
	{
        status = e1000_write_eeprom_microwire(unit, offset, words, data);
    } else 
	{
        status = e1000_write_eeprom_spi(unit, offset, words, data);
        sysMsDelay(10);
    }

    /* Done with writing */
    e1000_release_eeprom(unit);

    return status;
}

/******************************************************************************
 * Writes a 16 bit word to a given offset in an SPI EEPROM.
 *
 * unit  - Device unit number.
 * offset - offset within the EEPROM to be written to
 * words - number of words to write
 * data - pointer to array of 8 bit words to be written to the EEPROM
 *
 *****************************************************************************/
UINT32 e1000_write_eeprom_spi(int unit,UINT16 offset,UINT16 words,UINT16 *data)
{
    EEPROM *eeprom = &geiResources[unit].eeprom;
    UINT16 widx = 0;

    DEBUGFUNC("e1000_write_eeprom_spi");

    while (widx < words) 
	{
        uint8_t write_opcode = EEPROM_WRITE_OPCODE_SPI;

        if(e1000_spi_eeprom_ready(unit)) 
			return ERROR;

        e1000_standby_eeprom(unit);

        /*  Send the WRITE ENABLE command (8 bit opcode )  */
        e1000_shift_out_ee_bits(unit, EEPROM_WREN_OPCODE_SPI,eeprom->opcode_bits);

        e1000_standby_eeprom(unit);

        /* Some SPI eeproms use the 8th address bit embedded in the opcode */
        if((eeprom->address_bits == 8) && (offset >= 128))
            write_opcode |= EEPROM_A8_OPCODE_SPI;

        /* Send the Write command (8-bit opcode + addr) */
        e1000_shift_out_ee_bits(unit, write_opcode, eeprom->opcode_bits);

        e1000_shift_out_ee_bits(unit, (UINT16)((offset + widx)*2),eeprom->address_bits);

        /* Send the data */

        /* Loop to allow for up to whole page write (32 bytes) of eeprom */
        while (widx < words) 
		{
            UINT16 word_out = data[widx];
            word_out = (word_out >> 8) | (word_out << 8);
            e1000_shift_out_ee_bits(unit, word_out, 16);
            widx++;

            /* Some larger eeprom sizes are capable of a 32-byte PAGE WRITE
             * operation, while the smaller eeproms are capable of an 8-byte
             * PAGE WRITE operation.  Break the inner loop to pass new address
             */
            if((((offset + widx)*2) % eeprom->page_size) == 0) 
			{
                e1000_standby_eeprom(unit);
                break;
            }
        }
    }

    return OK;
}

/******************************************************************************
 * Writes a 16 bit word to a given offset in a Microwire EEPROM.
 *
 * unit  - Device unit number.
 * offset - offset within the EEPROM to be written to
 * words - number of words to write
 * data - pointer to array of 16 bit words to be written to the EEPROM
 *
 *****************************************************************************/
UINT32 e1000_write_eeprom_microwire(int unit,UINT16 offset,UINT16 words,UINT16 *data)
{
    EEPROM *eeprom = &geiResources[unit].eeprom;
    uint32_t eecd;
    UINT16 words_written = 0;
    UINT16 i = 0;

    DEBUGFUNC("e1000_write_eeprom_microwire");

    /* Send the write enable command to the EEPROM (3-bit opcode plus
     * 6/8-bit dummy address beginning with 11).  It's less work to include
     * the 11 of the dummy address as part of the opcode than it is to shift
     * it over the correct number of bits for the address.  This puts the
     * EEPROM into write/erase mode.
     */
    e1000_shift_out_ee_bits(unit, EEPROM_EWEN_OPCODE_MICROWIRE,(UINT16)(eeprom->opcode_bits + 2));

    e1000_shift_out_ee_bits(unit, 0, (UINT16)(eeprom->address_bits - 2));

    /* Prepare the EEPROM */
    e1000_standby_eeprom(unit);

    while (words_written < words) 
	{
        /* Send the Write command (3-bit opcode + addr) */
        e1000_shift_out_ee_bits(unit, EEPROM_WRITE_OPCODE_MICROWIRE,eeprom->opcode_bits);

        e1000_shift_out_ee_bits(unit, (UINT16)(offset + words_written),eeprom->address_bits);

        /* Send the data */
        e1000_shift_out_ee_bits(unit, data[words_written], 16);

        /* Toggle the CS line.  This in effect tells the EEPROM to execute
         * the previous command.
         */
        e1000_standby_eeprom(unit);

        /* Read DO repeatedly until it is high (equal to '1').  The EEPROM will
         * signal that the command has been completed by raising the DO signal.
         * If DO does not go high in 10 milliseconds, then error out.
         */
        for(i = 0; i < 200; i++) 
		{
            eecd = GEI_SYS_READ_REG(unit, INTEL_82543GC_EECD);
            if(eecd & E1000_EECD_DO) 
				break;
			sysUsDelay(5);
        }
        if(i == 200) 
		{
            mvOsPrintf("EEPROM Write did not complete\n");
            return ERROR;
        }

        /* Recover from write */
        e1000_standby_eeprom(unit);

        words_written++;
    }

    /* Send the write disable command to the EEPROM (3-bit opcode plus
     * 6/8-bit dummy address beginning with 10).  It's less work to include
     * the 10 of the dummy address as part of the opcode than it is to shift
     * it over the correct number of bits for the address.  This takes the
     * EEPROM out of write/erase mode.
     */
    e1000_shift_out_ee_bits(unit, EEPROM_EWDS_OPCODE_MICROWIRE,(UINT16)(eeprom->opcode_bits + 2));

    e1000_shift_out_ee_bits(unit, 0, (UINT16)(eeprom->address_bits - 2));

    return OK;
}
#else
STATUS sys82543BoardInit(int unit, ADAPTOR_INFO *pBoard)
{
	return (ERROR);
}

#endif /*defined(INCLUDE_GEI_END) && (INCLUDE_NETWORK) &&  (INCLUDE_END)) */
