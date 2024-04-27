/*******************************************************************************
*                   Copyright 2003, Marvell Semiconductor Israel LTD.          *
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
* (MJKK), MARVELL SEMICONDUCTOR ISRAEL LTD (MSIL).                             *
********************************************************************************
* sysPciIntCtrl.c - System PCI Interrupt controller library
*
* DESCRIPTION:
*       This file implements board dependent PCI interrupt controller library.
*       The PCI interrupt conectivity is board dependent.
*
* DEPENDENCIES:
*       None.
*
*******************************************************************************/

#include "mvBoardEnvLib.h"
#include "vxGppIntCtrl.h"
#include "vxPexIntCtrl.h"
#include "vxPciIntCtrl.h"
#include "mvPciIf.h"
#include "config.h"

#undef MV_DEBUG
#ifdef MV_DEBUG
    #define DB(x)   x
#else
    #define DB(x)
#endif

typedef enum _mvPciIntPin
{
    MV_PCI_INTA = 1,
    MV_PCI_INTB = 2,
    MV_PCI_INTC = 3,
    MV_PCI_INTD = 4
} MV_PCI_INT_PIN;

/*******************************************************************************
 * Defines
 */
#define IS_IRQ_FROM_PEX(pexIf, irqValue)					\
        (INT_LVL_PEX(pexIf, MV_PCI_INTA) <= irqValue) && 	\
        (INT_LVL_PEX(pexIf, MV_PCI_INTD) >= irqValue)
#if defined(MV646xx)
#define PEX_IF_FROM_IRQ(irqValue)	\
        ((irqValue > INT_LVL_PEX0_INTD )? (1+((irqValue-INT_LVL_PEX1_INTA)/4)):0)
#elif defined(MV78XX0)
#define PEX_IF_FROM_IRQ(irqValue)	\
        ((irqValue > INT_LVL_PEX0_INTD )? (1+((irqValue-INT_LVL_PEX1_INTA)/4)):0)
#elif defined(MV645xx)
#define PEX_IF_FROM_IRQ(irqValue)	0
#elif defined(MV88F6XXX)
#define PEX_IF_FROM_IRQ(irqValue)	0
#endif

/*******************************************************************************
* sysPciIntCtrlInit - Initiating the PCI Interrupt Controller driver.
*
* DESCRIPTION:
*       This function initialize the PCI interurpt controllers.
*
* INPUT:
*       None.
*
* OUTPUT:
*       Either GPIO or EGPIO interrupt controller is initiated.
*
* RETURN:
*       None.
*
*******************************************************************************/
void sysPciIntCtrlInit(void)
{
    /* Connect handler to PEX0.0 int A,B,C,D summary bit in main cause register */
    intConnect(INUM_TO_IVEC(INT_LVL_PEX00_ABCD) , pexIntHandler, 0);
    /* Unmask those bits */
    intEnable(INT_LVL_PEX00_ABCD);
}

/*******************************************************************************
* sysPciIntConnect - connect a C routine to a specific PCI A,B,C,D interrupt.
*
* DESCRIPTION:
*       This routine connects a specified ISR to a specified PCI interrupt
*       cause. This routine connects the user ISR to the E/GPIO Interrupt
*       Controller according to the given interrupt level.
*
* INPUT:
*       irqValue  - E/GPIO interrupt level. Describe the number of bit in
*                   E/GPIO cause register describing a PCI event.
*       routine   - user ISR.
*       parameter - user ISR parameter.
*
* OUTPUT:
*       The user ISR is connected to the E/GPIO Interrupt Controller.
*
* RETURN:
*       The E/GPIO interrupt connect routine return value.
*
*******************************************************************************/
STATUS sysPciIntConnect(int irqValue, VOIDFUNCPTR routine, int parameter)
{
    STATUS retVal = ERROR;
    UINT intPin; /* A=0, B=1, C=2, D=3 */
    int pexIf;

    /* Find out which PCI the irqValue belongs to */
    pexIf = PEX_IF_FROM_IRQ(irqValue);
    DB(mvOsPrintf("sysPciIntConnect(%d):    PEX_IF_FROM_IRQ %d,    INT_LVL_PEX(pexIf, MV_PCI_INTA)= %d \n",
                irqValue,pexIf, INT_LVL_PEX(pexIf, MV_PCI_INTA)));
    if(IS_IRQ_FROM_PEX(pexIf, irqValue))
    {
        /* The irqValue is from PEX 0. Extract the cause bit. */
        intPin = irqValue - INT_LVL_PEX(pexIf, MV_PCI_INTA);
        DB(mvOsPrintf("sysPciIntConnect(%d):IS_IRQ_FROM_PEX %d,   intPin=%d\n",irqValue,pexIf,intPin));

        /* Connect the PEX interrupt to PEX interrupt controller */
        retVal = vxPexIntConnect(pexIf, PEX_RCV_INT(intPin), routine, parameter, 0);
    }
    else
    {
        /* Connect all interrupts as if they were all from int B. This is   */
        /* because all PCI interrupts are shared on the same line           */
        intPin = (irqValue-INT_LVL_PCI);
        retVal = vxGppIntConnect((intPin % 32),(intPin / 32), routine, parameter, 0);
    }

    return retVal;
}

