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

#ifndef __INCmvGndHwIfH
#define __INCmvGndHwIfH

#include "mvTypes.h"
#include "mvEth.h"

typedef MV_VOID      (*MV_GND_HW_HOOK_INT_MASK_RX_READY)  (MV_U32 rxQBitMask);
typedef MV_VOID      (*MV_GND_HW_HOOK_INT_MASK_TX_DONE)   (MV_U32 txQBitMask);
typedef MV_VOID      (*MV_GND_HW_HOOK_INT_UNMASK_RX_READY)(MV_U32 rxQBitMask);
typedef MV_VOID      (*MV_GND_HW_HOOK_INT_UNMASK_TX_DONE) (MV_U32 txQBitMask);
typedef MV_VOID      (*MV_GND_HW_HOOK_INT_ACK_RX_READY) (MV_U32 rxQBitMask);
typedef MV_VOID      (*MV_GND_HW_HOOK_INT_ACK_TX_DONE)  (MV_U32 txQBitMask);
typedef MV_U32       (*MV_GND_HW_HOOK_Q_GET_RX_READY)   (MV_VOID);
typedef MV_U32       (*MV_GND_HW_HOOK_Q_GET_TX_DONE)    (MV_VOID);
typedef MV_STATUS    (*MV_GND_HW_HOOK_RX_REFILL)(MV_PKT_INFO *pktInfoP, MV_U32 rxQ);
typedef MV_PKT_INFO *(*MV_GND_HW_HOOK_GET_PKT_RX_READY) (MV_U32 rxQ);
typedef MV_PKT_INFO *(*MV_GND_HW_HOOK_GET_PKT_TX_DONE)  (MV_U32 txQ);
typedef MV_STATUS    (*MV_GND_HW_HOOK_PKT_TX)(MV_PKT_INFO *pktInfoP, MV_U32 txQ);
typedef MV_STATUS    (*MV_GND_HW_HOOK_IS_TX_DONE)       (MV_U32 txQ);
typedef MV_U32       (*MV_GND_HW_HOOK_RX_RESOURCE_GET)  (MV_U32 txQ);
typedef MV_STATUS    (*MV_GND_HW_HOOK_INIT)(MV_ETH_INIT *ethInitP);
typedef MV_STATUS    (*MV_GND_HW_HOOK_ENABLE)           (MV_VOID);
typedef MV_STATUS    (*MV_GND_HW_HOOK_MAC_ADDR_SET)(MV_U8 *macP, MV_U32 rxQ);
typedef MV_STATUS    (*MV_GND_HW_HOOK_MRU_SET)(MV_U32 mruBytes);

typedef struct
{
    MV_GND_HW_HOOK_INT_MASK_RX_READY            mvGndHwIntMaskRxReadyF;
    MV_GND_HW_HOOK_INT_MASK_TX_DONE             mvGndHwIntMaskTxDoneF;
    MV_GND_HW_HOOK_INT_UNMASK_RX_READY          mvGndHwIntUnmaskRxReadyF;
    MV_GND_HW_HOOK_INT_UNMASK_TX_DONE           mvGndHwIntUnmaskTxDoneF;
    MV_GND_HW_HOOK_INT_ACK_RX_READY             mvGndHwIntAckRxReadyF;
    MV_GND_HW_HOOK_INT_ACK_TX_DONE              mvGndHwIntAckTxDoneF;
    MV_GND_HW_HOOK_Q_GET_RX_READY               mvGndHwQGetRxReadyF;
    MV_GND_HW_HOOK_Q_GET_TX_DONE                mvGndHwQGetTxDoneF;
    MV_GND_HW_HOOK_RX_REFILL                    mvGndHwRxRefillF;
    MV_GND_HW_HOOK_GET_PKT_RX_READY             mvGndHwGetPktRxReadyF;
    MV_GND_HW_HOOK_GET_PKT_TX_DONE              mvGndHwGetPktTxDoneF;
    MV_GND_HW_HOOK_PKT_TX                       mvGndHwPktTxF;
    MV_GND_HW_HOOK_IS_TX_DONE                   mvGndHwIsTxDoneF;
    MV_GND_HW_HOOK_RX_RESOURCE_GET              mvGndHwRxResourceGetF;
    MV_GND_HW_HOOK_INIT                         mvGndHwInitF;
    MV_GND_HW_HOOK_ENABLE                       mvGndHwEnableF;
    MV_GND_HW_HOOK_MAC_ADDR_SET                 mvGndHwMacAddrSetF;
    MV_GND_HW_HOOK_MRU_SET                      mvGndHwMruSetF;
} MV_GND_HW_FUNCS;

#endif /* __INCmvGndHwIfH */

