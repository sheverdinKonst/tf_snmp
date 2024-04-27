/*******************************************************************************
*
*                   Copyright 2003,MARVELL SEMICONDUCTOR ISRAEL, LTD.
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL.
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.
*
* MARVELL COMPRISES MARVELL TECHNOLOGY GROUP LTD. (MTGL) AND ITS SUBSIDIARIES,
* MARVELL INTERNATIONAL LTD. (MIL), MARVELL TECHNOLOGY, INC. (MTI), MARVELL
* SEMICONDUCTOR, INC. (MSI), MARVELL ASIA PTE LTD. (MAPL), MARVELL JAPAN K.K.
* (MJKK), MARVELL SEMICONDUCTOR ISRAEL. (MSIL),  MARVELL TAIWAN, LTD. AND
* SYSKONNECT GMBH.
********************************************************************************
* file_name - vxMrvlSata.c
*
* DESCRIPTION:  implementation for VxWorks IAL
*
* DEPENDENCIES:
*******************************************************************************/
#include "mrvlSataLib.h"
#include "mvSysHwConfig.h"

/* Configurable options, scsi manager task */

int   mvSataSingleStepSemOptions = SEM_Q_PRIORITY;
char* mvSataScsiTaskName         = SCSI_DEF_TASK_NAME;
int   mvSataScsiTaskOptions      = SCSI_DEF_TASK_OPTIONS;
int   mvSataScsiTaskPriority     = SCSI_DEF_TASK_PRIORITY;
int   mvSataScsiTaskStackSize    = 8 * SCSI_DEF_TASK_STACK_SIZE;

/* Other miscellaneous defines */

#define MV_SATA_MAX_XFER_WIDTH 1		/* 16 bit wide transfer */
#define MV_SATA_MAX_XFER_LENGTH MRVL_MAX_IO_LEN	/* 16MB max transfer */

/* forward declarations */

LOCAL STATUS mvSataThreadActivate (SIOP* pSiop, MV_SATA_THREAD* pThread);
LOCAL void mvSataEvent (SIOP* pSiop, MV_SATA_EVENT* pEvent);
LOCAL STATUS mvSataThreadInit (SIOP* pSiop, MV_SATA_THREAD* pThread);
LOCAL BOOL mvSataThreadAbort (SIOP* pSiop, MV_SATA_THREAD* pThread);
LOCAL STATUS mvSataScsiBusControl (SIOP* pSiop, int operation);
LOCAL STATUS mvSataXferParamsQuery (SCSI_CTRL* pScsiCtrl, UINT8* pOffset,
					UINT8* pPeriod);
LOCAL STATUS mvSataXferParamsSet (SCSI_CTRL* pScsiCtrl, UINT8 offset,
					UINT8 period);
LOCAL STATUS mvSataWideXferParamsQuery (SCSI_CTRL* pScsiCtrl,
					UINT8* xferWidth);
LOCAL STATUS mvSataWideXferParamsSet (SCSI_CTRL* pScsiCtrl, UINT8 xferWidth);

LOCAL STATUS mvResetBus(SATA_INFO *pSataInfo, int channelIndex);
/* Core Driver related */

MV_BOOLEAN mvIalLibEventNotify(MV_SATA_ADAPTER *pMvSataAdapter, MV_EVENT_TYPE eventType,
                                   MV_U32 param1, MV_U32 param2);
MV_COMMAND_INFO *mrvlAllocateCib(SATA_INFO *sc);
void mrvlReleaseCib(MV_COMMAND_INFO *pCommandInfo);
int mrvlSetupPrdTable(SATA_INFO * sc, MV_COMMAND_INFO *pCommandInfo);
MV_BOOLEAN IALCompletion(struct mvSataAdapter *pSataAdapter,
                         MV_SATA_SCSI_CMD_BLOCK *pCmdBlock);

/*******************************************************************************
*
* mvSataCtrlCreate - create a structure for a MV_SATA device
*
* This routine creates a SCSI Controller data structure and must be called 
* before using an SCSI Controller chip.  It should be called once and only 
* once for a specified SCSI Controller controller. Since it allocates memory
* for a structure needed by all routines in mvSataLib, it must be called
* before any other routines in the library. After calling this routine,
* mvSataCtrlInit() should be called at least once before any SCSI
* transactions are initiated using the SCSI Controller.
*
* RETURNS: A pointer to MV_SATA_SCSI_CTRL structure, or NULL if memory 
* is unavailable or there are invalid parameters.
*/

