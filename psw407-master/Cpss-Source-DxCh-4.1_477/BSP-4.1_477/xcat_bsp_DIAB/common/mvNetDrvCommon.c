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

#include "mvNetDrvCommon.h"
#include "mvSysHwConfig.h"
#include "mvGndReg.h"
#include "mvGnd.h"
#include "mvGndOsIf.h"
#include "mvGndHwIf.h"
#include "mvGenSyncPool.h"
#include "mvPrestera.h"
#include "mvHookReg.h"
#include "mvCommon.h"
#include "mvBoardEnvLib.h"

#if 0
#define DB(x) x
#else
#define DB(x)
#endif

typedef struct _mvSwGenStat
{
    MV_U32           rxReadyIntCnt;
    MV_U32           txDoneIntCnt;
} MV_SW_GEN_STAT;

/*
 * Main internal control structure.
 */
typedef struct _mvNetGenCtrlS
{
    MV_U32            dev;
    MV_BOOL           isMiiMode;
    MV_BOOL           isTxSyncMode;
    MV_U32            rxPathSemId;
    MV_U32            txPathSemId;

    MV_BOOL           attached;   /* Interface has been attached    */
    MV_BOOL           started;    /* Interface has been started     */
    MV_BOOL           isUp;       /* Link is UP                     */

    GEN_SYNC_POOL    *rxPktInfoPoolP;
    GEN_SYNC_POOL    *txPktInfoPoolP;

    MV_VOID          *txDoneHookRegHandleF;
    MV_VOID          *hdrAltBeforeTxHandleF;
    MV_VOID          *fwdRxPktRegHandleF;
    MV_VOID          *hdrAltBeforeFwdRxPktHandleF;
    MV_VOID          *hdrAltBeforeFwdRxFreePktHandleF;
    MV_VOID          *hdrAltBeforeFwdTxDonePktHandleF;

    MV_SW_GEN_STAT    stat;

    MV_GND_HW_FUNCS  *hwIfP;

    MV_SWITCH_GEN_HOOK_CALC_PKT_ID       calcFwdRxPktHookIdF;

    MV_SWITCH_GEN_HW_INIT                hwInit;

} NET_GEN_CTRL;

/*
 * Struct needed to init Generic Network Driver (GND).
 */
static MV_GND_INIT G_gndInit;
static MV_U32      G_rxDescNumPerQ[MV_NET_NUM_OF_RX_Q] = {0};
static MV_U32      G_txDescNumPerQ[MV_NET_NUM_OF_TX_Q] = {0};
static MV_U32      G_maxFragsInPkt = 1;

/*******************************************************************************
 * Globals
 */
MV_U8     G_switchDefaultMacAddr[MV_MAC_ADDR_SIZE] =
                            {0x00, 0x45, 0x78, 0x14, 0x59, 0x00};

NET_GEN_CTRL      G_switchGenCtrl;
NET_GEN_CTRL     *G_switchGenCtrlP = &G_switchGenCtrl;

MV_BOOL G_isSwitchGenInited = MV_FALSE;

/*******************************************************************************
 * Declarations
 */
MV_STATUS switchGenRxJob           (void);
MV_STATUS switchGenTxJob           (void);
MV_VOID   switchGenRxReadyIsr      (void);
MV_VOID   switchGenTxDoneIsr       (void);

MV_VOID   switchRxReadyIsrCb       (void);
MV_VOID   switchTxDoneIsrCb        (void);
MV_VOID   switchEndRxPathLock      (void);
MV_VOID   switchEndRxPathUnlock    (void);
MV_VOID   switchEndTxPathLock      (void);
MV_VOID   switchEndTxPathUnlock    (void);
MV_STATUS switchGenUserTxDone      (MV_GND_PKT_INFO *pktInfoP);
MV_STATUS switchGenFwdRxPktToOs    (MV_GND_PKT_INFO *pktInfoP, MV_U32 rxQ);
MV_STATUS switchDrvOsPktFree       (MV_VOID *osPkt);

MV_STATUS switchDrvIntInitMiiRx    (void (*rxReadyIsrF)(void));
MV_STATUS switchDrvIntInitMiiTx    (void (*txReadyIsrF)(void));
MV_STATUS switchDrvIntInitSdmaRx   (void (*rxReadyIsrF)(void), MV_U32 dev);
MV_STATUS switchDrvIntInitSdmaTx   (void (*txReadyIsrF)(void), MV_U32 dev);
MV_STATUS switchDrvIntEnableMiiRx  (void);
MV_STATUS switchDrvIntEnableMiiTx  (void);
MV_STATUS switchDrvIntEnableSdmaRx (MV_U32 dev);
MV_STATUS switchDrvIntEnableSdmaTx (MV_U32 dev);

MV_GND_OS_FUNCS switchGndOsIf =
{
    /* .mvGndOsIsrCbRxReadyF       = */ switchRxReadyIsrCb,
    /* .mvGndOsIsrCbTxDoneF        = */ switchTxDoneIsrCb,
    /* .mvGndOsRxPathLockF         = */ switchEndRxPathLock,
    /* .mvGndOsRxPathUnlockF       = */ switchEndRxPathUnlock,
    /* .mvGndOsTxPathLockF         = */ switchEndTxPathLock,
    /* .mvGndOsTxPathUnlockF       = */ switchEndTxPathUnlock,
    /* .mvGndOsUserTxDoneF         = */ switchGenUserTxDone,
    /* .mvGndOsFwdRxPktF           = */ switchGenFwdRxPktToOs
};

/*******************************************************************************
 * switchDrvGenIsInited
 */
MV_STATUS switchDrvGenIsInited(void)
{
    return G_isSwitchGenInited;
}

/*******************************************************************************
 * switchGenIsLinkUp
 */
MV_BOOL switchGenIsLinkUp(void)
{
    NET_GEN_CTRL      *genCtrlP = G_switchGenCtrlP;

    return genCtrlP->isUp;
}

/*******************************************************************************
 * switchGenTxPktInfoGet
 */
MV_GND_PKT_INFO *switchGenTxPktInfoGet(void)
{
    NET_GEN_CTRL     *genCtrlP = G_switchGenCtrlP;
    MV_GND_PKT_INFO  *pktInfoP;

    pktInfoP = (MV_GND_PKT_INFO *)genSyncPoolGet(genCtrlP->txPktInfoPoolP);
    if (pktInfoP == NULL)
    {
        DB(mvOsPrintf("%s: genSyncPoolGet failed.\n", __func__));
        return NULL;
    }

    return pktInfoP;
}

/*******************************************************************************
 * switchGenIntLvlGet
 */
MV_U32 switchGenIntLvlGet(MV_U32 dev)
{
    MV_U32 intLvl = -1; /* invalid interrupt level */

    if (dev == PP_DEV0)
    {
        if (mvPpChipIsXCat2() == MV_TRUE)
        {
            intLvl = INT_LVL_XCAT2_SWITCH;
        }
        else
        {
            intLvl = INT_LVL_SWITCH_GPP_INT;
        }
    }
    else if (dev == PP_DEV1)
    {
        intLvl = INT_LVL_PEX0_INTA;
    }

    return intLvl;
}

