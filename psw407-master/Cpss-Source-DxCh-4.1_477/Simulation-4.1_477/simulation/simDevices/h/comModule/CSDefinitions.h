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
* Public files: CSDefinitions.h
*
* DESCRIPTION:
*       This file contains defines and structures for the Marvell EBU chip
*       Communication Model.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 2 $
*******************************************************************************/

#ifndef _CSDEFINITIONS_
#define _CSDEFINITIONS_

#include <os/simTypes.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef void (*ComServicesAdapterEventCallBack)
(
    IN unsigned long adapterId
);


typedef enum _ProtocolId_
{
    ProtocolRS232 = 1,
    ProtocolI2C = 2,
    ProtocolSMI = 3,
    ProtocolLOOPBACK = 4,
    ProtocolETHERNET = 5
} ProtocolId;

typedef enum _AdapterTypeId_
{
    AdpNone = 0,
    AdpSerial = 1,
    AdpLPTTOI2C = 2,
    AdpLPTTOI2CSE = 3,
    AdpUSBTOSMI = 4,
    AdpUSBTOI2C = 5,
    AdpMRVSMI = 6,
    AdpMRVI2C = 7,
    AdpLOOPBACK = 8,
    AdpEthernet = 9
} AdapterTypeId;

typedef enum _AdaptorStatus_
{
    DISABLED = 1,       /* disconnected */
    UNLOCKED,           /*  connected and unlocked ==> available */
    LOCKED_BY_OTHER,    /*  connected and locked by other client */
    LOCKED_BY_ME        /*  connected and locked by this client ==> available */
} AdaptorStatus;


#define ADP_DESC_NAME_LENGTH    (32)
#define ADP_DESC_FULL_LENGTH    (ADP_DESC_NAME_LENGTH + 4*sizeof(int))

typedef struct _AdpDesc_
{
    unsigned long _AdapterId;
    AdapterTypeId _TypeId;
    ProtocolId    _ProtocolId;
    AdaptorStatus _Status;
    unsigned char _Name[ADP_DESC_NAME_LENGTH];
} AdpDesc;

/* Error codes used by ComServicesServer */

#define RES_OK                                  (0)
#define RES_ERROR                               (1)
#define RES_YES                                 (2)
#define RES_NO                                  (3)
#define RES_ERR_CLIENT_NOT_REGISTERED           (4)
#define RES_ERR_BAD_CLIENT_ID                   (5)
#define RES_ERR_BAD_ADAPTER_ID                  (6)
#define RES_ERR_ADAPTER_DISABLED                (7)
#define RES_ERR_ADAPTER_LOCKED_BY_OTHER_CLIENT  (8)
#define RES_ERR_ADAPTER_NOT_LOCKED_GLOBALLY     (9)
#define RES_ERR_ADAPTER_NOT_LOCKED_FOR_TRANS    (10)
#define RES_ERR_CLIENT_NOT_CONNECTED_TO_ADAPTER (11)
#define RES_ERR_CLIENT_CONNECTED_TO_ADAPTER_WHILE_SET_PARAMS (12)
#define RES_ERR_CLIENT_ALREADY_CONNECTED_TO_ADAPTER (13)
#define RES_ERR_ADAPTER_PROTOCOL_MISMATCH       (14)

/*  Error codes used by ComServicesCppLib */

#define ERR_REGISTRATION_FAILED         (-1)
#define ERR_CLIENTIMPL_IS_NULL          (-2)
#define ERR_NULL_POINTER_PASSED         (-3)
#define ERR_CLIENT_ALREADY_REGISTERED   (-4)

#ifdef __cplusplus
}
#endif

#endif
