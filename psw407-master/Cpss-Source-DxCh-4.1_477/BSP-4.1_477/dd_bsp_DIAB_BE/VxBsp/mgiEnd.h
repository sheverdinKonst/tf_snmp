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
* mgiEnd.h - Header File for : Marvell Gigabit Interface driver
*
* DESCRIPTION:
*       This header includes definitions, typedefs and structs needed for the
*		mgi END driver operations.
*
* DEPENDENCIES:
*       WRS endLib library.
*
*******************************************************************************/


#ifndef __mgiEnd_h__
#define __mgiEnd_h__


/* includes */
#include "mvOs.h"

#if ((CPU_FAMILY==I960) && (defined __GNUC__))
#pragma align 1                 /* tell gcc960 not to optimize alignments */
#endif	/* CPU_FAMILY==I960 */

/* defines */

#define DEV_NAME        "mgi"
#define DEV_NAME_LEN    4
#define MGI_1GBS        1000000000	/* bits per sec */
#define MGI_100MBS      100000000	/* bits per sec */
#define MGI_10MBS       10000000    /* bits per sec */


#define N_MCAST		12

/* defines */
#define USE_PRIV_NET_POOL_FUNC
#define INCLUDE_MGI_END_2233_SUPPORT

#if defined(TORNADO_PID) || defined (WORKBENCH) /* Support polling */
#   define INCLUDE_MGI_END_2233_SUPPORT
#   define MGI_STATS_POLLING
#endif /* TORNADO_PID || WORKBENCH */

extern MV_BOOL   mgiStatsControl;

#if ((_WRS_VXWORKS_MAJOR == 6) && (_WRS_VXWORKS_MINOR >= 5))
#   undef INCLUDE_MGI_END_2233_SUPPORT
#endif
#define MGI_MAX_BUF_PER_PKT     30

/* General macros for reading/writing from/to specified locations */
#define MGI_RX_BUF_DEF_SIZE  (mgiMtuSize + 36)

#define MGI_PORT_RX_QUOTA       64   /* Default Port Quota for RX operation */
#define MGI_PORT_RX_MAX_QUOTA   128 /* Maximum allowed Port RX Quota */
#define CL_LOAN_NUM             16

#define MGI_RX_COAL_VALUE       0
#define MGI_TX_COAL_VALUE       0

#define EH_SIZE                 14      /* avoid structure padding issues   */
#define CL_OVERHEAD             4       /* Prepended cluster header         */
#define CL_CACHE_ALIGN          (_CACHE_ALIGN_SIZE - CL_OVERHEAD)
#define CL_BUFF_SIZE            (MGI_RX_BUF_DEF_SIZE + _CACHE_ALIGN_SIZE + CL_CACHE_ALIGN)

#define MV_ETH_RX_READY_ISR_MASK                                    \
            (((1<<MV_ETH_RX_Q_NUM)-1)<<ETH_CAUSE_RX_READY_OFFSET)

#define MV_ETH_TX_DONE_ISR_MASK                                    \
            (((1<<MV_ETH_TX_Q_NUM)-1)<<ETH_CAUSE_TX_BUF_OFFSET)

#define MGI_RX_READY_MASK           MV_ETH_RX_READY_ISR_MASK
#define MGI_TX_DONE_MASK            MV_ETH_TX_DONE_ISR_MASK 
#define MGI_MISC_MASK               ((1<<(ETH_CAUSE_LINK_STATE_CHANGE_BIT)) | \
 								     (1<<(ETH_CAUSE_PHY_STATUS_CHANGE_BIT)))


/* Debug */
#undef MGI_DEBUG

#ifdef MV_RT_DEBUG
#   define MGI_DEBUG
#endif 

#define MGI_DEBUG_FLAG_INIT     (1 << 0)
#define MGI_DEBUG_FLAG_RX       (1 << 1)
#define MGI_DEBUG_FLAG_TX       (1 << 2)
#define MGI_DEBUG_FLAG_ERR      (1 << 3)
#define MGI_DEBUG_FLAG_TRACE    (1 << 4)
#define MGI_DEBUG_FLAG_DUMP     (1 << 5)
#define MGI_DEBUG_FLAG_CACHE    (1 << 6)
#define MGI_DEBUG_FLAG_IOCTL    (1 << 7)
#define MGI_DEBUG_FLAG_STATS    (1 << 8)
#define MGI_DEBUG_FLAG_RX_DONE  (1 << 16)
#define MGI_DEBUG_FLAG_TX_DONE  (1 << 17)
#define MGI_DEBUG_FLAG_LINK     (1 << 18)
#define MGI_DEBUG_FLAG_COAL     (1 << 19)
#define MGI_DEBUG_FLAG_MBLK     (1 << 20)
#define MGI_DEBUG_FLAG_PKT      (1 << 23)
#define MGI_DEBUG_FLAG_CRC      (1 << 24)

