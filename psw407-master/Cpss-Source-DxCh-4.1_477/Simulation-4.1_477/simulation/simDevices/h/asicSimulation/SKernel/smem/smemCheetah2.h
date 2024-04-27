/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* smemCheetah2.h
*
* DESCRIPTION:
*       Data definitions for cheetah2 memories.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 12 $
*
*******************************************************************************/
#ifndef __smemCht2h
#define __smemCht2h

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <asicSimulation/SKernel/smem/smemCheetah.h>


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
 * Typedef: struct CHT2_GLOBAL_MEM
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
 *
 * Comments:
 *  For all registers from this struct Bits 23:28 == 0
 *
 *  globReg    - Registers with address mask 0x1FFFFF00 pattern 0x00000000
 *                  globRegNum = 76
 *
 *  sdmaReg - Registers with address mask 0x1FFFF000 pattern 0x00002000
 *                  sdmaRegNum = 150
 *
 *  twsiIntRegs - Registers with address mask 0x1FFFFF00 pattern 0x00400000
 *                  twsiIntRegs = 0x1C
 */

typedef struct {
    GT_U32                  globRegNum;
    SMEM_REGISTER         * globReg;

    GT_U32                  sdmaRegNum;
    SMEM_REGISTER         * sdmaReg;

    GT_U32                  twsiIntRegsNum;
    SMEM_REGISTER         * twsiIntRegs;
}CHT2_GLOBAL_MEM;

/*
 * Typedef: struct CHT2_EGR_MEM
 *
 * Description:
 *      Egress, Transmit Queue and VLAN Configuration Register Map Table,
 *      Port TxQ Configuration Register, Hyper.GStack Ports MIB Counters.
 *
 * Fields:
 *      egrGenRegNum    : Common registers number.
 *      egrGenReg       : Common registers number, index is port and group.
 *
 *      txqInternRegNum : TxQ internal registers number.
 *      txqInternReg    : TxQ internal registers.
 *
 *      trQueueRegNum   : Transmit queue registers number.
 *      trQueueReg      : Transmit queue registers.
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
 *      tailDropRegNum  : Tail Drop Profile Configuration Registers.
 *      tailDropReg     : Tail Drop Profile Configuration.
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
 *  txqInternReg - Registers with address mask 0x1FF00000 pattern 0x01840000,
 *                 0x01880000, 0x018C0000, 0x01900000
 *                 txQInternRegNum = 504(8 TC * 64 ports)
 *
 *  trQueueReg   - Registers with address mask 0x1FFFF000 pattern 0x01A40080
 *                 trQueueRegNum = 2032
 *
 *  portWdReg    - Registers with address mask 0x1FFF0000 pattern 0x01A80000
 *                 portWdRegNum = 64
 *
 *  egrMibCntReg - Registers with address mask 0x1FFFFF00 pattern 0x01B40100
 *                 egrMibCntReg = 120
 *
 *  hprMibCntReg - Registers with address mask 0x1FF00000 pattern 0x01C00000
 *                 hprMibCntRegNum = 64
 *
 *  tailDropReg  - Registers with address mask 0x1FFFF000 pattern 0x01940000
 *                 tailDropRegNum = 3132
 *
 *  egrRateShpReg- Registers with address mask 0x1FF00000 pattern 0x01A00000
 *                 egrRateShpRegNum = 512(8 TC * 64 ports)
 *
 *  xsmiReg      - Registers with address mask 0x1FFF0000 pattern 0x01CC0000
 *                 xsmiRegNum = 2
 *
 *  egrStcTblReg - Registers with address mask 0x1FFF0000 pattern 0x01D40000
 *                 egrStcTblRegNum = 0xDC0
 *
 *  txSniffCntr =  Register of tx sniffing
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

    GT_U32                  egrRateShapeRegNum;
    SMEM_REGISTER         * egrRateShapeReg;

}CHT2_EGR_MEM;

/*
 * Typedef: struct CHT2_BRDG_MNG_MEM
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
}CHT2_BRDG_MNG_MEM;

/*
 * Typedef: struct CHT2_BUF_MNG_MEM
 * Description:
 *      Describe a device's buffer management registers memory object.
 *
 * Fields:
 *
 *      bmRegNum            : General buffer management registers number.
 *      bmReg               : General buffer management registers.
 *      bmLLRegNum          : Buffer management linked list registers number.
 *      bmLLReg             : Buffer management linked list registers.
 *      bmCntrRegNum        : Buffer management controlled registers number.
 *      bmCntrReg           : Buffer management controlled registers.
 *
 * Comments:
 * For all registers from this struct Bits 23:28 == 6
 *
 *  bmReg          - Registers with address mask 0xfffff000 pattern 0x03000000
 *                    bmRegNum = 196
 *
 *  bmLLReg        - Registers with address mask 0xffff0000 pattern 0x03070000
 *                      bmLLRegNum = 4096
 *
 *  bmCntrReg      - Registers with address mask 0xffff0000 pattern 0x03010000
 *                      bmLLRegNum = 4096
 */

