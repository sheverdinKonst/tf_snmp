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

#ifndef __INCmvNetDrvCommonh
#define __INCmvNetDrvCommonh

#include "mvTypes.h"
#include "mvGnd.h"

typedef MV_STATUS (*MV_SWITCH_GEN_HOOK_DRV_OS_PKT_FREE)(MV_VOID *osPkt);

typedef MV_STATUS (*MV_SWITCH_GEN_HOOK_HDR_ALT_RX)(MV_GND_PKT_INFO *pktInfoP);

typedef MV_STATUS (*MV_SWITCH_GEN_HOOK_HDR_ALT_RX_FREE)(MV_GND_PKT_INFO *pktInfoP);

typedef MV_STATUS (*MV_SWITCH_GEN_HOOK_HDR_ALT_TX)(MV_GND_PKT_INFO *pktInfoP);

typedef MV_U32    (*MV_SWITCH_GEN_HOOK_CALC_PKT_ID)(MV_GND_PKT_INFO *pktInfoP);

typedef MV_STATUS (*MV_SWITCH_GEN_HOOK_FWD_RX_PKT)(MV_GND_PKT_INFO *pktInfoP,
                                                   MV_U32           rxQ);

typedef MV_STATUS (*MV_SWITCH_GEN_HOOK_HW_INIT_PORTS)(MV_VOID);

typedef MV_STATUS (*MV_SWITCH_GEN_HOOK_HW_CFG_CASCADE)(MV_VOID);

/*
 * Ownership of port, packets etc. (by CPSS, Network Drivers).
 */
typedef enum
{
    MV_NET_OWN_CPSS                              = 0,
    MV_NET_OWN_NET_DRV_RX_LEAVE_TX_NO_ADD_DSA    = 1,
    MV_NET_OWN_NET_DRV_RX_REMOVE_TX_ADD_DSA      = 2,
    MV_NET_OWN_TOTAL_NUM
} MV_NET_OWN;

/*
 * RX hooks.
 */
typedef struct
{
    MV_SWITCH_GEN_HOOK_FWD_RX_PKT        fwdRxReadyPktHookF;
    MV_SWITCH_GEN_HOOK_HDR_ALT_RX        hdrAltBeforeFwdRxPktHookF;
    MV_SWITCH_GEN_HOOK_HDR_ALT_RX_FREE   hdrAltBeforeFwdRxFreePktHookF;
} MV_SW_GEN_RX_HOOKS;

/*
 * TX hooks.
 */
typedef struct
{
    MV_SWITCH_GEN_HOOK_HDR_ALT_TX        hdrAltBeforeFwdTxDonePktHookF;
    MV_SWITCH_GEN_HOOK_HDR_ALT_TX        hdrAltBeforeTxF;
    MV_SWITCH_GEN_HOOK_DRV_OS_PKT_FREE   txDonePktFreeF;
} MV_SW_GEN_TX_HOOKS;

/*
 * Switch Gen. Hardware initializators.
 */
typedef struct _mvSwitchGenHwInit
{
    MV_SWITCH_GEN_HOOK_HW_INIT_PORTS     initPortsF;
    MV_SWITCH_GEN_HOOK_HW_CFG_CASCADE    cfgCascadePorts;
    MV_U8                                ethAddr[MV_MAC_ADDR_SIZE];
} MV_SWITCH_GEN_HW_INIT;

/*
 * Switch Gen. Compoment initializators.
 */
typedef struct _mvSwitchGenInit
{
    MV_BOOL                              isMiiMode;
    MV_BOOL                              isTxSyncMode;
    MV_U8                                gbeDefaultIndex;
    MV_GND_INIT                         *gndInitP;
    MV_SWITCH_GEN_HW_INIT               *genHwInitP;
} MV_SWITCH_GEN_INIT;

MV_STATUS        switchEndRxJob            (MV_VOID);
MV_STATUS        switchEndTxJob            (MV_VOID);
MV_VOID          switchGenRxReadyIsr       (MV_VOID);
MV_VOID          switchGenTxDoneIsr        (MV_VOID);
MV_VOID          switchRxReadyIsrCb        (MV_VOID);
MV_VOID          switchTxDoneIsrCb         (MV_VOID);
MV_VOID          switchEndRxPathLock       (MV_VOID);
MV_VOID          switchEndRxPathUnlock     (MV_VOID);
MV_VOID          switchEndTxPathLock       (MV_VOID);
MV_VOID          switchEndTxPathUnlock     (MV_VOID);
MV_U32           switchGenIntLvlGet        (MV_U32 dev);
MV_VOID          switchEndPrintActiveIface (MV_VOID);
MV_STATUS        switchDrvGenHookSetFwdRxPkt(MV_U32                        hookId,
                                             MV_SWITCH_GEN_HOOK_FWD_RX_PKT hookF);
