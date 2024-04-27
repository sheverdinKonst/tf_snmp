/*******************************************************************************
*         Copyright 2003, MARVELL SEMICONDUCTOR ISRAEL, LTD.                   *
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
* (MJKK), MARVELL SEMICONDUCTOR ISRAEL. (MSIL),  MARVELL TAIWAN, LTD. AND      *
* SYSKONNECT GMBH.                                                             *
********************************************************************************
* Public files: GmInitInterfaceAPI.h
*
* DESCRIPTION:
*       This file contains interface function of the Marvell EBU chip
*       Golden Model.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 2 $
*******************************************************************************/
#ifndef _GMINITINTERFACEAPI_H_
#define _GMINITINTERFACEAPI_H_

#ifdef __cplusplus
extern "C"
{
#endif

/* Callback definitions */

typedef void (*ppInterruptCallBack) (unsigned int uiDeviceId,
                                     unsigned int uiCoreId);
typedef void (*ppWriteMemoryBufferToCPU) (unsigned int uiDeviceId,
                                          unsigned int uiCoreId,
                                          unsigned int addr,
                                          char* bufferToWrite,
                                          unsigned int bufferSize);
typedef void (*ppReadMemoryBufferFromCPU) (unsigned int uiDeviceId,
                                           unsigned int uiCoreId,
                                           unsigned int addr,
                                           char* bufferToRead,
                                           unsigned int bufferSize);
typedef void (*ppGetPacketFromPort) (unsigned int uiDeviceId,
                                     unsigned int uiCoreId,
                                     unsigned int uiPortId,
                                     const char*  packetBuff,
                                     unsigned int packetSize);
typedef void (*ppGetCellFromPort) (unsigned int uiDeviceId,
                                   unsigned int uiCoreId,
                                   unsigned int uiPortId,
                                   const char*  cellBuff,
                                   unsigned int cellSize);
/* API definitions */

typedef struct
{
    ppInterruptCallBack         pfInterruptCallBack;
    ppWriteMemoryBufferToCPU    pfWriteMemoryBufferToCPU;
    ppReadMemoryBufferFromCPU   pfReadMemoryBufferFromCPU;
    ppGetPacketFromPort         pfGetPacketFromPort;
    ppGetCellFromPort           pfGetCellFromPort;

} CALLBACKS_SERVICE;

unsigned int ppGmInit(unsigned int uiDeviceId, const char* devType,
                      CALLBACKS_SERVICE* pParam, const char* iniFile);

#ifdef __cplusplus
}
#endif

#endif /*_GMINITINTERFACEAPI_H_*/
