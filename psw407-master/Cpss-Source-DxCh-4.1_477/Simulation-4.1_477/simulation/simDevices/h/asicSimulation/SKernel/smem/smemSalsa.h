/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* smemSalsa.h
*
* DESCRIPTION:
*       Data definitions for Salsa memories.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 5 $
*
*******************************************************************************/
#ifndef __smemSalsah
#define __smemSalsah

#include <asicSimulation/SKernel/smem/smem.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/***********************************************************************
*    Salsa memories address types:
*    Bits 23:28 of address defines one of types:
*
*   0 = Global Registers, CPU port registers and TWSI registers
*   1- 2 = reserved
*   3 = Transmit Queue registers
*   4 = Ethernet Bridge registers
*   5 = reserved
*   6 = Buffer Management register
*   7 = reserved
*   8 = Ports group0 configuration registers (port0 through port5),
*       LEDs interface0 configuration registers and Master SMI interface regs
*   9 = Ports group1 configuration registers (port6 through port11) (see as 8)
*   10 = Ports group2 configuration registers (port12 through port17)(see as 8)
*   11 = Ports group3 configuration registers (port18 through port23)(see as 8)
*   12 = MAC Table Memory
*   13 = Internal Buffers memory Bank0 address space
*   14 = Internal Buffers memory Bank1 address space
*   15 = buffers memory block configuration registers
*   20 = VLAN Table configuration registers and VLAN Table address space.
*   63-21 - reserved
*
*
*************************************************************************/
/*
 * Typedef: struct SALSA_GLOBAL_MEM
 *
 * Description:
 *      Global Registers, CPU port registers and TWSI registers.
 *
 * Fields:
 *      globRegsNum    : Global registers number.
 *      globRegs       : Global registers array with size of globRegsNum.
 *      twsiIntRegsNum : TWSI internal registers number.
 *      twsiIntRegs     : TWSI internal registers array with size of globRegsNum.
 *
 * Comments:
 * For all registers from this struct Bits 23:28 == 0
 *
 *  globRegs      - Registers with address mask 0xFFFFFF00 pattern 0x00000000
 *                    globRegsNum = 0xff / 4 + 1 = 64
 *
 * twsiIntRegsNum - Registers with address mask 0xFFFFFF00 pattern 0x00400000
 *                    twsiIntRegsNum = 0x1c / 4 + 1
 */

typedef struct {
    GT_U32                  globRegsNum;
    SMEM_REGISTER         * globRegs;
    GT_U32                  twsiIntRegsNum;
    SMEM_REGISTER         * twsiIntRegs;
}SALSA_GLOBAL_MEM;

/*
 * Typedef: struct SALSA_BUF_MNG_MEM
 *
 * Description:
 *      Describe a device's buffer management registers memory object.
 *
 * Fields:
 *      genRegsNum  : General buffers management registers number.
 *      genRegs     : General buffers management registers array.
 *      cntrlRegsNum : Control Linked List buffers management registers number.
 *      cntrlRegs    : Control Linked List buffers management registers array.
 *
 * Comments:
 * For all registers from this struct Bits 23:28 == 6
 *
 *  genRegs       - Registers with address mask 0xFFFFFE00 pattern 0x03000000
 *                    genRegsNum = 0x17C / 4 + 1
 *
 * cntrlRegsNum - Registers with address mask 0x00003000 pattern > 0
 *                    cntrlRegsNum = 2048
 */

typedef struct {
    GT_U32                  genRegsNum;
    SMEM_REGISTER         * genRegs;
    GT_U32                  cntrlRegsNum;
    SMEM_REGISTER         * cntrlRegs;
}SALSA_BUF_MNG_MEM;

