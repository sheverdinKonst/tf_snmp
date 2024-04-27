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

#ifndef __INCmiih
#define __INCmiih

#include "mvTypes.h"
#include "mvEth.h"
#include "mvPrestera.h"

/*******************************************************************************
 * Defines
 */

/*******************************************************************************
* miiDefaultRxBuffSizeGet
*
* DESCRIPTION:
*       Return the default of size of MII RX buffer.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       Return the default of size of MII RX buffer.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_U32 miiDefaultRxBuffSizeGet(MV_VOID);

/*******************************************************************************
* miiPortInit
*
* DESCRIPTION:
*       This function initialize the MII port.
*       1) Allocate and initialize internal port Control structure.
*       2) Create RX and TX descriptor rings.
*       3) Disable RX and TX operations, clear cause registers and
*          mask all interrupts.
*       4) Set all registers to default values and clean all MAC tables.
*
* INPUTS:
*       miiPortInitP   - pointer to init structure
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
MV_STATUS miiPortInit(MV_ETH_INIT *ethInitP);

/*******************************************************************************
* miiPortRxDone
*
* DESCRIPTION:
*       This function fills GbE RX SDMA descriptors by
*       (MV_PKT_INFO->MV_BUF_INFO->buff) structures.
*
* INPUTS:
*       None.
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
MV_STATUS miiPortRxDone(MV_PKT_INFO *pktInfoP, MV_U32 rxQ);

/*******************************************************************************
* miiPortTxDone
*
* DESCRIPTION:
*       Returns MV_PKT_INFO representing the transmitted packet.
*
* INPUTS:
*       txQ         -   TX queue
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       On success:
*           MV_PKT_INFO representing the transmitted packet.
*       On failure or no transmitted packets:
*           NULL
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_PKT_INFO *miiPortTxDone(MV_U32 txQ);

/*******************************************************************************
* miiPortRx
*
* DESCRIPTION:
*       Return received on GbE packet (possibly Scatter-Gather)
*
* INPUTS:
*       rxQ         -   RX queue
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       Success:
*           Linked list of MV_PKT_INFO representing the received packet
*       Failure or no received packet:
*           NULL
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_PKT_INFO *miiPortRx(MV_U32 rxQ);

/*******************************************************************************
* miiPortTx
*
* DESCRIPTION:
*       Transmits the packet represented by MV_PKT_INFO structure.
*       The TX packet may be Scatter-Gather or Single-Buffer packet.
*
* INPUTS:
*       txQ         -   TX queue
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
MV_STATUS miiPortTx(MV_PKT_INFO *pktInfoP, MV_U32 txQ);

/*******************************************************************************
* miiMruSet
*
* DESCRIPTION:
*       Sets  MRU configuration from specified RMGII iface
*
* INPUTS:
*       mru       - MRU to configure
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
MV_STATUS miiMruSet(MV_U32 mru);

/*******************************************************************************
* miiMruGet
*
* DESCRIPTION:
*       Gets MRU configuration from specified RMGII iface
*
* INPUTS:
*       mru       - MRU to configure
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
MV_ETH_MRU miiMruGet(MV_VOID);

/******************************************************************************\
 * miiMruPrintAllPorts
 */
void miiMruPrintAllPorts(void);

