
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
* vxPexIntCtrl.c - PEX Error interrupt controller library
*
* DESCRIPTION:
*       This driver provides various interface routines to manipulate and
*       connect the hardware interrupts concerning the PEX interface.
*
*       Main features:
*        - The controller provides an easy way to hook a C Interrupt Service
*          Routine (ISR) to a specific error events caused by the PEX.
*        - The controller interrupt mechanism provides a way for the programmer
*          to set the priority of an interrupt.
*        - Full interrupt control over the PEX interface error events.
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
*       All PEX interface various error interrupt causes are numbered using the 
*       PEX_CAUSE enumerator. 
*
*       The driver's execution flow has three phases:
*        1) Driver initialization. This initiation includes hooking driver's
*           handlers to the main Interrupt controller. Composed 
*           of vxPexIntCtrlInit() routine.
*        2) User ISR connecting. Here information about user ISR and interrupt
*           priority is gathered. Composed of vxPexIntConnect() routine.
*        3) Interrupt handler. Here interrupts are being handle by the
*           Interrupt Handler. Composed of PexIntHandler(),
*
*       Full API:
*         vxPexIntCtrlInit() - Initiate the PEX interrupt controller driver.
*         vxPexIntConnect()  - Connect a user ISR to a PEX error event.
*         vxPexIntEnable()   - Enable a given PEX error interrupt cause.
*         vxPexIntDisable()  - Disable a given PEX error interrupt cause.
*         pexIntHandler()    - Handles PEX user hooked ISR to int A, B, C, D.
*         pexIntErrHandler() - Handles PEX user hooked ISR to error events.
*
*       The controller concept is very simple:
*       Each PEX interface has two ISR tables, one for the INT A, B, C, and D
*		and one for the PEX error events. Those tables holds the user connected 
*		ISR information per event. An error interrupt generated by the PEX
*       will result a search through the error table in order to allocate
*       the generating interrupt cause (done in pexIntErrHandler), while an 
*		INT A, B, C or D generated by the PEX unit will result a search 
*		through the error table in order to allocate the generating interrupt 
*		cause (done in pexIntHandler).
*       After the initiating interrupt cause is identify, the ISR reside in
*       the same table entry is executed.
*
*       The controller interface also includes interrupt control routines which
*       can enable/disable specific interrupts: vxPexIntDisable() and
*       vxPexIntEnable().
*
* DEPENDENCIES:
*       PEX interface module.
*       VxWorks types and interrupt lib.
*
*******************************************************************************/

/* includes */
#include "VxWorks.h"
#include "config.h"

#include "mvPex.h"
#include "mvCtrlEnvLib.h"
#include "vxPexIntCtrl.h"
#include "logLib.h"

