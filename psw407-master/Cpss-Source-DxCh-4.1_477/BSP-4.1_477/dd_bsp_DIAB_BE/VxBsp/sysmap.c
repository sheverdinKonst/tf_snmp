/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#include "mvSysHwConfig.h"
#include "sysmap.h"

#if defined (MV78XX0)
MV_CPU_DEC_WIN cpuAddrWinMap[] = {
/*     	base low      base high      size        	WinNum		enable/disable */
	{{SDRAM_CS0_BASE ,    0,      SDRAM_CS0_SIZE }, 0xFFFFFFFF,	EN },/*  0 */
	{{SDRAM_CS1_BASE ,    0,      SDRAM_CS1_SIZE }, 0xFFFFFFFF,	EN },/*  1 */
	{{SDRAM_CS2_BASE ,    0,      SDRAM_CS2_SIZE }, 0xFFFFFFFF,	EN },/*  2 */
	{{SDRAM_CS3_BASE ,    0,      SDRAM_CS3_SIZE }, 0xFFFFFFFF,	EN },/*  3 */
#if defined (MV78100)
	{{DEVICE_CS0_BASE,    0,      DEVICE_CS0_SIZE}, 10,		EN },    /*  4 */
	{{DEVICE_CS1_BASE,    0,      DEVICE_CS1_SIZE}, 11,		EN },    /*  5 */
	{{DEVICE_CS2_BASE,    0,      DEVICE_CS2_SIZE}, 12,		EN },    /*  6 */
	{{DEVICE_CS3_BASE,    0,      DEVICE_CS3_SIZE}, 9,		DIS},    /*  7 */
#endif
#if defined (MV78200)
	{{DEVICE_CS0_BASE,    0,      DEVICE_CS0_SIZE}, 9,		DIS},    /*  4 */
	{{DEVICE_CS1_BASE,    0,      DEVICE_CS1_SIZE}, 10,		EN },    /*  5 */
	{{DEVICE_CS2_BASE,    0,      DEVICE_CS2_SIZE}, 11,		DIS},    /*  6 */
	{{DEVICE_CS3_BASE,    0,      DEVICE_CS3_SIZE}, 12,		EN },    /*  7 */
#endif
	{{BOOTDEV_CS_BASE,    0,      BOOTDEV_CS_SIZE}, 13,		EN },    /*  8 */

#if !defined(MV78XX0_Z0)
    {{DEVICE_SPI_BASE,    0,      DEVICE_SPI_SIZE}, 9,		EN },    /*  9 */ 
#endif


#ifdef  DB_MV78200_A_AMC
#ifdef LION_RD
	{{PCI0_IO_BASE   ,    0,      PCI0_IO_SIZE   }, 0,		DIS},    /* 10 */
	{{PCI0_MEM0_BASE ,    0,      PCI0_MEM0_SIZE }, 1,		EN },    /* 11 */
	{{PCI1_IO_BASE   ,    0,      PCI1_IO_SIZE   }, 2,		DIS},    /* 12 */
	{{PCI1_MEM0_BASE ,    0,      PCI1_MEM0_SIZE }, 3,		EN },    /* 13 */
	{{PCI2_IO_BASE   ,    0,      PCI2_IO_SIZE   }, 4,		DIS},    /* 14 */
	{{PCI2_MEM0_BASE ,    0,      PCI2_MEM0_SIZE }, 6,		EN },    /* 15 */
	{{PCI3_IO_BASE   ,    0,      PCI3_IO_SIZE   }, 4,		DIS},    /* 16 */
	{{PCI3_MEM0_BASE ,    0,      PCI3_MEM0_SIZE }, 5,		EN },    /* 17 */
	{{PCI4_IO_BASE   ,    0,      PCI4_IO_SIZE   }, 6,		DIS},    /* 18 */
	{{PCI4_MEM0_BASE ,    0,      PCI4_MEM0_SIZE }, 7,		DIS},    /* 19 */
	{{PCI5_IO_BASE   ,    0,      PCI5_IO_SIZE   }, 5,		DIS},    /* 20 */
	{{PCI5_MEM0_BASE ,    0,      PCI5_MEM0_SIZE }, 5,		DIS},    /* 21 */
	{{PCI6_IO_BASE   ,    0,      PCI6_IO_SIZE   }, 6,		DIS},    /* 22 */
	{{PCI6_MEM0_BASE ,    0,      PCI6_MEM0_SIZE }, 6,		DIS},    /* 23 */
	{{PCI7_IO_BASE   ,    0,      PCI7_IO_SIZE   }, 7,		DIS},    /* 24 */
	{{PCI7_MEM0_BASE ,    0,      PCI7_MEM0_SIZE }, 7,		DIS},    /* 25 */
#else
	{{PCI0_IO_BASE   ,    0,      PCI0_IO_SIZE   }, 0,		DIS},    /* 10 */
	{{PCI0_MEM0_BASE ,    0,      PCI0_MEM0_SIZE }, 1,		DIS},    /* 11 */
	{{PCI1_IO_BASE   ,    0,      PCI1_IO_SIZE   }, 2,		DIS},    /* 12 */
	{{PCI1_MEM0_BASE ,    0,      PCI1_MEM0_SIZE }, 3,		DIS},    /* 13 */
	{{PCI2_IO_BASE   ,    0,      PCI2_IO_SIZE   }, 4,		DIS},    /* 14 */
	{{PCI2_MEM0_BASE ,    0,      PCI2_MEM0_SIZE }, 5,		DIS},    /* 15 */
	{{PCI3_IO_BASE   ,    0,      PCI3_IO_SIZE   }, 4,		DIS},    /* 16 */
	{{PCI3_MEM0_BASE ,    0,      PCI3_MEM0_SIZE }, 5,		DIS},    /* 17 */
	{{PCI4_IO_BASE   ,    0,      PCI4_IO_SIZE   }, 6,		DIS},    /* 18 */
	{{PCI4_MEM0_BASE ,    0,      PCI4_MEM0_SIZE }, 7,		EN },    /* 19 */
	{{PCI5_IO_BASE   ,    0,      PCI5_IO_SIZE   }, 5,		DIS},    /* 20 */
	{{PCI5_MEM0_BASE ,    0,      PCI5_MEM0_SIZE }, 5,		DIS},    /* 21 */
	{{PCI6_IO_BASE   ,    0,      PCI6_IO_SIZE   }, 6,		DIS},    /* 22 */
	{{PCI6_MEM0_BASE ,    0,      PCI6_MEM0_SIZE }, 6,		DIS},    /* 23 */
	{{PCI7_IO_BASE   ,    0,      PCI7_IO_SIZE   }, 7,		DIS},    /* 24 */
	{{PCI7_MEM0_BASE ,    0,      PCI7_MEM0_SIZE }, 7,		DIS},    /* 25 */
#endif /* LION_RD */

#else
	{{PCI0_IO_BASE   ,    0,      PCI0_IO_SIZE   }, 0,		EN },    /* 10 */
	{{PCI0_MEM0_BASE ,    0,      PCI0_MEM0_SIZE }, 1,		EN },    /* 11 */
	{{PCI1_IO_BASE   ,    0,      PCI1_IO_SIZE   }, 2,		EN },    /* 12 */
	{{PCI1_MEM0_BASE ,    0,      PCI1_MEM0_SIZE }, 3,		EN },    /* 13 */
	{{PCI2_IO_BASE   ,    0,      PCI2_IO_SIZE   }, 4,		DIS},    /* 14 */
	{{PCI2_MEM0_BASE ,    0,      PCI2_MEM0_SIZE }, 5,		DIS},    /* 15 */
	{{PCI3_IO_BASE   ,    0,      PCI3_IO_SIZE   }, 4,		EN },    /* 16 */
	{{PCI3_MEM0_BASE ,    0,      PCI3_MEM0_SIZE }, 5,		EN },    /* 17 */
	{{PCI4_IO_BASE   ,    0,      PCI4_IO_SIZE   }, 6,		EN },    /* 18 */
	{{PCI4_MEM0_BASE ,    0,      PCI4_MEM0_SIZE }, 7,		EN },    /* 19 */
	{{PCI5_IO_BASE   ,    0,      PCI5_IO_SIZE   }, 5,		DIS},    /* 20 */
	{{PCI5_MEM0_BASE ,    0,      PCI5_MEM0_SIZE }, 5,		DIS},    /* 21 */
	{{PCI6_IO_BASE   ,    0,      PCI6_IO_SIZE   }, 6,		DIS},    /* 22 */
	{{PCI6_MEM0_BASE ,    0,      PCI6_MEM0_SIZE }, 6,		DIS},    /* 23 */
	{{PCI7_IO_BASE   ,    0,      PCI7_IO_SIZE   }, 7,		DIS},    /* 24 */
	{{PCI7_MEM0_BASE ,    0,      PCI7_MEM0_SIZE }, 7,		DIS},    /* 25 */
#endif

	{{CRYPTO_BASE ,	      0,      CRYPTO_SIZE    }, 8,		EN },    /* 26 */
	{{INTER_REGS_BASE,    0,      INTER_REGS_SIZE}, 14,		EN },    /* 27 */
    	/* Table terminator */
   	{{TBL_TERM, TBL_TERM, TBL_TERM}, TBL_TERM, TBL_TERM}
};