/*******************************************************************************
 * switchGenFwdRxPktToOs
 */
MV_STATUS switchGenFwdRxPktToOs(MV_GND_PKT_INFO *pktInfoP, MV_U32 rxQ)
{
    NET_GEN_CTRL      *genCtrlP  = G_switchGenCtrlP;
    MV_U32             fwdHookId;
    MV_SWITCH_GEN_HOOK_HDR_ALT_RX    hdrAltHookF;
    MV_SWITCH_GEN_HOOK_FWD_RX_PKT    fwdHookF;

    mvSwitchRememberExtDsa(pktInfoP);

    if (genCtrlP->calcFwdRxPktHookIdF != NULL)
    {
        fwdHookId = genCtrlP->calcFwdRxPktHookIdF(pktInfoP);

        hdrAltHookF = mvHookRegGet(genCtrlP->hdrAltBeforeFwdRxPktHandleF,
                                                                fwdHookId);
        if (hdrAltHookF != NULL)
        {
            /*
             * Perform packet header alternation if needed.
             */
            if (hdrAltHookF(pktInfoP) != MV_OK)
            {
                mvOsPrintf("%s: hdrAltHookF failed.\n", __func__);
                return MV_FAIL;
            }
        }

        fwdHookF = mvHookRegGet(genCtrlP->fwdRxPktRegHandleF, fwdHookId);
        if (fwdHookF != NULL)
        {
            /*
             * Forward the packet to upper layer (OS, CPSS etc.).
             */
            if (fwdHookF(pktInfoP, rxQ) != MV_OK)
            {
                DB(mvOsPrintf("%s: fwdHookF failed.\n", __func__));
                return MV_FAIL;
            }
        }
    }

    return MV_OK;
}

/*******************************************************************************
 * switchGenUserTxDone
 */
MV_STATUS switchGenUserTxDone(MV_GND_PKT_INFO *pktInfoP)
{
    NET_GEN_CTRL      *genCtrlP  = G_switchGenCtrlP;
    MV_U32             ownerId;
    MV_VOID           *callbackF;
    MV_SWITCH_GEN_HOOK_DRV_OS_PKT_FREE  switchDrvOsPktFreeF;
    MV_SWITCH_GEN_HOOK_HDR_ALT_TX       hdrAltHookF;

    ownerId   = pktInfoP->ownerId;

    /*
     * Perform the header alternation (not mandatory).
     */
    callbackF = mvHookRegGet(genCtrlP->hdrAltBeforeFwdTxDonePktHandleF, ownerId);
    if (callbackF != NULL)
    {
        hdrAltHookF = (MV_SWITCH_GEN_HOOK_HDR_ALT_TX)callbackF;
        if (hdrAltHookF(pktInfoP) != MV_OK)
        {
            mvOsPrintf("%s: hdrAltHookF failed.\n", __func__);
        }
    }

    /*
     * Get TX-done-free callback according to ownerId (mandatory).
     */
    callbackF = mvHookRegGet(genCtrlP->txDoneHookRegHandleF, ownerId);
    if (callbackF == NULL)
    {
        mvOsPrintf("%s: mvHookRegGet failed.\n", __func__);
        return MV_FAIL;
    }
    switchDrvOsPktFreeF = (MV_SWITCH_GEN_HOOK_DRV_OS_PKT_FREE)callbackF;

    /*
     * Free OS-specific packet wrapper.
     */
    if (switchDrvOsPktFreeF((MV_VOID *)pktInfoP->osInfo) != MV_OK)
    {
        mvOsPrintf("%s: switchDrvOsPktFreeF failed.\n", __func__);
        return MV_FAIL;
    }

    /*
     * Free pktInfo to its pool.
     */
    if (genSyncPoolPut(genCtrlP->txPktInfoPoolP, pktInfoP) != MV_OK)
    {
        mvOsPrintf("%s: genSyncPoolPut failed.\n", __func__);
        return MV_FAIL;
    }

    return MV_OK;
}

/*******************************************************************************
 * switchEndPrintActiveIface
 */
MV_VOID switchEndPrintActiveIface(void)
{
    NET_GEN_CTRL      *genCtrlP  = G_switchGenCtrlP;
    MV_BOOL            isMiiMode = genCtrlP->isMiiMode;

    mvOsPrintf("GND: %s iface is used (TxMode = %s)"
               "[#RxQ:#Desc=%d:%d; #TxQ:#Desc=%d:%d].\n",
               (isMiiMode == MV_TRUE) ? "PP-MII" : "PP-SDMA",
               (genCtrlP->isTxSyncMode == MV_TRUE) ? "Sync" : "Async",
               MV_NET_NUM_OF_RX_Q,
               MV_NET_NUM_OF_RX_DESC_TOTAL,
               MV_NET_NUM_OF_TX_Q,
               MV_NET_NUM_OF_TX_DESC_TOTAL);
}

/*******************************************************************************
 * switchDrvGenHookSetCalcFwdRxPktHookId
 *
 *     Note: calculates a key for RX_READY packet.
 */
MV_STATUS switchDrvGenHookSetCalcFwdRxPktHookId(MV_SWITCH_GEN_HOOK_CALC_PKT_ID hookF)
{
    NET_GEN_CTRL     *genCtrlP = G_switchGenCtrlP;

    if (switchDrvGenIsInited() == MV_FALSE)
    {
        mvOsPrintf("%s: switchDrvGen is not inited.\n", __func__);
        return MV_FAIL;
    }

    genCtrlP->calcFwdRxPktHookIdF = hookF;
    return MV_OK;
}

/*******************************************************************************
 * switchDrvGenHookSetFwdRxPkt
 */
MV_STATUS switchDrvGenHookSetFwdRxPkt(MV_U32 hookId,
                                      MV_SWITCH_GEN_HOOK_FWD_RX_PKT hookF)
{
    NET_GEN_CTRL     *genCtrlP = G_switchGenCtrlP;

    if (switchDrvGenIsInited() == MV_FALSE)
    {
        mvOsPrintf("%s: switchDrvGen is not inited.\n", __func__);
        return MV_FAIL;
    }

    if (mvHookRegPut(genCtrlP->fwdRxPktRegHandleF,
                     hookId, /* e.g. port number */
                     hookF) != MV_OK)
    {
        mvOsPrintf("%s: mvHookRegPut failed.\n", __func__);
        return MV_FAIL;
    }

    return MV_OK;
}

/*******************************************************************************
 * switchDrvGenHookSetHdrAltBeforeFwdRxPkt
 */
MV_STATUS switchDrvGenHookSetHdrAltBeforeFwdRxPkt(MV_U32 hookId,
                                            MV_SWITCH_GEN_HOOK_FWD_RX_PKT hookF)
{
    NET_GEN_CTRL     *genCtrlP = G_switchGenCtrlP;

    if (switchDrvGenIsInited() == MV_FALSE)
    {
        mvOsPrintf("%s: switchDrvGen is not inited.\n", __func__);
        return MV_FAIL;
    }

    if (mvHookRegPut(genCtrlP->hdrAltBeforeFwdRxPktHandleF,
                     hookId, /* e.g. port number */
                     hookF) != MV_OK)
    {
        mvOsPrintf("%s: mvHookRegPut failed.\n", __func__);
        return MV_FAIL;
    }

    return MV_OK;
}

