/******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* smemLion2.c
*
* DESCRIPTION:
*       Lion2 memory mapping implementation: the 'port group' and the shared memory
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 136 $
*
*******************************************************************************/
#include <asicSimulation/SInit/sinit.h>
#include <asicSimulation/SKernel/smem/smemLion2.h>
#include <asicSimulation/SKernel/cheetahCommon/sregCheetah.h>
#include <asicSimulation/SKernel/cheetahCommon/sregLion2.h>
#include <asicSimulation/SKernel/suserframes/snetCheetah3Pcl.h>
#include <asicSimulation/SKernel/smem/smemBobcat2.h>
#include <common/Utils/Math/sMath.h>

#define _1K         1024

/*need to support 72 ports for bobcat2 */
#define BOBCAT2_NUM_OF_PORTS    72

#define LION3_NUM_E_PORTS_BITS_CNS   (12)
#define LION3_NUM_E_VLANS_BITS_CNS   (13)

#define LION3_NUM_E_PORTS_CNS   (1<<LION3_NUM_E_PORTS_BITS_CNS)
#define LION3_NUM_E_VLANS_CNS   (1<<LION3_NUM_E_VLANS_BITS_CNS)

#define COPY_TO_OTHER_PORT(dev,unit,srcPort)                \
    if(dev->numOfPipes >= 2)                                \
    {                                                       \
        GT_U32  ii;                                         \
        for(ii = 1 ; ii < dev->numOfPipes; ii++)            \
        {                                                   \
            regAddrDbPtr->GOP.unit[(srcPort) + (ii * dev->numOfPortsPerPipe)] = \
                regAddrDbPtr->GOP.unit[(srcPort)];    \
        }                                                   \
    }

/* the port group# that used by CPU to access the shared memory -->
   the PEX address is the same one  */
#define PORT_GROUP_ID_FOR_PORT_GROUP_SHARED_ACCESS_CNS  0

#define LION2_IP_ROUT_TCAM_CNS   16*1024

/* do not INIT memory for core that is not the single instance */
#define SINGLE_INSTANCE_UNIT_MAC(dev,coreId)    \
    if(dev->portGroupId != coreId)          \
    {                                       \
        return;                             \
    }

/* get device memory of other port group */
#define GET_DEVICE_MEMORY_OF_PORT_GROUP_MAC(dev,portGroupId)    \
    ((dev)->portGroupSharedDevObjPtr ?                          \
        ((dev)->portGroupSharedDevObjPtr->coreDevInfoPtr[portGroupId].devObjPtr)->deviceMemory :    \
        (dev)->deviceMemory)

/* get device object of other port group */
#define GET_DEVICE_OBJ_OF_PORT_GROUP_MAC(dev,portGroupId)    \
    ((dev)->portGroupSharedDevObjPtr ?                          \
        ((dev)->portGroupSharedDevObjPtr->coreDevInfoPtr[portGroupId].devObjPtr) :    \
        (dev))

static SKERNEL_DEVICE_ROUTE_TCAM_INFO_STC  lion2PortGroupRoutTcamInfo =
{
    LION2_IP_ROUT_TCAM_CNS,
    LION2_IP_ROUT_TCAM_CNS / 4,
    1,
    4,
    16
};

static SKERNEL_DEVICE_TCAM_INFO_STC  lion2TtiTrillAdjacencyTcanInfo =
{
    256,
    1, /* the banks are consecutive*/
    4, /* the words are consecutive */
    2, /* number of banks to compare : 2 X bank and also 2 Y banks */
    52/* number of bits to compare :
    X part 1 in bank 0:
    48 +  Outer.SA
    == 48
    2  -  reserved
    2  - control bits
    ====== 52

    X part 2 in bank 0:
    1 +   TRILL.M
    16 +  TRILL.E-RBID
    12 +  Source Device
    1 +   Source Is Trunk
    20 +  Source Trunk-ID/ePort
    == 50
    2  - control bits
    ====== 52

    Y part 0 in bank 2
    Y part 1 in bank 3
    */
    ,
    SMAIN_NOT_VALID_CNS,/*xMemoryAddress*/
    SMAIN_NOT_VALID_CNS,/*yMemoryAddress*/
    snetLion2TtiTrillAdjacencyLookUpKeyWordBuild/*lookUpKeyWordBuildPtr*/
};

#define  REG_PTR_AND_SIZE_MAC(regArr) \
    (GT_U32*)(void*)((((SMEM_CHT_PP_REGS_UNIT_START_INFO_STC*)(void*)&regArr) + 1)) , ((sizeof(regArr) - sizeof(SMEM_CHT_PP_REGS_UNIT_START_INFO_STC))/sizeof(GT_U32))

static GT_U32 policerBaseAddr[3] = {0x0C000000,0x00800000,0x07800000};
/* define policer memory with 1792 entries */
#define POLICER_MEMORY_1792_CNS         1792
/* define policer memory with 256 entries */
#define POLICER_MEMORY_256_CNS         256

static GT_U32 timestampBaseAddr_lion2[2] = {0x01002000,0x0E800800};

/* Timestamp ingress and egress base addresses */
static GT_U32 *timestampBaseAddr = timestampBaseAddr_lion2;

/* Active memory table */
static SMEM_ACTIVE_MEM_ENTRY_STC smemLion2PortGroupActiveTable[] =
{
    /* Global interrupt cause register */
    {0x00000030, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteIntrCauseReg, 0},

    /* Port<n> Auto-Negotiation Configuration Register */
    {0x0880000C, 0xFFFC0FFF, NULL, 0 , smemChtActiveWriteForceLinkDown, 0},
    /* Port<n> Interrupt Cause Register  */
    {0x08800020, 0xFFFC0FFF,
        smemChtActiveReadIntrCauseReg, 18, smemChtActiveWriteIntrCauseReg, 0},
    /* Tri-Speed Port<n> Interrupt Mask Register */
    {0x08800024, 0xFFFC0FFF, NULL, 0, smemChtActiveWritePortInterruptsMaskReg, 0},

    /* Receive SDMA Interrupt Cause Register (RxSDMAInt) */
    {0x0000280C, SMEM_FULL_MASK_CNS,
        smemChtActiveReadIntrCauseReg, 22, smemChtActiveWriteIntrCauseReg, 0},
    /* Receive SDMA Interrupt Mask Register */
    {0x00002814, SMEM_FULL_MASK_CNS,
        NULL, 0, smemChtActiveWriteRcvSdmaInterruptsMaskReg, 0},

    /* Bridge Interrupt Cause Register */
    {0x02040130, SMEM_FULL_MASK_CNS,
        smemChtActiveReadIntrCauseReg, 14, smemChtActiveWriteIntrCauseReg, 0},
    /* Bridge Interrupt Mask Register */
    {0x02040134, SMEM_FULL_MASK_CNS,
        NULL, 0, smemChtActiveWriteBridgeInterruptsMaskReg, 0},

    /* Security Breach Status Register2 */
    {0x020401A8, SMEM_FULL_MASK_CNS, smemChtActiveReadBreachStatusReg, 0, NULL, 0},

    /* Set Incoming Packet Count */
    {0x020400E0, 0xFFFFFFF0, smemChtActiveReadCntrs, 0, NULL,0}, /* Set #0 counters */
    {0x020400F4, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0}, /* Set #1 Incoming counters */
    {0x020400F8, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0}, /* Set #1 VLAN ingress filtered counters */
    {0x020400FC, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0}, /* Set #1 Security filtered counters */
    {0x02040100, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0}, /* Set #1 Bridge filtered counters */

    /* XG port MAC MIB Counters */
    {0x09000000, SMEM_LION2_XG_MIB_COUNT_MSK_CNS, smemLion2ActiveReadMsmMibCounters, 0, NULL,0},

    /* COMPHY_H %t Registers/KVCO Calibration Control (0x09800200 + t*0x400: where t (0-23) represents SERDES) */
    {0x09800208, 0xFFFF03FF, NULL, 0, smemLion2ActiveWriteKVCOCalibrationControlReg, 0},

    /* Host Incoming Packets Count */
    {0x020400BC, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},
    /* Host Outgoing Packets Count */
    {0x020400C0, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},
    /* Host Outgoing Multicast Packet Count */
    {0x020400CC, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},
    /* Host Outgoing Broadcast Packet Count */
    {0x020400D0, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},

    /* Message to CPU register  */
    {0x06000034, SMEM_FULL_MASK_CNS, smemChtActiveReadMsg2Cpu, 0 , NULL,0},

    /* The SDMA packet count registers */
    {0x00002820, 0xFFFFFFF0, smemChtActiveReadCntrs, 0, NULL,0},
    {0x00002830, 0xFFFFFFF0, smemChtActiveReadCntrs, 0, NULL,0},

     /* The SDMA byte count registers */
    {0x00002840, 0xFFFFFFF0, smemChtActiveReadCntrs, 0, NULL,0},
    {0x00002850, 0xFFFFFFF0, smemChtActiveReadCntrs, 0, NULL,0},

    /* The Egress packet count registers */
    {0x10093210, 0xFFFFFFF0, smemChtActiveReadCntrs, 0, NULL,0},
    {0x10093220, 0xFFFFFFF0, smemChtActiveReadCntrs, 0, NULL,0},
    {0x10093230, 0xFFFFFFF0, smemChtActiveReadCntrs, 0, NULL,0},
    {0x10093240, 0xFFFFFFF0, smemChtActiveReadCntrs, 0, NULL,0},
    {0x10093250, 0xFFFFFFF0, smemChtActiveReadCntrs, 0, NULL,0},
    {0x10093260, 0xFFFFFFF0, smemChtActiveReadCntrs, 0, NULL,0},
    {0x10093270, 0xFFFFFFF0, smemChtActiveReadCntrs, 0, NULL,0},
    {0x10093280, 0xFFFFFFF0, smemChtActiveReadCntrs, 0, NULL,0},

    /* The SDMA resource error registers */
    {0x00002860, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},
    {0x00002864, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},

    /* Mac Table Access Control Register */
    {0x06000064, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteFdbMsg,0},

    /* Address Update Queue Base Address */
    {0x000000C0, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveAuqBaseWrite, 0},

    /* FDB Upload Queue Base Address */
    {0x000000C8, SMEM_FULL_MASK_CNS, NULL, 0 , smemCht2ActiveFuqBaseWrite, 0},

    {0x0D000138, SMEM_FULL_MASK_CNS, NULL, 0 , smemCht3ActiveWritePclAction, 0},

    /* Router and TT TCAM Access Control 1 Register  */
    {0x0D80041C, SMEM_FULL_MASK_CNS, NULL, 0 , smemCht3ActiveWriteRouterAction, 0},

    /* Policers Table Access Control Register */
    POLICER_ACTIVE_WRITE_MEMORY(0x00000070, smemXCatActiveWritePolicerTbl),

    /* Policer IPFIX memories */
    POLICER_ACTIVE_WRITE_MEMORY(0x00000014, smemXCatActiveWriteIPFixTimeStamp),
    POLICER_ACTIVE_READ_MEMORY(0x00000218, smemXCatActiveReadIPFixNanoTimeStamp),  /*part of Policer Timer Memory */
    POLICER_ACTIVE_READ_MEMORY(0x0000021C, smemXCatActiveReadIPFixSecLsbTimeStamp),/*part of Policer Timer Memory */
    POLICER_ACTIVE_READ_MEMORY(0x00000220, smemXCatActiveReadIPFixSecMsbTimeStamp),/*part of Policer Timer Memory */
    POLICER_ACTIVE_READ_MEMORY(0x00000048, smemXCatActiveReadIPFixSampleLog),
    POLICER_ACTIVE_READ_MEMORY(0x0000004C, smemXCatActiveReadIPFixSampleLog),

    /* Policer : when Read a Data Unit counter part(Entry LSB) of Management Counters Entry.
                 then value of LSB and MSB copied to Shadow registers */
    POLICER_ACTIVE_READ_MEMORY(POLICER_MANAGEMENT_COUNTER_ADDR_CNS, smemXCatActiveReadPolicerManagementCounters),

    /* Policer: when write iplr0 Hierarchical Policer Control register, "Policer memory control" configuration is
                            changed and ingress policer tables are redistributed between plr0,1 accordingly */
    {0x0C00002C, SMEM_FULL_MASK_CNS, NULL, 0 , smemLion2ActiveWriteHierarchicalPolicerControl,  0},

    /* iplr0 policer table 0x0c040000 - 0x0c04FFFF  Lion */
    {0x0c040000, 0xFFFF0000, smemXCatActiveReadIplr0Tables, 0 , smemXCatActiveWriteIplr0Tables,  0},
    /* iplr0 policerCounters table 0x0c060000 - 0x0c06FFFF  Lion */
    {0x0c060000, 0xFFFF0000, smemXCatActiveReadIplr0Tables, 0 , smemXCatActiveWriteIplr0Tables,  0},

    /* iplr1 policer table 0x00840000 - 0x0084FFFF  Lion */
    {0x00840000, 0xFFFF0000, smemXCatActiveReadIplr1Tables, 0 , smemXCatActiveWriteIplr1Tables,  0},
    /* iplr1 policerCounters table 0x00860000 - 0x0086FFFF  Lion */
    {0x00860000, 0xFFFF0000, smemXCatActiveReadIplr1Tables, 0 , smemXCatActiveWriteIplr1Tables,  0},

    /* Trunk table Access Control Register */
    {0x0B000028, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteTrunkTbl, 0},

    /* QoSProfile to QoS Access Control Register */
    {0x0B00002C, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteQosAccess, 0},

    /* CPU Code Access Control Register */
    {0x0B000030, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteCpuCodeAccess, 0},

    /* Statistical Rate Limits Table Access Control Register */
    {0x0B000034, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteStatRateLimTbl, 0},

    /* Ingress STC Table Access Control Register */
    {0x0B000038, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteIngrStcTbl, 0},

    /*ingress STC interrupt register*/
    {0x0B000020, SMEM_FULL_MASK_CNS, smemChtActiveReadIntrCauseReg, 20 , smemChtActiveWriteIntrCauseReg,0},

    /* Receive SDMA Queue Command */
    {0x00002680, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteSdmaCommand,0},

    /* Message From CPU Management */
    {0x06000050, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteNewFdbMsgFromCpu, 0},

    /* MAC Table Action0 Register */
    {0x06000004, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteFdbActionTrigger, 0},

    /* CNC Fast Dump Trigger Register Register */
    {0x08000030, SMEM_FULL_MASK_CNS, NULL, 0 , smemCht3ActiveWriteCncFastDumpTrigger, 0},

    /* CPU direct read from the counters */
    {0x08080000, 0xFFF80000, smemCht3ActiveCncBlockRead, 0, NULL, 0},

    /* CNC Block Wraparound Status Register */
    {0x080010A0, 0xFFFFF0F0, smemCht3ActiveCncWrapAroundStatusRead, 0, NULL, 0},

    /* Transmit SDMA Queue Command Register */
    {0x00002868, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteStartFromCpuDma, 0},

    /* Port MAC Control Register0 */
    {0x088C0000, 0xFFFF0FFF, NULL, 0 , smemChtActiveWriteForceLinkDownXg, 0},
    /* XG Port<n> Interrupt Cause Register  */
    {0x088C0014, 0xFFFF0FFF,  smemChtActiveReadIntrCauseReg, 29, smemChtActiveWriteIntrCauseReg, 0},
    /* XG Port<n> Interrupt Mask Register */
    {0x088C0018, 0xFFFF0FFF, NULL, 0, smemChtActiveWritePortInterruptsMaskReg, 0},
    /* stack gig ports - Port<n> Interrupt Cause Register  */
    {0x088C0020, 0xFFFF0FFF,smemChtActiveReadIntrCauseReg, 18, smemChtActiveWriteIntrCauseReg, 0},
    /* stack gig ports - Tri-Speed Port<n> Interrupt Mask Register */
    {0x088C0024, 0xFFFF0FFF, NULL, 0, smemChtActiveWritePortInterruptsMaskReg, 0},
    /* stack gig ports - Port<n> Auto-Negotiation Configuration Register */
    {0x088C000C, 0xFFFF0FFF, NULL, 0 , smemChtActiveWriteForceLinkDown, 0},
    /*Port MAC Control Register3*/
    {0x088C001C, 0xFFFF0FFF, NULL, 0 , smemXcatActiveWriteMacModeSelect, 0},

    /* Packet generator Control Register 0 Register */
    {0x08980050, 0xFFFF0FFF, NULL, 0 , smemLion2ActiveWriteTgControl0Reg, 0},

    /* Global control register */
    {0x00000058, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteGlobalReg, 0},

    /* Transmit SDMA Interrupt Cause Register */
    {0x00002810, SMEM_FULL_MASK_CNS,
        smemChtActiveReadIntrCauseReg, 21 , smemChtActiveWriteIntrCauseReg,0},
    /* Transmit SDMA Interrupt Mask Register */
    {0x00002818, SMEM_FULL_MASK_CNS,
        NULL, 0, smemChtActiveWriteTransSdmaInterruptsMaskReg, 0},

    {0x06000018, SMEM_FULL_MASK_CNS,
        smemChtActiveReadIntrCauseReg, 17, smemChtActiveWriteIntrCauseReg, 0},
    /* MAC Table Interrupt Mask Register */
    {0x0600001C, SMEM_FULL_MASK_CNS,
        NULL, 0, smemChtActiveWriteMacInterruptsMaskReg, 0},

    /* read interrupts cause registers Misc -- ROC register */
    {0x00000038, SMEM_FULL_MASK_CNS,
        smemChtActiveReadIntrCauseReg, 11, smemChtActiveWriteIntrCauseReg, 0},

    /* Write Interrupt Mask MIsc Register */
    {0x0000003c, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteMiscInterruptsMaskReg, 0},

    /* Matrix Bridge Register */
    {0x020400d4, 0xFFFFFFFF, smemChtActiveReadCntrs, 0, NULL, 0},

    /* SDMA configuration register - 0x00002800 */
    {0x00002800, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteSdmaConfigReg, 0},

    /* FDB Global Configuration register */
    {0x06000000, SMEM_FULL_MASK_CNS, NULL, 0 , smemLion2ActiveWriteFDBGlobalCfgReg, 0},

    /* Scheduler Configuration Register */
    {0x11001000, SMEM_FULL_MASK_CNS, NULL, 0 , smemLionActiveWriteSchedulerConfigReg, 0},

    /* Dequeue enable Register */
    {0x11002000, 0xFFFFFF00, NULL, 0, smemLionActiveWriteDequeueEnableReg, 0},

    /* PEX Interrupt Cause Register */
    {0x000F1900, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteIntrCauseReg, 0x00FF1F1F},

    /* read interrupts cause registers CNC -- ROC register */
    {0x08000100, SMEM_FULL_MASK_CNS,
        smemChtActiveReadIntrCauseReg, 3, smemChtActiveWriteIntrCauseReg, 0},

    /* Write Interrupt Mask CNC Register */
    {0x08000104, SMEM_FULL_MASK_CNS, NULL, 0 , smemCht3ActiveWriteCncInterruptsMaskReg, 0},

    {0x0B0000A0, SMEM_FULL_MASK_CNS, NULL, 0 , smemXCatActiveWriteLogTargetMap,  0},

    {0x0D000208, SMEM_FULL_MASK_CNS, NULL, 0 , smemXCatActiveWritePolicyTcamConfig_0,  0},

    /* IEEE Reserved Multicast Configuration register reading */
    {0x02000810, 0xFFFF0FFF, smemXCatActiveReadIeeeMcConfReg, 0 , NULL,  0},
    {0x02000818, 0xFFFF0FFF, smemXCatActiveReadIeeeMcConfReg, 0 , NULL,  0},
    {0x02000820, 0xFFFF0FFF, smemXCatActiveReadIeeeMcConfReg, 0 , NULL,  0},
    {0x02000828, 0xFFFF0FFF, smemXCatActiveReadIeeeMcConfReg, 0 , NULL,  0},

    /* TOD Counter - Nanoseconds */
    GTS_ACTIVE_READ_MEMORY(0x10, smemLionActiveReadPtpTodNanoSeconds),
    /* TOD Counter - Seconds word 0)*/
    GTS_ACTIVE_READ_MEMORY(0x14, smemLionActiveReadPtpTodSeconds),
    /* TOD Counter - Seconds word 1*/
    GTS_ACTIVE_READ_MEMORY(0x18, smemLionActiveReadPtpTodSeconds),
    /* Global FIFO Current Entry [31:0] register reading*/
    GTS_ACTIVE_READ_MEMORY(0x30, smemLionActiveReadPtpMessage),
    /* Global FIFO Current Entry [63:32] register reading */
    GTS_ACTIVE_READ_MEMORY(0x34, smemLionActiveReadPtpMessage),
    /* Global Configurations */
    GTS_ACTIVE_WRITE_MEMORY(0x0, smemLionActiveWriteTodGlobalReg),

    /* SMI0 Management Register : 0x04004054  */
    {0x04004054, 0xFFFFFFFF, NULL, 0 , smemChtActiveWriteSmi,0},

    /* XSMI Management Register */
    {0x06E40000, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteXSmii, 0},

    /*Egress STC interrupt register*/
    {0x11000118, SMEM_FULL_MASK_CNS, smemChtActiveReadIntrCauseReg, 6 , smemChtActiveWriteIntrCauseReg,0},


/* regAddrDbPtr->EQ.preEgrEngineGlobalConfig.dualDeviceIDAndLossyDropConfig        */    {0x0B0000AC, 0xFFFFFFFF, NULL, 0 , smemLion2ActiveWriteEqDualDeviceIdReg,0},
/* SMEM_LION_TTI_INTERNAL_CONF_REG        */    {0x01000204, 0xFFFFFFFF, NULL, 0 , smemLion2ActiveWriteTtiInternalConfReg,0},
/* SMEM_CHT3_ROUTE_HA_GLB_CNF_REG         */    {0x0E800100, 0xFFFFFFFF, NULL, 0 , smemLion2ActiveWriteHaGlobalConfReg,0},

    /* CPU Port MIB counters registers */
    {0x00000060, 0xFFFFFFE0, smemChtActiveReadCntrs, 0, NULL,0},

    /* must be last anyway */
    {END_OF_TABLE, SMEM_FULL_MASK_CNS, NULL,0,NULL,0}
};

static GT_U32   smemLion2PortGroupActiveTableSizeOf =
    sizeof(smemLion2PortGroupActiveTable)/sizeof(smemLion2PortGroupActiveTable[0]);

/*******************************************************************************
 * Typedef: enum SMEM_LION_2_3_UNIT_NAME_ENT
 *
 * Description:
 *      Memory units names -- used only as index into genericUsedUnitsAddressesArray[]
 *
 *******************************************************************************/
typedef enum{
/* 0*/    SMEM_LION2_UNIT_MG_E,
/* 1*/    SMEM_LION2_UNIT_IPLR1_E,       /*not for lion3*/
/* 2*/    SMEM_LION2_UNIT_TTI_E,
/* 3*/    SMEM_LION2_UNIT_DFX_SERVER_E,
/* 4*/    SMEM_LION2_UNIT_L2I_E,
/* 5*/    SMEM_LION2_UNIT_IPVX_E,
/* 6*/    SMEM_LION2_UNIT_BM_E,
/* 7*/    SMEM_LION2_UNIT_BMA_E,
/* 8*/    SMEM_LION2_UNIT_LMS_E,
/* 9*/    SMEM_LION2_UNIT_FDB_E,
/*10*/    SMEM_LION2_UNIT_MPPM_E,
/*11*/    SMEM_LION2_UNIT_CTU0_E,
/*12*/    SMEM_LION2_UNIT_EPLR_E,        /*not for lion3*/
/*13*/    SMEM_LION2_UNIT_CNC_E,
/*14*/    SMEM_LION2_UNIT_GOP_E,
/*15*/    SMEM_LION2_UNIT_XG_PORT_MIB_E,
/*16*/    SMEM_LION2_UNIT_SERDES_E,
/*17*/    SMEM_LION2_UNIT_MPPM1_E,
/*18*/    SMEM_LION2_UNIT_CTU1_E,
/*19*/    SMEM_LION2_UNIT_EQ_E,
/*20*/    SMEM_LION2_UNIT_IPCL_E,        /* not for Lion3 */
/*21*/    SMEM_LION2_UNIT_IPLR_E,        /*not for lion3*/
/*22*/    SMEM_LION2_UNIT_MLL_E,         /* not for Lion3 */
/*23*/    SMEM_LION2_UNIT_IPCL_TCC_E,
/*24*/    SMEM_LION2_UNIT_IPVX_TCC_E,
/*25*/    SMEM_LION2_UNIT_EPCL_E,        /*not for lion3*/
/*26*/    SMEM_LION2_UNIT_HA_E,          /*not for lion3*/
/*27*/    SMEM_LION2_UNIT_RX_DMA_E,
/*28*/    SMEM_LION2_UNIT_TX_DMA_E,
/*29*/    SMEM_LION2_UNIT_TXQ_QUEUE_E,
/*30*/    SMEM_LION2_UNIT_TXQ_LL_E,
/*31*/    SMEM_LION2_UNIT_TXQ_DQ_E,
/*32*/    SMEM_LION2_UNIT_TXQ_SHT_E,      /*not for lion3*/
/*33*/    SMEM_LION2_UNIT_TXQ_EGR0_E,     /*not for lion3*/
/*34*/    SMEM_LION2_UNIT_TXQ_EGR1_E,     /*not for lion3*/
/*35*/    SMEM_LION2_UNIT_TXQ_DIST_E,
/*36*/    SMEM_LION2_UNIT_CPFC_E,

    SMEM_LION2_UNIT_LAST_E,

}SMEM_LION_2_UNIT_NAME_ENT;

static SMEM_UNIT_NAME_AND_INDEX_STC lion2UnitNameAndIndexArr[]=
{
    {STR(UNIT_MG),                         SMEM_LION2_UNIT_MG_E                    },
    {STR(UNIT_IPLR1),                      SMEM_LION2_UNIT_IPLR1_E                 },
    {STR(UNIT_TTI),                        SMEM_LION2_UNIT_TTI_E                   },
    {STR(UNIT_DFX_SERVER),                 SMEM_LION2_UNIT_DFX_SERVER_E            },
    {STR(UNIT_L2I),                        SMEM_LION2_UNIT_L2I_E                   },
    {STR(UNIT_IPVX),                       SMEM_LION2_UNIT_IPVX_E                  },
    {STR(UNIT_BM),                         SMEM_LION2_UNIT_BM_E                    },
    {STR(UNIT_BMA),                        SMEM_LION2_UNIT_BMA_E                   },
    {STR(UNIT_LMS),                        SMEM_LION2_UNIT_LMS_E                   },
    {STR(UNIT_FDB),                        SMEM_LION2_UNIT_FDB_E                   },
    {STR(UNIT_MPPM),                       SMEM_LION2_UNIT_MPPM_E                  },
    {STR(UNIT_CTU0),                       SMEM_LION2_UNIT_CTU0_E                  },
    {STR(UNIT_EPLR),                       SMEM_LION2_UNIT_EPLR_E                  },
    {STR(UNIT_CNC),                        SMEM_LION2_UNIT_CNC_E                   },
    {STR(UNIT_GOP),                        SMEM_LION2_UNIT_GOP_E                   },
    {STR(UNIT_XG_PORT_MIB),                SMEM_LION2_UNIT_XG_PORT_MIB_E           },
    {STR(UNIT_SERDES),                     SMEM_LION2_UNIT_SERDES_E                },
    {STR(UNIT_MPPM1),                      SMEM_LION2_UNIT_MPPM1_E                 },
    {STR(UNIT_CTU1),                       SMEM_LION2_UNIT_CTU1_E                  },
    {STR(UNIT_EQ),                         SMEM_LION2_UNIT_EQ_E                    },
    {STR(UNIT_IPCL),                       SMEM_LION2_UNIT_IPCL_E                  },
    {STR(UNIT_IPLR),                       SMEM_LION2_UNIT_IPLR_E                  },
    {STR(UNIT_MLL),                        SMEM_LION2_UNIT_MLL_E                   },
    {STR(UNIT_IPCL_TCC),                   SMEM_LION2_UNIT_IPCL_TCC_E              },
    {STR(UNIT_IPVX_TCC),                   SMEM_LION2_UNIT_IPVX_TCC_E              },
    {STR(UNIT_EPCL),                       SMEM_LION2_UNIT_EPCL_E                  },
    {STR(UNIT_HA),                         SMEM_LION2_UNIT_HA_E                    },
    {STR(UNIT_RX_DMA),                     SMEM_LION2_UNIT_RX_DMA_E                },
    {STR(UNIT_TX_DMA),                     SMEM_LION2_UNIT_TX_DMA_E                },
    {STR(UNIT_TXQ_QUEUE),                  SMEM_LION2_UNIT_TXQ_QUEUE_E             },
    {STR(UNIT_TXQ_LL),                     SMEM_LION2_UNIT_TXQ_LL_E                },
    {STR(UNIT_TXQ_DQ),                     SMEM_LION2_UNIT_TXQ_DQ_E                },
    {STR(UNIT_TXQ_SHT),                    SMEM_LION2_UNIT_TXQ_SHT_E               },
    {STR(UNIT_TXQ_EGR0),                   SMEM_LION2_UNIT_TXQ_EGR0_E              },
    {STR(UNIT_TXQ_EGR1),                   SMEM_LION2_UNIT_TXQ_EGR1_E              },
    {STR(UNIT_TXQ_DIST),                   SMEM_LION2_UNIT_TXQ_DIST_E              },
    {STR(UNIT_CPFC),                       SMEM_LION2_UNIT_CPFC_E                  },

    {NULL ,                                SMAIN_NOT_VALID_CNS                     }
};

/* the addresses of the units that the Lion 2 port group use */
static GT_U32   lion2UsedUnitsAddressesArray[SMEM_LION2_UNIT_LAST_E]=
{
/* SMEM_LION2_UNIT_MG_E,                      */ 0x00000000  /* 0 */,
/* SMEM_LION2_UNIT_IPLR1_E,                   */ 0x00800000  /* 1 */,  /*not for lion3*/
/* SMEM_LION2_UNIT_TTI_E,                     */ 0x01000000  /* 2 */,
/* SMEM_LION2_UNIT_DFX_SERVER_E               */ 0x01800000  /* 3 */,
/* SMEM_LION2_UNIT_L2I_E,                     */ 0x02000000  /* 4 */,
/* SMEM_LION2_UNIT_IPVX_E,                    */ 0x02800000  /* 5 */,
/* SMEM_LION2_UNIT_BM_E,                      */ 0x03000000  /* 6 */,
/* SMEM_LION2_UNIT_BMA_E,                     */ 0x03800000  /* 7 */,
/* SMEM_LION2_UNIT_LMS_E,                     */ 0x04000000  /*0x04800000,0x05000000,0x05800000*//* 8,9,10,11*/ ,
/* SMEM_LION2_UNIT_FDB_E,                     */ 0x06000000  /* 12*/,
/* SMEM_LION2_UNIT_MPPM_E,                    */ 0x06800000  /* 13*/,
/* SMEM_LION2_UNIT_CTU0_E,                    */ 0x07000000  /* 14*/,
/* SMEM_LION2_UNIT_EPLR_E,                    */ 0x07800000  /* 15*/,  /*not for lion3*/
/* SMEM_LION2_UNIT_CNC_E,                     */ 0x08000000  /* 16*/,
/* SMEM_LION2_UNIT_GOP_E,                     */ 0x08800000  /* 17*/,
/* SMEM_LION2_UNIT_XG_PORT_MIB_E,             */ 0x09000000  /* 18*/,
/* SMEM_LION2_UNIT_SERDES_E,                  */ 0x09800000  /* 19*/,
/* SMEM_LION2_UNIT_MPPM1_E,                   */ 0x0A000000  /* 20*/,
/* SMEM_LION2_UNIT_CTU1_E,                    */ 0x0A800000  /* 21*/,
/* SMEM_LION2_UNIT_EQ_E,                      */ 0x0B000000  /* 22*/,  /*not for lion3*/
/* SMEM_LION2_UNIT_IPCL_E,                    */ 0x0B800000  /* 23*/,  /*not for lion3*/
/* SMEM_LION2_UNIT_IPLR_E,                    */ 0x0C000000  /* 24*/,  /*not for lion3*/
/* SMEM_LION2_UNIT_MLL_E,                     */ 0x0C800000  /* 25*/,  /*not for lion3*/
/* SMEM_LION2_UNIT_IPCL_TCC_E,                */ 0x0D000000  /* 26*/,
/* SMEM_LION2_UNIT_IPVX_TCC_E,                */ 0x0D800000  /* 27*/,
/* SMEM_LION2_UNIT_EPCL_E,                    */ 0x0E000000  /* 28*/,  /*not for lion3*/
/* SMEM_LION2_UNIT_HA_E,                      */ 0x0E800000  /* 29*/,  /*not for lion3*/

/* SMEM_LION2_UNIT_RX_DMA_E,                  */ 0x0F000000  /* 30*/,
/* SMEM_LION2_UNIT_TX_DMA_E,                  */ 0x0F800000  /* 31*/,
/* SMEM_LION2_UNIT_TXQ_QUEUE_E,               */ 0x10000000  /* 32*/,
/* SMEM_LION2_UNIT_TXQ_LL_E,                  */ 0x10800000  /* 33*/,
/* SMEM_LION2_UNIT_TXQ_DQ_E,                  */ 0x11000000  /* 34*/,
/* SMEM_LION2_UNIT_TXQ_SHT_E,                 */ 0x11800000  /* 35*/,  /*not for lion3*/
/* SMEM_LION2_UNIT_TXQ_EGR0_E,                */ 0x12000000  /* 36*/,  /*not for lion3*/
/* SMEM_LION2_UNIT_TXQ_EGR1_E,                */ 0x12800000  /* 37*/,  /*not for lion3*/
/* SMEM_LION2_UNIT_TXQ_DIST_E,                */ 0x13000000  /* 38*/,
/* SMEM_LION2_UNIT_CPFC_E,                    */ 0x13800000  /* 39*/,

};

/*******************************************************************************
*   smemLion2SubUnitMemoryGet
*
* DESCRIPTION:
*       Get the port-group object for specific sub-unit.

* INPUTS:
*       devObjPtr   - pointer to device object.
*       accessType  - Memory access type PCI/Regular
*       address     - address of memory(register or table).
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       Pointer to object for specific subunit
*
* COMMENTS:
*       TxQ unit contains several sub-units, that can be classed into three categories:
*           - Sub-units that are duplicated per core — each sub-unit is configured
*             through the management interface bound to its core.
*           - Sub-units that serve a pair of cores, and thus have two instances.
*             Each of the instances is configured through a different management interface.
*           - Sub-units that have a single instance, and serve the entire device.
*
*******************************************************************************/
SKERNEL_DEVICE_OBJECT * smemLion2SubUnitMemoryGet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32                  address
)
{
    GT_U32 index;           /* Memory unit index */
    SKERNEL_DEVICE_OBJECT * sharedObjPtr = devObjPtr->portGroupSharedDevObjPtr;
    GT_U32  portGroupIdLsbMask = 0x3;
    GT_U32  portGroupIdMsbMask = 0x4;
    GT_U32  portGroupIdMsb;
    GT_U32  portGroupIdLsb;
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = devObjPtr->deviceMemory;
    GT_U32  unitBaseAddr;

    if(sharedObjPtr == NULL)
    {
        /* non multi-core device */
        return devObjPtr;
    }

    /* the Lion2 device hold 2 sets of TXQ units !
       each of those units control 4 cores  */

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        /* support 8 * 64 = 512 sub units !!! */
        /* not only 64 sub units as in previous device */
        index = address >> SMEM_CHT_UNIT_INDEX_FIRST_BIT_CNS;
    }
    else
    {
        /* Get memory subunit index */
        index =
            (address & REG_SPEC_FUNC_INDEX) >> SMEM_CHT_UNIT_INDEX_FIRST_BIT_CNS;
    }

    unitBaseAddr =  index << SMEM_CHT_UNIT_INDEX_FIRST_BIT_CNS;

    portGroupIdMsb = devObjPtr->portGroupId & portGroupIdMsbMask;
    portGroupIdLsb = devObjPtr->portGroupId & portGroupIdLsbMask;

    if(index >= SMEM_CHT_NUM_UNITS_MAX_CNS)
    {
        skernelFatalError("unitIndex[%d] >= max of [%d] \n" ,
            index,SMEM_CHT_NUM_UNITS_MAX_CNS);
    }

    if(devMemInfoPtr->unitMemArr[index].hugeUnitSupportPtr)
    {
        index =
            devMemInfoPtr->unitMemArr[index].hugeUnitSupportPtr->chunkIndex;

        /* check the 'actual unit' that represent the current unit */
    }


    /* Device is port group and accessed from SKERNEL scope */
    if (SMEM_ACCESS_SKERNEL_FULL_MAC(accessType))
    {
        if(devMemInfoPtr->unitMemArr[index].otherPortGroupDevObjPtr)
        {
            return devMemInfoPtr->unitMemArr[index].otherPortGroupDevObjPtr;
        }

        /* NOTE: the TXQ_SHT sub unit is valid on all port groups ,
        the simulation uses the memory of the 'egress core' */


        /* for TXQ_LL and for TXQ_Queue use the first core of the TXQ unit (hemisphere)*/
        if(unitBaseAddr == devObjPtr->specialUnitsBaseAddr.UNIT_TXQ_LL ||
           unitBaseAddr == devObjPtr->specialUnitsBaseAddr.UNIT_TXQ_QUEUE)
        {
         /* Get object pointer for subunit - single instance of ports group 0.
            The unit exists only in port group 0 and 4 (so all port groups access to it) */
            return sharedObjPtr->coreDevInfoPtr[portGroupIdMsb+0].devObjPtr;
        }
        else if(unitBaseAddr == devObjPtr->specialUnitsBaseAddr.UNIT_TXQ_DIST)
        {
         /* Get object pointer for subunit - single instance of ports group 0.
            The unit exists only in port group 0 (so all port groups access to it) */
            devObjPtr = sharedObjPtr->coreDevInfoPtr[0].devObjPtr;
        }
        else if(unitBaseAddr == devObjPtr->specialUnitsBaseAddr.UNIT_BMA ||
                unitBaseAddr == devObjPtr->specialUnitsBaseAddr.UNIT_CPFC)
        {
            /* Get object pointer for subunit - single instance of ports group 0.
            The unit exists only in port group 0 (so all port groups access to it) */
            return  sharedObjPtr->coreDevInfoPtr[0].devObjPtr;
        }
        else if(unitBaseAddr == devObjPtr->specialUnitsBaseAddr.UNIT_DFX_SERVER)
        {
            /* NOTE : during INIT the Device for port group 1 may be still NULL
                      if this memory accessed by the port group 0*/

            /* Get object pointer for subunit - single instance of ports group 1.
            The unit exists only in port group 1 (so all port groups access to it) */
            return sharedObjPtr->coreDevInfoPtr[1].devObjPtr;
        }

        if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            switch(address)
            {
                case 0x000000c0:/*active memory*/ /* NA Address Update Queue Base Address */
                case 0x000000c4:/*SMEM_CHT_AUQ_CTRL_REG(dev)*/ /* NA Address Update Queue Control      */
                    return sharedObjPtr->coreDevInfoPtr[LION3_UNIT_FDB_TABLE_SINGLE_INSTANCE_PORT_GROUP_CNS].devObjPtr;
                default:
                    break;
            }
        }
    }
    else
    {
        if(devMemInfoPtr->unitMemArr[index].otherPortGroupDevObjPtr &&
           devObjPtr != devMemInfoPtr->unitMemArr[index].otherPortGroupDevObjPtr)
        {
            skernelFatalError("smemLion2SubUnitMemoryGet: illegal memory access in port group [%d] to address [0x%8.8x] , address is valid on port group [%d] \n",
                              devObjPtr->portGroupId, address,
                              ((SKERNEL_DEVICE_OBJECT *)devMemInfoPtr->unitMemArr[index].otherPortGroupDevObjPtr)->portGroupId);
        }

        if(unitBaseAddr == devObjPtr->specialUnitsBaseAddr.UNIT_TXQ_LL ||
           unitBaseAddr == devObjPtr->specialUnitsBaseAddr.UNIT_TXQ_QUEUE)
        {
            if (portGroupIdLsb != 0)
            {
                /* The unit exists only in port groups 0 and 4 */
                skernelFatalError("smemLion2SubUnitMemoryGet: illegal memory access - port group %d (only port groups 0 and 4 valid), sub-unit TXQ_LL/TXQ_Queue, address [0x%8.8x] \n",
                                  devObjPtr->portGroupId, address);
            }
        }
        else if(unitBaseAddr == devObjPtr->specialUnitsBaseAddr.UNIT_TXQ_DIST)
        {
            if (devObjPtr->portGroupId != 0)
            {
                /* The unit exists only in port groups 0 */
                skernelFatalError("smemLion2SubUnitMemoryGet: illegal memory access - port group %d (only port groups 0 ), sub-unit TXQ_DIST, address [0x%8.8x] \n",
                                  devObjPtr->portGroupId, address);
            }
        }
        else if(unitBaseAddr == devObjPtr->specialUnitsBaseAddr.UNIT_BMA ||
                unitBaseAddr == devObjPtr->specialUnitsBaseAddr.UNIT_CPFC)
        {
            if (devObjPtr->portGroupId != 0)
            {
                /* The unit exists only in port groups 0 */
                skernelFatalError("smemLion2SubUnitMemoryGet: illegal memory access - port group %d (only port groups 0 ), sub-units BMA,CPFC, address [0x%8.8x] \n",
                                  devObjPtr->portGroupId, address);
            }
        }
        else if(unitBaseAddr == devObjPtr->specialUnitsBaseAddr.UNIT_DFX_SERVER)
        {
            if (devObjPtr->portGroupId != 1)
            {
                /* The unit exists only in port groups 1 */
                skernelFatalError("smemLion2SubUnitMemoryGet: illegal memory access - port group %d (only port groups 1 ), sub-unit SMEM_LION2_UNIT_DFX_SERVER_E, address [0x%8.8x] \n",
                                  devObjPtr->portGroupId, address);
            }
        }

        if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            if (devObjPtr->portGroupId != LION3_UNIT_FDB_TABLE_SINGLE_INSTANCE_PORT_GROUP_CNS)
            {
                switch(address)
                {
                    case 0x000000c0:/*active memory*/ /* NA Address Update Queue Base Address */
                    case 0x000000c4:/*SMEM_CHT_AUQ_CTRL_REG(dev)*/ /* NA Address Update Queue Control      */
                    skernelFatalError("smemLion2SubUnitMemoryGet: illegal memory access - port group %d (only port groups 0 ), for single FDB unit, address [0x%8.8x] \n",
                                      devObjPtr->portGroupId, address);
                        break;
                    default:
                        break;
                }
            }
        }

    }

    return devObjPtr;
}

/*******************************************************************************
*   smemLion2InitFuncArray
*
* DESCRIPTION:
*       Init specific functions array.
*
* INPUTS:
*       commonDevMemInfoPtr   - pointer to common device memory object.
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
void smemLion2InitFuncArray
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_CHT_DEV_COMMON_MEM_INFO  * commonDevMemInfoPtr
)
{
    GT_U32              ii,jj;
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr;
    GT_U32  maxUnits = devObjPtr->genericNumUnitsAddresses;
    GT_U32  unitBaseAddr;

    /* set by default that all units bind to 'Fatal error' */
    for (ii = 0; ii < SMEM_CHT_NUM_UNITS_MAX_CNS; ii++)
    {
        commonDevMemInfoPtr->specFunTbl[ii].specFun    = smemChtFatalError;
    }

    devMemInfoPtr = devObjPtr->deviceMemory;

    /* bind the units to the specific unit chunk with the generic function */
    for (ii = 0; ii < maxUnits ; ii++)
    {
        unitBaseAddr = smemUnitBaseAddrByIndexGet(devObjPtr,ii);

        if(unitBaseAddr == SMAIN_NOT_VALID_CNS)
        {
            continue;
        }

        /* calculate the unit ID as from the address of the unit in the array */
        jj = (unitBaseAddr >> SMEM_CHT_UNIT_INDEX_FIRST_BIT_CNS);

        /* NOTE: the PEX is no longer accessed via the MG unit !!!! */
        /* when devMemInfoPtr->PEX_UnitMem is used */
        /* so all units can get the generic function */
            commonDevMemInfoPtr->specFunTbl[jj].specFun    = smemDevFindInUnitChunk;

        /* set the parameter to be (casting of) the pointer to the unit chunk */
        commonDevMemInfoPtr->specFunTbl[jj].specParam  = (GT_UINTPTR)(void*)(&devMemInfoPtr->unitMemArr[jj]);
        devMemInfoPtr->unitMemArr[jj].chunkIndex = jj;
    }
}

/*******************************************************************************
*   smemLion2FindMem
*
* DESCRIPTION:
*       Return pointer to the register's or tables's memory.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       accessType  - Read/Write operation
*       address     - address of memory(register or table).
*       memsize     - size of memory
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
void * smemLion2FindMem
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32                  address,
    IN GT_U32                  memSize,
    OUT SMEM_ACTIVE_MEM_ENTRY_STC ** activeMemPtrPtr
)
{
    /* Return pointer to object for specific memory subunit */
    devObjPtr = smemLion2SubUnitMemoryGet(devObjPtr, accessType, address);

    return smemGenericFindMem(devObjPtr, accessType, address, memSize,
                                  activeMemPtrPtr);
}

/*******************************************************************************
*   smemLion2IsDeviceMemoryOwner
*
* DESCRIPTION:
*       Return indication that the device is the owner of the memory.
*       relevant to multi port groups where there is 'shared memory' between port groups.
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       address     - address of memory(register or table).
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_TRUE  -  the device is     the owner of the memory.
*       GT_FALSE -  the device is NOT the owner of the memory.
*
* COMMENTS:
*
*
*******************************************************************************/
GT_BOOL smemLion2IsDeviceMemoryOwner
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32                  address
)
{

    SKERNEL_DEVICE_OBJECT   *new_devObjPtr;

    /* Return pointer to object for specific memory subunit */
    new_devObjPtr = smemLion2SubUnitMemoryGet(devObjPtr, SKERNEL_MEMORY_READ_E, address);

    if(new_devObjPtr == devObjPtr)
    {
        /* the device own the address */
        return GT_TRUE;
    }
    else
    {
        /* the device NOT own the address */
        return GT_FALSE;
    }
}

/*******************************************************************************
*   smemLion2UnitMg
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Lion2 MG unit
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr     - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemLion2UnitMg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        /* see lion3 smemLion3UnitMg */
        return;
    }


    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000000 ,0x000000fc)}  /* Global Configuration */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000364 ,0x00000398)}  /* Address Decoding  */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000500 ,0x00000514)}  /* Confi Processor Global Configuration */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000600 ,0x00000608)}  /* Init Stage */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000668 ,0x00000668)}  /* MG Miscellaneous */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002600 ,0x00002880)}  /* SDMA */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00030000 , 32768)}      /* Conf Processor Memory */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00040000 ,0x00040014)}  /* XSMI */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00400000 ,0x0040001C)}  /* TWSI */
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
}

/*******************************************************************************
*   smemLion2UnitPex
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  PEX unit
*
* INPUTS:
*       commonDevMemInfoPtr - pointer to common device memory object.
*       unitPtr             - pointer to the unit chunk
*       pexBaseAddr         - PCI unit base address
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
void smemLion2UnitPex
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr,
    IN GT_U32 pexBaseAddr
)
{
    /* Pex2 Units */
    {
        GT_U32 i;
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000000, 0x00000024)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000002C, 0x00000034)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000003C, 0x00000044)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000050, 0x00000098)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000100, 0x00000128)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001804, 0x0000180C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001820, 0x00001824)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000182C, 0x00001834)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000183C, 0x00001844)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000184C, 0x00001854)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000185C, 0x00001864)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000186C, 0x00001870)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001880, 0x00001884)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000188C, 0x00001890)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000018B0, 0x000018B0)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000018C0, 0x000018C4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000018D0, 0x000018D0)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000018E0, 0x000018E8)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000018F8, 0x00001900)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001910, 0x00001910)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001A00, 0x00001A18)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001A20, 0x00001A28)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001A50, 0x00001A50)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001A60, 0x00001A64)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001A70, 0x00001A78)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001A80, 0x00001A88)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001A90, 0x00001A94)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001AA0, 0x00001AA8)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001AB0, 0x00001ABC)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001B00, 0x00001B00)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        for(i = 0; i < numOfChunks; i++)
        {
            chunksMem[i].memFirstAddr |= pexBaseAddr;
        }

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
}

/*******************************************************************************
*   policerTablesSupport
*
* DESCRIPTION:
*       manage Policer tables:
*                   1. iplr 0 - all tables
*                   2. iplr 1 - none
*                   3. eplr   - other names for meter+counting
*
* INPUTS:
*       numOfChunks   - pointer to device object.
*       unitPtr - pointer to the unit chunk
*       plrUnit - PLR unit
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void policerTablesSupport
(
    IN GT_U32  numOfChunks,
    INOUT SMEM_CHUNK_BASIC_STC  chunksMem[],
    IN SMEM_SIP5_PP_PLR_UNIT_ENT   plrUnit
)
{
    GT_BIT  eplr  = (plrUnit == SMEM_SIP5_PP_PLR_UNIT_EPLR_E) ? 1 : 0;
    GT_BIT  iplr1 = (plrUnit == SMEM_SIP5_PP_PLR_UNIT_IPLR_1_E) ? 1 : 0;
    GT_U32  ii;

    for(ii = 0 ; ii < numOfChunks ; ii ++)
    {
        if (iplr1)
        {
            /* make sure that table are not bound to iplr1 (only to iplr0) */
            chunksMem[ii].tableOffsetValid = 0;
            chunksMem[ii].tableOffsetInBytes = 0;
        }
        else if(eplr)
        {
            switch(chunksMem[ii].tableOffsetInBytes)
            {
                case FIELD_OFFSET_IN_STC_MAC(policer,SKERNEL_TABLES_INFO_STC):
                    chunksMem[ii].tableOffsetInBytes =
                        FIELD_OFFSET_IN_STC_MAC(egressPolicerMeters,SKERNEL_TABLES_INFO_STC);
                    break;
                case FIELD_OFFSET_IN_STC_MAC(policerCounters,SKERNEL_TABLES_INFO_STC):
                    chunksMem[ii].tableOffsetInBytes =
                        FIELD_OFFSET_IN_STC_MAC(egressPolicerCounters,SKERNEL_TABLES_INFO_STC);
                    break;
                default:
                    /* make sure that table are not bound to EPLR (only to iplr0) */
                    chunksMem[ii].tableOffsetValid = 0;
                    chunksMem[ii].tableOffsetInBytes = 0;
                    break;
            }
        }
    }
}

/*******************************************************************************
*   smemLion2UnitPolicerUnify
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  any of the 3 Policers:
*                   1. iplr 0
*                   2. iplr 1
*                   3. eplr
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr - pointer to the unit chunk
*       plrUnit - PLR unit
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemLion2UnitPolicerUnify
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr,
    IN SMEM_SIP5_PP_PLR_UNIT_ENT   plrUnit
)
{
    /* chunks with flat memory (no formulas) */
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000000 , 0x0000003C )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000048 , 0x00000054 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000060 , 0x00000068 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000070 , 0x00000070 )}
            /*registers -- not table/memory !! -- Policer Table Access Data<%n> */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00000074 ,8*4),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32,4),SMEM_BIND_TABLE_MAC(policerTblAccessData)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000100 , 0x00000110 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000120 , 0x00000124 )}
            /*Policer Timer Memory  */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00000200 , 36) , SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32,4),SMEM_BIND_TABLE_MAC(policerTimer)}
            /*Policer Descriptor Sample Memory  */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00000400 , 96) , SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32,4)}
            /*Policer Management Counters Memory  */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00000500 , 192), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(74, 16),SMEM_BIND_TABLE_MAC(policerManagementCounters)}
            /*IPFIX wrap around alert Memory  */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00000800 , 256), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32,4),SMEM_BIND_TABLE_MAC(policerIpfixWaAlert)}
            /*IPFIX aging alert Memory  */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00000900 , 256), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32,4),SMEM_BIND_TABLE_MAC(policerIpfixAgingAlert)}
            /*DFX table  */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00001000 , 2048)}
            /*registers -- not table/memory !! -- Port%p and Packet Type Translation Table*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00001800 , 32*4), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32,4),SMEM_BIND_TABLE_MAC(policerMeterPointer)}
            /*Policer Metering and Counting Memory  */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00040000 , 262144/2), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(229, 32),SMEM_BIND_TABLE_MAC(policer)}/* error in cider */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00060000 , 262144/2), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(229, 32),SMEM_BIND_TABLE_MAC(policerCounters)}/* error in cider */
            /*Ingress Policer Re-Marking Memory  */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00080000 , 2560), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(51, 8),SMEM_BIND_TABLE_MAC(policerQos)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        policerTablesSupport(numOfChunks,chunksMem,plrUnit);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }

    /* chunks with formulas */
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
             {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x000000C0 ,0)}, FORMULA_SINGLE_PARAMETER(8 , 0x4)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);
    }

}

/*******************************************************************************
*   smemLion2UnitIplr1
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  IPLR1 unit
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemLion2UnitIplr1
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    smemLion2UnitPolicerUnify(devObjPtr,unitPtr,SMEM_SIP5_PP_PLR_UNIT_IPLR_1_E);
}

/*******************************************************************************
*   smemLion2UnitTti
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  TTI unit
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemLion2UnitTti
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{

        /* chunks with flat memory (no formulas) */
        {
            SMEM_CHUNK_BASIC_STC  chunksMem[]=
            {
                 {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01000000 ,0x01000018 )}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01000020 ,0x01000038 )}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01000040 ,0x01000068 )}
                /* Trunk Hash */
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01000070 ,0x01000078 )}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01000098 ,0x01000120 )}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01000140 ,0x01000188 )}
                /* 0x01000190 */
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01000190 ,0x010001AC )}
                /* 0x010001BC */
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x010001BC , 4)}
                /* 0x010001C0 */
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x010001C0 ,0x010001CC )}
               /* TTI Internal configurations */
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01000200, 0x01000204)}
                /* Received Flow Control Packets Counter and Dropped Flow Control Packets Counter */
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x010014E0 ,0x010014E4 )}
                /* Port VLAN and QoS Configuration Table */
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x01001000 , 1024), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(73, 16),SMEM_BIND_TABLE_MAC(portVlanQosConfig) }
                /* TTI DFX Area */
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x01001800 , 2048)}
                /* Port Protocol VID and QoS Configuration Table */
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x01004000 , 8192), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(28, 8),SMEM_BIND_TABLE_MAC(portProtocolVidQoSConf)}
                /* TTI_GTS_SIP Unit */
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x01002000 ,2048 )}
                /* User Defined Bytes ID Memory */
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x01030000 ,256 )}
                /* MapToParameters */
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01000500 ,0x01000520 )}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x01000600 ,512 )}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x01000800 ,256 )}
            };
            GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

            smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
        }

        /* chunks with formulas */
        {
            SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
            {
                 {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01000080 ,0)}, FORMULA_SINGLE_PARAMETER(6 , 0x4)}
                /* VLAN Assignment */
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01000300 ,0)}, FORMULA_SINGLE_PARAMETER(4 , 0x4)}
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01000310 ,0)}, FORMULA_SINGLE_PARAMETER(32 , 0x4)}
                /* QoS Mapping Tables */
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01000400 ,0),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4),SMEM_BIND_TABLE_MAC(dscpToQoSProfile)}, FORMULA_SINGLE_PARAMETER(16 , 0x4)}
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01000440 ,0),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4),SMEM_BIND_TABLE_MAC(upToQoSProfile)}, FORMULA_THREE_PARAMETERS(2,0x4, 2,0x8, 2,0x10)}
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01000460 ,0),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4),SMEM_BIND_TABLE_MAC(expToQoSProfile)}, FORMULA_SINGLE_PARAMETER(2 , 0x4)}
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01000470 ,0),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4),SMEM_BIND_TABLE_MAC(cfiUpQoSProfileSelect)}, FORMULA_SINGLE_PARAMETER(16 , 0x4)}
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01000900 ,0),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4),SMEM_BIND_TABLE_MAC(dscpToDscpMap)}, FORMULA_SINGLE_PARAMETER(16 , 0x4)}
                /* TTI Descriptor to PCL Word<%n> Register */
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01001400 ,0)}, FORMULA_SINGLE_PARAMETER(38 , 0x4)}
                /* Wr_DMA Descriptor to TTI Word<%n> Register */
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01001498 ,0)}, FORMULA_SINGLE_PARAMETER(2 , 0x4)}

                /* MAC2ME Registers */ /* registers not table -- SMEM_BIND_TABLE_MAC(macToMe)*/
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01001600 ,0)}, FORMULA_TWO_PARAMETERS(4, 0x4 , 8 , 0x10)}
            };
            GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
            SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

            smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

            /*add the tmp unit chunks to the main unit */
            smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);
        }

        {
            SMEM_CHUNK_BASIC_STC  chunksMem[]=
            {
                /* User Defined Bytes Configuration Memory */
                 {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x01010000 , 384), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(255, 32),SMEM_BIND_TABLE_MAC(ipclUserDefinedBytesConf)}
                /* VLAN Translation Table */
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0100C000 , 16384), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(12, 4),SMEM_BIND_TABLE_MAC(ingressVlanTranslation)}
            };
            GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);
            SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

            smemInitMemChunk(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

            /*add the tmp unit chunks to the main unit */
            smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);
        }
    }



/*******************************************************************************
*   smemLion2UnitDfxServer
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  DFX Server unit
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemLion2UnitDfxServer
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    GT_U32  baseAddr = 0x01800000;
    GT_U32  ii;


    /* chunks with flat memory (no formulas) */
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x01800000 ,0xFF000 )}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        /* unify the base addresses */
        for(ii = 0 ; ii < numOfChunks ; ii++)
        {
            chunksMem[ii].memFirstAddr -= baseAddr;
        }

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
}

/*******************************************************************************
*   smemLion2UnitTxqEgr
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  TXQ_EGR unit
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr - pointer to the unit chunk
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
static void smemLion2UnitTxqEgr
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr,
    IN GT_U32   txqEgrId/*0 or 1*/
)
{
    GT_U32  ii;

    /* chunks with flat memory (no formulas) */
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            /* base of 0x01800000 (Global)*/
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000000, 0x00000000)}
            /* EGR RAMS TEST CTR */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000800, 0x00000810)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001000, 0x00001008)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001100, 0x00001100)}
            /* Egress Filters Global Enables */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00010000, 0x00010000)}
            /* Relayed Port Number */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00010100, 0x00010100)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }

    /* chunks with formulas */
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
            /* Global */
             {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x000000a0 ,0)}, FORMULA_SINGLE_PARAMETER(2 , 0x10)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00000074 ,0)}, FORMULA_SINGLE_PARAMETER(3 , 0x10)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00000070 ,0)}, FORMULA_SINGLE_PARAMETER(3 , 0x10)}
            /* MC FIFO */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00001010 ,0)}, FORMULA_TWO_PARAMETERS(2 , 0x4, 4, 0x8)}
            /* MC FIFO <%m> Parity Errors Counter */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00001030 ,0)}, FORMULA_SINGLE_PARAMETER(4 , 0x4)}
            /* Local Src/Trg Port Map Own Dev En */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00001110 ,0)}, FORMULA_TWO_PARAMETERS(2 , 0x4, 3, 0x8)}
            /* UN/BC/MC Egress Filters */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00010010 ,0)}, FORMULA_TWO_PARAMETERS(2 , 0x4, 9, 0x10)}

        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);
    }


    if(!SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        {
            SMEM_CHUNK_BASIC_STC  chunksMem[]=
            {
                /* Secondary Target Port Table */
                 {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00001300, 4*64), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(8, 4) , SMEM_BIND_TABLE_MAC(secondTargetPort)}
                /* Designated Port Table */
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00010200, 8*64), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(64, 8),SMEM_BIND_TABLE_MAC(designatedPorts)}
                /* Source ID Members Table */
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00010500, 8*64), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(64, 8),SMEM_BIND_TABLE_MAC(sst)}
                /* Non Trunk Members Table */
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00010800, 8*128), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(64, 8),SMEM_BIND_TABLE_MAC(nonTrunkMembers)}
            };
            GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);
            SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

            for(ii = 0 ; ii < numOfChunks ; ii ++)
            {
                if(txqEgrId != 0)
                {
                    /* those memories are not used by simulation by skernel (packet walk through)
                       so remove the bind to tables */
                    chunksMem[ii].tableOffsetValid = 0;
                }

            }

            smemInitMemChunk(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

            /*add the tmp unit chunks to the main unit */
            smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);

        }
    }
}

/*******************************************************************************
*   smemLion2UnitL2i
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Lion2  L2i unit
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr     - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemLion2UnitL2i
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
        {
            SMEM_CHUNK_BASIC_STC  chunksMem[]=
            {
                /* base of 0x02000000 */
                 {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02000200 ,0x02000218 )}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02000300 ,0x0200030c )}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02000500 ,0x0200052c )}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02000600 ,0x0200061c )}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02000700 ,0x02000728 )}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0200080C ,0x0200080C ),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 0x10),SMEM_BIND_TABLE_MAC(ieeeTblSelect)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0200081C ,0x0200081C )}

                /* base of 0x02010000 */
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02012800 ,0x02012800 )}

                /* base of 0x02040000 */
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02040000 ,0x02040010 )}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02040070 ,0x02040070 )}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x020400A8 ,0x020400A8 )}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x020400b0 ,0x02040108 )}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02040130 ,0x02040150 )}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x020401a0 ,0x020401a8 )}

                /* Ingress Log */
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02040700 ,0x02040728 )}
                /* ingrPortsRateLimitConfig */
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02040740 ,0x02040748 )}

            };
            GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

            smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
        }

        /* chunks with formulas */
        {
            SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
            {
                /* base of 0x02000000 */
                 {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02000000 ,0)}, FORMULA_SINGLE_PARAMETER(64 , 0x1000)}
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02000010 ,0)}, FORMULA_SINGLE_PARAMETER(64 , 0x1000)}
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02000400 ,0)}, FORMULA_SINGLE_PARAMETER(28 , 0x1000)}
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02000804 ,0),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 0x1000),SMEM_BIND_TABLE_MAC(ieeeRsrvMcCpuIndex)}, FORMULA_SINGLE_PARAMETER(16 , 0x1000)}

                /* IEEE Reserved Multicast Configuration register */ /* register and not table SMEM_BIND_TABLE_MAC(ieeeRsrvMcConfTable) */
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02000800 ,0),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 0x1000),SMEM_BIND_TABLE_MAC(ieeeRsrvMcConfTable)}, FORMULA_TWO_PARAMETERS(16 , 0x1000, 4, 0x8)}
                /* IEEE Reserved Multicast Configuration register reading - memory space for 'read' Errata  */
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02000820 ,0)}, FORMULA_TWO_PARAMETERS(16 , 0x1000 , 2 , 0x8)}

                /* base of 0x02010000 */
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02010800 ,0)}, FORMULA_SINGLE_PARAMETER(2 , 0x1000)}

                /* base of 0x02020000 */
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02020800 ,0)}, FORMULA_SINGLE_PARAMETER(16 , 0x1000)}
                /* base of 0x02030000 */
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02030800 ,0)}, FORMULA_SINGLE_PARAMETER(16 , 0x1000)}
                /* Set0Word<%n>Desc Bits */
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02040020 ,0)}, FORMULA_SINGLE_PARAMETER(20 , 0x4)}
                /* Set1Word<%n>Desc Bits */
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02040A00 ,0)}, FORMULA_SINGLE_PARAMETER(20 , 0x4)}
                /* base of 0x02040000 */
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02040800 ,0)}, FORMULA_SINGLE_PARAMETER(16 , 0x1000)}


            };
            GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
            SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

            smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

            /*add the tmp unit chunks to the main unit */
            smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);
        }
    }

/*******************************************************************************
*   smemLion2UnitIpvx
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Lion2  IPVX unit
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr     - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemLion2UnitIpvx
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    GT_U32  baseAddr = 0x02800000;
    GT_U32  ii;

    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            /* base of 0x02800000 */
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02800000 ,0x02800014 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02800100 ,0x0280010c )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02800200 ,0x0280020c )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02800250 ,0x0280026c )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02800278 ,0x02800294 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02800300 ,0x0280033c )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02800440 ,0x0280045c )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02800940 ,0x02800974 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02800d00 ,0x02800d70 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02800e3c ,0x02800e3c )}
            /*IPVX FCoE Global configuration*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028009a4 ,0x028009a4 )}

            /* base of 0x02801000 */
            /* Router Next Hop Table Age Bits */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02801000 , 1024),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4),SMEM_BIND_TABLE_MAC(routeNextHopAgeBits)}
            /* Router Vlan uRPF table */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02801400 , 1024)}

            /* Router Next Hop Table */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02b00000 , 131072)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        /* unify the base addresses */
        for(ii = 0 ; ii < numOfChunks ; ii++)
        {
            chunksMem[ii].memFirstAddr -= baseAddr;
        }

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }

    /* chunks with formulas */
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
            /* Router Desc Log %n */
             {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02800CE0 ,0)}, FORMULA_SINGLE_PARAMETER(8 , 0x4)}
            /* base of 0x02800000 */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02800900 ,0)}, FORMULA_TWO_PARAMETERS(10 , 0x4 , 4 , 0x100)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02800980 ,0)}, FORMULA_SINGLE_PARAMETER(4 , 0x100)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

        /* unify the base addresses */
        for(ii = 0 ; ii < numOfChunks ; ii++)
        {
            chunksMem[ii].memChunkBasic.memFirstAddr -= baseAddr;
        }

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitPtr, &tmpUnitChunk);
    }


    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            /*Global configurations*/
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02800990 ,0x028009A0 )}
                                                                          /*0x4000*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02803000 ,LION3_NUM_E_PORTS_CNS*4), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(5, 4),SMEM_BIND_TABLE_MAC(ipvxIngressEPort)}
                                                                          /*0x8000*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02807000 ,LION3_NUM_E_VLANS_CNS*4), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(2, 4),SMEM_BIND_TABLE_MAC(ipvxIngressEVlan)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

        /* unify the base addresses */
        for(ii = 0 ; ii < numOfChunks ; ii++)
        {
            chunksMem[ii].memFirstAddr -= baseAddr;
        }

        smemInitMemChunk(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);
    }


}


/*******************************************************************************
*   smemLion2UnitBm
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Lion2  BM unit
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr     - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemLion2UnitBm
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    GT_U32  baseAddr = 0x03000000;
    GT_U32  ii;

    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            /* base of 0x03000000 */
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x03000000 ,0x03000084 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x03000200 ,0x03000210 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x03000300 ,0x03000308 )}
            /* Pizza Arbiter */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x03000350 ,0x03000368)}
            /* Link List */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x03010000 , 8192)}
            /* Control */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x03020000 , 8192)}

        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        /* unify the base addresses */
        for(ii = 0 ; ii < numOfChunks ; ii++)
        {
            chunksMem[ii].memFirstAddr -= baseAddr;
        }

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }

    /* chunks with formulas */
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
            /* base of 0x03000000 */
            {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x03000100 ,0)}, FORMULA_SINGLE_PARAMETER(32 , 0x4)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

        /* unify the base addresses */
        for(ii = 0 ; ii < numOfChunks ; ii++)
        {
            chunksMem[ii].memChunkBasic.memFirstAddr -= baseAddr;
        }

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);
    }
}

/*******************************************************************************
*   smemLion2UnitBma
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  BMA unit
*
* INPUTS:
*       commonDevMemInfoPtr   - pointer to common device memory object.
*       unitPtr - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemLion2UnitBma
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    GT_U32  baseAddr = 0x03800000;
    GT_U32  ii;

    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            /* BMA Cluster - interrupt cause and mask */
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x03810000 ,0x0381000C )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x03830000 ,0x0383000C )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x03850000 ,0x0385000C )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x03870000 ,0x0387000C )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x03880000 ,0x0388000C )}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        /* unify the base addresses */
        for(ii = 0 ; ii < numOfChunks ; ii++)
        {
            chunksMem[ii].memFirstAddr -= baseAddr;
        }

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
    /* chunks with formulas */
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
            /* BMA unit 0/1/2/3 */
            {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x03800000 ,0)}, FORMULA_TWO_PARAMETERS((65792/4) , 0x4, 4, 0x20000)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

        /* unify the base addresses */
        for(ii = 0 ; ii < numOfChunks ; ii++)
        {
            chunksMem[ii].memChunkBasic.memFirstAddr -= baseAddr;
        }

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);
    }
}


/*******************************************************************************
*   smemLion2UnitLms
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  LMS unit
*
* INPUTS:
*       commonDevMemInfoPtr   - pointer to common device memory object.
*       unitPtr - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemLion2UnitLms
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    GT_U32  baseAddr = 0x04000000;
    GT_U32  ii;

    unitPtr->numOfUnits = 4;/*0x04000000,0x04800000,0x05000000,0x05800000*/

    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            /* Internal Registers */
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x04000000 ,0x04000008 )}
            /* base of 0x04004000 */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x04004024 ,0x04004034 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x04004054 ,0x04004054 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x04004100 ,0x0400410c )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x04004140 ,0x04004140 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x04004200 ,0x04004200 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x04005100 ,0x04005104 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x04005110 ,0x04005110 )}
            /* base of 0x04800000 */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x04800000 ,0x04800004 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x04804024 ,0x04804034 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x04804100 ,0x0480410c )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x04805100 ,0x04805104 )}

            /* base of 0x05004000 */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x05004140 ,0x05004144 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x05005100 ,0x05005100 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x05005110 ,0x05005110 )}

        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        /* unify the base addresses */
        for(ii = 0 ; ii < numOfChunks ; ii++)
        {
            chunksMem[ii].memFirstAddr -= baseAddr;
        }

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
}


/*******************************************************************************
*   smemLion2UnitFdb
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the LION  FDB unit
*
* INPUTS:
*       devObjPtr   - pointer to device memory.
*       unitPtr     - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemLion2UnitFdb
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
#define  a0_fdbEntryNumBits        114
#define  a0_fdbEntryNumBytes       ((a0_fdbEntryNumBits + 7)/8)
#define  a0_fdbEntryNumBytes_align (a0_fdbEntryNumBytes <= 16 ? 16 : 32)
#define  a0_fdbSizeNumBytes  ((SMEM_MAC_TABLE_SIZE_64KB) * a0_fdbEntryNumBytes_align)

        {
            SMEM_CHUNK_BASIC_STC  chunksMem[]=
            {
                /* base of 0x06000000 */
                 {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x06000000 ,0x0600007c )}
                /* Bad Parity Address */
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x06000088 ,0x06000088 )}
                /* Recognition Bad Parity Address */
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x060000F0 ,0x06000100 )}
            };
            GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

            smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
        }

        {
            SMEM_CHUNK_BASIC_STC  chunksMem[]=
            {
                /*FDB table*/
                 {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x06400000 ,a0_fdbSizeNumBytes), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(a0_fdbEntryNumBits, a0_fdbEntryNumBytes_align),SMEM_BIND_TABLE_MAC(fdb)}
            };
            GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);
            SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

            smemInitMemChunk(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

            /*add the tmp unit chunks to the main unit */
            smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);
        }

}


/*******************************************************************************
*   smemLion2UnitMppm
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Lion2  MPPM unit
*
* INPUTS:
*       devObjPtr   - pointer to common device memory object.
*       unitPtr     - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemLion2UnitMppm
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{

    GT_U32  baseAddr = 0x06800000;
    GT_U32  ii;

    if(0 == devObjPtr->portGroupSharedDevObjPtr)
    {   /*bobcat2*/
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000000, 0x00000004)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000050, 0x00000054)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000100, 0x00000100)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000120, 0x00000120)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000001F0, 0x000001F0)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000200, 0x00000208)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000328, 0x00000328)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000500, 0x00000500)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000510, 0x00000518)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000600, 0x00000600)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000610, 0x00000610)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000620, 0x00000620)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000710, 0x00000710)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000800, 0x00000800)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000810, 0x00000810)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000900, 0x00000900)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000910, 0x00000910)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000A00, 0x00000A00)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000B00, 0x00000B00)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000C00, 0x00000C00)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000D00, 0x00000D00)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000E00, 0x00000E04)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000E50, 0x00000E54)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000F00, 0x00000F04)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000F50, 0x00000F54)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001000, 0x00001004)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002000, 0x00002008)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000202C, 0x0000202C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002080, 0x00002080)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000020B0, 0x000020B0)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002120, 0x00002124)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002170, 0x00002174)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000021B0, 0x000021B4)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
    else
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            /* base of 0x06800000 */
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x06800000 ,0x06800018 )}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        /* unify the base addresses */
        for(ii = 0 ; ii < numOfChunks ; ii++)
        {
            chunksMem[ii].memFirstAddr -= baseAddr;
        }


        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
}

/*******************************************************************************
*   smemLion2UnitCtu
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Lion2  CTU0 unit
*
* INPUTS:
*       devObjPtr   - pointer to common device memory object.
*       unitPtr     - pointer to the unit chunk
** OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemLion2UnitCtu
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{


    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            /* CTU Configuration Registers */
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000004 ,0x00000028 )}
            /* BMA CT Parallel DRO Counters */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000020d0 ,0x000020d8 )}
            /* CTU Interrupt Cause/Mask */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003000 ,0x00003004 )}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
    /* chunks with formulas */
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
            /* CTU Status Registers */
             {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00001000 ,0)}, FORMULA_TWO_PARAMETERS(4, 0x4, 13, 0x10)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00001100 ,0)}, FORMULA_TWO_PARAMETERS(4, 0x10, 4, 0x4)}
            /* CTU Counters */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00002000 ,0)}, FORMULA_TWO_PARAMETERS(13,  0x10, 4, 0x4)}
            /* Pending Table Src_Pipe0 Trg_Pipe0/1/2/3 */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00010000 ,0)}, FORMULA_TWO_PARAMETERS(16,  0x4, 16, 0x40)}
            /* DBF Src Pipe 0/1/2/3 */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00011000 ,0)}, FORMULA_TWO_PARAMETERS(64/4,  0x4, 4, 0x40)}
            /* DBM Src Pipe 0/1/2/3 */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00020000 ,0)}, FORMULA_TWO_PARAMETERS(12288/4,  0x4, 4, 0x10000)}
        };

        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);
    }
}
/*******************************************************************************
*   smemLion2UnitHa
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Lion2  HA unit
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr     - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemLion2UnitHa
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            /* HA SIP Registers */
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0E800004 ,0x0E800008 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0E800020 ,0x0E800024 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0E800070 ,0x0E800070 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0E800080 ,0x0E800084 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0E800100 ,0x0E80011C )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0E800138 ,0x0E80013C )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0E800370 ,0x0E80037C )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0E8003A0 ,0x0E8003B0 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0E800400 ,0x0E800434 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0E800550 ,0x0E800558 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0E800710 ,0x0E800710 )}
            /* HA_GTS_SIP */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0E800800 ,0x0E80083C )}
            /* Port/VLAN MAC SA Table */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0E808000 , 16640) , SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(8, 4),SMEM_BIND_TABLE_MAC(vlanPortMacSa)}
            /* DFX Memory */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0E818000 , 2048)}

            /*Port<4n...4n+3> TrunkNum Configuration Register<%n>*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0E800160 ,0x0E8001DC )}
            /* Egress VLAN Translation Table */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0E810000 , 4*4096), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(12, 4) , SMEM_BIND_TABLE_MAC(egressVlanTranslation)}
            /* Router ARP DA and Tunnel Start Table */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0E840000 , 64*1024), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(192, 32) , SMEM_BIND_TABLE_MAC(arp)/*tunnelStart*/}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }

    /* chunks with formulas */
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
            /* Router Header Alteration MAC SA Modification Mode */
             {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0E800120 ,0)}, FORMULA_SINGLE_PARAMETER( 4 , 0x4)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0E800130 ,0)}, FORMULA_TWO_PARAMETERS(2 ,0x4, 3, 0x10)}
            /* QoSProfile to EXP Register */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0E800300 ,0)}, FORMULA_SINGLE_PARAMETER( 16, 0x4)}
            /* Incoming Descriptor Word<<%w>> */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0E8003E0 ,0)}, FORMULA_SINGLE_PARAMETER( 5 , 0x4)}
            /* QoSProfile to DP Register */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0E800340 ,0)}, FORMULA_SINGLE_PARAMETER( 8 , 0x4)}
            /* Disable CRC Adding Configuration */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0E800380 ,0)}, FORMULA_SINGLE_PARAMETER( 2 , 0x4)}
            /* Keep VLAN1  */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0E800440 ,0),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4),SMEM_BIND_TABLE_MAC(haUp0PortKeepVlan1)}, FORMULA_SINGLE_PARAMETER( 16 , 0x4)}
            /* TP-ID Configuration Register  */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0E800500 ,0)}, FORMULA_SINGLE_PARAMETER( 4 , 0x4)}
            /* Port Tag0 TP-ID Select */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0E800510 ,0)}, FORMULA_TWO_PARAMETERS( 8 , 0x4, 2, 0x20)}
            /* lion 2 b0 dsa erspan */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0E8005D4, 0)}, FORMULA_SINGLE_PARAMETER(8, 0x18)}
        };

        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);
    }
}

/*******************************************************************************
*   smemLion2UnitRxDma
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Lion2  RX DMA unit
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr     - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemLion2UnitRxDma
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
        {
            SMEM_CHUNK_BASIC_STC  chunksMem[]=
            {
                 {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0F000000 ,0x0F00007C )}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0F000084 ,0x0F000084 )}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0F000090 ,0x0F0000B4 )}
            };
            GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

            smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
        }
    }

/*******************************************************************************
*   smemLion2UnitTxDma
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Lion2  TX DMA unit
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr     - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemLion2UnitTxDma
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    GT_U32  baseAddr = 0x0F800000;
    GT_U32  ii;

    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            /* TXDMA Engine Configuration */
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0F800000 ,0x0F800048 )}
             /* DFT */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0F800050 ,0x0F800070 )}
            /* Cut Through including pizza arbiter */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0F800080 ,0x0F8000C0 )}
            /* Burst Size Limit */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0F8000C4 ,0x0F8000C4)}
            /* Interrupts */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0F800130 ,0x0F800140)}
            /* Descriptor prefetch */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0F800200 , 512)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        /* unify the base addresses */
        for(ii = 0 ; ii < numOfChunks ; ii++)
        {
            chunksMem[ii].memFirstAddr -= baseAddr;
        }

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
    /* chunks with formulas */
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
             {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0F800164 ,0)}, FORMULA_SINGLE_PARAMETER(7 , 0x4)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0F800180 ,0)}, FORMULA_SINGLE_PARAMETER(6 , 0x4)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0F802000 ,0)}, FORMULA_TWO_PARAMETERS(2048 , 0x4, 8, 0x2000)}
        };

        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

        /* unify the base addresses */
        for(ii = 0 ; ii < numOfChunks ; ii++)
        {
            chunksMem[ii].memChunkBasic.memFirstAddr -= baseAddr;
    }

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitPtr, &tmpUnitChunk);
    }

    if(0 == devObjPtr->portGroupSharedDevObjPtr)
    {
        /* patch for bobcat2 many ports */
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
             {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0F8000C8 ,0)}, FORMULA_SINGLE_PARAMETER(64 , 0x4)}
        };

        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

        /* unify the base addresses */
        for(ii = 0 ; ii < numOfChunks ; ii++)
        {
            chunksMem[ii].memChunkBasic.memFirstAddr -= baseAddr;
        }

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitPtr, &tmpUnitChunk);
    }
    else
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
             {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0F8000C8 ,0)}, FORMULA_SINGLE_PARAMETER(13 , 0x4)}
        };

        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

        /* unify the base addresses */
        for(ii = 0 ; ii < numOfChunks ; ii++)
        {
            chunksMem[ii].memChunkBasic.memFirstAddr -= baseAddr;
        }

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitPtr, &tmpUnitChunk);
    }



}

/*******************************************************************************
*   smemLion2UnitEplr
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Lion2  EPLR unit
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr     - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemLion2UnitEplr
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{


    smemLion2UnitPolicerUnify(devObjPtr, unitPtr, SMEM_SIP5_PP_PLR_UNIT_EPLR_E);
#if 0
    {
        /* Update DB for EPLR unit */
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00080000 , 2560), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(51, 8)}

        };

        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

        smemInitMemChunk(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /* Update the main unit chunks */
        smemInitMemUpdateUnitChunks(devObjPtr,(unitPtr, &tmpUnitChunk);
    }
#endif
}

/*******************************************************************************
*   smemLion2UnitCentralizedCounters
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  Lion2 Centralized_Counters unit
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr     - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemLion2UnitCentralizedCounters
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    GT_U32  baseAddr = 0x08000000;
    GT_U32  ii;


    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            /* base of 0x08000000 */
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x08000000 ,0x08000000 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x08000030 ,0x08000030 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x08000040 ,0x08000044 )}
            /* base of 0x08000100 */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x08000100 ,0x0800010C )}
            /* Data Error */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC(0x08000110 , 0x08000114)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        /* unify the base addresses */
        for(ii = 0 ; ii < numOfChunks ; ii++)
        {
            chunksMem[ii].memFirstAddr -= baseAddr;
        }

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }

    /* chunks with formulas */
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
            /* CNC Block %n Configuration Register%m  */
             {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x08001180 ,0)}, FORMULA_TWO_PARAMETERS(8, 0x100, 3, 0x4)}
            /* CNC Block %n Wraparound Status Register */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x080010A0 ,0)}, FORMULA_TWO_PARAMETERS(8, 0x100, 4, 0x4)}
            /* Data Error Status Register */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x08000118 ,0)}, FORMULA_SINGLE_PARAMETER(8, 0x4)}
            /* CNC Block 0/1/2/3/4/5/6/7 Counters */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x08080000 ,0)}, FORMULA_TWO_PARAMETERS(8, 0x4000, (512*2), 0x4)}  /* manually fixed from 256 counters to 512 counters (each counter 2 words)*/
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

        /* unify the base addresses */
        for(ii = 0 ; ii < numOfChunks ; ii++)
        {
            chunksMem[ii].memChunkBasic.memFirstAddr -= baseAddr;
        }

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /* Update the main unit chunks */
        smemInitMemUpdateUnitChunks(devObjPtr,unitPtr, &tmpUnitChunk);
    }
}

/*******************************************************************************
*   smemLion2UnitXGPortMib
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  XGPortMib unit
*
* INPUTS:
*       commonDevMemInfoPtr   - pointer to common device memory object.
*       unitPtr - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemLion2UnitXGPortMib
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    GT_U32  baseAddr = 0x09000000;
    GT_U32  ii;


    /* chunks with formulas */
    if(devObjPtr->portGroupSharedDevObjPtr)
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
            /* base of 0x09000000 */
             {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x09000000 ,0)}, FORMULA_TWO_PARAMETERS(128/4 , 0x4 , 13 , XG_COUNTER_STEP_PER_PORT_CNS)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x09000000+14*XG_COUNTER_STEP_PER_PORT_CNS ,0)}, FORMULA_SINGLE_PARAMETER(128/4 , 0x4)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);

        /* unify the base addresses */
        for(ii = 0 ; ii < numOfChunks ; ii++)
        {
            chunksMem[ii].memChunkBasic.memFirstAddr -= baseAddr;
        }

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, unitPtr);
    }
    else
    {
#define     BOBCAT2_MIB_PORT_56_START_OFFSET_CNS     0x00800000
#define     BOBCAT2_MIB_OFFSET_CNS     (0x400)

        /* steps between each port */
        devObjPtr->xgCountersStepPerPort   = BOBCAT2_MIB_OFFSET_CNS;
        devObjPtr->xgCountersStepPerPort_1 = BOBCAT2_MIB_OFFSET_CNS;
        /* offset ot table xgPortMibCounters_1 */
        devObjPtr->offsetToXgCounters_1 = BOBCAT2_MIB_PORT_56_START_OFFSET_CNS;
        devObjPtr->startPortNumInXgCounters_1 = 56;

        unitPtr->chunkType = SMEM_UNIT_CHUNK_TYPE_8_MSB_E;

        /* chunks with formulas */
        {
            SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
            {
                /* base of 0x00000000 */
                /* ports 0..55 */
                  {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00000000 ,0)}, FORMULA_TWO_PARAMETERS(128/4 , 0x4 , 56 , BOBCAT2_MIB_OFFSET_CNS)}
                /* ports 56..71 */
                 ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00000000 + BOBCAT2_MIB_PORT_56_START_OFFSET_CNS ,0)}, FORMULA_TWO_PARAMETERS(128/4 , 0x4 , 16 , BOBCAT2_MIB_OFFSET_CNS)}
            };
            GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);

            smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, unitPtr);
        }
    }

}

/*******************************************************************************
*   smemLion2UnitLpSerdes
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Lion2  LpSerdes unit
*
* INPUTS:
*       devObjPtr   - pointer to common device memory object.
*       unitPtr     - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemLion2UnitLpSerdes
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    GT_U32  baseAddr = 0x09800000;
    GT_U32  ii;


    /* chunks with formulas */
    if(devObjPtr->portGroupSharedDevObjPtr)
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
            /* base of 0x09800000 SERDES SERDES SD_%s_IP Units - serdes external registers */
             {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x09800000 ,0)}, FORMULA_TWO_PARAMETERS(12, 0x4, 24, 0x400)}
            /* base of 0x09800200 - COMPHY_H %t Registers - Serdes Internal Register File */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x09800200 ,0)}, FORMULA_TWO_PARAMETERS(512/4, 0x4, 24, 0x400)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);

        /* unify the base addresses */
        for(ii = 0 ; ii < numOfChunks ; ii++)
        {
            chunksMem[ii].memChunkBasic.memFirstAddr -= baseAddr;
        }

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, unitPtr);
    }
    else
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
            /* bobcat 2  */
            /* serdes external registers */
             {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000000, 0x0000000C)} , FORMULA_SINGLE_PARAMETER(36 , 0x1000)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000014, 0x00000038)} , FORMULA_SINGLE_PARAMETER(36 , 0x1000)}
            /* serdes Internal registers File*/
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000800, 0x00000FFC)} , FORMULA_SINGLE_PARAMETER(36 , 0x1000)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, unitPtr);
    }


}

/*******************************************************************************
*   smemLion2UnitTxqQueue
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Lion2  Txq unit
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr     - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemLion2UnitTxqQueue
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    GT_U32  baseAddr = 0x10000000;
    GT_U32  ii;


    /* chunks with flat memory (no formulas) */
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            /* Unicast Multicast Arb */
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x10009800, 0x1000980C )}
            /* base of 0A090000 */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x10090000, 0x1009000C )}
            /* Port Descriptor Full Interrupt */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x10090040 ,0x10090054 )}
            /* Port Buffer Full Interrupt  */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x10090060 ,0x10090074 )}
            /* PFC Interrupts */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x10090080 ,0x10090144 )}
            /* PFC Interrupts */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x10090150 ,0x1009015C )}
            /* Interrupts */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x10090180 ,0x10090184 )}
            /* Distributor General Configurations */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x10091000 ,0x10091000 )}
            /* CNC Modes Register */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x100936A0 ,0x100936A0 )}
            /* Tail Drop Configuration */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x100A0000 ,0x100A0000 )}
            /* Byte Count for Tail Drop */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x100A0008 ,0x100A0008 )}
            /* Global Descriptors Limit */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x100A0300 ,0x100A0300 )}
            /* Global Buffer Limit */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x100A0310 ,0x100A0310 )}
            /* Shared Queue Limits */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x100A0500 , 256)}
            /* Eq Queue Limits DP0 */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x100A0800 , 256)}
            /* Dq Queue Buf Limits */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x100A0900 , 256)}
            /* Dq Queue Desc Limits */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x100A0A00 , 256)}
            /* Eq Queue Limits DP12 */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x100A0C00 , 512)}
            /* Total Desc Counter */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x100A1000 ,0x100A1004 )}
            /*Port<%n> Desc Counter  Port<%n> Desc Counter*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x100A1010 , 64*4)}
            /*Port<%n> Buffers Counter  Port<%n> Buffers Counter*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x100A1110 , 64*4)}
            /*TC<%t> Shared Desc Counter  TC<%t> Shared Desc Counter*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x100A1210 , 8*4)}
            /*TC<%t> Shared Buffers Counter  TC<%t> Shared Buffers Counter*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x100A1250 , 8*4)}
            /* Q Main Buff */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x100A4000 , 2048)}
            /* CN Global Configurations */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x100B0000 ,0x100B0000 )}
            /* CN Sample Intervals Table */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x100B1000 , 4096)}
            /*PFC Global Configurations*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x100C0000 ,0x100C0000 )}
            /* PFC Internal */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x100C1110 ,0x100C1110 )}
            /* FC Mode Profile TC XOff Thresholds */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x100C1800 , 256)}
            /* FC Mode Profile TC XOn Thresholds */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x100C1A00 , 256)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        /* unify the base addresses */
        for(ii = 0 ; ii < numOfChunks ; ii++)
        {
            chunksMem[ii].memFirstAddr -= baseAddr;
        }

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
    /* chunks with formulas */
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
            /* TC DP Remapping */
             {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x10091010, 0)}, FORMULA_SINGLE_PARAMETER(2, 0x4)}
            /* Stack TC <%tc> Remapping */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x10091020, 0)}, FORMULA_SINGLE_PARAMETER(8, 0x4)}
            /* Stack Remap En */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x10091050, 0)}, FORMULA_TWO_PARAMETERS(4, 0x4, 7, 0x10)}
            /* Peripheral Access Misc */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x10093000, 0)}, FORMULA_SINGLE_PARAMETER(5, 0x4)}
            /* Egress MIB Counters */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x10093200, 0)}, FORMULA_TWO_PARAMETERS(2, 0x4, 10, 0x10)}
            /* Port Enqueue Enable */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x100A0010, 0)}, FORMULA_SINGLE_PARAMETER(64, 0x4)}
            /* Port Tail Drop/CN Profile, TC Profile Enable Shared Pool Usage, Profile Enable WRTD DP,
               Profile Priority Queue to Shared Pool Association, WRTD Mask 0/1 */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x100A0120, 0)}, FORMULA_SINGLE_PARAMETER(86, 0x4)}
            /* Profile Port Limits */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x100A0320, 0)}, FORMULA_SINGLE_PARAMETER(8, 0x4)}
            /* Shared Pool Limits */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x100A0400, 0)}, FORMULA_SINGLE_PARAMETER(8, 0x4)}
            /* Multicast Filter Limits */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x100A3300, 0)}, FORMULA_SINGLE_PARAMETER(4, 0x10)}
            /* Muliticast Filter Counters */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x100A6000, 0)}, FORMULA_SINGLE_PARAMETER(4, 0x4)}
            /* Resource Histogram Limits */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x100A6300, 0)}, FORMULA_SINGLE_PARAMETER(2, 0x4)}
            /* Resource_Histogram_Counters */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x100A6310, 0)}, FORMULA_SINGLE_PARAMETER(4, 0x4)}
            /* CN Priority Queue En Profile and En CN Frame Tx on Port */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x100B0010, 0)}, FORMULA_TWO_PARAMETERS(2, 0x4, 3, 0x10)}
            /* Feedback Calc Configurations Feedback MIN/Max */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x100B0040, 0)}, FORMULA_SINGLE_PARAMETER(3, 0x10)}
            /* CN Sample Tbl  */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x100B0090, 0)}, FORMULA_SINGLE_PARAMETER(8, 0x4)}
            /* CN Buffer FIFO Overruns Counter, CN Buffer FIFO Parity Errors Counter and CN Drop Counter  */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x100B00D0, 0)}, FORMULA_SINGLE_PARAMETER(3, 0x10)}
            /* Profile Tc CN Threshold */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x100B0300, 0)}, FORMULA_THREE_PARAMETERS(8, 0x20, 8, 0x4, 4, 0x200)}
            /* Src Port FC Mode */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x100C0010, 0)}, FORMULA_SINGLE_PARAMETER(4, 0x4)}
            /* PFC Pipe Counters Status */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x100C0030, 0)}, FORMULA_SINGLE_PARAMETER(4, 0x4)}
            /* PFC Port Group Counters Status */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x100C0040, 0)}, FORMULA_SINGLE_PARAMETER(4, 0x4)}
            /* PFC Pipe */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x100C0050, 0)}, FORMULA_SINGLE_PARAMETER(24, 0x4)}
            /* PFC Source Port Profile */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x100C00B0, 0)}, FORMULA_SINGLE_PARAMETER(20, 0x4)}
            /* Link Level Flow Control Target Port Remap and PFC LLFC Source Port Remap */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x100C0100, 0)}, FORMULA_SINGLE_PARAMETER(12, 0x4)}
            /* LLFC_Target_Port_Remap */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x100C0140, 0)}, FORMULA_SINGLE_PARAMETER(16, 0x4)}
            /* PFC LLFC Source Port Remap For LLFC Counters */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x100C0200, 0)}, FORMULA_SINGLE_PARAMETER(32, 0x4)}
            /* Global PFC Group of Ports TC Counter and Global PFC Group of Ports TC Thresholds */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x100C0500, 0)}, FORMULA_SINGLE_PARAMETER(32, 0x4)}
            /* PFC Group of Ports TC Counter */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x100C0900, 0)}, FORMULA_SINGLE_PARAMETER(64, 0x4)}
            /* PFC Group of Ports TC Thresholds */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x100C1000, 0)}, FORMULA_TWO_PARAMETERS(8, 0x20, 8, 0x4)}
            /* PFC Port Group <%p> Hook Counters Ctrl, PFC Port Group CN Trig Hook Counter */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x100C1120, 0)}, FORMULA_TWO_PARAMETERS(8, 0x4, 2, 0x20)}
            /* Pipe 0/1/2/3/4/5/6/7 PFC LLFC Counters */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x100C2000, 0)}, FORMULA_TWO_PARAMETERS((4096/4), 0x4, 8, 0x1000)}
        };

        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

        /* unify the base addresses */
        for(ii = 0 ; ii < numOfChunks ; ii++)
        {
            chunksMem[ii].memChunkBasic.memFirstAddr -= baseAddr;
        }

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);
        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);
    }
}

/*******************************************************************************
*   smemLion2UnitTxqLl
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Lion2  TxqLl unit
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr     - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemLion2UnitTxqLl
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{

    GT_U32  baseAddr = 0x10800000;
    GT_U32  ii;


    /* chunks with flat memory (no formulas) */
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            /* Desc Link List */
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x10800000 , 262144)}
            /* FBUF */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x10880000 , 16384)}
            /* QTAIL */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x10888000 , 2048)}
            /* QHEAD */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x10889000 , 2048)}
            /* Link List ECC Control Config */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1088A000 ,0x1088A000 )}
            /* Q Main Desc */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x1088C000 , 2048)}
            /* Interrupts */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x108A0000 ,0x108A000C )}
        };

        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        /* unify the base addresses */
        for(ii = 0 ; ii < numOfChunks ; ii++)
        {
            chunksMem[ii].memFirstAddr -= baseAddr;
        }

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
    /* chunks with formulas */
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
            /* Link List ECC Control */
             {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x1088A004 ,0)}, FORMULA_SINGLE_PARAMETER(4, 0x10)}
            /* Ports Link Enable Status */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x108A0100 ,0)}, FORMULA_SINGLE_PARAMETER(2, 0x4)}
        };

        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

        /* unify the base addresses */
        for(ii = 0 ; ii < numOfChunks ; ii++)
        {
            chunksMem[ii].memChunkBasic.memFirstAddr -= baseAddr;
        }

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);
        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);
    }
}

/*******************************************************************************
*   smemLion2UnitTxqDq
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Lion2  TxqDq unit
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr     - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemLion2UnitTxqDq
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    GT_U32  baseAddr = 0x11000000;
    GT_U32  ii;


    /* chunks with flat memory (no formulas) */
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            /* Global Dequeue Configuration */
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x11000000, 0x11000000)}
             /* Interrupts */
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x11000100, 0x11000124)}
             /* Memory Parity Error */
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x11000150, 0x11000158)}
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x11000180, 0x11000180)}
             /* Pizza Arb Debug */
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x11001500, 0x11001504)}
             /* Scheduler Configuration */
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x11001000, 0x11001008)}
             /* Port Scheduler Profile */
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x11001240, 0x11001240)}
             /* Port Arbiter stage 2 Weight Register */
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x11001410, 0x11001410)}
             /* State Variable RAM */
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x11001800 , 512), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(168, 32)}
             /* Shaper Registers */
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x11002000, 0x11002010)}
             /* Prio Token Bucket Configuration */
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x11002400, 1024), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(432, 64)}
             /* Port Token Bucket Configuration */
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x11002800, 128), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(54, 8)}
             /* Flow Control Configuration */
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x11003000, 0x11003000)}
             /* Statistical and CPU-Triggered Egress Mirroring to Analyzer Port */
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x11004000, 0x11004004)}
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x11004010, 0x11004010)}
        };

        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        /* unify the base addresses */
        for(ii = 0 ; ii < numOfChunks ; ii++)
        {
            chunksMem[ii].memFirstAddr -= baseAddr;
        }

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
    /* chunks with formulas */
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
            /* Port TxQ Flush Trigger */
             {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x11000090, 0)}, FORMULA_SINGLE_PARAMETER(16, 0x4)}
            /* Profile %p SDWRR Weights Configuration Register 0/1 */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x11001140, 0)}, FORMULA_TWO_PARAMETERS(8, 0x4, 2, 0x40)}
            /* Profile %p SDWRR Group */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x110011C0 , 0)}, FORMULA_TWO_PARAMETERS(4, 0x10, 2, 0x40)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC((0x110011C0+0xEC0) , 0)}, FORMULA_TWO_PARAMETERS(4, 0x10, 2, 0x40)}
            /* Port<%n> Dequeue Enable */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x11002014, 0)}, FORMULA_SINGLE_PARAMETER(16, 0x4)}
            /* Port TxQ Flush Trigger */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x11001280, 0)}, FORMULA_SINGLE_PARAMETER(16, 0x4)}
            /* Port Arbiter stage 1 Weight Register */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x11001400, 0)}, FORMULA_SINGLE_PARAMETER((0xAC/4), 0x4)}
            /* Scheduler Profile LLFC MTU */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x11003008, 0)}, FORMULA_SINGLE_PARAMETER(8, 0x4)}
            /* TC<%n> To Priority Mapping */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x11003040, 0)}, FORMULA_SINGLE_PARAMETER(8, 0x4)}
            /* Scheduler Profile<%n>TC to Priority Map Enable */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x11003060, 0)}, FORMULA_SINGLE_PARAMETER(8, 0x4)}
            /* TC<%t>Profile<%p>PFCFactor */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x11003080, 0)}, FORMULA_TWO_PARAMETERS(8, 0x20, 8, 0x4)}
        };

        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

        /* unify the base addresses */
        for(ii = 0 ; ii < numOfChunks ; ii++)
        {
            chunksMem[ii].memChunkBasic.memFirstAddr -= baseAddr;
        }

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);
        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);
    }


    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {

        if(devObjPtr->portGroupSharedDevObjPtr)
        {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            /* Egress Port<n> Analyzer Index bit 2 */
              {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x11004014, 4)}
                 /* Egress STC Table*/
                 ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x11004400, 256), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(80, 16) , SMEM_BIND_TABLE_MAC(egressStc)}
            };
            GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);
            SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

            /* unify the base addresses */
            for(ii = 0 ; ii < numOfChunks ; ii++)
            {
                chunksMem[ii].memFirstAddr -= baseAddr;
            }

            smemInitMemChunk(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

            /*add the tmp unit chunks to the main unit */
            smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);

        }
        else
        {
            SMEM_CHUNK_BASIC_STC  chunksMem[]=
            {
                /* Egress Port<n> Analyzer Index bit 2 */
                /* support 3 registers per 16 ports , but needed for 64 ports in bobcat2
                   so needed total of 8 registers , but 1 already exists in 0x11004010 :

                   0x11004010 - bits 0,1 for ports 0..15
                   0x11004014 - bit 2 for ports 0..15

                   0x11004018 - bits 0,1 for ports 16..31
                   0x1100401c - bit 2 for ports 16..31

                   0x11004020 - bits 0,1 for ports 32..47
                   0x11004024 - bit 2 for ports 32..47

                   0x11004028 - bits 0,1 for ports 16..63
                   0x1100402c - bit 2 for ports 16..63

                   */
                  {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x11004014, (8 - 1)*4)}

                 /* Egress STC Table*/                                         /* time 4 to support 64 ports(not 16)*/
                 ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x11004400, 256*4), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(80, 16) , SMEM_BIND_TABLE_MAC(egressStc)}

            };
            GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);
            SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

            /* unify the base addresses */
            for(ii = 0 ; ii < numOfChunks ; ii++)
            {
                chunksMem[ii].memFirstAddr -= baseAddr;
            }

            smemInitMemChunk(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

            /*add the tmp unit chunks to the main unit */
            smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);
        }

        {
            SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
            {
                /* Port<%n> Dequeue Enable */             /*allocate those that not allocated by the first 16 ports*/
                 {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x11002014 + 4*16, 0)}, FORMULA_SINGLE_PARAMETER(BOBCAT2_NUM_OF_PORTS-16, 0x4)}
                /* Port TxQ Flush Trigger */              /*allocate those that not allocated by the first 16 ports*/   /*NOTE: those currently 'overlap' with line of(0x110011C0+0xEC0)*/
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x11001280 + 4*16, 0)}, FORMULA_SINGLE_PARAMETER(BOBCAT2_NUM_OF_PORTS-16, 0x4)}
            };

            GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
            SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

            /* unify the base addresses */
            for(ii = 0 ; ii < numOfChunks ; ii++)
            {
                chunksMem[ii].memChunkBasic.memFirstAddr -= baseAddr;
            }

            smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);
            /*add the tmp unit chunks to the main unit */
            smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);
        }
    }
    else
    {
        {
            SMEM_CHUNK_BASIC_STC  chunksMem[]=
            {
                /* Egress STC Table*/
                {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x11004400, 256), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(80, 16) , SMEM_BIND_TABLE_MAC(egressStc)}
            };

            GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);
            SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

            /* unify the base addresses */
            for(ii = 0 ; ii < numOfChunks ; ii++)
            {
                chunksMem[ii].memFirstAddr -= baseAddr;
            }

            smemInitMemChunk(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

            /*add the tmp unit chunks to the main unit */
            smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);
        }
    }

}

/*******************************************************************************
*   smemLion2UnitTxqDist
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Lion2  TxqDist unit
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr     - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemLion2UnitTxqDist
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    GT_U32  baseAddr = 0x13000000;
    GT_U32  ii;


    /* chunks with flat memory (no formulas) */
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             /* Dist Global Control */
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x13000000, 0x13000018)}
        };

        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        /* unify the base addresses */
        for(ii = 0 ; ii < numOfChunks ; ii++)
        {
            chunksMem[ii].memFirstAddr -= baseAddr;
        }

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
    /* chunks with formulas */
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
            /* Dist Burst Fifo Fill Level Status <<%n>> */
            {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x13000020, 0)}, FORMULA_SINGLE_PARAMETER(16, 0x4)}
            /* Dist Metal Fix Register */
           ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x13000100, 0)}, FORMULA_SINGLE_PARAMETER(2, 0x100)}
           /* Dist Local Src Port Map Own Dev En <%n> */
           ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x13000210, 0)}, FORMULA_TWO_PARAMETERS(4, 0x4, 2, 0x10)}
           /* Dist Entry <%n> Device Map Table */
           ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x13000230, 0), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4) , SMEM_BIND_TABLE_MAC(txqDistributorDeviceMapTable)}, FORMULA_SINGLE_PARAMETER(64, 0x4)}
        };

        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

        /* unify the base addresses */
        for(ii = 0 ; ii < numOfChunks ; ii++)
        {
            chunksMem[ii].memChunkBasic.memFirstAddr -= baseAddr;
        }

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);
        /* Add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);
    }
}




/*******************************************************************************
*   smemLion2UnitTxqSht
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Lion2  TxqSht unit
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr     - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*       NOT for Lion3
*
*******************************************************************************/
static void smemLion2UnitTxqSht
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{


    /* chunks with flat memory (no formulas) */
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            /* SHT Global Configurations */
              {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x118F0000, 0x118F0000)}
            /* Global registers */
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x118F0044, 0x118F0044)}
            /* DFX */
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x118F0D80, 0x118F0DA4)}
            /* DFX */
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x118F0D00, 0x118F0D04)}
            /* Redundancy repair done register */
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x118F0E00, 0x118F0E00)}
        };

        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
    /* chunks with formulas */
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
            /* Global registers */
             {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x118F0010, 0)}, FORMULA_SINGLE_PARAMETER(7, 0x10)}
            /* SHT Interrupts */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x118F0120, 0)}, FORMULA_SINGLE_PARAMETER(2, 0x10)}
        };

        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);
        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);
    }

        {

            SMEM_CHUNK_BASIC_STC  chunksMem[]=
            {
                 /* Egress vlan table */
                  {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x11800000 , 262144), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(274, 64),SMEM_BIND_TABLE_MAC(egressVlan)}
                 /* Egress Spanning Tree State Table */
                 ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x11840000 , 4096), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(126, 16),SMEM_BIND_TABLE_MAC(egressStp)}
                 /* Multicast Groups Table */
                 ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x11860000 , 65536), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(65, 16),SMEM_BIND_TABLE_MAC(mcast)}
                 /* Device Map Table */
                 ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x11870000 , 8192), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(16, 4),SMEM_BIND_TABLE_MAC(deviceMapTable)}
                 /* L2 Port Isolation Table */
                 ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x11880000 , 34816), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(65, 16),SMEM_BIND_TABLE_MAC(l2PortIsolation)}
                 /* L3 Port Isolation Table */
                 ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x11890000 , 34816), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(65, 16),SMEM_BIND_TABLE_MAC(l3PortIsolation)}
                 /* Ingress Vlan Table */
                 ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x118A0000 , 131072), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(140, 32),SMEM_BIND_TABLE_MAC(vlan)}
                 /* Ingress Spanning Tree State Table */
                 ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x118D0000 , 4096), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(126, 16),SMEM_BIND_TABLE_MAC(stp)}
            };

            GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);
            SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

            smemInitMemChunk(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

            /*add the tmp unit chunks to the main unit */
            smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);

        }
    }

/*******************************************************************************
*   smemLion2UnitGroupOfPorts
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Lion2  GOP unit
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr     - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemLion2UnitGroupOfPorts
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    GT_U32  baseAddr = 0x08800000;
    GT_U32  ii;

    if(devObjPtr->portGroupSharedDevObjPtr == 0)
    {
        return;
    }

    /*need to support 16 ports per core */

    /* chunks with formulas */
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
            /* Gig Ports*/
             {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x08800000 ,0)}, FORMULA_TWO_PARAMETERS(16, 0x4 , 15, 0x1000)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x08800040 ,0)}, FORMULA_TWO_PARAMETERS( 2, 0x8 , 15, 0x1000)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x08800058 ,0)}, FORMULA_TWO_PARAMETERS(16, 0x4 , 15, 0x1000)}
            /* CPU Port */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0883f000 ,0)}, FORMULA_SINGLE_PARAMETER(16, 0x4)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0883f040 ,0)}, FORMULA_SINGLE_PARAMETER( 2, 0x8)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0883f058 ,0)}, FORMULA_SINGLE_PARAMETER(15, 0x4)}
            /* HGL IP */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x089c0000 ,0)}, FORMULA_TWO_PARAMETERS(24, 0x4 ,  15, 0x1000)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x089c0100 ,0)}, FORMULA_TWO_PARAMETERS(23, 0x4 ,  15, 0x1000)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x089c0200 ,0)}, FORMULA_TWO_PARAMETERS(512/4, 0x4 ,  15, 0x1000)}
            /* XLG IP */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x088c0000 ,0)}, FORMULA_TWO_PARAMETERS(10, 0x4 ,  15, 0x1000)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x088c002c ,0)}, FORMULA_TWO_PARAMETERS( 2, 0x4 ,  15, 0x1000)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x088c0038 ,0)}, FORMULA_TWO_PARAMETERS(11, 0x4 ,  15, 0x1000)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x088c0068 ,0)}, FORMULA_TWO_PARAMETERS( 6, 0x4 ,  15, 0x1000)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x088c0084 ,0)}, FORMULA_TWO_PARAMETERS( 1, 0x4 ,  15, 0x1000)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x088c0090 ,0)}, FORMULA_TWO_PARAMETERS( 2, 0x4 ,  15, 0x1000)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x088c0400 ,0)}, FORMULA_TWO_PARAMETERS( 1024/4, 0x4 ,  16, 0x1000)}
            /* Packet Gen IP */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC( 0x08980000 ,0)}, FORMULA_TWO_PARAMETERS(12, 0x4 ,  15, 0x1000)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC( 0x08980040 ,0)}, FORMULA_TWO_PARAMETERS(3, 0x4 ,  15, 0x1000)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC( 0x08980050 ,0)}, FORMULA_TWO_PARAMETERS(1, 0x4 ,  15, 0x1000)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);

        /* unify the base addresses */
        for(ii = 0 ; ii < numOfChunks ; ii++)
        {
            chunksMem[ii].memChunkBasic.memFirstAddr -= baseAddr;
        }

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, unitPtr);
    }
}

/*******************************************************************************
*   smemLion2UnitEq
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Lion2  Eq unit
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr     - pointer to the unit chunk
*       eqPart      - part of the EQ unit (on Lion3 the EQ hold parts 0,1)
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemLion2UnitEq
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{

        {
            SMEM_CHUNK_BASIC_STC  chunksMem[]=
            {
                /* base of 0x0b000000 */
                 {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0b000000 ,0x0b0000ac )}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0b0000d4 ,0x0b0000d4 )}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0b000138 ,0x0b000138 )}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0b005000 ,0x0b005008 )}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0b007000 ,0x0b007080 - 4 ),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4),SMEM_BIND_TABLE_MAC(tcpUdpDstPortRangeCpuCode)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0b008000 ,0x0b008010),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4),SMEM_BIND_TABLE_MAC(ipProtCpuCode)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0b00a000 ,0x0b00a000)}

                /* Mirroring to Analyzer Port Configurations */
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0B00B000 ,0x0B00B008 )}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0B00B010 ,0x0B00B018 )}
                /* Mirror Interface Parameter Register */
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0B00B020 ,0x0B00B054 )}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0B00B070 ,0x0B00B08c )}

                /* base of 0x0b020000 */
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0b020000 ,0x0b02000c )}

                /* base of 0x0b040000 */
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0b040000 ,448 ) , SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(80, 0x10),SMEM_BIND_TABLE_MAC(ingrStc) }

                /* base of 0x0b050000 */
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0b050000 ,0x0b050004 )}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0b05000C ,0x0b05000C )}

                /* base of 0x0b100000 */
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0b100000 ,128 ) , SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4),SMEM_BIND_TABLE_MAC(statisticalRateLimit)}

                /* base of 0x0b200000 */
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0b200000 ,1024 ) , SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4),SMEM_BIND_TABLE_MAC(cpuCode)}

                /* base of 0x0b300000 */
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0b300000 ,512 )  , SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(14, 4),SMEM_BIND_TABLE_MAC(qosProfile)}

                /* base of 0x0b400000 */
    /*            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0b400000 ,4094 )}*/

                /* base of 0x0b500000 */
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0b500000 ,124 )}

                /* base of 0x0b600000 */
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0b600000 ,2048 ),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(27, 4),SMEM_BIND_TABLE_MAC(logicalTrgMappingTable)}

            };
            GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

            smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
        }

        /* chunks with formulas */
        {
            SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
            {
                /* base of 0x0b000000 */
                 {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0b001000 ,0)}, FORMULA_SINGLE_PARAMETER(4 , 0x1000)}
                /* base of 0x0b080000 -- support 32 registers to support address 0x0b080000 for diag on cpss - TBD */
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0b080000  ,0)}, FORMULA_SINGLE_PARAMETER(32 , 0x4)}
            };
            GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
            SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

            smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

            /*add the tmp unit chunks to the main unit */
            smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);
        }

        {
            SMEM_CHUNK_BASIC_STC  chunksMem[]=
            {
                /*Number of Trunk Members Table Entry%e*/
                 {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0b009000 ,0x0b009040 - 4), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32,4), SMEM_BIND_TABLE_MAC(trunkNumOfMembers)}
                /* base of 0x0b400000 - Trunk Table Trunk<n> Member<i> Entry */
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0b400000 ,4096) , SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(11, 4),SMEM_BIND_TABLE_MAC(trunkMembers)}
            };
            GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);
            SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

            smemInitMemChunk(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

            /*add the tmp unit chunks to the main unit */
            smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);
        }
}

/*******************************************************************************
*   smemLion2UnitIpcl
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Lion2 IPCL unit
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr     - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemLion2UnitIpcl
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
        {
            SMEM_CHUNK_BASIC_STC  chunksMem[]=
            {
                /* PCL_SIP */
                 {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC(0x0B800000 ,0x0B800024 )}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC(0x0B800060 ,0x0B800064 )}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC(0x0B800080 ,0x0B800088 )}
                /* CRC Hash Mask Memory */
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0B800400 ,448), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(70, 16),SMEM_BIND_TABLE_MAC(crcHashMask)}
                /* PCL DFX Area */
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0B800800 ,2048), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4)}
            };
            GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

            smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
        }
        /* chunks with formulas */
        {
            SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
            {
                {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0B80006C ,0)}, FORMULA_SINGLE_PARAMETER(2 , 0x4)}
                /* Policy Descriptor to Bridge Word<%n> Register */
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0B800200 ,0)}, FORMULA_SINGLE_PARAMETER(28 , 0x4)}
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0B8005C0 ,0),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4),SMEM_BIND_TABLE_MAC(pearsonHash)}, FORMULA_SINGLE_PARAMETER(16 , 0x4)}
                /* IPCL0_0,IPCL0_1,IPCL_1 Configuration Table */
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0B810000 ,0), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(23, 8),SMEM_BIND_TABLE_MAC(pclConfig)}, FORMULA_TWO_PARAMETERS(33792/4 , 0x4 , 3 , 0x00010000)}

            };
            GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
            SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

            smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

            /*add the tmp unit chunks to the main unit */
            smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);
        }
    }

/*******************************************************************************
*   smemLion2UnitIplr0
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Lion2 IPLR unit
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr     - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemLion2UnitIplr0
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    smemLion2UnitPolicerUnify(devObjPtr,unitPtr,SMEM_SIP5_PP_PLR_UNIT_IPLR_0_E);
}

/*******************************************************************************
*   smemLion2UnitMll
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  MLL unit
*
* INPUTS:
*       commonDevMemInfoPtr   - pointer to common device memory object.
*       unitPtr - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemLion2UnitMll
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
        {
            SMEM_CHUNK_BASIC_STC  chunksMem[]=
            {
                /* base of 0x0c800000 */
                 {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0c800000 ,0x0c800004 )}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0c800008 ,0x0c800010 )}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0c800100 ,0x0c80011c )}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0c800200 ,0x0c800214 )}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0c800984 ,0x0c800984 )}

                /* base of 0x0c880000 */
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0c880000 ,65536), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(128, 16) , SMEM_BIND_TABLE_MAC(mll)}      /* MLL Entry */
            };
            GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

            smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
        }

        /* chunks with formulas */
        {
            SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
            {
                /* base of 0x0c800000 */
                  {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0c800900 ,0),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 0x100),SMEM_BIND_TABLE_MAC(mllOutInterfaceCounter)}, FORMULA_SINGLE_PARAMETER(2, 0x100)}
                 ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0c800980 ,0),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 0x100),SMEM_BIND_TABLE_MAC(mllOutInterfaceConfig)}, FORMULA_SINGLE_PARAMETER(2, 0x100)}
            };
            GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
            SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

            smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

            /*add the tmp unit chunks to the main unit */
            smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);
        }
    }

/*******************************************************************************
*   smemLion2UnitEpcl
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Lion2 EPCL unit
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr     - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemLion2UnitEpcl
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
        {
            SMEM_CHUNK_BASIC_STC  chunksMem[]=
            {
                 {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0E000000 ,0x0E000000)} /*Egress Policy Global Configuration*/
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0E000090 ,0x0E0000A0 )} /*Egress Policy DIP Solicitation mask          */
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0E000200 ,0x0E000200 )} /*Egress Policer Global Configuration          */
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0E000230 ,0x0E000234 )} /*Egress Policer Port Based Metering           */
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0E008000 ,16896)    } /*Egress PCL Configuration Table                     */
            };
            GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

            smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
        }
        /* chunks with formulas */
        {
            SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
            {
                /* Egress Policy Configuration */
                  {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0E000010 ,0)}, FORMULA_TWO_PARAMETERS(4 , 0x4, 8, 0x10)}
                  /* Egress TCP/UDP Port Range Comparator */
                 ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0E000100 ,0)}, FORMULA_TWO_PARAMETERS(8 , 0x4, 2, 0x40)}
            };
            GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
            SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

            smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

            /*add the tmp unit chunks to the main unit */
            smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);
        }
    }

/*******************************************************************************
*   smemLion2UnitIpclTcc
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Lion2 IPCL_TCC unit
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr     - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemLion2UnitIpclTcc
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    GT_U32  baseAddr = 0x0D000000;
    GT_U32  ii;


    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            /* TCC Registers */
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0D000100 ,0x0D000170), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4),SMEM_BIND_TABLE_MAC(pclActionTcamData)} /*actionTableAndPolicyTCAMAccessCtrl*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0D000198 ,0x0D000198 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0D0001A4 ,0x0D0001C0 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0D000208 ,0x0D000238)} /*Policy_TCAM_Configuration         */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0D000304 ,0x0D00031C )}

            /* Tcam Bist Area */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0D000400 ,256 ),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4),SMEM_BIND_TABLE_MAC(tcamBistArea)}
            /* TCAM Array Compare Enable */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0D001000 ,256 ),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4),SMEM_BIND_TABLE_MAC(tcamArrayCompareEn)}
            /* LOWER TCC DFX Area */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0D002000 ,2048 )}
            /* SMEM_BIND_TABLE_MAC(pclTcam)     the table in simulation used with 0x40 step not 8 !!!! */
            /* SMEM_BIND_TABLE_MAC(pclTcamMask) the table in simulation used with 0x40 step not 8 !!!! */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0D040000 ,(2*_1K)*0x40) ,SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(52, 8)  } /*policy_TCAM  */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0D0C0000 ,(2*_1K)*4)    } /*policy_ECC_X */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0D0D0000 ,(2*_1K)*4)    } /*policy_ECC_Y */
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

        /* unify the base addresses */
        for(ii = 0 ; ii < numOfChunks ; ii++)
        {
            chunksMem[ii].memFirstAddr -= baseAddr;
        }

        smemInitMemChunk(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /* Update the main unit chunks */
        smemInitMemUpdateUnitChunks(devObjPtr,unitPtr, &tmpUnitChunk);
    }


    if(!SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        {
            SMEM_CHUNK_BASIC_STC  chunksMem[]=
            {
                 {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0D0B0000 ,(2*_1K)*0x10 ) ,SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(128, 0x10),SMEM_BIND_TABLE_MAC(pclAction)  } /*policy_Action_Table */
            };
            GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);
            SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

            /* unify the base addresses */
            for(ii = 0 ; ii < numOfChunks ; ii++)
            {
                chunksMem[ii].memFirstAddr -= baseAddr;
            }

            smemInitMemChunk(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

            /*add the tmp unit chunks to the main unit */
            smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);
        }
    }

}

/*******************************************************************************
*   smemLion2UnitIpvxTcc
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Lion2 IPvX_TCC unit
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr     - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemLion2UnitIpvxTcc
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    GT_U32  baseAddr = 0x0D800000;
    GT_U32  ii;


    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0D800400 ,0x0D80041C)} /*LTTAndRouterTCAMAccessCtrl_E*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0D800054 ,0x0D800054)} /*TCAM Error Counter               */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0D800060 ,0x0D80007C)} /*TCC_Registers               */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0D800080 ,0x0D800080)} /*TTI Key Type 2 Lookup ID              */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0D8000A0 ,0x0D8000A4)} /*Router DIP/SIP Lookup Key Type 2 Lookup ID             */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0D800960 ,0x0D800D78)} /* Router_TCAM_Configuration 0  */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0D802004 ,0x0D802004)} /*TCC RAM mask             */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0D802050 ,0x0D802058)} /*Data Error             */

            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0D800100 ,256)  } /* Tcam Bist Area           */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0D801000 ,256)  } /* TCAM Array Compare Enable      */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0D802000 ,2048 )  } /* UPPER TCC DFX Area      */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0D890000 ,8192)   } /*ECC_Table_X                 */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0D8a0000 ,8192)   } /*ECC_Table_y                 */

        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        /* unify the base addresses */
        for(ii = 0 ; ii < numOfChunks ; ii++)
        {
            chunksMem[ii].memFirstAddr -= baseAddr;
        }

        smemInitMemChunk(devObjPtr,chunksMem, numOfChunks, unitPtr);
    }

    if(!SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        {
            SMEM_CHUNK_BASIC_STC  chunksMem[]=
            {
                 {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0D840000 ,(131072*2)),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(52, 8),SMEM_BIND_TABLE_MAC(routerTcam) } /*routerAndTTTCAM_E
                            manually changed from  131072 ,
                            because there are 16K entries of 16 bytes --> total of 256K*/
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0D8C0000 ,131072),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(156, 0x20),SMEM_BIND_TABLE_MAC(ttiAction)   } /*lookup_Translation_Table_E  */
            };
            GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);
            SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

            /* unify the base addresses */
            for(ii = 0 ; ii < numOfChunks ; ii++)
            {
                chunksMem[ii].memFirstAddr -= baseAddr;
            }

            smemInitMemChunk(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

            /*add the tmp unit chunks to the main unit */
            smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);
        }
    }
    else
    {
        {
            SMEM_CHUNK_BASIC_STC  chunksMem[]=
            {
                /* size of routerTcam calculation: 8K entries of 5 banks of 8 bytes = 320K
                   the Lion2 had :                 8K entries of 4 banks of 8 bytes = 256K
                */
                 {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0D840000 ,((8*_1K)*(5*8))),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(52, 8),SMEM_BIND_TABLE_MAC(routerTcam) } /*routerAndTTTCAM_E*/
            };
            GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);
            SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

            /* unify the base addresses */
            for(ii = 0 ; ii < numOfChunks ; ii++)
            {
                chunksMem[ii].memFirstAddr -= baseAddr;
            }

            smemInitMemChunk(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

            /*add the tmp unit chunks to the main unit */
            smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);
        }

        {

            SMEM_CHUNK_BASIC_STC  chunksMem[]=
            {
                 {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0D8C0000 ,131072),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(246, 0x20),SMEM_BIND_TABLE_MAC(ttiAction)   } /*lookup_Translation_Table_E  */
            };
            GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);
            SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

            /* unify the base addresses */
            for(ii = 0 ; ii < numOfChunks ; ii++)
            {
                chunksMem[ii].memFirstAddr -= baseAddr;
            }

            smemInitMemChunk(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

            /*add the tmp unit chunks to the main unit */
            smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);
        }

    }


}

/*******************************************************************************
*   smemLion2UnitMppm1
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  MPPM1 unit
*
* INPUTS:
*       commonDevMemInfoPtr   - pointer to common device memory object.
*       unitPtr - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemLion2UnitMppm1
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{


    {
        /* Lion2 */
        SMEM_CHUNK_BASIC_STC chunksMem[] =
        {
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC(0x00000000 , 0x00000008)}
           ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC(0x00000010 , 0x00000010)}
           ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC(0x00000018 , 0x00000018)}
        };

        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
}

/*******************************************************************************
*   smemLion2UnitCPfc
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  CPFC unit
*
* INPUTS:
*       commonDevMemInfoPtr   - pointer to common device memory object.
*       unitPtr - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemLion2UnitCPfc
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    GT_U32  baseAddr = 0x13800000;
    GT_U32  ii;


    {
        /* Lion2 */
        SMEM_CHUNK_BASIC_STC chunksMem[] =
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC(0x13800000 , 0x13800000)}
            /* PFC Interrupts */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC(0x13800300 , 0x13800304)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC(0x13800320 , 0x13800324)}
        };

        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        /* unify the base addresses */
        for(ii = 0 ; ii < numOfChunks ; ii++)
        {
            chunksMem[ii].memFirstAddr -= baseAddr;
        }

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
    /* chunks with formulas */
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
            /* PFC Port Group <0/1/2/3/4/5/6/7> Global Overflow Interrupt Cause */
           {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x13800330 ,0)}, FORMULA_TWO_PARAMETERS(2 , 0x8, 8, 0x10)}
            /* PFC Port Group <0/1/2/3/4/5/6/7> Global Overflow Interrupt Mask */
           ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x13800334 ,0)}, FORMULA_TWO_PARAMETERS(2 , 0x8, 8, 0x10)}
            /* PFC Interrupts */
           ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x138003b0 ,0)}, FORMULA_TWO_PARAMETERS(2 , 0x4, 3, 0x10)}
           /* PFC Port Group Counters Statuses */
           ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x13800030 ,0)}, FORMULA_SINGLE_PARAMETER(8 , 0x4)}
           /* FC Port Group Flow Control Indication FIFO Overrun Entry*/
           ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x13800050 ,0)}, FORMULA_TWO_PARAMETERS(7 , 0x20, 16, 0x4)}
           /* PFC Group of Ports TC Counter and Threshhold */
           ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x13800500 ,0)}, FORMULA_THREE_PARAMETERS(8 , 0x20, 8, 0x4, 2, 0x100)}
           /* Profile TC XOff and XOn Thresholds */
           ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x13800800 ,0)}, FORMULA_TWO_PARAMETERS(64 , 0x4, 2, 0x200)}
           /* Port Group 0 PFC Counters */
           ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x13801000 ,0)}, FORMULA_TWO_PARAMETERS(104 , 0x4, 8, 0x800)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

        /* unify the base addresses */
        for(ii = 0 ; ii < numOfChunks ; ii++)
        {
            chunksMem[ii].memChunkBasic.memFirstAddr -= baseAddr;
        }

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);
    }

}


/*******************************************************************************
*   smemLion2UnitMemoryBindToChunk
*
* DESCRIPTION:
*       bind the units to the specific unit chunk with the generic function
*       build addresses of the units according to base address
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
void smemLion2UnitMemoryBindToChunk
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    GT_U32              ii,jj,kk,kkMax;
    SMEM_UNIT_CHUNKS_STC    *currUnitChunkPtr;
    SMEM_UNIT_CHUNKS_STC    *prevUnitChunkPtr;
    GT_U32  maxUnits = devObjPtr->genericNumUnitsAddresses;
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = devObjPtr->deviceMemory;
    SMEM_CHT_DEV_COMMON_MEM_INFO * commonDevMemInfoPtr = &devMemInfoPtr->common;
    GT_U32  unitBaseAddr;

    prevUnitChunkPtr = NULL;
    /* bind the units to the specific unit chunk with the generic function */
    for (ii = 0; ii < maxUnits ; ii++)
    {
        unitBaseAddr = smemUnitBaseAddrByIndexGet(devObjPtr,ii);
        if(unitBaseAddr == SMAIN_NOT_VALID_CNS)
        {
            continue;
        }

        /* calculate the unit ID as from the address of the unit in the array */
        jj = (unitBaseAddr >> SMEM_CHT_UNIT_INDEX_FIRST_BIT_CNS);

        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[jj];
        if(currUnitChunkPtr->hugeUnitSupportPtr)
        {
            /* check the 'actual unit' that represent the current unit */
            currUnitChunkPtr = currUnitChunkPtr->hugeUnitSupportPtr;
        }

        if((currUnitChunkPtr->otherPortGroupDevObjPtr == NULL || /*only the 'owner port group' init this */
            currUnitChunkPtr->otherPortGroupDevObjPtr == devObjPtr) &&
           currUnitChunkPtr->numOfChunks &&
           currUnitChunkPtr != prevUnitChunkPtr)
        {
        /* build addresses of the units according to base address */
            smemGenericUnitAddressesAlignToBaseAddress1(devObjPtr,
                currUnitChunkPtr,
                currUnitChunkPtr->hugeUnitSupportPtr ?
                    GT_TRUE : GT_FALSE);
        }

        /* support HUGE units with single unit indication ! - like 'LMS in Lion2' */
        kkMax = currUnitChunkPtr->numOfUnits;
        kkMax = kkMax ? kkMax : 1;
        if(currUnitChunkPtr->chunkType == SMEM_UNIT_CHUNK_TYPE_8_MSB_E)
        {
            kkMax *= 2;
        }

        for(kk = 1 ; kk < kkMax ; kk++)
        {
            commonDevMemInfoPtr->specFunTbl[jj + kk].specFun    =
                commonDevMemInfoPtr->specFunTbl[jj].specFun;

            commonDevMemInfoPtr->specFunTbl[jj + kk].specParam  =
                commonDevMemInfoPtr->specFunTbl[jj].specParam  ;

            devMemInfoPtr->unitMemArr[jj + kk].chunkIndex =
                devMemInfoPtr->unitMemArr[jj].chunkIndex;
        }


        prevUnitChunkPtr = currUnitChunkPtr;
    }
}

/*******************************************************************************
*   smemLion2AllocSpecMemory
*
* DESCRIPTION:
*       Allocate address type specific memories.
*
* INPUTS:
*       commonDevMemInfoPtr   - pointer to common device memory object.
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
void smemLion2AllocSpecMemory
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_CHT_DEV_COMMON_MEM_INFO  * commonDevMemInfoPtr
)
{
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = devObjPtr->deviceMemory;
    SMEM_UNIT_CHUNKS_STC    *currUnitChunkPtr;
    GT_U32  unitIndex;
    GT_BIT  allow_data_path_units = 1;
    GT_BIT  allow_control_pipe_units = 1;

    if (SMEM_CHT_IS_SIP5_15_GET(devObjPtr) == 1)
    {
        allow_data_path_units = 0;
        allow_control_pipe_units = 0;
    }
    else if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        allow_control_pipe_units = 0;
    }

    /* init the internal memory */
    smemChtInitInternalSimMemory(devObjPtr,commonDevMemInfoPtr);

    /* create all the memories of the units */
    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_MG)];
    smemLion2UnitMg(devObjPtr, currUnitChunkPtr);

    smemLion2UnitPex(devObjPtr,&devMemInfoPtr->PEX_UnitMem, devMemInfoPtr->common.pciUnitBaseAddr);

    if (allow_data_path_units)
    {
        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_MPPM)];
        smemLion2UnitMppm(devObjPtr,currUnitChunkPtr);

        if(devObjPtr->portGroupSharedDevObjPtr)
        {
            currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_CTU0)];
            smemLion2UnitCtu(devObjPtr,currUnitChunkPtr);

            currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_CTU1)];
            smemLion2UnitCtu(devObjPtr,currUnitChunkPtr);
        }

        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_XG_PORT_MIB)];
        smemLion2UnitXGPortMib(devObjPtr,currUnitChunkPtr);

        if(devObjPtr->portGroupSharedDevObjPtr == NULL)
        {
            /* bobcat2 with many ports */
            unitIndex = UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_XG_PORT_MIB);
            devMemInfoPtr->unitMemArr[unitIndex+ 0].hugeUnitSupportPtr =
            devMemInfoPtr->unitMemArr[unitIndex+ 1].hugeUnitSupportPtr =
                &devMemInfoPtr->unitMemArr[unitIndex];
        }

        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_SERDES)];
        smemLion2UnitLpSerdes(devObjPtr,currUnitChunkPtr);


        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_GOP)];
        smemLion2UnitGroupOfPorts(devObjPtr,currUnitChunkPtr);

        if(devObjPtr->portGroupSharedDevObjPtr)
        {
            currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_MPPM1)];
            smemLion2UnitMppm1(devObjPtr,currUnitChunkPtr);
        }
    }

    if(allow_control_pipe_units)
    {
        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TTI)];
        smemLion2UnitTti(devObjPtr, currUnitChunkPtr);

        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_L2I)];
        smemLion2UnitL2i(devObjPtr, currUnitChunkPtr);

        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_RX_DMA)];
        smemLion2UnitRxDma(devObjPtr,currUnitChunkPtr);

        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_FDB)];
        smemLion2UnitFdb(devObjPtr,currUnitChunkPtr);

        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TXQ_SHT)];
        smemLion2UnitTxqSht(devObjPtr,currUnitChunkPtr);

        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TXQ_EGR0)];
        smemLion2UnitTxqEgr(devObjPtr,currUnitChunkPtr,0);

        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TXQ_EGR1)];
        smemLion2UnitTxqEgr(devObjPtr,currUnitChunkPtr,1);

        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_EQ)];
        smemLion2UnitEq(devObjPtr,currUnitChunkPtr);

        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_HA)];
        smemLion2UnitHa(devObjPtr,currUnitChunkPtr);

        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_IPCL)];
        smemLion2UnitIpcl(devObjPtr,currUnitChunkPtr);

        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_EPCL)];
        smemLion2UnitEpcl(devObjPtr,currUnitChunkPtr);

        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_MLL)];
        smemLion2UnitMll(devObjPtr,currUnitChunkPtr);

        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_IPLR)];
        smemLion2UnitIplr0(devObjPtr,currUnitChunkPtr);

        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_IPLR1)];
        smemLion2UnitIplr1(devObjPtr, currUnitChunkPtr);

        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_EPLR)];
        smemLion2UnitEplr(devObjPtr,currUnitChunkPtr);

        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_CNC)];
        smemLion2UnitCentralizedCounters(devObjPtr,currUnitChunkPtr);

        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_IPVX)];
        smemLion2UnitIpvx(devObjPtr,currUnitChunkPtr);

        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_DFX_SERVER)];
        smemLion2UnitDfxServer(devObjPtr, currUnitChunkPtr);

        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_BM)];
        smemLion2UnitBm(devObjPtr,currUnitChunkPtr);

        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_BMA)];
        smemLion2UnitBma(devObjPtr,currUnitChunkPtr);

        unitIndex = UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_LMS);
        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[unitIndex];
        smemLion2UnitLms(devObjPtr,currUnitChunkPtr);
        /* 'connect' the next sub units to the first one
           and connect 'my self' */
        devMemInfoPtr->unitMemArr[unitIndex].hugeUnitSupportPtr =
        devMemInfoPtr->unitMemArr[unitIndex + 1].hugeUnitSupportPtr =
        devMemInfoPtr->unitMemArr[unitIndex + 2].hugeUnitSupportPtr =
        devMemInfoPtr->unitMemArr[unitIndex + 3].hugeUnitSupportPtr =
            &devMemInfoPtr->unitMemArr[unitIndex];

        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TXQ_QUEUE)];
        smemLion2UnitTxqQueue(devObjPtr,currUnitChunkPtr);

        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TXQ_LL)];
        smemLion2UnitTxqLl(devObjPtr,currUnitChunkPtr);

        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TXQ_DQ)];
        smemLion2UnitTxqDq(devObjPtr,currUnitChunkPtr);

        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_IPCL_TCC)];
        smemLion2UnitIpclTcc(devObjPtr,currUnitChunkPtr);

        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_IPVX_TCC)];
        smemLion2UnitIpvxTcc(devObjPtr,currUnitChunkPtr);

        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TX_DMA)];
        smemLion2UnitTxDma(devObjPtr,currUnitChunkPtr);

        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TXQ_DIST)];
        smemLion2UnitTxqDist(devObjPtr,currUnitChunkPtr);

        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_CPFC)];
        smemLion2UnitCPfc(devObjPtr,currUnitChunkPtr);
    }
}



/*******************************************************************************
*  smemLion2TableInfoSet
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
void smemLion2TableInfoSet
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    /* use lion table addresses */
    smemLionTableInfoSet(devObjPtr);

    /*******************/
    /* override lion-b */
    /*******************/

    if(devObjPtr->xgCountersStepPerPort == 0)
    {
        devObjPtr->xgCountersStepPerPort = XG_COUNTER_STEP_PER_PORT_CNS;
    }

    if(!SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        devObjPtr->flexFieldNumBitsSupport.phyPort          = 6;
        devObjPtr->flexFieldNumBitsSupport.trunkId          = 7;
        devObjPtr->flexFieldNumBitsSupport.ePort            = 6;
        devObjPtr->flexFieldNumBitsSupport.eVid             = 12;
        devObjPtr->flexFieldNumBitsSupport.eVidx            = 12;
        devObjPtr->flexFieldNumBitsSupport.hwDevNum         = 5;
        devObjPtr->flexFieldNumBitsSupport.sstId            = 5;
        devObjPtr->flexFieldNumBitsSupport.stgId            = 8;
        devObjPtr->flexFieldNumBitsSupport.arpPtr           = 14;
        devObjPtr->flexFieldNumBitsSupport.tunnelstartPtr   = 12;
        devObjPtr->flexFieldNumBitsSupport.fid              = 12;

        devObjPtr->limitedResources.eVid  = 4*1024;
        devObjPtr->limitedResources.ePort = 64;
        devObjPtr->limitedResources.phyPort = 64;

    }

    devObjPtr->globalInterruptCauseRegister.eq      = 20;
    devObjPtr->globalInterruptCauseRegister.rxDma   = SMAIN_NOT_VALID_CNS;
    devObjPtr->globalInterruptCauseRegister.txDma   = SMAIN_NOT_VALID_CNS;
    devObjPtr->globalInterruptCauseRegister.txqEgr1 = 26;
    devObjPtr->globalInterruptCauseRegister.eplr    = 27;
    devObjPtr->globalInterruptCauseRegister.ports   = 29;
    devObjPtr->globalInterruptCauseRegister.global1 = 29;

}
/*******************************************************************************
*  smemLion2TableInfoSetPart2
*
* DESCRIPTION:
*       set the table info for the device --> fill devObjPtr->tablesInfo
*       AFTER the bound of memories (after calling smemBindTablesToMemories)
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
void smemLion2TableInfoSetPart2
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr
)
{


    devObjPtr->fdbMaxNumEntries = SKERNEL_TABLE_NUM_OF_ENTRIES_GET_MAC(devObjPtr,fdb);

    devObjPtr->tablesInfo.trunkMembers.paramInfo[1].step = 0x20;
    /* the table was bound to memory that not match step 0 */
    devObjPtr->tablesInfo.logicalTrgMappingTable.paramInfo[0].step = 0x100;
    if(! SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        /* the table was bound to memory that not match step 0 */
        devObjPtr->tablesInfo.portProtocolVidQoSConf.paramInfo[0].step = 0x80;
    }

    devObjPtr->tablesInfo.xgPortMibCounters.commonInfo.baseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_XG_PORT_MIB) + 0;
    devObjPtr->tablesInfo.xgPortMibCounters.paramInfo[0].step = devObjPtr->xgCountersStepPerPort;

    if(devObjPtr->offsetToXgCounters_1)/*bobcat2*/
    {
        devObjPtr->tablesInfo.xgPortMibCounters_1.commonInfo.baseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_XG_PORT_MIB) + devObjPtr->offsetToXgCounters_1;
        devObjPtr->tablesInfo.xgPortMibCounters_1.paramInfo[0].step = devObjPtr->xgCountersStepPerPort_1;
    }

}

/*******************************************************************************
*   smemLion2Init
*
* DESCRIPTION:
*       Init memory module for the Lion2 device.
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
void smemLion2Init
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_CHT_DEV_COMMON_MEM_INFO * commonDevMemInfoPtr = NULL;
    GT_U32  ii;
    GT_U32  activeTableSizeOf;/* "sizeof" active table*/
    SMEM_CHT_GENERIC_DEV_MEM_INFO  * devMemInfoPtr;

    devObjPtr->devMemUnitNameAndIndexPtr = lion2UnitNameAndIndexArr;
    devObjPtr->genericUsedUnitsAddressesArray = lion2UsedUnitsAddressesArray;
    devObjPtr->genericNumUnitsAddresses = SMEM_LION2_UNIT_LAST_E;

    devObjPtr->specialUnitsBaseAddr.UNIT_TXQ_LL     = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_TXQ_LL);
    devObjPtr->specialUnitsBaseAddr.UNIT_TXQ_QUEUE  = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_TXQ_QUEUE);
    devObjPtr->specialUnitsBaseAddr.UNIT_TXQ_DIST   = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_TXQ_DIST);
    devObjPtr->specialUnitsBaseAddr.UNIT_BMA        = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_BMA);
    devObjPtr->specialUnitsBaseAddr.UNIT_CPFC       = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_CPFC);
    devObjPtr->specialUnitsBaseAddr.UNIT_DFX_SERVER = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_DFX_SERVER);

    if(devObjPtr->shellDevice == GT_TRUE)
    {
        GT_U32  ii;

        /* this is the port group shared device */
        /* so we need to fill info about it's port group devices */
        for( ii = 0 ; ii < devObjPtr->numOfCoreDevs ;ii++)
        {
            /* each of the port group devices start his ports at multiple of 16
               (each port group has 12 ports) */
            devObjPtr->coreDevInfoPtr[ii].startPortNum = 16*ii;
        }

        return;
    }


    devMemInfoPtr = smemDeviceObjMemoryAlloc(devObjPtr,1, sizeof(SMEM_CHT_GENERIC_DEV_MEM_INFO));
    if (devMemInfoPtr == 0)
    {
        skernelFatalError("smemLion2Init: allocation error\n");
    }

    devObjPtr->deviceMemory = devMemInfoPtr;
    commonDevMemInfoPtr= &devMemInfoPtr->common;
    commonDevMemInfoPtr->isPartOfGeneric = 1;

    devObjPtr->supportRegistersDb = 1;
    devObjPtr->designatedPortVersion = 1;
    devObjPtr->numOfTxqUnits = 2;

    /* lion2 */
    devObjPtr->supportExtPortMac = 1;
    if(devObjPtr->deviceRevisionId >= 1)
    {
        /* lion2 B0 and above */
        devObjPtr->supportStackAggregation = 1;

        devObjPtr->supportMaskAuFuMessageToCpuOnNonLocal_with3BitsCoreId = 1;

        if((devObjPtr->portGroupId == 0) ||
           (devObjPtr->portGroupId == 4))
        {
            devObjPtr->supportTxQPort14LinkUp = 1;
        }
    }

    /* set the pointer to the active memory */
    devObjPtr->activeMemPtr = smemLion2PortGroupActiveTable;
    activeTableSizeOf = smemLion2PortGroupActiveTableSizeOf;

    timestampBaseAddr = timestampBaseAddr_lion2;

    devObjPtr->supportDualDeviceID = 1;
    devObjPtr->ipvxSupport.ecmpIndexFormula = SKERNEL_ECMP_ROUTE_INDEX_FORMULA_TYPE_1_E;
    devObjPtr->iPclKeyFormatVersion = 1;
    devObjPtr->ePclKeyFormatVersion = 1;

    /* this relevant to lion2 and not to Lion3 (Sip5 should fix it) */
    devObjPtr->errata.crcTrunkHashL4InfoInIpv4Other = 1;

    /* bind the router TCAM info */
    devObjPtr->routeTcamInfo =  lion2PortGroupRoutTcamInfo;

    devObjPtr->supportMultiPortGroupFdbLookUpMode = 1;
    devObjPtr->notSupportIngressPortRateLimit = 1;
    devObjPtr->supportGemUserId = 1;
    devObjPtr->cncClientSupportBitmap = SNET_CHT3_CNC_CLIENTS_BMP_ALL_CNS;
    devObjPtr->supportRemoveVlanTag1WhenEmpty = 1;
    devObjPtr->pclSupport.ePclSupportVidUpTag1 = 1;
    devObjPtr->trafficGeneratorSupport.tgSupport = 1;

    devObjPtr->memUnitBaseAddrInfo.policer[0] = policerBaseAddr[0];
    devObjPtr->memUnitBaseAddrInfo.policer[1] = policerBaseAddr[1];
    devObjPtr->memUnitBaseAddrInfo.policer[2] = policerBaseAddr[2];
    /* Timestamp base addresses */
    devObjPtr->memUnitBaseAddrInfo.gts[0] = timestampBaseAddr[0];
    devObjPtr->memUnitBaseAddrInfo.gts[1] = timestampBaseAddr[1];

    if(devObjPtr->portGroupSharedDevObjPtr)
    {
        devObjPtr->portsArr[SMEM_LION_XLG_PORT_12_NUM_CNS].state = SKERNEL_PORT_STATE_XLG_40G_E;
        devObjPtr->portsArr[SMEM_LION_XLG_PORT_14_NUM_CNS].state = SKERNEL_PORT_STATE_XLG_40G_E;
    }

    devObjPtr->ttiTrillAdjacencyTcanInfo = lion2TtiTrillAdjacencyTcanInfo;
    /* bind the PCL TCAM info */
    devObjPtr->pclTcamInfoPtr = &lion2PortGroupGlobalPclData;
    devObjPtr->pclTcamMaxNumEntries = devObjPtr->pclTcamInfoPtr->bankEntriesNum * 4;

    devObjPtr->fdbNumEntries = SMEM_MAC_TABLE_SIZE_32KB;
    devObjPtr->supportFdbNewNaToCpuMsgFormat = 1;


    devObjPtr->supportEqEgressMonitoringNumPorts = 128;
    devObjPtr->deviceMapTableUseDstPort = 1;
    devObjPtr->supportVlanEntryVidxFloodMode = 1;
    devObjPtr->errata.routerGlueResetPartsOfDescriptor = 1;
    devObjPtr->errata.ttCopyToCpuWithAdditionalTag = 1;

    devObjPtr->isXcatA0Features = 0;/*state that features are NOT like xcat-A0*/
    devObjPtr->supportTunnelStartEthOverIpv4 = 1;
    devObjPtr->errata.fastStackFailover_Drop_on_source = 1;
    devObjPtr->errata.lttRedirectIndexError = 0;
    devObjPtr->errata.ieeeReservedMcConfigRegRead = 1;
    devObjPtr->errata.ipfixWrapArroundFreezeMode = 1;
    devObjPtr->errata.srcTrunkPortIsolationAsSrcPort = 1;
    devObjPtr->errata.ipfixTodIncrementModeWraparoundIssues = 1;

    devObjPtr->cncBlocksNum = 8;
    devObjPtr->cncBlockMaxRangeIndex = 512;
    devObjPtr->supportPortIsolation = 1;
    devObjPtr->supportOamPduTrap = 1;
    devObjPtr->supportPortOamLoopBack = 1;
    devObjPtr->policerEngineNum = 2;
    devObjPtr->supportMultiAnalyzerMirroring = 1;
    devObjPtr->supportCascadeTrunkHashDstPort = 1;
    devObjPtr->supportMultiPortGroupTTILookUpMode = 1;
    devObjPtr->supportTxQGlobalPorts = 1;
    devObjPtr->supportCrcTrunkHashMode = 1;
    devObjPtr->supportMacHeaderModification = 1;
    devObjPtr->supportHa64Ports = 1;
    devObjPtr->txqRevision = 1;
    devObjPtr->supportXgMacMibCountersControllOnAllPorts = 1;

    devObjPtr->supportLogicalTargetMapping = 1;/* support it only when 'EQ not support eArch' */
    devObjPtr->ipFixTimeStampClockDiff[0] =
    devObjPtr->ipFixTimeStampClockDiff[1] =
    devObjPtr->ipFixTimeStampClockDiff[2] = 0;
    devObjPtr->supportConfigurableTtiPclId = 1;
    devObjPtr->supportVlanEntryVidxFloodMode = 1;
    devObjPtr->supportForceNewDsaToCpu = 1;
    devObjPtr->supportCutThrough = 1;
    devObjPtr->supportKeepVlan1 = 1;
    devObjPtr->supportResourceHistogram = 1;

    devObjPtr->ipvxSupport.lttMaxNumberOfPaths = SKERNEL_LTT_MAX_NUMBER_OF_PATHS_64_E;
    devObjPtr->ipvxSupport.qosIndexFormula = SKERNEL_QOS_ROUTE_INDEX_FORMULA_TYPE_1_E;
    devObjPtr->ipvxSupport.supportPerVlanURpfMode = 1;
    devObjPtr->ipvxSupport.supportPerPortSipSaCheckEnable = 1;

    devObjPtr->supportMacSaAssignModePerPort = 1;
    devObjPtr->supportMaskAuFuMessageToCpuOnNonLocal = 1;

    devObjPtr->pclSupport.iPclSupport5Ude = 1;
    devObjPtr->pclSupport.iPclSupportTrunkHash = 1;
    devObjPtr->pclSupport.ipclSupportSeparatedIConfigTables = 1;
    devObjPtr->pclSupport.pclSupportPortList = 1;
    devObjPtr->pclTcamFormatVersion = 1;

    devObjPtr->supportDevMapTableOnOwnDev   = 1;
    devObjPtr->supportPtp = 1;

    devObjPtr->policerSupport.supportPolicerMemoryControl = 1;
    devObjPtr->policerSupport.iplr0TableSize = POLICER_MEMORY_1792_CNS + POLICER_MEMORY_256_CNS;
    devObjPtr->policerSupport.iplr1TableSize = 0;
    devObjPtr->policerSupport.iplrTableSize =
        devObjPtr->policerSupport.iplr0TableSize +
        devObjPtr->policerSupport.iplr1TableSize;
    devObjPtr->policerSupport.supportPolicerEnableCountingTriggerByPort   = 1;
    devObjPtr->policerSupport.supportPortModeAddressSelect = 1;
    devObjPtr->policerSupport.supportEplrPerPort = 1;
    devObjPtr->policerSupport.supportIpfixTimeStamp = 1;
    devObjPtr->policerSupport.supportCountingEntryFormatSelect = 1;
    devObjPtr->supportForwardFcPackets = 1;

    /* Update active memory table data only once */
    if (devObjPtr->portGroupId == 0)
    {
        for(ii = 0 ; ii < activeTableSizeOf ; ii++)
        {
            if(devObjPtr->activeMemPtr[ii].mask ==
                POLICER_ACTIVE_MEMORY_PLACE_HOLDER_MASK_CNS)
            {
                GT_U32  tmpMask = SMEM_FULL_MASK_CNS;

                if(devObjPtr->activeMemPtr[ii + 0].address == POLICER_MANAGEMENT_COUNTER_ADDR_CNS)
                {
                    /* management counters */
                    /* 0x40 between sets , 0x10 between counters */
                    /* 0x00 , 0x10 , 0x20 , 0x30 */
                    /* 0x40 , 0x50 , 0x60 , 0x70 */
                    /* 0x80 , 0x90 , 0xa0 , 0xb0 */
                    tmpMask = POLICER_MANAGEMENT_COUNTER_MASK_CNS;
                }

                devObjPtr->activeMemPtr[ii + 0].address += devObjPtr->memUnitBaseAddrInfo.policer[0];
                devObjPtr->activeMemPtr[ii + 0].mask = tmpMask;

                devObjPtr->activeMemPtr[ii + 1].address += devObjPtr->memUnitBaseAddrInfo.policer[1];
                devObjPtr->activeMemPtr[ii + 1].mask = tmpMask;

                devObjPtr->activeMemPtr[ii + 2].address += devObjPtr->memUnitBaseAddrInfo.policer[2];
                devObjPtr->activeMemPtr[ii + 2].mask = tmpMask;

                ii += 2;/* the jump of 2 here + the one of the loop will
                           jump to next 3 policer if exists */
            }
            else
            if(devObjPtr->activeMemPtr[ii].mask ==
                GTS_ACTIVE_MEMORY_PLACE_HOLDER_MASK_CNS)
            {
                /* init the GTS entry */
                GENERIC_ACTIVE_MEM_ENTRY_PLACE_HOLDER_INIT_MAC(devObjPtr,gts,&devObjPtr->activeMemPtr[ii]);
            }
        }
    }
    commonDevMemInfoPtr->pciUnitBaseAddrMask = 0x000f0000;
    commonDevMemInfoPtr->pciUnitBaseAddr = 0x00070000;

    devObjPtr->devIsOwnerMemFunPtr = smemLion2IsDeviceMemoryOwner;
    devObjPtr->devFindMemFunPtr = (void *)smemLion2FindMem;


    /* init specific functions array */
    smemLion2InitFuncArray(devObjPtr,commonDevMemInfoPtr);

    smemLion2AllocSpecMemory(devObjPtr,commonDevMemInfoPtr);

    smemLion2UnitMemoryBindToChunk(devObjPtr);

    smemLion2RegsInfoSet(devObjPtr);

    smemLion2TableInfoSet(devObjPtr);

    /* bind now tables to memories --
       MUST be done after calling smemLion2TableInfoSet(...)
       so the tables can override previous settings */
    smemBindTablesToMemories(devObjPtr,
        /* array of x units !!! (the units are not set as the first units only , but can be spread)*/
        devMemInfoPtr->unitMemArr,
        SMEM_CHT_NUM_UNITS_MAX_CNS);

    smemLion2TableInfoSetPart2(devObjPtr);
}

/*******************************************************************************
*   smemLion2Init2
*
* DESCRIPTION:
*       Init memory module for a device - after the load of the default
*           registers file
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
void smemLion2Init2
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    GT_U32  regAddr;
    GT_U32 fieldVal;

    if(devObjPtr->shellDevice == GT_TRUE)
    {
        /* the shell device ... no extra initialization */
        return;
    }

    smemChtInit2(devObjPtr);


    /* read the registers - as was set during load of the 'Registers defaults file' */
    if(!SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        if(devObjPtr->portGroupSharedDevObjPtr)
        {
            /* Set port group ID */
            smemRegFldSet(devObjPtr, SMEM_CHT_GLB_CTRL_REG(devObjPtr), 9, 3, devObjPtr->portGroupId);
        }

        /* cause to check the FDB size mode */
        regAddr = SMEM_CHT_MAC_TBL_GLB_CONF_REG(devObjPtr);
        smemRegUpdateAfterRegFile(devObjPtr,regAddr,1);

        /* cause to check EQ dual device id mode */
        regAddr = SMEM_CHT_MAC_REG_DB_GET(devObjPtr)->EQ.preEgrEngineGlobalConfig.dualDeviceIDAndLossyDropConfig;
        smemRegUpdateAfterRegFile(devObjPtr,regAddr,1);

        /* cause to check TTI dual device id mode */
        regAddr = SMEM_LION_TTI_INTERNAL_CONF_REG(devObjPtr);
        smemRegUpdateAfterRegFile(devObjPtr,regAddr,1);
        /* cause to check HA dual device id mode */
        regAddr = SMEM_CHT3_ROUTE_HA_GLB_CNF_REG(devObjPtr);
        smemRegUpdateAfterRegFile(devObjPtr,regAddr,1);

    /* Lion2 Hooper support - has 1 TxQ unit only */

        /* Read the DevID from register Device ID  */
        smemRegFldGet(devObjPtr, SMEM_CHT_DEVICE_ID_REG(devObjPtr), 4, 16, &fieldVal);

        if(fieldVal == 0x8000)
        {
            devObjPtr->numOfTxqUnits = 1;
            devObjPtr->supportStackAggregation = 1;
        }
    }

    return;
}

#define MIB_ENTRY_SIZE 32

/*******************************************************************************
*  smemLion2ActiveReadMsmMibCounters
*
* DESCRIPTION:
*       MSM MIB counters read.
*
* INPUTS:
*       devObjPtr   - device object PTR.
*       address     - Address for ASIC memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers' specific parameter -
*                     global interrupt bit number.
*
* OUTPUTS:
*       outMemPtr   - Pointer to the memory to copy register's content.
* RETURNS:
*
* COMMENTS:
*       Performs capturing of the MAC MIB entry to internal memory on
*       first word read. The capture is reseted on last word read.
*
*******************************************************************************/
void smemLion2ActiveReadMsmMibCounters
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    OUT        GT_U32 * outMemPtr
)
{
    GT_U32  regAddr;                /* Register address */
    GT_U32  wordNumber;
    GT_U32  port;                   /* port number */
    GT_U32  *internalMemPtr;
    GT_U32  *regCopyFromPtr;        /* pointer to registers content to copy from */
    SKERNEL_MIB_COUNTER_INFO_STC mibInfo;

    smemMibCounterInfoByAddrGet(devObjPtr,address,&mibInfo);

    port = mibInfo.portNum;

    wordNumber = (address & 0xFF) >> 2;

    internalMemPtr = CHT_INTERNAL_MEM_PTR(devObjPtr,CHT_INTERNAL_SIMULATION_USE_MEM_MSM_MIB_COUNTERS_START_E
                                      + (MIB_ENTRY_SIZE * port));

    if (wordNumber == 0 && devObjPtr->portsArr[port].isMibCountersCaptured == GT_FALSE)
    {
        /* copy the values of MIB entry to temp memory */

        regAddr = SMEM_CHT_HGS_PORT_MAC_COUNT_REG(devObjPtr, port);
        regCopyFromPtr = smemMemGet(devObjPtr, regAddr);

        memcpy(internalMemPtr, regCopyFromPtr, (MIB_ENTRY_SIZE * 4));
        /* clear the counters */
        memset(regCopyFromPtr, 0, (MIB_ENTRY_SIZE * 4));
        devObjPtr->portsArr[port].isMibCountersCaptured = GT_TRUE;
    }

    /* copy the value from temp memory*/
    *outMemPtr = *(internalMemPtr + wordNumber);

    if (wordNumber == (MIB_ENTRY_SIZE - 1))
    {
        /* set the reset */
        devObjPtr->portsArr[port].isMibCountersCaptured = GT_FALSE;
    }

}

/* Active memory for write HA Global Configuration register */
void smemLion2ActiveWriteHaGlobalConfReg
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR param,
    INOUT      GT_U32 * inMemPtr
)
{
    /* Output value */
    *memPtr = *inMemPtr;

    devObjPtr->dualDeviceIdEnable.ha = SMEM_U32_GET_FIELD(*memPtr, 27, 1);

    /* set the TXQ with save value -- TEMO until we aware about specific configuration */
    devObjPtr->dualDeviceIdEnable.txq = devObjPtr->dualDeviceIdEnable.ha;
}

/* Active memory for write TTI Internal Configuration register */
void smemLion2ActiveWriteTtiInternalConfReg
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    INOUT      GT_U32 * inMemPtr
)
{
    /* Output value */
    *memPtr = *inMemPtr;
    /* Dual Device Id */
    devObjPtr->dualDeviceIdEnable.tti = SMEM_U32_GET_FIELD(*memPtr, 5, 1);
}

/* Active memory for write EQ Dual Device ID register */
void smemLion2ActiveWriteEqDualDeviceIdReg
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    INOUT      GT_U32 * inMemPtr
)
{
    /* Output value */
    *memPtr = *inMemPtr;

    devObjPtr->dualDeviceIdEnable.eq = SMEM_U32_GET_FIELD(*memPtr, 0, 1);
}

/*******************************************************************************
*  smemLion2ActiveWriteFDBGlobalCfgReg
*
* DESCRIPTION:
*   The function updates FDB table size according to FDB_Global_Configuration
*
*
* INPUTS:
*    devObjPtr  - device object PTR.
*    address    - Address for ASIC memory.
*    memPtr     - Pointer to the register's memory in the simulation.
*    memSize    - Size of memory to be written
*    param      - Registers' specific parameter.
*    inMemPtr   - Pointer to the memory to get register's content.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void smemLion2ActiveWriteFDBGlobalCfgReg
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
)
{
    GT_U32 fdbSize;

    /* data to be written */
    *memPtr = *inMemPtr;

    /* bit[24],[25]  - FDBSize */
    fdbSize = (*memPtr >> 24) & 0x3;
    /* bit[30] - FDBSize-msb */
    fdbSize |= ((*memPtr >> 30) & 0x1) << 2;

    switch(fdbSize)
    {
        case 0:
            devObjPtr->fdbNumEntries = SMEM_MAC_TABLE_SIZE_4KB;
            break;
        case 1:
            devObjPtr->fdbNumEntries = SMEM_MAC_TABLE_SIZE_8KB;
            break;
        case 2:
            devObjPtr->fdbNumEntries = SMEM_MAC_TABLE_SIZE_16KB;
            break;
        case 3:
            devObjPtr->fdbNumEntries = SMEM_MAC_TABLE_SIZE_32KB;
            break;
        case 4:
            devObjPtr->fdbNumEntries = SMEM_MAC_TABLE_SIZE_64KB;
            break;
        default:
            break;
    }

    if(devObjPtr->fdbMaxNumEntries < devObjPtr->fdbNumEntries)
    {
        skernelFatalError("smemLion2ActiveWriteFDBGlobalCfgReg: FDB num entries in HW [0x%4.4x] , but set to use[0x%4.4x] \n",
            devObjPtr->fdbMaxNumEntries , devObjPtr->fdbNumEntries);
    }


    return;
}

/*******************************************************************************
*  smemLion2ActiveWriteHierarchicalPolicerControl
*
* DESCRIPTION:
*    Set the Hierarchical Policer Control register in iplr0,
*    and update Policer Memory Control configuration
*
* INPUTS:
*    devObjPtr  - device object PTR.
*    address    - Address for ASIC memory.
*    memPtr     - Pointer to the register's memory in the simulation.
*    memSize    - Size of memory to be written
*    param      - Registers' specific parameter.
*    inMemPtr   - Pointer to the memory to get register's content.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void smemLion2ActiveWriteHierarchicalPolicerControl
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
)
{
    GT_U32 fldValue;
    GT_U32 meterBase;        /* metering table start addr in iplr0 (tablesInfo.policer) */
    GT_U32 countBase;        /* counters table start addr in iplr0 (tablesInfo.policerCounters) */
    GT_U32 meterEntrySize, countEntrySize;        /* table entry size 0x20, equal for both tables */

    /* set register content */
    *memPtr = *inMemPtr ;

    meterBase = devObjPtr->policerSupport.meterTblBaseAddr;
    countBase = devObjPtr->policerSupport.countTblBaseAddr;
    meterEntrySize = devObjPtr->tablesInfo.policer.paramInfo[0].step;        /* size of entry */
    countEntrySize = devObjPtr->tablesInfo.policerCounters.paramInfo[0].step;        /* size of entry */

    /* get "Policer Memory Control 0/1" - bits 6:5 */
    fldValue = SMEM_U32_GET_FIELD(*inMemPtr, 5, 2);
    switch (fldValue)
    {
        case 0: /* PLR0_UP_AND_LW:Policer 0 uses both memories */
            devObjPtr->tablesInfo.policer.commonInfo.baseAddress = meterBase;
            devObjPtr->tablesInfo.policer.paramInfo[1].step = SMAIN_NOT_VALID_CNS;
            devObjPtr->tablesInfo.policerCounters.commonInfo.baseAddress = countBase;
            devObjPtr->tablesInfo.policerCounters.paramInfo[1].step = SMAIN_NOT_VALID_CNS;
            devObjPtr->policerSupport.iplr0TableSize = 2048;
            devObjPtr->policerSupport.iplr1TableSize = 0;
            devObjPtr->policerSupport.iplr1EntriesFirst = 0;
            break;
        case 1: /* PLR1_UP_PLR0_LW:Policer 1 uses the upper memory (1792) Policer 0 uses the lower memory (256) */
            devObjPtr->tablesInfo.policer.commonInfo.baseAddress = meterBase + POLICER_MEMORY_1792_CNS * meterEntrySize;
            devObjPtr->tablesInfo.policer.paramInfo[1].step = 0 - (POLICER_MEMORY_1792_CNS * meterEntrySize);
            devObjPtr->tablesInfo.policerCounters.commonInfo.baseAddress = countBase + POLICER_MEMORY_1792_CNS * countEntrySize;
            devObjPtr->tablesInfo.policerCounters.paramInfo[1].step = 0 - (POLICER_MEMORY_1792_CNS * countEntrySize);
            devObjPtr->policerSupport.iplr0TableSize = POLICER_MEMORY_256_CNS;
            devObjPtr->policerSupport.iplr1TableSize = POLICER_MEMORY_1792_CNS;
            devObjPtr->policerSupport.iplr1EntriesFirst = 1;
            break;
        case 2: /* PLR0_UP_PLR1_LW:Policer 0 uses the upper memory (1792) Policer 1 uses the lower memory (256) */
            devObjPtr->tablesInfo.policer.commonInfo.baseAddress = meterBase;
            devObjPtr->tablesInfo.policer.paramInfo[1].step = POLICER_MEMORY_1792_CNS * meterEntrySize;
            devObjPtr->tablesInfo.policerCounters.commonInfo.baseAddress = countBase;
            devObjPtr->tablesInfo.policerCounters.paramInfo[1].step = POLICER_MEMORY_1792_CNS * countEntrySize;
            devObjPtr->policerSupport.iplr0TableSize = POLICER_MEMORY_1792_CNS;
            devObjPtr->policerSupport.iplr1TableSize = POLICER_MEMORY_256_CNS;
            devObjPtr->policerSupport.iplr1EntriesFirst = 0;
            break;
        case 3: /* PLR1_UP_AND_LW:Policer 1 uses both memories */
            devObjPtr->tablesInfo.policer.commonInfo.baseAddress = SMAIN_NOT_VALID_CNS;
            devObjPtr->tablesInfo.policer.paramInfo[1].step = meterBase - SMAIN_NOT_VALID_CNS;
            devObjPtr->tablesInfo.policerCounters.commonInfo.baseAddress = SMAIN_NOT_VALID_CNS;
            devObjPtr->tablesInfo.policerCounters.paramInfo[1].step = countBase - SMAIN_NOT_VALID_CNS;
            /* Note: (commonInfo.baseAddress + paramInfo[1].step) now points to start of table */
            devObjPtr->policerSupport.iplr0TableSize = 0;
            devObjPtr->policerSupport.iplr1TableSize = 2048;
            devObjPtr->policerSupport.iplr1EntriesFirst = 0;
            break;
        default:
            break;
    }
}


/*******************************************************************************
*  smemLion2ActiveWriteKVCOCalibrationControlReg
*
* DESCRIPTION:
*       Set <Calibration Done> bit in KVCO Calibration Control register to
*       signal that calibration done
*
* INPUTS:
*       devObjPtr   - device object PTR.
*       address     - Address for ASIC memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers' specific parameter -
*                     global interrupt bit number.
*       inMemPtr    - Pointer to the memory to get register's content.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
void smemLion2ActiveWriteKVCOCalibrationControlReg
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
)
{
    GT_U32 reg_value;

    /* data to be written */
    reg_value = *inMemPtr;

    /* This is the main auto calibration state machine done signal. */
    reg_value |= (1<<14);

    /* write to simulation memory */
    *memPtr = reg_value;

    return;
}

/*******************************************************************************
*  smemLion2ActiveWriteTgControl0Reg
*
* DESCRIPTION:
*
*   The function starts task for traffic generator when <Enable generator> is enable
*   and <Start trigger> is set.
*   If task has started successfully -
*   devObjPtr->trafficGeneratorSupport.trafficGenData[tgNumber].trafficGenActive is set to GT_TRUE
*
* INPUTS:
*    devObjPtr  - device object PTR.
*    address    - Address for ASIC memory.
*    memPtr     - Pointer to the register's memory in the simulation.
*    memSize    - Size of memory to be written
*    param      - Registers' specific parameter.
*    inMemPtr   - Pointer to the memory to get register's content.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*   If traffic generator is active - task creation is bypassed.
*
*******************************************************************************/
void smemLion2ActiveWriteTgControl0Reg
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
)
{
    GT_U32 port;
    GT_U32 tgNumber;
    GT_U32 fieldVal;

    /* data to be written */
    *memPtr = *inMemPtr;

    port = CHT_FROM_ADDRESS_GET_PORT_CNS(devObjPtr, address);

    /* One traffic generator per mini-GOP(4 ports) */
    tgNumber = port/4;

    /* Check TG state */
    if(devObjPtr->trafficGeneratorSupport.trafficGenData[tgNumber].trafficGenActive == GT_FALSE)
    {
        /* Enable TG on port */
        fieldVal = SMEM_U32_GET_FIELD(memPtr[0], 0, 1);
        if(fieldVal == 0)
        {
            return;
        }

        /* Start trigger */
        fieldVal = SMEM_U32_GET_FIELD(memPtr[0], 4, 1);
        if(fieldVal == 0)
        {
            return ;
        }
        /* Create traffic generator task */
        skernelCreatePacketGenerator(devObjPtr, tgNumber);
    }
}


static void removeBaseAddrFromRegistersClear9Msb(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_CHT_PP_REGS_ADDR_STC        * regAddrDbPtr
)
{
    GT_U32  *u32Ptr = (GT_U32*)regAddrDbPtr;
    GT_U32   numOfWords = sizeof(SMEM_CHT_PP_REGS_ADDR_STC) / sizeof(GT_U32);
    GT_U32   numOfWordsForNewUnitHeader = sizeof(SMEM_CHT_PP_REGS_UNIT_START_INFO_STC) / sizeof(GT_U32);
    GT_U32  ii;
    GT_U32  currAddr;
    GT_U32  mask = 0x007FFFFF;/*23 bits*/
    GT_U32  unitAddr;
    GT_U32  unitMask = ~(mask);
    GT_U32  declaredUnitBaseAddr = 0;

    for(ii = 0 ; ii < numOfWords; ii++,u32Ptr++)
    {
        currAddr = (*u32Ptr);
        if(currAddr == SMEM_CHT_PP_REGS_UNIT_START_INFO_MAGIC_NUMBER_CNS)
        {
            declaredUnitBaseAddr = u32Ptr[1]; /*SMEM_CHT_PP_REGS_UNIT_START_INFO_STC::unitBaseAddress*/
            ii      += numOfWordsForNewUnitHeader;
            u32Ptr  += numOfWordsForNewUnitHeader;
            currAddr = (*u32Ptr);
        }

        if(declaredUnitBaseAddr == SMAIN_NOT_VALID_CNS)
        {
            /* the unit with all it's registers are not valid in this device */
            (*u32Ptr) = SMAIN_NOT_VALID_CNS;
            continue;
        }


        if(currAddr == SMAIN_NOT_VALID_CNS)
        {
            continue;
        }

        unitAddr = currAddr & unitMask;
        switch(unitAddr)
        {
            case 0x04000000:
            case 0x05000000:
            case 0x04800000:
                /*LMS sub units*/
                *u32Ptr -= 0x04000000;
                continue;
            default :
                break;
        }

        /* reduce the value of register to be 'Relative to start of unit' */
        *u32Ptr &= mask;
    }


}

/*******************************************************************************
* smemLion2RegsInfoSetPart2
*
* DESCRIPTION:
*       align the addresses of registers according to addresses of corresponding
*       units
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
static void smemLion2RegsInfoSetPart2
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_CHT_PP_REGS_ADDR_STC        * regAddrDbPtr;
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = devObjPtr->deviceMemory;
    SMEM_UNIT_CHUNKS_STC    *currUnitChunkPtr;

    regAddrDbPtr = SMEM_CHT_MAC_REG_DB_GET(devObjPtr);

    /* clear 9 MSBits of all legacy registers in the DB */
    removeBaseAddrFromRegistersClear9Msb(devObjPtr,regAddrDbPtr);

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_MG)];
    smemGenericRegistersArrayAlignToUnit(
        REG_PTR_AND_SIZE_MAC(regAddrDbPtr->MG),
        currUnitChunkPtr);

    if(!SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TTI)];
        smemGenericRegistersArrayAlignToUnit(
            REG_PTR_AND_SIZE_MAC(regAddrDbPtr->TTI),
            currUnitChunkPtr);
    }

    if(!SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_DFX_SERVER)];
        smemGenericRegistersArrayAlignToUnit(
            REG_PTR_AND_SIZE_MAC(regAddrDbPtr->DFX_SERVER),
            currUnitChunkPtr);

        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_L2I)];
        smemGenericRegistersArrayAlignToUnit(
            REG_PTR_AND_SIZE_MAC(regAddrDbPtr->L2I),
            currUnitChunkPtr);

        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_IPVX)];
        smemGenericRegistersArrayAlignToUnit(
        REG_PTR_AND_SIZE_MAC(regAddrDbPtr->IPvX),
        currUnitChunkPtr);

        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_IPVX)];
        smemGenericRegistersArrayAlignToUnit(
            REG_PTR_AND_SIZE_MAC(regAddrDbPtr->ucRouter),
            currUnitChunkPtr);
    }

    if(!SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_BM)];
        smemGenericRegistersArrayAlignToUnit(
            REG_PTR_AND_SIZE_MAC(regAddrDbPtr->BM),
            currUnitChunkPtr);

        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_FDB)];
        smemGenericRegistersArrayAlignToUnit(
            REG_PTR_AND_SIZE_MAC(regAddrDbPtr->FDB),
            currUnitChunkPtr);

        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_CNC)];
        smemGenericRegistersArrayAlignToUnit(
            REG_PTR_AND_SIZE_MAC(regAddrDbPtr->centralizedCntrs),
            currUnitChunkPtr);

    }

        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_MPPM)];
        smemGenericRegistersArrayAlignToUnit(
            REG_PTR_AND_SIZE_MAC(regAddrDbPtr->MPPM0),
            currUnitChunkPtr);

    if(devObjPtr->portGroupSharedDevObjPtr)
    {
    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_CTU0)];
    smemGenericRegistersArrayAlignToUnit(
        REG_PTR_AND_SIZE_MAC(regAddrDbPtr->CTU0),
        currUnitChunkPtr);
    }

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_GOP)];
    smemGenericRegistersArrayAlignToUnit(
        REG_PTR_AND_SIZE_MAC(regAddrDbPtr->GOP.ports),
        currUnitChunkPtr);

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_GOP)];
    smemGenericRegistersArrayAlignToUnit(
        REG_PTR_AND_SIZE_MAC(regAddrDbPtr->GOP.packGenConfig),
        currUnitChunkPtr);


    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_SERDES)];
    smemGenericRegistersArrayAlignToUnit(
        REG_PTR_AND_SIZE_MAC(regAddrDbPtr->GOP.SERDESes),
        currUnitChunkPtr);

    if(devObjPtr->portGroupSharedDevObjPtr)
    {
    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_MPPM1)];
    smemGenericRegistersArrayAlignToUnit(
        REG_PTR_AND_SIZE_MAC(regAddrDbPtr->MPPM1),
        currUnitChunkPtr);

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_CTU1)];
    smemGenericRegistersArrayAlignToUnit(
        REG_PTR_AND_SIZE_MAC(regAddrDbPtr->CTU1),
        currUnitChunkPtr);
    }

    if(!SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_EQ)];
        smemGenericRegistersArrayAlignToUnit(
            REG_PTR_AND_SIZE_MAC(regAddrDbPtr->EQ),
            currUnitChunkPtr);

        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_IPCL)];
        smemGenericRegistersArrayAlignToUnit(
            REG_PTR_AND_SIZE_MAC(regAddrDbPtr->IPCL),
            currUnitChunkPtr);
    }

    if(!SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_IPLR)];
        smemGenericRegistersArrayAlignToUnit(
            REG_PTR_AND_SIZE_MAC(regAddrDbPtr->PLR[0]),
            currUnitChunkPtr);

            currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_IPLR1)];
            smemGenericRegistersArrayAlignToUnit(
                REG_PTR_AND_SIZE_MAC(regAddrDbPtr->PLR[1]),
                currUnitChunkPtr);

            currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_EPLR)];
            smemGenericRegistersArrayAlignToUnit(
                REG_PTR_AND_SIZE_MAC(regAddrDbPtr->PLR[2]),
                currUnitChunkPtr);
    }

    if(!devObjPtr->supportEArch)
    {
        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_IPCL_TCC)];
    smemGenericRegistersArrayAlignToUnit(
        REG_PTR_AND_SIZE_MAC(regAddrDbPtr->TCCLowerIPCL),
        currUnitChunkPtr);

        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_IPVX_TCC)];
    smemGenericRegistersArrayAlignToUnit(
        REG_PTR_AND_SIZE_MAC(regAddrDbPtr->TCCUpperIPvX),
        currUnitChunkPtr);
    }

    if(!SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_EPCL)];
        smemGenericRegistersArrayAlignToUnit(
            REG_PTR_AND_SIZE_MAC(regAddrDbPtr->EPCL),
            currUnitChunkPtr);

        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_MLL)];
        smemGenericRegistersArrayAlignToUnit(
            REG_PTR_AND_SIZE_MAC(regAddrDbPtr->MLL),
            currUnitChunkPtr);

        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_HA)];
        smemGenericRegistersArrayAlignToUnit(
            REG_PTR_AND_SIZE_MAC(regAddrDbPtr->HA),
            currUnitChunkPtr);
    }

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_RX_DMA)];
    smemGenericRegistersArrayAlignToUnit(
        REG_PTR_AND_SIZE_MAC(regAddrDbPtr->RXDMA),
        currUnitChunkPtr);

    if(!SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TX_DMA)];
        smemGenericRegistersArrayAlignToUnit(
            REG_PTR_AND_SIZE_MAC(regAddrDbPtr->TXDMA),
            currUnitChunkPtr);

        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TXQ_DIST)];
        smemGenericRegistersArrayAlignToUnit(
            REG_PTR_AND_SIZE_MAC(regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_DIST),
            currUnitChunkPtr);

        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TXQ_EGR0)];
        smemGenericRegistersArrayAlignToUnit(
            REG_PTR_AND_SIZE_MAC(regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_EGR[0]),
            currUnitChunkPtr);

        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TXQ_EGR1)];
        smemGenericRegistersArrayAlignToUnit(
            REG_PTR_AND_SIZE_MAC(regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_EGR[1]),
            currUnitChunkPtr);

            currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TXQ_SHT)];
            smemGenericRegistersArrayAlignToUnit(
                REG_PTR_AND_SIZE_MAC(regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_SHT),
                currUnitChunkPtr);

        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TXQ_QUEUE)];
        smemGenericRegistersArrayAlignToUnit(
            REG_PTR_AND_SIZE_MAC(regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue),
            currUnitChunkPtr);

        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TXQ_LL)];
        smemGenericRegistersArrayAlignToUnit(
            REG_PTR_AND_SIZE_MAC(regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_LL),
            currUnitChunkPtr);

        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TXQ_DQ)];
        smemGenericRegistersArrayAlignToUnit(
            REG_PTR_AND_SIZE_MAC(regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_DQ),
            currUnitChunkPtr);

        currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_LMS)];
        smemGenericRegistersArrayAlignToUnit1(
            REG_PTR_AND_SIZE_MAC(regAddrDbPtr->LMS1),
            currUnitChunkPtr,
            GT_TRUE);/* addresses are '0' based and need += of the 'base unit address' */
    }
}


void smemLion2RegsInfoSet_GOP_SERDES
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SMEM_CHT_PP_REGS_ADDR_STC        * regAddrDbPtr,
    IN GT_U32                   s/*serdes*/,
    IN GT_U32                   extraOffset,
    IN GT_BIT                   isGopVer1
)
{
    GT_U32 serdesOffset = isGopVer1 ? 0x1000 : 0x400;

    {/*0x0000000+s*serdesOffset + extraOffset*/
        regAddrDbPtr->GOP.SERDESes.SERDES[s].SERDESExternalConfig[0] =
            0x0000000+s*serdesOffset + extraOffset;
    }/*0x0000000+s*serdesOffset + extraOffset*/


    {/*0x0000004+s*serdesOffset + extraOffset*/
        regAddrDbPtr->GOP.SERDESes.SERDES[s].SERDESExternalConfig[1] =
            0x0000004+s*serdesOffset + extraOffset;
    }/*0x0000004+s*serdesOffset + extraOffset*/


    {/*0x0000008+s*serdesOffset + extraOffset*/
        regAddrDbPtr->GOP.SERDESes.SERDES[s].SERDESExternalStatus =
            (isGopVer1 ? 0x0000018 : 0x0000008)
            +s*serdesOffset + extraOffset;
    }/*0x0000008+s*serdesOffset + extraOffset*/


    {/*0x000000c+s*serdesOffset + extraOffset*/
        regAddrDbPtr->GOP.SERDESes.SERDES[s].SERDESExternalConfig[2] =
            (isGopVer1 ? 0x0000008 : 0x000000c)
            +s*serdesOffset + extraOffset;
    }/*0x000000c+s*serdesOffset + extraOffset*/



    {/*0x0000010+s*serdesOffset + extraOffset*/
        regAddrDbPtr->GOP.SERDESes.SERDES[s].SERDESMiscellaneous =
            (isGopVer1 ? 0x0000014 : 0x0000010)
            +s*serdesOffset + extraOffset;
    }/*0x0000010+s*serdesOffset + extraOffset*/


    {/*0x0000014+s*serdesOffset + extraOffset*/
        regAddrDbPtr->GOP.SERDESes.SERDES[s].SERDESExternalConfig[3] =
            (isGopVer1 ? 0x000000c : 0x0000014)
            +s*serdesOffset + extraOffset;
    }/*0x0000014+s*serdesOffset + extraOffset*/
}

static void smemLion2RegsInfoSet_GOP_gigPort63CPU
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SMEM_CHT_PP_REGS_ADDR_STC        * regAddrDbPtr,
    IN GT_U32                   extraOffset
)
{
    regAddrDbPtr->GOP.ports.gigPort63CPU.portAutoNegConfig = 0x0003f00c + extraOffset;

    regAddrDbPtr->GOP.ports.gigPort63CPU.portInterruptCause = 0x0003f020 + extraOffset;
    regAddrDbPtr->GOP.ports.gigPort63CPU.portInterruptMask = 0x0003f024 + extraOffset;

    regAddrDbPtr->GOP.ports.gigPort63CPU.portSerialParametersConfig = 0x0003f014 + extraOffset;

    regAddrDbPtr->GOP.ports.gigPort63CPU.portStatusReg0 = 0x0003f010 + extraOffset;

    regAddrDbPtr->GOP.ports.gigPort63CPU.portPRBSStatusReg = 0x0003f038 + extraOffset;
    regAddrDbPtr->GOP.ports.gigPort63CPU.portPRBSErrorCntr = 0x0003f03c + extraOffset;

    regAddrDbPtr->GOP.ports.gigPort63CPU.portMACCtrlReg[0] = 0x0003f000 + extraOffset;
    regAddrDbPtr->GOP.ports.gigPort63CPU.portMACCtrlReg[1] = 0x0003f004 + extraOffset;
    regAddrDbPtr->GOP.ports.gigPort63CPU.portMACCtrlReg[2] = 0x0003f008 + extraOffset;
    regAddrDbPtr->GOP.ports.gigPort63CPU.portMACCtrlReg[3] = 0x0003f048 + extraOffset;
    {/*0x883f058+p*4*/
        GT_U32    p;
        for(p = 0 ; p <= 7 ; p++) {
            regAddrDbPtr->GOP.ports.gigPort63CPU.CCFCPortSpeedTimer[p] =
                0x883f058+p*4 + extraOffset;
        }/* end of loop p */
    }/*0x883f058+p*4*/


    regAddrDbPtr->GOP.ports.gigPort63CPU.portMACCtrlReg[4] = 0x0003f090 + extraOffset;
}

void smemLion2RegsInfoSet_GOP_gigPort
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SMEM_CHT_PP_REGS_ADDR_STC        * regAddrDbPtr,
    IN GT_U32                   p/*port*/,
    IN GT_U32                   minus_p/*compensation port*/,
    IN GT_U32                   extraOffset,
    IN GT_BIT                   isGopVer1
)
{/*start of unit gigPort */
    GT_U32    l;
    regAddrDbPtr->GOP.ports.gigPort[p].portAutoNegConfig =
        0x0000000c+(p-minus_p)*0x1000 + extraOffset;
    regAddrDbPtr->GOP.ports.gigPort[p].portInterruptCause =
        0x00000020+(p-minus_p)*0x1000 + extraOffset;
    regAddrDbPtr->GOP.ports.gigPort[p].portInterruptMask =
        0x00000024+(p-minus_p)*0x1000 + extraOffset;
    regAddrDbPtr->GOP.ports.gigPort[p].portSerialParametersConfig =
        0x00000014+(p-minus_p)*0x1000 + extraOffset;
    regAddrDbPtr->GOP.ports.gigPort[p].portStatusReg0 =
        0x00000010+(p-minus_p)*0x1000 + extraOffset;
    regAddrDbPtr->GOP.ports.gigPort[p].portPRBSStatusReg =
        0x00000038+(p-minus_p)*0x1000 + extraOffset;
    regAddrDbPtr->GOP.ports.gigPort[p].portPRBSErrorCntr =
        0x0000003c+(p-minus_p)*0x1000 + extraOffset;
    regAddrDbPtr->GOP.ports.gigPort[p].config100FXRegports2427Only =
        0x0000002c+(p-minus_p)*0x1000 + extraOffset;
    regAddrDbPtr->GOP.ports.gigPort[p].portMACCtrlReg[0] =
        0x00000000+(p-minus_p)*0x1000 + extraOffset;
    regAddrDbPtr->GOP.ports.gigPort[p].portMACCtrlReg[1] =
        0x00000004+(p-minus_p)*0x1000 + extraOffset;
    regAddrDbPtr->GOP.ports.gigPort[p].portMACCtrlReg[2] =
        0x00000008+(p-minus_p)*0x1000 + extraOffset;
    regAddrDbPtr->GOP.ports.gigPort[p].portMACCtrlReg[3] =
        0x00000048+(p-minus_p)*0x1000 + extraOffset;
    {/*0x0000058+l*4+(p-minus_p)*0x1000 + extraOffset*/
        for(l = 0 ; l <= 7 ; l++) {
            regAddrDbPtr->GOP.ports.gigPort[p].CCFCPortSpeedTimer[l] =
                0x0000058+l*4+(p-minus_p)*0x1000 + extraOffset;
        }/* end of loop p */
    }/*0x0000058+l*4+(p-minus_p)*0x1000 + extraOffset*/

    regAddrDbPtr->GOP.ports.gigPort[p].portMACCtrlReg[4] =
        0x00000090+(p-minus_p)*0x1000 + extraOffset;

    if(isGopVer1)
    {
        regAddrDbPtr->GOP.ports.gigPort[p].stackPortMIBCntrsCtrl =
            0x00000044+(p-minus_p)*0x1000 + extraOffset;

        regAddrDbPtr->GOP.ports.gigPort[p].portInterruptSummaryCause =
            0x000000A0+(p-minus_p)*0x1000 + extraOffset;

        regAddrDbPtr->GOP.ports.gigPort[p].portInterruptSummaryMask =
            0x000000A4+(p-minus_p)*0x1000 + extraOffset;

        regAddrDbPtr->GOP.ports.gigPort[p].lpiControlReg[0] =
            0x000000C0+(p-minus_p)*0x1000 + extraOffset;
        regAddrDbPtr->GOP.ports.gigPort[p].lpiControlReg[1] =
            0x000000C4+(p-minus_p)*0x1000 + extraOffset;
        regAddrDbPtr->GOP.ports.gigPort[p].lpiStatus =
            0x000000CC+(p-minus_p)*0x1000 + extraOffset;
    }


    COPY_TO_OTHER_PORT(devObjPtr,ports.gigPort,p);

}/*end of unit gigPort */

void smemLion2RegsInfoSet_GOP_XLGIP
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SMEM_CHT_PP_REGS_ADDR_STC        * regAddrDbPtr,
    IN GT_U32                   p/*port*/,
    IN GT_U32                   minus_p/*compensation port*/,
    IN GT_U32                   extraOffset
)
{
    {/*0x00c0000+(p-minus_p)*0x1000 and 0x00c0000+(p-minus_p)*0x1000*/
        regAddrDbPtr->GOP.ports.XLGIP[p].portMACCtrlReg[0] =
            0x00c0000+(p-minus_p)*0x1000 + extraOffset;
    }/*0x00c0000+(p-minus_p)*0x1000 + extraOffset and 0x00c0000+(p-minus_p)*0x1000 + extraOffset*/


    {/*0x00c0004+(p-minus_p)*0x1000 + extraOffset and 0x00c0004+(p-minus_p)*0x1000 + extraOffset*/
        regAddrDbPtr->GOP.ports.XLGIP[p].portMACCtrlReg[1] =
            0x00c0004+(p-minus_p)*0x1000 + extraOffset;
    }/*0x00c0004+(p-minus_p)*0x1000 + extraOffset and 0x00c0004+(p-minus_p)*0x1000 + extraOffset*/


    {/*0x00c0008+(p-minus_p)*0x1000 + extraOffset and 0x00c0008+(p-minus_p)*0x1000 + extraOffset*/
        regAddrDbPtr->GOP.ports.XLGIP[p].portMACCtrlReg[2] =
            0x00c0008+(p-minus_p)*0x1000 + extraOffset;
    }/*0x00c0008+(p-minus_p)*0x1000 + extraOffset and 0x00c0008+(p-minus_p)*0x1000 + extraOffset*/


    {/*0x00c000c+(p-minus_p)*0x1000 + extraOffset and 0x00c000c+(p-minus_p)*0x1000 + extraOffset*/
        regAddrDbPtr->GOP.ports.XLGIP[p].portStatus =
            0x00c000c+(p-minus_p)*0x1000 + extraOffset;
    }/*0x00c000c+(p-minus_p)*0x1000 + extraOffset and 0x00c000c+(p-minus_p)*0x1000 + extraOffset*/


    {/*0x00c0014+(p-minus_p)*0x1000 + extraOffset and 0x00c0014+(p-minus_p)*0x1000 + extraOffset*/
        regAddrDbPtr->GOP.ports.XLGIP[p].portInterruptCause =
            0x00c0014+(p-minus_p)*0x1000 + extraOffset;
    }/*0x00c0014+(p-minus_p)*0x1000 + extraOffset and 0x00c0014+(p-minus_p)*0x1000 + extraOffset*/


    {/*0x00c0018+(p-minus_p)*0x1000 + extraOffset and 0x00c0018+(p-minus_p)*0x1000 + extraOffset*/
        regAddrDbPtr->GOP.ports.XLGIP[p].portInterruptMask =
            0x00c0018+(p-minus_p)*0x1000 + extraOffset;
    }/*0x00c0018+(p-minus_p)*0x1000 + extraOffset and 0x00c0018+(p-minus_p)*0x1000 + extraOffset*/


    {/*0x00c001c+(p-minus_p)*0x1000 + extraOffset and 0x00c001c+(p-minus_p)*0x1000 + extraOffset*/
        regAddrDbPtr->GOP.ports.XLGIP[p].portMACCtrlReg[3] =
            0x00c001c+(p-minus_p)*0x1000 + extraOffset;
    }/*0x00c001c+(p-minus_p)*0x1000 + extraOffset and 0x00c001c+(p-minus_p)*0x1000 + extraOffset*/


    {/*0x00c0020+(p-minus_p)*0x1000 + extraOffset and 0x00c0020+(p-minus_p)*0x1000 + extraOffset*/
        regAddrDbPtr->GOP.ports.XLGIP[p].portPerPrioFlowCtrlStatus =
            0x00c0020+(p-minus_p)*0x1000 + extraOffset;
    }/*0x00c0020+(p-minus_p)*0x1000 + extraOffset and 0x00c0020+(p-minus_p)*0x1000 + extraOffset*/

    {/*0x00c0030+(p-minus_p)*0x1000 + extraOffset and 0x00c0030+(p-minus_p)*0x1000 + extraOffset*/
        regAddrDbPtr->GOP.ports.XLGIP[p].xgMIBCountersControl =
            0x00c0030+(p-minus_p)*0x1000 + extraOffset;
    }/*0x00c0020+(p-minus_p)*0x1000 + extraOffset and 0x00c0020+(p-minus_p)*0x1000 + extraOffset*/


    {/*0x00c0038+i*4 + p*0x1000 + extraOffset and 0x00c0038+i*4 + p*0x1000 + extraOffset*/
        GT_U32    i;
        for(i = 0 ; i <= 7 ; i++) {
            regAddrDbPtr->GOP.ports.XLGIP[p].CNCCFCTimer[i] =
                0x00c0038+i*4 +(p-minus_p)*0x1000 + extraOffset;
        }/* end of loop i */
    }/*0x00c0038+i*4 + p*0x1000 + extraOffset and 0x00c0038+i*4 + p*0x1000 + extraOffset*/


    {/*0x00c0058+(p-minus_p)*0x1000 + extraOffset and 0x00c0058+(p-minus_p)*0x1000 + extraOffset*/
        regAddrDbPtr->GOP.ports.XLGIP[p].externalUnitsInterruptsCause =
            0x00c0058+(p-minus_p)*0x1000 + extraOffset;
    }/*0x00c0058+(p-minus_p)*0x1000 + extraOffset and 0x00c0058+(p-minus_p)*0x1000 + extraOffset*/


    {/*0x00c005c+(p-minus_p)*0x1000 + extraOffset and 0x00c005c+(p-minus_p)*0x1000 + extraOffset*/
        regAddrDbPtr->GOP.ports.XLGIP[p].externalUnitsInterruptsMask =
            0x00c005c+(p-minus_p)*0x1000 + extraOffset;
    }/*0x00c005c+(p-minus_p)*0x1000 + extraOffset and 0x00c005c+(p-minus_p)*0x1000 + extraOffset*/


    {/*0x00c0060+(p-minus_p)*0x1000 + extraOffset and 0x00c0060+(p-minus_p)*0x1000 + extraOffset*/
        regAddrDbPtr->GOP.ports.XLGIP[p].PPFCCtrl =
            0x00c0060+(p-minus_p)*0x1000 + extraOffset;
    }/*0x00c0060+(p-minus_p)*0x1000 + extraOffset and 0x00c0060+(p-minus_p)*0x1000 + extraOffset*/


    {/*0x00c0084+(p-minus_p)*0x1000 + extraOffset and 0x00c0084+(p-minus_p)*0x1000 + extraOffset*/
        regAddrDbPtr->GOP.ports.XLGIP[p].portMACCtrlReg[4] =
            0x00c0084+(p-minus_p)*0x1000 + extraOffset;
    }/*0x00c0084+(p-minus_p)*0x1000 + extraOffset and 0x00c0084+(p-minus_p)*0x1000 + extraOffset*/


    /* multi pipe support */
    COPY_TO_OTHER_PORT(devObjPtr,ports.XLGIP,p);

}

void smemLion2RegsInfoSet_GOP_MPCSIP
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SMEM_CHT_PP_REGS_ADDR_STC        * regAddrDbPtr,
    IN GT_U32                   p/*port*/,
    IN GT_U32                   minus_p/*compensation port*/,
    IN GT_U32                   extraOffset
)
{
    {/*0x000C0400 + p*0x1000 + extraOffset and 0x000C0400 + p*0x1000 + extraOffset*/
        regAddrDbPtr->GOP.ports.MPCSIP[p].PCS40GCommonControl =
            0x000C0014+(p-minus_p)*0x1000 + extraOffset;
    }/*0x000C0400 + p*0x1000 + extraOffset and 0x000C0400 + p*0x1000*/

    COPY_TO_OTHER_PORT(devObjPtr,ports.MPCSIP,p);

}

static void smemLion2RegsInfoSet_GOP_packGenConfig
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SMEM_CHT_PP_REGS_ADDR_STC        * regAddrDbPtr,
    IN GT_U32                   p/*port*/,
    IN GT_U32                   minus_p/*compensation port*/,
    IN GT_U32                   extraOffset
)
{
    regAddrDbPtr->GOP.packGenConfig.packGenConfig[p].macDa0 =
        0x00000000 + (p * 0x1000 + extraOffset);
    regAddrDbPtr->GOP.packGenConfig.packGenConfig[p].macSa0 =
        0x0000000C + (p * 0x1000 + extraOffset);
    regAddrDbPtr->GOP.packGenConfig.packGenConfig[p].etherType =
        0x00000018 + (p * 0x1000 + extraOffset);
    regAddrDbPtr->GOP.packGenConfig.packGenConfig[p].vlanTag =
        0x0000001C + (p * 0x1000 + extraOffset);
    regAddrDbPtr->GOP.packGenConfig.packGenConfig[p].dataPattern0 =
        0x00000020 + (p * 0x1000 + extraOffset);
    regAddrDbPtr->GOP.packGenConfig.packGenConfig[p].packetLength =
        0x00000040 + (p * 0x1000 + extraOffset);
    regAddrDbPtr->GOP.packGenConfig.packGenConfig[p].packetCount  =
        0x00000044 + (p * 0x1000 + extraOffset);
    regAddrDbPtr->GOP.packGenConfig.packGenConfig[p].ifg =
        0x00000048 + (p * 0x1000 + extraOffset);
    regAddrDbPtr->GOP.packGenConfig.packGenConfig[p].controlReg0 =
        0x00000050 + (p * 0x1000 + extraOffset);

}

/*******************************************************************************
* smemLion2RegsInfoSet
*
* DESCRIPTION:
*       Init memory module for Lion2 and above devices.
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
void smemLion2RegsInfoSet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{

    SMEM_CHT_PP_REGS_ADDR_STC        * regAddrDbPtr;
    GT_U32  unitBaseAddress;
    GT_BIT  isBobcat2  =  (devObjPtr->portGroupSharedDevObjPtr == NULL) ? 1 : 0;
    GT_BIT  isBobcat3  =  (devObjPtr->deviceType == SKERNEL_BOBCAT3) ? 1 : 0;
    GT_BIT  isBobK     =  ((devObjPtr->deviceType == SKERNEL_BOBK_CAELUM) || 
                           (devObjPtr->deviceType == SKERNEL_BOBK_CETUS)) ? 1 : 0;
    GT_U32  numOfGigMacPorts = (isBobcat2 == 0) ? 12    : devObjPtr->portsNumber;
    GT_U32  numOfXlgMacPorts = (isBobcat2 == 0) ? 14    : (devObjPtr->portsNumber-1);
    GT_U32  skipXlgPorts  = (isBobcat2 == 0)    ? 1     : 0;
    GT_U32  numOfSerdeses = (isBobcat2 == 0)  ? 24    : 36;
    GT_BIT  isGopVer1 = isBobcat2 ? 1 : 0; /* GOP ver 0 for Lion2 ,
                                              GOP ver 1 for Bobcat2
                                              GOP ver 0 for Lion3 (TBD ... need to be ver1 too) */
    GT_U32  firstXlgPort = ((!SMEM_CHT_IS_SIP5_GET(devObjPtr)) ||
                            IS_CHT_HYPER_GIGA_PORT(devObjPtr,0)) ?
                            0 :
                            48;

    if(isBobcat3)
    {
        firstXlgPort = 0;
        numOfGigMacPorts =   36;/* init GOP of only 'pipe 0'*/
        numOfXlgMacPorts = 36-1;/* use '-1' because code uses '<=' with this value
                        ... unlike for numOfGigMacPorts that used '<' */
    }


    regAddrDbPtr = SMEM_CHT_MAC_REG_DB_GET(devObjPtr); 

    /*fill the DB with 'not valid addresses' */
    smemChtRegDbPreInit(devObjPtr);

    if(!SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_TTI);
        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,TTI                ,unitBaseAddress);

        unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_RX_DMA);
        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,RXDMA              ,unitBaseAddress);

        unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_FDB);
        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,FDB              ,unitBaseAddress);

        unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_L2I);
        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,L2I                ,unitBaseAddress);

        unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_EQ);
        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,EQ                 ,unitBaseAddress);

        unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_HA);
        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,HA                 ,unitBaseAddress);

        unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_MLL);
        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,MLL                ,unitBaseAddress);

        unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_IPCL);
        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,IPCL               ,unitBaseAddress);

        unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_EPCL);
        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,EPCL               ,unitBaseAddress);

        unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_IPLR);
        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,PLR[0]              ,unitBaseAddress);
        unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_IPLR1);
        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,PLR[1]              ,unitBaseAddress);
        unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_EPLR);
        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,PLR[2]             ,unitBaseAddress);
    }

    {
        /* must be done after calling smemChtRegDbPreInit(...) */
        unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_MG);
        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,MG                 ,unitBaseAddress);
/*        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,egrAndTxqVer0      ,0); not use unit */


        if(!SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_TXQ_QUEUE);
            SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,egrAndTxqVer1.TXQ.TXQ_Queue      ,unitBaseAddress);
            unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_TXQ_DIST);
            SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,egrAndTxqVer1.TXQ.TXQ_DIST      ,unitBaseAddress);
            unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_TXQ_SHT);
            SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,egrAndTxqVer1.TXQ.TXQ_SHT      ,unitBaseAddress);
            unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_TXQ_DQ);
            SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,egrAndTxqVer1.TXQ.TXQ_DQ      ,unitBaseAddress);
            unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_TXQ_LL);
            SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,egrAndTxqVer1.TXQ.TXQ_LL      ,unitBaseAddress);
            unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_TXQ_EGR0);
            SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,egrAndTxqVer1.TXQ.TXQ_EGR[0]      ,unitBaseAddress);
            unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_TXQ_EGR1);
            SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,egrAndTxqVer1.TXQ.TXQ_EGR[1]      ,unitBaseAddress);
            unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_LMS);
            SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,LMS1                              ,unitBaseAddress);
        }


        unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_IPVX);
        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,ucRouter           ,unitBaseAddress);
        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,IPvX               ,unitBaseAddress);
/*        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,LMS                ,0); not use unit*/
        unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_MPPM);
        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,MPPM0              ,unitBaseAddress);
        if(devObjPtr->portGroupSharedDevObjPtr)
        {
        unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_MPPM1);
        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,MPPM1              ,unitBaseAddress);
        unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_CTU0);
        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,CTU0               ,unitBaseAddress);
        unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_CTU1);
        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,CTU1               ,unitBaseAddress);
        }

        if(!devObjPtr->supportEArch)
        {
            unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_BM);
            SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,BM                 ,unitBaseAddress);

            unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_TX_DMA);
            SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,TXDMA              ,unitBaseAddress);


            unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_CNC);
            SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,centralizedCntrs   ,unitBaseAddress);

            unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_IPCL_TCC);
            SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,TCCLowerIPCL       ,unitBaseAddress);
            unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_IPVX_TCC);
            SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,TCCUpperIPvX       ,unitBaseAddress);
        }

        /* not for bobcat2 */
        if(devObjPtr->portGroupSharedDevObjPtr)
        {
        unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_DFX_SERVER);
        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,DFX_SERVER         ,unitBaseAddress);
        }
        unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_GOP);
        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,GOP.ports          ,unitBaseAddress);
        unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_GOP);
        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,GOP.packGenConfig       ,unitBaseAddress);
        unitBaseAddress = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_SERDES);
        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,GOP.SERDESes       ,unitBaseAddress);

        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,temp               ,0);/* dummy unit */
    }



    if(! SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {/*start of unit CNC - CNC  */
        {/*start of unit DFX */
            regAddrDbPtr->centralizedCntrs.DFX.dataErrorInterruptCause = 0x08000110;
            regAddrDbPtr->centralizedCntrs.DFX.dataErrorInterruptMask = 0x08000114;
            {/*0x08000118+n*4*/
                GT_U32    n;
                for(n = 0 ; n <= 7 ; n++) {
                    regAddrDbPtr->centralizedCntrs.DFX.dataErrorStatusReg[n] =
                        0x08000118+n*4;
                }/* end of loop n */
            }/*0x08000118+n*4*/
        }/*end of unit DFX */

        {/*start of unit globalRegs */
            regAddrDbPtr->centralizedCntrs.globalRegs.CNCGlobalConfigReg = 0x08000000;
            regAddrDbPtr->centralizedCntrs.globalRegs.CNCFastDumpTriggerReg = 0x08000030;
            regAddrDbPtr->centralizedCntrs.globalRegs.CNCClearByReadValueRegWord0 = 0x08000040;
            regAddrDbPtr->centralizedCntrs.globalRegs.CNCClearByReadValueRegWord1 = 0x08000044;
            regAddrDbPtr->centralizedCntrs.globalRegs.CNCInterruptSummaryCauseReg = 0x08000100;
            regAddrDbPtr->centralizedCntrs.globalRegs.CNCInterruptSummaryMaskReg = 0x08000104;
            regAddrDbPtr->centralizedCntrs.globalRegs.CNCFunctionalInterruptCauseReg = 0x08000108;
            regAddrDbPtr->centralizedCntrs.globalRegs.CNCFunctionalInterruptMaskReg = 0x0800010c;
        }/*end of unit globalRegs */

        {/*start of unit perBlockRegs */
            {/*0x080010a0+n*0x100+m*0x4*/
                GT_U32    n,m;
                for(n = 0 ; n <= 7 ; n++) {
                    for(m = 0 ; m <= 3 ; m++) {
                        regAddrDbPtr->centralizedCntrs.perBlockRegs.CNCBlockWraparoundStatusReg[n][m] =
                            0x080010a0+n*0x100+m*0x4;
                    }/* end of loop m */
                }/* end of loop n */
            }/*0x080010a0+n*0x100+m*0x4*/
            {/*0x08001180+n*0x100*/
                GT_U32    n;
                for(n = 0 ; n <= 7 ; n++) {
                    regAddrDbPtr->centralizedCntrs.perBlockRegs.CNCBlockConfigReg[0][n] =
                        0x08001180+n*0x100;
                }/* end of loop n */
            }/*0x08001180+n*0x100*/
            {/*0x08001184+n*0x100*/
                GT_U32    n;
                for(n = 0 ; n <= 7 ; n++) {
                    regAddrDbPtr->centralizedCntrs.perBlockRegs.CNCBlockConfigReg[1][n] =
                        0x08001184+n*0x100;
                }/* end of loop n */
            }/*0x08001184+n*0x100*/
        }/*end of unit perBlockRegs */
    }/*end of unit CNC - CNC  */

    {/*start of unit MG - MG  */
        {/*start of unit TWSIInternalRegs */
            /*not exists in Lion2
            regAddrDbPtr->MG.TWSIConfig.TWSIInternalBaudRate = 0x0040000c;*/
            regAddrDbPtr->MG.TWSIConfig.TWSIGlobalConfig = 0x00000010;
            regAddrDbPtr->MG.TWSIConfig.TWSILastAddr = 0x00000014;
            regAddrDbPtr->MG.TWSIConfig.TWSITimeoutLimit = 0x00000018;
            regAddrDbPtr->MG.TWSIConfig.TWSIStateHistory0 = 0x00000020;
            regAddrDbPtr->MG.TWSIConfig.TWSIStateHistory1 = 0x00000024;
        }/*end of unit TWSIInternalRegs */

        {/*start of unit addrUpdateQueueConfig */
            regAddrDbPtr->MG.addrUpdateQueueConfig.generalAddrUpdateQueueBaseAddr = 0x000000c0;
            regAddrDbPtr->MG.addrUpdateQueueConfig.generalAddrUpdateQueueCtrl = 0x000000c4;
            regAddrDbPtr->MG.addrUpdateQueueConfig.FUAddrUpdateQueueBaseAddr = 0x000000c8;
            regAddrDbPtr->MG.addrUpdateQueueConfig.FUAddrUpdateQueueCtrl = 0x000000cc;
        }/*end of unit addrUpdateQueueConfig */

        {/*start of unit MGRegsMGMiscellaneous */
            regAddrDbPtr->MG.MGRegsMGMiscellaneous.confiProcessorGlobalConfig = 0x00000500;
            regAddrDbPtr->MG.MGRegsMGMiscellaneous.Z80DoorbellReg = 0x00000504;
            regAddrDbPtr->MG.MGRegsMGMiscellaneous.Z80GlobalInterruptCause = 0x00000508;
            regAddrDbPtr->MG.MGRegsMGMiscellaneous.Z80GlobalInterruptMask = 0x0000050c;
            regAddrDbPtr->MG.MGRegsMGMiscellaneous.Z80InterruptCause = 0x00000510;
            regAddrDbPtr->MG.MGRegsMGMiscellaneous.Z80InterruptMask = 0x00000514;
            if(devObjPtr->cpuPortNoSpecialMac == 0)/* the BC2 not have those registers */
            {
                regAddrDbPtr->MG.cpuPortConfigRegAndMIBCntrs.cpuPortGoodFramesSentCntr = 0x00000060;
                regAddrDbPtr->MG.cpuPortConfigRegAndMIBCntrs.cpuPortMACTransErrorFramesSentCntr = 0x00000064;
                regAddrDbPtr->MG.cpuPortConfigRegAndMIBCntrs.cpuPortGoodOctetsSentCntr = 0x00000068;
                regAddrDbPtr->MG.cpuPortConfigRegAndMIBCntrs.cpuPortRxInternalDropCntr = 0x0000006c;
                regAddrDbPtr->MG.cpuPortConfigRegAndMIBCntrs.cpuPortGoodFramesReceivedCntr = 0x00000070;
                regAddrDbPtr->MG.cpuPortConfigRegAndMIBCntrs.cpuPortBadFramesReceivedCntr = 0x00000074;
                regAddrDbPtr->MG.cpuPortConfigRegAndMIBCntrs.cpuPortGoodOctetsReceivedCntr = 0x00000078;
                regAddrDbPtr->MG.cpuPortConfigRegAndMIBCntrs.cpuPortBadOctetsReceivedCntr = 0x0000007c;
                regAddrDbPtr->MG.cpuPortConfigRegAndMIBCntrs.cpuPortGlobalConfig = 0x000000a0;
            }

            regAddrDbPtr->MG.genxsReadDMARegs.genxsReadDMABufferPointer = 0x00000090;
            regAddrDbPtr->MG.genxsReadDMARegs.genxsReadDMAStartingAddr = 0x00000094;
            regAddrDbPtr->MG.genxsReadDMARegs.genxsReadDMACtrl = 0x00000098;

            regAddrDbPtr->MG.globalConfig.globalCtrl = 0x00000058;
            regAddrDbPtr->MG.globalConfig.extGlobalCtrl = 0x0000005c;
            /*  not exists in Lion2
                regAddrDbPtr->MG.globalConfig.extGlobalCtrl3;*/

            if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
            {
                /* manually added */
                regAddrDbPtr->MG.globalConfig.generalConfig = 0x0000001C;

                regAddrDbPtr->MG.globalInterrupt.functionalInterruptsSummaryCause = 0x000003F8;
                regAddrDbPtr->MG.globalInterrupt.functionalInterruptsSummaryMask = 0x000003FC;
                regAddrDbPtr->MG.globalInterrupt.functionalInterruptsSummary1Cause = 0x000003F4;
                regAddrDbPtr->MG.globalInterrupt.functionalInterruptsSummary1Mask = 0x000003F0;

                regAddrDbPtr->MG.globalInterrupt.ports1InterruptsSummaryCause = 0x00000150;
                regAddrDbPtr->MG.globalInterrupt.ports1InterruptsSummaryMask  = 0x00000154;

                regAddrDbPtr->MG.globalInterrupt.ports2InterruptsSummaryCause = 0x00000158;
                regAddrDbPtr->MG.globalInterrupt.ports2InterruptsSummaryMask  = 0x0000015c;

                regAddrDbPtr->MG.addrUpdateQueueConfig.GeneralControl = 0x000000D4;

                regAddrDbPtr->MG.addrUpdateQueueConfig.AUQHostConfiguration = 0x000000D8;
            }

            regAddrDbPtr->MG.globalInterrupt.globalInterruptCause = 0x00000030;
            regAddrDbPtr->MG.globalInterrupt.globalInterruptMask = 0x00000034;
            regAddrDbPtr->MG.globalInterrupt.miscellaneousInterruptCause = 0x00000038;
            regAddrDbPtr->MG.globalInterrupt.miscellaneousInterruptMask = 0x0000003c;
            regAddrDbPtr->MG.globalInterrupt.stackPortsInterruptCause = 0x00000080;
            regAddrDbPtr->MG.globalInterrupt.stackPortsInterruptMask = 0x00000084;
            regAddrDbPtr->MG.globalInterrupt.SERInterruptsSummary = 0x000000a4;
            regAddrDbPtr->MG.globalInterrupt.SERInterruptsMask = 0x000000a8;

            regAddrDbPtr->MG.MGRegsMGMiscellaneous.DFXInterruptsSummaryCause = 0x000000ac;
            regAddrDbPtr->MG.MGRegsMGMiscellaneous.DFXInterruptsSummaryMask = 0x000000b0;

            regAddrDbPtr->MG.interruptCoalescingConfig.interruptCoalescingConfig = 0x000000e0;
        }/*end of unit MGRegsMGMiscellaneous */


        {/*start of unit MGRegs */
            regAddrDbPtr->MG.MGMiscellaneous.GPPInputReg = 0x00000380;
            regAddrDbPtr->MG.MGMiscellaneous.GPPOutputReg = 0x00000384;
            regAddrDbPtr->MG.MGMiscellaneous.GPPIOCtrlReg = 0x00000388;

            regAddrDbPtr->MG.MGMiscellaneous.unitsInitDoneStatus0 = 0x00000600;
            regAddrDbPtr->MG.MGMiscellaneous.unitsInitDoneStatus1 = 0x00000604;
            /*  not exists in Lion2
                regAddrDbPtr->MG.MGMiscellaneous.initStage = 0x00000600;
            */
        }/*end of unit MGRegs */

        {
            GT_U32  offset;
            if(SMEM_CHT_IS_SIP5_10_GET(devObjPtr))
            {
                offset = 0x00030000;
            }
            else
            {
                offset = 0x00040000;
            }

            {/* start of unit MGRegs */
                regAddrDbPtr->MG.XSMI.XSMIManagement = offset + 0x0;
                regAddrDbPtr->MG.XSMI.XSMIAddr = offset + 0x8;
                regAddrDbPtr->MG.XSMI.XSMIConfig = offset + 0xc;
                regAddrDbPtr->MG.XSMI.XSMIInterruptCause = offset + 0x10;
                regAddrDbPtr->MG.XSMI.XSMIInterruptMask = offset + 0x14;
            }/* end of unit MGRegs */
        }

        regAddrDbPtr->MG.globalConfig.addrCompletion = 0x00000000;
        regAddrDbPtr->MG.globalConfig.sampledAtResetReg = 0x00000028;
        regAddrDbPtr->MG.globalConfig.lastReadTimeStamp = 0x00000040;
        regAddrDbPtr->MG.globalConfig.deviceID = 0x0000004c;
        regAddrDbPtr->MG.globalConfig.vendorID = 0x00000050;
        if(!SMEM_CHT_IS_SIP5_10_GET(devObjPtr))
        {
        regAddrDbPtr->MG.globalConfig.mediaInterface = 0x0000038c;
        }
        regAddrDbPtr->MG.globalConfig.CTGlobalConfig = 0x00000390;
    }/*end of unit MG - MG  */

    {/*start of unit SDMARegs */
        {/*0x260c+n*0X10*/
            GT_U32    n;
            for(n = 0 ; n <= 7 ; n++) {
                regAddrDbPtr->MG.SDMARegs.receiveSDMACurrentDescPointer[n] =
                    0x260c+n*0X10;
            }/* end of loop n */
        }/*0x260c+n*0X10*/


        regAddrDbPtr->MG.SDMARegs.receiveSDMAQueueCommand = 0x00002680;
        {/*0x26c0+n*0x4*/
            GT_U32    n;
            for(n = 0 ; n <= 7 ; n++) {
                regAddrDbPtr->MG.SDMARegs.transmitSDMACurrentDescPointer[n] =
                    0x26c0+n*0x4;
            }/* end of loop n */
        }/*0x26c0+n*0x4*/


        {/*0x2700+n*0x10*/
            GT_U32    n;
            for(n = 0 ; n <= 7 ; n++) {
                regAddrDbPtr->MG.SDMARegs.txSDMATokenBucketQueueCntr[n] =
                    0x2700+n*0x10;
            }/* end of loop n */
        }/*0x2700+n*0x10*/


        {/*0x2704+n*0x10*/
            GT_U32    n;
            for(n = 0 ; n <= 7 ; n++) {
                regAddrDbPtr->MG.SDMARegs.txSDMATokenBucketQueueConfig[n] =
                    0x2704+n*0x10;
            }/* end of loop n */
        }/*0x2704+n*0x10*/


        {/*0x2708+n*0x10*/
            GT_U32    n;
            for(n = 0 ; n <= 7 ; n++) {
                regAddrDbPtr->MG.SDMARegs.transmitSDMAWeightedRoundRobinQueueConfig[n] =
                    0x2708+n*0x10;
            }/* end of loop n */
        }/*0x2708+n*0x10*/


        regAddrDbPtr->MG.SDMARegs.txSDMATokenBucketCntr = 0x00002780;
        regAddrDbPtr->MG.SDMARegs.SDMAConfig = 0x00002800;
        regAddrDbPtr->MG.SDMARegs.receiveSDMAStatus = 0x0000281c;
        {/*0x2820+n*0x4*/
            GT_U32    n;
            for(n = 0 ; n <= 7 ; n++) {
                regAddrDbPtr->MG.SDMARegs.receiveSDMAPktCount[n] =
                    0x2820+n*0x4;
            }/* end of loop n */
        }/*0x2820+n*0x4*/


        {/*0x2840+n*0x4*/
            GT_U32    n;
            for(n = 0 ; n <= 7 ; n++) {
                regAddrDbPtr->MG.SDMARegs.receiveSDMAByteCount[n] =
                    0x2840+n*0x4;
            }/* end of loop n */
        }/*0x2840+n*0x4*/

        if( ! SMEM_CHT_IS_SIP5_GET(devObjPtr))/*replaced by receiveSDMAResourceErrorCountAndMode */
        {/*0x2860+n*4*/
            GT_U32    n;
            for(n = 0 ; n <= 1 ; n++) {
                regAddrDbPtr->MG.SDMARegs.receiveSDMAResourceErrorCount[n] =
                    0x2860+n*4;
            }/* end of loop n */
        }/*0x2860+n*4*/

        regAddrDbPtr->MG.SDMARegs.transmitSDMAQueueCommand = 0x00002868;
        regAddrDbPtr->MG.SDMARegs.transmitSDMAFixedPriorityConfig = 0x00002870;
        regAddrDbPtr->MG.SDMARegs.transmitSDMAWRRTokenParameters = 0x00002874;

        regAddrDbPtr->MG.SDMARegs.receiveSDMAInterruptCause = 0x0000280c;
        regAddrDbPtr->MG.SDMARegs.transmitSDMAInterruptCause = 0x00002810;
        regAddrDbPtr->MG.SDMARegs.receiveSDMAInterruptMask = 0x00002814;
        regAddrDbPtr->MG.SDMARegs.transmitSDMAInterruptMask = 0x00002818;

        if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            {/*0x28b0+n*4*/
                GT_U32    n;
                for(n = 0 ; n <= 7 ; n++) {
                    regAddrDbPtr->MG.SDMARegs.transmitSDMAPacketGeneratorConfigQueue[n] =
                        0x28b0+n*4;
                }/* end of loop n */
            }/*0x28b0+n*4*/

            {/*0x28d0+n*4*/
                GT_U32    n;
                for(n = 0 ; n <= 7 ; n++) {
                    regAddrDbPtr->MG.SDMARegs.transmitSDMAPacketCountConfigQueue[n] =
                        0x28d0+n*4;
                }/* end of loop n */
            }/*0x28d0+n*4*/

            {/*0x2860; 0x2878+n*4*/
                GT_U32    n;

                regAddrDbPtr->MG.SDMARegs.receiveSDMAResourceErrorCountAndMode[0] = 0x2860;
                regAddrDbPtr->MG.SDMARegs.receiveSDMAResourceErrorCountAndMode[1] = 0x2864;
                for(n = 2 ; n <= 7 ; n++) {
                    regAddrDbPtr->MG.SDMARegs.receiveSDMAResourceErrorCountAndMode[n] = 0x2878+(n-2)*4;
                }/* end of loop n */
            }/*0x2860; 0x2878+n*4*/
        }
    }/*end of unit SDMARegs */

    if(!SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {/*start of unit TXQ - TXQ  */
        {/*start of unit TXQ_DIST */
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_DIST.distDevMapTableAddrConstruct = 0x13000200;
            {/*0x13000210+0x4*n*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_DIST.distLocalSrcPortMapOwnDevEn[n] =
                        0x13000210+0x4*n;
                }/* end of loop n */
            }/*0x13000210+0x4*n*/


            {/*0x13000220+0x4*n*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_DIST.distLocalTrgPortMapOwnDevEn[n] =
                        0x13000220+0x4*n;
                }/* end of loop n */
            }/*0x13000220+0x4*n*/


            {/*0x13000230+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 63 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_DIST.distEntryDeviceMapTable[n] =
                        0x13000230+n*0x4;
                }/* end of loop n */
            }/*0x13000230+n*0x4*/



            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_DIST.distGlobalCtrl = 0x13000000;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_DIST.distUCMCArbiterCtrl = 0x13000004;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_DIST.distInterruptCause = 0x13000008;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_DIST.distInterruptMask = 0x1300000c;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_DIST.distBurstFifoThresholds = 0x13000010;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_DIST.distLossyDropEventCntr = 0x13000014;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_DIST.distPFCXoffEventCntr = 0x13000018;
            {/*0x13000020+n*4*/
                GT_U32    n;
                for(n = 0 ; n <= 15 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_DIST.distBurstFifoFillLevelStatus[n] =
                        0x13000020+n*4;
                }/* end of loop n */
            }/*0x13000020+n*4*/



        }/*end of unit TXQ_DIST */


        {/*start of unit TXQ_EGR[0] */
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_EGR[0].devMapTableAddrConstruct = 0x12001100;
            {/*0x12001110+0x4*n*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_EGR[0].localSrcPortMapOwnDevEn[n] =
                        0x12001110+0x4*n;
                }/* end of loop n */
            }/*0x12001110+0x4*n*/


            {/*0x12001120+0x4*n*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_EGR[0].localTrgPortMapOwnDevEn[n] =
                        0x12001120+0x4*n;
                }/* end of loop n */
            }/*0x12001120+0x4*n*/



            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_EGR[0].egrFiltersGlobalEnables = 0x12010000;
            {/*0x12010010+0x4*n*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_EGR[0].unknownUCFilterEn[n] =
                        0x12010010+0x4*n;
                }/* end of loop n */
            }/*0x12010010+0x4*n*/


            {/*0x12010020+0x4*n*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_EGR[0].unregedMCFilterEn[n] =
                        0x12010020+0x4*n;
                }/* end of loop n */
            }/*0x12010020+0x4*n*/


            {/*0x12010030+0x4*n*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_EGR[0].unregedBCFilterEn[n] =
                        0x12010030+0x4*n;
                }/* end of loop n */
            }/*0x12010030+0x4*n*/


            {/*0x12010040+0x4*n*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_EGR[0].MCLocalEn[n] =
                        0x12010040+0x4*n;
                }/* end of loop n */
            }/*0x12010040+0x4*n*/


            {/*0x12010050+0x4*n*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_EGR[0].IPMCRoutedFilterEn[n] =
                        0x12010050+0x4*n;
                }/* end of loop n */
            }/*0x12010050+0x4*n*/


            {/*0x12010060+0x4*n*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_EGR[0].UCSrcIDFilterEn[n] =
                        0x12010060+0x4*n;
                }/* end of loop n */
            }/*0x12010060+0x4*n*/


            {/*0x12010070+0x4*n*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_EGR[0].OAMLoopbackFilterEn[n] =
                        0x12010070+0x4*n;
                }/* end of loop n */
            }/*0x12010070+0x4*n*/


            {/*0x12010080+0x4*n*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_EGR[0].enCNFrameTxOnPort[n] =
                        0x12010080+0x4*n;
                }/* end of loop n */
            }/*0x12010080+0x4*n*/


            {/*0x12010090+0x4*n*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_EGR[0].enFCTriggerByCNFrameOnPort[n] =
                        0x12010090+0x4*n;
                }/* end of loop n */
            }/*0x12010090+0x4*n*/


            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_EGR[0].relayedPortNumber = 0x12010100;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_EGR[0].cpuPortDist = 0x12000000;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_EGR[0].egrInterruptsCause = 0x120000a0;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_EGR[0].egrInterruptsMask = 0x120000b0;

            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_EGR[0].MCFIFOGlobalConfigs = 0x12001000;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_EGR[0].MCFIFORelayedIFConfigs = 0x12001004;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_EGR[0].MCFIFOArbiterWeights = 0x12001008;
            {/*0x12001010+0x4*n*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_EGR[0].MCFIFO0DistMask[n] =
                        0x12001010+0x4*n;
                }/* end of loop n */
            }/*0x12001010+0x4*n*/


            {/*0x12001018+0x4*n*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_EGR[0].MCFIFO1DistMask[n] =
                        0x12001018+0x4*n;
                }/* end of loop n */
            }/*0x12001018+0x4*n*/


            {/*0x12001020+0x4*n*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_EGR[0].MCFIFO2DistMask[n] =
                        0x12001020+0x4*n;
                }/* end of loop n */
            }/*0x12001020+0x4*n*/


            {/*0x12001028+0x4*n*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_EGR[0].MCFIFO3DistMask[n] =
                        0x12001028+0x4*n;
                }/* end of loop n */
            }/*0x12001028+0x4*n*/


            {/*0x12001030+0x4*m*/
                GT_U32    m;
                for(m = 0 ; m <= 3 ; m++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_EGR[0].MCFIFOParityErrorsCntr[m] =
                        0x12001030+0x4*m;
                }/* end of loop m */
            }/*0x12001030+0x4*m*/



        }/*end of unit TXQ_EGR[0] */


        {/*start of unit TXQ_EGR[1] */
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_EGR[1].devMapTableAddrConstruct = 0x12801100;
            {/*0x12801110+0x4*n*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_EGR[1].localSrcPortMapOwnDevEn[n] =
                        0x12801110+0x4*n;
                }/* end of loop n */
            }/*0x12801110+0x4*n*/


            {/*0x12801120+0x4*n*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_EGR[1].localTrgPortMapOwnDevEn[n] =
                        0x12801120+0x4*n;
                }/* end of loop n */
            }/*0x12801120+0x4*n*/



            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_EGR[1].egrFiltersGlobalEnables = 0x12810000;
            {/*0x12810010+0x4*n*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_EGR[1].unknownUCFilterEn[n] =
                        0x12810010+0x4*n;
                }/* end of loop n */
            }/*0x12810010+0x4*n*/


            {/*0x12810020+0x4*n*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_EGR[1].unregedMCFilterEn[n] =
                        0x12810020+0x4*n;
                }/* end of loop n */
            }/*0x12810020+0x4*n*/


            {/*0x12810030+0x4*n*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_EGR[1].unregedBCFilterEn[n] =
                        0x12810030+0x4*n;
                }/* end of loop n */
            }/*0x12810030+0x4*n*/


            {/*0x12810040+0x4*n*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_EGR[1].MCLocalEn[n] =
                        0x12810040+0x4*n;
                }/* end of loop n */
            }/*0x12810040+0x4*n*/


            {/*0x12810050+0x4*n*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_EGR[1].IPMCRoutedFilterEn[n] =
                        0x12810050+0x4*n;
                }/* end of loop n */
            }/*0x12810050+0x4*n*/


            {/*0x12810060+0x4*n*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_EGR[1].UCSrcIDFilterEn[n] =
                        0x12810060+0x4*n;
                }/* end of loop n */
            }/*0x12810060+0x4*n*/


            {/*0x12810070+0x4*n*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_EGR[1].OAMLoopbackFilterEn[n] =
                        0x12810070+0x4*n;
                }/* end of loop n */
            }/*0x12810070+0x4*n*/


            {/*0x12810080+0x4*n*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_EGR[1].enCNFrameTxOnPort[n] =
                        0x12810080+0x4*n;
                }/* end of loop n */
            }/*0x12810080+0x4*n*/


            {/*0x12810090+0x4*n*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_EGR[1].enFCTriggerByCNFrameOnPort[n] =
                        0x12810090+0x4*n;
                }/* end of loop n */
            }/*0x12810090+0x4*n*/


            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_EGR[1].relayedPortNumber = 0x12810100;

            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_EGR[1].cpuPortDist = 0x12800000;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_EGR[1].egrInterruptsCause = 0x128000a0;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_EGR[1].egrInterruptsMask = 0x128000b0;

            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_EGR[1].MCFIFOGlobalConfigs = 0x12801000;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_EGR[1].MCFIFORelayedIFConfigs = 0x12801004;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_EGR[1].MCFIFOArbiterWeights = 0x12801008;
            {/*0x12801010+0x4*n*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_EGR[1].MCFIFO0DistMask[n] =
                        0x12801010+0x4*n;
                }/* end of loop n */
            }/*0x12801010+0x4*n*/


            {/*0x12801018+0x4*n*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_EGR[1].MCFIFO1DistMask[n] =
                        0x12801018+0x4*n;
                }/* end of loop n */
            }/*0x12801018+0x4*n*/


            {/*0x12801020+0x4*n*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_EGR[1].MCFIFO2DistMask[n] =
                        0x12801020+0x4*n;
                }/* end of loop n */
            }/*0x12801020+0x4*n*/


            {/*0x12801028+0x4*n*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_EGR[1].MCFIFO3DistMask[n] =
                        0x12801028+0x4*n;
                }/* end of loop n */
            }/*0x12801028+0x4*n*/


            {/*0x12801030+0x4*m*/
                GT_U32    m;
                for(m = 0 ; m <= 3 ; m++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_EGR[1].MCFIFOParityErrorsCntr[m] =
                        0x12801030+0x4*m;
                }/* end of loop m */
            }/*0x12801030+0x4*m*/



        }/*end of unit TXQ_EGR[1] */


        {/*start of unit TXQ_LL.global */
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_LL.global.LLInterruptCause = 0x108a0008;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_LL.global.LLInterruptMask = 0x108a000c;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_LL.global.portsLinkEnableStatus0 = 0x108a0100;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_LL.global.portsLinkEnableStatus1 = 0x108a0104;
        }/*end of unit TXQ_LL.global */


        {/*start of unit TXQ_LL */
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_LL.linkListECCCtrlConfig = 0x1088a000;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_LL.linkListRAMECCStatus = 0x1088a004;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_LL.linkListRAMECCErrorCntr = 0x1088a014;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_LL.FBUFRAMECCStatus = 0x1088a024;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_LL.FBUFRAMECCErrorCntr = 0x1088a034;
        }/*end of unit TXQ_LL */


        {/*start of unit TXQ_SHT */
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_SHT.global.SHTGlobalConfigs = 0x118f0000;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_SHT.global.VLANParityErrorLastEntries = 0x118f0010;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_SHT.global.egrVLANParityErrorCntr = 0x118f0020;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_SHT.global.ingrVLANParityErrorCntr = 0x118f0030;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_SHT.global.portIsolationParityErrorLastEntries = 0x118f0040;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_SHT.global.VIDXParityErrorLastEntry = 0x118f0044;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_SHT.global.L2PortIsolationParityErrorCntr = 0x118f0050;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_SHT.global.L3PortIsolationParityErrorCntr = 0x118f0060;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_SHT.global.VIDXParityErrorCntr = 0x118f0070;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_SHT.global.SHTInterruptsCause = 0x118f0120;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_SHT.global.SHTInterruptsMask = 0x118f0130;
        }/*end of unit TXQ_SHT */


        {/*start of unit TXQ_DQScheduler */
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_DQ.FlowControlResponse.flowCtrlResponseConfig = 0x11003000;
            {/*0x11003008+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 7 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_DQ.FlowControlResponse.schedulerProfileLLFCXOFFValue[n] =
                        0x11003008+n*0x4;
                }/* end of loop n */
            }/*0x11003008+n*0x4*/


            {/*0x11003040+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 7 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_DQ.FlowControlResponse.PFCTimerToPriorityQueueMap[n] =
                        0x11003040+n*0x4;
                }/* end of loop n */
            }/*0x11003040+n*0x4*/


            {/*0x11003060+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 7 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_DQ.FlowControlResponse.schedulerProfilePFCTimerToPriorityQueueMapEnable[n] =
                        0x11003060+n*0x4;
                }/* end of loop n */
            }/*0x11003060+n*0x4*/


            {/*0x11003080+0x20*t+0x4*p*/
                GT_U32    p,t;
                for(p = 0 ; p <= 7 ; p++) {
                    for(t = 0 ; t <= 7 ; t++) {
                        regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_DQ.FlowControlResponse.schedulerProfilePriorityQueuePFCFactor[p][t] =
                            0x11003080+0x20*t+0x4*p;
                    }/* end of loop t */
                }/* end of loop p */
            }/*0x11003080+0x20*t+0x4*p*/



        }/*end of unit FlowControlResponse */


        {/*start of unit TXQ_DQGlobal */
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_DQ.Scheduler.pizzaArbConfig = 0x11001400;
            {/*0x11001404+n*4*/
                GT_U32    n;
                for(n = 0 ; n <= 41 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_DQ.Scheduler.pizzaArbMap[n] =
                        0x11001404+n*4;
                }/* end of loop n */
            }/*0x11001404+n*4*/


            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_DQ.Scheduler.pizzaArbDebug = 0x11001500;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_DQ.Scheduler.pizzaArbDebugStatus = 0x11001504;

            {/*0x11001140+p*0x4*/
                GT_U32    p;
                for(p = 0 ; p <= 7 ; p++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_DQ.Scheduler.profileSDWRRWeightsConfigReg0[p] =
                        0x11001140+p*0x4;
                }/* end of loop p */
            }/*0x11001140+p*0x4*/


            {/*0x11001180+p*0x4*/
                GT_U32    p;
                for(p = 0 ; p <= 7 ; p++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_DQ.Scheduler.profileSDWRRWeightsConfigReg1[p] =
                        0x11001180+p*0x4;
                }/* end of loop p */
            }/*0x11001180+p*0x4*/


            {/*0x110011c0+p*0x10 and 0x110011c0+0xEC0+(p-4)*0x10*/
                GT_U32    p;
                for(p = 0 ; p <= 3 ; p++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_DQ.Scheduler.profileSDWRRGroup[p] =
                        0x110011c0+p*0x10;
                }/* end of loop p */
                for(p = 4 ; p <= 7 ; p++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_DQ.Scheduler.profileSDWRRGroup[(p - 4) + 4] =
                        0x110011c0+0xEC0+(p-4)*0x10;
                }/* end of loop p */
            }/*0x110011c0+p*0x10 and 0x110011c0+0xEC0+(p-4)*0x10*/


            {/*0x11001200+p*0x10 and 0x11001200+0xEC0+(p-4)*0x10*/
                GT_U32    p;
                for(p = 0 ; p <= 3 ; p++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_DQ.Scheduler.profileSDWRREnable[p] =
                        0x11001200+p*0x10;
                }/* end of loop p */
                for(p = 4 ; p <= 7 ; p++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_DQ.Scheduler.profileSDWRREnable[(p - 4) + 4] =
                        0x11001200+0xEC0+(p-4)*0x10;
                }/* end of loop p */
            }/*0x11001200+p*0x10 and 0x11001200+0xEC0+(p-4)*0x10*/


            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_DQ.Scheduler.portCPUSchedulerProfile = 0x11001240;
            {/*0x11001280+n * 0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 15 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_DQ.Scheduler.portSchedulerProfile[n] =
                        0x11001280+n * 0x4;
                }/* end of loop n */
            }/*0x11001280+n * 0x4*/



            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_DQ.Scheduler.schedulerConfig = 0x11001000;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_DQ.Scheduler.pktLengthForSchedulingAndTailDropDequeue = 0x11001008;

            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_DQ.Scheduler.tokenBucketUpdateRate = 0x11002000;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_DQ.Scheduler.tokenBucketBaseLine = 0x11002004;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_DQ.Scheduler.CPUTokenBucketMTU = 0x11002008;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_DQ.Scheduler.portsTokenBucketMTU = 0x1100200c;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_DQ.Scheduler.tokenBucketMode = 0x11002010;
            {/*0x11002014+n * 0x4*/
                GT_U32    n;
                GT_U32    max;

                max = (devObjPtr->portGroupSharedDevObjPtr == 0) ? numOfXlgMacPorts : 15;

                for(n = 0 ; n <= max ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_DQ.Scheduler.portDequeueEnable[n] =
                        0x11002014+n * 0x4;
                }/* end of loop n */
            }/*0x11002014+n * 0x4*/



            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_DQ.Scheduler.STCStatisticalTxSniffConfig = 0x11004000;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_DQ.Scheduler.egrCTMTrigger = 0x11004004;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_DQ.Scheduler.egrAnalyzerEnable = 0x11004010;

        }/*end of unit TXQ_DQGlobal */


        {/*start of unit TXQ_DQ */
            {/*0x11000090+n * 0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 15 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_DQ.portTxQFlushTrigger[n] =
                        0x11000090+n * 0x4;
                }/* end of loop n */
            }/*0x11000090+n * 0x4*/



            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_DQ.globalDequeueConfig = 0x11000000;

            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_DQ.txQDQInterruptSummaryCause = 0x11000100;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_DQ.txQDQInterruptSummaryMask = 0x11000104;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_DQ.flushDoneInterruptCause = 0x11000108;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_DQ.flushDoneInterruptMask = 0x1100010c;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_DQ.txQDQMemoryErrorInterruptCause = 0x11000110;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_DQ.txQDQMemoryErrorInterruptMask = 0x11000114;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_DQ.egrSTCInterruptCause = 0x11000118;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_DQ.egrSTCInterruptMask = 0x1100011c;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_DQ.txQDQGeneralInterruptCause = 0x11000120;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_DQ.txQDQGeneralInterruptMask = 0x11000124;

            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_DQ.tokenBucketPriorityParityErrorCntr = 0x11000150;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_DQ.stateVariablesParityErrorCntr = 0x11000154;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_DQ.parityErrorBadAddr = 0x11000158;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_DQ.dqMetalFixRegister = 0x11000180;

        }/*end of unit TXQ_DQ */


        {/*start of unit TXQ_QUEUEPeripheralAccess */
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.CongestionNotification.CNGlobalConfig = 0x100b0000;
            {/*0x100b0010+0x4*n*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.CongestionNotification.profileCNMTriggeringEnable[n] =
                        0x100b0010+0x4*n;
                }/* end of loop n */
            }/*0x100b0010+0x4*n*/


            {/*0x100b0020+0x4*n*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.CongestionNotification.enCNFrameTxOnPort[n] =
                        0x100b0020+0x4*n;
                }/* end of loop n */
            }/*0x100b0020+0x4*n*/


            {/*0x100b0030+0x4*n*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.CongestionNotification.enFCTriggerByCNFrameOnPort[n] =
                        0x100b0030+0x4*n;
                }/* end of loop n */
            }/*0x100b0030+0x4*n*/


            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.CongestionNotification.feedbackCalcConfigs = 0x100b0040;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.CongestionNotification.feedbackMIN = 0x100b0050;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.CongestionNotification.feedbackMAX = 0x100b0060;
            {/*0x100b0090+0x4*n*/
                GT_U32    n;
                for(n = 0 ; n <= 7 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.CongestionNotification.CNSampleTbl[n] =
                        0x100b0090+0x4*n;
                }/* end of loop n */
            }/*0x100b0090+0x4*n*/


            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.CongestionNotification.CNBufferFIFOOverrunsCntr = 0x100b00d0;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.CongestionNotification.CNBufferFIFOParityErrorsCntr = 0x100b00e0;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.CongestionNotification.CNDropCntr = 0x100b00f0;
            {/*0x100b0300+0x20*p + 0x4*t*/
                GT_U32    p,t;
                for(p = 0 ; p <= 7 ; p++) {
                    for(t = 0 ; t <= 7 ; t++) {
                        regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.CongestionNotification.profilePriorityQueueCNThreshold[p][t] =
                            0x100b0300+0x20*p + 0x4*t;
                    }/* end of loop t */
                }/* end of loop p */
            }/*0x100b0300+0x20*p + 0x4*t*/


            {/*0x100b0500+0x20*p + 0x4*t*/
                GT_U32    p,t;
                for(p = 0 ; p <= 7 ; p++) {
                    for(t = 0 ; t <= 7 ; t++) {
                        regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.CongestionNotification.profilePriorityQueueCNSevereThreshold[p][t] =
                            0x100b0500+0x20*p + 0x4*t;
                    }/* end of loop t */
                }/* end of loop p */
            }/*0x100b0500+0x20*p + 0x4*t*/


            {/*0x100b0700+0x20*p + 0x4*t*/
                GT_U32    p,t;
                for(p = 0 ; p <= 7 ; p++) {
                    for(t = 0 ; t <= 7 ; t++) {
                        regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.CongestionNotification.profilePriorityQueueCNMildThreshold[p][t] =
                            0x100b0700+0x20*p + 0x4*t;
                    }/* end of loop t */
                }/* end of loop p */
            }/*0x100b0700+0x20*p + 0x4*t*/


            {/*0x100b0900+0x20*p + 0x4*t*/
                GT_U32    p,t;
                for(p = 0 ; p <= 7 ; p++) {
                    for(t = 0 ; t <= 7 ; t++) {
                        regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.CongestionNotification.profilePriorityQueueCNSampleScaleThreshold[p][t] =
                            0x100b0900+0x20*p + 0x4*t;
                    }/* end of loop t */
                }/* end of loop p */
            }/*0x100b0900+0x20*p + 0x4*t*/



            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.CongestionNotification.distributorGeneralConfigs = 0x10091000;
            {/*0x10091020+tc*0x4*/
                GT_U32    tc;
                for(tc = 0 ; tc <= 7 ; tc++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.CongestionNotification.stackTCRemap[tc] =
                        0x10091020+tc*0x4;
                }/* end of loop tc */
            }/*0x10091020+tc*0x4*/


            {/*0x10091050+0x4*n*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.CongestionNotification.stackRemapEn[n] =
                        0x10091050+0x4*n;
                }/* end of loop n */
            }/*0x10091050+0x4*n*/


            {/*0x10091080+0x4*n*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.CongestionNotification.DPToCFIRemapEn[n] =
                        0x10091080+0x4*n;
                }/* end of loop n */
            }/*0x10091080+0x4*n*/


            {/*0x10091090+0x4*n*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.CongestionNotification.fromCPUForwardRestricted[n] =
                        0x10091090+0x4*n;
                }/* end of loop n */
            }/*0x10091090+0x4*n*/


            {/*0x100910a0+0x4*n*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.CongestionNotification.bridgedForwardRestricted[n] =
                        0x100910a0+0x4*n;
                }/* end of loop n */
            }/*0x100910a0+0x4*n*/


            {/*0x100910b0+0x4*n*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.CongestionNotification.routedForwardRestricted[n] =
                        0x100910b0+0x4*n;
                }/* end of loop n */
            }/*0x100910b0+0x4*n*/



        }/*end of unit CongestionNotification */


        {/*start of unit TXQ_QUEUEGlobal */
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.PeripheralAccess.CNCModesReg = 0x100936a0;

            {/*0x10093200+0x4*n*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.PeripheralAccess.txQMIBCntrsSetConfig[n] =
                        0x10093200+0x4*n;
                }/* end of loop n */
            }/*0x10093200+0x4*n*/


            {/*0x10093210+0x4*n*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.PeripheralAccess.setOutgoingUcPktCntr[n] =
                        0x10093210+0x4*n;
                }/* end of loop n */
            }/*0x10093210+0x4*n*/


            {/*0x10093220+0x4*n*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.PeripheralAccess.setOutgoingMcPktCntr[n] =
                        0x10093220+0x4*n;
                }/* end of loop n */
            }/*0x10093220+0x4*n*/


            {/*0x10093230+0x4*n*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.PeripheralAccess.setOutgoingBcPktCntr[n] =
                        0x10093230+0x4*n;
                }/* end of loop n */
            }/*0x10093230+0x4*n*/


            {/*0x10093240+0x4*n*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.PeripheralAccess.setBridgeEgrFilteredPktCntr[n] =
                        0x10093240+0x4*n;
                }/* end of loop n */
            }/*0x10093240+0x4*n*/


            {/*0x10093250+0x4*n*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.PeripheralAccess.setTailDroppedPktCntr[n] =
                        0x10093250+0x4*n;
                }/* end of loop n */
            }/*0x10093250+0x4*n*/


            {/*0x10093260+0x4*n*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.PeripheralAccess.setCtrlPktCntr[n] =
                        0x10093260+0x4*n;
                }/* end of loop n */
            }/*0x10093260+0x4*n*/


            {/*0x10093270+0x4*n*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.PeripheralAccess.setEgrForwardingRestrictionDroppedPktsCntr[n] =
                        0x10093270+0x4*n;
                }/* end of loop n */
            }/*0x10093270+0x4*n*/


            {/*0x10093280+0x4*n*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.PeripheralAccess.setMcFIFO3_0DroppedPktsCntr[n] =
                        0x10093280+0x4*n;
                }/* end of loop n */
            }/*0x10093280+0x4*n*/


            {/*0x10093290+0x4*n*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.PeripheralAccess.setMcFIFO7_4DroppedPktsCntr[n] =
                        0x10093290+0x4*n;
                }/* end of loop n */
            }/*0x10093290+0x4*n*/



            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.PeripheralAccess.peripheralAccessMisc = 0x10093000;
            {/*0x10093004+0x4*n*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.PeripheralAccess.portGroupDescToEQCntr[n] =
                        0x10093004+0x4*n;
                }/* end of loop n */
            }/*0x10093004+0x4*n*/



            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.PeripheralAccess.PFCTriggerGlobalConfig = 0x100c0000;
            {/*0x100c0010+0x4*n*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.PeripheralAccess.portFCMode[n] =
                        0x100c0010+0x4*n;
                }/* end of loop n */
            }/*0x100c0010+0x4*n*/


            {/*0x100c0030+0x4*p*/
                GT_U32    p;
                for(p = 0 ; p <= 7 ; p++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.PeripheralAccess.PFCPortGroupCntrsStatus[p] =
                        0x100c0030+0x4*p;
                }/* end of loop p */
            }/*0x100c0030+0x4*p*/


            {/*0x100c0050+0x4*p*/
                GT_U32    p;
                for(p = 0 ; p <= 7 ; p++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.PeripheralAccess.PFCPortGroupFlowCtrlIndicationFIFOOverrunEntry0[p] =
                        0x100c0050+0x4*p;
                }/* end of loop p */
            }/*0x100c0050+0x4*p*/


            {/*0x100c0070+0x4*p*/
                GT_U32    p;
                for(p = 0 ; p <= 7 ; p++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.PeripheralAccess.PFCPortGroupFlowCtrlIndicationFIFOOverrunEntry1[p] =
                        0x100c0070+0x4*p;
                }/* end of loop p */
            }/*0x100c0070+0x4*p*/


            {/*0x100c0090+0x4*p*/
                GT_U32    p;
                for(p = 0 ; p <= 7 ; p++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.PeripheralAccess.PFCPortGroupFlowCtrlIndicationFIFOOverrunCntr[p] =
                        0x100c0090+0x4*p;
                }/* end of loop p */
            }/*0x100c0090+0x4*p*/


            {/*0x100c00b0+0x4*p*/
                GT_U32    p;
                for(p = 0 ; p <= 7 ; p++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.PeripheralAccess.PFCPortGroupEqFIFOOverrunCntr[p] =
                        0x100c00b0+0x4*p;
                }/* end of loop p */
            }/*0x100c00b0+0x4*p*/


            {/*0x100c00d0+0x4*p*/
                GT_U32    p;
                for(p = 0 ; p <= 7 ; p++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.PeripheralAccess.PFCPortGroupCntrsParityErrorsCntr[p] =
                        0x100c00d0+0x4*p;
                }/* end of loop p */
            }/*0x100c00d0+0x4*p*/


            {/*0x100c00f0+0x4*r*/
                GT_U32    r;
                for(r = 0 ; r <= 15 ; r++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.PeripheralAccess.PFCPortProfile[r] =
                        0x100c00f0+0x4*r;
                }/* end of loop r */
            }/*0x100c00f0+0x4*r*/


            {/*0x100c0140+0x4*n*/
                GT_U32    n;
                for(n = 0 ; n <= 15 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.PeripheralAccess.LLFCTargetPortRemap[n] =
                        0x100c0140+0x4*n;
                }/* end of loop n */
            }/*0x100c0140+0x4*n*/


            {/*0x100c0200+0x4*n*/
                GT_U32    n;
                for(n = 0 ; n <= 31 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.PeripheralAccess.PFCLLFCSourcePortRemapForLLFCCntrs[n] =
                        0x100c0200+0x4*n;
                }/* end of loop n */
            }/*0x100c0200+0x4*n*/


            {/*0x100c0500+0x4*n*/
                GT_U32    n;
                for(n = 0 ; n <= 31 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.PeripheralAccess.PFCLLFCSourcePortRemapForLLFCMsgs[n] =
                        0x100c0500+0x4*n;
                }/* end of loop n */
            }/*0x100c0500+0x4*n*/


            {/*0x100c0900+0x20*p + 0x4*t*/
                GT_U32    p,t;
                for(p = 0 ; p <= 7 ; p++) {
                    for(t = 0 ; t <= 7 ; t++) {
                        regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.PeripheralAccess.PFCGroupOfPortsTCCntr[p][t] =
                            0x100c0900+0x20*p + 0x4*t;
                    }/* end of loop t */
                }/* end of loop p */
            }/*0x100c0900+0x20*p + 0x4*t*/


            {/*0x100c1000+0x20*p + 0x4*t*/
                GT_U32    t,p;
                for(t = 0 ; t <= 7 ; t++) {
                    for(p = 0 ; p <= 7 ; p++) {
                        regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.PeripheralAccess.PFCGroupOfPortsTCThresholds[t][p] =
                            0x100c1000+0x20*p + 0x4*t;
                    }/* end of loop p */
                }/* end of loop t */
            }/*0x100c1000+0x20*p + 0x4*t*/



        }/*end of unit TXQ_QUEUEGlobal */


        {/*start of unit PFC */
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.PFC.PFCInterruptSummaryCause = 0x10090080;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.PFC.PFCInterruptSummaryMask = 0x10090084;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.PFC.PFCParityInterruptCause = 0x10090090;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.PFC.PFCParityInterruptMask = 0x10090094;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.PFC.PFCCntrsOverUnderFlowInterruptCause = 0x100900a0;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.PFC.PFCCntrsOverUnderFlowInterruptMask = 0x100900a4;
            {/*0x100900d0+0x4*p*/
                GT_U32    p;
                for(p = 0 ; p <= 7 ; p++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.PFC.PFCPortGroupGlobalOverflowInterruptCause[p] =
                        0x100900b0+0x4*p;
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.PFC.PFCPortGroupGlobalOverflowInterruptMask[p] =
                        0x100900d0+0x4*p;
                }/* end of loop p */
            }/*0x100900d0+0x4*p*/


            {/*0x10090110+0x4*p*/
                GT_U32    p;
                for(p = 0 ; p <= 7 ; p++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.PFC.PFCPortGroupGlobalUnderflowInterruptCause[p] =
                        0x100900f0+0x4*p;
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.PFC.PFCPortGroupGlobalUnderflowInterruptMask[p] =
                        0x10090110+0x4*p;
                }/* end of loop p */
            }/*0x10090110+0x4*p*/


            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.PFC.PFCFIFOsOverrunsCause = 0x10090130;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.PFC.PFCFIFOsOverrunsMask = 0x10090134;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.PFC.PFCMsgsSetsOverrunsEgr3_0Cause0 = 0x10090138;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.PFC.PFCMsgsSetsOverrunsEgr3_0Mask0 = 0x1009013c;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.PFC.PFCMsgsSetsOverrunsEgr3_0Cause1 = 0x10090140;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.PFC.PFCMsgsSetsOverrunsEgr3_0Mask1 = 0x10090144;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.PFC.PFCMsgsSetsOverrunsEgr7_4Cause0 = 0x10090150;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.PFC.PFCMsgsSetsOverrunsEgr7_4Mask0 = 0x10090154;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.PFC.PFCMsgsSetsOverrunsEgr7_4Cause1 = 0x10090158;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.PFC.PFCMsgsSetsOverrunsEgr7_4Mask1 = 0x1009015c;

        }/*end of unit PFC */


        {/*start of unit tailDrop */
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.tailDrop.snifferDescsCntr = 0x100a6000;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.tailDrop.snifferBuffersCntr = 0x100a6004;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.tailDrop.mcDescsCntr = 0x100a6008;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.tailDrop.mcBuffersCntr = 0x100a600c;

            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.tailDrop.mirroredPktsToAnalyzerPortDescsLimit = 0x100a3300;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.tailDrop.mirroredPktsToAnalyzerPortBuffersLimit = 0x100a3310;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.tailDrop.mcDescsLimit = 0x100a3320;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.tailDrop.mcBuffersLimit = 0x100a3330;

        }/*end of unit tailDrop */


        {/*start of unit TXQ_QUEUE */
            {/*0x100a6310+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.ResourceHistogram.resourceHistogramCntr[n] =
                        0x100a6310+n*0x4;
                }/* end of loop n */
            }/*0x100a6310+n*0x4*/



            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.ResourceHistogram.resourceHistogramLimitReg1 = 0x100a6300;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.ResourceHistogram.resourceHistogramLimitReg2 = 0x100a6304;

            {/*0x100a0240+0x4*d*/
                GT_U32    d;
                for(d = 0 ; d <= 3 ; d++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.profileEnableWRTDDP[d] =
                        0x100a0240+0x4*d;
                }/* end of loop d */
            }/*0x100a0240+0x4*d*/


            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.tailDropConfig = 0x100a0000;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.pktLengthForTailDropEnqueue = 0x100a0008;
            {/*0x100a0010+n * 0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 63 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.portEnqueueEnable[n] =
                        0x100a0010+n * 0x4;
                }/* end of loop n */
            }/*0x100a0010+n * 0x4*/


            {/*0x100a0120+n * 0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 63 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.portTailDropCNProfile[n] =
                        0x100a0120+n * 0x4;
                }/* end of loop n */
            }/*0x100a0120+n * 0x4*/


            {/*0x100a0220+t*0x4*/
                GT_U32    t;
                for(t = 0 ; t <= 7 ; t++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.profilePriorityQueueEnablePoolUsage[t] =
                        0x100a0220+t*0x4;
                }/* end of loop t */
            }/*0x100a0220+t*0x4*/


            {/*0x100a0250+0x4*p*/
                GT_U32    p;
                for(p = 0 ; p <= 7 ; p++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.profilePriorityQueueToPoolAssociation[p] =
                        0x100a0250+0x4*p;
                }/* end of loop p */
            }/*0x100a0250+0x4*p*/


            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.WRTDMasks0 = 0x100a0270;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.WRTDMasks1 = 0x100a0274;

            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.totalDescCntr = 0x100a1000;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.totalBuffersCntr = 0x100a1004;
            {/*0x100a1010+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 63 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.portDescCntr[n] =
                        0x100a1010+n*0x4;
                }/* end of loop n */
            }/*0x100a1010+n*0x4*/


            {/*0x100a1110+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 63 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.portBuffersCntr[n] =
                        0x100a1110+n*0x4;
                }/* end of loop n */
            }/*0x100a1110+n*0x4*/


            {/*0x100a1210+t*0x4*/
                GT_U32    t;
                for(t = 0 ; t <= 7 ; t++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.priorityDescCntr[t] =
                        0x100a1210+t*0x4;
                }/* end of loop t */
            }/*0x100a1210+t*0x4*/


            {/*0x100a1250+t*0x4*/
                GT_U32    t;
                for(t = 0 ; t <= 7 ; t++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.priorityBuffersCntr[t] =
                        0x100a1250+t*0x4;
                }/* end of loop t */
            }/*0x100a1250+t*0x4*/



            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.globalDescsLimit = 0x100a0300;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.globalBufferLimit = 0x100a0310;
            {/*0x100a0320+0x4*n*/
                GT_U32    n;
                for(n = 0 ; n <= 7 ; n++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.profilePortLimits[n] =
                        0x100a0320+0x4*n;
                }/* end of loop n */
            }/*0x100a0320+0x4*n*/


            {/*0x100a0400+t*0x4*/
                GT_U32    t;
                for(t = 0 ; t <= 7 ; t++) {
                    regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.poolLimits[t] =
                        0x100a0400+t*0x4;
                }/* end of loop t */
            }/*0x100a0400+t*0x4*/



            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.ucMcCtrl = 0x10009800;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.ucWeights = 0x10009804;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.mcWeights = 0x10009808;
            regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.mcShaperCtrl = 0x1000980c;

        }/*end of unit TXQ_Queue */


        regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.global.transmitQueueInterruptSummaryCause = 0x10090000;
        regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.global.transmitQueueInterruptSummaryMask = 0x10090004;
        regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.global.generalInterruptCause = 0x10090008;
        regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.global.generalInterruptMask = 0x1009000c;
        regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.global.portDescFullInterruptCause0 = 0x10090040;
        regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.global.portDescFullInterruptMask0 = 0x10090044;
        regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.global.portDescFullInterruptCause1 = 0x10090048;
        regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.global.portDescFullInterruptMask1 = 0x1009004c;
        regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.global.portDescFullInterruptCause2 = 0x10090050;
        regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.global.portDescFullInterruptMask2 = 0x10090054;
        regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.global.portBufferFullInterruptCause0 = 0x10090060;
        regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.global.portBufferFullInterruptMask0 = 0x10090064;
        regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.global.portBufferFullInterruptCause1 = 0x10090068;
        regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.global.portBufferFullInterruptMask1 = 0x1009006c;
        regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.global.portBufferFullInterruptCause2 = 0x10090070;
        regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.global.portBufferFullInterruptMask2 = 0x10090074;
        regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.global.congestionNotificationInterruptCause = 0x10090180;
        regAddrDbPtr->egrAndTxqVer1.TXQ.TXQ_Queue.global.congestionNotificationInterruptMask = 0x10090184;
    }/*end of unit TXQ - TXQ  */

    if(! SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {/*start of unit TTI - TTI  */
        {/*start of unit ITS_IP */
            regAddrDbPtr->TTI.ITS.globalConfigs = 0x01002000;
            regAddrDbPtr->TTI.ITS.enableTimestamping = 0x01002004;
            {/*0x1002008+4*i*/
                GT_U32    i;
                for(i = 0 ; i <= 1 ; i++) {
                    regAddrDbPtr->TTI.ITS.timestampingPortEnable[i] =
                        0x1002008+4*i;
                }/* end of loop i */
            }/*0x1002008+4*i*/


            regAddrDbPtr->TTI.ITS.TODCntrNanoseconds = 0x01002010;
            regAddrDbPtr->TTI.ITS.TODCntrSeconds[0] = 0x01002014;
            regAddrDbPtr->TTI.ITS.TODCntrSeconds[1] = 0x01002018;
            regAddrDbPtr->TTI.ITS.TODCntrShadowNanoseconds = 0x0100201c;
            regAddrDbPtr->TTI.ITS.TODCntrShadowSeconds[0] = 0x01002020;
            regAddrDbPtr->TTI.ITS.TODCntrShadowSeconds[1] = 0x01002024;
            regAddrDbPtr->TTI.ITS.GTSInterruptCause = 0x01002028;
            regAddrDbPtr->TTI.ITS.GTSInterruptMask = 0x0100202c;
            regAddrDbPtr->TTI.ITS.globalFIFOCurrentEntry[0] = 0x01002030;
            regAddrDbPtr->TTI.ITS.globalFIFOCurrentEntry[1] = 0x01002034;
            regAddrDbPtr->TTI.ITS.overrunStatus = 0x01002038;
            regAddrDbPtr->TTI.ITS.underrunStatus = 0x0100203c;

        }/*end of unit ITS_IP */


        {/*start of unit logFlow */
            regAddrDbPtr->TTI.logFlow.bridgeLogFlowMACDALow = 0x01000100;
            regAddrDbPtr->TTI.logFlow.bridgeLogFlowMACDAHigh = 0x01000104;
            regAddrDbPtr->TTI.logFlow.bridgeLogFlowMACDALowMask = 0x01000108;
            regAddrDbPtr->TTI.logFlow.bridgeLogFlowMACDAHighMask = 0x0100010c;
            regAddrDbPtr->TTI.logFlow.bridgeLogFlowMACSALow = 0x01000110;
            regAddrDbPtr->TTI.logFlow.bridgeLogFlowMACSAHigh = 0x01000114;
            regAddrDbPtr->TTI.logFlow.bridgeLogFlowMACSALowMask = 0x01000118;
            regAddrDbPtr->TTI.logFlow.bridgeLogFlowMACSAHighMask = 0x0100011c;
            regAddrDbPtr->TTI.logFlow.bridgeLogFlowEtherType = 0x01000120;
            regAddrDbPtr->TTI.logFlow.routerLogFlowDIP[0] = 0x01000140;
            regAddrDbPtr->TTI.logFlow.routerLogFlowDIP[1] = 0x01000144;
            regAddrDbPtr->TTI.logFlow.routerLogFlowDIP[2] = 0x01000148;
            regAddrDbPtr->TTI.logFlow.routerLogFlowDIP[3] = 0x0100014c;
            regAddrDbPtr->TTI.logFlow.routerLogFlowDIPMask[0] = 0x01000150;
            regAddrDbPtr->TTI.logFlow.routerLogFlowDIPMask[1] = 0x01000154;
            regAddrDbPtr->TTI.logFlow.routerLogFlowDIPMask[2] = 0x01000158;
            regAddrDbPtr->TTI.logFlow.routerLogFlowDIPMask[3] = 0x0100015c;
            regAddrDbPtr->TTI.logFlow.routerLogFlowSIP[0] = 0x01000160;
            regAddrDbPtr->TTI.logFlow.routerLogFlowSIP[1] = 0x01000164;
            regAddrDbPtr->TTI.logFlow.routerLogFlowSIP[2] = 0x01000168;
            regAddrDbPtr->TTI.logFlow.routerLogFlowSIP[3] = 0x0100016c;
            regAddrDbPtr->TTI.logFlow.routerLogFlowSIPMask[0] = 0x01000170;
            regAddrDbPtr->TTI.logFlow.routerLogFlowSIPMask[1] = 0x01000174;
            regAddrDbPtr->TTI.logFlow.routerLogFlowSIPMask[2] = 0x01000178;
            regAddrDbPtr->TTI.logFlow.routerLogFlowSIPMask[3] = 0x0100017c;
            regAddrDbPtr->TTI.logFlow.routerLogFlowIPProtocol = 0x01000180;
            regAddrDbPtr->TTI.logFlow.routerLogFlowSourceTCPUDPPort = 0x01000184;
            regAddrDbPtr->TTI.logFlow.routerLogFlowDestinationTCPUDPPort = 0x01000188;

        }/*end of unit logFlow */


        {/*start of unit MAC2ME */
            {/*0x1001600+n*0x10*/
                GT_U32    n;
                for(n = 0 ; n <= 7 ; n++) {
                    regAddrDbPtr->TTI.MAC2ME.MAC2MELow[n] =
                        0x1001600+n*0x10;
                }/* end of loop n */
            }/*0x1001600+n*0x10*/


            {/*0x1001604+n*0x10*/
                GT_U32    n;
                for(n = 0 ; n <= 7 ; n++) {
                    regAddrDbPtr->TTI.MAC2ME.MAC2MEHigh[n] =
                        0x1001604+n*0x10;
                }/* end of loop n */
            }/*0x1001604+n*0x10*/


            {/*0x1001608+n*0x10*/
                GT_U32    n;
                for(n = 0 ; n <= 7 ; n++) {
                    regAddrDbPtr->TTI.MAC2ME.MAC2MELowMask[n] =
                        0x1001608+n*0x10;
                }/* end of loop n */
            }/*0x1001608+n*0x10*/


            {/*0x100160c+n*0x10*/
                GT_U32    n;
                for(n = 0 ; n <= 7 ; n++) {
                    regAddrDbPtr->TTI.MAC2ME.MAC2MEHighMask[n] =
                        0x100160c+n*0x10;
                }/* end of loop n */
            }/*0x100160c+n*0x10*/

        }/*end of unit MAC2ME */

        {/*start of unit protocolMatch */
            {/*0x1000080+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 5 ; n++) {
                    regAddrDbPtr->TTI.protocolMatch.protocolsConfig[n] =
                        0x1000080+n*0x4;
                }/* end of loop n */
            }/*0x1000080+n*0x4*/

            regAddrDbPtr->TTI.protocolMatch.protocolsEncapsulationConfig0 = 0x01000098;
            regAddrDbPtr->TTI.protocolMatch.protocolsEncapsulationConfig1 = 0x0100009c;
        }/*end of unit protocolMatch */

        {/*start of unit TTI_IP */
            {/*0x1000500+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->TTI.qoSMapTables.UPToQoSParametersMapTableReg[n] =
                        0x1000500+n*0x4;
                }/* end of loop n */
            }/*0x1000500+n*0x4*/


            {/*0x1000510+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->TTI.qoSMapTables.EXPToQoSParametersMapTableReg[n] =
                        0x1000510+n*0x4;
                }/* end of loop n */
            }/*0x1000510+n*0x4*/


            regAddrDbPtr->TTI.qoSMapTables.UP2UPMapTable = 0x01000520;

            {/*0x1000400+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 15 ; n++) {
                    regAddrDbPtr->TTI.qoSMapTables.DSCP_ToQoSProfileMap[n] =
                        0x1000400+n*0x4;
                }/* end of loop n */
            }/*0x1000400+n*0x4*/


            {/*0x1000440+n*0x4+m*0x8+l*0x10*/
                GT_U32    n,m,l;
                for(n = 0 ; n <= 1 ; n++) {
                    for(m = 0 ; m <= 1 ; m++) {
                        for(l = 0 ; l <= 1 ; l++) {
                            regAddrDbPtr->TTI.qoSMapTables.CFIUP_ToQoSProfileMapTable[n][m][l] =
                                0x1000440+n*0x4+m*0x8+l*0x10;
                        }/* end of loop l */
                    }/* end of loop m */
                }/* end of loop n */
            }/*0x1000440+n*0x4+m*0x8+l*0x10*/


            {/*0x1000460+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->TTI.qoSMapTables.EXPToQoSProfileMap[n] =
                        0x1000460+n*0x4;
                }/* end of loop n */
            }/*0x1000460+n*0x4*/


            {/*0x1000470+4*p*/
                GT_U32    p;
                for(p = 0 ; p <= 15 ; p++) {
                    regAddrDbPtr->TTI.qoSMapTables.CFI_UPToQoSProfileMapTableSelector[p] =
                        0x1000470+4*p;
                }/* end of loop p */
            }/*0x1000470+4*p*/


            {/*start of unit trunkHash */
                regAddrDbPtr->TTI.trunkHash.trunkHashConfigReg0 = 0x01000070;
                regAddrDbPtr->TTI.trunkHash.trunkHashConfigReg1 = 0x01000074;
                regAddrDbPtr->TTI.trunkHash.trunkHashConfigReg2 = 0x01000078;
            }/*end of unit trunkHash */

            {/*start of unit userDefinedBytes */
                {/*0x10000a0+n*0x4*/
                    GT_U32    n;
                    for(n = 0 ; n <= 7 ; n++) {
                        regAddrDbPtr->TTI.userDefinedBytes.IPCLTCPPortRangeComparatorConfig[n] =
                            0x10000a0+n*0x4;
                    }/* end of loop n */
                }/*0x10000a0+n*0x4*/


                {/*0x10000c0+n*0x4*/
                    GT_U32    n;
                    for(n = 0 ; n <= 7 ; n++) {
                        regAddrDbPtr->TTI.userDefinedBytes.IPCLUDPPortRangeComparatorConfig[n] =
                            0x10000c0+n*0x4;
                    }/* end of loop n */
                }/*0x10000c0+n*0x4*/

            }/*end of unit userDefinedBytes */

            {/*start of unit VLANAssignment */
                {/*0x1000300+i*4*/
                    GT_U32    i;
                    for(i = 0 ; i <= 3 ; i++) {
                        regAddrDbPtr->TTI.VLANAssignment.ingrTPIDConfig[i] =
                            0x1000300+i*4;
                    }/* end of loop i */
                }/*0x1000300+i*4*/


                {/*0x1000310+n*4*/
                    GT_U32    n;
                    for(n = 0 ; n <= 31 ; n++) {
                        regAddrDbPtr->TTI.VLANAssignment.ingrTPIDSelect[n] =
                            0x1000310+n*4;
                    }/* end of loop n */
                }/*0x1000310+n*4*/

            }/*end of unit VLANAssignment */

        }/*end of unit TTI_IP */


        regAddrDbPtr->TTI.TTIUnitGlobalConfig = 0x01000000;
        regAddrDbPtr->TTI.TTIEngineInterruptCause = 0x01000004;
        regAddrDbPtr->TTI.TTIEngineInterruptMask = 0x01000008;
        regAddrDbPtr->TTI.TTIEngineConfig = 0x0100000c;
        regAddrDbPtr->TTI.TTIPCLIDConfig0 = 0x01000010;
        regAddrDbPtr->TTI.TTIPCLIDConfig1 = 0x01000014;
        regAddrDbPtr->TTI.TTIIPv4GREEthertype = 0x01000018;
        regAddrDbPtr->TTI.IPMinimumOffset = 0x01000020;
        regAddrDbPtr->TTI.CNAndFCConfig = 0x01000024;
        regAddrDbPtr->TTI.specialEtherTypes = 0x01000028;
        regAddrDbPtr->TTI.timeStampTagConfiguration = 0x0100002c;   /* Added manually */
        regAddrDbPtr->TTI.MPLSEtherTypes = 0x01000030;
        regAddrDbPtr->TTI.IPv6ExtensionValue = 0x01000038;
        regAddrDbPtr->TTI.IPv6SolicitedNodeMcAddrConfigReg[0] = 0x01000040;
        regAddrDbPtr->TTI.IPv6SolicitedNodeMcAddrConfigReg[1] = 0x01000044;
        regAddrDbPtr->TTI.IPv6SolicitedNodeMcAddrConfigReg[2] = 0x01000048;
        regAddrDbPtr->TTI.IPv6SolicitedNodeMcAddrConfigReg[3] = 0x0100004c;
        regAddrDbPtr->TTI.IPv6SolicitedNodeMcAddrMaskReg[0] = 0x01000050;
        regAddrDbPtr->TTI.IPv6SolicitedNodeMcAddrMaskReg[1] = 0x01000054;
        regAddrDbPtr->TTI.IPv6SolicitedNodeMcAddrMaskReg[2] = 0x01000058;
        regAddrDbPtr->TTI.IPv6SolicitedNodeMcAddrMaskReg[3] = 0x0100005c;
        regAddrDbPtr->TTI.DSAConfig = 0x01000060;
        {/*0x1000064+n*4*/
            GT_U32    n;
            for(n = 0 ; n <= 1 ; n++) {
                regAddrDbPtr->TTI.loopPort[n] =
                    0x1000064+n*4;
            }/* end of loop n */
        }/*0x1000064+n*4*/


        {/*0x10000f0+4*i*/
            GT_U32    i;
            for(i = 0 ; i <= 1 ; i++) {
                regAddrDbPtr->TTI.userDefineEtherTypes[i] =
                    0x10000f0+4*i;
            }/* end of loop i */
        }/*0x10000f0+4*i*/


        {/*0x10000f8+4*i*/
            GT_U32    i;
            for(i = 0 ; i <= 1 ; i++) {
                regAddrDbPtr->TTI.coreRingPortEnable[i] =
                    0x10000f8+4*i;
            }/* end of loop i */
        }/*0x10000f8+4*i*/


        {/*0x1000190+4*n*/
            GT_U32    n;
            for(n = 0 ; n <= 1 ; n++) {
                regAddrDbPtr->TTI.ECMCPID[n] =
                    0x1000190+4*n;
            }/* end of loop n */
        }/*0x1000190+4*n*/


        {/*0x1000198+4*n*/
            GT_U32    n;
            for(n = 0 ; n <= 1 ; n++) {
                regAddrDbPtr->TTI.ECMCPIDMask[n] =
                    0x1000198+4*n;
            }/* end of loop n */
        }/*0x1000198+4*n*/


        regAddrDbPtr->TTI.CMtagEtherType = 0x010001a0;
        regAddrDbPtr->TTI.PTPEtherTypes = 0x010001a4;
        regAddrDbPtr->TTI.PTPOverUDPDestinationPorts = 0x010001a8;
        regAddrDbPtr->TTI.PTPPktCommandConfig0 = 0x010001ac;
        regAddrDbPtr->TTI.PTPPktCommandConfig1 = 0x010001bc;
        regAddrDbPtr->TTI.TTIUnitInterruptSummaryCause = 0x010001c0;
        regAddrDbPtr->TTI.TTIUnitInterruptSummaryMask = 0x010001c4;
        regAddrDbPtr->TTI.CFMEtherType = 0x010001c8;
        regAddrDbPtr->TTI.IPv4TotalLengthDeduction = 0x010001cc;
        regAddrDbPtr->TTI.TTIInternalConfigurations = 0x01000204;
        {/*0x1000900+n*0x4*/
            GT_U32    n;
            for(n = 0 ; n <= 15 ; n++) {
                regAddrDbPtr->TTI.DSCP2DSCPMapTable[n] =
                    0x1000900+n*0x4;
            }/* end of loop n */
        }/*0x1000900+n*0x4*/


        regAddrDbPtr->TTI.receivedFlowCtrlPktsCntr = 0x010014e0;
        regAddrDbPtr->TTI.droppedFlowCtrlPktsCntr = 0x010014e4;
    }/*end of unit TTI - TTI  */

    if(!SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {/*start of unit DFX_SERVER - added manually  */
        {
            regAddrDbPtr->DFX_SERVER.serverResetControl = 0x018f800c;
            regAddrDbPtr->DFX_SERVER.initDoneStatus = 0x018f8014;
        }
    }

    if(!SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {/*start of unit L2I - L2I  */
        {/*start of unit bridgeAccessMatrix */
            {/*0x2000600+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 7 ; n++) {
                    regAddrDbPtr->L2I.bridgeAccessMatrix.bridgeAccessMatrixLine[n] =
                        0x2000600+n*0x4;
                }/* end of loop n */
            }/*0x2000600+n*0x4*/



        }/*end of unit bridgeAccessMatrix */


        {/*start of unit bridgeDropCntrAndSecurityBreachDropCntrs */
            regAddrDbPtr->L2I.bridgeDropCntrAndSecurityBreachDropCntrs.globalSecurityBreachFilterCntr = 0x02040104;
            regAddrDbPtr->L2I.bridgeDropCntrAndSecurityBreachDropCntrs.portVLANSecurityBreachDropCntr = 0x02040108;
            regAddrDbPtr->L2I.bridgeDropCntrAndSecurityBreachDropCntrs.bridgeFilterCntr = 0x02040150;

        }/*end of unit bridgeDropCntrAndSecurityBreachDropCntrs */


        {/*start of unit bridgeEngineConfig */
            regAddrDbPtr->L2I.bridgeEngineConfig.bridgeGlobalConfig[0] = 0x02040000;
            regAddrDbPtr->L2I.bridgeEngineConfig.bridgeGlobalConfig[1] = 0x02040004;
            regAddrDbPtr->L2I.bridgeEngineConfig.bridgeGlobalConfig[2] = 0x0204000c;
            regAddrDbPtr->L2I.bridgeEngineConfig.sourceIDAssignmentModeConfigPorts = 0x02040070;

        }/*end of unit bridgeEngineConfig */


        {/*start of unit bridgeEngineInterrupts */
            regAddrDbPtr->L2I.bridgeEngineInterrupts.bridgeInterruptCause = 0x02040130;
            regAddrDbPtr->L2I.bridgeEngineInterrupts.bridgeInterruptMask = 0x02040134;

        }/*end of unit bridgeEngineInterrupts */


        {/*start of unit ctrlTrafficToCPUConfig */
            {/*0x2000800+n*0x1000*/
                GT_U32    n;
                for(n = 0 ; n <= 15 ; n++) {
                    regAddrDbPtr->L2I.ctrlTrafficToCPUConfig.IEEEReservedMcConfig0[n] =
                        0x2000800+n*0x1000;
                }/* end of loop n */
            }/*0x2000800+n*0x1000*/


            {/*0x2000804+n*0x1000*/
                GT_U32    n;
                for(n = 0 ; n <= 15 ; n++) {
                    regAddrDbPtr->L2I.ctrlTrafficToCPUConfig.IEEEReservedMcCPUIndex[n] =
                        0x2000804+n*0x1000;
                }/* end of loop n */
            }/*0x2000804+n*0x1000*/


            {/*0x2000808+n*0x1000*/
                GT_U32    n;
                for(n = 0 ; n <= 15 ; n++) {
                    regAddrDbPtr->L2I.ctrlTrafficToCPUConfig.IEEEReservedMcConfig1[n] =
                        0x2000808+n*0x1000;
                }/* end of loop n */
            }/*0x2000808+n*0x1000*/


            regAddrDbPtr->L2I.ctrlTrafficToCPUConfig.IEEETableSelect0 = 0x0200080c;
            {/*0x2000810+n*0x1000*/
                GT_U32    n;
                for(n = 0 ; n <= 15 ; n++) {
                    regAddrDbPtr->L2I.ctrlTrafficToCPUConfig.IEEEReservedMcConfig2[n] =
                        0x2000810+n*0x1000;
                }/* end of loop n */
            }/*0x2000810+n*0x1000*/


            {/*0x2000818+n*0x1000*/
                GT_U32    n;
                for(n = 0 ; n <= 15 ; n++) {
                    regAddrDbPtr->L2I.ctrlTrafficToCPUConfig.IEEEReservedMcConfig3[n] =
                        0x2000818+n*0x1000;
                }/* end of loop n */
            }/*0x2000818+n*0x1000*/


            regAddrDbPtr->L2I.ctrlTrafficToCPUConfig.IEEETableSelect1 = 0x0200081c;
            {/*0x2010800+n*0x1000*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->L2I.ctrlTrafficToCPUConfig.IPv6ICMPMsgTypeConfig[n] =
                        0x2010800+n*0x1000;
                }/* end of loop n */
            }/*0x2010800+n*0x1000*/


            regAddrDbPtr->L2I.ctrlTrafficToCPUConfig.IPv6ICMPCommand = 0x02012800;
            {/*0x2020800+n*0x1000*/
                GT_U32    n;
                for(n = 0 ; n <= 7 ; n++) {
                    regAddrDbPtr->L2I.ctrlTrafficToCPUConfig.IPv4McLinkLocalConfig[n] =
                        0x2020800+n*0x1000;
                }/* end of loop n */
            }/*0x2020800+n*0x1000*/


            {/*0x2028800+n*0x1000*/
                GT_U32    n;
                for(n = 0 ; n <= 7 ; n++) {
                    regAddrDbPtr->L2I.ctrlTrafficToCPUConfig.IPv6McLinkLocalConfig[n] =
                        0x2028800+n*0x1000;
                }/* end of loop n */
            }/*0x2028800+n*0x1000*/


            {/*0x2030800+n*0x1000+ t*0x8000*/
                GT_U32    n,t;
                for(n = 0 ; n <= 7 ; n++) {
                    for(t = 0 ; t <= 1 ; t++) {
                        regAddrDbPtr->L2I.ctrlTrafficToCPUConfig.IPv4McLinkLocalCPUCodeIndex[n][t] =
                            0x2030800+n*0x1000+ t*0x8000;
                    }/* end of loop t */
                }/* end of loop n */
            }/*0x2030800+n*0x1000+ t*0x8000*/


            {/*0x2040800+n*0x1000 +t*0x8000*/
                GT_U32    n,t;
                for(n = 0 ; n <= 7 ; n++) {
                    for(t = 0 ; t <= 1 ; t++) {
                        regAddrDbPtr->L2I.ctrlTrafficToCPUConfig.IPv6McLinkLocalCPUCodeIndex[n][t] =
                            0x2040800+n*0x1000 +t*0x8000;
                    }/* end of loop t */
                }/* end of loop n */
            }/*0x2040800+n*0x1000 +t*0x8000*/



        }/*end of unit ctrlTrafficToCPUConfig */


        {/*start of unit ingrLog */
            regAddrDbPtr->L2I.ingrLog.ingrLogEtherType = 0x02040700;
            regAddrDbPtr->L2I.ingrLog.ingrLogDALow = 0x02040704;
            regAddrDbPtr->L2I.ingrLog.ingrLogDAHigh = 0x02040708;
            regAddrDbPtr->L2I.ingrLog.ingrLogDALowMask = 0x0204070c;
            regAddrDbPtr->L2I.ingrLog.ingrLogDAHighMask = 0x02040710;
            regAddrDbPtr->L2I.ingrLog.ingrLogSALow = 0x02040714;
            regAddrDbPtr->L2I.ingrLog.ingrLogSAHigh = 0x02040718;
            regAddrDbPtr->L2I.ingrLog.ingrLogSALowMask = 0x0204071c;
            regAddrDbPtr->L2I.ingrLog.ingrLogSAHighMask = 0x02040720;
            regAddrDbPtr->L2I.ingrLog.ingrLogMacDAResult = 0x02040724;
            regAddrDbPtr->L2I.ingrLog.ingrLogMacSAResult = 0x02040728;

        }/*end of unit ingrLog */


        {/*start of unit ingrPortsBridgeConfig */
            {/*0x2000000+n*0x1000 and 0x2000000+n*0x1000*/
                GT_U32    n;
                for(n = 0 ; n <= 27 ; n++) {
                    regAddrDbPtr->L2I.ingrPortsBridgeConfig.ingrPortBridgeConfig0[n] =
                        0x2000000+n*0x1000;
                }/* end of loop n */
                for(n = 63 ; n <= 63 ; n++) {
                    regAddrDbPtr->L2I.ingrPortsBridgeConfig.ingrPortBridgeConfig0[(n - 63) + 28] =
                        0x2000000+n*0x1000;
                }/* end of loop n */
            }/*0x2000000+n*0x1000 and 0x2000000+n*0x1000*/


            {/*0x2000010+n*0x1000 and 0x2000010+n*0x1000*/
                GT_U32    n;
                for(n = 0 ; n <= 27 ; n++) {
                    regAddrDbPtr->L2I.ingrPortsBridgeConfig.ingrPortBridgeConfig1[n] =
                        0x2000010+n*0x1000;
                }/* end of loop n */
                for(n = 63 ; n <= 63 ; n++) {
                    regAddrDbPtr->L2I.ingrPortsBridgeConfig.ingrPortBridgeConfig1[(n - 63) + 28] =
                        0x2000010+n*0x1000;
                }/* end of loop n */
            }/*0x2000010+n*0x1000 and 0x2000010+n*0x1000*/



        }/*end of unit ingrPortsBridgeConfig */


        {/*start of unit ingrPortsRateLimitConfig */
            {/*0x2000400+0x1000*n*/
                GT_U32    n;
                for(n = 0 ; n <= 27 ; n++) {
                    regAddrDbPtr->L2I.ingrPortsRateLimitConfig.portRateLimitCntr[n] =
                        0x2000400+0x1000*n;
                }/* end of loop n */
            }/*0x2000400+0x1000*n*/


            regAddrDbPtr->L2I.ingrPortsRateLimitConfig.ingrRateLimitConfig0 = 0x02040140;
            regAddrDbPtr->L2I.ingrPortsRateLimitConfig.ingrRateLimitConfig1 = 0x02040144;
            regAddrDbPtr->L2I.ingrPortsRateLimitConfig.ingrRateLimitDropCntr310 = 0x02040148;
            regAddrDbPtr->L2I.ingrPortsRateLimitConfig.ingrRateLimitDropCntr3932 = 0x0204014c;
            regAddrDbPtr->L2I.ingrPortsRateLimitConfig.MIISpeedGranularity = 0x02040740;
            regAddrDbPtr->L2I.ingrPortsRateLimitConfig.GMIISpeedGranularity = 0x02040744;
            regAddrDbPtr->L2I.ingrPortsRateLimitConfig.stackSpeedGranularity = 0x02040748;

        }/*end of unit ingrPortsRateLimitConfig */


        {/*start of unit IPv6McBridgingBytesSelection */
            regAddrDbPtr->L2I.IPv6McBridgingBytesSelection.IPv6MCBridgingBytesSelectionConfig = 0x02040010;

        }/*end of unit IPv6McBridgingBytesSelection */


        {/*start of unit layer2BridgeMIBCntrs */
            regAddrDbPtr->L2I.layer2BridgeMIBCntrs.MACAddrCount[0] = 0x020400b0;
            regAddrDbPtr->L2I.layer2BridgeMIBCntrs.MACAddrCount[1] = 0x020400b4;
            regAddrDbPtr->L2I.layer2BridgeMIBCntrs.MACAddrCount[2] = 0x020400b8;
            regAddrDbPtr->L2I.layer2BridgeMIBCntrs.hostIncomingPktsCount = 0x020400bc;
            regAddrDbPtr->L2I.layer2BridgeMIBCntrs.hostOutgoingPktsCount = 0x020400c0;
            regAddrDbPtr->L2I.layer2BridgeMIBCntrs.hostOutgoingMcPktCount = 0x020400cc;
            regAddrDbPtr->L2I.layer2BridgeMIBCntrs.hostOutgoingBcPktCount = 0x020400d0;
            regAddrDbPtr->L2I.layer2BridgeMIBCntrs.matrixSourceDestinationPktCount = 0x020400d4;
            {/*0x20400dc+0x14*n*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->L2I.layer2BridgeMIBCntrs.cntrsSetConfig[n] =
                        0x20400dc+0x14*n;
                }/* end of loop n */
            }/*0x20400dc+0x14*n*/


            {/*0x20400e0+n*0x14*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->L2I.layer2BridgeMIBCntrs.setIncomingPktCount[n] =
                        0x20400e0+n*0x14;
                }/* end of loop n */
            }/*0x20400e0+n*0x14*/


            {/*0x20400e4+n*0x14*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->L2I.layer2BridgeMIBCntrs.setVLANIngrFilteredPktCount[n] =
                        0x20400e4+n*0x14;
                }/* end of loop n */
            }/*0x20400e4+n*0x14*/


            {/*0x20400e8+n*0x14*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->L2I.layer2BridgeMIBCntrs.setSecurityFilteredPktCount[n] =
                        0x20400e8+n*0x14;
                }/* end of loop n */
            }/*0x20400e8+n*0x14*/


            {/*0x20400ec+n*0x14*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->L2I.layer2BridgeMIBCntrs.setBridgeFilteredPktCount[n] =
                        0x20400ec+n*0x14;
                }/* end of loop n */
            }/*0x20400ec+n*0x14*/



        }/*end of unit layer2BridgeMIBCntrs */


        {/*start of unit MACBasedQoSTable */
            {/*0x2000200+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 6 ; n++) {
                    regAddrDbPtr->L2I.MACBasedQoSTable.MACQoSTableEntry[n] =
                        0x2000200+n*0x4;
                }/* end of loop n */
            }/*0x2000200+n*0x4*/



        }/*end of unit MACBasedQoSTable */


        {/*start of unit securityBreachStatus */
            regAddrDbPtr->L2I.securityBreachStatus.securityBreachStatus[0] = 0x020401a0;
            regAddrDbPtr->L2I.securityBreachStatus.securityBreachStatus[1] = 0x020401a4;
            regAddrDbPtr->L2I.securityBreachStatus.securityBreachStatus[2] = 0x020401a8;

        }/*end of unit securityBreachStatus */


        {/*start of unit UDPBcMirrorTrapUDPRelayConfig */
            {/*0x2000500+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 11 ; n++) {
                    regAddrDbPtr->L2I.UDPBcMirrorTrapUDPRelayConfig.UDPBcDestinationPortConfigTable[n] =
                        0x2000500+n*0x4;
                }/* end of loop n */
            }/*0x2000500+n*0x4*/



        }/*end of unit UDPBcMirrorTrapUDPRelayConfig */


        {/*start of unit VLANMRUProfilesConfig */
            {/*0x2000300+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->L2I.VLANMRUProfilesConfig.VLANMRUProfileConfig[n] =
                        0x2000300+n*0x4;
                }/* end of loop n */
            }/*0x2000300+n*0x4*/



        }/*end of unit VLANMRUProfilesConfig */


        {/*start of unit VLANRangesConfig */
            regAddrDbPtr->L2I.VLANRangesConfig.ingrVLANRangeConfig = 0x020400a8;

        }/*end of unit VLANRangesConfig */
    }/*end of unit L2I - L2I  */

    if(!SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
    {/*start of unit ucRouter - Unicast Router  */
        regAddrDbPtr->ucRouter.ucRoutingEngineConfigReg = 0x02800e3c;
    }/*end of unit ucRouter - Unicast Router  */

    {/*start of unit IPVX - IPVX  */
        {/*start of unit IPvXRouterDFX */
            {/*0x2800d00+n*4*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->IPvX.IPLog.IPHitLogDIPAddrReg[n] =
                        0x2800d00+n*4;
                }/* end of loop n */
            }/*0x2800d00+n*4*/


            {/*0x2800d10+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->IPvX.IPLog.IPHitLogDIPMaskAddrReg[n] =
                        0x2800d10+n*0x4;
                }/* end of loop n */
            }/*0x2800d10+n*0x4*/


            {/*0x2800d20+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->IPvX.IPLog.IPHitLogSIPAddrReg[n] =
                        0x2800d20+n*0x4;
                }/* end of loop n */
            }/*0x2800d20+n*0x4*/


            {/*0x2800d30+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->IPvX.IPLog.IPHitLogSIPMaskAddrReg[n] =
                        0x2800d30+n*0x4;
                }/* end of loop n */
            }/*0x2800d30+n*0x4*/


            regAddrDbPtr->IPvX.IPLog.IPHitLogLayer4DestinationPortReg = 0x02800d40;
            regAddrDbPtr->IPvX.IPLog.IPHitLogLayer4SourcePortReg = 0x02800d44;
            regAddrDbPtr->IPvX.IPLog.IPHitLogProtocolReg = 0x02800d48;
            {/*0x2800d50+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 2 ; n++) {
                    regAddrDbPtr->IPvX.IPLog.IPHitLogDestinationIPNHEEntryRegWord[n] =
                        0x2800d50+n*0x4;
                }/* end of loop n */
            }/*0x2800d50+n*0x4*/


            {/*0x2800d5c+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 2 ; n++) {
                    regAddrDbPtr->IPvX.IPLog.IPHitLogSourceIPNHEEntryRegWord[n] =
                        0x2800d5c+n*0x4;
                }/* end of loop n */
            }/*0x2800d5c+n*0x4*/



            regAddrDbPtr->IPvX.IPv4GlobalCtrl.IPv4CtrlReg0 = 0x02800100;
            regAddrDbPtr->IPvX.IPv4GlobalCtrl.IPv4CtrlReg1 = 0x02800104;
            regAddrDbPtr->IPvX.IPv4GlobalCtrl.IPv4UcEnableCtrl = 0x02800108;
            regAddrDbPtr->IPvX.IPv4GlobalCtrl.IPv4McEnableCtrl = 0x0280010c;

            regAddrDbPtr->IPvX.IPv6GlobalCtrl.IPv6CtrlReg0 = 0x02800200;
            regAddrDbPtr->IPvX.IPv6GlobalCtrl.IPv6CtrlReg1 = 0x02800204;
            regAddrDbPtr->IPvX.IPv6GlobalCtrl.IPv6UcEnableCtrl = 0x02800208;
            regAddrDbPtr->IPvX.IPv6GlobalCtrl.IPv6McEnableCtrl = 0x0280020c;

            {/*0x2800250+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->IPvX.IPv6Scope.IPv6UcScopePrefix[n] =
                        0x2800250+n*0x4;
                }/* end of loop n */
            }/*0x2800250+n*0x4*/


            {/*0x2800260+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->IPvX.IPv6Scope.IPv6UcScopeLevel[n] =
                        0x2800260+n*0x4;
                }/* end of loop n */
            }/*0x2800260+n*0x4*/


            {/*0x2800278+n*4*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->IPvX.IPv6Scope.IPv6UcScopeTableReg[n] =
                        0x2800278+n*4;
                }/* end of loop n */
            }/*0x2800278+n*4*/


            {/*0x2800288+n*4*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->IPvX.IPv6Scope.IPv6McScopeTableReg[n] =
                        0x2800288+n*4;
                }/* end of loop n */
            }/*0x2800288+n*4*/



            {/*0x2800300+n*4*/
                GT_U32    n;
                for(n = 0 ; n <= 15 ; n++) {
                    regAddrDbPtr->IPvX.qosRoutingConfig.qoSProfileToRouteBlockOffsetEntry[n] =
                        0x2800300+n*4;
                }/* end of loop n */
            }/*0x2800300+n*4*/



            {/*0x2800440+0x4*n*/
                GT_U32    n;
                for(n = 0 ; n <= 7 ; n++) {
                    regAddrDbPtr->IPvX.routerAccessMatrix.routerAccessMatrixLine[n] =
                        0x2800440+0x4*n;
                }/* end of loop n */
            }/*0x2800440+0x4*n*/



            regAddrDbPtr->IPvX.routerBridgedPktsExceptionCntr.routerBridgedPktExceptionsCntr = 0x02800940;

            regAddrDbPtr->IPvX.routerDropCntr.routerDropCntr = 0x02800950;
            regAddrDbPtr->IPvX.routerDropCntr.routerDropCntrConfig = 0x02800954;

            regAddrDbPtr->IPvX.routerGlobalCtrl.routerGlobalCtrl0 = 0x02800000;
            {/*0x2800004+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->IPvX.routerGlobalCtrl.routerMTUConfigReg[n] =
                        0x2800004+n*0x4;
                }/* end of loop n */
            }/*0x2800004+n*0x4*/


            regAddrDbPtr->IPvX.routerGlobalCtrl.routerPerPortSIPSAEnable0 = 0x02800014;
            regAddrDbPtr->IPvX.routerGlobalCtrl.routerAdditionalCtrl = 0x02800964;

            {/*0x2800900+0x100*n*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->IPvX.routerManagementCntrsSets.routerManagementInUcPktsCntrSet[n] =
                        0x2800900+0x100*n;
                }/* end of loop n */
            }/*0x2800900+0x100*n*/


            {/*0x2800904+0x100*n*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->IPvX.routerManagementCntrsSets.routerManagementInMcPktsCntrSet[n] =
                        0x2800904+0x100*n;
                }/* end of loop n */
            }/*0x2800904+0x100*n*/


            {/*0x2800908+0x100*n*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->IPvX.routerManagementCntrsSets.routerManagementInUcNonRoutedNonException[n] =
                        0x2800908+0x100*n;
                }/* end of loop n */
            }/*0x2800908+0x100*n*/


            {/*0x280090c+0x100*n*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->IPvX.routerManagementCntrsSets.routerManagementInUcNonRoutedExceptionPktsCntrSet[n] =
                        0x280090c+0x100*n;
                }/* end of loop n */
            }/*0x280090c+0x100*n*/


            {/*0x2800910+0x100*n*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->IPvX.routerManagementCntrsSets.routerManagementInMcNonRoutedNonExceptionPktsCntrSet[n] =
                        0x2800910+0x100*n;
                }/* end of loop n */
            }/*0x2800910+0x100*n*/


            {/*0x2800914+0x100*n*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->IPvX.routerManagementCntrsSets.routerManagementInMcNonRoutedExceptionPktsCntrSet[n] =
                        0x2800914+0x100*n;
                }/* end of loop n */
            }/*0x2800914+0x100*n*/


            {/*0x2800918+0x100*n*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->IPvX.routerManagementCntrsSets.routerManagementInUcTrappedMirroredPktCntrSet[n] =
                        0x2800918+0x100*n;
                }/* end of loop n */
            }/*0x2800918+0x100*n*/


            {/*0x280091c+0x100*n*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->IPvX.routerManagementCntrsSets.routerManagementInMcTrappedMirroredPktCntrSet[n] =
                        0x280091c+0x100*n;
                }/* end of loop n */
            }/*0x280091c+0x100*n*/


            {/*0x2800920+0x100*n*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->IPvX.routerManagementCntrsSets.routerManagementInMcRPFFailCntrSet[n] =
                        0x2800920+0x100*n;
                }/* end of loop n */
            }/*0x2800920+0x100*n*/


            {/*0x2800924+0x100*n*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->IPvX.routerManagementCntrsSets.routerManagementOutUcPktCntrSet[n] =
                        0x2800924+0x100*n;
                }/* end of loop n */
            }/*0x2800924+0x100*n*/


            regAddrDbPtr->IPvX.routerManagementCntrsSets.routerManagementIncomingPktCntr = 0x02800944;
            regAddrDbPtr->IPvX.routerManagementCntrsSets.routerManagementOutgoingPktCntr = 0x02800948;
            regAddrDbPtr->IPvX.routerManagementCntrsSets.routerManagementInSipFilterCntr = 0x02800958;
            {/*0x2800980+0x100*n*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->IPvX.routerManagementCntrsSets.routerManagementCntrsSetConfig[n] =
                        0x2800980+0x100*n;
                }/* end of loop n */
            }/*0x2800980+0x100*n*/

        }/*end of unit IPvXRouterDFX */
    }/*end of unit IPVX - IPVX  */

    }/* end not support earch or old ipvx mode */

    if(!SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {/*start of unit BM - BM  */
        {/*start of unit BMRegs */
            regAddrDbPtr->BM.BMRegs.bufferManagementInterruptCause0 = 0x03000040;
            regAddrDbPtr->BM.BMRegs.bufferManagementInterruptMask0 = 0x03000044;
            regAddrDbPtr->BM.BMRegs.bufferManagementInterruptCause1 = 0x03000048;
            regAddrDbPtr->BM.BMRegs.bufferManagementInterruptMask1 = 0x0300004c;

            regAddrDbPtr->BM.BMRegs.bufferManagementNumberOfAgedBuffers = 0x03000054;
            regAddrDbPtr->BM.BMRegs.totalBuffersAllocationCntr = 0x03000058;
            regAddrDbPtr->BM.BMRegs.bufferManagementPktCntr = 0x0300005c;
            regAddrDbPtr->BM.BMRegs.portsGroupAllocationCntr = 0x03000080;
            {/*0x3000100+n * 0x4 and 0x3000100+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 27 ; n++) {
                    regAddrDbPtr->BM.BMRegs.portBuffersAllocationCntr[n] =
                        0x3000100+n * 0x4;
                }/* end of loop n */
                for(n = 31 ; n <= 31 ; n++) {
                    regAddrDbPtr->BM.BMRegs.portBuffersAllocationCntr[(n - 31) + 28] =
                        0x3000100+n*0x4;
                }/* end of loop n */
            }/*0x3000100+n * 0x4 and 0x3000100+n*0x4*/



            regAddrDbPtr->BM.BMRegs.bufferManagementAgingConfig = 0x0300000c;

            regAddrDbPtr->BM.BMRegs.bufferManagementGlobalBuffersLimitsConfig = 0x03000000;
            regAddrDbPtr->BM.BMRegs.bufferManagementPortGroupLimitsConfig = 0x03000004;

            regAddrDbPtr->BM.BMRegs.portsBuffersLimitProfileConfig[0] = 0x03000020;
            regAddrDbPtr->BM.BMRegs.portsBuffersLimitProfileConfig[1]= 0x03000024;
            regAddrDbPtr->BM.BMRegs.portsBuffersLimitProfileConfig[2] = 0x03000028;
            regAddrDbPtr->BM.BMRegs.portsBuffersLimitProfileConfig[3] = 0x0300002c;
            regAddrDbPtr->BM.BMRegs.buffersLimitsProfileAssociation0 = 0x03000030;
            regAddrDbPtr->BM.BMRegs.buffersLimitsProfileAssociation1 = 0x03000034;

            regAddrDbPtr->BM.BMRegs.burstConfig = 0x03000350;
            regAddrDbPtr->BM.BMRegs.generalConfigs = 0x03000354;
            regAddrDbPtr->BM.BMRegs.sliceEnable = 0x03000358;
            regAddrDbPtr->BM.BMRegs.slicePortMap07 = 0x0300035c;
            regAddrDbPtr->BM.BMRegs.slicePortMap815 = 0x03000360;
            regAddrDbPtr->BM.BMRegs.slicePortMap1623 = 0x03000364;
            regAddrDbPtr->BM.BMRegs.slicePortMap2431 = 0x03000368;

        }/*end of unit BMRegs */
    }/*end of unit BM - BM  */

    if(!SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {/*start of unit LMS - LMS  */
        {/*start of unit configRegs */
            regAddrDbPtr->LMS1.configRegs.periodicFlowCtrlInterval1 = 0x05004140;
            regAddrDbPtr->LMS1.configRegs.periodicFlowCtrlIntervalSelection = 0x05004144;

        }/*end of unit configRegs */


        {/*start of unit internalRegs */
            regAddrDbPtr->LMS1.internalRegs.LSInterruptCause = 0x04800000;
            regAddrDbPtr->LMS1.internalRegs.LSInterruptMask = 0x04800004;

        }/*end of unit internalRegs */


        {/*start of unit LEDInterfaceConfig */
            regAddrDbPtr->LMS1.LEDInterfaceConfig.LEDInterfaceCtrlReg0 = 0x04004100;
            regAddrDbPtr->LMS1.LEDInterfaceConfig.LEDInterfaceGroup01ConfigReg = 0x04004104;
            regAddrDbPtr->LMS1.LEDInterfaceConfig.LEDInterfaceClass01ManipulationReg = 0x04004108;
            regAddrDbPtr->LMS1.LEDInterfaceConfig.LEDInterfaceClass4ManipulationReg = 0x0400410c;
            regAddrDbPtr->LMS1.LEDInterfaceConfig.XLGLEDInterfaceClass59ManipulationReg = 0x04005104;
            regAddrDbPtr->LMS1.LEDInterfaceConfig.XLGLEDInterfaceDebugSelectReg = 0x04005110;
            regAddrDbPtr->LMS1.LEDInterfaceConfig.LEDInterface0CtrlReg1AndClass6ManipulationRegforPorts0Through11 = 0x04804100;
            regAddrDbPtr->LMS1.LEDInterfaceConfig.LEDInterface0Group23ConfigRegforPorts0Through11 = 0x04804104;
            regAddrDbPtr->LMS1.LEDInterfaceConfig.LEDInterface0Class23ManipulationRegforPorts0Through11 = 0x04804108;
            regAddrDbPtr->LMS1.LEDInterfaceConfig.LEDInterface0Class5ManipulationRegforPorts0Through11 = 0x0480410c;
            regAddrDbPtr->LMS1.LEDInterfaceConfig.XGLEDInterface0Class1011ManipulationRegforXGPorts = 0x04805100;
            regAddrDbPtr->LMS1.LEDInterfaceConfig.XGLEDInterface0Group01ConfigRegforXGPort = 0x04805104;

        }/*end of unit LEDInterfaceConfig */


        {/*start of unit miscellaneousRegs */
            regAddrDbPtr->LMS1.miscellaneousRegs.periodicFlowCtrlInterval0 = 0x04004140;

        }/*end of unit miscellaneousRegs */


        {/*start of unit portsMACSourceAddr */
            regAddrDbPtr->LMS1.portsMACSourceAddr.sourceAddrMiddle = 0x04004024;
            regAddrDbPtr->LMS1.portsMACSourceAddr.sourceAddrHigh = 0x04004028;

        }/*end of unit portsMACSourceAddr */


        {/*start of unit SMIConfig */
            regAddrDbPtr->LMS1.SMIConfig.PHYAddrReg0forPorts0Through5 = 0x04004030;
            regAddrDbPtr->LMS1.SMIConfig.SMI0Management = 0x04004054;
            regAddrDbPtr->LMS1.SMIConfig.LMS0MiscConfigs = 0x04004200;
            regAddrDbPtr->LMS1.SMIConfig.PHYAddrReg1forPorts6Through11 = 0x04804030;

        }/*end of unit SMIConfig */
    }/*end of unit LMS - LMS  */

    if(!SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {/*start of unit FDB - FDB  */
        {/*start of unit deviceTable */
            regAddrDbPtr->FDB.deviceTable.deviceTable = 0x06000068;

        }/*end of unit deviceTable */


        {/*start of unit FDBAction */
            regAddrDbPtr->FDB.FDBAction.FDBAction[0] = 0x06000004;
            regAddrDbPtr->FDB.FDBAction.FDBAction[1] = 0x06000008;
            regAddrDbPtr->FDB.FDBAction.FDBAction[2] = 0x06000020;

        }/*end of unit FDBAction */


        {/*start of unit FDBAddrUpdateMsgs */
            regAddrDbPtr->FDB.FDBAddrUpdateMsgs.AUMsgToCPU = 0x06000034;
            regAddrDbPtr->FDB.FDBAddrUpdateMsgs.AUFIFOToCPUConfig = 0x06000038;

            {/*0x6000040+n*4*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->FDB.FDBAddrUpdateMsgs.AUMsgFromCPUWord[n] =
                        0x6000040+n*4;
                }/* end of loop n */
            }/*0x6000040+n*4*/


            regAddrDbPtr->FDB.FDBAddrUpdateMsgs.AUMsgFromCPUCtrl = 0x06000050;

        }/*end of unit FDBAddrUpdateMsgs */


        {/*start of unit FDBDirectAccess */
            {/*0x6000054+n*4*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->FDB.FDBDirectAccess.FDBDirectAccessData[n] =
                        0x6000054+n*4;
                }/* end of loop n */
            }/*0x6000054+n*4*/

            regAddrDbPtr->FDB.FDBDirectAccess.FDBDirectAccessCtrl = 0x06000064;

        }/*end of unit FDBDirectAccess */


        {/*start of unit FDBGlobalConfig */
            regAddrDbPtr->FDB.FDBGlobalConfig.FDBGlobalConfig = 0x06000000;
            regAddrDbPtr->FDB.FDBGlobalConfig.bridgeAccessLevelConfig = 0x0600003c;

        }/*end of unit FDBGlobalConfig */


        {/*start of unit FDBInternal */
            regAddrDbPtr->FDB.FDBInternal.metalFix = 0x0600006c;
            regAddrDbPtr->FDB.FDBInternal.FDBIdleState = 0x06000070;

        }/*end of unit FDBInternal */


        {/*start of unit FDBInterrupt */
            regAddrDbPtr->FDB.FDBInterrupt.FDBInterruptCauseReg = 0x06000018;
            regAddrDbPtr->FDB.FDBInterrupt.FDBInterruptMaskReg = 0x0600001c;

        }/*end of unit FDBInterrupt */


        {/*start of unit sourceAddrNotLearntCntr */
            regAddrDbPtr->FDB.sourceAddrNotLearntCntr.learnedEntryDiscardsCount = 0x06000030;

        }/*end of unit sourceAddrNotLearntCntr */
    }/*end of unit FDB - FDB  */

    /* not for bobcat2 */
    if(devObjPtr->portGroupSharedDevObjPtr)
    {/*start of unit MPPM0 - MPPM0  */
        {/*start of unit ecc */
            regAddrDbPtr->MPPM0.ecc.eccCtrl = 0x06800010;

        }/*end of unit ecc */

        regAddrDbPtr->MPPM0.xlgMode = 0x06800000;
        regAddrDbPtr->MPPM0.interruptCause = 0x06800004;
        regAddrDbPtr->MPPM0.interruptMask = 0x06800008;

    }/*end of unit MPPM0 - MPPM0  */

    /* not for bobcat2 */
    if(devObjPtr->portGroupSharedDevObjPtr)
    {/*start of unit CTU0 - CTU0  */
        {/*start of unit CTURegs */
            regAddrDbPtr->CTU0.CTURegs.CTUClearFIFOThreshold = 0x07000004;
            regAddrDbPtr->CTU0.CTURegs.CTUWRRDescTxDMAArbiterWeights = 0x07000008;
            regAddrDbPtr->CTU0.CTURegs.CTUFinishedCutThroughPktFCPLatency = 0x0700000c;
            regAddrDbPtr->CTU0.CTURegs.descRefreshFinishedCutThroughPktFCPThreshold = 0x07000010;
            regAddrDbPtr->CTU0.CTURegs.ECCConfig = 0x07000014;
            regAddrDbPtr->CTU0.CTURegs.finishedCutThroughPktFCPDFXReg = 0x07000018;
            regAddrDbPtr->CTU0.CTURegs.descRefreshDFX = 0x0700001c;
            regAddrDbPtr->CTU0.CTURegs.CTUMetalFix = 0x07000020;
            regAddrDbPtr->CTU0.CTURegs.BMACTParallelDROTimerValue = 0x07000024;
            regAddrDbPtr->CTU0.CTURegs.BMACTParallelDROCtrl = 0x07000028;

            {/*0x7002010+4*d*/
                GT_U32    d;
                for(d = 0 ; d <= 3 ; d++) {
                    regAddrDbPtr->CTU0.CTURegs.descCTDescCntr[d] =
                        0x7002010+4*d;
                }/* end of loop d */
            }/*0x7002010+4*d*/


            {/*0x7002000+4*d*/
                GT_U32    d;
                for(d = 0 ; d <= 3 ; d++) {
                    regAddrDbPtr->CTU0.CTURegs.descAllDescCntr[d] =
                        0x7002000+4*d;
                }/* end of loop d */
            }/*0x7002000+4*d*/


            {/*0x7002020+4*d*/
                GT_U32    d;
                for(d = 0 ; d <= 3 ; d++) {
                    regAddrDbPtr->CTU0.CTURegs.descNonCTDescCntr[d] =
                        0x7002020+4*d;
                }/* end of loop d */
            }/*0x7002020+4*d*/


            {/*0x7002030+4*d*/
                GT_U32    d;
                for(d = 0 ; d <= 3 ; d++) {
                    regAddrDbPtr->CTU0.CTURegs.descCTFinishedDescCntr[d] =
                        0x7002030+4*d;
                }/* end of loop d */
            }/*0x7002030+4*d*/


            {/*0x7002040+4*d*/
                GT_U32    d;
                for(d = 0 ; d <= 3 ; d++) {
                    regAddrDbPtr->CTU0.CTURegs.descCTNonFinishedDescCntr[d] =
                        0x7002040+4*d;
                }/* end of loop d */
            }/*0x7002040+4*d*/


            {/*0x7002050+4*d*/
                GT_U32    d;
                for(d = 0 ; d <= 3 ; d++) {
                    regAddrDbPtr->CTU0.CTURegs.clearAllClearCntr[d] =
                        0x7002050+4*d;
                }/* end of loop d */
            }/*0x7002050+4*d*/


            {/*0x7002060+4*d*/
                GT_U32    d;
                for(d = 0 ; d <= 3 ; d++) {
                    regAddrDbPtr->CTU0.CTURegs.clearCTClearCntr[d] =
                        0x7002060+4*d;
                }/* end of loop d */
            }/*0x7002060+4*d*/


            {/*0x7002070+4*d*/
                GT_U32    d;
                for(d = 0 ; d <= 3 ; d++) {
                    regAddrDbPtr->CTU0.CTURegs.clearNonCTClearCntr[d] =
                        0x7002070+4*d;
                }/* end of loop d */
            }/*0x7002070+4*d*/


            {/*0x7002080+4*d*/
                GT_U32    d;
                for(d = 0 ; d <= 3 ; d++) {
                    regAddrDbPtr->CTU0.CTURegs.clearCTFinishedClearCntr[d] =
                        0x7002080+4*d;
                }/* end of loop d */
            }/*0x7002080+4*d*/


            {/*0x7002090+4*d*/
                GT_U32    d;
                for(d = 0 ; d <= 3 ; d++) {
                    regAddrDbPtr->CTU0.CTURegs.clearCTNonFinishedClearCntr[d] =
                        0x7002090+4*d;
                }/* end of loop d */
            }/*0x7002090+4*d*/


            {/*0x70020a0+4*d*/
                GT_U32    d;
                for(d = 0 ; d <= 3 ; d++) {
                    regAddrDbPtr->CTU0.CTURegs.FCPRxDMAClearCntr[d] =
                        0x70020a0+4*d;
                }/* end of loop d */
            }/*0x70020a0+4*d*/


            {/*0x70020b0+4*d*/
                GT_U32    d;
                for(d = 0 ; d <= 3 ; d++) {
                    regAddrDbPtr->CTU0.CTURegs.rxDMATotalCTPktsCntr[d] =
                        0x70020b0+4*d;
                }/* end of loop d */
            }/*0x70020b0+4*d*/


            {/*0x70020c0+4*d*/
                GT_U32    d;
                for(d = 0 ; d <= 3 ; d++) {
                    regAddrDbPtr->CTU0.CTURegs.rxDMAOngoingCTPktsCntr[d] =
                        0x70020c0+4*d;
                }/* end of loop d */
            }/*0x70020c0+4*d*/


            regAddrDbPtr->CTU0.CTURegs.BMACTParallelDROLVTCntr = 0x070020d0;
            regAddrDbPtr->CTU0.CTURegs.BMACTParallelDRONVTCntr = 0x070020d4;
            regAddrDbPtr->CTU0.CTURegs.BMACTParallelDROHVTCntr = 0x070020d8;

            regAddrDbPtr->CTU0.CTURegs.CTUInterruptCause = 0x07003000;
            regAddrDbPtr->CTU0.CTURegs.CTUInterruptMask = 0x07003004;

            {/*0x7001000+4*d*/
                GT_U32    d;
                for(d = 0 ; d <= 3 ; d++) {
                    regAddrDbPtr->CTU0.CTURegs.BMA2CTUPipeLastClear[d] =
                        0x7001000+4*d;
                }/* end of loop d */
            }/*0x7001000+4*d*/


            {/*0x7001010+4*d*/
                GT_U32    d;
                for(d = 0 ; d <= 3 ; d++) {
                    regAddrDbPtr->CTU0.CTURegs.CTU2BMLastFinalClear[d] =
                        0x7001010+4*d;
                }/* end of loop d */
            }/*0x7001010+4*d*/


            {/*0x7001020+4*d*/
                GT_U32    d;
                for(d = 0 ; d <= 3 ; d++) {
                    regAddrDbPtr->CTU0.CTURegs.CTU2BMLastRxDMAClear[d] =
                        0x7001020+4*d;
                }/* end of loop d */
            }/*0x7001020+4*d*/


            {/*0x7001030+4*d*/
                GT_U32    d;
                for(d = 0 ; d <= 3 ; d++) {
                    regAddrDbPtr->CTU0.CTURegs.txDMA2CTULastBCQuery[d] =
                        0x7001030+4*d;
                }/* end of loop d */
            }/*0x7001030+4*d*/


            {/*0x7001040+4*d*/
                GT_U32    d;
                for(d = 0 ; d <= 3 ; d++) {
                    regAddrDbPtr->CTU0.CTURegs.CTU2TxDMALastBCReply[d] =
                        0x7001040+4*d;
                }/* end of loop d */
            }/*0x7001040+4*d*/


            {/*0x7001050+4*d*/
                GT_U32    d;
                for(d = 0 ; d <= 3 ; d++) {
                    regAddrDbPtr->CTU0.CTURegs.RXDMA2CTULastUpdate[d] =
                        0x7001050+4*d;
                }/* end of loop d */
            }/*0x7001050+4*d*/


            {/*0x7001060+4*d*/
                GT_U32    d;
                for(d = 0 ; d <= 3 ; d++) {
                    regAddrDbPtr->CTU0.CTURegs.FCPECCStatus[d] =
                        0x7001060+4*d;
                }/* end of loop d */
            }/*0x7001060+4*d*/


            regAddrDbPtr->CTU0.CTURegs.CTUDebugBus = 0x07001070;
            {/*0x7001080+4*d*/
                GT_U32    d;
                for(d = 0 ; d <= 3 ; d++) {
                    regAddrDbPtr->CTU0.CTURegs.clearFIFOStatus[d] =
                        0x7001080+4*d;
                }/* end of loop d */
            }/*0x7001080+4*d*/


            {/*0x7001090+4*d*/
                GT_U32    d;
                for(d = 0 ; d <= 3 ; d++) {
                    regAddrDbPtr->CTU0.CTURegs.descFIFOStatus[d] =
                        0x7001090+4*d;
                }/* end of loop d */
            }/*0x7001090+4*d*/


            regAddrDbPtr->CTU0.CTURegs.descClear01FIFOOverrunStatus = 0x070010a0;
            regAddrDbPtr->CTU0.CTURegs.descClear23FIFOOverrunStatus = 0x070010b0;
            {/*0x70010c0+4*d*/
                GT_U32    d;
                for(d = 0 ; d <= 3 ; d++) {
                    regAddrDbPtr->CTU0.CTURegs.FCP_OCP_Interrupt_Status[d] =
                        0x70010c0+4*d;
                }/* end of loop d */
            }/*0x70010c0+4*d*/


            {/*0x7001100+4*d*/
                GT_U32    d;
                for(d = 0 ; d <= 3 ; d++) {
                    regAddrDbPtr->CTU0.CTURegs.FCP_FIFO_Full_Status[d] =
                        0x7001100+4*d;
                }/* end of loop d */
            }/*0x7001100+4*d*/


            {/*0x7001110+4*d*/
                GT_U32    d;
                for(d = 0 ; d <= 3 ; d++) {
                    regAddrDbPtr->CTU0.CTURegs.FCP_FIFO_Empty_Status[d] =
                        0x7001110+4*d;
                }/* end of loop d */
            }/*0x7001110+4*d*/


            {/*0x7001120+4*d*/
                GT_U32    d;
                for(d = 0 ; d <= 3 ; d++) {
                    regAddrDbPtr->CTU0.CTURegs.FCP_FIFO_Overrun_Int_Status[d] =
                        0x7001120+4*d;
                }/* end of loop d */
            }/*0x7001120+4*d*/


            regAddrDbPtr->CTU0.CTURegs.descUnusedFIFOECCErrorType = 0x07001130;

        }/*end of unit CTURegs */
    }/*end of unit CTU0 - CTU0  */

    {/*start of unit GOP - GOP  */
        GT_U32  offset;

        {/*start of unit SERDES */
            GT_U32    s;
            for(s = 0 ; s < numOfSerdeses ; s++) {
                offset = 0; /*0x9800000*/
                if(isGopVer1)
                {
                    if (isBobK)
                    {
                        if (s == 20 || (s > 23 && s < 36))
                        {
                            offset = 0x40000;
                        }
                        else
                        {
                            if (devObjPtr->deviceType == SKERNEL_BOBK_CETUS)
                            {
                                continue;
                            } 
                            else 
                            {
                                if (s > 11)
                                {
                                    continue;
                                }
                            }
                        }
                    }
                }
                else
                {
                    offset = 0x09800000;
                }
                smemLion2RegsInfoSet_GOP_SERDES(devObjPtr, regAddrDbPtr, s, offset, isGopVer1); 
            }/* end of loop s */
        }/*end of unit SERDES */

        if(isGopVer1 == 0)/* patch for Bobcat2 that not hold special mac for the CPU port*/
        {/*start of unit cpuPort */
            offset = 0x08800000;
            smemLion2RegsInfoSet_GOP_gigPort63CPU(devObjPtr,regAddrDbPtr,offset);
        }/*end of unit cpuPort */

        if(isGopVer1)
        {
            devObjPtr->portMacOffset = 0x1000;
            devObjPtr->portMacMask   = 0x7F;/* first base support 56 port and second base 16 */
            devObjPtr->portMacSecondBase = BOBCAT2_PORT_56_START_OFFSET_CNS;
            devObjPtr->portMacSecondBaseFirstPort = 56;
        }

        {/*start of unit gigPort */
            GT_U32    p,minus_P;
            for(p = 0 ; p < numOfGigMacPorts ; p++) {
                offset = 0;/*0x0880000c*/
                minus_P = 0;
                if(isGopVer1)
                {
                    if(p >= 56)
                    {
                        offset = BOBCAT2_PORT_56_START_OFFSET_CNS;
                        minus_P = 56;
                    }
                    else
                    {
                    }
                }
                else
                {
                    offset = 0x08800000;
                }

                smemLion2RegsInfoSet_GOP_gigPort(devObjPtr,regAddrDbPtr,p,minus_P ,offset,isGopVer1);
            }
        }/*end of unit gigPort */

        {/*start of unit XLGIP */
            GT_U32    p,minus_P;
            for(p = 0 ; p <= numOfXlgMacPorts ; p++) {
                offset = 0;
                minus_P = 0;
                if(isGopVer1)
                {
                    if(p >= 56)
                    {
                        offset = BOBCAT2_PORT_56_START_OFFSET_CNS;
                        minus_P = 56;
                    }
                    else
                    {
                        if(p == 0 && (firstXlgPort != 0))
                        {
                            p = firstXlgPort;/* first port is 48 */
                        }
                    }
                }
                else
                {
                    offset = 0x08800000;
                    if(p== 13 && skipXlgPorts) continue;
                }

                smemLion2RegsInfoSet_GOP_XLGIP(devObjPtr,regAddrDbPtr,p,minus_P ,offset);
            }
        }/*end of unit XLGIP */

        {/*start of unit MPCSIP */
            GT_U32    p,minus_P;
            for(p = 0 ; p <= numOfXlgMacPorts ; p++) {
                offset = 0x000C0000;
                minus_P = 0;
                if(isGopVer1)
                {
                    if(p >= 56)
                    {
                        offset = BOBCAT2_PORT_56_START_OFFSET_CNS + 0x000C0000;
                        minus_P = 56;
                    }
                    else
                    {
                        if(p == 0 && (firstXlgPort != 0))
                        {
                            p = firstXlgPort;/* first port is 48 */
                        }
                    }
                }
                else
                {
                    offset = 0x08800400;
                }

                smemLion2RegsInfoSet_GOP_MPCSIP(devObjPtr,regAddrDbPtr,p,minus_P ,offset);
            }/* end of loop p */
        }/*end of unit MPCSIP */

        if(isGopVer1 == 0)/* no such for bobcat2 */
        { /* Packet Gen Packet Gen IP %p Units */
            GT_U32    p,minus_P=0;
            for(p = 0 ; p <= numOfXlgMacPorts ; p++) {
                offset = 0x08980000;
                smemLion2RegsInfoSet_GOP_packGenConfig(devObjPtr,regAddrDbPtr,p,minus_P ,offset);
            }/* end of loop p */
        }/* End of Packet Gen Packet Gen IP %p Units */
    }/*end of unit GOP - GOP  */

    /* not for bobcat2 */
    if(devObjPtr->portGroupSharedDevObjPtr)
    {/*start of unit CTU1 - CTU1  */
        {/*start of unit CTURegs */
            regAddrDbPtr->CTU1.CTURegs.CTUClearFIFOThreshold = 0x0a800004;
            regAddrDbPtr->CTU1.CTURegs.CTUWRRDescTxDMAArbiterWeights = 0x0a800008;
            regAddrDbPtr->CTU1.CTURegs.CTUFinishedCutThroughPktFCPLatency = 0x0a80000c;
            regAddrDbPtr->CTU1.CTURegs.descRefreshFinishedCutThroughPktFCPThreshold = 0x0a800010;
            regAddrDbPtr->CTU1.CTURegs.ECCConfig = 0x0a800014;
            regAddrDbPtr->CTU1.CTURegs.finishedCutThroughPktFCPDFXReg = 0x0a800018;
            regAddrDbPtr->CTU1.CTURegs.descRefreshDFX = 0x0a80001c;
            regAddrDbPtr->CTU1.CTURegs.CTUMetalFix = 0x0a800020;
            regAddrDbPtr->CTU1.CTURegs.BMACTParallelDROTimerValue = 0x0a800024;
            regAddrDbPtr->CTU1.CTURegs.BMACTParallelDROCtrl = 0x0a800028;

            {/*0xa802010+4*d*/
                GT_U32    d;
                for(d = 0 ; d <= 3 ; d++) {
                    regAddrDbPtr->CTU1.CTURegs.descCTDescCntr[d] =
                        0xa802010+4*d;
                }/* end of loop d */
            }/*0xa802010+4*d*/


            {/*0xa802000+4*d*/
                GT_U32    d;
                for(d = 0 ; d <= 3 ; d++) {
                    regAddrDbPtr->CTU1.CTURegs.descAllDescCntr[d] =
                        0xa802000+4*d;
                }/* end of loop d */
            }/*0xa802000+4*d*/


            {/*0xa802020+4*d*/
                GT_U32    d;
                for(d = 0 ; d <= 3 ; d++) {
                    regAddrDbPtr->CTU1.CTURegs.descNonCTDescCntr[d] =
                        0xa802020+4*d;
                }/* end of loop d */
            }/*0xa802020+4*d*/


            {/*0xa802030+4*d*/
                GT_U32    d;
                for(d = 0 ; d <= 3 ; d++) {
                    regAddrDbPtr->CTU1.CTURegs.descCTFinishedDescCntr[d] =
                        0xa802030+4*d;
                }/* end of loop d */
            }/*0xa802030+4*d*/


            {/*0xa802040+4*d*/
                GT_U32    d;
                for(d = 0 ; d <= 3 ; d++) {
                    regAddrDbPtr->CTU1.CTURegs.descCTNonFinishedDescCntr[d] =
                        0xa802040+4*d;
                }/* end of loop d */
            }/*0xa802040+4*d*/


            {/*0xa802050+4*d*/
                GT_U32    d;
                for(d = 0 ; d <= 3 ; d++) {
                    regAddrDbPtr->CTU1.CTURegs.clearAllClearCntr[d] =
                        0xa802050+4*d;
                }/* end of loop d */
            }/*0xa802050+4*d*/


            {/*0xa802060+4*d*/
                GT_U32    d;
                for(d = 0 ; d <= 3 ; d++) {
                    regAddrDbPtr->CTU1.CTURegs.clearCTClearCntr[d] =
                        0xa802060+4*d;
                }/* end of loop d */
            }/*0xa802060+4*d*/


            {/*0xa802070+4*d*/
                GT_U32    d;
                for(d = 0 ; d <= 3 ; d++) {
                    regAddrDbPtr->CTU1.CTURegs.clearNonCTClearCntr[d] =
                        0xa802070+4*d;
                }/* end of loop d */
            }/*0xa802070+4*d*/


            {/*0xa802080+4*d*/
                GT_U32    d;
                for(d = 0 ; d <= 3 ; d++) {
                    regAddrDbPtr->CTU1.CTURegs.clearCTFinishedClearCntr[d] =
                        0xa802080+4*d;
                }/* end of loop d */
            }/*0xa802080+4*d*/


            {/*0xa802090+4*d*/
                GT_U32    d;
                for(d = 0 ; d <= 3 ; d++) {
                    regAddrDbPtr->CTU1.CTURegs.clearCTNonFinishedClearCntr[d] =
                        0xa802090+4*d;
                }/* end of loop d */
            }/*0xa802090+4*d*/


            {/*0xa8020a0+4*d*/
                GT_U32    d;
                for(d = 0 ; d <= 3 ; d++) {
                    regAddrDbPtr->CTU1.CTURegs.FCPRxDMAClearCntr[d] =
                        0xa8020a0+4*d;
                }/* end of loop d */
            }/*0xa8020a0+4*d*/


            {/*0xa8020b0+4*d*/
                GT_U32    d;
                for(d = 0 ; d <= 3 ; d++) {
                    regAddrDbPtr->CTU1.CTURegs.rxDMATotalCTPktsCntr[d] =
                        0xa8020b0+4*d;
                }/* end of loop d */
            }/*0xa8020b0+4*d*/


            {/*0xa8020c0+4*d*/
                GT_U32    d;
                for(d = 0 ; d <= 3 ; d++) {
                    regAddrDbPtr->CTU1.CTURegs.rxDMAOngoingCTPktsCntr[d] =
                        0xa8020c0+4*d;
                }/* end of loop d */
            }/*0xa8020c0+4*d*/


            regAddrDbPtr->CTU1.CTURegs.BMACTParallelDROLVTCntr = 0x0a8020d0;
            regAddrDbPtr->CTU1.CTURegs.BMACTParallelDRONVTCntr = 0x0a8020d4;
            regAddrDbPtr->CTU1.CTURegs.BMACTParallelDROHVTCntr = 0x0a8020d8;

            regAddrDbPtr->CTU1.CTURegs.CTUInterruptCause = 0x0a803000;
            regAddrDbPtr->CTU1.CTURegs.CTUInterruptMask = 0x0a803004;

            {/*0xa801000+4*d*/
                GT_U32    d;
                for(d = 0 ; d <= 3 ; d++) {
                    regAddrDbPtr->CTU1.CTURegs.BMA2CTUPipeLastClear[d] =
                        0xa801000+4*d;
                }/* end of loop d */
            }/*0xa801000+4*d*/


            {/*0xa801010+4*d*/
                GT_U32    d;
                for(d = 0 ; d <= 3 ; d++) {
                    regAddrDbPtr->CTU1.CTURegs.CTU2BMLastFinalClear[d] =
                        0xa801010+4*d;
                }/* end of loop d */
            }/*0xa801010+4*d*/


            {/*0xa801020+4*d*/
                GT_U32    d;
                for(d = 0 ; d <= 3 ; d++) {
                    regAddrDbPtr->CTU1.CTURegs.CTU2BMLastRxDMAClear[d] =
                        0xa801020+4*d;
                }/* end of loop d */
            }/*0xa801020+4*d*/


            {/*0xa801030+4*d*/
                GT_U32    d;
                for(d = 0 ; d <= 3 ; d++) {
                    regAddrDbPtr->CTU1.CTURegs.txDMA2CTULastBCQuery[d] =
                        0xa801030+4*d;
                }/* end of loop d */
            }/*0xa801030+4*d*/


            {/*0xa801040+4*d*/
                GT_U32    d;
                for(d = 0 ; d <= 3 ; d++) {
                    regAddrDbPtr->CTU1.CTURegs.CTU2TxDMALastBCReply[d] =
                        0xa801040+4*d;
                }/* end of loop d */
            }/*0xa801040+4*d*/


            {/*0xa801050+4*d*/
                GT_U32    d;
                for(d = 0 ; d <= 3 ; d++) {
                    regAddrDbPtr->CTU1.CTURegs.RXDMA2CTULastUpdate[d] =
                        0xa801050+4*d;
                }/* end of loop d */
            }/*0xa801050+4*d*/


            {/*0xa801060+4*d*/
                GT_U32    d;
                for(d = 0 ; d <= 3 ; d++) {
                    regAddrDbPtr->CTU1.CTURegs.FCPECCStatus[d] =
                        0xa801060+4*d;
                }/* end of loop d */
            }/*0xa801060+4*d*/


            regAddrDbPtr->CTU1.CTURegs.CTUDebugBus = 0x0a801070;
            {/*0xa801080+4*d*/
                GT_U32    d;
                for(d = 0 ; d <= 3 ; d++) {
                    regAddrDbPtr->CTU1.CTURegs.clearFIFOStatus[d] =
                        0xa801080+4*d;
                }/* end of loop d */
            }/*0xa801080+4*d*/


            {/*0xa801090+4*d*/
                GT_U32    d;
                for(d = 0 ; d <= 3 ; d++) {
                    regAddrDbPtr->CTU1.CTURegs.descFIFOStatus[d] =
                        0xa801090+4*d;
                }/* end of loop d */
            }/*0xa801090+4*d*/


            regAddrDbPtr->CTU1.CTURegs.descClear01FIFOOverrunStatus = 0x0a8010a0;
            regAddrDbPtr->CTU1.CTURegs.descClear23FIFOOverrunStatus = 0x0a8010b0;
            {/*0xa8010c0+4*d*/
                GT_U32    d;
                for(d = 0 ; d <= 3 ; d++) {
                    regAddrDbPtr->CTU1.CTURegs.FCP_OCP_Interrupt_Status[d] =
                        0xa8010c0+4*d;
                }/* end of loop d */
            }/*0xa8010c0+4*d*/


            {/*0xa801100+4*d*/
                GT_U32    d;
                for(d = 0 ; d <= 3 ; d++) {
                    regAddrDbPtr->CTU1.CTURegs.FCP_FIFO_Full_Status[d] =
                        0xa801100+4*d;
                }/* end of loop d */
            }/*0xa801100+4*d*/


            {/*0xa801110+4*d*/
                GT_U32    d;
                for(d = 0 ; d <= 3 ; d++) {
                    regAddrDbPtr->CTU1.CTURegs.FCP_FIFO_Empty_Status[d] =
                        0xa801110+4*d;
                }/* end of loop d */
            }/*0xa801110+4*d*/


            {/*0xa801120+4*d*/
                GT_U32    d;
                for(d = 0 ; d <= 3 ; d++) {
                    regAddrDbPtr->CTU1.CTURegs.FCP_FIFO_Overrun_Int_Status[d] =
                        0xa801120+4*d;
                }/* end of loop d */
            }/*0xa801120+4*d*/


            regAddrDbPtr->CTU1.CTURegs.descUnusedFIFOECCErrorType = 0x0a801130;

        }/*end of unit CTURegs */
    }/*end of unit CTU1 - CTU1  */

    if(!SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {/*start of unit EQ - EQ  */
        {/*start of unit applicationSpecificCPUCodes */
            {/*0xb007000+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 15 ; n++) {
                    regAddrDbPtr->EQ.applicationSpecificCPUCodes.TCPUDPDestPortRangeCPUCodeEntryWord0[n] =
                        0xb007000+n*0x4;
                }/* end of loop n */
            }/*0xb007000+n*0x4*/


            {/*0xb007040+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 15 ; n++) {
                    regAddrDbPtr->EQ.applicationSpecificCPUCodes.TCPUDPDestPortRangeCPUCodeEntryWord1[n] =
                        0xb007040+n*0x4;
                }/* end of loop n */
            }/*0xb007040+n*0x4*/

            {/*0xb00007c+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 8 ; n++) {
                    regAddrDbPtr->EQ.multiCoreLookup.multiCoreLookup[n] =
                        0xb00007c+n*0x4;
                }/* end of loop n */
            }/*0xb00007c+n*0x4*/


            {/*0xb008000+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->EQ.applicationSpecificCPUCodes.IPProtocolCPUCodeEntry[n] =
                        0xb008000+n*0x4;
                }/* end of loop n */
            }/*0xb008000+n*0x4*/


            regAddrDbPtr->EQ.applicationSpecificCPUCodes.IPProtocolCPUCodeValidConfig = 0x0b008010;

        }/*end of unit applicationSpecificCPUCodes */


        {/*start of unit CPUCodeTable */
            regAddrDbPtr->EQ.CPUCodeTable.CPUCodeAccessCtrl = 0x0b000030;
            regAddrDbPtr->EQ.CPUCodeTable.CPUCodeTableDataAccess = 0x0b003000;

        }/*end of unit CPUCodeTable */


        {/*start of unit CPUTargetDeviceConfig */
            regAddrDbPtr->EQ.CPUTargetDeviceConfig.CPUTargetDeviceConfigReg0 = 0x0b000010;
            regAddrDbPtr->EQ.CPUTargetDeviceConfig.CPUTargetDeviceConfigReg1 = 0x0b000014;

        }/*end of unit CPUTargetDeviceConfig */


        {/*start of unit ingrDropCodesCntr */
            regAddrDbPtr->EQ.ingrDropCodesCntr.dropCodesCntrConfig = 0x0b0000a4;
            regAddrDbPtr->EQ.ingrDropCodesCntr.dropCodesCntr = 0x0b0000a8;

        }/*end of unit ingrDropCodesCntr */


        {/*start of unit ingrDropCntr */
            regAddrDbPtr->EQ.ingrDropCntr.ingrDropCntrConfig = 0x0b00003c;
            regAddrDbPtr->EQ.ingrDropCntr.ingrDropCntr = 0x0b000040;

        }/*end of unit ingrDropCntr */


        {/*start of unit ingrForwardingRestrictions */
            regAddrDbPtr->EQ.ingrForwardingRestrictions.toCpuIngrForwardingRestrictionsConfig = 0x0b020000;
            regAddrDbPtr->EQ.ingrForwardingRestrictions.TONETWORKIngrForwardingRestrictionsConfig = 0x0b020004;
            regAddrDbPtr->EQ.ingrForwardingRestrictions.toAnalyzerIngrForwardingRestrictionsConfig = 0x0b020008;
            regAddrDbPtr->EQ.ingrForwardingRestrictions.ingrForwardingRestrictionsDroppedPktsCntr = 0x0b02000c;

        }/*end of unit ingrForwardingRestrictions */


        {/*start of unit ingrSTCConfig */
            regAddrDbPtr->EQ.ingrSTCConfig.ingrSTCConfig = 0x0b00001c;

        }/*end of unit ingrSTCConfig */


        {/*start of unit ingrSTCInterrupt */
            regAddrDbPtr->EQ.ingrSTCInterrupt.ingrSTCInterruptCause = 0x0b000020;
            regAddrDbPtr->EQ.ingrSTCInterrupt.ingrSTCInterruptMask = 0x0b000024;

        }/*end of unit ingrSTCInterrupt */


        {/*start of unit ingrSTCTable */
            regAddrDbPtr->EQ.ingrSTCTable.ingrSTCTableAccessCtrl = 0x0b000038;
            regAddrDbPtr->EQ.ingrSTCTable.ingrSTCTableWord0Access = 0x0b005000;
            regAddrDbPtr->EQ.ingrSTCTable.ingrSTCTableWord1Read = 0x0b005004;
            regAddrDbPtr->EQ.ingrSTCTable.ingrSTCTableWord2Access = 0x0b005008;

        }/*end of unit ingrSTCTable */


        {/*start of unit logicalTargetMapTable */
            regAddrDbPtr->EQ.logicalTargetMapTable.logicalTargetMapTableAccessCtrl = 0x0b0000a0;
            regAddrDbPtr->EQ.logicalTargetMapTable.logicalTargetMapTableDataAccess = 0x0b00a000;

        }/*end of unit logicalTargetMapTable */


        {/*start of unit mirrToAnalyzerPortConfigs */
            regAddrDbPtr->EQ.mirrToAnalyzerPortConfigs.ingrStatisticMirrToAnalyzerPortConfig = 0x0b000000;
            regAddrDbPtr->EQ.mirrToAnalyzerPortConfigs.ingrAndEgrMonitoringToAnalyzerQoSConfig = 0x0b000004;
            {/*0xb00000c+r*0x64*/
                GT_U32    r;
                for(r = 0 ; r <= 3 ; r++) {
                    regAddrDbPtr->EQ.mirrToAnalyzerPortConfigs.egrMonitoringEnableConfig[r] =
                        0xb00000c+r*0x64;
                        /*0x0b00000c,0x0b000070,0x0b0000d4,0x0b000138*/
                }/* end of loop r */
            }/*0xb00000c+r*0x64*/

            regAddrDbPtr->EQ.mirrToAnalyzerPortConfigs.cascadeEgressMonitoringEnableConfiguration = 0xB000074;

            regAddrDbPtr->EQ.mirrToAnalyzerPortConfigs.portIngrMirrorIndex[0] = 0x0b00b000;
            regAddrDbPtr->EQ.mirrToAnalyzerPortConfigs.portIngrMirrorIndex[1] = 0x0b00b004;
            regAddrDbPtr->EQ.mirrToAnalyzerPortConfigs.portIngrMirrorIndex[2] = 0x0b00b008;

            regAddrDbPtr->EQ.mirrToAnalyzerPortConfigs.portEgrMirrorIndex[0][0] = 0x0b00b010;
            regAddrDbPtr->EQ.mirrToAnalyzerPortConfigs.portEgrMirrorIndex[0][1] = 0x0b00b014;
            regAddrDbPtr->EQ.mirrToAnalyzerPortConfigs.portEgrMirrorIndex[0][2] = 0x0b00b018;
            {/*0xb00b020+i*4*/
                GT_U32  i;
                for(i = 0; i <= 6; i++) {
                    regAddrDbPtr->EQ.mirrToAnalyzerPortConfigs.mirrorInterfaceParameterReg[i] =
                        0xb00b020+i*4;
                }/* end of loop i */
            }/*0xb00b020+i*4*/


            regAddrDbPtr->EQ.mirrToAnalyzerPortConfigs.analyzerPortGlobalConfig = 0x0b00b040;
            regAddrDbPtr->EQ.mirrToAnalyzerPortConfigs.portEgrMirrorIndex[0][3] = 0x0b00b044;
            regAddrDbPtr->EQ.mirrToAnalyzerPortConfigs.portEgrMirrorIndex[0][4] = 0x0b00b048;
            regAddrDbPtr->EQ.mirrToAnalyzerPortConfigs.portEgrMirrorIndex[0][5] = 0x0b00b04c;
            regAddrDbPtr->EQ.mirrToAnalyzerPortConfigs.portEgrMirrorIndex[0][6] = 0x0b00b070;
            {/*0xb00b074+(reg-7)*0x4*/
                GT_U32    reg;
                for(reg = 7; reg <= 13; reg++) {
                    regAddrDbPtr->EQ.mirrToAnalyzerPortConfigs.portEgrMirrorIndex[1][reg-7] =
                        0xb00b074+(reg-7)*0x4;
                }/* end of loop reg */
            }/*0xb00b074+(reg-7)*0x4*/

        }/*end of unit mirrToAnalyzerPortConfigs */


        {/*start of unit numberOfTrunkMembersTable */
            {/*0xb009000+e*0x4*/
                GT_U32    e;
                for(e = 0 ; e <= 15 ; e++) {
                    regAddrDbPtr->EQ.numberOfTrunkMembersTable.numberOfTrunkMembersTableEntry[e] =
                        0xb009000+e*0x4;
                }/* end of loop e */
            }/*0xb009000+e*0x4*/

        }/*end of unit numberOfTrunkMembersTable */


        {/*start of unit preEgrEngineGlobalConfig */
            regAddrDbPtr->EQ.preEgrEngineGlobalConfig.duplicationOfPktsToCPUConfig = 0x0b000018;
            regAddrDbPtr->EQ.preEgrEngineGlobalConfig.dualDeviceIDAndLossyDropConfig = 0x0b0000ac;
            regAddrDbPtr->EQ.preEgrEngineGlobalConfig.preEgrEngineGlobalConfig = 0x0b050000;
            regAddrDbPtr->EQ.preEgrEngineGlobalConfig.logicalTargetDeviceMapConfig = 0x0b050004;

        }/*end of unit preEgrEngineGlobalConfig */


        {/*start of unit preEgrEngineInternal */
            {/*0xb000044+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 4 ; n++) {
                    regAddrDbPtr->EQ.preEgrEngineInternal.preEgrEngineOutGoingDescWord[n] =
                        0xb000044+n*0x4;
                }/* end of loop n */
            }/*0xb000044+n*0x4*/


            {/*0xb00b050+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->EQ.preEgrEngineInternal.preEgrEngineOutGoingDescWord[n] =
                        0xb00b050+n*0x4;
                }/* end of loop n */
            }/*0xb00b050+n*0x4*/



        }/*end of unit preEgrEngineInternal */


        {/*start of unit preEgrInterrupt */
            regAddrDbPtr->EQ.preEgrInterrupt.preEgrInterruptSummary = 0x0b000058;
            regAddrDbPtr->EQ.preEgrInterrupt.preEgrInterruptSummaryMask = 0x0b00005c;

        }/*end of unit preEgrInterrupt */


        {/*start of unit qoSProfileToQoSTable */
            regAddrDbPtr->EQ.qoSProfileToQoSTable.qoSProfileToQoSAccessCtrl = 0x0b00002c;
            regAddrDbPtr->EQ.qoSProfileToQoSTable.qoSProfileToQoSTableDataAccess = 0x0b002000;

        }/*end of unit qoSProfileToQoSTable */


        {/*start of unit statisticalRateLimitsTable */
            regAddrDbPtr->EQ.statisticalRateLimitsTable.statisticalRateLimitsTableAccessCtrl = 0x0b000034;
            regAddrDbPtr->EQ.statisticalRateLimitsTable.statisticalRateLimitsTableDataAccess = 0x0b004000;

        }/*end of unit statisticalRateLimitsTable */


        {/*start of unit STCRateLimitersInterrupt */
            regAddrDbPtr->EQ.STCRateLimitersInterrupt.CPUCodeRateLimitersInterruptCause = 0x0b000060;
            regAddrDbPtr->EQ.STCRateLimitersInterrupt.CPUCodeRateLimitersInterruptMask = 0x0b000064;

        }/*end of unit STCRateLimitersInterrupt */


        {/*start of unit toCpuPktRateLimiters */
            regAddrDbPtr->EQ.toCpuPktRateLimiters.CPUCodeRateLimiterDropCntr = 0x0b000068;
            {/*0xb080000+n*0x4*/
                GT_U32    n;
                for(n = 1 ; n <= 31 ; n++) {
                    regAddrDbPtr->EQ.toCpuPktRateLimiters.toCpuRateLimiterPktCntr[n-1] =
                        0xb080000+n*0x4;
                }/* end of loop n */
            }/*0xb080000+n*0x4*/



        }/*end of unit toCpuPktRateLimiters */


        {/*start of unit trunksMembersTable */
            regAddrDbPtr->EQ.trunksMembersTable.trunkTableAccessCtrl = 0x0b000028;
            regAddrDbPtr->EQ.trunksMembersTable.trunkTableDataAccess = 0x0b001000;

        }/*end of unit trunksMembersTable */
    }/*end of unit EQ - EQ  */

    if(!SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {/*start of unit IPCL - IPCL  */
        regAddrDbPtr->IPCL.ingrPolicyGlobalConfig = 0x0b800000;
        regAddrDbPtr->IPCL.policyEngineInterruptCause = 0x0b800004;
        regAddrDbPtr->IPCL.policyEngineInterruptMask = 0x0b800008;
        regAddrDbPtr->IPCL.policyEngineConfig = 0x0b80000c;
        regAddrDbPtr->IPCL.policyEngineUserDefinedBytesConfig = 0x0b800014;
        regAddrDbPtr->IPCL.CRCHashConfig = 0x0b800020;
        regAddrDbPtr->IPCL.policyEngineUserDefinedBytesExtConfig = 0x0b800024;
        regAddrDbPtr->IPCL.PCLUnitInterruptSummaryCause = 0x0b800060;
        regAddrDbPtr->IPCL.PCLUnitInterruptSummaryMask = 0x0b800064;
        {/*0xb800080+4*i*/
            GT_U32    i;
            for(i = 0 ; i <= 1 ; i++) {
                regAddrDbPtr->IPCL.L2L3IngrVLANCountingEnable[i] =
                    0xb800080+4*i;
            }/* end of loop i */
        }/*0xb800080+4*i*/


        regAddrDbPtr->IPCL.countingModeConfig = 0x0b800088;
        {/*0xb8005c0+4*i*/
            GT_U32    i;
            for(i = 0 ; i <= 15 ; i++) {
                regAddrDbPtr->IPCL.pearsonHashTable[i] =
                    0xb8005c0+4*i;
            }/* end of loop i */
        }/*0xb8005c0+4*i*/
    }/*end of unit IPCL - IPCL  */

    if(!SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        /*start of unit IPLR0,1, EPLR  - added manually  */
        GT_U32 cycle;
        for(cycle = 0; cycle < 3; cycle++)
        {
            regAddrDbPtr->PLR[cycle].policerCtrl0 =
                SMEM_XCAT_POLICER_OFFSET_MAC(devObjPtr, cycle, 0x00000000);
            regAddrDbPtr->PLR[cycle].policerCtrl1 =
                SMEM_XCAT_POLICER_OFFSET_MAC(devObjPtr, cycle, 0x00000004);
            regAddrDbPtr->PLR[cycle].policerPortMetering0 =
                SMEM_XCAT_POLICER_OFFSET_MAC(devObjPtr, cycle, 0x00000008);
            regAddrDbPtr->PLR[cycle].policerPortMetering1 =
                SMEM_XCAT_POLICER_OFFSET_MAC(devObjPtr, cycle, 0x0000000c);
            regAddrDbPtr->PLR[cycle].policerMRU =
                SMEM_XCAT_POLICER_OFFSET_MAC(devObjPtr, cycle, 0x00000010);
            regAddrDbPtr->PLR[cycle].IPFIXCtrl =
                SMEM_XCAT_POLICER_OFFSET_MAC(devObjPtr, cycle, 0x00000014);
            regAddrDbPtr->PLR[cycle].IPFIXNanoTimerStampUpload =
                SMEM_XCAT_POLICER_OFFSET_MAC(devObjPtr, cycle, 0x00000018);
            regAddrDbPtr->PLR[cycle].IPFIXSecondsLSbTimerStampUpload =
                SMEM_XCAT_POLICER_OFFSET_MAC(devObjPtr, cycle, 0x0000001c);
            regAddrDbPtr->PLR[cycle].IPFIXSecondsMSbTimerStampUpload =
                SMEM_XCAT_POLICER_OFFSET_MAC(devObjPtr, cycle, 0x00000020);
            regAddrDbPtr->PLR[cycle].hierarchicalPolicerCtrl =
                SMEM_XCAT_POLICER_OFFSET_MAC(devObjPtr, cycle, 0x0000002c);
            regAddrDbPtr->PLR[cycle].policerIPFIXDroppedPktCountWAThreshold =
                SMEM_XCAT_POLICER_OFFSET_MAC(devObjPtr, cycle, 0x00000030);
            regAddrDbPtr->PLR[cycle].policerIPFIXPktCountWAThreshold =
                SMEM_XCAT_POLICER_OFFSET_MAC(devObjPtr, cycle, 0x00000034);
            regAddrDbPtr->PLR[cycle].policerIPFIXBytecountWAThresholdLSB =
                SMEM_XCAT_POLICER_OFFSET_MAC(devObjPtr, cycle, 0x00000038);
            regAddrDbPtr->PLR[cycle].policerIPFIXBytecountWAThresholdMSB =
                SMEM_XCAT_POLICER_OFFSET_MAC(devObjPtr, cycle, 0x0000003c);
            regAddrDbPtr->PLR[cycle].IPFIXSampleEntriesLog0 =
                SMEM_XCAT_POLICER_OFFSET_MAC(devObjPtr, cycle, 0x00000048);
            regAddrDbPtr->PLR[cycle].IPFIXSampleEntriesLog1 =
                SMEM_XCAT_POLICER_OFFSET_MAC(devObjPtr, cycle, 0x0000004c);
            regAddrDbPtr->PLR[cycle].policerError =
                SMEM_XCAT_POLICER_OFFSET_MAC(devObjPtr, cycle, 0x00000050);
            regAddrDbPtr->PLR[cycle].policerErrorCntr =
                SMEM_XCAT_POLICER_OFFSET_MAC(devObjPtr, cycle, 0x00000054);
            regAddrDbPtr->PLR[cycle].policerTableAccessCtrl =
                SMEM_XCAT_POLICER_OFFSET_MAC(devObjPtr, cycle, 0x00000070);
            {
                GT_U32    n;
                for(n = 0 ; n <= 7 ; n++) {
                    regAddrDbPtr->PLR[cycle].policerTableAccessData[n] =
                        SMEM_XCAT_POLICER_OFFSET_MAC(devObjPtr, cycle, 0x00000074+n*4);
                }/* end of loop n */
            }/*0xc800074+n*4*/


            {/*0xc8000c0+n*4*/
                GT_U32    n;
                for(n = 0 ; n <= 7 ; n++) {
                    regAddrDbPtr->PLR[cycle].policerInitialDP[n] =
                        SMEM_XCAT_POLICER_OFFSET_MAC(devObjPtr, cycle, 0x000000c0+n*4);
                }/* end of loop n */
            }/*0xc8000c0+n*4*/


            regAddrDbPtr->PLR[cycle].policerInterruptCause =
                SMEM_XCAT_POLICER_OFFSET_MAC(devObjPtr, cycle, 0x00000100);
            regAddrDbPtr->PLR[cycle].policerInterruptMask =
                SMEM_XCAT_POLICER_OFFSET_MAC(devObjPtr, cycle, 0x00000104);
            {/*0xc800108+n*4*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->PLR[cycle].policerShadow[n] =
                        SMEM_XCAT_POLICER_OFFSET_MAC(devObjPtr, cycle, 0x00000108+n*4);
                }/* end of loop n */
            }/*0xc800108+n*4*/


            {/*0xc801800+p*4*/
                GT_U32    p;
                for(p = 0 ; p <= 31 ; p++) {
                    regAddrDbPtr->PLR[cycle].portAndPktTypeTranslationTable[p] =
                        SMEM_XCAT_POLICER_OFFSET_MAC(devObjPtr, cycle, 0x00001800+p*4);
                }/* end of loop p */
            }/*0xc801800+p*4*/
        }/*end of unit IPLR0,1, EPLR  - added manually */
    }


    if(!SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {/*start of unit MLL - MLL  */
        {/*start of unit MLLGlobalCtrl */
            regAddrDbPtr->MLL.MLLGlobalCtrl.MLLGlobalCtrl = 0x0c800000;

        }/*end of unit MLLGlobalCtrl */


        {/*start of unit MLLOutInterfaceCntrs */
            {/*0xc800900+n*0x100*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->MLL.MLLOutInterfaceCntrs.MLLOutMcPktsCntr[n] =
                        0xc800900+n*0x100;
                }/* end of loop n */
            }/*0xc800900+n*0x100*/


            {/*0xc800980+n*0x100*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->MLL.MLLOutInterfaceCntrs.MLLOutInterfaceCntrConfig[n] =
                        0xc800980+n*0x100;
                }/* end of loop n */
            }/*0xc800980+n*0x100*/


            regAddrDbPtr->MLL.MLLOutInterfaceCntrs.MLLMCFIFODropCntr = 0x0c800984;

        }/*end of unit MLLOutInterfaceCntrs */


        {/*start of unit multiTargetTCQueuesAndArbitrationConfig */
            regAddrDbPtr->MLL.multiTargetTCQueuesAndArbitrationConfig.multiTargetTCQueuesGlobalConfig = 0x0c800004;
            regAddrDbPtr->MLL.multiTargetTCQueuesAndArbitrationConfig.multitargetTCQueuesStrictPriorityEnableConfig = 0x0c800200;
            regAddrDbPtr->MLL.multiTargetTCQueuesAndArbitrationConfig.multitargetTCQueuesWeightConfig = 0x0c800204;

        }/*end of unit multiTargetTCQueuesAndArbitrationConfig */


        {/*start of unit multiTargetVsUcSDWRRAndStrictPriorityScheduler */
            regAddrDbPtr->MLL.multiTargetVsUcSDWRRAndStrictPriorityScheduler.mcUcSDWRRAndStrictPriorityConfig = 0x0c800214;

        }/*end of unit multiTargetVsUcSDWRRAndStrictPriorityScheduler */


        {/*start of unit multiTargetRateShape */
            regAddrDbPtr->MLL.multiTargetRateShape.multiTargetRateShapeConfig = 0x0c800210;

        }/*end of unit multiTargetRateShape */


        {/*start of unit routerMcLinkedListMLLTables */
            {/*0xc800100+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 7 ; n++) {
                    regAddrDbPtr->MLL.routerMcLinkedListMLLTables.qoSProfileToMultiTargetTCQueuesReg[n] =
                        0xc800100+n*0x4;
                }/* end of loop n */
            }/*0xc800100+n*0x4*/



        }/*end of unit routerMcLinkedListMLLTables */
    }/*end of unit MLL - MLL  */

    if(!devObjPtr->supportEArch)
    {
    {/*start of unit IPCL_TCC - IPCL_TCC  */
        {/*start of unit actionTableAndPolicyTCAMAccessCtrl */
            {/*0xd000100+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 11 ; n++) {
                    regAddrDbPtr->TCCLowerIPCL.TCCIPCL.actionTableAndPolicyTCAMAccessData[n] =
                        0xd000100+n*0x4;
                }/* end of loop n */
            }/*0xd000100+n*0x4*/


            regAddrDbPtr->TCCLowerIPCL.TCCIPCL.actionTableAndPolicyTCAMAccessCtrlParameters1 = 0x0d000130;
            regAddrDbPtr->TCCLowerIPCL.TCCIPCL.actionTableAndPolicyTCAMAccessCtrlParameters2 = 0x0d000134;
            regAddrDbPtr->TCCLowerIPCL.TCCIPCL.actionTableAndPolicyTCAMAccessCtrl = 0x0d000138;

        }/*end of unit actionTableAndPolicyTCAMAccessCtrl */


        {/*start of unit activePowerSave */
            {/*0xd000304+8*p*/
                GT_U32    p;
                for(p = 0 ; p <= 2 ; p++) {
                    regAddrDbPtr->TCCLowerIPCL.TCCIPCL.IPCLKeyType2LookupIDReg0[p] =
                        0xd000304+8*p;
                }/* end of loop p */
            }/*0xd000304+8*p*/


            {/*0xd000308+8*p*/
                GT_U32    p;
                for(p = 0 ; p <= 2 ; p++) {
                    regAddrDbPtr->TCCLowerIPCL.TCCIPCL.IPCLKeyType2LookupIDReg1[p] =
                        0xd000308+8*p;
                }/* end of loop p */
            }/*0xd000308+8*p*/


            regAddrDbPtr->TCCLowerIPCL.TCCIPCL.EPCLKeyType2LookupIDReg = 0x0d00031c;

        }/*end of unit activePowerSave */


        {/*start of unit TCCRegs */
            regAddrDbPtr->TCCLowerIPCL.TCCIPCL.TCCECCErrorInformation = 0x0d000170;
            regAddrDbPtr->TCCLowerIPCL.TCCIPCL.TCAMErrorCntr = 0x0d000198;
            regAddrDbPtr->TCCLowerIPCL.TCCIPCL.TCCInterruptCause = 0x0d0001a4;
            regAddrDbPtr->TCCLowerIPCL.TCCIPCL.TCCInterruptMask = 0x0d0001a8;
            regAddrDbPtr->TCCLowerIPCL.TCCIPCL.TCCUnitInterruptSummaryCause = 0x0d0001bc;
            regAddrDbPtr->TCCLowerIPCL.TCCIPCL.TCCUnitInterruptSummaryMask = 0x0d0001c0;

        }/*end of unit TCCRegs */
    }/*end of unit IPCL_TCC - IPCL_TCC  */

    {/*start of unit IPVX_TCC - IPVX_TCC  */
        {/*start of unit activePowerSave */
            regAddrDbPtr->TCCUpperIPvX.TCCIPvX.TTIKeyType2LookupID = 0x0d800080;
            regAddrDbPtr->TCCUpperIPvX.TCCIPvX.routerDIPLookupKeyType2LookupID = 0x0d8000a0;
            regAddrDbPtr->TCCUpperIPvX.TCCIPvX.routerSIPLookupKeyType2LookupID = 0x0d8000a4;

        }/*end of unit activePowerSave */


        {/*start of unit LTTAndRouterTCAMAccessCtrl */
            {/*0xd800400+n*4*/
                GT_U32    n;
                for(n = 0 ; n <= 5 ; n++) {
                    regAddrDbPtr->TCCUpperIPvX.TCCIPvX.routerTCAMAccessDataReg[n] =
                        0xd800400+n*4;
                }/* end of loop n */
            }/*0xd800400+n*4*/


            regAddrDbPtr->TCCUpperIPvX.TCCIPvX.routerTCAMAccessCtrlReg0 = 0x0d800418;
            regAddrDbPtr->TCCUpperIPvX.TCCIPvX.routerTCAMAccessCtrlReg1 = 0x0d80041c;

        }/*end of unit LTTAndRouterTCAMAccessCtrl */


        {/*start of unit TCCRegs */
            regAddrDbPtr->TCCUpperIPvX.TCCIPvX.TCAMErrorCntr = 0x0d800054;
            regAddrDbPtr->TCCUpperIPvX.TCCIPvX.TCCInterruptCause = 0x0d800060;
            regAddrDbPtr->TCCUpperIPvX.TCCIPvX.TCCInterruptMask = 0x0d800064;
            regAddrDbPtr->TCCUpperIPvX.TCCIPvX.TCCUnitInterruptSummaryCause = 0x0d800078;
            regAddrDbPtr->TCCUpperIPvX.TCCIPvX.TCCUnitInterruptSummaryMask = 0x0d80007c;
            regAddrDbPtr->TCCUpperIPvX.TCCIPvX.TCCECCErrorInformation = 0x0d800d78;

        }/*end of unit TCCRegs */
    }/*end of unit IPVX_TCC - IPVX_TCC  */
    }

    if(!SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {/*start of unit EPCL - EPCL  */
        regAddrDbPtr->EPCL.egrPolicyGlobalConfig = 0x0e000000;
        {/*0xe000010+4*w*/
            GT_U32    w;
            for(w = 0 ; w <= 3 ; w++) {
                regAddrDbPtr->EPCL.egrPolicyConfigTableAccessModeConfig[w] =
                    0xe000010+4*w;
            }/* end of loop w */
        }/*0xe000010+4*w*/


        {/*0xe000020+w*4*/
            GT_U32    w;
            for(w = 0 ; w <= 3 ; w++) {
                regAddrDbPtr->EPCL.egrPolicyNonTSDataPktsConfig[w] =
                    0xe000020+w*4;
            }/* end of loop w */
        }/*0xe000020+w*4*/


        {/*0xe000030+4*w*/
            GT_U32    w;
            for(w = 0 ; w <= 3 ; w++) {
                regAddrDbPtr->EPCL.egrPolicyTSDataPktsConfig[w] =
                    0xe000030+4*w;
            }/* end of loop w */
        }/*0xe000030+4*w*/


        {/*0xe000040+4*w*/
            GT_U32    w;
            for(w = 0 ; w <= 3 ; w++) {
                regAddrDbPtr->EPCL.egrPolicyToCpuConfig[w] =
                    0xe000040+4*w;
            }/* end of loop w */
        }/*0xe000040+4*w*/


        {/*0xe000050+4*w*/
            GT_U32    w;
            for(w = 0 ; w <= 3 ; w++) {
                regAddrDbPtr->EPCL.egrPolicyFromCpuDataConfig[w] =
                    0xe000050+4*w;
            }/* end of loop w */
        }/*0xe000050+4*w*/


        {/*0xe000060+w*4*/
            GT_U32    w;
            for(w = 0 ; w <= 3 ; w++) {
                regAddrDbPtr->EPCL.egrPolicyFromCpuCtrlConfig[w] =
                    0xe000060+w*4;
            }/* end of loop w */
        }/*0xe000060+w*4*/


        {/*0xe000070+4*w*/
            GT_U32    w;
            for(w = 0 ; w <= 3 ; w++) {
                regAddrDbPtr->EPCL.egrPolicyToAnalyzerConfig[w] =
                    0xe000070+4*w;
            }/* end of loop w */
        }/*0xe000070+4*w*/


        regAddrDbPtr->EPCL.egrPolicyDIPSolicitationData[0] = 0x0e000080;
        regAddrDbPtr->EPCL.egrPolicyDIPSolicitationData[1] = 0x0e000084;
        regAddrDbPtr->EPCL.egrPolicyDIPSolicitationData[2] = 0x0e000088;
        regAddrDbPtr->EPCL.egrPolicyDIPSolicitationData[3] = 0x0e00008c;
        {/*0xe000090+n*4*/
            GT_U32    n;
            for(n = 0 ; n <= 3 ; n++) {
                regAddrDbPtr->EPCL.egrPolicyDIPSolicitationMask[n] =
                    0xe000090+n*4;
            }/* end of loop n */
        }/*0xe000090+n*4*/


        {/*0xe000100+n*0x4*/
            GT_U32    n;
            for(n = 0 ; n <= 7 ; n++) {
                regAddrDbPtr->EPCL.egrTCPPortRangeComparatorConfig[n] =
                    0xe000100+n*0x4;
            }/* end of loop n */
        }/*0xe000100+n*0x4*/


        {/*0xe000140+n*0x4*/
            GT_U32    n;
            for(n = 0 ; n <= 7 ; n++) {
                regAddrDbPtr->EPCL.egrUDPPortRangeComparatorConfig[n] =
                    0xe000140+n*0x4;
            }/* end of loop n */
        }/*0xe000140+n*0x4*/


        regAddrDbPtr->EPCL.egrPolicerGlobalConfig = 0x0e000200;
        {/*0xe000230+4*w*/
            GT_U32    w;
            for(w = 0 ; w <= 1 ; w++) {
                regAddrDbPtr->EPCL.egrPolicerPortBasedMetering[w] =
                    0xe000230+4*w;
            }/* end of loop w */
        }/*0xe000230+4*w*/
    }/*end of unit EPCL - EPCL  */

    if(!SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {/*start of unit HA - HA  */
        {/*start of unit ETS_IP */
            regAddrDbPtr->HA.ETS.globalConfigs = 0x0e800800;
            regAddrDbPtr->HA.ETS.enableTimestamping = 0x0e800804;
            {/*0xe800808+4*i*/
                GT_U32    i;
                for(i = 0 ; i <= 1 ; i++) {
                    regAddrDbPtr->HA.ETS.timestampingPortEnable[i] =
                        0xe800808+4*i;
                }/* end of loop i */
            }/*0xe800808+4*i*/


            regAddrDbPtr->HA.ETS.TODCntrNanoseconds = 0x0e800810;
            regAddrDbPtr->HA.ETS.TODCntrSeconds[0] = 0x0e800814;
            regAddrDbPtr->HA.ETS.TODCntrSeconds[1] = 0x0e800818;
            regAddrDbPtr->HA.ETS.TODCntrShadowNanoseconds = 0x0e80081c;
            regAddrDbPtr->HA.ETS.TODCntrShadowSeconds[0] = 0x0e800820;
            regAddrDbPtr->HA.ETS.TODCntrShadowSeconds[1] = 0x0e800824;
            regAddrDbPtr->HA.ETS.GTSInterruptCause = 0x0e800828;
            regAddrDbPtr->HA.ETS.GTSInterruptMask = 0x0e80082c;
            regAddrDbPtr->HA.ETS.globalFIFOCurrentEntry[0] = 0x0e800830;
            regAddrDbPtr->HA.ETS.globalFIFOCurrentEntry[1] = 0x0e800834;
            regAddrDbPtr->HA.ETS.overrunStatus = 0x0e800838;
            regAddrDbPtr->HA.ETS.underrunStatus = 0x0e80083c;

        }/*end of unit ETS_IP */


        {/*start of unit HA_IP */
            {/*0xe800004+w*4*/
                GT_U32    w;
                for(w = 0 ; w <= 1 ; w++) {
                    regAddrDbPtr->HA.cascadingAndHeaderInsertionConfig[w] =
                        0xe800004+w*4;
                }/* end of loop w */
            }/*0xe800004+w*4*/


            {/*0xe800020+w*4*/
                GT_U32    w;
                for(w = 0 ; w <= 1 ; w++) {
                    regAddrDbPtr->HA.egrDSATagTypeConfig[w] =
                        0xe800020+w*4;
                }/* end of loop w */
            }/*0xe800020+w*4*/


            regAddrDbPtr->HA.DP2CFITable = 0x0e800070;
            {/*0xe800080+4*w*/
                GT_U32    w;
                for(w = 0 ; w <= 1 ; w++) {
                    regAddrDbPtr->HA.mirrorToAnalyzerHeaderConfig[w] =
                        0xe800080+4*w;
                }/* end of loop w */
            }/*0xe800080+4*w*/


            regAddrDbPtr->HA.haGlobalConfig = 0x0e800100;
            {/*0xe800104+4*w*/
                GT_U32    w;
                for(w = 0 ; w <= 1 ; w++) {
                    regAddrDbPtr->HA.routerHaEnableMACSAModification[w] =
                        0xe800104+4*w;
                }/* end of loop w */
            }/*0xe800104+4*w*/


            regAddrDbPtr->HA.routerMACSABaseReg0 = 0x0e80010c;
            regAddrDbPtr->HA.routerMACSABaseReg1 = 0x0e800110;
            {/*0xe800114+4*w*/
                GT_U32    w;
                for(w = 0 ; w <= 1 ; w++) {
                    regAddrDbPtr->HA.deviceIDModificationEnable[w] =
                        0xe800114+4*w;
                }/* end of loop w */
            }/*0xe800114+4*w*/

            {/*0xe800120+k*0x4*/
                GT_U32    k;
                for(k = 0 ; k <= 3 ; k++) {
                    regAddrDbPtr->HA.routerHaMACSAModificationMode[k] =
                        0xe800120+k*0x4;
                }/* end of loop k */
            }/*0xe800120+k*0x4*/


            {/*0xe800130+4*w*/
                GT_U32    w;
                for(w = 0 ; w <= 3 ; w++) {
                    regAddrDbPtr->HA.haVLANTranslationEnable[w] =
                        0xe800130+4*w;
                }/* end of loop w */
            }/*0xe800130+4*w*/


            {/*0xe800140+4*w*/
                GT_U32    w;
                for(w = 0 ; w <= 1 ; w++) {
                    regAddrDbPtr->HA.coreRingConfig[w] =
                        0xe800140+4*w;
                }/* end of loop w */
            }/*0xe800140+4*w*/


            {/*0xe800150+4*w*/
                GT_U32    w;
                for(w = 0 ; w <= 1 ; w++) {
                    regAddrDbPtr->HA.replaceVIDWithUserID[w] =
                        0xe800150+4*w;
                }/* end of loop w */
            }/*0xe800150+4*w*/

            {/*0xe800160+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 31 ; n++) {
                    regAddrDbPtr->HA.portTrunkNumConfigReg[n] =
                        0xe800160+n*0x4;
                }/* end of loop n */
            }/*0xe800160+n*0x4*/

            {/*0xe800300+4*n*/
                GT_U32    n;
                for(n = 0 ; n <= 15 ; n++) {
                    regAddrDbPtr->HA.qoSProfileToEXPReg[n] =
                        0xe800300+4*n;
                }/* end of loop n */
            }/*0xe800300+4*n*/


            {/*0xe800340+4*n*/
                GT_U32    n;
                for(n = 0 ; n <= 7 ; n++) {
                    regAddrDbPtr->HA.qoSProfileToDPReg[n] =
                        0xe800340+4*n;
                }/* end of loop n */
            }/*0xe800340+4*n*/


            regAddrDbPtr->HA.HAInterruptCause = 0x0e8003a0;
            regAddrDbPtr->HA.HAInterruptMask = 0x0e8003a4;
            regAddrDbPtr->HA.hepInterruptSummaryCause = 0x0e8003a8;
            regAddrDbPtr->HA.hepInterruptSummaryMask = 0x0e8003ac;
            {/*0xe800400+4*w*/
                GT_U32    w;
                for(w = 0 ; w <= 1 ; w++) {
                    regAddrDbPtr->HA.toAnalyzerVLANTagAddEnableConfig[w] =
                        0xe800400+4*w;
                }/* end of loop w */
            }/*0xe800400+4*w*/


            regAddrDbPtr->HA.ingrAnalyzerVLANTagConfig = 0x0e800408;
            regAddrDbPtr->HA.egrAnalyzerVLANTagConfig = 0x0e80040c;
            regAddrDbPtr->HA.congestionNotificationConfig = 0x0e800420;
            regAddrDbPtr->HA.CNMHeaderConfig = 0x0e800424;
            regAddrDbPtr->HA.QCNCNtagFlowID = 0x0e800428;
            regAddrDbPtr->HA.CPIDReg0 = 0x0e800430;
            regAddrDbPtr->HA.CPIDReg1 = 0x0e800434;
            {/*0xe800440+4*p*/
                GT_U32    p;
                for(p = 0 ; p <= 15 ; p++) {
                    regAddrDbPtr->HA.keepVLAN1[p] =
                        0xe800440+4*p;
                }/* end of loop p */
            }/*0xe800440+4*p*/


            {/*0xe800500+n*4*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->HA.TPIDConfigReg[n] =
                        0xe800500+n*4;
                }/* end of loop n */
            }/*0xe800500+n*4*/


            {/*0xe800510+n*4*/
                GT_U32    n;
                for(n = 0 ; n <= 7 ; n++) {
                    regAddrDbPtr->HA.portTag0TPIDSelect[n] =
                        0xe800510+n*4;
                }/* end of loop n */
            }/*0xe800510+n*4*/


            {/*0xe800530+n*4*/
                GT_U32    n;
                for(n = 0 ; n <= 7 ; n++) {
                    regAddrDbPtr->HA.portTag1TPIDSelect[n] =
                        0xe800530+n*4;
                }/* end of loop n */
            }/*0xe800530+n*4*/


            regAddrDbPtr->HA.MPLSEthertype = 0x0e800550;
            regAddrDbPtr->HA.IEthertype = 0x0e800554;
            regAddrDbPtr->HA.ethernetOverGREProtocolType = 0x0e800558;

            {/*0x0E8005D4+n*0x18*/
                GT_U32    n;
                for(n = 0 ; n <= 7 ; n++) {
                    regAddrDbPtr->HA.dsaErspan[n] =
                        0x0e8005d4+n*0x18;
                }/* end of loop n */
            }/*0x0E8005D4+n*0x18*/

            regAddrDbPtr->HA.tunnelStartFragmentIDConfig = 0x0e800410;
        }/*end of unit HA_IP */
    }/*end of unit HA - HA  */

    if(! SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {/*start of unit RXDMA - RXDMA  */
        {/*start of unit arbiterConfig */
            regAddrDbPtr->RXDMA.arbiterConfig.WRDMAPizzaArbiterCtrlReg = 0x0f000000;
            {/*0xf000004+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 7 ; n++) {
                    regAddrDbPtr->RXDMA.arbiterConfig.WRDMAPizzaArbiterSliceMapReg[n] =
                        0xf000004+n*0x4;
                }/* end of loop n */
            }/*0xf000004+n*0x4*/


            regAddrDbPtr->RXDMA.arbiterConfig.WRDMAPizzaArbiterDebugReg = 0x0f000024;
            regAddrDbPtr->RXDMA.arbiterConfig.CTUPizzaArbiterCtrlReg = 0x0f000028;
            {/*0xf00002c+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 7 ; n++) {
                    regAddrDbPtr->RXDMA.arbiterConfig.CTUPizzaArbiterSliceMapReg[n] =
                        0xf00002c+n*0x4;
                }/* end of loop n */
            }/*0xf00002c+n*0x4*/


            regAddrDbPtr->RXDMA.arbiterConfig.CTUPizzaArbiterDebugReg = 0x0f00004c;

        }/*end of unit arbiterConfig */


        {/*start of unit cutThrough */
            regAddrDbPtr->RXDMA.cutThrough.CTFeatureEnablePerPort = 0x0f000050;
            regAddrDbPtr->RXDMA.cutThrough.CTEtherTypeIdentification = 0x0f000054;
            regAddrDbPtr->RXDMA.cutThrough.CTUPEnable = 0x0f000058;
            regAddrDbPtr->RXDMA.cutThrough.CTPktIndentification = 0x0f00005c;
            regAddrDbPtr->RXDMA.cutThrough.CTUParameters = 0x0f000060;
            regAddrDbPtr->RXDMA.cutThrough.cascadingPort = 0x0f000064;

        }/*end of unit cutThrough */


        {/*start of unit debug */
            regAddrDbPtr->RXDMA.debug.lastDescFromGetByBAWRDMA01 = 0x0f0000a0;

        }/*end of unit debug */


        {/*start of unit iBufConfigs */
            regAddrDbPtr->RXDMA.iBufConfigs.iBufSetThresholdLowPorts = 0x0f0000b0;
            regAddrDbPtr->RXDMA.iBufConfigs.iBufSetThresholdHighPorts = 0x0f0000b4;

        }/*end of unit iBufConfigs */


        {/*start of unit interrupts */
            regAddrDbPtr->RXDMA.interrupts.rxDMAInterruptSummaryCause = 0x0f000070;
            regAddrDbPtr->RXDMA.interrupts.rxDMAInterruptSummaryMask = 0x0f000074;
            regAddrDbPtr->RXDMA.interrupts.rxDMAFunctionalInterruptCause = 0x0f000078;
            regAddrDbPtr->RXDMA.interrupts.rxDMAFunctionalInterruptMask = 0x0f00007c;

        }/*end of unit interrupts */


        {/*start of unit WRDMABAConfigs */
            regAddrDbPtr->RXDMA.WRDMABAConfigs.lowDMAIFConfig = 0x0f000090;
            regAddrDbPtr->RXDMA.WRDMABAConfigs.highDMAIFConfig = 0x0f000094;
            regAddrDbPtr->RXDMA.WRDMABAConfigs.BADebugHook = 0x0f000098;

        }/*end of unit WRDMABAConfigs */



    }/*end of unit RXDMA - RXDMA  */

    {/*start of unit TXDMA - TXDMA  */
        {/*start of unit burstSizeLimit */
            regAddrDbPtr->TXDMA.burstSizeLimit.burstLimitGlobalConfig = 0x0F8000C4;
            {/*0xf8000b4+p*4*/
                GT_U32    p;
                for(p = 0 ; p <= 12 ; p++) {
                    regAddrDbPtr->TXDMA.burstSizeLimit.portBurstLimitThresholds[p] =
                        0x0F8000C8+p*4;
                }/* end of loop p */
            }/*0xf8000b4+p*4*/



        }/*end of unit burstSizeLimit */


        {/*start of unit cutThrough */
            {/*0xf800080+p*4*/
                GT_U32    p;
                for(p = 0 ; p <= 5 ; p++) {
                    regAddrDbPtr->TXDMA.cutThrough.port_MemoryRateLimitThreshold[p] =
                        0xf800080+p*4;
                }/* end of loop p */
            }/*0xf800080+p*4*/


            regAddrDbPtr->TXDMA.cutThrough.cpuPortMemoryRateLimitThreshold = 0x0f800098;

        }/*end of unit cutThrough */


        {/*start of unit interrupts */
            regAddrDbPtr->TXDMA.interrupts.txDMAInterruptSummary = 0x0f800130;
            regAddrDbPtr->TXDMA.interrupts.txDMAInterruptCause = 0x0f800134;
            regAddrDbPtr->TXDMA.interrupts.txDMAInterruptMask = 0x0f800138;
            regAddrDbPtr->TXDMA.interrupts.dataErrorInterruptCause = 0x0f80013c;
            regAddrDbPtr->TXDMA.interrupts.dataErrorInterruptMask = 0x0f800140;
            {/*0xf800164+n*4*/
                GT_U32    n;
                for(n = 0 ; n <= 6 ; n++) {
                    regAddrDbPtr->TXDMA.interrupts.dataErrorStatusReg[n] =
                        0xf800164+n*4;
                }/* end of loop n */
            }/*0xf800164+n*4*/


            {/*0xf800180+n*4*/
                GT_U32    n;
                for(n = 0 ; n <= 5 ; n++) {
                    regAddrDbPtr->TXDMA.interrupts.lastDescAccess[n] =
                        0xf800180+n*4;
                }/* end of loop n */
            }/*0xf800180+n*4*/



        }/*end of unit interrupts */


        {/*start of unit TXDMAEngineConfig */
            regAddrDbPtr->TXDMA.TXDMAEngineConfig.pizzaArbiterCtrlReg = 0x0f800000;
            {/*0xf800004+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 7 ; n++) {
                    regAddrDbPtr->TXDMA.TXDMAEngineConfig.pizzaArbiterSliceMapReg[n] =
                        0xf800004+n*0x4;
                }/* end of loop n */
            }/*0xf800004+n*0x4*/


            regAddrDbPtr->TXDMA.TXDMAEngineConfig.pizzaArbiterDebugReg = 0x0f800024;
            regAddrDbPtr->TXDMA.TXDMAEngineConfig.bufferMemoryMiscReg = 0x0f800028;
            regAddrDbPtr->TXDMA.TXDMAEngineConfig.bufferMemoryGigFIFOsThreshold = 0x0f80002c;
            regAddrDbPtr->TXDMA.TXDMAEngineConfig.bufferMemoryXGFIFOsThreshold = 0x0f800030;
            regAddrDbPtr->TXDMA.TXDMAEngineConfig.bufferMemoryHGL20GFIFOsThreshold = 0x0f800034;
            regAddrDbPtr->TXDMA.TXDMAEngineConfig.bufferMemoryXLGFIFOsThreshold = 0x0f800038;
            regAddrDbPtr->TXDMA.TXDMAEngineConfig.bufferMemoryCPUFIFOsThreshold = 0x0f80003c;
            regAddrDbPtr->TXDMA.TXDMAEngineConfig.bufferMemoryMiscThresholdsConfig = 0x0f800040;
            regAddrDbPtr->TXDMA.TXDMAEngineConfig.descPrefetchSharing = 0x0f800068;
            regAddrDbPtr->TXDMA.TXDMAEngineConfig.extPortsConfigReg = 0x0f800070;

        }/*end of unit TXDMAEngineConfig */



    }/*end of unit TXDMA - TXDMA  */

    /* not for bobcat2 */
    if(devObjPtr->portGroupSharedDevObjPtr)
    {/*start of unit MPPM1 - MPPM1  */
        {/*start of unit ecc */
            regAddrDbPtr->MPPM1.ecc.eccCtrl = 0x0A000010;

        }/*end of unit ecc */


        regAddrDbPtr->MPPM1.xlgMode = 0x0A000000;
        regAddrDbPtr->MPPM1.interruptCause = 0x0A000004;
        regAddrDbPtr->MPPM1.interruptMask = 0x0A000008;

    }/*end of unit MPPM1 - MPPM1  */


    smemLion2RegsInfoSetPart2(devObjPtr);
}


