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

#include "boardEnv/mvBoardEnvLib.h"
#include "ctrlEnv/mvCtrlEnvLib.h"
#include "ctrlEnv/sys/mvCpuIf.h"
#include "cpu/mvCpu.h"
#include "cntmr/mvCntmr.h"
#include "gpp/mvGpp.h"
#include "twsi/mvTwsi.h"
#include "pex/mvPex.h"
#include "device/mvDevice.h"
#include "eth-phy/mvEthPhy.h"
#include "mvSysHwConfig.h"
#include "mvPrestera.h"
#include "mvPresteraEEPROM.h"

/* defines  */
/* #define MV_DEBUG */
#ifdef MV_DEBUG
    #define DB(x)   x
#else
    #define DB(x)
#endif

/*******************************************************************************
 * externs
 */
MV_U32 _G_xCatIsFEBoardType;
MV_U32 _G_xCatRevision;

extern MV_CPU_ARM_CLK _cpuARMDDRCLK[];

#define CODE_IN_ROM     MV_FALSE
#define CODE_IN_RAM     MV_TRUE

extern  MV_BOARD_INFO *boardInfoTbl[];
#define BOARD_INFO(boardId) boardInfoTbl[boardId - BOARD_ID_BASE]

/* Locals */
static MV_DEV_CS_INFO *  boardGetDevEntry(MV_32 devNum, MV_BOARD_DEV_CLASS devClass);

MV_U32 tClkRate   = -1;

int db_98dx4122_detected = 0;

/*******************************************************************************
* mvBoardEnvInit - Init board
*
* DESCRIPTION:
*		In this function the board environment take care of device bank
*		initialization.
*
* INPUT:
*       None.
*
* OUTPUT:
*       None.
*
* RETURN:
*       None.
*
*******************************************************************************/
MV_VOID mvBoardEnvInit(MV_VOID)
{
    MV_U32 boardId = mvBoardIdGet ();

    if (boardId >= MV_MAX_BOARD_ID)
    {
        mvOsPrintf ("%s: Board unknown.\n", __func__);
        return;
    }

    /* Set GPP Out value */
    MV_REG_WRITE (GPP_DATA_OUT_REG (0), BOARD_INFO (boardId)->gppOutValLow);
    MV_REG_WRITE (GPP_DATA_OUT_REG (1), BOARD_INFO (boardId)->gppOutValHigh);

    /* set GPP polarity */
    mvGppPolaritySet (0, 0xFFFFFFFF, BOARD_INFO (boardId)->gppPolarityValLow);
    mvGppPolaritySet (1, 0xFFFFFFFF, BOARD_INFO (boardId)->gppPolarityValHigh);

    /* Workaround for Erratum FE-MISC-70*/
    if (mvCtrlRevGet () == MV_88F6XXX_A0_REV)
    {
        BOARD_INFO (boardId)->gppOutEnValLow &= 0xfffffffd;
        BOARD_INFO (boardId)->gppOutEnValLow |=
            (BOARD_INFO (boardId)->gppOutEnValHigh) & 0x00000002;
    } /* End of WA */

    /* Set GPP Out Enable*/
    mvGppTypeSet (0, 0xFFFFFFFF, BOARD_INFO (boardId)->gppOutEnValLow);
    mvGppTypeSet (1, 0xFFFFFFFF, BOARD_INFO (boardId)->gppOutEnValHigh);

    /* Nand CE */
    MV_REG_BIT_SET (NAND_CTRL_REG, NAND_ACTCEBOOT_BIT);
}

/*******************************************************************************
* mvBoardModelGet - Get Board model
*
* DESCRIPTION:
*       This function returns 16bit describing board model.
*       Board model is constructed of one byte major and minor numbers in the
*       following manner:
*
* INPUT:
*       None.
*
* OUTPUT:
*       None.
*
* RETURN:
*       String describing board model.
*
*******************************************************************************/
MV_U16 mvBoardModelGet(MV_VOID)
{
    return mvBoardIdGet () >> 16;
}

/*******************************************************************************
* mbBoardRevlGet - Get Board revision
*
* DESCRIPTION:
*       This function returns a 32bit describing the board revision.
*       Board revision is constructed of 4bytes. 2bytes describes major number
*       and the other 2bytes describes minor munber.
*       For example for board revision 3.4 the function will return
*       0x00030004.
*
* INPUT:
*       None.
*
* OUTPUT:
*       None.
*
* RETURN:
*       String describing board model.
*
*******************************************************************************/
MV_U16 mvBoardRevGet(MV_VOID)
{
    return mvBoardIdGet () & 0xFFFF;
}

/*******************************************************************************
* mvBoardNameGet - Get Board name
*
* DESCRIPTION:
*       This function returns a string describing the board model and revision.
*       String is extracted from board I2C EEPROM.
*
* INPUT:
*       None.
*
* OUTPUT:
*       pNameBuff - Buffer to contain board name string. Minimum size 32 chars.
*
* RETURN:
*
*       MV_ERROR if informantion can not be read.
*******************************************************************************/
MV_STATUS mvBoardNameGet(char *pNameBuff)
{
    MV_U32 boardId = mvBoardIdGet ();

    if (boardId >= MV_MAX_BOARD_ID)
    {
        mvOsSPrintf (pNameBuff, "Board unknown.\n");
        return MV_ERROR;
    }

    mvOsSPrintf (pNameBuff, "%s", BOARD_INFO (boardId)->boardName);

    return MV_OK;
}

/*******************************************************************************
* mvBoardIsPortInSgmii -
*
* DESCRIPTION:
*       This routine returns MV_TRUE for port number works in SGMII or MV_FALSE
*	For all other options.
*
* INPUT:
*       ethPortNum - Ethernet port number.
*
* OUTPUT:
*       None.
*
* RETURN:
*       MV_TRUE - port in SGMII.
*       MV_FALSE - other.
*
*******************************************************************************/
MV_BOOL mvBoardIsPortInSgmii(MV_U32 ethPortNum)
{
    MV_BOOL ethPortSgmiiSupport[BOARD_ETH_PORT_NUM] = MV_ETH_PORT_SGMII;

    if (ethPortNum >= BOARD_ETH_PORT_NUM)
    {
        mvOsPrintf ("Invalid portNo=%d\n", ethPortNum);
        return MV_FALSE;
    }
    return ethPortSgmiiSupport[ethPortNum];
}

/*******************************************************************************
* mvBoardIsPortInGmii -
*
* DESCRIPTION:
*       This routine returns MV_TRUE for port number works in GMII or MV_FALSE
*	For all other options.
*
* INPUT:
*
* OUTPUT:
*       None.
*
* RETURN:
*       MV_TRUE - port in GMII.
*       MV_FALSE - other.
*
*******************************************************************************/
MV_BOOL mvBoardIsPortInGmii(MV_VOID)
{
    MV_U32 devClassId, devClass = 0;
    if (mvBoardMppGroupTypeGet (devClass) == MV_BOARD_AUTO)
    {
        /* Get MPP module ID */
        devClassId = mvBoarModuleTypeGet (devClass);
        if (MV_BOARD_MODULE_GMII_ID == devClassId)
        {
            return MV_TRUE;
        }
    }
    else if (mvBoardMppGroupTypeGet (devClass) == MV_BOARD_GMII)
    {
        return MV_TRUE;
    }

    return MV_FALSE;
}
/*******************************************************************************
* mvBoardPhyAddrGet - Get the phy address
*
* DESCRIPTION:
*       This routine returns the Phy address of a given ethernet port.
*
* INPUT:
*       ethPortNum - Ethernet port number.
*
* OUTPUT:
*       None.
*
* RETURN:
*       32bit describing Phy address, -1 if the port number is wrong.
*
*******************************************************************************/
MV_32 mvBoardPhyAddrGet(MV_U32 ethPortNum)
{
    MV_U32 boardId = mvBoardIdGet ();

    if (boardId >= MV_MAX_BOARD_ID)
    {
        mvOsPrintf ("%s: Board unknown.\n", __func__);
        return MV_ERROR;
    }

    return BOARD_INFO (boardId)->pBoardMacInfo[ethPortNum].boardEthSmiAddr;
}

/*******************************************************************************
 * mvBoardPhyAddrSet
 */
MV_VOID mvBoardPhyAddrSet(MV_U32 ethPortNum, MV_32 smiAddr)
{
    MV_U32 boardId = mvBoardIdGet ();

    if (boardId >= MV_MAX_BOARD_ID)
    {
        mvOsPrintf ("%s: Board unknown.\n", __func__);
        return;
    }

    BOARD_INFO(boardId)->pBoardMacInfo[ethPortNum].boardEthSmiAddr = smiAddr;
}

/*******************************************************************************
* mvBoardMacSpeedGet - Get the Mac speed
*
* DESCRIPTION:
*       This routine returns the Mac speed if pre define of a given ethernet port.
*
* INPUT:
*       ethPortNum - Ethernet port number.
*
* OUTPUT:
*       None.
*
* RETURN:
*       MV_BOARD_MAC_SPEED, -1 if the port number is wrong.
*
*******************************************************************************/
MV_BOARD_MAC_SPEED      mvBoardMacSpeedGet(MV_U32 ethPortNum)
{
    MV_U32 boardId = mvBoardIdGet ();

    if (boardId >= MV_MAX_BOARD_ID)
    {
        mvOsPrintf ("mvBoardMacSpeedGet: Board unknown.\n");
        return MV_ERROR;
    }

    return BOARD_INFO (boardId)->pBoardMacInfo[ethPortNum].boardMacSpeed;
}

/*******************************************************************************
* mvBoardLinkStatusIrqGet - Get the IRQ number for the link status indication
*
* DESCRIPTION:
*       This routine returns the IRQ number for the link status indication.
*
* INPUT:
*       ethPortNum - Ethernet port number.
*
* OUTPUT:
*       None.
*
* RETURN:
*       the number of the IRQ for the link status indication, -1 if the port
*	number is wrong or if not relevant.
*
*******************************************************************************/
MV_32   mvBoardLinkStatusIrqGet(MV_U32 ethPortNum)
{
    MV_U32 boardId = mvBoardIdGet ();

    if (boardId >= MV_MAX_BOARD_ID)
    {
        mvOsPrintf ("mvBoardLinkStatusIrqGet: Board unknown.\n");
        return MV_ERROR;
    }

    return BOARD_INFO (boardId)->pSwitchInfo[ethPortNum].linkStatusIrq;
}

/*******************************************************************************
* mvBoardSwitchPortGet - Get the mapping between the board connector and the
* Ethernet Switch port
*
* DESCRIPTION:
*       This routine returns the matching Switch port.
*
* INPUT:
*       ethPortNum - Ethernet port number.
*	boardPortNum - logical number of the connector on the board
*
* OUTPUT:
*       None.
*
* RETURN:
*       the matching Switch port, -1 if the port number is wrong or if not relevant.
*
*******************************************************************************/
MV_32   mvBoardSwitchPortGet(MV_U32 ethPortNum, MV_U8 boardPortNum)
{
    MV_U32 boardId = mvBoardIdGet ();

    if (boardId >= MV_MAX_BOARD_ID)
    {
        mvOsPrintf ("%s: Board unknown.\n", __func__);
        return MV_ERROR;
    }
    if (boardPortNum >= BOARD_ETH_SWITCH_PORT_NUM)
    {
        mvOsPrintf ("mvBoardSwitchPortGet: Illegal board port number.\n");
        return MV_ERROR;
    }

    return BOARD_INFO (boardId)->pSwitchInfo[ethPortNum].qdPort[boardPortNum];
}

