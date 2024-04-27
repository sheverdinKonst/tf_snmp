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
* vxDramIntCtrl.c - DRAM Error interrupt controller library
*
* DESCRIPTION:
*       This driver provides various interface routines to manipulate and
*       connect the hardware interrupts concerning the DRAM errors.
*
*       Main features:
*        - The controller provides an easy way to hook a C Interrupt Service
*          Routine (ISR) to a specific DRAM error.
*        - The controller interrupt mechanism provides a way for the programmer
*          to set the priority of an interrupt.
*        - Full interrupt control over the DRAM error events.
*       
*       Overall Interrupt Architecture description:
*        The system controller handles interrupts in two stages:
*        STAGE 1: Main Cause register that summarize the interrupts generated
*                 by each Internal controller unit.
*        STAGE 2: Unit specified cause registers, that distinguish between each
*                 specific interrupt event.
*        This Interrupt Controller Driver handles the various interrupts
*        generated by the second stage.
*
*       All DRAM various error interrupt causes are numbered using  
*       the DRAM_ERR_CAUSE enumerator. 
*
*       The driver's execution flow has three phases:
*        1) Driver initialization. This initiation includes hooking driver's
*           handlers to the main Interrupt controller. Composed 
*           of vxDramIntCtrlInit() routine.
*        2) User ISR connecting. Here information about user ISR and interrupt
*           priority is gathered. Composed of vxDramIntConnect() routine.
*        3) Interrupt handler. Here interrupts are being handle by the
*           Interrupt Handler. Composed of dramIntHandler(),
*
*       Full API:
*         vxDramIntCtrlInit() - Initiate the DRAM interrupt controller driver.
*         vxDramIntConnect()  - Connect a user ISR to a DRAM error event.
*         vxDramIntEnable()   - Enable a given DRAM error interrupt cause.
*         vxDramIntDisable()  - Disable a given DRAM error interrupt cause.
*         dramIntErrHandler() - Handles DRAM errors user hooked ISR.
*
*       The controller concept is very simple:
*       The Interrupt handler has a table which holds information on
*       the connected user ISR. An error Interrupt generated by the DRAM
*       will result a search through this table in order to allocate
*       the generating interrupt cause.
*       After the initiating interrupt cause is identify, the ISR reside in
*       the same table entry is executed.
*
*       The controller interface also includes interrupt control routines which
*       can enable/disable specific interrupts: vxDramIntDisable() and
*       vxDramIntEnable().
*
*       NOTE:
*       This driver is designed to deal with multiple errors. The error 
*       latched in error select (SDRAM Error Address register bit[0]) is 
*       reported with address/data information. In case other errors are set 
*       in the cause register, they are also reported but without 
*       address/data information.
*
* DEPENDENCIES:
*       Main Interrupt Control Driver.
*       DRAM module.
*       VxWorks types and interrupt lib.
*
*******************************************************************************/

/* includes */
#include "VxWorks.h"
#include "config.h"

#include "mvSpd.h"
#include "vxDramIntCtrl.h"
#include "logLib.h"

