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

#include "mvPrestera.h"
#include "mvPresteraPriv.h"
#include "mvSysHwConfig.h"
#include "mvTypes.h"
#include "mvOs.h"
#include "mvCtrlEnvRegs.h"
#include "ddr2/mvDramIfRegs.h"
#include "mvPex.h"
#include "mvCpuIf.h"
#include "mvGenPool.h"
#include "mv802_3.h"
#include "mvWriteLog.h"
#include "mvErrata.h"
#include "mvErrataxCatPP.h"
#include "mvErrataxCatCPU.h"

#ifdef MV_DEBUG
#define DB(x) x
#else
#define DB(x)
#endif

#define MV_PP_HAL_WRITE_LOG_LEN       1000

MV_PP_HAL_CTRL  G_mvPpHalCtrl;
MV_PP_HAL_CTRL *G_mvPpHalCtrlP = &G_mvPpHalCtrl;

MV_PKT_INFO *G_headP = NULL;
MV_PKT_INFO *G_tailP = NULL;

/*
 * Pre-allocates memory for padding outgoing packets.
 * Add 8 byte padding (another buffer is added) should
 * be done only if packets length is 256*n+k (1<=k<=8) (including DSA tag),
 * where n and k are integers.
 */
NET_CACHE_DMA_BLOCKS_INFO   netCacheDmaBlocks[SYS_CONF_MAX_DEV];
SDMA_INTERRUPT_CTRL         G_sdmaInterCtrl[SYS_CONF_MAX_DEV];

GEN_POOL *mvPpRxPktInfoPoolP[PP_DEVS_NUM] = {0};

MV_BOOL G_isPpHalInited = MV_FALSE;
MV_BOOL G_forcePpWriteLogPause = MV_FALSE;

MV_U32 _prestera_dev_base_addr[] = MV_PP_DEV_BASE_TBL;

/*******************************************************************************
 * mvPpHalIsInited
 */
MV_BOOL mvPpHalIsInited(void)
{
    return G_isPpHalInited;
}

/*******************************************************************************
 * mvPpInitDevCfgInfo
 */
MV_VOID mvPpInitDevCfgInfo(void)
{
    MV_PP_HAL_CTRL *ctrlP = G_mvPpHalCtrlP;

    if (mvPpChipIsXCat2() == MV_TRUE)
    {
        ctrlP->devCfg.dsaEnableCfgReg = 0xF000008;
        ctrlP->devCfg.dsaEnableBit    = BIT_31;

        ctrlP->devCfg.prependCfgReg   = 0xF000100;
        ctrlP->devCfg.prependCfgBit   = BIT_15;
    }
    else
    {
        ctrlP->devCfg.dsaEnableCfgReg = CASCADE_AND_HEADER_CONFIG_REG;
        ctrlP->devCfg.dsaEnableBit    = BIT_31;

        ctrlP->devCfg.prependCfgReg   = CASCADE_AND_HEADER_CONFIG_REG;
        ctrlP->devCfg.prependCfgBit   = BIT_28;
    }
}

/*******************************************************************************
* mvPpHalInitHw
*
* DESCRIPTION:
*       This function initialize the Prestera Switch (PP) - its hardware part.
*       1) Enables propagation of interrupt from PP-DEV1 to PP-DEV0 through PEX.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       None.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_STATUS mvPpHalInitHw(void)
{
    MV_U32 intBitIdx, intBitMask;
    /* MV_U32 remoteBar0; */
    MV_U32 remoteBar1;
    MV_U32 temp, pexWin, base, remap;

    if (mvPpDevNumGet() <= 1 )
    {
        return MV_OK;
    }

    /* read PEX BAR 0 & 1 fom remote device */
    /* remoteBar0 = mvPexConfigRead(0, 0, 1, 0, 0x10); */
    remoteBar1 = mvPexConfigRead(0, 0, 1, 0, 0x18);

    /* remove control bits from address */
    /* remoteBar0 &= 0xfffffff0; */
    remoteBar1 &= 0xfffffff0;

    /* Fix the address if remap enabled. */
    /* 1.Find the PEX Window by scan all 8 windows */
    for (pexWin = 0; pexWin < MV_AHB_TO_MBUS_INTREG_WIN; pexWin++)
    {
        temp = MV_REG_READ(AHB_TO_MBUS_WIN_CTRL_REG(pexWin));
        /*Check if the Window have PEX target attribute*/
        if (( (temp & ATMWCR_WIN_TARGET_MASK) == 0x40 ) &&
            ( (temp & ATMWCR_WIN_ENABLE) == 1) )
        {
            mvOsPrintf("Found PEX remapping win #%x\n", pexWin);
            break;
        }
    }

    if (pexWin == MV_AHB_TO_MBUS_INTREG_WIN)
    {
        mvOsPrintf("Prestera PEX: not found PEX window!!\n");
        return MV_FAIL;
    }

    /*2. Fix the address by remapping base and remap addresses.*/
    base = MV_REG_READ(AHB_TO_MBUS_WIN_BASE_REG(pexWin));
    remap = MV_REG_READ(AHB_TO_MBUS_WIN_REMAP_LOW_REG(pexWin));

    /*fix PP base address array to the dynamic allocated base*/
    _prestera_dev_base_addr[1] = base + (remoteBar1 - remap);

    if (mvPpChipIsXCat2() == MV_TRUE)
    {
        /*config BAR1 window in order to access pp unit*/
        MV_CPU_DEC_WIN addrDecWin;
        MV_U32 regBase;

        if (mvCpuIfTargetWinGet(PEX0_MEM, &addrDecWin) != MV_OK)
        {
            mvOsPrintf("%s: mvCpuIfTargetWinGet failed.\n", __func__);
            return MV_FAIL;
        }

        if (addrDecWin.enable == MV_FALSE)
        {
            mvOsPrintf("%s: PEX win is disabled.\n", __func__);
            return MV_FAIL;
        }

        regBase = addrDecWin.addrWin.baseLow;
        MV_MEMIO_LE32_WRITE(regBase + 0x41820, 0x03ff00c1);
        MV_MEMIO_LE32_WRITE(regBase + 0x41824, remoteBar1);

        /*config interrupts to be routed to the PEX*/
        intBitIdx = INT_LVL_XCAT2_SWITCH % 32;
        intBitMask = 1 << intBitIdx;
        MV_MEMIO_LE32_WRITE(base + CPU_ENPOINT_MASK_HIGH_REG, intBitMask);
    }

    return MV_OK;
}

/*******************************************************************************
 * mvPpHalInit
 */
MV_STATUS mvPpHalInit(void)
{
    MV_PP_HAL_CTRL *ctrlP;
    static MV_BOOL funcAlreadyRun = MV_FALSE;
    if (funcAlreadyRun == MV_TRUE)
    {
        return MV_OK;
    }
    funcAlreadyRun = MV_TRUE;
    G_isPpHalInited = MV_TRUE;

    ctrlP = G_mvPpHalCtrlP;
    mvOsMemset(ctrlP, 0, sizeof(MV_PP_HAL_CTRL));

    ctrlP->ppLogHandleP = mvWriteLogCreateStatic(/* MV_PP_HAL_WRITE_LOG_LEN */);
    if (ctrlP->ppLogHandleP == NULL)
    {
        mvOsPrintf("%s: mvWriteLogCreate failed.\n", __func__);
        return MV_FAIL;
    }

    if (G_forcePpWriteLogPause == MV_FALSE)
    {
        if (mvWriteLogActivate(ctrlP->ppLogHandleP) != MV_OK)
        {
            mvOsPrintf("%s: mvWriteLogActivate failed.\n", __func__);
            return MV_FAIL;
        }
    }

    mvPpInitDevCfgInfo();

    if (mvPpHalInitHw() != MV_OK)
    {
        mvOsPrintf("%s: mvPpHalInitHw failed.\n", __func__);
        return MV_FAIL;
    }

    return MV_OK;
}

/*******************************************************************************
 * mvPpWriteLogActivate
 */
MV_STATUS mvPpWriteLogActivate(void)
{
    MV_PP_HAL_CTRL *ctrlP = G_mvPpHalCtrlP;

    G_forcePpWriteLogPause = MV_FALSE;
    if (mvPpHalIsInited() == MV_TRUE)
    {
        if (mvWriteLogActivate(ctrlP->ppLogHandleP) != MV_OK)
        {
            mvOsPrintf("%s: mvWriteLogActivate failed.\n", __func__);
            return MV_FAIL;
        }
    }

    return MV_OK;
}

/*******************************************************************************
 * mvPpWriteLogPause
 */
MV_STATUS mvPpWriteLogPause(void)
{
    MV_PP_HAL_CTRL *ctrlP = G_mvPpHalCtrlP;

    G_forcePpWriteLogPause = MV_TRUE;
    if (mvPpHalIsInited() == MV_TRUE)
    {
        if (mvWriteLogPause(ctrlP->ppLogHandleP) != MV_OK)
        {
            mvOsPrintf("%s: mvWriteLogPause failed.\n", __func__);
            return MV_FAIL;
        }
    }

    return MV_OK;
}

/*******************************************************************************
 * mvPpSwitchTx
 */
MV_STATUS mvPpSwitchTx(MV_U32 devNum, MV_PKT_INFO *pktInfoP, MV_U32 txQ)
{
 /*
  * MV_PP_HAL_CTRL *ctrlP = G_mvPpHalCtrlP;
  * GEN_POOL       *poolP = ctrlP->txPktInfoPoolP;
  */
    MV_U8          *buffList[1];
    MV_U8          *phyBuffList[1];
    MV_U32          buffLen[1];

    MV_PKT_INFO    *headP = G_headP;
    MV_PKT_INFO    *tailP = G_tailP;

    DB(mvOsPrintf("%s: ENTERED.\n", __func__));

    buffList[0]    = (MV_U8 *)pktInfoP->pFrags->bufVirtPtr;
    phyBuffList[0] = (MV_U8 *)pktInfoP->pFrags->bufPhysAddr;
    buffLen[0]     = pktInfoP->pktSize;

    if (mvSwitchDoTx(devNum,
                     txQ,
                     1, /* appendCrc */
                     buffList,
                     phyBuffList,
                     buffLen,
                     1 /* numOfBufs */) != MV_OK)
    {
        mvOsPrintf("%s: mvSwitchDoTx failed.\n", __func__);
        return MV_OK;
    }

    /*
     * Save meanwhile pktInfoP before returning it (or some other pktInfoP from
     * TX pool) to the driver on TX_DONE.
     */
    if (tailP == NULL)
    {
        headP = pktInfoP;
        tailP = pktInfoP;
        pktInfoP->nextP = NULL;
        pktInfoP->prevP = NULL;
    }
    else
    {
        pktInfoP->nextP = tailP;
        tailP->prevP = pktInfoP;
        pktInfoP->prevP = NULL;
        tailP = pktInfoP;
    }

    G_headP = headP;
    G_tailP = tailP;

    return MV_OK;
}

/*******************************************************************************
 * mvSwitchTxStart
 */
MV_STATUS mvSwitchTxStart(MV_PKT_DESC *packetDesc)
{
    if (mvSwitchDoTx(packetDesc->mrvlTag.swDsa.srcDev,
                     packetDesc->mrvlTag.swDsa.tc,
                     packetDesc->appendCrc,
                     packetDesc->pcktData,
                     packetDesc->pcktPhyData,
                     packetDesc->pcktDataLen,
                     1 /*numOfBufs */) != MV_OK)
    {
        mvOsPrintf("%s: mvSwitchDoTx failed.\n", __func__);
        return MV_FAIL;
    }

    if (mvSwitchTxDone(packetDesc->mrvlTag.swDsa.srcDev,
                       packetDesc->mrvlTag.swDsa.tc,
                       1) != MV_OK)
    {
        mvOsPrintf("%s: mvSwitchTxDone failed.\n", __func__);
        return MV_FAIL;
    }

    return MV_OK;
}

/*******************************************************************************
 * mvSwitchBuildPacket
 */
MV_STATUS mvSwitchBuildPacket(MV_U8         devNum,
                              MV_U32        portNum,
                              MV_U32        entryId,
                              MV_U8         appendCrc,
                              MV_U32        pktsNum,
                              MV_U8         gap,
                              MV_U8        *pktData,
                              MV_U32        pktSize,
                              MV_PKT_DESC  *netTxPacketDesc)
{
    MV_U32 dsaTag[2], bufferIndex;

    if (netTxPacketDesc->pcktData[0] == NULL)
    {
        mvOsPrintf("%s: buff pointer is NULL.\n", __func__);
        return MV_FAIL;
    }

    netTxPacketDesc->entryId        = entryId;
    netTxPacketDesc->appendCrc      = 1;
    netTxPacketDesc->pcktsNum       = pktsNum;
    netTxPacketDesc->gap            = gap;
    netTxPacketDesc->waitTime       = 0;

#ifdef PRESTERA_NO_CPU_PORT_DSA
    /* packet size should be at least MIN_PCKT_SIZE bytes */
    if (pktSize >= MIN_PCKT_SIZE - CRC_SIZE)
        netTxPacketDesc->pcktDataLen[0] = pktSize + CRC_SIZE;
    else
        netTxPacketDesc->pcktDataLen[0] = MIN_PCKT_SIZE;
#else
    /* packet size should be at least MIN_PCKT_SIZE bytes including DSA tag) */
    if (pktSize >= MIN_PCKT_SIZE - EXTEND_DSA_TAG_SIZE - CRC_SIZE)
        netTxPacketDesc->pcktDataLen[0] = pktSize + EXTEND_DSA_TAG_SIZE + CRC_SIZE;
    else
        netTxPacketDesc->pcktDataLen[0] = MIN_PCKT_SIZE;

    dsaTag[0] = calcFromCpuDsaTagWord0(devNum, portNum);
    dsaTag[1] = calcFromCpuDsaTagWord1(portNum);

    /* Set SA & DA */
    bufferIndex = 0;
    mvOsMemcpy(netTxPacketDesc->pcktData[0] + bufferIndex, pktData, MAC_SA_AND_DA_SIZE);

    /* Set DSA Tag (2 words) */
    bufferIndex += MAC_SA_AND_DA_SIZE;
    mvOsMemcpy(netTxPacketDesc->pcktData[0] + bufferIndex,
         &dsaTag[0], EXTEND_DSA_TAG_SIZE/2);
    bufferIndex += EXTEND_DSA_TAG_SIZE/2;
    mvOsMemcpy(netTxPacketDesc->pcktData[0] + bufferIndex,
         &dsaTag[1], EXTEND_DSA_TAG_SIZE/2);

    /* Set the rest of the packet */
    bufferIndex += EXTEND_DSA_TAG_SIZE/2;
    mvOsMemcpy(netTxPacketDesc->pcktData[0] + bufferIndex,
         pktData + MAC_SA_AND_DA_SIZE, pktSize - MAC_SA_AND_DA_SIZE);
#endif /* PRESTERA_NO_CPU_PORT_DSA */

    netTxPacketDesc->pcktDataLen[1] = 0;
    netTxPacketDesc->pcktDataAlign[0] = 0 ;
    netTxPacketDesc->mrvlTag.swDsa.srcDev = devNum;
    netTxPacketDesc->mrvlTag.swDsa.tc = 0;

    return MV_OK;
}

/*******************************************************************************
 * mvSwitchInjectExtDsa
 *     Assumes that buffer has pre-allocated header of at least 8 bytes.
 */
MV_STATUS mvSwitchInjectExtDsa(MV_GND_PKT_INFO *pktInfoP)
{
    MV_U32  dsaTag[2], shiftLen, offset;
    MV_U8  *resPktP;
    MV_U8  *pktP;

    pktP      = pktInfoP->pFrags->bufVirtPtr;
    resPktP   = pktP - EXTEND_DSA_TAG_SIZE;

    dsaTag[0] = mvSwitchFromCpuMultiTargetDsaTagWord0();
    dsaTag[1] = mvSwitchFromCpuMultiTargetDsaTagWord1();

    offset    = MAC_BYTES * 2;
    shiftLen  = MAC_BYTES * 2;

    mvOsMemmove(resPktP, pktP, shiftLen);
    mvOsMemmove(resPktP + offset, dsaTag, EXTEND_DSA_TAG_SIZE);

#ifdef ETH_DESCR_IN_HIGH_MEM
    pktInfoP->pFrags->bufPhysAddr = (MV_ULONG)bspVirt2Phys((MV_U32)resPktP);
#else
    pktInfoP->pFrags->bufPhysAddr = (MV_ULONG)resPktP;
#endif
    pktInfoP->pFrags->bufVirtPtr  = resPktP;
    pktInfoP->pFrags->dataSize   += EXTEND_DSA_TAG_SIZE;
    pktInfoP->pktSize            += EXTEND_DSA_TAG_SIZE;

    return MV_OK;
}

/*******************************************************************************
 * mvSwitchExtractExtDsa
 */
MV_STATUS mvSwitchExtractExtDsa(MV_GND_PKT_INFO *pktInfoP)
{
    MV_U32  shiftLen;
    MV_U8  *resPktP;
    MV_U8  *pktP;

    pktP      = pktInfoP->pFrags->bufVirtPtr;
    resPktP   = pktP + EXTEND_DSA_TAG_SIZE;

    shiftLen  = MAC_BYTES * 2;
    mvOsMemmove((void *)&pktInfoP->extDsa.ww.word0, pktP + shiftLen,     4);
    mvOsMemmove((void *)&pktInfoP->extDsa.ww.word1, pktP + shiftLen + 4, 4);

    mvOsMemmove(resPktP, pktP, shiftLen);

#ifdef ETH_DESCR_IN_HIGH_MEM
    pktInfoP->pFrags->bufPhysAddr = (MV_ULONG)bspVirt2Phys((MV_U32)resPktP);
#else
    pktInfoP->pFrags->bufPhysAddr = (MV_ULONG)resPktP;
#endif
    pktInfoP->pFrags->bufVirtPtr  = resPktP;
    pktInfoP->pFrags->dataSize   -= EXTEND_DSA_TAG_SIZE;
    pktInfoP->pktSize            -= EXTEND_DSA_TAG_SIZE;

    return MV_OK;
}

/*******************************************************************************
 * mvSwitchRememberExtDsa
 */
MV_STATUS mvSwitchRememberExtDsa(MV_GND_PKT_INFO *pktInfoP)
{
    MV_U8   *pktP;
    MV_U32  *extDsaP;

    pktP      = pktInfoP->pFrags->bufVirtPtr;
    extDsaP   = (MV_U32 *)(pktP + (MAC_BYTES * 2));

    mvOsMemmove((void *)&pktInfoP->extDsa.ww.word0, extDsaP,     4);
    mvOsMemmove((void *)&pktInfoP->extDsa.ww.word1, extDsaP + 4, 4);

    /*
     * pktInfoP->extDsa.ww.word0 = extDsaP[0];
     * pktInfoP->extDsa.ww.word1 = extDsaP[1];
     */

    return MV_OK;
}

/*******************************************************************************
 * mvSwitchRemoveExtDsa
 */
MV_STATUS mvSwitchRemoveExtDsa(MV_GND_PKT_INFO *pktInfoP)
{
    MV_U32  shiftLen;
    MV_U8  *resPktP;
    MV_U8  *pktP;

    pktP      = pktInfoP->pFrags->bufVirtPtr;
    resPktP   = pktP + EXTEND_DSA_TAG_SIZE;

    shiftLen  = MAC_BYTES * 2;

    mvOsMemmove(resPktP, pktP, shiftLen);

#ifdef ETH_DESCR_IN_HIGH_MEM
    pktInfoP->pFrags->bufPhysAddr = (MV_ULONG)bspVirt2Phys((MV_U32)resPktP);
#else
    pktInfoP->pFrags->bufPhysAddr = (MV_ULONG)resPktP;
#endif
    pktInfoP->pFrags->bufVirtPtr  = resPktP;
    pktInfoP->pFrags->dataSize   -= EXTEND_DSA_TAG_SIZE;
    pktInfoP->pktSize            -= EXTEND_DSA_TAG_SIZE;

    return MV_OK;
}

/*******************************************************************************
 * mvSwitchGetPortNumFromExtDsa
 */
MV_U32 mvSwitchGetPortNumFromExtDsa(MV_GND_PKT_INFO *pktInfoP)
{
    MV_U32 port;
    port = (MV_32BIT_LE_FAST(pktInfoP->extDsa.ww.word0) >> 11) & 0x1F;
    return port;
}

/*******************************************************************************
 * mvSwitchBuildPacketZeroCopy
 */
MV_STATUS mvSwitchBuildPacketZeroCopy(MV_U8         devNum,
                                      MV_U32        portNum,
                                      MV_U32        entryId,
                                      MV_U8         appendCrc,
                                      MV_U32        pktsNum,
                                      MV_U8         gap,
                                      MV_U8        *pktData,
                                      MV_U32        pktSize,
                                      MV_PKT_DESC  *pktDesc)
{
    MV_U32 dsaTag[2];
    MV_U8 *resPckt;

    if (pktData == NULL)
    {
        return MV_NO_RESOURCE;
    }

    resPckt = pktData - EXTEND_DSA_TAG_SIZE;

    pktDesc->entryId   = entryId;
    pktDesc->appendCrc = 1;
    pktDesc->pcktsNum  = pktsNum;
    pktDesc->gap       = gap;
    pktDesc->waitTime  = 0;

    if (pktSize >= MIN_PCKT_SIZE - EXTEND_DSA_TAG_SIZE - CRC_SIZE)
        pktDesc->pcktDataLen[0] = pktSize + EXTEND_DSA_TAG_SIZE + CRC_SIZE;
    else
        pktDesc->pcktDataLen[0] = MIN_PCKT_SIZE;

    dsaTag[0] = mvSwitchFromCpuMultiTargetDsaTagWord0();
    dsaTag[1] = mvSwitchFromCpuMultiTargetDsaTagWord1();

    mvOsMemmove(resPckt, pktData, MAC_BYTES * 2);
    mvOsMemmove(resPckt + MAC_BYTES * 2, dsaTag, EXTEND_DSA_TAG_SIZE);
    pktDesc->pcktData[0]          = resPckt;
    pktDesc->pcktDataLen[1]       = 0;
    pktDesc->pcktDataAlign[0]     = 0;
    pktDesc->mrvlTag.swDsa.srcDev = devNum;
    pktDesc->mrvlTag.swDsa.tc     = 0;
    return MV_OK;
}

/*******************************************************************************
 * mvSwitchReleasePacket
 */
MV_STATUS mvSwitchReleasePacket(MV_PKT_DESC *packetDesc)
{
    if (packetDesc->pcktData[0])
    {
        mvOsFree(packetDesc->pcktData[0]);
    }

    return MV_OK;
}

/*******************************************************************************
 * mvSwitchFromCpuMultiTargetDsaTagWord0
 */
MV_U32 mvSwitchFromCpuMultiTargetDsaTagWord0(void)
{
    MV_U32 baseTag = 0x400C1001;
    return MV_32BIT_BE(baseTag);
}

/*******************************************************************************
 * mvSwitchFromCpuMultiTargetDsaTagWord1
 */
MV_U32 mvSwitchFromCpuMultiTargetDsaTagWord1(void)
{
    MV_U32 baseTag = 0x000007FF;
    return MV_32BIT_BE(baseTag);
}

/*******************************************************************************
 * calcFromCpuDsaTagWord0
 */
MV_U32 calcFromCpuDsaTagWord0(MV_U8 devNum, MV_U32 portNum)
{
    MV_U32 baseTag = 0x40001001; /* From CPU, Extended, VID=1 */
    /* port bits [4:0] are in this tag */
    /* port bits [5:5] are in extended tag */

    return MV_32BIT_BE(baseTag | ((devNum  & 0x0F) << MRVL_TAG_DEV_BIT) |
                     ((portNum & 0x1F) << MRVL_TAG_PORT_BIT));
}

/*******************************************************************************
 * calcFromCpuDsaTagWord1
 */
MV_U32 calcFromCpuDsaTagWord1(MV_U32 portNum)
{
    /* port bits [4:0] are in word 0 */
    /* port bits [5:5] are in extended tag */
    MV_U32 dsaWord1 = 0;

    dsaWord1  = (((portNum & 0x20) >> 5) << MRVL_TAG_EXT_PORT_BIT);

    /* Egress filtering enable. */
    dsaWord1 |= BIT_30;

    dsaWord1  = MV_32BIT_BE(dsaWord1);

    return dsaWord1;
}

/*******************************************************************************
 * mvSwitchAddDsaTag
 */