/*******************************************************************************
* mvBoardSwitchCpuPortGet - Get the the Ethernet Switch CPU port
*
* DESCRIPTION:
*       This routine returns the Switch CPU port.
*
* INPUT:
*       ethPortNum - Ethernet port number.
*
* OUTPUT:
*       None.
*
* RETURN:
*       the Switch CPU port, -1 if the port number is wrong or if not relevant.
*
*******************************************************************************/
MV_32   mvBoardSwitchCpuPortGet(MV_U32 ethPortNum)
{
    MV_U32 boardId = mvBoardIdGet ();

    if (boardId >= MV_MAX_BOARD_ID)
    {
        mvOsPrintf ("%s: Board unknown.\n", __func__);
        return MV_ERROR;
    }

    return BOARD_INFO (boardId)->pSwitchInfo[ethPortNum].qdCpuPort;
}

/*******************************************************************************
* mvBoardIsSwitchConnected - Get switch connection status
* DESCRIPTION:
*       This routine returns port's connection status
*
* INPUT:
*       ethPortNum - Ethernet port number.
*
* OUTPUT:
*       None.
*
* RETURN:
*       1 - if ethPortNum is connected to switch, 0 otherwise
*
*******************************************************************************/
MV_32   mvBoardIsSwitchConnected(MV_U32 ethPortNum)
{
    MV_U32 boardId = mvBoardIdGet ();

    if (boardId >= MV_MAX_BOARD_ID)
    {
        mvOsPrintf ("%s: Board unknown.\n", __func__);
        return MV_ERROR;
    }

    if (ethPortNum >= BOARD_INFO (boardId)->numBoardMacInfo)
    {
        mvOsPrintf ("mvBoardIsSwitchConnected: Illegal port number(%u)\n",
                    ethPortNum);
        return MV_ERROR;
    }

    if ((MV_32)(BOARD_INFO (boardId)->pSwitchInfo))
    {
        return (MV_32)(BOARD_INFO (boardId)->pSwitchInfo[ethPortNum].switchOnPort ==
                       ethPortNum);
    }
    else
    {
        return 0;
    }
}
/*******************************************************************************
* mvBoardSmiScanModeGet - Get Switch SMI scan mode
*
* DESCRIPTION:
*       This routine returns Switch SMI scan mode.
*
* INPUT:
*       ethPortNum - Ethernet port number.
*
* OUTPUT:
*       None.
*
* RETURN:
*       1 for SMI_MANUAL_MODE, -1 if the port number is wrong or if not relevant.
*
*******************************************************************************/
MV_32   mvBoardSmiScanModeGet(MV_U32 ethPortNum)
{
    MV_U32 boardId = mvBoardIdGet ();

    if (boardId >= MV_MAX_BOARD_ID)
    {
        mvOsPrintf ("%s: Board unknown.\n", __func__);
        return MV_ERROR;
    }

    return BOARD_INFO (boardId)->pSwitchInfo[ethPortNum].smiScanMode;
}
/*******************************************************************************
* mvBoardSpecInitGet -
*
* DESCRIPTION:
*
* INPUT:
*
* OUTPUT:
*       None.
*
* RETURN: Return MV_TRUE and parameters in case board need spesific phy init,
*     otherwise return MV_FALSE.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_BOOL mvBoardSpecInitGet(MV_U32 *regOff, MV_U32 *data)
{
    return MV_FALSE;
}

/*******************************************************************************
* mvBoardTclkGetXCat
*
* DESCRIPTION:
*       This routine extract the controller core clock.
*       This function uses the controller counters to make identification.
*       Note: In order to avoid interference, make sure task context switch
*       and interrupts will not occure during this function operation
*
* INPUT:
*       countNum - Counter number.
*
* OUTPUT:
*       None.
*
* RETURN:
*       32bit clock cycles in Hertz.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_U32 mvBoardTclkGetXCat(MV_VOID)
{
#if defined(TCLK_AUTO_DETECT)
    MV_U32 tmpTClkRate = MV_BOARD_TCLK_166MHZ;

    tmpTClkRate = MV_REG_READ (MPP_SAMPLE_AT_RESET);
    tmpTClkRate &= MSAR_TCLCK_MASK;

    switch (tmpTClkRate)
    {
    case MSAR_TCLCK_166:
        tmpTClkRate = MV_BOARD_TCLK_166MHZ;
        break;
    case MSAR_TCLCK_200:
        tmpTClkRate = MV_BOARD_TCLK_200MHZ;
        break;
    }
    return tmpTClkRate;

#else
    if (mvCtrlModelGet () == MV_6281_DEV_ID)
    {
        return MV_BOARD_TCLK_200MHZ;
    }
    else
    {
        return MV_BOARD_TCLK_166MHZ;
    }
#endif
}

/*******************************************************************************
* mvBoardTclkGetXCat2
*
* DESCRIPTION:
*       This routine extract the controller core clock.
*       This function uses the controller counters to make identification.
*       Note: In order to avoid interference, make sure task context switch
*       and interrupts will not occure during this function operation
*
* INPUT:
*       countNum - Counter number.
*
* OUTPUT:
*       None.
*
* RETURN:
*       32bit clock cycles in Hertz.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_U32 mvBoardTclkGetXCat2(MV_VOID)
{
#if defined(TCLK_AUTO_DETECT)
    MV_U32 tclk, tmpTClkRate = MV_BOARD_TCLK_166MHZ;
    MV_U32 tclkArr[] = MV_TCLK_TBL_XCAT2;

    tmpTClkRate = MV_REG_READ (MPP_SAMPLE_AT_RESET);
    tmpTClkRate = (tmpTClkRate & 0x70000000); /* bits 28-30 */
    tmpTClkRate = (tmpTClkRate >> 28);

    tclk = tclkArr[tmpTClkRate];

    return tclk;
#else
    return MV_BOARD_TCLK_167MHZ;
#endif
}

/*******************************************************************************
* mvBoardTclkGet
*
* DESCRIPTION:
*       This routine extract the controller core clock.
*       This function uses the controller counters to make identification.
*       Note: In order to avoid interference, make sure task context switch
*       and interrupts will not occure during this function operation
*
* INPUT:
*       countNum - Counter number.
*
* OUTPUT:
*       None.
*
* RETURN:
*       32bit clock cycles in Hertz.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_U32 mvBoardTclkGet(MV_VOID)
{
    if (mvPpChipIsXCat2() == MV_TRUE)
    {
        return mvBoardTclkGetXCat2();
    }
    else
    {
        return mvBoardTclkGetXCat();
    }
}

/*******************************************************************************
* mvBoard6180SysClkGet - Get the board SysClk (CPU bus clock)
*
* DESCRIPTION:
*       This routine extract the CPU bus clock.
*
* INPUT:
*       countNum - Counter number.
*
* OUTPUT:
*       None.
*
* RETURN:
*       32bit clock cycles in Hertz.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static MV_U32 mvBoard6180SysClkGet(MV_VOID)
{
    MV_U32  sysClkRate = 0;
    MV_CPU_ARM_CLK _cpu6180_ddr_l2_CLK[] = MV_CPU6180_DDR_L2_CLCK_TBL;

    sysClkRate = MV_REG_READ (MPP_SAMPLE_AT_RESET);
    sysClkRate = sysClkRate & MSAR_CPUCLCK_MASK_6180;
    sysClkRate = sysClkRate >> MSAR_CPUCLCK_OFFS_6180;

    sysClkRate = _cpu6180_ddr_l2_CLK[sysClkRate].ddrClk;

    return sysClkRate;
}

/*******************************************************************************
* mvBoardSysClkGetXCat
*
* DESCRIPTION:
*       Finds out SYS_CLK value from SAR register.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       Value that represents chip SYSCLK. For example:
*       333000000 - 333 MHz
*       500000000 - 500 MHz
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_U32 mvBoardSysClkGetXCat(MV_VOID)
{
#ifdef SYSCLK_AUTO_DETECT
    MV_U32 sysClkRate, tmp, pClkRate, indexDdrRtio;
    MV_U32 cpuCLK[] = MV_CPU_CLCK_TBL_XCAT;
    MV_U32 ddrRtio[][2] = MV_DDR_CLCK_RTIO_TBL;

    if (mvCtrlModelGet () == MV_6180_DEV_ID)
    {
        return mvBoard6180SysClkGet ();
    }

    tmp = MV_REG_READ (MPP_SAMPLE_AT_RESET);
    pClkRate = MSAR_CPUCLCK_EXTRACT (tmp);
    pClkRate = cpuCLK[pClkRate];

    indexDdrRtio = tmp & MSAR_DDRCLCK_RTIO_MASK;
    indexDdrRtio = indexDdrRtio >> MSAR_DDRCLCK_RTIO_OFFS;
    if (ddrRtio[indexDdrRtio][0] != 0)
    {
        sysClkRate =
            ((pClkRate * ddrRtio[indexDdrRtio][1]) / ddrRtio[indexDdrRtio][0]);
    }
    else
    {
        sysClkRate = 0;
    }
    return sysClkRate;

#else
    return MV_BOARD_DEFAULT_SYSCLK;

#endif
}

/*******************************************************************************
* mvBoardSysClkGetXCat2
*
* DESCRIPTION:
*       Finds out SYS_CLK value from SAR register.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       Value that represents chip SYSCLK. For example:
*       333000000 - 333 MHz
*       500000000 - 500 MHz
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_U32 mvBoardSysClkGetXCat2(MV_VOID)
{
#ifdef SYSCLK_AUTO_DETECT
    MV_U32 sysClkRate, tmp, pClkRate, indexDdrRtio;
    MV_U32 cpuCLK[] = MV_CPU_CLCK_TBL_XCAT2;
    MV_U32 ddrRtio[][2] = MV_DDR_CLCK_RTIO_TBL_XCAT2;

    tmp = MV_REG_READ (MPP_SAMPLE_AT_RESET);
    pClkRate = MSAR_CPUCLCK_EXTRACT (tmp);
    pClkRate = cpuCLK[pClkRate];

    indexDdrRtio = tmp & MSAR_DDRCLCK_RTIO_MASK;
    indexDdrRtio = indexDdrRtio >> MSAR_DDRCLCK_RTIO_OFFS;
    if (ddrRtio[indexDdrRtio][0] != 0)
    {
        sysClkRate =
            ((pClkRate * ddrRtio[indexDdrRtio][1]) / ddrRtio[indexDdrRtio][0]);
    }
    else
    {
        sysClkRate = 0;
    }
    return sysClkRate;

#else
    return MV_BOARD_DEFAULT_SYSCLK;

#endif
}

/*******************************************************************************
* mvBoardSysClkGet
*
* DESCRIPTION:
*       Finds out SYS_CLK value from SAR register and with regard to the
*       chip type (xCat or xCat2).
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       Value that represents chip SYSCLK. For example:
*       333000000 - 333 MHz
*       500000000 - 500 MHz
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_U32 mvBoardSysClkGet(MV_VOID)
{
    if (mvPpChipIsXCat2() == MV_TRUE)
    {
        return mvBoardSysClkGetXCat2();
    }
    else
    {
        return mvBoardSysClkGetXCat();
    }
}

/*******************************************************************************
* mvBoardPexBridgeIntPinGet - Get PEX to PCI bridge interrupt pin number
*
* DESCRIPTION:
*		Multi-ported PCI Express bridges that is implemented on the board
*		collapse interrupts across multiple conventional PCI/PCI-X buses.
*		A dual-headed PCI Express bridge would map (or "swizzle") the
*		interrupts per the following table (in accordance with the respective
*		logical PCI/PCI-X bridge's Device Number), collapse the INTA#-INTD#
*		signals from its two logical PCI/PCI-X bridges, collapse the
*		INTA#-INTD# signals from any internal sources, and convert the
*		signals to in-band PCI Express messages. 10
*		This function returns the upstream interrupt as it was converted by
*		the bridge, according to board configuration and the following table:
*					        PCI dev num
*			Interrupt pin   7,  8,  9
*		            A  ->	A	D	C
*		            B  ->   B	A	D
*		            C  ->   C	B	A
*		            D  ->	D	C	B
*
*
* INPUT:
*       devNum - PCI/PCIX device number.
*       intPin - PCI Int pin
*
* OUTPUT:
*       None.
*
* RETURN:
*       Int pin connected to the Interrupt controller
*
*******************************************************************************/
MV_U32 mvBoardPexBridgeIntPinGet(MV_U32 devNum, MV_U32 intPin)
{
    MV_U32 realIntPin = ((intPin + (3 - (devNum % 4))) % 4);

    if (realIntPin == 0)
    {
        return 4;
    }
    else
    {
        return realIntPin;
    }
}