#ifdef __cplusplus
extern "C" {
#endif

/* defines  */
#define DRAM_INVALID_CAUSE(cause)    \
            ((cause) <= DRAM_ERR_CAUSE_START || DRAM_ERR_CAUSE_END <= (cause))

/* typedefs */


/* locals   */
LOCAL MV_ISR_ENTRY dramErrCauseArray[DRAM_ERR_CAUSE_END]; 
LOCAL int  dramErrCauseCount = 0;    /* Accumulates the number of connection */
LOCAL int  vxDramIntCtrlInitialized = 0;
LOCAL void sdramErrShow (UINT32 causeBit);

/*******************************************************************************
* vxDramIntCtrlInit - Initiating the DRAM error Interrupt Controller driver.
*
* DESCRIPTION:
*       This routines connects the driver's interrupt handler to its
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
void vxDramIntCtrlInit(void)
{
	MV_U32 dummy;

    MV_REG_WRITE(SDRAM_ERROR_CAUSE_REG, 0);    /* Clean Cause register */
	MV_REG_WRITE(SDRAM_ERROR_MASK_REG, 0);     /* Clean Mask register  */

    /* Set the error counters to work in normal mode (not the default) */
    MV_REG_BIT_SET(SDRAM_ECC_CONTROL_REG, SECR_NORMAL_COUNTER);

    /* Clear any previous latched error by reading error address and        */
    /* clear the interrupt cause                                            */
    dummy   = MV_REG_READ(SDRAM_ERROR_ADDR_REG);
    
    if(vxDramIntCtrlInitialized)
      return;

    /* Connect the default error handles that only prints a massage */
    vxDramIntConnect(DRAM_SINGLE_BIT, sdramErrShow, DRAM_SINGLE_BIT, 0);
    vxDramIntConnect(DRAM_DOUBLE_BIT, sdramErrShow, DRAM_DOUBLE_BIT, 0);
    vxDramIntConnect(DRAM_DPERR,      sdramErrShow, DRAM_DPERR,      0);

    /* Enable those bits */
    vxDramIntEnable(DRAM_SINGLE_BIT);
    vxDramIntEnable(DRAM_DOUBLE_BIT);
    vxDramIntEnable(DRAM_DPERR);

    vxDramIntCtrlInitialized = 1;
}

/*******************************************************************************
* vxDramIntConnect - connect a C routine to a specific DRAM error interrupt.
*
* DESCRIPTION:
*       This routine connects a specified user ISR to a specified integrates
*       DRAM error interrupt cause.
*       The ISR handler has its own user ISR array.
*       The connection is done by setting the desired routine and parameter in
*       the cause array (dramErrCauseArray[]):
*         1) Check for existing connection for the cause bit in the table.
*         2) Connecting the user ISR by inserting the given parameters into
*           an entry according to the user ISR given priority.
*
* INPUT:
*       cause     - DRAM error interrupt cause. See DRAM_ERR_CAUSE.
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
STATUS vxDramIntConnect(DRAM_ERR_CAUSE cause, VOIDFUNCPTR routine, 
                         int parameter, int prio)
{
    int i, sysIntOldConfig;

    /* Make sure that this is an atomic operation */
    sysIntOldConfig = intLock();

    /* Check the validity of the parameters */
    if(routine == NULL)
    {
        logMsg("\nvxDramIntConnect: NULL pointer routine\n",0,0,0,0,0,0);
        intUnlock(sysIntOldConfig);
        return ERROR;
    }
    else if(prio < 0)
    {
        logMsg("\nvxDramIntConnect: Invalid interrupt priority\n",0,0,0,0,0,0);
        intUnlock(sysIntOldConfig);
        return ERROR;
    }
    else if(DRAM_INVALID_CAUSE(cause))
    {
        logMsg("\nvxDramIntConnect: Invalid cause %d\n",cause,0,0,0,0,0);
        intUnlock(sysIntOldConfig);
        return ERROR;
    }

    /* Scan the table to check for reconnection to the same cause bit */
    for(i = dramErrCauseCount; i >= 0; i--)
        if(dramErrCauseArray[i].causeBit == MV_32BIT_LE(MV_BIT_MASK(cause)))
        {
            dramErrCauseArray[i].userISR  = routine;
            dramErrCauseArray[i].arg1     = parameter;
            
            intUnlock(sysIntOldConfig);
            return OK;
        }
    
    /* Connection phase */
    for(i = dramErrCauseCount; i >= 0; i--)
    {
        if(i == 0 || dramErrCauseArray[i-1].prio < prio) 
        {
            /* Make connection */
            dramErrCauseArray[i].causeBit = MV_32BIT_LE(MV_BIT_MASK(cause));
            dramErrCauseArray[i].userISR  = routine;
            dramErrCauseArray[i].arg1     = parameter;
            dramErrCauseArray[i].prio     = prio;
            dramErrCauseCount++;
            break;
        }
        else
        {
            /* Push the low priority connection down the table */
            dramErrCauseArray[i].causeBit = dramErrCauseArray[i-1].causeBit;
            dramErrCauseArray[i].userISR  = dramErrCauseArray[i-1].userISR;
            dramErrCauseArray[i].arg1     = dramErrCauseArray[i-1].arg1;
            dramErrCauseArray[i].prio     = dramErrCauseArray[i-1].prio;
        }
    }

    intUnlock(sysIntOldConfig);
    return OK;
}

