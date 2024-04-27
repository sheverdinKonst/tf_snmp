/******************************************************************************
 *
 * Name:        skdrv2nd.h
 * Project:     GEnesis, PCI Gigabit Ethernet Adapter
 * Version:     $Revision: 1 $
 * Date:        $Date: 12/04/08 3:10p $
 * Purpose:     Second header file for driver and all other modules
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

/******************************************************************************
 *
 * Description:
 *
 * This is the second include file of the driver, which includes all other
 * neccessary files and defines all structures and constants used by the
 * driver and the common modules.
 *
 * Include File Hierarchy:
 *
 *	see skge.c
 *
 ******************************************************************************/

#ifndef __INC_SKDRV2ND_H
#define __INC_SKDRV2ND_H

#include "h/skqueue.h"
#include "h/skgehwt.h"
#include "h/sktimer.h"
#include "h/sktwsi.h"
#include "h/skgepnmi.h"
#include "h/skvpd.h"
#include "h/skgehw.h"
#include "h/sky2le.h"
#include "h/skgeinit.h"
#include "h/skaddr.h"
#include "h/skgesirq.h"
#include "h/skcsum.h"
#include "h/skrlmt.h"
#include "h/skgedrv.h"
#include "h/mvyexhw.h"
#ifdef SK_ASF
#include "h/skgeasf.h"
#ifdef MV_INCLUDE_PNMI
#include "h/skgeasfconv.h"
#endif
#endif

#include "skGeY2End.h"
#include "semLib.h"
#include "netBufLib.h"


#define UNKNOWN -1

/* Override WindRiver definition */
#ifdef LLC_UI
	#undef LLC_UI
#endif                 
/* Type define */

/******************************************************************************
 *
 * Generic driver defines
 *
 ******************************************************************************/
#define SK_REL_SPIN_LOCK(IoC)
#define SK_ACQ_SPIN_LOCK(IoC)


/******************************************************************************
 *
 * ASF related stuff
 *
 ******************************************************************************/
#define SK_INET4_LENGTH		4
#define SK_INET4_OFFSET		2
#define SK_INET6_LENGTH		16
#define SK_INET6_OFFSET		6
#define SK_INET6_COUNT		7

/******************************************************************************
 *
 * VxWorks specific RLMT buffer structure (SK_MBUF typedef in skdrv1st)!
 *
 ******************************************************************************/

struct s_DrvRlmtMbuf {
	SK_MBUF         *pNext;    /* Pointer to next RLMT Mbuf.       */
	SK_U8           *pData;    /* Data buffer (virtually contig.). */
	unsigned         Size;     /* Data buffer size.                */
	unsigned         Length;   /* Length of packet (<= Size).      */
	SK_U32           PortIdx;  /* Receiving/transmitting port.     */
#ifdef SK_RLMT_MBUF_PRIVATE
	SK_RLMT_MBUF     Rlmt;     /* Private part for RLMT.           */
#endif
	M_BLK_ID		 pOs;      /* Pointer to message block         */
};

/******************************************************************************
 *
 * VxWorks specific TIME defines
 *
 ******************************************************************************/

#if (SK_TICKS_PER_SEC == 100)
#define SK_PNMI_HUNDREDS_SEC(t)	(t)
#else
#define SK_PNMI_HUNDREDS_SEC(t) ((((unsigned long)t)*100)/(SK_TICKS_PER_SEC))
#endif

/******************************************************************************
 *
 * Generic sizes and length definitions
 *
 ******************************************************************************/

#define TX_RING_SIZE  (24*1024)  /* Yukon-I */
#define RX_RING_SIZE  (24*1024)  /* Yukon-I */
#define RX_MAX_NBR_BUFFERS   128 /* Yukon-II */
#define TX_MAX_NBR_BUFFERS   256 /* Yukon-II */

#define	ETH_BUF_SIZE        1560  /* multiples of 8 bytes */
#define	ETH_MAX_MTU         1514
#define ETH_MIN_MTU         60
#define ETH_MULTICAST_BIT   0x01
#define SK_JUMBO_MTU        9000

