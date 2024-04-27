/******************************************************************************
 *
 * Name:	skGeY2End.c
 * Project:	Gigabit Ethernet Adapters
 * Version:	$Revision: 1 $
 * Date:	$Date: 12/04/08 3:10p $
 * Purpose:	The main driver source module
 *
 ******************************************************************************/

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

/* includes */

#include "vxWorks.h"

#include "h/skdrv1st.h"		/* Driver Specific Definitions */
#include "h/skdrv2nd.h"		/* Adapter Control- and Driver specific Def. */
#include "intLib.h"
#include "lstLib.h"             /* needed to maintain protocol list. */
#include "wdLib.h"
#include "iv.h"
#include "semLib.h"
#include "logLib.h"
#include "sysLib.h"
#include "errno.h"
#include "errnoLib.h"
#include "iosLib.h"
#undef    ETHER_MAP_IP_MULTICAST
#include "etherMultiLib.h"      /* multicast stuff. */

#include "tickLib.h"
#include "sys/times.h"
#include "net/mbuf.h"
#include "net/unixLib.h"
#include "net/protosw.h"
#include "net/systm.h"
#include "net/if_subr.h"
#include "net/route.h"
#include "sys/socket.h"
#include "sys/ioctl.h"
#include "drv/pci/pciConfigLib.h"

#ifdef WR_IPV6
#include "adv_net.h"
#endif /* WR_IPV6 */


/* IMPORT */

IMPORT STATUS sysSkGeBoardInit (int, ADAPTOR_INFO *);
IMPORT    int endMultiLstCnt (END_OBJ* pEnd);
 
/* defines */


/* 
 * a shortcut for getting the hardware address
 * from the MIB II stuff. 
 */
#ifdef INCLUDE_SGI_RFC_1213

#define END_HADDR(pEnd)     \
    ((pEnd)->mib2Tbl.ifPhysAddress.phyAddress)

#define END_HADDR_LEN(pEnd) \
    ((pEnd)->mib2Tbl.ifPhysAddress.addrLength)

#else /* !INCLUDE_SGI_RFC_1213 */

#define END_HADDR(pEnd)     \
     ((pEnd)->pMib2Tbl->m2Data.mibIfTbl.ifPhysAddress.phyAddress)

#define END_HADDR_LEN(pEnd) \
     ((pEnd)->pMib2Tbl->m2Data.mibIfTbl.ifPhysAddress.addrLength)

#endif /* INCLUDE_SGI_RFC_1213 */



/* Yukon II extern */
IMPORT SK_BOOL 	skY2AllocateResources	(END_DEVICE *);
IMPORT void 	skY2FreeResources 		(END_DEVICE *);
IMPORT void 	skY2AllocateRxBuffers	(SK_AC *pAC,SK_IOC IoC,int Port);
IMPORT void 	skY2PortStop			(SK_AC *pAC,SK_IOC IoC,int Port,int Dir,int RstMode);
IMPORT void 	skY2PortStart			(SK_AC *pAC,SK_IOC IoC,int Port);
IMPORT void 	skY2FreeRxBuffers		(SK_AC *pAC,SK_IOC IoC,int Port);
IMPORT void 	skY2FreeTxBuffers		(SK_AC *pAC,SK_IOC IoC,int Port);
IMPORT void 	skY2EndInt 				(END_DEVICE *pDrvCtrl);

IMPORT STATUS   skY2EndSend      		(END_DEVICE* pDrvCtrl, M_BLK_ID pBuf);
IMPORT void 	SkDimEnableModerationIfNeeded(SK_AC *pAC);
IMPORT void 	SkDimStartModerationTimer	 (SK_AC *pAC);
IMPORT void 	SkDimModerate				 (SK_AC *pAC);

/* forward functions */							    
LOCAL STATUS    skGeEndParse            (END_DEVICE *, char *);
LOCAL STATUS 	skGeDeviceInit			(END_DEVICE *);
LOCAL void 		skGeGetConfiguration	(END_DEVICE *);
LOCAL STATUS    skGeEndMemInit          (END_DEVICE *);
LOCAL void      skGeMemAllFree          (END_DEVICE *);
LOCAL SK_BOOL 	skGeAllocateResources	(END_DEVICE *);
LOCAL void 		skGeFreeResources       (END_DEVICE *);
LOCAL void 		skGeDescRingInit		(END_DEVICE *);
LOCAL void 		skGeFillRxRing			(SK_AC *, RX_PORT *);
LOCAL SK_BOOL 	skGeFillRxDescriptor	(SK_AC *, RX_PORT *);
LOCAL void      skGeEndInt              (END_DEVICE *);
LOCAL STATUS 	skGeRecv				(END_DEVICE *, RX_PORT *);
    
LOCAL void 		skGeProductStrGet		(SK_AC *pAC);
LOCAL int 		skGePortLinkIsUp		(SK_AC *pAC, SK_IOC IoC, int Port);

LOCAL void 		skGeClearRxRing			(END_DEVICE *, RX_PORT *);
LOCAL void 		skGeClearTxRing			(END_DEVICE *, TX_PORT *);
LOCAL void      skGeEndConfigure        (END_DEVICE *);

LOCAL STATUS    gei82543EndPollStart        (END_DEVICE* pDrvCtrl);
LOCAL STATUS    gei82543EndPollStop         (END_DEVICE* pDrvCtrl);


/* END specific interfaces. */
/* This is the only externally visible interface. */

END_OBJ*        skGeEndLoad      (char* initString);
LOCAL STATUS    skGeEndUnload	 (END_DEVICE* pDrvCtrl);
LOCAL STATUS    skGeEndStart     (END_DEVICE* pDrvCtrl);
LOCAL STATUS    skGeEndStop		 (END_DEVICE* pDrvCtrl);
LOCAL int       skGeEndIoctl	 (END_DEVICE* pDrvCtrl, int cmd, caddr_t data);
LOCAL STATUS    skGeEndSend      (END_DEVICE* pDrvCtrl, M_BLK_ID pBuf);
LOCAL STATUS    skGeEndMCastAdd  (END_DEVICE* pDrvCtrl, char* pAddress);
LOCAL STATUS    skGeEndMCastDel  (END_DEVICE* pDrvCtrl, char* pAddress);
LOCAL STATUS    skGeEndMCastGet  (END_DEVICE* pDrvCtrl, MULTI_TABLE* pTable);
LOCAL STATUS    skGeEndPollSend  (END_DEVICE* pDrvCtrl, M_BLK_ID pBuf);
LOCAL STATUS    skGeEndPollRcv   (END_DEVICE* pDrvCtrl, M_BLK_ID pBuf);

/*
 * Declare our function table.  This is LOCAL across all driver
 * instances.
 */
LOCAL NET_FUNCS skGeEndFuncTable =
    {
    (FUNCPTR) skGeEndStart,     /* Function to start the device. */
    (FUNCPTR) skGeEndStop,      /* Function to stop the device. */
    (FUNCPTR) skGeEndUnload,    /* Unloading function for the driver. */
    (FUNCPTR) skGeEndIoctl,     /* Ioctl function for the driver. */
    (FUNCPTR) skGeEndSend,      /* Send function for the driver. */
    (FUNCPTR) skGeEndMCastAdd,  /* Multicast add function for the */
                                    /* driver. */
    (FUNCPTR) skGeEndMCastDel,  /* Multicast delete function for */
                                    /* the driver. */
    (FUNCPTR) skGeEndMCastGet,  /* Multicast retrieve function for */
                                    /* the driver. */
    (FUNCPTR) skGeEndPollSend,  /* Polling send function */
    (FUNCPTR) skGeEndPollRcv,   /* Polling receive function */
    endEtherAddressForm,            /* put address info into a NET_BUFFER */
    endEtherPacketDataGet,          /* get pointer to data in NET_BUFFER */
    endEtherPacketAddrGet           /* Get packet addresses. */
    };

/* global variables *********************************************************/


/* local variables **********************************************************/
static uintptr_t TxQueueAddr[SK_MAX_MACS][2] = {{0x680, 0x600},{0x780, 0x700}};
static uintptr_t RxQueueAddr[SK_MAX_MACS] = {0x400, 0x480};
static SK_BOOL DoPrintInterfaceChange = SK_FALSE;

#define OEM_CONFIG_VALUE (	SK_ACT_LED_BLINK | \
				SK_DUP_LED_NORMAL | \
				SK_LED_LINK100_ON)

#define CLEAR_AND_START_RX(Port) 	\
	SK_OUT8(pAC->IoBase, RxQueueAddr[(Port)]+Q_CSR, CSR_START | CSR_IRQ_CL_F)
#define CLEAR_TX_IRQ(Port,Prio) 	\
	SK_OUT8(pAC->IoBase, TxQueueAddr[(Port)][(Prio)]+Q_CSR, CSR_IRQ_CL_F)

/*************************************************************************
*
* skGeEndLoad - initialize the driver and device
*
* This routine initializes the driver and the device to the operational state.
* All of the device specific parameters are passed in the initString.
*
* The string contains the target specific parameters like this:
* "unitnum:shmem_addr:shmem_size:RxDescrPerRing:TxDescrPerRing:usrFlags:offset:mtu"
*
* RETURNS: an END object pointer, NULL if error, or zero 
*/

END_OBJ* skGeEndLoad
    (
    char  *initString        /* String to be parsed by the driver. */
    )
    {
    END_DEVICE  *pDrvCtrl = NULL; /* pointer to device structure */
	SK_AC *pAC;

    DRV_LOG (DRV_DEBUG_LOAD, "Loading skGeEnd Driver...\n", 
                              1, 2, 3, 4, 5, 6);
    /* sanity check */

    if (initString == NULL)
       return NULL;

    if (initString[0] == 0)
       {
       bcopy ((char *) DEVICE_NAME, (void *)initString, DEVICE_NAME_LENGTH);
       return 0;
       }

    /* allocate and clean the device structure */
    pDrvCtrl = (END_DEVICE *) calloc (sizeof (END_DEVICE), 1);

    if (pDrvCtrl == NULL)
        goto errorExit;

	/* allocate and clean the private adapter structure */
	pDrvCtrl->pAC = (SK_AC*) calloc (sizeof (SK_AC), 1);
	
    if (pDrvCtrl->pAC == NULL)
        goto errorExit;

    pDrvCtrl->pAC->endDev = (void*)pDrvCtrl;
	
	/* This creates a loop where the pDrvCtrl->pAC and pAC->pDrvCtrl		  */
	/* This is done to enable the adapter APIs to access VxWorks network stuff*/
	pAC = pDrvCtrl->pAC;

    /* parse the init string, filling in the device structure */
    if (skGeEndParse (pDrvCtrl, initString) == ERROR)
        goto errorExit;

	/* call BSP routine to get PCI information*/
    if (sysSkGeBoardInit (pDrvCtrl->unit, &pDrvCtrl->adaptor) != OK)
        {
        DRV_LOG (DRV_DEBUG_LOAD, "Error in getting board info\n", 
                                  1, 2, 3, 4, 5, 6);
        goto errorExit;
        } 

	/* set up device structure based on user's flags */
    if (pDrvCtrl->usrFlags & SGI_END_JUMBO_FRAME_SUPPORT)
        {
        if (pDrvCtrl->mtu <= 0)
            pDrvCtrl->mtu = SK_JUMBO_MTU;
        
        pDrvCtrl->mtu = (pDrvCtrl->mtu <= ETHERMTU)? ETHERMTU :
                  ((pDrvCtrl->mtu > SK_JUMBO_MTU)? SK_JUMBO_MTU : pDrvCtrl->mtu);
        }
    else  /* normal frame */
        pDrvCtrl->mtu = ETHERMTU;
	
	/* set up the virtual base address for registers */
    pAC->IoBase = (SK_IOC)(pDrvCtrl->adaptor.regBaseLow);

	pAC->CheckQueue = SK_FALSE;
	
	pAC->pciDev.pciBusNo    = pDrvCtrl->adaptor.pciDev.pciBusNo;
	pAC->pciDev.pciDeviceNo = pDrvCtrl->adaptor.pciDev.pciDeviceNo;
	pAC->pciDev.pciFuncNo   = pDrvCtrl->adaptor.pciDev.pciFuncNo;
    
	if (skGeDeviceInit(pDrvCtrl) != OK)
		{
        DRV_LOG (DRV_DEBUG_LOAD, "Error in skGeDeviceInit\n", 1, 2, 3, 4, 5, 6);
        goto errorExit;
        } 
	
	if (CHIP_ID_YUKON_2(pAC)) {
		skGeEndFuncTable.send = (FUNCPTR)skY2EndSend;
	} else {
		skGeEndFuncTable.send =	(FUNCPTR)skGeEndSend;
	}

	/* Note that for dual port NICs first port must have an even unit number */
	pDrvCtrl->NetNr = (pDrvCtrl->unit % ((pAC)->GIni.GIMacsFound));

	skGeProductStrGet(pAC);

    /* perform memory allocation and pool initialization */

    if (skGeEndMemInit (pDrvCtrl) == ERROR)
	{				
		/* free all allocated memory */
        skGeMemAllFree (pDrvCtrl);

		goto errorExit;
	}

    /* turn off system interrupts */

    SYS_INT_DISABLE(pDrvCtrl);
    
	/* set the default value for device */
    pDrvCtrl->attach         = FALSE;
    pDrvCtrl->devStartFlag   = FALSE;
   
    /* initialize the END and MIB2 parts of the structure */
    /*
     * The M2 element must come from m2Lib.h 
     * This setting is for a DIX type ethernet device.
     */

#ifdef INCLUDE_SGI_RFC_1213
    
    if (END_OBJ_INIT (&pDrvCtrl->end, (DEV_OBJ *)pDrvCtrl, DEVICE_NAME,
                      pDrvCtrl->unit, &skGeEndFuncTable, pAC->DeviceStr)
					  == ERROR || 
        END_MIB_INIT (&pDrvCtrl->end, 
					  M2_ifType_ethernet_csmacd,
                      &pDrvCtrl->pAC->Addr.Net[0].CurrentMacAddress.a[0], 
					  ETHER_ADDRESS_SIZE, 
					  pDrvCtrl->mtu, 
					  END_SPEED) == ERROR )
	{				
		/* free all allocated memory */
        skGeMemAllFree (pDrvCtrl);

		goto errorExit;
	}
					  
#else /* !INCLUDE_SGI_RFC_1213 */


    if (END_OBJ_INIT(&pDrvCtrl->end, (DEV_OBJ *)pDrvCtrl, DEVICE_NAME,
                    pDrvCtrl->unit, &skGeEndFuncTable, pAC->DeviceStr) == ERROR)
	{				
		/* free all allocated memory */
        skGeMemAllFree (pDrvCtrl);

		goto errorExit;
	}

    /* New RFC 2233 mib2 interface */

    /* Initialize MIB-II entries (for RFC 2233 ifXTable) */

    pDrvCtrl->end.pMib2Tbl = 
		m2IfAlloc(M2_ifType_ethernet_csmacd,
                 (UINT8*) &pDrvCtrl->pAC->Addr.Net[0].CurrentMacAddress.a[0], 
				  ETHER_ADDRESS_SIZE,
                  pDrvCtrl->mtu, 
				  END_SPEED, 
				  DEVICE_NAME, 
				  pDrvCtrl->unit);

    if (pDrvCtrl->end.pMib2Tbl == NULL)
        {
        DRV_LOG (DRV_DEBUG_LOAD, ("%s%d - MIB-II initializations failed\n"),
                "gei", pDrvCtrl->unit, 3, 4, 5, 6);
            
		/* free all allocated memory */
        skGeMemAllFree (pDrvCtrl);

		goto errorExit;
        }

    /*
     * Set the RFC2233 flag bit in the END object flags field and
     * install the counter update routines.
     */

    m2IfPktCountRtnInstall(pDrvCtrl->end.pMib2Tbl, m2If8023PacketCount);

    /*
     * Make a copy of the data in mib2Tbl struct as well. We do this
     * mainly for backward compatibility issues. There might be some
     * code that might be referencing the END pointer and might
     * possibly do lookups on the mib2Tbl, which will cause all sorts
     * of problems.
     */

    bcopy ((char *)&pDrvCtrl->end.pMib2Tbl->m2Data.mibIfTbl,
                   (char *)&pDrvCtrl->end.mib2Tbl, sizeof (M2_INTERFACETBL));

    END_OBJ_READY (&pDrvCtrl->end, (IFF_UP | IFF_RUNNING | IFF_NOTRAILERS |
                    IFF_BROADCAST | IFF_MULTICAST | END_MIB_2233));
#endif /* INCLUDE_SGI_RFC_1213 */

    pDrvCtrl->attach = TRUE;

    DRV_LOG (DRV_DEBUG_LOAD, ("loading skGeEndLoad...OK\n"),1,2,3,4,5,6);

    return (&pDrvCtrl->end);

errorExit:

    if (pDrvCtrl != NULL)
         cfree ((char *)pDrvCtrl);
    
	if (pDrvCtrl->pAC != NULL)
         cfree ((char *)pDrvCtrl->pAC);

    DRV_LOG (DRV_DEBUG_LOAD, ("Loading skGeEndLoad...Error\n"), 
                               1, 2, 3, 4, 5, 6);
    return NULL;
    }