/*******************************************************************************
* vxDramIntEnable - Enable a DRAM error interrupt
*
* DESCRIPTION:
*       This routine unmasks a specified DRAM error cause in 
*		the mask register. The routine will preform argument validity check.
*
* INPUT:
*       cause   - DRAM error cause as defined in DRAM_ERR_CAUSE.
*
* OUTPUT:
*       None.
*
* RETURN:
*       OK    - If the bit was set
*       ERROR - In case of invalid parameters.
*
*******************************************************************************/
STATUS vxDramIntEnable(DRAM_ERR_CAUSE cause)
{
    if (DRAM_INVALID_CAUSE(cause))
    {
        logMsg("\nvxDramIntEnable: Invalid cause %d\n", cause,0,0,0,0,0);
        return ERROR;
    }

    /* Set mask bit to enable interrupt */
	MV_REG_BIT_SET(SDRAM_ERROR_MASK_REG, MV_BIT_MASK(cause));
    
    return OK;
}

/*******************************************************************************
* vxDramIntDisable - Disable an DRAM error interrupt
*
* DESCRIPTION:
*       This routine masks a specified DRAM error interrupt in 
*		the mask register. The routine will preform argument validity check.
*
* INPUT:
*       cause   - DRAM error cause as defined in DRAM_ERR_CAUSE.
*
* OUTPUT:
*       None.
*
* RETURN:
*       OK    - If the bit was set
*       ERROR - In case of invalid parameters.
*
*******************************************************************************/
STATUS vxDramIntDisable(DRAM_ERR_CAUSE cause)
{
    if (DRAM_INVALID_CAUSE(cause))
    {
        logMsg("\nvxDramIntDisable: Invalid cause %d\n", cause,0,0,0,0,0);
        return ERROR;
    }

    /* Reset mask bit to disable interrupt */
	MV_REG_BIT_RESET(SDRAM_ERROR_MASK_REG, MV_BIT_MASK(cause));

    return OK;
}

/*******************************************************************************
* vxDramIntClear - Clear an DRAM error interrupt
*
* DESCRIPTION:
*       This routine clears a specified DRAM error interrupt in the 
*       cause register.
*       The routine will preform argument validity check.
*
* INPUT:
*       cause   - DRAM error cause as defined in DRAM_ERR_CAUSE.
*
* OUTPUT:
*       None.
*
* RETURN:
*       OK    - If the bit was set
*       ERROR - In case of invalid parameters.
*
*******************************************************************************/
STATUS vxDramIntClear(DRAM_ERR_CAUSE cause)
{
    if (DRAM_INVALID_CAUSE(cause))
    {
        logMsg("\nvxDramIntClear: Invalid cause %d\n", cause,0,0,0,0,0);
        return ERROR;
    }

    /* Reset mask bit to disable interrupt */
	MV_REG_WRITE(SDRAM_ERROR_CAUSE_REG, ~(MV_BIT_MASK(cause)));

    return OK;
}

