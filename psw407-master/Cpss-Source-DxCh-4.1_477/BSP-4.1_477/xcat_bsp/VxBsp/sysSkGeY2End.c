/******************************************************************************
 *
 * Name:	sysSkGeY2End.c
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

#include "vxWorks.h"
#include "taskLib.h"
#include "sysLib.h"
#include "config.h"
#include "end.h"
#include "vmLib.h"
#include "intLib.h"
#include "drv/pci/pciIntLib.h"
#include "drv/pci/pciConfigLib.h"
#ifndef MV_78100_PLATFORM
#include "mvPciIf.h"
#endif
#include "mvCpuIf.h"
#include "mvOs.h"
#include "sysPciIntCtrl.h"

#include "h/skdrv1st.h"		/* Driver Specific Definitions */
#include "h/skdrv2nd.h"		/* Adapter Control- and Driver specific Def. */
#ifdef SK_ASF
/* Used for inetLib
 * INCLUDE_INETLIB is defined by default in configAll.h
 */
#include "wrn/coreip/net/inet.h"
#include "wrn/coreip/inetLib.h"
#endif

#ifdef I82543_DEBUG
#   undef    LOCAL
#   define    LOCAL
#endif    /* I82543_DEBUG */

/* include PCI Library */

#ifndef INCLUDE_PCI
#define INCLUDE_PCI
#endif /* INCLUDE_PCI */

/* PCI configuration type */
 
#ifndef PCI_CFG_TYPE
#define PCI_CFG_TYPE            PCI_CFG_NONE
#endif

/* Default RX descriptor  */

#ifndef SGI_RXDES_NUM
#define SGI_RXDES_NUM           RX_MAX_NBR_BUFFERS
#endif

/* Default TX descriptor  */

#ifndef SGI_TXDES_NUM
#define SGI_TXDES_NUM           TX_MAX_NBR_BUFFERS
#endif

/* Default User's flags  */

#ifndef SGI_USR_FLAG
#define SGI_USR_FLAG            0
#endif

#define SGI_OFFSET_VALUE 		SYS_SGI_OFFSET_VALUE

#define SGI0_MEMBASE0_LOW           0xfd000000    /* mem base for CSR */
#define SGI0_MEMBASE0_HIGH          0x00000000    /* mem base for CSR */
#define SGI0_MEMSIZE0               0x20000       /* mem size - CSR,128KB */
#define SGI0_MEMBASE1               0xfd100000    /* mem base - Flash */
#define SGI0_MEMSIZE1               0x00080000    /* mem size - Flash,512KB */
#define SGI0_INT_LVL                0x0b          /* IRQ 11 */
#define SGI0_INIT_STATE_MASK        (NONE)
#define SGI0_INIT_STATE             (NONE)
#define SGI0_SHMEM_BASE             NONE
#define SGI0_SHMEM_SIZE             0
#define SGI0_RXDES_NUM              SGI_RXDES_NUM
#define SGI0_TXDES_NUM              SGI_TXDES_NUM
#define SGI0_USR_FLAG               SGI_USR_FLAG

#define SGI1_MEMBASE0_LOW           0xfd200000    /* mem base for CSR */
#define SGI1_MEMBASE0_HIGH          0x00000000    /* mem base for CSR */
#define SGI1_MEMSIZE0               0x20000       /* mem size - CSR,128KB */
#define SGI1_MEMBASE1               0xfd300000    /* mem base for Flash */
#define SGI1_MEMSIZE1               0x00080000    /* mem size - Flash,512KB */
#define SGI1_INT_LVL                0x05          /* IRQ 5 */
#define SGI1_INIT_STATE_MASK        (NONE)
#define SGI1_INIT_STATE             (NONE)
#define SGI1_SHMEM_BASE             NONE
#define SGI1_SHMEM_SIZE             0
#define SGI1_RXDES_NUM              SGI_RXDES_NUM
#define SGI1_TXDES_NUM              SGI_TXDES_NUM
#define SGI1_USR_FLAG               SGI_USR_FLAG

