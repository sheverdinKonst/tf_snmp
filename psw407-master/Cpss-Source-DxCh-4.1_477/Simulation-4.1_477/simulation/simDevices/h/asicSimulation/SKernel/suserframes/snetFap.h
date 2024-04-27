/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetFAP.h
*
* DESCRIPTION:
*       This is a external API definition for snetFAP module of Dune system.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 5 $
*
*******************************************************************************/
#ifndef __snetFaph
#define __snetFaph


#include <asicSimulation/SKernel/smain/smain.h>
#include <asicSimulation/SKernel/smain/smainSwitchFabric.h>
#include <common/Utils/FrameInfo/sframeInfoAddr.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* unicast distribution table address */
#define SNET_FAP_UNICAST_DIST_TABLE_ENTRY_REG(entry_indx , address ) \
        *address = (0x08003000 + (entry_indx))

/* multicast distribution table address */
#define SNET_FAP_MULTICAST_DIST_TABLE_ENTRY_REG(vidx , address ) \
        *address = (0x08002000 + (vidx))

/* definition of reachability message type */
#define SNET_FAP_IS_RM_FRAME_TYPE(fieldValue ) \
        (fieldValue == SKERNEL_UPLINK_FRAME_DUNE_RM_E) ? 1 : 0

/* definition of connectivity map */
#define SNET_FAP_CONNECTIVITY_MAP_REG(linkId , address ) \
        *address = (0x40809A00 + (linkId * 4))

/* FAP global control register */
#define FAP_GLOBAL_CONTROL_REG              0x40000058

/* FAP routing processor register */
#define FAP_ROUT_PROCESS_ENA_REG            0x40808800

/* FAP label processor enabler register */
#define FAP_LBP_ENABLERS_ENA_REG            0x408057FC

/* FAP label processor packet counter register */
#define FAP_LBP_PACKET_COUNTER_REG          0x40807C04

/* FAP queue controller enable register */
#define FAP_QUEUE_CONTROLLER_ENA_REG        0x40808004

/* FAP tx data cell register */
#define FAP_TX_DATA_CELL_CNT_REG            0x40809400

/* FAP cell counter configuration register */
#define FAP_CELL_COUNTER_CONFIG_REG         0x4080A800

/* FAP reachability message size */
#define FAP_REACHABILITY_MSG_SIZE           0x10

/* FAP Maximum device ports number */
#define FAP_PORTS_NUMBER                    (0x8)

/*
    FAP label processing(LBP) counter type
    SNET_FAP_LBP_COUNT_ALL_STATE - Count all packets.
    SNET_FAP_LBP_COUNT_SPEC_QUEUE_STATE - Count packets on specific queue.
    SNET_FAP_LBP_COUNT_SPEC_MODE_STATE  - Count packets on specific mode
                                            (direct,unicast,multicast).
    SNET_FAP_LBP_COUNT_SPEC_MODE_AND_QUEUE_STATE - Count packets on specific
                                                    mode and queue
*/
typedef enum
{
    SNET_FAP_LBP_COUNT_ALL_STATE = 0,
    SNET_FAP_LBP_COUNT_SPEC_QUEUE_STATE,
    SNET_FAP_LBP_COUNT_SPEC_MODE_STATE,
    SNET_FAP_LBP_COUNT_SPEC_MODE_AND_QUEUE_STATE
}SFAP_LBP_COUNTER_MODE_TYPE_ENT;

/*
    FAP label processing(LBP) counter mode
    SNET_FAP_LBP_UNICAST_COUNTER_ENT - LBP counts only unicast packets.
    SNET_FAP_LBP_MULTICAST_COUNTER_ENT - LBP counts only multicast packets.
    SNET_FAP_LBP_RECYCLED_MULTICAST_COUNTER_ENT  - LBP counts only multicast packets.
    SNET_FAP_LBP_DIRECT_PACKET_COUNTER_ENT - LBP counts only direct counter.
*/
typedef enum
{
    SNET_FAP_LBP_UNICAST_COUNTER_ENT =0 ,
    SNET_FAP_LBP_MULTICAST_COUNTER_ENT ,
    SNET_FAP_LBP_RECYCLED_MULTICAST_COUNTER_ENT ,
    SNET_FAP_LBP_DIRECT_PACKET_COUNTER_ENT
}SFAP_LBP_PACKET_COUNTER_MODE_TYPE_ENT;

/*
    FAP cell counter mode
    SNET_CELL_DATA_CONTROL_COUNTER_ENT - count data and control cells.
    SNET_CELL_CONTROL_COUNTER_ENT      - count only control cells.
    SNET_CELL_DATA_COUNTER_ENT         - count only data cells.
*/
typedef enum
{
    SNET_CELL_DATA_CONTROL_COUNTER_ENT =0 ,
    SNET_CELL_CONTROL_COUNTER_ENT  ,
    SNET_CELL_DATA_COUNTER_ENT  ,
}SFAP_CELL_COUNTER_MODE_TYPE_ENT;

/*******************************************************************************
*   snetFapProcessInit
*
* DESCRIPTION:
*       Init module of the FAP networking algorithm.
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
void snetFapProcessInit
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
);

/*******************************************************************************
*   snetFapCpuMessageProcess
*
* DESCRIPTION:
*       Process the frame, get and do actions for a frame
*
* INPUTS:
*       devObjPTR    - pointer to device object.
*       dataPtr      - frame data buffer Id
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
void snetFapCpuMessageProcess
(
    IN SKERNEL_DEVICE_OBJECT      *devObjPTR,
    IN GT_U8                      *dataPtr
);


/*******************************************************************************
*   snetFapProcessFrameFromUpLink
*
* DESCRIPTION:
*
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       uplinkDesc  - pointer to uplink descriptor.
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
void snetFapProcessFrameFromUpLink
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_UPLINK_DESC_STC    * uplinkDesc
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __snetFaph */


