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

#include "presteraSwitchEnd.h"
#include "mvSysHwConfig.h"
#include "mvGndReg.h"
#include "mvGnd.h"
#include "mvGndOsIf.h"
#include "mvGndHwIf.h"
#include "mvGenSyncPool.h"
#include "mvNetDrvCommon.h"
#include "iv.h"
#include "pssBspApisPriv.h"
#include "mvCommon.h"
#include "bootLib.h"
#include "config.h"
#include "mvBoardEnvLib.h"
#include "mvVxWorksGlue.h"

#if 0
    #define MV_DEBUG
    #undef  MV_DEBUG_DETAILS
#else
    #undef  MV_DEBUG
    #undef  MV_DEBUG_DETAILS
#endif

#ifdef MV_DEBUG
    #define DB(x) x
#else
    #define DB(x)
#endif

#ifdef MV_DEBUG_DETAILS
    #define DB_DETAILS(x) x
#else
    #define DB_DETAILS
#endif

/*
 * Software loopback in the driver may be used for testing.
 */
#undef MV_SW_NET_DRV_LOOPBACK

/*******************************************************************************
 * externals
 */
extern END_TBL_ENTRY    endDevTbl[];

MV_U32  G_switchEndPpIface  = SWITCH_END_PP_IFACE_MII;
MV_BOOL G_switchEndIsTxSync = MV_FALSE;

MV_U32 switchRxQDescNum[MV_ETH_RX_Q_NUM];
extern MV_BOOL standalone_network_device;

/******************************************************************************
 * defines
 */
#define USE_PRIV_NET_POOL_FUNC

/******************************************************************************
 * internal definitions
 */
#define DEV_NAME                        "switch"
#define DEV_NAME_LEN                    7
#define PRESTERA_PORT_NUM               24

#define CL_LOAN_NUM     16
#define CL_OVERHEAD     4       /* Prepended cluster header         */
#define CL_CACHE_ALIGN  (_CACHE_ALIGN_SIZE - CL_OVERHEAD)
#define CL_BUFF_SIZE    (SWITCH_RX_BUF_DEF_SIZE + _CACHE_ALIGN_SIZE + CL_CACHE_ALIGN)

#if defined(SWITCH_TASK_LOCK)
#   define SWITCH_LOCK()   mvOsTaskLock()
#   define SWITCH_UNLOCK() mvOsTaskUnlock()
#elif defined(SWITCH_SEMAPHORE)
#   define SWITCH_LOCK()   semTake((G_switchDrvCtrlP->semId, WAIT_FOREVER);
#   define SWITCH_UNLOCK() semGive((G_switchDrvCtrlP->semId)
#else
#   define SWITCH_LOCK()
#   define SWITCH_UNLOCK()
#endif

typedef struct
{
    CL_POOL_ID       pClPoolId;           /* Cluster pool identifier    */
    char            *pMblkBase;           /* Mblk Clblk pool pointer    */
    char            *pClsBase;            /* RX Clusters memory base    */
    NET_POOL_ID      pNetPool;
    MV_U32           mBlksNum;
#ifdef SWITCH_END_M_BLK_CUSTOM_POOL
    GEN_SYNC_POOL   *rxMblkPoolP;
#endif
} SW_END_POOLS;

typedef struct
{
    MV_U32 rxReadyIsrCnt;
    MV_U32 txDoneIsrCnt;
} SW_DRV_STAT;

typedef struct
{
    END_OBJ          end;                 /* The class we inherit from. */
    SW_END_POOLS     pools;
    SW_DRV_STAT      stat;
} SW_DRV_CTRL;

MV_U32 switchRxQDescNum[MV_ETH_RX_Q_NUM] =
{
    /* rxQ = 0 */ PP_NUM_OF_RX_DESC_PER_Q,

#ifdef INCLUDE_MULTI_QUEUE
    /* rxQ = 1 */ PP_NUM_OF_RX_DESC_PER_Q,
    /* rxQ = 2 */ PP_NUM_OF_RX_DESC_PER_Q,
    /* rxQ = 3 */ PP_NUM_OF_RX_DESC_PER_Q,
    /* rxQ = 4 */ PP_NUM_OF_RX_DESC_PER_Q,
    /* rxQ = 5 */ PP_NUM_OF_RX_DESC_PER_Q,
    /* rxQ = 6 */ PP_NUM_OF_RX_DESC_PER_Q,
    /* rxQ = 7 */ PP_NUM_OF_RX_DESC_PER_Q,
#endif
};

/*******************************************************************************
 * Globals
 */
/* Change this variable to transmit all the traffic to some TX queue */
SW_DRV_CTRL   G_switchDrvCtrl;
SW_DRV_CTRL  *G_switchDrvCtrlP  = &G_switchDrvCtrl;
MV_U32        G_switchEndTxQ    = 0;
MV_BOOL       switchIsFirstTime = MV_TRUE;

static MV_U32      G_maxFragsInPkt = 10;

/*
 * Struct needed to init Generic Network Driver (GND).
 */
static MV_SWITCH_GEN_INIT G_swGenInit;

/*******************************************************************************
 * Declarations
 */
END_OBJ      *switchEndLoad         (char *initString, void* dummy);
LOCAL STATUS  switchEndUnload       (SW_DRV_CTRL* drvCtrlP);
LOCAL STATUS  switchEndIoctl        (SW_DRV_CTRL* drvCtrlP, unsigned int cmd,
                                     caddr_t data);
LOCAL STATUS  switchEndSend         (SW_DRV_CTRL* drvCtrlP,M_BLK *pMblkPkt);
LOCAL STATUS  switchEndStart        (SW_DRV_CTRL* drvCtrlP);
LOCAL STATUS  switchEndStop         (SW_DRV_CTRL* drvCtrlP);
LOCAL STATUS  switchEndMCastAddrAdd (SW_DRV_CTRL* drvCtrlP, char* pAddress);
LOCAL STATUS  switchEndMCastAddrDel (SW_DRV_CTRL* drvCtrlP, char* pAddress);
LOCAL STATUS  switchEndMCastAddrGet (SW_DRV_CTRL* drvCtrlP, MULTI_TABLE* pTable);
LOCAL STATUS  switchEndPollSend     (SW_DRV_CTRL* drvCtrlP, M_BLK_ID pBuf);
LOCAL STATUS  switchEndPollReceive  (SW_DRV_CTRL* drvCtrlP, M_BLK_ID pBuf);
LOCAL void    switchEndCleanup      (SW_DRV_CTRL* drvCtrlP);
LOCAL STATUS  switchEndNetPoolInit  ();

MV_VOID       switchRxReadyIsrCb    ();
MV_VOID       switchTxDoneIsrCb     ();
MV_VOID       switchEndRxPathLock   ();
MV_VOID       switchEndRxPathUnlock ();
MV_VOID       switchEndTxPathLock   ();
MV_VOID       switchEndTxPathUnlock ();
MV_STATUS     switchFwdRxPktToOs    (MV_GND_PKT_INFO *pktInfoP, MV_U32 rxQ);

#ifdef SWITCH_END_M_BLK_CUSTOM_POOL
MV_STATUS     switchEndMblkPoolInit ();
#endif

LOCAL NET_FUNCS switchFuncTable =
{
    (FUNCPTR)switchEndStart,        /* Function to start the device.         */
    (FUNCPTR)switchEndStop,         /* Function to stop the device.          */
    (FUNCPTR)switchEndUnload,       /* Unloading function for the driver.    */
    (FUNCPTR)switchEndIoctl,        /* Ioctl function for the driver.        */
    (FUNCPTR)switchEndSend,         /* Send function for the driver.         */
    (FUNCPTR)switchEndMCastAddrAdd, /* Multicast address add                 */
    (FUNCPTR)switchEndMCastAddrDel, /* Multicast address delete              */
    (FUNCPTR)switchEndMCastAddrGet, /* Multicast table retrieve              */
    (FUNCPTR)switchEndPollSend,     /* Polling send function for the driver. */
    (FUNCPTR)switchEndPollReceive,  /* Polling receive function for the drv. */
    endEtherAddressForm,            /* Put address info into a NET_BUFFER.   */
    endEtherPacketDataGet,          /* Get pointer to data in NET_BUFFER.    */
    endEtherPacketAddrGet           /* Get packet addresses.                 */
};