#define SGI2_MEMBASE0_LOW           0xfd400000    /* mem base - CSR */
#define SGI2_MEMBASE0_HIGH          0x00000000    /* mem base - CSR */
#define SGI2_MEMSIZE0               0x20000       /* mem size - CSR, 128KB */
#define SGI2_MEMBASE1               0xfd500000    /* mem base - Flash */
#define SGI2_MEMSIZE1               0x00080000    /* mem size - Flash,512KB */
#define SGI2_INT_LVL                0x0c          /* IRQ 12 */
#define SGI2_INIT_STATE_MASK        (NONE)
#define SGI2_INIT_STATE             (NONE)
#define SGI2_SHMEM_BASE             NONE
#define SGI2_SHMEM_SIZE             0
#define SGI2_RXDES_NUM              SGI_RXDES_NUM
#define SGI2_TXDES_NUM              SGI_TXDES_NUM
#define SGI2_USR_FLAG               SGI_USR_FLAG

#define SGI3_MEMBASE0_LOW           0xfd600000   /* mem base - CSR */
#define SGI3_MEMBASE0_HIGH          0x00000000   /* mem base - CSR */
#define SGI3_MEMSIZE0               0x20000      /* mem size - CSR,128KB */
#define SGI3_MEMBASE1               0xfd700000   /* mem base - Flash */
#define SGI3_MEMSIZE1               0x00080000   /* mem size - Flash,512KB */
#define SGI3_INT_LVL                0x09         /* IRQ 9 */
#define SGI3_INIT_STATE_MASK        (NONE)
#define SGI3_INIT_STATE             (NONE)
#define SGI3_SHMEM_BASE             NONE
#define SGI3_SHMEM_SIZE             0
#define SGI3_RXDES_NUM              SGI_RXDES_NUM
#define SGI3_TXDES_NUM              SGI_TXDES_NUM
#define SGI3_USR_FLAG               SGI_USR_FLAG

#define SGI_LOAD_FUNC          		skGeEndLoad

/* externs */

IMPORT END_TBL_ENTRY    endDevTbl[];    /* end device table */
IMPORT void     sysUsDelay (UINT32);
IMPORT UINT32   sysPciHostIfGet(int bus); /* For multiple PCI if support */

/* typedefs */
#define BUFF_SIZE       2048    /* buffer size for driver */

typedef struct sgiResource        /* SGI_RESOURCE */
    {
    UINT32 memBaseLow;            /* Base Address LOW */
    UINT32 memBaseHigh;           /* Base Address HIGH */
    UINT32 flashBase;             /* Base Address for FLASH */
    char   irq;                   /* Interrupt Request Level */
    BOOL   adr64;                 /* Indicator for 64-bit support */
    
    unsigned short	vendorId;	  /* adapter vendor ID */
    unsigned short	deviceId;	  /* adapter device ID */
	int    pciBus;                /* PCI Bus number */
    int    pciDevice;             /* PCI Device number */
    int    pciFunc;               /* PCI Function number */
    
	STATUS iniStatus;             /* initialization perform status */
    UINT32 shMemBase;             /* Share memory address if any */
    UINT32 shMemSize;             /* Share memory size if any */
    UINT32 rxDesNum;              /* RX descriptor for this unit */
    UINT32 txDesNum;              /* TX descriptor for this unit */
    UINT32 usrFlags;              /* user flags for this unit */
    } SGI_RESOURCE;

typedef struct _skPciDev
{
    unsigned short	vendorId;	/* adapter vendor ID */
    unsigned short	deviceId;	/* adapter device ID */
	int				instance;	/* Number of times device found in system */
}SK_PCI_DEV;

/* locals */

LOCAL UINT32 sysSkGeLocalToPciBusAdrs (int unit, UINT32 adrs);
LOCAL UINT32 sysSkGePciBusToLocalAdrs (int unit, UINT32 adrs);
LOCAL UINT32 sgiUnits = 0;     /* number of SGIs we found */

