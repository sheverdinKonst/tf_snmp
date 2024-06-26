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
* vxXorIntCtrl.c - XOR interrupt controller library
*
* DESCRIPTION:
*       This driver provides various interface routines to manipulate and
*       connect the hardware interrupts concerning the XOR unit.
*
*       Main features:
*        - The controller provides an easy way to hook a C Interrupt Service
*          Routine (ISR) to a specific interrupt caused by the XOR channels.
*        - The controller interrupt mechanism provides a way for the programmer
*          to set the priority of an interrupt.
*        - Full interrupt control over the XOR facility.
*       NOTE:
*       Eventhough this driver have XOR completion event handler, 
*       the user can connect to the XOR completion bit using the Main 
*       Interupt controller (VxIntConnect), thus saving the calling to this 
*       driver handler. Note that the completion interrupt is acknowledged 
*       in the XOR cause register and not in the main cause register, so the 
*       user ISR must use this driver interrupt clear routine vxXorIntClear ()
*		Also note that this driver supports one user ISR for all 
*		completion events (EOD, Stopped and Paused).
*       
*       Overall Interrupt Architecture description:
*        The system controller handles interrupts in two stages:
*        STAGE 1: Main Cause register that summarize the interrupts generated
*                 by each Internal controller unit.
*        STAGE 2: Unit specified cause registers, that distinguish between each
*                 specific interrupt event.
*        This XOR Interrupt Controller Driver handles the various interrupts
*        generated by the second stage.
*
*       All XOR various error interrupt causes are numbered using the 
*       XOR_ERR_CAUSE enumerator. 
*
*       The driver's execution flow has three phases:
*        1) Driver initialization. This initiation includes hooking driver's
*           handlers to the main Interrupt controller. Composed 
*           of vxXorIntCtrlInit()
*           routine.
*        2) User ISR connecting. Here information about user ISR and interrupt
*           priority is gathered. Composed of vxXorIntConnect() routine.
*        3) Interrupt handler. Here interrupts are being handle by the
*           Interrupt Handler. Composed of xorIntHandler(),
*
*       Full API:
*         vxXorIntCtrlInit() - Initiate the XOR interrupt controller driver.
*         vxXorIntConnect()  - Connect a user ISR to an XOR interrupt event.
*         vxXorIntEnable()   - Enable a given XOR interrupt cause.
*         vxXorIntDisable()  - Disable a given XOR interrupt cause.
*         xorIntErrHandler() - Handles XOR interrupts according to user ISR.
*
*       The controller concept is very simple:
*       The Interrupt handler has a table which holds information on
*       the connected user ISR. An Interrupt generated by one of the XOR
*       channels will result a search through this table in order to allocate
*       the generating interrupt cause.
*       After the initiating interrupt cause is identify, the ISR reside in
*       the same table entry is executed.
*
*       The controller interface also includes interrupt control routines which
*       can enable/disable specific interrupts: vxXorIntDisable() and
*       vxXorIntEnable().
*
* DEPENDENCIES:
*       Main Interrupt Control Driver.
*       XOR module.
*       VxWorks types and interrupt lib.
*
*******************************************************************************/

/* includes */
#include "VxWorks.h"
#include "config.h"

#include "mvXor.h"
#include "mvXorRegs.h"
#include "vxXorIntCtrl.h"
#include "logLib.h"