POOL_FUNC           switchPoolFuncTbl;
extern POOL_FUNC*   _pNetPoolFuncTbl;

/*******************************************************************************
 * switchEndRxPathLock
 */
MV_VOID switchEndRxPathLock()
{
    SWITCH_LOCK();
}

/*******************************************************************************
 * switchEndRxPathUnlock
 */
MV_VOID switchEndRxPathUnlock()
{
    SWITCH_UNLOCK();
}

/*******************************************************************************
 * switchEndTxPathLock
 */
MV_VOID switchEndTxPathLock()
{
    SWITCH_LOCK();
}

/*******************************************************************************
 * switchEndTxPathUnlock
 */
MV_VOID switchEndTxPathUnlock()
{
    SWITCH_UNLOCK();
}

/*******************************************************************************
 * switchDrvOsPktFree
 */
#ifdef MV_SW_NET_DRV_LOOPBACK
MV_STATUS switchDrvOsPktFree(MV_VOID *osPkt)
{
    return MV_OK;
}

#else

/*******************************************************************************
 * switchDrvOsPktFree
 */
MV_STATUS switchDrvOsPktFree(MV_VOID *osPkt)
{
    M_BLK           *pMblk      = (M_BLK *)osPkt;

    DB(mvOsPrintf("%s: ENTERED.\n", __func__));

    /* DB_DETAILS(mvPrintMblk(pMblk)); */

    DB(mvOsPrintf("%s: calling to netMblkClChainFree(pMblk=0x%08X).\n",
       __func__, pMblk));
    SWITCH_LOCK();

    netMblkClChainFree(pMblk);

    SWITCH_UNLOCK();


    return MV_OK;
}
#endif

/*******************************************************************************
 * switchEndRxDone
 */
MV_VOID switchEndRxDone(MV_VOID *dummy, MV_GND_PKT_INFO *pktInfoP, MV_U32 lenAndQ)
{
    MV_U32 rxQ;
    MV_U32 len;

    len      = (lenAndQ >> 16) & 0xFFFF;
    rxQ      = lenAndQ & 0xFF;

    if (switchGenFreeRxPkt(pktInfoP, rxQ) != MV_OK)
    {
        mvOsPrintf("%s: switchGenFreeRxPkt failed.\n", __func__);
    }
}

/*******************************************************************************
 * switchFwdRxPktToOs
 */
#ifdef MV_SW_NET_DRV_LOOPBACK
MV_STATUS switchFwdRxPktToOs(MV_GND_PKT_INFO *rxPktInfoP, MV_U32 rxQ)
{
    SW_DRV_CTRL     *drvCtrlP   = G_switchDrvCtrlP;
    NET_POOL        *pNetPool   = drvCtrlP->pools.pNetPool;
    CL_BLK          *pClBlk     = NULL;
    M_BLK           *pMblk      = NULL;
    MV_GND_PKT_INFO *txPktInfoP;
    MV_U8           *pktP;
    MV_U32           pktSize, i, tmp;

    pktP    = rxPktInfoP->pFrags->bufVirtPtr;
    pktSize = rxPktInfoP->pFrags->dataSize;

    /* exchange MAC addresses */
    for (i = 0; i < MV_MAC_ADDR_SIZE; i++)
    {
        tmp = pktP[i];
        pktP[i] = pktP[i + MV_MAC_ADDR_SIZE];
        pktP[i + MV_MAC_ADDR_SIZE] = tmp;
    }

    /*
     * Alloc pktInfo for TX purposes.
     */
    txPktInfoP = switchGenTxPktInfoGet();
    if (txPktInfoP == NULL)
    {
        DB(mvOsPrintf("%s: switchGenTxPktInfoGet failed.\n", __func__));

        if (switchGenFreeRxPkt(rxPktInfoP, rxQ) != MV_OK)
        {
            DB(mvOsPrintf("%s: switchGenFreeRxPkt failed.\n", __func__));
        }

        return MV_FAIL;
    }

    /*
     * Prepare mBlk to be freed in TX_DONE operation.
     */
    if (pktInfoToPktInfo(rxPktInfoP, txPktInfoP) != MV_OK)
    {
        mvOsPrintf("%s: mBlkToPktInfo failed.\n", __func__);
        return MV_FAIL;
    }
    txPktInfoP->status  = 0;
    txPktInfoP->ownerId = MV_NET_OWN_NET_DRV_RX_REMOVE_TX_ADD_DSA;

    /*
     * Actual send.
     */
    if (switchGenSendPkt(txPktInfoP, 0 /* txQ */) != MV_OK)
    {
        mvOsPrintf("%s: mvGndSendPkt failed.\n", __func__);
        return MV_FAIL;
    }

    if (switchGenFreeRxPkt(rxPktInfoP, rxQ) != MV_OK)
    {
        mvOsPrintf("%s: switchGenFreeRxPkt failed.\n", __func__);
        return MV_FAIL;
    }

    return MV_OK;
}

#else /* MV_SW_NET_DRV_LOOPBACK */

/*******************************************************************************
 * switchFwdRxPktToOs
 */
MV_STATUS switchFwdRxPktToOs(MV_GND_PKT_INFO *pktInfoP, MV_U32 rxQ)
{
    SW_DRV_CTRL     *drvCtrlP   = G_switchDrvCtrlP;
    NET_POOL        *pNetPool   = drvCtrlP->pools.pNetPool;
    CL_BLK          *pClBlk     = NULL;
    M_BLK           *pMblk      = NULL;
    M_BLK           *pFirstMblk = NULL;
    M_BLK           *pPrevMblk  = NULL;
    MV_U32           fragIdx;
    MV_U8           *pktP;
    MV_U32           pktSize;

    SWITCH_LOCK();
    DB(mvOsPrintf("%s: ENTERED.\n", __func__));

    /*
     * Process linked list of pktInfos.
     */
    fragIdx = 0;
    while (pktInfoP != NULL)
    {
        /* DB_DETAILS(mvPrintGndPktInfo(pktInfoP)); */

        pktP    = pktInfoP->pFrags->bufVirtPtr;
        pktSize = pktInfoP->pFrags->dataSize;

        /*
         * Get OS speficic packet wrapper.
         */
    #ifdef SWITCH_END_M_BLK_CUSTOM_POOL

        pMblk = (M_BLK *)genSyncPoolGet(drvCtrlP->pools.rxMblkPoolP);
        if (pMblk == NULL)
        {
            mvOsPrintf("%s: genSyncPoolGet failed.\n", __func__);
            return MV_FAIL;
        }

    #else

        /* Called from sync context, otherwise should be mutually excluded. */
        pMblk = netMblkGet(pNetPool, M_DONTWAIT, MT_DATA);
        if (pMblk == NULL)
        {
            mvOsPrintf("%s: netMblkGet failed.\n", __func__);
            return MV_FAIL;
        }

        pClBlk = netClBlkGet(pNetPool, M_DONTWAIT);
        if (pClBlk == NULL)
        {
            netMblkFree(pNetPool, pMblk);
            mvOsPrintf("%s: netClBlkGet failed.\n", __func__);
            return MV_FAIL;
        }

        if (netMblkClJoin(pMblk, pClBlk) == NULL)
        {
            netMblkFree(pNetPool, pMblk);
            netClBlkFree (pNetPool, pClBlk);
            mvOsPrintf("%s: netMblkClJoin failed.\n", __func__);
            return MV_FAIL;
        }

    #endif /* SWITCH_END_M_BLK_CUSTOM_POOL */

        /*
         * First mBlk will be forwarded to VxWorks.
         */
        if (pFirstMblk == NULL)
        {
            pFirstMblk = pMblk;
        }

        /*
         * Join the cluster to pktP
         */
        pMblk->mBlkPktHdr.len        = pktSize;
        pMblk->mBlkHdr.mLen          = pktSize;
        pMblk->mBlkHdr.mData         = pktP;
        pMblk->mBlkHdr.mNext         = NULL;
        pMblk->mBlkHdr.mNextPkt      = NULL;
        pMblk->mBlkHdr.mFlags       |= M_PKTHDR | M_EXT;

        pMblk->pClBlk->clNode.pClBuf = (void *)pktP;
        pMblk->pClBlk->clRefCnt      = 1;
        pMblk->pClBlk->pClFreeRtn    = (FUNCPTR)switchEndRxDone;
        pMblk->pClBlk->clFreeArg2    = (MV_U32)pktInfoP;
        pMblk->pClBlk->clFreeArg3    = (pMblk->mBlkHdr.mLen << 16);
        pMblk->pClBlk->clFreeArg3   |= rxQ;

        if (pPrevMblk != NULL)
        {
            pPrevMblk->mBlkHdr.mNext = pMblk;
        }
        pPrevMblk = pMblk;

        pktInfoP = pktInfoP->nextP;
        fragIdx++;
    }

    DB(mvPrintMblk(pFirstMblk));
    DB(mvOsPrintf("%s: calling to END_RCV_RTN_CALL.\n\n\n", __func__));
    END_RCV_RTN_CALL(&drvCtrlP->end, pFirstMblk);
    SWITCH_UNLOCK();
    return MV_OK;
}
#endif /* MV_SW_NET_DRV_LOOPBACK */

