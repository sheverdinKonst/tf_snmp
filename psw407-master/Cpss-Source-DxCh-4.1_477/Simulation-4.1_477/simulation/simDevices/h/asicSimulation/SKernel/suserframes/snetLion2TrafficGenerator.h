/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetLion2TrafficGenerator.h
*
* DESCRIPTION:
*       This is a external API definition for Lion2 traffic generator module.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 2 $
*
*******************************************************************************/
#ifndef __snetLion2TrafficGeneratorh
#define __snetLion2TrafficGeneratorh

#include <asicSimulation/SKernel/smain/smain.h>
#include <asicSimulation/SKernel/skernel.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* enumeration that hold the types of TG modes */
typedef enum
{
    SNET_LION2_TG_MODE_INCREMENT_MAC_DA_E,
    SNET_LION2_TG_MODE_RANDOM_DATA_E,
    SNET_LION2_TG_MODE_RANDOM_LENGTH_E,
} SNET_LION2_TG_MODE_ENT;

/*******************************************************************************
*   snetLion2TgPacketGenerator
*
* DESCRIPTION:
*       Packet generator main process.
*
* INPUTS:
*       tgDataPtr  - pointer to traffic generator data.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
GT_VOID snetLion2TgPacketGenerator
(
    IN SKERNEL_TRAFFIC_GEN_STC * tgDataPtr
);

/*******************************************************************************
*   snetLion3SdmaTaskPerQueue
*
* DESCRIPTION:
*       SDMA Message Transmission task.
*
* INPUTS:
*       sdmaTxDataPtr  - Pointer to Tx SDMA data
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
GT_VOID snetLion3SdmaTaskPerQueue
(
    SKERNEL_SDMA_TRANSMIT_DATA_STC  *sdmaTxDataPtr
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __snetLion2TrafficGeneratorh */


