/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* smemTiger.h
*
* DESCRIPTION:
*       Data definitions for Tiger memories.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 5 $
*
*******************************************************************************/
#ifndef __smemTigerh
#define __smemTigerh

#include <asicSimulation/SKernel/smem/smem.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define TIGER_SFLOW_FIFO_SIZE       32
/***********************************************************************
*   0 = Global Configuration, Host Management registers
*   1 = Configuration registers for Ports (Group Of Ports 0).
*   2 = Configuration Registers for Ports (Group Of Ports 1)
*   3 = Egress Queuing, Scheduling registers, Trunk configuration
*   4 = Ingress Layer 2 Bridge registers
*   5 = InLIF, Ingress Policing, IP Routing, Ingress Policy registers
*   6 = Buffer Management register
*   7 = Global MAC, LED, SMI and PHY registers
*   8 = TWSI registers
*   11-9 = Reserved
*   12 = MAC Table Memory
*   19-13 = Reserved
*   20 = VLAN Table Memory
*   24 = Internal Control, SRAM
*   63-21 = Reserved
*
***********************************************************************/
/*
 * Typedef: struct TIGER_GLOBAL_MEM
 *
 * Description:
 *      Global Configuration, Host Management, PCI registers
 *
 * Fields:
 *      globRegsNum    : Global registers number.
 *      globRegs       : Global registers array with size of globRegsNum.
 *      hostMngRegsNum : Host Management register number.
 *      hostMngRegs    : Host Management register array with size of hostMngRegsNum.
 *
 * Comments:
 *  For all registers from this struct Bits 23:28 == 0
 *
 *  globRegs        - Registers with address mask 0x1FFFFF00 pattern 0x00000000
 *                  globRegsNum = 0x74 / 4 + 1
 *
 *  twsiIntRegsNum - Registers with address mask 0x1FFFFF00 pattern 0x00400000
 *                   twsiIntRegsNum = 0x1C / 4 + 1
 *
 *  sdmaRegsNum     - Registers with address mask 0x1FFFF000 pattern 0x00002000
 *                   sdmaRegsNum = 0x864 / 4 + 1
 *
 */

typedef struct {
    GT_U32                  globRegsNum;
    SMEM_REGISTER         * globRegs;
    GT_U32                  twsiIntRegsNum;
    SMEM_REGISTER         * twsiIntRegs;
    GT_U32                  sdmaRegsNum;
    SMEM_REGISTER         * sdmaRegs;
}TIGER_GLOBAL_MEM;

/*
 * Typedef: struct TIGER_PORT_MEM
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
}TIGER_PORT_MEM;

/*
 * Typedef: struct TIGER_EGR_MEM
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
 *  mllTableRegs  - Control memory and MLL memory access with address
 *                  mask 0x07ff0000 pattern 0x000c0000
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
    GT_U32                  mllTableRegsNum;
    SMEM_REGISTER         * mllTableRegs;
}TIGER_EGR_MEM;

/*
 * Typedef: struct TIGER_BRDG_MNG_MEM
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
}TIGER_BRDG_MNG_MEM;

/*
 * Typedef: struct TIGER_LX_UNIT_MEM
 *
 * Description:
 *      InLIF, Ingress Policing, IP Routing, Ingress Policy registers
 *
* Comments:
 * For all registers from this struct Bits 23:28 == 5
 *
 *  genRegs  -   Registers with address mask 0x007f0000 pattern 0x00000000 (0x0280....)
 *               genRegs num = 0xfff / 4 + 1
 *
 *  ipFlowRegs - Registers with address mask 0x007f0000 pattern 0x00400000 (0x02c0....)
 *               ipFlowRegs num = 0x1f / 4 + 1
 *
 *  ftHushRegs - Registers with address mask 0x007f0000 pattern 0x00420000 (0x02c2....)
 *               ftHushRegs num = 0x1f / 4 + 1
 *
 *  userDefinedRegs - Registers with address mask 0x007f0000 pattern 0x00440000 (0x02c4....)
 *               ingrPolicyRegs num = 0x2ff / 4 + 1
 *
 *  inLifPerPortTblRegs - Registers with address mask 0x007f0000 pattern 0x00460000 (0x02c6....)
 *               inLifRegs num = 0x3ff / 4 + 1
 *
 *  ipFlowActTblRegs - Registers with address mask 0x007f0000 pattern 0x00480000 (0x02c8....)
 *               ipFlow1Regs num = 0x3ff / 4 + 1
 *
 *  dscp2CoSRegs - Registers with address mask 0x007f0000 pattern 0x004a0000 (0x02ca....)
 *               dscp2CoSRegs num = 0x1ff / 4 + 1
 *
 *  ingrPolicyMngRegs - Registers with address mask 0x007f0000 pattern 0x004c0000 (0x02cc....)
 *               ingrPolicyMngRegs num = 0x1ff / 4 + 1
 *
 *  ipv4MngRegs - Registers with address mask 0x007f0000 pattern 0x004e0000 (0x02ce....)
 *               ipv4MngRegs num = 0x3ff / 4 + 1
 *
 *  ingrPolicyRemTblRegs - Registers with address mask 0x007f0000 pattern 0x00560000 (0x02d6....)
 *               ingrPolicyRemTblRegs num = 0x3ff / 4 + 1
 *
 *  inLifPerVlanTblRegs - Registers with address mask 0x007f0000 pattern 0x00580000 (0x02d8....)
 *               inLifTblRegs num = 0x1fff / 4 + 1
 *
 *  sramRegs - Registers with address mask 0x007f0000 pattern 0x00600000 (0x02e0....)
 *               sramRegs num = 0x1ff / 4 + 1
 *
 *  tcam0MaskAccRegs - Registers with address mask 0x007f0000 pattern 0x00620000 (0x02e2....)
 *               tcam0Regs num = 0x1000 / 4 + 1
 *
 *  tcam1MaskAccRegs - Registers with address mask 0x007f0000 pattern 0x00640000 (0x02e4....)
 *               tcam1Regs num = 0x1000 / 4 + 1
 *
 *  tcam0ValueAccRegs
 *             - Registers with address mask 0x007f0000 pattern 0x00650000 (0x02e3....)
 *                tcam0AccessOffsetRegs = 0x1000 / 4 + 1
 *
 *  tcam1ValueAccRegs
 *             - Registers with address mask 0x007f0000 pattern 0x00650000 (0x02e5....)
 *                tcam1AccessOffsetRegs = 0x1000 / 4 + 1
 *
 *  tcamAccessBuffer
 *             - Buffer to store PCE entry (access memory for inderect write)
*/

