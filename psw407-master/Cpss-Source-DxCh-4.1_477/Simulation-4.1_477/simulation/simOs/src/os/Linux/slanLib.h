/*******************************************************************************
*              (c), Copyright 2008, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* slanLib.h
*
* DESCRIPTION:
*       SLAN library for UNIX-like OSes
*
* DEPENDENCIES:
*       
* FILE REVISION NUMBER:
*       $Revision 3 $
*
*******************************************************************************/
#ifndef __slanLib_h__
#define __slanLib_h__

#ifndef IN
#   define IN
#endif
#ifndef OUT
#   define OUT
#endif

/************ Defines  ********************************************************/
#define SLAN_CONNECTOR_ADDRESS "127.0.0.1"
#define SLAN_CONNECTOR_PORT    "34567"


/************ Protocol definitions ********************************************/
/* packet + header */
#define SLAN_INPUT_BUFFER_SIZE  12228
/* match only first 64 bytes of SLAN name */
#define SLAN_NAME_LEN           64

#define SLAN_PKT_HDR_SIZE       4
#define SLAN_LIB_UNPACK_HEADER(buffer,length,type) \
    do { \
        length = ((((unsigned char*)(buffer))[0] << 8) | ((unsigned char*)(buffer))[1]); \
        type = (SLAN_LIB_EVENT_TYPE)((((unsigned char*)(buffer))[2] << 8) | ((unsigned char*)(buffer))[3]); \
    } while (0)
#define SLAN_LIB_PACK_HEADER(buffer,length,type) \
    do { \
        (buffer)[0] = ((length) >> 8) & 0x0ff; \
        (buffer)[1] = (length) & 0x0ff; \
        (buffer)[2] = ((type) >> 8) & 0x0ff; \
        (buffer)[3] = (type) & 0x0ff; \
    } while (0)

/************* Typedefs *******************************************************/
typedef int SLAN_ID;

typedef enum {
    SLAN_LIB_EV_PACKET,
    SLAN_LIB_EV_LINKUP,
    SLAN_LIB_EV_LINKDOWN,
    SLAN_LIB_EV_CLOSED,
    SLAN_LIB_EV_HANDSHAKE,
    SLAN_LIB_EV_HANDSHAKE_SUCCESS,
    SLAN_LIB_EV_HANDSHAKE_FAILED,
} SLAN_LIB_EVENT_TYPE;

typedef enum {
    SLAN_LIB_LINK_UP,
    SLAN_LIB_LINK_DOWN
} SLAN_LIB_LINK_STATUS_TYPE;


/*******************************************************************************
* SLAN_LIB_EVENT_HANDLER_FUNC
*
* DESCRIPTION:
*       SLAN Event process function
*
* INPUTS:
*       slanId      - SLAN id
*       userData    - The data pointer passed to slanLibBind()
*       eventType   - event type
*       pktData     - pointer to packet
*       pktLen      - packet length
*
* OUTPUTS:
*       None
*
* RETURNS:
*       None
*
* COMMENTS:
*
*******************************************************************************/
typedef void (*SLAN_LIB_EVENT_HANDLER_FUNC)
(
    IN  SLAN_ID             slanId,
    IN  void*               userData,
    IN  SLAN_LIB_EVENT_TYPE eventType,
    IN  char*               pktData,
    IN  int                 pktLen
);


/************* Functions ******************************************************/

/*******************************************************************************
* slanLibInit
*
* DESCRIPTION:
*       Initialize library
*
* INPUTS:
*       slansMax    - Maximum number of SLANS
*       clientName  - Client name for debug tricks in slanConnector
*                     Can be NULL. The values will be
*                     "asic", "smbsim", "ether", etc
*
* OUTPUTS:
*       None
*
* RETURNS:
*       Zero if success
*       Non zero if error occured
*           EALREADY
*           EINVAL
*           ENOMEM
*
* COMMENTS:
*
*******************************************************************************/
int slanLibInit
(
    IN  int slansMax,
    IN  const char *clientName
);

