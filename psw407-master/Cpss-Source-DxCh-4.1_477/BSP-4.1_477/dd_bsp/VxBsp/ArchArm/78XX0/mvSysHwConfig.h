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
/*******************************************************************************
 * mvSysHwConfig.h - Marvell system HW configuration file
 *
 * DESCRIPTION:
 *       None.
 *
 * DEPENDENCIES:
 *       None.
 *
 *******************************************************************************/
#ifndef __INCmvSysHwConfigh
#define __INCmvSysHwConfigh

/* includes */
#include "mvCommon.h"

/* defines  */

/* System targers address window definitions (base)                */
#define SDRAM_CS0_BASE  0x00000000      /* Actual base set be auto detection */
#define SDRAM_CS1_BASE  0x10000000      /* Actual base set be auto detection */
#define SDRAM_CS2_BASE  0x20000000      /* Actual base set be auto detection */
#define SDRAM_CS3_BASE  0x30000000      /* Actual base set be auto detection */



/****************************************************************/
/************* General    configuration ********************/
/****************************************************************/
#define CONFIG_MARVELL  1

/* Disable the DEVICE BAR in the PEX */
#define MV_DISABLE_PEX_DEVICE_BAR

/* Allow the usage of early printings during initialization */
#define MV_INCLUDE_EARLY_PRINTK

/* Enable Clock Power Control (defined in makefile.user */
#define MV_INCLUDE_CLK_PWR_CNTRL

/* Enable PEX virtual bridge */
#ifndef LION_RD
#define PCIE_VIRTUAL_BRIDGE_SUPPORT
#endif

/* System targers address window definitions (size)                */
#ifdef RD_78XX0_AMC_ID
#define SDRAM_CS0_SIZE  _512M
#define SDRAM_CS1_SIZE  0    
#define SDRAM_CS2_SIZE  0    
#define SDRAM_CS3_SIZE  0    
#else
#define SDRAM_CS0_SIZE  _256M      /* Actual size set be auto detection */
#define SDRAM_CS1_SIZE  _256M      /* Actual size set be auto detection */
#define SDRAM_CS2_SIZE  _256M      /* Actual size set be auto detection */
#define SDRAM_CS3_SIZE  _256M      /* Actual size set be auto detection */
#endif

#if defined (MV78100)
#define DEVICE_CS0_BASE 0xF8000000  /* NOR FLASH  */
#define DEVICE_CS3_BASE 0xFA000000  /* SPI FLASH  */ 
#define DEVICE_CS1_BASE 0xFC000000  /* 7 segment  */ 
#define DEVICE_CS2_BASE 0xFD000000  /* NAND FLASH */ 

#ifdef DB_MV78200_A_AMC
#define BOOTDEV_CS_BASE 0xFF800000
#else
#define BOOTDEV_CS_BASE 0xFE000000
#endif

#define DEVICE_CS1_SIZE _1M
#define DEVICE_CS3_SIZE 0
#define DEVICE_CS2_SIZE _1M 
#define DEVICE_CS0_SIZE _32M

#ifdef DB_MV78200_A_AMC
#define BOOTDEV_CS_SIZE _8M
#else
#define BOOTDEV_CS_SIZE _32M
#endif

#define DEVICE_SPI_BASE 0xFA000000  /* SPI FLASH  */  
#define DEVICE_SPI_SIZE _8M

#endif
#if defined (MV78200)
#define DEVICE_CS0_BASE 0xFA800000  /* no such    */
#define DEVICE_CS2_BASE 0xFA900000  /* no such    */  
#define DEVICE_CS1_BASE 0xFA000000  /* 7 segment  */ 
#define DEVICE_CS3_BASE 0xF8000000  /* NAND FLASH */  
#define BOOTDEV_CS_BASE 0xFE000000  /* NOR FLASH  */

#define DEVICE_CS0_SIZE 0
#define DEVICE_CS1_SIZE _1M
#define DEVICE_CS2_SIZE 0 
#define DEVICE_CS3_SIZE _32M 
#define BOOTDEV_CS_SIZE _32M

#define DEVICE_SPI_BASE 0xFC000000
#define DEVICE_SPI_SIZE _8M

#endif

#ifdef DB_MV78200_A_AMC
#define PCI0_IO_SIZE    0
#define PCI1_IO_SIZE    0
#define PCI2_IO_SIZE    0
#define PCI3_IO_SIZE    0
#define PCI4_IO_SIZE    0
#define PCI5_IO_SIZE    0
#define PCI6_IO_SIZE    0
#define PCI7_IO_SIZE    0
#define PCI8_IO_SIZE    0

#else

