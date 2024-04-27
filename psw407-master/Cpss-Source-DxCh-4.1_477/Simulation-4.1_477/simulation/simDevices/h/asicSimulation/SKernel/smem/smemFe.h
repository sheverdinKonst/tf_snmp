/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* smemFe.h
*
* DESCRIPTION:
*       Data definitions for the Dune Fabric Element memory.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 1 $
*
*******************************************************************************/
#ifndef __smemFeh
#define __smemFeh

#include <asicSimulation/SKernel/smem/smem.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/***********************************************************************/
/*
 * Typedef: struct FE200_GLOBAL_MEM
 *
 * Description:
 *      Global Registers,
 *
 * Fields:
 *      feRange1RegNum    : Fabric(1) Element Registers number.
 *      feRange1Reg       : Fabric(1) Element Register array with the size of
 *                           feRange1RegNum
 *      feRange2RegNum    : Fabric(2) Element Registers number.
 *      feRange2Reg       : Fabric(2) Element Register array with the size of
 *                           feRange2RegNum
 * Comments:
 *
 *  feRange1Reg    - Registers with address mask 0xFFFFF000 pattern 0x00000000
 *                      globRegNum = 1024  (Range : 0x0000 - 0x0FFF).
 *  feRange2Reg    - Registers with address mask 0x3FFF0000 pattern 0x30000000
 *                      globRegNum = 16384  (Range : 0x30000000 - 0x3000FFFF).
 *
 */

typedef struct {
    GT_U32                  feRange1RegNum;
    SMEM_REGISTER         * feRange1Reg;
    GT_U32                  feRange2RegNum;
    SMEM_REGISTER         * feRange2Reg;
}FE200_CONFIG_MEM;


/*
 * Typedef: struct SMEM_FE_DEV_MEM_INFO
 *
 * Description:
 *      Describe a device's memory object in the simulation.
 *
 * Fields:
 *      memMutex        : Memory mutex for device's memory.
 *      specFunTbl      : Address type specific R/W functions.
 *      feMemRange      : fabric element configuration registers.
 *
 * Comments:
 */
typedef struct {
    FE200_CONFIG_MEM                feConfigMem;
}SMEM_FE_DEV_MEM_INFO;

/*******************************************************************************
*   smemFeInit
*
* DESCRIPTION:
*       Init memory module for a FE device.
*
* INPUTS:
*       deviceObj   - pointer to device object.
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
void smemFeInit
(
    IN SKERNEL_DEVICE_OBJECT * deviceObj
);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __smemfeh */


