/******************************************************************************
 *
 * Name:	skdrv1st.h
 * Project:	GEnesis, PCI Gigabit Ethernet Adapter
 * Version:	$Revision: 1 $
 * Date:	$Date: 12/04/08 3:10p $
 * Purpose:	First header file for driver and all other modules
 *
 ******************************************************************************/

/******************************************************************************
 *
 *      LICENSE:
 *      (C)Copyright Marvell,
 *      All Rights Reserved
 *      
 *      THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MARVELL
 *      The copyright notice above does not evidence any
 *      actual or intended publication of such source code.
 *      
 *      This Module contains Proprietary Information of Marvell
 *      and should be treated as Confidential.
 *      
 *      The information in this file is provided for the exclusive use of
 *      the licensees of Marvell.
 *      Such users have the right to use, modify, and incorporate this code
 *      into products for purposes authorized by the license agreement
 *      provided they include this notice and the associated copyright notice
 *      with any such product.
 *      The information in this file is provided "AS IS" without warranty.
 *      /LICENSE
 *
 ******************************************************************************/

#ifndef __INC_SKDRV1ST_H
#define __INC_SKDRV1ST_H

typedef struct s_AC	SK_AC;
typedef struct s_DrvRlmtMbuf SK_MBUF;

#include "mvOs.h"
#include "h/sktypes.h"
#include "h/skerror.h"
#include "h/skdebug.h"
#include "h/lm80.h"
#include "h/xmac_ii.h"

#ifdef __cplusplus
extern "C" {
#endif

#if ((CPU_FAMILY==I960) && (defined __GNUC__))
#pragma align 1  /* tell gcc960 not to optimize alignments */
#endif    /* CPU_FAMILY==I960 */

/******************************************************************************
*	Most of the driver code is OS independend, This section is the OS wrapper *
*																			  *
******************************************************************************/
/* Set card versions */
#define SK_FAR

#define MV_78100_PLATFORM
#define MV_ADD_EXTREME_SUPREME_SUPPORT

#ifdef MV_CPU_LE
#define SK_LITTLE_ENDIAN
#else
#define SK_BIG_ENDIAN
#endif

#define SK_CHECK_FW_IP

#ifdef SK_ASF
#define MV_INCLUDE_ASF_SUPPORT
#define USE_ASF_DASH_FW
#define SK_DRV_NAME "sgi"
#define SK_USE_DRIVER_INTERFACE
#endif

/* Override some default functions with optimized VxWorks functions */

#define SK_PNMI_STORE_U16(p,v)		memcpy((char*)(p),(char*)&(v),2)
#define SK_PNMI_STORE_U32(p,v)		memcpy((char*)(p),(char*)&(v),4)
#define SK_PNMI_STORE_U64(p,v)		memcpy((char*)(p),(char*)&(v),8)
#define SK_PNMI_READ_U16(p,v)		memcpy((char*)&(v),(char*)(p),2)
#define SK_PNMI_READ_U32(p,v)		memcpy((char*)&(v),(char*)(p),4)
#define SK_PNMI_READ_U64(p,v)		memcpy((char*)&(v),(char*)(p),8)

#define SK_STORE_U32(p,v)	{*(char *)(p) = *((char *)&(v)); \
					*((char *)(p) + 1) = \
						*(((char *)&(v)) + 1); \
					*((char *)(p) + 2) = \
						*(((char *)&(v)) + 2); \
					*((char *)(p) + 3) = \
						*(((char *)&(v)) + 3);}

#define SK_ADDR_EQUAL(a1,a2)		(!memcmp(a1,a2,6))
	
#define SK_CS_CALCULATE_CHECKSUM
#ifndef CONFIG_X86_64
#define SkCsCalculateChecksum(p,l)	
#else
#define SkCsCalculateChecksum(p,l)	
#endif

/* Driver configurations */
#define SK_EXTREME
#define YUKON		/* Support YUKON family */
#define YUK2		/* Support YUKON 2 family */

#define VPD_DO_IO	/* Perform VPD read/write using memory space */
#define SK_NO_RLMT

#ifndef SK_BMU_RX_WM_PEX
#define SK_BMU_RX_WM_PEX 0x80
#endif

/* CAUTION: this should be the value of sysClkRateGet */
#define SK_TICKS_PER_SEC	60

#define	SK_MEM_MAPPED_IO

#define SK_MAX_MACS		2
#define SK_MAX_NETS		2

#define SK_IOC			char*

#define	SK_CONST64	INT64_C
#define	SK_CONSTU64	UINT64_C

#define SK_MEMCPY(dest,src,size)	memcpy(dest,src,size)
#define SK_MEMCMP(s1,s2,size)		memcmp(s1,s2,size)
#define SK_MEMSET(dest,val,size)	memset(dest,val,size)
#define SK_STRLEN(pStr)				strlen((char*)(pStr))
#define SK_STRCMP(pStr1,pStr2)		strcmp((char*)(pStr1),(char*)(pStr2))
#define SK_STRNCMP(s1,s2,len)		strncmp(s1,s2,len)
#define SK_STRCPY(dest,src)			strcpy(dest,src)
#define SK_STRNCPY(pDest,pSrc,size)	strncpy((char*)(pDest),(char*)(pSrc),size)
	 