/*************************************************************************
*
* skGeEndParse - parse the init string
*
* Parse the input string.  Fill in values in the driver control structure.
*
* RETURNS: OK or ERROR for invalid arguments.
*/

LOCAL STATUS skGeEndParse
    (
    END_DEVICE *    pDrvCtrl,       /* device pointer */
    char *          initString      /* information string */
    )
    {
    char *    tok;
    char *    pHolder = NULL;

    DRV_LOG (DRV_DEBUG_LOAD, ("skGeEndParse starts...\n"), 1, 2, 3, 4, 5, 6);

    /* parse the initString */

    tok = strtok_r (initString, ":", &pHolder);
    if (tok == NULL)
    return ERROR;
    pDrvCtrl->unit = atoi (tok);
 
    /* address of shared memory */

    tok = strtok_r (NULL, ":", &pHolder);
    if (tok == NULL)
    return ERROR;
    pDrvCtrl->pDescMemBase = (char *) strtoul (tok, NULL, 16);
 
    /* size of shared memory */

    tok = strtok_r (NULL, ":", &pHolder);
    if (tok == NULL)
    return ERROR;
    pDrvCtrl->memSize = strtoul (tok, NULL, 16);

    /* number of rx descriptors */

    tok = strtok_r (NULL, ":", &pHolder);
    if (tok == NULL)
     return ERROR;
    pDrvCtrl->rxDescNum = strtoul (tok, NULL, 16);

    /* number of tx descriptors */

    tok = strtok_r (NULL, ":", &pHolder);
    if (tok == NULL)
    return ERROR;
    pDrvCtrl->txDescNum = strtoul (tok, NULL, 16);
   
    /* get the usrFlags */

    tok = strtok_r (NULL, ":", &pHolder);
    if (tok == NULL)
    return ERROR;
    pDrvCtrl->usrFlags = strtoul (tok, NULL, 16);

    /* get the offset value */

    tok = strtok_r (NULL, ":", &pHolder);
    if (tok != NULL)
        pDrvCtrl->offset = atoi (tok);

    DRV_LOG (DRV_DEBUG_LOAD,
            ("skGeEndParse: unit=%d pDescMemBase=0x%x memSize=0x%x" \
              ", usrFlags=0x%x\n"),
              pDrvCtrl->unit, 
              (int)pDrvCtrl->pDescMemBase,
              (int) pDrvCtrl->memSize,
              pDrvCtrl->usrFlags, 0, 0
              );

    /* check Jumbo Frame support */

    if (pDrvCtrl->usrFlags & SGI_END_JUMBO_FRAME_SUPPORT)
        {        
        /* get mtu */

        tok = strtok_r (NULL, ":", &pHolder);
        if (tok != NULL)
            pDrvCtrl->mtu = atoi (tok);

        DRV_LOG (DRV_DEBUG_LOAD, ("mtu = %d\n"),pDrvCtrl->mtu, 
                 2, 3, 4, 5, 6);
        }

    DRV_LOG (DRV_DEBUG_LOAD, ("skGeEndParse...OK!\n"), 1, 2, 3, 4, 5, 6);
    return OK;
    }

/*************************************************************************
*
* skGeMemAllFree - free all memory allocated by driver
*
* This routine returns all allocated memory by this driver to OS
*
* RETURN: N/A
*/

LOCAL void skGeMemAllFree 
    (
    END_DEVICE * pDrvCtrl    /* device to be initialized */
    )
    {
    SK_AC	*pAC = pDrvCtrl->pAC;
	SK_U8	Byte;
		
	if (pDrvCtrl == NULL)
        return;

    /* read Chip Identification Number */
	SK_IN8(pAC->IoBase, B2_CHIP_ID, &Byte);
	
	/* free adapter memory */
	if ((Byte >= CHIP_ID_YUKON_XL) && (Byte <= CHIP_ID_YUKON_UL_2)) {
		skY2FreeResources(pDrvCtrl);
	} else {
		skGeFreeResources(pDrvCtrl);
	}
    
	/* free RX buffer memory */
    if (pDrvCtrl->pRxBufMem != NULL && (pDrvCtrl->memAllocFlag == TRUE))
        {
        free (pDrvCtrl->pRxBufMem);
        }

    /* release RX mBlk pool */
    if (pDrvCtrl->pMclkArea != NULL)
        {
        cfree (pDrvCtrl->pMclkArea);
        }      

#ifndef INCLUDE_SGI_RFC_1213
    /* Free MIB-II entries */
    m2IfFree(pDrvCtrl->end.pMib2Tbl);
    pDrvCtrl->end.pMib2Tbl = NULL;

#endif /* INCLUDE_SGI_RFC_1213 */
 
    /* release netPool structure */
    if (pDrvCtrl->end.pNetPool != NULL)
        {
	    if (netPoolDelete(pDrvCtrl->end.pNetPool) != OK)
            {
            printf("netPoolDelete fails\n");
            }
	    free ((char *)pDrvCtrl->end.pNetPool);
        }

    return;
}

/*************************************************************************
*
* skGeEndMemInit - allocate and initialize memory for driver
*
* This routine allocates and initializes memory for descriptors and 
* corresponding buffers, and sets up the receive data pool for receiving 
* packets. 
*
* RETURNS: OK or ERROR.
*/

LOCAL STATUS skGeEndMemInit
    (
    END_DEVICE * pDrvCtrl    /* device to be initialized */
    )
    {
    M_CL_CONFIG geiMclBlkConfig; 	/* Mblk */
    CL_DESC     geiClDesc;      	/* Cluster descriptor */
    UINT32      size;           	/* required memory size */
    UINT32      rxBufAlignedMem;	/* temporary memory base */
    UINT32      bufSz;          	/* real cluster size */
    int         bufNum;         	/* temp variable */
	SK_AC	*pAC = pDrvCtrl->pAC;

    DRV_LOG (DRV_DEBUG_LOAD, ("skGeEndMemInit...\n"), 1, 2, 3, 4, 5, 6);

    /* set the default TX/RX descriptor Number */
		/* 
	** Alloc descriptor/LETable memory for this board (both RxD/TxD)
	*/
	if (CHIP_ID_YUKON_2(pAC)) {
		if (!skY2AllocateResources(pDrvCtrl)) {
			printf("skGeEndMemInit: No memory for Yukon2 settings\n");
			return(ERROR);
		}
	} else {
		if(!skGeAllocateResources(pDrvCtrl)) {
			printf("skGeEndMemInit: No memory for descriptor rings.\n");
			return(ERROR);
		}
		/* For Yukon dependent settings... */
		skGeDescRingInit(pDrvCtrl);		
	}


    /* calculate reasonable receive buffer size */
	/* Add architecture offset (data starts 2 bytes from cluster base) */

    bufSz = pDrvCtrl->offset + pDrvCtrl->mtu + SGI_DEFAULT_ETHERHEADER + 
															ETHER_CRC_LENGTH;

    if (bufSz > C_LEN_ETHERMTU_MAXSIZE_JUMBO + 
		       SGI_DEFAULT_ETHERHEADER + ETHER_CRC_LENGTH)
        return ERROR;
    

	/* RxBufSize holds the Rx packet payload available space */
	pDrvCtrl->rxBufSize = bufSz;
	bufNum = bufSz / BUFF_ALIGN;

	/* Check that buffer size is cache line aligned. This is done to ensure	*/
	/* cache operations will not affect other data then Rx buffer			*/
    if (bufSz != (bufNum * BUFF_ALIGN))  
            bufSz = (bufNum + 1) * BUFF_ALIGN;
	
    /* Add more alignment slack. This alignment is used so each cluster will */
	/* start at a cache line aligned address. 								 */
    bufSz += CL_OVERHEAD +			/* include cluster ID */
			 CL_CACHE_ALIGN_SIZE;	/* Make the following cluster cache 	 */
									/* line size aligned 					 */

    if ((int)(pDrvCtrl->pDescMemBase) == NONE)
        {
        if (!CACHE_DMA_IS_WRITE_COHERENT () || 
            !CACHE_DMA_IS_READ_COHERENT ())
            {
            DRV_LOG (DRV_DEBUG_LOAD, ("skGeEndMemInit: shared descriptor" \
                     "memory not cache coherent\n"), 1, 2, 3, 4, 5, 6);
            return ERROR;
            }
		pDrvCtrl->memAllocFlag = TRUE;
        }
    else
        { 
       /* 
        * Unsupported yet...
        */

        pDrvCtrl->memAllocFlag = FALSE;
		
		DRV_LOG (DRV_DEBUG_LOAD, ("skGeEndMemInit: User memory unsupported\n")
				 , 1, 2, 3, 4, 5, 6);
		return ERROR;
		}
        

    if (pDrvCtrl->memAllocFlag || 
         pDrvCtrl->usrFlags & SGI_END_USER_MEM_FOR_DESC_ONLY)
        {
        size = pDrvCtrl->rxDescNum * bufSz *        		/* for RX buffer */
               (DEFAULT_LOAN_RXBUF_FACTOR + 1) +    /* for RX loan buffer */
               1024;

        pDrvCtrl->pRxBufMem = (char *) malloc(size);
        
		if (pDrvCtrl->pRxBufMem == NULL)
            {
            DRV_LOG (DRV_DEBUG_LOAD, "skGeEndMemInit: RxBuffer memory "\
                                  "unavailable\n",1, 2, 3, 4, 5, 6);
            goto errorExit;
            }

        memset((void *)pDrvCtrl->pRxBufMem, 0, size);

        pDrvCtrl->pCacheFuncs = &cacheUserFuncs;

        }
     else
        {
        pDrvCtrl->pCacheFuncs = &cacheNullFuncs;

		DRV_LOG (DRV_DEBUG_LOAD, ("skGeEndMemInit: User memory unsupported\n")
				 , 1, 2, 3, 4, 5, 6);
		return ERROR;
        }

    rxBufAlignedMem = ((((UINT32)(pDrvCtrl->pRxBufMem) + (BUFF_ALIGN)) & 
										~(BUFF_ALIGN - 1)) - CL_OVERHEAD);
    /* 
     * set up RX mBlk pool
     * This is how we set up and END netPool using netBufLib(1).
     * This code is pretty generic.
     */

    if ((pDrvCtrl->end.pNetPool = malloc (sizeof(NET_POOL))) == NULL)
        goto errorExit;

    bzero ((char *)&geiMclBlkConfig, sizeof(geiMclBlkConfig));
    bzero ((char *)&geiClDesc, sizeof(geiClDesc));

    geiMclBlkConfig.mBlkNum = pDrvCtrl->rxDescNum * 
                                       (DEFAULT_MBLK_NUM_FACTOR + 1);

    geiClDesc.clNum = pDrvCtrl->rxDescNum * 
                                     (DEFAULT_LOAN_RXBUF_FACTOR + 1);
    geiClDesc.clSize = bufSz - CL_OVERHEAD;
    
	/* The clSz holds the Rx buffer + slack memory to align Rx buffer to 	*/
	/* cache line size and some more slack to align the next cluster to 	*/
	/* cache line size.														*/			
    pDrvCtrl->clSz = geiClDesc.clSize;

    geiMclBlkConfig.clBlkNum = geiClDesc.clNum;

    /* calculate the total memory for all the M-Blks and CL-Blks. */

    geiMclBlkConfig.memSize = 
        ((geiMclBlkConfig.mBlkNum * (M_BLK_SZ + sizeof (long))) +
         (geiMclBlkConfig.clBlkNum * CL_BLK_SZ + sizeof(long)));

    if ((geiMclBlkConfig.memArea = (char *) memalign (sizeof(long), 
                                            geiMclBlkConfig.memSize)) == 
                                            NULL)
        {
        DRV_LOG (DRV_DEBUG_LOAD, "skGeEndMemInit: MclBlkConfig memory "\
                                  "unavailable\n",1, 2, 3, 4, 5, 6);
        goto errorExit;
        }

    pDrvCtrl->pMclkArea = geiMclBlkConfig.memArea;

    geiClDesc.memSize = geiClDesc.clNum * (geiClDesc.clSize + CL_OVERHEAD);

    geiClDesc.memArea = (char *)rxBufAlignedMem;
    geiClDesc.memArea = (char *)(((UINT32) geiClDesc.memArea + 0x3) & ~0x3);

    END_DRV_CACHE_INVALIDATE (geiClDesc.memArea, geiClDesc.memSize);

    /* initialize the memory pool. */

    if (netPoolInit(pDrvCtrl->end.pNetPool, &geiMclBlkConfig,
                     &geiClDesc, 1,NULL) == ERROR)
        {
        DRV_LOG (DRV_DEBUG_LOAD, "skGeEndMemInit: Could not init buffering\n", 
				 1, 2, 3, 4, 5, 6);
        goto errorExit;
        }

    if ((pDrvCtrl->pClPoolId = netClPoolIdGet (pDrvCtrl->end.pNetPool,
                               geiClDesc.clSize, FALSE)) == NULL)
        {
        DRV_LOG (DRV_DEBUG_LOAD, "skGeEndMemInit: Could not find specified "\
                                  "pool ID\n", 1, 2, 3, 4, 5, 6);
        goto errorExit;
        }

    CACHE_PIPE_FLUSH ();

    DRV_LOG (DRV_DEBUG_LOAD, "skGeEndMemInit...OK\n", 1, 2, 3, 4, 5, 6);

    return OK;

errorExit:

    DRV_LOG (DRV_DEBUG_LOAD, "skGeEndMemInit...ERROR\n", 1, 2, 3,4,5,6);
  
    return ERROR;
}

