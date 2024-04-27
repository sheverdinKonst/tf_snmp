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
* sysNflash.c - BSP Nand Flash initialization
*
* DESCRIPTION:
*       None.
*
* DEPENDENCIES:
*       None.
*
*******************************************************************************/

/* includes */
#include "vxWorks.h"
#include "config.h"
#include "mvBoardEnvLib.h"
#include "mvDevice.h"
#include "mvNflash.h"
#include "mvCpuIf.h"


#ifdef __cplusplus
extern "C" {
#endif
extern MV_U32    boardGetDevCSNum(MV_32 devNum, MV_BOARD_DEV_CLASS devType);

/* defines  */

/* typedefs */

/* locals   */
/* Nand flash identifer */
LOCAL MV_NFLASH_INFO nandFlashInfo;

#define MV_BOARD_NFLASH_DEVICE_WIDTH       8  /* 8-bit width Flash mode*/

/*******************************************************************************
* sysNflashHwInit - initialize the BSP NAND Flash device drivers
*
* DESCRIPTION:
*       This routine initializes the NAND Flash HW interface so the
*		device driver can access the Flash device.
*		It is called from sysHwInit() with interrupts locked.
*
* INPUT:
*       None.
*
* OUTPUT:
*       None.
*
* RETURN:
*       MV_TRUE     - success.
*       MV_FALSE    - failure.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_STATUS sysNflashHwInit(void)
{
    MV_CPU_DEC_WIN addr_win;

    nandFlashInfo.nflashHwIf.devWidth = MV_BOARD_NFLASH_DEVICE_WIDTH;

    if (mvCpuIfTargetWinGet(NFLASH_CS, &addr_win) != MV_OK)
    {
        mvOsPrintf("Failed to init NAND driver.\n");
        return MV_FAIL;
    }

    if (addr_win.enable == MV_FALSE)
    {
        mvOsPrintf("NAND DEVICE-CS window disabled.\n");
        return MV_FAIL;
    }

    nandFlashInfo.nflashHwIf.devBaseAddr = addr_win.addrWin.baseLow;

    /* Use defaul data get/set routines */
    nandFlashInfo.nflashHwIf.nfDataGetRtn = NULL;
    nandFlashInfo.nflashHwIf.nfDataSetRtn = NULL;

    if (mvNflashInit(&nandFlashInfo) != MV_OK)
    {
        mvOsPrintf("%s: mvNflashInit failed.\n", __func__);
        return MV_FAIL;
    }

    return MV_OK;
}

/*******************************************************************************
* sysNflashDevGet - get the MV_NFLASH_INFO associated with a NAND flash device.
*
* DESCRIPTION:
*       This routine gets the MV_NFLASH_INFO NAND flash identifier associated
*		with a specified NAND FLash device.
*
* INPUT:
*       int index - Number of NAND flash.
*
* OUTPUT:
*       None.
*
* RETURN:
*       A pointer to the SIO_CHAN structure for the channel,
*       ERROR - if the channel is invalid.
*
*******************************************************************************/
MV_NFLASH_INFO * sysNflashDevGet(int index)
{
    return((MV_NFLASH_INFO *)&nandFlashInfo);
}

/*******************************************************************************
* sysNflashDevDisplay - Display the MV_NFLASH_INFO associated with a NAND flash.
*
* DESCRIPTION:
*       This routine displays the MV_NFLASH_INFO NAND flash identifier associated
*		with a specified NAND FLash device.
*
* INPUT:
*       int index - Number of NAND flash.
*
* OUTPUT:
*       None.
*
* RETURN:
*       None.
*
*******************************************************************************/
void sysNflashDevDisplay(int index)
{
    mvNflashPrint(&nandFlashInfo);
}


void sysFlashWriteEnable(void)
{
}
void sysFlashWriteDisable(void)
{
}
MV_STATUS mvNandflashIdGet(MV_U8 * pBuffer, int ulBuffLen)
{
        MV_U16 flashId;
        flashId = mvNflashIdGet(&nandFlashInfo);
        mvOsPrintf("flashId =0x%x \n", flashId);
        pBuffer[0] = (MV_U8)(flashId & 0xFF);
        pBuffer[1] = (MV_U8)((flashId & 0xFF00)>>8);
        mvOsPrintf("pBuffer[0] =0x%x ", pBuffer[0]);
        mvOsPrintf("pBuffer[1] =0x%x\n", pBuffer[1]);
        return MV_OK;
}
#ifdef INCLUDE_FLASHFX
void mvFlshTest(void)
{
#ifdef WORKBENCH
        usrFlashFXInit62();
        usrFlashFXTestsInit62();
        usrFlashFXToolsInit62();
        usrFlashFXDeviceInit62("/ffx0",0,0,0,0xffffffff,1,2);
#else
        usrFlashFXInit();
        usrFlashFXTestsInit();
        usrFlashFXToolsInit();
        usrFlashFXDeviceInit("/ffx0",0,0,0,0xffffffff,1,2,0);
#endif
}
#endif

#ifdef __cplusplus
}
#endif


