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
* Public files: TrafficAPI.h
*
* DESCRIPTION:
*       This file contains traffic interface functions of the Marvell EBU chip
*       Golden Model.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 1 $
*******************************************************************************/
#ifndef _GMTRAFFICAPI_H_
#define _GMTRAFFICAPI_H_

#ifdef __cplusplus
extern "C"
{
#endif

/* API definitions */
unsigned int ppSendPacket(unsigned int uiDeviceId,
                          unsigned int uiCoreId,
                          unsigned int uiPortId,
                          const char*  packetBuffer,
                          unsigned int packetSize);

unsigned int ppSendCell(unsigned int uiDeviceId,
                        unsigned int uiCoreId,
                        unsigned int uiPortId,
                        const char*  cellBuffer,
                        unsigned int cellSize);

unsigned int ppSetPortControl(unsigned int uiDeviceId,
                              unsigned int uiCoreId,
                              unsigned int uiPortId,
                              unsigned int isLinkUp);


#ifdef __cplusplus
}
#endif

#endif /*_GMTRAFFICAPI_H_*/
