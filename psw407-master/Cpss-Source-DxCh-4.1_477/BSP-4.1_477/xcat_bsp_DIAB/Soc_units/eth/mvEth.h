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

#ifndef __mvEth_h__
#define __mvEth_h__

#include "mvTypes.h"
#include "mv802_3.h"
#include "mvOs.h"
#include "eth/gbe/mvEthRegs.h"
#include "mvSysHwConfig.h"

/* defines  */

#define MV_ETH_EXTRA_FRAGS_NUM      2

/*
 * MV_ETH_RX_MAPPING (enum)
 *
 * Description:
 *      Enumerates all supported mapping of RX frame to GbE RX queue.
 *
 * Note:
 *      None.
 *
 */
typedef enum _mvEthRxMapping
{
    MV_ETH_RX_DEFAULT_MAP     = 0x0, /* default queue is used */
    MV_ETH_RX_UP_FIELD_MAP    = 0x1, /* RX Queue = UP[2:0] */
    MV_ETH_RX_CPU_CODE_MAP    = 0x2, /* CPU_Code2RxQ map <-- DFSMT table */
    MV_ETH_RX_DEV_PORT_UP_MAP = 0x3  /* RX Queue = */
                                     /* (SrcPort==<SPID> & SrcDev = <SDID>,UP[2:1]) */
} MV_ETH_RX_MAPPING;

/*
 * Typedef: enum MV_ETH_RX_Q
 *
 * Description:
 *      Describes available RX queue for MII interface
 *
 * Fields:
 *
 * Note:
 *      Should be full compatible to BSP_ETH_RX_Q.
 */
typedef enum _mvEthRxQ
{
    MV_ETH_RX_Q_0 = 0,
    MV_ETH_RX_Q_1,
    MV_ETH_RX_Q_2,
    MV_ETH_RX_Q_3,
    MV_ETH_RX_Q_4,
    MV_ETH_RX_Q_5,
    MV_ETH_RX_Q_6,
    MV_ETH_RX_Q_7,
    MV_ETH_RX_Q_TOTAL,
    MV_ETH_RX_Q_ALL = 0xFF
} MV_ETH_RX_Q;

/*
 * Typedef: enum MV_ETH_TX_Q
 *
 * Description:
 *      Describes available TX queue for MII interface
 *
 * Fields:
 *
 * Note:
 *      Should be full compatible to BSP_ETH_TX_Q.
 */
typedef enum _mvEthTxQ
{
    MV_ETH_TX_Q_0 = 0,
    MV_ETH_TX_Q_1,
    MV_ETH_TX_Q_2,
    MV_ETH_TX_Q_3,
    MV_ETH_TX_Q_4,
    MV_ETH_TX_Q_5,
    MV_ETH_TX_Q_6,
    MV_ETH_TX_Q_7,
    MV_ETH_TX_Q_TOTAL,
    MV_ETH_TX_Q_ALL = 0xFF
} MV_ETH_TX_Q;

typedef enum _mvEthPortSpeed
{
    MV_ETH_SPEED_AN,
    MV_ETH_SPEED_10,
    MV_ETH_SPEED_100,
    MV_ETH_SPEED_1000
} MV_ETH_PORT_SPEED;

typedef enum _mvEthPortDuplex
{
    MV_ETH_DUPLEX_AN,
    MV_ETH_DUPLEX_HALF,
    MV_ETH_DUPLEX_FULL
} MV_ETH_PORT_DUPLEX;

typedef enum _mvEthPortFc
{
    MV_ETH_FC_AN_ADV_DIS,
    MV_ETH_FC_AN_ADV_SYM,
    MV_ETH_FC_DISABLE,
    MV_ETH_FC_ENABLE
} MV_ETH_PORT_FC;

typedef enum _mvEthPrioMode
{
    MV_ETH_PRIO_FIXED = 0,  /* Fixed priority mode */
    MV_ETH_PRIO_WRR   = 1   /* Weighted round robin priority mode */
} MV_ETH_PRIO_MODE;

typedef enum _mvEthMru
{
    MV_ETH_MRU_1518 = 1518,
    MV_ETH_MRU_1522 = 1522,
    MV_ETH_MRU_1552 = 1552,
    MV_ETH_MRU_9022 = 9022,
    MV_ETH_MRU_9192 = 9192,
    MV_ETH_MRU_9700 = 9700,
    MV_ETH_MRU_ILLEGAL = 0xFFFFFFFF
} MV_ETH_MRU;