LOCAL SGI_RESOURCE sgiResources [] =
    {
    {SGI0_MEMBASE0_LOW,SGI0_MEMBASE0_HIGH, SGI0_MEMBASE1, SGI0_INT_LVL, 
     UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,
     ERROR, SGI0_SHMEM_BASE, SGI0_SHMEM_SIZE, SGI0_RXDES_NUM, SGI0_TXDES_NUM, 
     SGI0_USR_FLAG},
    {SGI1_MEMBASE0_LOW, SGI1_MEMBASE0_HIGH, SGI1_MEMBASE1, SGI1_INT_LVL, 
     UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,
     ERROR, SGI1_SHMEM_BASE, SGI1_SHMEM_SIZE, SGI1_RXDES_NUM, SGI1_TXDES_NUM, 
     SGI1_USR_FLAG},
    {SGI2_MEMBASE0_LOW, SGI2_MEMBASE0_HIGH, SGI2_MEMBASE1, SGI2_INT_LVL, 
     UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,
     ERROR,SGI2_SHMEM_BASE, SGI2_SHMEM_SIZE, SGI2_RXDES_NUM, SGI2_TXDES_NUM, 
     SGI2_USR_FLAG},
    {SGI3_MEMBASE0_LOW,SGI3_MEMBASE0_HIGH, SGI3_MEMBASE1, SGI3_INT_LVL, 
     UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,
     ERROR,SGI3_SHMEM_BASE, SGI3_SHMEM_SIZE, SGI3_RXDES_NUM, SGI3_TXDES_NUM, 
     SGI3_USR_FLAG},
     };


/* Driver supported PCI devices */
LOCAL SK_PCI_DEV sgiPciDevTbl[] = {
	/* SysKonnect (1148), SK-98xx V2.0 Gigabit Ethernet Adapter */
	{ 0x1148, 0x4320, 0},
	/* SysKonnect (1148), SK-9Exx PCI Express 10/100/1000Base-T Adapter */
	{ 0x1148, 0x9E00, 0},
	/* SysKonnect (1148), SK-9Sxx PCI-X 10/100/1000Base-T Adapter */
	{ 0x1148, 0x9000, 0},
	
	/* Marvell (11ab), Yukon1 Gigabit Ethernet Controller */
	{ 0x11ab, 0x4320, 0},
	/* Marvell (11ab), Yukon2 PCI-X Gigabit Ethernet Controller */
	{ 0x11ab, 0x4340, 0},
	/* Marvell (11ab), Yukon FE Fast Ethernet Controller */
	{ 0x11ab, 0x4350, 0},
	/* Marvell (11ab), Yukon FE Fast Ethernet Controller */
	{ 0x11ab, 0x4351, 0},
	/* Marvell (11ab), Yukon EC Gigabit Ethernet Controller */
	{ 0x11ab, 0x4360, 0},
	/* Marvell (11ab), Yukon EC Gigabit Ethernet Controller */
	{ 0x11ab, 0x4361, 0},
	/* Marvell (11ab), Yukon EC Gigabit Ethernet Controller */
	{ 0x11ab, 0x4362, 0},
    /* Marvell (11ab), Yukon EC Ultra Gigabit Ethernet Controller */
    { 0x11ab, 0x4363, 0},
    /* Marvell (11ab), Yukon EC Ultra Gigabit Ethernet Controller */
    { 0x11ab, 0x4364, 0},
#ifdef MV_ADD_EXTREME_SUPREME_SUPPORT
    /* Marvell (11ab), Yukon Extreme Gigabit Ethernet Controller */
    { 0x11ab, 0x4365, 0},
    /* Marvell (11ab), Yukon Extreme Gigabit Ethernet Controller */
    { 0x11ab, 0x436b, 0},
    /* Marvell (11ab), Yukon Extreme Gigabit Ethernet Controller */
    { 0x11ab, 0x436c, 0},
    /* Marvell (11ab), Yukon Supreme Gigabit Ethernet Controller */
    { 0x11ab, 0x4370, 0},
#endif
	/* Marvell (11ab), Belkin */
	{ 0x11ab, 0x5005, 0}
};