typedef struct {
    GT_U32                  genRegsNum;
    SMEM_REGISTER         * genRegs;
    GT_U32                  ipFlowRegsNum;
    SMEM_REGISTER         * ipFlowRegs;
    GT_U32                  ftHushRegsNum;
    SMEM_REGISTER         * ftHushRegs;
    GT_U32                  userDefinedRegsNum;
    SMEM_REGISTER         * userDefinedRegs;
    GT_U32                  inLifPerPortTblRegsNum;
    SMEM_REGISTER         * inLifPerPortTblRegs;
    GT_U32                  ipFlowActTblRegsNum;
    SMEM_REGISTER         * ipFlowActTblRegs;
    GT_U32                  dscp2CoSRegsNum;
    SMEM_REGISTER         * dscp2CoSRegs;
    GT_U32                  ingrPolicyMngRegsNum;
    SMEM_REGISTER         * ingrPolicyMngRegs;
    GT_U32                  ipv4MngRegsNum;
    SMEM_REGISTER         * ipv4MngRegs;
    GT_U32                  ingrPolicyRemTblRegsNum;
    SMEM_REGISTER         * ingrPolicyRemTblRegs;
    GT_U32                  inLifPerVlanTblRegsNum;
    SMEM_REGISTER         * inLifPerVlanTblRegs;
    GT_U32                  sramRegsNum;
    SMEM_REGISTER         * sramRegs;
    GT_U32                  tcam0MaskAccRegsNum;
    SMEM_REGISTER         * tcam0MaskAccRegs;
    GT_U32                  tcam1MaskAccRegsNum;
    SMEM_REGISTER         * tcam1MaskAccRegs;
    GT_U32                  tcam0ValueAccRegsNum;
    SMEM_REGISTER         * tcam0ValueAccRegs;
    GT_U32                  tcam1ValueAccRegsNum;
    SMEM_REGISTER         * tcam1ValueAccRegs;
    GT_U32                  tcamAccessAddr;
}TIGER_LX_UNIT_MEM;

