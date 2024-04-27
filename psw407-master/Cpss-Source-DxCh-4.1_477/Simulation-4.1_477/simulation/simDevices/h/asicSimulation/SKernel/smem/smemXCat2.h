/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* smemXCat2.h
*
* DESCRIPTION:
*       xCat2 memory mapping implementation
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 8 $
*
*******************************************************************************/
#ifndef __smemXCat2h
#define __smemXCat2h

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <asicSimulation/SKernel/smem/smemLion.h>

/*******************************************************************************
 * Typedef: enum SMEM_XCAT2_UNIT_NAME_ENT
 *
 * Description:
 *      Memory units names
 *
 *******************************************************************************/
typedef enum{
    SMEM_XCAT2_UNIT_MG_E,
    SMEM_XCAT2_UNIT_DFX_E,
    SMEM_XCAT2_UNIT_EGR_TXQ_E,
    SMEM_XCAT2_UNIT_L2I_E,
    SMEM_XCAT2_UNIT_UC_ROUTE_E,
    SMEM_XCAT2_UNIT_BM_E,
    SMEM_XCAT2_UNIT_EPLR_E,
    SMEM_XCAT2_UNIT_LMS_E,
    SMEM_XCAT2_UNIT_FDB_E,
    SMEM_XCAT2_UNIT_MPPM_BANK0_E,
    SMEM_XCAT2_UNIT_MPPM_BANK1_E,
    SMEM_XCAT2_UNIT_MEM_E,
    SMEM_XCAT2_UNIT_CENTRALIZED_COUNT_E,
    SMEM_XCAT2_UNIT_STACKING_PORTS_E,
    SMEM_XCAT2_UNIT_UNIPHY_SERDES_E,
    SMEM_XCAT2_UNIT_VLAN_MC_E,
    SMEM_XCAT2_UNIT_NETWORK_PORTS_E,
    SMEM_XCAT2_UNIT_EQ_E,
    SMEM_XCAT2_UNIT_IPCL_E,
    SMEM_XCAT2_UNIT_TTI_E,
    SMEM_XCAT2_UNIT_IPLR0_E,
    SMEM_XCAT2_UNIT_IPLR1_E,
    SMEM_XCAT2_UNIT_TCC_LOWER_E,
    SMEM_XCAT2_UNIT_TCC_UPPER_E,
    SMEM_XCAT2_UNIT_HA_E,
    SMEM_XCAT2_UNIT_EPCL_E,
    SMEM_XCAT2_UNIT_LAST_E
}SMEM_XCAT2_UNIT_NAME_ENT;


/*******************************************************************************
*   smemXCat2Init2
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
void smemXCat2Init2
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr
);

/*******************************************************************************
*   smemXCat2Init
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
void smemXCat2Init
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
);

/*******************************************************************************
*  smemXCat2TableInfoSet
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
void smemXCat2TableInfoSet
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr
);

/*******************************************************************************
* smemXCat2RegsInfoSet
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
void smemXCat2RegsInfoSet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
);

ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemXCat2ActiveWritePolicerMemoryControl);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __smemXCat2h */

