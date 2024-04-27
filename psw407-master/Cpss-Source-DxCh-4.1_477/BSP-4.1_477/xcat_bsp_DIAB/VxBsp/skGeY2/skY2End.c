/******************************************************************************
 *
 * Name:        skY2End.c
 * Project:     Yukon2 specific functions and implementations
 * Version:     $Revision: 1 $
 * Date:        $Date: 12/04/08 3:10p $
 * Purpose:     The main driver source module
 *
 *****************************************************************************/

/******************************************************************************
 *
 *	LICENSE:
 *	(C)Copyright Marvell,
 *	All Rights Reserved
 *	
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MARVELL
 *	The copyright notice above does not evidence any
 *	actual or intended publication of such source code.
 *	
 *	This Module contains Proprietary Information of Marvell
 *	and should be treated as Confidential.
 *	
 *	The information in this file is provided for the exclusive use of
 *	the licensees of Marvell.
 *	Such users have the right to use, modify, and incorporate this code
 *	into products for purposes authorized by the license agreement
 *	provided they include this notice and the associated copyright notice
 *	with any such product.
 *	The information in this file is provided "AS IS" without warranty.
 *	/LICENSE
 *
 ******************************************************************************/

/******************************************************************************
 *
 * DESCRIPTION:
 *		None.
 *
 * DEPENDENCIES:
 *		None.
 *
 ******************************************************************************/

#include "h/skdrv1st.h"
#include "h/skdrv2nd.h"

/* Forward declarations */
void skY2FreeResources (END_DEVICE *pDrvCtrl);

/******************************************************************************
 *
 * Local Function Prototypes
 *
 *****************************************************************************/

static void InitPacketQueues(SK_AC *pAC,int Port);
static void GiveTxBufferToHw(SK_AC *pAC,SK_IOC IoC,int Port);
static void GiveRxBufferToHw(SK_AC *pAC,SK_IOC IoC,int Port,SK_PACKET *pPacket);
static void FillReceiveTableYukon2(SK_AC *pAC,SK_IOC IoC,int Port);
#ifdef SK_EXTREME
static SK_BOOL HandleReceives(SK_AC *pAC,int Port,SK_U16 Len,SK_U32 FrameStatus,SK_U16 Tcp1,SK_U16 Tcp2,SK_U32 Tist,SK_U16 Vlan, SK_U32 ExtremeCsumResult);
#else
static SK_BOOL HandleReceives(SK_AC *pAC,int Port,SK_U16 Len,SK_U32 FrameStatus,SK_U16 Tcp1,SK_U16 Tcp2,SK_U32 Tist,SK_U16 Vlan);
#endif
static void CheckForSendComplete(SK_AC *pAC,SK_IOC IoC,int Port,SK_PKT_QUEUE *pPQ,SK_LE_TABLE *pLETab,unsigned int Done);
static void UnmapAndFreeTxPktBuffer(SK_AC *pAC,SK_PACKET *pSkPacket,int TxPort);
static SK_BOOL AllocateAndInitLETables(SK_AC *pAC);
static SK_BOOL AllocatePacketBuffersYukon2(SK_AC *pAC);
static SK_BOOL AllocAndMapRxBuffer(SK_AC *pAC,SK_PACKET *pSkPacket,int Port);
static void HandleStatusLEs(SK_AC *pAC);

/******************************************************************************
 *
 * Local Variables
 *
 *****************************************************************************/

#define MAX_NBR_RX_BUFFERS_IN_HW	0x15
static SK_U8 NbrRxBuffersInHW;

#define FLUSH_OPC(le)

/*****************************************************************************
 *
 * 	SkY2RestartStatusUnit - restarts the status unit
 *
 * Description:
 *	Reenables the status unit after any De-Init (e.g. when altering 
 *	the sie of the MTU via 'ifconfig a.b.c.d mtu xxx')
 *
 * Returns:	N/A
 */
void SkY2RestartStatusUnit(
SK_AC  *pAC)  /* pointer to adapter control context */
{
	SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_MSG,
		("==> SkY2RestartStatusUnit\n"));

	/*
	** It might be that the TX timer is not started. Therefore
	** it is initialized here -> to be more investigated!
	*/
	SK_OUT32(pAC->IoBase, STAT_TX_TIMER_INI, HW_MS_TO_TICKS(pAC,10));

	pAC->StatusLETable.Done  = 0;
	pAC->StatusLETable.Put   = 0;
	pAC->StatusLETable.HwPut = 0;

	SkGeY2InitStatBmu(pAC, pAC->IoBase, &pAC->StatusLETable);

	SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_MSG,
		("<== SkY2RestartStatusUnit\n"));
}

/*****************************************************************************
 *
 * 	skY2RlmtSend - sends out a single RLMT notification
 *
 * Description:
 *	This function sends out an RLMT frame
 *
 * Returns:	
 *	> 0 - on succes: the number of bytes in the message
 *	= 0 - on resource shortage: this frame sent or dropped, now
 *	      the ring is full ( -> set tbusy)
 *	< 0 - on failure: other problems ( -> return failure to upper layers)
 */
int skY2RlmtSend (
SK_AC *pAC,       /* pointer to adapter control context           */
int   PortNr,     /* index of port the packet(s) shall be send to */
char  *pMessage)  /* pointer to send-message                      */
{
	SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_MSG,
		("=== skY2RlmtSend\n"));
	return -1;   /* temporarily do not send out RLMT frames */
}

#ifdef INCLUDE_BRIDGING
void    skSetDstEndObj(int unit, END_OBJ* pDstEnd)
{
    END_DEVICE * pDrvCtrl; /* driver structure */

    pDrvCtrl = (END_DEVICE *) endFindByName (DEVICE_NAME, unit);  
    if(pDrvCtrl == NULL)
    {
        printf("%s%d: No such interface\n", DEVICE_NAME, unit);
        return;
    }
    pDrvCtrl->pDstEnd = pDstEnd;
}
#endif /* INCLUDE_BRIDGING */

/*****************************************************************************
 *
 * 	skY2AllocateResources - Allocates all required resources for Yukon2
 *
 * Description:
 *	This function allocates all memory needed for the Yukon2. 
 *	It maps also RX buffers to the LETables and initializes the
 *	status list element table.
 *
 * Returns:	
 *	SK_TRUE, if all resources could be allocated and setup succeeded
 *	SK_FALSE, if an error 
 */
SK_BOOL skY2AllocateResources (END_DEVICE  *pDrvCtrl)
{
	int CurrMac;
	SK_AC	*pAC = pDrvCtrl->pAC;

	SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_INIT,
			   ("==> skY2AllocateResources\n"));

	/*
	** Initialize the packet queue variables first
	*/
	for (CurrMac = 0; CurrMac < pAC->GIni.GIMacsFound; CurrMac++) {
		InitPacketQueues(pAC, CurrMac);
	}

	/* 
	** Get sufficient memory for the LETables
	*/
	if (!AllocateAndInitLETables(pAC)) {
		SK_DBG_MSG(pAC, SK_DBGMOD_DRV, 
			SK_DBGCAT_INIT | SK_DBGCAT_DRV_ERROR,
			("No memory for LETable.\n"));
		return(SK_FALSE);
	}

	/*
	** Allocate and intialize memory for both RX and TX 
	** packet and fragment buffers. On an error, free 
	** previously allocated LETable memory and quit.
	*/
	if (!AllocatePacketBuffersYukon2(pAC)) {
		skY2FreeResources(pDrvCtrl);
		SK_DBG_MSG(pAC, SK_DBGMOD_DRV, 
			SK_DBGCAT_INIT | SK_DBGCAT_DRV_ERROR,
			("No memory for Packetbuffers.\n"));
		return(SK_FALSE);
	}

	/* 
	** Rx and Tx LE tables will be initialized in skGeOpen() 
	**
	** It might be that the TX timer is not started. Therefore
	** it is initialized here -> to be more investigated!
	*/
	SK_OUT32(pAC->IoBase, STAT_TX_TIMER_INI, HW_MS_TO_TICKS(pAC,10));

	pAC->MaxUnusedRxLeWorking = MAX_UNUSED_RX_LE_WORKING;

	SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_INIT,
		("<== skY2AllocateResources\n"));

	return (SK_TRUE);
}

/*****************************************************************************
 *
 * 	skY2FreeResources - Frees previously allocated resources of Yukon2
 *
 * Description:
 *	This function frees all previously allocated memory of the Yukon2. 
 *
 * Returns: N/A
 */
void skY2FreeResources (END_DEVICE *pDrvCtrl)
{
	int Port;
	SK_AC  *pAC = pDrvCtrl ->pAC;

	SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_MSG,
		("==> skY2FreeResources\n"));

	/* Free LETables memory */
	if (pAC->pY2LETMemBase != NULL)
	{
        cacheDmaFree(pAC->pY2LETMemBase);
	}
			
	/* Free Rx/Tx packet queues */
	for (Port = 0; Port < pAC->GIni.GIMacsFound; Port++)
	{
		if (pAC->RxPort[Port].ReceivePacketTable != NULL)
			free(pAC->RxPort[Port].ReceivePacketTable);

		if (pAC->TxPort[Port][0].TransmitPacketTable != NULL)
			free(pAC->TxPort[Port][0].TransmitPacketTable);
	}
	SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_MSG,
		("<== skY2FreeResources\n"));
}

/*****************************************************************************
 *
 * 	skY2AllocateRxBuffers - Allocates the receive buffers for a port
 *
 * Description:
 *	This function allocated all the RX buffers of the Yukon2. 
 *
 * Returns: N/A
 */
void skY2AllocateRxBuffers (
SK_AC    *pAC,   /* pointer to adapter control context */
SK_IOC    IoC,	 /* I/O control context                */
int       Port)	 /* port index of RX                   */
{
	SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_INIT,
		("==> skY2AllocateRxBuffers (Port %c)\n", Port));

	FillReceiveTableYukon2(pAC, IoC, Port);

	SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_INIT,
		("<== skY2AllocateRxBuffers\n"));
}

/*****************************************************************************
 *
 * 	skY2FreeRxBuffers - Free's all allocates RX buffers of
 *
 * Description:
 *	This function frees all RX buffers of the Yukon2 for a single port
 *
 * Returns: N/A
 */
void skY2FreeRxBuffers (
SK_AC    *pAC,   /* pointer to adapter control context */
SK_IOC    IoC,	 /* I/O control context                */
int       Port)	 /* port index of RX                   */
{
	END_DEVICE	*pDrvCtrl = pAC->endDev;
	SK_PACKET   *pSkPacket;

	SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_MSG,
		("==> skY2FreeRxBuffers (Port %c)\n", Port));

	if (pAC->RxPort[Port].ReceivePacketTable   != NULL) {
		POP_FIRST_PKT_FROM_QUEUE(&pAC->RxPort[Port].RxQ_working, pSkPacket);
		while (pSkPacket != NULL) {
			if ((pSkPacket->pFrag) != NULL) {
				
				netClFree (pDrvCtrl->end.pNetPool, 
						   pSkPacket->pMBuf->pClBlk->clNode.pClBuf);
    
				pSkPacket->pMBuf        = NULL;
				pSkPacket->pFrag->pPhys = (SK_U64) 0;
				pSkPacket->pFrag->pVirt = NULL;
			}
			PUSH_PKT_AS_LAST_IN_QUEUE(&pAC->RxPort[Port].RxQ_waiting, pSkPacket);
			POP_FIRST_PKT_FROM_QUEUE(&pAC->RxPort[Port].RxQ_working, pSkPacket);
		}
	}

	SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_MSG,
		("<== skY2FreeRxBuffers\n"));
}

/*****************************************************************************
 *
 * 	skY2FreeTxBuffers - Free's any currently maintained Tx buffer
 *
 * Description:
 *	This function frees the TX buffers of the Yukon2 for a single port
 *	which might be in use by a transmit action
 *
 * Returns: N/A
 */