/*******************************************************************************
* switchEndMblkClFreeRtn - Free or Reuse Mblk+Clblk+Cluster tuple.
*
* DESCRIPTION:
*       This function replace standard pMblkClFreeRtn from netBufLib.
*       1) If the cluster was used for RX,
*           - Invalidate used buffer: Maximum of a. number of bytes used for RX,
*                                                b. number of bytes used for TX.
*           - Return Cluster to ethernet RX queue.
*          Else return cluster to netPool of the driver.
*       2) If Cluster can not be freed (clRefCnt > 1)
*           - return only Mlbk;
*
* INPUT:
*       NET_POOL_ID pNetPool    - pointer to NET_POOL
*       M_BLK_ID    pMblk       - pointer to Mblk+Clblk+Cluster tuple.
*
* RETURN:
*       M_BLK_ID - Pointer to next Mblk if exist.
*
*******************************************************************************/
#ifdef USE_PRIV_NET_POOL_FUNC
M_BLK_ID switchEndMblkClFreeRtn(NET_POOL_ID pNetPool, M_BLK_ID pMblk)
{
#ifdef SWITCH_END_M_BLK_CUSTOM_POOL
    SW_DRV_CTRL     *drvCtrlP    = G_switchDrvCtrlP;
#endif
    M_BLK_ID         pMblkNext = NULL;
    MV_GND_PKT_INFO *pktInfoP  = NULL;
    MV_U32           rxQ;

    SWITCH_LOCK();

    if (pNetPool == NULL)
    {
        mvOsPrintf("%s: ERROR! pNetPool is NULL.\n", __func__);
        return NULL;
    }

    if (pMblk == NULL)
    {
        mvOsPrintf("%s: ERROR! pMblk is NULL.\n", __func__);
        return NULL;
    }
    pMblkNext = pMblk->mBlkHdr.mNext;

    DB(mvOsPrintf("%s: ENTER: pNetPool = 0x%08X, pMblk = 0x%08X\n",
                 __func__, pNetPool, pMblk));

    DB(mvPrintMblk(pMblk));

    /* free the cluster first if it is attached to the mBlk */
    if (M_HASCL(pMblk))
    {
        if (pMblk->pClBlk->clRefCnt == 1)
        {
            if (pMblk->pClBlk->pClFreeRtn == (FUNCPTR)switchEndRxDone)
            {
                if ((pMblk->pClBlk->clFreeArg3 & ~0xFFFF) > 0)
                {
                    /* Clear Mblk structure */
                    pMblk->mBlkHdr.mNext    = NULL;
                    pMblk->mBlkHdr.mNextPkt = NULL;

                    pktInfoP = (MV_GND_PKT_INFO *)pMblk->pClBlk->clFreeArg2;
                    rxQ      = pMblk->pClBlk->clFreeArg3 & 0xFF;

                    DB(mvOsPrintf("%s: Calling to switchGenFreeRxPkt(pktInfoP=0x%08X, rxQ=%d).\n",
                                  __func__, pktInfoP, rxQ));

                    /* DB_DETAILS(mvPrintGndPktInfo(pktInfoP)); */

                    pMblk->pClBlk->clFreeArg3 &= 0xFFFF; /* clearing mBlk struct */
                #ifdef SWITCH_END_M_BLK_CUSTOM_POOL
                    if (genSyncPoolPut(drvCtrlP->pools.rxMblkPoolP, pMblk) != MV_OK)
                    {
                        mvOsPrintf("%s: genSyncPoolPut failed.\n", __func__);
                        return NULL;
                    }
                #else
                    _pNetPoolFuncTbl->pClBlkFreeRtn(pMblk->pClBlk);
                    _pNetPoolFuncTbl->pMblkFreeRtn(pNetPool, pMblk);
                #endif /* SWITCH_END_M_BLK_CUSTOM_POOL */
                    pMblk = NULL;
                }
                else
                {
                    mvOsPrintf("%s: Double free error.\n", __func__);
                }
            }
            else
            {
                DB(mvOsPrintf("%s: ??? 'pClFreeRtn' is NOT 'switchEndRxDone' ???\n",
                              __func__));
                pMblkNext = _pNetPoolFuncTbl->pMblkClFreeRtn(pNetPool, pMblk);
                pMblk = NULL;
            }
        }
        else
        {
#if 1
           _pNetPoolFuncTbl->pClBlkFreeRtn(pMblk->pClBlk);
           _pNetPoolFuncTbl->pMblkFreeRtn(pNetPool, pMblk);
           pMblk = NULL;
#else
           pMblk->pClBlk->clRefCnt--;
#endif
        }
    }

    if (pMblk != NULL)
    {
        mvOsPrintf("%s: clRefCnt = %d\n", __func__, pMblk->pClBlk->clRefCnt);
        /* free the mBlk */
        DB( mvOsPrintf("\nCall pMblkFreeRtn\n"));
        _pNetPoolFuncTbl->pMblkFreeRtn(pNetPool, pMblk);
        DB( mvOsPrintf("\nReturn pMblkFreeRtn\n"));
    }

    DB(mvOsPrintf("%s: EXITING, pNetPool=0x%08X.\n\n\n",
       __func__, (unsigned int)pNetPool));

    SWITCH_UNLOCK();

    return pMblkNext;
}
#endif /* USE_PRIV_NET_POOL_FUNC */

/*******************************************************************************
 * switchEndHookAddTxDone
 */
MV_STATUS switchEndHookAddTxDone(MV_U32 ownerId,
                                 MV_SWITCH_GEN_HOOK_DRV_OS_PKT_FREE hook)
{
    if (switchDrvGenIsInited() == MV_FALSE)
    {
        mvOsPrintf("%s: switchDrvGenIsInited returned false.\n", __func__);
        return MV_FAIL;
    }

    if (switchDrvGenHookSetTxDone(ownerId, hook) != MV_OK)
    {
        mvOsPrintf("%s: switchDrvGenHookSetTxDone failed.\n", __func__);
        return MV_FAIL;
    }

    return MV_OK;
}

/*******************************************************************************
 * switchEndInitSwitchGen
 */
