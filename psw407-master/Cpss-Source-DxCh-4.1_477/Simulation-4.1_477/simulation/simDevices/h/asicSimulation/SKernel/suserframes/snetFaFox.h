/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetFaFox.h
*
* DESCRIPTION:
*       This is a external API definition for SMem module of SKernel.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 8 $
*
*******************************************************************************/
#ifndef __snetFalFoxh
#define __snetFalFoxh


#include <asicSimulation/SKernel/smain/smain.h>
#include <asicSimulation/SKernel/smain/smainSwitchFabric.h>
#include <common/Utils/FrameInfo/sframeInfoAddr.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define FAFOX_PORT_INDEX_ENTRY_NUM_CNS               (0x40)
#define FAFOX_GLOBAL_CONTROL_REGISTER_CNS            (0x42000000)
#define FAFOX_TARGET_DEV_FABRIC_PORT_CONFIG_REG_CNS  (0x42004000)
#define FAFOX_VOQ_GLOB_CONFIG_REG_CNS                (0x40800000)
#define FAFOX_GLOB_CONGIF_REG_CNS                    (0x40000058)
#define FAFOX_LOCAL_SWITCH_REG_CNS                   (0x4200004C)
#define FAFOX_DEVID_FA_MAP_REG_CNS                   (0x40806000)
#define FAFOX_FABRIC_PORT_ENABLE_REG_CNS             (0x40800020)
#define FAFOX_DEVICE_ENABLE_REGISTER_CNS             (0x40800010)
#define FAFOX_MULTI_GROUP_CONFIG_REG_CNS             (0x42040000)
#define FA_REASSEMBLY_CAUSE_INTER_MASK_REG           (0x41000030)
#define FA_REASSEMBLY_CAUSE_INTER_REG                (0x41000034)
#define FA_GLOBAL_INT_MASK_REG                       (0x40000084)
#define FA_GLOBAL_INT_CAUSE_REG                      (0x40000080)
#define FA_HYPERGLINK_INT_MASK_REG                   (0x4208000C)
#define FA_HYPERGLINK_INT_CAUSE_REG                  (0x42080008)
#define FA_PING_CELL_RX_REG                          (0x4208001C)
#define FA_CPU_MAIL_READ_REG                         (0x41004000)
#define FA_PING_CELL_TX_REG                          (0x42080018)
#define FA_CPU_MAIL_BOX_REG                          (0x41900000)
#define FA_MG_CELLS_TARGET_REG                       (0x42108058)

/* counters */
#define FAFOX_MAIL_RCV_COUNTER_REG                   (0x4100202C)
#define FAFOX_MAIL_DRP_COUNTER_REG                   (0x4100203C)
#define FSFOX_DEV_ENABLE_DRP_COUNTER_REG             (0x40800230)

/*******************************************************************************
*   snetFaFoxProcessInit
*
* DESCRIPTION:
*       Init module.
*
* INPUTS:
*       deviceObjPtr   - pointer to device object.
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
void snetFaFoxProcessInit
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr
);

/*******************************************************************************
*   snetFaFoxProcessFrameFromSLAN
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
*      GT_OK - when the data is sent successfully.
*      GT_FAIL - in other case.
*
* COMMENTS:
*      The function is used by the FA and by the PP.
*
*******************************************************************************/
void snetFaFoxProcessFrameFromSLAN
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SBUF_BUF_ID bufferId,
    IN GT_U32 srcPort
);

/*******************************************************************************
*   snetFaProcessPacketFromUplink
*
* DESCRIPTION:
*       Process frame that was received on the uplink . The frame was sent from
*       pp to the fa.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - pointer to the frame's descriptor.
* OUTPUTS:
*       None.
*
* RETURNS:
*      GT_OK - when the data is sent successfully.
*      GT_FAIL - in other case.
*
* COMMENTS:
*      The function is used by the FA for processing uc , mc or bc frame from pp
*
*******************************************************************************/
void snetFaFoxProcessFrameFromUplink
(
    IN SKERNEL_DEVICE_OBJECT      * devObjPtr,
    IN SKERNEL_UPLINK_DESC_STC    * descrPtr
);



/*******************************************************************************
*   snetFoxCpuMessageProcess
*
* DESCRIPTION:
*       Process message that was sent from CPU .
*
* INPUTS:
*       deviceObj_PTR   - pointer to device object.
*       descr_PTR       - pointer to buffer of message
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*      The message can be pcs ping or cpu mail box .
*******************************************************************************/
GT_VOID snetFoxCpuMessageProcess
(
    IN SKERNEL_DEVICE_OBJECT      *deviceObj_PTR,
    IN GT_U8                      *dataPtr
);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __snetFalFoxh */