/*******************************************************************************
* mvBoardDebugLedNumGet - Get number of debug Leds
*
* DESCRIPTION:
* INPUT:
*       boardId
*
* OUTPUT:
*       None.
*
* RETURN:
*       None.
*
*******************************************************************************/
MV_U32 mvBoardDebugLedNumGet(MV_U32 boardId)
{
    return BOARD_INFO (boardId)->activeLedsNumber;
}

/*******************************************************************************
* mvBoardDebugLeg - Set the board debug Leds
*
* DESCRIPTION: turn on/off status leds.
*          Note: assume MPP leds are part of group 0 only.
*
* INPUT:
*       hexNum - Number to be displied in hex by Leds.
*
* OUTPUT:
*       None.
*
* RETURN:
*       None.
*
*******************************************************************************/
MV_VOID mvBoardDebugLed(MV_U32 hexNum)
{
    MV_U32 val = 0, totalMask, currentBitMask = 1, i;
    MV_U32 boardId = mvBoardIdGet ();

    if (BOARD_INFO (boardId)->pLedGppPin == NULL)
    {
        return;
    }

    totalMask = (1 << BOARD_INFO (boardId)->activeLedsNumber) - 1;
    hexNum &= totalMask;
    totalMask = 0;

    for (i = 0 ; i < BOARD_INFO (boardId)->activeLedsNumber ; i++)
    {
        if (hexNum & currentBitMask)
        {
            val |= (1 << BOARD_INFO (boardId)->pLedGppPin[i]);
        }

        totalMask |= (1 << BOARD_INFO (boardId)->pLedGppPin[i]);

        currentBitMask = (currentBitMask << 1);
    }

    if (BOARD_INFO (boardId)->ledsPolarity)
    {
        mvGppValueSet (0, totalMask, val);
    }
    else
    {
        mvGppValueSet (0, totalMask, ~val);
    }
}

/*******************************************************************************
* mvBoarGpioPinGet - mvBoarGpioPinGet
*
* DESCRIPTION:
*
* INPUT:
*		class - MV_BOARD_GPP_CLASS enum.
*
* OUTPUT:
*		None.
*
* RETURN:
*       GPIO pin number. The function return -1 for bad parameters.
*
*******************************************************************************/
MV_32 mvBoarGpioPinNumGet(MV_BOARD_GPP_CLASS boardClass, MV_U32 index)
{
    MV_U32 i, indexFound = 0;
    MV_U32 boardId = mvBoardIdGet ();

    if (boardId >= MV_MAX_BOARD_ID)
    {
        mvOsPrintf ("%s: Board unknown.\n", __func__);
        return MV_ERROR;
    }

    for (i = 0; i < BOARD_INFO (boardId)->numBoardGppInfo; i++)
    {
        if (BOARD_INFO (boardId)->pBoardGppInfo[i].devClass == boardClass)
        {
            if (indexFound == index)
            {
                return (MV_U32)BOARD_INFO (boardId)->pBoardGppInfo[i].gppPinNum;
            }
            else
            {
                indexFound++;
            }
        }
    }

    return MV_ERROR;
}

/*******************************************************************************
* mvBoardRTCGpioPinGet - mvBoardRTCGpioPinGet
*
* DESCRIPTION:
*
* INPUT:
*		None.
*
* OUTPUT:
*		None.
*
* RETURN:
*       GPIO pin number. The function return -1 for bad parameters.
*
*******************************************************************************/
MV_32 mvBoardRTCGpioPinGet(MV_VOID)
{
    return mvBoarGpioPinNumGet (BOARD_GPP_RTC, 0);
}

/*******************************************************************************
* mvBoardReset - mvBoardReset
*
* DESCRIPTION:
*			Reset the board
* INPUT:
*		None.
*
* OUTPUT:
*		None.
*
* RETURN:
*       None
*
*******************************************************************************/
MV_VOID mvBoardReset(MV_VOID)
{
    MV_32 resetPin;

    /* Get gpp reset pin if define */
    resetPin = mvBoardResetGpioPinGet ();
    if (resetPin != MV_ERROR)
    {
        MV_REG_BIT_RESET (GPP_DATA_OUT_REG (0), (1 << resetPin));
        MV_REG_BIT_RESET (GPP_DATA_OUT_EN_REG (0), (1 << resetPin));
    }
    else
    {
        /* No gpp reset pin was found, try to reset ussing
           system reset out */
        MV_REG_BIT_SET (CPU_RSTOUTN_MASK_REG, BIT2);
        MV_REG_BIT_SET (CPU_SYS_SOFT_RST_REG, BIT0);
    }
}

/*******************************************************************************
* mvBoardResetGpioPinGet - mvBoardResetGpioPinGet
*
* DESCRIPTION:
*
* INPUT:
*		None.
*
* OUTPUT:
*		None.
*
* RETURN:
*       GPIO pin number. The function return -1 for bad parameters.
*
*******************************************************************************/
MV_32 mvBoardResetGpioPinGet(MV_VOID)
{
    return mvBoarGpioPinNumGet (BOARD_GPP_RESET, 0);
}
/*******************************************************************************
* mvBoardSDIOGpioPinGet - mvBoardSDIOGpioPinGet
*
* DESCRIPTION:
*	used for hotswap detection
* INPUT:
*		None.
*
* OUTPUT:
*		None.
*
* RETURN:
*       GPIO pin number. The function return -1 for bad parameters.
*
*******************************************************************************/
MV_32  mvBoardSDIOGpioPinGet(MV_VOID)
{
    return mvBoarGpioPinNumGet (BOARD_GPP_SDIO_DETECT, 0);
}

/*******************************************************************************
* mvBoardUSBVbusGpioPinGet - return Vbus input GPP
*
* DESCRIPTION:
*
* INPUT:
*		int  devNo.
*
* OUTPUT:
*		None.
*
* RETURN:
*       GPIO pin number. The function return -1 for bad parameters.
*
*******************************************************************************/
MV_32 mvBoardUSBVbusGpioPinGet(MV_32 devId)
{
    return mvBoarGpioPinNumGet (BOARD_GPP_USB_VBUS, devId);
}

/*******************************************************************************
* mvBoardUSBVbusEnGpioPinGet - return Vbus Enable output GPP
*
* DESCRIPTION:
*
* INPUT:
*		int  devNo.
*
* OUTPUT:
*		None.
*
* RETURN:
*       GPIO pin number. The function return -1 for bad parameters.
*
*******************************************************************************/
MV_32 mvBoardUSBVbusEnGpioPinGet(MV_32 devId)
{
    return mvBoarGpioPinNumGet (BOARD_GPP_USB_VBUS_EN, devId);
}

/*******************************************************************************
* mvBoardGpioIntMaskGet - Get GPIO mask for interrupt pins
*
* DESCRIPTION:
*		This function returns a 32-bit mask of GPP pins that connected to
*		interrupt generating sources on board.
*		For example if UART channel A is hardwired to GPP pin 8 and
*		UART channel B is hardwired to GPP pin 4 the fuinction will return
*		the value 0x000000110
*
* INPUT:
*		None.
*
* OUTPUT:
*		None.
*
* RETURN:
*		See description. The function return -1 if board is not identified.
*
*******************************************************************************/
MV_32 mvBoardGpioIntMaskLowGet(MV_VOID)
{
    MV_U32 boardId = mvBoardIdGet ();

    if (boardId >= MV_MAX_BOARD_ID)
    {
        mvOsPrintf ("%s: Board unknown.\n", __func__);
        return MV_ERROR;
    }

    return BOARD_INFO (boardId)->intsGppMaskLow;
}

MV_32 mvBoardGpioIntMaskHighGet(MV_VOID)
{
    MV_U32 boardId = mvBoardIdGet ();

    if (boardId >= MV_MAX_BOARD_ID)
    {
        mvOsPrintf ("%s: Board unknown.\n", __func__);
        return MV_ERROR;
    }

    return BOARD_INFO (boardId)->intsGppMaskHigh;
}

/*******************************************************************************
* mvBoardMppGet - Get board dependent MPP register value
*
* DESCRIPTION:
*		MPP settings are derived from board design.
*		MPP group consist of 8 MPPs. An MPP group represent MPP
*		control register.
*       This function retrieves board dependend MPP register value.
*
* INPUT:
*       mppGroupNum - MPP group number.
*
* OUTPUT:
*       None.
*
* RETURN:
*       32bit value describing MPP control register value.
*
*******************************************************************************/
MV_32 mvBoardMppGet(MV_U32 mppGroupNum)
{
    MV_U32 boardId = mvBoardIdGet ();

    if (boardId >= MV_MAX_BOARD_ID)
    {
        mvOsPrintf ("%s: Board unknown.\n", __func__);
        return MV_ERROR;
    }

    /* Fix MPP values for xCat2 (NAND related). */
    if (mvPpChipIsXCat2() == MV_TRUE)
    {
        DB(mvOsPrintf("%s: XCAT2 case.\n", __func__));
        BOARD_INFO(boardId)->pBoardMppConfigValue[0].mppGroup[2] =
                   DB_98DX4122_MPP16_23_XCAT2;
        BOARD_INFO(boardId)->pBoardMppConfigValue[0].mppGroup[3] =
                   DB_98DX4122_MPP24_31_XCAT2;
        BOARD_INFO(boardId)->pBoardMppConfigValue[0].mppGroup[4] =
                   DB_98DX4122_MPP32_39_XCAT2;
    }

    return BOARD_INFO (boardId)->pBoardMppConfigValue[0].mppGroup[mppGroupNum];
}