void skY2FreeTxBuffers (
SK_AC    *pAC,   /* pointer to adapter control context */
SK_IOC    IoC,	 /* I/O control context                */
int       Port)	 /* port index of TX                   */
{
	SK_PACKET	*pSkPacket;
	SK_FRAG     *pSkFrag;

	SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_MSG,
		("==> skY2FreeTxBuffers (Port %c)\n", Port));
 
	if (pAC->TxPort[Port][0].TransmitPacketTable != NULL) {
		POP_FIRST_PKT_FROM_QUEUE(&pAC->TxPort[Port][0].TxAQ_working, pSkPacket);
		while (pSkPacket != NULL) {
			if ((pSkFrag = pSkPacket->pFrag) != NULL) {
				UnmapAndFreeTxPktBuffer(pAC, pSkPacket, Port);
			}
			PUSH_PKT_AS_LAST_IN_QUEUE(&pAC->TxPort[Port][0].TxQ_free, pSkPacket);
			POP_FIRST_PKT_FROM_QUEUE(&pAC->TxPort[Port][0].TxAQ_working, pSkPacket);
		}
	}

	SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_MSG,
		("<== skY2FreeTxBuffers\n"));
}

/*****************************************************************************
 *
 * 	skY2Isr - handle a receive IRQ for all yukon2 cards
 *
 * Description:
 *	This function is called when a receive IRQ is set. (only for yukon2)
 *	HandleReceives does the deferred processing of all outstanding
 *	interrupt operations.
 *
 * Returns:	N/A
 */
void skY2EndInt (END_DEVICE *pDrvCtrl)
{
	SK_AC   *pAC  = pDrvCtrl->pAC;
	SK_U32	IntSrc;
	SK_U32	IntMask;
    

	SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_INT_SRC,
		("==> skY2Isr\n"));

	SK_IN32(pAC->IoBase, B0_ISRC, &IntSrc);
	SK_IN32(pAC->IoBase, B0_IMSK, &IntMask);
	
	IntSrc &= IntMask;
	
	SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_INT_SRC,
		("IntSrc = 0x%x\n", IntSrc));

	if (IntSrc == 0) {
		/*SK_OUT32(pAC->IoBase, B0_Y2_SP_ICR, 2);*/
		SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_INT_SRC,
			("No Interrupt\n ==> skY2Isr\n"));
		return;
	}

	/* Check for Special Interrupts */
	if (IntSrc & ~Y2_IS_STAT_BMU) {
#ifdef SK_CHECK_ISR
		logMsg("ISR!\n", 1, 2, 3, 4, 5, 6);
#endif		
		SkGeSirqIsr(pAC, pAC->IoBase, IntSrc);

		SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_INT_SRC,
			("netJobAdd SkEventDispatcher\n"));
		
		netJobAdd((FUNCPTR)SkEventDispatcher, (int)pAC, (int)pAC->IoBase,0,0,0);
	}
			
    /* tNetTask handles RX/TX interrupts */		
	if (IntSrc & Y2_IS_STAT_BMU) {
		
		/* Disable BMU interrupts. Will re-enabled in tNetTask job	*/
		SK_OUT32(pAC->IoBase, B0_IMSK, (IntMask & ~Y2_IS_STAT_BMU));
	
		SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_INT_SRC,
			("netJobAdd HandleStatusLEs\n"));
#ifdef SK_CHECK_ISR
		logMsg("ISRS!\n", 1, 2, 3, 4, 5, 6);
#endif
		/* Handle status LE in net task job */
		netJobAdd ((FUNCPTR) HandleStatusLEs, (int)pAC, 0, 0, 0, 0);
	}

	/* Speed enhancement for a2 chipsets */
	if (HW_FEATURE(pAC, HWF_WA_DEV_42)) {
		SkGeY2SetPutIndex(pAC, pAC->IoBase, Y2_PREF_Q_ADDR(Q_XA1,0), &pAC->TxPort[0][0].TxALET);
		SkGeY2SetPutIndex(pAC, pAC->IoBase, Y2_PREF_Q_ADDR(Q_R1,0), &pAC->RxPort[0].RxLET);
	} 

	/*
	** Stop and restart TX timer
	*/

	if (HW_FEATURE(pAC, HWF_WA_DEV_43_418)) {
		SK_OUT8(pAC->IoBase, STAT_TX_TIMER_CTRL, TIM_STOP);
		SK_OUT8(pAC->IoBase, STAT_TX_TIMER_CTRL, TIM_START);
	}

	SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_INT_SRC,
		("<== skY2Isr\n"));

	return;
}	/* skY2Isr */

/*************************************************************************
*
* skY2EndSend - driver send routine
*
* This routine takes a M_BLK_ID sends off the data in the M_BLK_ID.
* The buffer must already have the addressing information properly installed
* in it. This is done by a higher layer.  
*
* RETURNS: OK or ERROR.
*
* ERRNO: EINVAL
*/

STATUS skY2EndSend
    (
    END_DEVICE * pDrvCtrl,    /* device ptr */
    M_BLK_ID     pMblk        /* data to send */
    )
    {

    int         	numFrags;          /* num of mBlk of a packet */
	SK_AC           *pAC     = pDrvCtrl->pAC;
	SK_U8            FragIdx = 0;
	SK_PACKET       *pSkPacket;
	SK_FRAG         *PrevFrag;
	SK_FRAG         *CurrFrag;
	SK_PKT_QUEUE    *pWorkQueue;  /* corresponding TX queue */
	SK_PKT_QUEUE    *pWaitQueue; 
	SK_PKT_QUEUE    *pFreeQueue; 
	SK_LE_TABLE     *pLETab;      /* corresponding LETable  */ 
    M_BLK_ID    	 pMblkDup = pMblk; /* temporary pMblk */ 
	unsigned int     Port;
	int              CurrFragCtr;
#ifdef SK_CHECK_LE
	static int Count = 0;

	printf("skY2EndSend CALLED Count: %d!\n", Count);
	Count++;
#endif
    if (pMblk == NULL)
        return ERROR;

	END_TX_SEM_TAKE (&pDrvCtrl->end, WAIT_FOREVER);

    /* return if entering polling mode */
    if (pDrvCtrl->flags & FLAG_POLLING_MODE)
        {
		SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_ERROR,
			("skY2EndSend can't be used in poll mode\n"));

		netMblkClChainFree (pMblk);
		errno = EINVAL;
        
		END_TX_SEM_GIVE (&pDrvCtrl->end);
		return (ERROR);
        }

    /* Get port and return if no free packet is available */
	Port = pDrvCtrl->NetNr;
	
	if (IS_Q_EMPTY(&(pAC->TxPort[Port][TX_PRIO_LOW].TxQ_free))) 
	{
		SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_ERROR,
			("skY2EndSend: Port %d Tx free Queue is empty\n", Port));
		
		if (pDrvCtrl->txStall != TRUE)
			pDrvCtrl->txStall = TRUE;

		END_TX_SEM_GIVE (&pDrvCtrl->end);

		return (END_ERR_BLOCK);     /* No free packet available */
	}
	
    /* Get the number of fragments (mBlks) that this packet consist of */
    numFrags = 0;
    
    while( (pMblkDup != NULL) 			&& 
		   (pMblkDup->mBlkHdr.mLen > 0) && 
		   (pMblkDup->mBlkHdr.mData != NULL) )
    {
        numFrags++;
        pMblkDup = pMblkDup->mBlkHdr.mNext;
    }

	/* Check if we have enough fragments in SK_PACKET to describe this packet */
	if (numFrags > MAX_NUM_FRAGS)
	{
        SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_ERROR,
			("skY2EndSend: mBlks num exceeds max TX desc\n"));
		
		netMblkClChainFree (pMblk);
		errno = EINVAL;
		
		END_TX_SEM_GIVE (&pDrvCtrl->end);
        return (ERROR);
	}
		  	
	pLETab     = &(pAC->TxPort[Port][TX_PRIO_LOW].TxALET);

    /* Check if we have enough LE for this packet */
	if ((numFrags * MAX_FRAG_OVERHEAD) > NUM_FREE_LE_IN_TABLE(pLETab)) 
	{
		SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_ERROR,
			("skY2EndSend: Not enough LE available for send\n"));
        
		if (pDrvCtrl->txStall != TRUE)
			pDrvCtrl->txStall = TRUE;

		END_TX_SEM_GIVE (&pDrvCtrl->end);
		return (END_ERR_BLOCK);     /* No free packet available */
	}

	/*
	** Put any new packet to be sent in the waiting queue and 
	** handle also any possible fragment of that packet.
	*/
	pWorkQueue = &(pAC->TxPort[Port][TX_PRIO_LOW].TxAQ_working);
	pWaitQueue = &(pAC->TxPort[Port][TX_PRIO_LOW].TxAQ_waiting);
	pFreeQueue = &(pAC->TxPort[Port][TX_PRIO_LOW].TxQ_free);

	/*
	** Get first packet from free packet queue
	*/
	POP_FIRST_PKT_FROM_QUEUE(pFreeQueue, pSkPacket);
	if(pSkPacket == NULL) 
	{
		SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_ERROR,
			("skY2EndSend: Could not obtain packet from free queue\n"));
        
		if (pDrvCtrl->txStall != TRUE)
            pDrvCtrl->txStall = TRUE;

		END_TX_SEM_GIVE (&pDrvCtrl->end);

        return (END_ERR_BLOCK);     /* No free packet available */
	}

	pSkPacket->pFrag = &(pSkPacket->FragArray[FragIdx]);

	/*
	** map the cluster to be available for the adapter 
	*/	
	pSkPacket->pMBuf	  	  = pMblk;
	pSkPacket->pFrag->pPhys   = (SK_UPTR) SGI_VIRT_TO_PHYS(pMblk->mBlkHdr.mData);
	pSkPacket->pFrag->FragLen = pMblk->mBlkHdr.mLen;
	pSkPacket->pFrag->pNext   = NULL; /* initial has no next default */
	pSkPacket->NumFrags	  	  = numFrags;

	/* flush buffer for cache coherence */
	END_USR_CACHE_FLUSH(((void *)(pMblk->mBlkHdr.mData)), pMblk->mBlkHdr.mLen);

	PrevFrag = pSkPacket->pFrag;

	/*
	** Each scatter-gather fragment need to be mapped...
	*/
	for (CurrFragCtr = 0; CurrFragCtr < numFrags - 1; CurrFragCtr++)
	{
		FragIdx++;
        pMblk = pMblk->mBlkHdr.mNext;
		CurrFrag = &(pSkPacket->FragArray[FragIdx]);

		/* 
		** map the cluster to be available for the adapter 
		*/
		CurrFrag->pPhys   = (SK_UPTR) SGI_VIRT_TO_PHYS(pMblk->mBlkHdr.mData);
 		CurrFrag->FragLen = pMblk->mBlkHdr.mLen;
 		CurrFrag->pNext   = NULL;

		/* flush buffer for cache coherence */
		END_USR_CACHE_FLUSH(((void *)(pMblk->mBlkHdr.mData)), pMblk->mBlkHdr.mLen);

		/*
		** Add the new fragment to the list of fragments
		*/
		PrevFrag->pNext = CurrFrag;
		PrevFrag = CurrFrag;
	}

	/* 
	** Add packet to waiting packets queue 
	*/
	PUSH_PKT_AS_LAST_IN_QUEUE(pWaitQueue, pSkPacket);
	    
	/* bump statistic counter */
#ifdef INCLUDE_SGI_RFC_1213    
    if (pMblk->mBlkHdr.mData[0] & (UINT8) 0x01)
        pDrvCtrl->end.mib2Tbl.ifOutNUcastPkts += 1;   
    else
        END_ERR_ADD (&pDrvCtrl->end, MIB2_OUT_UCAST, +1);
#else
    pDrvCtrl->end.pMib2Tbl->m2PktCountRtn(pDrvCtrl->end.pMib2Tbl,               
                               M2_PACKET_OUT, pMblk->mBlkHdr.mData, 
                                     pMblk->mBlkPktHdr.len);
#endif

	SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_TX_PROGRESS,
		("skY2EndSend: Give Tx Buffer To Hw...\n"));
	
	GiveTxBufferToHw(pAC, pAC->IoBase, Port);
	
    CACHE_PIPE_FLUSH();

    /* release semaphore now */
    END_TX_SEM_GIVE (&pDrvCtrl->end);

	SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_TX_PROGRESS,
		("skY2EndSend: send done...\n"));

    return (OK);    

}	/* skY2EndSend */