#define PCI0_IO_SIZE    _2M
#define PCI1_IO_SIZE    _2M
#define PCI2_IO_SIZE    0
#define PCI3_IO_SIZE    _2M
#define PCI4_IO_SIZE    _2M
#define PCI5_IO_SIZE    0
#define PCI6_IO_SIZE    0
#define PCI7_IO_SIZE    0
#define PCI8_IO_SIZE    0
#endif


#ifdef DB_MV78200_A_AMC
#ifdef LION_RD
#define PCI0_MEM0_SIZE   _128M
#define PCI1_MEM0_SIZE   _128M
#define PCI2_MEM0_SIZE   _128M
#define PCI3_MEM0_SIZE   _128M
#define PCI4_MEM0_SIZE   0
#else
#define PCI0_MEM0_SIZE   0
#define PCI1_MEM0_SIZE   0
#define PCI2_MEM0_SIZE   0
#define PCI3_MEM0_SIZE   0
#define PCI4_MEM0_SIZE   _1G  
#endif
#define PCI5_MEM0_SIZE   0
#define PCI6_MEM0_SIZE   0
#define PCI7_MEM0_SIZE   0
#define PCI8_MEM0_SIZE   0


#else

#define PCI0_MEM0_SIZE  _64M
#define PCI1_MEM0_SIZE  _64M
#define PCI2_MEM0_SIZE   0
#define PCI3_MEM0_SIZE   _64M
#define PCI4_MEM0_SIZE   _32M  
#define PCI5_MEM0_SIZE   0
#define PCI6_MEM0_SIZE   0
#define PCI7_MEM0_SIZE   0
#define PCI8_MEM0_SIZE   0
#endif

#define ALL_PCI_MEM_SIZE (PCI0_MEM0_SIZE +      \
                          PCI1_MEM0_SIZE +      \
                          PCI2_MEM0_SIZE +      \
                          PCI3_MEM0_SIZE +      \
                          PCI4_MEM0_SIZE +      \
                          PCI5_MEM0_SIZE +      \
                          PCI6_MEM0_SIZE +      \
                          PCI7_MEM0_SIZE +      \
                          PCI8_MEM0_SIZE )

 
#ifdef DB_MV78200_A_AMC
#ifdef LION_RD
#define PCI0_MEM0_BASE  0x80000000
#define PCI1_MEM0_BASE  (PCI0_MEM0_BASE + PCI0_MEM0_SIZE)
#define PCI2_MEM0_BASE  (PCI1_MEM0_BASE + PCI1_MEM0_SIZE) 
#define PCI3_MEM0_BASE  (PCI2_MEM0_BASE + PCI2_MEM0_SIZE) 
#define PCI4_MEM0_BASE  (PCI3_MEM0_BASE + PCI3_MEM0_SIZE)  
#define PCI5_MEM0_BASE  (PCI4_MEM0_BASE + PCI4_MEM0_SIZE)
#define PCI6_MEM0_BASE  (PCI5_MEM0_BASE + PCI5_MEM0_SIZE)
#define PCI7_MEM0_BASE  (PCI6_MEM0_BASE + PCI6_MEM0_SIZE)
#define PCI8_MEM0_BASE  (PCI7_MEM0_BASE + PCI7_MEM0_SIZE)
#else
#define PCI0_MEM0_BASE  0
#define PCI1_MEM0_BASE  0
#define PCI2_MEM0_BASE  0
#define PCI3_MEM0_BASE  0
#define PCI4_MEM0_BASE  0x80000000
#define PCI5_MEM0_BASE  0
#define PCI6_MEM0_BASE  0
#define PCI7_MEM0_BASE  0
#define PCI8_MEM0_BASE  0
#endif
#else

#define PCI0_MEM0_BASE  0xC0000000 
#define PCI1_MEM0_BASE  (PCI0_MEM0_BASE + PCI0_MEM0_SIZE)
#define PCI2_MEM0_BASE  (PCI1_MEM0_BASE + PCI1_MEM0_SIZE) 
#define PCI3_MEM0_BASE  (PCI2_MEM0_BASE + PCI2_MEM0_SIZE) 
#define PCI4_MEM0_BASE  (PCI3_MEM0_BASE + PCI3_MEM0_SIZE)  
#define PCI5_MEM0_BASE  (PCI4_MEM0_BASE + PCI4_MEM0_SIZE)
#define PCI6_MEM0_BASE  (PCI5_MEM0_BASE + PCI5_MEM0_SIZE)
#define PCI7_MEM0_BASE  (PCI6_MEM0_BASE + PCI6_MEM0_SIZE)
#define PCI8_MEM0_BASE  (PCI7_MEM0_BASE + PCI7_MEM0_SIZE)
#endif