/*******************************************************************************
* mvBoardMppGroupId - If MPP group type is AUTO then identify it using twsi
*
* DESCRIPTION:
*
* INPUT:
*
* OUTPUT:
*       None.
*
* RETURN:
*
*******************************************************************************/
MV_VOID mvBoardMppGroupIdUpdate(MV_VOID)
{
    MV_BOARD_MPP_GROUP_CLASS devClass;
    MV_BOARD_MODULE_ID_CLASS devClassId;
    MV_BOARD_MPP_TYPE_CLASS mppGroupType;
    MV_U32 devId;
    MV_U32 maxMppGrp = 1;

    devId = mvCtrlModelGet ();

    switch (devId)
    {
    case MV_6281_DEV_ID:
        maxMppGrp = MV_6281_MPP_MAX_MODULE;
        break;
    case MV_6192_DEV_ID:
        maxMppGrp = MV_6192_MPP_MAX_MODULE;
        break;
    case MV_6190_DEV_ID:
        maxMppGrp = MV_6190_MPP_MAX_MODULE;
        break;
    case MV_6180_DEV_ID:
        maxMppGrp = MV_6180_MPP_MAX_MODULE;
        break;
    }

    for (devClass = 0; devClass < maxMppGrp; devClass++)
    {
        /* If MPP group can be defined by the module connected to it */
        if (mvBoardMppGroupTypeGet (devClass) == MV_BOARD_AUTO)
        {
            /* Get MPP module ID */
            devClassId = mvBoarModuleTypeGet (devClass);
            if (MV_ERROR != devClassId)
            {
                switch (devClassId)
                {
                case MV_BOARD_MODULE_TDM_ID:
                    mppGroupType = MV_BOARD_TDM;
                    break;
                case MV_BOARD_MODULE_AUDIO_ID:
                    mppGroupType = MV_BOARD_AUDIO;
                    break;
                case MV_BOARD_MODULE_RGMII_ID:
                    mppGroupType = MV_BOARD_RGMII;
                    break;
                case MV_BOARD_MODULE_GMII_ID:
                    mppGroupType = MV_BOARD_GMII;
                    break;
                case MV_BOARD_MODULE_TS_ID:
                    mppGroupType = MV_BOARD_TS;
                    break;
                default:
                    mppGroupType = MV_BOARD_OTHER;
                    break;
                }
            }
            else
            {
                /* The module bay is empty */
                mppGroupType = MV_BOARD_OTHER;
            }

            /* Update MPP group type */
            mvBoardMppGroupTypeSet (devClass, mppGroupType);
        }

        /* Update MPP output voltage for RGMII 1.8V */
        if ((mvBoardMppGroupTypeGet (devClass) == MV_BOARD_RGMII))
        {
            MV_REG_BIT_SET (MPP_OUTPUT_DRIVE_REG, MPP_1_8_RGMII1_OUTPUT_DRIVE);
        }
    }
}

/*******************************************************************************
* mvBoardMppGroupTypeGet
*
* DESCRIPTION:
*
* INPUT:
*       mppGroupClass - MPP group number 0  for MPP[35:20] or 1 for MPP[49:36].
*
* OUTPUT:
*       None.
*
* RETURN:
*
*******************************************************************************/
MV_BOARD_MPP_TYPE_CLASS mvBoardMppGroupTypeGet(
    MV_BOARD_MPP_GROUP_CLASS mppGroupClass)
{
    MV_U32 boardId = mvBoardIdGet ();

    if (boardId >= MV_MAX_BOARD_ID)
    {
        mvOsPrintf ("%s: Board unknown.\n", __func__);
        return MV_ERROR;
    }

    if (mppGroupClass == MV_BOARD_MPP_GROUP_1)
    {
        return BOARD_INFO (boardId)->pBoardMppTypeValue[0].boardMppGroup1;
    }
    else
    {
        return BOARD_INFO (boardId)->pBoardMppTypeValue[0].boardMppGroup2;
    }
}

/*******************************************************************************
* mvBoardMppGroupTypeSet
*
* DESCRIPTION:
*
* INPUT:
*       mppGroupClass - MPP group number 0  for MPP[35:20] or 1 for MPP[49:36].
*       mppGroupType - MPP group type for MPP[35:20] or for MPP[49:36].
*
* OUTPUT:
*       None.
*
* RETURN:
*
*******************************************************************************/
MV_VOID mvBoardMppGroupTypeSet(MV_BOARD_MPP_GROUP_CLASS mppGroupClass,
                               MV_BOARD_MPP_TYPE_CLASS  mppGroupType)
{
    MV_U32 boardId = mvBoardIdGet ();

    if (boardId >= MV_MAX_BOARD_ID)
    {
        mvOsPrintf ("%s: Board unknown.\n", __func__);
        return;
    }

    if (mppGroupClass == MV_BOARD_MPP_GROUP_1)
    {
        BOARD_INFO (boardId)->pBoardMppTypeValue[0].boardMppGroup1 = mppGroupType;
    }
    else
    {
        BOARD_INFO (boardId)->pBoardMppTypeValue[0].boardMppGroup2 = mppGroupType;
    }
}

/*******************************************************************************
* mvBoardMppMuxSet - Update MPP mux
*
* DESCRIPTION:
*
* INPUT:
*
* OUTPUT:
*       None.
*
* RETURN:
*
*******************************************************************************/
MV_VOID mvBoardMppMuxSet(MV_VOID)
{
    MV_BOARD_MPP_GROUP_CLASS devClass;
    MV_BOARD_MPP_TYPE_CLASS mppGroupType;
    MV_U32 devId;
    MV_U8 muxVal = 0;
    MV_U32 maxMppGrp = 1;
    MV_TWSI_SLAVE twsiSlave;
    MV_TWSI_ADDR slave;

    devId = mvCtrlModelGet ();

    switch (devId)
    {
    case MV_6281_DEV_ID:
        maxMppGrp = MV_6281_MPP_MAX_MODULE;
        break;
    case MV_6192_DEV_ID:
        maxMppGrp = MV_6192_MPP_MAX_MODULE;
        break;
    case MV_6190_DEV_ID:
        maxMppGrp = MV_6190_MPP_MAX_MODULE;
        break;
    case MV_6180_DEV_ID:
        maxMppGrp = MV_6180_MPP_MAX_MODULE;
        break;
    }

    for (devClass = 0; devClass < maxMppGrp; devClass++)
    {
        mppGroupType = mvBoardMppGroupTypeGet (devClass);

        switch (mppGroupType)
        {
        case MV_BOARD_TDM:
            muxVal &= ~(devClass ? (0x2 << (devClass * 2)) : 0);
            break;
        case MV_BOARD_AUDIO:
            muxVal &= ~(devClass ? 0xd : 0);
            break;
        case MV_BOARD_TS:
            muxVal &= ~(devClass ? (0x1 << (devClass * 2)) : 0);
            break;
        default:
            muxVal |= (devClass ? 0xf : 0);
            break;
        }
    }

    /* TWSI init */
    slave.type = ADDR7_BIT;
    slave.address = 0;
    mvTwsiInit (0, TWSI_SPEED, mvBoardTclkGet (), &slave, 0);

    /* Read MPP module ID */
    DB (mvOsPrintf ("Board: twsi exp set\n"));
    twsiSlave.slaveAddr.address = mvBoardTwsiExpAddrGet (MV_BOARD_MUX_I2C_ADDR_ENTRY);
    twsiSlave.slaveAddr.type = mvBoardTwsiExpAddrTypeGet (
        MV_BOARD_MUX_I2C_ADDR_ENTRY);
    twsiSlave.validOffset = MV_TRUE;
    /* Offset is the first command after the address which indicate the register
       number to be read
       in next operation */
    twsiSlave.offset = 2;
    twsiSlave.moreThen256 = MV_FALSE;

    if (MV_OK != mvTwsiWrite (0, &twsiSlave, &muxVal, 1))
    {
        DB (mvOsPrintf ("Board: twsi exp out val fail\n"));
        return;
    }
    DB (mvOsPrintf ("Board: twsi exp out val succeded\n"));

    /* Change twsi exp to output */
    twsiSlave.offset = 6;
    muxVal = 0;
    if (MV_OK != mvTwsiWrite (0, &twsiSlave, &muxVal, 1))
    {
        DB (mvOsPrintf ("Board: twsi exp change to out fail\n"));
        return;
    }
    DB (mvOsPrintf ("Board: twsi exp change to out succeded\n"));
}

/*******************************************************************************
* mvBoardTdmMppSet - set MPPs in TDM module
*
* DESCRIPTION:
*
* INPUT: type of second telephony device
*
* OUTPUT:
*       None.
*
* RETURN:
*
*******************************************************************************/
MV_VOID mvBoardTdmMppSet(MV_32 chType)
{
    MV_BOARD_MPP_GROUP_CLASS devClass;
    MV_BOARD_MPP_TYPE_CLASS mppGroupType;
    MV_U32 devId;
    MV_U8 muxVal = 1;
    MV_U8 muxValMask = 1;
    MV_U8 twsiVal;
    MV_U32 maxMppGrp = 1;
    MV_TWSI_SLAVE twsiSlave;
    MV_TWSI_ADDR slave;

    devId = mvCtrlModelGet ();

    switch (devId)
    {
    case MV_6281_DEV_ID:
        maxMppGrp = MV_6281_MPP_MAX_MODULE;
        break;
    case MV_6192_DEV_ID:
        maxMppGrp = MV_6192_MPP_MAX_MODULE;
        break;
    case MV_6190_DEV_ID:
        maxMppGrp = MV_6190_MPP_MAX_MODULE;
        break;
    case MV_6180_DEV_ID:
        maxMppGrp = MV_6180_MPP_MAX_MODULE;
        break;
    }

    for (devClass = 0; devClass < maxMppGrp; devClass++)
    {
        mppGroupType = mvBoardMppGroupTypeGet (devClass);
        if (mppGroupType == MV_BOARD_TDM)
        {
            break;
        }
    }

    if (devClass == maxMppGrp)
    {
        return;     /* TDM module not found */
    }
    /* TWSI init */
    slave.type = ADDR7_BIT;
    slave.address = 0;
    mvTwsiInit (0, TWSI_SPEED, mvBoardTclkGet (), &slave, 0);

    /* Read MPP module ID */
    DB (mvOsPrintf ("Board: twsi exp set\n"));
    twsiSlave.slaveAddr.address = mvBoardTwsiExpAddrGet (0);
    twsiSlave.slaveAddr.type = ADDR7_BIT;
    twsiSlave.validOffset = MV_TRUE;
    /* Offset is the first command after the address which indicate the register
       number to be read
       in next operation */
    twsiSlave.offset = 3;
    twsiSlave.moreThen256 = MV_FALSE;

    if (mvBoardIdGet () == RD_88F6281A_ID)
    {
        muxVal = 0xc;
        muxValMask = 0xf3;
    }

    mvTwsiRead (0, &twsiSlave, &twsiVal, 1);
    muxVal = (twsiVal & muxValMask) | muxVal;

    if (MV_OK != mvTwsiWrite (0, &twsiSlave, &muxVal, 1))
    {
        mvOsPrintf ("Board: twsi exp out val fail\n");
        return;
    }
    DB (mvOsPrintf ("Board: twsi exp out val succeded\n"));

    /* Change twsi exp to output */
    twsiSlave.offset = 7;
    muxVal = 0xfe;
    if (mvBoardIdGet () == RD_88F6281A_ID)
    {
        muxVal = 0xf3;
    }

    mvTwsiRead (0, &twsiSlave, &twsiVal, 1);
    muxVal = (twsiVal & muxVal);

    if (MV_OK != mvTwsiWrite (0, &twsiSlave, &muxVal, 1))
    {
        mvOsPrintf ("Board: twsi exp change to out fail\n");
        return;
    }
    DB (mvOsPrintf ("Board: twsi exp change to out succeded\n"));
    /* reset the line to 0 */
    twsiSlave.offset = 3;
    muxVal = 0;
    muxValMask = 1;

    if (mvBoardIdGet () == RD_88F6281A_ID)
    {
        muxVal = 0x0;
        muxValMask = 0xf3;
    }

    mvTwsiRead (0, &twsiSlave, &twsiVal, 1);
    muxVal = (twsiVal & muxValMask) | muxVal;

    if (MV_OK != mvTwsiWrite (0, &twsiSlave, &muxVal, 1))
    {
        mvOsPrintf ("Board: twsi exp out val fail\n");
        return;
    }
    DB (mvOsPrintf ("Board: twsi exp out val succeded\n"));

    mvOsDelay (20);

    /* set the line to 1 */
    twsiSlave.offset = 3;
    muxVal = 1;
    muxValMask = 1;

    if (mvBoardIdGet () == RD_88F6281A_ID)
    {
        muxVal = 0xc;
        muxValMask = 0xf3;
        if (chType) /* FXS - issue reset properly */
        {
            MV_REG_BIT_SET (GPP_DATA_OUT_REG (1), MV_GPP12);
            mvOsDelay (50);
            MV_REG_BIT_RESET (GPP_DATA_OUT_REG (1), MV_GPP12);
        }
        else /* FXO - issue reset via TDM_CODEC_RST*/
        {
            /* change MPP44 type to TDM_CODEC_RST(0x2) */
            MV_REG_WRITE (MPP_CONTROL_REG5,
                          ((MV_REG_READ (MPP_CONTROL_REG5) & 0xFFF0FFFF)  | BIT17));
        }
    }

    mvTwsiRead (0, &twsiSlave, &twsiVal, 1);
    muxVal = (twsiVal & muxValMask) | muxVal;

    if (MV_OK != mvTwsiWrite (0, &twsiSlave, &muxVal, 1))
    {
        mvOsPrintf ("Board: twsi exp out val fail\n");
        return;
    }
    DB (mvOsPrintf ("Board: twsi exp out val succeded\n"));
}
/*******************************************************************************
* mvBoardVoiceAssembleModeGet - return SLIC/DAA assembly & interrupt modes
*
* DESCRIPTION:
*
* INPUT:
*
* OUTPUT:
*       None.
*
* RETURN:
*
*******************************************************************************/