/* globals */

/* forward declarations */

LOCAL int       sysSkSGIntEnable  (int unit);
LOCAL int       sysSkSGIntDisable (int unit);
LOCAL int       sysSkSGIntAck     (int unit);
LOCAL void      sysSkGeLoadStrCompose (int unit);
LOCAL void      sysSkGePhySpecRegsInit(PHY_INFO *, UINT8);
/*****************************************************************************
*
* sysSkGePciInit - Initialize and get the PCI configuration for 82543 Chips
*
* This routine finds out PCI device, and maps its memory and IO address.
* It must be done prior to initializing of 82543 chips.  Also
* must be done prior to MMU initialization, usrMmuInit().
*
* RETURNS: N/A
*/

STATUS sysSkGePciInit (void)
{
    SGI_RESOURCE *pReso;         /* chip resources */
    int typeInx;			 	 /* adapter device type index*/
	int pciBus;                  /* PCI bus number */
    int pciDevice;               /* PCI device number */
    int pciFunc;                 /* PCI function number */
    int unit;                    /* unit number */
    UINT32 bar0;                 /* PCI BAR_0 */
    UINT32 memBaseLow;           /* mem base low */
    UINT32 memBaseHigh;          /* mem base High */
    UINT32 flashBase;            /* flash base */
    UINT16 boardId;              /* adaptor Id */
    char   irq;                  /* irq number */
	BOOL   devFound;

#ifdef SK_CHECK_DEVICE
	printf("sysSkGePciInit SEARCH DEVICE\n");
#endif

    for (unit = 0; unit < NELEMENTS(sgiResources); unit++)
    {
        boardId = UNKNOWN;
		devFound = FALSE;
		for (typeInx = 0; typeInx < NELEMENTS(sgiPciDevTbl); typeInx++)
		{
#ifdef SK_CHECK_DEVICE
			printf("sysSkGePciInit VENDOR 0x%x DEVICE 0x%x INSTANCE %d\n",
					   sgiPciDevTbl[typeInx].vendorId,
					   sgiPciDevTbl[typeInx].deviceId,
					   sgiPciDevTbl[typeInx].instance);
#endif
			/* Detect possible adaptors */
			if (pciFindDevice (sgiPciDevTbl[typeInx].vendorId,
							   sgiPciDevTbl[typeInx].deviceId,
                               sgiPciDevTbl[typeInx].instance,
							   &pciBus, &pciDevice, &pciFunc) == OK)
            {
				sgiPciDevTbl[typeInx].instance++;
#ifdef SK_CHECK_DEVICE
				printf("sysSkGePciInit DEVICE FOUND\n");
#endif
                devFound = TRUE;
				break;
			}
		}

		if(devFound == FALSE) {	/* No more devices */
#ifdef SK_CHECK_DEVICE
			printf("sysSkGePciInit NO DEVICE FOUND\n");
#endif
			break;
		}

        pReso = &sgiResources [unit];
        pReso->vendorId  = sgiPciDevTbl[typeInx].vendorId;
        pReso->deviceId  = sgiPciDevTbl[typeInx].deviceId;
		pReso->pciBus    = pciBus;
        pReso->pciDevice = pciDevice;
        pReso->pciFunc   = pciFunc;

        pciConfigInLong (pReso->pciBus, pReso->pciDevice, pReso->pciFunc,
                          PCI_CFG_BASE_ADDRESS_0, &bar0);

        pReso->adr64 = ((bar0 & BAR0_64_BIT) == BAR0_64_BIT)? TRUE : FALSE;
        
        /* If configured, set the PCI Configuration manually */

        if (PCI_CFG_TYPE == PCI_CFG_FORCE) 
            {
            pciConfigOutLong (pReso->pciBus, pReso->pciDevice, pReso->pciFunc,
                              PCI_CFG_BASE_ADDRESS_0, pReso->memBaseLow);
            if (pReso->adr64)
                {
                pciConfigOutLong (pReso->pciBus, pReso->pciDevice, 
                                  pReso->pciFunc,PCI_CFG_BASE_ADDRESS_1, 
                                  pReso->memBaseHigh);

                pciConfigOutLong (pReso->pciBus, pReso->pciDevice, 
                                  pReso->pciFunc, PCI_CFG_BASE_ADDRESS_2, 
                                  pReso->flashBase);
                }
            else
                {
                pciConfigOutLong (pReso->pciBus, pReso->pciDevice, 
                                  pReso->pciFunc, PCI_CFG_BASE_ADDRESS_1, 
                                  pReso->flashBase);

                pciConfigOutLong (pReso->pciBus, pReso->pciDevice, 
                                  pReso->pciFunc, PCI_CFG_BASE_ADDRESS_2, 
                                  pReso->flashBase);
                }

            pciConfigOutByte (pReso->pciBus, pReso->pciDevice, pReso->pciFunc,
                              PCI_CFG_DEV_INT_LINE, pReso->irq);
            }
        
        /* read back memory base address and IO base address */

        pciConfigInLong (pReso->pciBus, pReso->pciDevice, pReso->pciFunc,
                          PCI_CFG_BASE_ADDRESS_0, &memBaseLow);
        if (pReso->adr64)
            {        
            pciConfigInLong (pReso->pciBus, pReso->pciDevice, pReso->pciFunc,
                              PCI_CFG_BASE_ADDRESS_1, &memBaseHigh);
            pciConfigInLong (pReso->pciBus, pReso->pciDevice, pReso->pciFunc,
                              PCI_CFG_BASE_ADDRESS_2, &flashBase);
            }
        else
            {
            pciConfigInLong (pReso->pciBus, pReso->pciDevice, pReso->pciFunc,
                              PCI_CFG_BASE_ADDRESS_1, &flashBase);
            memBaseHigh = 0x0;
            }

        pciConfigInByte (pReso->pciBus, pReso->pciDevice, pReso->pciFunc,
                          PCI_CFG_DEV_INT_LINE, &irq);

        memBaseLow &= PCI_MEMBASE_MASK;
        flashBase  &= PCI_MEMBASE_MASK;

        /* over write the resource table with read value */

        pReso->memBaseLow   = memBaseLow;
        pReso->memBaseHigh  = memBaseHigh;
        pReso->flashBase    = flashBase;
        pReso->irq          = irq;

        /* enable mapped memory and IO addresses */

        pciConfigOutWord (pReso->pciBus, pReso->pciDevice, pReso->pciFunc,
                           PCI_CFG_COMMAND, PCI_CMD_IO_ENABLE |
                           PCI_CMD_MEM_ENABLE | PCI_CMD_MASTER_ENABLE);
           
        /* compose loadString in endDevTbl for this unit */        

        sysSkGeLoadStrCompose (unit);

        sgiUnits++;
    }

#ifdef SK_CHECK_DEVICE
	printf("sysSkGePciInit SEARCH DEVICE ENDS\n");
#endif

    return OK;
}


