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
* db64660.h - Header File for : Development Board BSP definition module.
*
* DESCRIPTION:
*       This file contains I/O addresses and related constants.
*
* DEPENDENCIES:
*       None.
*
*******************************************************************************/

#ifndef __INCdb78100h
#define __INCdb78100h

#ifdef __cplusplus
extern "C" {
#endif

/* includes */
#include "mvBoardEnvSpec.h"
#include "mvCtrlEnvSpec.h"
/* defines  */

/* Platform clock settings */
#define DEC_CLOCK_FREQ  sysClockRate


/* create a single macro INCLUDE_MMU */

#if defined(INCLUDE_MMU_BASIC) || defined(INCLUDE_MMU_FULL)
    #define INCLUDE_MMU
#endif

/* Only one can be selected, FULL overrides BASIC */

#ifdef INCLUDE_MMU_FULL
    #undef INCLUDE_MMU_BASIC
#endif

#define WRONG_CPU_MSG "\nThis VxWorks image was not compiled for this CPU!\n"

/* Interrupt levels */
    #define INT_LVL_MIN 			0	

/* Main Interrupt Error Cause Register bits */


/* Main Interrupt Cause Register bits */
#define INT_LVL_ERRSUM  	   	0	/* Summary of Main Interrupt Error Cause register*/
#define INT_LVL_SPI	   			1	/* SPI interrupt	*/
#define INT_LVL_TWSI0  			2	/* TWSI0 interrupt  */
#define INT_LVL_TWSI1 			3	/* TWSI1 interrupt 	*/
#define INT_LVL_IDMA0  			4	/* IDMA0 interrupt	*/
#define INT_LVL_IDMA1  			5	/* IDMA1 interrupt	*/
#define INT_LVL_IDMA2  			6	/* IDMA2 interrupt	*/
#define INT_LVL_IDMA3  			7	/* IDMA3 interrupt	*/
#define INT_LVL_TIMER0   		8	/* Timer0 interrupt */
#define INT_LVL_TIMER1   		9	/* Timer1 interrupt */
#define INT_LVL_TIMER2   	   10 	/* Timer2 interrupt */
#define INT_LVL_TIMER3   	   11 	/* Timer3 interrupt */
#define INT_LVL_UART0   	   12 	/* UART0 interrupt  */
#define INT_LVL_UART1   	   13 	/* UART1 interrupt  */
#define INT_LVL_UART2   	   14 	/* UART2 interrupt  */
#define INT_LVL_UART3   	   15 	/* UART3 interrupt  */
#define INT_LVL_USB0   	       16 	/* USB0 interrupt   */
#define INT_LVL_USB1   	       17 	/* USB1 interrupt   */
#define INT_LVL_USB2   	       18 	/* USB2 interrupt   */
#define INT_LVL_CESA	   	   19	/* Crypto engine completion interrupt */
#define INT_LVL_x20   		   20	/* 20 reserved	*/
#define INT_LVL_x21   		   21	/* 21 reserved	*/
#define INT_LVL_XOR0   		   22	/* XOR engine 0 completion interrupt */
#define INT_LVL_XOR1   		   23	/* XOR engine 1 completion interrupt */
#define INT_LVL_x24   		   24	/* 24 reserved	*/
#define INT_LVL_x25   		   25	/* 24 reserved	*/
#define INT_LVL_SATA_CTRL  	   26	/* SATA controller interrupt     	*/
#define INT_LVL_x27   		   31	/* 27-31 reserved	*/

#define INT_LVL_PEX00_ABCD	   32	/* PCI Express port0.0 INTA/B/C/D assert message interrupt. */
#define INT_LVL_PEX01_ABCD	   33	/* PCI Express port0.1 INTA/B/C/D assert message interrupt. */
#define INT_LVL_PEX02_ABCD	   34	/* PCI Express port0.2 INTA/B/C/D assert message interrupt. */
#define INT_LVL_PEX03_ABCD	   35	/* PCI Express port0.3 INTA/B/C/D assert message interrupt. */
#define INT_LVL_PEX10_ABCD	   36	/* PCI Express port1.0 INTA/B/C/D assert message interrupt. */
#define INT_LVL_PEX11_ABCD	   37	/* PCI Express port1.1 INTA/B/C/D assert message interrupt. */
#define INT_LVL_PEX12_ABCD	   38	/* PCI Express port1.2 INTA/B/C/D assert message interrupt. */
#define INT_LVL_PEX13_ABCD	   39	/* PCI Express port1.3 INTA/B/C/D assert message interrupt. */
#define INT_LVL_GBE0_SUM   	   40	/* Gigabit Ethernet Port 0.0 summary	 	*/
#define INT_LVL_GBE0_RX		   41	/* Gigabit Ethernet Port 0.0 Rx summary    	*/
#define INT_LVL_GBE0_TX        42	/* Gigabit Ethernet Port 0.0 Tx summary    	*/
#define INT_LVL_GBE0_MISC      43	/* Gigabit Ethernet Port 0.0 misc. summary 	*/
#define INT_LVL_GBE1_SUM   	   44	/* Gigabit Ethernet Port 0.1 summary	 	*/
#define INT_LVL_GBE1_RX		   45	/* Gigabit Ethernet Port 0.1 Rx summary    	*/
#define INT_LVL_GBE1_TX        46	/* Gigabit Ethernet Port 0.1 Tx summary    	*/
#define INT_LVL_GBE1_MISC      47	/* Gigabit Ethernet Port 0.1 misc. summary 	*/
#define INT_LVL_x48   		   48	/* 48-55 reserved							*/
#define INT_LVL_P0_GPIO0_7	   56	/* GPIO[00:07] Interrupt 						*/
#define INT_LVL_P0_GPIO8_15	   57	/* GPIO[O8:15] Interrupt 						*/
#define INT_LVL_P0_GPIO16_23   58	/* GPIO[16:23] Interrupt 						*/
#define INT_LVL_P0_GPIO24_31   59	/* GPIO[24:31] Interrupt 						*/
#define INT_LVL_DB_IN 		   60	/* Summary of Inbound Doorbell Cause register  	*/
#define INT_LVL_DB_OUT		   61	/* Summary of Outbound Doorbell Cause register 	*/
#define INT_LVL_x62   		   62	/* 62-63 reserved							*/

#define INT_LVL_CRYPT_ERR	   64	/* Crypto engine error 												*/
#define INT_LVL_DEV_ERR        65	/* Device bus error (DEV_READYn timer) 								*/
#define INT_LVL_DMA_ERR	       66	/* DMA error (address decoding and protection) 						*/
#define INT_LVL_CPU_ERR	       67	/* CPU error (parity, address decoding and protection) 				*/
#define INT_LVL_PEX0_ERR       68	/* PCI-Express port0 Error (summary of PCI Express Cause register)  */
#define INT_LVL_PEX1_ERR       69	/* PCI-Express port1 Error (summary of PCI Express Cause register)  */
#define INT_LVL_GBE_ERR	       70	/* Gigabit Ethernet error (address decoding and protection) Summary of 
							         Ethernet Unit Interrupt Cause (EUIC) registers of all GbE ports. */
#define INT_LVL_err_x7	       71	/* reserved */
#define INT_LVL_USB_ERR	       72	/* USB error (address decoding) Summary of errors from all USB ports */
#define INT_LVL_DRAM_ERR       73	/* DRAM ECC error   												 */
#define INT_LVL_XOR_ERR	       74	/* XOR engine error 												 */
#define INT_LVL_ERR_xx11 	   75   /* 11-14 (75,76,77,78 )Reserved */
#define INT_LVL_WD_ERR	 	   79   /* WD Timer interrupt */
#define INT_LVL_ERR_xx16 	   80   /* 16-31 Reserved */


#define INT_LVL_PEX0_INTA		96

/* PEX interrupt levels */
#define INT_LVL_PEX00_INTA		96
#define INT_LVL_PEX00_INTB		(INT_LVL_PEX00_INTA+1) 
#define INT_LVL_PEX00_INTC		(INT_LVL_PEX00_INTA+2)
#define INT_LVL_PEX00_INTD	    (INT_LVL_PEX00_INTA+3)

/* PEX1 interrupt levels */
#define INT_LVL_PEX01_INTA		(INT_LVL_PEX00_INTA+4)
#define INT_LVL_PEX01_INTB		(INT_LVL_PEX00_INTA+5) 
#define INT_LVL_PEX01_INTC		(INT_LVL_PEX00_INTA+6) 
#define INT_LVL_PEX01_INTD		(INT_LVL_PEX00_INTA+7) 

/* PEX2 interrupt levels */
#define INT_LVL_PEX02_INTA		(INT_LVL_PEX01_INTA+4)
#define INT_LVL_PEX02_INTB		(INT_LVL_PEX01_INTA+5)
#define INT_LVL_PEX02_INTC		(INT_LVL_PEX01_INTA+6) 
#define INT_LVL_PEX02_INTD		(INT_LVL_PEX01_INTA+7) 
 
/* PEX3 interrupt levels */      
#define INT_LVL_PEX03_INTA		(INT_LVL_PEX02_INTA+4) 
#define INT_LVL_PEX03_INTB		(INT_LVL_PEX02_INTA+5) 
#define INT_LVL_PEX03_INTC		(INT_LVL_PEX02_INTA+6) 
#define INT_LVL_PEX03_INTD		(INT_LVL_PEX02_INTA+7) 

/* PEX4 interrupt levels */
#define INT_LVL_PEX10_INTA		(INT_LVL_PEX03_INTA+4) 
#define INT_LVL_PEX10_INTB		(INT_LVL_PEX03_INTA+5) 
#define INT_LVL_PEX10_INTC		(INT_LVL_PEX03_INTA+6) 
#define INT_LVL_PEX10_INTD		(INT_LVL_PEX03_INTA+7) 
												   
/* PEX5 interrupt levels */
#define INT_LVL_PEX11_INTA		(INT_LVL_PEX10_INTA+4) 
#define INT_LVL_PEX11_INTB		(INT_LVL_PEX10_INTA+5) 
#define INT_LVL_PEX11_INTC		(INT_LVL_PEX10_INTA+6) 
#define INT_LVL_PEX11_INTD		(INT_LVL_PEX10_INTA+7) 
												   
/* PEX6 interrupt levels */
#define INT_LVL_PEX12_INTA		(INT_LVL_PEX11_INTA+4) 
#define INT_LVL_PEX12_INTB		(INT_LVL_PEX11_INTA+5) 
#define INT_LVL_PEX12_INTC		(INT_LVL_PEX11_INTA+6) 
#define INT_LVL_PEX12_INTD		(INT_LVL_PEX11_INTA+7) 

/* PEX7 interrupt levels */
#define INT_LVL_PEX13_INTA		(INT_LVL_PEX13_INTA+4) 
#define INT_LVL_PEX13_INTB		(INT_LVL_PEX13_INTA+5) 
#define INT_LVL_PEX13_INTC		(INT_LVL_PEX13_INTA+6) 
#define INT_LVL_PEX13_INTD		(INT_LVL_PEX13_INTA+7) 
											      

#define INT_LVL_PCI            129 /* PCI int A,B,C,D hooked to the same pin */



#define INT_LVL_GBE_PORT_RX(port)   (INT_LVL_GBE0_RX + (4 * (port)))
#define INT_LVL_GBE_PORT_TX(port)   (INT_LVL_GBE0_TX + (4 * (port))) 
#define INT_LVL_GBE_PORT_MISC(port) (INT_LVL_GBE0_MISC+ (4 * (port))) 
#define INT_LVL_USB_CNT(dev)    	(INT_LVL_USB0 + (dev))


/* e.g. PEX1 int C = 76  |  PEX0 int C = 67 
#define INT_LVL_PEX(pexIf, pin)	((pexIf > 2 )? \
								(INT_LVL_PEX0_INTA -1 + pin + ((pexIf-1)*4)): (INT_LVL_PEX0_INTA -1 + pin + ((pexIf)*4))) 
*/
#define INT_LVL_PEX(pexIf, pin) (INT_LVL_PEX00_INTA -1 + pin + (pexIf*4))
/* interrupt vectors */
#define INT_VEC_ERRSUM  			INUM_TO_IVEC( INT_LVL_ERRSUM   )
#define INT_VEC_SPI   				INUM_TO_IVEC( INT_LVL_SPI     )
#define INT_VEC_TWSI0 				INUM_TO_IVEC( INT_LVL_TWSI0   )
#define INT_VEC_TWSI1 				INUM_TO_IVEC( INT_LVL_TWSI1   )
#define INT_VEC_IDMA0 				INUM_TO_IVEC( INT_LVL_IDMA0   )
#define INT_VEC_IDMA1 				INUM_TO_IVEC( INT_LVL_IDMA1   )
#define INT_VEC_IDMA2 				INUM_TO_IVEC( INT_LVL_IDMA2   )
#define INT_VEC_IDMA3 				INUM_TO_IVEC( INT_LVL_IDMA3   )
#define INT_VEC_TIMER0				INUM_TO_IVEC( INT_LVL_TIMER0  )
#define INT_VEC_TIMER1				INUM_TO_IVEC( INT_LVL_TIMER1  )
#define INT_VEC_TIMER2				INUM_TO_IVEC( INT_LVL_TIMER2  )
#define INT_VEC_TIMER3				INUM_TO_IVEC( INT_LVL_TIMER3  )
#define INT_VEC_UART0 				INUM_TO_IVEC( INT_LVL_UART0   )
#define INT_VEC_UART1 				INUM_TO_IVEC( INT_LVL_UART1   )
#define INT_VEC_UART2 				INUM_TO_IVEC( INT_LVL_UART2   )
#define INT_VEC_UART3 				INUM_TO_IVEC( INT_LVL_UART3   )
#define INT_VEC_USB0  				INUM_TO_IVEC( INT_LVL_USB0    )
#define INT_VEC_USB1  				INUM_TO_IVEC( INT_LVL_USB1    )
#define INT_VEC_USB2  				INUM_TO_IVEC( INT_LVL_USB2    )
#define INT_VEC_CESA				INUM_TO_IVEC( INT_LVL_CESA  )
#define INT_VEC_XOR0  				INUM_TO_IVEC( INT_LVL_XOR0   )
#define INT_VEC_XOR1  				INUM_TO_IVEC( INT_LVL_XOR1   )
#define INT_VEC_XOR0  				INUM_TO_IVEC( INT_LVL_XOR0   )
#define INT_VEC_XOR1  				INUM_TO_IVEC( INT_LVL_XOR1   )
#define INT_VEC_PEX0_ABCD		   	INUM_TO_IVEC( INT_LVL_PEX0_ABCD  )
#define INT_VEC_PEX1_ABCD			INUM_TO_IVEC( INT_LVL_PEX1_ABCD  )
#define INT_VEC_PEX02INTA			INUM_TO_IVEC( INT_LVL_PEX02INTA  )
#define INT_VEC_PEX03INTA			INUM_TO_IVEC( INT_LVL_PEX03INTA  )
#define INT_VEC_PEX10INTA			INUM_TO_IVEC( INT_LVL_PEX10INTA  )
#define INT_VEC_PEX11INTA			INUM_TO_IVEC( INT_LVL_PEX11INTA  )
#define INT_VEC_PEX12INTA			INUM_TO_IVEC( INT_LVL_PEX12INTA  )
#define INT_VEC_PEX13INTA			INUM_TO_IVEC( INT_LVL_PEX13INTA  )
#define INT_VEC_GBE0_SUM 			INUM_TO_IVEC( INT_LVL_GBE0_SUM   )
#define INT_VEC_GBE0_RX  			INUM_TO_IVEC( INT_LVL_GBE0_RX    )
#define INT_VEC_GBE0_TX  			INUM_TO_IVEC( INT_LVL_GBE0_TX    )
#define INT_VEC_GBE0_MISC			INUM_TO_IVEC( INT_LVL_GBE0_MISC  )
#define INT_VEC_GBE1_SUM 			INUM_TO_IVEC( INT_LVL_GBE1_SUM   )
#define INT_VEC_GBE1_RX  			INUM_TO_IVEC( INT_LVL_GBE1_RX    )
#define INT_VEC_GBE1_TX  			INUM_TO_IVEC( INT_LVL_GBE1_TX    )
#define INT_VEC_GBE1_MISC			INUM_TO_IVEC( INT_LVL_GBE1_MISC  )
#define INT_VEC_P0_GPIO0_7  		INUM_TO_IVEC( INT_LVL_P0_GPIO0_7 )
#define INT_VEC_P0_GPIO8_15			INUM_TO_IVEC( INT_LVL_P0_GPIO8_15)
#define INT_VEC_P0_GPIO16_23		INUM_TO_IVEC( INT_LVL_P0_GPIO16_23)
#define INT_VEC_P0_GPIO24_31		INUM_TO_IVEC( INT_LVL_P0_GPIO24_31)
#define INT_VEC_DB_IN    			INUM_TO_IVEC( INT_LVL_DB_IN     )
#define INT_VEC_DB_OUT   			INUM_TO_IVEC( INT_LVL_DB_OUT    )

#define INT_VEC_CRYPT_ERR			INUM_TO_IVEC( INT_LVL_CRYPT_ERR  )
#define INT_VEC_DEV_ERR  			INUM_TO_IVEC( INT_LVL_DEV_ERR    )
#define INT_VEC_DMA_ERR  			INUM_TO_IVEC( INT_LVL_DMA_ERR    )
#define INT_VEC_CPU_ERR  			INUM_TO_IVEC( INT_LVL_CPU_ERR    )
#define INT_VEC_PEX0_ERR 			INUM_TO_IVEC( INT_LVL_PEX0_ERR   )
#define INT_VEC_PEX1_ERR 			INUM_TO_IVEC( INT_LVL_PEX1_ERR   )
#define INT_VEC_GBE_ERR  			INUM_TO_IVEC( INT_LVL_GBE_ERR    )
#define INT_VEC_USB_ERR 			INUM_TO_IVEC( INT_LVL_USB_ERR   )
#define INT_VEC_DRAM_ERR			INUM_TO_IVEC( INT_LVL_DRAM_ERR  )
#define INT_VEC_XOR_ERR 			INUM_TO_IVEC( INT_LVL_XOR_ERR   )
#define INT_VEC_WD_ERR				INUM_TO_IVEC( INT_LVL_WD_ERR )



/* definitions for the UART */

#define RS232_CHAN_A_BASE	(INTER_REGS_BASE + MV_UART_CHAN_BASE(0))
#define RS232_CHAN_B_BASE	(INTER_REGS_BASE + MV_UART_CHAN_BASE(1))
#define RS232_CHAN_C_BASE	(INTER_REGS_BASE + MV_UART_CHAN_BASE(2))
#define RS232_CHAN_D_BASE	(INTER_REGS_BASE + MV_UART_CHAN_BASE(3))

#define RS232_INT_A_LVL		INT_LVL_UART0
#define RS232_INT_B_LVL		INT_LVL_UART1
#define RS232_INT_C_LVL		INT_LVL_UART2
#define RS232_INT_D_LVL		INT_LVL_UART3

#define RS232_INT_A_VEC		INT_VEC_UART0
#define RS232_INT_B_VEC		INT_VEC_UART1
#define RS232_INT_C_VEC		INT_VEC_UART2
#define RS232_INT_D_VEC		INT_VEC_UART3

#define RS232_REG_INTERVAL	4
#define RS232_CLOCK			mvBoardTclkGet()
#define N_SIO_CHANNELS		MV_UART_MAX_CHAN


/* BSP timer constants */

/* definitions for the AMBA Timer */
#define SYS_TIMER_NUM	0	/* System timer uses timer number 0 	*/
#define AUX_TIMER_NUM	1	/* Auxiliry timer uses timer number 1 	*/
#define STMP_TIMER_NUM	2	/* Stamp timer uses timer number 0 	*/

/* Frequency of counter/timers */

#define	SYS_TIMER_CLK		(mvBoardTclkGet()) 
#define AUX_TIMER_CLK		(mvBoardTclkGet()) 
#define	STAMP_TIMER_CLK		(mvBoardTclkGet()) 

#define AMBA_RELOAD_TICKS	0	/* No overhead */

/* Mask out unused bits from timer register. */
#define AMBA_TIMER_VALUE_MASK	0xFFFFFFFF

#define SYS_CLK_RATE_MIN 1
#define SYS_CLK_RATE_MAX 25000

#define AUX_CLK_RATE_MIN 1
#define AUX_CLK_RATE_MAX 25000


/* Board chip-select definition */
#	define NAIN_FLASH_CS    DEVICE_CS1
#	define FLASH_SEC_SIZE	_256K

	
/* PCI definitions */

#define BUS				    BUS_TYPE_PCI
#undef	PCI_IRQ_LINES 
#define PCI_IRQ_LINES	(INT_LVL_PEX00_INTA + 32)
/* Interrupt number for PCI */
#define PCI_MSTR_MEMIO_BUS	PCI_MSTR_MEMIO_LOCAL	/* 1-1 mapping */
#define PCI_MSTR_IO_BUS	    PCI_MSTR_IO_LOCAL	    /* 1-1 mapping */
#define PCI_SLV_MEM_BUS	    PCI_SLV_MEM_LOCAL	    /* 1-1 mapping */


/* PCI access macros */
/* PCI MEMIO memory adrs to CPU (60x bus) adrs */
#define PCI_MEMIO2LOCAL(x) \
     ((int)(x) + PCI_MSTR_MEMIO_LOCAL - PCI_MSTR_MEMIO_BUS)

/* PCI IO adrs to CPU (60x bus) adrs */
#define PCI_IO2LOCAL(x) \
     ((int)(x) + PCI_MSTR_IO_LOCAL - PCI_MSTR_IO_BUS)

/* 60x bus adrs to PCI memory address */
#define LOCAL2PCI_MEMIO(x) \
     ((int)(x) + PCI_SLV_MEM_BUS - PCI_SLV_MEM_LOCAL)
#define PCI2LOCAL_MEMIO(x) \
     ((int)(x) - PCI_SLV_MEM_BUS + PCI_SLV_MEM_LOCAL)

/* Provide intConnect via a macro to pciIntLib.c */
#define PCI_INT_HANDLER_BIND(vector, routine, param, pResult)			\
    {																	\
    IMPORT STATUS sysPciIntConnect();									\
    *pResult = sysPciIntConnect ((vector), (routine), (int)(param));	\
    }

/* defines for generic pciIoMapLib.c code */
#define PCI_IN_BYTE(x)		*(volatile UINT8 *) (x)
#define PCI_OUT_BYTE(x,y)	*(volatile UINT8 *) (x) = (UINT8)  y
#define PCI_IN_WORD(x)		*(volatile UINT16 *)(x)
#define PCI_OUT_WORD(x,y)	*(volatile UINT16 *)(x) = (UINT16) y
#define PCI_IN_LONG(x)		*(volatile UINT32 *)(x)
#define PCI_OUT_LONG(x,y)	*(volatile UINT32 *)(x) = (UINT32) y
     

#ifdef __cplusplus
}
#endif

#endif /* __INCdb78100h */