#elif defined (MV88F6XXX) 

MV_CPU_DEC_WIN SYSMAP_88F6281[] = {
  	 /* base low        base high    size       	WinNum     enable */
	{{SDRAM_CS0_BASE ,    0,      SDRAM_CS0_SIZE } ,0xFFFFFFFF,DIS},
	{{SDRAM_CS1_BASE ,    0,      SDRAM_CS1_SIZE } ,0xFFFFFFFF,DIS},
	{{SDRAM_CS2_BASE ,    0,      SDRAM_CS2_SIZE } ,0xFFFFFFFF,DIS},
	{{SDRAM_CS3_BASE ,    0,      SDRAM_CS3_SIZE } ,0xFFFFFFFF,DIS},
	{{PEX0_MEM_BASE  ,    0,      PEX0_MEM_SIZE  } ,0x1       ,EN},
	{{PEX0_IO_BASE   ,    0,      PEX0_IO_SIZE   } ,0x0       ,EN},
	{{INTER_REGS_BASE,    0,      INTER_REGS_SIZE} ,0x8       ,EN},
	{{NFLASH_CS_BASE,     0,      NFLASH_CS_SIZE}  ,0x2	  ,EN}, 
	{{SPI_CS_BASE,        0,      SPI_CS_SIZE    } ,0x5       ,EN},
	{{DEVICE_CS2_BASE,    0,      DEVICE_CS2_SIZE}, 0x6	  ,DIS},
 	{{BOOTDEV_CS_BASE,    0,      BOOTDEV_CS_SIZE}, 0x4	  ,DIS},
	{{CRYPT_ENG_BASE,     0,      CRYPT_ENG_SIZE}  ,0x7  	  ,EN},
#ifdef MV_INCLUDE_SAGE
	{{SAGE_UNIT_BASE,     0,      SAGE_UNIT_SIZE}  ,0x6  	  ,EN},	
#endif
	{{TBL_TERM,		      0, 	  TBL_TERM		 },TBL_TERM    ,TBL_TERM}		
};
#endif /* MV78XX0 */

MV_CPU_DEC_WIN* mv_sys_map(void)
{
	switch(mvBoardIdGet()) 
	{
#if defined (MV78XX0)
	case DB_78XX0_ID:
	case DB_78200_ID:
  case RD_78XX0_AMC_ID:
		return cpuAddrWinMap;
#elif defined (MV88F6XXX) 

		case DB_88F6281_BP_ID:
		case RD_88F6281_ID: 
			return SYSMAP_88F6281;
#endif
		default:
			mvOsPrintf("ERROR: can't find system address map\n");
			return NULL;
    }
}

