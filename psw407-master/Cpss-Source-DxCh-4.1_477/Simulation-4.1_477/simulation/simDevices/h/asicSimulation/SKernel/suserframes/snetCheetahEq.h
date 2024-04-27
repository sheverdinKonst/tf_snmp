/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetCheetahEq.h
*
* DESCRIPTION:
*
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 6 $
*
*******************************************************************************/
#ifndef __snetCheetahEqh
#define __snetCheetahEqh

#include <asicSimulation/SKernel/smain/smain.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum{
    SNET_CHT_EQ_HASH_INDEX_RESOLUTION_INSTANCE_TRUNK_E,
    SNET_CHT_EQ_HASH_INDEX_RESOLUTION_INSTANCE_L2_ECMP_E,
    SNET_CHT_EQ_HASH_INDEX_RESOLUTION_INSTANCE_L3_ECMP_E
}SNET_CHT_EQ_HASH_INDEX_RESOLUTION_INSTANCE_ENT;

/*
 * Typedef: struct SNET_CHT_EQ_INTERNAL_DESC_STC
 *
 * Description:
 *      EQ internal descriptor info.
 *
 * Fields:
 *  sniffTargetIsTrunk - is the sniffer targeted to trunk
 *                      0 - to port (ePort)
 *                      1 - to trunk
 *  sniffTargeTrunkId - the trunk ID of sniffer trunk
 *                      (valid when sniffTargetIsTrunk == 1)
*/
typedef struct {
    GT_BIT      sniffTargetIsTrunk;
    GT_U32      sniffTargeTrunkId;
}SNET_CHT_EQ_INTERNAL_DESC_STC;


/*******************************************************************************
*  snetChtEq
*
* DESCRIPTION:
*        EQ block processing
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - frame data buffer Id
*
* RETURN:
*
*******************************************************************************/
GT_VOID snetChtEq
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
);

/*******************************************************************************
*   snetChtEqTxMirror
*
* DESCRIPTION:
*        Send to Tx Sniffer or To CPU by Egress STC. Called from TxQ unit
* INPUTS:
*        devObjPtr       - pointer to device object.
*        descrPtr        - pointer to the frame's descriptor.
*
* OUTPUTS:
*
* RETURNS:
*
*
*******************************************************************************/
GT_VOID  snetChtEqTxMirror
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
);

/*******************************************************************************
*  snetChtEqDoTargetSniff
*
* DESCRIPTION:
*        Send Rx/Tx sniffed frame to Target Sniffer
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - frame data buffer Id
*       rxSniffDev  - Rx sniffer device
*       rxSniffPort - Rx sniffer port
*
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
GT_VOID snetChtEqDoTargetSniff
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_U32 trgSniffDev,
    IN GT_U32 trgSniffPort
);


/*******************************************************************************
*  snetChtEqDuplicateDescr
*
* DESCRIPTION:
*        Duplicate Cheetah's descriptor
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - frame data buffer Id
*
* RETURN:
*       SKERNEL_FRAME_CHEETAH_DESCR_STC *
*                   - pointer to the duplicated descriptor of the Cheetah
*
*******************************************************************************/
SKERNEL_FRAME_CHEETAH_DESCR_STC * snetChtEqDuplicateDescr
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
);

/*******************************************************************************
*  snetChtEqDoToCpuNoCallToTxq
*
* DESCRIPTION:
*        To CPU frame handling from the egress pipe line - don't call the TXQ.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - frame data buffer Id
*
* RETURN:
*
* COMMENT:
*
*
*******************************************************************************/
extern GT_VOID snetChtEqDoToCpuNoCallToTxq
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
);

/*******************************************************************************
*   snetChtEqHashIndexResolution
*
* DESCRIPTION:
*        EQ/IPvX - Hash Index Resolution (for eArch)
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - pointer to the frame's descriptor.
*       numOfMembers - number of members to select index from.
*           this number may be different for each packet.
*           In ePort ECMP this number specifies the number of ECMP members,
*           in the range 1-64.
*           In Trunk member selection the possible values of #members is 1-8.
*       randomEcmpPathEnable  -      Random ECMP Path Enable
*       instanceType - instance type : trunk/ecmp .
*       selectedEPortPtr  - (pointer to) the primary EPort (for
*           SNET_CHT_EQ_HASH_INDEX_RESOLUTION_INSTANCE_L2_ECMP_E)
*
* OUTPUTS:
*       newHashIndexPtr - pointer to the new hash index to be used by the calling
*           engine.
*           the selected member. In ePort ECMP this is a number in the range 0-63,
*           while in trunk member selection it is a number in the range 0-7
*           NOTE: this value should not modify the descriptor value.
*       selectedDevNumPtr - (pointer to) the selected devNum field
*       selectedEPortPtr  - (pointer to) the selected EPort field
*
* RETURNS:
*
*******************************************************************************/
GT_VOID snetChtEqHashIndexResolution
(
    IN SKERNEL_DEVICE_OBJECT                          *devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC                *descrPtr,
    IN GT_U32                                          numOfMembers,
    IN GT_U32                                          randomEcmpPathEnable,
    OUT GT_U32                                        *newHashIndexPtr,
    IN SNET_CHT_EQ_HASH_INDEX_RESOLUTION_INSTANCE_ENT  instanceType,
    OUT GT_U32                                        *selectedDevNumPtr,
    INOUT GT_U32                                      *selectedEPortPtr
);

/*******************************************************************************
* snetEqTablesFormatInit
*
* DESCRIPTION:
*        init the format of EQ tables.
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
void snetEqTablesFormatInit(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr
);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __snetCheetahEqh */


