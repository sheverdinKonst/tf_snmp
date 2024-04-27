/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* simOsBindConsole.h
*
* DESCRIPTION:
*       Operating System wrapper. Console facility.
*
*       simOsGetCommandLine             SIM_OS_GET_COMMAND_LINE_FUN
*       simOsAllocConsole               SIM_OS_ALLOC_CONSOLE_FUN
*       simOsSetConsoleTitle            SIM_OS_SET_CONSOLE_TITLE_FUN
*
* FILE REVISION NUMBER:
*       $Revision: 1 $
*******************************************************************************/

#ifndef __simOsBindConsoleh
#define __simOsBindConsoleh

/************* Includes *******************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/************ Defines  ********************************************************/

/************ Functions *******************************************************/
/*******************************************************************************
* SIM_OS_GET_COMMAND_LINE_FUN
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
*       A pointer to the command line
*
* COMMENTS:
*       None
*
*******************************************************************************/
typedef char * (*SIM_OS_GET_COMMAND_LINE_FUN)
(
    void
);

/*******************************************************************************
* SIM_OS_ALLOC_CONSOLE_FUN
*
* DESCRIPTION:
*       Allocates a new console for the calling process.
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
typedef GT_STATUS (*SIM_OS_ALLOC_CONSOLE_FUN)
(
    void
);

/*******************************************************************************
* SIM_OS_SET_CONSOLE_TITLE_FUN
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
typedef GT_STATUS (*SIM_OS_SET_CONSOLE_TITLE_FUN)
(
    IN char *consoleTitlePtr
);

/* SIM_OS_FUNC_BIND_CONSOLE_STC -
*    structure that hold the "os console" functions needed be bound to SIM.
*
*/
typedef struct{
    SIM_OS_GET_COMMAND_LINE_FUN      simOsGetCommandLine;
    SIM_OS_ALLOC_CONSOLE_FUN         simOsAllocConsole;
    SIM_OS_SET_CONSOLE_TITLE_FUN     simOsSetConsoleTitle;
}SIM_OS_FUNC_BIND_CONSOLE_STC;

extern    SIM_OS_GET_COMMAND_LINE_FUN      SIM_OS_MAC(simOsGetCommandLine);
extern    SIM_OS_ALLOC_CONSOLE_FUN         SIM_OS_MAC(simOsAllocConsole);
extern    SIM_OS_SET_CONSOLE_TITLE_FUN     SIM_OS_MAC(simOsSetConsoleTitle);

#ifdef __cplusplus
}
#endif

#endif  /* __simOsBindConsoleh */

