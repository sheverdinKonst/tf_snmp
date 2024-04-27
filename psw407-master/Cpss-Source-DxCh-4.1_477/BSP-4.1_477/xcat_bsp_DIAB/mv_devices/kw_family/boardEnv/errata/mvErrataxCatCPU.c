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
#include "mvErrataxCatCPU.h"
#include "mvOs.h"
#include "mvSysHwConfig.h"

void *mvErrataCpuHandle;

#define MV_ERRATA_DESCR_XCAT_CPU "xCat-CPU"

MV_ERRATA_ENTRY cpuArray[] = {
    {"Ref # GL-MISL-70 (Speculative Instruction Prefetch)",NULL},
    {"Ref # BTS-KW-121", NULL},
    {"Ref # RES-SPI-20 (Chip Select (SPI_CSn) Deselect Timing)",NULL}};

/* Errata functions. */
void mvErrataApply_i_prefetch(void *pDev, MV_U8*  pBuf, int bufSize);
void mvErrataApply_buf_prefetch(void);
void mvErrataApply_spi_assert_delay(MV_U32 *isCsAssertDelayNeeded);

/*******************************************************************************
 * mvErrataCpuHandleGet
 */
void *mvErrataCpuHandleGet(void)
{
    return mvErrataCpuHandle;
}

/*******************************************************************************
 * mvErrataInitXCatCPU
 */
MV_STATUS mvErrataInitXCatCPU(void)
{
    MV_U32 size;

    cpuArray[MV_ERRATA_ID_SPEC_I_PREFETCH].funcP =
        mvErrataApply_i_prefetch;
    cpuArray[MV_ERRATA_ID_PREFETCH_BUF_ENABLE].funcP =
        mvErrataApply_buf_prefetch;
    cpuArray[MV_ERRATA_SPI_ASSERT_DELAY].funcP =
        mvErrataApply_spi_assert_delay;

    size = sizeof(cpuArray) / sizeof(MV_ERRATA_ENTRY);
    mvErrataCpuHandle = mvErrataInit(cpuArray, size, MV_ERRATA_DESCR_XCAT_CPU);
    if (mvErrataCpuHandle == NULL)
    {
        mvOsPrintf("%s: mvErrataInit failed.\n", __func__);
        return MV_FAIL;
    }

    return MV_OK;
}

/*******************************************************************************
 * mvErrataApply_i_prefetch
 */
void mvErrataApply_i_prefetch(void *pDev, MV_U8*  pBuf, int bufSize)
{
    mvOsCacheUnmap(pDev, pBuf, bufSize);
}

/*******************************************************************************
 * mvErrataApply_buf_prefetch
 */
void mvErrataApply_buf_prefetch(void)
{
    MV_REG_BIT_SET (CPU_CONFIG_REG, CCR_DCACH_PREF_BUF_ENABLE);
    MV_REG_BIT_SET (CPU_CONFIG_REG, CCR_ICACH_PREF_BUF_ENABLE);
}

/*******************************************************************************
 * mvErrataApply_spi_assert_delay
 */
void mvErrataApply_spi_assert_delay(MV_U32 *isCsAssertDelayNeeded)
{
    mvOsUDelay(1);
}