#if defined(MGI_DEBUG)
extern MV_U32  mgiDebugFlags;
# define MGI_DEBUG_PRINT(flags, msg)    \
    if( (mgiDebugFlags & (flags)) )     \
        mvOsPrintf msg

# define MGI_DEBUG_CODE(flags, code)    \
    if( (mgiDebugFlags & (flags)) )     \
        code
#else
# define MGI_DEBUG_PRINT(flags, msg) 
# define MGI_DEBUG_CODE(flags, code) 
#endif

#ifdef MGI_DEBUG_TRACE
#   define MGI_TRACE_RX_READY_ISR          0xF0
#   define MGI_TRACE_RX_READY_JOB_ENTER    0xF1
#   define MGI_TRACE_RX_READY_JOB_EXIT     0xF2
#   define MGI_TRACE_TX_ENTER              0xF3
#   define MGI_TRACE_TX_EXIT               0xF4
#   define MGI_TRACE_TX_DONE_ISR           0xF5
#   define MGI_TRACE_TX_DONE_JOB_ENTER     0xF6
#   define MGI_TRACE_TX_DONE_JOB_EXIT      0xF7
#   define MGI_TRACE_RX_DONE_ENTER         0xF8
#   define MGI_TRACE_RX_DONE_EXIT          0xF9
#   define MGI_TRACE_RX_TX_ISR             0xFA

#define MGI_TRACE_ARRAY_SIZE    2048
extern MV_U8   mgiTrace[MGI_TRACE_ARRAY_SIZE];
extern int     mgiTraceIdx = 0;

#   define MGI_TRACE_ADD(type)                          \
            mgiTrace[mgiTraceIdx++] = type;             \
            if(mgiTraceIdx == MGI_TRACE_ARRAY_SIZE)     \
                mgiTraceIdx = 0;
#else
#   define MGI_TRACE_ADD(type)
#endif /* MGI_DEBUG_TRACE */

#define END_FLAGS_ISSET(setBits)                        \
    ((&pDrvCtrl->endObj)->flags & (setBits))

#ifdef INCLUDE_MGI_END_2233_SUPPORT

#ifdef END_MIB_2233
#   define MGI_END_MIB_2233     END_MIB_2233
#else
#   define MGI_END_MIB_2233     0x0
#endif /* END_MIB_2233 */

#   define MGI_HADDR(pEnd)                                 \
        ((pEnd)->pMib2Tbl->m2Data.mibIfTbl.ifPhysAddress.phyAddress)

#   define MGI_HADDR_LEN(pEnd)                             \
        ((pEnd)->pMib2Tbl->m2Data.mibIfTbl.ifPhysAddress.addrLength)

#   define MGI_IN_ERRORS_ADD(pEnd, num)                    \
        (pEnd)->pMib2Tbl->m2CtrUpdateRtn((pEnd)->pMib2Tbl, \
                                         M2_ctrId_ifInErrors, (num))

#   define MGI_OUT_ERRORS_ADD(pEnd, num)                   \
        (pEnd)->pMib2Tbl->m2CtrUpdateRtn((pEnd)->pMib2Tbl, \
                                         M2_ctrId_ifOutErrors, (num))

#   define MGI_OUT_COUNTERS_UPDATE(pEnd, pMblk)            \
        (pEnd)->pMib2Tbl->m2PktCountRtn((pEnd)->pMib2Tbl,  \
                                        M2_PACKET_OUT, (pMblk)->mBlkHdr.mData, \
                                        (pMblk)->mBlkPktHdr.len);

#   define MGI_IN_COUNTERS_UPDATE(pEnd, pMblk)                                 \
        (pMblk)->mBlkHdr.mFlags = M_EXT | M_PKTHDR;                            \
        (pEnd)->pMib2Tbl->m2PktCountRtn((pEnd)->pMib2Tbl,                      \
                                        M2_PACKET_IN, (pMblk)->mBlkHdr.mData,  \
                                        (pMblk)->mBlkPktHdr.len);
#else

#   define MGI_END_MIB_2233     0

#   define MGI_HADDR(pEnd)                              \
        ((pEnd)->mib2Tbl.ifPhysAddress.phyAddress)

#   define MGI_HADDR_LEN(pEnd)                          \
        ((pEnd)->mib2Tbl.ifPhysAddress.addrLength)

#   define MGI_IN_ERRORS_ADD(pEnd, num)                 \
        ((pEnd)->mib2Tbl.ifInErrors += (num))

#   define MGI_OUT_ERRORS_ADD(pEnd, num)                \
        ((pEnd)->mib2Tbl.ifOutErrors += (num))