/******************************************************************************
 *
 *	skY2PortStop - stop a port on Yukon2
 *
 * Description:
 *	This function stops a port of the Yukon2 chip. This stop 
 *	stop needs to be performed in a specific order:
 * 
 *	a) Stop the Prefetch unit
 *	b) Stop the Port (MAC, PHY etc.)
 *
 * Returns: N/A
 */
void skY2PortStop(
SK_AC   *pAC,      /* adapter control context                             */
SK_IOC   IoC,      /* I/O control context (address of adapter registers)  */
int      Port,     /* port to stop (MAC_1 + n)                            */
int      Dir,      /* StopDirection (SK_STOP_RX, SK_STOP_TX, SK_STOP_ALL) */
int      RstMode)  /* Reset Mode (SK_SOFT_RST, SK_HARD_RST)               */
{
	SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_MSG,
		("==> SkY2PortStop (Port %c)\n", 'A' + Port));

	/*
	** Stop the HW
	*/
	SkGeStopPort(pAC, IoC, Port, Dir, RstMode);

	/*
	** Move any TX packet from work queues into the free queue again
	** and initialize the TX LETable variables
	*/
	skY2FreeTxBuffers(pAC, pAC->IoBase, Port);
	pAC->TxPort[Port][TX_PRIO_LOW].TxALET.Bmu.RxTx.TcpWp    = 0;
	pAC->TxPort[Port][TX_PRIO_LOW].TxALET.Bmu.RxTx.MssValue = 0;
	pAC->TxPort[Port][TX_PRIO_LOW].TxALET.BufHighAddr       = 0;
	pAC->TxPort[Port][TX_PRIO_LOW].TxALET.Done              = 0;    
	pAC->TxPort[Port][TX_PRIO_LOW].TxALET.Put               = 0;
#ifndef USE_ASF_DASH_FW
	pAC->GIni.GP[Port].PState = SK_PRT_STOP;
#endif
	/*
	** Move any RX packet from work queue into the waiting queue
	** and initialize the RX LETable variables
	*/
	skY2FreeRxBuffers(pAC, pAC->IoBase, Port);
	pAC->RxPort[Port].RxLET.BufHighAddr = 0;

	SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_MSG,
		("<== SkY2PortStop()\n"));
}

/******************************************************************************
 *
 *	skY2PortStart - start a port on Yukon2
 *
 * Description:
 *	This function starts a port of the Yukon2 chip. This start 
 *	action needs to be performed in a specific order:
 * 
 *	a) Initialize the LET indices (PUT/GET to 0)
 *	b) Initialize the LET in HW (enables also prefetch unit)
 *	c) Move all RX buffers from waiting queue to working queue
 *	   which involves also setting up of RX list elements
 *	d) Initialize the FIFO settings of Yukon2 (Watermark etc.)
 *	e) Initialize the Port (MAC, PHY etc.)
 *	f) Initialize the MC addresses
 *
 * Returns:	N/A
 */
void skY2PortStart(
SK_AC   *pAC,   /* adapter control context                            */
SK_IOC   IoC,   /* I/O control context (address of adapter registers) */
int      Port)  /* port to start                                      */
{
	END_DEVICE	*pDrvCtrl = pAC->endDev;

	SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_MSG,
		("==> skY2PortStart (Port %c)\n", 'A' + Port));

	/*
	** Initialize the LET indices
	*/
	pAC->RxPort[Port].RxLET.Done                = 0; 
	pAC->RxPort[Port].RxLET.Put                 = 0;
	pAC->RxPort[Port].RxLET.HwPut               = 0;
	pAC->TxPort[Port][TX_PRIO_LOW].TxALET.Done  = 0;    
	pAC->TxPort[Port][TX_PRIO_LOW].TxALET.Put   = 0;
	pAC->TxPort[Port][TX_PRIO_LOW].TxALET.HwPut = 0;
	if (HW_SYNC_TX_SUPPORTED(pAC)) {
		pAC->TxPort[Port][TX_PRIO_LOW].TxSLET.Done  = 0;    
		pAC->TxPort[Port][TX_PRIO_LOW].TxSLET.Put   = 0;
		pAC->TxPort[Port][TX_PRIO_LOW].TxSLET.HwPut = 0;
	}
	
	if (HW_FEATURE(pAC, HWF_WA_DEV_420)) {
		/*
		** It might be that we have to limit the RX buffers 
		** effectively passed to HW. Initialize the start
		** value in that case...
		*/
		NbrRxBuffersInHW = 0;
	}

	/*
	** Initialize the LET in HW (enables also prefetch unit)
	*/
	SkGeY2InitPrefetchUnit(pAC, IoC,(Port == 0) ? Q_R1 : Q_R2,
			&pAC->RxPort[Port].RxLET);
	SkGeY2InitPrefetchUnit( pAC, IoC,(Port == 0) ? Q_XA1 : Q_XA2, 
			&pAC->TxPort[Port][TX_PRIO_LOW].TxALET);
	if (HW_SYNC_TX_SUPPORTED(pAC)) {
		SkGeY2InitPrefetchUnit( pAC, IoC, (Port == 0) ? Q_XS1 : Q_XS2,
				&pAC->TxPort[Port][TX_PRIO_HIGH].TxSLET);
	}

	/*
	** Using new values for the watermarks and the timer for
	** low latency optimization
	*/

	if (pAC->LowLatency) {
		SK_OUT8(IoC, STAT_FIFO_WM, 1);
		SK_OUT8(IoC, STAT_FIFO_ISR_WM, 1);
		SK_OUT32(IoC, STAT_LEV_TIMER_INI, 50);
		SK_OUT32(IoC, STAT_ISR_TIMER_INI, 10);
	}


	/*
	** Initialize the Port (MAC, PHY etc.)
	*/
	if (SkGeInitPort(pAC, IoC, Port)) {
        printf("%s: SkGeInitPort %d failed.\n",pDrvCtrl->end.devObject.name, Port);
	}

	/* Disable Rx GMAC FIFO Flush Mode */
	SK_OUT8(IoC, MR_ADDR(Port, RX_GMF_CTRL_T), (SK_U8) GMF_RX_F_FL_OFF);

	/* Allocates the receive buffers for a port */
    skY2AllocateRxBuffers (pAC, IoC, Port);

	/*
	** Initialize the MC addresses
	*/
	SkAddrMcUpdate(pAC,IoC, Port);

	SkMacRxTxEnable(pAC, IoC,Port);

	pAC->GIni.GP[Port].PState = SK_PRT_RUN;

	SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_MSG,("<== skY2PortStart()\n"));
}


/******************************************************************************
 *
 * Local Functions
 *
 *****************************************************************************/

/*****************************************************************************
 *
 *	InitPacketQueues - initialize SW settings of packet queues
 *
 * Description:
 *	This function will initialize the packet queues for a port.
 *
 * Returns: N/A
 */
static void InitPacketQueues(
SK_AC  *pAC,   /* pointer to adapter control context */
int     Port)  /* index of port to be initialized    */
{
	SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_INIT,
		("==> InitPacketQueues(Port %c)\n", 'A' + Port));
	
	pAC->RxPort[Port].RxQ_working.pHead = NULL;
	pAC->RxPort[Port].RxQ_working.pTail = NULL;
	
	pAC->RxPort[Port].RxQ_waiting.pHead = NULL;
	pAC->RxPort[Port].RxQ_waiting.pTail = NULL;
	
	pAC->TxPort[Port][TX_PRIO_LOW].TxQ_free.pHead = NULL;
	pAC->TxPort[Port][TX_PRIO_LOW].TxQ_free.pTail = NULL;

	pAC->TxPort[Port][TX_PRIO_LOW].TxAQ_working.pHead = NULL;
	pAC->TxPort[Port][TX_PRIO_LOW].TxAQ_working.pTail = NULL;
	
	pAC->TxPort[Port][TX_PRIO_LOW].TxAQ_waiting.pHead = NULL;
	pAC->TxPort[Port][TX_PRIO_LOW].TxAQ_waiting.pTail = NULL;
		
	SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_INIT,
		("<== InitPacketQueues(Port %c)\n", 'A' + Port));
}	/* InitPacketQueues */

/*****************************************************************************
 *
 *	GiveTxBufferToHw - commits a previously allocated DMA area to HW
 *
 * Description:
 *	This functions gives transmit buffers to HW. If no list elements
 *	are available the buffers will be queued. 
 *
 * Notes:
 *       This function can run only once in a system at one time.
 *
 * Returns: N/A
 */
static void GiveTxBufferToHw(
SK_AC   *pAC,   /* pointer to adapter control context         */
SK_IOC   IoC,   /* I/O control context (address of registers) */
int      Port)  /* port index for which the buffer is used    */
{
	SK_HWLE         *pLE;
	SK_PACKET       *pSkPacket;
	SK_FRAG         *pFrag;
	SK_PKT_QUEUE    *pWorkQueue;   /* corresponding TX queue */
	SK_PKT_QUEUE    *pWaitQueue; 
	SK_LE_TABLE     *pLETab;       /* corresponding LETable  */ 
	SK_BOOL          SetOpcodePacketFlag;
	SK_U32           HighAddress;
	SK_U32           LowAddress;
	SK_U8            OpCode;
	SK_U8            Ctrl;

	SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_TX_PROGRESS,
		("==> GiveTxBufferToHw\n"));

	if (HW_FEATURE(pAC, HWF_WA_DEV_510)) {
		SK_OUT32(pAC->IoBase, TBMU_TEST, TBMU_TEST_BMU_TX_CHK_AUTO_OFF);
	}

	if (IS_Q_EMPTY(&(pAC->TxPort[Port][TX_PRIO_LOW].TxAQ_waiting))) {
		SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_ERROR,
			("GiveTxBufferToHw: Nothing to give to HW!\n"));
		return;
	}

	/*
	** Initialize queue settings
	*/
	pWorkQueue = &(pAC->TxPort[Port][TX_PRIO_LOW].TxAQ_working);
	pWaitQueue = &(pAC->TxPort[Port][TX_PRIO_LOW].TxAQ_waiting);
	pLETab     = &(pAC->TxPort[Port][TX_PRIO_LOW].TxALET);

	POP_FIRST_PKT_FROM_QUEUE(pWaitQueue, pSkPacket);
	while (pSkPacket != NULL) {
		SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_TX_PROGRESS,
			("\tWe have a packet to send %p\n", pSkPacket));

		/* 
		** the first frag of a packet gets opcode OP_PACKET 
		*/
		SetOpcodePacketFlag	= SK_TRUE;
		pFrag			= pSkPacket->pFrag;

		/* 
		** fill list elements with data from fragments 
		*/
		while (pFrag != NULL) {
			SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_TX_PROGRESS,
				("\tGet LE\n"));

			GET_TX_LE(pLE, pLETab);
			Ctrl = 0;

			SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_TX_PROGRESS,
				("\tGot empty LE %p idx %d\n", pLE, GET_PUT_IDX(pLETab)));

			SK_DBG_DUMP_TX_LE(pLE);

			LowAddress  = (SK_U32) (pFrag->pPhys & 0xffffffff);
			HighAddress = (SK_U32) (pFrag->pPhys >> 32);

			if (HighAddress != pLETab->BufHighAddr) {
				/* set opcode high part of the address in one LE */
				OpCode = OP_ADDR64 | HW_OWNER;
	
				/* Set now the 32 high bits of the address */
				TXLE_SET_ADDR( pLE, HighAddress);
	
				/* Set the opcode into the LE */
				TXLE_SET_OPC(pLE, OpCode);
	
				/* Flush the LE to memory */
				FLUSH_OPC(pLE);
	
				/* remember the HighAddress we gave to the Hardware */
				pLETab->BufHighAddr = HighAddress;
				
				/* get a new LE because we filled one with high address */
				GET_TX_LE(pLE, pLETab);
			}

			TXLE_SET_ADDR(pLE, LowAddress);
			TXLE_SET_LEN(pLE, pFrag->FragLen);
	
			if (SetOpcodePacketFlag){

				OpCode = OP_PACKET | HW_OWNER;
				SetOpcodePacketFlag = SK_FALSE;
			} else {
				/* Follow packet in a sequence has always OP_BUFFER */
				OpCode = OP_BUFFER | HW_OWNER;
			}

			pFrag = pFrag->pNext;
			if (pFrag == NULL) {
				/* mark last fragment */
				Ctrl |= EOP;
			}
			TXLE_SET_CTRL(pLE, Ctrl);
			TXLE_SET_OPC(pLE, OpCode);
			FLUSH_OPC(pLE);

			SK_DBG_DUMP_TX_LE(pLE);
		}

		/* 
		** Remember next LE for tx complete 
		*/
		pSkPacket->NextLE = GET_PUT_IDX(pLETab);
		SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_TX_PROGRESS,
			("\tNext LE for pkt %p is %d\n", pSkPacket, pSkPacket->NextLE));

		/* 
		** Add packet to working packets queue 
		*/
		PUSH_PKT_AS_LAST_IN_QUEUE(pWorkQueue, pSkPacket);

		/* 
		** give transmit start command
		*/
		if (HW_FEATURE(pAC, HWF_WA_DEV_42)) {

			SkGeY2SetPutIndex(pAC, pAC->IoBase, Y2_PREF_Q_ADDR(Q_XA1,0), 
													&pAC->TxPort[0][0].TxALET);

		} else {
			/* write put index */
			if (Port == 0) { 
				SK_OUT32(pAC->IoBase, 
					Y2_PREF_Q_ADDR(Q_XA1,PREF_UNIT_PUT_IDX_REG), 
					GET_PUT_IDX(&pAC->TxPort[0][0].TxALET)); 
				UPDATE_HWPUT_IDX(&pAC->TxPort[0][0].TxALET);
			} else {
				SK_OUT32(pAC->IoBase, 
					Y2_PREF_Q_ADDR(Q_XA2, PREF_UNIT_PUT_IDX_REG), 
					GET_PUT_IDX(&pAC->TxPort[1][0].TxALET)); 
				UPDATE_HWPUT_IDX(&pAC->TxPort[1][0].TxALET);
			}
		}
	
		if (IS_Q_EMPTY(&(pAC->TxPort[Port][TX_PRIO_LOW].TxAQ_waiting))) {
			break; /* get out of while */
		}
		POP_FIRST_PKT_FROM_QUEUE(pWaitQueue, pSkPacket);
	} /* while (pSkPacket != NULL) */

	SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_TX_PROGRESS,
		("<== GiveTxBufferToHw\n"));
	return;
}	/* GiveTxBufferToHw */

