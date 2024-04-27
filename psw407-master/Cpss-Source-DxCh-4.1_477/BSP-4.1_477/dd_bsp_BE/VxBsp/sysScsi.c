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
#include "ctrlEnv/sys/mvSysSata.h"

/*#include "mvSataAddrDec.h"*/


#include "mrvlSataLib.h"
#undef _DEBUG_
#ifdef _DEBUG_
	#define DB(x)	x
#else
	#define DB(x)
#endif	


/* include PCI Library */

#if (defined(MV_SATA_SUPPORT) && defined (INCLUDE_PCI))

IMPORT STATUS sysMvSataPciProbe (void);
IMPORT STATUS sysMvSataDeviceInit(int    	unit,SATA_INFO  *pSataInfo);
IMPORT  MV_SATA_SCSI_CTRL *mvSataCtrlCreate (SATA_INFO* pSataInfo,int channel);
IMPORT  void mvSataIntr(SIOP *pSiop);

#ifdef MV_SATA_INTR_AS_TASK
IMPORT  void mvSataIntrTask(SIOP *pSiop); 
#endif
IMPORT  void mvSataRescanTask(SIOP *pSiop);

SATA_INFO  sataDevices[TOTAL_SATA_DEVICE];
SCSI_CTRL* sysScsiCtrl[TOTAL_SCSI_CTRL];
UINT32 sataUnits;

STATUS sysScsiInit(void)
{
	
	int unitInx;
	SATA_INFO* pSata;
	VOIDFUNCPTR		isrRtn 	= mvSataIntr;

#ifdef _DEBUG_
	scsiDebug = FALSE;
	scsiIntsDebug = FALSE;

	sataDebugIdMask = 0xffff;
	sataDebugTypeMask = 0xffff;
#else
	scsiDebug = FALSE;
	scsiIntsDebug = FALSE;
	sataDebugIdMask = 0;
	sataDebugTypeMask = 0;
#endif

	mvSataWinInit();
	sataUnits = sysMvSataPciProbe();

	if (sataUnits == 0)	return ERROR;

	for (unitInx=0 ; unitInx< sataUnits ; unitInx++)
	{

		pSata = &sataDevices[unitInx];
		DB(mvOsPrintf ("sysScsiInit: sataUnit = %d \n",unitInx));

		if (ERROR == sysMvSataDeviceInit(unitInx, pSata))
		{
			mvOsPrintf(" Sata Device initialize core failed unit=%d\n",unitInx);
			continue;
		}

#ifdef _DEBUG_
		if ((sataDebugIdMask)||(sataDebugTypeMask))
		{
			mvMicroSecondsDelay(NULL,4000000);
		}
#endif

		pSysScsiCtrl = sysScsiCtrl[unitInx] = (SCSI_CTRL*)mvSataCtrlCreate(pSata , unitInx);

#ifdef _DEBUG_
		if ((sataDebugIdMask)||(sataDebugTypeMask))
		{
			mvMicroSecondsDelay(NULL,4000000);
		}
#endif

		/* Connecting the Interrupt */

		if (pSata->intConnect)
		{
			if (pSata->intConnect((VOIDFUNCPTR *)INUM_TO_IVEC (pSata->vector),
								 isrRtn,(int)pSysScsiCtrl) == ERROR)
			{
				mvOsPrintf("sysScsiInit: intConnect Failed \n");
				return ERROR;
			}
			DB(mvOsPrintf ("sysScsiInit: intConnect to vector = 0x%x \n",pSata->vector));
		taskSpawn("sataRescan", MV_SATA_INTR_TASK_PRIO, 0, 8*4000,

		         (FUNCPTR)mvSataRescanTask, (int)pSysScsiCtrl, 0, 0, 0, 0, 0,

				  0, 0, 0, 0);

			#ifdef MV_SATA_INTR_AS_TASK
			
			taskSpawn("sataIntr", MV_SATA_INTR_TASK_PRIO, 0, 8*4000,
							(FUNCPTR)mvSataIntrTask, (int)pSysScsiCtrl, 0, 0, 0, 0, 0,
							0, 0, 0, 0);
			
			#endif
		}

		/* Optional auto-config of physical devices on SCSI bus */

		taskDelay (sysClkRateGet () * 1);	/* allow devices to reset */

		mvOsPrintf ("Auto-configuring SCSI bus on Controller 0x%08x...\n\n",(int)pSysScsiCtrl);

		scsiAutoConfig (pSysScsiCtrl);

		scsiShow (pSysScsiCtrl);

		printf ("\n");
		


	}
	DB(mvOsPrintf ("sysScsiInit: ended !!!\n"));

	return OK;

}

void
sysScsiShow (pSysScsiCtrl)
{
	int unitIndex;

	for (unitIndex=0 ; unitIndex < sataUnits ; unitIndex++)
	{
		mvOsPrintf ("Auto-configuring SCSI bus on Controller 0x%08x...\n\n",(int)sysScsiCtrl[unitIndex]);
		scsiShow(sysScsiCtrl[unitIndex]);
	}
}

#endif
