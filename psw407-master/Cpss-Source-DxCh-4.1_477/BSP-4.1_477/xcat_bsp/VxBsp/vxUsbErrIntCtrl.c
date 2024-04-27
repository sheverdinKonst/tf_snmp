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
* vxUsbErrIntCtrl.c - USB interrupt controller library
*
* DESCRIPTION:
*       This driver provides various interface routines to manipulate and
*       connect the hardware interrupts concerning the USB unit.
*
*       Main features:
*        - The controller provides an easy way to hook a C Interrupt Service
*          Routine (ISR) to a specific interrupt caused by the USB channels.
*        - The controller interrupt mechanism provides a way for the programmer
*          to set the priority of an interrupt.
*        - Full interrupt control over the USB Errors.
*       
*       Overall Interrupt Architecture description:
*        The system controller handles interrupts in two stages:
*        STAGE 1: Main Cause register that summarize the interrupts generated
*                 by each Internal controller unit.
*        STAGE 2: Unit specified cause registers, that distinguish between each
*                 specific interrupt event.
*        This USB Interrupt Controller Driver handles the various interrupts
*        generated by the second stage.
*
*       All USB various error interrupt causes are numbered using the 
*       USB_ERR_CAUSE enumerator. 
*
*       The driver's execution flow has three phases:
*        1) Driver initialization. This initiation includes hooking driver's
*           handlers to the main Interrupt controller. Composed 
*           of vxUsbErrIntCtrlInit()
*           routine.
*        2) User ISR connecting. Here information about user ISR and interrupt
*           priority is gathered. Composed of vxUsbErrIntConnect() routine.
*        3) Interrupt handler. Here interrupts are being handle by the
*           Interrupt Handler. Composed of usbErrHandler(),
*
*       Full API:
*         vxUsbErrIntCtrlInit() - Initiate the USB interrupt controller driver.
*         vxUsbErrIntConnect()  - Connect a user ISR to an USB interrupt event.
*         vxUsbErrIntEnable()   - Enable a given USB interrupt cause.
*         vxUsbErrIntDisable()  - Disable a given USB interrupt cause.
*         usbErrHandler()       - Handles USB interrupts according to user ISR.
*
*       The controller concept is very simple:
*       The Interrupt handler has a table which holds information on
*       the connected user ISR. An Interrupt generated by one of the USB
*       channels will result a search through this table in order to allocate
*       the generating interrupt cause.
*       After the initiating interrupt cause is identify, the ISR reside in
*       the same table entry is executed.
*
*       The controller interface also includes interrupt control routines which
*       can enable/disable specific interrupts: vxUsbErrIntDisable() and
*       vxUsbErrIntEnable().
*
* DEPENDENCIES:
*       Main Interrupt Control Driver.
*       USB module.
*       VxWorks types and interrupt lib.
*
*******************************************************************************/

/* includes */
#include "VxWorks.h"
#include "config.h"

#include "mvUsb.h"
#include "mvCtrlEnvLib.h"
#include "vxUsbErrIntCtrl.h"
#include "logLib.h"
#include "intLib.h"
#include "mvOs.h"