/*
 * Typedef: struct TIGER_TCAM_MEM
 * Description:
 *      Describe a device's TCAM memory object.
 *
 * Fields:
 *      tcam0MaskRegsNum    rule mask number of entries in TCAM0
 *      tcam0MaskRegs       pointer to rule mask entries in TCAM0
 *      tcam1MaskRegsNum    rule mask number of entries in TCAM1
 *      tcam1MaskRegs       pointer to rule mask entries in TCAM1
 *      tcam0ValueRegsNum   rule value number of entries in TCAM0
 *      tcam0ValueRegs      pointer to rule value entries in TCAM0
 *      tcam1ValueRegsNum   rule value number of entries in TCAM1
 *      tcam1ValueRegs      pointer to rule value entries in TCAM1
*
 * Comments:
 *      For all registers from this struct Bits 23:28 == 21
 *
 *  tcam0MaskRegs   - Registers with address mask 0x0FFF0000 pattern 0x0A820000
 *  tcam1MaskRegs   - Registers with address mask 0x0FFF0000 pattern 0x0A840000
 *  tcam0ValueRegs  - Registers with address mask 0x0FFF0000 pattern 0x0A830000
 *  tcam1ValueRegs  - Registers with address mask 0x0FFF0000 pattern 0x0A850000
 */
typedef struct {
    GT_U32                  tcam0MaskRegsNum;
    SMEM_REGISTER         * tcam0MaskRegs;
    GT_U32                  tcam1MaskRegsNum;
    SMEM_REGISTER         * tcam1MaskRegs;
    GT_U32                  tcam0ValueRegsNum;
    SMEM_REGISTER         * tcam0ValueRegs;
    GT_U32                  tcam1ValueRegsNum;
    SMEM_REGISTER         * tcam1ValueRegs;
}TIGER_TCAM_MEM;

/*
 * Typedef: struct TIGER_BUF_MNG_MEM
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
 *      trunkConfRegsNum: Trunk Member Table Configuration registers number
 *      trunkConfRegs: Trunk Member Table Configuration registers array
 *      trapMirrToCpuNum: Trap/Mirror to CPU, QoS Attributes registers number
 *      trapMirrToCpu: Trap/Mirror to CPU, QoS Attributes registers array
 *
 * Comments:
 * For all registers from this struct Bits 23:28 == 6
 *
 *  bmRegs       - Registers with address mask 0x1CF10000 pattern 0x00010000
 *                    eqRegs = 0x4004 / 4 + 1
 *
 *  epfRegs      - Registers with address mask 0x7ff0000 pattern 0x00020000
 *                    eqRegs = 0x4004 / 4 + 1
 *
 *  cpuQoSAttrRegs - Registers with address mask 0x007ff000 pattern 0x00002000
 *                    eqRegs = 0xfc / 4 + 1
 *
 *  vlanMemConfRegs - Registers with address mask 0x007ff000 pattern 0x00034000
 *                    eqRegs = 0x7c / 4 + 1
 *
 *  egrRegs      - Registers with address mask 0x007ff000 pattern 0x00004000
 *                    eqRegs = 0x28 / 4 + 1
 *
 *  trunkConfRegs- Registers with address mask 0x01fff000 pattern 0x00000000
 *                    eqRegs = 0x2ff / 4 + 1
 *
 *  mcBufCountRegs- Registers with address mask 0x007ff000 pattern 0x00040000
 *                    eqRegs = 0x2fff / 4 + 1
 */

typedef struct {
    GT_U32                  bmRegsNum;
    SMEM_REGISTER         * bmRegs;
    GT_U32                  epfRegsNum;
    SMEM_REGISTER         * epfRegs;
    GT_U32                  cpuQoSAttrRegsNum;
    SMEM_REGISTER         * cpuQoSAttrRegs;
    GT_U32                  vlanMemConfRegsNum;
    SMEM_REGISTER         * vlanMemConfRegs;
    GT_U32                  egrRegsNum;
    SMEM_REGISTER         * egrRegs;
    GT_U32                  trunkConfRegsNum;
    SMEM_REGISTER         * trunkConfRegs;
    GT_U32                  mcBufCountRegsNum;
    SMEM_REGISTER         * mcBufCountRegs;
}TIGER_BUF_MNG_MEM;

/*
 * Typedef: struct TIGER_MAC_MEM
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
 *
 */

typedef struct {
    GT_U32                  genRegsNum;
    SMEM_REGISTER         * genRegs;
    GT_U32                  sflowRegsNum;
    SMEM_REGISTER         * sflowRegs;
    GT_U32                  sflowFifo[TIGER_SFLOW_FIFO_SIZE];
}TIGER_MAC_MEM;

/*
 * Typedef: struct TIGER_MAC_FDB_MEM
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
}TIGER_MAC_FDB_MEM;

/*
 * Typedef: struct TIGER_VLAN_TBL_MEM
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
}TIGER_VLAN_TBL_MEM;

/*
 * Typedef: struct TIGER_WSRAM_MEM
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
}TIGER_WSRAM_MEM;

/*
 * Typedef: enum TIGER_NARROW_SRAM_ENT
 *
 * Comments: define the different narrow sram types
 *
 */

