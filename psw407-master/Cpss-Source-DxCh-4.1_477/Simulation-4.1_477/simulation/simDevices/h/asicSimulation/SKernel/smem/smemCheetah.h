/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* smemCheetah.h
*
* DESCRIPTION:
*       Data definitions for Cht memories.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 145 $
*
*******************************************************************************/
#ifndef __smemChth
#define __smemChth

#include <asicSimulation/SKernel/smem/smem.h>
#include <asicSimulation/SKernel/suserframes/snetCheetahIngress.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* indication that for the register did FULL align to sip5 register(s) */
#define DONE_SIP5_ALIGN

/* indication that for the register don't know how to support in sip 5 */
#define NOT_SUPPORTED_SIP5_ALIGN

#define NEED_TO_BE_USED_BY_SIP5

#define SHOULD_BE_IGNORED_BY_SIP5

#define NOT_RELEVANT_TO_SIM
/*the register hold macro */
#define REG_MACRO_EXIST

/* Register special function index in function array (Bits 23:28)*/
#define     REG_SPEC_FUNC_INDEX                 0x1F800000

#define SMEM_CHT_FIELD_GET_MAC(dev,field)   \
            (((SMEM_CHT_DEV_COMMON_MEM_INFO*)(dev)->deviceMemory)->field)

#define SMEM_CHT_FIELD_PTR_GET_MAC(dev,field)   \
            (&SMEM_CHT_FIELD_GET_MAC(dev,field))

/* get pointer to AUQ memory for the device */
#define SMEM_CHT_MAC_AUQ_MEM_GET(dev) \
            SMEM_CHT_FIELD_PTR_GET_MAC(smemChtMemoryDeviceGet(dev,SMEM_CHT_MEMORY_DEVICE_AUQ_E),auqMem)

/* get pointer to FUQ memory for the device */
#define SMEM_CHT_MAC_FUQ_MEM_GET(dev) \
            SMEM_CHT_FIELD_PTR_GET_MAC(smemChtMemoryDeviceGet(dev,SMEM_CHT_MEMORY_DEVICE_FUQ_E),fuqMem)

/* get pointer to on chip fifo AUQ memory for the device */
#define SMEM_CHT_MAC_AUQ_FIFO_MEM_GET(dev) \
            SMEM_CHT_FIELD_PTR_GET_MAC(smemChtMemoryDeviceGet(dev,SMEM_CHT_MEMORY_DEVICE_ON_CHIP_FIFO_AUQ_E),auqFifoMem)

/*get the regDb of SIP4 or below*/
#define SMEM_CHT_MAC_REG_DB_GET(dev) \
            SMEM_CHT_FIELD_PTR_GET_MAC(dev,regAddrDb)

/* set/get if device support sip5 */
#define SMEM_CHT_IS_SIP5_GET(dev)   \
            dev->sipRevInfo.isSip5

/* set/get if device support sip5_10 */
#define SMEM_CHT_IS_SIP5_10_GET(dev)   \
            dev->sipRevInfo.isSip5_10

/* set/get if device support sip5_15 */
#define SMEM_CHT_IS_SIP5_15_GET(dev)   \
            dev->sipRevInfo.isSip5_15

/* set/get if device support sip5_20 */
#define SMEM_CHT_IS_SIP5_20_GET(dev)   \
            dev->sipRevInfo.isSip5_20


/* get the regDb of SIP5 */
#define SMEM_CHT_MAC_REG_DB_SIP5_GET(dev) \
            SMEM_CHT_FIELD_PTR_GET_MAC(dev,regAddrDbSip5)

/* set/get if device support DFX server */
#define SMEM_CHT_IS_DFX_SERVER(dev)   \
            SMEM_CHT_FIELD_GET_MAC(dev,supportExternalDfx)

/* get the regDb of DFX server */
#define SMEM_CHT_MAC_REG_DB_DFX_SERVER_GET(dev) \
            SMEM_CHT_FIELD_PTR_GET_MAC(dev,regAddrExternalDfx)

/* check if port is special port */
#define SMEM_CHT_PORT_IS_SPECIAL_MAC(dev,portNum)   \
    (((portNum) >= SNET_CHT_NULL_PORT_CNS && (portNum) <= SNET_CHT_CPU_PORT_CNS) ? 1 : 0)

/* number of bits that the formula of local to global port should use for the core id shift */
#define CORE_ID_NUM_BITS_LEFT_SHIFT_FOR_GLOBAL_PORT_CNS     4
/* mask on the global port to get the local port */
#define GLOBA_TO_LOCAL_PORT_MASK_CNS     (SMEM_BIT_MASK(CORE_ID_NUM_BITS_LEFT_SHIFT_FOR_GLOBAL_PORT_CNS))


/* get the 'port group' id from the global port number (bits 4..5)*/
#define SMEM_CHT_PORT_GROUP_ID_FROM_GLOBAL_PORT_MAC(dev,portNum)       \
        (SMEM_CHT_PORT_IS_SPECIAL_MAC(dev,portNum) ? (dev)->portGroupId :       \
        SMEM_U32_GET_FIELD((portNum),CORE_ID_NUM_BITS_LEFT_SHIFT_FOR_GLOBAL_PORT_CNS,((dev)->supportEArch ? 4 : 2)))

/* get the local port from the global port number (bits 0..3)*/
#define SMEM_CHT_LOCAL_PORT_FROM_GLOBAL_PORT_MAC(dev,portNum)          \
        (((dev)->portGroupSharedDevObjPtr == NULL || SMEM_CHT_PORT_IS_SPECIAL_MAC(dev,portNum)) ? (portNum) :                \
        SMEM_U32_GET_FIELD((portNum),0,CORE_ID_NUM_BITS_LEFT_SHIFT_FOR_GLOBAL_PORT_CNS))

/* get the global port from the local port number (bits 0..3)*/
#define SMEM_CHT_GLOBAL_PORT_FROM_LOCAL_PORT_MAC(dev,portNum)   \
        SMEM_U32_GET_FIELD(                                     \
        (((dev)->portGroupSharedDevObjPtr == NULL || SMEM_CHT_PORT_IS_SPECIAL_MAC(dev,portNum)) ? \
            (portNum) :  \
          ((dev)->portGroupSharedDevObjPtr->coreDevInfoPtr[(dev)->portGroupId].startPortNum + (portNum & GLOBA_TO_LOCAL_PORT_MASK_CNS)))\
        ,0,(dev)->flexFieldNumBitsSupport.phyPort)

/* the dual device Id (Lion2 only) uses only 2 bits of coreId in some cases of the Egress pipe */
#define SMEM_CHT_PORT_GROUP_ID_MASK_CORE_MAC(dev, portGroupId, dualDeviceIdEnable) \
        ((dualDeviceIdEnable) ? \
          ((portGroupId) & 0x3): /* use only 2 bits */  \
           (portGroupId)) /* don't modify */

/* get index into devMemInfoPtr->unitMemArr[] ,by convert it from unit name (string) */
#define UNIT_INDEX_FROM_STRING_GET_MAC(devObjPtr,unitNameStr)   \
    (UNIT_BASE_ADDR_FROM_STRING_GET_MAC(devObjPtr,unitNameStr) >> SMEM_CHT_UNIT_INDEX_FIRST_BIT_CNS)

/* get the base address of the unit ,by convert it from unit name (string) */
#define UNIT_BASE_ADDR_FROM_STRING_GET_MAC(devObjPtr,unitNameStr)   \
    (smemUnitBaseAddrByNameGet(devObjPtr,unitNameStr,0))


/* get index into devMemInfoPtr->unitMemArr[] ,by convert it from value of SMEM_LION_2_3_UNIT_NAME_ENT */
#define UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,unitName)   \
    (UNIT_BASE_ADDR_GET_MAC(devObjPtr,unitName) >> SMEM_CHT_UNIT_INDEX_FIRST_BIT_CNS)

/* get the base address of the unit according to pointer in the device memory */
#define UNIT_BASE_ADDR_GET_MAC(devObjPtr,unitName)   \
    (smemUnitBaseAddrByNameGet(devObjPtr,STR(unitName),0))

/* get the base address of the unit according to pointer in the device memory -
   allow not exists unit */
#define UNIT_BASE_ADDR_GET_ALLOW_NON_EXIST_UNIT_MAC(devObjPtr,unitName)   \
    (smemUnitBaseAddrByNameGet(devObjPtr,STR(unitName),1))


#define CHT_SFLOW_FIFO_SIZE       32
/***********************************************************************
*    0  Global Registers, SDMA registers, Master XSMI registers and TWSI
*       registers.
*    1 - 2  reserved
*    3  Transmit Queue registers
*    4  Ethernet Bridge registers
*    5  reserved
*    6  Buffer Management register
*    7  reserved
*    8  Ports group0 configuration registers (port0 through port5),
*       LEDs interface0 configuration registers and Master SMI
*       interface0 registers
*    9  Ports group1 configuration registers (port6 through port11) and
*       LEDs interface0.
*    10 Ports group2 configuration registers (port12 through port17),
*       LEDs interface1 configuration registersand Master SMI interface1
*       registers
*    11 Ports group3 configuration registers (port18through port23) and
*       LEDs interface1
*    12 MAC Table Memory
*    13 Internal Buffers memory Bank0 address space
*    14 Internal Buffers memory Bank1 address space
*    15 Buffers memory block configuration registers
*    20 VLAN Table configuration registers and VLAN Table address space.
*    21 Ports Registers, including CPU port (port# 0x3F).Bits 13:8 are used
*       as port number,Bits 7:2 are used as the register address,
*       bits 1:0 should be "00".
*    22 Eq unit registers
*    23 PCL registers and TCAM memory space
*    24 Policer registers and meters memory space
*    63-25  Reserved.
*
***********************************************************************/
/*
 * Typedef: struct CHT_GLOBAL_MEM
 *
 * Description:
 *      Global Registers, TWSI, CPU port, SDMA and PCI internal registers
 *
 * Fields:
 *      globRegNum    : Global registers number.
 *      globReg       : Global registers array with size of globRegNum.
 *      sdmaRegNum    : SDMA register number.
 *      sdmaReg       : SDMA register array with size of sdmaRegNum.
 *
 * Comments:
 *  For all registers from this struct Bits 23:28 == 0
 *
 *  globReg    - Registers with address mask 0x1FFFFF00 pattern 0x00000000
 *                globRegNum = 76
 *
 *  sdmaRegNum - Registers with address mask 0x1FFFF000 pattern 0x00002000
 *                sdmaRegNum = 150
 *
 */

typedef struct {
    GT_U32                  globRegNum;
    SMEM_REGISTER         * globReg;

    GT_U32                  sdmaRegNum;
    SMEM_REGISTER         * sdmaReg;
}CHT_GLOBAL_MEM;

/*
 * Typedef: struct CHT_EGR_MEM
 *
 * Description:
 *      Egress, Transmit Queue and VLAN Configuration Register Map Table,
 *      Port TxQ Configuration Register, Hyper.GStack Ports MIB Counters
 *
 * Fields:
 *      genRegNum      : Common registers number.
 *      genReg         : Common registers number, index is port and group
 *      txqInternRegNum : TxQ interanal registers number.
 *      txqInternReg    : TxQ interanal registers.
 *      hprMibCntRegNum: Hyper.GStack Ports MIB Counters number.
 *      hprMibCntReg   : Hyper.GStack Ports MIB Counters
 *      xsmiRegNum     : Master XSMI Interface Register Map Table number.
 *      xsmiReg        : Master XSMI Interface Register Map Table
 *      stcEgrReg      : Egress STC Table number.
 *      stcEgr         : Egress STC Table
 *
 * Comments:
 * For all registers from this struct Bits 23:28 == 3
 *
 *  genReg       - Registers with address mask 0x1FF00000 pattern 0x01800000
 *                 genRegNum = 308
 *  txqInternReg - Registers with address mask 0x1FF00000 pattern 0x01840000,
 *                 0x01880000, 0x018C0000, 0x01900000
 *                 txQInternRegNum = 512(8 TC * 64 ports)
 *  trnFilterReg - Registers with address mask 0x1FFFF000 pattern 0x01A40080
 *                 trFilterRegNum = 2032
 *  portWdReg    - Registers with address mask 0x1FFF0000 pattern 0x01A80000
 *                 portWdRegNum = 64
 *  egrMibCntReg - Registers with address mask 0x1FFFFF00 pattern 0x01B40100
 *                 egrMibCntReg = 120
 *  hprMibCntReg - Registers with address mask 0x1FF00000 pattern 0x01C00000
 *                 hprMibCntRegNum = 64
 *  xsmiReg      - Registers with address mask 0x1FFF0000 pattern 0x01CC0000
 *                 xsmiRegNum = 2
 *  stcEgrReg    - Registers with address mask 0x1FFF0000 pattern 0x01D40000
 *                 stcEgrRegNum = 27
 *  tailDropReg  - Registers with address mask 0x1FFFF000 pattern 0x01940000
 *                 tailDropRegNum = 3132
 *  egrRateShpReg- Registers with address mask 0x1FF00000 pattern 0x01A00000
 *                 egrRateShpRegNum = 512(8 TC * 64 ports)
 *  txSniffCntr =  Register of tx sniffering
 */

typedef struct {
    GT_U32                  genRegNum;
    SMEM_REGISTER         * genReg;

    GT_U32                  txqInternalRegNum;
    SMEM_REGISTER         * txqInternalReg[4];

    GT_U32                  trnFilterRegNum;
    SMEM_REGISTER         * trnFilterReg;

    GT_U32                  portWdRegNum;
    SMEM_REGISTER         * portWdReg;

    GT_U32                  egrMibCntRegNum;
    SMEM_REGISTER         * egrMibCntReg;

    GT_U32                  hprMibCntRegNum;
    SMEM_REGISTER         * hprMibCntReg[3];

    GT_U32                  xsmiRegNum;
    SMEM_REGISTER         * xsmiReg;

    GT_U32                  stcEgrRegNum;
    SMEM_REGISTER         * stcEgrReg[3];

    GT_U32                  tailDropRegNum;
    SMEM_REGISTER         * tailDropReg;

    GT_U32                  egrRateShpRegNum;
    SMEM_REGISTER         * egrRateShpReg[9];

}CHT_EGR_MEM;

/*
 * Typedef: struct CHT_BRDG_MNG_MEM
 *
 * Description:
 *      Describe a device's Bridge management registers memory object.
 *
 * Fields:
 *      genRegsMum        : Ethernet bridge registers number
 *      genRegs           : Ethernet bridge registers
 *
 * Comments:
 * For all registers from this struct Bits 23:28 == 4
 *
 *  genReg        -  Registers with address mask 0xFFFFFF00 pattern 0X02000200
 *                   genRegsNum = 262655
 */

typedef struct {
    GT_U32                  genRegsNum;
    SMEM_REGISTER         * genReg;
}CHT_BRDG_MNG_MEM;

/*
 * Typedef: struct CHT_BUF_MNG_MEM
 * Description:
 *      Describe a device's buffer management registers memory object.
 *
 * Fields:
 *
 *      bmRegNum           : General buffer management registers number
 *      bmReg              : General buffer management registers
 *
 * Comments:
 * For all registers from this struct Bits 23:28 == 6
 *
 *  bmReg          - Registers with address mask 0xfffff000 pattern 0x03000000
 *                    bmRegNum = 196
 *
 */

typedef struct {
    GT_U32                  bmRegNum;
    SMEM_REGISTER         * bmReg;
}CHT_BUF_MNG_MEM;

/*
 * Typedef: struct CHT_GOP_CONF_MEM
 * Description:
 *      LEDs, Tri-Speed Ports MIB Counters and Master SMI
 *      Configuration Registers
 *
 * Fields:
 *      gopRegNum          : Group configuration registers numbers
 *      gopReg             : Group configuration registers
 *
 *      macMibCountRegNum  : MAC MIB Counters registers numbers
 *      macMibCountReg     : MAC MIB Counters registers
 *
 *      ledRegNum          : LED Interfaces Configuration Registers numbers
 *      ledReg             : LED Interfaces Configuration Registers
 *
 * Comments:
 *      For all registers from this struct Bits 23:28 == 8,9,10,11
 *
 *      gopReg    -  Registers with address mask 0xFFFFF000 pattern 0x04004000
 *                   gopRegNum = 512
 *
 *      macMib0CountReg
 *                 - Registers with address mask 0xFFFFF000 pattern 0x04010000
 *                   macMibCountRegNum = 892
 *
 *      ledReg     - Registers with address mask 0xFFFFFF00 pattern 0x04005100
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
}CHT_GOP_CONF_MEM;

/*
 * Typedef: struct CHT_MAC_FDB_MEM
 *
 * Description:
 *      Describe a device's Bridge registers and FDB.
 *
 * Fields:
 *      fdbRegNum          : FDB Global Configuration Registers number.
 *      fdbReg             : FDB Global Configuration Registers.
 *      macTblRegNum       : Number of MAC Table registers.
 *      macTblReg          : MAC Table Registers.
 *
 * Comments:
 * For all registers from this struct Bits 23:28 == 12
 *
 *      fdbReg              Registers with address mask 0xFF00000 pattern
 *                          0x06000000
 *                          fdbRegNum = 124
 *
 *      macTblReg           This table may be accessed by the CPU using the
 *                          MAC Table Access Control registers
 *                          fdbRegNum = 4096
 *
 *      macUpdFifoRegs      MAC update FIFO registers.
 *                          macUpdFifoRegsNum = 4 * 16
 *                          (16 MAC Update entires, 4 words in the entry)
 *      macTblReg -         MAC table register.
 */
typedef struct {
    GT_U32                  fdbRegNum;
    SMEM_REGISTER         * fdbReg;

    GT_U32                  macTblRegNum;
    SMEM_REGISTER         * macTblReg;
}CHT_MAC_FDB_MEM;


/*
 * Typedef: struct CHT_BANKS_MEM
 *
 * Description:
 *      Describe a buffers memory banks data register
 *
 * Fields:
 *      bankWriteRegNum    : Buffers Memory Bank0 Write Data Register number.
 *      bankWriteReg       : Registers, index is group number.
 *
 *      bankMemRegNum      : Buffers Memory Address Space Bank number.
 *      bankMemReg         : Buffers Memory Address Space Bank
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
 */
typedef struct {
    GT_U32                  bankWriteRegNum;
    SMEM_REGISTER         * bankWriteReg;

    GT_U32                  bankMemRegNum;
    SMEM_REGISTER         * bankMemReg[2];
}CHT_BANKS_MEM;

/*
 * Typedef: struct CHT_BUF_MEM
 *
 * Description:
 *      Describe a Buffers Memory, Ingress MAC Errors Indications and
 *      Egress Header Alteration Register Map Table registers,
 *      Router Header Alteration Configuration Registers
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
}CHT_BUF_MEM;

/*
 * Typedef: struct CHT_VLAN_TBL_MEM
 *
 * Description:
 *      VLAN Table, Multicast Groups Table, and Span State Group Table
 *
 * Fields:
 *      vlanCnfRegNum   : VLAN configuration registers number
 *      vlanCnfReg      : VLAN configuration registers

 *      vlanTblRegNum   : Registers number for Vlan Table
 *      vlanTblReg      : Vlan Table Registers.
 *
 * Comments:
 * For all registers from this struct Bits 23:28 == 20
 *
 *   vlanCnfReg :   Registers with address mask 0xFF000000 pattern 0x0A0000000
 *                  vlanTblRegNum = 4
 *
 *   vlanTblReg :   VLAN<n> Entry (0<=n<4096)
 *                  vlanTblRegNum = 4096

 *   mcstTblReg :   Multicast Group<n> Entry (0<=n<4096)
 *                  mcstTblRegNum = 4096

 *   spanTblReg :   Span State Group<n> Entry (0<=n<256)
 *                  spanTblRegNum = 256
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

}CHT_VLAN_TBL_MEM;

/*
 * Typedef: struct CHT_TRI_SPEED_PORTS_MEM
 *
 * Description:
 *      Describe a Tri-Speed Ports MAC, CPU Port MAC, and 1.25 Gbps SERDES
 *      Configuration Registers Map Table,  Hyper.GStack Ports MAC and
 *      XAUI PHYs Configuration Registers Map Table
 *
 * Fields:
 *      portsRegNum     : Ports registers number
 *      portsReg        : Ports registers
 *
 * Comments:
 * For all registers from this struct Bits 23:28 == 21
 *
 *   portsReg  :  Registers with address mask 0xFFFF0000 pattern 0x0A800000
 *                portsRegNum = 16383
 */
typedef struct {
    GT_U32                  triSpeedPortsRegNum;
    SMEM_REGISTER         * triSpeedPortsReg;
}CHT_TRI_SPEED_PORTS_MEM;


/*
 * Typedef: struct CHT_PREEGR_MEM
 *
 * Description:
 *      Pre-Egress Engine Registers Map Table, Trunk Table,
 *      QoSProfile to QoS Table, CPU Code Table,
 *      Data Access Statistical Rate Limits Table Data Access,
 *      Ingress STC Table Registers
 *
 * Fields:
 *      portsRegNum         : Pre-Egress Engine Registers number
 *      portsReg            : Pre-Egress Engine Registers
 *
 *      genRegNum           : Ports registers number
 *      genReg              : Ports registers
 *
 *      trunkTblRegNum      : Trunk table memory size
 *      trunkTblReg         : Trunk table memory
 *
 *      qosTblRegNum        : QoSProfile to QoS table size
 *      qosTblReg           : QoSProfile to QoS table
 *
 *      cpuCodeTblRegNum    : CPU Code Table size
 *      cpuCodeTblReg       : CPU Code Table
 *
 *      portInStcTblRegNum  : Port Ingress STC Table size
 *      portInStcTblReg     : Port<n> Ingress STC Table
 *
 * Comments:
 *
 * For all registers from this struct Bits 23:28 == 22
 *
 *   portsReg       : Registers with address mask 0xFFFFFF00 pattern 0x0B000000
 *                   portsRegNum = 22
 *
 *   genReg         : Registers with address mask 0xFFFFF000 pattern 0x0B000000
 *                    portsRegNum = 22
 *
 *   trunkTblReg    : Trunk table Trunk<n> Member<i> Entry (1<=n<128, 0<=i<8)
 *                    trunkTblRegNum = 128
 *
 *   qosTblReg      : QoSProfile to QoS table Entry<n> (0<=n<72)
 *                    qosTblRegNum = 72
 *
 *   cpuCodeTblReg  : CPU Code Table Entry<n> (0<=n<256)
 *                    cpuCodeTblReg = 256
 *
 *   statRateTblReg:  Statistical Rate Limits Table Entry<n> (0<=n<32)
 *                    cpuCodeTblReg = 32
 *
 *   portInStcTblReg: Port<n> Ingress STC Table Entry (0<=n<27)
 *                    cpuCodeTblReg = 27
 *
 *   rxSniffCnrt    : Rx Sniffer Statistical counter
 */
typedef struct {
    GT_U32                  portsRegNum;
    SMEM_REGISTER         * portsReg;

    GT_U32                  genRegNum;
    SMEM_REGISTER         * genReg;

    GT_U32                  trunkTblRegNum;
    SMEM_REGISTER         * trunkTblReg[8];

    GT_U32                  qosTblRegNum;
    SMEM_REGISTER         * qosTblReg;

    GT_U32                  cpuCodeTblRegNum;
    SMEM_REGISTER         * cpuCodeTblReg;

    GT_U32                  statRateTblRegNum;
    SMEM_REGISTER         * statRateTblReg;

    GT_U32                  portInStcTblRegNum;
    SMEM_REGISTER         * portInStcTblReg;

}CHT_PREEGR_MEM;

/*
 * Typedef: struct CHT_PCL_MEM
 *
 * Description:
 *      PCL registers and TCAM memory space
 *
 * Fields:
 *   pclConfRegNum              : PCL Registers number
 *   pclConfReg                 : PCL Registers
 *
 *   tcamRegNum                 : TCAM memory size
 *   tcamReg                    : TCAM memory pointer
 *
 *   pclIntRegNum               : PCL internal registers number
 *   pclIntReg                  : PCL internal registers pointer
 *
 *   portVlanQosTblRegNum       : Ports VLAN and QoS configuration table size
 *   portVlanQosTblReg          : Ports VLAN and QoS configuration table
 *
 *   protBasedVlanQosTblRegNum  : Port Protocol Based VLANs and
                                  Protocol Based QoS table size
 *   protBaeddVlanQosTblReg     :
 *
 *   pceActionsTblRegNum        : Policy Action Table size
 *   pceActionsTblReg           : Policy Action Table
 *
 *   pclIdTblRegNum             :
 *   pclIdTblReg                :
 *
 * Comments:
 * For all registers from this struct Bits 23:28 == 23
 *
 *   pclConfReg :   Registers with address mask 0xFFFF0000 pattern 0x0B800000
 *                  portsRegNum = 1056
 *
 *   tcamReg    :   Registers with address mask 0xFFFF0000 pattern 0x0B810000
 *                  portsRegNum = 16384
 *
 *   pclIntReg  :   Registers with address mask 0xFFFF0000 pattern 0x0B820000
 *                  portsRegNum = 22
 *
 *   portVlanQosTblReg:
 *                  Ports VLAN and QoS configuration table
 *                  portVlanQosTblRegNum = 63 * 2 words
 *
 *   protBasedVlanQosTblReg:
 *                  Port Protocol Based VLANs and Protocol Based QoS table
 *                  protVlanQosTblRegNum = 63 * 2 * 8 words
 *
 *   pceActionsTblReg:
 *                  Policy Action Table
 *                  pceActionsTblRegNum = 1024 * 3 words
 *
 *   pclIdTblReg:   PCL-ID configuration Table
 *                  pclIdTblRegNum = 1152 (1024 + 128)
 */
typedef struct {
    GT_U32                  pclConfRegNum;
    SMEM_REGISTER         * pclConfReg;

    GT_U32                  tcamRegNum;
    SMEM_REGISTER         * tcamReg;

    GT_U32                  pclIntRegNum;
    SMEM_REGISTER         * pclIntReg;
    /* PCL tables */
    GT_U32                  portVlanQosTblRegNum;
    SMEM_REGISTER         * portVlanQosTblReg;

    GT_U32                  protBasedVlanQosTblRegNum;
    SMEM_REGISTER         * protBasedVlanQosTblReg[8];

    GT_U32                  pceActionsTblRegNum;
    SMEM_REGISTER         * pceActionsTblReg;

    GT_U32                  pclIdTblRegNum;
    SMEM_REGISTER         * pclIdTblReg;
}CHT_PCL_MEM;


/*
 * Typedef: struct CHT_POLICER_MEM
 *
 * Description:
 *      Policer registers and meters memory space
 *
 * Fields:
 *      policerRegNum           : Policer Registers number
 *      policerReg              : Policer Registers pointer
 *
 *      policerTblRegNum        : Policers Table size
 *      policerTblReg           : Policers Table
 *
 *      policerQosRmTblRegNum   : Policers QoS Remark. and Initial DP Table size
 *      policerQosRmTblReg      : Policers QoS Remark. and Initial DP Table
 *
 *      policerCntTblRegNum     : Policers Counters Table size
 *      policerCntTblReg        : Policers Counters Table
 *
 * Comments:
 * For all registers from this struct Bits 23:28 == 24
 *
 *   policerReg    :  Registers with address mask 0xFFFFFF00 pattern 0x0C000000
 *                    portsRegNum = 27
 *
 *   policerTblReg :  Policers Table Entry<n> (0<=n<256)
 *                    portsRegNum = 256
 *
 *   policerQosRmTblReg :
 *                    Policers QoS Remarking and Initial DP Table Entry
 *                    policerQosRmTblRegNum = 72
 *
 *   policerCntTblReg :
 *                    Policers Counters Table Entry<n> (0<=n<16)
 *                    policerQosRmTblRegNum = 16
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
}CHT_POLICER_MEM;

/*
 * Typedef: struct CHT_PCI_MEM
 *
 * Description:
 *      PCI Registers.
 *
 * Fields:
 *      pciRegNum      : PCI registers number.
 *      pciReg         : PCI registers
 *
 *
 * Comments:
 *      pciRegNum = 280
 */

typedef struct {
    GT_U32                  pciRegNum;
    SMEM_REGISTER         * pciReg;
}CHT_PCI_MEM;

/*
 * Typedef: struct CHT_PHY_MEM
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

}CHT_PHY_MEM;

/*
 * Typedef: struct CHT_AUQ_MEM
 *
 * Description:
 *      Address update message interface
 *
 * Fields:
 *     auqBase        - PCI base address space
 *     auqBaseValid   - TRUE  upon every new load of shadow to base.
 *                      FALSE upon write to the last write of AU to given section.
 *     auqBaseSize    - size of base AUQ
 *     auqShadow      - PCI shadow address space,
 *     auqShadowValid - TRUE upon every new shadow register update from PCI.
 *                      FALSE upon every load of shadow to base.
 *     auqShadowSize  - size of shadow AUQ
 *     auqOffset      - increment at 16 Bytes at every new AU write.
 *
 * Comments:
*/
typedef struct {
    GT_U32                      auqBase;
    GT_BOOL                     auqBaseValid;
    GT_U32                      auqBaseSize;
    GT_U32                      auqShadow;
    GT_BOOL                     auqShadowValid;
    GT_U32                      auqShadowSize;
    GT_U32                      auqOffset;
    GT_BOOL                     baseInit;
}CHT_AUQ_MEM;


/*
 * Typedef: struct CHT_IP_MEM
 *
 * Description:
 *      ARP MAC Destination Address and VLAN/Port MAC SA Tables
 *
 * Fields:
 *      arpTblReg   - ARP table memory pointer
 *      macSaTblReg - MAC SRC address table memory pointer
 *
 * Comments:
*/
typedef struct {
    GT_U32                  arpTblRegNum;
    SMEM_REGISTER         * arpTblReg;
    GT_U32                  macSaTblRegNum;
    SMEM_REGISTER         * macSaTblReg;
}CHT_IP_MEM;

/*
 * Typedef: struct CHT2_FUQ_MEM
 *
 * Description:
 *      FDB upload messages queue configuration
 *
 * Fields:
 *     fuqBase        - PCI FU base address space
 *     fuqBaseValid   - TRUE  upon every new load of shadow to base.
 *                      FALSE upon write to the last write of FU to given section.
 *
 *     fuqShadow      - PCI shadow address space,
 *     fuqShadowValid - TRUE upon every new shadow register update from PCI.
 *                      FALSE upon every load of shadow to base.
 *
 *     fuqOffset      - this value is in words !!! and not in messages !!!  --> UNLIKE AUQ !!!
 *                      (for SIP5) This to support 2 CNC messages that are 4 words size (aligned on 4 words) and the
 *                              FU messages that are 6 words size (aligned on 8 words).
 *     fuqNumMessages - number Of Messages already put on the AUQ
 *
 * Comments:
*/
typedef struct {
    GT_U32                      fuqBase;
    GT_BOOL                     fuqBaseValid;
    GT_U32                      fuqShadow;
    GT_BOOL                     fuqShadowValid;
    GT_U32                      fuqOffset;
    GT_BOOL                     baseInit;
    GT_U32                      fuqNumMessages;
}CHT2_FUQ_MEM;

/* number of registers per entry in line of MIB */
#define CHT_INTERNAL_SIMULATION_MIB_ENTRY_NUM_REGISTERS_CNS    32
/* up to 256 ports for MIB counters */
#define CHT_INTERNAL_SIMULATION_MIB_NUM_PORTS_CNS    SKERNEL_DEV_MAX_SUPPORTED_PORTS_CNS
/* number of registers needed for MIB counters (for all ports) */
#define CHT_INTERNAL_SIMULATION_MIB_COUNTERS_NUM_CNS \
            (CHT_INTERNAL_SIMULATION_MIB_NUM_PORTS_CNS * CHT_INTERNAL_SIMULATION_MIB_ENTRY_NUM_REGISTERS_CNS)


/* current know-memories are :
    CHT_INTERNAL_SIMULATION_USE_MEM_SECURITY_BREACH_STATUS_E - security breach status
        bit 0 - indication that the registers of 'security breach status' ready for the device to update
                value 0 - the CPU did not read the values yet , so the device can not update it.
                value 1 - the CPU read the values , so the device can update it.
    CHT_INTERNAL_SIMULATION_USE_MEM_RX_STATISTICAL_SNIFF_COUNTER_E - counter for statistical rx sniffer
    CHT_INTERNAL_SIMULATION_USE_MEM_TX_STATISTICAL_SNIFF_COUNTER_E - counter for statistical tx sniffer
    CHT_INTERNAL_SIMULATION_USE_MEM_CAPWAP_RSSI_EXCEPTION_STATUS_E - CAPWAP RSSI exception status
        it contain the status of the SMEM_CHT3_CAPWAP_RSSI_EXCEPTION_REG registers,
        where those registers read by CPU , and ready to be updated by new interrupt
        bit 0 - 1 bits - ready For new write
            value 0 - the device set values to SMEM_CHT3_CAPWAP_RSSI_EXCEPTION_REG registers
                       but CPU didn't read it yet.
                       is this state the device will not update those registers
            value 1 - the device can set new values to SMEM_CHT3_CAPWAP_RSSI_EXCEPTION_REG registers
                       this state is the AFTER initialization.
                       and after the CPU read the registers of SMEM_CHT3_CAPWAP_RSSI_EXCEPTION_REG
    CHT_INTERNAL_SIMULATION_USE_MEM_MSM_MIB_COUNTERS_START_E - start of internal memory to capture
                      MSM MIB counters on first word read of the entry. Per port. (From Lion2).
    CHT_INTERNAL_SIMULATION_USE_MEM_TX_STATISTICAL_SNIFF_COUNTER_E - end of internal memory to
                     capture MSM MIB counters.

    CHT_INTERNAL_SIMULATION_USE_MEM_INGRESS_STC_WRITE_WORD0_E - SIP5 : the ingress STC (sampling to CPU)
                     depend on control register , so we need active memory for this table and not the regular
                     table 'write on last word'


    CHT_INTERNAL_SIMULATION_USE_MEM_FOR_TRAFFIC_MANAGER_ADDRESS_23_MSBITS_E - TM unit holding 23 MSBits of address
    CHT_INTERNAL_SIMULATION_USE_MEM_FOR_TRAFFIC_MANAGER_DATA_32_LSBITS_E    - TM unit holding 32 LSBits of data


    CHT_INTERNAL_SIMULATION_USE_MEM_FOR_REMOTE_TRAFFIC_MANAGER_DATA_32_LSBITS_E    - remote TM unit holding 32 LSBits of data
    CHT_INTERNAL_SIMULATION_USE_MEM_FOR_REMOTE_TRAFFIC_MANAGER_DATA_32_MSBITS_E    - remote TM unit holding 32 MSBits of data


*/
typedef enum{
    CHT_INTERNAL_SIMULATION_USE_MEM_SECURITY_BREACH_STATUS_E,
    CHT_INTERNAL_SIMULATION_USE_MEM_RX_STATISTICAL_SNIFF_COUNTER_E,
    CHT_INTERNAL_SIMULATION_USE_MEM_TX_STATISTICAL_SNIFF_COUNTER_E,

    CHT_INTERNAL_SIMULATION_USE_MEM_CAPWAP_RSSI_EXCEPTION_STATUS_E,/*ch3p only*/

    CHT_INTERNAL_SIMULATION_USE_MEM_MSM_MIB_COUNTERS_START_E,
    CHT_INTERNAL_SIMULATION_USE_MEM_MSM_MIB_COUNTERS_END_E =
        CHT_INTERNAL_SIMULATION_USE_MEM_MSM_MIB_COUNTERS_START_E +
            CHT_INTERNAL_SIMULATION_MIB_COUNTERS_NUM_CNS - 1,

    CHT_INTERNAL_SIMULATION_USE_MEM_INGRESS_STC_TEMP_WORD0_E,

    CHT_INTERNAL_SIMULATION_USE_MEM_FOR_TRAFFIC_MANAGER_ADDRESS_23_MSBITS_E,
    CHT_INTERNAL_SIMULATION_USE_MEM_FOR_TRAFFIC_MANAGER_DATA_32_LSBITS_E,

    CHT_INTERNAL_SIMULATION_USE_MEM_FOR_REMOTE_TRAFFIC_MANAGER_DATA_32_LSBITS_E,
    CHT_INTERNAL_SIMULATION_USE_MEM_FOR_REMOTE_TRAFFIC_MANAGER_DATA_32_MSBITS_E,


    CHT_INTERNAL_SIMULATION_USE_MEM_TOTAL_E/* must be last one !!! */
}CHT_INTERNAL_SIMULATION_USE_MEM_ENT;

/*
 * Typedef: struct CHT3_INTERNAL_SIMULATION_USE_MEM
 *
 * Description:
 *      internal simulation memory use
 *
 * Fields:
 *      internalRegNum      - number of internal registers
 *      internalRegPtr      - pointer to internal registers
 *
 * Comments:
*/
typedef struct {
    GT_U32                  internalRegNum;
    SMEM_REGISTER         * internalRegPtr;
}CHT_INTERNAL_SIMULATION_USE_MEM;

/* macro to access the internal memory registers */
#define CHT_INTERNAL_MEM_PTR(dev,memIndex)   \
            SMEM_CHT_FIELD_PTR_GET_MAC(dev,internalSimMem.internalRegPtr[memIndex])

/* on chip fifo : number of entries */
#define MAC_ON_CHIP_FIFO_NUM_ENTRIES_CNS    16
/* MAX number of words in the AU message (ch1..lion2 need 4 words , Lion3 need 6 words) */
#define AU_MESSAGE_MAX_NUM_WORDS_CNS        8

/* number of words to hold in macUpdFifoRegs */
#define     MAC_UPD_FIFO_REGS_NUM           (AU_MESSAGE_MAX_NUM_WORDS_CNS * MAC_ON_CHIP_FIFO_NUM_ENTRIES_CNS)

/* invalid MAC message  */
#define     SMEM_CHEETAH_INVALID_MAC_MSG_CNS    0xffffffff


/* the internal memory of the AU fifo on chip memory */
typedef struct {
    SMEM_REGISTER         macUpdFifoRegs[MAC_UPD_FIFO_REGS_NUM];
}CHT_AUQ_FIFO_MEM;

/* PTP timestamp message length in words */
#define GTS_PTP_MSG_WORDS                       2
/* Timestamp fifo size */
#define     GTS_FIFO_REGS_NUM                   (32 * GTS_PTP_MSG_WORDS)

/* The internal memory of the timestamp fifo on chip memory */
typedef struct {
    SMEM_REGISTER gtsFifoRegs[GTS_FIFO_REGS_NUM];
}SMEM_LION_GTSQ_FIFO;

/*
 * Description:
 *      Typedef of the allocated memory structure
 *
 * Fields:
 *      regNum      - number of allocated registers
 *      regPtr      - pointer to allocated registers
 */
typedef struct {
    GT_U32                  regNum;
    SMEM_REGISTER         * regPtr;
} SMEM_CALLOC_STC;

/* Maximum allocations that could be stored in internal array */
#define SMEM_MAX_CALLOC_MEM_SIZE                    256

#define SMEM_CHT_PP_REGS_UNIT_START_INFO_MAGIC_NUMBER_CNS   0xF0E1D2C3

/* macro to set SMEM_CHT_PP_REGS_UNIT_START_INFO_STC of a unit/subUnit :
    dev - pointer to device info
    unitPath - path to the unit/sub unit  --> also used as 'name of the unit'
    baseAddress - base address for the unit/sub unit

*/
#define SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(dev,unitPath,baseAddress)                                  \
    SMEM_CHT_MAC_REG_DB_GET(dev)->unitPath.startUnitInfo.magicNumber = SMEM_CHT_PP_REGS_UNIT_START_INFO_MAGIC_NUMBER_CNS; \
    SMEM_CHT_MAC_REG_DB_GET(dev)->unitPath.startUnitInfo.unitBaseAddress = baseAddress;                                   \
    SMEM_CHT_MAC_REG_DB_GET(dev)->unitPath.startUnitInfo.unitNameStr = #unitPath

/* macro to set SMEM_CHT_PP_REGS_UNIT_START_INFO_STC of a unit/subUnit - for SIP5 :
    dev - pointer to device info
    unitPath - path to the unit/sub unit  --> also used as 'name of the unit'
    baseAddress - base address for the unit/sub unit

    NOTE: function smemGenericRegistersArrayAlignToUnit() set the
        startUnitInfo.unitBaseAddress into the registers of the unit
*/
#define SMEM_CHT_SIP5_PP_REGS_UNIT_START_INFO_SET_MAC(dev,unitPath,baseAddress)                                  \
    SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->unitPath.startUnitInfo.magicNumber = SMEM_CHT_PP_REGS_UNIT_START_INFO_MAGIC_NUMBER_CNS; \
    SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->unitPath.startUnitInfo.unitBaseAddress = baseAddress;                                   \
    SMEM_CHT_MAC_REG_DB_SIP5_GET(dev)->unitPath.startUnitInfo.unitNameStr = #unitPath


/*  SMEM_CHT_PP_REGS_UNIT_START_INFO_STC :
    structure to hold info about the unit/sub unit that start

    magicNumber - magic number to recognize that the unit/sub unit starts
                NOTE: this field MUST be FIRST in the structure.
    unitBaseAddress - base address of the unit/sub unit
    unitNameStr - string with the unit/sub unit name
*/
typedef struct{
    GT_U32      magicNumber; /*SMEM_CHT_PP_REGS_UNIT_START_INFO_MAGIC_NUMBER_CNS*/
    GT_U32      unitBaseAddress;
    char       *unitNameStr;
}SMEM_CHT_PP_REGS_UNIT_START_INFO_STC;


typedef struct /*SMEM_CHT_PP_REGS_ITS_ETS_UNIT_STC*/{

    GT_U32    globalConfigs;
    GT_U32    enableTimestamping;
    GT_U32    timestampingPortEnable[2]/*port div 32*/;
    GT_U32    TODCntrNanoseconds;
    GT_U32    TODCntrSeconds[2]/*index*/;
    GT_U32    TODCntrShadowNanoseconds;
    GT_U32    TODCntrShadowSeconds[2]/*index*/;
    GT_U32    GTSInterruptCause;
    GT_U32    GTSInterruptMask;
    GT_U32    globalFIFOCurrentEntry[2];/*index*/
    GT_U32    overrunStatus;
    GT_U32    underrunStatus;

}SMEM_CHT_PP_REGS_GTS_UNIT_STC;

typedef struct /*SMEM_CHT_PP_REGS_MPPM_UNIT_STC */{
    SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;

    struct /*ecc*/{

        GT_U32    eccCtrl;

    }ecc;

    GT_U32    xlgMode;
    GT_U32    interruptCause;
    GT_U32    interruptMask;

}SMEM_CHT_PP_REGS_MPPM_UNIT_STC ;


typedef struct /*SMEM_CHT_PP_REGS_CTU_UNIT_STC */{
    SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;

    struct /*CTURegs*/{

        GT_U32    CTUClearFIFOThreshold;
        GT_U32    CTUWRRDescTxDMAArbiterWeights;
        GT_U32    CTUFinishedCutThroughPktFCPLatency;
        GT_U32    descRefreshFinishedCutThroughPktFCPThreshold;
        GT_U32    ECCConfig;
        GT_U32    finishedCutThroughPktFCPDFXReg;
        GT_U32    descRefreshDFX;
        GT_U32    CTUMetalFix;
        GT_U32    BMACTParallelDROTimerValue;
        GT_U32    BMACTParallelDROCtrl;

        GT_U32    descCTDescCntr[4]/*desc*/;
        GT_U32    descAllDescCntr[4]/*desc*/;
        GT_U32    descNonCTDescCntr[4]/*desc*/;
        GT_U32    descCTFinishedDescCntr[4]/*desc*/;
        GT_U32    descCTNonFinishedDescCntr[4]/*desc*/;
        GT_U32    clearAllClearCntr[4]/*clr*/;
        GT_U32    clearCTClearCntr[4]/*clr*/;
        GT_U32    clearNonCTClearCntr[4]/*clr*/;
        GT_U32    clearCTFinishedClearCntr[4]/*clr*/;
        GT_U32    clearCTNonFinishedClearCntr[4]/*clr*/;
        GT_U32    FCPRxDMAClearCntr[4]/*dbm*/;
        GT_U32    rxDMATotalCTPktsCntr[4]/*dbm*/;
        GT_U32    rxDMAOngoingCTPktsCntr[4]/*dbm*/;
        GT_U32    BMACTParallelDROLVTCntr;
        GT_U32    BMACTParallelDRONVTCntr;
        GT_U32    BMACTParallelDROHVTCntr;

        GT_U32    CTUInterruptCause;
        GT_U32    CTUInterruptMask;

        GT_U32    BMA2CTUPipeLastClear[4]/*pipe*/;
        GT_U32    CTU2BMLastFinalClear[4]/*pipe*/;
        GT_U32    CTU2BMLastRxDMAClear[4]/*pipe*/;
        GT_U32    txDMA2CTULastBCQuery[4]/*pipe*/;
        GT_U32    CTU2TxDMALastBCReply[4]/*txdma*/;
        GT_U32    RXDMA2CTULastUpdate[4]/*rxdma*/;
        GT_U32    FCPECCStatus[4]/*dbm*/;
        GT_U32    CTUDebugBus;
        GT_U32    clearFIFOStatus[4]/*clear*/;
        GT_U32    descFIFOStatus[4]/*clear*/;
        GT_U32    descClear01FIFOOverrunStatus;
        GT_U32    descClear23FIFOOverrunStatus;
        GT_U32    FCP_OCP_Interrupt_Status[4]/*dbm*/;
        GT_U32    FCP_FIFO_Full_Status[4]/*dbm*/;
        GT_U32    FCP_FIFO_Empty_Status[4]/*dbm*/;
        GT_U32    FCP_FIFO_Overrun_Int_Status[4]/*dbm*/;
        GT_U32    descUnusedFIFOECCErrorType;

    }CTURegs;

}SMEM_CHT_PP_REGS_CTU_UNIT_STC;


typedef struct /*SMEM_CHT_PP_REGS_POLICER_UNIT_STC */{
    SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;

DONE_SIP5_ALIGN    GT_U32    policerCtrl0;
DONE_SIP5_ALIGN    GT_U32    policerCtrl1;
DONE_SIP5_ALIGN    GT_U32    policerPortMetering0;
DONE_SIP5_ALIGN    GT_U32    policerPortMetering1;
    GT_U32    policerMRU;
REG_MACRO_EXIST    GT_U32    IPFIXCtrl;
REG_MACRO_EXIST    GT_U32    IPFIXNanoTimerStampUpload;
    GT_U32    IPFIXSecondsLSbTimerStampUpload;
    GT_U32    IPFIXSecondsMSbTimerStampUpload;
REG_MACRO_EXIST    GT_U32    hierarchicalPolicerCtrl;
REG_MACRO_EXIST    GT_U32    policerIPFIXDroppedPktCountWAThreshold;
REG_MACRO_EXIST    GT_U32    policerIPFIXPktCountWAThreshold;
REG_MACRO_EXIST    GT_U32    policerIPFIXBytecountWAThresholdLSB;
    GT_U32    policerIPFIXBytecountWAThresholdMSB;
REG_MACRO_EXIST    GT_U32    IPFIXSampleEntriesLog0;
    GT_U32    IPFIXSampleEntriesLog1;
    GT_U32    policerError;
    GT_U32    policerErrorCntr;
    GT_U32    meteringAndCountingRamSize[3];
    GT_U32    policerTableAccessCtrl;
    GT_U32    policerTableAccessData[8]/*Reg*/;
REG_MACRO_EXIST    GT_U32    policerInitialDP[8]/*RegNum*/;
REG_MACRO_EXIST    GT_U32    policerInterruptCause;
    GT_U32    policerInterruptMask;
    GT_U32    policerShadow[2]/*RegNum*/;
    GT_U32    portAndPktTypeTranslationTable[32]/*port*/;
}SMEM_CHT_PP_REGS_POLICER_UNIT_STC /*iplr0,1,eplr*/ ;

/*
 * Typedef: struct SMEM_CHT_PP_REGS_ADDR_STC
 *
 * Description:
 *      Global registers data structure that represents registers addresses
 *      in common units
 *
 * Fields:
 *      unitBaseAddress     - unit base address
 *      Generic register names in common units
 *
 * Comments:
 *      All units structures taken from Cider and built by 'AutoBuilder'
 */
typedef struct {
    struct /*MG - MG */{
        SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;

        struct /*addrDecoding*/{

            GT_U32    unitDefaultAddrUDA;
            GT_U32    unitDefaultIDUDID;
            GT_U32    unitErrorAddrUEA;
            GT_U32    baseAddr[6]/*BA*/;
            GT_U32    sizeS[6]/*SR*/;
            GT_U32    highAddrRemapHA[4]/*HARR*/;
            GT_U32    baseAddrEnableBARE;
            GT_U32    accessProtectAP;

        }addrDecoding;

        struct /*addrUpdateQueueConfig*/{

            GT_U32    generalAddrUpdateQueueBaseAddr;
            GT_U32    generalAddrUpdateQueueCtrl;
            GT_U32    FUAddrUpdateQueueBaseAddr;
            GT_U32    FUAddrUpdateQueueCtrl;

            GT_U32    GeneralControl;/* sip5 only */
            GT_U32    AUQHostConfiguration;/* sip5 only */

        }addrUpdateQueueConfig;

        struct /*cpuPortConfigRegAndMIBCntrs*/{

            GT_U32    cpuPortGoodFramesSentCntr;
            GT_U32    cpuPortMACTransErrorFramesSentCntr;
            GT_U32    cpuPortGoodOctetsSentCntr;
            GT_U32    cpuPortRxInternalDropCntr;
            GT_U32    cpuPortGoodFramesReceivedCntr;
            GT_U32    cpuPortBadFramesReceivedCntr;
            GT_U32    cpuPortGoodOctetsReceivedCntr;
            GT_U32    cpuPortBadOctetsReceivedCntr;
            GT_U32    cpuPortGlobalConfig;

        }cpuPortConfigRegAndMIBCntrs;

        struct /*genxsReadDMARegs*/{

            GT_U32    genxsReadDMABufferPointer;
            GT_U32    genxsReadDMAStartingAddr;
            GT_U32    genxsReadDMACtrl;

        }genxsReadDMARegs;

        struct /*globalConfig*/{

            GT_U32    addrCompletion;
            GT_U32    FTDLL;
            GT_U32    lastReadTimeStamp;
            GT_U32    initDoneSummary;
            GT_U32    deviceID;
            GT_U32    vendorID;
            GT_U32    globalCtrl;
            GT_U32    extGlobalCtrl;
            GT_U32    extGlobalCtrl2;
            GT_U32    extGlobalCtrl3;
            GT_U32    IOSelectors;
            GT_U32    XBARArbiterCfg;
            GT_U32    sampledAtResetReg;
            GT_U32    mediaInterface;
            GT_U32    CTGlobalConfig;

            GT_U32    generalConfig;/*sip5*/
        }globalConfig;

        struct /*globalInterrupt*/{

            GT_U32    globalInterruptCause;
            GT_U32    globalInterruptMask;
            GT_U32    miscellaneousInterruptCause;
            GT_U32    miscellaneousInterruptMask;
            GT_U32    stackPortsInterruptCause;
            GT_U32    stackPortsInterruptMask;
            GT_U32    SERInterruptsSummary;
            GT_U32    SERInterruptsMask;

            /* sip5 */ /* manually added */
            GT_U32    functionalInterruptsSummaryCause;
            GT_U32    functionalInterruptsSummaryMask;
            GT_U32    functionalInterruptsSummary1Cause;
            GT_U32    functionalInterruptsSummary1Mask;
            GT_U32    ports1InterruptsSummaryCause;
            GT_U32    ports1InterruptsSummaryMask;
            GT_U32    ports2InterruptsSummaryCause;
            GT_U32    ports2InterruptsSummaryMask;

        }globalInterrupt;

        struct /*interruptCoalescingConfig*/{

            GT_U32    interruptCoalescingConfig;

        }interruptCoalescingConfig;
        struct /*MGMiscellaneous*/{

            GT_U32    GPPInputReg;
            GT_U32    GPPOutputReg;
            GT_U32    GPPIOCtrlReg;

            GT_U32    unitsInitDoneStatus0;
            GT_U32    unitsInitDoneStatus1;
            GT_U32    initStage;

            GT_U32    userDefinedReg[4]/*Reg_Num*/;
        }MGMiscellaneous;

        struct /*PLLConfig*/{

            GT_U32    core_PLL_Parameters;
            GT_U32    core_PLL_Ctrl;

            GT_U32    MPPM_PLL_Parameters;
            GT_U32    MPPM_PLL_Ctrl;

        }PLLConfig;

        struct /*SDMARegs*/{

            GT_U32    receiveSDMACurrentDescPointer[8]/*Pointer*/;
            GT_U32    receiveSDMAQueueCommand;
            GT_U32    transmitSDMACurrentDescPointer[8]/*Pointer*/;
            GT_U32    txSDMATokenBucketQueueCntr[8]/*Queue*/;
            GT_U32    txSDMATokenBucketQueueConfig[8]/*Queue*/;
            GT_U32    transmitSDMAWeightedRoundRobinQueueConfig[8]/*Queue*/;
            GT_U32    txSDMATokenBucketCntr;
            GT_U32    SDMAConfig;
            GT_U32    receiveSDMAStatus;
            GT_U32    receiveSDMAPktCount[8]/*SDMA*/;
            GT_U32    receiveSDMAByteCount[8]/*SMDA*/;
            GT_U32    receiveSDMAResourceErrorCount[2]/*SDMA_Num -> for SIP5 replcaed by receiveSDMAResourceErrorCountAndMode */;
            GT_U32    transmitSDMAQueueCommand;
            GT_U32    transmitSDMAFixedPriorityConfig;
            GT_U32    transmitSDMAWRRTokenParameters;
            GT_U32    transmitSDMAPacketGeneratorConfigQueue[8]; /* sip5 & xCat3: Queue */
            GT_U32    transmitSDMAPacketCountConfigQueue[8]; /* sip5 & xCat3: Queue */
            GT_U32    receiveSDMAResourceErrorCountAndMode[8]; /* sip5 & xCat3: Queue */
            GT_U32    receiveSDMAInterruptCause;
            GT_U32    transmitSDMAInterruptCause;
            GT_U32    receiveSDMAInterruptMask;
            GT_U32    transmitSDMAInterruptMask;

        }SDMARegs;

        struct /*TWSIConfig*/{

            GT_U32    TWSIGlobalConfig;
            GT_U32    TWSILastAddr;
            GT_U32    TWSITimeoutLimit;
            GT_U32    TWSIStateHistory0;
            GT_U32    TWSIStateHistory1;
            GT_U32    TWSIInternalBaudRate;

        }TWSIConfig;
        struct /*MGRegsMGMiscellaneous*/{
            GT_U32    confiProcessorGlobalConfig;
            GT_U32    Z80DoorbellReg;
            GT_U32    Z80GlobalInterruptCause;
            GT_U32    Z80GlobalInterruptMask;
            GT_U32    Z80InterruptCause;
            GT_U32    Z80InterruptMask;

            GT_U32    DFXInterruptsSummaryCause;
            GT_U32    DFXInterruptsSummaryMask;

        }MGRegsMGMiscellaneous;

        struct /*userDefined*/{

            GT_U32    userDefinedReg[4]/*Reg_Num*/;

        }userDefined;

        struct /*XSMI*/{

            GT_U32    XSMIManagement;
            GT_U32    XSMIAddr;
            GT_U32    XSMIConfig;
            GT_U32    XSMIInterruptCause;
            GT_U32    XSMIInterruptMask;

        }XSMI;

    }MG /*- MG*/ ;
    struct /*egrAndTxqVer0 - Egress and Txq */{
        SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;

        struct /*oam8023ahLoopbackEnablePerPortReg*/{

            GT_U32    oam8023ahLoopbackEnablePerPortReg;

        }oam8023ahLoopbackEnablePerPortReg;

        struct /*bridgeEgrConfig*/{

            GT_U32    mcLocalEnableConfig;
            GT_U32    egrFilteringReg0;
            GT_U32    egrFilteringReg1;
            GT_U32    IPMcRoutedPktsEgrFilterConfig;
            GT_U32    egrFilteringReg2;

        }bridgeEgrConfig;

        struct /*CCFC*/{

            GT_U32    BCNCtrl;
            GT_U32    pauseTriggerEnable;
            GT_U32    BCNGlobalPauseThresholds;
            GT_U32    BCNInterruptCause;
            GT_U32    BCNInterruptMask;
            GT_U32    portsBCNProfileReg[3];
            GT_U32    portsSpeedIndexesReg[3];
            GT_U32    globalBuffersCntr;
            GT_U32    BCNPortsTriggerCntrsConfig;
            GT_U32    BCNQueuesTriggerCntrsConfig;
            GT_U32    globalXoffCntr;
            GT_U32    portBCNTriggerCntr;
            GT_U32    queueBCNTriggerCntr;

        }CCFC;

        struct /*CNCModes*/{

            GT_U32    CNCModesReg;

        }CNCModes;

        struct /*dequeueSWRR*/{

            GT_U32    dequeueSWRRWeightReg[4];

        }dequeueSWRR;

        struct /*deviceMapTable*/{

            GT_U32    deviceMapTableEntry[32]/*Dev*/;

        }deviceMapTable;

        struct /*DPToCFIMap*/{

            GT_U32    DP2CFIEnablePerPortReg;
            GT_U32    DP2CFIReg;

        }DPToCFIMap;

        struct /*egrForwardingRestrictions*/{

            GT_U32    fromCPUPktsFilterConfig;
            GT_U32    bridgedPktsFilterConfig;
            GT_U32    routedPktsFilterConfig;

        }egrForwardingRestrictions;

        struct /*egrRateShapersConfig*/{

            GT_U32    tokenBucketUpdateRateAndMCFIFOConfig;
            GT_U32    tokenBucketUpdateRate1;
            GT_U32    portTCTokenBucketConfig[29][8]/*Port*//*TC*/;
            GT_U32    portTokenBucketConfig[29]/*Port*/;

        }egrRateShapersConfig;

        struct /*egrSTCTable*/{

            GT_U32    portEgrSTCTableEntryWord0[28]/*Port*/;
            GT_U32    portEgrSTCTableEntryWord1[28]/*Port*/;
            GT_U32    portEgrSTCTableEntryWord2[28]/*Port*/;

        }egrSTCTable;

        struct /*fastStackFailover*/{

            GT_U32    secondaryTargetPortMapTableEntry[28]/*Port*/;

        }fastStackFailover;

        struct /*GPPRegs*/{

            GT_U32    GPPInputReg;
            GT_U32    GPPOutputReg;
            GT_U32    GPPIOCtrlReg;

        }GPPRegs;

        struct /*linkListCtrl*/{

            GT_U32    linkListCtrlConfig;

        }linkListCtrl;

#if 0
        struct /*linksStatus*/{

            GT_U32    linksStatusReg;

        }linksStatus;
#endif /*0*/

        struct /*mirroredPktsToAnalyzerPortDescsLimit*/{

            GT_U32    mirroredPktsToAnalyzerPortDescsLimitConfig;

        }mirroredPktsToAnalyzerPortDescsLimit;

        struct /*perPortTransmitQueueConfig*/{

            GT_U32    portTxqConfig[29]/*Port*/;

        }perPortTransmitQueueConfig;

        struct /*portIsolationConfig*/{

            GT_U32    L2PortIsolationTableEntry[2176]/*SrcDev_SrcPort*/;
            GT_U32    L3PortIsolationTableEntry[2176]/*SrcDev_SrcPort*/;

        }portIsolationConfig;

        struct /*portL1OverheadConfig*/{

            GT_U32    portL1OverheadConfig[29]/*Port*/;

        }portL1OverheadConfig;

        struct /*resourceSharingRegs*/{

            GT_U32    priorityReg[2]/*Profile*/;

        }resourceSharingRegs;

        struct /*sourceIDEgrFiltering*/{

            GT_U32    ucSrcIDEgrFilteringConfig;
            GT_U32    srcIDEgrFilteringTableEntry[32]/*SrcID*/;

        }sourceIDEgrFiltering;

        struct /*stackTrafficSeparation*/{

            GT_U32    isStackPortReg;
            GT_U32    stackTCRemapTableEntry[8]/*TC*/;

        }stackTrafficSeparation;

        struct /*statisticalAndCPUTrigEgrMirrToAnalyzerPort*/{

            GT_U32    statisticalAndCPUTrigEgrMirrToAnalyzerPortConfig;

        }statisticalAndCPUTrigEgrMirrToAnalyzerPort;

        struct /*tailDropProfileConfig*/{

            GT_U32    profilePortTailDropLimitsConfig[8]/*Profile*/;
            GT_U32    queueWRTDMask;
            GT_U32    portWRTDMask;
            GT_U32    poolWRTDMask;
            GT_U32    profilePortWRTDEn[8]/*profile*/;
            GT_U32    profileQueueWRTDEn[8]/*profile*/;
            GT_U32    profilePoolWRTDEn[8]/*profile*/;
            GT_U32    profileTCDP0TailDropLimits[8][8]/*Profile*//*TC*/;
            GT_U32    profileTCDP1TailDropLimits[8][8]/*Profile*//*TC*/;

        }tailDropProfileConfig;

        struct /*transmitQueueTxAndEgrMIBCntrs*/{

            GT_U32    txqMIBCntrsSetConfig[2]/*Set*/;
            GT_U32    setOutgoingUcPktCntr[2]/*Set*/;
            GT_U32    setOutgoingMcPktCntr[2]/*Set*/;
            GT_U32    setOutgoingBcPktCount[2]/*Set*/;
            GT_U32    setBridgeEgrFilteredPktCount[2]/*Set*/;
            GT_U32    setTailDroppedPktCntr[2]/*Set*/;
            GT_U32    setCtrlPktCntr[2]/*Set*/;
            GT_U32    setEgrForwardingRestrictionDroppedPktsCntr[2]/*Set*/;
            GT_U32    setMcFIFODroppedPktsCntr[2]/*Set*/;

        }transmitQueueTxAndEgrMIBCntrs;

        struct /*transmitQueueGlobalConfig*/{

            GT_U32    transmitQueueCtrl;
            GT_U32    transmitQueueExtCtrl;
            GT_U32    transmitQueueExtCtrl1;
            GT_U32    transmitQueueResourceSharing;
            GT_U32    BZTransmitQueueMetalFix;
            GT_U32    portsLinkEnable;

        }transmitQueueGlobalConfig;

        struct /*transmitQueueInterrupt*/{

            GT_U32    transmitQueueInterruptSummaryCause;
            GT_U32    transmitQueueInterruptSummaryMask;
            GT_U32    transmitQueueFlushDoneInterruptCause;
            GT_U32    transmitQueueFlushInterruptMask;
            GT_U32    transmitQueueGeneralInterruptCause;
            GT_U32    transmitQueueGeneralInterruptMask;
            GT_U32    egrSTCInterruptCause;
            GT_U32    egrSTCInterruptMask;
            GT_U32    transmitQueueDescFullInterruptSummaryCause;
            GT_U32    GPPInterruptMaskReg;
            GT_U32    GPPInterruptCauseReg;
            GT_U32    transmitQueueFullInterruptMask;

        }transmitQueueInterrupt;

        struct /*transmitQueueTotalBuffersLimit*/{

            GT_U32    totalBufferLimitConfig;

        }transmitQueueTotalBuffersLimit;

        struct /*transmitSchedulerProfilesConfig*/{

            GT_U32    profileSDWRRWeightsConfigReg0[8]/*Profile*/;
            GT_U32    profileSDWRRWeightsConfigReg1[8]/*Profile*/;
            GT_U32    profileSDWRRAndStrictPriorityConfig[8]/*Profile*/;

        }transmitSchedulerProfilesConfig;

        struct /*trunksFilteringAndMcDistConfig*/{

            GT_U32    trunkNonTrunkMembersTable[128]/*Trunk*/;
            GT_U32    old_designatedTrunkPortEntryTable[8]/*Entry*/;    /* xcat3 legacy table - usually not used */
            GT_U32    designatedTrunkPortEntryTable[16]/*Entry*/;

        }trunksFilteringAndMcDistConfig;

        struct /*txqCntrs*/{

            GT_U32    totalBuffersCntr;
            GT_U32    totalDescCntr;
            GT_U32    totalBufferCntr;
            GT_U32    totalAllocatedDescsCntr;
            GT_U32    mcDescsCntr;
            GT_U32    snifferDescsCntr;
            GT_U32    portDescsCntr[29]/*Port*/;
            GT_U32    portTCDescCntrs[29][8]/*Port*//*TC*/;
            GT_U32    portBuffersCntr[29]/*Port*/;
            GT_U32    portTCBuffersCntrs[29][8]/*Port*//*TC*/;

        }txqCntrs;

        struct /*VLANAndMcGroupAndSpanStateGroupTables*/{

            GT_U32    globalConfig;
            GT_U32    VLTTablesAccessDataReg[6];
            GT_U32    VLTTablesAccessCtrl;

        }VLANAndMcGroupAndSpanStateGroupTables;

        struct /*XGCtrl*/{

            GT_U32    XGCtrlReg;

        }XGCtrl;

        struct /*XSMIConfig*/{

            GT_U32    XSMIManagement;
            GT_U32    XSMIAddr;

        }XSMIConfig;

        struct /*br8021Config*/{

            GT_U32    globalConfig;
            GT_U32    dp2cfiEnablePerPortReg;
            GT_U32    pcidOfTrgPort[32];

        }br8021Config;

    }egrAndTxqVer0 /*- Egress and Txq*/ ;

    struct /*egrAndTxqVer1 - Egress and Txq*/ {
        struct /*TXQ - TXQ */{

            struct /*TXQ_EGRTXQ_EGR_SIP*/{
            SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;

NOT_RELEVANT_TO_SIM                GT_U32    EGRRAMSTESTCTRLReg;
NOT_RELEVANT_TO_SIM                GT_U32    EGRRAMSInterruptCause;
NOT_RELEVANT_TO_SIM                GT_U32    EGRRAMSInterruptMask;
NOT_RELEVANT_TO_SIM                GT_U32    EGRRamsParityErrorAddrReg0;
NOT_RELEVANT_TO_SIM                GT_U32    EGRRamsParityErrorAddrReg1;

            }TXQ_EGRTXQ_EGR_SIP;

            struct /*TXQ_EGR0*/{
                SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;

DONE_SIP5_ALIGN                GT_U32    devMapTableAddrConstruct;
DONE_SIP5_ALIGN                GT_U32    localSrcPortMapOwnDevEn[4]/*Port div 32*/;
DONE_SIP5_ALIGN                GT_U32    localTrgPortMapOwnDevEn[2]/*Port div 32*/;

DONE_SIP5_ALIGN                GT_U32    egrFiltersGlobalEnables;
DONE_SIP5_ALIGN                GT_U32    unknownUCFilterEn[2]/*Port div 32*/;
DONE_SIP5_ALIGN                GT_U32    unregedMCFilterEn[2]/*Port div 32*/;
DONE_SIP5_ALIGN                GT_U32    unregedBCFilterEn[2]/*Port div 32*/;
DONE_SIP5_ALIGN                GT_U32    MCLocalEn[2]/*Port div 32*/;
NEED_TO_BE_USED_BY_SIP5                GT_U32    IPMCRoutedFilterEn[2]/*Port div 32*/;
DONE_SIP5_ALIGN                GT_U32    UCSrcIDFilterEn[2]/*Port div 32*/;
DONE_SIP5_ALIGN                GT_U32    OAMLoopbackFilterEn[2]/*Port div 32*/;
NOT_RELEVANT_TO_SIM                GT_U32    enCNFrameTxOnPort[2]/*Port div 32*/;
NOT_RELEVANT_TO_SIM                GT_U32    profileCNMTriggeringEnable[2];
NOT_RELEVANT_TO_SIM                GT_U32    enFCTriggerByCNFrameOnPort[2]/*Port div 32*/;
DONE_SIP5_ALIGN                GT_U32    relayedPortNumber;

DONE_SIP5_ALIGN                GT_U32    cpuPortDist;
DONE_SIP5_ALIGN                GT_U32    egrInterruptsCause;
DONE_SIP5_ALIGN                GT_U32    egrInterruptsMask;

NOT_RELEVANT_TO_SIM                GT_U32    MCFIFOGlobalConfigs;
NOT_RELEVANT_TO_SIM                GT_U32    MCFIFORelayedIFConfigs;
NOT_RELEVANT_TO_SIM                GT_U32    MCFIFOArbiterWeights;
NOT_RELEVANT_TO_SIM                GT_U32    MCFIFO0DistMask[2]/*Port div 32*/;
NOT_RELEVANT_TO_SIM                GT_U32    MCFIFO1DistMask[2]/*Port div 32*/;
NOT_RELEVANT_TO_SIM                GT_U32    MCFIFO2DistMask[2]/*Port div 32*/;
NOT_RELEVANT_TO_SIM                GT_U32    MCFIFO3DistMask[2]/*Port div 32*/;
NOT_RELEVANT_TO_SIM                GT_U32    MCFIFOParityErrorsCntr[4]/*mc fifo*/;

            }TXQ_EGR[2];

            struct /*TXQ_LL*/{
                SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;

                struct /*global*/{

                    GT_U32    LLInterruptCause;
                    GT_U32    LLInterruptMask;

                    GT_U32    portsLinkEnableStatus0;
                    GT_U32    portsLinkEnableStatus1;

                }global;

                GT_U32    linkListECCCtrlConfig;
                GT_U32    linkListRAMECCStatus;
                GT_U32    linkListRAMECCErrorCntr;
                GT_U32    FBUFRAMECCStatus;
                GT_U32    FBUFRAMECCErrorCntr;

            }TXQ_LL;

            struct /*TXQ_Queue*/{
                SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;

                struct /*CongestionNotification*/{

NOT_RELEVANT_TO_SIM                    GT_U32    CNGlobalConfig;
NOT_RELEVANT_TO_SIM                    GT_U32    enFCTriggerByCNFrameOnPort[2]/*Port div 32*/;
NOT_RELEVANT_TO_SIM                    GT_U32    feedbackCalcConfigs;
NOT_RELEVANT_TO_SIM                    GT_U32    feedbackMIN;
NOT_RELEVANT_TO_SIM                    GT_U32    feedbackMAX;
NOT_RELEVANT_TO_SIM                    GT_U32    CNSampleTbl[8]/*qFb 3 msb*/;
NOT_RELEVANT_TO_SIM                    GT_U32    CNBufferFIFOOverrunsCntr;
NOT_RELEVANT_TO_SIM                    GT_U32    CNBufferFIFOParityErrorsCntr;
NOT_RELEVANT_TO_SIM                    GT_U32    CNDropCntr;
NOT_RELEVANT_TO_SIM                    GT_U32    profilePriorityQueueCNThreshold[8][8]/*profile*//*priority*/;
NOT_RELEVANT_TO_SIM                    GT_U32    profilePriorityQueueCNSevereThreshold[8][8]/*profile*//*tc*/;
NOT_RELEVANT_TO_SIM                    GT_U32    profilePriorityQueueCNMildThreshold[8][8]/*profile*//*priority*/;
NOT_RELEVANT_TO_SIM                    GT_U32    profilePriorityQueueCNSampleScaleThreshold[8][8]/*profile*//*priority*/;

DONE_SIP5_ALIGN                    GT_U32    distributorGeneralConfigs;
NOT_RELEVANT_TO_SIM                    GT_U32    stackTCRemap[8]/*TC*/;
NOT_RELEVANT_TO_SIM                    GT_U32    stackRemapEn[4]/*Port div 32*/;
DONE_SIP5_ALIGN                    GT_U32    DPToCFIRemapEn[2]/*Port div 32*/;
DONE_SIP5_ALIGN                    GT_U32    fromCPUForwardRestricted[2]/*Port div 32*/;
DONE_SIP5_ALIGN                    GT_U32    bridgedForwardRestricted[2]/*Port div 32*/;
DONE_SIP5_ALIGN                    GT_U32    routedForwardRestricted[2]/*Port div 32*/;
NOT_RELEVANT_TO_SIM                    GT_U32    profileCNMTriggeringEnable[2]/*Port div 32*/;
NOT_RELEVANT_TO_SIM                    GT_U32    enCNFrameTxOnPort[2]/*Port div 32*/;
                }CongestionNotification;

                struct /*PeripheralAccess*/{

                    GT_U32    CNCModesReg;

                    GT_U32    txQMIBCntrsSetConfig[2]/*Set*/;
                    GT_U32    setOutgoingUcPktCntr[2]/*Set*/;
                    GT_U32    setOutgoingMcPktCntr[2]/*Set*/;
                    GT_U32    setOutgoingBcPktCntr[2]/*Set*/;
                    GT_U32    setBridgeEgrFilteredPktCntr[2]/*Set*/;
                    GT_U32    setTailDroppedPktCntr[2]/*Set*/;
                    GT_U32    setCtrlPktCntr[2]/*Set*/;
                    GT_U32    setEgrForwardingRestrictionDroppedPktsCntr[2]/*Set*/;
                    GT_U32    setMcFIFODroppedPktsCntr[2]/*Set*/;

                    GT_U32    peripheralAccessMisc;
                    GT_U32    portGroupDescToEQCntr[4]/*port group*/;

                    GT_U32    PFCTriggerGlobalConfig;
                    GT_U32    portFCMode[8]/*Port div 32*/;
                    GT_U32    PFCPortGroupCntrsStatus[8]/*port group*/;
                    GT_U32    PFCPortGroupFlowCtrlIndicationFIFOOverrunEntry0[8]/*port group*/;
                    GT_U32    PFCPortGroupFlowCtrlIndicationFIFOOverrunEntry1[8]/*port group*/;
                    GT_U32    PFCPortGroupFlowCtrlIndicationFIFOOverrunCntr[8]/*port group*/;
                    GT_U32    PFCPortGroupEqFIFOOverrunCntr[8]/*port group*/;
                    GT_U32    PFCPortGroupCntrsParityErrorsCntr[8]/*port group*/;
                    GT_U32    PFCPortProfile[16]/*Port div 8*/;
                    GT_U32    LLFCTargetPortRemap[16]/*Port div 4*/;
                    GT_U32    PFCLLFCSourcePortRemap[16]/*Port div 4*/;
                    GT_U32    PFCGroupOfPortsTCCntr[8][8]/*port group*//*tc*/;
                    GT_U32    PFCGroupOfPortsTCThresholds[8][8]/*tc*//*pipe*/;
                    GT_U32    setMcFIFO3_0DroppedPktsCntr[2]/*Set*/;
                    GT_U32    setMcFIFO7_4DroppedPktsCntr[2]/*Set*/;
                    GT_U32    PFCLLFCSourcePortRemapForLLFCCntrs[32]/*Port div 4*/;
                    GT_U32    PFCLLFCSourcePortRemapForLLFCMsgs[32]/*Port div 4*/;

                }PeripheralAccess;

                struct /*PFC*/{

                    GT_U32    PFCInterruptSummaryCause;
                    GT_U32    PFCInterruptSummaryMask;
                    GT_U32    PFCParityInterruptCause;
                    GT_U32    PFCParityInterruptMask;
                    GT_U32    PFCCntrsOverUnderFlowInterruptCause;
                    GT_U32    PFCCntrsOverUnderFlowInterruptMask;
                    GT_U32    PFCPortGroupGlobalOverflowInterruptCause[8]/*port group*/;
                    GT_U32    PFCPortGroupGlobalOverflowInterruptMask[8]/*port group*/;
                    GT_U32    PFCPortGroupGlobalUnderflowInterruptCause[8]/*port group*/;
                    GT_U32    PFCPortGroupGlobalUnderflowInterruptMask[8]/*port group*/;
                    GT_U32    PFCFIFOsOverrunsCause;
                    GT_U32    PFCFIFOsOverrunsMask;
                    GT_U32    PFCMsgsSetsOverrunsCause;
                    GT_U32    PFCMsgsSetsOverrunsMask;
                    GT_U32    PFCMsgsSetsOverrunsEgr3_0Cause0;
                    GT_U32    PFCMsgsSetsOverrunsEgr3_0Mask0;
                    GT_U32    PFCMsgsSetsOverrunsEgr3_0Cause1;
                    GT_U32    PFCMsgsSetsOverrunsEgr3_0Mask1;
                    GT_U32    PFCMsgsSetsOverrunsEgr7_4Cause0;
                    GT_U32    PFCMsgsSetsOverrunsEgr7_4Mask0;
                    GT_U32    PFCMsgsSetsOverrunsEgr7_4Cause1;
                    GT_U32    PFCMsgsSetsOverrunsEgr7_4Mask1;
                }PFC;

                struct /*tailDrop*/{

                    GT_U32    snifferDescsCntr;
                    GT_U32    snifferBuffersCntr;
                    GT_U32    mcDescsCntr;
                    GT_U32    mcBuffersCntr;

                    GT_U32    mirroredPktsToAnalyzerPortDescsLimit;
                    GT_U32    mirroredPktsToAnalyzerPortBuffersLimit;
                    GT_U32    mcDescsLimit;
                    GT_U32    mcBuffersLimit;

                }tailDrop;

                struct{/*ResourceHistogram*/
                    GT_U32    resourceHistogramCntr[4]/*counter*/;

                    GT_U32    resourceHistogramLimitReg1;
                    GT_U32    resourceHistogramLimitReg2;
                }ResourceHistogram;

                struct{/*global*/
                    GT_U32    transmitQueueInterruptSummaryCause;
                    GT_U32    transmitQueueInterruptSummaryMask;
                    GT_U32    generalInterruptCause;
                    GT_U32    generalInterruptMask;
                    GT_U32    portDescFullInterruptCause0;
                    GT_U32    portDescFullInterruptMask0;
                    GT_U32    portDescFullInterruptCause1;
                    GT_U32    portDescFullInterruptMask1;
                    GT_U32    portDescFullInterruptCause2;
                    GT_U32    portDescFullInterruptMask2;
                    GT_U32    portBufferFullInterruptCause0;
                    GT_U32    portBufferFullInterruptMask0;
                    GT_U32    portBufferFullInterruptCause1;
                    GT_U32    portBufferFullInterruptMask1;
                    GT_U32    portBufferFullInterruptCause2;
                    GT_U32    portBufferFullInterruptMask2;
                    GT_U32    congestionNotificationInterruptCause;
                    GT_U32    congestionNotificationInterruptMask;
                }global;

                GT_U32    profileEnableWRTDDP[4]/*DP*/;
                GT_U32    tailDropConfig;
                GT_U32    pktLengthForTailDropEnqueue;
                GT_U32    portEnqueueEnable[64]/*Port*/;
                GT_U32    portTailDropCNProfile[64]/*Port*/;
                GT_U32    profilePriorityQueueEnablePoolUsage[8]/*Priority*/;
                GT_U32    profilePriorityQueueToPoolAssociation[8]/*profile*/;
                GT_U32    WRTDMasks0;
                GT_U32    WRTDMasks1;

                GT_U32    totalDescCntr;
                GT_U32    totalBuffersCntr;
                GT_U32    portDescCntr[64]/*Port*/;
                GT_U32    portBuffersCntr[64]/*Port*/;
                GT_U32    priorityDescCntr[8]/*TC*/;
                GT_U32    priorityBuffersCntr[8]/*TC*/;

                GT_U32    globalDescsLimit;
                GT_U32    globalBufferLimit;
                GT_U32    profilePortLimits[8]/*profile*/;
                GT_U32    poolLimits[8]/*Priority*/;
                GT_U32    ucMcCtrl;
                GT_U32    ucWeights;
                GT_U32    mcWeights;
                GT_U32    mcShaperCtrl;

            }TXQ_Queue;

            struct /*TXQ_DQ*/{
                SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;

                struct /*FlowControlResponse*/{

                    GT_U32    flowCtrlResponseConfig;
                    GT_U32    schedulerProfileLLFCXOFFValue[8]/*profile*/;
                    GT_U32    PFCTimerToPriorityQueueMap[8]/*TC*/;
                    GT_U32    schedulerProfilePFCTimerToPriorityQueueMapEnable[8]/*profile*/;
                    GT_U32    schedulerProfilePriorityQueuePFCFactor[8][8]/*Profile*//*TC*/;

                }FlowControlResponse;

                struct /*Scheduler*/{

                    GT_U32    profileSDWRRWeightsConfigReg0[8]/*Profile*/;
                    GT_U32    profileSDWRRWeightsConfigReg1[8]/*Profile*/;
                    GT_U32    profileSDWRRGroup[8]/*Profile*/;
                    GT_U32    profileSDWRREnable[8]/*Profile*/;
                    GT_U32    portCPUSchedulerProfile;
                    GT_U32    portSchedulerProfile[16]/*Port*/;

                    GT_U32    schedulerConfig;
                    GT_U32    portRequestMask;
                    GT_U32    pktLengthForSchedulingAndTailDropDequeue;
                    GT_U32    portRequestMaskSelector[16]/*Port*/;

                    GT_U32    tokenBucketUpdateRate;
                    GT_U32    tokenBucketBaseLine;
                    GT_U32    CPUTokenBucketMTU;
                    GT_U32    portsTokenBucketMTU;
                    GT_U32    tokenBucketMode;
                    GT_U32    portDequeueEnable[64/*was 16 from Lion2*/]/*Port*/;

                    GT_U32    STCStatisticalTxSniffConfig;
                    GT_U32    egrCTMTrigger;
                    GT_U32    egrAnalyzerEnable;

                    GT_U32    pizzaArbConfig;
                    GT_U32    pizzaArbMap[42]/*Slice_Group*/;
                    GT_U32    pizzaArbDebug;
                    GT_U32    pizzaArbDebugStatus;
                    GT_U32    egrCNEn[2];
                    GT_U32    egrCNFCEn[2];

                }Scheduler;


                GT_U32    portTxQFlushTrigger[16]/*Port*/;

                GT_U32    globalDequeueConfig;

                GT_U32    txQDQInterruptSummaryCause;
                GT_U32    txQDQInterruptSummaryMask;
                GT_U32    flushDoneInterruptCause;
                GT_U32    flushDoneInterruptMask;
                GT_U32    txQDQMemoryErrorInterruptCause;
                GT_U32    txQDQMemoryErrorInterruptMask;
                GT_U32    egrSTCInterruptCause;
                GT_U32    egrSTCInterruptMask;
                GT_U32    txQDQGeneralInterruptCause;
                GT_U32    txQDQGeneralInterruptMask;

                GT_U32    tokenBucketPriorityParityErrorCntr;
                GT_U32    stateVariablesParityErrorCntr;
                GT_U32    parityErrorBadAddr;
                GT_U32    dqMetalFixRegister;

            }TXQ_DQ;

            struct /*TXQ_SHT*/{
                SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;

NOT_RELEVANT_TO_SIM                GT_U32    DFXReg0;
NOT_RELEVANT_TO_SIM                GT_U32    DFXReg1;
NOT_RELEVANT_TO_SIM                GT_U32    DFXReg2;
NOT_RELEVANT_TO_SIM                GT_U32    DFXReg3;
NOT_RELEVANT_TO_SIM                GT_U32    DFXStatusReg0;
NOT_RELEVANT_TO_SIM                GT_U32    DFXStatusReg1;
NOT_RELEVANT_TO_SIM                GT_U32    SHTRAMSInterruptCause;
NOT_RELEVANT_TO_SIM                GT_U32    SHTRAMSInterruptMask;
NOT_RELEVANT_TO_SIM                GT_U32    SHTRamsParityErrorAddrReg0;
NOT_RELEVANT_TO_SIM                GT_U32    SHTRamsParityErrorAddrReg1;

                struct{ /*global*/
DONE_SIP5_ALIGN                    GT_U32    SHTGlobalConfigs;
NOT_RELEVANT_TO_SIM                    GT_U32    VLANParityErrorLastEntries;
NOT_RELEVANT_TO_SIM                    GT_U32    egrVLANParityErrorCntr;
NOT_RELEVANT_TO_SIM                    GT_U32    ingrVLANParityErrorCntr;
NOT_RELEVANT_TO_SIM                    GT_U32    portIsolationParityErrorLastEntries;
NOT_RELEVANT_TO_SIM                    GT_U32    VIDXParityErrorLastEntry;
NOT_RELEVANT_TO_SIM                    GT_U32    L2PortIsolationParityErrorCntr;
NOT_RELEVANT_TO_SIM                    GT_U32    L3PortIsolationParityErrorCntr;
NOT_RELEVANT_TO_SIM                    GT_U32    VIDXParityErrorCntr;
NOT_RELEVANT_TO_SIM                    GT_U32    SHTInterruptsSumCause;
NOT_RELEVANT_TO_SIM                    GT_U32    SHTInterruptsSumMask;
NOT_RELEVANT_TO_SIM                    GT_U32    SHTInterruptsCause;
NOT_RELEVANT_TO_SIM                    GT_U32    SHTInterruptsMask;
NOT_RELEVANT_TO_SIM                    GT_U32    SHTRAMSTESTCTRLReg;
NOT_RELEVANT_TO_SIM                    GT_U32    SHTDebugBusMonitorReg;
NOT_RELEVANT_TO_SIM                    GT_U32    redundancyRepairDoneReg;
                }global;
            }TXQ_SHT;

            struct /*TXQ_DIST*/{
                SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;

DONE_SIP5_ALIGN                GT_U32    distDevMapTableAddrConstruct;
DONE_SIP5_ALIGN                GT_U32    distLocalSrcPortMapOwnDevEn[4]/*Port div 32*/;
DONE_SIP5_ALIGN                GT_U32    distLocalTrgPortMapOwnDevEn[4]/*Port div 32*/;
DONE_SIP5_ALIGN                GT_U32    distEntryDeviceMapTable[64]/*entry*/;

NOT_RELEVANT_TO_SIM                GT_U32    distGlobalCtrl;
NOT_RELEVANT_TO_SIM                GT_U32    distUCMCArbiterCtrl;
NOT_RELEVANT_TO_SIM                GT_U32    distInterruptCause;
NOT_RELEVANT_TO_SIM                GT_U32    distInterruptMask;
NOT_RELEVANT_TO_SIM                GT_U32    distBurstFifoThresholds;
NOT_RELEVANT_TO_SIM                GT_U32    distLossyDropEventCntr;
NOT_RELEVANT_TO_SIM                GT_U32    distPFCXoffEventCntr;
NOT_RELEVANT_TO_SIM                GT_U32    distBurstFifoFillLevelStatus[16]/*fifo*/;

            }TXQ_DIST;

        }TXQ /*- TXQ*/ ;
    }egrAndTxqVer1 /*- Egress and Txq*/ ;

    struct /*L2I - L2I */{
        SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;

        struct /*bridgeAccessMatrix*/{

DONE_SIP5_ALIGN            GT_U32    bridgeAccessMatrixLine[8]/*SA Access Level*/;

        }bridgeAccessMatrix;

        struct /*bridgeDropCntrAndSecurityBreachDropCntrs*/{

DONE_SIP5_ALIGN            GT_U32    globalSecurityBreachFilterCntr;
DONE_SIP5_ALIGN            GT_U32    portVLANSecurityBreachDropCntr;
DONE_SIP5_ALIGN            GT_U32    bridgeFilterCntr;

        }bridgeDropCntrAndSecurityBreachDropCntrs;

        struct /*bridgeEngineConfig*/{

DONE_SIP5_ALIGN            GT_U32    bridgeGlobalConfig[3];
DONE_SIP5_ALIGN            GT_U32    sourceIDAssignmentModeConfigPorts;

        }bridgeEngineConfig;

        struct /*bridgeEngineInterrupts*/{

DONE_SIP5_ALIGN            GT_U32    bridgeInterruptCause;
DONE_SIP5_ALIGN            GT_U32    bridgeInterruptMask;

        }bridgeEngineInterrupts;

        struct /*ctrlTrafficToCPUConfig*/{

DONE_SIP5_ALIGN            GT_U32    IEEEReservedMcConfig0[16]/*Register*/;   /*code use ieeeRsrvMcConfTable , tableSet 0 */
DONE_SIP5_ALIGN            GT_U32    IEEEReservedMcCPUIndex[16]/*Register*/;  /*code uses ieeeRsrvMcCpuIndex */
DONE_SIP5_ALIGN            GT_U32    IEEEReservedMcConfig1[16]/*Register*/;   /*code use ieeeRsrvMcConfTable , tableSet 1 */
DONE_SIP5_ALIGN            GT_U32    IEEETableSelect0;/*code use ieeeTblSelect */
DONE_SIP5_ALIGN            GT_U32    IEEEReservedMcConfig2[16]/*Register*/;   /*code use ieeeRsrvMcConfTable , tableSet 2 */
DONE_SIP5_ALIGN            GT_U32    IEEEReservedMcConfig3[16]/*Register*/;   /*code use ieeeRsrvMcConfTable , tableSet 3 */
DONE_SIP5_ALIGN            GT_U32    IEEETableSelect1;/*code use ieeeTblSelect */
DONE_SIP5_ALIGN            GT_U32    IPv6ICMPMsgTypeConfig[2]/*ICMP_Msg_Type*/;
DONE_SIP5_ALIGN            GT_U32    IPv6ICMPCommand;
DONE_SIP5_ALIGN            GT_U32    IPv4McLinkLocalConfig[8]/*Register*/;
DONE_SIP5_ALIGN            GT_U32    IPv6McLinkLocalConfig[8]/*Register*/;
DONE_SIP5_ALIGN            GT_U32    IPv4McLinkLocalCPUCodeIndex[8][2]/*Register*//*Register_t*/;
DONE_SIP5_ALIGN            GT_U32    IPv6McLinkLocalCPUCodeIndex[8][2]/*CPU CodeI ndex_n*//*CPU Code Index_t*/;

        }ctrlTrafficToCPUConfig;

        struct /*ingrLog*/{

DONE_SIP5_ALIGN            GT_U32    ingrLogEtherType;
DONE_SIP5_ALIGN            GT_U32    ingrLogDALow;
DONE_SIP5_ALIGN            GT_U32    ingrLogDAHigh;
DONE_SIP5_ALIGN            GT_U32    ingrLogDALowMask;
DONE_SIP5_ALIGN            GT_U32    ingrLogDAHighMask;
DONE_SIP5_ALIGN            GT_U32    ingrLogSALow;
DONE_SIP5_ALIGN            GT_U32    ingrLogSAHigh;
DONE_SIP5_ALIGN            GT_U32    ingrLogSALowMask;
DONE_SIP5_ALIGN            GT_U32    ingrLogSAHighMask;
DONE_SIP5_ALIGN            GT_U32    ingrLogMacDAResult;
DONE_SIP5_ALIGN            GT_U32    ingrLogMacSAResult;

        }ingrLog;

        struct /*ingrPortsBridgeConfig*/{

DONE_SIP5_ALIGN            GT_U32    ingrPortBridgeConfig0[29]/*Port*/;
DONE_SIP5_ALIGN            GT_U32    ingrPortBridgeConfig1[29]/*Port*/;

        }ingrPortsBridgeConfig;

        struct /*ingrPortsRateLimitConfig*/{

DONE_SIP5_ALIGN            GT_U32    portRateLimitCntr[28]/*port*/;
DONE_SIP5_ALIGN            GT_U32    ingrRateLimitConfig0;
DONE_SIP5_ALIGN            GT_U32    ingrRateLimitConfig1;
DONE_SIP5_ALIGN            GT_U32    ingrRateLimitDropCntr310;
DONE_SIP5_ALIGN            GT_U32    ingrRateLimitDropCntr3932;
DONE_SIP5_ALIGN            GT_U32    MIISpeedGranularity;
DONE_SIP5_ALIGN            GT_U32    GMIISpeedGranularity;
DONE_SIP5_ALIGN            GT_U32    stackSpeedGranularity;

        }ingrPortsRateLimitConfig;

        struct /*IPv6McBridgingBytesSelection*/{

DONE_SIP5_ALIGN            GT_U32    IPv6MCBridgingBytesSelectionConfig;

        }IPv6McBridgingBytesSelection;

        struct /*layer2BridgeMIBCntrs*/{

DONE_SIP5_ALIGN            GT_U32    MACAddrCount[3];
DONE_SIP5_ALIGN            GT_U32    hostIncomingPktsCount;
DONE_SIP5_ALIGN            GT_U32    hostOutgoingPktsCount;
DONE_SIP5_ALIGN            GT_U32    hostOutgoingMcPktCount;
DONE_SIP5_ALIGN            GT_U32    hostOutgoingBcPktCount;
DONE_SIP5_ALIGN            GT_U32    matrixSourceDestinationPktCount;
DONE_SIP5_ALIGN            GT_U32    cntrsSetConfig[2]/*Set*/;
DONE_SIP5_ALIGN            GT_U32    setIncomingPktCount[2]/*SetNum*/;
DONE_SIP5_ALIGN            GT_U32    setVLANIngrFilteredPktCount[2]/*SetNum*/;
DONE_SIP5_ALIGN            GT_U32    setSecurityFilteredPktCount[2]/*SetNum*/;
DONE_SIP5_ALIGN            GT_U32    setBridgeFilteredPktCount[2]/*SetNum*/;

        }layer2BridgeMIBCntrs;

        struct /*MACBasedQoSTable*/{

DONE_SIP5_ALIGN            GT_U32    MACQoSTableEntry[7]/*QoS Profile*/;

        }MACBasedQoSTable;

        struct /*securityBreachStatus*/{

DONE_SIP5_ALIGN            GT_U32    securityBreachStatus[3];

        }securityBreachStatus;

        struct /*UDPBcMirrorTrapUDPRelayConfig*/{

DONE_SIP5_ALIGN            GT_U32    UDPBcDestinationPortConfigTable[12]/*Port*/;

        }UDPBcMirrorTrapUDPRelayConfig;

        struct /*VLANMRUProfilesConfig*/{

DONE_SIP5_ALIGN            GT_U32    VLANMRUProfileConfig[4]/*Register*/;

        }VLANMRUProfilesConfig;

        struct /*VLANRangesConfig*/{

DONE_SIP5_ALIGN            GT_U32    ingrVLANRangeConfig;

        }VLANRangesConfig;


    }L2I /*- L2I*/ ;
    struct /*ucRouter - Unicast Router */{
        SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;

        GT_U32    ucRoutingEngineConfigReg;

    }ucRouter /*- Unicast Router*/ ;
    struct /*IPvX - IPvX */{
        SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;

        struct /*IPLog*/{

            GT_U32    IPHitLogDIPAddrReg[4]/*Word*/;
            GT_U32    IPHitLogDIPMaskAddrReg[4]/*Word*/;
            GT_U32    IPHitLogSIPAddrReg[4]/*Word*/;
            GT_U32    IPHitLogSIPMaskAddrReg[4]/*Word*/;
            GT_U32    IPHitLogLayer4DestinationPortReg;
            GT_U32    IPHitLogLayer4SourcePortReg;
            GT_U32    IPHitLogProtocolReg;
            GT_U32    IPHitLogDestinationIPNHEEntryRegWord[3]/*Word*/;
            GT_U32    IPHitLogSourceIPNHEEntryRegWord[3]/*Word*/;

        }IPLog;

        struct /*IPv4GlobalCtrl*/{

DONE_SIP5_ALIGN            GT_U32    IPv4CtrlReg0;
DONE_SIP5_ALIGN            GT_U32    IPv4CtrlReg1;
SHOULD_BE_IGNORED_BY_SIP5            GT_U32    IPv4UcEnableCtrl;
SHOULD_BE_IGNORED_BY_SIP5            GT_U32    IPv4McEnableCtrl;

        }IPv4GlobalCtrl;

        struct /*IPv6GlobalCtrl*/{

DONE_SIP5_ALIGN            GT_U32    IPv6CtrlReg0;
DONE_SIP5_ALIGN            GT_U32    IPv6CtrlReg1;
SHOULD_BE_IGNORED_BY_SIP5            GT_U32    IPv6UcEnableCtrl;
SHOULD_BE_IGNORED_BY_SIP5            GT_U32    IPv6McEnableCtrl;

        }IPv6GlobalCtrl;

        struct /*IPv6Scope*/{

DONE_SIP5_ALIGN            GT_U32    IPv6UcScopePrefix[4]/*Scope Prefix*/;
DONE_SIP5_ALIGN            GT_U32    IPv6UcScopeLevel[4]/*Scope Level*/;
DONE_SIP5_ALIGN            GT_U32    IPv6UcScopeTableReg[4]/*RegNum*/;
DONE_SIP5_ALIGN            GT_U32    IPv6McScopeTableReg[4]/*RegNum*/;

        }IPv6Scope;

        struct /*routerGlobalCtrl*/{

DONE_SIP5_ALIGN            GT_U32    routerGlobalCtrl0;
/* not used at all */      GT_U32    routerMTUConfigReg[4]/*MTU Index*/;
            GT_U32    routerPerPortSIPSAEnable0;
            GT_U32    routerAdditionalCtrl;

        }routerGlobalCtrl;

        struct /*qosRoutingConfig*/{

            GT_U32    qoSProfileToRouteBlockOffsetEntry[16]/*number*/;

        }qosRoutingConfig;

        struct /*routerAccessMatrix*/{

            GT_U32    routerAccessMatrixLine[8]/*SIP Access Level*/;

        }routerAccessMatrix;

        struct /*routerBridgedPktsExceptionCntr*/{

            GT_U32    routerBridgedPktExceptionsCntr;

        }routerBridgedPktsExceptionCntr;

        struct /*routerDropCntr*/{

            GT_U32    routerDropCntr;
            GT_U32    routerDropCntrConfig;

        }routerDropCntr;

        struct /*routerManagementCntrsSets*/{

            GT_U32    routerManagementInUcPktsCntrSet[4]/*set number*/;
            GT_U32    routerManagementInMcPktsCntrSet[4]/*set number*/;
            GT_U32    routerManagementInUcNonRoutedNonException[4]/*set number*/;
            GT_U32    routerManagementInUcNonRoutedExceptionPktsCntrSet[4]/*set number*/;
            GT_U32    routerManagementInMcNonRoutedNonExceptionPktsCntrSet[4]/*set number*/;
            GT_U32    routerManagementInMcNonRoutedExceptionPktsCntrSet[4]/*set nmber*/;
            GT_U32    routerManagementInUcTrappedMirroredPktCntrSet[4]/*set number*/;
            GT_U32    routerManagementInMcTrappedMirroredPktCntrSet[4]/*set number*/;
            GT_U32    routerManagementInMcRPFFailCntrSet[4]/*set number*/;
            GT_U32    routerManagementOutUcPktCntrSet[4]/*set number*/;
            GT_U32    routerManagementInUcRPFFailCntr;
            GT_U32    routerManagementInSipSAMismatchCntr;
            GT_U32    routerManagementIncomingPktCntr;
            GT_U32    routerManagementOutgoingPktCntr;
            GT_U32    routerManagementInSipFilterCntr;
            GT_U32    routerManagementCntrsSetConfig[4]/*set number*/;

        }routerManagementCntrsSets;

    }IPvX /*- IPvX*/ ;
    struct /*BM - BM */{
        SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;

        struct /*BMRegs*/{

            GT_U32    bufferManagementInterruptCause0;
            GT_U32    bufferManagementInterruptMask0;
            GT_U32    bufferManagementInterruptCause1;
            GT_U32    bufferManagementInterruptMask1;

            GT_U32    bufferManagementNumberOfAgedBuffers;
            GT_U32    totalBuffersAllocationCntr;
            GT_U32    bufferManagementPktCntr;
            GT_U32    portsGroupAllocationCntr;
            GT_U32    networkPortsGroupAllocationCntr;
            GT_U32    hyperGStackPortsGroupAllocationCntr;
            GT_U32    portBuffersAllocationCntr[29]/*Port*/;

            GT_U32    bufferManagementAgingConfig;

            GT_U32    bufferManagementGlobalBuffersLimitsConfig;
            GT_U32    bufferManagementPortGroupLimitsConfig;
            GT_U32    bufferManagementNetworkPortGroupLimitsConfig;
            GT_U32    bufferManagementHyperGStackPortsGroupLimits;
            GT_U32    bufferManagementBuffersConfig;

            GT_U32    portsBuffersLimitProfileConfig[4];
            GT_U32    buffersLimitsProfileAssociation0;
            GT_U32    buffersLimitsProfileAssociation1;

            GT_U32    burstConfig;
            GT_U32    generalConfigs;
            GT_U32    sliceEnable;
            GT_U32    slicePortMap07;
            GT_U32    slicePortMap815;
            GT_U32    slicePortMap1623;
            GT_U32    slicePortMap2431;
        }BMRegs;


    }BM /*- BM*/ ;
    /* LMS used by xcat2 : with 2 LMS sub units LMS0,1 */
    struct /*LMS - LMS */{
        SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;

        struct /*LMS1LMS1Group0*/{

            GT_U32    LEDInterface1CtrlReg1AndClass6ManipulationforPorts12Through23;
            GT_U32    LEDInterface1Group23ConfigRegforPorts12Through23;
            GT_U32    LEDInterface1Class23ManipulationRegforPorts12Through23;
            GT_U32    LEDInterface1Class5ManipulationRegforPorts12Through23;
            GT_U32    stackLEDInterface1Class1011ManipulationRegforStackPort;
            GT_U32    stackLEDInterface1Group01ConfigRegforStackPort;

            GT_U32    MIBCntrsCtrlReg0forPorts18Through23;

            GT_U32    portsGOP3AndStackMIBsInterruptCause;
            GT_U32    portsGOP3AndStackMIBsInterruptMask;

            GT_U32    flowCtrlAdvertiseForFiberMediaSelectedConfigReg1forPorts12Through23;
            GT_U32    PHYAddrReg3forPorts18Through23;
            GT_U32    PHYAutoNegConfigReg3;

        }LMS1LMS1Group0;

        struct /*LMS1*/{

            GT_U32    LEDInterface1CtrlReg0forPorts12Through23AndStackPort;
            GT_U32    LEDInterface1Group01ConfigRegforPorts12Through23;
            GT_U32    LEDInterface1Class01ManipulationRegforPorts12Through23;
            GT_U32    LEDInterface1Class4ManipulationRegforPorts12Through23;
            GT_U32    stackLEDInterface1Class04ManipulationRegforStackPorts;
            GT_U32    stackLEDInterface1Class59ManipulationRegforStackPorts;
            GT_U32    LEDInterface1FlexlinkPortsDebugSelectRegforStackPort;
            GT_U32    LEDInterface1FlexlinkPortsDebugSelectReg1forStackPort;

            GT_U32    portsInterruptSummary;

            GT_U32    MIBCntrsCtrlReg0forPorts12Through17;

            GT_U32    portsGOP2MIBsInterruptCause;
            GT_U32    portsGOP2MIBsInterruptMask;

            GT_U32    PHYAddrReg2forPorts12Through17;
            GT_U32    PHYAutoNegConfigReg2;
            GT_U32    SMI1Management;
            GT_U32    LMS1MiscConfigs;

        }LMS1;

        struct /*LMS0LMS0Group0*/{

            GT_U32    LEDInterface0CtrlReg1AndClass6ManipulationRegforPorts0Through11;
            GT_U32    LEDInterface0Group23ConfigRegforPorts0Through11;
            GT_U32    LEDInterface0Class23ManipulationRegforPorts0Through11;
            GT_U32    LEDInterface0Class5ManipulationRegforPorts0Through11;
            GT_U32    stackLEDInterface0Class1011ManipulationRegforStackPorts;
            GT_U32    stackLEDInterface0Group01ConfigRegforStackPort;

            GT_U32    portsMIBCntrsInterruptSummaryMask;
            GT_U32    portsInterruptSummaryMask;

            GT_U32    MIBCntrsCtrlReg0forPorts6Through11;

            GT_U32    triSpeedPortsGOP1MIBsInterruptCause;
            GT_U32    triSpeedPortsGOP1MIBsInterruptMask;

            GT_U32    flowCtrlAdvertiseForFiberMediaSelectedConfigReg0forPorts0Through11;
            GT_U32    PHYAddrReg1forPorts6Through11;
            GT_U32    PHYAutoNegConfigReg1;

        }LMS0LMS0Group0;

        struct /*LMS0*/{

            GT_U32    LEDInterface0CtrlReg0forPorts0Through11AndStackPort;
            GT_U32    LEDInterface0Group01ConfigRegforPorts0Through11;
            GT_U32    LEDInterface0Class01ManipulationRegforPorts0Through11;
            GT_U32    LEDInterface0Class4ManipulationRegforPorts0Through11;
            GT_U32    stackLEDInterface0Class04ManipulationRegforStackPorts;
            GT_U32    stackLEDInterface0Class59ManipulationRegforStackPorts;
            GT_U32    LEDInterface0FlexlinkPortsDebugSelectRegforStackPort;
            GT_U32    LEDInterface0FlexlinkPortsDebugSelectReg1forStackPort;

            GT_U32    MIBCntrsInterruptSummary;

            GT_U32    MIBCntrsCtrlReg0forPorts0Through5;

            GT_U32    sourceAddrMiddle;
            GT_U32    sourceAddrHigh;

            GT_U32    portsGOP0MIBsInterruptCause;
            GT_U32    portsGOP0MIBsInterruptMask;

            GT_U32    PHYAddrReg0forPorts0Through5;
            GT_U32    PHYAutoNegConfigReg0;
            GT_U32    SMI0Management;
            GT_U32    LMS0MiscConfigs;

        }LMS0;

        GT_U32    stackPortsModeReg;

    }LMS /*- LMS*/ ;

    /* LMS1 used by Lion2 : -- not the same as LMS.LMS1 */
    struct /*LMS1 - LMS1 */{
        SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;

        struct /*configRegs*/{

            GT_U32    periodicFlowCtrlInterval1;
            GT_U32    periodicFlowCtrlIntervalSelection;

        }configRegs;

        struct /*internalRegs*/{

            GT_U32    LSInterruptCause;
            GT_U32    LSInterruptMask;

        }internalRegs;

        struct /*LEDInterfaceConfig*/{

            GT_U32    LEDInterfaceCtrlReg0;
            GT_U32    LEDInterfaceGroup01ConfigReg;
            GT_U32    LEDInterfaceClass01ManipulationReg;
            GT_U32    LEDInterfaceClass4ManipulationReg;
            GT_U32    XLGLEDInterfaceClass59ManipulationReg;
            GT_U32    XLGLEDInterfaceDebugSelectReg;
            GT_U32    LEDInterface0CtrlReg1AndClass6ManipulationRegforPorts0Through11;
            GT_U32    LEDInterface0Group23ConfigRegforPorts0Through11;
            GT_U32    LEDInterface0Class23ManipulationRegforPorts0Through11;
            GT_U32    LEDInterface0Class5ManipulationRegforPorts0Through11;
            GT_U32    XGLEDInterface0Class1011ManipulationRegforXGPorts;
            GT_U32    XGLEDInterface0Group01ConfigRegforXGPort;

        }LEDInterfaceConfig;

        struct /*miscellaneousRegs*/{

            GT_U32    periodicFlowCtrlInterval0;

        }miscellaneousRegs;

        struct /*portsMACSourceAddr*/{

            GT_U32    sourceAddrMiddle;
            GT_U32    sourceAddrHigh;

        }portsMACSourceAddr;

        struct /*SMIConfig*/{

            GT_U32    PHYAddrReg0forPorts0Through5;
            GT_U32    SMI0Management;
            GT_U32    LMS0MiscConfigs;
            GT_U32    PHYAddrReg1forPorts6Through11;

        }SMIConfig;


    }LMS1 /*- LMS*/ ;

    struct /*FDB - FDB */{
        SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;

        struct /*deviceTable*/{

DONE_SIP5_ALIGN            GT_U32    deviceTable;

        }deviceTable;

        struct /*FDBAction*/{

DONE_SIP5_ALIGN            GT_U32    FDBAction[3];

        }FDBAction;

        struct /*FDBAddrUpdateMsgs*/{

DONE_SIP5_ALIGN            GT_U32    AUMsgToCPU;
DONE_SIP5_ALIGN            GT_U32    AUFIFOToCPUConfig;
DONE_SIP5_ALIGN            GT_U32    AUMsgFromCPUWord[4]/*word*/;
DONE_SIP5_ALIGN            GT_U32    AUMsgFromCPUCtrl;

        }FDBAddrUpdateMsgs;

        struct /*FDBDirectAccess*/{

DONE_SIP5_ALIGN            GT_U32    FDBDirectAccessData[4]/*word*/;
DONE_SIP5_ALIGN            GT_U32    FDBDirectAccessCtrl;

        }FDBDirectAccess;

        struct /*FDBGlobalConfig*/{

DONE_SIP5_ALIGN            GT_U32    FDBGlobalConfig;
DONE_SIP5_ALIGN            GT_U32    bridgeAccessLevelConfig;

        }FDBGlobalConfig;

        struct /*FDBInternal*/{

DONE_SIP5_ALIGN            GT_U32    metalFix;
DONE_SIP5_ALIGN            GT_U32    FDBIdleState;

        }FDBInternal;

        struct /*FDBInterrupt*/{

DONE_SIP5_ALIGN            GT_U32    FDBInterruptCauseReg;
DONE_SIP5_ALIGN            GT_U32    FDBInterruptMaskReg;

        }FDBInterrupt;

        struct /*sourceAddrNotLearntCntr*/{

DONE_SIP5_ALIGN            GT_U32    learnedEntryDiscardsCount;

        }sourceAddrNotLearntCntr;


    }FDB /*- FDB*/ ;

    SMEM_CHT_PP_REGS_MPPM_UNIT_STC MPPM0;
    SMEM_CHT_PP_REGS_MPPM_UNIT_STC MPPM1;

    SMEM_CHT_PP_REGS_CTU_UNIT_STC CTU0;
    SMEM_CHT_PP_REGS_CTU_UNIT_STC CTU1;

    struct /*EPCL - EPCL */{
        SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;

DONE_SIP5_ALIGN        GT_U32    egrPolicyGlobalConfig;
DONE_SIP5_ALIGN        GT_U32    egrPolicyConfigTableAccessModeConfig[4]/*virtual_port*/;
DONE_SIP5_ALIGN        GT_U32    egrPolicyNonTSDataPktsConfig[4]/*virtual_port*/;
DONE_SIP5_ALIGN        GT_U32    egrPolicyTSDataPktsConfig[4]/*virtual_port*/;
DONE_SIP5_ALIGN        GT_U32    egrPolicyToCpuConfig[4]/*port*/;
DONE_SIP5_ALIGN        GT_U32    egrPolicyFromCpuDataConfig[4]/*port*/;
DONE_SIP5_ALIGN        GT_U32    egrPolicyFromCpuCtrlConfig[4]/*port*/;
DONE_SIP5_ALIGN        GT_U32    egrPolicyToAnalyzerConfig[4]/*port*/;
NEED_TO_BE_USED_BY_SIP5        GT_U32    egrPolicyDIPSolicitationData[4];
NEED_TO_BE_USED_BY_SIP5        GT_U32    egrPolicyDIPSolicitationMask[4]/*Word*/;
DONE_SIP5_ALIGN        GT_U32    egrTCPPortRangeComparatorConfig[8]/*TCPCmp*/;
DONE_SIP5_ALIGN        GT_U32    egrUDPPortRangeComparatorConfig[8]/*UDPCmp*/;
        GT_U32    egrPolicerGlobalConfig;
        GT_U32    egrPolicerPortBasedMetering[2]/*port*/;

    }EPCL /*- EPCL*/ ;
    struct /*HA - HA */{
        SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;

        SMEM_CHT_PP_REGS_GTS_UNIT_STC ETS;

SHOULD_BE_IGNORED_BY_SIP5        GT_U32    cascadingAndHeaderInsertionConfig[2]/*port*/;
SHOULD_BE_IGNORED_BY_SIP5        GT_U32    egrDSATagTypeConfig[2]/*port*/;
NOT_RELEVANT_TO_SIM        GT_U32    DP2CFITable;
SHOULD_BE_IGNORED_BY_SIP5        GT_U32    portTrunkNumConfigReg[32]/*Port div 4*/;
SHOULD_BE_IGNORED_BY_SIP5        GT_U32    mirrorToAnalyzerHeaderConfig[2]/*port*/;
DONE_SIP5_ALIGN        GT_U32    haGlobalConfig;
SHOULD_BE_IGNORED_BY_SIP5        GT_U32    routerHaEnableMACSAModification[2]/*port*/;
DONE_SIP5_ALIGN        GT_U32    routerMACSABaseReg0;
DONE_SIP5_ALIGN        GT_U32    routerMACSABaseReg1;
DONE_SIP5_ALIGN        GT_U32    deviceIDModificationEnable[2]/*port*/;
SHOULD_BE_IGNORED_BY_SIP5        GT_U32    routerHaMACSAModificationMode[4]/*Port*/;
SHOULD_BE_IGNORED_BY_SIP5        GT_U32    haVLANTranslationEnable[4]/*port*/;
SHOULD_BE_IGNORED_BY_SIP5        GT_U32    coreRingConfig[2]/*port*/;
SHOULD_BE_IGNORED_BY_SIP5        GT_U32    replaceVIDWithUserID[2]/*port*/;
SHOULD_BE_IGNORED_BY_SIP5        GT_U32    qoSProfileToEXPReg[16]/*QoS Profile*/;
SHOULD_BE_IGNORED_BY_SIP5        GT_U32    qoSProfileToDPReg[8]/*QoS Profile*/;
NOT_RELEVANT_TO_SIM        GT_U32    HAInterruptCause;
NOT_RELEVANT_TO_SIM        GT_U32    HAInterruptMask;
NEED_TO_BE_USED_BY_SIP5        GT_U32    hepInterruptSummaryCause;
NEED_TO_BE_USED_BY_SIP5        GT_U32    hepInterruptSummaryMask;
SHOULD_BE_IGNORED_BY_SIP5        GT_U32    toAnalyzerVLANTagAddEnableConfig[2]/*port*/;
DONE_SIP5_ALIGN        GT_U32    ingrAnalyzerVLANTagConfig;
DONE_SIP5_ALIGN        GT_U32    egrAnalyzerVLANTagConfig;
NOT_RELEVANT_TO_SIM        GT_U32    congestionNotificationConfig;
NOT_RELEVANT_TO_SIM        GT_U32    CNMHeaderConfig;
NOT_RELEVANT_TO_SIM        GT_U32    QCNCNtagFlowID;
NOT_RELEVANT_TO_SIM        GT_U32    CPIDReg0;
DONE_SIP5_ALIGN        GT_U32    CPIDReg1;
SHOULD_BE_IGNORED_BY_SIP5        GT_U32    keepVLAN1[16]/*port div4*/;
DONE_SIP5_ALIGN        GT_U32    TPIDConfigReg[4]/*entry*/;
SHOULD_BE_IGNORED_BY_SIP5        GT_U32    portTag0TPIDSelect[8]/*port*/;
SHOULD_BE_IGNORED_BY_SIP5        GT_U32    portTag1TPIDSelect[8]/*port*/;
SHOULD_BE_IGNORED_BY_SIP5        GT_U32    BpeInternalConfig;
SHOULD_BE_IGNORED_BY_SIP5        GT_U32    PerPortBpeEnable;
DONE_SIP5_ALIGN        GT_U32    MPLSEthertype;
DONE_SIP5_ALIGN        GT_U32    IEthertype;
DONE_SIP5_ALIGN        GT_U32    ethernetOverGREProtocolType;

SHOULD_BE_IGNORED_BY_SIP5        GT_U32    tsTPIDConfigReg[4]/*entry*/;
SHOULD_BE_IGNORED_BY_SIP5        GT_U32    portTsTagTpidSelect[8]/*port*/;
DONE_SIP5_ALIGN        GT_U32    mplsControlWord[4];/*word*/

SHOULD_BE_IGNORED_BY_SIP5        GT_U32    passengerTpidIndexSource;
SHOULD_BE_IGNORED_BY_SIP5        GT_U32    dsaPortisFanoutDevice;
SHOULD_BE_IGNORED_BY_SIP5        GT_U32    from_cpuConstantBits;
SHOULD_BE_IGNORED_BY_SIP5        GT_U32    passengerTpid0Tpid1[4];/*TPID index 0..3*/

SHOULD_BE_IGNORED_BY_SIP5        GT_U32    dsaErspan[8]; /* entry index 0..7 */

DONE_SIP5_ALIGN        GT_U32    tunnelStartFragmentIDConfig;

    }HA /*- HA*/ ;
    struct /*RXDMA - RXDMA */{
        SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;

        struct /*arbiterConfig*/{

            GT_U32    WRDMAPizzaArbiterCtrlReg;
            GT_U32    WRDMAPizzaArbiterSliceMapReg[8]/*reg number*/;
            GT_U32    WRDMAPizzaArbiterDebugReg;
            GT_U32    CTUPizzaArbiterCtrlReg;
            GT_U32    CTUPizzaArbiterSliceMapReg[8]/*reg number*/;
            GT_U32    CTUPizzaArbiterDebugReg;

        }arbiterConfig;

        struct /*cutThrough*/{

            GT_U32    CTFeatureEnablePerPort;
            GT_U32    CTEtherTypeIdentification;
            GT_U32    CTUPEnable;
            GT_U32    CTPktIndentification;
            GT_U32    CTUParameters;
            GT_U32    cascadingPort;

        }cutThrough;

        struct /*debug*/{

            GT_U32    lastDescFromGetByBAWRDMA01;

        }debug;

        struct /*DFX*/{

            GT_U32    dataErrorInterruptCause;
            GT_U32    dataErrorInterruptMask;
            GT_U32    dataErrorStatusReg[4]/*reg*/;

        }DFX;

        struct /*interrupts*/{

            GT_U32    rxDMAInterruptSummaryCause;
            GT_U32    rxDMAInterruptSummaryMask;
            GT_U32    rxDMAFunctionalInterruptCause;
            GT_U32    rxDMAFunctionalInterruptMask;

        }interrupts;

        struct /*iBufConfigs*/{

            GT_U32    iBufSetThresholdLowPorts;
            GT_U32    iBufSetThresholdHighPorts;

        }iBufConfigs;

        struct /*WRDMABAConfigs*/{

            GT_U32    lowDMAIFConfig;
            GT_U32    highDMAIFConfig;
            GT_U32    BADebugHook;

        }WRDMABAConfigs;

    }RXDMA /*- RXDMA*/ ;
    struct /*TXDMA - TXDMA */{
        SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;

        struct /*burstSizeLimit*/{

            GT_U32    burstLimitGlobalConfig;
            GT_U32    portBurstLimitThresholds[13]/*port*/;

        }burstSizeLimit;

        struct /*cutThrough*/{

            GT_U32    port_MemoryRateLimitThreshold[6]/*port*/;
            GT_U32    cpuPortMemoryRateLimitThreshold;

        }cutThrough;

        struct /*DFT*/{

            GT_U32    txDMADFT0;
            GT_U32    txDMADFT1;
            GT_U32    txDMADFT3;

        }DFT;

        struct /*interrupts*/{

            GT_U32    txDMAInterruptSummary;
            GT_U32    txDMAInterruptCause;
            GT_U32    txDMAInterruptMask;
            GT_U32    dataErrorInterruptCause;
            GT_U32    dataErrorInterruptMask;
            GT_U32    dataErrorStatusReg[7]/*reg*/;
            GT_U32    lastDescAccess[6]/*Reg*/;

        }interrupts;

        struct /*TXDMAEngineConfig*/{

            GT_U32    bufferMemoryArbitersConfig;
            GT_U32    bufferMemoryFirstXGReadBandwidthConfig;
            GT_U32    bufferMemoryRdDMA2ndStageArbiterConfig;
            GT_U32    bufferMemoryMiscReg;
            GT_U32    bufferMemoryGigFIFOsThreshold;
            GT_U32    bufferMemoryXGFIFOsThreshold;
            GT_U32    bufferMemoryHGL20GFIFOsThreshold;
            GT_U32    bufferMemoryXLGFIFOsThreshold;
            GT_U32    bufferMemoryCPUFIFOsThreshold;
            GT_U32    bufferMemoryMiscThresholdsConfig;
            GT_U32    descPrefetchSharing;
            GT_U32    extPortsConfigReg;
            GT_U32    pizzaArbiterCtrlReg;
            GT_U32    pizzaArbiterSliceMapReg[8]/*reg number*/;
            GT_U32    pizzaArbiterDebugReg;

        }TXDMAEngineConfig;


    }TXDMA /*- TXDMA*/ ;
    struct /*MEM - MEM */{
        SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;

        struct /*arbitersConfigRegs*/{

            GT_U32    arbitersConfigReg0;

        }arbitersConfigRegs;

        struct /*buffersMemoryAndMACErrorsIndicationsInterrupts*/{

            GT_U32    bufferMemoryInterruptCauseReg0;
            GT_U32    bufferMemoryInterruptMaskReg0;
            GT_U32    bufferMemoryMainInterruptCauseReg;
            GT_U32    bufferMemoryMainInterruptMaskReg;
            GT_U32    bufferMemoryInterruptCauseReg1;
            GT_U32    bufferMemoryInterruptMaskReg1;

        }buffersMemoryAndMACErrorsIndicationsInterrupts;


    }MEM /*- MEM*/ ;
    struct /*centralizedCntrs - Centralized Counters */{
        SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;

        struct /*DFX*/{

            GT_U32    dataErrorInterruptCause;
            GT_U32    dataErrorInterruptMask;
            GT_U32    dataErrorStatusReg[8]/*reg*/;

        }DFX;

        struct /*globalRegs*/{

            GT_U32    CNCGlobalConfigReg;
            GT_U32    CNCFastDumpTriggerReg;
            GT_U32    CNCClearByReadValueRegWord0;
            GT_U32    CNCClearByReadValueRegWord1;
            GT_U32    CNCInterruptSummaryCauseReg;
            GT_U32    CNCInterruptSummaryMaskReg;
            GT_U32    CNCFunctionalInterruptCauseReg;
            GT_U32    CNCFunctionalInterruptMaskReg;

        }globalRegs;

        struct /*perBlockRegs*/{

            GT_U32    CNCBlockWraparoundStatusReg[8][4]/*Block*//*Entry*/;
            GT_U32    CNCBlockConfigReg[2][8]/*Block*//*Client*/;

        }perBlockRegs;


    }centralizedCntrs /*- Centralized Counters*/ ;

    struct /*uniphySERDES - Uniphy SERDES */{
        SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;

        struct /*LPSERDESInternalRegsSERDES1*/{

            GT_U32    PHYRevision;
            GT_U32    PLLINTPReg0;
            GT_U32    PLLINTPReg1;
            GT_U32    PLLINTPReg5;
            GT_U32    calibrationReg0;
            GT_U32    calibrationReg7;
            GT_U32    calibrationReg1;
            GT_U32    calibrationReg2;
            GT_U32    calibrationReg3;
            GT_U32    calibrationReg4;
            GT_U32    calibrationReg5;
            GT_U32    transmitterReg0;
            GT_U32    transmitterReg1;
            GT_U32    transmitterReg2;
            GT_U32    transmitterReg4;
            GT_U32    receiverReg[3];
            GT_U32    FFEReg0;
            GT_U32    DFEF0F1CoefficientCtrl;
            GT_U32    readyflagReg;
            GT_U32    G1Setting0;
            GT_U32    G1Setting1;
            GT_U32    G2Setting0;
            GT_U32    G2Setting1;
            GT_U32    G3Setting0;
            GT_U32    G3Setting1;
            GT_U32    configReg0;
            GT_U32    referenceReg0;
            GT_U32    powerReg0;
            GT_U32    resetReg0;
            GT_U32    PHYTestReg0;
            GT_U32    PHYTestReg1;
            GT_U32    PHYTestDataReg[6];
            GT_U32    PHYTestPRBSCntrReg[3];
            GT_U32    PHYTestPRBSErrorCntrReg0;
            GT_U32    PHYTestPRBSErrorCntrReg1;
            GT_U32    PHYTestOOBReg0;
            GT_U32    PHYTestOOBReg1;
            GT_U32    testReg0;
            GT_U32    digitalInterfaceReg0;

        }LPSERDESInternalRegsSERDES1;

    }uniphySERDES /*- Uniphy SERDES*/ ;
    struct /*networkPorts - Network Ports */{
        SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;

        struct /*QSGMIICtrlAndStatus*/{

            GT_U32    QSGMIICtrlReg0[6]/*QSGMII*/;
            GT_U32    QSGMIIStatusReg[6]/*QSGMII*/;
            GT_U32    QSGMIIPRBSErrorCntr[6]/*QSGMII*/;
            GT_U32    QSGMIICtrlReg1[6]/*QSGMII*/;
            GT_U32    QSGMIICtrlReg2[6]/*QSGMII*/;

        }QSGMIICtrlAndStatus;

    }networkPorts /*- Network Ports*/ ;
    struct /*EQ - EQ */{
        SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;

        struct /*applicationSpecificCPUCodes*/{

DONE_SIP5_ALIGN            GT_U32    TCPUDPDestPortRangeCPUCodeEntryWord0[16]/*Entry*/;
DONE_SIP5_ALIGN            GT_U32    TCPUDPDestPortRangeCPUCodeEntryWord1[16]/*Entry*/;
DONE_SIP5_ALIGN            GT_U32    IPProtocolCPUCodeEntry[4]/*Protocol*/;
DONE_SIP5_ALIGN            GT_U32    IPProtocolCPUCodeValidConfig;

        }applicationSpecificCPUCodes;

        struct /*CPUCodeTable*/{

SHOULD_BE_IGNORED_BY_SIP5            GT_U32    CPUCodeAccessCtrl;
SHOULD_BE_IGNORED_BY_SIP5            GT_U32    CPUCodeTableDataAccess;

        }CPUCodeTable;

        struct /*CPUTargetDeviceConfig*/{

DONE_SIP5_ALIGN            GT_U32    CPUTargetDeviceConfigReg0;
DONE_SIP5_ALIGN            GT_U32    CPUTargetDeviceConfigReg1;

        }CPUTargetDeviceConfig;

        struct /*ingrDropCodesCntr*/{

DONE_SIP5_ALIGN            GT_U32    dropCodesCntrConfig;
DONE_SIP5_ALIGN            GT_U32    dropCodesCntr;

        }ingrDropCodesCntr;

        struct /*ingrDropCntr*/{

DONE_SIP5_ALIGN            GT_U32    ingrDropCntrConfig;
DONE_SIP5_ALIGN            GT_U32    ingrDropCntr;

        }ingrDropCntr;

        struct /*ingrForwardingRestrictions*/{

DONE_SIP5_ALIGN            GT_U32    toCpuIngrForwardingRestrictionsConfig;
DONE_SIP5_ALIGN            GT_U32    TONETWORKIngrForwardingRestrictionsConfig;
DONE_SIP5_ALIGN            GT_U32    toAnalyzerIngrForwardingRestrictionsConfig;
DONE_SIP5_ALIGN            GT_U32    ingrForwardingRestrictionsDroppedPktsCntr;

        }ingrForwardingRestrictions;

        struct /*ingrSTCConfig*/{

DONE_SIP5_ALIGN            GT_U32    ingrSTCConfig;

        }ingrSTCConfig;

        struct /*ingrSTCInterrupt*/{

DONE_SIP5_ALIGN            GT_U32    ingrSTCInterruptCause;
DONE_SIP5_ALIGN            GT_U32    ingrSTCInterruptMask;

        }ingrSTCInterrupt;

        struct /*ingrSTCTable*/{

SHOULD_BE_IGNORED_BY_SIP5            GT_U32    ingrSTCTableAccessCtrl;
SHOULD_BE_IGNORED_BY_SIP5            GT_U32    ingrSTCTableWord0Access;
SHOULD_BE_IGNORED_BY_SIP5            GT_U32    ingrSTCTableWord1Read;
SHOULD_BE_IGNORED_BY_SIP5            GT_U32    ingrSTCTableWord2Access;

        }ingrSTCTable;

        struct /*logicalTargetMapTable*/{

SHOULD_BE_IGNORED_BY_SIP5            GT_U32    logicalTargetMapTableAccessCtrl;
SHOULD_BE_IGNORED_BY_SIP5            GT_U32    logicalTargetMapTableDataAccess;
SHOULD_BE_IGNORED_BY_SIP5            GT_U32    logicalTargetMapTableDataAccess2;

        }logicalTargetMapTable;

        struct /*mirrToAnalyzerPortConfigs*/{

DONE_SIP5_ALIGN            GT_U32    ingrStatisticMirrToAnalyzerPortConfig;
DONE_SIP5_ALIGN            GT_U32    ingrAndEgrMonitoringToAnalyzerQoSConfig;
SHOULD_BE_IGNORED_BY_SIP5            GT_U32    egrMonitoringEnableConfig[4]/*Reg*/;
SHOULD_BE_IGNORED_BY_SIP5            GT_U32    cascadeEgrMonitoringEnableConfig;
DONE_SIP5_ALIGN            GT_U32    portIngrMirrorIndex[3];
SHOULD_BE_IGNORED_BY_SIP5            GT_U32    portEgrMirrorIndex[2][7]; /*[(port / 64) --> 0..1] [((port % 63)/10) --> 0..7]*/
DONE_SIP5_ALIGN            GT_U32    mirrorInterfaceParameterReg[7]/*Parameter Number*/;
DONE_SIP5_ALIGN            GT_U32    analyzerPortGlobalConfig;
SHOULD_BE_IGNORED_BY_SIP5            GT_U32    cascadeEgressMonitoringEnableConfiguration;
        }mirrToAnalyzerPortConfigs;

        struct /*multiCoreLookup*/{

SHOULD_BE_IGNORED_BY_SIP5            GT_U32    multiCoreLookup[9]; /* (1(lookup 0)+ 2(lookup 1,2 for LionB)+ 6(lookup 3-8 for Lion2)) */
        }multiCoreLookup;

        struct /*numberOfTrunkMembersTable*/{

SHOULD_BE_IGNORED_BY_SIP5            GT_U32    numberOfTrunkMembersTableEntry[16]/*Entry*/;

        }numberOfTrunkMembersTable;

        struct /*parityError*/{

NOT_RELEVANT_TO_SIM            GT_U32    parityErrorInterruptCause;
NOT_RELEVANT_TO_SIM            GT_U32    parityErrorInterruptMask;
NOT_RELEVANT_TO_SIM            GT_U32    parityErrorStatusReg[4];
        }parityError;

        struct /*preEgrEngineGlobalConfig*/{

NEED_TO_BE_USED_BY_SIP5            GT_U32    duplicationOfPktsToCPUConfig;
NOT_RELEVANT_TO_SIM            GT_U32    dualDeviceIDAndLossyDropConfig;
DONE_SIP5_ALIGN            GT_U32    preEgrEngineGlobalConfig;
DONE_SIP5_ALIGN            GT_U32    logicalTargetDeviceMapConfig;

        }preEgrEngineGlobalConfig;

        struct /*preEgrEngineInternal*/{

SHOULD_BE_IGNORED_BY_SIP5            GT_U32    preEgrEngineOutGoingDescWord[5]/*Word*/;
SHOULD_BE_IGNORED_BY_SIP5            GT_U32    preEgrEngineOutGoingDescWord1[2]/*Word*/;


        }preEgrEngineInternal;

        struct /*preEgrInterrupt*/{

DONE_SIP5_ALIGN            GT_U32    preEgrInterruptSummary;
DONE_SIP5_ALIGN            GT_U32    preEgrInterruptSummaryMask;

        }preEgrInterrupt;

        struct /*qoSProfileToQoSTable*/{

SHOULD_BE_IGNORED_BY_SIP5            GT_U32    qoSProfileToQoSAccessCtrl;
SHOULD_BE_IGNORED_BY_SIP5            GT_U32    qoSProfileToQoSTableDataAccess;

        }qoSProfileToQoSTable;

        struct /*statisticalRateLimitsTable*/{

SHOULD_BE_IGNORED_BY_SIP5            GT_U32    statisticalRateLimitsTableAccessCtrl;
SHOULD_BE_IGNORED_BY_SIP5            GT_U32    statisticalRateLimitsTableDataAccess;

        }statisticalRateLimitsTable;

        struct /*STCRateLimitersInterrupt*/{

NOT_RELEVANT_TO_SIM            GT_U32    CPUCodeRateLimitersInterruptCause;
NOT_RELEVANT_TO_SIM            GT_U32    CPUCodeRateLimitersInterruptMask;

        }STCRateLimitersInterrupt;

        struct /*toCpuPktRateLimiters*/{

NOT_RELEVANT_TO_SIM            GT_U32    CPUCodeRateLimiterDropCntr;
NOT_RELEVANT_TO_SIM            GT_U32    toCpuRateLimiterPktCntr[31]/*Limiter*/;

        }toCpuPktRateLimiters;

        struct /*Vlan Egress Filter Counter*/{
SHOULD_BE_IGNORED_BY_SIP5            GT_U32  vlanEgressFilterCounter;
        }vlanEgressFilter;


        struct /*egress Filter Vlan Map*/{
SHOULD_BE_IGNORED_BY_SIP5            GT_U32    egressFilterVlanMapTableAccessCtrl;
SHOULD_BE_IGNORED_BY_SIP5            GT_U32    egressFilterVlanMapTableDataAccess;
        }egressFilterVlanMap;


        struct /*egress Filter Vlan Member Table*/{
SHOULD_BE_IGNORED_BY_SIP5            GT_U32    egressFilterVlanMemberTableAccessCtrl;
SHOULD_BE_IGNORED_BY_SIP5            GT_U32    egressFilterVlanMemberTableDataAccess;
SHOULD_BE_IGNORED_BY_SIP5            GT_U32    egressFilterVlanMemberTableDataAccess2;
        }egressFilterVlanMemberTable;


        struct /*trunksMembersTable*/{

SHOULD_BE_IGNORED_BY_SIP5            GT_U32    trunkTableAccessCtrl;
SHOULD_BE_IGNORED_BY_SIP5            GT_U32    trunkTableDataAccess;

        }trunksMembersTable;

    }EQ /*- EQ*/ ;

    struct /*IPCL - IPCL */{
        SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;

DONE_SIP5_ALIGN        GT_U32    ingrPolicyGlobalConfig;
NOT_RELEVANT_TO_SIM        GT_U32    policyEngineInterruptCause;
NOT_RELEVANT_TO_SIM        GT_U32    policyEngineInterruptMask;
DONE_SIP5_ALIGN        GT_U32    policyEngineConfig;
DONE_SIP5_ALIGN        GT_U32    policyEngineUserDefinedBytesConfig;
DONE_SIP5_ALIGN        GT_U32    CRCHashConfig;
DONE_SIP5_ALIGN        GT_U32    policyEngineUserDefinedBytesExtConfig;
        GT_U32    PCLUnitInterruptSummaryCause;
        GT_U32    PCLUnitInterruptSummaryMask;
        GT_U32    configTableParityErrorCntr[2]/*pcl config table*/;
DONE_SIP5_ALIGN        GT_U32    L2L3IngrVLANCountingEnable[2]/*Reg*/;
DONE_SIP5_ALIGN        GT_U32    countingModeConfig;
DONE_SIP5_ALIGN        GT_U32    pearsonHashTable[16]/*index div 4*/;

        struct /*lookupKeyUserDefinedBytes*/{

            GT_U32    key0UserDefinedBytesConfigReg0;
            GT_U32    key0UserDefinedBytesConfigReg1;
            GT_U32    key1UserDefinedBytesConfigReg0;
            GT_U32    key1UserDefinedBytesConfigReg1;
            GT_U32    key2UserDefinedBytesConfig;
            GT_U32    key3UserDefinedBytesConfig;
            GT_U32    key3AndKey4UserDefinedBytesConfig;
            GT_U32    key5UserDefinedBytesConfig;

        }lookupKeyUserDefinedBytes;
    }IPCL /*- IPCL*/ ;
    SMEM_CHT_PP_REGS_POLICER_UNIT_STC PLR[3] /*- IPLR0, IPLR1, EPLR (shared)*/ ;

    struct /*MLL - MLL */{
        SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;

        struct /*MLLGlobalCtrl*/{

DONE_SIP5_ALIGN            GT_U32    MLLGlobalCtrl;

        }MLLGlobalCtrl;

        struct /*MLLOutInterfaceCntrs*/{

DONE_SIP5_ALIGN            GT_U32    MLLOutMcPktsCntr[2]/*Counter*/;
DONE_SIP5_ALIGN            GT_U32    MLLOutInterfaceCntrConfig[2]/*Counter*/;
DONE_SIP5_ALIGN            GT_U32    MLLMCFIFODropCntr;

        }MLLOutInterfaceCntrs;

        struct /*L2 MLL VIDX enable*/{
SHOULD_BE_IGNORED_BY_SIP5            GT_U32    l2MllVidxEnableTableAccessCtrl;
SHOULD_BE_IGNORED_BY_SIP5            GT_U32    l2MllVidxEnableTableDataAccess;
        }l2MllVidxEnableTable;

        struct /*L2 MLL Pointer Map*/{
SHOULD_BE_IGNORED_BY_SIP5            GT_U32    l2MllPointerMapTableAccessCtrl;
SHOULD_BE_IGNORED_BY_SIP5            GT_U32    l2MllPointerMapTableDataAccess;
        }l2MllPointerMapTable;

        struct /*multiTargetTCQueuesAndArbitrationConfig*/{

NOT_RELEVANT_TO_SIM            GT_U32    multiTargetTCQueuesGlobalConfig;
NOT_RELEVANT_TO_SIM            GT_U32    multitargetTCQueuesStrictPriorityEnableConfig;
NOT_RELEVANT_TO_SIM            GT_U32    multitargetTCQueuesWeightConfig;

        }multiTargetTCQueuesAndArbitrationConfig;

        struct /*multiTargetVsUcSDWRRAndStrictPriorityScheduler*/{

NOT_RELEVANT_TO_SIM            GT_U32    mcUcSDWRRAndStrictPriorityConfig;

        }multiTargetVsUcSDWRRAndStrictPriorityScheduler;

        struct /*multiTargetRateShape*/{

NOT_RELEVANT_TO_SIM            GT_U32    multiTargetRateShapeConfig;

        }multiTargetRateShape;

        struct /*routerMcLinkedListMLLTables*/{

NOT_RELEVANT_TO_SIM            GT_U32    qoSProfileToMultiTargetTCQueuesReg[8]/*Index*/;
NOT_RELEVANT_TO_SIM            GT_U32    tableAccessCtrl;

        }routerMcLinkedListMLLTables;

    }MLL /*- MLL*/ ;
    struct /*TCCLowerIPCL - TCC Lower (IPCL) */{
        SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;

        struct /*TCCIPCL*/{

            GT_U32    actionTableAndPolicyTCAMAccessData[12]/*Data*/;
            GT_U32    actionTableAndPolicyTCAMAccessCtrlParameters1;
            GT_U32    actionTableAndPolicyTCAMAccessCtrlParameters2;
            GT_U32    actionTableAndPolicyTCAMAccessCtrl;

            GT_U32    IPCLKeyType2LookupIDReg0[3]/*Policy_Lookup*/;
            GT_U32    IPCLKeyType2LookupIDReg1[3]/*Policy_Lookup*/;
            GT_U32    EPCLKeyType2LookupIDReg;

            GT_U32    TCCECCErrorInformation;
            GT_U32    TCAMErrorCntr;
            GT_U32    TCCInterruptCause;
            GT_U32    TCCInterruptMask;
            GT_U32    TCCUnitInterruptSummaryCause;
            GT_U32    TCCUnitInterruptSummaryMask;

        }TCCIPCL;


    }TCCLowerIPCL /*- TCC Lower (IPCL)*/ ;
    struct /*TCCUpperIPvX - TCC Upper (IPvX) */{
        SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;

        struct /*TCCIPvX*/{

            GT_U32    TTIKeyType2LookupID;
            GT_U32    routerDIPLookupKeyType2LookupID;
            GT_U32    routerSIPLookupKeyType2LookupID;

            GT_U32    routerTCAMAccessDataReg[6]/*word*/;
            GT_U32    routerTCAMAccessCtrlReg0;
            GT_U32    routerTCAMAccessCtrlReg1;

            GT_U32    TCAMErrorCntr;
            GT_U32    TCCInterruptCause;
            GT_U32    TCCInterruptMask;
            GT_U32    TCCUnitInterruptSummaryCause;
            GT_U32    TCCUnitInterruptSummaryMask;
            GT_U32    TCCECCErrorInformation;

        }TCCIPvX;


    }TCCUpperIPvX /*- TCC Upper (IPvX)*/;
    struct /*TTI - TTI */{
        SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;

        SMEM_CHT_PP_REGS_GTS_UNIT_STC ITS;

        struct /*logFlow*/{

DONE_SIP5_ALIGN            GT_U32    bridgeLogFlowMACDALow;
DONE_SIP5_ALIGN            GT_U32    bridgeLogFlowMACDAHigh;
DONE_SIP5_ALIGN            GT_U32    bridgeLogFlowMACDALowMask;
DONE_SIP5_ALIGN            GT_U32    bridgeLogFlowMACDAHighMask;
DONE_SIP5_ALIGN            GT_U32    bridgeLogFlowMACSALow;
DONE_SIP5_ALIGN            GT_U32    bridgeLogFlowMACSAHigh;
DONE_SIP5_ALIGN            GT_U32    bridgeLogFlowMACSALowMask;
DONE_SIP5_ALIGN            GT_U32    bridgeLogFlowMACSAHighMask;
DONE_SIP5_ALIGN            GT_U32    bridgeLogFlowEtherType;
DONE_SIP5_ALIGN            GT_U32    routerLogFlowDIP[4]/*DIP_Part*/;
DONE_SIP5_ALIGN            GT_U32    routerLogFlowDIPMask[4]/*DIP_Part*/;
DONE_SIP5_ALIGN            GT_U32    routerLogFlowSIP[4]/*SIP_Part*/;
DONE_SIP5_ALIGN            GT_U32    routerLogFlowSIPMask[4]/*SIP_Part*/;
DONE_SIP5_ALIGN            GT_U32    routerLogFlowIPProtocol;
DONE_SIP5_ALIGN            GT_U32    routerLogFlowSourceTCPUDPPort;
DONE_SIP5_ALIGN            GT_U32    routerLogFlowDestinationTCPUDPPort;

        }logFlow;

        struct /*MAC2ME*/{

DONE_SIP5_ALIGN            GT_U32    MAC2MELow[8]/*Number of MAC*/;
DONE_SIP5_ALIGN            GT_U32    MAC2MEHigh[8]/*Number of MAC*/;
DONE_SIP5_ALIGN            GT_U32    MAC2MELowMask[8]/*Number of MAC*/;
DONE_SIP5_ALIGN            GT_U32    MAC2MEHighMask[8]/*Number of MAC*/;

DONE_SIP5_ALIGN            GT_U32    MGLookupToInternalTCAMData[2]/*Data*/;
DONE_SIP5_ALIGN            GT_U32    MGLookupToInternalTCAMCtrl;
DONE_SIP5_ALIGN            GT_U32    MGLookupToInternalTCAMResult;

        }MAC2ME;

        struct /*protocolMatch*/{

DONE_SIP5_ALIGN            GT_U32    protocolsConfig[6]/*Protocols*/;
DONE_SIP5_ALIGN            GT_U32    protocolsEncapsulationConfig0;
DONE_SIP5_ALIGN            GT_U32    protocolsEncapsulationConfig1;

        }protocolMatch;

        struct /*qoSMapTables*/{

DONE_SIP5_ALIGN            GT_U32    UPToQoSParametersMapTableReg[4]/*a mapping table register index*/;
DONE_SIP5_ALIGN            GT_U32    EXPToQoSParametersMapTableReg[4]/*a mapping table register index*/;
DONE_SIP5_ALIGN            GT_U32    UP2UPMapTable;

DONE_SIP5_ALIGN            GT_U32    DSCP_ToQoSProfileMap[16]/*DSCP Value*/;
DONE_SIP5_ALIGN            GT_U32    CFIUP_ToQoSProfileMapTable[2][2][2]/*UPEntry*//*CFI*//*UP Profile*/;
DONE_SIP5_ALIGN            GT_U32    EXPToQoSProfileMap[2]/*EXPEntry*/;
DONE_SIP5_ALIGN            GT_U32    CFI_UPToQoSProfileMapTableSelector[16]/*SrcPort div 4*/;

        }qoSMapTables;

        struct /*trunkHash*/{

DONE_SIP5_ALIGN            GT_U32    trunkHashConfigReg0;
DONE_SIP5_ALIGN            GT_U32    trunkHashConfigReg1;
DONE_SIP5_ALIGN            GT_U32    trunkHashConfigReg2;

        }trunkHash;

        struct /*userDefinedBytes*/{

DONE_SIP5_ALIGN            GT_U32    IPCLTCPPortRangeComparatorConfig[8]/*TCPCmp*/;
DONE_SIP5_ALIGN            GT_U32    IPCLUDPPortRangeComparatorConfig[8]/*UDPCmp*/;

        }userDefinedBytes;

        struct /*VLANAssignment*/{

DONE_SIP5_ALIGN            GT_U32    ingrTPIDConfig[4]/*VLAN_Ethertype_index*/;
DONE_SIP5_ALIGN            GT_U32    ingrTPIDSelect[32]/*Port*/;
DONE_SIP5_ALIGN            GT_U32    passengerIngrTPIDSelect[32]/*Port*/;

        }VLANAssignment;

DONE_SIP5_ALIGN        GT_U32    TTIUnitGlobalConfig;
DONE_SIP5_ALIGN        GT_U32    TTIEngineInterruptCause;
DONE_SIP5_ALIGN        GT_U32    TTIEngineInterruptMask;
DONE_SIP5_ALIGN        GT_U32    TTIEngineConfig;
DONE_SIP5_ALIGN        GT_U32    TTIPCLIDConfig0;
DONE_SIP5_ALIGN        GT_U32    TTIPCLIDConfig1;
DONE_SIP5_ALIGN        GT_U32    TTIIPv4GREEthertype;
NOT_SUPPORTED_SIP5_ALIGN        GT_U32    SrcPortBpeEnable;
DONE_SIP5_ALIGN        GT_U32    IPMinimumOffset;
DONE_SIP5_ALIGN        GT_U32    CNAndFCConfig;
DONE_SIP5_ALIGN        GT_U32    specialEtherTypes;
DONE_SIP5_ALIGN        GT_U32    ingrCPUTrigSamplingConfig;
DONE_SIP5_ALIGN        GT_U32    MPLSEtherTypes;
DONE_SIP5_ALIGN        GT_U32    pseudoWireConfig;
DONE_SIP5_ALIGN        GT_U32    IPv6ExtensionValue;
DONE_SIP5_ALIGN        GT_U32    IPv6SolicitedNodeMcAddrConfigReg[4];
DONE_SIP5_ALIGN        GT_U32    IPv6SolicitedNodeMcAddrMaskReg[4];
DONE_SIP5_ALIGN        GT_U32    DSAConfig;
DONE_SIP5_ALIGN        GT_U32    loopPort[2];
DONE_SIP5_ALIGN        GT_U32    userDefineEtherTypes[2];
DONE_SIP5_ALIGN        GT_U32    coreRingPortEnable[2];
DONE_SIP5_ALIGN        GT_U32    ECMCPID[2];
DONE_SIP5_ALIGN        GT_U32    ECMCPIDMask[2];
DONE_SIP5_ALIGN        GT_U32    CMtagEtherType;
DONE_SIP5_ALIGN        GT_U32    TTIInternalConfigurations;
DONE_SIP5_ALIGN        GT_U32    PTPEtherTypes;
DONE_SIP5_ALIGN        GT_U32    PTPOverUDPDestinationPorts;
NOT_SUPPORTED_SIP5_ALIGN        GT_U32    PTPPktCommandConfig0;
NOT_SUPPORTED_SIP5_ALIGN        GT_U32    PTPPktCommandConfig1;
NOT_SUPPORTED_SIP5_ALIGN        GT_U32    TTIUnitInterruptSummaryCause;
NOT_SUPPORTED_SIP5_ALIGN        GT_U32    TTIUnitInterruptSummaryMask;
DONE_SIP5_ALIGN        GT_U32    CFMEtherType;
DONE_SIP5_ALIGN        GT_U32    IPv4TotalLengthDeduction;
DONE_SIP5_ALIGN        GT_U32    DSCP2DSCPMapTable[16]/*DSCP*/;
DONE_SIP5_ALIGN        GT_U32    receivedFlowCtrlPktsCntr;
DONE_SIP5_ALIGN        GT_U32    droppedFlowCtrlPktsCntr;
SIM_TBD_BOOKMARK       GT_U32    timeStampTagConfiguration;
NOT_SUPPORTED_SIP5_ALIGN        GT_U32    metalFix;
    }TTI /*- TTI*/ ;

    struct /*DFX_SERVER - added manually */{
        SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;

        GT_U32    serverResetControl;
        GT_U32    initDoneStatus;
    }DFX_SERVER /* DFX_SERVER - added manually */;
#define MAX_MAC_PORTS_CNS 256/* needed for 256 ports in vlan entry in bobcat2*/
    struct /*GOP - GOP */{
        struct/*ports*/{
            SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;
            /* All network ports with speeds: 10/100 MB/s, 1/1.2GB/s */
            struct /*gigPort*/{

                GT_U32    LPICtrl0;
                GT_U32    LPICtrl1;
                GT_U32    LPIStatus;
                GT_U32    LPICntr;

                GT_U32    portAutoNegConfig;

                GT_U32    portInterruptCause;
                GT_U32    portInterruptMask;

                GT_U32    portSerialParametersConfig;
                GT_U32    portSerialParameters1Config;

                GT_U32    portStatusReg0;
                GT_U32    portStatusReg1;

                GT_U32    portPRBSStatusReg;
                GT_U32    portPRBSErrorCntr;
                GT_U32    portSERDESConfigReg[4];

                GT_U32    portMACCtrlReg[5];
                GT_U32    config100FXRegports2427Only;
                GT_U32    stackPortMIBCntrsCtrl;
                GT_U32    CCFCPortSpeedTimer[8]/*Port Speed Index*/;

                GT_U32    portInterruptSummaryCause;/*bobcat2: bit 1 - point to Giga interrupt register , bit 2 - point to PTP interrupt register */
                GT_U32    portInterruptSummaryMask; /*bobcat2*/

                /* start EEE support - bobcat2 */
                GT_U32    lpiControlReg[2];
                GT_U32    lpiStatus;
                /* end EEE support */

            }gigPort[MAX_MAC_PORTS_CNS];

            struct /*gigPort63CPU*/{

                GT_U32    portAutoNegConfig;

                GT_U32    portInterruptCause;
                GT_U32    portInterruptMask;

                GT_U32    portSerialParametersConfig;

                GT_U32    portStatusReg0;
                GT_U32    portStatusReg1;

                GT_U32    portPRBSStatusReg;
                GT_U32    portPRBSErrorCntr;

                GT_U32    portSERDESConfigReg[4];

                GT_U32    portMACCtrlReg[5];
                GT_U32    CCFCPortSpeedTimer[8]/*Port Speed Index*/;

            }gigPort63CPU;

            struct /*XLGIP*/{

                GT_U32    portMACCtrlReg[5];
                GT_U32    portStatus;
                GT_U32    portInterruptCause;
                GT_U32    portInterruptMask;
                GT_U32    portPerPrioFlowCtrlStatus;
                GT_U32    xgMIBCountersControl;
                GT_U32    CNCCFCTimer[8]/*Port Speed Index*/;
                GT_U32    externalUnitsInterruptsCause;
                GT_U32    externalUnitsInterruptsMask;
                GT_U32    PPFCCtrl;

            }XLGIP[MAX_MAC_PORTS_CNS];

            struct /* MPCSIP */{
                GT_U32    PCS40GCommonControl;
            }MPCSIP[MAX_MAC_PORTS_CNS];
        }ports;

        struct /*SERDESes*/{
            SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;
            struct /*SERDES*/{
                GT_U32    SERDESExternalConfig[5];
                GT_U32    SERDESExternalStatus;
                GT_U32    SERDESMiscellaneous;
                GT_U32    SERDESMetalFix;

            }SERDES[36/*support bobcat2*/];
        }SERDESes;

        struct{
            SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;
        struct
        {

            GT_U32 macDa0;
            GT_U32 macSa0;
            GT_U32 etherType;
            GT_U32 vlanTag;
            GT_U32 dataPattern0;
            GT_U32 packetLength;
            GT_U32 packetCount;
            GT_U32 ifg;
            GT_U32 controlReg0;
            }packGenConfig[MAX_MAC_PORTS_CNS/*28*/];
        }packGenConfig;
    }GOP /*- GOP*/ ;


    struct /*temp*/{
        SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;

        /*IPVX unit*/
        GT_U32    _SMEM_LION2_IPVX_ROUTER_ADDITIONAL_CONTROL_2_REG;
        GT_U32    _SMEM_LION2_IPVX_ROUTER_EXTRA_MANAGEMENT_COUNTER_CONFIGURATION_REG[4];
DONE_SIP5_ALIGN        GT_U32    _SMEM_LION3_IPVX_FCOE_GLOBAL_CONFIG_REG;
    }temp;

}SMEM_CHT_PP_REGS_ADDR_STC;
/*
 * Typedef: enum SMEM_SIP5_PP_PLR_UNIT_ENT
 *
 * Description:
 *      types of PLR duplicated units
 *
 * Fields:
 *      SMEM_SIP5_PP_PLR_UNIT_IPLR_0_E  - IPLR 0
 *      SMEM_SIP5_PP_PLR_UNIT_IPLR_1_E  - IPLR 1
 *      SMEM_SIP5_PP_PLR_UNIT_EPLR_E    - EPLR
 *
 *      SMEM_SIP5_PP_PLR_UNIT__LAST__E - total number of PLR units
 * Comments:
 *
*/
typedef enum {
    SMEM_SIP5_PP_PLR_UNIT_IPLR_0_E,
    SMEM_SIP5_PP_PLR_UNIT_IPLR_1_E,
    SMEM_SIP5_PP_PLR_UNIT_EPLR_E,

    SMEM_SIP5_PP_PLR_UNIT__LAST__E

}SMEM_SIP5_PP_PLR_UNIT_ENT;


/*
 * Typedef: struct SMEM_SIP5_PP_REGS_ADDR_STC
 *
 * Description:
 *      Global registers data structure that represents registers addresses
 *      in common units
 *
 * Fields:
 *      unitBaseAddress     - unit base address
 *      Generic register names in common units
 *
 * Comments:
 *      All units structures taken from Cider and built by 'AutoBuilder'
 */
typedef struct{
    struct /*TTI*/{
        SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;

        struct /*VLANAssignment*/{

DONE_SIP5_ALIGN            GT_U32    ingrTPIDConfig_[4]/*VLAN_Ethertype_index*/;
DONE_SIP5_ALIGN            GT_U32    ingrTagProfileTPIDSelect[2][2]/*Tag*//*Profile group*/;
DONE_SIP5_ALIGN            GT_U32    ingrTPIDTagType;

        }VLANAssignment;

        struct /*userDefinedBytes*/{

DONE_SIP5_ALIGN            GT_U32    IPCLTCPPortRangeComparatorConfig[8]/*TCPCmp*/;
DONE_SIP5_ALIGN            GT_U32    IPCLUDPPortRangeComparatorConfig[8]/*UDPCmp*/;

        }userDefinedBytes;

        struct /*TTIUnitGlobalConfigs*/{

DONE_SIP5_ALIGN            GT_U32    TTIUnitGlobalConfig;
DONE_SIP5_ALIGN            GT_U32    TTIUnitGlobalConfigForCCPorts;
DONE_SIP5_ALIGN            GT_U32    TTIUnitGlobalConfigExt;

        }TTIUnitGlobalConfigs;

        struct /*TTIEngine*/{

NOT_SUPPORTED_SIP5_ALIGN   GT_U32    TTIEngineInterruptCause;
DONE_SIP5_ALIGN            GT_U32    TTIEngineInterruptMask;
DONE_SIP5_ALIGN            GT_U32    TTIEngineConfig;
DONE_SIP5_ALIGN            GT_U32    TTIPCLIDConfig0;
DONE_SIP5_ALIGN            GT_U32    TTIPCLIDConfig1;
DONE_SIP5_ALIGN            GT_U32    TTILookupIPExceptionCommands;
DONE_SIP5_ALIGN            GT_U32    TTIPktTypeKeys[8]/*packet types*/;
DONE_SIP5_ALIGN            GT_U32    TTILookupIPExceptionCodes0;
DONE_SIP5_ALIGN            GT_U32    TTIKeySegmentMode0;
DONE_SIP5_ALIGN            GT_U32    TTIKeySegmentMode1;
NOT_RELEVANT_TO_SIM        GT_U32    TTIKeyType;
DONE_SIP5_ALIGN            GT_U32    TTILookupMPLSExceptionCommands;
DONE_SIP5_ALIGN            GT_U32    TTILookupIPExceptionCodes1; /* used for ipv6 extensions exceptions checks only */

        }TTIEngine;

        struct /*trunkHashSimpleHash*/{

DONE_SIP5_ALIGN            GT_U32    trunkHashConfigReg0;
DONE_SIP5_ALIGN            GT_U32    trunkHashConfigReg1;
DONE_SIP5_ALIGN            GT_U32    trunkHashConfigReg2;

        }trunkHashSimpleHash;

        struct /*TRILLConfigs*/{

DONE_SIP5_ALIGN            GT_U32    TRILLGlobalConfigs0;
DONE_SIP5_ALIGN            GT_U32    TRILLGlobalConfigs1;
DONE_SIP5_ALIGN            GT_U32    TRILLExceptionCommands0;
DONE_SIP5_ALIGN            GT_U32    TRILLExceptionCommands1;
DONE_SIP5_ALIGN            GT_U32    TRILLGlobalConfigs2;

        }TRILLConfigs;

        struct /*TRILLAdjacency*/{

NEED_TO_BE_USED_BY_SIP5    GT_U32    adjacencyEntryReg2[128]/*entry*/; /* trill no need at this stage */
NEED_TO_BE_USED_BY_SIP5    GT_U32    adjacencyEntryReg1[128]/*entry*/; /* trill no need at this stage */
NEED_TO_BE_USED_BY_SIP5    GT_U32    adjacencyEntryReg0[128]/*entry*/; /* trill no need at this stage */

        }TRILLAdjacency;

        struct /*PTP*/{

DONE_SIP5_ALIGN            GT_U32    _1588V1DomainDomainNumber[4][4]/*i*//*Domain*/;
DONE_SIP5_ALIGN            GT_U32    PTPExceptionCheckingModeDomain[5]/*Domain*/;
DONE_SIP5_ALIGN            GT_U32    piggyBackTSConfigs[8]/*group of ports*/;
DONE_SIP5_ALIGN            GT_U32    PTPConfigs;
DONE_SIP5_ALIGN            GT_U32    PTPOverUDPDestinationPorts;
DONE_SIP5_ALIGN            GT_U32    PTPExceptionsAndCPUCodeConfig;
DONE_SIP5_ALIGN            GT_U32    PTPEtherTypes;
DONE_SIP5_ALIGN            GT_U32    PTPExceptionsCntr;
DONE_SIP5_ALIGN            GT_U32    _1588V2DomainDomainNumber;
DONE_SIP5_ALIGN            GT_U32    timestampEtherTypes;
DONE_SIP5_ALIGN            GT_U32    timestampConfigs;

        }PTP;

        struct /*pseudoWire*/{

DONE_SIP5_ALIGN            GT_U32    MPLSOAMChannelTypeProfile[15]/*channel type*/;
DONE_SIP5_ALIGN            GT_U32    PWE3Config;
DONE_SIP5_ALIGN            GT_U32    PWTagModeExceptionConfig;

        }pseudoWire;

        struct /*protocolMatch*/{

DONE_SIP5_ALIGN            GT_U32    protocolsConfig[6]/*Protocols*/;
DONE_SIP5_ALIGN            GT_U32    protocolsEncapsulationConfig0;
DONE_SIP5_ALIGN            GT_U32    protocolsEncapsulationConfig1;

        }protocolMatch;

        struct /*portMap*/{

NEED_TO_BE_USED_BY_SIP5    GT_U32    portGroupMap[32]/*8 ports*/; /* need to be used only after sip5 pcl alignment */
NEED_TO_BE_USED_BY_SIP5    GT_U32    portListMap[43]/*6 ports*/;  /* need to be used only after sip5 pcl alignment */

        }portMap;

        struct /*phyPortAndEPortAttributes*/{

DONE_SIP5_ALIGN   GT_U32    myPhysicalPortAttributes[256]/*Port*/;
DONE_SIP5_ALIGN   GT_U32    remotePhysicalPortAssignmentGlobalConfig;

        }phyPortAndEPortAttributes;

        struct /*MRUException*/{

DONE_SIP5_ALIGN   GT_U32    MRUSize[4]/*2 profiles*/;
DONE_SIP5_ALIGN   GT_U32    MRUCommandAndCode;

        }MRUException;


        struct /*MAC2ME*/{

            struct /*MAC2METCAMRegs*/{

SHOULD_BE_IGNORED_BY_SIP5  GT_U32    MGLookupToInternalTCAMData[3]/*Data*/;
SHOULD_BE_IGNORED_BY_SIP5  GT_U32    MGLookupToInternalTCAMCtrl;
SHOULD_BE_IGNORED_BY_SIP5  GT_U32    MGLookupToInternalTCAMResult;

            }MAC2METCAMRegs;

DONE_SIP5_ALIGN            GT_U32    MAC2MEHighMask[128]/*Number of MAC*/;
DONE_SIP5_ALIGN            GT_U32    MAC2MELow[128]/*Number of MAC*/;
DONE_SIP5_ALIGN            GT_U32    MAC2MEHigh[128]/*Number of MAC*/;
DONE_SIP5_ALIGN            GT_U32    MAC2MELowMask[128]/*Number of MAC*/;
DONE_SIP5_ALIGN            GT_U32    MAC2MESrcInterface[128]/*Mac2Me entry*/;
DONE_SIP5_ALIGN            GT_U32    MAC2MESrcInterfaceSel[128]/*Mac2Me Entry*/;

        }MAC2ME;

        struct /*IPv6SolicitedNode*/{

DONE_SIP5_ALIGN            GT_U32    IPv6SolicitedNodeMcAddrConfigReg0;
DONE_SIP5_ALIGN            GT_U32    IPv6SolicitedNodeMcAddrConfigReg1;
DONE_SIP5_ALIGN            GT_U32    IPv6SolicitedNodeMcAddrConfigReg2;
DONE_SIP5_ALIGN            GT_U32    IPv6SolicitedNodeMcAddrConfigReg3;
DONE_SIP5_ALIGN            GT_U32    IPv6SolicitedNodeMcAddrMaskReg0;
DONE_SIP5_ALIGN            GT_U32    IPv6SolicitedNodeMcAddrMaskReg1;
DONE_SIP5_ALIGN            GT_U32    IPv6SolicitedNodeMcAddrMaskReg2;
DONE_SIP5_ALIGN            GT_U32    IPv6SolicitedNodeMcAddrMaskReg3;

        }IPv6SolicitedNode;

        struct /*FCOE*/{

DONE_SIP5_ALIGN            GT_U32    FCoEGlobalConfig;
DONE_SIP5_ALIGN            GT_U32    FCoEExceptionConfigs;
DONE_SIP5_ALIGN            GT_U32    FCoEExceptionCntrs0;
DONE_SIP5_ALIGN            GT_U32    FCoEExceptionCntrs1;
DONE_SIP5_ALIGN            GT_U32    FCoEExceptionCntrs2;

        }FCOE;


        struct /*TTI_Internal*/{/* manually added : TTI_Internal */

NOT_RELEVANT_TO_SIM            GT_U32    ClearInterfaceCounter;
NOT_RELEVANT_TO_SIM            GT_U32    InbandClear;
NOT_RELEVANT_TO_SIM            GT_U32    TTIIDLEState;
NOT_RELEVANT_TO_SIM            GT_U32    TTIFIFOThresholdConfig;
NOT_RELEVANT_TO_SIM            GT_U32    TTIKeyDebugWord[8];/*word*/
NOT_RELEVANT_TO_SIM            GT_U32    TTIMessageToTxQ;
NOT_RELEVANT_TO_SIM            GT_U32    MetalFixRegister;
DONE_SIP5_ALIGN            GT_U32    TTIInternalConfig;
NOT_RELEVANT_TO_SIM            GT_U32    TTIUDBMetadata[8];
NOT_RELEVANT_TO_SIM            GT_U32    PCLUDBMetadata[23];
        }TTI_Internal;


        struct /*duplicationConfigs*/{

NOT_RELEVANT_TO_SIM        GT_U32    duplicationConfigs0;
DONE_SIP5_ALIGN            GT_U32    duplicationConfigs1;
DONE_SIP5_ALIGN            GT_U32    MPLSMCDALow;
DONE_SIP5_ALIGN            GT_U32    MPLSMCDAHigh;
DONE_SIP5_ALIGN            GT_U32    MPLSMCDALowMask;
DONE_SIP5_ALIGN            GT_U32    MPLSMCDAHighMask;

        }duplicationConfigs;

NOT_RELEVANT_TO_SIM    GT_U32    TTIKRuleMatchCntr[12]/*TTICounter*/;
DONE_SIP5_ALIGN        GT_U32    TTIUserDefinedEtherTypes_[4]/*index*/;
DONE_SIP5_ALIGN        GT_U32    TTIIPv6GREEtherType;
DONE_SIP5_ALIGN        GT_U32    TTIIPv4GREEthertype;
DONE_SIP5_ALIGN        GT_U32    specialEtherTypes;
DONE_SIP5_ALIGN        GT_U32    oamLabelGlobalConfiguration0;
DONE_SIP5_ALIGN        GT_U32    oamLabelGlobalConfiguration1;
DONE_SIP5_ALIGN        GT_U32    sourceIDAssignment0;
DONE_SIP5_ALIGN        GT_U32    sourceIDAssignment1;
DONE_SIP5_ALIGN        GT_U32    receivedFlowCtrlPktsCntr;
DONE_SIP5_ALIGN        GT_U32    preRouteExceptionMasks;
DONE_SIP5_ALIGN        GT_U32    PCLUserDefinedEtherTypes_[4]/*index*/;
DONE_SIP5_ALIGN        GT_U32    MPLSEtherTypes;
DONE_SIP5_ALIGN        GT_U32    IPv6ExtensionValue;
NEED_TO_BE_USED_BY_SIP5        GT_U32    IPMinimumOffset; /* should be implemented only after pcl alignment */
DONE_SIP5_ALIGN        GT_U32    IPLengthChecks;
NOT_RELEVANT_TO_SIM        GT_U32    ECCTwoErrorsCntr;
NOT_RELEVANT_TO_SIM        GT_U32    ECCOneErrorCntr;
DONE_SIP5_ALIGN        GT_U32    DSAConfig;
NOT_RELEVANT_TO_SIM    GT_U32    droppedFlowCtrlPktsCntr; /* no simulation of TXQ congestion */
NOT_RELEVANT_TO_SIM    GT_U32    CNAndFCConfig;
DONE_SIP5_ALIGN        GT_U32    CFMEtherType;

    }TTI;

     struct /*BM - BM */{

        SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;

        struct /*BMInterrupts*/{

            GT_U32    generalInterruptCause1;
            GT_U32    generalInterruptMask1;
            GT_U32    generalInterruptCause2;
            GT_U32    generalInterruptMask2;
            GT_U32    summaryInterruptCause;
            GT_U32    summaryInterruptMask;

        }BMInterrupts /*- BMInterrupts*/ ;

    }BM /*- BM*/ ;
    struct /*TXDMA*/{
        SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;
        struct /*interrupts*/{
            GT_U32  txDMAInterruptCause;
            GT_U32  txDMAInterruptMask;
            GT_U32  txDMAInterruptSummary;
            GT_U32  txDMAInterruptSummaryMask;
        }interrupts;
    }TXDMA[6];
    struct /*TXFIFO*/
    {
        SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;
        struct /*interrupts*/{
            GT_U32  txFIFOInterruptCause;
            GT_U32  txFIFOInterruptMask;
            GT_U32  txFIFOInterruptSummary;
            GT_U32  txFIFOInterruptSummaryMask;
        }interrupts;
    }TXFIFO[6];
    struct /*MPPM*/
    {
        SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;
        struct /*ECC*/
        {
            struct /*interrupts*/{
                GT_U32 EccErrorInterruptCause;
                GT_U32 EccErrorInterruptMask;
            }interrupts;
        }ECC;
        struct /*interrupts*/{
            GT_U32 MPPMInterruptsSummary;
            GT_U32 MPPMInterruptsSummaryMask;
        }interrupts;
    }MPPM;
    struct /*ETH*/{
        SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;
        struct /*TXFIFO*/{
            struct /*interrupts*/{
                GT_U32 EthTxFIFOInterruptCause;
                GT_U32 EthTxFIFOInterruptMask;
                GT_U32 EthTxFIFOInterruptsSummary;
                GT_U32 EthTxFIFOInterruptsSummaryMask;
            }interrupts;
        }TXFIFO;
    }ETH;

    struct /*rxDMA*/{
        SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;

        struct /*singleChannelDMAConfigs*/{

            GT_U32    SCDMAConfig0[73]/*Port*/;
            GT_U32    SCDMAConfig1[73]/*Port*/;
            GT_U32    SCDMACTPktIndentification[73]/*port*/;

        }singleChannelDMAConfigs;

        struct /*globalRxDMAConfigs*/{

            struct /*rejectConfigs*/{

                GT_U32    rejectPktCommand;
                GT_U32    rejectCPUCode;

            }rejectConfigs;

            struct /*globalConfigCutThrough*/{

                GT_U32    CTEtherTypeIdentification;
                GT_U32    CTUPEnable;

            }globalConfigCutThrough;

            struct /*buffersLimitProfiles*/{

                GT_U32    portsBuffersLimitProfile0XONXOFFConfig;
                GT_U32    portsBuffersLimitProfile0RxlimitConfig;
                GT_U32    portsBuffersLimitProfile1XONXOFFConfig;
                GT_U32    portsBuffersLimitProfile1RxlimitConfig;
                GT_U32    portsBuffersLimitProfile7XONXOFFConfig;
                GT_U32    portsBuffersLimitProfile2RxlimitConfig;
                GT_U32    portsBuffersLimitProfile3XONXOFFConfig;
                GT_U32    portsBuffersLimitProfile3RxlimitConfig;
                GT_U32    portsBuffersLimitProfile4XONXOFFConfig;
                GT_U32    portsBuffersLimitProfile4RxlimitConfig;
                GT_U32    portsBuffersLimitProfile5XONXOFFConfig;
                GT_U32    portsBuffersLimitProfile5RxlimitConfig;
                GT_U32    portsBuffersLimitProfile6XONXOFFConfig;
                GT_U32    portsBuffersLimitProfile6RxlimitConfig;
                GT_U32    portsBuffersLimitProfile2XONXOFFConfig;
                GT_U32    portsBuffersLimitProfile7RxlimitConfig;
                GT_U32    RFCntrOverrunLmitConfig;

            }buffersLimitProfiles;

            struct /*preIngrPrioritizationConfStatus*/{

                GT_U32    VLANUPPriorityArrayProfile[4]/*profile*/;
                GT_U32    MPLSEtherTypeConf[2]/*MPLS*/;
                GT_U32    VIDConf[4]/*index*/;
                GT_U32    genericEtherType[4]/*index*/;
                GT_U32    qoSDSAPriorityArrayProfileSegment[4][8]/*profile*//*segment*/;
                GT_U32    preingressPrioritizationEnable;
                GT_U32    PIPPriorityDropGlobalCntrsClear;
                GT_U32    PIPPrioThresholds;
                GT_U32    PIPMedPriorityDropGlobalCntrMsb;
                GT_U32    PIPMedPriorityDropGlobalCntrLsb;
                GT_U32    PIPLowPriorityDropGlobalCntrMsb;
                GT_U32    VLANEtherTypeConf[4]/*index*/;
                GT_U32    MACDA2MsbBytesConf[4]/*MAC DA conf*/;
                GT_U32    PIPHighPriorityDropGlobalCntrLsb;
                GT_U32    MPLSEXPPriorityArrayProfile[4]/*profile*/;
                GT_U32    UPDSATagArrayProfile[4]/*profile*/;
                GT_U32    MACDAPriorityArrayProfile[4]/*profile*/;
                GT_U32    MACDAMask4LsbConf[4]/*mask*/;
                GT_U32    MACDAMask2MsbConf[4]/*mask*/;
                GT_U32    MACDA4LsbBytesConf[4]/*MAC DA conf*/;
                GT_U32    PIPLowPriorityDropGlobalCntrLSb;
                GT_U32    IPv6TCPriorityArrayProfileSegment[4][16]/*profile*//*segment*/;
                GT_U32    IPv6EtherTypeConf;
                GT_U32    IPv4ToSPriorityArrayProfileSegment[4][16]/*profile*//*segment*/;
                GT_U32    IPv4EtherTypeConf;
                GT_U32    PIPHighPriorityDropGlobalCntrMsb;
                GT_U32    etherTypePriorityArrayProfile[4]/*profile*/;

            }preIngrPrioritizationConfStatus;

        }globalRxDMAConfigs;

        struct /*SIP_COMMON_MODULES*/{

            struct /*pizzaArbiter*/{

                GT_U32    pizzaArbiterConfigReg[85]/*reg number*/;
                GT_U32    pizzaArbiterCtrlReg;
                GT_U32    pizzaArbiterStatusReg;

            }pizzaArbiter;

        }SIP_COMMON_MODULES;

        struct /*interrupts*/{

            GT_U32    rxDMASCDMAInterruptMask[73]/*Port*/;
            GT_U32    rxDMASCDMAInterruptCause[73]/*Port*/;
            GT_U32    rxDMAInterrupt0Cause;
            GT_U32    rxDMAInterrupt0Mask;
            GT_U32    rxDMAInterruptSummary0Cause;
            GT_U32    rxDMAInterruptSummary0Mask;
            GT_U32    rxDMAInterruptSummary1Cause;
            GT_U32    rxDMAInterruptSummary1Mask;
            GT_U32    rxDMAInterruptSummary2Cause;
            GT_U32    rxDMAInterruptSummary2Mask;
            GT_U32    rxDMAInterruptSummary3Cause;
            GT_U32    rxDMAInterruptSummary3Mask;
            GT_U32    rxDMAInterruptSummaryCause;
            GT_U32    rxDMAInterruptSummaryMask;

        }interrupts;

        struct /*singleChannelDMAPip*/ {
            GT_U32    SCDMA_n_PIP_Config[73];
            GT_U32    SCDMA_n_Drop_Pkt_Counter[73];
        }singleChannelDMAPip;

    }rxDMA[6];

    struct /*GTS*/{
        SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;

        SMEM_CHT_PP_REGS_GTS_UNIT_STC           GTS;
    }GTS[2];

    struct /*L2I*/{
        SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;

        struct /*VLANRangesConfig*/{

DONE_SIP5_ALIGN            GT_U32    ingrVLANRangeConfig;

        }VLANRangesConfig;

        struct /*VLANMRUProfilesConfig*/{

DONE_SIP5_ALIGN            GT_U32    VLANMRUProfileConfig[4]/*Register*/;

        }VLANMRUProfilesConfig;

        struct /*UDPBcMirrorTrapUDPRelayConfig*/{

DONE_SIP5_ALIGN            GT_U32    UDPBcDestinationPortConfigTable[12]/*Port*/;

        }UDPBcMirrorTrapUDPRelayConfig;

        struct /*securityBreachStatus*/{

DONE_SIP5_ALIGN            GT_U32    securityBreachStatus0;
DONE_SIP5_ALIGN            GT_U32    securityBreachStatus1;
DONE_SIP5_ALIGN            GT_U32    securityBreachStatus2;
DONE_SIP5_ALIGN            GT_U32    securityBreachStatus3;

        }securityBreachStatus;

        struct /*MACBasedQoSTable*/{

DONE_SIP5_ALIGN            GT_U32    MACQoSTableEntry[7]/*QoS Profile*/;

        }MACBasedQoSTable;

        struct /*layer2BridgeMIBCntrs*/{

DONE_SIP5_ALIGN            GT_U32    setVLANIngrFilteredPktCount[2]/*SetNum*/;
DONE_SIP5_ALIGN            GT_U32    cntrsSetConfig0[2]/*Set*/;
DONE_SIP5_ALIGN            GT_U32    cntrsSetConfig1[2]/*Set*/;
DONE_SIP5_ALIGN            GT_U32    setIncomingPktCount[2]/*SetNum*/;
DONE_SIP5_ALIGN            GT_U32    setSecurityFilteredPktCount[2]/*SetNum*/;
DONE_SIP5_ALIGN            GT_U32    setBridgeFilteredPktCount[2]/*SetNum*/;
DONE_SIP5_ALIGN            GT_U32    MACAddrCount0;
DONE_SIP5_ALIGN            GT_U32    MACAddrCount1;
DONE_SIP5_ALIGN            GT_U32    MACAddrCount2;
DONE_SIP5_ALIGN            GT_U32    hostOutgoingMcPktCount;
DONE_SIP5_ALIGN            GT_U32    hostOutgoingPktsCount;
DONE_SIP5_ALIGN            GT_U32    hostIncomingPktsCount;
DONE_SIP5_ALIGN            GT_U32    hostOutgoingBcPktCount;
DONE_SIP5_ALIGN            GT_U32    matrixSourceDestinationPktCount;

        }layer2BridgeMIBCntrs;

        struct /*IPv6McBridgingBytesSelection*/{

DONE_SIP5_ALIGN            GT_U32    IPv6MCBridgingBytesSelectionConfig;

        }IPv6McBridgingBytesSelection;

        struct /*ingrPortsRateLimitConfig*/{

NOT_RELEVANT_TO_SIM        GT_U32    portSpeedOverrideEnable[8]/*Register*/;
NOT_RELEVANT_TO_SIM        GT_U32    MIISpeedGranularity[8]/*Register*/;
NOT_RELEVANT_TO_SIM        GT_U32    GMIISpeedGranularity[8]/*Register*/;
NOT_RELEVANT_TO_SIM        GT_U32    XGSpeedGranularity[8]/*Register*/;
NOT_RELEVANT_TO_SIM        GT_U32    CGSpeedGranularity[8]/*Register*/;
DONE_SIP5_ALIGN            GT_U32    ingrRateLimitConfig0;
NOT_RELEVANT_TO_SIM        GT_U32    ingrRateLimitConfig3;
NOT_RELEVANT_TO_SIM        GT_U32    ingrRateLimitConfig2;
DONE_SIP5_ALIGN            GT_U32    ingrRateLimitConfig1;
DONE_SIP5_ALIGN            GT_U32    ingrRateLimitDropCntr310;
DONE_SIP5_ALIGN            GT_U32    ingrRateLimitDropCntr3932;

        }ingrPortsRateLimitConfig;

        struct /*globalEportConifguration*/{

DONE_SIP5_ALIGN            GT_U32    globalEPortValue;
DONE_SIP5_ALIGN            GT_U32    globalEPortMask;

        }globalEportConifguration;

        struct /*eVLANMirrToAnalyzerEnable*/{

            GT_U32    eVLAN_Mirr_to_Analyzer_Enable[8]/*Register*/;

        }eVLANMirrToAnalyzerEnable;

        struct /*ctrlTrafficToCPUConfig*/{

DONE_SIP5_ALIGN            GT_U32    IPv6McLinkLocalCPUCodeIndex[8][2]/*CPU Code Index_n*//*CPU Code Index_t*/;
DONE_SIP5_ALIGN            GT_U32    IPv6McLinkLocalReged[8]/*Register*/;
DONE_SIP5_ALIGN            GT_U32    IPv6McLinkLocalConfig[32]/*Register*/;
DONE_SIP5_ALIGN            GT_U32    IPv6ICMPMsgTypeConfig[2]/*ICMP_Msg_Type*/;
DONE_SIP5_ALIGN            GT_U32    IPv6ICMPCommand;
DONE_SIP5_ALIGN            GT_U32    IPv4McLinkLocalReged[8]/*Register*/;
DONE_SIP5_ALIGN            GT_U32    IPv4McLinkLocalCPUCodeIndex[8][2]/*Register*//*Register_t*/;
DONE_SIP5_ALIGN            GT_U32    IPv4McLinkLocalConfig[32]/*Register*/;
DONE_SIP5_ALIGN            GT_U32    IEEEReservedMcReged[8]/*Register*/;
DONE_SIP5_ALIGN            GT_U32    IEEEReservedMcCPUIndex[16]/*Register*/;/*code uses ieeeRsrvMcCpuIndex */
DONE_SIP5_ALIGN            GT_U32    IEEEReservedMcConfig7[16]/*Register*/;/*code use ieeeRsrvMcConfTable , tableSet 7 */
DONE_SIP5_ALIGN            GT_U32    IEEEReservedMcConfig6[16]/*Register*/;/*code use ieeeRsrvMcConfTable , tableSet 6 */
DONE_SIP5_ALIGN            GT_U32    IEEEReservedMcConfig5[16]/*Register*/;/*code use ieeeRsrvMcConfTable , tableSet 5 */
DONE_SIP5_ALIGN            GT_U32    IEEEReservedMcConfig4[16]/*Register*/;/*code use ieeeRsrvMcConfTable , tableSet 4*/
DONE_SIP5_ALIGN            GT_U32    IEEEReservedMcConfig3[16]/*Register*/;/*code use ieeeRsrvMcConfTable , tableSet 3 */
DONE_SIP5_ALIGN            GT_U32    IEEEReservedMcConfig2[16]/*Register*/;/*code use ieeeRsrvMcConfTable , tableSet 2 */
DONE_SIP5_ALIGN            GT_U32    IEEEReservedMcConfig1[16]/*Register*/;/*code use ieeeRsrvMcConfTable , tableSet 1 */
DONE_SIP5_ALIGN            GT_U32    IEEEReservedMcConfig0[16]/*Register*/;/*code use ieeeRsrvMcConfTable , tableSet 0 */

        }ctrlTrafficToCPUConfig;

        struct /*bridgeEngineInterrupts*/{

DONE_SIP5_ALIGN            GT_U32    bridgeInterruptCause;
DONE_SIP5_ALIGN            GT_U32    bridgeInterruptMask;

        }bridgeEngineInterrupts;

        struct /*bridgeEngineConfig*/{

DONE_SIP5_ALIGN            GT_U32    bridgeGlobalConfig0;
DONE_SIP5_ALIGN            GT_U32    bridgeGlobalConfig1;
DONE_SIP5_ALIGN            GT_U32    bridgeCommandConfig0;
DONE_SIP5_ALIGN            GT_U32    bridgeCommandConfig1;
DONE_SIP5_ALIGN            GT_U32    bridgeCommandConfig2;
DONE_SIP5_ALIGN            GT_U32    bridgeCommandConfig3;
DONE_SIP5_ALIGN            GT_U32    bridgeSecurityBreachDropCntrCfg0;
DONE_SIP5_ALIGN            GT_U32    bridgeSecurityBreachDropCntrCfg1;
NEED_TO_BE_USED_BY_SIP5    GT_U32    bridgecpuPortConfig;

        }bridgeEngineConfig;

        struct /*bridgeDropCntrAndSecurityBreachDropCntrs*/{

DONE_SIP5_ALIGN            GT_U32    globalSecurityBreachFilterCntr;
DONE_SIP5_ALIGN            GT_U32    portVLANSecurityBreachDropCntr;
DONE_SIP5_ALIGN            GT_U32    bridgeFilterCntr;

        }bridgeDropCntrAndSecurityBreachDropCntrs;

        struct /*bridgeAccessMatrix*/{

DONE_SIP5_ALIGN            GT_U32    bridgeAccessMatrixLine[8]/*SA Access Level*/;
DONE_SIP5_ALIGN            GT_U32    bridgeAccessMatrixDefault;

        }bridgeAccessMatrix;
        GT_U32    l2iBadAddressLatchReg;
    }L2I;

    struct /*FDB*/{
        SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;

        struct /*FDBCore*/{

            struct /*sourceAddrNotLearntCntr*/{

DONE_SIP5_ALIGN                GT_U32    learnedEntryDiscardsCount;

            }sourceAddrNotLearntCntr;

            struct /*FDB*/{

NEED_TO_BE_USED_BY_SIP5                GT_U32    MTCPipeDispatcher;

            }FDB;

            struct /*FDBSERProtection*/{

                GT_U32    FDBFaultAddr[5]/*register*/;
                GT_U32    FDBSerConfig;
                GT_U32    FDBSerErrData;
                GT_U32    FDBSerErrAddr;
                GT_U32    FDBSerErrCntr;

            }FDBSERProtection;

            struct /*FDBSchedulerConfig*/{

                GT_U32    FDBSchedulerSWRRCtrl;
                GT_U32    FDBSchedulerSWRRConfig;

            }FDBSchedulerConfig;

            struct /*FDBInterrupt*/{

DONE_SIP5_ALIGN                GT_U32    FDBInterruptCauseReg;
DONE_SIP5_ALIGN                GT_U32    FDBInterruptMaskReg;

            }FDBInterrupt;

            struct /*FDBInternal*/{

DONE_SIP5_ALIGN                GT_U32    metalFix;
DONE_SIP5_ALIGN                GT_U32    FDBIdleState;

            }FDBInternal;

            struct /*FDBIndirectAccess*/{

DONE_SIP5_ALIGN                GT_U32    FDBIndirectAccessData[5]/*word*/;
DONE_SIP5_ALIGN                GT_U32    FDBIndirectAccessCtrl;
NEED_TO_BE_USED_BY_SIP5        GT_U32    FDBIndirectAccessAccess;

            }FDBIndirectAccess;

            struct /*FDBHashResults*/{

DONE_SIP5_ALIGN                GT_U32    FDBMultiHashCRCResultReg[8]/*reg_num*/;
DONE_SIP5_ALIGN                GT_U32    FDBNonMulti_hash_crc_result;
DONE_SIP5_ALIGN                GT_U32    FDBXorHash;

            }FDBHashResults;

            struct /*FDBGlobalConfig*/{

DONE_SIP5_ALIGN                GT_U32    FDBGlobalConfig;
DONE_SIP5_ALIGN                GT_U32    FDBGlobalConfig1;
NOT_SUPPORTED_SIP5_ALIGN                GT_U32    FDBCRCHashInitConfig;
DONE_SIP5_ALIGN                GT_U32    bridgeAccessLevelConfig;
DONE_SIP5_ALIGN                GT_U32    FDBAgingWindowSize;

            }FDBGlobalConfig;

            struct /*FDBCntrsUpdate*/{

DONE_SIP5_ALIGN                GT_U32    FDBCntrsUpdateCtrl;

            }FDBCntrsUpdate;

            struct /*FDBBankCntrs*/{

DONE_SIP5_ALIGN                GT_U32    FDBBankCntr[16]/*BankNum*/;
DONE_SIP5_ALIGN                GT_U32    FDBBankRank[16];/*rank -- manually replaced 16 registers into array*/
DONE_SIP5_ALIGN                GT_U32    FDBBankCntrsCtrl;

            }FDBBankCntrs;

            struct /*FDBAddrUpdateMsgs*/{

                struct /*AUMsgToCPU*/{

DONE_SIP5_ALIGN                    GT_U32    AUMsgToCPU;
DONE_SIP5_ALIGN                    GT_U32    AUFIFOToCPUConfig;

                }AUMsgToCPU;

                struct /*AUMsgFromCPU*/{

DONE_SIP5_ALIGN                    GT_U32    AUMsgFromCPUWord[6]/*word*/;
DONE_SIP5_ALIGN                    GT_U32    AUMsgFromCPUCtrl;

                }AUMsgFromCPU;

            }FDBAddrUpdateMsgs;

            struct /*FDBAction*/{

DONE_SIP5_ALIGN                GT_U32    FDBActionGeneral;
DONE_SIP5_ALIGN                GT_U32    FDBAction0;
DONE_SIP5_ALIGN                GT_U32    FDBAction1;
DONE_SIP5_ALIGN                GT_U32    FDBAction2;
DONE_SIP5_ALIGN                GT_U32    FDBAction3;
DONE_SIP5_ALIGN                GT_U32    FDBAction4;
DONE_SIP5_ALIGN                GT_U32    FDBActionOldParameters;
DONE_SIP5_ALIGN                GT_U32    FDBActionNewParameters;

            }FDBAction;

        }FDBCore;

    }FDB;

    struct /*EQ*/{
        SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;

        struct /*toCpuPktRateLimiters*/{

NOT_RELEVANT_TO_SIM            GT_U32    CPUCodeRateLimiterDropCntr;
NOT_RELEVANT_TO_SIM            GT_U32    toCpuRateLimiterPktCntr[255]/*Limiter*/;

        }toCpuPktRateLimiters;

        struct /*SCTRateLimitersInterrupt*/{

NOT_RELEVANT_TO_SIM            GT_U32    CPUCodeRateLimitersInterruptCause[32]/*reg*/;
NOT_RELEVANT_TO_SIM            GT_U32    CPUCodeRateLimitersInterruptMask[32]/*reg*/;
NOT_RELEVANT_TO_SIM            GT_U32    CPUCodeRateLimitersInterruptSummary;
NOT_RELEVANT_TO_SIM            GT_U32    CPUCodeRateLimitersInterruptSummaryMask;

        }SCTRateLimitersInterrupt;

        struct /*preEgrInterrupt*/{

DONE_SIP5_ALIGN            GT_U32    preEgrInterruptSummary;
DONE_SIP5_ALIGN            GT_U32    preEgrInterruptSummaryMask;

        }preEgrInterrupt;

        struct /*preEgrEngineGlobalConfig*/{

NEED_TO_BE_USED_BY_SIP5            GT_U32    duplicationOfPktsToCPUConfig[16]/*port*/;
NEED_TO_BE_USED_BY_SIP5            GT_U32    MTUTableEntry[4]/*port*/;
DONE_SIP5_ALIGN            GT_U32    preEgrEngineGlobalConfig;
NEED_TO_BE_USED_BY_SIP5            GT_U32    MTUGlobalConfig;
NOT_RELEVANT_TO_SIM            GT_U32    lossyDropConfig;
DONE_SIP5_ALIGN            GT_U32    cscdTrunkHashBitSelectionConfig;
NEED_TO_BE_USED_BY_SIP5            GT_U32    targetEPortMTUExceededCntr;

        }preEgrEngineGlobalConfig;

        struct /*mirrToAnalyzerPortConfigs*/{

DONE_SIP5_ALIGN            GT_U32    mirrorInterfaceParameterReg[7]/*Parameter Number*/;
DONE_SIP5_ALIGN            GT_U32    analyzerPortGlobalConfig;
SHOULD_BE_IGNORED_BY_SIP5            GT_U32    ingrStatisticMirrToAnalyzerPortConfig;
SHOULD_BE_IGNORED_BY_SIP5            GT_U32    ingrAndEgrMonitoringToAnalyzerQoSConfig;

        }mirrToAnalyzerPortConfigs;

        struct /*L2ECMP*/{

DONE_SIP5_ALIGN            GT_U32    ePortECMPEPortValue;
DONE_SIP5_ALIGN            GT_U32    ePortECMPEPortMask;
DONE_SIP5_ALIGN            GT_U32    ePortECMPLTTIndexBaseEPort;
DONE_SIP5_ALIGN            GT_U32    ePortECMPLFSRConfig;
DONE_SIP5_ALIGN            GT_U32    ePortECMPLFSRSeed;
DONE_SIP5_ALIGN            GT_U32    ePortECMPHashBitSelectionConfig;
DONE_SIP5_ALIGN            GT_U32    trunkLFSRConfig;
DONE_SIP5_ALIGN            GT_U32    trunkLFSRSeed;
DONE_SIP5_ALIGN            GT_U32    trunkHashBitSelectionConfig;

        }L2ECMP;

        struct /*ingrSTCInterrupt*/{

DONE_SIP5_ALIGN            GT_U32    ingrSTCInterruptCause[32]/*Port*/;
DONE_SIP5_ALIGN            GT_U32    ingrSTCInterruptMask[32]/*Port*/;
DONE_SIP5_ALIGN            GT_U32    ingrSTCInterruptSummary;
DONE_SIP5_ALIGN            GT_U32    ingrSTCInterruptSummaryMask;

DONE_SIP5_ALIGN            GT_U32    ingrSTCInterruptSummary1;   /* sip5_20 : handle high 256 ports from ingrSTCInterruptCause */
DONE_SIP5_ALIGN            GT_U32    ingrSTCInterruptSummary1Mask;/* sip5_20 : handle high 256 ports from ingrSTCInterruptMask */
        }ingrSTCInterrupt;

        struct /*ingrSTCConfig*/{

DONE_SIP5_ALIGN            GT_U32    ingrSTCConfig;

        }ingrSTCConfig;

        struct /*ingrForwardingRestrictions*/{

DONE_SIP5_ALIGN            GT_U32    ingrForwardingRestrictionsDroppedPktsCntr;
NEED_TO_BE_USED_BY_SIP5            GT_U32    ingrForwardingRestrictionsDropCode;

        }ingrForwardingRestrictions;

        struct /*ingrDropCntr*/{

DONE_SIP5_ALIGN            GT_U32    ingrDropCntrConfig;
DONE_SIP5_ALIGN            GT_U32    ingrDropCntr;

        }ingrDropCntr;

        struct /*ingrDropCodesCntr*/{

DONE_SIP5_ALIGN            GT_U32    dropCodesCntrConfig;
DONE_SIP5_ALIGN            GT_U32    dropCodesCntr;

        }ingrDropCodesCntr;

        struct /*ePort*/{

DONE_SIP5_ALIGN            GT_U32    ePortGlobalConfig;
DONE_SIP5_ALIGN            GT_U32    protectionLOCWriteMask;
DONE_SIP5_ALIGN            GT_U32    protectionSwitchingRXExceptionConfig;

        }ePort;

        struct /*criticalECCCntrs*/{

NOT_RELEVANT_TO_SIM            GT_U32    criticalECCConfig;
NOT_RELEVANT_TO_SIM            GT_U32    BMClearCriticalECCErrorCntr;
NOT_RELEVANT_TO_SIM            GT_U32    BMClearCriticalECCErrorParams;
NOT_RELEVANT_TO_SIM            GT_U32    BMIncCriticalECCErrorCntr;
NOT_RELEVANT_TO_SIM            GT_U32    BMIncCriticalECCErrorParams;
NOT_RELEVANT_TO_SIM            GT_U32    BMCriticalECCInterruptCause;
NOT_RELEVANT_TO_SIM            GT_U32    BMCriticalECCInterruptMask;

        }criticalECCCntrs;

        struct /*CPUTargetDeviceConfig*/{

DONE_SIP5_ALIGN            GT_U32    CPUTargetDeviceConfigReg0;
DONE_SIP5_ALIGN            GT_U32    CPUTargetDeviceConfigReg1;
DONE_SIP5_ALIGN            GT_U32    CPUTargetDeviceConfigReg2;
DONE_SIP5_ALIGN            GT_U32    CPUTargetDeviceConfigReg3;

        }CPUTargetDeviceConfig;

        struct /*applicationSpecificCPUCodes*/{

DONE_SIP5_ALIGN            GT_U32    IPProtocolCPUCodeEntry[4]/*Protocol*/;
DONE_SIP5_ALIGN            GT_U32    TCPUDPDestPortRangeCPUCodeEntryWord0[16]/*Entry*/;
DONE_SIP5_ALIGN            GT_U32    TCPUDPDestPortRangeCPUCodeEntryWord1[16]/*Entry*/;
DONE_SIP5_ALIGN            GT_U32    IPProtocolCPUCodeValidConfig;

        }applicationSpecificCPUCodes;

    }EQ;

    struct /*LPM*/{
        SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;

DONE_SIP5_ALIGN             GT_U32    globalConfig;
DONE_SIP5_ALIGN             GT_U32    directAccessMode; /* LPM Policy Based Routing Configuration Register */
DONE_SIP5_ALIGN             GT_U32    exceptionStatus;
        GT_U32    LPMInterruptsCause;
        GT_U32    LPMInterruptsMask;
        GT_U32    LPMGeneralIntCause;
        GT_U32    LPMGeneralIntMask;

    }LPM;

    struct /*TCAM*/{
        SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;

        struct /*tcamInterrupts*/{

            GT_U32    TCAMInterruptsSummaryMask;
            GT_U32    TCAMInterruptsSummaryCause;
            GT_U32    TCAMInterruptMask;
            GT_U32    TCAMInterruptCause;
            GT_U32    tcamParityErrorAddr;

        }tcamInterrupts;

        struct /*tcamArrayConf*/{

            GT_U32    tcamActiveFloors;

        }tcamArrayConf;

        struct /*parityDaemonRegs*/{

            GT_U32    parityDaemonCtrl;
            GT_U32    parityDaemonWriteAdressStart;
            GT_U32    parityDaemonWriteAdressEnd;
            GT_U32    parityDaemonWriteAdressJump;
            GT_U32    parityDaemonReadAdressStart;
            GT_U32    parityDaemonReadAdressEnd;
            GT_U32    parityDaemonReadAdressJump;
            GT_U32    parityDaemonDefaultWriteDataX0;
            GT_U32    parityDaemonDefaultWriteDataX1;
            GT_U32    parityDaemonDefaultWriteDataY0;
            GT_U32    parityDaemonDefaultWriteDataX2;
            GT_U32    parityDaemonDefaultWriteDataY1;
            GT_U32    parityDaemonDefaultWriteDataY2;
            GT_U32    parityDaemonStatus;

        }parityDaemonRegs;

        struct /*mgAccess*/{

            GT_U32    mgKeySizeAndTypeGroup[5]/*Tcam Group Num*/;
            GT_U32    mgHitGroupHitNum[5][4]/*Tcam Group Num*//*Hit Number*/;
            GT_U32    mgKeyGroupChunk[5][21]/*Tcam Group Num*//*Chunk Of The Key*/;
            GT_U32    mgCompRequest;
            GT_U32    mgKeyTypeUpdateReq;
            GT_U32    keyTypeData;

        }mgAccess;

        GT_U32    groupClientEnable[5]          /*Tcam Group num*/;
        GT_U32    tcamHitNumAndGroupSelFloor[12]/*Floor num*/;
        GT_U32    tcamGlobalConfig;

    }TCAM;


    struct /*EGF_eft*/{
        SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;

        struct /*MCFIFO*/{

            struct /*MCFIFOConfigs*/{

NOT_RELEVANT_TO_SIM                GT_U32    portToMcFIFOAttributionReg[32]/*reg_offset*/;
DONE_SIP5_ALIGN                GT_U32    portToHemisphereMapReg[16]/*reg offset*/;
NOT_RELEVANT_TO_SIM                GT_U32    MCFIFOGlobalConfigs;
NOT_RELEVANT_TO_SIM                GT_U32    MCFIFOArbiterWeights0;
NOT_RELEVANT_TO_SIM                GT_U32    lossyDropAndPfcUcThreshold;

            }MCFIFOConfigs;

        }MCFIFO;

        struct /*global*/{

NOT_RELEVANT_TO_SIM            GT_U32    cpuPortMapOfReg[128]/*reg_offset*/;
DONE_SIP5_ALIGN            GT_U32    cpuPortDist;
DONE_SIP5_ALIGN            GT_U32    portIsolationLookup0;
DONE_SIP5_ALIGN            GT_U32    portIsolationLookup1;
NOT_RELEVANT_TO_SIM            GT_U32    EFTInterruptsCause;
NOT_RELEVANT_TO_SIM            GT_U32    EFTInterruptsMask;
NOT_RELEVANT_TO_SIM            GT_U32    ECCConf;
NOT_RELEVANT_TO_SIM            GT_U32    ECCStatus;

        }global;

        struct /*egrFilterConfigs*/{

NOT_RELEVANT_TO_SIM            GT_U32    enFCTriggerByCNFrameOnPort[16]/*Port div 32*/;
NOT_RELEVANT_TO_SIM            GT_U32    enCNFrameTxOnPort[16]/*Port div 32*/;
DONE_SIP5_ALIGN            GT_U32    unregedBCFilterEn[16]/*Port div 32*/;
DONE_SIP5_ALIGN            GT_U32    unknownUCFilterEn[16]/*Port div 32*/;
DONE_SIP5_ALIGN            GT_U32    unregedMCFilterEn[16]/*Port div 32*/;
DONE_SIP5_ALIGN            GT_U32    OAMLoopbackFilterEn[16]/*Port div 32*/;
DONE_SIP5_ALIGN            GT_U32    virtual2PhysicalPortRemap[256]/*offset*/;
DONE_SIP5_ALIGN            GT_U32    physicalPortLinkStatusMask[32]/*reg offset*/;
DONE_SIP5_ALIGN            GT_U32    egrFiltersGlobal;
DONE_SIP5_ALIGN            GT_U32    egrFiltersEnable;

        }egrFilterConfigs;

        struct /*deviceMapConfigs*/{

DONE_SIP5_ALIGN            GT_U32    devMapTableAddrConstructProfile[8]/*profile_num*/;
DONE_SIP5_ALIGN            GT_U32    localSrcPortMapOwnDevEn[16]/*Port div 32*/;
DONE_SIP5_ALIGN            GT_U32    localTrgPortMapOwnDevEn[16]/*Port div 32*/;
DONE_SIP5_ALIGN            GT_U32    portAddrConstructMode[256]/*local physical source port*/;

        }deviceMapConfigs;
        struct /*counters*/ {
                           GT_U32    descEccCriticalDropCounter;
                           GT_U32    descEccNonCriticalDropCounter;
NOT_RELEVANT_TO_SIM        GT_U32    eftDescEcc1ErrorCntr;

        }counters;

        struct /*miscellaneous*/ {
                           GT_U32    badAddressLatch;
        }miscellaneous;
    }EGF_eft;

    struct /*EGF_qag*/{
        SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;

        struct /*uniMultiArb*/{

NOT_RELEVANT_TO_SIM            GT_U32    uniMultiCtrl;
NOT_RELEVANT_TO_SIM            GT_U32    uniWeights;
NOT_RELEVANT_TO_SIM            GT_U32    multiWeights;
NOT_RELEVANT_TO_SIM            GT_U32    multiShaperCtrl;

        }uniMultiArb;

        struct /*global*/{

NOT_RELEVANT_TO_SIM            GT_U32    QAGInterruptsCause;
NOT_RELEVANT_TO_SIM            GT_U32    QAGInterruptsMask;

        }global;

        struct /*distributor*/{

DONE_SIP5_ALIGN            GT_U32    useVlanTag1ForTagStateReg[8]/*reg_offset*/;
NEED_TO_BE_USED_BY_SIP5    GT_U32    toAnalyzerLoopback[8]/*Reg Offset*/;
DONE_SIP5_ALIGN            GT_U32    TCRemap[32]/*remapped_tc*/;
DONE_SIP5_ALIGN            GT_U32    stackRemapEn[8]/*Port div 32*/;
NEED_TO_BE_USED_BY_SIP5    GT_U32    perTrgPhyPortLbProfile[16]/*Reg Offset*/;
NEED_TO_BE_USED_BY_SIP5    GT_U32    perSrcPhyPortLbProfile[16]/*Reg Offset*/;
NEED_TO_BE_USED_BY_SIP5    GT_U32    loopbackForwardingGlobalConfig;
NEED_TO_BE_USED_BY_SIP5    GT_U32    loopbackEnableConfig[4];
DONE_SIP5_ALIGN            GT_U32    evlanMirrEnable[8]/*Reg Offset*/;
DONE_SIP5_ALIGN            GT_U32    DPRemap[16]/*remapped_dp*/;
DONE_SIP5_ALIGN            GT_U32    distributorGeneralConfigs;

        }distributor;

    }EGF_qag;

    struct /*EGF_sht*/{
        SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;

        struct /*global*/{

DONE_SIP5_ALIGN            GT_U32    eportStpState[16]/*Port div 8*/;
DONE_SIP5_ALIGN            GT_U32    eportEvlanFilteringEnable[16]/*Port div 32*/;
NOT_RELEVANT_TO_SIM            GT_U32    SHTInterruptsCause;
DONE_SIP5_ALIGN            GT_U32    UCSrcIDFilterEn[16]/*Port div 32*/;
DONE_SIP5_ALIGN            GT_U32    SHTEgrFiltersEnable;
                    GT_U32    MESHIDConfigs;
DONE_SIP5_ALIGN            GT_U32    MCLocalEn[16]/*Port div 32*/;
DONE_SIP5_ALIGN            GT_U32    ignorePhySrcMcFilterEn[16]/*Reg Offset*/;
DONE_SIP5_ALIGN            GT_U32    eportVlanEgrFiltering;
DONE_SIP5_ALIGN            GT_U32    eportStpStateMode[16]/*Port div 32*/;
NEED_TO_BE_USED_BY_SIP5            GT_U32    eportOddOnlySrcIdFilterEnable[16]/*Port div 32*/;
DONE_SIP5_ALIGN            GT_U32    eportRoutedForwardRestricted[16]/*Port div 32*/;
DONE_SIP5_ALIGN            GT_U32    eportPortIsolationMode[32]/*Port div 16*/;
DONE_SIP5_ALIGN            GT_U32    SHTGlobalConfigs;
                    GT_U32    ePortMeshID[64]/*Port div 8*/;
NEED_TO_BE_USED_BY_SIP5            GT_U32    eportIPMCRoutedFilterEn[16]/*Port div 32*/;
DONE_SIP5_ALIGN            GT_U32    eportFromCPUForwardRestricted[16]/*Port div 32*/;
NOT_RELEVANT_TO_SIM            GT_U32    SHTInterruptsMask;
DONE_SIP5_ALIGN            GT_U32    eportBridgedForwardRestricted[16]/*Port div 32*/;
                           GT_U32    eportDropOnEportVid1Mismatch[16]/*Port div 32*/;
                           GT_U32    eportAssociatedVid1[256]/*Port div 2*/;
        }global;

        struct /*miscellaneous*/{
                           GT_U32    badAddressLatch;
        }miscellaneous;
    }EGF_sht;

    struct /*HA*/{
        SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;

DONE_SIP5_ALIGN        GT_U32    invalidPTPHeaderCntr;
DONE_SIP5_ALIGN        GT_U32    TRILLEtherType;
DONE_SIP5_ALIGN        GT_U32    TPIDConfigReg[8]/*entry*/;
DONE_SIP5_ALIGN        GT_U32    routerMACSABaseReg1;
DONE_SIP5_ALIGN        GT_U32    PWCtrlWord[15]/*PW word*/;
        GT_U32    PWETreeEnable;
DONE_SIP5_ALIGN        GT_U32    invalidPTPOutgoingPiggybackCntr;
DONE_SIP5_ALIGN        GT_U32    PTPTimestampTagEthertype;
DONE_SIP5_ALIGN        GT_U32    PTPAndTimestampingExceptionConfig;
DONE_SIP5_ALIGN        GT_U32    MPLSEtherType;
        GT_U32    MPLSChannelTypeProfileReg7;
        GT_U32    MPLSChannelTypeProfileReg[7]/*Reg*/;
DONE_SIP5_ALIGN        GT_U32    IPLengthOffset;
DONE_SIP5_ALIGN        GT_U32    egrUDPPortRangeComparatorConfig[8]/*UDPCmp*/;
DONE_SIP5_ALIGN        GT_U32    invalidPTPIncomingPiggybackCntr;
        GT_U32    egrPolicyDIPSolicitationData1;
DONE_SIP5_ALIGN        GT_U32    ingrAnalyzerVLANTagConfig;
DONE_SIP5_ALIGN        GT_U32    IEtherType;
DONE_SIP5_ALIGN        GT_U32    haGlobalConfig1;
DONE_SIP5_ALIGN        GT_U32    haGlobalConfig;
        GT_U32    HAInterruptMask;
        GT_U32    HAInterruptCause;
DONE_SIP5_ALIGN        GT_U32    ethernetOverGREProtocolType;
DONE_SIP5_ALIGN        GT_U32    routerMACSABaseReg0;
DONE_SIP5_ALIGN        GT_U32    egrTCPPortRangeComparatorConfig[8]/*TCPCmp*/;
DONE_SIP5_ALIGN        GT_U32    egrPolicyDIPSolicitationMask3;
DONE_SIP5_ALIGN        GT_U32    egrPolicyDIPSolicitationMask2;
DONE_SIP5_ALIGN        GT_U32    egrPolicyDIPSolicitationMask1;
DONE_SIP5_ALIGN        GT_U32    egrPolicyDIPSolicitationMask0;
DONE_SIP5_ALIGN        GT_U32    egrPolicyDIPSolicitationData3;
DONE_SIP5_ALIGN        GT_U32    egrPolicyDIPSolicitationData2;
DONE_SIP5_ALIGN        GT_U32    tunnelStartFragmentIDConfig;
DONE_SIP5_ALIGN        GT_U32    egrPolicyDIPSolicitationData0;
DONE_SIP5_ALIGN        GT_U32    egrAnalyzerVLANTagConfig;
        GT_U32    dataECCStatus;
NOT_RELEVANT_TO_SIM        GT_U32    CPIDReg1;
NOT_RELEVANT_TO_SIM        GT_U32    CPIDReg0;
NOT_RELEVANT_TO_SIM        GT_U32    congestionNotificationConfig;
NOT_RELEVANT_TO_SIM        GT_U32    CNMHeaderConfig;
        GT_U32    BPEConfigReg2;
        GT_U32    BPEConfigReg1;

        GT_U32    haBadAddrLatchReg;/*manual added for Sip5_10 : Latch bad address in case of regfile bad address indication.*/
DONE_SIP5_ALIGN        GT_U32    NATConfig;              /*manual added for Sip5_10*/
DONE_SIP5_ALIGN        GT_U32    NATExceptionDropCounter;/*manual added for Sip5_10*/

    }HA;

    struct /*MLL*/{
        SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;

        struct /*multiTargetRateShape*/{

NOT_RELEVANT_TO_SIM            GT_U32    multiTargetRateShapeConfig;
NOT_RELEVANT_TO_SIM            GT_U32    multiTargetRateShapeConfig2;
NOT_RELEVANT_TO_SIM            GT_U32    multiTargetMTUReg;
NOT_RELEVANT_TO_SIM            GT_U32    tokenBucketBaseLine;

        }multiTargetRateShape;

        struct /*mcLinkedListMLLTables*/{

NOT_RELEVANT_TO_SIM            GT_U32    qoSProfileToMultiTargetTCQueuesReg[64]/*Index*/;
NOT_RELEVANT_TO_SIM            GT_U32    extQoSModeMCQueueSelectionReg[2]/*Index*/;

        }mcLinkedListMLLTables;

        struct /*multiTargetVsUcSDWRRAndStrictPriorityScheduler*/{

NOT_RELEVANT_TO_SIM            GT_U32    mcUcSDWRRAndStrictPriorityConfig;

        }multiTargetVsUcSDWRRAndStrictPriorityScheduler;

        struct /*multiTargetTCQueuesAndArbitrationConfig*/{

NOT_RELEVANT_TO_SIM            GT_U32    multitargetTCQueuesGlobalConfig;
NOT_RELEVANT_TO_SIM            GT_U32    multitargetTCQueuesStrictPriorityEnableConfig;
NOT_RELEVANT_TO_SIM            GT_U32    multitargetTCQueuesWeightConfig;

        }multiTargetTCQueuesAndArbitrationConfig;

        struct /*multiTargetEPortMap*/{

DONE_SIP5_ALIGN            GT_U32    multiTargetEPortValue;
DONE_SIP5_ALIGN            GT_U32    multiTargetEPortMask;
DONE_SIP5_ALIGN            GT_U32    multiTargetEPortBase;
DONE_SIP5_ALIGN            GT_U32    ePortToEVIDXBase;

        }multiTargetEPortMap;

        struct /*MLLOutInterfaceCntrs*/{

DONE_SIP5_ALIGN            GT_U32    L2MLLOutInterfaceCntrConfig1[2]/*Counter*/;
DONE_SIP5_ALIGN            GT_U32    L2MLLOutInterfaceCntrConfig[2]/*Counter*/;
DONE_SIP5_ALIGN            GT_U32    L2MLLValidProcessedEntriesCntr[3]/*Counter*/;
DONE_SIP5_ALIGN            GT_U32    L2MLLTTLExceptionCntr;
DONE_SIP5_ALIGN            GT_U32    L2MLLSkippedEntriesCntr;
DONE_SIP5_ALIGN            GT_U32    L2MLLSilentDropCntr;
DONE_SIP5_ALIGN            GT_U32    L2MLLOutMcPktsCntr[2]/*Counter*/;
NOT_RELEVANT_TO_SIM        GT_U32    MLLSilentDropCntr; /* tail drop not supported by simulation */
DONE_SIP5_ALIGN            GT_U32    MLLMCFIFODropCntr;
DONE_SIP5_ALIGN            GT_U32    IPMLLSkippedEntriesCntr;
DONE_SIP5_ALIGN            GT_U32    IPMLLOutMcPktsCntr[2]/*Counter*/;
DONE_SIP5_ALIGN            GT_U32    IPMLLOutInterfaceCntrConfig1[2]/*Counter*/;
DONE_SIP5_ALIGN            GT_U32    IPMLLOutInterfaceCntrConfig[2]/*Counter*/;

        }MLLOutInterfaceCntrs;

        struct /*MLLGlobalCtrl*/{

DONE_SIP5_ALIGN            GT_U32    MLLGlobalCtrl;
DONE_SIP5_ALIGN            GT_U32    MLLLookupTriggerConfig;
DONE_SIP5_ALIGN            GT_U32    globalTTLExceptionConfig;
DONE_SIP5_ALIGN            GT_U32    globalEPortRangeConfig;
DONE_SIP5_ALIGN            GT_U32    globalEPortRangeMask;
DONE_SIP5_ALIGN            GT_U32    IPMLLTableResourceSharingConfig;
DONE_SIP5_ALIGN            GT_U32    L2MLLTableResourceSharingConfig;
DONE_SIP5_ALIGN            GT_U32    MLLInterruptCauseReg;
DONE_SIP5_ALIGN            GT_U32    MLLInterruptMaskReg;
DONE_SIP5_ALIGN            GT_U32    MLLReplicatedTrafficCtrl;
DONE_SIP5_ALIGN            GT_U32    sourceBasedL2MLLFiltering;
NOT_RELEVANT_TO_SIM            GT_U32    MLLEccStatus;
DONE_SIP5_ALIGN            GT_U32    MLLMCSourceID;

        }MLLGlobalCtrl;

    }MLL;


    struct /*PCL*/{
        SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;

        struct /*OAMConfigs*/{

            GT_U32    channelTypeOAMOpcode[15]/*channel type*/;
DONE_SIP5_ALIGN            GT_U32    OAMRDIConfigs;
            GT_U32    OAMMEGLevelModeConfigs;
            GT_U32    OAMRFlagConfigs;
            GT_U32    RBitAssignmentEnable;

        }OAMConfigs;

        struct /*HASH*/{

DONE_SIP5_ALIGN            GT_U32    pearsonHashTable[16]/*index div 4*/;
DONE_SIP5_ALIGN            GT_U32    CRCHashConfig;
DONE_SIP5_ALIGN            GT_U32    CRC32Salt[20]/*n*/;
DONE_SIP5_ALIGN            GT_U32    CRC32Seed;

        }HASH;

DONE_SIP5_ALIGN        GT_U32    ingrPolicyGlobalConfig;
NOT_RELEVANT_TO_SIM    GT_U32    PCLUnitInterruptCause;
NOT_RELEVANT_TO_SIM    GT_U32    PCLUnitInterruptMask;
DONE_SIP5_ALIGN        GT_U32    policyEngineConfig;
DONE_SIP5_ALIGN        GT_U32    policyEngineUserDefinedBytesConfig;
DONE_SIP5_ALIGN        GT_U32    lookupSourceIDMask[3]/*lookup num*/;
DONE_SIP5_ALIGN        GT_U32    policyEngineUserDefinedBytesExtConfig;
        GT_U32    TCAMKeyTypeMap[2]/*reg num*/;
DONE_SIP5_ALIGN        GT_U32    L2L3IngrVLANCountingEnable[16]/*port div 32*/;
DONE_SIP5_ALIGN        GT_U32    countingModeConfig;
DONE_SIP5_ALIGN        GT_U32    CFMEtherType;
DONE_SIP5_ALIGN        GT_U32    pclBadAddressLatchReg;
    }PCL;

    struct /*EPCL*/{
        SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;

DONE_SIP5_ALIGN        GT_U32    egrPolicyGlobalConfig;
        GT_U32    EPCLOAMPktDetection;
        GT_U32    EPCLOAMMEGLevelMode;
DONE_SIP5_ALIGN        GT_U32    EPCLOAMRDI;
NOT_RELEVANT_TO_SIM        GT_U32    EPCLInterruptCause;
NOT_RELEVANT_TO_SIM        GT_U32    EPCLInterruptMask;
        GT_U32    EPCLChannelTypeToOAMOpcodeMap[5]/*Set*/;
        GT_U32    EPCLReservedBitMask;
        GT_U32    TCAMKeyTypeMap;

        GT_U32    EPCLBadAddrLatchReg;
        GT_U32    oamRBbitConfigReg;

    }EPCL;


    struct /*ERMRK*/{
        SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;

        GT_U32    ERMRKGlobalConfig;
        GT_U32    ERMRKInterruptCause;
        GT_U32    ERMRKInterruptMask;
        GT_U32    ERMRKPTPConfig;
        GT_U32    timestampingUDPChecksumMode;
        GT_U32    timestampQueueMsgType;
        GT_U32    timestampQueueEntryID;
        GT_U32    PTPNTPOffset;
        GT_U32    ERMRKUP2UPMapTable[12]/*set*/;
        GT_U32    ERMRKPTPInvalidTimestampCntr;
        GT_U32    ERMRKDP2CFITable;
        GT_U32    ERMRKEXP2EXPMapTable[12]/*set*/;
        GT_U32    timestampQueueEntryWord0[4]/*queue*/;
        GT_U32    timestampQueueEntryWord1[4]/*queue*/;
        GT_U32    timestampQueueEntryWord2[2]/*queue*/;
        GT_U32    timestampQueueEntryIDClear;
        GT_U32    ECCErrorInjection;
        GT_U32    ERMRKBadAddrLatch;

    }ERMRK;


    struct /*OAMUnit*/{
        SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;

        GT_U32    OAMGlobalCtrl;
        GT_U32    agingEntryGroupStatus[2]/*reg*/;
        GT_U32    summaryExceptionGroupStatus[2]/*reg*/;
        GT_U32    txPeriodExceptionGroupStatus[2]/*reg*/;
        GT_U32    sourceInterfaceMismatchCntr;
        GT_U32    OAMGlobalCtrl1;
        GT_U32    RDIStatusChangeExceptionGroupStatus[2]/*reg*/;
        GT_U32    RDIStatusChangeCntr;
        GT_U32    OAMUnitInterruptMask;
        GT_U32    OAMUnitInterruptCause;
        GT_U32    OAMTableAccessData[3]/*Reg*/;
        GT_U32    OAMTableAccessCtrl;
        GT_U32    OAMLossMeasurementOpcodes[4]/*reg*/;
        GT_U32    OAMLMCountedOpcodes[2]/*reg*/;
        GT_U32    OAMKeepAliveOpcodes[4]/*reg*/;
        GT_U32    OAMInvalidKeepaliveExceptionConfigs;
        GT_U32    agingPeriodEntryHigh[8]/*entry*/;
        GT_U32    dualEndedLM;
        GT_U32    OAMExceptionConfigs2;
        GT_U32    OAMExceptionConfigs1;
        GT_U32    OAMExceptionConfigs;
        GT_U32    OAMDelayMeasurementOpcodes[4]/*reg*/;
        GT_U32    MEGLevelGroupStatus[2]/*reg*/;
        GT_U32    MEGLevelExceptionCntr;
        GT_U32    sourceInterfaceGroupStatus[2]/*reg*/;
        GT_U32    invalidKeepaliveHashCntr;
        GT_U32    invalidKeepaliveGroupStatus[2]/*reg*/;
        GT_U32    hashBitsSelection;
        GT_U32    excessKeepaliveGroupStatus[2]/*reg*/;
        GT_U32    excessKeepaliveCntr;
        GT_U32    keepaliveAgingCntr;
        GT_U32    agingPeriodEntryLow[8]/*entry*/;
        GT_U32    txPeriodExceptionCntr;
        GT_U32    summaryExceptionCntr;

    }OAMUnit[2];


    struct /*IPvX*/{
        SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;

        struct /*routerManagementCntrsSets*/{

DONE_SIP5_ALIGN            GT_U32    routerManagementInUcNonRoutedExceptionPktsCntrSet[4]/*set number*/;
DONE_SIP5_ALIGN            GT_U32    routerManagementInMcTrappedMirroredPktCntrSet[4]/*set number*/;
DONE_SIP5_ALIGN            GT_U32    routerManagementInUcPktsCntrSet[4]/*set number*/;
DONE_SIP5_ALIGN            GT_U32    routerManagementInMcPktsCntrSet[4]/*set number*/;
DONE_SIP5_ALIGN            GT_U32    routerManagementInUcNonRoutedNonException[4]/*set number*/;
DONE_SIP5_ALIGN            GT_U32    routerManagementInMcNonRoutedNonExceptionPktsCntrSet[4]/*set number*/;
DONE_SIP5_ALIGN            GT_U32    routerManagementInMcNonRoutedExceptionPktsCntrSet[4]/*set nmber*/;
DONE_SIP5_ALIGN            GT_U32    routerManagementInUcTrappedMirroredPktCntrSet[4]/*set number*/;
DONE_SIP5_ALIGN            GT_U32    routerManagementInMcRPFFailCntrSet[4]/*set number*/;
DONE_SIP5_ALIGN            GT_U32    routerManagementOutUcPktCntrSet[4]/*set number*/;
DONE_SIP5_ALIGN            GT_U32    routerManagementCntrsSetConfig0[4]/*set number*/;
DONE_SIP5_ALIGN            GT_U32    routerManagementCntrsSetConfig1[4]/*set number*/;
            GT_U32    routerManagementIncomingPktCntr;
            GT_U32    routerManagementOutgoingPktCntr;
            GT_U32    routerManagementInSIPFilterCntr;

        }routerManagementCntrsSets;

        struct /*routerGlobalCtrl*/{

DONE_SIP5_ALIGN            GT_U32    routerGlobalCtrl0;
DONE_SIP5_ALIGN            GT_U32    routerMTUConfigReg[4]/*MTU Index*/;
            GT_U32    routerGlobalCtrl1;
            GT_U32    routerGlobalCtrl2;

        }routerGlobalCtrl;

        struct /*routerDropCntr*/{

            GT_U32    routerDropCntr;
            GT_U32    routerDropCntrConfig;

        }routerDropCntr;

        struct /*routerBridgedPktsExceptionCntr*/{

            GT_U32    routerBridgedPktExceptionsCntr;

        }routerBridgedPktsExceptionCntr;

        struct /*IPv6Scope*/{

DONE_SIP5_ALIGN            GT_U32    IPv6UcScopeTableReg[4]/*RegNum*/;
DONE_SIP5_ALIGN            GT_U32    IPv6McScopeTableReg[4]/*RegNum*/;
DONE_SIP5_ALIGN            GT_U32    IPv6UcScopePrefix[4];
DONE_SIP5_ALIGN            GT_U32    IPv6UcScopeLevel[4];

        }IPv6Scope;

        struct /*IPv6GlobalCtrl*/{

DONE_SIP5_ALIGN            GT_U32    IPv6CtrlReg0;
DONE_SIP5_ALIGN            GT_U32    IPv6CtrlReg1;

        }IPv6GlobalCtrl;

        struct /*IPv4GlobalCtrl*/{

DONE_SIP5_ALIGN            GT_U32    IPv4CtrlReg0;
DONE_SIP5_ALIGN            GT_U32    IPv4CtrlReg1;

        }IPv4GlobalCtrl;

        struct /*internalRouter*/{

            GT_U32    routerInterruptCause;
            GT_U32    routerInterruptMask;

        }internalRouter;

        struct /*FCoEGlobalCtrl*/{

DONE_SIP5_ALIGN            GT_U32    routerFCoEGlobalConfig;

        }FCoEGlobalCtrl;

        struct /*ECMPRoutingConfig*/{

DONE_SIP5_ALIGN           GT_U32    ECMPConfig;
            GT_U32    ECMPSeed;

        }ECMPRoutingConfig;

        struct /*CPUCodes*/{

            GT_U32    CPUCodes0;
            GT_U32    CPUCodes1;
            GT_U32    CPUCodes2;
            GT_U32    CPUCodes3;
            GT_U32    CPUCodes4;
            GT_U32    CPUCodes5;
            GT_U32    CPUCodes6;
            GT_U32    CPUCodes7;
            GT_U32    CPUCodes8;
            GT_U32    CPUCodes9;
            GT_U32    CPUCodes10;
            GT_U32    CPUCodes11;

        }CPUCodes;

    }IPvX;

    struct {/*TAI*/
        SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;

        struct /*TAI[9][2]--two TAIs----group of ports served by those TAIs--*/{

                GT_U32    TAIInterruptCause;
                GT_U32    TAIInterruptMask;
                GT_U32    TAICtrlReg0;
                GT_U32    TAICtrlReg1;
                GT_U32    timeCntrFunctionConfig0;
                GT_U32    timeCntrFunctionConfig1;
                GT_U32    timeCntrFunctionConfig2;
                GT_U32    TODStepNanoConfigHigh;
                GT_U32    TODStepNanoConfigLow;
                GT_U32    TODStepFracConfigHigh;
                GT_U32    TODStepFracConfigLow;
                GT_U32    timeAdjustmentPropagationDelayConfigHigh;
                GT_U32    timeAdjustmentPropagationDelayConfigLow;
                GT_U32    triggerGenerationTODSecHigh;
                GT_U32    triggerGenerationTODSecMed;
                GT_U32    triggerGenerationTODSecLow;
                GT_U32    triggerGenerationTODNanoHigh;
                GT_U32    triggerGenerationTODNanoLow;
                GT_U32    triggerGenerationTODFracHigh;
                GT_U32    triggerGenerationTODFracLow;
                GT_U32    timeLoadValueSecHigh;
                GT_U32    timeLoadValueSecMed;
                GT_U32    timeLoadValueSecLow;
                GT_U32    timeLoadValueNanoHigh;
                GT_U32    timeLoadValueNanoLow;
                GT_U32    timeLoadValueFracHigh;
                GT_U32    timeLoadValueFracLow;
                GT_U32    timeCaptureValue0SecHigh;
                GT_U32    timeCaptureValue0SecMed;
                GT_U32    timeCaptureValue0SecLow;
                GT_U32    timeCaptureValue0NanoHigh;
                GT_U32    timeCaptureValue0NanoLow;
                GT_U32    timeCaptureValue0FracHigh;
                GT_U32    timeCaptureValue0FracLow;
                GT_U32    timeCaptureValue1SecHigh;
                GT_U32    timeCaptureValue1SecMed;
                GT_U32    timeCaptureValue1SecLow;
                GT_U32    timeCaptureValue1NanoHigh;
                GT_U32    timeCaptureValue1NanoLow;
                GT_U32    timeCaptureValue1FracHigh;
                GT_U32    timeCaptureValue1FracLow;
                GT_U32    timeCaptureStatus;
                GT_U32    timeUpdateCntr;
                GT_U32    PClkClockCycleConfigLow;
                GT_U32    PClkClockCycleConfigHigh;
                GT_U32    incomingClockInCountingConfigLow;
                GT_U32    incomingClockInCountingConfigHigh;
                GT_U32    generateFunctionMaskSecMed;
                GT_U32    generateFunctionMaskSecLow;
                GT_U32    generateFunctionMaskSecHigh;
                GT_U32    generateFunctionMaskNanoLow;
                GT_U32    generateFunctionMaskNanoHigh;
                GT_U32    generateFunctionMaskFracLow;
                GT_U32    generateFunctionMaskFracHigh;
                GT_U32    externalClockPropagationDelayConfigLow;
                GT_U32    externalClockPropagationDelayConfigHigh;
                GT_U32    driftThresholdConfigLow;
                GT_U32    driftThresholdConfigHigh;
                GT_U32    driftAdjustmentConfigLow;
                GT_U32    driftAdjustmentConfigHigh;
                GT_U32    clockCycleConfigLow;
                GT_U32    clockCycleConfigHigh;
                GT_U32    captureTriggerCntr;

        }TAI[9][2]/*two TAIs*//*group of ports served by those TAIs*/;
    }TAI;

    struct /*GOP*/{
        SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;

        struct /*PTP[72]--Network PTP--*/{

            GT_U32    PTPInterruptCause;
            GT_U32    PTPInterruptMask;
            GT_U32    PTPGeneralCtrl;
            GT_U32    PTPTXTimestampQueue0Reg0;
            GT_U32    PTPTXTimestampQueue0Reg1;
            GT_U32    PTPTXTimestampQueue0Reg2;
            GT_U32    PTPTXTimestampQueue1Reg0;
            GT_U32    PTPTXTimestampQueue1Reg1;
            GT_U32    PTPTXTimestampQueue1Reg2;
            GT_U32    totalPTPPktsCntr;
            GT_U32    PTPv1PktCntr;
            GT_U32    PTPv2PktCntr;
            GT_U32    Y1731PktCntr;
            GT_U32    NTPTsPktCntr;
            GT_U32    NTPReceivePktCntr;
            GT_U32    NTPTransmitPktCntr;
            GT_U32    WAMPPktCntr;
            GT_U32    addCorrectedTimeActionPktCntr;
            GT_U32    NTPPTPOffsetHigh;
            GT_U32    noneActionPktCntr;
            GT_U32    forwardActionPktCntr;
            GT_U32    dropActionPktCntr;
            GT_U32    captureIngrTimeActionPktCntr;
            GT_U32    captureAddTimeActionPktCntr;
            GT_U32    captureAddIngrTimeActionPktCntr;
            GT_U32    captureAddCorrectedTimeActionPktCntr;
            GT_U32    captureActionPktCntr;
            GT_U32    addTimeActionPktCntr;
            GT_U32    addIngrTimeActionPktCntr;
            GT_U32    NTPPTPOffsetLow;

        }PTP[72]/*Network PTP*/;
    }GOP;

    struct /*LMS*/{
        SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;

        struct /*LMS0*/{

            struct /*LMS0Group0*/{

                struct /*SMIConfig*/{

                    GT_U32    PHYAddrReg0forPorts0Through5;
                    GT_U32    PHYAutoNegConfigReg0;
                    GT_U32    SMI0Management;
                    GT_U32    LMS0MiscConfigs;

                }SMIConfig;

                struct /*portsMIBCntrsInterrupts*/{

                    GT_U32    portsGOP0MIBsInterruptCause;
                    GT_U32    portsGOP0MIBsInterruptMask;

                }portsMIBCntrsInterrupts;

                struct /*portsMACSourceAddr*/{

                    GT_U32    sourceAddrMiddle;
                    GT_U32    sourceAddrHigh;

                }portsMACSourceAddr;

                struct /*portsMACMIBCntrs*/{

                    GT_U32    MIBCntrsCtrlReg0forPorts0Through5;

                }portsMACMIBCntrs;

                struct /*portsAndMIBCntrsInterruptSummary*/{

                    GT_U32    MIBCntrsInterruptSummary;

                }portsAndMIBCntrsInterruptSummary;

                struct /*periodicFCRegs*/{

                    GT_U32    gigPortsPeriodicFCCntrMaxValueReg;

                }periodicFCRegs;

                struct /*LEDInterfaceConfig*/{

                    GT_U32    LEDInterface0CtrlReg0forPorts0Through11AndStackPort;
                    GT_U32    LEDInterface0Group01ConfigRegforPorts0Through11;
                    GT_U32    LEDInterface0Class01ManipulationRegforPorts0Through11;
                    GT_U32    LEDInterface0Class4ManipulationRegforPorts0Through11;
                    GT_U32    stackLEDInterface0Class04ManipulationRegforStackPorts;
                    GT_U32    stackLEDInterface0Class59ManipulationRegforStackPorts;
                    GT_U32    LEDInterface0FlexlinkPortsDebugSelectRegforStackPort;
                    GT_U32    LEDInterface0FlexlinkPortsDebugSelectReg1forStackPort;

                }LEDInterfaceConfig;

            }LMS0Group0;

            struct /*LMS0Group1*/{

                struct /*SMIConfig*/{

                    GT_U32    flowCtrlAdvertiseForFiberMediaSelectedConfigReg0forPorts0Through11;
                    GT_U32    PHYAddrReg1forPorts6Through11;
                    GT_U32    PHYAutoNegConfigReg1;

                }SMIConfig;

                struct /*portsMIBCntrsInterrupts*/{

                    GT_U32    triSpeedPortsGOP1MIBsInterruptCause;
                    GT_U32    triSpeedPortsGOP1MIBsInterruptMask;

                }portsMIBCntrsInterrupts;

                struct /*portsMACMIBCntrs*/{

                    GT_U32    MIBCntrsCtrlReg0forPorts6Through11;

                }portsMACMIBCntrs;

                struct /*portsAndMIBCntrsInterruptSummary*/{

                    GT_U32    portsMIBCntrsInterruptSummaryMask;
                    GT_U32    portsInterruptSummaryMask;

                }portsAndMIBCntrsInterruptSummary;

                struct /*periodicFCRegs*/{

                    GT_U32    twoAndHalfGigPortsPeriodicFCCntrMaxValueReg;

                }periodicFCRegs;

                struct /*LEDInterfaceConfig*/{

                    GT_U32    LEDInterface0CtrlReg1AndClass6ManipulationRegforPorts0Through11;
                    GT_U32    LEDInterface0Group23ConfigRegforPorts0Through11;
                    GT_U32    LEDInterface0Class23ManipulationRegforPorts0Through11;
                    GT_U32    LEDInterface0Class5ManipulationRegforPorts0Through11;
                    GT_U32    stackLEDInterface0Class1011ManipulationRegforStackPorts;
                    GT_U32    stackLEDInterface0Group01ConfigRegforStackPort;

                }LEDInterfaceConfig;

                GT_U32    stackPortsModeReg;

            }LMS0Group1;

        }LMS0;

        struct /*LMS1*/{

            struct /*LMS1Group0*/{

                struct /*SMIConfig*/{

                    GT_U32    PHYAddrReg2forPorts12Through17;
                    GT_U32    PHYAutoNegConfigReg2;
                    GT_U32    SMI1Management;
                    GT_U32    LMS1MiscConfigs;

                }SMIConfig;

                struct /*portsMIBCntrsInterrupts*/{

                    GT_U32    portsGOP2MIBsInterruptCause;
                    GT_U32    portsGOP2MIBsInterruptMask;

                }portsMIBCntrsInterrupts;

                struct /*portsMACMIBCntrs*/{

                    GT_U32    MIBCntrsCtrlReg0forPorts12Through17;

                }portsMACMIBCntrs;

                struct /*portsInterruptSummary*/{

                    GT_U32    portsInterruptSummary;

                }portsInterruptSummary;

                struct /*LEDInterfaceConfig*/{

                    GT_U32    LEDInterface1CtrlReg0forPorts12Through23AndStackPort;
                    GT_U32    LEDInterface1Group01ConfigRegforPorts12Through23;
                    GT_U32    LEDInterface1Class01ManipulationRegforPorts12Through23;
                    GT_U32    LEDInterface1Class4ManipulationRegforPorts12Through23;
                    GT_U32    stackLEDInterface1Class04ManipulationRegforStackPorts;
                    GT_U32    stackLEDInterface1Class59ManipulationRegforStackPorts;
                    GT_U32    LEDInterface1FlexlinkPortsDebugSelectRegforStackPort;
                    GT_U32    LEDInterface1FlexlinkPortsDebugSelectReg1forStackPort;

                }LEDInterfaceConfig;

            }LMS1Group0;

            struct /*LMS1Group1*/{

                struct /*SMIConfig*/{

                    GT_U32    flowCtrlAdvertiseForFiberMediaSelectedConfigReg1forPorts12Through23;
                    GT_U32    PHYAddrReg3forPorts18Through23;
                    GT_U32    PHYAutoNegConfigReg3;

                }SMIConfig;

                struct /*portsMIBCntrsInterrupts*/{

                    GT_U32    portsGOP3AndStackMIBsInterruptCause;
                    GT_U32    portsGOP3AndStackMIBsInterruptMask;

                }portsMIBCntrsInterrupts;

                struct /*portsMACMIBCntrs*/{

                    GT_U32    MIBCntrsCtrlReg0forPorts18Through23;

                }portsMACMIBCntrs;

                struct /*periodicFCRegs*/{

                    GT_U32    stackPorts1GPeriodicFCCntrMaxValueReg;

                }periodicFCRegs;

                struct /*LEDInterfaceConfig*/{

                    GT_U32    LEDInterface1CtrlReg1AndClass6ManipulationforPorts12Through23;
                    GT_U32    LEDInterface1Group23ConfigRegforPorts12Through23;
                    GT_U32    LEDInterface1Class23ManipulationRegforPorts12Through23;
                    GT_U32    LEDInterface1Class5ManipulationRegforPorts12Through23;
                    GT_U32    stackLEDInterface1Class1011ManipulationRegforStackPort;
                    GT_U32    stackLEDInterface1Group01ConfigRegforStackPort;

                }LEDInterfaceConfig;

            }LMS1Group1;

        }LMS1;

    }LMS[3];

    struct /*PLR[SMEM_SIP5_PP_PLR_UNIT__LAST__E]*/{
        SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;

DONE_SIP5_ALIGN        GT_U32    policerInitialDP[64]/*QoSProfile*/;
DONE_SIP5_ALIGN        GT_U32    policerCtrl0;
DONE_SIP5_ALIGN        GT_U32    policerCtrl1;
DONE_SIP5_ALIGN        GT_U32    globalBillingCntrIndexingModeConfig0;
        GT_U32    policerScanCtrl;
DONE_SIP5_ALIGN        GT_U32    policerPortMeteringReg[16]/*port/32*/;
        GT_U32    policerPerPortRemarkTableID[32]/*Port/16*/;
DONE_SIP5_ALIGN        GT_U32    policerOAM;
        GT_U32    policerMRU;
        GT_U32    IPFIXSecondsMSbTimerStampUpload;
        GT_U32    policerCountingBaseAddr;
        GT_U32    policerMeteringBaseAddr;
DONE_SIP5_ALIGN        GT_U32    policerIPFIXPktCountWAThreshold;
DONE_SIP5_ALIGN        GT_U32    policerIPFIXDroppedPktCountWAThreshold;
        GT_U32    policerIPFIXByteCountWAThresholdMSB;
DONE_SIP5_ALIGN        GT_U32    policerIPFIXByteCountWAThresholdLSB;
        GT_U32    policerInterruptMask;
DONE_SIP5_ALIGN        GT_U32    policerInterruptCause;
        GT_U32    policerTableAccessData[8]/*Reg*/;
        GT_U32    IPFIXSampleEntriesLog1;
        GT_U32    policerError;
        GT_U32    policerErrorCntr;
        GT_U32    meteringAndCountingRAMSize0;
        GT_U32    meteringAndCountingRAMSize1;
        GT_U32    meteringAndCountingRAMSize2;
        GT_U32    policerTableAccessCtrl;
        GT_U32    policerMeteringRefreshScanRateLimit;
        GT_U32    policerMeteringRefreshScanAddrRange;
        GT_U32    IPFIXSecondsLSbTimerStampUpload;
        GT_U32    policerShadow[2]/*RegNum*/;
DONE_SIP5_ALIGN        GT_U32    IPFIXSampleEntriesLog0;
DONE_SIP5_ALIGN        GT_U32    IPFIXNanoTimerStampUpload;
DONE_SIP5_ALIGN        GT_U32    IPFIXCtrl;
DONE_SIP5_ALIGN        GT_U32    hierarchicalPolicerCtrl;
DONE_SIP5_ALIGN        GT_U32    globalBillingCntrIndexingModeConfig1;
        GT_U32    portAndPktTypeTranslationTable[512]/*port<perPort>*/;
        GT_U32    policerBadAddressLatch;

        /*sip5_15*/
        GT_U32    statisticalMeteringConfig0;
        GT_U32    statisticalMeteringConfig1;
        GT_U32    meteringAddressingModeConfiguration0;
        GT_U32    meteringAddressingModeConfiguration1;
        GT_U32    mef10_3_bucket0_max_rate_tab; /* 128 regs*/
        GT_U32    mef10_3_bucket1_max_rate_tab; /* 128 regs */
        GT_U32    qosProfileToPriority_tab; /* 128 regs each (8 * 4bits) */

    }PLR[SMEM_SIP5_PP_PLR_UNIT__LAST__E];


    struct /*CNC*/{
        SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;

        struct /*perBlockRegs*/{

            struct /*wraparound*/{

                GT_U32    CNCBlockWraparoundStatusReg[16][4]/*Block*//*Entry*/;

            }wraparound;

            struct /*rangeCount*/{

                GT_U32    CNCBlockRangeCountEnable[2][16]/*group of ranges*//*block*/;

            }rangeCount;

            struct /*clientEnable*/{

                GT_U32    CNCBlockConfigReg0[16]/*Block*/;

            }clientEnable;

            struct /*entryMode*/
            {
                GT_U32    CNCBlocksCounterEntryModeRegister[2]/*Block/8*/;
            }entryMode;

        }perBlockRegs;

        struct /*globalRegs*/{

            GT_U32    CNCGlobalConfigReg;
            GT_U32    CNCFastDumpTriggerReg;
            GT_U32    CNCClearByReadValueRegWord0;
            GT_U32    CNCClearByReadValueRegWord1;
            GT_U32    CNCInterruptSummaryCauseReg;
            GT_U32    CNCInterruptSummaryMaskReg;
            GT_U32    wraparoundFunctionalInterruptCauseReg;
            GT_U32    wraparoundFunctionalInterruptMaskReg;
            GT_U32    rateLimitFunctionalInterruptCauseReg;
            GT_U32    rateLimitFunctionalInterruptMaskReg;
            GT_U32    miscFunctionalInterruptCauseReg;
            GT_U32    miscFunctionalInterruptMaskReg;

        }globalRegs;

    }CNC[2]/*CNC number*/;

    struct /*TXQ*/{

        struct /*queue*/{
            SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;

            struct /*tailDrop*/{

                struct /*tailDropLimits*/{

                    GT_U32    poolBufferLimits[8]/*Priority*/;
                    GT_U32    poolDescLimits[8]/*Priority*/;
                    GT_U32    profilePortDescLimits[16]/*profile*/;
                    GT_U32    profilePortBufferLimits[16]/*profile*/;
                    GT_U32    globalDescsLimit;
                    GT_U32    globalBufferLimit;

                }tailDropLimits;

                struct /*tailDropCntrs*/{

                    GT_U32    portBuffersCntr[72]/*Port*/;
                    GT_U32    portDescCntr[72]/*Port*/;
                    GT_U32    priorityDescCntr[8]/*TC*/;
                    GT_U32    priorityBuffersCntr[8]/*TC*/;
                    GT_U32    totalDescCntr;
                    GT_U32    totalBuffersCntr;

                }tailDropCntrs;

                struct /*tailDropConfig*/{

                    GT_U32    portTailDropCNProfile[72]/*Port*/;
                    GT_U32    portEnqueueEnable[72]/*Port*/;
                    GT_U32    profileTailDropConfig[16]/*Profile*/;
                    GT_U32    profilePriorityQueueToPoolAssociation[16]/*profile*/;
                    GT_U32    profilePriorityQueueEnablePoolUsage[8]/*Priority*/;
                    GT_U32    profilePoolWRTDEn[16]/*profile*/;
                    GT_U32    profileQueueWRTDEn[16]/*profile*/;
                    GT_U32    profilePortWRTDEn[16]/*profile*/;
                    GT_U32    profileMaxQueueWRTDEn[16]/*profile*/;
                    GT_U32    profileECNMarkEn[16]/*Profile*/;
                    GT_U32    profileCNMTriggeringEnable[16]/*Profile*/;
                    GT_U32    globalTailDropConfig;
                    GT_U32    pktLengthForTailDropEnqueue;
                    GT_U32    maxQueueWRTDMasks;
                    GT_U32    portWRTDMasks;
                    GT_U32    queueWRTDMasks;
                    GT_U32    poolWRTDMasks;

                }tailDropConfig;

                struct /*resourceHistogram*/{

                    struct /*resourceHistogramLimits*/{

                        GT_U32    resourceHistogramLimitReg1;
                        GT_U32    resourceHistogramLimitReg2;

                    }resourceHistogramLimits;

                    struct /*resourceHistogramCntrs*/{

                        GT_U32    resourceHistogramCntr[4]/*counter*/;

                    }resourceHistogramCntrs;

                }resourceHistogram;

                struct /*mcFilterLimits*/{

                    GT_U32    mirroredPktsToAnalyzerPortDescsLimit;
                    GT_U32    ingrMirroredPktsToAnalyzerPortBuffersLimit;
                    GT_U32    egrMirroredPktsToAnalyzerPortBuffersLimit;
                    GT_U32    mcDescsLimit;
                    GT_U32    mcBuffersLimit;

                }mcFilterLimits;

                struct /*muliticastFilterCntrs*/{

                    GT_U32    snifferDescsCntr;
                    GT_U32    ingrSnifferBuffersCntr;
                    GT_U32    egrSnifferBuffersCntr;
                    GT_U32    mcDescsCntr;
                    GT_U32    mcBuffersCntr;

                }muliticastFilterCntrs;

                struct /*FCUAndQueueStatisticsLimits*/{

                    GT_U32    XONLimit[8]/*SharedLimit*/;
                    GT_U32    XOFFLimit[8]/*SharedLimit*/;
                    GT_U32    queueXONLimitProfile[16]/*Profile*/;
                    GT_U32    queueXOFFLimitProfile[16]/*Profile*/;
                    GT_U32    portXONLimitProfile[16]/*Profile*/;
                    GT_U32    portXOFFLimitProfile[16]/*Profile*/;
                    GT_U32    globalXONLimit;
                    GT_U32    globalXOFFLimit;
                    GT_U32    FCUMode;

                }FCUAndQueueStatisticsLimits;

            }tailDrop;

            struct /*global*/{

                struct /*interrupt*/{

                    struct /*queueStatistics*/{

                        GT_U32    lowCrossingStatusReg[18]/*Index*/;
                        GT_U32    highCrossingInterruptMaskPort[72]/*Port*/;
                        GT_U32    lowCrossingInterruptSummaryMaskPorts61To31;
                        GT_U32    lowCrossingInterruptSummaryMaskPorts30To0;
                        GT_U32    lowCrossingInterruptSummaryCausePorts71To62;
                        GT_U32    lowCrossingInterruptSummaryCausePorts61To31;
                        GT_U32    lowCrossingInterruptSummaryCausePorts30To0;
                        GT_U32    lowCrossingInterruptMaskPort[72]/*Port*/;
                        GT_U32    lowCrossingInterruptCausePort[72]/*Port*/;
                        GT_U32    highCrossingStatusReg[18]/*Index*/;
                        GT_U32    highCrossingInterruptSummaryMaskPorts71To62;
                        GT_U32    lowCrossingInterruptSummaryMaskPorts71To62;
                        GT_U32    highCrossingInterruptSummaryCausePorts30To0;
                        GT_U32    highCrossingInterruptSummaryMaskPorts30To0;
                        GT_U32    highCrossingInterruptSummaryCausePorts61To31;
                        GT_U32    highCrossingInterruptSummaryMaskPorts61To31;
                        GT_U32    highCrossingInterruptSummaryCausePorts71To62;
                        GT_U32    highCrossingInterruptCausePort[72]/*Port*/;

                    }queueStatistics;

                    GT_U32    transmitQueueInterruptSummaryCause;
                    GT_U32    transmitQueueInterruptSummaryMask;
                    GT_U32    generalInterrupt1Cause;
                    GT_U32    generalInterrupt1Mask;
                    GT_U32    portDescFullInterruptCause0;
                    GT_U32    portDescFullInterruptMask0;
                    GT_U32    portDescFullInterruptCause1;
                    GT_U32    portDescFullInterruptMask1;
                    GT_U32    portDescFullInterruptCause2;
                    GT_U32    portDescFullInterruptMask2;
                    GT_U32    portBufferFullInterruptCause0;
                    GT_U32    portBufferFullInterruptMask0;
                    GT_U32    portBufferFullInterruptCause1;
                    GT_U32    portBufferFullInterruptMask1;
                    GT_U32    portBufferFullInterruptCause2;
                    GT_U32    portBufferFullInterruptMask2;
                    GT_U32    txqQueueBadAddressLatch;
                }interrupt;

                struct /*ECCCntrs*/{

                    GT_U32    TDClrECCErrorCntr;

                }ECCCntrs;

                struct /*dropCntrs*/{

                    GT_U32    clearPktsDroppedCntrPipe[4]/*Pipe*/;

                }dropCntrs;

                struct /*CPFC Interface*/{

                    GT_U32    cpuPortCPFCCountConfiguration;
                    GT_U32    cpuPortsNumber;

                }CPFCInterface;

            }global;

            struct /*peripheralAccess*/{

                struct /*peripheralAccessMisc*/{

                    GT_U32    portGroupDescToEQCntr[4]/*port group*/;
                    GT_U32    peripheralAccessMisc;
                    GT_U32    QCNIncArbiterCtrl;

                }peripheralAccessMisc;

                struct /*egrMIBCntrs*/{

                    GT_U32    txQMIBCntrsPortSetConfig[2]/*Set*/;
                    GT_U32    txQMIBCntrsSetConfig[2]/*Set*/;
                    GT_U32    setTailDroppedPktCntr[2]/*Set*/;
                    GT_U32    setOutgoingUcPktCntr[2]/*Set*/;
                    GT_U32    setOutgoingMcPktCntr[2]/*Set*/;
                    GT_U32    setOutgoingBcPktCntr[2]/*Set*/;
                    GT_U32    setMcFIFO7_4DroppedPktsCntr[2]/*Set*/;
                    GT_U32    setMcFIFO3_0DroppedPktsCntr[2]/*Set*/;
                    GT_U32    setCtrlPktCntr[2]/*Set*/;
                    GT_U32    setBridgeEgrFilteredPktCntr[2]/*Set*/;

                }egrMIBCntrs;

                struct /*CNCModes*/{

                    GT_U32    CNCModes;

                }CNCModes;

            }peripheralAccess;

        }queue;

        struct /*ll*/{
            SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;

            struct /*global*/{

                struct /*portsLinkEnableStatus*/{

                    GT_U32    portsLinkEnableStatus[18];/*port/32*/

                }portsLinkEnableStatus;

                struct /*interrupts*/{

                    GT_U32    LLInterruptCause;
                    GT_U32    LLInterruptMask;
                    GT_U32    LLECCInterruptCause;
                    GT_U32    LLECCInterruptMask;
                    GT_U32    txqLLBadAddressLatch;

                }interrupts;

                struct /*globalLLConfig*/{

                    GT_U32    profilePriorityQueueToPoolAssociation[16]/*profile*/;
                    GT_U32    portProfile[576]/*Port*/;
                    GT_U32    highSpeedPort[16]/*port*/;

                }globalLLConfig;

                struct /* LLECC */ {

                    GT_U32    llECCControlAndStatusBank[38];
                    GT_U32    llECCErrorCounterBank[38];

                }LLECC;

            }global;

        }ll;


        struct /*pfc*/{
            SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;

            struct /*PFCInterrupts*/{

                GT_U32    PFCPortGroupGlobalOverUnderflowInterruptCause;
                GT_U32    PFCPortGroupGlobalOverUnderflowInterruptMask;
                GT_U32    PFCInterruptSummaryCause;
                GT_U32    PFCInterruptSummaryMask;
                GT_U32    PFCParityInterruptCause;
                GT_U32    PFCCntrsOverUnderFlowInterruptMask;
                GT_U32    PFCParityInterruptMask;
                GT_U32    PFCFIFOsOverrunsInterruptCause;
                GT_U32    PFCFIFOsOverrunsInterruptMask;
                GT_U32    PFCMsgsSetsOverrunsInterruptCause;
                GT_U32    PFCMsgsSetsOverrunsInterruptMask;
                GT_U32    PFCCntrsOverUnderFlowInterruptCause;

            }PFCInterrupts;

            GT_U32    LLFCTargetPortRemap[64]/*Port div 4*/;
            GT_U32    portFCMode[8]/*Port div 32*/;
            GT_U32    PFCSourcePortToPFCIndexMap[256]/*Global Port*/;
            GT_U32    PFCPortProfile[32]/*Port div 8*/;
            GT_U32    PFCPortGroupFlowCtrlIndicationFIFOOverrunEntry0[1]/*port group*/;
            GT_U32    PFCPortGroupFlowCtrlIndicationFIFOOverrunEntry1[1]/*port group*/;
            GT_U32    PFCPortGroupFlowCtrlIndicationFIFOOverrunCntr[1]/*port group*/;
            GT_U32    PFCPortGroupEqFIFOOverrunCntr[1]/*port group*/;
            GT_U32    PFCPortGroupCntrsStatus[1]/*port group*/;
            GT_U32    PFCPortGroupCntrsParityErrorsCntr[1]/*port group*/;
            GT_U32    PFCGroupOfPortsTCXonThresholds[8][1]/*tc*//*pipe*/;
            GT_U32    PFCGroupOfPortsTCXoffThresholds[8][1]/*tc*//*pipe*/;
            GT_U32    PFCGroupOfPortsTCDropThresholds[8][1]/*tc*//*pipe*/;
            GT_U32    PFCGroupOfPortsTCCntr[1][8]/*port group*//*tc*/;
            GT_U32    PFCTriggerGlobalConfig;

        }pfc;

        struct /*qcn*/{
            SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;

            GT_U32    CCFCSpeedIndex[9]/*Reg offset*/;
            GT_U32    CNSampleTbl[8]/*qFb 3 msb*/;
            GT_U32    profilePriorityQueueCNThreshold[8][8]/*profile*//*priority*/;
            GT_U32    feedbackMIN;
            GT_U32    feedbackMAX;
            GT_U32    CNGlobalConfig;
            GT_U32    enFCTriggerByCNFrameOnPort[8]/*Port div 32*/;
            GT_U32    enCNFrameTxOnPort[8]/*Port div 32*/;
            GT_U32    feedbackCalcConfigs;
            GT_U32    txqECCConf;
            GT_U32    descECCSingleErrorCntr;
            GT_U32    descECCDoubleErrorCntr;
            GT_U32    CNTargetAssignment;
            GT_U32    CNDropCntr;
            GT_U32    QCNInterruptCause;
            GT_U32    QCNInterruptMask;
            GT_U32    CNDescAttributes;
            GT_U32    CNBufferFIFOParityErrorsCntr;
            GT_U32    CNBufferFIFOOverrunsCntr;
            GT_U32    ECCStatus;

        }qcn;


        struct /*dq*/{
            SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;
#define MAX_DQ_NUM_PORTS_CNS        SIP_5_20_DQ_NUM_PORTS_CNS
#define SIP_5_20_DQ_NUM_PORTS_CNS   96/* 96 ports per DQ (BC3 with 6 DQs) */
#define SIP_5_DQ_NUM_PORTS_CNS      72/* 72 ports per DQ (BC2,bobk with single DQ) */

            struct /*statisticalAndCPUTrigEgrMirrToAnalyzerPort*/{

                GT_U32    egrAnalyzerEnableIndex[32]/*index*/;
                GT_U32    STCStatisticalTxSniffConfig;
                GT_U32    egrCTMTrigger;
                GT_U32    CTMAnalyzerIndex;
                GT_U32    STCAnalyzerIndex;

            }statisticalAndCPUTrigEgrMirrToAnalyzerPort;

            struct /*shaper*/{

                GT_U32    portTokenBucketMode[MAX_DQ_NUM_PORTS_CNS]/*Port*/;
                GT_U32    portDequeueEnable[MAX_DQ_NUM_PORTS_CNS]/*Port*/;
                GT_U32    tokenBucketUpdateRate;
                GT_U32    tokenBucketBaseLine;
                GT_U32    CPUTokenBucketMTU;
                GT_U32    portsTokenBucketMTU;

            }shaper;

            struct /*scheduler*/{

                struct /*schedulerConfig*/{

                    GT_U32    conditionalMaskForPort[MAX_DQ_NUM_PORTS_CNS]/*Port*/;
                    GT_U32    schedulerConfig;
                    GT_U32    pktLengthForTailDropDequeue;
                    GT_U32    highSpeedPorts[2];

                }schedulerConfig;

                struct /*priorityArbiterWeights*/{

                    GT_U32    profileSDWRRGroup[16]/*Profile*/;
                    GT_U32    profileSDWRRWeightsConfigReg0[16]/*Profile*/;
                    GT_U32    profileSDWRRWeightsConfigReg1[16]/*Profile*/;
                    GT_U32    profileSDWRREnable[16]/*Profile*/;
                    GT_U32    portSchedulerProfile[MAX_DQ_NUM_PORTS_CNS]/*Port*/;

                }priorityArbiterWeights;

                struct /*portShaper*/{

                    GT_U32    portRequestMaskSelector[MAX_DQ_NUM_PORTS_CNS]/*Port*/;
                    GT_U32    portRequestMask;
                    GT_U32    fastPortShaperThreshold;

                }portShaper;

                struct /*portArbiterConfig*/{

                    GT_U32    portsArbiterPortWorkConservingEnable[MAX_DQ_NUM_PORTS_CNS]/*port*/;
                    GT_U32    portsArbiterMap[85]/*Slice_Group*/;
                    GT_U32    portsArbiterConfig;
                    GT_U32    portsArbiterStatus;

                }portArbiterConfig;

            }scheduler;

            struct /*global*/{

                struct /*memoryParityError*/{

                    GT_U32    tokenBucketPriorityParityErrorCntr;
                    GT_U32    parityErrorBadAddr;

                }memoryParityError;

                struct /*interrupt*/{

                    GT_U32    flushDoneInterruptCause[4]/*pipe*/;
                    GT_U32    flushDoneInterruptMask[4]/*pipe*/;
                    GT_U32    egrSTCInterruptCause[4]/*pipe*/;
                    GT_U32    egrSTCInterruptMask[4]/*pipe*/;
                    GT_U32    txQDQInterruptSummaryCause;
                    GT_U32    txQDQInterruptSummaryMask;
                    GT_U32    txQDQMemoryErrorInterruptCause;
                    GT_U32    txQDQMemoryErrorInterruptMask;
                    GT_U32    txQDQGeneralInterruptCause;
                    GT_U32    txQDQGeneralInterruptMask;
                    GT_U32    txQDQBadAddressLatch;

                }interrupt;

                struct /*globalDQConfig*/{

                    GT_U32    portToDMAMapTable[MAX_DQ_NUM_PORTS_CNS]/*Port*/;
                    GT_U32    profileByteCountModification[16]/*Profile*/;
                    GT_U32    portBCConstantValue[MAX_DQ_NUM_PORTS_CNS]/*Port*/;
                    GT_U32    globalDequeueConfig;
                    GT_U32    BCForCutThrough;

                }globalDQConfig;

                struct /*flushTrig*/{

                    GT_U32    portTxQFlushTrigger[MAX_DQ_NUM_PORTS_CNS]/*Port*/;

                }flushTrig;

                struct /*ECCCntrs*/{

                    GT_U32    DQIncECCErrorCntr;
                    GT_U32    DQClrECCErrorCntr;

                }ECCCntrs;

                struct /*dropCntrs*/{

                    GT_U32    egrMirroredDroppedCntr;
                    GT_U32    STCDroppedCntr;

                }dropCntrs;

                struct /* creditCounters */{
                    GT_U32    txDmaPortCreditCntr[74];
                }creditCounters;


            }global;

            struct /*flowCtrlResponse*/{

                GT_U32    schedulerProfilePFCTimerToPriorityQueueMapEnable[16]/*profile*/;
                GT_U32    schedulerProfilePriorityQueuePFCFactor[16][8]/*Profile*//*TC*/;
                GT_U32    schedulerProfileLLFCXOFFValue[16]/*profile*/;
                GT_U32    portToTxQPortMap[256]/*Source Port*/;
                GT_U32    PFCTimerToPriorityQueueMap[8]/*TC*/;
                GT_U32    flowCtrlResponseConfig;

            }flowCtrlResponse;

        }dq[3];/* BC3 hold 3 DQ per pipe (total 6) */
    }TXQ;

    struct /*TMDROP*/
    {
        SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;

        GT_U32    portsInterface[8]/*32 ports*/;
        GT_U32    TMDROPTMTCMap8To87[2]/*8 TM TC*/;
        GT_U32    globalConfigs;
        GT_U32    TMDROPInterruptCause;

        GT_U32    TMDROPInterruptMask;
    }TMDROP;

    struct /*TMQMAP*/
    {
        SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;

        GT_U32    TC2TCMap8To16[6]/*8 TC2TC addresses*/;
        GT_U32    TMQMAPGlobalConfig;
        GT_U32    TMQMAPInterruptCause;
        GT_U32    TMQMAPInterruptMask;

    }TMQMAP;

    struct /*TM_INGR_GLUE*/{
        SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;

        GT_U32    globalConfig;
        GT_U32    debug;
        GT_U32    TMIngrGlueInterruptCause;
        GT_U32    TMIngrGlueInterruptMask;

    }TM_INGR_GLUE;

    struct /*TM_EGR_GLUE*/
    {
        SMEM_CHT_PP_REGS_UNIT_START_INFO_STC    startUnitInfo;

        GT_U32    statisticalAgingTimeCntrMsb[100]/*statistical aging set*/;
        GT_U32    statisticalAgingTimeCntrLsb[100]/*statistical aging set*/;
        GT_U32    TMEgrGlueInterruptMask;
        GT_U32    TMEgrGlueInterruptCause;
        GT_U32    statisticalMinAgeTime[100]/*statistical aging set*/;
        GT_U32    statisticalMinAgeTimeShadow;
        GT_U32    statisticalMaxAgeTime[100]/*statistical aging set*/;
        GT_U32    statisticalMaxAgeTimeShadow;
        GT_U32    statisticalAgingTimeCntrsWraparoundInterruptMask0;
        GT_U32    statisticalAgingTimeCntrsWraparoundInterruptMask1;
        GT_U32    statisticalAgingTimeCntrsWraparoundInterruptMask2;
        GT_U32    statisticalAgingTimeCntrsWraparoundInterruptMask3;
        GT_U32    statisticalAgingTimeCntrsWraparoundInterruptCause0;
        GT_U32    statisticalAgingTimeCntrsWraparoundInterruptCause1;
        GT_U32    statisticalAgingTimeCntrsWraparoundInterruptCause2;
        GT_U32    statisticalAgingTimeCntrsWraparoundInterruptCause3;
        GT_U32    statisticalAgingTimeCntrsMsbShadow;
        GT_U32    statisticalAgingTimeCntrsLsbShadow;
        GT_U32    TMEgrGlueSummaryCause;
        GT_U32    TMEgrGlueSummaryMask;
        GT_U32    statisticalAgingQueueID[100]/*statistical aging set*/;
        GT_U32    statisticalAgingPktCntrsWraparoundInterruptMask0;
        GT_U32    statisticalAgingPktCntrsWraparoundInterruptMask1;
        GT_U32    statisticalAgingPktCntrsWraparoundInterruptMask2;
        GT_U32    statisticalAgingPktCntrsWraparoundInterruptMask3;
        GT_U32    statisticalAgingPktCntrsWraparoundInterruptCause0;
        GT_U32    statisticalAgingPktCntrsWraparoundInterruptCause1;
        GT_U32    statisticalAgingPktCntrsWraparoundInterruptCause2;
        GT_U32    statisticalAgingPktCntrsWraparoundInterruptCause3;
        GT_U32    statisticalAgingPktCntr[100]/*statistical aging set*/;
        GT_U32    statisticalAgingOctetCntrsWraparoundInterruptMask0;
        GT_U32    statisticalAgingOctetCntrsWraparoundInterruptMask1;
        GT_U32    statisticalAgingOctetCntrsWraparoundInterruptMask2;
        GT_U32    statisticalAgingOctetCntrsWraparoundInterruptMask3;
        GT_U32    statisticalAgingOctetCntrsWraparoundInterruptCause0;
        GT_U32    statisticalAgingOctetCntrsWraparoundInterruptCause1;
        GT_U32    statisticalAgingOctetCntrsWraparoundInterruptCause2;
        GT_U32    statisticalAgingOctetCntrsWraparoundInterruptCause3;
        GT_U32    statisticalAgingOctetCntrsMsbShadow;
        GT_U32    statisticalAgingOctetCntrsLsbShadow;
        GT_U32    statisticalAgingOctetCntrMsb[100]/*statistical aging set*/;
        GT_U32    statisticalAgingOctetCntrLsb[100]/*statistical aging set*/;
        GT_U32    statisticalAgingAgedOutPktCntrsWraparoundInterruptMask0;
        GT_U32    statisticalAgingAgedOutPktCntrsWraparoundInterruptMask1;
        GT_U32    statisticalAgingAgedOutPktCntrsWraparoundInterruptMask2;
        GT_U32    statisticalAgingAgedOutPktCntrsWraparoundInterruptMask3;
        GT_U32    statisticalAgingAgedOutPktCntrsWraparoundInterruptCause0;
        GT_U32    statisticalAgingAgedOutPktCntrsWraparoundInterruptCause1;
        GT_U32    statisticalAgingAgedOutPktCntrsWraparoundInterruptCause2;
        GT_U32    statisticalAgingAgedOutPktCntrsWraparoundInterruptCause3;
        GT_U32    statisticalAgingAgedOutPktCntrsShadow;
        GT_U32    statisticalAgingAgedOutPktCntr[100]/*statistical aging set*/;
        GT_U32    queueTimerConfig;
        GT_U32    queueTimer;
        GT_U32    portInterface0;
        GT_U32    portInterface1;
        GT_U32    portInterface2;
        GT_U32    perPortAgedOutPktCntr[192]/*TmPort*/;
        GT_U32    perPortAgedOutPktCntrsWraparoundInterruptMask0;
        GT_U32    perPortAgedOutPktCntrsWraparoundInterruptMask1;
        GT_U32    perPortAgedOutPktCntrsWraparoundInterruptMask2;
        GT_U32    perPortAgedOutPktCntrsWraparoundInterruptMask3;
        GT_U32    perPortAgedOutPktCntrsWraparoundInterruptMask4;
        GT_U32    perPortAgedOutPktCntrsWraparoundInterruptMask5;
        GT_U32    perPortAgedOutPktCntrsWraparoundInterruptMask6;
        GT_U32    perPortAgedOutPktCntrsWraparoundInterruptCause0;
        GT_U32    perPortAgedOutPktCntrsWraparoundInterruptCause1;
        GT_U32    perPortAgedOutPktCntrsWraparoundInterruptCause2;
        GT_U32    perPortAgedOutPktCntrsWraparoundInterruptCause3;
        GT_U32    perPortAgedOutPktCntrsWraparoundInterruptCause4;
        GT_U32    perPortAgedOutPktCntrsWraparoundInterruptCause5;
        GT_U32    perPortAgedOutPktCntrsWraparoundInterruptCause6;
        GT_U32    globalConfig;
        GT_U32    ECCStatus;
        GT_U32    dropPortAllOutgoingPkts[192]/*TmPort*/;
        GT_U32    dropAgedPktCntrConfig;
        GT_U32    dropAgedPktCntr;
        GT_U32    debug;

    }TM_EGR_GLUE;


}SMEM_SIP5_PP_REGS_ADDR_STC;

typedef struct{
    struct /*resetAndInitCtrller*/{

        struct /*XSB_XBAR*/{

            struct /*DRAMCtrllerRegsDDR3*/{

                struct /*dunitRegs*/{

                    struct /*SDRAMScratchPad*/{

                        struct /*D_Domain_Regs*/{

                            GT_U32    scratchPad0;
                            GT_U32    scratchPad1;
                            GT_U32    scratchPad2;
                            GT_U32    scratchPad3;

                        }D_Domain_Regs;

                    }SDRAMScratchPad;

                    struct /*SDRAMDataProtectionAndErrorReport*/{

                        struct /*R_Domain_Regs*/{

                            GT_U32    SDRAMErrorCtrl;

                        }R_Domain_Regs;

                        struct /*D_Domain_Regs*/{

                            GT_U32    DDRCtrllerErrorInterruptCause;
                            GT_U32    DDRCtrllerErrorInterruptMask;
                            GT_U32    DDRCtrllerMsgInterruptCause;
                            GT_U32    DDRCtrllerMsgInterruptMask;

                        }D_Domain_Regs;

                    }SDRAMDataProtectionAndErrorReport;

                    struct /*SDRAMCtrlRegs*/{

                        struct /*T_Domain_Regs*/{

                            struct /*newTrainingUnit*/{

                                GT_U32    currentObjectValueReg[2]/*ObjectIndex*/;
                                GT_U32    calibratedObjectPRFAReg[2]/*ObjectIndex*/;
                                GT_U32    resultCtrlReg[5][5]/*Jump4*//*Add100*/;
                                GT_U32    PUPTestSequenceRegs[8]/*jump4*/;
                                GT_U32    PUPTestCtrl;
                                GT_U32    objectOpCodeReg1[2]/*ObjectIndex*/;
                                GT_U32    objectOpCodeReg0[2]/*addr_index*/;
                                GT_U32    maxWindowSizeObjectValueReg;
                                GT_U32    globalCtrlAndStatusReg;
                                GT_U32    generalPhaseOpcodeReg;
                                GT_U32    writeLevelingDoneCntrReference;
                                GT_U32    writeReadTestCtrl;

                            }newTrainingUnit;

                            GT_U32    SDRAMInterfaceMbusCtrlLow;
                            GT_U32    SDRAMInterfaceMbusCtrlHigh;
                            GT_U32    SDRAMInterfaceMbusTimeout;
                            GT_U32    DRAMCtrllerMiscellaneous;
                            GT_U32    DRAMAddrAndCtrlDrivingStrengthLow;
                            GT_U32    DRAMDataAndDQSDrivingStrengthLow;
                            GT_U32    DRAMVerticalCalibrationMachineCtrl;
                            GT_U32    DRAMMainPadsCalibrationMachineCtrl;
                            GT_U32    SDRAMInterfaceMbusCtrl2Low;
                            GT_U32    SDRAMInterfaceMbusCtrl2High;
                            GT_U32    readDataSampleDelays;
                            GT_U32    readDataReadyDelays;

                        }T_Domain_Regs;

                        struct /*D_Domain_Regs*/{

                            struct /*newTrainingUnit*/{

                                GT_U32    write_Leveling_DQS_pattern;

                            }newTrainingUnit;

                            GT_U32    DDR3MR2CS[4]/*CS number*/;
                            GT_U32    DDR3MR3CS[4]/*CS number*/;
                            GT_U32    SDRAMTimingHigh;
                            GT_U32    SDRAMOperation;
                            GT_U32    SDRAMODTCtrlLow;
                            GT_U32    SDRAMODTCtrlHigh;
                            GT_U32    SDRAMTimingLow;
                            GT_U32    SDRAMConfig;
                            GT_U32    SDRAMAddrCtrl;
                            GT_U32    SDRAMOpenPagesCtrl;
                            GT_U32    readBufferSelect;
                            GT_U32    DDRCtrllerCtrlHigh;
                            GT_U32    PHYLockMask;
                            GT_U32    DDR3Timing;
                            GT_U32    DDRODTTimingHigh;
                            GT_U32    SDRAMInitializationCtrl;
                            GT_U32    DRAMPHYConfig;
                            GT_U32    DFS;
                            GT_U32    DDRCtrllerODTCtrl;
                            GT_U32    DDR3RegedDRAMTiming;
                            GT_U32    DDRIO;
                            GT_U32    DDR3RankCtrl;
                            GT_U32    DDR3MR0;
                            GT_U32    DDR3MR1;
                            GT_U32    DDR3MR2;
                            GT_U32    DDR3MR3;
                            GT_U32    DDR3MR1CS[4]/*CS number*/;
                            GT_U32    ZQCConfig;
                            GT_U32    DDR3MR0CS[4]/*CS number*/;
                            GT_U32    ODPGCTRLCtrl;
                            GT_U32    DDRODTTimingLow;
                            GT_U32    DRAMPHYLockStatus;
                            GT_U32    PHYRegFileAccess;
                            GT_U32    DDR3RegedDRAMCtrl;
                            GT_U32    DDRCtrllerCtrlLow;
                            GT_U32    dualDRAMCtrllerConfig;

                        }D_Domain_Regs;

                    }SDRAMCtrlRegs;

                }dunitRegs;

            }DRAMCtrllerRegsDDR3;

            struct /*DRAMCtrllerRegsDDR1*/{

                struct /*dunitRegs*/{

                    struct /*SDRAMScratchPad*/{

                        struct /*D_Domain_Regs*/{

                            GT_U32    scratchPad0;
                            GT_U32    scratchPad1;
                            GT_U32    scratchPad2;
                            GT_U32    scratchPad3;

                        }D_Domain_Regs;

                    }SDRAMScratchPad;

                    struct /*SDRAMDataProtectionAndErrorReport*/{

                        struct /*R_Domain_Regs*/{

                            GT_U32    SDRAMErrorCtrl;

                        }R_Domain_Regs;

                        struct /*D_Domain_Regs*/{

                            GT_U32    DDRCtrllerErrorInterruptCause;
                            GT_U32    DDRCtrllerErrorInterruptMask;
                            GT_U32    DDRCtrllerMsgInterruptCause;
                            GT_U32    DDRCtrllerMsgInterruptMask;

                        }D_Domain_Regs;

                    }SDRAMDataProtectionAndErrorReport;

                    struct /*SDRAMCtrlRegs*/{

                        struct /*T_Domain_Regs*/{

                            struct /*newTrainingUnit*/{

                                GT_U32    currentObjectValueReg[2]/*ObjectIndex*/;
                                GT_U32    calibratedObjectPRFAReg[2]/*ObjectIndex*/;
                                GT_U32    resultCtrlReg[5][5]/*Jump4*//*Add100*/;
                                GT_U32    PUPTestSequenceRegs[8]/*jump4*/;
                                GT_U32    PUPTestCtrl;
                                GT_U32    objectOpCodeReg1[2]/*ObjectIndex*/;
                                GT_U32    objectOpCodeReg0[2]/*addr_index*/;
                                GT_U32    maxWindowSizeObjectValueReg;
                                GT_U32    globalCtrlAndStatusReg;
                                GT_U32    generalPhaseOpcodeReg;
                                GT_U32    writeLevelingDoneCntrReference;
                                GT_U32    writeReadTestCtrl;

                            }newTrainingUnit;

                            GT_U32    SDRAMInterfaceMbusCtrlLow;
                            GT_U32    SDRAMInterfaceMbusCtrlHigh;
                            GT_U32    SDRAMInterfaceMbusTimeout;
                            GT_U32    DRAMCtrllerMiscellaneous;
                            GT_U32    DRAMAddrAndCtrlDrivingStrengthLow;
                            GT_U32    DRAMDataAndDQSDrivingStrengthLow;
                            GT_U32    DRAMVerticalCalibrationMachineCtrl;
                            GT_U32    DRAMMainPadsCalibrationMachineCtrl;
                            GT_U32    SDRAMInterfaceMbusCtrl2Low;
                            GT_U32    SDRAMInterfaceMbusCtrl2High;
                            GT_U32    readDataSampleDelays;
                            GT_U32    readDataReadyDelays;

                        }T_Domain_Regs;

                        struct /*D_Domain_Regs*/{

                            struct /*newTrainingUnit*/{

                                GT_U32    write_Leveling_DQS_pattern;

                            }newTrainingUnit;

                            GT_U32    DDR3MR3CS[4]/*CS number*/;
                            GT_U32    DDR3MR0CS[4]/*CS number*/;
                            GT_U32    DDR3MR1CS[4]/*CS number*/;
                            GT_U32    DDR3MR2CS[4]/*CS number*/;
                            GT_U32    SDRAMConfig;
                            GT_U32    DDRCtrllerCtrlLow;
                            GT_U32    SDRAMTimingLow;
                            GT_U32    SDRAMTimingHigh;
                            GT_U32    SDRAMAddrCtrl;
                            GT_U32    SDRAMOpenPagesCtrl;
                            GT_U32    SDRAMOperation;
                            GT_U32    DDRCtrllerCtrlHigh;
                            GT_U32    PHYLockMask;
                            GT_U32    DDR3Timing;
                            GT_U32    DDRODTTimingHigh;
                            GT_U32    SDRAMInitializationCtrl;
                            GT_U32    SDRAMODTCtrlLow;
                            GT_U32    SDRAMODTCtrlHigh;
                            GT_U32    DDRCtrllerODTCtrl;
                            GT_U32    readBufferSelect;
                            GT_U32    DDRIO;
                            GT_U32    DFS;
                            GT_U32    DDR3MR0;
                            GT_U32    DDR3MR1;
                            GT_U32    DDR3MR2;
                            GT_U32    DDR3MR3;
                            GT_U32    DDR3RankCtrl;
                            GT_U32    ZQCConfig;
                            GT_U32    DRAMPHYConfig;
                            GT_U32    ODPGCTRLCtrl;
                            GT_U32    DDRODTTimingLow;
                            GT_U32    DRAMPHYLockStatus;
                            GT_U32    PHYRegFileAccess;
                            GT_U32    DDR3RegedDRAMCtrl;
                            GT_U32    DDR3RegedDRAMTiming;
                            GT_U32    dualDRAMCtrllerConfig;

                        }D_Domain_Regs;

                    }SDRAMCtrlRegs;

                }dunitRegs;

            }DRAMCtrllerRegsDDR1;

            struct /*DRAMCtrllerRegsDDR2*/{

                struct /*dunitRegs*/{

                    struct /*SDRAMScratchPad*/{

                        struct /*D_Domain_Regs*/{

                            GT_U32    scratchPad0;
                            GT_U32    scratchPad1;
                            GT_U32    scratchPad2;
                            GT_U32    scratchPad3;

                        }D_Domain_Regs;

                    }SDRAMScratchPad;

                    struct /*SDRAMDataProtectionAndErrorReport*/{

                        struct /*R_Domain_Regs*/{

                            GT_U32    SDRAMErrorCtrl;

                        }R_Domain_Regs;

                        struct /*D_Domain_Regs*/{

                            GT_U32    DDRCtrllerErrorInterruptCause;
                            GT_U32    DDRCtrllerErrorInterruptMask;
                            GT_U32    DDRCtrllerMsgInterruptCause;
                            GT_U32    DDRCtrllerMsgInterruptMask;

                        }D_Domain_Regs;

                    }SDRAMDataProtectionAndErrorReport;

                    struct /*SDRAMCtrlRegs*/{

                        struct /*T_Domain_Regs*/{

                            struct /*newTrainingUnit*/{

                                GT_U32    currentObjectValueReg[2]/*ObjectIndex*/;
                                GT_U32    calibratedObjectPRFAReg[2]/*ObjectIndex*/;
                                GT_U32    resultCtrlReg[5][5]/*Jump4*//*Add100*/;
                                GT_U32    PUPTestSequenceRegs[8]/*jump4*/;
                                GT_U32    PUPTestCtrl;
                                GT_U32    objectOpCodeReg1[2]/*ObjectIndex*/;
                                GT_U32    objectOpCodeReg0[2]/*addr_index*/;
                                GT_U32    maxWindowSizeObjectValueReg;
                                GT_U32    globalCtrlAndStatusReg;
                                GT_U32    generalPhaseOpcodeReg;
                                GT_U32    writeLevelingDoneCntrReference;
                                GT_U32    writeReadTestCtrl;

                            }newTrainingUnit;

                            GT_U32    SDRAMInterfaceMbusCtrlLow;
                            GT_U32    SDRAMInterfaceMbusCtrlHigh;
                            GT_U32    SDRAMInterfaceMbusTimeout;
                            GT_U32    DRAMCtrllerMiscellaneous;
                            GT_U32    DRAMAddrAndCtrlDrivingStrengthLow;
                            GT_U32    DRAMDataAndDQSDrivingStrengthLow;
                            GT_U32    DRAMVerticalCalibrationMachineCtrl;
                            GT_U32    DRAMMainPadsCalibrationMachineCtrl;
                            GT_U32    SDRAMInterfaceMbusCtrl2Low;
                            GT_U32    SDRAMInterfaceMbusCtrl2High;
                            GT_U32    readDataSampleDelays;
                            GT_U32    readDataReadyDelays;

                        }T_Domain_Regs;

                        struct /*D_Domain_Regs*/{

                            struct /*newTrainingUnit*/{

                                GT_U32    write_Leveling_DQS_pattern;

                            }newTrainingUnit;

                            GT_U32    DDR3MR3CS[4]/*CS number*/;
                            GT_U32    DDR3MR0CS[4]/*CS number*/;
                            GT_U32    DDR3MR1CS[4]/*CS number*/;
                            GT_U32    DDR3MR2CS[4]/*CS number*/;
                            GT_U32    SDRAMConfig;
                            GT_U32    DDRCtrllerCtrlLow;
                            GT_U32    SDRAMTimingLow;
                            GT_U32    SDRAMTimingHigh;
                            GT_U32    SDRAMAddrCtrl;
                            GT_U32    SDRAMOpenPagesCtrl;
                            GT_U32    SDRAMOperation;
                            GT_U32    DDRCtrllerCtrlHigh;
                            GT_U32    PHYLockMask;
                            GT_U32    DDR3Timing;
                            GT_U32    DDRODTTimingHigh;
                            GT_U32    SDRAMInitializationCtrl;
                            GT_U32    SDRAMODTCtrlLow;
                            GT_U32    SDRAMODTCtrlHigh;
                            GT_U32    DDRCtrllerODTCtrl;
                            GT_U32    readBufferSelect;
                            GT_U32    DDRIO;
                            GT_U32    DFS;
                            GT_U32    DDR3MR0;
                            GT_U32    DDR3MR1;
                            GT_U32    DDR3MR2;
                            GT_U32    DDR3MR3;
                            GT_U32    DDR3RankCtrl;
                            GT_U32    ZQCConfig;
                            GT_U32    DRAMPHYConfig;
                            GT_U32    ODPGCTRLCtrl;
                            GT_U32    DDRODTTimingLow;
                            GT_U32    DRAMPHYLockStatus;
                            GT_U32    PHYRegFileAccess;
                            GT_U32    DDR3RegedDRAMCtrl;
                            GT_U32    DDR3RegedDRAMTiming;
                            GT_U32    dualDRAMCtrllerConfig;

                        }D_Domain_Regs;

                    }SDRAMCtrlRegs;

                }dunitRegs;

            }DRAMCtrllerRegsDDR2;

            struct /*DRAMCtrllerRegsDDR0*/{

                struct /*dunitRegs*/{

                    struct /*SDRAMDataProtectionAndErrorReport*/{

                        struct /*R_Domain_Regs*/{

                            GT_U32    SDRAMErrorCtrl;

                        }R_Domain_Regs;

                        struct /*D_Domain_Regs*/{

                            GT_U32    DDRCtrllerErrorInterruptCause;
                            GT_U32    DDRCtrllerErrorInterruptMask;
                            GT_U32    DDRCtrllerMsgInterruptCause;
                            GT_U32    DDRCtrllerMsgInterruptMask;

                        }D_Domain_Regs;

                    }SDRAMDataProtectionAndErrorReport;

                    struct /*SDRAMCtrlRegs*/{

                        struct /*T_Domain_Regs*/{

                            struct /*newTrainingUnit*/{

                                GT_U32    currentObjectValueReg[2]/*ObjectIndex*/;
                                GT_U32    calibratedObjectPRFAReg[2]/*ObjectIndex*/;
                                GT_U32    resultCtrlReg[5][5]/*Jump4*//*Add100*/;
                                GT_U32    PUPTestSequenceRegs[8]/*jump4*/;
                                GT_U32    PUPTestCtrl;
                                GT_U32    objectOpCodeReg1[2]/*ObjectIndex*/;
                                GT_U32    objectOpCodeReg0[2]/*addr_index*/;
                                GT_U32    maxWindowSizeObjectValueReg;
                                GT_U32    globalCtrlAndStatusReg;
                                GT_U32    generalPhaseOpcodeReg;
                                GT_U32    writeLevelingDoneCntrReference;
                                GT_U32    writeReadTestCtrl;

                            }newTrainingUnit;

                            GT_U32    SDRAMInterfaceMbusCtrlLow;
                            GT_U32    SDRAMInterfaceMbusCtrlHigh;
                            GT_U32    SDRAMInterfaceMbusTimeout;
                            GT_U32    DRAMCtrllerMiscellaneous;
                            GT_U32    DRAMAddrAndCtrlDrivingStrengthLow;
                            GT_U32    DRAMDataAndDQSDrivingStrengthLow;
                            GT_U32    DRAMVerticalCalibrationMachineCtrl;
                            GT_U32    DRAMMainPadsCalibrationMachineCtrl;
                            GT_U32    SDRAMInterfaceMbusCtrl2Low;
                            GT_U32    SDRAMInterfaceMbusCtrl2High;
                            GT_U32    readDataSampleDelays;
                            GT_U32    readDataReadyDelays;

                        }T_Domain_Regs;

                        struct /*D_Domain_Regs*/{

                            struct /*newTrainingUnit*/{

                                GT_U32    write_Leveling_DQS_pattern;

                            }newTrainingUnit;

                            GT_U32    DDR3MR2CS[4]/*CS number*/;
                            GT_U32    DDR3MR3CS[4]/*CS number*/;
                            GT_U32    SDRAMTimingHigh;
                            GT_U32    SDRAMOperation;
                            GT_U32    SDRAMODTCtrlLow;
                            GT_U32    SDRAMODTCtrlHigh;
                            GT_U32    SDRAMTimingLow;
                            GT_U32    SDRAMConfig;
                            GT_U32    SDRAMAddrCtrl;
                            GT_U32    SDRAMOpenPagesCtrl;
                            GT_U32    readBufferSelect;
                            GT_U32    DDRCtrllerCtrlHigh;
                            GT_U32    PHYLockMask;
                            GT_U32    DDR3Timing;
                            GT_U32    DDRODTTimingHigh;
                            GT_U32    SDRAMInitializationCtrl;
                            GT_U32    DRAMPHYConfig;
                            GT_U32    DFS;
                            GT_U32    DDRCtrllerODTCtrl;
                            GT_U32    DDR3RegedDRAMTiming;
                            GT_U32    DDRIO;
                            GT_U32    DDR3RankCtrl;
                            GT_U32    DDR3MR0;
                            GT_U32    DDR3MR1;
                            GT_U32    DDR3MR2;
                            GT_U32    DDR3MR3;
                            GT_U32    DDR3MR1CS[4]/*CS number*/;
                            GT_U32    ZQCConfig;
                            GT_U32    DDR3MR0CS[4]/*CS number*/;
                            GT_U32    ODPGCTRLCtrl;
                            GT_U32    DDRODTTimingLow;
                            GT_U32    DRAMPHYLockStatus;
                            GT_U32    PHYRegFileAccess;
                            GT_U32    DDR3RegedDRAMCtrl;
                            GT_U32    DDRCtrllerCtrlLow;
                            GT_U32    dualDRAMCtrllerConfig;

                        }D_Domain_Regs;

                    }SDRAMCtrlRegs;

                    struct /*SDRAMScratchPad*/{

                        struct /*D_Domain_Regs*/{

                            GT_U32    scratchPad0;
                            GT_U32    scratchPad1;
                            GT_U32    scratchPad2;
                            GT_U32    scratchPad3;

                        }D_Domain_Regs;

                    }SDRAMScratchPad;

                }dunitRegs;

            }DRAMCtrllerRegsDDR0;

            struct /*DRAMCtrllerRegsDDR4*/{

                struct /*dunitRegs*/{

                    struct /*SDRAMScratchPad*/{

                        struct /*D_Domain_Regs*/{

                            GT_U32    scratchPad0;
                            GT_U32    scratchPad1;
                            GT_U32    scratchPad2;
                            GT_U32    scratchPad3;

                        }D_Domain_Regs;

                    }SDRAMScratchPad;

                    struct /*SDRAMCtrlRegs*/{

                        struct /*T_Domain_Regs*/{

                            struct /*newTrainingUnit*/{

                                GT_U32    currentObjectValueReg[2]/*ObjectIndex*/;
                                GT_U32    calibratedObjectPRFAReg[2]/*ObjectIndex*/;
                                GT_U32    resultCtrlReg[5][5]/*Jump4*//*Add100*/;
                                GT_U32    PUPTestSequenceRegs[8]/*jump4*/;
                                GT_U32    PUPTestCtrl;
                                GT_U32    objectOpCodeReg1[2]/*ObjectIndex*/;
                                GT_U32    objectOpCodeReg0[2]/*addr_index*/;
                                GT_U32    maxWindowSizeObjectValueReg;
                                GT_U32    globalCtrlAndStatusReg;
                                GT_U32    generalPhaseOpcodeReg;
                                GT_U32    writeLevelingDoneCntrReference;
                                GT_U32    writeReadTestCtrl;

                            }newTrainingUnit;

                            GT_U32    SDRAMInterfaceMbusCtrlLow;
                            GT_U32    SDRAMInterfaceMbusCtrlHigh;
                            GT_U32    SDRAMInterfaceMbusTimeout;
                            GT_U32    DRAMCtrllerMiscellaneous;
                            GT_U32    DRAMAddrAndCtrlDrivingStrengthLow;
                            GT_U32    DRAMDataAndDQSDrivingStrengthLow;
                            GT_U32    DRAMVerticalCalibrationMachineCtrl;
                            GT_U32    DRAMMainPadsCalibrationMachineCtrl;
                            GT_U32    SDRAMInterfaceMbusCtrl2Low;
                            GT_U32    SDRAMInterfaceMbusCtrl2High;
                            GT_U32    readDataSampleDelays;
                            GT_U32    readDataReadyDelays;

                        }T_Domain_Regs;

                        struct /*D_Domain_Regs*/{

                            struct /*newTrainingUnit*/{

                                GT_U32    write_Leveling_DQS_pattern;

                            }newTrainingUnit;

                            GT_U32    DDR3MR2CS[4]/*CS number*/;
                            GT_U32    DDR3MR3CS[4]/*CS number*/;
                            GT_U32    SDRAMTimingHigh;
                            GT_U32    SDRAMOperation;
                            GT_U32    SDRAMODTCtrlLow;
                            GT_U32    SDRAMODTCtrlHigh;
                            GT_U32    SDRAMTimingLow;
                            GT_U32    SDRAMConfig;
                            GT_U32    SDRAMAddrCtrl;
                            GT_U32    SDRAMOpenPagesCtrl;
                            GT_U32    readBufferSelect;
                            GT_U32    DDRCtrllerCtrlHigh;
                            GT_U32    PHYLockMask;
                            GT_U32    DDR3Timing;
                            GT_U32    DDRODTTimingHigh;
                            GT_U32    SDRAMInitializationCtrl;
                            GT_U32    DRAMPHYConfig;
                            GT_U32    DFS;
                            GT_U32    DDRCtrllerODTCtrl;
                            GT_U32    DDR3RegedDRAMTiming;
                            GT_U32    DDRIO;
                            GT_U32    DDR3RankCtrl;
                            GT_U32    DDR3MR0;
                            GT_U32    DDR3MR1;
                            GT_U32    DDR3MR2;
                            GT_U32    DDR3MR3;
                            GT_U32    DDR3MR1CS[4]/*CS number*/;
                            GT_U32    ZQCConfig;
                            GT_U32    DDR3MR0CS[4]/*CS number*/;
                            GT_U32    ODPGCTRLCtrl;
                            GT_U32    DDRODTTimingLow;
                            GT_U32    DRAMPHYLockStatus;
                            GT_U32    PHYRegFileAccess;
                            GT_U32    DDR3RegedDRAMCtrl;
                            GT_U32    DDRCtrllerCtrlLow;
                            GT_U32    dualDRAMCtrllerConfig;

                        }D_Domain_Regs;

                    }SDRAMCtrlRegs;

                    struct /*SDRAMDataProtectionAndErrorReport*/{

                        struct /*R_Domain_Regs*/{

                            GT_U32    SDRAMErrorCtrl;

                        }R_Domain_Regs;

                        struct /*D_Domain_Regs*/{

                            GT_U32    DDRCtrllerErrorInterruptCause;
                            GT_U32    DDRCtrllerErrorInterruptMask;
                            GT_U32    DDRCtrllerMsgInterruptCause;
                            GT_U32    DDRCtrllerMsgInterruptMask;

                        }D_Domain_Regs;

                    }SDRAMDataProtectionAndErrorReport;

                }dunitRegs;

            }DRAMCtrllerRegsDDR4;

        }XSB_XBAR;

        struct /*DFXServerUnitsBC2SpecificRegs*/{

            GT_U32    deviceResetCtrl;
            GT_U32    configSkipInitializationMatrix;
            GT_U32    RAMInitSkipInitializationMatrix;
            GT_U32    trafficManagerSkipInitializationMatrix;
            GT_U32    tableSkipInitializationMatrix;
            GT_U32    SERDESSkipInitializationMatrix;
            GT_U32    EEPROMSkipInitializationMatrix;
            GT_U32    PCIeSkipInitializationMatrix;
            GT_U32    deviceSAR1;
            GT_U32    deviceSAR2;
            GT_U32    deviceCtrl4;
            GT_U32    deviceCtrl5;
            GT_U32    deviceCtrl8;
            GT_U32    deviceCtrl10;
            GT_U32    deviceCtrl12;
            GT_U32    deviceCtrl21;
            GT_U32    deviceCtrl20;
            GT_U32    deviceCtrl19;
            GT_U32    deviceCtrl18;
            GT_U32    deviceCtrl15;
            GT_U32    deviceCtrl14;
            GT_U32    deviceStatus0;
            GT_U32    deviceStatus1;
            GT_U32    initializationStatusDone;

        }DFXServerUnitsBC2SpecificRegs;

        struct /*DFXServerUnits*/{

            struct /*DFXServerRegs*/{

                GT_U32    AVSDisabledCtrl2[1]/*AVS*/;
                GT_U32    AVSDisabledCtrl1[1]/*AVS*/;
                GT_U32    temperatureSensor28nmCtrlLSB;
                GT_U32    serverXBARTargetPortConfig[4]/*Target Port*/;
                GT_U32    serverStatus;
                GT_U32    serverInterruptSummaryCause;
                GT_U32    serverInterruptSummaryMask;
                GT_U32    serverInterruptMask;
                GT_U32    serverInterruptCause;
                GT_U32    serverAddrSpace;
                GT_U32    pipeSelect;
                GT_U32    temperatureSensor28nmCtrlMSB;
                GT_U32    temperatureSensorStatus;
                GT_U32    AVSStatus[1]/*AVS*/;
                GT_U32    AVSMinStatus[1]/*AVS*/;
                GT_U32    AVSMaxStatus[1]/*AVS*/;
                GT_U32    AVSEnabledCtrl[1]/*AVS*/;
                GT_U32    snoopBusStatus;
                GT_U32    deviceIDStatus;
                GT_U32    JTAGDeviceIDStatus;

            }DFXServerRegs;

        }DFXServerUnits;

        struct /*DDRPHYUnits*/{

            struct /*DDRPHYRegs*/{

                GT_U32    chipSelectCSIOWriteDeskew[2][11]/*M_CS number*//*IO signal number*/;
                GT_U32    chipSelectCSWriteCentralization[2]/*M_CS number*/;
                GT_U32    PADPREEMPDisable;
                GT_U32    chipSelectCSWriteDeskewBc[2]/*M_CS number*/;
                GT_U32    PADOutputDisable;
                GT_U32    chipSelectCSReadLeveling[2]/*M_CS number*/;
                GT_U32    PADODTCalibration;
                GT_U32    PADImpedanceCalibration;
                GT_U32    PHYCtrl;
                GT_U32    PADCOMPBDisable;
                GT_U32    PADCMOSBDisable;
                GT_U32    chipSelectCSWriteLeveling[2]/*M_CS number*/;
                GT_U32    PADODTDisable;
                GT_U32    PADZRICalibration;
                GT_U32    PADPREEMPCalibration;
                GT_U32    chipSelectCSReadDeskewBc[2]/*M_CS number*/;
                GT_U32    chipSelectCSReadCentralization[2]/*M_CS number*/;
                GT_U32    PADConfig;
                GT_U32    chipSelectCSIOReadDeskew[2][11]/*M_CS number*//*IO signal number*/;

            }DDRPHYRegs;

        }DDRPHYUnits;

        struct /*DFX_RAM_4_8[1]--Memory--*/{

            GT_U32    errorsCntr;
            GT_U32    erroneousAddr;
            GT_U32    ECCSyndrome;
            GT_U32    erroneousSegment;
            GT_U32    memoryCtrl;
            GT_U32    memoryInterruptMask;
            GT_U32    memoryInterruptCause;

        }DFX_RAM_4_8[1]/*Memory*/;

    }resetAndInitCtrller;

}SMEM_DFX_SERVER_PP_REGS_ADDR_STC;
#define SIP5_TXQ_DQ     TXQ.dq
#define SIP5_TXQ_LL     TXQ.ll
#define SIP5_TXQ_QUEUE  TXQ.queue
#define SIP5_TXQ_PFC    TXQ.pfc
#define SIP5_TXQ_QCN    TXQ.qcn

#define SMEM_CHT_NUM_UNITS_MAX_CNS  512

/*******************************************************************************
*   SMEM_CONVERT_DEV_AND_ADDR_TO_NEW_DEV_AND_ADDR_FUNC
*
* DESCRIPTION:
*       Convert {dev,address} to new {dev,address}.
*       needed for multi-pipe device.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       address     - address of memory(register or table).
*       accessType  - the access type
* OUTPUTS:
*       newDevObjPtrPtr      - (pointer to) pointer to new device object.
*       newAddressPtr        - (pointer to) address of memory (register or table).
*
* RETURNS:
*        None
*
* COMMENTS:
*       function MUST be called before calling smemFindMemory()
*
*******************************************************************************/
typedef void (*SMEM_CONVERT_DEV_AND_ADDR_TO_NEW_DEV_AND_ADDR_FUNC)
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32                  address,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    OUT SKERNEL_DEVICE_OBJECT * *newDevObjPtrPtr,
    OUT GT_U32                  *newAddressPtr
);


/* common memory for all dxch devices */
typedef struct {
    SMEM_SPEC_FIND_FUN_ENTRY_STC    specFunTbl[SMEM_CHT_NUM_UNITS_MAX_CNS];
    CHT_AUQ_MEM                     auqMem;             /* AUQ manager info */
    CHT2_FUQ_MEM                    fuqMem;             /* FUQ manager info - for ch2 and above */
    CHT_AUQ_FIFO_MEM                auqFifoMem;         /*'fifo on chip' AUQ registers */
    CHT_INTERNAL_SIMULATION_USE_MEM internalSimMem;     /* internal use registers (simulation) */
    CHT_PHY_MEM                     phyMem;             /* registers needed for PHY accessing */
    SMEM_LION_GTSQ_FIFO             gtsIngressFifoMem;  /* ingress timestamping fifo - for Lion B0 and above */
    SMEM_LION_GTSQ_FIFO             gtsEgressFifoMem;   /* egress timestamping fifo - for Lion B0 and above */
    SMEM_CALLOC_STC                 callocMemArray[SMEM_MAX_CALLOC_MEM_SIZE]; /* array of allocated memories used for SW reset */
    GT_U32                          callocMemSize;      /* size of allocated memories used for SW reset */
    GT_U8                       *   unitChunksBasePtr;  /* pointer to first unit chunk in device memory */
    GT_U32                          unitChunksSizeOf;   /* summary of unit chunks structures size in bytes */
    SMEM_CHT_PP_REGS_ADDR_STC       regAddrDb;          /* registers address data base */
    GT_BOOL                         support_regAddrDbSip5; /* indication that the device support info from the regAddrDb1 */
    GT_BOOL                         support_regAddrDbSip5_10; /* device support SIP5_10 */
    SMEM_SIP5_PP_REGS_ADDR_STC      regAddrDbSip5;          /* registers address data base - used for SIP5 devices */\
    GT_BIT                          accessPexMemorySpaceOnlyOnExplicitAction;/* allow pex memory space only for 'memory access to pex' (and not for regular memory access) */
    SMEM_UNIT_CHUNK_BASE_ADDRESS_STC pciExtMemArr[SMEM_UNIT_PCI_BUS_UNIT_LAST_E];  /* Extra PCI/PEX/MBUS/DFX memory for lookup before accessing the PCI/PEX/MBUS memory of the device */
    GT_U32                          pciUnitBaseAddr;    /* PCI/PEX memory base address */
    GT_U32                          pciUnitBaseAddrMask; /* Mask of the bits used as the PCI/PEX unit offset */
    GT_U32                          dfxMemBaseAddr;     /* PCI/PEX memory base address */
    GT_U32                          dfxMemBaseAddrMask; /* Mask of the bits used as the PCI/PEX unit offset */
    GT_BOOL                         supportExternalDfx;  /* indication that the device support info from the DFX server */
    SMEM_DFX_SERVER_PP_REGS_ADDR_STC regAddrExternalDfx; /* registers address data base - used for DFX server memory access */
    GT_BIT                          isPartOfGeneric;/* indication that this object is part of SMEM_CHT_GENERIC_DEV_MEM_INFO .
                                                NOTE: the ch1,2,3 and xcat --> not use SMEM_CHT_GENERIC_DEV_MEM_INFO
                                                the lion,xcat2,lion2,lion3,bc2 --> use it

                                                */
    SMEM_CONVERT_DEV_AND_ADDR_TO_NEW_DEV_AND_ADDR_FUNC  smemConvertDevAndAddrToNewDevAndAddrFunc;
}SMEM_CHT_DEV_COMMON_MEM_INFO;

/*******************************************************************************
* Typedef: struct SMEM_CHT_GENERIC_DEV_MEM_INFO
*
* Description:
*      may describe ALL DXCH memory object in the simulation.
*
* Fields:
*
* Comments:
*******************************************************************************/

typedef struct {
    SMEM_CHT_DEV_COMMON_MEM_INFO    common;/* must be first */
    SMEM_UNIT_CHUNKS_STC            unitMemArr[SMEM_CHT_NUM_UNITS_MAX_CNS];
    /* extra units that reside in parallel memory space */
    SMEM_UNIT_CHUNKS_STC            PEX_UnitMem;
    /* pointer to extra memory units that maybe used*/
    GT_U32                          numOfExtraUnits;/* number of extra units */
    SMEM_UNIT_CHUNKS_STC            *extraUnitsPtr; /* array of extra units  */
}SMEM_CHT_GENERIC_DEV_MEM_INFO;


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
    SMEM_CHT_DEV_COMMON_MEM_INFO    common;/* must be first */
    CHT_GLOBAL_MEM              globalMem;
    CHT_EGR_MEM                 egrMem;
    CHT_BRDG_MNG_MEM            brdgMngMem;
    CHT_BUF_MNG_MEM             bufMngMem;
    CHT_GOP_CONF_MEM            gopCnfMem;
    CHT_MAC_FDB_MEM             macFdbMem;
    CHT_BANKS_MEM               banksMem;
    CHT_BUF_MEM                 bufMem;
    CHT_VLAN_TBL_MEM            vlanTblMem;
    CHT_TRI_SPEED_PORTS_MEM     triSpeedPortsMem;
    CHT_PREEGR_MEM              preegressMem;
    CHT_PCL_MEM                 pclMem;
    CHT_POLICER_MEM             policerMem;
    CHT_PCI_MEM                 pciMem;
    CHT_IP_MEM                  ipMem;
}SMEM_CHT_DEV_MEM_INFO;

/* the num of bits used in hash calculation */
#define SMEM_CHT_FDB_HASH_NUM_BITS(tableSize)             \
    (                                                     \
     (tableSize) == 0x1000 /*4K entries*/  ? 10 :         \
     (tableSize) == 0x2000 /*8K entries*/  ? 11 :         \
     (tableSize) == 0x4000 /*16K entries*/ ? 12 :         \
     (tableSize) == 0x8000 /*32K entries*/ ? 13 :         \
     (tableSize) == 0x10000 /*64K entries*/? 14 :         \
     (tableSize) == 0x20000 /*128K entries*/? 15 :        \
     (tableSize) == 0x40000 /*256K entries*/? 16 :        \
     0)

typedef enum{
    SMEM_CHT_MEMORY_DEVICE_AUQ_E,
    SMEM_CHT_MEMORY_DEVICE_FUQ_E,
    SMEM_CHT_MEMORY_DEVICE_ON_CHIP_FIFO_AUQ_E,

}SMEM_CHT_MEMORY_DEVICE_ENT;

/* Lion3 : the port group for the FDB unit */
#define LION3_UNIT_FDB_TABLE_SINGLE_INSTANCE_PORT_GROUP_CNS     0

/* get the device that hold the needed memory */
SKERNEL_DEVICE_OBJECT * smemChtMemoryDeviceGet(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SMEM_CHT_MEMORY_DEVICE_ENT memoryType
);

/* MAC entry width in words :
    ch1,2,3,xcat,lion,xcat2 - 4 words
    lion2 - 5 words
*/
#define SMEM_CHT_MAC_TABLE_WORDS            (5)

/* MAC update message in words */
/* number of words alignment in AU Message format .
in ch1,2,3,xcat1,2,lion - 4 words
in lion2 6 words entry format - but alignment is 8 words
*/
#define SMEM_CHT_AU_MSG_WORDS               (8)

/* macro to short active memory entry for 'write' */
#define ACTIVE_WRITE_MEMORY_MAC(offset, writeTableFunc, index)  \
    { offset, SMEM_FULL_MASK_CNS, NULL, 0 , writeTableFunc, index}

/* macro to short active memory entry for 'read' */
#define ACTIVE_READ_MEMORY_MAC(offset, readTableFunc, index)  \
    { offset, SMEM_FULL_MASK_CNS, readTableFunc, index , NULL, 0}

/* enum for 'units' place holders in the active memory tables */
typedef enum{
/* xcat A1 , Lion B policer macros */
    POLICER_ACTIVE_MEMORY_PLACE_HOLDER_MASK_CNS   = 0x7e000000,
    /*GTS*/
    GTS_ACTIVE_MEMORY_PLACE_HOLDER_MASK_CNS       ,
/* Lion3 and above CNC macros */
    CNC_ACTIVE_MEMORY_PLACE_HOLDER_MASK_CNS       ,
/* Lion3 and above CNC macros */
    OAM_ACTIVE_MEMORY_PLACE_HOLDER_MASK_CNS       ,

    ACTIVE_MEMORY_PLACE_HOLDER_MASK___LAST___E    = 0x7f000000/* last --> internal use */
}ACTIVE_MEMORY_PLACE_HOLDER_MASK_ENT;

/* generic place holder for write entry in active memory table */
/*myUnit is one of ACTIVE_MEMORY_PLACE_HOLDER_MASK_ENT*/
#define ACTIVE_WRITE_MEMORY_PLACE_HOLDER_MAC(myUnit , offset , writeTableFunc , index)  \
    { offset, myUnit/*the mask field*/, NULL, index/* used in init*/ , writeTableFunc, index}

/* generic place holder for read entry in active memory table */
/*myUnit is one of ACTIVE_MEMORY_PLACE_HOLDER_MASK_ENT*/
#define GENERIC_ACTIVE_READ_MEMORY_PLACE_HOLDER_MAC(myUnit , offset , readTableFunc , index)  \
    { offset, myUnit/*the mask field*/, readTableFunc, index , NULL, index/* used in init*/}


/* generic place holder for read+write entry in active memory table */
/*myUnit is one of ACTIVE_MEMORY_PLACE_HOLDER_MASK_ENT*/
#define GENERIC_ACTIVE_READ_WRITE_MEMORY_PLACE_HOLDER_MAC(myUnit , offset , readTableFunc,readFunParam , writeTableFunc,writeFunParam)  \
    { offset, myUnit/*the mask field*/, readTableFunc, readFunParam , writeTableFunc, writeFunParam}



/* place holder for active memory entry that needed both 'read' and 'write' functions
   but  'units' entries with proper offsets */
/* NOTE: we store the needed info spread into 2 entries , so the 'place holder' can be set !!! */
/* NOTE: the 'unit_index' (0 or 1) is not set as there is no additional 'free' parameter */
/* so if the functions that bound to the 'CNC active memory' need the 'unit_id' (0 or 1)
    they need to call function cncUnitIndexFromAddrGet(dev,address)*/
#define GENERIC_2_UNITS_ACTIVE_READ_WRITE_MEMORY(myUnit ,offset , mask, readFun,readFunParam , writeFun,writeFunParam)       \
    GENERIC_ACTIVE_READ_WRITE_MEMORY_PLACE_HOLDER_MAC(myUnit ,offset,readFun,readFunParam , writeFun,writeFunParam), \
    GENERIC_ACTIVE_READ_WRITE_MEMORY_PLACE_HOLDER_MAC(myUnit ,offset,NULL,mask , NULL,0)

/* init 2 'units' entries with proper offsets */
#define GENERIC_2_UNITS_ACTIVE_MEM_ENTRY_INIT_MAC(dev,unitName,entryPtr) \
    /* fix address of entry for 'unitName_0' */                          \
    (entryPtr)[0].address += dev->memUnitBaseAddrInfo.unitName[0];       \
    /* fix mask of entry for 'unitName_0' from 'place holder' to actual mask (taken from 'next entry' !!!)*/\
    (entryPtr)[0].mask     = (entryPtr)[1].readFunParam;            \
    /* copy entry to next entry */                                  \
    (entryPtr)[1] = (entryPtr)[0];                                  \
    /* update address of next entry to be address of unitName_1*/        \
    (entryPtr)[1].address -= devObjPtr->memUnitBaseAddrInfo.unitName[0]; \
    (entryPtr)[1].address += devObjPtr->memUnitBaseAddrInfo.unitName[1]



/* policer write active memories place holder for all 3 PLRs */
#define POLICER_ACTIVE_WRITE_MEMORY(offset , writeTableFunc)      \
    ACTIVE_WRITE_MEMORY_PLACE_HOLDER_MAC(POLICER_ACTIVE_MEMORY_PLACE_HOLDER_MASK_CNS, offset , writeTableFunc , 0), \
    ACTIVE_WRITE_MEMORY_PLACE_HOLDER_MAC(POLICER_ACTIVE_MEMORY_PLACE_HOLDER_MASK_CNS, offset , writeTableFunc , 1), \
    ACTIVE_WRITE_MEMORY_PLACE_HOLDER_MAC(POLICER_ACTIVE_MEMORY_PLACE_HOLDER_MASK_CNS, offset , writeTableFunc , 2)

/* policer read active memories place holder for all 3 PLRs */
#define POLICER_ACTIVE_READ_MEMORY(offset , readTableFunc)      \
    GENERIC_ACTIVE_READ_MEMORY_PLACE_HOLDER_MAC(POLICER_ACTIVE_MEMORY_PLACE_HOLDER_MASK_CNS, offset , readTableFunc , 0), \
    GENERIC_ACTIVE_READ_MEMORY_PLACE_HOLDER_MAC(POLICER_ACTIVE_MEMORY_PLACE_HOLDER_MASK_CNS, offset , readTableFunc , 1), \
    GENERIC_ACTIVE_READ_MEMORY_PLACE_HOLDER_MAC(POLICER_ACTIVE_MEMORY_PLACE_HOLDER_MASK_CNS, offset , readTableFunc , 2)

/* GTS read active memories place holder for all 2 GTS */
#define GTS_ACTIVE_READ_MEMORY(offset, readTableFunc)      \
    GENERIC_ACTIVE_READ_MEMORY_PLACE_HOLDER_MAC(GTS_ACTIVE_MEMORY_PLACE_HOLDER_MASK_CNS, offset , readTableFunc , 0), \
    GENERIC_ACTIVE_READ_MEMORY_PLACE_HOLDER_MAC(GTS_ACTIVE_MEMORY_PLACE_HOLDER_MASK_CNS, offset , readTableFunc , 1)

/* GTS write active memories place holder for all 2 GTS */
#define GTS_ACTIVE_WRITE_MEMORY(offset, writeTableFunc)      \
    ACTIVE_WRITE_MEMORY_PLACE_HOLDER_MAC(GTS_ACTIVE_MEMORY_PLACE_HOLDER_MASK_CNS, offset , writeTableFunc , 0), \
    ACTIVE_WRITE_MEMORY_PLACE_HOLDER_MAC(GTS_ACTIVE_MEMORY_PLACE_HOLDER_MASK_CNS, offset , writeTableFunc , 1)

/* Generic OAM active read functions place holder for all 2 OAM*/
#define OAM_ACTIVE_READ_MEMORY(offset, readTableFunc)      \
    GENERIC_ACTIVE_READ_MEMORY_PLACE_HOLDER_MAC(OAM_ACTIVE_MEMORY_PLACE_HOLDER_MASK_CNS, offset , readTableFunc , 0), \
    GENERIC_ACTIVE_READ_MEMORY_PLACE_HOLDER_MAC(OAM_ACTIVE_MEMORY_PLACE_HOLDER_MASK_CNS, offset , readTableFunc , 1)

/* Generic OAM active write functions place holder for all 2 OAM */
#define OAM_ACTIVE_WRITE_MEMORY(offset, writeTableFunc)      \
    ACTIVE_WRITE_MEMORY_PLACE_HOLDER_MAC(OAM_ACTIVE_MEMORY_PLACE_HOLDER_MASK_CNS, offset , writeTableFunc , 0), \
    ACTIVE_WRITE_MEMORY_PLACE_HOLDER_MAC(OAM_ACTIVE_MEMORY_PLACE_HOLDER_MASK_CNS, offset , writeTableFunc , 1)


/* init the active entry of read/write --> entry that was represented by 'place holder' */
#define GENERIC_ACTIVE_MEM_ENTRY_PLACE_HOLDER_INIT_MAC(dev,unitName,entryPtr)       \
    {                                                                               \
        /*read function should hold index ... regardless to read/write entry*/      \
        GT_U32  _tmpIndex = (entryPtr)->readFunParam;                               \
        (entryPtr)->address += dev->memUnitBaseAddrInfo.unitName[_tmpIndex];        \
        (entryPtr)->mask = SMEM_FULL_MASK_CNS;                                      \
    }


/*******************************************************************************
*   smemChtInit
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
void smemChtInit
(
    IN SKERNEL_DEVICE_OBJECT * deviceObj
);

/*******************************************************************************
*   smemChtInit2
*
* DESCRIPTION:
*       Init memory module for a Cht device - after the load of the default
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
void smemChtInit2
(
    IN SKERNEL_DEVICE_OBJECT * deviceObj
);

/*******************************************************************************
*   smemChtFatalError
*
* DESCRIPTION:
*       function for non-bound memories
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       address     - address of memory(register or table).
*       memSize     - size of the requested memory
*       param       - extra parameter might be used in
*
* OUTPUTS:
*       None.
*
* RETURNS:
*        pointer to the memory location
*        NULL - if memory not exist
*
* COMMENTS:
*
*
*******************************************************************************/
GT_U32 *  smemChtFatalError(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);

/*******************************************************************************
*   smemChtInitInternalSimMemory
*
* DESCRIPTION:
*       Allocate and init the internal simulation memory
*
* INPUTS:
*       devMemInfoPtr   - pointer to device memory object.
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
void smemChtInitInternalSimMemory
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_CHT_DEV_COMMON_MEM_INFO  * commonDevMemInfoPtr
);

/*******************************************************************************
*   smemChtPhyReg
*
* DESCRIPTION:
*       PHY related Registers access
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       address     - address of memory(register or table).
*       memSize     - size of the requested memory
*       param       - extra parameter might be used in
*
* OUTPUTS:
*       None.
*
* RETURNS:
*        pointer to the memory location
*        NULL - if memory not exist
*
* COMMENTS:
*
*
*******************************************************************************/
GT_U32 *  smemChtPhyReg(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 address,
    IN GT_U32 memSize
);

/*******************************************************************************
*   smemConvertChunkIndexToPointerAsParam
*
* DESCRIPTION:
*       convert the index of chunk memory to a pointer to the memory chunk,
*       and then convert it to GT_U32 to be used as parameter for generic chunk
*       accessing
*
* INPUTS:
*       devMemInfoPtr   - pointer to device memory object.
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
GT_UINTPTR  smemConvertChunkIndexToPointerAsParam
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32                  index /*not the unitId*/
);

/*******************************************************************************
*  smemChtTableInfoSet
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
void smemChtTableInfoSet
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr
);

/*******************************************************************************
*   smemChtMemReset
*
* DESCRIPTION:
*       Reset memory module with default values for a Cht device.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
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
void smemChtMemReset
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
);

/*******************************************************************************
*   smemChtSetInternalSimMemory
*
* DESCRIPTION:
*       Set the internal simulation memory to it's default
*
* INPUTS:
*       devMemInfoPtr           - pointer to device memory object.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
void smemChtSetInternalSimMemory
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
);

/*******************************************************************************
*   smemChtPolicerTableIndexCheck
*
* DESCRIPTION:
*       Perform Ingress policer table index out-of-range check.
*       Used only when Policer Memory Control supported
*       Do a fatal error if index is out-of-range.
*
* INPUTS:
*       devObjPtr - pointer to device object.
*       cycle     - 0 - iplr0, 1 - iplr1, 2 - egress policer stage
*       entryIndex  - index of ingress policer metering or counters table entry
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       0 - index is out-of-range
*       1 - index is ok (or Policer Memory Control not supported)
* COMMENTS:
*       Without Policer Memory Control - index is checked by another mechanize
*******************************************************************************/
GT_U8 smemChtPolicerTableIndexCheck
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 cycle,
    IN GT_U32 entryIndex
);

/*******************************************************************************
*   smemChtInitPciRegistres
*
* DESCRIPTION:
*       Init PCI registers : devId,revision for a device - to the 0x4c register
*
* INPUTS:
*       devObjPtr   - pointer to device object.
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
void smemChtInitPciRegistres
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
);

/*******************************************************************************
* smemChtInitStateSet
*
* DESCRIPTION:
*       State init status to global control registers
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
void smemChtInitStateSet
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr
);

/*******************************************************************************
*  smemChtRegDbPreInit
*
* DESCRIPTION:
*       set reg DB info for the device --> from SMEM_CHT_MAC_REG_DB_GET(devObjPtr)
*       all value to SMAIN_NOT_VALID_CNS
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
void smemChtRegDbPreInit
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr
);

/*******************************************************************************
*  smemChtActiveReadRocRegister
*
* DESCRIPTION:
*       Read register and then clear it's data
*
* INPUTS:
*       devObjPtr   - device object PTR.
*       memPtr      - pointer to the register's memory in the simulation.
*
* OUTPUTS:
*       outMemPtr   - pointer to the memory to copy register's content.
* RETURNS:
*
* COMMENTS:
*       Generic function is used in active memory read functions for ROC registers.
*
*******************************************************************************/
void smemChtActiveReadRocRegister
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32 * memPtr,
    OUT        GT_U32 * outMemPtr
);

/*******************************************************************************
*  smemCheetahBackUpMemory
*
* DESCRIPTION:
*      Definition of backup/restore memory function
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       readWrite   - GT_TRUE  - backup device memory data
*                     GT_FALSE - restore device memory data
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
GT_VOID smemCheetahBackUpMemory
(
    IN SKERNEL_DEVICE_OBJECT *devObjPtr,
    IN GT_BOOL                readWrite
);

/*******************************************************************************
*   smemUnitBaseAddrByNameGet
*
* DESCRIPTION:
*       Get the base address of a unit by it's name (STRING).
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitNameStr - string of the name of the unit.
*       allowNonExistUnit - indication to allow not exists unit ..
*                  when != 0
*                       if unit not exists return SMAIN_NOT_VALID_CNS
*                  when == 0
*                       if unit not exists --> fatal error
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       base address of unit .
*       returns SMAIN_NOT_VALID_CNS for not valid name for the device.
*
* COMMENTS:
*
*******************************************************************************/
GT_U32 smemUnitBaseAddrByNameGet
(
    IN SKERNEL_DEVICE_OBJECT_T_PTR devObjPtr,
    IN GT_CHAR*                    unitNameStr,
    IN GT_BIT                      allowNonExistUnit
);

/*******************************************************************************
*   smemUnitBaseAddrByIndexGet
*
* DESCRIPTION:
*       Get the base address of a unit by it's index in
*       devObjPtr->genericUsedUnitsAddressesArray[] .
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       addressIndex - index in devObjPtr->genericUsedUnitsAddressesArray[]
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       base address of unit .
*       returns SMAIN_NOT_VALID_CNS for not valid name for the device.
*
* COMMENTS:
*
*******************************************************************************/
GT_U32 smemUnitBaseAddrByIndexGet
(
    IN SKERNEL_DEVICE_OBJECT_T_PTR devObjPtr,
    IN GT_U32                      addressIndex
);

/*******************************************************************************
*   smemUnitNameByAddressGet
*
* DESCRIPTION:
*       Get the name of the unit by the address .
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       address     - address to get unit name for.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       name of unit or NULL
*
* COMMENTS:
*
*******************************************************************************/
GT_CHAR* smemUnitNameByAddressGet
(
    IN SKERNEL_DEVICE_OBJECT_T_PTR devObjPtr,
    IN GT_U32                      address
);

/*******************************************************************************
* smemSip5RegsInfoSet
*
* DESCRIPTION:
*       Init memory module for Sip5 DB devices.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
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
void smemSip5RegsInfoSet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
);

/*******************************************************************************
* smemDfxServerRegsInfoSet
*
* DESCRIPTION:
*       Init memory module for DFX server DB devices.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
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
void smemDfxServerRegsInfoSet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
);

/*******************************************************************************
* smemInitRegistersWithDefaultValues
*
* DESCRIPTION:
*       Init registers with default values.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       accessType  - memory access type
*       linkListRegistersDefaultsPtr - (pointer to) link list of arrays of registers with default values.
*       specificUnitPtr - (pointer to) specific unit info in case of 'per unit'.
*                           can be NULL (not 'per port')
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
void smemInitRegistersWithDefaultValues
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN SMEM_LINK_LIST_REGISTER_DEFAULT_VALUE_STC  *linkListRegistersDefaultsPtr,
    IN SMEM_UNIT_CHUNKS_STC*   specificUnitPtr
);

/*******************************************************************************
*   smemMibCounterInfoByAddrGet
*
* DESCRIPTION:
*       get info about mib counter according to associated address .
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       address     - the address in the MIB
* OUTPUTS:
*       infoPtr     - pointer to the needed info.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
void smemMibCounterInfoByAddrGet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32                   address,
    OUT SKERNEL_MIB_COUNTER_INFO_STC   *infoPtr
);

/*******************************************************************************
*   smemGopPortByAddrGet
*
* DESCRIPTION:
*       get port id according to associated address .
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       address     - the address in the address space of the port
* OUTPUTS:
*       none.
*
* RETURNS:
*
* COMMENTS:
*       the port id
*
*******************************************************************************/
GT_U32 smemGopPortByAddrGet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32                   address
);


/*******************************************************************************
*   smemMibCounterAddrByPortGet
*
* DESCRIPTION:
*       get start address of MIB counter memory.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       portNum     - the port number for the MIB
* OUTPUTS:
*       None
*
* RETURNS:
*       start address of MIB counter memory
* COMMENTS:
*
*
*******************************************************************************/
GT_U32 smemMibCounterAddrByPortGet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32                   portNum
);

/*******************************************************************************
* smemMultiDpUnitIndexGet
*
* DESCRIPTION:
*       Get DP unit according to the port number.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*        portNum        - the port number
* OUTPUTS:
*       None.
*
* RETURNS:
*       return the index of the DP unit
* COMMENTS:
*
*
*******************************************************************************/
GT_U32 smemMultiDpUnitIndexGet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32   portNum
);

/*******************************************************************************
* smemMultiDpUnitRelativePortGet
*
* DESCRIPTION:
*       Get DP local port number according to the global port number.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*        portNum  - the global port number (in the Pipe)
* OUTPUTS:
*       None.
*
* RETURNS:
*       return the local port number
* COMMENTS:
*
*
*******************************************************************************/
GT_U32 smemMultiDpUnitRelativePortGet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32   portNum
);


/*******************************************************************************
* smemCheetahDuplicateUnits
*
* DESCRIPTION:
*       do duplications of the units. according to unitsDuplicationsPtr.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
* OUTPUTS:
*       None.
*
* RETURNS:
*       None.
* COMMENTS:
*
*
*******************************************************************************/
void smemCheetahDuplicateUnits
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
);

/*******************************************************************************
* smemCheetahUpdateCoreClockRegister
*
* DESCRIPTION:
*       Update the value of the Core Clock in the register.
*       there are 2 options:
*       1. give 'coreClockInMHz' which will be translated to proper 'hw field value'
*           and then written to the HW.
*       2. give 'hwFieldValue' which written directly to the HW.
*
* INPUTS:
*       deviceObjPtr         - the device pointer
*       coreClockInMHz  - core clock in MHz.
*                           value SMAIN_NOT_VALID_CNS means ignored
*       hwFieldValue -  value to set to the core clock field in the HW.
*                           NOTE: used only when coreClockInMHz == SMAIN_NOT_VALID_CNS.
* OUTPUTS:
*       refCoreClkPtr  - Pp's reference core clock in MHz
*       coreClkPtr     - Pp's core clock in MHz
*
* RETURNS:
*       GT_OK        - on success,
*       GT_BAD_STATE - can't map HW value to core clock value.
*       GT_FAIL      - otherwise.
*
* COMMENTS:
*
*******************************************************************************/
void smemCheetahUpdateCoreClockRegister
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN GT_U32                  coreClockInMHz,
    IN GT_U32                  hwFieldValue
);


/* active functions for Write */
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemChtActiveWriteForceLinkDown);
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemChtActiveWriteForceLinkDownXg);
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemChtActiveAuqBaseWrite);
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemChtActiveWriteIntrCauseReg);
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemChtActiveWriteMibCntCtrl);
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemChtActiveWriteHgsMibCntCtrl);
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemChtActiveWriteInterruptsMaskReg);
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemChtActiveWritePortInterruptsMaskReg);
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemChtActiveWriteRcvSdmaInterruptsMaskReg);
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemChtActiveWriteBridgeInterruptsMaskReg);
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemChtActiveWriteHgsInterruptsMaskReg);
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemChtActiveWriteMacInterruptsMaskReg);
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemChtActiveWriteTransSdmaInterruptsMaskReg);
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemChtActiveWriteMiscInterruptsMaskReg);
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemChtActiveGenericWriteInterruptsMaskReg);
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemChtActiveWriteRedirect);
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemChtActiveWriteGlobalReg);
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemChtActiveWriteSdmaConfigReg);
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemChtActiveWriteStartFromCpuDma);
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemChtActiveWriteIngrStcTbl);
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemChtActiveWriteTxQuCtrlReg);
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemChtActiveWriteTxQConfigReg);
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemChtActiveWriteNewFdbMsgFromCpu);
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemChtActiveWriteFdbActionTrigger);
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemChtActiveWriteXSmii);
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemChtActiveWriteArpTable);
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemChtActiveWriteVlanQos           );
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemChtActiveWritePclId             );
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemChtActiveWriteTrunkTbl          );
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemChtActiveWriteQosAccess         );
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemChtActiveWriteCpuCodeAccess     );
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemChtActiveWriteStatRateLimTbl    );
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemChtActiveWriteSdmaCommand       );
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemChtActiveWriteMacInterruptsMaskReg );
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemChtActiveWriteFdbMsg);
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemChtActiveWritePolicerTbl);
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemChtActiveWritePolicerQos);
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemChtActiveWritePolicerCnt);
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemChtActiveWriteSmi);
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemChtActiveWriteToReadOnlyReg);

/* active functions for Read */
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemChtActiveReadIntrCauseReg);
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemChtActiveReadCntrs);
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemChtActiveReadRedirect);
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemChtActiveReadMsg2Cpu);
ACTIVE_READ_FUNC_PROTOTYPE_MAC (smemChtActiveReadBreachStatusReg);
ACTIVE_READ_FUNC_PROTOTYPE_MAC (smemChtActiveReadRateLimitCntReg);

ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemChtActiveReadCntrs64Bit);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __smemChth */




