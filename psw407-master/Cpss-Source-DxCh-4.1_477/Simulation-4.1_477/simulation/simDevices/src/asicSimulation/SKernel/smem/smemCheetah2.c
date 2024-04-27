/******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* smemCheetah2.c
*
* DESCRIPTION:
*       Cheetah2 memory mapping implementation.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 48 $
*
*******************************************************************************/
#include <os/simTypes.h>
#include <asicSimulation/SKernel/smem/smemCheetah.h>
#include <asicSimulation/SKernel/smem/smemCheetah2.h>
#include <asicSimulation/SKernel/smem/smemCheetah3.h>
#include <asicSimulation/SKernel/cheetahCommon/sregCheetah.h>
#include <asicSimulation/SKernel/sfdb/sfdbCheetah.h>
#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SKernel/smain/smain.h>
#include <asicSimulation/SKernel/suserframes/snetCheetahEgress.h>
#include <asicSimulation/SKernel/suserframes/snetCheetahL2.h>

static DECLARE_UNIT_MEM_FUNCTION_MAC(smemCht2GlobalReg               );
static DECLARE_UNIT_MEM_FUNCTION_MAC(smemCht2TransQueReg             );
static DECLARE_UNIT_MEM_FUNCTION_MAC(smemCht2EtherBrdgReg            );
static DECLARE_UNIT_MEM_FUNCTION_MAC(smemCht2RouterReg               );
static DECLARE_UNIT_MEM_FUNCTION_MAC(smemCht2MllEngineReg            );
static DECLARE_UNIT_MEM_FUNCTION_MAC(smemCht2BufMngReg               );
static DECLARE_UNIT_MEM_FUNCTION_MAC(smemCht2MacTableReg             );
static DECLARE_UNIT_MEM_FUNCTION_MAC(smemCht2InterBufBankReg         );
static DECLARE_UNIT_MEM_FUNCTION_MAC(smemCht2BufMemoryConfReg        );
static DECLARE_UNIT_MEM_FUNCTION_MAC(smemCht2VlanTableReg            );
static DECLARE_UNIT_MEM_FUNCTION_MAC(smemCht2TriSpeedReg             );
static DECLARE_UNIT_MEM_FUNCTION_MAC(smemCht2PreEgressReg            );
static DECLARE_UNIT_MEM_FUNCTION_MAC(smemCht2PclReg                  );
static DECLARE_UNIT_MEM_FUNCTION_MAC(smemCht2PolicerReg              );
static DECLARE_UNIT_MEM_FUNCTION_MAC(smemCht2PciReg                  );
static DECLARE_UNIT_MEM_FUNCTION_MAC(smemCht2PolicerTbl              );
static DECLARE_UNIT_MEM_FUNCTION_MAC(smemCht2VlanTbl                 );
static DECLARE_UNIT_MEM_FUNCTION_MAC(smemCht2ArpTbl                  );


static ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemCht2ActiveWritePclAction      );
static ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemCht2ActiveWriteRouterAction   );

static void * smemCht2FindMem
(
    IN SKERNEL_DEVICE_OBJECT * deviceObj,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32                  address,
    IN GT_U32                  memSize,
    OUT SMEM_ACTIVE_MEM_ENTRY_STC ** activeMemPtrPtr
);

static void smemCht2InitFuncArray
(
    INOUT SMEM_CHT2_DEV_MEM_INFO  * devMemInfoPtr
);

static void smemCht2AllocSpecMemory
(
    INOUT SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_CHT2_DEV_MEM_INFO  * devMemInfoPtr
);


static void smemCht2ReadWritePolicyTcam
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32  lineNumber,
    IN         GT_U32  tcamWriteMode,
    IN         GT_U32  tcamDataType,
    IN         GT_U32  rdWr,
    IN         GT_U32  tcamValid       ,
    IN         GT_U32  tcamCompMode
);

static void smemCht2ReadPolicyTcamCtrl
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32  line,
    IN         GT_U32  type,
    IN         GT_U32  mode
);

static void smemCht2ReadPolicyTcamData
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32  line,
    IN         GT_U32  type,
    IN         GT_U32  mode
);

static void smemCht2WritePolicyTcamCtrl
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32  lineNumber,
    IN         GT_U32  tcamDataType,
    IN         GT_U32  tcamWriteMode,
    IN         GT_U32  tcamValid  ,
    IN         GT_U32  tcamCompMode
);

static void smemCht2WritePolicyTcamData
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32  lineNumber ,
    IN         GT_U32  tcamDataType ,
    IN         GT_U32  tcamWriteMode ,
    IN         GT_U32  tcamValid ,
    IN         GT_U32  tcamCompMode
);

static void smemCht2ReadWritePclAction
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32  line,
    IN         GT_U32  rdWr
);

static void smemCht2WriteRouterTTTcam
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32  rtLineNumber,
    IN         GT_U32  rtDataType,
    IN         GT_U32  rtWrTargetArr,
    IN         GT_U32  rtValidBit ,
    IN         GT_U32  rtCompMode
);

/* Private definition */
#define     GLB_REGS_NUM                        (76)
#define     GLB_SDMA_REGS_NUM                   (1023)
#define     EGR_GLB_REGS_NUM                    (0xffff / 4)
#define     EGR_TRUNK_FILTER_REGS_NUM           (2032)
#define     TXQ_INTERNAL_REGS_NUM               (512)
#define     EGR_PORT_WD_REGS_NUM                (64)
#define     EGR_MIB_CNT_REGS_NUM                (120)
#define     EGR_HYP_MIB_REGS_NUM                (60 + 28)
#define     EGR_TALE_DROP_REGS_NUM              (3132)
#define     EGR_RATE_SHAPES_REGS_NUM            (512)
#define     BRDG_ETH_BRDG_REGS_NUM              (262655)
#define     BUF_MNG_REGS_NUM                    (196)
#define     BUF_MNG_LL_REGS_NUM                 (4096)
#define     GOP_CONF_REGS_NUM                   (512)
#define     GOP_MAC_MIB_REGS_NUM                (892)
#define     GOP_LED_REGS_NUM                    (16)
#define     MAC_FDB_REGS_NUM                    (124)
#define     MAC_TBL_REGS_NUM                    (16384 * 4)
#define     BANK_MEM_REGS_NUM                   (98304)
#define     BUF_MEM_REGS_NUM                    (0x4ff)
#define     VTL_VLAN_CONF_REGS_NUM              (5)
#define     VTL_VLAN_TBL_REGS_NUM               (4096 * 4)
#define     VTL_MCST_TBL_REGS_NUM               (4096)
#define     VTL_STP_TBL_REGS_NUM                (256 * 2)
#define     TRI_SPEED_REGS_NUM                  (16383)
#define     PRE_EGR_PORTS_REGS_NUM              (0x7F / 4)
#define     PRE_EGR_GEN_REGS_NUM                (0xFF)
#define     PRE_EGR_TRUNK_TBL_REGS_NUM          (0x1000 / 4)
#define     PRE_QOS_TBL_REGS_NUM                (0xFF)
#define     PRE_CPU_CODE_TBL_REGS_NUM           (0x400 / 4)
#define     PRE_STAT_RATE_TBL_REGS_NUM          (0x90 / 4)
#define     PRE_PORTS_INGR_STC_REGS_NUM         (0x1FF)
#define     PCL_CONF_REGS_NUM                   (4220)
#define     PCL_ACCESS_DATA_REGS_NUM            (0x1fff / 4 + 1)
#define     EPCL_CONF_REGS_NUM                  (88)
#define     PCL_TCAM_REGS_NUM                   (0x14000)
#define     PCL_INTERN_REGS_NUM                 (69)
#define     PCL_PORT_VLAN_QOS_REGS_NUM          (256 * 2)
#define     PCL_PROT_BASED_VLAN_QOS_REGS_NUM    (64)
#define     PCL_ACTION_TBL_REGS_NUM             (1024 * 4)
#define     PCL_ID_CONF_TBL_REGS_NUM            (4224)
#define     PCL_RULE_MATCH_CNT_REGS_NUM         (32)
#define     PCL_TCP_TBL_REGS_NUM                                (63)
#define     EPCL_ID_CONF_TBL_REGS_NUM           (4160)
#define     POLICER_REGS_NUM                    (27)
#define     POLICER_TBL_REGS_NUM                (256 * 4)
#define     POLICER_QOS_REM_TBL_REGS_NUM        (128)
#define     POLICER_COUNT_TBL_REGS_NUM          (0x40)
#define     PCI_REGS_NUM                        (280)
#define     PHY_XAUI_DEV_NUM                    (3)
#define     PHY_IEEE_XAUI_REGS_NUM              (0xffff)
#define     PHY_EXT_XAUI_REGS_NUM               (0xffff)

#define     ARP_TABLE_REGS_NUM                  (1024 * 8)
#define     MAC_SA_TABLE_REGS_NUM               (4096)
#define     TWSI_INT_REGS_NUM                   (0x1C / 4 + 1)
#define     ROUTER_CONF_REGS_NUM                (0x1200/3)
#define     ROUTER_TCAM_REGS_NUM                (0x5000)
#define     LTT_ACTION_TBL_REGS_NUM             (0x1000)
#define     NEXT_HOP_TBL_REGS_NUM               (0xFFFF / 4 + 1)
#define     MLL_GLOB_REGS_NUM                   (0xFFF / 4 + 1)
#define     MLL_TBL_REGS_NUM                    (0x3FFF / 4 + 1)
#define     EGR_XSMI_REGS_NUM                   (2)
#define     IP_PROT_REGS_NUM                    (16)
#define     CPU_RATE_LIMIT_REGS_NUM             (128)
#define     TRUNK_MEMBER_TBL_REGS_NUM           (0x40)
#define     ING_RESTRICT_REGS_NUM               (0x4)
#define     EGR_STC_REGS_NUM                    (0xDC0)
#define     INGR_STC_REGS_NUM                   (0x70)

/* invalid MAC message  */
#define     SMEM_CHEETAH2_INVALID_MAC_MSG_CNS   0xffffffff

/* number of capacity in ipv4 */
#define IP_ROUT_TCAM_5120_IPV4_CNS                (0x1400)
/* number of capacity in ipv6  */
#define IP_ROUT_TCAM_1024_IPV6_CNS                (0x400)
/* the range of address between one word to another */
#define IP_ROUT_TCAM_WORDS_WIDTH_CNS              (0x4000 / 4)
/* the range of address between one word to another */
#define IP_ROUT_TCAM_ENTRY_WIDTH_CNS              (0x04 )

static SKERNEL_DEVICE_ROUTE_TCAM_INFO_STC  ch2RoutTcamInfo =
{
    IP_ROUT_TCAM_5120_IPV4_CNS,
    IP_ROUT_TCAM_1024_IPV6_CNS,
    IP_ROUT_TCAM_WORDS_WIDTH_CNS,
    IP_ROUT_TCAM_ENTRY_WIDTH_CNS,
    0/* don't care */
};