/*
 * Typedef: struct SALSA_PORT_MEM
 *
 * Description:
 *      Describe a device's Port registers memory object.
 *
 * Fields:
 *      macPortsNum     : Number of MAC ports, defines number of other registers.
 *      perPortRegsNum  : Number of types for per port registers.
 *      perPortRegs     : Per port registers, index is port's number and type.
 *      perGroupRegsNum : Number of types for per group registers.
 *      perGroupRegs    : Per group registers , index is port's number and type.
 *      macCntsNum      : Number of counters types.
 *      macCnts         : MAC counters array, index is port's number and type number.
 *
 * Comments:
 * For all registers from this struct Bits 23:28 == 8 or 9 or 10 or 11
 *
 *  macPortsNum = 24
 *  perPortRegs   - Registers with address mask 0xE07FFC00 pattern 0x00000000
 *                  Per port types num = 0x14 / 4 + 1
 *                  number of perPortRegsNum = macPortsNum * perPortTypesNum
 *
 *  perGroupRegs -         Registers with address mask 0xE07F4000 pattern 0x00004000
 *                  Per group types num = 0x104 / 4 + 1
 *                  number of perGroupRegsNum = macPortsNum * perGroupTypesNum
 *
 *  macCnts -                 Registers with address mask 0xE071F000 pattern 0x00010000
 *                  Mac count types num = 0x7C / 4 + 1
 *                  number of macCntsNum = macPortsNum * macCntTypesNum
 */

typedef struct {
    GT_U32                  macPortsNum;
    GT_U32                  perPortRegsNum;
    SMEM_REGISTER         * perPortRegs;
    GT_U32                  perGroupRegsNum;
    SMEM_REGISTER         * perGroupRegs;
    GT_U32                  macCntsNum;
    SMEM_REGISTER         * macCnts;
}SALSA_PORT_MEM;

/*
 * Typedef: struct SALSA_BRDG_MNG_MEM
 *
 * Description:
 *      Describe a device's Bridge Management registers memory object.
 *
 * Fields:
 *      brgPortsNum       : Bridge ports number.
 *      portRegsGroupsNum : Group number of Port based bridge registers.
 *      portRegs          : Port Bridge control registers, index is port number
 *                          and group number
 *      portProtVidNum    : Group number of Ports Protocol Based VLANs configuration Registers.
 *      portProtVidRegs   : Ports Protocol Based VLANs configuration Registers,
 *                          index is port and group number
 *      genRegsNum      : General Bridge management registers number.
 *      genRegs         : General Bridge management registers array.
 *
 * Comments:
 * For all registers from this struct Bits 23:28 == 4
 *
 *  brgPortsNum = 24 + 1 (CPU) = 25
 *  portRegs   -         Registers with address mask 0xE07E0F00 pattern 0x00000000
 *                  Port regs groups num = 0x10 / 4 + 1
 *                  number of portRegsNum = brgPortsNum * portRegsGroupsNum
 *
 *  portProtVidRegs -
                                         Registers with address mask 0xE07E0800 pattern 0x00000800
 *                  Port prot vid num = 2
 *                  number of portProtVidRegsNum = brgPortsNum * portProtVidNum
 *
 *  genRegs -                 Registers with address mask 0xE07F0000 pattern 0x00040000
 *                  Gen regs num = 0x27C / 4 + 1
 *                  number of genRegsNum = genRegsNum
 */

typedef struct {
    GT_U32                  brgPortsNum;
    GT_U32                  portRegsNum;
    SMEM_REGISTER         * portRegs;
    GT_U32                  portProtVidRegsNum;
    SMEM_REGISTER         * portProtVidRegs;
    GT_U32                  genRegsNum;
    SMEM_REGISTER         * genRegs;
}SALSA_BRDG_MNG_MEM;

/*
 * Typedef: struct SALSA_EGR_MEM
 *
 * Description:
 *      Describe a device's Egress Transmit Queue and VLT registers memory object.
 *
 * Fields:
 *      genRegsNum        : Common registers number.
 *      genRegs           : Common registers number, index is group number.
 *      stackCfgRegNum    : Group number of Target Device to Cascading port Configuration Registers.
 *      stackCfgRegs      : Target Device to Cascading port Configuration Registers,
 *                          index group number
 *
 * Comments:
 * For all registers from this struct Bits 23:28 == 3
 *
 *  genRegs - Registers with address mask 0xE07E0000 pattern 0x00000000
 *                    genRegsNum = 0x1ffff / 4 + 1
 *                  number of genRegs = genRegsNum
 *
 *  stackCfgRegs   - Registers with address mask 0xE07F0000 pattern 0x00040000
 *                    stackCfgRegNum = 0x1C / 4 + 1
 *                  number of stackCfgRegs = stackCfgRegNum
 *  txQueRegs  - Registers with address mask 0xE07F0000 pattern 0x00050000
 *                    txQueRegNum = 0x1C / 4 + 1
 *                  number of txQueRegNum = txQueRegNum
 *
 */