MV_STATUS mvSwitchAddDsaTag(MV_U8           devNum,
                            MV_U32          portNum,
                            MV_U8          *buffP,
                            MV_U32          len,
                            MV_BUF_INFO    *bufInfoP)
{
    MV_U32    dsaTag[2], bufferIndex;

    if (bufInfoP == NULL || bufInfoP->bufVirtPtr == NULL)
        return MV_NO_RESOURCE;

    if(len > (SWITCH_MTU - EXTEND_DSA_TAG_SIZE - CRC_SIZE))
    return MV_BAD_PARAM;

    /* packet size should be at least MIN_PCKT_SIZE bytes including DSA tag) */
    if (len >= MIN_PCKT_SIZE - EXTEND_DSA_TAG_SIZE - CRC_SIZE)
        bufInfoP->dataSize = len + EXTEND_DSA_TAG_SIZE + CRC_SIZE;
    else
        bufInfoP->dataSize = MIN_PCKT_SIZE;

    dsaTag[0] = calcFromCpuDsaTagWord0(devNum, portNum);
    dsaTag[1] = calcFromCpuDsaTagWord1(portNum);

    /* Set SA & DA */
    bufferIndex = 0;
    mvOsMemcpy(bufInfoP->bufVirtPtr + bufferIndex, buffP, MAC_SA_AND_DA_SIZE);

    /* Set DSA Tag (2 words) */
    bufferIndex += MAC_SA_AND_DA_SIZE;
    mvOsMemcpy(bufInfoP->bufVirtPtr + bufferIndex,
         &dsaTag[0], EXTEND_DSA_TAG_SIZE/2);
    bufferIndex += EXTEND_DSA_TAG_SIZE/2;
    mvOsMemcpy(bufInfoP->bufVirtPtr + bufferIndex,
         &dsaTag[1], EXTEND_DSA_TAG_SIZE/2);

    /* Set the rest of the packet */
    bufferIndex += EXTEND_DSA_TAG_SIZE/2;
    mvOsMemcpy(bufInfoP->bufVirtPtr + bufferIndex,
         buffP + MAC_SA_AND_DA_SIZE, len - MAC_SA_AND_DA_SIZE);

    return MV_OK;
}

/*******************************************************************************
 * mvSwitchGetDsaTag
 */
MV_STATUS mvSwitchGetDsaTag(MV_U8        devNum,
                            MV_U32       portNum,
                            MV_U8       *buffP)
{
    MV_U32	    dsaTag[2], bufferIndex;

    dsaTag[0] = calcFromCpuDsaTagWord0(devNum, portNum);
    dsaTag[1] = calcFromCpuDsaTagWord1(portNum);

    bufferIndex = 0;

    /* Set DSA Tag (2 words) */
    mvOsMemcpy(buffP + bufferIndex,
         &dsaTag[0], EXTEND_DSA_TAG_SIZE/2);
    bufferIndex   += EXTEND_DSA_TAG_SIZE/2;
    mvOsMemcpy(buffP + bufferIndex,
         &dsaTag[1], EXTEND_DSA_TAG_SIZE/2);

    return MV_OK;
}

/*******************************************************************************
 * mvSwitchDoTx
 */
MV_STATUS mvSwitchDoTx(MV_U8     dev,
                       MV_U8     txQ,
                       MV_BOOL   recalcCrc,
                       MV_U8    *buffList[],
                       MV_U8    *phyBuffList[],
                       MV_U32   *buffLen,
                       MV_U32    numOfBufs)
{
    TX_DESC_LIST *txDescListP;  /* The Tx desc. list control structure. */
    STRUCT_SW_TX_DESC *currSwDesc;  /* The current handled descriptor of*/
                                /* the currently transmitted packet.    */
    STRUCT_TX_DESC  tmpDesc;    /* Temporary Tx descriptor              */
    STRUCT_TX_DESC  tmpFirstTxDesc={0}; /* Temp Tx desc used for         */
                                /* preparing the real first descriptor  */
                                /* data.                                */
    STRUCT_TX_DESC *firstDesc;  /* The first descriptor of the          */
                                /* currently transmitted packet.        */
    MV_U32  descWord1;          /* The first word of the Tx descriptor. */
    MV_U32          i;
    MV_U32          pktLen = 0, addBuff = 0;

    DB(mvOsPrintf( "%s - Start\n", __func__));

    if (dev >= mvSwitchGetDevicesNum())
    {
        mvOsPrintf("%s: Wrong devNum (%d).\n", __func__, dev);
        return MV_FAIL;
    }

    if (txQ >= NUM_OF_TX_QUEUES)
    {
        mvOsPrintf( "%s: Too big txQ.\n", __func__);
        return MV_BAD_PARAM;
    }

    if (buffLen[0] >= MIN_PCKT_SIZE - CRC_SIZE)
    {
        buffLen[0] += CRC_SIZE;
    }
    else
    {
        buffLen[0]  = MIN_PCKT_SIZE;
    }

    MV_ERRATA_APPLY(mvErrataXCatPpHandleGet(), MV_ERRATA_ID_SDMA_256_LEN_FIX,
            &pktLen, &addBuff, buffLen, numOfBufs);

    txDescListP = &(G_sdmaInterCtrl[dev].txDescList[txQ]);
    descWord1   = 0;

    if (numOfBufs + addBuff > txDescListP->freeDescNum)
    {
        mvOsPrintf("%s: No TX Descs(%d).\n", __func__, txDescListP->freeDescNum);
        return MV_NO_RESOURCE;
    }

    if (recalcCrc == MV_TRUE)
    {
        descWord1 |= (1 << 12);
    }

    currSwDesc = txDescListP->next2Feed;

    if (currSwDesc->txDesc->nextDescPointer == (MV_U32)txDescListP->next2Free->txDesc)
    {
        mvOsPrintf("%s: No resources(%d).\n", __func__, txDescListP->freeDescNum);
        return MV_NO_RESOURCE;
    }
    firstDesc  = currSwDesc->txDesc;

    /* build descriotors list */
    for (i = 0; i < numOfBufs; i++)
    {
        /* Check if the buffers length is larger than (TX_SHORT_BUFF_SIZE)  */
        if (buffLen[i] < TX_SHORT_BUFF_SIZE)
        {
            return MV_FAIL;
        }

        TX_DESC_RESET(&tmpDesc);
        tmpDesc.word1 = descWord1;

        tmpDesc.buffPointer = (MV_U32)hwByteSwap((MV_U32)phyBuffList[i]);

        PP_PKT_CACHE_FLUSH(buffList[i],buffLen[i]);

        TX_DESC_SET_BYTE_CNT(&tmpDesc,buffLen[i]);

        /* in case first or last descriptor don't swap */
        if (i == 0)
        {
            /* store first descriptor */
            TX_DESC_COPY(&tmpFirstTxDesc, &tmpDesc);
        }
        else if (i != (numOfBufs - 1))
        {
            TX_DESC_SET_OWN_BIT(&tmpDesc,MV_OWNERSHIP_DMA);
            tmpDesc.word1 = hwByteSwap(tmpDesc.word1);
            tmpDesc.word2 = hwByteSwap(tmpDesc.word2);
            TX_DESC_COPY(currSwDesc->txDesc,&tmpDesc);
            PP_DESCR_FLUSH_INV(NULL, currSwDesc->txDesc);
            currSwDesc = currSwDesc->swNextDesc;
        }
    }

    MV_ERRATA_APPLY(mvErrataXCatPpHandleGet(), MV_ERRATA_ID_SDMA_256_DESC_FIX,
            currSwDesc, &tmpDesc, descWord1, addBuff);

    /* Set the LAST desc params.        */
    if (currSwDesc->txDesc != firstDesc)
    {
        TX_DESC_SET_LAST_BIT(&tmpDesc,1);
        TX_DESC_SET_INT_BIT(&tmpDesc,1);
        tmpDesc.word1 = hwByteSwap(tmpDesc.word1);
        tmpDesc.word2 = hwByteSwap(tmpDesc.word2);
        TX_DESC_COPY(currSwDesc->txDesc,&tmpDesc);
        PP_DESCR_FLUSH_INV(NULL, currSwDesc->txDesc);
    }
    else
    {
        TX_DESC_SET_LAST_BIT(&tmpFirstTxDesc,1);
        TX_DESC_SET_INT_BIT(&tmpFirstTxDesc,1);
    }

    txDescListP->freeDescNum -= numOfBufs;
    txDescListP->next2Feed    = currSwDesc->swNextDesc;

    /* Make sure that all previous operations where */
    /* executed before changing the own bit of the  */
    /* first desc.                                  */
    MV_SYNC;

    /* Set the FIRST descriptor own bit to start transmitting.  */
    TX_DESC_SET_FIRST_BIT(&tmpFirstTxDesc,1);
    TX_DESC_SET_OWN_BIT(&tmpFirstTxDesc,MV_OWNERSHIP_DMA);
    tmpFirstTxDesc.word1    = hwByteSwap(tmpFirstTxDesc.word1);
    tmpFirstTxDesc.word2    = hwByteSwap(tmpFirstTxDesc.word2);
    TX_DESC_COPY(firstDesc,&tmpFirstTxDesc);

    PP_DESCR_FLUSH_INV(NULL,firstDesc);

    /* The Enable DMA operation should be done only */
    /* AFTER all desc. operations where completed.  */
    MV_SYNC;

    /* Enable the Tx DMA.   */
    CHECK_STATUS(mvDbTxDmaSetMode(dev, txQ, MV_TRUE));

    return MV_OK;
}

/*******************************************************************************
 * mvSwitchIsTxDone
 */
MV_BOOL mvSwitchIsTxDone(MV_U8 devNum, MV_U8 txQ)
{
    TX_DESC_LIST        *txDescList;
    STRUCT_SW_TX_DESC   *swTxDesc;

    txDescList = &G_sdmaInterCtrl[devNum].txDescList[txQ];
    swTxDesc   = txDescList->next2Free;

    if (IS_TX_DESC_CPU_OWNED(swTxDesc->txDesc))
    {
        return MV_TRUE;
    }

    return MV_FALSE;
}

/*******************************************************************************
 * mvSwitchTxDone
 */
MV_STATUS mvSwitchTxDone(MV_U8         devNum,
                         MV_U8         txQ,
                         MV_U32        descNum)
{
    MV_STATUS            status = MV_OK;
    TX_DESC_LIST        *txDescList;
    STRUCT_SW_TX_DESC   *swTxDesc;
    MV_U32               freeDescNum;
    MV_U8                ownerBit;
    MV_U32               firstBit, lastBit, count = 0;

    freeDescNum = descNum;

    txDescList = &(G_sdmaInterCtrl[devNum].txDescList[txQ]);

    swTxDesc = txDescList->next2Free;

    DB(mvOsPrintf( "%s - Start\n", __func__));

    PP_DESCR_INV(NULL, swTxDesc->txDesc);
    firstBit = TX_DESC_GET_FIRST_BIT(swTxDesc->txDesc);
    lastBit  = TX_DESC_GET_LAST_BIT (swTxDesc->txDesc);

    /* check if ownership was changed to CPU */
    /* wait for 100 uSec */
    while (1)
    {
        mvOsUDelay(10);

        ownerBit = TX_DESC_GET_OWN_BIT(swTxDesc->txDesc);
        if (ownerBit == 0)
        {
            break;
        }

        if (count >= 500)
        {
            status = MV_TIMEOUT;
            /* Disable the Tx DMA. */
            mvDbTxDmaSetMode(devNum, txQ, MV_FALSE);
            break;
        }
        count++;
    }

    while (freeDescNum > 0)
    {
        swTxDesc->txDesc->word1 = 0x0;
        swTxDesc = swTxDesc->swNextDesc;
        freeDescNum--;
    }

    txDescList->freeDescNum += descNum;
    txDescList->next2Free = swTxDesc;

    if (firstBit == 1 && lastBit == 0)
    {
        txDescList->freeDescNum++;
        txDescList->next2Free = swTxDesc->swNextDesc;
    }

    return status;
}


/*******************************************************************************
* mvDbRxDmaSetMode
*
* DESCRIPTION:
*       Enable/Disable Rx on specified TX DMA Queue
*
* INPUTS:
*        devNum    - device number
*        queue     - tx queue
*        enable    - enable/disable
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK   - on success
*       MV_FAIL - on error
*
* COMMENTS:
*
* GalTis:
*       Command - netQueueEnable
*
*******************************************************************************/
MV_STATUS mvDbRxDmaSetMode(MV_U8    devNum,
                           MV_U8    rxQueue,
                           MV_BOOL  enable)
{
    MV_U32 tmpData = 0;

    if (enable == MV_TRUE)
    {
        U32_SET_BIT(tmpData, rxQueue);
    }
    else
    {
        U32_SET_BIT(tmpData, (rxQueue+8));
    }

    CHECK_STATUS(hwIfSetReg(devNum, 0x2680, tmpData));
    return MV_OK;
}

/*******************************************************************************
* mvDbTxDmaSetMode
*
* DESCRIPTION:
*       Enable/Disable Tx on specified TX DMA Queue
*
* INPUTS:
*        devNum    - device number
*        queue     - tx queue
*        enable    - enable/disable
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK   - on success
*       MV_FAIL - on error
*
* COMMENTS:
*
* GalTis:
*       Command - netQueueEnable
*
*******************************************************************************/
MV_STATUS mvDbTxDmaSetMode(MV_U8    devNum,
                           MV_U8    txQueue,
                           MV_BOOL  enable)
{
    MV_U32 tmpData = 0;

    if (enable == MV_TRUE)
    {
        U32_SET_BIT(tmpData, txQueue);
    }
    else
    {
        U32_SET_BIT(tmpData, (txQueue+8));
    }

    CHECK_STATUS(hwIfSetReg(devNum, 0x2868, tmpData));
    return MV_OK;
}

/*******************************************************************************
 * mvPpTxDmaStart
 */
void mvPpTxDmaStart(MV_U8 dev, MV_U8 txQ)
{
    MV_U32 tmpData = 0;

    U32_SET_BIT(tmpData, txQ);

    mvPpWriteReg(dev, 0x2868, tmpData);
}

static MV_BOOL            enableRxProcces = MV_TRUE;
static MV_U32             numRxDataBufs;    /* These vars. are used to be   */
static MV_U32             numRxDescriptors; /* sent to Tx /Rx initialization*/
static MV_U32             numTxDescriptors; /* functions. and hold the      */
                                            /* number of the different descs*/
                                            /* and buffers allocated.       */

/*******************************************************************************
 * mvPpSdmaRxQEnable
 */
MV_STATUS mvPpSdmaRxQEnable(MV_U32 dev, MV_U32 rxQueue)
{
    if (dev >= mvSwitchGetDevicesNum())
    {
        mvOsPrintf("%s: Wrong dev (%d).\n", __func__, dev);
        return MV_FALSE;
    }

    if (hwIfSetReg(dev, 0x2680, 1 << rxQueue) != MV_OK)
    {
        mvOsPrintf("%s: hwIfSetReg failed.\n", __func__);
        return MV_FALSE;
    }

    return MV_OK;
}

/*******************************************************************************
* mvPpRefillRxDesc
*
* DESCRIPTION:
*       Frees a list of buffers, that where previously passed to the upper layer
*       in an Rx event.
*
* INPUTS:
*       pktInfoP    - represents packet to free.
*       dev         - The device number throw which the buffers where received.
*       rxQ         - The Rx queue throw which these buffers where received.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK on success, or
*       MV_FAIL otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_STATUS mvPpRefillRxDesc(MV_U8         devNum,
                           MV_U8         rxQueue,
                           MV_PKT_INFO  *pktInfoP)
{
    STRUCT_SW_RX_DESC *swRxDescP;
    RX_DESC_LIST      *rxDescListP;
    STRUCT_RX_DESC    *rxDescP;
    MV_U8             *virtBuffP;
    MV_U32             physAddr;

    virtBuffP = pktInfoP->pFrags->bufVirtPtr;
    physAddr  = (MV_U32)pktInfoP->pFrags->bufPhysAddr;
    physAddr = hwByteSwap(physAddr);

    rxDescListP = &(G_sdmaInterCtrl[devNum].rxDescList[rxQueue]);

    rxDescListP->rxFreeDescNum--;
    rxDescListP->rxResource++;

    swRxDescP = rxDescListP->next2Return;
    swRxDescP->buffVirtPointer = (MV_U32)virtBuffP;

    rxDescP = swRxDescP->rxDesc;
    rxDescP->buffPointer = (MV_U32)physAddr;

    RX_DESC_SET_BUFF_SIZE_FIELD(rxDescP, pktInfoP->pFrags->bufSize);

    MV_SYNC;

    RX_DESC_RESET(rxDescListP->next2Return->rxDesc);

    MV_SYNC;

    PP_DESCR_FLUSH_INV(NULL, rxDescP);
    PP_CACHE_INV(virtBuffP, (RX_DESC_GET_BUFF_SIZE_FIELD(rxDescP) >> 3));
    rxDescListP->next2Return = rxDescListP->next2Return->swNextDesc;

    if (rxDescListP->forbidQEn == MV_FALSE)
    {
        if (mvPpSdmaRxQEnable(devNum, rxQueue) != MV_OK)
        {
            mvOsPrintf("%s: mvPpSdmaRxQEnable failed.\n", __func__);
            return MV_FALSE;
        }
    }

    return MV_OK;
}

/*******************************************************************************
* mvFreeRxBuf
*
* DESCRIPTION:
*       Frees a list of buffers, that where previously passed to the upper layer
*       in an Rx event.
*
* INPUTS:
*       virtBuffP   - virtual buffer pointer.
*       physBuffP   - physical buffer pointer.
*       devNum      - The device number throw which these buffers where
*                     received.
*       rxQueue     - The Rx queue number throw which these buffers where
*                     received.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK on success, or
*       MV_FAIL otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_STATUS mvFreeRxBuf(MV_U8  *virtBuffP,
                      MV_U32  physBuffP,
                      MV_U8   devNum,
                      MV_U8   rxQueue)
{
    STRUCT_SW_RX_DESC *swRxDescP;
    RX_DESC_LIST      *rxDescList;
    STRUCT_RX_DESC    *rxDesc;

    rxDescList = &(G_sdmaInterCtrl[devNum].rxDescList[rxQueue]);

    rxDescList->rxFreeDescNum--;
    rxDescList->rxResource++;

    swRxDescP = rxDescList->next2Return;
    swRxDescP->buffVirtPointer = (MV_U32)virtBuffP;

    rxDesc = swRxDescP->rxDesc;
    rxDesc->buffPointer = physBuffP;

    MV_SYNC;

    RX_DESC_RESET(rxDescList->next2Return->rxDesc);

    MV_SYNC;

    PP_DESCR_FLUSH_INV(NULL, rxDesc);
    PP_CACHE_INV(virtBuffP, (RX_DESC_GET_BUFF_SIZE_FIELD(rxDesc) >> 3));
    rxDescList->next2Return = rxDescList->next2Return->swNextDesc;

    if (rxDescList->forbidQEn == MV_FALSE)
    {
        if (mvPpSdmaRxQEnable(devNum, rxQueue) != MV_OK)
        {
            mvOsPrintf("%s: mvPpSdmaRxQEnable failed.\n", __func__);
            return MV_FALSE;
        }
    }

    return MV_OK;
}

/*******************************************************************************
* mvFreeSdmaNetIfDev
*
* DESCRIPTION:
*       Free the network interface structures, Rx descriptors & buffers
*       and Tx descriptors (For a single device).
*
* INPUTS:
*       devNum  - The device to initialize.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK   - on success,
*       MV_FAIL - otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_STATUS mvFreeSdmaNetIfDev(MV_U8 devNum)
{
    if (netCacheDmaBlocks[devNum].auDescBlock != NULL)
            mvPpFree(
                netCacheDmaBlocks[devNum].auDescBlockSize,
                netCacheDmaBlocks[devNum].auDescBlockPhy,
                netCacheDmaBlocks[devNum].auDescBlock,
                NULL);
    if (netCacheDmaBlocks[devNum].auDescBlockSecond != NULL)
            mvPpFree(
                netCacheDmaBlocks[devNum].auDescBlockSize,
                netCacheDmaBlocks[devNum].auDescBlockSecondPhy,
                netCacheDmaBlocks[devNum].auDescBlockSecond,
                NULL);
    if (netCacheDmaBlocks[devNum].fuDescBlock != NULL)
            mvPpFree(
                netCacheDmaBlocks[devNum].fuDescBlockSize,
                netCacheDmaBlocks[devNum].fuDescBlockPhy,
                netCacheDmaBlocks[devNum].fuDescBlock,
                NULL);
    if (netCacheDmaBlocks[devNum].txDescBlock != NULL)
            mvPpFree(
                netCacheDmaBlocks[devNum].txDescBlockSize,
                netCacheDmaBlocks[devNum].txDescBlockPhy,
                netCacheDmaBlocks[devNum].txDescBlock,
                NULL);
    if (netCacheDmaBlocks[devNum].rxDescBlock != NULL)
            mvPpFree(
                netCacheDmaBlocks[devNum].rxDescBlockSize,
                netCacheDmaBlocks[devNum].rxDescBlockPhy,
                netCacheDmaBlocks[devNum].rxDescBlock,
                NULL);
    if (netCacheDmaBlocks[devNum].rxBufBlock != NULL)
            mvPpFree(
                netCacheDmaBlocks[devNum].rxBufBlockSize,
                netCacheDmaBlocks[devNum].rxBufBlockPhy,
                netCacheDmaBlocks[devNum].rxBufBlock,
                NULL);

    netCacheDmaBlocks[devNum].auDescBlock = NULL;
    netCacheDmaBlocks[devNum].auDescBlockSecond = NULL;
    netCacheDmaBlocks[devNum].fuDescBlock = NULL;
    netCacheDmaBlocks[devNum].txDescBlock = NULL;
    netCacheDmaBlocks[devNum].rxDescBlock = NULL;
    netCacheDmaBlocks[devNum].rxBufBlock = NULL;

    return MV_OK;
}

/*******************************************************************************
* coreTxRegConfig
*
* DESCRIPTION:
*       Set the needed values for SDMA registers to enable Tx activity.
*
* INPUTS:
*       devNum  - The Pp device number.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK   - on success,
*       MV_FAIL - otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static MV_STATUS coreTxRegConfig(MV_U8 devNum)
{
    MV_U8 i;

    /* transmit queue WRR */
    for (i = 0; i < 8; i++)
    {
        CHECK_STATUS(hwIfSetReg(devNum, 0x2708 + i*0x10, 0));
        CHECK_STATUS(hwIfSetReg(devNum, 0x2700 + i*0x10, 0));
        CHECK_STATUS(hwIfSetReg(devNum, 0x2704 + i*0x10, 0xfffffcff));
    }

    CHECK_STATUS(hwIfSetReg(devNum, 0x2874, 0xffffffc1));
    CHECK_STATUS(hwIfSetReg(devNum, 0x2870, 0));
    return MV_OK;
}

/*******************************************************************************
* coreInitTx
*
* DESCRIPTION:
*       This function initializes the Core Tx module, by allocating the cyclic
*       Tx descriptors list, and the tx Headers buffers.
*
* INPUTS:
*       devNum      - The device number to init the Tx unit for.
*       descBlock   - A block to allocate the descriptors from.
*       descBlockSize - Size in bytes of descBlock.
*
* OUTPUTS:
*       numOfDescs  - Number of allocated Tx descriptors.
*
* RETURNS:
*       MV_OK on success, or
*       MV_FAIL otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static MV_STATUS coreInitTx(MV_U8   devNum,
                            MV_U8  *descBlock,
                            MV_U32  descBlockSize,
                            MV_U32 *numOfDescs)
{
    TX_DESC_LIST *txDescList;   /* Points to the relevant Tx desc. list */

    STRUCT_TX_DESC *firstTxDesc;/* Points to the first Tx desc in list. */
    MV_U8 txQueue;              /* Index of the Tx Queue.               */
    MV_U32 numOfTxDesc;         /* Number of Tx desc. that may be       */
                                /* allocated from the given block.      */
    MV_U32 sizeOfDesc;          /* The ammount of memory (in bytes) that*/
                                /* a single desc. will occupy, including*/
                                /* the alignment.                       */
    MV_U32 descPerQueue;        /* Number of descriptors per Tx Queue.  */
    MV_U32 i;

    STRUCT_SW_TX_DESC *swTxDesc = NULL;/* Points to the Sw Tx desc to   */
                                /* init.                                */
    STRUCT_SW_TX_DESC *firstSwTxDesc;/* Points to the first Sw Tx desc  */
                                /* in list.                             */
    MV_U32  phyNext2Feed;       /* The physical address of the next2Feed*/
                                /* field.                               */

    txDescList = G_sdmaInterCtrl[devNum].txDescList;

    /* Tx Descriptor list initialization.   */
    sizeOfDesc  = sizeof(STRUCT_TX_DESC);
    numOfTxDesc = descBlockSize / sizeOfDesc;
    if((sizeOfDesc % TX_DESC_ALIGN) != 0)
    {
        sizeOfDesc += (TX_DESC_ALIGN-(sizeof(STRUCT_TX_DESC) % TX_DESC_ALIGN));
        numOfTxDesc = (descBlockSize -
                       (TX_DESC_ALIGN -
                        (((MV_U32)descBlock) % TX_DESC_ALIGN))) / sizeOfDesc;
    }
    /* Set the descBlock to point to an alligned start address. */
    if(((MV_U32)descBlock % TX_DESC_ALIGN) != 0)
    {
        descBlock =
            (MV_U8*)((MV_U32)descBlock +
                     (TX_DESC_ALIGN - (((MV_U32)descBlock) % TX_DESC_ALIGN)));
    }

    descPerQueue = numOfTxDesc / NUM_OF_TX_QUEUES;
    /* Number of descriptors must devide by 2.  */
    descPerQueue -= (descPerQueue & 0x1);
    txDescList->sizeOfList = descPerQueue * sizeOfDesc;

    *numOfDescs = descPerQueue * NUM_OF_TX_QUEUES;

    for(txQueue = 0; txQueue < NUM_OF_TX_QUEUES; txQueue++)
    {
        txDescList[txQueue].freeDescNum = descPerQueue;

        firstTxDesc = (STRUCT_TX_DESC*)descBlock;

        firstSwTxDesc = (STRUCT_SW_TX_DESC*)mvOsMalloc(sizeof(STRUCT_SW_TX_DESC) *
                                                       descPerQueue);

        if (firstSwTxDesc == NULL)
        {
            return MV_FAIL;
        }

        for (i = 0; i < descPerQueue; i++)
        {
            swTxDesc = firstSwTxDesc + i;

            swTxDesc->txDesc = (STRUCT_TX_DESC*)descBlock;
            descBlock   = (MV_U8*)((MV_U32)descBlock + sizeOfDesc);

            TX_DESC_RESET(swTxDesc->txDesc);

            if((descPerQueue - 1) != i)
            {
                /* Next descriptor should not be configured for the last one.*/
                swTxDesc->swNextDesc  = firstSwTxDesc + i + 1;

                /* Calc Physical address */
                swTxDesc->txDesc->nextDescPointer = (MV_U32)descBlock -
                          (MV_U32)netCacheDmaBlocks[devNum].txDescBlock +
                          (MV_U32)netCacheDmaBlocks[devNum].txDescBlockPhy;

                swTxDesc->txDesc->nextDescPointer =
                    hwByteSwap(swTxDesc->txDesc->nextDescPointer);
            }

            /* Initilaize the aligment */
            swTxDesc->txBufferAligment = 0;
        }

        /* Close the cyclic desc. list. */
        swTxDesc->swNextDesc = firstSwTxDesc;

        /* Calc Phyiscal address */
        swTxDesc->txDesc->nextDescPointer = (MV_U32)firstTxDesc -
                          (MV_U32)netCacheDmaBlocks[devNum].txDescBlock +
                          (MV_U32)netCacheDmaBlocks[devNum].txDescBlockPhy;

        swTxDesc->txDesc->nextDescPointer =
            hwByteSwap(swTxDesc->txDesc->nextDescPointer);

        txDescList[txQueue].next2Feed   = firstSwTxDesc;
        txDescList[txQueue].next2Free   = firstSwTxDesc;
    }

    for(i = 0; i < NUM_OF_TX_QUEUES; i++)
    {
        /* Calc Phyiscal address */
        phyNext2Feed = (MV_U32)txDescList[i].next2Feed->txDesc -
                       (MV_U32)netCacheDmaBlocks[devNum].txDescBlock +
                       (MV_U32)netCacheDmaBlocks[devNum].txDescBlockPhy;


        CHECK_STATUS(hwIfSetReg(devNum, 0x26c0 + i*0x4, phyNext2Feed));
    }

    /* Posibile HW BTS: TxWordSwap and TxByteSwap are DISABLED  */
    CHECK_STATUS(hwIfSetMaskReg(devNum, 0x2800, (3 << 23), 0));

    coreTxRegConfig(devNum);

    return MV_OK;
}