#define TX_PRIO_LOW    0 /* asynchronous queue */
#define TX_PRIO_HIGH   1 /* synchronous queue */
#define DESCR_ALIGN   64 /* alignment of Rx/Tx descriptors */
#define BUFF_ALIGN	  _CACHE_ALIGN_SIZE

/******************************************************************************
 *
 * PNMI related definitions
 *
 ******************************************************************************/

#define SK_DRIVER_RESET(pAC, IoC)	0
#define SK_DRIVER_SENDEVENT(pAC, IoC)	0
#define SK_DRIVER_SELFTEST(pAC, IoC)	0
/* For get mtu you must add an own function */
#define SK_DRIVER_GET_MTU(pAc,IoC,i)	0
#define SK_DRIVER_SET_MTU(pAc,IoC,i,v)	0
#define SK_DRIVER_PRESET_MTU(pAc,IoC,i,v)	0


/******************************************************************************
 *
 * Various offsets and sizes
 *
 ******************************************************************************/

#define	SK_DRV_MODERATION_TIMER         1   /* id */
#define SK_DRV_MODERATION_TIMER_LENGTH  1   /* 1 second */

#define C_LEN_ETHERMAC_HEADER_DEST_ADDR 6
#define C_LEN_ETHERMAC_HEADER_SRC_ADDR  6
#define C_LEN_ETHERMAC_HEADER_LENTYPE   2
#define C_LEN_ETHERMAC_HEADER           ( (C_LEN_ETHERMAC_HEADER_DEST_ADDR) + \
                                          (C_LEN_ETHERMAC_HEADER_SRC_ADDR)  + \
                                          (C_LEN_ETHERMAC_HEADER_LENTYPE) )

#define C_LEN_ETHERMTU_MINSIZE          46
#define C_LEN_ETHERMTU_MAXSIZE_STD      1500
#define C_LEN_ETHERMTU_MAXSIZE_JUMBO    9000

#define C_LEN_ETHERNET_MINSIZE          ( (C_LEN_ETHERMAC_HEADER) + \
                                          (C_LEN_ETHERMTU_MINSIZE) )

#define C_OFFSET_IPHEADER               C_LEN_ETHERMAC_HEADER
#define C_OFFSET_IPHEADER_IPPROTO       9
#define C_OFFSET_TCPHEADER_TCPCS        16
#define C_OFFSET_UDPHEADER_UDPCS        6

#define C_OFFSET_IPPROTO                ( (C_LEN_ETHERMAC_HEADER) + \
                                          (C_OFFSET_IPHEADER_IPPROTO) )

#define C_PROTO_ID_UDP                  17       /* refer to RFC 790 or Stevens'   */
#define C_PROTO_ID_TCP                  6        /* TCP/IP illustrated for details */

/******************************************************************************
 *
 * Tx and Rx descriptor definitions
 *
 ******************************************************************************/

typedef struct s_RxD RXD; /* the receive descriptor */
struct s_RxD {
	volatile SK_U32  RBControl;     /* Receive Buffer Control             */
	SK_U32           VNextRxd;      /* Next receive descriptor,low dword  */
	SK_U32           VDataLow;      /* Receive buffer Addr, low dword     */
	SK_U32           VDataHigh;     /* Receive buffer Addr, high dword    */
	SK_U32           FrameStat;     /* Receive Frame Status word          */
	SK_U32           TimeStamp;     /* Time stamp from XMAC               */
	SK_U32           TcpSums;       /* TCP Sum 2 / TCP Sum 1              */
	SK_U32           TcpSumStarts;  /* TCP Sum Start 2 / TCP Sum Start 1  */
	RXD             *pNextRxd;      /* Pointer to next Rxd                */
	M_BLK_ID		 pMBuf;         /* Pointer to VxWorks M-blk structure */
};