#ifdef __cplusplus
extern "C" {
#endif

/* defines  */

#ifdef DEBUG
	#define DB(x)	x
#else
	#define DB(x)
#endif	

#define PEX_INVALID_CAUSE(cause)    \
            ((cause) <= PEX_CAUSE_START || PEX_CAUSE_END <= (cause))
#define PEX_INT_ABCD_CAUSE(cause)	\
			((PEX_RCV_INT_A <= cause) && (cause <=PEX_RCV_INT_D))
#define PEX_INT_ERR_CAUSE(cause)	\
			((PEX_CAUSE_START <= cause) && (cause <=PEX_PEX_LINK_FAIL))

/* typedefs */
typedef struct _mvIsrEntryTable 
{
    MV_ISR_ENTRY *pexCauseArray;
	MV_ISR_ENTRY *pexErrCauseArray; 
	int  pexCauseCount;
	int  pexErrCauseCount;   
} MV_ISR_ENTRY_TABLE;


/* locals   */
LOCAL MV_ISR_ENTRY pex0CauseArray[PEX_CAUSE_END];  
LOCAL MV_ISR_ENTRY pex1CauseArray[PEX_CAUSE_END];  
LOCAL MV_ISR_ENTRY pex2CauseArray[PEX_CAUSE_END];  
LOCAL MV_ISR_ENTRY pex3CauseArray[PEX_CAUSE_END];  
LOCAL MV_ISR_ENTRY pex4CauseArray[PEX_CAUSE_END];  
LOCAL MV_ISR_ENTRY pex5CauseArray[PEX_CAUSE_END];  
LOCAL MV_ISR_ENTRY pex6CauseArray[PEX_CAUSE_END];  
LOCAL MV_ISR_ENTRY pex7CauseArray[PEX_CAUSE_END];  
LOCAL MV_ISR_ENTRY pex8CauseArray[PEX_CAUSE_END];  
LOCAL MV_ISR_ENTRY pex0ErrCauseArray[PEX_CAUSE_END];  
LOCAL MV_ISR_ENTRY pex1ErrCauseArray[PEX_CAUSE_END];  
LOCAL MV_ISR_ENTRY pex2ErrCauseArray[PEX_CAUSE_END];  
LOCAL MV_ISR_ENTRY pex3ErrCauseArray[PEX_CAUSE_END];  
LOCAL MV_ISR_ENTRY pex4ErrCauseArray[PEX_CAUSE_END];  
LOCAL MV_ISR_ENTRY pex5ErrCauseArray[PEX_CAUSE_END];  
LOCAL MV_ISR_ENTRY pex6ErrCauseArray[PEX_CAUSE_END];  
LOCAL MV_ISR_ENTRY pex7ErrCauseArray[PEX_CAUSE_END];  
LOCAL MV_ISR_ENTRY pex8ErrCauseArray[PEX_CAUSE_END];  
MV_ISR_ENTRY_TABLE pexCauseArray_TABLE[]=
{
	{pex0CauseArray,pex0ErrCauseArray,0,0},
	{pex1CauseArray,pex1ErrCauseArray,0,0},
	{pex2CauseArray,pex2ErrCauseArray,0,0},
	{pex3CauseArray,pex3ErrCauseArray,0,0},
	{pex4CauseArray,pex4ErrCauseArray,0,0},
	{pex5CauseArray,pex5ErrCauseArray,0,0},
	{pex6CauseArray,pex6ErrCauseArray,0,0},
	{pex7CauseArray,pex7ErrCauseArray,0,0},
	{pex8CauseArray,pex8ErrCauseArray,0,0},
};

MV_STATUS pci0IsQuadby1, pci1IsQuadby1;
MV_STATUS pciIsPowerOn[MV_PEX_MAX_IF];

void pexErrShow(UINT32 pexIf, UINT32 causeBit);
LOCAL void pexRcvErrCorrShow(UINT32 pexIf);
void pexRcvErrFtalShow (UINT32 pexIf);
void pexRcvErrNFtalShow(UINT32 pexIf);


/*******************************************************************************
* vxPexIntCtrlInit - Initiating the PEX error Interrupt Controller driver.
*
* DESCRIPTION:
*       This routines clears the PEX cause and mask register. It also connect 
*		the default error handles that only prints a message and enables it.
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
void vxPexIntCtrlInit(void)
{
    int pexIf;

	pci0IsQuadby1 = PCI0_IS_QUAD_X1;
	pci1IsQuadby1 = PCI1_IS_QUAD_X1;

    for (pexIf = 0; pexIf < mvCtrlPexMaxIfGet(); pexIf++)
    {
		pciIsPowerOn[pexIf] = MV_FALSE;
		if (mvCtrlPwrClckGet(PEX_UNIT_ID, pexIf) == MV_FALSE)
				continue;
        MV_REG_WRITE(PEX_CAUSE_REG(pexIf), 0);      /* Clean Cause register */
	    MV_REG_WRITE(PEX_MASK_REG(pexIf), 0);       /* Clean Mask register  */
		
		/* Connect the default error handles that only prints a message */
        vxPexIntConnect(pexIf, PEX_M_DIS            , pexErrShow, PEX_M_DIS            , 5);
        vxPexIntConnect(pexIf, PEX_ERR_WR_TO_REG    , pexErrShow, PEX_ERR_WR_TO_REG    , 5);
        vxPexIntConnect(pexIf, PEX_HIT_DFLT_WIN_ERR , pexErrShow, PEX_HIT_DFLT_WIN_ERR , 5);
        vxPexIntConnect(pexIf, PEX_COR_ERR_DET      , pexErrShow, PEX_COR_ERR_DET      , 5);
        vxPexIntConnect(pexIf, PEX_N_F_ERR_DET      , pexErrShow, PEX_N_F_ERR_DET      , 5);
        vxPexIntConnect(pexIf, PEX_F_ERR_DET        , pexErrShow, PEX_F_ERR_DET        , 5);
        vxPexIntConnect(pexIf, PEX_D_STATE_CHANGE   , pexErrShow, PEX_D_STATE_CHANGE   , 5);
        vxPexIntConnect(pexIf, PEX_BIST             , pexErrShow, PEX_BIST             , 5);
        vxPexIntConnect(pexIf, PEX_RCV_ERR_FATAL    , pexErrShow, PEX_RCV_ERR_FATAL    , 5);
        vxPexIntConnect(pexIf, PEX_RCV_ERR_NON_FATAL, pexErrShow, PEX_RCV_ERR_NON_FATAL, 5);
        vxPexIntConnect(pexIf, PEX_RCV_ERR_COR      , pexErrShow, PEX_RCV_ERR_COR      , 5);
        vxPexIntConnect(pexIf, PEX_RCV_CRS          , pexErrShow, PEX_RCV_CRS          , 5);
        vxPexIntConnect(pexIf, PEX_PEX_SLV_HOT_RESET, pexErrShow, PEX_PEX_SLV_HOT_RESET, 5);
        vxPexIntConnect(pexIf, PEX_PEX_SLV_DIS_LINK , pexErrShow, PEX_PEX_SLV_DIS_LINK , 5);
        vxPexIntConnect(pexIf, PEX_PEX_SLV_LB       , pexErrShow, PEX_PEX_SLV_LB       , 5);
        vxPexIntConnect(pexIf, PEX_PEX_LINK_FAIL    , pexErrShow, PEX_PEX_LINK_FAIL    , 5);

        /* Enable those interrupts */
        vxPexIntEnable(pexIf, PEX_M_DIS            );
        vxPexIntEnable(pexIf, PEX_ERR_WR_TO_REG    );
        vxPexIntEnable(pexIf, PEX_HIT_DFLT_WIN_ERR );
        vxPexIntEnable(pexIf, PEX_COR_ERR_DET      );
        vxPexIntEnable(pexIf, PEX_N_F_ERR_DET      );
        vxPexIntEnable(pexIf, PEX_F_ERR_DET        );
        vxPexIntEnable(pexIf, PEX_D_STATE_CHANGE   );
        vxPexIntEnable(pexIf, PEX_BIST             );
        vxPexIntEnable(pexIf, PEX_RCV_ERR_FATAL    );
        vxPexIntEnable(pexIf, PEX_RCV_ERR_NON_FATAL);
        vxPexIntEnable(pexIf, PEX_RCV_ERR_COR      );
        vxPexIntEnable(pexIf, PEX_RCV_CRS          );
        vxPexIntEnable(pexIf, PEX_PEX_SLV_HOT_RESET);
        vxPexIntEnable(pexIf, PEX_PEX_SLV_DIS_LINK );
        vxPexIntEnable(pexIf, PEX_PEX_SLV_LB       );
        vxPexIntEnable(pexIf, PEX_PEX_LINK_FAIL    );
    }

	return;
}

