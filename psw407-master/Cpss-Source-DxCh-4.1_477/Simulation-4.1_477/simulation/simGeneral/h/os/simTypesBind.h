/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* simTypesBind.h
*
* DESCRIPTION:
*       This file define prototypes for the binding of OS functions for
*       simulation usage.
*
* FILE REVISION NUMBER:
*       $Revision: 1 $
*******************************************************************************/

#ifndef __simTypesBindh
#define __simTypesBindh

#include <os/simTypes.h>

/* macro to convert pss/cpss OS function name to one of OS simulation */
#define SIM_OS_MAC(funcName)   sim_##funcName

#include <os/simOsBind/simOsBindTask.h>
#include <os/simOsBind/simOsBindSync.h>
#include <os/simOsBind/simOsBindSlan.h>
#include <os/simOsBind/simOsBindIntr.h>
#include <os/simOsBind/simOsBindIniFile.h>
#include <os/simOsBind/simOsBindConsole.h>
#include <os/simOsBind/simOsBindSockets.h>
#include <os/simOsBind/simOsBindProcess.h>

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

/*******************************************************************************
* osWrapperOpen
*
* DESCRIPTION:
*       Initialize OS wrapper facility.
*
* INPUTS:
*       osSpec  - ptr to OS specific init parameter(s)
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK            - on success
*       GT_FAIL          - on error
*       GT_ALREADY_EXIST - wrapper was already open
*
* COMMENTS:
*       None
*
*******************************************************************************/
typedef GT_STATUS (*SIM_OS_WRAPPER_OPEN_FUN)(void * osSpec);


/*******************************************************************************
* SIM_OS_SERIAL_DESTROY_SERIAL_FUN
*
* DESCRIPTION:
*       close last created serial port.
*
* INPUTS:
*   None
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on error
*
* COMMENTS:
*       None
*
*******************************************************************************/
typedef GT_STATUS   (*SIM_OS_SERIAL_DESTROY_SERIAL_FUN)
(
    void
);


/* SIM_OS_FUNC_BIND_STC -
*    structure that hold the "os" functions needed be bound to SIM.
*
*       tasks - set of tasks functions
*       sync - set of synchronization functions (mutex/semaphore/events..)
*       slan - set of SLAN functions
*       iniFile - set of INI file functions
*       console - set of console functions
*       sockets -  set of sockets functions
*
*/
typedef struct{
    SIM_OS_WRAPPER_OPEN_FUN             osWrapperOpen;
    SIM_OS_SERIAL_DESTROY_SERIAL_FUN    osSerialDestroySerial;

    SIM_OS_FUNC_BIND_TASK_STC           tasks;
    SIM_OS_FUNC_BIND_SYNC_STC           sync;
    SIM_OS_FUNC_BIND_SLAN_STC           slan;
    SIM_OS_FUNC_BIND_INTERRUPTS_STC     interrupts;
    SIM_OS_FUNC_BIND_INI_FILE_STC       iniFile;
    SIM_OS_FUNC_BIND_CONSOLE_STC        console;
    SIM_OS_FUNC_BIND_SOCKETS_STC        sockets;
    SIM_OS_FUNC_BIND_PROCESS_STC        processes;

}SIM_OS_FUNC_BIND_STC;


/*******************************************************************************
* simOsFuncBind
*
* DESCRIPTION:
*       bind the Simulation with OS functions.
* INPUTS:
*       osFuncBindPtr - (pointer to) set of OS call back functions
*
* OUTPUTS:
*       none
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on error
*
* COMMENTS:
*       function must be called by application before simulationLibInit()
*
*******************************************************************************/
GT_STATUS   simOsFuncBind(
    IN SIM_OS_FUNC_BIND_STC        *osFuncBindPtr
);


extern SIM_OS_WRAPPER_OPEN_FUN             SIM_OS_MAC(osWrapperOpen);
extern SIM_OS_SERIAL_DESTROY_SERIAL_FUN    SIM_OS_MAC(osSerialDestroySerial);



#endif   /* __simTypesBindh */



