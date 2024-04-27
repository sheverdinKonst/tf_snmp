/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* smemTiger.c
*
* DESCRIPTION:
*       This is API implementation for Tiger memories.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 17 $
*
*******************************************************************************/
#include <os/simTypes.h>
#include <asicSimulation/SKernel/smem/smemTiger.h>
#include <asicSimulation/SKernel/twistCommon/sregTwist.h>
#include <asicSimulation/SKernel/twistCommon/sregTiger.h>
#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SKernel/smain/smain.h>
#include <asicSimulation/SKernel/suserframes/snetTwistTrafficCond.h>
#include <asicSimulation/SKernel/suserframes/snet.h>


static GT_U32 *  smemTigerFatalError(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemTigerGlobalReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemTigerTransQueReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);

static GT_U32 *  smemTigerEtherBrdgReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemTigerLxReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemTigerBufMngReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemTigerMacReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemTigerPortGroupConfReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemTigerMacTableReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemTigerVlanTableReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static void smemTigerInitFuncArray(
    INOUT TIGER_DEV_MEM_INFO  * devMemInfoPtr
);
static void smemTigerAllocSpecMemory(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    INOUT TIGER_DEV_MEM_INFO  * devMemInfoPtr
);

static void smemTigerResetAuq(
    INOUT TIGER_DEV_MEM_INFO  * devMemInfoPtr
);

static void smemTigerSetPciDefault(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    INOUT TIGER_DEV_MEM_INFO  * devMemInfoPtr
);


extern void smemTwistActiveWriteFdbActionTrigger(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
);

extern void smemTwistActiveWriteMacControl (
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
);

static void smemTigerActiveReadPortIntReg (
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    OUT        GT_U32 * outMemPtr
);

static void smemTigerActiveReadBrdgIntReg (
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    OUT        GT_U32 * outMemPtr
);

static void smemTigerActiveReadSdmaIntReg (
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    OUT        GT_U32 * outMemPtr
);

static GT_U32 *  smemTigerExternalMem(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);

static void smemTigerActiveWriteFdbMsg (
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
);

static void smemTigerActiveAuqBaseWrite (
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
);

static void smemTigerActiveReadCntrs (
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    OUT        GT_U32 * outMemPtr
);

static void smemTigerActiveWriteXSmii (
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
);

static void smemTigerActiveWriteSFlowCnt(
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
);

static void smemTigerActiveWriteTcbGenxsReg(
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
);

static void smemTigerActiveReadPortEgressCountersReg(
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    OUT        GT_U32 * outMemPtr
);


static void smemTigerActiveWriteTransmitQueueCmdReg (
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
);

static void smemTigerActivePciWriteIntr (
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
);

static GT_U32 *  smemTigerPciReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);

static void smemMemSizeToBytes(
        IN GT_CHAR * memSizePtr,
        INOUT GT_U32 * memBytesPtr
);

static void smemTigerPhyRegsInit(
    IN SKERNEL_DEVICE_OBJECT    *       deviceObjPtr,
    INOUT TIGER_DEV_MEM_INFO    *       devMemInfoPtr
);

static GT_U32 *  smemTigerFaMem(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);

static void smemTigerActiveReadTcbCounterAlarmReg (
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    OUT        GT_U32 * outMemPtr
);

static void smemTigerActiveReadTcbPolicingCounterAlarmReg (
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    OUT        GT_U32 * outMemPtr
);

static void smemTigerActiveReadTcbGlobalCounterReg (
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    OUT        GT_U32 * outMemPtr
);

static GT_VOID smemTigerNsramConfigUpdate (
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr
);

static void smemTigerActiveReadInterruptsCauseReg (
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    OUT        GT_U32 * outMemPtr
);

static void smemTigerActiveWriteMacInterruptsMaskReg (
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
);

static void smemTigerActiveWriteTcamMemory (
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
);

static void smemTigerActiveReadTcamMemory (
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    OUT        GT_U32 * outMemPtr
);


/* Private definition */
#define     GLOB_REGS_NUM               (0x3ff / 4 + 1)
#define     TWSI_INT_REGS_NUM           (0x28 / 4 + 1)
#define     SDMA_REGS_NUM               (0x8ff / 4 + 1)
#define     BM_REGS_NUM                 (0x4fff / 4 + 1)
#define     EPF_REGS_NUM                (0x4fff / 4 + 1)
#define     EGR_REGS_NUM                (0x28 / 4 + 1)
#define     VLAN_CONF_MEM_REGS_NUM      (0x7c / 4 + 1)
#define     CPU_QOS_ATTR_REGS_NUM       (0xfc / 4 + 1)
#define     LX_GEN_REGS_NUM             (0x3ff / 4 + 1)
#define     DSCP_REGS_NUM               (0x48)
#define     IP_FLOW_CLASS_REGS_NUM      (0x1f / 4 + 1)
#define     FLOW_TEMPLATE_CNF_REGS_NUM  (0x8fff / 4 + 1)
#define     USER_DEF_TBL_REGS_NUM       (0x2ff / 4 + 1)
#define     INLIF_PER_PORT_TBL_REGS_NUM (0x3ff / 4 + 1)
#define     INLIF_PER_VLAN_TBL_REGS_NUM (0x1fff / 4 + 1)
#define     IP_FLOW_ACTION_REGS_NUM     (0x3ff / 4 + 1)
#define     DSCP_2_COS_REGS_NUM         (0x1ff / 4 + 1)
#define     INGR_POLICY_MNG_REGS_NUM    (0x3ff / 4 + 1)
#define     IPV4_IPV6_MNG_REGS_NUM      (0x3ff / 4 + 1)
#define     INGR_POL_REM_TBL_REGS_NUM   (0x3ff / 4 + 1)
#define     SRAM_INTER_REGS_NUM         (0x1ff / 4 + 1)
#define     TCAM_REGS_NUM               (0x4000)
#define     TCAM_ACCESS_REGS_NUM        (0xFF)
#define     TRUNK_TBL_REGS_NUM          (0x2DC / 4 + 1)
#define     MC_BUF_COUNT_REGS_NUM       (0x2fff / 4 + 1)
#define     MLL_TBL_REGS_NUM            (0x6000 / 4 + 1)
#define     EXT_MEM_CONF_NUM            (0x2CF / 4 + 1)
#define     MAC_REGS_NUM                (0xEC / 4 + 1)
#define     MAC_PORTS_NUM               (0x3F)
#define     PER_GROUPS_TYPE_NUM         (0x48)
#define     MAC_CNT_TYPES_NUM           (0xD7C / 4 + 1)
#define     SFLOW_NUM                   (0x0C / 4 + 1)
#define     BRG_PORTS_NUM                (64)
#define     PORT_REGS_GROUPS_NUM        (0x14 / 4 + 1)
#define     PORT_PROT_VID_NUM           (0x20)
#define     BRG_GEN_REGS_NUM            (0xB270 / 4 + 1)
#define     GEN_EGRS_GROUP_NUM          (0x1FFF / 4 + 1)
#define     TC_QUE_DESCR_NUM            (8)
#define     MAC_TBL_REGS_NUM            (16 * 1024 * 4)
#define     VLAN_TABLE_REGS_NUM         (8 * 1024 * 4 * 4)
#define     PCI_MEM_REGS_NUM            (0x3C / 4 + 1)
#define     PCI_INTERNAL_REGS_NUM       (0x18 / 4 + 1)
#define     PHY_XAUI_DEV_NUM            (6)
#define     PHY_IEEE_XAUI_REGS_NUM      (0xff * PHY_XAUI_DEV_NUM)
#define     PHY_EXT_XAUI_REGS_NUM       (0xff * PHY_XAUI_DEV_NUM)
#define     EGR_TRUNK_REGS_NUM          (48 * 2)


/* Register special function index in function array (Bits 23:28)*/
#define     REG_SPEC_FUNC_INDEX         0x1F800000

/* number of words in the FDB update message */
#define     SMEM_TIGER_FDB_UPD_MSG_WORDS_NUM    4

/* MAC Counters address mask */
#define     SMEM_TIGER_COUNT_MSK_CNS        0xfffff000
#define     SMEM_TIGER_GOPINT_MSK_CNS       0xfffffff7
#define     SMEM_TIGER_PORT_EGRESS_COUNTERS_CNS 0xffffefff

#define     SMEM_TIGER_NSRAM_INTERNAL      (32 * 0x400)
#define     SMEM_TIGER_NSRAM_EXTERNAL      (8 * 0x100000)

/* Active memory table */
static SMEM_ACTIVE_MEM_ENTRY_STC smemTigerActiveTable[] =
{
    /* GOP<n> Interrupt Cause Register0*/
    {0x03800090, SMEM_TIGER_GOPINT_MSK_CNS,
                                smemTigerActiveReadPortIntReg,  14, NULL, 0},
    {0x038000A0, SMEM_TIGER_GOPINT_MSK_CNS,
                                smemTigerActiveReadPortIntReg,  14, NULL, 0},
    {0x038000B0, SMEM_TIGER_GOPINT_MSK_CNS,
                                smemTigerActiveReadPortIntReg,  14, NULL, 0},
    {0x038000C0, SMEM_TIGER_GOPINT_MSK_CNS,
                                smemTigerActiveReadPortIntReg,  19, NULL, 0},
    {0x038000D0, SMEM_TIGER_GOPINT_MSK_CNS,
                                smemTigerActiveReadPortIntReg,  19, NULL, 0},
    {0x038000E0, SMEM_TIGER_GOPINT_MSK_CNS,
                                smemTigerActiveReadPortIntReg,  19, NULL, 0},

    {0x00000040, SMEM_FULL_MASK_CNS,
                                smemTigerActiveReadPortIntReg,  16, NULL, 0},


    /* MAC Table Action0 Register */
    {0x02040014, SMEM_FULL_MASK_CNS, NULL, 0 ,
                                smemTwistActiveWriteFdbActionTrigger, 0},

    /* MAC Table Interrupt Cause Register */
    {0x02040130, SMEM_FULL_MASK_CNS,
                                smemTigerActiveReadBrdgIntReg,11, NULL, 0},

    /* SDMA Receive SDMA Interrupt Cause Register (RxSDMAInt) */
    {0x0000280C, SMEM_FULL_MASK_CNS,
                                smemTigerActiveReadSdmaIntReg, 17, NULL, 0},

    /* Message from CPU Register0 */
    {0x00000048, SMEM_FULL_MASK_CNS, NULL, 0 , smemTigerActiveWriteFdbMsg, 0},

    /* AUQ Base address reg 0x14 */
    {0x00000014, SMEM_FULL_MASK_CNS, NULL, 0 , smemTigerActiveAuqBaseWrite, 0},

    /* MAC counters */
    {0x00810000, SMEM_TIGER_COUNT_MSK_CNS, smemTigerActiveReadCntrs, 0, NULL,0},
    {0x01010000, SMEM_TIGER_COUNT_MSK_CNS, smemTigerActiveReadCntrs, 0, NULL,0},

    /* MAC Control Register mask 0x00800000 */
    {0x00800000, SMEM_TWIST_MAC_CTRL_MASK_CNS,
        NULL, 0, smemTwistActiveWriteMacControl, 0},

    /* MAC Control Register mask 0x01000000 */
    {0x01000000, SMEM_TWIST_MAC_CTRL_MASK_CNS,
        NULL, 0, smemTwistActiveWriteMacControl, 1},

    /* 10 G SMII control register */
    {0x03800004, SMEM_FULL_MASK_CNS, NULL, 0 , smemTigerActiveWriteXSmii, 0},

    /* sFlow Value Register */
    {0x03800104, SMEM_FULL_MASK_CNS, NULL, 0 , smemTigerActiveWriteSFlowCnt, 0},

    /* tcbGenxsRegAddr Samba */
    {0x028000C4, SMEM_FULL_MASK_CNS, NULL, 0 , smemTigerActiveWriteTcbGenxsReg, 0},

    /* port egress counters --- read only , reset on read */
    /*1*/    {0x01800E04, SMEM_TIGER_PORT_EGRESS_COUNTERS_CNS,
            smemTigerActiveReadPortEgressCountersReg, 0 , NULL ,0},
    /*2*/    {0x01800E08, SMEM_TIGER_PORT_EGRESS_COUNTERS_CNS,
            smemTigerActiveReadPortEgressCountersReg, 0 , NULL ,0},
    /*3*/    {0x01800E0C, SMEM_TIGER_PORT_EGRESS_COUNTERS_CNS,
            smemTigerActiveReadPortEgressCountersReg, 0 , NULL ,0},
    /*4*/    {0x01800F00, SMEM_TIGER_PORT_EGRESS_COUNTERS_CNS,
            smemTigerActiveReadPortEgressCountersReg, 0 , NULL ,0},
    /*5*/    {0x01800F04, SMEM_TIGER_PORT_EGRESS_COUNTERS_CNS,
            smemTigerActiveReadPortEgressCountersReg, 0 , NULL ,0},

    /* txQCmdReg Register */
    {0x00002868, SMEM_FULL_MASK_CNS, NULL, 0 ,
                                     smemTigerActiveWriteTransmitQueueCmdReg, 0},

    /* TCB count alarm register */
    {0x02800074, (SMEM_FULL_MASK_CNS),
        smemTigerActiveReadTcbCounterAlarmReg, 0 , NULL, 0},

    /* TCB policing count alarm register */
    {0x02800078, (SMEM_FULL_MASK_CNS),
        smemTigerActiveReadTcbPolicingCounterAlarmReg, 0 , NULL, 0},

    /* tcb global counters */
    /* 1 */
    {TCB_GLOBAL_RECEIVED_PACKETS_REG ,(SMEM_FULL_MASK_CNS),
        smemTigerActiveReadTcbGlobalCounterReg, 0 , NULL, 0},
    /* 2 */
    {TCB_GLOBAL_RECEIVED_OCTETS_LOW_REG ,(SMEM_FULL_MASK_CNS),
        smemTigerActiveReadTcbGlobalCounterReg, 0 , NULL, 0},
    /* 3 */
    {TCB_GLOBAL_RECEIVED_OCTETS_HI_REG ,(SMEM_FULL_MASK_CNS),
        smemTigerActiveReadTcbGlobalCounterReg, 0 , NULL, 0},
    /* 4 */
    {TCB_GLOBAL_DROPPED_PACKETS_REG ,(SMEM_FULL_MASK_CNS),
        smemTigerActiveReadTcbGlobalCounterReg, 0 , NULL, 0},
    /* 5 */
    {TCB_GLOBAL_DROPPED_OCTETS_LOW_REG ,(SMEM_FULL_MASK_CNS),
        smemTigerActiveReadTcbGlobalCounterReg, 0 , NULL, 0},
    /* 6 */
    {TCB_GLOBAL_DROPPED_OCTETS_HI_REG ,(SMEM_FULL_MASK_CNS),
        smemTigerActiveReadTcbGlobalCounterReg, 0 , NULL, 0},

    /* read interrupts cause registers -- ROC register */
    {0x00000114, SMEM_FULL_MASK_CNS, smemTigerActiveReadInterruptsCauseReg, 0 , NULL,0},
    {0x00002810, SMEM_FULL_MASK_CNS, smemTigerActiveReadInterruptsCauseReg, 0 , NULL,0},
    {MISC_INTR_MASK_REG,SMEM_FULL_MASK_CNS, NULL, 0 , smemTigerActiveWriteMacInterruptsMaskReg,0},

    /* Policy TCAM Access Control Register */
    {0x02E30000, TCAM_WRITE_ACCESS_MASK_CNS,
        smemTigerActiveReadTcamMemory, 0, smemTigerActiveWriteTcamMemory, 0},
    {0x02E20000, TCAM_WRITE_ACCESS_MASK_CNS,
        smemTigerActiveReadTcamMemory, 0, smemTigerActiveWriteTcamMemory, 0},
    {0x02E50000, TCAM_WRITE_ACCESS_MASK_CNS,
        smemTigerActiveReadTcamMemory, 0, smemTigerActiveWriteTcamMemory, 0},
    {0x02E40000, TCAM_WRITE_ACCESS_MASK_CNS,
        smemTigerActiveReadTcamMemory, 0, smemTigerActiveWriteTcamMemory, 0},

    /* must be last anyway */
    {0xffffffff, SMEM_FULL_MASK_CNS, NULL,0,NULL,0}
};

