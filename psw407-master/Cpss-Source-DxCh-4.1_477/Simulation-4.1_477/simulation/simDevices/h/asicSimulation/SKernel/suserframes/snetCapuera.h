/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetCapoeira.h
*
* DESCRIPTION:
*       This is a external API definition for Sbet Capoeira module of SKernel.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 3$
*
*******************************************************************************/
#ifndef __snetCapoeirah
#define __snetCapoeirah


#include <asicSimulation/SKernel/smain/smain.h>
#include <common/Utils/FrameInfo/sframeInfoAddr.h>
#include <asicSimulation/SKernel/smain/smainSwitchFabric.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define CAP_PORT_ENABLE_REGISTER_CNS                 (0x00000014)
#define CAP_GLOB_CONGIF_REG_CNS                      (0x00000000)
#define CAP_PG0_MC_TABLE_CNS                         (0x00840000)
#define CAP_GLOB_INTERUPT_CAUSE_REG_CNS              (0x00000030)
#define CAP_GLOB_INTERUPT_MASK_REG_CNS               (0x00000034)
#define CAP_RX_CELL_PEND_INTERUPT_MASK_REG_CNS       (0x00000208)
#define CAP_RX_CELL_PEND_INTERUPT_CAUSE_REG_CNS      (0x00000204)


/*******************************************************************************
*   snetCapoeiraProcessInit
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
void snetCapoeiraProcessInit
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr
);

/*******************************************************************************
*   snetCapoeiraProcessFrameFromSLAN
*
* DESCRIPTION:
*       Process the frame, get and do actions for a frame
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       bufferId     - frame data buffer Id
*       srcPort      - source port number
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
void snetCapoeiraProcessFrameFromSLAN
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SBUF_BUF_ID bufferId,
    IN GT_U32 srcPort
);

/*******************************************************************************
*   snetCapoeiraProcessFrameFromUplink
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
*      The function is used by the FA for processing uc, mc or bc frame from pp
*
*******************************************************************************/
void snetCapoeiraProcessFrameFromUplink
(
    IN SKERNEL_DEVICE_OBJECT      * devObjPtr,
    IN SKERNEL_UPLINK_DESC_STC    * descrPtr
);



/*******************************************************************************
*   snetCapoeiraMessageProcess
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
GT_VOID snetCapoeiraMessageProcess
(
    IN SKERNEL_DEVICE_OBJECT      *deviceObj_PTR,
    IN GT_U8                      *dataPtr
);


/*******************************************************************************
*   snetCapoeiraCpuMessageProcess
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
GT_VOID snetCapoeiraCpuMessageProcess
(
    IN SKERNEL_DEVICE_OBJECT      *deviceObj_PTR,
    IN GT_U8                      *dataPtr
);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __snetCapoeirah */


