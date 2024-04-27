/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* sinit.c
*
* DESCRIPTION:
*       This is a external API for SInit module of Simulation.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 17 $
*
*******************************************************************************/
#include <stdarg.h>
#include <assert.h>
#include <os/simTypesBind.h>
#include <common/Utils/Error/serror.h>
#include <asicSimulation/SInit/sinit.h>
#include <asicSimulation/SEmbedded/simFS.h>
#include <asicSimulation/SCIB/scib.h>
#include <asicSimulation/SLog/simLog.h>

#ifndef APPLICATION_SIDE_ONLY
/* those files exists only when it is not application side only , because those
   files are on the devices side */
    #include <asicSimulation/SKernel/skernel.h>
    #include <common/SBUF/sbuf.h>
    #include <common/SQue/squeue.h>
#endif /*!APPLICATION_SIDE_ONLY*/
#include <asicSimulation/SDistributed/sdistributed.h>


/* defines */
#define MAX_POOL_BUF_NUMBER             256
#define MAX_DEV_NUMBER                  256

/* Max number of frames to print */
#define MAX_DEBUG_FRAMES                100
/* Max debug frame length */
#define MAX_FRAME_LENGTH                180


/* is the dist role application */
GT_BOOL sasicgSimulationRoleIsApplication=GT_FALSE;
/* is the dist role device(s) */
GT_BOOL sasicgSimulationRoleIsDevices=GT_FALSE;
/* is the dist role broker */
GT_BOOL sasicgSimulationRoleIsBroker=GT_FALSE;
/* is the dist role bus */
GT_BOOL sasicgSimulationRoleIsBus=GT_FALSE;

/* the type of running simulation */
SASICG_SIMULATION_ROLES_ENT sasicgSimulationRole = SASICG_SIMULATION_ROLE_NON_DISTRIBUTED_E;
GT_U32  simulationInitReady = 0;

/* fatal error log file name */
static char         smainErrorLogFile[SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS] = {0};

/* the window title name */
static char         titleName[SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS] = {0};

/* number of devices in the simulation */
GT_U32       smainDevicesNumber = 0;
/* number of applications in the CPU :
   for broker system - number of applications connected to the broker
*/
GT_U32  sinitNumOfApplications = 1;

/* the info about own (this) section of board
   relevant to device(s) that connected to 'Interface BUS'
*/
SINIT_BOARD_SECTION_INFO_STC  sinitOwnBoardSection = {0,0,NULL};

/* number of connections that the interface BUS have to the devices/board sections */
GT_U32  sinitInterfaceBusNumConnections = 0;

/* ROS millisecond to tick ratio -- needed for linker with SHOST */
UINT_32 OSTIMG_ms_in_tick = 20;

/* short names of the system , index is sasicgSimulationRole */
char* consoleDistStr[] = {
    "non-dist",/*SASICG_SIMULATION_ROLE_NON_DISTRIBUTED_E*/
    "dist-application",/*SASICG_SIMULATION_ROLE_DISTRIBUTED_APPLICATION_SIDE_E*/
    "dist-devices",/*SASICG_SIMULATION_ROLE_DISTRIBUTED_ASIC_SIDE_E*/
    "interface bus",/*SASICG_SIMULATION_ROLE_DISTRIBUTED_INTERFACE_BUS_BRIDGE_E*/
    "broker",/*SASICG_SIMULATION_ROLE_BROKER_E*/
    "dist-application via broker",/*SASICG_SIMULATION_ROLE_DISTRIBUTED_APPLICATION_SIDE_VIA_BROKER_E*/
    "dist-devices via bus",/*SASICG_SIMULATION_ROLE_DISTRIBUTED_ASIC_SIDE_VIA_INTERFACE_BUS_BRIDGE_E*/

    ""
};

/* the application ID in multi process environment */
GT_U32  sinitMultiProcessOwnApplicationId = 0;

/* the mode of application that connected to broker.*/
BROKER_INTERRUPT_MASK_MODE   sdistAppViaBrokerInterruptMaskMode = INTERRUPT_MODE_BROKER_USE_MASK_INTERRUPT_LINE_MSG;

extern void applicationViaBrokerInitPart2(void);

/* main INI file name */
static char   sinitMainIniFile[SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS] = {0};

/* distributed INI file name */
char   sdistIniFile[SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS] = {0};

