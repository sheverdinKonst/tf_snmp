/*******************************************************************************
*                Copyright 2004, MARVELL SEMICONDUCTOR, LTD.                   *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL.                      *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
*                                                                              *
* MARVELL COMPRISES MARVELL TECHNOLOGY GROUP LTD. (MTGL) AND ITS SUBSIDIARIES, *
* MARVELL INTERNATIONAL LTD. (MIL), MARVELL TECHNOLOGY, INC. (MTI), MARVELL    *
* SEMICONDUCTOR, INC. (MSI), MARVELL ASIA PTE LTD. (MAPL), MARVELL JAPAN K.K.  *
* (MJKK), MARVELL ISRAEL LTD. (MSIL).                                          *
*******************************************************************************/
#include "vxWorks.h"
#include "taskLib.h"
#include "sysLib.h"
#include "config.h"
#include "vmLib.h"
#include "intLib.h"
#include "wdLib.h"
#include "drv/pci/pciIntLib.h"
#include "drv/pci/pciConfigLib.h"
#include "mvPciIf.h"
#include "mvCpuIf.h"
#include "mvOs.h"
#include "sysPciIntCtrl.h"

#include "mrvlSataLib.h"

#undef _DEBUG_ 
#ifdef _DEBUG_
    #define DB(x)	x
#else
#define DB(x)
#endif	

#if  defined(MV78XX0)  
#define  MV_DEV_ID   	MV_78XX0_DEV_ID
#elif  defined(MV88F6XXX)  
#define  MV_DEV_ID      MV_6281_DEV_ID
#endif

/* include PCI Library */

#ifndef INCLUDE_PCI
#define INCLUDE_PCI
#endif /* INCLUDE_PCI */

/* PCI configuration type */
#ifndef PCI_CFG_TYPE
#define PCI_CFG_TYPE            PCI_CFG_NONE
#endif

/* Global variables */
int mvLogInitialized = 0;
/* externs */

IMPORT void     sysUsDelay (UINT32);
IMPORT void mvIalInitLog(void);
IMPORT void initMvSataAdapter(SATA_INFO  *pSataInfo);
IMPORT int mrvlDmaMemInitChannels(SATA_INFO *sc);
IMPORT int mrvlPrdPoolAllocate(SATA_INFO *sc);
IMPORT void mrvlInitCibs(SATA_INFO *sc);
IMPORT int asyncStartTimerFunction(void *arg);

typedef struct mvSataResource        /* SATA_RESOURCE */

    {

    UINT32 memBaseLow;            /* Base Address LOW */
    char   irq;                   /* Interrupt Request Level */
    unsigned short	vendorId;	  /* adapter vendor ID */
    unsigned short	deviceId;	  /* adapter device ID */
	int    pciBus;                /* PCI Bus number */
    int    pciDevice;             /* PCI Device number */
    int    pciFunc;               /* PCI Function number */
    MV_BOOLEAN  integratedSataDetected;  /* None PCI SATA */
    } MV_SATA_RESOURCE;

typedef struct _mvSataDev
{
    unsigned short	vendorId;	/* adapter vendor ID */
    unsigned short	deviceId;	/* adapter device ID */
	int				instance;	/* Number of times device found in system */
}MV_SATA_DEV;

/* locals */

LOCAL UINT32 sysMvSataLocalToPciBusAdrs (int unit, UINT32 adrs);
LOCAL UINT32 sysMvSataPciBusToLocalAdrs (int unit, UINT32 adrs);
LOCAL UINT32 mvSataUnits = 0;     /* number of mvSatas we found */

