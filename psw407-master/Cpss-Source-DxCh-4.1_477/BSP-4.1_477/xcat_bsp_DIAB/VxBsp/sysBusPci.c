/*******************************************************************************
*                Copyright 2004, MARVELL SEMICONDUCTOR, LTD.                   *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL.                      *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
*                                           MARVELL COMPRISES MARVELL          *
* TECHNOLOGY GROUP LTD. (MTGL) AND ITS SUBSIDIARIES, MARVELL INTERNATIONAL     *
* LTD. (MIL), MARVELL TECHNOLOGY, INC. (MTI), MARVELL SEMICONDUCTOR, INC.      *
* (MSI), MARVELL ASIA PTE LTD. (MAPL), MARVELL JAPAN K.K. (MJKK), MARVELL      *
* ISRAEL LTD. (MSIL).                                                          *
*******************************************************************************/
/* sysBusPci.c - PCI Autoconfig support */

/* Copyright 1984-2003 Wind River Systems, Inc. */

/*
 * This file has been developed or significantly modified by the
 * MIPS Center of Excellence Dedicated Engineering Staff.
 * This notice is as per the MIPS Center of Excellence Master Partner
 * Agreement, do not remove this notice without checking first with
 * WR/Platforms MIPS Center of Excellence engineering management.
 */


/*
DESCRIPTION

This module contains the "non-generic" or "board specific" PCI
initialization code, including initializing the PCI Host bridge,
and initiating PCI auto-config.
*/

/* includes */
#ifdef INCLUDE_PCI

#include "vxWorks.h"
#include "logLib.h"
#include "taskLib.h"
#include "config.h"
#include "drv/pci/pciConfigLib.h"
#include "drv/pci/pciAutoConfigLib.h"
#include "pci-if\mvPciIfRegs.h"

#include "mvPciif.h"
#include "mvCpuIf.h"
#include "mvBoardEnvLib.h"
#include "mvCtrlEnvLib.h"
#include "vxPciIntCtrl.h"

#define MV_DEBUG 0
#if MV_DEBUG == 1
	#define DB(x) x
#else
	#define DB(x)
#endif

#define ILLEGAL_INTERFACE 255

/* forward declarations */

PCI_IF_MODE sysPciIfModeGet(MV_U32 pciIf);
STATUS sysPciConfigRead ( int bus, int dev, int func, int regNum,
			        int size, void* regData );
STATUS sysPciConfigWrite ( int bus, int dev, int func, int regNum,
				 int size, UINT32 regData );
UCHAR sysPciAutoConfigIntAsgn ( PCI_SYSTEM * pSys, PCI_LOC * pFunc,
    UCHAR intPin );
STATUS sysPciAutoConfigInclude ( PCI_SYSTEM *pSys, PCI_LOC *pciLoc,
    UINT devVend );
void sysPciAutoConfig (void);

LOCAL PCI_LOC * pciAutoListCreate1(PCI_AUTO_CONFIG_OPTS * pSystem,int *pListSize);
LOCAL STATUS pciAutoCfgFunc1(void *pCookie);
void pciAutoConfig1(PCI_SYSTEM * pSystem);
LOCAL STATUS mvPciHeaderShow(int bus, int device, int function, void *pArg);


UINT32 sysPciHostIfGet(int bus);

/* the Bus that we want to start the Configuration cycle from */
int startBus=0;
int bus2Pci[MAX_PCI_BUSSES];


/******************************************************************************
*
* sysPciAutoConfigInclude - Determine if function is to be autoConfigured
*
* This function is called with PCI bus, device, function, and vendor 
* information.  It returns an indication of whether or not the particular
* function should be included in the automatic configuration process.
* This capability is useful if it is desired that a particular function
* NOT be automatically configured.  Of course, if the device is not
* included in automatic configuration, it will be unusable unless the
* user's code made provisions to configure the function outside of the
* the automatic process.
*
* RETURNS: OK if function is to be included in automatic configuration,
* ERROR otherwise.
*/
 
