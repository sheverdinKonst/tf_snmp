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
#include "mvGenUtUart.h"
#include "mvUartLog.h"
#include "mvBoardEnvLib.h"
#include "mvCtrlEnvSpec.h"

#define MV_GEN_UT_UART_LOG_LEN (100)
static MV_U8  s_logArr[MV_GEN_UT_UART_LOG_LEN];
static MV_U32 s_logArrI;
static MV_U32 s_logCharLoggedNum;
static MV_U8  s_logTestStr[MV_GEN_UT_UART_LOG_LEN] = "1234567890-qwerty";

/*******************************************************************************
 * mvGenUtUartRunAll
 */
void mvGenUtUartRunAll(void)
{
    if (mvGenUtUartPrintEnable() != MV_OK)
    {
        mvOsPrintf("mvGenUtUartPrintEnable FAILED.\n\n");
    }
    else
    {
        mvOsPrintf("mvGenUtUartPrintEnable succeeded.\n\n");
    }

    if (mvGenUtUartLog() != MV_OK)
    {
        mvOsPrintf("mvGenUtUartLog FAILED.\n\n");
    }
    else
    {
        mvOsPrintf("mvGenUtUartLog succeeded.\n\n");
    }
}

/*******************************************************************************
 * mvUartLogger
 */
static void mvUartLogger(MV_U32 port, MV_U8 c)
{
    if (port >= MV_UART_MAX_CHAN)
    {
        /* Do not print anything here, otherwise, it is infinite loop. */
        return;
    }

    s_logArr[s_logArrI++] = c;
    s_logCharLoggedNum++;

    if (s_logArrI >= MV_GEN_UT_UART_LOG_LEN)
    {
        s_logArrI = 0;
    }
}

/*******************************************************************************
 * mvGenUtUartPrintEnable
 */
MV_STATUS mvGenUtUartPrintEnable(void)
{
    MV_U32 tclk, clockDivisor, logStrLen, i;

    logStrLen = strlen(s_logTestStr);

    tclk = mvBoardTclkGet();
    clockDivisor = (tclk / 16) / DUART_BAUD_RATE;

    mvUartInit(0, clockDivisor, mvUartBase(0));

    if (mvUartPrintEnable(0) != MV_OK)
    {
        mvOsPrintf("%s: mvUartPrintEnable failed.\n", __func__);
        return MV_FAIL;
    }
    mvOsPrintf("%s: print is enabled.\n", __func__);

    /*
     * Print some string to the screen or terminal - it should be seen.
     */
    for (i = 0; i < logStrLen; i++)
    {
        mvUartPutc(0, s_logTestStr[i]);
    }
    mvOsPrintf("\n");

    if (mvUartPrintDisable(0) != MV_OK)
    {
        mvOsPrintf("%s: mvUartPrintDisable failed.\n", __func__);
        return MV_FAIL;
    }
    mvOsPrintf("%s: print is disabled.\n", __func__);

    /*
     * Print some string to the screen or terminal - it should NOT be seen.
     */
    for (i = 0; i < logStrLen; i++)
    {
        mvUartPutc(0, s_logTestStr[i]);
    }

    if (mvUartPrintEnable(0) != MV_OK)
    {
        mvOsPrintf("%s: mvUartPrintEnable failed.\n", __func__);
        return MV_FAIL;
    }
    mvOsPrintf("%s: print is enabled.\n", __func__);

    return MV_OK;
}

/*******************************************************************************
 * mvGenUtUartLog
 */
MV_STATUS mvGenUtUartLog(void)
{
    MV_U32 tclk, clockDivisor, logStrLen, i;

    logStrLen = strlen(s_logTestStr);

    tclk = mvBoardTclkGet();
    clockDivisor = (tclk / 16) / DUART_BAUD_RATE;

    /* mvUartInit(0, clockDivisor, mvUartBase(0)); */

    mvOsMemset(s_logArr, 0, sizeof(MV_GEN_UT_UART_LOG_LEN));
    s_logArrI = 0;
    s_logCharLoggedNum = 0;

    if (mvUartLogInit(0) != MV_OK)
    {
        mvOsPrintf("%s: mvUartLogInit failed.\n", __func__);
        return MV_FAIL;
    }

    if (mvUartLogReg(0, mvUartLogger) != MV_OK)
    {
        mvOsPrintf("%s: mvUartLogReg failed.\n", __func__);
        return MV_FAIL;
    }

    /*
     * Print some string that will be logged.
     */
    for (i = 0; i < logStrLen; i++)
    {
        mvUartPutc(0, s_logTestStr[i]);
    }

    if (mvUartLogUnreg(0) != MV_OK)
    {
        mvOsPrintf("%s: mvUartLogUnreg failed.\n", __func__);
        return MV_FAIL;
    }

    mvOsPrintf("\n%s: Hook was unregistered.\n", __func__);
    mvOsPrintf("Num of chars logged = %d.\n", s_logCharLoggedNum);
    if (s_logCharLoggedNum != logStrLen)
    {
        mvOsPrintf("%s: TEST FAILED.\n", __func__);
        return MV_FAIL;
    }
    mvOsPrintf("log contains: [%s]\n", s_logArr);

    return MV_OK;
}