/*******************************************************************************
* mvPpInitDescRings
*
* DESCRIPTION:
*       Initialized RX Descriptor Ring.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_TRUE     - if xCat2 package type is BGA.
*       MV_FALSE    - if xCat2 package type is QFP.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_STATUS mvPpInitDescRings(MV_U8 dev, MV_ETH_INIT *ethInitP)
{
    MV_U8   *descBlockP;
    MV_U32   descBlockSize;
    MV_U32   numOfDescs;
    MV_U32   physAddr, alignment, delta;

    /*
     * Init RX Desctiptors.
     */
    numOfDescs = ethInitP->rxDescTotal;
    descBlockSize = (numOfDescs + 1 /* for alignment */) * sizeof(STRUCT_RX_DESC);
    descBlockP = mvPpMalloc(descBlockSize, &physAddr, NULL);
    if (descBlockP == NULL)
    {
        mvOsPrintf("%s: alloc failed.\n", __func__);
        return MV_FAIL;
    }
    mvOsMemset(descBlockP, 0, descBlockSize);

    /* Take care of alignment. */
    alignment = RX_DESC_ALIGN;
    delta = (MV_U32)descBlockP % alignment;
    if (delta > 0)
    {
        descBlockP += alignment - delta;
    }

    if (mvPpInitRxDescRing(dev,
                           descBlockP,
                           descBlockSize,
                           numOfDescs) != MV_OK)
    {
        mvOsPrintf("%s: mvPpInitRxDescRing failed.\n", __func__);
        return MV_FAIL;
    }

    /*
     * Init TX Desctiptors.
     */
    numOfDescs = ethInitP->txDescTotal;
    descBlockSize = (numOfDescs + 1 /* for alignment */) * sizeof(STRUCT_RX_DESC);
    descBlockP = mvPpMalloc(descBlockSize, &physAddr, NULL);
    if (descBlockP == NULL)
    {
        mvOsPrintf("%s: alloc failed.\n", __func__);
        return MV_FAIL;
    }
    mvOsMemset(descBlockP, 0, descBlockSize);

    /* Take care of alignment. */
    alignment = TX_DESC_ALIGN;
    delta = (MV_U32)descBlockP % alignment;
    if (delta > 0)
    {
        descBlockP += alignment - delta;
    }

    if (mvPpInitTxDescRing(dev,
                           descBlockP,
                           descBlockSize,
                           numOfDescs) != MV_OK)
    {
        mvOsPrintf("%s: mvPpInitTxDescRing failed.\n", __func__);
        return MV_FAIL;
    }

    return MV_OK;
}

/*******************************************************************************
* mvPpSetSdmaCurrTxDesc
*
* DESCRIPTION:
*       Configure/set Transmit SDMA Current Descriptor pointer.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_TRUE     - success.
*       MV_FALSE    - failure.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_STATUS mvPpSetSdmaCurrTxDesc(MV_U8 dev, MV_U32 txQ, MV_U32 txDescP)
{
    CHECK_STATUS(hwIfSetReg(dev, 0x26c0 + txQ * 0x4, txDescP));

    return MV_OK;
}

/*******************************************************************************
* mvPpInitRxDescRing
*
* DESCRIPTION:
*       Initialized RX Descriptor Ring.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_TRUE     - success.
*       MV_FALSE    - failure.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_STATUS mvPpInitRxDescRing(MV_U8    devNum,
                             MV_U8   *descBlockP,
                             MV_U32   descBlockSize,
                             MV_U32   numOfDescs)
{
    RX_DESC_LIST       *rxDescList;
    STRUCT_RX_DESC     *rxDescP = NULL;
    STRUCT_RX_DESC     *firstRxDesc;
    MV_U32              sizeOfDesc;
    MV_U32              descPerQ;
    STRUCT_SW_RX_DESC  *swRxDescP = NULL;
    STRUCT_SW_RX_DESC  *firstSwRxDesc;
    MV_U32              physAddr;
    MV_U32              rxQ, i;

    rxDescList  = G_sdmaInterCtrl[devNum].rxDescList;

    /* Rx Descriptor list initialization. */
    sizeOfDesc = sizeof(STRUCT_RX_DESC);

    if ((sizeOfDesc % RX_DESC_ALIGN) != 0)
    {
        sizeOfDesc += (RX_DESC_ALIGN - (sizeof(STRUCT_RX_DESC) % RX_DESC_ALIGN));
    }

    descPerQ = numOfDescs / NUM_OF_RX_QUEUES;

    for (rxQ = 0; rxQ < NUM_OF_RX_QUEUES; rxQ++)
    {
        rxDescList[rxQ].rxFreeDescNum = descPerQ;
        rxDescList[rxQ].rxResource    = 0;
        rxDescList[rxQ].forbidQEn     = MV_FALSE;

        /* store pointer to first RX hw desc */
        firstRxDesc = (STRUCT_RX_DESC *)descBlockP;

        /* allocate block for sw rx descriptors */
        firstSwRxDesc = (STRUCT_SW_RX_DESC *)mvOsCalloc(descPerQ, sizeof(STRUCT_SW_RX_DESC));
        if (firstSwRxDesc == NULL)
        {
            mvOsPrintf("%s: alloc failed.\n", __func__);
            return MV_FAIL;
        }

        for (i = 0; i < descPerQ; i++)
        {
            swRxDescP = firstSwRxDesc + i;

            rxDescP      = (STRUCT_RX_DESC*)descBlockP;
            descBlockP   = (MV_U8*)((MV_U32)descBlockP + sizeOfDesc);

            swRxDescP->rxDesc = rxDescP;
            RX_DESC_RESET(swRxDescP->rxDesc);

            if ((descPerQ - 1) != i)
            {
                /* Next descriptor should not be configured for the last one.*/
                swRxDescP->swNextDesc  = (firstSwRxDesc + i + 1);

                physAddr = (MV_U32)mvOsIoVirtToPhy(NULL, (void *)descBlockP);
                physAddr = hwByteSwap(physAddr);
                swRxDescP->rxDesc->nextDescPointer = physAddr;
            }
        }

        /* Close the cyclic desc list. */
        swRxDescP->swNextDesc = firstSwRxDesc;

        /* Calc physical address. */
        physAddr = (MV_U32)mvOsIoVirtToPhy(NULL, (void *)firstRxDesc);
        physAddr = hwByteSwap(physAddr);
        swRxDescP->rxDesc->nextDescPointer = physAddr;

        rxDescList[rxQ].next2Receive    = firstSwRxDesc;
        rxDescList[rxQ].next2Return     = firstSwRxDesc;
    }

    /* disable Rx queues - before enable */
    CHECK_STATUS(hwIfSetReg(devNum, 0x2680, 0xFF00))

    for (rxQ = 0; rxQ < NUM_OF_RX_QUEUES; rxQ++)
    {
        /* Calc Physical address */
        physAddr = (MV_U32)rxDescList[rxQ].next2Receive->rxDesc;
        physAddr = (MV_U32)mvOsIoVirtToPhy(NULL, (void *)physAddr);

        CHECK_STATUS(hwIfSetReg(devNum, 0x260c + rxQ * 0x10, physAddr))
    }

    /* Set the Receive Interrupt Frame Boundaries   */
    CHECK_STATUS(hwIfSetMaskReg(devNum, 0x2800, 1, 1))

    /* Posibile HW BTS: RxWordSwap and RxByteSwap are ENABLED  */
    CHECK_STATUS(hwIfSetMaskReg(devNum, 0x2800, 0xC0, 0xC0))

    return MV_OK;
}

/*******************************************************************************
* mvPpInitTxDescRing
*
* DESCRIPTION:
*       Initialized TX Descriptor Ring.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_TRUE     - success.
*       MV_FALSE    - failure.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_STATUS mvPpInitTxDescRing(MV_U8    dev,
                             MV_U8   *descBlockP,
                             MV_U32   descBlockSize,
                             MV_U32   numOfDescs)
{
    TX_DESC_LIST *txDescList;   /* Points to the relevant Tx desc. list */

    STRUCT_TX_DESC *firstTxDescP;/* Points to the first Tx desc in list. */
    MV_U32 numOfTxDesc;         /* Number of Tx desc. that may be       */
                                /* allocated from the given block.      */
    MV_U32 sizeOfDesc;          /* The ammount of memory (in bytes) that*/
                                /* a single desc. will occupy, including*/
                                /* the alignment.                       */
    MV_U32 descPerQ;        /* Number of descriptors per Tx Queue.  */
    MV_U32 txQ;

    STRUCT_SW_TX_DESC *swTxDescP = NULL;/* Points to the Sw Tx desc to   */
                                /* init.                                */
    STRUCT_SW_TX_DESC *firstSwTxDescP;/* Points to the first Sw Tx desc  */
                                /* in list.                             */
    MV_U32  phyNext2Feed;       /* The physical address of the next2Feed*/
                                /* field.                               */
    MV_U32 physAddr, i;

    txDescList = G_sdmaInterCtrl[dev].txDescList;

    /* Tx Descriptor list initialization.   */
    sizeOfDesc  = sizeof(STRUCT_TX_DESC);
    numOfTxDesc = numOfDescs;
    if((sizeOfDesc % TX_DESC_ALIGN) != 0)
    {
        sizeOfDesc += (TX_DESC_ALIGN-(sizeof(STRUCT_TX_DESC) % TX_DESC_ALIGN));
        numOfTxDesc = (descBlockSize -
                       (TX_DESC_ALIGN -
                        (((MV_U32)descBlockP) % TX_DESC_ALIGN))) / sizeOfDesc;
    }

    /* Set the descBlock to point to an aligned start address. */
    if(((MV_U32)descBlockP % TX_DESC_ALIGN) != 0)
    {
        descBlockP =
            (MV_U8*)((MV_U32)descBlockP +
                     (TX_DESC_ALIGN - (((MV_U32)descBlockP) % TX_DESC_ALIGN)));
    }

    descPerQ = numOfTxDesc / NUM_OF_TX_QUEUES;
    /* Number of descriptors must devide by 2.  */
    descPerQ -= (descPerQ & 0x1);
    txDescList->sizeOfList = descPerQ * sizeOfDesc;

    for (txQ = 0; txQ < NUM_OF_TX_QUEUES; txQ++)
    {
        txDescList[txQ].freeDescNum = descPerQ;

        firstTxDescP = (STRUCT_TX_DESC*)descBlockP;

        firstSwTxDescP = (STRUCT_SW_TX_DESC *)mvOsCalloc(descPerQ, sizeof(STRUCT_SW_TX_DESC));
        if (firstSwTxDescP == NULL)
        {
            mvOsPrintf("%s: alloc failed.\n", __func__);
            return MV_FAIL;
        }

        for (i = 0; i < descPerQ; i++)
        {
            swTxDescP = firstSwTxDescP + i;

            swTxDescP->txDesc = (STRUCT_TX_DESC *)descBlockP;
            descBlockP       = (MV_U8*)((MV_U32)descBlockP + sizeOfDesc);

            TX_DESC_RESET(swTxDescP->txDesc);

            if ((descPerQ - 1) != i)
            {
                /* Next descriptor should not be configured for the last one.*/
                swTxDescP->swNextDesc  = firstSwTxDescP + i + 1;

                physAddr = (MV_U32)mvOsIoVirtToPhy(NULL, (void *)descBlockP);
                physAddr = hwByteSwap(physAddr);
                swTxDescP->txDesc->nextDescPointer = physAddr;
            }

            /* Initilaize the aligment */
            swTxDescP->txBufferAligment = 0;
        }

        /* Close the cyclic desc. list. */
        swTxDescP->swNextDesc = firstSwTxDescP;

        physAddr = (MV_U32)mvOsIoVirtToPhy(NULL, (void *)firstTxDescP);
        physAddr = hwByteSwap(physAddr);
        swTxDescP->txDesc->nextDescPointer = physAddr;

        txDescList[txQ].next2Feed   = firstSwTxDescP;
        txDescList[txQ].next2Free   = firstSwTxDescP;
    }

    for (txQ = 0; txQ < NUM_OF_TX_QUEUES; txQ++)
    {
        /* Calc Phyiscal address */
        phyNext2Feed = (MV_U32)txDescList[txQ].next2Feed->txDesc;
        phyNext2Feed = (MV_U32)mvOsIoVirtToPhy(NULL, (void *)phyNext2Feed);

        if (mvPpSetSdmaCurrTxDesc(dev, txQ, phyNext2Feed) != MV_OK)
        {
            mvOsPrintf("%s: alloc failed.\n", __func__);
            return MV_FAIL;
        }
    }

    /* Posibile HW BTS: TxWordSwap and TxByteSwap are DISABLED  */
    CHECK_STATUS(hwIfSetMaskReg(dev, 0x2800, (3 << 23), 0));

    coreTxRegConfig(dev);

    return MV_OK;
}

/*******************************************************************************
* coreInitRx
*
* DESCRIPTION:
*       This function initializes the Core Rx module, by allocating the cyclic
*       Rx descriptors list, and the rx buffers.
*
* INPUTS:
*       devNum      - The device number to init the Rx unit for.
*       descBlock   - A block to allocate the descriptors from.
*       descBlockSize - Size in bytes of descBlock.
*       buffBlock   - A block to allocate the Rx buffers from.
*       buffBlockSize - Size in bytes of buffBlock.
*       headerOffset - The application required header offset to be kept before
*                      the Rx buffer.
*       buffSize    - Size of a single Rx buffer in list.
*
* OUTPUTS:
*       numOFDescs  - Number of Rx descriptors allocated.
*       numOfBufs   - Number of Rx buffers allocated.
*
* RETURNS:
*       MV_OK on success, or
*       MV_FAIL otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static MV_STATUS coreInitRx(MV_U8    devNum,
                            MV_U8   *descBlock,
                            MV_U32   descBlockSize,
                            MV_U8   *rxBufBlock,
                            MV_U32   rxBufBlockSize,
                            MV_U32   buffSize,
                            MV_U32  *numOfDescs,
                            MV_U32  *numOfBufs)
{
    RX_DESC_LIST *rxDescList;   /* Points to the relevant Rx desc. list */
    STRUCT_RX_DESC *rxDesc = NULL; /* Points to the Rx desc to init.    */
    STRUCT_RX_DESC *firstRxDesc;/* Points to the first Rx desc in list. */
    MV_U8 rxQueue;              /* Index of the Rx Queue.               */
    MV_U32 numOfRxDesc;         /* Number of Rx desc. that may be       */
                                /* allocated from the given block.      */
    MV_U32 sizeOfDesc;          /* The ammount of memory (in bytes) that*/
                                /* a single desc. will occupy, including*/
                                /* the alignment.                       */
    MV_U32 descPerQueue;        /* Number of descriptors per Rx Queue.  */
    MV_U32 actualBuffSize;      /* Size of a single buffer, after taking*/
                                /* the required allignment into account.*/
    STRUCT_SW_RX_DESC *swRxDesc = NULL;/* Points to the Sw Rx desc to   */
                                /* init.                                */
    STRUCT_SW_RX_DESC *firstSwRxDesc;/* Points to the first Sw Rx desc  */
                                /* in list.                             */
    MV_U32 virtBuffAddr;        /* The virtual address of the Rx buffer */
                                /* To be enqueued into the current Rx   */
                                /* Descriptor.                          */
    MV_U32 phyBuffAddr;         /* The physical address of the Rx buffer*/
                                /* To be enqueued into the current Rx   */
                                /* Descriptor.                          */
    MV_U32 phyAddr;             /* Physical Rx descriptor's address.    */

    MV_U32 i;

    rxDescList  = G_sdmaInterCtrl[devNum].rxDescList;

    /* Rx Descriptor list initialization. */
    sizeOfDesc = sizeof(STRUCT_RX_DESC);

    if ((sizeOfDesc % RX_DESC_ALIGN) != 0)
    {
        sizeOfDesc += (RX_DESC_ALIGN -(sizeof(STRUCT_RX_DESC) % RX_DESC_ALIGN));
    }

    /* The buffer size must be a multiple of RX_BUFF_SIZE_MULT  */
    actualBuffSize = buffSize - (buffSize % RX_BUFF_SIZE_MULT);

    /* Number of Rx descriptors is calculated according to the  */
    /* size of the given Rx Buffers block.                      */
    /* Take the "dead" block in head of the buffers block as a  */
    /* result of the allignment.                                */
    if (((MV_U32)descBlock % RX_DESC_ALIGN) != 0)
    {
        numOfRxDesc = (rxBufBlockSize -
                       ((MV_U32)descBlock %RX_DESC_ALIGN)) / actualBuffSize;
    }
    else
    {
        numOfRxDesc = rxBufBlockSize / actualBuffSize;
    }

    /* Set numOfRxDesc according to the number of descriptors that  */
    /* may be allocated from descBlock and the number of buffers    */
    /* that may be allocated from rxBufBlock.                        */
    if((descBlockSize / sizeOfDesc) < numOfRxDesc)
        numOfRxDesc = descBlockSize / sizeOfDesc;

    /* Set the descBlock to point to an aligned start address. */
    if(((MV_U32)descBlock % RX_DESC_ALIGN) != 0)
    {
        descBlock =
            (MV_U8*)((MV_U32)descBlock - (((MV_U32)descBlock) % RX_DESC_ALIGN));
    }

    descPerQueue = numOfRxDesc / NUM_OF_RX_QUEUES;

    *numOfDescs = descPerQueue * NUM_OF_RX_QUEUES;

    for (rxQueue = 0; rxQueue < NUM_OF_RX_QUEUES; rxQueue++)
    {
        rxDescList[rxQueue].rxFreeDescNum = descPerQueue;
        rxDescList[rxQueue].rxResource    = 0;
        rxDescList[rxQueue].forbidQEn     = MV_FALSE;

        /* store pointer to first RX hw desc */
        firstRxDesc = (STRUCT_RX_DESC*)descBlock;

        /* allocate block for sw rx descriptors */
        firstSwRxDesc = (STRUCT_SW_RX_DESC*)mvOsMalloc(sizeof(STRUCT_SW_RX_DESC) *
                                                       descPerQueue);
        if (firstSwRxDesc == NULL)
        {
            mvOsPrintf("%s: firstSwRxDesc is NULL.\n", __func__);
            return MV_FAIL;
        }
        mvOsMemset(firstSwRxDesc, 0, sizeof(STRUCT_SW_RX_DESC) * descPerQueue);

        for (i = 0; i < descPerQueue; i++)
        {
            swRxDesc = firstSwRxDesc + i;

            rxDesc      = (STRUCT_RX_DESC*)descBlock;
            descBlock   = (MV_U8*)((MV_U32)descBlock + sizeOfDesc);
            swRxDesc->rxDesc = rxDesc;
            RX_DESC_RESET(swRxDesc->rxDesc);

            if ((descPerQueue - 1) != i)
            {
                /* Next descriptor should not be configured for the last one.*/
                swRxDesc->swNextDesc  = (firstSwRxDesc + i + 1);

                /* Calc Phyiscal address */
                swRxDesc->rxDesc->nextDescPointer = (MV_U32)descBlock -
                  (MV_U32)netCacheDmaBlocks[devNum].rxDescBlock +
                  (MV_U32)netCacheDmaBlocks[devNum].rxDescBlockPhy;

                DB( printf("*** RX Ring build: Virtual: 0x%x	Physical: 0x%x		\
                            (Virtual base: 0x%x		Physical base: 0x%x)\n" ,
                (MV_U32)descBlock,
                swRxDesc->rxDesc->nextDescPointer,
                (MV_U32)netCacheDmaBlocks[devNum].rxDescBlock,
                (MV_U32)netCacheDmaBlocks[devNum].rxDescBlockPhy) );

                swRxDesc->rxDesc->nextDescPointer =
                    hwByteSwap(swRxDesc->rxDesc->nextDescPointer);
            }
        }

        /* Close the cyclic desc. list. */
        swRxDesc->swNextDesc = firstSwRxDesc;

        /* calc physical address */
        swRxDesc->rxDesc->nextDescPointer = (MV_U32)firstRxDesc -
              (MV_U32)netCacheDmaBlocks[devNum].rxDescBlock +
              (MV_U32)netCacheDmaBlocks[devNum].rxDescBlockPhy;

        swRxDesc->rxDesc->nextDescPointer =
            hwByteSwap(swRxDesc->rxDesc->nextDescPointer);

        rxDescList[rxQueue].next2Receive    = firstSwRxDesc;
        rxDescList[rxQueue].next2Return     = firstSwRxDesc;
    }

    /* Rx Buffers initialization. */

    /* Set the buffers block to point to a properly alligned block. */
    if (((MV_U32)rxBufBlock % RX_BUFF_ALIGN) != 0)
    {
        rxBufBlockSize = (rxBufBlockSize -
                         (RX_BUFF_ALIGN -
                          ((MV_U32)rxBufBlock % RX_BUFF_ALIGN)));

        rxBufBlock =
            (MV_U8*)((MV_U32)rxBufBlock +
                     (RX_BUFF_ALIGN - ((MV_U32)rxBufBlock % RX_BUFF_ALIGN)));
    }

    /* Check if the given buffers block, is large enough to be cut  */
    /* into the needed number of buffers.                           */
    if ((rxBufBlockSize / (descPerQueue * NUM_OF_RX_QUEUES)) < RX_BUFF_SIZE_MULT)
    {
        mvOsPrintf("%s: Too small buffers block.\n", __func__);
        return MV_FAIL;
    }

    *numOfBufs = descPerQueue * NUM_OF_RX_QUEUES;
    for(rxQueue = 0; rxQueue < NUM_OF_RX_QUEUES; rxQueue++)
    {
        swRxDesc = rxDescList[rxQueue].next2Receive;

        for(i = 0; i < descPerQueue; i++)
        {
            RX_DESC_SET_BUFF_SIZE_FIELD(swRxDesc->rxDesc, actualBuffSize);

            /* Set the Rx desc. buff pointer field. */
            virtBuffAddr = (MV_U32)rxBufBlock;
            phyBuffAddr = (MV_U32)rxBufBlock -
                          (MV_U32)netCacheDmaBlocks[devNum].rxBufBlock +
                          (MV_U32)netCacheDmaBlocks[devNum].rxBufBlockPhy;

            /* Calc Physical address */
            swRxDesc->rxDesc->buffPointer = (MV_U32)rxBufBlock -
                (MV_U32)netCacheDmaBlocks[devNum].rxBufBlock +
                (MV_U32)netCacheDmaBlocks[devNum].rxBufBlockPhy;

            swRxDesc->rxDesc->buffPointer =
                hwByteSwap(swRxDesc->rxDesc->buffPointer);

            /* save the virtual address */
            swRxDesc->buffVirtPointer = virtBuffAddr; /* hwByteSwap(virtBuffAddr); */

            rxBufBlock = (MV_U8*)(((MV_U32)rxBufBlock) + actualBuffSize);

            /* Set the buffers block to point to a properly alligned block*/
            if(((MV_U32)rxBufBlock % RX_BUFF_ALIGN) != 0)
            {
                rxBufBlock =
                    (MV_U8*)((MV_U32)rxBufBlock +
                             (RX_BUFF_ALIGN -
                              ((MV_U32)rxBufBlock % RX_BUFF_ALIGN)));
            }

            PP_DESCR_FLUSH_INV(NULL, swRxDesc->rxDesc);
            PP_CACHE_INV((MV_U32*)virtBuffAddr,actualBuffSize);

            swRxDesc = swRxDesc->swNextDesc;
        }
    }

    /* disable Rx queues - before enable */
    CHECK_STATUS(hwIfSetReg(devNum, 0x2680, 0xFF00));

    for (i = 0; i < NUM_OF_RX_QUEUES; i++)
    {
        MV_U32  tmpData;

        /* Calc Physical address */
        phyAddr = (MV_U32)rxDescList[i].next2Receive->rxDesc -
                  (MV_U32)netCacheDmaBlocks[devNum].rxDescBlock +
                  (MV_U32)netCacheDmaBlocks[devNum].rxDescBlockPhy;

        CHECK_STATUS(hwIfSetReg(devNum, 0x260c + i*0x10, phyAddr));

        /* Enable Rx DMA    */
        CHECK_STATUS(hwIfGetReg(devNum, 0x2680, &tmpData));
        tmpData |= (1 << i);
        tmpData &= ~(1 << (i + 8));
        CHECK_STATUS(hwIfSetReg(devNum, 0x2680, tmpData));
    }

    /* Set the Receive Interrupt Frame Boundaries   */
    CHECK_STATUS(hwIfSetMaskReg(devNum, 0x2800, 1, 1));

    /* Posibile HW BTS: RxWordSwap and RxByteSwap are ENABLED  */
    CHECK_STATUS(hwIfSetMaskReg(devNum, 0x2800, 0xC0, 0xC0));

    return MV_OK;
}

