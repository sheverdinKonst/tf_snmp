/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* smemTigerGM.h
*
* DESCRIPTION:
*       Data definitions for Twist memories.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 3 $
*
*******************************************************************************/
#ifndef __smemTigerGMh
#define __smemTigerGMh

#include <asicSimulation/SKernel/smem/smem.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define TIGER_SFLOW_FIFO_SIZE       32
/***********************************************************************
*   0 = Global Registers, TWSI registers, and packet DMA registers
*   1 = Configuration registers for Ports (Group Of Ports 0).
*   2 = Configuration Registers for Ports (Group Of Ports 1)
*   3 = Transmit Queue registers and Buffer SDRAM Configuration   registers
*   4 = Ethernet Bridge registers
*   5 = IPv4,Traffic Control Block SRAM (NS) registers.
*   6 = Buffer Management register
*   7 = Header Alteration
*   11-8 = Reserved
*   12 = MAC Table Memory
*   19-13 = Reserved
*   20 = VLAN Table Memory
*   63-21 = Reserved
*
***********************************************************************/
/*
 * Typedef: struct TIGER_GM_GLOBAL_MEM
 *
 * Description:
 *      Global Registers, TWSI registers, and packet DMA registers
 *
 * Fields:
 *      globRegsNum    : Global registers number.
 *      globRegs       : Global registers array with size of globRegsNum.
 *      twsiIntRegsNum : TWSI register number.
 *      twsiIntRegs    : TWSI register array with size of twsiIntRegsNum.
 *      sdmaRegsNum    : SDMA register number.
 *      sdmaRegs       : SDMA register array with size of twsiIntRegsNum.
 * Comments:
 *  For all registers from this struct Bits 23:28 == 0
 *
 *  globRegs      - Registers with address mask 0x1FFFFF00 pattern 0x00000000
 *                  globRegsNum = 0x80 / 4 + 1 = 64
 *
 *  twsiIntRegsNum - Registers with address mask 0x1FFFFF00 pattern 0x00400000
 *                   twsiIntRegsNum = 0x1C / 4 + 1
 *
 *  sdmaRegsNum     - Registers with address mask 0x1FFFF000 pattern 0x00002000
 *                   sdmaRegsNum = 0x864 / 4 + 1
 */

typedef struct {
    GT_U32                  globRegsNum;
    SMEM_REGISTER         * globRegs;
    GT_U32                  twsiIntRegsNum;
    SMEM_REGISTER         * twsiIntRegs;
    GT_U32                  sdmaRegsNum;
    SMEM_REGISTER         * sdmaRegs;
    GT_U32                * currentDescrPtr;
}TIGER_GM_GLOBAL_MEM;

/*
 * Typedef: struct TIGER_GM_PORT_MEM
 *
 * Description:
 *      Describe a device's Port registers memory object.
 *
 * Fields:
 *
 *      perPortCtrlRegsNum  : Number of types for per port control registers.
 *      perPortCtrlRegs     : Per port control registers status,
 *                            index is port's number and type.
 *      perPortStatRegsNum  : Number of types for per port status registers.
 *      perPortStatRegs     : Per port registers status,
 *                            index is port's number and type.
 *      perGroupRegsNum     : Number of types for per group control registers.
 *      perGroupRegs        : Per group registers status,
 *                            index is group number and type.
 *      macCntsNum          : Number of counters types.
 *      macCnts             : MAC counters array,
 *                            index is port's number and type number.
 * Comments:
 * For all registers from this struct Bits 23:28 == 1 or 2
 *
 *  perPortCtrlRegs
 *                - Registers with address mask 0x1E7F4000 pattern 0x00000000
 *                  number of perPortRegsNum = MAC_PORTS_NUM * perPortTypesNum
 *
 *  perPortStatRegs
 *                - Registers with address mask 0x1E7F4004 pattern 0x00000004
 *                  number of perPortRegsNum = MAC_PORTS_NUM
 *
 *  perGroupRegs -  Registers with address mask 0x1E7F4000 pattern 0x00004000
 *                  Per group types num = 72
 *                  number of perGroupRegsNum = perGroupsTypesNum
 *
 *  macCnts -       Registers with address mask 0x1E710000 pattern 0x00010000
 *                  Mac count types num = 0x7C / 4 + 1
 *                  number of macCntsNum = MAC_PORTS_NUM * macCntTypesNum
 */