#ifdef __cplusplus
extern "C" {
#endif

/* defines  */
#define USB_INVALID_CAUSE(cause)    \
            ((cause) <= USB_ERR_CAUSE_START || USB_ERR_CAUSE_END <= (cause))

/* typedefs */


/* locals   */
LOCAL MV_ISR_ENTRY usbErrCauseArray[32]; 
LOCAL int  usbErrCauseCount = 0;    /* Accumulates the number of connection */
LOCAL int  vxUsbErrIntCtrlInitialized = 0;
LOCAL void usbErrShow(UINT32 chan, UINT32 causeBit);

/*******************************************************************************
* vxUsbErrIntCtrlInit - Initiating the USB Error Interrupt Controller driver.
*
* DESCRIPTION:
*       This routines connects the drivers error show routine to the interrupt handlers to its
*       corresponding bits in the main Interrupt Controller using the
*       mvIntConnect() routine.
*       It is also cleans and masks interrupts.
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
void vxUsbErrIntCtrlInit(void)
{
	int chan;
	volatile MV_U32 dummy;

	for (chan = 0; chan < mvCtrlUsbMaxGet(); chan++)
    {
        MV_REG_WRITE(USB_CAUSE_REG(chan), 0);    /* Clean Cause register */
        MV_REG_WRITE(USB_MASK_REG(chan), 0);     /* Clean Mask register  */
        
        /* Clear any previous latched error by reading error address 	        */
        dummy = MV_REG_READ(USB_ERROR_ADDR_REG(chan));
    }

    if(vxUsbErrIntCtrlInitialized)
       return;
     
	for (chan = 0; chan < mvCtrlUsbMaxGet(); chan++)
	{
        /* Connect the default error handles that only prints a massage */
        vxUsbErrIntConnect(USB_ADDR_DEC, usbErrShow, USB_ADDR_DEC, 5);
        vxUsbErrIntConnect(USB_HOF     , usbErrShow, USB_HOF     , 5);
        vxUsbErrIntConnect(USB_DOF     , usbErrShow, USB_DOF     , 5);
        vxUsbErrIntConnect(USB_DUF     , usbErrShow, USB_DUF     , 5);

        vxUsbErrIntEnable(chan, USB_ADDR_DEC);
        vxUsbErrIntEnable(chan, USB_HOF);
        vxUsbErrIntEnable(chan, USB_DOF);
        vxUsbErrIntEnable(chan, USB_DUF);

	}

    vxUsbErrIntCtrlInitialized = 1;
}

/*******************************************************************************
* vxUsbErrIntConnect - connect a C routine to a specific USB Error interrupt.
*
* DESCRIPTION:
*       This routine connects a specified user ISR to a specified USB
*       Error interrupt cause.
*       The ISR handler has its own user ISR array.
*       The connection is done by setting the desired routine and parameter in
*       the cause array (usbErrCauseArray[]):
*         1) Check for existing connection for the cause bit in the table.
*         2) Connecting the user ISR by inserting the given parameters into
*           an entry according to the user ISR given priority.
*
* INPUT:
*       cause   - USB interrupt cause. See USB_ERR_CAUSE.
*       routine - user ISR.
*       arg1    - user ISR argument1.
*       prio    - Interrupt handling priority where 0 is highest.
*
* OUTPUT:
*       None.
*
* RETURN:
*       OK    - if the table entry of the cause bit, was filled.
*       ERROR - if cause argument is invalid or connected cause is already
*                found in table.
*
*******************************************************************************/
STATUS vxUsbErrIntConnect(USB_ERR_CAUSE cause, VOIDFUNCPTR routine, 
                          int arg1, int prio)
{
    int i, sysIntOldConfig, causeBitMask;
     
    /* Make sure that this is an atomic operation */
    sysIntOldConfig = intLock();

    /* Check the validity of the parameters */
    if(routine == NULL)
    {
        mvOsPrintf("\nvxUsbErrIntConnect: NULL pointer routine\n");
        intUnlock(sysIntOldConfig);
        return ERROR;
    }
    else if(prio < 0)
    {
        mvOsPrintf("\nvxUsbErrIntConnect: Invalid interrupt priority\n");
        intUnlock(sysIntOldConfig);
        return ERROR;
    }
    else if(USB_INVALID_CAUSE(cause))
    {
        mvOsPrintf("\nvxUsbErrIntConnect: Invalid cause %d\n",cause);
        intUnlock(sysIntOldConfig);
        return ERROR;
    }

    /* Calculate the cause bit location in the register */
    causeBitMask = MV_32BIT_BE(MV_BIT_MASK(cause));
    /* Scan the table to check for reconnection to the same cause bit */
    for(i = usbErrCauseCount; i >= 0; i--)
        if(usbErrCauseArray[i].causeBit == causeBitMask)
        {
            usbErrCauseArray[i].userISR  = routine;
            usbErrCauseArray[i].arg1     = arg1;
            intUnlock(sysIntOldConfig);
            return OK;
        }
    
    /* Connection phase */
    for(i = usbErrCauseCount; i >= 0; i--)
    {
        if(i == 0 || usbErrCauseArray[i-1].prio < prio) 
        {
            /* Make connection */
            usbErrCauseArray[i].causeBit = causeBitMask;
            usbErrCauseArray[i].userISR  = routine;
            usbErrCauseArray[i].arg1     = arg1;
            usbErrCauseArray[i].prio     = prio;
            usbErrCauseCount++;
            break;
        }
        else
        {
            /* Push the low priority connection down the table */
            usbErrCauseArray[i].causeBit = usbErrCauseArray[i-1].causeBit;
            usbErrCauseArray[i].userISR  = usbErrCauseArray[i-1].userISR;
            usbErrCauseArray[i].arg1     = usbErrCauseArray[i-1].arg1;
            usbErrCauseArray[i].prio     = usbErrCauseArray[i-1].prio;
        }
    }

    intUnlock(sysIntOldConfig);
    return OK;
}

