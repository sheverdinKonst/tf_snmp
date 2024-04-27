/*******************************************************************************
*              Copyright 2001, GALILEO TECHNOLOGY, LTD.
*
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL. NO RIGHTS ARE GRANTED
* HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT OF MARVELL OR ANY THIRD
* PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE DISCRETION TO REQUEST THAT THIS
* CODE BE IMMEDIATELY RETURNED TO MARVELL. THIS CODE IS PROVIDED "AS IS".
* MARVELL MAKES NO WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS
* ACCURACY, COMPLETENESS OR PERFORMANCE. MARVELL COMPRISES MARVELL TECHNOLOGY
* GROUP LTD. (MTGL) AND ITS SUBSIDIARIES, MARVELL INTERNATIONAL LTD. (MIL),
* MARVELL TECHNOLOGY, INC. (MTI), MARVELL SEMICONDUCTOR, INC. (MSI), MARVELL
* ASIA PTE LTD. (MAPL), MARVELL JAPAN K.K. (MJKK), GALILEO TECHNOLOGY LTD. (GTL)
* AND GALILEO TECHNOLOGY, INC. (GTI).
********************************************************************************
* simOsLinuxIntr.c
*
* DESCRIPTION:
*       Linux Operating System Simulation. Task facility implementation.
*
* DEPENDENCIES:
*       CPU independed.
*
* FILE REVISION NUMBER:
*       $Revision: 5 $
*******************************************************************************/

#include <os/simTypes.h>
#define EXPLICIT_INCLUDE_TO_SIM_OS_H_FILES
#include <os/simOsIntr.h>
#include <gtOs/linuxStubs.h>

/************* Externals *********************************************/
extern UINT_32  SHOSTG_set_interrupt (UINT_32 intr);
extern void     SHOSTC_init_intr_table (void);
extern void     exit (int __status);
extern void     simOsTaskLibInit (void);

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
        return GT_FALSE;
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
    SHOSTC_init_intr_table();
}

