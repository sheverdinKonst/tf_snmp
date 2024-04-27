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
* gtAppInit.c
*
* DESCRIPTION:
*       This file includes functions to be called on system initialization,
*       for demo and special purposes.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 47 $
*
*******************************************************************************/



#include <gtOs/gtOsGen.h>
#include <gtOs/gtOsMem.h>
#include <gtOs/gtOsIo.h>
#include <cmdShell/common/cmdCommon.h>
#include <cmdShell/shell/cmdMain.h>

/*support 'makefile' ability to define the size of the allocation */
#ifndef APP_DEMO_OSMEM_DEFAULT_MEM_INIT_SIZE
    /* Default memory size */
    #define APP_DEMO_OSMEM_DEFAULT_MEM_INIT_SIZE (2048*1024)
#endif /* ! APP_DEMO_OSMEM_DEFAULT_MEM_INIT_SIZE */


GT_STATUS appDemoCpssInit(GT_VOID);

/* sample code fo initializatuion befor the shell starts */
#ifdef INIT_BEFORE_SHELL

#include <gtOs/gtOsSem.h>
#include <gtOs/gtOsTask.h>


GT_SEM semCmdSysInit;

/*******************************************************************************
* cpssInitSystemCall
*
* DESCRIPTION:
*       This routine is the starting point of the Driver.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK      - on success.
*       GT_FAIL    - otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
GT_STATUS cpssInitSystemCall(GT_VOID)
{
    GT_STATUS rc;
    printf("cpssInitSystem(28,1,0) will be called\n");
    rc = cpssInitSystem(28,1,0);
    printf("cpssInitSystem(28,1,0) returned 0x%X\n", rc );
#if 0
    rc = osSemSignal(semCmdSysInit);
    printf("Signal semaphore, rc = 0x%X\n", rc);
#endif
    return GT_OK;
}
/*******************************************************************************
* cpssInitSystemCallTask
*
* DESCRIPTION:
*       This routine is the starting point of the Driver.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK      - on success.
*       GT_FAIL    - otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
GT_STATUS cpssInitSystemCallTask(GT_VOID)
{
    GT_STATUS rc;
    GT_U32    taskSysInitCmd;

    rc = osWrapperOpen(NULL);
    printf("osWrapperOpen called, rc = 0x%X\n", rc);
#if 0
    rc = osSemBinCreate("semCmdSysInit", OS_SEMB_EMPTY, &semCmdSysInit);
    printf("Create semCmdSysInit semaphore, rc = 0x%X\n", rc);
#endif
    osTimerWkAfter(1000);
    rc = osTaskCreate (
        "CMD_InitSystem", 10 /* priority */,
        32768 /*stack size */,
        (unsigned (__TASKCONV *)(void*)) cpssInitSystemCall,
        NULL, &taskSysInitCmd);
    printf("Create taskSysInitCmd task, rc = 0x%X, id = 0x%X \n",
           rc, taskSysInitCmd);
#if 0
    rc = osSemWait(semCmdSysInit, 600000); /* 10 minutes */
    osTimerWkAfter(1000);
    printf("Wait for semaphore, rc = 0x%X\n", rc);
#endif
    return GT_OK;
}

#endif /*INIT_BEFORE_SHELL*/

/*******************************************************************************
* userAppInitialize
*
* DESCRIPTION:
*       This routine is the starting point of the Driver.
*       Called from  userAppInit() or from win32:main()
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK      - on success.
*       GT_FAIL    - otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
GT_STATUS userAppInitialize(
    IN GT_U32 devNum
)
{
    GT_STATUS rc;

    /* must be called before any Os function */
    rc = osWrapperOpen(NULL);
    if(rc != GT_OK)
    {
        printf("osWrapper initialization failure!\n");
        return rc;
    }

    /* Initialize memory pool. It must be done before any memory allocations */
    /* must be before osWrapperOpen(...) that calls osStaticMalloc(...) */
    rc = osMemInit(APP_DEMO_OSMEM_DEFAULT_MEM_INIT_SIZE, GT_TRUE);
    if (rc != GT_OK)
    {
        osPrintf("osMemInit() failed, rc=%d\n", rc);
        return rc;
    }

    /* run appDemoCpssInit() directly, not from console => show board list => show sw version
     * this allow application to work without console task */
    rc = appDemoCpssInit();
    if (rc != GT_OK)
    {
        osPrintf("appDemoCpssInit() failed, rc=%d\n", rc);
    }

    /* Set gtInitSystem to be the init function */
    /*cpssInitSystemFuncPtr = (GT_INTFUNCPTR)cpssInitSystem;*/

#ifdef INIT_BEFORE_SHELL
    printf("cpssInitSystemCallTask will be called\n");
    cpssInitSystemCallTask();
    printf("cpssInitSystemCallTask was called\n");
#endif /*INIT_BEFORE_SHELL*/
    /* Start the command shell */



    cmdInit(devNum);

    return GT_OK;
} /* userAppInitialize */

/*******************************************************************************
* userAppInit
*
* DESCRIPTION:
*       This routine is the starting point of the Driver.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK      - on success.
*       GT_FAIL    - otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
GT_STATUS userAppInit(GT_VOID)
{
    return userAppInitialize(0);
} /* userAppInit */