typedef struct {
    GT_U32                  perPortCtrlRegsNum;
    SMEM_REGISTER         * perPortCtrlRegs;
    GT_U32                  perPortStatRegsNum;
    SMEM_REGISTER         * perPortStatRegs;
    GT_U32                  perGroupRegsNum;
    SMEM_REGISTER         * perGroupRegs;
    GT_U32                  macCntsNum;
    SMEM_REGISTER         * macCnts;
}TIGER_GM_PORT_MEM;

/*
 * Typedef: struct TIGER_GM_EGR_MEM
 *
 * Description:
 *      Egress and Transmit (Tx) Queue Configuration Registers.
 *
 * Fields:
 *      genRegsNum      : Common registers number.
 *      genRegs         : Common registers number, index is port and group
 *      tcRegsNum       : Port Traffic Class Queue Descriptors number
 *      tcRegs          : Port Traffic Class Queue Descriptors Limit Registers
 *      transConfRegsNum: Port Transmit Configuration Register number.
 *      transConfRegs   : Port Transmit Configuration Register
 *      descLimRegsNum  : Port Descriptor Limit Register number
 *      descLimRegs     : Port Descriptor Limit Register
 *      transqRegsNum   : Transmit Queue Registers number
 *      transqRegs      : Transmit Queue Registers
 *      extMemRegs      : External Memory Registers
 *
 * Comments:
 * For all registers from this struct Bits 23:28 == 3
 *
 *  genRegs     -   Registers with address mask 0x1E700000 pattern 0x00000000
 *                  genRegsNum = 0xFFF / 4 + 1
 *                  number of genRegs = MAC_PORTS_NUM * genRegsNum
 *  tcRegs      -   Registers with address mask 0x1E700FFF
 *                  0x00000200 <= pattern < 0x00000270,
                    tcRegs = TC_QUE_DESCR_NUM * MAC_PORTS_NUM
 *  transConfRegs-  Registers with address mask 0x1E700FFF pattern 0x00000000
                    transConfRegsNum = MAC_PORTS_NUM
 *  descLimRegs -   Registers with address mask 0x1E700FFF pattern 0x00000100
 *                  descLimRegsNum = MAC_PORTS_NUM
 *  extMemRegs  -   Registers with address mask 0x11900000 pattern 0x01900000
 *                  extMemRegsNum = EXT_MEM_CONF_NUM
 *  trunkRegs     - Trunk registers 0x0180_280 , 0x0181_280 ,0x0182_280 ,
 *                                  0x0183_280
 *                  24 registers for low (8 non-trunk , 16 designated ports )
 *                  24 registers for Hi  (8 non-trunk , 16 designated ports )
 *                  total of 48 registers
 *
 */

typedef struct {
    GT_U32                  genRegsNum;
    SMEM_REGISTER         * genRegs;
    GT_U32                  tcRegsNum;
    SMEM_REGISTER         * tcRegs;
    GT_U32                  transConfRegsNum;
    SMEM_REGISTER         * transConfRegs;
    GT_U32                  descLimRegsNum;
    SMEM_REGISTER         * descLimRegs;
    GT_U32                  extMemRegsNum;
    SMEM_REGISTER         * extMemRegs;
    GT_U32                  trunkRegsNum;
    SMEM_REGISTER         * trunkMemRegs;
}TIGER_GM_EGR_MEM;

/*
 * Typedef: struct TIGER_GM_BRDG_MNG_MEM
 *
 * Description:
 *      Describe a device's Bridge Management registers memory object.
 *
 * Fields:
 *      portRegsGroupsNum : Group number of Port based bridge registers.
 *      portRegs          : Port Bridge control registers, index is port number
 *                          and group number
 *      portProtVidNum    : Group number of Ports Protocol Based VLANs configuration Registers.
 *      portProtVidRegs   : Ports Protocol Based VLANs configuration Registers,
 *                          index is port and group number
 *      genRegsNum        : General Bridge management registers number.
 *      genRegs           : General Bridge management registers array.
 *
 * Comments:
 * For all registers from this struct Bits 23:28 == 4
 *
 *  portRegs   -    Registers with address mask 0x1DF00000 pattern 0x00000000
 *                  Port regs groups num = 0x10 / 4 + 1
 *                  number of portRegsNum = MAC_PORTS_NUM * portRegsGroupsNum
 *
 *  portProtVidRegs -
                    Registers with address mask 0x1DF00F00 pattern 0x00000800
 *                  Port prot vid num = 2
 *                  number of portProtVidRegsNum = MAC_PORTS_NUM * portProtVidNum
 *
 *  genRegs -       Registers with address mask 0x1DFFF000 pattern 0x00040000
 *                  Gen regs num = 0xB270 / 4 + 1
 *                  number of genRegsNum = genRegsNum
 */