LOCAL MV_SATA_RESOURCE mvSataResources [] =
    {
    {0,0,-1, -1, -1, -1, -1, MV_FALSE},		/* 1 */ 
    {0,0,-1, -1, -1, -1, -1, MV_FALSE},     /* 2 */ 
    {0,0,-1, -1, -1, -1, -1, MV_FALSE},     /* 3 */ 
    {0,0,-1, -1, -1, -1, -1, MV_FALSE},     /* 4 */ 
    {0,0,-1, -1, -1, -1, -1, MV_FALSE},     /* 5 */ 
    {0,0,-1, -1, -1, -1, -1, MV_FALSE},     /* 6 */ 
    {0,0,-1, -1, -1, -1, -1, MV_FALSE},     /* 7 */ 
    {0,0,-1, -1, -1, -1, -1, MV_FALSE},     /* 8 */ 
     };

/* Driver supported PCI devices */

LOCAL MV_SATA_DEV mvSataPciDevTbl[] = {
	{ MV_SATA_VENDOR_ID, MV_SATA_DEVICE_ID_5080, 0},
	{ MV_SATA_VENDOR_ID, MV_SATA_DEVICE_ID_5081, 0},
	{ MV_SATA_VENDOR_ID, MV_SATA_DEVICE_ID_5040, 0},
	{ MV_SATA_VENDOR_ID, MV_SATA_DEVICE_ID_5041, 0},
	{ MV_SATA_VENDOR_ID, MV_SATA_DEVICE_ID_6041, 0},
	{ MV_SATA_VENDOR_ID, MV_SATA_DEVICE_ID_6081, 0},
	{ MV_SATA_VENDOR_ID, MV_SATA_DEVICE_ID_6042, 0},
	{ MV_SATA_VENDOR_ID, MV_SATA_DEVICE_ID_7042, 0}
};

/* globals */

/* forward declarations */

LOCAL int       sysMvSataIntEnable  (int unit);
LOCAL int       sysMvSataIntDisable (int unit);
LOCAL int       sysMvSataIntAck     (int unit);

/*****************************************************************************
*
* sysMvSataPciInit - Initialize and get the PCI configuration for Mv Sata Chips
*
* This routine finds out PCI device, and maps its memory and IO address.
*
* RETURNS: N/A
*/