/* pointer to the command line */
char   commandLineInit[512] = {0};
/* path to the embedded file system */
char   embeddedFsPath[512] = {0};
/* application executable path */
char   appExePath[512] = {0};

#ifdef LINUX_SIM
extern void simOsSlanUniquePerProcess(void);
#endif

/*******************************************************************************
*   distributedParamGet
*
* DESCRIPTION:
*       read the INI file in order to get the simulation type of role.
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
static void distributedInit(void)
{
    char    getStr[SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS];/* string to read from INI file */
    char    *tockenPrt = NULL;/* pointer in the command line */
    char    *roleTypePtr = NULL;
    GT_U32  tmpVal;
    char    tmpStr[SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS];
    GT_U32  devices[256];
    GT_U32  ii,jj;
    char          tmpCommandLine[512];
    /* get command line */

    /* error log file name */
    if(!SIM_OS_MAC(simOsGetCnfValue)("distribution", "file_name",
                       SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS, sdistIniFile))
    {
        goto after_ini_file_info_lbl;
    }

    /* set the distributed INI file as the working INI file */
    sinitIniFileSet(sdistIniFile);

    /* first token is full exe file name */
    strcpy(tmpCommandLine,commandLineInit);
    tockenPrt = tmpCommandLine;

    /* first token is full exe file name  */
    strtok(tockenPrt, " ");

    while ((tockenPrt = strtok(NULL, " ")) != NULL)
    {
        /* find -role option */
        if (strcmp(tockenPrt, "-role") == 0)
        {
            tockenPrt = strtok(NULL, " ");
            if (tockenPrt == NULL)
                skernelFatalError("distributedInit: role type absent");

            roleTypePtr = tockenPrt;
            break;
        }
    }

    if(tockenPrt == NULL)
    {
        /****************************************************/
        /* read the INI file to see the "role" we play with */
        /****************************************************/

        /* the command line is without the -role option */
        /* so we need to look for in the INI file for this info */
        if(SIM_OS_MAC(simOsGetCnfValue)("distribution",  "role",
                                 SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS, getStr))
        {
            roleTypePtr = getStr;
        }
        else
        {
            roleTypePtr = NULL;
        }
    }

    if(roleTypePtr)
    {
        if(0 == strcmp(roleTypePtr,"application"))
        {
            sasicgSimulationRole = SASICG_SIMULATION_ROLE_DISTRIBUTED_APPLICATION_SIDE_E;

            /* check if this application is using 'broker'  */
            if(SIM_OS_MAC(simOsGetCnfValue)("distribution",  "use_broker",
                                     SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS, getStr))
            {
                if(0 == strcmp(getStr,"1"))
                {
                    sasicgSimulationRole = SASICG_SIMULATION_ROLE_DISTRIBUTED_APPLICATION_SIDE_VIA_BROKER_E;

                    /* get the application Id  'Application using broker' */

                    /* first token is full exe file name */
                    strcpy(tmpCommandLine,commandLineInit);
                    tockenPrt = tmpCommandLine;

                    /* first token is full exe file name  */
                    strtok(tockenPrt, " ");
                    while ((tockenPrt = strtok(NULL, " ")) != NULL)
                    {
                        /* find -appId option */
                        if (strcmp(tockenPrt, "-appId") == 0)
                        {
                            tockenPrt = strtok(NULL, " ");
                            if (tockenPrt == NULL)
                                skernelFatalError("distributedInit: appId value absent");

                            sscanf(tockenPrt, "%u", &tmpVal);
                            sinitMultiProcessOwnApplicationId = tmpVal;
                            break;
                        }
                    }


                    /* check if this application is using 'broker'  */
                    if(SIM_OS_MAC(simOsGetCnfValue)("broker",  "interrupt_mask_mode",
                                             SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS, getStr))
                    {
                        sscanf(getStr, "%u", &tmpVal);
                        if(tmpVal == 0)
                        {
                            sdistAppViaBrokerInterruptMaskMode = INTERRUPT_MODE_BROKER_AUTOMATICALLY_MASK_INTERRUPT_LINE;
                        }
                    }
                }
            }
        }
        else if(0 == strcmp(roleTypePtr,"asic"))
        {
            sasicgSimulationRole = SASICG_SIMULATION_ROLE_DISTRIBUTED_ASIC_SIDE_E;

            /* check if this application is using 'broker'  */
            if(SIM_OS_MAC(simOsGetCnfValue)("distribution",  "use_interface_bus",
                                     SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS, getStr))
            {
                sscanf(getStr, "%u", &tmpVal);
                if(tmpVal)
                {
                    sasicgSimulationRole = SASICG_SIMULATION_ROLE_DISTRIBUTED_ASIC_SIDE_VIA_INTERFACE_BUS_BRIDGE_E;
                }
            }

            if(sasicgSimulationRole == SASICG_SIMULATION_ROLE_DISTRIBUTED_ASIC_SIDE_VIA_INTERFACE_BUS_BRIDGE_E)
            {
                /* the board sections relevant only when there is 'Interface bus' */

                /* first token is full exe file name */
                strcpy(tmpCommandLine,commandLineInit);
                tockenPrt = tmpCommandLine;

                /* first token is full exe file name  */
                strtok(tockenPrt, " ");
                while ((tockenPrt = strtok(NULL, " ")) != NULL)
                {
                    /* find -board_section option */
                    if (strcmp(tockenPrt, "-board_section") == 0)
                    {
                        tockenPrt = strtok(NULL, " ");
                        if (tockenPrt == NULL)
                            skernelFatalError("distributedInit: board_section type absent");

                        sscanf(tockenPrt, "%u", &tmpVal);
                        sinitOwnBoardSection.ownSectionId = tmpVal;
                        break;
                    }
                }

                /* restore working with the main INI file */
                sinitIniFileRestoreMain();

                for(ii = 0 ;ii < smainDevicesNumber; ii++)
                {
                    sprintf(tmpStr, "dev_%d_board_section", ii);

                    /* check if this application is using 'broker'  */
                    if(SIM_OS_MAC(simOsGetCnfValue)("board_sections", tmpStr ,
                                             SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS, getStr))
                    {
                        sscanf(getStr, "%u", &tmpVal);

                        if(tmpVal == sinitOwnBoardSection.ownSectionId)
                        {
                            sinitOwnBoardSection.numDevices++;
                            devices[ii] = 1;
                        }
                        else
                        {
                            devices[ii] = 0;
                        }
                    }
                    else
                    {
                        devices[ii] = 0;
                    }
                }

                /* set the distributed INI file as the working INI file */
                sinitIniFileSet(sdistIniFile);


                if(sinitOwnBoardSection.numDevices == 0)
                {
                    /* all the devices are in our section */
                    sinitOwnBoardSection.numDevices = smainDevicesNumber;
                }

                sinitOwnBoardSection.devicesIdArray =
                    malloc(sinitOwnBoardSection.numDevices * sizeof(GT_U32));

                jj = 0;
                for(ii = 0 ; ii < smainDevicesNumber; ii++)
                {
                    if(devices[ii])
                    {
                        sinitOwnBoardSection.devicesIdArray[jj++] = ii;
                    }
                }
            }
        }
        else if(0 == strcmp(roleTypePtr,"bus"))
        {
            sasicgSimulationRole = SASICG_SIMULATION_ROLE_DISTRIBUTED_INTERFACE_BUS_BRIDGE_E;

            /* restore working with the main INI file */
            sinitIniFileRestoreMain();

            /*calculate the number of board sections */
            for(ii = 0 ; ii < smainDevicesNumber; ii++)
            {
                sprintf(tmpStr, "dev_%d_board_section", ii);

                if(! SIM_OS_MAC(simOsGetCnfValue)("board_sections", tmpStr ,
                                         SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS, getStr))
                {
                    skernelFatalError("distributedInit: device[%d] missing in section [%s] assign to board section [%s]\n",
                        ii,"board_sections",tmpStr);
                }

                sscanf(getStr, "%u", &tmpVal);

                if(tmpVal >= smainDevicesNumber)
                {
                    /* bad configuration */
                    skernelFatalError("distributedInit: device[%d] bad value in [%s] = [%d] >= num of devices [%d]\n",
                        ii,tmpStr,tmpVal,smainDevicesNumber);
                }

                if(sinitInterfaceBusNumConnections < tmpVal)
                {
                    /* update value only if this is higher value then previous */
                    sinitInterfaceBusNumConnections = tmpVal;
                }
            }

            /* set the distributed INI file as the working INI file */
            sinitIniFileSet(sdistIniFile);

            sinitInterfaceBusNumConnections++;/* add 1 to make 1 based number */

        }
        else if(0 == strcmp(roleTypePtr,"broker"))
        {
            sasicgSimulationRole = SASICG_SIMULATION_ROLE_BROKER_E;

            if(SIM_OS_MAC(simOsGetCnfValue)("broker",  "num_clients",
                                     SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS, getStr))
            {
                sscanf(getStr, "%u", &tmpVal);
                sinitNumOfApplications = tmpVal;
            }
        }
    }

