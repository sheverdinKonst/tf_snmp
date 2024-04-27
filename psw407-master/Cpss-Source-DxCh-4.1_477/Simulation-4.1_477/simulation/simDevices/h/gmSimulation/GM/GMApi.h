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
* Public files: gmApi.h
*
* DESCRIPTION:
*       This file contains interface function of the Marvell EBU chip
*       Golden Model.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 8 $
*******************************************************************************/
#ifndef _gmApi_h_
#define _gmApi_h_


#ifdef __cplusplus
extern "C"
{
#endif

#include "gmExtServices.h"

/* Old fashion API. Tiger simulation compatible */
typedef void (*gm_interrupt_callback) ();
typedef void (*dev_mgr_snd_pkt) (unsigned int uSourceDevice, unsigned int uSourcePort, char* cPacket);
typedef void (*gm_write_memory_buffer) (unsigned int addr, unsigned char* cBuffer, unsigned int len);
typedef void (*gm_read_memory_buffer) (unsigned int addr, unsigned char* cBuffer, unsigned int len);

unsigned int initGoldenModel(const char *libraryPath);

void registerCallBack(gm_interrupt_callback pCallBackMethod,
                      gm_write_memory_buffer pWriteMemoryBufferMethod,
                      gm_read_memory_buffer pReadMemoryBufferMethod);

unsigned int initDevice(unsigned int eDeviceType, unsigned int uDeviceId, unsigned int uPciBaseAddress);

unsigned int pciWrite(unsigned int uDeviceId, unsigned int uAddress, unsigned char* value,  unsigned int size);

unsigned int pciRead(unsigned int uDeviceId, unsigned int uAddress, unsigned char* retValue, unsigned int size);

unsigned int pciConfigWrite(unsigned int uDeviceId, unsigned int uAddress, unsigned char* value,  unsigned int size);

unsigned int pciConfigRead(unsigned int uDeviceId, unsigned int uAddress, unsigned char* retValue, unsigned int size);

unsigned int connectPorts(unsigned int uLeftDev, unsigned int uLeftPort,
                                  unsigned int uRightDev, unsigned int uRightPort);

unsigned int disconnectPort(unsigned int uDevice, unsigned int uPort);

unsigned int connectPortToNIC(unsigned int uDevice, unsigned int uPort);

void sendPacket(unsigned int uSourceDevice, unsigned int uSourcePort, char* cPacket);

unsigned int ReleaseGoldenModel(void);
/* End of old fashion API. Tiger simulation compatible */
#ifdef __cplusplus
}
#endif

#endif /*_gmApi_h_*/