/* Ethernet port specific infomation */
typedef struct _mvEthPortCfg
{
    int          maxRxPktSize;
    MV_ETH_MRU   mru;
    int          rxDefQ;
    int          rxBpduQ;
    int          rxArpQ;
    int          rxTcpQ;
    int          rxUdpQ;
    int          ejpMode;
} MV_ETH_PORT_CFG;

typedef struct _mvEthRxQCfg
{
    int     descrNum;
} MV_ETH_RX_Q_CFG;

typedef struct _mvEthTxQCfg
{
    int                 descrNum;
    MV_ETH_PRIO_MODE    prioMode;
    int                 quota;
} MV_ETH_TX_Q_CFG;

typedef struct _mvEthPortInit
{
    int          maxRxPktSize;
    MV_ETH_MRU   mru;
    int          rxDefQ;
    int          txDescrNum[MV_ETH_TX_Q_NUM];
    int          rxDescrNum[MV_ETH_RX_Q_NUM];
    void        *osHandle;
} MV_ETH_PORT_INIT;

typedef struct _mvEthInit
{
    MV_ETH_PORT_INIT   *ethP;
    MV_U32              rxDescTotal;
    MV_U32              txDescTotal;
    MV_U32              gbeIndex;
} MV_ETH_INIT;

typedef struct _mvEthPortStatus
{
    MV_BOOL             isLinkUp;
    MV_ETH_PORT_SPEED   speed;
    MV_ETH_PORT_DUPLEX  duplex;
    MV_ETH_PORT_FC      flowControl;
} MV_ETH_PORT_STATUS;

typedef enum _mvEthHeaderMode
{
    MV_ETH_DISABLE_HEADER_MODE = 0,
    MV_ETH_ENABLE_HEADER_MODE_PRI_2_1 = 1,
    MV_ETH_ENABLE_HEADER_MODE_PRI_DBNUM = 2,
    MV_ETH_ENABLE_HEADER_MODE_PRI_SPID = 3
} MV_ETH_HEADER_MODE;

/* ethernet.h API list */
void        mvEthHalInit(void);
void        mvEthHalInitPort(int port);
void        mvEthMemAttrGet(MV_BOOL* pIsSram, MV_BOOL* pIsSwCoher);

/* Port Initalization routines */
void*       mvEthPortInit (int port, MV_ETH_PORT_INIT *pPortInit);
void        ethResetTxDescRing(void* pPortHndl, int queue);
void        ethResetRxDescRing(void* pPortHndl, int queue);

void*       mvEthPortHndlGet(int port);

void        mvEthPortFinish(void* pEthPortHndl);
MV_STATUS   mvEthPortDown(void* pEthPortHndl);
MV_STATUS   mvEthPortDisable(void* pEthPortHndl);
MV_STATUS   mvEthPortDisableSimple(int ethPortNum);
MV_STATUS   mvEthPortUp(void* pEthPortHndl);
MV_STATUS   mvEthPortEnable(void* pEthPortHndl);
MV_STATUS   mvEthCpuPortEnable(void* pEthPortHndl);

/* Port data flow routines */
MV_PKT_INFO *mvEthPortForceTxDone(void* pEthPortHndl, int txQueue);
MV_PKT_INFO *mvEthPortForceRx(void* pEthPortHndl, int rxQueue);

/* Port Configuration routines */
MV_STATUS   mvEthPortLinkCfgFromPhy(void* pPortHndl);
MV_STATUS   mvEthDefaultsSet(void* pEthPortHndl);
MV_STATUS   mvEthMaxRxSizeSet(void* pPortHndl, int maxRxSize);

/* Port RX MAC Filtering control routines */
MV_U8       mvEthMcastCrc8Get(MV_U8* pAddr);
MV_STATUS   mvEthRxFilterModeSet(void* pPortHndl, MV_BOOL isPromisc);
MV_STATUS   mvEthMacAddrSet(void* pPortHandle, MV_U8* pMacAddr, int queue);
MV_STATUS   mvEthMcastAddrSet(void* pPortHandle, MV_U8 *pAddr, int queue);

/* MIB Counters APIs */
MV_U32      mvEthMibCounterRead(void* pPortHndl, unsigned int mibOffset,
                               MV_U32* pHigh32);
MV_U32  mvEthMibCounterReadSimple(MV_U32         portNo,
                                  unsigned int   mibOffset,
                                  MV_U32        *pHigh32);
