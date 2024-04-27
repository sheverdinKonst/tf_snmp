/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* smemCheetah3.h
*
* DESCRIPTION:
*       Data definitions for Cheetah3 memories.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 34 $
*
*******************************************************************************/
#ifndef __smemCht3h
#define __smemCht3h

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <asicSimulation/SKernel/smem/smemCheetah2.h>

/* Central counters blocks number */
#define     CNC_CNT_BLOCKS(dev)  ((dev)->cncBlocksNum)

/***********************************************************************
*    0- Global Registers, SDMA registers, Master XSMI registers and TWSI
*       registers.
*    1 - 2  reserved
*    3- Transmit Queue registers
*    4- Ethernet Bridge registers
*    5- IP Routing Registers
*    6- Buffer Management register
*    7- reserved
*    8- Ports group0 configuration registers (port0 through port5),
*       LEDs interface0 configuration registers and Master SMI
*       interface0 registers
*    9- Ports group1 configuration registers (port6 through port11) and
*       LEDs interface0.
*    10-Ports group2 configuration registers (port12 through port17),
*       LEDs interface1 configuration registers and Master SMI interface1
*       registers
*    11-Ports group3 configuration registers (port18through port23) and
*       LEDs interface1
*    12-MAC Table Memory
*    13-Internal Buffers memory Bank0 address space
*    14-Internal Buffers memory Bank1 address space
*    15-Buffers memory block configuration registers
*    20-VLAN Table configuration registers and VLAN Table address space.
*    21-Tri-Speed Ports MAC, CPU Port MAC, and 1.25 Gbps SERDES
*           Configuration Registers Map Table.
*    22-Pre-Egress registers
*    23-PCL registers and TCAM memory space
*    24-Policer registers and meters memory space
*    25-MLL Engine registers table map
*    26-63  Reserved.
*
***********************************************************************/
/*
 * Typedef: struct CHT3_GLOBAL_MEM
 *
 * Description:
 *      Global Registers, TWSI, CPU port, SDMA and PCI internal registers.
 *
 * Fields:
 *      globRegNum    : Global registers number.
 *      globReg       : Global registers array with size of globRegNum.
 *      sdmaRegNum    : SDMA register number.
 *      sdmaReg       : SDMA register array with size of sdmaRegNum.
 *      twsiRegNum    : TWSI configuration register number.
 *      twsiReg       : TWSI registers array with the size of twsiRegNum.
 *      pexReg        : PEX registers array with the size of pexRegNum.
 *      epclConfigTblReg: Egress PCL configuration table with size of epclConfigTblRegNum
 *      macSaTblReg   : Port/VLAN MAC SA Table with size macSaTblRegNum
 *
 * Comments:
 *  For all registers from this struct Bits 23:28 == 0
 *
 *  globReg    - Registers with address mask 0x1FFFF000 pattern 0x00000000
 *                  globRegNum = 0xfff / 4 + 1
 *
 *  sdmaReg - Registers with address mask 0x1FFFF000 pattern 0x00002000
 *                  sdmaRegNum = 150
 *
 *  twsiIntRegs - Registers with address mask 0x1FFFFF00 pattern 0x00400000
 *                  twsiIntRegs = 0x1C
 *
 *  pexReg      - Registers with address mask 0x1FFFFF00 pattern 0x00001000
 *                  pexRegNum = 0xfff / 4 + 1
 *
 *  epclConfigTblReg  - Registers with address mask 0xFFFFF000 pattern 0x00018000
 *                  epclConfigTblRegNum = 4160
 *
 *  macSaTblReg - MAC SRC address table memory pointer
 *
 *
 */

typedef struct {
    GT_U32                  globRegNum;
    SMEM_REGISTER         * globReg;

    GT_U32                  sdmaRegNum;
    SMEM_REGISTER         * sdmaReg;

    GT_U32                  twsiIntRegsNum;
    SMEM_REGISTER         * twsiIntRegs;

    GT_U32                  pexRegNum;
    SMEM_REGISTER         * pexReg;

    GT_U32                  epclConfigTblRegNum;
    SMEM_REGISTER         * epclConfigTblReg;

    GT_U32                  macSaTblRegNum;
    SMEM_REGISTER         * macSaTblReg;

}CHT3_GLOBAL_MEM;

/*
 * Typedef: struct CHT3_EGR_MEM
 *
 * Description:
 *      Egress, Transmit Queue and VLAN Configuration Register Map Table,
 *      Port TxQ Configuration Register, Hyper.GStack Ports MIB Counters.
 *
 * Fields:
 *      egrGenRegNum    : Common registers number.
 *      egrGenReg       : Common registers number, index is port and group.
 *
 *      txqInternRegNum : TxQ interanal registers number.
 *      txqInternReg    : TxQ interanal registers.
 *
 *      portWdRegNum    : Port Watchdog Configuration Register Number.
 *      portWdReg       : Port Watchdog Configuration Register.
 *
 *      egrMibCntRegNum : Egress MIB counter number.
 *      egrMibCntReg    : Egress MIB counter.
 *
 *      hprMibCntRegNum : Hyper.GStack Ports MIB Counters number.
 *      hprMibCntReg    : Hyper.GStack Ports MIB Counters.
 *
 *      egrRateShpRegNum: Egress rate shape register number.
 *      egrRateShpReg   : Egress rate shape register.
 *
 *      xsmiRegNum      : Master XSMI Interface Register Map Table number.
 *      xsmiReg         : Master XSMI Interface Register Map Table
 *
 * Comments:
 * For all registers from this struct Bits 23:28 == 3
 *
 *  egrGenReg    - Registers with address mask 0x1FF00000 pattern 0x01800000
 *                 genRegNum = 308
 *
 *  txQInternReg - Registers with address mask 0x1FF00000 pattern 0x04840000,
 *                 0x04880000, 0x048C0000, 0x04900000
 *                 txQInternRegNum = 512(8 TC * 64 ports)
 *
 *  egrFilterReg - Registers with address mask 0x1FFFF000 pattern 0x01A40080
 *                 trQueueRegNum = 2032
 *
 *  portWdReg    - Registers with address mask 0x1FFF0000 pattern 0x01A80000
 *                 portWdRegNum = 64
 *
 *  egrMibCntReg - Registers with address mask 0x1FFFFF00 pattern 0x01B40100
 *                 egrMibCntReg = 120
 *
 *  egrRateShpReg- Registers with address mask 0x1FF00000 pattern 0x01A00000
 *                 egrRateShpRegNum = 512(8 TC * 64 ports)
 *
 *  hprMibCntReg - Registers with address mask 0x1FF00000 pattern 0x01C00000
 *                 hprMibCntRegNum = 64
 *
 *  stackTcRemapMapReg - Registers with address mask 0x1FFF0000 pattern 0x01E80000
 *                 hprMibCntRegNum = 8
 *
 *
 *  xsmiReg      - Registers with address mask 0x1FFF0000 pattern 0x01CC0000
 *                 xsmiRegNum = 2
 *
 *
 *  txSniffCntr =  Register of tx sniffering
 */
