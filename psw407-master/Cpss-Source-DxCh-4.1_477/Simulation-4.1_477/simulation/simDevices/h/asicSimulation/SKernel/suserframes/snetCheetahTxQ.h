/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetCheetahTxQ.h
*
* DESCRIPTION:
*       This is a header file for TxQ processing
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 10 $
*
*******************************************************************************/
#ifndef __snetCheetahTxQh
#define __snetCheetahTxQh

#include <asicSimulation/SKernel/smain/smain.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/*******************************************************************************
*   simTxqPrintDebugInfo
*
* DESCRIPTION:
*        Print info for given port and tc
*
* INPUTS:
*        devObjPtr   - pointer to device object.
*        port        - port num
*        tc          - traffic class
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
GT_VOID simTxqPrintDebugInfo
(
    IN SKERNEL_DEVICE_OBJECT   *devObjPtr,
    IN GT_U32                   port,
    IN GT_U32                   tc
);

/*******************************************************************************
*   snetChtTxQPhase2
*
* DESCRIPTION:
*        Analyse destPorts for disabled TC
*
* INPUTS:
*        devObjPtr   - pointer to device object.
*        descrPtr    - pointer to the frame's descriptor.
*        destPorts   - egress ports list
*
* OUTPUTS:
*        destPorts   - egress ports list
*
* RETURNS:
*        outPorts    - number of outgoing ports
*
* COMMENTS:
*
*   Egress ports with (disabled TC) == (packet TC for egress port)
*   need to be removed from destPorts[portInx].
*   All other ports where disabled TC != packet TC
*   need to be handled in the snetChtEgressDev.
*
*******************************************************************************/
GT_U32 snetChtTxQPhase2
(
    IN SKERNEL_DEVICE_OBJECT           *devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC *descrPtr,
    INOUT GT_U32                        destPorts[],
    IN GT_U8                            destVlanTagged[]
);

/*******************************************************************************
*  smemTxqSendDequeueMessages
*
* DESCRIPTION:
*       Send dequeue messages
*
* INPUTS:
*       devObjPtr     - device object PTR.
*       regValue      - register value.
*       port          - port num
*       startBitToSum - start bit to read
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
GT_VOID smemTxqSendDequeueMessages
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32  regValue,
    IN GT_U32  port,
    IN GT_U32  startBitToSum
);

/*******************************************************************************
* snetHaTablesFormatInit
*
* DESCRIPTION:
*        init the format of HA tables.
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*
* OUTPUTS:
*       None.
*
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
void snetHaTablesFormatInit(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __snetCheetahTxQh */


