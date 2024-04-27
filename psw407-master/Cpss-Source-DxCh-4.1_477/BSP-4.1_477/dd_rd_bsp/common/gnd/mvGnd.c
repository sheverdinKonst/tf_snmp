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

#include "mvGnd.h"
#include "mvGndHwIf.h"
#include "mvGndOsIf.h"
#include "mvGndReg.h"
#include "mvOs.h"
#include "bitOps.h"
#include "mvEth.h"
#include "mv802_3.h"
#include "mvGenSyncPool.h"
#include "mvGenBuffPool.h"
#include "mvSysHwConfig.h"

#undef MV_DEBUG         
#ifdef MV_DEBUG         
#define DB(x) x
#else
#define DB(x)
#endif

MV_U32 G_mvGndRxReadyQBits = 0;
MV_U32 G_mvGndTxDoneQBits  = 0;

typedef struct _mvGndStat
{
    MV_U32           rxReadyIntCnt;
    MV_U32           txDoneIntCnt;
} GND_STAT;

typedef struct _mvGndCtrl
{
    MV_BOOL          started;
    MV_U32           gbeIndex;
    MV_U8            macAddr[MV_MAC_ADDR_SIZE];
    MV_BOOL          isTxSyncMode;
    MV_U32           pollTimoutUSec;
    MV_U32           maxPollTimes;
    MV_BOOL          isToDel2PrepBytes;
    MV_U32           maxFragsInPkt;
    GEN_SYNC_POOL   *rxPktInfoPoolP;
    GEN_BUFF_POOL   *rxBuffPoolP;
    MV_U32           rxDescNumPerQ[MV_ETH_RX_Q_NUM];
    MV_U32           txDescNumPerQ[MV_ETH_TX_Q_NUM];
    MV_GND_HW_FUNCS *hwP;
    MV_GND_OS_FUNCS *osP;
    GND_STAT         stat;
} GND_CTRL;

GND_CTRL  G_genDrvCtrl  = {
    0
};
GND_CTRL *G_genDrvCtrlP = &G_genDrvCtrl;

MV_BOOL   G_isGndInited = MV_FALSE;
extern MV_BOOL standalone_network_device;

/*******************************************************************************
 * mvGndMacAddrGet
 */
MV_BOOL mvGndIsInited()
{
    return G_isGndInited;
}

/*******************************************************************************
 * mvGndMacAddrGet
 */
MV_U8 * mvGndMacAddrGet()
{
    return G_genDrvCtrlP->macAddr;
}

/*******************************************************************************
 * mvGndIsTxSyncModeSet
 */
MV_VOID mvGndIsTxSyncModeSet(MV_U32 pollTimoutUSec,
                             MV_U32 maxPollTimes)
{
    GND_CTRL           *genCtrlP = G_genDrvCtrlP;
    MV_GND_HW_FUNCS    *hwP      = genCtrlP->hwP;

    hwP->mvGndHwIntMaskTxDoneF (MV_ETH_RX_Q_ALL);
    hwP->mvGndHwIntAckTxDoneF (MV_ETH_RX_Q_ALL);

    G_genDrvCtrlP->isTxSyncMode   = MV_TRUE;
    G_genDrvCtrlP->pollTimoutUSec = pollTimoutUSec;
    G_genDrvCtrlP->maxPollTimes   = maxPollTimes;
}

/*******************************************************************************
 * mvGndIsTxSyncModeUnset
 */
MV_VOID mvGndIsTxSyncModeUnset()
{
    GND_CTRL           *genCtrlP = G_genDrvCtrlP;
    MV_GND_HW_FUNCS    *hwP      = genCtrlP->hwP;

    hwP->mvGndHwIntAckTxDoneF   (MV_ETH_RX_Q_ALL);
    hwP->mvGndHwIntUnmaskTxDoneF (MV_ETH_RX_Q_ALL);

    G_genDrvCtrlP->isTxSyncMode   = MV_FALSE;
}

/*******************************************************************************
 * mvGndIsTxSyncModeGet
 */
MV_BOOL mvGndIsTxSyncModeGet()
{
    return G_genDrvCtrlP->isTxSyncMode;
}

/*******************************************************************************
 * mvGndDel2PrepBytesModeSet
 */
MV_VOID mvGndDel2PrepBytesModeSet(MV_BOOL isToDel2PrepBytes)
{
    G_genDrvCtrlP->isToDel2PrepBytes = isToDel2PrepBytes;
}

/*******************************************************************************
 * mvGndDel2PrepBytesModeGet
 */
MV_BOOL mvGndDel2PrepBytesModeGet()
{
    return G_genDrvCtrlP->isToDel2PrepBytes;
}

/*******************************************************************************
 * mvGndMaxFragsInPktGet
 */
MV_U32 mvGndMaxFragsInPktGet()
{
    return G_genDrvCtrlP->maxFragsInPkt;
}

/*******************************************************************************
 * mvGndInitSw
 */