/*************************************************************************
*
* gei82543EndStart - setup and start the device 
*
* This routine connects interrupts, initializes receiver and transmitter, 
* and enable the device for running 
*
* RETURNS: OK or ERROR
*/

LOCAL STATUS skGeEndStart
    (
    END_DEVICE * pDrvCtrl    /* device ID */
    )
    {
	STATUS result;           	/* results of start device */
	int             CurrMac;  	/* loop ctr for ports    */
	int				startTicks;	/* Clock tick base for link timeout */
	SK_EVPARA       EvPara;   	/* event parameter union */
	VOIDFUNCPTR		isrRtn 	= skGeEndInt;
    SK_AC          *pAC 	= pDrvCtrl->pAC;
#ifdef MV_INCLUDE_ASF_SUPPORT
	SK_U32          TmpVal32;
#endif
    
	DRV_LOG (DRV_DEBUG_LOAD, "skGeEndStart...\n", 1, 2, 3, 4, 5, 6);

    if (pDrvCtrl->attach != TRUE)
       return ERROR;
    
	/* Set blink mode */
	pAC->GIni.GILedBlinkCtrl = OEM_CONFIG_VALUE;
	
	if (pAC->BoardLevel == SK_INIT_DATA) {
		/* level 1 init common modules here */
#ifdef SK_CHECK_INIT
		printf("skGeEndStart: SkGeInit called at level %d!\n", SK_INIT_IO);
#endif
		if (SkGeInit(pAC, pAC->IoBase, SK_INIT_IO) != 0) {
			printf("%s: HWInit (1) failed.\n", pDrvCtrl->end.devObject.name);
			return (-1);
		}
		SkI2cInit	(pAC, pAC->IoBase, SK_INIT_IO);
		SkEventInit	(pAC, pAC->IoBase, SK_INIT_IO);
#ifdef MV_INCLUDE_PNMI
		SkPnmiInit	(pAC, pAC->IoBase, SK_INIT_IO);
#endif
		SkAddrInit	(pAC, pAC->IoBase, SK_INIT_IO);
		SkRlmtInit	(pAC, pAC->IoBase, SK_INIT_IO);
		SkTimerInit	(pAC, pAC->IoBase, SK_INIT_IO);
#ifdef MV_INCLUDE_ASF_SUPPORT
#ifdef SK_CHECK_INIT
		printf("skGeEndStart: SkAsfInit called at level %d!\n", SK_INIT_IO);
#endif
		SkAsfInit(pAC, pAC->IoBase, SK_INIT_IO);
#endif
		pAC->BoardLevel = SK_INIT_IO;
	}
	
	if (pAC->BoardLevel != SK_INIT_RUN) {
		/* tschilling: Level 2 init modules here, check return value. */
#ifdef SK_CHECK_INIT
		printf("skGeEndStart: SkGeInit called at level %d!\n", SK_INIT_RUN);
#endif
		if (SkGeInit(pAC, pAC->IoBase, SK_INIT_RUN) != 0) {
            printf("%s: HWInit (2) failed.\n", pDrvCtrl->end.devObject.name);
			return (-1);
		}
		SkI2cInit	(pAC, pAC->IoBase, SK_INIT_RUN);
		SkEventInit	(pAC, pAC->IoBase, SK_INIT_RUN);
#ifdef MV_INCLUDE_PMNI
		SkPnmiInit	(pAC, pAC->IoBase, SK_INIT_RUN);
#endif
		SkAddrInit	(pAC, pAC->IoBase, SK_INIT_RUN);
		SkRlmtInit	(pAC, pAC->IoBase, SK_INIT_RUN);
		SkTimerInit	(pAC, pAC->IoBase, SK_INIT_RUN);

		pAC->BoardLevel = SK_INIT_RUN;
	}
#ifdef MV_INCLUDE_ASF_SUPPORT
#ifdef SK_CHECK_INIT
	printf("skGeEndStart: SkAsfInit called at level %d!\n", SK_INIT_RUN);
#endif
	if (SkAsfInit(pAC, pAC->IoBase, SK_INIT_RUN) != 0) {
		printf("%s: HWInit (2) failed SkAsfInit().\n", pDrvCtrl->end.devObject.name);
		return (-1);
	}
#endif

#ifdef SK_CHECK_INIT
	printf("Call SkGeY2InitStatBmu from skGeEndStart() after SK_INIT_RUN!\n");
#endif
	/* Moved from skY2AllocateResources() */
	SkGeY2InitStatBmu(pAC, pAC->IoBase, &pAC->StatusLETable);

	for (CurrMac=0; CurrMac<pAC->GIni.GIMacsFound; CurrMac++) {
		if (CHIP_ID_YUKON_2(pAC)) {
			skY2PortStart(pAC, pAC->IoBase, CurrMac);
		} else {
			/* Enable transmit descriptor polling. */
			SkGePollTxD(pAC, pAC->IoBase, CurrMac, SK_TRUE);
			skGeFillRxRing(pAC, &pAC->RxPort[CurrMac]);
			SkMacRxTxEnable(pAC, pAC->IoBase, pDrvCtrl->NetNr);
		}
	}
	
	SkGeYellowLED(pAC, pAC->IoBase, 1);
	SkDimEnableModerationIfNeeded(pAC);	

	if (!CHIP_ID_YUKON_2(pAC)) {
		/*
		** Has been setup already at SkGeInit(SK_INIT_IO),
		** but additional masking added for Genesis & Yukon
		** chipsets -> modify it...
		*/
		pAC->GIni.GIValIrqMask &= IRQ_MASK;
	#ifndef USE_TX_COMPLETE
		pAC->GIni.GIValIrqMask &= ~(TX_COMPL_IRQS);
	#endif
	}
	else
	{
		/* Set the interrupt handler pointer */
		isrRtn = skY2EndInt;
	}

#ifdef MV_INCLUDE_ASF_SUPPORT
	SK_IN32( pAC->IoBase, REG_ASF_STATUS_CMD, &TmpVal32 );
	TmpVal32 |= BIT_2; /* Dash works with Yukon Extreme */
	SK_OUT32( pAC->IoBase, REG_ASF_STATUS_CMD, TmpVal32 );
#endif

	/* enable Interrupts */
	SK_OUT32(pAC->IoBase, B0_IMSK, pAC->GIni.GIValIrqMask);
	SK_OUT32(pAC->IoBase, B0_HWE_IMSK, IRQ_HWE_MASK);
	
	/*semTake(pAC->SlowPathLock, WAIT_FOREVER);*/
	
	if ((pAC->RlmtMode != 0) && (pAC->MaxPorts == 0)) {
		EvPara.Para32[0] = pAC->RlmtNets;
		EvPara.Para32[1] = -1;
		SkEventQueue(pAC, SKGE_RLMT, SK_RLMT_SET_NETS,
			EvPara);
		EvPara.Para32[0] = pAC->RlmtMode;
		EvPara.Para32[1] = 0;
		SkEventQueue(pAC, SKGE_RLMT, SK_RLMT_MODE_CHANGE,
			EvPara);
	}
	
	EvPara.Para32[0] = pDrvCtrl->NetNr;
	EvPara.Para32[1] = -1;
	SkEventQueue(pAC, SKGE_RLMT, SK_RLMT_START, EvPara);
	SkEventDispatcher(pAC, pAC->IoBase);
	/*semGive(pAC->SlowPathLock);*/
	
	pAC->MaxPorts++;

	/* Wait for link to come up */
	for (CurrMac=0; CurrMac<pAC->GIni.GIMacsFound; CurrMac++) {
		startTicks = tickGet();
		while(!skGePortLinkIsUp(pAC, pAC->IoBase, CurrMac))
		{
			if (((tickGet() - startTicks) / sysClkRateGet()) > SK_LINK_TIMEOUT)
			{
				printf("Fail to get link for port %d\n", CurrMac);
				break;
			}
		}
	}

	pDrvCtrl->flags = DEFAULT_DRV_FLAGS;

    DRV_LOG (DRV_DEBUG_LOAD, "skGeEndStart: Connect interrupt\n", 
								1, 2, 3, 4, 5, 6);
    /* connect interrupt handler */

    SYS_INT_CONNECT(pDrvCtrl, isrRtn, (int)pDrvCtrl, &result);

    if (result == ERROR)
       {
       DRV_LOG (DRV_DEBUG_LOAD, "skGeEndStart: connect interrupt ERROR !\n", 
								1, 2, 3, 4, 5, 6);    
       return ERROR;
       }

    /* turn on system interrupt */

    SYS_INT_ENABLE(pDrvCtrl);


    DRV_LOG (DRV_DEBUG_LOAD, "skGeEndStart: Interrupt enable\n",  
                              1, 2, 3, 4, 5, 6);

    /* set the END flags, make driver ready to start */

#ifdef INCLUDE_SGI_RFC_1213
    END_FLAGS_SET (&pDrvCtrl->end, (IFF_UP | IFF_RUNNING | IFF_NOTRAILERS | 
                    IFF_BROADCAST | IFF_MULTICAST));   
#else
    END_FLAGS_SET (&pDrvCtrl->end, (IFF_UP | IFF_RUNNING | IFF_NOTRAILERS |
                    IFF_BROADCAST | IFF_MULTICAST | END_MIB_2233));
#endif

	pDrvCtrl->devStartFlag = TRUE;

    DRV_LOG (DRV_DEBUG_LOAD, "skGeEndStart...Done\n", 1, 2, 3, 4, 5, 6);
    
	return (OK);
    }

/*************************************************************************
*
* skGeEndSend - driver send routine
*
* This routine takes a M_BLK_ID sends off the data in the M_BLK_ID.
* The buffer must already have the addressing information properly installed
* in it. This is done by a higher layer.  
*
* RETURNS: OK or ERROR.
*
* ERRNO: EINVAL
*/

LOCAL STATUS skGeEndSend
    (
    END_DEVICE * pDrvCtrl,    /* device ptr */
    M_BLK_ID     pMblk        /* data to send */
    )
    {
    return (OK);
    }

/*************************************************************************
*
* skGeEndInt - handle SK-9xxx chip interrupt event
*
* This routine is called at interrupt level in response to an interrupt from
* the controller.
*
* RETURNS: N/A.
*/

LOCAL void skGeEndInt
    (
    END_DEVICE *    pDrvCtrl    /* device to handle interrupt */
    )
    {
	/* No support for Yukon II yet */

    return;
    }
/*************************************************************************
*
* skGeRecv - process an incoming packet
*
* This routine checks the incoming packet, and passes it the upper layer.
*
* RETURNS: OK, or ERROR if receiving fails
*/

LOCAL STATUS skGeRecv
    (
    END_DEVICE *    pDrvCtrl,    /* device structure */
    RX_PORT *       pRxPort
    )
    {
	/* No support for Yukon II yet */
    
	return ERROR;
    }