after_ini_file_info_lbl:

#ifdef APPLICATION_SIDE_ONLY
    switch(sasicgSimulationRole)
    {
        case SASICG_SIMULATION_ROLE_DISTRIBUTED_APPLICATION_SIDE_E:
        case SASICG_SIMULATION_ROLE_DISTRIBUTED_APPLICATION_SIDE_VIA_BROKER_E:
        case SASICG_SIMULATION_ROLE_BROKER_E:
        case SASICG_SIMULATION_ROLE_DISTRIBUTED_INTERFACE_BUS_BRIDGE_E:
            break;
        default:
            sasicgSimulationRole = SASICG_SIMULATION_ROLE_DISTRIBUTED_APPLICATION_SIDE_E;
            break;
    }
#endif /*APPLICATION_SIDE_ONLY*/


    if(sasicgSimulationRole == SASICG_SIMULATION_ROLE_NON_DISTRIBUTED_E)
    {
        /* restore working with the main INI file */
        sinitIniFileRestoreMain();

        /* no need to continue */
        return;
    }

    /******************************/
    /* set name of window's title */
    /******************************/
    sprintf(titleName ,"%s --> %s" ,titleName , consoleDistStr[sasicgSimulationRole]);

    if(sasicgSimulationRole == SASICG_SIMULATION_ROLE_DISTRIBUTED_ASIC_SIDE_VIA_INTERFACE_BUS_BRIDGE_E &&
       sinitOwnBoardSection.numDevices != smainDevicesNumber)
    {
        /* only device via bus may use the board sections */

        /* add the number of the board section to the title of the window */
        /* add it only when there are really more then single board sections*/

        sprintf(titleName ,"%s , board section [%d]" ,titleName , sinitOwnBoardSection.ownSectionId);
    }

    SIM_OS_MAC(simOsSetConsoleTitle)(titleName) ;

    /* call the simulation distribution to initialize according to the role */
    simDistributedInit();

    /* restore working with the main INI file */
    sinitIniFileRestoreMain();

    return;
}

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
)
{
#ifndef APPLICATION_SIDE_ONLY
    /* Set fatal error specific function */
    sUtilsFatalErrorBind((SUTILS_FATAL_ERROR_FUN)skernelFatalError);

    /* Init Sbuff lib, allocate buffers and pools */
    sbufInit(MAX_POOL_BUF_NUMBER);

    /* Init Squeue library. */
    squeInit();

#endif /*!APPLICATION_SIDE_ONLY*/

    /* Init the distributed architecture (if needed) */
    distributedInit();

    /* the init of scib must be before skernelInit */
    scibInit(MAX_DEV_NUMBER);


#ifndef APPLICATION_SIDE_ONLY
    if(sasicgSimulationRole == SASICG_SIMULATION_ROLE_NON_DISTRIBUTED_E ||
       sasicgSimulationRoleIsDevices == GT_TRUE)
    {
        /* Init SKernel library */
        skernelInit();
    }

    {
        GT_U32  titleNameLen = (GT_U32)strlen(titleName);

        /* update the name with actual device names */
        smainGetDevicesString(titleName+titleNameLen ,
                              sizeof(titleName) - titleNameLen - 1);
    }
#endif /*!APPLICATION_SIDE_ONLY*/

}

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
)
{
    /* do specific initialization that needed at the end of the initialization */
    switch(sasicgSimulationRole)
    {
        case SASICG_SIMULATION_ROLE_DISTRIBUTED_APPLICATION_SIDE_VIA_BROKER_E:
            applicationViaBrokerInitPart2();
            break;
        default:
            break;
    }

    return;
}

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
)
{
    if(sasicgSimulationRole != SASICG_SIMULATION_ROLE_NON_DISTRIBUTED_E)
    {
        /* shut down the simulation distributed engine */
        simDistributedExit();
    }

#ifndef APPLICATION_SIDE_ONLY
    /* shut down SKernel library before reboot */
    skernelShutDown();
#endif /*APPLICATION_SIDE_ONLY*/
}

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
)
{
    char  param_str[SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS];
#ifndef RTOS_ON_SIM
    char *tockenPrt;
    char  tmpCommandLine[512];
    int   embeddedFS = 0;
    char  tempDir[512] = {0}; /* temp dir name */
    char  files[20][100] = {{0}}; /* reg files names */
    int   reg_name = 0; /* reg files names */

#endif  /*RTOS_ON_SIM*/
    GT_U32  param_val; /* parameter value */
    GT_BOOL printGeneralAllowed;/* do we allow general printings */

    /* Must be called before use any of OS functions. */
    SIM_OS_MAC(osWrapperOpen)(NULL);

    scibInit0();
#ifndef RTOS_ON_SIM
    tockenPrt = SIM_OS_MAC(simOsGetCommandLine)();

    SIM_OS_MAC(simOsSetConsoleTitle)(" Started simulation initialization ...please wait...") ;

    /* get command line , and copy to the buffer */
    strcpy(commandLineInit,tockenPrt);

    tockenPrt = commandLineInit;

    while(NULL != (tockenPrt = strchr(tockenPrt,'"')))/* look for character " */
    {
        tockenPrt[0] = ' ';/* replace " with space */
    }

    /*  command line format:
     *  path/file.exe -i config_file.ini ...
     */

    strcpy(tmpCommandLine,commandLineInit);
    tockenPrt = tmpCommandLine;

    /* first token is full exe file name  */
    tockenPrt = strtok(tockenPrt, " ");

    /* Save application executable path   */
    strcpy(appExePath, tockenPrt);

    while (1)
    {
        if ((tockenPrt = strtok(NULL, " ")) == NULL)
            skernelFatalError("simulationLibInit: -i or -e options absent");

        /* find -i or -e option */
        embeddedFS = (strcmp(tockenPrt, "-e") == 0) ? 1 : 0;
        if ((strcmp(tockenPrt, "-i") == 0) || embeddedFS)
        {
            tockenPrt = strtok(NULL, " ");
            if (tockenPrt == NULL)
                skernelFatalError("simulationLibInit: ini file absent");

            break;
        }
    }

    if(embeddedFS)
    {
        /* save ini file to temp dir */
        if(simFSsave(tempDir, tockenPrt, files))
        {
            printf("The INI file [%s] not exist \n\n", tockenPrt);
            simFSprintIniList();

            SIM_OS_MAC(simOsAbort)();
        }

        /* Save path to the embedded file system */
        strcpy(embeddedFsPath, tempDir);

        /* save reg files to temp dir  */
        for(reg_name = 0; strlen(files[reg_name]); reg_name++)
        {
            simFSsave(tempDir, files[reg_name], 0);
        }

        /* change "embedded" ini file path to point to temp dir on real FS */
        tockenPrt = strcat(tempDir, tockenPrt);
    }

    /* set config file name */
    SIM_OS_MAC(simOsSetCnfFile)(tockenPrt) ;

    /* save main INI file name */
    strcpy(sinitMainIniFile,tockenPrt);


    tockenPrt = commandLineInit;
    while ((tockenPrt = strtok(NULL, "-")) != NULL)
    {
        if(strcmp(tockenPrt , "unique_slans"))
        {
            /* not the proper '-' */
            continue;
        }
        /* exists option of '-unique_slans' */
#ifdef LINUX_SIM
        printf("-unique_slans : slan names unique per process \n");
        simOsSlanUniquePerProcess();
#else
        printf("-unique_slans : on WIN32 option not implemented (ignored !!) \n");
#endif
        break;
    }

    /* initialize logger */
    if ( GT_OK != simLogInit() )
    {
        skernelFatalError("simLogInit: can't init logger");
    }

    /* error log file name */
    if(!SIM_OS_MAC(simOsGetCnfValue)("fatal_error_file", "file_name",
                       SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS, smainErrorLogFile))
    {
#ifndef LINUX_SIM
        strcpy(smainErrorLogFile, "c:\\temp\\scor_fatal_error_file.txt");
#else /*LINUX_SIM*/
        strcpy(smainErrorLogFile, "/tmp/scor_fatal_error_file.txt");
#endif /*LINUX_SIM*/
    }

#endif  /*RTOS_ON_SIM*/

    /* get devices number from *.ini file */
    if(!SIM_OS_MAC(simOsGetCnfValue)("system",  "devices_number",
                             SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS, param_str))
    {
        skernelFatalError(" skernelInit: number of devices not defined\n");
    }

    /* about getting titleName
       this line should be before calling SASICG_init1() because inside
       SASICG_init1() we may change to other INI file so , to avoid the
       changing again of INI file name we get the info here and may use it later
       */
    SIM_OS_MAC(simOsGetCnfValue)( "rs","name", 80, titleName) ;

    sscanf(param_str, "%d", &smainDevicesNumber);

    /* init SHOST companents */
    SIM_OS_MAC(simOsInitInterrupt)();

    /* allocate console */
    SIM_OS_MAC(simOsAllocConsole)() ;

    /* init basic simulation libs -- part 1 */
    SASICG_init1();

    if(sasicgSimulationRole == SASICG_SIMULATION_ROLE_NON_DISTRIBUTED_E)
    {
        /* set name of window's title */
        SIM_OS_MAC(simOsSetConsoleTitle)(titleName) ;
    }

    simulationInitReady = 1;

#ifndef APPLICATION_SIDE_ONLY
    /*send to log all the info about the devices.*/
    simLogAddDevicesInfo();
#endif /*APPLICATION_SIDE_ONLY*/


    /* init basic simulation libs -- part 2 */
    SASICG_init2();

    /* check if general printing allowed */
    if(SIM_OS_MAC(simOsGetCnfValue)("debug", "print_general_allowed",
                       SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS, param_str))
    {
        sscanf(param_str, "%u", &param_val);
        printGeneralAllowed = param_val ? GT_TRUE : GT_FALSE;
    }
    else
    {
        /* by default the general printing is NOT allowed */
        printGeneralAllowed = GT_FALSE;
    }

    if(printGeneralAllowed == GT_TRUE)
    {
        printf(" simulation: initialization done \n");
    }
}