MV_SATA_SCSI_CTRL *mvSataCtrlCreate (SATA_INFO* pSataInfo,
									 int index)
    {
    FAST SIOP *pSiop = NULL;		    /* ptr to SCSI Controller info */
    SCSI_CTRL *pScsiCtrl;
    SCSI_OPTIONS	scsiOpt;
	int i;
    
    /* cacheDmaMalloc the controller struct and any shared data areas */
    if ((pSiop = (SIOP *) mvOsMalloc(sizeof(SIOP))) == (SIOP *) NULL)
    {
        mvOsPrintf("mvSataCtrlCreate: cacheDmaMalloc Failed\n");
        return ((SIOP *) NULL);
    }
        

	bzero ((char *) pSiop, sizeof(SIOP));


    

    pScsiCtrl = &(pSiop->scsiCtrl);
    pSataInfo->pSataScsiCtrl = pSiop;
    /* fill in generic SCSI info for this controller */

    pScsiCtrl->eventSize 	   = sizeof (MV_SATA_EVENT);
    pScsiCtrl->threadSize 	   = sizeof (MV_SATA_THREAD);
    pScsiCtrl->maxBytesPerXfer 	   = MV_SATA_MAX_XFER_LENGTH;
    pScsiCtrl->wideXfer 	   = FALSE;
    pScsiCtrl->scsiTransact 	   = (FUNCPTR)	   scsiTransact;
    pScsiCtrl->scsiEventProc       = (VOIDFUNCPTR) mvSataEvent;
    pScsiCtrl->scsiThreadInit      = (FUNCPTR)     mvSataThreadInit;
    pScsiCtrl->scsiThreadActivate  = (FUNCPTR)     mvSataThreadActivate;
    pScsiCtrl->scsiThreadAbort     = (FUNCPTR)     mvSataThreadAbort;
    pScsiCtrl->scsiBusControl      = (FUNCPTR)     mvSataScsiBusControl;
    pScsiCtrl->scsiXferParamsQuery = (FUNCPTR)     mvSataXferParamsQuery;
    pScsiCtrl->scsiXferParamsSet   = (FUNCPTR)     mvSataXferParamsSet;
    pScsiCtrl->scsiWideXferParamsQuery = (FUNCPTR) mvSataWideXferParamsQuery;
    pScsiCtrl->scsiWideXferParamsSet   = (FUNCPTR) mvSataWideXferParamsSet;

    /* Device specific data for this controller */

	pSiop->pSataInfo = pSataInfo;

    /* the following virtual functions are not used with the  Sata */
    
    pScsiCtrl->scsiDevSelect = NULL;
    pScsiCtrl->scsiInfoXfer  = NULL;

    pScsiCtrl->scsiCtrlBusId = pSataInfo->mvSataAdapter.numberOfChannels;

	for (i=0 ; i < pScsiCtrl->scsiCtrlBusId ; i++)
	{
		pSataInfo->mvChannelData[i].osData = (UINT32)pScsiCtrl;
        pSataInfo->mvChannelData[i].targetsToAdd = 0;
        pSataInfo->mvChannelData[i].targetsToRemove = 0;
	}
	

    scsiCtrlInit (pScsiCtrl);

	/* set Controller options */
	scsiOpt.messages  = FALSE;  /* do not use SCSI messages */
	scsiOpt.disconnect = FALSE; /* do not use disconnect  */

    scsiTargetOptionsSet(pScsiCtrl , 
						 SCSI_SET_OPT_ALL_TARGETS , 
						 &scsiOpt , 
						 SCSI_SET_OPT_MESSAGES | SCSI_SET_OPT_DISCONNECT );



	

    /* Create synchronisation semaphore for single-step support  */

    pSiop->singleStepSem = semBCreate (mvSataSingleStepSemOptions, SEM_EMPTY);

#ifdef MV_SATA_INTR_AS_TASK
    pSiop->interruptsSem = semBCreate (mvSataSingleStepSemOptions, SEM_EMPTY);
	mvSataSetInterruptsScheme(&pSiop->pSataInfo->mvSataAdapter,
							  MV_SATA_INTERRUPT_HANDLING_IN_TASK);


#endif
    pSiop->rescanSem = semBCreate (mvSataSingleStepSemOptions, SEM_EMPTY);

    /* TODO - Initialize fields in any client shared data area */

    /* spawn SCSI manager - use generic code from "scsiLib.c" */

    pScsiCtrl->scsiMgrId = taskSpawn (mvSataScsiTaskName,
		       	    	      mvSataScsiTaskPriority,
		       	    	      mvSataScsiTaskOptions,
		       	    	      mvSataScsiTaskStackSize,
		       	    	      (FUNCPTR) scsiMgr,
		       	    	      (int) pSiop, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    
    return (pSiop);
    }





/*******************************************************************************
*
* mvSataIntr - interrupt service routine for the SCSI Controller
*
* The first thing to determine is if the device is generating an interrupt.
* If not, then this routine must exit as quickly as possible.
*
* Find the event type corresponding to this interrupt, and carry out any
* actions which must be done before the SCSI Controller is re-started.  
* Determine  whether or not the SCSI Controller is connected to the bus 
* (depending on the event type - see note below).  If not, start a client 
* script if possible or else just make the SCSI Controller wait for something 
* else to happen.
*
* Notify the SCSI manager of a controller event.
*
* RETURNS: N/A
*/

void mvSataIntr
    (
    SIOP *pSiop
    )
{

	SATA_INFO*		pSataInfo = pSiop->pSataInfo;
	
    SCSI_DEBUG_MSG ("mvSataIntr: pSiop 0x%08x &pSataInfo->mvSataAdapter 0x%08x\n\n",
		    (int) pSiop,(int)&pSataInfo->mvSataAdapter, 0, 0, 0, 0);

#ifdef MV_SATA_INTR_AS_TASK

	if (mvSataCheckPendingInterrupt(&pSataInfo->mvSataAdapter) == MV_TRUE)
	{
		semGive(pSiop->interruptsSem);
	}
	
#else
	/* Send the event to the SCSI manager to be processed. */
	if (mvSataInterruptServiceRoutine(&pSataInfo->mvSataAdapter) == MV_TRUE)
	{
		/* pAdapter->procNumOfInterrupts ++;*/
		mvSataScsiPostIntService(&pSataInfo->ataScsiAdapterExt);
	}

#endif

}

#ifdef MV_SATA_INTR_AS_TASK
void mvSataIntrTask
    (
    SIOP *pSiop
    )
{

	SATA_INFO*		pSataInfo = pSiop->pSataInfo;
	


	while(1)
	{
		semTake(pSiop->interruptsSem, WAIT_FOREVER);

		semTake(pSataInfo->mutualSem, WAIT_FOREVER);
		SCSI_DEBUG_MSG ("mvSataIntrTask: pSiop 0x%08x &pSataInfo->mvSataAdapter 0x%08x\n\n",
				(int) pSiop,(int)&pSataInfo->mvSataAdapter, 0, 0, 0, 0);

		
		/* Send the event to the SCSI manager to be processed. */
		if (mvSataInterruptServiceRoutine(&pSataInfo->mvSataAdapter) == MV_TRUE)
		{
			/* pAdapter->procNumOfInterrupts ++;*/
			mvSataScsiPostIntService(&pSataInfo->ataScsiAdapterExt);
		}
		semGive(pSataInfo->mutualSem);

	}

}
#endif

void mvSataRescanTask
    (
    SIOP *pSiop
    )
{
    SATA_INFO*		pSataInfo = pSiop->pSataInfo;
    SCSI_PHYS_DEV *pScsiPhysDev;
    int channelIndex, port;
    MV_U16 targetsToRemove;
    MV_U16 targetsToAdd; 
	
	while(1)
	{
        SCSI_DEBUG_MSG(" mvSataRescanTask %d\n", __LINE__, 0, 0, 0, 0, 0);
        semTake(pSiop->rescanSem, WAIT_FOREVER);
        SCSI_DEBUG_MSG(" mvSataRescanTask %d\n", __LINE__, 0, 0, 0, 0, 0);

        for(channelIndex = 0; channelIndex < pSataInfo->mvSataAdapter.numberOfChannels; channelIndex++){
            semTake(pSataInfo->mutualSem, WAIT_FOREVER);
            targetsToRemove = pSataInfo->mvChannelData[channelIndex].targetsToRemove;
            targetsToAdd = pSataInfo->mvChannelData[channelIndex].targetsToAdd;
            pSataInfo->mvChannelData[channelIndex].targetsToRemove = 0;
            pSataInfo->mvChannelData[channelIndex].targetsToAdd = 0;
            semGive(pSataInfo->mutualSem);
            
            if (targetsToRemove) {
                for (port = 0; port < 15; port++) {
                    if(targetsToRemove & (1 << port)){
                        pScsiPhysDev = scsiPhysDevIdGet(&pSataInfo->pSataScsiCtrl->scsiCtrl, channelIndex, port);
                        mvLogMsg(MV_IAL_LOG_ID,  MV_DEBUG_ERROR, "delete device %p, bus %d lun %d \n",
                                 pScsiPhysDev, channelIndex, port);
                        if(scsiPhysDevDelete(pScsiPhysDev) == OK){
                            SCSI_DEBUG_MSG("scsiPhysDevDelete succeeded. pScsiPhysDev %p. bus %d lun %d\n",
                                           (int)pScsiPhysDev, channelIndex, port, 0,0,0);

                        }
                        else
                        {
                            SCSI_DEBUG_MSG("scsiPhysDevDelete failed. pScsiPhysDev %p. bus %d lun %d\n",
                                           (int)pScsiPhysDev, channelIndex, port, 0,0,0);
                        }
                    }
                }/* for*/
            } /* if */
            if (targetsToAdd) {
                for (port = 0; port < 15; port++) {
                    if(targetsToAdd & (1 << port)){
                        pScsiPhysDev = scsiPhysDevCreate(&pSataInfo->pSataScsiCtrl->scsiCtrl,
                                                         channelIndex, port, 0, NONE, 0, 0, 0);
                        if(pScsiPhysDev)
                        {
                            mvLogMsg(MV_IAL_LOG_ID,  MV_DEBUG_ERROR, "scsi device %p created, bus %d lun %d\n",
                                     pScsiPhysDev, channelIndex, port);
                        }else{
                            mvLogMsg(MV_IAL_LOG_ID,  MV_DEBUG_ERROR, "failed to create scsi device. bus %d lun %x\n",
                                     channelIndex, port);
                        }
                    }
                }
            } /* if*/
        } /* for*/
	}/*while*/
}

/******************************************************************************
*
* mvSataThreadStateSet - set the state of a thread
*
* This is really just a place-holder for debugging and possible future
* enhancements such as state-change logging.
*
* RETURNS: N/A
*
* NOMANUAL
*/
LOCAL void mvSataThreadStateSet
    (
    MV_SATA_THREAD *   pThread,		/* ptr to thread info */
    SCSI_THREAD_STATE state
    )
{
    SCSI_THREAD * pScsiThread = (SCSI_THREAD *) pThread;
    
    SCSI_DEBUG_MSG ("mvSataThreadStateSet: thread 0x%08x: %d -> %d\n",
		    (int) pThread, pScsiThread->state, state, 0, 0, 0);

    pScsiThread->state = state;
}

/*******************************************************************************
*
* mvSataThreadComplete - successfully complete execution of a client thread
*
* Set the thread status and errno appropriately, depending on whether or
* not the thread has been aborted.  Set the thread inactive, and notify
* the SCSI manager of the completion.
*
* RETURNS: N/A
*
* NOMANUAL
*/
LOCAL void mvSataThreadComplete
    (
    MV_SATA_THREAD * pThread
    )
{
	SCSI_THREAD * pScsiThread = (SCSI_THREAD *) pThread;
	SCSI_CTRL*	  pScsiCtrl = (SCSI_CTRL*)pScsiThread->pScsiCtrl;

    SCSI_DEBUG_MSG ("mvSataThreadComplete: thread 0x%08x completed\n",
		    (int) pThread, 0, 0, 0, 0, 0);

    
	pScsiCtrl->peerBusId = NONE;
	pScsiCtrl->pThread   = 0;

	scsiMgrCtrlEvent (pScsiCtrl, SCSI_EVENT_DISCONNECTED);

    mvSataThreadStateSet (pThread, SCSI_THREAD_INACTIVE);

#ifdef CONFIG_MV_SP_I_FTCH_DB_INV

    if((pScsiThread->cmdAddress[0] == SCSI_OPCODE_READ6) || 
	   (pScsiThread->cmdAddress[0] == SCSI_OPCODE_READ10))
       mv_l2_inv_range ((UINT32)pScsiThread->activeDataAddress, 
						(UINT32)pScsiThread->activeDataAddress+pScsiThread->activeDataLength);
#endif

    scsiCacheSynchronize (pScsiThread, SCSI_CACHE_POST_COMMAND);

    scsiMgrThreadEvent (pScsiThread, SCSI_THREAD_EVENT_COMPLETED);


}


/*******************************************************************************
*
* mvSataEvent - SCSI Controller event processing routine
*
* Parse the event type and act accordingly.  Controller-level events are
* handled within this function, and the event is then passed to the current
* thread (if any) for thread-level processing.
*
* RETURNS: N/A
*/
LOCAL void mvSataEvent
    (
    SIOP *         pSiop,
    MV_SATA_EVENT * pEvent
    )
{
    MV_SATA_SCSI_CMD_BLOCK *pCmdBlock = pEvent->pCmdBlock;
	MV_COMMAND_INFO *pCommandInfo = (MV_COMMAND_INFO *)pCmdBlock->IALData;
	MV_SATA_THREAD * pScsiThread = (MV_SATA_THREAD *) pCommandInfo->pThread;
	SCSI_THREAD * pThread = (SCSI_THREAD *) pCommandInfo->pThread;
	SCSI_CTRL*	  pScsiCtrl = (SCSI_CTRL*)pSiop;
	SCSI_PHYS_DEV * pScsiPhysDev = pThread->pScsiPhysDev;
	
	SCSI_DEBUG_MSG ("mvSataEvent: pSiop 0x%08x pEvent 0x%08x pScsiThread 0x%08x pCmdBlock 0x%08x pScsiCtrl->pThread=0x%08x\n",
			(int) pSiop, (int)pEvent, (int)pScsiThread, (int)pCmdBlock
			 ,(int)pScsiCtrl->pThread,0);

    SCSI_DEBUG_MSG ("mvSataEvent: pEvent 0x%08x \n",
		    (int) pEvent, 0, 0, 0, 0, 0);


	SCSI_DEBUG_MSG ("pThread->state = SCSI_THREAD_%d\n",pThread->state,0,0,0,0,0);

	/* Set Scsi Status */
	*pThread->statusAddress = pCmdBlock->ScsiStatus;

    /* Set cache coherency type */
    if(ETHER_DRAM_COHER == MV_CACHE_COHER_SW)
        pThread->pScsiCtrl->cacheSnooping = FALSE;
	else
        pThread->pScsiCtrl->cacheSnooping = TRUE;

    switch (pCmdBlock->ScsiCommandCompletion)
	{
	case MV_SCSI_COMPLETION_SUCCESS:

		mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG, "Scsi command completed with MV_SCSI_COMPLETION_SUCCESS\n");

		/* Set Sense Data Length*/
		pScsiPhysDev->reqSenseDataLength = pCmdBlock->senseDataLength;
        memcpy(pScsiPhysDev->pReqSenseData, pCmdBlock->pSenseBuffer,
               pScsiPhysDev->reqSenseDataLength);
        /*pScsiPhysDev->pReqSenseData = pCmdBlock->pSenseBuffer;*/

		pThread->status = OK;
		pThread->errNum = 0;
		break;
	
	case MV_SCSI_COMPLETION_BAD_SCSI_COMMAND:
		mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG, "Scsi command completed with BAD_SCSI_COMMAND\n");
	
		pThread->status = ERROR;
		pThread->errNum = S_scsiLib_ILLEGAL_REQUEST;

		break;
	case MV_SCSI_COMPLETION_ATA_FAILED:
		mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG, "Scsi command completed with ATA FAILED\n");

		pThread->status = ERROR;
		pThread->errNum = S_scsiLib_ABORTED_COMMAND;

	
		break;
	case MV_SCSI_COMPLETION_UA_RESET:
		mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG, "Scsi command completed with UA BUS"
		   " RESET\n");

		pThread->status = ERROR;
		pThread->errNum = S_scsiLib_BUS_RESET;

	
		break;
	case MV_SCSI_COMPLETION_UA_PARAMS_CHANGED:
		mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG, "Scsi command completed with UA "
		   "PARAMETERS CHANGED\n");
	
		pThread->status = ERROR;
		pThread->errNum = S_scsiLib_ILLEGAL_PARAMETER;

		break;
	
	case MV_SCSI_COMPLETION_QUEUE_FULL:
		mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG, "Scsi command completed with QUEUE FULL\n");

		pThread->status = ERROR;
		pThread->errNum = S_scsiLib_ABORTED_COMMAND;

		break;
	
	case MV_SCSI_COMPLETION_ABORTED:
		mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG, "Scsi command completed with ABORTED\n");

		pThread->status = ERROR;
		pThread->errNum = S_scsiLib_ABORTED_COMMAND;

		break;
	
	case MV_SCSI_COMPLETION_OVERRUN:
		mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG, "Scsi command completed with OVERRUN\n");

		pThread->status = ERROR;
		pThread->errNum = S_scsiLib_ABORTED_COMMAND;

		break;
	case MV_SCSI_COMPLETION_UNDERRUN:
		mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG, "Scsi command completed with UNDERRUN\n");

		pThread->status = ERROR;
		pThread->errNum = S_scsiLib_ILLEGAL_PARAMETER;

		break;
	
	case MV_SCSI_COMPLETION_PARITY_ERROR:
		mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG_ERROR, "Scsi command completed with PARITY ERROR\n");
	
		pThread->status = ERROR;
		pThread->errNum = S_scsiLib_ILLEGAL_PARAMETER;

		break;
	case MV_SCSI_COMPLETION_INVALID_BUS:
		mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG, "Scsi command completed with INVALID BUS\n");

		pThread->status = ERROR;
		pThread->errNum = S_scsiLib_ILLEGAL_BUS_ID;

		break;
	case MV_SCSI_COMPLETION_NO_DEVICE:
		mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG, "Scsi command completed with NO DEVICE\n");

		pThread->status = ERROR;
		pThread->errNum = S_scsiLib_ILLEGAL_BUS_ID;
	
		break;
	case MV_SCSI_COMPLETION_INVALID_STATUS:
	case MV_SCSI_COMPLETION_BAD_SCB:
	case MV_SCSI_COMPLETION_NOT_READY:
	case MV_SCSI_COMPLETION_DISCONNECT:
	case MV_SCSI_COMPLETION_BUS_RESET:
	case MV_SCSI_COMPLETION_BUSY:
	default:
		mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG_ERROR,"Bad Scsi completion status %d\n",
		   pCmdBlock->ScsiCommandCompletion);

		pThread->status = ERROR;
		pThread->errNum = S_scsiLib_ILLEGAL_PARAMETER;
	
	}


	mvSataThreadComplete(pScsiThread);

	if ((pCmdBlock->senseDataLength == 0) && (pCmdBlock->senseBufferLength))
	{
		pCmdBlock->pSenseBuffer[0] = 0;
	}
	

	
	mrvlReleaseCib(pCommandInfo);
	
	/*mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG," complete pCommandInfo %p command\n",
				pCommandInfo);*/


}
    