typedef struct {
    GT_U32                  bmRegNum;
    SMEM_REGISTER         * bmReg;

    GT_U32                  bmLLRegNum;
    SMEM_REGISTER         * bmLLReg;

    GT_U32                  bmCntrRegNum;
    SMEM_REGISTER         * bmCntrReg;
}CHT2_BUF_MNG_MEM;

/*
 * Typedef: struct CHT2_GOP_CONF_MEM
 * Description:
 *      LEDs, Tri-Speed Ports MIB Counters and Master SMI
 *      Configuration Registers.
 *
 * Fields:
 *      gopRegNum          : Group configuration registers numbers.
 *      gopReg             : Group configuration registers.
 *      macMibCountRegNum  : MAC MIB Counters registers numbers.
 *      macMibCountReg     : MAC MIB Counters registers.
 *      ledRegNum          : LED Interfaces Configuration Registers numbers.
 *      ledReg             : LED Interfaces Configuration Registers.
 *
 * Comments:
 *      For all registers from this struct Bits 23:28 == 8,9,10,11
 *
 *      gopReg    -  Registers with address mask 0xFFFFF000 pattern 0x04004000
 *                   gopRegNum = 512
 *
 *      macMibCountReg[]
 *                 - Registers with address mask 0xFFFFF000 pattern 0x04010000
 *                   macMibCountRegNum = 892
 *
 *      ledReg[]   - Registers with address mask 0xFFFFFF00 pattern 0x04005100
 *                   gopRegNum = 16
 *
 */

typedef struct {
    GT_U32                  gopRegNum;
    SMEM_REGISTER         * gopReg[4];

    GT_U32                  macMibCountRegNum;
    SMEM_REGISTER         * macMibCountReg[4];

    GT_U32                  ledRegNum;
    SMEM_REGISTER         * ledReg[4];
}CHT2_GOP_CONF_MEM;

/*
 * Typedef: struct CHT2_MAC_FDB_MEM
 *
 * Description:
 *      Describe a device's Bridge registers and FDB.
 *
 * Fields:
 *      fdbRegNum          : FDB Global Configuration Registers number.
 *      fdbReg             : FDB Global Configuration Registers.
 *      macUpdFifoRegsNum  : Mac update fifo registers number.
 *      macUpdFifoRegs     : Mac update fifo registers.
 *      macTblRegNum       : Number of MAC Table registers.
 *      macTblReg          : MAC Table Registers.
 *
 * Comments:
 * For all registers from this struct Bits 23:28 == 12
 *
 *      fdbReg -        Registers with address mask 0xFF00000 pattern 0x06000000
 *                      fdbRegNum = 124
 *
 *      macTblReg -     This table may be accessed by the CPU using the MAC
 *                       Table Access Control registers
 *                      fdbRegNum = 4096
 *
 *      macUpdFifoRegs -    MAC update FIFO registers. macUpdFifoRegsNum = 4 * 16
 *                          (16 MAC Update entires, 4 words in the entry)
 *
 */