/*******************************************************************************
* slanLibMainLoop
*
* DESCRIPTION:
*       Mail loop function. slanLibInit() doesn't create thread because
*       thread can require to be created by specific API
*       Must be called after slanLinInit()
*
* INPUTS:
*       timeoutSec  - return if no input more than timeoutSec
*                     Zero means wait forewer
*
* OUTPUTS:
*       None
*
* RETURNS:
*       0   if timeout
*       Non zero if error
*
* COMMENTS:
*
*******************************************************************************/
int slanLibMainLoop(int timeoutSec);

/*******************************************************************************
* slanLibBind
*
* DESCRIPTION:
*       This function connects to SLAN Connector and send slanName identifier.
*       Connection is added to slanLibMainLoop() pool
*
* INPUTS:
*       slanName    - SLAN name
*       handler     - event handler function
*       userData    - pointer to user data
*
* OUTPUTS:
*       slanId      - SLAN id
*
* RETURNS:
*       Zero if success
*       Non zero if error occured
*
* COMMENTS:
*       $SLAN_PREFIX environment variable will be added to slanName
*
*******************************************************************************/
int slanLibBind
(
    IN  const char*                     slanName,
    IN  SLAN_LIB_EVENT_HANDLER_FUNC     handler,
    IN  void*                           userData,
    OUT SLAN_ID                         *slanId
);

/*******************************************************************************
* slanLibUnbind
*
* DESCRIPTION:
*       Remove connection from slanLibMainLoop() pool.
*       Then connection to SLAN Connector closed
*
* INPUTS:
*       slanId      - SLAN id
*
* OUTPUTS:
*       None
*
* RETURNS:
*       Zero if success
*       Non zero if error occured
*
* COMMENTS:
*
*******************************************************************************/
int slanLibUnbind
(
    IN  SLAN_ID     slanId
);

/*******************************************************************************
* slanLibGetUserData
*
* DESCRIPTION:
*       Get custom pointer passed to bind function
*
* INPUTS:
*       slanId      - SLAN id
*
* OUTPUTS:
*       None
*
* RETURNS:
*       User data
*       NULL if error occured
*
* COMMENTS:
*
*******************************************************************************/
void* slanLibGetUserData
(
    IN  SLAN_ID     slanId
);

/*******************************************************************************
* slanLibGetSlanName
*
* DESCRIPTION:
*       Get slan name
*
* INPUTS:
*       slanId      - SLAN id
*
* OUTPUTS:
*       None
*
* RETURNS:
*       Slan name
*       NULL if error occured
*
* COMMENTS:
*
*******************************************************************************/
const char* slanLibGetSlanName
(
    IN  SLAN_ID     slanId
);

/*******************************************************************************
* slanLibTransmit
*
* DESCRIPTION:
*       Transmit packet to SLAN
*
* INPUTS:
*       slanId      - SLAN id
*       packetData  - pointer to packet data
*       packetLen   - pointer to packet length
*
* OUTPUTS:
*       None
*
* RETURNS:
*       Number or bytes sent
*       < 0 if error occured
*
* COMMENTS:
*
*******************************************************************************/
int slanLibTransmit
(
    IN  SLAN_ID     slanId,
    IN  const char  *packetData,
    IN  int         packetLen
);

/*******************************************************************************
* slanLibSetLinkStatus
*
* DESCRIPTION:
*       Change SLANs link status
*
* INPUTS:
*       slanId      - SLAN id
*       linkStatus  - new link status
*
* OUTPUTS:
*       None
*
* RETURNS:
*       Number or bytes sent
*       < 0 if error occured
*
* COMMENTS:
*
*******************************************************************************/
int slanLibSetLinkStatus
(
    IN  SLAN_ID                     slanId,
    IN  SLAN_LIB_LINK_STATUS_TYPE   linkStatus
);

#endif /* __slanLib_h__ */