STATUS sysPciAutoConfigInclude
    (
    PCI_SYSTEM *pSys,		/* input: AutoConfig system information */
    PCI_LOC *pciLoc,		/* input: PCI address of this function */
    UINT     devVend		/* input: Device/vendor ID number      */
    )
    {
    int retVal = OK;
	int pciIf;
    
	/* first : determine to which interface the bus of this device belongs */
    pciIf = sysPciHostIfGet(pciLoc->bus);
	DB(mvOsPrintf("sysPciAutoConfigInclude:  sysPciHostIfGet(bus=%x) return  pciIf=%d \n",pciLoc->bus , pciIf));
	if (ILLEGAL_INTERFACE == pciIf)
	{
		return ERROR;
	}
	if (mvPexIsPowerUp(pciIf) == MV_FALSE)
		return ERROR;


    /* In case pciIf is in device mode, do not configure it */
    if (PCI_IF_MODE_DEVICE == sysPciIfModeGet(pciIf)) 
    {
		return ERROR;
    }

    /* If it's the host bridge then exclude it */
    if ((pciLoc->bus == mvPciIfLocalBusNumGet(pciIf) ) && 
		(pciLoc->device ==  mvPciIfLocalDevNumGet(pciIf)))
	{
		return ERROR;
	}


    switch(devVend)
	{

	/* TODO - add any excluded devices by device/vendor ID here */

	default:
	    retVal = OK;
	    break;
	}

    return retVal;
    }

/*******************************************************************************
*
* sysPciAutoConfig - PCI autoConfig support routine
*
* This routine instantiates the PCI_SYSTEM structure needed to configure
* the system. This consists of assigning address ranges to each category
* of PCI system resource: Prefetchable and Non-Prefetchable 32-bit Memory, and
* 16- and 32-bit I/O. Global values for the Cache Line Size and Maximum
* Latency are also specified. Finally, the four supplemental routines for 
* device inclusion/exclusion, interrupt assignment, and pre- and
* post-enumeration bridge initialization are specified. 
*
* Note: Out of the IO address space we allocate 1/4 size to 16-bit IO and
* the rest (3/4 size) to 32-bit IO
*
* RETURNS: N/A
*/


void sysPciAutoConfig (void)
    {
    LOCAL PCI_SYSTEM sysParams;
	int pciIf, pciBus,i;
    MV_U32 retVal;
    UINT32 pciIoBase, pciMemBase, pciIoSize, pciMemSize, maxIf ;
    maxIf = mvCtrlPexMaxIfGet();
	pciBus = 0;
	for (pciIf=0 ; pciIf < (maxIf); pciIf++)
	{        
		for (i=pciBus;i<MAX_PCI_BUSSES;i++)
		{
			bus2Pci[i]=pciIf;
		}
		DB(mvOsPrintf("sysPciAutoConfig: pciIf=%d pciBus=0x%x maxIf=%d \n",pciIf, pciBus, maxIf));
		if (mvPexIsPowerUp(pciIf) == MV_FALSE)
		{
			continue;
		}
		retVal = mvCpuIfTargetWinSizeGet(PCI_MEM(pciIf, 0));
		if (0 == retVal)
		{
			mvOsPrintf("sysPciAutoConfig: pciIf=%d is received incorrect window size \n",pciIf);
			continue;
		}
		mvPciIfLocalBusNumSet(pciIf, pciBus);
		bus2Pci[pciBus] = pciIf;
        pciIoBase = mvCpuIfTargetWinBaseLowGet(PCI_IO(pciIf));
        pciIoSize = mvCpuIfTargetWinSizeGet(PCI_IO(pciIf));
        pciMemBase = mvCpuIfTargetWinBaseLowGet(PCI_MEM(pciIf, 0));
		pciMemSize = retVal;
		DB(mvOsPrintf("sysPciAutoConfig: pciIf=%d pciIoBase =0x%x, pciIoSize =0x%x \n ",pciIf,pciIoBase, pciIoSize));
	    DB(mvOsPrintf("                           pciMemBase=0x%x, pciMemSize=0x%x\n", pciMemBase, pciMemSize ));

		/* Initial available PCI memory addresses */
		sysParams.pciIo16 = pciIoBase;
		sysParams.pciIo16Size = pciIoSize / 2;
		sysParams.pciIo32 = sysParams.pciIo16;
		sysParams.pciIo32Size = pciIoSize - sysParams.pciIo16Size;
        sysParams.pciMem32 = 0;
		sysParams.pciMem32Size = 0;
		sysParams.pciMemIo32 = pciMemBase;
		sysParams.pciMemIo32Size = pciMemSize;
		sysParams.maxBus = mvPciIfLocalBusNumGet(pciIf)+10;
		sysParams.cacheSize = PCI_CACHE_LINE_SIZE;
		sysParams.maxLatency = PCI_LATENCY_TIMER;
		sysParams.autoIntRouting = FALSE;

		sysParams.includeRtn = sysPciAutoConfigInclude ;
		sysParams.intAssignRtn = sysPciAutoConfigIntAsgn;

		sysParams.bridgePreConfigInit = NULL;
		sysParams.bridgePostConfigInit = NULL;

		sysParams.pciRollcallRtn = NULL;

		startBus = mvPciIfLocalBusNumGet(pciIf);
		pciAutoConfig1(&sysParams);
		DB(mvOsPrintf("sysPciAutoConfig: return from pciAutoConfig1, sysParams.maxBus = 0x%x\n", sysParams.maxBus));
		pciBus = sysParams.maxBus+1;
	}
	for (i=pciBus;i<MAX_PCI_BUSSES;i++)
	{
		bus2Pci[i]=ILLEGAL_INTERFACE;
	}

	/* Connect PCI IRQ routines */
	pciIntLibInit();

    return;
}

