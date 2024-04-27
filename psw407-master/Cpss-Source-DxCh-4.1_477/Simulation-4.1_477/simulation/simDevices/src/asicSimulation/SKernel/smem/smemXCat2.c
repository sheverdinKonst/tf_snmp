/******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* smemXCat2.c
*
* DESCRIPTION:
*       xCat2 memory mapping implementation
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 60 $
*
*******************************************************************************/
#include <os/simTypes.h>
#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SKernel/smem/smemXCat2.h>
#include <asicSimulation/SKernel/cheetahCommon/sregCheetah.h>
#include <asicSimulation/SKernel/suserframes/snetCheetah3Pcl.h>

static GT_U32 policerBaseAddr[3] = {0x0c800000,0x0d000000,0x03800000};
/* define policer memory with 28 entries */
#define POLICER_MEMORY_28_CNS         28
/* define policer memory with 84 entries */
#define POLICER_MEMORY_84_CNS         84
/* define policer memory with 172 entries */
#define POLICER_MEMORY_172_CNS        172

#define POLICER_TOTAL_MEMORY_CNS /*284*/ (POLICER_MEMORY_172_CNS+POLICER_MEMORY_84_CNS+POLICER_MEMORY_28_CNS)

static GT_U32 mppmBankBaseAddr[2] = {0x06800000,0x07000000};

/* Timestamp ingress and egress base addresses */
static GT_U32 timestampBaseAddr[2] = {0x0c002000,0x0f000800};

/* Active memory table */
static SMEM_ACTIVE_MEM_ENTRY_STC smemXCat2ActiveTable[] =
{
    {0x00000030, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteIntrCauseReg, 0}, /* Global interrupt cause register */
    {0x00000038, SMEM_FULL_MASK_CNS, smemChtActiveReadIntrCauseReg, 11, smemChtActiveWriteIntrCauseReg, 0}, /* Read interrupts cause registers Misc -- ROC register */
    {0x0000003c, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteMiscInterruptsMaskReg, 0}, /* Write Interrupt Mask Misc Register */
    {0x00000058, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteGlobalReg, 0}, /* Global control register */
    {0x000000C0, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveAuqBaseWrite, 0}, /* Address Update Queue Base Address */
    {0x000000C8, SMEM_FULL_MASK_CNS, NULL, 0 , smemCht2ActiveFuqBaseWrite, 0}, /* FDB Upload Queue Base Address */
    {0x00002680, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteSdmaCommand,0}, /* Receive SDMA Queue Command */
    {0x00002800, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteSdmaConfigReg, 0}, /* SDMA configuration register - 0x00002800 */
    {0x0000280C, SMEM_FULL_MASK_CNS, smemChtActiveReadIntrCauseReg, 22, smemChtActiveWriteIntrCauseReg, 0}, /* Receive SDMA Interrupt Cause Register (RxSDMAInt) */
    {0x00002810, SMEM_FULL_MASK_CNS, smemChtActiveReadIntrCauseReg, 21 , smemChtActiveWriteIntrCauseReg,0}, /* Transmit SDMA Interrupt Cause Register */
    {0x00002814, SMEM_FULL_MASK_CNS, NULL, 0, smemChtActiveWriteRcvSdmaInterruptsMaskReg, 0}, /* Receive SDMA Interrupt Mask Register */
    {0x00002818, SMEM_FULL_MASK_CNS, NULL, 0, smemChtActiveWriteTransSdmaInterruptsMaskReg, 0}, /* Transmit SDMA Interrupt Mask Register */
    {0x00002820, 0xFFFFFFF0, smemChtActiveReadCntrs, 0, NULL,0}, /* The SDMA packet count registers */
    {0x00002830, 0xFFFFFFF0, smemChtActiveReadCntrs, 0, NULL,0},
    {0x00002840, 0xFFFFFFF0, smemChtActiveReadCntrs, 0, NULL,0},  /* The SDMA byte count registers */
    {0x00002850, 0xFFFFFFF0, smemChtActiveReadCntrs, 0, NULL,0},
    {0x00002860, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0}, /* The SDMA resource error registers */
    {0x00002864, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},
    {0x00002868, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteStartFromCpuDma, 0}, /* Transmit SDMA Queue Command Register */
    {0x000F1900, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteIntrCauseReg, 0x00FF1F1F}, /* PEX Interrupt Cause Register */
    {0x02000810, 0xFFFF0FFF, smemXCatActiveReadIeeeMcConfReg, 0 , NULL,  0}, /* IEEE Reserved Multicast Configuration register reading */
    {0x02000818, 0xFFFF0FFF, smemXCatActiveReadIeeeMcConfReg, 0 , NULL,  0},
    {0x02000820, 0xFFFF0FFF, smemXCatActiveReadIeeeMcConfReg, 0 , NULL,  0},
    {0x02000828, 0xFFFF0FFF, smemXCatActiveReadIeeeMcConfReg, 0 , NULL,  0},
    {0x020400BC, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0}, /* Host Incoming Packets Count */
    {0x020400C0, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0}, /* Host Outgoing Packets Count */
    {0x020400CC, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0}, /* Host Outgoing Multicast Packet Count */
    {0x020400D0, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0}, /* Host Outgoing Broadcast Packet Count */
    {0x020400d4, 0xFFFFFFFF, smemChtActiveReadCntrs, 0, NULL, 0}, /* Matrix Bridge Register */
    {0x020400E0, 0xFFFFFFF0, smemChtActiveReadCntrs, 0, NULL,0}, /* Set #0 counters */
    {0x020400F4, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0}, /* Set #1 Incoming counters */
    {0x020400F8, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0}, /* Set #1 VLAN ingress filtered counters */
    {0x020400FC, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0}, /* Set #1 Security filtered counters */
    {0x02040100, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0}, /* Set #1 Bridge filtered counters */
    {0x02040130, SMEM_FULL_MASK_CNS, smemChtActiveReadIntrCauseReg, 14, smemChtActiveWriteIntrCauseReg, 0}, /* Bridge Interrupt Cause Register */
    {0x02040134, SMEM_FULL_MASK_CNS, NULL, 0, smemChtActiveWriteBridgeInterruptsMaskReg, 0}, /* Bridge Interrupt Mask Register */
    {0x020401A8, SMEM_FULL_MASK_CNS, smemChtActiveReadBreachStatusReg, 0, NULL, 0}, /* Security Breach Status Register2 */
    {0x04010000, SMEM_CHT_COUNT_MSK_CNS, smemChtActiveReadCntrs, 0, NULL, 0}, /* The MAC MIB Counters */
    {0x04810000, SMEM_CHT_COUNT_MSK_CNS, smemChtActiveReadCntrs, 1, NULL, 0},
    {0x05010000, SMEM_CHT_COUNT_MSK_CNS, smemChtActiveReadCntrs, 2, NULL, 0},
    {0x05810000, SMEM_CHT_COUNT_MSK_CNS, smemChtActiveReadCntrs, 3, NULL, 0},
    /* Port MIB Counters Capture */
    {0x04004020, SMEM_FULL_MASK_CNS, NULL, 0, smemChtActiveWriteMibCntCtrl, 0},
    {0x04804020, SMEM_FULL_MASK_CNS, NULL, 0, smemChtActiveWriteMibCntCtrl, 1},
    {0x05004020, SMEM_FULL_MASK_CNS, NULL, 0, smemChtActiveWriteMibCntCtrl, 2},
    {0x05804020, SMEM_FULL_MASK_CNS, NULL, 0, smemChtActiveWriteMibCntCtrl, 3},
    {0x06000000, SMEM_FULL_MASK_CNS, NULL, 0 , smemCht3ActiveWriteFDBGlobalCfgReg, 0}, /* FDB Global Configuration register */
    {0x01800000, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteTxQuCtrlReg, 0},/* Transmit Queue Control Register */
    {0x01800080, 0XFFFF01FF, NULL, 0 , smemChtActiveWriteTxQConfigReg, 0}, /* Transmit Queue Config Register */
    {0x01800130, SMEM_FULL_MASK_CNS, smemChtActiveReadIntrCauseReg, 11 , smemChtActiveWriteIntrCauseReg,0},/*Egress STC interrupt register*/
    {0x06000004, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteFdbActionTrigger, 0}, /* MAC Table Action0 Register */
    {0x06000018, SMEM_FULL_MASK_CNS, smemChtActiveReadIntrCauseReg, 17, smemChtActiveWriteIntrCauseReg, 0}, /* FDB Interrupt Cause Register */
    {0x0600001C, SMEM_FULL_MASK_CNS, NULL, 0, smemChtActiveWriteMacInterruptsMaskReg, 0}, /* FDB Table Interrupt Mask Register */
    {0x06000034, SMEM_FULL_MASK_CNS, smemChtActiveReadMsg2Cpu, 0 , NULL,0}, /* Message to CPU register  */
    {0x06000050, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteNewFdbMsgFromCpu, 0}, /* Message From CPU Management */
    {0x06000064, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteFdbMsg,0}, /* Mac Table Access Control Register */
    {0x08000030, SMEM_FULL_MASK_CNS, NULL, 0 , smemCht3ActiveWriteCncFastDumpTrigger, 0}, /* CNC Fast Dump Trigger Register Register */
    {0x08000100, SMEM_FULL_MASK_CNS, smemChtActiveReadIntrCauseReg, 3, smemChtActiveWriteIntrCauseReg, 0}, /* Read interrupts cause registers CNC -- ROC register */
    {0x08000104, SMEM_FULL_MASK_CNS, NULL, 0 , smemCht3ActiveWriteCncInterruptsMaskReg, 0}, /* Write Interrupt Mask CNC Register */
    {0x080010A0, 0xFFFFF0F0, smemCht3ActiveCncWrapAroundStatusRead, 0, NULL, 0}, /* CNC Block Wraparound Status Register */
    {0x08080000, 0xFFF80000, smemCht3ActiveCncBlockRead, 0, NULL, 0}, /* CPU direct read from the counters */
    {0x09000000, 0xFF9FFFFF, smemChtActiveReadCntrs, 0, NULL,0}, /* Stack port MAC MIB Counters */
    {0x09300000, SMEM_CHT_COUNT_MSK_CNS, smemChtActiveReadCntrs, 24, NULL, 0}, /* The MAC MIB Counters of XG ports 24..26 */
    {0x09320000, SMEM_CHT_COUNT_MSK_CNS, smemChtActiveReadCntrs, 25, NULL, 0},
    {0x09340000, SMEM_CHT_COUNT_MSK_CNS, smemChtActiveReadCntrs, 26, NULL, 0},
    {0x09360000, SMEM_CHT_COUNT_MSK_CNS, smemChtActiveReadCntrs, 27, NULL, 0},
    {0x0A80000C, 0xFFFF03FF, NULL, 0 , smemChtActiveWriteForceLinkDown, 0}, /* Port<n> Auto-Negotiation Configuration Register */
    {0x0A80000C, 0xFFFF20FF, NULL, 0 , smemChtActiveWriteForceLinkDown, 0}, /* Port<n> Auto-Negotiation Configuration Register */
    {0x0A80000C, 0xFFFF03FF, NULL, 0 , smemChtActiveWriteForceLinkDown, 0}, /* Stack gig ports - Port<n> Auto-Negotiation Configuration Register */
    {0x0A800020, SMEM_CHT_GOPINT_MSK_CNS, smemChtActiveReadIntrCauseReg, 18, smemChtActiveWriteIntrCauseReg, 0}, /* Port<n> Interrupt Cause Register  */
    {0x0A800020, 0xFFFF03FF,smemChtActiveReadIntrCauseReg, 18, smemChtActiveWriteIntrCauseReg, 0}, /* Stack gig ports - Port<n> Interrupt Cause Register  */
    {0x0A800024, SMEM_CHT_GOPINT_MSK_CNS, NULL, 0, smemChtActiveWritePortInterruptsMaskReg, 0}, /* Tri-Speed Port<n> Interrupt Mask Register */
    {0x0A800024, 0xFFFF03FF, NULL, 0, smemChtActiveWritePortInterruptsMaskReg, 0}, /* Stack gig ports - Tri-Speed Port<n> Interrupt Mask Register */
    {0x0A800044, 0xFFFF03FF, NULL, 0, smemChtActiveWriteHgsMibCntCtrl, 0}, /* Stack port MIB Counters Control Register */
    {0x0B000028, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteTrunkTbl, 0}, /* Trunk table Access Control Register */
    {0x0B00002C, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteQosAccess, 0}, /* QoSProfile to QoS Access Control Register */
    {0x0B000030, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteCpuCodeAccess, 0}, /* CPU Code Access Control Register */
    {0x0B000034, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteStatRateLimTbl, 0}, /* Statistical Rate Limits Table Access Control Register */
    {0x0B000038, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteIngrStcTbl, 0}, /* Ingress STC Table Access Control Register */
    {0x0B000020, SMEM_FULL_MASK_CNS, smemChtActiveReadIntrCauseReg, 24 , smemChtActiveWriteIntrCauseReg,0},/*ingress STC interrupt register*/
    {0x0B0000A0, SMEM_FULL_MASK_CNS, NULL, 0 , smemXCatActiveWriteLogTargetMap,  0},
    {0x0E000138, SMEM_FULL_MASK_CNS, NULL, 0 , smemCht3ActiveWritePclAction, 0},
    {0x0E000208, SMEM_FULL_MASK_CNS, NULL, 0 , smemXCatActiveWritePolicyTcamConfig_0,  0},
    {0x0E80041C, SMEM_FULL_MASK_CNS, NULL, 0 , smemCht3ActiveWriteRouterAction, 0}, /* Router and TT TCAM Access Control 1 Register  */

    POLICER_ACTIVE_WRITE_MEMORY(0x00000070, smemXCatActiveWritePolicerTbl), /* Policers Table Access Control Register */
    POLICER_ACTIVE_WRITE_MEMORY(0x00000014, smemXCatActiveWriteIPFixTimeStamp), /* Policer IPFIX memories */

    POLICER_ACTIVE_READ_MEMORY(0x00000218, smemXCatActiveReadIPFixNanoTimeStamp),  /*part of Policer Timer Memory */
    POLICER_ACTIVE_READ_MEMORY(0x0000021C, smemXCatActiveReadIPFixSecLsbTimeStamp),/*part of Policer Timer Memory */
    POLICER_ACTIVE_READ_MEMORY(0x00000220, smemXCatActiveReadIPFixSecMsbTimeStamp),/*part of Policer Timer Memory */
    POLICER_ACTIVE_READ_MEMORY(0x00000048, smemXCatActiveReadIPFixSampleLog),
    POLICER_ACTIVE_READ_MEMORY(0x0000004C, smemXCatActiveReadIPFixSampleLog),
    POLICER_ACTIVE_READ_MEMORY(POLICER_MANAGEMENT_COUNTER_ADDR_CNS, smemXCatActiveReadPolicerManagementCounters), /* Policer : when Read a Data Unit counter part(Entry LSB) of Management Counters Entry, then value of LSB and MSB copied to Shadow registers */

    /* Policer: when write iplr0 Hierarchical Policer control register, "Policer memory control" configuration is
        changed ang ingress policer tables are redistributed between plr0,1 accordingly */
    {0x0C80002C, SMEM_FULL_MASK_CNS, NULL, 0 , smemXCat2ActiveWritePolicerMemoryControl,  0},

    /* iplr0 policer table 0x0c840000 - 0x0c84237F  xCat2 */
    {0x0c840000, 0xFFFFC000, smemXCatActiveReadIplr0Tables, 0 , smemXCatActiveWriteIplr0Tables,  0},
    /* iplr0 policerCounters table 0x0c860000 - 0x0c86237F  xCat2 */
    {0x0c860000, 0xFFFFC000, smemXCatActiveReadIplr0Tables, 0 , smemXCatActiveWriteIplr0Tables,  0},

    /* iplr1 policer table 0x0D040000 - 0x0D04237F  xCat2 */
    {0x0D040000, 0xFFFFC000, smemXCatActiveReadIplr1Tables, 0 , smemXCatActiveWriteIplr1Tables,  0},
    /* iplr1 policerCounters table 0x0D060000 - 0x0D06237F  xCat2 */
    {0x0D060000, 0xFFFFC000, smemXCatActiveReadIplr1Tables, 0 , smemXCatActiveWriteIplr1Tables,  0},

    GTS_ACTIVE_READ_MEMORY(0x10, smemLionActiveReadPtpTodNanoSeconds), /* TOD Counter - Nanoseconds */
    GTS_ACTIVE_READ_MEMORY(0x14, smemLionActiveReadPtpTodSeconds), /* TOD Counter - Seconds word 0)*/
    GTS_ACTIVE_READ_MEMORY(0x18, smemLionActiveReadPtpTodSeconds), /* TOD Counter - Seconds word 1*/
    GTS_ACTIVE_READ_MEMORY(0x30, smemLionActiveReadPtpMessage), /* Global FIFO Current Entry [31:0] register reading*/
    GTS_ACTIVE_READ_MEMORY(0x34, smemLionActiveReadPtpMessage), /* Global FIFO Current Entry [63:32] register reading */
    GTS_ACTIVE_WRITE_MEMORY(0x0, smemLionActiveWriteTodGlobalReg), /* Global Configurations */

     /* The Egress packet count registers */
    {0x01B40144, 0xFFFFFFFF, smemChtActiveReadCntrs, 0, NULL,0},
    {0x01B40148, 0xFFFFFFFF, smemChtActiveReadCntrs, 0, NULL,0},
    {0x01B4014C, 0xFFFFFFFF, smemChtActiveReadCntrs, 0, NULL,0},
    {0x01B40150, 0xFFFFFFF0, smemChtActiveReadCntrs, 0, NULL,0},
    {0x01B40160, 0xFFFFFFF0, smemChtActiveReadCntrs, 0, NULL,0},
    {0x01B40170, 0xFFFFFFF0, smemChtActiveReadCntrs, 0, NULL,0},
    {0x01B40180, 0xFFFFFFFF, smemChtActiveReadCntrs, 0, NULL,0},
    {0x01B40184, 0xFFFFFFFF, smemChtActiveReadCntrs, 0, NULL,0},

    /* Port<%n> Rate Limit Counter */
    {0x02000400, 0XFFFE0FFF,
        smemChtActiveReadRateLimitCntReg, 0 , smemChtActiveWriteToReadOnlyReg, 0},

    /* SMI0 Management Register : 0x04004054 , 0x05004054 */
    {0x04004054, 0xFEFFFFFF, NULL, 0 , smemChtActiveWriteSmi,0},

    /* XSMI Management Register */
    {0x01CC0000, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteXSmii, 0},

    /* CPU Port MIB counters registers */
    {0x00000060, 0xFFFFFFE0, smemChtActiveReadCntrs, 0, NULL,0},

    /* must be last anyway */
    {END_OF_TABLE, SMEM_FULL_MASK_CNS, NULL,0,NULL,0}
};

#define XCAT2_IP_ROUT_TCAM_CNS   8*1024

static SKERNEL_DEVICE_ROUTE_TCAM_INFO_STC  xcat2RoutTcamInfo =
{
    XCAT2_IP_ROUT_TCAM_CNS,
    XCAT2_IP_ROUT_TCAM_CNS / 4,
    1,
    4,
    16
};

static GT_U32   smemXCat2ActiveTableSizeOf =
    sizeof(smemXCat2ActiveTable)/sizeof(smemXCat2ActiveTable[0]);


/* the units that the xCat2 use */
static GT_U32   usedUnitsNumbersArray[] =
{
/*    SMEM_XCAT2_UNIT_MG_E,               0x00000000 */   0,
/*    SMEM_XCAT2_UNIT_DFX_E,              0x00800000 */   1,
/*    SMEM_XCAT2_UNIT_EGR_TXQ_E,          0x01800000 */   3,
/*    SMEM_XCAT2_UNIT_L2I_E,              0x02000000 */   4,
/*    SMEM_XCAT2_UNIT_UC_ROUTE_E,         0x02800000 */   5,
/*    SMEM_XCAT2_UNIT_BM_E,               0x03000000 */   6,
/*    SMEM_XCAT2_UNIT_EPLR_E,             0x03800000 */   7,
/*    SMEM_XCAT2_UNIT_LMS_E,              0x04000000 */   8,
/*    SMEM_XCAT2_UNIT_FDB_E,              0x06000000 */  12,
/*    SMEM_XCAT2_UNIT_MPPM_BANK0_E,       0x06800000 */  13,
/*    SMEM_XCAT2_UNIT_MPPM_BANK1_E,       0x07000000 */  14,
/*    SMEM_XCAT2_UNIT_MEM_E,              0x07800000 */  15,
/*    SMEM_XCAT2_UNIT_CENTRALIZED_COUNT_E,0x08000000 */  16,
/*    SMEM_XCAT2_UNIT_STACKING_PORTS_E,   0x09000000 */  18,
/*    SMEM_XCAT2_UNIT_UNIPHY_SERDES_E,    0x09800000 */  19,
/*    SMEM_XCAT2_UNIT_VLAN_MC_E           0x0a080000 */  20,
/*    SMEM_XCAT2_UNIT_NETWORK_PORTS_E,    0x0A800000 */  21,
/*    SMEM_XCAT2_UNIT_EQ_E,               0x0B000000 */  22,
/*    SMEM_XCAT2_UNIT_IPCL_E,             0x0B800000 */  23,
/*    SMEM_XCAT2_UNIT_TTI_E,              0x0C000000 */  24,
/*    SMEM_XCAT2_UNIT_IPLR0_E,            0x0C800000 */  25,
/*    SMEM_XCAT2_UNIT_IPLR1_E,            0x0D000000 */  26,
/*    SMEM_XCAT2_UNIT_TCC_LOWER_E,        0x0E000000 */  28,
/*    SMEM_XCAT2_UNIT_TCC_UPPER_E,        0x0E800000 */  29,
/*    SMEM_XCAT2_UNIT_HA_E,               0x0F000000 */  30,
/*    SMEM_XCAT2_UNIT_EPCL_E,             0x0F800000 */  31,
      END_OF_TABLE /* must be last*/
};

/* Return unit chunk pointer by real unit index */
#define UNIT_CHUNK_PTR_MAC(dev_info, unit) \
    &dev_info->unitMemArr[usedUnitsNumbersArray[unit]]