UINT32 sysMvSataPciProbe (void)
{
	MV_SATA_RESOURCE *pReso;	  /* chip resources */
	int typeInx;				 /* adapter device type index*/
	int pciBus;					 /* PCI bus number */
	int pciDevice;				 /* PCI device number */
	int pciFunc;				 /* PCI function number */
	int unit;					 /* unit number */
	UINT32 bar0;				 /* PCI BAR_0 */

	UINT32 memBaseLow;			 /* mem base low */
	char   irq;					 /* irq number */
	BOOL   devFound;
	MV_BOOLEAN integratedSataDetected = MV_FALSE;
#if defined(MV_INCLUDE_INTEG_SATA) 
	MV_BOOLEAN integratedSataInitialized = MV_FALSE;
#endif	
	for (unit = 0; unit < NELEMENTS(mvSataResources); unit++)
	{
		devFound = FALSE;
		integratedSataDetected = MV_FALSE;
#if defined(MV_INCLUDE_INTEG_SATA) 
		if (0 == unit)
		{
			if (integratedSataInitialized == MV_FALSE)
			{
				integratedSataDetected = MV_TRUE;

				/* mvSataWinInit(); */

				mvOsPrintf("Integrated Sata device found\n");
				devFound = TRUE;
			}
		}
#endif

		for (typeInx = 0; (devFound == MV_FALSE)&&(typeInx < NELEMENTS(mvSataPciDevTbl)); typeInx++)
		{
			/* Detect possible adaptors */          
			if (pciFindDevice (mvSataPciDevTbl[typeInx].vendorId, 
							   mvSataPciDevTbl[typeInx].deviceId, 
							   mvSataPciDevTbl[typeInx].instance,
							   &pciBus, &pciDevice, &pciFunc) == OK)
			{
				DB(mvOsPrintf("pciFindDevice: find device typeInx=%d, ID=%x, instance=0x%x, vendorId=0x%x \n",
						   typeInx,
						   mvSataPciDevTbl[typeInx].instance,  
						   mvSataPciDevTbl[typeInx].deviceId,  
						   mvSataPciDevTbl[typeInx].vendorId));
				DB(mvOsPrintf("             : pciBus=%x, pciDevice=%x, pciFunc=%x \n",pciBus, pciDevice, pciFunc));

				mvSataPciDevTbl[typeInx].instance++;
				devFound = TRUE;
				break;
			}
		}

		if (devFound == FALSE)	 /* No more devices */
			break;

		pReso = &mvSataResources [unit];

#if defined(MV_INCLUDE_INTEG_SATA) 
		if (integratedSataDetected == MV_TRUE)
		{
			pReso->vendorId   = MV_SATA_VENDOR_ID;
			pReso->deviceId   = mvCtrlModelGet();
			pReso->pciBus     = 0;
			pReso->pciDevice  = 0;
			pReso->pciFunc    = 0;
			pReso->memBaseLow = INTER_REGS_BASE + SATA_REG_BASE - 0x20000;
			pReso->irq        = INT_LVL_SATA_CTRL;
			pReso->integratedSataDetected = MV_TRUE;
			integratedSataInitialized 	  = MV_TRUE;
		} 
		else
#endif
		{
			pReso->vendorId  = mvSataPciDevTbl[typeInx].vendorId;
			pReso->deviceId  = mvSataPciDevTbl[typeInx].deviceId;
			pReso->pciBus    = pciBus;
			pReso->pciDevice = pciDevice;
			pReso->pciFunc   = pciFunc;
			pciConfigInLong (pReso->pciBus, pReso->pciDevice, pReso->pciFunc,
							 PCI_CFG_BASE_ADDRESS_0, &bar0);

			/* read back memory base address and IO base address */
			pciConfigInLong (pReso->pciBus, pReso->pciDevice, pReso->pciFunc,
							 PCI_CFG_BASE_ADDRESS_0, &memBaseLow);

			pciConfigInByte (pReso->pciBus, pReso->pciDevice, pReso->pciFunc,
							 PCI_CFG_DEV_INT_LINE, &irq);

			DB(mvOsPrintf("sysMvSataPciProbe: find device bus=%d, pciDevice=%d, pciFunc=%d, memBaseLow=0x%x, irq=0x%x\n",
						  pReso->pciBus, pReso->pciDevice, pReso->pciFunc, memBaseLow, irq));
			/* over write the resource table with read value */
			memBaseLow &= PCI_MEMBASE_MASK;
			/* over write the resource table with read value */
			pReso->memBaseLow   = memBaseLow;
			pReso->irq          = irq;
			/* enable mapped memory and IO addresses */
			pciConfigOutWord (pReso->pciBus, pReso->pciDevice, pReso->pciFunc,
							  PCI_CFG_COMMAND, PCI_CMD_IO_ENABLE |
							  PCI_CMD_MEM_ENABLE | PCI_CMD_MASTER_ENABLE);

		}
		mvSataUnits++;
	}
	DB(mvOsPrintf ("sysMvSataPciProbe return mvSataUnits=%d \n",mvSataUnits));
	return mvSataUnits;
}

/*****************************************************************************
*
* sysMvSataBoardInit - Adaptor initialization for Marvell Sata chip
*
* This routine is expected to perform any adapter-specific or target-specific
* initialization that must be done prior to initializing the MvSata chip.
*
* RETURNS: OK or ERROR
*/

