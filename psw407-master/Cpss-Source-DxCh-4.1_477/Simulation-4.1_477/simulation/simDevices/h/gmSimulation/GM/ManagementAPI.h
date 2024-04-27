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
* Public files: ManagementAPI.h
*
* DESCRIPTION:
*       This file contains management interface functions of the Marvell EBU chip
*       Golden Model.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 2 $
*******************************************************************************/
#ifndef _MANAGEMENTAPI_H_
#define _MANAGEMENTAPI_H_

#ifdef __cplusplus
extern "C"
{
#endif

unsigned int ppPciWrite(unsigned int uiDeviceId,
                        unsigned int uiCoreId,
                        unsigned int uiAddress,
                        unsigned char* data,
                        unsigned int size);

unsigned int ppPciRead(unsigned int uiDeviceId,
                       unsigned int uiCoreId,
                       unsigned int uiAddress,
                       unsigned char* data,
                       unsigned int size);

unsigned int ppPciConfigWrite(unsigned int uiDeviceId,
                              unsigned int uiCoreId,
                              unsigned int uiAddress,
                              unsigned char* data,
                              unsigned int size);

unsigned int ppPciConfigRead(unsigned int uiDeviceId,
                             unsigned int uiCoreId,
                             unsigned int uiAddress,
                             unsigned char* data,
                             unsigned int size);

unsigned int ppSetErrorCallback(unsigned int cbPtr);


#ifdef __cplusplus
}
#endif

#endif /*_MANAGEMENTAPI_H_*/
