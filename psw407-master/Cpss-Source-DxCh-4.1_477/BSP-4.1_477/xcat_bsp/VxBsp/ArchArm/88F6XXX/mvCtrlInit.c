/*******************************************************************************
*                Copyright 2004, MARVELL SEMICONDUCTOR, LTD.                   *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL.                      *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
*                                                                              *
* MARVELL COMPRISES MARVELL TECHNOLOGY GROUP LTD. (MTGL) AND ITS SUBSIDIARIES, *
* MARVELL INTERNATIONAL LTD. (MIL), MARVELL TECHNOLOGY, INC. (MTI), MARVELL    *
* SEMICONDUCTOR, INC. (MSI), MARVELL ASIA PTE LTD. (MAPL), MARVELL JAPAN K.K.  *
* (MJKK), MARVELL ISRAEL LTD. (MSIL).                                          *
*******************************************************************************/
/*******************************************************************************
* mvCtrlInit.c - System Controller Initialization library file
*
* DESCRIPTION:
*       None.
*
* DEPENDENCIES:
*       None.
*
*******************************************************************************/

/* includes */
#include "VxWorks.h"
#include <stdlib.h>
#include "config.h"
#include "sysLib.h"
#include "mvCtrlEnvLib.h"
#include "mvDramIf.h"
#include "mvCpu.h"
#include "mvCpuIf.h"
#include "mvDevice.h"
#include "mvXor.h"
#include "mvSysXor.h"
#include "mvEth.h"
#include "mvEthPhy.h"
#include "mvSysGbe.h"
#include "sysmap.h"
#include "end.h"
#include "mvPrestera.h"
#include <ctype.h>
#include "mvCtrlEnvLib.h"
#include "mvBoardErrata.h"