typedef struct {
    GT_U32                  egrGenRegNum;
    SMEM_REGISTER         * egrGenReg;

    GT_U32                  txqInternalRegNum;
    SMEM_REGISTER         * txqInternalReg[4];

    GT_U32                  trQueueRegNum;
    SMEM_REGISTER         * trQueueReg;

    GT_U32                  portWdRegNum;
    SMEM_REGISTER         * portWdReg;

    GT_U32                  egrMibCntRegNum;
    SMEM_REGISTER         * egrMibCntReg;

    GT_U32                  hprMibCntRegNum;
    SMEM_REGISTER         * hprMibCntReg[4];

    GT_U32                  tailDropRegNum;
    SMEM_REGISTER         * tailDropReg;

    GT_U32                  egrRateShpRegNum;
    SMEM_REGISTER         * egrRateShpReg[9];

    GT_U32                  xsmiRegNum;
    SMEM_REGISTER         * xsmiReg;

    GT_U32                  egrStcTblRegNum;
    SMEM_REGISTER         * egrStcTblReg;

    GT_U32                  secondaryTargetPortMapRegNum;
    SMEM_REGISTER         * secondaryTargetPortMapReg;

    GT_U32                  stackTcRemapRegNum;
    SMEM_REGISTER         * stackTcRemapMapReg;

}CHT3_EGR_MEM;

/*
 * Typedef: struct CHT3_BRDG_MNG_MEM
 *
 * Description:
 *      Describe a device's Bridge Management registers memory object.
 *
 * Fields:
 *      bridgeGenRegNum   : Ethernet bridge registers number.
 *      bridgeGenReg      : Ethernet bridge registers.
 *
 * Comments:
 * For bridge generic register , this struct Bits 23:28 == 4
 * For MAC QOS Table , this struct bits 23:28 == 0
 *
 *  bridgeGenReg  -  Registers with address mask 0xFFFFFF00 pattern 0x02000200
 *                   genRegsNum = 262655
 */

typedef struct {
    GT_U32                  bridgeGenRegsNum;
    SMEM_REGISTER         * bridgeGenReg;
}CHT3_BRDG_MNG_MEM;

/*
 * Typedef: struct CHT3_BUF_MNG_MEM
 * Description:
 *      Describe a device's buffer management registers memory object and
 *      egress memory.
 *
 * Fields:
 *
 *      bmRegNum            : General buffer management registers number.
 *      bmReg               : General buffer management registers.
 *      bmCntrRegNum        : Buffer management controlled registers number.
 *      bmCntrReg           : Buffer management controlled registers.
 * Comments:
 * For all registers from this struct Bits 23:28 == 6
 *
 *  bmReg          - Registers with address mask 0xfffff000 pattern 0x03000000
 *                    bmRegNum = 196
 *
 *  bmCntrReg      - Registers with address mask 0xffff0000 pattern 0x03010000
 *                      bmLLRegNum = 4096
 */

typedef struct {
    GT_U32                  bmRegNum;
    SMEM_REGISTER         * bmReg;

    GT_U32                  bmCntrRegNum;
    SMEM_REGISTER         * bmCntrReg;

}CHT3_BUF_MNG_MEM;

/*
 * Typedef: struct CHT3_GOP_CONF_MEM
 * Description:
 *      LEDs, Tri-Speed Ports MIB Counters and Master SMI
 *      Configuration Registers.
 *
 * Fields:
 *      portGroupRegNum    : Group configuration registers numbers.
 *      portGroupReg       : Group configuration registers.
 *      xgPortGroupRegNum  : XG ports group configuration registers numbers.
 *      xgPortGroupReg     : XG ports group configuration registers.
 *      macMibCountRegNum  : MAC MIB counters registers numbers.
 *      macMibCountReg     : MAC MIB counters registers.
 *      xgMacMibCountRegNum: XG ports MAC MIB counters registers numbers.
 *      xgMacMibCountReg   : XG ports MAC MIB counters registers.
 *
 * Comments:
 *      For all registers from this struct Bits 23:28 == 8,9,10,11
 *
 *      portGroupReg[]
 *                 -  Registers with address mask 0x0000F000 pattern 0x4000
 *                    portGroupRegNum = 512
 *
 *      xgPortGroupRegNum[]
 *                 - Registers with address mask 0x0000F000 pattern 0x4000
 *                   xgPortGroupRegNum = 16
 *
 *      macMibCountReg[]
 *                 - Registers with address mask 0x000F0000 pattern 0x10000
 *                   macMibCountRegNum = 224
 *
 *      xgMacMibCountReg[] -
 *                 - Registers with address mask 0x000F0000 pattern 0x10000
 *                   xgMacMibCountRegNum = 64
 */