/*******************************************************************************
* vxPexIntCtrlInit2 - Initiating the PEX power up stat 
*
* DESCRIPTION:
*       This routines set PEX power up for improve interrupt performance 
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
void vxPexIntCtrlInit2(void)
{
    int pexIf;
    for (pexIf = 0; pexIf < mvCtrlPexMaxIfGet(); pexIf++)
		pciIsPowerOn[pexIf] = mvPexIsPowerUp(pexIf);
}
/*******************************************************************************
* vxPexIntConnect - connect a C routine to a specific PEX error interrupt.
*
* DESCRIPTION:
*       This routine connects a specified user ISR to a specified PEX
*       interrupt cause.
*		This routine handles both error and INT A,B,C,D events. 
*       User ISR connection is done by setting the desired routine and its 
*		parameter in the correct table (pexCauseArray[] or pexErrCauseArray[]).
*		NOTE: In case of repeated connection to the same cause, its priority
*			  will be the same as the last one.
*
* INPUT:
*       pexIf     - PEX interface number.
*       cause     - PEX error interrupt cause. See PEX_CAUSE.
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
STATUS vxPexIntConnect(int pexIf, PEX_CAUSE cause, VOIDFUNCPTR routine, 
                         int parameter, int prio)
{
    MV_ISR_ENTRY *intCauseArray = NULL; 
    int *pIntConnectCount = 0;
    int i, sysIntOldConfig;

    /* Make sure that this is an atomic operation */
    sysIntOldConfig = intLock();

    /* Check the validity of the parameters */
    if(routine == NULL)
    {
        printf("\nvxPexIntConnect: NULL pointer routine\n");
        intUnlock(sysIntOldConfig);
        return ERROR;
    }
    else if(prio < 0)
    {
        printf("\nvxPexIntConnect: Invalid interrupt priority\n");
        intUnlock(sysIntOldConfig);
        return ERROR;
    }

	if (PEX_INT_ABCD_CAUSE(cause))
	{
		intCauseArray 	 = pexCauseArray_TABLE[pexIf].pexCauseArray;
		pIntConnectCount = &pexCauseArray_TABLE[pexIf].pexCauseCount;
	}
	else if (PEX_INT_ERR_CAUSE(cause))
	{
		intCauseArray 	 = pexCauseArray_TABLE[pexIf].pexErrCauseArray;
		pIntConnectCount = &pexCauseArray_TABLE[pexIf].pexErrCauseCount;
	}
	else
	{
		printf("\nvxPexIntConnect: Invalid cause %d\n",cause);
        intUnlock(sysIntOldConfig);
        return ERROR;
    }
    
	/* Scan the table to check for reconnection to the same cause bit */
    for(i = *pIntConnectCount; i >= 0; i--)
        if(intCauseArray[i].causeBit == MV_BIT_MASK(cause))
        {
            /* Note: priority is unchanged */
			intCauseArray[i].userISR  = routine;
            intCauseArray[i].arg1     = parameter;
            intUnlock(sysIntOldConfig);
            return OK;
        }
    
    /* Connection phase */
    for(i = *pIntConnectCount; i >= 0; i--)
    {
        if(i == 0 || intCauseArray[i-1].prio < prio) 
        {
            /* Make connection */
            intCauseArray[i].causeBit = MV_BIT_MASK(cause);
            intCauseArray[i].userISR  = routine;
            intCauseArray[i].arg1     = parameter;
            intCauseArray[i].prio     = prio;
            (*pIntConnectCount)++;
            break;
        }
        else
        {
            /* Push the low priority connection down the table */
            intCauseArray[i].causeBit = intCauseArray[i-1].causeBit;
            intCauseArray[i].userISR  = intCauseArray[i-1].userISR;
            intCauseArray[i].arg1     = intCauseArray[i-1].arg1;
            intCauseArray[i].prio     = intCauseArray[i-1].prio;
        }
    }

    intUnlock(sysIntOldConfig);
    return OK;
}

