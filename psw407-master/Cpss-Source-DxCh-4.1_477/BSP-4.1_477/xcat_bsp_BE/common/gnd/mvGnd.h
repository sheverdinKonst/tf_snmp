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

#ifndef __INCmvGndH
#define __INCmvGndH

#include "mvTypes.h"
#include "mvGenSyncPool.h"
#include "mv802_3.h"
#include "mvGndHwIf.h"
#include "mvGndOsIf.h"

typedef struct
{
    MV_U32           rxBuffsBulkSize;
    MV_U8           *rxBuffsBulkP;
    MV_U32           rxBuffSize;
    MV_U32          *actualRxBuffsNumP;
    MV_U32           rxBuffHdrOffset;
    MV_U32          *rxDescNumPerQ;
    MV_U32           rxDescNumTotal;
    MV_U32           numOfRxQueues;
    MV_BOOL          isToDel2PrepBytes;
    MV_U8            macAddr[MV_MAC_ADDR_SIZE];
    MV_U32           mruBytes;
} MV_GND_RX_INIT;

typedef struct
{
    MV_U32          *txDescNumPerQ;
    MV_U32           txDescNumTotal;
    MV_U32           numOfTxQueues;
    MV_BOOL          isTxSyncMode;
    MV_U32           pollTimoutUSec;
    MV_U32           maxPollTimes;
} MV_GND_TX_INIT;

typedef struct
{
    MV_GND_RX_INIT   rx;
    MV_GND_TX_INIT   tx;
    MV_GND_HW_FUNCS *hwP;
    MV_GND_OS_FUNCS *osP;
    MV_U32           maxFragsInPkt;
    MV_U32           gbeIndex;
} MV_GND_INIT;

MV_STATUS    mvGndInitSw(MV_GND_INIT *initP);
MV_STATUS    mvGndRxInit(MV_GND_RX_INIT *rxInitP);
MV_STATUS    mvGndTxInit(MV_GND_TX_INIT *txInitP);

MV_BOOL      mvGndIsInited       (MV_VOID);
MV_VOID      mvGndMacAddrSet     (MV_U8 *macAddr);
MV_U8       * mvGndMacAddrGet    (MV_VOID);
MV_VOID      mvGndIsTxSyncModeSet(MV_U32 pollTimoutUSec, MV_U32 maxPollTimes);

MV_VOID      mvGndIsTxSyncModeUnset   (MV_VOID);
MV_BOOL      mvGndIsTxSyncModeGet     (MV_VOID);
MV_VOID      mvGndDel2PrepBytesModeSet(MV_BOOL isToDel2PrepBytes);
MV_BOOL      mvGndDel2PrepBytesModeGet(MV_VOID);

MV_U32       mvGndMaxFragsInPktGet(MV_VOID);
MV_VOID      mvGndRxReadyIsr      (MV_VOID);
MV_VOID      mvGndTxDoneIsr       (MV_VOID);
MV_STATUS    mvGndRxBottomHalf    (MV_VOID);
MV_STATUS    mvGndTxBottomHalf    (MV_VOID);
MV_STATUS    mvGndFreeRxPkt(MV_GND_PKT_INFO *pktInfoP, MV_U32 rxQ);
MV_STATUS    mvGndSendPkt  (MV_GND_PKT_INFO *pktInfoP, MV_U32 txQ);
MV_U32       mvGndRxResourceGet   (MV_VOID);
MV_STATUS    mvGndRxRefill        (MV_U32 rxQ, MV_U32 num);
MV_STATUS    mvGndEnableHw(MV_VOID);
MV_STATUS    mvGndEnable(MV_U32  defaultRxQ,
                         MV_U32  rxBuffSize,
                         MV_U32  mru);
MV_VOID      mvGndIntEnable       (MV_BOOL rxIntEnable, MV_BOOL txIntEnable);
MV_VOID      mvGndIntDisable      (MV_BOOL rxIntEnable, MV_BOOL txIntEnable);
MV_U32       mvGndBuffSizeGet     (MV_VOID);

GEN_SYNC_POOL   * mvGndTxPktInfoPoolCreate(MV_U32 totalNum, MV_U32 maxFragsInPkt);
MV_GND_PKT_INFO * mvGndGetRxPkt(MV_U32 rxQ);

#endif /* __INCmvGndH */

