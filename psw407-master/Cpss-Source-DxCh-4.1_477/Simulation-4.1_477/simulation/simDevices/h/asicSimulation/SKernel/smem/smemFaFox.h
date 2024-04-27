/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* smemFaFox.h
*
* DESCRIPTION:
*       Data definitions for Fox/Leopard fabric adapter memories.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 12 $
*
*******************************************************************************/
#ifndef __smemFaFoxh
#define __smemFaFoxh

#include <asicSimulation/SKernel/smem/smem.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



/***********************************************************************
*   0 = VOQ Global Configuration Register and Descriptors.Device and Fport enable
*   1 = Device ID to Fabric Port Mapping
*
***********************************************************************/
/*
 * Typedef: struct FA_GLOBAL_MEM
 *
 * Description:
 *  1. Global , Hot Swap , GPP  , Buff Management allocation ,
 *     Miscellaneous Interrupt, UPLINK configuration, Buff. Mgmt max.
 *       Address space : 0x40000000 - 0x400000FF
 *  2. FTDLL Registers
 *      Address space  : 0x43000000 - 0x430000FF
 *  3. Buffer Management
 *      Address space  : 0x40010000 - 0x4001FFFF
 *  4. TWSI interface registers .
 *      Address space  : 0x43800000 - 0x438FFF00
 *
 * Fields:
 *      globRegsNum     : Global registers number.
 *      globRegs        : Global registers array with size of globRegsNum.
 *      ftdllRegsNum    : ftdll registers number.
 *      ftdllRegs       : ftdll registers array with size of ftdllRegsNum.
 *      BuffMngRegsNum  : Buffer management registers numbers.
 *      BuffMngRegs     : Buffer management registers array with size of
 *                        BuffMngRegsNum.
 *      twsiRegs        : Two-Wire Serial Interface (TWSI) Registers.
 *      twsiRegsNum     : Two-Wire Serial Interface (TWSI) Registers number.
 *
 * Comments:
 *
 *  globRegs      - Registers with address mask 0x4FFFFF00 pattern 0x400000XX
 *                  globRegsNum = 0xFF/4 + 1 = 0x40
 *  ftdllRegs     - Registers with address mask 0x43FFFF00 pattern 0x430000XX
 *                  ftdllRegsNum = 0xFF/4 + 1 = 0x40
 *  BuffMngRegs   - Registers with address mask 0x4FF10000 pattern 0x4001XXXX
 *                  BuffMngRegs = 0xFFFF /4 + 1 = 0x4000
 *  twsiRegs      - Registers with address mask 0x438000FF pattern 0x438000XX
 *                  twsiRegsNum = 0xFF / 4 + 1 = 0x40
 ****/
typedef struct
{
    GT_U32                  globRegsNum;
    SMEM_REGISTER         * globRegs;
    GT_U32                  ftdllRegsNum;
    SMEM_REGISTER         * ftdllRegs;
    GT_U32                  BuffMngRegsNum;
    SMEM_REGISTER         * BuffMngRegs;
    GT_U32                  twsiRegsNum;
    SMEM_REGISTER         * twsiRegs;
}FA_GLOBAL_MEM;

/*
 * Typedef: struct FA_VOQ_MEM
 *          Virtual Output Queue
 * Description:
 *
 *  1. VOQ configuration , Descriptors , Counters , Linked list ,
 *     Status and Interrupts.
 *       Address space : 0x40800000 - 0x4080FFFF
 *  2. VOQ entry registers
 *      Address space  : 0x40880000 - 0x4088FFFF
 *  3. VOQ E2E Flow Control
 *      Address space  : 0x40810000 - 0x4081FFFF
 *
 * Fields:
 *
 *      VOQRegsNum         : VOQ registers numbers.
 *      VOQRegs            : VOQ registers.
 *      VOQEntryRegsNum    : VOQ entry registers numbers.
 *      VOQEntryRegs       : VOQ entry registers.
 *      VOQFCRegsNum       : VOQ entry registers numbers.
 *      VOQFCRegs          : VOQ entry registers.
 *
 * Comments:
 *
 *  VOQRegs         -   Registers with address mask 0x4080FFFF pattern 0x4080XXXX
 *                    VOQRegsNum = 0xFFFF/4 + 1 = 0x4000
 *  VOQEntryRegs    -   Registers with address mask 0x4088FFFF pattern 0x4088XXXX
 *                    VOQRegsNum = 0xFFFF/4 + 1 = 0x4000
 *  VOQFCRegs       -   Registers with address mask 0x4081FFFF pattern 0x4081XXXX
 *                    VOQFCRegsNum = 0xFFFF/4 + 1 = 0x4000
 */
typedef struct
{
    GT_U32                  VOQRegsNum;
    SMEM_REGISTER         * VOQRegs;
    GT_U32                  VOQEntryRegsNum;
    SMEM_REGISTER         * VOQEntryRegs;
    GT_U32                  VOQFCRegsNum;
    SMEM_REGISTER         * VOQFCRegs;
}FA_VOQ_MEM;


