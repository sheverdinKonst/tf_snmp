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

#include "mvGenPool.h"
#include "mvGenBuffPool.h"
#include "mvOs.h"

#if 0
    #define DB(x) x
#else
    #define DB(x)
#endif

/*******************************************************************************
 * GEN_BUFF_POOL
 */
struct _genBuffPool
{
    GEN_POOL *genPoolP;
    MV_U8    *origBuffBulkP;
    MV_U32    buffSize;
    MV_U32    hdrOffset;
    MV_U32    chunkSize;
    MV_U32    buffSizeMin;
    MV_U32    alignment;
};

/*******************************************************************************
 * genBuffPoolCreate
 */
GEN_BUFF_POOL * genBuffPoolCreate(MV_U32  buffBulkSize,
                                  MV_U8 * buffBulkP,
                                  MV_U32  buffSize,
                                  MV_U32 *actualBuffNumP,
                                  MV_U32  hdrOffset,
                                  MV_U32  alignment,
                                  MV_U32  buffSizeMin)
{
    GEN_BUFF_POOL *poolP = NULL;
    MV_U8         *buffP;
    MV_U32         delta, totalNum, i, chunkSize;

    if (buffBulkP   == NULL            ||
        buffSize     < buffSizeMin     ||
        buffBulkSize < buffSize        ||
        hdrOffset    > buffSize)
    {
        mvOsPrintf ("%s: Bad params.\n", __func__);
        return NULL;
    }

    poolP = (GEN_BUFF_POOL *)mvOsCalloc (1, sizeof(GEN_BUFF_POOL));
    if (poolP == NULL)
    {
        mvOsPrintf ("%s: Alloc failed.\n", __func__);
        return NULL;
    }

    poolP->origBuffBulkP = buffBulkP;

    /* Set the buffers pool to point to a properly alligned block. */
    delta = (MV_U32)buffBulkP % alignment;
    if (delta > 0)
    {
        buffBulkSize -= alignment - delta;
        buffBulkP    += alignment - delta;
    }

    /* The buffer size must be a multiple of alignment. */
    buffSize -= buffSize % alignment;

    /* allign header offset */
    delta = hdrOffset % alignment;
    if (delta != 0)
    {
        hdrOffset += alignment - delta;
    }

    poolP->alignment   = alignment;
    poolP->buffSize    = buffSize;
    poolP->hdrOffset   = hdrOffset;
    poolP->chunkSize   = buffSize + hdrOffset;
    poolP->buffSizeMin = buffSizeMin;

    chunkSize = poolP->chunkSize;

    totalNum = buffBulkSize / chunkSize;
    poolP->genPoolP = genPoolCreate (totalNum);
    if (poolP->genPoolP == NULL)
    {
        mvOsPrintf ("%s: genPoolCreate failed.\n", __func__);
        mvOsFree (poolP);
        return NULL;
    }

    for (i = 0; i < totalNum; i++)
    {
        buffP = buffBulkP + (i * chunkSize) + hdrOffset;
        if (genPoolPut (poolP->genPoolP, buffP) != MV_OK)
        {
            mvOsPrintf ("%s: genPoolCreate failed.\n", __func__);
            genPoolDestroy (poolP->genPoolP);
            mvOsFree (poolP);
            return NULL;
        }
    }

    if (actualBuffNumP != NULL)
    {
        *actualBuffNumP = totalNum;
    }

    return poolP;
}

/*******************************************************************************
 * genBuffPoolDestroy
 */
MV_VOID genBuffPoolDestroy(GEN_BUFF_POOL *poolP)
{
    genPoolDestroy (poolP->genPoolP);
    mvOsFree (poolP->origBuffBulkP);
    mvOsFree (poolP);
}

/*******************************************************************************
 * genBuffPoolPut
 */
MV_STATUS genBuffPoolPut(GEN_BUFF_POOL *poolP, MV_U8 *buffP)
{
    return genPoolPut (poolP->genPoolP, buffP);
}

/*******************************************************************************
 * genBuffPoolGet
 */
MV_U8 * genBuffPoolGet(GEN_BUFF_POOL *poolP)
{
    return (MV_U8 *)genPoolGet (poolP->genPoolP);
}

/*******************************************************************************
 * genBuffPoolTotalNumGet
 */
MV_U32 genBuffPoolTotalNumGet(GEN_BUFF_POOL *poolP)
{
    return genPoolTotalNumGet (poolP->genPoolP);
}

/*******************************************************************************
 * genBuffPoolBuffSizeGet
 */
MV_U32 genBuffPoolBuffSizeGet(GEN_BUFF_POOL *poolP)
{
    return poolP->buffSize;
}

/*******************************************************************************
 * genBuffPoolPrint
 */
MV_VOID genBuffPoolPrint(GEN_BUFF_POOL *poolP)
{
    mvOsPrintf ("genBuffPoolP         = 0x%08X.\n", (MV_U32)poolP);
    mvOsPrintf ("poolP->genPoolP      = 0x%08X.\n", (MV_U32)poolP->genPoolP);
    mvOsPrintf ("poolP->origBuffBulkP = 0x%08X.\n", (MV_U32)poolP->origBuffBulkP);
    mvOsPrintf ("poolP->buffSize      = %d.\n", (MV_U32)poolP->buffSize);
    mvOsPrintf ("poolP->hdrOffset     = %d.\n", (MV_U32)poolP->hdrOffset);
    mvOsPrintf ("poolP->buffSizeMin   = %d.\n", (MV_U32)poolP->buffSizeMin);
    mvOsPrintf ("poolP->alignment     = %d.\n", (MV_U32)poolP->alignment);

    genPoolPrint (poolP->genPoolP);
}

/*******************************************************************************
 * genBuffPoolCalcBuffNum
 */
MV_U32 genBuffPoolCalcBuffNum(MV_U32 buffBulkSize,
                              MV_U8 *buffBulkP,
                              MV_U32 buffSize,
                              MV_U32 hdrOffset,
                              MV_U32 alignment)
{
    MV_U32         delta, totalNum, chunkSize;

    /* Set the buffers pool to point to a properly alligned block. */
    delta = (MV_U32)buffBulkP % alignment;
    if (delta > 0)
    {
        buffBulkSize -= alignment - delta;
        buffBulkP    += alignment - delta;
    }

    /* The buffer size must be a multiple of alignment. */
    buffSize -= buffSize % alignment;

    /* allign header offset */
    delta = hdrOffset % alignment;
    if (delta != 0)
    {
        hdrOffset += alignment - delta;
    }

    chunkSize = buffSize + hdrOffset;
    totalNum  = buffBulkSize / chunkSize;

    return totalNum;
}