MV_VOID mvBoardVoiceAssemblyModeGet(MV_32 *assmeblyMode, MV_32 *irqMode)
{
    switch (mvBoardIdGet ())
    {
    case RD_88F6281A_ID:
        *assmeblyMode = DAISY_CHAIN_MODE;
        *irqMode = INTERRUPT_TO_TDM;
        break;
    case DB_88F6281A_BP_ID:
        *assmeblyMode = DUAL_CHIP_SELECT_MODE;
        *irqMode = INTERRUPT_TO_TDM;
        break;
    case RD_88F6192A_ID:
        *assmeblyMode = DUAL_CHIP_SELECT_MODE;
        *irqMode = INTERRUPT_TO_TDM;
        break;
    case DB_88F6192A_BP_ID:
        *assmeblyMode = DUAL_CHIP_SELECT_MODE;
        *irqMode = INTERRUPT_TO_TDM;
        break;
    default:
        *assmeblyMode = *irqMode = -1;
        mvOsPrintf ("mvBoardVoiceAssembleModeGet: TDM not supported(boardId=0x%x)\n",
                    mvBoardIdGet ());
    }
    return;
}

/*******************************************************************************
* mvBoardMppModuleTypePrint - print module detect
*
* DESCRIPTION:
*
* INPUT:
*
* OUTPUT:
*       None.
*
* RETURN:
*
*******************************************************************************/
MV_VOID mvBoardMppModuleTypePrint(MV_VOID)
{
    MV_BOARD_MPP_GROUP_CLASS devClass;
    MV_BOARD_MPP_TYPE_CLASS mppGroupType;
    MV_U32 devId;
    MV_U32 maxMppGrp = 1;

    devId = mvCtrlModelGet ();

    switch (devId)
    {
    case MV_6281_DEV_ID:
        maxMppGrp = MV_6281_MPP_MAX_MODULE;
        break;
    case MV_6192_DEV_ID:
        maxMppGrp = MV_6192_MPP_MAX_MODULE;
        break;
    case MV_6190_DEV_ID:
        maxMppGrp = MV_6190_MPP_MAX_MODULE;
        break;
    case MV_6180_DEV_ID:
        maxMppGrp = MV_6180_MPP_MAX_MODULE;
        break;
    }

    for (devClass = 0; devClass < maxMppGrp; devClass++)
    {
        mppGroupType = mvBoardMppGroupTypeGet (devClass);

        switch (mppGroupType)
        {
        case MV_BOARD_TDM:
            if (devId != MV_6190_DEV_ID)
            {
                mvOsPrintf ("Module %d is TDM\n", devClass);
            }
            break;
        case MV_BOARD_AUDIO:
            if (devId != MV_6190_DEV_ID)
            {
                mvOsPrintf ("Module %d is AUDIO\n", devClass);
            }
            break;
        case MV_BOARD_RGMII:
            if (devId != MV_6190_DEV_ID)
            {
                mvOsPrintf ("Module %d is RGMII\n", devClass);
            }
            break;
        case MV_BOARD_GMII:
            if (devId != MV_6190_DEV_ID)
            {
                mvOsPrintf ("Module %d is GMII\n", devClass);
            }
            break;
        case MV_BOARD_TS:
            if (devId != MV_6190_DEV_ID)
            {
                mvOsPrintf ("Module %d is TS\n", devClass);
            }
            break;
        default:
            break;
        }
    }
}

/* Board devices API managments */

/*******************************************************************************
* mvBoardGetDeviceNumber - Get number of device of some type on the board
*
* DESCRIPTION:
*
* INPUT:
*		devType - The device type ( Flash,RTC , etc .. )
*
* OUTPUT:
*       None.
*
* RETURN:
*       If the device is found on the board the then the functions returns the
*		number of those devices else the function returns 0
*
*
*******************************************************************************/
MV_32 mvBoardGetDevicesNumber(MV_BOARD_DEV_CLASS devClass)
{
    MV_U32  foundIndex = 0, devNum;
    MV_U32 boardId = mvBoardIdGet ();

    if (boardId >= MV_MAX_BOARD_ID)
    {
        mvOsPrintf ("%s: Board unknown.\n", __func__);
        return 0xFFFFFFFF;
    }

    for (devNum = START_DEV_CS;
         devNum < BOARD_INFO (boardId)->numBoardDeviceIf;
         devNum++)
    {
        if (BOARD_INFO (boardId)->pDevCsInfo[devNum].devClass == devClass)
        {
            foundIndex++;
        }
    }

    return foundIndex;
}

/*******************************************************************************
* mvBoardGetDeviceBaseAddr - Get base address of a device existing on the board
*
* DESCRIPTION:
*
* INPUT:
*       devIndex - The device sequential number on the board
*		devType - The device type ( Flash,RTC , etc .. )
*
* OUTPUT:
*       None.
*
* RETURN:
*       If the device is found on the board the then the functions returns the
*		Base address else the function returns 0xffffffff
*
*
*******************************************************************************/
MV_32 mvBoardGetDeviceBaseAddr(MV_32 devNum, MV_BOARD_DEV_CLASS devClass)
{
    MV_DEV_CS_INFO *devEntry;
    devEntry = boardGetDevEntry (devNum, devClass);
    if (devEntry != NULL)
    {
        return mvCpuIfTargetWinBaseLowGet (DEV_TO_TARGET (devEntry->deviceCS));
    }

    return 0xFFFFFFFF;
}

/*******************************************************************************
* mvBoardGetDeviceBusWidth - Get Bus width of a device existing on the board
*
* DESCRIPTION:
*
* INPUT:
*       devIndex - The device sequential number on the board
*		devType - The device type ( Flash,RTC , etc .. )
*
* OUTPUT:
*       None.
*
* RETURN:
*       If the device is found on the board the then the functions returns the
*		Bus width else the function returns 0xffffffff
*
*
*******************************************************************************/
MV_32 mvBoardGetDeviceBusWidth(MV_32 devNum, MV_BOARD_DEV_CLASS devClass)
{
    MV_DEV_CS_INFO *devEntry;

    devEntry = boardGetDevEntry (devNum, devClass);
    if (devEntry != NULL)
    {
        return 8;
    }

    return 0xFFFFFFFF;
}

/*******************************************************************************
* mvBoardGetDeviceWidth - Get dev width of a device existing on the board
*
* DESCRIPTION:
*
* INPUT:
*       devIndex - The device sequential number on the board
*		devType - The device type ( Flash,RTC , etc .. )
*
* OUTPUT:
*       None.
*
* RETURN:
*       If the device is found on the board the then the functions returns the
*		dev width else the function returns 0xffffffff
*
*
*******************************************************************************/
MV_32 mvBoardGetDeviceWidth(MV_32 devNum, MV_BOARD_DEV_CLASS devClass)
{
    MV_DEV_CS_INFO *devEntry;
    MV_U32 boardId = mvBoardIdGet ();

    if (boardId >= MV_MAX_BOARD_ID)
    {
        mvOsPrintf ("%s: Board unknown.\n", __func__);
        return 0xFFFFFFFF;
    }

    devEntry = boardGetDevEntry (devNum, devClass);
    if (devEntry != NULL)
    {
        return devEntry->devWidth;
    }

    return MV_ERROR;
}

/*******************************************************************************
* mvBoardGetDeviceWinSize - Get the window size of a device existing on the board
*
* DESCRIPTION:
*
* INPUT:
*       devIndex - The device sequential number on the board
*		devType - The device type ( Flash,RTC , etc .. )
*
* OUTPUT:
*       None.
*
* RETURN:
*       If the device is found on the board the then the functions returns the
*		window size else the function returns 0xffffffff
*
*
*******************************************************************************/
MV_32 mvBoardGetDeviceWinSize(MV_32 devNum, MV_BOARD_DEV_CLASS devClass)
{
    MV_DEV_CS_INFO *devEntry;
    MV_U32 boardId = mvBoardIdGet ();

    if (boardId >= MV_MAX_BOARD_ID)
    {
        mvOsPrintf ("%s: Board unknown.\n", __func__);
        return 0xFFFFFFFF;
    }

    devEntry = boardGetDevEntry (devNum, devClass);
    if (devEntry != NULL)
    {
        return mvCpuIfTargetWinSizeGet (DEV_TO_TARGET (devEntry->deviceCS));
    }

    return 0xFFFFFFFF;
}

/*******************************************************************************
* boardGetDevEntry - returns the entry pointer of a device on the board
*
* DESCRIPTION:
*
* INPUT:
*       devIndex - The device sequential number on the board
*		devType - The device type ( Flash,RTC , etc .. )
*
* OUTPUT:
*       None.
*
* RETURN:
*       If the device is found on the board the then the functions returns the
*		dev number else the function returns 0x0
*
*
*******************************************************************************/
static MV_DEV_CS_INFO *boardGetDevEntry(MV_32 devNum, MV_BOARD_DEV_CLASS devClass)
{
    MV_U32  foundIndex = 0, devIndex;
    MV_U32 boardId = mvBoardIdGet ();

    if (boardId >= MV_MAX_BOARD_ID)
    {
        mvOsPrintf ("%s: Board unknown.\n", __func__);
        return NULL;
    }

    for (devIndex = START_DEV_CS;
         devIndex < BOARD_INFO (boardId)->numBoardDeviceIf;
         devIndex++)
    {
        /* TBR */
        /*if (BOARD_INFO(boardId)->pDevCsInfo[devIndex].deviceCS ==
           MV_BOOTDEVICE_INDEX)
             continue;*/

        if (BOARD_INFO (boardId)->pDevCsInfo[devIndex].devClass == devClass)
        {
            if (foundIndex == devNum)
            {
                return &(BOARD_INFO (boardId)->pDevCsInfo[devIndex]);
            }
            foundIndex++;
        }
    }

    /* device not found */
    return NULL;
}

