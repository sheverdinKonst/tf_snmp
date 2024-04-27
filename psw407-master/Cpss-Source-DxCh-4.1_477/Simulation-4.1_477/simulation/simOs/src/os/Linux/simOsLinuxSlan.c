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
* simOsLinuxSlan.c
*
* DESCRIPTION:
*       SLAN API functions
*
* DEPENDENCIES:
*
* FILE REVISION NUMBER:
*       $Revision: 14 $
*******************************************************************************/

#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <gtOs/gtOsSem.h>
#include <gtOs/gtOsIo.h>
#include <os/simTypesBind.h>
#define EXPLICIT_INCLUDE_TO_SIM_OS_H_FILES
#include <os/simOsSlan.h>
#include <os/simOsTask.h>
#include <common/SLAN/SAGENT/EXP/SAGENT.H>

#include "slanLib.h"

#define SLANS_MAX 64

typedef struct slanHandlerParamSTCT
{
    SIM_OS_SLAN_RCV_FUN     funcPtr;
    void                    *usrInfoPtr;
    const char *name ; /* unused */
    const char *cl_name ; /* unused */
} slanHandlerParamSTCT ;

int rxThreadCreated = 0;                  /* indication flag for Rx thread creation          */

int allNicsBindSem = 0;                   /* semaphore to synchronize all calls to bind and packetRxThread */

/*************** Forward declarations ********************/
static void slanLibEventHandlerFunc
(
    IN  SLAN_ID             slanId,
    IN  void*               userData,
    IN  SLAN_LIB_EVENT_TYPE eventType,
    IN  char*               pktData,
    IN  int                 pktLen
);
static unsigned __TASKCONV packetRxThread(void* pPtr);

/************ Function definitions ***********************/
static GT_U32   slanUniquePerProcess = 0;
extern void simOsSlanUniquePerProcess(void)
{
    slanUniquePerProcess = 1;
}
/*******************************************************************************
*   simOsSlanBind
*
* DESCRIPTION:
*       Binds a slan to client.
*
* INPUTS:
*       slanNamePtr   - pointer to slan name.
*       clientNamePtr - pointer to client name.
*       funcPtr       - pointer to function
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       Pointer to void
*
* COMMENTS:
*
*
*******************************************************************************/
extern SIM_OS_SLAN_ID simOsSlanBind (
    char                *slanNamePtr,
    char                *clientNamePtr,
    void                *usrInfoPtr,
    SIM_OS_SLAN_RCV_FUN     funcPtr

)
{
#ifndef APPLICATION_SIDE_ONLY
    slanHandlerParamSTCT * params;
    SLAN_ID slan;
    char slanCpuUniqueName[SLAN_NAME_LEN];/*64*/

    if(0 == strcmp(slanNamePtr,"slanCpu") ||
       0 == strcmp(slanNamePtr,"slancpu"))/*it seems we get here with 'Lower case' */
    {
        /*we give this special treatment for the CPU port only (and not to the B2B ports) ,
          because the B2B connections can be replaced by 'Internal connection' mechanism
          but the CPU port that sends packet to the CPU has limitation in MST projects:
          in MST each thread that cals MST_OS functions must be registered (SLAN are registered --> SHOST,SLAN Libs)
          but the 'Asic simualtion' threads are not. */

        /*use 'unique' SLAN to allow running of several processes in parallel*/
        sprintf(slanCpuUniqueName,"%8.8x",(GT_U32)getpid());

        slanNamePtr = slanCpuUniqueName;
    }
    else if(slanUniquePerProcess)
    {
        /* concatenate the name of slan to its process id */
        sprintf(slanCpuUniqueName,"%s_%d", slanNamePtr , getpid());

        slanNamePtr = slanCpuUniqueName;
    }

    params = calloc(1, sizeof(*params));
    params->name = slanNamePtr;
    params->cl_name = clientNamePtr;
    params->usrInfoPtr = usrInfoPtr;
    params->funcPtr = funcPtr;

    if (slanLibBind(slanNamePtr, slanLibEventHandlerFunc, params, &slan) != 0)
    {
        printf("file: %s(%d), failed to bind slan [%s]\n", __FILE__, __LINE__ , slanNamePtr);
        exit(0);
    }

    return (SIM_OS_SLAN_ID)((GT_UINTPTR)slan);
#else /* APPLICATION_SIDE_ONLY */
    return NULL;

#endif /* APPLICATION_SIDE_ONLY */
}
/*******************************************************************************
*   simOsSlanTransmit
*
* DESCRIPTION:
*       Transmit a message from slan id.
*
* INPUTS:
*       slanId         -  slan id.
*       msgCode         - message code.
*       len             - message length.
*       msgPtr          - pointer to the message
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
extern unsigned int simOsSlanTransmit (
    SIM_OS_SLAN_ID              slanId,
    SIM_OS_SLAN_MSG_TYPE_ENT    msgType,
    GT_U32                      len,
    char                        *msgPtr
)
{
#ifndef APPLICATION_SIDE_ONLY
    return slanLibTransmit((SLAN_ID)((GT_UINTPTR)slanId), msgPtr, len);
#else /* APPLICATION_SIDE_ONLY */
    return 0;