typedef struct {
    GT_U32                  portRegsNum;
    SMEM_REGISTER         * portRegs;
    GT_U32                  portProtVidRegsNum;
    SMEM_REGISTER         * portProtVidRegs;
    GT_U32                  genRegsNum;
    SMEM_REGISTER         * genRegs;
}TIGER_GM_BRDG_MNG_MEM;

/*
 * Typedef: struct TIGER_GM_LX_UNIT_MEM
 *
 * Description:
 *      LX unit Registers.
 *
 * Fields:
 *      genRegsNum  : General LX unit registers number.
 *      genRegs     : General LX unit registers array.
 *      mplsRegsNum : MPLS Processor Registers number
 *      mplsRegs    : MPLS Processor Registers array
 *      dscpRegsNum : DSCP registers number.
 *      dscpRegs    : DSCP registers array.
 *      pclTblRegsNum : PCL Table Registers number
 *      pclTblRegs  : PCL Table Registers array
 *      cosRegsNum  : Class of Service Re-marking Table Register number
 *      cosRegs     : Class of Service Re-marking Table Register array
 *      trunkRegsNum : Trunk Member Table Register number
 *      trunkRegs: Trunk Member Table Register array
 *      countRegs   : Counter block registers registers array
 *      cpuTrap     : Trap/Mirror to CPU , QoS Attributes Register.
 * Comments:
 * For all registers from this struct Bits 23:28 == 5
 *
 *  genRegs  -   Registers with address mask 0x1D700000 pattern 0x00000000
 *               Gen regs num = 0x1C8 / 4 + 1
 *
 *  mplsRegs  -  Registers with address mask 0x1D2F000 pattern 0x00000000
 *               mplsRegs = 0x178 / 4 + 1
 *
 *  dscpRegs  -  Registers with address mask 0x1D22000 pattern 0x00020000
 *               dscp regs num = 0x3C / 4 + 1
 *
 *  pclTblRegs-  Registers with address mask 0x1D24000 pattern 0x00040000
 *               pclRegsNum = 0x8FFC / 4 + 1
 *
 *  cosRegs    - Registers with address mask 0x1D26000 pattern 0x00060000
 *               cosRegsNum = 0x3DC / 4 + 1
 *
 *  trunkRegs  - Registers with address mask 0x1D28000 pattern 0x00080000
 *               trunkRegsNum = 0x1EC / 4 + 1
 *
 *  countRegs  - Registers with address mask 0x1D310000 pattern 0x00000000
 *               Count regs num = 0x118 / 4 + 1
 *
 *  tcRegs      - Registers with address mask 0x1D700000 pattern 0x00000000
 *               TC regs num = 0xFF / 4 + 1
 *
 *  tcamTblReg - Registers with address mask 0x02e20000 pattern 0x02e20000
 *               including the range of data and range of masks
 *  tcamTblRegNum = 0xfffc/4+1
 *
 *  pceActionsTblReg -Registers with address mask 0x02e34000 pattern 0x02e34000
 *  pceActionsTblRegNum = 0x3ffc/4+1
 *
 */