/*******************************************************************************
* miiBuffAlignGet
*
* DESCRIPTION:
*       The driver should use this RX buffer alignment for RX buffer allocation.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       Alignment for the RX buffer.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_U32 miiBuffAlignGet(MV_VOID);

/*******************************************************************************
* miiBuffMinSize
*
* DESCRIPTION:
*       Return the minimum supported buffer size.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       Return the minimum supported buffer size.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_U32 miiBuffMinSize(MV_VOID);

/*******************************************************************************
* miiRxReadyQGet
*
* DESCRIPTION:
*       Reads appropriate cause register of GbE controller and
*       clears all bits excepts RX_READY queueu bits.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       The mask representing ready RX queues.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_U32 miiRxReadyQGet(MV_VOID);

/*******************************************************************************
* miiTxDoneQGet
*
* DESCRIPTION:
*       Reads appropriate cause register of GbE controller and
*       clears all bits excepts TX_DONE queueu bits.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       The mask representing TX_DONE queues.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_U32 miiTxDoneQGet(MV_VOID);

/*******************************************************************************
* miiRxReadyIntAck
*
* DESCRIPTION:
*       Acknowledges the appropriate interrupts.
*
* INPUTS:
*       ackBitMask  - Bits mask representing the RX Queue to be acknowledged.
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
MV_VOID miiRxReadyIntAck(MV_U32 ackBitMask);

/*******************************************************************************
* miiTxDoneIntAck
*
* DESCRIPTION:
*       Acknowledges the appropriate interrupts.
*
* INPUTS:
*       ackBitMask  - Bits mask representing the TX Queue to be acknowledged.
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
MV_VOID miiTxDoneIntAck(MV_U32 ackBitMask);

/*******************************************************************************
* miiRxReadyIntUnmask
*
* DESCRIPTION:
*       Unmasks RX interrupts on GbE for the given RX queues.
*
* INPUTS:
*       rxQBitMask  - Bits mask representing the RX Queue to be unmasked.
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
MV_VOID miiRxReadyIntUnmask(MV_U32 rxQBitMask);

/*******************************************************************************
* miiTxDoneIntUnmask
*
* DESCRIPTION:
*       Unmasks TX interrupts on GbE for the given RX queues.
*
* INPUTS:
*       txQBitMask  - Bits mask representing the TX Queue to be unmasked.
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
MV_VOID miiTxDoneIntUnmask(MV_U32 txQBitMask);

/*******************************************************************************
* miiRxReadyIntMask
*
* DESCRIPTION:
*       Masks RX interrupts on GbE for the given RX queues.
*
* INPUTS:
*       rxQBitMask  - Bits mask representing the RX Queue to be masked.
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
MV_VOID miiRxReadyIntMask(MV_U32 rxQBitMask);

/*******************************************************************************
* miiTxDoneIntMask
*
* DESCRIPTION:
*       Masks TX interrupts on GbE for the given TX queues.
*
* INPUTS:
*       txQBitMask  - Bits mask representing the TX Queue to be masked.
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
MV_VOID miiTxDoneIntMask(MV_U32 txQBitMask);

/*******************************************************************************
* miiRxQEnable
*
* DESCRIPTION:
*       Enables in hardware designated queue.
*
* INPUTS:
*       rxQ         - RX queue number to enable.
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
MV_VOID miiRxQEnable(MV_U32 rxQ);

/*******************************************************************************
* miiTxQEnable
*
* DESCRIPTION:
*       Enables in hardware designated queue.
*
* INPUTS:
*       txQ         - TX queue number to enable.
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
MV_VOID miiTxQEnable(MV_U32 txQ);

/*******************************************************************************
* miiRxQDisable
*
* DESCRIPTION:
*       Disables in hardware designated queue.
*
* INPUTS:
*       rxQ         - RX queue number to enable.
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
MV_VOID miiRxQDisable(MV_U32 rxQ);

/*******************************************************************************
* miiTxQDisable
*
* DESCRIPTION:
*       Disables in hardware designated queue.
*
*       txQ         - TX queue number to enable.
* INPUTS:
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
MV_VOID miiTxQDisable(MV_U32 txQ);

/*******************************************************************************
* miiRxQMapGet
*
* DESCRIPTION:
*       Returns configured mapping of RX packet to GbE RX queue.
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
MV_ETH_RX_MAPPING miiRxQMapGet(MV_VOID);

/*******************************************************************************
* miiRxQMapSet
*
* DESCRIPTION:
*       Configures the mapping of RX packet to GbE RX queue.
*
* INPUTS:
*       map     - mapping type to configure
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
MV_VOID miiRxQMapSet(MV_ETH_RX_MAPPING map);

/*******************************************************************************
* miiCpuCodeToRxQMap
*
* DESCRIPTION:
*       Configures the mapping of DSA CPU_CODE of incoming packet to GbE
*       RX queue.
*
* INPUTS:
*       rxQ         - RX queue
*       cpuCode     - DSA CPU_CODE of incoming packet
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
MV_STATUS miiCpuCodeToRxQMap(MV_U32 cpuCode, MV_U32 rxQ);

/*******************************************************************************
* miiAllCpuCodesToRxQMap
*
* DESCRIPTION:
*       Configures the mapping of all DSA CPU_CODE of incoming packet to GbE
*       RX queue.
*
* INPUTS:
*       rxQ         - RX queue
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
MV_STATUS miiAllCpuCodesToRxQMap(MV_U32 rxQ);

/*******************************************************************************
* miiMacAddrSet
*
* DESCRIPTION:
*       This function Set the port Ethernet MAC address and registers this
*       MAC in the Prestera Switch.
*
* INPUTS:
*       rxQ         - RX queue
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
MV_STATUS miiMacAddrSet(unsigned char *pAddr, MV_U32 queue);

/*******************************************************************************
* miiPortEnable
*
* DESCRIPTION:
*
* INPUTS:
*
* OUTPUTS:
*
* RETURNS:
*       MV_OK if successful, or
*       MV_FAIL otherwise.
*
* COMMENTS:
*
*******************************************************************************/
MV_STATUS miiPortEnable(void);

/*******************************************************************************
* miiPortDisable
*
* DESCRIPTION:
*
* INPUTS:
*       None.
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
MV_STATUS miiPortDisable(void);

/*******************************************************************************
* miiIsTxDone
*
* DESCRIPTION:
*       Tests if TX operation is done for specific TX queue of GbE.
*
* INPUTS:
*       None.
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
MV_BOOL miiIsTxDone(MV_U32 txQ);

/*******************************************************************************
* miiTxResourceGet
*
* DESCRIPTION:
*       Get number of Free resources in specific TX queue.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       Number of free resources.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_U32 miiTxResourceGet(MV_U32 txQ);

/*******************************************************************************
* miiRxResourceGet
*
* DESCRIPTION:
*       Get number of Free resources in specific RX queue.
*
* INPUTS:
*       None.
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
MV_U32 miiRxResourceGet(MV_U32 rxQ);

#endif /* __INCmiih */