/*******************************************************************************
* mvInitSdmaNetIfDev
*
* DESCRIPTION:
*       Initialize the network interface structures, Rx descriptors & buffers
*       and Tx descriptors (For a single device).
*
* INPUTS:
*       devNum  - The device to initialize.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK   - on success,
*       MV_FAIL - otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_STATUS mvInitSdmaNetIfDev(MV_U8       devNum,
                             MV_U32      auDescNum,
                             MV_U32      txDescNum,
                             MV_U32      rxDescNum,
                             MV_U32      rxBufSize)
{
    static MV_BOOL     initDone[SYS_CONF_MAX_DEV] = {MV_FALSE};

    /* Allocate space for the Address update, Rx & Tx descriptors,  */
    /* and for the Rx buffers.                                      */

    if (initDone[devNum] == MV_TRUE)
    {
        return MV_OK;
    }
    netCacheDmaBlocks[devNum].devNum = devNum;

    /* Au blocks size calc & malloc  */
    netCacheDmaBlocks[devNum].auDescBlockSize = sizeof(STRUCT_AU_DESC) * auDescNum;
    netCacheDmaBlocks[devNum].auDescBlock =
    mvPpMalloc(netCacheDmaBlocks[devNum].auDescBlockSize,
                       (MV_U32*)&netCacheDmaBlocks[devNum].auDescBlockPhy,
                    (MV_U32*)&netCacheDmaBlocks[devNum].auDescMemHandle);
    if(netCacheDmaBlocks[devNum].auDescBlock == NULL)
        return MV_FAIL;
    mvOsMemset(netCacheDmaBlocks[devNum].auDescBlock,0,
             netCacheDmaBlocks[devNum].auDescBlockSize);

    netCacheDmaBlocks[devNum].auDescBlockSecond =
    mvPpMalloc(netCacheDmaBlocks[devNum].auDescBlockSize,
                       (MV_U32*)&netCacheDmaBlocks[devNum].auDescBlockSecondPhy,
                    (MV_U32*)&netCacheDmaBlocks[devNum].auDescMemSecondHandle);
    if(netCacheDmaBlocks[devNum].auDescBlockSecond == NULL)
        return MV_FAIL;
    mvOsMemset(netCacheDmaBlocks[devNum].auDescBlockSecond,0,
             netCacheDmaBlocks[devNum].auDescBlockSize);

    /* FDB upload blocks size calc & malloc  */
    netCacheDmaBlocks[devNum].fuDescBlockSize = sizeof(STRUCT_AU_DESC) * auDescNum;
    netCacheDmaBlocks[devNum].fuDescBlock =
    mvPpMalloc(netCacheDmaBlocks[devNum].fuDescBlockSize,
                       (MV_U32*)&netCacheDmaBlocks[devNum].fuDescBlockPhy,
                    (MV_U32*)&netCacheDmaBlocks[devNum].fuDescMemHandle);
    if(netCacheDmaBlocks[devNum].fuDescBlock == NULL)
        return MV_FAIL;
    mvOsMemset(netCacheDmaBlocks[devNum].fuDescBlock,0,
             netCacheDmaBlocks[devNum].fuDescBlockSize);

    netCacheDmaBlocks[devNum].fuDescBlockSecond = 0;

    /* Tx block size calc & malloc  */
    netCacheDmaBlocks[devNum].txDescBlockSize = sizeof(STRUCT_TX_DESC) * txDescNum;
    netCacheDmaBlocks[devNum].txDescBlock = (MV_U32*)
    mvPpMalloc(netCacheDmaBlocks[devNum].txDescBlockSize,
                    (MV_U32*)&netCacheDmaBlocks[devNum].txDescBlockPhy,
                    (MV_U32*)&netCacheDmaBlocks[devNum].txDescMemHandle);
    if(netCacheDmaBlocks[devNum].txDescBlock == NULL)
        return MV_FAIL;

    mvOsMemset(netCacheDmaBlocks[devNum].txDescBlock,0,
             netCacheDmaBlocks[devNum].txDescBlockSize);

    /* Rx block size calc & malloc  */
    netCacheDmaBlocks[devNum].rxDescBlockSize = sizeof(STRUCT_RX_DESC) * rxDescNum;
    netCacheDmaBlocks[devNum].rxDescBlock = (MV_U32*)
    mvPpMalloc(netCacheDmaBlocks[devNum].rxDescBlockSize,
                    (MV_U32*)&netCacheDmaBlocks[devNum].rxDescBlockPhy,
                    (MV_U32*)&netCacheDmaBlocks[devNum].rxDescMemHandle);
    if(netCacheDmaBlocks[devNum].rxDescBlock == NULL)
        return MV_FAIL;
    mvOsMemset(netCacheDmaBlocks[devNum].rxDescBlock,0,
             netCacheDmaBlocks[devNum].rxDescBlockSize);

    DB( printf("*** RX Desc Block alocation: Virtual: 0x%x	Physical: 0x%x	Size: 0x%x\n" ,
                 (MV_U32)netCacheDmaBlocks[devNum].rxDescBlock,
                 (MV_U32)netCacheDmaBlocks[devNum].rxDescBlockPhy,
                 (MV_U32)netCacheDmaBlocks[devNum].rxDescBlockSize) );

    /* Rx buffer malloc */
    netCacheDmaBlocks[devNum].rxBufBlockSize = rxBufSize * rxDescNum;
    netCacheDmaBlocks[devNum].rxBufBlock = (MV_U32*)
    mvPpMalloc(netCacheDmaBlocks[devNum].rxBufBlockSize,
                    (MV_U32*)&netCacheDmaBlocks[devNum].rxBufBlockPhy,
                    (MV_U32*)&netCacheDmaBlocks[devNum].rxBufMemHandle);
    if(netCacheDmaBlocks[devNum].rxBufBlock == NULL)
        return MV_FAIL;
    mvOsMemset(netCacheDmaBlocks[devNum].rxDescBlock,0,
             netCacheDmaBlocks[devNum].rxDescBlockSize);

    DB( printf("*** RX Buf  Block alocation: Virtual: 0x%x	Physical: 0x%x	Size: 0x%x\n" ,
                 (MV_U32)netCacheDmaBlocks[devNum].rxBufBlock,
                 (MV_U32)netCacheDmaBlocks[devNum].rxBufBlockPhy,
                 (MV_U32)netCacheDmaBlocks[devNum].rxBufBlockSize) );

    if (coreInitRx(devNum,
        (MV_U8*)netCacheDmaBlocks[devNum].rxDescBlock,
        netCacheDmaBlocks[devNum].rxDescBlockSize,
        (MV_U8*)netCacheDmaBlocks[devNum].rxBufBlock,
        netCacheDmaBlocks[devNum].rxBufBlockSize,
        rxBufSize, &numRxDescriptors, &numRxDataBufs))
    {
        mvOsPrintf("%s: coreInitTx failed.\n", __func__);
        return MV_FAIL;
    }

    if (coreInitTx(devNum,
        (MV_U8*)netCacheDmaBlocks[devNum].txDescBlock,
        netCacheDmaBlocks[devNum].txDescBlockSize,
        &numTxDescriptors))
    {
        mvOsPrintf("%s: coreInitTx failed.\n", __func__);
        return MV_FAIL;
    }

    initDone[devNum] = MV_TRUE;

    return MV_OK;
}

/*******************************************************************************
* mvNetEnableRxProcess
*
* DESCRIPTION:
*   Enable the RX process.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS :
*       MV_OK        - Operation succeede
*
* COMMENTS:
*
*
*******************************************************************************/
MV_STATUS mvNetEnableRxProcess(MV_VOID)
{
    enableRxProcces = MV_TRUE;
    return MV_OK;
}

/*******************************************************************************
* mvNetGetEnableRxProcess
*
* DESCRIPTION:
*   Get the status of Enable the RX process.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       MV_TRUE  - Enable RX process
*       MV_FALSE - Disable RX process
*
* RETURNS :
*       None
*
* COMMENTS:
*
*
*******************************************************************************/
MV_BOOL mvNetGetEnableRxProcess(MV_VOID)
{
    return enableRxProcces;
}

/*******************************************************************************
* mvBrgTeachNewAddress
*
* DESCRIPTION:
*
* INPUTS:
*       devNum      - Device number
*       updateMsg   - an update message
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       None.
*
* COMMENTS:
*
*******************************************************************************/
MV_STATUS mvBrgTeachNewAddress(MV_U8                    devNum,
                               MAC_TBL_UPDATE_MSG      *updateMsg)
{
    MV_U32      regAddr;
    MV_U32      data[5];
    MV_U32      timeout;
    int i;

    /* clear address update registers before build new message */
    regAddr = 0x6000040;
    mvOsMemset(data, 0, sizeof(data));

    if (updateMsg->macEntryData.entryType == UPDATE_MSG_ENTRY_MAC)
    {
        /* set mac address */
        data[0] |= (MV_HW_MAC_LOW16(&(updateMsg->macEntryData.macAddr)) << 16);
        data[1] |= MV_HW_MAC_HIGH32(&(updateMsg->macEntryData.macAddr));

        /* set devNum */
        data[3] |= ((updateMsg->macEntryData.ownDevNum & 0x1F) << 7);

        /* Multipple = 1, or Mac[40] = 1 */
        if ((updateMsg->macEntryData.multiple == 1) ||
            ((updateMsg->macEntryData.macAddr.arEther[0] & 1) == 1))
        {
            /* set vidx */
            data[2] |= (updateMsg->macEntryData.vidx << 17);
        }
        else
        {
            /* Trunk = 1 */
            if (updateMsg->macEntryData.trunk == 1)
            {
                /* set trunk */
                data[2] |= (updateMsg->macEntryData.trunkNum << 18);
            }
            else
            {
                /* set regular feature */
                data[2] |= (updateMsg->macEntryData.portNum << 18);
            }

            /* set IsTrunk */
            data[2] |= ((updateMsg->macEntryData.trunk & 1) << 17);
        }
        /* set SrcID */
        data[3] |= ((updateMsg->macEntryData.srcId & 0x1F) << 2);

        /* set UserDefined */
        data[2] |= (updateMsg->macEntryData.userDefined << 25);
    }
    else
    {
        /* set dip & sip */
        data[0] |= (updateMsg->macEntryData.dipAddr.arIP[0] << 16);
        data[0] |= (updateMsg->macEntryData.dipAddr.arIP[1] << 24);
        data[1] |= updateMsg->macEntryData.dipAddr.arIP[2];
        data[1] |= (updateMsg->macEntryData.dipAddr.arIP[3] << 8);

        data[1] |= (updateMsg->macEntryData.sipAddr.arIP[0] << 16);
        data[1] |= (updateMsg->macEntryData.sipAddr.arIP[1] << 24);
        data[3] |= updateMsg->macEntryData.sipAddr.arIP[2];
        data[3] |= ((updateMsg->macEntryData.sipAddr.arIP[3] & 0xf) << 8);
        data[3] |= ((updateMsg->macEntryData.sipAddr.arIP[3] >> 4) << 27);

        /* set vidx */
        data[2] |= (updateMsg->macEntryData.vidx << 17);

        /* set search type */
        data[2] |= (updateMsg->searchType << 16);
    }

    /* set multiple */
    data[2] |= (updateMsg->macEntryData.multiple << 15);
    /* set SPUnknown must be set to 0 by CPU */
    data[2] |= (updateMsg->macEntryData.spUnknown << 14);
    /* set Age */
    data[2] |= (updateMsg->macEntryData.age << 13);
    /* set Skip */
    data[2] |= (updateMsg->macEntryData.skip << 12);
    /* set regular feature */
    data[2] |= updateMsg->macEntryData.vid;

    /* set mirror2anal */
    data[3] |= (updateMsg->macEntryData.mirrorToAnalyzer << 31);
    /* set SA_CMD */
    data[3] |= (updateMsg->macEntryData.saCmd << 24);
    /* set DA_CMD */
    data[3] |= (updateMsg->macEntryData.daCmd << 21);

    /* set EntryType */
    data[3] |= (updateMsg->macEntryData.entryType << 19);
    /* set Static */
    data[3] |= (updateMsg->macEntryData.isStatic << 18);

    /* SASecurityLevel */
    data[0] |= (updateMsg->macEntryData.saSecurityLevel << 12);

    /* DASecurityLevel */
    data[0] |= (updateMsg->macEntryData.daSecurityLevel << 1);

    /* set DARoute */
    data[2] |= (updateMsg->daRoute << 30);

    /* set AppSpecificCPUCodeEn */
    data[2] |= (updateMsg->macEntryData.appSpecCpuCodeEn << 29);

    /* set SaQosProfile */
    data[3] |= (updateMsg->macEntryData.saQosProfileIndex << 12);
    /* set DaQosProfile */
    data[3] |= (updateMsg->macEntryData.daQosProfileIndex << 15);


    /* WRITE TO hw */
    for (i = 0 ; i <= 4; i++)
    {
        CHECK_STATUS(hwIfSetReg(devNum, regAddr + i*0x4, data[i]));
    }

    /* trigeer operation in FDB CPU Update Message Control */
    CHECK_STATUS(hwIfSetReg(devNum, 0x06000050, 1));

    /* wait to operation done */
    timeout = 0;
    do
    {
        CHECK_STATUS(hwIfGetReg(devNum, 0x06000050, &(data[0])));
        if ((data[0] & 1) == 0)
        {
            break;
        }
        timeout++;
    } while(timeout < 10);

    if (timeout == 10)
    {
        mvOsPrintf("timeout in teach MAC address.\n");
        return MV_FAIL;
    }
    return MV_OK;
}

/*******************************************************************************
* setCPUAddressInMACTAble
*
* DESCRIPTION:
*
* INPUTS:
*       devId           - Device Id
*       macAddr		- MAC address
*	vid		- VLAN ID
*
* OUTPUTS:
*       None
*
* RETURNS:
*       CMD_OK            - on success.
*       CMD_AGENT_ERROR   - on failure.
*       CMD_FIELD_UNDERFLOW - not enough field arguments.
*       CMD_FIELD_OVERFLOW  - too many field arguments.
*       CMD_SYNTAX_ERROR    - on fail
*
* COMMENTS:
*       None
*
*******************************************************************************/
MV_STATUS setCPUAddressInMACTAble(MV_U8     devNum,
                                  MV_U8    *macAddr,
                                  MV_U32    vid)
{
    MAC_TBL_UPDATE_MSG updMsg;

    mvOsMemset(&updMsg, 0, sizeof(updMsg));

    updMsg.macEntryData.entryType = UPDATE_MSG_ENTRY_MAC;

    /* set mac address */
    createMacAddr(&updMsg.macEntryData.macAddr, macAddr);

    /* set devNum */
    updMsg.macEntryData.ownDevNum = devNum;

    /* set regular feature */
    updMsg.macEntryData.portNum = 63; /* CPU Port */
    /* set regular feature */
    updMsg.macEntryData.vid = vid;

    /* set SA_CMD */
    updMsg.macEntryData.saCmd = 0; /* Forward */

    /* set DA_CMD */
    updMsg.macEntryData.daCmd = 2; /* Trap to CPU */

    /* set Static */
    updMsg.macEntryData.isStatic = 1;

    /* set UserDefined */
    updMsg.macEntryData.userDefined = 0;

    /* set SaQosProfile */
    updMsg.macEntryData.saQosProfileIndex = 0;
    /* set DaQosProfile */
    updMsg.macEntryData.daQosProfileIndex = 0;

    CHECK_STATUS(mvBrgTeachNewAddress(devNum, &updMsg));

    return MV_OK;
}

/*******************************************************************************
* mvSwitchRxStart
*
* DESCRIPTION:
*       This function handles packet receive interrupts.
*
* INPUTS:
*       devNum      - Device number
*       rxQ         - Q ID
*
* OUTPUTS:
*       pktInfo     - Packet Data
*
* RETURNS:
*       MV_REDO     - if more interrupts of this type need to be handled,
*       MV_OK       - Otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_PKT_INFO *mvSwitchRxStart(MV_U8 devNum, MV_U8 rxQ, MV_PKT_INFO *pktInfo)
{
    RX_DESC_LIST        *rxDescList;
    STRUCT_SW_RX_DESC   *descPtr;
    MV_BOOL              enableProcessRxMsg;
    STRUCT_SW_RX_DESC   *firstRxDesc = NULL;/* Points to the first desc to*/
                                    /* be sent to the callback function.*/
    MV_U32 descNum;                 /* Number of descriptors this packet*/
                                    /* occupies to be sent to the       */
                                    /* callback function.               */

    rxDescList = &G_sdmaInterCtrl[devNum].rxDescList[rxQ];

    DB(mvOsPrintf( "%s - Start\n", __func__) );

    /* Check if process of RX messages is enabled.  */
    enableProcessRxMsg = mvNetGetEnableRxProcess();

    if (rxDescList->rxResource == 0)
    {
        mvOsPrintf("%s: rxDescList->rxResource = 0.\n", __func__);
        return NULL;
    }

    descPtr = rxDescList->next2Receive;
    PP_DESCR_INV(NULL, (descPtr->rxDesc));

    MV_ERRATA_APPLY(mvErrataCpuHandleGet(), MV_ERRATA_ID_SPEC_I_PREFETCH,
                    NULL, (MV_U8 *)descPtr->rxDesc, sizeof(STRUCT_RX_DESC));

    descPtr->shadowRxDesc.word1 = hwByteSwap(descPtr->rxDesc->word1);
    descPtr->shadowRxDesc.word2 = hwByteSwap(descPtr->rxDesc->word2);

    /* No more Packets to process, return. */
    if (RX_DESC_GET_OWN_BIT(&(descPtr->shadowRxDesc)) != MV_OWNERSHIP_CPU)
    {
        PP_DESCR_INV(NULL, (descPtr->rxDesc));
        return NULL;
    }

    if (RX_DESC_GET_FIRST_BIT(&(descPtr->shadowRxDesc)) == 0x0)
    {
        mvOsPrintf("%s: No first bit.\n", __func__);
        return NULL;
    }

    descNum     = 1;
    firstRxDesc = descPtr;

    if (RX_DESC_GET_REC_ERR_BIT(&(descPtr->shadowRxDesc)) == 1)
    {
        mvOsPrintf("%s:%d: ERR_BIT is on.\n", __func__, __LINE__);
        return NULL;
    }

    /* Get the packet's descriptors.        */
    while (RX_DESC_GET_LAST_BIT(&(descPtr->shadowRxDesc)) == 0)
    {
        descPtr     = descPtr->swNextDesc;
        MV_ERRATA_APPLY(mvErrataCpuHandleGet(), MV_ERRATA_ID_SPEC_I_PREFETCH,
                        NULL, (MV_U8 *)descPtr->rxDesc, sizeof(STRUCT_RX_DESC));
        descPtr->shadowRxDesc.word1 = hwByteSwap(descPtr->rxDesc->word1);
        descPtr->shadowRxDesc.word2 = hwByteSwap(descPtr->rxDesc->word2);
        descNum++;

        /* If enable RX process if disabled then the descriptor is cleared */
        if (enableProcessRxMsg == MV_FALSE)
        {
            MV_SYNC;
            RX_DESC_RESET(descPtr->rxDesc);
            MV_SYNC;
        }

        if(RX_DESC_GET_REC_ERR_BIT(&(descPtr->shadowRxDesc)) == 1)
        {
            mvOsPrintf("%s:%d: ERR_BIT is on.\n", __func__, __LINE__);
            return NULL;
        }
    }

    /* If enable RX process if disabled then the descriptor is cleared */
    if (enableProcessRxMsg == MV_FALSE)
    {
        MV_SYNC;
        RX_DESC_RESET(rxDescList->next2Return->rxDesc);
        MV_SYNC;

        /* Updating the next to free pointer */
        rxDescList->next2Return = descPtr->swNextDesc;
    }

    rxDescList->next2Receive = descPtr->swNextDesc;

    if (enableProcessRxMsg == MV_TRUE)
    {
        rxDescList->rxFreeDescNum  -= descNum;
        rxDescList->rxResource     += descNum;
    }

    if (enableProcessRxMsg == MV_TRUE)
    {
        if (mvReceivePacket(devNum, rxQ, firstRxDesc, descNum, pktInfo) != MV_OK)
        {
            mvOsPrintf("%s: mvReceivePacket failed.\n", __func__);
            return NULL;
        }
    }

    return pktInfo;
}

/*******************************************************************************
* mvReceivePacket
*
* DESCRIPTION:
*       This function receives a packet descriptor from the Rx interrupt handler
*       and passes it to the Tapi Rx handler (tapiIntRxHandler()).
*
* INPUTS:
*       devNum	    - Device number
*       rxQ    - Q ID
*       swRxDesc    - A pointer to the first Sw Rx desc. of the packet.
*       descNum     - Number of descriptors (buffers) this packet occupies.
*
* OUTPUTS:
*       pktInfo     - Packet Data
*
* RETURNS:
*       MV_OK if successful, or
*       MV_FAIL otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_STATUS mvReceivePacket(MV_U8                 devNum,
                          MV_U8                 rxQ,
                          STRUCT_SW_RX_DESC    *swRxDesc,
                          MV_U32                descNum,
                          MV_PKT_INFO          *pktInfoP)
{
    MV_U32  buffLen[RX_MAX_PACKET_DESC];    /* List of buffers lengths  */
    MV_U32  packetLen;                      /* Length of packet in bytes*/
    MV_U32  bufferSize;                     /* Size of a single buffer. */
    MV_U32  numOfBufs;                      /* Real number of buffers   */
    MV_BOOL isLast;                         /* Last descriptor          */
    MV_U32  i;
    MV_RX_DESC_DATA rxDesc;
    MV_BOOL         rxExtDataAdded;
    MV_U8          *lastBuffP = NULL;

    DB(mvOsPrintf("%s - Start\n", __func__));

    /* Filling the unit and device the interrupt came on  */
    rxDesc.intDev  = devNum;

    /* Filling the descriptor parameters that are vaild only in the first     */
    /* descriptor in the packet                                               */
    rxDesc.word1 = swRxDesc->shadowRxDesc.word1;
    rxDesc.word2 = swRxDesc->shadowRxDesc.word2;

    rxDesc.rxDesc.busError = RX_DESC_GET_BUS_ERR_BIT(&(swRxDesc->shadowRxDesc));
    rxDesc.rxDesc.byteCount =
                        RX_DESC_GET_BYTE_COUNT_FIELD(&(swRxDesc->shadowRxDesc));
    rxDesc.rxDesc.resourceError =
                        RX_DESC_GET_REC_ERR_BIT(&(swRxDesc->shadowRxDesc));
    rxDesc.rxDesc.validCrc =
    (RX_DESC_GET_VALID_CRC_BIT(&(swRxDesc->shadowRxDesc)) ? MV_FALSE : MV_TRUE);

    /* Clearing the number of buffers in the packet */
    numOfBufs   = 0;

    /* Check that the number of descriptors in this     */
    /* packet is not greater than RX_MAX_PACKET_DESC.   */
    if (descNum > RX_MAX_PACKET_DESC)
    {
        mvOsPrintf("%s: descNum(%d) > RX_MAX_PACKET_DESC(%d).\n",
                   __func__, descNum, RX_MAX_PACKET_DESC);

        /* Free all buffers */
        for (i = 0; i < descNum; i++)
        {
            if (mvFreeRxBuf((MV_U8 *)swRxDesc->buffVirtPointer,
                            0,
                            rxDesc.intDev,
                            rxQ) != MV_OK)
            {
                mvOsPrintf("%s: mvFreeRxBuf failed.\n", __func__);
                return MV_FAIL;
            }
            swRxDesc  = swRxDesc->swNextDesc;
        }
        return MV_OK;
    }

    packetLen  = rxDesc.rxDesc.byteCount;

    /* Setting the extended data to be invalid */
    /*rxDesc.extDataValid = MV_FALSE;*/

    /* Get the SDMA mode. If there were added 2 words of extended RX data */
    /*rc = coreGetRxSdmaMode(&rxExtDataAdded);*/
    rxExtDataAdded = MV_FALSE;

    pktInfoP->pktSize  = 0;

    /* Gathering all the data from all the descriptors */
    for (i = 0; i < descNum; i++)
    {
        /* Get the buffer size of the current descriptor */
        bufferSize = RX_DESC_GET_BUFF_SIZE_FIELD_NO_SWAP(&(swRxDesc->shadowRxDesc));

        /* Checking if this is the last descriptor */
        isLast = ((i == (descNum - 1)) ? MV_TRUE : MV_FALSE);

        /* Set the length of each buffer */
        if (packetLen > bufferSize)
        {
            buffLen[i] = bufferSize;
        }
        else
        {
            buffLen[i] = packetLen;
        }

        /* Descrementing the packet's length */
        packetLen -= buffLen[i];

        /* Update packet structure */
        if (buffLen[i] > 0)
        {
            MV_ERRATA_APPLY(mvErrataCpuHandleGet(), MV_ERRATA_ID_SPEC_I_PREFETCH,
                            NULL, (MV_U8 *)swRxDesc->buffVirtPointer,
                            buffLen[i]);
        }
        pktInfoP->pFrags[i].bufPhysAddr = hwByteSwap(swRxDesc->rxDesc->buffPointer);
        pktInfoP->pFrags[i].bufVirtPtr = (MV_U8*)swRxDesc->buffVirtPointer;
        pktInfoP->pFrags[i].dataSize = buffLen[i];
        pktInfoP->pktSize += buffLen[i];

        lastBuffP = (MV_U8*)swRxDesc->buffVirtPointer;

        /* If this is the last buffer then we get the extended data  */
        numOfBufs++;
        swRxDesc  = swRxDesc->swNextDesc;
    }

    /* If the buffer length of the last descriptor in 0 then it is freed */
    if (buffLen[descNum - 1] == 0 && lastBuffP != NULL)
    {
        if (mvFreeRxBuf(lastBuffP,
                        0,
                        rxDesc.intDev, rxQ) != MV_OK)
        {
            mvOsPrintf("%s: mvFreeRxBuf failed.\n", __func__);
            return MV_FAIL;
        }

        numOfBufs--;
    }

    return MV_OK;
}