STATUS sysMvSataDeviceInit
    (
    int    	unit,                /* unit number */
	SATA_INFO  *pSataInfo       /* board information for the SGI driver */
    )
    {
	int typeInx;			 	 /* adapter device type index*/
    MV_SATA_RESOURCE *pReso = &mvSataResources [unit];
	MV_SATA_ADAPTER  *pMvSataAdapter = &pSataInfo->mvSataAdapter;

	DB(mvOsPrintf(" sysMvSataDeviceInit: Unit=%d , pSataInfo=0x%x\n",unit,pSataInfo));
    /* sanity check */

    if (unit >= mvSataUnits)
	{
        return (ERROR);
		mvOsPrintf(" sysMvSataDeviceInit: Sata Device unit=%d >  total unit=%d\n",unit,mvSataUnits);
	}

#if defined(MV_INCLUDE_INTEG_SATA) 
	if (pReso->integratedSataDetected == MV_FALSE)
#endif
	{
		for (typeInx = 0; typeInx < NELEMENTS(mvSataPciDevTbl); typeInx++)
		{      
			if ((pReso->vendorId == mvSataPciDevTbl[typeInx].vendorId) &&
				(pReso->deviceId == mvSataPciDevTbl[typeInx].deviceId))
				break;
		}

		if (typeInx == NELEMENTS(mvSataPciDevTbl))
		{
			mvOsPrintf(" sysMvSataDeviceInit: typeInx=%d == NELEMENTS(mvSataPciDevTbl)=%d\n",
					   typeInx,NELEMENTS(mvSataPciDevTbl));
			return ERROR;
		}
	}

	/* Initialize logging */
	if (mvLogInitialized == 0)
	  {
		mvIalInitLog();
		mvLogInitialized = 1;
	  }

    pSataInfo->mutualSem = semMCreate (SEM_Q_PRIORITY|SEM_INVERSION_SAFE);
	DB(mvOsPrintf(" sysMvSataDeviceInit: pSataInfo->mutualSem =0x%d \n",pSataInfo->mutualSem));
    /* initializes the board information structure */    
    pSataInfo->pciDev.vendorId    = pReso->vendorId;
    pSataInfo->pciDev.deviceId    = pReso->deviceId;
	pSataInfo->pciDev.pciBusNo    = pReso->pciBus;
    pSataInfo->pciDev.pciDeviceNo = pReso->pciDevice;
    pSataInfo->pciDev.pciFuncNo   = pReso->pciFunc;

#if defined(MV_INCLUDE_INTEG_SATA) 
	if (pReso->integratedSataDetected == MV_FALSE)
#endif
	{
		pciConfigInByte (pReso->pciBus, pReso->pciDevice, pReso->pciFunc,
                          PCI_CFG_REVISION, &pSataInfo->pciDev.chipRev);
		pSataInfo->regBase    	 = PCI_MEMIO2LOCAL(pReso->memBaseLow);
		pSataInfo->intConnect    = (FUNCPTR) pciIntConnect;
		pSataInfo->intDisConnect = (FUNCPTR) pciIntDisconnect;
		DB(mvOsPrintf(" sysMvSataDeviceInit: pciConfigInByte-> pSataInfo->pciDev.chipRev =0x%d \n",pSataInfo->pciDev.chipRev));
	}
#if defined(MV_INCLUDE_INTEG_SATA) 
	else
	{
		pSataInfo->regBase    	 = pReso->memBaseLow;
		pSataInfo->intConnect    = (FUNCPTR) intConnect; 
		pSataInfo->intDisConnect = NULL;
	}

#endif
	pSataInfo->unit   		= unit;
    pSataInfo->vector        = pReso->irq;
    pSataInfo->intEnable     = sysMvSataIntEnable;
    pSataInfo->intDisable    = sysMvSataIntDisable;
    pSataInfo->intAck        = sysMvSataIntAck;
    pSataInfo->sysLocalToBus = sysMvSataLocalToPciBusAdrs; 
    pSataInfo->sysBusToLocal = sysMvSataPciBusToLocalAdrs; 
    /* specify the interrupt connect/disconnect routines to be used */
	/* initialize the Core Driver Adapter data structure*/

	initMvSataAdapter(pSataInfo);

	if (mvSataInitAdapter(pMvSataAdapter) == MV_FALSE)
	{
		mvOsPrintf("[%d]: core failed to initialize the adapter\n",
		  pMvSataAdapter->adapterId);

		 /*TODO: release the adapter resources*/
		return -1;
	}
	if (mrvlDmaMemInitChannels(pSataInfo))
	{
		mvOsPrintf("mrvlDmaMemInitChannels faild\n");
        return -1;
    }
	if (mrvlPrdPoolAllocate(pSataInfo))
	{
		mvOsPrintf("mrvlPrdPoolAllocate faild\n");
		return -1;
	}
	mrvlInitCibs(pSataInfo);

	mvSataScsiInitAdapterExt(&pSataInfo->ataScsiAdapterExt,
				 pMvSataAdapter);

	pSataInfo->ataScsiAdapterExt.UAMask = 0;

	if (MV_FALSE == mvAdapterStartInitialization(pMvSataAdapter,
												  &pSataInfo->ialCommonExt,
												  &pSataInfo->ataScsiAdapterExt))
	{
		printf("[%d]: mvAdapterStartInitialization"
			  " Failed\n", pMvSataAdapter->adapterId);
        return -1;
	}
/*    scsiDebug=1;*/
	/*mrvl_attach(sc);*/
#ifdef MV_SATA_TIMER_AS_TASK
	taskSpawn("sataTimer", MV_SATA_INTR_TASK_PRIO - 1, 0, 8*4000,
					(FUNCPTR)asyncStartTimerFunction, (int)pSataInfo, 0, 0, 0, 0, 0,
					0, 0, 0, 0);
#else
	if ((pSataInfo->sataWatchDogId = wdCreate()) == NULL)
				return (ERROR);
	
	if (wdStart (pSataInfo->sataWatchDogId, sysClkRateGet()/2 , asyncStartTimerFunction,
				(int)pSataInfo) == ERROR)

    return (ERROR);
#endif

	/* enable adaptor interrupt */
#if defined(MV_INCLUDE_INTEG_SATA) 
	if (pReso->integratedSataDetected == MV_TRUE)
	{
		DB(mvOsPrintf ("sysMvSataDeviceInit Enable interrupt %d \n",pReso->irq));
		intEnable(pReso->irq);
	}
	else
#endif
	{
		DB(mvOsPrintf ("sysMvSataDeviceInit go to sysPciIntEnable irq= %d \n",pReso->irq));
		sysPciIntEnable(pReso->irq);
	}
    return (OK);
}
/*****************************************************************************
*
* sysMvSataIntAck - acknowledge an interrupt
*
*
* RETURNS: OK, or ERROR if the interrupt could not be acknowledged.
*/