/*******************************************************************************
* dramIntErrHandler - DRAM Error interrupt handler.
*
* DESCRIPTION:
*       This routine handles the DRAM error interrupts.
*       As soon as the interrupt signal is active the CPU analyzes the 
*       DRAM Interrupt Cause register in order to locate the 
*		originating interrupt event, after which a message is printed.
*       Then the routine calls the user specified service routine for that
*       interrupt cause.
*       The function scans the dramErrCauseArray[] (dramErrCauseCount 
*       valid entries) trying to find a hit in the dramErrCauseArray 
*       cause table. When found, the ISR in the same entry is executed.
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
void dramIntErrHandler (void)
{
    int i;
    UINT32 activeErr, dummy;

    /* Extract the pending error interrupts (In LE). 	*/
	activeErr = (MV_REG_READ(SDRAM_ERROR_CAUSE_REG) & 
				 MV_REG_READ(SDRAM_ERROR_MASK_REG));
            
    /* Execute user ISR if connected */
    for (i = 0; i < dramErrCauseCount; i++)
        if(activeErr & dramErrCauseArray[i].causeBit)
            (*dramErrCauseArray[i].userISR) (dramErrCauseArray[i].arg1);
    
    /* If multiple errors occur, only the first error is latched.           */
    /* New error report latching is only enabled after the SDRAM Error      */
    /* Address register is being read, and interrupt is cleared. Only after */
    /* this acknowledge, new interrupts could be latched assuming ISR has   */
    /* read address register                                                */
    dummy  = MV_REG_READ(SDRAM_ERROR_ADDR_REG);
    
    MV_MEMIO32_WRITE((INTER_REGS_BASE | SDRAM_ERROR_CAUSE_REG), ~(activeErr));

	return;
}


/*******************************************************************************
* sdramErrShow - Display error.
*
* DESCRIPTION:
*       This routine display DRAM error according to error report registers.
*
* INPUT:
*       causeBit - cause bit number.
*
* OUTPUT:
*       None.
*
* RETURN:
*       None.
*
*******************************************************************************/
LOCAL void sdramErrShow(UINT32 causeBit)
{
    UINT32 errType;
	UINT32 eccErrCnt, dramCsNum;
    UINT32 errAddr;
    UINT32 errDataLow, errDataHigh;
    UINT32 errEcc, calcEcc;

	logMsg("DRAM Error.\n", 0,0,0,0,0,0);

	switch (causeBit)
	{
		case(DRAM_SINGLE_BIT):
            logMsg("Single bit ECC error detected.\n",0,0,0,0,0,0);
			break;
		case(DRAM_DOUBLE_BIT):
            logMsg("Double bit ECC error detected.\n",0,0,0,0,0,0);
			break;
        case(DRAM_DPERR):
            logMsg("Dunit internal data path parity error.\n",0,0,0,0,0,0);
            return;
    }
	
    errAddr = MV_REG_READ(SDRAM_ERROR_ADDR_REG);

	/* Extract the error type out of the error high address register.		*/
    errType   = (errAddr & SEAR_ERR_TYPE_MASK) >> SEAR_ERR_TYPE_OFFS;
	
    /* Print error information if the data latched belongs to this cause */
    if (causeBit == errType)
    {
        errDataHigh  = MV_REG_READ(SDRAM_ERROR_DATA_HIGH_REG);
        errDataLow   = MV_REG_READ(SDRAM_ERROR_DATA_LOW_REG);
        errEcc       = MV_REG_READ(SDRAM_ERROR_ECC_REG);
	    calcEcc      = MV_REG_READ(SDRAM_CALC_ECC_REG);
        
        dramCsNum = (errAddr & SEAR_ERR_CS_MASK) >> SEAR_ERR_CS_OFFS;
        
        errAddr &= SEAR_ERR_ADDR_MASK;

        if (errType == DRAM_SINGLE_BIT)
		{
			eccErrCnt = MV_REG_READ(SDRAM_SINGLE_BIT_ERR_CNTR_REG);
		}
        else
		{
			eccErrCnt = MV_REG_READ(SDRAM_DOUBLE_BIT_ERR_CNTR_REG);
		}

	    logMsg("DRAM Error data     0x%08x.%08x\n", errDataHigh,errDataLow,0,0,0,0);
	    logMsg("DRAM Error address  0x%08x\n", errAddr,0,0,0,0,0);
        logMsg("DRAM Bank number    %d\n", dramCsNum,0,0,0,0,0);
        logMsg("DRAM Error number   %d\n", eccErrCnt,0,0,0,0,0);
        logMsg("DRAM detected ECC   0x%02x\n", errEcc,0,0,0,0,0);
	    logMsg("DRAM calculated ECC 0x%02x\n", calcEcc,0,0,0,0,0);
    }

    return;
}

#ifdef __cplusplus
}
#endif