#define toupper(c)  c

static const char hexcode[] = "0123456789ABCDEF";

/*******************************************************************************
* createMacAddr
*
* DESCRIPTION:
*       Create Ethernet MAC Address from hexadecimal coded string
*       6 elements, string size = 17 bytes
*       MAC address is on te format of  XX:XX:XX:XX:XX:XX
*
* INPUTS:
*       source - hexadecimal coded string reference
*
* OUTPUTS:
*       dest   - pointer to MV_ETHERADDR structure
*
* RETURNS:
*       none
*
* COMMENTS:
*       no assertion is performed on validity of coded string
*
*******************************************************************************/
MV_VOID createMacAddr(MV_ETHERADDR *dest, MV_U8 *source)
{
    int i,j;

    /* Remove ':' from MAC affress */
    MV_8 enet_addr[6*2+1];

    for( i = 0, j = 0;i < 6*2+1 ; j++ ){
        if( source[j] != ':' ) {
            enet_addr[i] = source[j];
            i++;
        }
    }

    mvStrToMac(enet_addr, (MV_8*)dest);
}

/***********************************************************
 * string helpers for mac address setting                  *
 ***********************************************************/
MV_VOID mvStrToMac(MV_8 *source, MV_8 *dest)
{
    dest[0] = (mvStrToHex( source[0] ) << 4) + mvStrToHex( source[1] );
    dest[1] = (mvStrToHex( source[2] ) << 4) + mvStrToHex( source[3] );
    dest[2] = (mvStrToHex( source[4] ) << 4) + mvStrToHex( source[5] );
    dest[3] = (mvStrToHex( source[6] ) << 4) + mvStrToHex( source[7] );
    dest[4] = (mvStrToHex( source[8] ) << 4) + mvStrToHex( source[9] );
    dest[5] = (mvStrToHex( source[10] ) << 4) + mvStrToHex( source[11] );
}

MV_U32 mvStrToHex(MV_8 ch)
{
    if( (ch >= '0') && (ch <= '9') ) return( ch - '0' );
    if( (ch >= 'a') && (ch <= 'f') ) return( ch - 'a' + 10 );
    if( (ch >= 'A') && (ch <= 'F') ) return( ch - 'A' + 10 );

    return 0;
}

/*******************************************************************************
* mvPresteraMibCounterRead - Read a MIB counter
*
* DESCRIPTION:
*       This function reads a MIB counter of a specific ethernet port.
*       NOTE - Read from PRESTERA_MIB_GOOD_OCTETS_RECEIVED_LOW or
*              PRESTERA_MIB_GOOD_OCTETS_SENT_LOW counters will return 64 bits value,
*              so pHigh32 pointer should not be NULL in this case.
*
* INPUT:
*       int           port  	  - Ethernet Port number.
*       unsigned int  mibOffset   - MIB counter offset.
*
* OUTPUT:
*       MV_U32*       pHigh32 - pointer to place where 32 most significant bits
*                             of the counter will be stored.
*
* RETURN:
*       32 low sgnificant bits of MIB counter value.
*
*******************************************************************************/
MV_U32  mvPresteraMibCounterRead(int devNum, int portNum, unsigned int mibOffset,
                                                        MV_U32* pHigh32)
{
    MV_U32          valLow32, valHigh32;

    if( mvSwitchReadReg(devNum, PRESTERA_MIB_REG_BASE(portNum) + mibOffset,
            &valLow32) != MV_OK)
   {
        return MV_FALSE;
   }

    /* Implement FEr ETH. Erroneous Value when Reading the Upper 32-bits    */
    /* of a 64-bit MIB Counter.                                             */
    if( (mibOffset == PRESTERA_MIB_GOOD_OCTETS_RECEIVED_LOW) ||
        (mibOffset == PRESTERA_MIB_GOOD_OCTETS_SENT_LOW) )
    {
    if( mvSwitchReadReg(devNum, PRESTERA_MIB_REG_BASE(portNum) + mibOffset,
            &valHigh32) != MV_OK)
       {
        return MV_FALSE;
       }

        if(pHigh32 != NULL)
            *pHigh32 = valHigh32;
    }
    return valLow32;
}


/*******************************************************************************
 * mvPpReadPortMibCntCpuPort
 */
void mvPpReadPortMibCntCpuPort(MV_U32 dev)
{
    MV_U32 regVal, tmp;

    /*
     * Check if CPU_Port is active.
     */
    regVal = mvPpReadReg(dev, 0xA0);
    tmp = regVal & 1;
    if (tmp == 0)
    {
        mvOsPrintf("CPU_Port is inactive (CPUPortActive = 0).\n");
        return;
    }

    mvOsPrintf("CPU Port GoodFramesSent           = %d\n",
               mvPpReadReg(dev, 0x60));
    mvOsPrintf("CPU Port MACTransErrorFramesSent  = %d\n",
               mvPpReadReg(dev, 0x64));
    mvOsPrintf("CPU Port GoodOctetsSent           = %d\n",
               mvPpReadReg(dev, 0x68));
    mvOsPrintf("CPU Port Rx Internal Drop         = %d\n",
               mvPpReadReg(dev, 0x6C));
    mvOsPrintf("CPU Port GoodFramesReceived       = %d\n",
               mvPpReadReg(dev, 0x70));
    mvOsPrintf("CPU Port BadFramesReceived        = %d\n",
               mvPpReadReg(dev, 0x74));
    mvOsPrintf("CPU Port GoodOctetsReceived       = %d\n",
               mvPpReadReg(dev, 0x78));
    mvOsPrintf("CPU Port BadOctetsReceived        = %d\n",
               mvPpReadReg(dev, 0x7C));
    mvOsPrintf("CPU Port Global Configuration     = %d\n",
               mvPpReadReg(dev, 0xA0));
}

/*******************************************************************************
 * mvPpReadStackingPortMibCounters
 */
void mvPpReadStackingPortMibCounters(MV_U32 dev, MV_U32 port)
{
    MV_U32 mibBase = 0x09300000 + (port - 24) * 0x20000;
    MV_U32 val, high;

    if (port < 24 || port > 27)
    {
        mvOsPrintf("%s: Wrong port number (%d).\n", __func__, port);
        return;
    }

    mvOsPrintf("Reading Stacking Port counters (Port %d Device %d)\n\n", port, dev);
    mvOsPrintf("Port MIB base address: 0x%08x\n", mibBase);

    val = mvPpReadReg(dev, mibBase);
    high = mvPpReadReg(dev, mibBase + 0x4);
    mvOsPrintf("GoodOctetsReceived          = 0x%08x%08x\n", high, val);

    val = mvPpReadReg(dev, mibBase + 0x8);
    mvOsPrintf("BadOctetsReceived           = %u\n", val);

    val = mvPpReadReg(dev, mibBase + 0xC);
    mvOsPrintf("CRCErrorsSent               = %u\n", val);

    val = mvPpReadReg(dev, mibBase + 0x10);
    mvOsPrintf("GoodUnicastFramesReceived   = %u\n", val);

    val = mvPpReadReg(dev, mibBase + 0x14);
    mvOsPrintf("Reserved                    = %u\n", val);

    val = mvPpReadReg(dev, mibBase + 0x18);
    mvOsPrintf("BroadcastFramesReceived     = %u\n", val);

    val = mvPpReadReg(dev, mibBase + 0x1C);
    mvOsPrintf("MulticastFramesReceived     = %u\n", val);

    val = mvPpReadReg(dev, mibBase + 0x20);
    mvOsPrintf("Frames64 Octets             = %u\n", val);

    val = mvPpReadReg(dev, mibBase + 0x24);
    mvOsPrintf("Frames65to127 Octets        = %u\n", val);

    val = mvPpReadReg(dev, mibBase + 0x28);
    mvOsPrintf("Frames128to255 Octets       = %u\n", val);

    val = mvPpReadReg(dev, mibBase + 0x2C);
    mvOsPrintf("Frames256to511 Octets       = %u\n", val);

    val = mvPpReadReg(dev, mibBase + 0x30);
    mvOsPrintf("Frames512to1023 Octets      = %u\n", val);
    
    val = mvPpReadReg(dev, mibBase + 0x34);
    mvOsPrintf("Frames1024toMax Octets      = %u\n", val);
    
    val = mvPpReadReg(dev, mibBase + 0x38);
    high = mvPpReadReg(dev, mibBase + 0x3C);
    mvOsPrintf("GoodOctetsSent              = 0x%08x%08x\n", high, val);
    
    val = mvPpReadReg(dev, mibBase + 0x40);
    mvOsPrintf("UnicastFramesSent           = %u\n", val);
    
    val = mvPpReadReg(dev, mibBase + 0x44);
    mvOsPrintf("Reserved                    = %u\n", val);
    
    val = mvPpReadReg(dev, mibBase + 0x48);
    mvOsPrintf("MulticastFramesSent         = %u\n", val);
    
    val = mvPpReadReg(dev, mibBase + 0x4C);
    mvOsPrintf("BroadcastFramesSent         = %u\n", val);
    
    val = mvPpReadReg(dev, mibBase + 0x50);
    mvOsPrintf("Reserved                    = %u\n", val);
    
    val = mvPpReadReg(dev, mibBase + 0x54);
    mvOsPrintf("FlowControlSent             = %u\n", val);

    val = mvPpReadReg(dev, mibBase + 0x58);
    mvOsPrintf("FlowControlReceived         = %u\n", val);

    val = mvPpReadReg(dev, mibBase + 0x5C);
    mvOsPrintf("ReceivedFIFOOverrun         = %u\n", val);

    val = mvPpReadReg(dev, mibBase + 0x60);
    mvOsPrintf("Undersize                   = %u\n", val);

    val = mvPpReadReg(dev, mibBase + 0x64);
    mvOsPrintf("Fragments                   = %u\n", val);

    val = mvPpReadReg(dev, mibBase + 0x68);
    mvOsPrintf("Oversize                    = %u\n", val);

    val = mvPpReadReg(dev, mibBase + 0x6C);
    mvOsPrintf("Jabber                      = %u\n", val);

    val = mvPpReadReg(dev, mibBase + 0x70);
    mvOsPrintf("RxErrorFrameReceived        = %u\n", val);

    val = mvPpReadReg(dev, mibBase + 0x74);
    mvOsPrintf("BadCRC                      = %u\n", val);

    val = mvPpReadReg(dev, mibBase + 0x78);
    mvOsPrintf("Reserved                    = %u\n", val);

    val = mvPpReadReg(dev, mibBase + 0x7C);
    mvOsPrintf("Reserved                    = %u\n", val);
}

/*******************************************************************************
 * mvPresteraReadPortMibCounters
 */
void mvPresteraReadPortMibCounters(int port)
{
    MV_U32  regValue, regValHigh;
    MV_U8   devNum = port / 28;
    MV_U32  portNum = port % 28;

    if (port == PP_CPU_PORT_NUM)
    {
        mvOsPrintf("CPU Port is a special port:\n");
        mvPpReadPortMibCntCpuPort(PP_DEV0);
        return;
    }

    if (portNum == 24 || portNum == 25 || portNum == 26 || portNum == 27)
    {
        mvPpReadStackingPortMibCounters(devNum, portNum);
        return;
    }

    printf("\n\t Port #%d MIB Counters (Port %d Device %d)\n\n",port, portNum, devNum);
    printf("Port MIB base address: 0x%08x\n",PRESTERA_MIB_REG_BASE(portNum));

    printf("GoodFramesReceived          = %u\n",
              mvPresteraMibCounterRead(devNum, portNum, PRESTERA_MIB_GOOD_FRAMES_RECEIVED, NULL));
    printf("BroadcastFramesReceived     = %u\n",
              mvPresteraMibCounterRead(devNum, portNum, PRESTERA_MIB_BROADCAST_FRAMES_RECEIVED, NULL));
    printf("MulticastFramesReceived     = %u\n",
              mvPresteraMibCounterRead(devNum, portNum, PRESTERA_MIB_MULTICAST_FRAMES_RECEIVED, NULL));

    regValue = mvPresteraMibCounterRead(devNum, portNum, PRESTERA_MIB_GOOD_OCTETS_RECEIVED_LOW,
                                 &regValHigh);
    printf("GoodOctetsReceived          = 0x%08x%08x\n",
               regValHigh, regValue);

    printf("\n");
    printf("GoodFramesSent              = %u\n",
              mvPresteraMibCounterRead(devNum, portNum, PRESTERA_MIB_GOOD_FRAMES_SENT, NULL));
    printf("BroadcastFramesSent         = %u\n",
              mvPresteraMibCounterRead(devNum, portNum, PRESTERA_MIB_BROADCAST_FRAMES_SENT, NULL));
    printf("MulticastFramesSent         = %u\n",
              mvPresteraMibCounterRead(devNum, portNum, PRESTERA_MIB_MULTICAST_FRAMES_SENT, NULL));

    regValue = mvPresteraMibCounterRead(devNum, portNum, PRESTERA_MIB_GOOD_OCTETS_SENT_LOW,
                                 &regValHigh);
    printf("GoodOctetsSent              = 0x%08x%08x\n", regValHigh, regValue);

    regValue = mvPresteraMibCounterRead(devNum, portNum, PRESTERA_MIB_SENT_MULTIPLE, NULL);
    printf("SentMultiple                = %u\n", regValue);

    printf("SentDeferred                = %u\n",
              mvPresteraMibCounterRead(devNum, portNum, PRESTERA_MIB_SENT_DEFERRED, NULL));

    printf("\n\t FC Control Counters\n");

    regValue = mvPresteraMibCounterRead(devNum, portNum, PRESTERA_MIB_GOOD_FC_RECEIVED, NULL);
    printf("GoodFCFramesReceived        = %u\n", regValue);

    regValue = mvPresteraMibCounterRead(devNum, portNum, PRESTERA_MIB_RECEIVED_FIFO_OVERRUN, NULL);
    printf("ReceivedFifoOverrun         = %u\n", regValue);

    regValue = mvPresteraMibCounterRead(devNum, portNum, PRESTERA_MIB_FC_SENT, NULL);
    printf("FCFramesSent                = %u\n", regValue);


    printf("\n\t RX Errors\n");

    regValue = mvPresteraMibCounterRead(devNum, portNum, PRESTERA_MIB_BAD_OCTETS_RECEIVED, NULL);
    printf("BadOctetsReceived           = %u\n", regValue);

    regValue = mvPresteraMibCounterRead(devNum, portNum, PRESTERA_MIB_UNDERSIZE_RECEIVED, NULL);
    printf("UndersizeFramesReceived     = %u\n", regValue);

    regValue = mvPresteraMibCounterRead(devNum, portNum, PRESTERA_MIB_FRAGMENTS_RECEIVED, NULL);
    printf("FragmentsReceived           = %u\n", regValue);

    regValue = mvPresteraMibCounterRead(devNum, portNum, PRESTERA_MIB_OVERSIZE_RECEIVED, NULL);
    printf("OversizeFramesReceived      = %u\n", regValue);

    regValue = mvPresteraMibCounterRead(devNum, portNum, PRESTERA_MIB_JABBER_RECEIVED, NULL);
    printf("JabbersReceived             = %u\n", regValue);

    regValue = mvPresteraMibCounterRead(devNum, portNum, PRESTERA_MIB_RX_ERROR_FRAME_RECEIVED, NULL);
    printf("RxErrorFrameReceived        = %u\n", regValue);

    /*regValue = mvPresteraMibCounterRead(devNum, portNum, PRESTERA_MIB_BAD_CRC_EVENT, NULL);
    printf("BadCrcReceived              = %u\n", regValue);*/

    printf("\n\t TX Errors\n");

    regValue = mvPresteraMibCounterRead(devNum, portNum, PRESTERA_MIB_TX_FIFO_UNDERRUN_AND_CRC, NULL);
    printf("TxFifoUnderrunAndCRC        = %u\n", regValue);

    regValue = mvPresteraMibCounterRead(devNum, portNum, PRESTERA_MIB_EXCESSIVE_COLLISION, NULL);
    printf("TxExcessiveCollisions       = %u\n", regValue);

    /*regValue = mvPresteraMibCounterRead(devNum, portNum, PRESTERA_MIB_COLLISION, NULL);
    printf("TxCollisions                = %u\n", regValue);

    regValue = mvPresteraMibCounterRead(devNum, portNum, PRESTERA_MIB_LATE_COLLISION, NULL);
    printf("TxLateCollisions            = %u\n", regValue);*/

    printf("\n");
    /*regValue = MV_REG_READ( PRESTERA_RX_DISCARD_PKTS_CNTR_REG(port));
    printf("Rx Discard packets counter    = %u\n", regValue);

    regValue = MV_REG_READ(PRESTERA_RX_OVERRUN_PKTS_CNTR_REG(port));
    printf("Rx Overrun packets counter  = %u\n", regValue);*/
}

void readPort0MIBCounters(int devNum)
{
    MV_U32 countersBaseAddr = 0x04010000, offset, regVal;
    for(offset = countersBaseAddr; offset < countersBaseAddr+0x80; offset+=4)
    {
        CHK_STS_VOID(mvSwitchReadReg(devNum, offset, &regVal));
        mvOsPrintf("Address: 0x%08x \tValue: 0x%08x\n",offset,regVal);
    }
}

/*******************************************************************************
* mvPpMalloc
*
* INPUT:
*       size - size of memory should be allocated.
*
* RETURN: None
*
*******************************************************************************/
void* mvPpMalloc(MV_U32  size,
                 MV_U32 *pPhysAddr,
                 MV_U32 *pMemHandle)
{
#ifdef PRESTERA_CACHED
    return mvOsIoCachedMalloc  (NULL, size, (MV_ULONG *)pPhysAddr, pMemHandle);
#else
    return mvOsIoUncachedMalloc(NULL, size, (MV_ULONG *)pPhysAddr, pMemHandle);
#endif
}

/*******************************************************************************
* mvPpFree
*
* INPUT:
*
* RETURN: None
*
*******************************************************************************/
void mvPpFree(MV_U32  size,
              MV_U32 *phyAddr,
              void   *pVirtAddr,
              MV_U32 *memHandle)
{
#ifdef PRESTERA_CACHED
    mvOsIoCachedFree  (NULL, size, (MV_ULONG)phyAddr, pVirtAddr, (MV_U32)memHandle);
#else
    mvOsIoUncachedFree(NULL, size, (MV_ULONG)phyAddr, pVirtAddr, (MV_U32*)memHandle);
#endif
}

#define VLAN_READ_TIMEOUT 1000

/****************************************
 * VLAN Table handeling functions.
 */

#define PRESTERA_VLAN_DIRECT

#if defined PRESTERA_VLAN_INDIRECT
    #define VLT_CTRL_REG_OFFSET	0xA000118
    #define VLT_DATA_REG_OFFSET	0xA000000
    #define VLAN_ENTRY_SIZE		0x10
#elif defined PRESTERA_VLAN_DIRECT
    #define VLT_CTRL_REG_OFFSET	0xA000118 /*not in use in direct access*/
    #define VLT_DATA_REG_OFFSET	0xA200000
    #define VLAN_ENTRY_SIZE		0x20
#else
    #error "Prestera Hal: VLAN table access isn't defined!\n"
#endif

#define VLT_ENTRY_BIT               0
#define VLT_TRIGGER_BIT             15
#define VLT_OPERATION_BIT           12

#define CPU_VLAN_MEMBER_BIT         2
#define IPv4_CTRL_TO_CPU_ENABLE     18
#define UNREGISTERED_IPv4_BC        18
#define UNREGISTERED_NON_IPv4_BC    21
#define PORT24_VLAN_MEMBER_BIT      72
#define PORT25_VLAN_MEMBER_BIT      74
#define PORT26_VLAN_MEMBER_BIT      76
#define PORT27_VLAN_MEMBER_BIT      104
#define UNKNOWN_UNICATS_CMD_BIT     12

/*******************************************************************************
* setVLANTableCtrlReg
*
* DESCRIPTION:
*       	Sets the access control register in order to
* 		read/write VLAN Table entries
*
* INPUTS:
*       entryNum - VLAN Table entry number
*	operation- 0 - Read 1 - Write
*
* RETURNS:
*
*
*******************************************************************************/
MV_STATUS setVLANTableCtrlReg(int devNum, int entryNum, int operation)
{
    MV_U32 timeout = 0;
    MV_U32 ctrlRegVal = entryNum << VLT_ENTRY_BIT |
                operation << VLT_OPERATION_BIT |
                1 << VLT_TRIGGER_BIT;

    DB( printf( "%s: \n", __FUNCTION__) );
    /* Write VLT Table Access Control register */
    if (mvSwitchWriteReg(devNum, VLT_CTRL_REG_OFFSET, ctrlRegVal)!=MV_OK) {
        DB( printf( "%s: Error: Problem writing register 0x%08x\n",
                    __FUNCTION__, VLT_CTRL_REG_OFFSET) );
        return MV_FAIL;
    }

    while((ctrlRegVal & (1 << VLT_TRIGGER_BIT)) && timeout < VLAN_READ_TIMEOUT)
    {
        if (mvSwitchReadReg(devNum,VLT_CTRL_REG_OFFSET, &ctrlRegVal)!=MV_OK) {
            DB( printf( "%s: Error: Problem reading register 0x%08x\n",
                    __FUNCTION__, VLT_CTRL_REG_OFFSET) );
            return MV_FAIL;
        }
        timeout++;
    }

    return (timeout==VLAN_READ_TIMEOUT) ? MV_FAIL : MV_OK;
}

MV_STATUS readVLANEntry(int devNum, int entryNum, STRUCT_VLAN_ENTRY* vlanTableEntry)
{
    MV_U32 dataRegVal, i=0;
    DB( printf( "%s: \n", __FUNCTION__) );

#if defined PRESTERA_VLAN_INDIRECT
    if(setVLANTableCtrlReg(devNum, entryNum, 0 /*Read*/)!=MV_OK)
    {
        DB( printf( "%s: Error: Problem setting VLAN control register\n", __FUNCTION__) );
        return MV_FAIL;
    }
#endif
    for(i=0; i<4; i++)
    {
        if (mvSwitchReadReg(devNum,VLT_DATA_REG_OFFSET + entryNum*VLAN_ENTRY_SIZE + i*4,
                    &dataRegVal)!=MV_OK) {
            DB( printf( "%s: Error: Problem reading register 0x%08x\n",
                        __FUNCTION__ , VLT_DATA_REG_OFFSET + i*4) );
            return MV_FAIL;
        }
         vlanTableEntry->VLTData[3-i] = dataRegVal; /* Word 0 is in reg 3 and  */
                               /* Word 3 is in reg 0, etc */
        DB( printf("vlanTableEntry->VLTData[%d] = 0x%x\n",i,vlanTableEntry->VLTData[3-i]));
    }

    return MV_OK;
}

