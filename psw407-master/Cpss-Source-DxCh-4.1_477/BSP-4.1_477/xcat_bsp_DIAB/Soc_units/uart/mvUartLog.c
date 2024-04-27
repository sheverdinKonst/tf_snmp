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

#include "mvUart.h"
#include "mvUartLog.h"
#include "mvCtrlEnvSpec.h"

typedef struct _mvUartLogCtrl
{
    MV_UART_LOG_HOOK prevHookP;
    MV_UART_LOG_HOOK currHookP;
} MV_UART_LOG_CTRL;

/* static variables */
static MV_UART_LOG_CTRL  s_UartLogCtrl[MV_UART_MAX_CHAN];
static MV_BOOL           s_mvUartLogIsInited = MV_FALSE;
/*******************************************************************************
 * mvUartLogIsInited
 */
MV_BOOL mvUartLogIsInited(void)
{
    return s_mvUartLogIsInited;
}

/*******************************************************************************
 * mvUartLogInit - Inits uart log facility.
 */
MV_STATUS mvUartLogInit(MV_U32 port)
{
    MV_UART_LOG_CTRL *ctrlP;

    if (mvUartLogIsInited() == MV_TRUE)
    {
        return MV_OK;
    }

    if (port >= MV_UART_MAX_CHAN)
    {
        mvOsPrintf("%s: wrong port num (%d).\n", __func__, port);
        return MV_FAIL;
    }

    ctrlP = &s_UartLogCtrl[port];

    /*
     * Save original hook.
     */
    ctrlP->prevHookP = NULL;
    ctrlP->currHookP = mvUartPutc;

    s_mvUartLogIsInited = MV_TRUE;
    return MV_OK;
}

/*******************************************************************************
 * mvUartLogReg - Registers and activates delivered hook.
 */
MV_STATUS mvUartLogReg(MV_U32 port, MV_UART_LOG_HOOK hookP)
{
    MV_UART_LOG_CTRL *ctrlP;

    if (port >= MV_UART_MAX_CHAN)
    {
        mvOsPrintf("%s: wrong port num (%d).\n", __func__, port);
        return MV_FAIL;
    }

    ctrlP = &s_UartLogCtrl[port];

    /*
     * Save original hook and register the new one.
     */
    ctrlP->prevHookP = ctrlP->currHookP;
    ctrlP->currHookP = hookP;

    if (mvUartSetHook(port, ctrlP->currHookP) != MV_OK)
    {
        mvOsPrintf("%s: mvUartSetHook failed.\n", __func__);
        return MV_FAIL;
    }

    return MV_OK;
}

/*******************************************************************************
 * mvUartLogUnreg - Removes the hook and restores the previous one.
 */
MV_STATUS mvUartLogUnreg(MV_U32 port)
{
    MV_UART_LOG_CTRL *ctrlP;

    if (port >= MV_UART_MAX_CHAN)
    {
        mvOsPrintf("%s: wrong port num (%d).\n", __func__, port);
        return MV_FAIL;
    }

    ctrlP = &s_UartLogCtrl[port];

    /*
     * Restore original hook.
     */
    ctrlP->currHookP = ctrlP->prevHookP;

    if (mvUartSetHook(port, NULL) != MV_OK)
    {
        mvOsPrintf("%s: mvUartSetHook failed.\n", __func__);
        return MV_FAIL;
    }

    return MV_OK;
}