MV_STATUS switchEndInitSwitchGen()
{
    MV_SW_GEN_RX_HOOKS    rxHooks;
    MV_SW_GEN_TX_HOOKS    txHooks;
    MV_U32                hookId;

    if (switchDrvGenIsInited() == MV_TRUE)
    {
        return MV_OK;
    }

    standalone_network_device = MV_TRUE;

    /*
     * Init and configure lower layer (including hardware).
     */
    if (G_switchEndPpIface == SWITCH_END_PP_IFACE_MII)
    {
        G_swGenInit.isMiiMode   = MV_TRUE;
    }
    else
    {
        G_swGenInit.isMiiMode   = MV_FALSE;
    }
    G_swGenInit.gndInitP        = NULL; /* use defaults */
    G_swGenInit.isTxSyncMode    = G_switchEndIsTxSync;
    G_swGenInit.gbeDefaultIndex = mvChipFeaturesGet()->miiGbeIndex;
    if (switchDrvGenInit(&G_swGenInit) != MV_OK)
    {
        mvOsPrintf("%s: switchDrvGenInit failed.\n", __func__);
        return MV_FAIL;
    }

    /*
     * Register RX_READY callbacks.
     */
    if (switchDrvGenHookSetCalcFwdRxPktHookId(
         (MV_SWITCH_GEN_HOOK_CALC_PKT_ID)mvSwitchGetPortNumFromExtDsa) != MV_OK)
    {
        mvOsPrintf("%s: switchDrvGenHookSetCalcFwdRxPktHookId failed.\n", __func__);
        return MV_FAIL;
    }

    if (switchGenHooksInitRx(MV_PP_NUM_OF_PORTS) != MV_OK)
    {
        mvOsPrintf("%s: switchGenHooksInitRx failed.\n", __func__);
        return MV_FAIL;
    }

    rxHooks.fwdRxReadyPktHookF =
            (MV_SWITCH_GEN_HOOK_FWD_RX_PKT)switchFwdRxPktToOs;
    rxHooks.hdrAltBeforeFwdRxPktHookF =
            (MV_SWITCH_GEN_HOOK_HDR_ALT_RX)mvSwitchExtractExtDsa;
    rxHooks.hdrAltBeforeFwdRxFreePktHookF =
            (MV_SWITCH_GEN_HOOK_HDR_ALT_RX_FREE)mvSwitchInjectExtDsa;

    for (hookId = 0; hookId < MV_PP_NUM_OF_PORTS; hookId++)
    {
        if (switchGenHooksFillRx(&rxHooks, hookId) != MV_OK)
        {
            mvOsPrintf("%s: switchGenHooksFillRx failed.\n", __func__);
            return MV_FAIL;
        }
    }

    /*
     * Register TX_DONE callbacks.
     */
    if (switchGenHooksInitTx(MV_NET_OWN_TOTAL_NUM) != MV_OK)
    {
        mvOsPrintf("%s: switchGenHooksInitTx failed.\n", __func__);
        return MV_FAIL;
    }

    txHooks.hdrAltBeforeFwdTxDonePktHookF =
            (MV_SWITCH_GEN_HOOK_HDR_ALT_TX)mvSwitchRemoveExtDsa;
    txHooks.hdrAltBeforeTxF =
            (MV_SWITCH_GEN_HOOK_HDR_ALT_TX)mvSwitchInjectExtDsa;
    txHooks.txDonePktFreeF  =
            (MV_SWITCH_GEN_HOOK_DRV_OS_PKT_FREE)switchDrvOsPktFree;

    if (switchGenHooksFillTx(&txHooks,
                             MV_NET_OWN_NET_DRV_RX_REMOVE_TX_ADD_DSA) != MV_OK)
    {
        mvOsPrintf("%s: switchGenHooksFillTx failed.\n", __func__);
        return MV_FAIL;
    }

    return MV_OK;
}

/*******************************************************************************
* switchEndLoad - Load END device driver to MUX.
*
* DESCRIPTION:
*       This routine initializes both, driver and device to an operational
*       state using device specific parameters specified by <initString>.
*
*       The <initString> includes single parameter <portNum> - interface and
*       dev number.
*
*       The <portMacAddr> is a MAC address to assign the switch dev.
*       At first driver look for its MAC address in the Boot line.
*       If address is not found, driver use default one "switchUserDefMacAddr",
*       when "portNum" added to the last byte of MAC address.
*
*       - Allocate END_OBJ structure.
*       - Get "unit" from "InitString".
*       - Get "macAddr" from BootLine or build default.
*       - Init switch ports network
*       - Init END_OBJ and MIB structures
*       - Mark the device ready
*
* INPUT:
*       char *initString            Parameter string
*
* RETURN:
*       An END object pointer, or NULL on error.
*
*******************************************************************************/
END_OBJ* switchEndLoad(char *initString, void* dummy)
{
    SW_DRV_CTRL     *drvCtrlP = G_switchDrvCtrlP;
    MV_U32           dev, i;

    if (initString == NULL)
    {
        return NULL;
    }

    if (initString[0] == 0)
    {
        bcopy ((char *)DEV_NAME, (void *)initString, DEV_NAME_LEN);
        return NULL;
    }

    if (switchIsFirstTime == MV_TRUE)
    {
        switchIsFirstTime = MV_FALSE;
        /*
         * Init switch driver control structure.
         */
        mvOsMemset(drvCtrlP, 0, sizeof(SW_DRV_CTRL));

        /*
         * Init memory pools for mBlk, clBlk and clusters.
         */
        if (switchEndNetPoolInit() == ERROR)
        {
            mvOsPrintf("%s: switchEndNetPoolInit failed.\n", __func__);
            goto errorExit;
        }

        /*
         * Store mBlk and clBlk in generic pool, thus mBlk and clBlks are
         * never release into VxWorks NET_POOL.
         */
    #ifdef SWITCH_END_M_BLK_CUSTOM_POOL
        if (switchEndMblkPoolInit() == ERROR)
        {
            mvOsPrintf("%s: switchEndMblkPoolInit failed.\n", __func__);
            goto errorExit;
        }
    #endif

        /*
         * Enable following devices to be loaded
         */
        for (i = 0; endDevTbl[i].endLoadFunc != END_TBL_END; i++)
        {
            if (endDevTbl[i].endLoadFunc == switchEndLoad)
            {
                endDevTbl[i].processed = FALSE;
            }
        }

        /*
         * Init generic (OS independent) part of the driver.
         */
        if (switchEndInitSwitchGen() == MV_FAIL)
        {
            mvOsPrintf("%s: switchEndInitSwitchGen failed.\n", __func__);
            return NULL;
        }
    }

    if (END_OBJ_INIT(&drvCtrlP->end,
                     (DEV_OBJ *)NULL,
                     DEV_NAME,
                     0, /* unit */
                     &switchFuncTable,
                     "Prestera dev Network Driver") != OK)
    {
        mvOsPrintf("%s: END_OBJ_INIT failed.\n", __func__);
        return NULL;
    }

    if (END_MIB_INIT(&drvCtrlP->end,
                        M2_ifType_ethernet_csmacd,
                        mvGndMacAddrGet(),
                        MV_MAC_ADDR_SIZE,
                        SWITCH_MTU,
                        MV_NET_1GBS) != OK)
    {
        mvOsPrintf("%s: END_MIB_INIT failed.\n", __func__);
        return NULL;
    }

    END_OBJ_READY (&drvCtrlP->end,
                   IFF_NOTRAILERS | IFF_MULTICAST | IFF_BROADCAST);

    mvOsPrintf("%s Interface Loaded \n", DEV_NAME);

    DB(mvOsPrintf("EXIT EndLoad: pEndObj=0x%x\n",
                  (unsigned int)&G_switchDrvCtrlP->end));
    return &G_switchDrvCtrlP->end;

errorExit:
    mvOsPrintf("%s: ERROR!!! EndLoad: failed.\n", __func__);
    for(dev = 0; dev < mvSwitchGetDevicesNum(); dev++)
    {
        switchEndCleanup(G_switchDrvCtrlP);
    }
    return NULL;
}

/*******************************************************************************
* switchEndUnload - unload a driver from the system
*
* DESCRIPTION:
*       This function unloads the ethernet dev END driver from the MUX.
*
* INPUT:
*       SW_DRV_CTRL *drvCtrlP       pointer to SW_DRV_CTRL structure
*
* OUTPUT:
*       Driver memory allocated is freed.
*       Driver is disconnected from the MUX.
*
* RETURN:
*       OK.
*
*******************************************************************************/
LOCAL STATUS switchEndUnload(SW_DRV_CTRL* drvCtrlP)
{
    if (switchDrvGenUnload() != MV_OK)
    {
        mvOsPrintf("%s: switchDrvGenUnload failed.\n", __func__);
        return ERROR;
    }

    END_OBJECT_UNLOAD(&drvCtrlP->end);
    switchEndCleanup(drvCtrlP);

    DB(mvOsPrintf("%s: completed.\n", __func__));
    return OK;
}

