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

#include "mvWriteLog.h"
#include "mvOs.h"
#include "mvSysHwConfig.h"

typedef struct _mvWriteLogEntry
{
    MV_U32 addr;
    MV_U32 val;
    MV_U32 globalIndex;
} MV_WRITE_LOG_ENTRY;

typedef struct _mvWriteLogCtrl
{
    MV_WRITE_LOG_ENTRY   *logEntriesP;
    MV_U32                logIndex;
    MV_U32                logNumOfEntries;
    MV_U32                logWrapAroundCnt;
    MV_BOOL               isLogActive;
} MV_WRITE_LOG_CTRL;

static MV_U32 mvWriteLogGlobalIndex = 0;

static MV_WRITE_LOG_CTRL  G_mvWriteLogCtrl[MV_WRITE_LOG_NUM_OF_INSTANCE];
static MV_WRITE_LOG_ENTRY G_mvWriteLogEnriesArr[MV_WRITE_LOG_LEN_TOTAL];

/*******************************************************************************
 * mvWriteLogIsActive
 */
MV_BOOL mvWriteLogIsActive(void *logHandleP)
{
    MV_WRITE_LOG_CTRL *logCtrlP = (MV_WRITE_LOG_CTRL *)logHandleP;

    if (logCtrlP == NULL)
    {
        mvOsPrintf("%s: logCtrlP is NULL.\n", __func__);
        return MV_FALSE;
    }

    return logCtrlP->isLogActive;
}

/*******************************************************************************
 * mvWriteLogIsPaused
 */
MV_BOOL mvWriteLogIsPaused(void *logHandleP)
{
    MV_WRITE_LOG_CTRL *logCtrlP = (MV_WRITE_LOG_CTRL *)logHandleP;

    if (logCtrlP == NULL)
    {
        mvOsPrintf("%s: logCtrlP is NULL.\n", __func__);
        return MV_TRUE;
    }

    return !logCtrlP->isLogActive;
}

/*******************************************************************************
 * mvWriteLogActivate
 */
MV_STATUS mvWriteLogActivate(void *logHandleP)
{
    MV_WRITE_LOG_CTRL *logCtrlP = (MV_WRITE_LOG_CTRL *)logHandleP;

    if (logCtrlP == NULL)
    {
        mvOsPrintf("%s: logCtrlP is NULL.\n", __func__);
        return MV_FAIL;
    }

    if (logCtrlP->isLogActive == MV_TRUE)
    {
        return MV_OK;
    }

    logCtrlP->isLogActive = MV_TRUE;
    return MV_OK;
}

/*******************************************************************************
 * mvWriteLogPause
 */
MV_STATUS mvWriteLogPause(void *logHandleP)
{
    MV_WRITE_LOG_CTRL *logCtrlP = (MV_WRITE_LOG_CTRL *)logHandleP;

    if (logCtrlP == NULL)
    {
        mvOsPrintf("%s: logCtrlP is NULL.\n", __func__);
        return MV_FAIL;
    }

    if (logCtrlP->isLogActive == MV_FALSE)
    {
        return MV_OK;
    }

    logCtrlP->isLogActive = MV_FALSE;
    return MV_OK;
}

/*******************************************************************************
 * mvWriteLogCreate
 */
void *mvWriteLogCreate(MV_U32 numOfEntries)
{
    MV_WRITE_LOG_CTRL    *logCtrlP = NULL;
    MV_WRITE_LOG_ENTRY   *logEntriesP = NULL;

    if (numOfEntries == 0)
    {
        mvOsPrintf("%s: Wrong numOfEntries (= 0).\n", __func__);
        return NULL;
    }

    logCtrlP = mvOsCalloc(1, sizeof(MV_WRITE_LOG_CTRL));
    if (logCtrlP == NULL)
    {
        mvOsPrintf("%s: Mememory alloc failed.\n", __func__);
        return NULL;
    }

    logEntriesP = mvOsCalloc(numOfEntries, sizeof(MV_WRITE_LOG_ENTRY));
    if (logEntriesP == NULL)
    {
        mvOsFree(logCtrlP);
        mvOsPrintf("%s: Mememory alloc failed.\n", __func__);
        return NULL;
    }
    logCtrlP->logEntriesP = logEntriesP;

    logCtrlP->logNumOfEntries = numOfEntries;
    logCtrlP->logWrapAroundCnt = 0;
    logCtrlP->isLogActive = MV_FALSE;

    return logCtrlP;
}

/*******************************************************************************
 * mvWriteLogCreateStatic
 */