typedef struct {
    GT_U32                  fdbRegNum;
    SMEM_REGISTER         * fdbReg;

    GT_U32                  macTblRegNum;
    SMEM_REGISTER         * macTblReg;

    GT_U32                  macUpdFifoRegsNum;
    SMEM_REGISTER         * macUpdFifoRegs;
}CHT2_MAC_FDB_MEM;


/*
 * Typedef: struct CHT2_BANKS_MEM
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
    GT_U32                  bankWriteRegNum;
    SMEM_REGISTER         * bankWriteReg;

    GT_U32                  bankMemRegNum;
    SMEM_REGISTER         * bankMemReg[2];

    GT_U32                  buffMemReg;
}CHT2_BANKS_MEM;

/*
 * Typedef: struct CHT2_BUF_MEM
 *
 * Description:
 *      Describe a Buffers Memory, Ingress MAC Errors Indications and
 *      Egress Header Alteration Register Map Table ,
 *      Router Configuration Registers.
 *
 * Fields:
 *      bufMemRegNum       : Buffers Memory Registers number.
 *      bufMemReg          : Buffers Memory Registers
 *
 * Comments:
 * For all registers from this struct Bits 23:28 == 15
 *
 *      bufMemReg : Registers with address mask 0xFFF00000 pattern 0x078000000
 *                   bufMemRegNum = 204
 */
typedef struct {
    GT_U32                  bufMemRegNum;
    SMEM_REGISTER         * bufMemReg;
}CHT2_BUF_MEM;