#ifdef __cplusplus
extern "C" {
#endif

/* defines  */
#define XOR_INVALID_CAUSE(cause)    \
            ((cause) <= XOR_ERR_CAUSE_START || XOR_ERR_CAUSE_END <= (cause))

/* typedefs */


/* locals   */
LOCAL MV_ISR_ENTRY xorErrCauseArray[32]; 
LOCAL int  xorErrCauseCount = 0;    /* Accumulates the number of connection */
LOCAL int  vxXorIntCtrlInitialized = 0;
LOCAL void xorErrShow(UINT chan, UINT32 activeErr);
LOCAL void xorErrSelShow(UINT chan, UINT32 errType);

/*******************************************************************************
* vxXorIntCtrlInit - Initiating the XOR Interrupt Controller driver.
*
* DESCRIPTION:
*       This routines connects the drivers interrupt handlers to its
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
void vxXorIntCtrlInit(void)
{
	int chan;
	MV_U32 dummy;

	MV_REG_WRITE(XOR_CAUSE_REG(0), 0);    /* Clean Cause register */
	MV_REG_WRITE(XOR_MASK_REG(0), 0);     /* Clean Mask register  */
#if defined (MV88F6XXX) 
	MV_REG_WRITE(XOR_MASK_REG(1), 0);     /* Clean Mask register  */
	MV_REG_WRITE(XOR_CAUSE_REG(1), 0);    /* Clean Cause register */
#endif
    /* Clear any previous latched error by reading error address 	        */
    dummy = MV_REG_READ(XOR_ERROR_ADDR_REG(0));
#if defined (MV88F6XXX) 
    dummy = MV_REG_READ(XOR_ERROR_ADDR_REG(1));
#endif
    if(vxXorIntCtrlInitialized)
       return;

	/* cause register. Set priority for those interrupts is done here   */
	for (chan = 0; chan < MV_XOR_MAX_CHAN; chan++)
	{
        /* Connect the default error handles that only prints a massage */
        vxXorIntConnect(chan, XOR_ADDR_DEC,xorErrSelShow,chan, XOR_ADDR_DEC, 0);
        vxXorIntConnect(chan, XOR_ACCPROT, xorErrSelShow,chan, XOR_ACCPROT,  0);
        vxXorIntConnect(chan, XOR_WRPROT,  xorErrSelShow,chan, XOR_WRPROT,   0);
        vxXorIntConnect(chan, XOR_PAR_ERR, xorErrShow,   chan, XOR_PAR_ERR,  0);
        vxXorIntConnect(chan, XOR_XBAR_ERR,xorErrShow,   chan, XOR_XBAR_ERR, 0);
        
        /* Enable those interrupts */
        vxXorIntEnable(chan, XOR_ADDR_DEC);
        vxXorIntEnable(chan, XOR_ACCPROT);
        vxXorIntEnable(chan, XOR_WRPROT);
        vxXorIntEnable(chan, XOR_PAR_ERR);
        vxXorIntEnable(chan, XOR_XBAR_ERR);
	}

    vxXorIntCtrlInitialized = 1;
}

