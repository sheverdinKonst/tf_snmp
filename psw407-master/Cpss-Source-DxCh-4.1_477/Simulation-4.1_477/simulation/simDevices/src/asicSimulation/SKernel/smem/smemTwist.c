/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* smemTwist.c
*
* DESCRIPTION:
*       This is API implementation for Twist memories.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 40 $
*
*******************************************************************************/
#include <os/simTypes.h>
#include <asicSimulation/SKernel/smem/smemTwist.h>
#include <asicSimulation/SKernel/twistCommon/sregTwist.h>
#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SKernel/smain/smain.h>
#include <asicSimulation/SKernel/suserframes/snetTwistTrafficCond.h>
#include <asicSimulation/SKernel/suserframes/snet.h>

/* Global external declarations */
extern SKERNEL_USER_DEBUG_INFO_STC skernelUserDebugInfo;

#define SFDB_MAC_TBL_ACT_WORDS  2
#define SFDB_MAC_TBL_ACT_BYTES  (SFDB_MAC_TBL_ACT_WORDS * sizeof(GT_U32))

#define SMEM_TWIST_LINK_FORCE_MSG_SIZE  4

static GT_U32 *  smemTwistFatalError(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemTwistGlobalReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemTwistTransQueReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);

static GT_U32 *  smemTwistEtherBrdgReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemTwistLxReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemTwistBufMngReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemTwistMacReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemTwistPortGroupConfReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemTwistMacTableReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemTwistVlanTableReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static void smemTwistInitFuncArray(
    INOUT TWIST_DEV_MEM_INFO  * devMemInfoPtr
);
static void smemTwistAllocSpecMemory(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    INOUT TWIST_DEV_MEM_INFO  * devMemInfoPtr
);

static void smemTwistResetAuq(
    INOUT TWIST_DEV_MEM_INFO  * devMemInfoPtr
);

static void smemTwistSetPciDefault(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    INOUT TWIST_DEV_MEM_INFO  * devMemInfoPtr
);


static void smemTwistActiveReadPortIntReg (
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    OUT        GT_U32 * outMemPtr
);

static void smemTwistActiveReadBrdgIntReg (
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    OUT        GT_U32 * outMemPtr
);

static void smemTwistActiveReadSdmaIntReg (
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    OUT        GT_U32 * outMemPtr
);

static GT_U32 *  smemTwistExternalMem(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);

static void smemTwistActiveWriteFdbMsg (
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
);

static void smemTwistActiveAuqBaseWrite (
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
);

static void smemTwistActiveReadCntrs (
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    OUT        GT_U32 * outMemPtr
);

static void smemTwistActiveWriteXSmii (
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
);

static void smemTwistActiveWriteSFlowCnt(
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
);

static void smemTwistActiveWriteTcbGenxsReg(
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
);

static void smemTwistActiveReadPortEgressCountersReg(
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    OUT        GT_U32 * outMemPtr
);


static void smemTwistActiveWriteTransmitQueueCmdReg (
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
);

static void smemTwistActivePciWriteIntr (
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
);

static GT_U32 *  smemTwistPciReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);

static void smemMemSizeToBytes(
        IN GT_CHAR * memSizePtr,
        INOUT GT_U32 * memBytesPtr
);

static void smemTwistPhyRegsInit(
    IN SKERNEL_DEVICE_OBJECT    *       deviceObjPtr,
    INOUT TWIST_DEV_MEM_INFO    *       devMemInfoPtr
);

static GT_U32 *  smemTwistFaMem(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);

static void smemTwistActiveReadTcbCounterAlarmReg (
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    OUT        GT_U32 * outMemPtr
);

static void smemTwistActiveReadTcbPolicingCounterAlarmReg (
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    OUT        GT_U32 * outMemPtr
);

static void smemTwistActiveReadTcbGlobalCounterReg (
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    OUT        GT_U32 * outMemPtr
);

static GT_VOID smemTwistNsramConfigUpdate (
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr
);

void smemTwistActiveWriteFdbActionTrigger(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
);

static void smemTwistActiveReadInterruptsCauseReg (
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    OUT        GT_U32 * outMemPtr
);

static void smemTwistActiveWriteMacInterruptsMaskReg (
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
);

static GT_VOID smemTwistBackUpMemory
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_BOOL readWrite
);


void smemTwistActiveWriteMacControl (
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
);


/* Private definition */
#define     GLOB_REGS_NUM               (0x80 / 4 + 1)
#define     TWSI_INT_REGS_NUM           (0x28 / 4 + 1)
#define     SDMA_REGS_NUM               (0x8FF / 4 + 1)
#define     BM_REGS_NUM                 (0x4FFF / 4 + 1)
#define     EPF_REGS_NUM                (0x4FFF / 4 + 1)
#define     EGR_REGS_NUM                (0x28 / 4 + 1)
#define     LX_GEN_REGS_NUM             (0x1FF / 4 + 1)
#define     DSCP_REGS_NUM               (0x3c / 4 + 1)
/* was (0x8FFC / 4) + 1, matches the "Prestera PP reisters */
/* according to working code there are two segments  */
/* from 0x02D40000 and from 0x02D48000 for PCE# %8 0-3 and 4-7 */
#define     PCL_TBL_REGS_NUM            (0xFFFC / 4 + 1)
#define     COS_REGS_NUM                (0x3DC / 4 + 1)
#define     TIGER_TRUNK_TBL_REGS_NUM    (0x2DC / 4 + 1)
#define     TRUNK_TBL_REGS_NUM          (0x1FFC / 4 + 1)
#define     TWIST_C_TRUNK_TBL_REGS_NUM  (0x0A4 / 4 + 1)
#define     COUNT_BLOCK_REGS_NUM        (0x178 / 4 + 1)
#define     IPV4_TC_NUM                 (0xFFFFF / 4 + 1)
#define     MPLS_NUM                    (0x178 / 4 + 1)
#define     EXT_MEM_CONF_NUM            (0x2CF / 4 + 1)
#define     MAC_REGS_NUM                (0xEC / 4 + 1)
#define     MAC_PORTS_NUM               (0x3F)
#define     PER_PORT_TYPES_NUM          (0x04 / 4 + 1)
#define     PER_GROUPS_TYPE_NUM         (0x48)
#define     MAC_CNT_TYPES_NUM           (0xD7C / 4 + 1)
#define     SFLOW_NUM                   (0x0C / 4 + 1)
#define     BRG_PORTS_NUM                (64)
#define     PORT_REGS_GROUPS_NUM        (0x14 / 4 + 1)
#define     PORT_PROT_VID_NUM           (0x20)
#define     BRG_GEN_REGS_NUM            (0xB270 / 4 + 1)
#define     GEN_EGRS_GROUP_NUM          (0x1FFF / 4 + 1)
#define     TC_QUE_DESCR_NUM            (8)
#define     STACK_CFG_REG_NUM           (0x1C / 4 + 1)
#define     BUF_MEM_REGS_NUM            (0x3)
#define     MAC_TBL_REGS_NUM            (16 * 1024 * 4)
#define     MAC_UPD_FIFO_REGS_NUM       (4 * 16)
#define     VLAN_REGS_NUM               (2048)
#define     VLAN_TABLE_REGS_NUM         (8 * 1024 * 4 * 4)
#define     STP_TABLE_REGS_NUM          (32 * 2)
#define     PCI_MEM_REGS_NUM            (0x3C / 4 + 1)
#define     PCI_INTERNAL_REGS_NUM       (0x18 / 4 + 1)
#define     PHY_XAUI_DEV_NUM            (6)
#define     PHY_IEEE_XAUI_REGS_NUM      (0xff * PHY_XAUI_DEV_NUM)
#define     PHY_EXT_XAUI_REGS_NUM       (0xff * PHY_XAUI_DEV_NUM)
#define     TCAM_TBL_REG_NUM            (0xfffc /4+1)
#define     PCE_ACTIONS_TBL_REG_NUM     (0x3ffc /4+1)
#define     COS_LOOKUP_TBL_REG_NUM      (0x24   /4+1)
#define     COS_TABLES_REG_NUM          (0x1ff  /4+1)
#define     DSCP_DP_TO_COS_MARK_REMARK_TABLE_REG_NUM (0x3dc /4+1)
#define     INTERNAL_INLIF_TBL_REG_NUM               (0x360 /4+1)
#define     FLOW_TEMPLATE_HASH_CONFIG_TBL_NUM        (0x3c  /4+1)
#define     FLOW_TEMPLATE_HASH_SELECT_TBL_NUM        (0x3fc /4+1)
#define     FLOW_TEMPLATE_SELECT_TBL_NUM             (0x2ff /4+1)
#define     COUNT_BLOCK_2_REGS_NUM        (0x118 / 4 + 1)
#define     EGR_TRUNK_REGS_NUM                       (48)


/* Register special function index in function array (Bits 23:28)*/
#define     REG_SPEC_FUNC_INDEX         0x1F800000

/* GOP interrup cause registers */
#define     SMEM_TWIST_PORT_INTR_CAUSE0_CNS 0x03800090
#define     SMEM_TWIST_PORT_INTR_CAUSE1_CNS 0x03800098
#define     SMEM_TWIST_PORT_INTR_CAUSE5_CNS 0x038000B8

/* number of words in the FDB update message */
#define     SMEM_TWIST_FDB_UPD_MSG_WORDS_NUM    4

/* MAC Counters address mask */
#define     SMEM_TWIST_COUNT_MSK_CNS        0xfffff000
#define     SMEM_TWIST_GOPINT_MSK_CNS       0xfffffff7
#define     SMEM_TWIST_PORT_EGRESS_COUNTERS_CNS 0xffffefff

#define     SMEM_TWIST_NSRAM_INTERNAL      (32 * 0x400)
#define     SMEM_TWIST_NSRAM_EXTERNAL      (8 * 0x100000)