typedef struct {
    GT_U32                  genRegsNum;
    SMEM_REGISTER         * genRegs;

    GT_U32                  mplsRegsNum;
    SMEM_REGISTER         * mplsRegs;

    GT_U32                  dscpRegsNum;
    SMEM_REGISTER         * dscpRegs;

    GT_U32                  pclTblRegsNum;
    SMEM_REGISTER         * pclTblRegs;

    GT_U32                  cosRegsNum;
    SMEM_REGISTER         * cosRegs;

    GT_U32                  trunkRegsNum;
    SMEM_REGISTER         * trunkRegs;

    GT_U32                  countRegsNum;
    SMEM_REGISTER         * countRegs;

    GT_U32                  count2RegsNum;
    SMEM_REGISTER         * count2Regs;

    GT_U32                  extMemRegsNum;
    SMEM_REGISTER         * extMemRegs;

    GT_U32                  tcamTblRegNum;
    SMEM_REGISTER         * tcamTblReg;

    GT_U32                  pceActionsTblRegNum;
    SMEM_REGISTER         * pceActionsTblReg;

    GT_U32                  cosLookupTblRegNum;
    SMEM_REGISTER         * cosLookupTblReg;

    GT_U32                  cosTablesRegNum;
    SMEM_REGISTER         * cosTablesReg;

    GT_U32                  dscpDptoCosMarkRemarkTableRegNum;
    SMEM_REGISTER         * dscpDptoCosMarkRemarkTableReg;

    GT_U32                  internalInlifTblRegNum;
    SMEM_REGISTER         * internalInlifTblReg;

    GT_U32                  flowTemplateHashConfigTblNum;
    SMEM_REGISTER         * flowTemplateHashConfigTbl;

    GT_U32                  flowTemplateHashSelectTblNum;
    SMEM_REGISTER         * flowTemplateHashSelectTbl;

    GT_U32                  flowTemplateSelectTblNum;
    SMEM_REGISTER         * flowTemplateSelectTbl;

}TIGER_GM_LX_UNIT_MEM;

/*
 * Typedef: struct TIGER_GM_BUF_MNG_MEM
 * Description:
 *      Describe a device's buffer management registers memory object.
 *
 * Fields:
 *      bmRegsNum  : General buffer management registers number
 *      bmRegs     : General buffer management registers
 *      epfRegsNum : External Processor FIFO register number
 *      epfRegs    : External Processor FIFO register
 *      egrRegsNum : Egress Mirrored Ports registers number.
 *      egrRegs    : Egress Mirrored Ports registers array.
 *******************************************************************************
 *  registers needed for the twist_C (simulation used twist c to simulate the
 *  SAMBA in order to work the classifier
 *******************************************************************************
 *      trunkRegsNum : Trunk Member Table Register number
 *      trunkRegs: Trunk Member Table Register array
 *
 * Comments:
 * For all registers from this struct Bits 23:28 == 6
 *
 *  bmRegs       - Registers with address mask 0x1CF10000 pattern 0x00010000
 *                    eqRegs = 0x4004 / 4 + 1
 *
 *  epfRegs      - Registers with address mask 0x1CF20000 pattern 0x00020000
 *                    eqRegs = 0x4004 / 4 + 1
 *
 *  egrRegs      - Registers with address mask 0x1CF04000 pattern 0x00004000
 *                    eqRegs = 0x28 / 4 + 1
 *
 *  trunkRegs  - Registers with address mask 0x03000000 pattern 0x03000000
 *               and mask 0x000fff00 pattern 0x00000000
 *               trunkRegsNum = 0x1EC / 4 + 1
 *
 *
 *  cpuTrapReg - Register with address mask 0x030020FF pattern 0x030020FC
 *  cpuTrapRegNum - cpuTrapRegNum 0x0FC/4 + 1
 *
 */

typedef struct {
    GT_U32                  bmRegsNum;
    SMEM_REGISTER         * bmRegs;
    GT_U32                  epfRegsNum;
    SMEM_REGISTER         * epfRegs;
    GT_U32                  egrRegsNum;
    SMEM_REGISTER         * egrRegs;
    GT_U32                  trunkRegsNum;/* for twist C*/
    SMEM_REGISTER         * trunkRegs;   /* for twist C*/
    GT_U32                  cpuTrapRegNum;
    SMEM_REGISTER         * cpuTrapReg;
}TIGER_GM_BUF_MNG_MEM;

/*
 * Typedef: struct TIGER_GM_MAC_MEM
 * Description:
 *      MAC Related Registers
 *
 *
 * Fields:
 *      genRegsNum : General MAC Related Registers number
 *      genRegs    : General MAC Related Registers array
 *
 * Comments:
 *      For all registers from this struct Bits 23:28 == 7
 *
 *      genRegs     - Registers with address mask 0x1C7FFF00 pattern 0x00000000
 *                    genRegs = 0xEC / 4 + 1
 *
 *      sFlowRegs   - Registers with address mask 0x1C7FF100 pattern 0x00000100
 *                    sflowRegsNum = 0x0C / 4 + 1
 */