/****************************************************************************
*
* sysSkGeLoadStrCompose - Compose the END load string for the device
*
* The END device load string formed by this routine is in the following
* following format.
* <shMemBase>:<shMemSize>:<rxDesNum>:<txDesNum>:<usrFlags>:<offset>
*
* RETURN: N/A
*/

LOCAL void sysSkGeLoadStrCompose(int unit)
{
    END_TBL_ENTRY *     pDevTbl;

    for (pDevTbl = endDevTbl; pDevTbl->endLoadFunc != END_TBL_END; pDevTbl++)
    {
		if (((UINT32)pDevTbl->endLoadFunc == (UINT32)SGI_LOAD_FUNC) &&
            (pDevTbl->unit == unit))
		{
			sprintf (pDevTbl->endLoadString, "0x%x:0x%x:0x%x:0x%x:0x%x:%d",
			/* This will need to be setup in the original sgiResources table. */
					sgiResources [unit].shMemBase,
					sgiResources [unit].shMemSize,
					sgiResources [unit].rxDesNum,  /* RX Descriptor Number*/
					sgiResources [unit].txDesNum,  /* TX Descriptor Number*/
					sgiResources [unit].usrFlags,  /* user's flags */
					SYS_SGI_OFFSET_VALUE);         /* offset value */                 
					              
			/* Also, enable the table entry */
			pDevTbl->processed = FALSE;
			return;
		}
    }
    return;
}