/*******************************************************************************
 * switchDrvGenHookSetTxDone
 */
MV_STATUS switchDrvGenHookSetTxDone(MV_U32 ownerId,
                                    MV_SWITCH_GEN_HOOK_DRV_OS_PKT_FREE hook)
{
    NET_GEN_CTRL     *genCtrlP = G_switchGenCtrlP;

    if (mvHookRegPut(genCtrlP->txDoneHookRegHandleF, ownerId, hook) != MV_OK)
    {
        mvOsPrintf("%s: mvHookRegPut failed.\n", __func__);
        return MV_FAIL;
    }

    return MV_OK;
}

/*******************************************************************************
 * switchGenFillDefaults
 */
MV_STATUS switchGenFillDefaults(void)
{
    NET_GEN_CTRL     *genCtrlP = G_switchGenCtrlP;
    MV_GND_HW_FUNCS  *hwIfP            = NULL;
    MV_U32            rxBuffsBulkSize;
    MV_U8            *rxBuffsBulkP   = NULL;
    MV_U32            rxBuffSize;
    MV_U32            rxBuffHdrOffset;
    MV_U32            queue;
    MV_U32            totalBuffNum   = 0;
    MV_U32            txDescNumTotal = 0;

    mvOsMemset(&G_gndInit, 0, sizeof(MV_GND_INIT));

    /*
     * Register upper layer interface.
     */
    G_gndInit.osP = &switchGndOsIf;

    /*
     * Set max number of RX/TX fragments in packet.
     */
    G_gndInit.maxFragsInPkt = G_maxFragsInPkt;

    /*
     * Register hardware interface.
     */
    if (genCtrlP->isMiiMode == MV_TRUE)
    {
        hwIfP = mvGndHwIfGet(MV_GND_HW_IF_MII);
    }
    else
    {
        hwIfP = mvGndHwIfGet(MV_GND_HW_IF_SDMA);
    }

    if (hwIfP == NULL)
    {
        mvOsPrintf("%s: mvGndHwIfGet failed.\n", __func__);
        return MV_FAIL;
    }
    G_gndInit.hwP = hwIfP;

    /*
     * Register upper layer interface.
     */
    G_gndInit.osP = &switchGndOsIf;

    /*
     * Set max number of RX/TX fragments in packet.
     */
    G_gndInit.maxFragsInPkt = G_maxFragsInPkt;

    /*
     * RX init
     */
    mvEthRandomizeMac(G_switchDefaultMacAddr);
    G_gndInit.rx.mruBytes = 1522 + 8 /* DSA tag */;

    totalBuffNum = 0;
    for (queue = 0; queue < MV_ETH_RX_Q_NUM; queue++)
    {
        G_rxDescNumPerQ[queue]  = PP_NUM_OF_RX_DESC_PER_Q;
        totalBuffNum           += PP_NUM_OF_RX_DESC_PER_Q;
    }

    rxBuffSize      = MII_RX_BUFF_SIZE_DEFAULT;
    rxBuffHdrOffset = 8;
    rxBuffsBulkSize = (rxBuffSize + rxBuffHdrOffset) * totalBuffNum;
    rxBuffsBulkP    = (MV_U8 *)mvOsCalloc(1, rxBuffsBulkSize);
    if (rxBuffsBulkP == NULL)
    {
        mvOsPrintf("%s: mvOsMalloc failed.\n", __func__);
        return MV_FAIL;
    }

    G_gndInit.rx.rxBuffsBulkSize   = rxBuffsBulkSize;
    G_gndInit.rx.rxBuffsBulkP      = rxBuffsBulkP;
    G_gndInit.rx.rxBuffSize        = rxBuffSize;
    G_gndInit.rx.actualRxBuffsNumP = NULL;
    G_gndInit.rx.rxBuffHdrOffset   = rxBuffHdrOffset;
    G_gndInit.rx.rxDescNumPerQ     = G_rxDescNumPerQ;
    G_gndInit.rx.rxDescNumTotal    = totalBuffNum;
    G_gndInit.rx.numOfRxQueues     = MV_ETH_RX_Q_NUM;
    G_gndInit.rx.isToDel2PrepBytes = MV_TRUE; /* for MII and SDMA both */
    mvOsMemcpy(G_gndInit.rx.macAddr, G_switchDefaultMacAddr, MV_MAC_ADDR_SIZE);

    /*
     * TX init
     */
    txDescNumTotal = 0;
    for (queue = 0; queue < MV_ETH_TX_Q_NUM; queue++)
    {
        G_txDescNumPerQ[queue] = PP_NUM_OF_TX_DESC_PER_Q;
        txDescNumTotal        += PP_NUM_OF_TX_DESC_PER_Q;
    }

    G_gndInit.tx.txDescNumPerQ     = G_txDescNumPerQ;
    G_gndInit.tx.txDescNumTotal    = txDescNumTotal;
    G_gndInit.tx.numOfTxQueues     = MV_ETH_TX_Q_NUM;
    G_gndInit.tx.isTxSyncMode      = genCtrlP->isTxSyncMode;
    G_gndInit.tx.pollTimoutUSec    = MV_GND_POLL_TIMOUT_USEC_DEFAULT;
    G_gndInit.tx.maxPollTimes      = MV_GND_POLL_COUNT_MAX_DEFAULT;

    return MV_OK;
}

/*******************************************************************************
 * switchGenCfgCascadePorts
 *     This is the default configuration.
 */
MV_STATUS switchGenCfgCascadePorts(MV_VOID)
{
    /* Force Link Down on port 25 dev 1. */
    if (mvPpDevNumGet() > 1)
    {
        mvPpBitReset(PP_DEV1, 0xA80640C, BIT_1);
        mvPpBitSet  (PP_DEV1, 0xA80640C, BIT_0);
    }
    mvPpCascadePortCfg();
    mvSwitchVidxCfg();
    mvSwitchEgressFilterCfg();
    switchEndPrintActiveIface();

    return MV_OK;
}

/*******************************************************************************
 * switchDrvGenInitHw
 */
MV_STATUS switchDrvGenInitHw(MV_SWITCH_GEN_HW_INIT *hwInitP)
{
    NET_GEN_CTRL     *ctrlP = G_switchGenCtrlP;
    MV_SWITCH_GEN_HOOK_HW_INIT_PORTS fP;

    if (hwInitP == NULL)
    {
        /* Nothing to do. */
        return MV_OK;
    }

    if (ctrlP->hwInit.cfgCascadePorts != NULL)
    {
        if (ctrlP->hwInit.cfgCascadePorts() != MV_OK)
        {
            mvOsPrintf("%s: hwInit.cfgCascadePorts() failed.\n", __func__);
            return MV_FAIL;
        }
    }

    fP = mvBoardSwitchPortsInitFuncGet();

    mvOsPrintf("Initializing switch ports...");

    if (fP != NULL)
    {
        if (fP() != MV_OK)
        {
            mvOsPrintf("PP-EEPROM simulation failed.\n");
            return 1; /* failure */
        }
        mvOsPrintf("Done.\n");
    }
    else
    {
        mvOsPrintf("PP-EEPROM simulation failed (no function found).\n");
        return 1; /* failure */
    }

    return MV_OK;
}