typedef struct {
    GT_U32                  genRegsNum;
    SMEM_REGISTER         * genRegs;
    GT_U32                  stackCfgRegNum;
    SMEM_REGISTER         * stackCfgRegs;
    GT_U32                  txQueRegNum;
    SMEM_REGISTER         * txQueRegs;
}SALSA_EGR_MEM;

/*
 * Typedef: struct SALSA_BUF_CFG_MEM
 *
 * Description:
 *      Describe a device's Buffers Memory Configuration Registers registers.
 *
 * Fields:
 *      regsNum          : Registers number.
 *      regs             : Registers, index is group number.
 *
 * Comments:
 * For all registers from this struct Bits 23:28 == 15
 *
 *  regsNum = 3
 *
 *  regs   - Registers with address mask 0xE07FFFF0 pattern 0x00000000
 *                  number of regs = regsNum
 *
 */

typedef struct {
    GT_U32                  regsNum;
    SMEM_REGISTER         * regs;
}SALSA_BUF_CFG_MEM;

/*
 * Typedef: struct SALSA_MAC_FDB_MEM
 *
 * Description:
 *      Describe a device's Bridge registers and FDB.
 *
 * Fields:
 *      regsNum          : Registers number.
 *      regs             : Registers, index is group number.
 *      macTblRegNum     : Number of MAC Table registers.
 *      macTblRegs       : MAC Table Registers.
 *      macUpdFifoRegsNum : Number of MAC update FIFO registers.
 *      macUpdFifoRegs    : MAC update FIFO registers.
 *
 * Comments:
 * For all registers from this struct Bits 23:28 == 12
 *
 *  regsNum = 0x2ff /4 + 1
 *
 *  regs   - Registers with address mask 0xFFFFF000 pattern 0x06000000
 *                  number of regs = regsNum
 *
 *  macTblRegNum = 4 * 1024 * 4 (4K entries, 4 words in the entry)
 *
 *  macUpdFifoRegsNum = 4 * 16 (16 MAC Update entires, 4 words in the entry)
 *
 */

typedef struct {
    GT_U32                  regsNum;
    SMEM_REGISTER         * regs;
    GT_U32                  macTblRegNum;
    SMEM_REGISTER         * macTblRegs;
    GT_U32                  macUpdFifoRegsNum;
    SMEM_REGISTER         * macUpdFifoRegs;
}SALSA_MAC_FDB_MEM;

/*
 * Typedef: struct SALSA_VLAN_TBL_MEM
 *
 * Description:
 *      Describe a device's Bridge registers and FDB.
 *
 * Fields:
 *      vltRegsNum       : Registers number for Vlan Translation Tablem (VLT)
 *      vltRegs          : VLT Registers.
 *      vlanTblRegNum    : Registers number for Vlan Table
 *      vlanTblRegs      : Vlan Table Registers.
 *      mcastTblRegNum   : Registers number for Vlan Table
 *      mcastTblRegs     : Vlan Table Registers.
 *      stpTblRegNum     : Registers number for Span State Table
 *      stpTblRegs       : Span State Table Registers.
 *
 * Comments:
 * For all registers from this struct Bits 23:28 == 20
 *
 *  vltRegsNum = 2048
 *
 *  vltRegs   - Registers with address mask 0x0000FFFF pattern < 0x2000
 *
 *  vlanTblRegNum = 256 * 4 (256 entries, 4 words in the entry)
 *
 *  vlanTblRegs - Registers with address mask 0x0000FFFF 0x2000 <= pattern < 0x3000
 *
 *  mcastTblRegNum = 64 (64 entries, 1 word in the entry)
 *
 *  mcastTblRegs - Registers with address mask 0x0000FFFF 0x4000 <= pattern < 0x4100
 *
 *  stpTblRegNum = 32 * 2 (32 entries, 2 word in the entry)
 *
 *  stpTblRegs - Registers with address mask 0x0000FFFF 0x6000 <= pattern < 0x6100
 *
 */