/*******************************************************************************
*
* mvSataThreadInit - initialize a client thread structure
*
* Initialize the fixed data for a thread (i.e., independent of the command).
* Called once when a thread structure is first created.
*
* RETURNS: OK, or ERROR if an error occurs
*/

LOCAL STATUS mvSataThreadInit
    (
    SIOP *          pSiop,
    MV_SATA_THREAD * pThread
    )
    {

	/*SCSI_DEBUG_MSG ("mvSataThreadInit: thread 0x%08x: Initing\n",
			(int) pThread, 0, 0, 0, 0, 0);*/

    if (scsiThreadInit (&pThread->scsiThread) != OK)
	{
		SCSI_DEBUG_MSG ("mvSataThreadInit: Error creating thread 0x%08x: Initing\n",
				(int) pThread, 0, 0, 0, 0, 0);

		return (ERROR);
	}



    /* TODO - device specific thread initialization */



    return (OK);
    }

/*******************************************************************************
*
* mvSataThreadActivate - activate a SCSI connection for an initiator thread
*
* Set whatever thread/controller state variables need to be set.  Ensure that
* all buffers used by the thread are coherent with the contents of the
* system caches (if any).
*
* Set transfer parameters for the thread based on what its target device
* last negotiated.
*
* Update the thread context (including shared memory area) and note that
* there is a new client script to be activated (see "mvSataActivate()").
*
* Set the thread's state to ESTABLISHED.
* Do not wait for the script to be activated.  Completion of the script will
* be signalled by an event which is handled by "mvSataEvent()".
*
* RETURNS: OK or ERROR
*/