/******************************************************************************
*
* sysPciBusNumGet - 
*
* Get the host PCI Interface number according to the bus number
*
* RETURNS: N/A
*
* NOMANUAL
*/

UINT32 sysPciHostIfGet(int bus)
{          
	if (bus >= MAX_PCI_BUSSES)
	{
		DB(mvOsPrintf("sysPciHostIfGet: Illegal bus number=%d \n",bus));
		return ILLEGAL_INTERFACE;
	}
	DB(if (bus2Pci[bus] == ILLEGAL_INTERFACE) \
		mvOsPrintf("sysPciHostIfGet: return ILLEGAL_INTERFACE for bus=%d \n",bus));
	return bus2Pci[bus];
}


/*******************************************************************************
* sysPciIfModeGet - Get PCI bus mode.
*
* DESCRIPTION:
*		This routine get the PCI mode as defined by mvSysHwConfig.h file.
*
* INPUT:
*       pciIf - PCI interface number
*
* OUTPUT:
*		None.
*
* RETURN:
*		PCI_IF_MODE (host/device). -1 if the parameter is invalid.
*
*******************************************************************************/
PCI_IF_MODE sysPciIfModeGet(MV_U32 pciIf)
{
    if (pciIf > mvCtrlPexMaxIfGet())
        return -1;

    return (PCI_IF_MODE_HOST);
}

/***************************************************************************
*
* sysPciConfigRead - read data from a device PCI configuration register
*
* Read from a device PCI configuration register.
*
* RETURNS: OK on success, ERROR on failure
*/

STATUS sysPciConfigRead
    (
    int bus,
    int dev,
    int func,
    int regNum,
    int size,
    void* regData
    )
{

	volatile UINT32 	 pciData,offset=0;
	int 	 key;
    STATUS   status = OK;
	int pciIf;
    
	/* first: determine to which interface the bus of this device belongs */

    pciIf = sysPciHostIfGet(bus);
	if (ILLEGAL_INTERFACE == pciIf)
	{
		return ERROR;
	}
	if (mvPexIsPowerUp(pciIf) == MV_FALSE)
	{
		/* DB(mvOsPrintf("sysPciConfigRead: pciIf=%d is shutdown \n",pciIf)); */
		return ERROR;
	}

	key = intLock();

	pciData = mvPciIfConfigRead (pciIf,bus,dev,func,(regNum&0xfc));

	offset = regNum & 0x3;

    switch (size)
        {
	case 1:
		pciData >>= (offset*8);
		pciData &= 0xff;
        *(UINT8*)regData = (UINT8)(pciData);
		break;
	case 2:
		pciData >>= (offset*8);
		pciData &= 0xffff;
		*(UINT16*)regData = (UINT16)(pciData);
		break;
    case 4:
       *(UINT32*)regData = pciData;

       break;
	default:
	    status = ERROR;
	    break;
        }

    intUnlock(key);

    return (status);
}


