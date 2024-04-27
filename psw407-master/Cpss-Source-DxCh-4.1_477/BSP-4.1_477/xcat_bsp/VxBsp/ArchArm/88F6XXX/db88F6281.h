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

#ifndef __INCdb88F66281h
#define __INCdb88F66281h

#ifdef __cplusplus
extern "C" {
#endif

/* includes */
#include "mvBoardEnvSpec.h"
#include "mvCtrlEnvSpec.h"
#include "mvSysHwConfig.h"
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

#ifdef MV_MMU_DISABLE
#undef INCLUDE_MMU_BASIC
#undef INCLUDE_MMU_FULL
#undef INCLUDE_MMU_MPU
#undef INCLUDE_MMU
#endif

#define WRONG_CPU_MSG "\nThis VxWorks image was not compiled for this CPU!\n"

/* interrupt vectors */
#define INT_VEC_HIGHSUM       	INUM_TO_IVEC(INT_LVL_HIGHSUM     )
#define INT_VEC_BRIDGE        	INUM_TO_IVEC(INT_LVL_BRIDGE      )
#define INT_VEC_DB_IN         	INUM_TO_IVEC(INT_LVL_DB_IN       )
#define INT_VEC_DB_OUT        	INUM_TO_IVEC(INT_LVL_DB_OUT      )
#define INT_VEC_x4            	INUM_TO_IVEC(INT_LVL_x4          )
#define INT_VEC_XOR0_CHAN0    	INUM_TO_IVEC(INT_LVL_XOR0_CHAN0  )
#define INT_VEC_XOR0_CHAN1    	INUM_TO_IVEC(INT_LVL_XOR0_CHAN1  )
#define INT_VEC_XOR1_CHAN0    	INUM_TO_IVEC(INT_LVL_XOR1_CHAN0  )
#define INT_VEC_XOR1_CHAN1    	INUM_TO_IVEC(INT_LVL_XOR1_CHAN1  )
#define INT_VEC_PEX00_ABCD    	INUM_TO_IVEC(INT_LVL_PEX00_ABCD  )
#define INT_VEC_x10           	INUM_TO_IVEC(INT_LVL_x10         )
#define INT_VEC_GBE0_SUM      	INUM_TO_IVEC(INT_LVL_GBE0_SUM    )
#define INT_VEC_GBE0_RX       	INUM_TO_IVEC(INT_LVL_GBE0_RX     )
#define INT_VEC_GBE0_TX       	INUM_TO_IVEC(INT_LVL_GBE0_TX     )
#define INT_VEC_GBE0_MISC     	INUM_TO_IVEC(INT_LVL_GBE0_MISC   )
#define INT_VEC_GBE1_SUM      	INUM_TO_IVEC(INT_LVL_GBE1_SUM    )
#define INT_VEC_GBE1_RX       	INUM_TO_IVEC(INT_LVL_GBE1_RX     )
#define INT_VEC_GBE1_TX       	INUM_TO_IVEC(INT_LVL_GBE1_TX     )
#define INT_VEC_GBE1_MISC     	INUM_TO_IVEC(INT_LVL_GBE1_MISC   )
#define INT_VEC_USB0          	INUM_TO_IVEC(INT_LVL_USB0        )
#define INT_VEC_x20           	INUM_TO_IVEC(INT_LVL_x20         )
#define INT_VEC_SATA_CTRL     	INUM_TO_IVEC(INT_LVL_SATA_CTRL   )
#define INT_VEC_SECURITY      	INUM_TO_IVEC(INT_LVL_SECURITY    )
#define INT_VEC_SPI           	INUM_TO_IVEC(INT_LVL_SPI         )
#define INT_VEC_AUDIO           INUM_TO_IVEC(INT_LVL_AUDIO       )
#define INT_VEC_x25           	INUM_TO_IVEC(INT_LVL_x25         )
#define INT_VEC_TS0           	INUM_TO_IVEC(INT_LVL_TS0         )
#define INT_VEC_x27           	INUM_TO_IVEC(INT_LVL_x27         )
#define INT_VEC_DSIO          	INUM_TO_IVEC(INT_LVL_DSIO        )
#define INT_VEC_TWSI          	INUM_TO_IVEC(INT_LVL_TWSI        )
#define INT_VEC_AVB           	INUM_TO_IVEC(INT_LVL_AVB         )
#define INT_VEC_TDM           	INUM_TO_IVEC(INT_LVL_TDM         )
#define INT_VEC_x32           	INUM_TO_IVEC(INT_LVL_x32         )
#define INT_VEC_UART0         	INUM_TO_IVEC(INT_LVL_UART0       )
#define INT_VEC_UART1         	INUM_TO_IVEC(INT_LVL_UART1       )
#define INT_VEC_GPIOLO0_7     	INUM_TO_IVEC(INT_LVL_GPIOLO0_7   )
#define INT_VEC_GPIOLO8_15    	INUM_TO_IVEC(INT_LVL_GPIOLO8_15  )
#define INT_VEC_GPIOLO16_23   	INUM_TO_IVEC(INT_LVL_GPIOLO16_23 )
#define INT_VEC_GPIOLO24_31   	INUM_TO_IVEC(INT_LVL_GPIOLO24_31 )
#define INT_VEC_GPIOHI0_7     	INUM_TO_IVEC(INT_LVL_GPIOHI0_7   )
#define INT_VEC_GPIOHI8_15    	INUM_TO_IVEC(INT_LVL_GPIOHI8_15  )
#define INT_VEC_GPIOHI16_23   	INUM_TO_IVEC(INT_LVL_GPIOHI16_23 )
#define INT_VEC_XOR0_ERR      	INUM_TO_IVEC(INT_LVL_XOR0_ERR    )
#define INT_VEC_XOR1_ERR      	INUM_TO_IVEC(INT_LVL_XOR1_ERR    )
#define INT_VEC_PEX0_ERR      	INUM_TO_IVEC(INT_LVL_PEX0_ERR    )
#define INT_VEC_x45           	INUM_TO_IVEC(INT_LVL_x45         )
#define INT_VEC_GBE0_ERR      	INUM_TO_IVEC(INT_LVL_GBE0_ERR    )
#define INT_VEC_GBE1_ERR      	INUM_TO_IVEC(INT_LVL_GBE1_ERR    )
#define INT_VEC_USB_ERR       	INUM_TO_IVEC(INT_LVL_USB_ERR     )
#define INT_VEC_SECURITY_ERR  	INUM_TO_IVEC(INT_LVL_SECURITY_ERR)
#define INT_VEC_AUDIO_ERR     	INUM_TO_IVEC(INT_LVL_AUDIO_ERR   )
#define INT_VEC_x52           	INUM_TO_IVEC(INT_LVL_x52         )



/* definitions for the UART */

#define RS232_CHAN_A_BASE	(INTER_REGS_BASE + MV_UART_CHAN_BASE(0))
#define RS232_CHAN_B_BASE	(INTER_REGS_BASE + MV_UART_CHAN_BASE(1))
#define RS232_INT_A_LVL		INT_LVL_UART0
#define RS232_INT_B_LVL		INT_LVL_UART1
#define RS232_INT_A_VEC		INT_VEC_UART0
#define RS232_INT_B_VEC		INT_VEC_UART1
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


#define INT_LVL_USB_CNT(dev) (INT_LVL_USB0 + (dev))

/* PCI definitions */

#define BUS				    BUS_TYPE_PCI
#undef	PCI_IRQ_LINES
#define PCI_IRQ_LINES	(INT_LVL_PEX0_INTA + 32)
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

#endif /* __INCdb88F66281h  */