LOCAL STATUS mvSataThreadActivate
    (
    SIOP *          pSiop,		/* ptr to controller info */
    MV_SATA_THREAD * pThread		/* ptr to thread info     */
    )
    {

    SCSI_CTRL *   pScsiCtrl   = (SCSI_CTRL *)   pSiop;
    SCSI_THREAD * pScsiThread = (SCSI_THREAD *) pThread;
    SCSI_TARGET * pScsiTarget = pScsiThread->pScsiTarget;
	SCSI_PHYS_DEV * pScsiPhysDev = pScsiThread->pScsiPhysDev;
	MV_SATA_SCSI_CMD_BLOCK           *pScb;
    MV_COMMAND_INFO* mvCmdInfo;
	SCSI_THREAD * pOrigThread;


    SCSI_DEBUG_MSG ("mvSataThreadActivate: thread 0x%08x: activating\n",
		    (int) pThread, 0, 0, 0, 0, 0);
	/*mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG, "mvSataThreadActivate: pSiop =0x%08x  thread 0x%08x pScsiPhysDev 0x%08x: activating\n",
				(int)pSiop, (int) pThread,(int)pScsiPhysDev);*/


	semTake(pSiop->pSataInfo->mutualSem, WAIT_FOREVER);
    /*
     *	Reset controller state variables: set sync xfer parameters
     */
    pScsiCtrl->msgOutState = SCSI_MSG_OUT_NONE;
    pScsiCtrl->msgInState  = SCSI_MSG_IN_NONE;

	mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG, "mvSataThreadActivate: pScsiCtrl->pThread 0x%08x p\n",
				(int) pScsiCtrl->pThread);

    pOrigThread = pScsiCtrl->pThread;
    pScsiCtrl->pThread = pScsiThread;


	SCSI_DEBUG_MSG ("mvSataThreadActivate: pScsiCtrl->pThread 0x%08x p\n",
				(int) pScsiCtrl->pThread,0,0,0,0,0);



    scsiSyncXferNegotiate (pScsiCtrl, pScsiTarget, SYNC_XFER_NEW_THREAD);

	mvCmdInfo = mrvlAllocateCib(pSiop->pSataInfo);
	

	mvCmdInfo->pThread = (UINT32) pThread;

	mvCmdInfo->xferVirtMem = (UINT32)pScsiThread->activeDataAddress;
	mvCmdInfo->xferPhysMem = CPU_PHY_MEM(mvCmdInfo->xferVirtMem);
	mvCmdInfo->xferLen = pScsiThread->activeDataLength;


	mvCmdInfo->sataInfo = pSiop->pSataInfo;

	pScb = &mvCmdInfo->scb;

	pScb->IALData = mvCmdInfo;

	if ((pScsiTarget->scsiDevBusId >= pScsiCtrl->scsiCtrlBusId)
		/*||(pScsiPhysDev->scsiDevLUN > 0)*/)
	{


		pScb->ScsiCommandCompletion = MV_SCSI_COMPLETION_INVALID_BUS;
		IALCompletion(&pSiop->pSataInfo->mvSataAdapter,	pScb);
		semGive(pSiop->pSataInfo->mutualSem);
		return (OK);


	}


	mrvlSetupPrdTable(pSiop->pSataInfo , mvCmdInfo);

	mvCmdInfo->flags = 0;
	mvCmdInfo->flags |= MRVL_COMMAND_INFO_FLAGE_DMA;
	mvCmdInfo->flags |= MRVL_COMMAND_INFO_FLAGE_PRD_TABLE;

	pScb->bus = pScsiTarget->scsiDevBusId;
    pScb->target = pScsiPhysDev->scsiDevLUN; /*pScsiTarget->scsiDevBusId;*/
	pScb->lun = 0;/*pScsiPhysDev->scsiDevLUN;*/


    pScb->pSalAdapterExtension = &pSiop->pSataInfo->ataScsiAdapterExt;
    pScb->pIalAdapterExtension = &pSiop->pSataInfo->ialCommonExt;
    pScb->completionCallBack = IALCompletion;
    pScb->dataBufferLength =  mvCmdInfo->xferLen;
    pScb->pDataBuffer =  (MV_U8*)mvCmdInfo->xferVirtMem;
    pScb->ScsiCdb = pScsiThread->cmdAddress;
    pScb->ScsiCdbLength = (UINT32)pScsiThread->cmdLength;

	/* Set Sense buffer information */	
    pScb->pSenseBuffer = &mvCmdInfo->senseBuffer[0];
	pScb->senseBufferLength = MV_MAX_SENSE_BUFFER;

	
	SCSI_DEBUG_MSG ("mvSataThreadActivate: pScb->pSenseBuffer 0x%08x pScb->senseBufferLength 0x%08x\n",
				(int) pScb->pSenseBuffer,pScb->senseBufferLength,0,0,0,0);
 
	SCSI_DEBUG_MSG ("mvSataThreadActivate: pScsiThread->identMsgLength 0x%08x\n",
				(int) pScsiThread->identMsgLength,0,0,0,0,0);

	SCSI_DEBUG_MSG ("mvSataThreadActivate: pScsiThread->identMsgLength 0x%08x\n",
				(int) pScsiThread->identMsgLength,0,0,0,0,0);

	

	if ((sataDebugIdMask)||(sataDebugTypeMask))
	{
		if (pScb->lun == 0)
		{
			/*mvMicroSecondsDelay(NULL,4000000);*/
		}

	}


    scsiCacheSynchronize (pScsiThread, SCSI_CACHE_PRE_COMMAND);
    
    if((pScb->ScsiCdb[0] == SCSI_OPCODE_READ6) || (pScb->ScsiCdb[0] == SCSI_OPCODE_READ10))
       mvOsCacheInvalidate (NULL, pScb->pDataBuffer, pScb->dataBufferLength);
	
    if((pScb->ScsiCdb[0] == SCSI_OPCODE_WRITE6) || (pScb->ScsiCdb[0] == SCSI_OPCODE_WRITE10))
          mvOsCacheFlush (NULL, pScb->pDataBuffer, pScb->dataBufferLength);    
    

    mvExecuteScsiCommand(pScb, MV_TRUE);

	semGive(pSiop->pSataInfo->mutualSem);
    return (OK);
    }

/*******************************************************************************
*
* mvSataThreadAbort - abort a thread
*
* If the thread is not currently connected, do nothing and return FALSE to
* indicate that the SCSI manager should abort the thread.
*
* RETURNS: TRUE if the thread is being aborted by this driver (i.e. it is
* currently active on the controller, else FALSE.
*/

LOCAL BOOL mvSataThreadAbort
    (
    SIOP *          pSiop,		/* ptr to controller info */
    MV_SATA_THREAD * pThread		/* ptr to thread info     */
    )
    {

	SCSI_DEBUG_MSG ("mvSataThreadAbort: pSiop 0x%08x thread 0x%08x\n",
			(int)pSiop ,(int) pThread, 0, 0, 0, 0);

    mvResetBus(pSiop->pSataInfo, pThread->scsiThread.pScsiTarget->scsiDevBusId);
    return (TRUE);
}

LOCAL STATUS mvResetBus(SATA_INFO *pSataInfo, int channelIndex)
{
        SCSI_DEBUG_MSG("Bus Reset: host=%d, channel=%d, target=%d\n",
                      pSataInfo->mvSataAdapter.adapterId, channelIndex,
                      0, 0, 0, 0);

       semTake(pSataInfo->mutualSem, WAIT_FOREVER);
       if (pSataInfo->mvSataAdapter.sataChannel[channelIndex] == NULL)
       {
           SCSI_DEBUG_MSG("trying to reset disabled channel\n",
                          0, 0, 0, 0, 0, 0);
           semGive(pSataInfo->mutualSem);
           return ERROR;
    }

       mvSataDisableChannelDma(&pSataInfo->mvSataAdapter, channelIndex);

       /* Flush pending commands */
       mvSataFlushDmaQueue (&pSataInfo->mvSataAdapter, channelIndex, MV_FLUSH_TYPE_CALLBACK);

       /* Hardware reset channel */
       mvSataChannelHardReset(&pSataInfo->mvSataAdapter, channelIndex);

       if (pSataInfo->mvSataAdapter.sataChannel[channelIndex])
       {
           mvRestartChannel(&pSataInfo->ialCommonExt, channelIndex,
                            &pSataInfo->ataScsiAdapterExt, MV_TRUE);
       }
       semGive(pSataInfo->mutualSem);
       return OK;
}
/*******************************************************************************
*
* mvSataScsiBusControl - miscellaneous low-level SCSI bus control operations
*
* Currently supports only the SCSI_BUS_RESET operation; other operations are
* not used explicitly by the driver because they are carried out automatically
* by the script program.
*
* NOTE: after the SCSI bus has been reset, we expect the device
* to create a MV_SATA_BUS_RESET event to be sent to the SCSI manager.
* See "mvSataIntr()".
*
* RETURNS: OK, or ERROR if an invalid operation is requested.
*
* NOMANUAL
*/