typedef struct {
    GT_U32                  portGroupRegNum;
    SMEM_REGISTER         * portGroupReg[4];

    GT_U32                  xgPortGroupRegNum;
    SMEM_REGISTER         * xgPortGroupReg[4];

    GT_U32                  macMibCountRegNum;
    SMEM_REGISTER         * macMibCountReg[4];

    GT_U32                  xgMacMibCountRegNum;
    SMEM_REGISTER         * xgMacMibCountReg[4];

}CHT3_GOP_CONF_MEM;


/*
 * Typedef: struct CHT3_BANKS_MEM
 *
 * Description:
 *      Describe a buffers memory banks data register.
 *
 * Fields:
 *      bankWriteRegNum    : Buffers Memory Bank0 Write Data Register number.
 *      bankWriteReg       : Registers, index is group number.
 *      bankMemRegNum      : Buffers Memory Address Space Bank number.
 *      bankMemReg         : Buffers Memory Address Space Bank.
 *
 * Comments:
 * For all registers from this struct Bits 23:28 == 13
 *
 *      bankWriteReg   -   Registers with address 0x06840000
 *                         bankWriteRegNum = 1
 *
 *      bankMemReg     -   Registers with address mask 0xFFF00000
 *                         pattern 0x06900000 and 0x06980000
 *                         bankMemRegNum = 98304
 *
 *      buffMemReg     -   Register with address 0x06800000.
 */
typedef struct {
    SMEM_REGISTER         bankWriteReg[2];

    GT_U32                  bankMemRegNum;
    SMEM_REGISTER         * bankMemReg[2];

}CHT3_BANKS_MEM;


/*
 * Typedef: struct CHT3_CENTRAL_CNT_MEM
 *
 * Description:
 *      Centralized counters.
 *
 * Fields:
 *      cncCntrsReg      : Centralized counters.

 * Comments:
 * For all registers from this struct Bits 23:28 == 16
 *
 *  cncCntrsReg:    Centralized counters address mask 0xFFFF0000
 *                  pattrern 0x08000000, cncCntrsRegNum = 0xfff / 4 + 1
 *
 *  cncCntrsPerBlockReg:
 *                  Centralized counters address mask 0xFFFFF000
 *                  pattrern 0x08001000, cncCntrsRegNum = 0xfff / 4 + 1
 *
 *  cncCntrsTbl:    Centralized counters address mask 0xFFF80000
 *                  pattrern 0x08080000, cncCntrsRegNum = 0x800 * 2
 */
typedef struct {

    GT_U32                  cncCntrsRegNum;
    SMEM_REGISTER         * cncCntrsReg;

    GT_U32                  cncCntrsPerBlockRegNum;
    SMEM_REGISTER         * cncCntrsPerBlockReg;

    GT_U32                  cncCntrsTblNum;/*number of registers in each table*/
    SMEM_REGISTER         ** cncCntrsTbl;/* array of pointer to tables */

}CHT3_CENTRAL_CNT_MEM;

/*
 * Typedef: struct CHT3_TRI_SPEED_PORTS_MEM
 *
 * Description:
 *      Describe a Tri-Speed Ports MAC, CPU Port MAC, and 1.25 Gbps SERDES
 *      Configuration Registers Map Table,  Hyper.GStack Ports MAC and
 *      XAUI PHYs Configuration Registers Map Table.
 *
 * Fields:
 *      triSpeedPortsRegNum     : Ports registers number.
 *      triSpeedPortsReg        : Ports registers.
 *
 * Comments:
 * For all registers from this struct Bits 23:28 == 21
 *
 *   triSpeedPortsReg  :  Registers with address mask 0xFFFF0000 pattern 0x0A800000
 *                          triSpeedPortsRegNum = 16383
 */
typedef struct {
    GT_U32                  triSpeedPortsRegNum;
    SMEM_REGISTER         * triSpeedPortsReg;
}CHT3_TRI_SPEED_PORTS_MEM;


/*
 * Typedef: struct CHT3_XG_PORTS_MEM
 *
 * Description:
 *      Describe XG port MAC control and status registers and port interrupt
 *      and mask registers
 *
 * Fields:
 *      xgPortsRegNum     : XG ports registers number.
 *      xgPortsReg        : XG ports registers.
 *
 * Comments:
 * For all registers from this struct Bits 23:28 == 17
 *
 *   xgPortsReg  :  Registers with address mask 0xFFFFFF00 pattern 0x08800000
 *                  xgPortsRegNum = 0xff / 4 + 1
 */
typedef struct {
    GT_U32                  xgPortsRegNum;
    SMEM_REGISTER         * xgPortsReg;
}CHT3_XG_CTRL_PORTS_MEM;