/*******************************************************************************
* vxPexIntEnable - Enable a PEX interrupt
*
* DESCRIPTION:
*       This routine unmasks a specified PEX cause in the mask register.
*       The routine will preform argument validity check.
*
* INPUT:
*       pexIf    - PEX interface number.
*       cause    - PEX error interrupt cause as defined in PEX_CAUSE.
*
* OUTPUT:
*       None.
*
* RETURN:
*       OK    - If the bit was set
*       ERROR - In case of invalid parameters.
*
*******************************************************************************/
STATUS vxPexIntEnable(int pexIf, PEX_CAUSE cause)
{
	/* Parameter checking  */
	if (pexIf >= mvCtrlPexMaxIfGet())
	{
        printf("\nvxPexIntEnable: Invalid PEX interface %d\n", pexIf);
		return ERROR;
	}

    if (PEX_INVALID_CAUSE(cause))
    {
        printf("\nvxPexIntEnable: Invalid cause %d\n", cause);
        return ERROR;
    }
    
	/* Set mask bit to enable interrupt */
	MV_REG_BIT_SET(PEX_MASK_REG(pexIf), MV_BIT_MASK(cause));

    return OK;
}

/*******************************************************************************
* vxPexIntDisable - Disable a PEX interrupt
*
* DESCRIPTION:
*       This routine masks a specified PEX cause in the mask register.
*       The routine will preform argument validity check.
*
* INPUT:
*       pexIf    - PEX interface number.
*       cause    - PEX error interrupt cause as defined in PEX_CAUSE.
*
* OUTPUT:
*       None.
*
* RETURN:
*       OK    - If the bit was set
*       ERROR - In case of invalid parameters.
*
*******************************************************************************/
STATUS vxPexIntDisable(int pexIf, PEX_CAUSE cause)
{
	/* Parameter checking  */
	if (pexIf >= mvCtrlPexMaxIfGet())
	{
        printf("\vxPexIntDisable: Invalid PEX interface %d\n", pexIf);
		return ERROR;
	}

    if (PEX_INVALID_CAUSE(cause))
    {
        printf("\vxPexIntDisable: Invalid cause %d\n", cause);
        return ERROR;
    }

    /* Set mask bit to enable interrupt */
	MV_REG_BIT_RESET(PEX_MASK_REG(pexIf), MV_BIT_MASK(cause));

    return OK;
}

/*******************************************************************************
* vxPexIntClear - Clear a PEX interrupt
*
* DESCRIPTION:
*       This routine clears a specified PEX interrupt in the cause register.
*       The routine will preform argument validity check.
*
* INPUT:
*       pexIf - PEX interface number.
*       cause - PEX error interrupt cause as defined in PEX_CAUSE.
*
* OUTPUT:
*       None.
*
* RETURN:
*       OK    - If the bit was set
*       ERROR - In case of invalid parameters.
*
*******************************************************************************/
STATUS vxPexIntClear(int pexIf, PEX_CAUSE cause)
{
	/* Parameter checking  */
	if (pexIf >= mvCtrlPexMaxIfGet())
	{
        printf("\vxPexIntClear: Invalid PEX interface %d\n", pexIf);
		return ERROR;
	}

    if (PEX_INVALID_CAUSE(cause))
    {
        printf("\nvxPexIntClear: Invalid cause %d\n", cause);
        return ERROR;
    }

    /* Reset mask bit to disable interrupt */
	MV_REG_WRITE(PEX_CAUSE_REG(pexIf), ~(MV_BIT_MASK(cause)));

    return OK;
}