typedef struct s_TxD TXD; /* the transmit descriptor */
struct s_TxD {
	volatile SK_U32  TBControl;     /* Transmit Buffer Control            */
	SK_U32           VNextTxd;      /* Next transmit descriptor,low dword */
	SK_U32           VDataLow;      /* Transmit Buffer Addr, low dword    */
	SK_U32           VDataHigh;     /* Transmit Buffer Addr, high dword   */
	SK_U32           FrameStat;     /* Transmit Frame Status Word         */
	SK_U32           TcpSumOfs;     /* Reserved / TCP Sum Offset          */
	SK_U16           TcpSumSt;      /* TCP Sum Start                      */
	SK_U16           TcpSumWr;      /* TCP Sum Write                      */
	SK_U32           TcpReserved;   /* not used                           */
	TXD             *pNextTxd;      /* Pointer to next Txd                */
	M_BLK_ID		 pMBuf;         /* Pointer to VxWorks M-blk structure */
};

/******************************************************************************
 *
 * Generic Yukon-II defines
 *
 ******************************************************************************/


/* Number of Status LE which will be allocated at init time. */
#define NUMBER_OF_ST_LE 4096L

/* Number of revceive LE which will be allocated at init time. */
#define NUMBER_OF_RX_LE 512

/* Number of transmit LE which will be allocated at init time. */
#define NUMBER_OF_TX_LE 1024L

#define LE_SIZE   sizeof(SK_HWLE)
#define MAX_NUM_FRAGS   30	/* assaf was (MAX_SKB_FRAGS + 1) */
#define MIN_LEN_OF_LE_TAB   128
#define MAX_LEN_OF_LE_TAB   4096
#define MAX_UNUSED_RX_LE_WORKING   8
#ifdef MAX_FRAG_OVERHEAD
#undef MAX_FRAG_OVERHEAD
#define MAX_FRAG_OVERHEAD   4
#endif
/* as we have a maximum of 16 physical fragments,
   maximum 1 ADDR64 per physical fragment
   maximum 4 LEs for VLAN, Csum, LargeSend, Packet */
#define MIN_LE_FREE_REQUIRED   ((16*2) + 4)
#ifdef USE_SYNC_TX_QUEUE
#define TXS_MAX_LE   256
#else /* !USE_SYNC_TX_QUEUE */
#define TXS_MAX_LE   0
#endif

#define ETHER_MAC_HDR_LEN   (6+6+2) /* MAC SRC ADDR, MAC DST ADDR, TYPE */
#define IP_HDR_LEN   20
#define TCP_CSUM_OFFS   0x10
#define UDP_CSUM_OFFS   0x06
#define TXA_MAX_LE   512
#define RX_MAX_LE   256
#define ST_MAX_LE   (SK_MAX_MACS)*((3*RX_MAX_LE)+(TXA_MAX_LE)+(TXS_MAX_LE))

/******************************************************************************
 *
 * Structures specific for Yukon-II
 *
 ******************************************************************************/

typedef struct s_frag SK_FRAG;
struct s_frag{
 	SK_FRAG       *pNext;
 	char          *pVirt;
  	SK_U64         pPhys;
 	unsigned int   FragLen;
};

typedef struct s_packet SK_PACKET;
struct s_packet{
	/* Common infos: */
	SK_PACKET       *pNext;         /* pointer for packet queues          */
	unsigned int     PacketLen;     /* length of packet                   */
	unsigned int     NumFrags;      /* nbr of fragments (for Rx always 1) */
	SK_FRAG         *pFrag;         /* fragment list                      */
	SK_FRAG          FragArray[MAX_NUM_FRAGS]; /* TX fragment array       */
	unsigned int     NextLE;        /* next LE to use for the next packet */

	/* Private infos: */
	M_BLK_ID		pMBuf;         /* Pointer to VxWorks Packet 		  */
};

typedef	struct s_queue SK_PKT_QUEUE;
struct s_queue{
 	SK_PACKET     *pHead;
 	SK_PACKET     *pTail;
	SEM_ID        QueueLock;     /* serialize packet accesses          */
};

/*******************************************************************************
 *
 * Macros specific for Yukon-II queues
 *
 ******************************************************************************/

#define IS_Q_EMPTY(pQueue)  ((pQueue)->pHead != NULL) ? SK_FALSE : SK_TRUE