/*******************************************************************************
 * switchDrvGenInitSw
 */
MV_STATUS switchDrvGenInitSw(MV_SWITCH_GEN_INIT *initP)
{
    NET_GEN_CTRL     *genCtrlP = G_switchGenCtrlP;
    GEN_SYNC_POOL    *poolP;
    MV_U32            maxFragsInPkt  = G_maxFragsInPkt;
    MV_GND_INIT      *gndInitP       = NULL;

    genCtrlP->isMiiMode    = initP->isMiiMode;
    genCtrlP->isTxSyncMode = initP->isTxSyncMode;

    if (initP->gndInitP != NULL)
    {
        mvOsMemcpy(&G_gndInit, initP->gndInitP, sizeof(MV_GND_INIT));
        G_gndInit.osP->mvGndOsFwdRxPktF      = switchGenFwdRxPktToOs;
        G_gndInit.osP->mvGndOsUserTxDoneF    = switchGenUserTxDone;
    }
    else
    {
        if (switchGenFillDefaults() != MV_OK)
        {
            mvOsPrintf("%s: switchGenFillDefaults failed.\n", __func__);
            return MV_FAIL;
        }
    }

    gndInitP = &G_gndInit;

    poolP = mvGndTxPktInfoPoolCreate(gndInitP->tx.txDescNumTotal, maxFragsInPkt);
    if (poolP == NULL)
    {
        mvOsPrintf("%s: mvGndTxPktInfoPoolCreate failed.\n", __func__);
        return MV_FAIL;
    }
    genCtrlP->txPktInfoPoolP = poolP;

    /*
     * Init and configure Generic Network Driver.
     */
    if (mvGndInitSw(&G_gndInit) != MV_OK)
    {
        mvOsPrintf("%s: mvGndInitSw failed.\n", __func__);
        return MV_FAIL;
    }

    return MV_OK;
}

/*******************************************************************************
 * switchDrvGenInit
 */
MV_STATUS switchDrvGenInit(MV_SWITCH_GEN_INIT *initP)
{
    NET_GEN_CTRL     *genCtrlP = G_switchGenCtrlP;
    GEN_SYNC_POOL    *poolP;
    MV_U32            maxFragsInPkt  = G_maxFragsInPkt;
    MV_GND_INIT      *gndInitP       = NULL;

    if (initP == NULL)
    {
        mvOsPrintf("%s: Wrong param (NULL pointer).\n", __func__);
        return MV_FAIL;
    }

    /*
     * SwitchGen component may be inited only once.
     */
    if (switchDrvGenIsInited() == MV_TRUE)
    {
        return MV_OK;
    }
    G_isSwitchGenInited = MV_TRUE;

    mvOsMemset(genCtrlP, 0, sizeof(NET_GEN_CTRL));
    genCtrlP->isMiiMode    = initP->isMiiMode;
    genCtrlP->isTxSyncMode = initP->isTxSyncMode;

    if (initP->gndInitP != NULL)
    {
        mvOsMemcpy(&G_gndInit, initP->gndInitP, sizeof(MV_GND_INIT));
        G_gndInit.osP->mvGndOsFwdRxPktF      = switchGenFwdRxPktToOs;
        G_gndInit.osP->mvGndOsUserTxDoneF    = switchGenUserTxDone;
    }
    else
    {
        if (switchGenFillDefaults() != MV_OK)
        {
            mvOsPrintf("%s: switchGenFillDefaults failed.\n", __func__);
            return MV_FAIL;
        }
        G_gndInit.gbeIndex = initP->gbeDefaultIndex;
    }

    gndInitP = &G_gndInit;

    poolP = mvGndTxPktInfoPoolCreate(gndInitP->tx.txDescNumTotal, maxFragsInPkt);
    if (poolP == NULL)
    {
        mvOsPrintf("%s: mvGndTxPktInfoPoolCreate failed.\n", __func__);
        return MV_FAIL;
    }
    genCtrlP->txPktInfoPoolP = poolP;

    /*
     * Init and configure Generic Network Driver (GND).
     */
    if (mvGndInitSw(&G_gndInit) != MV_OK)
    {
        mvOsPrintf("%s: mvGndInitSw failed.\n", __func__);
        return MV_FAIL;
    }

    /*
     * Set switchGen operational state.
     */
#ifndef PSS_MODE
    switchGenCfgCascadePorts();
#endif

    genCtrlP->attached = MV_TRUE;
    genCtrlP->started  = MV_FALSE;
    genCtrlP->isUp     = MV_FALSE;

    return MV_OK;
}

/*******************************************************************************
 * switchGenHooksInitRx
 */
MV_STATUS switchGenHooksInitRx(MV_U32 numOfHooks)
{
    NET_GEN_CTRL     *genCtrlP = G_switchGenCtrlP;
    MV_VOID          *fwdRxPktRegHandleF;
    MV_VOID          *hdrAltBeforeFwdRxPktHandleF;
    MV_VOID          *hdrAltBeforeFwdRxFreePktHandleF;
    static MV_BOOL    isFuncAlreadyRun = MV_FALSE;

    if (isFuncAlreadyRun == MV_TRUE)
    {
        return MV_OK;
    }
    isFuncAlreadyRun = MV_TRUE;

    /*
     * Init and fill hook registry for RX packet forward operation.
     */
    fwdRxPktRegHandleF = mvHookRegInit(numOfHooks);
    if (fwdRxPktRegHandleF == NULL)
    {
        mvOsPrintf("%s: mvHookRegInit failed.\n", __func__);
        return MV_FAIL;
    }
    genCtrlP->fwdRxPktRegHandleF = fwdRxPktRegHandleF;

    /*
     * Init and fill hook registry for RX header alternation operation (optional).
     */
    hdrAltBeforeFwdRxPktHandleF = mvHookRegInit(numOfHooks);
    if (hdrAltBeforeFwdRxPktHandleF == NULL)
    {
        mvOsPrintf("%s: mvHookRegInit failed.\n", __func__);
        return MV_FAIL;
    }
    genCtrlP->hdrAltBeforeFwdRxPktHandleF = hdrAltBeforeFwdRxPktHandleF;

    /*
     * Init and fill hook registry for RX header alternation operation
     * when freeing RX packet back to hardware queue (optional).
     */
    hdrAltBeforeFwdRxFreePktHandleF = mvHookRegInit(numOfHooks);
    if (hdrAltBeforeFwdRxFreePktHandleF == NULL)
    {
        mvOsPrintf("%s: mvHookRegInit failed.\n", __func__);
        return MV_FAIL;
    }
    genCtrlP->hdrAltBeforeFwdRxFreePktHandleF = hdrAltBeforeFwdRxFreePktHandleF;

    return MV_OK;
}