/*****************************************************************************
*
* sysSkGeBoardInit - Adaptor initialization for 82543 chip
*
* This routine is expected to perform any adapter-specific or target-specific
* initialization that must be done prior to initializing the 82543 chip.
*
* The 82543 driver calls this routine from the driver load routine before
* any other routines.
*
* RETURNS: OK or ERROR
*/

STATUS sysSkGeBoardInit
    (
    int    	unit,                /* unit number */
	ADAPTOR_INFO  *pBoard       /* board information for the SGI driver */
    )
    {
	int typeInx;			 	 /* adapter device type index*/
    SGI_RESOURCE *pReso = &sgiResources [unit];

    /* sanity check */
    if (unit >= sgiUnits)
        return (ERROR);

	for (typeInx = 0; typeInx < NELEMENTS(sgiPciDevTbl); typeInx++)
	{      
        if ((pReso->vendorId == sgiPciDevTbl[typeInx].vendorId) &&
			(pReso->deviceId == sgiPciDevTbl[typeInx].deviceId))
            break;
	}

	if (typeInx == NELEMENTS(sgiPciDevTbl))
	{
		return ERROR;
	}
    
	/* initializes the board information structure */    
    pBoard->pciDev.vendorId    = pReso->vendorId;
    pBoard->pciDev.deviceId    = pReso->deviceId;
	pBoard->pciDev.pciBusNo    = pReso->pciBus;
    pBoard->pciDev.pciDeviceNo = pReso->pciDevice;
    pBoard->pciDev.pciFuncNo   = pReso->pciFunc;
	
    pBoard->vector        = pReso->irq;

    pBoard->regBaseLow    = PCI_MEMIO2LOCAL(pReso->memBaseLow);
    pBoard->regBaseHigh   = PCI_MEMIO2LOCAL(pReso->memBaseHigh);
    pBoard->flashBase     = PCI_MEMIO2LOCAL(pReso->flashBase);
    pBoard->adr64         = pReso->adr64;

    pBoard->intEnable     = sysSkSGIntEnable;
    pBoard->intDisable    = sysSkSGIntDisable;
    pBoard->intAck        = sysSkSGIntAck;
    
	pBoard->phySpecInit   = sysSkGePhySpecRegsInit;
    pBoard->sysLocalToBus = sysSkGeLocalToPciBusAdrs; 
    pBoard->sysBusToLocal = sysSkGePciBusToLocalAdrs; 

    /* specify the interrupt connect/disconnect routines to be used */

    pBoard->intConnect    = (FUNCPTR) pciIntConnect;
    pBoard->intDisConnect = (FUNCPTR) pciIntDisconnect;

    /* we finished adaptor initialization */
    pReso->iniStatus = OK;

#ifndef MV_78100_PLATFORM
    /* enable adaptor interrupt */

    sysPciIntEnable(pReso->irq);
#endif

    return (OK);
    }
/**************************************************************************
*
* sysSkGePhySpecRegsInit - Initialize PHY specific registers
*
* This routine initialize PHY specific registers
*
* RETURN: N/A
*/

LOCAL void sysSkGePhySpecRegsInit
    (
    PHY_INFO * pPhyInfo,    /* PHY's info structure pointer */
    UINT8     phyAddr       /* PHY's bus number */
    )
    {
     /* other PHYS .... */

     return;
     }

/*****************************************************************************
*
* sysSkSGIntAck - acknowledge an 82543 interrupt
*
* This routine performs any 82543 interrupt acknowledge that may be
* required.  This typically involves an operation to some interrupt
* control hardware.
*
* This routine gets called from the 82543 driver's interrupt handler.
*
* This routine assumes that the PCI configuration information has already
* been setup.
*
* RETURNS: OK, or ERROR if the interrupt could not be acknowledged.
*/