void        mvEthMibCountersClear(void* pPortHandle);

/* TX Scheduling configuration routines */
MV_STATUS   mvEthTxQueueConfig(void* pPortHandle, int txQueue,
                               MV_ETH_PRIO_MODE txPrioMode, int txQuota);

/* RX Dispatching configuration routines */
MV_STATUS   mvEthBpduRxQueue(void* pPortHandle, int bpduQueue);
MV_STATUS   mvEthVlanPrioRxQueue(void* pPortHandle, int vlanPrio, int vlanPrioQueue);
MV_STATUS   mvEthTosToRxqSet(void* pPortHandle, int tos, int rxq);
int         mvEthTosToRxqGet(void* pPortHandle, int tos);

/* Speed, Duplex, FlowControl routines */
MV_STATUS   mvEthSpeedDuplexSet(void* pPortHandle, MV_ETH_PORT_SPEED speed,
                                                   MV_ETH_PORT_DUPLEX duplex);

MV_STATUS   mvEthFlowCtrlSet(void* pPortHandle, MV_ETH_PORT_FC flowControl);

#if (MV_ETH_VERSION >= 4)
MV_STATUS   mvEthEjpModeSet(void* pPortHandle, int mode);
#endif /* (MV_ETH_VERSION >= 4) */

void mvEthStatusGet       (void* pPortHandle, MV_ETH_PORT_STATUS* pStatus);
void mvEthStatusGetFromGbe(void* pPortHandle, MV_ETH_PORT_STATUS* pStatus);
void mvEthStatusGetFromPhy(void* pPortHandle, MV_ETH_PORT_STATUS* pStatus);

/* Marvell Header control               */
MV_STATUS   mvEthHeaderModeSet(void* pPortHandle, MV_ETH_HEADER_MODE headerMode);

/* PHY routines */
void       mvEthPhyAddrSet(void* pPortHandle, int phyAddr);
int        mvEthPhyAddrGet(void* pPortHandle);

/* Power management routines */
void        mvEthPortPowerDown(int port);
void        mvEthPortPowerUp(int port);
void        mvEthCpuPortPowerUp(int port);

MV_STATUS   mvEthActualMruSet(void* pEthPortHndl, MV_ETH_MRU mru);
MV_ETH_MRU  mvEthActualMruGet(MV_U32 ethPortNo);

void        mvEthRandomizeMac(MV_U8 *mac);
void        mvEthMrvlHdrEnable(MV_U32 port);
void        mvEthMrvlHdrDisable(MV_U32 port);
void        mvEthExtDsaEnable(MV_U32 port);
void        mvEthDsaDisable(MV_U32 port);
MV_STATUS   mvEthCpuPortConfig(MV_U32 port);
MV_STATUS   mvEthCfgDFSMTForCpuCodeToRxQ(MV_U32 port, MV_U32 cpuCode, MV_U32 rxQ);
MV_STATUS   mvEthCfgDFSMTForAllCpuCodes(MV_U32 port, MV_U32 rxQ);
void        mvEthClearDFSMT(MV_U32 port, MV_U32 rxQ);
MV_ETH_RX_MAPPING mvEthDsaRxQMapGet(MV_U32 port);
void        mvEthDsaRxQMapSet(MV_U32 port, MV_ETH_RX_MAPPING map);
MV_STATUS   mvEthMacAddrSetSimple(MV_U32 port, unsigned char *pAddr, int queue);