#define PLAIN_POP_FIRST_PKT_FROM_QUEUE(pQueue, pPacket)	{	\
        if ((pQueue)->pHead != NULL) {				\
		(pPacket)       = (pQueue)->pHead;		\
		(pQueue)->pHead = (pPacket)->pNext;		\
		if ((pQueue)->pHead == NULL) {			\
			(pQueue)->pTail = NULL;			\
		}						\
		(pPacket)->pNext = NULL;			\
	} else {						\
		(pPacket) = NULL;				\
	}							\
}

#define PLAIN_PUSH_PKT_AS_FIRST_IN_QUEUE(pQueue, pPacket) {	\
	if ((pQueue)->pHead != NULL) {				\
		(pPacket)->pNext = (pQueue)->pHead;		\
	} else {						\
		(pPacket)->pNext = NULL;			\
		(pQueue)->pTail  = (pPacket);			\
	}							\
      	(pQueue)->pHead  = (pPacket);				\
}

#define PLAIN_PUSH_PKT_AS_LAST_IN_QUEUE(pQueue, pPacket) {	\
	(pPacket)->pNext = NULL;				\
	if ((pQueue)->pTail != NULL) {				\
		(pQueue)->pTail->pNext = (pPacket);		\
	} else {						\
		(pQueue)->pHead        = (pPacket);		\
	}							\
	(pQueue)->pTail = (pPacket);				\
}

#define PLAIN_PUSH_MULTIPLE_PKT_AS_LAST_IN_QUEUE(pQueue,pPktGrpStart,pPktGrpEnd) { \
	if ((pPktGrpStart) != NULL) {					\
		if ((pQueue)->pTail != NULL) {				\
			(pQueue)->pTail->pNext = (pPktGrpStart);\
		} else {									\
			(pQueue)->pHead = (pPktGrpStart);		\
		}											\
		(pQueue)->pTail = (pPktGrpEnd);				\
	}												\
}

/* Required: 'Flags' */ 
#define POP_FIRST_PKT_FROM_QUEUE(pQueue, pPacket)	{	\
												\
	if ((pQueue)->pHead != NULL) {				\
		(pPacket)       = (pQueue)->pHead;		\
		(pQueue)->pHead = (pPacket)->pNext;		\
		if ((pQueue)->pHead == NULL) {			\
			(pQueue)->pTail = NULL;				\
		}										\
		(pPacket)->pNext = NULL;				\
	} else {									\
		(pPacket) = NULL;						\
	}											\
												\
}

/* Required: 'Flags' */
#define PUSH_PKT_AS_FIRST_IN_QUEUE(pQueue, pPacket)	{	\
												\
	if ((pQueue)->pHead != NULL) {				\
		(pPacket)->pNext = (pQueue)->pHead;		\
	} else {									\
		(pPacket)->pNext = NULL;				\
		(pQueue)->pTail  = (pPacket);			\
	}											\
	(pQueue)->pHead = (pPacket);				\
												\
}

/* Required: 'Flags' */
#define PUSH_PKT_AS_LAST_IN_QUEUE(pQueue, pPacket)	{	\
	(pPacket)->pNext = NULL;					\
												\
	if ((pQueue)->pTail != NULL) {				\
		(pQueue)->pTail->pNext = (pPacket);		\
	} else {									\
		(pQueue)->pHead = (pPacket);			\
	}											\
	(pQueue)->pTail = (pPacket);				\
												\
}

/* Required: 'Flags' */
#define PUSH_MULTIPLE_PKT_AS_LAST_IN_QUEUE(pQueue,pPktGrpStart,pPktGrpEnd) {	\
	if ((pPktGrpStart) != NULL) {					\
													\
		if ((pQueue)->pTail != NULL) {				\
			(pQueue)->pTail->pNext = (pPktGrpStart);\
		} else {									\
			(pQueue)->pHead = (pPktGrpStart);		\
		}											\
		(pQueue)->pTail = (pPktGrpEnd);				\
													\
	}												\
}

/*******************************************************************************
 *
 * Used interrupt bits in the interrupts source register
 *
 ******************************************************************************/

#define DRIVER_IRQS	((IS_IRQ_SW) | \
			 (IS_R1_F)   | (IS_R2_F)  | \
			 (IS_XS1_F)  | (IS_XA1_F) | \
			 (IS_XS2_F)  | (IS_XA2_F))