LOCAL STATUS mvSataScsiBusControl
    (
    SIOP * pSiop,   	    /* ptr to controller info                   */
    int    operation	    /* bitmask for operation(s) to be performed */
    )
    {
    SATA_INFO*		pSataInfo = pSiop->pSataInfo;
    int channelIndex;
	SCSI_DEBUG_MSG ("mvSataScsiBusControl: pSiop 0x%08x operation 0x%08x\n",
			(int)pSiop ,(int) operation, 0, 0, 0, 0);

    if (operation & SCSI_BUS_RESET)
	{
        for(channelIndex = 0; channelIndex < pSataInfo->mvSataAdapter.numberOfChannels; channelIndex++)
            mvResetBus(pSataInfo, channelIndex);
	}

    return ERROR; /* unknown operation */
    }

/*******************************************************************************
*
* mvSataXferParamsQuery - get synchronous transfer parameters
*
* Updates the synchronous transfer parameters suggested in the call to match
* the TEMPLATE SCSI Controller's capabilities.  Transfer period is in SCSI units
* (multiples * of 4 ns).
*
* RETURNS: OK
*/

LOCAL STATUS mvSataXferParamsQuery
    (
    SCSI_CTRL *pScsiCtrl,		/* ptr to controller info       */
    UINT8     *pOffset,			/* max REQ/ACK offset  [in/out] */
    UINT8     *pPeriod			/* min transfer period [in/out] */
    )
    {

	SCSI_DEBUG_MSG ("mvSataXferParamsQuery: pScsiCtrl 0x%08x pOffset 0x%08x pPeriod 0x%08x\n",
			(int)pScsiCtrl ,(int) pOffset, (int)pPeriod, 0, 0, 0);

    /* TODO - alter the offset and period variables to acceptable values */
	*pOffset = 0;
	*pPeriod = 4;
    return (OK);
    }
    
/*******************************************************************************
*
* mvSataXferParamsSet - set transfer parameters
*
* Validate the requested parameters, convert to the TEMPLATE SCSI Controller's 
* native format and save in the current thread for later use (the chip's
* registers are not actually set until the next script activation for this 
* thread).
*
* Transfer period is specified in SCSI units (multiples of 4 ns).  An offset
* of zero specifies asynchronous transfer.
*
* RETURNS: OK if transfer parameters are OK, else ERROR.
*/

LOCAL STATUS mvSataXferParamsSet
    (
    SCSI_CTRL *pScsiCtrl,		/* ptr to controller info */
    UINT8      offset,			/* max REQ/ACK offset     */
    UINT8      period			/* min transfer period    */
    )
    {
	SCSI_DEBUG_MSG ("mvSataXferParamsSet: pScsiCtrl 0x%08x offset 0x%08x period 0x%08x\n",
			(int)pScsiCtrl ,(int) offset, (int)period, 0, 0, 0);

		return (OK);
    }

/*******************************************************************************
*
* mvSataWideXferParamsQuery - get wide data transfer parameters
*
* Updates the wide data transfer parameters suggested in the call to match
* the TEMPLATE SCSI Controller's capabilities.  Transfer width is in the units 
* of the WIDE DATA TRANSFER message's transfer width exponent field. This is
* an 8 bit field where 0 represents a narrow transfer of 8 bits, 1 represents
* a wide transfer of 16 bits and 2 represents a wide transfer of 32 bits.
*
* RETURNS: OK
*/

LOCAL STATUS mvSataWideXferParamsQuery
    (
    SCSI_CTRL *pScsiCtrl,		/* ptr to controller info       */
    UINT8     *xferWidth		/* suggested transfer width     */
    )
    {
		SCSI_DEBUG_MSG ("mvSataWideXferParamsQuery: pScsiCtrl 0x%08x\n",
				(int)pScsiCtrl ,0,0 ,0, 0, 0);

    return (OK);
    }

/*******************************************************************************
*
* mvSataWideXferParamsSet - set wide transfer parameters
*
* Assume valid parameters and set the TEMPLATE's thread parameters to the
* appropriate values. The actual registers are not written yet, but will
* be written from the thread values when it is activated.
*
* Transfer width is specified in SCSI transfer width exponent units. 
*
* RETURNS: OK 
*/

LOCAL STATUS mvSataWideXferParamsSet
    (
    SCSI_CTRL *pScsiCtrl,		/* ptr to controller info */
    UINT8      xferWidth 		/* wide data transfer width */
    )
    {

		SCSI_DEBUG_MSG ("mvSataWideXferParamsSet: pScsiCtrl 0x%08x\n",
				(int)pScsiCtrl ,0,0 ,0, 0, 0);

    /* TODO - update thread to enable/disable wide data xfers */

    return OK;
    }





/************************************************************************************/
/*																					*/
/* 				Marvell Sata Core Driver related function							*/
/*																					*/
/************************************************************************************/




/**********************************************************************************/

MV_BOOLEAN IALCompletion(struct mvSataAdapter *pSataAdapter,
                         MV_SATA_SCSI_CMD_BLOCK *pCmdBlock)
{
	MV_SATA_EVENT	event;
	SCSI_EVENT*		pScsiEvent = (SCSI_EVENT *) &event.scsiEvent;
	SATA_INFO*		pSataInfo = (SATA_INFO*)pSataAdapter->IALData;
	SIOP			*pSiop = (SIOP*)pSataInfo->mvChannelData[pCmdBlock->bus].osData;
  
	mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG, "IALCompletion: pSataAdapter 0x%08x pCmdBlock 0x%08x\n",
			(int)pSataAdapter ,(int) pCmdBlock);


	pScsiEvent->busId= pCmdBlock->bus;
	pScsiEvent->type = SCSI_EVENT_CONNECTED;
	pScsiEvent->phase = SCSI_COMMAND_PHASE;
	
	event.pCmdBlock = pCmdBlock;
  
	
	/* Send the event to the SCSI manager to be processed. */
	scsiMgrEventNotify ((SCSI_CTRL *) pSiop, pScsiEvent, sizeof (event));
	
	
	return MV_TRUE;

}
/**********************************************************************************/
MV_BOOLEAN IALBusChangeNotify(MV_SATA_ADAPTER *pSataAdapter,
                              MV_U8 channelIndex)
{
	SCSI_DEBUG_MSG ("IALBusChangeNotify: pSataAdapter 0x%08x channelIndex 0x%08x\n",
			(int)pSataAdapter ,(int) channelIndex, 0 , 0, 0, 0);

    return MV_TRUE;
}
/**********************************************************************************/
MV_BOOLEAN IALBusChangeNotifyEx(MV_SATA_ADAPTER *pSataAdapter,
                                MV_U8 channelIndex,
                                MV_U16 targetsToRemove,
                                MV_U16 targetsToAdd)
{
    SATA_INFO *sc = (SATA_INFO *)pSataAdapter->IALData;
        
	SCSI_DEBUG_MSG ("IALBusChangeNotifyEx: pSataAdapter 0x%08x channelIndex 0x%08x\n",
			(int)pSataAdapter ,(int) channelIndex, 0 , 0, 0, 0);

    sc->mvChannelData[channelIndex].targetsToRemove &= ~targetsToAdd;
    sc->mvChannelData[channelIndex].targetsToRemove |= targetsToRemove;
    sc->mvChannelData[channelIndex].targetsToAdd |= targetsToAdd;
    sc->mvChannelData[channelIndex].targetsToAdd &= ~targetsToRemove;

    SCSI_DEBUG_MSG ("IALBusChangeNotifyEx: targets to add 0x%04x to remove 0x%04x\n",
                    (int)sc->mvChannelData[channelIndex].targetsToAdd,
                    (int)sc->mvChannelData[channelIndex].targetsToRemove, 0 , 0, 0, 0);
    semGive(sc->pSataScsiCtrl->rescanSem);
    return MV_TRUE;
}
/**********************************************************************************/
MV_BOOLEAN IALConfigQueuingMode(MV_SATA_ADAPTER *pSataAdapter,
                                MV_U8 channelIndex,
                                MV_EDMA_MODE mode,
                                MV_SATA_SWITCHING_MODE switchingMode,
                                MV_BOOLEAN  use128Entries)

{
SCSI_DEBUG_MSG ("IALConfigQueuingMode: pSataAdapter 0x%08x channelIndex 0x%08x\n",
		(int)pSataAdapter ,(int) channelIndex, 0 , 0, 0, 0);

  mvSataConfigEdmaMode(pSataAdapter, channelIndex, mode, 31);
  return MV_TRUE;
}

/**********************************************************************************/

