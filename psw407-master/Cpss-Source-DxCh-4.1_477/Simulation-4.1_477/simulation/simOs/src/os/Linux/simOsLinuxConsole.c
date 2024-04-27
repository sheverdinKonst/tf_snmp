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
* simOsLinuxConsole.c
*
* DESCRIPTION:
*       Operating System wrapper. Ini file facility.
*
* DEPENDENCIES:
*       CPU independed.
*
* FILE REVISION NUMBER:
*       $Revision: 5 $
*******************************************************************************/

#include <os/simTypes.h>
#define EXPLICIT_INCLUDE_TO_SIM_OS_H_FILES
#include <os/simOsConsole.h>


/************* Externals *********************************************/
extern char *GetCommandLine(void);

/************* Defines ***********************************************/


/************ Public Functions ************************************************/
/*******************************************************************************
* simOsGetCommandLine
*
* DESCRIPTION:
*       Gets a command line.
*
* INPUTS:
*       None
*
* OUTPUTS:
*
* RETURNS:
*        A pointer to the command line
*
* COMMENTS:
*       None
*
*******************************************************************************/
char *simOsGetCommandLine
(
    void
)
{
    return  GetCommandLine();
}
/*******************************************************************************
* simOsAllocConsole
*
* DESCRIPTION:
*       Sets the config file name.
*

* INPUTS:
*       None
*
* OUTPUTS:
*
* RETURNS:
*       GT_OK   - if succeeds
*       GT_FAIL - if fails
*
* COMMENTS:
*       None
*
*******************************************************************************/
GT_STATUS simOsAllocConsole
(
    void
)
{
    return GT_OK;
}
/*******************************************************************************
* simOsSetConsoleTitle
*
* DESCRIPTION:
*       Displays the string in the title bar of the console window.
*
* INPUTS:
*       consoleTitlePtr - pointer to the title string
*
* OUTPUTS:
*
* RETURNS:
*       GT_OK   - if succeeds
*       GT_FAIL - if fails
*
* COMMENTS:
*       None
*
*******************************************************************************/
GT_STATUS simOsSetConsoleTitle
(
    IN char *consoleTitlePtr
)
{
    return GT_OK;
}