/*
 * Typedef: struct TxD_MEM
 *
 * Description:
 *      Describe a Segmentation Engine and Tx DMA registers,.
 *
 *      1. segmentation register , Fabric Port, TC registers , WRR registers ,
 *          Address space : 0x41800000 - 0x418000FF
 *      2. segmentation CPU mailbox register
 *          Address space : 0x41900000 - 0x419000FF
 *
 * Fields:
 *
 *      segmentRegsNum         : segment registers numbers.
 *      segmentRegsNum         : segment registers.
 *      segmentCpuMailRegsNum  : segment cpu mail registers numbers.
 *      segmentCpuMailRegs     : segment cpu mail registers.
 *
 * Comments:
 *
 *  segmentRegs  -   Registers with address mask 0x418FFF00 pattern 0x418000XX
 *                    segmentRegsNum = 0xFF/4 + 1 = 0x40
 *  segmentCpuMailRegs - Registers with address mask 0x419FFF00 pattern 0x419000XX
 *                    segmentCpuMailRegs = 0xFF/4 + 1 = 0x40
 *
 */
typedef struct
{
    GT_U32                  segmentRegsNum;
    SMEM_REGISTER         * segmentRegs;
    GT_U32                  segmentCpuMailRegsNum;
    SMEM_REGISTER         * segmentCpuMailRegs;
}FA_SEG_MEM;

/*
 * Typedef: struct CRX_MEM
 *
 * Description:
 *      Cell Reassembly Processor registers mep .
 *
 * Fields:
 *
 *      cellRegsNum   : Cell Reassembly register numbers.
 *      cellRegs      : Cell Reassembly register.
 *
 * Comments:
 *
 *  cellRegs  -   Registers with address mask 0x41FF0000 pattern 0x4100XXXX
 *                    cellRegsNum = 0xFFFF/4 + 1 = 0x4000
 */
typedef struct
{
    GT_U32                  cellRegsNum;
    SMEM_REGISTER         * cellRegs;
}FA_CRX_MEM;


/*
 * Typedef: struct XBAR_MEM
 *
 * Description:
 *
 *  Describe Cross bar registers map .
 *
 *  1. LED stream configuration register
 *      Address space : 0x42700000 - 0x427000FF
 *  2. Group 1 xbar registers
 *      Address space : 0x42000000 - 0x420CFFFF
 *  3. Group 2 xbar registers
 *      Address space : 0x42100000 - 0x42143FF0
 *
 * Fields:
 *
 *      configRegs      : Xbar config registers .
 *      configRegsNum   : Xbar config registers numbers.
 *      group1Regs      : Xbar group 1 registers .
 *      group1RegsNum   : Xbar group 1 registers numbers.
 *      group2Regs      : Xbar group 2 registers .
 *      group2RegsNum   : Xbar group 2 registers numbers.
 *
 * Comments:
 *
 *      configRegs      : Registers with address mask 0x427FFF00 pttern 0x42700XX
 *                          configRegsNum = 0xFF /4 +1 = 0x60
 *      group1Regs      : Registers with address mask 0x420FFFFF pttern 0x420XXXX
 *                          group1RegsNum = 0x4FFFF /4 +1 = 0x40000
 *      group2Regs      : Registers with address mask 0x421FFFFF pttern 0x421FFFFF
 *                          group2RegsNum = 0x4FFFF /4 +1 = 0x40000
 */

typedef struct
{
    GT_U32                  configRegsNum;
    SMEM_REGISTER         * configRegs;
    GT_U32                  group1RegsNum;
    SMEM_REGISTER         * group1Regs;
    GT_U32                  group2RegsNum;
    SMEM_REGISTER         * group2Regs;
}XBAR_MEM;

/*
 * Typedef: struct DB_OPERATE_MEM
 *
 *
 * Description:
 *
 *  External memory registers map .
 *
 *  1. DB operation register
 *      Address space : 0x42800000 - 0x428001FF
 *
 * Fields:
 *
 *      DbRegs      : external memory registers .
 *      DbRegsNum   : external memory registers numbers.
 *
 * Comments:
 *
 *      DbRegs      : Registers with address mask 0x428FF100 pttern 0x428001XX
 *                          configRegsNum = 0x1FF /4 +1 = 0x80
 *
 */

typedef struct
{
    GT_U32                  DbRegsNum;
    SMEM_REGISTER         * DbRegs;
}DB_OPERATE_MEM;


