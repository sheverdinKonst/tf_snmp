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
#if INCLUDE_VX_SWITCH_ACCESS
#include "mvPrestera.h"
#include "mvCpuIf.h"

/*******************************************************************************
* switchWriteReg
*
* DESCRIPTION:
*       Writes the unmasked bits to the switch internal register.
*
* INPUTS:
*       regAddr  - Register address to write to.
*       value    - Data to be written to register.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK     - on success
*       MV_FAIL   - on error
*
* COMMENTS:
*
*******************************************************************************/
MV_STATUS switchWriteReg(MV_U8 devNum, MV_U32 regAddr, MV_U32 value)
{
    MV_CPU_DEC_WIN addrWin;

    /* get the switch window */
    if (mvCpuIfTargetWinGet(PP_TARGET_ID, &addrWin) != MV_OK) {
        mvOsPrintf("\nFail get base address ");
        return MV_FAIL;
    }

    /* write the register value */
    return mvSwitchWriteReg(devNum,regAddr,value);
}

/*******************************************************************************
* switchReadReg
*
* DESCRIPTION:
*       Reads the unmasked bits of the switch internal register.
*
* INPUTS:
*       regAddr   - Register address to read.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK      - on success
*       MV_FAIL      - on error
*
* COMMENTS:
*
*******************************************************************************/
MV_STATUS switchReadReg(MV_U8 devNum, MV_U32 regAddr)
{
    MV_U32 value;

    /* read the register value */
    if (mvSwitchReadReg(devNum,regAddr,&value) != MV_OK)
    {
        mvOsPrintf("\nFail read register");
        return MV_FAIL;
    }

    mvOsPrintf("\n0x%x: 0x%x\n",regAddr,value);

    return MV_OK;
}

/*******************************************************************************
* switchReadPortMibCounters
*
* DESCRIPTION:
*       Print MIB counters of the switch port
*
* INPUTS:
*       port - port number
*
* OUTPUTS:
*       none
*
* RETURNS:
*       none
*
* COMMENTS:
*
*******************************************************************************/
void    switchReadPortMibCounters(int port)
{
    mvPresteraReadPortMibCounters(port);
}
#else
volatile static void empty_func(void)
{
}
#endif


