/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* smemFAP.h
*
* DESCRIPTION:
*       Data definitions for the Dune Fabric Access Processor memory.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 3 $
*
*******************************************************************************/
#ifndef __smemFaph
#define __smemFaph

#include <asicSimulation/SKernel/smem/smem.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/***********************************************************************/
/*
 * Typedef: struct FAP10M_CONFIG_MEM
 *
 * Description:
 *      Uplink Registers, Twsi Registers and label process registers
 *
 * Fields:
 *      uplinkRegNum    : Uplink Related Registers number.
 *      uplinkReg       : Uplink Related Registers array with the size of
 *                           uplinkRegNum
 *
 *      twsiRegNum      : TWSI Commands Registers number.
 *      twsiReg         : TWSI Commands Related Registers array with the size of
 *                           twsiRegNum
 *
 *      lbpRegNum      : Load Balance Processor Commands Registers number.
 *      lbpReg         : Load Balance Processor Related Registers array with the size of
 *                           twsiRegNum
 *
 *      schTblReg1Num  : Scheduler Static Sub-Flow Descriptor Table Registers number.
 *      schTblReg1     : Scheduler Static Sub-Flow Descriptor Table Register with
 *                           the size of schTblReg1Num
 *
 *      schTblReg2Num  : Scheduler Static Sub-Flow Descriptor Table Registers number.
 *      schTblReg2     : Scheduler Flow to Sub-Flow Mapping Table Register with
 *                           the size of schTblReg1Num
 *
 * Comments:
 *
 *  uplinkReg  - Registers with address mask 0x4FFFFF00 pattern 0x40000000
 *                      uplinkRegNum = 64  (Range : 0x0000 - 0x00FF).
 *
 *  twsiReg    - Registers with address mask 0x438FFF00 pattern 0x43800000
 *                      twsiRegNum = 64  (Range : 0x0000 - 0x00FF).
 *
 *  lbpReg    - Registers with address mask 0x4F800000 pattern 0x40800000
 *                      lbpRegNum = 16384  (Range : 0x0000 - 0xFFFF).
 *
 *  schDescTblReg - Registers with address mask 0x4FF10000 pattern 0x40010000
 *                      schDescTblRegNum = 20480  (Range : 0x00000 - 0x13FFF).
 *
 *  schMapTblReg    - Registers with address mask 0x4FF2F000 pattern 0x40020000
 *                      schMapTblRegNum = 1024  (Range : 0x000 - 0xFFF).
 *
 *  schCreditGenTblReg    - Registers with address mask 0x4FF4FF00 pattern 0x40040000
 *                      schCreditGenTblRegNum = 64  (Range : 0x000 - 0xFF).
 *
 *  schLookUpGenTblReg    - Registers with address mask 0x4FF50000 pattern 0x40050000
 *                      schLookUpGenTblRegNum = 2048  (Range : 0x000 - 0x1fFF).
 *
 *  schTriggerReg    - Registers with address mask 0xFFFFFFFF pattern 0x40100000
 *                      schTriggerRegNum = 1
 */

typedef struct {
    GT_U32                  uplinkRegNum;
    SMEM_REGISTER         * uplinkReg;

    GT_U32                  twsiRegNum;
    SMEM_REGISTER         * twsiReg;

    GT_U32                  lbpRegNum;
    SMEM_REGISTER         * lbpReg;

    GT_U32                  schDescTblRegNum;
    SMEM_REGISTER         * schDescTblReg;

    GT_U32                  schMapTblRegNum;
    SMEM_REGISTER         * schMapTblReg;

    GT_U32                  schCreditGenTblRegNum;
    SMEM_REGISTER         * schCreditGenTblReg;

    GT_U32                  schLookUpGenTblRegNum;
    SMEM_REGISTER         * schLookUpGenTblReg;

    GT_U32                  schCntTblRegNum;
    SMEM_REGISTER         * schCntTblReg;

    GT_U32                  schTriggerRegNum;
    SMEM_REGISTER         * schTriggerReg;
}FAP10M_CONFIG_MEM;