/* Get device CS number */

MV_U32 boardGetDevCSNum(MV_32 devNum, MV_BOARD_DEV_CLASS devClass)
{
    MV_DEV_CS_INFO *devEntry;
    MV_U32 boardId = mvBoardIdGet ();

    if (boardId >= MV_MAX_BOARD_ID)
    {
        mvOsPrintf ("%s: Board unknown.\n", __func__);
        return 0xFFFFFFFF;
    }

    devEntry = boardGetDevEntry (devNum, devClass);
    if (devEntry != NULL)
    {
        return devEntry->deviceCS;
    }

    return 0xFFFFFFFF;
}

/*******************************************************************************
* mvBoardRtcTwsiAddrTypeGet -
*
* DESCRIPTION:
*
* INPUT:
*
* OUTPUT:
*       None.
*
* RETURN:
*
*
*******************************************************************************/
MV_U8 mvBoardRtcTwsiAddrTypeGet()
{
    int i;
    MV_U32 boardId = mvBoardIdGet ();

    for (i = 0; i < BOARD_INFO (boardId)->numBoardTwsiDev; i++)
    {
        if (BOARD_INFO (boardId)->pBoardTwsiDev[i].devClass == BOARD_TWSI_RTC)
        {
            return BOARD_INFO (boardId)->pBoardTwsiDev[i].twsiDevAddrType;
        }
    }
    return MV_ERROR;
}

/*******************************************************************************
* mvBoardRtcTwsiAddrGet -
*
* DESCRIPTION:
*
* INPUT:
*
* OUTPUT:
*       None.
*
* RETURN:
*
*
*******************************************************************************/
MV_U8 mvBoardRtcTwsiAddrGet()
{
    int i;
    MV_U32 boardId = mvBoardIdGet ();

    for (i = 0; i < BOARD_INFO (boardId)->numBoardTwsiDev; i++)
    {
        if (BOARD_INFO (boardId)->pBoardTwsiDev[i].devClass == BOARD_TWSI_RTC)
        {
            return BOARD_INFO (boardId)->pBoardTwsiDev[i].twsiDevAddr;
        }
    }
    return 0xFF;
}

/*******************************************************************************
* mvBoardA2DTwsiAddrTypeGet -
*
* DESCRIPTION:
*
* INPUT:
*
* OUTPUT:
*       None.
*
* RETURN:
*
*
*******************************************************************************/
MV_U8 mvBoardA2DTwsiAddrTypeGet()
{
    int i;
    MV_U32 boardId = mvBoardIdGet ();

    for (i = 0; i < BOARD_INFO (boardId)->numBoardTwsiDev; i++)
    {
        if (BOARD_INFO (boardId)->pBoardTwsiDev[i].devClass == BOARD_TWSI_AUDIO_DEC)
        {
            return BOARD_INFO (boardId)->pBoardTwsiDev[i].twsiDevAddrType;
        }
    }
    return MV_ERROR;
}

/*******************************************************************************
* mvBoardA2DTwsiAddrGet -
*
* DESCRIPTION:
*
* INPUT:
*
* OUTPUT:
*       None.
*
* RETURN:
*
*
*******************************************************************************/
MV_U8 mvBoardA2DTwsiAddrGet()
{
    int i;
    MV_U32 boardId = mvBoardIdGet ();

    for (i = 0; i < BOARD_INFO (boardId)->numBoardTwsiDev; i++)
    {
        if (BOARD_INFO (boardId)->pBoardTwsiDev[i].devClass == BOARD_TWSI_AUDIO_DEC)
        {
            return BOARD_INFO (boardId)->pBoardTwsiDev[i].twsiDevAddr;
        }
    }
    return 0xFF;
}

/*******************************************************************************
* mvBoardTwsiExpAddrTypeGet -
*
* DESCRIPTION:
*
* INPUT:
*
* OUTPUT:
*       None.
*
* RETURN:
*
*
*******************************************************************************/
MV_U8 mvBoardTwsiExpAddrTypeGet(MV_U32 index)
{
    int i;
    MV_U32 indexFound = 0;
    MV_U32 boardId = mvBoardIdGet ();

    for (i = 0; i < BOARD_INFO (boardId)->numBoardTwsiDev; i++)
    {
        if (BOARD_INFO (boardId)->pBoardTwsiDev[i].devClass == BOARD_DEV_TWSI_EXP)
        {
            if (indexFound == index)
            {
                return BOARD_INFO (boardId)->pBoardTwsiDev[i].twsiDevAddrType;
            }
            else
            {
                indexFound++;
            }
        }
    }

    return MV_ERROR;
}

/*******************************************************************************
* mvBoardTwsiExpAddrGet -
*
* DESCRIPTION:
*
* INPUT:
*
* OUTPUT:
*       None.
*
* RETURN:
*
*
*******************************************************************************/
MV_U8 mvBoardTwsiExpAddrGet(MV_U32 index)
{
    int i;
    MV_U32 indexFound = 0;
    MV_U32 boardId = mvBoardIdGet ();

    for (i = 0; i < BOARD_INFO (boardId)->numBoardTwsiDev; i++)
    {
        if (BOARD_INFO (boardId)->pBoardTwsiDev[i].devClass == BOARD_DEV_TWSI_EXP)
        {
            if (indexFound == index)
            {
                return BOARD_INFO (boardId)->pBoardTwsiDev[i].twsiDevAddr;
            }
            else
            {
                indexFound++;
            }
        }
    }

    return 0xFF;
}

/*******************************************************************************
* mvBoardTwsiSatRAddrTypeGet -
*
* DESCRIPTION:
*
* INPUT:
*
* OUTPUT:
*       None.
*
* RETURN:
*
*
*******************************************************************************/
MV_U8 mvBoardTwsiSatRAddrTypeGet(MV_U32 index)
{
    int i;
    MV_U32 indexFound = 0;
    MV_U32 boardId = mvBoardIdGet ();

    for (i = 0; i < BOARD_INFO (boardId)->numBoardTwsiDev; i++)
    {
        if (BOARD_INFO (boardId)->pBoardTwsiDev[i].devClass == BOARD_DEV_TWSI_SATR)
        {
            if (indexFound == index)
            {
                return BOARD_INFO (boardId)->pBoardTwsiDev[i].twsiDevAddrType;
            }
            else
            {
                indexFound++;
            }
        }
    }

    return MV_ERROR;
}

/*******************************************************************************
* mvBoardTwsiSatRAddrGet -
*
* DESCRIPTION:
*
* INPUT:
*
* OUTPUT:
*       None.
*
* RETURN:
*
*
*******************************************************************************/
MV_U8 mvBoardTwsiSatRAddrGet(MV_U32 index)
{
    int i;
    MV_U32 indexFound = 0;
    MV_U32 boardId = mvBoardIdGet ();

    for (i = 0; i < BOARD_INFO (boardId)->numBoardTwsiDev; i++)
    {
        if (BOARD_INFO (boardId)->pBoardTwsiDev[i].devClass == BOARD_DEV_TWSI_SATR)
        {
            if (indexFound == index)
            {
                return BOARD_INFO (boardId)->pBoardTwsiDev[i].twsiDevAddr;
            }
            else
            {
                indexFound++;
            }
        }
    }

    return 0xFF;
}

/*******************************************************************************
* mvBoardNandWidthGet -
*
* DESCRIPTION: Get the width of the first NAND device in byte.
*
* INPUT:
*
* OUTPUT:
*       None.
*
* RETURN: 1, 2, 4 or MV_ERROR
*
*
*******************************************************************************/
/*  */
MV_32 mvBoardNandWidthGet(void)
{
    MV_U32 devNum;
    MV_U32 devWidth;
    MV_U32 boardId = mvBoardIdGet ();

    for (devNum = START_DEV_CS;
         devNum < BOARD_INFO (boardId)->numBoardDeviceIf;
         devNum++)
    {
        devWidth = mvBoardGetDeviceWidth (devNum, BOARD_DEV_NAND_FLASH);
        if (devWidth != MV_ERROR)
        {
            return devWidth / 8;
        }
    }

    /* NAND wasn't found */
    return MV_ERROR;
}

MV_U32 gBoardId = -1;

/*******************************************************************************
* mvBoardIdGet - Get Board model
*
* DESCRIPTION:
*       This function returns board ID.
*       Board ID is 32bit word constructed of board model (16bit) and
*       board revision (16bit) in the following way: 0xMMMMRRRR.
*
* INPUT:
*       None.
*
* OUTPUT:
*       None.
*
* RETURN:
*       32bit board ID number, '-1' if board is undefined.
*
*******************************************************************************/
MV_U32 mvBoardIdGet(MV_VOID)
{
    MV_U32 tmpBoardId = -1;
    MV_U16 reg;

    if (gBoardId == -1)
    {
        #if defined(DB_88F6281A)
        tmpBoardId = DB_88F6281A_BP_ID;
        #elif defined(RD_88F6281A)
        tmpBoardId = RD_88F6281A_ID;
        #elif defined(DB_88F6192A)
        tmpBoardId = DB_88F6192A_BP_ID;
        #elif defined(DB_88F6190A)
        tmpBoardId = DB_88F6190A_BP_ID;
        #elif defined(RD_88F6192A)
        tmpBoardId = RD_88F6192A_ID;
        #elif defined(RD_88F6190A)
        tmpBoardId = RD_88F6190A_ID;
        #elif defined(DB_88F6180A)
        tmpBoardId = DB_88F6180A_BP_ID;
        #elif defined(RD_88F6281A_PCAC)
        tmpBoardId = RD_88F6281A_PCAC_ID;
        #elif defined(DB_CUSTOMER)
        tmpBoardId = DB_CUSTOMER_ID;
#elif defined(RD_98DX3121)
        tmpBoardId = RD_98DX3121_ID;
#elif defined(DB_98DX4122)
        tmpBoardId = DB_98DX4122_ID;
        #endif
        gBoardId = tmpBoardId;

        /*
            Detecting actual board:	db98dx4122 or rd98dx3121
            ------------------------------------------------

            boardEthSmiAddr is 0x8 on db98dx4122, and 0xb on db98dx4122.
            We perform mvEthPhyRegRead() to detect the board.

            Giora
         */

        if (gBoardId == RD_98DX3121_ID)
        {
            mvEthPhyRegRead (0x8, 0, &reg);

            if (reg != 0xffff)
            {
                /* this is dx4122 */

                db_98dx4122_detected = 1;
                gBoardId = DB_98DX4122_ID; /* replace the board_id */

                /* Do not fix diferrences in dramregs for
                   dramregs_db98dx4122_h200.txt
                   Because it's now too late. We are going to stick with 64MB
                   unfortunately */
            }
        }
        else
        {
            /* Decide if it is GE or FE board */
            if (_G_xCatIsFEBoardType == 1)
            {
                gBoardId = DB_98DX2122_ID;
            }
            else
            {
                gBoardId = DB_98DX4122_ID;
            }
        }
    }

    return gBoardId;
}