/*******************************************************************************
* switchEndSend - Send a switch packet
*
* DESCRIPTION:
*       This routine takes a M_BLK and sends off the data to the switch ports
*       using the swtitch layer
*
* INPUT:
*       SW_DRV_CTRL *drvCtrlP       pointer to SW_DRV_CTRL structure
*       M_BLK *     pMblk        pointer to the mBlk/cluster pair
*
* OUTPUT:
*       The packet described by pMblk is sent to the Ethernet low level driver
*       for transmission.
*
* RETURN:
*       OK, END_ERR_BLOCK in case there are no Tx descriptors available.
*
*******************************************************************************/

MV_U8 G_endTxBuff[2000] = {0};

LOCAL STATUS switchEndSend(SW_DRV_CTRL *drvCtrlP, M_BLK *pMblkPkt)
{
    MV_GND_PKT_INFO  *pktInfoP;
    MV_U32            txQ, i;
    MV_U32            pktTotalLen;
    MV_U8            *resPktP;
    MV_U8            *pktStartP;
    STATUS            retVal;

    DB(mvOsPrintf("%s: ENTERED.\n", __func__));

    txQ    = G_switchEndTxQ;
    retVal = OK;

    SWITCH_LOCK();

    if (switchGenIsLinkUp() == MV_FALSE)
    {
        netMblkClChainFree(pMblkPkt);
        mvOsPrintf("%s: Driver is not UP.\n", __func__);
        goto tx_out;
    }

    /*
     * Process all concatinated packets.
     */
    while ((pMblkPkt != NULL) &&
           (pMblkPkt->mBlkHdr.mLen > 0) &&
           (pMblkPkt->mBlkHdr.mData != NULL))
    {
        /*
         * Alloc pktInfo for TX purposes.
         */
        pktInfoP = switchGenTxPktInfoGet();
        if (pktInfoP == NULL)
        {
            mvOsPrintf("%s: switchGenTxPktInfoGet failed.\n", __func__);
            retVal = ERROR;
            goto tx_out;
        }

        /*
         * Prepare mBlk to be freed in TX_DONE operation.
         */
        if (mBlkToPktInfo(pMblkPkt,
                          (MV_PKT_INFO *)pktInfoP,
                          G_maxFragsInPkt) != MV_OK)
        {
            mvOsPrintf("%s: mBlkToPktInfo failed.\n", __func__);
            retVal = ERROR;
            goto tx_out;
        }
        pktInfoP->status  = 0;
        pktInfoP->ownerId = MV_NET_OWN_NET_DRV_RX_REMOVE_TX_ADD_DSA;

        DB(mvPrintMblk(pMblkPkt));

        pktTotalLen = 0;
        pktStartP   = G_endTxBuff + 30 /* LEAVE PLACE FOR DSA TAG */;
        resPktP     = pktStartP;
        for (i = 0; i < pktInfoP->numFrags; i++)
        {
            mvOsMemcpy(resPktP,
                       pktInfoP->pFrags[i].bufVirtPtr,
                       pktInfoP->pFrags[i].dataSize);

            resPktP += pktInfoP->pFrags[i].dataSize;
            pktTotalLen += pktInfoP->pFrags[i].dataSize;
        }

        pktInfoP->pFrags[0].bufVirtPtr  = pktStartP;
        pktInfoP->pFrags[0].bufPhysAddr = (MV_ULONG)pktStartP;
        pktInfoP->pFrags[0].dataSize    = pktTotalLen;
        pktInfoP->pktSize               = pktTotalLen;
        mvOsCacheClear(NULL, pktInfoP->pFrags[0].bufVirtPtr, pktInfoP->pktSize);

        DB(mvPrintGndPktInfo(pktInfoP));
        DB(mvDebugMemDump(pktInfoP->pFrags->bufVirtPtr,
                          pktInfoP->pFrags->dataSize, 1));

        /*
         * Actual send.
         */
        if (switchGenSendPkt(pktInfoP, txQ) != MV_OK)
        {
            mvOsPrintf("%s: mvGndSendPkt failed.\n", __func__);
            retVal = ERROR;
            goto tx_out;
        }

        pMblkPkt = pMblkPkt->mBlkHdr.mNextPkt;
    }

    SWITCH_UNLOCK();

tx_out:
    return retVal;
}

/*******************************************************************************
 * switchRxReadyIsrCb
 */
MV_VOID switchRxReadyIsrCb()
{
    G_switchDrvCtrlP->stat.rxReadyIsrCnt++;

    if (netJobAdd((FUNCPTR)switchGenRxJob, 0, 0, 0, 0, 0) != OK)
    {
        /* Problem. May do something here. */
    }
}

/*******************************************************************************
 * switchTxDoneIsrCb
 */
MV_VOID switchTxDoneIsrCb()
{
    G_switchDrvCtrlP->stat.txDoneIsrCnt++;

    if (netJobAdd((FUNCPTR)switchGenTxJob, 0, 0, 0, 0, 0) != OK)
    {
        /* Problem. May do something here. */
    }
}

/*******************************************************************************
 * switchDrvIntInitMiiRx
 */
MV_STATUS switchDrvIntInitMiiRx(void (*rxReadyIsrF)(void *))
{
    if (mvPpChipIsXCat2() == MV_FALSE)
    {
        if (intConnect(INT_VEC_GBE1_RX, rxReadyIsrF, (MV_U32)0) != OK)
        {
            mvOsPrintf("%s: intConnect failed.\n", __func__);
            return MV_FAIL;
        }
    }
    else
    {
        if (intConnect(INT_VEC_GBE0_RX, rxReadyIsrF, (MV_U32)0) != OK)
        {
            mvOsPrintf("%s: intConnect failed.\n", __func__);
            return MV_FAIL;
        }
    }
    return MV_OK;
}

/*******************************************************************************
 * switchDrvIntInitMiiTx
 */
MV_STATUS switchDrvIntInitMiiTx(void (*txDoneIsrF) (void *))
{
    if (mvPpChipIsXCat2() == MV_FALSE)
    {
        if (intConnect(INT_VEC_GBE1_TX, txDoneIsrF, (MV_U32)0) != OK)
        {
            mvOsPrintf("%s: intConnect failed.\n", __func__);
            return MV_FAIL;
        }
    }
    else
    {
        if (intConnect(INT_VEC_GBE0_TX, txDoneIsrF, (MV_U32)0) != OK)
        {
            mvOsPrintf("%s: intConnect failed.\n", __func__);
            return MV_FAIL;
        }
    }

    return MV_OK;
}

/*******************************************************************************
 * switchDrvIntInitSdmaRx
 */
MV_STATUS switchDrvIntInitSdmaRx(void (*rxReadyIsrF)(void *), MV_U32 dev)
{
    int intLvl = switchGenIntLvlGet(dev);

    if (mvPpChipIsXCat2() == MV_TRUE)
    {
        if (intConnect(INUM_TO_IVEC(intLvl),
                       rxReadyIsrF,
                       (MV_U32)0) != OK)
        {
            mvOsPrintf("%s: intConnect failed.\n", __func__);
            return MV_FAIL;
        }
    }
    else
    {
        if (sysPciIntConnect(intLvl, rxReadyIsrF, 0) != OK)
        {
            mvOsPrintf("%s: sysPciIntConnect failed.\n", __func__);
            return MV_FAIL;
        }
    }

    return MV_OK;
}

/*******************************************************************************
 * switchDrvIntInitSdmaTx
 */
MV_STATUS switchDrvIntInitSdmaTx(void (*txDoneIsrF) (void *), MV_U32 dev)
{
    return MV_OK;
}

/*******************************************************************************
 * switchDrvIntEnableMiiRx
 */
MV_STATUS switchDrvIntEnableMiiRx()
{
    /*
     * Enable Layer 1 interrupts (in xCat internal CPU)
     */
    if (mvPpChipIsXCat2() == MV_FALSE)
    {
        intEnable(INT_LVL_GBE1_RX);
    }
    else
    {
        intEnable(INT_LVL_GBE0_RX);
    }

    return MV_OK;
}