/*******************************************************************************
* pexIntHandler - PEX interrupt handler.
*
* DESCRIPTION:
*       This routine handles the PEX INT A,B,C,D interrupts.
*       As soon as the interrupt signal is active (as a result of INT A,B,C,D), 
*		the CPU analyzes the PEX Cause register (according to PEX interface 
*		number) in order to locate the originating interrupt event.
*       Then the routine calls the user specified service routine for that
*       interrupt cause.
*       The function scans the pexCauseArray[] (pexCauseCount valid entries) 
*		trying to find a hit in the pexxCauseArray cause table. When found, 
*		the ISR in the same entry is executed.
*		Note: The handler automatically acknowledges the generating interrupts.
*
* INPUT:
*       pexIf - PEX interface number.
*
* OUTPUT:
*       None.
*
* RETURN:
*       None.
*
*******************************************************************************/
void pexIntHandler (int pexIf)
{
    int i;
    MV_ISR_ENTRY *intCauseArray; /* pointer to the selected cause table     */
    int *pIntConnectCount;       /* pointer to the table connection counter */

    UINT32 activeErr;

    /* First execute user service routine connected to PEX events */
	intCauseArray 	 = pexCauseArray_TABLE[pexIf].pexCauseArray;
    pIntConnectCount = &pexCauseArray_TABLE[pexIf].pexCauseCount;

    /* Extract the pending error interrupts (in LE). Ignore Error events	*/
    activeErr = (MV_REG_READ(PEX_CAUSE_REG(pexIf)) & 
                 MV_REG_READ(PEX_MASK_REG(pexIf))) & PEX_INT_ABCD_MASK;

    /* Execute int A, B, C, or D ISRs if connected */
    for(i = 0; (i < *pIntConnectCount); i++)
        if(activeErr & intCauseArray[i].causeBit)
      {
            (*intCauseArray[i].userISR) (intCauseArray[i].arg1);                        
        intCauseArray[i].counter++;
      }
	    
	/* Acknowledge pending interrupts to allow next interrupts to register  */
	MV_REG_WRITE((PEX_CAUSE_REG(pexIf)), ~(activeErr));

	return;
}
/*******************************************************************************
* pexIntErrHandler - PEX error interrupt handler.
*
* DESCRIPTION:
*       This routine handles the PEX error interrupts.
*       As soon as the interrupt signal is active as a result of PEX error, 
*		the CPU analyzes the PEX Cause register (according to PEX interface 
*		number) in order to locate the originating error event, after which a 
*       message is printed.
*       The function scans the pexErrCauseArray[] (pexCauseCount valid entries) 
*		trying to find a hit in the pexErrCauseArray cause table. When found, 
*		the ISR in the same entry is executed.
*		Note: The handler automatically acknowledges the generating interrupts.
*
* INPUT:
*       pexIf - PEX interface number (0-8).
*
* OUTPUT:
*       None.
*
* RETURN:
*       None.
*
*******************************************************************************/
void pexIntErrHandler2 (int pexIf)
{
    int i;
    MV_ISR_ENTRY *intCauseArray; /* pointer to the selected cause table     */
    int *pIntConnectCount;       /* pointer to the table connection counter */

    UINT32 activeErr;

    /* First execute user service routine connected to PEX events */
	intCauseArray 	 = pexCauseArray_TABLE[pexIf].pexErrCauseArray;
	pIntConnectCount = &pexCauseArray_TABLE[pexIf].pexErrCauseCount;
    /* Extract the pending error interrupts (in LE). Ignore INT A,B,C,D		*/
    activeErr = (MV_REG_READ(PEX_CAUSE_REG(pexIf)) & 
                 MV_REG_READ(PEX_MASK_REG(pexIf))) & ~PEX_INT_ABCD_MASK;	
	if (activeErr)
	{
		/* Execute Error user ISR if connected */
		for (i = 0; i < *pIntConnectCount; i++)
			if (activeErr & intCauseArray[i].causeBit)
				(*intCauseArray[i].userISR) (pexIf, intCauseArray[i].arg1);

			/* Acknowledge pending interrupts to allow next interrupts to register  */
		MV_REG_WRITE((PEX_CAUSE_REG(pexIf)), ~(activeErr));
	}
    return;
}
    
void pexIntErrHandler (int pexIf)
{
	int pciIf;
	if (0 == pexIf)
	{
		if (pci0IsQuadby1)
		{
			for (pciIf = 0; pciIf < 4; pciIf++)
			{
				if (pciIsPowerOn[pciIf])
				{
					pexIntErrHandler2(pciIf);
				}
			}
			return;
		}
		if (pciIsPowerOn[0])
			pexIntErrHandler2(0);
		return;
	}
	if (pci1IsQuadby1)
	{
		for (pciIf = 4; pciIf < 8; pciIf++)
		{
			if (pciIsPowerOn[pciIf])
				pexIntErrHandler2(pciIf);
		}
		return;
	}
	if (pciIsPowerOn[4])
		pexIntErrHandler2(4);
	return;
}
    
