/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* simLogInfoTypeMemory.h
*
* DESCRIPTION:
*       simulation logger memory functions
*
* FILE REVISION NUMBER:
*       $Revision: 5 $
*
*******************************************************************************/
#ifndef __simLogMemory_h__
#define __simLogMemory_h__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <os/simTypes.h>
#include <asicSimulation/SKernel/smain/smain.h>

typedef enum {
      SIM_LOG_MEMORY_READ_E,
      SIM_LOG_MEMORY_WRITE_E,

      /* read write on the PEX/PCI configuration space*/
      SIM_LOG_MEMORY_PCI_READ_E,
      SIM_LOG_MEMORY_PCI_WRITE_E,

      /* read write on the DFX configuration space*/
      SIM_LOG_MEMORY_DFX_READ_E,
      SIM_LOG_MEMORY_DFX_WRITE_E
}SIM_LOG_MEMORY_ACTION_ENT;

typedef enum {
      SIM_LOG_MEMORY_CPU_E, /* SCIB    */
      SIM_LOG_MEMORY_DEV_E  /* SKERNEL */
}SIM_LOG_MEMORY_SOURCE_ENT;

/*******************************************************************************
* simLogMemory
*
* DESCRIPTION:
*       log memory info
*
* INPUTS:
*       devObjPtr - pointer to device object
*       action    - memory action
*       source    - memory source
*       address   - memory address
*       value     - value
*       oldValue  - old value, relevant only when action is SIM_LOG_MEMORY_WRITE_E
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK        - success
*       GT_BAD_PARAM - on wrong parameters
*       GT_FAIL      - error
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS simLogMemory
(
    IN SKERNEL_DEVICE_OBJECT     const *devObjPtr,
    IN SIM_LOG_MEMORY_ACTION_ENT action,
    IN SIM_LOG_MEMORY_SOURCE_ENT source,
    IN GT_U32                    address,
    IN GT_U32                    value,
    IN GT_U32                    oldValue
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __simLogMemory_h__ */