/*******************************************************************************
*   smemXCat2InitFuncArray
*
* DESCRIPTION:
*       Init specific xCat2 functions array.
*
* INPUTS:
*       devObjPtr   - pointer to device object
*       commonDevMemInfoPtr   - pointer to common device memory object.
*
* OUTPUTS:
*       commonDevMemInfoPtr   - pointer to common device memory object.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemXCat2InitFuncArray
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_CHT_DEV_COMMON_MEM_INFO  * commonDevMemInfoPtr
)
{
    GT_U32                 unitIndex;
    GT_U32                 unit;
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = devObjPtr->deviceMemory;

    /* Set by default that all units bind to 'Fatal error' */
    for (unit = 0; unit < 64; unit++)
    {
        commonDevMemInfoPtr->specFunTbl[unit].specFun = smemChtFatalError;
    }

    /* Bind the units to the specific unit chunk with the generic function */
    for (unit = SMEM_XCAT2_UNIT_MG_E; unit < SMEM_XCAT2_UNIT_LAST_E; unit++)
    {
        unitIndex = usedUnitsNumbersArray[unit];
        devMemInfoPtr->unitMemArr[unitIndex].chunkIndex = unitIndex;

        commonDevMemInfoPtr->specFunTbl[unitIndex].specFun = smemDevFindInUnitChunk;

        /* Set the parameter to be (casting of) the pointer to the unit chunk */
        commonDevMemInfoPtr->specFunTbl[unitIndex].specParam  =
            smemConvertChunkIndexToPointerAsParam(devObjPtr, unitIndex);

        if(commonDevMemInfoPtr->specFunTbl[unitIndex].specParam ==
            (GT_UINTPTR)(void*)(UNIT_CHUNK_PTR_MAC(devMemInfoPtr, SMEM_XCAT2_UNIT_LMS_E)))
        {
            commonDevMemInfoPtr->specFunTbl[unitIndex+1].specFun    =
            commonDevMemInfoPtr->specFunTbl[unitIndex+2].specFun    =
            commonDevMemInfoPtr->specFunTbl[unitIndex+3].specFun    =
            commonDevMemInfoPtr->specFunTbl[unitIndex].specFun;

            /* set the parameter to be (casting of) the pointer to the unit chunk */
            commonDevMemInfoPtr->specFunTbl[unitIndex+1].specParam  =
            commonDevMemInfoPtr->specFunTbl[unitIndex+2].specParam  =
            commonDevMemInfoPtr->specFunTbl[unitIndex+3].specParam  =
            commonDevMemInfoPtr->specFunTbl[unitIndex].specParam;

            devMemInfoPtr->unitMemArr[unitIndex+1].chunkIndex = unitIndex + 1;
            devMemInfoPtr->unitMemArr[unitIndex+2].chunkIndex = unitIndex + 2;
            devMemInfoPtr->unitMemArr[unitIndex+3].chunkIndex = unitIndex + 3;
        }
    }
}

/*******************************************************************************
*   smemXCat2FindMem
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
static void * smemXCat2FindMem
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32                  address,
    IN GT_U32                  memSize,
    OUT SMEM_ACTIVE_MEM_ENTRY_STC ** activeMemPtrPtr
)
{
    return smemGenericFindMem(devObjPtr, accessType, address, memSize,
                                  activeMemPtrPtr);
}


/*******************************************************************************
*   smemXCat2UnitMg
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  MG unit
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
static void smemXCat2UnitMg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = devObjPtr->deviceMemory;
    SMEM_UNIT_CHUNKS_STC * unitChunkPtr = UNIT_CHUNK_PTR_MAC(devMemInfoPtr, SMEM_XCAT2_UNIT_MG_E);
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {


             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000000 ,0x000000e0)}  /* Global Configuration */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000000e4 ,0x000000ec)}  /* Debug Bus */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000000f0 ,0x000000fc)}   /* User Defined Register */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000300 ,0x00000350)}  /* Address Decoding */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002680 ,0x00002680)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002780 ,0x00002780)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002800 ,0x0000281c)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002868 ,0x00002874)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0040000c ,0x0040000c)}   /* TWSI Registers */

        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem, numOfChunks, unitChunkPtr);
    }

    /* chunks with formulas */
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
             {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0000260c ,0)}, FORMULA_SINGLE_PARAMETER(8 , 0x10)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x000026c0 ,0)}, FORMULA_SINGLE_PARAMETER(8 , 0x4)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00002700 ,0)}, FORMULA_TWO_PARAMETERS(3 , 0x4, 8, 0x10)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00002820 ,0)}, FORMULA_TWO_PARAMETERS(2 , 0x20, 8, 0x4)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00002860 ,0)}, FORMULA_SINGLE_PARAMETER(2 , 0x4)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

        tmpUnitChunk.chunkIndex = unitChunkPtr->chunkIndex;

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitChunkPtr, &tmpUnitChunk);
    }
}

/*******************************************************************************
*   smemXCat2UnitPolicerUnify
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  any of the 3 Policers:
*                   1. iplr 0
*                   2. iplr 1
*                   3. eplr
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitName    - unit name
*       baseAddr    - unit base address
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemXCat2UnitPolicerUnify
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SMEM_XCAT2_UNIT_NAME_ENT unitName,
    IN GT_U32   baseAddr
)
{
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = devObjPtr->deviceMemory;
    SMEM_UNIT_CHUNKS_STC * unitChunkPtr = UNIT_CHUNK_PTR_MAC(devMemInfoPtr, unitName);
    /* chunks with flat memory (no formulas) */
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000000, 0x00000054)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000070, 0x00000070)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000100, 0x00000104)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00000200, 36)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00000400, 96)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00000500, 192), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(74, 16)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00000800, 256)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00000900, 256)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00001000, 2048)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00040000, 131072), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(229, 32)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00060000, 131072), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(229, 32)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00080000, 2560), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(51, 8)}

        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);
        GT_U32  ii;

        for(ii = 0 ; ii < numOfChunks ; ii ++)
        {
            chunksMem[ii].memFirstAddr += baseAddr;
        }

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitChunkPtr);
    }

    /* chunks with formulas */
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
             {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00000074 ,0)}, FORMULA_SINGLE_PARAMETER(8 , 0x4)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x000000c0 ,0)}, FORMULA_SINGLE_PARAMETER(8 , 0x4)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00000108 ,0)}, FORMULA_SINGLE_PARAMETER(2 , 0x4)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00001800 ,0)}, FORMULA_SINGLE_PARAMETER(32 , 0x4)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;
        GT_U32  ii;
        tmpUnitChunk.chunkIndex = unitChunkPtr->chunkIndex;

        for(ii = 0 ; ii < numOfChunks ; ii ++)
        {
            chunksMem[ii].memChunkBasic.memFirstAddr += baseAddr;
        }

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitChunkPtr,&tmpUnitChunk);
    }
}

/*******************************************************************************
*   smemXCat2UnitIplr1
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  IPLR1 unit
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
static void smemXCat2UnitIplr1
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    smemXCat2UnitPolicerUnify(devObjPtr, SMEM_XCAT2_UNIT_IPLR1_E, policerBaseAddr[1]);
}

/*******************************************************************************
*   smemXCat2UnitTti
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  TTI unit
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
static void smemXCat2UnitTti
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = devObjPtr->deviceMemory;
    SMEM_UNIT_CHUNKS_STC * unitChunkPtr = UNIT_CHUNK_PTR_MAC(devMemInfoPtr, SMEM_XCAT2_UNIT_TTI_E);

    /* chunks with flat memory (no formulas) */
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
              {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0c000000, 0x0c000060)}
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0c000070, 0x0c000078)}
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0c000098, 0x0c00009c)}
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0c000100, 0x0c000120)}
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0c000180, 0x0c000188)}
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0c0001a0, 0x0c0001c8)}
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0c000200, 0x0c000204)}
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0c000520, 0x0c000520)}
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0c000600, 512)}
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0c000800, 256)}
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0c001000, 1024)}
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0c0014e0, 0x0c0014e4)}
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0c001498, 0x0c00149c)} /* Descriptor registers  manually added */
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0c001708, 0x0c00170c)}
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0c001800, 2048)}
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC(0x0c002000, 0x0c002004)}
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC(0x0c002010, 0x0c00201c)}
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC(0x0c002028, 0x0c00203c)}
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0c004000, 8192)}
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0c00c000, 16384)}
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0c010000, 384)}
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0c030000, 256)}

        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitChunkPtr);
    }

    /* chunks with formulas */
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
             {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0c000064 ,0)}, FORMULA_SINGLE_PARAMETER(2, 0x4)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0c000080 ,0)}, FORMULA_SINGLE_PARAMETER(6, 0x4)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0c0000a0 ,0)}, FORMULA_TWO_PARAMETERS(2, 0x20, 8 , 0x4)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0c0000f0 ,0)}, FORMULA_TWO_PARAMETERS(2, 0x8, 2 , 0x4)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0c000140 ,0)}, FORMULA_TWO_PARAMETERS(4, 0x10, 4 , 0x4)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0c000190 ,0)}, FORMULA_TWO_PARAMETERS(2, 0x8, 2 , 0x4)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0c000300 ,0)}, FORMULA_SINGLE_PARAMETER(4, 0x4)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0c000310 ,0)}, FORMULA_SINGLE_PARAMETER(32, 0x4)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0c000400 ,0)}, FORMULA_SINGLE_PARAMETER(16, 0x4)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0c000440 ,0)}, FORMULA_THREE_PARAMETERS(2, 0x4, 2, 0x8, 2, 0x10)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0c000460 ,0)}, FORMULA_SINGLE_PARAMETER(2, 0x4)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0c000470 ,0)}, FORMULA_SINGLE_PARAMETER(16, 0x4)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0c000500 ,0)}, FORMULA_TWO_PARAMETERS(2, 0x10, 4 , 0x4)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0c000900 ,0)}, FORMULA_SINGLE_PARAMETER(16, 0x4)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0c001400 ,0)}, FORMULA_SINGLE_PARAMETER(38, 0x4)} /* Descriptor registers  manually added */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0c001600 ,0)}, FORMULA_TWO_PARAMETERS(4, 0x4, 8 , 0x10)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0c001700 ,0)}, FORMULA_SINGLE_PARAMETER(2, 0x4)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0c002008 ,0)}, FORMULA_TWO_PARAMETERS(3, 0xc, 2 , 0x4)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;
        tmpUnitChunk.chunkIndex = unitChunkPtr->chunkIndex;

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitChunkPtr,&tmpUnitChunk);
    }
}

/*******************************************************************************
*   smemXCat2UnitDfx
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  DFX unit
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
static void smemXCat2UnitDfx
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = devObjPtr->deviceMemory;
    SMEM_UNIT_CHUNKS_STC * unitChunkPtr = UNIT_CHUNK_PTR_MAC(devMemInfoPtr, SMEM_XCAT2_UNIT_DFX_E);

    /* chunks with flat memory (no formulas) */
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00800000, 0x008ffffc)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitChunkPtr);
    }
}

/*******************************************************************************
*   smemXCat2UnitTxqEgr
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  TXQ_EGR unit
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
static void smemXCat2UnitTxqEgr
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = devObjPtr->deviceMemory;
    SMEM_UNIT_CHUNKS_STC * unitChunkPtr = UNIT_CHUNK_PTR_MAC(devMemInfoPtr, SMEM_XCAT2_UNIT_EGR_TXQ_E);

    /* chunks with flat memory (no formulas) */
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01800000, 0x01800014)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01800020, 0x0180002c)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01800084, 0x018000bc)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x018000d0, 0x018000d0)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01800100, 0x01800104)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01800110, 0x0180011c)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01800120, 0x01800124)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01800130, 0x01800134)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01800180, 0x018001dc)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x018001e0, 0x018001e0)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x018001ec, 0x018001ec)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x018001f4, 0x018001f4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01800230, 0x0180024c)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x018001f0, 0x018001f0)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x018002f0, 0x018002f0)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01807e80, 0x01807e80)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01847e00, 0x01847e00)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x018c7e00, 0x018c7e00)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01a87e00, 0x01a87e00)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01ac7e08, 0x01ac7e08)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01cc0000, 0x01cc0008)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitChunkPtr);
    }

    /* chunks with formulas */
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
             {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01800018 ,0)}, FORMULA_SINGLE_PARAMETER(2, 0x4)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01800030 ,0)}, FORMULA_SINGLE_PARAMETER(4, 0x4)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01800040 ,0)}, FORMULA_TWO_PARAMETERS(3, 0x4, 4, 0x10)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01800080 ,0)}, FORMULA_SINGLE_PARAMETER(28, 0x200)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x018000c0 ,0)}, FORMULA_SINGLE_PARAMETER(4, 0x4)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01800140 ,0)}, FORMULA_SINGLE_PARAMETER(2, 0x20)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01800260 ,0)}, FORMULA_TWO_PARAMETERS(2, 0x40, 8, 0x4)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x018002c0 ,0)}, FORMULA_SINGLE_PARAMETER(8, 0x4)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01800f00 ,0)}, FORMULA_TWO_PARAMETERS(3, 0x4, 4 , 0x10)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01840000 ,0)}, FORMULA_SINGLE_PARAMETER(28 , 0x200)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01880000 ,0)}, FORMULA_THREE_PARAMETERS(3, 0x40000, 28, 0x200, 8, 0x8000)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01887e00 ,0)}, FORMULA_SINGLE_PARAMETER(8, 0x8000)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01907e00 ,0)}, FORMULA_SINGLE_PARAMETER(8 , 0x8000)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01940000 ,0)}, FORMULA_THREE_PARAMETERS(2, 0x4, 8, 0x8, 8, 0x400)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01a40000 ,0)}, FORMULA_SINGLE_PARAMETER(128 , 0x10)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01a40004 ,0)}, FORMULA_SINGLE_PARAMETER(32 , 0x10)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01a40008 ,0)}, FORMULA_SINGLE_PARAMETER(8 , 0x10)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01a4000c ,0)}, FORMULA_SINGLE_PARAMETER(32 , 0x10)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01a80000 ,0)}, FORMULA_SINGLE_PARAMETER(28 , 0x200)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01ac0000 ,0)}, FORMULA_TWO_PARAMETERS(8 , 0x8000, 28, 0x200)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01ac0004 ,0)}, FORMULA_TWO_PARAMETERS(64 , 0x200 , 8 , 0x8000)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01ac0008 ,0)}, FORMULA_SINGLE_PARAMETER(28 , 0x200)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01ac000c ,0)}, FORMULA_TWO_PARAMETERS(64 , 0x200 , 8 , 0x8000)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01ac7e00 ,0)}, FORMULA_SINGLE_PARAMETER(8 , 0x8000)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01b40144 ,0)}, FORMULA_TWO_PARAMETERS(7, 0x4, 2, 0x20)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01b40180 ,0)}, FORMULA_SINGLE_PARAMETER(2 , 0x4)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01d40000 ,0)}, FORMULA_TWO_PARAMETERS(3 , 0x4, 28, 0x200)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01e40000 ,0)}, FORMULA_SINGLE_PARAMETER(28 , 0x10)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01e40004 ,0)}, FORMULA_TWO_PARAMETERS(2 , 0x4, 2176, 0x10)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01e80000 ,0)}, FORMULA_SINGLE_PARAMETER(8 , 0x4)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;
        tmpUnitChunk.chunkIndex = unitChunkPtr->chunkIndex;

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitChunkPtr,&tmpUnitChunk);
    }
}

/*******************************************************************************
*   smemXCat2UnitTxqEgrVlan
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  TXQ_EGR - Vlan/MC unit
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
static void smemXCat2UnitTxqEgrVlan
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = devObjPtr->deviceMemory;
    SMEM_UNIT_CHUNKS_STC * unitChunkPtr = UNIT_CHUNK_PTR_MAC(devMemInfoPtr, SMEM_XCAT2_UNIT_VLAN_MC_E);

    /* chunks with flat memory (no formulas) */
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0a000000, 0x0a000118)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0a080000, 4096)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0a100000, 65536)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0a200000, 131072)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0a400000, 0x0a40003c)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0a400100, 128)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0a4001fc, 0x0a4001fc)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0a400200, 112)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0a401000, 0x0a401018)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitChunkPtr);
    }
}

/*******************************************************************************
*   smemXCat2UnitL2i
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  L2i unit
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
static void smemXCat2UnitL2i
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = devObjPtr->deviceMemory;
    SMEM_UNIT_CHUNKS_STC * unitChunkPtr = UNIT_CHUNK_PTR_MAC(devMemInfoPtr, SMEM_XCAT2_UNIT_L2I_E);
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0200080c, 0x0200080c)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0200081c, 0x0200081c)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02012800, 0x02012800)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02040000, 0x020400d4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02040104, 0x02040108)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02040130, 0x02040150)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x020401a0, 0x020401a8)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02040700, 0x02040748)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitChunkPtr);
    }

    /* chunks with formulas */
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
             {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02000000 ,0)}, FORMULA_TWO_PARAMETERS(2, 0x10, 28, 0x1000)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02000200 ,0)}, FORMULA_SINGLE_PARAMETER(7, 0x4)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02000300 ,0)}, FORMULA_SINGLE_PARAMETER(4, 0x4)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02000400 ,0)}, FORMULA_SINGLE_PARAMETER(28, 0x1000)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02000500 ,0)}, FORMULA_SINGLE_PARAMETER(12, 0x4)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02000600 ,0)}, FORMULA_SINGLE_PARAMETER(8 , 0x4)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02000800 ,0)}, FORMULA_TWO_PARAMETERS(3, 0x4, 16 , 0x1000)}
            /* IEEE Reserved Multicast Configuration1/2/3 */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02000810 ,0)}, FORMULA_TWO_PARAMETERS(2, 0x8, 16 , 0x1000)}
            /* IEEE Reserved Multicast Configuration2 errata */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02000820 ,0)}, FORMULA_SINGLE_PARAMETER(16 , 0x1000)}
            /* IEEE Reserved Multicast Configuration3 errata*/
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02000828 ,0)}, FORMULA_SINGLE_PARAMETER(16 , 0x1000)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02010800 ,0)}, FORMULA_SINGLE_PARAMETER(2, 0x1000)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02020800 ,0)}, FORMULA_TWO_PARAMETERS(2, 0x8000, 8 , 0x1000)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02030800 ,0)}, FORMULA_THREE_PARAMETERS(2, 0x10000, 8 , 0x1000, 2 , 0x8000)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0203f000 ,0)}, FORMULA_SINGLE_PARAMETER(2, 0x10)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x020400dc ,0)}, FORMULA_TWO_PARAMETERS(5, 0x4, 2 , 0x14)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02040a00 ,0)}, FORMULA_SINGLE_PARAMETER(20, 0x4)} /* Descriptor registers  manually added */
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;
        tmpUnitChunk.chunkIndex = unitChunkPtr->chunkIndex;

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitChunkPtr,&tmpUnitChunk);
    }
}

/*******************************************************************************
*   smemXCat2UnitUcRoute
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  UC route unit
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
static void smemXCat2UnitUcRoute
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = devObjPtr->deviceMemory;
    SMEM_UNIT_CHUNKS_STC * unitChunkPtr = UNIT_CHUNK_PTR_MAC(devMemInfoPtr, SMEM_XCAT2_UNIT_UC_ROUTE_E);
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02800e3c, 0x02800e3c)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitChunkPtr);
    }
}

/*******************************************************************************
*   smemXCat2UnitBm
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  BM unit
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
static void smemXCat2UnitBm
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = devObjPtr->deviceMemory;
    SMEM_UNIT_CHUNKS_STC * unitChunkPtr = UNIT_CHUNK_PTR_MAC(devMemInfoPtr, SMEM_XCAT2_UNIT_BM_E);
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x03000000, 0x03000084)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0300017c, 0x0300017c)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x03000200, 0x03000210)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x03000300, 0x03000328)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x03010000, 16384)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x03020000, 16384)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x03040000, 16384)}


        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitChunkPtr);
    }

    /* chunks with formulas */
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
            {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x03000100 ,0)}, FORMULA_SINGLE_PARAMETER(28 , 0x4)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;
        tmpUnitChunk.chunkIndex = unitChunkPtr->chunkIndex;

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitChunkPtr,&tmpUnitChunk);
    }
}

/*******************************************************************************
*   smemXCat2UnitLms
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  LMS unit
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
static void smemXCat2UnitLms
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = devObjPtr->deviceMemory;
    SMEM_UNIT_CHUNKS_STC * unitChunkPtr = UNIT_CHUNK_PTR_MAC(devMemInfoPtr, SMEM_XCAT2_UNIT_LMS_E);
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x04004000, 0x04004200)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x04005100, 0x04005114)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x04010000, 896)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x04804000, 0x04804144)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x04805100, 0x04805114)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x04810000, 896)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x05004000, 0x05004054)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x05004100, 0x05004200)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x05005100, 0x05005114)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x05010000, 896)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x05804000, 0x0580410c)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x05804140, 0x05804144)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x05805100, 0x05805104)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x05810000, 896)}

        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitChunkPtr);
    }
}

/*******************************************************************************
*   smemXCat2UnitFdb
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  FDB unit
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
static void smemXCat2UnitFdb
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = devObjPtr->deviceMemory;
    SMEM_UNIT_CHUNKS_STC * unitChunkPtr = UNIT_CHUNK_PTR_MAC(devMemInfoPtr, SMEM_XCAT2_UNIT_FDB_E);
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x06000000, 0x0600003c)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x06000050, 0x06000050)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x06000064, 0x06000070)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x06400000, 262144 )} /*FDB support 16K entries*/

        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitChunkPtr);
    }
    /* chunks with formulas */
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
            {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x06000040 ,0)}, FORMULA_TWO_PARAMETERS(2, 0x14, 4 , 0x4)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;
        tmpUnitChunk.chunkIndex = unitChunkPtr->chunkIndex;

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitChunkPtr,&tmpUnitChunk);
    }

}


/*******************************************************************************
*   smemXCat2UnitMppm
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the xCat2 MPPM unit
*
* INPUTS:
*       devObjPtr   - pointer to common device memory object.
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
static void smemXCat2UnitMppm
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SMEM_XCAT2_UNIT_NAME_ENT unitName,
    IN GT_U32   baseAddr
)
{
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = devObjPtr->deviceMemory;
    SMEM_UNIT_CHUNKS_STC * unitChunkPtr = UNIT_CHUNK_PTR_MAC(devMemInfoPtr, unitName);
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000000 , 524288)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00100000 , 4)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);
        GT_U32  ii;

        for(ii = 0; ii < numOfChunks; ii ++)
        {
            chunksMem[ii].memFirstAddr += baseAddr;
        }
        smemInitMemChunk(devObjPtr,chunksMem, numOfChunks, unitChunkPtr);
    }
}

/*******************************************************************************
*   smemXCat2UnitMppm0
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the xCat2 MPPM0 unit
*
* INPUTS:
*       devObjPtr   - pointer to common device memory object.
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
static void smemXCat2UnitMppm0
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    smemXCat2UnitMppm(devObjPtr, SMEM_XCAT2_UNIT_MPPM_BANK0_E, mppmBankBaseAddr[0]);
}

/*******************************************************************************
*   smemXCat2UnitMppm0
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the xCat2 MPPM0 unit
*
* INPUTS:
*       devObjPtr   - pointer to common device memory object.
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
static void smemXCat2UnitMppm1
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    smemXCat2UnitMppm(devObjPtr, SMEM_XCAT2_UNIT_MPPM_BANK1_E, mppmBankBaseAddr[1]);
}

/*******************************************************************************
*   smemXCat2UnitHa
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the xCat2 HA_and_EPCL unit
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
static void smemXCat2UnitHa
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = devObjPtr->deviceMemory;
    SMEM_UNIT_CHUNKS_STC * unitChunkPtr = UNIT_CHUNK_PTR_MAC(devMemInfoPtr, SMEM_XCAT2_UNIT_HA_E);
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0f000100, 0x0f000100 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0f00010c, 0x0f000110 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0f000370, 0x0f000378 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0f0003a0, 0x0f0003ac )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0f000408, 0x0f000410 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0f000420, 0x0f000428 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0f000430, 0x0f000434 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0f000550, 0x0f000558 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0f000710, 0x0f000710 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0f000800, 0x0f000804)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0f000810, 0x0f00081c)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0f000828, 0x0f00083c)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0f008000, 16640)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0f010000, 16384)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0f018000, 2048)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0f040000, 32768)}

        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitChunkPtr);
    }

    /* chunks with formulas */
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
             {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0f000004 ,0)}, FORMULA_TWO_PARAMETERS(2, 0x1c, 2 , 0x4)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0f00002c ,0)}, FORMULA_SINGLE_PARAMETER(16 , 0x4)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0f0003e0 ,0)}, FORMULA_SINGLE_PARAMETER(5 , 0x4)} /* Descriptor registers  manually added */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0f000080 ,0)}, FORMULA_SINGLE_PARAMETER(2 , 0x4)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0f000104 ,0)}, FORMULA_TWO_PARAMETERS(2, 0x10, 2 , 0x4)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0f000120 ,0)}, FORMULA_SINGLE_PARAMETER(4 , 0x4)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0f000130 ,0)}, FORMULA_TWO_PARAMETERS(2, 0x10, 2 , 0x4)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0f000300 ,0)}, FORMULA_SINGLE_PARAMETER(16 , 0x4)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0f000340 ,0)}, FORMULA_SINGLE_PARAMETER(8 , 0x4)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0f000400 ,0)}, FORMULA_SINGLE_PARAMETER(2 , 0x4)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0f000440 ,0)}, FORMULA_SINGLE_PARAMETER(16 , 0x4)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0f000500 ,0)}, FORMULA_SINGLE_PARAMETER(4 , 0x4)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0f000510 ,0)}, FORMULA_TWO_PARAMETERS(2, 0x20, 8 , 0x4)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0f000808 ,0)}, FORMULA_TWO_PARAMETERS(3, 0xc, 2 , 0x4)}

        };

        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;
        tmpUnitChunk.chunkIndex = unitChunkPtr->chunkIndex;

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitChunkPtr,&tmpUnitChunk);
    }
}