LOCAL STATUS sysSkSGIntAck
    (
    int    unit        /* unit number */
    )
    {
    return (OK);
    }
/******************************************************************************
*
* sysSkGeLocalToPciBusAdrs - convert a local address to a bus address
*
* This routine returns a PCIbus address for the LOCAL bus address.
*
*/
 
LOCAL UINT32 sysSkGeLocalToPciBusAdrs
    (
    int unit,
    UINT32      adrs    /* Local Address */
    )
    {
    return (LOCAL2PCI_MEMIO(adrs));
    }
 
 
/******************************************************************************
*
* sysSkGePciBusToLocalAdrs - convert a bus address to a local address
*
* This routine returns a local address that is used to access the PCIbus.
* The bus address that is passed into this routine is the PCIbus address
* as it would be seen on the local bus.
*
*/
 
LOCAL UINT32 sysSkGePciBusToLocalAdrs
    (
    int unit,
    UINT32      adrs    /* PCI Address */
    )
    {
    return (PCI2LOCAL_MEMIO(adrs));
    }

/*****************************************************************************
*
* sysSkSGIntEnable - enable 82543 interrupts
*
* This routine enables 82543 interrupts.  This may involve operations on
* interrupt control hardware.
*
* The 82543 driver calls this routine throughout normal operation to terminate
* critical sections of code.
*
* This routine assumes that the PCI configuration information has already
* been setup.
*
* RETURNS: OK, or ERROR if interrupts could not be enabled.
*/

LOCAL STATUS sysSkSGIntEnable
    (
    int    unit        /* local unit number */
    )
    {
#ifdef MV_78100_PLATFORM
        SGI_RESOURCE *pReso = &sgiResources [unit];

        /* enable adaptor interrupt */
        sysPciIntEnable(pReso->irq);
#endif
        return (OK);
    }

/*****************************************************************************
*
* sysSkSGIntDisable - disable 82543 interrupts
*
* This routine disables 82543 interrupts.  This may involve operations on
* interrupt control hardware.
*
* The 82543 driver calls this routine throughout normal operation to enter
* critical sections of code.
*
* This routine assumes that the PCI configuration information has already
* been setup.
*
* RETURNS: OK, or ERROR if interrupts could not be disabled.
*/

LOCAL STATUS sysSkSGIntDisable
    (
    int    unit        /* local unit number */
    )
    {
#ifdef MV_78100_PLATFORM
        SGI_RESOURCE *pReso = &sgiResources [unit];

        /* disable adaptor interrupt */
        sysPciIntDisable(pReso->irq);
#endif
        return (OK);
    }

/*****************************************************************************
*
* sysSkGeShow - shows 82543 configuration 
*
* This routine shows adapters configuration 
*
* RETURNS: N/A
*/

