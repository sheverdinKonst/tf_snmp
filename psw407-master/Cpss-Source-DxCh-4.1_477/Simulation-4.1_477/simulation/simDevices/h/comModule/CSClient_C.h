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
* Public files: CSClient.h
*
* DESCRIPTION:
*       This file contains interface functions for the Marvell EBU chip
*       Communication Model.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 2 $
*******************************************************************************/
#ifndef _CSCLIENT_C_H_
#define _CSCLIENT_C_H_

#include "CSDefinitions.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /* ------ Client Registration ------ */

    int  CS_RegisterClient(const char* sClientName);
    void CS_UnregisterClient();


    /* ------ Set ComServices Events Callback  ------ */

    void CS_SetEventCallbackAdapterStatusChanged(ComServicesAdapterEventCallBack callback);
    void CS_SetEventCallbackAdapterConfigChanged(ComServicesAdapterEventCallBack callback);


    /* ------ Connect & Disconnect To/From Adapter ------ */

    int  CS_ConnectToAdapter(unsigned long adapterId);
    int  CS_DisconnectFromAdapter(unsigned long adapterId);

    /* ------ Detected Adapters Description and Status ------ */

    int  CS_GetAdapterStatus(unsigned long adapterId, int* pStatus);
    void CS_DetectAdapters(void);
    int  CS_GetDetectedAdaptersDescription(int* pNumOfAdapters, AdpDesc** ppAdpDesc);

    /* ------ Lock & Unlock Adapters for individual usage ------ */

    int  CS_LockAdapter(unsigned long adapterId);
    int  CS_UnlockAdapter(unsigned long adapterId);

    /* ------ Transaction ------ */

    int  CS_StartTransaction( unsigned long adapterId);
    int  CS_StopTransaction( unsigned long adapterId);

    /* ------ SMI ------ */

    int  CS_WriteSMI( unsigned long adapterId,  unsigned long phyAddress,  unsigned long offset,  unsigned long data);
    int  CS_ReadSMI( unsigned long adapterId,  unsigned long phyAddress,  unsigned long offset, unsigned long* pData);

    int  CS_SetSMIConfigParams( unsigned long adapterId,  unsigned long frequency,  unsigned char bSlowMode);
    int  CS_GetSMIConfigParams( unsigned long adapterId, unsigned long* pFrequency, unsigned char* pbSlowMode);

    /* ------ I2C ------ */

    int  CS_WriteI2C(unsigned long adapterId,  char addr,  unsigned short nBytes, unsigned char* data,  unsigned char sendStop);
    int  CS_ReadI2C( unsigned long adapterId,  char addr,  unsigned short nBytes, unsigned char** data,  unsigned char sendStop);

    int  CS_SetI2CConfigParams( unsigned long adapterId, unsigned long frequency);
    int  CS_GetI2CConfigParams( unsigned long adapterId, unsigned long* pFrequency);

    /* ------ Serial ------ */

    int  CS_WriteSerial( unsigned long adapterId, unsigned char* sData);
    int  CS_ReadSerial( unsigned long adapterId, unsigned char** sData);

    int  CS_SetSerialConfigParams( unsigned long adapterId, unsigned long maxSpeed,  unsigned long parity,  unsigned long dataBits,  float stopBits);
    int  CS_GetSerialConfigParams( unsigned long adapterId, unsigned long* pMaxSpeed, unsigned long* pParity, unsigned long* pDataBits, float* pStopBits);

    /* ------ Loopback ------ */

    int  CS_WriteLoopback( unsigned long adapterId, unsigned char* sData);
    int  CS_ReadLoopback( unsigned long adapterId, unsigned char** sData);

    int  CS_SetLoopbackConfigParams( unsigned long adapterId, unsigned long speed);
    int  CS_GetLoopbackConfigParams( unsigned long adapterId, unsigned long* pSpeed);

    /* ------ Ethernet ------ */

    int  CS_ClientWriteEthernet( unsigned long adapterId, unsigned char* sData);
    int  CS_ClientReadEthernet( unsigned long adapterId, unsigned char** sData);

    int  CS_ClientSetEthernetConfigParams( unsigned long adapterId,  unsigned char* sIpAddress,  unsigned long portNumber,  unsigned long protocol);
    int  CS_ClientGetEthernetConfigParams( unsigned long adapterId, unsigned char** sIpAddress, unsigned long* pPortNumber, unsigned long* pProtocol);

#ifdef __cplusplus
}
#endif

#endif