/* Macros to access the adapter */
#define SK_OUT8(b,a,v)		MV_MEMIO8_WRITE(((unsigned int)(b) + (a)), v)
#define SK_OUT16(b,a,v)		MV_MEMIO_LE16_WRITE(((unsigned int)(b) + (a)), v)
#define SK_OUT32(b,a,v)		MV_MEMIO_LE32_WRITE(((unsigned int)(b) + (a)), v)	
#define SK_IN8(b,a,pv)		(*(unsigned char*)(pv)  = MV_MEMIO8_READ((unsigned int)(b) + (a)))
#define SK_IN16(b,a,pv)		(*(unsigned short*)(pv) = MV_MEMIO_LE16_READ((unsigned int)(b) + (a)))
#define SK_IN32(b,a,pv)		(*(unsigned int*)(pv)   = MV_MEMIO_LE32_READ((unsigned int)(b) + (a)))


#define t_scalar_t	int
#define t_uscalar_t	unsigned int
#define uintptr_t	unsigned long
#define dma_addr_t	unsigned int

#define __CONCAT__(A,B) A##B

#define INT32_C(a)		__CONCAT__(a,L)
#define INT64_C(a)		__CONCAT__(a,LL)
#define UINT32_C(a)		__CONCAT__(a,UL)
#define UINT64_C(a)		__CONCAT__(a,ULL)


#ifdef DEBUG

#define SK_DBG_PRINTF		printf

#define SK_DBG_CHKMOD(pAC) (SK_DEBUG_CHKMOD)
#define SK_DBG_CHKCAT(pAC) (SK_DEBUG_CHKCAT)

/**** possible debug modules for SK_DEBUG_CHKMOD ******************/
#define SK_DBGMOD_MERR  0x00000001L     /* general module error indication 	*/
#define SK_DBGMOD_HWM   0x00000002L     /* Hardware init module 		   	*/
#define SK_DBGMOD_RLMT  0x00000004L     /* RLMT module 						*/
#define SK_DBGMOD_VPD   0x00000008L     /* VPD module 						*/
#define SK_DBGMOD_I2C   0x00000010L     /* I2C module 						*/
#define SK_DBGMOD_PNMI  0x00000020L     /* PNMI module 						*/
#define SK_DBGMOD_CSUM  0x00000040L     /* CSUM module 						*/
#define SK_DBGMOD_ADDR  0x00000080L     /* ADDR module 						*/
#define SK_DBGMOD_DRV   0x00010000L     /* DRV module 						*/

/**** possible debug categories for SK_DEBUG_CHKCAT **************/
#define SK_DBGCAT_INIT  0x00000001L     /* module/driver initialization    	*/
#define SK_DBGCAT_CTRL  0x00000002L     /* controlling: add/rmv MCA/MAC    	*/
#define SK_DBGCAT_ERR   0x00000004L     /* error handling paths            	*/
#define SK_DBGCAT_TX    0x00000008L     /* transmit path                   	*/
#define SK_DBGCAT_RX    0x00000010L     /* receive path                    	*/
#define SK_DBGCAT_IRQ   0x00000020L     /* general IRQ handling           	*/
#define SK_DBGCAT_QUEUE 0x00000040L     /* any queue management            	*/
#define SK_DBGCAT_DUMP  0x00000080L     /* large data output e.g. hex dump 	*/
#define SK_DBGCAT_FATAL 0x00000100L     /* large data output e.g. hex dump 	*/

/**** possible driver debug categories ********************************/
#define SK_DBGCAT_DRV_ENTRY           0x00010000      /* entry points		*/
#define SK_DBGCAT_DRV_SAP			  0x00020000
#define SK_DBGCAT_DRV_MCA             0x00040000      /* multicast			*/
#define SK_DBGCAT_DRV_TX_PROGRESS     0x00080000      /* tx path			*/
#define SK_DBGCAT_DRV_RX_PROGRESS     0x00100000      /* rx path			*/
#define SK_DBGCAT_DRV_PROGRESS        0x00200000      /* general runtime	*/
#define SK_DBGCAT_DRV_MSG			  0x00400000
#define SK_DBGCAT_DRV_PROM            0x00800000      /* promiscuous mode	*/
#define SK_DBGCAT_DRV_TX_FRAME        0x01000000      /* display tx frames	*/
#define SK_DBGCAT_DRV_ERROR           0x02000000      /* error conditions	*/
#define SK_DBGCAT_DRV_INT_SRC         0x04000000      /* interrupts sources */
#define SK_DBGCAT_DRV_EVENT           0x08000000      /* driver events		*/
#define SK_DBGCAT_DRV_BMU_STATUS      0x10000000      /* BMU status process */

#endif

#define SK_ERR_LOG		SkErrorLog

extern	void 	SkErrorLog(SK_AC*, int, int, char*);
extern	SK_U64 	SkOsGetTime(SK_AC *pAC);
extern	SK_MBUF *SkDrvAllocRlmtMbuf(SK_AC *pAC, SK_IOC IoC, unsigned BufferSize);
extern  void  	SkDrvFreeRlmtMbuf(SK_AC *pAC, SK_IOC IoC, SK_MBUF *pMbuf);


#endif

/*******************************************************************************
 *
 * End of file
 *
 ******************************************************************************/