/* Active memory table */
static SMEM_ACTIVE_MEM_ENTRY_STC smemTwistActiveTable[] =
{
    /* GOP<n> Interrupt Cause Register0*/
    {0x03800090, SMEM_TWIST_GOPINT_MSK_CNS,
                                smemTwistActiveReadPortIntReg,  14, NULL, 0},
    {0x038000A0, SMEM_TWIST_GOPINT_MSK_CNS,
                                smemTwistActiveReadPortIntReg,  14, NULL, 0},
    {0x038000B0, SMEM_TWIST_GOPINT_MSK_CNS,
                                smemTwistActiveReadPortIntReg,  14, NULL, 0},
    {0x038000C0, SMEM_TWIST_GOPINT_MSK_CNS,
                                smemTwistActiveReadPortIntReg,  19, NULL, 0},
    {0x038000D0, SMEM_TWIST_GOPINT_MSK_CNS,
                                smemTwistActiveReadPortIntReg,  19, NULL, 0},
    {0x038000E0, SMEM_TWIST_GOPINT_MSK_CNS,
                                smemTwistActiveReadPortIntReg,  19, NULL, 0},

    {0x00000040, SMEM_FULL_MASK_CNS,
                                smemTwistActiveReadPortIntReg,  16, NULL, 0},

    /* MAC Table Action0 Register */
    {0x02040014, SMEM_FULL_MASK_CNS, NULL, 0 ,
                                smemTwistActiveWriteFdbActionTrigger, 0},

    /* MAC Table Interrupt Cause Register */
    {0x02040130, SMEM_FULL_MASK_CNS,
                                smemTwistActiveReadBrdgIntReg,11, NULL, 0},

    /* SDMA Receive SDMA Interrupt Cause Register (RxSDMAInt) */
    {0x0000280C, SMEM_FULL_MASK_CNS,
                                smemTwistActiveReadSdmaIntReg, 17, NULL, 0},

    /* Message from CPU Register0 */
    {0x00000048, SMEM_FULL_MASK_CNS, NULL, 0 , smemTwistActiveWriteFdbMsg, 0},

    /* AUQ Base address reg 0x14 */
    {0x00000014, SMEM_FULL_MASK_CNS, NULL, 0 , smemTwistActiveAuqBaseWrite, 0},

    /* MAC counters */
    {0x00810000, SMEM_TWIST_COUNT_MSK_CNS, smemTwistActiveReadCntrs, 0, NULL,0},
    {0x01010000, SMEM_TWIST_COUNT_MSK_CNS, smemTwistActiveReadCntrs, 0, NULL,0},

    /* MAC Control Register mask 0x00800000 */
    {0x00800000, SMEM_TWIST_MAC_CTRL_MASK_CNS,
        NULL, 0, smemTwistActiveWriteMacControl, 0},

    /* MAC Control Register mask 0x01000000 */
    {0x01000000, SMEM_TWIST_MAC_CTRL_MASK_CNS,
        NULL, 0, smemTwistActiveWriteMacControl, 1},

    /* 10 G SMII control register */
    {0x03800004, SMEM_FULL_MASK_CNS, NULL, 0 , smemTwistActiveWriteXSmii, 0},

    /* sFlow Value Register */
    {0x03800104, SMEM_FULL_MASK_CNS, NULL, 0 , smemTwistActiveWriteSFlowCnt, 0},

    /* tcbGenxsRegAddr Samba */
    {0x028000C4, SMEM_FULL_MASK_CNS, NULL, 0 , smemTwistActiveWriteTcbGenxsReg, 0},

    /* port egress counters --- read only , reset on read */
    /*1*/    {0x01800E04, SMEM_TWIST_PORT_EGRESS_COUNTERS_CNS,
            smemTwistActiveReadPortEgressCountersReg, 0 , NULL ,0},
    /*2*/    {0x01800E08, SMEM_TWIST_PORT_EGRESS_COUNTERS_CNS,
            smemTwistActiveReadPortEgressCountersReg, 0 , NULL ,0},
    /*3*/    {0x01800E0C, SMEM_TWIST_PORT_EGRESS_COUNTERS_CNS,
            smemTwistActiveReadPortEgressCountersReg, 0 , NULL ,0},
    /*4*/    {0x01800F00, SMEM_TWIST_PORT_EGRESS_COUNTERS_CNS,
            smemTwistActiveReadPortEgressCountersReg, 0 , NULL ,0},
    /*5*/    {0x01800F04, SMEM_TWIST_PORT_EGRESS_COUNTERS_CNS,
            smemTwistActiveReadPortEgressCountersReg, 0 , NULL ,0},

    /* txQCmdReg Register */
    {0x00002868, SMEM_FULL_MASK_CNS, NULL, 0 ,
                                     smemTwistActiveWriteTransmitQueueCmdReg, 0},

    /* TCB count alarm register */
    {0x02800074, (SMEM_FULL_MASK_CNS),
        smemTwistActiveReadTcbCounterAlarmReg, 0 , NULL, 0},

    /* TCB policing count alarm register */
    {0x02800078, (SMEM_FULL_MASK_CNS),
        smemTwistActiveReadTcbPolicingCounterAlarmReg, 0 , NULL, 0},

    /* tcb global counters */
    /* 1 */
    {TCB_GLOBAL_RECEIVED_PACKETS_REG ,(SMEM_FULL_MASK_CNS),
        smemTwistActiveReadTcbGlobalCounterReg, 0 , NULL, 0},
    /* 2 */
    {TCB_GLOBAL_RECEIVED_OCTETS_LOW_REG ,(SMEM_FULL_MASK_CNS),
        smemTwistActiveReadTcbGlobalCounterReg, 0 , NULL, 0},
    /* 3 */
    {TCB_GLOBAL_RECEIVED_OCTETS_HI_REG ,(SMEM_FULL_MASK_CNS),
        smemTwistActiveReadTcbGlobalCounterReg, 0 , NULL, 0},
    /* 4 */
    {TCB_GLOBAL_DROPPED_PACKETS_REG ,(SMEM_FULL_MASK_CNS),
        smemTwistActiveReadTcbGlobalCounterReg, 0 , NULL, 0},
    /* 5 */
    {TCB_GLOBAL_DROPPED_OCTETS_LOW_REG ,(SMEM_FULL_MASK_CNS),
        smemTwistActiveReadTcbGlobalCounterReg, 0 , NULL, 0},
    /* 6 */
    {TCB_GLOBAL_DROPPED_OCTETS_HI_REG ,(SMEM_FULL_MASK_CNS),
        smemTwistActiveReadTcbGlobalCounterReg, 0 , NULL, 0},

    /* read interrupts cause registers -- ROC register */
    {0x00000114, SMEM_FULL_MASK_CNS, smemTwistActiveReadInterruptsCauseReg, 0 , NULL,0},
    {0x00002810, SMEM_FULL_MASK_CNS, smemTwistActiveReadInterruptsCauseReg, 0 , NULL,0},
    {MISC_INTR_MASK_REG,SMEM_FULL_MASK_CNS, NULL, 0 , smemTwistActiveWriteMacInterruptsMaskReg,0},

    /* must be last anyway */
    {0xffffffff, SMEM_FULL_MASK_CNS, NULL,0,NULL,0}
};

#define SMEM_ACTIVE_MEM_TABLE_SIZE \
    (sizeof(smemTwistActiveTable)/sizeof(smemTwistActiveTable[0]))

/* Active memory table */
static SMEM_ACTIVE_MEM_ENTRY_STC smemTwistPciActiveTable[] =
{
    /* PCI interrupt cause */
    {0x00000114, SMEM_FULL_MASK_CNS, NULL, 0 , smemTwistActivePciWriteIntr, 0},

    /* must be last anyway */
    {0xffffffff, SMEM_FULL_MASK_CNS, NULL,0,NULL,0}
};

#define SMEM_ACTIVE_PCI_MEM_TABLE_SIZE \
    (sizeof(smemTwistPciActiveTable)/sizeof(smemTwistPciActiveTable[0]))

/* temporary patch for FA/XBAR familly */
static GT_U32 smemFaMemory[0xff];

/*******************************************************************************
*   smemTwistNsramConfigUpdate
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
static GT_VOID smemTwistNsramConfigUpdate
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr
)
{
    TWIST_DEV_MEM_INFO * devMemInfoPtr; /* device memory info pointer */
    char   param_str[SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS];
                                        /* string for parameter */
    GT_U32 param_val;                   /* parameter value */
    char   keyString[20];               /* key string */
    TWIST_NSRAM_MODE_ENT nSramMode;     /* narrow SRAM configuration */
    GT_U32 nSramSize;                   /* external NSRAM size */

    devMemInfoPtr = (TWIST_DEV_MEM_INFO*)(deviceObjPtr->deviceMemory);

    /* narow sram type */
    nSramSize = SMEM_TWIST_NSRAM_EXTERNAL / sizeof(SMEM_REGISTER);

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

    if(SKERNEL_DEVICE_FAMILY_TIGER(deviceObjPtr->deviceType))
    {
        nSramMode = TWIST_NSRAM_ALL_EXTERNAL_E;

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
        case TWIST_NSRAM_ONE_FOUR_INTERNAL_E:
            devMemInfoPtr->nsramsMem[0].nSramSize = 0;
            devMemInfoPtr->nsramsMem[1].nSramSize = nSramSize;
            devMemInfoPtr->nsramsMem[2].nSramSize = nSramSize;
            devMemInfoPtr->nsramsMem[3].nSramSize = SMEM_TWIST_NSRAM_INTERNAL;
        break;
        case TWIST_NSRAM_ALL_EXTERNAL_E:
            devMemInfoPtr->nsramsMem[0].nSramSize = 0;
            devMemInfoPtr->nsramsMem[1].nSramSize = nSramSize;
            devMemInfoPtr->nsramsMem[2].nSramSize = nSramSize;
            devMemInfoPtr->nsramsMem[3].nSramSize = 0;
            break;
        case TWIST_NSRAM_ONE_TWO_INTERNAL_E:
            devMemInfoPtr->nsramsMem[0].nSramSize = 0;
            devMemInfoPtr->nsramsMem[1].nSramSize = nSramSize;
            devMemInfoPtr->nsramsMem[2].nSramSize = SMEM_TWIST_NSRAM_INTERNAL;
            devMemInfoPtr->nsramsMem[3].nSramSize = 0;
            break;
        case TWIST_NSRAM_THREE_FOUR_INTERNAL_E:
            devMemInfoPtr->nsramsMem[0].nSramSize = 0;
            devMemInfoPtr->nsramsMem[1].nSramSize = nSramSize;
            devMemInfoPtr->nsramsMem[2].nSramSize = SMEM_TWIST_NSRAM_INTERNAL;
            devMemInfoPtr->nsramsMem[3].nSramSize = SMEM_TWIST_NSRAM_INTERNAL;
            break;
        case TWIST_NSRAM_ALL_INTERNAL_E:
            devMemInfoPtr->nsramsMem[0].nSramSize = 0;
            devMemInfoPtr->nsramsMem[1].nSramSize = 0;
            devMemInfoPtr->nsramsMem[2].nSramSize = SMEM_TWIST_NSRAM_INTERNAL;
            devMemInfoPtr->nsramsMem[3].nSramSize = SMEM_TWIST_NSRAM_INTERNAL;
            break;
        default:
            skernelFatalError("smemTwistNsramConfigUpdate: invalid NSRAM mode\n");
        }
    }
    else
    {
        devMemInfoPtr->nsramsMem[0].nSramSize = nSramSize;
    }
}