/*
 * Typedef: struct CHT3_PCL_MEM
 *
 * Description:
 *      PCL registers(ingress and egress) and TCAM memory space.
 *
 * Fields:
 *
 *   pclConfRegNum              : PCL Registers number
 *   pclConfReg                 : PCL Registers
 *
 *
 * Comments:
 *  For epclConfReg registers Bits 23:28 =15
 *  For other registers from this struct Bits 23:28 == 23
 *
 *  ingrPclCommonReg:
 *              : Registers with address mask 0xFFFFF000 pattern 0x0b800000
 *                number = 0xff / 4 + 1
 *  pclTcamInternalConfigReg
 *              : Registers with address mask 0xFFFFF000 pattern 0x0b820000
 *                number = 0xfff / 4 + 1
 *  pclTcamTestConfigReg
 *              : Registers with address mask 0xFFFFF000 pattern 0x0b830000
 *                number = 0x58 / 4 + 1
 *  ingrPclConfigTableReg:
 *              : Registers with address mask 0xFFFF0000 pattern 0x0b840000
 *                number = 0x00001080 * 0x00000002
 *  policyActionTableReg:
 *              : Registers with address mask 0xFFFF0000 pattern 0x0b880000
 *                number = 0x00003800 * 0x00000004
 *  policyEccReg:
 *              : Registers with address mask 0xFFF00000 pattern 0x0b900000
 *                number = 0x00000e00 * 0x00000004
 *  policyTcamNum:
 *              : Registers with address mask 0xFFF00000 pattern 0x0ba00000
 *                number = 0x00003800 * 0x0000000d
 *  vlanTranslationTableReg:
 *              : Registers with address mask 0xFFFFF000 pattern 0x0b804000
 *                number = 0x00001000
 *
 *  portVlanQoSConfigReg:
 *              : Registers with address mask 0xFFFFF000 pattern 0x0b810000
 *                number = 0x00000040 * 0x00000002
 *  portProtVidQoSConfigReg:
 *              : Registers with address mask 0xFFFFF000 pattern 0x0b810800
 *                number = 0x00000040 * 0x00000008
 *  epclConfReg:
 *              : Registers with address mask 0xFFFFFF00 pattern 0x07c00000
 *                number = 0xff / 4 + 1
 *  epclConfigTblReg:
 *              : Registers with address mask 0xFFF00000 pattern 0x07F00000
 *                number = 0x1040
 *  tcpPorRangeTblReg:
 *              : Registers with address mask 0xFFFFFF00 pattern 0x07c00100
 *                number = 0xff / 4 + 1
 *
 *
 */
typedef struct {
    GT_U32            ingrPclCommonRegNum;
    SMEM_REGISTER   * ingrPclCommonReg;
    GT_U32            pclTcamInternalConfigRegNum;
    SMEM_REGISTER   * pclTcamInternalConfigReg;
    GT_U32            pclTcamTestConfigRegNum;
    SMEM_REGISTER   * pclTcamTestConfigReg;
    GT_U32            ingrPclConfigTableRegNum;
    SMEM_REGISTER   * ingrPclConfigTableReg;
    GT_U32            policyActionTableRegNum;
    SMEM_REGISTER   * policyActionTableReg;
    GT_U32            policyEccRegNum;
    SMEM_REGISTER   * policyEccReg[2];
    GT_U32            policyTcamRegNum;
    SMEM_REGISTER   * policyTcamReg;
    GT_U32            vlanTranslationTableRegNum;
    SMEM_REGISTER   * vlanTranslationTableReg;
    GT_U32            portVlanQoSConfigRegNum;
    SMEM_REGISTER   * portVlanQoSConfigReg;
    GT_U32            portProtVidQoSConfigRegNum;
    SMEM_REGISTER   * portProtVidQoSConfigReg;

}CHT3_PCL_MEM;


/*
 * Typedef: struct CHT3_POLICER_MEM
 *
 * Description:
 *      Policer registers and meters memory space.
 *
 * Fields:
 *
 *  policerReg(Num) - ingress policer control and global registers(number)
 *  policerTimerReg(Num) - ingress policer timer memory(size)
 *  policerDescrSampleReg(Num) - ingress policer descriptor sample memory(size)
 *  policerTblReg(Num) - metering table(size)
 *  policerCntTblReg(Num) - billing table - counters(size)
 *  policerQosRmTblReg(Num) - ingress policer relative Qos remarking memory(size)
 *  policerMngCntTblReg(Num) - ingress policer management counters memory(size)
 *
 * Comments:
 *
 * For all registers from this struct Bits 23:28 == 24
 *
 *   policerReg         :  Registers with address mask 0xFFFFFF00 pattern 0x0C000000
 *                         portsRegNum = 0x1ff/0x4 + 1
 *
 *   policerTimerReg    :  Registers with address mask 0xFFFFFF00 pattern 0x0C000200
 *                         portsRegNum = 0x1f/0x4 + 1
 *
 *   policerDescrSampleReg
 *                      :  Registers with address mask 0xFFFFFF00 pattern 0x0C000400
 *                         portsRegNum = 0x1f/0x4 + 1
 *
 *   policerTblReg      :  Policers Table Entry<n> (0<=n<1024)
 *                         with address mask 0xFFFFE000 pattern 0x0C040000
 *                         portsRegNum = 1024 * 8
 *
 *   policerCntTblReg   :  Policers Counters Table Entry<n> (0<=n<1024)
 *                         with address mask 0xFFFFE000 pattern 0x0C042000
 *                         policerQosRmTblRegNum = 1024 * 4
 *
 *   policerQosRmTblReg :  Policers QoS Remarking and Initial DP Table Entry<0<=n<128>
 *                         with address mask 0xFFFF0000 pattern 0x0C080000
 *                         policerQosRmTblRegNum = 128
 *
 *   policerMngCntTblReg:  Management Counters Table Entry <0<=n<12>
 *                         with address mask 0xFFFF0000 pattern 0x0C0C0000
 *                         policerQosRmTblRegNum = 12*2
 */