/*************************************************************************
*
* skGeEndIoctl - the driver I/O control routine
*
* Process an ioctl request.
*
* RETURNS: A command specific response, usually OK or ERROR.
*/

LOCAL int skGeEndIoctl
    (
    END_DEVICE *    pDrvCtrl,    /* device receiving command */
    int             cmd,         /* ioctl command code */
    caddr_t         data         /* command argument */
    )
    {
    int     error = 0;
    long    value;

    DRV_LOG (DRV_DEBUG_IOCTL, ("IOCTL command begin\n"), 0,0,0,0,0,0);

    switch ((UINT)cmd)
        {

        case EIOCSADDR:
            if (data == NULL) {
                return (EINVAL);
			}

            bcopy ((char *)data, (char *)END_HADDR(&pDrvCtrl->end),
                      END_HADDR_LEN(&pDrvCtrl->end));
            break;

        case EIOCGADDR:
            if (data == NULL)
                return (EINVAL);
            bcopy ((char *)END_HADDR(&pDrvCtrl->end), (char *)data,
                                           END_HADDR_LEN(&pDrvCtrl->end));
            break;

        case EIOCSFLAGS:
            value = (long)data;
            if (value < 0)
                {
                value = -value;
                value--;

                END_FLAGS_CLR (&pDrvCtrl->end, value);
                }
            else
                END_FLAGS_SET (&pDrvCtrl->end, value);

            DRV_LOG (DRV_DEBUG_IOCTL, ("endFlag=0x%x\n"), 
                                END_FLAGS_GET(&pDrvCtrl->end), 0,0,0,0,0);

            END_FLAGS_CLR (&pDrvCtrl->end, IFF_UP | IFF_RUNNING);

            skGeEndConfigure(pDrvCtrl);

            END_FLAGS_SET (&pDrvCtrl->end, IFF_UP | IFF_RUNNING);
            break;

        case EIOCGFLAGS:
            *(int *)data = END_FLAGS_GET(&pDrvCtrl->end);
            break;

        case EIOCPOLLSTART:    /* Begin polled operation */
            printf("sgi IOCTL EIOCPOLLSTART not supported\n");
            gei82543EndPollStart (pDrvCtrl);
            break;

        case EIOCPOLLSTOP:    /* End polled operation */
            printf("sgi IOCTL EIOCPOLLSTOP not supported\n");
            gei82543EndPollStop (pDrvCtrl);
            break;
           
        case EIOCGMIB2:        /* return MIB information */
            if (data == NULL)
                return (EINVAL);

            bcopy((char *)&pDrvCtrl->end.mib2Tbl, (char *)data,
                  sizeof(pDrvCtrl->end.mib2Tbl));
            break;

        case EIOCMULTIADD:
            printf("sgi IOCTL EIOCMULTIADD not supported\n");
            error = skGeEndMCastAdd (pDrvCtrl, (char *) data);
            break;

        case EIOCMULTIDEL:
            printf("sgi IOCTL EIOCMULTIDEL not supported\n");
            error = skGeEndMCastDel (pDrvCtrl, (char *) data);
            break;

        case EIOCMULTIGET:
            printf("sgi IOCTL EIOCMULTIGET not supported\n");
            error = skGeEndMCastGet (pDrvCtrl, (MULTI_TABLE *) data);
            break;

#ifndef INCLUDE_SGI_RFC_1213

        /* New RFC 2233 mib2 interface */

        case EIOCGMIB2233:
            if ((data == NULL) || (pDrvCtrl->end.pMib2Tbl == NULL))
                error = EINVAL;
            else
                *((M2_ID **)data) = pDrvCtrl->end.pMib2Tbl;
            break;

        case EIOCGPOLLCONF:
            if ((data == NULL))
                error = EINVAL;
            else
                *((END_IFDRVCONF **)data) = &pDrvCtrl->endStatsConf;
            break;

        case EIOCGPOLLSTATS:
            printf("sgi IOCTL EIOCGPOLLSTATS not supported\n");
            if ((data == NULL))
                error = EINVAL;
            else
                {
                /*error = gei82543EndStatsDump(pDrvCtrl);*/
                error = EINVAL;
                if (error == OK)
                    *((END_IFCOUNTERS **)data) = &pDrvCtrl->endStatsCounters;
                }
            break;

#endif /* INCLUDE_SGI_RFC_1213 */

        default:
            DRV_LOG (DRV_DEBUG_IOCTL, ("invalid IOCTL command \n"), 
                                        0,0,0,0,0,0);
            error = EINVAL;
        }

    DRV_LOG (DRV_DEBUG_IOCTL, ("IOCTL command Done\n"), 0,0,0,0,0,0);

    return (error);
    }

/*************************************************************************
*
* skGeEndConfigure - re-configure chip interface 
*
* This routine re-configures the interface such as promisc, multicast
* and broadcast modes
*
* RETURNS: N/A.
*/

LOCAL void skGeEndConfigure
    (
    END_DEVICE *    pDrvCtrl        /* device to configure */
    )
    {
    int    oldVal = 0;
    int    newVal = 0;
    DRV_LOG (DRV_DEBUG_IOCTL, ("skGeEndConfigure...\n"), 1,2,3,4,5,6);

    (void)taskLock ();
     
    /* save flags for later use */

    oldVal = pDrvCtrl->flags;

    newVal = END_FLAGS_GET(&pDrvCtrl->end);

    /* set the driver flag based on END_FLAGS */ 

    if (newVal & IFF_PROMISC)
        pDrvCtrl->flags |= FLAG_PROMISC_MODE;            
    else
        pDrvCtrl->flags &= ~FLAG_PROMISC_MODE;

    if (newVal & IFF_ALLMULTI)
        pDrvCtrl->flags |= FLAG_ALLMULTI_MODE;
    else
        pDrvCtrl->flags &= ~FLAG_ALLMULTI_MODE;

    if (newVal & IFF_MULTICAST)
        pDrvCtrl->flags |= FLAG_MULTICAST_MODE;
    else
        pDrvCtrl->flags &= ~FLAG_MULTICAST_MODE;

    if (newVal & IFF_BROADCAST)
        pDrvCtrl->flags |= FLAG_BROADCAST_MODE;
    else
        pDrvCtrl->flags &= ~FLAG_BROADCAST_MODE;

    if (oldVal == pDrvCtrl->flags)
        {
        (void)taskUnlock ();
        return;
        }

    newVal = oldVal;

    /* prepare new value for RX control register */

    if (pDrvCtrl->flags & FLAG_PROMISC_MODE)
		SkAddrPromiscuousChange(pDrvCtrl->pAC, pDrvCtrl->pAC->IoBase, pDrvCtrl->NetNr, (SK_PROM_MODE_LLC | SK_PROM_MODE_ALL_MC));
    else
		SkAddrPromiscuousChange(pDrvCtrl->pAC, pDrvCtrl->pAC->IoBase, pDrvCtrl->NetNr, SK_PROM_MODE_NONE);
    (void)taskUnlock ();

    DRV_LOG (DRV_DEBUG_IOCTL, ("skGeEndConfigure...Done\n"), 
                                0, 0, 0, 0, 0, 0);

    return;
    }

/*************************************************************************
*
* skGeEndMCastAdd - add a multicast address for the device
*
* This routine adds a multicast address to whatever the driver
* is already listening for.  It then resets the address filter.
*
* RETURNS: OK or ERROR.
*/

LOCAL STATUS skGeEndMCastAdd
    (
    END_DEVICE *    pDrvCtrl,        /* device pointer */
    char *          pAddress         /* new address to add */
    )
    {
    return (OK);
    }

/*************************************************************************
*
* skGeEndMCastDel - delete a multicast address for the device
*
* This routine removes a multicast address from whatever the driver
* is listening for.  It then resets the address filter.
*
* RETURNS: OK or ERROR.
*/

LOCAL STATUS skGeEndMCastDel
    (
    END_DEVICE *     pDrvCtrl,        /* device pointer */
    char *           pAddress         /* address to be deleted */
    )
    {
    return (OK);
    }

/*****************************************************************************
*
* skGeEndMCastGet - get the multicast address list for the device
*
* This routine gets the multicast list of whatever the driver
* is already listening for.
*
* RETURNS: OK or ERROR.
*/

LOCAL STATUS skGeEndMCastGet
    (
    END_DEVICE *    pDrvCtrl,        /* device pointer */
    MULTI_TABLE *   pTable            /* address table to be filled in */
    )
    {
    return (etherMultiGet (&pDrvCtrl->end.multiList, pTable));
    }

/*************************************************************************
*
* skGeEndPollRcv - routine to receive a packet in polling mode.
*
* This routine receive a packet in polling mode
*
* RETURNS: OK or ERROR.
*/

LOCAL STATUS skGeEndPollRcv
    (
    END_DEVICE * pDrvCtrl,    /* device to be polled */
    M_BLK_ID     pMblk        /* mBlk to receive */
    )
    {
	STATUS   retVal = OK;     /* return value */
	/* Poll mode unsupported yet */
    return (retVal);
    }
/*************************************************************************
*
* skGeEndPollSend - routine to send a packet in polling mode.
*
* This routine send a packet in polling mode
*
* RETURNS: OK or ERROR
*/

LOCAL STATUS skGeEndPollSend
    (
    END_DEVICE*     pDrvCtrl,    /* device to be polled */
    M_BLK_ID        pMblk        /* packet to send */
    )
    {
	/* Poll mode unsupported yet */
    return (OK);
    }
/*************************************************************************
*
* gei82543EndPollStart - start polling mode operations
*
* This routine starts the polling mode operation
*
* RETURNS: OK or ERROR.
*/

LOCAL STATUS gei82543EndPollStart
    (
    END_DEVICE * pDrvCtrl    /* device to be polled */
    )
    {
	/* Poll mode unsupported yet */
    return (OK);
    }

/*************************************************************************
*
* gei82543EndPollStop - stop polling mode operations
*
* This function terminates polled mode operation.  The device returns to
* interrupt mode. The device interrupts are enabled, the current mode flag is 
* switched to indicate interrupt mode and the device is then reconfigured for
* interrupt operation.
*
* RETURNS: OK or ERROR.
*/

LOCAL STATUS gei82543EndPollStop
    (
    END_DEVICE * pDrvCtrl    /* device to be polled */
    )
    {
	/* Poll mode unsupported yet */
    return (OK);
    }
/*************************************************************************
*
* skGeEndStop - stop the device
*
* This function calls BSP functions to disconnect interrupts and stop
* the device from operating in interrupt mode.
*
* RETURNS: OK or ERROR.
*/

LOCAL STATUS skGeEndStop
    (
    END_DEVICE *pDrvCtrl    /* device to be stopped */
    )
    {
    STATUS 		result = OK;
	SK_AC		*pAC;
	SK_EVPARA   EvPara;
	int         PortIdx;
	int         CurrMac;      /* loop ctr for the current MAC */

	pAC = pDrvCtrl->pAC;

    if (pDrvCtrl->attach != TRUE)
        return ERROR;

#ifdef MV_INCLUDE_ASF_SUPPORT
	SkAsfDeInit(pAC, pAC->IoBase);
#endif

    /* mark interface down */

    END_FLAGS_CLR (&pDrvCtrl->end, (IFF_UP | IFF_RUNNING));    

    /* turn off system interrupts */

    SYS_INT_DISABLE(pDrvCtrl);

	if(pAC->BoardLevel == SK_INIT_RUN) {
		PortIdx = pDrvCtrl->NetNr;
		
		/* disable TX/RX operation */
	
#ifdef MV_INCLUDE_ASF_SUPPORT
		SkAddrPromiscuousChange(pAC, pAC->IoBase, PortIdx,
								SK_PROM_MODE_ALL_MC);
#else
        /* Clear multicast table, promiscuous mode .... */
		SkAddrMcClear(pAC, pAC->IoBase, PortIdx, 0);
		SkAddrPromiscuousChange(pAC, pAC->IoBase, PortIdx, SK_PROM_MODE_NONE);
#endif
		/*semTake(pAC->SlowPathLock, WAIT_FOREVER);*/
		EvPara.Para32[0] = PortIdx;
		EvPara.Para32[1] = -1;
		SkEventQueue(pAC, SKGE_RLMT, SK_RLMT_STOP, EvPara);
		
		SkEventDispatcher(pAC, pAC->IoBase);
		
		/* disable interrupts */
		SK_OUT32(pAC->IoBase, B0_IMSK, 0);

#ifdef MV_INCLUDE_ASF_SUPPORT
		SkGeStopPort(pAC, pAC->IoBase, PortIdx, SK_STOP_RX, SK_HARD_RST);
		SkGeStopPort(pAC, pAC->IoBase, PortIdx, SK_STOP_TX, SK_HARD_RST);
#else
		SkGeStopPort(pAC, pAC->IoBase, PortIdx, SK_STOP_ALL, SK_HARD_RST);
#endif
		/*semGive(pAC->SlowPathLock);*/
	
		/* clear all descriptor rings */
		for (CurrMac=0; CurrMac<pAC->GIni.GIMacsFound; CurrMac++) {
			if (!CHIP_ID_YUKON_2(pAC)) {
				skGeRecv(pDrvCtrl, &pAC->RxPort[CurrMac]);
				skGeClearRxRing(pDrvCtrl, &pAC->RxPort[CurrMac]);
				skGeClearTxRing(pDrvCtrl, &pAC->TxPort[CurrMac][TX_PRIO_LOW]);
			} else {
				skY2FreeRxBuffers(pAC, pAC->IoBase, CurrMac);
				skY2FreeTxBuffers(pAC, pAC->IoBase, CurrMac);
			}
		}

		/* Stop the hardware */
		SkGeDeInit(pAC, pAC->IoBase);
	}    
	
	if(pAC->BoardLevel == SK_INIT_IO) {
		/* board is still alive */
		SkGeDeInit(pAC, pAC->IoBase);
	}

	pAC->BoardLevel = SK_INIT_DATA;

	/* check device start flag */
    if (pDrvCtrl->devStartFlag != TRUE)
        return OK;

    /* disconnect interrupt handler */

    SYS_INT_DISCONNECT(pDrvCtrl, skGeEndInt, (int)pDrvCtrl, &result);

    if (result == ERROR)
        {
        DRV_LOG (DRV_DEBUG_LOAD, "Could not disconnect interrupt!\n", 1, 2, 
                                  3, 4, 5, 6);
        }

    pDrvCtrl->devStartFlag = FALSE;
	
    return (result);
    }