/*******************************************************************************
*   smemTwistInit
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
void smemTwistInit
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr
)
{
    TWIST_DEV_MEM_INFO  * devMemInfoPtr;

    /* string for parameter */
    char   param_str[SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS];
    /* key string */
    char   keyString[20];

    /* alloc TWIST_DEV_MEM_INFO */
    devMemInfoPtr = (TWIST_DEV_MEM_INFO *)calloc(1, sizeof(TWIST_DEV_MEM_INFO));
    if (devMemInfoPtr == 0)
    {
            skernelFatalError("smemTwistInit: allocation error\n");
    }

    deviceObjPtr->deviceMemory = devMemInfoPtr;

    /* get wide SRAM size from *.ini file */
    sprintf(keyString, "dev%d_wsram", deviceObjPtr->deviceId);
    if (!SIM_OS_MAC(simOsGetCnfValue)("system",  keyString,
                             SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS, param_str))
    {
        /* use internal 128Kb ram */
        devMemInfoPtr->wsramMem.wSramSize = 128 * 1024 / sizeof(SMEM_REGISTER);
    }
    else
    {
        /* get wsram size in bytes */
        smemMemSizeToBytes(param_str, &devMemInfoPtr->wsramMem.wSramSize);
        /* convert to words number */
        devMemInfoPtr->wsramMem.wSramSize /= sizeof(SMEM_REGISTER);
    }

    /* init narrow sram lpm access per ip octets for ipv4 & ipv6 */
    smemTwistNsramConfigUpdate(deviceObjPtr);

    /* get flow DRAM size from *.ini file */
    sprintf(keyString, "dev%d_fdram", deviceObjPtr->deviceId);
    if (!SIM_OS_MAC(simOsGetCnfValue)("system",  keyString,
                             SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS, param_str))
    {
        devMemInfoPtr->fdramMem.fDramSize = 0;
    }
    else
    {
        /* get FDram size in bytes */
                smemMemSizeToBytes(param_str, &devMemInfoPtr->fdramMem.fDramSize);
        /* convert to words number */
        devMemInfoPtr->fdramMem.fDramSize /= sizeof(SMEM_REGISTER);
    }

    /* init specific functions array */
    smemTwistInitFuncArray(devMemInfoPtr);

    /* allocate address type specific memories */
    smemTwistAllocSpecMemory(deviceObjPtr,devMemInfoPtr);

    /* Clear AU structure */
    smemTwistResetAuq(devMemInfoPtr);

    /* Set PCI registers default values */
    smemTwistSetPciDefault(deviceObjPtr, devMemInfoPtr);

    /* init PHY registers memory */
    smemTwistPhyRegsInit(deviceObjPtr, devMemInfoPtr);

    deviceObjPtr->devFindMemFunPtr = (void *)smemTwistFindMem;

    deviceObjPtr->devMemBackUpPtr = smemTwistBackUpMemory;

    /* init FA registers - temporary patch */
    smemFaMemory[(0xAC/4)] = 0x11AB;

}
/*******************************************************************************
*   smemTwistFindMem
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
void * smemTwistFindMem
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32                  address,
    IN GT_U32                  memSize,
    OUT SMEM_ACTIVE_MEM_ENTRY_STC ** activeMemPtrPtr
)
{
    void                * memPtr;
    TWIST_DEV_MEM_INFO  * devMemInfoPtr;
    GT_32               index;
    GT_U32              param;
    GT_32               dramType;

    if (deviceObjPtr == 0)
    {
        skernelFatalError("smemTwistFindMem: illegal pointer \n");
    }
    if (activeMemPtrPtr != NULL)
    {
        *activeMemPtrPtr = NULL;
    }

    memPtr = 0;
    devMemInfoPtr = (TWIST_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;
    /* Find PCI registers memory  */
    if (SMEM_ACCESS_PCI_FULL_MAC(accessType))
    {
        memPtr = smemTwistPciReg(deviceObjPtr, address, memSize, 0);

        /* find PCI active memory entry */
        if (activeMemPtrPtr != NULL)
        {
            for (index = 0; index < (SMEM_ACTIVE_PCI_MEM_TABLE_SIZE - 1);
                  index++)
            {
                /* check address */
                if ((address & smemTwistPciActiveTable[index].mask)
                      == smemTwistPciActiveTable[index].address)
                    *activeMemPtrPtr = &smemTwistPciActiveTable[index];
            }
        }
        return memPtr;
    }

    if ((address & 0x40000000) > 0 )
    {
        /* FA memory */
        memPtr = smemTwistFaMem(deviceObjPtr, address, memSize, 0);
        return memPtr;
    }
    else if ((address & 0x20000000) > 0 ) {
        /* Find external memory */
        dramType = ((address >> 27) & 0x03);

        memPtr = smemTwistExternalMem(deviceObjPtr, address, memSize, dramType);
    }
    else {
        /* Find common registers memory */
        index = (address & REG_SPEC_FUNC_INDEX) >> 23;
        if (index > 63)
        {
            skernelFatalError("smemTwistFindMem: index is out of range\n");
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
            if ((address & smemTwistActiveTable[index].mask)
                  == smemTwistActiveTable[index].address)
                *activeMemPtrPtr = &smemTwistActiveTable[index];
        }
    }

    return memPtr;
}

/*******************************************************************************
*   smemTwistGlobalReg
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
static GT_U32 *  smemTwistGlobalReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    TWIST_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32              * regValPtr;
    GT_U32              index;

    regValPtr = 0;
    devMemInfoPtr = (TWIST_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;
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
*   smemTwistTransQueReg
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
static GT_U32 *  smemTwistTransQueReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    TWIST_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32              * regValPtr;
    GT_U32              port, offset, index;

    regValPtr = 0;
    devMemInfoPtr = (TWIST_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;

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
    /* Trunk registers 0x0180.280 , 0x0181.280 , 0x0182.280 , 0x0183.280
        24 registers for low (8 non-trunk , 16 designated ports )
        24 registers for Hi  (8 non-trunk , 16 designated ports )*/
    if ((address & 0x1E7C0000) == 0x0 && ((address & 0xFFF)==0x280)){
        index = ((address&0x20000)?1:0)*24 + ((address & 0x1F000)>>12);
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

    return regValPtr;
}