/*******************************************************************************
* vxUsbErrIntEnable - Enable an USB interrupt
*
* DESCRIPTION:
*       This routine unmasks a specified USB cause in the mask register.
*       The routine will preform argument validity check.
*
* INPUT:
*       chan  - USB channel number.
*       cause - USB error interrupt cause as defined in USB_ERR_CAUSE.
*
* OUTPUT:
*       None.
*
* RETURN:
*       OK    - If the bit was set
*       ERROR - In case of invalid parameters.
*
*******************************************************************************/
STATUS vxUsbErrIntEnable(MV_U32 chan, USB_ERR_CAUSE cause)
{
    if (USB_INVALID_CAUSE(cause))
    {
        printf("\nvxUsbErrIntEnable: Invalid cause %d\n", cause);
        return ERROR;
    }
    else if(chan >= mvCtrlUsbMaxGet())
    {
        printf("\nvxUsbErrIntEnable: Invalid chan %d\n",chan);
        return ERROR;
    }

    /* Set mask bit to enable interrupt */
	MV_REG_BIT_SET(USB_MASK_REG(chan), MV_BIT_MASK(cause));
    
    return OK;
}

/*******************************************************************************
* vxUsbErrIntDisable - Disable an USB interrupt
*
* DESCRIPTION:
*       This routine masks a specified USB erorr interrupt in the mask register.
*       The routine will preform argument validity check.
*
* INPUT:
*       chan  - USB channel number.
*       cause - USB error interrupt cause as defined in USB_ERR_CAUSE.
*
* OUTPUT:
*       None.
*
* RETURN:
*       OK    - If the bit was set
*       ERROR - In case of invalid parameters.
*
*******************************************************************************/
STATUS vxUsbErrIntDisable(MV_U32 chan, USB_ERR_CAUSE cause)
{
    if (USB_INVALID_CAUSE(cause))
    {
        printf("\nvxUsbErrIntDisable: Invalid cause %d\n", cause);
        return ERROR;
    }
    else if(chan >= mvCtrlUsbMaxGet())
    {
        printf("\nvxUsbErrIntDisable: Invalid chan %d\n",chan);
        return ERROR;
    }

    /* Reset mask bit to disable interrupt */
	MV_REG_BIT_RESET(USB_MASK_REG(chan), MV_BIT_MASK(cause));

    return OK;
}
/*******************************************************************************
* vxUsbErrIntClear - Clear an USB interrupt
*
* DESCRIPTION:
*       This routine clears a specified USB interrupt in the cause register.
*       The routine will preform argument validity check.
*
* INPUT:
*       chan  - USB channel number.
*       cause - USB error interrupt cause as defined in USB_ERR_CAUSE
*
* OUTPUT:
*       None.
*
* RETURN:
*       OK    - If the bit was set
*       ERROR - In case of invalid parameters.
*
*******************************************************************************/
STATUS vxUsbErrIntClear(MV_U32 chan, USB_ERR_CAUSE cause)
{
    if (USB_INVALID_CAUSE(cause))
    {
        printf("\nvxUsbErrIntClear: Invalid cause %d\n", cause);
        return ERROR;
    }
    else if(chan >= mvCtrlUsbMaxGet())
    {
        printf("\nvxUsbErrIntClear: Invalid chan %d\n",chan);
        return ERROR;
    }

    /* Reset mask bit to disable interrupt */
	MV_REG_WRITE(USB_CAUSE_REG(chan), ~MV_BIT_MASK(cause));

    return OK;
}