#define SMEM_ACTIVE_MEM_TABLE_SIZE \
    (sizeof(smemTigerActiveTable)/sizeof(smemTigerActiveTable[0]))

/* Active memory table */
static SMEM_ACTIVE_MEM_ENTRY_STC smemTigerPciActiveTable[] =
{
    /* PCI interrupt cause */
    {0x00000114, SMEM_FULL_MASK_CNS, NULL, 0 , smemTigerActivePciWriteIntr, 0},

    /* must be last anyway */
    {0xffffffff, SMEM_FULL_MASK_CNS, NULL,0,NULL,0}
};

#define SMEM_ACTIVE_PCI_MEM_TABLE_SIZE \
    (sizeof(smemTigerPciActiveTable)/sizeof(smemTigerPciActiveTable[0]))

/* temporary patch for FA/XBAR familly */
static GT_U32 smemFaMemory[0xff];

/*******************************************************************************
*   smemTigerNsramConfigUpdate
*
* DESCRIPTION:
*       Set up narrow sram configuration
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
static GT_VOID smemTigerNsramConfigUpdate
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr
)
{
    TIGER_DEV_MEM_INFO * devMemInfoPtr; /* device memory info pointer */
    char   param_str[SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS];
                                        /* string for parameter */
    GT_U32 param_val;                   /* parameter value */
    char   keyString[20];               /* key string */
    TIGER_NSRAM_MODE_ENT nSramMode;     /* narrow SRAM configuration */
    GT_U32 nSramSize;                   /* external NSRAM size */

    devMemInfoPtr = (TIGER_DEV_MEM_INFO*)(deviceObjPtr->deviceMemory);

    /* narow sram type */
    nSramSize = SMEM_TIGER_NSRAM_EXTERNAL / sizeof(SMEM_REGISTER);

    sprintf(keyString, "dev%d_nsram", deviceObjPtr->deviceId);
    if (SIM_OS_MAC(simOsGetCnfValue)("system",  keyString,
                          SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS,
                          param_str))
    {
        /* get nsram size in bytes */
        smemMemSizeToBytes(param_str, &nSramSize);
        /* convert to words number */
        nSramSize /= sizeof(SMEM_REGISTER);
    }

    nSramMode = TIGER_NSRAM_ALL_EXTERNAL_E;

    /* narow sram type */
    sprintf(keyString, "dev%d_nsram_mode", deviceObjPtr->deviceId);
    if (SIM_OS_MAC(simOsGetCnfValue)("system", keyString,
                          SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS,
                          param_str))
    {
        sscanf(param_str, "%d", &param_val);
        nSramMode = param_val;
    }

    switch (nSramMode)
    {
    case TIGER_NSRAM_ONE_FOUR_INTERNAL_E:
        devMemInfoPtr->nsramsMem[0].nSramSize = 0;
        devMemInfoPtr->nsramsMem[1].nSramSize = nSramSize;
        devMemInfoPtr->nsramsMem[2].nSramSize = nSramSize;
        devMemInfoPtr->nsramsMem[3].nSramSize = SMEM_TIGER_NSRAM_INTERNAL;
    break;
    case TIGER_NSRAM_ALL_EXTERNAL_E:
        devMemInfoPtr->nsramsMem[0].nSramSize = 0;
        devMemInfoPtr->nsramsMem[1].nSramSize = nSramSize;
        devMemInfoPtr->nsramsMem[2].nSramSize = nSramSize;
        devMemInfoPtr->nsramsMem[3].nSramSize = 0;
        break;
    case TIGER_NSRAM_ONE_TWO_INTERNAL_E:
        devMemInfoPtr->nsramsMem[0].nSramSize = 0;
        devMemInfoPtr->nsramsMem[1].nSramSize = nSramSize;
        devMemInfoPtr->nsramsMem[2].nSramSize = SMEM_TIGER_NSRAM_INTERNAL;
        devMemInfoPtr->nsramsMem[3].nSramSize = 0;
        break;
    case TIGER_NSRAM_THREE_FOUR_INTERNAL_E:
        devMemInfoPtr->nsramsMem[0].nSramSize = 0;
        devMemInfoPtr->nsramsMem[1].nSramSize = nSramSize;
        devMemInfoPtr->nsramsMem[2].nSramSize = SMEM_TIGER_NSRAM_INTERNAL;
        devMemInfoPtr->nsramsMem[3].nSramSize = SMEM_TIGER_NSRAM_INTERNAL;
        break;
    case TIGER_NSRAM_ALL_INTERNAL_E:
        devMemInfoPtr->nsramsMem[0].nSramSize = 0;
        devMemInfoPtr->nsramsMem[1].nSramSize = 0;
        devMemInfoPtr->nsramsMem[2].nSramSize = SMEM_TIGER_NSRAM_INTERNAL;
        devMemInfoPtr->nsramsMem[3].nSramSize = SMEM_TIGER_NSRAM_INTERNAL;
        break;
    default:
        skernelFatalError("smemTigerNsramConfigUpdate: invalid NSRAM mode\n");
    }
}

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
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr
)
{
    TIGER_DEV_MEM_INFO  * devMemInfoPtr;

    /* string for parameter */
    char   param_str[SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS];
    /* key string */
    char   keyString[20];

    /* alloc TIGER_DEV_MEM_INFO */
    devMemInfoPtr = (TIGER_DEV_MEM_INFO *)calloc(1, sizeof(TIGER_DEV_MEM_INFO));
    if (devMemInfoPtr == 0)
    {
        skernelFatalError("smemTigerInit: allocation error\n");
    }

    deviceObjPtr->deviceMemory = devMemInfoPtr;

    /* get wide SRAM size from *.ini file */
    sprintf(keyString, "dev%d_wsram", deviceObjPtr->deviceId);
    if (!SIM_OS_MAC(simOsGetCnfValue)("system",  keyString,
                             SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS, param_str))
    {
        /* The control (wide SRAM) memory is arranged in 8K lines x 16 bytes. */
        devMemInfoPtr->wsramMem.wSramSize = 8 * 1024 * 16 / sizeof(SMEM_REGISTER);
    }
    else
    {
        /* get wsram size in bytes */
        smemMemSizeToBytes(param_str, &devMemInfoPtr->wsramMem.wSramSize);
        /* convert to words number */
        devMemInfoPtr->wsramMem.wSramSize /= sizeof(SMEM_REGISTER);
    }

    /* init narrow sram lpm access per ip octets for ipv4 & ipv6 */
    smemTigerNsramConfigUpdate(deviceObjPtr);

    /* init specific functions array */
    smemTigerInitFuncArray(devMemInfoPtr);

    /* allocate address type specific memories */
    smemTigerAllocSpecMemory(deviceObjPtr,devMemInfoPtr);

    /* Clear AU structure */
    smemTigerResetAuq(devMemInfoPtr);

    /* Set PCI registers default values */
    smemTigerSetPciDefault(deviceObjPtr, devMemInfoPtr);

    /* init PHY registers memory */
    smemTigerPhyRegsInit(deviceObjPtr, devMemInfoPtr);

    deviceObjPtr->devFindMemFunPtr = (void *)smemTigerFindMem;

    /* init FA registers - temporary patch */
    smemFaMemory[(0xAC/4)] = 0x11AB;

}
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
)
{
    void                * memPtr;
    TIGER_DEV_MEM_INFO  * devMemInfoPtr;
    GT_32               index;
    GT_U32              param;
    GT_32               dramType;

    if (deviceObjPtr == 0)
    {
        skernelFatalError("smemTigerFindMem: illegal pointer \n");
    }
    if (activeMemPtrPtr != NULL)
    {
        *activeMemPtrPtr = NULL;
    }

    memPtr = 0;
    devMemInfoPtr = (TIGER_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;
    /* Find PCI registers memory  */
    if (SMEM_ACCESS_PCI_FULL_MAC(accessType))
    {
        memPtr = smemTigerPciReg(deviceObjPtr, address, memSize, 0);

        /* find PCI active memory entry */
        if (activeMemPtrPtr != NULL)
        {
            for (index = 0; index < (SMEM_ACTIVE_PCI_MEM_TABLE_SIZE - 1);
                  index++)
            {
                /* check address */
                if ((address & smemTigerPciActiveTable[index].mask)
                      == smemTigerPciActiveTable[index].address)
                    *activeMemPtrPtr = &smemTigerPciActiveTable[index];
            }
        }
        return memPtr;
    }

    if ((address & 0x40000000) > 0 )
    {
        /* FA memory */
        memPtr = smemTigerFaMem(deviceObjPtr, address, memSize, 0);
        return memPtr;
    }
    else if ( (address & 0x20000000) )
    {
        /* Find external memory */
        dramType = ((address >> 27) & 0x07);

        memPtr = smemTigerExternalMem(deviceObjPtr, address, memSize, dramType);
    }
    else {
        /* Find common registers memory */
        index = (address & REG_SPEC_FUNC_INDEX) >> 23;
        if (index > 63)
        {
            skernelFatalError("smemTigerFindMem: index is out of range\n");
        }
        /* Call register spec function to obtain pointer to register memory */
        param   = devMemInfoPtr->specFunTbl[index].specParam;
        memPtr  = devMemInfoPtr->specFunTbl[index].specFun(deviceObjPtr,
                                                           accessType,
                                                           address,
                                                           memSize,
                                                           param);
    }
    /* find active memory entry */
    if (activeMemPtrPtr != NULL)
    {
        for (index = 0; index < (SMEM_ACTIVE_MEM_TABLE_SIZE - 1); index++)
        {
            /* check address */
            if ((address & smemTigerActiveTable[index].mask)
                  == smemTigerActiveTable[index].address)
                *activeMemPtrPtr = &smemTigerActiveTable[index];
        }
    }

    return memPtr;
}

/*******************************************************************************
*   smemTigerGlobalReg
*
* DESCRIPTION:
*       Global, TWSI, GPP & CPU Port Configuration Registers.
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
static GT_U32 *  smemTigerGlobalReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    TIGER_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32              * regValPtr;
    GT_U32              index;

    regValPtr = 0;
    devMemInfoPtr = (TIGER_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;
    /* Global registers */
    if ((address & 0x1FFFFF00) == 0x0){
        index = (address & 0xFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->globalMem.globRegs,
                         devMemInfoPtr->globalMem.globRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->globalMem.globRegs[index];
    }else
     /* TWSI registers */
    if ((address & 0x1FFFFF00) == 0x00400000) {
        index = (address & 0x18) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->globalMem.twsiIntRegs,
                         devMemInfoPtr->globalMem.twsiIntRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->globalMem.twsiIntRegs[index];
    }
     /* SDMA registers */
    if ((address & 0x1FFFF000) == 0x00002000) {
        index = (address & 0x8FF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->globalMem.sdmaRegs,
                         devMemInfoPtr->globalMem.sdmaRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->globalMem.sdmaRegs[index];
    }

    return regValPtr;
}
/*******************************************************************************
*   smemTigerTransQueReg
*
* DESCRIPTION:
*       Egress, Transmit (Tx) Queue and VLAN Table (VLT) Configuration Registers
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
static GT_U32 *  smemTigerTransQueReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    TIGER_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32              * regValPtr;
    GT_U32              port, offset, index;

    regValPtr = 0;
    devMemInfoPtr = (TIGER_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;

    /* Port Traffic Class Descriptor Limit Register */
    if ((address & 0x1E7F0FFF) >= 0x200 &&
        (address & 0x1E7F0FFF) < 0x280){
        port = ((address >> 12) & 0xFF);
        offset =  ((address >> 4) & 0xF);
        index = port + offset;
        CHECK_MEM_BOUNDS(devMemInfoPtr->egressMem.tcRegs,
                         devMemInfoPtr->egressMem.tcRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->egressMem.tcRegs[index];
    }
    else
    /* Port Transmit Configuration Register */
    if ((address & 0x1E7F0FFF) == 0x0){
        index = ((address >> 12) & 0xFF);
        CHECK_MEM_BOUNDS(devMemInfoPtr->egressMem.transConfRegs,
                         devMemInfoPtr->egressMem.transConfRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->egressMem.transConfRegs[index];
    }
    else
    /* Port Descriptor Limit Register */
    if ((address & 0x1E7F0FFF) == 0x100){
        index = ((address >> 12) & 0xFF);
        CHECK_MEM_BOUNDS(devMemInfoPtr->egressMem.descLimRegs,
                         devMemInfoPtr->egressMem.descLimRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->egressMem.descLimRegs[index];
    }
    else
    /*
        --- 32 registers for non-trunk low bmp
        Table 378: Non-Trunk Members n Ports 0–31 Register
        Offset: Trunk1: 0x01801280, Trunk2: 0x01802280..., Trunk31: 0x0181F280
        Offset Formula: 0x01801280 + Non Trunk n * 0x1000 (where n represents 1–31)

        --- 32 registers for non-trunk high bmp
        Table 379: Non-Trunk Members n Ports 32–52 Register
        Offset: Trunk1: 0x01801284, Trunk2: 0x01802284..., Trunk31: 0x0181F284
        Offset Formula: 0x01801284 + Non Trunk n * 0x1000 (where n represents 1–31)

        --- 16 registers for designated trunk low bmp
        Table 380: Trunk Designated Ports Combination n Ports 0–31 Register
        Offset: Combination0: 0x01820280, Combination1: 0x01821280...,
        Combination15: 0x0182F280
        Offset Formula: 0x01820280 + Combination n * 0x1000 (where n represents 0–15)

        --- 16 registers for designated trunk low bmp
        Table 381: Trunk Designated Ports Combination n Ports 31–52 Register
        Offset: Combination0: 0x01820284, Combination2: 0x01821284...,
        Combination15: 0x0182F284
        Offset Formula: 0x01820284 + Combination n * 0x1000 (where n represents 0–15)


        ================
        total of 96 registers:
        48 = 32 + 16 registers ended with address 0x280
        48 = 32 + 16 registers ended with address 0x284

    */
    if ((address & 0x1E7C0000) == 0x0 && ((address & 0xFFB)==0x280)){/*0x280 or 0x284*/
        index = ((address&0x20000)?1:0)*32 + ((address & 0x1F000)>>12);
        index += (address & 0x4) ? 48 : 0;
        CHECK_MEM_BOUNDS(devMemInfoPtr->egressMem.trunkMemRegs,
                         devMemInfoPtr->egressMem.trunkRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->egressMem.trunkMemRegs[index];
    }
    else
    /* General registers */
    if ((address & 0x1E700000) == 0x0){
        index = (address & 0x1FFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->egressMem.genRegs,
                         devMemInfoPtr->egressMem.genRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->egressMem.genRegs[index];
    }
    else
    /* External Memory configuration register */
    if ((address & 0x11900000) == 0x01900000){
        index = ((address & 0x294) >> 4) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->egressMem.extMemRegs,
                         devMemInfoPtr->egressMem.extMemRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->egressMem.extMemRegs[index];
    }
    else
    /* Control memory and MLL memory */
    if ((address & 0x07ff0000) == 0x000c0000){
        index = (address & 0xFFFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->egressMem.mllTableRegs,
                         devMemInfoPtr->egressMem.mllTableRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->egressMem.mllTableRegs[index];
    }

    return regValPtr;
}

/*******************************************************************************
*   smemTigerEtherBrdgReg
*
* DESCRIPTION:
*       Bridge Configuration Registers
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
static GT_U32 *  smemTigerEtherBrdgReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    TIGER_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32              * regValPtr;
    GT_U32              index, port;

    regValPtr = 0;
    devMemInfoPtr = (TIGER_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;
    /* Port Bridge control registers */
    if ((address & 0x1DFC0800) == 0x0){
        port = (address & 0x3F000) >> 12;
        index = ((address & 0x1F) / 0x4 * BRG_PORTS_NUM) + port;
        CHECK_MEM_BOUNDS(devMemInfoPtr->bridgeMngMem.portRegs,
                         devMemInfoPtr->bridgeMngMem.portRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->bridgeMngMem.portRegs[index];
    }
    else
    /* Ports Protocol Based VLANs configuration Registers */
    if ((address & 0x1DFC0F00) == 0x00000800) {
        port = (address & 0x3F000) >> 12;
        index = (((address & 0x1F) / 0x4) * BRG_PORTS_NUM) + port;
        CHECK_MEM_BOUNDS(devMemInfoPtr->bridgeMngMem.portProtVidRegs,
                         devMemInfoPtr->bridgeMngMem.portProtVidRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->bridgeMngMem.portProtVidRegs[index];
    }
    else
    /* General Bridge managment registers array */
    if ((address & 0x1DFFF000) == 0x00040000) {
        index = (address & 0x3FF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->bridgeMngMem.genRegs,
                         devMemInfoPtr->bridgeMngMem.genRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->bridgeMngMem.genRegs[index];
    }
    return regValPtr;
}

/*******************************************************************************
*   smemTigerLxReg
*
* DESCRIPTION:
*       IPv4 Processor Registers
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
static GT_U32 *  smemTigerLxReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    TIGER_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32              * regValPtr = 0;
    GT_U32              index;

    devMemInfoPtr = (TIGER_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;
    /* General LX unit registers */
    if ((address & 0x007f0000) == 0x00000000){ /*0x0280....*/
        index = (address & 0x3FF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->lxMem.genRegs,
                         devMemInfoPtr->lxMem.genRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->lxMem.genRegs[index];
    }
    else
    /* IP Flow Classifier Table Registers */
    if ((address & 0x007f0000) == 0x00400000){ /*0x02c0....*/
        index = (address & 0x1f) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->lxMem.ipFlowRegs,
                         devMemInfoPtr->lxMem.ipFlowRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->lxMem.ipFlowRegs[index];
    }
    else
    /* Flow template hush configuration registers */
    if ((address & 0x007f0000) == 0x00420000){ /*0x02c2....*/
        index = (address & 0xffff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->lxMem.ftHushRegs,
                         devMemInfoPtr->lxMem.ftHushRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->lxMem.ftHushRegs[index];
    }
    else
    /*  User Defined Bytes Configuration Table Registers */
    if ((address & 0x007f0000) == 0x00440000){ /*0x02c4....*/
        index = (address & 0x2ff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->lxMem.userDefinedRegs,
                         devMemInfoPtr->lxMem.userDefinedRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->lxMem.userDefinedRegs[index];
    }
    else
    /* Per-Port InLIF Table */
    if ((address & 0x007f0000) == 0x00460000){ /*0x02c6....*/
        index = (address & 0x3ff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->lxMem.inLifPerPortTblRegs,
                         devMemInfoPtr->lxMem.inLifPerPortTblRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->lxMem.inLifPerPortTblRegs[index];
    }
    else
    /* IP Flow Action Table Address Locator */
    if ((address & 0x007f0000) == 0x00480000){ /*0x02c8....*/
        index = (address & 0x3ff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->lxMem.ipFlowActTblRegs,
                         devMemInfoPtr->lxMem.ipFlowActTblRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->lxMem.ipFlowActTblRegs[index];
    }
    else
    /* DSCP to CoS Marking Table and EXP to CoS Marking Table */
    if ((address & 0x007f0000) == 0x004a0000){ /*0x02ca....*/
        index = (address & 0x1ff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->lxMem.dscp2CoSRegs,
                         devMemInfoPtr->lxMem.dscp2CoSRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->lxMem.dscp2CoSRegs[index];
    }
    else
    /* Ingress Policing Management registers */
    if ((address & 0x007f0000) == 0x004c0000){ /*0x02cc....*/
        index = (address & 0x1ff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->lxMem.ingrPolicyMngRegs,
                         devMemInfoPtr->lxMem.ingrPolicyMngRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->lxMem.ingrPolicyMngRegs[index];
    }
    else
    /* IPv4/IPv6 Routing Engine Registers */
    if ((address & 0x007f0000) == 0x004e0000){ /*0x02ce....*/
        index = (address & 0x3ff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->lxMem.ipv4MngRegs,
                         devMemInfoPtr->lxMem.ipv4MngRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->lxMem.ipv4MngRegs[index];
    }
    else
    /* Ingress Policing Re-marking Tables */
    if ((address & 0x007f0000) == 0x00560000){ /*0x02d6....*/
        index = (address & 0x3ff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->lxMem.ingrPolicyRemTblRegs,
                         devMemInfoPtr->lxMem.ingrPolicyRemTblRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->lxMem.ingrPolicyRemTblRegs[index];
    }
    else
    /* Input Logical Interface Per Vlan Table */
    if ((address & 0x007f0000) == 0x00580000){ /*0x02d8....*/
        index = (address & 0x3ff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->lxMem.inLifPerVlanTblRegs,
                         devMemInfoPtr->lxMem.inLifPerVlanTblRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->lxMem.inLifPerVlanTblRegs[index];
    }
    else
    /* SRAM Interface Configuration Registers */
    if ((address & 0x007f0000) == 0x00600000){ /*0x02e0....*/
        index = (address & 0x1ff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->lxMem.sramRegs,
                         devMemInfoPtr->lxMem.sramRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->lxMem.sramRegs[index];
    }
    else
    /* TCAM0 Access Registers */
    if ((address & 0x007f0000) == 0x00620000){ /*0x02e2....*/
        index = (address & 0xff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->lxMem.tcam0MaskAccRegs,
                         devMemInfoPtr->lxMem.tcam0MaskAccRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->lxMem.tcam0MaskAccRegs[index];
    }
    else
    /* TCAM1 Access Registers */
    if ((address & 0x007f0000) == 0x00640000){ /*0x02e4....*/
        index = (address  & 0xff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->lxMem.tcam1MaskAccRegs,
                         devMemInfoPtr->lxMem.tcam1MaskAccRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->lxMem.tcam1MaskAccRegs[index];
    }
    else
    /* TCAM Write Access Registers */
    if ((address & 0x007f0000) == 0x00630000){ /*0x02e3....*/
        index = (address & 0xff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->lxMem.tcam0ValueAccRegs,
                         devMemInfoPtr->lxMem.tcam0ValueAccRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->lxMem.tcam0ValueAccRegs[index];
    }
    else
    /* TCAM Write Access Registers */
    if ((address & 0x007f0000) == 0x00650000){ /*0x02e5....*/
        index = (address & 0xff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->lxMem.tcam1ValueAccRegs,
                         devMemInfoPtr->lxMem.tcam1ValueAccRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->lxMem.tcam1ValueAccRegs[index];
    }
    return regValPtr;
}

/*******************************************************************************
*   smemTigerTcamTableReg
*
* DESCRIPTION:
*       TCAM table memory
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
static GT_U32 *  smemTigerTcamTableReg
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    TIGER_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32              * regValPtr = 0;
    GT_U32              index;

    devMemInfoPtr = (TIGER_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;
    /* TCAM0 Mask Memory */
    if ((address & 0x0fff0000) == 0x0a820000){ /*0x02e2....(access) */
        index = (address & 0xffff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->tcamMem.tcam0MaskRegs,
                         devMemInfoPtr->tcamMem.tcam0MaskRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->tcamMem.tcam0MaskRegs[index];
    }
    else
    /* TCAM1 Mask Memory */
    if ((address & 0x0fff0000) == 0x0a840000){ /*0x02e4....(access) */
        index = (address  & 0xffff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->tcamMem.tcam1MaskRegs,
                         devMemInfoPtr->tcamMem.tcam1MaskRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->tcamMem.tcam1MaskRegs[index];
    }
    else
    /* TCAM0 Pattern Memory */
    if ((address & 0x0fff0000) == 0x0a830000){ /*0x02e3....(access) */
        index = (address & 0xffff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->tcamMem.tcam0ValueRegs,
                         devMemInfoPtr->tcamMem.tcam0ValueRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->tcamMem.tcam0ValueRegs[index];
    }
    else
    /* TCAM1 Pattern Memory */
    if ((address & 0x0fff0000) == 0x0a850000){ /*0x02e5....(access) */
        index = (address & 0xffff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->tcamMem.tcam1ValueRegs,
                         devMemInfoPtr->tcamMem.tcam1ValueRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->tcamMem.tcam1ValueRegs[index];
    }
    return regValPtr;
}
/*******************************************************************************
*   smemTigerBufMngReg
*
* DESCRIPTION:
*       Describe a device's buffer managment registers memory object
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
static GT_U32 *  smemTigerBufMngReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    TIGER_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32              * regValPtr;
    GT_U32              index;

    regValPtr = 0;
    devMemInfoPtr = (TIGER_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;
    /* Control Linked List buffers managment registers array */
    if ((address & 0x1CFF0000) == 0x00010000) {
        index = (address & 0xFFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->bufMngMem.bmRegs,
                         devMemInfoPtr->bufMngMem.bmRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->bufMngMem.bmRegs[index];
    }
    else
    /* Control Linked List buffers managment registers array */
    if ((address & 0x1CFF0000) == 0x00020000) {
        index = (address & 0xFFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->bufMngMem.epfRegs,
                         devMemInfoPtr->bufMngMem.epfRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->bufMngMem.epfRegs[index];
    }
    else
    /* CPU Trap/Mirror QoS Attributes Register */
    if ((address & 0x007ff000) == 0x00002000) {
        index = (address & 0xfff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->bufMngMem.cpuQoSAttrRegs,
                         devMemInfoPtr->bufMngMem.cpuQoSAttrRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->bufMngMem.cpuQoSAttrRegs[index];
    }
    else
    /* VLAN Memory Configuration Register and EPF-HA Macro DRO Control Register */
    if ((address & 0x007ff000) == 0x00034000) {
        index = (address & 0x7c) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->bufMngMem.vlanMemConfRegs,
                         devMemInfoPtr->bufMngMem.vlanMemConfRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->bufMngMem.vlanMemConfRegs[index];
    }
    else
    /* Control Linked List buffers managment registers array */
    if ((address & 0x007ff000) == 0x00004000) {
        index = (address & 0xFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->bufMngMem.egrRegs,
                         devMemInfoPtr->bufMngMem.egrRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->bufMngMem.egrRegs[index];
    }
    /* Trunk Member Table Configuration Registers */
    else if((address & 0x03fff000) == 0x03000000)
    {
        index = (address & 0x00000fff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->bufMngMem.trunkConfRegs,
                         devMemInfoPtr->bufMngMem.trunkConfRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->bufMngMem.trunkConfRegs[index];
    }
    /* Multicast Buffer Counter Register */
    else if((address & 0x007f0000) == 0x00040000)
    {
        index = (address & 0x000002fff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->bufMngMem.mcBufCountRegs,
                         devMemInfoPtr->bufMngMem.mcBufCountRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->bufMngMem.mcBufCountRegs[index];
    }

    return regValPtr;
}

/*******************************************************************************
*   smemTigerPciReg
*
* DESCRIPTION:
*       PCI memory access
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
static GT_U32 *  smemTigerPciReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    TIGER_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32              * regValPtr;
    GT_U32              index;

    regValPtr = 0;
    devMemInfoPtr = (TIGER_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;
    /* Control Linked List buffers managment registers array */
    if ((address & 0xFFFFF00) == 0x00000000) {
        index = (address & 0x3C) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->pciMem.pciRegsRegs,
                         devMemInfoPtr->pciMem.pciRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->pciMem.pciRegsRegs[index];
    }
    else
    /* Prestera specific PCI registers */
    if ((address & 0xFFFFFF00) == 0x00000100) {
        index = (address & 0x18) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->pciMem.pciInterRegs,
                         devMemInfoPtr->pciMem.pciInterRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->pciMem.pciInterRegs[index];
    }

    return regValPtr;
}
/*******************************************************************************
*   smemTigerMacReg
*
* DESCRIPTION:
*       Describe a MAC registers
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
static GT_U32 *  smemTigerMacReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    TIGER_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32              * regValPtr;
    GT_U32              index;

    regValPtr = 0;
    devMemInfoPtr = (TIGER_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;
    /* Control Linked List buffers managment registers array */
    if ((address & 0x1C7FFF00) == 0x00000000) {
        index = (address & 0xff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->macMem.genRegs,
                         devMemInfoPtr->macMem.genRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->macMem.genRegs[index];
    }
    else
    if ((address & 0x1C700100) == 0x00000100) {
        index = (address & 0xff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->macMem.sflowRegs,
                         devMemInfoPtr->macMem.sflowRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->macMem.sflowRegs[index];
    }

    return regValPtr;
}
/*******************************************************************************
*   smemTigerPortGroupConfReg
*
* DESCRIPTION:
*       Describe a device's Port registers memory object
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
static GT_U32 *  smemTigerPortGroupConfReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    TIGER_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32              * regValPtr;
    GT_U32              index;

    regValPtr = 0;
    devMemInfoPtr = (TIGER_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;
    /* Per port MAC Control Register */
    if ((address & 0x1E7F400F) == 0x00000000){
        index  = 24 * param + ((address & 0xFF00) >> 8);
        CHECK_MEM_BOUNDS(devMemInfoPtr->portMem.perPortCtrlRegs,
                         devMemInfoPtr->portMem.perPortCtrlRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->portMem.perPortCtrlRegs[index];
    }
    else
    /* Per port MAC Status Register */
    if ((address & 0x1E7F400F) == 0x00000004){
        index  = 24 * param + ((address & 0xFF00) >> 8);
        CHECK_MEM_BOUNDS(devMemInfoPtr->portMem.perPortStatRegs,
                         devMemInfoPtr->portMem.perPortStatRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->portMem.perPortStatRegs[index];
    }
    else
    /* Per group registres */
    if ((address & 0x1E7F4000) == 0x00004000) {
        index = param * (PER_GROUPS_TYPE_NUM / 2) + (address & 0xFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->portMem.perGroupRegs,
                         devMemInfoPtr->portMem.perGroupRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->portMem.perGroupRegs[index];
    }
    else
    /* MAC counters array */
    if ((address & 0x1E010000) == 0x00010000) {
        index = (address & 0xFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->portMem.macCnts,
                         devMemInfoPtr->portMem.macCntsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->portMem.macCnts[index];
    }
    return regValPtr;
}

/*******************************************************************************
*   smemTigerMacTableReg
*
* DESCRIPTION:
*       Describe a device's Bridge registers and FDB
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
static GT_U32 *  smemTigerMacTableReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    TIGER_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32              * regValPtr;
    GT_U32              index;

    regValPtr = 0;
    devMemInfoPtr = (TIGER_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;
    /* Registers, index is group number */
    if ((address & 0x16000000) == 0x06000000){
        index = (address & 0xFFFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->macFbdMem.macTblRegs,
                         devMemInfoPtr->macFbdMem.macTblRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->macFbdMem.macTblRegs[index];
    }
    return regValPtr;
}


/*******************************************************************************
*   smemTigerVlanTableReg
*
* DESCRIPTION:
*       Describe a device's Bridge registers and FDB
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
static GT_U32 *  smemTigerVlanTableReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    TIGER_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32              * regValPtr;
    GT_U32              index;

    regValPtr = 0;
    devMemInfoPtr = (TIGER_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;
    /* Vlan Table Registers */
    index = (address & 0xFFFFF) / 0x4;
    CHECK_MEM_BOUNDS(devMemInfoPtr->vlanTblMem.vlanTblRegs,
                     devMemInfoPtr->vlanTblMem.vlanTblRegNum,
                     index, memSize);
    regValPtr = &devMemInfoPtr->vlanTblMem.vlanTblRegs[index];


    return regValPtr;
}

/*******************************************************************************
*   smemTigerWsramTableReg
*
* DESCRIPTION:
*       Describe a device's Wide SRAM memory
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
static GT_U32 *  smemTigerWsramTableReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    TIGER_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32              * regValPtr;
    GT_U32              index;

    regValPtr = 0;
    devMemInfoPtr = (TIGER_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;
    /* Wide sram memory access */
    index = (address & 0xFFFFF) / 0x4;
    CHECK_MEM_BOUNDS(devMemInfoPtr->wsramMem.wSram,
                     devMemInfoPtr->wsramMem.wSramSize,
                     index, memSize);
    regValPtr = &devMemInfoPtr->wsramMem.wSram[index];


    return regValPtr;
}

static GT_U32 *  smemTigerFatalError(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    skernelFatalError("smemTigerFatalError: illegal function pointer\n");

    return 0;
}
/*******************************************************************************
*   smemTigerInitMemArray
*
* DESCRIPTION:
*       Init specific functions array.
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
static void smemTigerInitFuncArray(
    INOUT TIGER_DEV_MEM_INFO  * devMemInfoPtr
)
{
    GT_U32              i;

    for (i = 0; i < 64; i++) {
        devMemInfoPtr->specFunTbl[i].specFun    = smemTigerFatalError;
    }
    devMemInfoPtr->specFunTbl[0].specFun        = smemTigerGlobalReg;

    devMemInfoPtr->specFunTbl[1].specFun        = smemTigerPortGroupConfReg;
    devMemInfoPtr->specFunTbl[1].specParam      = 0;

    devMemInfoPtr->specFunTbl[2].specFun        = smemTigerPortGroupConfReg;
    devMemInfoPtr->specFunTbl[2].specParam      = 1;

    devMemInfoPtr->specFunTbl[3].specFun        = smemTigerTransQueReg;

    devMemInfoPtr->specFunTbl[4].specFun        = smemTigerEtherBrdgReg;

    devMemInfoPtr->specFunTbl[5].specFun        = smemTigerLxReg;

    devMemInfoPtr->specFunTbl[6].specFun        = smemTigerBufMngReg;

    devMemInfoPtr->specFunTbl[7].specFun        = smemTigerMacReg;

    devMemInfoPtr->specFunTbl[12].specFun       = smemTigerMacTableReg;

    devMemInfoPtr->specFunTbl[20].specFun       = smemTigerVlanTableReg;

    devMemInfoPtr->specFunTbl[21].specFun       = smemTigerTcamTableReg;

    devMemInfoPtr->specFunTbl[24].specFun       = smemTigerWsramTableReg;
}
/*******************************************************************************
*   smemTigerAllocSpecMemory
*
* DESCRIPTION:
*       Allocate address type specific memories.
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
static void smemTigerAllocSpecMemory(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    INOUT TIGER_DEV_MEM_INFO  * devMemInfoPtr
)
{
    int i;                      /* index of memory array */

    /* Global register memory allocation */
    devMemInfoPtr->globalMem.globRegsNum = GLOB_REGS_NUM;
    devMemInfoPtr->globalMem.globRegs =
            calloc(GLOB_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->globalMem.globRegs == 0)
    {
        skernelFatalError("smemTigerAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->globalMem.twsiIntRegsNum = TWSI_INT_REGS_NUM;
    devMemInfoPtr->globalMem.twsiIntRegs =
        calloc(TWSI_INT_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->globalMem.twsiIntRegs == 0)
    {
        skernelFatalError("smemTigerAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->globalMem.sdmaRegsNum = SDMA_REGS_NUM;
    devMemInfoPtr->globalMem.sdmaRegs =
        calloc(SDMA_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->globalMem.sdmaRegs == 0)
    {
        skernelFatalError("smemTigerAllocSpecMemory: allocation error\n");
    }

    /* Buffer management register memory allocation */
    devMemInfoPtr->bufMngMem.bmRegsNum = BM_REGS_NUM;
    devMemInfoPtr->bufMngMem.bmRegs =
        calloc(BM_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->bufMngMem.bmRegs == 0)
    {
        skernelFatalError("smemTigerAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->bufMngMem.epfRegsNum = EPF_REGS_NUM;
    devMemInfoPtr->bufMngMem.epfRegs =
        calloc(EPF_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->bufMngMem.epfRegs == 0)
    {
        skernelFatalError("smemTigerAllocSpecMemory: allocation error\n");
    }
    /* CPU Trap/Mirror QoS Attributes Register */
    devMemInfoPtr->bufMngMem.cpuQoSAttrRegsNum = CPU_QOS_ATTR_REGS_NUM;
    devMemInfoPtr->bufMngMem.cpuQoSAttrRegs =
        calloc(CPU_QOS_ATTR_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->bufMngMem.cpuQoSAttrRegs == 0)
    {
        skernelFatalError("smemTigerAllocSpecMemory: allocation error\n");
    }

    /* VLAN Memory Configuration Register and EPF-HA Macro DRO Control Register */
    devMemInfoPtr->bufMngMem.vlanMemConfRegsNum = VLAN_CONF_MEM_REGS_NUM;
    devMemInfoPtr->bufMngMem.vlanMemConfRegs =
        calloc(VLAN_CONF_MEM_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->bufMngMem.vlanMemConfRegs == 0)
    {
        skernelFatalError("smemTigerAllocSpecMemory: allocation error\n");
    }

    /* Egress Mirrored Ports registers memory allocation */
    devMemInfoPtr->bufMngMem.egrRegsNum = EGR_REGS_NUM;
    devMemInfoPtr->bufMngMem.egrRegs =
        calloc(EGR_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->bufMngMem.egrRegs == 0)
    {
        skernelFatalError("smemTigerAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->bufMngMem.trunkConfRegsNum = TRUNK_TBL_REGS_NUM;
    devMemInfoPtr->bufMngMem.trunkConfRegs =
        calloc(TRUNK_TBL_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->bufMngMem.trunkConfRegs == 0)
    {
        skernelFatalError("smemTigerAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->bufMngMem.mcBufCountRegsNum = MC_BUF_COUNT_REGS_NUM;
    devMemInfoPtr->bufMngMem.mcBufCountRegs =
        calloc(MC_BUF_COUNT_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->bufMngMem.mcBufCountRegs == 0)
    {
        skernelFatalError("smemTigerAllocSpecMemory: allocation error\n");
    }

    /*  IPV4 General registers memory allocation */
    devMemInfoPtr->lxMem.genRegsNum = LX_GEN_REGS_NUM;
    devMemInfoPtr->lxMem.genRegs =
        calloc(LX_GEN_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->lxMem.genRegs == 0)
    {
        skernelFatalError("smemTigerAllocSpecMemory: allocation error\n");
    }
    /* IP Flow Classifier Table Registers */
    devMemInfoPtr->lxMem.ipFlowRegsNum = IP_FLOW_CLASS_REGS_NUM;
    devMemInfoPtr->lxMem.ipFlowRegs =
        calloc(IP_FLOW_CLASS_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->lxMem.ipFlowRegs == 0)
    {
        skernelFatalError("smemTigerAllocSpecMemory: allocation error\n");
    }

    /* Flow template hush configuration registers */
    devMemInfoPtr->lxMem.ftHushRegsNum = FLOW_TEMPLATE_CNF_REGS_NUM;
    devMemInfoPtr->lxMem.ftHushRegs =
        calloc(FLOW_TEMPLATE_CNF_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->lxMem.ftHushRegs == 0)
    {
        skernelFatalError("smemTigerAllocSpecMemory: allocation error\n");
    }

    /* User Defined Bytes Configuration Table Registers */
    devMemInfoPtr->lxMem.userDefinedRegsNum = USER_DEF_TBL_REGS_NUM;
    devMemInfoPtr->lxMem.userDefinedRegs =
        calloc(USER_DEF_TBL_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->lxMem.userDefinedRegs == 0)
    {
        skernelFatalError("smemTigerAllocSpecMemory: allocation error\n");
    }
    /* User Defined Bytes Configuration Table Registers */
    devMemInfoPtr->lxMem.inLifPerPortTblRegsNum = INLIF_PER_PORT_TBL_REGS_NUM;
    devMemInfoPtr->lxMem.inLifPerPortTblRegs =
        calloc(INLIF_PER_PORT_TBL_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->lxMem.inLifPerPortTblRegs == 0)
    {
        skernelFatalError("smemTigerAllocSpecMemory: allocation error\n");
    }
    /* IP Flow Action Table Address Locator */
    devMemInfoPtr->lxMem.ipFlowActTblRegsNum = IP_FLOW_ACTION_REGS_NUM;
    devMemInfoPtr->lxMem.ipFlowActTblRegs =
        calloc(IP_FLOW_ACTION_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->lxMem.ipFlowActTblRegs == 0)
    {
        skernelFatalError("smemTigerAllocSpecMemory: allocation error\n");
    }
    /* DSCP registers memory allocation */
    devMemInfoPtr->lxMem.dscp2CoSRegsNum = DSCP_REGS_NUM;
    devMemInfoPtr->lxMem.dscp2CoSRegs =
        calloc(DSCP_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->lxMem.dscp2CoSRegs == 0)
    {
        skernelFatalError("smemTigerAllocSpecMemory: allocation error\n");
    }
    /* Ingress Policing Management registers */
    devMemInfoPtr->lxMem.ingrPolicyMngRegsNum = INGR_POLICY_MNG_REGS_NUM;
    devMemInfoPtr->lxMem.ingrPolicyMngRegs =
        calloc(INGR_POLICY_MNG_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->lxMem.ingrPolicyMngRegs == 0)
    {
        skernelFatalError("smemTigerAllocSpecMemory: allocation error\n");
    }
    /* IPv4/IPv6 Routing Engine Registers */
    devMemInfoPtr->lxMem.ipv4MngRegsNum = IPV4_IPV6_MNG_REGS_NUM;
    devMemInfoPtr->lxMem.ipv4MngRegs =
        calloc(IPV4_IPV6_MNG_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->lxMem.ipv4MngRegs == 0)
    {
        skernelFatalError("smemTigerAllocSpecMemory: allocation error\n");
    }
    /* Ingress Policing Re-marking Tables */
    devMemInfoPtr->lxMem.ingrPolicyRemTblRegsNum = INGR_POL_REM_TBL_REGS_NUM;
    devMemInfoPtr->lxMem.ingrPolicyRemTblRegs =
        calloc(INGR_POL_REM_TBL_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->lxMem.ingrPolicyRemTblRegs == 0)
    {
        skernelFatalError("smemTigerAllocSpecMemory: allocation error\n");
    }
    /* Input Logical Interface Per Vlan Table */
    devMemInfoPtr->lxMem.inLifPerVlanTblRegsNum = INLIF_PER_VLAN_TBL_REGS_NUM;
    devMemInfoPtr->lxMem.inLifPerVlanTblRegs =
        calloc(INLIF_PER_VLAN_TBL_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->lxMem.inLifPerVlanTblRegs == 0)
    {
        skernelFatalError("smemTigerAllocSpecMemory: allocation error\n");
    }
    /* SRAM Interface Configuration Registers */
    devMemInfoPtr->lxMem.sramRegsNum = SRAM_INTER_REGS_NUM;
    devMemInfoPtr->lxMem.sramRegs =
        calloc(SRAM_INTER_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->lxMem.sramRegs == 0)
    {
        skernelFatalError("smemTigerAllocSpecMemory: allocation error\n");
    }
    /* TCAM0 Access Registers */
    devMemInfoPtr->lxMem.tcam0MaskAccRegsNum = TCAM_ACCESS_REGS_NUM;
    devMemInfoPtr->lxMem.tcam0MaskAccRegs =
        calloc(TCAM_ACCESS_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->lxMem.tcam0MaskAccRegs == 0)
    {
        skernelFatalError("smemTigerAllocSpecMemory: allocation error\n");
    }
    /* TCAM1 Access Registers */
    devMemInfoPtr->lxMem.tcam1MaskAccRegsNum = TCAM_ACCESS_REGS_NUM;
    devMemInfoPtr->lxMem.tcam1MaskAccRegs =
        calloc(TCAM_ACCESS_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->lxMem.tcam1MaskAccRegs == 0)
    {
        skernelFatalError("smemTigerAllocSpecMemory: allocation error\n");
    }

    /* TCAM Write Access Registers */
    devMemInfoPtr->lxMem.tcam0ValueAccRegsNum = TCAM_ACCESS_REGS_NUM;
    devMemInfoPtr->lxMem.tcam0ValueAccRegs =
        calloc(TCAM_ACCESS_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->lxMem.tcam0ValueAccRegs == 0)
    {
        skernelFatalError("smemTigerAllocSpecMemory: allocation error\n");
    }

    /* TCAM Write Access Registers */
    devMemInfoPtr->lxMem.tcam1ValueAccRegsNum = TCAM_ACCESS_REGS_NUM;
    devMemInfoPtr->lxMem.tcam1ValueAccRegs =
        calloc(TCAM_ACCESS_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->lxMem.tcam1ValueAccRegs == 0)
    {
        skernelFatalError("smemTigerAllocSpecMemory: allocation error\n");
    }

    /* TCAM access address */
    devMemInfoPtr->lxMem.tcamAccessAddr = SMEM_TIGER_NOT_VALID_ADDRESS;

    /* TCAM0 mask memory */
    devMemInfoPtr->tcamMem.tcam0MaskRegsNum = TCAM_REGS_NUM;
    devMemInfoPtr->tcamMem.tcam0MaskRegs =
        calloc(TCAM_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->tcamMem.tcam0MaskRegs == 0)
    {
        skernelFatalError("smemTigerAllocSpecMemory: allocation error\n");
    }
    /* TCAM1 mask memory */
    devMemInfoPtr->tcamMem.tcam1MaskRegsNum = TCAM_REGS_NUM;
    devMemInfoPtr->tcamMem.tcam1MaskRegs =
        calloc(TCAM_ACCESS_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->tcamMem.tcam1MaskRegs == 0)
    {
        skernelFatalError("smemTigerAllocSpecMemory: allocation error\n");
    }

    /* TCAM0 pattern memory */
    devMemInfoPtr->tcamMem.tcam0ValueRegsNum = TCAM_REGS_NUM;
    devMemInfoPtr->tcamMem.tcam0ValueRegs =
        calloc(TCAM_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->tcamMem.tcam0ValueRegs == 0)
    {
        skernelFatalError("smemTigerAllocSpecMemory: allocation error\n");
    }

    /* TCAM1 pattern memory */
    devMemInfoPtr->tcamMem.tcam1ValueRegsNum = TCAM_REGS_NUM;
    devMemInfoPtr->tcamMem.tcam1ValueRegs =
        calloc(TCAM_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->tcamMem.tcam1ValueRegs == 0)
    {
        skernelFatalError("smemTigerAllocSpecMemory: allocation error\n");
    }

    /* Serial management interface registers memory allocation */
    devMemInfoPtr->macMem.genRegsNum = MAC_REGS_NUM;
    devMemInfoPtr->macMem.genRegs =
        calloc(MAC_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->macMem.genRegs == 0)
    {
        skernelFatalError("smemTigerAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->macMem.sflowRegsNum = SFLOW_NUM;
    devMemInfoPtr->macMem.sflowRegs = calloc(SFLOW_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->macMem.sflowRegs == 0)
    {
        skernelFatalError("smemTwistAllocSpecMemory: allocation error\n");
    }
    memset(devMemInfoPtr->macMem.sflowFifo, 0,
             TIGER_SFLOW_FIFO_SIZE * sizeof(GT_U32));

    /* Network ports register memory allocation */
    devMemInfoPtr->portMem.perPortCtrlRegsNum = MAC_PORTS_NUM;
    devMemInfoPtr->portMem.perPortCtrlRegs =
        calloc(MAC_PORTS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->portMem.perPortCtrlRegs == 0)
    {
        skernelFatalError("smemTigerAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->portMem.perPortStatRegsNum = MAC_PORTS_NUM;
    devMemInfoPtr->portMem.perPortStatRegs =
        calloc(MAC_PORTS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->portMem.perPortStatRegs == 0)
    {
        skernelFatalError("smemTigerAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->portMem.perGroupRegsNum = PER_GROUPS_TYPE_NUM;
    devMemInfoPtr->portMem.perGroupRegs =
        calloc(MAC_PORTS_NUM * PER_GROUPS_TYPE_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->portMem.perGroupRegs == 0)
    {
        skernelFatalError("smemTigerAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->portMem.macCntsNum = MAC_CNT_TYPES_NUM;
    devMemInfoPtr->portMem.macCnts =
        calloc(MAC_CNT_TYPES_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->portMem.macCnts == 0)
    {
        skernelFatalError("smemTigerAllocSpecMemory: allocation error\n");
    }

    /* Bridge configuration register memory allocation */
    devMemInfoPtr->bridgeMngMem.portRegsNum =
            BRG_PORTS_NUM * PORT_REGS_GROUPS_NUM;
    devMemInfoPtr->bridgeMngMem.portRegs =
        calloc(BRG_PORTS_NUM * PORT_REGS_GROUPS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->bridgeMngMem.portRegs == 0)
    {
        skernelFatalError("smemTigerAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->bridgeMngMem.portProtVidRegsNum =
            BRG_PORTS_NUM * PORT_PROT_VID_NUM;
    devMemInfoPtr->bridgeMngMem.portProtVidRegs =
        calloc(BRG_PORTS_NUM * PORT_PROT_VID_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->bridgeMngMem.portProtVidRegs == 0)
    {
        skernelFatalError("smemTigerAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->bridgeMngMem.genRegsNum = BRG_GEN_REGS_NUM;
    devMemInfoPtr->bridgeMngMem.genRegs =
        calloc(BRG_GEN_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->bridgeMngMem.genRegs == 0)
    {
        skernelFatalError("smemTigerAllocSpecMemory: allocation error\n");
    }

    /* Egress configuration register memory allocation */
    devMemInfoPtr->egressMem.genRegsNum = GEN_EGRS_GROUP_NUM;
    devMemInfoPtr->egressMem.genRegs =
        calloc(GEN_EGRS_GROUP_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->egressMem.genRegs == 0)
    {
        skernelFatalError("smemTigerAllocSpecMemory: allocation error\n");
    }
    /* Port Traffic Class Descriptor Limit Register */
    devMemInfoPtr->egressMem.tcRegsNum = TC_QUE_DESCR_NUM * MAC_PORTS_NUM;
    devMemInfoPtr->egressMem.tcRegs =
        calloc(TC_QUE_DESCR_NUM * MAC_PORTS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->egressMem.tcRegs == 0)
    {
        skernelFatalError("smemTigerAllocSpecMemory: allocation error\n");
    }
    /* Port Transmit Configuration Register */
    devMemInfoPtr->egressMem.transConfRegsNum = MAC_PORTS_NUM;
    devMemInfoPtr->egressMem.transConfRegs =
        calloc(MAC_PORTS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->egressMem.transConfRegs == 0)
    {
        skernelFatalError("smemTigerAllocSpecMemory: allocation error\n");
    }

    /* Port Descriptor Limit Register */
    devMemInfoPtr->egressMem.descLimRegsNum = MAC_PORTS_NUM;
    devMemInfoPtr->egressMem.descLimRegs =
        calloc(MAC_PORTS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->egressMem.descLimRegs == 0)
    {
        skernelFatalError("smemTigerAllocSpecMemory: allocation error\n");
    }

    /* External memory configuration registers */
    devMemInfoPtr->egressMem.extMemRegsNum = EXT_MEM_CONF_NUM;
    devMemInfoPtr->egressMem.extMemRegs =
        calloc(EXT_MEM_CONF_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->egressMem.extMemRegs == 0)
    {
        skernelFatalError("smemTigerAllocSpecMemory: allocation error\n");
    }
    /* Control memory and MLL memory */
    devMemInfoPtr->egressMem.mllTableRegsNum = MLL_TBL_REGS_NUM;
    devMemInfoPtr->egressMem.mllTableRegs =
        calloc(MLL_TBL_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->egressMem.mllTableRegs == 0)
    {
        skernelFatalError("smemTigerAllocSpecMemory: allocation error\n");
    }

    /* trunk non-trunk table and designated trunk ports registers */
    devMemInfoPtr->egressMem.trunkRegsNum = EGR_TRUNK_REGS_NUM;
    devMemInfoPtr->egressMem.trunkMemRegs =
        calloc(EGR_TRUNK_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->egressMem.trunkMemRegs == 0)
    {
        skernelFatalError("smemTigerAllocSpecMemory: allocation error\n");
    }

    /* MAC MIB counters register memory allocation */
    devMemInfoPtr->macFbdMem.macTblRegsNum = MAC_TBL_REGS_NUM;
    devMemInfoPtr->macFbdMem.macTblRegs =
        calloc(MAC_TBL_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->macFbdMem.macTblRegs == 0)
    {
        skernelFatalError("smemTigerAllocSpecMemory: allocation error\n");
    }

    /* VLAN translation table register memory allocation */
    devMemInfoPtr->vlanTblMem.vlanTblRegNum = VLAN_TABLE_REGS_NUM;
    devMemInfoPtr->vlanTblMem.vlanTblRegs =
        calloc(VLAN_TABLE_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->vlanTblMem.vlanTblRegs == 0)
    {
        skernelFatalError("smemTigerAllocSpecMemory: allocation error\n");
    }

    /* Internal SRAM memory allocation */
    devMemInfoPtr->wsramMem.wSram =
        calloc(devMemInfoPtr->wsramMem.wSramSize, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->wsramMem.wSram == 0)
    {
        skernelFatalError("smemTigerAllocSpecMemory: allocation error\n");
    }

    for (i = 0; i < 4; i++)
    {
        if (devMemInfoPtr->nsramsMem[i].nSramSize == 0)
        {
            continue;
        }
        devMemInfoPtr->nsramsMem[i].nSram =
            calloc(devMemInfoPtr->nsramsMem[i].nSramSize, sizeof(SMEM_REGISTER));
        if (devMemInfoPtr->nsramsMem[i].nSram == 0)
        {
            skernelFatalError("smemTigerAllocSpecMemory: allocation error\n");
        }
    }

    /* PCI memory allocation */
    devMemInfoPtr->pciMem.pciRegsNum = PCI_MEM_REGS_NUM;
    devMemInfoPtr->pciMem.pciRegsRegs =
        calloc(PCI_MEM_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->pciMem.pciRegsRegs == 0)
    {
        skernelFatalError("smemTigerAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->pciMem.pciInterRegsNum = PCI_INTERNAL_REGS_NUM;
    devMemInfoPtr->pciMem.pciInterRegs =
        calloc(PCI_INTERNAL_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->pciMem.pciInterRegs == 0)
    {
        skernelFatalError("smemTigerAllocSpecMemory: allocation error\n");
    }
}

/*******************************************************************************
*  smemTigerResetAuq
*
* DESCRIPTION:
*      Reset AUQ structure
* INPUTS:
*      devMemInfoPtr   - pointer to device memory object.
*
* OUTPUTS:
*      devMemInfoPtr   - pointer to device memory object.
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void smemTigerResetAuq(
    INOUT TIGER_DEV_MEM_INFO  * devMemInfoPtr
)
{
    devMemInfoPtr->auqMem.auqBase = 0;
    devMemInfoPtr->auqMem.auqBaseValid = GT_FALSE;
    devMemInfoPtr->auqMem.auqShadow = 0;
    devMemInfoPtr->auqMem.auqShadowValid = GT_FALSE;
    devMemInfoPtr->auqMem.auqOffset = 0;
    devMemInfoPtr->auqMem.baseInit = GT_FALSE;
}

/*******************************************************************************
*  smemTigerSetPciDefault
*
* DESCRIPTION:
*      Set PCI registers default values
*
* INPUTS:
*    devObjPtr        - device object PTR.
*    devMemInfoPtr   - pointer to device memory object.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void smemTigerSetPciDefault(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    INOUT TIGER_DEV_MEM_INFO  * devMemInfoPtr
)
{
    GT_U32              index;
    GT_U32              revision = 0x02000000;

    /* Device and Vendor ID */
    index = 0;

    devMemInfoPtr->pciMem.pciRegsRegs[index] = devObjPtr->deviceType;
    /* Class Code and Revision ID */
    index = 0x08 / 4;
    devMemInfoPtr->pciMem.pciRegsRegs[index] = revision;

    /* Base address */
    index = 0x14 / 4;
    devMemInfoPtr->pciMem.pciRegsRegs[index] = devObjPtr->deviceHwId;
}
/*******************************************************************************
*  smemTigerActiveReadPortIntReg
*
* DESCRIPTION:
*      Definition of the Active register read function.
* INPUTS:
*       deviceObjPtr - device object PTR.
*       address     - Address for ASIC memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers's specific parameter -
*                      global interrupt bit number.
*
* OUTPUTS:
*       outMemPtr   - Pointer to the memory to copy register's content.
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void smemTigerActiveReadPortIntReg (
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    OUT        GT_U32 * outMemPtr
)
{
        GT_U32 fldFirstBit, * regValPtr;
    /* copy registers content to the output memory */
    *outMemPtr = *memPtr;

    fldFirstBit = param;

    /* clear register */
    smemRegSet(deviceObjPtr, address, 0);

    /* update MACSumInt bit in the PCI Interrupt Summary Cause */
    smemPciRegFldSet(deviceObjPtr, PCI_INT_CAUSE_REG, fldFirstBit, 1, 0);

    regValPtr = smemTigerPciReg(deviceObjPtr, PCI_INT_CAUSE_REG, 1, 0);
    /* check enother causes existence */
    if ((*regValPtr & 0xfffffffe) == 0)
    {
        /* clear if not any cause of interrupt */
        *regValPtr = 0;
    }
}
/*******************************************************************************
*  smemTigerActiveReadBrdgIntReg
*
* DESCRIPTION:
*       Definition of the Active register clear function.
* INPUTS:
*       deviceObjPtr - device object PTR.
*       address     - Address for ASIC memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers's specific parameter -
*                      global interrupt bit number.
*
* OUTPUTS:
*       outMemPtr   - Pointer to the memory to copy register's content.
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void smemTigerActiveReadBrdgIntReg (
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    OUT        GT_U32 * outMemPtr
)
{
    GT_U32 fldFirstBit;
    GT_U32 * regValPtr;

    /* copy registers content to the output memory */
    *outMemPtr = *memPtr;

    /* clear register */
    smemRegSet(deviceObjPtr, address, 0);
    fldFirstBit = param;

    /* update Ethernet BridgeSumInt bit in the PCI Interrupt Summary Cause */
    smemPciRegFldSet(deviceObjPtr, PCI_INT_CAUSE_REG, fldFirstBit, 1, 0);

    regValPtr = smemTigerPciReg(deviceObjPtr, PCI_INT_CAUSE_REG, 1, 0);
    /* check enother causes existence */
    if ((*regValPtr & 0xfffffffe) == 0)
    {
        /* clear if not any cause of interrupt */
        *regValPtr = 0;
    }
}

/*******************************************************************************
*  smemTigerActiveReadSdmaIntReg
*
* DESCRIPTION:
*       Definition of the Active register clear function.
* INPUTS:
*       deviceObjPtr - device object PTR.
*       address     - Address for ASIC memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers's specific parameter -
*                      global interrupt bit number.
*
* OUTPUTS:
*       outMemPtr   - Pointer to the memory to copy register's content.
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void smemTigerActiveReadSdmaIntReg (
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    OUT        GT_U32 * outMemPtr
)
{
    GT_U32 fldFirstBit;         /* first field's bit in register */
    GT_U32 * regValPtr;         /* pointer to register value */

    /* copy registers content to the output memory */
    *outMemPtr = *memPtr;

    /* clear register */
    smemRegSet(deviceObjPtr, address, 0);

    /* RxSDMASum Int */
    fldFirstBit = param;
    /* update Ethernet BridgeSumInt bit in the PCI Interrupt Summary Cause */
    smemPciRegFldSet(deviceObjPtr, PCI_INT_CAUSE_REG, fldFirstBit, 1, 0);

    regValPtr = smemTigerPciReg(deviceObjPtr, PCI_INT_CAUSE_REG, 1, 0);
    /* check another causes existence */
    if ((*regValPtr & 0xfffffffe) == 0)
    {
        /* clear if not any cause of interrupt */
        *regValPtr = 0;
    }
}
/*******************************************************************************
*  smemTigerActiveWriteFdbMsg
*
* DESCRIPTION:
*      Write to the Message from CPU Register3 - activate update FDB message.
* INPUTS:
*       deviceObjPtr - device object PTR.
*       address     - Address for ASIC memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers's specific parameter.
*       inMemPtr    - Pointer to the memory to get register's content.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void smemTigerActiveWriteFdbMsg (
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
)
{
    SBUF_BUF_ID             bufferId;    /* buffer */
    GT_U8                *  data_PTR;  /* pointer to the data in the buffer */
    GT_U32                  data_size;  /* data size */
    GT_U32               *  msgFromCpu0Addr; /* pointer to the memory of
                                               Message from CPU Register0*/
    *memPtr = *inMemPtr;

    /* send message to SKernel task to process it. */

    /* get buffer */
    bufferId = sbufAlloc(deviceObjPtr->bufPool,
                SMEM_TIGER_FDB_UPD_MSG_WORDS_NUM * sizeof(GT_U32));

    if ( bufferId == NULL )
    {
        printf(" smemTigerActiveWriteFdbMsg: "\
                "no buffers to update MAC table\n");
        return;
    }

    /* get actual data pointer */
    sbufDataGet(bufferId, &data_PTR, &data_size);

    /* find location of Message from CPU Register0 Offset: 0x48 */
    msgFromCpu0Addr = memPtr;

    /* copy fdb update message to buffer */
    memcpy(data_PTR, (GT_U8_PTR)msgFromCpu0Addr,
            SMEM_TIGER_FDB_UPD_MSG_WORDS_NUM * sizeof(GT_U32));

    /* set source type of buffer */
    bufferId->srcType = SMAIN_SRC_TYPE_CPU_E;

    /* set message type of buffer */
    bufferId->dataType = SMAIN_MSG_TYPE_FDB_UPDATE_E;

    /* put buffer to queue */
    squeBufPut(deviceObjPtr->queueId, SIM_CAST_BUFF(bufferId));

}

/*******************************************************************************
*  smemTigerActiveAuqBaseWrite
*
* DESCRIPTION:
*       AUQ Base address reg 0x14
* INPUTS:
*       deviceObjPtr - device object PTR.
*       address     - Address for ASIC memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers's specific parameter.
*       inMemPtr    - Pointer to the memory to get register's content.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void smemTigerActiveAuqBaseWrite (
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
)
{
    TIGER_DEV_MEM_INFO  * devMemInfoPtr;

    *memPtr = *inMemPtr;

    devMemInfoPtr = (TIGER_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    /* First time */
    if(devMemInfoPtr->auqMem.baseInit ==  GT_FALSE) {
        devMemInfoPtr->auqMem.auqBase = *inMemPtr;
        devMemInfoPtr->auqMem.baseInit = GT_TRUE;
        devMemInfoPtr->auqMem.auqBaseValid = GT_TRUE;
    }
    else {
        if (devMemInfoPtr->auqMem.auqBaseValid == GT_TRUE)
        {
            devMemInfoPtr->auqMem.auqShadow = *inMemPtr;
            devMemInfoPtr->auqMem.auqShadowValid = GT_TRUE;
        }
        else
        {
            devMemInfoPtr->auqMem.auqBase = *inMemPtr;
            devMemInfoPtr->auqMem.auqBaseValid = GT_TRUE;
            devMemInfoPtr->auqMem.auqOffset = 0;
        }
    }
}

/*******************************************************************************
*  smemTigerExternalMem
*
* DESCRIPTION:
*       External memory access
* INPUTS:
*       deviceObjPtr - device object PTR.
*       address     - Address for ASIC memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers's specific parameter.
*       inMemPtr    - Pointer to the memory to get register's content.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*       01 - SRAM 0, the wide one
       10 - SRAM 1, the large one
       11 - Flow DDR, the one with 16 bit
*
*******************************************************************************/
static GT_U32 *  smemTigerExternalMem(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    TIGER_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32              * regValPtr;
    GT_U32              index;
    regValPtr = 0;
    devMemInfoPtr = (TIGER_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;

    index = (address & 0x7FFFFFF) / 0x4;

    switch(param) {
        case 4: /* 0x20000000 */
            CHECK_MEM_BOUNDS(devMemInfoPtr->nsramsMem[0].nSram,
                             devMemInfoPtr->nsramsMem[0].nSramSize,
                             index, memSize);
            regValPtr = &devMemInfoPtr->nsramsMem[0].nSram[index];
        break;
        case 5: /* 0x28000000 */
            CHECK_MEM_BOUNDS(devMemInfoPtr->nsramsMem[1].nSram,
                             devMemInfoPtr->nsramsMem[1].nSramSize,
                             index, memSize);
            regValPtr = &devMemInfoPtr->nsramsMem[1].nSram[index];
        break;
        case 6: /* 0x30000000 */
            CHECK_MEM_BOUNDS(devMemInfoPtr->nsramsMem[2].nSram,
                             devMemInfoPtr->nsramsMem[2].nSramSize,
                             index, memSize);
            regValPtr = &devMemInfoPtr->nsramsMem[2].nSram[index];
        break;
        case 7: /* 0x38000000 */
            CHECK_MEM_BOUNDS(devMemInfoPtr->nsramsMem[3].nSram,
                             devMemInfoPtr->nsramsMem[3].nSramSize,
                             index, memSize);
            regValPtr = &devMemInfoPtr->nsramsMem[3].nSram[index];
        break;
    }

    return regValPtr;
}

/*******************************************************************************
*  smemMemSizeToBytes
*
* DESCRIPTION:
*       Conversion DRAM memory size from INI files to number of bytes
* INPUTS:
*       memSize - DRAM memory size
*       memBytes - memory size in bytes
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void smemMemSizeToBytes(
        IN GT_CHAR * memSizePtr,
        INOUT GT_U32 * memBytesPtr
)
{
        GT_U32 len;
        GT_U32 megaByte = 1;

        ASSERT_PTR(memSizePtr);
        ASSERT_PTR(memBytesPtr);

        len = (GT_U32)strlen(memSizePtr);
        if (memSizePtr[len - 1] == 'M' ||
                memSizePtr[len - 1] == 'm')
        {
                memSizePtr[len - 1] = 0;
                megaByte = 0x100000;
        }
        sscanf(memSizePtr, "%u", memBytesPtr);
        *memBytesPtr *= megaByte;
}
/*******************************************************************************
*  smemTigerActiveReadCntrs
*
* DESCRIPTION:
*      Read counters.
* INPUTS:
*       deviceObjPtr - device object PTR.
*       address     - Address for ASIC memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers's specific parameter -
*                      global interrupt bit number.
*
* OUTPUTS:
*       outMemPtr   - Pointer to the memory to copy register's content.
* RETURNS:
*
* COMMENTS:
*         Output counter value and reset it.
*
*******************************************************************************/
static void smemTigerActiveReadCntrs (
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    OUT        GT_U32 * outMemPtr
)
{
    /* output actual value of counter */
    * outMemPtr = * memPtr;

    /* reset counters value */
    * memPtr = 0;
}

/*******************************************************************************
*  smemTigerPhyRegsInit
*
* DESCRIPTION:
*      Set PCI registers default values
* INPUTS:
*      deviceObjPtr   - pointer to device object.
*      devMemInfoPtr  - pointer to device memory object.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void smemTigerPhyRegsInit(
    IN SKERNEL_DEVICE_OBJECT    *       deviceObjPtr,
    INOUT TIGER_DEV_MEM_INFO    *       devMemInfoPtr
)
{
    /* init XAUI (10GB port) phy related register */
    devMemInfoPtr->phyMem.ieeeXauiRegsNum = PHY_IEEE_XAUI_REGS_NUM;
    devMemInfoPtr->phyMem.ieeeXauiRegs =
                        calloc(devMemInfoPtr->phyMem.ieeeXauiRegsNum,
                                            sizeof(SMEM_PHY_REGISTER));

    if (devMemInfoPtr->phyMem.ieeeXauiRegs == 0)
    {
        skernelFatalError("smemTigerPhyRegsInit: ieee allocation error\n");
    }

    devMemInfoPtr->phyMem.extXauiRegsNum = PHY_EXT_XAUI_REGS_NUM;
    devMemInfoPtr->phyMem.extXauiRegs =
                        calloc(devMemInfoPtr->phyMem.extXauiRegsNum,
                                            sizeof(SMEM_PHY_REGISTER));

    if (devMemInfoPtr->phyMem.extXauiRegs == 0)
    {
        skernelFatalError("smemTigerPhyRegsInit: ext allocation error\n");
    }
}
/*******************************************************************************
*  smemTigerXauiPhyRegsFing
*
* DESCRIPTION:
*      Find memory location for XAUI register
* INPUTS:
*      deviceObjPtr   - pointer to device object.
*      deviceAddr     - address of XAUI device.
*      addr           - address of XAUI register.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static SMEM_PHY_REGISTER * smemTigerXauiPhyRegsFing(
    IN SKERNEL_DEVICE_OBJECT    *       deviceObjPtr,
    IN GT_U32                           deviceAddr,
    IN GT_U32                           regAddr
)
{
    TIGER_DEV_MEM_INFO  *   devMemInfoPtr;
    GT_U32                  index; /* index of register */

    devMemInfoPtr = (TIGER_DEV_MEM_INFO *) deviceObjPtr->deviceMemory;
    index = deviceAddr * (regAddr & 0xff);

    if (regAddr >= 0x8000)
    {
        /* XAUI marvell extended registers */
        if (index < devMemInfoPtr->phyMem.extXauiRegsNum)
        {
            return &(devMemInfoPtr->phyMem.extXauiRegs[index]);
        }
    }
    else
    {
        /* XAUI standard registers */
        if (index < devMemInfoPtr->phyMem.ieeeXauiRegsNum)
        {
            return &(devMemInfoPtr->phyMem.ieeeXauiRegs[index]);
        }
    }

    /* not found */
    skernelFatalError("smemTigerXauiPhyRegsFing: wrong PHY address,"\
    "PP device is %d, PHY device is %X, Register address %04X",
                    deviceObjPtr->deviceId, deviceAddr, regAddr);

    return NULL;

}

/*******************************************************************************
*  smemTigerActiveWriteFdbMsg
*
* DESCRIPTION:
*      Handler for write to the 10G SMII control register - phy configuration.
* INPUTS:
*       deviceObjPtr - device object PTR.
*       address     - Address for ASIC memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers's specific parameter.
*       inMemPtr    - Pointer to the memory to get register's content.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void smemTigerActiveWriteXSmii (
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
)
{
    SMEM_PHY_REGISTER   * simRegPtr; /* location of register in the simulation*/
    GT_U32                regAddr;   /* addres of the phy register */
    GT_U32                phyDevice; /* device id of phy */
    GT_U32                operCode; /* SMII Operation code */
    GT_U16                regData; /* Register data */

    phyDevice = ((*inMemPtr) >> 21) & 0x1f;

    /* get phy reg address */
    smemRegFldGet(deviceObjPtr, 0x03800008,0,15,&regAddr);

    /* find register in simulation */
    simRegPtr = smemTigerXauiPhyRegsFing(deviceObjPtr, phyDevice, regAddr);

    regData = (GT_U16)((*inMemPtr) & 0xff);
    operCode = ((*inMemPtr) >> 26) & 0x7;
    switch (operCode)
    {
        case 0x1 :
        case 0x5 :
            /* Write only */
            *simRegPtr = regData;
            break;

        case 0x3 :
        case 0x6 :
        case 0x7 :
            /* Read only */
            *inMemPtr &= 0xffff0000;
            *inMemPtr |= *simRegPtr;

            /* read data ready */
            *inMemPtr |= (1 << 29);
            break;

      default: skernelFatalError("smemTigerActiveWriteXSmii: wrong opCode %d",
                                    operCode);
    }
}

/*******************************************************************************
*  smemTigerActiveWriteSFlowCnt
*
* DESCRIPTION:
*      Update sflow Fifo
* INPUTS:
*       deviceObjPtr - device object PTR.
*       address     - Address for ASIC memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers's specific parameter.
*       inMemPtr    - Pointer to the memory to get register's content.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void smemTigerActiveWriteSFlowCnt(
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
)
{
    GT_U32                fldValue;
    GT_U32                fifoStatus,sflowCounter, i;
    GT_U32              * sflowFifoPtr;
    TIGER_DEV_MEM_INFO  * devMemInfoPtr;

    devMemInfoPtr = (TIGER_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;

    smemRegFldGet(deviceObjPtr, SFLOW_STATUS_REG, 20, 5, &fldValue);
    /* FifoStatus */
    fifoStatus = fldValue;
    if (fifoStatus == 0)
    {
        /* Get sFlow counter status register */
        smemRegGet(deviceObjPtr, SFLOW_COUNT_STATUS_REG, &sflowCounter);
        if (sflowCounter == 0)
        {
                smemRegSet(deviceObjPtr, SFLOW_COUNT_STATUS_REG, *inMemPtr);
                return;
        }
    }


    if (fifoStatus != 32)
    {
        fifoStatus = fifoStatus  +  1;
    }
    fldValue = fifoStatus;
    /* FifoStatus */
    smemRegFldSet(deviceObjPtr, SFLOW_STATUS_REG, 20, 5, fldValue);

    sflowFifoPtr = devMemInfoPtr->macMem.sflowFifo;
    for (i = fifoStatus ; i > 0 && i < 32; i--)
    {
        sflowFifoPtr[i] = sflowFifoPtr[i - 1];
    }


    sflowFifoPtr[0] = *inMemPtr;
    *memPtr = *inMemPtr;
}

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
GT_U32 smemTigerNsramRead (
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SMEM_TIGER_NSRAM_BASE_ADDR_ENT nsramBaseAddr,
    IN GT_U32 sramAddrOffset
)
{
    GT_U32 address;
    GT_U32 dramType;
    GT_U32 * nSramWordPtr;

    address = SMEM_TIGER_NSRAM_ADDR(nsramBaseAddr, sramAddrOffset);
    dramType = ((nsramBaseAddr >> 27) & 0x07);

    nSramWordPtr = smemTigerExternalMem(deviceObjPtr, address, 1, dramType);

    return (*nSramWordPtr);
}

/*******************************************************************************
*  smemTigerActivePciWriteIntr
*
* DESCRIPTION:
*       PCI interrupt cause reg 0x114
* INPUTS:
*       deviceObjPtr - device object PTR.
*       address     - Address for ASIC memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers's specific parameter.
*       inMemPtr    - Pointer to the memory to get register's content.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void smemTigerActivePciWriteIntr (
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
)
{
    /* do nothing - don't let PSS write to Read only register */
    return;

}
/*******************************************************************************
*  smemTigerFaMem
*
* DESCRIPTION:
*       External memory access
* INPUTS:
*       deviceObjPtr - device object PTR.
*       address     - Address for FA memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers's specific parameter.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*       01 - SRAM 0, the wide one
       10 - SRAM 1, the large one
       11 - Flow DDR, the one with 16 bit
*
*******************************************************************************/
static GT_U32 *  smemTigerFaMem(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    GT_U32              * regValPtr;
    GT_U32                index;

    index = (address & 0xff) / 4;
    regValPtr = &smemFaMemory[index];

    if ((address & 0xff) == 0x5c)
    {
        smemFaMemory[index] = 0x3;
    }

    return regValPtr;
}


/*******************************************************************************
*  smemTigerActiveWriteTcbGenxsReg
*
* DESCRIPTION:
*      the action of writing to tcb genxs register suppose to "reset" counters
*      of traffic condition entry as specified in bits[0:15] with options as
*      as in bit [16] but the end of action is "done" when the PP is setting
*      bit [31] to 1 .
*
*      in real PP the PP is coping to registers the "old value" of the counters
*      that are about to be "reset"
*
* INPUTS:
*       deviceObjPtr - device object PTR.
*       address     - Address for ASIC memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers's specific parameter.
*       inMemPtr    - Pointer to the memory to get register's content.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void smemTigerActiveWriteTcbGenxsReg(
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
)
{
    /* let traffic condition class do it */
    snetSambaTrafficConditionWriteGenxsReg(deviceObjPtr,
                                           inMemPtr[0]&0xffff,
                                           (inMemPtr[0]&0x10000)>>16);
}

/*******************************************************************************
*  smemTigerActiveReadPortEgressCountersReg
*
* DESCRIPTION:
*       action of read the Egress port counters
*       the action of reading those registers(counters) cause them to reset
*
*
* INPUTS:
*       deviceObjPtr - device object PTR.
*       address     - Address for ASIC memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers's specific parameter.
*       inMemPtr    - Pointer to the memory to get register's content.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void smemTigerActiveReadPortEgressCountersReg(
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    OUT        GT_U32 * outMemPtr
)
{
    /* output actual value of counter */
    * outMemPtr = * memPtr;

    /* reset counters value */
    * memPtr = 0;
}

/*******************************************************************************
*   smemTigerTxEndIntrCall
*
* DESCRIPTION:
*         simulate tx end interrupt
* INPUTS:
*        devObj_PTR - pointer to device object.
*        queueNum  - number of TX queue interrupt occured for.
* OUTPUTS:
*
* COMMENTS:
*           The Tx End for Queue 0 bit indicates that the Tx DMA stopped processing
*           the queue 0 after setting the DISQ bit, or that it reached the end of the queue
*           0 descriptor chain (through a NULL pointer or a not owned descriptor).
*
*******************************************************************************/

static void smemTigerTxEndIntrCall(
    IN SKERNEL_DEVICE_OBJECT    *devObj_PTR,
    IN GT_U8                    queueNum
)
{
    GT_U32  fldValue;
    GT_BOOL globIntrNotify;

    globIntrNotify = GT_FALSE;
    /* check that interrupt enabled */
    smemRegFldGet(devObj_PTR, TRANS_SDMA_INTR_MASK_REG, 17 + queueNum, 1, &fldValue);
    /* set The Tx End for Queue <n> */
    if (fldValue)
    {
        smemRegFldSet(devObj_PTR, TRANS_SDMA_INTR_CAUSE_REG, 17 + queueNum, 1, 1);
        smemRegFldSet(devObj_PTR, TRANS_SDMA_INTR_CAUSE_REG, 17 + queueNum, 1, 1);

        /* PCI interrupt */
        smemPciRegFldGet(devObj_PTR, PCI_INT_MASK_REG, 18, 1, &fldValue);
        if (fldValue)
        {
            smemPciRegFldSet(devObj_PTR, PCI_INT_CAUSE_REG, 18, 1, 1);
            smemPciRegFldSet(devObj_PTR, PCI_INT_CAUSE_REG, 0, 1, 1);
            globIntrNotify = GT_TRUE;
        }
    }

    if (globIntrNotify == GT_TRUE)
    {
        scibSetInterrupt(devObj_PTR->deviceId);
    }

}

/*******************************************************************************
*   smemTigerTxResourceErrorIntrCall
*
* DESCRIPTION:
*         simulate Tx Resource Error interrupt
* INPUTS:
*        devObj_PTR - pointer to device object.
*        queueNum  - number of TX queue interrupt occured for.
* OUTPUTS:
*
* COMMENTS:
*           The Tx Resource Error for Queue 0 bit indicates a Tx resource error event
*           during packet transmission from the queue.
*
*******************************************************************************/

static void smemTigerTxResourceErrorIntrCall(
    IN SKERNEL_DEVICE_OBJECT    *devObj_PTR,
    IN GT_U8                    queueNum
)
{
    GT_U32  fldValue;
    GT_BOOL globIntrNotify = GT_FALSE;

    /* check that interrupt enabled */
    smemRegFldGet(devObj_PTR, TRANS_SDMA_INTR_MASK_REG, 9 + queueNum, 1, &fldValue);
    /* set The Tx Resource Error for Queue <n> */
    if (fldValue)
    {
        smemRegFldSet(devObj_PTR, TRANS_SDMA_INTR_CAUSE_REG, 9 + queueNum, 1, 1);
        smemRegFldSet(devObj_PTR, TRANS_SDMA_INTR_CAUSE_REG, 9 + queueNum, 1, 1);

        /* PCI interrupt */
        smemPciRegFldGet(devObj_PTR, PCI_INT_MASK_REG, 18, 1, &fldValue);
        if (fldValue)
        {
            smemPciRegFldSet(devObj_PTR, PCI_INT_CAUSE_REG, 18, 1, 1);
            smemPciRegFldSet(devObj_PTR, PCI_INT_CAUSE_REG, 0, 1, 1);
            globIntrNotify = GT_TRUE;
        }
    }

    if (globIntrNotify == GT_TRUE)
    {
        scibSetInterrupt(devObj_PTR->deviceId);
    }

}

/*******************************************************************************
*  smemTigerActiveWriteTransmitQueueCmdReg
*
* DESCRIPTION:
*      Handler for write Transmit Queue Command Register
*              responsible for Tx from CPU and enable/disable transmit
*              on given queue
* INPUTS:
*       deviceObjPtr - device object PTR.
*       address     - Address for ASIC memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers's specific parameter.
*       inMemPtr    - Pointer to the memory to get register's content.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*           The Enable Queue [7:0] field action:
* In the Enable Queue [7:0] field each bit corresponds to one out of the 8
* SDMA queue/channels.
* Writing 1 to a given bit, enables the corresponding queue. The Transmit
* SDMA will fetch the first descriptor programmed to the FDP register for that
* queue and start the transmit process.
* Writing 1 to the ENQ bit resets the matching DISQ bit.
* Writing 1 to the ENQ bit of a TxSDMA that is already in enabled state, has
* no effect.
* Writing 0 to the ENQ bit has no effect.
* When the Transmit SDMA encounters a queue ended either by a NULL terminated
* descriptor pointer or by a CPU owned descriptor, the DMA will
* clear the ENQ bit for that queue. Thus reading these bits reports the active
* enable status for each queue.
*
*            The Disable Queue [7:0] field action:
* In the Disable Queue [7:0] each bit corresponds to one out of the 8 SDMA
* queue/channels.
* Writing 1 to a given bit disables the corresponding queue.
* The Transmit SDMA stops the transmit process from this queue, on the
* next packet boundary.
* Writing 1 to the DISQ bit resets the matching ENQ bit.
* Writing 0 to the DISQ bit has no effect.
* When theTransmit SDMA encounters a queue ended either by a NULL terminated
* descriptor pointer or by a CPU owned descriptor, the SDMA disables
* the queue but doesnot set the DISQ bit for that queue, thus reading
* DISQ and ENQ bits discriminates between queues disables by the CPU
* and those stopped by the SDMA due to a NULL pointer or a CPU owned
* descriptor.
*
*******************************************************************************/
static void smemTigerActiveWriteTransmitQueueCmdReg (
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
)
{
    GT_U32                  txQueueCmdRegVal, txCurrDescPtrRegVal;
    SBUF_BUF_ID             buffId = NULL;
    GT_U32                  msgSize = 0, currBuffSize = 0;
    SBUF_BUF_STC            *bufInfo_PTR = NULL;
    SNET_STRUCT_TX_DESC     *txDesc;
    GT_U8                   *tmpDataPtr = NULL;
    GT_BOOL                 queueLinkListFinished;
    GT_U32                  extTxDescDataWord3, extTxDescDataWord2;
    GT_U8                   i, currState;
    GT_U32                  errataStripHeaderOffset =0;
    GT_U8                   *actualDataPtr;


    /* write new value into register */
    *memPtr |= *inMemPtr;

    txQueueCmdRegVal = *memPtr;

    /* run through all 8 tx queues */
    for (i = 0; (i < TRANS_QUEUE_MAX_NUMBER) && (txQueueCmdRegVal >> i); i++)
    {
        if(txQueueCmdRegVal & (1 << i))
        {
            /* reset the matching DISQ bit */
            smemRegFldSet(deviceObjPtr, TRANS_QUEUE_COMMAND_REG,
                          TRANS_QUEUE_MAX_NUMBER - 1 + i, 1, 0);

            /* read descriptor address of appropriate queue */
            smemRegGet(deviceObjPtr, TRANS_CURR_DESC_PTR_REG + i*4, &txCurrDescPtrRegVal);

            txDesc = (SNET_STRUCT_TX_DESC*)(GT_UINTPTR)txCurrDescPtrRegVal;

            currState = 1;

            /* run through whole linked list of tx buffers for current queue */
            queueLinkListFinished = GT_FALSE;
            while(!queueLinkListFinished)
            {
                switch(currState)
                {
                    case 1:/* first descriptor of frame expected */
                        /**********************
                         * first buffer of frame is 16 bytes of extended descriptor data
                         **********************/

                        if((txDesc == NULL) || (TX_DESC_GET_OWN_BIT(txDesc) == 0))
                        {
                            /* clear matching ENQ bit */
                            smemRegFldSet(deviceObjPtr, TRANS_QUEUE_COMMAND_REG, i, 1, 0);

                            if(txDesc == NULL)
                            {
                                smemTigerTxResourceErrorIntrCall(deviceObjPtr, i);
                            }
                            else
                            {
                                /* move current TX descriptor pointer to first descriptor
                                 * of next frame
                                 */
                                smemRegSet(deviceObjPtr, TRANS_CURR_DESC_PTR_REG + i*4,
                                           (GT_U32)((GT_UINTPTR)txDesc));
                            }

                            /* continue to next tx queue */
                            queueLinkListFinished = GT_TRUE;
                            continue;
                        }

                        if(TX_DESC_GET_FIRST_BIT(txDesc) == 0)
                        {
                            skernelFatalError("smemTigerActiveWriteTransmitQueueCmdReg: first descriptor of frame omited\n");
                        }

                        /* allocate simulation buffer for frame tx */
                        buffId = sbufAlloc(deviceObjPtr->bufPool, SBUF_DATA_SIZE_CNS);
                        if ( buffId == NULL )
                        {
                            skernelFatalError("smemTigerActiveWriteTransmitQueueCmdReg: no free buffer\n");
                        }

                        bufInfo_PTR = (SBUF_BUF_STC*)buffId;

                        tmpDataPtr = bufInfo_PTR->data;

                        /* get num of bytes in buffer */
                        /* Note: buffer length of first buffer doesn't contain
                         * length of extended descriptor data
                         */
                        if(U32_GET_FIELD((txDesc)->word2,16,14) != TX_HEADER_SIZE)
                        {
                            skernelFatalError("smemTigerActiveWriteTransmitQueueCmdReg: extended descriptor data omited\n");
                        }

                        /* copy frame to simulation tx buffer */
                        /*memcpy(tmpDataPtr, (GT_U8*)txDesc->buffPointer, TX_HEADER_SIZE);*/
                        scibDmaRead(deviceObjPtr->deviceId,txDesc->buffPointer,TX_HEADER_SIZE/4,(GT_U32*)tmpDataPtr,SCIB_DMA_BYTE_STREAM);

                        /* init general msg size */
                        msgSize = TX_HEADER_SIZE;

                        /* set simulation tx descriptor */
                        bufInfo_PTR->srcData                    = TIGER_CPU_PORT_CNS;
                        bufInfo_PTR->srcType                    = SMAIN_SRC_TYPE_CPU_E;
                        bufInfo_PTR->dataType                   = SMAIN_MSG_TYPE_FRAME_E;
                        bufInfo_PTR->userInfo.target.devObj_PTR = deviceObjPtr;
                        bufInfo_PTR->userInfo.type              = SBUF_USER_INFO_TYPE_SAPI_TX_E;

                        bufInfo_PTR->userInfo.data.sapiTxPacket.txQueue         = i;
                        bufInfo_PTR->userInfo.data.sapiTxPacket.txDevice        = (GT_U8)deviceObjPtr->deviceId;

                        bufInfo_PTR->userInfo.data.sapiTxPacket.dropPrecedence  = (GT_U8)U32_GET_FIELD((txDesc)->word1,14,2);
                        bufInfo_PTR->userInfo.data.sapiTxPacket.packetEncap     = U32_GET_FIELD((txDesc)->word1,16,3);
                        bufInfo_PTR->userInfo.data.sapiTxPacket.packetTagged    = U32_GET_FIELD((txDesc)->word1,22,1);
                        bufInfo_PTR->userInfo.data.sapiTxPacket.recalcCrc       = U32_GET_FIELD((txDesc)->word1,12,1);
                        bufInfo_PTR->userInfo.data.sapiTxPacket.userPrioTag     = (GT_U8)U32_GET_FIELD((txDesc)->word1,24,3);
                        bufInfo_PTR->userInfo.data.sapiTxPacket.vid             = (GT_U16)U32_GET_FIELD((txDesc)->word1,0,12);
                        /* ??? bufInfo_PTR->userInfo.data.sapiTxPacket.sendTagged  ??? */

                        /* take info from extended tx descriptor data  - first 16 bytes of packet */
                        /*extTxDescDataWord2 = ((GT_U32*)txDesc->buffPointer)[2];*/
                        scibDmaRead(deviceObjPtr->deviceId,txDesc->buffPointer+2*4,TX_HEADER_SIZE/4,(GT_U32*)&extTxDescDataWord2,SCIB_DMA_WORDS);
                        /*extTxDescDataWord3 = ((GT_U32*)txDesc->buffPointer)[3];*/
                        scibDmaRead(deviceObjPtr->deviceId,txDesc->buffPointer+3*4,TX_HEADER_SIZE/4,(GT_U32*)&extTxDescDataWord3,SCIB_DMA_WORDS);
                        if(U32_GET_FIELD(extTxDescDataWord3,1,1) == 1)
                            bufInfo_PTR->userInfo.data.sapiTxPacket.useVidx = GT_TRUE;
                        else
                            bufInfo_PTR->userInfo.data.sapiTxPacket.useVidx = GT_FALSE;

                        if(bufInfo_PTR->userInfo.data.sapiTxPacket.useVidx)
                        {
                            bufInfo_PTR->userInfo.data.sapiTxPacket.dest.vidx = (GT_U16)U32_GET_FIELD(extTxDescDataWord3,2,14);
                        }
                        else
                        {
                            bufInfo_PTR->userInfo.data.sapiTxPacket.dest.devPort.tgtDev = (GT_U8)U32_GET_FIELD(extTxDescDataWord3,9,7);
                            bufInfo_PTR->userInfo.data.sapiTxPacket.dest.devPort.tgtPort = (GT_U8)U32_GET_FIELD(extTxDescDataWord3,3,6);
                        }

                        bufInfo_PTR->userInfo.data.sapiTxPacket.macDaType = U32_GET_FIELD(extTxDescDataWord3,17,2);

                        /* ??? bufInfo_PTR->userInfo.data.sapiTxPacket.cookie ??? */

                        /* Check descriptor (MPLS Header params) for CPU Tx Packet Size Bug WA
                         * errata FEr #32 Buffer loss in CPU memory (129 bytes errata)
                         *
                         * To avoid sending packets with hazardous byte count from the CPU to the
                         * packet processor, the Prestera TAPI drivers are able to detect packets with
                         * problematic sizes and append a 22-byte header to the beginning of the
                         * packet. The packet processor strips the header before it is sent to the
                         * network.
                         */
                        if((U32_GET_FIELD(extTxDescDataWord3,24,1) == 1)
                           && (U32_GET_FIELD(extTxDescDataWord3,25,3) == 3)
                           && (U32_GET_FIELD(extTxDescDataWord3,28,1) == 0)
                           && (U32_GET_FIELD(extTxDescDataWord3,29,2) == 0)
                           && (U32_GET_FIELD(extTxDescDataWord2,5,3)  == 3)
                           && (U32_GET_FIELD(extTxDescDataWord2,8,3)  == 6))
                        {
                            errataStripHeaderOffset = 22;
                        }
                        else
                        {
                            errataStripHeaderOffset = 0;
                        }

                        tmpDataPtr += TX_HEADER_SIZE;
                        currState = 2;

                        /* return ownership on descr to CPU to enable release of descr */
                        TX_DESC_SET_OWN_BIT(txDesc, TX_DESC_CPU_OWN);

                        break;

                    case 2:/* read linked list until last descriptor of frame found */
                        txDesc = (SNET_STRUCT_TX_DESC*)((GT_UINTPTR)((txDesc)->nextDescPointer));
                        if((txDesc == NULL) || (TX_DESC_GET_OWN_BIT(txDesc) == 0))
                        {
                            /* clear matching ENQ bit */
                            smemRegFldSet(deviceObjPtr, TRANS_QUEUE_COMMAND_REG, i, 1, 0);

                            if(txDesc == NULL)
                                smemTigerTxResourceErrorIntrCall(deviceObjPtr, i);

                            /* continue to next tx queue */
                            queueLinkListFinished = GT_TRUE;
                            continue;
                        }

                        /* count num of bytes in frame */
                        currBuffSize = U32_GET_FIELD((txDesc)->word2,16,14);
                        msgSize += currBuffSize;
                        if(msgSize > SBUF_DATA_SIZE_CNS)
                        {
                            skernelFatalError("smemTigerActiveWriteTransmitQueueCmdReg: packet too long\n");
                        }

                        /* take info from extended tx descriptor data  - first 16 bytes of packet */
                        /*extTxDescDataWord2 = ((GT_U32*)txDesc->buffPointer)[2];*/
                        scibDmaRead(deviceObjPtr->deviceId,txDesc->buffPointer+2*4,TX_HEADER_SIZE/4,(GT_U32*)&extTxDescDataWord2,SCIB_DMA_WORDS);
                        /*extTxDescDataWord3 = ((GT_U32*)txDesc->buffPointer)[3];*/
                        scibDmaRead(deviceObjPtr->deviceId,txDesc->buffPointer+3*4,TX_HEADER_SIZE/4,(GT_U32*)&extTxDescDataWord3,SCIB_DMA_WORDS);
                        if((U32_GET_FIELD(extTxDescDataWord3,24,1) == 1)
                           && (U32_GET_FIELD(extTxDescDataWord3,25,3) == 3)
                           && (U32_GET_FIELD(extTxDescDataWord3,28,1) == 0)
                           && (U32_GET_FIELD(extTxDescDataWord3,29,2) == 0)
                           && (U32_GET_FIELD(extTxDescDataWord2,5,3)  == 3)
                           && (U32_GET_FIELD(extTxDescDataWord2,8,3)  == 6))
                        {
                            errataStripHeaderOffset += currBuffSize;
                        }

                        /* copy frame to simulation tx buffer */
                        /*memcpy(tmpDataPtr, (GT_U8*)txDesc->buffPointer, currBuffSize);*/
                        scibDmaRead(deviceObjPtr->deviceId,txDesc->buffPointer,NUM_BYTES_TO_WORDS(currBuffSize),(GT_U32*)tmpDataPtr,SCIB_DMA_BYTE_STREAM);

                        if(TX_DESC_GET_LAST_BIT(txDesc) == 1)
                        {/* if last descriptor jump to send packet */
                            currState = 3;
                        }
                        else
                        {/* continue to read buffers of current frame */
                            tmpDataPtr += currBuffSize;
                            currState = 2;
                        }

                        /* return ownership on descr to CPU to enable release of descr */
                        TX_DESC_SET_OWN_BIT(txDesc, TX_DESC_CPU_OWN);

                        break;

                    case 3: /* last descriptor of frame found */

                        msgSize -= TX_HEADER_SIZE;
                        actualDataPtr = bufInfo_PTR->data + TX_HEADER_SIZE;

                        /* remove errata FEr #32 added header if needed */
                        msgSize -= errataStripHeaderOffset;
                        actualDataPtr += errataStripHeaderOffset;

                        sbufDataSet(buffId, actualDataPtr, msgSize);

                        /* put the data on the queue to scor */
                        squeBufPut(deviceObjPtr->queueId, SIM_CAST_BUFF(buffId));

                        /* check if there is another frame in list */
                        txDesc = (SNET_STRUCT_TX_DESC*)((GT_UINTPTR)((txDesc)->nextDescPointer));
                        currState = 1;

                        break;

                    default:
                        skernelFatalError("smemTigerActiveWriteTransmitQueueCmdReg: simulation algorithm error\n");
                        break;
                }

            }/* end of while(!queueLinkListFinished) */

            smemTigerTxEndIntrCall(deviceObjPtr, i);

        } /* end of if(txQueueCmdRegVal & (1 << i)) */

    } /* end of for (i = 0; */

}

/*******************************************************************************
*  smemTigerActiveReadTcbCountAlarmReg
*
* DESCRIPTION:
*      Read the register on TCB alarm counter .
*      some bits in this register will be 0 after this read action
* INPUTS:
*       deviceObjPtr - device object PTR.
*       address     - Address for ASIC memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers's specific parameter -
*                      global interrupt bit number.
*
* OUTPUTS:
*       outMemPtr   - Pointer to the memory to copy register's content.
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemTigerActiveReadTcbCounterAlarmReg (
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    OUT        GT_U32 * outMemPtr
)
{
    /* output actual value of counter */
    * outMemPtr = * memPtr;

    /* reset some of its bits */
    * memPtr &= ~(0xffff<<3);/* leave tc pointer field */
}


/*******************************************************************************
*  smemTigerActiveReadTcbPolicingCounterAlarmReg
*
* DESCRIPTION:
*      Read the register on TCB policing alarm counter .
*      some bits in this register will be 0 after this read action
* INPUTS:
*       deviceObjPtr - device object PTR.
*       address     - Address for ASIC memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers's specific parameter -
*                      global interrupt bit number.
*
* OUTPUTS:
*       outMemPtr   - Pointer to the memory to copy register's content.
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemTigerActiveReadTcbPolicingCounterAlarmReg (
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    OUT        GT_U32 * outMemPtr
)
{
    /* output actual value of counter */
    * outMemPtr = * memPtr;

    /* reset some of its bits */
    * memPtr &= ~(0xffff<<4);/* leave tc pointer field */
}


/*******************************************************************************
*  smemTigerActiveReadTcbGlobalCounterReg
*
* DESCRIPTION:
*      Read the register on TCB global counter .
*      some bits in this register will be 0 after this read action
* INPUTS:
*       deviceObjPtr - device object PTR.
*       address     - Address for ASIC memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers's specific parameter -
*                      global interrupt bit number.
*
* OUTPUTS:
*       outMemPtr   - Pointer to the memory to copy register's content.
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemTigerActiveReadTcbGlobalCounterReg (
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    OUT        GT_U32 * outMemPtr
)
{
    /* output actual value of counter */
    * outMemPtr = * memPtr;

    /* reset register */
    * memPtr = 0;
}


/*******************************************************************************
*  smemTigerActiveReadInterruptsCauseReg
*
* DESCRIPTION:
*      Read interrupts cause registers .
* INPUTS:
*       devObjPtr   - device object PTR.
*       address     - Address for ASIC memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers's specific parameter -
*                     global interrupt bit number.
*
* OUTPUTS:
*       outMemPtr   - Pointer to the memory to copy register's content.
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemTigerActiveReadInterruptsCauseReg (
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    OUT        GT_U32 * outMemPtr
)
{
    *outMemPtr = *memPtr;

    *memPtr = 0;

    return;
}
/*******************************************************************************
*  smemTigerActiveWriteMacInterruptsMaskReg
*
* DESCRIPTION:
*
*   the application changed the value of the interrupts mask register.
*   check if there is waiting interrupt for that.
*
*
* INPUTS:
*    devObjPtr  - device object PTR.
*    address    - Address for ASIC memory.
*    memPtr     - Pointer to the register's memory in the simulation.
*    memSize    - Size of memory to be written
*    param      - Registers's specific parameter.
*    inMemPtr   - Pointer to the memory to get register's content.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void smemTigerActiveWriteMacInterruptsMaskReg (
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
)
{
    SBUF_BUF_ID bufferId;               /* buffer */
    GT_U32 *dataPtr;                    /* pointer to the data in the buffer */
    GT_U32 dataSize;                    /* data size */
    GT_U8  *dataU8Ptr;                  /* pointer to the data in the buffer */

    GT_U32  *regPtr; /* register's entry pointer */
    GT_U32  currCauseReg = 0;
    GT_U32  lastValue = *memPtr;/* last value before the change */

    /* read the cause register --- without clearing it */
    regPtr = smemMemGet(devObjPtr, MISC_INTR_MASK_REG);
    currCauseReg = *regPtr;

    /* update the register value */
    *memPtr = *inMemPtr;

    /* Get buffer -- allocate size for max supported frame size */
    bufferId = sbufAlloc(devObjPtr->bufPool, SBUF_DATA_SIZE_CNS);/*2000*/

    if (bufferId == NULL)
    {
        printf(" smemTigerActiveWriteMacInterruptsMaskReg: "\
                "no buffers to update MAC table\n");
        return;
    }
    /* Get actual data pointer */
    sbufDataGet(bufferId, (GT_U8**)&dataU8Ptr, &dataSize);
    dataPtr = (GT_U32*)dataU8Ptr;

    dataPtr[0] = MISC_INTR_CAUSE_REG;/* address of cause reg */
    dataPtr[1] = MISC_INTR_MASK_REG;/* address of mask reg */
    dataPtr[2] = (1<<16);/* interrupt register */
    dataPtr[3] = currCauseReg;/* current cause reg */
    dataPtr[4] = lastValue;/* mask last value */
    dataPtr[5] = *memPtr;/* mask new value */

    /* set source type of buffer */
    bufferId->srcType = SMAIN_SRC_TYPE_CPU_E;

    /* set message type of buffer */
    bufferId->dataType = SMAIN_INTERRUPTS_MASK_REG_E;

    /* put buffer to queue */
    squeBufPut(devObjPtr->queueId, SIM_CAST_BUFF(bufferId));


}

/*******************************************************************************
*  smemTwistWritePolicyTcam
*
* DESCRIPTION:
*      Handler for write  policy TCAM memory
* INPUTS:
*       devObjPtr       - device object pointer
*       address         - address to be translate to internal TCAM address
*       pceIndex        - PCE index in particular TCAM
*       pattern         - pattern/mask PCE type
*       pceSizeType     - standard/extended PCE size
*       pceValid        - valid/invalid PCE entry
*       tcamNum         - TCAM0/TCAM1 memory space
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void smemTwistWritePolicyTcam
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32 tcamAddress,
    IN         TIGER_PCL_PCE_SIZE_TYPE  pceSizeType,
    IN         GT_U8   pceValid
)
{
    GT_U32  *regPtr;        /* register's entry pointer */
    GT_U32  regAddr0;       /* register's address group 0-2 */
    GT_U32  regAddr3;       /* register's address group 3-5 */
    GT_U32  hiddenRegData;  /* hidden register word include PCE size and valid bits */
    GT_U8   pceSize;        /* Standard or extended PCE size */

    pceSize = (pceSizeType == TIGER_PCL_PCE_SIZE_STANDARD) ? 0 : 1;
    hiddenRegData = (pceValid << 1) | (pceSize << 3);

    regAddr0 = SMEM_TIGER_PCL_TCAM_ENTRY_WORD0_REG(tcamAddress);
    regAddr3 = SMEM_TIGER_PCL_TCAM_ENTRY_WORD3_REG(tcamAddress);

    /* Read stored PCE data */
    regPtr = smemMemGet(devObjPtr, tcamAddress);

    /* Write words 0-2 */
    smemMemSet(devObjPtr, regAddr0, regPtr, 3);
    regPtr += 3;
    /* Write words 3-5 */
    smemMemSet(devObjPtr, regAddr3, regPtr, 3);
    regPtr += 3;
    /* Write hidden words data */
    smemMemSet(devObjPtr, regAddr0 + 0xC, &hiddenRegData, 1);
    smemMemSet(devObjPtr, regAddr3 + 0xC, &hiddenRegData, 1);

    if (pceSizeType == TIGER_PCL_PCE_SIZE_EXTENDED)
    {
        regAddr0 = SMEM_TIGER_PCL_TCAM_ENTRY_WORD0_EXT_REG(tcamAddress);
        regAddr3 = SMEM_TIGER_PCL_TCAM_ENTRY_WORD3_EXT_REG(tcamAddress);

        /* Write words 6-8 */
        smemMemSet(devObjPtr, regAddr0, regPtr, 3);
        regPtr += 3;
        /* Write words 9-11 */
        smemMemSet(devObjPtr, regAddr3, regPtr, 3);

        /* Write hidden words data */
        smemMemSet(devObjPtr, regAddr0 + 0xC, &hiddenRegData, 1);
        smemMemSet(devObjPtr, regAddr3 + 0xC, &hiddenRegData, 1);
    }
}

/*******************************************************************************
*  smemTigerActiveWriteTcamMemory
*
* DESCRIPTION:
*
*    The policy TCAM write access
*
* INPUTS:
*    devObjPtr  - device object PTR.
*    address    - Address for ASIC memory.
*    memPtr     - Pointer to the register's memory in the simulation.
*    memSize    - Size of memory to be written
*    param      - Registers's specific parameter.
*    inMemPtr   - Pointer to the memory to get register's content.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void smemTigerActiveWriteTcamMemory
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
)
{
    TIGER_PCL_PCE_SIZE_TYPE pceSizeType;    /* pce Size Type (standard or extended) */
    GT_U8                   pceValid;       /* pce entry valid bit */
    GT_U32                  fldValue;       /* register's field value */
    TIGER_DEV_MEM_INFO  *   devMemInfoPtr;  /* device memory pointer */

    devMemInfoPtr = (TIGER_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    /* Stored as is to be retrieved in called functions */
    *memPtr = *inMemPtr;

    /* When pattern or mask written of both standard or extended rule       */
    /* written line of 12 words with address base+0, base+4... base+0x2C    */
    /* When invalidating the rule the 12-th word (base+0x2C) written        */
    /* Here saved last value of base                                        */
    devMemInfoPtr->lxMem.tcamAccessAddr = address & (~ 0x3F);

    if ((address & 0x3F) != 0x2C)
    {
        /* the data and the base already stored */
        return;
    }

    /* Read additional PCE info from control register */
    smemRegFldGet(devObjPtr, POLICY_ENGINE_CTRL1_REG, 24, 2, &fldValue);

    pceValid = (GT_U8)(fldValue & 0x1);

    pceSizeType = (fldValue >> 1) ?
        TIGER_PCL_PCE_SIZE_EXTENDED : TIGER_PCL_PCE_SIZE_STANDARD;

    smemTwistWritePolicyTcam(devObjPtr, devMemInfoPtr->lxMem.tcamAccessAddr,
                             pceSizeType, pceValid);

    /* Reset access memory address after write operation */
    devMemInfoPtr->lxMem.tcamAccessAddr = SMEM_TIGER_NOT_VALID_ADDRESS;
}

/*******************************************************************************
*  smemTigerActiveReadTcamMemory
*
* DESCRIPTION:
*      Read TCAM memory.
* INPUTS:
*       devObjPtr   - device object PTR.
*       address     - Address for ASIC memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers's specific parameter -
*                     global interrupt bit number.
*
* OUTPUTS:
*       outMemPtr   - Pointer to the memory to copy register's content.
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemTigerActiveReadTcamMemory
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    OUT        GT_U32 * outMemPtr
)
{
    GT_U32 * regPtr;
    GT_U32 tcamAddress;

    tcamAddress = SMEM_TIGER_PCL_TCAM_ENTRY_WORD0_REG(address);

    regPtr = smemMemGet(devObjPtr, tcamAddress);

    memcpy(outMemPtr, regPtr, memSize * sizeof(GT_U32));
}