#endif /* APPLICATION_SIDE_ONLY */
}

/*******************************************************************************
*   simOsSlanUnbind
*
* DESCRIPTION:
*       Unbinds a client from slan.
*
* INPUTS:
*       slanId   - slan id.
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
extern void simOsSlanUnbind (
    SIM_OS_SLAN_ID  slanId
)
{
#ifndef APPLICATION_SIDE_ONLY
    slanHandlerParamSTCT * params;

    params = slanLibGetUserData((SLAN_ID)((GT_UINTPTR)slanId));
    slanLibUnbind((SLAN_ID)((GT_UINTPTR)slanId));
    if (params)
    {
        free(params);
    }
#endif /* APPLICATION_SIDE_ONLY */
}


/*******************************************************************************
*   simOsSlanInit
*
* DESCRIPTION:
*       Calls Linux related slan-like initialization.
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
extern void simOsSlanInit (
    void
)
{
    if (rxThreadCreated == 0)
    {
        /* create a semaphore to synchronize listening task with NIC bindings */
        if (osSemBinCreate("rxTaskSem", 0, (void*)&allNicsBindSem) != 0)
        {
            printf("Failed to create semaphore!\r\n");
            exit (0);
        }

#ifndef APPLICATION_SIDE_ONLY
        slanLibInit(SLANS_MAX, "asic");
        /* create packet handling task */
        if (simOsTaskCreate(1/*GT_TASK_PRIORITY_ABOVE_NORMAL*/,
                            (unsigned (*)(void*))packetRxThread,
                            NULL) <= 0)
        {
            printf("Failed to create packetRxThread task!\r\n");
            exit (0);
        }
#endif /* APPLICATION_SIDE_ONLY */

        rxThreadCreated = 1;
    }
}

/*******************************************************************************
*   simOsSlanStart
*
* DESCRIPTION:
*       Calls Linux related slan-like start mechanism.
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
extern void simOsSlanStart (
    void
)
{
    osSemSignal((GT_SEM)allNicsBindSem);
}


/*******************************************************************************
*   simOsChangeLinkStatus
*
* DESCRIPTION:
*       Change the state of link for the SLAN .
*
* INPUTS:
*       slanId   - slan id.
*       linkState - 1 for up , 0 for down.
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
extern void simOsChangeLinkStatus
(
    SIM_OS_SLAN_ID  slanId ,
    GT_BOOL         linkState
)
{
    /* this is a stub */
    printf("not implemented! file %s line %d slan=%u link=%d\n",
           __FILE__, __LINE__, (unsigned int)((GT_UINTPTR)slanId), linkState);
}



