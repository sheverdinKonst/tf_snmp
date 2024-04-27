/******************************************************************************
 *
 * Name:	skGeY2End.h
 * Project:	GEnesis, PCI Gigabit Ethernet Adapter
 * Version:	$Revision: 1 $
 * Date:	$Date: 12/04/08 3:10p $
 * Purpose:	Header file for driver
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

#ifndef __INCskGeY2Endh
#define __INCskGeY2Endh

/* includes */
#include "stdlib.h"
#include "memLib.h"
#include "cacheLib.h"
#include "end.h"                /* common END structures. */
#include "endLib.h"
#include "netLib.h"
#include "etherLib.h"
#include "miiLib.h"
#include "stdio.h"
#include "config.h"
#include "errnoLib.h"

#ifdef __cplusplus
extern "C" {
#endif

#if ((CPU_FAMILY==I960) && (defined __GNUC__))
#pragma align 1  /* tell gcc960 not to optimize alignments */
#endif    /* CPU_FAMILY==I960 */

#define INCLUDE_SGI_RFC_1213

#ifdef DEBUG 
#define LOGMSG(x,a,b,c,d,e,f) \
   if (endDebug)               \
      {                       \
      logMsg (x,a,b,c,d,e,f); \
      }
#else
#define LOGMSG(x,a,b,c,d,e,f)
#endif /* DEBUG */

/* #define  DRV_DEBUG */
#ifdef  DRV_DEBUG
#define DRV_DEBUG_OFF           0x0000
#define DRV_DEBUG_RX            0x0001
#define DRV_DEBUG_TX            0x0002
#define DRV_DEBUG_INT           0x0004
#define DRV_DEBUG_POLL          (DRV_DEBUG_POLL_RX | DRV_DEBUG_POLL_TX)
#define DRV_DEBUG_POLL_RX       0x0008
#define DRV_DEBUG_POLL_TX       0x0010
#define DRV_DEBUG_LOAD          0x0020
#define DRV_DEBUG_IOCTL         0x0040
#define DRV_DEBUG_TIMER         0x20000
static int sgiDebug = DRV_DEBUG_LOAD | DRV_DEBUG_TX;


#define DRV_LOG(FLG, X0, X1, X2, X3, X4, X5, X6)    \
    if (sgiDebug & FLG)                      \
       logMsg(X0, X1, X2, X3, X4, X5, X6);
	   
#define DRV_PRINT(FLG,X)                            \
    if (sgiDebug & FLG) printf X;

#else /* DRV_DEBUG */
#define DRV_LOG(DBG_SW, X0, X1, X2, X3, X4, X5, X6)
#define DRV_PRINT(DBG_SW,X)
#endif /* DRV_DEBUG */      

#define SGI_END_JUMBO_FRAME_SUPPORT     0x0008 /* Jumbo Frame allowed */
#define SGI_END_RX_IP_XSUM              0x0010 /* RX IP XSUM allowed, not supported */
#define SGI_END_RX_TCPUDP_XSUM          0x0020 /* RX TCP XSUM allowed, not supported */
#define SGI_END_TX_IP_XSUM              0x0040 /* TX IP XSUM allowed, not supported */
#define SGI_END_TX_TCPUDP_XSUM          0x0080 /* TX TCP XSUM allowed, not supported */
#define SGI_END_TX_TCP_SEGMENTATION     0x0100 /* TX TCP segmentation, not supported */
#define SGI_END_USER_MEM_FOR_DESC_ONLY  0x0400 /* cacheable user mem for RX descriptors only */ 

#define END_SPEED_10M                   10000000    /* 10Mbs */
#define END_SPEED_100M                  100000000   /* 100Mbs */
#define END_SPEED_1000M                 1000000000  /* 1000Mbs */
#define END_SPEED                       END_SPEED_1000M

#define BAR0_64_BIT                     0x04

/* general flags */

#define FLAG_POLLING_MODE               0x0001
#define FLAG_PROMISC_MODE               0x0002
#define FLAG_ALLMULTI_MODE              0x0004
#define FLAG_MULTICAST_MODE             0x0008
#define FLAG_BROADCAST_MODE             0x0010

#define ETHER_ADDRESS_SIZE          6
#define ETHER_CRC_LENGTH            4
#define SGI_DEFAULT_ETHERHEADER     (SIZEOF_ETHERHEADER)

#define DEFAULT_DRV_FLAGS               0
#define DEFAULT_LOAN_RXBUF_FACTOR       4
#define DEFAULT_MBLK_NUM_FACTOR         4

#define SYSKONNECT_PCI_VENDOR_ID    0x1148
#define MARVELL_PCI_VENDOR_ID       0x11AB
#define SK9E21D_PCI_DEVICE_ID       0x9e00
#define SK9821_PCI_DEVICE_ID        0x4350

#define SK9821_BOARD				0x1
#define SK9E21D_BOARD				0x2

#define DEVICE_NAME                 "sgi" 
#define DEVICE_NAME_LENGTH           4 

#define SK_LINK_TIMEOUT				5		/* In seconds */

#define UNKNOWN -1
                 

/* bus/CPU address translation macros */

#define SGI_VIRT_TO_BUS(virtAddr)                                           \
        (SGI_PHYS_TO_BUS (((UINT32) SGI_VIRT_TO_PHYS (virtAddr))))

#define SGI_PHYS_TO_VIRT(physAddr)                                          \
        END_CACHE_PHYS_TO_VIRT ((char *)(physAddr))

#define SGI_VIRT_TO_PHYS(virtAddr)                                          \
        END_CACHE_VIRT_TO_PHYS ((char *)(virtAddr))

#define SGI_PHYS_TO_BUS(physAddr)                                          \
        PHYS_TO_BUS_ADDR (pDrvCtrl->unit, (physAddr))

#define SGI_BUS_TO_PHYS(busAddr)                                           \
        BUS_TO_PHYS_ADDR (pDrvCtrl->unit, (busAddr))

/* cache macros */

#define END_USR_CACHE_FLUSH(address, len) \
        CACHE_DRV_FLUSH (&cacheUserFuncs, (address), (len))

#define END_DRV_CACHE_INVALIDATE(address, len) \
        CACHE_DRV_INVALIDATE (pDrvCtrl->pCacheFuncs, (address), (len))

#define END_CACHE_PHYS_TO_VIRT(address) \
        CACHE_DRV_PHYS_TO_VIRT (&cacheDmaFuncs, (address))

#define END_CACHE_VIRT_TO_PHYS(address) \
        CACHE_DRV_VIRT_TO_PHYS (&cacheDmaFuncs, (address))

/* BSP macros */
#ifndef PHYS_TO_BUS_ADDR
#define PHYS_TO_BUS_ADDR(unit,physAddr)                                   \
    ((int) pDrvCtrl->adaptor.sysLocalToBus ?                                  \
    (*pDrvCtrl->adaptor.sysLocalToBus) (unit, physAddr) : physAddr)
#endif /* PHYS_TO_BUS_ADDR */

#ifndef BUS_TO_PHYS_ADDR
#define BUS_TO_PHYS_ADDR(unit,busAddr)                                     \
    ((int) pDrvCtrl->adaptor.sysBusToLocal ?                                  \
    (*pDrvCtrl->adaptor.sysBusToLocal)(unit, busAddr) : busAddr)
#endif /* BUS_TO_PHYS_ADDR */

/*
 * Default macro definitions for BSP interface.
 * These macros can be redefined in a wrapper file, to generate
 * a new module with an optimized interface.
 */
/* macro to connect interrupt handler to vector */

#ifndef SYS_INT_CONNECT
#define SYS_INT_CONNECT(pDrvCtrl,rtn,arg,pResult)          \
    {                                                      \
    *pResult = OK;                                         \
    if (pDrvCtrl->adaptor.intConnect)                                  \
       *pResult = (pDrvCtrl->adaptor.intConnect) ((VOIDFUNCPTR *)      \
          INUM_TO_IVEC (pDrvCtrl->adaptor.vector), \
              rtn, (int)arg);                          \
    }
#endif

/* macro to disconnect interrupt handler from vector */

#ifndef SYS_INT_DISCONNECT
#define SYS_INT_DISCONNECT(pDrvCtrl,rtn,arg,pResult)        			\
    {                                                                 	\
    *pResult = OK;                                                    	\
    if (pDrvCtrl->adaptor.intDisConnect)                               		\
       *pResult =  (pDrvCtrl->adaptor.intDisConnect) ((VOIDFUNCPTR *)  		\
                  INUM_TO_IVEC (pDrvCtrl->adaptor.vector), 				\
               rtn, (int)arg);                         					\
    }
#endif

/* macro to enable the appropriate interrupt level */

#ifndef SYS_INT_ENABLE
#define SYS_INT_ENABLE(pDrvCtrl)                            \
    {                                                       \
    pDrvCtrl->adaptor.intEnable(pDrvCtrl->unit);  \
    }
#endif
#ifndef SYS_INT_DISABLE
#define SYS_INT_DISABLE(pDrvCtrl)                           \
    {                                                       \
    pDrvCtrl->adaptor.intDisable(pDrvCtrl->unit); \
    }
#endif

#define CL_OVERHEAD                     4
#define CL_CACHE_ALIGN_SIZE	        (_CACHE_ALIGN_SIZE - CL_OVERHEAD)
		  /* device structure */

typedef struct _pciDevice
{
    unsigned short	vendorId;	/* adapter vendor ID */
    unsigned short	deviceId;	/* adapter device ID */
	int	pciBusNo;    			/* adapter bus number */
    int	pciDeviceNo; 			/* adapter device number */
    int	pciFuncNo;	  			/* adapter function number */
}PCI_DEV;
	 

typedef struct adapter_info 
    {
    PCI_DEV pciDev;
	int     vector;          /* interrupt vector */
    UINT32  regBaseLow;      /* register PCI base address - low  */
    UINT32  regBaseHigh;     /* register PCI base address - high */
    UINT32  flashBase;       /* flash PCI base address */

    BOOL    adr64;           /* indictor of 64 bit address */
    UINT32  boardType;       /* board type */
    UINT32  phyType;         /* PHY type (MII/GMII) */

    UINT32  delayUnit;       /* delay unit(in ns) for the delay function */
    FUNCPTR delayFunc;      /* BSP specified delay function */        

    UINT32 (*sysLocalToBus)(int,UINT32);
    UINT32 (*sysBusToLocal)(int,UINT32);

    STATUS (*intEnable)(int);    /* board specific interrupt enable routine */
    STATUS (*intDisable)(int);   /* board specific interrupt disable routine */
    STATUS (*intAck) (int);      /* interrupt ack */

    void   (*phySpecInit)(PHY_INFO *, UINT8); /* vendor specified PHY's init */

    FUNCPTR intConnect;     /* interrupt connect function */ 
    FUNCPTR intDisConnect;  /* interrupt disconnect function */

    UINT16  eeprom_icw1;    /* ICW1 in EEPROM */
    UINT16  eeprom_icw2;    /* ICW2 in EEPROM */

    UCHAR   enetAddr[6];    /* Ether address for this adaptor */ 
    UCHAR   reserved1[2];   /* reserved */

    FUNCPTR phyDelayRtn;    /* phy delay function */ 
    UINT32  phyMaxDelay;    /* max phy detection retry */
    UINT32  phyDelayParm;   /* delay parameter for phy delay function */
    UINT32  phyAddr;        /* phy Addr */

/*    DEVDRV_STAT devDrvStat;     */  /* statistic data for devices */
/*    DEV_TIMER   devTimerUpdate; */  /* timer register value for update */

    void   (*phyBspPreInit)(PHY_INFO *, UINT8); /* BSP specific PHY init for 82543-copper */
}ADAPTOR_INFO;


typedef struct end_device
    {
    END_OBJ     end;            /* the class we inherit from. */
    int         unit;           /* unit number */
	int			NetNr;			/* In case of dual port NIC, this is port No */
    UINT32      flags;          /* flags for device configuration */
    UINT32      usrFlags;       /* flags for user customization */ 
    CACHE_FUNCS * pCacheFuncs;  /* cache function pointers */ 
    CL_POOL_ID  pClPoolId;      /* cluster pool ID */
    ADAPTOR_INFO adaptor;       /* adaptor information */
	SK_AC		*pAC;				/* SysKonnect Adapter context */
    UINT32      txReqDescNum;   /* request number for TX descriptor */        /* Assaf, delete??? */
    int         mtu;            /* maximum transfer unit */
    volatile int rxtxHandling;  /* indicator for RX handling */               /* Assaf, move to SK_AC??? */
    volatile BOOL txStall;      /* indicator for transmit stall */            
    UINT32      clSz;           /* cluster size for RX buffer */              /* Assaf, move to SK_AC??? */
    UINT32      offset;         /* offset for IP header */
    UINT32      rxBuffHwOffs;   /* Offset from cluster given to HW RX buffer */
    UINT32      rxBuffSwShift;  /* Number of bytes SW shift Rx buffer */
    BOOL        memAllocFlag;   /* indicator that the shared memory */
                                   /* allocated by driver */
    char *      pDescMemBase;   /* User memory base for driver */
    int         memSize;        /* User memory size for driver */
	char *		pRxBufMem;		/* Rx buffer memory area */
	int         rxBufSize;      /* length of receive buffers  */
	char *		pMclkArea;		/* Mblk and Cblk pool memory area */
    int         txDescNum;      /* num of TX descriptors */
	int         rxDescNum;      /* num of RX descriptors */
    PHY_INFO *  pPhyInfo;       /* pointer to phyInfo structure */
    UINT32      phyInitFlags;   /* Initial PHY flags for phyInfo */
    BOOL        txResoFreeQuick; /* flag to free loaned mBlk quickly */  
    volatile BOOL attach;        /* indicator for drive attach */
    volatile BOOL devStartFlag;  /* indicator for device start */
#ifndef INCLUDE_SGI_RFC_1213
    END_IFDRVCONF endStatsConf;
    END_IFCOUNTERS endStatsCounters;
#endif /* INCLUDE_SGI_RFC_1213 */
#ifdef INCLUDE_BRIDGING
    END_OBJ* pDstEnd;
#endif /* INCLUDE_BRIDGING */
    } END_DEVICE;
	
extern int SkDrvEvent(SK_AC *pAC, SK_IOC IoC, SK_U32 Event, SK_EVPARA Param);

#if ((CPU_FAMILY==I960) && (defined __GNUC__))
#pragma align 0                 /* turn off alignment requirement */
#endif  /* CPU_FAMILY==I960 */

#if defined(__STDC__) || defined(__cplusplus)
IMPORT END_OBJ * skGeEndLoad (char * initString);
#else
IMPORT END_OBJ * skGeEndLoad ();
#endif /* defined(__STDC__) || defined(__cplusplus) */

#ifdef __cplusplus
}
#endif

#endif /* __INCskGeY2Endh */