/***********************************************************************
 *
 *	GiveRxBufferToHw - commits a previously allocated DMA area to HW
 *
 * Description:
 *	This functions gives receive buffers to HW. If no list elements
 *	are available the buffers will be queued. 
 *
 * Notes:
 *       This function can run only once in a system at one time.
 *
 * Returns: N/A
 */
static void GiveRxBufferToHw(
SK_AC      *pAC,      /* pointer to adapter control context         */
SK_IOC      IoC,      /* I/O control context (address of registers) */
int         Port,     /* port index for which the buffer is used    */
SK_PACKET  *pPacket)  /* receive buffer(s)                          */
{
	SK_HWLE         *pLE;
	SK_LE_TABLE     *pLETab;
	SK_BOOL         Done = SK_FALSE;  /* at least on LE changed? */
	SK_U32          LowAddress;
	SK_U32          HighAddress;
	SK_U32          PrefetchReg;      /* register for Put index  */
	unsigned        NumFree;
	unsigned        Required;

	SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_RX_PROGRESS,
	("==> GiveRxBufferToHw(Port %c, Packet %p)\n", 'A' + Port, pPacket));

	pLETab	= &pAC->RxPort[Port].RxLET;

	if (Port == 0) {
		PrefetchReg = Y2_PREF_Q_ADDR(Q_R1, PREF_UNIT_PUT_IDX_REG);
	} else {
		PrefetchReg = Y2_PREF_Q_ADDR(Q_R2, PREF_UNIT_PUT_IDX_REG);
	} 

	if (pPacket != NULL) {
		/*
		** For the time being, we have only one packet passed
		** to this function which might be changed in future!
		*/
		PUSH_PKT_AS_LAST_IN_QUEUE(&pAC->RxPort[Port].RxQ_waiting, pPacket);
	}

	/* 
	** now pPacket contains the very first waiting packet
	*/
	POP_FIRST_PKT_FROM_QUEUE(&pAC->RxPort[Port].RxQ_waiting, pPacket);
	while (pPacket != NULL) {
		if (HW_FEATURE(pAC, HWF_WA_DEV_420)) {
			if (NbrRxBuffersInHW >= MAX_NBR_RX_BUFFERS_IN_HW) {
				PUSH_PKT_AS_FIRST_IN_QUEUE(&pAC->RxPort[Port].RxQ_waiting, pPacket);
				SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_RX_PROGRESS,
					("<== GiveRxBufferToHw()\n"));
				return;
			} 
			NbrRxBuffersInHW++;
		}

		SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_RX_PROGRESS,
			("Try to add packet %p\n", pPacket));

		/* 
		** Check whether we have enough listelements:
		**
		** we have to take into account that each fragment 
		** may need an additional list element for the high 
		** part of the address here I simplified it by 
		** using MAX_FRAG_OVERHEAD maybe it's worth to split 
		** this constant for Rx and Tx or to calculate the
		** real number of needed LE's
		*/
		SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_RX_PROGRESS,
			("\tNum %d Put %d Done %d Free %d %d\n",
			pLETab->Num, pLETab->Put, pLETab->Done,
			NUM_FREE_LE_IN_TABLE(pLETab),
			(NUM_FREE_LE_IN_TABLE(pLETab))));

		Required = pPacket->NumFrags + MAX_FRAG_OVERHEAD;
		NumFree = NUM_FREE_LE_IN_TABLE(pLETab);
		if (NumFree) {
			NumFree--;
		}

		if (Required > NumFree ) {
			SK_DBG_MSG(pAC, SK_DBGMOD_DRV, 
				SK_DBGCAT_DRV_RX_PROGRESS | SK_DBGCAT_DRV_ERROR,
				("\tOut of LEs have %d need %d\n",
				NumFree, Required));

			SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_RX_PROGRESS,
				("\tWaitQueue starts with packet %p\n", pPacket));
			PUSH_PKT_AS_FIRST_IN_QUEUE(&pAC->RxPort[Port].RxQ_waiting, pPacket);
			if (Done) {
				/*
				** write Put index to BMU or Polling Unit and make the LE's
				** available for the hardware
				*/
				SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_RX_PROGRESS,
					("\tWrite new Put Idx %d\n",GET_PUT_IDX(pLETab)));

				SK_OUT32(IoC, PrefetchReg, GET_PUT_IDX(pLETab));
				UPDATE_HWPUT_IDX(pLETab);
			}
			SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_RX_PROGRESS,
				("<== GiveRxBufferToHw()\n"));
			return;
		} else {
			if (!AllocAndMapRxBuffer(pAC, pPacket, Port)) {
				/*
				** Failure while allocating Mblk tuple might
				** be due to temporary short of resources
				** Maybe next time buffers are available.
				** Until this, the packet remains in the 
				** RX waiting queue...
				*/
				SK_DBG_MSG(pAC, SK_DBGMOD_DRV, 
					SK_DBGCAT_DRV_RX_PROGRESS | SK_DBGCAT_DRV_ERROR,
					("Failed to allocate Rx buffer\n"));

				SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_RX_PROGRESS | SK_DBGCAT_DRV_ERROR,
					("WaitQueue starts with packet %p\n", pPacket));
				PUSH_PKT_AS_FIRST_IN_QUEUE(&pAC->RxPort[Port].RxQ_waiting, pPacket);
				if (Done) {
					/*
					** write Put index to BMU or Polling 
					** Unit and make the LE's
					** available for the hardware
					*/
					SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_RX_PROGRESS | SK_DBGCAT_DRV_ERROR,
						("\tWrite new Put Idx %d\n", GET_PUT_IDX(pLETab)));
	
					SK_OUT32(IoC, PrefetchReg, GET_PUT_IDX(pLETab));
					UPDATE_HWPUT_IDX(pLETab);
				}
				SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_RX_PROGRESS,
					("<== GiveRxBufferToHw()\n"));
				return;
			}
		}
		
		Done = SK_TRUE;

		LowAddress = (SK_U32) (pPacket->pFrag->pPhys & 0xffffffff);
		HighAddress = (SK_U32) (pPacket->pFrag->pPhys >> 32);
		if (HighAddress != pLETab->BufHighAddr) {
			/* get a new LE for high address */
			GET_RX_LE(pLE, pLETab);

			/* Set now the 32 high bits of the address */
			RXLE_SET_ADDR(pLE, HighAddress);

			/* Set the control bits of the address */
			RXLE_SET_CTRL(pLE, 0);

			/* Set the opcode into the LE */
			RXLE_SET_OPC(pLE, (OP_ADDR64 | HW_OWNER));

			/* Flush the LE to memory */
			FLUSH_OPC(pLE);

			/* remember the HighAddress we gave to the Hardware */
			pLETab->BufHighAddr = HighAddress;
		}

		/*
		** Fill data into listelement
		*/
		GET_RX_LE(pLE, pLETab);
		RXLE_SET_ADDR(pLE, LowAddress);
		RXLE_SET_LEN(pLE, pPacket->pFrag->FragLen);
		RXLE_SET_CTRL(pLE, 0);
		RXLE_SET_OPC(pLE, (OP_PACKET | HW_OWNER));
		FLUSH_OPC(pLE);

		SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_RX_PROGRESS,
			("=== LE filled\n"));

		SK_DBG_DUMP_RX_LE(pLE);

		/* 
		** Remember next LE for rx complete 
		*/
		pPacket->NextLE = GET_PUT_IDX(pLETab);

		SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_RX_PROGRESS,
			("\tPackets Next LE is %d\n", pPacket->NextLE));

		/* 
		** Add packet to working receive buffer queue and get
		** any next packet out of the waiting queue
		*/
		PUSH_PKT_AS_LAST_IN_QUEUE(&pAC->RxPort[Port].RxQ_working, pPacket);
		if (IS_Q_EMPTY(&(pAC->RxPort[Port].RxQ_waiting))) {
			break; /* get out of while processing */
		}
		POP_FIRST_PKT_FROM_QUEUE(&pAC->RxPort[Port].RxQ_waiting, pPacket);
	}

	SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_RX_PROGRESS,
		("\tWaitQueue is empty\n"));

	if (Done) {
		/*
		** write Put index to BMU or Polling Unit and make the LE's
		** available for the hardware
		*/
		SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_RX_PROGRESS,
			("\tWrite new Put Idx %d\n",GET_PUT_IDX(pLETab)));

		/* Speed enhancement for a2 chipsets */
		if (HW_FEATURE(pAC, HWF_WA_DEV_42)) {
			SkGeY2SetPutIndex(pAC, pAC->IoBase, Y2_PREF_Q_ADDR(Q_R1,0), pLETab);
		} else {
			/* write put index */
			if (Port == 0) { 
				SK_OUT32(IoC, 
					Y2_PREF_Q_ADDR(Q_R1, PREF_UNIT_PUT_IDX_REG), 
					GET_PUT_IDX(pLETab)); 
			} else {
				SK_OUT32(IoC, 
					Y2_PREF_Q_ADDR(Q_R2, PREF_UNIT_PUT_IDX_REG), 
					GET_PUT_IDX(pLETab)); 
			}

			/* Update put index */
			UPDATE_HWPUT_IDX(pLETab);
		}
	}

	SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_RX_PROGRESS,
		("<== GiveRxBufferToHw()\n"));
}       /* GiveRxBufferToHw */

/***********************************************************************
 *
 *	FillReceiveTableYukon2 - map any waiting RX buffers to HW
 *
 * Description:
 *	If the list element table contains more empty elements than 
 *	specified this function tries to refill them.
 *
 * Notes:
 *       This function can run only once per port in a system at one time.
 *
 * Returns: N/A
 */
