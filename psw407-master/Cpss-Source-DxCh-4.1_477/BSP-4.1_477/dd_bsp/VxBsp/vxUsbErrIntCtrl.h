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
* vxUsbErrIntCtrl.h - Header File for : IDMA interrupt controller driver
*
* DESCRIPTION:
*       None.
*
* DEPENDENCIES:
*       Main interrupt controller driver.
*
*******************************************************************************/

#ifndef __INCvxUsbErrIntCtrlh
#define __INCvxUsbErrIntCtrlh

/* includes */

#ifdef __cplusplus
extern "C" {
#endif

/* defines  */

/* typedefs */

/* USB_ERR_CAUSE is an enumerator that describes the USB error causes.    */
/* the enumerator describes the error cause bits positions in the USB      */
/* cause register.                                                          */

typedef enum _usbErrCause
{
    USB_ERR_CAUSE_START = -1, /* Cause Start boundry                */
    USB_ADDR_DEC        = 0,  /* Channel failed address decoding.   */
    USB_HOF             = 1,  /* USB Host Underflow                 */
    USB_DOF             = 2,  /* USB Host/Device overflow           */
    USB_DUF             = 3,  /* USB Device underflow               */
    USB_ERR_CAUSE_END         /* Cause End boundry                  */
} USB_ERR_CAUSE;

/* vxUsbErrIntCtrl.h API list */
void   vxUsbErrIntCtrlInit(void);
STATUS vxUsbErrIntConnect (USB_ERR_CAUSE cause, VOIDFUNCPTR routine, 
                           int arg1, int prio);
STATUS vxUsbErrIntEnable  (MV_U32 chan, USB_ERR_CAUSE cause);
STATUS vxUsbErrIntDisable (MV_U32 chan, USB_ERR_CAUSE cause);
STATUS vxUsbErrIntClear   (MV_U32 chan, USB_ERR_CAUSE cause);

void usbErrHandler(void);        /* USB error events handler */

#ifdef __cplusplus
}
#endif

#endif /* __INCvxUsbErrIntCtrlh */