typedef struct {
    GT_U32                  vltRegsNum;
    SMEM_REGISTER         * vltRegs;
    GT_U32                  vlanTblRegNum;
    SMEM_REGISTER         * vlanTblRegs;
    GT_U32                  mcastTblRegNum;
    SMEM_REGISTER         * mcastTblRegs;
    GT_U32                  stpTblRegNum;
    SMEM_REGISTER         * stpTblRegs;
}SALSA_VLAN_TBL_MEM;

/*
 * Typedef: struct SALSA_BANK_MEM
 *
 * Description:
 *      Describe a device's Buffers Memory Banks.
 *
 * Fields:
 *      bank0RegsNum - registers number of Buffers Memory Bank0
 *      bank0Regs    - pointer to Buffers Memory Bank0
 *      bank1RegsNum - registers number of Buffers Memory Bank1
 *      bank1Regs    - pointer to Buffers Memory Bank1
 *
 * Comments:
 * For all registers from this struct Bits 23:28 == 13 and 14
 *
 *  bank0RegsNum = 32K of 32 bits words
 *
 *  bank0Regs   - Registers with Bits 23:28 == 13
 *
 *  bank1RegsNum = 32K of 32 bits words
 *
 *  bank1Regs   - Registers with Bits 23:28 == 14
*/
typedef struct {
    GT_U32                  bank0RegsNum;
    SMEM_REGISTER         * bank0Regs;
    GT_U32                  bank1RegsNum;
    SMEM_REGISTER         * bank1Regs;
}SALSA_BUFFER_MEM;

/*
 * Typedef: struct SALSA_DEV_MEM_INFO
 *
 * Description:
 *      Describe a device's memory object in the simulation.
 *
 * Fields:
 *      memMutex        : Memory mutex for device's memory.
 *      specFunTbl      : Address type specific R/W functions.
 *      globalMem       : Global configuration registers.
 *      bufMngMem       : Buffers management registers.
 *      portMem         : Port/MAC control registers and counters.
 *      bridgeMngMem    : Bridge management registers.
 *      egressMem       : TX queues management registers.
 *      bufMem          : Buffers access registers.
 *      macFbdMem       : MAC FDB table.
 *      vlanTblMem      : VLAN tables.
 *
 * Comments:
 */

typedef struct {
    SMEM_SPEC_FIND_FUN_ENTRY_STC  specFunTbl[64];
    SALSA_GLOBAL_MEM            globalMem;
    SALSA_BUF_MNG_MEM           bufMngMem;
    SALSA_PORT_MEM              portMem;
    SALSA_BRDG_MNG_MEM          bridgeMngMem;
    SALSA_EGR_MEM               egressMem;
    SALSA_BUF_CFG_MEM           bufMem;
    SALSA_MAC_FDB_MEM           macFbdMem;
    SALSA_VLAN_TBL_MEM          vlanTblMem;
    SALSA_BUFFER_MEM            bufBanksMem;
}SALSA_DEV_MEM_INFO;

/*******************************************************************************
*   smemSalsaInit
*
* DESCRIPTION:
*       Init memory module for a Salsa device.
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
void smemSalsaInit
(
    IN SKERNEL_DEVICE_OBJECT * deviceObj
);

/*******************************************************************************
*   smemSalsaFindMem
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
void * smemSalsaFindMem
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32                  address,
    IN GT_U32                  memSize,
    OUT SMEM_ACTIVE_MEM_ENTRY_STC ** activeMemPtrPtr
);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __smemh */


