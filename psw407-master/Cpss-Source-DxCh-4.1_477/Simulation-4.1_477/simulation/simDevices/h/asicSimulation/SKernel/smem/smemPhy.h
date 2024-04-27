/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* smemPhy.h
*
* DESCRIPTION:
*       Data definitions for PHY memories.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 1 $
*
*******************************************************************************/
#ifndef __smemPhyh
#define __smemPhyh

#include <asicSimulation/SKernel/smem/smem.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* 16 bits offset in the register addr is the page number */
#define PHY_PAGE_OFFSET_CNS     16

/* convert address of PHY register , as described in datasheet to address in simulation.

    regPage - the page of the register
            NOTE: value 0..255 (8bits)
    regAddr - register address as given in the datasheet of the PHY
            NOTE: regAddr is 5 bits value 0..31

    NOTE: use this macro when calling scib / skernel APIs of memory access.
*/
#define PHY_REG_ADDR_MAC(regPage , regAddr) \
    (((regPage) << PHY_PAGE_OFFSET_CNS) | (regAddr))

/* macro to by used for paging , to easily find references */
#define PAGE_MAC(page)  (page)

/* define registers (with page) */
/*
page 0 registers
*/
/* page register */
#define PHY_PAGE_ADDRESS_REG            PHY_REG_ADDR_MAC(PAGE_MAC(0),22)
/*
page 16 registers
*/
#define PHY_LINKCRYPT_READ_REG          PHY_REG_ADDR_MAC(PAGE_MAC(16),0)
#define PHY_LINKCRYPT_WRITE_REG         PHY_REG_ADDR_MAC(PAGE_MAC(16),1)
#define PHY_LINKCRYPT_DATA_LO_REG       PHY_REG_ADDR_MAC(PAGE_MAC(16),2)
#define PHY_LINKCRYPT_DATA_HI_REG       PHY_REG_ADDR_MAC(PAGE_MAC(16),3)

/*
 * Typedef: struct PHY_DEV_MEM_INFO
 *
 * Description:
 *      Describe a device's memory object in the simulation.
 *
 * Fields:
 *
 * Comments:
 */
#define NUM_PAGES_CNS   256

typedef struct {
    SMEM_SPEC_FIND_FUN_ENTRY_STC    specFunTbl[NUM_PAGES_CNS];

    GT_U32                 currentActivePage;/* current active page */

    SMEM_UNIT_CHUNKS_STC   page0;        /*0*/
    SMEM_UNIT_CHUNKS_STC   page1;        /*1*/
    SMEM_UNIT_CHUNKS_STC   page2;        /*2*/
    SMEM_UNIT_CHUNKS_STC   page3;        /*3*/
    SMEM_UNIT_CHUNKS_STC   page4;        /*4*/
    SMEM_UNIT_CHUNKS_STC   page5;        /*5*/
    SMEM_UNIT_CHUNKS_STC   page6;        /*6*/
    SMEM_UNIT_CHUNKS_STC   page7;        /*7*/
    SMEM_UNIT_CHUNKS_STC   page8;        /*8*/
    SMEM_UNIT_CHUNKS_STC   page9;        /*9*/
    SMEM_UNIT_CHUNKS_STC   page12;       /*12*/
    SMEM_UNIT_CHUNKS_STC   page14;       /*14*/
    SMEM_UNIT_CHUNKS_STC   page16;       /*16*/
    SMEM_UNIT_CHUNKS_STC   page18;       /*18*/
    SMEM_UNIT_CHUNKS_STC   page253;      /*253*/
    SMEM_UNIT_CHUNKS_STC   page255;      /*255*/

}PHY_DEV_MEM_INFO;

/*******************************************************************************
*   smemPhyInit
*
* DESCRIPTION:
*       Init memory module for a Phy device.
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
void smemPhyInit
(
    IN SKERNEL_DEVICE_OBJECT * deviceObj
);

/*******************************************************************************
*   smemPhyInit2
*
* DESCRIPTION:
*       Init memory module for a device - after the load of the default
*           registers file
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
void smemPhyInit2
(
    IN SKERNEL_DEVICE_OBJECT * deviceObj
);


/*******************************************************************************
*   smemPhyProcessInit
*
* DESCRIPTION:
*       Init the frame processing module.
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
void smemPhyProcessInit
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __smemPhyh */