/*******************************************************************************
* sysPciIntEnable - Enable a System PCI interrupt
*
* DESCRIPTION:
*       This routine makes a specified PCI interrupt available to the CPU.
*
* INPUT:
*       irqValue  - E/GPIO interrupt level. Describe the number of bit in
*                   E/GPIO cause register describing a PCI event.
*
* OUTPUT:
*       The E/GPIO Interrupt Controller is invoke to enable the apropreate
*       interrupt.
*
* RETURN:
*       The E/GPIO interrupt enable routine return value.
*
*******************************************************************************/
STATUS sysPciIntEnable(int irqValue)
{
    STATUS retVal = ERROR;
    UINT intPin;	/* A=0, B=1, C=2, D=3 */
    int pexIf;

    DB(mvOsPrintf("sysPciIntEnable: irqValue=%d\n",irqValue));

    /* Find out which PCI the irqValue belongs to */
    pexIf = PEX_IF_FROM_IRQ(irqValue);

    DB(mvOsPrintf("sysPciIntEnable(%d):    PEX_IF_FROM_IRQ %d,
                  INT_LVL_PEX(pexIf, MV_PCI_INTA)= %d \n",
                irqValue,pexIf, INT_LVL_PEX(pexIf, MV_PCI_INTA)));

    if(IS_IRQ_FROM_PEX(pexIf, irqValue))
    {
        /* The irqValue is from PEX 0. Extract the cause bit. */
        intPin = irqValue - INT_LVL_PEX(pexIf, MV_PCI_INTA);

        DB(mvOsPrintf("sysPciIntEnable(%d):IS_IRQ_FROM_PEX %d,   intPin=%d\n",irqValue,pexIf,intPin));

        /* Connect the PEX interrupt to PEX interrupt controller */
        retVal = vxPexIntEnable(pexIf, PEX_RCV_INT(intPin));
    }
    else
    {
        intPin = (irqValue-INT_LVL_PCI);
        retVal = vxGppIntEnable((intPin % 32),(intPin / 32));

        DB(mvOsPrintf("sysPciIntEnable:vxGppIntEnable ,   retVal=%d\n",retVal));
    }

    return retVal;
}

/*******************************************************************************
* sysPciIntDisable - Disable a System PCI interrupt
*
* DESCRIPTION:
*       This routine makes a specified PCI interrupt unavailable to the CPU.
*
* INPUT:
*       irqValue  - E/GPIO interrupt level. Describe the number of bit in
*                   E/GPIO cause register describing a PCI event.
*
* OUTPUT:
*       The E/GPIO Interrupt Controller is invoke to disable the apropreate
*       interrupt.
*
* RETURN:
*       The E/GPIO interrupt disable routine return value.
*
*******************************************************************************/
MV_STATUS sysPciIntDisable(int irqValue)
{
    STATUS retVal = ERROR;
    UINT intPin;	/* A=0, B=1, C=2, D=3 */
    int pexIf;

    /* Find out which PCI the irqValue belongs to */
    pexIf = PEX_IF_FROM_IRQ(irqValue);
    if(IS_IRQ_FROM_PEX(pexIf, irqValue))
    {
        /* The irqValue is from PEX 0. Extract the cause bit. */
        intPin = irqValue - INT_LVL_PEX(pexIf, MV_PCI_INTA);

        /* Connect the PEX interrupt to PEX interrupt controller */
        retVal = vxPexIntDisable(pexIf, PEX_RCV_INT(intPin));
    }
    else
    {
        intPin = (irqValue-INT_LVL_PCI);
        retVal = vxGppIntDisable((intPin % 32),(intPin / 32));
    }

    return retVal;
}