/*
 * Typedef: struct CHT2_VLAN_TBL_MEM
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
}CHT2_VLAN_TBL_MEM;

/*
 * Typedef: struct CHT2_TRI_SPEED_PORTS_MEM
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
}CHT2_TRI_SPEED_PORTS_MEM;


/*
 * Typedef: struct CHT2_PREEGR_MEM
 *
 * Description:
 *      Pre-Egress Engine Registers Map Table, Trunk Table,
 *      QoSProfile to QoS Table, CPU Code Table, Number of Trunk Members Table
 *      Data Access Statistical Rate Limits Table Data Access,
 *      Ingress STC Table Registers , Ingress Forwarding Decision
 *
 * Fields:
 *      portsRegNum                 : Pre-Egress Engine Registers number.
 *      portsReg                    : Pre-Egress Engine Registers.
 *      genRegNum                   : Ports registers number.
 *      genReg                      : Ports registers.
 *      trunkTblRegNum              : Trunk table memory size.
 *      trunkTblReg                 : Trunk table memory.
 *      qosTblRegNum                : QoSProfile to QoS table size.
 *      qosTblReg                   : QoSProfile to QoS table.
 *      cpuCodeTblRegNum            : CPU Code Table size.
 *      cpuCodeTblReg               : CPU Code Table.
 *      statRateTblReg              : Statistical Rate Limits Table Entry.
 *      statRateTblRegNum           : Statistical Rate Limits Table Entry Num.
 *      portInStcTblRegNum          : Port Ingress STC Table size.
 *      portInStcTblReg             : Port<n> Ingress STC Table.
 *      toCpuRateLimiterReg         : To CPU Rate Limiter<n> packet counter.
 *      toCpuRateLimiterRegNum      : To CPU Rate Limiter<n> packet counter size.
 *      trunkMemberTblRegNum        : Trunk members Table  size.
 *      trunkMemberTblReg           : Trunk members Table  .
 *      ingFwrdRestConfigReg        : Ingress Forwarding Restriction.
 *      ingFwrdRestConfigRegNum     : Ingress Forwarding Restriction size.
 *      portIngressStcTblReg        : Port<n> Ingress STC Table Entry.
 *      portIngressStcTblRegNum     : Port<n> Ingress STC Table Entry size.
 *      statRateLimitReg            : Statistical Rate Limits Table Entry<n>
 *      statRateLimitRegNum         : Statistical Rate Limits Table Entry<n> size.
 *
 * Comments:
 *
 * For all registers from this struct Bits 23:28 == 22
 *
 *   portsReg             : Registers with address mask 0xFFFFFF00 pattern 0x0B000000
 *                          portsRegNum = 22
 *
 *   genReg               : Registers with address mask 0xFFFFF000 pattern 0x0B000000
 *                          portsRegNum = 22
 *
 *   trunkTblReg          : Trunk table Trunk<n> Member<i> Entry (1<=n<128, 0<=i<8)
 *                          address mask 0xFFFFF000 pattern 0x0B000000
 *                          trunkTblRegNum = 128
 *
 *   qosTblReg            : QoSProfile to QoS table Entry<n> (0<=n<72)
 *                          address mask 0xFFFFF000 pattern 0x0B300000
 *                          qosTblRegNum = 72
 *
 *   cpuCodeTblReg        : CPU Code Table Entry<n> (0<=n<256)
 *                          address mask 0xFFFFF000 pattern 0x0B200000
 *                          cpuCodeTblReg = 256
 *
 *   statRateTblReg       : Statistical Rate Limits Table Entry<n> (0<=n<32)
 *                          address mask 0xFFFFFF00 pattern 0x0B100000
 *                          statRateTblReg = 0x24
 *
 *   portInStcTblReg      : Registers with address mask 0xFFFFF000
 *                          pattern 0x0b040000 . portInStcTblRegNum = 0x1FF
 *
 *   toCpuRateLimiterReg  : To CPU Rate Limiter<n> packet counter.
 *                          Registers with address mask 0xFFFFFF00
 *                          pattern 0x0b080000.
 *
 *   trunkMemberTblReg    : No. of Trunk Members Table Entry register.
 *                          address mask 0xffffff00 pattern : 0x0B009000 ,
 *                          trunkMemberTblRegNum = 0x40
 *
 *   ingFwrdRestConfigReg : Ingress Forwarding Restriction.
 *                          address mask 0xFFFFFFF0 pattern 0x0B020000
 *                          ingFwrdRestConfigRegNum =  4
 *
 *   portIngressStcTblReg   : Registers with address mask 0xFFFF0000 pattern
 *                            0x0B040000 . portIngressStcTblRegNum = 0x1FF
 *   statRateLimitReg       : Registers with address mask 0xFFFFFF00 pattern
 *                            0x0B100000 . statRateLimitRegNum  = 0x7C
 */
typedef struct {
    GT_U32                  portsRegNum;
    SMEM_REGISTER         * portsReg;

    GT_U32                  genRegNum;
    SMEM_REGISTER         * genReg;

    GT_U32                  trunkTblRegNum;
    SMEM_REGISTER         * trunkTblReg;

    GT_U32                  qosTblRegNum;
    SMEM_REGISTER         * qosTblReg;

    GT_U32                  cpuCodeTblRegNum;
    SMEM_REGISTER         * cpuCodeTblReg;

    GT_U32                  statRateTblRegNum;
    SMEM_REGISTER         * statRateTblReg;

    GT_U32                  portInStcTblRegNum;
    SMEM_REGISTER         * portInStcTblReg;

    GT_U32                  toCpuRateLimiterRegNum;
    SMEM_REGISTER         * toCpuRateLimiterReg;

    GT_U32                  trunkMemberTblRegNum;
    SMEM_REGISTER         * trunkMemberTblReg;

    GT_U32                  ingFwrdRestConfigRegNum;
    SMEM_REGISTER         * ingFwrdRestConfigReg;

    GT_U32                  portIngressStcTblRegNum;
    SMEM_REGISTER         * portIngressStcTblReg;

    GT_U32                  statRateLimitRegNum;
    SMEM_REGISTER         * statRateLimitReg;

}CHT2_PREEGR_MEM;