static void FillReceiveTableYukon2(
SK_AC   *pAC,   /* pointer to adapter control context */
SK_IOC   IoC,   /* I/O control context                */
int      Port)  /* port index of RX                   */
{
	SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_RX_PROGRESS,
		("==> FillReceiveTableYukon2 (Port %c)\n", 'A' + Port));

	if (NUM_FREE_LE_IN_TABLE(&pAC->RxPort[Port].RxLET) >
		pAC->MaxUnusedRxLeWorking) {

		/* 
		** Give alle waiting receive buffers down 
		** The queue holds all RX packets that
		** need a fresh allocation of the sk_buff.
		*/
		if (pAC->RxPort[Port].RxQ_waiting.pHead != NULL) {
			SK_DBG_MSG(NULL, SK_DBGMOD_DRV, SK_DBGCAT_DRV_RX_PROGRESS,
			("Rx Waiting queue is not empty -> give it to HW\n"));
			GiveRxBufferToHw(pAC, IoC, Port, NULL);
		}
		else
		{
			SK_DBG_MSG(NULL, SK_DBGMOD_DRV, 
				SK_DBGCAT_DRV_RX_PROGRESS | SK_DBGCAT_DRV_ERROR,
				("Rx Waiting queue is empty. Nothing to fill\n"));
		}
	}

	SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_RX_PROGRESS,
		("<== FillReceiveTableYukon2 ()\n"));
}	/* FillReceiveTableYukon2 */

/******************************************************************************
 *
 *
 *	HandleReceives - will pass any ready RX packet to kernel
 *
 * Description:
 *	This functions handles a received packet. It checks wether it is
 *	valid, updates the receive list element table and gives the receive
 *	buffer to Linux
 *
 * Notes:
 *	This function can run only once per port at one time in the system.
 *
 * Returns: N/A
 */
static SK_BOOL HandleReceives(
SK_AC  *pAC,          /* adapter control context                     */
int     Port,         /* port on which a packet has been received    */
SK_U16  Len,          /* number of bytes which was actually received */
SK_U32  FrameStatus,  /* MAC frame status word                       */
SK_U16  Tcp1,         /* first hw checksum                           */
SK_U16  Tcp2,         /* second hw checksum                          */
SK_U32  Tist,         /* timestamp                                   */
SK_U16  Vlan          /* Vlan Id                                     */
#ifdef SK_EXTREME
, SK_U32  ExtremeCsumResult
#endif
)
{
	END_DEVICE		*pDrvCtrl = pAC->endDev;
	SK_PACKET       *pSkPacket;
	SK_LE_TABLE     *pLETab;
	SK_MBUF         *pRlmtMbuf;  /* buffer for giving RLMT frame */
	M_BLK_ID     	pMblk;       /* ptr to message holding frame */

	SK_BOOL         SlowPathLock = SK_TRUE;
	SK_BOOL         IsGoodPkt;
	SK_BOOL         IsBc;
	SK_BOOL         IsMc;
	SK_EVPARA       EvPara;      /* an event parameter union     */
	SK_I16          LenToFree;   /* must be signed integer       */

	unsigned int    RlmtNotifier;
	int             FrameLength; /* total length of recvd frame  */
	int             NumBytes; 
	int             Offset = 0;
#if 1
	SK_U16          HighVal;
	SK_BOOL         CheckBcMc = SK_FALSE;
#endif
#ifdef Y2_SYNC_CHECK
	SK_U16		MyTcp;
#endif

	SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_RX_PROGRESS,
		("==> HandleReceives (Port %c)\n", 'A' + Port));
#if 1
	if (HW_FEATURE(pAC, HWF_WA_DEV_521)) {
		HighVal = (SK_U16)(((FrameStatus) & GMR_FS_LEN_MSK) >> GMR_FS_LEN_SHIFT);

		if (FrameStatus == 0x7ffc0001) {
			FrameStatus = 0;
		}
		else if (HighVal != Len) {
				SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_RX_PROGRESS,
					("521: We take pkt len %x stat: %x\n", Len, FrameStatus));

				/*
				** Try to recover the status word. The multicast and broadcast bits
				** from status word are set when we have a valid packet header.
				*/
				FrameStatus = 0;

				/* Set the ReceiveOK bit. */
				FrameStatus |= GMR_FS_RX_OK;

				/* Set now the length into the status word */
				FrameStatus |= (Len << GMR_FS_LEN_SHIFT);

				/* Insure that bit 31 is reset */
				FrameStatus &= ~GMR_FS_LKUP_BIT2;
				CheckBcMc = SK_TRUE;
		}
	}
#endif
	/* 
	** Check whether we want to receive this packet 
	*/
	SK_Y2_RXSTAT_CHECK_PKT(Len, FrameStatus, IsGoodPkt);

	/*
	** Remember length to free (in case of RxBuffer overruns;
	** unlikely, but might happen once in a while)
	*/
	LenToFree = (SK_I16) Len;

	/* 
	** maybe we put these two checks into the SK_RXDESC_CHECK_PKT macro too 
	*/
	if (Len > pDrvCtrl->rxBufSize) {
		IsGoodPkt = SK_FALSE;
	}

	/*
	** Take first receive buffer out of working queue 
	*/
	POP_FIRST_PKT_FROM_QUEUE(&pAC->RxPort[Port].RxQ_working, pSkPacket);

	if (pSkPacket == NULL) {
 		SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_ERROR,
			("Packet not available. NULL pointer.\n"));
		return(SK_TRUE);
	}

	if (HW_FEATURE(pAC, HWF_WA_DEV_420)) {
		NbrRxBuffersInHW--;
	}

	/* 
	** Verify the received length of the frame! Note that having 
	** multiple RxBuffers being aware of one single receive packet
	** (one packet spread over multiple RxBuffers) is not supported 
	** by this driver!
	*/
	if ((Len > pDrvCtrl->rxBufSize) || (Len > (SK_U16) pSkPacket->PacketLen)) {
		IsGoodPkt = SK_FALSE;
	}

	/* 
	** Initialize vars for selected port 
	*/
	pLETab = &pAC->RxPort[Port].RxLET;

	/* 
	** Reset own bit in LE's between old and new Done index
	** This is not really necessary but makes debugging easier 
	*/
	CLEAR_LE_OWN_FROM_DONE_TO(pLETab, pSkPacket->NextLE);

	/* 
	** Free the list elements for new Rx buffers 
	*/
	SET_DONE_INDEX(pLETab, pSkPacket->NextLE);
	pMblk = pSkPacket->pMBuf;
	FrameLength = Len;

	SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_RX_PROGRESS,
		("Received frame of length %d on port %d\n",FrameLength, Port));

	if (!IsGoodPkt) {
		
		netMblkClChainFree (pSkPacket->pMBuf);
		PUSH_PKT_AS_LAST_IN_QUEUE(&pAC->RxPort[Port].RxQ_waiting, pSkPacket);
		
		if (!(FrameStatus & GMR_FS_RX_FF_OV))
		{
			SK_DBG_MSG(pAC, SK_DBGMOD_DRV,
		SK_DBGCAT_DRV_RX_PROGRESS | SK_DBGCAT_DRV_ERROR,
		("(Port %c) Bad packet (Status 0x%x)\n",'A'+Port, FrameStatus));
		}

		/* In case of Rx packet spanned over multiple buffers			*/
		LenToFree = LenToFree - (pSkPacket->pFrag->FragLen);
		while (LenToFree > 0) {

			SK_DBG_MSG(pAC, SK_DBGMOD_DRV,
				SK_DBGCAT_DRV_RX_PROGRESS | SK_DBGCAT_DRV_ERROR,
				("Release packet fragments\n"));

			POP_FIRST_PKT_FROM_QUEUE(&pAC->RxPort[Port].RxQ_working, pSkPacket);
			
			if (pSkPacket == NULL) {
				SK_DBG_MSG(pAC, SK_DBGMOD_DRV, 
					SK_DBGCAT_DRV_RX_PROGRESS | SK_DBGCAT_DRV_ERROR,
					("Err while releasing packet pragment. NULL pointer.\n"));
				return(SK_TRUE);
			}
			if (HW_FEATURE(pAC, HWF_WA_DEV_420)) {
				NbrRxBuffersInHW--;
			}
			CLEAR_LE_OWN_FROM_DONE_TO(pLETab, pSkPacket->NextLE);
			SET_DONE_INDEX(pLETab, pSkPacket->NextLE);
			
			netMblkClChainFree (pSkPacket->pMBuf);
			PUSH_PKT_AS_LAST_IN_QUEUE(&pAC->RxPort[Port].RxQ_waiting, pSkPacket);
			LenToFree = LenToFree - ((SK_I16)(pSkPacket->pFrag->FragLen));
			
			SK_DBG_MSG(pAC, SK_DBGMOD_DRV,
				SK_DBGCAT_DRV_RX_PROGRESS | SK_DBGCAT_DRV_ERROR,
				("<== HandleReceives (Port %c)\n", 'A' + Port));
		}
		return(SK_TRUE);
	} else {
		
		if (pDrvCtrl->rxBuffSwShift)
		{
			memmove((void *)(pMblk->mBlkHdr.mData + pDrvCtrl->rxBuffSwShift), 
					(const void *)pMblk->mBlkHdr.mData, 
					(size_t)FrameLength);
		}

		/* Update Mblk with Rx packet info */
        pMblk->mBlkHdr.mData += pDrvCtrl->offset;
		pMblk->mBlkHdr.mLen = FrameLength;
		pMblk->mBlkHdr.mFlags |= M_PKTHDR;
		pMblk->mBlkPktHdr.len = FrameLength;

		SK_DBG_MSG(NULL, SK_DBGMOD_DRV, SK_DBGCAT_DRV_RX_PROGRESS,("V"));
		
		RlmtNotifier = SK_RLMT_RX_PROTOCOL;
#if 1
		if (CheckBcMc == SK_TRUE) {

			/* Check if this is a broadcast packet */
			if (SK_ADDR_EQUAL(pMblk->mBlkHdr.mData, "\xFF\xFF\xFF\xFF\xFF\xFF")) {

				/* Broadcast packet */
				FrameStatus |= GMR_FS_BC;
			}
			else if (pMblk->mBlkHdr.mData[0] & 0x01) {

				/* Multicast packet */
				FrameStatus |= GMR_FS_MC;
			}
		}
#endif
		IsBc = (FrameStatus & GMR_FS_BC) ? SK_TRUE : SK_FALSE;
		SK_RLMT_PRE_LOOKAHEAD(pAC,Port,FrameLength,
					IsBc,&Offset,&NumBytes);
		if (NumBytes != 0) {
			IsMc = (FrameStatus & GMR_FS_MC) ? SK_TRUE : SK_FALSE;
			SK_RLMT_LOOKAHEAD(pAC,Port,&pMblk->mBlkHdr.mData[Offset],
						IsBc,IsMc,&RlmtNotifier);
		}

		if (RlmtNotifier == SK_RLMT_RX_PROTOCOL) {
			SK_DBG_MSG(NULL,SK_DBGMOD_DRV,
				SK_DBGCAT_DRV_RX_PROGRESS,("W"));
			if ((Port == pAC->ActivePort)||(pAC->RlmtNets == 2)) {
				/* send up only frames from active port */
				SK_DBG_MSG(NULL, SK_DBGMOD_DRV,
					SK_DBGCAT_DRV_RX_PROGRESS,("U"));

#ifdef MV_INCLUDE_PNMI
				SK_PNMI_CNT_RX_OCTETS_DELIVERED(pAC,
					FrameLength, Port);
#endif

#ifdef INCLUDE_SGI_RFC_1213
				if ((*(char *) pMblk->mBlkHdr.mData ) & (UINT8) 0x01)
					pDrvCtrl->end.mib2Tbl.ifInNUcastPkts += 1;
				else
					END_ERR_ADD (&pDrvCtrl->end, MIB2_IN_UCAST, +1);
#else
                pDrvCtrl->end.pMib2Tbl->m2PktCountRtn(pDrvCtrl->end.pMib2Tbl,               
                                        M2_PACKET_IN, pMblk->mBlkHdr.mData, 
                                        pMblk->mBlkHdr.mLen);
#endif /* INCLUDE_SGI_RFC_1213 */				

#ifdef INCLUDE_BRIDGING
            if(pDrvCtrl->pDstEnd != NULL)
            {
                pDrvCtrl->pDstEnd->pFuncTable->send(pDrvCtrl->pDstEnd, pMblk);
            }
            else
#endif /* INCLUDE_BRIDGING */
#ifdef DEBUG
				{
					STATUS      vxStatus;
	
					errnoSet(0);
					vxStatus = pDrvCtrl->end.receiveRtn(&pDrvCtrl->end, pMblk, NULL, NULL, NULL, NULL);
					if( (vxStatus != OK) || (errnoGet() != 0) )
					{
						SK_DBG_MSG(NULL, SK_DBGMOD_DRV, 
							SK_DBGCAT_DRV_RX_PROGRESS | SK_DBGCAT_DRV_ERROR, 
							("END_RCV_RTN_CALL failed: Status=0x%x, errno=0x%x\n",
							 vxStatus, errnoGet()));
					}
				}
#else
#ifdef SK_CHECK_LE
				printf("END_RCV_RTN_CALL()!\n");
#endif
				/* call the upper layer's receive routine. */
				END_RCV_RTN_CALL(&pDrvCtrl->end, pMblk);
#endif /* MV_RT_DEBUG */

			} else { /* drop frame */
				SK_DBG_MSG(NULL,SK_DBGMOD_DRV,
					SK_DBGCAT_DRV_RX_PROGRESS,("D"));
				
#ifdef INCLUDE_SGI_RFC_1213
				END_ERR_ADD (&pDrvCtrl->end, MIB2_IN_ERRS, +1);
#else
                pDrvCtrl->end.pMib2Tbl->m2CtrUpdateRtn(pDrvCtrl->end.pMib2Tbl, 
                                                        M2_ctrId_ifInErrors, 1);
#endif /* INCLUDE_SGI_RFC_1213 */
				
				netMblkClChainFree (pSkPacket->pMBuf);
			}
		} else { /* This is an RLMT-packet! */
			SK_DBG_MSG(NULL, SK_DBGMOD_DRV,
				SK_DBGCAT_DRV_RX_PROGRESS | SK_DBGCAT_DRV_ERROR,("R !!!!!!"));
			pRlmtMbuf = SkDrvAllocRlmtMbuf(pAC,
				pAC->IoBase, FrameLength);
			if (pRlmtMbuf != NULL) {
				pRlmtMbuf->pNext = NULL;
				pRlmtMbuf->Length = FrameLength;
				pRlmtMbuf->PortIdx = Port;
				EvPara.pParaPtr = pRlmtMbuf;
				SK_MEMCPY((char*)(pRlmtMbuf->pData),
				          (char*)(pMblk->mBlkHdr.mData),FrameLength);

				if (SlowPathLock == SK_TRUE) {
					/*semTake(pAC->SlowPathLock, WAIT_FOREVER);*/
					SkEventQueue(pAC, SKGE_RLMT,
						SK_RLMT_PACKET_RECEIVED,
						EvPara);
					pAC->CheckQueue = SK_TRUE;
					/*semGive(pAC->SlowPathLock);*/
				} else {
					SkEventQueue(pAC, SKGE_RLMT,
						SK_RLMT_PACKET_RECEIVED,
						EvPara);
					pAC->CheckQueue = SK_TRUE;
				}

				SK_DBG_MSG(NULL, SK_DBGMOD_DRV,
					SK_DBGCAT_DRV_RX_PROGRESS,("Q"));
			}
			if (pAC->endDev->flags & (IFF_PROMISC | IFF_ALLMULTI)) 
			
				/* RLMT not supported !! */
				netMblkClChainFree (pSkPacket->pMBuf);
		
		} /* if packet for rlmt */
		PUSH_PKT_AS_LAST_IN_QUEUE(&pAC->RxPort[Port].RxQ_waiting, pSkPacket);
	} /* end if-else (IsGoodPkt) */

	SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_RX_PROGRESS,
		("<== HandleReceives (Port %c)\n", 'A' + Port));

	return(SK_TRUE);

}	/* HandleReceives */

