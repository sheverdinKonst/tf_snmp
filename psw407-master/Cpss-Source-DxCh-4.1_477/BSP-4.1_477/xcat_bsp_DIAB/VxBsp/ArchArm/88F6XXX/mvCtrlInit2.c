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
#include "VxWorks.h"
#include <stdlib.h>
#include "config.h"
#include "mvXor.h"
#include "vxPexIntCtrl.h"
#include "vxEthErrIntCtrl.h"
#include "vxGppIntCtrl.h"
#include "vxUsbErrIntCtrl.h"
#include "vxXorIntCtrl.h"
#include "mvUsb.h"
#include "vxCntmrIntCtrl.h"
#include "mvPciIf.h"
#include "mvsysXor.h"
#include "mvSysUsb.h"
#include "sysLib.h"
#ifdef __cplusplus
extern "C" {
#endif

/* defines */
PCI_IF_MODE sysPciIfModeGet(MV_U32 pciIf);

/* Globals */
BOOT_PARAMS sysBootParams;		/* parameters from boot line */
#if defined(INCLUDE_USER_APPL) /*	not for bootrom */
/* extern BOOT_PARAMS sysBootParams; */
MV_BOOL     mvUsbCtrlIsHost[MV_USB_MAX_PORTS];
#endif
/* extern */
IMPORT void sysPrintPoll(char *pString);
#ifdef MV_INCLUDE_PEX
void sysPciIntCtrlInit(void);
#endif
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

    /* Connect and enable GbE interface error report interrupts */
    vxEthErrIntCtrlInit();

    /* Connect and enable Ethernet unit error report interrupts */
    intConnect(INT_VEC_GBE0_ERR ,ethErrIntHandler, (int)0);
    intEnable(INT_LVL_GBE0_ERR);
    intConnect(INT_VEC_GBE1_ERR ,ethErrIntHandler, (int)1);
    intEnable(INT_LVL_GBE1_ERR);

    mvOsPrintf("Eth int Ctrl.... Done\n");
    /* Enable GPP registartion of interrupt event */
    vxGppIntCtrlInit();

    /* Connect and enable GPP interurpt controlelr */
    intConnect(INT_VEC_GPIOLO0_7  , gpp7_0Int  , (int)0);
    intConnect(INT_VEC_GPIOLO8_15 , gpp15_8Int , (int)0);
    intConnect(INT_VEC_GPIOLO16_23, gpp23_16Int, (int)0);
    intConnect(INT_VEC_GPIOLO24_31, gpp31_24Int, (int)0);
    intConnect(INT_VEC_GPIOHI0_7  , gpp7_0Int  , (int)1);
    intConnect(INT_VEC_GPIOHI8_15 , gpp15_8Int , (int)1);
    intConnect(INT_VEC_GPIOHI16_23, gpp23_16Int, (int)1);

    intEnable(INT_LVL_GPIOLO0_7  );
    intEnable(INT_LVL_GPIOLO8_15 );
    intEnable(INT_LVL_GPIOLO16_23);
    intEnable(INT_LVL_GPIOLO24_31);
    intEnable(INT_LVL_GPIOHI0_7  );
    intEnable(INT_LVL_GPIOHI8_15 );
    intEnable(INT_LVL_GPIOHI16_23);

    mvOsPrintf("GPIO int Ctrl... Done\n");

#ifdef INCLUDE_PCI
    if (mvChipFeaturesGet()->numOfPex > 0)
    {
        int pciIf, retVal;

        /* Connect and enable PEX1 interrupt controller */
        vxPexIntCtrlInit();

        /* Connect driver handler to PEX0 Err summary bit in main cause register */

        intConnect(INT_VEC_PEX0_ERR ,pexIntErrHandler, 0);
        intEnable(INT_LVL_PEX0_ERR);
        /* Up to now we connected and enabled the PCI/PEX error interrupts. Now	*/
        /* connect and enable PCI/PEX A, B, C, D interrupt controller */
        sysPciIntCtrlInit();

        mvOsPrintf("PCI int Ctrl.... Done\n");
        /* Init PCI interfaces */
        for(pciIf = 0; pciIf < (mvCtrlPexMaxIfGet()); pciIf++)
        {
            if (mvPexIsPowerUp(pciIf) == MV_FALSE)
            {
                continue;
            }
            retVal = mvPciIfInit(pciIf, sysPciIfModeGet(pciIf));
            if (retVal == MV_NO_SUCH)
            {
                mvOsPrintf("PCI if (%d) unsupported!!!\n", pciIf);
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
#endif /* INCLUDE_PCI */

#if defined(MV_INCLUDE_USB) /* not for bootrom */
    if (mvChipFeaturesGet()->numOfUsb > 0)
    {
        int i, UsbFlg;
        UsbFlg = SYSFLG_VENDOR_1;

        /* Init USB ports */
        for (i = 0; i < mvCtrlUsbMaxGet(); i++)
        {
            mvUsbCtrlIsHost[i] = (sysBootParams.flags & UsbFlg) ? USB_HOST:USB_DEVICE;
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
    vxXorIntCtrlInit();

    /* Connect the user ISR's to the XOR error bit in main cause register  */
    intConnect(INT_VEC_XOR0_ERR ,xorIntErrHandler, (int)0);
    intEnable(INT_LVL_XOR0_ERR);

    intConnect(INT_VEC_XOR1_ERR ,xorIntErrHandler,(int)1);
    intEnable(INT_LVL_XOR1_ERR);

    mvOsPrintf("XOR int Ctrl.... Done\n");

    /* Init XOR channels */
    if (mvXorInit() != MV_OK)
    {
            mvOsPrintf("XOR init failed!!!\n");
            return MV_ERROR;
    }
    mvOsPrintf("XOR init........ Done\n");
    return MV_OK;
}

#ifdef __cplusplus
}
#endif