typedef struct {
    GT_U32                  genRegsNum;
    SMEM_REGISTER         * genRegs;
    GT_U32                  sflowRegsNum;
    SMEM_REGISTER         * sflowRegs;
    GT_U32                  sflowFifo[TIGER_SFLOW_FIFO_SIZE];
}TIGER_GM_MAC_MEM;

/*
 * Typedef: struct TIGER_GM_MAC_FDB_MEM
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
 *  macTblRegNum = 4 * 1024 * 4 (4K entries, 4 words in the entry)
 *
 *
 */

typedef struct {
    GT_U32                  macTblRegsNum;
    SMEM_REGISTER         * macTblRegs;
}TIGER_GM_MAC_FDB_MEM;

/*
 * Typedef: struct TIGER_GM_VLAN_TBL_MEM
 *
 * Description:
 *      Describe a device's Bridge registers and FDB.
 *
 * Fields:
 *      vlanTblRegNum    : Registers number for Vlan Table
 *      vlanTblRegs      : Vlan Table Registers.
 *      mcastTblRegNum   : Registers number for Mcast Table
 *      mcastTblRegs     : Mcast Table Registers.
 *
 * Comments:
 * For all registers from this struct Bits 23:28 == 20
 *
 *  mcastTblRegs   base  addr 0x0A000000
 *
 *  mcastTblRegsNum = 4 * 1024 * 16
 *
 *  mcastTblRegNum   base  addr 0x0A000FFF
 *
 *  mcastTblRegs = 4 * 1024 * 2 * 8
 */
typedef struct {
    GT_U32                  vlanTblRegNum;
    SMEM_REGISTER         * vlanTblRegs;
    GT_U32                  mcastTblRegsNum;
    SMEM_REGISTER         * mcastTblRegs;
}TIGER_GM_VLAN_TBL_MEM;

/*
 * Typedef: struct TIGER_GM_WSRAM_MEM
 *
 * Description:
 *      Wide Sram D-unit (sr0 unit) External interfaces
 *
 * Fields:
 *      wSramSize     : WSRAM memory size
 *      wSramMem      : WSRAM memory pointer .
 *
 * Comments:
*/
typedef struct {
    SMEM_WSRAM                * wSram;
    GT_U32                      wSramSize;
}TIGER_GM_WSRAM_MEM;


/*
 * Typedef: enum TIGER_GM_NARROW_SRAM_ENT
 *
 * Comments: define the different narrow sram types
 *
 */

typedef enum
{
    TIGER_GM_NSRAM_ONE_FOUR_INTERNAL_E = 0,
    TIGER_GM_NSRAM_ALL_EXTERNAL_E = 1,
    TIGER_GM_NSRAM_ONE_TWO_INTERNAL_E = 2,
    TIGER_GM_NSRAM_THREE_FOUR_INTERNAL_E = 3,
    TIGER_GM_NSRAM_ALL_INTERNAL_E = 6
}TIGER_GM_NSRAM_MODE_ENT;



/*
 * Typedef: struct TIGER_GM_NSRAM_MEM
 *
 * Description:
 *      Narrow Sram D-unit (sn unit) External interfaces
 *
 * Fields:
 *      nSramSize       : NSRAM number for Vlan Table
 *      nSramMem        : NSRAM memory pointer .
 *
 * Comments:
*/
typedef struct {
    SMEM_NSRAM               *nSram;
    GT_U32                    nSramSize;
}TIGER_GM_NSRAM_MEM;

/*
 * Typedef: struct TIGER_GM_FDRAM_MEM
 *
 * Description:
 *      Flow Dram D-unit (fd unit) External interfaces
 *
 * Fields:
 *      fDramMem        : FDRAM number for Vlan Table
 *      fDramMem        : FDRAM memory pointer .
 *
 * Comments:
*/
typedef struct {
    SMEM_FDRAM                * fDram;
    GT_U32                      fDramSize;
}TIGER_GM_FDRAM_MEM;

