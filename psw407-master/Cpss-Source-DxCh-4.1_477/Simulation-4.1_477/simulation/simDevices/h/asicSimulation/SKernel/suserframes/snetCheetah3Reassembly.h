/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetCheetah3Reassembly.c
*
* DESCRIPTION:
*       This is the definitions for the Reassembly engine for cheetah
*       module of SKernel.
*
* FILE REVISION NUMBER:
*       $Revision: 1 $
*
*******************************************************************************/
#ifndef __snetCheetah3Reassemblyh
#define __snetCheetah3Reassemblyh


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* number of entries in the CAPWAP reassembly table */
#define SNET_CHT3_CAPWAP_REASSEMBLY_NUM_ENTRIES_CNS     2048


/*******************************************************************************
*   snetCht3ReassemblyCapwap
*
* DESCRIPTION:
*        CAPWAP Reassembly engine
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - pointer to frame data buffer Id
*       keyPtr       - pointer to the key of the reassembly/fragment
*
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_TRUE  - when this frame complete the assembly of waiting frames
*       GT_FALSE - when this frame is not the last needed fragment
*
* COMMENTS:
*
*
*******************************************************************************/
GT_BOOL snetCht3ReassemblyCapwap
(
    IN SKERNEL_DEVICE_OBJECT             *  devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC   *  descrPtr,
    IN SKERNEL_CAPWAP_REASSEMBLY_KEY_STC *  keyPtr
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __snetCheetah3Reassemblyh */