/*******************************************************************************
*   smemTwistEtherBrdgReg
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
static GT_U32 *  smemTwistEtherBrdgReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    TWIST_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32              * regValPtr;
    GT_U32              index, port;

    regValPtr = 0;
    devMemInfoPtr = (TWIST_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;
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
*   smemTwistIpV4Reg
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
static GT_U32 *  smemTwistLxReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    TWIST_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32              * regValPtr;
    GT_U32              index;
    GT_BOOL             sambaDev ;

    sambaDev = (deviceObjPtr->deviceFamily == SKERNEL_SAMBA_FAMILY)?
                GT_TRUE:GT_FALSE;

    regValPtr = 0;
    devMemInfoPtr = (TWIST_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;
    /* General LX unit registers */
    if ((address & 0x007f0000) == 0x00000000){ /*0x0280....*/
        index = (address & 0x1FF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->lxMem.genRegs,
                         devMemInfoPtr->lxMem.genRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->lxMem.genRegs[index];
    }
    else
    /* MPLS Processor  Registers array */
    if ((address & 0x007f0000) == 0x00500000){ /*0x02d0....*/
        index = (address & 0xff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->lxMem.mplsRegs,
                         devMemInfoPtr->lxMem.mplsRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->lxMem.mplsRegs[index];
    }
    else
    /* DSCP to CoS Table */
    if ((address & 0x007f0000) == 0x00520000) { /*0x02d2....*/
        index = (address & 0x3ff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->lxMem.dscpRegs,
                         devMemInfoPtr->lxMem.dscpRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->lxMem.dscpRegs[index];
    }
    else
    /* PCL tables registers number */
    if ((address & 0x007f0000) == 0x00540000) { /*0x02d4....*/
        index = (address & 0xffff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->lxMem.pclTblRegs,
                         devMemInfoPtr->lxMem.pclTblRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->lxMem.pclTblRegs[index];
    }
    else
    /* Class of Service Re-marking Table Register */
    if ((address & 0x007f0000) == 0x00560000) { /*0x02d6....*/
        index = (address & 0xffff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->lxMem.cosRegs,
                         devMemInfoPtr->lxMem.cosRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->lxMem.cosRegs[index];
    }
    else
    /* Trunk Member Table Register number */
    if ((address & 0x007f0000) == 0x00580000) { /*0x02d8....*/
        index = (address & 0xffff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->lxMem.trunkRegs,
                         devMemInfoPtr->lxMem.trunkRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->lxMem.trunkRegs[index];
    }
    else
    /* Counter block registers registers */
    if ((address & 0x007f0000) == 0x004C0000) { /*0x02cc....*/
        index = (address & 0xffff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->lxMem.countRegs,
                         devMemInfoPtr->lxMem.countRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->lxMem.countRegs[index];
    }
    /* cos Lookup Table registers */
    else if ((address & 0x007f0000) == 0x00400000) { /*0x02c0....*/
        index = (address & 0xffff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->lxMem.cosLookupTblReg,
                         devMemInfoPtr->lxMem.cosLookupTblRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->lxMem.cosLookupTblReg[index];
    }
    /* cos Tables registers */
    else if ((address & 0x007f0000) == 0x00480000) { /*0x02c8....*/
        index = (address & 0xffff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->lxMem.cosTablesReg,
                         devMemInfoPtr->lxMem.cosTablesRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->lxMem.cosTablesReg[index];
    }
    /*  dscp and Dp to Cos Mark and Remark Table registers */
    else if ((address & 0x007f0000) == 0x004a0000) { /*0x02ca....*/
        index = (address & 0xffff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->lxMem.dscpDptoCosMarkRemarkTableReg,
                         devMemInfoPtr->lxMem.dscpDptoCosMarkRemarkTableRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->lxMem.dscpDptoCosMarkRemarkTableReg[index];
    }
    /*   internal Inlif table registers */
    else if ((address & 0x007f0000) == 0x00460000) { /*0x02c6....*/
        index = (address & 0xffff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->lxMem.internalInlifTblReg,
                         devMemInfoPtr->lxMem.internalInlifTblRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->lxMem.internalInlifTblReg[index];
    }
    /*  flow Template Hash Config table registers */
    else if ((address & 0x007f8000) == 0x00428000) { /*0x02c28...*/
        index = (address & 0x7fff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->lxMem.flowTemplateHashConfigTbl,
                         devMemInfoPtr->lxMem.flowTemplateHashConfigTblNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->lxMem.flowTemplateHashConfigTbl[index];
    }
    /*  flow Template Hash select table registers */
    else if ((address & 0x007f8000) == 0x00420000) { /*0x02c20...*/
        index = (address & 0x7fff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->lxMem.flowTemplateHashSelectTbl,
                         devMemInfoPtr->lxMem.flowTemplateHashSelectTblNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->lxMem.flowTemplateHashSelectTbl[index];
    }
    /*  flow Template select table registers */
    else if ((address & 0x007f8000) == 0x00440000) { /*0x02c4....*/
        index = (address & 0xffff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->lxMem.flowTemplateSelectTbl,
                         devMemInfoPtr->lxMem.flowTemplateSelectTblNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->lxMem.flowTemplateSelectTbl[index];
    }
    /* Counter block 2 registers registers */
    if ((address & 0x007f0000) == 0x004e0000) { /*0x02ce....*/
        index = (address & 0xffff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->lxMem.count2Regs,
                         devMemInfoPtr->lxMem.count2RegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->lxMem.count2Regs[index];
    }
    else
    /* External Memory configuration registers */
    if ((address & 0x007f0000) == 0x00600000) { /*0x02e0....*/
        index = (address & 0xffff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->lxMem.extMemRegs,
                         devMemInfoPtr->lxMem.extMemRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->lxMem.extMemRegs[index];
    }
    else if(sambaDev && ((address & 0x007f0000) == 0x00620000)) /*0x02e2....*/
    {
        /* tcam data memory */
        index = (address & 0xffff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->lxMem.tcamTblReg,
                         devMemInfoPtr->lxMem.tcamTblRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->lxMem.tcamTblReg[index];
    }
    else if(sambaDev && ((address & 0x007f4000) == 0x00634000)) /*0x02e34...*/
    {
        /* pce table memory */
        index = (address & 0x3fff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->lxMem.pceActionsTblReg,
                         devMemInfoPtr->lxMem.pceActionsTblRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->lxMem.pceActionsTblReg[index];
    }

    return regValPtr;
}

/*******************************************************************************
*   smemTwistBufMngReg
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
static GT_U32 *  smemTwistBufMngReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    TWIST_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32              * regValPtr;
    GT_U32              index;

    regValPtr = 0;
    devMemInfoPtr = (TWIST_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;
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
    /* Control Linked List buffers managment registers array */
    if ((address & 0x1CF0F000) == 0x00004000) {
        index = (address & 0xFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->bufMngMem.egrRegs,
                         devMemInfoPtr->bufMngMem.egrRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->bufMngMem.egrRegs[index];
    }
    else if((address & 0x03fff000) == 0x03000000)
    {
        index = (address & 0x00000fff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->bufMngMem.trunkRegs,
                         devMemInfoPtr->bufMngMem.trunkRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->bufMngMem.trunkRegs[index];
    }

    return regValPtr;
}

/*******************************************************************************
*   smemTwistPciReg
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
static GT_U32 *  smemTwistPciReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    TWIST_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32              * regValPtr;
    GT_U32              index;

    regValPtr = 0;
    devMemInfoPtr = (TWIST_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;
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
*   smemTwistMacReg
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
static GT_U32 *  smemTwistMacReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    TWIST_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32              * regValPtr;
    GT_U32              index;

    regValPtr = 0;
    devMemInfoPtr = (TWIST_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;
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
*   smemTwistPortGroupConfReg
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
static GT_U32 *  smemTwistPortGroupConfReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    TWIST_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32              * regValPtr;
    GT_U32              index;

    regValPtr = 0;
    devMemInfoPtr = (TWIST_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;
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
*   smemTwistMacTableReg
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
static GT_U32 *  smemTwistMacTableReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    TWIST_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32              * regValPtr;
    GT_U32              index;

    regValPtr = 0;
    devMemInfoPtr = (TWIST_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;
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
*   smemTwistVlanTableReg
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
static GT_U32 *  smemTwistVlanTableReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    TWIST_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32              * regValPtr;
    GT_U32              index;

    regValPtr = 0;
    devMemInfoPtr = (TWIST_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;
    /* Vlan Table Registers */
    index = (address & 0xFFFFF) / 0x4;
    CHECK_MEM_BOUNDS(devMemInfoPtr->vlanTblMem.vlanTblRegs,
                     devMemInfoPtr->vlanTblMem.vlanTblRegNum,
                     index, memSize);
    regValPtr = &devMemInfoPtr->vlanTblMem.vlanTblRegs[index];


    return regValPtr;
}

static GT_U32 *  smemTwistFatalError(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    if (skernelUserDebugInfo.disableFatalError == GT_FALSE)
        skernelFatalError("smemTwistFatalError: illegal function pointer\n");

    return 0;
}
/*******************************************************************************
*   smemTwistInitMemArray
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
static void smemTwistInitFuncArray(
    INOUT TWIST_DEV_MEM_INFO  * devMemInfoPtr
)
{
    GT_U32              i;

    for (i = 0; i < 64; i++) {
        devMemInfoPtr->specFunTbl[i].specFun    = smemTwistFatalError;
    }
    devMemInfoPtr->specFunTbl[0].specFun        = smemTwistGlobalReg;

    devMemInfoPtr->specFunTbl[1].specFun        = smemTwistPortGroupConfReg;
    devMemInfoPtr->specFunTbl[1].specParam      = 0;

    devMemInfoPtr->specFunTbl[2].specFun        = smemTwistPortGroupConfReg;
    devMemInfoPtr->specFunTbl[2].specParam      = 1;

    devMemInfoPtr->specFunTbl[3].specFun        = smemTwistTransQueReg;

    devMemInfoPtr->specFunTbl[4].specFun        = smemTwistEtherBrdgReg;

    devMemInfoPtr->specFunTbl[5].specFun        = smemTwistLxReg;

    devMemInfoPtr->specFunTbl[6].specFun        = smemTwistBufMngReg;

    devMemInfoPtr->specFunTbl[7].specFun        = smemTwistMacReg;

    devMemInfoPtr->specFunTbl[12].specFun       = smemTwistMacTableReg;

    devMemInfoPtr->specFunTbl[20].specFun       = smemTwistVlanTableReg;
}
/*******************************************************************************
*   smemTwistAllocSpecMemory
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
static void smemTwistAllocSpecMemory(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    INOUT TWIST_DEV_MEM_INFO  * devMemInfoPtr
)
{
    int i;                      /* index of memory array */

    /* Global register memory allocation */
    devMemInfoPtr->globalMem.globRegsNum = GLOB_REGS_NUM;
    devMemInfoPtr->globalMem.globRegs =
            calloc(GLOB_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->globalMem.globRegs == 0)
    {
        skernelFatalError("smemTwistAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->globalMem.twsiIntRegsNum = TWSI_INT_REGS_NUM;
    devMemInfoPtr->globalMem.twsiIntRegs =
        calloc(TWSI_INT_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->globalMem.twsiIntRegs == 0)
    {
        skernelFatalError("smemTwistAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->globalMem.sdmaRegsNum = SDMA_REGS_NUM;
    devMemInfoPtr->globalMem.sdmaRegs =
        calloc(SDMA_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->globalMem.sdmaRegs == 0)
    {
        skernelFatalError("smemTwistAllocSpecMemory: allocation error\n");
    }
    devMemInfoPtr->globalMem.currentDescrPtr = 0;

    /* Buffer management register memory allocation */
    devMemInfoPtr->bufMngMem.bmRegsNum = BM_REGS_NUM;
    devMemInfoPtr->bufMngMem.bmRegs =
        calloc(BM_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->bufMngMem.bmRegs == 0)
    {
        skernelFatalError("smemTwistAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->bufMngMem.epfRegsNum = EPF_REGS_NUM;
    devMemInfoPtr->bufMngMem.epfRegs =
        calloc(EPF_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->bufMngMem.epfRegs == 0)
    {
        skernelFatalError("smemTwistAllocSpecMemory: allocation error\n");
    }

    /* Egress Mirrored Ports registers memory allocation */
    devMemInfoPtr->bufMngMem.egrRegsNum = EGR_REGS_NUM;
    devMemInfoPtr->bufMngMem.egrRegs =
        calloc(EGR_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->bufMngMem.egrRegs == 0)
    {
        skernelFatalError("smemTwistAllocSpecMemory: allocation error\n");
    }

    if(SKERNEL_DEVICE_FAMILY_TIGER(deviceObjPtr->deviceType))
    {
        devMemInfoPtr->bufMngMem.trunkRegsNum = TIGER_TRUNK_TBL_REGS_NUM;
    }
    else
    {
        devMemInfoPtr->bufMngMem.trunkRegsNum = TWIST_C_TRUNK_TBL_REGS_NUM;
    }
    devMemInfoPtr->bufMngMem.trunkRegs =
        calloc(devMemInfoPtr->bufMngMem.trunkRegsNum, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->bufMngMem.trunkRegs == 0)
    {
        skernelFatalError("smemTwistAllocSpecMemory: allocation error\n");
    }


    /*  IPV4 General registers memory allocation */
    devMemInfoPtr->lxMem.genRegsNum = LX_GEN_REGS_NUM;
    devMemInfoPtr->lxMem.genRegs =
        calloc(LX_GEN_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->lxMem.genRegs == 0)
    {
        skernelFatalError("smemTwistAllocSpecMemory: allocation error\n");
    }
    /* MPLS Processor Registers array */
    devMemInfoPtr->lxMem.mplsRegsNum = MPLS_NUM;
    devMemInfoPtr->lxMem.mplsRegs =
        calloc(MPLS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->lxMem.mplsRegs == 0)
    {
        skernelFatalError("smemTwistAllocSpecMemory: allocation error\n");
    }

    /* DSCP registers memory allocation */
    devMemInfoPtr->lxMem.dscpRegsNum = DSCP_REGS_NUM;
    devMemInfoPtr->lxMem.dscpRegs =
        calloc(DSCP_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->lxMem.dscpRegs == 0)
    {
        skernelFatalError("smemTwistAllocSpecMemory: allocation error\n");
    }
    /* PCL table Registers  */
    devMemInfoPtr->lxMem.pclTblRegsNum = PCL_TBL_REGS_NUM;
    devMemInfoPtr->lxMem.pclTblRegs =
        calloc(PCL_TBL_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->lxMem.pclTblRegs == 0)
    {
        skernelFatalError("smemTwistAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->lxMem.tcamTblRegNum = TCAM_TBL_REG_NUM;
    devMemInfoPtr->lxMem.tcamTblReg =
        calloc(TCAM_TBL_REG_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->lxMem.tcamTblReg == 0)
    {
        skernelFatalError("smemTwistAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->lxMem.pceActionsTblRegNum = PCE_ACTIONS_TBL_REG_NUM;
    devMemInfoPtr->lxMem.pceActionsTblReg =
        calloc(PCE_ACTIONS_TBL_REG_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->lxMem.pceActionsTblReg == 0)
    {
        skernelFatalError("smemTwistAllocSpecMemory: allocation error\n");
    }

    /* Class of Service Re-marking Table Register number */
    devMemInfoPtr->lxMem.cosRegsNum = COS_REGS_NUM;
    devMemInfoPtr->lxMem.cosRegs =
        calloc(COS_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->lxMem.cosRegs == 0)
    {
        skernelFatalError("smemTwistAllocSpecMemory: allocation error\n");
    }

    /* Trunk Member Table Register array */
    devMemInfoPtr->lxMem.trunkRegsNum = TRUNK_TBL_REGS_NUM;
    devMemInfoPtr->lxMem.trunkRegs =
        calloc(TRUNK_TBL_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->lxMem.trunkRegs == 0)
    {
        skernelFatalError("smemTwistAllocSpecMemory: allocation error\n");
    }

    /* Counter block registers memory allocation */
    devMemInfoPtr->lxMem.countRegsNum = COUNT_BLOCK_REGS_NUM;
    devMemInfoPtr->lxMem.countRegs =
        calloc(COUNT_BLOCK_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->lxMem.countRegs == 0)
    {
        skernelFatalError("smemTwistAllocSpecMemory: allocation error\n");
    }

    /* Counter block 2 registers memory allocation */
    devMemInfoPtr->lxMem.count2RegsNum = COUNT_BLOCK_2_REGS_NUM;
    devMemInfoPtr->lxMem.count2Regs =
        calloc(COUNT_BLOCK_2_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->lxMem.count2Regs == 0)
    {
        skernelFatalError("smemTwistAllocSpecMemory: allocation error\n");
    }

    /* cos lookup tbl reg */
    devMemInfoPtr->lxMem.cosLookupTblRegNum = COS_LOOKUP_TBL_REG_NUM;
    devMemInfoPtr->lxMem.cosLookupTblReg =
        calloc(COS_LOOKUP_TBL_REG_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->lxMem.cosLookupTblReg == 0)
    {
        skernelFatalError("smemTwistAllocSpecMemory: allocation error\n");
    }

    /* cos tables reg*/
    devMemInfoPtr->lxMem.cosTablesRegNum = COS_TABLES_REG_NUM;
    devMemInfoPtr->lxMem.cosTablesReg =
        calloc(COS_TABLES_REG_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->lxMem.cosTablesReg == 0)
    {
        skernelFatalError("smemTwistAllocSpecMemory: allocation error\n");
    }

    /* dscp dp to cos mark remark table reg */
    devMemInfoPtr->lxMem.dscpDptoCosMarkRemarkTableRegNum = DSCP_DP_TO_COS_MARK_REMARK_TABLE_REG_NUM;
    devMemInfoPtr->lxMem.dscpDptoCosMarkRemarkTableReg =
        calloc(DSCP_DP_TO_COS_MARK_REMARK_TABLE_REG_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->lxMem.dscpDptoCosMarkRemarkTableReg == 0)
    {
        skernelFatalError("smemTwistAllocSpecMemory: allocation error\n");
    }

    /* internal inlif tbl reg*/
    devMemInfoPtr->lxMem.internalInlifTblRegNum = INTERNAL_INLIF_TBL_REG_NUM ;
    devMemInfoPtr->lxMem.internalInlifTblReg =
        calloc(INTERNAL_INLIF_TBL_REG_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->lxMem.internalInlifTblReg == 0)
    {
        skernelFatalError("smemTwistAllocSpecMemory: allocation error\n");
    }

    /* flow template hash config tbl*/
    devMemInfoPtr->lxMem.flowTemplateHashConfigTblNum = FLOW_TEMPLATE_HASH_CONFIG_TBL_NUM;
    devMemInfoPtr->lxMem.flowTemplateHashConfigTbl =
        calloc(FLOW_TEMPLATE_HASH_CONFIG_TBL_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->lxMem.flowTemplateHashConfigTbl == 0)
    {
        skernelFatalError("smemTwistAllocSpecMemory: allocation error\n");
    }

    /* flow template hash select tbl */
    devMemInfoPtr->lxMem.flowTemplateHashSelectTblNum = FLOW_TEMPLATE_HASH_SELECT_TBL_NUM;
    devMemInfoPtr->lxMem.flowTemplateHashSelectTbl =
        calloc(FLOW_TEMPLATE_HASH_SELECT_TBL_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->lxMem.flowTemplateHashSelectTbl == 0)
    {
        skernelFatalError("smemTwistAllocSpecMemory: allocation error\n");
    }

    /* flow template select tbl */
    devMemInfoPtr->lxMem.flowTemplateSelectTblNum = FLOW_TEMPLATE_SELECT_TBL_NUM;
    devMemInfoPtr->lxMem.flowTemplateSelectTbl =
        calloc(FLOW_TEMPLATE_SELECT_TBL_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->lxMem.flowTemplateSelectTbl == 0)
    {
        skernelFatalError("smemTwistAllocSpecMemory: allocation error\n");
    }


    devMemInfoPtr->lxMem.extMemRegsNum = EXT_MEM_CONF_NUM;
    devMemInfoPtr->lxMem.extMemRegs =
        calloc(EXT_MEM_CONF_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->lxMem.extMemRegs == 0)
    {
        skernelFatalError("smemTwistAllocSpecMemory: allocation error\n");
    }

    /* Serial management interface registers memory allocation */
    devMemInfoPtr->macMem.genRegsNum = MAC_REGS_NUM;
    devMemInfoPtr->macMem.genRegs =
        calloc(MAC_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->macMem.genRegs == 0)
    {
        skernelFatalError("smemTwistAllocSpecMemory: allocation error\n");
    }

    /* Network ports register memory allocation */
    devMemInfoPtr->portMem.perPortCtrlRegsNum = MAC_PORTS_NUM;
    devMemInfoPtr->portMem.perPortCtrlRegs =
        calloc(MAC_PORTS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->portMem.perPortCtrlRegs == 0)
    {
        skernelFatalError("smemTwistAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->portMem.perPortStatRegsNum = MAC_PORTS_NUM;
    devMemInfoPtr->portMem.perPortStatRegs =
        calloc(MAC_PORTS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->portMem.perPortStatRegs == 0)
    {
        skernelFatalError("smemTwistAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->portMem.perGroupRegsNum = PER_GROUPS_TYPE_NUM;
    devMemInfoPtr->portMem.perGroupRegs =
        calloc(MAC_PORTS_NUM * PER_GROUPS_TYPE_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->portMem.perGroupRegs == 0)
    {
        skernelFatalError("smemTwistAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->portMem.macCntsNum = MAC_CNT_TYPES_NUM;
    devMemInfoPtr->portMem.macCnts =
        calloc(MAC_CNT_TYPES_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->portMem.macCnts == 0)
    {
        skernelFatalError("smemTwistAllocSpecMemory: allocation error\n");
    }
    devMemInfoPtr->macMem.sflowRegsNum = SFLOW_NUM;
    devMemInfoPtr->macMem.sflowRegs = calloc(SFLOW_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->macMem.sflowRegs == 0)
    {
        skernelFatalError("smemTwistAllocSpecMemory: allocation error\n");
    }
    memset(devMemInfoPtr->macMem.sflowFifo, 0,
             TWIST_SFLOW_FIFO_SIZE * sizeof(GT_U32));

    /* Bridge configuration register memory allocation */
    devMemInfoPtr->bridgeMngMem.portRegsNum =
            BRG_PORTS_NUM * PORT_REGS_GROUPS_NUM;
    devMemInfoPtr->bridgeMngMem.portRegs =
        calloc(BRG_PORTS_NUM * PORT_REGS_GROUPS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->bridgeMngMem.portRegs == 0)
    {
        skernelFatalError("smemTwistAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->bridgeMngMem.portProtVidRegsNum =
            BRG_PORTS_NUM * PORT_PROT_VID_NUM;
    devMemInfoPtr->bridgeMngMem.portProtVidRegs =
        calloc(BRG_PORTS_NUM * PORT_PROT_VID_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->bridgeMngMem.portProtVidRegs == 0)
    {
        skernelFatalError("smemTwistAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->bridgeMngMem.genRegsNum = BRG_GEN_REGS_NUM;
    devMemInfoPtr->bridgeMngMem.genRegs =
        calloc(BRG_GEN_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->bridgeMngMem.genRegs == 0)
    {
        skernelFatalError("smemTwistAllocSpecMemory: allocation error\n");
    }

    /* Egress configuration register memory allocation */
    devMemInfoPtr->egressMem.genRegsNum = GEN_EGRS_GROUP_NUM;
    devMemInfoPtr->egressMem.genRegs =
        calloc(GEN_EGRS_GROUP_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->egressMem.genRegs == 0)
    {
        skernelFatalError("smemTwistAllocSpecMemory: allocation error\n");
    }
    /* Port Traffic Class Descriptor Limit Register */
    devMemInfoPtr->egressMem.tcRegsNum = TC_QUE_DESCR_NUM * MAC_PORTS_NUM;
    devMemInfoPtr->egressMem.tcRegs =
        calloc(TC_QUE_DESCR_NUM * MAC_PORTS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->egressMem.tcRegs == 0)
    {
        skernelFatalError("smemTwistAllocSpecMemory: allocation error\n");
    }
    /* Port Transmit Configuration Register */
    devMemInfoPtr->egressMem.transConfRegsNum = MAC_PORTS_NUM;
    devMemInfoPtr->egressMem.transConfRegs =
        calloc(MAC_PORTS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->egressMem.transConfRegs == 0)
    {
        skernelFatalError("smemTwistAllocSpecMemory: allocation error\n");
    }

    /* Port Descriptor Limit Register */
    devMemInfoPtr->egressMem.descLimRegsNum = MAC_PORTS_NUM;
    devMemInfoPtr->egressMem.descLimRegs =
        calloc(MAC_PORTS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->egressMem.descLimRegs == 0)
    {
        skernelFatalError("smemTwistAllocSpecMemory: allocation error\n");
    }

    /* External memory configuration registers */
    devMemInfoPtr->egressMem.extMemRegsNum = EXT_MEM_CONF_NUM;
    devMemInfoPtr->egressMem.extMemRegs =
        calloc(EXT_MEM_CONF_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->egressMem.extMemRegs == 0)
    {
        skernelFatalError("smemTwistAllocSpecMemory: allocation error\n");
    }

    /* trunk non-trunk table and designated trunk ports registers */
    devMemInfoPtr->egressMem.trunkRegsNum = EGR_TRUNK_REGS_NUM;
    devMemInfoPtr->egressMem.trunkMemRegs =
        calloc(EGR_TRUNK_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->egressMem.trunkMemRegs == 0)
    {
        skernelFatalError("smemTwistAllocSpecMemory: allocation error\n");
    }

    /* MAC MIB counters register memory allocation */
    devMemInfoPtr->macFbdMem.macTblRegsNum = MAC_TBL_REGS_NUM;
    devMemInfoPtr->macFbdMem.macTblRegs =
        calloc(MAC_TBL_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->macFbdMem.macTblRegs == 0)
    {
        skernelFatalError("smemTwistAllocSpecMemory: allocation error\n");
    }

    /* VLAN translation table register memory allocation */
    devMemInfoPtr->vlanTblMem.vlanTblRegNum = VLAN_TABLE_REGS_NUM;
    devMemInfoPtr->vlanTblMem.vlanTblRegs =
        calloc(VLAN_TABLE_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->vlanTblMem.vlanTblRegs == 0)
    {
        skernelFatalError("smemTwistAllocSpecMemory: allocation error\n");
    }

    /* External memory allocation */
    devMemInfoPtr->wsramMem.wSram =
        calloc(devMemInfoPtr->wsramMem.wSramSize, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->wsramMem.wSram == 0)
    {
        skernelFatalError("smemTwistAllocSpecMemory: allocation error\n");
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
            skernelFatalError("smemTwistAllocSpecMemory: allocation error\n");
        }
    }

    if (devMemInfoPtr->fdramMem.fDramSize)
    {
        devMemInfoPtr->fdramMem.fDram =
            calloc(devMemInfoPtr->fdramMem.fDramSize, sizeof(SMEM_REGISTER));
        if (devMemInfoPtr->fdramMem.fDram == 0)
        {
            skernelFatalError("smemTwistAllocSpecMemory: allocation error\n");
        }
    }

    /* PCI memory allocation */
    devMemInfoPtr->pciMem.pciRegsNum = PCI_MEM_REGS_NUM;
    devMemInfoPtr->pciMem.pciRegsRegs =
        calloc(PCI_MEM_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->pciMem.pciRegsRegs == 0)
    {
        skernelFatalError("smemTwistAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->pciMem.pciInterRegsNum = PCI_INTERNAL_REGS_NUM;
    devMemInfoPtr->pciMem.pciInterRegs =
        calloc(PCI_INTERNAL_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->pciMem.pciInterRegs == 0)
    {
        skernelFatalError("smemTwistAllocSpecMemory: allocation error\n");
    }
}

/*******************************************************************************
*  smemTwistResetAuq
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
static void smemTwistResetAuq(
    INOUT TWIST_DEV_MEM_INFO  * devMemInfoPtr
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
*  smemTwistSetPciDefault
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
static void smemTwistSetPciDefault(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    INOUT TWIST_DEV_MEM_INFO  * devMemInfoPtr
)
{
    GT_U32              index;
    GT_U32              revision = 0;

    /* Device and Vendor ID */
    index = 0;

    devMemInfoPtr->pciMem.pciRegsRegs[index] = devObjPtr->deviceType;
    switch(devObjPtr->deviceFamily)
    {
    case SKERNEL_TWIST_D_FAMILY:
        revision = 0x02000001;
    break;
    case SKERNEL_TWIST_C_FAMILY:
        revision = 0x02000003;
    break;
    case SKERNEL_SAMBA_FAMILY:
        revision = 0x02000000;
    break;
    case SKERNEL_TIGER_FAMILY:
        revision = 0x02000000;
    break;

    default:
        skernelFatalError(" smemTwistSetPciDefault: not valid mode[%d]",
                                devObjPtr->deviceFamily);
    break;
    }
    /* Class Code and Revision ID */
    index = 0x08 / 4;
    devMemInfoPtr->pciMem.pciRegsRegs[index] = revision;

    /* Base address */
    index = 0x14 / 4;
    devMemInfoPtr->pciMem.pciRegsRegs[index] = devObjPtr->deviceHwId;
}
/*******************************************************************************
*  smemTwistActiveReadPortIntReg
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
static void smemTwistActiveReadPortIntReg (
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

    regValPtr = smemTwistPciReg(deviceObjPtr, PCI_INT_CAUSE_REG, 1, 0);
    /* check enother causes existence */
    if ((*regValPtr & 0xfffffffe) == 0)
    {
        /* clear if not any cause of interrupt */
        *regValPtr = 0;
    }
}
/*******************************************************************************
*  smemTwistActiveReadBrdgIntReg
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
static void smemTwistActiveReadBrdgIntReg (
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

    regValPtr = smemTwistPciReg(deviceObjPtr, PCI_INT_CAUSE_REG, 1, 0);
    /* check enother causes existence */
    if ((*regValPtr & 0xfffffffe) == 0)
    {
        /* clear if not any cause of interrupt */
        *regValPtr = 0;
    }
}

/*******************************************************************************
*  smemTwistActiveReadSdmaIntReg
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
static void smemTwistActiveReadSdmaIntReg (
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

    regValPtr = smemTwistPciReg(deviceObjPtr, PCI_INT_CAUSE_REG, 1, 0);
    /* check another causes existence */
    if ((*regValPtr & 0xfffffffe) == 0)
    {
        /* clear if not any cause of interrupt */
        *regValPtr = 0;
    }
}
/*******************************************************************************
*  smemTwistActiveWriteFdbMsg
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
static void smemTwistActiveWriteFdbMsg (
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
                SMEM_TWIST_FDB_UPD_MSG_WORDS_NUM * sizeof(GT_U32));

    if ( bufferId == NULL )
    {
        printf(" smemTwistActiveWriteFdbMsg: "\
                "no buffers to update MAC table\n");
        return;
    }

    /* get actual data pointer */
    sbufDataGet(bufferId, &data_PTR, &data_size);

    /* find location of Message from CPU Register0 Offset: 0x48 */
    msgFromCpu0Addr = memPtr;

    /* copy fdb update message to buffer */
    memcpy(data_PTR, (GT_U8_PTR)msgFromCpu0Addr,
            SMEM_TWIST_FDB_UPD_MSG_WORDS_NUM * sizeof(GT_U32));

    /* set source type of buffer */
    bufferId->srcType = SMAIN_SRC_TYPE_CPU_E;

    /* set message type of buffer */
    bufferId->dataType = SMAIN_MSG_TYPE_FDB_UPDATE_E;

    /* put buffer to queue */
    squeBufPut(deviceObjPtr->queueId, SIM_CAST_BUFF(bufferId));

}

/*******************************************************************************
*  smemTwistActiveAuqBaseWrite
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
static void smemTwistActiveAuqBaseWrite (
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
)
{
    TWIST_DEV_MEM_INFO  * devMemInfoPtr;

    *memPtr = *inMemPtr;

    devMemInfoPtr = (TWIST_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    /* First time */
    if(devMemInfoPtr->auqMem.baseInit ==  GT_FALSE) {
        devMemInfoPtr->auqMem.auqBase = *inMemPtr;
        devMemInfoPtr->auqMem.baseInit = GT_TRUE;
        devMemInfoPtr->auqMem.auqBaseValid = GT_TRUE;

        /* Work around for HSU memory dump */
        smemRegFldGet(devObjPtr, 0x8, 16, 15,
                      &devMemInfoPtr->auqMem.auqOffset);
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
*  smemTwistExternalMem
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
static GT_U32 *  smemTwistExternalMem(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    TWIST_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32              * regValPtr;
    GT_U32              index;
    regValPtr = 0;
    devMemInfoPtr = (TWIST_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;

    index = (address & 0x7FFFFFF) / 0x4;

    if(SKERNEL_DEVICE_FAMILY_TIGER(deviceObjPtr->deviceType))
    {
        CHECK_MEM_BOUNDS(devMemInfoPtr->nsramsMem[param].nSram,
                         devMemInfoPtr->nsramsMem[param].nSramSize,
                         index, memSize);
        return &devMemInfoPtr->nsramsMem[param].nSram[index];
    }

    switch(param) {
        case 1: /* 0x28000000 */
            CHECK_MEM_BOUNDS(devMemInfoPtr->wsramMem.wSram,
                             devMemInfoPtr->wsramMem.wSramSize,
                             index, memSize);
            regValPtr = &devMemInfoPtr->wsramMem.wSram[index];
        break;
        case 2: /* 0x30000000 */
            CHECK_MEM_BOUNDS(devMemInfoPtr->nsramsMem[0].nSram,
                             devMemInfoPtr->nsramsMem[0].nSramSize,
                             index, memSize);
            regValPtr = &devMemInfoPtr->nsramsMem[0].nSram[index];
        break;
        case 3: /* 0x38000000 */
            CHECK_MEM_BOUNDS(devMemInfoPtr->fdramMem.fDram,
                             devMemInfoPtr->fdramMem.fDramSize,
                             index, memSize);
            regValPtr = &devMemInfoPtr->fdramMem.fDram[index];
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
*  smemTwistActiveReadCntrs
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
static void smemTwistActiveReadCntrs (
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
*  smemTwistPhyRegsInit
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
static void smemTwistPhyRegsInit(
    IN SKERNEL_DEVICE_OBJECT    *       deviceObjPtr,
    INOUT TWIST_DEV_MEM_INFO    *       devMemInfoPtr
)
{
    if (deviceObjPtr->deviceType == SKERNEL_98EX130D ||
        deviceObjPtr->deviceType == SKERNEL_98EX135D ||
        deviceObjPtr->deviceType == SKERNEL_98MX635D)
    {
        /* init XAUI (10GB port) phy related register */
        devMemInfoPtr->phyMem.ieeeXauiRegsNum = PHY_IEEE_XAUI_REGS_NUM;
        devMemInfoPtr->phyMem.ieeeXauiRegs =
                            calloc(devMemInfoPtr->phyMem.ieeeXauiRegsNum,
                                                sizeof(SMEM_PHY_REGISTER));

        if (devMemInfoPtr->phyMem.ieeeXauiRegs == 0)
        {
            skernelFatalError("smemTwistPhyRegsInit: ieee allocation error\n");
        }

        devMemInfoPtr->phyMem.extXauiRegsNum = PHY_EXT_XAUI_REGS_NUM;
        devMemInfoPtr->phyMem.extXauiRegs =
                            calloc(devMemInfoPtr->phyMem.extXauiRegsNum,
                                                sizeof(SMEM_PHY_REGISTER));

        if (devMemInfoPtr->phyMem.extXauiRegs == 0)
        {
            skernelFatalError("smemTwistPhyRegsInit: ext allocation error\n");
        }

    }


}
/*******************************************************************************
*  smemTwistXauiPhyRegsFing
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
static SMEM_PHY_REGISTER * smemTwistXauiPhyRegsFing(
    IN SKERNEL_DEVICE_OBJECT    *       deviceObjPtr,
    IN GT_U32                           deviceAddr,
    IN GT_U32                           regAddr
)
{
    TWIST_DEV_MEM_INFO  *   devMemInfoPtr;
    GT_U32                  index; /* index of register */

    devMemInfoPtr = (TWIST_DEV_MEM_INFO *) deviceObjPtr->deviceMemory;
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
    skernelFatalError("smemTwistXauiPhyRegsFing: wrong PHY address,"\
    "PP device is %d, PHY device is %X, Register address %04X",
                    deviceObjPtr->deviceId, deviceAddr, regAddr);

    return NULL;

}

/*******************************************************************************
*  smemTwistActiveWriteFdbMsg
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
static void smemTwistActiveWriteXSmii (
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
    simRegPtr = smemTwistXauiPhyRegsFing(deviceObjPtr, phyDevice, regAddr);

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

      default: skernelFatalError("smemTwistActiveWriteXSmii: wrong opCode %d",
                                    operCode);
    }
}

/*******************************************************************************
*  smemTwistActiveWriteSFlowCnt
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
static void smemTwistActiveWriteSFlowCnt(
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
    TWIST_DEV_MEM_INFO  * devMemInfoPtr;

    devMemInfoPtr = (TWIST_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;

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
*  smemTwistNsramRead
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
GT_U32 smemTwistNsramRead (
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SMEM_NSRAM_BASE_ADDR_ENT nsramBaseAddr,
    IN GT_U32 sramAddrOffset
)
{
    GT_U32 address;
    GT_U32 dramType;
    GT_U32 * nSramWordPtr;

    address = SMEM_TWIST_NSRAM_ADDR(nsramBaseAddr, sramAddrOffset);
    dramType = ((nsramBaseAddr >> 27) & 0x03);

    nSramWordPtr = smemTwistExternalMem(deviceObjPtr, address, 1, dramType);

    return (*nSramWordPtr);
}

/*******************************************************************************
*  smemTwistActivePciWriteIntr
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
static void smemTwistActivePciWriteIntr (
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
*  smemTwistFaMem
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
static GT_U32 *  smemTwistFaMem(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    /* TWIST_DEV_MEM_INFO  * devMemInfoPtr; */
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
*  smemTwistActiveWriteTcbGenxsReg
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
static void smemTwistActiveWriteTcbGenxsReg(
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
*  smemTwistActiveReadPortEgressCountersReg
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
static void smemTwistActiveReadPortEgressCountersReg(
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
*   smemTwistTxEndIntrCall
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

static void smemTwistTxEndIntrCall(
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
*   smemTwistTxResourceErrorIntrCall
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

static void smemTwistTxResourceErrorIntrCall(
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
*  smemTwistActiveWriteTransmitQueueCmdReg
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
static void smemTwistActiveWriteTransmitQueueCmdReg (
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
)
{
    GT_U32                  txQueueCmdRegVal, txCurrDescPtrRegVal;
    SBUF_BUF_ID             buffId;
    GT_U32                  msgSize = 0, currBuffSize;
    SBUF_BUF_STC            *bufInfo_PTR;
    SNET_STRUCT_TX_DESC     *txDesc;
    GT_U8                   *tmpDataPtr;
    GT_BOOL                 queueLinkListFinished;
    GT_U32                  extTxDescDataWord3, extTxDescDataWord2;
    GT_U8                   i, currState;
    GT_U32                  errataStripHeaderOffset = 0;
    GT_U8                   *actualDataPtr;


    /* write new value into register */
    *memPtr |= *inMemPtr;

    txQueueCmdRegVal = *memPtr;

    /* Get buffer */
    buffId = sbufAlloc(deviceObjPtr->bufPool, SBUF_DATA_SIZE_CNS);

    if (buffId == NULL)
    {
        simWarningPrintf(" smemTwistActiveWriteTransmitQueueCmdReg: "\
                "no buffers for Transmit Queue Command\n");
        return;
    }

    bufInfo_PTR = (SBUF_BUF_STC*)buffId;

    tmpDataPtr = bufInfo_PTR->data;

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
                                smemTwistTxResourceErrorIntrCall(deviceObjPtr, i);
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
                            skernelFatalError("smemTwistActiveWriteTransmitQueueCmdReg: first descriptor of frame omited\n");
                        }

                        /* get num of bytes in buffer */
                        /* Note: buffer length of first buffer doesn't contain
                         * length of extended descriptor data
                         */
                        if(U32_GET_FIELD((txDesc)->word2,16,14) != TX_HEADER_SIZE)
                        {
                            skernelFatalError("smemTwistActiveWriteTransmitQueueCmdReg: extended descriptor data omited\n");
                        }

                        /* copy frame to simulation tx buffer */
                        /*memcpy(tmpDataPtr, (GT_U8*)txDesc->buffPointer, TX_HEADER_SIZE);*/
                        scibDmaRead(deviceObjPtr->deviceId,txDesc->buffPointer,TX_HEADER_SIZE/4,(GT_U32*)tmpDataPtr,SCIB_DMA_BYTE_STREAM);

                        /* init general msg size */
                        msgSize = TX_HEADER_SIZE;

                        /* set simulation tx descriptor */
                        bufInfo_PTR->srcData                    = TWIST_CPU_PORT_CNS;
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
                        txDesc = (SNET_STRUCT_TX_DESC*)((GT_UINTPTR)(txDesc)->nextDescPointer);
                        if((txDesc == NULL) || (TX_DESC_GET_OWN_BIT(txDesc) == 0))
                        {
                            /* clear matching ENQ bit */
                            smemRegFldSet(deviceObjPtr, TRANS_QUEUE_COMMAND_REG, i, 1, 0);

                            if(txDesc == NULL)
                                smemTwistTxResourceErrorIntrCall(deviceObjPtr, i);

                            /* continue to next tx queue */
                            queueLinkListFinished = GT_TRUE;
                            continue;
                        }

                        /* count num of bytes in frame */
                        currBuffSize = U32_GET_FIELD((txDesc)->word2,16,14);
                        msgSize += currBuffSize;
                        if(msgSize > SBUF_DATA_SIZE_CNS)
                        {
                            skernelFatalError("smemTwistActiveWriteTransmitQueueCmdReg: packet too long\n");
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
                        txDesc = (SNET_STRUCT_TX_DESC*)((GT_UINTPTR)(txDesc)->nextDescPointer);
                        currState = 1;

                        break;

                    default:
                        skernelFatalError("smemTwistActiveWriteTransmitQueueCmdReg: simulation algorithm error\n");
                        break;
                }

            }/* end of while(!queueLinkListFinished) */

            smemTwistTxEndIntrCall(deviceObjPtr, i);

        } /* end of if(txQueueCmdRegVal & (1 << i)) */

    } /* end of for (i = 0; */

}

/*******************************************************************************
*  smemTwistActiveReadTcbCountAlarmReg
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
static void smemTwistActiveReadTcbCounterAlarmReg (
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
*  smemTwistActiveReadTcbPolicingCounterAlarmReg
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
static void smemTwistActiveReadTcbPolicingCounterAlarmReg (
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
*  smemTwistActiveReadTcbGlobalCounterReg
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
static void smemTwistActiveReadTcbGlobalCounterReg (
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
*  smemTwistActiveWriteFdbActionTrigger
*
* DESCRIPTION:
*
*   Trigger for start of FDB Action processing, called from Application/PSS
*   task context by write to MAC Table Action0 Register
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
void smemTwistActiveWriteFdbActionTrigger
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
)
{
    SBUF_BUF_ID bufferId;               /* buffer */
    GT_U32 fieldVal;                    /* register's field value */
    GT_U8  * dataPtr;                   /* pointer to the data in the buffer */
    GT_U32 dataSize;                    /* data size */

    *memPtr = *inMemPtr;

    /* Aging Trigger */
    fieldVal = SMEM_U32_GET_FIELD(inMemPtr[0], 1, 1);
    if (fieldVal == 0)
    {
        return;
    }

    /* Get buffer */
    bufferId = sbufAlloc(devObjPtr->bufPool, SFDB_MAC_TBL_ACT_BYTES);

    if (bufferId == NULL)
    {
        printf(" Active Write Fdb Action Trigger: "\
               " no buffers to update MAC table\n");
        return;
    }
    /* Get actual data pointer */
    sbufDataGet(bufferId, &dataPtr, &dataSize);

    /* copy MAC table action 2 words to buffer */
    memcpy(dataPtr, memPtr, SFDB_MAC_TBL_ACT_BYTES);

    /* set source type of buffer */
    bufferId->srcType = SMAIN_SRC_TYPE_CPU_E;

    /* set message type of buffer */
    bufferId->dataType = SMAIN_CPU_FDB_ACT_TRG_E;

    /* put buffer to queue */
    squeBufPut(devObjPtr->queueId, SIM_CAST_BUFF(bufferId));
}

/*******************************************************************************
*  smemTwistActiveReadInterruptsCauseReg
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
static void smemTwistActiveReadInterruptsCauseReg (
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
*  smemTwistActiveWriteMacInterruptsMaskReg
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
static void smemTwistActiveWriteMacInterruptsMaskReg (
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
)
{
    SBUF_BUF_ID bufferId;               /* buffer */
    GT_U32 *dataPtr;                   /* pointer to the data in the buffer */
    GT_U32 dataSize;                    /* data size */
    GT_U8  *dataU8Ptr;

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
    sbufDataGet(bufferId, (GT_U8 **)&dataU8Ptr, &dataSize);

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
*  smemTwistForceLink
*
* DESCRIPTION:
*       Write Message to the main task - if link has forced to UP/DOWN,
*
* INPUTS:
*       devObjPtr   - device object PTR.
*       portNo      - port number
*       forceLinkUp - force link up/down.
*
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*******************************************************************************/
static void smemTwistForceLink
(
    IN      SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN      GT_U32 forceLinkUp,
    IN      GT_U32 portNo
)
{
    SBUF_BUF_ID bufferId;       /* buffer */
    GT_U8  * dataPtr;           /* pointer to the data in the buffer */
    GT_U32 dataSize;            /* data size */

    /* Get buffer      */
    bufferId = sbufAlloc(devObjPtr->bufPool, SMEM_TWIST_LINK_FORCE_MSG_SIZE);
    if (bufferId == NULL)
    {
       printf("smemTwistForceLink : no buffers for force link bit \n");
       return;
    }

    /* Get actual data pointer */
    sbufDataGet(bufferId, (GT_U8 **)&dataPtr, &dataSize);

    memcpy(&dataPtr[0], (GT_U8 *)&portNo , sizeof(GT_U8));
    memcpy(&dataPtr[1], (GT_U8 *)&forceLinkUp , sizeof(GT_U8));

    /* set source type of buffer */
    bufferId->srcType = SMAIN_SRC_TYPE_CPU_E;

    /* set message type of buffer */
    bufferId->dataType = SMAIN_LINK_CHG_MSG_E;

    /* put buffer to queue */
    squeBufPut(devObjPtr->queueId, SIM_CAST_BUFF(bufferId));
}

/*******************************************************************************
*  smemTwistActiveWriteMacControl
*
* DESCRIPTION:
*       Write Message to the main task - if link has forced to UP/DOWN,
*       update port MAC status register
*
* INPUTS:
*       devObjPtr - device object PTR.
*       address     - Address for ASIC memory.
*       memSize     - size of the requested memory
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers's specific parameter.
*       inMemPtr    - Pointer to the memory to get register's content.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*******************************************************************************/
void smemTwistActiveWriteMacControl
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
)
{
    GT_U32 fldCtrlVal;          /* control register's field value */
    GT_U32 forceLinkUpVal;      /* control register's force link pass field value */
    GT_U32 forceLinkDownVal;    /* control register's force link down field value */
    GT_U32 fldStatVal;          /* status register's field value */
    GT_U32 regStatAddr = 0;     /* status register address */
    GT_U32 regStatVal;          /* status register's value */
    GT_U32 portNo;              /* port number */

    *memPtr = *inMemPtr ;

    /* Retrieve port from MAC control register address */
    MAC_CTRL_ADDR_2_PORT(devObjPtr->deviceType, address, param, portNo);

    /* MAC status register */
    MAC_STATUS_REG(devObjPtr->deviceType, portNo, regStatAddr);

    smemRegGet(devObjPtr, regStatAddr, &regStatVal);

    /* Get control and status port force link value */
    fldStatVal = SMEM_U32_GET_FIELD(regStatVal, 0, 1);
    forceLinkDownVal = SMEM_U32_GET_FIELD(inMemPtr[0], 1, 1);
    forceLinkUpVal = SMEM_U32_GET_FIELD(inMemPtr[0], 2, 1);

    /* Force link up */
    if (forceLinkUpVal == 1 && (fldStatVal == 0))
    {
        smemTwistForceLink(devObjPtr, 1, portNo);
    }
    else
    if (forceLinkUpVal == 0)
    {
        /* Force link down */
        if (forceLinkDownVal == 1 && (fldStatVal == 1))
        {
            smemTwistForceLink(devObjPtr, 0, portNo);
        }
    }

    /* Enable Auto-Negotiation for duplex mode */
    fldCtrlVal = SMEM_U32_GET_FIELD(inMemPtr[0], 3, 1);
    if (fldCtrlVal == 0)
    {
        /* Control and status port duplex value */
        fldCtrlVal = SMEM_U32_GET_FIELD(inMemPtr[0], 4, 1);
        fldStatVal = SMEM_U32_GET_FIELD(regStatVal, 4, 1);
        if (fldCtrlVal != fldStatVal)
        {
            smemRegFldSet(devObjPtr, regStatAddr, 4, 1, fldCtrlVal);
        }
    }

    /* Enable Auto-Negotiation of interface speed */
    fldCtrlVal = SMEM_U32_GET_FIELD(inMemPtr[0], 17, 1);
    if (fldCtrlVal == 0)
    {
        /* Control and status port Gbps/Fast Speed */
        fldCtrlVal = SMEM_U32_GET_FIELD(inMemPtr[0], 18, 1);
        fldStatVal = SMEM_U32_GET_FIELD(regStatVal, 2, 1);
        if (fldCtrlVal != fldStatVal)
        {
            smemRegFldSet(devObjPtr, regStatAddr, 2, 1, fldCtrlVal);
        }
        if (fldCtrlVal == 0)
        {
            /* Port works in 10/100 Mbps (Fast Ethernet) */
            fldCtrlVal = SMEM_U32_GET_FIELD(inMemPtr[0], 28, 1);
            fldStatVal = SMEM_U32_GET_FIELD(regStatVal, 3, 1);
            if (fldCtrlVal != fldStatVal)
            {
                smemRegFldSet(devObjPtr, regStatAddr, 3, 1, fldCtrlVal);
            }
        }
    }
}

/*******************************************************************************
*  smemTwistBackUpMemory
*
* DESCRIPTION:
*      Definition of backup/restore memory function
* INPUTS:
*       devObjPtr   - pointer to device object.
*       readWrite   - GT_TRUE - backup device memory data
*                     GT_FALSE - restore device memory data
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static GT_VOID smemTwistBackUpMemory
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_BOOL readWrite
)
{
    TWIST_DEV_MEM_INFO  * devMemInfoPtr;/* Twist memory object pointer */
    GT_U32 * memPtr;                    /* Memory pointer */
    GT_U32  address;                    /* Memory space address */
    GT_U32  unit;                       /* Unit index bit index */
    GT_32   dramType;                   /* External memory trype */
    GT_CHAR buffer[SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS]; /* Read/Write buffer */
    GT_CHAR keyStr[20];                 /* Key string */
    GT_U32  param;                      /* Address type specific parameter */
    GT_CHAR *fileNamePtr;               /* Dump memory file name */
    FILE  * memObjFilePtr;              /* Dump memory file descriptor */

    /* Read dump memory file name from INI file */
    sprintf(keyStr, "dev%u_mem_dump", devObjPtr->deviceId);

    if (SIM_OS_MAC(simOsGetCnfValue)("system", keyStr,
            SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS, buffer))
    {
        fileNamePtr = buffer;
    }
    else
    {
        fileNamePtr = "asic_memory_dump.txt";
    }

    if (readWrite == GT_FALSE)
    {
        /* Restore data from dump memory file */
        smainMemDefaultsLoad(devObjPtr, fileNamePtr);

        return;
    }

    skernelUserDebugInfo.disableFatalError = GT_TRUE;

    /* Open file for write */
    memObjFilePtr = fopen(fileNamePtr, "w");

    devMemInfoPtr = (TWIST_DEV_MEM_INFO*)(devObjPtr->deviceMemory);

    /* Work around for HSU memory dump */
    smemRegFldSet(devObjPtr, 0x8, 16, 15,
                  devMemInfoPtr->auqMem.auqOffset);

    /* Iterate all address space and check address memory type */
    for (address = 0; address < 0xffffffff; address += 0x4)
    {
        if ((address & 0x40000000) > 0 )
        {
            /* FA memory */
            break;
        }
        else
        if ((address & 0x20000000) > 0 ) {
            /* Find external memory */
            dramType = ((address >> 27) & 0x03);

            memPtr = smemTwistExternalMem(devObjPtr, address, 1, dramType);
        }
        else
        {
            /* Find common registers memory */
            unit = (address & REG_SPEC_FUNC_INDEX) >> 23;
            if (unit > 63)
            {
                break;
            }

            /* Call register spec function to obtain pointer to register memory */
            param = devMemInfoPtr->specFunTbl[unit].specParam;
            memPtr = devMemInfoPtr->specFunTbl[unit].specFun(devObjPtr,
                                                             SKERNEL_MEMORY_READ_E,
                                                             address,
                                                             1,
                                                             param);
        }

        /* Does memory exist and set */
        if (memPtr && memPtr[0] && memPtr[0] != 0xcdcdcdcd)
        {
            /* Print memory value */
            sprintf(buffer, "%08x %08x\n", address, memPtr[0]);
            fprintf(memObjFilePtr, buffer);
        }
    }

    fclose(memObjFilePtr);

    skernelUserDebugInfo.disableFatalError =  GT_FALSE;
}