MV_STATUS mvGndInitSw(MV_GND_INIT *initP)
{
    GND_CTRL           *genCtrlP = G_genDrvCtrlP;
    MV_U32              i, txDescNumTotal = 0;

    mvOsMemset (genCtrlP, 0, sizeof(GND_CTRL));
    G_isGndInited = MV_TRUE;

    genCtrlP->hwP           = initP->hwP;
    genCtrlP->osP           = initP->osP;
    genCtrlP->maxFragsInPkt = initP->maxFragsInPkt;
    genCtrlP->gbeIndex      = initP->gbeIndex;

    if (mvGndRxInit (&initP->rx) != MV_OK)
    {
        mvOsPrintf ("%s: mvGndRxInit failed.\n", __func__);
        return MV_FAIL;
    }

    for (i = 0; i < MV_ETH_TX_Q_NUM; i++)
    {
        txDescNumTotal += initP->tx.txDescNumPerQ[i];
    }
    if (mvGndTxInit (&initP->tx) != MV_OK)
    {
        mvOsPrintf ("%s: mvGndTxInit failed.\n", __func__);
        return MV_FAIL;
    }

    return MV_OK;
}

/*******************************************************************************
 * mvGndRxReadyIsr
 */
MV_VOID mvGndRxReadyIsr()
{
    GND_CTRL           *genCtrlP = G_genDrvCtrlP;
    MV_GND_HW_FUNCS    *hwP      = genCtrlP->hwP;
    MV_GND_OS_FUNCS    *osP      = genCtrlP->osP;
    MV_U32              rxReadyQBits;

    genCtrlP->stat.rxReadyIntCnt++;

    hwP->mvGndHwIntMaskRxReadyF (MV_ETH_RX_Q_ALL);
    rxReadyQBits = hwP->mvGndHwQGetRxReadyF ();
    hwP->mvGndHwIntAckRxReadyF (rxReadyQBits);

    /* Store bit mask of RX Ready queues for processing in bottom half. */
    G_mvGndRxReadyQBits = rxReadyQBits;

    /* Schedule bottom half to run. */
    osP->mvGndOsIsrCbRxReadyF ();
}

/*******************************************************************************
 * mvGndTxDoneIsr
 */
MV_VOID mvGndTxDoneIsr()
{
    GND_CTRL           *genCtrlP = G_genDrvCtrlP;
    MV_GND_HW_FUNCS    *hwP      = genCtrlP->hwP;
    MV_GND_OS_FUNCS    *osP      = genCtrlP->osP;
    MV_U32              txDoneQBits;

    genCtrlP->stat.txDoneIntCnt++;

    hwP->mvGndHwIntMaskTxDoneF (MV_ETH_TX_Q_ALL);
    txDoneQBits = hwP->mvGndHwQGetTxDoneF ();
    hwP->mvGndHwIntAckTxDoneF (txDoneQBits);

    /* Store bit mask of RX Ready queues for processing in bottom half. */
    G_mvGndTxDoneQBits = txDoneQBits;

    /* Schedule bottom half to run. */
    osP->mvGndOsIsrCbTxDoneF ();
}

/*******************************************************************************
 * mvGndGetRxPktDo
 */
MV_GND_PKT_INFO * mvGndGetRxPktDo(MV_U32 rxQ)
{
    GND_CTRL           *genCtrlP = G_genDrvCtrlP;
    MV_GND_HW_FUNCS    *hwP      = genCtrlP->hwP;
    MV_GND_PKT_INFO    *pktInfoP;

    pktInfoP = (MV_GND_PKT_INFO *)hwP->mvGndHwGetPktRxReadyF (rxQ);
    if (pktInfoP == NULL)
    {
        /*
         * No RX ready packets.
         */
        return NULL;
    }

    if (ETH_ERROR_SUMMARY_GOOD_PKT (pktInfoP->status) == MV_TRUE)
    {
        /*
         * If needed, adjust RX buffer according to policy of
         * two bytes always-prepended to every RX frame by GbE1.
         */
        if (genCtrlP->isToDel2PrepBytes == MV_TRUE)
        {
            pktInfoP->pFrags->bufPhysAddr += 2;
            pktInfoP->pFrags->bufVirtPtr  += 2;
            pktInfoP->pFrags->dataSize    -= 2;
            pktInfoP->pktSize             -= 2;
        }
    }
    else
    {
        /*
         * Error in the frame, ==> return resources.
         */
    }

    return pktInfoP;
}

/*******************************************************************************
 * mvGndGetRxPkt
 */
MV_GND_PKT_INFO * mvGndGetRxPkt(MV_U32 rxQ)
{
    GND_CTRL           *genCtrlP = G_genDrvCtrlP;
    MV_GND_OS_FUNCS    *osP      = genCtrlP->osP;
    MV_GND_PKT_INFO    *pktInfoP;

    osP->mvGndOsRxPathLockF ();
    pktInfoP = mvGndGetRxPktDo (rxQ);
    osP->mvGndOsRxPathUnlockF ();

    return pktInfoP;
}

/*******************************************************************************
 * mvGndRxBottomHalf
 */