/***************************************************************************
*
* sysPciConfigWrite - write data to a device PCI configuration register
*
* Write to a device PCI configuration register.
*
* RETURNS: OK on success, ERROR on failure
*/

STATUS sysPciConfigWrite
    (
    int bus,
    int dev,
    int func,
    int regNum,
    int size,
    UINT32 regData
    )

{
	int 	 key;
	STATUS   status = OK;
	volatile UINT32 	 pciData=0,offset;
	int pciIf;

	/* first : determine to which interface the bus of this device belongs */
    pciIf = sysPciHostIfGet(bus);
	if (ILLEGAL_INTERFACE == pciIf)
	{
		return ERROR;
	}
	if (mvPexIsPowerUp(pciIf) == MV_FALSE)
	{
		DB(mvOsPrintf("sysPciConfigWrite: pciIf=%d is shutdown \n",pciIf));
		return ERROR;
	}

	key = intLock();

    /* check argument validity */

    if ((regNum % size) != 0)
	{
	intUnlock (key);
	return (ERROR);         /* unaligned configuration write */
	}
    
	pciData = mvPciIfConfigRead (pciIf,bus,dev,func,(regNum&0xfc));
	
	offset = regNum & 0x3;

	switch (size)
	{
	case 1:
		regData &= 0xff;
		regData <<= (offset*8);
		pciData &= ~(0xff << (offset*8));
		pciData |= regData;

		break;
	case 2:
		regData &= 0xffff;
		regData <<= (offset*8);
		pciData &= ~(0xffff << (offset*8));
		pciData |= regData;

		break;
	case 4:
		pciData = regData;

	   break;
	default:
		status = ERROR;
		break;
	}


	status = mvPciIfConfigWrite (pciIf,bus,dev,func,(regNum&0xfc),pciData);

	intUnlock(key);
	
	return (status);
}

/******************************************************************************
*
* sysPciAutoConfigIntAssign - Assign the "interrupt line" value
*
* RETURNS: "interrupt line" value.
*
*/

UCHAR sysPciAutoConfigIntAsgn
    ( 
    PCI_SYSTEM * pSys,		/* input: AutoConfig system information */
    PCI_LOC * pFunc,
    UCHAR intPin 		/* input: interrupt pin number. A=1, B=2, C=3, D=4 */
    )
    {
    UCHAR irqValue = 0xff;    /* Calculated value                */
	int pciIf;

	/* first : determine to which interface the bus of this device belongs 	*/
    pciIf = sysPciHostIfGet(pFunc->bus);

	DB(mvOsPrintf("sysPciAutoConfigIntAsgn: sysPciHostIfGet(bus=%x) return  pciIf=%d device=0x%x,intPin=%d \n",
				pFunc->bus , pciIf, pFunc->device, intPin));
	if (ILLEGAL_INTERFACE == pciIf)
	{
		return ERROR;
	}

        irqValue = INT_LVL_PEX(pciIf, intPin); 
	DB(mvOsPrintf("sysPciAutoConfigIntAsgn: INT_LVL_PEX return  irqValue=%d pexIf=%d \n",irqValue, pciIf));
    return (irqValue);    
}

/******************************************************************************
*
* pciShow - Show Pci Devices
*
*
*/