/*******************************************************************************
 * switchGenHooksInitTx
 */
MV_STATUS switchGenHooksInitTx(MV_U32 numOfHooks)
{
    NET_GEN_CTRL     *genCtrlP = G_switchGenCtrlP;
    MV_VOID          *txDoneHookRegHandleF;
    MV_VOID          *hdrAltBeforeFwdTxDonePktHandleF;
    MV_VOID          *hdrAltBeforeTxHandleF;
    static MV_BOOL    isFuncAlreadyRun = MV_FALSE;

    if (isFuncAlreadyRun == MV_TRUE)
    {
        return MV_OK;
    }
    isFuncAlreadyRun = MV_TRUE;

    /*
     * TX_DONE hook registry for header alternation (optional).
     */
    hdrAltBeforeFwdTxDonePktHandleF = mvHookRegInit(numOfHooks);
    if (hdrAltBeforeFwdTxDonePktHandleF == NULL)
    {
        mvOsPrintf("%s: mvHookRegInit failed.\n", __func__);
        return MV_FAIL;
    }
    genCtrlP->hdrAltBeforeFwdTxDonePktHandleF = hdrAltBeforeFwdTxDonePktHandleF;

    /*
     * TX_DONE hook registry for OS-specific callback (mandatory).
     */
    txDoneHookRegHandleF = mvHookRegInit(numOfHooks);
    if (txDoneHookRegHandleF == NULL)
    {
        mvOsPrintf("%s: mvHookRegInit failed.\n", __func__);
        return MV_FAIL;
    }
    genCtrlP->txDoneHookRegHandleF = txDoneHookRegHandleF;

    /*
     * TX_DONE hook registry for header alternation before TX (mandatory).
     */
    hdrAltBeforeTxHandleF = mvHookRegInit(numOfHooks);
    if (hdrAltBeforeTxHandleF == NULL)
    {
        mvOsPrintf("%s: mvHookRegInit failed.\n", __func__);
        return MV_FAIL;
    }
    genCtrlP->hdrAltBeforeTxHandleF = hdrAltBeforeTxHandleF;

    return MV_OK;
}

/*******************************************************************************
 * switchGenHooksFillRx
 */
MV_STATUS switchGenHooksFillRx(MV_SW_GEN_RX_HOOKS *rxHooksP, MV_U32 hookId)
{
    NET_GEN_CTRL     *genCtrlP = G_switchGenCtrlP;

    if (genCtrlP->fwdRxPktRegHandleF != NULL)
    {
        if (mvHookRegPut(genCtrlP->fwdRxPktRegHandleF,
                         hookId, /* e.g. port number */
                         rxHooksP->fwdRxReadyPktHookF) != MV_OK)
        {
            mvOsPrintf("%s: mvHookRegPut failed.\n", __func__);
            return MV_FAIL;
        }
    }

    if (genCtrlP->hdrAltBeforeFwdRxPktHandleF != NULL)
    {
        if (mvHookRegPut(genCtrlP->hdrAltBeforeFwdRxPktHandleF,
                         hookId,
                         rxHooksP->hdrAltBeforeFwdRxPktHookF) != MV_OK)
        {
            mvOsPrintf("%s: mvHookRegPut failed.\n", __func__);
            return MV_FAIL;
        }
    }

    if (genCtrlP->hdrAltBeforeFwdRxFreePktHandleF != NULL)
    {
        if (mvHookRegPut(genCtrlP->hdrAltBeforeFwdRxFreePktHandleF,
                         hookId,
                         rxHooksP->hdrAltBeforeFwdRxFreePktHookF) != MV_OK)
        {
            mvOsPrintf("%s: mvHookRegPut failed.\n", __func__);
            return MV_FAIL;
        }
    }

    return MV_OK;
}

/*******************************************************************************
 * switchGenHooksFillTx
 */
MV_STATUS switchGenHooksFillTx(MV_SW_GEN_TX_HOOKS *txHooksP, MV_U32 hookId)
{
    NET_GEN_CTRL     *genCtrlP = G_switchGenCtrlP;

    /*
     * TX_DONE is mandatory hook.
     */
    if (genCtrlP->txDoneHookRegHandleF != NULL)
    {
        if (mvHookRegPut(genCtrlP->txDoneHookRegHandleF,
                         hookId,
                         txHooksP->txDonePktFreeF) != MV_OK)
        {
            mvOsPrintf("%s: mvHookRegPut failed.\n", __func__);
            return MV_FAIL;
        }
    }

    /*
     * TX_DONE header alternation for TX_DONE packet (optional).
     */
    if (genCtrlP->hdrAltBeforeFwdTxDonePktHandleF != NULL)
    {
        if (mvHookRegPut(genCtrlP->hdrAltBeforeFwdTxDonePktHandleF,
                         hookId,
                         txHooksP->hdrAltBeforeFwdTxDonePktHookF) != MV_OK)
        {
            mvOsPrintf("%s: mvHookRegPut failed.\n", __func__);
            return MV_FAIL;
        }
    }

    /*
     * TX_DONE header alternation before TX (mandatory).
     */
    if (genCtrlP->hdrAltBeforeTxHandleF != NULL)
    {
        if (mvHookRegPut(genCtrlP->hdrAltBeforeTxHandleF,
                         hookId,
                         txHooksP->hdrAltBeforeTxF) != MV_OK)
        {
            mvOsPrintf("%s: mvHookRegPut failed.\n", __func__);
            return MV_FAIL;
        }
    }

    return MV_OK;
}

/*******************************************************************************
 * switchDrvGenUnload
 */
MV_STATUS switchDrvGenUnload(void)
{
    NET_GEN_CTRL      *genCtrlP = G_switchGenCtrlP;

    genCtrlP->started = MV_FALSE;
    genCtrlP->isUp    = MV_FALSE;

    return MV_OK;
}

/*******************************************************************************
 * switchGenRxReadyIsr
 */
MV_VOID switchGenRxReadyIsr(void)
{
    NET_GEN_CTRL      *genCtrlP = G_switchGenCtrlP;

    genCtrlP->stat.rxReadyIntCnt++;
    mvGndRxReadyIsr();
}

/*******************************************************************************
 * switchGenTxDoneIsr
 */
MV_VOID switchGenTxDoneIsr(void)
{
    NET_GEN_CTRL      *genCtrlP = G_switchGenCtrlP;

    genCtrlP->stat.txDoneIntCnt++;
    mvGndTxDoneIsr();
}

/*******************************************************************************
 * switchGenSdmaIfaceIsr
 */
MV_VOID switchGenSdmaIfaceIsr(void)
{
    NET_GEN_CTRL      *genCtrlP = G_switchGenCtrlP;

    if (mvPpIsRxReadyIntPending() == MV_TRUE)
    {
        genCtrlP->stat.rxReadyIntCnt++;
        mvGndRxReadyIsr();
    }

    if (mvPpIsTxDoneIntPending() == MV_TRUE)
    {
        genCtrlP->stat.txDoneIntCnt++;
        mvGndTxDoneIsr();
    }
}

