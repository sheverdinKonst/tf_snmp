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
*       Operating System wrapper. Ini file facility.
*
* DEPENDENCIES:
*       Win32, CPU independed.
*
* FILE REVISION NUMBER:
*       $Revision: 2 $
*******************************************************************************/

#include <os/simTypesBind.h>

extern void SHOSTC_flash_close (void);

extern  void SHOSTC_exit(
    unsigned int  status
);

BOOL WINAPI simOsConsoleHandlerRoutine
(
    DWORD dwCtrlType   /*  control signal type */
);

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
    return  ((char *)GetCommandLine());
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
    GT_BOOL retVal;
    static GT_U32 alreadyInit = 0;

    if(alreadyInit != 0)
    {
        return GT_OK;
    }

    alreadyInit = 1;

    /* Allocate console for the process */
    AllocConsole();

    /* Set handler for the process */
    retVal = SetConsoleCtrlHandler(simOsConsoleHandlerRoutine, GT_TRUE);
    if (retVal == FALSE)
    {
        return GT_FAIL;
    }

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
    if (SetConsoleTitle(consoleTitlePtr))
    {
        return GT_OK;
    }
    else
    {
        return GT_FAIL;
    }
}

/*******************************************************************************
* simOsConsoleHandlerRoutine
*
* DESCRIPTION:
*       This is the control handler routine activated by ctrl+c
*       or ctrl+break keys
*
* INPUTS:
*       dwCtrlType - The type of control event
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
BOOL WINAPI simOsConsoleHandlerRoutine
(
    IN DWORD dwCtrlType
)
{
    switch(dwCtrlType)
    {
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT:
        case CTRL_CLOSE_EVENT:
        case CTRL_LOGOFF_EVENT:
        case CTRL_SHUTDOWN_EVENT:
            printf("!!!!!SHUT DOWN PROCESS!!!!! \n");

            /* Close the flash mapped file */
            SHOSTC_flash_close() ;
            /* Exit all but not process */
            SHOSTC_exit(1000);
            Sleep(1000);
            /* Terminate process */
            ExitProcess(0);
        default:
        break;
    }

    return (TRUE);
}