/*******************************************************************************
* mvBoarModuleTypeGet - mvBoarModuleTypeGet
*
* DESCRIPTION:
*
* INPUT:
*		group num - MV_BOARD_MPP_GROUP_CLASS enum
*
* OUTPUT:
*		None.
*
* RETURN:
*		module num - MV_BOARD_MODULE_CLASS enum
*
*******************************************************************************/
MV_BOARD_MODULE_ID_CLASS mvBoarModuleTypeGet(MV_BOARD_MPP_GROUP_CLASS devClass)
{
    MV_TWSI_SLAVE twsiSlave;
    MV_TWSI_ADDR slave;
    MV_U8 data;

    /* TWSI init */
    slave.type = ADDR7_BIT;
    slave.address = 0;
    mvTwsiInit (0, TWSI_SPEED, mvBoardTclkGet (), &slave, 0);

    /* Read MPP module ID */
    DB (mvOsPrintf ("Board: Read MPP module ID\n"));
    twsiSlave.slaveAddr.address = mvBoardTwsiExpAddrGet (devClass);
    twsiSlave.slaveAddr.type = mvBoardTwsiExpAddrTypeGet (devClass);
    twsiSlave.validOffset = MV_TRUE;
    /* Offset is the first command after the address which indicate the register
       number to be read
       in next operation */
    twsiSlave.offset = 0;
    twsiSlave.moreThen256 = MV_FALSE;

    if (MV_OK != mvTwsiRead (0, &twsiSlave, &data, 1))
    {
        DB (mvOsPrintf ("Board: Read MPP module ID fail\n"));
        return MV_ERROR;
    }
    DB (mvOsPrintf ("Board: Read MPP module ID succeded\n"));

    return data;
}

/*******************************************************************************
* mvBoarTwsiSatRGet -
*
* DESCRIPTION:
*
* INPUT:
*		device num - one of three devices
*		reg num - 0 or 1
*
* OUTPUT:
*		None.
*
* RETURN:
*		reg value
*
*******************************************************************************/
MV_U8 mvBoarTwsiSatRGet(MV_U8 devNum, MV_U8 regNum)
{
    MV_TWSI_SLAVE twsiSlave;
    MV_TWSI_ADDR slave;
    MV_U8 data;

    /* TWSI init */
    slave.type = ADDR7_BIT;
    slave.address = 0;
    mvTwsiInit (0, TWSI_SPEED, mvBoardTclkGet (), &slave, 0);

    /* Read MPP module ID */
    DB (mvOsPrintf ("Board: Read S@R device read\n"));
    twsiSlave.slaveAddr.address = mvBoardTwsiSatRAddrGet (devNum);
    twsiSlave.slaveAddr.type = mvBoardTwsiSatRAddrTypeGet (devNum);
    twsiSlave.validOffset = MV_TRUE;
    /* Use offset as command */
    twsiSlave.offset = regNum;
    twsiSlave.moreThen256 = MV_FALSE;

    if (MV_OK != mvTwsiRead (0, &twsiSlave, &data, 1))
    {
        DB (mvOsPrintf ("Board: Read S@R fail\n"));
        return MV_ERROR;
    }
    DB (mvOsPrintf ("Board: Read S@R succeded\n"));

    return data;
}

/*******************************************************************************
* mvBoarTwsiSatRSet -
*
* DESCRIPTION:
*
* INPUT:
*		devNum - one of three devices
*		regNum - 0 or 1
*		regVal - value
*
*
* OUTPUT:
*		None.
*
* RETURN:
*		reg value
*
*******************************************************************************/
MV_STATUS mvBoarTwsiSatRSet(MV_U8 devNum, MV_U8 regNum, MV_U8 regVal)
{
    MV_TWSI_SLAVE twsiSlave;
    MV_TWSI_ADDR slave;

    /* TWSI init */
    slave.type = ADDR7_BIT;
    slave.address = 0;
    mvTwsiInit (0, TWSI_SPEED, mvBoardTclkGet (), &slave, 0);

    /* Read MPP module ID */
    twsiSlave.slaveAddr.address = mvBoardTwsiSatRAddrGet (devNum);
    twsiSlave.slaveAddr.type = mvBoardTwsiSatRAddrTypeGet (devNum);
    twsiSlave.validOffset = MV_TRUE;
    DB (mvOsPrintf ("Board: Write S@R device addr %x, type %x, data %x\n",
                    twsiSlave.slaveAddr.address, \
                    twsiSlave.slaveAddr.type, regVal));
    /* Use offset as command */
    twsiSlave.offset = regNum;
    twsiSlave.moreThen256 = MV_FALSE;
    if (MV_OK != mvTwsiWrite (0, &twsiSlave, &regVal, 1))
    {
        DB (mvOsPrintf ("Board: Write S@R fail\n"));
        return MV_ERROR;
    }
    DB (mvOsPrintf ("Board: Write S@R succeded\n"));

    return MV_OK;
}

/*******************************************************************************
* mvBoardSlicGpioPinGet -
*
* DESCRIPTION:
*
* INPUT:
*
* OUTPUT:
*       None.
*
* RETURN:
*
*
*******************************************************************************/
MV_32 mvBoardSlicGpioPinGet(MV_U32 slicNum)
{
    MV_U32 boardId;
    boardId = mvBoardIdGet ();

    switch (boardId)
    {
    case DB_88F6281A_BP_ID:
    case RD_88F6281A_ID:
    default:
        return MV_ERROR;

        break;
    }
}

/*******************************************************************************
* mvBoardFanPowerControl - Turn on/off the fan power control on the RD-6281A
*
* DESCRIPTION:
*
* INPUT:
*        mode - MV_TRUE = on ; MV_FALSE = off
*
* OUTPUT:
*       MV_STATUS - MV_OK , MV_ERROR.
*
* RETURN:
*
*******************************************************************************/
MV_STATUS mvBoardFanPowerControl(MV_BOOL mode)
{
    MV_U8 val = 1, twsiVal;
    MV_TWSI_SLAVE twsiSlave;
    MV_TWSI_ADDR slave;

    if (mvBoardIdGet () != RD_88F6281A_ID)
    {
        return MV_ERROR;
    }

    /* TWSI init */
    slave.type = ADDR7_BIT;
    slave.address = 0;
    mvTwsiInit (0, TWSI_SPEED, mvBoardTclkGet (), &slave, 0);

    /* Read MPP module ID */
    DB (mvOsPrintf ("Board: twsi exp set\n"));
    twsiSlave.slaveAddr.address = mvBoardTwsiExpAddrGet (0);
    twsiSlave.slaveAddr.type = ADDR7_BIT;
    twsiSlave.validOffset = MV_TRUE;
    /* Offset is the first command after the address which indicate the register
       number to be read
       in next operation */
    twsiSlave.offset = 3;
    twsiSlave.moreThen256 = MV_FALSE;
    if (mode == MV_TRUE)
    {
        val = 0x1;
    }
    else
    {
        val = 0;
    }
    mvTwsiRead (0, &twsiSlave, &twsiVal, 1);
    val = (twsiVal & 0xfe) | val;
    if (MV_OK != mvTwsiWrite (0, &twsiSlave, &val, 1))
    {
        DB (mvOsPrintf ("Board: twsi exp out val fail\n"));
        return MV_ERROR;
    }
    DB (mvOsPrintf ("Board: twsi exp out val succeded\n"));

    /* Change twsi exp to output */
    twsiSlave.offset = 7;
    mvTwsiRead (0, &twsiSlave, &twsiVal, 1);
    val = (twsiVal & 0xfe);
    if (MV_OK != mvTwsiWrite (0, &twsiSlave, &val, 1))
    {
        DB (mvOsPrintf ("Board: twsi exp change to out fail\n"));
        return MV_ERROR;
    }
    DB (mvOsPrintf ("Board: twsi exp change to out succeded\n"));
    return MV_OK;
}

/*******************************************************************************
* mvBoardHDDPowerControl - Turn on/off the HDD power control on the RD-6281A
*
* DESCRIPTION:
*
* INPUT:
*        mode - MV_TRUE = on ; MV_FALSE = off
*
* OUTPUT:
*       MV_STATUS - MV_OK , MV_ERROR.
*
* RETURN:
*
*******************************************************************************/
MV_STATUS mvBoardHDDPowerControl(MV_BOOL mode)
{
    MV_U8 val = 1, twsiVal;
    MV_TWSI_SLAVE twsiSlave;
    MV_TWSI_ADDR slave;

    if (mvBoardIdGet () != RD_88F6281A_ID)
    {
        return MV_ERROR;
    }

    /* TWSI init */
    slave.type = ADDR7_BIT;
    slave.address = 0;
    mvTwsiInit (0, TWSI_SPEED, mvBoardTclkGet (), &slave, 0);

    /* Read MPP module ID */
    DB (mvOsPrintf ("Board: twsi exp set\n"));
    twsiSlave.slaveAddr.address = mvBoardTwsiExpAddrGet (0);
    twsiSlave.slaveAddr.type = ADDR7_BIT;
    twsiSlave.validOffset = MV_TRUE;
    /* Offset is the first command after the address which indicate the register
       number to be read
       in next operation */
    twsiSlave.offset = 3;
    twsiSlave.moreThen256 = MV_FALSE;
    if (mode == MV_TRUE)
    {
        val = 0x2;
    }
    else
    {
        val = 0;
    }
    mvTwsiRead (0, &twsiSlave, &twsiVal, 1);
    val = (twsiVal & 0xfd) | val;
    if (MV_OK != mvTwsiWrite (0, &twsiSlave, &val, 1))
    {
        DB (mvOsPrintf ("Board: twsi exp out val fail\n"));
        return MV_ERROR;
    }
    DB (mvOsPrintf ("Board: twsi exp out val succeded\n"));

    /* Change twsi exp to output */
    twsiSlave.offset = 7;
    mvTwsiRead (0, &twsiSlave, &twsiVal, 1);
    val = (twsiVal & 0xfd);
    if (MV_OK != mvTwsiWrite (0, &twsiSlave, &val, 1))
    {
        DB (mvOsPrintf ("Board: twsi exp change to out fail\n"));
        return MV_ERROR;
    }
    DB (mvOsPrintf ("Board: twsi exp change to out succeded\n"));
    return MV_OK;
}

/*******************************************************************************
* mvBoardSDioWPControl - Turn on/off the SDIO WP on the RD-6281A
*
* DESCRIPTION:
*
* INPUT:
*        mode - MV_TRUE = on ; MV_FALSE = off
*
* OUTPUT:
*       MV_STATUS - MV_OK , MV_ERROR.
*
* RETURN:
*
*******************************************************************************/
MV_STATUS mvBoardSDioWPControl(MV_BOOL mode)
{
    MV_U8 val = 1, twsiVal;
    MV_TWSI_SLAVE twsiSlave;
    MV_TWSI_ADDR slave;

    if (mvBoardIdGet () != RD_88F6281A_ID)
    {
        return MV_ERROR;
    }

    /* TWSI init */
    slave.type = ADDR7_BIT;
    slave.address = 0;
    mvTwsiInit (0, TWSI_SPEED, mvBoardTclkGet (), &slave, 0);

    /* Read MPP module ID */
    DB (mvOsPrintf ("Board: twsi exp set\n"));
    twsiSlave.slaveAddr.address = mvBoardTwsiExpAddrGet (0);
    twsiSlave.slaveAddr.type = ADDR7_BIT;
    twsiSlave.validOffset = MV_TRUE;
    /* Offset is the first command after the address which indicate the register
       number to be read
       in next operation */
    twsiSlave.offset = 3;
    twsiSlave.moreThen256 = MV_FALSE;
    if (mode == MV_TRUE)
    {
        val = 0x10;
    }
    else
    {
        val = 0;
    }
    mvTwsiRead (0, &twsiSlave, &twsiVal, 1);
    val = (twsiVal & 0xef) | val;
    if (MV_OK != mvTwsiWrite (0, &twsiSlave, &val, 1))
    {
        DB (mvOsPrintf ("Board: twsi exp out val fail\n"));
        return MV_ERROR;
    }
    DB (mvOsPrintf ("Board: twsi exp out val succeded\n"));

    /* Change twsi exp to output */
    twsiSlave.offset = 7;
    mvTwsiRead (0, &twsiSlave, &twsiVal, 1);
    val = (twsiVal & 0xef);
    if (MV_OK != mvTwsiWrite (0, &twsiSlave, &val, 1))
    {
        DB (mvOsPrintf ("Board: twsi exp change to out fail\n"));
        return MV_ERROR;
    }
    DB (mvOsPrintf ("Board: twsi exp change to out succeded\n"));
    return MV_OK;
}