LOCAL STATUS sysMvSataIntAck
(
    int    unit        /* unit number */
)
{
    return (OK);
}
/******************************************************************************
*
* sysMvSataLocalToPciBusAdrs - convert a local address to a bus address
*
* This routine returns a PCIbus address for the LOCAL bus address.
*
*/

LOCAL UINT32 sysMvSataLocalToPciBusAdrs
    (
    int unit,
    UINT32      adrs    /* Local Address */
    )
    {
    return (LOCAL2PCI_MEMIO(adrs));
    }

/******************************************************************************
*
* sysMvSataPciBusToLocalAdrs - convert a bus address to a local address
*
* This routine returns a local address that is used to access the PCIbus.
* The bus address that is passed into this routine is the PCIbus address
* as it would be seen on the local bus.
*
*/

LOCAL UINT32 sysMvSataPciBusToLocalAdrs
    (
    int unit,
    UINT32      adrs    /* PCI Address */
    )
    {
    return (PCI2LOCAL_MEMIO(adrs));
    }

/*****************************************************************************
*
* sysMvSataIntEnable - enable  interrupts
*
*
* RETURNS: OK, or ERROR if interrupts could not be enabled.
*/

LOCAL STATUS sysMvSataIntEnable
    (
    int    unit        /* local unit number */
    )
    {
    return (OK);
    }

/*****************************************************************************
*
* sysMvSataIntDisable - disable  interrupts
*
* RETURNS: OK, or ERROR if interrupts could not be disabled.
*/

LOCAL STATUS sysMvSataIntDisable
    (
    int    unit        /* local unit number */
    )
    {
    return (OK);
    }
