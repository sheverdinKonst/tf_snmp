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
* osLinuxExc.c
*
* DESCRIPTION:
*       Linux Operating System wrapper.
*       Exception handling and Fatal Error facilities.
*
* DEPENDENCIES:
*       
*
* FILE REVISION NUMBER:
*       $Revision: 5 $
*******************************************************************************/

#include <gtOs/gtOsMem.h>
#include <gtOs/gtOsExc.h>
#include <gtOs/gtOsIo.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>


/************* Includes ******************************************************/

#define MAX_STR_LEN 300

/************* Defines ***********************************************/

/* External function definitions */


/*******************************************************************************
* osLoadFuctionCalls
*
* DESCRIPTION:
*       Loading the last function calls to a given buffer
* INPUTS:
*       pc - program counter pointer
*       sp - stack pointer
*       funcCallList - given buffer to load in the call stack
* OUTPUTS:
*       None
* RETURNS:
*       None
* COMMENTS:
*       None
*
*******************************************************************************/
void  osLoadFuctionCalls
(
  unsigned int * pc,
  unsigned int * sp,
  unsigned int * funcCallList
)
{
    (void)pc;
    (void)sp;
    (void)funcCallList;
    return;
}

/************ Public Functions ************************************************/

/*******************************************************************************
* osReset
*
* DESCRIPTION:
*       Reset the CPU
* INPUTS:
*       type - reset type (cold, hot ... TBD )
* OUTPUTS:
*       None
* RETURNS:
*       None
* COMMENTS:
*       Calling to specific function from mainExtDrv
*
*******************************************************************************/
void  osReset(int type)
{
    (void)type;
#ifndef LINUX_SIM
    if (system("reboot") != 0)
    {
        fprintf(stderr, "\r\n\n*** Failed to while executing system(\"reboot\") ***\r\n");
    }
#endif
    return;
}

/*******************************************************************************
* osFatalErrorInit
*
* DESCRIPTION:
*       Initalize the Fatal Error mechanism.
* INPUTS:
*       fatalErrFunc - Pointer to function handling the user fatal error calles.
*                      If using the default fatal error hendling - use NULL
*
* OUTPUTS:
*       None
* RETURNS:
*       0 - success, 1 - fail.
* COMMENTS:
*       None
*
*******************************************************************************/
GT_STATUS osFatalErrorInit
(
    FATAL_FUNC_PTR funcPtr
)
{
    (void)funcPtr;
    return(GT_OK);
}

/*******************************************************************************
* osFatalError
*
* DESCRIPTION:
*       Handling fatal error message.
* INPUTS:
*       type - Type of fatal error.
*       message - formated text.
*       ... - arguments related to the formated text.
* OUTPUTS:
*       None
* RETURNS:
*       None
* COMMENTS:
*       None
*
*******************************************************************************/
void osFatalError
(
  OS_FATAL_ERROR_TYPE fatalErrorType,
  char * messageAttached
)
{
    (void)fatalErrorType;
    /* screan print */
    osPrintf("------------ Fatal Error ------------\n");
    osPrintf(messageAttached);

    /* abort application */
    abort();
}

/*******************************************************************************
* osExceptionInit
*
* DESCRIPTION:
*       Replacing the OS exception handling.
* INPUTS:
*
* OUTPUTS:
*       None
* RETURNS:
*       None
* COMMENTS:
*       None
*
*******************************************************************************/
void osExceptionInit(void)
{
    return;
}


