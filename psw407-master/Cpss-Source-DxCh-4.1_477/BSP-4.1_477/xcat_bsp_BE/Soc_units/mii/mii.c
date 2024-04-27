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

#include "mvSysHwConfig.h"
#include "mii.h"
#include "eth/mvEth.h"
#include "eth/gbe/mvEthGbe.h"
#include "mvPrestera.h"
#include "mvSysGbe.h"

/*******************************************************************************
 * Debug
 */
#if defined DEBUG_MII_GND
    #define DB(x) x
#else
    #define DB(x)
#endif

extern MV_BOOL standalone_network_device;
extern MV_BOOL g_toCfgLinkParams;

/*******************************************************************************
 * Defines
 */

#define MII_BUFF_ALIGN                  8
#define MII_BUFF_MIN_SIZE               8
#define MII_ETH_PORT_ENABLE_TRIALS      50

/*
 * MII_PORT_CTRL (struct)
 *
 * Description:
 *      Internal MII HAL Control structure may hold various MII HAL stuff and
 *      handler(s) to lower software layers. It is allocated and initialized on
 *      MII HAL initialization.
 *
 * Note:
 *      None.
 *
 */
typedef struct
{
    MV_VOID   *pEthPortHndl;
    MV_U32     gbePortNum;
} MII_PORT_CTRL;

/*******************************************************************************
 * Globals
 */

MII_PORT_CTRL *G_miiPortCtrlP = NULL;

/*******************************************************************************
 * miiDefaultRxBuffSizeGet
 */
MV_U32 miiDefaultRxBuffSizeGet(MV_VOID)
{
    return MII_RX_BUFF_SIZE_DEFAULT;
}

/*******************************************************************************
 * miiEthPhysInit
 */
MV_VOID miiEthPhysInit(MV_VOID)
{
    MII_PORT_CTRL *portCtrlP = G_miiPortCtrlP;

    /*
     * In case GbE was previously used, it should be re-enabled
     */
    mvCtrlPwrClckSet (ETH_GIG_UNIT_ID, portCtrlP->gbePortNum, MV_TRUE);
    mvCtrlPwrMemSet (ETH_GIG_UNIT_ID, portCtrlP->gbePortNum, MV_TRUE);
    mvOsDelay (5);
    mvEthPortDisableSimple (portCtrlP->gbePortNum);
    mvEthCpuPortPowerUp    (portCtrlP->gbePortNum);
    mvEthWinInit           (portCtrlP->gbePortNum);
    mvEthHalInitPort       (portCtrlP->gbePortNum);
}

/*******************************************************************************
 * miiPortInit
 */
MV_STATUS miiPortInit(MV_ETH_INIT *ethInitP)
{
    MII_PORT_CTRL *portCtrlP = NULL;

    portCtrlP = (MII_PORT_CTRL *)mvOsCalloc(1, sizeof(MII_PORT_CTRL));
    if (portCtrlP == NULL)
    {
        mvOsPrintf("%s: mvOsCalloc failed.\n", __func__);
        return MV_FAIL;
    }
    G_miiPortCtrlP = portCtrlP;

    portCtrlP->gbePortNum = ethInitP->gbeIndex;

    miiEthPhysInit();

    mvEthInit();

    /* Disregard OOB PHY parameters when configuring GbE for ppmii interface. */
    g_toCfgLinkParams = MV_FALSE;
    portCtrlP->pEthPortHndl = mvEthPortInit(portCtrlP->gbePortNum,
                                            ethInitP->ethP);
    if (portCtrlP->pEthPortHndl == NULL)
    {
        mvOsPrintf ("%s: mvEthPortInit failed.\n", __func__);
        return MV_FAIL;
    }

    return MV_OK;
}

/*******************************************************************************
 * miiPortRxDone
 */
MV_STATUS miiPortRxDone(MV_PKT_INFO *pktInfoP, MV_U32 rxQ)
{
    MII_PORT_CTRL *portCtrlP = G_miiPortCtrlP;

    return mvEthPortSgRxDone(portCtrlP->pEthPortHndl, rxQ, pktInfoP);
}

/*******************************************************************************
 * miiPortTxDone
 */
MV_PKT_INFO *miiPortTxDone(MV_U32 txQ)
{
    MII_PORT_CTRL *portCtrlP = G_miiPortCtrlP;

    return mvEthPortTxDone (portCtrlP->pEthPortHndl, txQ);
}