MV_BOOLEAN IALInitChannel(MV_SATA_ADAPTER *pSataAdapter,
						  MV_U8 channelIndex)
{
	SATA_INFO *sc = (SATA_INFO *)pSataAdapter->IALData;

	SCSI_DEBUG_MSG ("IALInitChannel: pSataAdapter 0x%08x channelIndex 0x%08x\n",
			(int)pSataAdapter ,(int) channelIndex, 0 , 0, 0, 0);
	
	if (channelIndex >= pSataAdapter->numberOfChannels)
	{
		mvLogMsg(MV_IAL_LOG_ID,  MV_DEBUG_ERROR, "[%d]: Bad channelIndex =%d",
		   pSataAdapter->adapterId, channelIndex);
		return MV_FALSE;
	}
	
	pSataAdapter->sataChannel[channelIndex] = &sc->mvChannelData[channelIndex].mvSataChannel;
	
	
	return MV_TRUE;
}
/**********************************************************************************/
void IALReleaseChannel(MV_SATA_ADAPTER *pSataAdapter, MV_U8 channelIndex)
{
	SCSI_DEBUG_MSG ("IALReleaseChannel: pSataAdapter 0x%08x channelIndex 0x%08x\n",
			(int)pSataAdapter ,(int) channelIndex, 0 , 0, 0, 0);

  pSataAdapter->sataChannel[channelIndex] = NULL;
  return;
}
/**********************************************************************************/
/**/
void IALChannelCommandsQueueFlushed(MV_SATA_ADAPTER *pSataAdapter, MV_U8 channelIndex)
{
	SCSI_DEBUG_MSG ("IALChannelCommandsQueueFlushed: pSataAdapter 0x%08x channelIndex 0x%08x\n",
			(int)pSataAdapter ,(int) channelIndex, 0 , 0, 0, 0);

  return;
}
/**********************************************************************************/


void mvIalInitLog(void)
{
#ifdef MV_LOGGER
    char *szModules[] = {
      "Core Driver",
      "SAL",
      "Common IAL",
      "VxWorks IAL"
    };
#if defined (MV_LOG_DEBUG)
    mvLogRegisterModule(MV_CORE_DRIVER_LOG_ID, 0x1FF,
                        szModules[MV_CORE_DRIVER_LOG_ID]);
    mvLogRegisterModule(MV_SAL_LOG_ID, 0x1FF,
                        szModules[MV_SAL_LOG_ID]);
    mvLogRegisterModule(MV_IAL_COMMON_LOG_ID, 0x1FF,
                        szModules[MV_IAL_COMMON_LOG_ID]);
    mvLogRegisterModule(MV_IAL_LOG_ID, 0x1FF,
                        szModules[MV_IAL_LOG_ID]);
#elif defined (MV_LOG_ERROR)
    mvLogRegisterModule(MV_CORE_DRIVER_LOG_ID, MV_DEBUG_FATAL_ERROR|MV_DEBUG_ERROR,
                        szModules[MV_CORE_DRIVER_LOG_ID]);
    mvLogRegisterModule(MV_SAL_LOG_ID, MV_DEBUG_FATAL_ERROR|MV_DEBUG_ERROR,
                        szModules[MV_SAL_LOG_ID]);
    mvLogRegisterModule(MV_IAL_COMMON_LOG_ID, MV_DEBUG_FATAL_ERROR|MV_DEBUG_ERROR,
                        szModules[MV_IAL_COMMON_LOG_ID]);
    mvLogRegisterModule(MV_IAL_LOG_ID, MV_DEBUG_FATAL_ERROR|MV_DEBUG_ERROR,
                        szModules[MV_IAL_LOG_ID]);
#endif
#endif
}


/**********************************************************************************/


void initMvSataAdapter(SATA_INFO  *pSataInfo)
{
    MV_SATA_ADAPTER  *pMvSataAdapter = &pSataInfo->mvSataAdapter;
    
    memset(pMvSataAdapter, 0, sizeof(MV_SATA_ADAPTER));
    pMvSataAdapter->adapterIoBaseAddress = pSataInfo->regBase;
    pMvSataAdapter->adapterId = pSataInfo->unit;
    pMvSataAdapter->pciConfigRevisionId = pSataInfo->pciDev.chipRev;
    pMvSataAdapter->pciConfigDeviceId = pSataInfo->pciDev.deviceId;
    pMvSataAdapter->intCoalThre[0]= MV_IAL_SACOALT_DEFAULT;
    pMvSataAdapter->intCoalThre[1]= MV_IAL_SACOALT_DEFAULT;
    pMvSataAdapter->intTimeThre[0] = MV_IAL_SAITMTH_DEFAULT;
    pMvSataAdapter->intTimeThre[1] = MV_IAL_SAITMTH_DEFAULT;
    pMvSataAdapter->pciCommand = MV_PCI_COMMAND_REG_DEFAULT;
    pMvSataAdapter->pciSerrMask = MV_PCI_SERR_MASK_REG_ENABLE_ALL;
    pMvSataAdapter->pciInterruptMask = MV_PCI_INTERRUPT_MASK_REG_ENABLE_ALL;
    pMvSataAdapter->mvSataEventNotify = mvIalLibEventNotify;
    pMvSataAdapter->IALData = pSataInfo;
    
}

/**********************************************************************************/

MV_BOOLEAN mvIalLibEventNotify(MV_SATA_ADAPTER *pMvSataAdapter, 
							   MV_EVENT_TYPE eventType,
                               MV_U32 param1, MV_U32 param2)
{

  SATA_INFO *sc = (SATA_INFO *)pMvSataAdapter->IALData;
  MV_U8   channel = param2;

    switch (eventType)
    {
    case MV_EVENT_TYPE_SATA_CABLE:
        {
            if (param1 == MV_SATA_CABLE_EVENT_CONNECT)
            {

                mvLogMsg(MV_IAL_LOG_ID,  MV_DEBUG, "[%d,%d]: device connected event received\n",
                         pMvSataAdapter->adapterId, channel);
                mvRestartChannel(&sc->ialCommonExt, channel,
                                 &sc->ataScsiAdapterExt, MV_FALSE);

            }
            else if (param1 == MV_SATA_CABLE_EVENT_DISCONNECT)
            {
                mvLogMsg(MV_IAL_LOG_ID,  MV_DEBUG, "[%d,%d]: device disconnected event received \n",
                         pMvSataAdapter->adapterId, channel);
                if (mvSataIsStorageDeviceConnected(pMvSataAdapter, channel, NULL) ==
                    MV_FALSE)
                {
                    mvStopChannel(&sc->ialCommonExt, channel,
                                  &sc->ataScsiAdapterExt);
                }
                else
                {
                    mvLogMsg(MV_IAL_LOG_ID,  MV_DEBUG, "[%d,%d]: device disconnected event ignored.\n",
                             pMvSataAdapter->adapterId, channel);
                }

            }
            else if (param1 == MV_SATA_CABLE_EVENT_PM_HOT_PLUG)
            {
                mvPMHotPlugDetected(&sc->ialCommonExt, channel,
                                    &sc->ataScsiAdapterExt);

            }
            else
            {

                mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG_ERROR, "illegal value for param1(%d) at "
                         "connect/disconect event, host=%d\n", param1,
                         pMvSataAdapter->adapterId );
            }
        }
        break;
    case MV_EVENT_TYPE_ADAPTER_ERROR:
        mvLogMsg(MV_IAL_LOG_ID,  MV_DEBUG_ERROR, "DEVICE error event received, pci cause "
                 "reg=%x, don't know how to handle this\n", param1);
        return MV_TRUE;
    case MV_EVENT_TYPE_SATA_ERROR:
        switch (param1)
        {
        case MV_SATA_RECOVERABLE_COMMUNICATION_ERROR:
            mvLogMsg(MV_IAL_LOG_ID,  MV_DEBUG_ERROR,
                     " [%d %d] sata recoverable error occured\n",
                     pMvSataAdapter->adapterId, channel);
            break;
        case MV_SATA_UNRECOVERABLE_COMMUNICATION_ERROR:
            mvLogMsg(MV_IAL_LOG_ID,  MV_DEBUG_ERROR,
                     " [%d %d] sata unrecoverable error occured, restart channel\n",
                     pMvSataAdapter->adapterId, channel);
            mvSataChannelHardReset(pMvSataAdapter, channel);
            mvRestartChannel(&sc->ialCommonExt, channel,
                             &sc->ataScsiAdapterExt, MV_TRUE);

            break;
        case MV_SATA_DEVICE_ERROR:
            mvLogMsg(MV_IAL_LOG_ID,  MV_DEBUG_ERROR,
                     " [%d %d] device error occured\n",
                     pMvSataAdapter->adapterId, channel);
            break;
        }
        break;
    default:
        mvLogMsg(MV_IAL_LOG_ID,  MV_DEBUG_ERROR,  " adapter %d unknown event %d"
                 " param1= %x param2 = %x\n", pMvSataAdapter->adapterId,
                 eventType - MV_EVENT_TYPE_ADAPTER_ERROR, param1, param2);
        return MV_FALSE;

    }/*switch*/

    return MV_TRUE;
}

/**********************************************************************************/