MV_STATUS mvGndRxBottomHalf()
{
    GND_CTRL           *genCtrlP = G_genDrvCtrlP;
    MV_GND_HW_FUNCS    *hwP      = genCtrlP->hwP;
    MV_GND_OS_FUNCS    *osP      = genCtrlP->osP;
    MV_GND_PKT_INFO    *pktInfoP = NULL;
    MV_U32              rxReadyQBits, bits, rxQ;
    MV_STATUS           retVal;

    osP->mvGndOsRxPathLockF ();
    retVal = MV_OK;

    /*
     * If new RX queues have become Ready, process them.
     */
    rxReadyQBits  = hwP->mvGndHwQGetRxReadyF ();
    rxReadyQBits |= G_mvGndRxReadyQBits;
    if (rxReadyQBits == 0)
    {
        /* If no RX_READY queues found, then nothing to do. */
        goto rx_out;
    }

    /*
     * Process all RX_READY queues.
     */
    do
    {
        /*
         * Process all RX_READY queues starting from RX queue 0 (LSB bit).
         */
        hwP->mvGndHwIntAckRxReadyF (rxReadyQBits);
        bits = rxReadyQBits;
        while (bits != 0)
        {
            /* Find rxQ bit: possible values [0-7]. */
            rxQ = mvBitMaskFfs (bits);
            /* Unset processed RX queue in bit mask. */
            bits &= ~(1 << rxQ);

            /*
             * Process all received frames for the current queue.
             */
            pktInfoP = mvGndGetRxPktDo (rxQ);
            while (pktInfoP != NULL)
            {
                if (osP->mvGndOsFwdRxPktF (pktInfoP, rxQ) != MV_OK)
                {
                    DB (mvOsPrintf ("%s: mvGndOsFwdRxPktF failed.\n", __func__));
                    retVal = MV_FAIL;
                    /* goto rx_out; */
                }

                pktInfoP = mvGndGetRxPktDo (rxQ);
            }
        }

        rxReadyQBits = hwP->mvGndHwQGetRxReadyF ();
    } while (rxReadyQBits != 0);

rx_out:
    hwP->mvGndHwIntUnmaskRxReadyF (MV_ETH_RX_Q_ALL);
    osP->mvGndOsRxPathUnlockF ();
    return retVal;
}

/*******************************************************************************
 * mvGndTxBottomHalfDo
 */
MV_STATUS mvGndTxBottomHalfDo(MV_VOID)
{
    GND_CTRL           *genCtrlP = G_genDrvCtrlP;
    MV_GND_HW_FUNCS    *hwP      = genCtrlP->hwP;
    MV_GND_OS_FUNCS    *osP      = genCtrlP->osP;
    MV_GND_PKT_INFO    *pktInfoP = NULL;
    MV_U32              txDoneQBits, bits, txQ, numFrags;

    /*
     * If new TX queues have become Done, process them.
     */
    txDoneQBits  = hwP->mvGndHwQGetTxDoneF ();
    txDoneQBits |= G_mvGndTxDoneQBits;
    if (txDoneQBits == 0)
    {
        /* If no TX_DONE queues found, then nothing to do. */
        return MV_OK;
    }

    /*
     * Process all TX_DONE queues.
     */
    do
    {
        /*
         * Process all TX_DONE queues starting from TX queue 0 (LSB bit).
         */
        hwP->mvGndHwIntAckTxDoneF (txDoneQBits);
        bits = txDoneQBits;
        while (bits != 0)
        {
            /* Find txQ bit: possible values [0-7]. */
            txQ = mvBitMaskFfs (bits);
            /* Unset processed TX queue in bit mask. */
            bits &= ~(1 << txQ);

            /*
             * Process all transmitted frames for the current queue.
             */
            DB(mvOsPrintf("%s: calling mvGndHwGetPktTxDoneF(txQ = %d).\n",
                          __func__, txQ));
            pktInfoP = (MV_GND_PKT_INFO *)hwP->mvGndHwGetPktTxDoneF (txQ);
            if (pktInfoP == NULL)
            {
                mvOsPrintf ("%s: mvGndHwGetPktTxDoneF failed.\n", __func__);
            }

            while (pktInfoP != NULL)
            {
                numFrags = pktInfoP->numFrags;

                if ((pktInfoP->status & ETH_ERROR_SUMMARY_MASK) == 0)
                {
                    /*
                     * Process TX_DONE frame.
                     */
                    if (osP->mvGndOsUserTxDoneF (pktInfoP) != MV_OK)
                    {
                        mvOsPrintf ("%s: mvGndOsUserTxDoneF failed.\n", __func__);
                        /*
                         * If callback fails, GND should NOT be 'punished' or
                         * 'punish' the caller.
                         */
                        /* return MV_FAIL; */
                    }
                }
                else
                {
                    /* Error packet */
                    mvOsPrintf ("%s: Error pkt (status = 0x%08X).\n",
                                __func__, pktInfoP->status);
                }

                DB(mvOsPrintf("%s: calling mvGndHwGetPktTxDoneF(txQ = %d).\n",
                              __func__, txQ));
                pktInfoP = (MV_GND_PKT_INFO *)hwP->mvGndHwGetPktTxDoneF (txQ);
            }
        }

        txDoneQBits = hwP->mvGndHwQGetTxDoneF ();
    } while (txDoneQBits != 0);

    return MV_OK;
}

/*******************************************************************************
 * mvGndTxBottomHalf
 */
MV_STATUS mvGndTxBottomHalf()
{
    GND_CTRL           *genCtrlP = G_genDrvCtrlP;
    MV_GND_HW_FUNCS    *hwP      = genCtrlP->hwP;
    MV_GND_OS_FUNCS    *osP      = genCtrlP->osP;

    osP->mvGndOsTxPathLockF ();

    if (mvGndTxBottomHalfDo () != MV_OK)
    {
        mvOsPrintf ("%s: mvGndTxBottomHalfDo failed.\n", __func__);
        osP->mvGndOsTxPathUnlockF ();
        return MV_FAIL;
    }

    hwP->mvGndHwIntUnmaskTxDoneF (MV_ETH_TX_Q_ALL);
    osP->mvGndOsTxPathUnlockF ();
    return MV_OK;
}