void pciShow()
{
	UINT pciIf;

	for (pciIf=0 ; pciIf < (mvCtrlPexMaxIfGet()); pciIf++ )
	{        
        /* PCI type (PEX/PCI) auto detection */	
        {
            mvOsPrintf("------------------------------------------------------- \n");
		    mvOsPrintf("------------------PEX Interface %d---------------------- \n",pciIf);
		    mvOsPrintf("------------------------------------------------------- \n\n");
			if (mvPexIsPowerUp(pciIf) == MV_FALSE)
				continue;
        }
		pciConfigForeachFunc(mvPciIfLocalBusNumGet(pciIf),TRUE,mvPciHeaderShow,NULL);

    }
}

/******************************************************************************
*
* mvPciHeaderShow - Warp pciHeaderShow to fit to PCI_FOREACH_FUNC function type
*					that pciConfigForeachFunc requires.
*
*/
LOCAL STATUS mvPciHeaderShow(int bus, int device, int function, void *pArg)
{
	pciConfigFuncShow(bus, device, function, pArg);
	pciHeaderShow(bus, device, function);
	printf("\n");

	return OK;
}

/*******************************************************************************
*  - I M P O R T A N T     N O T E - 
*
*	Because VxWorks PCI stack start the configuration cycle always from 
*   bus number 0 !this prevent us from start a configeration cycle on a PCI 
*   Interface host that has a bus not equal to 0 , and because some Marvell 
*   controllers have more than a one PCI host, we configure those hosts to be
*   in different busses ! and so we have oviredden the following functions;
*
*   pciAutoListCreate,pciAutoCfgFunc,pciAutoConfig
* 
*  with the following functions 
*
*   pciAutoListCreate1,pciAutoCfgFunc1,pciAutoConfig1
* 
*   the only difference is that pciAutoListCreate1 function start its config
*   cycle from bus startBus than starting it from 0 .
*/


/******************************************************************************
*
* pciAutoListCreate - probe for all functions and make a PCI probe list.
*
*/


LOCAL PCI_LOC * pciAutoListCreate1
    (
    PCI_AUTO_CONFIG_OPTS * pSystem,     /* cookie returned by pciAutoConfigLibInit() */
    int *pListSize			/* size of the PCI_LOC list */
    )
    {
    PCI_LOC  pciLoc;		/* PCI bus/device/function structure */
    PCI_LOC *pPciList;
    PCI_LOC *pRetPciList;
    UCHAR    busAttr;

   /* Initialize the list pointer in preparation for probing */

#if defined(PCI_AUTO_STATIC_LIST)
    pPciList = pciAutoLocalFuncList;
    pRetPciList = pPciList;
#else
    pPciList = malloc(sizeof(PCI_LOC) *  PCI_AUTO_MAX_FUNCTIONS);
    if (pPciList == NULL)
	{
	return NULL;
	}
    pRetPciList = pPciList;
#endif

    lastPciListSize = 0;
    *pListSize = 0;

    /* Begin the scanning process at [startBus,0,0] */

    pciLoc.bus = (UINT8)startBus;
    pciLoc.device = (UINT8)0;
    pciLoc.function = (UINT8)0;

    /*
     * Note that the host bridge is assumed to support prefetchable memory
     * (PCI_AUTO_ATTR_BUS_PREFETCH) and only assumed to support 32-bit I/O 
     * addressing (PCI_AUTO_ATTR_BUS_4GB_IO) if pciIo32Size is non-zero.
     */

		
    if (pSystem->pciIo32Size == 0)
        busAttr = PCI_AUTO_ATTR_BUS_PREFETCH;
    else
        busAttr = PCI_AUTO_ATTR_BUS_4GB_IO | PCI_AUTO_ATTR_BUS_PREFETCH;

	DB(mvOsPrintf("call pciAutoDevProbe(pSystem=0x%x, pciLoc.bus=0x%x,0, busAttr=0x%x, &pPciList=0x%x, *pListSize=0x%x)\n",
								pSystem, pciLoc.bus,busAttr, &pPciList, *pListSize));

    pciMaxBus = pciAutoDevProbe (pSystem, pciLoc.bus, (UCHAR)0,
                                 busAttr, &pPciList, pListSize);
 
	DB(mvOsPrintf("return from pciAutoDevProbe(pSystem=0x%x, pciLoc.bus=0x%x,0, busAttr=0x%x, &pPciList=0x%x, *pListSize=0x%x)\n",
								pSystem, pciLoc.bus,busAttr, &pPciList, *pListSize));

	DB(mvOsPrintf("pciAutoListCreate1: pciMaxBus = 0x%x\n", pciMaxBus));

    pSystem->maxBus = pciMaxBus;

    return (pRetPciList);
    }