STATUS sysSkGeShow
	(
	int    unit        /* unit number */
	)
{
	SGI_RESOURCE *pReso = &sgiResources [unit];

	if (unit >= sgiUnits) {
		printf ("invalid unit number: %d\n", unit);
		return ERROR;
	}

	if ((pReso->vendorId == 0x1148) && (pReso->deviceId == 0x9e00))
		printf ("********* SysKonnect SK-9E21 Adapter ***********\n");

	else if ((pReso->vendorId == 0x1148) && (pReso->deviceId == 0x9000))
		printf ("********* SysKonnect SK-9S21 Adapter ***********\n");

	else if ((pReso->vendorId == 0x11ab) && (pReso->deviceId == 0x4340))
		printf ("***** Marvell SK-9E21 PCI-Express Adapter *******\n");

	else if ((pReso->vendorId == 0x11ab) && (pReso->deviceId == 0x4360))
		printf ("******* Marvell GbE PCI-Express Adapter *********\n");

	else if ((pReso->vendorId == 0x11ab) && (pReso->deviceId == 0x4351))
		printf ("***** Marvell Fast Eth PCI-Express Adapter ******\n");
#ifdef MV_ADD_EXTREME_SUPREME_SUPPORT
	else if ((pReso->vendorId == 0x11ab) && (pReso->deviceId == 0x4365))
		printf ("******* Marvell GbE PCI-Express Adapter *********\n");

	else if ((pReso->vendorId == 0x11ab) && (pReso->deviceId == 0x436b))
		printf ("******* Marvell GbE PCI-Express Adapter *********\n");

	else if ((pReso->vendorId == 0x11ab) && (pReso->deviceId == 0x436c))
		printf ("******* Marvell GbE PCI-Express Adapter *********\n");

	else if ((pReso->vendorId == 0x11ab) && (pReso->deviceId == 0x4370))
		printf ("******* Marvell GbE PCI-Express Adapter *********\n");
#endif
	else
		printf ("*************** UNKNOWN Adapter *****************\n");

	printf ("  PCI VendorId              = 0x%x\n", pReso->vendorId);
	printf ("  PCI DeviceId              = 0x%x\n", pReso->deviceId);
	printf ("  PCI bus number            = 0x%x\n", pReso->pciBus);
	printf ("  PCI device number         = 0x%x\n", pReso->pciDevice);
	printf ("  PCI function number       = 0x%x\n", pReso->pciFunc);
	printf ("  CSR PCI Membase address   = 0x%x\n", pReso->memBaseLow);
	printf ("  Flash PCI Membase address = 0x%x\n", pReso->flashBase);
	printf ("  IRQ = %d\n", pReso->irq);

	if (pReso->iniStatus == ERROR)
		printf ("  Was not initialized by any network interface\n");  
	else
		printf ("  Initialized by a network interface\n");          

	printf ("*************************************************\n");
	return OK;
}

#ifdef SK_ASF
STATUS sysSkGeSetAddress
	(
	int  Unit,
	char *String        /* IP address */
	)
{
	char   InetString[SK_INET6_LENGTH];
	u_long InetAddr;
	int    i;
	END_DEVICE *pDrvCtrl; /* driver structure */
	SK_AC      *pAC;

	if (String == NULL) {
		printf("%s%d: sysSkGeSetAddress  No IP Address given!\n", DEVICE_NAME, Unit);
		return ERROR;
	}
	else {
		printf("%s%d: sysSkGeSetAddress  Try to give IP Address %s to FW!\n", DEVICE_NAME, Unit, String);
	}

	InetAddr = inet_addr(String);
	if (InetAddr == ERROR) {
		printf("%s%d: sysSkGeSetAddress  Illegal IP Address given!\n", DEVICE_NAME, Unit);
		return ERROR;
	}

	SK_MEMSET(InetString, 0, SK_INET6_LENGTH);
	SK_STORE_U32(InetString, InetAddr);

	pDrvCtrl = (END_DEVICE *)endFindByName(DEVICE_NAME, Unit);
	if(pDrvCtrl == NULL)
	{
		printf("%s%d: No such interface\n", DEVICE_NAME, Unit);
		return ERROR;
	}
	pAC = pDrvCtrl->pAC;

#ifdef SK_CHECK_FW_IP
	printf("sysSkGeSetAddress InetAddr:        0x%x\n", InetAddr);
	for (i=0; i<4; i++) {
		printf("sysSkGeSetAddress InetString[%d]:   0x%x\n", i, InetString[i]);
	}
#endif

	SK_MEMSET(&pAC->IpAddr, 0, SK_INET4_LENGTH);
	bcopy((char *)InetString, (char *)&pAC->IpAddr, SK_INET4_LENGTH);

#ifdef SK_CHECK_FW_IP
	for (i=0; i<4; i++) {
		printf("sysSkGeSetAddress pAC->IpAddr[%d]:  %d\n", i, pAC->IpAddr[i]);
	}
#endif

	pAC->ForceFWIPUpdate = SK_TRUE;
	printf("%s%d: sysSkGeSetAddress  IP address %s was given to FW!\n", DEVICE_NAME, Unit, String);

	return OK;
}
#endif