/*******************************************************************************
 * mvGndFreeRxPkt
 */
MV_STATUS mvGndFreeRxPkt(MV_GND_PKT_INFO *pktInfoP, MV_U32 rxQ)
{
    GND_CTRL           *genCtrlP = G_genDrvCtrlP;
    MV_GND_HW_FUNCS    *hwP      = genCtrlP->hwP;
    MV_GND_OS_FUNCS    *osP      = genCtrlP->osP;
    MV_STATUS           retVal;
    MV_STATUS           status;

    retVal = MV_OK;

    /*
     * Take care of two bytes always-prepended to every RX frame by GbE1.
     */
    if (genCtrlP->isToDel2PrepBytes == MV_TRUE)
    {
        pktInfoP->pktSize             += 2;
        pktInfoP->pFrags->dataSize    += 2;
        pktInfoP->pFrags->bufVirtPtr  -= 2;
        pktInfoP->pFrags->bufPhysAddr -= 2;
    }

    osP->mvGndOsRxPathLockF ();

    status = hwP->mvGndHwRxRefillF ((MV_PKT_INFO *)pktInfoP, rxQ);
    if (status != MV_OK && status != MV_FULL)
    {
        mvOsPrintf ("%s: mvGndHwRxRefillF failed.\n", __func__);
        retVal = MV_FAIL;
        goto out;
    }

out:
    osP->mvGndOsRxPathUnlockF ();
    return retVal;
}

/*******************************************************************************
 * mvGndDoSendPkt
 */
static MV_STATUS mvGndDoSendPkt(MV_GND_PKT_INFO *pktInfoP, MV_U32 txQ)
{
    GND_CTRL           *genCtrlP = G_genDrvCtrlP;
    MV_GND_HW_FUNCS    *hwP      = genCtrlP->hwP;
    MV_U32              count;
    MV_STATUS           status;

    status = hwP->mvGndHwPktTxF ((MV_PKT_INFO *)pktInfoP, txQ);
    if (status != MV_OK && status != MV_NO_RESOURCE)
    {
        DB (mvOsPrintf ("%s: mvGndHwPktTxF failed.\n", __func__));
        return MV_FAIL;
    }

    /*
     * SYNC TX mode: wait till TX is finished.
     */
    if (genCtrlP->isTxSyncMode == MV_TRUE)
    {
        count = 0;
        while (hwP->mvGndHwIsTxDoneF (txQ) == MV_FALSE)
        {
            mvOsUDelay (genCtrlP->pollTimoutUSec);

            count++;
            if (count > genCtrlP->maxPollTimes)
            {
                mvOsPrintf ("%s: Polling timeout.\n", __func__);
                return MV_FAIL;
            }
        }

        if (mvGndTxBottomHalfDo () != MV_OK)
        {
            mvOsPrintf ("%s: mvGndTxBottomHalfDo failed.\n", __func__);
            return MV_FAIL;
        }
    }

    return MV_OK;
}

/*******************************************************************************
 * mvGndSendPkt
 */
MV_STATUS mvGndSendPkt(MV_GND_PKT_INFO *pktInfoP, MV_U32 txQ)
{
    GND_CTRL           *genCtrlP = G_genDrvCtrlP;
    MV_GND_OS_FUNCS    *osP      = genCtrlP->osP;
    MV_STATUS           retVal;

    osP->mvGndOsTxPathLockF ();
    retVal   = MV_OK;

    if (mvGndDoSendPkt (pktInfoP, txQ) != MV_OK)
    {
        mvOsPrintf ("%s: mvGndDoSendPkt failed.\n", __func__);
        retVal = MV_FAIL;
        goto out;
    }

out:
    osP->mvGndOsTxPathUnlockF ();
    return retVal;
}

/*******************************************************************************
 * mvGndRxResourceGet
 */
MV_U32 mvGndRxResourceGet()
{
    GND_CTRL *genCtrlP = G_genDrvCtrlP;
    return genBuffPoolTotalNumGet (genCtrlP->rxBuffPoolP);
}

/*******************************************************************************
 * mvGndRxInitDo
 */