MV_STATUS        switchDrvGenHookSetCalcFwdRxPktHookId(
    MV_SWITCH_GEN_HOOK_CALC_PKT_ID hookF);
MV_STATUS        switchDrvGenHookSetHdrAltBeforeFwdRxPkt(
    MV_U32 hookId,
    MV_SWITCH_GEN_HOOK_FWD_RX_PKT
    hookF);
MV_STATUS        switchDrvGenHookSetTxDone (
    MV_U32                             ownerId,
    MV_SWITCH_GEN_HOOK_DRV_OS_PKT_FREE hook);
MV_STATUS        switchDrvGenInit          (MV_SWITCH_GEN_INIT *initP);
MV_STATUS        switchGenCfgCascadePorts  (MV_VOID);
MV_STATUS        switchDrvGenInitHw        (MV_SWITCH_GEN_HW_INIT *hwInitP);
MV_STATUS        switchGenHooksInitRx      (MV_U32 numOfHooks);
MV_STATUS        switchGenHooksInitTx      (MV_U32 numOfPktOwners);
MV_STATUS        switchGenHooksFillRx(MV_SW_GEN_RX_HOOKS *rxHooksP, MV_U32 hookId);
MV_STATUS        switchGenHooksFillTx(MV_SW_GEN_TX_HOOKS *txHooksP, MV_U32 hookId);
MV_STATUS        switchGenInitHw           (MV_VOID);
MV_STATUS        switchGenInitHwGbe        (MV_VOID);
MV_STATUS        switchGenInitHwPp         (MV_VOID);
MV_STATUS        switchDrvGenUnload        (MV_VOID);
MV_VOID          switchGenRxReadyIsr       (MV_VOID);
MV_VOID          switchGenTxDoneIsr        (MV_VOID);
MV_STATUS        switchGenRxJob            (MV_VOID);
MV_STATUS        switchGenTxJob            (MV_VOID);
MV_STATUS        switchGenSendPkt  (MV_GND_PKT_INFO *pktInfoP, MV_U32 txQ);
MV_STATUS        switchGenFreeRxPkt(MV_GND_PKT_INFO *pktInfoP, MV_U32 rxQ);
MV_STATUS        switchGenIntInit  (MV_VOID (*rxReadyIsrF)(MV_VOID),
                                    MV_VOID (*txDoneIsrF) (MV_VOID));
MV_VOID          switchGenIntEnable        (MV_BOOL rxIntEnable, MV_BOOL txIntEnable);
MV_STATUS        switchGenStart            (MV_VOID);
MV_STATUS        switchGenStop             (MV_VOID);
MV_STATUS        pktInfoToPktInfo(MV_GND_PKT_INFO *pktFromP,
                                  MV_GND_PKT_INFO *pktToP);
MV_GND_PKT_INFO *switchGenWrapBuffs(MV_U8 *        buffs[],
                                    MV_U32         buffsLen[],
                                    MV_U32         buffNum,
                                    GEN_SYNC_POOL *pktInfoPoolP);
MV_GND_PKT_INFO *switchGenWrapBuff (MV_U8 *buffP,
                                    MV_U32 buffLen,
                                    GEN_SYNC_POOL *pktInfoPoolP);
MV_STATUS switchGenWrapBuffsSend(MV_U8          *buffs[],
                                 MV_U32          buffsLen[],
                                 MV_U32          buffNum,
                                 MV_U32          txQ,
                                 GEN_SYNC_POOL  *pktInfoPoolP,
                                 MV_U32          ownerId);
MV_STATUS switchGenWrapBuffSend(MV_U8          *buffP,
                                MV_U32          buffLen,
                                MV_U32          txQ,
                                GEN_SYNC_POOL  *pktInfoPoolP,
                                MV_U32          ownerId);
MV_GND_PKT_INFO *switchGenGetRxPkt         (MV_U32 rxQ);
void             switchGenPrint            (void);
MV_VOID          switchGenPrintStat        (void);
MV_BOOL          switchGenIsAttached       (void);
MV_BOOL          switchGenIsStarted        (void);
MV_STATUS        switchDrvGenIsInited      (void);
MV_BOOL          switchGenIsLinkUp         (void);
MV_GND_PKT_INFO *switchGenTxPktInfoGet     (void);

#endif /* __INCmvNetDrvCommonh */