#define TX_COMPL_IRQS	((IS_XS1_B)  | (IS_XS1_F) | \
			 (IS_XA1_B)  | (IS_XA1_F) | \
			 (IS_XS2_B)  | (IS_XS2_F) | \
			 (IS_XA2_B)  | (IS_XA2_F))

#define NAPI_DRV_IRQS	((IS_R1_F)   | (IS_R2_F) | \
			 (IS_XS1_F)  | (IS_XA1_F)| \
			 (IS_XS2_F)  | (IS_XA2_F))

#define Y2_DRIVER_IRQS	((Y2_IS_STAT_BMU) | (Y2_IS_IRQ_SW) | (Y2_IS_POLL_CHK))

#define SPECIAL_IRQS	((IS_HW_ERR)    |(IS_I2C_READY)  | \
			 (IS_EXT_REG)   |(IS_TIMINT)     | \
			 (IS_PA_TO_RX1) |(IS_PA_TO_RX2)  | \
			 (IS_PA_TO_TX1) |(IS_PA_TO_TX2)  | \
			 (IS_MAC1)      |(IS_LNK_SYNC_M1)| \
			 (IS_MAC2)      |(IS_LNK_SYNC_M2)| \
			 (IS_R1_C)      |(IS_R2_C)       | \
			 (IS_XS1_C)     |(IS_XA1_C)      | \
			 (IS_XS2_C)     |(IS_XA2_C))

#define Y2_SPECIAL_IRQS	((Y2_IS_HW_ERR)   |(Y2_IS_ASF)      | \
			 (Y2_IS_TWSI_RDY) |(Y2_IS_TIMINT)   | \
			 (Y2_IS_IRQ_PHY2) |(Y2_IS_IRQ_MAC2) | \
			 (Y2_IS_CHK_RX2)  |(Y2_IS_CHK_TXS2) | \
			 (Y2_IS_CHK_TXA2) |(Y2_IS_IRQ_PHY1) | \
			 (Y2_IS_IRQ_MAC1) |(Y2_IS_CHK_RX1)  | \
			 (Y2_IS_CHK_TXS1) |(Y2_IS_CHK_TXA1))

#define IRQ_MASK	((IS_IRQ_SW)    | \
			 (IS_R1_F)      |(IS_R2_F)     | \
			 (IS_XS1_F)     |(IS_XA1_F)    | \
			 (IS_XS2_F)     |(IS_XA2_F)    | \
			 (IS_HW_ERR)    |(IS_I2C_READY)| \
			 (IS_EXT_REG)   |(IS_TIMINT)   | \
			 (IS_PA_TO_RX1) |(IS_PA_TO_RX2)| \
			 (IS_PA_TO_TX1) |(IS_PA_TO_TX2)| \
			 (IS_MAC1)      |(IS_MAC2)     | \
			 (IS_R1_C)      |(IS_R2_C)     | \
			 (IS_XS1_C)     |(IS_XA1_C)    | \
			 (IS_XS2_C)     |(IS_XA2_C))

#define Y2_IRQ_MASK	((Y2_DRIVER_IRQS) | (Y2_SPECIAL_IRQS))

#define IRQ_HWE_MASK	(IS_ERR_MSK)		/* enable all HW irqs */
#define Y2_IRQ_HWE_MASK	(Y2_HWE_ALL_MSK)	/* enable all HW irqs */

/*******************************************************************************
 *
 * Rx/Tx Port structures
 *
 ******************************************************************************/