int mrvlDmaMemInitChannels(SATA_INFO *sc)
{

	MV_SATA_ADAPTER       *pMvSataAdapter = &sc->mvSataAdapter;
	int i,j;
	
	UINT32    requestQSize;
	UINT32	requestQAlignment;
	UINT32    responseQSize;
	UINT32    responseQAlignment;
	UINT32	  TotalSizeUncached;
	
	if ( pMvSataAdapter->sataAdapterGeneration >= MV_SATA_GEN_IIE )
	{
	  requestQSize = MV_EDMA_GEN2E_REQUEST_QUEUE_SIZE;
	  requestQAlignment = MV_EDMA_GEN2E_REQUEST_QUEUE_SIZE;
	  responseQSize = MV_EDMA_GEN2E_RESPONSE_QUEUE_SIZE;
	  responseQAlignment = MV_EDMA_GEN2E_RESPONSE_QUEUE_SIZE;
	}
	else
	{
	  requestQSize = MV_EDMA_REQUEST_QUEUE_SIZE;
	  requestQAlignment = MV_EDMA_REQUEST_QUEUE_SIZE;
	  responseQSize = MV_EDMA_RESPONSE_QUEUE_SIZE;
	  responseQAlignment = MV_EDMA_RESPONSE_QUEUE_SIZE;
	}

	mvLogMsg(MV_IAL_LOG_ID,  MV_DEBUG_INIT,
			 "requestQAlignment 0x%x responseQAlignment 0x%x\n",requestQAlignment,responseQAlignment);
	TotalSizeUncached  = (1+MV_EDMA_GEN2E_REQUEST_QUEUE_SIZE) * pMvSataAdapter->numberOfChannels ;
	TotalSizeUncached += MV_EDMA_GEN2E_RESPONSE_QUEUE_SIZE    * pMvSataAdapter->numberOfChannels;
	TotalSizeUncached += MRVL_MAX_CMDS_PER_ADAPTER *(MRVL_MAX_SG_LIST * MV_EDMA_PRD_ENTRY_SIZE);

	sc->UncacheVirtMemoryBuffer = (UINT32)mvOsIoUncachedMalloc(NULL,TotalSizeUncached,&sc->UncachePhyMemoryBuffer,NULL);
	if (sc->UncacheVirtMemoryBuffer == 0)
	{
		mvOsPrintf("Alloc uncached memory buffer faild for size=0x%x\n", TotalSizeUncached);
		return -1;
	}

	sc->UncacheVirtMemoryBuffer = (MV_ULONG )MV_ALIGN_UP(sc->UncacheVirtMemoryBuffer, MV_EDMA_GEN2E_REQUEST_QUEUE_SIZE);
	sc->UncachePhyMemoryBuffer  = (MV_ULONG )MV_ALIGN_UP(sc->UncachePhyMemoryBuffer , MV_EDMA_GEN2E_REQUEST_QUEUE_SIZE);

	 
	for(i = 0; i < pMvSataAdapter->numberOfChannels; i++)
	{

		sc->mvChannelData[i].requestVirtMem = sc->UncacheVirtMemoryBuffer   + i*MV_EDMA_GEN2E_REQUEST_QUEUE_SIZE;
		sc->mvChannelData[i].requestPhysMem = sc->UncachePhyMemoryBuffer+ i*MV_EDMA_GEN2E_REQUEST_QUEUE_SIZE; 


		j = pMvSataAdapter->numberOfChannels * MV_EDMA_GEN2E_REQUEST_QUEUE_SIZE;
		sc->mvChannelData[i].respondVirtMem = sc->UncacheVirtMemoryBuffer + j + i*MV_EDMA_GEN2E_RESPONSE_QUEUE_SIZE;
		sc->mvChannelData[i].respondPhysMem	= sc->UncachePhyMemoryBuffer  + j + i*MV_EDMA_GEN2E_RESPONSE_QUEUE_SIZE; 


		
		sc->mvChannelData[i].mvSataChannel.channelNumber = i;
	   
		sc->mvChannelData[i].mvSataChannel.requestQueue = 
		(struct mvDmaRequestQueueEntry *)sc->mvChannelData[i].requestVirtMem;

		sc->mvChannelData[i].mvSataChannel.requestQueuePciLowAddress = sc->mvChannelData[i].requestPhysMem;
		sc->mvChannelData[i].mvSataChannel.requestQueuePciHiAddress = 0; 

	
		mvLogMsg(MV_IAL_LOG_ID,  MV_DEBUG_INIT,
				 " channel %d request q phy addr %x\n", i,
		  sc->mvChannelData[i].mvSataChannel.requestQueuePciLowAddress );
	
		sc->mvChannelData[i].mvSataChannel.responseQueue = 
		(struct mvDmaResponseQueueEntry *)sc->mvChannelData[i].respondVirtMem;

		sc->mvChannelData[i].mvSataChannel.responseQueuePciLowAddress = sc->mvChannelData[i].respondPhysMem;
		sc->mvChannelData[i].mvSataChannel.responseQueuePciHiAddress = 0; 

		mvLogMsg(MV_IAL_LOG_ID,  MV_DEBUG_INIT,
				 " channel %d response q phy addr %x\n", i,sc->mvChannelData[i].mvSataChannel.responseQueuePciLowAddress );
	
	   
	}
	return 0;
}

/**********************************************************************************/


int mrvlPrdPoolAllocate(SATA_INFO *sc)
{
	MV_SATA_ADAPTER       *pMvSataAdapter = &sc->mvSataAdapter;
	int i,j;

	j = pMvSataAdapter->numberOfChannels * (MV_EDMA_GEN2E_REQUEST_QUEUE_SIZE + MV_EDMA_GEN2E_RESPONSE_QUEUE_SIZE);
	for(i = 0; i< MRVL_MAX_CMDS_PER_ADAPTER; i++)
	{

		sc->commandsInfoBlocks[i].mvPrdVirtMem = sc->UncacheVirtMemoryBuffer+ j + i*(MRVL_MAX_SG_LIST * MV_EDMA_PRD_ENTRY_SIZE);
		sc->commandsInfoBlocks[i].mvPrdPhysMem = sc->UncachePhyMemoryBuffer + j + i*(MRVL_MAX_SG_LIST * MV_EDMA_PRD_ENTRY_SIZE); 
	}
	
	return 0;
}
/**********************************************************************************/



#ifdef MV_SATA_TIMER_AS_TASK

int asyncStartTimerFunction(void *arg)
{
    SATA_INFO *sc = (SATA_INFO *)arg;

    SCSI_DEBUG_MSG ("asyncStartTimerFunction: arg 0x%08x: \n",
		    (int) arg, 0, 0, 0, 0, 0);

	while(1)
	{
        semTake(sc->mutualSem, WAIT_FOREVER);
        mvIALTimerCallback(&sc->ialCommonExt,
							&sc->ataScsiAdapterExt);
		semGive(sc->mutualSem);
	
		/*mvMicroSecondsDelay(NULL,500000);*/
        taskDelay(sysClkRateGet() / 2);
	}
	return 0;
}
#else

int asyncStartTimerFunction(void *arg)
{
    SATA_INFO *sc = (SATA_INFO *)arg;

    /*SCSI_DEBUG_MSG ("asyncStartTimerFunction: arg 0x%08x: \n",
		    (int) arg, 0, 0, 0, 0, 0);*/

    mvIALTimerCallback(&sc->ialCommonExt,
						&sc->ataScsiAdapterExt);

	wdStart (sc->sataWatchDogId, sysClkRateGet()/2 , asyncStartTimerFunction, (int)sc);
	return 0;
}
#endif


/**********************************************************************************/

void mrvlInitCibs(SATA_INFO *sc)
{
  int i= 0;

  for ( i = 0; i < MRVL_MAX_CMDS_PER_ADAPTER; i++)
  {
      sc->commandsInfoArray[i] = &sc->commandsInfoBlocks[i];
  }

  sc->cibsTop = MRVL_MAX_CMDS_PER_ADAPTER;

}
/**********************************************************************************/

MV_COMMAND_INFO *mrvlAllocateCib(SATA_INFO *sc)
{
  if (sc->cibsTop <= 0)
  {
        mvOsPrintf("out of SCBs !!\n");
		return NULL;
  }

  --(sc->cibsTop);


  mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG,"allocate pCommandInfo %p from %d\n",
	   sc->commandsInfoArray[sc->cibsTop], sc->cibsTop);


  return sc->commandsInfoArray[sc->cibsTop];
}

/**********************************************************************************/