/*******************************************************************************
 * switchDrvIntEnableMiiTx
 */
MV_STATUS switchDrvIntEnableMiiTx()
{
    /*
     * Enable Layer 1 interrupts (in xCat internal CPU)
     */
    if (mvPpChipIsXCat2() == MV_FALSE)
    {
        intEnable(INT_LVL_GBE1_TX);
    }
    else
    {
        intEnable(INT_LVL_GBE0_TX);
    }

    return MV_OK;
}

/*******************************************************************************
 * switchDrvIntEnableSdmaRx
 */
MV_STATUS switchDrvIntEnableSdmaRx(MV_U32 dev)
{
    int intLvl = switchGenIntLvlGet(dev);

    /*
     * Enable Layer 1 interrupts (in xCat internal CPU)
     */
    if (mvPpChipIsXCat2() == MV_TRUE)
    {
        if (intEnable(intLvl) != OK)
        {
            mvOsPrintf("%s: intEnable failed.\n", __func__);
            return MV_FAIL;
        }
    }
    else
    {
        if (sysPciIntEnable(intLvl) != OK)
        {
            mvOsPrintf("%s: sysPciIntEnable failed.\n", __func__);
            return MV_FAIL;
        }
    }

    return MV_OK;
}

/*******************************************************************************
 * switchDrvIntEnableSdmaTx
 */
MV_STATUS switchDrvIntEnableSdmaTx(MV_U32 dev)
{
    return MV_OK;
}

/*******************************************************************************
* switchEndStart - Start the device
*
* DESCRIPTION:
*       This routine prepare the switch and system for operation:
*       1) Connects the driver ISR.
*       2) Enables interrupts.
*       4) Marks the interface as active.
*
* INPUT:
*       SW_DRV_CTRL *drvCtrlP       pointer to SW_DRV_CTRL structure
*
* OUTPUT:
*       see description.
*
* RETURN:
*       OK always.
*
*******************************************************************************/
LOCAL STATUS switchEndStart (SW_DRV_CTRL* drvCtrlP)
{
    if (switchGenIsStarted() == MV_TRUE)
    {
        mvOsPrintf("%s Interface is already started.\n", DEV_NAME);
        return OK;
    }

    if (switchGenStart() != MV_OK)
    {
        mvOsPrintf("%s: switchGenStart failed.\n", __func__);
        return ERROR;
    }

    /* Mark the interface as up */
    END_FLAGS_SET (&drvCtrlP->end, IFF_UP | IFF_RUNNING);
    return OK;
}

/*******************************************************************************
* switchEndStop - Stop the END device activity
*
* DESCRIPTION:
*       This routine marks the interface as inactive, disables interrupts and
*       It brings down the interface to a non-operational state.
*       To bring the interface back up, switchEndStart() must be called.
*
* INPUT:
*       None.
*
* OUTPUT:
*       SW_DRV_CTRL *drvCtrlP       pointer to SW_DRV_CTRL structure
*
* RETURN:
*       OK, always.
*
*******************************************************************************/
LOCAL STATUS switchEndStop(SW_DRV_CTRL *drvCtrlP)
{
    if (switchGenIsStarted() == MV_FALSE)
    {
        mvOsPrintf("%s: Interface is NOT started.\n", __func__);
        return OK;
    }

    if (switchGenStop() != MV_OK)
    {
        mvOsPrintf("%s: switchGenStot failed.\n", __func__);
        return ERROR;
    }

    /* Mark the interface as down */
    END_FLAGS_CLR (&drvCtrlP->end, (IFF_UP | IFF_RUNNING));

    mvOsPrintf("%s Interface Stopped.\n", DEV_NAME);
    return OK;
}

/*******************************************************************************
* switchEndIoctl - Interface ioctl procedure
*
* DESCRIPTION:
*       Process an interface ioctl request.
*
* INPUT:
*       SW_DRV_CTRL *drvCtrlP      pointer to SW_DRV_CTRL structure
*       int            cmd      command to process
*       caddr_t       data      pointer to data
*
* OUTPUT:
*       None.
*
* RETURN:
*       OK, or ERROR if the config command failed.
*
*******************************************************************************/
LOCAL STATUS switchEndIoctl(SW_DRV_CTRL *drvCtrlP, unsigned int cmd, caddr_t data)
{
    STATUS           error = OK;
    long             value;
    MV_U32           dev = 0;

    switch (cmd)
        {
    case EIOCGNPT:
        DB(mvOsPrintf ("Ioctl: ifId=0x%x cmd=EIOCGNPT(0x%x), data=0x%x\n",
                 dev, cmd, (int)data));
        error = EINVAL;
        break;

    case EIOCGNAME:
        {
        char nameString[DEV_NAME_LEN + 10];
        DB(mvOsPrintf ("Ioctl: ifId=0x%x cmd=EIOCGNAME(0x%x), data=0x%x\n",
                 dev, cmd, (unsigned int)data));
        sprintf(nameString,"%s%d",DEV_NAME,dev);
        strcpy((char*)data, nameString);
        break;
        }
    case EIOCGHDRLEN:
       DB(mvOsPrintf ("Ioctl: ifId=0x%x cmd=EIOCGHDRLEN(0x%x), data=0x%x\n",
                dev, cmd, (unsigned int)data));
       *(long *)data =  SIZEOF_ETHERHEADER;
       break;
    case EIOCSADDR:
        mvOsPrintf("Ioctl: ifId=0x%x cmd=EIOCSADDR(0x%x), data=0x%x\n",
                     dev, cmd, (unsigned int)data);
        error = EINVAL;
        break;
    case EIOCGADDR:
         mvOsPrintf("Ioctl: ifId=0x%x cmd=EIOCGADDR(0x%x), data=0x%x\n",
                      dev, cmd, (unsigned int)data);
         error = EINVAL;
         break;
    case EIOCSFLAGS:
        {
        value = (long)data;

        DB(mvOsPrintf("Ioctl: ifId=%d, cmd=EIOCSFLAGS(0x%x), data=0x%08lx, oldFlags=0x%08lx\n",
            dev, cmd, (long)data, END_FLAGS_GET(&drvCtrlP->end)) );

        if (value < 0)
        {
        value = -value;
        value--;		/* HELP: WHY ??? */
        END_FLAGS_CLR (&drvCtrlP->end, value);
        }
        else
        {
        END_FLAGS_SET (&drvCtrlP->end, value);
        }
            break;
        }
    case EIOCGFLAGS:
        DB(mvOsPrintf("Ioctl: ifId=0x%x, cmd=EIOCGFLAGS(0x%x), data=0x%x\n",
                          dev, cmd, (int)data) );

        if (data == NULL)
            error = EINVAL;
        else
            *(long *)data = END_FLAGS_GET(&drvCtrlP->end);

        break;
    case EIOCMULTIADD:
    case EIOCMULTIDEL:
    case EIOCMULTIGET:
        DB(mvOsPrintf("Ioctl: ifId=0x%x, cmd=EIOCMULTIX(0x%x), data=0x%x\n",
                  dev, cmd, (int)data) );
        error = EINVAL;
        break;
    case EIOCPOLLSTART: /* fall through */
    case EIOCPOLLSTOP:
        DB(mvOsPrintf("Ioctl: ifId=0x%x, cmd=EIOCPOLLSTART(0x%x), data=0x%x\n",
                dev, cmd, (int)data) );

        error = EINVAL; /* No support for Polling operation mode */
        break;
    case EIOCGMIB2:
        DB(mvOsPrintf("Ioctl: ifId=0x%x, cmd=EIOCGMIB2(0x%x), data=0x%x\n",
                    dev, cmd, (int)data) );

        if (data == NULL)
            return (EINVAL);
        bcopy((char *)&drvCtrlP->end.mib2Tbl, (char *)data,
              sizeof(drvCtrlP->end.mib2Tbl));
        break;
    default:
            DB(mvOsPrintf("Ioctl: UNKNOWN COMMAND: ifId=0x%x, cmd=0x%x, data=0x%x!! \n",
                    dev, cmd, (int)data) );
            error = EINVAL;
        }
    return error;
}

