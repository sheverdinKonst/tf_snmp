/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* smemXCat3.h
*
* DESCRIPTION:
*       xCat3 memory mapping implementation
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 7 $
*
*******************************************************************************/
#ifndef __smemXCat3h
#define __smemXCat3h

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <asicSimulation/SKernel/smem/smemLion.h>

/*******************************************************************************
 * Typedef: enum SMEM_XCAT3_UNIT_NAME_ENT
 *
 * Description:
 *      Memory units names
 *
 *******************************************************************************/
typedef enum{
    SMEM_XCAT3_UNIT_MG_E,
    SMEM_XCAT3_UNIT_EGR_TXQ_E,
    SMEM_XCAT3_UNIT_L2I_E,
    SMEM_XCAT3_UNIT_IPVX_E,
    SMEM_XCAT3_UNIT_BM_E,
    SMEM_XCAT3_UNIT_EPLR_E,
    SMEM_XCAT3_UNIT_LMS_E,
    SMEM_XCAT3_UNIT_FDB_E,
    SMEM_XCAT3_UNIT_MPPM_BANK0_E,
    SMEM_XCAT3_UNIT_MPPM_BANK1_E,
    SMEM_XCAT3_UNIT_MEM_E,
    SMEM_XCAT3_UNIT_CENTRALIZED_COUNT_E,
    SMEM_XCAT3_UNIT_MSM_E,
    SMEM_XCAT3_UNIT_GOP_E,
    SMEM_XCAT3_UNIT_SERDES_E,
    SMEM_XCAT3_UNIT_VLAN_MC_E,
    SMEM_XCAT3_UNIT_EQ_E,
    SMEM_XCAT3_UNIT_IPCL_E,
    SMEM_XCAT3_UNIT_TTI_E,
    SMEM_XCAT3_UNIT_IPLR0_E,
    SMEM_XCAT3_UNIT_IPLR1_E,
    SMEM_XCAT3_UNIT_MLL_E,
    SMEM_XCAT3_UNIT_TCC_LOWER_E,
    SMEM_XCAT3_UNIT_TCC_UPPER_E,
    SMEM_XCAT3_UNIT_HA_E,
    SMEM_XCAT3_UNIT_EPCL_E,
    SMEM_XCAT3_UNIT_CCFC_E,
    SMEM_XCAT3_UNIT_LAST_E
}SMEM_XCAT3_UNIT_NAME_ENT;

/*******************************************************************************
*   smemXCat3Init2
*
* DESCRIPTION:
*       Init memory module for a device - after the load of the default
*           registers file
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
void smemXCat3Init2
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr
);

/*******************************************************************************
*   smemXCat3Init
*
* DESCRIPTION:
*       Init memory module for the xCat2 device.
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
void smemXCat3Init
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
);

/*******************************************************************************
*  smemXCat3TableInfoSet
*
* DESCRIPTION:
*       set the table info for the device --> fill devObjPtr->tablesInfo
*
* INPUTS:
*       devObjPtr   - device object PTR.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
void smemXCat3TableInfoSet
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr
);

/*******************************************************************************
*  smemXCat3TableInfoSetPart2
*
* DESCRIPTION:
*       set the table info for the device --> fill devObjPtr->tablesInfo
*       AFTER the bound of memories (after calling smemBindTablesToMemories)
*
* INPUTS:
*       devObjPtr   - device object PTR.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
void smemXCat3TableInfoSetPart2
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr
);

/*******************************************************************************
* smemXCat3RegsInfoSet
*
* DESCRIPTION:
*       Init memory module for xCat2 and above devices.
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
void smemXCat3RegsInfoSet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __smemXCat3h */