/*******************************************************************************
*   smemXCat2UnitMem
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the xCat2 MEM unit
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
static void smemXCat2UnitMem
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = devObjPtr->deviceMemory;
    SMEM_UNIT_CHUNKS_STC * unitChunkPtr = UNIT_CHUNK_PTR_MAC(devMemInfoPtr, SMEM_XCAT2_UNIT_MEM_E);
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x07800000, 0x07800000)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0780000c, 0x0780000c)} /* Buffer Memory MPPM Configuration */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x07800010, 0x07800010)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x07800024, 0x07800028)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x07800090, 0x078000a0)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x07e00000, 528)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitChunkPtr);
    }
}

/*******************************************************************************
*   smemXCat2UnitEplr
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the xCat2 EPLR unit
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
static void smemXCat2UnitEplr
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    smemXCat2UnitPolicerUnify(devObjPtr, SMEM_XCAT2_UNIT_EPLR_E, policerBaseAddr[2]);
}

/*******************************************************************************
*   smemXCat2UnitCentralizedCounters
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  Centralized_Counters unit
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
static void smemXCat2UnitCentralizedCounters
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = devObjPtr->deviceMemory;
    SMEM_UNIT_CHUNKS_STC * unitChunkPtr = UNIT_CHUNK_PTR_MAC(devMemInfoPtr, SMEM_XCAT2_UNIT_CENTRALIZED_COUNT_E);
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x08000000, 0x08000000)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x08000030, 0x08000044)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x08000100, 0x08000114)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x08080000, 0x800)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x08084000, 0x800)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x08088000, 0x800)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0808c000, 0x800)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x08090000, 0x800)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x08094000, 0x800)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x08098000, 0x800)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0809c000, 0x800)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitChunkPtr);
    }
    /* chunks with formulas */
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
             {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x08000118 ,0)}, FORMULA_SINGLE_PARAMETER(8, 0x4)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x080010a0 ,0)}, FORMULA_TWO_PARAMETERS(8, 0x100, 4, 0x4)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x08001180 ,0)}, FORMULA_TWO_PARAMETERS(3, 0x4, 8, 0x100)}
        };

        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;
        tmpUnitChunk.chunkIndex = unitChunkPtr->chunkIndex;

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitChunkPtr,&tmpUnitChunk);
    }
}

/*******************************************************************************
*   smemXCat2UnitUnifySerdes
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  Unify Serdes unit
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
static void smemXCat2UnitUnifySerdes
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = devObjPtr->deviceMemory;
    SMEM_UNIT_CHUNKS_STC * unitChunkPtr = UNIT_CHUNK_PTR_MAC(devMemInfoPtr, SMEM_XCAT2_UNIT_UNIPHY_SERDES_E);

    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
              {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x09800000 ,0)}, FORMULA_THREE_PARAMETERS(2, 0x400, 5, 0x1000, 9, 0x4)}
             ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x09800200, 512)}}
             ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x09800600, 512)}}
             ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x09801200, 512)}}
             ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x09801600, 512)}}
             ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x09802200, 512)}}
             ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x09802600, 512)}}
             ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x09803200, 512)}}
             ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x09803600, 512)}}
             ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x09804200, 512)}}
             ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x09804600, 512)}}
        };

        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, unitChunkPtr);
    }
}

/*******************************************************************************
*   smemXCat2UnitGigPorts
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  Network ports unit
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
static void smemXCat2UnitNetworkPorts
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = devObjPtr->deviceMemory;
    SMEM_UNIT_CHUNKS_STC * unitChunkPtr = UNIT_CHUNK_PTR_MAC(devMemInfoPtr, SMEM_XCAT2_UNIT_NETWORK_PORTS_E);

    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
            /* CPU ports */
            {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0a80fc00, 0x0a80fc48)}}
        };

        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, unitChunkPtr);
    }

    /* chunks with formulas */
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
            /* Network + stack ports */
             {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0a800000 ,0)}, FORMULA_TWO_PARAMETERS(6, 0x4, 28, 0x400)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0a800020 ,0)}, FORMULA_TWO_PARAMETERS(11, 0x4, 28, 0x400)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0a80004c ,0)}, FORMULA_TWO_PARAMETERS(3, 0x4, 6, 0x1000)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0a800058 ,0)}, FORMULA_TWO_PARAMETERS(8, 0x4, 28, 0x400)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0a800090 ,0)}, FORMULA_TWO_PARAMETERS(2, 0x4, 28, 0x400)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0a8000c0 ,0)}, FORMULA_TWO_PARAMETERS(5, 0x4, 28, 0x400)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0a80044c ,0)}, FORMULA_TWO_PARAMETERS(2, 0x400, 6, 0x1000)}
        };

        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;
        tmpUnitChunk.chunkIndex = unitChunkPtr->chunkIndex;

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitChunkPtr,&tmpUnitChunk);
    }
}

/*******************************************************************************
*   smemXCat2UnitStackPorts
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  Stacking ports unit
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
static void smemXCat2UnitStackPorts
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = devObjPtr->deviceMemory;
    SMEM_UNIT_CHUNKS_STC * unitChunkPtr = UNIT_CHUNK_PTR_MAC(devMemInfoPtr, SMEM_XCAT2_UNIT_STACKING_PORTS_E);

    /* chunks with formulas */
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
               {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x09000000,4)}}/* this line needed only for WA for simulation limits : devObjPtr->tablesInfo.xgPortMibCounters*/
              ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x09300000, 256)}}
              ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x09320000, 256)}}
              ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x09340000, 256)}}
              ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x09360000, 256)}}

        };

        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, unitChunkPtr);
    }

}

/*******************************************************************************
*   smemXCat2UnitEq
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  Eq unit
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
static void smemXCat2UnitEq
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = devObjPtr->deviceMemory;
    SMEM_UNIT_CHUNKS_STC * unitChunkPtr = UNIT_CHUNK_PTR_MAC(devMemInfoPtr, SMEM_XCAT2_UNIT_EQ_E);

    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
              {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0b000000, 0x0b000004)}
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0b000010, 0x0b000040)}
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0b000058, 0x0b000068)}
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0b000074, 0x0b000074)}
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0b0000a0, 0x0b0000a8)}
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0b001000, 0x0b005008)}
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0b008010, 0x0b008010)}
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0b00a000, 0x0b00a000)}
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0b00b000, 0x0b00b018)}
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0b00b040, 0x0b00b04c)}
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0b00b070, 0x0b00b070)}
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0b020000, 0x0b02000c)}
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0b040000, 448)}
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0b050000, 0x0b050004)}
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0b100000, 128)}
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0b200000, 1024)}
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0b300000, 512)}
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0b400000, 4096)}
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0b500000, 124)}
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0b600000, 2048)}

        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitChunkPtr);
    }

    /* chunks with formulas */
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
              {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0b00000c ,0)}, FORMULA_SINGLE_PARAMETER(2 , 0x64)}
             ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0b000044 ,0)}, FORMULA_SINGLE_PARAMETER(5 , 0x4)}
             ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0b007000 ,0)}, FORMULA_TWO_PARAMETERS(2, 0x40, 16 , 0x4)}
             ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0b008000 ,0)}, FORMULA_SINGLE_PARAMETER(4 , 0x4)}
             ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0b009000 ,0)}, FORMULA_SINGLE_PARAMETER(16 , 0x4)}
             ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0b00b020 ,0)}, FORMULA_SINGLE_PARAMETER(7 , 0x4)}
             ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0b00b050 ,0)}, FORMULA_SINGLE_PARAMETER(2 , 0x4)}
             ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0b080004 ,0)}, FORMULA_SINGLE_PARAMETER(31 , 0x4)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;
        tmpUnitChunk.chunkIndex = unitChunkPtr->chunkIndex;

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitChunkPtr,&tmpUnitChunk);
    }
}

/*******************************************************************************
*   smemXCat2UnitIpcl
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  IPCL unit
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
static void smemXCat2UnitIpcl
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = devObjPtr->deviceMemory;
    SMEM_UNIT_CHUNKS_STC * unitChunkPtr = UNIT_CHUNK_PTR_MAC(devMemInfoPtr, SMEM_XCAT2_UNIT_IPCL_E);

    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0b800000, 0x0b800024)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0b800060, 0x0b800064)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0b800088, 0x0b800088)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0B800400, 448), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(70, 16)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0B800800, 2048), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0B810000, 33792), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(23, 8)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0B820000, 33792), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(23, 8)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0B830000, 33792), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(23, 8)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitChunkPtr);
    }
    /* chunks with formulas */
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
              {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0b800080 ,0)}, FORMULA_SINGLE_PARAMETER(2 , 0x4)}
             ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0b8005c0 ,0)}, FORMULA_SINGLE_PARAMETER(16 , 0x4)}
             ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0b800200 ,0)}, FORMULA_SINGLE_PARAMETER(28 , 0x4)} /* Descriptor registers  manually added */
        };

        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;
        tmpUnitChunk.chunkIndex = unitChunkPtr->chunkIndex;

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitChunkPtr,&tmpUnitChunk);
    }

}

/*******************************************************************************
*   smemXCat2UnitIplr0
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the xCat2 IPLR0 unit
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
static void smemXCat2UnitIplr0
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    smemXCat2UnitPolicerUnify(devObjPtr, SMEM_XCAT2_UNIT_IPLR0_E, policerBaseAddr[0]);
}

/*******************************************************************************
*   smemXCat2UnitIpclTcc
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  IPCL_TCC unit
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
static void smemXCat2UnitIpclTcc
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = devObjPtr->deviceMemory;
    SMEM_UNIT_CHUNKS_STC * unitChunkPtr = UNIT_CHUNK_PTR_MAC(devMemInfoPtr, SMEM_XCAT2_UNIT_TCC_LOWER_E);

    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
              {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0e000130, 0x0e000138)}
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0e000148, 0x0e000148)}
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0e000170, 0x0e000170)}
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0e000198, 0x0e0001c0)}
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0e000208, 0x0e000238)}
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0e00031c, 0x0e00031c)}
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0e000400, 256)}
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0e001000, 256)}
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0e002000, 2048)}
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0e040000, 65536)}
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0e0b0000, 16384)}
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0e0c0000, 4096)}
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0e0d0000, 4096)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitChunkPtr);
    }
    /* chunks with formulas */
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
              {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0e000100 ,0)}, FORMULA_SINGLE_PARAMETER(12 , 0x4)}
             ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0e000304 ,0)}, FORMULA_TWO_PARAMETERS(12 , 0x4, 3, 0x8)}
        };

        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;
        tmpUnitChunk.chunkIndex = unitChunkPtr->chunkIndex;

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitChunkPtr,&tmpUnitChunk);
    }

}

/*******************************************************************************
*   smemXCat2UnitEpcl
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the xCat2 EPCL unit
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
static void smemXCat2UnitEpcl
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = devObjPtr->deviceMemory;
    SMEM_UNIT_CHUNKS_STC * unitChunkPtr = UNIT_CHUNK_PTR_MAC(devMemInfoPtr, SMEM_XCAT2_UNIT_EPCL_E);

    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0f800000, 0x0f800000)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0f800080, 0x0f80008c)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0f800200, 0x0f800200)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0f808000, 16640)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitChunkPtr);
    }
    /* chunks with formulas */
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
             {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0f800010 ,0)}, FORMULA_TWO_PARAMETERS(7, 0x10, 2 , 0x4)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0f800090 ,0)}, FORMULA_SINGLE_PARAMETER(4 , 0x4)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0f800100 ,0)}, FORMULA_TWO_PARAMETERS(2, 0x40, 8 , 0x4)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0f800230 ,0)}, FORMULA_SINGLE_PARAMETER(2 , 0x4)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;
        tmpUnitChunk.chunkIndex = unitChunkPtr->chunkIndex;

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitChunkPtr,&tmpUnitChunk);
    }
}

/*******************************************************************************
*   smemXCat2UnitIpvxTcc
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  IPvX_TCC unit
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
static void smemXCat2UnitIpvxTcc
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = devObjPtr->deviceMemory;
    SMEM_UNIT_CHUNKS_STC * unitChunkPtr = UNIT_CHUNK_PTR_MAC(devMemInfoPtr, SMEM_XCAT2_UNIT_TCC_UPPER_E);

    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
              {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0e800054, 0x0e800080)}
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0e8000a0, 0x0e8000a4)}
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0e800100, 256)}
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0e800418, 0x0e80041c)}
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0e80096c, 0x0e80097c)}
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0e800d78, 0x0e800d78)}
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0e801000, 256)}
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0e802000, 2048)}
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0e840000, 212992), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(52, 8)}
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0e890000, 4096)}
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0e8a0000, 4096)}
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0e8c0000, 32768)}

        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitChunkPtr);
    }
    /* chunks with formulas */
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
             {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0e800400 ,0)}, FORMULA_SINGLE_PARAMETER(6 , 0x4)}
        };

        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;
        tmpUnitChunk.chunkIndex = unitChunkPtr->chunkIndex;

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitChunkPtr,&tmpUnitChunk);
    }

}

/*******************************************************************************
*   smemXCat2AllocSpecMemory
*
* DESCRIPTION:
*       Allocate xCat2 address type specific memories.
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
static void smemXCat2AllocSpecMemory
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = devObjPtr->deviceMemory;
    SMEM_UNIT_CHUNKS_STC * unitChunkPtr;

    /* init the internal memory */
    smemChtInitInternalSimMemory(devObjPtr,&devMemInfoPtr->common);

    /* Create all the memories of the units */
    smemXCat2UnitMg(devObjPtr);
    smemXCat2UnitDfx(devObjPtr);
    smemXCat2UnitTxqEgr(devObjPtr);
    smemXCat2UnitEq(devObjPtr);
    smemXCat2UnitIplr0(devObjPtr);
    smemXCat2UnitIplr1(devObjPtr);
    smemXCat2UnitEplr(devObjPtr);
    smemXCat2UnitTti(devObjPtr);
    smemXCat2UnitL2i(devObjPtr);
    smemXCat2UnitUcRoute(devObjPtr);
    smemXCat2UnitBm(devObjPtr);
    smemXCat2UnitLms(devObjPtr);
    smemXCat2UnitFdb(devObjPtr);
    smemXCat2UnitEpcl(devObjPtr);
    smemXCat2UnitHa(devObjPtr);
    smemXCat2UnitMppm0(devObjPtr);
    smemXCat2UnitMppm1(devObjPtr);
    smemXCat2UnitCentralizedCounters(devObjPtr);
    smemXCat2UnitUnifySerdes(devObjPtr);
    smemXCat2UnitTxqEgrVlan(devObjPtr);
    smemXCat2UnitNetworkPorts(devObjPtr);
    smemXCat2UnitStackPorts(devObjPtr);
    smemXCat2UnitIpcl(devObjPtr);
    smemXCat2UnitIpclTcc(devObjPtr);
    smemXCat2UnitIpvxTcc(devObjPtr);
    smemXCat2UnitMem(devObjPtr);

    /* Allocate MBus unit memory */
    unitChunkPtr = &devMemInfoPtr->common.pciExtMemArr[SMEM_UNIT_PCI_BUS_MBUS_E].unitMem;
    smemXCatA1UnitPex(devObjPtr, unitChunkPtr, devMemInfoPtr->common.pciExtMemArr[SMEM_UNIT_PCI_BUS_MBUS_E].unitBaseAddr);

    devMemInfoPtr->common.unitChunksBasePtr = (GT_U8 *)devMemInfoPtr->unitMemArr;
    /* Calculate summary of all memory structures - for future SW reset use */
    devMemInfoPtr->common.unitChunksSizeOf = sizeof(devMemInfoPtr->unitMemArr);

}

/*******************************************************************************
*  smemXCat2TableInfoSet
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
void smemXCat2TableInfoSet
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr
)
{

    /* must be set before calling smemXcatA1TableInfoSet(...) */

    /* use xcat A1 table addresses */
    smemXcatA1TableInfoSet(devObjPtr);

    devObjPtr->tablesInfo.pclActionTcamData.commonInfo.baseAddress = 0x0e040000;
    devObjPtr->tablesInfo.pclActionTcamData.paramInfo[0].step = 0x8;
    /* Not exist */
    devObjPtr->tablesInfo.vrfId.commonInfo.baseAddress = 0;
    devObjPtr->tablesInfo.mll.commonInfo.baseAddress = 0;
    devObjPtr->tablesInfo.mllOutInterfaceConfig.commonInfo.baseAddress = 0;
    devObjPtr->tablesInfo.mllOutInterfaceCounter.commonInfo.baseAddress = 0;
    devObjPtr->memUnitBaseAddrInfo.mll = 0;
    devObjPtr->tablesInfo.routeNextHopAgeBits.commonInfo.baseAddress = 0;

    devObjPtr->tablesInfo.pearsonHash.commonInfo.baseAddress = 0xb8005c0;
    devObjPtr->tablesInfo.pearsonHash.paramInfo[0].step = 0x4;

    devObjPtr->tablesInfo.crcHashMask.commonInfo.baseAddress = 0x0b800400;
    devObjPtr->tablesInfo.crcHashMask.paramInfo[0].step = 0x10;

    devObjPtr->tablesInfo.tcamBistArea.commonInfo.baseAddress = 0x0e000400;
    devObjPtr->tablesInfo.tcamBistArea.paramInfo[0].step = 0x4;

    devObjPtr->tablesInfo.tcamArrayCompareEn.commonInfo.baseAddress = 0x0e001000;
    devObjPtr->tablesInfo.tcamArrayCompareEn.paramInfo[0].step = 0x4;

    devObjPtr->tablesInfo.haUp0PortKeepVlan1.commonInfo.baseAddress = 0x0f000440;
    devObjPtr->tablesInfo.haUp0PortKeepVlan1.paramInfo[0].step = 0x4;

    devObjPtr->tablesInfo.ingressVlanTranslation.commonInfo.baseAddress = 0x0c00c000;

    devObjPtr->tablesInfo.arp.commonInfo.baseAddress = 0x0f040000;
    devObjPtr->tablesInfo.tunnelStart.commonInfo.baseAddress = devObjPtr->tablesInfo.arp.commonInfo.baseAddress;
    devObjPtr->tablesInfo.tunnelStart.paramInfo[0].step = devObjPtr->tablesInfo.arp.paramInfo[0].step;

    devObjPtr->tablesInfo.vlanPortMacSa.commonInfo.baseAddress = 0x0f008000;
    devObjPtr->tablesInfo.egressVlanTranslation.commonInfo.baseAddress = 0x0f010000;

    devObjPtr->globalInterruptCauseRegister.ha  = 25;

    devObjPtr->tablesInfo.mcast.paramInfo[0].modulo = 1024;/* the index into the table use modulo 1024 */

    devObjPtr->policerSupport.meterTblBaseAddr = devObjPtr->tablesInfo.policer.commonInfo.baseAddress;
    devObjPtr->policerSupport.countTblBaseAddr = devObjPtr->tablesInfo.policerCounters.commonInfo.baseAddress;

    devObjPtr->tablesInfo.upToQoSProfile.paramInfo[1].step  = 0x8;    /* CFI */
    devObjPtr->tablesInfo.upToQoSProfile.paramInfo[2].step = 0x10;    /*upProfile*/

}

/*******************************************************************************
* smemXCat2RegsInfoSet
*
* DESCRIPTION:
*       Init memory module for xCat2 and above devices.
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
void smemXCat2RegsInfoSet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_CHT_PP_REGS_ADDR_STC        * regAddrDbPtr;


    regAddrDbPtr = SMEM_CHT_MAC_REG_DB_GET(devObjPtr);

    /*fill the DB with 'not valid addresses' */
    smemChtRegDbPreInit(devObjPtr);

    {
        /* must be done after calling smemChtRegDbPreInit(...) */
        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,MG                 ,0);
        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,egrAndTxqVer0      ,0x01800000);
    /*    SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,egrAndTxqVer1      ,0); not use unit */
        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,L2I                ,0x02000000);
        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,ucRouter           ,0x02800000);
    /*    SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,IPvX               ,0); not used unit */
        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,BM                 ,0x03000000);
        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,LMS                ,0x04000000);