/*******************************************************************************
* vxXorIntConnect - connect a C routine to a specific XOR interrupt.
*
* DESCRIPTION:
*       This routine connects a specified user ISR to a specified XOR
*       interrupt cause. Note: For completion routine, this driver returns 
*		ERROR. Each of the ISR handlers has its own user ISR array.
*       The connection is done by setting the desired routine and parameter in
*       the cause array (xorErrCauseArray[]):
*         1) Check for existing connection for the cause bit in the table.
*         2) Connecting the user ISR by inserting the given parameters into
*           an entry according to the user ISR given priority.
*		NOTE: This driver does NOT support user ISR for all completion events
*		      (EOD, Stopped and Paused).
*
* INPUT:
*       chan      - XOR Channel number.
*       cause     - XOR interrupt cause. See XOR_ERR_CAUSE.
*       routine   - user ISR.
*       parameter - user ISR parameter.
*       prio      - Interrupt handling priority where 0 is highest.
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
STATUS vxXorIntConnect(MV_U32 chan, XOR_ERR_CAUSE cause,
                        VOIDFUNCPTR routine, int arg1, int arg2, int prio)
{
    int i, sysIntOldConfig, causeBitMask;

    /* Make sure that this is an atomic operation */
    sysIntOldConfig = intLock();

    /* Check the validity of the parameters */
    if(routine == NULL)
    {
        logMsg("\nvxXorIntConnect: NULL pointer routine\n",0,0,0,0,0,0);
        intUnlock(sysIntOldConfig);
        return ERROR;
    }
    else if(prio < 0)
    {
        logMsg("\nvxXorIntConnect: Invalid interrupt priority\n",0,0,0,0,0,0);
        intUnlock(sysIntOldConfig);
        return ERROR;
    }
    else if(XOR_INVALID_CAUSE(cause))
    {
        logMsg("\nvxXorIntConnect: Invalid cause %d\n",cause,0,0,0,0,0);
        intUnlock(sysIntOldConfig);
        return ERROR;
    }
    else if(chan >= MV_XOR_MAX_CHAN)
    {
        logMsg("\nvxXorIntConnect: Invalid chan %d\n",chan,0,0,0,0,0);
        intUnlock(sysIntOldConfig);
        return ERROR;
    }
	
	/* This driver does NOT support user ISR for all completion events	*/
    if ((XOR_EOD == cause) 		||
		(XOR_EOC == cause) 		||
		(XOR_STOPPED == cause) 	||
		(XOR_PAUSED == cause))
    {
        printf("\nvxXorIntConnect: Use intConnect for XOR completion\n");
        return ERROR;
    }
    else
    {
        /* Calculate the cause bit location in the register according to chan */
        causeBitMask = MV_32BIT_LE(XEICR_CAUSE_MASK(XOR_CHAN(chan), cause));
        
        /* Scan the table to check for reconnection to the same cause bit */
        for(i = xorErrCauseCount; i >= 0; i--)
            if(xorErrCauseArray[i].causeBit == causeBitMask)
            {
                xorErrCauseArray[i].userISR  = routine;
                xorErrCauseArray[i].arg1     = arg1;
                xorErrCauseArray[i].arg2     = arg2;

                intUnlock(sysIntOldConfig);
                return OK;
            }
        
        /* Connection phase */
        for(i = xorErrCauseCount; i >= 0; i--)
        {
            if(i == 0 || xorErrCauseArray[i-1].prio < prio) 
            {
                /* Make connection */
                xorErrCauseArray[i].causeBit = causeBitMask;
                xorErrCauseArray[i].userISR  = routine;
                xorErrCauseArray[i].arg1     = arg1;
                xorErrCauseArray[i].arg2     = arg2;
                xorErrCauseArray[i].prio     = prio;
                xorErrCauseCount++;
                break;
            }
            else
            {
                /* Push the low priority connection down the table */
                xorErrCauseArray[i].causeBit = xorErrCauseArray[i-1].causeBit;
                xorErrCauseArray[i].userISR  = xorErrCauseArray[i-1].userISR;
                xorErrCauseArray[i].arg1     = xorErrCauseArray[i-1].arg1;
                xorErrCauseArray[i].arg2     = xorErrCauseArray[i-1].arg2;
                xorErrCauseArray[i].prio     = xorErrCauseArray[i-1].prio;
            }
        }
    }

    intUnlock(sysIntOldConfig);
    return OK;
}

/*******************************************************************************
* vxXorIntEnable - Enable an XOR interrupt
*
* DESCRIPTION:
*       This routine unmasks a specified XOR cause in the mask register.
*       The routine will preform argument validity check.
*
* INPUT:
*       chan  - XOR channel number.
*       cause - XOR interrupt cause as defined in XOR_ERR_CAUSE.
*
* OUTPUT:
*       None.
*
* RETURN:
*       OK    - If the bit was set
*       ERROR - In case of invalid parameters.
*
*******************************************************************************/
STATUS vxXorIntEnable(MV_U32 chan, XOR_ERR_CAUSE cause)
{
    if (XOR_INVALID_CAUSE(cause))
    {
        logMsg("\nvxXorIntEnable: Invalid cause %d\n", cause,0,0,0,0,0);
        return ERROR;
    }
    else if(chan >= MV_XOR_MAX_CHAN)
    {
        logMsg("\nvxXorIntEnable: Invalid chan %d\n",chan,0,0,0,0,0);
        return ERROR;
    }

    /* Set mask bit to enable interrupt */
	MV_REG_BIT_SET(XOR_MASK_REG(XOR_UNIT(chan)), XEICR_CAUSE_MASK(XOR_CHAN(chan),cause));
    
    return OK;
}

