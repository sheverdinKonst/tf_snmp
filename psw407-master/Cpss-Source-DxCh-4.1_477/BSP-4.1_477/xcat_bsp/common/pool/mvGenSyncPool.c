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
#include "mvGenSyncPool.h"
#include "mvOs.h"

#if 0
    #define DB(x) x
#else
    #define DB(x)
#endif

/*
 * struct _genSyncPool (GEN_SYNC_POOL)
 *
 * Description:
 *      Generic pool may be used to store any objects.
 *      Every 'get' and 'put' pool operations are internally synchronized
 *      by pool-specific-semaphore.
 *
 */
struct _genSyncPool
{
    GEN_POOL *genPoolP;
    MV_U32    semId;     /* binary semaphore */
};

/*******************************************************************************
 * genSyncPoolCreate
 */
GEN_SYNC_POOL *genSyncPoolCreate(MV_U32 numOfPoolChunks)
{
    GEN_SYNC_POOL *poolP = NULL;

    poolP = (GEN_SYNC_POOL *)mvOsCalloc (1, sizeof(GEN_SYNC_POOL));
    if (poolP == NULL)
    {
        mvOsPrintf ("%s: Alloc failed.\n", __func__);
        return NULL;
    }

    poolP->genPoolP = (GEN_POOL *)genPoolCreate (numOfPoolChunks);
    if (poolP->genPoolP == NULL)
    {
        mvOsPrintf ("%s: genPoolCreate failed.\n", __func__);
        mvOsFree (poolP);
        return NULL;
    }

    /*
     * create binary semaphore
     */
    if (mvOsSemCreate ("genSyncPool",
                       1, /* init sem value is free  */
                       1, /* 1 ==> create binary sem */
                       &poolP->semId) != MV_OK)
    {
        mvOsPrintf ("%s: Alloc failed.\n", __func__);
        genPoolDestroy (poolP->genPoolP);
        mvOsFree (poolP);
        return NULL;
    }

    return poolP;
}

/*******************************************************************************
 * genSyncPoolDestroy
 */
MV_VOID genSyncPoolDestroy(GEN_SYNC_POOL *poolP)
{
    if (mvOsSemDelete (poolP->semId) != MV_OK)
    {
        mvOsPrintf ("%s: mvOsSemDelete failed.\n", __func__);
    }

    genPoolDestroy (poolP->genPoolP);
    mvOsFree (poolP);
}

/*******************************************************************************
 * genSyncPoolPut
 */
MV_STATUS genSyncPoolPut(GEN_SYNC_POOL *poolP, MV_VOID *newChunkP)
{
    if (mvOsSemWait (poolP->semId, MV_OS_WAIT_FOREVER) != MV_OK)
    {
        mvOsPrintf ("%s: mvOsSemWait failed.\n", __func__);
        return MV_FAIL;
    }

    if (genPoolPut (poolP->genPoolP, newChunkP) != MV_OK)
    {
        mvOsSemSignal (poolP->semId);
        return MV_FAIL;
    }

    if (mvOsSemSignal (poolP->semId) != MV_OK)
    {
        mvOsPrintf ("%s: mvOsSemSignal failed.\n", __func__);
        return MV_FAIL;
    }

    return MV_OK;
}

/*******************************************************************************
 * genSyncPoolGet
 */
MV_VOID * genSyncPoolGet(GEN_SYNC_POOL *poolP)
{
    MV_VOID *chunkToReturnP;

    if (mvOsSemWait (poolP->semId, MV_OS_WAIT_FOREVER) != MV_OK)
    {
        mvOsPrintf ("%s: mvOsSemWait failed.\n", __func__);
        mvOsSemSignal (poolP->semId);
        return NULL;
    }

    chunkToReturnP = genPoolGet (poolP->genPoolP);
    if (chunkToReturnP == NULL)
    {
        DB (mvOsPrintf ("%s: genPoolGet returned nothing.\n", __func__));
    }

    if (mvOsSemSignal (poolP->semId) != MV_OK)
    {
        mvOsPrintf ("%s: mvOsSemSignal failed.\n", __func__);
        return NULL;
    }

    return chunkToReturnP;
}

/*******************************************************************************
 * genSyncPoolTotalNumGet
 */
MV_U32 genSyncPoolTotalNumGet(GEN_SYNC_POOL *poolP)
{
    return genPoolTotalNumGet (poolP->genPoolP);
}