MV_STATUS mvGndRxInitDo(MV_U32  rxBuffsBulkSize,
                        MV_U8 * rxBuffsBulk,
                        MV_U32  buffSize,
                        MV_U32 *actualBuffNumP,
                        MV_U32  hdrOffset)
{
    GND_CTRL             *genCtrlP      = G_genDrvCtrlP;
    GEN_BUFF_POOL        *rxBuffPoolP   = NULL;
    MV_GND_PKT_INFO      *pktInfoBulkP  = NULL;
    MV_GND_PKT_INFO      *pktInfoP      = NULL;
    MV_GND_BUF_INFO      *bufInfoBulkP  = NULL;
    MV_GND_BUF_INFO      *bufInfoP      = NULL;
    MV_U32                totalBuffNum  = 0, i;

    /*
     * create RX pool of buffers
     */
    rxBuffPoolP = genBuffPoolCreate (rxBuffsBulkSize,
                                     rxBuffsBulk,
                                     buffSize,
                                     actualBuffNumP,
                                     hdrOffset,
                                     MV_GND_BUFF_ALIGN_DEFAULT,
                                     MV_GND_BUFF_MIN_SIZE_DEFAULT);
    if (rxBuffPoolP == NULL)
    {
        mvOsPrintf ("%s: genBuffPoolCreate failed.\n", __func__);
        return MV_FAIL;
    }
    genCtrlP->rxBuffPoolP = rxBuffPoolP;

    /*
     * Create RX pool of (MV_GND_PKT_INFO-->MV_GND_BUF_INFO) tuples.
     */
    totalBuffNum = genBuffPoolTotalNumGet (rxBuffPoolP);

    if (totalBuffNum == 0)
    {
        mvOsPrintf ("%s: totalBuffNum is 0.\n", __func__);
        goto fail_out;
    }

    genCtrlP->rxPktInfoPoolP = genSyncPoolCreate (totalBuffNum);
    if (genCtrlP->rxPktInfoPoolP == NULL)
    {
        mvOsPrintf ("%s: genSyncPoolCreate failed.\n", __func__);
        goto fail_out;
    }

    pktInfoBulkP = (MV_GND_PKT_INFO *)mvOsCalloc (totalBuffNum,
                                                  sizeof(MV_GND_PKT_INFO));
    if (pktInfoBulkP == NULL)
    {
        mvOsPrintf ("%s:%d: mvOsCalloc failed.\n", __func__, __LINE__);
        goto fail_out;
    }

    bufInfoBulkP = (MV_GND_BUF_INFO *)mvOsCalloc (totalBuffNum,
                                                  sizeof(MV_GND_BUF_INFO));
    if (bufInfoBulkP == NULL)
    {
        mvOsPrintf ("%s:%d: mvOsCalloc failed.\n", __func__, __LINE__);
        goto fail_out;
    }

    DB (mvOsPrintf ("%s: Filling rxPktInfoPool by %d pktInfos:\n",
                    __func__, totalBuffNum));

    for (i = 0; i < totalBuffNum; i++)
    {
        bufInfoP = bufInfoBulkP + i;
        pktInfoP = pktInfoBulkP + i;

        DB (mvOsPrintf ("%s: bufInfoP = 0x%08X, pktInfoP = 0x%08X.\n",
                        __func__, bufInfoP, pktInfoP));

        pktInfoP->pFrags = bufInfoP;
        if (genSyncPoolPut (genCtrlP->rxPktInfoPoolP, pktInfoP) != MV_OK)
        {
            mvOsPrintf ("%s:%d: genSyncPoolPut failed.\n", __func__, __LINE__);
            goto fail_out;
        }
    }

    goto ok_out;

fail_out:
    if (rxBuffPoolP)
    {
        genBuffPoolDestroy (rxBuffPoolP);
    }
    if (genCtrlP->rxPktInfoPoolP)
    {
        genSyncPoolDestroy (genCtrlP->rxPktInfoPoolP);
    }
    if (pktInfoBulkP)
    {
        mvOsFree (pktInfoBulkP);
    }
    if (bufInfoBulkP)
    {
        mvOsFree (bufInfoBulkP);
    }
    return MV_FAIL;

ok_out:
    return MV_OK;
}

/*******************************************************************************
 * mvGndRxInit
 */
MV_STATUS mvGndRxInit(MV_GND_RX_INIT *rxInitP)
{
    GND_CTRL             *genCtrlP     = G_genDrvCtrlP;

    if (mvGndRxInitDo (rxInitP->rxBuffsBulkSize,
                       rxInitP->rxBuffsBulkP,
                       rxInitP->rxBuffSize,
                       rxInitP->actualRxBuffsNumP,
                       rxInitP->rxBuffHdrOffset) != MV_OK)
    {
        mvOsPrintf ("%s: mvGndRxInitDo failed.\n", __func__);
        return MV_FAIL;
    }

    mvOsMemcpy (genCtrlP->rxDescNumPerQ,
                rxInitP->rxDescNumPerQ,
                MV_ETH_RX_Q_NUM * sizeof(MV_U32));

    genCtrlP->isToDel2PrepBytes = rxInitP->isToDel2PrepBytes;
    mvOsMemcpy (genCtrlP->macAddr, rxInitP->macAddr, MV_MAC_ADDR_SIZE);

    return MV_OK;
}

/*******************************************************************************
 * mvGndTxPktInfoPoolCreate
 */