/***********************************************************************/
/*
 * Typedef: struct FAP10M_TABLES_MEM
 *
 * Description:
 *      Lable Processor Table Registers , Queue Controller Table Registers ,
 *       , Schedular Registers
 *
 * Fields:
 *      lpUnicastTablRegNum    : Label Processor Unicast Table Related Registers number.
 *      lpUnicastTablReg       : Label Processor Unicast Table Register array with the size of
 *                                unicastTablRegNum
 *
 *      lpTreeRouteTablRegNum    : Label Processor Tree Route Table Registers number.
 *      lpTreeRouteTablReg       : Label Processor Tree Route Table Register
 *                                   array with the size of unicastTreeRouteTablRegNum
 *
 *      lpRouteProcessorTablRegNum  : Routing Processor Tables Registers number.
 *      lpRouteProcessorTablReg     : Routing Processor Tables Register  array
 *                                     with the size of lpRouteProcessorTablRegNum array.
 *
 *      qcTablRegNum    : Queue Controler Table Registers number.
 *      qcTablReg       : Queue Controler Table
 *                         register array with the size of qcTablRegNum
 *
 *      qwredParamTblRegNum    : queue WRED Table Registers number.
 *      qwredParamTblReg       : queue WRED Table Registers
 *                                    array with the size of qwredParamTblRegNum
 *      qtParamMemTblNum    : Queue-Type Parameters memory Registers number.
 *      qtParamMemTbl       : Queue-Type Parameters memory Registers
 *                              array with the size of qtParamMemTblNum
 * Comments:
 *
 *  lpUnicastTblReg      - Registers with address mask 0xFFFFFF00 pattern
 *                          0x00000000 lpUnicastTableRegNum = 64
 *                            (Range : 0x00 - 0xFF).
 *
 *  lpTreeRouteTblReg    - Registers with address mask 0xF1FF0000 pattern
 *                           0x01000000  lpTreeRouteTblRegRegNum = 3FFF
 *                            (Range : 0x0000 - 0x3FFFF)
 *
 *  lpRecyclQueueTblReg - Registers with address mask 0xF2FFFF00 pattern 0x02000000
 *                          lpRecyclQueueTblRegNum = 64
 *                            (Range : 0x00 - 0xFF)
 *
 *  lpRouteProcessorTablReg - Registers with address mask 0xF8FF2FFF pattern 0x08002000
 *                              lpRouteProcessorTablRegNum = 1536
 *                            (Range : 0x2000 - 0x37FF)
 *
 *  qcTblReg             - Registers with address mask 0x3F0F0000 pattern 0x30000000
 *                          qcTblRegNum = 64 (Range : 0x30000000 - 0x30801FFF)
 *
 *  qtParamMemTblReg     - Registers with address mask 0x30100000 pattern 0x3010000X
 *                          qtParamMemTblRegNum = 4 (Range : 0x30001000 - 0x3000100F)
 *
 *  qwredParamTblReg     - Registers with address mask 0x30200000 pattern 0x302000XX
 *                          qcTblRegNum = 64 (Range : 0x30200000 - 0x302000FF)
 *
 *  qsizeMemReg          - Registers with address mask 0x30300000 pattern 0x3030XXXX
 *                          qsizeMemRegNum = 1FFF /4 +1  (Range : 0x30300000 - 0x30301FFF)
 *
 *  qflowMemReg          - Registers with address mask 0x30800000 pattern 0x3030XXXX
 *                          qcTblRegNum = 1FFF /4 +1  (Range : 0x30800000 - 0x30801FFF)
 */

typedef struct {
    GT_U32                  lpUnicastTablRegNum;
    SMEM_REGISTER         * lpUnicastTablReg;

    GT_U32                  lpTreeRouteTblRegNum;
    SMEM_REGISTER         * lpTreeRouteTblReg;

    GT_U32                  lpRecyclQueueTblRegNum;
    SMEM_REGISTER         * lpRecyclQueueTblReg;

    GT_U32                  qcTblRegNum;
    SMEM_REGISTER         * qcTblReg;

    GT_U32                  qtParamMemTblRegNum;
    SMEM_REGISTER         * qtParamMemTblReg;

    GT_U32                  qwredParamTblRegNum;
    SMEM_REGISTER         * qwredParamTblReg;

    GT_U32                  qsizeMemRegNum;
    SMEM_REGISTER         * qsizeMemReg;

    GT_U32                  qflowMemRegNum;
    SMEM_REGISTER         * qflowMemReg;

    GT_U32                  lpRouteProcessorTablRegNum;
    SMEM_REGISTER         * lpRouteProcessorTablReg;
}FAP10M_TABLES_MEM;

/*
 * Typedef: struct SMEM_FAP_DEV_MEM_INFO
 *
 * Description:
 *      Describe a device's memory object in the simulation.
 *
 * Fields:
 *      memMutex        : Memory mutex for device's memory.
 *      configMem       : Address type specific R/W functions.
 *      tblMem          : Global configuration registers.
 *
 * Comments:
 */

typedef struct {
    FAP10M_CONFIG_MEM               configMem;
    FAP10M_TABLES_MEM               tblMem;
}SMEM_FAP_DEV_MEM_INFO;

/*******************************************************************************
*   smemFapInit
*
* DESCRIPTION:
*       Init memory module for a dune fabric access processor device.
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
void smemFapInit
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
);


#define SMEM_FAP_REACHABILITY_MSG_SIZE        5

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __smemFaph */