/*******************************************************************************
* switchEndPollSend - Transmit a packet in polled mode.
*
* DESCRIPTION:
*       This routine is called by a user to try and send a packet on the
*       device. It sends a packet directly on the network, without having to
*       go through the normal process of queuing a packet on an output queue
*       and the waiting for the device to decide to transmit it.
*
*       This routine should not call any kernel functions.
*       NOTE: This routine is not supported in this version !!
*
* INPUT:
*       SW_DRV_CTRL *drvCtrlP      pointer to SW_DRV_CTRL structure
*       M_BLK_ID    pMblk       pointer to the mBlk/cluster pair
*
* OUTPUT:
*       None.
*
* RETURN:
*       OK or EAGAIN.
*
*******************************************************************************/
LOCAL STATUS switchEndPollSend(SW_DRV_CTRL *drvCtrlP, M_BLK_ID pMblk)
{
    /* No support for Poll mode */
    return OK;
}

/*******************************************************************************
* switchEndPollReceive - Receive a packet in polled mode.
*
* DESCRIPTION:
*       This routine is called by a user to try and get a packet from the
*       device. It returns EAGAIN if no packet is available. The caller must
*       supply a M_BLK_ID with enough space to contain the received packet. If
*       enough buffer is not available then EAGAIN is returned.
*
*       This routine should not call any kernel functions.
*       NOTE: This routine is not supported in this version !!
*
* INPUT:
*       SW_DRV_CTRL *drvCtrlP      pointer to SW_DRV_CTRL structure
*       M_BLK_ID    pMblk       pointer to the mBlk/cluster pair
*
* OUTPUT:
*       None.
*
* RETURN:
*       OK or EAGAIN.
*
*******************************************************************************/
LOCAL STATUS switchEndPollReceive(SW_DRV_CTRL *drvCtrlP, M_BLK_ID pMblk)
{
    /* No support for Poll mode */
    return OK;
}

/*******************************************************************************
* switchEndMCastAddrAdd - Add a multicast address for the device.
*
* DESCRIPTION:
*       This routine adds a multicast address to whatever the driver is
*       already listening for.
*       NOTE: This routine is not supported in this version !!
*
* INPUT:
*       SW_DRV_CTRL *drvCtrlP      pointer to SW_DRV_CTRL structure
*       char *      pAddr       address to be added
*
* OUTPUT:
*       Multicast address will be accepted.
*
* RETURN:
*       OK or ERROR.
*
*******************************************************************************/
LOCAL STATUS switchEndMCastAddrAdd (SW_DRV_CTRL* drvCtrlP, char* pAddress)
{
    DB(mvOsPrintf("Wwitch interface not support switchEndMCastAddrAdd.\n"));
    return ERROR;
}

/*******************************************************************************
* switchEndMCastAddrDel - Delete a multicast address for the device.
*
* DESCRIPTION:
*       This routine deletes a multicast address from the current list of
*       multicast addresses.
*       NOTE: This routine is not supported in this version !!
*
* INPUT:
*       SW_DRV_CTRL *drvCtrlP      pointer to SW_DRV_CTRL structure
*       char *      pAddr       address to be added
*
* OUTPUT:
*       Multicast address will be accepted.
*
* RETURN:
*       OK or ERROR.
*
*******************************************************************************/
LOCAL STATUS switchEndMCastAddrDel (SW_DRV_CTRL* drvCtrlP, char* pAddress)
{
    DB(mvOsPrintf("Switch interface not support switchEndMCastAddrDel.\n"));
    return ERROR;
}

/*******************************************************************************
* switchEndMCastAddrGet - Get the current multicast address list.
*
* DESCRIPTION:
*       This routine returns the current multicast address list in <pTable>.
*       NOTE: This routine is not supported in this version !!
*
* INPUT:
*       SW_DRV_CTRL *drvCtrlP      pointer to SW_DRV_CTRL structure
*       char *      pAddr       address to be added
*
* OUTPUT:
*       Multicast address will be accepted.
*
* RETURN:
*       OK or ERROR.
*
*******************************************************************************/
LOCAL STATUS switchEndMCastAddrGet (SW_DRV_CTRL* drvCtrlP,
                                   MULTI_TABLE* pTable)
{
    DB(mvOsPrintf("Switch interface not support switchEndMCastAddrGet.\n"));
    return ERROR;
}

/*******************************************************************************
* switchEndNetPoolInit - Initialize memory.
*
* DESCRIPTION:
*       This function allocates the necessary memory space for the Tx/Rx
*       descriptors as well as Rx buffers. Both Tx/Rx descriptor memory space
*       and Rx buffers allocated using the malloc() routine which defines this
*       memory space cacheable as the low level driver supports cacheable
*       descriptors. This is done to allow better packet process performance.
*       The function assigns the Rx buffer memory space to be the network pool
*       cluster memory space which means that the network pool clusters and the
*       Gigabit Ethernet Controller buffers are the same. This allow the zero
*       copy in Rx where the Rx data is placed into the network pool clusters
*       with no copying.
*       This function also makes sure the descriptor and cluster addresses are
*       cache line size aligned in order to avoid data loss when performing
*       data cache flush or invalidate.
*
*       After memory allocation the driver creates and initialize
*           - netBufLib.
*               The pool of mBlk and clBlk is created.
*               The pool of clusters is defined to be the Rx buffers.
*               This way the driver can practice the Zero Copy Buff methodology.
*           - Tx/Rx descriptors.
*               Call low level routine to create Tx descriptor data structure.
*               Call low level routine to create Rx descriptor data structure.
*               Assign each Rx descriptor with netBufLib cluster.
*
* INPUT:
*       None
*
* OUTPUT:
*       Driver memory is ready for Rx and Tx operation.
*
* RETURN:
*       OK if output succeeded.
*       ERROR if - driver failed to allocate one of the address spaces.
*                - <memBase> parameter was not NONE.
*                - failure to initialize netBufLib pools.
*                - fail to retrieve cluster from pool for all Rx descriptors.
*
*******************************************************************************/
static STATUS switchEndNetPoolInit()
{
    SW_DRV_CTRL     *drvCtrlP = G_switchDrvCtrlP;
    M_CL_CONFIG      switchMclBlkConfig;
    POOL_FUNC*       pNetPoolFunc = NULL;
    CL_DESC          switchClDescTbl[1];
    int              queue;
    int              switchClDescTblNumEnt = (NELEMENTS (switchClDescTbl));

    /* Initialize the netPool */
    drvCtrlP->pools.pNetPool = mvOsCalloc(1, sizeof (NET_POOL));
    if (drvCtrlP->pools.pNetPool == NULL)
    {
        mvOsPrintf("%s: Alloc failed.\n", __func__);
        return ERROR;
    }

    /* Calculate number of mBlks, clBlks and Clusters for all queues */
    switchMclBlkConfig.mBlkNum  = 0;
    switchMclBlkConfig.clBlkNum = 0;

    for(queue = 0; queue < MV_ETH_RX_Q_NUM; queue++)
    {
        switchMclBlkConfig.mBlkNum += switchRxQDescNum[queue];
    }

    drvCtrlP->pools.mBlksNum = switchMclBlkConfig.mBlkNum;

    switchMclBlkConfig.mBlkNum += CL_LOAN_NUM;
    switchMclBlkConfig.clBlkNum = switchMclBlkConfig.mBlkNum;
    switchMclBlkConfig.mBlkNum  = switchMclBlkConfig.mBlkNum*4;

    /* Get memory for mBlks */
    switchMclBlkConfig.memSize = (
                    (switchMclBlkConfig.mBlkNum  * (M_BLK_SZ + 4))  +
                    (switchMclBlkConfig.clBlkNum * CL_BLK_SZ + 4));

    drvCtrlP->pools.pMblkBase = mvOsMalloc(switchMclBlkConfig.memSize + 4);
    if(drvCtrlP->pools.pMblkBase == NULL)
    {
        mvOsPrintf("SWITCH: Failed to allocate %d bytes for mBlk and clBlk mem\n",
                            switchMclBlkConfig.memSize+4);
        return ERROR;
    }
    mvOsMemset (drvCtrlP->pools.pMblkBase, 0, (switchMclBlkConfig.memSize+4) );

    /* Align address */
    switchMclBlkConfig.memArea =(void*)MV_ALIGN_UP((MV_U32)drvCtrlP->pools.pMblkBase, 4);

    /* Allocate memory for Rx clusters (including loan) */

    switchClDescTbl[0].clNum = switchMclBlkConfig.clBlkNum;
    switchClDescTbl[0].clSize = CL_BUFF_SIZE;
    switchClDescTbl[0].memSize = (switchClDescTbl[0].clNum *
                  (switchClDescTbl[0].clSize + CL_OVERHEAD));

#if defined(MV_PP_SDMA_UNCACHED_RX_BUFFS)
      drvCtrlP->pools.pClsBase = mvOsIoUncachedMalloc(NULL,
                                (switchClDescTbl[0].memSize + 0x20), NULL, NULL);
#else
      drvCtrlP->pools.pClsBase = mvOsIoCachedMalloc(NULL,
                                (switchClDescTbl[0].memSize + 0x20), NULL, NULL);
#endif

    if(drvCtrlP->pools.pClsBase == NULL)
    {
        mvOsPrintf("SWITCH: Failed to allocate %d bytes for RX Clusters\n",
                            switchClDescTbl[0].memSize + 0x20);
        return ERROR;
    }
    mvOsMemset(drvCtrlP->pools.pClsBase, 0, (switchClDescTbl[0].memSize + 0x20));

    /* Align cluster so the cluser data poiner will be cache line aligned */
    switchClDescTbl[0].memArea = (char*) ((MV_U32)drvCtrlP->pools.pClsBase +
                   (0xC - ((MV_U32)drvCtrlP->pools.pClsBase & 0xF)));

    switchClDescTbl[0].memArea = (char*)((MV_U32)switchClDescTbl[0].memArea|0x10);

#ifdef USE_PRIV_NET_POOL_FUNC
    /* Replace only one function in POOL_FUNC table */
    memcpy(&switchPoolFuncTbl, _pNetPoolFuncTbl, sizeof(POOL_FUNC));
    switchPoolFuncTbl.pMblkClFreeRtn = switchEndMblkClFreeRtn;
    pNetPoolFunc = &switchPoolFuncTbl;
#endif

    /* Init the mem pool */
    if (netPoolInit(drvCtrlP->pools.pNetPool,
                    &switchMclBlkConfig,
                    &switchClDescTbl[0],
                    switchClDescTblNumEnt,
                    pNetPoolFunc) == ERROR)
    {
        mvOsPrintf("netPoolInit failed\n");
        return ERROR;
    }

    if ((drvCtrlP->pools.pClPoolId = netClPoolIdGet (drvCtrlP->pools.pNetPool,
                                                CL_BUFF_SIZE, FALSE)) == NULL)
    {
        mvOsPrintf("%s: netClPoolIdGet failed.\n", __func__);
        return ERROR;
    }

    return OK;
}