/*        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,LMS1               ,0); not used unit */
        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,FDB                ,0x06000000);
    /*    SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,MPPM0              ,0); not used unit */
    /*    SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,MPPM1              ,0); not used unit */
    /*    SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,CTU0               ,0); not used unit */
    /*    SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,CTU1               ,0); not used unit */
        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,EPCL               ,0x0f800000);
        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,HA                 ,0x0f000000);
    /*    SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,RXDMA              ,0); not used unit */
    /*    SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,TXDMA              ,0); not used unit */
        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,MEM                ,0x07800000);
        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,centralizedCntrs   ,0x08000000);
        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,uniphySERDES       ,0x09800000);
        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,GOP.SERDESes       ,0x09800000);
        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,networkPorts       ,0x0a800000);
        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,GOP.ports          ,0x0a800000);
        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,EQ                 ,0x0b000000);
        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,IPCL               ,0x0b800000);
        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,PLR[0]              ,0x0c800000);
        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,PLR[1]              ,0x0d000000);
    /*    SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,MLL                ,0); not used unit */
        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,TCCLowerIPCL       ,0x0e000000);
        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,TCCUpperIPvX       ,0x0e800000);
        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,TTI                ,0x0c000000);
        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,PLR[2]               ,0x03800000);
    /*        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,temp               ,0); not used unit */
    }



    {/*start of unit MG - MG  */
        {/*start of unit addrDecoding */
            regAddrDbPtr->MG.addrDecoding.unitDefaultAddrUDA = 0x00000300;
            regAddrDbPtr->MG.addrDecoding.unitDefaultIDUDID = 0x00000304;
            regAddrDbPtr->MG.addrDecoding.unitErrorAddrUEA = 0x00000308;
            {/*0x30c+n*8*/
                GT_U32    n;
                for(n = 0 ; n <= 5 ; n++) {
                    regAddrDbPtr->MG.addrDecoding.baseAddr[n] =
                        0x30c+n*8;
                }/* end of loop n */
            }/*0x30c+n*8*/


            {/*0x310+n*0x8*/
                GT_U32    n;
                for(n = 0 ; n <= 5 ; n++) {
                    regAddrDbPtr->MG.addrDecoding.sizeS[n] =
                        0x310+n*0x8;
                }/* end of loop n */
            }/*0x310+n*0x8*/


            {/*0x33c+n*4*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->MG.addrDecoding.highAddrRemapHA[n] =
                        0x33c+n*4;
                }/* end of loop n */
            }/*0x33c+n*4*/


            regAddrDbPtr->MG.addrDecoding.baseAddrEnableBARE = 0x0000034c;
            regAddrDbPtr->MG.addrDecoding.accessProtectAP = 0x00000350;

        }/*end of unit addrDecoding */


        {/*start of unit addrUpdateQueueConfig */
            regAddrDbPtr->MG.addrUpdateQueueConfig.generalAddrUpdateQueueBaseAddr = 0x000000c0;
            regAddrDbPtr->MG.addrUpdateQueueConfig.generalAddrUpdateQueueCtrl = 0x000000c4;
            regAddrDbPtr->MG.addrUpdateQueueConfig.FUAddrUpdateQueueBaseAddr = 0x000000c8;
            regAddrDbPtr->MG.addrUpdateQueueConfig.FUAddrUpdateQueueCtrl = 0x000000cc;

        }/*end of unit addrUpdateQueueConfig */


        {/*start of unit cpuPortConfigRegAndMIBCntrs */
            regAddrDbPtr->MG.cpuPortConfigRegAndMIBCntrs.cpuPortGoodFramesSentCntr = 0x00000060;
            regAddrDbPtr->MG.cpuPortConfigRegAndMIBCntrs.cpuPortMACTransErrorFramesSentCntr = 0x00000064;
            regAddrDbPtr->MG.cpuPortConfigRegAndMIBCntrs.cpuPortGoodOctetsSentCntr = 0x00000068;
            regAddrDbPtr->MG.cpuPortConfigRegAndMIBCntrs.cpuPortRxInternalDropCntr = 0x0000006c;
            regAddrDbPtr->MG.cpuPortConfigRegAndMIBCntrs.cpuPortGoodFramesReceivedCntr = 0x00000070;
            regAddrDbPtr->MG.cpuPortConfigRegAndMIBCntrs.cpuPortBadFramesReceivedCntr = 0x00000074;
            regAddrDbPtr->MG.cpuPortConfigRegAndMIBCntrs.cpuPortGoodOctetsReceivedCntr = 0x00000078;
            regAddrDbPtr->MG.cpuPortConfigRegAndMIBCntrs.cpuPortBadOctetsReceivedCntr = 0x0000007c;
            regAddrDbPtr->MG.cpuPortConfigRegAndMIBCntrs.cpuPortGlobalConfig = 0x000000a0;

        }/*end of unit cpuPortConfigRegAndMIBCntrs */


        {/*start of unit genxsReadDMARegs */
            regAddrDbPtr->MG.genxsReadDMARegs.genxsReadDMABufferPointer = 0x00000090;
            regAddrDbPtr->MG.genxsReadDMARegs.genxsReadDMAStartingAddr = 0x00000094;
            regAddrDbPtr->MG.genxsReadDMARegs.genxsReadDMACtrl = 0x00000098;

        }/*end of unit genxsReadDMARegs */


        {/*start of unit globalConfig */
            regAddrDbPtr->MG.globalConfig.addrCompletion = 0x00000000;
            regAddrDbPtr->MG.globalConfig.FTDLL = 0x0000000c;
            regAddrDbPtr->MG.globalConfig.lastReadTimeStamp = 0x00000040;
            regAddrDbPtr->MG.globalConfig.initDoneSummary = 0x00000048;
            regAddrDbPtr->MG.globalConfig.deviceID = 0x0000004c;
            regAddrDbPtr->MG.globalConfig.vendorID = 0x00000050;
            regAddrDbPtr->MG.globalConfig.globalCtrl = 0x00000058;
            regAddrDbPtr->MG.globalConfig.extGlobalCtrl = 0x0000005c;
            regAddrDbPtr->MG.globalConfig.extGlobalCtrl2 = 0x0000008c;
            regAddrDbPtr->MG.globalConfig.IOSelectors = 0x000000b0;
            regAddrDbPtr->MG.globalConfig.XBARArbiterCfg = 0x000000dc;

        }/*end of unit globalConfig */


        {/*start of unit globalInterrupt */
            regAddrDbPtr->MG.globalInterrupt.globalInterruptCause = 0x00000030;
            regAddrDbPtr->MG.globalInterrupt.globalInterruptMask = 0x00000034;
            regAddrDbPtr->MG.globalInterrupt.miscellaneousInterruptCause = 0x00000038;
            regAddrDbPtr->MG.globalInterrupt.miscellaneousInterruptMask = 0x0000003c;
            regAddrDbPtr->MG.globalInterrupt.stackPortsInterruptCause = 0x00000080;
            regAddrDbPtr->MG.globalInterrupt.stackPortsInterruptMask = 0x00000084;
            regAddrDbPtr->MG.globalInterrupt.SERInterruptsSummary = 0x000000a4;
            regAddrDbPtr->MG.globalInterrupt.SERInterruptsMask = 0x000000a8;

        }/*end of unit globalInterrupt */


        {/*start of unit interruptCoalescingConfig */
            regAddrDbPtr->MG.interruptCoalescingConfig.interruptCoalescingConfig = 0x000000e0;

        }/*end of unit interruptCoalescingConfig */


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

        }/*end of unit SDMARegs */


        {/*start of unit TWSIConfig */
            regAddrDbPtr->MG.TWSIConfig.TWSIGlobalConfig = 0x00000010;
            regAddrDbPtr->MG.TWSIConfig.TWSILastAddr = 0x00000014;
            regAddrDbPtr->MG.TWSIConfig.TWSITimeoutLimit = 0x00000018;
            regAddrDbPtr->MG.TWSIConfig.TWSIStateHistory0 = 0x0000001c;
            regAddrDbPtr->MG.TWSIConfig.TWSIStateHistory1 = 0x00000020;
            regAddrDbPtr->MG.TWSIConfig.TWSIInternalBaudRate = 0x0040000c;

        }/*end of unit TWSIConfig */


        {/*start of unit userDefined */
            {/*0xf0+n*4*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->MG.userDefined.userDefinedReg[n] =
                        0xf0+n*4;
                }/* end of loop n */
            }/*0xf0+n*4*/



        }/*end of unit userDefined */



    }/*end of unit MG - MG  */

    {/*start of unit egrAndTxqVer0 - Egress and Txq  */
        {/*start of unit oam8023ahLoopbackEnablePerPortReg */
            regAddrDbPtr->egrAndTxqVer0.oam8023ahLoopbackEnablePerPortReg.oam8023ahLoopbackEnablePerPortReg = 0x01800094;

        }/*end of unit oam8023ahLoopbackEnablePerPortReg */


        {/*start of unit bridgeEgrConfig */
            regAddrDbPtr->egrAndTxqVer0.bridgeEgrConfig.mcLocalEnableConfig = 0x01800008;
            regAddrDbPtr->egrAndTxqVer0.bridgeEgrConfig.egrFilteringReg0 = 0x01800010;
            regAddrDbPtr->egrAndTxqVer0.bridgeEgrConfig.egrFilteringReg1 = 0x01800014;
            regAddrDbPtr->egrAndTxqVer0.bridgeEgrConfig.IPMcRoutedPktsEgrFilterConfig = 0x018001d0;
            regAddrDbPtr->egrAndTxqVer0.bridgeEgrConfig.egrFilteringReg2 = 0x018001f4;

        }/*end of unit bridgeEgrConfig */


        {/*start of unit CCFC */
            regAddrDbPtr->egrAndTxqVer0.CCFC.BCNCtrl = 0x0a400000;
            regAddrDbPtr->egrAndTxqVer0.CCFC.pauseTriggerEnable = 0x0a40000c;
            regAddrDbPtr->egrAndTxqVer0.CCFC.BCNGlobalPauseThresholds = 0x0a400014;
            regAddrDbPtr->egrAndTxqVer0.CCFC.BCNInterruptCause = 0x0a400018;
            regAddrDbPtr->egrAndTxqVer0.CCFC.BCNInterruptMask = 0x0a40001c;
            regAddrDbPtr->egrAndTxqVer0.CCFC.portsBCNProfileReg[0] = 0x0a400020;
            regAddrDbPtr->egrAndTxqVer0.CCFC.portsBCNProfileReg[1] = 0x0a400024;
            regAddrDbPtr->egrAndTxqVer0.CCFC.portsBCNProfileReg[2] = 0x0a400028;
            regAddrDbPtr->egrAndTxqVer0.CCFC.portsSpeedIndexesReg[0] = 0x0a400034;
            regAddrDbPtr->egrAndTxqVer0.CCFC.portsSpeedIndexesReg[1] = 0x0a400038;
            regAddrDbPtr->egrAndTxqVer0.CCFC.portsSpeedIndexesReg[2] = 0x0a40003c;
            regAddrDbPtr->egrAndTxqVer0.CCFC.globalBuffersCntr = 0x0a4001fc;
            regAddrDbPtr->egrAndTxqVer0.CCFC.BCNPortsTriggerCntrsConfig = 0x0a401000;
            regAddrDbPtr->egrAndTxqVer0.CCFC.BCNQueuesTriggerCntrsConfig = 0x0a401004;
            regAddrDbPtr->egrAndTxqVer0.CCFC.globalXoffCntr = 0x0a40100c;
            regAddrDbPtr->egrAndTxqVer0.CCFC.portBCNTriggerCntr = 0x0a401014;
            regAddrDbPtr->egrAndTxqVer0.CCFC.queueBCNTriggerCntr = 0x0a401018;

        }/*end of unit CCFC */


        {/*start of unit CNCModes */
            regAddrDbPtr->egrAndTxqVer0.CNCModes.CNCModesReg = 0x018000a0;

        }/*end of unit CNCModes */


        {/*start of unit dequeueSWRR */
            regAddrDbPtr->egrAndTxqVer0.dequeueSWRR.dequeueSWRRWeightReg[0] = 0x018000b0;
            regAddrDbPtr->egrAndTxqVer0.dequeueSWRR.dequeueSWRRWeightReg[1] = 0x018000b4;
            regAddrDbPtr->egrAndTxqVer0.dequeueSWRR.dequeueSWRRWeightReg[2] = 0x018000b8;
            regAddrDbPtr->egrAndTxqVer0.dequeueSWRR.dequeueSWRRWeightReg[3] = 0x018000bc;

        }/*end of unit dequeueSWRR */


        {/*start of unit deviceMapTable */
            {/*0x1a40004+n*0x10*/
                GT_U32    n;
                for(n = 0 ; n <= 31 ; n++) {
                    regAddrDbPtr->egrAndTxqVer0.deviceMapTable.deviceMapTableEntry[n] =
                        0x1a40004+n*0x10;
                }/* end of loop n */
            }/*0x1a40004+n*0x10*/



        }/*end of unit deviceMapTable */


        {/*start of unit DPToCFIMap */
            regAddrDbPtr->egrAndTxqVer0.DPToCFIMap.DP2CFIEnablePerPortReg = 0x018000a4;
            regAddrDbPtr->egrAndTxqVer0.DPToCFIMap.DP2CFIReg = 0x018000a8;

        }/*end of unit DPToCFIMap */


        {/*start of unit egrForwardingRestrictions */
            regAddrDbPtr->egrAndTxqVer0.egrForwardingRestrictions.fromCPUPktsFilterConfig = 0x018001d8;
            regAddrDbPtr->egrAndTxqVer0.egrForwardingRestrictions.bridgedPktsFilterConfig = 0x018001dc;
            regAddrDbPtr->egrAndTxqVer0.egrForwardingRestrictions.routedPktsFilterConfig = 0x018001e0;

        }/*end of unit egrForwardingRestrictions */


        {/*start of unit egrRateShapersConfig */
            regAddrDbPtr->egrAndTxqVer0.egrRateShapersConfig.tokenBucketUpdateRateAndMCFIFOConfig = 0x0180000c;
            regAddrDbPtr->egrAndTxqVer0.egrRateShapersConfig.tokenBucketUpdateRate1 = 0x01800248;
            {/*0x1ac0000+k * 0x8000 + n * 0x200 and 0x1ac0000+k * 0x8000 + n * 0x200*/
                GT_U32    n,k;
                for(n = 0 ; n <= 27 ; n++) {
                    for(k = 0 ; k <= 7 ; k++) {
                        regAddrDbPtr->egrAndTxqVer0.egrRateShapersConfig.portTCTokenBucketConfig[n][k] =
                            0x1ac0000+k * 0x8000 + n * 0x200;
                    }/* end of loop k */
                }/* end of loop n */
                for(n = 63 ; n <= 63 ; n++) {
                    for(k = 0 ; k <= 7 ; k++) {
                        regAddrDbPtr->egrAndTxqVer0.egrRateShapersConfig.portTCTokenBucketConfig[(n - 63) + 28][k] =
                            0x1ac0000+k * 0x8000 + n * 0x200;
                    }/* end of loop k */
                }/* end of loop n */
            }/*0x1ac0000+k * 0x8000 + n * 0x200 and 0x1ac0000+k * 0x8000 + n * 0x200*/


            {/*0x1ac0008+n * 0x200 and 0x1ac0008+n * 0x200*/
                GT_U32    n;
                for(n = 0 ; n <= 27 ; n++) {
                    regAddrDbPtr->egrAndTxqVer0.egrRateShapersConfig.portTokenBucketConfig[n] =
                        0x1ac0008+n * 0x200;
                }/* end of loop n */
                for(n = 63 ; n <= 63 ; n++) {
                    regAddrDbPtr->egrAndTxqVer0.egrRateShapersConfig.portTokenBucketConfig[(n - 63) + 28] =
                        0x1ac0008+n * 0x200;
                }/* end of loop n */
            }/*0x1ac0008+n * 0x200 and 0x1ac0008+n * 0x200*/



        }/*end of unit egrRateShapersConfig */


        {/*start of unit egrSTCTable */
            {/*0x1d40000+n*0x200*/
                GT_U32    n;
                for(n = 0 ; n <= 27 ; n++) {
                    regAddrDbPtr->egrAndTxqVer0.egrSTCTable.portEgrSTCTableEntryWord0[n] =
                        0x1d40000+n*0x200;
                }/* end of loop n */
            }/*0x1d40000+n*0x200*/


            {/*0x1d40004+n*0x200*/
                GT_U32    n;
                for(n = 0 ; n <= 27 ; n++) {
                    regAddrDbPtr->egrAndTxqVer0.egrSTCTable.portEgrSTCTableEntryWord1[n] =
                        0x1d40004+n*0x200;
                }/* end of loop n */
            }/*0x1d40004+n*0x200*/


            {/*0x1d40008+n*0x200*/
                GT_U32    n;
                for(n = 0 ; n <= 27 ; n++) {
                    regAddrDbPtr->egrAndTxqVer0.egrSTCTable.portEgrSTCTableEntryWord2[n] =
                        0x1d40008+n*0x200;
                }/* end of loop n */
            }/*0x1d40008+n*0x200*/



        }/*end of unit egrSTCTable */


        {/*start of unit fastStackFailover */
            {/*0x1e40000+n*0x10*/
                GT_U32    n;
                for(n = 0 ; n <= 27 ; n++) {
                    regAddrDbPtr->egrAndTxqVer0.fastStackFailover.secondaryTargetPortMapTableEntry[n] =
                        0x1e40000+n*0x10;
                }/* end of loop n */
            }/*0x1e40000+n*0x10*/



        }/*end of unit fastStackFailover */


        {/*start of unit GPPRegs */
            regAddrDbPtr->egrAndTxqVer0.GPPRegs.GPPInputReg = 0x018001bc;
            regAddrDbPtr->egrAndTxqVer0.GPPRegs.GPPOutputReg = 0x018001c4;
            regAddrDbPtr->egrAndTxqVer0.GPPRegs.GPPIOCtrlReg = 0x018001c8;

        }/*end of unit GPPRegs */


        {/*start of unit linkListCtrl */
            regAddrDbPtr->egrAndTxqVer0.linkListCtrl.linkListCtrlConfig = 0x018000d0;

        }/*end of unit linkListCtrl */


        {/*start of unit mirroredPktsToAnalyzerPortDescsLimit */
            regAddrDbPtr->egrAndTxqVer0.mirroredPktsToAnalyzerPortDescsLimit.mirroredPktsToAnalyzerPortDescsLimitConfig = 0x01800024;

        }/*end of unit mirroredPktsToAnalyzerPortDescsLimit */


        {/*start of unit perPortTransmitQueueConfig */
            {/*0x1800080+n * 0x200 and 0x1800080+n * 0x200*/
                GT_U32    n;
                for(n = 0 ; n <= 27 ; n++) {
                    regAddrDbPtr->egrAndTxqVer0.perPortTransmitQueueConfig.portTxqConfig[n] =
                        0x1800080 + n * 0x200;
                }/* end of loop n */
                for(n = 63 ; n <= 63 ; n++) {
                    regAddrDbPtr->egrAndTxqVer0.perPortTransmitQueueConfig.portTxqConfig[(n - 63) + 28] =
                        0x1800080 + n * 0x200;
                }/* end of loop n */
            }/*0x1800080+n * 0x200 and 0x1800080+n * 0x200*/



        }/*end of unit perPortTransmitQueueConfig */


        {/*start of unit portIsolationConfig */
            {/*0x1e40004+0x10*n*/
                GT_U32    n;
                for(n = 0 ; n <= 2175 ; n++) {
                    regAddrDbPtr->egrAndTxqVer0.portIsolationConfig.L2PortIsolationTableEntry[n] =
                        0x1e40004+0x10*n;
                }/* end of loop n */
            }/*0x1e40004+0x10*n*/


            {/*0x1e40008+0x10*n*/
                GT_U32    n;
                for(n = 0 ; n <= 2175 ; n++) {
                    regAddrDbPtr->egrAndTxqVer0.portIsolationConfig.L3PortIsolationTableEntry[n] =
                        0x1e40008+0x10*n;
                }/* end of loop n */
            }/*0x1e40008+0x10*n*/



        }/*end of unit portIsolationConfig */


        {/*start of unit portL1OverheadConfig */
            {/*0x1a80000+n * 0x200 and 0x1a80000+n * 0x200*/
                GT_U32    n;
                for(n = 0 ; n <= 27 ; n++) {
                    regAddrDbPtr->egrAndTxqVer0.portL1OverheadConfig.portL1OverheadConfig[n] =
                        0x1a80000+n * 0x200;
                }/* end of loop n */
                for(n = 63 ; n <= 63 ; n++) {
                    regAddrDbPtr->egrAndTxqVer0.portL1OverheadConfig.portL1OverheadConfig[(n - 63) + 28] =
                        0x1a80000+n * 0x200;
                }/* end of loop n */
            }/*0x1a80000+n * 0x200 and 0x1a80000+n * 0x200*/



        }/*end of unit portL1OverheadConfig */


        {/*start of unit resourceSharingRegs */
            {/*0x1800028+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->egrAndTxqVer0.resourceSharingRegs.priorityReg[n] =
                        0x1800028+n*0x4;
                }/* end of loop n */
            }/*0x1800028+n*0x4*/



        }/*end of unit resourceSharingRegs */


        {/*start of unit sourceIDEgrFiltering */
            regAddrDbPtr->egrAndTxqVer0.sourceIDEgrFiltering.ucSrcIDEgrFilteringConfig = 0x018001d4;
            {/*0x1a4000c+m*0x10*/
                GT_U32    m;
                for(m = 0 ; m <= 31 ; m++) {
                    regAddrDbPtr->egrAndTxqVer0.sourceIDEgrFiltering.srcIDEgrFilteringTableEntry[m] =
                        0x1a4000c+m*0x10;
                }/* end of loop m */
            }/*0x1a4000c+m*0x10*/



        }/*end of unit sourceIDEgrFiltering */


        {/*start of unit stackTrafficSeparation */
            regAddrDbPtr->egrAndTxqVer0.stackTrafficSeparation.isStackPortReg = 0x01800098;
            {/*0x1e80000+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 7 ; n++) {
                    regAddrDbPtr->egrAndTxqVer0.stackTrafficSeparation.stackTCRemapTableEntry[n] =
                        0x1e80000+n*0x4;
                }/* end of loop n */
            }/*0x1e80000+n*0x4*/



        }/*end of unit stackTrafficSeparation */


        {/*start of unit statisticalAndCPUTrigEgrMirrToAnalyzerPort */
            regAddrDbPtr->egrAndTxqVer0.statisticalAndCPUTrigEgrMirrToAnalyzerPort.statisticalAndCPUTrigEgrMirrToAnalyzerPortConfig = 0x01800020;

        }/*end of unit statisticalAndCPUTrigEgrMirrToAnalyzerPort */


        {/*start of unit tailDropProfileConfig */
            {/*0x1800030+n*0x4 and 0x1800030+0x90+(n-4)*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->egrAndTxqVer0.tailDropProfileConfig.profilePortTailDropLimitsConfig[n] =
                        0x1800030+n*0x4;
                }/* end of loop n */
                for(n = 4 ; n <= 7 ; n++) {
                    regAddrDbPtr->egrAndTxqVer0.tailDropProfileConfig.profilePortTailDropLimitsConfig[(n - 4) + 4] =
                        0x1800030+0x90+(n-4)*0x4;
                }/* end of loop n */
            }/*0x1800030+n*0x4 and 0x1800030+0x90+(n-4)*0x4*/


            regAddrDbPtr->egrAndTxqVer0.tailDropProfileConfig.queueWRTDMask = 0x01800230;
            regAddrDbPtr->egrAndTxqVer0.tailDropProfileConfig.portWRTDMask = 0x01800234;
            regAddrDbPtr->egrAndTxqVer0.tailDropProfileConfig.poolWRTDMask = 0x01800238;
            {/*0x1800260+0x4*p*/
                GT_U32    p;
                for(p = 0 ; p <= 7 ; p++) {
                    regAddrDbPtr->egrAndTxqVer0.tailDropProfileConfig.profilePortWRTDEn[p] =
                        0x1800260+0x4*p;
                }/* end of loop p */
            }/*0x1800260+0x4*p*/


            {/*0x18002a0+0x4*p*/
                GT_U32    p;
                for(p = 0 ; p <= 7 ; p++) {
                    regAddrDbPtr->egrAndTxqVer0.tailDropProfileConfig.profileQueueWRTDEn[p] =
                        0x18002a0+0x4*p;
                }/* end of loop p */
            }/*0x18002a0+0x4*p*/


            {/*0x18002c0+0x4*p*/
                GT_U32    p;
                for(p = 0 ; p <= 7 ; p++) {
                    regAddrDbPtr->egrAndTxqVer0.tailDropProfileConfig.profilePoolWRTDEn[p] =
                        0x18002c0+0x4*p;
                }/* end of loop p */
            }/*0x18002c0+0x4*p*/


            {/*0x1940000+n*0x08+p*0x400*/
                GT_U32    p,n;
                for(p = 0 ; p <= 7 ; p++) {
                    for(n = 0 ; n <= 7 ; n++) {
                        regAddrDbPtr->egrAndTxqVer0.tailDropProfileConfig.profileTCDP0TailDropLimits[p][n] =
                            0x1940000+n*0x08+p*0x400;
                    }/* end of loop n */
                }/* end of loop p */
            }/*0x1940000+n*0x08+p*0x400*/


            {/*0x1940004+n*0x08+p*0x400*/
                GT_U32    p,n;
                for(p = 0 ; p <= 7 ; p++) {
                    for(n = 0 ; n <= 7 ; n++) {
                        regAddrDbPtr->egrAndTxqVer0.tailDropProfileConfig.profileTCDP1TailDropLimits[p][n] =
                            0x1940004+n*0x08+p*0x400;
                    }/* end of loop n */
                }/* end of loop p */
            }/*0x1940004+n*0x08+p*0x400*/



        }/*end of unit tailDropProfileConfig */


        {/*start of unit transmitQueueTxAndEgrMIBCntrs */
            {/*0x1800140+n*0x20*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->egrAndTxqVer0.transmitQueueTxAndEgrMIBCntrs.txqMIBCntrsSetConfig[n] =
                        0x1800140+n*0x20;
                }/* end of loop n */
            }/*0x1800140+n*0x20*/


            {/*0x1b40144+n*0x20*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->egrAndTxqVer0.transmitQueueTxAndEgrMIBCntrs.setOutgoingUcPktCntr[n] =
                        0x1b40144+n*0x20;
                }/* end of loop n */
            }/*0x1b40144+n*0x20*/


            {/*0x1b40148+n*0x20*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->egrAndTxqVer0.transmitQueueTxAndEgrMIBCntrs.setOutgoingMcPktCntr[n] =
                        0x1b40148+n*0x20;
                }/* end of loop n */
            }/*0x1b40148+n*0x20*/


            {/*0x1b4014c+n*0x20*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->egrAndTxqVer0.transmitQueueTxAndEgrMIBCntrs.setOutgoingBcPktCount[n] =
                        0x1b4014c+n*0x20;
                }/* end of loop n */
            }/*0x1b4014c+n*0x20*/


            {/*0x1b40150+n*0x20*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->egrAndTxqVer0.transmitQueueTxAndEgrMIBCntrs.setBridgeEgrFilteredPktCount[n] =
                        0x1b40150+n*0x20;
                }/* end of loop n */
            }/*0x1b40150+n*0x20*/


            {/*0x1b40154+n*0x20*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->egrAndTxqVer0.transmitQueueTxAndEgrMIBCntrs.setTailDroppedPktCntr[n] =
                        0x1b40154+n*0x20;
                }/* end of loop n */
            }/*0x1b40154+n*0x20*/


            {/*0x1b40158+n*0x20*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->egrAndTxqVer0.transmitQueueTxAndEgrMIBCntrs.setCtrlPktCntr[n] =
                        0x1b40158+n*0x20;
                }/* end of loop n */
            }/*0x1b40158+n*0x20*/


            {/*0x1b4015c+n*0x20*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->egrAndTxqVer0.transmitQueueTxAndEgrMIBCntrs.setEgrForwardingRestrictionDroppedPktsCntr[n] =
                        0x1b4015c+n*0x20;
                }/* end of loop n */
            }/*0x1b4015c+n*0x20*/


            {/*0x1b40180+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->egrAndTxqVer0.transmitQueueTxAndEgrMIBCntrs.setMcFIFODroppedPktsCntr[n] =
                        0x1b40180+n*0x4;
                }/* end of loop n */
            }/*0x1b40180+n*0x4*/



        }/*end of unit transmitQueueTxAndEgrMIBCntrs */


        {/*start of unit transmitQueueGlobalConfig */
            regAddrDbPtr->egrAndTxqVer0.transmitQueueGlobalConfig.transmitQueueCtrl = 0x01800000;
            regAddrDbPtr->egrAndTxqVer0.transmitQueueGlobalConfig.transmitQueueExtCtrl = 0x01800004;
            regAddrDbPtr->egrAndTxqVer0.transmitQueueGlobalConfig.transmitQueueExtCtrl1 = 0x018001b0;
            regAddrDbPtr->egrAndTxqVer0.transmitQueueGlobalConfig.transmitQueueResourceSharing = 0x018001f0;
            regAddrDbPtr->egrAndTxqVer0.transmitQueueGlobalConfig.BZTransmitQueueMetalFix = 0x0180024c;
            regAddrDbPtr->egrAndTxqVer0.transmitQueueGlobalConfig.portsLinkEnable = 0x018002f0;

        }/*end of unit transmitQueueGlobalConfig */


        {/*start of unit transmitQueueInterrupt */
            regAddrDbPtr->egrAndTxqVer0.transmitQueueInterrupt.transmitQueueInterruptSummaryCause = 0x01800100;
            regAddrDbPtr->egrAndTxqVer0.transmitQueueInterrupt.transmitQueueInterruptSummaryMask = 0x01800104;
            regAddrDbPtr->egrAndTxqVer0.transmitQueueInterrupt.transmitQueueFlushDoneInterruptCause = 0x01800110;
            regAddrDbPtr->egrAndTxqVer0.transmitQueueInterrupt.transmitQueueFlushInterruptMask = 0x01800114;
            regAddrDbPtr->egrAndTxqVer0.transmitQueueInterrupt.transmitQueueGeneralInterruptCause = 0x01800118;
            regAddrDbPtr->egrAndTxqVer0.transmitQueueInterrupt.transmitQueueGeneralInterruptMask = 0x0180011c;
            regAddrDbPtr->egrAndTxqVer0.transmitQueueInterrupt.egrSTCInterruptCause = 0x01800130;
            regAddrDbPtr->egrAndTxqVer0.transmitQueueInterrupt.egrSTCInterruptMask = 0x01800134;
            regAddrDbPtr->egrAndTxqVer0.transmitQueueInterrupt.transmitQueueDescFullInterruptSummaryCause = 0x0180018c;
            regAddrDbPtr->egrAndTxqVer0.transmitQueueInterrupt.GPPInterruptMaskReg = 0x018001c0;
            regAddrDbPtr->egrAndTxqVer0.transmitQueueInterrupt.GPPInterruptCauseReg = 0x018001cc;
            regAddrDbPtr->egrAndTxqVer0.transmitQueueInterrupt.transmitQueueFullInterruptMask = 0x018001ec;

        }/*end of unit transmitQueueInterrupt */


        {/*start of unit transmitQueueTotalBuffersLimit */
            regAddrDbPtr->egrAndTxqVer0.transmitQueueTotalBuffersLimit.totalBufferLimitConfig = 0x01800084;

        }/*end of unit transmitQueueTotalBuffersLimit */


        {/*start of unit transmitSchedulerProfilesConfig */
            {/*0x1800040+p*0x10 and 0x1800040+0xEC0+(p-4)*0x10*/
                GT_U32    p;
                for(p = 0 ; p <= 3 ; p++) {
                    regAddrDbPtr->egrAndTxqVer0.transmitSchedulerProfilesConfig.profileSDWRRWeightsConfigReg0[p] =
                        0x1800040+p*0x10;
                }/* end of loop p */
                for(p = 4 ; p <= 7 ; p++) {
                    regAddrDbPtr->egrAndTxqVer0.transmitSchedulerProfilesConfig.profileSDWRRWeightsConfigReg0[(p - 4) + 4] =
                        0x1800040+0xEC0+(p-4)*0x10;
                }/* end of loop p */
            }/*0x1800040+p*0x10 and 0x1800040+0xEC0+(p-4)*0x10*/


            {/*0x1800044+p*0x10 and 0x1800044+0xEC0+(p-4)*0x10*/
                GT_U32    p;
                for(p = 0 ; p <= 3 ; p++) {
                    regAddrDbPtr->egrAndTxqVer0.transmitSchedulerProfilesConfig.profileSDWRRWeightsConfigReg1[p] =
                        0x1800044+p*0x10;
                }/* end of loop p */
                for(p = 4 ; p <= 7 ; p++) {
                    regAddrDbPtr->egrAndTxqVer0.transmitSchedulerProfilesConfig.profileSDWRRWeightsConfigReg1[(p - 4) + 4] =
                        0x1800044+0xEC0+(p-4)*0x10;
                }/* end of loop p */
            }/*0x1800044+p*0x10 and 0x1800044+0xEC0+(p-4)*0x10*/


            {/*0x1800048+p*0x10 and 0x1800048+0xEC0+(p-4)*0x10*/
                GT_U32    p;
                for(p = 0 ; p <= 3 ; p++) {
                    regAddrDbPtr->egrAndTxqVer0.transmitSchedulerProfilesConfig.profileSDWRRAndStrictPriorityConfig[p] =
                        0x1800048+p*0x10;
                }/* end of loop p */
                for(p = 4 ; p <= 7 ; p++) {
                    regAddrDbPtr->egrAndTxqVer0.transmitSchedulerProfilesConfig.profileSDWRRAndStrictPriorityConfig[(p - 4) + 4] =
                        0x1800048+0xEC0+(p-4)*0x10;
                }/* end of loop p */
            }/*0x1800048+p*0x10 and 0x1800048+0xEC0+(p-4)*0x10*/



        }/*end of unit transmitSchedulerProfilesConfig */


        {/*start of unit trunksFilteringAndMcDistConfig */
            {/*0x1a40000+n*0x10*/
                GT_U32    n;
                for(n = 0 ; n <= 127 ; n++) {
                    regAddrDbPtr->egrAndTxqVer0.trunksFilteringAndMcDistConfig.trunkNonTrunkMembersTable[n] =
                        0x1a40000+n*0x10;
                }/* end of loop n */
            }/*0x1a40000+n*0x10*/


            {/*0x1a40008+n*0x10*/
                GT_U32    n;
                for(n = 0 ; n <= 7 ; n++) {
                    regAddrDbPtr->egrAndTxqVer0.trunksFilteringAndMcDistConfig.designatedTrunkPortEntryTable[n] =
                        0x1a40008+n*0x10;
                }/* end of loop n */
            }/*0x1a40008+n*0x10*/



        }/*end of unit trunksFilteringAndMcDistConfig */


        {/*start of unit txqCntrs */
            regAddrDbPtr->egrAndTxqVer0.txqCntrs.totalBuffersCntr = 0x01800088;
            regAddrDbPtr->egrAndTxqVer0.txqCntrs.totalDescCntr = 0x0180008c;
            regAddrDbPtr->egrAndTxqVer0.txqCntrs.totalBufferCntr = 0x01800090;
            regAddrDbPtr->egrAndTxqVer0.txqCntrs.totalAllocatedDescsCntr = 0x01800120;
            regAddrDbPtr->egrAndTxqVer0.txqCntrs.mcDescsCntr = 0x01800124;
            regAddrDbPtr->egrAndTxqVer0.txqCntrs.snifferDescsCntr = 0x01800188;
            {/*0x1840000+n*0x200 and 0x1840000+n*0x200*/
                GT_U32    n;
                for(n = 0 ; n <= 27 ; n++) {
                    regAddrDbPtr->egrAndTxqVer0.txqCntrs.portDescsCntr[n] =
                        0x1840000+n*0x200;
                }/* end of loop n */
                for(n = 63 ; n <= 63 ; n++) {
                    regAddrDbPtr->egrAndTxqVer0.txqCntrs.portDescsCntr[(n - 63) + 28] =
                        0x1840000+n*0x200;
                }/* end of loop n */
            }/*0x1840000+n*0x200 and 0x1840000+n*0x200*/


            {/*0x1880000+t*0x8000 + n * 0x200 and 0x1880000+t*0x8000 + n * 0x200*/
                GT_U32    n,t;
                for(n = 0 ; n <= 27 ; n++) {
                    for(t = 0 ; t <= 7 ; t++) {
                        regAddrDbPtr->egrAndTxqVer0.txqCntrs.portTCDescCntrs[n][t] =
                            0x1880000+t*0x8000 + n * 0x200;
                    }/* end of loop t */
                }/* end of loop n */
                for(n = 63 ; n <= 63 ; n++) {
                    for(t = 0 ; t <= 7 ; t++) {
                        regAddrDbPtr->egrAndTxqVer0.txqCntrs.portTCDescCntrs[(n - 63) + 28][t] =
                            0x1880000+t*0x8000 + n * 0x200;
                    }/* end of loop t */
                }/* end of loop n */
            }/*0x1880000+t*0x8000 + n * 0x200 and 0x1880000+t*0x8000 + n * 0x200*/


            {/*0x18c0000+n*0x200 and 0x18c0000+n*0x200*/
                GT_U32    n;
                for(n = 0 ; n <= 27 ; n++) {
                    regAddrDbPtr->egrAndTxqVer0.txqCntrs.portBuffersCntr[n] =
                        0x18c0000+n*0x200;
                }/* end of loop n */
                for(n = 63 ; n <= 63 ; n++) {
                    regAddrDbPtr->egrAndTxqVer0.txqCntrs.portBuffersCntr[(n - 63) + 28] =
                        0x18c0000+n*0x200;
                }/* end of loop n */
            }/*0x18c0000+n*0x200 and 0x18c0000+n*0x200*/


            {/*0x1900000+t*0x8000+n*0x200 and 0x1900000+t*0x8000+n*0x200*/
                GT_U32    n,t;
                for(n = 0 ; n <= 27 ; n++) {
                    for(t = 0 ; t <= 7 ; t++) {
                        regAddrDbPtr->egrAndTxqVer0.txqCntrs.portTCBuffersCntrs[n][t] =
                            0x1900000+t*0x8000+n*0x200;
                    }/* end of loop t */
                }/* end of loop n */
                for(n = 63 ; n <= 63 ; n++) {
                    for(t = 0 ; t <= 7 ; t++) {
                        regAddrDbPtr->egrAndTxqVer0.txqCntrs.portTCBuffersCntrs[(n - 63) + 28][t] =
                            0x1900000+t*0x8000+n*0x200;
                    }/* end of loop t */
                }/* end of loop n */
            }/*0x1900000+t*0x8000+n*0x200 and 0x1900000+t*0x8000+n*0x200*/



        }/*end of unit txqCntrs */


        {/*start of unit VLANAndMcGroupAndSpanStateGroupTables */
            regAddrDbPtr->egrAndTxqVer0.VLANAndMcGroupAndSpanStateGroupTables.globalConfig = 0x0a000000;
            regAddrDbPtr->egrAndTxqVer0.VLANAndMcGroupAndSpanStateGroupTables.VLTTablesAccessDataReg[5] = 0x0a000100;
            regAddrDbPtr->egrAndTxqVer0.VLANAndMcGroupAndSpanStateGroupTables.VLTTablesAccessDataReg[4] = 0x0a000104;
            regAddrDbPtr->egrAndTxqVer0.VLANAndMcGroupAndSpanStateGroupTables.VLTTablesAccessDataReg[3] = 0x0a000108;
            regAddrDbPtr->egrAndTxqVer0.VLANAndMcGroupAndSpanStateGroupTables.VLTTablesAccessDataReg[2] = 0x0a00010c;
            regAddrDbPtr->egrAndTxqVer0.VLANAndMcGroupAndSpanStateGroupTables.VLTTablesAccessDataReg[1] = 0x0a000110;
            regAddrDbPtr->egrAndTxqVer0.VLANAndMcGroupAndSpanStateGroupTables.VLTTablesAccessDataReg[0] = 0x0a000114;
            regAddrDbPtr->egrAndTxqVer0.VLANAndMcGroupAndSpanStateGroupTables.VLTTablesAccessCtrl = 0x0a000118;

        }/*end of unit VLANAndMcGroupAndSpanStateGroupTables */


        {/*start of unit XGCtrl */
            regAddrDbPtr->egrAndTxqVer0.XGCtrl.XGCtrlReg = 0x01800180;

        }/*end of unit XGCtrl */


        {/*start of unit XSMIConfig */
            regAddrDbPtr->egrAndTxqVer0.XSMIConfig.XSMIManagement = 0x01cc0000;
            regAddrDbPtr->egrAndTxqVer0.XSMIConfig.XSMIAddr = 0x01cc0008;

        }/*end of unit XSMIConfig */



    }/*end of unit egrAndTxqVer0 - Egress and Txq  */

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

    {/*start of unit ucRouter - Unicast Router  */
        regAddrDbPtr->ucRouter.ucRoutingEngineConfigReg = 0x02800e3c;
    }/*end of unit ucRouter - Unicast Router  */

    {/*start of unit BM - BM  */
        {/*start of unit BMRegs */
            regAddrDbPtr->BM.BMRegs.bufferManagementInterruptCause0 = 0x03000040;
            regAddrDbPtr->BM.BMRegs.bufferManagementInterruptMask0 = 0x03000044;
            regAddrDbPtr->BM.BMRegs.bufferManagementInterruptCause1 = 0x03000048;
            regAddrDbPtr->BM.BMRegs.bufferManagementInterruptMask1 = 0x0300004c;

            regAddrDbPtr->BM.BMRegs.bufferManagementNumberOfAgedBuffers = 0x03000054;
            regAddrDbPtr->BM.BMRegs.totalBuffersAllocationCntr = 0x03000058;
            regAddrDbPtr->BM.BMRegs.bufferManagementPktCntr = 0x0300005c;
            regAddrDbPtr->BM.BMRegs.networkPortsGroupAllocationCntr = 0x03000080;
            regAddrDbPtr->BM.BMRegs.hyperGStackPortsGroupAllocationCntr = 0x03000084;
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
            regAddrDbPtr->BM.BMRegs.bufferManagementNetworkPortGroupLimitsConfig = 0x03000004;
            regAddrDbPtr->BM.BMRegs.bufferManagementHyperGStackPortsGroupLimits = 0x03000008;
            regAddrDbPtr->BM.BMRegs.bufferManagementBuffersConfig = 0x03000014;

            regAddrDbPtr->BM.BMRegs.portsBuffersLimitProfileConfig[0] = 0x03000020;
            regAddrDbPtr->BM.BMRegs.portsBuffersLimitProfileConfig[1] = 0x03000024;
            regAddrDbPtr->BM.BMRegs.portsBuffersLimitProfileConfig[2] = 0x03000028;
            regAddrDbPtr->BM.BMRegs.portsBuffersLimitProfileConfig[3] = 0x0300002c;
            regAddrDbPtr->BM.BMRegs.buffersLimitsProfileAssociation0 = 0x03000030;
            regAddrDbPtr->BM.BMRegs.buffersLimitsProfileAssociation1 = 0x03000034;

        }/*end of unit BMRegs */



    }/*end of unit BM - BM  */

    {/*start of unit LMS - LMS  */
        {/*start of unit LMS1LMS1Group0 */
            regAddrDbPtr->LMS.LMS1LMS1Group0.LEDInterface1CtrlReg1AndClass6ManipulationforPorts12Through23 = 0x05804100;
            regAddrDbPtr->LMS.LMS1LMS1Group0.LEDInterface1Group23ConfigRegforPorts12Through23 = 0x05804104;
            regAddrDbPtr->LMS.LMS1LMS1Group0.LEDInterface1Class23ManipulationRegforPorts12Through23 = 0x05804108;
            regAddrDbPtr->LMS.LMS1LMS1Group0.LEDInterface1Class5ManipulationRegforPorts12Through23 = 0x0580410c;
            regAddrDbPtr->LMS.LMS1LMS1Group0.stackLEDInterface1Class1011ManipulationRegforStackPort = 0x05805100;
            regAddrDbPtr->LMS.LMS1LMS1Group0.stackLEDInterface1Group01ConfigRegforStackPort = 0x05805104;

            regAddrDbPtr->LMS.LMS1LMS1Group0.MIBCntrsCtrlReg0forPorts18Through23 = 0x05804020;

            regAddrDbPtr->LMS.LMS1LMS1Group0.portsGOP3AndStackMIBsInterruptCause = 0x05804000;
            regAddrDbPtr->LMS.LMS1LMS1Group0.portsGOP3AndStackMIBsInterruptMask = 0x05804004;

            regAddrDbPtr->LMS.LMS1LMS1Group0.flowCtrlAdvertiseForFiberMediaSelectedConfigReg1forPorts12Through23 = 0x05804024;
            regAddrDbPtr->LMS.LMS1LMS1Group0.PHYAddrReg3forPorts18Through23 = 0x05804030;
            regAddrDbPtr->LMS.LMS1LMS1Group0.PHYAutoNegConfigReg3 = 0x05804034;

        }/*end of unit LMS1LMS1Group0 */


        {/*start of unit LMS1 */
            regAddrDbPtr->LMS.LMS1.LEDInterface1CtrlReg0forPorts12Through23AndStackPort = 0x05004100;
            regAddrDbPtr->LMS.LMS1.LEDInterface1Group01ConfigRegforPorts12Through23 = 0x05004104;
            regAddrDbPtr->LMS.LMS1.LEDInterface1Class01ManipulationRegforPorts12Through23 = 0x05004108;
            regAddrDbPtr->LMS.LMS1.LEDInterface1Class4ManipulationRegforPorts12Through23 = 0x0500410c;
            regAddrDbPtr->LMS.LMS1.stackLEDInterface1Class04ManipulationRegforStackPorts = 0x05005100;
            regAddrDbPtr->LMS.LMS1.stackLEDInterface1Class59ManipulationRegforStackPorts = 0x05005104;
            regAddrDbPtr->LMS.LMS1.LEDInterface1FlexlinkPortsDebugSelectRegforStackPort = 0x05005110;
            regAddrDbPtr->LMS.LMS1.LEDInterface1FlexlinkPortsDebugSelectReg1forStackPort = 0x05005114;

            regAddrDbPtr->LMS.LMS1.portsInterruptSummary = 0x05004010;

            regAddrDbPtr->LMS.LMS1.MIBCntrsCtrlReg0forPorts12Through17 = 0x05004020;

            regAddrDbPtr->LMS.LMS1.portsGOP2MIBsInterruptCause = 0x05004000;
            regAddrDbPtr->LMS.LMS1.portsGOP2MIBsInterruptMask = 0x05004004;

            regAddrDbPtr->LMS.LMS1.PHYAddrReg2forPorts12Through17 = 0x05004030;
            regAddrDbPtr->LMS.LMS1.PHYAutoNegConfigReg2 = 0x05004034;
            regAddrDbPtr->LMS.LMS1.SMI1Management = 0x05004054;
            regAddrDbPtr->LMS.LMS1.LMS1MiscConfigs = 0x05004200;

        }/*end of unit LMS1 */


        {/*start of unit LMS0LMS0Group0 */
            regAddrDbPtr->LMS.LMS0LMS0Group0.LEDInterface0CtrlReg1AndClass6ManipulationRegforPorts0Through11 = 0x04804100;
            regAddrDbPtr->LMS.LMS0LMS0Group0.LEDInterface0Group23ConfigRegforPorts0Through11 = 0x04804104;
            regAddrDbPtr->LMS.LMS0LMS0Group0.LEDInterface0Class23ManipulationRegforPorts0Through11 = 0x04804108;
            regAddrDbPtr->LMS.LMS0LMS0Group0.LEDInterface0Class5ManipulationRegforPorts0Through11 = 0x0480410c;
            regAddrDbPtr->LMS.LMS0LMS0Group0.stackLEDInterface0Class1011ManipulationRegforStackPorts = 0x04805100;
            regAddrDbPtr->LMS.LMS0LMS0Group0.stackLEDInterface0Group01ConfigRegforStackPort = 0x04805104;

            regAddrDbPtr->LMS.LMS0LMS0Group0.portsMIBCntrsInterruptSummaryMask = 0x04805110;
            regAddrDbPtr->LMS.LMS0LMS0Group0.portsInterruptSummaryMask = 0x04805114;

            regAddrDbPtr->LMS.LMS0LMS0Group0.MIBCntrsCtrlReg0forPorts6Through11 = 0x04804020;

            regAddrDbPtr->LMS.LMS0LMS0Group0.triSpeedPortsGOP1MIBsInterruptCause = 0x04804000;
            regAddrDbPtr->LMS.LMS0LMS0Group0.triSpeedPortsGOP1MIBsInterruptMask = 0x04804004;

            regAddrDbPtr->LMS.LMS0LMS0Group0.flowCtrlAdvertiseForFiberMediaSelectedConfigReg0forPorts0Through11 = 0x04804024;
            regAddrDbPtr->LMS.LMS0LMS0Group0.PHYAddrReg1forPorts6Through11 = 0x04804030;
            regAddrDbPtr->LMS.LMS0LMS0Group0.PHYAutoNegConfigReg1 = 0x04804034;

        }/*end of unit LMS0LMS0Group0 */


        {/*start of unit LMS0 */
            regAddrDbPtr->LMS.LMS0.LEDInterface0CtrlReg0forPorts0Through11AndStackPort = 0x04004100;
            regAddrDbPtr->LMS.LMS0.LEDInterface0Group01ConfigRegforPorts0Through11 = 0x04004104;
            regAddrDbPtr->LMS.LMS0.LEDInterface0Class01ManipulationRegforPorts0Through11 = 0x04004108;
            regAddrDbPtr->LMS.LMS0.LEDInterface0Class4ManipulationRegforPorts0Through11 = 0x0400410c;
            regAddrDbPtr->LMS.LMS0.stackLEDInterface0Class04ManipulationRegforStackPorts = 0x04005100;
            regAddrDbPtr->LMS.LMS0.stackLEDInterface0Class59ManipulationRegforStackPorts = 0x04005104;
            regAddrDbPtr->LMS.LMS0.LEDInterface0FlexlinkPortsDebugSelectRegforStackPort = 0x04005110;
            regAddrDbPtr->LMS.LMS0.LEDInterface0FlexlinkPortsDebugSelectReg1forStackPort = 0x04005114;

            regAddrDbPtr->LMS.LMS0.MIBCntrsInterruptSummary = 0x04004010;

            regAddrDbPtr->LMS.LMS0.MIBCntrsCtrlReg0forPorts0Through5 = 0x04004020;

            regAddrDbPtr->LMS.LMS0.sourceAddrMiddle = 0x04004024;
            regAddrDbPtr->LMS.LMS0.sourceAddrHigh = 0x04004028;

            regAddrDbPtr->LMS.LMS0.portsGOP0MIBsInterruptCause = 0x04004000;
            regAddrDbPtr->LMS.LMS0.portsGOP0MIBsInterruptMask = 0x04004004;

            regAddrDbPtr->LMS.LMS0.PHYAddrReg0forPorts0Through5 = 0x04004030;
            regAddrDbPtr->LMS.LMS0.PHYAutoNegConfigReg0 = 0x04004034;
            regAddrDbPtr->LMS.LMS0.SMI0Management = 0x04004054;
            regAddrDbPtr->LMS.LMS0.LMS0MiscConfigs = 0x04004200;

        }/*end of unit LMS0 */


        regAddrDbPtr->LMS.stackPortsModeReg = 0x04804144;
    }/*end of unit LMS - LMS  */

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

    {/*start of unit EPCL - EPCL  */
        regAddrDbPtr->EPCL.egrPolicyGlobalConfig = 0x0f800000;
        {
            GT_U32    w;
            for(w = 0 ; w <= 1 ; w++) {
                regAddrDbPtr->EPCL.egrPolicyConfigTableAccessModeConfig[w] =
                    0xf800010+4*w;
            }/* end of loop w */
        }/*0xf800010+4*w*/


        {/*0xf800020+w*4*/
            GT_U32    w;
            for(w = 0 ; w <= 1 ; w++) {
                regAddrDbPtr->EPCL.egrPolicyNonTSDataPktsConfig[w] =
                    0xf800020+w*4;
            }/* end of loop w */
        }/*0xf800020+w*4*/


        {/*0xf800030+4*w*/
            GT_U32    w;
            for(w = 0 ; w <= 1 ; w++) {
                regAddrDbPtr->EPCL.egrPolicyTSDataPktsConfig[w] =
                    0xf800030+4*w;
            }/* end of loop w */
        }/*0xf800030+4*w*/


        {/*0xf800040+4*w*/
            GT_U32    w;
            for(w = 0 ; w <= 1 ; w++) {
                regAddrDbPtr->EPCL.egrPolicyToCpuConfig[w] =
                    0xf800040+4*w;
            }/* end of loop w */
        }/*0xf800040+4*w*/


        {/*0xf800050+4*w*/
            GT_U32    w;
            for(w = 0 ; w <= 1 ; w++) {
                regAddrDbPtr->EPCL.egrPolicyFromCpuDataConfig[w] =
                    0xf800050+4*w;
            }/* end of loop w */
        }/*0xf800050+4*w*/


        {/*0xf800060+w*4*/
            GT_U32    w;
            for(w = 0 ; w <= 1 ; w++) {
                regAddrDbPtr->EPCL.egrPolicyFromCpuCtrlConfig[w] =
                    0xf800060+w*4;
            }/* end of loop w */
        }/*0xf800060+w*4*/


        {/*0xf800070+4*w*/
            GT_U32    w;
            for(w = 0 ; w <= 1 ; w++) {
                regAddrDbPtr->EPCL.egrPolicyToAnalyzerConfig[w] =
                    0xf800070+4*w;
            }/* end of loop w */
        }/*0xf800070+4*w*/


        regAddrDbPtr->EPCL.egrPolicyDIPSolicitationData[0] = 0x0f800080;
        regAddrDbPtr->EPCL.egrPolicyDIPSolicitationData[1] = 0x0f800084;
        regAddrDbPtr->EPCL.egrPolicyDIPSolicitationData[2] = 0x0f800088;
        regAddrDbPtr->EPCL.egrPolicyDIPSolicitationData[3] = 0x0f80008c;
        {/*0xf800090+n*4*/
            GT_U32    n;
            for(n = 0 ; n <= 3 ; n++) {
                regAddrDbPtr->EPCL.egrPolicyDIPSolicitationMask[n] =
                    0xf800090+n*4;
            }/* end of loop n */
        }/*0xf800090+n*4*/


        {/*0xf800100+n*0x4*/
            GT_U32    n;
            for(n = 0 ; n <= 7 ; n++) {
                regAddrDbPtr->EPCL.egrTCPPortRangeComparatorConfig[n] =
                    0xf800100+n*0x4;
            }/* end of loop n */
        }/*0xf800100+n*0x4*/


        {/*0xf800140+n*0x4*/
            GT_U32    n;
            for(n = 0 ; n <= 7 ; n++) {
                regAddrDbPtr->EPCL.egrUDPPortRangeComparatorConfig[n] =
                    0xf800140+n*0x4;
            }/* end of loop n */
        }/*0xf800140+n*0x4*/


        regAddrDbPtr->EPCL.egrPolicerGlobalConfig = 0x0f800200;
        {
            GT_U32    w;
            for(w = 0 ; w <= 1 ; w++) {
                regAddrDbPtr->EPCL.egrPolicerPortBasedMetering[w] =
                    0xf800230+4*w;
            }/* end of loop w */
        }/*0xf800230+4*w*/



    }/*end of unit EPCL - EPCL  */

    {/*start of unit HA - HA  */
        {/*start of unit ETS */
            regAddrDbPtr->HA.ETS.globalConfigs = 0x0f000800;
            regAddrDbPtr->HA.ETS.enableTimestamping = 0x0f000804;
            {/*0xf000808+4*i*/
                GT_U32    i;
                for(i = 0 ; i <= 1 ; i++) {
                    regAddrDbPtr->HA.ETS.timestampingPortEnable[i] =
                        0xf000808+4*i;
                }/* end of loop i */
            }/*0xf000808+4*i*/


            regAddrDbPtr->HA.ETS.TODCntrNanoseconds = 0x0f000810;
            {/*0xf000814+4*i*/
                GT_U32    i;
                for(i = 0 ; i <= 1 ; i++) {
                    regAddrDbPtr->HA.ETS.TODCntrSeconds[i] =
                        0xf000814+4*i;
                }/* end of loop i */
            }/*0xf000814+4*i*/


            regAddrDbPtr->HA.ETS.TODCntrShadowNanoseconds = 0x0f00081c;
            {/*0xf000820+4*i*/
                GT_U32    i;
                for(i = 0 ; i <= 1 ; i++) {
                    regAddrDbPtr->HA.ETS.TODCntrShadowSeconds[i] =
                        0xf000820+4*i;
                }/* end of loop i */
            }/*0xf000820+4*i*/


            regAddrDbPtr->HA.ETS.GTSInterruptCause = 0x0f000828;
            regAddrDbPtr->HA.ETS.GTSInterruptMask = 0x0f00082c;
            regAddrDbPtr->HA.ETS.globalFIFOCurrentEntry[0] = 0x0f000830;
            regAddrDbPtr->HA.ETS.globalFIFOCurrentEntry[1] = 0x0f000834;
            regAddrDbPtr->HA.ETS.overrunStatus = 0x0f000838;
            regAddrDbPtr->HA.ETS.underrunStatus = 0x0f00083c;

        }/*end of unit ETS */


        {/*0xf000004+w*4*/
            GT_U32    w;
            for(w = 0 ; w <= 1 ; w++) {
                regAddrDbPtr->HA.cascadingAndHeaderInsertionConfig[w] =
                    0xf000004+w*4;
            }/* end of loop w */
        }/*0xf000004+w*4*/


        {/*0xf000020+w*4*/
            GT_U32    w;
            for(w = 0 ; w <= 1 ; w++) {
                regAddrDbPtr->HA.egrDSATagTypeConfig[w] =
                    0xf000020+w*4;
            }/* end of loop w */
        }/*0xf000020+w*4*/


        {/*0xf00002c+n*0x4*/
            GT_U32    n;
            for(n = 0 ; n <= 15 ; n++) {
                regAddrDbPtr->HA.portTrunkNumConfigReg[n] =
                    0xf00002c+n*0x4;
            }/* end of loop n */
        }/*0xf00002c+n*0x4*/


        {/*0xf000080+4*w*/
            GT_U32    w;
            for(w = 0 ; w <= 1 ; w++) {
                regAddrDbPtr->HA.mirrorToAnalyzerHeaderConfig[w] =
                    0xf000080+4*w;
            }/* end of loop w */
        }/*0xf000080+4*w*/


        regAddrDbPtr->HA.haGlobalConfig = 0x0f000100;
        {
            GT_U32    w;
            for(w = 0 ; w <= 1 ; w++) {
                regAddrDbPtr->HA.routerHaEnableMACSAModification[w] =
                    0xf000104+4*w;
            }/* end of loop w */
        }/*0xf000104+4*w*/


        regAddrDbPtr->HA.routerMACSABaseReg0 = 0x0f00010c;
        {/*0xf000114+4*w*/
            GT_U32    w;
            for(w = 0 ; w <= 1 ; w++) {
                regAddrDbPtr->HA.deviceIDModificationEnable[w] =
                    0xf000114+4*w;
            }/* end of loop w */
        }/*0xf000114+4*w*/


        {/*0xf000120+k*0x4*/
            GT_U32    k;
            for(k = 0 ; k <= 3 ; k++) {
                regAddrDbPtr->HA.routerHaMACSAModificationMode[k] =
                    0xf000120+k*0x4;
            }/* end of loop k */
        }/*0xf000120+k*0x4*/


        {/*0xf000130+4*w*/
            GT_U32    w;
            for(w = 0 ; w <= 1 ; w++) {
                regAddrDbPtr->HA.haVLANTranslationEnable[w] =
                    0xf000130+4*w;
            }/* end of loop w */
        }/*0xf000130+4*w*/


        {/*0xf000140+4*w*/
            GT_U32    w;
            for(w = 0 ; w <= 1 ; w++) {
                regAddrDbPtr->HA.coreRingConfig[w] =
                    0xf000140+4*w;
            }/* end of loop w */
        }/*0xf000140+4*w*/


        {/*0xf000300+4*n*/
            GT_U32    n;
            for(n = 0 ; n <= 15 ; n++) {
                regAddrDbPtr->HA.qoSProfileToEXPReg[n] =
                    0xf000300+4*n;
            }/* end of loop n */
        }/*0xf000300+4*n*/


        {/*0xf000340+4*n*/
            GT_U32    n;
            for(n = 0 ; n <= 7 ; n++) {
                regAddrDbPtr->HA.qoSProfileToDPReg[n] =
                    0xf000340+4*n;
            }/* end of loop n */
        }/*0xf000340+4*n*/


        regAddrDbPtr->HA.HAInterruptCause = 0x0f0003a0;
        regAddrDbPtr->HA.hepInterruptSummaryCause = 0x0f0003a8;
        regAddrDbPtr->HA.hepInterruptSummaryMask = 0x0f0003ac;
        {/*0xf000400+4*w*/
            GT_U32    w;
            for(w = 0 ; w <= 1 ; w++) {
                regAddrDbPtr->HA.toAnalyzerVLANTagAddEnableConfig[w] =
                    0xf000400+4*w;
            }/* end of loop w */
        }/*0xf000400+4*w*/


        regAddrDbPtr->HA.ingrAnalyzerVLANTagConfig = 0x0f000408;
        regAddrDbPtr->HA.egrAnalyzerVLANTagConfig = 0x0f00040c;
        regAddrDbPtr->HA.congestionNotificationConfig = 0x0f000420;
        regAddrDbPtr->HA.QCNCNtagFlowID = 0x0f000428;
        regAddrDbPtr->HA.CPIDReg0 = 0x0f000430;
        regAddrDbPtr->HA.CPIDReg1 = 0x0f000434;
        {
            GT_U32    p;
            for(p = 0 ; p <= 15 ; p++) {
                regAddrDbPtr->HA.keepVLAN1[p] =
                    0xf000440+4*p;
            }/* end of loop p */
        }/*0xf000440+4*p*/


        {/*0xf000500+n*4*/
            GT_U32    n;
            for(n = 0 ; n <= 3 ; n++) {
                regAddrDbPtr->HA.TPIDConfigReg[n] =
                    0xf000500+n*4;
            }/* end of loop n */
        }/*0xf000500+n*4*/


        {/*0xf000510+n*4*/
            GT_U32    n;
            for(n = 0 ; n <= 7 ; n++) {
                regAddrDbPtr->HA.portTag0TPIDSelect[n] =
                    0xf000510+n*4;
            }/* end of loop n */
        }/*0xf000510+n*4*/


        {/*0xf000530+n*4*/
            GT_U32    n;
            for(n = 0 ; n <= 7 ; n++) {
                regAddrDbPtr->HA.portTag1TPIDSelect[n] =
                    0xf000530+n*4;
            }/* end of loop n */
        }/*0xf000530+n*4*/


        regAddrDbPtr->HA.MPLSEthertype = 0x0f000550;
        regAddrDbPtr->HA.IEthertype = 0x0f000554;
        regAddrDbPtr->HA.ethernetOverGREProtocolType = 0x0f000558;

        regAddrDbPtr->HA.tunnelStartFragmentIDConfig = 0x0f000410;

    }/*end of unit HA - HA  */

    {/*start of unit MEM - MEM  */
        {/*start of unit arbitersConfigRegs */
            regAddrDbPtr->MEM.arbitersConfigRegs.arbitersConfigReg0 = 0x078000a0;

        }/*end of unit arbitersConfigRegs */


        {/*start of unit buffersMemoryAndMACErrorsIndicationsInterrupts */
            regAddrDbPtr->MEM.buffersMemoryAndMACErrorsIndicationsInterrupts.bufferMemoryInterruptCauseReg0 = 0x07800024;
            regAddrDbPtr->MEM.buffersMemoryAndMACErrorsIndicationsInterrupts.bufferMemoryInterruptMaskReg0 = 0x07800028;
            regAddrDbPtr->MEM.buffersMemoryAndMACErrorsIndicationsInterrupts.bufferMemoryMainInterruptCauseReg = 0x07800090;
            regAddrDbPtr->MEM.buffersMemoryAndMACErrorsIndicationsInterrupts.bufferMemoryMainInterruptMaskReg = 0x07800094;
            regAddrDbPtr->MEM.buffersMemoryAndMACErrorsIndicationsInterrupts.bufferMemoryInterruptCauseReg1 = 0x07800098;
            regAddrDbPtr->MEM.buffersMemoryAndMACErrorsIndicationsInterrupts.bufferMemoryInterruptMaskReg1 = 0x0780009c;

        }/*end of unit buffersMemoryAndMACErrorsIndicationsInterrupts */



    }/*end of unit MEM - MEM  */

    {/*start of unit centralizedCntrs - Centralized Counters  */
        {/*start of unit DFX */
            regAddrDbPtr->centralizedCntrs.DFX.dataErrorInterruptCause = 0x08000110;
            regAddrDbPtr->centralizedCntrs.DFX.dataErrorInterruptMask = 0x08000114;
            {/*0x8000118+n*4*/
                GT_U32    n;
                for(n = 0 ; n <= 7 ; n++) {
                    regAddrDbPtr->centralizedCntrs.DFX.dataErrorStatusReg[n] =
                        0x8000118+n*4;
                }/* end of loop n */
            }/*0x8000118+n*4*/



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
            {/*0x80010a0+n*0x100+m*0x4*/
                GT_U32    n,m;
                for(n = 0 ; n <= 7 ; n++) {
                    for(m = 0 ; m <= 3 ; m++) {
                        regAddrDbPtr->centralizedCntrs.perBlockRegs.CNCBlockWraparoundStatusReg[n][m] =
                            0x80010a0+n*0x100+m*0x4;
                    }/* end of loop m */
                }/* end of loop n */
            }/*0x80010a0+n*0x100+m*0x4*/


            {/*0x8001180+n*0x100*/
                GT_U32    n;
                for(n = 0 ; n <= 7 ; n++) {
                    regAddrDbPtr->centralizedCntrs.perBlockRegs.CNCBlockConfigReg[0][n] =
                        0x8001180+n*0x100;
                }/* end of loop n */
            }/*0x8001180+n*0x100*/


            {/*0x8001184+n*0x100*/
                GT_U32    n;
                for(n = 0 ; n <= 7 ; n++) {
                    regAddrDbPtr->centralizedCntrs.perBlockRegs.CNCBlockConfigReg[1][n] =
                        0x8001184+n*0x100;
                }/* end of loop n */
            }/*0x8001184+n*0x100*/



        }/*end of unit perBlockRegs */



    }/*end of unit centralizedCntrs - Centralized Counters  */

    {/*start of unit uniphySERDES - Uniphy SERDES  */
        {/*start of unit LPSERDESInternalRegsSERDES1 */
            regAddrDbPtr->uniphySERDES.LPSERDESInternalRegsSERDES1.PHYRevision = 0x09800200;
            regAddrDbPtr->uniphySERDES.LPSERDESInternalRegsSERDES1.PLLINTPReg0 = 0x09800208;
            regAddrDbPtr->uniphySERDES.LPSERDESInternalRegsSERDES1.PLLINTPReg1 = 0x0980020c;
            regAddrDbPtr->uniphySERDES.LPSERDESInternalRegsSERDES1.PLLINTPReg5 = 0x0980021c;
            regAddrDbPtr->uniphySERDES.LPSERDESInternalRegsSERDES1.calibrationReg0 = 0x09800224;
            regAddrDbPtr->uniphySERDES.LPSERDESInternalRegsSERDES1.calibrationReg7 = 0x09800228;
            regAddrDbPtr->uniphySERDES.LPSERDESInternalRegsSERDES1.calibrationReg1 = 0x0980022c;
            regAddrDbPtr->uniphySERDES.LPSERDESInternalRegsSERDES1.calibrationReg2 = 0x09800230;
            regAddrDbPtr->uniphySERDES.LPSERDESInternalRegsSERDES1.calibrationReg3 = 0x09800234;
            regAddrDbPtr->uniphySERDES.LPSERDESInternalRegsSERDES1.calibrationReg4 = 0x09800238;
            regAddrDbPtr->uniphySERDES.LPSERDESInternalRegsSERDES1.calibrationReg5 = 0x0980023c;
            regAddrDbPtr->uniphySERDES.LPSERDESInternalRegsSERDES1.transmitterReg0 = 0x09800250;
            regAddrDbPtr->uniphySERDES.LPSERDESInternalRegsSERDES1.transmitterReg1 = 0x09800254;
            regAddrDbPtr->uniphySERDES.LPSERDESInternalRegsSERDES1.transmitterReg2 = 0x09800258;
            regAddrDbPtr->uniphySERDES.LPSERDESInternalRegsSERDES1.transmitterReg4 = 0x09800260;
            regAddrDbPtr->uniphySERDES.LPSERDESInternalRegsSERDES1.receiverReg[0] = 0x09800278;
            regAddrDbPtr->uniphySERDES.LPSERDESInternalRegsSERDES1.receiverReg[1] = 0x0980027c;
            regAddrDbPtr->uniphySERDES.LPSERDESInternalRegsSERDES1.receiverReg[2] = 0x09800280;
            regAddrDbPtr->uniphySERDES.LPSERDESInternalRegsSERDES1.FFEReg0 = 0x0980028c;
            regAddrDbPtr->uniphySERDES.LPSERDESInternalRegsSERDES1.DFEF0F1CoefficientCtrl = 0x09800294;
            regAddrDbPtr->uniphySERDES.LPSERDESInternalRegsSERDES1.readyflagReg = 0x098002ac;
            regAddrDbPtr->uniphySERDES.LPSERDESInternalRegsSERDES1.G1Setting0 = 0x09800338;
            regAddrDbPtr->uniphySERDES.LPSERDESInternalRegsSERDES1.G1Setting1 = 0x0980033c;
            regAddrDbPtr->uniphySERDES.LPSERDESInternalRegsSERDES1.G2Setting0 = 0x09800340;
            regAddrDbPtr->uniphySERDES.LPSERDESInternalRegsSERDES1.G2Setting1 = 0x09800344;
            regAddrDbPtr->uniphySERDES.LPSERDESInternalRegsSERDES1.G3Setting0 = 0x09800348;
            regAddrDbPtr->uniphySERDES.LPSERDESInternalRegsSERDES1.G3Setting1 = 0x0980034c;
            regAddrDbPtr->uniphySERDES.LPSERDESInternalRegsSERDES1.configReg0 = 0x09800358;
            regAddrDbPtr->uniphySERDES.LPSERDESInternalRegsSERDES1.referenceReg0 = 0x09800360;
            regAddrDbPtr->uniphySERDES.LPSERDESInternalRegsSERDES1.powerReg0 = 0x09800368;
            regAddrDbPtr->uniphySERDES.LPSERDESInternalRegsSERDES1.resetReg0 = 0x0980036c;
            regAddrDbPtr->uniphySERDES.LPSERDESInternalRegsSERDES1.PHYTestReg0 = 0x09800378;
            regAddrDbPtr->uniphySERDES.LPSERDESInternalRegsSERDES1.PHYTestReg1 = 0x0980037c;
            regAddrDbPtr->uniphySERDES.LPSERDESInternalRegsSERDES1.PHYTestDataReg[0] = 0x09800380;
            regAddrDbPtr->uniphySERDES.LPSERDESInternalRegsSERDES1.PHYTestDataReg[1] = 0x09800384;
            regAddrDbPtr->uniphySERDES.LPSERDESInternalRegsSERDES1.PHYTestDataReg[2] = 0x09800388;
            regAddrDbPtr->uniphySERDES.LPSERDESInternalRegsSERDES1.PHYTestDataReg[3] = 0x0980038c;
            regAddrDbPtr->uniphySERDES.LPSERDESInternalRegsSERDES1.PHYTestDataReg[4] = 0x09800390;
            regAddrDbPtr->uniphySERDES.LPSERDESInternalRegsSERDES1.PHYTestDataReg[5] = 0x09800394;
            regAddrDbPtr->uniphySERDES.LPSERDESInternalRegsSERDES1.PHYTestPRBSCntrReg[0] = 0x09800398;
            regAddrDbPtr->uniphySERDES.LPSERDESInternalRegsSERDES1.PHYTestPRBSCntrReg[1] = 0x0980039c;
            regAddrDbPtr->uniphySERDES.LPSERDESInternalRegsSERDES1.PHYTestPRBSCntrReg[2] = 0x098003a0;
            regAddrDbPtr->uniphySERDES.LPSERDESInternalRegsSERDES1.PHYTestPRBSErrorCntrReg0 = 0x098003a4;
            regAddrDbPtr->uniphySERDES.LPSERDESInternalRegsSERDES1.PHYTestPRBSErrorCntrReg1 = 0x098003a8;
            regAddrDbPtr->uniphySERDES.LPSERDESInternalRegsSERDES1.PHYTestOOBReg0 = 0x098003ac;
            regAddrDbPtr->uniphySERDES.LPSERDESInternalRegsSERDES1.PHYTestOOBReg1 = 0x098003b0;
            regAddrDbPtr->uniphySERDES.LPSERDESInternalRegsSERDES1.testReg0 = 0x098003b8;
            regAddrDbPtr->uniphySERDES.LPSERDESInternalRegsSERDES1.digitalInterfaceReg0 = 0x098003e0;

        }/*end of unit LPSERDESInternalRegsSERDES1 */


        {/*start of unit networkPorts815SERDES */
            regAddrDbPtr->GOP.SERDESes.SERDES[0].SERDESExternalConfig[0] = 0x09800000;
            regAddrDbPtr->GOP.SERDESes.SERDES[0].SERDESExternalConfig[1] = 0x09800004;
            regAddrDbPtr->GOP.SERDESes.SERDES[0].SERDESExternalStatus = 0x09800008;
            regAddrDbPtr->GOP.SERDESes.SERDES[0].SERDESExternalConfig[2] = 0x0980000c;
            regAddrDbPtr->GOP.SERDESes.SERDES[0].SERDESExternalConfig[3] = 0x09800010;
            regAddrDbPtr->GOP.SERDESes.SERDES[0].SERDESExternalConfig[4] = 0x09800014;
            regAddrDbPtr->GOP.SERDESes.SERDES[0].SERDESMetalFix = 0x09800020;

        }/*end of unit networkPorts815SERDES */



    }/*end of unit uniphySERDES - Uniphy SERDES  */

    {/*start of unit networkPorts - Network Ports  */
        {/*start of unit QSGMIICtrlAndStatus */
            {/*0xa80004c+4*n*0x400*/
                GT_U32    n;
                for(n = 0 ; n <= 5 ; n++) {
                    regAddrDbPtr->networkPorts.QSGMIICtrlAndStatus.QSGMIICtrlReg0[n] =
                        0xa80004c+4*n*0x400;
                }/* end of loop n */
            }/*0xa80004c+4*n*0x400*/


            {/*0xa800050+4*n*0x400*/
                GT_U32    n;
                for(n = 0 ; n <= 5 ; n++) {
                    regAddrDbPtr->networkPorts.QSGMIICtrlAndStatus.QSGMIIStatusReg[n] =
                        0xa800050+4*n*0x400;
                }/* end of loop n */
            }/*0xa800050+4*n*0x400*/


            {/*0xa800054+4*n*0x400*/
                GT_U32    n;
                for(n = 0 ; n <= 5 ; n++) {
                    regAddrDbPtr->networkPorts.QSGMIICtrlAndStatus.QSGMIIPRBSErrorCntr[n] =
                        0xa800054+4*n*0x400;
                }/* end of loop n */
            }/*0xa800054+4*n*0x400*/


            {/*0xa80044c+4*n*0x400*/
                GT_U32    n;
                for(n = 0 ; n <= 5 ; n++) {
                    regAddrDbPtr->networkPorts.QSGMIICtrlAndStatus.QSGMIICtrlReg1[n] =
                        0xa80044c+4*n*0x400;
                }/* end of loop n */
            }/*0xa80044c+4*n*0x400*/


            {/*0xa80084c+4*n*0x400*/
                GT_U32    n;
                for(n = 0 ; n <= 5 ; n++) {
                    regAddrDbPtr->networkPorts.QSGMIICtrlAndStatus.QSGMIICtrlReg2[n] =
                        0xa80084c+4*n*0x400;
                }/* end of loop n */
            }/*0xa80084c+4*n*0x400*/



        }/*end of unit QSGMIICtrlAndStatus */


        {/*start of unit gigPort63CPU */
            regAddrDbPtr->GOP.ports.gigPort63CPU.portAutoNegConfig = 0x0a80fc0c;

            regAddrDbPtr->GOP.ports.gigPort63CPU.portInterruptCause = 0x0a80fc20;
            regAddrDbPtr->GOP.ports.gigPort63CPU.portInterruptMask = 0x0a80fc24;

            regAddrDbPtr->GOP.ports.gigPort63CPU.portSerialParametersConfig = 0x0a80fc14;

            regAddrDbPtr->GOP.ports.gigPort63CPU.portStatusReg0 = 0x0a80fc10;
            regAddrDbPtr->GOP.ports.gigPort63CPU.portStatusReg1 = 0x0a80fc40;

            regAddrDbPtr->GOP.ports.gigPort63CPU.portPRBSStatusReg = 0x0a80fc38;
            regAddrDbPtr->GOP.ports.gigPort63CPU.portPRBSErrorCntr = 0x0a80fc3c;

            regAddrDbPtr->GOP.ports.gigPort63CPU.portSERDESConfigReg[0] = 0x0a80fc28;
            regAddrDbPtr->GOP.ports.gigPort63CPU.portSERDESConfigReg[1] = 0x0a80fc2c;
            regAddrDbPtr->GOP.ports.gigPort63CPU.portSERDESConfigReg[2] = 0x0a80fc30;
            regAddrDbPtr->GOP.ports.gigPort63CPU.portSERDESConfigReg[3] = 0x0a80fc34;

            regAddrDbPtr->GOP.ports.gigPort63CPU.portMACCtrlReg[0] = 0x0a80fc00;
            regAddrDbPtr->GOP.ports.gigPort63CPU.portMACCtrlReg[1] = 0x0a80fc04;
            regAddrDbPtr->GOP.ports.gigPort63CPU.portMACCtrlReg[2] = 0x0a80fc08;
            regAddrDbPtr->GOP.ports.gigPort63CPU.portMACCtrlReg[3] = 0x0a80fc48;

        }/*end of unit gigPort63CPU */


        {/*start of unit gigPort */
            GT_U32 n;
            for(n = 0; n < 28; n++)
            {
                regAddrDbPtr->GOP.ports.gigPort[n].LPICtrl0 = 0x0a8000c0 + n * 0x400;
                regAddrDbPtr->GOP.ports.gigPort[n].LPICtrl1 = 0x0a8000c4 + n * 0x400;
                regAddrDbPtr->GOP.ports.gigPort[n].LPIStatus = 0x0a8000cc + n * 0x400;
                regAddrDbPtr->GOP.ports.gigPort[n].LPICntr = 0x0a8000d0 + n * 0x400;

                regAddrDbPtr->GOP.ports.gigPort[n].portAutoNegConfig = 0x0a80000c + n * 0x400;

                regAddrDbPtr->GOP.ports.gigPort[n].portInterruptCause = 0x0a800020 + n * 0x400;
                regAddrDbPtr->GOP.ports.gigPort[n].portInterruptMask = 0x0a800024 + n * 0x400;

                regAddrDbPtr->GOP.ports.gigPort[n].portSerialParametersConfig = 0x0a800014 + n * 0x400;
                regAddrDbPtr->GOP.ports.gigPort[n].portSerialParameters1Config = 0x0a800094 + n * 0x400;

                regAddrDbPtr->GOP.ports.gigPort[n].portStatusReg0 = 0x0a800010 + n * 0x400;
                regAddrDbPtr->GOP.ports.gigPort[n].portStatusReg1 = 0x0a800040 + n * 0x400;

                regAddrDbPtr->GOP.ports.gigPort[n].portPRBSStatusReg = 0x0a800038 + n * 0x400;
                regAddrDbPtr->GOP.ports.gigPort[n].portPRBSErrorCntr = 0x0a80003c + n * 0x400;

                regAddrDbPtr->GOP.ports.gigPort[n].portMACCtrlReg[0] = 0x0a800000 + n * 0x400;
                regAddrDbPtr->GOP.ports.gigPort[n].portMACCtrlReg[1] = 0x0a800004 + n * 0x400;
                regAddrDbPtr->GOP.ports.gigPort[n].portMACCtrlReg[2] = 0x0a800008 + n * 0x400;
                regAddrDbPtr->GOP.ports.gigPort[n].config100FXRegports2427Only = 0x0a80002c + n * 0x400;
                regAddrDbPtr->GOP.ports.gigPort[n].stackPortMIBCntrsCtrl = 0x0a800044 + n * 0x400;
                regAddrDbPtr->GOP.ports.gigPort[n].portMACCtrlReg[3] = 0x0a800048 + n * 0x400;
                {/*0xa800058+p*4*/
                    GT_U32    p;
                    for(p = 0 ; p <= 7 ; p++) {
                        regAddrDbPtr->GOP.ports.gigPort[n].CCFCPortSpeedTimer[p] =
                            0xa800058+ + n * 0x400+ p*4;
                    }/* end of loop p */
                }/*0xa800058+p*4*/

                regAddrDbPtr->GOP.ports.gigPort[n].portMACCtrlReg[4] = 0x0a800090 + n * 0x400;
            }
        }/*end of unit gigPort */



    }/*end of unit networkPorts - Network Ports  */

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
                for(r = 0 ; r <= 1 ; r++) {
                    regAddrDbPtr->EQ.mirrToAnalyzerPortConfigs.egrMonitoringEnableConfig[r] =
                        0xb00000c+r*0x64;
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
                GT_U32    i;
                for(i = 0 ; i <= 6 ; i++) {
                    regAddrDbPtr->EQ.mirrToAnalyzerPortConfigs.mirrorInterfaceParameterReg[i] =
                        0xb00b020+i*4;
                }/* end of loop i */
            }/*0xb00b020+i*4*/


            regAddrDbPtr->EQ.mirrToAnalyzerPortConfigs.analyzerPortGlobalConfig = 0x0b00b040;
            regAddrDbPtr->EQ.mirrToAnalyzerPortConfigs.portEgrMirrorIndex[0][3] = 0x0b00b044;
            regAddrDbPtr->EQ.mirrToAnalyzerPortConfigs.portEgrMirrorIndex[0][4] = 0x0b00b048;
            regAddrDbPtr->EQ.mirrToAnalyzerPortConfigs.portEgrMirrorIndex[0][5] = 0x0b00b04c;
            regAddrDbPtr->EQ.mirrToAnalyzerPortConfigs.portEgrMirrorIndex[0][6] = 0x0b00b070;

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
                    regAddrDbPtr->EQ.preEgrEngineInternal.preEgrEngineOutGoingDescWord1[n] =
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
        {
            GT_U32    i;
            for(i = 0 ; i <= 1 ; i++) {
                regAddrDbPtr->IPCL.L2L3IngrVLANCountingEnable[i] =
                    0xb800080+4*i;
            }/* end of loop i */
        }/*0xb800080+4*i*/


        regAddrDbPtr->IPCL.countingModeConfig = 0x0b800088;
        {
            GT_U32    i;
            for(i = 0 ; i <= 15 ; i++) {
                regAddrDbPtr->IPCL.pearsonHashTable[i] =
                    0xb8005c0+4*i;
            }/* end of loop i */
        }/*0xb8005c0+4*i*/



    }/*end of unit IPCL - IPCL  */

    {/*start of unit IPLR0, IPLR1, EPLR  - added manually  */
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
        }
    }/*end of unit PLR[0],1, PLR[2]  - added manually */

    {/*start of unit TCCLowerIPCL - TCC Lower (IPCL)  */
        {/*start of unit TCCIPCL */
            {/*0xe000100+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 11 ; n++) {
                    regAddrDbPtr->TCCLowerIPCL.TCCIPCL.actionTableAndPolicyTCAMAccessData[n] =
                        0xe000100+n*0x4;
                }/* end of loop n */
            }/*0xe000100+n*0x4*/


            regAddrDbPtr->TCCLowerIPCL.TCCIPCL.actionTableAndPolicyTCAMAccessCtrlParameters1 = 0x0e000130;
            regAddrDbPtr->TCCLowerIPCL.TCCIPCL.actionTableAndPolicyTCAMAccessCtrlParameters2 = 0x0e000134;
            regAddrDbPtr->TCCLowerIPCL.TCCIPCL.actionTableAndPolicyTCAMAccessCtrl = 0x0e000138;

            {/*0xe000304+8*p*/
                GT_U32    p;
                for(p = 0 ; p <= 2 ; p++) {
                    regAddrDbPtr->TCCLowerIPCL.TCCIPCL.IPCLKeyType2LookupIDReg0[p] =
                        0xe000304+8*p;
                }/* end of loop p */
            }/*0xe000304+8*p*/


            {/*0xe000308+8*p*/
                GT_U32    p;
                for(p = 0 ; p <= 2 ; p++) {
                    regAddrDbPtr->TCCLowerIPCL.TCCIPCL.IPCLKeyType2LookupIDReg1[p] =
                        0xe000308+8*p;
                }/* end of loop p */
            }/*0xe000308+8*p*/


            regAddrDbPtr->TCCLowerIPCL.TCCIPCL.EPCLKeyType2LookupIDReg = 0x0e00031c;

       regAddrDbPtr->TCCLowerIPCL.TCCIPCL.TCCECCErrorInformation = 0x0e000170;
            regAddrDbPtr->TCCLowerIPCL.TCCIPCL.TCAMErrorCntr = 0x0e000198;
            regAddrDbPtr->TCCLowerIPCL.TCCIPCL.TCCInterruptCause = 0x0e0001a4;
            regAddrDbPtr->TCCLowerIPCL.TCCIPCL.TCCInterruptMask = 0x0e0001a8;
            regAddrDbPtr->TCCLowerIPCL.TCCIPCL.TCCUnitInterruptSummaryCause = 0x0e0001bc;
            regAddrDbPtr->TCCLowerIPCL.TCCIPCL.TCCUnitInterruptSummaryMask = 0x0e0001c0;

        }/*end of unit TCCIPCL */



    }/*end of unit TCCLowerIPCL - TCC Lower (IPCL)  */

    {/*start of unit TCCUpperIPvX - TCC Upper (IPvX)  */
        {/*start of unit TCCIPvX */
            regAddrDbPtr->TCCUpperIPvX.TCCIPvX.TTIKeyType2LookupID = 0x0e800080;
            regAddrDbPtr->TCCUpperIPvX.TCCIPvX.routerDIPLookupKeyType2LookupID = 0x0e8000a0;
            regAddrDbPtr->TCCUpperIPvX.TCCIPvX.routerSIPLookupKeyType2LookupID = 0x0e8000a4;

            {/*0xe800400+n*4*/
                GT_U32    n;
                for(n = 0 ; n <= 5 ; n++) {
                    regAddrDbPtr->TCCUpperIPvX.TCCIPvX.routerTCAMAccessDataReg[n] =
                        0xe800400+n*4;
                }/* end of loop n */
            }/*0xe800400+n*4*/


            regAddrDbPtr->TCCUpperIPvX.TCCIPvX.routerTCAMAccessCtrlReg0 = 0x0e800418;
            regAddrDbPtr->TCCUpperIPvX.TCCIPvX.routerTCAMAccessCtrlReg1 = 0x0e80041c;

            regAddrDbPtr->TCCUpperIPvX.TCCIPvX.TCAMErrorCntr = 0x0e800054;
            regAddrDbPtr->TCCUpperIPvX.TCCIPvX.TCCInterruptCause = 0x0e800060;
            regAddrDbPtr->TCCUpperIPvX.TCCIPvX.TCCInterruptMask = 0x0e800064;
            regAddrDbPtr->TCCUpperIPvX.TCCIPvX.TCCUnitInterruptSummaryMask = 0x0e80007c;
            regAddrDbPtr->TCCUpperIPvX.TCCIPvX.TCCECCErrorInformation = 0x0e800d78;

        }/*end of unit TCCIPvX */



    }/*end of unit TCCUpperIPvX - TCC Upper (IPvX)  */

    {/*start of unit TTI - TTI  */
        {/*start of unit ITS */
            regAddrDbPtr->TTI.ITS.globalConfigs = 0x0c002000;
            regAddrDbPtr->TTI.ITS.enableTimestamping = 0x0c002004;
            {/*0xc002008+4*i*/
                GT_U32    i;
                for(i = 0 ; i <= 1 ; i++) {
                    regAddrDbPtr->TTI.ITS.timestampingPortEnable[i] =
                        0xc002008+4*i;
                }/* end of loop i */
            }/*0xc002008+4*i*/


            regAddrDbPtr->TTI.ITS.TODCntrNanoseconds = 0x0c002010;
            {/*0xc002014+4*i*/
                GT_U32    i;
                for(i = 0 ; i <= 1 ; i++) {
                    regAddrDbPtr->TTI.ITS.TODCntrSeconds[i] =
                        0xc002014+4*i;
                }/* end of loop i */
            }/*0xc002014+4*i*/


            regAddrDbPtr->TTI.ITS.TODCntrShadowNanoseconds = 0x0c00201c;
            {/*0xc002020+4*i*/
                GT_U32    i;
                for(i = 0 ; i <= 1 ; i++) {
                    regAddrDbPtr->TTI.ITS.TODCntrShadowSeconds[i] =
                        0xc002020+4*i;
                }/* end of loop i */
            }/*0xc002020+4*i*/


            regAddrDbPtr->TTI.ITS.GTSInterruptCause = 0x0c002028;
            regAddrDbPtr->TTI.ITS.GTSInterruptMask = 0x0c00202c;
            regAddrDbPtr->TTI.ITS.globalFIFOCurrentEntry[0] = 0x0c002030;
            regAddrDbPtr->TTI.ITS.globalFIFOCurrentEntry[1] = 0x0c002034;
            regAddrDbPtr->TTI.ITS.overrunStatus = 0x0c002038;
            regAddrDbPtr->TTI.ITS.underrunStatus = 0x0c00203c;

        }/*end of unit ITS */


        {/*start of unit logFlow */
            regAddrDbPtr->TTI.logFlow.bridgeLogFlowMACDALow = 0x0c000100;
            regAddrDbPtr->TTI.logFlow.bridgeLogFlowMACDAHigh = 0x0c000104;
            regAddrDbPtr->TTI.logFlow.bridgeLogFlowMACDALowMask = 0x0c000108;
            regAddrDbPtr->TTI.logFlow.bridgeLogFlowMACDAHighMask = 0x0c00010c;
            regAddrDbPtr->TTI.logFlow.bridgeLogFlowMACSALow = 0x0c000110;
            regAddrDbPtr->TTI.logFlow.bridgeLogFlowMACSAHigh = 0x0c000114;
            regAddrDbPtr->TTI.logFlow.bridgeLogFlowMACSALowMask = 0x0c000118;
            regAddrDbPtr->TTI.logFlow.bridgeLogFlowMACSAHighMask = 0x0c00011c;
            regAddrDbPtr->TTI.logFlow.bridgeLogFlowEtherType = 0x0c000120;
            {/*0xc000140+n*4*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->TTI.logFlow.routerLogFlowDIP[n] =
                        0xc000140+n*4;
                }/* end of loop n */
            }/*0xc000140+n*4*/


            {/*0xc000150+n*4*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->TTI.logFlow.routerLogFlowDIPMask[n] =
                        0xc000150+n*4;
                }/* end of loop n */
            }/*0xc000150+n*4*/


            {/*0xc000160+n*4*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->TTI.logFlow.routerLogFlowSIP[n] =
                        0xc000160+n*4;
                }/* end of loop n */
            }/*0xc000160+n*4*/


            {/*0xc000170+n*4*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->TTI.logFlow.routerLogFlowSIPMask[n] =
                        0xc000170+n*4;
                }/* end of loop n */
            }/*0xc000170+n*4*/


            regAddrDbPtr->TTI.logFlow.routerLogFlowIPProtocol = 0x0c000180;
            regAddrDbPtr->TTI.logFlow.routerLogFlowSourceTCPUDPPort = 0x0c000184;
            regAddrDbPtr->TTI.logFlow.routerLogFlowDestinationTCPUDPPort = 0x0c000188;

        }/*end of unit logFlow */


        {/*start of unit MAC2ME */
            {/*0xc001600+n*0x10*/
                GT_U32    n;
                for(n = 0 ; n <= 7 ; n++) {
                    regAddrDbPtr->TTI.MAC2ME.MAC2MELow[n] =
                        0xc001600+n*0x10;
                }/* end of loop n */
            }/*0xc001600+n*0x10*/


            {/*0xc001604+n*0x10*/
                GT_U32    n;
                for(n = 0 ; n <= 7 ; n++) {
                    regAddrDbPtr->TTI.MAC2ME.MAC2MEHigh[n] =
                        0xc001604+n*0x10;
                }/* end of loop n */
            }/*0xc001604+n*0x10*/


            {/*0xc001608+n*0x10*/
                GT_U32    n;
                for(n = 0 ; n <= 7 ; n++) {
                    regAddrDbPtr->TTI.MAC2ME.MAC2MELowMask[n] =
                        0xc001608+n*0x10;
                }/* end of loop n */
            }/*0xc001608+n*0x10*/


            {/*0xc00160c+n*0x10*/
                GT_U32    n;
                for(n = 0 ; n <= 7 ; n++) {
                    regAddrDbPtr->TTI.MAC2ME.MAC2MEHighMask[n] =
                        0xc00160c+n*0x10;
                }/* end of loop n */
            }/*0xc00160c+n*0x10*/



            {/*0xc001700+n*4*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->TTI.MAC2ME.MGLookupToInternalTCAMData[n] =
                        0xc001700+n*4;
                }/* end of loop n */
            }/*0xc001700+n*4*/


            regAddrDbPtr->TTI.MAC2ME.MGLookupToInternalTCAMCtrl = 0x0c001708;
            regAddrDbPtr->TTI.MAC2ME.MGLookupToInternalTCAMResult = 0x0c00170c;

        }/*end of unit MAC2ME */


        {/*start of unit protocolMatch */
            {/*0xc000080+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 5 ; n++) {
                    regAddrDbPtr->TTI.protocolMatch.protocolsConfig[n] =
                        0xc000080+n*0x4;
                }/* end of loop n */
            }/*0xc000080+n*0x4*/


            regAddrDbPtr->TTI.protocolMatch.protocolsEncapsulationConfig0 = 0x0c000098;
            regAddrDbPtr->TTI.protocolMatch.protocolsEncapsulationConfig1 = 0x0c00009c;

        }/*end of unit protocolMatch */


        {/*start of unit qoSMapTables */
            {/*0xc000500+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->TTI.qoSMapTables.UPToQoSParametersMapTableReg[n] =
                        0xc000500+n*0x4;
                }/* end of loop n */
            }/*0xc000500+n*0x4*/


            {/*0xc000510+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->TTI.qoSMapTables.EXPToQoSParametersMapTableReg[n] =
                        0xc000510+n*0x4;
                }/* end of loop n */
            }/*0xc000510+n*0x4*/


            regAddrDbPtr->TTI.qoSMapTables.UP2UPMapTable = 0x0c000520;

            {/*0xc000400+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 15 ; n++) {
                    regAddrDbPtr->TTI.qoSMapTables.DSCP_ToQoSProfileMap[n] =
                        0xc000400+n*0x4;
                }/* end of loop n */
            }/*0xc000400+n*0x4*/


            {/*0xc000440+n*0x4+m*0x8+l*0x10*/
                GT_U32    n,m,l;
                for(n = 0 ; n <= 1 ; n++) {
                    for(m = 0 ; m <= 1 ; m++) {
                        for(l = 0 ; l <= 1 ; l++) {
                            regAddrDbPtr->TTI.qoSMapTables.CFIUP_ToQoSProfileMapTable[n][m][l] =
                                0xc000440+n*0x4+m*0x8+l*0x10;
                        }/* end of loop l */
                    }/* end of loop m */
                }/* end of loop n */
            }/*0xc000440+n*0x4+m*0x8+l*0x10*/


            {/*0xc000460+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->TTI.qoSMapTables.EXPToQoSProfileMap[n] =
                        0xc000460+n*0x4;
                }/* end of loop n */
            }/*0xc000460+n*0x4*/


            {/*0xc000470+4*p*/
                GT_U32    p;
                for(p = 0 ; p <= 15 ; p++) {
                    regAddrDbPtr->TTI.qoSMapTables.CFI_UPToQoSProfileMapTableSelector[p] =
                        0xc000470+4*p;
                }/* end of loop p */
            }/*0xc000470+4*p*/



        }/*end of unit qoSMapTables */


        {/*start of unit trunkHash */
            regAddrDbPtr->TTI.trunkHash.trunkHashConfigReg0 = 0x0c000070;
            regAddrDbPtr->TTI.trunkHash.trunkHashConfigReg1 = 0x0c000074;
            regAddrDbPtr->TTI.trunkHash.trunkHashConfigReg2 = 0x0c000078;

        }/*end of unit trunkHash */


        {/*start of unit userDefinedBytes */
            {/*0xc0000a0+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 7 ; n++) {
                    regAddrDbPtr->TTI.userDefinedBytes.IPCLTCPPortRangeComparatorConfig[n] =
                        0xc0000a0+n*0x4;
                }/* end of loop n */
            }/*0xc0000a0+n*0x4*/


            {/*0xc0000c0+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 7 ; n++) {
                    regAddrDbPtr->TTI.userDefinedBytes.IPCLUDPPortRangeComparatorConfig[n] =
                        0xc0000c0+n*0x4;
                }/* end of loop n */
            }/*0xc0000c0+n*0x4*/



        }/*end of unit userDefinedBytes */


        {/*start of unit VLANAssignment */
            {/*0xc000300+i*4*/
                GT_U32    i;
                for(i = 0 ; i <= 3 ; i++) {
                    regAddrDbPtr->TTI.VLANAssignment.ingrTPIDConfig[i] =
                        0xc000300+i*4;
                }/* end of loop i */
            }/*0xc000300+i*4*/


            {/*0xc000310+n*4*/
                GT_U32    n;
                for(n = 0 ; n <= 31 ; n++) {
                    regAddrDbPtr->TTI.VLANAssignment.ingrTPIDSelect[n] =
                        0xc000310+n*4;
                }/* end of loop n */
            }/*0xc000310+n*4*/



        }/*end of unit VLANAssignment */


        regAddrDbPtr->TTI.TTIUnitGlobalConfig = 0x0c000000;
        regAddrDbPtr->TTI.TTIEngineInterruptCause = 0x0c000004;
        regAddrDbPtr->TTI.TTIEngineInterruptMask = 0x0c000008;
        regAddrDbPtr->TTI.TTIEngineConfig = 0x0c00000c;
        regAddrDbPtr->TTI.TTIPCLIDConfig0 = 0x0c000010;
        regAddrDbPtr->TTI.TTIPCLIDConfig1 = 0x0c000014;
        regAddrDbPtr->TTI.TTIIPv4GREEthertype = 0x0c000018;
        regAddrDbPtr->TTI.CNAndFCConfig = 0x0c000024;
        regAddrDbPtr->TTI.specialEtherTypes = 0x0c000028;
        regAddrDbPtr->TTI.ingrCPUTrigSamplingConfig = 0x0c00002c;
        regAddrDbPtr->TTI.MPLSEtherTypes = 0x0c000030;
        regAddrDbPtr->TTI.IPv6ExtensionValue = 0x0c000038;
        regAddrDbPtr->TTI.IPv6SolicitedNodeMcAddrConfigReg[0] = 0x0c000040;
        regAddrDbPtr->TTI.IPv6SolicitedNodeMcAddrConfigReg[1] = 0x0c000044;
        regAddrDbPtr->TTI.IPv6SolicitedNodeMcAddrConfigReg[2] = 0x0c000048;
        regAddrDbPtr->TTI.IPv6SolicitedNodeMcAddrConfigReg[3] = 0x0c00004c;
        regAddrDbPtr->TTI.IPv6SolicitedNodeMcAddrMaskReg[0] = 0x0c000050;
        regAddrDbPtr->TTI.IPv6SolicitedNodeMcAddrMaskReg[1] = 0x0c000054;
        regAddrDbPtr->TTI.IPv6SolicitedNodeMcAddrMaskReg[2] = 0x0c000058;
        regAddrDbPtr->TTI.IPv6SolicitedNodeMcAddrMaskReg[3] = 0x0c00005c;
        regAddrDbPtr->TTI.DSAConfig = 0x0c000060;
        {/*0xc000064+n*4*/
            GT_U32    n;
            for(n = 0 ; n <= 1 ; n++) {
                regAddrDbPtr->TTI.loopPort[n] =
                    0xc000064+n*4;
            }/* end of loop n */
        }/*0xc000064+n*4*/


        {/*0xc0000f0+4*i*/
            GT_U32    i;
            for(i = 0 ; i <= 1 ; i++) {
                regAddrDbPtr->TTI.userDefineEtherTypes[i] =
                    0xc0000f0+4*i;
            }/* end of loop i */
        }/*0xc0000f0+4*i*/


        {/*0xc0000f8+4*i*/
            GT_U32    i;
            for(i = 0 ; i <= 1 ; i++) {
                regAddrDbPtr->TTI.coreRingPortEnable[i] =
                    0xc0000f8+4*i;
            }/* end of loop i */
        }/*0xc0000f8+4*i*/


        {/*0xc000190+4*n*/
            GT_U32    n;
            for(n = 0 ; n <= 1 ; n++) {
                regAddrDbPtr->TTI.ECMCPID[n] =
                    0xc000190+4*n;
            }/* end of loop n */
        }/*0xc000190+4*n*/


        {/*0xc000198+4*n*/
            GT_U32    n;
            for(n = 0 ; n <= 1 ; n++) {
                regAddrDbPtr->TTI.ECMCPIDMask[n] =
                    0xc000198+4*n;
            }/* end of loop n */
        }/*0xc000198+4*n*/


        regAddrDbPtr->TTI.CMtagEtherType = 0x0c0001a0;
        regAddrDbPtr->TTI.PTPEtherTypes = 0x0c0001a4;
        regAddrDbPtr->TTI.PTPOverUDPDestinationPorts = 0x0c0001a8;
        regAddrDbPtr->TTI.PTPPktCommandConfig0 = 0x0c0001ac;
        regAddrDbPtr->TTI.PTPPktCommandConfig1 = 0x0c0001bc;
        regAddrDbPtr->TTI.TTIUnitInterruptSummaryCause = 0x0c0001c0;
        regAddrDbPtr->TTI.TTIUnitInterruptSummaryMask = 0x0c0001c4;
        regAddrDbPtr->TTI.CFMEtherType = 0x0c0001c8;
        regAddrDbPtr->TTI.TTIInternalConfigurations = 0x0c000204;
        {/*0xc000900+n*0x4*/
            GT_U32    n;
            for(n = 0 ; n <= 15 ; n++) {
                regAddrDbPtr->TTI.DSCP2DSCPMapTable[n] =
                    0xc000900+n*0x4;
            }/* end of loop n */
        }/*0xc000900+n*0x4*/


        regAddrDbPtr->TTI.receivedFlowCtrlPktsCntr = 0x0c0014e0;
        regAddrDbPtr->TTI.droppedFlowCtrlPktsCntr = 0x0c0014e4;

    }/*end of unit TTI - TTI  */

}