/*************************************************************************
*
* skGeEndUnload - unload a driver from the system
*
* This function first brings down the device, and then frees any
* stuff that was allocated by the driver in the load function.
*
* RETURNS: OK or ERROR.
*/

LOCAL STATUS skGeEndUnload
    (
    END_DEVICE* pDrvCtrl    /* device to be unloaded */
    )
    {
	SK_EVPARA EvPara;
	SK_AC	*pAC;

	pAC = pDrvCtrl->pAC;

    /* delay a while in case device tries to stop */

    taskDelay (sysClkRateGet () * 2);

    END_OBJECT_UNLOAD (&pDrvCtrl->end);

	SkGeYellowLED(pAC, pAC->IoBase, 0);

	if(pAC->BoardLevel == SK_INIT_RUN) {
		/* board is still alive */
		/*semTake(pAC->SlowPathLock, WAIT_FOREVER);*/
		EvPara.Para32[0] = 0;
		EvPara.Para32[1] = -1;
		SkEventQueue(pAC, SKGE_RLMT, SK_RLMT_STOP, EvPara);
		SkEventDispatcher(pAC, pAC->IoBase);
		/* disable interrupts */
		SK_OUT32(pAC->IoBase, B0_IMSK, 0);
		SkGeDeInit(pAC, pAC->IoBase);
		/*semGive(pAC->SlowPathLock);*/
		pAC->BoardLevel = SK_INIT_DATA;
		/* We do NOT check here, if IRQ was pending, of course*/
	}
	
	if(pAC->BoardLevel == SK_INIT_IO) {
		/* board is still alive */
		SkGeDeInit(pAC, pAC->IoBase);
		pAC->BoardLevel = SK_INIT_DATA;
	}
		
    /* free allocated memory */
    skGeMemAllFree (pDrvCtrl);    

    pDrvCtrl->attach = FALSE;

    /* release the adapter structure */
	cfree ((char *)pDrvCtrl->pAC);

	/* release the END device structure */
    cfree ((char *)pDrvCtrl);
    
    return (OK);
    }

/*****************************************************************************
 *
 *	SkPciReadCfgDWord - read a 32 bit value from pci config space
 *
 * Description:
 *	This routine reads a 32 bit value from the pci configuration
 *	space.
 *
 * Returns:
 *	0 - indicate everything worked ok.
 *	!= 0 - error indication
 */
int SkPciReadCfgDWord(
	SK_AC *pAC,			/* Adapter Control structure pointer */
	int PciAddr,		/* PCI register address */
	SK_U32 *pVal)		/* pointer to store the read value */
{
	pciConfigInLong(pAC->pciDev.pciBusNo,
					pAC->pciDev.pciDeviceNo,
					pAC->pciDev.pciFuncNo,
					PciAddr, pVal);
	return OK;
} /* SkPciReadCfgDWord */


/*****************************************************************************
 *
 *	SkPciReadCfgWord - read a 16 bit value from pci config space
 *
 * Description:
 *	This routine reads a 16 bit value from the pci configuration
 *	space.
 *
 * Returns:
 *	0 - indicate everything worked ok.
 *	!= 0 - error indication
 */
int SkPciReadCfgWord(
SK_AC *pAC,	/* Adapter Control structure pointer */
int PciAddr,		/* PCI register address */
SK_U16 *pVal)		/* pointer to store the read value */
{
	pciConfigInWord(pAC->pciDev.pciBusNo,
					pAC->pciDev.pciDeviceNo,
					pAC->pciDev.pciFuncNo,
					PciAddr, pVal);
	return OK;
} /* SkPciReadCfgWord */


/*****************************************************************************
 *
 *	SkPciReadCfgByte - read a 8 bit value from pci config space
 *
 * Description:
 *	This routine reads a 8 bit value from the pci configuration
 *	space.
 *
 * Returns:
 *	0 - indicate everything worked ok.
 *	!= 0 - error indication
 */
int SkPciReadCfgByte(
SK_AC *pAC,	/* Adapter Control structure pointer */
int PciAddr,		/* PCI register address */
SK_U8 *pVal)		/* pointer to store the read value */
{
	pciConfigInByte(pAC->pciDev.pciBusNo,
					pAC->pciDev.pciDeviceNo,
					pAC->pciDev.pciFuncNo,
					PciAddr, pVal);
	return OK;
} /* SkPciReadCfgByte */


/*****************************************************************************
 *
 *	SkPciWriteCfgDWord - write a 32 bit value to pci config space
 *
 * Description:
 *	This routine writes a 32 bit value to the pci configuration
 *	space.
 *
 * Returns:
 *	0 - indicate everything worked ok.
 *	!= 0 - error indication
 */
int SkPciWriteCfgDWord(
SK_AC *pAC,	/* Adapter Control structure pointer */
int PciAddr,		/* PCI register address */
SK_U32 Val)		/* pointer to store the read value */
{
	pciConfigOutLong(pAC->pciDev.pciBusNo,
					pAC->pciDev.pciDeviceNo,
					pAC->pciDev.pciFuncNo,
					PciAddr, Val);
	return OK;
} /* SkPciWriteCfgDWord */


/*****************************************************************************
 *
 *	SkPciWriteCfgWord - write a 16 bit value to pci config space
 *
 * Description:
 *	This routine writes a 16 bit value to the pci configuration
 *	space. The flag PciConfigUp indicates whether the config space
 *	is accesible or must be set up first.
 *
 * Returns:
 *	0 - indicate everything worked ok.
 *	!= 0 - error indication
 */
int SkPciWriteCfgWord(
SK_AC *pAC,	/* Adapter Control structure pointer */
int PciAddr,		/* PCI register address */
SK_U16 Val)		/* pointer to store the read value */
{
	pciConfigOutWord(pAC->pciDev.pciBusNo,
					pAC->pciDev.pciDeviceNo,
					pAC->pciDev.pciFuncNo,
					PciAddr, Val);
	return OK;
} /* SkPciWriteCfgWord */


/*****************************************************************************
 *
 *	SkPciWriteCfgWord - write a 8 bit value to pci config space
 *
 * Description:
 *	This routine writes a 8 bit value to the pci configuration
 *	space. The flag PciConfigUp indicates whether the config space
 *	is accesible or must be set up first.
 *
 * Returns:
 *	0 - indicate everything worked ok.
 *	!= 0 - error indication
 */
int SkPciWriteCfgByte(
SK_AC *pAC,	/* Adapter Control structure pointer */
int PciAddr,		/* PCI register address */
SK_U8 Val)		/* pointer to store the read value */
{
	pciConfigOutByte(pAC->pciDev.pciBusNo,
					pAC->pciDev.pciDeviceNo,
					pAC->pciDev.pciFuncNo,
					PciAddr, Val);
	return OK;
} /* SkPciWriteCfgByte */

/*****************************************************************************
 *
 * 	skGeDeviceInit - do level 0 and 1 initialization
 *
 * Description:
 *	This function prepares the board hardware for running. The desriptor
 *	ring is set up, the IRQ is allocated and the configuration settings
 *	are examined.
 *
 * Returns:
 *	0, if everything is ok
 *	!=0, on error
 */
static STATUS skGeDeviceInit(END_DEVICE *pDrvCtrl)
{
	short  	i;
	SK_BOOL	DualNet;
	SK_AC  	*pAC = pDrvCtrl->pAC;
#ifdef MV_INCLUDE_PNMI
	char	*DescrString = DRIVER_FILE_NAME; /* this is given to PNMI */
	char	*VerStr	= VER_STRING;
#endif

	SK_DBG_MSG(NULL, SK_DBGMOD_DRV, SK_DBGCAT_DRV_ENTRY,
		("IoBase: %08lX\n", (unsigned long)pAC->IoBase));

	for (i=0; i<SK_MAX_MACS; i++) {
		pAC->TxPort[i][0].HwAddr = pAC->IoBase + TxQueueAddr[i][0];
		pAC->TxPort[i][0].PortIndex = i;
		pAC->RxPort[i].HwAddr = pAC->IoBase + RxQueueAddr[i];
		pAC->RxPort[i].PortIndex = i;

	}
	/* Initialize the mutexes */
	for (i=0; i<SK_MAX_MACS; i++) {
		pAC->TxPort[i][0].TxDesRingLock = semBCreate(SEM_Q_FIFO, SEM_FULL);
		pAC->RxPort[i].RxDesRingLock = semBCreate(SEM_Q_FIFO, SEM_FULL);
	}

	/*pAC->SlowPathLock = semBCreate(SEM_Q_FIFO, SEM_FULL);*/
	pAC->SetPutIndexLock = semBCreate(SEM_Q_FIFO, SEM_FULL); /* for Yukon2 chipsets */

	/* level 0 init common modules here */
	
	/*semTake(pAC->SlowPathLock, WAIT_FOREVER);*/
	/* Does a RESET on board ...*/
#ifdef SK_CHECK_INIT
	printf("skGeDeviceInit: SkGeInit called at level %d!\n", SK_INIT_DATA);
#endif
	if (SkGeInit(pAC, pAC->IoBase, SK_INIT_DATA) != 0) {
		printf("sgi: HWInit (0) failed.\n");
		/*semGive(pAC->SlowPathLock);*/
		return(ERROR);
	}
	SkI2cInit(  pAC, pAC->IoBase, SK_INIT_DATA);
	SkEventInit(pAC, pAC->IoBase, SK_INIT_DATA);

#ifdef MV_INCLUDE_PNMI
	SkPnmiInit( pAC, pAC->IoBase, SK_INIT_DATA);
#endif
	SkAddrInit( pAC, pAC->IoBase, SK_INIT_DATA);
	SkRlmtInit( pAC, pAC->IoBase, SK_INIT_DATA);
	SkTimerInit(pAC, pAC->IoBase, SK_INIT_DATA);
#ifdef MV_INCLUDE_ASF_SUPPORT
#ifdef SK_CHECK_INIT
	printf("skGeDeviceInit: SkAsfInit called at level %d!\n", SK_INIT_DATA);
#endif
	SkAsfInit(pAC, pAC->IoBase, SK_INIT_DATA);
#endif

	pAC->BoardLevel = SK_INIT_DATA;

#ifdef MV_INCLUDE_PNMI
	SK_PNMI_SET_DRIVER_DESCR(pAC, DescrString);
	SK_PNMI_SET_DRIVER_VER(pAC, VerStr);
#endif
	/* level 1 init common modules here (HW init) */
#ifdef SK_CHECK_INIT
	printf("skGeDeviceInit: SkGeInit called at level %d!\n", SK_INIT_IO);
#endif
	if (SkGeInit(pAC, pAC->IoBase, SK_INIT_IO) != 0) {
		printf("sgi: HWInit (1) failed.\n");
		return(ERROR);
	}
	SkI2cInit(  pAC, pAC->IoBase, SK_INIT_IO);
	SkEventInit(pAC, pAC->IoBase, SK_INIT_IO);
#ifdef MV_INCLUDE_PNMI
	SkPnmiInit( pAC, pAC->IoBase, SK_INIT_IO);
#endif
	SkAddrInit( pAC, pAC->IoBase, SK_INIT_IO);
	SkRlmtInit( pAC, pAC->IoBase, SK_INIT_IO);
	SkTimerInit(pAC, pAC->IoBase, SK_INIT_IO);
#ifdef MV_INCLUDE_ASF_SUPPORT
#ifdef SK_CHECK_INIT
	printf("skGeDeviceInit: SkAsfInit called at level %d!\n", SK_INIT_IO);
#endif
	SkAsfInit(pAC, pAC->IoBase, SK_INIT_IO);
#endif

	/* Set chipset type support */
	if ((pAC->GIni.GIChipId == CHIP_ID_YUKON) ||
		(pAC->GIni.GIChipId == CHIP_ID_YUKON_LITE) ||
		(pAC->GIni.GIChipId == CHIP_ID_YUKON_LP)) {
		pAC->ChipsetType = 1;	/* Yukon chipset (descriptor logic) */
	} else if (CHIP_ID_YUKON_2(pAC)) {
		pAC->ChipsetType = 2;	/* Yukon2 chipset (list logic) */
	}

#ifdef MV_INCLUDE_WOL
	/* wake on lan support */
	pAC->WolInfo.SupportedWolOptions = 0;
#if defined (ETHTOOL_GWOL) && defined (ETHTOOL_SWOL)
	if (pAC->GIni.GIChipId != CHIP_ID_GENESIS) {
		pAC->WolInfo.SupportedWolOptions  = WAKE_MAGIC;
		if (pAC->GIni.GIChipId == CHIP_ID_YUKON) {
			if (pAC->GIni.GIChipRev == 0) {
				pAC->WolInfo.SupportedWolOptions = 0;
			}
		} 
	}
#endif
	pAC->WolInfo.ConfiguredWolOptions = pAC->WolInfo.SupportedWolOptions;
#endif /* MV_INCLUDE_WOL */
	 
	/* The rxBuffHwOffs holds the offset from Rx cluster that is given 	*/
	/* to HW as Rx buffer base. It is done when the device has no Rx 	*/
	/* alignment restrictions. This way the the HW places the Rx buffer */
	/* so the IP header will be 4bytes aligned. 						*/
	/* The rxBuffSwShift holds the number of bytes the Rx buffer 		*/
	/* (already received) should be shifted so the IP header will be 	*/
	/* 4bytes aligned. 													*/
	if ((pAC->GIni.GIChipId == CHIP_ID_YUKON_EC) ||
		(pAC->GIni.GIChipId == CHIP_ID_YUKON_FE) ||
		(pAC->GIni.GIChipId == CHIP_ID_YUKON_EX) ||
		(pAC->GIni.GIChipId == CHIP_ID_YUKON_SUPR))
	{
		/* Yukon EC and FE must have the Rx buffers 8 bytes aligned. 	*/
		/* The CPU must shift the Rx buffer 2 bytes in order for the 	*/
		/* IP header to be 4 bytes aligned. 							*/
		pDrvCtrl->rxBuffHwOffs  = 0;
		pDrvCtrl->rxBuffSwShift = pDrvCtrl->offset;     /* skGeEndParse */
	}
	else
	{
		/* Yukon XL does not have any alignment restriction. 			*/
		pDrvCtrl->rxBuffHwOffs  = pDrvCtrl->offset;     /* skGeEndParse */
		pDrvCtrl->rxBuffSwShift = 0;
	}
	
	skGeGetConfiguration(pDrvCtrl);

	if (pAC->RlmtNets == 2) {
		pAC->GIni.GP[0].PPortUsage = SK_MUL_LINK;
		pAC->GIni.GP[1].PPortUsage = SK_MUL_LINK;
	}

	pAC->BoardLevel = SK_INIT_IO;
	
	DualNet = SK_FALSE;
	if (pAC->RlmtNets == 2) {
		DualNet = SK_TRUE;
	}

	if (SkGeInitAssignRamToQueues(pAC, pAC->ActivePort, DualNet)) {
		printf("sk98lin: SkGeInitAssignRamToQueues failed.\n");
		return(ERROR);
	}
	
	return OK;
} /* SkGeBoardInit */


