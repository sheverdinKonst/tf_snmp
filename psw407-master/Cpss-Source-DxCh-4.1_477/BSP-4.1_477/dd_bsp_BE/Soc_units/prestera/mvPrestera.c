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

#include "mvPrestera.h"
#include "mvSysHwConfig.h"
#include "mvTypes.h"
#include "mvOs.h"
#include "mvPex.h"
#include "mvCpuIf.h"
#include "mvCpuIfRegs.h"
#include "ctrlEnv/sys/mvSysPex.h"


#undef MV_DEBUG         
#ifdef MV_DEBUG
#define DB(x) x
#else
#define DB(x)
#endif

/*******************************************************************************
* mvPpHalInitHw
*
* DESCRIPTION:
*       This function initialize the Prestera Switch (PP) - its hardware part.
*       1) Enables propagation of interrupt from PP-DEV1 to PP-DEV0 through PEX.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       None.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_STATUS mvPpHalInitHw(void)
{
    MV_U32 intBitIdx, intBitMask;
    MV_U32 remoteBar1;
    MV_U32 temp, pexWin, base;

    /* Fix the address if remap enabled. */
    /* 1.Find the PEX Window by scan all 8 windows */
    for (pexWin = 0; pexWin < MV_AHB_TO_MBUS_INTREG_WIN; pexWin++)
    {
        temp = MV_REG_READ(AHB_TO_MBUS_WIN_CTRL_REG(0,pexWin));
        /*Check if the Window have PEX target attribute*/
        if (( (temp & ATMWCR_WIN_TARGET_MASK) == 0x80 ) &&
            ( (temp & ATMWCR_WIN_ENABLE) == 1) )
        {
            mvOsPrintf("Found PEX remapping win #%x\n", pexWin);
            break;
        }
    }

    if (pexWin == MV_AHB_TO_MBUS_INTREG_WIN)
    {
        mvOsPrintf("Prestera PEX: not found PEX window!!\n");
        return MV_FAIL;
    }


    /*2. Fix the address by remapping base */
    base = MV_REG_READ(AHB_TO_MBUS_WIN_BASE_REG(0,pexWin));



    if (mvPpChipIsXCat2(&remoteBar1) == MV_TRUE)
    {

        /*config BAR1 window in order to access pp unit*/
        MV_CPU_DEC_WIN addrDecWin;
        MV_U32 regBase;

        if (mvCpuIfTargetWinGet(PCI4_MEM0, &addrDecWin) != MV_OK)
        {
            mvOsPrintf("%s: mvCpuIfTargetWinGet failed.\n", __func__);
            return MV_FAIL;
        }

        if (addrDecWin.enable == MV_FALSE)
        {
            mvOsPrintf("%s: PEX win is disabled.\n", __func__);
            return MV_FAIL;
        }

        regBase = addrDecWin.addrWin.baseLow;
        MV_MEMIO_LE32_WRITE(regBase + 0x41820, 0x03ff00c1);
        MV_MEMIO_LE32_WRITE(regBase + 0x41824, remoteBar1);

		/* enable switch access to dram - write to switch registers */
        MV_MEMIO_LE32_WRITE(regBase + 0x4000000 + 0x30c, 0xe804);
        MV_MEMIO_LE32_WRITE(regBase + 0x4000000 + 0x310, 0xffff0000);
        MV_MEMIO_LE32_WRITE(regBase + 0x4000000 + 0x34c, 0x3e);


        /*config interrupts to be routed to the PEX*/
        intBitIdx = INT_LVL_XCAT2_SWITCH % 32;
        intBitMask = 1 << intBitIdx;
        MV_MEMIO_LE32_WRITE(base + CPU_ENPOINT_MASK_HIGH_REG(0), intBitMask);
    }

    return MV_OK;
}


/*******************************************************************************
* mvPpChipIsXCat2
*
* DESCRIPTION:
*       Checks if the chip type is xCat2.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_TRUE     - if the chip is xCat2.
*       MV_FALSE    - if the chip is xCat (e.g. xCat-A1/A2).
*
* COMMENTS:
*       For xCat2:
*       Rev_id = 1 hard coded
*       Dev_id:
*           dev_id[15:10]    = 6'b111001 (6h39)
*           dev_id[9 :4]     = device_mode[5:0] (sample at reset)
*           dev_id[3]        = device_mode[6] (sample at reset)
*           dev_id[2]        = device_mode[7] (sample at reset)
*           dev_id[1]        = bga_pkg  - package
*           dev_id[0]        = ssmii_mode - package
*       As you can see, dev_id[15:10] is not board dependent
*       and distinguishes xcat from xcat2 (xcat has 0x37 on these bits).
*       dev_id[1:0] are also not board dependent and tells you
*       which package and FE/GE device is on board.
*       Of course, for CPSS you must follow the device matrix excel
*       which maps all the dev_id bits.
*
*       Note: this function may be called on the very early boot stage, ==>
*             printf() should not be used.
*
*******************************************************************************/
MV_BOOL mvPpChipIsXCat2(MV_U32 *remoteBar1)
{
	
    MV_U32 chipType, vendor,data;

	MV_U32 pexData,bus, device,func,reg;
	
	/* read the PEX 4 parameters */
	pexData = MV_REG_READ(PEX_CFG_ADDR_REG(4));

	bus =  (pexData >> PXCAR_BUS_NUM_OFFS) & PXCAR_BUS_NUM_MAX;
	device =  (pexData >> PXCAR_DEVICE_NUM_OFFS) & PXCAR_DEVICE_NUM_MAX;
	func =  (pexData >> PXCAR_FUNC_NUM_OFFS) & PXCAR_FUNC_NUM_MAX;
	reg =  (pexData >> PXCAR_REG_NUM_OFFS) & PXCAR_REG_NUM_MAX;

	/* read the device ID from the switch pex register */
    data = mvPexConfigRead(4,bus,device + 1,func,0x0);


    *remoteBar1 = 0;

	chipType = ((data >> 16) & MV_PP_CHIP_TYPE_MASK) >> 
		MV_PP_CHIP_TYPE_OFFSET;

	vendor = data & 0xffff;

    if ((vendor == MV_VENDOR_ID) && (chipType == MV_PP_CHIP_TYPE_XCAT2))
    {
		/* found xCat2 */
		/* read PEX BAR 1 from remote device */
		*remoteBar1 = mvPexConfigRead(4,bus,device + 1,func,0x18);
		/* remove control bits from address */
		*remoteBar1 = *remoteBar1 & 0xfffffff0;
		return MV_TRUE;
    }

    return MV_FALSE;
}