/***********************************************************************
 *
 * 	CheckForSendComplete - Frees any freeable Tx bufffer 
 *
 * Description:
 *	This function checks the queues of a port for completed send
 *	packets and returns these packets back to the OS.
 *
 * Notes:
 *	This function can run simultaneously for both ports if
 *	the OS function OSReturnPacket() can handle this,
 *
 *	Such a send complete does not mean, that the packet is really
 *	out on the wire. We just know that the adapter has copied it
 *	into its internal memory and the buffer in the systems memory
 *	is no longer needed.
 *
 * Returns: N/A
 */
static void CheckForSendComplete(
SK_AC         *pAC,     /* pointer to adapter control context  */
SK_IOC         IoC,     /* I/O control context                 */
int            Port,    /* port index                          */
SK_PKT_QUEUE  *pPQ,     /* tx working packet queue to check    */
SK_LE_TABLE   *pLETab,  /* corresponding list element table    */
unsigned int   Done)    /* done index reported for this LET    */
{
	SK_PACKET       *pSkPacket;
	SK_PKT_QUEUE     SendCmplPktQ = { NULL, NULL, NULL };
	SK_BOOL          DoWakeQueue  = SK_FALSE;
	unsigned         Put;
	END_DEVICE		*pDrvCtrl = pAC->endDev;

    END_TX_SEM_TAKE (&pDrvCtrl->end, WAIT_FOREVER);

	SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_TX_PROGRESS,
		("==> CheckForSendComplete(Port %c)\n", 'A' + Port));

	/* 
	** Reset own bit in LE's between old and new Done index
	** This is not really necessairy but makes debugging easier 
	*/
	SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_TX_PROGRESS,
		("Clear Own Bits in TxTable from %d to %d\n",
		pLETab->Done, (Done == 0) ?
		NUM_LE_IN_TABLE(pLETab) :
		(Done - 1)));

	/*semTake((pPQ->QueueLock), WAIT_FOREVER);*/

	CLEAR_LE_OWN_FROM_DONE_TO(pLETab, Done);

	Put = GET_PUT_IDX(pLETab);

	/* 
	** Check whether some packets have been completed 
	*/
	PLAIN_POP_FIRST_PKT_FROM_QUEUE(pPQ, pSkPacket);

	if (pSkPacket == NULL) {
 		SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_ERROR,
			("CheckForSendComplete: Nothing to release!! NULL pointer.\n"));
		
		END_TX_SEM_GIVE (&pDrvCtrl->end);
		return;
	}
	while (pSkPacket != NULL) {
		
		SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_TX_PROGRESS,
			("Check Completion of Tx packet %p\n", pSkPacket));
		SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_TX_PROGRESS,
			("Put %d NewDone %d NextLe of Packet %d\n", Put, Done,
			pSkPacket->NextLE));

		if ((Put > Done) &&
			((pSkPacket->NextLE > Put) || (pSkPacket->NextLE <= Done))) {
			PLAIN_PUSH_PKT_AS_LAST_IN_QUEUE(&SendCmplPktQ, pSkPacket);
			SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_TX_PROGRESS,
				("Packet finished (a)\n"));
		} else if ((Done > Put) &&
			(pSkPacket->NextLE > Put) && (pSkPacket->NextLE <= Done)) {
			PLAIN_PUSH_PKT_AS_LAST_IN_QUEUE(&SendCmplPktQ, pSkPacket);
			SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_TX_PROGRESS,
				("Packet finished (b)\n"));
		} else if ((Done == TXA_MAX_LE-1) && (Put == 0) && (pSkPacket->NextLE == 0)) {
			PLAIN_PUSH_PKT_AS_LAST_IN_QUEUE(&SendCmplPktQ, pSkPacket);
			SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_TX_PROGRESS,
				("Packet finished (b)\n"));
			DoWakeQueue = SK_TRUE;
		} else if (Done == Put) {
			/* all packets have been sent */
			PLAIN_PUSH_PKT_AS_LAST_IN_QUEUE(&SendCmplPktQ, pSkPacket);
			SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_TX_PROGRESS,
				("Packet finished (c)\n"));
		} else {
			SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_TX_PROGRESS,
				("Packet not yet finished\n"));
			PLAIN_PUSH_PKT_AS_FIRST_IN_QUEUE(pPQ, pSkPacket);
			break;
		}
		PLAIN_POP_FIRST_PKT_FROM_QUEUE(pPQ, pSkPacket);
	}
/*	semGive(pPQ->QueueLock);*/

	/* 
	** Set new done index in list element table
	*/
	SET_DONE_INDEX(pLETab, Done);
	 
	/*
	** All TX packets that are send complete should be added to
	** the free queue again for new sents to come
	*/
	pSkPacket = SendCmplPktQ.pHead;
	while (pSkPacket != NULL) {
        netMblkClChainFree (pSkPacket->pMBuf);
		pSkPacket->pMBuf	= NULL;
		pSkPacket = pSkPacket->pNext; /* get next packet */
	}
	
	/*
	** Append the available TX packets back to free queue
	*/
	if (SendCmplPktQ.pHead != NULL) { 
		/*semTake(pAC->TxPort[Port][0].TxQ_free.QueueLock, WAIT_FOREVER);*/
		if (pAC->TxPort[Port][0].TxQ_free.pTail != NULL) {
			pAC->TxPort[Port][0].TxQ_free.pTail->pNext = SendCmplPktQ.pHead;
			pAC->TxPort[Port][0].TxQ_free.pTail        = SendCmplPktQ.pTail;
		} else {
			pAC->TxPort[Port][0].TxQ_free.pHead = SendCmplPktQ.pHead;
			pAC->TxPort[Port][0].TxQ_free.pTail = SendCmplPktQ.pTail; 
		}
		
		if (pDrvCtrl->txStall == TRUE)
		{		
			/* TX LEs available, restart transmit */
			SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_TX_PROGRESS,
				 ("Restart mux\n"));
			(void) netJobAdd ((FUNCPTR) muxTxRestart, (int)&pDrvCtrl->end, 
							  0, 0, 0, 0);
			pDrvCtrl->txStall = FALSE;             
		}
		
		/*semGive(pAC->TxPort[Port][0].TxQ_free.QueueLock);*/
	}

	END_TX_SEM_GIVE (&pDrvCtrl->end);
	SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_TX_PROGRESS,
		("<== CheckForSendComplete()\n"));

	return;
}	/* CheckForSendComplete */

/*****************************************************************************
 *
 *	UnmapAndFreeTxPktBuffer
 *
 * Description:
 *      This function free any allocated space of receive buffers
 *
 * Arguments:
 *      pAC - A pointer to the adapter context struct.
 *
 */
static void UnmapAndFreeTxPktBuffer(
SK_AC       *pAC,       /* pointer to adapter context             */
SK_PACKET   *pSkPacket,	/* pointer to port struct of ring to fill */
int          TxPort)    /* TX port index                          */
{
	/*SK_FRAG	 *pFrag = pSkPacket->pFrag;*/

	SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_TX_PROGRESS,
		("--> UnmapAndFreeTxPktBuffer\n"));
	
	netMblkClChainFree(pSkPacket->pMBuf);
	pSkPacket->pMBuf	= NULL;

	SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_TX_PROGRESS,
		("<-- UnmapAndFreeTxPktBuffer\n"));
}

/*****************************************************************************
 *
 * 	HandleStatusLEs
 *
 * Description:
 *	This function checks for any new status LEs that may have been 
 *	received. Those status LEs may either be Rx or Tx ones.
 *
 * Returns:	N/A
 */