/* Active memory table */
static SMEM_ACTIVE_MEM_ENTRY_STC smemCht2ActiveTable[] =
{
    /* Global interrupt cause register */
    {0x00000030, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteIntrCauseReg, 0},

    /* Port<n> Interrupt Cause Register  */
    {0x0A800020, SMEM_CHT_GOPINT_MSK_CNS,
        smemChtActiveReadIntrCauseReg, 18, smemChtActiveWriteIntrCauseReg, 0},
    /* Tri-Speed Port<n> Interrupt Mask Register */
    {0x0A800024, SMEM_CHT_GOPINT_MSK_CNS,
        NULL, 0, smemChtActiveWritePortInterruptsMaskReg, 0},

    /* Receive SDMA Interrupt Cause Register (RxSDMAInt) */
    {0x0000280C, SMEM_FULL_MASK_CNS,
        smemChtActiveReadIntrCauseReg, 22, smemChtActiveWriteIntrCauseReg, 0},
    /* Receive SDMA Interrupt Mask Register */
    {0x00002814, SMEM_FULL_MASK_CNS,
        NULL, 0, smemChtActiveWriteRcvSdmaInterruptsMaskReg, 0},

    /* Bridge  Interrupt Cause Register */
    {0x02040130, SMEM_FULL_MASK_CNS,
        smemChtActiveReadIntrCauseReg, 14, smemChtActiveWriteIntrCauseReg, 0},
    /* Bridge Interrupt Mask Register */
    {0x02040134, SMEM_FULL_MASK_CNS,
        NULL, 0, smemChtActiveWriteBridgeInterruptsMaskReg, 0},

    /* Security Breach Status Register2 */
    {0x020401A8, SMEM_FULL_MASK_CNS, smemChtActiveReadBreachStatusReg, 0, NULL, 0},

    /* Matrix Bridge Register */
    {0x020400d4, 0xFFFFFFFF, smemChtActiveReadCntrs, 0, NULL, 0},

    /* Port MIB Counters Capture */
    {0x04004020, SMEM_FULL_MASK_CNS, NULL, 0, smemChtActiveWriteMibCntCtrl, 0},
    {0x04804020, SMEM_FULL_MASK_CNS, NULL, 0, smemChtActiveWriteMibCntCtrl, 1},
    {0x05004020, SMEM_FULL_MASK_CNS, NULL, 0, smemChtActiveWriteMibCntCtrl, 2},
    {0x05804020, SMEM_FULL_MASK_CNS, NULL, 0, smemChtActiveWriteMibCntCtrl, 3},

    /* SMI0 Management Register : 0x04004054 , 0x05004054 */
    {0x04004054, 0xFEFFFFFF, NULL, 0 , smemChtActiveWriteSmi,0},

    /* HyperG.Stack Ports MIB Counters and XSMII Configuration Register */
    {0x01800180, SMEM_FULL_MASK_CNS, NULL, 0, smemChtActiveWriteHgsMibCntCtrl, 0},

    /* Set Incoming Packet Count */
    {0x020400E0, 0xFFFFFFF0, smemChtActiveReadCntrs, 0, NULL,0}, /* Set #0 counters */
    {0x020400F4, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0}, /* Set #1 Incoming counters */
    {0x020400F8, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0}, /* Set #1 VLAN ingress filtered counters */
    {0x020400FC, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0}, /* Set #1 Security filtered counters */
    {0x02040100, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0}, /* Set #1 Bridge filtered counters */

    /* The MAC MIB Counters */
    {0x04010000, SMEM_CHT_COUNT_MSK_CNS, smemChtActiveReadCntrs, 0, NULL,0},
    {0x04810000, SMEM_CHT_COUNT_MSK_CNS, smemChtActiveReadCntrs, 1, NULL,0},
    {0x05010000, SMEM_CHT_COUNT_MSK_CNS, smemChtActiveReadCntrs, 2, NULL,0},
    {0x05810000, SMEM_CHT_COUNT_MSK_CNS, smemChtActiveReadCntrs, 3, NULL,0},

    /* The MAC MIB Counters of XG ports 24..27 */
    {0x01C00000, SMEM_CHT_COUNT_MSK_CNS, smemChtActiveReadCntrs, 24, NULL, 0},
    {0x01C40000, SMEM_CHT_COUNT_MSK_CNS, smemChtActiveReadCntrs, 25, NULL, 0},
    {0x01C80000, SMEM_CHT_COUNT_MSK_CNS, smemChtActiveReadCntrs, 26, NULL, 0},
    {0x01DC0000, SMEM_CHT_COUNT_MSK_CNS, smemChtActiveReadCntrs, 27, NULL, 0},

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
    {0x01B40144, 0xFFFFFFFF, smemChtActiveReadCntrs, 0, NULL,0},
    {0x01B40148, 0xFFFFFFFF, smemChtActiveReadCntrs, 0, NULL,0},
    {0x01B4014C, 0xFFFFFFFF, smemChtActiveReadCntrs, 0, NULL,0},
    {0x01B40150, 0xFFFFFFF0, smemChtActiveReadCntrs, 0, NULL,0},
    {0x01B40160, 0xFFFFFFF0, smemChtActiveReadCntrs, 0, NULL,0},
    {0x01B40170, 0xFFFFFFF0, smemChtActiveReadCntrs, 0, NULL,0},
    {0x01B40180, 0xFFFFFFFF, smemChtActiveReadCntrs, 0, NULL,0},
    {0x01B40184, 0xFFFFFFFF, smemChtActiveReadCntrs, 0, NULL,0},

    /* The SDMA resource error registers */
    {0x00002860, SMEM_CHT_COUNT_MSK_CNS, smemChtActiveReadCntrs, 0, NULL,0},
    {0x00002864, SMEM_CHT_COUNT_MSK_CNS, smemChtActiveReadCntrs, 0, NULL,0},

    /* Mac Table Access Control Register */
    {0x06000064, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteFdbMsg,0},

    /* Address Update Queue Base Address */
    {0x000000C0, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveAuqBaseWrite, 0},

    /* FDB Upload Queue Base Address */
    {0x000000C8, SMEM_FULL_MASK_CNS, NULL, 0 , smemCht2ActiveFuqBaseWrite, 0},

    /* XSMI Management Register */
    {0x01CC0000, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteXSmii, 0},

    /* Ports VLAN, QoS and Protocol Access Control Register */
    {0x0B800328, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteVlanQos, 0},

    /* Action Table and Policy TCAM Access Control Register */
    {0x0B800130, SMEM_FULL_MASK_CNS, NULL, 0 , smemCht2ActiveWritePclAction, 0},

    /* Router and TT TCAM Access Control Register */
    {0x0280041C, SMEM_FULL_MASK_CNS, NULL, 0 , smemCht2ActiveWriteRouterAction, 0},

    /* PCL-ID Table Access Control Register */
    {0x0B800204, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWritePclId, 0},

    /* Policers Table Access Control Register */
    {0x0C000014, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWritePolicerTbl, 0},

    /* Policers QoS Remarking and Initial DP Table Access Control Register */
    {0x0C000024, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWritePolicerQos, 0},

    /* Policer Counters Table Access Control Register */
    {0x0C000038, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWritePolicerCnt, 0},

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

    /* VLT Tables Access Control Register */
    {0x0A000010, SMEM_FULL_MASK_CNS, NULL, 0 , smemCht2ActiveWriteVlanTbl, 0},

    /* SDMA */
    {0x00002680, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteSdmaCommand,0},

    /* Message From CPU Management */
    {0x06000050, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteNewFdbMsgFromCpu, 0},

    /* MAC Table Action0 Register */
    {0x06000004, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteFdbActionTrigger, 0},

    /* Transmit SDMA Queue Command Register */
    {0x00002868, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteStartFromCpuDma, 0},

    /* Port<n> Auto-Negotiation Configuration Register */
    {0x0A80000C, 0xFFFF20FF, NULL, 0 , smemChtActiveWriteForceLinkDown, 0},

    {0x0A801800, 0xFFFFF8FF, NULL, 0 , smemChtActiveWriteForceLinkDownXg, 0},
    /* ARP MAC DA table */
    {0x07800208, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteArpTable, 0},

    /* HGS Port<n> Interrupt Cause Register  */
    {0x0A801814, SMEM_FULL_MASK_CNS,
        smemChtActiveReadIntrCauseReg, 24, smemChtActiveWriteIntrCauseReg, 0},
    /* HGS Port<n> Interrupt Mask Register */
    {0x0A801818, SMEM_FULL_MASK_CNS,
        NULL, 0, smemChtActiveWriteHgsInterruptsMaskReg, 0},

    {0x0A801914, SMEM_FULL_MASK_CNS,
        smemChtActiveReadIntrCauseReg, 25, smemChtActiveWriteIntrCauseReg, 0},
    /* HGS Port<n> Interrupt Mask Register */
    {0x0A801918, SMEM_FULL_MASK_CNS,
        NULL, 0, smemChtActiveWriteHgsInterruptsMaskReg, 0},

    {0x0A801A14, SMEM_FULL_MASK_CNS,
        smemChtActiveReadIntrCauseReg, 26, smemChtActiveWriteIntrCauseReg, 0},
    /* HGS Port<n> Interrupt Mask Register */
    {0x0A801A18, SMEM_FULL_MASK_CNS,
        NULL, 0, smemChtActiveWriteHgsInterruptsMaskReg, 0},

    {0x0A801B14, SMEM_FULL_MASK_CNS,
        smemChtActiveReadIntrCauseReg, 27, smemChtActiveWriteIntrCauseReg, 0},
    /* HGS Port<n> Interrupt Mask Register */
    {0x0A801B18, SMEM_FULL_MASK_CNS,
        NULL, 0, smemChtActiveWriteHgsInterruptsMaskReg, 0},

    /* MAC Table Interrupt Mask Register */
    {0x0600001c, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteMacInterruptsMaskReg, 0},

    /* Transmit SDMA Interrupt Cause Register */
    {0x00002810, SMEM_FULL_MASK_CNS,
        smemChtActiveReadIntrCauseReg, 21 , smemChtActiveWriteIntrCauseReg, 0},
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

    /* SDMA configuration register */
    {0x00002800, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteSdmaConfigReg, 0},

    /* Transmit Queue Control Register */
    {0x01800000, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteTxQuCtrlReg, 0},

    /* Transmit Queue Config Register */
    {0x01800080, 0XFFFF01FF, NULL, 0 , smemChtActiveWriteTxQConfigReg, 0},

    /* Port<%n> Rate Limit Counter */
    {0x02000400, 0XFFFE0FFF,
        smemChtActiveReadRateLimitCntReg, 0 , smemChtActiveWriteToReadOnlyReg, 0},

    /* CPU Port MIB counters registers */
    {0x00000060, 0xFFFFFFE0, smemChtActiveReadCntrs, 0, NULL,0},

    /* must be last anyway */
    {0xffffffff, SMEM_FULL_MASK_CNS, NULL,0,NULL,0}
};

#define SMEM_ACTIVE_MEM_TABLE_SIZE \
    (sizeof(smemCht2ActiveTable)/sizeof(smemCht2ActiveTable[0]))

/* Active memory table */
static SMEM_ACTIVE_MEM_ENTRY_STC smemCht2PciActiveTable[] =
{
    /* PCI Interrupt Cause Register */
    {0x00000114, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteIntrCauseReg, 0},
    /* PCI Interrupt Mask Register -- all actions here --> do on 0x00000034 */
    {0x00000118, SMEM_FULL_MASK_CNS, smemChtActiveReadRedirect, 0x00000034 , smemChtActiveWriteRedirect, 0x00000034},

    /* must be last anyway */
    {0xffffffff, SMEM_FULL_MASK_CNS, NULL,0,NULL,0}
};

#define SMEM_ACTIVE_PCI_MEM_TABLE_SIZE \
    (sizeof(smemCht2PciActiveTable)/sizeof(smemCht2PciActiveTable[0]))


/*******************************************************************************
*   smemCht2Init
*
* DESCRIPTION:
*       Init memory module for the Cheetah2 device.
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
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_CHT2_DEV_MEM_INFO  * devMemInfoPtr;

    devMemInfoPtr = (SMEM_CHT2_DEV_MEM_INFO *)smemDeviceObjMemoryAlloc(devObjPtr,1, sizeof(SMEM_CHT2_DEV_MEM_INFO));
    if (devMemInfoPtr == 0)
    {
        skernelFatalError("smemCht2Init: allocation error\n");
    }

    devObjPtr->deviceMemory = devMemInfoPtr;

    devObjPtr->fdbNumEntries = SMEM_MAC_TABLE_SIZE_16KB;
    devObjPtr->fdbMaxNumEntries = devObjPtr->fdbNumEntries;
    devObjPtr->supportEqEgressMonitoringNumPorts = 32;

    /* init specific functions array */
    smemCht2InitFuncArray(devMemInfoPtr);

    /* allocate address type specific memories */
    smemCht2AllocSpecMemory(devObjPtr, devMemInfoPtr);

    /*override default of 0 */
    devObjPtr->globalInterruptCauseRegWriteBitmap_pci = 0x1fe;/* bits 1..8 */

    devObjPtr->activeMemPtr = smemCht2ActiveTable;

    /* set router tcam info */
    devObjPtr->routeTcamInfo =  ch2RoutTcamInfo;

    smemCht2TableInfoSet(devObjPtr);
}

/*******************************************************************************
*   smemCht2FindMem
*
* DESCRIPTION:
*       Return pointer to the register's or tables's memory.
*
* INPUTS:
*       deviceObj   - pointer to device object.
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
static void * smemCht2FindMem
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32                  address,
    IN GT_U32                  memSize,
    OUT SMEM_ACTIVE_MEM_ENTRY_STC ** activeMemPtrPtr
)
{
    void                    * memPtr;
    SMEM_CHT2_DEV_MEM_INFO  * devMemInfoPtr;
    GT_32                     index;
    GT_U32                    param;

    if (devObjPtr == 0)
    {
        skernelFatalError("smemCht2FindMem: illegal pointer \n");
    }
    memPtr = 0;
    devMemInfoPtr = (SMEM_CHT2_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    /* Find PCI registers memory  */
    if (SMEM_ACCESS_PCI_FULL_MAC(accessType))
    {
        memPtr = smemCht2PciReg(devObjPtr, accessType, address, memSize, 0);

        /* find PCI active memory entry */
        if (activeMemPtrPtr != NULL)
        {
            for (index = 0; index < (SMEM_ACTIVE_PCI_MEM_TABLE_SIZE - 1);
                  index++)
            {
                /* check address */
                if ((address & smemCht2PciActiveTable[index].mask)
                      == smemCht2PciActiveTable[index].address)
                {
                    *activeMemPtrPtr = &smemCht2PciActiveTable[index];
                    break;
                }
            }
        }
        return memPtr;
    }

    index = (address & REG_SPEC_FUNC_INDEX) >>
                 SMEM_CHT_UNIT_INDEX_FIRST_BIT_CNS;

    if (((address >> SMEM_PHY_UNIT_INDEX_CNS) & 0x3) == 0x3)
    {
      memPtr  = smemChtPhyReg(devObjPtr, address, memSize);
    }
    else
    {
      param   = devMemInfoPtr->common.specFunTbl[index].specParam;
      memPtr  = devMemInfoPtr->common.specFunTbl[index].specFun(devObjPtr,
                                                                accessType,
                                                                address,
                                                                memSize,
                                                                param);
    }

    /* find active memory entry                */
    if (activeMemPtrPtr != NULL)
    {
        *activeMemPtrPtr = NULL;
        for (index = 0; index < (SMEM_ACTIVE_MEM_TABLE_SIZE - 1); index++)
        {
            /* check address    */
            if ((address & smemCht2ActiveTable[index].mask)
                 == smemCht2ActiveTable[index].address)
            {
                *activeMemPtrPtr = &smemCht2ActiveTable[index];
                break;
            }
        }
    }

    return memPtr;
}

