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
* mvCtrlInit2.c - System Controller Initialization library file
*
* DESCRIPTION:
*       None.
*
* DEPENDENCIES:
*       None.
*
*******************************************************************************/

/* includes */
#include <VxWorks.h>
#include <stdlib.h>
#include <arch/arm/ivArm.h>

#include "config.h"

#ifdef MV_INCLUDE_IDMA
#include "idma/mvIdma.h"
#endif
#include "pci-if/mvPciIf.h"
#include "ddr2/mvDramIf.h"
#include "ctrlEnv/sys/mvSysXor.h"
#include "vxDramIntCtrl.h"
#include "vxPciIntCtrl.h"
#include "vxPexIntCtrl.h"
#include "vxEthErrIntCtrl.h"
#ifdef MV_INCLUDE_IDMA
#include "vxIdmaIntCtrl.h"
#include "ctrlEnv/sys/mvSysIdma.h"
#endif
#include "vxGppIntCtrl.h"
#include "vxXorIntCtrl.h"
#include "vxCntmrIntCtrl.h"
#include "sysLib.h"
#if defined(MV_INCLUDE_USB) || defined(MV_INCLUDE_USB_DEV_MARVELL) /*	not for bootrom */
#include "vxUsbErrIntCtrl.h"
#include "usb/mvUsb.h"
#include "ctrlEnv/sys/mvSysUsb.h"
#endif
#if defined (MV78200)
	#include "mv78200/mvSocUnitMap.h"
	#include "cpu/mvCpu.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* defines */

#define DRAM_SINGLE_BIT_ECC_ERR_THRESHOLD   100

/* Globals */
#if defined(MV_INCLUDE_USB) || defined(MV_INCLUDE_USB_DEV_MARVELL) /*	not for bootrom */
extern BOOT_PARAMS	sysBootParams;		/* parameters from boot line */
MV_BOOL     mvUsbCtrlIsHost[MV_USB_MAX_PORTS];
#endif
/* extern */
IMPORT void sysPrintPoll(char *pString);
IMPORT void sysPciIntCtrlInit(void);
IMPORT PCI_IF_MODE sysPciIfModeGet(MV_U32 pciIf);

/* typedefs */

/* locals   */