#   define MGI_OUT_COUNTERS_UPDATE(pEnd, pMblk)                 \
        (pEnd)->mib2Tbl.ifOutOctets += (pMblk)->mBlkPktHdr.len; \
        if( ((*(MV_U8*)((pMblk)->mBlkHdr.mData)) & 1) == 0 )    \
            (pEnd)->mib2Tbl.ifOutUcastPkts++;                   \
        else                                                    \
            (pEnd)->mib2Tbl.ifOutNUcastPkts++;

#   define MGI_IN_COUNTERS_UPDATE(pEnd, pMblk)                      \
        (pEnd)->mib2Tbl.ifInOctets += (pMblk)->mBlkPktHdr.len;      \
        if( ((*(MV_U8*)(pMblk)->mBlkHdr.mData) & 1) == 0 )          \
        {                                                           \
            (pMblk)->mBlkHdr.mFlags = M_EXT | M_PKTHDR;             \
            (pEnd)->mib2Tbl.ifInUcastPkts++;                        \
        }                                                           \
        else                                                        \
        {                                                           \
            (pMblk)->mBlkHdr.mFlags = M_EXT | M_PKTHDR | M_BCAST;   \
            (pEnd)->mib2Tbl.ifInNUcastPkts++;                       \
        }
#endif /* INCLUDE_MGI_END_2233_SUPPORT */

#if defined(MGI_TASK_LOCK)
#   define MGI_LOCK(pDrvCtrl)   mvOsTaskLock()    
#   define MGI_UNLOCK(pDrvCtrl) mvOsTaskUnlock()
#elif defined(MGI_SEMAPHORE)
#   define MGI_LOCK(pDrvCtrl)   semTake((pDrvCtrl)->semId, WAIT_FOREVER);
#   define MGI_UNLOCK(pDrvCtrl) semGive((pDrvCtrl)->semId)
#else
#   define MGI_LOCK(pDrvCtrl)   
#   define MGI_UNLOCK(pDrvCtrl) 
#endif /* MGI_TASK_LOCK - MGI_SEMAPHORE */

typedef struct
{
    MV_U32 stackBP;
    MV_U32 stackResume;
    MV_U32 genMblkError;
    MV_U32 txFailedCount;
    MV_U32 rxFailedCount;
    MV_U32 netJobAddError;

    MV_U32 rxReadyIsrCount;
    MV_U32 rxReadyJobCount;
    MV_U32 rxPktsCount[MV_ETH_RX_Q_NUM];

    MV_U32 rxDoneCount;
    MV_U32 rxDonePktsCount[MV_ETH_RX_Q_NUM];

    MV_U32 txCount;
    MV_U32 txPktsCount[MV_ETH_TX_Q_NUM];
    MV_U32 txMaxPktsCount;
    MV_U32 txMaxBufCount;

    MV_U32 txDoneIsrCount;
    MV_U32 txDoneJobCount;
    MV_U32 txDonePktsCount[MV_ETH_TX_Q_NUM];

    MV_U32 miscIsrCount;
    MV_U32 linkUpCount; 
    MV_U32 linkDownCount; 
    MV_U32 linkMissCount;

} MGI_DEBUG_STATS;

/* The definition of the driver control structure */
typedef struct _drvCtrl
{
    END_OBJ         endObj;     /* Base class               */
    int             unit;       /* Unit number              */
    int             ethPortNo;  /* Ethernet port number     */
    void*           pEthPortHndl;
    MV_BOOL         attached;   /* Interface has been attached  */
    MV_BOOL         started;    /* Interface has been started   */
    MV_BOOL         isUp;       /* Link is UP */
    MV_BOOL         txStall;    /* Tx handler stalled - no CFDs */
    CL_POOL_ID      pClPoolId;  /* Cluster pool identifier      */
    char *          pMblkBase;  /* Mblk Clblk pool pointer  */
    char *          pClsBase;   /* RX Clusters memory base  */
    SEM_ID          semId;      /* Mutual exclusion semaphore */
    MV_U8           macAddr[MV_MAC_ADDR_SIZE];
    int             rxUcastQ;
    int             rxMcastQ;
    int             portRxQuota;
    MGI_DEBUG_STATS debugStats;

#ifdef MGI_DEBUG
    MV_U32*         rxPktsDist;
    MV_U32*         txDonePktsDist;
    int             txDonePktsDistSize;
#endif /* MGI_DEBUG */

    MV_PKT_INFO**   txPktInfo;
    int             txPktInfoIdx;
    int             tx_count;

#ifdef INCLUDE_BRIDGING
    END_OBJ*        pDstEnd;
#endif
#if defined(MGI_STATS_POLLING)
    END_IFDRVCONF   endStatsConf;
    END_IFCOUNTERS  endStatsCounters;
    MV_U32          inPackets;
    MV_U32          inOctets;
#endif /* MGI_STATS_POLLING */
} DRV_CTRL;

void            mgiStatus(void);
void            mgiPortStatus(int port);
void            mgiClearStatus(void);


#endif /* __mgiEnd_h__ */