/*******************************************************************************
* usbErrHandler - USB interrupt handler.
*
* DESCRIPTION:
*       This routine handles the USB error interrupts.
*       As soon as the interrupt signal is active the CPU analyzes the USB
*       Interrupt Cause register in order to locate the originating
*       interrupt event.
*       Then the routine calls the user specified service routine for that
*       interrupt cause.
*       The function scans the usbErrCauseArray[] (usbErrCauseCount valid 
*		entries) trying to find a hit in the usbErrCauseArray cause table.
*       When found, the ISR in the same entry is executed.
*		Note: The handler automatically acknowledges the generating interrupts.
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
void usbErrHandler (void)
{
    int i, chan;
    UINT32 activeErr, dummy;

	/* Check all channels interrupt registers for active interrupts */
    for (chan = 0; chan < mvCtrlUsbMaxGet(); chan++)
    {
        /* Read cause and mask registers */
        activeErr = MV_REG_READ(USB_MASK_REG(chan)) & 
	    		    MV_REG_READ(USB_CAUSE_REG(chan));				
        
         /* Execute user ISR if connected */
        for(i = 0; i < usbErrCauseCount; i++)
            if(activeErr & usbErrCauseArray[i].causeBit)
               (*usbErrCauseArray[i].userISR) (chan, usbErrCauseArray[i].arg1);
            
        /* If multiple errors occur, only the first error is latched.       */
        /* New error report latching is only enabled after the Error        */
        /* Address register Low is being read, and interrupt is cleared.    */
        dummy  = MV_REG_READ(USB_ERROR_ADDR_REG(chan));
        
        /* Acknowledge pending interrupts to allow next interrupts to       */
        /* register.Pay attention not to acknowledge completion bits		*/
        MV_REG_WRITE(USB_CAUSE_REG(chan), ~(activeErr));

    }
    return;
}

/*******************************************************************************
* usbErrShow - Display error information.
*
* DESCRIPTION:
*       This routine display USB error information according to cause
*		register for errors that does not latch error address/data.
*
* INPUT:
*       chan     - Channel number.
*       causeBit - cause bit number.
*
* OUTPUT:
*       None.
*
* RETURN:
*       None.
*
* NOTE:
*       The user can add here messaging about End Of Transfer (EOT) interrupt. 
*******************************************************************************/
LOCAL void usbErrShow	(UINT32 chan, UINT32 causeBit)
{
    UINT32 errAddr;

    logMsg("USB error:\n",0,0,0,0,0,0);
    
    switch(causeBit)
    {
        case(USB_ADDR_DEC):
            errAddr = MV_REG_READ(USB_ERROR_ADDR_REG(chan));
            logMsg("Channel %d Address Decoding Error\n", chan,0,0,0,0,0);
            logMsg("Error address 0x%08x\n", errAddr,0,0,0,0,0);            
            break;
        case(USB_HOF):
            logMsg("Channel %d USB Host Underflow\n", chan,0,0,0,0,0);
            break;
        case(USB_DOF):
            logMsg("Channel %d USB Host/Device overflow\n", chan,0,0,0,0,0);
            break;
        case(USB_DUF):
            logMsg("Channel %d USB Device underflow\n", chan,0,0,0,0,0);
            break;
        default:
            logMsg("Channel %d Unknown error!\n", chan,0,0,0,0,0);
    }
    return;
}

#ifdef __cplusplus
}
#endif