#ifdef MV_ETH_DEBUG_ETH
MV_STATUS mvEthPortTx(void* pEthPortHndl, int txQueue, MV_PKT_INFO* pPktInfo);
MV_STATUS mvEthPortSgTx(void *pEthPortHndl, int txQueue, MV_PKT_INFO *pPktInfo);
MV_PKT_INFO *mvEthPortTxDone(void* pEthPortHndl, int txQueue);
MV_PKT_INFO *mvEthPortRx(void* pEthPortHndl, int rxQueue);
MV_PKT_INFO *mvEthPortSgRx(void *pEthPortHndl, int rxQ);
MV_STATUS mvEthPortRxDone(void* pEthPortHndl, int rxQ, MV_PKT_INFO *pPktInfo);
MV_STATUS mvEthPortSgRxDone(void* pEthPortHndl, int rxQ, MV_PKT_INFO *pPktInfo);
MV_BOOL mvEthIsTxDone(MV_U32 port, MV_U32 txQ);
MV_U32 mvEthRxReadyQGet(MV_U32 port);
MV_U32 mvEthTxDoneQGet(MV_U32 port);
MV_VOID mvEthRxReadyIntAck(MV_U32 port, MV_U32 ackBitMask);
MV_VOID mvEthTxDoneIntAck(MV_U32 port, MV_U32 ackBitMask);
MV_VOID mvEthRxReadyIntUnmask(MV_U32 port, MV_U32 rxQBitMask);
MV_VOID mvEthTxDoneIntUnmask(MV_U32 port, MV_U32 txQBitMask);
MV_VOID mvEthRxReadyIntMask(MV_U32 port, MV_U32 rxQBitMask);
MV_VOID mvEthTxDoneIntMask(MV_U32 port, MV_U32 txQBitMask);
MV_VOID mvEthRxQEnable(MV_U32 port, MV_U32 rxQ);
MV_VOID mvEthTxQEnable(MV_U32 port, MV_U32 txQ);
MV_VOID mvEthRxQDisable(MV_U32 port, MV_U32 rxQ);
MV_VOID mvEthTxQDisable(MV_U32 port, MV_U32 txQ);
#endif
/******************** ETH PRIVATE ************************/

/*#define UNCACHED_TX_BUFFERS*/
/*#define UNCACHED_RX_BUFFERS*/

/* Port attributes */
/* Size of a Tx/Rx descriptor used in chain list data structure */
#define ETH_RX_DESC_ALIGNED_SIZE        32
#define ETH_TX_DESC_ALIGNED_SIZE        32

#define TX_DISABLE_TIMEOUT_MSEC     1000
#define RX_DISABLE_TIMEOUT_MSEC     1000
#define TX_FIFO_EMPTY_TIMEOUT_MSEC  10000
#define PORT_DISABLE_WAIT_TCLOCKS   5000

/* Macros that save access to desc in order to find next desc pointer  */
#define RX_NEXT_DESC_PTR(pRxDescr, pQueueCtrl)                              \
        ((pRxDescr) == (pQueueCtrl)->pLastDescr) ?                          \
               (ETH_RX_DESC*)((pQueueCtrl)->pFirstDescr) :                  \
               (ETH_RX_DESC*)(((MV_ULONG)(pRxDescr)) + ETH_RX_DESC_ALIGNED_SIZE)

#define TX_NEXT_DESC_PTR(pTxDescr, pQueueCtrl)                              \
        ((pTxDescr) == (pQueueCtrl)->pLastDescr) ?                          \
               (ETH_TX_DESC*)((pQueueCtrl)->pFirstDescr) :                  \
               (ETH_TX_DESC*)(((MV_ULONG)(pTxDescr)) + ETH_TX_DESC_ALIGNED_SIZE)

#define RX_PREV_DESC_PTR(pRxDescr, pQueueCtrl)                              \
        ((pRxDescr) == (pQueueCtrl)->pFirstDescr) ?                          \
               (ETH_RX_DESC*)((pQueueCtrl)->pLastDescr) :                  \
               (ETH_RX_DESC*)(((MV_ULONG)(pRxDescr)) - ETH_RX_DESC_ALIGNED_SIZE)

#define TX_PREV_DESC_PTR(pTxDescr, pQueueCtrl)                              \
        ((pTxDescr) == (pQueueCtrl)->pFirstDescr) ?                          \
               (ETH_TX_DESC*)((pQueueCtrl)->pLastDescr) :                  \
               (ETH_TX_DESC*)(((MV_ULONG)(pTxDescr)) - ETH_TX_DESC_ALIGNED_SIZE)


/* Queue specific information */
typedef struct _ethQueueCtrl
{
    void*       pFirstDescr;
    void*       pLastDescr;
    void*       pCurrentDescr;
    void*       pUsedDescr;
    int         resource;
    MV_BUF_INFO descBuf;
} ETH_QUEUE_CTRL;

/* Ethernet port specific infomation */
typedef struct _ethPortCtrl
{
    int             portNo;
    ETH_QUEUE_CTRL  rxQueue[MV_ETH_RX_Q_NUM]; /* Rx ring resource  */
    ETH_QUEUE_CTRL  txQueue[MV_ETH_TX_Q_NUM]; /* Tx ring resource  */

    MV_ETH_PORT_CFG portConfig;
    MV_ETH_RX_Q_CFG rxQueueConfig[MV_ETH_RX_Q_NUM];
    MV_ETH_TX_Q_CFG txQueueConfig[MV_ETH_TX_Q_NUM];

    /* Register images - For DP */
    MV_U32          portTxQueueCmdReg;   /* Port active Tx queues summary    */
    MV_U32          portRxQueueCmdReg;   /* Port active Rx queues summary    */

    MV_STATE        portState;

    MV_U8           mcastCount[256];
    MV_U32*         hashPtr;
    void           *osHandle;
} ETH_PORT_CTRL;

