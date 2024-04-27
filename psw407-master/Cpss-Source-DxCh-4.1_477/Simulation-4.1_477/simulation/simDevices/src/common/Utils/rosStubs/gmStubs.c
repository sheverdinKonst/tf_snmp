/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* gmStubs.c
*
* DESCRIPTION:
*       This is API stub for the golden model simulation .
*       When using there is no need for GM libraries .
*       Implementation is only for the function that
*       the simulation calls.
*
* FILE REVISION NUMBER:
*       $Revision: 8 $
*
*******************************************************************************/

#include <gmSimulation/GM/GMApi.h>
#include <gmSimulation/GM/gmTrafficApi.h>
#include <gmSimulation/GM/GmInitInterfaceAPI.h>
#include <gmSimulation/GM/ManagementAPI.h>
#include <gmSimulation/GM/TrafficAPI.h>

/* -------------- stubs for GM ------------- */
#if (! defined (USE_TIGER_GM)) && (! defined (USE_CHEEATAH_GM))

unsigned int initGoldenModel(const char *libraryPath)
{
    return 0;
}

unsigned int initDevice(unsigned int eDeviceType, unsigned int uDeviceId, unsigned int uPciBaseAddress)
{
    return 0;
}

unsigned int pciWrite(unsigned int uDeviceId, unsigned int uAddress, unsigned char* value,  unsigned int size)
{
    return 0;
}

unsigned int pciRead(unsigned int uDeviceId, unsigned int uAddress, unsigned char* retValue, unsigned int size)
{
    return 0;
}

unsigned int pciConfigWrite(unsigned int uDeviceId, unsigned int uAddress, unsigned char* value,  unsigned int size)
{
    return 0;
}

unsigned int pciConfigRead(unsigned int uDeviceId, unsigned int uAddress, unsigned char* retValue, unsigned int size)
{
    return 0;
}

unsigned int ReleaseGoldenModel(void)
{
    return 0;
}

unsigned int InitTraffic(void)
{
    return 0;
}

unsigned int registerTrafficCallBack(gm_receive_packet_callback pCallBackMethod)
{
    return 0;
}

unsigned int SendPkt(unsigned int uSourceDevice, unsigned int uSourcePort, unsigned char* cPacket, unsigned int uiSize)
{
    return 0;
}

unsigned int ReleaseTraffic(void)
{
    return 0;
}

void registerCallBack(gm_interrupt_callback pCallBackMethod,
                      gm_write_memory_buffer pWriteMemoryBufferMethod,
                      gm_read_memory_buffer pReadMemoryBufferMethod)
{
    return;
}

#endif

/* -------------- stubs for GM ------------- */
#if defined (NO_GM_USED)

unsigned int ppGmInit(unsigned int uiDeviceId,
                      const char* devType,
                      CALLBACKS_SERVICE* pParam,
                      const char* iniFile)
{
    return 0;
}


unsigned int ppPciWrite(unsigned int uiDeviceId,
                        unsigned int uiCoreId,
                        unsigned int uiAddress,
                        unsigned char* data,
                        unsigned int size)
{
    return 0;
}

unsigned int ppPciRead(unsigned int uiDeviceId,
                       unsigned int uiCoreId,
                       unsigned int uiAddress,
                       unsigned char* data,
                       unsigned int size)
{
    return 0;
}


unsigned int ppPciConfigWrite(unsigned int uiDeviceId,
                              unsigned int uiCoreId,
                              unsigned int uiAddress,
                              unsigned char* data,
                              unsigned int size)
{
    return 0;
}

unsigned int ppPciConfigRead(unsigned int uiDeviceId,
                             unsigned int uiCoreId,
                             unsigned int uiAddress,
                             unsigned char* data,
                             unsigned int size)
{
    return 0;
}

unsigned int ppSendPacket(unsigned int uiDeviceId,
                          unsigned int uiCoreId,
                          unsigned int uiPortId,
                          const char*  packetBuffer,
                          unsigned int packetSize)
{
    return 0;
}


unsigned int ppSendCell(unsigned int uiDeviceId,
                        unsigned int uiCoreId,
                        unsigned int uiPortId,
                        const char*  cellBuffer,
                        unsigned int cellSize)
{
    return 0;
}


unsigned int ppSetPortControl(unsigned int uiDeviceId,
                              unsigned int uiCoreId,
                              unsigned int uiPortId,
                              unsigned int isLinkUp)
{
    return 0;
}

unsigned int ppSetErrorCallback(unsigned int cbPtr)
{
    return 0;
}

#endif


