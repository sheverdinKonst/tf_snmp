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
* mrvlSataLib.h - H File for VxWorks IAL.
*
* DESCRIPTION:
*       None.
*
* DEPENDENCIES:
*
*******************************************************************************/
#ifndef _MRVL_SATA_VXWORKS_H_
#define _MRVL_SATA_VXWORKS_H_

#define INCLUDE_SCSI2	/* This is a SCSI2 driver */
#include "vxWorks.h"
#include "memLib.h"
#include "ctype.h"
#include "stdlib.h"
#include "string.h"
#include "stdio.h"
#include "logLib.h"
#include "semLib.h"
#include "intLib.h"
#include "errnoLib.h"
#include "cacheLib.h"
#include "taskLib.h"
#include "scsiLib.h"
#include "sata/CoreDriver/mvSata.h"
#include "mvScsiAtaLayer.h"
#include "mvIALCommon.h"


#define MV_IAL_SACOALT_DEFAULT       4
#define MV_IAL_SAITMTH_DEFAULT       (150 *50)

#define MRVL_MAX_CMDS_PER_ADAPTER   16      /*2 commands per channel*/
#define MRVL_MAX_SG_LIST             16
#define MRVL_MAX_IO_LEN              (1 << 16)
#define MRVL_MAX_BUFFER_PER_PRD_ENTRY    (1 << 16)

#define MRVL_COMMAND_INFO_FLAGE_DMA    1
#define MRVL_COMMAND_INFO_FLAGE_PRD_TABLE  2
#define MV_MAX_SENSE_BUFFER	16



#define  MV_SATA_INTR_AS_TASK
#define  MV_SATA_TIMER_AS_TASK
#define MV_SATA_INTR_TASK_PRIO	SCSI_DEF_TASK_PRIORITY
/*#define MV_SATA_CACHED*/

struct sata_info;

/*****************************************************************************/


/* The expanded THREAD structure, with add'l device specific info */

typedef struct mvSataThread	/* MV_SATA_THREAD, extended thread type */
{
    SCSI_THREAD	scsiThread;	/* generic SCSI thread structure */

    /* TODO - add device specific thread info */



}MV_SATA_THREAD;

/* The expanded SCSI_CTRL structure with add'l device specific info */

/**************************/

typedef struct 
{		/* MV_SATA_SCSI_CTRL */
    SCSI_CTRL scsiCtrl;         /* generic SCSI controller info */

    /* TODO - add device specific information */

	struct sata_info*	pSataInfo;
	
	SEM_ID    singleStepSem;	/* use to debug script in single step mode */
#ifdef MV_SATA_INTR_AS_TASK
	SEM_ID    interruptsSem;	
#endif
	SEM_ID    rescanSem;	

    /* TODO Hardware implementation dependencies */

}MV_SATA_SCSI_CTRL;

/**************************/

typedef struct 
{ 		/* MV_SATA_EVENT, extended EVENT type */

 SCSI_EVENT scsiEvent;
 MV_SATA_SCSI_CMD_BLOCK *pCmdBlock;


}MV_SATA_EVENT;


typedef MV_SATA_SCSI_CTRL SIOP;	/* shorthand */

/*****************************************************************************/

typedef struct _pciDevice
{
    unsigned short	vendorId;	/* adapter vendor ID */
    unsigned short	deviceId;	/* adapter device ID */
	int	pciBusNo;    			/* adapter bus number */
    int	pciDeviceNo; 			/* adapter device number */
    int	pciFuncNo;	  			/* adapter function number */
	unsigned char	chipRev;
}PCI_DEV;


/**************************/

typedef struct _mvSataChannelData
{
  MV_SATA_CHANNEL	mvSataChannel;
  UINT32			osData;	
  UINT32			requestVirtMem , requestPhysMem;	
  UINT32			respondVirtMem , respondPhysMem;
  MV_U16 targetsToRemove;
  MV_U16 targetsToAdd;
}MV_SATA_CHANNEL_DATA;

/**************************/



typedef struct _mvCommandInfo
{
  MV_SATA_SCSI_CMD_BLOCK           scb;
  int               flags;
  UINT32  			mvPrdVirtMem , mvPrdPhysMem;
  UINT32			xferVirtMem , xferPhysMem, xferLen;
  UINT32			pThread;
  UINT8				senseBuffer[MV_MAX_SENSE_BUFFER];
  struct sata_info	*sataInfo;


}MV_COMMAND_INFO;

/**************************/

typedef struct sata_info 
    {
    PCI_DEV pciDev;
	int     vector;          /* interrupt vector */
    UINT32  regBase;      	/* register PCI base address - low  */

	SEM_ID    mutualSem;	

    int  	unit;      	/* unit  */

    UINT32 (*sysLocalToBus)(int,UINT32);
    UINT32 (*sysBusToLocal)(int,UINT32);

    STATUS (*intEnable)(int);    /* board specific interrupt enable routine */
    STATUS (*intDisable)(int);   /* board specific interrupt disable routine */
    STATUS (*intAck) (int);      /* interrupt ack */

    FUNCPTR intConnect;     /* interrupt connect function */ 
    FUNCPTR intDisConnect;  /* interrupt disconnect function */


	MV_SATA_ADAPTER       			mvSataAdapter;
	MV_SATA_CHANNEL_DATA			mvChannelData[MV_SATA_CHANNELS_NUM];
	MV_SATA_SCSI_CTRL*				pSataScsiCtrl;
	MV_SAL_ADAPTER_EXTENSION 		ataScsiAdapterExt;
	MV_IAL_COMMON_ADAPTER_EXTENSION ialCommonExt;
	int                             cibsTop;

	MV_COMMAND_INFO 				commandsInfoBlocks[MRVL_MAX_CMDS_PER_ADAPTER];
	MV_COMMAND_INFO  				*commandsInfoArray[MRVL_MAX_CMDS_PER_ADAPTER];
	WDOG_ID 						sataWatchDogId;
	WDOG_ID 						sataRescanWatchDogId;
	MV_ULONG						UncacheVirtMemoryBuffer;
	MV_ULONG						UncachePhyMemoryBuffer;

}SATA_INFO;


/******************************************************************************/

#endif /* _MRVL_SATA_VXWORKS_H_ */