#ifdef  DB_MV78200_A_AMC
#define PCI0_IO_BASE    0
#define PCI1_IO_BASE    0
#define PCI2_IO_BASE    0
#define PCI3_IO_BASE    0
#define PCI4_IO_BASE    0
#define PCI5_IO_BASE    0
#define PCI6_IO_BASE    0
#define PCI7_IO_BASE    0
#define PCI8_IO_BASE    0


#else

#define PCI0_IO_BASE    0xF0000000
#define PCI1_IO_BASE    (PCI0_IO_BASE+PCI0_IO_SIZE) 
#define PCI2_IO_BASE    (PCI1_IO_BASE+PCI1_IO_SIZE) 
#define PCI3_IO_BASE    (PCI2_IO_BASE+PCI2_IO_SIZE) 
#define PCI4_IO_BASE    (PCI3_IO_BASE+PCI3_IO_SIZE) 
#define PCI5_IO_BASE    (PCI4_IO_BASE+PCI4_IO_SIZE) 
#define PCI6_IO_BASE    (PCI5_IO_BASE+PCI5_IO_SIZE) 
#define PCI7_IO_BASE    (PCI6_IO_BASE+PCI6_IO_SIZE) 
#define PCI8_IO_BASE    (PCI7_IO_BASE+PCI7_IO_SIZE) 

#endif
                     

#ifdef  DB_MV78200_A_AMC
#ifdef LION_RD
#define PCI_IF0_MEM_BASE    PCI0_MEM0_BASE    
#define PCI_IF0_MEM_SIZE    PCI0_MEM0_SIZE
#define PCI_IF0_IO_BASE     PCI0_IO_BASE
#define PCI_IF0_IO_SIZE     PCI0_IO_SIZE
#else
#define PCI_IF0_MEM_BASE    PCI4_MEM0_BASE
#define PCI_IF0_MEM_SIZE    PCI4_MEM0_SIZE
#define PCI_IF0_IO_BASE     PCI4_IO_BASE
#define PCI_IF0_IO_SIZE     PCI4_IO_SIZE
#endif
#else

#define PCI_IF0_MEM_BASE    PCI0_MEM0_BASE
#define PCI_IF0_MEM_SIZE    PCI0_MEM0_SIZE
#define PCI_IF0_IO_BASE     PCI0_IO_BASE
#define PCI_IF0_IO_SIZE     PCI0_IO_SIZE
#endif


#define PCI_IF1_MEM_BASE    PCI1_MEM0_BASE
#define PCI_IF1_MEM_SIZE    PCI1_MEM0_SIZE
#define PCI_IF1_IO_BASE     PCI1_IO_BASE
#define PCI_IF1_IO_SIZE     PCI1_IO_SIZE


#define INTER_REGS_BASE 0xF1000000

/* Crypto memory space */
#define CRYPTO_BASE   (INTER_REGS_BASE + 0x1000000)
#define CRYPTO_SIZE   _1M


/* enable IPM support - nesting interrupts */
#undef MV_IPM_ENABLE

/* DRAM detection stuff */
#define MV_DRAM_AUTO_SIZE

/* These addresses defines the place where global parameters will be placed */
/* in case running from ROM. We Use SYS_MEM_TOP. See bootInit.c file    */
#define DRAM_CONFIG_ROM_ADDR  ROM_CONFIG_ADRS

/* We use the following registers to store DRAM interface pre configuration   */
/* auto-detection results                           */
#define DRAM_BUF_REG0 0x60810 /* sdram bank 0 size        CPU_CS0_SIZE_REG        0x2010  */
#define DRAM_BUF_REG1 0x60814 /* sdram config           SDRAM_CONFIG_REG        0x1400  */
#define DRAM_BUF_REG2   0x60818 /* sdram mode             SDRAM_MODE_REG          0x141c  */
#define DRAM_BUF_REG3 0x6081c /* dunit control low        SDRAM_DUNIT_CTRL_REG      0x1404  */          
#define DRAM_BUF_REG4 0x60820 /* sdram address control      SDRAM_ADDR_CTRL_REG       0x1410  */
#define DRAM_BUF_REG5 0x60824 /* sdram timing control low   SDRAM_TIMING_CTRL_LOW_REG   0x1408  */
#define DRAM_BUF_REG6 0x60828 /* sdram timing control high  SDRAM_TIMING_CTRL_HIGH_REG    0x140c  */
#define DRAM_BUF_REG7 0x6082c /* sdram ODT control low      DDR2_SDRAM_ODT_CTRL_LOW_REG   0x1494  */
#define DRAM_BUF_REG8 0x60830 /* sdram ODT control high     DDR2_SDRAM_ODT_CTRL_HIGH_REG  0x1498  */
#define DRAM_BUF_REG9 0x60834 /* sdram Dunit ODT control    DDR2_DUNIT_ODT_CONTROL_REG    0x149c  */
#define DRAM_BUF_REG10  0x60838 /* sdram Extended Mode      SDRAM_EXTENDED_MODE_REG     0x1420  */
#define DRAM_BUF_REG11  0x6083c /* sdram Ddr2 Timing Low    SDRAM_DDR2_TIMING_LO_REG    0x1428  */
#define DRAM_BUF_REG12  0x60870 /* sdram Ddr2 Timing High   SDRAM_DDR2_TIMING_HI_REG    0x147C  */
#define DRAM_BUF_REG13  0x60874 /* dunit control high                     */
#define DRAM_BUF_REG14  0x60878 /* if == '1' second dimm exist  */


 