/*******************************************************************************
* pexErrShow - Show error information.
*
* DESCRIPTION:
*       This routine show PEX error information according to error report
*		registers.
*
* INPUT:
*       pexIf   - PEX interface number.
*       errType - Unit error type. Value read from register.
*
* OUTPUT:
*       None.
*
* RETURN:
*       None.
*
*******************************************************************************/
void pexErrShow(UINT32 pexIf, UINT32 causeBit)
{
	switch (causeBit)
	{
	case (PEX_TX_REQ_IN_DLDOWN_ERR):
		logMsg("PEX %d ERR. Receive TxReq while PEX is in Dl-Down " , pexIf,0,0,0,0,0);break;
	case(PEX_M_DIS):
		logMsg("PEX %d ERR. Generate PEX transaction while master is disabled\n", pexIf,0,0,0,0,0);break;
	case(PEX_ERR_WR_TO_REG):
		logMsg("PEX %d ERR. Erroneous write to PEX internal register\n", pexIf, 0,0,0,0,0);break;
	case(PEX_HIT_DFLT_WIN_ERR):
		logMsg("PEX %d ERR. PEX hit Default Window Error\n", pexIf,0,0,0,0,0); break;
	case(PEX_RX_RAM_PAR_ERR): 
		logMsg("PEX %d ERR. Rx RAM Parity Error\n", pexIf,0,0,0,0,0);break;
	case(PEX_TX_RAM_PAR_ERR): 
		logMsg("PEX %d ERR. Tx RAM Parity Error\n", pexIf,0,0,0,0,0);break;
	case(PEX_COR_ERR_DET):    
		pexRcvErrCorrShow(pexIf); 
		break;
	case(PEX_N_F_ERR_DET):
		logMsg("PEX %d ERR. Non-Fatal Error Detected\n", pexIf,0,0,0,0,0);
		pexRcvErrNFtalShow(pexIf);
		break;
	case(PEX_F_ERR_DET):
		logMsg("PEX %d ERR. Fatal Error Detected\n", pexIf,0,0,0,0,0);
		pexRcvErrFtalShow(pexIf);
		break;
	case(PEX_D_STATE_CHANGE):
		logMsg("PEX %d ERR. Endpoint Dstate Change Indication\n", 
			   pexIf,
			   MV_REG_READ( PEX_CFG_DIRECT_ACCESS(pexIf,PEX_POWER_MNG_STATUS_CONTROL)),
			   0,0,0,0);
		break; 
	case(PEX_BIST):
		logMsg("PEX %d ERR. BIST activated\n", pexIf,0,0,0,0,0);break;
	case (PEX_FLOW_CTRL_PROTOCOL):
		logMsg("PEX %d ERR. Flow Control Protocol Error.\n", pexIf,0,0,0,0,0);break;
	case (PEX_RCV_UR_CA_ERR):
		logMsg("PEX %d ERR. Received either UR or CA completion status.\n", pexIf,0,0,0,0,0);break;
	case(PEX_RCV_ERR_FATAL):
		logMsg("PEX %d ERR. Received ERR_FATAL message\n", pexIf,0,0,0,0,0);
		break;
	case(PEX_RCV_ERR_NON_FATAL):
		logMsg("PEX %d ERR. Received ERR_NONFATAL message\n", 
			   pexIf,0,0,0,0,0);
		break;
	case(PEX_RCV_ERR_COR):
		logMsg("PEX %d ERR. Received ERR_COR message\n", pexIf,0,0,0,0,0);
		break;
	case(PEX_RCV_CRS):
		logMsg("PEX %d ERR. Received CRS completion status\n", 
			   pexIf,0,0,0,0,0);
		break;                                                                            
	case(PEX_PEX_SLV_HOT_RESET):
		logMsg("PEX %d ERR. Received Hot Reset Indication\n", 
			   pexIf,0,0,0,0,0);
		break;
	case(PEX_PEX_SLV_DIS_LINK):
		logMsg("PEX %d ERR. Slave Disable Link Indication\n", 
			   pexIf,0,0,0,0,0);
		break;
	case(PEX_PEX_SLV_LB):
		logMsg("PEX %d ERR. Slave Loopback Indication\n", pexIf,0,0,0,0,0);break;
	case(PEX_PEX_LINK_FAIL):
		logMsg("PEX %d ERR. Link Failure indication\n", pexIf,0,0,0,0,0);break;
	case(PEX_RCV_Turn_Off):
		logMsg("PEX %d ERR. RcvTurnOffMsg: Receive Turn OFF MSG from the RC.\n", pexIf,0,0,0,0,0);break;
	case(PEX_RCV_MSI):
		logMsg("PEX %d ERR. RC receive MemWr that hit the MSI attributes.\n", pexIf,0,0,0,0,0);break;
	default:
		logMsg("PEX %d ERR. Unknown error 0x%x\n", pexIf, causeBit,0,0,0,0);
	}

	return;
}