typedef struct s_TxPort {           /* the transmit descriptor rings */
	caddr_t         pTxDescrRing;   /* descriptor area memory        */
	SK_U64          VTxDescrRing;   /* descr. area bus virt. addr.   */
	TXD            *pTxdRingHead;   /* Head of Tx rings              */
	TXD            *pTxdRingTail;   /* Tail of Tx rings              */
	TXD            *pTxdRingPrev;   /* descriptor sent previously    */
	int             TxdRingPrevFree;/* previously # of free entrys   */
	int             TxdRingFree;    /* # of free entrys              */
	SEM_ID       	TxDesRingLock;  /* serialize descriptor accesses */
	caddr_t         HwAddr;         /* bmu registers address         */
	int             PortIndex;      /* index number of port (0 or 1) */
	SK_PACKET      *TransmitPacketTable;
	SK_LE_TABLE     TxALET;         /* tx (async) list element table */
	SK_LE_TABLE     TxSLET;         /* tx (sync) list element table  */
	SK_PKT_QUEUE    TxQ_free;
	SK_PKT_QUEUE    TxAQ_waiting;
	SK_PKT_QUEUE    TxSQ_waiting;
	SK_PKT_QUEUE    TxAQ_working;
	SK_PKT_QUEUE    TxSQ_working;
}TX_PORT;

typedef struct s_RxPort {           /* the receive descriptor rings  */
	caddr_t         pRxDescrRing;   /* descriptor area memory        */
	SK_U64          VRxDescrRing;   /* descr. area bus virt. addr.   */
	RXD            *pRxdRingHead;   /* Head of Rx rings              */
	RXD            *pRxdRingTail;   /* Tail of Rx rings              */
	RXD            *pRxdRingPrev;   /* descr given to BMU previously */
	int             RxdRingFree;    /* # of free entrys              */
	SEM_ID  		RxDesRingLock;  /* serialize descriptor accesses */
	int             RxFillLimit;    /* limit for buffers in ring     */
	caddr_t         HwAddr;         /* bmu registers address         */
	int             PortIndex;      /* index number of port (0 or 1) */
	SK_BOOL         UseRxCsum;      /* use Rx checksumming (yes/no)  */
	SK_PACKET      *ReceivePacketTable;
	SK_LE_TABLE     RxLET;          /* rx list element table         */
	SK_PKT_QUEUE    RxQ_working;
	SK_PKT_QUEUE    RxQ_waiting;
}RX_PORT;

/*******************************************************************************
 *
 * Interrupt masks used in combination with interrupt moderation
 *
 ******************************************************************************/

#define IRQ_EOF_AS_TX     ((IS_XA1_F)     | (IS_XA2_F))
#define IRQ_EOF_SY_TX     ((IS_XS1_F)     | (IS_XS2_F))
#define IRQ_MASK_TX_ONLY  ((IRQ_EOF_AS_TX)| (IRQ_EOF_SY_TX))
#define IRQ_MASK_RX_ONLY  ((IS_R1_F)      | (IS_R2_F))
#define IRQ_MASK_SP_ONLY  (SPECIAL_IRQS)
#define IRQ_MASK_TX_RX    ((IRQ_MASK_TX_ONLY)| (IRQ_MASK_RX_ONLY))
#define IRQ_MASK_SP_RX    ((SPECIAL_IRQS)    | (IRQ_MASK_RX_ONLY))
#define IRQ_MASK_SP_TX    ((SPECIAL_IRQS)    | (IRQ_MASK_TX_ONLY))
#define IRQ_MASK_RX_TX_SP ((SPECIAL_IRQS)    | (IRQ_MASK_TX_RX))

#define IRQ_MASK_Y2_TX_ONLY  (Y2_IS_STAT_BMU)
#define IRQ_MASK_Y2_RX_ONLY  (Y2_IS_STAT_BMU)
#define IRQ_MASK_Y2_SP_ONLY  (SPECIAL_IRQS)
#define IRQ_MASK_Y2_TX_RX    ((IRQ_MASK_TX_ONLY)| (IRQ_MASK_RX_ONLY))
#define IRQ_MASK_Y2_SP_RX    ((SPECIAL_IRQS)    | (IRQ_MASK_RX_ONLY))
#define IRQ_MASK_Y2_SP_TX    ((SPECIAL_IRQS)    | (IRQ_MASK_TX_ONLY))
#define IRQ_MASK_Y2_RX_TX_SP ((SPECIAL_IRQS)    | (IRQ_MASK_TX_RX))

/*******************************************************************************
 *
 * Defines and typedefs regarding interrupt moderation
 *
 ******************************************************************************/

#define C_INT_MOD_NONE			1
#define C_INT_MOD_STATIC		2
#define C_INT_MOD_DYNAMIC		4

