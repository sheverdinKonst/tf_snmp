/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* smemMacsec.h
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
#ifndef __smemMacsech
#define __smemMacsech

#include <asicSimulation/SKernel/smem/smem.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * Typedef: struct MACSEC_DEV_MEM_INFO
 *
 * Description:
 *      Describe a device's memory object in the simulation.
 *
 * Fields:
 *
 * Comments:
 */
/* 6 regions - each region is 0x800 :
0000 - port 0
0800 - port 1
1000 - port 2
1800 - port 3
2000 - 2000 - mac stat ports 0,1,2,3
       2100 - global regs
       2200 - pet stat ports  0,1,2,3
2800 - mac statistics port 0,1,2,3
3000 - empty
...  - empty

*/
#define MACSEC_NUM_MEMORY_SPACES_CNS   6
/* number of ports in the macsec device */
#define MACSEC_NUM_PORTS_CNS   4
/* get the unit according to address (0x800 steps of units)*/
#define MACSEC_UNIT_INDEX_GET_MAC(address)  ((address) >> 11)

typedef struct {
    SMEM_SPEC_FIND_FUN_ENTRY_STC    specFunTbl[MACSEC_NUM_MEMORY_SPACES_CNS];

    SMEM_UNIT_CHUNKS_STC   ports[MACSEC_NUM_PORTS_CNS]; /*0,1,2,3*/
    SMEM_UNIT_CHUNKS_STC   macStat;          /*4*/
    SMEM_UNIT_CHUNKS_STC   global;           /*4*/
    SMEM_UNIT_CHUNKS_STC   petStat;          /*4*/
    SMEM_UNIT_CHUNKS_STC   macsecStatistics; /*5*/
}MACSEC_DEV_MEM_INFO;

/*******************************************************************************
*   smemMacsecInit
*
* DESCRIPTION:
*       Init memory module for a Macsec device.
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
void smemMacsecInit
(
    IN SKERNEL_DEVICE_OBJECT * deviceObj
);

/*******************************************************************************
*   smemMacsecInit2
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
void smemMacsecInit2
(
    IN SKERNEL_DEVICE_OBJECT * deviceObj
);

/*******************************************************************************
*   smemMacsecProcessInit
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
void smemMacsecProcessInit
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __smemMacsech */