/*******************************************************************************
 * switchEndGetMblkClBlkPair
 */
#ifdef SWITCH_END_M_BLK_CUSTOM_POOL
M_BLK_ID switchEndGetMblkClBlkPair()
{
    SW_DRV_CTRL     *drvCtrlP    = G_switchDrvCtrlP;
    M_BLK           *pMblk       = NULL;
    CL_BLK          *pClBlk      = NULL;

    /* Called from sync context, otherwise should be mutually excluded. */
    pMblk = netMblkGet(drvCtrlP->pools.pNetPool, M_DONTWAIT, MT_DATA);
    if (pMblk == NULL)
    {
        mvOsPrintf("%s: netMblkGet failed.\n", __func__);
        return NULL;
    }

    pClBlk = netClBlkGet(drvCtrlP->pools.pNetPool, M_DONTWAIT);
    if (pClBlk == NULL)
    {
        mvOsPrintf("%s: netClBlkGet failed.\n", __func__);
        return NULL;
    }

    if (netMblkClJoin(pMblk, pClBlk) == NULL)
    {
        mvOsPrintf("%s: netMblkClJoin failed.\n", __func__);
        return NULL;
    }

    return pMblk;
}
#endif /* SWITCH_END_M_BLK_CUSTOM_POOL */

/*******************************************************************************
 * switchEndMblkPoolInit
 */
#ifdef SWITCH_END_M_BLK_CUSTOM_POOL
MV_STATUS switchEndMblkPoolInit()
{
    SW_DRV_CTRL     *drvCtrlP    = G_switchDrvCtrlP;
    MV_U32           numOfChunks = drvCtrlP->pools.mBlksNum;
    GEN_SYNC_POOL   *rxMblkPoolP = NULL;
    M_BLK           *pMblk       = NULL;
    MV_U32           i;

    /*
     * Create the pool for mBlk.
     */
    rxMblkPoolP = genSyncPoolCreate(numOfChunks);
    if (rxMblkPoolP == NULL)
    {
        mvOsPrintf("%s: genSyncPoolCreate failed.\n", __func__);
        return MV_FAIL;
    }
    drvCtrlP->pools.rxMblkPoolP = rxMblkPoolP;

    /*
     * Populate the pool.
     */
    for (i = 0; i < numOfChunks; i++)
    {
        pMblk = switchEndGetMblkClBlkPair();
        if (pMblk == NULL)
        {
            mvOsPrintf("%s: switchEndGetMblkClBlkPair failed.\n", __func__);
            return MV_FAIL;
        }

        if (genSyncPoolPut(drvCtrlP->pools.rxMblkPoolP, pMblk) != MV_OK)
        {
            mvOsPrintf("%s: genSyncPoolPut failed.\n", __func__);
            return MV_FAIL;
        }
    }

    return MV_OK;
}
#endif /* SWITCH_END_M_BLK_CUSTOM_POOL */

/*******************************************************************************
* switchEndCleanup
*
* DESCRIPTION:
*       Free all resources allocated for this interface
*
* INPUT:
*       SW_DRV_CTRL *drvCtrlP      pointer to SW_DRV_CTRL structure
*
* OUTPUT:
*       None.
*
* RETURN:
*       None.
*
*******************************************************************************/
LOCAL void switchEndCleanup(SW_DRV_CTRL *drvCtrlP)
{
    /* Free allocated memory if necessary */
    if (drvCtrlP->pools.pClsBase != NULL)
    {
#if defined(MV_PP_SDMA_UNCACHED_RX_BUFFS)
        mvOsIoUncachedFree(NULL, 0, 0, drvCtrlP->pools.pClsBase);
#else
        mvOsIoCachedFree  (NULL, 0, 0, drvCtrlP->pools.pClsBase, 0);
#endif
    }

    if (drvCtrlP->pools.pMblkBase != NULL)
    {
        mvOsFree(drvCtrlP->pools.pMblkBase);
    }

    if (drvCtrlP->pools.pNetPool != NULL)
    {
        if (netPoolDelete(drvCtrlP->pools.pNetPool) != OK)
        {
            mvOsPrintf("%s: netPoolDelete failed.\n");
        }
        mvOsFree(drvCtrlP->pools.pNetPool);
    }
}

/*******************************************************************************
 * switchEndStatus
 */
MV_VOID switchEndPrintStatistics()
{
    SW_DRV_STAT *statP = &G_switchDrvCtrlP->stat;

    mvOsPrintf("%s: Printing SWITCH END statistics:\n", __func__);
    mvOsPrintf("\trxReadyIsrCnt = %8d.\n", statP->rxReadyIsrCnt);
    mvOsPrintf("\ttxDoneIsrCnt  = %8d.\n", statP->txDoneIsrCnt);
}

/*******************************************************************************
 * switchEndStatus
 */
void switchEndStatus(int dev)
{
    mvOsPrintf("\t SWITCH dev #%d Status\n\n", dev);

    mvOsPrintf("Interface %s State: %s %s %s\n", DEV_NAME,
                switchGenIsAttached() ? "Loaded"    : "Unloaded",
                switchGenIsStarted()  ? "+ Started" : "+ Stopped",
                switchGenIsLinkUp()   ? "+ Up"      : "+ Down");

    switchEndPrintStatistics();
}