MV_STATUS setVLANEntry(int devNum, int entryNum,STRUCT_VLAN_ENTRY* vlanTableEntry)
{
    MV_U32 i=0;
    DB( printf( "%s: \n", __FUNCTION__) );

    for(i=0; i<4; i++)
    {
        DB( printf("vlanTableEntry->VLTData[%d] = 0x%x\n",i,vlanTableEntry->VLTData[3-i]));

        /* Word 0 is in reg 3 and  Word 3 is in reg 0, etc*/
        if (mvSwitchWriteReg(devNum,VLT_DATA_REG_OFFSET + entryNum*VLAN_ENTRY_SIZE + i*4,
            vlanTableEntry->VLTData[3-i])!=MV_OK) {
            DB( printf( "%s: Error: Problem reading register 0x%08x\n",
                     __FUNCTION__ , VLT_DATA_REG_OFFSET + i*4) );
            return MV_FAIL;
        }
    }

#if defined PRESTERA_VLAN_INDIRECT
    if(setVLANTableCtrlReg(devNum, entryNum, 1 /*Write*/)!=MV_OK)
    {
        DB(printf("%s: Error: Problem setting VLAN control register\n", __func__));
        return MV_FAIL;
    }
#endif
    return MV_OK;
}

/*******************************************************************************
 * mvPpValidMacEntryPrint
 */
void mvPpValidMacEntryPrint(void)
{
    MV_U8  devNum = 0;
    MV_U32 mac[4];
    MV_U32 i, addr;

    for (i = 0; i < 32767; i++)
    {
        addr = 0x6400000 + i*0x10;
        CHK_STS_VOID(hwIfGetReg(devNum, addr, &mac[0]))

        /* if entry valid */
        if (mac[0] & 1)
        {
            CHK_STS_VOID(hwIfGetReg(devNum, addr+4, &mac[1]))
            CHK_STS_VOID(hwIfGetReg(devNum, addr+8, &mac[2]))
            CHK_STS_VOID(hwIfGetReg(devNum, addr+12, &mac[3]))

            mvOsPrintf("%04d: %x %x %x %x.\n",
                       i, mac[0], mac[1], mac[2], mac[3]);
        }
    }
}

/*******************************************************************************
 * setCpuAsVLANMember
 */
MV_STATUS setCpuAsVLANMember(int devNum, int vlanNum)
{
    STRUCT_VLAN_ENTRY vlanTableEntry;
    DB( printf( "%s: \n", __FUNCTION__) );
    if (readVLANEntry(devNum, vlanNum, &vlanTableEntry)!=MV_OK)
        return MV_FAIL;

    vlanTableEntry.VLTData[3] |= (1 << CPU_VLAN_MEMBER_BIT) |
                                 (1 << IPv4_CTRL_TO_CPU_ENABLE);

    return setVLANEntry(devNum, vlanNum, &vlanTableEntry);
}

/*******************************************************************************
 * Statics
 */
static MV_U8 decodingTargetAttribures[2 /* dev */][4 /* target */] =
{
    {0xE, 0xD, 0xB, 0x7},
    {0xE8, 0xE8, 0xE8, 0xE8}
};

static MV_U8 targetIds[2 /* dev */] = {0x0, 0x4};
/* Number of devices in the system */
static int devicesNum = -1;

/*******************************************************************************
* mvSwitchAddrComp
*
* DESCRIPTION:
*       Writes the unmasked bits of a register using Pci.
*
* INPUTS:
*       baseAddr    - Device base address
*       regAddr     - Register address to perform address completion to.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK       - on success
*       MV_FAIL      - on error
*
* COMMENTS:
*
*******************************************************************************/
static MV_STATUS mvSwitchAddrComp(MV_U32 baseAddr, MV_U32 regAddr, MV_U32 *pciAddr)
{
    MV_U32      addressCompletion;
    MV_U32      region, regionValue, regionMask;

    /* read the contents of the address completion register */
    addressCompletion = (MV_U32)MV_32BIT_LE_FAST(*((volatile MV_U32 *)baseAddr));

    /* check if the region need to be changed */
    /* calculate the region (bits 24,25) */
    region = (regAddr & 0x3000000) >> 24;
    /* the region value is the 8 MSB of the register address */
    regionValue = (regAddr & 0xFF000000) >> 24;

    /* region 0 value is always 0 therefore we need to
        check that the regionValue is also 0 */
    if (region == 0 && regionValue !=0 )
    {
        /* change to a region that can be changed */
        region = DEFAULT_REGION;
    }

    /* the region mask for read modify write */
    regionMask = ~(0xFF << (region * 8));

    /* check if update of region is neccessary */
    if (regionValue != (addressCompletion & regionMask) >> (region * 8))
    {
        /* unset all the specific region bits */
        addressCompletion &= regionMask;
        /* write the new region value to the right place */
        addressCompletion |= regionValue << (region * 8);

        /* write back the updated address completion register */
        *((volatile MV_U32 *)baseAddr) = (MV_U32)MV_32BIT_LE_FAST(addressCompletion);
    }

    /* Calculate the PCI address to return */
    /* Remove 8 MSB and add region ID + base address */
    *pciAddr = baseAddr | (region << 24) | (regAddr & 0x00FFFFFF);

    return MV_OK;
}

/*******************************************************************************
* mvSwitchWriteReg
*
* DESCRIPTION:
*       Writes the unmasked bits to the switch internal register.
*
* INPUTS:
*       devNum    - Device Number
*       regAddr  - Register address to write to.
*       value    - Data to be written to register.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK     - on success
*       MV_FAIL   - on error
*
* COMMENTS:
*
*******************************************************************************/
MV_BOOL G_disableSwitchWriteReg = MV_FALSE;

void mvPpDisableSwitchWriteReg(void)
{
    G_disableSwitchWriteReg = MV_TRUE;
}

void mvPpEnableSwitchWriteReg(void)
{
    G_disableSwitchWriteReg = MV_FALSE;
}

MV_VOID mvPpWriteReg(MV_U8 devNum, MV_U32 regAddr, MV_U32 value)
{
    CHK_STS_VOID(mvSwitchWriteReg(devNum, regAddr, value))
}

MV_STATUS mvSwitchWriteReg(MV_U8 devNum, MV_U32 regAddr, MV_U32 value)
{
    MV_U32 address;

    if (G_disableSwitchWriteReg == MV_TRUE)
    {
        DB(mvOsPrintf("%s: Disabled write: addr = 0x%08X, val = 0x%08X.\n",
                   __func__, regAddr, value));
        return MV_OK;
    }

    if (devNum >= mvSwitchGetDevicesNum())
    {
        mvOsPrintf("%s: Wrong devNum (%d).\n", __func__, devNum);
        return MV_BAD_PARAM;
    }

#ifdef MV_PP_HAL_WRITE_LOG_ENABLE
    {
        MV_PP_HAL_CTRL *ctrlP = G_mvPpHalCtrlP;

        if (mvPpHalInit() != MV_OK)
        {
            mvOsPrintf("%s: mvPpHalInit failed.\n", __func__);
            return MV_FAIL;
        }

        if (G_disableSwitchWriteReg == MV_TRUE)
        {
            mvOsPrintf("%s: Disabled write: addr = 0x%08X, val = 0x%08X.\n",
                       __func__, regAddr, value);
            return MV_OK;
        }

        if (mvWriteLogAddEntry(ctrlP->ppLogHandleP, regAddr, value) != MV_OK)
        {
            mvOsPrintf("%s: mvWriteLogAddEntry failed.\n", __func__);
            return MV_FAIL;
        }
    }
#endif

    if (mvSwitchAddrComp(_prestera_dev_base_addr[devNum], regAddr, &address)!=MV_OK)
    {
        return MV_FAIL;
    }

    *((volatile MV_U32 *)address) = (MV_U32)MV_32BIT_LE_FAST(value);

    return MV_OK;
}

/*******************************************************************************
* mvSwitchReadReg
*
* DESCRIPTION:
*       Reads the unmasked bits of the switch internal register.
*
* INPUTS:
*       devNum    - Device Number
*       regAddr   - Register address to read.
*       pValue    - Data to read from the register.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK      - on success
*       MV_FAIL      - on error
*
* COMMENTS:
*
*******************************************************************************/
MV_STATUS mvSwitchReadReg(MV_U8   devNum, MV_U32   regAddr, MV_U32  *pValue)
{
    MV_U32 address;
    MV_U32 tmp;

    if (devNum >= mvSwitchGetDevicesNum())
    {
        return MV_BAD_PARAM;
    }

    if ((mvSwitchAddrComp(_prestera_dev_base_addr[devNum],
                          regAddr,
                          &address)) != MV_OK)
    {
        return MV_FAIL;
    }

    tmp = (MV_U32)(*((volatile MV_U32 *)address));
    *pValue = MV_32BIT_LE_FAST(tmp);

    return MV_OK;
}

/*******************************************************************************
 * mvPpReadReg
 */
MV_U32 mvPpReadReg(MV_U8 devNum, MV_U32 regAddr)
{
    MV_U32 address;
    MV_U32 value;

    if (devNum >= mvSwitchGetDevicesNum())
    {
        mvOsPrintf("%s: devNum(%d) is too big.\n", __func__, devNum);
        return 0xFFFFFFFF;
    }

    if ((mvSwitchAddrComp(_prestera_dev_base_addr[devNum],
                          regAddr,
                          &address)) != MV_OK)
    {
        mvOsPrintf("%s: mvSwitchAddrComp failed.\n", __func__);
        return 0xFFFFFFFF;
    }

    value = (MV_U32)(*((volatile MV_U32 *)address));
    value = MV_32BIT_LE_FAST(value);

    return value;
}

/*******************************************************************************
 * mvPpReadRegDev0
 */
MV_U32 mvPpReadRegDev0(MV_U32 regAddr)
{
    MV_STATUS stat;
    MV_U32    actualAddr, val, dev = MV_PP_DEV0;
    MV_U32   *ptr;

    stat = mvSwitchAddrComp(_prestera_dev_base_addr[dev], regAddr, &actualAddr);
    if (stat != MV_OK)
    {
        mvOsPrintf("%s: mvSwitchAddrComp failed.\n", __func__);
        return 0xFFFFFFFF;
    }

    ptr = (MV_U32 *)actualAddr;
    val = *ptr;
    val = MV_32BIT_LE_FAST(val);

    return val;
}

/*******************************************************************************
* mvSwitchReadModWriteReg
*
* DESCRIPTION:
*       Writes the masked bits to the switch internal register.
*
* INPUTS:
*       devNum    - Device Number
*       regAddr   - Register address to write.
*       mask      - Bit mask value selecting the bits needed to be written.
*       value     - Data to write to the register.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK      - on success
*       MV_FAIL      - on error
*
* COMMENTS:
*
*******************************************************************************/
MV_STATUS mvSwitchReadModWriteReg(MV_U8  devNum,
                                  MV_U32 regAddr,
                                  MV_U32 mask,
                                  MV_U32 value)
{
    MV_U32      regData;
    MV_STATUS   rc;

    if (devNum >= mvSwitchGetDevicesNum())
    {
        return MV_BAD_PARAM;
    }

    rc = mvSwitchReadReg(devNum,regAddr,&regData);
    if (rc != MV_OK)
    {
        return rc;
    }

    /* Update the relevant bits at the register data */
    regData = (regData & ~mask) | (value & mask);
    rc = mvSwitchWriteReg(devNum, regAddr, regData);
    return rc;
}

/*******************************************************************************
 * mvSwitchBitSet
 */
MV_STATUS mvSwitchBitSet(MV_U8 devNum, MV_U32 regAddr, MV_U32 bitMask)
{
    return mvSwitchReadModWriteReg(devNum, regAddr, bitMask, bitMask);
}

/*******************************************************************************
 * mvPpBitSet
 */
MV_VOID mvPpBitSet(MV_U8 devNum, MV_U32 regAddr, MV_U32 bitMask)
{
    CHK_STS_VOID(mvSwitchReadModWriteReg(devNum, regAddr, bitMask, bitMask))
}

/*******************************************************************************
 * mvSwitchBitReset
 */
MV_STATUS mvSwitchBitReset(MV_U8 devNum, MV_U32 regAddr, MV_U32 bitMask)
{
    return mvSwitchReadModWriteReg(devNum, regAddr, bitMask, 0);
}

/*******************************************************************************
 * mvPpBitReset
 */
MV_VOID mvPpBitReset(MV_U8 devNum, MV_U32 regAddr, MV_U32 bitMask)
{
    CHK_STS_VOID(mvSwitchReadModWriteReg(devNum, regAddr, bitMask, 0))
}

/*******************************************************************************
* mvSwitchGetDevicesNum
*
* DESCRIPTION:
*       returns the number of devices on the system.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       none.
*
* RETURNS:
*        number of devices
*
* COMMENTS:
*
*******************************************************************************/

extern MV_U32 G_mvIsPexInited;

MV_U8 mvPpDevNumGet(void)
{
    return mvSwitchGetDevicesNum();
}

MV_U8 mvSwitchGetDevicesNumXCat(void)
{
    #if defined (MV_XCAT_INTERPOSER)
    devicesNum = 1;
    #endif

#if !defined(MV_INCLUDE_PEX)
    devicesNum = 1;
#else
    if (G_mvIsPexInited == 0)
    {
        return 1;
    }

    if (devicesNum == -1)
    {
        int dev;
        MV_U32 pexData;
        devicesNum = 1;

        for (dev=0;dev<32;dev++)
        {
            pexData = mvPexConfigRead(0/*pexIf*/, 0/*bus*/, dev/*dev*/,
                                      0/*func*/,0x0/*VENDOR_ID_REG*/);

            if ((pexData&0xFFFF) == 0x11AB && (pexData&0xF0000000) == 0xD0000000)
            {
                pexData = mvPexConfigRead(0/*pexIf*/, 0/*bus*/,
                                         dev/*dev*/ , 0/*func*/,0x18);
                devicesNum ++;
            }
        }
    }
#endif

    return devicesNum;
}

MV_U8 mvSwitchGetDevicesNumXcat2(void)
{
    static MV_U32 numOfPpDev = 0;
    MV_U32 pexData, dev;

    if (G_mvIsPexInited == 0)
    {
        return 1;
    }

#if !defined(MV_INCLUDE_PEX)
    numOfPpDev = 1;
#else
    if (numOfPpDev == 0)
    {
        for (dev=0;dev<32;dev++)
        {
            pexData = mvPexConfigRead(0, 0, dev, 0, 0x0);
            pexData &= 0xFFFF;

            if (pexData == 0x11AB)
            {
                numOfPpDev++;
            }
        }
    }
#endif

    return numOfPpDev;
}

/*******************************************************************************
 * mvSwitchGetDevicesNum
 */
MV_U8 mvSwitchGetDevicesNum(void)
{
    if (mvPpChipIsXCat2Simple() == MV_TRUE)
    {
        return mvSwitchGetDevicesNumXcat2();
    }
    else
    {
        return mvSwitchGetDevicesNumXCat();
    }
}

/*******************************************************************************
 * mvSwitchGetPortsNum
 */
MV_U32 mvSwitchGetPortsNum(void)
{
    return mvSwitchGetDevicesNum() * PP_DEV_PORT_NUM;
}

/*******************************************************************************
 * mvPpNetPortsNumGet
 */
MV_U32 mvPpNetPortsNumGet(void)
{
    return PP_DEV_PORT_NUM;
}

/*******************************************************************************
 * mvPpTwoBytePrependDisable
 */
MV_STATUS mvPpTwoBytePrependDisable(void)
{
    MV_PP_HAL_CTRL *ctrlP = G_mvPpHalCtrlP;
    MV_U32 dev, numOfDevs = mvSwitchGetDevicesNum();

    for (dev = 0; dev < numOfDevs; dev++)
    {
        mvPpBitReset(dev, ctrlP->devCfg.prependCfgReg, ctrlP->devCfg.prependCfgBit);
    }

    return MV_OK;
}

/*******************************************************************************
 * mvSwitchCpuPortConfig
 */
MV_STATUS mvSwitchCpuPortConfig(MV_U8 devNum)
{
    /* Mg Global Control: set SelPortSDMA = 0 and PowerSave = 0 */
    CHECK_STATUS(mvSwitchReadModWriteReg(devNum,
                                        PRESTERA_GLOBAL_CTRL_REG,
                                        BIT_19|BIT_20, 0))

    /* Set ref_clk_125_sel to PLL */
    CHECK_STATUS(mvSwitchReadModWriteReg(devNum,
                                  PRESTERA_EXT_GLOBAL_CFG_REG, BIT_10, BIT_10))

    if (mvPpChipIsXCat2() == MV_TRUE)
    {
        mvPpBitSet(PP_DEV0, 0xE8, BIT_8);
        /* Set CPUPortActive = 1, CPUPortIFType = 1 (GMII), MIBCountMode = 1 */
        CHECK_STATUS(hwIfSetReg(devNum, PRESTERA_CPU_PORT_GLOBAL_CFG_REG, 0xB))

        if (mvPpIsChipGE() == MV_TRUE)
        {
            mvPpWriteReg(devNum, PRESTERA_EXT_GLOBAL_CFG_REG, 0x740D);
        }
        else
        {
            mvPpWriteReg(devNum, PRESTERA_EXT_GLOBAL_CFG_REG, 0x80740D);
        }
    }
    else
    {
        /* Set CPUPortActive = 1, CPUPortIFType = 2, MIBCountMode = 1. */
        CHECK_STATUS(hwIfSetReg(devNum, PRESTERA_CPU_PORT_GLOBAL_CFG_REG, 0xd))
    }

    /* Set R0_Active = 0, RGPP_TEST = 0, GPP_Active = 1 */
    CHECK_STATUS(mvSwitchReadModWriteReg(devNum, PRESTERA_DEV_CONFIG_REG,
                                          (BIT_18 | BIT_19 | BIT_20), BIT_18))

    CHECK_STATUS(mvSwitchReadModWriteReg(devNum, PRESTERA_SAMPLE_AT_RESET_REG,
                                 (BIT_10 | BIT_11 | BIT_12), (BIT_10 | BIT_11)))

    /*
     * Set PortMacControl fot port 63:
     * PcsEn = 0, UseIntClkforEn = 1, PortMACReset = 0,
     * CollisionOnBackPressureCntEn=1
     */
    CHECK_STATUS(hwIfSetReg(devNum, PRESTERA_CPU_PORT_MAC_CTRL_REG(3), 0x300))
    CHECK_STATUS(hwIfSetReg(devNum, PRESTERA_CPU_PORT_MAC_CTRL_REG(2), 0x4010))
    CHECK_STATUS(hwIfSetReg(devNum, PRESTERA_CPU_PORT_MAC_CTRL_REG(1), 0x1f87))
    CHECK_STATUS(hwIfSetReg(devNum, PRESTERA_CPU_PORT_MAC_CTRL_REG(0), 0x8be5))

    return MV_OK;
}

/*******************************************************************************
* mvSwitchOpenMemWinAsKirkwood
*
* DESCRIPTION:
*       Initialization of decoding windows for memory access
*       Purpose of this function is to open window to xBar for all enabled
*       SDRAM ChipSelects
*
* INPUTS:
*       baseAddr  - Device Base address
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK      - on success
*       MV_FAIL      - on error
*
* COMMENTS:
*
*******************************************************************************/
MV_STATUS mvSwitchOpenMemWinAsKirkwood(MV_U8 devNum)
{
    MV_U32 baseReg = 0, sizeReg = 0, baseToReg = 0, sizeToReg = 0, target,
           decodingIndex = 0;
    /* init value of BaseAddressRegisterEnable - 0x3F - all off */
    MV_U8 initBare = 0x3F;

    if (mvPpChipIsXCat() == MV_TRUE && devNum == 1)
    {
        /* Open PEX Window for second device */

        /* Window to PEX Target ID 0xF, Attribute 0x80 */
        if (mvSwitchWriteReg(1,PRESTERA_ADDR_DECODE_WIN_BASE,0x80F) != MV_OK)
        {
            DB(mvOsPrintf("%s: mvSwitchWriteReg failed.\n", __func__));
        }
        /* Set window size */
        if (mvSwitchWriteReg(1,PRESTERA_ADDR_DECODE_WIN_SIZE, 0xFFFF0000) != MV_OK)
        {
            DB(mvOsPrintf("%s: mvSwitchWriteReg failed.\n", __func__));
        }

        /* Enable window 0 */
        mvSwitchReadModWriteReg(1,PRESTERA_ADDR_DECODE_WIN_ENABLE,BIT_0,0x0);
    }

    for (target = 0; target < 4; target ++)
    {
        sizeReg = MV_REG_READ(SDRAM_SIZE_REG(0, target));
        if (!(sizeReg & SCSR_WIN_EN))
            continue;

        baseReg = MV_REG_READ(SDRAM_BASE_ADDR_REG(0, target));
        baseToReg = PRESTERA_AD_BASE_ADDR_BITS_SET(baseReg,
                                decodingTargetAttribures[devNum][target],
                                targetIds[devNum]);

        /* Define window */
        CHECK_STATUS(mvSwitchWriteReg(devNum,
                         PRESTERA_AD_BASE_ADDR(decodingIndex), baseToReg))

        /*
         * Prestera size decoding field is 16 bit [32-16] and
         * CPU size decoding field is 8 bit [32-24]
         */
        sizeToReg = ((sizeReg & SCSR_SIZE_MASK) | 0xFF0000);

        /* Set window size */
        CHECK_STATUS(mvSwitchWriteReg(devNum,
                         PRESTERA_AD_SIZE(decodingIndex), sizeToReg))

        /* set corresponding bit to 0 - enable windows */
        PRESTERA_AD_BARE_BITS_SET(initBare, decodingIndex);
        decodingIndex++;
    }

    /* Enable relevant windows */
    CHECK_STATUS(mvSwitchWriteReg(devNum, PRESTERA_AD_BARE, initBare))
    return MV_OK;
}

/*******************************************************************************
 * mvSwitchUseSdmaWa
 *     Checks if SDMA work-around should be used.
 */
MV_BOOL mvSwitchUseSdmaWa(MV_VOID)
{
    if (mvPpChipIsXCat2() == MV_FALSE && mvPpGetChipRev() == 2)
    {
        /* WA is needed only for xCat-A1. */
        return MV_TRUE;
    }

    return MV_FALSE;
}

/*******************************************************************************
* mvPpSwitchInit
*
* DESCRIPTION:
*       Basic PP configuration.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_TRUE     - on success.
*       MV_FALSE    - on failure.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_STATUS mvPpSwitchInit(MV_U32 dev)
{
    MV_PP_HAL_CTRL *ctrlP = G_mvPpHalCtrlP;

    MV_ERRATA_APPLY(mvErrataXCatPpHandleGet(), MV_ERRATA_ID_SDMA_256_INIT);

    mvSwitchOpenMemWinAsKirkwood(dev);

    /* SelPortSDMA = 1: select SDMA interface. */
    mvPpBitSet(MV_PP_DEV0, PRESTERA_GLOBAL_CTRL_REG, BIT20);

    /* disable Rx queues - before enable */
    mvPpWriteReg(dev, 0x2680, 0xFF00);

    /* SDMA - disable retransmit on resource error */
    CHECK_STATUS(mvSwitchReadModWriteReg(dev, 0x2800, 0xff00, 0xff00))

    /* Configures PP CPU_Port to be DSA tagged (Cascade port). */
    mvPpBitSet(dev, ctrlP->devCfg.dsaEnableCfgReg, ctrlP->devCfg.dsaEnableBit);

    /* Enable two-byte prepending for RX packet. */
    mvPpBitSet(dev, ctrlP->devCfg.prependCfgReg, ctrlP->devCfg.prependCfgBit);

    if (setCpuAsVLANMember(dev, 1 /* vlan */) != MV_OK)
    {
        mvOsPrintf("%s: setCpuAsVLANMember failed.\n", __func__);
        return MV_FAIL;
    }

    return MV_OK;
}

/*******************************************************************************
 * mvPpSetCpuPortToCascadeMode
 */
MV_STATUS mvPpSetCpuPortToCascadeMode(MV_U32 dev)
{
    MV_PP_HAL_CTRL *ctrlP = G_mvPpHalCtrlP;

    if (mvPpHalIsInited() == MV_FALSE)
    {
        mvOsPrintf("%s: PP HAL is not inited.\n", __func__);
        return MV_FAIL;
    }

    /* Configures PP CPU_Port to be DSA tagged (Cascade port). */
    mvPpBitSet(dev, ctrlP->devCfg.dsaEnableCfgReg, ctrlP->devCfg.dsaEnableBit);

    return MV_OK;
}

/*******************************************************************************
 * mvSwitchMruGet
 */
MV_U32 mvSwitchMruGet(MV_U32 dev, MV_U32 port)
{
    MV_U32 regAddr, regVal;

    if (port < 28)
    {
        regAddr = PP_PORT_MAC_CTRL_REG0(port);
    }
    else if (port == PP_CPU_PORT_NUM)
    {
        regAddr = PP_CPU_PORT_MAC_CTRL_REG;
    }
    else
    {
        mvOsPrintf("%s: Wrong port (%d).\n", __func__, port);
        return -1;
    }

    regVal = mvPpReadReg(dev, regAddr);
    regVal &= (0x1FFF << 2); /* MRU in double-bytes */
    regVal /= 2; /* MRU in bytes */
    return regVal;
}

/*******************************************************************************
 * mvSwitchMruSet
 */