static void HandleStatusLEs(SK_AC *pAC)
{
	int       DoneTxA[SK_MAX_MACS];
	int       DoneTxS[SK_MAX_MACS];
	int       Port;
	SK_BOOL   handledStatLE = SK_FALSE;
	SK_BOOL   NewDone       = SK_FALSE;
	SK_HWLE  *pLE;
	SK_U16    HighVal;
	SK_U32    LowVal;
	SK_U8     OpCode;
#ifdef SK_CHECK_LE
	static int RxCount = 0;
	static int TxCount = 0;
#endif
	SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_BMU_STATUS,
		("==> HandleStatusLEs\n"));

	do {
		SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_BMU_STATUS,
			("Own Bit of done ST-LE[%d]: 0x%x \n",
			pAC->StatusLETable.Done, (SK_U32)OWN_OF_FIRST_LE(&pAC->StatusLETable)));

		while (OWN_OF_FIRST_LE(&pAC->StatusLETable) == HW_OWNER) {
			SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_BMU_STATUS,
				("Working on finished status LE[%d]:\n",
				GET_DONE_INDEX(&pAC->StatusLETable)));

			GET_ST_LE(pLE, &pAC->StatusLETable);

			handledStatLE = SK_TRUE;
			OpCode = STLE_GET_OPC(pLE) & ~HW_OWNER;
			Port = STLE_GET_LINK(pLE);

			switch (OpCode) {
			case OP_RXSTAT:
				/* 
				** This is always the last Status LE belonging
				** to a received packet -> handle it...
				*/
#ifdef SK_CHECK_LE
				printf("OP_RXSTAT Count: %d!\n", RxCount);
				RxCount++;
#endif
				HandleReceives(
					pAC,
#ifdef SK_EXTREME
					CSS_GET_PORT(Port),
#else
 					Port,
#endif
					STLE_GET_LEN(pLE),
					STLE_GET_FRSTATUS(pLE),
					pAC->StatusLETable.Bmu.Stat.TcpSum1,
					pAC->StatusLETable.Bmu.Stat.TcpSum2,
					pAC->StatusLETable.Bmu.Stat.RxTimeStamp,
					pAC->StatusLETable.Bmu.Stat.VlanId
#ifdef SK_EXTREME
					,Port
#endif
					);
				break;
			case OP_RXVLAN:
#ifdef SK_CHECK_LE
				printf("OP_RXVLAN!\n");
#endif
				/* this value will be used for next RXSTAT */
				pAC->StatusLETable.Bmu.Stat.VlanId = STLE_GET_VLAN(pLE);
				break;
			case OP_RXTIMEVLAN:
				/* this value will be used for next RXSTAT */
				pAC->StatusLETable.Bmu.Stat.VlanId = STLE_GET_VLAN(pLE);
				/* fall through */
			case OP_RXTIMESTAMP:
				/* this value will be used for next RXSTAT */
				pAC->StatusLETable.Bmu.Stat.RxTimeStamp = STLE_GET_TIST(pLE);
#ifdef SK_CHECK_LE
				printf("OP_RXTIMESTAMP!\n");
#endif
				break;
			case OP_RXCHKSVLAN:
				/* this value will be used for next RXSTAT */
				pAC->StatusLETable.Bmu.Stat.VlanId = STLE_GET_VLAN(pLE);
				/* fall through */
			case OP_RXCHKS:
				/* this value will be used for next RXSTAT */
				pAC->StatusLETable.Bmu.Stat.TcpSum1 = STLE_GET_TCP1(pLE);
				pAC->StatusLETable.Bmu.Stat.TcpSum2 = STLE_GET_TCP2(pLE);
#ifdef SK_CHECK_LE
				printf("OP_RXCHKS!\n");
#endif
				break;
			case OP_RSS_HASH:
				/* this value will be used for next RXSTAT */
#ifdef SK_CHECK_LE
				printf("OP_RSS_HASH!\n");
#endif
				break;
			case OP_TXINDEXLE:
				/*
				** :;:; TODO
				** it would be possible to check for which queues
				** the index has been changed and call 
				** CheckForSendComplete() only for such queues
				*/
				STLE_GET_DONE_IDX(pLE,LowVal,HighVal);

				/*
				** It would be possible to check whether we really
				** need the values for second port or sync queue, 
				** but I think checking whether we need them is 
				** more expensive than the calculation
				*/
#ifdef SK_CHECK_LE
				printf("OP_TXINDEXLE Count: %d!\n", TxCount);
				TxCount++;
#endif
				DoneTxA[0] = STLE_GET_DONE_IDX_TXA1(LowVal,HighVal);
				DoneTxS[0] = STLE_GET_DONE_IDX_TXS1(LowVal,HighVal);
				DoneTxA[1] = STLE_GET_DONE_IDX_TXA2(LowVal,HighVal);
				DoneTxS[1] = STLE_GET_DONE_IDX_TXS2(LowVal,HighVal);

				SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_BMU_STATUS | SK_DBGCAT_DRV_TX_PROGRESS,
					("DoneTxa1 0x%x DoneTxS1: 0x%x DoneTxa2 0x%x DoneTxS2: 0x%x\n",
					DoneTxA[0], DoneTxS[0], DoneTxA[1], DoneTxS[1]));

				NewDone = SK_TRUE;
				break;
			default:
				/* 
				** Have to handle the illegal Opcode in Status LE 
				*/
#ifdef SK_CHECK_LE
				printf("Unexpected OpCode!\n");
#endif
				SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_BMU_STATUS | SK_DBGCAT_DRV_ERROR,
					("Unexpected OpCode\n"));
				break;
			}
#if 1
			/* 
			** Reset own bit we have to do this in order to detect a overflow 
			*/
			STLE_SET_OPC(pLE, SW_OWNER);
		}
#endif

		/* 
		** Now handle any new transmit complete 
		*/
		if (NewDone) {
			SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_BMU_STATUS | SK_DBGCAT_DRV_TX_PROGRESS,
				("Done Index for Tx BMU has been changed\n"));
			for (Port = 0; Port < pAC->GIni.GIMacsFound; Port++) {
				/* 
				** Do we have a new Done idx ? 
				*/
				if (DoneTxA[Port] != GET_DONE_INDEX(&pAC->TxPort[Port][0].TxALET)) {
					SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_BMU_STATUS | SK_DBGCAT_DRV_TX_PROGRESS,
						("Check TxA%d\n", Port + 1));
					CheckForSendComplete(pAC, pAC->IoBase, Port,
						&(pAC->TxPort[Port][0].TxAQ_working),
						&pAC->TxPort[Port][0].TxALET,
						DoneTxA[Port]);
				} else {
					SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_BMU_STATUS | SK_DBGCAT_DRV_TX_PROGRESS,
						("No changes for TxA%d\n", Port + 1));
				}
			}
		}
		NewDone = SK_FALSE;

		/* 
		** Check whether we have to refill our RX table  
		*/
		if (HW_FEATURE(pAC, HWF_WA_DEV_420)) {
			if (NbrRxBuffersInHW < MAX_NBR_RX_BUFFERS_IN_HW) {
				for (Port = 0; Port < pAC->GIni.GIMacsFound; Port++) {
					SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_BMU_STATUS,
						(" Check for refill of RxBuffers on Port %c\n", 'A' + Port));
					FillReceiveTableYukon2(pAC, pAC->IoBase, Port);
				}
			}
		} else {
			for (Port = 0; Port < pAC->GIni.GIMacsFound; Port++) {
				SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_BMU_STATUS,
					("Check for refill of RxBuffers on Port %c\n", 'A' + Port));
				if (NUM_FREE_LE_IN_TABLE(&pAC->RxPort[Port].RxLET) >= 64) {
					FillReceiveTableYukon2(pAC, pAC->IoBase, Port);
				}
			}
		}
	} while (OWN_OF_FIRST_LE(&pAC->StatusLETable) == HW_OWNER);

	/* 
	** Clear status BMU 
	*/
	SK_OUT32(pAC->IoBase, STAT_CTRL, SC_STAT_CLR_IRQ);
	
	/* 
	** Reenable BMU interrupts
	*/
	SK_OUT32(pAC->IoBase, B0_IMSK, pAC->GIni.GIValIrqMask);
	

	SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_BMU_STATUS,
		("<== HandleStatusLEs\n"));

	return;
}	/* HandleStatusLEs */

/*****************************************************************************
 *
 *	AllocateAndInitLETables - allocate memory for the LETable and init
 *
 * Description:
 *	This function will allocate space for the LETable and will also  
 *	initialize them. The size of the tables must have been specified 
 *	before.
 *
 * Arguments:
 *	pAC - A pointer to the adapter context struct.
 *
 * Returns:
 *	SK_TRUE  - all LETables initialized
 *	SK_FALSE - failed
 */
static SK_BOOL AllocateAndInitLETables(
SK_AC *pAC)  /* pointer to adapter context */
{
	char           *pVirtMemAddr;
	dma_addr_t     pPhysMemAddr = 0;
	SK_U32         CurrMac;
	unsigned       Size;
	unsigned       Aligned;
	unsigned       Alignment;
#ifdef SK_CHECK_ALIGN
	SK_U32	PhysOffset = 0;
	SK_U32	TestSize = 0;
#endif

	SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_INIT,
		("==> AllocateAndInitLETables()\n"));

	/*
	** Determine how much memory we need with respect to alignment
	*/
	Alignment = (SK_U32) LE_TAB_SIZE(NUMBER_OF_ST_LE);
	Size = 0;
	for (CurrMac = 0; CurrMac < pAC->GIni.GIMacsFound; CurrMac++) {
		SK_ALIGN_SIZE(LE_TAB_SIZE(RX_MAX_LE), Alignment, Aligned); /* NUMBER_OF_RX_LE */
		Size += Aligned;
		SK_ALIGN_SIZE(LE_TAB_SIZE(TXA_MAX_LE), Alignment, Aligned);/* NUMBER_OF_TX_LE */
		Size += Aligned;
	}
	SK_ALIGN_SIZE(LE_TAB_SIZE(ST_MAX_LE), Alignment, Aligned); /* NUMBER_OF_ST_LE */
	Size += Aligned;
	Size += Alignment;
	pAC->SizeOfAlignedLETables = Size;

	SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_INIT, 
			("Need %08x bytes in total\n", Size));

#ifdef SK_CHECK_ALIGN
    printf("Need 0x%x bytes in total\n", Size);
#endif

	/*
	** Allocate the memory
	*/
	pAC->pY2LETMemBase = cacheDmaMalloc(Size);
	pVirtMemAddr = pAC->pY2LETMemBase;
	if (pVirtMemAddr == NULL) {
		SK_DBG_MSG(pAC, SK_DBGMOD_DRV, 
			SK_DBGCAT_INIT | SK_DBGCAT_DRV_ERROR,
			("AllocateAndInitLETables: kernel malloc failed!\n"));
		return (SK_FALSE); 
	}

	/* 
	** Initialize the memory
	*/
	SK_MEMSET(pVirtMemAddr, 0, Size);
	ALIGN_ADDR(pVirtMemAddr, Alignment); /* Macro defined in skgew.h */
	
	pPhysMemAddr = (dma_addr_t)SGI_VIRT_TO_PHYS(pVirtMemAddr);

	SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_INIT,
		("Virtual address of LETab is %8p!\n", pVirtMemAddr));
	SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_INIT,
		("Phys address of LETab is %8p!\n", (void *) pPhysMemAddr));

	for (CurrMac = 0; CurrMac < pAC->GIni.GIMacsFound; CurrMac++) {
		SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_INIT,
			("RxLeTable for Port %c", 'A' + CurrMac));

#ifdef SK_CHECK_ALIGN
		TestSize = RX_MAX_LE * 8;
		PhysOffset = (TestSize - (pPhysMemAddr & (TestSize - 1)));
		if (PhysOffset == TestSize) {
			PhysOffset = 0;
		}
		printf("RXLE Virt: 0x%x Phys: 0x%x Size: 0x%x PhysOffset: 0x%x 0x%x\n",
				   pVirtMemAddr, pPhysMemAddr, TestSize, PhysOffset, &pAC->RxPort[CurrMac].RxLET);
#endif

		SkGeY2InitSingleLETable(
			pAC,
			&pAC->RxPort[CurrMac].RxLET,
			RX_MAX_LE, /* NUMBER_OF_RX_LE */
			pVirtMemAddr,
			(SK_U32) (pPhysMemAddr & 0xffffffff),
			(SK_U32) (((SK_U64) pPhysMemAddr) >> 32));

		SK_ALIGN_SIZE(LE_TAB_SIZE(RX_MAX_LE), Alignment, Aligned); /* NUMBER_OF_RX_LE */
		pVirtMemAddr += Aligned;
		pPhysMemAddr += Aligned;

		SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_INIT,
			("TxALeTable for Port %c", 'A' + CurrMac));

#ifdef SK_CHECK_ALIGN
		TestSize = TXA_MAX_LE * 8;
		PhysOffset = (TestSize - (pPhysMemAddr & (TestSize - 1)));
		if (PhysOffset == TestSize) {
			PhysOffset = 0;
		}
		printf("TXLE Virt: 0x%x Phys: 0x%x Size: 0x%x PhysOffset: 0x%x 0x%x\n",
				   pVirtMemAddr, pPhysMemAddr, TestSize, PhysOffset, &pAC->TxPort[CurrMac][0].TxALET);
