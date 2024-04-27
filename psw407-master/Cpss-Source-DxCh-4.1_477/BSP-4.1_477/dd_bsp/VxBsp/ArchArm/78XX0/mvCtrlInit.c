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
#include <vxWorks.h>
#include "config.h"
#include <stdlib.h>
#include <sysLib.h>

#include "ctrlEnv/mvCtrlEnvLib.h"
#include "ddr2/mvDramIf.h"
#include "cpu/mvCpu.h"
#include "ctrlEnv/sys/mvCpuIf.h"
#include "device/mvDevice.h"
#ifdef MV_INCLUDE_IDMA
#include "idma/mvIdma.h"
#include "ctrlEnv/sys/mvSysIdma.h"
#endif
#include "xor/mvXor.h"
#include "ctrlEnv/sys/mvSysXor.h"
#include "eth/mvEth.h"
#include "eth-phy/mvEthPhy.h"
#include "ctrlEnv/sys/mvSysGbe.h"
#include "sysmap.h"
#ifdef __cplusplus
extern "C" {
#endif

/* defines */
#define CFG_I2C_SPEED   	100000		/* I2C speed default */
#define CFG_TCLK		200000000

/* extern */

IMPORT long sysL2PrefetchFlag;  /* Enable/disable L2 prefetch in runtime    */

/* typedefs */

/* locals   */
#if !((_WRS_VXWORKS_MAJOR == 6) && (_WRS_VXWORKS_MINOR > 5)) || defined(BOOTROM)

int ipipsec_conf_enable_outfilter = 1;
int ipipsec_conf_enable_infilter = 1;
int ipipsec_conf_esp_enable = 1;
int ipipsec_conf_ah_enable = 1;
int ipipsec_conf_ipip_allow = 1;
#endif
#if defined(BOOTROM)
int* ipcom_shell_find_cmd(const char *name) 
{
	return NULL;
}
#endif



/*******************************************************************************
* mvCtrlInit0 - MV Initialization routine.
*
* DESCRIPTION:
*       The main initialize for this routine is the auto detect DRAM memory
*
*       Thie routine is caled befor clear the BSS the DRAM 
*
*       This routine also makes the following initialization:
*       1) Initialize Marvell controller environment
*       2) Initialize Controller CPU interface
*       2) Initialize TWSI interface
*
* INPUT:
*       None.
*
* OUTPUT:
*       OK or ERROR
*
* RETURN:
*       None.
*
*******************************************************************************/
MV_STATUS mvCtrlInit0 (MV_VOID)
{
#if defined(MV_INC_BOARD_DDIM)
	MV_U32 pDramConfig;
#endif
#if defined (MV78100)
    MV_U32	twsiActualFreq;
	MV_TWSI_ADDR twsiAddr;
	twsiAddr.address = MV_BOARD_CTRL_I2C_ADDR;
	twsiAddr.type = ADDR7_BIT;
#endif
		
#if defined (MV78200)
	if (MASTER_CPU == whoAmI())
#endif
	{
		if (MV_OK != mvCtrlEnvInit())
		{
			mvOsPrintf("Controller env init failed!!\n");
			return MV_ERROR;
		}
		mvOsPrintf("Ctrl env init............. Done.\n");
	}

	/* Init CPU interface. This will set CPU to periferal target address 	*/
	/* windows as defined by system HW configuration file. This will also 	*/
	/* Enable memory auto detection as it moves all other target windows	*/
	/* up in address space making room for DRAM								*/
    /* NOTE! It is assumed that this code is executed out of DRAM bank 0!!  */
	if (MV_OK != mvCpuIfInit(mv_sys_map()))
	{
		mvOsPrintf("CPU if init failed!!!\n");
		return MV_ERROR;
	}
	mvOsPrintf("CPU if init............... Done.\n"); 

#if defined (MV78100)
	{
		mvOsPrintf("TWSI init Channel %d....... ", MV_BOARD_DIMM_I2C_CHANNEL);
		/* Initialize TWSI interface in order to access DIMMs */
		twsiActualFreq = mvTwsiInit(MV_BOARD_DIMM_I2C_CHANNEL,(MV_U32)100000, mvBoardTclkGet(), &twsiAddr, MV_FALSE); 
		mvOsPrintf("Done. Actual freq = %dHz\n", twsiActualFreq); 
	}
#endif
#if defined(MV_INC_BOARD_DDIM)

	/* Init the DRAM interface */
	/* Note the pre configuration does not set DRAM register as we are	    */
	/* running from DRAM.											   	    */


	if (MV_OK != mvDramIfDetect(CAL_AUTO_DETECT, ECC_ENABLE))
	{
		mvOsPrintf("failed. System halt...\n");
		while(1);
	}
		
	pDramConfig  = (*(MV_U32*)(DRAM_CONFIG_ROM_ADDR+4)) - (*(MV_U32*)(DRAM_CONFIG_ROM_ADDR+8));
	pDramConfig += (*(MV_U32*)DRAM_CONFIG_ROM_ADDR);
	/* mvOsPrintf("Jump to ROMable DRAM config...0x%x \n", pDramConfig);  */
    mvOsPrintf("Memory init............... "); 
		
	/* Now call the configuration routine. The routine will run from 	*/
	/* ROM and configure DRAM registers									*/
	mvOsPrintf("Jump to ROMable DRAM config...0x%x ", pDramConfig);

	((FUNCPTR)(pDramConfig))(1);
    
	mvOsPrintf("Done.\n");
#else
	mvOsPrintf("Memory on board initialized .\n");
#endif

	return OK;
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
MV_STATUS 	mvCtrlInit(MV_VOID)
{
	int port;
	/* Stop IDMA engines */
#ifdef MV_INCLUDE_IDMA
#if defined (MV78200)
	if (mvSocUnitIsMappedToThisCpu(IDMA))
#endif
	{
		mvDmaAllStop();
		mvOsPrintf("IDMA channels............. stopped.\n");
	}

#endif
		/* Stop XOR engines */
#if defined (MV78200)
	if (mvSocUnitIsMappedToThisCpu(XOR))
#endif
	{
		mvXorAllStop();
		mvOsPrintf("XOR channels.............. stopped.\n");
	}
    
    /* Initialize Giga Ethernet Address decode, Clear and Disable ISRs */



	for (port = BOARD_ETH_START_PORT_NUM;port < mvCtrlEthMaxPortGet(); port++)
	{
		if (MV_FALSE == mvCtrlPwrClckGet(ETH_GIG_UNIT_ID, port)) 
		{
			mvOsPrintf("\nWarning Giga %d is Powered Off\n",port);
			continue;
        }
#if defined (MV78200)
		if (MV_FALSE == mvSocUnitIsMappedToThisCpu(GIGA0+port))
		{
		   /* mvOsPrintf("\nWarning Giga %d is is set to CPU%d\n",port,whoAmI()?0:1); */
			continue;
		}
#endif
		switch(mvBoardIdGet()) {
			/* This id DB-78XX0-BP */
#if defined (MV78XX0)
		case DB_78200_ID:
		case DB_78XX0_ID:
		case RD_78XX0_MASA_ID:
			mvEth1121PhyBasicInit(port);
			break;
		case RD_78XX0_H3C_ID:
			mvEthSgmiiToCopperPhyBasicInit(port);
			mvEth1145PhyBasicInit(port);
			/* This id RD-78XX0-AMC */
			break;
		case RD_78XX0_AMC_ID:
				mvEth1145PhyBasicInit(port);
				break;
#elif defined (MV88F6XXX) 
		case DB_88F6281_BP_ID:
		case RD_88F6281_ID: 
			mvEthE1116PhyBasicInit(port);
			break;
#endif
		default:
			mvOsPrintf("ERROR: can't find system phy init\n");

		}
		mvOsPrintf("Init Phy port %d .......... Done\n",port);
	}

    mvEthInit();
    mvOsPrintf("Eth unit init............. Done.\n");


	/* load interface(s) */ 
	for( port=0; port < mvCtrlEthMaxPortGet(); port++ )
	{
#if defined (MV78200)
		if (MV_FALSE == mvSocUnitIsMappedToThisCpu(GIGA0+port))
		{
			continue;
		}
#endif
		if (MV_FALSE == mvCtrlPwrClckGet(ETH_GIG_UNIT_ID, port)) continue;
        mvEthPortPowerUp(port);
		mvOsPrintf("Power up Giga Ethernet port%d .. Done.\n",port);
	}
    return MV_OK;
}

#ifdef __cplusplus
}
#endif
