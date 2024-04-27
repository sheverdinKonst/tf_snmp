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
#include "mvOs.h"
#include "mvSysHwConfig.h"

typedef struct _mvErrataTbl
{
    MV_ERRATA_ENTRY *errataP;
    MV_8            *descr;
    MV_U32           len;
} MV_ERRATA_TBL;

typedef struct _mvErrataCtrl
{
    MV_U32           freeTblIndex;
    MV_ERRATA_TBL    errataTbl[MV_ERRATA_NUM_OF_TBL];
} MV_ERRATA_CTRL;

static MV_ERRATA_CTRL  mvErrataCtrl;
static MV_ERRATA_CTRL *mvErrataCtrlP = &mvErrataCtrl;
static MV_BOOL         mvIsCtrlInited = MV_FALSE;

/*******************************************************************************
 * mvIsErrataCtrlInited
 */
static MV_BOOL mvIsErrataCtrlInited(void)
{
    return mvIsCtrlInited;
}

/*******************************************************************************
 * mvErrataInitCtrl
 */
static MV_BOOL mvErrataInitCtrl(void)
{
    MV_ERRATA_CTRL *ctrlP = mvErrataCtrlP;
    mvOsMemset(ctrlP, 0, sizeof(MV_ERRATA_CTRL));
    return MV_OK;
}

/*******************************************************************************
 * mvErrataInit
 */
void *mvErrataInit(MV_ERRATA_ENTRY *errataEntryP, MV_U32 arrayLen, MV_8 *descr)
{
    MV_ERRATA_CTRL  *ctrlP = mvErrataCtrlP;
    MV_ERRATA_TBL   *freeFuncTblP;

    /* Init errata controler struct only once. */
    if (mvIsErrataCtrlInited() == MV_FALSE)
    {
        mvIsCtrlInited = MV_TRUE;
        if (mvErrataInitCtrl() != MV_OK)
        {
            mvOsPrintf("mvErrataInit: ERROR: mvErrataInitCtrl failed.\n");
            return 0;
        }
    }

    if (errataEntryP == NULL || arrayLen == 0)
    {
        mvOsPrintf("mvErrataInit: ERROR: Bad param.\n");
        return 0;
    }

    /* Store errata functions in global structure. */
    freeFuncTblP = &ctrlP->errataTbl[ctrlP->freeTblIndex];
    ctrlP->freeTblIndex++;
    freeFuncTblP->errataP = errataEntryP;
    freeFuncTblP->len = arrayLen;
    freeFuncTblP->descr = descr;

    return freeFuncTblP;
}

/*******************************************************************************
 * mvGetErrataFunc
 */
void *mvGetErrataFunc(void *errataHandle, MV_U32 errataId)
{
    MV_ERRATA_TBL   *errataTblP = (MV_ERRATA_TBL *)errataHandle;
    MV_ERRATA_ENTRY *errataEntryP;

    if (errataTblP == NULL || errataId >= errataTblP->len)
    {
        return NULL;
    }

    errataEntryP = &errataTblP->errataP[errataId];
    return errataEntryP->funcP;
}

/*******************************************************************************
 * mvErratPrint
 */
void mvErrataPrint(void)
{
    MV_ERRATA_CTRL  *ctrlP = mvErrataCtrlP;
    MV_ERRATA_TBL   *errataTblP;
    MV_ERRATA_ENTRY *errataEntryP;
    int tblIndex, errataIndex = 0;

    for (tblIndex = 0; tblIndex < ctrlP->freeTblIndex; tblIndex++)
    {
        errataTblP = &ctrlP->errataTbl[tblIndex];
        for (errataIndex = 0; errataIndex < errataTblP->len; errataIndex++)
        {
            errataEntryP = &errataTblP->errataP[errataIndex];
            mvOsPrintf("Errata %s: %s (%s).\n", errataTblP->descr,
                errataEntryP->descr,
                (errataEntryP->funcP == NULL) ? "inactive" : "active");
        }
    }
}

#ifdef _DIAB_TOOL
typedef MV_VOID (*MV_ERRATA_GENERAL_PURPOSE_VOID_FUNC)(void *arg, ...);
int find_symbol_by_address(unsigned long address, char *name);

void  MV_ERRATA_APPLY(void *errataHandle, int errataId, ... )
{
  void *errataFuncP; 
  va_list	args;
  char name[128];

  errataFuncP = mvGetErrataFunc(errataHandle, errataId); 
  if (errataFuncP != NULL) 
  { 
#if 0
    find_symbol_by_address((unsigned long)errataFuncP, name);
    mvOsPrintf("Applying errata. errataId=%d, errataFuncP=0x%08x(%s)\n",
               errataId, errataFuncP, name);
#endif
    va_start(args, errataId);
    ((MV_ERRATA_GENERAL_PURPOSE_VOID_FUNC)(errataFuncP))(args);
    va_end(args);
  }  
}
#endif