typedef struct {
    GT_U32                  policerRegNum;
    SMEM_REGISTER         * policerReg;

    GT_U32                  policerTimerRegNum;
    SMEM_REGISTER         * policerTimerReg;

    GT_U32                  policerDescrSampleRegNum;
    SMEM_REGISTER         * policerDescrSampleReg;

    GT_U32                  policerTblRegNum;
    SMEM_REGISTER         * policerTblReg;

    GT_U32                  policerCntTblRegNum;
    SMEM_REGISTER         * policerCntTblReg;

    GT_U32                  policerQosRmTblRegNum;
    SMEM_REGISTER         * policerQosRmTblReg;

    GT_U32                  policerMngCntTblRegNum;
    SMEM_REGISTER         * policerMngCntTblReg;
}CHT3_POLICER_MEM;

/*
 * Typedef: struct CHT3_PCI_MEM
 *
 * Description:
 *      PCI Registers.
 *
 * Fields:
 *      pciRegNum      : PCI registers number.
 *      pciReg         : PCI registers
 *
 * Comments:
 *      pciRegNum = 128 * 4
 */

typedef struct {
    GT_U32                  pciRegNum;
    SMEM_REGISTER         * pciReg[4];
}CHT3_PCI_MEM;

/*
 * Typedef: struct CHT3_PHY_MEM
 *
 * Description:
 *      PHY related Registers.
 *
 * Fields:
 *   ieeeXauiRegNum         : IEEE standard XAUE registers number.
 *   ieeeXauiReg            : IEEE standard XAUE registers.
 *   extXauiRegNum          : Marvell Extention XAUE registers number.
 *   extXauiReg             : Marvell Extention standard XAUE registers.
 *
 *
 * Comments:
 *
 *   ieeeXauiReg    : IEEE Control Register 0000 affects all four lanes
 *                    portsRegNum = 0x20 * 3
 *
 *   extXauiReg     : Vendor-specific registers begin at address 8000
 *                    portsRegNum = 0x2D * 3
 */

typedef struct {
    GT_U32                   PhyIdieeeXauiRegNum;
    GT_U32                   PhyIdextXauiRegNum;

    SMEM_PHY_REGISTER     *  PhyId0ieeeXauiReg[3];
    SMEM_PHY_REGISTER     *  PhyId0extXauiReg[3];

    SMEM_PHY_REGISTER     *  PhyId1ieeeXauiReg[3];
    SMEM_PHY_REGISTER     *  PhyId1extXauiReg[3];

    SMEM_PHY_REGISTER     *  PhyId2ieeeXauiReg[3];
    SMEM_PHY_REGISTER     *  PhyId2extXauiReg[3];
}CHT3_PHY_MEM;


/*
 * Typedef: struct CHT3_MLL_MEM
 *
 * Description:
 *      MLL engine Configuration registers map table.
 *
 * Fields:
 *
 *   mllConfRegNum              : MLL global and configuration Registers number
 *   mllConfReg                 : MLL global and configuration Registers
 *   mllEntryRegNum             : MLL Entry Registers number
 *   mllEntryReg                : MLL Entry Registers
 *
 * Comments:
 * For all registers from this struct Bits 23:28 == 19
 *
 *   mllConfRegNum : Registers with address mask 0xFFFFF000 pattern 0x0C800000
 *                          pclConfRegNum = 1024
 *
 *   mllEntryRegNum : Registers with address mask 0xFFFF0000 pattern 0x0C880000
 *                          pclConfRegNum = 4096
 */
typedef struct {
    GT_U32            mllConfRegNum;
    SMEM_REGISTER   * mllConfReg;

    GT_U32            mllEntryRegNum;
    SMEM_REGISTER   * mllEntryReg;
}CHT3_MLL_MEM;


/*
 * Typedef: struct CHT3_ROUTER_MEM
 *
 * Description:
 *      Router Configuration Registers and Tables.
 *
 * Fields:
 *   configRegNum         : Global and Configuration registers number.
 *   configReg            : Global and Configuration registers.
 *   tcamRegNum           : TCAM Router registers number.
 *   tcamReg              : TCAM Router  registers.
 *   actionTblRegNum      : LTT and TT Action Table Entry number.
 *   actionTblReg         : LTT and TT Action Table Entry.
 *   nextHopTblRegNum     : Unicast/Multicast Router Next Hop Entry Number.
 *   nextHopTblReg        : Unicast/Multicast Router Next Hop Entry.
 *
 * Comments:
 * For all registers from this struct Bits 23:28 == 5
 *
 *   configReg    : Registers with address mask 0xFFFFF000 pattern 0x02800000
 *                    portsRegNum = 1152
 *
 *   ageReg       : Registers with address mask 0xFFFFF000 pattern 0x02801000
 *                    portsRegNum = 1000
 *
 *   tcamTestReg  : Registers with address mask 0xFFF00000 pattern 0x02802000
 *                    tcamTestRegNum = 23
 *
 *   tcamReg      : Registers with address mask 0xFFF00000 pattern 0x02A00000
 *                    tcamRegNum = 20477
 *
 *   actionTblReg : Registers with address mask 0xFFFF0000 pattern 0x02900000
 *                     actionTblRegNum = 4096
 *
 *   nextHopTblReg : Registers with address mask 0xFFFF0000 pattern 0x02B00000
 *                     actionTblRegNum = 16380
 *
 *   vrfTblReg :    Registers with address mask 0xFFFF0000 pattern 0x02B00000
 *                     vrfTblRegNum = 1024
 */

typedef struct {
    GT_U32                          configRegNum;
    SMEM_REGISTER         *         configReg;

    GT_U32                          ageRegNum;
    SMEM_REGISTER         *         ageReg;

    GT_U32                          tcamTestRegNum;
    SMEM_REGISTER         *         tcamTestReg;

    GT_U32                          tcamRegNum;
    SMEM_REGISTER         *         tcamReg;

    GT_U32                          actionTblRegNum;
    SMEM_REGISTER         *         actionTblReg;

    GT_U32                          nextHopTblRegNum;
    SMEM_REGISTER         *         nextHopTblReg;

    GT_U32                          vrfTblRegNum;
    SMEM_REGISTER         *         vrfTblReg;
}CHT3_ROUTER_MEM;