/*******************************************************************************
 * mvBoardSwitchSetChipType - sets FE/GE chip type by reading deviceId register
 *
 * DESCRIPTION:
 *   If bit 5 of deviceId resister if 1, then the Prestera chip is FE.
 *   Otherwise, it is GE.
 *   4 LSB bits of deviceId register designate chip revision:
 *       if >= 3 then xCat-A2 chip revision
 *       if <3   then xCat-A1 chip revision
 *
 * RETURN:
 *   MV_OK   - configuration succeeded.
 *   MV_FAIL - configuration failed.
 *
 */
MV_STATUS mvBoardSwitchSetChipType(void)
{
    MV_U32 devIdReg = 0;

    /* Read deviceId register */
    devIdReg = *(MV_U32*)(PP_DEV0_BASE + 0x4C);
    devIdReg = MV_32BIT_LE(devIdReg);

    _G_xCatIsFEBoardType   = devIdReg;
    _G_xCatIsFEBoardType  &= 0x10;
    _G_xCatIsFEBoardType >>= 4;

    _G_xCatRevision      = (devIdReg & 0xF);

    return MV_OK;
}

/*******************************************************************************
 * mvBoardSwitchPortsInitFuncGet
 */
MV_SWITCH_GEN_HOOK_HW_INIT_PORTS mvBoardSwitchPortsInitFuncGet(void)
{
    MV_SWITCH_GEN_HOOK_HW_INIT_PORTS fP = NULL;

    if (mvPpChipIsXCat2() == MV_TRUE)
    {
        if (mvPpIsChipGE() == MV_TRUE)
        {
            if (mvChipXCat2IsBGA() == MV_TRUE)
            {
                if (mvBoardChipRevGet() == 0)
                {
                    fP = (MV_SWITCH_GEN_HOOK_HW_INIT_PORTS)simulate_PP_EEPROM_DB_xCat2A0_BGA_24GE_4GP;
                }
                else
                {
                    fP = (MV_SWITCH_GEN_HOOK_HW_INIT_PORTS)mvPpEeprom_DB_XCat2A1_24GE_4GP;
                }
            }
            else
            {
                if (mvBoardChipRevGet() == 0)
                {
                    fP = (MV_SWITCH_GEN_HOOK_HW_INIT_PORTS)simulate_PP_EEPROM_DB_xCat2A0_QFP_24GE_2SFP;
                }
                else
                {
                    fP = (MV_SWITCH_GEN_HOOK_HW_INIT_PORTS)simulate_PP_EEPROM_DB_xCat2A1_QFP_24GE_2SFP;
                }
            }
        }
        else
        {
            /*
             * xCat2 FE case.
             */
            if (mvChipXCat2IsBGA() == MV_TRUE)
            {
                fP = (MV_SWITCH_GEN_HOOK_HW_INIT_PORTS)mvPpEeprom_DB_XCat2A1_48FE_4GP;
            }
            else
            {
                fP = (MV_SWITCH_GEN_HOOK_HW_INIT_PORTS)mvPpEeprom_DB_XCat2A1_QFP_24FE_4GP;
            }
        }
    }
    else
    {
        if (mvPpIsChipFE() == MV_TRUE)
        {
            fP = (MV_SWITCH_GEN_HOOK_HW_INIT_PORTS)simulate_PP_EEPROM_2122_24FE;
        }
        else
        {
            fP = (MV_SWITCH_GEN_HOOK_HW_INIT_PORTS)simulate_PP_EEPROM_4122_24GE_PHY_A0;
        }
    }

    return fP;
}

static MV_CHIP_FEATURES mvChipFeaturesXCat =
{
    /* .numOfUsb          = */ MV_USB_MAX_PORTS,
    /* .numOfSpiCS        = */ 1,
    /* .numOfGbe          = */ 2,
    /* .numOfTdm          = */ 0,
    /* .numOfDragonite    = */ 1,
    /* .numOfPex          = */ MV_PEX_MAX_IF,
    /* .miiGbeIndex       = */ 1,
    /* .oobGbeIndex       = */ 0,
    /* .sramSize          = */ _2K
};

static MV_CHIP_FEATURES mvChipFeaturesXCat2BGA =
{
    /* .numOfUsb          = */ 0,
    /* .numOfSpiCS        = */ 4,
    /* .numOfGbe          = */ 1,
    /* .numOfTdm          = */ 1,
    /* .numOfDragonite    = */ 0,
    /* .numOfPex          = */ 1,
    /* .miiGbeIndex       = */ 0,
    /* .oobGbeIndex       = */ 0,
    /* .sramSize          = */ _32K
};

static MV_CHIP_FEATURES mvChipFeaturesXCat2QFP =
{
    /* .numOfUsb          = */ 0,
    /* .numOfSpiCS        = */ 4,
    /* .numOfGbe          = */ 1,
    /* .numOfTdm          = */ 1,
    /* .numOfDragonite    = */ 0,
    /* .numOfPex          = */ 0,
    /* .miiGbeIndex       = */ 0,
    /* .oobGbeIndex       = */ 0,
    /* .sramSize          = */ _32K
};

/*******************************************************************************
* mvChipFeatures
*
* DESCRIPTION:
*       Represents all possible feature set for all supported chips.
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
*       This array should be synchronized to the MV_CHIP_TYPE enum.
*
*******************************************************************************/
static MV_CHIP_FEATURES *mvChipFeatures[MV_CHIP_TYPE_LAST] =
{
    &mvChipFeaturesXCat,
    &mvChipFeaturesXCat2BGA,
    &mvChipFeaturesXCat2QFP
};

/*******************************************************************************
* mvChipFeaturesGet
*
* DESCRIPTION:
*       Prints all the supported features of the chip (e.g. the number of
*       GbE controllers, USB controllers etc.).
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
MV_CHIP_FEATURES *mvChipFeaturesGet(void)
{
    static MV_CHIP_TYPE chipType = MV_CHIP_TYPE_LAST;

    if (chipType == MV_CHIP_TYPE_LAST)
    {
        if (mvPpChipIsXCat2() == MV_TRUE)
        {
            if (mvChipXCat2IsBGA() == MV_TRUE)
            {
                chipType = MV_CHIP_TYPE_XCAT2_BGA;
            }
            else
            {
                chipType = MV_CHIP_TYPE_XCAT2_QFP;
            }
        }
        else
        {
            chipType = MV_CHIP_TYPE_XCAT;
        }
    }

    return mvChipFeatures[chipType];
}

/*******************************************************************************
* mvChipFeaturesPrint
*
* DESCRIPTION:
*       Prints all the supported features of the chip (e.g. the number of
*       GbE controllers, USB controllers etc.).
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
void mvChipFeaturesPrint(void)
{
    MV_CHIP_FEATURES *featuresP = mvChipFeaturesGet();

    mvOsPrintf("\n");
    mvOsPrintf("Device ID is 0x%08X.\n", mvPpGetDevId());

    if (mvPpChipIsXCat2() == MV_TRUE)
    {
        MV_U32 devIdStat = mvPpReadReg(0, 0x8f8240);

        mvOsPrintf("The chip type is xCat2 (rev=%d, %s).\n",
                   mvBoardChipRevGet(),
                   mvPpIsChipFE() == MV_TRUE ? "FE" : "GE");
        mvOsPrintf("DeviceId status (0x8f8240) is 0x%08X.\n", devIdStat);

        if (mvChipXCat2IsBGA() == MV_TRUE)
        {
            mvOsPrintf("The chip package is BGA.\n");
        }
        else
        {
            mvOsPrintf("The chip package is QFP.\n");
        }
    }
    else
    {
        mvOsPrintf("The chip type is xCat (rev=%d, %s).\n",
                   mvBoardChipRevGet(),
                   mvPpIsChipFE() == MV_TRUE ? "FE" : "GE");
    }

    mvOsPrintf("Printing the feature set of the board:\n");
    mvOsPrintf("numOfUsb          = %d.\n", featuresP->numOfUsb);
    mvOsPrintf("numOfSpiCS        = %d.\n", featuresP->numOfSpiCS);
    mvOsPrintf("numOfGbe          = %d.\n", featuresP->numOfGbe);
    mvOsPrintf("numOfTdm          = %d.\n", featuresP->numOfTdm);
    mvOsPrintf("numOfDragonite    = %d.\n", featuresP->numOfDragonite);
    mvOsPrintf("numOfPex          = %d.\n", featuresP->numOfPex);
    mvOsPrintf("miiGbeIndex       = %d.\n", featuresP->miiGbeIndex);
    mvOsPrintf("oooGbeIndex       = %d.\n", featuresP->oobGbeIndex);
    mvOsPrintf("sramSize          = 0x%X bytes.\n", featuresP->sramSize);
}

/*******************************************************************************
* mvChipXCat2IsBGA
*
* DESCRIPTION:
*       Checks if the xCat2 package type is BGA or QFP.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_TRUE     - if xCat2 package type is BGA.
*       MV_FALSE    - if xCat2 package type is QFP.
*
* COMMENTS:
*       This function should be called only for xCat2 chip (not xCat).
*
*******************************************************************************/
MV_BOOL mvChipXCat2IsBGA(void)
{
    MV_U32 sarReg = MV_REG_READ (MPP_SAMPLE_AT_RESET);

    sarReg &= BIT23;

    if (sarReg == 0)
    {
        /* QFP package */
        return MV_FALSE;
    }

    /* BGA package */
    return MV_TRUE;
}

/*******************************************************************************
* mvChipXCat2IsQFP
*
* DESCRIPTION:
*       Checks if the xCat2 package type is BGA or QFP.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_FALSE    - if xCat2 package type is BGA.
*       MV_TRUE     - if xCat2 package type is QFP.
*
* COMMENTS:
*       This function should be called only for xCat2 chip (not xCat).
*
*******************************************************************************/
MV_BOOL mvChipXCat2IsQFP(void)
{
    return !mvChipXCat2IsBGA();
}

/*******************************************************************************
* mvBoardChipRevGet
*
* DESCRIPTION:
*       Returns switch chip revision.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       Returns switch chip revision.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_U32 mvBoardChipRevGet(void)
{
    MV_U32 rev = mvPpGetChipRev();
    return rev;
}

/*******************************************************************************
* mvBoardChipIsXCat2
*
* DESCRIPTION:
*       Answers if the chip is xCat2 or xCat.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_TRUE     - if the chip is xCat2.
*       MV_FALSE    - otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_BOOL mvBoardChipIsXCat2(void)
{
    return mvPpChipIsXCat2();
}

/*******************************************************************************
* mvBoardChipIsXCat
*
* DESCRIPTION:
*       Answers if the chip is xCat2 or xCat.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_TRUE     - if the chip is xCat.
*       MV_FALSE    - otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_BOOL mvBoardChipIsXCat(void)
{
    return mvPpChipIsXCat();
}