/*******************************************************************************
*   smemXCat2Init
*
* DESCRIPTION:
*       Init memory module for the xCat2 device.
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
void smemXCat2Init
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_CHT_DEV_COMMON_MEM_INFO * commonDevMemInfoPtr;
    SMEM_CHT_GENERIC_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32 ii;

    devMemInfoPtr = smemDeviceObjMemoryAlloc(devObjPtr,1, sizeof(SMEM_CHT_GENERIC_DEV_MEM_INFO));
    if (devMemInfoPtr == 0)
    {
        skernelFatalError("smemXCat2Init: allocation error\n");
    }

    devObjPtr->deviceMemory = devMemInfoPtr;
    commonDevMemInfoPtr = &devMemInfoPtr->common;
    commonDevMemInfoPtr->isPartOfGeneric = 1;

    commonDevMemInfoPtr->pciUnitBaseAddrMask = 0x000f0000;
    commonDevMemInfoPtr->pciUnitBaseAddr = 0x00040000;
    commonDevMemInfoPtr->pciExtMemArr[SMEM_UNIT_PCI_BUS_MBUS_E].unitBaseAddr =
        commonDevMemInfoPtr->pciUnitBaseAddr;
    devObjPtr->supportCpuInternalInterface = 1;

    /* Set the pointer to the active memory */
    devObjPtr->activeMemPtr = smemXCat2ActiveTable;
    devObjPtr->devFindMemFunPtr = (void *)smemXCat2FindMem;


    /* xCat2 */
    /* bind the router TCAM info */
    devObjPtr->routeTcamInfo =  xcat2RoutTcamInfo;
    devObjPtr->pclTcamInfoPtr = &xcat2GlobalPclData;
    devObjPtr->pclTcamMaxNumEntries = devObjPtr->pclTcamInfoPtr->bankEntriesNum * 4;
    devObjPtr->cncBlocksNum = 8;
    devObjPtr->cncBlockMaxRangeIndex = 256;
    devObjPtr->fdbNumEntries = SMEM_MAC_TABLE_SIZE_16KB;
    devObjPtr->fdbMaxNumEntries = devObjPtr->fdbNumEntries;
    devObjPtr->supportEqEgressMonitoringNumPorts = 32;

    devObjPtr->policerEngineNum = 2;

    devObjPtr->isXcatA0Features = 0;/*state that features are NOT like xcat-A0*/
    devObjPtr->supportTunnelStartEthOverIpv4 = 1;

    devObjPtr->supportPortIsolation = 1;
    devObjPtr->supportOamPduTrap = 1;
    devObjPtr->supportPortOamLoopBack = 1;
    devObjPtr->supportMultiAnalyzerMirroring = 1;
    devObjPtr->supportLogicalTargetMapping = 1;
    devObjPtr->ipFixTimeStampClockDiff[0] =
    devObjPtr->ipFixTimeStampClockDiff[1] =
    devObjPtr->ipFixTimeStampClockDiff[2] = 0;
    devObjPtr->supportConfigurableTtiPclId = 1;
    devObjPtr->supportVlanEntryVidxFloodMode = 1;
    devObjPtr->supportRemoveVlanTag1WhenEmpty = 1;
    devObjPtr->errata.ttCopyToCpuWithAdditionalTag = 1;
    devObjPtr->errata.fastStackFailover_Drop_on_source = 1;


    devObjPtr->supportOnlyUcRouter = 1;
    devObjPtr->policerSupport.supportPolicerMemoryControl = 1;
    devObjPtr->policerSupport.iplr0TableSize = POLICER_TOTAL_MEMORY_CNS;
    devObjPtr->policerSupport.iplr1TableSize = 0;
    devObjPtr->policerSupport.iplrTableSize =
        devObjPtr->policerSupport.iplr0TableSize +
        devObjPtr->policerSupport.iplr1TableSize;

    devObjPtr->supportRegistersDb = 1;
    devObjPtr->policerSupport.supportCountingEntryFormatSelect = 1;
    devObjPtr->policerSupport.supportOnlyShortCountingEntryFormat = 1;
    devObjPtr->supportPtp = 1;
    devObjPtr->supportCrcTrunkHashMode = 1;
    devObjPtr->supportGemUserId = 0;
    devObjPtr->supportKeepVlan1 = 1;
    devObjPtr->supportResourceHistogram = 0;
    devObjPtr->policerSupport.supportPolicerEnableCountingTriggerByPort = 1;
    devObjPtr->policerSupport.supportPortModeAddressSelect = 1;
    devObjPtr->policerSupport.supportEplrPerPort = 1;
    devObjPtr->supportHa64Ports = 1;
    devObjPtr->supportFdbNewNaToCpuMsgFormat = 1;
    devObjPtr->pclSupport.iPclSupport5Ude = 1;
    devObjPtr->pclSupport.iPclSupportTrunkHash = 1;
    devObjPtr->pclSupport.ePclSupportVidUpTag1 = 1;
    devObjPtr->pclSupport.ipclSupportSeparatedIConfigTables = 1;
    devObjPtr->pclSupport.pclSupportPortList = 1;
    devObjPtr->cncClientSupportBitmap = SNET_CHT3_CNC_CLIENTS_BMP_ALL_CNS;
    devObjPtr->supportXgMacMibCountersControllOnAllPorts = 1;
    devObjPtr->iPclKeyFormatVersion = 1;
    devObjPtr->ePclKeyFormatVersion = 1;
    devObjPtr->pclTcamFormatVersion = 1;
    devObjPtr->supportMcTrunkHashSrcInfo = 1;
    devObjPtr->supportMacSaAssignModePerPort = 1;

    devObjPtr->errata.ieeeReservedMcConfigRegRead = 1;
    devObjPtr->errata.crcTrunkHashL4InfoInIpv4Other = 1;

    devObjPtr->supportForwardFcPackets = 1;
    devObjPtr->supportForwardUnknowMacControlFrames = 1;
    devObjPtr->supportForwardPfcFrames = 1;
    devObjPtr->support1024to1518MibCounter = 1;

    devObjPtr->memUnitBaseAddrInfo.policer[0] = policerBaseAddr[0];
    devObjPtr->memUnitBaseAddrInfo.policer[1] = policerBaseAddr[1];
    devObjPtr->memUnitBaseAddrInfo.policer[2] = policerBaseAddr[2];

    /* Timestamp base addresses */
    devObjPtr->memUnitBaseAddrInfo.gts[0] = timestampBaseAddr[0];
    devObjPtr->memUnitBaseAddrInfo.gts[1] = timestampBaseAddr[1];

    for(ii = 0 ; ii < smemXCat2ActiveTableSizeOf; ii++)
    {
        if(smemXCat2ActiveTable[ii].mask ==
            POLICER_ACTIVE_MEMORY_PLACE_HOLDER_MASK_CNS)
        {
            GT_U32  tmpMask = SMEM_FULL_MASK_CNS;

            if(smemXCat2ActiveTable[ii + 0].address == POLICER_MANAGEMENT_COUNTER_ADDR_CNS)
            {
                /* management counters */
                /* 0x40 between sets , 0x10 between counters */
                /* 0x00 , 0x10 , 0x20 , 0x30 */
                /* 0x40 , 0x50 , 0x60 , 0x70 */
                /* 0x80 , 0x90 , 0xa0 , 0xb0 */
                tmpMask = POLICER_MANAGEMENT_COUNTER_MASK_CNS;
            }

            smemXCat2ActiveTable[ii + 0].address += policerBaseAddr[0];
            smemXCat2ActiveTable[ii + 0].mask = tmpMask;

            smemXCat2ActiveTable[ii + 1].address += policerBaseAddr[1];
            smemXCat2ActiveTable[ii + 1].mask = tmpMask;

            smemXCat2ActiveTable[ii + 2].address += policerBaseAddr[2];
            smemXCat2ActiveTable[ii + 2].mask = tmpMask;

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

    /* init specific functions array */
    smemXCat2InitFuncArray(devObjPtr, commonDevMemInfoPtr);
    smemXCat2AllocSpecMemory(devObjPtr);
    smemXCat2RegsInfoSet(devObjPtr);
    smemXCat2TableInfoSet(devObjPtr);
}

/*******************************************************************************
*   smemXCat2Init2
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
void smemXCat2Init2
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    GT_U32 fieldVal;

    smemChtInit2(devObjPtr);

    /* Do specific init for XCat2 */

    /* Read the DevID from register Device ID  */
    smemRegFldGet(devObjPtr, SMEM_CHT_DEVICE_ID_REG(devObjPtr), 4, 16, &fieldVal);
    /* Write the DevID to register DeviceID Status */
    smemRegFldSet(devObjPtr, 0x008F8240, 0, 16, fieldVal);

    /* Read Revision ID from register DeviceID Status */
    smemRegFldGet(devObjPtr, 0x008F8240, 20, 4, &fieldVal);
    devObjPtr->deviceRevisionId = fieldVal;

    return;
}