/*******************************************************************************
 * switchGenRxJob
 */
MV_STATUS switchGenRxJob(void)
{
    if (mvGndRxBottomHalf() != MV_OK)
    {
        DB(mvOsPrintf("%s: mvGndRxBottomHalf failed.\n", __func__));
        return MV_FAIL;
    }

    return MV_OK;
}

/*******************************************************************************
 * switchGenTxJob
 */
MV_STATUS switchGenTxJob(void)
{
    if (mvGndTxBottomHalf() != MV_OK)
    {
        mvOsPrintf("%s: mvGndTxBottomHalf failed.\n", __func__);
        return MV_FAIL;
    }

    return MV_OK;
}

/*******************************************************************************
 * mvGndSendPkt
 */
MV_STATUS switchGenSendPkt(MV_GND_PKT_INFO *pktInfoP, MV_U32 txQ)
{
    NET_GEN_CTRL      *genCtrlP = G_switchGenCtrlP;
    MV_VOID           *callbackF;
    MV_SWITCH_GEN_HOOK_HDR_ALT_TX hdrAltF;

    /*
     * Header alternation before TX (optional).
     * Assumes that buffer has pre-allocated header of at least 8 bytes.
     */
    callbackF = mvHookRegGet(genCtrlP->hdrAltBeforeTxHandleF, pktInfoP->ownerId);
    if (callbackF != NULL)
    {
        hdrAltF = (MV_SWITCH_GEN_HOOK_HDR_ALT_TX)callbackF;
        if (hdrAltF(pktInfoP) != MV_OK)
        {
            mvOsPrintf("%s: hdrAltF failed.\n", __func__);
            return MV_FAIL;
        }
    }

    if (mvGndSendPkt(pktInfoP, txQ) != MV_OK)
    {
        mvOsPrintf("%s: mvGndSendPkt failed.\n", __func__);
        return MV_FAIL;
    }

    return MV_OK;
}

/*******************************************************************************
 * switchGenFreeRxPkt
 */
MV_STATUS switchGenFreeRxPkt(MV_GND_PKT_INFO *pktInfoP, MV_U32 rxQ)
{
    NET_GEN_CTRL      *genCtrlP = G_switchGenCtrlP;
    MV_U32             fwdHookId;
    MV_SWITCH_GEN_HOOK_HDR_ALT_RX_FREE    hdrAltHookF;

    if (genCtrlP->calcFwdRxPktHookIdF != NULL)
    {
        fwdHookId = genCtrlP->calcFwdRxPktHookIdF(pktInfoP);
        hdrAltHookF = mvHookRegGet(genCtrlP->hdrAltBeforeFwdRxFreePktHandleF,
                                                                   fwdHookId);
        if (hdrAltHookF != NULL)
        {
            /*
             * Perform packet header alternation if needed (optional).
             */
            if (hdrAltHookF(pktInfoP) != MV_OK)
            {
                mvOsPrintf("%s: hdrAltHookF failed.\n", __func__);
                return MV_FAIL;
            }
        }
    }
    else
    {
        /*
         * Amend buffer pointer, because of DSA tag was removed previously
         * (optional).
         */
        mvSwitchInjectExtDsa(pktInfoP);
    }

    if (mvGndFreeRxPkt(pktInfoP, rxQ) != MV_OK)
    {
        mvOsPrintf("%s: mvGndFreeRxPkt failed.\n", __func__);
        return MV_FAIL;
    }

    return MV_OK;
}

/*******************************************************************************
 * switchGenIntInit
 */
MV_STATUS switchGenIntInit(MV_VOID (*rxReadyIsrF)(MV_VOID),
                           MV_VOID (*txDoneIsrF) (MV_VOID))
{
    NET_GEN_CTRL     *genCtrlP = G_switchGenCtrlP;
    MV_U32           dev = genCtrlP->dev;

    if (genCtrlP->isMiiMode == MV_TRUE)
    {
        if (rxReadyIsrF != NULL)
        {
            if (switchDrvIntInitMiiRx(rxReadyIsrF) != MV_OK)
            {
                mvOsPrintf("%s: switchDrvIntInitMiiRx failed.\n", __func__);
                return MV_FAIL;
            }
        }

        if (txDoneIsrF != NULL)
        {
            if (switchDrvIntInitMiiTx(txDoneIsrF) != MV_OK)
            {
                mvOsPrintf("%s: switchDrvIntInitMiiTx failed.\n", __func__);
                return MV_FAIL;
            }
        }
    }
    else
    {
        for (dev = 0; dev < 1 /* mvSwitchGetDevicesNum() */; dev++)
        {
            if (rxReadyIsrF != NULL)
            {
                if (switchDrvIntInitSdmaRx(rxReadyIsrF, dev) != MV_OK)
                {
                    mvOsPrintf("%s: switchDrvIntInitSdmaRx failed.\n", __func__);
                    return MV_FAIL;
                }
            }

            if (txDoneIsrF != NULL)
            {
                if (switchDrvIntInitSdmaTx(txDoneIsrF, dev) != MV_OK)
                {
                    mvOsPrintf("%s: switchDrvIntInitSdmaTx failed.\n", __func__);
                    return MV_FAIL;
                }
            }
        }
    }

    return MV_OK;
}

/*******************************************************************************
 * switchGenIntEnable
 */
MV_VOID switchGenIntEnable(MV_BOOL rxIntEnable, MV_BOOL txIntEnable)
{
    NET_GEN_CTRL     *genCtrlP = G_switchGenCtrlP;
    MV_U32           dev = genCtrlP->dev;

    /*
     * Enable Layer 1 interrupts (in xCat internal CPU)
     */
    if (genCtrlP->isMiiMode == MV_TRUE)
    {
        if (rxIntEnable == MV_TRUE)
        {
            if (switchDrvIntEnableMiiRx() != MV_OK)
            {
                mvOsPrintf("%s: switchDrvIntEnableMiiRx failed.\n", __func__);
                return;
            }
        }

        if (txIntEnable == MV_TRUE)
        {
            if (switchDrvIntEnableMiiTx() != MV_OK)
            {
                mvOsPrintf("%s: switchDrvIntEnableMiiTx failed.\n", __func__);
                return;
            }
        }
    }
    else
    {
        for (dev = 0; dev < 1 /* mvSwitchGetDevicesNum() */; dev++)
        {
            if (rxIntEnable == MV_TRUE)
            {
                if (switchDrvIntEnableSdmaRx(dev) != MV_OK)
                {
                    mvOsPrintf("%s: switchDrvIntEnableSdmaRx failed.\n", __func__);
                    return;
                }
            }

            if (txIntEnable == MV_TRUE)
            {
                if (switchDrvIntEnableSdmaTx(dev) != MV_OK)
                {
                    mvOsPrintf("%s: switchDrvIntEnableSdmaTx failed.\n", __func__);
                    return;
                }
            }
        }
    }
}

/*******************************************************************************
 * switchGenIsStarted
 */
