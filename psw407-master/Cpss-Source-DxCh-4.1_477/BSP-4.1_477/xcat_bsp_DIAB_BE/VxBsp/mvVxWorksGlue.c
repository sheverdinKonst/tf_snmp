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

#include "mvVxWorksGlue.h"
#include "mvOsVxw.h"

/* Convert Mblk-Clblk-Cluster list to BUF_INFO-Buffer list */
MV_STATUS mBlkToPktInfo(M_BLK_ID pMblkPkt, MV_PKT_INFO  *pPktInfo, MV_U32 maxFrags)
{
    M_BLK_ID     pMblk    = NULL;
    MV_BUF_INFO *pBufInfo = pPktInfo->pFrags;
    int          bufCount = 0;

    pMblk = pMblkPkt;

    while (pMblk && pMblk->mBlkHdr.mLen && pMblk->mBlkHdr.mData)
    {
        if (bufCount >= maxFrags)
        {
            mvOsPrintf("%s: Too many frags.\n", __func__);
            return MV_FAIL;
        }
        /* set a BufInfo */
        pBufInfo[bufCount].bufVirtPtr  = pMblk->mBlkHdr.mData;
        pBufInfo[bufCount].dataSize    = pMblk->mBlkHdr.mLen;

        /* move to next Mblk */
        pMblk = pMblk->mBlkHdr.mNext;

        /* move to next BufInfo */
        bufCount++;
    }

    /* Store pointer to Mblk structure in MV_PKT_INFO */
    pPktInfo->osInfo   = (MV_ULONG)pMblkPkt;
    pPktInfo->numFrags = bufCount;
    pPktInfo->pktSize  = pMblkPkt->mBlkPktHdr.len;

    return MV_OK;
}

/* Print Mblk structure */
void mvPrintMblk(M_BLK_ID pMblk)
{
    M_BLK_ID    pMblkPkt = pMblk;

    while(pMblkPkt)
    {
        mvOsPrintf("\n");
        mvOsPrintf("Start of Packet: pMblkPkt=0x%x, pktLen=%d\n",
                (unsigned int)pMblkPkt, pMblkPkt->mBlkPktHdr.len);

        while( pMblk )
        {
            mvOsPrintf("pNetPool=0x%x, pMblk: 0x%x:\n",
                (unsigned int)pMblk->pClBlk->pNetPool, (unsigned int)pMblk);

            mvOsPrintf("pMblk->mBlkHdr.mData         = 0x%x\n",
                (unsigned int)pMblk->mBlkHdr.mData);
            mvOsPrintf("pMblk->mBlkHdr.mFlags        = 0x%x\n",
                (unsigned int)pMblk->mBlkHdr.mFlags);
            mvOsPrintf("pMblk->mBlkHdr.mLen          = %d\n",
                pMblk->mBlkHdr.mLen);
            mvOsPrintf("pMblk->mBlkHdr.mNext         = 0x%x\n",
                (unsigned int)pMblk->mBlkHdr.mNext);
            mvOsPrintf("pMblk->mBlkHdr.mNextPkt      = 0x%x\n",
                (unsigned int)pMblk->mBlkHdr.mNextPkt);
            mvOsPrintf("pMblk->mBlkHdr.mType         = 0x%x\n",
                (unsigned int)pMblk->mBlkHdr.mType);
            mvOsPrintf("pMblk->pClBlk->clNode.pClBuf = 0x%x\n",
                (unsigned int)pMblk->pClBlk->clNode.pClBuf);
            mvOsPrintf("pMblk->pClBlk->clSize        = %d\n",
                pMblk->pClBlk->clSize);
            mvOsPrintf("pMblk->pClBlk->clRefCnt      = %d\n",
                pMblk->pClBlk->clRefCnt);
            mvOsPrintf("pMblk->pClBlk->pClFreeRtn    = 0x%x\n",
                (unsigned int)pMblk->pClBlk->pClFreeRtn);
            mvOsPrintf("pMblk->pClBlk->clFreeArg1    = 0x%x\n",
                (unsigned int)pMblk->pClBlk->clFreeArg1);
            mvOsPrintf("pMblk->pClBlk->clFreeArg2    = 0x%x\n",
                (unsigned int)pMblk->pClBlk->clFreeArg2);
            mvOsPrintf("pMblk->pClBlk->clFreeArg3    = 0x%x\n\n",
                pMblk->pClBlk->clFreeArg3);

            pMblk = pMblk->mBlkHdr.mNext;
            if((pMblk == NULL) || (pMblk->mBlkHdr.mLen == 0))
                break;
        }
        pMblkPkt = pMblkPkt->mBlkHdr.mNextPkt;
    }
    mvOsPrintf("\n");
}