/*
 * Typedef: struct CHT3_VLAN_TBL_MEM
 *
 * Description:
 *      VLAN Table, Multicast Groups Table, and Span State Group Table.
 *
 * Fields:
 *      vlanCnfRegNum   : VLAN configuration registers number
 *      vlanCnfReg      : VLAN configuration registers
 *      vlanTblRegNum   : Registers number for Vlan Table
 *      vlanTblReg      : Vlan Table Registers.
 *      mcstTblRegNum   : Multicast Group configuration table number.
 *      mcstTblReg      : Multicast Group configuration table.
 *      spanTblRegNum   : STP configuration table number.
 *      spanTblReg      : STP configuration table .

 * Comments:
 * For all registers from this struct Bits 23:28 == 20
 *
 *   vlanCnfReg :   Registers with address mask 0xFF000000 pattern 0x0A0000000
 *                  vlanTblRegNum = 4
 *
 *   vlanTblReg :   VLAN<n> Entry (0<=n<4096) address mask 0xFFF00000 pattern 0x0A400000
 *                  vlanTblRegNum = 4096
 *
 *   mcstTblReg :   Multicast Group<n> Entry (0<=n<4096) address mask 0xFFFF0000
 *                  pattern 0x0A200000 , mcstTblRegNum = 4096

 *   spanTblReg :   Span State Group<n> Entry (0<=n<256) address mask 0xFFFF0000
 *                  pattern 0x0A100000 , spanTblRegNum = 256
 *
 *   vrfTblReg :    VRF Id table
 *                  0xa300000 + n*0x4: where n (0-4095) represents Entry
 */
typedef struct {
    GT_U32                  vlanCnfRegNum;
    SMEM_REGISTER         * vlanCnfReg;

    GT_U32                  vlanTblRegNum;
    SMEM_REGISTER         * vlanTblReg;

    GT_U32                  mcstTblRegNum;
    SMEM_REGISTER         * mcstTblReg;

    GT_U32                  spanTblRegNum;
    SMEM_REGISTER         * spanTblReg;

    GT_U32                  vrfTblRegNum;
    SMEM_REGISTER         * vrfTblReg;
}CHT3_VLAN_TBL_MEM;

/*
 * Typedef: struct CHT3_XG_MIB_CNTRS_MEM
 *
 * Description:
 *      XG Ports MIB counters registers memory
 *
 * Fields:
 *      mibRegNum   - XG MIB Counters registers memory
 *      mibReg      - XG MIB Counters registers number
 *
 * Comments:
*/
typedef struct{
    GT_U32                  mibRegNum;
    SMEM_REGISTER         * mibReg;
}CHT3_XG_MIB_CNTRS_MEM;
/*
 * Typedef: struct CHT3_PLUS_EXTRA_MEM
 *
 * Description:
 *      extra memory for the cheetah3+
 *
 * Fields:
 *      extraRegNum           - number of registers in extra memory
 *      extraRegPtr      - pointer to the memory
 *
 * Comments:
*/
typedef struct {
    GT_U32                  extraRegNum;
    SMEM_REGISTER         * extraRegPtr;
}CHT3_PLUS_EXTRA_MEM;

/** xCat revision A1 and above memory
 *
 *      vlanTblUnitMem : VLAN and Multicast Group and Span State Group Tables
 *      brdgMngUnitMem : Bridge management registers
 *      policerMemIngress0UnitMem : Ingress Policer memory
 *      policerMemIngress1UnitMem : Ingress Policer memory
 *      policerMemEgressUnitMem : Egress Policer memory
 *      mllMemUnitMem : MLL memory
 *      pclMemUnitMem : PCL memory
 *      pclMemEgrUnitMem : PCL Egress memory
 *      egrMemUnitMem : Egress Memory
 *      eqUnitUnitMem : EQ memory
 *      ipvxTccUnitMem : IPVX   memory
 *      haAndEpclUnitMem : HA and EPCL memory
 *      bufMngMemUnitMem : Buffer Management memory
 *      ttiMemUnitMem : TTI
 *      dfxUnitMem  :   DFX Memory
 *
******************************************************************************/
typedef struct {
    SMEM_UNIT_CHUNKS_STC            vlanTblUnitMem;
    SMEM_UNIT_CHUNKS_STC            brdgMngUnitMem;
    SMEM_UNIT_CHUNKS_STC            policerMemIngress0UnitMem;
    SMEM_UNIT_CHUNKS_STC            policerMemIngress1UnitMem;
    SMEM_UNIT_CHUNKS_STC            policerMemEgressUnitMem;
    SMEM_UNIT_CHUNKS_STC            mllMemUnitMem;
    SMEM_UNIT_CHUNKS_STC            pclMemUnitMem;
    SMEM_UNIT_CHUNKS_STC            pclTccMemUnitMem;
    SMEM_UNIT_CHUNKS_STC            pclMemEgrUnitMem;
    SMEM_UNIT_CHUNKS_STC            egrMemUnitMem;
    SMEM_UNIT_CHUNKS_STC            eqUnitUnitMem;
    SMEM_UNIT_CHUNKS_STC            ipvxTccUnitMem;
    SMEM_UNIT_CHUNKS_STC            haAndEpclUnitMem;
    SMEM_UNIT_CHUNKS_STC            bufMngMemUnitMem;
    SMEM_UNIT_CHUNKS_STC            ttiMemUnitMem;
    SMEM_UNIT_CHUNKS_STC            dfxUnitMem;
} SMEM_XCAT_DEV_EXTRA_MEM_INFO;