/*******************************************************************************
 * miiPortRx
 */
MV_PKT_INFO *miiPortRx(MV_U32 rxQ)
{
    MII_PORT_CTRL *portCtrlP = G_miiPortCtrlP;

    return mvEthPortSgRx (portCtrlP->pEthPortHndl, rxQ);
}

/*******************************************************************************
 * miiPortTx
 */
MV_STATUS miiPortTx(MV_PKT_INFO *pktInfoP, MV_U32 txQ)
{
    MII_PORT_CTRL *portCtrlP = G_miiPortCtrlP;

    return mvEthPortSgTx (portCtrlP->pEthPortHndl, txQ, pktInfoP);
}

/*******************************************************************************
 * miiMruSet
 *      Configured MRU for all the ports for all the switch devices.
 *      Note: for CPU_Port and GbE (which is connected internally to CPU_Port),
 *      additional 4 bytes should be added to serve Extended DSA tag (8 bytes).
 */
MV_STATUS miiMruSet(MV_U32 mruBytes)
{
    MII_PORT_CTRL *portCtrlP = G_miiPortCtrlP;

    if (portCtrlP->pEthPortHndl == NULL)
    {
        mvOsPrintf ("%s: portCtrlP->pEthPortHndl = NULL.\n", __func__);
        return MV_FAIL;
    }

    /* mruBytes  = mruBytes + 8; */ /* due to DSA tag */

    if (mvEthActualMruSet (portCtrlP->pEthPortHndl, mruBytes) != MV_OK)
    {
        mvOsPrintf ("%s: mvEthActualMruSet(mruBytes=%d) failed.\n",
                __func__, mruBytes);
        return MV_FAIL;
    }

    if (standalone_network_device == MV_TRUE)
    {
        if (mvSwitchMruSetAllPorts(mruBytes) != MV_OK)
        {
            mvOsPrintf("%s: mvSwitchMruSet(mruBytes=%d) failed.\n",
                       __func__, mruBytes);
            return MV_FAIL;
        }
    }

    return MV_OK;
}

/*******************************************************************************
 * miiMruGet
 */
MV_ETH_MRU miiMruGet(MV_VOID)
{
    MII_PORT_CTRL *portCtrlP = G_miiPortCtrlP;

    return mvEthActualMruGet(portCtrlP->gbePortNum);
}

/*******************************************************************************
 * miiMruPrintAllPorts
 */
void miiMruPrintAllPorts(void)
{
    MII_PORT_CTRL *portCtrlP = G_miiPortCtrlP;

    mvOsPrintf("GbE%d port MRU = %d\n", portCtrlP->gbePortNum, miiMruGet());
    mvSwitchMruPrintAllPorts();
}

/*******************************************************************************
 * miiBuffAlignGet
 */
MV_U32 miiBuffAlignGet(MV_VOID)
{
    return MII_BUFF_ALIGN;
}

/*******************************************************************************
 * miiBuffMinSize
 */
MV_U32 miiBuffMinSize(MV_VOID)
{
    return MII_BUFF_MIN_SIZE;
}

/*******************************************************************************
 * miiRxReadyQGet
 */
MV_U32 miiRxReadyQGet(MV_VOID)
{
    MII_PORT_CTRL *portCtrlP = G_miiPortCtrlP;

    return mvEthRxReadyQGet(portCtrlP->gbePortNum);
}

/*******************************************************************************
 * miiTxDoneQGet
 */
MV_U32 miiTxDoneQGet(MV_VOID)
{
    MII_PORT_CTRL *portCtrlP = G_miiPortCtrlP;

    return mvEthTxDoneQGet(portCtrlP->gbePortNum);
}

/*******************************************************************************
 * miiRxReadyIntAck
 */
MV_VOID miiRxReadyIntAck(MV_U32 ackBitMask)
{
    MII_PORT_CTRL *portCtrlP = G_miiPortCtrlP;

    mvEthRxReadyIntAck(portCtrlP->gbePortNum, ackBitMask);
}

/*******************************************************************************
 * miiTxDoneIntAck
 */
MV_VOID miiTxDoneIntAck(MV_U32 ackBitMask)
{
    MII_PORT_CTRL *portCtrlP = G_miiPortCtrlP;

    mvEthTxDoneIntAck(portCtrlP->gbePortNum, ackBitMask);
}