GEN_SYNC_POOL *mvGndTxPktInfoPoolCreate(MV_U32 totalNum, MV_U32 maxFragsInPkt)
{
    GND_CTRL             *genCtrlP = G_genDrvCtrlP;
    GEN_SYNC_POOL        *txPktInfoPoolP = NULL;
    MV_GND_PKT_INFO      *pktInfoBulkP   = NULL;
    MV_GND_PKT_INFO      *pktInfoP       = NULL;
    MV_GND_BUF_INFO      *bufInfoBulkP   = NULL;
    MV_GND_BUF_INFO      *bufInfoP       = NULL;
    MV_U32                i;

    if (totalNum == 0)
    {
        mvOsPrintf ("%s: totalNum is 0.\n", __func__);
        return NULL;
    }

    /*
     * Create TX pool of (MV_GND_PKT_INFO-->MV_GND_BUF_INFO[]) tuples.
     */
    txPktInfoPoolP = genSyncPoolCreate (totalNum);
    if (txPktInfoPoolP == NULL)
    {
        mvOsPrintf ("%s: genSyncPoolCreate failed.\n", __func__);
        return NULL;
    }

    pktInfoBulkP = (MV_GND_PKT_INFO *)mvOsCalloc (totalNum, sizeof(MV_GND_PKT_INFO));
    if (pktInfoBulkP == NULL)
    {
        mvOsPrintf ("%s:%d: mvOsCalloc failed.\n", __func__, __LINE__);
        return NULL;
    }

    bufInfoBulkP = (MV_GND_BUF_INFO *)mvOsCalloc (
        totalNum,
        sizeof(MV_GND_BUF_INFO) *
        maxFragsInPkt);
    if (bufInfoBulkP == NULL)
    {
        mvOsFree(pktInfoBulkP);
        mvOsPrintf ("%s:%d: mvOsCalloc failed.\n", __func__, __LINE__);
        return NULL;
    }

    DB (mvOsPrintf ("%s: Creating TX pool of {pktInfo->bufInfo} pairs.\n", __func__));

    for (i = 0; i < totalNum; i++)
    {
        bufInfoP = bufInfoBulkP + i * genCtrlP->maxFragsInPkt;
        pktInfoP = pktInfoBulkP + i;

        DB (mvOsPrintf ("%s: bufInfoP = 0x%08X, pktInfoP = 0x%08X.\n",
                        __func__, bufInfoP, pktInfoP));

        pktInfoP->pFrags = bufInfoP;
        if (genSyncPoolPut (txPktInfoPoolP, pktInfoP) != MV_OK)
        {
            mvOsPrintf ("%s:%d: genSyncPoolPut failed.\n", __func__, __LINE__);
            return NULL;
        }
    }

    return txPktInfoPoolP;
}

/*******************************************************************************
 * mvGndTxInit
 */
MV_STATUS mvGndTxInit(MV_GND_TX_INIT *txInitP)
{
    GND_CTRL             *genCtrlP = G_genDrvCtrlP;

    mvOsMemcpy (genCtrlP->txDescNumPerQ,
                txInitP->txDescNumPerQ,
                MV_ETH_TX_Q_NUM * sizeof(MV_U32));

    genCtrlP->isTxSyncMode   = txInitP->isTxSyncMode;
    genCtrlP->pollTimoutUSec = txInitP->pollTimoutUSec;
    genCtrlP->maxPollTimes   = txInitP->maxPollTimes;

    return MV_OK;
}

/*******************************************************************************
* mvGndRxRefill
*
* DESCRIPTION:
*       Fills GbE1 RX SDMA descriptors with
*       (MV_GND_PKT_INFO->MV_GND_BUF_INFO->RX_buff) tuples.
*
* INPUTS:
*       genCtrlP     - pointer to the driver control structure
*       queue        - RX queue for which to fill RX SDMA descriptors
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK if successful, or
*       MV_FAIL otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_STATUS mvGndRxRefill(MV_U32 rxQ, MV_U32 num)
{
    GND_CTRL           *genCtrlP = G_genDrvCtrlP;
    MV_GND_HW_FUNCS    *hwP      = genCtrlP->hwP;
    MV_U8              *buffP;
    MV_GND_BUF_INFO    *bufInfoP;
    MV_GND_PKT_INFO    *pktInfoP;
    MV_U32              count;
    MV_U32              buffSize;
    MV_STATUS           status;

    buffSize = genBuffPoolBuffSizeGet (genCtrlP->rxBuffPoolP);
    count    = hwP->mvGndHwRxResourceGetF (rxQ);
    while (count < num)
    {
        buffP = genBuffPoolGet (genCtrlP->rxBuffPoolP);
        if (buffP == NULL)
        {
            mvOsPrintf ("%s: genBuffPoolGet failed.\n", __func__);
            genBuffPoolPrint (genCtrlP->rxBuffPoolP);
            return MV_FAIL;
        }

        pktInfoP = (MV_GND_PKT_INFO *)genSyncPoolGet (genCtrlP->rxPktInfoPoolP);
        if (pktInfoP == NULL)
        {
            mvOsPrintf ("%s: genSyncPoolGet failed.\n", __func__);
            return MV_FAIL;
        }
        pktInfoP->nextP       = NULL;
        pktInfoP->pktSize     = buffSize;

        bufInfoP = pktInfoP->pFrags;
        bufInfoP->bufVirtPtr  = buffP;
        bufInfoP->bufPhysAddr = mvOsIoVirtToPhy (NULL, buffP);
        bufInfoP->bufSize     = buffSize;
        bufInfoP->dataSize    = buffSize;

        DB (mvOsPrintf (
                "%s: %3d: bufInfoP = 0x%08X, pktInfoP = 0x%08X, buffP = 0x%08X\n",
                __func__, count, bufInfoP, pktInfoP, buffP));

        status = hwP->mvGndHwRxRefillF ((MV_PKT_INFO *)pktInfoP, rxQ);
        if (status != MV_OK && status != MV_FULL)
        {
            mvOsPrintf ("%s: mvGndHwRxRefillF failed.\n", __func__);
            return MV_FAIL;
        }
        count++;
    }

    return MV_OK;
}

/*******************************************************************************
 * mvGndEnableHw
 */