/*******************************************************************************
* vxXorIntDisable - Disable an XOR interrupt
*
* DESCRIPTION:
*       This routine masks a specified XOR interrupt in the mask register.
*       The routine will preform argument validity check.
*
* INPUT:
*       chan  - XOR channel number.
*       cause - XOR interrupt cause as defined in XOR_ERR_CAUSE.
*
* OUTPUT:
*       None.
*
* RETURN:
*       OK    - If the bit was set
*       ERROR - In case of invalid parameters.
*
*******************************************************************************/
STATUS vxXorIntDisable(MV_U32 chan, XOR_ERR_CAUSE cause)
{
    if (XOR_INVALID_CAUSE(cause))
    {
        logMsg("\nvxXorIntDisable: Invalid cause %d\n", cause,0,0,0,0,0);
        return ERROR;
    }
    else if(chan >= MV_XOR_MAX_CHAN)
    {
        logMsg("\nvxXorIntDisable: Invalid chan %d\n",chan,0,0,0,0,0);
        return ERROR;
    }

    /* Reset mask bit to disable interrupt */
	MV_REG_BIT_RESET(XOR_MASK_REG(XOR_UNIT(chan)), XEICR_CAUSE_MASK(XOR_CHAN(chan),cause));

    return OK;
}
/*******************************************************************************
* vxXorIntClear - Clear an XOR interrupt
*
* DESCRIPTION:
*       This routine clears a specified XOR interrupt in the cause register.
*       The routine will preform argument validity check.
*
* INPUT:
*       chan  - XOR channel number.
*       cause - XOR interrupt cause as defined in XOR_ERR_CAUSE
*
* OUTPUT:
*       None.
*
* RETURN:
*       OK    - If the bit was set
*       ERROR - In case of invalid parameters.
*
*******************************************************************************/
STATUS vxXorIntClear(MV_U32 chan, XOR_ERR_CAUSE cause)
{
    if (XOR_INVALID_CAUSE(cause))
    {
        logMsg("\nvxXorIntClear: Invalid cause %d\n", cause,0,0,0,0,0);
        return ERROR;
    }
    else if(chan >= MV_XOR_MAX_CHAN)
    {
        logMsg("\nvxXorIntClear: Invalid chan %d\n",chan,0,0,0,0,0);
        return ERROR;
    }

    /* Reset mask bit to disable interrupt */
	MV_REG_WRITE(XOR_CAUSE_REG(XOR_UNIT(chan)), ~XEICR_CAUSE_MASK(XOR_CHAN(chan), cause));

    return OK;
}