#define C_CLK_FREQ_GENESIS		 53215000 /* or:  53.125 MHz */
#define C_CLK_FREQ_YUKON		 78215000 /* or:  78.125 MHz */
#define C_CLK_FREQ_YUKON_EC		125000000 /* or: 125.000 MHz */

#define C_Y2_INTS_PER_SEC_DEFAULT	5000 
#define C_INTS_PER_SEC_DEFAULT		2000 
#define C_INT_MOD_IPS_LOWER_RANGE	30        /* in IRQs/second */
#define C_INT_MOD_IPS_UPPER_RANGE	40000     /* in IRQs/second */

typedef struct s_DynIrqModInfo {
	SK_U64     PrevPort0RxIntrCts;
	SK_U64     PrevPort1RxIntrCts;
	SK_U64     PrevPort0TxIntrCts;
	SK_U64     PrevPort1TxIntrCts;
	SK_U64     PrevPort0StatusLeIntrCts;
	SK_U64     PrevPort1StatusLeIntrCts;
	int        MaxModIntsPerSec;            /* Moderation Threshold   */
	int        MaxModIntsPerSecUpperLimit;  /* Upper limit for DIM    */
	int        MaxModIntsPerSecLowerLimit;  /* Lower limit for DIM    */
	long       MaskIrqModeration;           /* IRQ Mask (eg. 'TxRx')  */
	int        IntModTypeSelect;            /* Type  (eg. 'dynamic')  */
	int        DynIrqModSampleInterval;     /* expressed in seconds!  */
	SK_TIMER   ModTimer;                    /* Timer for dynamic mod. */
} DIM_INFO;

/*******************************************************************************
 *
 * Defines and typedefs regarding wake-on-lan
 *
 ******************************************************************************/

typedef struct s_WakeOnLanInfo {
	SK_U32     SupportedWolOptions;         /* e.g. WAKE_PHY...         */
	SK_U32     ConfiguredWolOptions;        /* e.g. WAKE_PHY...         */
} WOL_INFO;

#define SK_ALLOC_IRQ	0x00000001
#define	DIAG_ACTIVE		1
#define	DIAG_NOTACTIVE		0

/****************************************************************************
 *
 * Per board structure / Adapter Context structure:
 * Contains all 'per device' necessary handles, flags, locks etc.:
 *
 ******************************************************************************/