/*
 * Typedef: struct INTERRUPT_MEM
 *
 * Description:
 *
 *  Describe hyperGlink main interrupt cause registers  .
 *
 *  1. HyperGLink0 main interrupt cause register
 *      Address space : 0x42080008 - 0x420800FF
 *  2. HyperGLink1 main interrupt cause register
 *      Address space : 0x42088000 - 0x420880FF
 *  3. HyperGLink2 main interrupt cause register
 *      Address space : 0x42090000 - 0x420900FF
 *  4. HyperGLink3 main interrupt cause register
 *      Address space : 0x42180000 - 0x421800FF
 *
 * Fields:
 *
 *      HyperGLink0InterRegs   : HyperGLink0 main interrupt cause register.
 *      HyperGLink0InterRegsNum: HyperGLink0 main interrupt cause register number
 *      HyperGLink1InterRegs   : HyperGLink1 main interrupt cause register.
 *      HyperGLink1InterRegsNum: HyperGLink1 main interrupt cause register number
 *      HyperGLink2InterRegs   : HyperGLink2 main interrupt cause register.
 *      HyperGLink2InterRegsNum: HyperGLink2 main interrupt cause register number
 *      HyperGLink3InterRegs   : HyperGLink3 main interrupt cause register.
 *      HyperGLink3InterRegsNum: HyperGLink3 main interrupt cause register number
 *
 * Comments:
 *
 *      HyperGLink0InterRegs : Registers with address mask 0x42F8FF00 pattern 0x420800XX
 *                          HyperGLink0InterRegsNum = 0xFF /4 +1 = 0x40
 *      HyperGLink1InterRegs : Registers with address mask 0x42F88F00 pattern 0x420880XX
 *                          HyperGLink1InterRegsNum = 0xFF /4 +1 = 0x40
 *      HyperGLink2InterRegs : Registers with address mask 0x42F9FF00 pattern 0x420900XX
 *                          HyperGLink2InterRegsNum = 0xFF /4 +1 = 0x40
 *      HyperGLink3InterRegs : Registers with address mask 0x4218FF00 pattern 0x421800XX
 *                          HyperGLink3InterRegsNum = 0xFF /4 +1 = 0x40
 */

typedef struct
{
    GT_U32                  HyperGLink0InterRegsNum;
    SMEM_REGISTER         * HyperGLink0InterRegs;
    GT_U32                  HyperGLink1InterRegsNum;
    SMEM_REGISTER         * HyperGLink1InterRegs;
    GT_U32                  HyperGLink2InterRegsNum;
    SMEM_REGISTER         * HyperGLink2InterRegs;
    GT_U32                  HyperGLink3InterRegsNum;
    SMEM_REGISTER         * HyperGLink3InterRegs;
}HYPERGLINK_INTER_MEM;


typedef struct
{
    GT_U32                  CPUMailMsgCounter;
    SMEM_REGISTER           CPUMailMsgData[0x64];
}CPUMAIL_MSG_MEM;


/*
 * Typedef: struct FA_DEV_MEM_INFO
 *
 * Description:
 *      Describe a device's memory object in the simulation.
 *
 * Fields:
 *      memMutex        : Memory mutex for device's memory.
 *      specFunTbl      : Address type specific R/W functions.
 *      globalMem       : Global configuration registers.
 *      VOQMem          : Virtual Output Queue registers.
 *      CRXMem          : Cell Reassembly Processor Registers.
 *      XbarMem         : X-Var configuration registers.
 *      segRegsMem      : Segmentation registers.
 *      RcvCpuMailMsg   : incoming CPU mail registers.s
 *      RamMem          : DMA external memory registers.
 *      BuffMem         : Buffer Management Registers.
 *
 * Comments:
 */

typedef struct
{
    SMEM_SPEC_FIND_FUN_ENTRY_STC specFunTbl[64];
    FA_GLOBAL_MEM                globalMem;
    FA_VOQ_MEM                   VOQMem;
    FA_CRX_MEM                   CRXMem;
    XBAR_MEM                     XbarMem;
    FA_SEG_MEM                   segRegsMem;
    CPUMAIL_MSG_MEM              RcvCpuMailMsg;
    DB_OPERATE_MEM               RamMem;
    HYPERGLINK_INTER_MEM         HyperGLinkinterMem;
}FA_DEV_MEM_INFO;


/*******************************************************************************
* smemFaFoxInit
*
* DESCRIPTION:
*       Init memory module for a fabric adapter device.
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
void smemFaFoxInit
(
    IN SKERNEL_DEVICE_OBJECT * deviceObj
);

/*******************************************************************************
* smemFaFoxFindMem
*
* DESCRIPTION:
*       Return pointer to the register's or tables's memory.
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       address     - address of memory(register or table).
*
* OUTPUTS:
*     activeMemPtrPtr - pointer to the active memory entry or NULL if not exist.
*
* RETURNS:
*        pointer to the memory location
*        NULL - if memory not exist
*
* COMMENTS:
*
*
*******************************************************************************/
void * smemFaFoxFindMem
(
    IN SKERNEL_DEVICE_OBJECT * deviceObj,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32                  address,
    IN GT_U32                  memSize,
    OUT SMEM_ACTIVE_MEM_ENTRY_STC ** activeMemPtrPtr
);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __smemfaFoxh */