/*
 * Typedef: struct CHT_DEV_MEM_INFO
 *
 * Description:
 *      Describe a device's memory object in the simulation.
 *
 * Fields:
 *      memMutex        : Memory mutex for device's memory.
 *      internalSimMem  : memory for internal simulation management
 *      specFunTbl      : Address type specific R/W functions.
 *      globalMem       : Global configuration registers.
 *      egrMem          : Egress registers.
 *      brdgMngMem      : Bridge management registers.
 *      bufMngMem       : Buffers management registers.
 *      gopCnfMem       : GOP registers.
 *      macFbdMem       : MAC FDB table.
 *      banksMem        : buffers memory banks data register
 *      vlanTblMem      : VLAN tables.
 *      triSpeedPortsMem: Tri-Speed Ports MAC registers.
 *      preegressMem    : Pre-Egress registers.
 *      pclMem          : PCL engine registers and tables.
 *      policerMem      : Policer engine registers.
 *      pciMem          : Pci internal memory.
 *      phyMem          : PHY related registers.
 *      auqMem          : Address update message
 *      fuqMem          : fdb upload message
 *      ipMem           : ARP table and TT.
 *      routerMem       : Router engine registers and tables.
 *      mllMem          : MLL engine registers.
 *      policyEngIntMem : Policy engine interrupt registers
 *
 *      haAndEpclUnitMem: HA and EPCL unit
 *      eqUnitMem       : eq (pre-egress) unit
 *
 *      ipclTccUnitMem  : ingress pcl Tcc Unit Memory (xcat)
 *      ipvxTccUnitMem  : ipvx Tcc Unit Memory (xcat)
 *      uniphySerdesUnitMem : a unit that unified the XG,GE,combo ports (xcat)
 *
 *      xCatExtraMem : xCat A1 and above
 *      ch3pExtraMem    : cheetah3+ extra memory
 *
 * Comments:
 */

typedef struct {
    SMEM_CHT_DEV_COMMON_MEM_INFO    common;/* must be first */
    CHT3_GLOBAL_MEM                 globalMem;
    CHT3_EGR_MEM                    egrMem;
    CHT3_BRDG_MNG_MEM               brdgMngMem;
    CHT3_BUF_MNG_MEM                bufMngMem;
    CHT3_GOP_CONF_MEM               gopCnfMem;
    CHT_MAC_FDB_MEM                 macFdbMem;
    CHT3_BANKS_MEM                  banksMem;
    CHT3_CENTRAL_CNT_MEM            centralCntMem;
    CHT3_TRI_SPEED_PORTS_MEM        triSpeedPortsMem;
    CHT3_XG_CTRL_PORTS_MEM          xgCtrlPortsMem;
    CHT3_PCL_MEM                    pclMem;
    CHT3_POLICER_MEM                policerMem;
    CHT3_PCI_MEM                    pciMem;
    CHT3_ROUTER_MEM                 routerMem;
    CHT3_MLL_MEM                    mllMem;
    CHT3_VLAN_TBL_MEM               vlanTblMem;
    CHT3_XG_MIB_CNTRS_MEM           xgMibCntrsMem;
    /* Cheetah3+ memory */
    CHT3_PLUS_EXTRA_MEM             ch3pExtraMem;

    SMEM_UNIT_CHUNKS_STC            haAndEpclUnitMem;
    SMEM_UNIT_CHUNKS_STC            eqUnitMem;

    /* xCat unique memory */
    SMEM_UNIT_CHUNKS_STC            ipclTccUnitMem;
    SMEM_UNIT_CHUNKS_STC            ipvxTccUnitMem;
    SMEM_UNIT_CHUNKS_STC            uniphySerdesUnitMem;

    /* xCat A1 and above */
    SMEM_XCAT_DEV_EXTRA_MEM_INFO    xCatExtraMem;
}SMEM_CHT3_DEV_MEM_INFO;

/*******************************************************************************
*   smemCHT3Init
*
* DESCRIPTION:
*       Init memory module for a Cht device.
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
void smemCht3Init
(
    IN SKERNEL_DEVICE_OBJECT * deviceObj
);

/*******************************************************************************
*   smemCht3Init2
*
* DESCRIPTION:
*       Init memory module for a device - after the load of the default
*           registers file
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
void smemCht3Init2
(
    IN SKERNEL_DEVICE_OBJECT * deviceObj
);

/*******************************************************************************
*  smemCht3TableInfoSet
*
* DESCRIPTION:
*       set the table info for the device --> fill devObjPtr->tablesInfo
*
* INPUTS:
*       devObjPtr   - device object PTR.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
void smemCht3TableInfoSet
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr
);

/*******************************************************************************
*  smemXcatA1TableInfoSet
*
* DESCRIPTION:
*       set the table info for the device --> fill devObjPtr->tablesInfo
*
* INPUTS:
*       devObjPtr   - device object PTR.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
void smemXcatA1TableInfoSet
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr
);

/*******************************************************************************
*   smemXCatA1UnitPex
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  PEX/MBus unit
*
* INPUTS:
*       devObjPtr           - pointer to device memory object.
*       unitPtr             - pointer to the unit chunk
*       pexBaseAddr         - PCI/PEX/MNus unit base address
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
void smemXCatA1UnitPex
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr,
    IN GT_U32 pexBaseAddr
);

/* MAC Counters address mask */
#define     SMEM_CHT3_COUNT_MSK_CNS              0xff8ff000