MV_STATUS mvGndEnableHw(MV_VOID)
{
    GND_CTRL           *genCtrlP = G_genDrvCtrlP;
    MV_GND_HW_FUNCS    *hwP      = genCtrlP->hwP;

    /* Set MAC address for GbE1 and configure CPU MAC in Prestera MAC table */
    if (hwP->mvGndHwMacAddrSetF (genCtrlP->macAddr, MII_DEF_RXQ) != MV_OK)
    {
        mvOsPrintf ("%s: mvGndHwMacAddrSetF failed.\n", __func__);
        return MV_FAIL;
    }

    if (hwP->mvGndHwEnableF () != MV_OK)
    {
        mvOsPrintf ("%s: mvGndHwEnableF failed.\n", __func__);
        return MV_FAIL;
    }

    return MV_OK;
}

/*******************************************************************************
 * mvGndEnable
 */
#include "mvEth.h"
#include "mii.h"

MV_STATUS mvGndEnable(MV_U32 defaultRxQ,
                      MV_U32 rxBuffSize,
                      MV_U32 mru)
{
    GND_CTRL           *genCtrlP = G_genDrvCtrlP;
    MV_GND_HW_FUNCS    *hwP      = genCtrlP->hwP;
    MV_ETH_PORT_INIT    portInit;
    MV_ETH_INIT         ethInit;
    MV_U32              queue, i;

    /*
     * alloc RX/TX desc rings + config RMGII to default values
     */
    mvOsMemset (&portInit, 0, sizeof(MV_ETH_PORT_INIT));
    mvOsMemset (&ethInit,  0, sizeof(MV_ETH_INIT));

    portInit.rxDefQ       = MII_DEF_RXQ;
    portInit.maxRxPktSize = MII_RX_BUFF_SIZE_DEFAULT;
    portInit.mru          = MII_MRU_DEFAULT;

    DB (mvOsPrintf ("%s: Configuring # of RX descriptors.\n", __func__));
    for (i = 0; i < MV_ETH_RX_Q_NUM; i++)
    {
        portInit.rxDescrNum[i] = genCtrlP->rxDescNumPerQ[i];
        ethInit.rxDescTotal   += genCtrlP->rxDescNumPerQ[i];
        DB (mvOsPrintf ("%s: queue = %d, descNum = %d.\n",
                        __func__, i, genCtrlP->rxDescNumPerQ[i]));
    }

    DB (mvOsPrintf ("%s: Configuring # of TX descriptors.\n", __func__));
    for (i = 0; i < MV_ETH_TX_Q_NUM; i++)
    {
        portInit.txDescrNum[i] = genCtrlP->txDescNumPerQ[i];
        ethInit.txDescTotal   += genCtrlP->txDescNumPerQ[i];
        DB (mvOsPrintf ("%s: queue = %d, descNum = %d.\n",
                        __func__, i, genCtrlP->txDescNumPerQ[i]));
    }

    ethInit.ethP     = &portInit;
    ethInit.gbeIndex = genCtrlP->gbeIndex;
    if (hwP->mvGndHwInitF (&ethInit) != MV_OK)
    {
        mvOsPrintf ("%s: mvGndHwInitF failed.\n", __func__);
        return MV_FAIL;
    }

    /* Fill MII RX queue with RX buffers coupled with MV_GND_PKT_INFO */
    for (queue = 0; queue < MV_ETH_RX_Q_NUM; queue++)
    {
        DB (mvOsPrintf ("%s: calling to mvGndRxRefill: queue = %d, descNum = %d.\n",
                        __func__, queue, portInit.rxDescrNum[queue]));

        if (mvGndRxRefill (queue, portInit.rxDescrNum[queue]) != MV_OK)
        {
            mvOsPrintf ("%s: mvGndHwEnableF failed.\n", __func__);
            return MV_FAIL;
        }
    }

    if (standalone_network_device == MV_TRUE)
    {
        if (hwP->mvGndHwMacAddrSetF (genCtrlP->macAddr, MII_DEF_RXQ) != MV_OK)
        {
            mvOsPrintf ("%s: mvGndEnableHw failed.\n", __func__);
            return MV_FAIL;
        }
    }

    /* Mark the interface as up */
    genCtrlP->started = MV_TRUE;

    if (hwP->mvGndHwEnableF () != MV_OK)
    {
        printf ("%s: mvGndHwEnableF failed.\n", __func__);
        return MV_FAIL;
    }

    return MV_OK;
}

/*******************************************************************************
 * mvGndIntEnable
 */
MV_VOID mvGndIntEnable(MV_BOOL rxIntEnable, MV_BOOL txIntEnable)
{
    GND_CTRL           *genCtrlP = G_genDrvCtrlP;
    MV_GND_HW_FUNCS    *hwP      = genCtrlP->hwP;

    /*
     * Enable Layer 2 interrupts.
     */
    if (rxIntEnable == MV_TRUE)
    {
        hwP->mvGndHwIntAckRxReadyF    (MV_ETH_RX_Q_ALL);
        hwP->mvGndHwIntUnmaskRxReadyF (MV_ETH_RX_Q_ALL);
    }

    if (txIntEnable == MV_TRUE)
    {
        hwP->mvGndHwIntAckTxDoneF     (MV_ETH_TX_Q_ALL);
        hwP->mvGndHwIntUnmaskTxDoneF  (MV_ETH_TX_Q_ALL);
    }
}