MV_BOOL switchGenIsStarted(void)
{
    NET_GEN_CTRL     *genCtrlP = G_switchGenCtrlP;

    return genCtrlP->started;
}

/*******************************************************************************
 * switchGenIsAttached
 */
MV_BOOL switchGenIsAttached(void)
{
    NET_GEN_CTRL     *genCtrlP = G_switchGenCtrlP;

    return genCtrlP->attached;
}

/*******************************************************************************
 * switchGenStart
 */
MV_STATUS switchGenStart(void)
{
    NET_GEN_CTRL     *genCtrlP = G_switchGenCtrlP;

    if (genCtrlP->attached == MV_FALSE)
    {
        mvOsPrintf("Switch dev iface is not loaded.\n");
        return MV_FAIL;
    }

    if (switchDrvGenInitHw(&genCtrlP->hwInit) != MV_OK)
    {
        mvOsPrintf("%s: switchDrvGenInitHw failed.\n", __func__);
        return MV_FAIL;
    }

    /*
     * Enable Generic Network Driver.
     */
    if (mvGndEnable(PP_DRV_DEF_RX_Q,
                    PP_DRV_RX_BUFF_SIZE_DEFAULT,
                    PP_DRV_MRU_DEFAULT) != MV_OK)
    {
        mvOsPrintf("%s: mvGndEnable failed.\n", __func__);
        return MV_FAIL;
    }

    /*
     * Init and Enable interrupts (OS dependent).
     * This should be done after the creation of RX/TX tasks (if any).
     */
    if (genCtrlP->isTxSyncMode == MV_TRUE)
    {
        if (switchGenIntInit(switchGenRxReadyIsr, NULL /* TX ISR */) != MV_OK)
        {
            mvOsPrintf("%s: switchGenIntInit failed.\n", __func__);
            return MV_FAIL;
        }
        switchGenIntEnable(MV_TRUE /* RX */, MV_FALSE /* TX */);
        mvGndIntEnable    (MV_TRUE /* RX */, MV_FALSE /* TX */);
    }
    else
    {
        if (genCtrlP->isMiiMode == MV_TRUE)
        {
            if (switchGenIntInit(switchGenRxReadyIsr,
                                 switchGenTxDoneIsr) != MV_OK)
            {
                mvOsPrintf("%s: switchGenIntInit failed.\n", __func__);
                return MV_FAIL;
            }
        }
        else
        {
            /*
             * PP-SDMA interface is treated differently, because RX and TX
             * interrupt are on the same interrupt bit.
             */
            if (switchGenIntInit(switchGenSdmaIfaceIsr,
                                 switchGenSdmaIfaceIsr) != MV_OK)
            {
                mvOsPrintf("%s: switchGenIntInit failed.\n", __func__);
                return MV_FAIL;
            }
        }
        switchGenIntEnable(MV_TRUE /* RX */, MV_TRUE /* TX */);
        mvGndIntEnable    (MV_TRUE /* RX */, MV_TRUE /* TX */);
    }

    genCtrlP->isUp    = MV_TRUE;
    genCtrlP->started = MV_TRUE;

    return MV_OK;
}

/*******************************************************************************
 * switchGenStop
 */
MV_STATUS switchGenStop(void)
{
    NET_GEN_CTRL     *genCtrlP = G_switchGenCtrlP;

    if (genCtrlP->attached == MV_FALSE)
    {
        mvOsPrintf("%s: Switch dev iface is not attached.\n", __func__);
        return MV_FAIL;
    }

    genCtrlP->started = MV_FALSE;
    genCtrlP->isUp    = MV_FALSE;

    /*
     * Stop interrupts.
     * Call to GND stop here.
     */

    return MV_OK;
}

/*******************************************************************************
 * pktInfoToPktInfo
 */
MV_STATUS pktInfoToPktInfo(MV_GND_PKT_INFO *pktFromP,
                           MV_GND_PKT_INFO *pktToP)
{
    MV_GND_BUF_INFO   *bufFromP;
    MV_GND_BUF_INFO   *bufToP;
    MV_U32             i;

    /*
     * Conversion itself.
     */
    pktToP->pktSize  = pktFromP->pktSize;
    pktToP->numFrags = pktFromP->numFrags;
    bufFromP         = pktFromP->pFrags;
    bufToP           = pktToP->pFrags;

    for (i = 0; i < pktFromP->numFrags; i++)
    {
        bufToP[i].bufPhysAddr = bufFromP[i].bufPhysAddr;
        bufToP[i].bufVirtPtr  = bufFromP[i].bufVirtPtr;
        bufToP[i].dataSize    = bufFromP[i].dataSize;
    }

    /*
     * Prepare cockie for free purposes.
     */
    pktToP->osInfo   = (MV_ULONG)pktFromP;
    pktToP->status   = 0;

    return MV_OK;
}

/*******************************************************************************
 * switchGenWrapBuffs
 */
MV_GND_PKT_INFO *switchGenWrapBuffs(MV_U8         *buffs[],
                                    MV_U32         buffsLen[],
                                    MV_U32         buffNum,
                                    GEN_SYNC_POOL *pktInfoPoolP)
{
    MV_GND_PKT_INFO  *pktInfoP;
    MV_GND_BUF_INFO  *bufInfoP;
    MV_GND_BUF_INFO  *bufInfoFirstP;
    MV_U8            *buffP;
    MV_U32            buffLen, i;

    if (buffNum > G_maxFragsInPkt)
    {
        mvOsPrintf("%s: buffNum(%d) > G_maxFragsInPkt(%d).\n",
                   __func__, buffNum, G_maxFragsInPkt);
        return NULL;
    }

    /*
     * Allocate packet wrapper (analagous to mBlk, skb etc.).
     */
    pktInfoP = (MV_GND_PKT_INFO *)genSyncPoolGet(pktInfoPoolP);
    if (pktInfoP == NULL)
    {
        mvOsPrintf("%s: genSyncPoolGet failed.\n", __func__);
        return NULL;
    }

    /*
     * Fill packet wrapper.
     */
    bufInfoFirstP = pktInfoP->pFrags;
    for (i = 0; i < buffNum; i++)
    {
        bufInfoP = &bufInfoFirstP[i];
        buffP    = buffs[i];
        buffLen  = buffsLen[i];
        mvEthInitBuffInfo(bufInfoP, buffP, buffLen);
        pktInfoP->pktSize += buffLen;
    }
    pktInfoP->numFrags = buffNum;
    pktInfoP->status   = 0;

    return pktInfoP;
}

/*******************************************************************************
 * switchGenWrapBuff
 */
MV_GND_PKT_INFO *switchGenWrapBuff(MV_U8         *buffP,
                                   MV_U32         buffLen,
                                   GEN_SYNC_POOL *pktInfoPoolP)
{
    MV_GND_PKT_INFO  *pktInfoP;
    MV_GND_BUF_INFO  *bufInfoP;

    /*
     * Allocate packet wrapper (analagous to mBlk, skb etc.).
     */
    pktInfoP = (MV_GND_PKT_INFO *)genSyncPoolGet(pktInfoPoolP);
    if (pktInfoP == NULL)
    {
        mvOsPrintf("%s: genSyncPoolGet failed.\n", __func__);
        return NULL;
    }

    /*
     * Fill packet wrapper.
     */
    bufInfoP = pktInfoP->pFrags;
    mvEthInitBuffInfo(bufInfoP, buffP, buffLen);
    pktInfoP->pktSize  = buffLen;
    pktInfoP->numFrags = 1;
    pktInfoP->status   = 0;

    return pktInfoP;
}