/*******************************************************************************
*  smemXCat2ActiveWritePolicerMemoryControl
*
* DESCRIPTION:
*    Set Hierarchical Policer control register in iplr0, and update Policer Memory Control
*         configuration
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
void smemXCat2ActiveWritePolicerMemoryControl
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

    /* get "Policer Memory Control" field - bits 7:5 */
    fldValue = SMEM_U32_GET_FIELD(*inMemPtr, 5, 3);
    switch (fldValue)
    {
        case 0: /* PLR0_172_84_28:Policer 0 uses all three memories (172+84+172) Policer 1 has no memory resources */
            devObjPtr->tablesInfo.policer.commonInfo.baseAddress = meterBase;
            devObjPtr->tablesInfo.policer.paramInfo[1].step = SMAIN_NOT_VALID_CNS;
            devObjPtr->tablesInfo.policerCounters.commonInfo.baseAddress = countBase;
            devObjPtr->tablesInfo.policerCounters.paramInfo[1].step = SMAIN_NOT_VALID_CNS;
            devObjPtr->policerSupport.iplr0TableSize = POLICER_TOTAL_MEMORY_CNS;
            devObjPtr->policerSupport.iplr1TableSize = POLICER_TOTAL_MEMORY_CNS-devObjPtr->policerSupport.iplr0TableSize;
            devObjPtr->policerSupport.iplr1EntriesFirst = 0;
            break;
        case 1: /* PLR0_172_84_PLR1_28:Policer 0 uses two memories (172+84) Policer 1 uses one memory (28) */
            devObjPtr->tablesInfo.policer.commonInfo.baseAddress = meterBase;
            devObjPtr->tablesInfo.policer.paramInfo[1].step = (POLICER_MEMORY_172_CNS+POLICER_MEMORY_84_CNS) * meterEntrySize;
            devObjPtr->tablesInfo.policerCounters.commonInfo.baseAddress = countBase;
            devObjPtr->tablesInfo.policerCounters.paramInfo[1].step = (POLICER_MEMORY_172_CNS+POLICER_MEMORY_84_CNS) * countEntrySize;
            devObjPtr->policerSupport.iplr0TableSize = POLICER_MEMORY_172_CNS+POLICER_MEMORY_84_CNS;
            devObjPtr->policerSupport.iplr1TableSize = POLICER_TOTAL_MEMORY_CNS-devObjPtr->policerSupport.iplr0TableSize;
            devObjPtr->policerSupport.iplr1EntriesFirst = 0;
            break;
        case 2: /* PLR0_172_PLR1_84_28:Policer 0 uses one memories (172) Policer 1 uses two memory (84+28) */
            devObjPtr->tablesInfo.policer.commonInfo.baseAddress = meterBase;
            devObjPtr->tablesInfo.policer.paramInfo[1].step = (POLICER_MEMORY_172_CNS) * meterEntrySize;
            devObjPtr->tablesInfo.policerCounters.commonInfo.baseAddress = countBase;
            devObjPtr->tablesInfo.policerCounters.paramInfo[1].step = (POLICER_MEMORY_172_CNS) * countEntrySize;
            devObjPtr->policerSupport.iplr0TableSize = POLICER_MEMORY_172_CNS;
            devObjPtr->policerSupport.iplr1TableSize = POLICER_TOTAL_MEMORY_CNS-devObjPtr->policerSupport.iplr0TableSize;
            devObjPtr->policerSupport.iplr1EntriesFirst = 0;
            break;
        case 4: /* PLR1_172_84_28:Policer 0 has no memory resources Policer 1 uses all three memories (172+84+172) */
            devObjPtr->tablesInfo.policer.commonInfo.baseAddress = SMAIN_NOT_VALID_CNS;
            devObjPtr->tablesInfo.policer.paramInfo[1].step = meterBase - SMAIN_NOT_VALID_CNS;
            devObjPtr->tablesInfo.policerCounters.commonInfo.baseAddress = SMAIN_NOT_VALID_CNS;
            devObjPtr->tablesInfo.policerCounters.paramInfo[1].step = countBase - SMAIN_NOT_VALID_CNS;
            /* Note: (commonInfo.baseAddress + paramInfo[1].step) now points to start of table */
            devObjPtr->policerSupport.iplr0TableSize = 0;
            devObjPtr->policerSupport.iplr1TableSize = POLICER_TOTAL_MEMORY_CNS-devObjPtr->policerSupport.iplr0TableSize;
            devObjPtr->policerSupport.iplr1EntriesFirst = 0;
            break;
        case 5: /* PLR0_28_PLR1_172_84:Policer 0 uses one memory (28) Policer 1 uses two memories (172+84) */
            devObjPtr->tablesInfo.policer.commonInfo.baseAddress = meterBase + (POLICER_MEMORY_172_CNS+POLICER_MEMORY_84_CNS) * meterEntrySize;
            devObjPtr->tablesInfo.policer.paramInfo[1].step = 0 - ((POLICER_MEMORY_172_CNS+POLICER_MEMORY_84_CNS) * meterEntrySize);
            devObjPtr->tablesInfo.policerCounters.commonInfo.baseAddress = countBase + (POLICER_MEMORY_172_CNS+POLICER_MEMORY_84_CNS) * countEntrySize;
            devObjPtr->tablesInfo.policerCounters.paramInfo[1].step = 0 - ((POLICER_MEMORY_172_CNS+POLICER_MEMORY_84_CNS) * countEntrySize);
            devObjPtr->policerSupport.iplr0TableSize = POLICER_MEMORY_28_CNS;
            devObjPtr->policerSupport.iplr1TableSize = POLICER_TOTAL_MEMORY_CNS-devObjPtr->policerSupport.iplr0TableSize;
            devObjPtr->policerSupport.iplr1EntriesFirst = 1;
            break;
        case 6: /* PLR0_84_28_PLR1_172:Policer 0 uses two memory (84+28) Policer 1 uses one memories (172) */
            devObjPtr->tablesInfo.policer.commonInfo.baseAddress = meterBase + POLICER_MEMORY_172_CNS * meterEntrySize;
            devObjPtr->tablesInfo.policer.paramInfo[1].step = 0 - (POLICER_MEMORY_172_CNS * meterEntrySize);
            devObjPtr->tablesInfo.policerCounters.commonInfo.baseAddress = countBase + POLICER_MEMORY_172_CNS * countEntrySize;
            devObjPtr->tablesInfo.policerCounters.paramInfo[1].step = 0 - (POLICER_MEMORY_172_CNS * countEntrySize);
            devObjPtr->policerSupport.iplr0TableSize = POLICER_MEMORY_84_CNS+POLICER_MEMORY_28_CNS;
            devObjPtr->policerSupport.iplr1TableSize = POLICER_TOTAL_MEMORY_CNS-devObjPtr->policerSupport.iplr0TableSize;
            devObjPtr->policerSupport.iplr1EntriesFirst = 1;
            break;
        default:
            break;
    }
}
