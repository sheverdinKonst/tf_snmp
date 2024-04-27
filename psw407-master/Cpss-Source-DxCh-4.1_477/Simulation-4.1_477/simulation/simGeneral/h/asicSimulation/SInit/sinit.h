/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* smain.h
*
* DESCRIPTION:
*       This is a external API definition for SInit module of Simulation.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 4 $
*
*******************************************************************************/
#ifndef __sinith
#define __sinith

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <os/simTypesBind.h>

/*******************************************************************************
*   enumeration: SASICG_SIMULATION_ROLES_ENT
*
* DESCRIPTION:
*       types of simulations architectures :
*       SASICG_SIMULATION_ROLE_NON_DISTRIBUTED_E - non distributed simulation.
*       SASICG_SIMULATION_ROLE_DISTRIBUTED_APPLICATION_SIDE_E - the simulation
*               is distributed , and this side is the "application side" (the "CPU" side)
*       SASICG_SIMULATION_ROLE_DISTRIBUTED_ASIC_SIDE_E - the simulation
*               is distributed , and this side is the "asic side" (the "PP" side)
*       SASICG_SIMULATION_ROLE_DISTRIBUTED_INTERFACE_BUS_BRIDGE_E - the simulation
*               of 'Interface bus bridge' that is in the middle between the
*               devices and the application side (like  PCI bridge)
*       SASICG_SIMULATION_ROLE_BROKER_E - the broker on application side.
*               the broker serialize several application processes to access the
*               other side.
*       SASICG_SIMULATION_ROLE_DISTRIBUTED_APPLICATION_SIDE_VIA_BROKER_E - the
*               simulation is distributed , and this side is the "application side" (the "CPU" side)
*               that connect via the broker to the remote simulation
*******************************************************************************/
typedef enum {
    SASICG_SIMULATION_ROLE_NON_DISTRIBUTED_E,
    SASICG_SIMULATION_ROLE_DISTRIBUTED_APPLICATION_SIDE_E,
    SASICG_SIMULATION_ROLE_DISTRIBUTED_ASIC_SIDE_E,
    SASICG_SIMULATION_ROLE_DISTRIBUTED_INTERFACE_BUS_BRIDGE_E,
    SASICG_SIMULATION_ROLE_BROKER_E,
    SASICG_SIMULATION_ROLE_DISTRIBUTED_APPLICATION_SIDE_VIA_BROKER_E,
    SASICG_SIMULATION_ROLE_DISTRIBUTED_ASIC_SIDE_VIA_INTERFACE_BUS_BRIDGE_E,

    SASICG_SIMULATION_ROLE_LAST_E
}SASICG_SIMULATION_ROLES_ENT;

/* is the dist role application */
extern GT_BOOL sasicgSimulationRoleIsApplication;
/* is the dist role device(s) */
extern GT_BOOL sasicgSimulationRoleIsDevices;
/* is the dist role broker */
extern GT_BOOL sasicgSimulationRoleIsBroker;
/* is the dist role bus */
extern GT_BOOL sasicgSimulationRoleIsBus;

/* the type of running simulation */
extern SASICG_SIMULATION_ROLES_ENT sasicgSimulationRole;
/* indicate that simulation done. and system can start using it's API.
   -- this is mainly needed for terminal to be able to start while the simulation
   try to connect on distributed system.
*/
extern GT_U32  simulationInitReady;
/* number of devices in the system */
extern GT_U32  smainDevicesNumber;
/* number of applications in the CPU :
   for broker system - number of applications connected to the broker
*/
extern GT_U32  sinitNumOfApplications;

typedef struct{
    GT_U32  ownSectionId;
    GT_U32  numDevices;
    GT_U32  *devicesIdArray;/* dynamic allocated */
}SINIT_BOARD_SECTION_INFO_STC;

/* the info about own (this) section of board
   relevant to device(s) that connected to 'Interface BUS'
*/
extern SINIT_BOARD_SECTION_INFO_STC  sinitOwnBoardSection;

/* number of connections that the interface BUS have to the devices/board sections */
extern GT_U32  sinitInterfaceBusNumConnections;

/* the application ID in multi process environment */
extern GT_U32  sinitMultiProcessOwnApplicationId;

/* short names of the system , index is sasicgSimulationRole */
extern char* consoleDistStr[];

/* distributed INI file name */
extern char   sdistIniFile[SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS];

#define FUNCTION_NOT_SUPPORTED(funcName)\
    skernelFatalError(" %s : function not supported \n",funcName)

/*******************************************************************************
*   SASICG_init1
*
* DESCRIPTION:
*       Init Simulation.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
void SASICG_init1(
    void
);

/*******************************************************************************
*   SASICG_init2
*
* DESCRIPTION:
*       Stub function for backward compatibility.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
void SASICG_init2(
    void
);

/*******************************************************************************
*   SASICG_exit
*
* DESCRIPTION:
*       Shut down Simulation.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
void SASICG_exit(
    void
);

/*******************************************************************************
*   simulationLibInit
*
* DESCRIPTION:
*       Init all parts of simulation Simulation.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
void simulationLibInit(
    void
);

/*******************************************************************************
*   skernelFatalError
*
* DESCRIPTION:
*       Fatal error handler for SKernel
*
* INPUTS:
*       format - format for printing.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
void skernelFatalError
(
    IN char * format, ...
);

/*******************************************************************************
* sinitIniFileSet
*
* DESCRIPTION:
*       Sets temporary INI file name.
*
* INPUTS:
*       iniFileName - pointer to file name
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*       None
*
*******************************************************************************/
void sinitIniFileSet
(
    IN char* iniFileName
);

/*******************************************************************************
* sinitIniFileRestoreMain
*
* DESCRIPTION:
*       restore the INI file that is the 'Main INI' file.
*
* INPUTS:
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*       None
*
*******************************************************************************/
void sinitIniFileRestoreMain
(
    void
);

/*******************************************************************************
*   simulationPrintf
*
* DESCRIPTION:
*       function to replace printf ... in order to make is under SCIB lock
*
* INPUTS:
*       format - format for printing.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       On success, printf returns the number of bytes output.
*       On error, printf returns EOF.
* COMMENTS:
*
*
*******************************************************************************/
int simulationPrintf
(
    IN char * format, ...
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __sinith */