void mrvlReleaseCib(MV_COMMAND_INFO *pCommandInfo)
{
  SATA_INFO *sc = pCommandInfo->sataInfo;

  if (pCommandInfo->flags & MRVL_COMMAND_INFO_FLAGE_DMA)
  {
	  /* TODO : */	  
  }
  
  if (pCommandInfo->flags & MRVL_COMMAND_INFO_FLAGE_PRD_TABLE)
  {
	  /* TODO : */
  }

  mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG,"release pCommandInfo %p to %d\n",
	   pCommandInfo, sc->cibsTop);

  sc->commandsInfoArray[sc->cibsTop] = pCommandInfo;
  (sc->cibsTop)++;
}

/**********************************************************************************/

int mrvlSetupPrdTable(SATA_INFO * sc, 
							 MV_COMMAND_INFO *pCommandInfo)
{
	volatile int nsegs, error=0;
    UINT32 count = pCommandInfo->xferLen, segment_len;
    UINT32 segment_addr = pCommandInfo->xferPhysMem;
	MV_SATA_EDMA_PRD_ENTRY *pPRDTable = (MV_SATA_EDMA_PRD_ENTRY *)pCommandInfo->mvPrdVirtMem;

	for (nsegs = 0; nsegs < MRVL_MAX_SG_LIST ; nsegs++) 
	{
        if(count == 0)
            break;
		pPRDTable[nsegs].lowBaseAddr = MV_CPU_TO_LE32(segment_addr);
		pPRDTable[nsegs].highBaseAddr = 0;
        segment_len =  MRVL_MAX_BUFFER_PER_PRD_ENTRY;
        if(count < MRVL_MAX_BUFFER_PER_PRD_ENTRY)
        {
            segment_len = count;
        }
        pPRDTable[nsegs].byteCount = MV_CPU_TO_LE16(0xFFFF & segment_len);
		pPRDTable[nsegs].reserved = 0;
		pPRDTable[nsegs].flags = 0;
		mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG,"add buffer to prd(%x): addr %x len %x, ( %x, %x)\n",
				 (unsigned int)pPRDTable,
				 segment_addr,
				 segment_len,
				 pPRDTable[nsegs].lowBaseAddr, 
				 pPRDTable[nsegs].byteCount);
        segment_addr += segment_len;
        count -= segment_len;
    }
    if(count != 0)
    {
        printf("ERROR in building SATA PRD Table, buffer too large\n");
    }
	pPRDTable[nsegs - 1].flags = MV_CPU_TO_LE16(MV_EDMA_PRD_EOT_FLAG);

	/*mvOsCacheFlush (NULL, pPRDTable, sizeof(MV_SATA_EDMA_PRD_ENTRY) * nsegs);*/

	pCommandInfo->scb.PRDTableLowPhyAddress = pCommandInfo->mvPrdPhysMem;
    pCommandInfo->scb.PRDTableHighPhyAddress = 0;
	
	return error;
}

static MV_BOOLEAN
ATACommandCompletionCB(MV_SATA_ADAPTER *pSataAdapter,
                       MV_U8 channelNum,
                       MV_COMPLETION_TYPE comp_type,
                       MV_VOID_PTR commandId,
                       MV_U16 responseFlags,
                       MV_U32 timeStamp,
                       MV_STORAGE_DEVICE_REGISTERS *registerStruct)
{
    SEM_ID command_sem = (SEM_ID) commandId;
    switch (comp_type)
    {
    case MV_COMPLETION_TYPE_NORMAL:
        /* finish */
        SCSI_DEBUG_MSG("command completed. \n", 0,0,0,0,0,0);
        break;
    case MV_COMPLETION_TYPE_ABORT:
        SCSI_DEBUG_MSG("Error: command Aborted.\n", 0,0,0,0,0,0);
        break;
    case MV_COMPLETION_TYPE_ERROR:
        SCSI_DEBUG_MSG("COMPLETION ERROR \n", 0,0,0,0,0,0);
        SCSI_DEBUG_MSG("ATA Status 0x%x Error 0x%x \n", registerStruct->statusRegister,
                       registerStruct->errorRegister,0,0,0,0);
        /*dumpAtaDeviceRegisters(pSataAdapter, channelNum, MV_FALSE, registerStruct);*/
        break;
    default:
        SCSI_DEBUG_MSG(" Unknown completion type (%d)\n", comp_type, 0,0,0,0,0);
        return MV_FALSE;
    }
    semGive(command_sem);
    return MV_TRUE;
}

/**********************************************************************************/
/*                                                              
 Power management commands
 1. IDLE
 2. IDEL IMMEDIATE
 3. STANDBY
 4. STANDBY IMMEDIATE                                                                
 5. Enable APM
 6. Disable APM
*/
void setPowerState(SCSI_PHYS_DEV * pScsiPhysDev, int command, int timer)
{
    SIOP *pSiop = (SIOP *)pScsiPhysDev->pScsiCtrl;
    SATA_INFO*		pSataInfo = pSiop->pSataInfo;
    MV_QUEUE_COMMAND_INFO   commandInfo;
    MV_QUEUE_COMMAND_INFO   *pCommandInfo = &commandInfo;
    MV_QUEUE_COMMAND_RESULT result;
    MV_U8 ata_command = 0, subcommand = 0;
    SEM_ID command_sem;

    memset(&commandInfo, 0, sizeof(MV_QUEUE_COMMAND_INFO));
    switch(command)
    {
    case 0:
        printf(" error: bad command\n");
        printf("commands:\n");
        printf("(1) IDLE\n");
        printf("(2) IDLE IMMEDIATE\n");
        printf("(3) STANDBY\n");
        printf("(4) STANDBY IMMEDIATE\n");
        printf("(5) APM enable\n");
        printf("(6) APM disable\n");
        return;
    case 1:
        ata_command = MV_ATA_COMMAND_IDLE;
        break;
    case 2:
        ata_command = MV_ATA_COMMAND_IDLE_IMMEDIATE;
        break;
    case 3:
        ata_command = MV_ATA_COMMAND_STANDBY;
        break;
    case 4:
        ata_command = MV_ATA_COMMAND_STANDBY_IMMEDIATE;
        break;
    case 5:
        ata_command = MV_ATA_COMMAND_SET_FEATURES;
        subcommand = MV_ATA_SET_FEATURES_ENABLE_APM;
        break;
    case 6:
        ata_command = MV_ATA_COMMAND_SET_FEATURES;
        subcommand = MV_ATA_SET_FEATURES_DISABLE_APM;
        break;
    default:
        printf("bad command (%d)\n", command);
    }
    command_sem = semBCreate (mvSataSingleStepSemOptions, SEM_EMPTY);

    pCommandInfo->type = MV_QUEUED_COMMAND_TYPE_NONE_UDMA;
    pCommandInfo->PMPort = pScsiPhysDev->scsiDevLUN;
    pCommandInfo->commandParams.NoneUdmaCommand.bufPtr = NULL;
    pCommandInfo->commandParams.NoneUdmaCommand.callBack = ATACommandCompletionCB;
    
    pCommandInfo->commandParams.NoneUdmaCommand.command = ata_command;
    pCommandInfo->commandParams.NoneUdmaCommand.commandId = (MV_VOID_PTR) command_sem;
    pCommandInfo->commandParams.NoneUdmaCommand.count = 0;
    pCommandInfo->commandParams.NoneUdmaCommand.features = subcommand;
    pCommandInfo->commandParams.NoneUdmaCommand.isEXT = MV_FALSE;
    pCommandInfo->commandParams.NoneUdmaCommand.protocolType = MV_NON_UDMA_PROTOCOL_NON_DATA;
    pCommandInfo->commandParams.NoneUdmaCommand.sectorCount = timer;
    pCommandInfo->commandParams.NoneUdmaCommand.device = (MV_U8)(MV_BIT6);

    SCSI_DEBUG_MSG("Sending power management command\n",0,0,0,0,0,0);
    semTake(pSiop->pSataInfo->mutualSem, WAIT_FOREVER);
    result = mvSataQueueCommand(&pSataInfo->mvSataAdapter, 
                                pScsiPhysDev->pScsiTarget->scsiDevBusId,
                                pCommandInfo);
    if (result != MV_QUEUE_COMMAND_RESULT_OK)
   {
           SCSI_DEBUG_MSG("command failed\n",0,0,0,0,0,0);
           semGive(pSiop->pSataInfo->mutualSem);
           semDelete(command_sem);
           return;
    }
    semGive(pSiop->pSataInfo->mutualSem);
    semTake(command_sem, WAIT_FOREVER);
    semDelete(command_sem);
    return;
}

/**********************************************************************************/

void _dumpSataRegs(MV_SATA_ADAPTER *pAdapter, MV_U8 channelIndex);
void dump_sata_regs(SCSI_CTRL *pScsiCtrl, int channel)
{
    SIOP *pSiop = (SIOP *)pScsiCtrl;
    SATA_INFO*		pSataInfo = pSiop->pSataInfo;

	_dumpSataRegs(&pSataInfo->mvSataAdapter, channel);

}