#ifndef APPLICATION_SIDE_ONLY
/*******************************************************************************
* slanLibEventHandlerFunc
*
* DESCRIPTION:
*       SLAN Event process function
*
* INPUTS:
*       slanId      - SLAN id
*       userData    - The data pointer passed to slanLibBind()
*       eventType   - event type
*       pktData     - pointer to packet
*       pktLen      - packet length
*
* OUTPUTS:
*       None
*
* RETURNS:
*       None
*
* COMMENTS:
*
*******************************************************************************/
static void slanLibEventHandlerFunc
(
    IN  SLAN_ID             slanId,
    IN  void*               userData,
    IN  SLAN_LIB_EVENT_TYPE eventType,
    IN  char*               pktData,
    IN  int                 pktLen
)
{
    slanHandlerParamSTCT    *prm = (slanHandlerParamSTCT*)userData;
    char    * buff = 0;
    int     res;

    if (prm->funcPtr == NULL)
        return;
#if 0
    fprintf(stderr, "Got event for slan %d, userData=%p, evType=%d pktLen=%d\n",
            slanId, userData, eventType, pktLen);
#endif
    switch (eventType)
    {
        case SLAN_LIB_EV_PACKET:
            /* call the client for a buffer space */
            if (pktLen == 0)
                break;
            buff = (prm->funcPtr)(1/* should not be 0 */,
                                        SIM_OS_SLAN_GET_BUFF_RSN_CNS,
                                        0/*don't care */,
                                        pktLen,
                                        prm->usrInfoPtr,
                                        pktData) ;

            if(!buff) /* check we have a buffer to copy to */
            {
                /* we don't have a buffer to copy to */
                return;
            }
            if (pktLen > 0)
            {
                res = SIM_OS_SLAN_GIVE_BUFF_SUCCS_RSN_CNS ;
            }
            else
            {
                res = SIM_OS_SLAN_GIVE_BUFF_ERR_RSN_CNS ;
osPrintf("file: %s(%d), res = SIM_OS_SLAN_GIVE_BUFF_ERR_RSN_CNS\n", __FILE__, __LINE__);
            }
            /* copy data to buffer */
            memcpy(buff, pktData, pktLen);

            (prm->funcPtr)(1/* should not be 0 */,
                                    res,
                                    0 /*don't care */,
                                    pktLen,
                                    prm->usrInfoPtr,
                                    buff);
            break;
#define SAGNTP_SLANUP_CNS            (0x400 + 3)
#define SAGNTP_SLANDN_CNS            (0x400 + 4)
        case SLAN_LIB_EV_LINKUP:
            (prm->funcPtr)(SAGNTP_SLANUP_CNS, 0,
                                        0/*don't care */,
                                        0,
                                        prm->usrInfoPtr,
                                        NULL) ;
            break;
        case SLAN_LIB_EV_LINKDOWN:
            (prm->funcPtr)(SAGNTP_SLANDN_CNS, 0,
                                        0/*don't care */,
                                        0,
                                        prm->usrInfoPtr,
                                        NULL) ;
            break;
        case SLAN_LIB_EV_CLOSED:
            osPrintf("The slan '%s' has closed, exiting\n",
                    slanLibGetSlanName(slanId));
            exit (0);
        default:
            break;
    }
}
/*******************************************************************************
* packetRxThread
*
* DESCRIPTION:
*       This is the task charged on receiving packet to "ASIC ports" or to
*       CPU. The task is waiting using select call for any packet to be received
*       over one of the 5 interfaces, 4 "ASIC port" and 1 ASIC-CPU port.
*
* INPUTS:
*      None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*      None
*
* COMMENTS:
*       None
*
*******************************************************************************/
static unsigned __TASKCONV packetRxThread(void* pPtr)
{
    osSemWait((GT_SEM)allNicsBindSem, 0) ; /* wait for all NICS to be bind */

    slanLibMainLoop(0);

    /* never reached */
    return 0 ;
}
#endif /* APPLICATION_SIDE_ONLY */
