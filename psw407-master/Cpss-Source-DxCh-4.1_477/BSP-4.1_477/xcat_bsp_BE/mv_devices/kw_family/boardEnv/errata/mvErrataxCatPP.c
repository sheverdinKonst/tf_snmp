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

#include "mvErrata.h"
#include "mvErrataxCatPP.h"
#include "mvOs.h"
#include "mvSysHwConfig.h"

static void *mvErrataXCatPpHandle;

#define MV_ERRATA_DESCR_XCAT_PP "xCat-PP"

MV_ERRATA_ENTRY ppArray[] = {
    {"Errata for packets length is 256*n+k(Init phase)",NULL},
    {"Errata for packets length is 256*n+k(Buff len fix)",NULL},
    {"Errata for packets length is 256*n+k(Descr fix)",NULL}};

/* Errata functions prototypes. */
INLINE void mvErrataApply_sdma_256_init(void);

INLINE void mvErrataApply_sdma_256_len_fix(
        MV_U32 *pktLen,
        MV_U32 *addBuff,
        MV_U32 *buffLen,
        MV_U32 numOfBufs);

INLINE void mvErrataApply_sdma_256_desc_fix(
        STRUCT_SW_TX_DESC *currSwDesc,
        STRUCT_TX_DESC  *tmpDesc,
        MV_U32 descWord1,
        MV_U32 addBuff);

/*******************************************************************************
* mvErrataXCatPpHandleGet
*/
void *mvErrataXCatPpHandleGet(void)
{
    return mvErrataXCatPpHandle;
}

/*******************************************************************************
 * mvErrataInitXCat2RevA1
 */
MV_STATUS mvErrataInitXCat2RevA1(void)
{
    MV_U32 size;

    size = sizeof(ppArray) / sizeof(MV_ERRATA_ENTRY);
    mvErrataXCatPpHandle = mvErrataInit(ppArray, size, MV_ERRATA_DESCR_XCAT_PP);
    if (mvErrataXCatPpHandle == NULL)
    {
        mvOsPrintf("%s: mvErrataInit failed.\n", __func__);
        return MV_FAIL;
    }

    return MV_OK;
}

/*******************************************************************************
 * mvErrataInitXCat2RevA0
 */
MV_STATUS mvErrataInitXCat2RevA0(void)
{
    MV_U32 size;

    size = sizeof(ppArray) / sizeof(MV_ERRATA_ENTRY);
    mvErrataXCatPpHandle = mvErrataInit(ppArray, size, MV_ERRATA_DESCR_XCAT_PP);
    if (mvErrataXCatPpHandle == 0)
    {
        mvOsPrintf("%s: mvErrataInit failed.\n", __func__);
        return MV_FAIL;
    }

    return MV_OK;
}

/*******************************************************************************
 * mvErrataInitXCatRevA0
 */
MV_STATUS mvErrataInitXCatRevA0(void)
{
    MV_U32 size;

    size = sizeof(ppArray) / sizeof(MV_ERRATA_ENTRY);
    mvErrataXCatPpHandle = mvErrataInit(ppArray, size, MV_ERRATA_DESCR_XCAT_PP);
    if (mvErrataXCatPpHandle == 0)
    {
        mvOsPrintf("%s: mvErrataInit failed.\n", __func__);
        return MV_FAIL;
    }

    return MV_OK;
}

/*******************************************************************************
 * mvErrataInitXCatRevA1
 */
MV_STATUS mvErrataInitXCatRevA1(void)
{
    MV_U32 size;

    ppArray[MV_ERRATA_ID_SDMA_256_INIT].funcP = mvErrataApply_sdma_256_init;
    ppArray[MV_ERRATA_ID_SDMA_256_LEN_FIX].funcP = mvErrataApply_sdma_256_len_fix;
    ppArray[MV_ERRATA_ID_SDMA_256_DESC_FIX].funcP = mvErrataApply_sdma_256_desc_fix;

    size = sizeof(ppArray) / sizeof(MV_ERRATA_ENTRY);
    mvErrataXCatPpHandle = mvErrataInit(ppArray, size, MV_ERRATA_DESCR_XCAT_PP);
    if (mvErrataXCatPpHandle == 0)
    {
        mvOsPrintf("%s: mvErrataInit failed.\n", __func__);
        return MV_FAIL;
    }

    return MV_OK;
}

static MV_U8 *G_8bytePad = NULL;
/*******************************************************************************
* mvErrataApply_sdma_256_init
*
* DESCRIPTION:
*     Pre-allocates memory for padding outgoing packets.
*     Add 8 byte padding (another buffer is added) should
*     be done only if packets length is 256*n+k (1<=k<=8) (including DSA tag),
*     where n and k are integers.
*******************************************************************************/
INLINE void mvErrataApply_sdma_256_init(void)
{
    MV_U32 pPhysAddr = 0;

    G_8bytePad = mvOsIoUncachedMalloc(NULL, 8, (MV_ULONG *)&pPhysAddr, NULL);
    if (G_8bytePad == NULL)
    {
        mvOsPrintf("%s: alloc failed.\n", __func__);
        return;
    }
    mvOsMemset(G_8bytePad, 0x00, 8);
}

/*******************************************************************************
 * mvErrataApply_sdma_256_len_fix
 */
INLINE void mvErrataApply_sdma_256_len_fix(
        MV_U32 *pktLen,
        MV_U32 *addBuff,
        MV_U32 *buffLen,
        MV_U32 numOfBufs)
{
    int i;
    *pktLen = 0;
    for (i = 0; i < numOfBufs; i++)
        *pktLen += buffLen[i];
    *pktLen &= 0xFF;

    if (*pktLen >= 1 && *pktLen <= 8)
        *addBuff = 1;
}

/*******************************************************************************
 * mvErrataApply_sdma_256_desc_fix
 */
INLINE void mvErrataApply_sdma_256_desc_fix(
        STRUCT_SW_TX_DESC *currSwDesc ,
        STRUCT_TX_DESC  *tmpDesc,
        MV_U32 descWord1,
        MV_U32 addBuff)
{
    if (addBuff == 1)
    {
        currSwDesc = currSwDesc->swNextDesc;
        TX_DESC_RESET(tmpDesc);
        tmpDesc->word1 = descWord1;
        tmpDesc->buffPointer = (MV_U32)G_8bytePad; /* DMA safe pad buffer */
        TX_DESC_SET_BYTE_CNT(tmpDesc, 8);
        TX_DESC_SET_OWN_BIT(tmpDesc, MV_OWNERSHIP_DMA);
        tmpDesc->word1 = hwByteSwap(tmpDesc->word1);
        tmpDesc->word2 = hwByteSwap(tmpDesc->word2);
        TX_DESC_COPY(currSwDesc->txDesc, tmpDesc);
    }
}