MV_STATUS mvSwitchMruSet(MV_U32 dev, MV_U32 port, MV_U32 mruBytes)
{
    /* FrameSizeLimit mask */
    MV_U32 mruBits = ((mruBytes / 2) << 2) & (0x1FFF << 2);
    MV_U32 regVal, regAddr;

    if (port < 28)
    {
        regAddr = PP_PORT_MAC_CTRL_REG0(port);
    }
    else if (port == PP_CPU_PORT_NUM)
    {
        regAddr = PP_CPU_PORT_MAC_CTRL_REG;
    }
    else
    {
        mvOsPrintf("%s: Wrong port (%d).\n", __func__, port);
        return MV_FAIL;
    }

    /*
     * Configure MRU for Prestera Switch CPU_Port (only for dev 0)
     */
    regVal = mvPpReadReg(dev, regAddr);
    regVal &= ~(0x1FFF << 2); /* clean size limit bits */
    regVal |= mruBits;
    mvPpWriteReg(dev, regAddr, regVal);

    return MV_OK;
}

/*******************************************************************************
 * mvSwitchMruSetAllPorts
 */
MV_STATUS mvSwitchMruSetAllPorts(MV_U32 mruBytes)
{
    MV_U32 dev, port, numOfDevs;

    numOfDevs = mvSwitchGetDevicesNum();

    /* mru  = mru + 8; */ /* due to DSA tag */

    for (dev = 0; dev < numOfDevs; dev++)
    {
        for (port = 0; port < 28; port++)
        {
            if (mvSwitchMruSet(dev, port, mruBytes) != MV_OK)
            {
                mvOsPrintf("%s: mvSwitchMruSet(mruBytes=%d) failed.\n",
                           __func__, mruBytes);
                return MV_FAIL;
            }
        }
    }

    if (mvSwitchMruSet(PP_DEV0, PP_CPU_PORT_NUM, mruBytes) != MV_OK)
    {
        mvOsPrintf ("%s: mvSwitchMruSet(%d) failed.\n", __func__, mruBytes);
        return MV_FAIL;
    }

    return MV_OK;
}

/*******************************************************************************
 * mvSwitchMruPrintAllPorts
 */
void mvSwitchMruPrintAllPorts(void)
{
    MV_U32 dev, port, numOfDevs, mruBytes;

    numOfDevs = mvSwitchGetDevicesNum();
    for (dev = 0; dev < numOfDevs; dev++)
    {
        for (port = 0; port < 28; port++)
        {
            mruBytes = mvSwitchMruGet(dev, port);
            mvOsPrintf("dev%d:port%d: MRU(bytes) = %d\n", dev, port, mruBytes);
        }
    }

    mruBytes = mvSwitchMruGet(PP_DEV0, PP_CPU_PORT_NUM);
    mvOsPrintf("dev%d:port%d: MRU(bytes) = %d\n",
            PP_DEV0, PP_CPU_PORT_NUM, mruBytes);
}

/*******************************************************************************
 * mvPpIsTwoBytesPrepended
 */
MV_BOOL mvPpIsTwoBytesPrepended(MV_U32 dev)
{
    MV_PP_HAL_CTRL *ctrlP = G_mvPpHalCtrlP;
    MV_U32          regVal;
    MV_BOOL         retVal;

    regVal = mvPpReadReg(dev, ctrlP->devCfg.prependCfgReg);

    if (regVal & ctrlP->devCfg.prependCfgBit)
    {
        retVal = MV_TRUE;
    }
    else
    {
        retVal = MV_FALSE;
    }

    return retVal;
}

/*******************************************************************************
 * mvSwitchEgressFilterCfg
 */
void mvSwitchEgressFilterCfg(void)
{
    /* Multi-Destination Source-ID Egress Filtering */
    mvPpWriteReg(PP_DEV0, 0x1A4001C, 0xFFFFFFFF);
    mvPpBitReset(PP_DEV0, 0x1A4001C, BIT_26);

    if (mvSwitchGetDevicesNum() > 1)
    {
        mvPpWriteReg(PP_DEV1, 0x1A4000C, 0xFFFFFFFF);
        mvPpBitReset(PP_DEV1, 0x1A4000C, BIT_25);
    }
}

/*******************************************************************************
 * mvSwitchVidxCfg
 */
void mvSwitchVidxCfg(void)
{
    /* All ports are members of VIDX 1 except [dev 0, port 26] */
    CHK_STS_VOID(mvSwitchWriteReg(PRESTERA_DEFAULT_DEV,
                                  0xA100010, 0x1FFFFFFF));
    CHK_STS_VOID(mvSwitchBitReset(PRESTERA_DEFAULT_DEV,
                                  0xA100010, BIT_27 /* port 26 */));

    if (mvSwitchGetDevicesNum() > 1)
    {
        /* All ports are members of VIDX 1 except [dev 0, port 25] */
        CHK_STS_VOID(mvSwitchWriteReg(PRESTERA_SECOND_DEV,
                                      0xA100010, 0x1FFFFFFF));
        CHK_STS_VOID(mvSwitchBitReset(PRESTERA_SECOND_DEV,
                                      0xA100010, BIT_26 /* port 25 */));
    }
}

/*******************************************************************************
 * mvPpCascadePortCfg
 */
void mvPpCascadePortCfg(void)
{
    /*
     * Switch dev 0 port 27 is connected to swithc dev 1 port 24.
     * Switch dev 0 port 26 is connected to switch dev 1 port 25.
     * Let's neutraulize [dev 0, port 26] and [dev 1, port 25].
     */
    if (mvPpChipIsXCat2() == MV_TRUE)
    {
     /* mvPpBitSet(PP_DEV0, CASCADE_AND_HEADER_CONFIG_REG, BIT_26); */
        mvPpBitSet(PP_DEV0, CASCADE_AND_HEADER_CONFIG_REG, BIT_27);
        if (mvSwitchGetDevicesNum() > 1)
        {
            mvPpBitSet(PP_DEV1, CASCADE_AND_HEADER_CONFIG_REG, BIT_24);
         /* mvPpBitSet(PP_DEV1, CASCADE_AND_HEADER_CONFIG_REG, BIT_25); */
        }
    }
    else
    {
        CHK_STS_VOID(mvSwitchBitReset(PRESTERA_DEFAULT_DEV,
                                  CASCADE_AND_HEADER_CONFIG_REG, BIT_26));

        if (mvSwitchGetDevicesNum() > 1)
        {
            CHK_STS_VOID(mvSwitchBitReset(PRESTERA_SECOND_DEV,
                                          CASCADE_AND_HEADER_CONFIG_REG, BIT_25));
        }
    }
}

/*******************************************************************************
 * mvSwitchRxIntEnable
 */
void mvSwitchRxIntEnable(MV_U32 dev)
{
    mvPpBitSet(dev, SWITCH_GLOBAL_MASK_REG,  SWITCH_RX_SDMA_INT_MASK);
    mvPpBitSet(dev, SWITCH_SDMA_RX_MASK_REG, SWITCH_RX_QUEUE_MASK);
}

/*******************************************************************************
 * mvSwitchTxIntEnable
 */
void mvSwitchTxIntEnable(MV_U32 dev)
{
    mvPpBitSet(dev, SWITCH_GLOBAL_MASK_REG,  SWITCH_TX_SDMA_INT_MASK);
    mvPpBitSet(dev, SWITCH_SDMA_TX_MASK_REG, SWITCH_TX_QUEUE_MASK);
}

/*******************************************************************************
 * mvSwitchRxIntDisable
 */
void mvSwitchRxIntDisable(MV_U32 dev)
{
    mvPpBitReset(dev, SWITCH_SDMA_RX_MASK_REG, SWITCH_RX_QUEUE_MASK);
}

/*******************************************************************************
 * mvSwitchTxIntDisable
 */
void mvSwitchTxIntDisable(MV_U32 dev)
{
    mvPpBitReset(dev, SWITCH_SDMA_TX_MASK_REG, SWITCH_TX_QUEUE_MASK);
}

/*******************************************************************************
* switchWriteReg
*
* DESCRIPTION:
*       Writes the unmasked bits to the switch internal register.
*
* INPUTS:
*       regAddr  - Register address to write to.
*       value    - Data to be written to register.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK     - on success
*       MV_FAIL   - on error
*
* COMMENTS:
*
*******************************************************************************/
MV_STATUS switchWriteReg(MV_U8 devNum, MV_U32 regAddr, MV_U32 value)
{
    MV_CPU_DEC_WIN addrWin;

    if (mvCpuIfTargetWinGet(PP_TARGET_ID, &addrWin) != MV_OK)
    {
        mvOsPrintf("%s: Fail get base address.\n", __func__);
        return MV_FAIL;
    }

    return mvSwitchWriteReg(devNum,regAddr,value);
}

/*******************************************************************************
* switchReadReg
*
* DESCRIPTION:
*       Reads the unmasked bits of the switch internal register.
*
* INPUTS:
*       regAddr   - Register address to read.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK      - on success
*       MV_FAIL      - on error
*
* COMMENTS:
*
*******************************************************************************/
MV_STATUS switchReadReg(MV_U8 devNum, MV_U32 regAddr)
{
    MV_U32 value;

    /* read the register value */
    if (mvSwitchReadReg(devNum,regAddr,&value) != MV_OK)
    {
        mvOsPrintf("\nFail read register");
        return MV_FAIL;
    }

    mvOsPrintf("\n0x%x: 0x%x\n",regAddr,value);

    return MV_OK;
}

/*******************************************************************************
* switchReadPortMibCounters
*
* DESCRIPTION:
*       Print MIB counters of the switch port
*
* INPUTS:
*       port - port number
*
* OUTPUTS:
*       none
*
* RETURNS:
*       none
*
* COMMENTS:
*
*******************************************************************************/
void switchReadPortMibCounters(int port)
{
    mvPresteraReadPortMibCounters(port);
}

/*******************************************************************************
 * mvPpQbitsOfDevGet
 */
static __inline MV_U32 mvPpQ2Dev(MV_U32 queue)
{
    if (queue > NUM_OF_RX_QUEUES)
    {
        return PP_DEV1;
    }

    return PP_DEV0;
}

/*******************************************************************************
 * mvPpQbitsOfDevGet
 */
static __inline MV_U32 mvPpQbitsOfDevGet(MV_U32 dev, MV_U32 qBitMask)
{
    MV_U32 shift;

    shift    = dev * NUM_OF_RX_QUEUES;
    qBitMask = (qBitMask & (PP_ALL_Q_MASK << shift)) >> shift;

    return qBitMask;
}

/*******************************************************************************
 * mvPpRxReadyIntMask
 */
MV_VOID mvPpRxReadyIntMask(MV_U32 rxQBitMask)
{
    MV_U32 bits, dev;

    for (dev = PP_DEV0; dev < 1 /* mvSwitchGetDevicesNum() */; dev++)
    {
        bits = mvPpQbitsOfDevGet(dev, rxQBitMask);

        /*
         * Currently disables all the queues.
         */
        mvSwitchRxIntDisable(dev);
    }
}

/*******************************************************************************
 * mvPpTxDoneIntMask
 */
MV_VOID mvPpTxDoneIntMask(MV_U32 txQBitMask)
{
    MV_U32 bits, dev;

    for (dev = PP_DEV0; dev < 1 /* mvSwitchGetDevicesNum() */; dev++)
    {
        bits = mvPpQbitsOfDevGet(dev, txQBitMask);

        /*
         * Currently disables all the queues.
         */
        mvSwitchTxIntDisable(dev);
    }
}

/*******************************************************************************
 * mvPpRxReadyIntUnmask
 */
MV_VOID mvPpRxReadyIntUnmask(MV_U32 rxQBitMask)
{
    MV_U32 bits, dev;

    for (dev = PP_DEV0; dev < 1 /* mvSwitchGetDevicesNum() */; dev++)
    {
        bits = mvPpQbitsOfDevGet(dev, rxQBitMask);

        /*
         * Currently enable all the queues.
         */
        mvSwitchRxIntEnable(dev);
    }
}

/*******************************************************************************
 * mvPpTxDoneIntUnmask
 */
MV_VOID mvPpTxDoneIntUnmask(MV_U32 txQBitMask)
{
    MV_U32 bits, dev;

    for (dev = PP_DEV0; dev < 1 /* mvSwitchGetDevicesNum() */; dev++)
    {
        bits = mvPpQbitsOfDevGet(dev, txQBitMask);

        /*
         * Currently disables all the queues.
         */
        mvSwitchTxIntEnable(dev);
    }
}

/*******************************************************************************
 * mvPpRxReadyIntAck
 */
MV_VOID mvPpRxReadyIntAck(MV_U32 ackBitMask)
{
    /*
     * Layer 1 PP interrupts: PP global interrupt register is RO (Read Only).
     * Layer 2 PP interrupts: SDMA interrupt registers are ROC (Read Only Clear),
     *                        meaning the register is zeroed after it is read.
     */
}

/*******************************************************************************
 * mvPpTxDoneIntAck
 */
MV_VOID mvPpTxDoneIntAck(MV_U32 ackBitMask)
{
    /*
     * Layer 1 PP interrupts: PP global interrupt register is RO (Read Only).
     * Layer 2 PP interrupts: SDMA interrupt registers are ROC (Read Only Clear),
     *                        meaning the register is zeroed after it is read.
     */
}

/*******************************************************************************
 * mvPpRxReadyQGet
 */
MV_U32 mvPpRxReadyQGet()
{
    MV_U32 bits, dev, rxQBitMask;

    rxQBitMask = 0;
    for (dev = PP_DEV0; dev < 1 /* mvSwitchGetDevicesNum() */; dev++)
    {
        if (hwIfGetReg(dev, SWITCH_SDMA_RX_CAUSE_REG, &bits) != MV_OK)
        {
            mvOsPrintf("%s: hwIfGetReg failed.\n", __func__);
            return 0;
        }

        bits       >>= 2;
        if ((bits & 0xFF00) != 0)
        {
            DB(mvOsPrintf("%s: RxError Queue[0:7] = 0x02%X.\n",
                       __func__, (bits & 0xFF00) >> 8));
        }
        bits        &= 0xFF; /* leave RxBuffer Queue [0:7] bits only */

        bits       <<= dev * 8;
        rxQBitMask  |= bits;
    }

    return rxQBitMask;
}

/*******************************************************************************
 * mvPpTxDoneQGet
 */
MV_U32 mvPpTxDoneQGet()
{
    MV_U32 bits, dev, txQBitMask;

    txQBitMask = 0;
    for (dev = PP_DEV0; dev < 1 /* mvSwitchGetDevicesNum() */; dev++)
    {
        if (hwIfGetReg(dev, SWITCH_SDMA_TX_CAUSE_REG, &bits) != MV_OK)
        {
            mvOsPrintf("%s: hwIfGetReg failed.\n", __func__);
            return 0;
        }

        /* mvOsPrintf("%s: TxError Queue[0:7] = 0x08%X.\n", __func__, bits); */

        bits       >>= 1;
        if ((bits & 0xFF00) != 0)
        {
            /* mvOsPrintf("%s: TxError Queue[0:7] = 0x08%X.\n",
                       __func__, (bits & 0xFF00) >> 8); */
        }
        bits        &= 0xFF; /* leave TxBuffer Queue [0:7] bits only */

        bits       <<= dev * 8 /* 8=numOfRxQueue for one dev */;
        txQBitMask  |= bits;
    }

    return txQBitMask;
}

/*******************************************************************************
 * mvPpRxDone
 */
MV_STATUS mvPpRxDone(MV_PKT_INFO *pktInfoP, MV_U32 rxQ)
{
    MV_U32 dev;

    dev = mvPpQ2Dev(rxQ);
    if (dev >= 1 /* mvSwitchGetDevicesNum() */)
    {
        mvOsPrintf("%s: Wrong dev (%d).\n", __func__, dev);
        return MV_FAIL;
    }
    rxQ = rxQ - NUM_OF_RX_QUEUES * dev;

    while (pktInfoP != NULL)
    {
        if (mvPpRefillRxDesc(dev, rxQ, pktInfoP) != MV_OK)
        {
            mvOsPrintf("%s: mvPpRefillRxDesc failed.\n", __func__);
            return MV_FAIL;
        }

        if (genPoolPut(mvPpRxPktInfoPoolP[dev], pktInfoP) != MV_OK)
        {
            mvOsPrintf("%s: genPoolPut failed.\n", __func__);
            return MV_FAIL;
        }

        pktInfoP = pktInfoP->nextP;
    }

    return MV_OK;
}

/*******************************************************************************
 * mvPpPortRx
 */
MV_PKT_INFO *mvPpPortRx(MV_U32 rxQ)
{
    MV_PKT_INFO       *pktInfoP;
    MV_PKT_INFO       *origPktInfoP;
    MV_U32             dev;

    dev = mvPpQ2Dev(rxQ);
    if (dev >= 1 /* mvSwitchGetDevicesNum() */)
    {
        mvOsPrintf("%s: Wrong dev (%d).\n", __func__, dev);
        return NULL;
    }
    rxQ = rxQ - NUM_OF_RX_QUEUES * dev;

    pktInfoP = (MV_PKT_INFO *)genPoolGet(mvPpRxPktInfoPoolP[dev]);
    if (pktInfoP == NULL)
    {
        mvOsPrintf("%s: genPoolGet failed.\n", __func__);
        return NULL;
    }
    origPktInfoP = pktInfoP;

    pktInfoP = mvSwitchRxStart(dev, rxQ, pktInfoP);
    if (pktInfoP == NULL)
    {
        /* No ready RX packets. */
        if (genPoolPut(mvPpRxPktInfoPoolP[dev], origPktInfoP) != MV_OK)
        {
            mvOsPrintf("%s: genPoolPut failed.\n", __func__);
            return NULL;
        }
        return NULL;
    }

    return pktInfoP;
}

/*******************************************************************************
 * mvPpPortTxDone
 */
MV_PKT_INFO *mvPpPortTxDone(MV_U32 txQ)
{
 /* MV_PP_HAL_CTRL *ctrlP = G_mvPpHalCtrlP; */
 /* GEN_POOL       *poolP = ctrlP->txPktInfoPoolP; */
    MV_PKT_INFO    *pktInfoP;
    MV_U32          dev;

    MV_PKT_INFO    *headP = G_headP;
    MV_PKT_INFO    *tailP = G_tailP;

    DB(mvOsPrintf("%s: ENTERED.\n", __func__));

    dev = mvPpQ2Dev(txQ);
    if (dev >= 1 /* mvSwitchGetDevicesNum() */)
    {
        mvOsPrintf("%s: Wrong dev (%d).\n", __func__, dev);
        return NULL;
    }
    txQ = txQ - NUM_OF_TX_QUEUES * dev;

    /*
     * TX pktInfo is saved on TX to Prestera HAL internal pool.
     */

    if (headP == NULL)
    {
        DB(mvOsPrintf("%s: no pktInfoP.\n", __func__));
        return NULL;
    }
    else if (headP == tailP)
    {
        pktInfoP = headP;
        headP = NULL;
        tailP = NULL;
    }
    else
    {
        pktInfoP = headP;
        headP = headP->prevP;
        headP->nextP = NULL;
    }

    G_headP = headP;
    G_tailP = tailP;

    if (mvSwitchTxDone(dev, txQ, 1 /* descNum */) != MV_OK)
    {
        mvOsPrintf("%s: mvSwitchTxDone failed.\n", __func__);
        return NULL;
    }

    return pktInfoP;
}

/*******************************************************************************
 * mvPpPortTx
 */
MV_STATUS mvPpPortTx(MV_PKT_INFO *pktInfoP, MV_U32 txQ)
{
    MV_U32 dev;

    dev = mvPpQ2Dev(txQ);
    if (dev >= 1 /* mvSwitchGetDevicesNum() */)
    {
        mvOsPrintf("%s: Wrong dev (%d).\n", __func__, dev);
        return MV_FAIL;
    }
    txQ = txQ - NUM_OF_RX_QUEUES * dev;

    if (mvPpSwitchTx(dev, pktInfoP, txQ) != MV_OK)
    {
        mvOsPrintf("%s: mvPpSwitchTx failed.\n", __func__);
        return MV_FAIL;
    }

    return MV_OK;
}

/*******************************************************************************
 * mvPpIsTxDone
 */
MV_BOOL mvPpIsTxDone(MV_U32 txQ)
{
    MV_U32 regVal;
    MV_U32 dev;

    dev = mvPpQ2Dev(txQ);
    if (dev >= 1 /* mvSwitchGetDevicesNum() */)
    {
        mvOsPrintf("%s: Wrong dev (%d).\n", __func__, dev);
        return MV_FALSE;
    }
    txQ = txQ - NUM_OF_RX_QUEUES * dev;

    CHECK_STATUS(hwIfGetReg(dev, 0x2868, &regVal));

    regVal &= (1 << txQ);
    if (regVal == 0)
    {
        return MV_TRUE;
    }

    return MV_FALSE;
}

/*******************************************************************************
 * mvPpRxResourceGet
 */
MV_U32 mvPpRxResourceGet(MV_U32 rxQ)
{
    RX_DESC_LIST    *rxDescList;
    MV_U32           dev;

    dev = mvPpQ2Dev(rxQ);
    if (dev >= 1 /* mvSwitchGetDevicesNum() */)
    {
        mvOsPrintf("%s: Wrong dev (%d).\n", __func__, dev);
        return MV_FAIL;
    }
    rxQ = rxQ - NUM_OF_RX_QUEUES * dev;

    rxDescList = &(G_sdmaInterCtrl[dev].rxDescList[rxQ]);
    return rxDescList->rxResource;
}

/*******************************************************************************
 * mvPpEthInit
 */
MV_STATUS mvPpEthInit(MV_ETH_INIT *ethInitP)
{
    MV_PP_HAL_CTRL *ctrlP   = G_mvPpHalCtrlP;
    GEN_POOL       *rxPoolP = NULL;
    GEN_POOL       *txPoolP = NULL;
    MV_U32         dev, poolSize;

    for (dev = PP_DEV0; dev < 1 /* mvSwitchGetDevicesNum() */; dev++)
    {
        if (mvPpSwitchInit(dev) != MV_OK)
        {
            mvOsPrintf("%s: mvPpSwitchInit failed.\n", __func__);
            return MV_FAIL;
        }

        /*
         * Init RX pktInfo pool.
         */
        poolSize = NUM_OF_RX_QUEUES * PRESTERA_RXQ_LEN;
        rxPoolP = genPoolCreate(poolSize);
        if (rxPoolP == NULL)
        {
            mvOsPrintf("%s:%d: genPoolCreate failed.\n", __func__, __LINE__);
            return MV_FAIL;
        }
        mvPpRxPktInfoPoolP[dev] = rxPoolP;

        /*
         * Init TX pktInfo pool.
         */
        txPoolP = genPoolCreate(ethInitP->txDescTotal);
        if (txPoolP == NULL)
        {
            mvOsPrintf("%s:%d: genPoolCreate failed.\n", __func__, __LINE__);
            return MV_FAIL;
        }
        ctrlP->txPktInfoPoolP = txPoolP;

        if (mvPpInitDescRings(dev, ethInitP) != MV_OK)
        {
            mvOsPrintf("%s: mvPpInitDescRings failed.\n", __func__);
            return MV_FAIL;
        }
    }

    return MV_OK;
}

/*******************************************************************************
 * mvPpPortEnable
 */
MV_STATUS mvPpPortEnable()
{
    MV_U32 dev, tmpData;

    for (dev = PP_DEV0; dev < 1 /* mvSwitchGetDevicesNum() */; dev++)
    {
        /* Enable Rx DMA */
        CHECK_STATUS(hwIfGetReg(dev, 0x2680, &tmpData))
        tmpData |= (1 << 0);
        tmpData &= ~(1 << (0 + 8));
        CHECK_STATUS(hwIfSetReg(dev, 0x2680, tmpData))
    }

    return MV_OK;
}

/*******************************************************************************
 * mvPpMacAddrSet
 */
MV_STATUS mvPpMacAddrSet(MV_U8 *macAddr, MV_U32 queue)
{

    MV_U32 dev;
    MV_8   macAddrDot[18];

    dev = mvPpQ2Dev(queue);
    if (dev >= 1 /* mvSwitchGetDevicesNum() */)
    {
        mvOsPrintf("%s: Wrong dev (%d).\n", __func__, dev);
        return MV_FAIL;
    }

    sprintf(macAddrDot,"%02x:%02x:%02x:%02x:%02x:%02x",
                                            macAddr[0],
                                            macAddr[1],
                                            macAddr[2],
                                            macAddr[3],
                                            macAddr[4],
                                            macAddr[5]);

    if (setCPUAddressInMACTAble(dev, (MV_U8 *)macAddrDot, VID(1)) != MV_OK)
    {
        mvOsPrintf("%s: setCPUAddressInMACTAble failed.\n", __func__);
        return MV_FAIL;
    }

    return MV_OK;
}

/*******************************************************************************
 * DEBUG functions
 ******************************************************************************/

/*******************************************************************************
 * preDebugFunc
 */
void  preDebugFunc(void)
{
    /* set MAC SA DA counters */
    CHK_STS_VOID(hwIfSetReg(0,0x020400B0,0x78145900))
    CHK_STS_VOID(hwIfSetReg(0,0x020400B4,0x4bcd0045))
    CHK_STS_VOID(hwIfSetReg(0,0x020400B8,0x001641a9))

    /* set vlan 1 */
    CHK_STS_VOID(hwIfSetReg(0,0x01800140,0x402))
    CHK_STS_VOID(hwIfSetReg(0,0x01800160,0x402))
}

/*******************************************************************************
 * preDebugFuncVid
 */