void *mvWriteLogCreateStatic(void)
{
    static MV_U32 logIndex = 0;
    MV_WRITE_LOG_CTRL    *logCtrlP = NULL;
    MV_WRITE_LOG_ENTRY   *logEntriesP = NULL;

    if (logIndex >= MV_WRITE_LOG_NUM_OF_INSTANCE)
    {
        mvOsPrintf("%s: Static alloc failed.\n", __func__);
        return NULL;
    }

    logCtrlP = &G_mvWriteLogCtrl[logIndex];
    logEntriesP = &G_mvWriteLogEnriesArr[logIndex * MV_WRITE_LOG_INSTANCE_LEN];

    mvOsMemset(logCtrlP, 0, sizeof(MV_WRITE_LOG_CTRL));
    mvOsMemset(logEntriesP, 0,
               MV_WRITE_LOG_INSTANCE_LEN * sizeof(MV_WRITE_LOG_ENTRY));

    logCtrlP->logEntriesP = logEntriesP;
    logCtrlP->logNumOfEntries = MV_WRITE_LOG_INSTANCE_LEN;
    logCtrlP->logWrapAroundCnt = 0;
    logCtrlP->isLogActive = MV_FALSE;
    logCtrlP->logIndex = 0;

    logIndex++;

    return logCtrlP;
}

/*******************************************************************************
 * mvWriteLogDestroy
 */
MV_STATUS mvWriteLogDestroy(void *logHandleP)
{
    MV_WRITE_LOG_CTRL *logCtrlP = (MV_WRITE_LOG_CTRL *)logHandleP;

    if (logCtrlP == NULL)
    {
        mvOsPrintf("%s: logCtrlP is NULL.\n", __func__);
        return MV_FAIL;
    }

    mvOsFree(logCtrlP->logEntriesP);
    mvOsFree(logCtrlP);

    return MV_OK;
}

/*******************************************************************************
 * mvWriteLogAddEntry
 */
MV_STATUS mvWriteLogAddEntry(void *logHandleP, MV_U32 addr, MV_U32 val)
{
    MV_WRITE_LOG_CTRL    *logCtrlP = (MV_WRITE_LOG_CTRL *)logHandleP;
    MV_WRITE_LOG_ENTRY   *logEntryP;

    if (logCtrlP == NULL)
    {
        mvOsPrintf("%s: logCtrlP is NULL.\n", __func__);
        return MV_FAIL;
    }

    if (logCtrlP->isLogActive == MV_FALSE)
    {
        /* Silently ignore the log request when in the inactive state. */
        return MV_OK;
    }

    logEntryP = &logCtrlP->logEntriesP[logCtrlP->logIndex];
    logEntryP->addr = addr;
    logEntryP->val = val;
    logEntryP->globalIndex = mvWriteLogGlobalIndex++;

    logCtrlP->logIndex++;
    if (logCtrlP->logIndex == logCtrlP->logNumOfEntries)
    {
        logCtrlP->logWrapAroundCnt++;
        logCtrlP->logIndex = 0;
    }

    return MV_OK;
}

/*******************************************************************************
 * mvWriteLogRestart
 */
MV_STATUS mvWriteLogRestart(void *logHandleP)
{
    MV_WRITE_LOG_CTRL    *logCtrlP = (MV_WRITE_LOG_CTRL *)logHandleP;

    if (logCtrlP == NULL)
    {
        mvOsPrintf("%s: logCtrlP is NULL.\n", __func__);
        return MV_FAIL;
    }

    logCtrlP->isLogActive = MV_TRUE;
    logCtrlP->logIndex = 0;
    logCtrlP->logWrapAroundCnt = 0;

    return MV_OK;
}

/*******************************************************************************
 * mvWriteLogPrint
 */
void mvWriteLogPrint(void *logHandleP)
{
    MV_WRITE_LOG_CTRL *logCtrlP = (MV_WRITE_LOG_CTRL *)logHandleP;
    MV_U32 i, numOfEntriesToPrint, startIndex, endIndex, loopCnt;
    MV_WRITE_LOG_ENTRY   *logEntryP;

    if (logCtrlP == NULL)
    {
        mvOsPrintf("%s: logCtrlP is NULL.\n", __func__);
        return;
    }

    mvOsPrintf("Log size = %d entries.\n", logCtrlP->logNumOfEntries);
    mvOsPrintf("Current log index = %d.\n", logCtrlP->logIndex);
    mvOsPrintf("# of log wrap arounds = %d.\n", logCtrlP->logWrapAroundCnt);

    if (logCtrlP->logWrapAroundCnt > 0)
    {
        numOfEntriesToPrint = logCtrlP->logNumOfEntries;
        startIndex = logCtrlP->logIndex;
        if (logCtrlP->logIndex == 0)
        {
            endIndex = logCtrlP->logNumOfEntries; /* not including */
        }
        else
        {
            endIndex = logCtrlP->logIndex - 1; /* not including */
        }
    }
    else
    {
        numOfEntriesToPrint = logCtrlP->logIndex;
        startIndex = 0;
        endIndex = logCtrlP->logIndex; /* not including */
    }

    loopCnt = 0;

    for (i = startIndex; i != endIndex; i++)
    {
        if (i == logCtrlP->logNumOfEntries)
        {
            i = 0;
            continue;
        }

        logEntryP = &logCtrlP->logEntriesP[i];
        mvOsPrintf("%04d: globIdx = %04d: addr = 0x%08X, value = 0x%08X.\n",
                   i,
                   logEntryP->globalIndex,
                   logEntryP->addr,
                   logEntryP->val);

        if (loopCnt >= logCtrlP->logNumOfEntries)
        {
            mvOsPrintf("%s: Tried to print too much.\n", __func__);
        }
        loopCnt++;
    }
}