/*****************************************************************************
 *
 * 	skGeProductStrGet - return a adapter identification string from vpd
 *
 * Description:
 *	This function reads the product name string from the vpd area
 *	and puts it the field pAC->DeviceString.
 *
 * Returns: N/A
 */
static void skGeProductStrGet(SK_AC	*pAC)
{
	int	 StrLen = 80;			/* length of the string, defined in SK_AC */
	char Keyword[] = VPD_NAME;	/* vpd productname identifier */
	int	 ReturnCode;			/* return code from vpd_read */

    ReturnCode = VpdRead(pAC, pAC->IoBase, Keyword, pAC->DeviceStr, &StrLen);
	
	if (ReturnCode != 0) {
		/* there was an error reading the vpd data */
		SK_DBG_MSG(NULL, SK_DBGMOD_DRV, SK_DBGCAT_DRV_ERROR,
			("Error reading VPD data: %d\n", ReturnCode));
		pAC->DeviceStr[0] = '\0';
	}
} /* ProductStr */


/*****************************************************************************
 *
 * 	skGeGetConfiguration - read configuration information
 *
 * Description:
 *
 * Returns:
 *	none
 */
static void skGeGetConfiguration(END_DEVICE *pDrvCtrl)
{
	SK_I32	Port;		/* preferred port */
	SK_AC	*pAC = pDrvCtrl->pAC;

	/*
	** Set the default values first for both ports!
	*/
	for (Port = 0; Port < SK_MAX_MACS; Port++) {
		pAC->GIni.GP[Port].PLinkModeConf = SK_LMODE_AUTOBOTH;
		pAC->GIni.GP[Port].PFlowCtrlMode = SK_FLOW_MODE_SYM_OR_REM;
		pAC->GIni.GP[Port].PMSMode       = SK_MS_MODE_AUTO;
		pAC->GIni.GP[Port].PLinkSpeed    = SK_LSPEED_AUTO;
	}

	pAC->RlmtNets = 1;
	pAC->RlmtMode = 0;
	pAC->LowLatency = SK_FALSE;
    pAC->DynIrqModInfo.IntModTypeSelect = C_INT_MOD_NONE;

	if (!CHIP_ID_YUKON_2(pAC)) {
		pAC->DynIrqModInfo.MaskIrqModeration = IRQ_MASK_RX_TX_SP;
	} else {
		pAC->DynIrqModInfo.MaskIrqModeration = Y2_IRQ_MASK;
	}

	if (!CHIP_ID_YUKON_2(pAC)) {
		pAC->DynIrqModInfo.MaxModIntsPerSec = C_INTS_PER_SEC_DEFAULT;
	} else {
		pAC->DynIrqModInfo.MaxModIntsPerSec = C_Y2_INTS_PER_SEC_DEFAULT;
	}

	/*
	** Evaluate upper and lower moderation threshold
	*/
	pAC->DynIrqModInfo.MaxModIntsPerSecUpperLimit =
		pAC->DynIrqModInfo.MaxModIntsPerSec +
		(pAC->DynIrqModInfo.MaxModIntsPerSec / 5);

	pAC->DynIrqModInfo.MaxModIntsPerSecLowerLimit =
		pAC->DynIrqModInfo.MaxModIntsPerSec -
		(pAC->DynIrqModInfo.MaxModIntsPerSec / 5);

	pAC->DynIrqModInfo.DynIrqModSampleInterval = 
		SK_DRV_MODERATION_TIMER_LENGTH;

} /* GetConfiguration */

/*****************************************************************************
 *
 * 	FillRxRing - fill the receive ring with valid descriptors
 *
 * Description:
 *	This function fills the receive ring descriptors with data
 *	segments and makes them valid for the BMU.
 *	The active ring is filled completely, if possible.
 *	The non-active ring is filled only partial to save memory.
 *
 * Description of rx ring structure:
 *	head - points to the descriptor which will be used next by the BMU
 *	tail - points to the next descriptor to give to the BMU
 *	
 * Returns:	N/A
 */
static void skGeFillRxRing(
SK_AC		*pAC,		/* pointer to the adapter context */
RX_PORT		*pRxPort)	/* ptr to port struct for which the ring
				   should be filled */
{
	semTake(pRxPort->RxDesRingLock, WAIT_FOREVER);
	while (pRxPort->RxdRingFree > pRxPort->RxFillLimit) {
		if(!skGeFillRxDescriptor(pAC, pRxPort))
			break;
	}
	semGive(pRxPort->RxDesRingLock);
} /* FillRxRing */

/*****************************************************************************
 *
 * 	skGeFillRxDescriptor - fill one buffer into the receive ring
 *
 * Description:
 *	The function allocates a new receive buffer and
 *	puts it into the next descriptor.
 *
 * Returns:
 *	SK_TRUE - a buffer was added to the ring
 *	SK_FALSE - a buffer could not be added
 */
static SK_BOOL skGeFillRxDescriptor(
SK_AC		*pAC,		/* pointer to the adapter context struct */
RX_PORT		*pRxPort)	/* ptr to port struct of ring to fill */
{
    return (SK_TRUE);
} /* skGeFillRxDescriptor */

/*****************************************************************************
 *
 * 	skGeAllocateResources - allocate the memory for the descriptor rings
 *
 * Description:
 *	This function allocates the memory for all descriptor rings.
 *	Each ring is aligned for the desriptor alignment and no ring
 *	has a 4 GByte boundary in it (because the upper 32 bit must
 *	be constant for all descriptiors in one rings).
 *
 * Returns:
 *	SK_TRUE, if all memory could be allocated
 *	SK_FALSE, if not
 */
static SK_BOOL skGeAllocateResources(END_DEVICE *pDrvCtrl)
{
	caddr_t		pDescrMem;	/* pointer to descriptor memory area */
	size_t		AllocLength;	/* length of complete descriptor area */
	int		i;		/* loop counter */
	unsigned long	BusAddr;
	SK_AC	*pAC = pDrvCtrl->pAC;

	
	/* rings plus one for alignment (do not cross 4 GB boundary) */
	/* RX_RING_SIZE is assumed bigger than TX_RING_SIZE */
#if (BITS_PER_LONG == 32)
	AllocLength = (RX_RING_SIZE + TX_RING_SIZE) * pAC->GIni.GIMacsFound + 8;
#else
	AllocLength = (RX_RING_SIZE + TX_RING_SIZE) * pAC->GIni.GIMacsFound
		+ RX_RING_SIZE + 8;
#endif

	pDescrMem = cacheDmaMalloc(AllocLength);

	if (pDescrMem == NULL) {
		return (SK_FALSE);
	}
	pDrvCtrl->pDescMemBase = pDescrMem;
	BusAddr = (unsigned long)SGI_VIRT_TO_PHYS(pDescrMem);

	/* Descriptors need 8 byte alignment, and this is ensured
	 * by pci_alloc_consistent.
	 */
	for (i=0; i<pAC->GIni.GIMacsFound; i++) {
		SK_DBG_MSG(NULL, SK_DBGMOD_DRV, SK_DBGCAT_DRV_TX_PROGRESS,
			("TX%d/A: pDescrMem: %lX,   PhysDescrMem: %lX\n",
			i, (unsigned long) pDescrMem,
			BusAddr));
		pAC->TxPort[i][0].pTxDescrRing = pDescrMem;
		pAC->TxPort[i][0].VTxDescrRing = BusAddr;
		pDescrMem += TX_RING_SIZE;
		BusAddr += TX_RING_SIZE;
	
		SK_DBG_MSG(NULL, SK_DBGMOD_DRV, SK_DBGCAT_DRV_TX_PROGRESS,
			("RX%d: pDescrMem: %lX,   PhysDescrMem: %lX\n",
			i, (unsigned long) pDescrMem,
			(unsigned long)BusAddr));
		pAC->RxPort[i].pRxDescrRing = pDescrMem;
		pAC->RxPort[i].VRxDescrRing = BusAddr;
		pDescrMem += RX_RING_SIZE;
		BusAddr += RX_RING_SIZE;
	} /* for */
	
	return (SK_TRUE);
} /* skGeAllocateResources */


/****************************************************************************
 *
 *	skGeFreeResources - reverse of skGeAllocateResources
 *
 * Description:
 *	Free all memory allocated in skGeAllocateResources: adapter context,
 *	descriptor rings, locks.
 *
 * Returns:	N/A
 */
static void skGeFreeResources(END_DEVICE *pDrvCtrl)
{

	SK_DBG_MSG(NULL, SK_DBGMOD_DRV, SK_DBGCAT_DRV_ENTRY,
		("skGeFreeResources\n"));

	if (pDrvCtrl->pDescMemBase) {
		cacheDmaFree(pDrvCtrl->pDescMemBase);
		pDrvCtrl->pDescMemBase = NULL;
	}
} /* skGeFreeResources */


/*****************************************************************************
 *
 * 	SetupRing - create one descriptor ring
 *
 * Description:
 *	This function creates one descriptor ring in the given memory area.
 *	The head, tail and number of free descriptors in the ring are set.
 *
 * Returns:
 *	none
 */
static void SetupRing(
	SK_AC		*pAC,
	void		*pMemArea,	/* a pointer to the memory area for the ring */
	uintptr_t	VMemArea,	/* the virtual bus address of the memory area */
	RXD		**ppRingHead,	/* address where the head should be written */
	RXD		**ppRingTail,	/* address where the tail should be written */
	RXD		**ppRingPrev,	/* address where the tail should be written */
	int		*pRingFree,	/* address where the # of free descr. goes */
	int		*pRingPrevFree,	/* address where the # of free descr. goes */
	SK_BOOL		IsTx)		/* flag: is this a tx ring */
{
	int	i;		/* loop counter */
	int	DescrSize;	/* the size of a descriptor rounded up to alignment*/
	int	DescrNum;	/* number of descriptors per ring */
	RXD	*pDescr;	/* pointer to a descriptor (receive or transmit) */
	RXD	*pNextDescr;	/* pointer to the next descriptor */
	RXD	*pPrevDescr;	/* pointer to the previous descriptor */
	uintptr_t VNextDescr;	/* the virtual bus address of the next descriptor */

	if (IsTx == SK_TRUE) {
		DescrSize = (((sizeof(TXD) - 1) / DESCR_ALIGN) + 1) *
			DESCR_ALIGN;
		DescrNum = TX_RING_SIZE / DescrSize;
	} else {
		DescrSize = (((sizeof(RXD) - 1) / DESCR_ALIGN) + 1) *
			DESCR_ALIGN;
		DescrNum = RX_RING_SIZE / DescrSize;
	}
	
	SK_DBG_MSG(NULL, SK_DBGMOD_DRV, SK_DBGCAT_DRV_TX_PROGRESS,
		("Descriptor size: %d   Descriptor Number: %d\n",
		DescrSize,DescrNum));
	
	pDescr = (RXD*) pMemArea;
	pPrevDescr = NULL;
	pNextDescr = (RXD*) (((char*)pDescr) + DescrSize);
	VNextDescr = VMemArea + DescrSize;
	for(i=0; i<DescrNum; i++) {
		/* set the pointers right */
		pDescr->VNextRxd = VNextDescr & 0xffffffffULL;
		pDescr->pNextRxd = pNextDescr;
		pDescr->TcpSumStarts = pAC->CsOfs;

		/* advance one step */
		pPrevDescr = pDescr;
		pDescr = pNextDescr;
		pNextDescr = (RXD*) (((char*)pDescr) + DescrSize);
		VNextDescr += DescrSize;
	}
	pPrevDescr->pNextRxd = (RXD*) pMemArea;
	pPrevDescr->VNextRxd = VMemArea;
	pDescr               = (RXD*) pMemArea;
	*ppRingHead          = (RXD*) pMemArea;
	*ppRingTail          = *ppRingHead;
	*ppRingPrev          = pPrevDescr;
	*pRingFree           = DescrNum;
	*pRingPrevFree       = DescrNum;
} /* SetupRing */