/*******************************************************************************
* xorIntErrHandler - XOR interrupt handler.
*
* DESCRIPTION:
*       This routine handles the XOR interrupts.
*       As soon as the interrupt signal is active the CPU analyzes the XOR
*       Interrupt Cause register in order to locate the originating
*       interrupt event.
*       Then the routine calls the user specified service routine for that
*       interrupt cause.
*       The function scans the xorErrCauseArray[] (xorErrCauseCount valid entries)
*       trying to find a hit in the xorErrCauseArray cause table.
*       When found, the ISR in the same entry is executed.
*		Note: The handler automatically acknowledges the generating interrupts.
*
* INPUT:
*       None.
*
* OUTPUT:
*       If a cause bit is active and it's connected to an ISR function,
*       the function will be called.
*
* RETURN:
*       None.
*
*******************************************************************************/
void xorIntErrHandler (UINT32 chan)
{
    int i;
    UINT32 activeErr, dummy;

    logMsg("XOR error:\n",0,0,0,0,0,0);
    /* Extract the pending error interrupts. Exclude completion interrupts 	*/
    activeErr = MV_REG_READ(XOR_CAUSE_REG(XOR_UNIT(chan))) & 
                MV_REG_READ(XOR_MASK_REG(XOR_UNIT(chan)))  & 
                MV_32BIT_LE(~XEICR_COMP_MASK_ALL);

    /* Execute user ISR if connected */
    for(i = 0; i < xorErrCauseCount; i++)
        if(activeErr & xorErrCauseArray[i].causeBit)
            (*xorErrCauseArray[i].userISR) (xorErrCauseArray[i].arg1,
                                            xorErrCauseArray[i].arg2);
    		
    /* If multiple errors occur, only the first error is latched.           */
    /* New error report latching is only enabled after the XOR Error        */
    /* Address register is being read.                                      */
    dummy  = MV_REG_READ(XOR_ERROR_ADDR_REG(XOR_UNIT(chan)));

    /* Acknowledge pending interrupts to allow next interrupts to register  */
    /* Pay attention not to acknowledge completion bits						*/
    MV_MEMIO32_WRITE((INTER_REGS_BASE | XOR_CAUSE_REG(XOR_UNIT(chan))), ~(activeErr));

    return;
}
/*******************************************************************************
* xorErrSelShow - Display error select information.
*
* DESCRIPTION:
*       This routine display XOR error select information according to error 
*		report registers.
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
*******************************************************************************/
LOCAL void xorErrSelShow	(UINT32 chan, UINT32 causeBit)
{
    UINT32 errSelect;
    UINT32 errAddr;

    logMsg("XOR error:\n",0,0,0,0,0,0);

    switch(causeBit)
    {
        case(XOR_ADDR_DEC):
            logMsg("Channel %d address miss error\n", chan,0,0,0,0,0);
            break;
        case(XOR_ACCPROT):
            logMsg("Channel %d access protection violation\n", chan,0,0,0,0,0);
            break;
        case(XOR_WRPROT):
            logMsg("Channel %d write protection violation\n", chan,0,0,0,0,0);
            break;
        default:
            return;
    }

    /* Read the error type latched						*/
    errSelect = MV_REG_READ(XOR_ERROR_CAUSE_REG(XOR_UNIT(chan))) & XEECR_ERR_TYPE_MASK;

    /* Print error information if the data latched belongs to this cause */
    if ((causeBit + XEICR_CAUSE_OFFS(XOR_CHAN(chan))) == errSelect)
    {
        errAddr = MV_REG_READ(XOR_ERROR_ADDR_REG(XOR_UNIT(chan)));
        logMsg("Error address 0x%08x\n", errAddr,0,0,0,0,0);
    }

	return;
}

/*******************************************************************************
* xorErrShow - Display error information.
*
* DESCRIPTION:
*       This routine display XOR error information according to cause
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
*       The user can add here messaging about OwnErr0 (Descriptor 
*       Ownership Violation) interrupt which is not supported. 
*******************************************************************************/
LOCAL void xorErrShow(UINT32 chan, UINT32 causeBit)
{
    
    logMsg("XOR error: chan=%d \n",chan,0,0,0,0,0);

    switch(causeBit)
    {
        case(XOR_PAR_ERR):
            logMsg("Parity error. caused by internal buffer read\n",0,0,0,0,0,0);
            break;
        case(XOR_XBAR_ERR):
            logMsg("Crossbar Parity error\n",0,0,0,0,0,0);
            logMsg("Caused by erroneous read response from Crossbar\n",
                                                                    0,0,0,0,0,0);
            break;
        default:
            return;
    }
    return;
}

#ifdef __cplusplus
}
#endif

