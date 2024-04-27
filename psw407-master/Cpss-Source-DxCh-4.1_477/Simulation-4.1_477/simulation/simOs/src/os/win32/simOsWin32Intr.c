/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* simOsWin32Task.c
*
* DESCRIPTION:
*       Win32 Operating System Simulation. Task facility implementation.
*
* DEPENDENCIES:
*       Win32, CPU independed.
*
* FILE REVISION NUMBER:
*       $Revision: 3 $
*******************************************************************************/

#include <os/simTypesBind.h>
#include <gen/intr/exp/intr.h>
/* calling this function will create the DPmutex , that needed for correct use
    inside the SHOST code */
extern void   SHOSTG_psos_pre_init(void);
/************* Defines ***********************************************/

/************ Public Functions ************************************************/

/*******************************************************************************
* simOsInteruptSet
*
* DESCRIPTION:
*       Create OS Task/Thread and start it.
*
* INPUTS:
*       intr    - interrupt
*
* OUTPUTS:
*
* RETURNS:
*       GT_OK   - Succeed
*       GT_FAIL - failed
*
* COMMENTS:
*       None
*
*******************************************************************************/
GT_STATUS simOsInterruptSet
(
    IN  GT_U32 intr
)
{

    if(SHOSTG_set_interrupt(intr))
        return GT_OK;
    else
        return GT_FAIL;
}

/*******************************************************************************
* simOsInitInterrupt
*
* DESCRIPTION:
*
*
* INPUTS:
*       None
*
* OUTPUTS:
*
* RETURNS:
*       None
*
* COMMENTS:
*       None
*
*******************************************************************************/
void simOsInitInterrupt
(
    void
)
{
    /* calling this function will create the DPmutex , that needed for correct use
       inside the SHOST code */
    SHOSTG_psos_pre_init();

    /* calling this function initialize the interrupts table (in the SHOST) */
    SHOSTC_init_intr_table();
}