/*******************************************************************************
* pexRcvErrCorrShow - Show Receive Error Correction status.
*
* DESCRIPTION:
*       This routine show PEX Receive Error Correction according to 
*		error report registers.
*
* INPUT:
*       pexIf   - PEX interface number.
*
* OUTPUT:
*       None.
*
* RETURN:
*       None.
*
*******************************************************************************/
LOCAL void pexRcvErrCorrShow(UINT32 pexIf)
{
	UINT32 activeErr;
	UINT32 RegOffset = PEX_CFG_DIRECT_ACCESS(pexIf, PEX_CORRECT_ERR_STAT_REG);
	/* Extract the pending error interrupts (in LE).	*/
	activeErr = MV_REG_READ(RegOffset);

	if (activeErr & MV_BIT_MASK(PEX_CORR_RCV_ERR))
	{
		MV_REG_WRITE(RegOffset,MV_BIT_MASK(PEX_CORR_RCV_ERR)); /* Clear interrupt */
	}
	else if (activeErr & MV_BIT_MASK(PEX_CORR_BAD_TLP_ERR))
	{
		logMsg("PEX %d ERR. Correctable Error: Bad TLP Status\n\n", 
			   pexIf,0,0,0,0,0);
		MV_REG_WRITE(RegOffset,MV_BIT_MASK(PEX_CORR_BAD_TLP_ERR)); /* Clear interrupt */
	}
	else if (activeErr & MV_BIT_MASK(PEX_CORR_BAD_DLLP_ERR))
	{
		logMsg("PEX %d ERR. Correctable Error: Bad DLLP Status\n\n", 
			   pexIf,0,0,0,0,0);
		MV_REG_WRITE(RegOffset,MV_BIT_MASK(PEX_CORR_BAD_DLLP_ERR));	/* Clear interrupt */
	}
	else if (activeErr & MV_BIT_MASK(PEX_CORR_RPLY_RLLOVR_ERR))
	{
		logMsg("PEX %d ERR. Correctable Error: Replay Number Rollover Status\n\n", 
			   pexIf,0,0,0,0,0);
		MV_REG_WRITE(RegOffset,MV_BIT_MASK(PEX_CORR_RPLY_RLLOVR_ERR));/* Clear interrupt*/
	}
	else if (activeErr & MV_BIT_MASK(PEX_CORR_RPLY_TO_ERR))
	{
		logMsg("PEX %d ERR. Correctable Error: Replay Timer Timeout status\n\n", 
			   pexIf,0,0,0,0,0);
		MV_REG_WRITE(RegOffset,MV_BIT_MASK(PEX_CORR_RPLY_TO_ERR)); /* Clear interrupt */
	}
	else if (activeErr & MV_BIT_MASK(PEX_CORE_NON_FATAL_ERR))
	{
		logMsg("PEX %d ERR. Correctable Error: Replay Advisory Non-Fatal Error status\n\n", 
			   pexIf,0,0,0,0,0);
		MV_REG_WRITE(RegOffset,MV_BIT_MASK(PEX_CORE_NON_FATAL_ERR)); /* Clear interrupt */
	}
	else
	{
		logMsg("PEX %d ERR. Correctable Error: Unknown error 0x%x\n", 
			   pexIf, activeErr,0,0,0,0);
	}

	return;
}
void pexRcvErrNFtalShow(UINT32 pexIf)
{
	UINT32 activeErr, SeverityMaskErr;
	UINT32 RegOffset = PEX_CFG_DIRECT_ACCESS(pexIf, PEX_UNCORRECT_ERR_STAT_REG);
	UINT32 SeverityOffset = PEX_CFG_DIRECT_ACCESS(pexIf, PEX_UNCORRECT_ERR_SERVITY_REG);
	/* Extract the pending error interrupts (in LE).	*/
	activeErr 		= MV_REG_READ(RegOffset);
	SeverityMaskErr =  MV_REG_READ(SeverityOffset);
	if (activeErr & MV_BIT_MASK(PEX_UNCORR_RCV_DLPRT_ERR) &&
		((SeverityMaskErr & MV_BIT_MASK(PEX_UNCORR_RCV_DLPRT_ERR)) == 0))
	{
		logMsg("PEX %d Non-Fatal ERR. Data Link Protocol Error Status\n\n", pexIf,0,0,0,0,0);  
		MV_REG_WRITE(RegOffset,MV_BIT_MASK(PEX_UNCORR_RCV_DLPRT_ERR)); /* Clear interrupt */
	}
	if (activeErr & MV_BIT_MASK(PEX_UNCORR_RPSN_TLP_ERR) &&
		((SeverityMaskErr & MV_BIT_MASK(PEX_UNCORR_RPSN_TLP_ERR)) == 0))
	{
		logMsg("PEX %d Non-Fatal ERR. Poisoned TLP Status\n\n", pexIf,0,0,0,0,0); 
		MV_REG_WRITE(RegOffset,MV_BIT_MASK(PEX_UNCORR_RPSN_TLP_ERR)); /* Clear interrupt */
	}
	if (activeErr & MV_BIT_MASK(PEX_UNCORR_CMP_TO_ERR) &&
		((SeverityMaskErr & MV_BIT_MASK(PEX_UNCORR_CMP_TO_ERR)) == 0))
	{
		logMsg("PEX %d Non-Fatal ERR. Completion Timeout Status\n\n", pexIf,0,0,0,0,0); 
		MV_REG_WRITE(RegOffset,MV_BIT_MASK(PEX_UNCORR_CMP_TO_ERR)); /* Clear interrupt */
	}
	if (activeErr & MV_BIT_MASK(PEX_UNCORR_CA_ERR) &&
		((SeverityMaskErr & MV_BIT_MASK(PEX_UNCORR_CA_ERR)) == 0))
	{
		logMsg("PEX %d Non-Fatal ERR. Completer Abort Status\n\n", pexIf,0,0,0,0,0); 
		MV_REG_WRITE(RegOffset,MV_BIT_MASK(PEX_UNCORR_CA_ERR)); /* Clear interrupt */
	}
	if (activeErr & MV_BIT_MASK(PEX_UNCORR_UN_EXP_CMP_ERR) &&
		((SeverityMaskErr & MV_BIT_MASK(PEX_UNCORR_UN_EXP_CMP_ERR)) == 0))
	{
		logMsg("PEX %d Non-Fatal ERR. Unexpected Completion Status\n\n", pexIf,0,0,0,0,0); 
		MV_REG_WRITE(RegOffset,MV_BIT_MASK(PEX_UNCORR_UN_EXP_CMP_ERR)); /* Clear interrupt */
	}
	if (activeErr & MV_BIT_MASK(PEX_UNCORR_MALF_TLP_ERR) &&
		((SeverityMaskErr & MV_BIT_MASK(PEX_UNCORR_MALF_TLP_ERR)) == 0))
	{
		logMsg("PEX %d Non-Fatal ERR. Malformed TLP Status   \n\n", pexIf,0,0,0,0,0); 
		MV_REG_WRITE(RegOffset,MV_BIT_MASK(PEX_UNCORR_MALF_TLP_ERR)); /* Clear interrupt */
	}
	if (activeErr & MV_BIT_MASK(PEX_UNCORR_UR_ERR) &&
		((SeverityMaskErr & MV_BIT_MASK(PEX_UNCORR_UR_ERR)) == 0))
	{
		logMsg("PEX %d Non-Fatal ERR. Unsupported Request Error Status\n\n", pexIf,0,0,0,0,0); 
		MV_REG_WRITE(RegOffset,MV_BIT_MASK(PEX_UNCORR_UR_ERR)); /* Clear interrupt */
	}
	return;
}
void pexRcvErrFtalShow(UINT32 pexIf)
{
	UINT32 activeErr, SeverityMaskErr;
	UINT32 RegOffset = PEX_CFG_DIRECT_ACCESS(pexIf, PEX_UNCORRECT_ERR_STAT_REG);
	UINT32 SeverityOffset = PEX_CFG_DIRECT_ACCESS(pexIf, PEX_UNCORRECT_ERR_SERVITY_REG);
	/* Extract the pending error interrupts (in LE).	*/
	activeErr 		= MV_REG_READ(RegOffset);
	SeverityMaskErr =  MV_REG_READ(SeverityOffset);
	if (activeErr & MV_BIT_MASK(PEX_UNCORR_RCV_DLPRT_ERR) &&
		(SeverityMaskErr & MV_BIT_MASK(PEX_UNCORR_RCV_DLPRT_ERR)))
	{
		logMsg("PEX %d Fatal ERR. Data Link Protocol Error Status\n\n", pexIf,0,0,0,0,0);  
		MV_REG_WRITE(RegOffset,MV_BIT_MASK(PEX_UNCORR_RCV_DLPRT_ERR)); /* Clear interrupt */
	}
	if (activeErr & MV_BIT_MASK(PEX_UNCORR_RPSN_TLP_ERR) &&
		(SeverityMaskErr & MV_BIT_MASK(PEX_UNCORR_RPSN_TLP_ERR)))
	{
		logMsg("PEX %d Fatal ERR. Poisoned TLP Status\n\n", pexIf,0,0,0,0,0); 
		MV_REG_WRITE(RegOffset,MV_BIT_MASK(PEX_UNCORR_RPSN_TLP_ERR)); /* Clear interrupt */
	}
	if (activeErr & MV_BIT_MASK(PEX_UNCORR_CMP_TO_ERR) &&
		(SeverityMaskErr & MV_BIT_MASK(PEX_UNCORR_CMP_TO_ERR)))
	{
		logMsg("PEX %d Fatal ERR. Completion Timeout Status\n\n", pexIf,0,0,0,0,0); 
		MV_REG_WRITE(RegOffset,MV_BIT_MASK(PEX_UNCORR_CMP_TO_ERR)); /* Clear interrupt */
	}
	if (activeErr & MV_BIT_MASK(PEX_UNCORR_CA_ERR) &&
		(SeverityMaskErr & MV_BIT_MASK(PEX_UNCORR_CA_ERR)))
	{
		logMsg("PEX %d Fatal ERR. Completer Abort Status\n\n", pexIf,0,0,0,0,0); 
		MV_REG_WRITE(RegOffset,MV_BIT_MASK(PEX_UNCORR_CA_ERR)); /* Clear interrupt */
	}
	if (activeErr & MV_BIT_MASK(PEX_UNCORR_UN_EXP_CMP_ERR) &&
		(SeverityMaskErr & MV_BIT_MASK(PEX_UNCORR_UN_EXP_CMP_ERR)))
	{
		logMsg("PEX %d Fatal ERR. Unexpected Completion Status\n\n", pexIf,0,0,0,0,0); 
		MV_REG_WRITE(RegOffset,MV_BIT_MASK(PEX_UNCORR_UN_EXP_CMP_ERR)); /* Clear interrupt */
	}
	if (activeErr & MV_BIT_MASK(PEX_UNCORR_MALF_TLP_ERR) &&
		(SeverityMaskErr & MV_BIT_MASK(PEX_UNCORR_MALF_TLP_ERR)))
	{
		logMsg("PEX %d Fatal ERR. Malformed TLP Status   \n\n", pexIf,0,0,0,0,0); 
		MV_REG_WRITE(RegOffset,MV_BIT_MASK(PEX_UNCORR_MALF_TLP_ERR)); /* Clear interrupt */
	}
	if (activeErr & MV_BIT_MASK(PEX_UNCORR_UR_ERR) &&
		(SeverityMaskErr & MV_BIT_MASK(PEX_UNCORR_UR_ERR)))
	{
		logMsg("PEX %d Fatal ERR. Unsupported Request Error Status\n\n", pexIf,0,0,0,0,0); 
		MV_REG_WRITE(RegOffset,MV_BIT_MASK(PEX_UNCORR_UR_ERR)); /* Clear interrupt */
	}
	return;
}

#ifdef __cplusplus
}
#endif