/*
 * Typedef: struct CHT2_PCL_MEM
 *
 * Description:
 *      PCL registers(ingress and egress) and TCAM memory space.
 *
 * Fields:
 *
 *   pclConfRegNum              : PCL Registers number
 *   pclConfReg                 : PCL Registers
 *   accessDataRegNum           :
 *   accessDataReg              :
 *   epclConfRegNum             : Egress PCL Registers number
 *   epclConfReg                : Egress PCL Registers
 *   pceActionTblRegNum         : Policy Action table registers number
 *   pceActionTblReg            : Policy Action table registers
 *   tcamRegNum                 : TCAM memory size
 *   tcamReg                    : TCAM memory pointer
 *   pclIdTblRegNum             : PCL-ID configuration table register number.
 *   pclIdTblReg                : PCL-ID configuration table register.
 *   pceRuleMatchCntRegNum      : PCL Rule match counters size.
 *   pceRuleMatchCntReg         : PCL Rule match counters.
 *   epclConfigTblRegNum        : EPCL configuration table register number.
 *   epclConfigTblReg           : EPCL configuration table register.
 *   pclIntRegNum               : PCL internal registers number.
 *   pclIntReg                  : PCL internal registers pointer.
 *
 * Comments:
 * For epclConfReg registers Bits 23:28 =15
 * For other registers from this struct Bits 23:28 == 23
 *
 *   pclConfReg             : Registers with address mask 0xFFFF0000 pattern 0x0b810000
 *                                      pclConfRegNum = 4300
 *
 *   accessData             : Registers with address mask 0xFFFF0000 pattern 0x0b800000
 *                                      accessDataRegNum = 814
 *
 *   epclConfReg            : Registers with address mask 0xFFFFF000 pattern 0x07c00000
 *                                      epclConfRegNum = 81
 *
 *   pceActionTblReg        : Registers with address mask 0xFFFF0000 pattern 0x0b8c0000
 *                                      pclActionTblRegNum = 4096
 *
 *   tcamReg                : Registers with address mask 0xFFFF0000 pattern 0x0B880000
 *                                      tcamRegNum =  0x7FFF
 *
 *   pclIdTblReg            : Registers with address mask 0xFFFF0000 pattern 0x0b400000
 *                                      pclIdTblRegNum = 4224
 *
 *   epclConfigTblReg       : Registers with address mask 0xFFFF0000 pattern 0x07700000
 *                                      epclConfigTblRegNum = 4160
 *
 *   pclIntReg              : Registers with address mask 0xFFFF0000 pattern 0x0B820000
 *                                      portsRegNum = 69
 *
 *   tcpPorRangeTblReg      : Registers with address mask 0xFFFFFF00 pattern 0x0B007000
 *                                      tcpPorRangeTblRegNum = 63
 */
typedef struct {
    GT_U32            pclConfRegNum;
    SMEM_REGISTER   * pclConfReg;

    GT_U32            epclConfRegNum;
    SMEM_REGISTER   * epclConfReg;

    GT_U32            ingPclIPCompsRegNum;
    SMEM_REGISTER   * ingPclIPCompsReg;

    GT_U32            ePclIPCompsRegNum;
    SMEM_REGISTER   * ePclIPCompsReg;

    GT_U32            pceActionTblRegNum;
    SMEM_REGISTER   * pceActionTblReg;

    GT_U32            tcamRegNum;
    SMEM_REGISTER   * tcamReg;

    GT_U32            accessDataRegNum;
    SMEM_REGISTER   * accessDataReg;

    GT_U32            pclIdTblRegNum;
    SMEM_REGISTER   * pclIdTblReg;

    GT_U32            pceRuleMatchCntRegNum;
    SMEM_REGISTER   * pceRuleMatchCntReg;

    GT_U32            epclConfigTblRegNum;
    SMEM_REGISTER   * epclConfigTblReg;

    GT_U32            pclDescriptorRegNum;
    SMEM_REGISTER   * pclDescriptorReg;

    GT_U32            protBasedVlanQosTblRegNum;
    SMEM_REGISTER   * protBasedVlanQosTblReg[8];

    GT_U32            pclIntRegNum;
    SMEM_REGISTER   * pclIntReg;

    GT_U32            tcpPorRangeTblRegNum;
    SMEM_REGISTER   * tcpPorRangeTblReg;
}CHT2_PCL_MEM;