/*
 * Typedef: struct TIGER_GM_AUQ_MEM
 *
 * Description:
 *      Address update message interface
 *
 * Fields:
 *     auqBase        - PCI base address space
 *     auqBaseValid   - TRUE  upon every new load of shadow to base.
 *                      FALSE upon write to the last write of AU to given section.
 *     auqShadow      - PCI shadow address space,
 *     auqShadowValid - TRUE upon every new shadow register update from PCI.
 *                      FALSE upon every load of shadow to base.
 *     auqOffset      - increment at 16 Bytes at every new AU write.
 *
 * Comments:
*/
typedef struct {
    GT_U32                      auqBase;
    GT_BOOL                     auqBaseValid;
    GT_U32                      auqShadow;
    GT_BOOL                     auqShadowValid;
    GT_U32                      auqOffset;
    GT_BOOL                     baseInit;
}TIGER_GM_AUQ_MEM;

/*
 * Typedef: struct TIGER_GM_PCI_MEM
 *
 * Description:
 *      PCI Registers.
 *
 * Fields:
 *      genRegsNum      : Common registers number.
 *      genRegs         : Common registers number, index is port and group
 *
 *
 * Comments:
 *
 */

typedef struct {
    GT_U32                  pciRegsNum;
    SMEM_REGISTER         * pciRegsRegs;
    GT_U32                  pciInterRegsNum;
    SMEM_REGISTER         * pciInterRegs;
}TIGER_GM_PCI_MEM;

/*
 * Typedef: struct TIGER_GM_PHY_MEM
 *
 * Description:
 *      PHY related Registers.
 *
 * Fields:
 *      ieeeXauiRegsNum : IEEE standard XAUE registers number.
 *      ieeeXauiRegs    : IEEE standard XAUE registers.
 *      extXauiRegsNum  : Marvell Extention XAUE registers number.
 *      extXauiRegs     : Marvell Extention standard XAUE registers.
 *
 *
 * Comments:
 *
 */

typedef struct {
    GT_U32                   ieeeXauiRegsNum;
    SMEM_PHY_REGISTER     *  ieeeXauiRegs;
    GT_U32                   extXauiRegsNum;
    SMEM_PHY_REGISTER     *  extXauiRegs;
}TIGER_GM_PHY_MEM;


/*
 * Typedef: struct TIGER_GM_DEV_MEM_INFO
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
 *      phyMem          : PHY related registers.
 *
 * Comments:
 */

typedef struct {
    SMEM_SPEC_FIND_FUN_ENTRY_STC specFunTbl[64];
    TIGER_GM_GLOBAL_MEM            globalMem;
    TIGER_GM_BUF_MNG_MEM           bufMngMem;
    TIGER_GM_MAC_MEM               macMem;
    TIGER_GM_PORT_MEM              portMem;
    TIGER_GM_BRDG_MNG_MEM          bridgeMngMem;
    TIGER_GM_EGR_MEM               egressMem;
    TIGER_GM_LX_UNIT_MEM           lxMem;
    TIGER_GM_MAC_FDB_MEM           macFbdMem;
    TIGER_GM_VLAN_TBL_MEM          vlanTblMem;
    /* External memory */
    TIGER_GM_WSRAM_MEM             wsramMem;
    TIGER_GM_FDRAM_MEM             fdramMem;

    /* Narrow srams */
    TIGER_GM_NSRAM_MEM             nsramsMem[4];

    /* Address Update messages */
    TIGER_GM_AUQ_MEM               auqMem;
    /* PCI registers */
    TIGER_GM_PCI_MEM               pciMem;
    /* PHY related registers */
    TIGER_GM_PHY_MEM               phyMem;
}TIGER_GM_DEV_MEM_INFO;

/*******************************************************************************
*   smemTigerGMInit
*
* DESCRIPTION:
*       Init memory module for a Twist device.
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
void smemTigerGMInit
(
    IN SKERNEL_DEVICE_OBJECT * deviceObj
);

/*******************************************************************************
*   smemTigerGMFindMem
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
void * smemTigerGMFindMem
(
    IN SKERNEL_DEVICE_OBJECT * deviceObj,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32                  address,
    IN GT_U32                  memSize,
    OUT SMEM_ACTIVE_MEM_ENTRY_STC ** activeMemPtrPtr
);

/*******************************************************************************
*   smemTigerFindMem
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
void * smemTigerFindMem
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