/************** MACROs ****************/

#ifdef ETH_DESCR_IN_HIGH_MEM
/*
 * if y is in marvell virtual himem then translate it to marvell physical himem.
 * else
 * if y happens to be in marvell physical himem then do nothing
 * else
 * call mvOsIoVirtToPhy (x, y)
 */
#define  mvOsIoAddrToPhy(x, y) \
  (bspVirtIsInMvHiMem((MV_U32)(y))) ? bspVirt2Phys((MV_U32)(y)) : \
    (bspPhysIsInMvHiMem((MV_U32)(y))) ? (MV_U32)y : mvOsIoVirtToPhy (x, y)

#endif /* ETH_DESCR_IN_HIGH_MEM */

/* MACROs to Flush / Invalidate TX / RX Buffers */

#ifdef ETH_DESCR_IN_HIGH_MEM
#define HIMEM_ETH_PACKET_CACHE_FLUSH(pAddr, size) \
  mvOsCacheLineFlushInv(NULL, pAddr)

#define HIMEM_ETH_DESCR_FLUSH_INV(pPortCtrl, pDescr) \
  mvOsCacheLineFlushInv(pPortCtrl->osHandle, (MV_ULONG)(pDescr))

#define HIMEM_ETH_DESCR_INV(pPortCtrl, pDescr)            \
        mvOsCacheLineInv(pPortCtrl->osHandle, (MV_ULONG)(pDescr))
#else
#define HIMEM_ETH_PACKET_CACHE_FLUSH(pAddr, size) \
  ETH_PACKET_CACHE_FLUSH(pAddr, size)

#define HIMEM_ETH_DESCR_FLUSH_INV(pPortCtrl, pDescr) \
  ETH_DESCR_FLUSH_INV(pPortCtrl, pDescr)

#define HIMEM_ETH_DESCR_INV(pPortCtrl, pDescr)            \
  ETH_DESCR_INV(pPortCtrl, pDescr)

#endif

#if (ETHER_DRAM_COHER == MV_CACHE_COHER_SW) && !defined(UNCACHED_TX_BUFFERS)
#   define ETH_PACKET_CACHE_FLUSH(pAddr, size)                                  \
            mvOsCacheClear(NULL, (pAddr), (size));
#else
#   define ETH_PACKET_CACHE_FLUSH(pAddr, size)                                  \
        mvOsIoVirtToPhy(NULL, (pAddr));
#endif /* ETHER_DRAM_COHER == MV_CACHE_COHER_SW */

#if ( (ETHER_DRAM_COHER == MV_CACHE_COHER_SW) && !defined(UNCACHED_RX_BUFFERS) )
#   define ETH_PACKET_CACHE_INVALIDATE(pAddr, size)                             \
            mvOsCacheInvalidate (NULL, (pAddr), (size));
#   define ETH_PKT_CACHE_INV(pAddr, size)                             \
            mvOsCacheInvalidate (NULL, (pAddr), (size));
#else
#   define ETH_PACKET_CACHE_INVALIDATE(pAddr, size)
#   define ETH_PKT_CACHE_INV(pAddr, size)
#endif /* ETHER_DRAM_COHER == MV_CACHE_COHER_SW && !UNCACHED_RX_BUFFERS */

#ifdef ETH_DESCR_UNCACHED

#define ETH_DESCR_FLUSH_INV(pPortCtrl, pDescr)
#define ETH_DESCR_INV(pPortCtrl, pDescr)

#else

#define ETH_DESCR_FLUSH_INV(pPortCtrl, pDescr)      \
        mvOsCacheLineFlushInv(pPortCtrl->osHandle, (MV_ULONG)(pDescr))

#define ETH_DESCR_INV(pPortCtrl, pDescr)            \
        mvOsCacheLineInv(pPortCtrl->osHandle, (MV_ULONG)(pDescr))

#endif /* ETH_DESCR_UNCACHED */

#include "eth/gbe/mvEthGbe.h"

#endif /* __mvEth_h__ */