/*******************************************************************************
 * switchGenWrapBuffsSend
 *     Use this function when there is no any specific packet payload
 *     wrapper (e.g., VxWorks uses mBlk, Linux uses skb - they are
 *     OS specific paket paylod wrappers).
 */
MV_STATUS switchGenWrapBuffsSend(MV_U8          *buffs[],
                                 MV_U32          buffsLen[],
                                 MV_U32          buffNum,
                                 MV_U32          txQ,
                                 GEN_SYNC_POOL  *pktInfoPoolP,
                                 MV_U32          ownerId)
{
    MV_GND_PKT_INFO *pktInfoP;
    MV_GND_PKT_INFO *pktWrapperP;

    /*
     * Alloc and fill packet wrapper (analagous to mBlk, skb etc.).
     */
    pktWrapperP = switchGenWrapBuffs(buffs,
                                     buffsLen,
                                     buffNum,
                                     pktInfoPoolP);
    if (pktWrapperP == NULL)
    {
        mvOsPrintf("%s: switchGenWrapBuffs failed.\n", __func__);
        return MV_FAIL;
    }

    /*
     * Alloc pktInfo for TX purposes.
     */
    pktInfoP = switchGenTxPktInfoGet();
    if (pktInfoP == NULL)
    {
        mvOsPrintf("%s: switchGenTxPktInfoGet failed.\n", __func__);
        return MV_FAIL;
    }

    /*
     * Prepare pktInfo to be freed in TX_DONE operation.
     */
    if (pktInfoToPktInfo(pktWrapperP /* from */, pktInfoP /* to */) != MV_OK)
    {
        mvOsPrintf("%s: pktInfoToPktInfo failed.\n", __func__);
        return MV_FAIL;
    }
    pktInfoP->ownerId = ownerId;

    /*
     * Actual send.
     */
    if (switchGenSendPkt(pktInfoP, txQ) != MV_OK)
    {
        mvOsPrintf("%s: switchGenSendPkt failed.\n", __func__);
        return MV_FAIL;
    }

    return MV_OK;
}
/*******************************************************************************
 * switchGenWrapBuffSend
 *     Use this function when there is no any specific packet payload
 *     wrapper (e.g., VxWorks uses mBlk, Linux uses skb - they are
 *     OS specific paket paylod wrappers).
 */
MV_STATUS switchGenWrapBuffSend(MV_U8          *buffP,
                                MV_U32          buffLen,
                                MV_U32          txQ,
                                GEN_SYNC_POOL  *pktInfoPoolP,
                                MV_U32          ownerId)
{
    MV_GND_PKT_INFO *pktInfoP;
    MV_GND_PKT_INFO *pktWrapperP;

    /*
     * Alloc and fill packet wrapper (analagous to mBlk, skb etc.).
     */
    pktWrapperP = switchGenWrapBuff(buffP,
                                    buffLen,
                                    pktInfoPoolP);
    if (pktWrapperP == NULL)
    {
        mvOsPrintf("%s: switchGenWrapBuffs failed.\n", __func__);
        return MV_FAIL;
    }

    /*
     * Alloc pktInfo for TX purposes.
     */
    pktInfoP = switchGenTxPktInfoGet();
    if (pktInfoP == NULL)
    {
        mvOsPrintf("%s: switchGenTxPktInfoGet failed.\n", __func__);
        return MV_FAIL;
    }

    /*
     * Prepare pktInfo to be freed in TX_DONE operation.
     */
    if (pktInfoToPktInfo(pktWrapperP /* from */, pktInfoP /* to */) != MV_OK)
    {
        mvOsPrintf("%s: pktInfoToPktInfo failed.\n", __func__);
        return MV_FAIL;
    }
    pktInfoP->ownerId = ownerId;

    /*
     * Actual send.
     */
    if (switchGenSendPkt(pktInfoP, txQ) != MV_OK)
    {
        mvOsPrintf("%s: switchGenSendPkt failed.\n", __func__);
        return MV_FAIL;
    }

    return MV_OK;
}

/*******************************************************************************
 * switchGenGetRxPkt
 */
MV_GND_PKT_INFO *switchGenGetRxPkt(MV_U32 rxQ)
{
    NET_GEN_CTRL      *genCtrlP  = G_switchGenCtrlP;
    MV_GND_PKT_INFO   *pktInfoP;
    MV_U32             fwdHookId;
    MV_SWITCH_GEN_HOOK_HDR_ALT_RX    hdrAltHookF;

    pktInfoP = mvGndGetRxPkt(rxQ);
    if (pktInfoP == NULL)
    {
        /* Nothing to receive - RX Queue is empty. */
        return NULL;
    }

    mvSwitchRememberExtDsa(pktInfoP);

    if (genCtrlP->calcFwdRxPktHookIdF != NULL)
    {
        fwdHookId = genCtrlP->calcFwdRxPktHookIdF(pktInfoP);

        hdrAltHookF = mvHookRegGet(genCtrlP->hdrAltBeforeFwdRxPktHandleF,
                                                                fwdHookId);
        if (hdrAltHookF != NULL)
        {
            /*
             * Perform packet header alternation if needed.
             */
            if (hdrAltHookF(pktInfoP) != MV_OK)
            {
                mvOsPrintf("%s: hdrAltHookF failed.\n", __func__);
                return NULL;
            }
        }

        return pktInfoP;
    }

    return NULL;
}

/*******************************************************************************
 * switchGenPrint
 */
void switchGenPrint(void)
{
    NET_GEN_CTRL      *genCtrlP  = G_switchGenCtrlP;

    if (switchDrvGenIsInited() == MV_FALSE)
    {
        return;
    }

    mvOsPrintf("\n");
    mvOsPrintf("Printing SwitchGen Info.\n");

    mvOsPrintf("isMiiMode             = %d.\n", genCtrlP->isMiiMode);
    mvOsPrintf("attached              = %d.\n", genCtrlP->attached);
    mvOsPrintf("started               = %d.\n", genCtrlP->started);
    mvOsPrintf("isUp                  = %d.\n", genCtrlP->isUp);
}

/*******************************************************************************
 * switchGenPrintStat
 */
MV_VOID switchGenPrintStat(MV_VOID)
{
    NET_GEN_CTRL      *genCtrlP  = G_switchGenCtrlP;
    MV_SW_GEN_STAT    *statP     = &genCtrlP->stat;

    if (switchDrvGenIsInited() == MV_FALSE)
    {
        return;
    }

    mvOsPrintf("rxReadyIntCnt           = %d.\n", statP->rxReadyIntCnt);
    mvOsPrintf("txDoneIntCnt            = %d.\n", statP->txDoneIntCnt);
}

