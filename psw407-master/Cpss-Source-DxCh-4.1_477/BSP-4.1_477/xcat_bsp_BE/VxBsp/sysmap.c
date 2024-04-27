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

#include "mvSysHwConfig.h"
#include "sysmap.h"

MV_CPU_DEC_WIN SYSMAP_88F6281[] = {
    /* base low        base high    size                WinNum      enable */
    {{SDRAM_CS0_BASE,     0,      SDRAM_CS0_SIZE     }, 0xFFFFFFFF, DIS},
    {{SDRAM_CS1_BASE,     0,      SDRAM_CS1_SIZE     }, 0xFFFFFFFF, DIS},
    {{SDRAM_CS2_BASE,     0,      SDRAM_CS2_SIZE     }, 0xFFFFFFFF, DIS},
    {{SDRAM_CS3_BASE,     0,      SDRAM_CS3_SIZE     }, 0xFFFFFFFF, DIS},
    {{PEX0_MEM_BASE,      0,      PEX0_MEM_SIZE      }, 0x0,        EN},
    {{PEX0_IO_BASE,       0,      PEX0_IO_SIZE       }, 0x2,        EN},
    {{INTER_REGS_BASE,    0,      INTER_REGS_SIZE    }, 0x8,        EN},
    {{NFLASH_CS_BASE,     0,      NFLASH_CS_SIZE     }, 0x3,        EN},
    {{DEVICE_SPI_BASE,    0,      DEVICE_SPI_SIZE    }, 0x5,        EN},
    {{DEVICE_CS2_BASE,    0,      DEVICE_CS2_SIZE    }, 0x4,        DIS},
    {{BOOTDEV_CS_BASE,    0,      BOOTDEV_CS_SIZE    }, 0x5,        DIS},
    {{CRYPT_ENG_BASE,     0,      CRYPT_ENG_SIZE     }, 0x7,        EN},
#ifdef MV_INCLUDE_SAGE
    {{SAGE_UNIT_BASE,     0,      SAGE_UNIT_SIZE     }, 0x6,        EN},
#endif
#ifdef MV_INCLUDE_DRAGONITE
    {{DRAGONITE_DTCM_BASE,0,      DRAGONITE_DTCM_SIZE}, 0x1,        EN},
#endif
    {{TBL_TERM,           0,      TBL_TERM           }, TBL_TERM,   TBL_TERM}
};

MV_CPU_DEC_WIN *mvCpuAddrWinMap = SYSMAP_88F6281;
MV_CPU_DEC_WIN *mv_sys_map(void)
{
    switch (mvBoardIdGet())
    {
    case DB_88F6180A_BP_ID:
    case DB_88F6281A_BP_ID:
    case RD_98DX3121_ID:
    case DB_98DX4122_ID:
    case DB_98DX2122_ID:
        return mvCpuAddrWinMap;
    default:
        mvOsPrintf("ERROR: can't find system address map\n");
        return NULL;
    }
}