/*****************************************************************************
 *
 * 	skGeDescRingInit - initiate the descriptor rings
 *
 * Description:
 *	This function sets the descriptor rings or LETables up in memory.
 *	The adapter is initialized with the descriptor start addresses.
 *
 * Returns:	N/A
 */
static void skGeDescRingInit(END_DEVICE *pDrvCtrl)
{
	int	i;				/* loop counter */
	int	RxDescrSize;	/* the size of a rx descriptor rounded up to alignment*/
	int	TxDescrSize;	/* the size of a tx descriptor rounded up to alignment*/
	SK_AC	*pAC = pDrvCtrl->pAC;

	SK_DBG_MSG(NULL, SK_DBGMOD_DRV, SK_DBGCAT_DRV_ENTRY,
		("skGeDescRingInit\n"));

	if (!pAC->GIni.GIYukon2) {
		RxDescrSize = (((sizeof(RXD) - 1) / DESCR_ALIGN) + 1) * DESCR_ALIGN;
		pAC->RxDescrPerRing = RX_RING_SIZE / RxDescrSize;
		TxDescrSize = (((sizeof(TXD) - 1) / DESCR_ALIGN) + 1) * DESCR_ALIGN;
		pAC->TxDescrPerRing = TX_RING_SIZE / RxDescrSize;
	
		for (i=0; i<pAC->GIni.GIMacsFound; i++) {
			SetupRing(
				pAC,
				pAC->TxPort[i][0].pTxDescrRing,
				pAC->TxPort[i][0].VTxDescrRing,
				(RXD**)&pAC->TxPort[i][0].pTxdRingHead,
				(RXD**)&pAC->TxPort[i][0].pTxdRingTail,
				(RXD**)&pAC->TxPort[i][0].pTxdRingPrev,
				&pAC->TxPort[i][0].TxdRingFree,
				&pAC->TxPort[i][0].TxdRingPrevFree,
				SK_TRUE);
			SetupRing(
				pAC,
				pAC->RxPort[i].pRxDescrRing,
				pAC->RxPort[i].VRxDescrRing,
				&pAC->RxPort[i].pRxdRingHead,
				&pAC->RxPort[i].pRxdRingTail,
				&pAC->RxPort[i].pRxdRingPrev,
				&pAC->RxPort[i].RxdRingFree,
				&pAC->RxPort[i].RxdRingFree,
				SK_FALSE);
		}
	}
} /* skGeDescRingInit */
	  
/*****************************************************************************
 *
 * 	PortReInitBmu - re-initiate the descriptor rings for one port
 *
 * Description:
 *	This function reinitializes the descriptor rings of one port
 *	in memory. The port must be stopped before.
 *	The HW is initialized with the descriptor start addresses.
 *
 * Returns:
 *	none
 */
static void PortReInitBmu(
SK_AC	*pAC,		/* pointer to adapter context */
int	PortIndex)	/* index of the port for which to re-init */
{
	SK_DBG_MSG(NULL, SK_DBGMOD_DRV, SK_DBGCAT_DRV_ENTRY,
		("PortReInitBmu "));

	/* set address of first descriptor of ring in BMU */
	SK_OUT32(pAC->IoBase, TxQueueAddr[PortIndex][TX_PRIO_LOW]+ Q_DA_L,
		(uint32_t)(((caddr_t)
		(pAC->TxPort[PortIndex][TX_PRIO_LOW].pTxdRingHead) -
		pAC->TxPort[PortIndex][TX_PRIO_LOW].pTxDescrRing +
		pAC->TxPort[PortIndex][TX_PRIO_LOW].VTxDescrRing) &
		0xFFFFFFFF));
	SK_OUT32(pAC->IoBase, TxQueueAddr[PortIndex][TX_PRIO_LOW]+ Q_DA_H,
		(uint32_t)(((caddr_t)
		(pAC->TxPort[PortIndex][TX_PRIO_LOW].pTxdRingHead) -
		pAC->TxPort[PortIndex][TX_PRIO_LOW].pTxDescrRing +
		pAC->TxPort[PortIndex][TX_PRIO_LOW].VTxDescrRing) >> 32));
	SK_OUT32(pAC->IoBase, RxQueueAddr[PortIndex]+Q_DA_L,
		(uint32_t)(((caddr_t)(pAC->RxPort[PortIndex].pRxdRingHead) -
		pAC->RxPort[PortIndex].pRxDescrRing +
		pAC->RxPort[PortIndex].VRxDescrRing) & 0xFFFFFFFF));
	SK_OUT32(pAC->IoBase, RxQueueAddr[PortIndex]+Q_DA_H,
		(uint32_t)(((caddr_t)(pAC->RxPort[PortIndex].pRxdRingHead) -
		pAC->RxPort[PortIndex].pRxDescrRing +
		pAC->RxPort[PortIndex].VRxDescrRing) >> 32));
} /* PortReInitBmu */

/*****************************************************************************
 *
 * 	skGeClearRxRing - remove all buffers from the receive ring
 *
 * Description:
 *	This function removes all receive buffers from the ring.
 *	The receive BMU must be stopped before calling this function.
 *
 * Returns: N/A
 */
static void skGeClearRxRing(
	END_DEVICE	*pDrvCtrl,		/* pointer to adapter context */
	RX_PORT	*pRxPort)	/* pointer to rx port struct */
{
	RXD				*pRxd;	/* pointer to the current descriptor */
	SK_AC			*pAC = pDrvCtrl->pAC;

	if (pRxPort->RxdRingFree == pAC->RxDescrPerRing) {
		return;
	}
	semTake(pRxPort->RxDesRingLock, WAIT_FOREVER);
	pRxd = pRxPort->pRxdRingHead;
	do {
		if (pRxd->pMBuf != NULL) {

			netClFree (pDrvCtrl->end.pNetPool, pRxd->pMBuf->pClBlk->clNode.pClBuf);
			pRxd->pMBuf = NULL;
		}
		pRxd->RBControl &= BMU_OWN;
		pRxd = pRxd->pNextRxd;
		pRxPort->RxdRingFree++;
	} while (pRxd != pRxPort->pRxdRingTail);
	pRxPort->pRxdRingTail = pRxPort->pRxdRingHead;
	semGive(pRxPort->RxDesRingLock);
} /* skGeClearRxRing */

/*****************************************************************************
 *
 *	skGeClearTxRing - remove all buffers from the transmit ring
 *
 * Description:
 *	This function removes all transmit buffers from the ring.
 *	The transmit BMU must be stopped before calling this function
 *	and transmitting at the upper level must be disabled.
 *	The BMU own bit of all descriptors is cleared, the rest is
 *	done by calling FreeTxDescriptors.
 *
 * Returns: N/A
 */
static void skGeClearTxRing(
	END_DEVICE	*pDrvCtrl,		/* pointer to adapter context */
	TX_PORT	*pTxPort)	/* pointer to tx prt struct */
{
	TXD	  *pTxd;		/* pointer to the current descriptor */
	int	  i;
	SK_AC *pAC = pDrvCtrl->pAC;

	semTake(pTxPort->TxDesRingLock, WAIT_FOREVER);
	pTxd = pTxPort->pTxdRingHead;
	for (i=0; i<pAC->TxDescrPerRing; i++) {
		pTxd->TBControl &= ~BMU_OWN;
		pTxd = pTxd->pNextTxd;
	}
	/*FreeTxDescriptors(pAC, pTxPort); Assaf, implement later */
	semGive(pTxPort->TxDesRingLock);
} /* skGeClearTxRing */

/*****************************************************************************
 *
 *	SkDrvEvent - handle driver events
 *
 * Description:
 *	This function handles events from all modules directed to the driver
 *
 * Context:
 *	Is called under protection of slow path lock.
 *
 * Returns:
 *	0 if everything ok
 *	< 0  on error
 *	
 */