/*******************************************************************************
 * miiRxReadyIntUnmask
 */
MV_VOID miiRxReadyIntUnmask(MV_U32 rxQBitMask)
{
    MII_PORT_CTRL *portCtrlP = G_miiPortCtrlP;

    mvEthRxReadyIntUnmask(portCtrlP->gbePortNum, rxQBitMask);
}

/*******************************************************************************
 * miiTxDoneIntUnmask
 */
MV_VOID miiTxDoneIntUnmask(MV_U32 txQBitMask)
{
    MII_PORT_CTRL *portCtrlP = G_miiPortCtrlP;

    mvEthTxDoneIntUnmask(portCtrlP->gbePortNum, txQBitMask);
}

/*******************************************************************************
 * miiRxReadyIntMask
 */
MV_VOID miiRxReadyIntMask(MV_U32 rxQBitMask)
{
    MII_PORT_CTRL *portCtrlP = G_miiPortCtrlP;

    mvEthRxReadyIntMask(portCtrlP->gbePortNum, rxQBitMask);
}

/*******************************************************************************
 * miiTxDoneIntMask
 */
MV_VOID miiTxDoneIntMask(MV_U32 txQBitMask)
{
    MII_PORT_CTRL *portCtrlP = G_miiPortCtrlP;

    mvEthTxDoneIntMask(portCtrlP->gbePortNum, txQBitMask);
}

/*******************************************************************************
 * miiRxQEnable
 */
MV_VOID miiRxQEnable(MV_U32 rxQ)
{
    MII_PORT_CTRL *portCtrlP = G_miiPortCtrlP;

    mvEthRxQEnable(portCtrlP->gbePortNum, rxQ);
}

/*******************************************************************************
 * miiTxQEnable
 */
MV_VOID miiTxQEnable(MV_U32 txQ)
{
    MII_PORT_CTRL *portCtrlP = G_miiPortCtrlP;

    mvEthTxQEnable(portCtrlP->gbePortNum, txQ);
}

/*******************************************************************************
 * miiRxQDisable
 */
MV_VOID miiRxQDisable(MV_U32 rxQ)
{
    MII_PORT_CTRL *portCtrlP = G_miiPortCtrlP;

    mvEthRxQDisable(portCtrlP->gbePortNum, rxQ);
}

/*******************************************************************************
 * miiTxQDisable
 */
MV_VOID miiTxQDisable(MV_U32 txQ)
{
    MII_PORT_CTRL *portCtrlP = G_miiPortCtrlP;

    mvEthTxQDisable(portCtrlP->gbePortNum, txQ);
}

/*******************************************************************************
 * miiRxQMapGet
 */
MV_ETH_RX_MAPPING miiRxQMapGet(MV_VOID)
{
    MII_PORT_CTRL *portCtrlP = G_miiPortCtrlP;

    return mvEthDsaRxQMapGet(portCtrlP->gbePortNum);
}

/*******************************************************************************
 * miiRxQMapSet
 */
MV_VOID miiRxQMapSet(MV_ETH_RX_MAPPING map)
{
    MII_PORT_CTRL *portCtrlP = G_miiPortCtrlP;

    mvEthDsaRxQMapSet(portCtrlP->gbePortNum, map);
}

/*******************************************************************************
 * miiCpuCodeToRxQMap
 */
MV_STATUS miiCpuCodeToRxQMap(MV_U32 cpuCode, MV_U32 rxQ)
{
    MII_PORT_CTRL *portCtrlP = G_miiPortCtrlP;

    return mvEthCfgDFSMTForCpuCodeToRxQ(portCtrlP->gbePortNum, cpuCode, rxQ);
}

/*******************************************************************************
 * miiAllCpuCodesToRxQMap
 */
MV_STATUS miiAllCpuCodesToRxQMap(MV_U32 rxQ)
{
    MII_PORT_CTRL *portCtrlP = G_miiPortCtrlP;

    return mvEthCfgDFSMTForAllCpuCodes(portCtrlP->gbePortNum, rxQ);
}

/*******************************************************************************
 * miiMacAddrSet
 */