/*******************************************************************************
 * mvGndIntDisable
 */
MV_VOID mvGndIntDisable(MV_BOOL rxIntEnable, MV_BOOL txIntEnable)
{
    GND_CTRL           *genCtrlP = G_genDrvCtrlP;
    MV_GND_HW_FUNCS    *hwP      = genCtrlP->hwP;

    /*
     * Disable Layer 2 interrupts.
     */
    if (rxIntEnable == MV_TRUE)
    {
        hwP->mvGndHwIntMaskRxReadyF   (MV_ETH_RX_Q_ALL);
        hwP->mvGndHwIntAckRxReadyF    (MV_ETH_RX_Q_ALL);
    }

    if (txIntEnable == MV_TRUE)
    {
        hwP->mvGndHwIntMaskTxDoneF    (MV_ETH_TX_Q_ALL);
        hwP->mvGndHwIntAckTxDoneF     (MV_ETH_TX_Q_ALL);
    }
}

/*******************************************************************************
 * mvGndBuffSizeGet
 */
MV_U32 mvGndBuffSizeGet()
{
    return genBuffPoolBuffSizeGet (G_genDrvCtrlP->rxBuffPoolP);
}

/*******************************************************************************
 * mvGndRegHwIfDo
 */
MV_STATUS mvGndRegHwIfDo(MV_GND_HW_FUNCS *hwIfP)
{
    GND_CTRL *genCtrlP = G_genDrvCtrlP;
    genCtrlP->hwP = hwIfP;
    return MV_OK;
}

/*******************************************************************************
 * mvGndRegOsIfDo
 */
MV_STATUS mvGndRegOsIfDo(MV_GND_OS_FUNCS *osIfP)
{
    GND_CTRL *genCtrlP = G_genDrvCtrlP;
    genCtrlP->osP = osIfP;
    return MV_OK;
}

/*******************************************************************************
 * mvGndPrint
 */
MV_VOID mvGndPrint(MV_VOID)
{
    GND_CTRL *genCtrlP = G_genDrvCtrlP;
    MV_U8     macAddrDotFormat[MV_MAC_ADDR_SIZE * 2 + 5];
    MV_U32    i;

    if (mvGndIsInited() == MV_FALSE || genCtrlP->started == MV_FALSE)
    {
        return;
    }

    mvOsPrintf("\n");
    mvOsPrintf("Printing Network Generic Driver (GND) Info.\n");
    mvOsPrintf("started               = %d.\n", genCtrlP->started);
    mvOsPrintf("gbeIndex              = %d.\n", genCtrlP->gbeIndex);

    sprintf ((MV_8 *)macAddrDotFormat, "%02x:%02x:%02x:%02x:%02x:%02x",
             genCtrlP->macAddr[0],
             genCtrlP->macAddr[1],
             genCtrlP->macAddr[2],
             genCtrlP->macAddr[3],
             genCtrlP->macAddr[4],
             genCtrlP->macAddr[5]);
    mvOsPrintf("macAddr               = %s.\n", macAddrDotFormat);

    mvOsPrintf("isTxSyncMode          = %d.\n", genCtrlP->isTxSyncMode);
    mvOsPrintf("pollTimoutUSec        = %d.\n", genCtrlP->pollTimoutUSec);
    mvOsPrintf("maxPollTimes          = %d.\n", genCtrlP->maxPollTimes);
    mvOsPrintf("isToDel2PrepBytes     = %d.\n", genCtrlP->isToDel2PrepBytes);
    mvOsPrintf("maxFragsInPkt         = %d.\n", genCtrlP->maxFragsInPkt);
    mvOsPrintf("rxPktInfoPoolP        = 0x%08X.\n",
               (MV_U32)genCtrlP->rxPktInfoPoolP);
    mvOsPrintf("rxBuffPoolP           = 0x%08X.\n",
               (MV_U32)genCtrlP->rxBuffPoolP);

    mvOsPrintf("rxDescNumPerQ[Q:Desc] = ");
    for (i = 0; i < MV_ETH_RX_Q_NUM; i++)
    {
        mvOsPrintf("%d:%d ", i, genCtrlP->rxDescNumPerQ[i]);
    }
    mvOsPrintf("\n");

    mvOsPrintf("txDescNumPerQ[Q:Desc] = ");
    for (i = 0; i < MV_ETH_TX_Q_NUM; i++)
    {
        mvOsPrintf("%d:%d ", i, genCtrlP->txDescNumPerQ[i]);
    }
    mvOsPrintf("\n");

    mvOsPrintf("hwP                   = 0x%08X.\n", (MV_U32)genCtrlP->hwP);
    mvOsPrintf("osP                   = 0x%08X.\n",(MV_U32) genCtrlP->osP);
}

/*******************************************************************************
 * mvGndPrintStat
 */
MV_VOID mvGndPrintStat(MV_VOID)
{
    GND_CTRL *genCtrlP = G_genDrvCtrlP;
    GND_STAT *statP    = &genCtrlP->stat;

    if (mvGndIsInited() == MV_FALSE)
    {
        mvOsPrintf("%s: GND is not intialized.\n", __func__);
        return;
    }

    mvOsPrintf("rxReadyIntCnt           = %d.\n", statP->rxReadyIntCnt);
    mvOsPrintf("txDoneIntCnt            = %d.\n", statP->txDoneIntCnt);
}