/*
 * Typedef: struct CHT2_POLICER_MEM
 *
 * Description:
 *      Policer registers and meters memory space.
 *
 * Fields:
 *      policerRegNum           : Policer Registers number.
 *      policerReg              : Policer Registers pointer.
 *      policerTblRegNum        : Policers Table size.
 *      policerTblReg           : Policers Table.
 *      policerQosRmTblRegNum   : Policers QoS Remark. and Initial DP Table size.
 *      policerQosRmTblReg      : Policers QoS Remark. and Initial DP Table.
 *      policerCntTblRegNum     : Policers Counters Table size.
 *      policerCntTblReg        : Policers Counters Table.
 *
 * Comments:
 *
 * For all registers from this struct Bits 23:28 == 24
 *
 *   policerReg         :  Registers with address mask 0xFFFFFF00 pattern 0x0C000000
 *                          portsRegNum = 27
 *
 *   policerTblReg      :  Policers Table Entry<n> (0<=n<256)
 *                          portsRegNum = 256
 *
 *   policerQosRmTblReg : Policers QoS Remarking and Initial DP Table Entry
 *                          policerQosRmTblRegNum = 72
 *
 *   policerCntTblReg   : Policers Counters Table Entry<n> (0<=n<16)
 *                          policerQosRmTblRegNum = 16
 */
typedef struct {
    GT_U32                  policerRegNum;
    SMEM_REGISTER         * policerReg;

    GT_U32                  policerTblRegNum;
    SMEM_REGISTER         * policerTblReg;

    GT_U32                  policerQosRmTblRegNum;
    SMEM_REGISTER         * policerQosRmTblReg;

    GT_U32                  policerCntTblRegNum;
    SMEM_REGISTER         * policerCntTblReg;
}CHT2_POLICER_MEM;

/*
 * Typedef: struct CHT2_PCI_MEM
 *
 * Description:
 *      PCI Registers.
 *
 * Fields:
 *      pciRegNum      : PCI registers number.
 *      pciReg         : PCI registers
 *
 * Comments:
 *      pciRegNum = 280
 */

typedef struct {
    GT_U32                  pciRegNum;
    SMEM_REGISTER         * pciReg;
}CHT2_PCI_MEM;

/*
 * Typedef: struct CHT2_PHY_MEM
 *
 * Description:
 *      PHY related Registers.
 *
 * Fields:
 *   ieeeXauiRegNum         : IEEE standard XAUE registers number.
 *   ieeeXauiReg            : IEEE standard XAUE registers.
 *   extXauiRegNum          : Marvell Extension XAUE registers number.
 *   extXauiReg             : Marvell Extension standard XAUE registers.
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
}CHT2_PHY_MEM;


/*
 * Typedef: struct CHT2_MLL_MEM
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
}CHT2_MLL_MEM;


/*
 * Typedef: struct CHT2_ROUTER_MEM
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
 *   configReg    : Registers with address mask 0xFFFF0000 pattern 0x02800000
 *                    portsRegNum = 1152
 *
 *   tcamReg      : Registers with address mask 0xFFF00000 pattern 0x02A00000
 *                    tcamRegNum = 20477
 *
 *   actionTblReg : Registers with address mask 0xFFFF0000 pattern 0x02900000
 *                     actionTblRegNum = 4096
 *
 *   nextHopTblReg : Registers with address mask 0xFFFF0000 pattern 0x02B00000
 *                     actionTblRegNum = 16380
 */