/*******************************************************************************
* mvCtrlInit2 - Controller Initialization routine 2.
*
* DESCRIPTION:
*       This routine initialize controller units drivers. 
*       This routine allows for driver initialization who requires "malloc"
*       which is unavailable in sysHwInit() but available in sysHwInit2().
*
* INPUT:
*       None.
*
* OUTPUT:
*       None.
*
* RETURN:
*       OK if routine succeeds.
*
*******************************************************************************/
MV_STATUS mvCtrlInit2(MV_VOID)
{
	
    
	/* auxiliary clock interrupt are located  */
	vxCntmrIntCtrlInit();


#if defined (MV78200)
	if (MASTER_CPU == whoAmI())
#endif
	{
		/* Connect and enable DRAM unit error report interrupts */
		vxDramIntCtrlInit();
		/* Connect and enable DRAM unit error report interrupts */
		intConnect(INT_VEC_DRAM_ERR ,dramIntErrHandler, 0);
		intEnable(INT_LVL_DRAM_ERR);

		mvOsPrintf("DRAM int Ctrl... Done\n");
		/* Note that in order to get single bit ECC error report, the single    */
		/* bit ECC threshold should be set to a value other then zero.          */
		/* If it is zero, ECC single bit error will not be reported             */
		mvDramIfSingleBitErrThresholdSet(DRAM_SINGLE_BIT_ECC_ERR_THRESHOLD);

	}

#ifdef MV_INCLUDE_IDMA
#if defined (MV78200)
	if (mvSocUnitIsMappedToThisCpu(IDMA))
#endif
	{
		/* Init IDMA channels */
		if (MV_OK != mvDmaInit())
		{
			mvOsPrintf("IDMA init failed!!!\n");
			return MV_ERROR;
		}
		mvOsPrintf("IDMA init....... Done\n");

		/* Connect and enable IDMA unit error report interrupts */
		vxIdmaIntCtrlInit();

		/* Connect and enable IDMA unit error report interrupts */
		intConnect(INT_VEC_DMA_ERR ,idmaErrHandler, 0);
		intEnable(INT_LVL_DMA_ERR);
		mvOsPrintf("IDMA int Ctrl... Done\n");
	}
#endif
	/* Connect and enable GbE interface error report interrupts */
	vxEthErrIntCtrlInit();

	/* Connect and enable Ethernet unit error report interrupts */
	intConnect(INT_VEC_GBE_ERR ,ethErrIntHandler, 0);
	intEnable(INT_LVL_GBE_ERR);

	mvOsPrintf("Eth int Ctrl.... Done\n");
	
#if defined (MV78200)
	if (MASTER_CPU == whoAmI())
#endif
	{
		/* Enable GPP registartion of interrupt event */
		vxGppIntCtrlInit();

		/* Connect and enable GPP interurpt controlelr */
		intConnect(INT_VEC_P0_GPIO0_7  , gpp7_0Int  , 0);
		intConnect(INT_VEC_P0_GPIO8_15 , gpp15_8Int , 0);
		intConnect(INT_VEC_P0_GPIO16_23, gpp23_16Int, 0);


		intEnable(INT_LVL_P0_GPIO0_7);
		intEnable(INT_LVL_P0_GPIO8_15);
		intEnable(INT_LVL_P0_GPIO16_23);

		mvOsPrintf("GPIO int Ctrl... Done\n");

	}

#ifdef INCLUDE_PCI
	{
		int pciIf, retVal;
	
		/* Connect and enable PEX1 interrupt controller */
		vxPexIntCtrlInit();
	
		/* Connect driver handler to PEX0 Err summary bit in main cause register */
#if defined (MV78200)
		if (mvSocUnitIsMappedToThisCpu(PEX00))
#endif
		{
			intConnect(INT_VEC_PEX0_ERR ,pexIntErrHandler, 0);
			intEnable(INT_LVL_PEX0_ERR);
		}
	
#if defined (MV78200)
		if (mvSocUnitIsMappedToThisCpu(PEX10))
#endif
		{
		/* Connect driver handler to PEX1 Err summary bit in main cause  register */
			intConnect(INT_VEC_PEX1_ERR ,pexIntErrHandler, 1);
			intEnable(INT_LVL_PEX1_ERR);
		}
	
		/* Up to now we connected and enabled the PCI/PEX error interrupts. Now	*/
		/* connect and enable PCI/PEX A, B, C, D interrupt controller 			*/
		sysPciIntCtrlInit();
	
		mvOsPrintf("PCI int Ctrl.... Done\n");
		/* Init PCI interfaces */	

#if defined (MV78200)
		if (mvSocUnitIsMappedToThisCpu(PEX00))
#endif
		{
			/* Power down the none lanes 0.1, 0.2, and 0.3 if PEX0 is X4 */
			if ( !(PCI0_IS_QUAD_X1) )
			{
				for (pciIf = 1; pciIf < 4; pciIf++)
					mvPexPowerDown(pciIf);
			}
	#ifdef MV78XX0
			else
		{
		#ifndef LION_RD /* interface 2 used on Lion RD board */
			mvPexPowerDown(2);	/*  interface 2 is not supported*/
		#endif	
		}
		#endif	
		}


		/* Power down the none lanes 1.1, 1.2, and 1.3 if PEX1 is X4 */
#if defined (MV78200)
		if (mvSocUnitIsMappedToThisCpu(PEX10))
#endif
		{
			if ( !(PCI1_IS_QUAD_X1) )
			{
				for (pciIf = 5; pciIf < 8; pciIf++)
					mvPexPowerDown(pciIf);
			}
		}
		for(pciIf = 0; pciIf < (mvCtrlPexMaxIfGet()); pciIf++)
		{
#if defined (MV78200)
			if (mvSocUnitIsMappedToThisCpu(PEX00 + (pciIf/4)) == MV_FALSE )
			{
				continue;
			}
#endif
			if (mvPexIsPowerUp(pciIf) == MV_FALSE)
			{
				continue;
			}
			retVal = mvPciIfInit(pciIf, sysPciIfModeGet(pciIf));
			if (retVal == MV_NO_SUCH)
			{   
				/* No Link. Shut down the Phy */
				if (((PCI0_IS_QUAD_X1) && (0 == pciIf)) ||
					((PCI1_IS_QUAD_X1) && (4 == pciIf)))
				{
					mvOsPrintf("PCI if (%d) unsupported!!!\n", pciIf);
				}
				else
				{
					mvPexPowerDown(pciIf);
					mvOsPrintf("PEX%d is shutdown!!!\n", pciIf);
				}
			}
			else if (retVal == MV_ERROR) 
			{
				mvOsPrintf("PCI if (%d) init failed!!!\n", pciIf);            
			}
			else
			{
				mvOsPrintf("PCI%d init....... Done. %s type selected\n", pciIf,
					(sysPciIfModeGet(pciIf) ? "DEVICE" : "HOST"));
			}
		}
		vxPexIntCtrlInit2();
	}
#endif  /* INCLUDE_PCI */
#if defined(MV_INCLUDE_USB) || defined(MV_INCLUDE_USB_DEV_MARVELL) /*	not for bootrom */
	{
		int i,UsbFlg;
        UsbFlg = SYSFLG_VENDOR_1;
		/* Init USB ports */
		for(i=0; i<mvCtrlUsbMaxGet(); i++)
		{
#if defined (MV78200)
			if (MV_FALSE == mvSocUnitIsMappedToThisCpu(USB0 + i))
			{
				UsbFlg = UsbFlg << 1;
				continue;
			}
#endif
#if defined(MV_INCLUDE_USB) 
			mvUsbCtrlIsHost[i] = (sysBootParams.flags & UsbFlg) ? USB_HOST:USB_DEVICE;
#else
			mvUsbCtrlIsHost[i] = USB_DEVICE;
#endif
			mvUsbInit(i, mvUsbCtrlIsHost[i]);
			mvOsPrintf("USB%d init....... Done. %s type selected\n",
					   i,(sysBootParams.flags & UsbFlg) ? "HOST  ":"DEVICE" ); 
			UsbFlg = UsbFlg << 1;
		}
	
		/* Connect and enable USB unit error report interrupts */
		vxUsbErrIntCtrlInit();
	
		/* Connect the user ISR's to the USB error bit in main cause register  */
		intConnect(INT_VEC_USB_ERR ,usbErrHandler, 0);
		intEnable(INT_LVL_USB_ERR);
		mvOsPrintf("USB int Ctrl.... Done\n");
	}
#endif
    
   
	/* Connect and enable XOR unit error report interrupts */
#if defined (MV78200)
	if (mvSocUnitIsMappedToThisCpu(XOR))
#endif
	{
		vxXorIntCtrlInit();

		/* Connect the user ISR's to the XOR error bit in main cause register  */
		intConnect(INT_VEC_XOR_ERR ,xorIntErrHandler, 0);
		intEnable(INT_LVL_XOR_ERR);
		mvOsPrintf("XOR int Ctrl.... Done\n");

		/* Init XOR channels */
		if (MV_OK != mvXorInit())
		{
			mvOsPrintf("XOR init failed!!!\n");
			return MV_ERROR;
		}
		mvOsPrintf("XOR init........ Done\n");
	}
	return MV_OK;

}

#ifdef __cplusplus
}
#endif