typedef enum
{
    TIGER_NSRAM_ONE_FOUR_INTERNAL_E = 0,
    TIGER_NSRAM_ALL_EXTERNAL_E = 1,
    TIGER_NSRAM_ONE_TWO_INTERNAL_E = 2,
    TIGER_NSRAM_THREE_FOUR_INTERNAL_E = 3,
    TIGER_NSRAM_ALL_INTERNAL_E = 6
}TIGER_NSRAM_MODE_ENT;

/*
 * Typedef: enum TIGER_NARROW_SRAM_ENT
 *
 * Comments: define the different narrow sram types
 *
 */

typedef enum
{
    TIGER_NSRAM_E = 0x30000000,
    TIGER_NSRAM_INTERNAL_0_E = 0x20000000,
    TIGER_NSRAM_INTERNAL_1_E = 0x28000000,
    TIGER_NSRAM_EXTERNAL_0_E = 0x30000000,
    TIGER_NSRAM_EXTERNAL_1_E = 0x38000000
}SMEM_TIGER_NSRAM_BASE_ADDR_ENT;

/*
 * Typedef: struct TIGER_NSRAM_MEM
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
}TIGER_NSRAM_MEM;

/*
 * Typedef: struct TIGER_AUQ_MEM
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
}TIGER_AUQ_MEM;

/*
 * Typedef: struct TIGER_PCI_MEM
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
}TIGER_PCI_MEM;

/*
 * Typedef: struct TIGER_PHY_MEM
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
}TIGER_PHY_MEM;


/*
 * Typedef: struct TIGER_DEV_MEM_INFO
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
    TIGER_GLOBAL_MEM            globalMem;
    TIGER_BUF_MNG_MEM           bufMngMem;
    TIGER_MAC_MEM               macMem;
    TIGER_PORT_MEM              portMem;
    TIGER_BRDG_MNG_MEM          bridgeMngMem;
    TIGER_EGR_MEM               egressMem;
    TIGER_LX_UNIT_MEM           lxMem;
    TIGER_TCAM_MEM              tcamMem;
    TIGER_MAC_FDB_MEM           macFbdMem;
    TIGER_VLAN_TBL_MEM          vlanTblMem;
    /* External memory */
    TIGER_WSRAM_MEM             wsramMem;
    /* Narrow srams */
    TIGER_NSRAM_MEM             nsramsMem[4];

    /* Address Update messages */
    TIGER_AUQ_MEM               auqMem;
    /* PCI registers */
    TIGER_PCI_MEM               pciMem;
    /* PHY related registers */
    TIGER_PHY_MEM               phyMem;
}TIGER_DEV_MEM_INFO;

/*
 * typedef enum TIGER_PCL_PCE_SIZE_TYPE
 *
 * Description:
 *    PCE size types
 *
 *    TIGER_PCL_PCE_SIZE_STANDARD  - standard PCE used for PCL
 *    TIGER_PCL_PCE_SIZE_EXTENDED  - extended PCE used for PCL
 *    TIGER_PCL_PCE_SIZE_DEFAULT   - for not 98EX1x6, 98EX1x5,
 *                                  or for Homogeneous mode
 *
 */
typedef enum
{
    TIGER_PCL_PCE_SIZE_STANDARD,
    TIGER_PCL_PCE_SIZE_EXTENDED,
    TIGER_PCL_PCE_SIZE_DEFAULT

} TIGER_PCL_PCE_SIZE_TYPE;

#define SMEM_TIGER_NOT_VALID_ADDRESS        0xCFCFCFCF
#define SMEM_TIGER_TCAM_RULE_WORDS          12
/*******************************************************************************
*   smemTigerInit
*
* DESCRIPTION:
*       Init memory module for a Tiger device.
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
void smemTigerInit
(
    IN SKERNEL_DEVICE_OBJECT * deviceObj
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
    IN SKERNEL_DEVICE_OBJECT * deviceObj,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32                  address,
    IN GT_U32                  memSize,
    OUT SMEM_ACTIVE_MEM_ENTRY_STC ** activeMemPtrPtr
);

/*******************************************************************************
*  smemTigerNsramRead
*
* DESCRIPTION:
*        Read word from Narow Sram
* INPUTS:
*       deviceObjPtr   - device object PTR.
*       nsramBaseAddr  - base SRAM address
*       sramAddrOffset - NARROW address offset
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
GT_U32 smemTigerNsramRead
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SMEM_TIGER_NSRAM_BASE_ADDR_ENT nsramBaseAddr,
    IN GT_U32 sramAddrOffset
);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __smemh */