MV_STATUS miiMacAddrSet(MV_U8 *macAddr, MV_U32 queue)
{
    MII_PORT_CTRL *portCtrlP = G_miiPortCtrlP;
    MV_STATUS status;
    MV_U8     macAddrDotFormat[MV_MAC_ADDR_SIZE * 2 + 5];

    status = mvEthMacAddrSetSimple (portCtrlP->gbePortNum, macAddr, queue);
    if (status != MV_OK)
    {
        mvOsPrintf ("%s: mvEthMacAddrSetSimple failed.\n", __func__);
        return MV_FAIL;
    }

    /*
     * Configure CPU MAC address in Prestera Switch.
     */
    sprintf ((MV_8 *)macAddrDotFormat, "%02x:%02x:%02x:%02x:%02x:%02x",
             macAddr[0],
             macAddr[1],
             macAddr[2],
             macAddr[3],
             macAddr[4],
             macAddr[5]);

    if (setCPUAddressInMACTAble (PP_DEV0,
                                 macAddrDotFormat, 1 /* vid */) != MV_OK)
    {
        mvOsPrintf ("%s: Unable to teach CPU MAC addr.\n", __func__);
        return MV_FAIL;
    }

    return MV_OK;
}

/*******************************************************************************
 * miiPortEnable
 */
MV_STATUS miiPortEnable(MV_VOID)
{
    MII_PORT_CTRL *portCtrlP = G_miiPortCtrlP;
    MV_U32 i;

    for (i = 0; i < MII_ETH_PORT_ENABLE_TRIALS; i++)
    {
        if (mvEthCpuPortEnable (portCtrlP->pEthPortHndl) == MV_OK)
        {
            break;
        }
        mvOsDelay (20);
    }

    if (i == MII_ETH_PORT_ENABLE_TRIALS)
    {
        mvOsPrintf ("%s: mvEthPortEnable failed.\n", __func__);
        return MV_FAIL;
    }

    if (mvSwitchCpuPortConfig (PP_DEV0) != MV_OK)
    {
        mvOsPrintf ("%s: mvSwitchCpuPortConfig failed.\n", __func__);
        return MV_FAIL;
    }

    if (standalone_network_device == MV_TRUE)
    {
        if (mvPpSetCpuPortToCascadeMode(PP_DEV0) != MV_OK)
        {
            mvOsPrintf ("%s: mvSwitchCpuPortConfig failed.\n", __func__);
            return MV_FAIL;
        }

        if (mvPpTwoBytePrependDisable() != MV_OK)
        {
            mvOsPrintf ("%s: mvPpTwoBytePrependDisable failed.\n", __func__);
            return MV_FAIL;
        }
    }

    if (mvEthCpuPortConfig (portCtrlP->gbePortNum) != MV_OK)
    {
        mvOsPrintf ("%s: mvEthCpuPortConfig failed.\n", __func__);
        return MV_FAIL;
    }

    /* Config DFOMT, DFSMT and DFUT to accept mode */
    if (mvEthRxFilterModeSet (portCtrlP->pEthPortHndl, MV_TRUE) != MV_OK)
    {
        mvOsPrintf ("%s: mvEthRxFilterModeSet failed.\n", __func__);
        return MV_FAIL;
    }

    MV_REG_BIT_SET (ETH_PORT_CONFIG_REG (portCtrlP->gbePortNum),
                    ETH_UNICAST_PROMISCUOUS_MODE_MASK);

    return MV_OK;
}

/*******************************************************************************
 * miiPortDisable
 */
MV_STATUS miiPortDisable(MV_VOID)
{
    MII_PORT_CTRL *portCtrlP = G_miiPortCtrlP;

    return mvEthPortDisable (portCtrlP->pEthPortHndl);
}

/*******************************************************************************
 * miiIsTxDone
 */
MV_BOOL miiIsTxDone(MV_U32 txQ)
{
    MII_PORT_CTRL *portCtrlP = G_miiPortCtrlP;

    return mvEthIsTxDone (portCtrlP->gbePortNum, txQ);
}

/*******************************************************************************
 * miiTxResourceGet
 */
MV_U32 miiTxResourceGet(MV_U32 txQ)
{
    MII_PORT_CTRL *portCtrlP = G_miiPortCtrlP;

    return mvEthTxResourceGet (portCtrlP->pEthPortHndl, txQ);
}

/*******************************************************************************
 * miiRxResourceGet
 */
MV_U32 miiRxResourceGet(MV_U32 rxQ)
{
    MII_PORT_CTRL *portCtrlP = G_miiPortCtrlP;

    return mvEthRxResourceGet (portCtrlP->pEthPortHndl, rxQ);
}