/* System DRAM cache coherency configuration */
/* The DRAM_COHERENCY macro set the CPU cache mode using sysPhyMemDesc[]    */
/* and the Discovery device HW cache coherency.                             */
#define MV_CACHE_COHERENCY MV_CACHE_COHER_SW     
#define DRAM_COHERENCY      MV_CACHE_COHER_SW    

#define ETHER_DRAM_COHER    DRAM_COHERENCY


/* PCI stuff */
/* Local bus and device number of PCI0/1*/
#define PCI_HOST_BUS_NUM(pciIf) (pciIf)
#define PCI_HOST_DEV_NUM(pciIf) 0

#define PEX_HOST_BUS_NUM(pexIf) (pexIf)
#define PEX_HOST_DEV_NUM(pexIf) 0

/* select interface mode PCI_IF_MODE_HOST or PCI_IF_MODE_DEVICE */
#ifdef RD_MV645XX
#define PCI0_IF_MODE    PCI_IF_MODE_DEVICE
#else                                                
#define PCI0_IF_MODE    PCI_IF_MODE_HOST
#endif /* RD_MV645XX */

#define PCI_ARBITER_CTRL    /* Use/unuse the Marvell integrated PCI arbiter */

/****************************************************************/
/************* CESA configuration ********************/
/****************************************************************/ 
#ifdef MV_INCLUDE_CESA

#define MV_CESA_MAX_CHAN               1

/* Use 2K of SRAM */
#define MV_CESA_MAX_BUF_SIZE           1600

#endif /* MV_INCLUDE_CESA */ 

/************* Ethernet driver configuration ********************/

/* un-comment if you want to perform tx_done from within the poll function */
/* #define ETH_TX_DONE_ISR */

/* put descriptors in uncached memory */
/* #define ETH_DESCR_UNCACHED */

/* Descriptors location: DRAM/internal-SRAM */
#define ETH_DESCR_IN_SDRAM
#undef  ETH_DESCR_IN_SRAM    /* No integrated SRAM in 88Fxx81 devices */

#if defined(ETH_DESCR_IN_SRAM)
#if defined(ETH_DESCR_UNCACHED)
#define ETH_DESCR_CONFIG_STR    "Uncached descriptors in integrated SRAM"
#else
#define ETH_DESCR_CONFIG_STR    "Cached descriptors in integrated SRAM"
#endif
#elif defined(ETH_DESCR_IN_SDRAM)
#if defined(ETH_DESCR_UNCACHED)
#define ETH_DESCR_CONFIG_STR    "Uncached descriptors in DRAM"
#else
#define ETH_DESCR_CONFIG_STR    "Cached descriptors in DRAM"
#endif
#else 
#error "Ethernet descriptors location undefined"
#endif /* ETH_DESCR_IN_SRAM or ETH_DESCR_IN_SDRAM*/

#define MV_ETH_TX_Q_NUM     1
#define MV_ETH_RX_Q_NUM   1

/* port's default queueus */
#define ETH_DEF_TXQ         0
#define ETH_DEF_RXQ         0 

/* a few extra Tx descriptors, because we stop the interface for Tx in advance */        
#define MV_ETH_EXTRA_TX_DESCR     20 

#if (MV_ETH_RX_Q_NUM > 1)
#define ETH_NUM_OF_RX_DESCR         64
#else
#   define ETH_NUM_OF_RX_DESCR     128
#endif

#define ETH_NUM_OF_TX_DESCR        (ETH_NUM_OF_RX_DESCR*2 + MV_ETH_EXTRA_TX_DESCR)


#define INT_LVL_XCAT2_SWITCH   55   /* Switch MG interrupt */


#endif /* __INCmvSysHwConfigh */