void preDebugFuncVid(int vid)
{
    MV_U32 data = 0;
    CHK_STS_VOID(hwIfGetReg(0,0x01800140,&data))

    data &= ~0x3FFC02;

    data |= 2;
    data |= (vid << 10);
    CHK_STS_VOID(hwIfSetReg(0,0x01800140,data))
    CHK_STS_VOID(hwIfSetReg(0,0x01800160,data))
}

/*******************************************************************************
 * preDebugFuncPort
 */
void preDebugFuncPort(int dev)
{
    MV_U32 data = 0;
    CHK_STS_VOID(hwIfGetReg(0,0x01800140,&data))

    data &= ~0x3F1;

    data |= 1;
    data |= (dev << 4);

    CHK_STS_VOID(hwIfSetReg(0,0x01800140,data))
    CHK_STS_VOID(hwIfSetReg(0,0x01800160,data))
}

/*******************************************************************************
 * swRegRd
 */
MV_U32 swRegRd(MV_U32 dev, MV_U32 regAddr)
{
    MV_U32 dataReg = 0xABCDEBBE; /* magic */
    if (hwIfGetReg(dev, regAddr, &dataReg) != MV_OK)
    {
        mvOsPrintf("%s:%d: hwIfGetReg failed.\n", __func__, __LINE__);
    }
    return dataReg;
}

/*******************************************************************************
 * swRegRdMany
 */
void swRegRdMany(int dev, MV_U32 startRegAddr, int regsNum)
{
    int i;
    for (i = 0; i < regsNum; i++)
    {
        mvOsPrintf("0x%08X: 0x%08X\n", startRegAddr, swRegRd(dev, startRegAddr));
        startRegAddr += 4;
    }
}

/*******************************************************************************
 * swPrintSdmaConfig
 */
void swPrintSdmaConfig(int dev)
{
    mvOsPrintf("------- SDMA Configuration -------------------------\n");
    mvOsPrintf("SDMA Configuration:                0x%08X\n",
               swRegRd(dev, PRESTERA_SDMA_CFG_REG));
}

/*******************************************************************************
 * swPrintConfig
 */
void swPrintConfig(int dev)
{
    mvOsPrintf("####### Configuration #################################\n");
    mvOsPrintf("CPU Port Global Configuration:     0x%08X\n",
               swRegRd(dev, 0xA0));
    mvOsPrintf("Brigde Global Configuration 1 Reg: 0x%08X\n",
               swRegRd(dev, 0x2040004));

    swPrintSdmaConfig(dev);

    mvOsPrintf("------- Layer 2 Brigde MIB Counters ----------------\n");
    mvOsPrintf("Counters Set<0> Cfg Reg:           0x%08X\n",
               swRegRd(dev, 0x20400DC));
    mvOsPrintf("Counters Set<1> Cfg Reg:           0x%08X\n",
               swRegRd(dev, 0x20400F0));
}

/*******************************************************************************
 * swPrintSdmaCounters
 */
void swPrintSdmaCounters(int dev)
{
    mvOsPrintf("------- SDMA -------------------------------\n");
    mvOsPrintf("RX SDMA Status:                    0x%08X\n",
               swRegRd(dev, PRESTERA_SDMA_RX_STATUS_REG));
    mvOsPrintf("RX SDMA Current Desc:              0x%08X\n",
               swRegRd(dev, PRESTERA_SDMA_RX_CURR_DESC_REG(0)));
    mvOsPrintf("RX SDMA Packets (ROC):             0x%08X\n",
               swRegRd(dev, PRESTERA_SDMA_RX_PKTS_REG));
    mvOsPrintf("RX SDMA Bytes (ROC):               0x%08X\n",
               swRegRd(dev, PRESTERA_SDMA_RX_BYTES_REG));
    mvOsPrintf("TX SDMA Current Desc:              0x%08X\n",
               swRegRd(dev, PRESTERA_SDMA_TX_CURR_DESC_REG(0)));
}

/*******************************************************************************
 * mvPpRxSdmaCurrDescGetSoft
 */
STRUCT_RX_DESC *mvPpRxSdmaCurrDescGetSoft(MV_U8 dev, MV_U8 rxQ)
{
    RX_DESC_LIST        *rxDescList;
    STRUCT_SW_RX_DESC   *softDescP;
    STRUCT_RX_DESC      *hardDescP;

    rxDescList = &G_sdmaInterCtrl[dev].rxDescList[rxQ];

    softDescP = rxDescList->next2Receive;
    hardDescP = softDescP->rxDesc;

    return hardDescP;
}

/*******************************************************************************
 * mvPpTxSdmaCurrDescGetSoft
 */
STRUCT_TX_DESC *mvPpTxSdmaCurrDescGetSoft(MV_U8 dev, MV_U8 txQ)
{
    TX_DESC_LIST        *txDescList;
    STRUCT_SW_TX_DESC   *softDescP;
    STRUCT_TX_DESC      *hardDescP;

    txDescList = &G_sdmaInterCtrl[dev].txDescList[txQ];

    softDescP = txDescList->next2Feed;
    hardDescP = softDescP->txDesc;

    return hardDescP;
}

/*******************************************************************************
 * mvPpRxSdmaCurrDescGetHard
 */
STRUCT_RX_DESC *mvPpRxSdmaCurrDescGetHard(MV_U8 dev, MV_U8 rxQ)
{
    MV_U32 regVal;

    regVal = mvPpReadReg(dev, PRESTERA_SDMA_RX_CURR_DESC_REG(rxQ));

    return (STRUCT_RX_DESC *)regVal;
}

/*******************************************************************************
 * mvPpTxSdmaCurrDescGetHard
 */
STRUCT_TX_DESC *mvPpTxSdmaCurrDescGetHard(MV_U8 dev, MV_U8 txQ)
{
    MV_U32 regVal;

    regVal = mvPpReadReg(dev, PRESTERA_SDMA_TX_CURR_DESC_REG(txQ));

    return (STRUCT_TX_DESC *)regVal;
}

/*******************************************************************************
 * ppDbgPrintSdmaStatus
 */
void ppDbgPrintSdmaStatus(void)
{
    MV_U32               dev = PP_DEV0;
    MV_U32               rxQ = 0;

    mvOsPrintf("------- SDMA Status ------------------------\n");
    mvOsPrintf("RX SDMA Current Desc (HARD):              0x%08x\n",
               (MV_U32)mvPpRxSdmaCurrDescGetHard(dev, rxQ));
    mvOsPrintf("RX SDMA Current Desc (SOFT):              0x%08x\n",
               (MV_U32)mvPpRxSdmaCurrDescGetSoft(dev, rxQ));

    mvOsPrintf("TX SDMA Current Desc (HARD):              0x%08x\n",
               (MV_U32)mvPpTxSdmaCurrDescGetHard(dev, rxQ));
    mvOsPrintf("TX SDMA Current Desc (SOFT):              0x%08x\n",
               (MV_U32)mvPpTxSdmaCurrDescGetSoft(dev, rxQ));

    mvOsPrintf("RX SDMA Packets (ROC):                    0x%08x\n",
               mvPpReadReg(dev, PRESTERA_SDMA_RX_PKTS_REG));
    mvOsPrintf("RX SDMA Bytes (ROC):                      0x%08x\n",
               mvPpReadReg(dev, PRESTERA_SDMA_RX_BYTES_REG));
}

/*******************************************************************************
 * swPrintCounters
 */
void swPrintCounters(int dev)
{
    MV_U32 data[10] = {0};
    MV_U32 dataReg;

    swPrintSdmaCounters(dev);
    mvOsPrintf("------- Switch RGMII Port MIB Counters ----------------------------------------\n");
    mvOsPrintf("GoodFramesSent (ROC):              0x%8X\n", swRegRd(dev, 0x60));
    mvOsPrintf("MACTransErrorFramesSent:           0x%8X\n", swRegRd(dev, 0x64));
    mvOsPrintf("GoodOctetsSent (ROC):              0x%8X\n", swRegRd(dev, 0x68));
    mvOsPrintf("Rx Internal Drop:                  0x%8X\n", swRegRd(dev, 0x6C));
    mvOsPrintf("GoodFramesReceived:                0x%8X\n", swRegRd(dev, 0x70));
    mvOsPrintf("BadFramesReceived:                 0x%8X\n", swRegRd(dev, 0x74));
    mvOsPrintf("GoodOctetsReceived:                0x%8X\n", swRegRd(dev, 0x78));
    mvOsPrintf("BadOctetsReceived:                 0x%8X\n", swRegRd(dev, 0x7C));
    mvOsPrintf("BrigdeDropCnt:                     0x%8X\n", swRegRd(dev, 0x02040150));
    mvOsPrintf("Security Breach Status2 Reg:       0x%8X\n", swRegRd(dev, 0x020401A8));

    mvOsPrintf("------- Layer 2 Brigde MIB Counters -------------------------------------------\n");
    mvOsPrintf("Set<0> Incoming Pkt Count:         0x%8X\n", swRegRd(dev, 0x020400E0));
    mvOsPrintf("Set<1> Incoming Pkt Count:         0x%8X\n", swRegRd(dev, 0x020400F4));

    mvOsPrintf("------- Previous Counters -----------------------------------------------------\n");
    mvOsPrintf("Global int:                        0x%8X\n", swRegRd(dev, 0x30));

    CHK_STS_VOID(hwIfGetReg(0, 0x34, &dataReg))
    mvOsPrintf("Mask int:                          0x%8X\n", dataReg);

    CHK_STS_VOID(hwIfGetReg(0, 0x280C, &dataReg))
    mvOsPrintf("Rx SDMA int:                       0x%8X\n", dataReg);

    CHK_STS_VOID(hwIfGetReg(0, 0x2814, &dataReg))
    mvOsPrintf("Mask:                              0x%8X\n", dataReg);

    CHK_STS_VOID(hwIfGetReg(0, 0x2810, &data[0]))
    CHK_STS_VOID(hwIfGetReg(0, 0x2818, &data[1]))
    mvOsPrintf("Tx SDMA int:                       0x%8X\n", data[0]);
    mvOsPrintf("Mask:                              0x%8X\n", data[1]);

    CHK_STS_VOID(hwIfGetReg(0, 0x0000260C, &data[0]))
    CHK_STS_VOID(hwIfGetReg(0, 0x00002680, &data[1]))
    mvOsPrintf("Rx Current desc:                   0x%8X\n", data[0]);
    mvOsPrintf("QueueEn                            0x%8X\n", data[1]);

    CHK_STS_VOID(hwIfGetReg(0, 0x0000281C, &data[0]))
    CHK_STS_VOID(hwIfGetReg(0, 0x00002820, &data[1]))
    mvOsPrintf("Rx Status (pending = 0x1):         0x%8X\n", data[0]);
    mvOsPrintf("Rx SDMA Packet Counters:           0x%8X\n", data[1]);

    mvOsPrintf("\n");
    mvOsPrintf("------- Egress MIB ------------------------------------------------------------\n");
    CHK_STS_VOID(hwIfGetReg(0, 0x01B40144, &data[0]))
    mvOsPrintf("\n");

    CHK_STS_VOID(hwIfGetReg(0, 0x01B40148, &data[1]))
    CHK_STS_VOID(hwIfGetReg(0, 0x01B4014c, &data[2]))
    CHK_STS_VOID(hwIfGetReg(0, 0x01B40150, &data[3]))
    CHK_STS_VOID(hwIfGetReg(0, 0x01B40154, &data[4]))
    CHK_STS_VOID(hwIfGetReg(0, 0x01B40158, &data[5]))
    CHK_STS_VOID(hwIfGetReg(0, 0x01B4015c, &data[6]))
    CHK_STS_VOID(hwIfGetReg(0, 0x01B40180, &data[7]))
    mvOsPrintf("Uni 0x%x "
               "MC 0x%x "
               "BC 0x%x \n"
               "filter 0x%x "
               "tail drop 0x%x "
               "control 0x%x "
               "drop 0x%x "
               "FIFO drop 0x%x",
               data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7]);

    mvOsPrintf("\nMT counters:\n");

    CHK_STS_VOID(hwIfGetReg(0, 0x020400BC, &data[0]))
    CHK_STS_VOID(hwIfGetReg(0, 0x020400C0, &data[1]))
    mvOsPrintf("MAC DA:                            0x%X\n", data[0]);
    mvOsPrintf("MAC SA:                            0x%X\n", data[1]);

    CHK_STS_VOID(hwIfGetReg(0, 0x020400E0, &data[0]))
    mvOsPrintf("Host Incoming Packet Count:        0x%X\n", data[0]);

    CHK_STS_VOID(hwIfGetReg(0, 0x020400E4, &data[0]))
    mvOsPrintf("Set Vlan Ingress Filtered Pkt Cnt: 0x%X\n", data[0]);

    CHK_STS_VOID(hwIfGetReg(0, 0x020400E8, &data[0]))
    CHK_STS_VOID(hwIfGetReg(0, 0x020400EC, &data[1]))
    mvOsPrintf("Security Filtered Packet Count:    0x%X\n", data[0]);
    mvOsPrintf("Bridge   Filtered Packet Count:    0x%X\n", data[1]);

    CHK_STS_VOID(hwIfGetReg(0, 0x02040000, &data[0]))
    mvOsPrintf("Bridge Global Configuration 0:     0x%X\n", data[0]);
}

/*******************************************************************************
 * mvSwitchCheckPortStatus
 */
MV_BOOL mvSwitchCheckPortStatus(int devNum, int portNum)
{
    unsigned long regVal = 0;
    if (mvSwitchReadReg (devNum,
                         PRESTERA_PORT_STATUS_REG + (portNum * 0x400),
                         (MV_U32 *)&regVal) != MV_OK)
    {
        return MV_FALSE;
    }

    return (regVal & 0x1) ? MV_TRUE : MV_FALSE;
}

/*******************************************************************************
 * mvSwitchIsAnyLinkUp
 */
MV_BOOL mvSwitchIsAnyLinkUp(void)
{
    int dev, port, portNums = mvPpNetPortsNumGet();
    unsigned long regVal = 0, isLinkUpBit;
    MV_U32 numOfDevs = mvPpDevNumGet();

    for (dev = 0; dev < numOfDevs; dev++)
    {
    for (port = 0; port < portNums; port++)
    {
        regVal = mvPpReadReg (dev, PRESTERA_PORT_STATUS_REG + (port * 0x400));

        isLinkUpBit = regVal & BIT_0;
        DB( mvOsPrintf("%s: %02d: isLinkUpBit = %d.\n",
                       __func__, port, isLinkUpBit));
        if (isLinkUpBit != 0)
        {
            return MV_TRUE;
        }
    }
    }

    return MV_FALSE;
}

/*******************************************************************************
 * mvSwitchFirstPortLinkUpGet
 */
MV_U32 mvSwitchFirstPortLinkUpGet(void)
{
    MV_U32 port, portNums = mvPpNetPortsNumGet();
    MV_U32 dev, numOfDevs = mvPpDevNumGet();
    MV_U32 regVal = 0, isLinkUpBit;
    MV_U32 portWithLinkUp = -1;

    for (dev = 0; dev < numOfDevs; dev++)
    {
    for (port = 0; port < portNums; port++)
    {
        regVal = mvPpReadReg (dev, PRESTERA_PORT_STATUS_REG + (port * 0x400));

        isLinkUpBit = regVal & BIT_0;

        if (isLinkUpBit != 0)
        {
                portWithLinkUp = port + dev * portNums;
            break;
        }
    }
    }

    return portWithLinkUp;
}

/*******************************************************************************
 * mvEFuseConfig
 *     Should be done right after opening the memory window to Prestera Switch
 *     and BEFORE any traffic.
 */
MV_VOID mvEFuseConfig(MV_VOID)
{
    MV_U32 dev;

    for (dev = 0; dev < mvSwitchGetDevicesNum(); dev++)
    {
        if (mvSwitchWriteReg(dev, 0x03000060, 0Xffff0011) != MV_OK)
        {
            mvOsPrintf("%s:%d: mvSwitchWriteReg failed.\n", __func__, __LINE__);
            return;
        }

        if (mvSwitchWriteReg(dev, 0x03000200, 0X000007ff) != MV_OK)
        {
            mvOsPrintf("%s:%d: mvSwitchWriteReg failed.\n", __func__, __LINE__);
            return;
        }

        if (mvSwitchWriteReg(dev, 0x03000060, 0Xffff0010) != MV_OK)
        {
            mvOsPrintf("%s:%d: mvSwitchWriteReg failed.\n", __func__, __LINE__);
            return;
        }
    }
}

/*******************************************************************************
 * mvPpWriteLogPrint
 */
void mvPpWriteLogPrint(void)
{
    MV_PP_HAL_CTRL *ctrlP = G_mvPpHalCtrlP;

    if (mvPpHalIsInited() == MV_FALSE)
    {
        mvOsPrintf("%s: PP HAL is not initialized.\n", __func__);
        return;
    }

    mvWriteLogPrint(ctrlP->ppLogHandleP);
}

/*******************************************************************************
* mvPpChipIsXCat2
*
* DESCRIPTION:
*       Checks if the chip type is xCat2.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_TRUE     - if the chip is xCat2.
*       MV_FALSE    - if the chip is xCat (e.g. xCat-A1/A2).
*
* COMMENTS:
*       For xCat2:
*       Rev_id = 1 hard coded
*       Dev_id:
*           dev_id[15:10]    = 6'b111001 (6h39)
*           dev_id[9 :4]     = device_mode[5:0] (sample at reset)
*           dev_id[3]        = device_mode[6] (sample at reset)
*           dev_id[2]        = device_mode[7] (sample at reset)
*           dev_id[1]        = bga_pkg  - package
*           dev_id[0]        = ssmii_mode - package
*       As you can see, dev_id[15:10] is not board dependent
*       and distinguishes xcat from xcat2 (xcat has 0x37 on these bits).
*       dev_id[1:0] are also not board dependent and tells you
*       which package and FE/GE device is on board.
*       Of course, for CPSS you must follow the device matrix excel
*       which maps all the dev_id bits.
*
*       Note: this function may be called on the very early boot stage, ==>
*             printf() should not be used.
*
*******************************************************************************/
MV_BOOL mvPpChipIsXCat2(void)
{
    MV_U32 devId, chipType;
    MV_BOOL isXCat2;

    devId = mvPpReadReg(PP_DEV0, PRESTERA_DEV_ID_REG);
    chipType = (devId & MV_PP_CHIP_TYPE_MASK) >> MV_PP_CHIP_TYPE_OFFSET;

    if (chipType == MV_PP_CHIP_TYPE_XCAT)
    {
        isXCat2 = MV_FALSE;
    }
    else if (chipType == MV_PP_CHIP_TYPE_XCAT2)
    {
        isXCat2 = MV_TRUE;
    }
    else
    {
        /* Default value. This code should be unreacheable. */
        /*
         * mvOsPrintf("%s: wrong chip type (devId = 0x%08X.\n",
         *            __func__, devId);
         */
        isXCat2 = MV_FALSE;
    }

    return isXCat2;
}

/*******************************************************************************
* mvPpChipIsXCat
*
* DESCRIPTION:
*       Checks if the chip type is xCat.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_TRUE     - if the chip is xCat
*       MV_FALSE    - if the chip is xCat2
*
* COMMENTS:
*       See mvPpChipIsXCat2()
*
*******************************************************************************/
MV_BOOL mvPpChipIsXCat(void)
{
    return !mvPpChipIsXCat2();
}

MV_BOOL mvPpChipIsXCat2Simple(void)
{
    MV_U32 devId, chipType;
    MV_BOOL isXCat2;

    devId = *(MV_U32 *)0xf400004c;
    devId = MV_32BIT_LE_FAST(devId);
    chipType = (devId & MV_PP_CHIP_TYPE_MASK) >> MV_PP_CHIP_TYPE_OFFSET;

    if (chipType == MV_PP_CHIP_TYPE_XCAT)
    {
        isXCat2 = MV_FALSE;
    }
    else if (chipType == MV_PP_CHIP_TYPE_XCAT2)
    {
        isXCat2 = MV_TRUE;
    }
    else
    {
        /* Default value. This code should be unreacheable. */
        /*
         * mvOsPrintf("%s: wrong chip type (devId = 0x%08X.\n",
         *            __func__, devId);
         */
        isXCat2 = MV_FALSE;
    }

    return isXCat2;
}

/*******************************************************************************
 * mvPpGetDevId
 */
MV_U32 mvPpGetDevId(void)
{
    MV_U32 devIdReg = 0;

    /* Read deviceId register */
    devIdReg  = *(MV_U32*)(PP_DEV0_BASE + 0x4C);
    devIdReg  = MV_32BIT_LE(devIdReg);

    return devIdReg;
}

/*******************************************************************************
 * mvPpGetXcatChipRev
 */
MV_U32 mvPpGetXcatChipRev(void)
{
    MV_U32 devIdReg = 0;

    /* Read deviceId register */
    devIdReg  = *(MV_U32*)(PP_DEV0_BASE + 0x4C);
    devIdReg  = MV_32BIT_LE(devIdReg);
    devIdReg &= 0xF;

    return devIdReg;
}

/*******************************************************************************
 * mvPpGetXcat2ChipRev
 */
MV_U32 mvPpGetXcat2ChipRev(void)
{
    /* Read DeviceId status register. */
    MV_U32 revId = mvPpReadReg(0, 0x8f8240);

    /* Read JTAG Revision status register. */
    revId >>= 20;
    revId &= 0xF;
    revId -= 1;

    return revId;
}

/*******************************************************************************
 * mvPpGetChipRev
 */
MV_U32 mvPpGetChipRev(void)
{
    MV_U32 rev;

    if (mvPpChipIsXCat2() == MV_TRUE)
    {
        rev = mvPpGetXcat2ChipRev();
    }
    else
    {
        rev = mvPpGetXcatChipRev();
    }

    return rev;
}

/*******************************************************************************
* mvPpIsChipFE
*
* DESCRIPTION:
*       Checks if the chip is of FE type.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       Returns switch chip revision.
*
* COMMENTS:
*       Valid for xCat and xCat2 chips.
*
*******************************************************************************/
MV_BOOL mvPpIsChipFE(void)
{
    MV_U32 devIdReg = 0;

    /* Read deviceId register */
    devIdReg   = *(MV_U32*)(PP_DEV0_BASE + 0x4C);
    devIdReg   = MV_32BIT_LE(devIdReg);
    devIdReg  &= 0x10;
    devIdReg >>= 4;

    return devIdReg;
}

/*******************************************************************************
* mvPpIsChipGE
*
* DESCRIPTION:
*       Checks if the chip is of GE type.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       Returns switch chip revision.
*
* COMMENTS:
*       Valid for xCat and xCat2 chips.
*
*******************************************************************************/
MV_BOOL mvPpIsChipGE(void)
{
    return !mvPpIsChipFE();
}

/*******************************************************************************
* mvPpFtdllWaXcat2
*
* DESCRIPTION:
*       RGMII WA - CPU_Subsystem port enable.
*       This WA enables OOB port.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       None.
*
* COMMENTS:
*       Relevant only for the case of xCat2 chip and OOB PHY-1116.
*
*******************************************************************************/
void mvPpFtdllWaXcat2(void)
{
    MV_U32 i;

    if (mvPpChipIsXCat2() == MV_FALSE)
    {
        /* FTDLL WA is relevant for xCat2 only. */
        return;
    }

    /*
     * stack_ports_dp_sync_fifo_bypass=disable, dp_clk_sel_forSP=dp_clk,
     * ref_clk_125 is taken from internal PLL (for RGMII WA).
     */
    mvPpWriteReg(PP_DEV0, 0x0000005c, 0x00C03405);
    /* RGMII WA - CPU_Subsystem port enable. */
    mvPpWriteReg(PP_DEV0, 0x000000B0, 0x00000681);

    for (i = 0; i < 128; i++)
    {
        /* writing 128 increments to reach max FTDLL. */
        mvPpWriteReg(PP_DEV0, 0x0000000C, 0x5C6FB191);
        /* increase in order to achieve timing. */
        mvPpWriteReg(PP_DEV0, 0x0000000C, 0x5C6FB181);
    }
}

/*******************************************************************************
* mvPpMaskAllInts
*
* DESCRIPTION:
*       Masks all interrupts from switch.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       None.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
void mvPpMaskAllInts(void)
{
    MV_U32 queue, dev;

    for (queue = 0; queue < NUM_OF_RX_QUEUES; queue++)
    {
        mvPpRxReadyIntMask(queue);
    }

    for (queue = 0; queue < NUM_OF_TX_QUEUES; queue++)
    {
        mvPpTxDoneIntMask(queue);
    }

    /* Mask Global Interrupts */
    for (dev = 0; dev < 1 /* mvSwitchGetDevicesNum() */; dev++)
    {
        mvPpWriteReg(dev, 0x34, 0);
    }
}

/*******************************************************************************
 * mvPpIsRxReadyIntPending
 */
MV_BOOL mvPpIsRxReadyIntPending(void)
{
    MV_U32 regVal = mvPpReadReg(MV_PP_DEV0, SWITCH_GLOBAL_CAUSE_REG);

    if ((regVal & BIT20))
    {
        return MV_TRUE;
    }

    return MV_FALSE;
}

/*******************************************************************************
 * mvPpIsTxDoneIntPending
 */
MV_BOOL mvPpIsTxDoneIntPending(void)
{
    MV_U32 regVal = mvPpReadReg(MV_PP_DEV0, SWITCH_GLOBAL_CAUSE_REG);

    if ((regVal & BIT19))
    {
        return MV_TRUE;
    }

    return MV_FALSE;
}


