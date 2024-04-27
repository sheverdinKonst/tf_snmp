/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* simLogInfoTypeDevice.h
*
* DESCRIPTION:
*       simulation logger device object functions
*
* FILE REVISION NUMBER:
*       $Revision: 2 $
*
*******************************************************************************/
#ifndef __simLogDevice_h__
#define __simLogDevice_h__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <asicSimulation/SKernel/smain/smain.h>
#include <os/simTypes.h>

/*******************************************************************************
* simLogDevDescrPortGroupId
*
* DESCRIPTION:
*       log PortGroupId
*
* INPUTS:
*       descr   - dev descriptor pointer
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       None.
*
* COMMENTS:
*
*******************************************************************************/
GT_VOID simLogDevDescrPortGroupId
(
    IN SKERNEL_DEVICE_OBJECT const *devObjPtr
);

/*******************************************************************************
* simLogDevObjCompare
*
* DESCRIPTION:
*       log changes between saved device object and given
*
* INPUTS:
*       old      - old device object pointer
*       new      - new device object pointer
*       funcName - pointer to function name
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       None.
*
* COMMENTS:
*
*******************************************************************************/
GT_VOID simLogDevObjCompare
(
    IN SKERNEL_DEVICE_OBJECT const *old,
    IN SKERNEL_DEVICE_OBJECT const *new,
    IN GT_CHAR               const *funcName
);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __simLogDevice_h__ */