/* policer management counters mask */
/* 0x40 between sets , 0x10 between counters */
/* 0x00 , 0x10 , 0x20 , 0x30 */
/* 0x40 , 0x50 , 0x60 , 0x70 */
/* 0x80 , 0x90 , 0xa0 , 0xb0 */
#define POLICER_MANAGEMENT_COUNTER_MASK_CNS         0xFFFFFF0F
/* policer management counters base address */
#define POLICER_MANAGEMENT_COUNTER_ADDR_CNS         0x00000500


/* CH3 - policer management counters mask */
/* 0x20 between sets , 0x8 between counters */
/* 0x00 , 0x08 , 0x10 , 0x18 */
/* 0x20 , 0x28 , 0x30 , 0x38 */
/* 0x40 , 0x48 , 0x50 , 0x58 */
#define CH3_POLICER_MANAGEMENT_COUNTER_MASK_CNS         0xFFFFFF87



ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemCht3ActiveWritePclAction         );
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemCht3ActiveWritePolicerTbl        );
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemCht3ActiveWriteCncFastDumpTrigger);
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemCht3ActiveWriteRouterAction         );
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemCht3ActiveWriteFDBGlobalCfgReg      );
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemXcatA1ActiveWriteVlanTbl            );
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemXcatActiveWriteMacModeSelect        );
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemXCatActiveWritePolicerTbl           );
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemXCatActiveWriteIPFixTimeStamp       );
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemXCatActiveWriteLogTargetMap         );
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemCht3ActiveWriteCncInterruptsMaskReg );
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemXCatActiveWritePolicyTcamConfig_0   );
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemXCatActiveWritePolicerMemoryControl );
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemXCatActiveWriteIplr0Tables                    );
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemXCatActiveWriteIplr1Tables                    );

ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemXCatActiveWriteBridgeGlobalConfig2Reg);

ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemXCatActiveWriteEgressFilterVlanMap);
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemXCatActiveWriteEgressFilterVlanMember);

ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemXCatActiveWriteEqGlobalConfigReg);
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemXCatActiveWriteIpclGlobalConfigReg);
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemXCatActiveWriteTtiInternalMetalFix);
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemXCatActiveWriteTtiGlobalConfigReg);
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemXCatActiveWriteHaGlobalConfigReg);
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemXCatActiveWriteMllGlobalConfigReg);

/*l2 mll*/
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemXCatActiveWriteL2MllVidxEnable);
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemXCatActiveWriteL2MllPointerMap);

/*read active memory*/
ACTIVE_READ_FUNC_PROTOTYPE_MAC(smemCht3ActiveCncBlockRead            );
ACTIVE_READ_FUNC_PROTOTYPE_MAC(smemCht3ActiveCncWrapAroundStatusRead );
ACTIVE_READ_FUNC_PROTOTYPE_MAC(smemXCatActiveReadIPFixNanoTimeStamp  );
ACTIVE_READ_FUNC_PROTOTYPE_MAC(smemXCatActiveReadIPFixSecLsbTimeStamp);
ACTIVE_READ_FUNC_PROTOTYPE_MAC(smemXCatActiveReadIPFixSecMsbTimeStamp);
ACTIVE_READ_FUNC_PROTOTYPE_MAC(smemXCatActiveReadIPFixSampleLog);
ACTIVE_READ_FUNC_PROTOTYPE_MAC(smemXCatActiveReadPolicerManagementCounters);
ACTIVE_READ_FUNC_PROTOTYPE_MAC(smemXCatActiveReadIeeeMcConfReg);
ACTIVE_READ_FUNC_PROTOTYPE_MAC(smemXCatActiveReadIplr0Tables);
ACTIVE_READ_FUNC_PROTOTYPE_MAC(smemXCatActiveReadIplr1Tables);

/* PCL/Router TCAM BIST done simulation */
ACTIVE_READ_FUNC_PROTOTYPE_MAC(smemCht3ActiveReadTcamBistConfigAction);

extern SKERNEL_DEVICE_ROUTE_TCAM_INFO_STC  xcatRoutTcamInfo;

/*******************************************************************************
*  cncUnitIndexFromAddrGet
*
* DESCRIPTION:
*
*   get the CNC unit index from the address. (if address is not in CNC unit --> fatal error)
*   NOTE: sip5 support 2 CNC units (legacy devices supports only 1)
*
* INPUTS:
*    devObjPtr  - device object PTR.
*    address    - Address in one of the CNC units.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
GT_U32  smemCht3CncUnitIndexFromAddrGet
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address
);


void smemLion2RegsInfoSet_GOP_SERDES
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SMEM_CHT_PP_REGS_ADDR_STC        * regAddrDbPtr,
    IN GT_U32                   s/*serdes*/,
    IN GT_U32                   extraOffset,
    IN GT_BIT                   isGopVer1
);

void smemLion2RegsInfoSet_GOP_gigPort
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SMEM_CHT_PP_REGS_ADDR_STC        * regAddrDbPtr,
    IN GT_U32                   p/*port*/,
    IN GT_U32                   minus_p/*compensation port*/,
    IN GT_U32                   extraOffset,
    IN GT_BIT                   isGopVer1
);

void smemLion2RegsInfoSet_GOP_XLGIP
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SMEM_CHT_PP_REGS_ADDR_STC        * regAddrDbPtr,
    IN GT_U32                   p/*port*/,
    IN GT_U32                   minus_p/*compensation port*/,
    IN GT_U32                   extraOffset
);

void smemLion2RegsInfoSet_GOP_MPCSIP
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SMEM_CHT_PP_REGS_ADDR_STC        * regAddrDbPtr,
    IN GT_U32                   p/*port*/,
    IN GT_U32                   minus_p/*compensation port*/,
    IN GT_U32                   extraOffset
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __smemCht3h */