int SkDrvEvent(
	SK_AC     *pAC,    /* pointer to adapter context */
	SK_IOC     IoC,    /* IO control context         */
	SK_U32     Event,  /* event-id                   */
	SK_EVPARA  Param)  /* event-parameter            */
{

	/*SK_MBUF         *pRlmtMbuf;  VxWorks */ /* pointer to a rlmt-mbuf structure   */
	/*struct sk_buff  *pMsg;    VxWorks */    /* pointer to a message block         */
	SK_EVPARA        NewPara;     /* parameter for further events       */
	SK_BOOL          DualNet;
	SK_U32           Reason;
	int              FromPort;    /* the port from which we switch away */
	int              ToPort;      /* the port we switch to              */
	int              Stat;
	END_DEVICE	*pDrvCtrl = pAC->endDev;


	switch (Event) {
	case SK_DRV_ADAP_FAIL:
		SK_DBG_MSG(NULL, SK_DBGMOD_DRV, SK_DBGCAT_DRV_EVENT,
			("ADAPTER FAIL EVENT\n"));
		printf("%s: Adapter failed.\n", pDrvCtrl->end.devObject.name);
		SK_OUT32(pAC->IoBase, B0_IMSK, 0); /* disable interrupts */
		break;
	case SK_DRV_PORT_FAIL:
		FromPort = Param.Para32[0];
		SK_DBG_MSG(NULL, SK_DBGMOD_DRV, SK_DBGCAT_DRV_EVENT,
			("PORT FAIL EVENT, Port: %d\n", FromPort));
		if (FromPort == 0) {
			printf("%s: Port A failed.\n", pDrvCtrl->end.devObject.name);
		} else {
			printf("%s: Port B failed.\n", pDrvCtrl->end.devObject.name);
		}
		break;
	case SK_DRV_PORT_RESET:
		FromPort = Param.Para32[0];
		SK_DBG_MSG(NULL, SK_DBGMOD_DRV, SK_DBGCAT_DRV_EVENT,
			("PORT RESET EVENT, Port: %d ", FromPort));
		NewPara.Para64 = FromPort;
		
#ifdef MV_INCLUDE_PNMI
		SkPnmiEvent(pAC, IoC, SK_PNMI_EVT_XMAC_RESET, NewPara);
#endif
		semTake(pAC->TxPort[FromPort][TX_PRIO_LOW].TxDesRingLock, WAIT_FOREVER);
		if (CHIP_ID_YUKON_2(pAC)) {
			skY2PortStop(pAC, IoC, FromPort, SK_STOP_ALL, SK_HARD_RST);
		} else {
			SkGeStopPort(pAC, IoC, FromPort, SK_STOP_ALL, SK_HARD_RST);
		}      
		/* mark interface down */
        END_FLAGS_CLR (&pDrvCtrl->end, (IFF_RUNNING));    

		semGive(pAC->TxPort[FromPort][TX_PRIO_LOW].TxDesRingLock);
		
		if (!CHIP_ID_YUKON_2(pAC)) {
			skGeRecv(pDrvCtrl, &pAC->RxPort[FromPort]);
			skGeClearTxRing(pAC->endDev, &pAC->TxPort[FromPort][TX_PRIO_LOW]);
		}
		semTake(pAC->TxPort[FromPort][TX_PRIO_LOW].TxDesRingLock, WAIT_FOREVER);
            
		if (CHIP_ID_YUKON_2(pAC)) {
			skY2PortStart(pAC, IoC, FromPort);
		} else {
			/* tschilling: Handling of return value inserted. */
			if (SkGeInitPort(pAC, IoC, FromPort)) {
				if (FromPort == 0) {
					printf("%s: SkGeInitPort A failed.\n", pDrvCtrl->end.devObject.name);
				} else {
					printf("%s: SkGeInitPort B failed.\n", pDrvCtrl->end.devObject.name);
				}
			}
			SkAddrMcUpdate(pAC,IoC, FromPort);
			PortReInitBmu(pAC, FromPort);
			SkGePollTxD(pAC, IoC, FromPort, SK_TRUE);
			CLEAR_AND_START_RX(FromPort);
		}
		semGive(pAC->TxPort[FromPort][TX_PRIO_LOW].TxDesRingLock);
		break;
	case SK_DRV_NET_UP:
		FromPort = Param.Para32[0];
		SK_DBG_MSG(NULL, SK_DBGMOD_DRV, SK_DBGCAT_DRV_EVENT,
			("NET UP EVENT, Port: %d ", FromPort));
		SkAddrMcUpdate(pAC,IoC, FromPort); /* Mac update */
#ifdef SK_CHECK_LE
		printf("%s: network connection up using port %c\n",
			pDrvCtrl->end.devObject.name, 'A'+FromPort);
#endif
		if (DoPrintInterfaceChange) {
			printf("%s: network connection up using port %c\n",
				pDrvCtrl->end.devObject.name, 'A'+FromPort);

			/* tschilling: Values changed according to LinkSpeedUsed. */
			Stat = pAC->GIni.GP[FromPort].PLinkSpeedUsed;
			if (Stat == SK_LSPEED_STAT_10MBPS) {
				printf("    speed:           10\n");
			} else if (Stat == SK_LSPEED_STAT_100MBPS) {
				printf("    speed:           100\n");
			} else if (Stat == SK_LSPEED_STAT_1000MBPS) {
				printf("    speed:           1000\n");
			} else {
				printf("    speed:           unknown\n");
			}

			Stat = pAC->GIni.GP[FromPort].PLinkModeStatus;
			if ((Stat == SK_LMODE_STAT_AUTOHALF) ||
			    (Stat == SK_LMODE_STAT_AUTOFULL)) {
				printf("    autonegotiation: yes\n");
			} else {
				printf("    autonegotiation: no\n");
			}

			if ((Stat == SK_LMODE_STAT_AUTOHALF) ||
			    (Stat == SK_LMODE_STAT_HALF)) {
				printf("    duplex mode:     half\n");
			} else {
				printf("    duplex mode:     full\n");
			}

			Stat = pAC->GIni.GP[FromPort].PFlowCtrlStatus;
			if (Stat == SK_FLOW_STAT_REM_SEND ) {
				printf("    flowctrl:        remote send\n");
			} else if (Stat == SK_FLOW_STAT_LOC_SEND ) {
				printf("    flowctrl:        local send\n");
			} else if (Stat == SK_FLOW_STAT_SYMMETRIC ) {
				printf("    flowctrl:        symmetric\n");
			} else {
				printf("    flowctrl:        none\n");
			}
		
			/* tschilling: Check against CopperType now. */
			if ((pAC->GIni.GICopperType == SK_TRUE) &&
				(pAC->GIni.GP[FromPort].PLinkSpeedUsed ==
				SK_LSPEED_STAT_1000MBPS)) {
				Stat = pAC->GIni.GP[FromPort].PMSStatus;
				if (Stat == SK_MS_STAT_MASTER ) {
					printf("    role:            master\n");
				} else if (Stat == SK_MS_STAT_SLAVE ) {
					printf("    role:            slave\n");
				} else {
					printf("    role:            ???\n");
				}
			}

			/* Display interrupt moderation informations */
			if (pAC->DynIrqModInfo.IntModTypeSelect == C_INT_MOD_STATIC) {
				printf("    irq moderation:  static (%d ints/sec)\n",
					pAC->DynIrqModInfo.MaxModIntsPerSec);
			} else if (pAC->DynIrqModInfo.IntModTypeSelect == C_INT_MOD_DYNAMIC) {
				printf("    irq moderation:  dynamic (%d ints/sec)\n",
					pAC->DynIrqModInfo.MaxModIntsPerSec);
			} else {
				printf("    irq moderation:  disabled\n");
			}
			if (pAC->RxPort[FromPort].UseRxCsum) {
				printf("    rx-checksum:     enabled\n");
			} else {
				printf("    rx-checksum:     disabled\n");
			}
			
			if (pAC->LowLatency) {
				printf("    low latency:     enabled\n");
			}
		} else {
			DoPrintInterfaceChange = SK_TRUE;
		}
	
		if ((FromPort != pAC->ActivePort)&&(pAC->RlmtNets == 1)) {
			NewPara.Para32[0] = pAC->ActivePort;
			NewPara.Para32[1] = FromPort;
			SkEventQueue(pAC,SKGE_DRV,SK_DRV_SWITCH_INTERN,NewPara);
		}

		/* Inform the world that link protocol is up. */
		/* set the END flags, make driver ready to start */	
#ifdef INCLUDE_SGI_RFC_1213
		END_FLAGS_SET (&pDrvCtrl->end, (IFF_UP | IFF_RUNNING | IFF_NOTRAILERS | 
						IFF_BROADCAST | IFF_MULTICAST));   
#else
		END_FLAGS_SET (&pDrvCtrl->end, (IFF_UP | IFF_RUNNING | IFF_NOTRAILERS |
						IFF_BROADCAST | IFF_MULTICAST | END_MIB_2233));
#endif
		break;
	case SK_DRV_NET_DOWN:	
		Reason   = Param.Para32[0];
		FromPort = Param.Para32[1];
		SK_DBG_MSG(NULL, SK_DBGMOD_DRV, SK_DBGCAT_DRV_EVENT,
			("NET DOWN EVENT "));
		if (DoPrintInterfaceChange) {
			if (!(END_FLAGS_GET(&pDrvCtrl->end) & IFF_RUNNING)) {
				printf("%s: network connection down\n", pDrvCtrl->end.devObject.name);
			}
		} else {
			DoPrintInterfaceChange = SK_TRUE;
		}
		/* mark interface down */
        END_FLAGS_CLR (&pDrvCtrl->end, (IFF_RUNNING));    
		break;
	case SK_DRV_SWITCH_HARD:   /* FALL THRU */
	case SK_DRV_SWITCH_SOFT:   /* FALL THRU */
	case SK_DRV_SWITCH_INTERN: 
		FromPort = Param.Para32[0];
		ToPort   = Param.Para32[1];
		printf("%s: switching from port %c to port %c\n",
			pDrvCtrl->end.devObject.name, 'A'+FromPort, 'A'+ToPort);
		SK_DBG_MSG(NULL, SK_DBGMOD_DRV, SK_DBGCAT_DRV_EVENT,
			("PORT SWITCH EVENT, From: %d  To: %d (Pref %d) ",
			FromPort, ToPort, pAC->Rlmt.Net[0].PrefPort));
		NewPara.Para64 = FromPort;

#ifdef MV_INCLUDE_PNMI
		SkPnmiEvent(pAC, IoC, SK_PNMI_EVT_XMAC_RESET, NewPara);
#endif
		NewPara.Para64 = ToPort;

#ifdef MV_INCLUDE_PNMI
		SkPnmiEvent(pAC, IoC, SK_PNMI_EVT_XMAC_RESET, NewPara);
#endif
		semTake(pAC->TxPort[FromPort][TX_PRIO_LOW].TxDesRingLock, WAIT_FOREVER);
		semTake(pAC->TxPort[ToPort][TX_PRIO_LOW].TxDesRingLock, WAIT_FOREVER);
		if (CHIP_ID_YUKON_2(pAC)) {
			skY2PortStop(pAC, IoC, FromPort, SK_STOP_ALL, SK_HARD_RST);
			skY2PortStop(pAC, IoC, ToPort, SK_STOP_ALL, SK_HARD_RST);
		}
		else {
			SkGeStopPort(pAC, IoC, FromPort, SK_STOP_ALL, SK_SOFT_RST);
			SkGeStopPort(pAC, IoC, ToPort, SK_STOP_ALL, SK_SOFT_RST);
		}
		semGive(pAC->TxPort[ToPort][TX_PRIO_LOW].TxDesRingLock);
		semGive(pAC->TxPort[FromPort][TX_PRIO_LOW].TxDesRingLock);

		
		if (!CHIP_ID_YUKON_2(pAC)) {
			skGeRecv(pDrvCtrl, &pAC->RxPort[FromPort]); /* clears rx ring */
			skGeRecv(pDrvCtrl, &pAC->RxPort[ToPort]); /* clears rx ring */
			
			skGeClearTxRing(pAC->endDev, &pAC->TxPort[FromPort][TX_PRIO_LOW]);
			skGeClearTxRing(pAC->endDev, &pAC->TxPort[ToPort][TX_PRIO_LOW]);
		} 

		semTake(pAC->TxPort[FromPort][TX_PRIO_LOW].TxDesRingLock, WAIT_FOREVER);
		semTake(pAC->TxPort[ToPort][TX_PRIO_LOW].TxDesRingLock, WAIT_FOREVER);
		pAC->ActivePort = ToPort;
		
		/* tschilling: New common function with minimum size check. */
		DualNet = SK_FALSE;
		if (pAC->RlmtNets == 2) {
			DualNet = SK_TRUE;
		}
		
		if (SkGeInitAssignRamToQueues(pAC, pAC->ActivePort, DualNet)) {
			semGive(pAC->TxPort[ToPort][TX_PRIO_LOW].TxDesRingLock);
			semGive(pAC->TxPort[FromPort][TX_PRIO_LOW].TxDesRingLock);
			printf("SkGeInitAssignRamToQueues failed.\n");
			break;
		}

		if (!CHIP_ID_YUKON_2(pAC)) {
			/* tschilling: Handling of return values inserted. */
			if (SkGeInitPort(pAC, IoC, FromPort) ||
				SkGeInitPort(pAC, IoC, ToPort)) {
				printf("%s: SkGeInitPort failed.\n", pDrvCtrl->end.devObject.name);
			}
		}
		if (Event == SK_DRV_SWITCH_SOFT) {
			SkMacRxTxEnable(pAC, IoC, FromPort);
		}

		SkMacRxTxEnable(pAC, IoC, ToPort);
		SkAddrSwap(pAC, IoC, FromPort, ToPort);
		SkAddrMcUpdate(pAC, IoC, FromPort);
		SkAddrMcUpdate(pAC, IoC, ToPort);

		if (!CHIP_ID_YUKON_2(pAC)) {
			PortReInitBmu(pAC, FromPort);
			PortReInitBmu(pAC, ToPort);
			SkGePollTxD(pAC, IoC, FromPort, SK_TRUE);
			SkGePollTxD(pAC, IoC, ToPort, SK_TRUE);
			CLEAR_AND_START_RX(FromPort);
			CLEAR_AND_START_RX(ToPort);
		} else {
			skY2PortStart(pAC, IoC, FromPort);
			skY2PortStart(pAC, IoC, ToPort);
		}
		semGive(pAC->TxPort[ToPort][TX_PRIO_LOW].TxDesRingLock);
		semGive(pAC->TxPort[FromPort][TX_PRIO_LOW].TxDesRingLock);
		break;
	case SK_DRV_RLMT_SEND:	 /* SK_MBUF *pMb */
		break;
	case SK_DRV_TIMER:
		if (Param.Para32[0] == SK_DRV_MODERATION_TIMER) {
			/* check what IRQs are to be moderated */
			SkDimStartModerationTimer(pAC);
			SkDimModerate(pAC);
		} else {
			printf("Expiration of unknown timer\n");
		}
		break;
	default:
		break;
	}

	SK_DBG_MSG(NULL, SK_DBGMOD_DRV, SK_DBGCAT_DRV_EVENT, ("END EVENT "));
	return OK;
} /* SkDrvEvent */


/*****************************************************************************
 *
 *	SkErrorLog - log errors
 *
 * Description:
 *	This function logs errors to the system buffer and to the console
 *
 * Returns:
 *	0 if everything ok
 *	< 0  on error
 *	
 */
void SkErrorLog(SK_AC	*pAC, int	ErrClass, int	ErrNum, char	*pErrorMsg)
{
	char	ClassStr[80];
	END_DEVICE	*pDrvCtrl = pAC->endDev;

	switch (ErrClass) {
	case SK_ERRCL_OTHER:
		strcpy(ClassStr, "Other error");
		break;
	case SK_ERRCL_CONFIG:
		strcpy(ClassStr, "Configuration error");
		break;
	case SK_ERRCL_INIT:
		strcpy(ClassStr, "Initialization error");
		break;
	case SK_ERRCL_NORES:
		strcpy(ClassStr, "Out of resources error");
		break;
	case SK_ERRCL_SW:
		strcpy(ClassStr, "internal Software error");
		break;
	case SK_ERRCL_HW:
		strcpy(ClassStr, "Hardware failure");
		break;
	case SK_ERRCL_COMM:
		strcpy(ClassStr, "Communication error");
		break;
	}
	printf("%s: -- ERROR --\n        Class:  %s\n"
		"        Nr:  0x%x\n        Msg:  %s\n", pDrvCtrl->end.devObject.name,
		ClassStr, ErrNum, pErrorMsg);
} /* SkErrorLog */

/*****************************************************************************
 *
 *	SkDrvAllocRlmtMbuf - allocate an RLMT mbuf
 *
 * Description:
 *	This routine returns an RLMT mbuf or NULL. The RLMT Mbuf structure
 *	is embedded into a socket buff data area.
 *
 * Context:
 *	runtime
 *
 * Returns:
 *	NULL or pointer to Mbuf.
 */
SK_MBUF *SkDrvAllocRlmtMbuf(
SK_AC		*pAC,		/* pointer to adapter context */
SK_IOC		IoC,		/* the IO-context */
unsigned	BufferSize)	/* size of the requested buffer */
{
SK_MBUF		*pRlmtMbuf = NULL;	/* pointer to a new rlmt-mbuf structure */
	/* RLMT not supported */
	return (pRlmtMbuf);

} /* SkDrvAllocRlmtMbuf */


/*****************************************************************************
 *
 *	SkDrvFreeRlmtMbuf - free an RLMT mbuf
 *
 * Description:
 *	This routine frees one or more RLMT mbuf(s).
 *
 * Context:
 *	runtime
 *
 * Returns:
 *	Nothing
 */
void  SkDrvFreeRlmtMbuf(
SK_AC		*pAC,		/* pointer to adapter context */
SK_IOC		IoC,		/* the IO-context */
SK_MBUF		*pMbuf)		/* size of the requested buffer */
{
	/* RLMT not supported */
} /* SkDrvFreeRlmtMbuf */

/*****************************************************************************
 *
 *	SkOsGetTime - provide a time value
 *
 * Description:
 *	This routine provides a time value. The unit is 1/HZ (defined by Linux).
 *	It is not used for absolute time, but only for time differences.
 *
 *
 * Returns:
 *	Time value
 */
SK_U64 SkOsGetTime(SK_AC *pAC)
{
	return (SK_U64)tickGet();
} /* SkOsGetTime */


/******************************************************************************
 *
 * skGePortLinkIsUp() - Check if the link is up
 *
 * return:
 *	1	Link is up
 *	0	Link is down
 */
static int skGePortLinkIsUp(
SK_AC	*pAC,		/* Adapter Context */
SK_IOC	IoC,		/* I/O Context */
int		Port)		/* Port Index (MAC_1 + n) */
{
	SK_GEPORT	*pPrt;		/* GIni Port struct pointer */
	SK_BOOL		AutoNeg;	/* Is Auto-negotiation used ? */
	SK_U16		PhyStat;	/* Phy Status Register */

	pPrt = &pAC->GIni.GP[Port];

	if (pPrt->PLinkMode == SK_LMODE_HALF || pPrt->PLinkMode == SK_LMODE_FULL) {
		AutoNeg = SK_FALSE;
	}
	else {
		AutoNeg = SK_TRUE;
	}

	SkGmPhyRead(pAC, IoC, Port, PHY_MARV_STAT, &PhyStat);
	
	if (PhyStat & PHY_ST_LSYNC)
	{
		if (SK_AND_OK == SkMacAutoNegDone(pAC, IoC, Port)) {
#ifdef SK_CHECK_INIT
			printf("skGePortLinkIsUp => Link up!\n");
#endif
			return 1;
		}
		else
			return 0;
	}
	else
	{
		return 0;	
	}
}	/* SkGePortCheckUp */