/****************************************************************
*
* pciAutoCfgFunc - the actual guts of pciAutoCfg().
*/

LOCAL STATUS pciAutoCfgFunc1
    (
    void *pCookie		/* cookie returned by pciAutoConfigLibInit() */
    )
    {
    PCI_AUTO_CONFIG_OPTS *	pSystem; /* named for backward compatibility */

    PCI_LOC* pPciList;		/* Pointer to PCI include list	*/
    int listSize;		/* Size of PCI include list	*/
    BOOL rollcallSuccess;	/* has pciRollcallRtn() succeeded? */

    /* Input parameter sanity checking */

    if (pCookie == NULL)
	{
	errnoSet(EINVAL);
	return(ERROR);
	}

    pSystem = (PCI_AUTO_CONFIG_OPTS *)pCookie;

    /*
     * If a roll-call routine has been configured, call the roll-call
     * routine repeatedly until it returns TRUE.  A return value
     * of TRUE indicates that either (1) the specified number and
     * type of devices named in the roll call list have been found
     * during PCI bus enumeration or (2) the timeout has expired
     * without finding all of the specified number and type of
     * devices.  In either case, we will assume that all of the PCI
     * devices which are going to appear on the busses have appeared
     * and we can proceed with PCI bus configuration.
     */

    if (pSystem->pciRollcallRtn != NULL)
	{

		DB(mvOsPrintf("pciAutoCfgFunc1: pSystem->pciRollcallRtn != NULL\n"));


	rollcallSuccess = FALSE;
		while ( ! rollcallSuccess )
		{

			/*
			 * Probe all PCI busses dynamically creating a function list
			 * of all functions found.  Excluded devices are skipped over.
			 */


			/* pPciList = */ pciAutoListCreate1 (pSystem, &listSize);


			/* Perform roll call function, if we pass, exit the loop */

			if ( (*pSystem->pciRollcallRtn)() == OK )
				rollcallSuccess = TRUE;

#ifdef PCI_AUTO_RECLAIM_LIST
			free(pPciList);
#endif /* PCI_AUTO_RECLAIM_LIST */

			if ( rollcallSuccess == TRUE )
				break;
		}
	}


	/*
	 * Probe all PCI busses dynamically creating a function list
	 * of all functions found.  Excluded devices are skipped over.
	 */

    pPciList = pciAutoListCreate1 (pSystem, &listSize);
    pciAutoFuncConfigAll (pSystem, pPciList, listSize);

    lastPciListSize = listSize;
    pLastPciList = pPciList;

    /* If the function list is malloc'ed at runtime, then release it */

#if defined(PCI_AUTO_RECLAIM_LIST)
#   if defined(PCI_AUTO_STATIC_LIST)
#       error "Can't do PCI_AUTO_RECLAIM_LIST with PCI_AUTO_STATIC_LIST"
#   endif
    free(pPciList);

    lastPciListSize = 0;
    pLastPciList = NULL;
#endif

    return(OK);
    }


/******************************************************************************
*
* pciAutoConfig1 -
*/
void pciAutoConfig1
    (
    PCI_SYSTEM * pSystem	/* PCI system to configure */
    )
    {
    PCI_AUTO_CONFIG_OPTS *pCookie;
    
    pCookie = pciAutoConfigLibInit(NULL);
/*	DB(mvOsPrintf("pciAutoConfig1: 1, pCookie=0x%x\n",pCookie));  */
    pciAutoCfgCtl(pCookie, PCI_PSYSTEM_STRUCT_COPY, (void *)pSystem);
    pciAutoCfgFunc1(pCookie);

    pciAutoConfigCopyback(pCookie, pSystem);
    }


#endif