/*******************************************************************************
*   smemCht2GlobalReg
*
* DESCRIPTION:
*       Global Registers, TWSI, CPU port, SDMA and PCI internal registers
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
static GT_U32 *  smemCht2GlobalReg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32                  address,
    IN GT_U32                  memSize,
    IN GT_UINTPTR              param
)
{
    SMEM_CHT2_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32              * regValPtr;
    GT_U32                index;

    regValPtr = 0;
    devMemInfoPtr = (SMEM_CHT2_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    /* Global registers */
    if ((address & 0x1FFFFF00) == 0x0)
    {
        index = (address & 0xFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->globalMem.globReg,
                         devMemInfoPtr->globalMem.globRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->globalMem.globReg[index];
    }
    else
     /* SDMA registers */
    if ((address & 0x1FFFF000) == 0x00002000)
    {
        index = (address & 0xFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->globalMem.sdmaReg,
                         devMemInfoPtr->globalMem.sdmaRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->globalMem.sdmaReg[index];
    }
    else
    /* TWSI registers */
    if ((address & 0x1FFFFF00) == 0x00400000)
    {
        index = (address & 0xFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->globalMem.twsiIntRegs,
                         devMemInfoPtr->globalMem.twsiIntRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->globalMem.twsiIntRegs[index];
    }

    return regValPtr;
}

/*******************************************************************************
*   smemCht2TransQueReg
*
* DESCRIPTION:
*       Egress, GPP configuration registers ,
*       Transmit Queue and VLAN Configuration Register Map Table,
*       Port TxQ Configuration Register, Hyper.GStack Ports MIB Counters
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
static GT_U32 *  smemCht2TransQueReg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SMEM_CHT2_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32                  * regValPtr;
    GT_U32                    index;
    GT_U32                    group;

    regValPtr = 0;
    devMemInfoPtr = (SMEM_CHT2_DEV_MEM_INFO *)devObjPtr->deviceMemory;

    /* Common registers , GPP configuration registers */
    if ((address & 0x1FFF0000) == 0x01800000)
    {
        index = (address & 0xFFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->egrMem.egrGenReg,
                         devMemInfoPtr->egrMem.egrGenRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->egrMem.egrGenReg[index];
    }
    else
    /* Transmit Queue Configuration Registers       */
    if ((address & 0x1FFFF000) == 0x01A40000)
    {
        index = (address & 0xFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->egrMem.trQueueReg,
                         devMemInfoPtr->egrMem.trQueueRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->egrMem.trQueueReg[index];
    }
    else
    /* Port Watchdog Configuration Registers */
    if ((address & 0x1FFF0000) == 0x01A80000)
    {
        index = (address & 0xFFFF) / 0x200;
        CHECK_MEM_BOUNDS(devMemInfoPtr->egrMem.portWdReg,
                         devMemInfoPtr->egrMem.portWdRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->egrMem.portWdReg[index];
    }
    else
    /* Egress MIB Counters */
    if ((address & 0x1FFFFF00) == 0x01B40100)
    {
        index = (address & 0xFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->egrMem.egrMibCntReg,
                         devMemInfoPtr->egrMem.egrMibCntRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->egrMem.egrMibCntReg[index];
    }
    else
    /* Master XSMI Interface Configuration Registers */
    if ((address & 0x1FFF0000) == 0x01CC0000)
    {
        index = (address & 0xF) / 0x8;
        CHECK_MEM_BOUNDS(devMemInfoPtr->egrMem.xsmiReg,
                         devMemInfoPtr->egrMem.xsmiRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->egrMem.xsmiReg[index];
    }
    else
    if ((address & 0x01FF0000) == 0x01D40000)
    {
        index = (address & 0xFFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->egrMem.egrStcTblReg,
                         devMemInfoPtr->egrMem.egrStcTblRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->egrMem.egrStcTblReg[index];
    }
    else
    /* Hyper.GStack Ports MIB Counters */
    /* Egress STC Table */
    if ((address & 0x1FFF0000) >= 0x01C00000 &&
        (address & 0x1FFF0000) <= 0x01DC0000)
    {
        group = ((address >> SMEM_CHT_GROUP_INDEX_FIRST_BIT_CNS) & 0xF) / 0x4;
        index = (address & 0xFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->egrMem.hprMibCntReg[group],
                         devMemInfoPtr->egrMem.hprMibCntRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->egrMem.hprMibCntReg[group][index];
    }
    else
    /* Tail Drop Profile Configuration Registers */
    if ((address & 0x1FFF0000) == 0x01940000)
    {
        index = (address & 0xFFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->egrMem.tailDropReg,
                         devMemInfoPtr->egrMem.tailDropRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->egrMem.tailDropReg[index];
    }
    else
    /* Port<n> Token Bucket Configuration Registers */
    if ((address & 0x1FFF000F) == 0x01AC0008)
    {
        index = ((address & 0xFFFF) - 0x8) / 0x200;
        CHECK_MEM_BOUNDS(devMemInfoPtr->egrMem.egrRateShpReg[8],
                         devMemInfoPtr->egrMem.egrRateShpRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->egrMem.egrRateShpReg[8][index];
    }
    else
    /* Port<n> TC<t> Token Bucket Configuration Registers */
    if (address >= 0x01AC0000 && address <= 0x01AFFE00)
    {
        group = (address & 0x3FFFF) / 0x8000;
        index = (address & 0x3FFFF) / 0x200;
        CHECK_MEM_BOUNDS(devMemInfoPtr->egrMem.egrRateShpReg[group],
                         devMemInfoPtr->egrMem.egrRateShpRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->egrMem.egrRateShpReg[group][index];
    }
    else
    /* Txq Internal Registers */
    if ((address & 0x01FF0000) >= 0x01840000 &&
        (address & 0x01FF0000) <= 0x01930000)
    {
        group = ((address & 0x1C0000) >> 18) - 1;
        index = (address & 0x3FFFF) / 0x200;
        CHECK_MEM_BOUNDS(devMemInfoPtr->egrMem.txqInternalReg[group],
                         devMemInfoPtr->egrMem.txqInternalRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->egrMem.txqInternalReg[group][index];
    }

    return regValPtr;
}

/*******************************************************************************
*   smemCht2EtherBrdgReg
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
static GT_U32 *  smemCht2EtherBrdgReg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SMEM_CHT2_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32                  * regValPtr;
    GT_U32                    index;

    regValPtr = 0;
    devMemInfoPtr = (SMEM_CHT2_DEV_MEM_INFO  *)devObjPtr->deviceMemory;
    if ((address & 0xFF000000) == 0x02000000)
    {
        index = (address & 0x000FFFFF) / 4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->brdgMngMem.bridgeGenReg,
                         devMemInfoPtr->brdgMngMem.bridgeGenRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->brdgMngMem.bridgeGenReg[index];
    }

    return regValPtr;
}

/*******************************************************************************
*   smemCht2BufMngReg
*
* DESCRIPTION:
*       Describe a device's buffer managment registers and EPCL memory object
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
static GT_U32 *  smemCht2BufMemoryConfReg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SMEM_CHT2_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32                  * regValPtr;
    GT_U32                    index;

    regValPtr = 0;
    devMemInfoPtr = (SMEM_CHT2_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    /* Buffers Memory Registers */
    if ((address & 0xFFF00000) == 0x07800000)
    {
        index = (address & 0xFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->bufMem.bufMemReg,
                         devMemInfoPtr->bufMem.bufMemRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->bufMem.bufMemReg[index];
    }
    else
    /* Egress Policy configuration registers */
    if ((address & 0xFFFFF000) == 0x07C00000)
    {
        index = (address & 0xFFFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->pclMem.epclConfReg,
                         devMemInfoPtr->pclMem.epclConfRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->pclMem.epclConfReg[index];
    }
    else
    {
        /* ARP Entry (0<=n<1024) */
        index = (address & 0xffff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->ipMem.arpTblReg,
                         devMemInfoPtr->ipMem.arpTblRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->ipMem.arpTblReg[index];
    }


    return regValPtr;
}

/*******************************************************************************
*   smemCht2PortGroupConfReg
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
static GT_U32 *  smemCht2PortGroupConfReg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SMEM_CHT2_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32                  * regValPtr;
    GT_U32                    index;
    GT_U32                    groupNum;
    GT_U32                    gopMaskResult = 0;
    GT_U32                    macMibMaskResult = 0;
    GT_U32                    ledMaskResult = 0;

    regValPtr = 0;
    devMemInfoPtr = (SMEM_CHT2_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    groupNum = param;

    if (groupNum == 0)
    {
        gopMaskResult =     0x04004000;
        macMibMaskResult =  0x04010000;
        ledMaskResult =     0x04005100;
    }
    else
    if (groupNum == 1)
    {
        gopMaskResult =     0x04804000;
        macMibMaskResult =  0x04810000;
        ledMaskResult =     0x04805100;
    }
    else
    if (groupNum == 2)
    {
        gopMaskResult =     0x05004000;
        macMibMaskResult =  0x05010000;
        ledMaskResult =     0x05005100;
    }
    else
    if (groupNum == 3)
    {
        gopMaskResult =     0x05804000;
        macMibMaskResult =  0x05810000;
        ledMaskResult =     0x05805100;
    }

    /*  Group configuration registers */
    if ((address & 0xFFFFF000) == gopMaskResult)
    {
        index = (address & 0xFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->gopCnfMem.gopReg[groupNum],
                         devMemInfoPtr->gopCnfMem.gopRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->gopCnfMem.gopReg[groupNum][index];
    }
    else
    /* MAC MIB Counters registers */
    if ((address & 0xFFFF0000) == macMibMaskResult)
    {
        index = (address & 0xFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->gopCnfMem.macMibCountReg[groupNum],
                         devMemInfoPtr->gopCnfMem.macMibCountRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->gopCnfMem.macMibCountReg[groupNum][index];
    }
    else
    /* LED Interfaces Configuration Registers */
    if ((address & 0xFFFFFF00) == ledMaskResult)
    {
        index = (address & 0xFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->gopCnfMem.ledReg[groupNum],
                         devMemInfoPtr->gopCnfMem.ledRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->gopCnfMem.ledReg[groupNum][index];
    }

    return regValPtr;
}

/*******************************************************************************
*   smemCht2MacTableReg
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
static GT_U32 *  smemCht2MacTableReg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SMEM_CHT2_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32                  * regValPtr;
    GT_U32                    index;

    regValPtr = 0;
    devMemInfoPtr = (SMEM_CHT2_DEV_MEM_INFO  *)devObjPtr->deviceMemory;
    /* FDB Global Configuration Registers */
    if ((address & 0xFFF00000) == 0x06000000)
    {
        index = (address & 0xFFFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->macFdbMem.fdbReg,
                         devMemInfoPtr->macFdbMem.fdbRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->macFdbMem.fdbReg[index];
    }
    else
    if ((address & 0xFFF00000) == 0x06400000)
    {
        /* MAC Table Entry */
        index = (address & 0xFFFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->macFdbMem.macTblReg,
                         devMemInfoPtr->macFdbMem.macTblRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->macFdbMem.macTblReg[index];
    }

    return regValPtr;
}

/*******************************************************************************
*   smemCht2InterBufBankReg
*
* DESCRIPTION:
*       Describe a buffers memory banks data register and EPCL-ID configuration.
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
static GT_U32 *  smemCht2InterBufBankReg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SMEM_CHT2_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32                  * regValPtr;
    GT_U32                    index;
    GT_U32                    groupNum;

    regValPtr = 0;
    devMemInfoPtr = (SMEM_CHT2_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    groupNum = param;
    /* EPCL-ID configuration table */
    if ((address & 0xFFFF0000) == 0x07700000)
    {
        index = (address & 0xFFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->pclMem.epclConfigTblReg,
                         devMemInfoPtr->pclMem.epclConfigTblRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->pclMem.epclConfigTblReg[index];
    }
    /* Buffers Memory Bank0 Write Data Register number */
    else
    if (address == 0x06800000)
    {
        regValPtr = &devMemInfoPtr->banksMem.buffMemReg;
    }
    else
    if (address < 0x06980000)
    {
        groupNum = 0;
        index = (address & 0xFFFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->banksMem.bankMemReg[groupNum],
                         devMemInfoPtr->banksMem.bankMemRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->banksMem.bankMemReg[groupNum][index];
    }
    else
    /* Buffers Memory Bank1 Write Data Register number */
    {
        groupNum = 1;
        index = (((address  - 0x80000) & 0xFFFFF)) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->banksMem.bankMemReg[groupNum],
                         devMemInfoPtr->banksMem.bankMemRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->banksMem.bankMemReg[groupNum][index];
    }

    return regValPtr;
}

/*******************************************************************************
*   smemCht2BufMngReg
*
* DESCRIPTION:
*       Describe a device's buffer managment registers memory object.
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
*******************************************************************************/
static GT_U32 *  smemCht2BufMngReg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SMEM_CHT2_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32                  * regValPtr;
    GT_U32                    index;

    regValPtr = 0;
    devMemInfoPtr = (SMEM_CHT2_DEV_MEM_INFO *)devObjPtr->deviceMemory;

    /* Buffer management registers */
    if ((address & 0xFFFFF000) == 0x03000000)
    {
        index = (address & 0xFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->bufMngMem.bmReg,
                         devMemInfoPtr->bufMngMem.bmRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->bufMngMem.bmReg[index];
    }
    else
    if ((address & 0xFFFF0000) == 0x03010000)
    {
        index = (address & 0xFFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->bufMngMem.bmCntrReg,
                         devMemInfoPtr->bufMngMem.bmCntrRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->bufMngMem.bmCntrReg[index];
    }
    else
    {
        index = (address & 0xFFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->bufMngMem.bmLLReg,
                         devMemInfoPtr->bufMngMem.bmLLRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->bufMngMem.bmLLReg[index];
    }

    return regValPtr;
}

/*******************************************************************************
*  smemCht2RouterReg
*
* DESCRIPTION:
*      Router Engine registers and tables.
*
* INPUTS:
*       devObjPtr - device object PTR.
*       address     - Address for ASIC memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers's specific parameter.
*       inMemPtr    - Pointer to the memory to get register's content.
*
* OUTPUTS:
*
* RETURNS:
*        pointer to the memory location
*        NULL - if memory not exist
*
* COMMENTS:
*
*******************************************************************************/
static GT_U32 * smemCht2RouterReg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SMEM_CHT2_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32                  * regValPtr;
    GT_U32                    index;

    regValPtr = 0;
    devMemInfoPtr = (SMEM_CHT2_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    /* Global and Configuration registers. */
    if ((address & 0xFFFF0000) == 0x02800000)
    {
        index = (address & 0xFFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->routerMem.configReg,
                         devMemInfoPtr->routerMem.configRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->routerMem.configReg[index];
    }
    else
    /* TCAM registers. */
    if ((address & 0xFFF00000) == 0x02A00000)
    {
        index = (address & 0xFFFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->routerMem.tcamReg,
                         devMemInfoPtr->routerMem.tcamRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->routerMem.tcamReg[index];
    }
    else
    /* Action Table registers */
    if ((address & 0xFFFF0000) == 0x02900000)
    {
        index = (address & 0xFFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->routerMem.actionTblReg,
                         devMemInfoPtr->routerMem.actionTblRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->routerMem.actionTblReg[index];
    }
    else
    /* Next hop table */
    if ((address & 0xFFFF0000) == 0x02B00000)
    {
        index = (address & 0xFFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->routerMem.nextHopTblReg ,
                         devMemInfoPtr->routerMem.nextHopTblRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->routerMem.nextHopTblReg[index];
    }

    return   regValPtr;
}

/*******************************************************************************
*  smemCht2MllEngineReg
*
* DESCRIPTION:
*      Write MLL global and configuration Register
*
* INPUTS:
*       devObjPtr - device object PTR.
*       address     - Address for Cheetah2 packet processor memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers's specific parameter.
*       inMemPtr    - Pointer to the memory to get register's content.
*
* OUTPUTS:
*
* RETURNS:
*
*        pointer to the memory location
*        NULL - if memory not exist
* COMMENTS:
*
*******************************************************************************/
static GT_U32 * smemCht2MllEngineReg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SMEM_CHT2_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32                  * regValPtr;
    GT_U32                    index;

    regValPtr = 0;
    devMemInfoPtr = (SMEM_CHT2_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    /* MLL configuration register **/
    if ((address & 0xFFFF0000) == 0x0C800000)
    {
        index = (address & 0xFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->mllMem.mllConfReg,
                         devMemInfoPtr->mllMem.mllConfRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->mllMem.mllConfReg[index];
    }
    else
    /* MLL entry **/
    if ((address & 0xFFFF0000) == 0x0C880000)
    {
        index = (address & 0xFFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->mllMem.mllEntryReg,
                         devMemInfoPtr->mllMem.mllEntryRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->mllMem.mllEntryReg[index];
    }

    return   regValPtr;
}

/*******************************************************************************
*   smemCht2VlanTableReg
*
* DESCRIPTION:
*       Describe a device's Vlan table
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
static GT_U32 *  smemCht2VlanTableReg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SMEM_CHT2_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32              * regValPtr;
    GT_U32                index;

    regValPtr = 0;
    devMemInfoPtr = (SMEM_CHT2_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    if ((address & 0xFFF00000) == 0x0A400000)
    {
        /* VLAN Table */
        index = ((address & 0xFFFFF) / 0x10) * 4;
        index += ((address & 0xf) / 4);
        CHECK_MEM_BOUNDS(devMemInfoPtr->vlanTblMem.vlanTblReg,
                         devMemInfoPtr->vlanTblMem.vlanTblRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->vlanTblMem.vlanTblReg[index];
    }
    else
    if ((address & 0xFFF00000) == 0x0A200000)
    {
        /* Multicast table */
        index = (address & 0xFFFF0) / 0x10;
        CHECK_MEM_BOUNDS(devMemInfoPtr->vlanTblMem.mcstTblReg,
                         devMemInfoPtr->vlanTblMem.mcstTblRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->vlanTblMem.mcstTblReg[index];
    }
    else
    if ((address & 0xFFF00000) == 0x0A100000)
    {
        /* STP table */
        index = ((address & 0xFFFFF) / 0x10) * 2;
        index += ((address & 0xf) / 4);
        CHECK_MEM_BOUNDS(devMemInfoPtr->vlanTblMem.spanTblReg,
                         devMemInfoPtr->vlanTblMem.spanTblRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->vlanTblMem.spanTblReg[index];
    }
    else
    if ((address & 0xFF000000) == 0x0A000000)
    {
        /* VLAN configuration registers */
        index = (address & 0xFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->vlanTblMem.vlanCnfReg,
                         devMemInfoPtr->vlanTblMem.vlanCnfRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->vlanTblMem.vlanCnfReg[index];
    }

    return regValPtr;
}

/*******************************************************************************
*   smemCht2InitMemArray
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
static void smemCht2InitFuncArray
(
    INOUT SMEM_CHT2_DEV_MEM_INFO  * devMemInfoPtr
)
{
    GT_U32              i;

    for (i = 0; i < 64; i++)
    {
        devMemInfoPtr->common.specFunTbl[i].specFun    = smemChtFatalError;
    }
    devMemInfoPtr->common.specFunTbl[0].specFun        = smemCht2GlobalReg;

    devMemInfoPtr->common.specFunTbl[3].specFun        = smemCht2TransQueReg;

    devMemInfoPtr->common.specFunTbl[4].specFun        = smemCht2EtherBrdgReg;

    devMemInfoPtr->common.specFunTbl[5].specFun        = smemCht2RouterReg;

    devMemInfoPtr->common.specFunTbl[6].specFun        = smemCht2BufMngReg;

    devMemInfoPtr->common.specFunTbl[8].specFun        = smemCht2PortGroupConfReg;
    devMemInfoPtr->common.specFunTbl[8].specParam      = 0;

    devMemInfoPtr->common.specFunTbl[9].specFun        = smemCht2PortGroupConfReg;
    devMemInfoPtr->common.specFunTbl[9].specParam      = 1;

    devMemInfoPtr->common.specFunTbl[10].specFun       = smemCht2PortGroupConfReg;
    devMemInfoPtr->common.specFunTbl[10].specParam     = 2;

    devMemInfoPtr->common.specFunTbl[11].specFun       = smemCht2PortGroupConfReg;
    devMemInfoPtr->common.specFunTbl[11].specParam     = 3;

    devMemInfoPtr->common.specFunTbl[12].specFun       = smemCht2MacTableReg;

    devMemInfoPtr->common.specFunTbl[13].specFun       = smemCht2InterBufBankReg;
    devMemInfoPtr->common.specFunTbl[13].specParam     = 0;

    devMemInfoPtr->common.specFunTbl[14].specFun       = smemCht2InterBufBankReg;
    devMemInfoPtr->common.specFunTbl[14].specParam     = 1;

    devMemInfoPtr->common.specFunTbl[15].specFun       = smemCht2BufMemoryConfReg;

    devMemInfoPtr->common.specFunTbl[20].specFun       = smemCht2VlanTableReg;

    devMemInfoPtr->common.specFunTbl[21].specFun       = smemCht2TriSpeedReg;

    devMemInfoPtr->common.specFunTbl[22].specFun       = smemCht2PreEgressReg;

    devMemInfoPtr->common.specFunTbl[23].specFun       = smemCht2PclReg;

    devMemInfoPtr->common.specFunTbl[24].specFun       = smemCht2PolicerReg;

    devMemInfoPtr->common.specFunTbl[25].specFun       = smemCht2MllEngineReg ;

    devMemInfoPtr->common.specFunTbl[28].specFun       = smemCht2PolicerTbl;

    devMemInfoPtr->common.specFunTbl[30].specFun       = smemCht2VlanTbl;

    devMemInfoPtr->common.specFunTbl[31].specFun       = smemCht2ArpTbl;
}

/*******************************************************************************
*   smemCht2AllocSpecMemory
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
static void smemCht2AllocSpecMemory
(
    INOUT   SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_CHT2_DEV_MEM_INFO  * devMemInfoPtr
)
{
    GT_U8 i;

    /* init the internal memory */
    smemChtInitInternalSimMemory(devObjPtr,&devMemInfoPtr->common);

    /* Global register, TWSI, CPU port, SDMA and PCI internal registers */
    devMemInfoPtr->globalMem.globRegNum = GLB_REGS_NUM;
    devMemInfoPtr->globalMem.globReg =
        smemDevMemoryCalloc(devObjPtr, GLB_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->globalMem.globReg == 0)
    {
        skernelFatalError("smemCht2AllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->globalMem.sdmaRegNum = GLB_SDMA_REGS_NUM;
    devMemInfoPtr->globalMem.sdmaReg =
        smemDevMemoryCalloc(devObjPtr, GLB_SDMA_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->globalMem.sdmaReg == 0)
    {
        skernelFatalError("smemCht2AllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->globalMem.twsiIntRegsNum = TWSI_INT_REGS_NUM;
    devMemInfoPtr->globalMem.twsiIntRegs =
        smemDevMemoryCalloc(devObjPtr, TWSI_INT_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->globalMem.twsiIntRegs == 0)
    {
        skernelFatalError("smemCht2AllocSpecMemory: allocation error\n");
    }

    /* GPP Configuration Registers , Egress,
       Transmit Queue and VLAN Configuration Register Map Table,
       Port TxQ Configuration Register, Hyper.GStack Ports MIB Counters */
    devMemInfoPtr->egrMem.egrGenRegNum = EGR_GLB_REGS_NUM;
    devMemInfoPtr->egrMem.egrGenReg =
        smemDevMemoryCalloc(devObjPtr, EGR_GLB_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->egrMem.egrGenReg == 0)
    {
        skernelFatalError("smemCht2AllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->egrMem.trQueueRegNum = EGR_TRUNK_FILTER_REGS_NUM;
    devMemInfoPtr->egrMem.trQueueReg =
        smemDevMemoryCalloc(devObjPtr, EGR_TRUNK_FILTER_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->egrMem.trQueueReg == 0)
    {
        skernelFatalError("smemCht2AllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->egrMem.portWdRegNum = EGR_PORT_WD_REGS_NUM;
    devMemInfoPtr->egrMem.portWdReg =
        smemDevMemoryCalloc(devObjPtr, EGR_PORT_WD_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->egrMem.portWdReg == 0)
    {
        skernelFatalError("smemCht2AllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->egrMem.egrMibCntRegNum = EGR_MIB_CNT_REGS_NUM;
    devMemInfoPtr->egrMem.egrMibCntReg =
        smemDevMemoryCalloc(devObjPtr, EGR_MIB_CNT_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->egrMem.egrMibCntReg == 0)
    {
        skernelFatalError("smemCht2AllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->egrMem.hprMibCntRegNum = EGR_HYP_MIB_REGS_NUM;
    for (i = 0; i < 4; i++)
    {
        devMemInfoPtr->egrMem.hprMibCntReg[i] =
            smemDevMemoryCalloc(devObjPtr, EGR_HYP_MIB_REGS_NUM, sizeof(SMEM_REGISTER));
        if (devMemInfoPtr->egrMem.hprMibCntReg[i] == 0)
        {
            skernelFatalError("smemCht2AllocSpecMemory: allocation error\n");
        }
    }

    devMemInfoPtr->egrMem.txqInternalRegNum = TXQ_INTERNAL_REGS_NUM;
    for (i = 0; i < 4; i++)
    {
        devMemInfoPtr->egrMem.txqInternalReg[i] =
            smemDevMemoryCalloc(devObjPtr, TXQ_INTERNAL_REGS_NUM, sizeof(SMEM_REGISTER));
        if (devMemInfoPtr->egrMem.txqInternalReg[i] == 0)
        {
            skernelFatalError("smemCht2AllocSpecMemory: allocation error\n");
        }
    }

    devMemInfoPtr->egrMem.tailDropRegNum = EGR_TALE_DROP_REGS_NUM;
    devMemInfoPtr->egrMem.tailDropReg =
            smemDevMemoryCalloc(devObjPtr, EGR_TALE_DROP_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->egrMem.tailDropReg == 0)
    {
        skernelFatalError("smemCht2AllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->egrMem.egrRateShpRegNum = EGR_RATE_SHAPES_REGS_NUM;
    for (i = 0; i < 9; i++)
    {
        devMemInfoPtr->egrMem.egrRateShpReg[i] =
            smemDevMemoryCalloc(devObjPtr, EGR_RATE_SHAPES_REGS_NUM, sizeof(SMEM_REGISTER));
        if (devMemInfoPtr->egrMem.egrRateShpReg[i] == 0)
        {
            skernelFatalError("smemCht2AllocSpecMemory: allocation error\n");
        }
    }

    devMemInfoPtr->egrMem.xsmiRegNum = EGR_XSMI_REGS_NUM;
    devMemInfoPtr->egrMem.xsmiReg =
            smemDevMemoryCalloc(devObjPtr, EGR_XSMI_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->egrMem.xsmiReg == 0)
    {
        skernelFatalError("smemChtAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->egrMem.egrStcTblRegNum = EGR_STC_REGS_NUM;
    devMemInfoPtr->egrMem.egrStcTblReg =
            smemDevMemoryCalloc(devObjPtr, EGR_STC_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->egrMem.egrStcTblReg == 0)
    {
        skernelFatalError("smemChtAllocSpecMemory: allocation error\n");
    }

    /* Bridge Managment registers memory object */

    devMemInfoPtr->brdgMngMem.bridgeGenRegsNum = BRDG_ETH_BRDG_REGS_NUM;
    devMemInfoPtr->brdgMngMem.bridgeGenReg =
            smemDevMemoryCalloc(devObjPtr, BRDG_ETH_BRDG_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->brdgMngMem.bridgeGenReg == 0)
    {
        skernelFatalError("smemCht2AllocSpecMemory: allocation error\n");
    }

    /* Buffer managment registers memory object */
    devMemInfoPtr->bufMngMem.bmRegNum = BUF_MNG_REGS_NUM;
    devMemInfoPtr->bufMngMem.bmReg =
            smemDevMemoryCalloc(devObjPtr, BUF_MNG_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->bufMngMem.bmReg == 0)
    {
        skernelFatalError("smemCht2AllocSpecMemory: allocation error\n");
    }
    devMemInfoPtr->bufMngMem.bmLLRegNum = BUF_MNG_LL_REGS_NUM;
    devMemInfoPtr->bufMngMem.bmLLReg =
            smemDevMemoryCalloc(devObjPtr, BUF_MNG_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->bufMngMem.bmLLReg == 0)
    {
        skernelFatalError("smemCht2AllocSpecMemory: allocation error\n");
    }
    devMemInfoPtr->bufMngMem.bmCntrRegNum = BUF_MNG_LL_REGS_NUM;
    devMemInfoPtr->bufMngMem.bmCntrReg =
            smemDevMemoryCalloc(devObjPtr, BUF_MNG_LL_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->bufMngMem.bmCntrReg == 0)
    {
        skernelFatalError("smemCht2AllocSpecMemory: allocation error\n");
    }

    /* Ports groups configuration registers (port0 through port5),
       LEDs interfaces configuration registers and Master SMI
       interfaces registers                                         */

    devMemInfoPtr->gopCnfMem.gopRegNum = GOP_CONF_REGS_NUM;
    devMemInfoPtr->gopCnfMem.macMibCountRegNum = GOP_MAC_MIB_REGS_NUM;
    devMemInfoPtr->gopCnfMem.ledRegNum = GOP_LED_REGS_NUM;
    for (i = 0; i < 4; i++)
    {
        devMemInfoPtr->gopCnfMem.gopReg[i] =
            smemDevMemoryCalloc(devObjPtr, devMemInfoPtr->gopCnfMem.gopRegNum, sizeof(SMEM_REGISTER));
        if (devMemInfoPtr->gopCnfMem.gopReg[i] == 0)
        {
            skernelFatalError("smemCht2AllocSpecMemory: allocation error\n");
        }

        devMemInfoPtr->gopCnfMem.macMibCountReg[i] =
            smemDevMemoryCalloc(devObjPtr, devMemInfoPtr->gopCnfMem.macMibCountRegNum,
                                                    sizeof(SMEM_REGISTER));
        if (devMemInfoPtr->gopCnfMem.macMibCountReg[i] == 0)
        {
            skernelFatalError("smemCht2AllocSpecMemory: allocation error\n");
        }

        devMemInfoPtr->gopCnfMem.ledReg[i] =
            smemDevMemoryCalloc(devObjPtr, devMemInfoPtr->gopCnfMem.ledRegNum, sizeof(SMEM_REGISTER));
        if (devMemInfoPtr->gopCnfMem.ledReg[i] == 0)
        {
            skernelFatalError("smemCht2AllocSpecMemory: allocation error\n");
        }
    }

    /* Bridge registers and FDB */
    devMemInfoPtr->macFdbMem.fdbRegNum = MAC_FDB_REGS_NUM;
    devMemInfoPtr->macFdbMem.fdbReg =
                                smemDevMemoryCalloc(devObjPtr, MAC_FDB_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->macFdbMem.fdbReg == 0)
    {
        skernelFatalError("smemCht2AllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->macFdbMem.macTblRegNum = MAC_TBL_REGS_NUM;
    devMemInfoPtr->macFdbMem.macTblReg =
            smemDevMemoryCalloc(devObjPtr, MAC_TBL_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->macFdbMem.macTblReg == 0)
    {
        skernelFatalError("smemCht2AllocSpecMemory: allocation error\n");
    }

    /* Buffers memory banks data register */
    devMemInfoPtr->banksMem.bankMemRegNum = BANK_MEM_REGS_NUM;
    for (i = 0; i < 2; i++)
    {
        devMemInfoPtr->banksMem.bankMemReg[i] =
            smemDevMemoryCalloc(devObjPtr, BANK_MEM_REGS_NUM, sizeof(SMEM_REGISTER));
        if (devMemInfoPtr->banksMem.bankMemReg[i] == 0)
        {
            skernelFatalError("smemCht2AllocSpecMemory: allocation error\n");
        }
    }

    /* Buffers Memory, Ingress MAC Errors Indications and
       Egress Header Alteration Register Map Table registers            */
    devMemInfoPtr->bufMem.bufMemRegNum = BUF_MEM_REGS_NUM;
    devMemInfoPtr->bufMem.bufMemReg =
            smemDevMemoryCalloc(devObjPtr, BUF_MEM_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->bufMem.bufMemReg == 0)
    {
        skernelFatalError("smemCht2AllocSpecMemory: allocation error\n");
    }

    /* VLAN Table, Multicast Groups Table, and Span State Group Table */
    devMemInfoPtr->vlanTblMem.vlanCnfRegNum = VTL_VLAN_CONF_REGS_NUM;
    devMemInfoPtr->vlanTblMem.vlanCnfReg =
            smemDevMemoryCalloc(devObjPtr, VTL_VLAN_CONF_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->vlanTblMem.vlanCnfReg == 0)
    {
        skernelFatalError("smemCht2AllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->vlanTblMem.vlanTblRegNum = VTL_VLAN_TBL_REGS_NUM;
    devMemInfoPtr->vlanTblMem.vlanTblReg =
            smemDevMemoryCalloc(devObjPtr, VTL_VLAN_TBL_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->vlanTblMem.vlanTblReg == 0)
    {
        skernelFatalError("smemCht2AllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->vlanTblMem.mcstTblRegNum = VTL_MCST_TBL_REGS_NUM;
    devMemInfoPtr->vlanTblMem.mcstTblReg =
            smemDevMemoryCalloc(devObjPtr, VTL_MCST_TBL_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->vlanTblMem.mcstTblReg == 0)
    {
        skernelFatalError("smemCht2AllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->vlanTblMem.spanTblRegNum = VTL_STP_TBL_REGS_NUM;
    devMemInfoPtr->vlanTblMem.spanTblReg =
            smemDevMemoryCalloc(devObjPtr, VTL_STP_TBL_REGS_NUM/*5000*/, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->vlanTblMem.spanTblReg == 0)
    {
        skernelFatalError("smemCht2AllocSpecMemory: allocation error\n");
    }

    /* Tri-Speed Ports MAC */

    devMemInfoPtr->triSpeedPortsMem.triSpeedPortsRegNum = TRI_SPEED_REGS_NUM;
    devMemInfoPtr->triSpeedPortsMem.triSpeedPortsReg =
            smemDevMemoryCalloc(devObjPtr, TRI_SPEED_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->triSpeedPortsMem.triSpeedPortsReg == 0)
    {
        skernelFatalError("smemCht2AllocSpecMemory: allocation error\n");
    }

    /* Pre-Egress Engine Registers Map Table, Trunk Table,
       QoSProfile to QoS Table, CPU Code Table, Data Access Statistical Rate
       Limits Table Data Access, Ingress STC Table Registers                */

    devMemInfoPtr->preegressMem.portsRegNum = PRE_EGR_PORTS_REGS_NUM;
    devMemInfoPtr->preegressMem.portsReg =
            smemDevMemoryCalloc(devObjPtr, PRE_EGR_PORTS_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->preegressMem.portsReg == 0)
    {
        skernelFatalError("smemCht2AllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->preegressMem.genRegNum = PRE_EGR_GEN_REGS_NUM;
    devMemInfoPtr->preegressMem.genReg =
            smemDevMemoryCalloc(devObjPtr, PRE_EGR_GEN_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->preegressMem.genReg == 0)
    {
        skernelFatalError("smemCht2AllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->preegressMem.trunkTblRegNum = PRE_EGR_TRUNK_TBL_REGS_NUM;
    devMemInfoPtr->preegressMem.trunkTblReg =
            smemDevMemoryCalloc(devObjPtr, PRE_EGR_TRUNK_TBL_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->preegressMem.trunkTblReg == 0)
    {
        skernelFatalError("smemCht2AllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->preegressMem.qosTblRegNum = PRE_QOS_TBL_REGS_NUM;
    devMemInfoPtr->preegressMem.qosTblReg =
            smemDevMemoryCalloc(devObjPtr, PRE_QOS_TBL_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->preegressMem.qosTblReg == 0)
    {
        skernelFatalError("smemCht2AllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->preegressMem.cpuCodeTblRegNum = PRE_CPU_CODE_TBL_REGS_NUM;
    devMemInfoPtr->preegressMem.cpuCodeTblReg =
            smemDevMemoryCalloc(devObjPtr, PRE_CPU_CODE_TBL_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->preegressMem.cpuCodeTblReg == 0)
    {
        skernelFatalError("smemCht2AllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->preegressMem.statRateTblRegNum = PRE_STAT_RATE_TBL_REGS_NUM;
    devMemInfoPtr->preegressMem.statRateTblReg =
            smemDevMemoryCalloc(devObjPtr, PRE_STAT_RATE_TBL_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->preegressMem.statRateTblReg == 0)
    {
        skernelFatalError("smemCht2AllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->preegressMem.portInStcTblRegNum = PRE_PORTS_INGR_STC_REGS_NUM;
    devMemInfoPtr->preegressMem.portInStcTblReg =
            smemDevMemoryCalloc(devObjPtr, PRE_PORTS_INGR_STC_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->preegressMem.portInStcTblReg == 0)
    {
        skernelFatalError("smemCht2AllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->preegressMem.trunkMemberTblRegNum = TRUNK_MEMBER_TBL_REGS_NUM;
    devMemInfoPtr->preegressMem.trunkMemberTblReg =
            smemDevMemoryCalloc(devObjPtr, TRUNK_MEMBER_TBL_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->preegressMem.trunkMemberTblReg == 0)
    {
        skernelFatalError("smemCht2AllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->preegressMem.ingFwrdRestConfigRegNum = ING_RESTRICT_REGS_NUM;
    devMemInfoPtr->preegressMem.ingFwrdRestConfigReg =
            smemDevMemoryCalloc(devObjPtr, ING_RESTRICT_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->preegressMem.ingFwrdRestConfigReg == 0)
    {
        skernelFatalError("smemCht2AllocSpecMemory: allocation error\n");
    }

    /* PCL registers and TCAM memory space   */
    devMemInfoPtr->pclMem.pclConfRegNum = PCL_CONF_REGS_NUM;
    devMemInfoPtr->pclMem.pclConfReg =
            smemDevMemoryCalloc(devObjPtr, PCL_CONF_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->pclMem.pclConfReg == 0)
    {
        skernelFatalError("smemCht2AllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->pclMem.accessDataRegNum = PCL_ACCESS_DATA_REGS_NUM;
    devMemInfoPtr->pclMem.accessDataReg =
            smemDevMemoryCalloc(devObjPtr, PCL_ACCESS_DATA_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->pclMem.accessDataReg == 0)
    {
        skernelFatalError("smemCht2AllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->pclMem.epclConfRegNum = EPCL_CONF_REGS_NUM;
    devMemInfoPtr->pclMem.epclConfReg =
            smemDevMemoryCalloc(devObjPtr, EPCL_CONF_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->pclMem.epclConfReg == 0)
    {
        skernelFatalError("smemCht2AllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->pclMem.pceActionTblRegNum = PCL_ACTION_TBL_REGS_NUM;
    devMemInfoPtr->pclMem.pceActionTblReg =
            smemDevMemoryCalloc(devObjPtr, PCL_ACTION_TBL_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->pclMem.pceActionTblReg == 0)
    {
        skernelFatalError("smemCht2AllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->pclMem.tcamRegNum = PCL_TCAM_REGS_NUM;
    devMemInfoPtr->pclMem.tcamReg =
            smemDevMemoryCalloc(devObjPtr, PCL_TCAM_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->pclMem.tcamReg == 0)
    {
        skernelFatalError("smemCht2AllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->pclMem.pclIdTblRegNum = PCL_ID_CONF_TBL_REGS_NUM;
    devMemInfoPtr->pclMem.pclIdTblReg =
            smemDevMemoryCalloc(devObjPtr, PCL_ID_CONF_TBL_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->pclMem.pclIdTblReg == 0)
    {
        skernelFatalError("smemCht2AllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->pclMem.pceRuleMatchCntRegNum = PCL_RULE_MATCH_CNT_REGS_NUM;
    devMemInfoPtr->pclMem.pceRuleMatchCntReg =
            smemDevMemoryCalloc(devObjPtr, PCL_RULE_MATCH_CNT_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->pclMem.pceRuleMatchCntReg == 0)
    {
        skernelFatalError("smemCht2AllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->pclMem.epclConfigTblRegNum = EPCL_ID_CONF_TBL_REGS_NUM;
    devMemInfoPtr->pclMem.epclConfigTblReg =
            smemDevMemoryCalloc(devObjPtr, EPCL_ID_CONF_TBL_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->pclMem.epclConfigTblReg == 0)
    {
        skernelFatalError("smemCht2AllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->pclMem.pclIntRegNum = PCL_INTERN_REGS_NUM;
    devMemInfoPtr->pclMem.pclIntReg =
            smemDevMemoryCalloc(devObjPtr, PCL_INTERN_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->pclMem.pclIntReg == 0)
    {
        skernelFatalError("smemCht2AllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->pclMem.pclIdTblRegNum = PCL_ID_CONF_TBL_REGS_NUM;
    devMemInfoPtr->pclMem.pclIdTblReg =
            smemDevMemoryCalloc(devObjPtr, PCL_ID_CONF_TBL_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->pclMem.pclIdTblReg == 0)
    {
        skernelFatalError("smemChtAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->pclMem.tcpPorRangeTblRegNum = PCL_TCP_TBL_REGS_NUM;
    devMemInfoPtr->pclMem.tcpPorRangeTblReg =
            smemDevMemoryCalloc(devObjPtr, PCL_TCP_TBL_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->pclMem.tcpPorRangeTblReg == 0)
    {
        skernelFatalError("smemChtAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->ipMem.ipProtCpuCodeRegNum = IP_PROT_REGS_NUM;
    devMemInfoPtr->ipMem.ipProtCpuCodeReg =
            smemDevMemoryCalloc(devObjPtr, PCL_TCP_TBL_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->ipMem.ipProtCpuCodeReg == 0)
    {
        skernelFatalError("smemChtAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->ipMem.ipProtCpuCodeRegNum = IP_PROT_REGS_NUM;
    devMemInfoPtr->ipMem.ipProtCpuCodeReg =
            smemDevMemoryCalloc(devObjPtr, PCL_TCP_TBL_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->ipMem.ipProtCpuCodeReg == 0)
    {
        skernelFatalError("smemChtAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->preegressMem.toCpuRateLimiterRegNum = CPU_RATE_LIMIT_REGS_NUM;
    devMemInfoPtr->preegressMem.toCpuRateLimiterReg =
            smemDevMemoryCalloc(devObjPtr, CPU_RATE_LIMIT_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->preegressMem.toCpuRateLimiterReg == 0)
    {
        skernelFatalError("smemChtAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->preegressMem.portIngressStcTblRegNum = INGR_STC_REGS_NUM;
    devMemInfoPtr->preegressMem.portIngressStcTblReg =
            smemDevMemoryCalloc(devObjPtr, INGR_STC_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->preegressMem.portIngressStcTblReg == 0)
    {
        skernelFatalError("smemChtAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->preegressMem.statRateLimitRegNum = 100;
    devMemInfoPtr->preegressMem.statRateLimitReg         =
            smemDevMemoryCalloc(devObjPtr, 100, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->preegressMem.statRateLimitReg == 0)
    {
        skernelFatalError("smemChtAllocSpecMemory: allocation error\n");
    }

    /* Policer registers and meters memory space */
    devMemInfoPtr->policerMem.policerRegNum = POLICER_REGS_NUM;
    devMemInfoPtr->policerMem.policerReg =
            smemDevMemoryCalloc(devObjPtr, POLICER_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->policerMem.policerReg == 0)
    {
        skernelFatalError("smemCht2AllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->policerMem.policerTblRegNum = POLICER_TBL_REGS_NUM;
    devMemInfoPtr->policerMem.policerTblReg =
            smemDevMemoryCalloc(devObjPtr, POLICER_TBL_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->policerMem.policerTblReg == 0)
    {
        skernelFatalError("smemCht2AllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->policerMem.policerQosRmTblRegNum =
                POLICER_QOS_REM_TBL_REGS_NUM;
    devMemInfoPtr->policerMem.policerQosRmTblReg =
                smemDevMemoryCalloc(devObjPtr, POLICER_QOS_REM_TBL_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->policerMem.policerQosRmTblReg == 0)
    {
        skernelFatalError("smemCht2AllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->policerMem.policerCntTblRegNum = POLICER_COUNT_TBL_REGS_NUM;
    devMemInfoPtr->policerMem.policerCntTblReg =
                smemDevMemoryCalloc(devObjPtr, POLICER_COUNT_TBL_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->policerMem.policerCntTblReg == 0)
    {
        skernelFatalError("smemCht2AllocSpecMemory: allocation error\n");
    }

    /* PCI Registers */
    devMemInfoPtr->pciMem.pciRegNum = PCI_REGS_NUM;
    devMemInfoPtr->pciMem.pciReg = smemDevMemoryCalloc(devObjPtr, PCI_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->pciMem.pciReg == 0)
    {
        skernelFatalError("smemCht2AllocSpecMemory: allocation error\n");
    }

    /* ARP table memory */
    devMemInfoPtr->ipMem.arpTblRegNum = ARP_TABLE_REGS_NUM;
    devMemInfoPtr->ipMem.arpTblReg =
                smemDevMemoryCalloc(devObjPtr, ARP_TABLE_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->ipMem.arpTblReg == 0)
    {
        skernelFatalError("smemCht2AllocSpecMemory: allocation error\n");
    }

    /*  VLAN/Port MAC SA Table */
    devMemInfoPtr->ipMem.macSaTblRegNum = MAC_SA_TABLE_REGS_NUM;
    devMemInfoPtr->ipMem.macSaTblReg =
        smemDevMemoryCalloc(devObjPtr, MAC_SA_TABLE_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->ipMem.macSaTblReg == 0)
    {
        skernelFatalError("smemCht2AllocSpecMemory: allocation error\n");
    }

    /*  Router Tables and Registers */
    devMemInfoPtr->routerMem.configRegNum  = ROUTER_CONF_REGS_NUM;
    devMemInfoPtr->routerMem.configReg =
                            smemDevMemoryCalloc(devObjPtr, ROUTER_CONF_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->routerMem.configReg == 0)
    {
        skernelFatalError("smemCht2AllocSpecMemory: allocation error\n");
    }
    devMemInfoPtr->routerMem.tcamRegNum  = ROUTER_TCAM_REGS_NUM;
    devMemInfoPtr->routerMem.tcamReg =
                    smemDevMemoryCalloc(devObjPtr, ROUTER_TCAM_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->routerMem.tcamReg == 0)
    {
        skernelFatalError("smemCht2AllocSpecMemory: allocation error\n");
    }
    devMemInfoPtr->routerMem.actionTblRegNum  = LTT_ACTION_TBL_REGS_NUM;
    devMemInfoPtr->routerMem.actionTblReg =
                        smemDevMemoryCalloc(devObjPtr, LTT_ACTION_TBL_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->routerMem.actionTblReg == 0)
    {
        skernelFatalError("smemCht2AllocSpecMemory: allocation error\n");
    }
    devMemInfoPtr->routerMem.nextHopTblRegNum   = NEXT_HOP_TBL_REGS_NUM;
    devMemInfoPtr->routerMem.nextHopTblReg =
        smemDevMemoryCalloc(devObjPtr, NEXT_HOP_TBL_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->routerMem.nextHopTblReg == 0)
    {
        skernelFatalError("smemCht2AllocSpecMemory: allocation error\n");
    }

    /* MLL Engine Configuration Registers */
    devMemInfoPtr->mllMem.mllConfRegNum = MLL_GLOB_REGS_NUM;
    devMemInfoPtr->mllMem.mllConfReg =
                        smemDevMemoryCalloc(devObjPtr, MLL_GLOB_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->mllMem.mllConfReg == 0)
    {
        skernelFatalError("smemCht2AllocSpecMemory: allocation error\n");
    }
    devMemInfoPtr->mllMem.mllEntryRegNum = MLL_TBL_REGS_NUM;
    devMemInfoPtr->mllMem.mllEntryReg =
                        smemDevMemoryCalloc(devObjPtr, MLL_TBL_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->mllMem.mllEntryReg == 0)
    {
        skernelFatalError("smemCht2AllocSpecMemory: allocation error\n");
    }


    memset(devMemInfoPtr->common.auqFifoMem.macUpdFifoRegs, 0xFF,
        MAC_UPD_FIFO_REGS_NUM * sizeof(SMEM_REGISTER));


    devObjPtr->devFindMemFunPtr = (void *)smemCht2FindMem;
}

/*******************************************************************************
*   smemCht2PciReg
*
* DESCRIPTION:
*       PCI memory access.
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
static GT_U32 *  smemCht2PciReg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SMEM_CHT2_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32                  * regValPtr;
    GT_U32                    index;

    regValPtr = 0;
    devMemInfoPtr = (SMEM_CHT2_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    if ((address & 0xFFFFFFFF) == 0x00000114)
    {
        /* Reference to global interrupt cause address */
        regValPtr = smemCht2GlobalReg(devObjPtr, accessType,
                                      SMEM_CHT_GLB_INT_CAUSE_REG(devObjPtr), memSize, param);
    }
    else
    {
        index = (address & 0xFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->pciMem.pciReg,
                         devMemInfoPtr->pciMem.pciRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->pciMem.pciReg[index];
    }

    return regValPtr;
}

/*******************************************************************************
*   smemCht2PclReg
*
* DESCRIPTION:
*       PCL registers and TCAM memory space access.
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
static GT_U32 *  smemCht2PclReg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SMEM_CHT2_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32                  * regValPtr;
    GT_U32                    index;

    regValPtr = 0;
    devMemInfoPtr = (SMEM_CHT2_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    /* PCL Registers */
    if ((address & 0xFFFF0000) == 0x0B810000)
    {
        index = (address & 0xFFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->pclMem.pclConfReg,
                         devMemInfoPtr->pclMem.pclConfRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->pclMem.pclConfReg[index];
    }
    else
    if ((address & 0xFFFFE000) == 0x0B800000)
    {
        index = (address & 0x1FFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->pclMem.accessDataReg,
                         devMemInfoPtr->pclMem.accessDataRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->pclMem.accessDataReg[index];
    }
    else
    /* PCL action table */
    if ((address & 0xFFFF0000) == 0x0b8C0000)
    {
        index = (address & 0xFFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->pclMem.pceActionTblReg,
                         devMemInfoPtr->pclMem.pceActionTblRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->pclMem.pceActionTblReg[index];
    }
    else
    /* TCAM Registers */
    if ( ((address & 0xFFF80000) == 0x0B880000) ||
         ((address & 0xFFF90000) == 0x0B890000) )
    {
        index = ((address  - 0x80000 )& 0xFFFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->pclMem.tcamReg,
                         devMemInfoPtr->pclMem.tcamRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->pclMem.tcamReg[index];
    }
    else
    /* PCL-ID table */
    if ((address & 0xFFF40000) == 0x0B840000)
    {
        index = (address & 0xFFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->pclMem.pclIdTblReg,
                         devMemInfoPtr->pclMem.pclIdTblRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->pclMem.pclIdTblReg[index];
    }
    else
    /* PCE action table */
    if ((address & 0xFFFF1000) == 0x0B801000)
    {
        index = (address & 0xFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->pclMem.pceRuleMatchCntReg,
                         devMemInfoPtr->pclMem.pceRuleMatchCntRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->pclMem.pceRuleMatchCntReg[index];
    }
    /* PCL internal registers */
    if ((address & 0xFFFF000) == 0x0B820000)
    {
        index = (address & 0xFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->pclMem.pclIntReg,
                         devMemInfoPtr->pclMem.pclIntRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->pclMem.pclIntReg[index];
    }

    return regValPtr;
}

/*******************************************************************************
*   smemCht2PolicerReg
*
* DESCRIPTION:
*       Policer registers and meters memory space access
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
static GT_U32 *  smemCht2PolicerReg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SMEM_CHT2_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32                  * regValPtr;
    GT_32                     index;
    GT_U32                    tbl;

    regValPtr = 0;
    devMemInfoPtr = (SMEM_CHT2_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    /* Policer Configuration Registers */
    if ((address & 0xFFFFFF00) == 0x0C000000)
    {
        index = (address & 0xFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->policerMem.policerReg,
                         devMemInfoPtr->policerMem.policerRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->policerMem.policerReg[index];
    }
    else
    {
        tbl = ((address >> SMEM_CHT_SUB_UNIT_INDEX_FIRST_BIT_CNS) & 0xf ) - 1 ;
        index = (address & 0xffff) / 0x4;

        /* Policers Table Entry<n> (0<=n<256) */
        if (tbl == SMEM_CHT_POLICER_TBL_SUB_UNIT_INDEX_CNS)
        {
            CHECK_MEM_BOUNDS(devMemInfoPtr->policerMem.policerTblReg,
                             devMemInfoPtr->policerMem.policerTblRegNum,
                             index, memSize);
            regValPtr = &devMemInfoPtr->policerMem.policerTblReg[index];
        }
        else
        /* Policers QoS Remarking and Initial DP Table Entry<n> (0<=n<72) */
        if (tbl == SMEM_CHT_POLICER_QOS_REM_UNIT_INDEX_CNS)
        {
            CHECK_MEM_BOUNDS(devMemInfoPtr->policerMem.policerQosRmTblReg,
                             devMemInfoPtr->policerMem.policerQosRmTblRegNum,
                             index, memSize);
            regValPtr = &devMemInfoPtr->policerMem.policerQosRmTblReg[index];
        }
        else
        /* Policers Counters Table Entry<n> (0<=n<16) */
        if (tbl == SMEM_CHT_POLICER_COUNTER_UNIT_INDEX_CNS)
        {
            index = (address & 0xf0) / 0x10 + (address & 0xf) / 0x4 ;
            /* Policers Table Entry */
            CHECK_MEM_BOUNDS(devMemInfoPtr->policerMem.policerCntTblReg,
                             devMemInfoPtr->policerMem.policerCntTblRegNum,
                             index, memSize);
            regValPtr = &devMemInfoPtr->policerMem.policerCntTblReg[index];
        }
    }

    return regValPtr;
}

/*******************************************************************************
*   smemCht2TriSpeedReg
*
* DESCRIPTION:
*      Describe a Tri-Speed Ports MAC, CPU Port MAC, and 1.25 Gbps SERDES
*      Configuration Registers Map Table,  Hyper.GStack Ports MAC and
*      XAUI PHYs Configuration Registers Map Table memory access
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
static GT_U32 *  smemCht2TriSpeedReg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SMEM_CHT2_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32                  * regValPtr;
    GT_32                     index;

    regValPtr = 0;
    devMemInfoPtr = (SMEM_CHT2_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    /* Tri-Speed Ports */
    if ((address & 0xFFFF0000) == 0x0A800000)
    {
        index = (address & 0x3FFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->triSpeedPortsMem.triSpeedPortsReg,
                         devMemInfoPtr->triSpeedPortsMem.triSpeedPortsRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->triSpeedPortsMem.triSpeedPortsReg[index];
    }

    return regValPtr;
}

/*******************************************************************************
*   smemCht2PreEgressReg
*
* DESCRIPTION:
*      Pre-Egress Engine Registers Map Table, Trunk Table,
*      QoSProfile to QoS Table, CPU Code Table,
*      Data Access Statistical Rate Limits Table Data Access,
*      Ingress STC Table Registers memory access
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
static GT_U32 *  smemCht2PreEgressReg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SMEM_CHT2_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32                  * regValPtr;
    GT_U32                    index;
    GT_U32                                          tbl;

    regValPtr = 0;
    devMemInfoPtr = (SMEM_CHT2_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    /* Pre-Egress Engine Registers */
    if ((address & 0xFFFFFF00) == 0x0B000000)
    {
        index = (address & 0xFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->preegressMem.portsReg,
                         devMemInfoPtr->preegressMem.portsRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->preegressMem.portsReg[index];
    }
    else if ((address & 0xFFFFFF00) == 0x00b007000)
    {
        index = (address & 0xFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->pclMem.tcpPorRangeTblReg,
                         devMemInfoPtr->pclMem.tcpPorRangeTblRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->pclMem.tcpPorRangeTblReg[index];
    }
    else if ((address & 0xFFFFFF00) == 0x00b008000)
    {
        index = (address & 0xFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->ipMem.ipProtCpuCodeReg,
                         devMemInfoPtr->ipMem.ipProtCpuCodeRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->ipMem.ipProtCpuCodeReg[index];
    }
    else if ((address & 0xFFFFF000) == 0x0B009000)
    {
        index = (address & 0xFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->preegressMem.trunkMemberTblReg,
                         devMemInfoPtr->preegressMem.trunkMemberTblRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->preegressMem.trunkMemberTblReg[index];
    }
    else if ((address & 0xFFFFFFF0) == 0x0B020000)
    {
        index = (address & 0xFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->preegressMem.ingFwrdRestConfigReg,
                         devMemInfoPtr->preegressMem.ingFwrdRestConfigRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->preegressMem.ingFwrdRestConfigReg[index];
    }
    else if ((address & 0xFFFFFF00) == 0x0b080000)
    {
        index = (address & 0xFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->preegressMem.toCpuRateLimiterReg,
                         devMemInfoPtr->preegressMem.toCpuRateLimiterRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->preegressMem.toCpuRateLimiterReg[index];
    }
    else if ((address & 0xFFFF0000) == 0x0b040000)
    {
        index = (address & 0xFFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->preegressMem.portIngressStcTblReg,
                         devMemInfoPtr->preegressMem.portIngressStcTblRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->preegressMem.portIngressStcTblReg[index];
    }
    else if ((address & 0xFFFFFF00) == 0x0B100000)
    {
        index = (address & 0xFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->preegressMem.statRateLimitReg,
                         devMemInfoPtr->preegressMem.statRateLimitRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->preegressMem.statRateLimitReg[index];
    }
    else if ((address & 0xFFFFF000) ==  0x0b040000)
    {
        index = (address & 0xfff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->preegressMem.portInStcTblReg,
                         devMemInfoPtr->preegressMem.portInStcTblRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->preegressMem.portInStcTblReg[index];
    }
    else
    {
        tbl = (address >> SMEM_CHT_SUB_UNIT_INDEX_FIRST_BIT_CNS) & 0xf;
        index = (address & 0xfff) / 0x4;
        /* Trunk table Trunk<n> Member<i> Entry */
        if (tbl == SMEM_CHT2_TRUNK_TBL_MEMBER_SUB_UNIT_INDEX_CNS)
        {
            CHECK_MEM_BOUNDS(devMemInfoPtr->preegressMem.trunkTblReg[index],
                             devMemInfoPtr->preegressMem.trunkTblRegNum,
                             index, memSize);
            regValPtr = &devMemInfoPtr->preegressMem.trunkTblReg[index];
        }
        else
        /* QoSProfile to QoS table Entry<n> (0<=n<72) */
        if (tbl == SMEM_CHT2_QOS_PROFILE_UNIT_INDEX_CNS)
        {
            CHECK_MEM_BOUNDS(devMemInfoPtr->preegressMem.qosTblReg,
                             devMemInfoPtr->preegressMem.qosTblRegNum,
                             index, memSize);
            regValPtr = &devMemInfoPtr->preegressMem.qosTblReg[index];
        }
        else
        /* CPU Code Table Entry<n> (0<=n<256) */
        if (tbl == SMEM_CHT2_CPU_CODE_UNIT_INDEX_CNS)
        {
            CHECK_MEM_BOUNDS(devMemInfoPtr->preegressMem.cpuCodeTblReg,
                             devMemInfoPtr->preegressMem.cpuCodeTblRegNum,
                             index, memSize);
            regValPtr = &devMemInfoPtr->preegressMem.cpuCodeTblReg[index];
        }
        else
        /* Statistical Rate Limits Table Entry<n> (0<=n<32) */
        if (tbl == SMEM_CHT2_STAT_RATE_LIM_UNIT_INDEX_CNS)
        {
            CHECK_MEM_BOUNDS(devMemInfoPtr->preegressMem.statRateTblReg,
                             devMemInfoPtr->preegressMem.statRateTblRegNum,
                             index, memSize);
            regValPtr = &devMemInfoPtr->preegressMem.statRateTblReg[index];
        }
        else
        {
            index = (address & 0xFFFFF) / 0x1000;
            index += (address & 0xF) / 0x4;
            CHECK_MEM_BOUNDS(devMemInfoPtr->preegressMem.genReg,
                             devMemInfoPtr->preegressMem.genRegNum,
                             index, memSize);
            regValPtr = &devMemInfoPtr->preegressMem.genReg[index];
        }
    }

    return regValPtr;
}


/*******************************************************************************
* smemCht2PolicerTbl
*
*
* DESCRIPTION:
*   Visualizer specific table memory access functions
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
*******************************************************************************/
static GT_U32 *  smemCht2PolicerTbl
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SMEM_CHT2_DEV_MEM_INFO* devMemInfoPtr;
    GT_U32              * regValPtr;
    GT_U32                index;
    GT_U32                tbl;


    regValPtr = 0;
    devMemInfoPtr = (SMEM_CHT2_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    tbl = (address >> SMEM_CHT_SUB_UNIT_INDEX_FIRST_BIT_CNS) & 0x7;
    index = (address & 0xffff) / 0x4;

    /* Policers Table Entry<n> (0<=n<256) */
    if (tbl == SMEM_CHT_POLICER_TBL_SUB_UNIT_INDEX_CNS)
    {
        CHECK_MEM_BOUNDS(devMemInfoPtr->policerMem.policerTblReg,
                         devMemInfoPtr->policerMem.policerTblRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->policerMem.policerTblReg[index];
    }
    else
    /* Policers QoS Remarking and Initial DP Table Entry<n> (0<=n<72) */
    if (tbl == SMEM_CHT_POLICER_QOS_REM_UNIT_INDEX_CNS)
    {
        CHECK_MEM_BOUNDS(devMemInfoPtr->policerMem.policerQosRmTblReg,
                         devMemInfoPtr->policerMem.policerQosRmTblRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->policerMem.policerQosRmTblReg[index];
    }
    else
    /* Policers Counters Table Entry<n> (0<=n<16) */
    if (tbl == SMEM_CHT_POLICER_COUNTER_UNIT_INDEX_CNS)
    {
        /* Policers Table Entry */
        CHECK_MEM_BOUNDS(devMemInfoPtr->policerMem.policerCntTblReg,
                         devMemInfoPtr->policerMem.policerCntTblRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->policerMem.policerCntTblReg[index];
    }

    return regValPtr;
}


/*******************************************************************************
* smemCht2VlanTbl
*******************************************************************************/
static GT_U32 *  smemCht2VlanTbl
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SMEM_CHT2_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32              * regValPtr;
    GT_U32                index;
    GT_U32                tbl;

    regValPtr = 0;
    devMemInfoPtr = (SMEM_CHT2_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    tbl = (address >> SMEM_CHT_SUB_UNIT_INDEX_FIRST_BIT_CNS) & 0x7;
    index = (address & 0xffff) / 0x4;
    /* VLAN<n> Entry (0<=n<4096) */
    if (tbl == SMEM_CHT_VLAN_TBL_SUB_UNIT_INDEX_CNS)
    {
        CHECK_MEM_BOUNDS(devMemInfoPtr->vlanTblMem.vlanTblReg,
                         devMemInfoPtr->vlanTblMem.vlanTblRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->vlanTblMem.vlanTblReg[index];
    }
    else
    /* Multicast Group<n> Entry (0<=n<4096) */
    if (tbl == SMEM_CHT_MCST_TBL_SUB_UNIT_INDEX_CNS)
    {
        /* the entry paramInfo[0].step is 0x10,  so devide index to 4 */
        index /= 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->vlanTblMem.mcstTblReg,
                         devMemInfoPtr->vlanTblMem.mcstTblRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->vlanTblMem.mcstTblReg[index];
    }
    else
    /* Span State Group<n> Entry (0<=n<256) */
    if (tbl == SMEM_CHT_STP_TBL_SUB_UNIT_INDEX_CNS)
    {
        CHECK_MEM_BOUNDS(devMemInfoPtr->vlanTblMem.spanTblReg,
                         devMemInfoPtr->vlanTblMem.spanTblRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->vlanTblMem.spanTblReg[index];
    }

    return regValPtr;
}

/*******************************************************************************
/ ARP table and MAC SA table
*******************************************************************************/
static GT_U32 *  smemCht2ArpTbl
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SMEM_CHT2_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32              * regValPtr;
    GT_U32                index;
    GT_U32                tbl;

    regValPtr = 0;
    devMemInfoPtr = (SMEM_CHT2_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    tbl = (address >> SMEM_CHT_SUB_UNIT_INDEX_FIRST_BIT_CNS) & 0x1;

    if (tbl == SMEM_CHT_ARP_TBL_SUB_UNIT_INDEX_CNS)
    {
        /* ARP Entry (0<=n<1024) */
        index = (address & 0x1fff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->ipMem.arpTblReg,
                         devMemInfoPtr->ipMem.arpTblRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->ipMem.arpTblReg[index];
    }
    else
    if (tbl == SMEM_CHT_MAC_SA_TBL_SUB_UNIT_INDEX_CNS)
    {
        /* MAC SRC Entry (0<=n<4096) */
        index = (address & 0x1ffff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->ipMem.macSaTblReg,
                         devMemInfoPtr->ipMem.macSaTblRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->ipMem.macSaTblReg[index];
    }

    return regValPtr;
}



/*******************************************************************************
*  smemCht2ActiveWriteRouterAction
*
* DESCRIPTION:
*    The Tcam router engine table and TT write access.
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
static void smemCht2ActiveWriteRouterAction
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR param,
    IN         GT_U32 * inMemPtr
)
{
    GT_U32     rtLineNumber;    /* Router Tcam Line Number */
    GT_U32     rtWr;            /* Write operation */
    GT_U32     rtWrTargetArr;   /*  Write Target Array(Tcam) */
    GT_U32     rtDataType;      /* Router Tcam data type */
    GT_U32     rtValidBit;       /* TCAM valid entry bit */
    GT_U32     rtCompMode;    /* TCAM compare mode , IPv4 ... IPv6 */
    GT_U32     fldValue;

    /* Read router and TT tcam Access Control Register */
    rtLineNumber = (*inMemPtr >> 2) & 0x1FFF;
    rtDataType = (*inMemPtr >> 21) & 0x1;
    rtWrTargetArr = (*inMemPtr >> SMEM_CHT_GROUP_INDEX_FIRST_BIT_CNS) & 0x1f;
    rtWr = (*inMemPtr & 0x1);    /* write action */

    smemRegGet(devObjPtr, SMEM_CHT2_ROUTER_ACCESS_DATA_CTRL_REG(devObjPtr) , &fldValue);
    rtCompMode = (fldValue >> 16) & 0x3FF;
    rtValidBit = (fldValue & 0x1F);

    if (rtWr)
    {
        smemCht2WriteRouterTTTcam(devObjPtr, rtLineNumber, rtDataType,
                                    rtWrTargetArr,rtValidBit,rtCompMode);
    }
}

/*******************************************************************************
*  smemCht2ActiveWritePclAction
*
* DESCRIPTION:
*    The action table and policy TCAM write access.
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
static void smemCht2ActiveWritePclAction
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR param,
    IN         GT_U32 * inMemPtr
)
{
    GT_U32     lineNumber;      /* line number in TCAM and policy action table*/
    GT_U32     rdWr;            /* Read or write operation */
    GT_U32     rdWrTarget;      /* Read or Write Target Data Structure(Tcam/Action) */
    GT_U32     tcamWriteMode;   /* Mode for tne TCAM key short/long */
    GT_U32     tcamDataType;    /* TCAM data type */
    GT_U32     tcamValid;       /* TCAM valid entry bit */
    GT_U32     tcamCompMode;    /* TCAM compare mode , IPv4 ... IPv6 */

    /* Read Action Table and Policy TCAM Access Control Register */
    tcamWriteMode = (*inMemPtr >> 17) & 0x1;
    rdWrTarget = (*inMemPtr >> SMEM_CHT_GROUP_INDEX_FIRST_BIT_CNS) & 0x1;
    tcamDataType = (*inMemPtr >> 15) & 0x1;
    lineNumber = (*inMemPtr >> 2) & 0x1FFF;
    rdWr = (*inMemPtr >> 1) & 0x1;    /* 0 - read , 1 - write*/
    tcamValid = (*inMemPtr >> 18) & 0x1;
    tcamCompMode = (*inMemPtr >> 19) & 0x3;

    /* stored as is to be retrieved in called functions */
    *memPtr = *inMemPtr ;

    if (rdWrTarget == 0) /* Read or Write the TCAM policy table */
    {
        smemCht2ReadWritePolicyTcam(devObjPtr, lineNumber, tcamWriteMode,
                                    tcamDataType, rdWr,tcamValid,tcamCompMode);
    }
    else
    {   /* Read or Write the PCL action table */
        smemCht2ReadWritePclAction(devObjPtr, lineNumber, rdWr);
    }

    /* Cleared by the device when the read or write action is completed. */
    *memPtr = *inMemPtr & (~1);
}

/*******************************************************************************
*  smemCht2ActiveWriteVlanTbl
*
* DESCRIPTION:
*    VLT Tables Access Control Register.
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
void smemCht2ActiveWriteVlanTbl
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR param,
    IN         GT_U32 * inMemPtr
)
{
    GT_U32                  rdWr;            /* Read or write operation */
    GT_U32                  entry;           /* table entry number */
    GT_U32                  trgTbl;          /* tafget table */
    GT_U32                * regPtr;          /* register's entry pointer */
    GT_U32                  regAddr;         /* register's address */
    GT_U32                  hwWords[4];      /* swaped entry */

    rdWr = (*inMemPtr >> 12) & 0x1;
    entry = *inMemPtr & 0xFFF;
    trgTbl = (*inMemPtr >> 13)& 0x3;

    /* VLAN table */
    if (trgTbl == 0)
    {
        regAddr = SMEM_CHT_VLAN_TBL_MEM(devObjPtr, entry);
        if (rdWr)
        {
            /* VLT Tables Access Data Register0 (0x0a000000) */
            regPtr = smemMemGet(devObjPtr, SMEM_CHT_VLAN_TBL_WORD2_REG(devObjPtr));
            /* Swap VLAN entry 4 words */
            hwWords[0] = regPtr[3];
            hwWords[1] = regPtr[2];
            hwWords[2] = regPtr[1];
            hwWords[3] = regPtr[0];
            smemMemSet(devObjPtr, regAddr, hwWords, 4);
        }
        else
        {
            regPtr = smemMemGet(devObjPtr, regAddr);
            /* Swap VLAN entry 4 words. */
            hwWords[0] = regPtr[3];
            hwWords[1] = regPtr[2];
            hwWords[2] = regPtr[1];
            hwWords[3] = regPtr[0];
            /* VLT Tables Access Data Register0 (0x0a000000) */
            smemMemSet(devObjPtr, SMEM_CHT_VLAN_TBL_WORD2_REG(devObjPtr), hwWords, 4);
        }
    }
    else
    /* Multicast Groups table */
    if (trgTbl == 1)
    {
        regAddr = SMEM_CHT_MCST_TBL_MEM(devObjPtr, entry);
        if (rdWr)
        {
            /* VLT Tables Access Data Register0 for cht2 (0x0a00000C)*/
            regPtr = smemMemGet(devObjPtr, SMEM_CHT2_VLAN_TBL_WORD0_REG(devObjPtr));
            smemMemSet(devObjPtr, regAddr, regPtr, 1);
        }
        else
        {
            regPtr = smemMemGet(devObjPtr, regAddr);
            /* VLT Tables Access Data Register0 , (0x0a00000C)*/
            smemMemSet(devObjPtr, SMEM_CHT2_VLAN_TBL_WORD0_REG(devObjPtr), regPtr, 1);
        }
    }
    else
    /* Span Stage Groups table */
    if (trgTbl == 2)
    {
        regAddr = SMEM_CHT_STP_TBL_MEM(devObjPtr, entry);
        if (rdWr)
        {
            /* VLT Tables Access Data Register0 , , (0x0a000008) */
            regPtr = smemMemGet(devObjPtr, SMEM_CHT_VLAN_TBL_WORD0_REG(devObjPtr));
            /* Swap STP entry words */
            hwWords[0] = regPtr[1];
            hwWords[1] = regPtr[0];
            smemMemSet(devObjPtr, regAddr, hwWords, 2);
        }
        else
        {
            regPtr = smemMemGet(devObjPtr, regAddr);
            /* Swap STP entry words */
            hwWords[0] = regPtr[1];
            hwWords[1] = regPtr[0];
            /* VLT Tables Access Data Register0 , , (0x0a00000C) */
            smemMemSet(devObjPtr, SMEM_CHT_VLAN_TBL_WORD0_REG(devObjPtr), hwWords, 2);
        }
    }

    /* Cleared by the device when the read or write action is completed. */
    *memPtr = *inMemPtr & (~(1 << 15));
}


/*******************************************************************************
*  smemCht2WriteRouterTTTcam
*
* DESCRIPTION:
*      Handler for read/write  policy TCAM memory.
*
* INPUTS:
*       devObjPtr       -
*       rtLineNumber    -
*       rtDataType      -
*       rtWrTargetArr   -
*       rtValidBit      -
*       rtCompMode      -
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void smemCht2WriteRouterTTTcam
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32  rtLineNumber,
    IN         GT_U32  rtDataType,
    IN         GT_U32  rtWrTargetArr,
    IN         GT_U32  rtValidBit ,
    IN         GT_U32  rtCompMode
)
{
    GT_U32  *regPtr;         /* register's entry pointer */
    GT_U32  tcamRegAddr0;    /* register's address group 0-2 */
    GT_U32  ctrlRegAddr0;
    GT_U32  word2;
    GT_U32  word1 = 0;
    GT_U32  validBit = 0 ;
    GT_U32  compModeBit = 0 ;
    GT_U32  ii;

    if (rtDataType == 0)
    {
        /* Write is done to the data part of the Router TCAM and CTRL entries */
        tcamRegAddr0 = SMEM_CHT2_ROUTER_TCAM_ENTRY_DATA_WORD0_TBL_MEM(rtLineNumber);
        ctrlRegAddr0 = SMEM_CHT2_ROUTER_TCAM_ENTRY_CTRL_WORD0_TBL_MEM(rtLineNumber);
    }
    else
    {
        tcamRegAddr0 = SMEM_CHT2_ROUTER_TCAM_ENTRY_MASK_WORD0_TBL_MEM(rtLineNumber);
        ctrlRegAddr0 = SMEM_CHT2_ROUTER_TCAM_ENTRY_CTRL_MASK_WORD0_TBL_MEM(rtLineNumber);
    }

    /* Read TCAM data register word 0 - long key */
    regPtr = smemMemGet(devObjPtr, SMEM_CHT2_ROUTER_TCAM_ACCESS_DATA_TBL_MEM(0));

    for (ii = 0; (ii < 5) ; ii++)
    {

        word2 = 0;
        switch (ii)
        {
        case 0x1: /* Write tcam word1 from data 1 and word2 from data2 16 LSB*/
            word2 = regPtr[ii + 1] & 0xFFFF;
        case 0x0: /* Write tcam word0 from data 0 */
            word1 = regPtr[ii];
            break;

        case 0x2: /* Write tcam word1 from data 2 and 3 and word2 from data4 16 MSB*/
            word1 = ((regPtr[ii] >> 16) & 0xFFFF)
                    | ((regPtr[ii + 1] & 0xFFFF) << 16);
            word2 = (regPtr[ii + 1] >> 16) & 0xFFFF;
                    break;
        case 0x3: /* Write tcam word3 from data 4 */
        case 0x4: /* Write tcam word4 from data 5 */
            word1 = regPtr[ii + 1];
            break;
        }
        if ((1 << ii) & rtWrTargetArr)
        {
            smemRegSet(devObjPtr, (tcamRegAddr0 + (0x4000 * ii)) , word1);
            validBit = ((rtValidBit & (1 << ii)) ? 1 : 0) << 17;
            compModeBit =  (rtCompMode >> (2 * ii)) & 0x3;
            word2 |= validBit | (compModeBit << 18);
            smemRegSet(devObjPtr , (ctrlRegAddr0 + (0x4000 * ii)) , word2);
        }
    }

}

/*******************************************************************************
*  smemCht2ReadWritePolicyTcam
*
* DESCRIPTION:
*      Handler for read/write router TT TCAM memory.
*
* INPUTS:
*       devObjPtr       - device object PTR.
*       lineNumber      - table line number to be read or write
*       tcamWriteMode   - short/long key mode
*       tcamDataType    - Mask/Data part of entry.
*       rdWr            - read/write action. (1 = for write)
*       tcamValid       - valid entry bit
*       tcamCompMode    - compare mode
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void smemCht2ReadWritePolicyTcam
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32  lineNumber,
    IN GT_U32  tcamWriteMode,
    IN GT_U32  tcamDataType,
    IN GT_U32  rdWr,
    IN GT_U32  tcamValid ,
    IN GT_U32  tcamCompMode
)
{
    if (rdWr)
    {
        smemCht2WritePolicyTcamData(devObjPtr, lineNumber, tcamDataType,
                                    tcamWriteMode , tcamValid , tcamCompMode);
        smemCht2WritePolicyTcamCtrl(devObjPtr, lineNumber, tcamDataType,
                                    tcamWriteMode , tcamValid , tcamCompMode);
    }
    else
    {
        smemCht2ReadPolicyTcamData(devObjPtr, lineNumber, tcamDataType, tcamWriteMode);
        smemCht2ReadPolicyTcamCtrl(devObjPtr, lineNumber, tcamDataType, tcamWriteMode);
    }
}

/*******************************************************************************
*  smemCht2WritePolicyTcamData
*
* DESCRIPTION:
*      Handler for write  policy TCAM data memory.
*
* INPUTS:
*       devObjPtr           - device object PTR.
*       lineNumber          - table line number to be read or write.
*       tcamDataType        - TCAM data(=0) or mask(=1) part of entry
*       tcamWriteMode       - short/long key mode
*       tcamValid           - valid entry bit  ,
*       tcamCompMode        - compare mode
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void smemCht2WritePolicyTcamData
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32  lineNumber ,
    IN         GT_U32  tcamDataType ,
    IN         GT_U32  tcamWriteMode ,
    IN         GT_U32  tcamValid ,
    IN         GT_U32  tcamCompMode
)
{
    GT_U32  *regPtr6;         /* register's entry pointer */
    GT_U32  tcamRegAddr0;       /* register's address group 0-2 */
    GT_U32  word2 = 0;
    GT_U32  ii;

    /* TCAM data(=0) or mask(=1) */
    if (tcamDataType == 0)
    {   /* Write is done to the data part of the entry */
        tcamRegAddr0 = SMEM_CHT2_PCL_TCAM_ENTRY_WORD0_TBL_MEM(lineNumber);
    }
    else
    {   /* Write is done to the mask part of the entry */
        tcamRegAddr0 = SMEM_CHT2_PCL_TCAM_ENTRY_MASK0_TBL_MEM(lineNumber);
    }

    /* Read TCAM data register word 0 - long key, 6 - short */
    regPtr6 = smemMemGet(devObjPtr, SMEM_CHT2_PCL_ACTION_TCAM_TBL_MEM(
        ((tcamWriteMode == 0) ? 6 : 0)));
    /* write pcl standard key (data or mask) for tcam and control *
     * The function of smemMemSet is irrelevant because of        *
     * the structure of the PCL tcam table                        */
    for (ii = 0; (ii < 5) ; ii++)
    {
        switch (ii)
        {
            case 0x0: /* Write tcam word0 from data 6 */
            case 0x1: /* Write tcam word1 from data 7 */
                word2 = regPtr6[ii];
                break;
            case 0x2: /* Write tcam word1 from data 8 and 9 */
                word2 = ((regPtr6[ii] >> 16) & 0xFFFF)
                    | ((regPtr6[ii + 1] & 0xFFFF) << 16);
                break;
            case 0x3: /* Write tcam word1 from data 10 */
            case 0x4: /* Write tcam word1 from data 11 */
                word2 = regPtr6[ii + 1];
                break;
        }
        smemRegSet(devObjPtr, (tcamRegAddr0 + (0x4000 * ii)) , word2);
    }

    /* A long pcl key */
    if (tcamWriteMode)
    {
        if (tcamDataType == 0)
        {
            tcamRegAddr0 = SMEM_CHT2_PCL_TCAM_ENTRY_WORD0_TBL_MEM(lineNumber + 512);
        }
        else
        {
            tcamRegAddr0 = SMEM_CHT2_PCL_TCAM_ENTRY_MASK0_TBL_MEM(lineNumber + 512);
        }

        /* Read TCAM data register word 6*/
        regPtr6 = smemMemGet(devObjPtr, SMEM_CHT2_PCL_ACTION_TCAM_TBL_MEM(6));

        /* write pcl standard key (data or mask) for tcam and control */
        for (ii = 0; (ii < 5) ; ii++)
        {
            switch (ii)
            {
                case 0x0: /* Write tcam word0 from data 6 */
                case 0x1: /* Write tcam word1 from data 7 */
                    word2 = regPtr6[ii];
                    break;
                case 0x2: /* Write tcam word1 from data 8 and 9 */
                    word2 = ((regPtr6[ii] >> 16) & 0xFFFF)
                        | ((regPtr6[ii + 1] & 0xFFFF) << 16);
                    break;
                case 0x3: /* Write tcam word1 from data 10 */
                case 0x4: /* Write tcam word1 from data 11 */
                    word2 = regPtr6[ii + 1];
                    break;
            }
            smemRegSet(devObjPtr, (tcamRegAddr0 + (0x4000 * ii)) , word2);
        }
    }
}


/*******************************************************************************
*  smemCht2ReadPolicyTcamData
*
* DESCRIPTION:
*      Handler for read/write  policy TCAM memory .
*
* INPUTS:
*       devObjPtr   - device object PTR.
*       line        - table line number to be read or write
*       type        - TCAM data or mask
*       mode        - short/long key mode
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void smemCht2ReadPolicyTcamData
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32  line,
    IN         GT_U32  type,
    IN         GT_U32  mode
)
{
    GT_U32                * regPtr0;        /* register's entry pointer */
    GT_U32                * regPtr3;        /* register's entry pointer */
    GT_U32                  regAddr0;       /* register's address group 0-2 */
    GT_U32                  regAddr3;       /* register's address group 3-5 */

    /* TCAM data or mask */
    if (type == 1)
    {
        regAddr0 = SMEM_CHT_PCL_TCAM_ENTRY_WORD0_REG(line);
        regAddr3 = SMEM_CHT_PCL_TCAM_ENTRY_WORD3_REG(line);
    }
    else
    {
        regAddr0 = SMEM_CHT_PCL_TCAM_ENTRY_MASK0_REG(line);
        regAddr3 = SMEM_CHT_PCL_TCAM_ENTRY_MASK3_REG(line);
    }

    /* Read words 0-2 */
    regPtr0 = smemMemGet(devObjPtr, regAddr0);
    /* Read words 3-5 */
    regPtr3 = smemMemGet(devObjPtr, regAddr3);

    /* A long key */
    if (mode)
    {
        /* Write TCAM data register word 0 */
        smemMemSet(devObjPtr, SMEM_CHT_PCL_ACTION_TCAM_DATA0_REG(devObjPtr), regPtr0, 3);
        /* Write TCAM data register word 3 */
        smemMemSet(devObjPtr, SMEM_CHT_PCL_ACTION_TCAM_DATA3_REG(devObjPtr), regPtr3, 3);
    }
    else
    {
        /* Write TCAM data register word 6*/
        smemMemSet(devObjPtr, SMEM_CHT_PCL_ACTION_TCAM_DATA6_REG(devObjPtr), regPtr0, 3);
        /* Write TCAM data register word 9 */
        smemMemSet(devObjPtr, SMEM_CHT_PCL_ACTION_TCAM_DATA9_REG(devObjPtr), regPtr3, 3);
    }

    /* A long key */
    if (mode)
    {
        if (type == 1)
        {
            regAddr0 = SMEM_CHT_PCL_TCAM_ENTRY_WORD0_REG(line + 512);
            regAddr3 = SMEM_CHT_PCL_TCAM_ENTRY_WORD3_REG(line + 512);
        }
        else
        {
            regAddr0 = SMEM_CHT_PCL_TCAM_ENTRY_MASK0_REG(line + 512);
            regAddr3 = SMEM_CHT_PCL_TCAM_ENTRY_MASK3_REG(line + 512);
        }

        /* Read words 6-8 */
        regPtr0 = smemMemGet(devObjPtr, regAddr0);
        /* Read words 9-11 */
        regPtr3 = smemMemGet(devObjPtr, regAddr3);

        /* Write TCAM data register word 0 */
        smemMemSet(devObjPtr, SMEM_CHT_PCL_ACTION_TCAM_DATA6_REG(devObjPtr), regPtr0, 3);
        /* Write TCAM data register word 3 */
        smemMemSet(devObjPtr, SMEM_CHT_PCL_ACTION_TCAM_DATA9_REG(devObjPtr), regPtr3, 3);
    }
}

/*******************************************************************************
*  smemCht2WritePolicyTcamCtrl
*
* DESCRIPTION:
*      Handler for read/write policy TCAM control memory .
*
* INPUTS:
*       devObjPtr           - device object PTR.
*       lineNumber          - table line number to be read or write
*       tcamWriteMode       - short/long key mode
*       tcamDataType        - TCAM data(=0) or mask
*       tcamValid           - TCAM valid bit.
*       tcamCompMode        - Tcam compare mode (ipv4 , ipv6...)
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void smemCht2WritePolicyTcamCtrl
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32  lineNumber,
    IN         GT_U32  tcamDataType,
    IN         GT_U32  tcamWriteMode,
    IN         GT_U32  tcamValid       ,
    IN         GT_U32  tcamCompMode
)
{
    GT_U32      ii;
    GT_U32      ctrlData;
    GT_U32      validBit;
    GT_U32      *regPtr8;         /* register's entry pointer */
    GT_U32      tcamCtrlRegAddr1;

    /* TCAM data(=0) or mask(=1) */
    if (tcamDataType == 0)
    {
        tcamCtrlRegAddr1 = SMEM_CHT2_PCL_TCAM_WORD_CTRL0_TBL_MEM(lineNumber);
        validBit =  tcamValid;
    }
    else
    {
        tcamCtrlRegAddr1 = SMEM_CHT2_PCL_TCAM_WORD_CTRL_MASK0_TBL_MEM(lineNumber);
        validBit = 1;
    }

    /* Read TCAM data register word 0 - long key, 6 - short key */
    regPtr8 = smemMemGet(devObjPtr, SMEM_CHT2_PCL_ACTION_TCAM_TBL_MEM(
        ((tcamWriteMode == 0) ? 6 : 0)));
    /* write pcl standard key (data or mask) for control table */
  /* write pcl standard key (data or mask) for control table */
    for (ii = 0; (ii < 5); ii++, tcamCtrlRegAddr1 += 0x4000)
    {
        ctrlData = (validBit << 17) | ((tcamCompMode & 3) << 18);
        switch (ii)
        {
        case 0:
        case 3:
        case 4:
            break;
        case 1:
            ctrlData |= (regPtr8[2] & 0xFFFF);
            break;
        case 2:
            ctrlData |= ((regPtr8[3] >> 16) & 0xFFFF);
            break;
        }
        smemRegSet(devObjPtr, tcamCtrlRegAddr1 , ctrlData);
    }

    /* A long key */
    if (tcamWriteMode)
    {
        if (tcamDataType == 0)
        {
            tcamCtrlRegAddr1 = SMEM_CHT2_PCL_TCAM_WORD_CTRL0_TBL_MEM(lineNumber + 512);
        }
        else
        {
            tcamCtrlRegAddr1 = SMEM_CHT2_PCL_TCAM_WORD_CTRL_MASK0_TBL_MEM(lineNumber + 512);
        }

        /* Read TCAM data register word 6*/
        regPtr8 = smemMemGet(devObjPtr, SMEM_CHT2_PCL_ACTION_TCAM_TBL_MEM(6));

        /* write pcl standard key (data or mask) for control table */
        for (ii = 0; (ii < 5); ii++, tcamCtrlRegAddr1 += 0x4000)
        {
            ctrlData = (validBit << 17) | ((tcamCompMode & 3) << 18);
            switch (ii)
            {
            case 0:
            case 3:
            case 4:
                break;
            case 1:
                ctrlData |= (regPtr8[2] & 0xFFFF);
                break;
            case 2:
                ctrlData |= ((regPtr8[3] >> 16) & 0xFFFF);
                break;
            }
            smemRegSet(devObjPtr, tcamCtrlRegAddr1 , ctrlData);
        }
    }
}


/*******************************************************************************
*  smemCht2ReadPolicyTcamCtrl
*
* DESCRIPTION:
*      Handler for read/write  policy TCAM memory.
*
* INPUTS:
*       devObjPtr   - device object PTR.
*       line        - table line number to be read or write
*       mode        - short/long key mode
*       type        - TCAM data or mask
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void smemCht2ReadPolicyTcamCtrl
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32  line,
    IN         GT_U32  type,
    IN         GT_U32  mode
)
{
    GT_U32                * regReadPtr;     /* read register's entry pointer */
    GT_U32                * regWritePtr;    /* write register's entry pointer*/
    GT_U32                  regAddr;        /* register's address */

    /* Read Action Table and Policy TCAM Access Control Register */
    regWritePtr = smemMemGet(devObjPtr, SMEM_CHT_PCL_ACTION_TCAM_CTRL_REG(devObjPtr));

    /* TCAM data type */
    if (type == 0)
    {
        regAddr = SMEM_CHT_PCL_TCAM_MASK_CTRL0_REG(line);
        regReadPtr = smemMemGet(devObjPtr, regAddr);
    }
    else
    {
        regAddr = SMEM_CHT_PCL_TCAM_WORD_CTRL0_REG(line);
        regReadPtr = smemMemGet(devObjPtr, regAddr);
    }

    *regWritePtr = *regReadPtr;
    /* A long key */
    if (mode)
    {
        /* TCAM data type */
        if (type == 0)
        {
            regAddr = SMEM_CHT_PCL_TCAM_MASK_CTRL1_REG(line + 512);
            regReadPtr = smemMemGet(devObjPtr, regAddr);
        }
        else
        {
            regAddr = SMEM_CHT_PCL_TCAM_WORD_CTRL1_REG(line + 512);
            regReadPtr = smemMemGet(devObjPtr, regAddr);
        }

        *regWritePtr = *regReadPtr;
    }
}


/*******************************************************************************
*  smemCht2ReadWritePclAction
*
* DESCRIPTION:
*      Handler write for cheetah2 PCL indirect action table.
*
* INPUTS:
*       devObjPtr - device object PTR.
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
static void smemCht2ReadWritePclAction (
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32  line,
    IN         GT_U32  rdWr
)
{
    GT_U32                * regPtr;           /* register's entry pointer   */
    GT_U32                  regAddr;          /* register's address         */
    GT_U32                  dataRegAddr;      /* address of data9 register  */

    /* PCL action table address */
    regAddr = SMEM_CHT_PCL_ACTION_TBL_MEM(devObjPtr, line);
    dataRegAddr = SMEM_CHT2_PCL_ACTION_TCAM_TBL_MEM(9);

    if (rdWr)
    {
        /* Write policy table . Get the data from register word9 */
        regPtr = smemMemGet(devObjPtr, dataRegAddr);
        smemMemSet(devObjPtr, regAddr, regPtr, 3);
    }
    else
    {
        /* Read policy table . Set data in register word9 */
        regPtr = smemMemGet(devObjPtr, regAddr);
        smemMemSet(devObjPtr, dataRegAddr, regPtr, 3);
    }
}


/*******************************************************************************
*  smemCht2ActiveFuqBaseWrite
*
* DESCRIPTION:
*       FDB Upload Queue Base Address active write.
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
void smemCht2ActiveFuqBaseWrite
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR param,
    IN         GT_U32 * inMemPtr
)
{
    CHT2_FUQ_MEM  * fuqMemPtr;

    *memPtr = *inMemPtr;

    fuqMemPtr = SMEM_CHT_MAC_FUQ_MEM_GET(devObjPtr);

    /* First time */
    if(fuqMemPtr->baseInit ==  GT_FALSE)
    {
        fuqMemPtr->fuqBase = *inMemPtr;
        fuqMemPtr->baseInit = GT_TRUE;
        fuqMemPtr->fuqBaseValid = GT_TRUE;
    }
    else
    {
        if (fuqMemPtr->fuqBaseValid == GT_TRUE)
        {
            fuqMemPtr->fuqShadow = *inMemPtr;
            fuqMemPtr->fuqShadowValid = GT_TRUE;
        }
        else
        {
            fuqMemPtr->fuqBase = *inMemPtr;
            fuqMemPtr->fuqOffset = 0;
            fuqMemPtr->fuqNumMessages = 0;
            fuqMemPtr->fuqBaseValid = GT_TRUE;
        }
    }

    if (SKERNEL_IS_CHEETAH3_DEV(devObjPtr))
    {
        /* Set Queue Full bit (bit 30) to zero */
        smemRegFldSet(devObjPtr, SMEM_CHT_FU_QUE_BASE_ADDR_REG(devObjPtr), 30, 1, 0);
    }
}



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
)
{
    smemChtTableInfoSet(devObjPtr);

    /* override the ch1 values */

    devObjPtr->globalInterruptCauseRegister.port27     = 27;

    devObjPtr->tablesInfo.stp.commonInfo.baseAddress = 0x0A100000;
    devObjPtr->tablesInfo.stp.paramInfo[0].step = 0x10;

    devObjPtr->tablesInfo.mcast.commonInfo.baseAddress = 0x0A200000;
    devObjPtr->tablesInfo.mcast.paramInfo[0].step = 0x10;

    devObjPtr->tablesInfo.vlan.commonInfo.baseAddress = 0x0A400000;
    devObjPtr->tablesInfo.vlan.paramInfo[0].step = 0x10;

    devObjPtr->tablesInfo.ingrStc.commonInfo.baseAddress = 0x0B040000;
    devObjPtr->tablesInfo.ingrStc.paramInfo[0].step = 0x10;

    devObjPtr->tablesInfo.statisticalRateLimit.commonInfo.baseAddress = 0x0B100000;

    devObjPtr->tablesInfo.cpuCode.commonInfo.baseAddress = 0x0B200000;

    devObjPtr->tablesInfo.qosProfile.commonInfo.baseAddress = 0x0B300000;

    devObjPtr->tablesInfo.fdb.commonInfo.baseAddress = 0x06400000;

    devObjPtr->tablesInfo.portVlanQosConfig.commonInfo.baseAddress = 0x0B810000;
    devObjPtr->tablesInfo.portVlanQosConfig.paramInfo[0].step = 0x20;

/*  xxx  devObjPtr->tablesInfo.pclId.commonInfo.baseAddress = 0x0B800200;*/

    if(devObjPtr->policerSupport.tablesBaseAddrSetByOrigDev == 0)
    {
        devObjPtr->tablesInfo.policer.commonInfo.baseAddress = 0x0C100000;
        devObjPtr->tablesInfo.policerQos.commonInfo.baseAddress = 0x0C200000;

        devObjPtr->tablesInfo.policerCounters.commonInfo.baseAddress = 0x0C300000;
        devObjPtr->tablesInfo.policerCounters.paramInfo[0].step = 0x8;
        devObjPtr->tablesInfo.policerCounters.paramInfo[1].step = 0; /* not supported*/
    }

    devObjPtr->tablesInfo.arp.commonInfo.baseAddress = 0x07E80000;
    devObjPtr->tablesInfo.arp.paramInfo[0].step = 0x20;
    devObjPtr->tablesInfo.arp.paramInfo[0].divider = 4;/* 4 sub entries in each line */

    devObjPtr->tablesInfo.vlanPortMacSa.commonInfo.baseAddress = 0x07F80000;

    devObjPtr->tablesInfo.pclConfig.commonInfo.baseAddress = 0x0B840000;
    devObjPtr->tablesInfo.pclConfig.paramInfo[0].step = 0x4; /*entry*/


    devObjPtr->tablesInfo.pclAction.commonInfo.baseAddress = 0x0B8C0000;
    devObjPtr->tablesInfo.pclAction.paramInfo[0].step = 0x10;

    devObjPtr->tablesInfo.pclTcam.commonInfo.baseAddress = 0x0B880000;
    devObjPtr->tablesInfo.pclTcam.paramInfo[0].step = 0x4;  /*entry*/
    devObjPtr->tablesInfo.pclTcam.paramInfo[1].step = 0x10; /*word*/

    devObjPtr->tablesInfo.pclTcamMask.commonInfo.baseAddress = 0x0B880008;
    devObjPtr->tablesInfo.pclTcamMask.paramInfo[0].step = 0x4; /*entry*/
    devObjPtr->tablesInfo.pclTcamMask.paramInfo[1].step = 0x10;/*word*/

    devObjPtr->tablesInfo.tcpUdpDstPortRangeCpuCode.commonInfo.baseAddress = 0x0B007000;
    devObjPtr->tablesInfo.tcpUdpDstPortRangeCpuCode.paramInfo[0].step = 0x4;      /*entry*/
    devObjPtr->tablesInfo.tcpUdpDstPortRangeCpuCode.paramInfo[1].step = 0x40;     /*word*/


    devObjPtr->tablesInfo.ipProtCpuCode.commonInfo.baseAddress = 0x0B008000;
    devObjPtr->tablesInfo.ipProtCpuCode.paramInfo[0].step = 0x4;
    devObjPtr->tablesInfo.ipProtCpuCode.paramInfo[0].divider = 2;

    devObjPtr->tablesInfo.tunnelStart.commonInfo.baseAddress = devObjPtr->tablesInfo.arp.commonInfo.baseAddress;
    devObjPtr->tablesInfo.tunnelStart.paramInfo[0].step = devObjPtr->tablesInfo.arp.paramInfo[0].step;


    devObjPtr->tablesInfo.routerTcam.commonInfo.baseAddress = 0x02A00000;
    devObjPtr->tablesInfo.routerTcam.paramInfo[0].step = devObjPtr->routeTcamInfo.entryWidth; /*entry*/
    devObjPtr->tablesInfo.routerTcam.paramInfo[1].step = 0x4;                                 /*word*/

    devObjPtr->tablesInfo.ttiAction.commonInfo.baseAddress = 0x02900000;
    devObjPtr->tablesInfo.ttiAction.paramInfo[0].step = 0x10;   /*entry*/
    devObjPtr->tablesInfo.ttiAction.paramInfo[1].step = 0x4;    /*word*/

    devObjPtr->tablesInfo.mll.commonInfo.baseAddress = 0x0C880000;
    devObjPtr->tablesInfo.mll.paramInfo[0].step = 0x10;

    devObjPtr->tablesInfo.mllOutInterfaceConfig.commonInfo.baseAddress = 0x0C800980;
    devObjPtr->tablesInfo.mllOutInterfaceConfig.paramInfo[0].step = 0x100;

    devObjPtr->tablesInfo.mllOutInterfaceCounter.commonInfo.baseAddress = 0x0C800900;
    devObjPtr->tablesInfo.mllOutInterfaceCounter.paramInfo[0].step = 0x100;

    devObjPtr->memUnitBaseAddrInfo.mll = 0x0C800000;

    devObjPtr->tablesInfo.trunkNumOfMembers.commonInfo.baseAddress = 0x0b009000;
    devObjPtr->tablesInfo.trunkNumOfMembers.paramInfo[0].step = 0x4;
    devObjPtr->tablesInfo.trunkNumOfMembers.paramInfo[0].divider = 8;

    devObjPtr->tablesInfo.trunkMembers.commonInfo.baseAddress = 0x0b400000;
    devObjPtr->tablesInfo.trunkMembers.paramInfo[0].step       = 0x4 ;/*member*/
    devObjPtr->tablesInfo.trunkMembers.paramInfo[1].step       = 0x20;/*trunkId*/

    devObjPtr->tablesInfo.ieeeRsrvMcCpuIndex.commonInfo.baseAddress = 0x02000804;
    devObjPtr->tablesInfo.ieeeRsrvMcCpuIndex.paramInfo[0].step = 0x1000;
    devObjPtr->tablesInfo.ieeeRsrvMcCpuIndex.paramInfo[0].divider = 16;

    devObjPtr->tablesInfo.routeNextHopAgeBits.commonInfo.baseAddress = 0x0280100C; /*Router Next Hop Table Age Bits*/
    devObjPtr->tablesInfo.routeNextHopAgeBits.paramInfo[0].step = 0x10;

    devObjPtr->tablesInfo.portProtocolVidQoSConf.commonInfo.baseAddress = 0x0B810800;
    devObjPtr->tablesInfo.portProtocolVidQoSConf.paramInfo[0].step = 0x20;
    devObjPtr->tablesInfo.portProtocolVidQoSConf.paramInfo[1].step = 0x4;
}