/*******************************************************************************
*   skernelFatalError
*
* DESCRIPTION:
*       Fatal error handler for simulation
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
)
{
    DECLARE_FUNC_NAME(skernelFatalError);

    static char buff[MAX_FRAME_LENGTH * MAX_DEBUG_FRAMES];    /* main buffer */
    GT_U32  len, btlen = 0;
    SKERNEL_DEVICE_OBJECT * devObjPtr = NULL;/* needed for __LOG macro */

    va_list args;

    if(GT_TRUE == skernelUserDebugInfo.disableFatalError)
    {
        __LOG(("Fatal error is disabled \n"));
        return;
    }

    va_start(args, format);
    len = vsprintf(buff, format, args);
    va_end(args);

    buff[len++] = '\n';
    buff[len++] = '\n';
    btlen = SIM_OS_MAC(simOsBacktrace)(MAX_DEBUG_FRAMES, /* max num of frames to print */
                            buff+len, sizeof(buff)-len-1);

    buff[len+btlen] = 0;

    /* log the message */
    __LOG(("fatal error: %s",buff));
    /* NOTE: log is closed inside skernelShutDown(...) */

    /* screen print */
    printf("%s", buff);
    fflush(stdout);

    /* file print */
    if(strlen(smainErrorLogFile) != 0)
    {
        FILE *errorLogFilePrt;
        errorLogFilePrt = fopen( smainErrorLogFile , "w" );
        if(errorLogFilePrt)
        {
            fprintf(errorLogFilePrt, "%s\n", buff);
            fflush(errorLogFilePrt);
            fclose (errorLogFilePrt);
        }
    }

#ifndef APPLICATION_SIDE_ONLY
    /* unbind SLANs */
    skernelShutDown();
#endif /*APPLICATION_SIDE_ONLY*/

    if(sasicgSimulationRole != SASICG_SIMULATION_ROLE_NON_DISTRIBUTED_E)
    {
        /* exit the Distributed system */
        simDistributedExit();
    }

    /* abort application */
    SIM_OS_MAC(simOsAbort)();

    abort();/* avoid all warnings/errors from klockwork that program keeps running after fatal error asserted */
}

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
)
{
    /* set config file name */
    SIM_OS_MAC(simOsSetCnfFile)(iniFileName) ;
}

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
)
{
    /* restore main INI file name */
    SIM_OS_MAC(simOsSetCnfFile)(sinitMainIniFile) ;
}

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
)
{
    GT_U32  len;
    GT_U32     useProtection;

    va_list args;

    useProtection = simulationInitReady;

    if(useProtection)
    {
        SCIB_SEM_TAKE;
    }

    va_start(args, format);
    len = vprintf(format, args);
    va_end(args);

    if(useProtection)
    {
        SCIB_SEM_SIGNAL;
    }

    return len;
}