#endif

		SkGeY2InitSingleLETable(
			pAC,
			&pAC->TxPort[CurrMac][0].TxALET,
			TXA_MAX_LE, /* NUMBER_OF_TX_LE */
			pVirtMemAddr,
			(SK_U32) (pPhysMemAddr & 0xffffffff),
			(SK_U32) (((SK_U64) pPhysMemAddr) >> 32));

		SK_ALIGN_SIZE(LE_TAB_SIZE(TXA_MAX_LE), Alignment, Aligned); /* NUMBER_OF_TX_LE */
		pVirtMemAddr += Aligned;
		pPhysMemAddr += Aligned;
	}

	SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_DRV_MSG,("StLeTable"));

#ifdef SK_CHECK_ALIGN
	TestSize = ST_MAX_LE * 8;
	PhysOffset = (TestSize - (pPhysMemAddr & (TestSize - 1)));
	if (PhysOffset == TestSize) {
		PhysOffset = 0;
	}
	printf("STLE Virt: 0x%x Phys: 0x%x Size: 0x%x PhysOffset: 0x%x 0x%x\n",
			   pVirtMemAddr, pPhysMemAddr, TestSize, PhysOffset, &pAC->StatusLETable);
#endif

	SkGeY2InitSingleLETable(
		pAC,
		&pAC->StatusLETable,
		ST_MAX_LE, /* NUMBER_OF_ST_LE */
		pVirtMemAddr,
		(SK_U32) (pPhysMemAddr & 0xffffffff),
		(SK_U32) (((SK_U64) pPhysMemAddr) >> 32));

	SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_INIT, 
		("<== AllocateAndInitLETables(OK)\n"));
	return(SK_TRUE);
}	/* AllocateAndInitLETables */

/*****************************************************************************
 *
 *	AllocatePacketBuffersYukon2 - allocate packet and fragment buffers
 *
 * Description:
 *      This function will allocate space for the packets and fragments
 *
 * Arguments:
 *      pAC - A pointer to the adapter context struct.
 *
 * Returns:
 *      SK_TRUE  - Memory was allocated correctly
 *      SK_FALSE - An error occured
 */
static SK_BOOL AllocatePacketBuffersYukon2(
SK_AC *pAC)  /* pointer to adapter context */
{
	SK_PACKET       *pRxPacket;
	SK_PACKET       *pTxPacket;
	SK_U32           CurrBuff;
	SK_U32           CurrMac;

	SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_INIT,
		("==> AllocatePacketBuffersYukon2()"));

	for (CurrMac = 0; CurrMac < pAC->GIni.GIMacsFound; CurrMac++) {
		/* 
		** Allocate RX packet space, initialize the packets and
		** add them to the RX waiting queue. Waiting queue means 
		** that packet and fragment are initialized, but no sk_buff
		** has been assigned to it yet.
		*/
		pAC->RxPort[CurrMac].ReceivePacketTable = 
			malloc(RX_MAX_NBR_BUFFERS * sizeof(SK_PACKET));

		if (pAC->RxPort[CurrMac].ReceivePacketTable == NULL) {
			SK_DBG_MSG(NULL, SK_DBGMOD_DRV, SK_DBGCAT_INIT | SK_DBGCAT_DRV_ERROR,
				("AllocatePacketBuffersYukon2: no mem RxPkts (port %i)",CurrMac));
			break;
		} else {
			SK_MEMSET(pAC->RxPort[CurrMac].ReceivePacketTable, 0, 
				(RX_MAX_NBR_BUFFERS * sizeof(SK_PACKET)));

			pRxPacket = pAC->RxPort[CurrMac].ReceivePacketTable;

			for (CurrBuff=0;CurrBuff<RX_MAX_NBR_BUFFERS;CurrBuff++) {
				pRxPacket->pFrag = &(pRxPacket->FragArray[0]);
				pRxPacket->NumFrags = 1;
				PUSH_PKT_AS_LAST_IN_QUEUE(&pAC->RxPort[CurrMac].RxQ_waiting, pRxPacket);
				pRxPacket++;
			}
		}

		/*
		** Allocate TX packet space, initialize the packets and
		** add them to the TX free queue. Free queue means that
		** packet is available and initialized, but no fragment
		** has been assigned to it. (Must be done at TX side)
		*/
		pAC->TxPort[CurrMac][0].TransmitPacketTable = 
			malloc(TX_MAX_NBR_BUFFERS * sizeof(SK_PACKET));

		if (pAC->TxPort[CurrMac][0].TransmitPacketTable == NULL) {
			SK_DBG_MSG(NULL, SK_DBGMOD_DRV, SK_DBGCAT_INIT | SK_DBGCAT_DRV_ERROR,
				("AllocatePacketBuffersYukon2: no mem TxPkts (port %i)",CurrMac));
			free(pAC->RxPort[CurrMac].ReceivePacketTable);
			return(SK_FALSE);
		} else {
			SK_MEMSET(pAC->TxPort[CurrMac][0].TransmitPacketTable, 0, 
				(TX_MAX_NBR_BUFFERS * sizeof(SK_PACKET)));
		
			pTxPacket = pAC->TxPort[CurrMac][0].TransmitPacketTable;

			for (CurrBuff=0;CurrBuff<TX_MAX_NBR_BUFFERS;CurrBuff++) {
				PUSH_PKT_AS_LAST_IN_QUEUE(&pAC->TxPort[CurrMac][0].TxQ_free, pTxPacket);
				pTxPacket++;
			}
		}
	} /* end for (CurrMac = 0; CurrMac < pAC->GIni.GIMacsFound; CurrMac++) */

	SK_DBG_MSG(pAC, SK_DBGMOD_DRV, SK_DBGCAT_INIT,
		("<== AllocatePacketBuffersYukon2 (OK)\n"));
	return(SK_TRUE);

}	/* AllocatePacketBuffersYukon2 */

/*****************************************************************************
 *
 * 	AllocAndMapRxBuffer - fill one buffer into the receive packet/fragment
 *
 * Description:
 *	The function allocates a new receive buffer and assigns it to the
 *	the passed receive packet/fragment
 *
 * Returns:
 *	SK_TRUE - a buffer was allocated and assigned
 *	SK_FALSE - a buffer could not be added
 */
static SK_BOOL AllocAndMapRxBuffer(
SK_AC      *pAC,        /* pointer to the adapter control context */
SK_PACKET  *pSkPacket,  /* pointer to packet that is to fill      */
int         Port)       /* port the packet belongs to             */
{
    M_BLK_ID              pMblk;            /* mBlk to store this packet */
	volatile char *       pNewCluster = NULL; /* new cluster pointer */
    volatile CL_BLK_ID    pClBlk = NULL;    /* cluster block pointer */
	END_DEVICE	*pDrvCtrl = pAC->endDev;

	SK_DBG_MSG(NULL, SK_DBGMOD_DRV, SK_DBGCAT_DRV_RX_PROGRESS,
		("--> AllocAndMapRxBuffer (Port: %i)\n", Port));
	
   /*
    * we implicitly using loaning here, if copying is necessary this
    * step may be skipped, but the data must be copied before being
    * passed up to the protocols.
    */

    pNewCluster = netClusterGet (pDrvCtrl->end.pNetPool, pDrvCtrl->pClPoolId);
    if (pNewCluster == NULL)
        {
		SK_DBG_MSG(NULL, SK_DBGMOD_DRV,
			SK_DBGCAT_DRV_RX_PROGRESS | SK_DBGCAT_DRV_ERROR,
			("%s: Allocation of rx cluster failed !\n", pAC->DeviceStr));
#ifdef MV_INCLUDE_PNMI
		SK_PNMI_CNT_NO_RX_BUF(pAC, pAC->RxPort[Port].PortIndex);
#endif
		return(SK_FALSE);
        }

	SK_DBG_MSG(NULL, SK_DBGMOD_DRV, SK_DBGCAT_DRV_RX_PROGRESS,
		("Allocate cluster 0x%x\n", (SK_U32)pNewCluster));	
    
    /* grab a cluster block to marry to the cluster we received. */

    if ((pClBlk = netClBlkGet (pDrvCtrl->end.pNetPool, M_DONTWAIT)) == NULL)
        {
        netClFree (pDrvCtrl->end.pNetPool, (unsigned char *)pNewCluster);
		
		SK_DBG_MSG(NULL, SK_DBGMOD_DRV,
			SK_DBGCAT_DRV_RX_PROGRESS | SK_DBGCAT_DRV_ERROR,
			("%s: Allocation of Cluster Blocks failed !\n", pAC->DeviceStr));

#ifdef MV_INCLUDE_PNMI
		SK_PNMI_CNT_NO_RX_BUF(pAC, pAC->RxPort[Port].PortIndex);
#endif
		return(SK_FALSE);
        }   

    /*
     * Let's get an M_BLK_ID and marry it to ClBlk-Cluster to create a tuple.
     */
    if ((pMblk = netMblkGet (pDrvCtrl->end.pNetPool, M_DONTWAIT, MT_DATA)) == NULL)
        {
        netClBlkFree (pDrvCtrl->end.pNetPool, pClBlk); 

        netClFree (pDrvCtrl->end.pNetPool, (unsigned char *)pNewCluster);

		SK_DBG_MSG(NULL, SK_DBGMOD_DRV,
			SK_DBGCAT_DRV_RX_PROGRESS | SK_DBGCAT_DRV_ERROR,
			("%s: Allocation of M Blocks failed !\n", pAC->DeviceStr));

#ifdef MV_INCLUDE_PNMI
		SK_PNMI_CNT_NO_RX_BUF(pAC, pAC->RxPort[Port].PortIndex);
#endif
		return(SK_FALSE);
        }


    if (NULL == netClBlkJoin (pClBlk, (char*)pNewCluster, pDrvCtrl->clSz, NULL, 0, 0, 0))
	{
		netMblkFree (pDrvCtrl->end.pNetPool, pMblk); 
    
		netClBlkFree (pDrvCtrl->end.pNetPool, pClBlk); 

        netClFree (pDrvCtrl->end.pNetPool, (unsigned char *)pNewCluster);
		
		SK_DBG_MSG(NULL, SK_DBGMOD_DRV,
			SK_DBGCAT_DRV_RX_PROGRESS | SK_DBGCAT_DRV_ERROR,
			("%s: netClBlkJoin failed !\n", pAC->DeviceStr));
        
#ifdef MV_INCLUDE_PNMI
		SK_PNMI_CNT_NO_RX_BUF(pAC, pAC->RxPort[Port].PortIndex);
#endif
		return(SK_FALSE);
	}

    if (NULL == netMblkClJoin (pMblk, pClBlk))	
	{
		netMblkFree (pDrvCtrl->end.pNetPool, pMblk); 
    
		netClBlkFree (pDrvCtrl->end.pNetPool, pClBlk); 

        netClFree (pDrvCtrl->end.pNetPool, (unsigned char *)pNewCluster);
		
		SK_DBG_MSG(NULL, SK_DBGMOD_DRV,
			SK_DBGCAT_DRV_RX_PROGRESS | SK_DBGCAT_DRV_ERROR,
			("%s: netMblkClJoin failed !\n", pAC->DeviceStr));
        
#ifdef MV_INCLUDE_PNMI
		SK_PNMI_CNT_NO_RX_BUF(pAC, pAC->RxPort[Port].PortIndex);
#endif
		return(SK_FALSE);
	}
	pSkPacket->pFrag->pVirt = pMblk->mBlkHdr.mData + pDrvCtrl->rxBuffHwOffs;
	pSkPacket->pFrag->pPhys = (SK_UPTR)SGI_VIRT_TO_PHYS(pSkPacket->pFrag->pVirt);
	pSkPacket->pFrag->FragLen = pDrvCtrl->rxBufSize;
	pSkPacket->pMBuf        = pMblk;	
	pSkPacket->PacketLen    = pDrvCtrl->rxBufSize;
	

	/* Make sure stack reads DRAM and not cache for the first time */
    END_DRV_CACHE_INVALIDATE (((UINT32)pNewCluster), pDrvCtrl->rxBufSize);   

	SK_DBG_MSG(NULL, SK_DBGMOD_DRV, SK_DBGCAT_DRV_RX_PROGRESS,
		("<-- AllocAndMapRxBuffer\n"));

	return (SK_TRUE);
}	/* AllocAndMapRxBuffer */

/*******************************************************************************
 *
 * End of file
 *
 ******************************************************************************/