struct s_AC  {
	SK_GEINIT                GIni;          /* GE init struct             */
#ifdef MV_INCLUDE_PNMI
	SK_PNMI                  Pnmi;          /* PNMI data struct           */
#endif
	SK_VPD                   vpd;           /* vpd data struct            */
	SK_QUEUE                 Event;         /* Event queue                */
	SK_HWT                   Hwt;           /* Hardware Timer ctrl struct */
	SK_TIMCTRL               Tim;           /* Software Timer ctrl struct */
	SK_I2C                   I2c;           /* I2C relevant data structure*/
	SK_ADDR                  Addr;          /* for Address module         */
	SK_CSUM                  Csum;          /* for checksum module        */
	SK_RLMT                  Rlmt;          /* for rlmt module            */
#ifdef SK_ASF
	SK_ASF_DATA              AsfData;
	unsigned char            IpAddr[4];
#ifdef USE_ASF_DASH_FW
	unsigned char            IpV6Addr[16*7];
	unsigned int             ForceFWIPUpdate;
	unsigned int             RecvNewPattern;
	DRIVER_INTERFACE         NewPatternDef; /* => packed, see skgeasf.h! */
	SK_U32                   RamAddr;
	int                      RamSelect;
	unsigned int             ReturningFromSuspend; /* Just resumed? Reset by ASF */
	unsigned int             ReceivedPacket;
	unsigned int             SendWolPattern;
	unsigned char            LinkSpeed;
	unsigned char            LinkSpeedSet;
	unsigned char            SetIFFRunning;
#endif
#endif
	SEM_ID               	 SlowPathLock;  /* Normal IRQ lock            */
#ifdef MV_INCLUDE_PNMI
	SK_PNMI_STRUCT_DATA      PnmiStruct;    /* struct for all Pnmi-Data   */
#endif
	int                      RlmtMode;      /* link check mode to set     */
	int                      RlmtNets;      /* Number of nets             */
	SK_IOC                   IoBase;        /* register set of adapter    */
	int                      BoardLevel;    /* level of hw init (0-2)     */
	char                     DeviceStr[80]; /* adapter string from vpd    */
	PCI_DEV			         pciDev;        /* for access to pci cfg space*/
	END_DEVICE				 *endDev;		/* END device pointer. 			  */
											/* This creates a loop with SK_AC */
	int                      Index;         /* internal board idx number  */
	int                      RxQueueSize;   /* memory used for RX queue   */
	int                      TxSQueueSize;  /* memory used for TXS queue  */
	int                      TxAQueueSize;  /* memory used for TXA queue  */
	int                      PromiscCount;  /* promiscuous mode counter   */
	int                      AllMultiCount; /* allmulticast mode counter  */
	int                      MulticCount;   /* number of MC addresses used*/
	int                      HWRevision;	/* Hardware revision          */
	int                      ActivePort;	/* the active XMAC port       */
	int                      MaxPorts;      /* number of activated ports  */
	int                      TxDescrPerRing;/* # of descriptors TX ring   */
	int                      RxDescrPerRing;/* # of descriptors RX ring   */	
    caddr_t      			 pY2LETMemBase;  /* memory base for Yukon 2 LE tables */
    caddr_t      			 pRxQueueMemBase;/* memory base for Rx packet queue */
    caddr_t      			 pTxQueueMemBase;/* memory base for Tx packet queue */
    int          			 memSize;        /* total memory size for RX/TX area */ /* Assaf, delete??? */
    caddr_t      			 pMclkArea;      /* address of Mclk */                  /* Assaf, delete??? */
	TX_PORT                  TxPort[SK_MAX_MACS][2];
	RX_PORT                  RxPort[SK_MAX_MACS];
	SK_LE_TABLE              TxListElement[SK_MAX_MACS][2];
	SK_LE_TABLE              RxListElement[SK_MAX_MACS];
	SK_LE_TABLE              StatusLETable; 
	unsigned                 SizeOfAlignedLETables;	
	SEM_ID                   SetPutIndexLock;
	int                      MaxUnusedRxLeWorking;
	unsigned int             CsOfs1;        /* for checksum calculation   */
	unsigned int             CsOfs2;        /* for checksum calculation   */
	SK_U32                   CsOfs;         /* for checksum calculation   */
	SK_BOOL                  CheckQueue;    /* check event queue soon     */
	DIM_INFO                 DynIrqModInfo; /* all data related to IntMod */
	int                      PortUp;
	int                      PortDown;
	int                      ChipsetType;   /* 0=GENESIS; 1=Yukon         */
	SK_BOOL                  LowLatency;	/* LowLatency optimization on?*/
#ifdef MV_INCLUDE_PNMI
	SK_PNMI_STRUCT_DATA      PnmiBackup;    /* backup structure for PNMI  */
#endif
	SK_BOOL                  WasIfUp[SK_MAX_MACS];
};

/* PCI configuration read/write */
int SkPciReadCfgByte  (SK_AC *pAC, int PciAddr, SK_U8  *pVal);
int SkPciReadCfgWord  (SK_AC *pAC, int PciAddr, SK_U16 *pVal);
int SkPciReadCfgDWord (SK_AC *pAC, int PciAddr, SK_U32 *pVal);
int SkPciWriteCfgByte (SK_AC *pAC, int PciAddr, SK_U8  Val);
int SkPciWriteCfgWord (SK_AC *pAC, int PciAddr, SK_U16 Val);
int SkPciWriteCfgDWord(SK_AC *pAC, int PciAddr, SK_U32 Val);

#if ((CPU_FAMILY==I960) && (defined __GNUC__))
#pragma align 0                 /* turn off alignment requirement */
#endif  /* CPU_FAMILY==I960 */

#ifdef __cplusplus
}
#endif

#endif

/*******************************************************************************
 *
 * End of file
 *
 ******************************************************************************/