typedef struct {
    GT_U32                          configRegNum;
    SMEM_REGISTER         *         configReg;

    GT_U32                          tcamRegNum;
    SMEM_REGISTER         *         tcamReg;

    GT_U32                          actionTblRegNum;
    SMEM_REGISTER         *         actionTblReg;

    GT_U32                          nextHopTblRegNum;
    SMEM_REGISTER         *         nextHopTblReg;
}CHT2_ROUTER_MEM;

struct CHT_AUQ_MEM;

/*
 * Typedef: struct CHT2_IP_MEM
 *
 * Description:
 *      ARP MAC Destination Address and VLAN/Port MAC SA Tables
 *
 * Fields:
 *      arpTblReg   - ARP table memory pointer
 *      macSaTblReg - MAC SRC address table memory pointer
 *                ipProtCpuuCodeReg - IP protocol CPU Code Entry
 * Comments:
*/
typedef struct {
    GT_U32                  arpTblRegNum;
    SMEM_REGISTER         * arpTblReg;

    GT_U32                  macSaTblRegNum;
    SMEM_REGISTER         * macSaTblReg;

    GT_U32                  ipProtCpuCodeRegNum;
    SMEM_REGISTER         * ipProtCpuCodeReg;
}CHT2_IP_MEM;


/*
 * Typedef: struct CHT_DEV_MEM_INFO
 *
 * Description:
 *      Describe a device's memory object in the simulation.
 *
 * Fields:
 *      memMutex        : Memory mutex for device's memory.
 *      specFunTbl      : Address type specific R/W functions.
 *      globalMem       : Global configuration registers.
 *      egrMem          : Egress registers.
 *      brdgMngMem      : Bridge management registers.
 *      bufMngMem       : Buffers management registers.
 *      gopCnfMem       : GOP registers.
 *      macFbdMem       : MAC FDB table.
 *      banksMem        : buffers memory banks data register
 *      bufMem          : Buffers access registers.
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
 *
 * Comments:
 */

typedef struct {
    SMEM_CHT_DEV_COMMON_MEM_INFO    common;/* must be first */
    CHT2_GLOBAL_MEM                 globalMem;
    CHT2_EGR_MEM                    egrMem;
    CHT2_BRDG_MNG_MEM               brdgMngMem;
    CHT2_BUF_MNG_MEM                bufMngMem;
    CHT2_GOP_CONF_MEM               gopCnfMem;
    CHT_MAC_FDB_MEM                 macFdbMem;
    CHT2_BANKS_MEM                  banksMem;
    CHT2_BUF_MEM                    bufMem;
    CHT2_VLAN_TBL_MEM               vlanTblMem;
    CHT2_TRI_SPEED_PORTS_MEM        triSpeedPortsMem;
    CHT2_PREEGR_MEM                 preegressMem;
    CHT2_PCL_MEM                    pclMem;
    CHT2_POLICER_MEM                policerMem;
    CHT2_PCI_MEM                    pciMem;
    CHT2_IP_MEM                     ipMem;
    CHT2_ROUTER_MEM                 routerMem;
    CHT2_MLL_MEM                    mllMem;
}SMEM_CHT2_DEV_MEM_INFO;

/*******************************************************************************
*   smemCht2Init
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
void smemCht2Init
(
    IN SKERNEL_DEVICE_OBJECT * deviceObj
);

/*******************************************************************************
*  smemCht2TableInfoSet
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
void smemCht2TableInfoSet
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr
);

/* MAC entry width in words */
#define SMEM_CHT2_MAC_TABLE_ENTRY_WORDS        (4)

/* MAC table entry in bytes */
#define SMEM_CHT2_MAC_ENTRY_BYTES \
    (SMEM_CHT2_MAC_TABLE_ENTRY_WORDS * sizeof(GT_U32))

/* active functions for Write */
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemCht2ActiveFuqBaseWrite);
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemCht2ActiveWriteVlanTbl);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __smemCht2h */


