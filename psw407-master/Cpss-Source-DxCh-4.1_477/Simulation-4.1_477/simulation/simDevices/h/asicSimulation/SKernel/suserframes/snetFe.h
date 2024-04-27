/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetFE.h
*
* DESCRIPTION:
*       This is a external API definition for snetFE module of Dune system.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 21 $
*
*******************************************************************************/
#ifndef __snetFeh
#define __snetFeh


#include <asicSimulation/SKernel/smain/smain.h>
#include <common/Utils/FrameInfo/sframeInfoAddr.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* unicast distribution table address */
#define SNET_FE_UNICAST_DIST_TABLE_ENTRY_REG(entry_indx , address ) \
        *address = (0x30003000 + (entry_indx))

/* multicast distribution table address */
#define SNET_FE_MULTICAST_DIST_TABLE_ENTRY_REG(vidx , address ) \
        *address = (0x08002000 + (vidx))

/* connectivity map register */
#define SNET_FE_CONNECTIVITY_MAP_REG(linkId , address ) \
        (linkId < 32) ? (*address = (0x0640 + (linkId * 4))) : \
                        (*address = (0x0740 + (linkId * 4)))

/* received link counter */
#define SNET_FE_RX_CELL_COUNTER_REG(linkId , address)\
        (linkId < 16) ? (*address = 0x0B50 + 4 * linkId ) : \
        (linkId < 32) ? (*address = 0x0C10 + 4 * linkId ) : \
        (linkId < 48) ? (*address = 0x0CD0 + 4 * linkId ) : \
                        (*address = 0x0D90 + 4 * linkId )

/* transmit link counter */
#define SNET_FE_TX_CELL_COUNTER_REG(linkId , address)\
        (linkId < 16) ? (*address = 0x0B00 + 4 * linkId ) : \
        (linkId < 32) ? (*address = 0x0BC0 + 4 * linkId ) : \
        (linkId < 48) ? (*address = 0x0C80 + 4 * linkId ) : \
                        (*address = 0x0D40 + 4 * linkId )

/* is reachability message */
#define SNET_FE_IS_RM_FRAME_TYPE(fieldValue ) \
        (fieldValue == SKERNEL_UPLINK_FRAME_DUNE_RM_E) ? 1 : 0

/* routing processor enabler register */
#define FE_ROUT_PROCESS_ENA_REG    0x800

/* reachability message size */
#define FE_REACHABILITY_MSG_SIZE   0x10

/* mac enabler register */
#define FE_MAC_ENABLER_REG         0x0A00

/* Maximum device ports number */
#define FE_PORTS_NUMBER            (0x40)

/* definition of reachability message type */
#define SNET_FE_IS_RM_FRAME_TY(fieldValue ) \
        (*fieldValue == SKERNEL_UPLINK_FRAME_DUNE_RM_E) ? 1 : 0

/*
    FE cell counter mode
    SNET_FE_MAC_COUNT_ALL_MODE - Count all cells.
    SNET_FE_MAC_COUNT_CTRL_CELL_ONLY_MODE - Count control cells only.
    SNET_FAP_LBP_COUNT_SPEC_MODE_STATE  - Count data cells only.
*/
typedef enum
{
    SNET_FE_MAC_COUNT_ALL_MODE = 0,
    SNET_FE_MAC_COUNT_CTRL_CELL_ONLY_MODE,
    SNET_FE_MAC_COUNT_DATA_CELL_MODE,
}SFE_MAC_COUNTER_MODE_TYPE_ENT;

/*******************************************************************************
*   snetFeProcessInit
*
* DESCRIPTION:
*       Init module of the FE networking algorithm.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
void snetFeProcessInit
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
);

/*******************************************************************************
*   snetFeFrameProcess
*
* DESCRIPTION:
*       Process the frame, get and do actions for a frame
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       bufferId     - frame data buffer Id
*       srcPort      - source port number
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
void snetFeFrameProcess
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SBUF_BUF_ID bufferId,
    IN GT_U32 srcPort
);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __snetFeh */