#ifdef __cplusplus
extern "C" {
#endif

/* defines */

/* extern */
extern MV_U32  G_switchEndPpIface;
extern MV_BOOL G_switchEndIsTxSync;
IMPORT long sysL2PrefetchFlag;  /* Enable/disable L2 prefetch in runtime    */

/* typedefs */

/* locals   */
int ipipsec_conf_enable_outfilter = 1;
int ipipsec_conf_enable_infilter = 1;
int ipipsec_conf_esp_enable = 1;
int ipipsec_conf_ah_enable = 1;
int ipipsec_conf_ipip_allow = 1;
#if !defined(INCLUDE_USER_APPL)

int* ipcom_shell_find_cmd(const char *name)
{
    return NULL;
}
#endif

extern END_OBJ* mgiEndLoad   (char *, void*);
extern END_OBJ* switchEndLoad(char *, void*);
extern BOOT_PARAMS	bootParams;		/* parameters from boot line */
IMPORT END_TBL_ENTRY    endDevTbl[];    /* end device table */
MV_STATUS mvSwitchOpenMemWinAsKirkwood(MV_U8 devNum);

#ifdef BSP_MODE
MV_STATUS simulate_PP_EEPROM_4122_24GE_PHY_X0(void);
MV_STATUS simulate_PP_EEPROM_2122_24FE(void);
#endif

MV_VOID mvXorAllStop(MV_VOID);
MV_STATUS mvSysFlashInit(void);

/*******************************************************************************
* mvSwitchGenBootCfg - configuration needed by the upper software layers
*
* DESCRIPTION:
*   Performs the Prestera configuration needed by the upper software layers.
*   Initializes memory windows and switch PHYs (needed when working without
*   PP-EEPROM).
*
* RETURN:
*   MV_OK   - configuration succeeded.
*   MV_FAIL - configuration failed.
*
*******************************************************************************/
MV_STATUS mvSwitchGenBootCfg()
{
    MV_STATUS status = MV_OK;
    MV_U8     dev;

    /* Open memory windows from Prestera to DRAM */
#ifdef MV_PP_TO_DRAM_WIN_CFG
    for (dev = 0; dev < mvSwitchGetDevicesNum(); dev++)
    {
        mvSwitchOpenMemWinAsKirkwood(dev);
    }
    mvOsPrintf("Memory window(s) from switch to DRAM/PEX ... Done.\n");

#endif

    return status;
}

/*******************************************************************************
* isValidBootLine - Check boot line.
*
* DESCRIPTION:
*
* RETURN:
*       true - valid boot line.
*
*******************************************************************************/
MV_BOOL isValidBootLine(char *pBootLine)
{
    if ( (strncmp(pBootLine, "mgi",    3) == 0) ||
         (strncmp(pBootLine, "switch", 5) == 0) )
    {
        return MV_TRUE; /* valid boot line */
    }

    return MV_FALSE;
}

/*******************************************************************************
* mvCtrlInit - MV Initialization routine.
*
* DESCRIPTION:
*       This routine aborts the MV major facilities activities and masks all
*       interrupts in order to ensure quiet state in early boot stages.
*       The following facilities are stopped:
*       watchdog, timer/counter, DMA engines
*
*       This routine also makes the following initialization:
*       1) In case the MV internal PCI arbiter is included, enable it.
*       2) Establish the SRAM size on board and adjust the PCI BARs according
*          to it. Any other MV initialization which is called in the sysHwInit
*          phase can be located here.
*		Note that in this point DEV_CS[1] and DEV_CS[2] base addresses are
*		already initialized to allow debug leds and early UART operation
*
* INPUT:
*       None.
*
* OUTPUT:
*       See description.
*
* RETURN:
*       None.
*
*******************************************************************************/
MV_STATUS mvCtrlInit(MV_VOID)
{
    MV_CHIP_FEATURES *setP = mvChipFeaturesGet();
    int           port;
    MV_U32        twsiActualFreq;
    MV_TWSI_ADDR  twsiAddr;

#ifdef INCLUDE_SWITCH_END
    MV_U32        bootLineFlag = 0;
    MV_U32        i;
#endif

    twsiAddr.address = MV_BOARD_CTRL_I2C_ADDR;
    twsiAddr.type = ADDR7_BIT;

    if (mvCtrlEnvInit() != MV_OK)
    {
        mvOsPrintf("Controller env init failed!!\n");
        return MV_ERROR;
    }
    mvOsPrintf("Ctrl env init... Done.\n");

    /* Init CPU interface. This will set CPU to periferal target address    */
    /* windows as defined by system HW configuration file. This will also   */
    /* Enable memory auto detection as it moves all other target windows    */
    /* up in address space making room for DRAM                             */
    /* NOTE! It is assumed that this code is executed out of DRAM bank 0!!  */
    if (mvCpuIfInit(mv_sys_map()) != MV_OK)
    {
        mvOsPrintf("%s: CPU if init failed.\n", __func__);
        return MV_ERROR;
    }
    mvOsPrintf("CPU if init..... Done.\n");

    if (mvBoardErrataInit() != MV_OK)
    {
        mvOsPrintf("%s: mvBoardErrataInit failed.\n", __func__);
        return MV_ERROR;
    }

    /* After controller envionment initialization (set MPPs) is done we  */
    /* can activate internal PCI arbiter to allow access into the system */
    /* via the PCI controller */

    /* Initialize TWSI interface in order to access DIMMs */
    mvOsPrintf("TWSI init Channel %d....... ", MV_BOARD_DIMM_I2C_CHANNEL);
    twsiActualFreq = mvTwsiInit(0, (MV_U32)100000, mvBoardTclkGet(), &twsiAddr, MV_FALSE);
    mvOsPrintf("Done. Actual freq = %dHz\n", twsiActualFreq);

    /* SPI WA should be done for xCat-A1 only. */
    if (mvBoardChipIsXCat2() == MV_FALSE && mvBoardChipRevGet() == 1)
    {
        mvXorAllStop();
        mvOsPrintf("XOR channels.... stopped.\n");
    }

    if (mvSysFlashInit() != MV_OK)
    {
        mvOsPrintf("%s: mvSysFlashInit failed.\n", __func__);
        /* Continue the boot process anyway. */
    }

    if (isValidBootLine(BOOT_LINE_ADRS) == MV_FALSE)
    {
#ifdef BOOT_LINE_ON_SPI_FLASH
        if (sysNvRamGet(BOOT_LINE_ADRS, BOOT_LINE_SIZE, 0) != OK)
        {
            mvOsPrintf("%s: sysNvRamGet failed.\n", __func__);
        }
        else
        {
            /* If Flash has no valid boot line, use default. */
            if (isValidBootLine(BOOT_LINE_ADRS) == MV_FALSE)
            {
                strcpy(BOOT_LINE_ADRS, DEFAULT_BOOT_LINE);
            }
        }
#else
        strcpy(BOOT_LINE_ADRS, DEFAULT_BOOT_LINE);
#endif
    }

    bootStringToStruct(BOOT_LINE_ADRS, &bootParams);

    if (setP->numOfGbe >= 2)
    {
        mvCtrlPwrMemSet (ETH_GIG_UNIT_ID, MV_GBE_PORT1, MV_TRUE);
        mvCtrlPwrClckSet(ETH_GIG_UNIT_ID, MV_GBE_PORT1, MV_TRUE);
    }

#ifdef INCLUDE_SWITCH_END
    bootStringToStruct (BOOT_LINE_ADRS, &sysBootParams);
    if (sysBootParams.flags & MV_USE_END_SWITCH_CPU)
    {
        /* Deactivate MgiEND. */
        for(i = 0; endDevTbl[i].endLoadFunc != END_TBL_END; i++)
        {
            if (endDevTbl[i].endLoadFunc == mgiEndLoad)
            {
                mvOsPrintf("MgiEND is deactivated.\n");
                endDevTbl[i].processed = TRUE;
            }
        }
    }
    else
    {
        /* Deactivate SwitchEND. */
        for(i = 0; endDevTbl[i].endLoadFunc != END_TBL_END; i++)
        {
            if (endDevTbl[i].endLoadFunc == switchEndLoad)
            {
                mvOsPrintf("SwitchEND is deactivated.\n");
                endDevTbl[i].processed = TRUE;
            }
        }
    }

    bootLineFlag  = sysBootParams.flags;
    bootLineFlag &= 0x30000;
    if (bootLineFlag == MV_USE_END_SWITCH_SDMA)
    {
        G_switchEndPpIface = SWITCH_END_PP_IFACE_SDMA;
    }
    else
    {
        G_switchEndPpIface = SWITCH_END_PP_IFACE_MII;
    }

    bootLineFlag  = sysBootParams.flags;
    bootLineFlag &= MV_USE_END_SWITCH_TX_MODE;
    if (bootLineFlag == 0)
    {
        G_switchEndIsTxSync = MV_FALSE;
    }
    else
    {
        G_switchEndIsTxSync = MV_TRUE;
    }
#endif

    /*
     * QFP boards does not have OOB port and hence mgiEnd is not needed.
     */
    if (mvPpChipIsXCat2() == MV_TRUE && mvChipXCat2IsQFP() == MV_TRUE)
    {
        MV_U32 i;

        /* Deactivate MgiEND anyway. */
        for(i = 0; endDevTbl[i].endLoadFunc != END_TBL_END; i++)
        {
            if (endDevTbl[i].endLoadFunc == mgiEndLoad)
            {
                mvOsPrintf("MgiEND is deactivated.\n");
                endDevTbl[i].processed = TRUE;
            }
        }
    }

    for( port=0; port < mvCtrlEthMaxPortGet(); port++ )
    {
        if (MV_FALSE == mvCtrlPwrClckGet(ETH_GIG_UNIT_ID, port))
        {
            mvOsPrintf("Warning Giga %d is Powered Off.\n", port);
            continue;
        }

        switch (mvBoardIdGet())
        {
        case RD_98DX3121_ID:
            mvEthE1116PhyBasicInit(port);
            break;
        case DB_98DX4122_ID:
        case DB_98DX2122_ID:
        {
            mvEthE1116PhyBasicInit(0);
            break;
            /*
             * MV_U8 boardRev = mvBoardRevRead();
             * if (boardRev == MV_BOARD_REV1)
             *     mvEthE1111PhyBasicInit(0);
             * else if (boardRev == MV_BOARD_REV2)
             *     mvEthE1116PhyBasicInit(0);
             * break;
             */
        }
        default:
            mvOsPrintf("%s: ERROR: can't find system phy init.\n", __func__);
        }

        mvOsPrintf("Init Phy port %d .. Done.\n", port);
    }

    mvEthInit();
    mvOsPrintf("Eth unit init (HAL & mem wins) ... Done.\n");

    /* load interface(s) */
    for (port=0; port < mvCtrlEthMaxPortGet(); port++)
    {
        if (MV_FALSE == mvCtrlPwrClckGet(ETH_GIG_UNIT_ID, port))
        {
            continue;
        }

        /* If port works in SGMII mode then SerDes should be init */
        if (mvBoardIsPortInSgmii(port) == MV_FALSE)
        {
            mvEthPortPowerUp(port);
            mvOsPrintf("Power up GMII port%d .. Done.\n",port);
        }
    }
    return MV_OK;
}

#ifdef __cplusplus
}
#endif
