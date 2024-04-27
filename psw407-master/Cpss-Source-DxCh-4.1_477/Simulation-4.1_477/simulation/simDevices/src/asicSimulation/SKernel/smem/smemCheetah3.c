/******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* smemCheetah3.c
*
* DESCRIPTION:
*       Cheetah3 memory mapping implementation.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 128 $
*
*******************************************************************************/
#include <os/simTypes.h>
#include <asicSimulation/SKernel/smem/smemCheetah.h>
#include <asicSimulation/SKernel/smem/smemCheetah3.h>
#include <asicSimulation/SKernel/cheetahCommon/sregCheetah.h>
#include <asicSimulation/SKernel/sfdb/sfdbCheetah.h>
#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SKernel/smain/smain.h>
#include <asicSimulation/SKernel/suserframes/snetCheetahEgress.h>
#include <asicSimulation/SKernel/suserframes/snetCheetahL2.h>
#include <asicSimulation/SKernel/suserframes/snetCheetah3CentralizedCnt.h>
#include <asicSimulation/SKernel/suserframes/snetCheetahPclSrv.h>
#include <asicSimulation/SKernel/suserframes/snetCheetah3Pcl.h>
#include <asicSimulation/SKernel/suserframes/snetXCatPolicer.h>
#include <common/Utils/Math/sMath.h>
#include <asicSimulation/SKernel/suserframes/snet.h>
#include <asicSimulation/SLog/simLog.h>

#define XCAT_A1_VLAN_NUM_WORDS_CNS  6

#define SFDB_MAC_TBL_ACT_WORDS  8
#define SFDB_MAC_TBL_ACT_BYTES  (SFDB_MAC_TBL_ACT_WORDS * sizeof(GT_U32))

/* Take the port number from address */
#define STACK_PORT_NUM_FROM_ADDRESS_MAC(_address, _port) \
{ \
    _port = (((_address) / 0x400) & 0x1f); \
    _port = ((_port) % 24) + 24; \
}

/* Policer counting entry size in words for xCat and above devices */
#define XCAT_POLICER_CNTR_ENTRY_NUM_WORDS_CNS 8

/* 1 Billion nanoseconds in a second */
#define NANO_SEC_MAX_VALUE_CNS  1000000000

/* memory units */
static DECLARE_UNIT_MEM_FUNCTION_MAC(smemCht3IpclTccEngineReg);
static DECLARE_UNIT_MEM_FUNCTION_MAC(smemCht3IPvxTccEngineReg);
static DECLARE_UNIT_MEM_FUNCTION_MAC(smemCht3MllEngineReg    );
static DECLARE_UNIT_MEM_FUNCTION_MAC(smemCht3SataSerdesReg   );
static DECLARE_UNIT_MEM_FUNCTION_MAC(smemCht3VlanTableReg    );
static DECLARE_UNIT_MEM_FUNCTION_MAC(smemCht3TriSpeedReg     );
static DECLARE_UNIT_MEM_FUNCTION_MAC(smemCht3XgPortsReg      );
static DECLARE_UNIT_MEM_FUNCTION_MAC(smemCht3XgMibReg        );
static DECLARE_UNIT_MEM_FUNCTION_MAC(smemCht3PclReg          );
static DECLARE_UNIT_MEM_FUNCTION_MAC(smemCht3PolicerReg      );
static DECLARE_UNIT_MEM_FUNCTION_MAC(smemCht3PciReg          );
static DECLARE_UNIT_MEM_FUNCTION_MAC(smemCht3pExtraMem       );
static DECLARE_UNIT_MEM_FUNCTION_MAC(smemCht3PreEgressReg    );
static DECLARE_UNIT_MEM_FUNCTION_MAC(smemXcatVlanTableReg    );
static DECLARE_UNIT_MEM_FUNCTION_MAC(smemXcatEtherBrdgReg    );
static DECLARE_UNIT_MEM_FUNCTION_MAC(smemXcatTransQueReg     );
static DECLARE_UNIT_MEM_FUNCTION_MAC(smemXcatPolicerReg      );
static DECLARE_UNIT_MEM_FUNCTION_MAC(smemXcatMllEngineReg    );
static DECLARE_UNIT_MEM_FUNCTION_MAC(smemXcatPclReg          );
static DECLARE_UNIT_MEM_FUNCTION_MAC(smemXcatEgrPclReg       );
static DECLARE_UNIT_MEM_FUNCTION_MAC(smemXcatVlanTableReg    );
static DECLARE_UNIT_MEM_FUNCTION_MAC(smemXcatPreEgressUnitReg );
static DECLARE_UNIT_MEM_FUNCTION_MAC(smemXcatIPvxTccEngineUniReg );
static DECLARE_UNIT_MEM_FUNCTION_MAC(smemXcatHaAndEpclConfUnitReg );
static DECLARE_UNIT_MEM_FUNCTION_MAC(smemXcatBufMngUnitReg );
static DECLARE_UNIT_MEM_FUNCTION_MAC(smemXcatTtiReg );
static DECLARE_UNIT_MEM_FUNCTION_MAC(smemXcatPclTccReg       );
static DECLARE_UNIT_MEM_FUNCTION_MAC(smemXcatDfxMemReg       );

/*write active memory*/
static ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemCht3ActiveWriteSerdesSpeed          );
static ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemXCatActiveWriteIntrCauseFlexPortA0Reg);
static ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemXCatActiveWritePortInterruptsMaskFlexPortA0Reg);
static ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemXCatActiveWriteForceLinkDownFlexPortA0Reg);

/*read active memory*/
static ACTIVE_READ_FUNC_PROTOTYPE_MAC(smemCht3ReadRssiExceptionReg1         );
static ACTIVE_READ_FUNC_PROTOTYPE_MAC(smemXCatActiveReadIntrCauseFlexPortA0Reg );

/* calc sign of bit in table 'Metering Conformance Level Sign Memory' (without byte count considerations)*/
#define SIP_5_15_CONF_SIGN_CALC(bucket_size,mru)  \
        ((bucket_size > mru) ? 1 : 0)

static GT_U32 policerBaseAddr[3] = {0x0C800000,0x0D000000,0x03800000};

/* policer xcat 2 size */
#define POLICER_XCAT2_SIZE_CNS         0x237F

void smemXcatA1TableInfoSetPart2
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr
);


static void smemCht3WritePolicyTcamData
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32  lineNumber ,
    IN         GT_U32  tcamDataType ,
    IN         GT_U32  tcamCompMode
);

static void smemCht3ReadWritePolicyTcam
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32  lineNumber,
    IN         GT_U32  tcamDataType,
    IN         GT_U32  rdWr,
    IN         GT_U32  tcamCompMode
);

static void smemCht3ReadWriteRouterTTTcam
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_BOOL  writeAction,
    IN         GT_U32  rtLineNumber,
    IN         GT_U32  rtDataType,
    IN         GT_U32  rtBankEnBmp,
    IN         GT_U32  rtValidBit ,
    IN         GT_U32  rtCompMode,
    IN         GT_U32  rtSpare
);

static void smemCht3InitFuncArray
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_CHT3_DEV_MEM_INFO  * devMemInfoPtr
);

static void smemCht3AllocSpecMemory
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_CHT3_DEV_MEM_INFO  * devMemInfoPtr
);

static void smemCht3ReadPolicyTcamData
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32  line,
    IN         GT_U32  type
);

static void smemCht3ReadWritePclAction
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32  line,
    IN         GT_U32  rdWr
);

/* non valid address to cause no match when looking for valid address */
#define NON_VALID_ADDRESS_CNS           0x00000001

/* Private definition */
#define     GLB_REGS_NUM                        (0xfff / 4 + 1)
#define     GLB_SDMA_REGS_NUM                   (1023)
#define     EGR_GLB_REGS_NUM                    (0xffff / 4)
#define     TXQ_INTERNAL_REGS_NUM               (512)
#define     EGR_TRUNK_FILTER_REGS_NUM           (2032)
#define     EGR_PORT_WD_REGS_NUM                (64)
#define     EGR_SECONDARY_TARGET_REGS_NUM       (32)
#define     EGR_STACK_TC_REMAP_REGS_NUM         (8)
#define     EGR_MIB_CNT_REGS_NUM                (120)
#define     EGR_HYP_MIB_REGS_NUM                (60 + 28)
#define     EGR_TALE_DROP_REGS_NUM              (0xffff / 4 + 1)
#define     EGR_RATE_SHAPES_REGS_NUM            (512)
#define     BRDG_ETH_BRDG_REGS_NUM              (262655)
#define     BUF_MNG_REGS_NUM                    (196)
#define     EGR_STC_REGS_NUM                    (3 * 28) /* 3 registers per port */
#define     BUF_MNG_LL_REGS_NUM                 (0xffff / 4 + 1)
#define     GOP_PORT_GROUP_REGS_NUM             (0x200  / 4 + 1)
#define     GOP_XG_PORT_GROUP_REGS_NUM          (0x14   / 4 + 1)
#define     MAC_PORT_GROUP_CNT_REGS_NUM         (0x37c  / 4 + 1)
#define     MAC_XG_PORT_GROUP_CNT_REGS_NUM      (64)
#define     MAC_FDB_REGS_NUM                    (124)
#define     MAC_TBL_REGS_NUM(dev)               (SKERNEL_DEVICE_FAMILY_CHEETAH_3_ONLY(dev)? (32 * 1024 * 4) :\
                                                 SKERNEL_DEVICE_FAMILY_XCAT_ONLY(dev) ?     (16 * 1024 * 4)   :\
                                                 0)
#define     BANK_MEM_REGS_NUM                   (393216) /*  98304 * 4 */
#define     VTL_VLAN_CONF_REGS_NUM              (5)
#define     VTL_VLAN_TBL_REGS_NUM               (4096 * 4)
#define     VTL_MCST_TBL_REGS_NUM               (4096)
#define     VTL_VRF_TBL_REGS_NUM                (4096)
#define     VTL_STP_TBL_REGS_NUM                (256 * 2)
#define     TRI_SPEED_REGS_NUM                  (0xffff / 4 + 1)
#define     XG_PORT_REGS_NUM                    (0xFFFF / 4 + 1)
#define     XG_MIB_CNTRS_REGS_PER_PORT_NUM      (0xFF / 4 + 1)
#define     XG_MIB_CNTRS_REGS_NUM               ((XG_MIB_CNTRS_REGS_PER_PORT_NUM) * 10)

#define     PCL_COMMON_REGS_NUM                 (0x1fff / 4 + 1)
#define     PCL_TCAM_INTERNAL_CONFIG_REGS_NUM   ((0xfff + 3) / 4)
#define     PCL_TCAM_TEST_CONFIG_REGS_NUM       ((0x58 / 4) + 1)
#define     PCL_CONFIG_REGS_NUM                 (4224 * 2)
#define     PCL_ACTION_TBL_REGS_NUM(dev)        (4 * (dev->pclTcamInfoPtr->bankEntriesNum *  dev->pclTcamInfoPtr->bankNumbers))
#define     CH3_PCL_ECC_REGS_NUM                ((7*1024/2)* 16) /*3.5K *16*/
#define     CH3_PCL_TCAM_REGS_NUM               (14*1024* 16)
#define     PCL_VLAN_TRANS_REGS_NUM             (4096)
#define     PCL_PORT_VLAN_QOS_REGS_NUM          (64 * 8)
#define     PCL_PROT_BASED_VLAN_QOS_REGS_NUM    (64 * 8)
#define     EPCL_CONF_REGS_NUM                  (0x160 / 4 + 1)

#define     POLICER_REGS_NUM                    (0x1ff/4+1)
#define     POLICER_DESCR_SAMP_REGS_NUM         (0x3f/4+1)
#define     POLICER_TBL_REGS_NUM                (1024 * 8)
#define     POLICER_QOS_REM_TBL_REGS_NUM        (128)
#define     POLICER_COUNT_TBL_REGS_NUM          (1024 * 8)
#define     POLICER_MNG_COUNT_TBL_REGS_NUM      (12 * 2)
#define     PCI_REGS_NUM                        (280)
#define     PHY_XAUI_DEV_NUM                    (3)
#define     PHY_IEEE_XAUI_REGS_NUM              (0xffff)
#define     PHY_EXT_XAUI_REGS_NUM               (0xffff)

#define     MAC_SA_TABLE_REGS_NUM               (0x7fff)
#define     TWSI_INT_REGS_NUM                   (0xff/4 + 1)
#define     PEX_REGS_NUM                        (0xfff / 4 + 1)
#define     ROUTER_CONF_REGS_NUM                (0x1200/3)

#define     ROUTER_AGE_REGS_NUM(dev)            (SKERNEL_DEVICE_FAMILY_CHEETAH_3_ONLY(dev) ? (2*1024 / 4) :\
                                                 SKERNEL_DEVICE_FAMILY_XCAT_ONLY(dev) ? ((!dev->isXcatA0Features) ? (512/4) : (1024 / 4)) : 0)

#define     CH3_ROUTER_TCAM_TEST_REGS_NUM       ((0x58 / 4) + 1)
#define     CH3_ROUTER_TCAM_REGS_NUM            ((20*1024) * 4)
#define     CH3_LTT_ACTION_TBL_REGS_NUM         ((20*1024))
#define     NEXT_HOP_TBL_REGS_NUM(dev)          (SKERNEL_DEVICE_FAMILY_CHEETAH_3_ONLY(dev)? (16 * 1024 * 4):\
                                                 SKERNEL_DEVICE_FAMILY_XCAT_ONLY(dev) ?     ( 4 * 1024 * 4):\
                                                 0)
#define     VRF_TBL_REGS_NUM                    (1024)
#define     MLL_GLOB_REGS_NUM                   (0xFFF / 4 + 1)
#define     MLL_TBL_REGS_NUM(dev)               (SKERNEL_DEVICE_FAMILY_CHEETAH_3_ONLY(dev)? (4 * 1024 * 4):\
                                                 SKERNEL_DEVICE_FAMILY_XCAT_ONLY(dev) ?     (2 * 1024 * 4):\
                                                 0)
#define     EGR_XSMI_REGS_NUM                   (2)
#define     IP_PROT_REGS_NUM                    (0xff)
#define     CENTRAL_CNT_GLB_REGS_NUM            (0xfff/4 + 1)
#define     CENTRAL_CNT_TBL_SIZE                (0x800 * 2)

#define     CH3P_EXTRA_MEM_REGS_NUM             (SMEM_CHT3_PLUS_EXTRA_MEM_SIZE / 4)

/* invalid MAC message  */
#define     SMEM_CHT3_INVALID_MAC_MSG_CNS       0xffffffff

/* maximal number of ports */
#define     SMEM_CHt3_MAX_PORT_NUM_CNS           28
/* XG port number to port index conversion
  0 for port 0,
  1 for port 4,
  2 for port 10,
  3 for port 12
  4 for port 16,
  5 for port 22
  6..9 for port 24..27*/

static GT_U32  smemCht3XgPortToIndexArray[SMEM_CHt3_MAX_PORT_NUM_CNS] =
     /* 0    1    2     3     4     5     6     7     8     9  */
    {   0, 0xff, 0xff, 0xff,    1, 0xff, 0xff, 0xff, 0xff, 0xff,
        2, 0xff,    3, 0xff, 0xff, 0xff,    4, 0xff, 0xff, 0xff,
     0xff, 0xff,    5, 0xff,    6,    7,    8,    9};

static GT_U32  smemXcat24_4XgPortToIndexArray[SMEM_CHt3_MAX_PORT_NUM_CNS] =
     /* 0    1    2     3     4     5     6     7     8     9  */
    {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
     0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
     0xff, 0xff, 0xff, 0xff,    0,    1,    2,    3};



/* number ipv4 lines in router tcam  */
#define CH3_IP_ROUT_TCAM_CNS                      (20*1024)
/* the range of address between one word to another */
#define IP_ROUT_TCAM_ENTRY_WIDTH_CNS              (0x10 /4)

static SKERNEL_DEVICE_ROUTE_TCAM_INFO_STC  ch3RoutTcamInfo =
{
    CH3_IP_ROUT_TCAM_CNS,
    CH3_IP_ROUT_TCAM_CNS / 4,
    CH3_IP_ROUT_TCAM_CNS / 4,
    IP_ROUT_TCAM_ENTRY_WIDTH_CNS,
    CH3_IP_ROUT_TCAM_CNS * 4
};

#define XCAT_IP_ROUT_TCAM_CNS               (13*1024)

SKERNEL_DEVICE_ROUTE_TCAM_INFO_STC  xcatRoutTcamInfo =
{
    XCAT_IP_ROUT_TCAM_CNS,
    XCAT_IP_ROUT_TCAM_CNS / 4,
    1,
    4,
    16
};

/* sampled at reset register */
#define SAMPLED_AT_RESET_ADDR  0x00000028

/* MAC Counters address mask */
#define     XG_MIB_COUNT_MSK_CNS              0xff81ff00

/* Active memory table */
static SMEM_ACTIVE_MEM_ENTRY_STC smemCht3ActiveTable[] =
{
    /* Global interrupt cause register */
    {0x00000030, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteIntrCauseReg, 0},

    /* Port<n> Auto-Negotiation Configuration Register */
    {0x0A80000C, 0xFFFF03FF, NULL, 0 , smemChtActiveWriteForceLinkDown, 0},
    /* Port<n> Interrupt Cause Register  */
    {0x0A800020, SMEM_CHT_GOPINT_MSK_CNS,
        smemChtActiveReadIntrCauseReg, 18, smemChtActiveWriteIntrCauseReg, 0},
    /* Tri-Speed Port<n> Interrupt Mask Register */
    {0x0A800024, SMEM_CHT_GOPINT_MSK_CNS, NULL, 0, smemChtActiveWritePortInterruptsMaskReg, 0},

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

    /* Port MIB Counters Capture */
    {0x04004020, SMEM_FULL_MASK_CNS, NULL, 0, smemChtActiveWriteMibCntCtrl, 0},
    {0x04804020, SMEM_FULL_MASK_CNS, NULL, 0, smemChtActiveWriteMibCntCtrl, 1},
    {0x05004020, SMEM_FULL_MASK_CNS, NULL, 0, smemChtActiveWriteMibCntCtrl, 2},
    {0x05804020, SMEM_FULL_MASK_CNS, NULL, 0, smemChtActiveWriteMibCntCtrl, 3},

    /* SMI0 Management Register : 0x04004054 , 0x05004054 */
    {0x04004054, 0xFEFFFFFF, NULL, 0 , smemChtActiveWriteSmi,0},

    /* Set Incoming Packet Count */
    {0x020400E0, 0xFFFFFFF0, smemChtActiveReadCntrs, 0, NULL,0}, /* Set #0 counters */
    {0x020400F4, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0}, /* Set #1 Incoming counters */
    {0x020400F8, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0}, /* Set #1 VLAN ingress filtered counters */
    {0x020400FC, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0}, /* Set #1 Security filtered counters */
    {0x02040100, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0}, /* Set #1 Bridge filtered counters */

    /* The MAC MIB Counters */
    {0x04010000, SMEM_CHT3_COUNT_MSK_CNS, smemChtActiveReadCntrs, 0, NULL,0},
    {0x04810000, SMEM_CHT3_COUNT_MSK_CNS, smemChtActiveReadCntrs, 1, NULL,0},
    {0x05010000, SMEM_CHT3_COUNT_MSK_CNS, smemChtActiveReadCntrs, 2, NULL,0},
    {0x05810000, SMEM_CHT3_COUNT_MSK_CNS, smemChtActiveReadCntrs, 3, NULL,0},
    /* XG port MAC MIB Counters (each port start at jumps of 0x20000)*/
    {0x09000000, XG_MIB_COUNT_MSK_CNS   , smemChtActiveReadCntrs, 0, NULL,0},

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
    {0x00002860, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},
    {0x00002864, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},

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
    {0x0B800138, SMEM_FULL_MASK_CNS, NULL, 0 , smemCht3ActiveWritePclAction, 0},

    /* Router and TT TCAM Access Control 1 Register  */
    {0x0280041C, SMEM_FULL_MASK_CNS, NULL, 0 , smemCht3ActiveWriteRouterAction, 0},

    /* PCL-ID Table Access Control Register */
    {0x0B800208, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWritePclId, 0},

    /* Policers Table Access Control Register */
    {0x0C000028, SMEM_FULL_MASK_CNS, NULL, 0 , smemCht3ActiveWritePolicerTbl, 0},

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
    {0x0B000020, SMEM_FULL_MASK_CNS, smemChtActiveReadIntrCauseReg, 24 , smemChtActiveWriteIntrCauseReg,0},

    /* VLT Tables Access Control Register */
    {0x0A000010, SMEM_FULL_MASK_CNS, NULL, 0 , smemCht2ActiveWriteVlanTbl, 0},

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
    {0x08800000, 0xFFFF03FF, NULL, 0 , smemChtActiveWriteForceLinkDownXg, 0},
    /* HyperG.Stack Ports MIB Counters and XSMII Configuration Register */
    {0x08800030, 0xFFFF03FF, NULL, 0, smemChtActiveWriteHgsMibCntCtrl, 0},
    /* XG Port<n> Interrupt Cause Register  */
/*    {0x08800014, 0xFFFF03FF,  smemCht3ActiveReadXgPortIntReg, 0, NULL, 0},*/
    {0x08800014, 0xFFFF03FF,  smemChtActiveReadIntrCauseReg, 20, smemChtActiveWriteIntrCauseReg, 0},
    /* XG Port<n> Interrupt Mask Register */
    {0x08800018, 0xFFFF03FF, NULL, 0, smemChtActiveWritePortInterruptsMaskReg, 0},

    /* ARP MAC DA table ??? */
    {0x07800208, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteArpTable, 0},

    /* MAC Table Interrupt Mask Register ??? */
    {0x0600001c, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteMacInterruptsMaskReg, 0},

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

    /* CAPWAP Rssi Exception register 1 */
    {SMEM_CHT3_CAPWAP_RSSI_EXCEPTION_REG + 4, SMEM_FULL_MASK_CNS, smemCht3ReadRssiExceptionReg1, 0, NULL, 0},

    /* Matrix Bridge Register */
    {0x020400d4, 0xFFFFFFFF, smemChtActiveReadCntrs, 0, NULL, 0},

    /* SDMA configuration register */
    {0x00002800, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteSdmaConfigReg, 0},

    /* FDB Global Configuration register */
    {0x06000000, SMEM_FULL_MASK_CNS, NULL, 0 , smemCht3ActiveWriteFDBGlobalCfgReg, 0},

    /* Transmit Queue Control Register */
    {0x01800000, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteTxQuCtrlReg, 0},

    /* Transmit Queue Config Register */
    {0x01800080, 0XFFFF01FF, NULL, 0 , smemChtActiveWriteTxQConfigReg, 0},

    /*Egress STC interrupt register*/
    {0x01800130, SMEM_FULL_MASK_CNS, smemChtActiveReadIntrCauseReg, 11 , smemChtActiveWriteIntrCauseReg,0},

    /* read interrupts cause registers CNC -- ROC register */
    {0x08000100, SMEM_FULL_MASK_CNS,
        smemChtActiveReadIntrCauseReg, 3, smemChtActiveWriteIntrCauseReg, 0},

    /* Write Interrupt Mask CNC Register */
    {0x08000104, SMEM_FULL_MASK_CNS, NULL, 0 , smemCht3ActiveWriteCncInterruptsMaskReg, 0},

    /* Policer : when Read a Data Unit counter part(Entry LSB) of Management Counters Entry.
                 then value of LSB and MSB copied to Shadow registers */
    {0x0C0C0000, CH3_POLICER_MANAGEMENT_COUNTER_MASK_CNS, smemXCatActiveReadPolicerManagementCounters, 0 , NULL, 0},

    /* Port<%n> Rate Limit Counter */
    {0x02000400, 0xFFFE0FFF,
        smemChtActiveReadRateLimitCntReg, 0 , smemChtActiveWriteToReadOnlyReg, 0},

    /* CPU Port MIB counters registers */
    {0x00000060, 0xFFFFFFE0, smemChtActiveReadCntrs, 0, NULL,0},

    /*** BIST done simulation ***/

    /* Policy TCAM BIST Config Register */
    {0x0B830050, SMEM_FULL_MASK_CNS, smemCht3ActiveReadTcamBistConfigAction, 0 , NULL, 0},
    /* Router TCAM BIST Config Register */
    {0x02802050, SMEM_FULL_MASK_CNS, smemCht3ActiveReadTcamBistConfigAction, 0 , NULL, 0},

    /*** BIST done simulation ends ***/

    /* must be last anyway */
    {END_OF_TABLE, SMEM_FULL_MASK_CNS, NULL,0,NULL,0}
};

#define CH3_ACTIVE_MEM_TABLE_SIZE \
    (sizeof(smemCht3ActiveTable)/sizeof(smemCht3ActiveTable[0]))

/* info for the xcat to override in the ch3 info */
static SMEM_ACTIVE_MEM_ENTRY_REPLACEMENT_STC  smemXcatActiveTableOverrideCh3[]=
{
    {END_OF_TABLE, {END_OF_TABLE, SMEM_FULL_MASK_CNS, NULL,0,NULL,0}}
};

/* info for the xcat A1 to override in the ch3 info */
static SMEM_ACTIVE_MEM_ENTRY_REPLACEMENT_STC  smemXcatA1ActiveTableOverrideCh3[]=
{
    /* VLT Tables Access Control Register */
    { 0x0A000010,
        {0x0A000118, SMEM_FULL_MASK_CNS, NULL, 0 , smemXcatA1ActiveWriteVlanTbl, 0}},

    /* XG Port<n> Interrupt Cause Register  */
    { 0x08800014,                                           /* was 20 in ch3 */
        {0x08800014, 0xFFFF03FF,  smemChtActiveReadIntrCauseReg, 18, smemChtActiveWriteIntrCauseReg, 0}},

    /* Port<n> Interrupt Cause Register  */
    { 0x0A800020,                                           /* was 18 in ch3 */
        {0x0A800020, SMEM_CHT_GOPINT_MSK_CNS, smemChtActiveReadIntrCauseReg, 16, smemChtActiveWriteIntrCauseReg, 0}},

    /* Receive SDMA Interrupt Cause Register (RxSDMAInt) */
    { 0x0000280C,                                           /* was 22 in ch3 */
        {0x0000280C, SMEM_FULL_MASK_CNS,smemChtActiveReadIntrCauseReg, 20, smemChtActiveWriteIntrCauseReg, 0}},

    /* Bridge Interrupt Cause Register */
    { 0x02040130,                                           /* was 14 in ch3 */
        {0x02040130, SMEM_FULL_MASK_CNS, smemChtActiveReadIntrCauseReg, 12, smemChtActiveWriteIntrCauseReg, 0}},

    /* Transmit SDMA Interrupt Cause Register */
    { 0x00002810,                                           /* was 21 in ch3 */
        {0x00002810, SMEM_FULL_MASK_CNS, smemChtActiveReadIntrCauseReg, 19 , smemChtActiveWriteIntrCauseReg,0}},

    /* bridge Interrupt Cause Register */
    { 0x06000018,                                           /* was 17 in ch3 */
        {0x06000018, SMEM_FULL_MASK_CNS, smemChtActiveReadIntrCauseReg, 15, smemChtActiveWriteIntrCauseReg, 0}},

    /* read interrupts cause registers Misc -- ROC register */
    { 0x00000038,                                           /* was 11 in ch3 */
        {0x00000038, SMEM_FULL_MASK_CNS, smemChtActiveReadIntrCauseReg, 9, smemChtActiveWriteIntrCauseReg, 0}},


    /* CH3 address of Policers Table Access Control Register -->
       the xcat uses other function in IPLR0,1 for other addresses */
    /* the address 0x0C000028 is not active memory in xcat !!! */
    {0x0C000028,
        {NON_VALID_ADDRESS_CNS, SMEM_FULL_MASK_CNS, NULL, 0 , smemCht3ActiveWritePolicerTbl, 0}},

    /* ch3 address for policer --> xcat has 3 policers --> will be managed in other way */
    {0x0C0C0000,
        {NON_VALID_ADDRESS_CNS, CH3_POLICER_MANAGEMENT_COUNTER_MASK_CNS, smemXCatActiveReadPolicerManagementCounters, 0 , NULL, 0}},

    {END_OF_TABLE, {END_OF_TABLE, SMEM_FULL_MASK_CNS, NULL,0,NULL,0}}
};


#define XCAT_ACTIVE_MEM_OVERRIDE_TABLE_SIZE \
    (sizeof(smemXcatActiveTableOverrideCh3)/sizeof(smemXcatActiveTableOverrideCh3[0]))


#define XCAT_A1_ACTIVE_MEM_OVERRIDE_TABLE_SIZE \
    (sizeof(smemXcatA1ActiveTableOverrideCh3)/sizeof(smemXcatA1ActiveTableOverrideCh3[0]))

/* currently size for xcat like size for ch3 */
#define  XCAT_ACTIVE_TABLE_SIZE  (CH3_ACTIVE_MEM_TABLE_SIZE)

/* entries for xcat that the ch3 not have */
static SMEM_ACTIVE_MEM_ENTRY_STC smemXcatActiveTableExtra[] =
{
    /* stack gig ports - Port<n> Interrupt Cause Register  */
    {0x08800020, SMEM_CHT_GOPINT_MSK_CNS,
        smemXCatActiveReadIntrCauseFlexPortA0Reg, 18, smemXCatActiveWriteIntrCauseFlexPortA0Reg, 0},
    /* stack gig ports - Tri-Speed Port<n> Interrupt Mask Register */
    {0x08800024, SMEM_CHT_GOPINT_MSK_CNS,
        NULL, 0, smemXCatActiveWritePortInterruptsMaskFlexPortA0Reg, 0},
    /* stack gig ports - Port<n> Auto-Negotiation Configuration Register */
    {0x0880000C, 0xFFFF03FF, NULL, 0 , smemXCatActiveWriteForceLinkDownFlexPortA0Reg, 0},
    /*Port MAC Control Register3*/
    {0x0880001C, 0xFFFF03FF, NULL, 0 , smemXcatActiveWriteMacModeSelect, 0},

    /* SERDES Speed Register1  mask allow only SERDESs >= 32 that relate to ports 24..27  support FLEX ports of xcat A0 */
    /* SERDESs of other ports 0.. 23 not need active memory in simulation */
    {0x09808014, 0xFFFF83FF, NULL, 0 , smemCht3ActiveWriteSerdesSpeed, 0},

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

    /* Policer: when write iplr0 control0 register, "Policer memory control" configuration is
                changed and ingress policer tables are redistributed between plr0,1 accordingly */
    {0x0C800000, SMEM_FULL_MASK_CNS, NULL, 0 , smemXCatActiveWritePolicerMemoryControl,  0},

    /* iplr0 policer table 0x0c840000 -        0x0c84FFFF  xCat A1,A2 */
    {0x0c840000, 0xFFFF0000, smemXCatActiveReadIplr0Tables, 0 , smemXCatActiveWriteIplr0Tables,  0},
    /* iplr0 policerCounters table 0x0c860000 -        0x0c86FFFF  xCat A1,A2 */
    {0x0c860000, 0xFFFF0000, smemXCatActiveReadIplr0Tables, 0 , smemXCatActiveWriteIplr0Tables,  0},

    /* iplr1 policer table 0x0d040000 -        0x0d04FFFF  xCat A1,A2 (access redirected to iplr0 table)*/
    {0x0d040000, 0xFFFF0000, smemXCatActiveReadIplr1Tables, 0 , smemXCatActiveWriteIplr1Tables,  0},
    /* iplr1 policerCounters table 0x0d060000 -        0x0d06FFFF  xCat A1,A2 (redirect to iplr0) */
    {0x0d060000, 0xFFFF0000, smemXCatActiveReadIplr1Tables, 0 , smemXCatActiveWriteIplr1Tables,  0},

    /* IEEE Reserved Multicast Configuration register reading */
    {0x02000810, 0xFFFF0FFF, smemXCatActiveReadIeeeMcConfReg, 0 , NULL,  0},
    {0x02000818, 0xFFFF0FFF, smemXCatActiveReadIeeeMcConfReg, 0 , NULL,  0},
    {0x02000820, 0xFFFF0FFF, smemXCatActiveReadIeeeMcConfReg, 0 , NULL,  0},
    {0x02000828, 0xFFFF0FFF, smemXCatActiveReadIeeeMcConfReg, 0 , NULL,  0},


    {0x0B0000A0, SMEM_FULL_MASK_CNS, NULL, 0 , smemXCatActiveWriteLogTargetMap,  0},
    {0x0B0000A4, SMEM_FULL_MASK_CNS, NULL, 0 , smemXCatActiveWriteEgressFilterVlanMap,  0},
    {0x0B0000AC, SMEM_FULL_MASK_CNS, NULL, 0 , smemXCatActiveWriteEgressFilterVlanMember,  0},

    {0x0E000208, SMEM_FULL_MASK_CNS, NULL, 0 , smemXCatActiveWritePolicyTcamConfig_0,  0},
    /* TTI Internal, Metal Fix */
    {0x0C0002B0, SMEM_FULL_MASK_CNS, NULL, 0 , smemXCatActiveWriteTtiInternalMetalFix,  0},
    /* Global control register */
    {0x00000058, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteGlobalReg, 0},

    /*Bridge Global Configuration2*/
    {0x0204000C, SMEM_FULL_MASK_CNS, NULL, 0 , smemXCatActiveWriteBridgeGlobalConfig2Reg, 0},
    /*Pre-Egress Engine Global Configuration*/
    {0x0B050000, SMEM_FULL_MASK_CNS, NULL, 0 , smemXCatActiveWriteEqGlobalConfigReg, 0},
    /*Ingress Policy Global Configuration*/
    {0x0B800000, SMEM_FULL_MASK_CNS, NULL, 0 , smemXCatActiveWriteIpclGlobalConfigReg, 0},
    /*TTI Global Configuration*/
    {0x0c000000, SMEM_FULL_MASK_CNS, NULL, 0 , smemXCatActiveWriteTtiGlobalConfigReg, 0},
    /*HA Global Configuration*/
    {0x0f000100, SMEM_FULL_MASK_CNS, NULL, 0 , smemXCatActiveWriteHaGlobalConfigReg, 0},
    /*MLL Global Configuration*/
    {0x0d800000, SMEM_FULL_MASK_CNS, NULL, 0 , smemXCatActiveWriteMllGlobalConfigReg, 0},

    /*L2 MLL*/
    {0x0D800464, SMEM_FULL_MASK_CNS, NULL, 0 , smemXCatActiveWriteL2MllVidxEnable,  0},
    {0x0D800474, SMEM_FULL_MASK_CNS, NULL, 0 , smemXCatActiveWriteL2MllPointerMap,  0}

};

#define EXTRA_XCAT_ACTIVE_TABLE_SIZE (sizeof(smemXcatActiveTableExtra)/sizeof(smemXcatActiveTableExtra[0]))


/* Active memory table */
static SMEM_ACTIVE_MEM_ENTRY_STC smemXcatActiveTable[XCAT_ACTIVE_TABLE_SIZE+EXTRA_XCAT_ACTIVE_TABLE_SIZE] =
{
    {END_OF_TABLE, SMEM_FULL_MASK_CNS, NULL,0,NULL,0}
    /* filled with entries from ch3 during initialization */
    /* new entries can be added to those of the ch3 by using new array for 'xcat extra' */
};

static GT_BOOL smemXcatActiveTableInitialized = GT_FALSE;


/* Active memory table */
static SMEM_ACTIVE_MEM_ENTRY_STC smemCht3PexActiveTable[] =
{
    /* PEX Interrupt Cause Register */
    {0x00071900, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteIntrCauseReg, 0x00FF1F1F},

    /* must be last anyway */
    {0xffffffff, SMEM_FULL_MASK_CNS, NULL, 0, NULL, 0}
};

#define SMEM_ACTIVE_PCI_MEM_TABLE_SIZE \
    (sizeof(smemCht3PexActiveTable)/sizeof(smemCht3PexActiveTable[0]))

/*******************************************************************************
*   smemCht3Init
*
* DESCRIPTION:
*       Init memory module for the Cheetah3 device.
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
*******************************************************************************/
void smemCht3Init
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    GT_U32  ii,jj;
    SMEM_CHT3_DEV_MEM_INFO  * devMemInfoPtr;
    SMEM_ACTIVE_MEM_ENTRY_REPLACEMENT_STC     *replacementMemoryPtr;
    SMEM_CHT_DEV_COMMON_MEM_INFO * commonDevMemInfoPtr;
    GT_U32 regData;
    GT_STATUS rc;
    GT_BIT  isXcatA3Features = 0;/* is xcat-A3 ? */

    devMemInfoPtr = (SMEM_CHT3_DEV_MEM_INFO *)smemDeviceObjMemoryAlloc(devObjPtr,1, sizeof(SMEM_CHT3_DEV_MEM_INFO));
    if (devMemInfoPtr == 0)
    {
        skernelFatalError("smemCht3Init: allocation error\n");
    }
    devObjPtr->deviceMemory = devMemInfoPtr;
    commonDevMemInfoPtr= &devMemInfoPtr->common;
    commonDevMemInfoPtr->pciUnitBaseAddrMask = 0x000f0000;

    devObjPtr->supportMacSaAssignModePerPort = 1;
    devObjPtr->cncBlockMaxRangeIndex = 2048;
    devObjPtr->cncClientSupportBitmap = SNET_CHT3_CNC_CLIENTS_BMP_ALL_CNS;
    devObjPtr->supportForwardFcPackets = 1;

    /* set router tcam , PCL tacm info -->
       must be done before calling smemCht3AllocSpecMemory(...)*/
    if(SKERNEL_DEVICE_FAMILY_CHEETAH_3_ONLY(devObjPtr))
    {
        commonDevMemInfoPtr->pciUnitBaseAddr = 0x00070000;
        devObjPtr->routeTcamInfo =  ch3RoutTcamInfo;
        devObjPtr->pclTcamInfoPtr = &cht3GlobalPclData;
        devObjPtr->pclTcamMaxNumEntries = devObjPtr->pclTcamInfoPtr->bankEntriesNum * 4;

        devObjPtr->cncBlocksNum = 10;
        devObjPtr->fdbNumEntries = SMEM_MAC_TABLE_SIZE_32KB;
    }
    else
    {
        devObjPtr->supportCpuInternalInterface = 1;
        /* xcat */
        commonDevMemInfoPtr->pciUnitBaseAddr = 0x000f0000;
        commonDevMemInfoPtr->pciExtMemArr[SMEM_UNIT_PCI_BUS_MBUS_E].unitBaseAddr = commonDevMemInfoPtr->pciUnitBaseAddr;
        devObjPtr->routeTcamInfo =  xcatRoutTcamInfo;
        devObjPtr->pclTcamInfoPtr = &xcatGlobalPclData;
        devObjPtr->pclTcamMaxNumEntries = devObjPtr->pclTcamInfoPtr->bankEntriesNum * 4;
        devObjPtr->cncBlocksNum = 1;
        devObjPtr->fdbNumEntries = SMEM_MAC_TABLE_SIZE_16KB;
        devObjPtr->policerEngineNum = 1;
        devObjPtr->errata.lttRedirectIndexError = 1;
        devObjPtr->errata.routerGlueResetPartsOfDescriptor = 1;
        devObjPtr->errata.ttCopyToCpuWithAdditionalTag = 1;

        if(devObjPtr->deviceRevisionId == 0)
        {
            devObjPtr->isXcatA0Features = 1;
        }
        else
        {
            devObjPtr->isXcatA0Features = 0;

            devObjPtr->cncBlocksNum = 2;
            devObjPtr->supportPortIsolation = 1;
            devObjPtr->supportOamPduTrap = 1;
            devObjPtr->supportPortOamLoopBack = 1;
            devObjPtr->policerEngineNum = 2;
            devObjPtr->supportMultiAnalyzerMirroring = 1;
            devObjPtr->supportLogicalTargetMapping = 1;
            devObjPtr->ipFixTimeStampClockDiff[0] =
            devObjPtr->ipFixTimeStampClockDiff[1] =
            devObjPtr->ipFixTimeStampClockDiff[2] = 0;
            devObjPtr->supportConfigurableTtiPclId = 1;
            devObjPtr->supportVlanEntryVidxFloodMode = 1;
            devObjPtr->errata.srcTrunkToCpuIndication = 1;
            devObjPtr->errata.ieeeReservedMcConfigRegRead = 1;
            devObjPtr->errata.ipfixWrapArroundFreezeMode = 1;
            devObjPtr->errata.tunnelStartPassengerEthPacketCrcRemoved = 1;
            devObjPtr->policerSupport.supportPolicerMemoryControl = 1;
            devObjPtr->policerSupport.iplr0TableSize = POLICER_MEMORY_1792_CNS;
            devObjPtr->policerSupport.iplr1TableSize = POLICER_MEMORY_256_CNS;
            devObjPtr->policerSupport.iplrTableSize =
                devObjPtr->policerSupport.iplr0TableSize +
                devObjPtr->policerSupport.iplr1TableSize;
            devObjPtr->policerSupport.supportIpfixTimeStamp = 1;

            devObjPtr->errata.fastStackFailover_Drop_on_source = 1;


            /* XCat A2 and above related features */
            if (devObjPtr->deviceRevisionId >= 3)
            {
                devObjPtr->supportRemoveVlanTag1WhenEmpty = 1;
                devObjPtr->errata.supportSrcTrunkToCpuIndicationEnable = 1;
            }

            if(devObjPtr->deviceRevisionId == 3)
            {
                /* Metal Fix Register bit [2] as the xCat A3 revision identification */
                rc = smainMemAdditionalAddrDefaultGet(devObjPtr,
                                                      devObjPtr->deviceId,
                                                      SMEM_XCAT_MG_METAL_FIX_REG(devObjPtr),
                                                      &regData);
                if(rc == GT_OK)
                {
                    isXcatA3Features = ((regData >> 2) & 0x1);
                }

                /* Metal Fix Register bit [31] as the xCat C0 revision identification */
                rc = smainMemAdditionalAddrDefaultGet(devObjPtr,
                                                      devObjPtr->deviceId,
                                                      SMEM_XCAT_TTI_INTERNAL_METAL_FIX_REG(devObjPtr),
                                                      &regData);
                if(rc == GT_OK && SMEM_U32_GET_FIELD(regData,31,1))
                {
                    /* for simulation we will call it 'Revision 4' */
                    devObjPtr->deviceRevisionId = 4;
                }
            }

            if (devObjPtr->deviceRevisionId >= 4)
            {
                isXcatA3Features = 1;

                devObjPtr->supportVpls = 1;
                devObjPtr->supportLogicalMapTableInfo.tableFormatVersion = 1;
                devObjPtr->supportLogicalMapTableInfo.supportFullRange = 1;
                devObjPtr->supportLogicalMapTableInfo.supportEgressVlanFiltering = 1;

                devObjPtr->l2MllVersionSupport = 1;
            }
        }

        if(isXcatA3Features)
        {
            devObjPtr->supportTunnelStartEthOverIpv4 = 1;
            devObjPtr->supportTunnelstartIpTotalLengthAddValue = 1;
        }

    }

    devObjPtr->fdbMaxNumEntries = devObjPtr->fdbNumEntries;
    devObjPtr->supportEqEgressMonitoringNumPorts = 32;

    /* init specific functions array */
    smemCht3InitFuncArray(devObjPtr,devMemInfoPtr);

    /* allocate address type specific memories */
    smemCht3AllocSpecMemory(devObjPtr,devMemInfoPtr);

    if(SKERNEL_DEVICE_FAMILY_CHEETAH_3_ONLY(devObjPtr))
    {
        devObjPtr->activeMemPtr = smemCht3ActiveTable;
    }
    else
    {
        /* xcat */
        devObjPtr->activeMemPtr = smemXcatActiveTable;

        if(SKERNEL_IS_XCAT_REVISON_A1_DEV(devObjPtr))
        {
            replacementMemoryPtr = smemXcatA1ActiveTableOverrideCh3;
        }
        else
        {
            replacementMemoryPtr = smemXcatActiveTableOverrideCh3;
        }

        if(smemXcatActiveTableInitialized == GT_FALSE)
        {
            smemXcatActiveTableInitialized = GT_TRUE;
            /* initialize smemXcatActiveTable */
            memcpy(smemXcatActiveTable, smemCht3ActiveTable, sizeof(smemCht3ActiveTable));

            /* initialize smemXcatActiveTableExtra starting
               from CH3 "Last Entry"*/
            memcpy((smemXcatActiveTable + XCAT_ACTIVE_TABLE_SIZE - 1),
                   smemXcatActiveTableExtra, sizeof(smemXcatActiveTableExtra));

            /* loop on entries to override */
            for(ii = 0; replacementMemoryPtr[ii].oldAddress != END_OF_TABLE; ii++)
            {
                for(jj = 0; jj < CH3_ACTIVE_MEM_TABLE_SIZE; jj++)
                {
                    if(replacementMemoryPtr[ii].oldAddress == smemCht3ActiveTable[jj].address)
                    {
                        memcpy(&smemXcatActiveTable[jj],
                               &replacementMemoryPtr[ii].newEntry,
                               sizeof(SMEM_ACTIVE_MEM_ENTRY_STC));
                    }
                }
            }
            /* add here loop on extra xcat line */

            /* set last line with 'end of table' indication */
            smemXcatActiveTable[XCAT_ACTIVE_TABLE_SIZE +
                                EXTRA_XCAT_ACTIVE_TABLE_SIZE - 1].address = END_OF_TABLE;
        }
    }

    for(ii = 0 ; ii < sizeof(smemXcatActiveTable) / sizeof(smemXcatActiveTable[0]) ; ii++)
    {
        if(smemXcatActiveTable[ii].mask ==
            POLICER_ACTIVE_MEMORY_PLACE_HOLDER_MASK_CNS)
        {
            GT_U32  tmpMask = SMEM_FULL_MASK_CNS;

            if(0 == SKERNEL_IS_XCAT_REVISON_A1_DEV(devObjPtr))
            {
                /* A0 */

                /* those are not relevant to A0 */
                smemXcatActiveTable[ii].address = 0xFFFFFFFF;/* no such address */
                smemXcatActiveTable[ii].mask    = 0xFFFFFFFF;/* exact match on this 'non-exist' address */

                continue;
            }


            if(smemXcatActiveTable[ii + 0].address == POLICER_MANAGEMENT_COUNTER_ADDR_CNS)
            {
                /* management counters */
                /* 0x40 between sets , 0x10 between counters */
                /* 0x00 , 0x10 , 0x20 , 0x30 */
                /* 0x40 , 0x50 , 0x60 , 0x70 */
                /* 0x80 , 0x90 , 0xa0 , 0xb0 */
                tmpMask = POLICER_MANAGEMENT_COUNTER_MASK_CNS;
            }

            smemXcatActiveTable[ii + 0].address += policerBaseAddr[0];
            smemXcatActiveTable[ii + 0].mask = tmpMask;

            smemXcatActiveTable[ii + 1].address += policerBaseAddr[1];
            smemXcatActiveTable[ii + 1].mask = tmpMask;

            smemXcatActiveTable[ii + 2].address += policerBaseAddr[2];
            smemXcatActiveTable[ii + 2].mask = tmpMask;

            ii += 2;/* the jump of 2 here + the one of the loop will
                       jump to next 3 policer if exists */
        }
    }

    if(SKERNEL_IS_XCAT_REVISON_A1_DEV(devObjPtr))
    {
        smemXcatA1TableInfoSet(devObjPtr);

        /* bind now tables to memories --
           MUST be done after calling smemXcatA1TableInfoSet(...)
           so the tables can override previous settings */
        smemBindTablesToMemories(devObjPtr,
            &devMemInfoPtr->haAndEpclUnitMem,
            1);
        smemBindTablesToMemories(devObjPtr,
            &devMemInfoPtr->eqUnitMem,
            1);
        smemBindTablesToMemories(devObjPtr,
            &devMemInfoPtr->ipclTccUnitMem,
            1);
        smemBindTablesToMemories(devObjPtr,
            &devMemInfoPtr->ipvxTccUnitMem,
            1);
        smemBindTablesToMemories(devObjPtr,
            &devMemInfoPtr->uniphySerdesUnitMem,
            1);
        smemBindTablesToMemories(devObjPtr,
            (void*)&devMemInfoPtr->xCatExtraMem,
            sizeof(SMEM_XCAT_DEV_EXTRA_MEM_INFO) / sizeof(SMEM_UNIT_CHUNKS_STC));

        /* override info set by the 'auto bind' */
        smemXcatA1TableInfoSetPart2(devObjPtr);
    }
    else
    {
        smemCht3TableInfoSet(devObjPtr);
    }

}

/*******************************************************************************
*   smemCht3Init2
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
void smemCht3Init2
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    GT_U32  serdes,port;
    GT_U32  serdesBaseAddr = SMEM_CHT3_SERDES_SPEED_1_REG(devObjPtr);/* SERDES Speed Register1 */
    GT_U32  currAddr;

    if(SKERNEL_IS_CHEETAH3_ONLY_DEV(devObjPtr))
    {
        /* no more need to do */
        return;
    }

    /* read the registers - as was set during load of the 'Registers defaults file' */
    if(!SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        /*Bridge Global Configuration2*/
        currAddr = SMEM_CHT_BRIDGE_GLOBAL_CONF2_REG(devObjPtr);
        smemRegUpdateAfterRegFile(devObjPtr,currAddr,1);

        /*Pre-Egress Engine Global Configuration*/
        currAddr = SMEM_CHT_PRE_EGR_GLB_CONF_REG(devObjPtr);
        smemRegUpdateAfterRegFile(devObjPtr,currAddr,1);
    }

    /*Ingress Policy Global Configuration*/
    currAddr = SMEM_CHT_PCL_GLOBAL_REG(devObjPtr);
    smemRegUpdateAfterRegFile(devObjPtr,currAddr,1);
    /*TTI Global Configuration*/
    currAddr = SMEM_XCAT_A1_TTI_UNIT_GLB_CONF_REG(devObjPtr);
    smemRegUpdateAfterRegFile(devObjPtr,currAddr,1);
    /*HA Global Configuration*/
    currAddr = SMEM_CHT3_ROUTE_HA_GLB_CNF_REG(devObjPtr);
    smemRegUpdateAfterRegFile(devObjPtr,currAddr,1);

    if(devObjPtr->policerSupport.supportPolicerMemoryControl)
    {
        /* Policer Memory Control */
        if (SKERNEL_IS_XCAT2_DEV(devObjPtr))
        {
            currAddr = SMEM_XCAT_PLR_HIERARCHICAL_POLICER_CTRL_REG(devObjPtr,  0);  /* Hierarchical Policer control register */
        }
        else
        {
            currAddr = SMEM_CHT_POLICER_GLB_CONF_REG(devObjPtr, 0 /* cycle */);
        }
        /* update the register , using the SCIB interface so the
          'Active memory' will be called --> meaning that function
           smemXCatActiveWritePolicerMemoryControl(...) will be called */
        smemRegUpdateAfterRegFile(devObjPtr,currAddr,1);
    }

    if(SKERNEL_DEVICE_FAMILY_XCAT2_ONLY(devObjPtr))
    {
        /* no more need to do */
        return;
    }

    if(SKERNEL_IS_XCAT_REVISON_A1_DEV(devObjPtr))
    {
        /* check the port registers to see what ports uses GE/XG registers */
        for(port = 0 ; port < devObjPtr->portsNumber  ; port++)
        {
            if(devObjPtr->supportRegistersDb &&
               (0 == IS_CHT_HYPER_GIGA_PORT(devObjPtr,port)))
            {
                /* no XG mac for this port */
                continue;
            }

            currAddr = SMEM_XCAT_XG_MAC_CONTROL3_REG(devObjPtr, port);
            /* update the register back , using the SCIB interface so the
              'Active memory' will be called --> meaning that function
               smemXcatActiveWriteMacModeSelect(...) will be called */
            smemRegUpdateAfterRegFile(devObjPtr,currAddr,1);
        }
    }
    else if (skernelIsXcatOnlyDev(devObjPtr))  /* only for XCAT A0 */
    {
        /* check the serdess registers to see what ports 24..27 uses GE/XG registers */
        for(serdes = 32 ; serdes < 47 ; serdes+=4)
        {
            currAddr = serdesBaseAddr + serdes * 0x400;
            /* update the register back , using the SCIB interface so the
              'Active memory' will be called --> meaning that function
               smemCht3ActiveWriteSerdesSpeed(...) will be called */
            smemRegUpdateAfterRegFile(devObjPtr,currAddr,1);
        }
    }

    if (devObjPtr->errata.supportSrcTrunkToCpuIndicationEnable)
    {
        currAddr = SMEM_XCAT_TTI_INTERNAL_METAL_FIX_REG(devObjPtr);
        /* write the register back , using the SCIB interface so the
          'Active memory' will be called --> meaning that function
           smemXCatActiveWriteTtiInternalMetalFix(...) will be called */
        smemRegUpdateAfterRegFile(devObjPtr,currAddr,1);
    }

    return;
}



/*******************************************************************************
*   smemCht3FindMem
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
static void * smemCht3FindMem
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32                  address,
    IN GT_U32                  memSize,
    OUT SMEM_ACTIVE_MEM_ENTRY_STC ** activeMemPtrPtr
)
{
    void                    * memPtr;
    SMEM_CHT3_DEV_MEM_INFO  * devMemInfoPtr;
    GT_32                     index;
    GT_UINTPTR                param;

    if (devObjPtr == 0)
    {
        skernelFatalError("smemCht3FindMem: illegal pointer \n");
    }

    memPtr = 0;
    devMemInfoPtr = (SMEM_CHT3_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    /* Find PCI registers memory  */
    if (SMEM_ACCESS_PCI_FULL_MAC(accessType))
    {
        memPtr = smemCht3PciReg(devObjPtr, accessType, address, memSize, 0);

        /* find PCI active memory entry */
        if (activeMemPtrPtr != NULL)
        {
            for (index = 0; index < (SMEM_ACTIVE_PCI_MEM_TABLE_SIZE - 1);
                  index++)
            {
                /* check address */
                if ((address & smemCht3PexActiveTable[index].mask)
                      == smemCht3PexActiveTable[index].address)
                {
                    *activeMemPtrPtr = &smemCht3PexActiveTable[index];
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
        for (index = 0; devObjPtr->activeMemPtr[index].address != END_OF_TABLE; index++)
        {
            /* check address    */
            if ((address & devObjPtr->activeMemPtr[index].mask)
                 == devObjPtr->activeMemPtr[index].address)
            {
                *activeMemPtrPtr = &devObjPtr->activeMemPtr[index];
                break;
            }
        }
    }

    return memPtr;
}

/*******************************************************************************
*   smemCht3GlobalReg
*
* DESCRIPTION:
*       Global Registers, TWSI, CPU port, SDMA and PCI internal registers
*
* INPUTS:
*       devObjPtr   - pointer to device object.
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
static GT_U32 *  smemCht3GlobalReg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32                  address,
    IN GT_U32                  memSize,
    IN GT_UINTPTR                  param
)
{
    SMEM_CHT3_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32              * regValPtr;
    GT_U32                index;

    regValPtr = 0;
    devMemInfoPtr = (SMEM_CHT3_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    /* Global registers */
    if ((address & 0x1FFFF000) == 0x0)
    {
        index = (address & 0xFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->globalMem.globReg,
                         devMemInfoPtr->globalMem.globRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->globalMem.globReg[index];
    }
    else
    /* PEX registers */
    if ((address & 0x1FFFF000) == 0x00001000)
    {
        index = (address & 0xFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->globalMem.pexReg,
                         devMemInfoPtr->globalMem.pexRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->globalMem.pexReg[index];
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
    if ((address & 0x1FFFFF00) == 0x00010000)
    {
        index = (address & 0xFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->globalMem.twsiIntRegs,
                         devMemInfoPtr->globalMem.twsiIntRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->globalMem.twsiIntRegs[index];
    }
    else
    /* EPCL-ID configuration table */
    if ((address & 0xFFFF8000) == 0x00018000)
    {
        index = (address & 0x7FFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->globalMem.epclConfigTblReg,
                         devMemInfoPtr->globalMem.epclConfigTblRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->globalMem.epclConfigTblReg[index];
    }
    else
    /* MAC SRC Entry (0<=n<4096) */
    {
        index = (address & 0x1ffff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->globalMem.macSaTblReg,
                         devMemInfoPtr->globalMem.macSaTblRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->globalMem.macSaTblReg[index];
    }


    return regValPtr;
}

/*******************************************************************************
*   smemCht3TransQueReg
*
* DESCRIPTION:
*       Egress, GPP configuration registers ,
*       Transmit Queue and VLAN Configuration Register Map Table,
*       Port TxQ Configuration Register, Hyper.GStack Ports MIB Counters
*
* INPUTS:
*       devObjPtr   - pointer to device object.
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
static GT_U32 *  smemCht3TransQueReg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SMEM_CHT3_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32                  * regValPtr;
    GT_U32                    index;
    GT_U32                    group;

    regValPtr = 0;
    devMemInfoPtr = (SMEM_CHT3_DEV_MEM_INFO *)devObjPtr->deviceMemory;

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
    /* Secondary Target Port Map  Registers */
    if ((address & 0x1FFF0000) == 0x01E40000)
    {
        index = (address & 0xFFFF) / 0x10;
        CHECK_MEM_BOUNDS(devMemInfoPtr->egrMem.secondaryTargetPortMapReg,
                         devMemInfoPtr->egrMem.secondaryTargetPortMapRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->egrMem.secondaryTargetPortMapReg[index];
    }
    else
    /* Stack TC Remap Per Profile Table Entry <%n> */
    if ((address & 0x1FFF0000) == 0x01E80000)
    {
        index = (address & 0xFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->egrMem.stackTcRemapMapReg,
                         devMemInfoPtr->egrMem.stackTcRemapRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->egrMem.stackTcRemapMapReg[index];
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
        index = 3 *((address & 0x3FFF) / 0x200)  + ((address & 0xC)>>2);
        if((address & 0xC) == 0xC)/* I.E address 0x01D4000C + 0x200*j    */
        {
            return NULL;
              /*skernelFatalError(" not defined memory ");*/
        }
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
*   smemCht3EtherBrdgReg
*
* DESCRIPTION:
*       Bridge Configuration Registers
*
* INPUTS:
*       devObjPtr   - pointer to device object.
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
static GT_U32 *  smemCht3EtherBrdgReg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SMEM_CHT3_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32                  * regValPtr;
    GT_U32                    index;

    regValPtr = 0;
    devMemInfoPtr = (SMEM_CHT3_DEV_MEM_INFO  *)devObjPtr->deviceMemory;
    if ((address & 0xFF000000) == 0x02000000)
    {
        index = (address & 0x000FFFFF) / 4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->brdgMngMem.bridgeGenReg,
                         devMemInfoPtr->brdgMngMem.bridgeGenRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->brdgMngMem.bridgeGenReg[index];
    }
    else
    {
        if(GT_FALSE == skernelUserDebugInfo.disableFatalError)
        {
            skernelFatalError("smemCht3EtherBrdgReg: not supported addr [0x8.8x]\n",address);
        }
    }

    return regValPtr;
}

/*******************************************************************************
*   smemCht3HaAndEpclConfReg
*
* DESCRIPTION:
*       Describe a device's HA registers and EPCL memory object
*
* INPUTS:
*       devObjPtr   - pointer to device object.
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
static GT_U32 *  smemCht3HaAndEpclConfReg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    return smemDevFindInUnitChunk(devObjPtr, accessType, address, memSize,
        (GT_UINTPTR)&(((SMEM_CHT3_DEV_MEM_INFO*)devObjPtr->deviceMemory)->haAndEpclUnitMem));
}

/*******************************************************************************
*   smemCht3PortGroupConfReg
*
* DESCRIPTION:
*       Describe a device's Port registers memory object
*
* INPUTS:
*       devObjPtr   - pointer to device object.
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
static GT_U32 *  smemCht3PortGroupConfReg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SMEM_CHT3_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32                  * regValPtr;
    GT_U32                    index;
    GT_U32                    groupNum;
    GT_U32                    groupMaskResult;

    regValPtr = 0;
    devMemInfoPtr = (SMEM_CHT3_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    /* LMS group (GOP) is para, */
    groupNum = param;

    switch(groupNum)
    {
    case 0:
        groupMaskResult = 0x04004000;
        break;
    case 1:
        groupMaskResult = 0x04804000;
        break;
    case 2:
        groupMaskResult = 0x05004000;
        break;
    case 3:
        groupMaskResult = 0x05804000;
        break;
    default:
        return regValPtr;
    }

    /* Group configuration registers, MAC MIB Counters registers,
       LED Interfaces Configuration Registers*/
    if ((address & 0xFFFF4000) == groupMaskResult)
    {
        if ((address & 0x0000F000) == 0x4000)
        {
            /* addresses with mask xxx04xxx */
            index = (address & 0xFFF) / 0x4;
            CHECK_MEM_BOUNDS(devMemInfoPtr->gopCnfMem.portGroupReg[groupNum],
                             devMemInfoPtr->gopCnfMem.portGroupRegNum,
                             index, memSize);
            regValPtr = &devMemInfoPtr->gopCnfMem.portGroupReg[groupNum][index];
        }
        else
        {
            /* addresses with mask xxx051xx */
            index = (address & 0xFF) / 0x4;
            CHECK_MEM_BOUNDS(devMemInfoPtr->gopCnfMem.xgPortGroupReg[groupNum],
                             devMemInfoPtr->gopCnfMem.xgPortGroupRegNum,
                             index, memSize);
            regValPtr = &devMemInfoPtr->gopCnfMem.xgPortGroupReg[groupNum][index];
        }
    }
    else
    {
        /* addresses with mask xxx10xxx */
        if ((address & 0x000F0000) == 0x10000)
        {
            index = (address & 0xFFF) / 0x4;
            CHECK_MEM_BOUNDS(devMemInfoPtr->gopCnfMem.macMibCountReg[groupNum],
                             devMemInfoPtr->gopCnfMem.macMibCountRegNum,
                             index, memSize);
            regValPtr = &devMemInfoPtr->gopCnfMem.macMibCountReg[groupNum][index];
        }
        else
        {
            /* all other */
            index = (address & 0xFF) / 0x4;
            CHECK_MEM_BOUNDS(devMemInfoPtr->gopCnfMem.xgMacMibCountReg[groupNum],
                             devMemInfoPtr->gopCnfMem.xgMacMibCountRegNum,
                             index, memSize);
            regValPtr = &devMemInfoPtr->gopCnfMem.xgMacMibCountReg[groupNum][index];
        }

    }

    return regValPtr;
}

/*******************************************************************************
*   smemCht3MacTableReg
*
* DESCRIPTION:
*       Describe a device's Bridge registers and FDB
*
* INPUTS:
*       devObjPtr   - pointer to device object.
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
static GT_U32 *  smemCht3MacTableReg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SMEM_CHT3_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32                  * regValPtr;
    GT_U32                    index;

    regValPtr = 0;
    devMemInfoPtr = (SMEM_CHT3_DEV_MEM_INFO  *)devObjPtr->deviceMemory;
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
*   smemCht3InterBufBankReg
*
* DESCRIPTION:
*       Describe a buffers memory banks data register and EPCL-ID configuration.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
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
static GT_U32 *  smemCht3InterBufBankReg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SMEM_CHT3_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32                  * regValPtr;
    GT_U32                    index;
    GT_U32                    groupNum;

    regValPtr = NULL;
    devMemInfoPtr = (SMEM_CHT3_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    groupNum = param;
    if (address == 0x06900000 || address == 0x07100000)
    {
        regValPtr = &devMemInfoPtr->banksMem.bankWriteReg[groupNum];
    }
    else if((address >= 0x07000000 && address < 0x07100000) ||
            (address >= 0x06800000 && address < 0x06900000))
    {
        index = (address & 0x7FFFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->banksMem.bankMemReg[groupNum],
                         devMemInfoPtr->banksMem.bankMemRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->banksMem.bankMemReg[groupNum][index];
    }

    return regValPtr;
}

/*******************************************************************************
*   smemCht3BufMngReg
*
* DESCRIPTION:
*       Describe a device's buffer management registers memory object.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
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
static GT_U32 *  smemCht3BufMngReg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SMEM_CHT3_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32                  * regValPtr;
    GT_U32                    index;

    regValPtr = 0;
    devMemInfoPtr = (SMEM_CHT3_DEV_MEM_INFO *)devObjPtr->deviceMemory;

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

    return regValPtr;
}

/*******************************************************************************
*  smemCht3RouterReg
*
* DESCRIPTION:
*      Router Engine registers and tables.
*
* INPUTS:
*       devObjPtr - device object PTR.
*       address     - Address for ASIC memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers' specific parameter.
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
static GT_U32 * smemCht3RouterReg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SMEM_CHT3_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32                  * regValPtr;
    GT_U32                    index;

    regValPtr = 0;
    devMemInfoPtr = (SMEM_CHT3_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    /* Age  */
    if ((address & 0xFFFFF000) == 0x02801000)/*also in xcat*/
    {
        index = (address & 0xFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->routerMem.ageReg,
                         devMemInfoPtr->routerMem.ageRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->routerMem.ageReg[index];
    }
    else
    /* Global and Configuration registers. */
    if ((address & 0xFFFFF000) == 0x02800000) /*also in xcat*/
    {
        index = (address & 0xFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->routerMem.configReg,
                         devMemInfoPtr->routerMem.configRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->routerMem.configReg[index];
    }
    else
    /* TCAM test registers. */
    if ((address & 0xFFFFF000) == 0x02802000)
    {
        index = (address & 0xFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->routerMem.tcamTestReg,
                         devMemInfoPtr->routerMem.tcamTestRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->routerMem.tcamTestReg[index];
    }
    else
    /* Next hop table */
    if ((address & 0xFFF00000) == 0x02B00000)/*also in xcat*/
    {
        index = (address & 0xFFFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->routerMem.nextHopTblReg ,
                         devMemInfoPtr->routerMem.nextHopTblRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->routerMem.nextHopTblReg[index];
    }
    else
    if(SKERNEL_IS_CHEETAH3_ONLY_DEV(devObjPtr)) /*for xcat to other unit "IPvX TCC " with base address 0x0d800000*/
    {
        /* TCAM registers. */
        if ((address & 0xFFF00000) == (GT_U32)SSMEM_CHT3_TCAM_X_DATA_TBL_MEM(devObjPtr)/*0x02A00000*/)   /*for xcat to other unit "IPvX TCC " with address 0x0D840000 called Router and TT TCAM*/
        {
            index = (address & 0xFFFFF) / 0x4;
            CHECK_MEM_BOUNDS(devMemInfoPtr->routerMem.tcamReg,
                             devMemInfoPtr->routerMem.tcamRegNum,
                             index, memSize);
            regValPtr = &devMemInfoPtr->routerMem.tcamReg[index];
        }
        else
        /* Action Table registers */
        if ((address & 0xFFF00000) == (GT_U32)SMEM_CHT3_TUNNEL_ACTION_TBL_MEM(devObjPtr, 0)/*0x02900000*/) /*for xcat to other unit "IPvX TCC " with address 0x0d880000*/
        {
            index = (address & 0xFFFFF) / 0x4;
            CHECK_MEM_BOUNDS(devMemInfoPtr->routerMem.actionTblReg,
                             devMemInfoPtr->routerMem.actionTblRegNum,
                             index, memSize);
            regValPtr = &devMemInfoPtr->routerMem.actionTblReg[index];
        }
        else
        /*ECC Table X */
        if ((address & 0xFFFF0000) == 0x02C00000)/*xcat ECC Table X -->0x0D890000 ,ECC Table Y -->0xD8a0000 */
        {
            index = (address & 0xFF) / 0x4;
            CHECK_MEM_BOUNDS(devMemInfoPtr->routerMem.vrfTblReg,
                             devMemInfoPtr->routerMem.vrfTblRegNum,
                             index, memSize);
            regValPtr = &devMemInfoPtr->routerMem.vrfTblReg[index];
        }
    }

    return   regValPtr;
}

/*******************************************************************************
*   smemCht3CentalCntReg
*
* DESCRIPTION:
*       Describe a centralized counters memory
*
* INPUTS:
*       devObjPtr   - pointer to device object.
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
static GT_U32 *  smemCht3CentalCntReg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SMEM_CHT3_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32              * regValPtr;
    GT_U32                index;
    GT_U32                cntGroup;

    regValPtr = 0;
    devMemInfoPtr = (SMEM_CHT3_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    if ((address & 0xFFFFF000) == 0x08000000)
    {
        index = (address & 0xFFF) / 4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->centralCntMem.cncCntrsReg,
                         devMemInfoPtr->centralCntMem.cncCntrsRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->centralCntMem.cncCntrsReg[index];
    }
    else
    {
        if ((address & 0xFFFFF000) == 0x08001000)
        {
            index = (address & 0xFFF) / 4;
            CHECK_MEM_BOUNDS(devMemInfoPtr->centralCntMem.cncCntrsPerBlockReg,
                             devMemInfoPtr->centralCntMem.cncCntrsPerBlockRegNum,
                             index, memSize);
            regValPtr = &devMemInfoPtr->centralCntMem.cncCntrsPerBlockReg[index];
        }
        else
        {
            if ((address & 0xFFF80000) == 0x08080000)
            {
                cntGroup = (address >> 14) & 0xF;
                if(cntGroup >= (GT_U32)CNC_CNT_BLOCKS(devObjPtr))
                {
                    if(GT_FALSE == skernelUserDebugInfo.disableFatalError)
                    {
                        skernelFatalError("smemCht3CentalCntReg: cntGroup value %d out of range, address %x\n",
                                            cntGroup, address);
                    }
                    else
                    {
                        return NULL;
                    }
                }

                index = (address & 0x3FFF) / 4;
                CHECK_MEM_BOUNDS(devMemInfoPtr->centralCntMem.cncCntrsTbl[cntGroup],
                                 devMemInfoPtr->centralCntMem.cncCntrsTblNum,
                                 index, memSize);
                regValPtr = &devMemInfoPtr->centralCntMem.cncCntrsTbl[cntGroup][index];
            }
        }
    }

    return regValPtr;
}

/*******************************************************************************
*   smemCht3InitMemArray
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
static void smemCht3InitFuncArray
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_CHT3_DEV_MEM_INFO  * devMemInfoPtr
)
{
    GT_U32              i;

    for (i = 0; i < 64; i++)
    {
        devMemInfoPtr->common.specFunTbl[i].specFun    = smemChtFatalError;
    }
    devMemInfoPtr->common.specFunTbl[0].specFun        = smemCht3GlobalReg;

    devMemInfoPtr->common.specFunTbl[3].specFun        = smemCht3TransQueReg;

    devMemInfoPtr->common.specFunTbl[4].specFun        = smemCht3EtherBrdgReg;

    devMemInfoPtr->common.specFunTbl[5].specFun        = smemCht3RouterReg;

    devMemInfoPtr->common.specFunTbl[6].specFun        = smemCht3BufMngReg;

    devMemInfoPtr->common.specFunTbl[8].specFun        = smemCht3PortGroupConfReg;
    devMemInfoPtr->common.specFunTbl[8].specParam      = 0;

    devMemInfoPtr->common.specFunTbl[9].specFun        = smemCht3PortGroupConfReg;
    devMemInfoPtr->common.specFunTbl[9].specParam      = 1;

    devMemInfoPtr->common.specFunTbl[10].specFun       = smemCht3PortGroupConfReg;
    devMemInfoPtr->common.specFunTbl[10].specParam     = 2;

    devMemInfoPtr->common.specFunTbl[11].specFun       = smemCht3PortGroupConfReg;
    devMemInfoPtr->common.specFunTbl[11].specParam     = 3;

    devMemInfoPtr->common.specFunTbl[12].specFun       = smemCht3MacTableReg;

    devMemInfoPtr->common.specFunTbl[13].specFun       = smemCht3InterBufBankReg;
    devMemInfoPtr->common.specFunTbl[13].specParam     = 0;

    devMemInfoPtr->common.specFunTbl[14].specFun       = smemCht3InterBufBankReg;
    devMemInfoPtr->common.specFunTbl[14].specParam     = 1;

    devMemInfoPtr->common.specFunTbl[15].specFun       = smemCht3HaAndEpclConfReg;

    devMemInfoPtr->common.specFunTbl[16].specFun       = smemCht3CentalCntReg;

    devMemInfoPtr->common.specFunTbl[17].specFun       = smemCht3XgPortsReg;

    devMemInfoPtr->common.specFunTbl[18].specFun       = smemCht3XgMibReg;

    devMemInfoPtr->common.specFunTbl[19].specFun       = smemCht3SataSerdesReg;

    devMemInfoPtr->common.specFunTbl[20].specFun       = smemCht3VlanTableReg;

    devMemInfoPtr->common.specFunTbl[21].specFun       = smemCht3TriSpeedReg;

    devMemInfoPtr->common.specFunTbl[22].specFun       = smemCht3PreEgressReg;

    devMemInfoPtr->common.specFunTbl[23].specFun       = smemCht3PclReg;

    devMemInfoPtr->common.specFunTbl[24].specFun       = smemCht3PolicerReg;

    devMemInfoPtr->common.specFunTbl[25].specFun       = smemCht3MllEngineReg;

    if(SKERNEL_IS_XCAT_DEV(devObjPtr))
    {

        devMemInfoPtr->common.specFunTbl[26].specFun       = smemCht3IpclTccEngineReg;

        devMemInfoPtr->common.specFunTbl[27].specFun       = smemCht3IPvxTccEngineReg;
    }

    if(SKERNEL_IS_XCAT_REVISON_A1_DEV(devObjPtr))
    {
        devMemInfoPtr->common.specFunTbl[1].specFun        = smemXcatDfxMemReg;
        devMemInfoPtr->common.specFunTbl[3].specFun        = smemXcatTransQueReg;
        devMemInfoPtr->common.specFunTbl[4].specFun        = smemXcatEtherBrdgReg;

        devMemInfoPtr->common.specFunTbl[6].specFun        = smemXcatBufMngUnitReg;
        devMemInfoPtr->common.specFunTbl[7].specFun        = smemXcatPolicerReg;
        devMemInfoPtr->common.specFunTbl[7].specParam      = 2;

        devMemInfoPtr->common.specFunTbl[20].specFun       = smemXcatVlanTableReg;

        devMemInfoPtr->common.specFunTbl[22].specFun       = smemXcatPreEgressUnitReg;
        devMemInfoPtr->common.specFunTbl[23].specFun       = smemXcatPclReg;
        devMemInfoPtr->common.specFunTbl[24].specFun       = smemXcatTtiReg;
        devMemInfoPtr->common.specFunTbl[25].specFun       = smemXcatPolicerReg;
        devMemInfoPtr->common.specFunTbl[25].specParam     = 0;
        devMemInfoPtr->common.specFunTbl[26].specFun       = smemXcatPolicerReg;
        devMemInfoPtr->common.specFunTbl[26].specParam     = 1;
        devMemInfoPtr->common.specFunTbl[27].specFun       = smemXcatMllEngineReg;
        devMemInfoPtr->common.specFunTbl[28].specFun       = smemXcatPclTccReg;
        devMemInfoPtr->common.specFunTbl[29].specFun       = smemXcatIPvxTccEngineUniReg;
        devMemInfoPtr->common.specFunTbl[30].specFun       = smemXcatHaAndEpclConfUnitReg;
        devMemInfoPtr->common.specFunTbl[31].specFun       = smemXcatEgrPclReg;

    }

    /* define TEMPORARY memory chunk for CH3+ */
    if(SKERNEL_IS_CHEETAH3P_ONLY_DEV(devObjPtr))
    {
        GT_U32  ii;

        /* check that non of the indexes already used by ch3 */
        for(ii = 0; ii < SMEM_CHT3_PLUS_EXTRA_MEM_NUM_INDEX ; ii++)
        {
            if(devMemInfoPtr->common.specFunTbl[ii+SMEM_CHT3_PLUS_EXTRA_MEM_INDEX].specFun != smemChtFatalError)
            {
                skernelFatalError("smemCht3InitFuncArray: the index of [%d] is already occupied , look for new one \n",
                    (ii+SMEM_CHT3_PLUS_EXTRA_MEM_INDEX));
            }
        }


        /* chunk of memory for CH3+ extra memory development */
        devMemInfoPtr->common.specFunTbl[SMEM_CHT3_PLUS_EXTRA_MEM_INDEX].specFun = smemCht3pExtraMem;
    }

}

/*******************************************************************************
*   smemCht3InitInternalSimMemory
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
static void smemCht3InitInternalSimMemory
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_CHT3_DEV_MEM_INFO  * devMemInfoPtr
)
{
    GT_U32  *internalMemPtr;

    /* allocate  and init the all cht devices common info */
    smemChtInitInternalSimMemory(devObjPtr,&devMemInfoPtr->common);

    /* init the specific bit 0 of this memory */
    internalMemPtr = CHT_INTERNAL_MEM_PTR(devObjPtr,
        CHT_INTERNAL_SIMULATION_USE_MEM_CAPWAP_RSSI_EXCEPTION_STATUS_E);

    *internalMemPtr = 1;

    return;
}
/*******************************************************************************
*   smemCht3AllocSpecMemory
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
static void smemCht3AllocSpecMemory
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_CHT3_DEV_MEM_INFO  * devMemInfoPtr
)
{
    GT_U8 i;

    /* init the internal memory */
    smemCht3InitInternalSimMemory(devObjPtr,devMemInfoPtr);

    /* Global register, TWSI, CPU port, SDMA and PCI internal registers */

    devMemInfoPtr->globalMem.globRegNum = GLB_REGS_NUM;
    devMemInfoPtr->globalMem.globReg =
        smemDevMemoryCalloc(devObjPtr, GLB_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->globalMem.globReg == 0)
    {
        skernelFatalError("smemCht3AllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->globalMem.sdmaRegNum = GLB_SDMA_REGS_NUM;
    devMemInfoPtr->globalMem.sdmaReg =
        smemDevMemoryCalloc(devObjPtr, GLB_SDMA_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->globalMem.sdmaReg == 0)
    {
        skernelFatalError("smemCht3AllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->globalMem.twsiIntRegsNum = TWSI_INT_REGS_NUM;
    devMemInfoPtr->globalMem.twsiIntRegs =
        smemDevMemoryCalloc(devObjPtr, TWSI_INT_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->globalMem.twsiIntRegs == 0)
    {
        skernelFatalError("smemCht3AllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->globalMem.pexRegNum = PEX_REGS_NUM;
    devMemInfoPtr->globalMem.pexReg =
        smemDevMemoryCalloc(devObjPtr, PEX_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->globalMem.pexReg == 0)
    {
        skernelFatalError("smemCht3AllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->globalMem.epclConfigTblRegNum = EPCL_CONF_REGS_NUM;
    devMemInfoPtr->globalMem.epclConfigTblReg =
            smemDevMemoryCalloc(devObjPtr, EPCL_CONF_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->globalMem.epclConfigTblReg == 0)
    {
        skernelFatalError("smemCht3AllocSpecMemory: allocation error\n");
    }

    /*  VLAN/Port MAC SA Table */
    devMemInfoPtr->globalMem.macSaTblRegNum = MAC_SA_TABLE_REGS_NUM;
    devMemInfoPtr->globalMem.macSaTblReg =
        smemDevMemoryCalloc(devObjPtr, MAC_SA_TABLE_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->globalMem.macSaTblReg == 0)
    {
        skernelFatalError("smemCht3AllocSpecMemory: allocation error\n");
    }

    /* GPP Configuration Registers , Egress,
       Transmit Queue and VLAN Configuration Register Map Table,
       Port TxQ Configuration Register, Hyper.GStack Ports MIB Counters */
    devMemInfoPtr->egrMem.egrGenRegNum = EGR_GLB_REGS_NUM;
    devMemInfoPtr->egrMem.egrGenReg =
        smemDevMemoryCalloc(devObjPtr, EGR_GLB_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->egrMem.egrGenReg == 0)
    {
        skernelFatalError("smemCht3AllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->egrMem.trQueueRegNum = EGR_TRUNK_FILTER_REGS_NUM;
    devMemInfoPtr->egrMem.trQueueReg =
        smemDevMemoryCalloc(devObjPtr, EGR_TRUNK_FILTER_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->egrMem.trQueueReg == 0)
    {
        skernelFatalError("smemCht3AllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->egrMem.portWdRegNum = EGR_PORT_WD_REGS_NUM;
    devMemInfoPtr->egrMem.portWdReg =
        smemDevMemoryCalloc(devObjPtr, EGR_PORT_WD_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->egrMem.portWdReg == 0)
    {
        skernelFatalError("smemCht3AllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->egrMem.secondaryTargetPortMapRegNum = EGR_SECONDARY_TARGET_REGS_NUM;
    devMemInfoPtr->egrMem.secondaryTargetPortMapReg =
        smemDevMemoryCalloc(devObjPtr, EGR_SECONDARY_TARGET_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->egrMem.secondaryTargetPortMapReg == 0)
    {
        skernelFatalError("smemCht3AllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->egrMem.stackTcRemapRegNum = EGR_STACK_TC_REMAP_REGS_NUM;
    devMemInfoPtr->egrMem.stackTcRemapMapReg =
        smemDevMemoryCalloc(devObjPtr, EGR_STACK_TC_REMAP_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->egrMem.stackTcRemapMapReg == 0)
    {
        skernelFatalError("smemCht3AllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->egrMem.egrMibCntRegNum = EGR_MIB_CNT_REGS_NUM;
    devMemInfoPtr->egrMem.egrMibCntReg =
        smemDevMemoryCalloc(devObjPtr, EGR_MIB_CNT_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->egrMem.egrMibCntReg == 0)
    {
        skernelFatalError("smemCht3AllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->egrMem.hprMibCntRegNum = EGR_HYP_MIB_REGS_NUM;
    for (i = 0; i < 4; i++)
    {
        devMemInfoPtr->egrMem.hprMibCntReg[i] =
            smemDevMemoryCalloc(devObjPtr, EGR_HYP_MIB_REGS_NUM, sizeof(SMEM_REGISTER));
        if (devMemInfoPtr->egrMem.hprMibCntReg[i] == 0)
        {
            skernelFatalError("smemCht3AllocSpecMemory: allocation error\n");
        }
    }

    devMemInfoPtr->egrMem.txqInternalRegNum = TXQ_INTERNAL_REGS_NUM;
    for (i = 0; i < 4; i++)
    {
        devMemInfoPtr->egrMem.txqInternalReg[i] =
            smemDevMemoryCalloc(devObjPtr, TXQ_INTERNAL_REGS_NUM, sizeof(SMEM_REGISTER));
        if (devMemInfoPtr->egrMem.txqInternalReg[i] == 0)
        {
            skernelFatalError("smemCht3AllocSpecMemory: allocation error\n");
        }
    }

    devMemInfoPtr->egrMem.tailDropRegNum = EGR_TALE_DROP_REGS_NUM;
    devMemInfoPtr->egrMem.tailDropReg =
            smemDevMemoryCalloc(devObjPtr, EGR_TALE_DROP_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->egrMem.tailDropReg == 0)
    {
        skernelFatalError("smemCht3AllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->egrMem.egrRateShpRegNum = EGR_RATE_SHAPES_REGS_NUM;
    for (i = 0; i < 9; i++)
    {
        devMemInfoPtr->egrMem.egrRateShpReg[i] =
            smemDevMemoryCalloc(devObjPtr, EGR_RATE_SHAPES_REGS_NUM, sizeof(SMEM_REGISTER));
        if (devMemInfoPtr->egrMem.egrRateShpReg[i] == 0)
        {
            skernelFatalError("smemCht3AllocSpecMemory: allocation error\n");
        }
    }

    devMemInfoPtr->egrMem.xsmiRegNum = EGR_XSMI_REGS_NUM;
    devMemInfoPtr->egrMem.xsmiReg =
            smemDevMemoryCalloc(devObjPtr, EGR_XSMI_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->egrMem.xsmiReg == 0)
    {
        skernelFatalError("smemCht3AllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->egrMem.egrStcTblRegNum = EGR_STC_REGS_NUM;
    devMemInfoPtr->egrMem.egrStcTblReg =
            smemDevMemoryCalloc(devObjPtr, EGR_STC_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->egrMem.egrStcTblReg == 0)
    {
        skernelFatalError("smemCht3AllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->egrMem.egrRateShpRegNum = EGR_RATE_SHAPES_REGS_NUM;
    for (i = 0; i < 9; i++)
    {
        devMemInfoPtr->egrMem.egrRateShpReg[i] =
            smemDevMemoryCalloc(devObjPtr, EGR_RATE_SHAPES_REGS_NUM, sizeof(SMEM_REGISTER));
        if (devMemInfoPtr->egrMem.egrRateShpReg[i] == 0)
        {
            skernelFatalError("smemCht3AllocSpecMemory: allocation error\n");
        }
    }

    devMemInfoPtr->egrMem.xsmiRegNum = EGR_XSMI_REGS_NUM;
    devMemInfoPtr->egrMem.xsmiReg =
            smemDevMemoryCalloc(devObjPtr, EGR_XSMI_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->egrMem.xsmiReg == 0)
    {
        skernelFatalError("smemChtAllocSpecMemory: allocation error\n");
    }

    /* Bridge Management registers memory object */
    devMemInfoPtr->brdgMngMem.bridgeGenRegsNum = BRDG_ETH_BRDG_REGS_NUM;
    devMemInfoPtr->brdgMngMem.bridgeGenReg =
            smemDevMemoryCalloc(devObjPtr, BRDG_ETH_BRDG_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->brdgMngMem.bridgeGenReg == 0)
    {
        skernelFatalError("smemCht3AllocSpecMemory: allocation error\n");
    }

    /* Buffer management registers memory object */
    devMemInfoPtr->bufMngMem.bmRegNum = BUF_MNG_REGS_NUM;
    devMemInfoPtr->bufMngMem.bmReg =
            smemDevMemoryCalloc(devObjPtr, BUF_MNG_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->bufMngMem.bmReg == 0)
    {
        skernelFatalError("smemCht3AllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->bufMngMem.bmCntrRegNum = BUF_MNG_LL_REGS_NUM;
    devMemInfoPtr->bufMngMem.bmCntrReg =
            smemDevMemoryCalloc(devObjPtr, BUF_MNG_LL_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->bufMngMem.bmCntrReg == 0)
    {
        skernelFatalError("smemCht3AllocSpecMemory: allocation error\n");
    }

    /* Ports groups configuration registers (port0 through port5),
       LEDs interfaces configuration registers and Master SMI
       interfaces registers                                         */

    devMemInfoPtr->gopCnfMem.portGroupRegNum = GOP_PORT_GROUP_REGS_NUM;
    devMemInfoPtr->gopCnfMem.xgPortGroupRegNum = GOP_XG_PORT_GROUP_REGS_NUM;
    devMemInfoPtr->gopCnfMem.macMibCountRegNum = MAC_PORT_GROUP_CNT_REGS_NUM;
    devMemInfoPtr->gopCnfMem.xgMacMibCountRegNum = MAC_XG_PORT_GROUP_CNT_REGS_NUM;
    for (i = 0; i < 4; i++)
    {
        devMemInfoPtr->gopCnfMem.portGroupReg[i] =
            smemDevMemoryCalloc(devObjPtr, devMemInfoPtr->gopCnfMem.portGroupRegNum, sizeof(SMEM_REGISTER));
        if (devMemInfoPtr->gopCnfMem.portGroupReg[i] == 0)
        {
            skernelFatalError("smemCht3AllocSpecMemory: allocation error\n");
        }

        devMemInfoPtr->gopCnfMem.xgPortGroupReg[i] =
            smemDevMemoryCalloc(devObjPtr, devMemInfoPtr->gopCnfMem.xgPortGroupRegNum, sizeof(SMEM_REGISTER));
        if (devMemInfoPtr->gopCnfMem.xgPortGroupReg[i] == 0)
        {
            skernelFatalError("smemCht3AllocSpecMemory: allocation error\n");
        }

        devMemInfoPtr->gopCnfMem.macMibCountReg[i] =
            smemDevMemoryCalloc(devObjPtr, devMemInfoPtr->gopCnfMem.macMibCountRegNum, sizeof(SMEM_REGISTER));
        if (devMemInfoPtr->gopCnfMem.portGroupReg[i] == 0)
        {
            skernelFatalError("smemCht3AllocSpecMemory: allocation error\n");
        }

        devMemInfoPtr->gopCnfMem.xgMacMibCountReg[i] =
            smemDevMemoryCalloc(devObjPtr, devMemInfoPtr->gopCnfMem.xgMacMibCountRegNum, sizeof(SMEM_REGISTER));
        if (devMemInfoPtr->gopCnfMem.xgMacMibCountReg[i] == 0)
        {
            skernelFatalError("smemCht3AllocSpecMemory: allocation error\n");
        }
    }

    /* Bridge registers and FDB */
    devMemInfoPtr->macFdbMem.fdbRegNum = MAC_FDB_REGS_NUM;
    devMemInfoPtr->macFdbMem.fdbReg =
                                smemDevMemoryCalloc(devObjPtr, MAC_FDB_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->macFdbMem.fdbReg == 0)
    {
        skernelFatalError("smemCht3AllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->macFdbMem.macTblRegNum = MAC_TBL_REGS_NUM(devObjPtr);
    devMemInfoPtr->macFdbMem.macTblReg =
            smemDevMemoryCalloc(devObjPtr, MAC_TBL_REGS_NUM(devObjPtr), sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->macFdbMem.macTblReg == 0)
    {
        skernelFatalError("smemCht3AllocSpecMemory: allocation error\n");
    }

    /* Buffers memory banks data register */
    devMemInfoPtr->banksMem.bankMemRegNum = BANK_MEM_REGS_NUM;
    for (i = 0; i < 2; i++)
    {
        devMemInfoPtr->banksMem.bankMemReg[i] =
            smemDevMemoryCalloc(devObjPtr, BANK_MEM_REGS_NUM, sizeof(SMEM_REGISTER));
        if (devMemInfoPtr->banksMem.bankMemReg[i] == 0)
        {
            skernelFatalError("smemCht3AllocSpecMemory: allocation error\n");
        }
    }


    /* Tri-Speed Ports MAC */
    devMemInfoPtr->triSpeedPortsMem.triSpeedPortsRegNum = TRI_SPEED_REGS_NUM;
    devMemInfoPtr->triSpeedPortsMem.triSpeedPortsReg =
            smemDevMemoryCalloc(devObjPtr, TRI_SPEED_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->triSpeedPortsMem.triSpeedPortsReg == 0)
    {
        skernelFatalError("smemCht3AllocSpecMemory: allocation error\n");
    }

    /* XGPort0 Registers */
    devMemInfoPtr->xgCtrlPortsMem.xgPortsRegNum = XG_PORT_REGS_NUM;
    devMemInfoPtr->xgCtrlPortsMem.xgPortsReg =
        smemDevMemoryCalloc(devObjPtr, XG_PORT_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->xgCtrlPortsMem.xgPortsReg == 0)
    {
        skernelFatalError("smemCht3AllocSpecMemory: allocation error\n");
    }

    /* XG MIB Counters */
    devMemInfoPtr->xgMibCntrsMem.mibRegNum = XG_MIB_CNTRS_REGS_NUM;
    devMemInfoPtr->xgMibCntrsMem.mibReg =
        smemDevMemoryCalloc(devObjPtr, devMemInfoPtr->xgMibCntrsMem.mibRegNum, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->xgMibCntrsMem.mibReg == 0)
    {
        skernelFatalError("smemCht3AllocSpecMemory: allocation error\n");
    }

        /* PCL registers and TCAM memory space   */
        devMemInfoPtr->pclMem.ingrPclCommonRegNum = PCL_COMMON_REGS_NUM;
        devMemInfoPtr->pclMem.ingrPclCommonReg =
            smemDevMemoryCalloc(devObjPtr, PCL_COMMON_REGS_NUM, sizeof(SMEM_REGISTER));
        if (devMemInfoPtr->pclMem.ingrPclCommonReg == 0)
        {
            skernelFatalError("smemCht3AllocSpecMemory: allocation error\n");
        }

        devMemInfoPtr->pclMem.pclTcamInternalConfigRegNum =
            PCL_TCAM_INTERNAL_CONFIG_REGS_NUM;
        devMemInfoPtr->pclMem.pclTcamInternalConfigReg =
            smemDevMemoryCalloc(devObjPtr, PCL_TCAM_INTERNAL_CONFIG_REGS_NUM, sizeof(SMEM_REGISTER));
        if (devMemInfoPtr->pclMem.pclTcamInternalConfigReg == 0)
        {
            skernelFatalError("smemCht3AllocSpecMemory: allocation error\n");
        }

        devMemInfoPtr->pclMem.pclTcamTestConfigRegNum =
            PCL_TCAM_TEST_CONFIG_REGS_NUM;
        devMemInfoPtr->pclMem.pclTcamTestConfigReg =
            smemDevMemoryCalloc(devObjPtr, PCL_TCAM_TEST_CONFIG_REGS_NUM, sizeof(SMEM_REGISTER));
        if (devMemInfoPtr->pclMem.pclTcamTestConfigReg == 0)
        {
            skernelFatalError("smemCht3AllocSpecMemory: allocation error\n");
        }

        devMemInfoPtr->pclMem.ingrPclConfigTableRegNum = PCL_CONFIG_REGS_NUM;
        devMemInfoPtr->pclMem.ingrPclConfigTableReg =
            smemDevMemoryCalloc(devObjPtr, PCL_CONFIG_REGS_NUM, sizeof(SMEM_REGISTER));
        if (devMemInfoPtr->pclMem.ingrPclConfigTableReg == 0)
        {
            skernelFatalError("smemCht3AllocSpecMemory: allocation error\n");
        }

        if(SKERNEL_DEVICE_FAMILY_CHEETAH_3_ONLY(devObjPtr))
        {
            devMemInfoPtr->pclMem.policyActionTableRegNum = PCL_ACTION_TBL_REGS_NUM(devObjPtr);
            devMemInfoPtr->pclMem.policyActionTableReg =
                smemDevMemoryCalloc(devObjPtr, PCL_ACTION_TBL_REGS_NUM(devObjPtr), sizeof(SMEM_REGISTER));
            if (devMemInfoPtr->pclMem.policyActionTableReg == 0)
            {
                skernelFatalError("smemCht3AllocSpecMemory: allocation error\n");
            }

            devMemInfoPtr->pclMem.policyEccRegNum = CH3_PCL_ECC_REGS_NUM;
            for (i = 0; i < 2; i++)
            {
                devMemInfoPtr->pclMem.policyEccReg[i] =
                    smemDevMemoryCalloc(devObjPtr, CH3_PCL_ECC_REGS_NUM, sizeof(SMEM_REGISTER));
                if (devMemInfoPtr->pclMem.policyEccReg[i] == 0)
                {
                    skernelFatalError("smemCht3AllocSpecMemory: allocation error\n");
                }
            }

            devMemInfoPtr->pclMem.policyTcamRegNum = CH3_PCL_TCAM_REGS_NUM;
            devMemInfoPtr->pclMem.policyTcamReg =
                smemDevMemoryCalloc(devObjPtr, CH3_PCL_TCAM_REGS_NUM, sizeof(SMEM_REGISTER));
            if (devMemInfoPtr->pclMem.policyTcamReg == 0)
            {
                skernelFatalError("smemCht3AllocSpecMemory: allocation error\n");
            }
        }
        devMemInfoPtr->pclMem.vlanTranslationTableRegNum = PCL_VLAN_TRANS_REGS_NUM;
        devMemInfoPtr->pclMem.vlanTranslationTableReg =
            smemDevMemoryCalloc(devObjPtr, PCL_VLAN_TRANS_REGS_NUM, sizeof(SMEM_REGISTER));
        if (devMemInfoPtr->pclMem.vlanTranslationTableReg == 0)
        {
            skernelFatalError("smemCht3AllocSpecMemory: allocation error\n");
        }

        devMemInfoPtr->pclMem.portVlanQoSConfigRegNum = PCL_PORT_VLAN_QOS_REGS_NUM;
        devMemInfoPtr->pclMem.portVlanQoSConfigReg =
            smemDevMemoryCalloc(devObjPtr, PCL_PORT_VLAN_QOS_REGS_NUM, sizeof(SMEM_REGISTER));
        if (devMemInfoPtr->pclMem.portVlanQoSConfigReg == 0)
        {
            skernelFatalError("smemCht3AllocSpecMemory: allocation error\n");
        }

        devMemInfoPtr->pclMem.portProtVidQoSConfigRegNum = PCL_PROT_BASED_VLAN_QOS_REGS_NUM;
        devMemInfoPtr->pclMem.portProtVidQoSConfigReg =
            smemDevMemoryCalloc(devObjPtr, PCL_PROT_BASED_VLAN_QOS_REGS_NUM, sizeof(SMEM_REGISTER));
        if (devMemInfoPtr->pclMem.portProtVidQoSConfigReg == 0)
        {
            skernelFatalError("smemCht3AllocSpecMemory: allocation error\n");
        }

    if(SKERNEL_IS_XCAT_REVISON_A1_DEV(devObjPtr) == GT_FALSE)
    {
        /* VLAN configuration Table */
        devMemInfoPtr->vlanTblMem.vlanCnfRegNum = VTL_VLAN_CONF_REGS_NUM;
        devMemInfoPtr->vlanTblMem.vlanCnfReg =
                smemDevMemoryCalloc(devObjPtr, VTL_VLAN_CONF_REGS_NUM, sizeof(SMEM_REGISTER));
        if (devMemInfoPtr->vlanTblMem.vlanCnfReg == 0)
        {
            skernelFatalError("smemCht3AllocSpecMemory: allocation error\n");
        }

        /* VLAN Table, Multicast Groups Table, and Span State Group Table */
        devMemInfoPtr->vlanTblMem.vlanTblRegNum = VTL_VLAN_TBL_REGS_NUM;
        devMemInfoPtr->vlanTblMem.vlanTblReg =
                smemDevMemoryCalloc(devObjPtr, VTL_VLAN_TBL_REGS_NUM, sizeof(SMEM_REGISTER));
        if (devMemInfoPtr->vlanTblMem.vlanTblReg == 0)
        {
            skernelFatalError("smemCht3AllocSpecMemory: allocation error\n");
        }

        devMemInfoPtr->vlanTblMem.mcstTblRegNum = VTL_MCST_TBL_REGS_NUM;
        devMemInfoPtr->vlanTblMem.mcstTblReg =
                smemDevMemoryCalloc(devObjPtr, VTL_MCST_TBL_REGS_NUM, sizeof(SMEM_REGISTER));
        if (devMemInfoPtr->vlanTblMem.mcstTblReg == 0)
        {
            skernelFatalError("smemCht3AllocSpecMemory: allocation error\n");
        }

        devMemInfoPtr->vlanTblMem.spanTblRegNum = VTL_STP_TBL_REGS_NUM;
        devMemInfoPtr->vlanTblMem.spanTblReg =
                smemDevMemoryCalloc(devObjPtr, VTL_STP_TBL_REGS_NUM/*5000*/, sizeof(SMEM_REGISTER));
        if (devMemInfoPtr->vlanTblMem.spanTblReg == 0)
        {
            skernelFatalError("smemCht3AllocSpecMemory: allocation error\n");
        }

        devMemInfoPtr->vlanTblMem.vrfTblRegNum = VTL_VRF_TBL_REGS_NUM;
        devMemInfoPtr->vlanTblMem.vrfTblReg =
                smemDevMemoryCalloc(devObjPtr, VTL_MCST_TBL_REGS_NUM, sizeof(SMEM_REGISTER));
        if (devMemInfoPtr->vlanTblMem.vrfTblReg == 0)
        {
            skernelFatalError("smemCht3AllocSpecMemory: allocation error\n");
        }

    /* Policer registers and meters memory space */
    devMemInfoPtr->policerMem.policerRegNum = POLICER_REGS_NUM;
    devMemInfoPtr->policerMem.policerReg =
            smemDevMemoryCalloc(devObjPtr, POLICER_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->policerMem.policerReg == 0)
    {
        skernelFatalError("smemCht3AllocSpecMemory: allocation error\n");
    }

    /* Policer timer memory space */
    devMemInfoPtr->policerMem.policerTimerRegNum = POLICER_REGS_NUM;
    devMemInfoPtr->policerMem.policerTimerReg =
            smemDevMemoryCalloc(devObjPtr, POLICER_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->policerMem.policerTimerReg == 0)
    {
        skernelFatalError("smemCht3AllocSpecMemory: allocation error\n");
    }

    /* Policer descriptor sample memory space */
    devMemInfoPtr->policerMem.policerDescrSampleRegNum = POLICER_DESCR_SAMP_REGS_NUM;
    devMemInfoPtr->policerMem.policerDescrSampleReg =
            smemDevMemoryCalloc(devObjPtr, POLICER_DESCR_SAMP_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->policerMem.policerDescrSampleReg == 0)
    {
        skernelFatalError("smemCht3AllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->policerMem.policerTblRegNum = POLICER_TBL_REGS_NUM;
    devMemInfoPtr->policerMem.policerTblReg =
            smemDevMemoryCalloc(devObjPtr, POLICER_TBL_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->policerMem.policerTblReg == 0)
    {
        skernelFatalError("smemCht3AllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->policerMem.policerQosRmTblRegNum = POLICER_QOS_REM_TBL_REGS_NUM;
    devMemInfoPtr->policerMem.policerQosRmTblReg =
                smemDevMemoryCalloc(devObjPtr, POLICER_QOS_REM_TBL_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->policerMem.policerQosRmTblReg == 0)
    {
        skernelFatalError("smemCht3AllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->policerMem.policerCntTblRegNum = POLICER_COUNT_TBL_REGS_NUM;
    devMemInfoPtr->policerMem.policerCntTblReg =
                smemDevMemoryCalloc(devObjPtr, POLICER_COUNT_TBL_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->policerMem.policerCntTblReg == 0)
    {
        skernelFatalError("smemCht3AllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->policerMem.policerMngCntTblRegNum = POLICER_MNG_COUNT_TBL_REGS_NUM;
    devMemInfoPtr->policerMem.policerMngCntTblReg =
                smemDevMemoryCalloc(devObjPtr, POLICER_MNG_COUNT_TBL_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->policerMem.policerMngCntTblReg == 0)
    {
        skernelFatalError("smemCht3AllocSpecMemory: allocation error\n");
    }
    }

    /* PCI express registers */
    devMemInfoPtr->pciMem.pciRegNum = PCI_REGS_NUM;
    for (i = 0; i < 4; i++)
    {
        devMemInfoPtr->pciMem.pciReg[i]= smemDevMemoryCalloc(devObjPtr, PCI_REGS_NUM, sizeof(SMEM_REGISTER));
        if (devMemInfoPtr->pciMem.pciReg[i] == 0)
        {
            skernelFatalError("smemCht3AllocSpecMemory: allocation error\n");
        }
    }

    /*  AGE Tables and Registers */
    devMemInfoPtr->routerMem.ageRegNum  = ROUTER_AGE_REGS_NUM(devObjPtr);
    devMemInfoPtr->routerMem.ageReg =
                            smemDevMemoryCalloc(devObjPtr, ROUTER_AGE_REGS_NUM(devObjPtr), sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->routerMem.ageReg == 0)
    {
        skernelFatalError("smemCht3AllocSpecMemory: allocation error\n");
    }
    /*  Router Tables and Registers */
    devMemInfoPtr->routerMem.configRegNum  = ROUTER_CONF_REGS_NUM;
    devMemInfoPtr->routerMem.configReg =
                            smemDevMemoryCalloc(devObjPtr, ROUTER_CONF_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->routerMem.configReg == 0)
    {
        skernelFatalError("smemCht3AllocSpecMemory: allocation error\n");
    }

    if(SKERNEL_DEVICE_FAMILY_CHEETAH_3_ONLY(devObjPtr))
    {
        devMemInfoPtr->routerMem.tcamTestRegNum  = CH3_ROUTER_TCAM_TEST_REGS_NUM;
        devMemInfoPtr->routerMem.tcamTestReg =
                        smemDevMemoryCalloc(devObjPtr, CH3_ROUTER_TCAM_TEST_REGS_NUM, sizeof(SMEM_REGISTER));
        if (devMemInfoPtr->routerMem.tcamTestReg == 0)
        {
            skernelFatalError("smemCht3AllocSpecMemory: allocation error\n");
        }
        devMemInfoPtr->routerMem.tcamRegNum  = CH3_ROUTER_TCAM_REGS_NUM;
        devMemInfoPtr->routerMem.tcamReg =
                        smemDevMemoryCalloc(devObjPtr, CH3_ROUTER_TCAM_REGS_NUM, sizeof(SMEM_REGISTER));
        if (devMemInfoPtr->routerMem.tcamReg == 0)
        {
            skernelFatalError("smemCht3AllocSpecMemory: allocation error\n");
        }
        devMemInfoPtr->routerMem.actionTblRegNum  = CH3_LTT_ACTION_TBL_REGS_NUM;
        devMemInfoPtr->routerMem.actionTblReg =
                            smemDevMemoryCalloc(devObjPtr, CH3_LTT_ACTION_TBL_REGS_NUM, sizeof(SMEM_REGISTER));
        if (devMemInfoPtr->routerMem.actionTblReg == 0)
        {
            skernelFatalError("smemCht3AllocSpecMemory: allocation error\n");
        }
    }
    devMemInfoPtr->routerMem.nextHopTblRegNum   = NEXT_HOP_TBL_REGS_NUM(devObjPtr);
    devMemInfoPtr->routerMem.nextHopTblReg =
        smemDevMemoryCalloc(devObjPtr, NEXT_HOP_TBL_REGS_NUM(devObjPtr), sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->routerMem.nextHopTblReg == 0)
    {
        skernelFatalError("smemCht3AllocSpecMemory: allocation error\n");
    }
    devMemInfoPtr->routerMem.vrfTblRegNum   = VRF_TBL_REGS_NUM;
    devMemInfoPtr->routerMem.vrfTblReg =
        smemDevMemoryCalloc(devObjPtr, VRF_TBL_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->routerMem.vrfTblReg == 0)
    {
        skernelFatalError("smemCht3AllocSpecMemory: allocation error\n");
    }

    if(SKERNEL_IS_XCAT_REVISON_A1_DEV(devObjPtr) == GT_FALSE)
    {
        /* MLL Engine Configuration Registers */
        devMemInfoPtr->mllMem.mllConfRegNum = MLL_GLOB_REGS_NUM;
        devMemInfoPtr->mllMem.mllConfReg =
                            smemDevMemoryCalloc(devObjPtr, MLL_GLOB_REGS_NUM, sizeof(SMEM_REGISTER));
        if (devMemInfoPtr->mllMem.mllConfReg == 0)
        {
            skernelFatalError("smemCht3AllocSpecMemory: allocation error\n");
        }
        devMemInfoPtr->mllMem.mllEntryRegNum = MLL_TBL_REGS_NUM(devObjPtr);
        devMemInfoPtr->mllMem.mllEntryReg =
                            smemDevMemoryCalloc(devObjPtr, MLL_TBL_REGS_NUM(devObjPtr), sizeof(SMEM_REGISTER));
        if (devMemInfoPtr->mllMem.mllEntryReg == 0)
        {
            skernelFatalError("smemCht3AllocSpecMemory: allocation error\n");
        }
    }

    devMemInfoPtr->centralCntMem.cncCntrsRegNum = CENTRAL_CNT_GLB_REGS_NUM;
    devMemInfoPtr->centralCntMem.cncCntrsReg =
                        smemDevMemoryCalloc(devObjPtr, CENTRAL_CNT_GLB_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->centralCntMem.cncCntrsReg == 0)
    {
        skernelFatalError("smemCht3AllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->centralCntMem.cncCntrsPerBlockRegNum = CENTRAL_CNT_GLB_REGS_NUM;
    devMemInfoPtr->centralCntMem.cncCntrsPerBlockReg =
                        smemDevMemoryCalloc(devObjPtr, CENTRAL_CNT_GLB_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->centralCntMem.cncCntrsPerBlockReg == 0)
    {
        skernelFatalError("smemCht3AllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->centralCntMem.cncCntrsTblNum = CENTRAL_CNT_TBL_SIZE;
    devMemInfoPtr->centralCntMem.cncCntrsTbl =
        smemDevMemoryCalloc(devObjPtr, CNC_CNT_BLOCKS(devObjPtr),sizeof(SMEM_REGISTER*));
    if (devMemInfoPtr->centralCntMem.cncCntrsTbl == 0)
    {
        skernelFatalError("smemCht3AllocSpecMemory: allocation error\n");
    }

    for (i = 0; i < CNC_CNT_BLOCKS(devObjPtr); i++)
    {
        devMemInfoPtr->centralCntMem.cncCntrsTbl[i] =
                            smemDevMemoryCalloc(devObjPtr, CENTRAL_CNT_TBL_SIZE, sizeof(SMEM_REGISTER));
        if (devMemInfoPtr->centralCntMem.cncCntrsTbl[i] == 0)
        {
            skernelFatalError("smemCht3AllocSpecMemory: allocation error\n");
        }
    }

    if(SKERNEL_IS_CHEETAH3P_ONLY_DEV(devObjPtr))
    {
        /* CH3p */
        devMemInfoPtr->ch3pExtraMem.extraRegNum = CH3P_EXTRA_MEM_REGS_NUM;
        devMemInfoPtr->ch3pExtraMem.extraRegPtr =
                smemDevMemoryCalloc(devObjPtr, CH3P_EXTRA_MEM_REGS_NUM, sizeof(SMEM_REGISTER));
        if (devMemInfoPtr->ch3pExtraMem.extraRegPtr == NULL)
        {
            skernelFatalError("smemChtAllocSpecMemory: allocation error\n");
        }
    }

    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x0b000000 ,0x0b000100-4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0b100000 ,128  )}/*Statistical Rate Limits Table*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0b200000 ,1024 )}/*CPU Code Table*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0b300000 ,512  )}/*QoSProfile to QoS Table */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0b400000 ,4096 )}/*Trunks Members Table*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0b500000 ,124  )}/*TO CPU Rate LimiterConfiguration*/

/* defined at next chunk  -->  ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0b600000 ,2048 )} Virtual Port Mapping Table*/

            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x0b020000 ,0x0b02000c  )} /*Ingress Forwarding Restrictions*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0b040000 ,448  )} /*Ingress STC Table*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x0b050000 ,0x0b050008  )} /*Pre-Egress Engine Global Configuration*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0b080000 ,32*4  )} /*TO CPU Rate Limiter<%n> Packet Counter*/

            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x0b005000 ,0x0b005008)}/*Ingress STC Table Word0 Access*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x0b007000 ,0x0b007080-4)} /*TCP/UDP Dest Port Range CPU Code Entry*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x0b008000 ,0x0b008010  )} /*IP Protocol CPU Code */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0b009000 ,16*4  )} /*Number of Trunk Members Table Entry%e */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x0b001000 ,0x0b001000)}/*Trunk Table Data Access */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x0b002000 ,0x0b002000)}/*QoSProfile to QoS Table Data Access*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x0b003000 ,0x0b003000)}/*CPU Code Table Data Access*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x0b004000 ,0x0b004000)}/*Statistical Rate Limits Table Data Access*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x0b00a000 ,0x0b00a000)}/*Virtual Port Mapping Table Data Access */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x0b00b050 ,0x0b00b05c)}/*Pre-Egress Engine OutGoing Desc*/

        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,&devMemInfoPtr->eqUnitMem);
    }


    if (SKERNEL_DEVICE_FAMILY_XCAT_ONLY(devObjPtr) && devObjPtr->deviceRevisionId >= 4)
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             /*Virtual Port Mapping Table Data Access2 */
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x0b00a004 ,0x0b00a004)}
             /*Virtual Port Mapping Table*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0b600000 , 16384  ),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(45,8),SMEM_BIND_TABLE_MAC(logicalTrgMappingTable)}
             /*VLAN mapping table*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0B700000 , 16384  ),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(12,4),SMEM_BIND_TABLE_MAC(eqVlanMapping)}
            /*LP Egress VLAN member table Entry*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0B704000 , 16384  ),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(64,8),SMEM_BIND_TABLE_MAC(eqLogicalPortEgressVlanMember)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

        smemInitMemChunk(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,&devMemInfoPtr->eqUnitMem,&tmpUnitChunk);
    }
    else
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {                                                                 /*4*(64*8)*/
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0b600000 ,2048 )} /*Virtual Port Mapping Table*/
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

        smemInitMemChunk(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,&devMemInfoPtr->eqUnitMem,&tmpUnitChunk);
    }

    if(SKERNEL_IS_CHEETAH3_ONLY_DEV(devObjPtr))
    {
        {
            SMEM_CHUNK_BASIC_STC  chunksMem[]=
            {
                 {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x07800000 ,0x07800000)}/*Buffer Memory Write Access Register*/
                /* HA top */
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x07800004 ,0x07800420)}/*HA config registers*/
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x07E80000 ,131072 )   }/*Router ARP DA and Tunnel Start Table*/
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x07F80000 ,16640  )   }/*Port/VLAN MAC SA Table*/
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x07FC0000 ,16384  )   }/*VLAN Translation Table*/

                /* EPCL top */
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC(0x07C00000,0x07C00140+8*4)}/*EPCL config registers*/
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x07F00000,16640 )  }/*Egress PCL Configuration Table*/

                /* Buffer memory */
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x07E00000,264*2 )  }/*Prefetch Descriptors*/

            };
            GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

            smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,&devMemInfoPtr->haAndEpclUnitMem);
        }
    }
    else
    {
        SMEM_UNIT_CHUNKS_STC * unitChunkPtr;
        {
            SMEM_CHUNK_BASIC_STC  chunksMem[]=
            {
                 {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0D000100 ,0x0D000170)} /*actionTableAndPolicyTCAMAccessCtrl*/
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0D000208 ,0x0D000238)} /*Policy_TCAM_Configuration         */
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0D040000 ,196608)   } /*policy_TCAM                       */
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0D0B0000 ,49152 )   } /*policy_Action_Table               */
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0D0C0000 ,12288)    } /*policy_ECC_X                      */
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0D0D0000 ,12288)    } /*policy_ECC_Y                      */
            };
            GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

            smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,&devMemInfoPtr->ipclTccUnitMem);
        }
        if(devObjPtr->isXcatA0Features)
        {
            /* the register of unit 1..5 , 32..47 */
            GT_U32  serdessIdArray[]=
                {0,1,2,3,4,5,
                 32,33,34,35,36,37,38,39,
                 40,41,42,43,44,45,46,47};

            #define XCAT_SERDESS_NUM (sizeof(serdessIdArray)/sizeof(serdessIdArray[0]))

            SMEM_CHUNK_BASIC_STC  chunksMem[2*XCAT_SERDESS_NUM]=
            {
                /* the register of unit 0*/
                 {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC(0x09800004,0x098000d8)}
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC(0x09808000,0x09808000)}
                /* the register of unit 1..5 , 32..47 */
            };
            GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

            GT_U32  ii;

            for(ii = 1;ii < XCAT_SERDESS_NUM;ii++)
            {
                chunksMem[ii*2].memFirstAddr    = 0x400*serdessIdArray[ii] + chunksMem[0].memFirstAddr;
                chunksMem[ii*2].numOfRegisters  = chunksMem[0].numOfRegisters;

                chunksMem[ii*2+1].memFirstAddr    = 0x400*serdessIdArray[ii] + chunksMem[1].memFirstAddr;
                chunksMem[ii*2+1].numOfRegisters  = chunksMem[1].numOfRegisters;
            }

            smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,&devMemInfoPtr->uniphySerdesUnitMem);
        }
        /* Allocate unique memory for xCat revision A1 and above */
        if(!devObjPtr->isXcatA0Features)
        {
            {
                #define XCAT_SERDESS_A1_NUM    (11)
                SMEM_CHUNK_BASIC_STC  chunksMem[4 * XCAT_SERDESS_A1_NUM]=
                {
                     {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC(0x09800000 ,0x09800008)}   /* Serdes External Configuration 0 */
                    ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x09800200 ,512)}         /* Serdes Internal Register File 0 */
                    ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC(0x09800400 ,0x09800408)}   /* Serdes External Configuration 1 */
                    ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x09800600 ,512)}         /* Serdes Internal Register File 1 */
                };
                GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

                GT_U32 ii;
                SMEM_UNIT_CHUNKS_STC tmpUnitChunk;

                for(ii = 1; ii < XCAT_SERDESS_A1_NUM; ii++)
                {
                    chunksMem[ii*4].memFirstAddr      = 0x1000*ii + chunksMem[0].memFirstAddr;
                    chunksMem[ii*4].numOfRegisters    = chunksMem[0].numOfRegisters;

                    chunksMem[ii*4+1].memFirstAddr    = 0x1000*ii + chunksMem[1].memFirstAddr;
                    chunksMem[ii*4+1].numOfRegisters  = chunksMem[1].numOfRegisters;

                    chunksMem[ii*4+2].memFirstAddr    = 0x1000*ii + chunksMem[2].memFirstAddr;
                    chunksMem[ii*4+2].numOfRegisters  = chunksMem[2].numOfRegisters;

                    chunksMem[ii*4+3].memFirstAddr    = 0x1000*ii + chunksMem[3].memFirstAddr;
                    chunksMem[ii*4+3].numOfRegisters  = chunksMem[3].numOfRegisters;
                }

                smemInitMemChunk(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

                /*add the tmp unit chunks to the main unit */
                smemInitMemCombineUnitChunks(devObjPtr,&devMemInfoPtr->uniphySerdesUnitMem,
                                             &tmpUnitChunk);
            }
            {
                SMEM_CHUNK_BASIC_STC  chunksMem[]=
                {
                     {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0a000000 ,0x0a000000)}  /* Global Configuration */
                    ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0a000100 ,0x0a000118)}  /* VLT Tables Access Control */
                    ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0a080000 ,4096)      }  /* Spanning Tree State */
                    ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0a100000 ,65536)     }  /* Multicast Group */
                    ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0a180000 ,65536)     }  /* VRF-ID */
                    ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0a200000 ,131072)    }  /* VLAN */
                    ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC(0x0a400000 ,0x0a40003c) }  /* BCN control */
                    ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC(0x0a400100 ,0x0a4001fc) }  /* BCN Profiles, Global Buffers Counter */
                    ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC(0x0a401000 ,0x0a401020) }  /* BCN Ports Trigger Counters Configuration */
                    ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0a400200 ,112) }        /* Ports buffers counters */

                };
                GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

                smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,&devMemInfoPtr->xCatExtraMem.vlanTblUnitMem);
            }
            {
                SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
                {
                     {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02000800 ,0)}, FORMULA_TWO_PARAMETERS(4, 0x8, 16 , 0x1000)} /* IEEE Reserved Multicast Configuration0/1/2/3 <%n> */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02000804 ,0)}, FORMULA_SINGLE_PARAMETER(16 , 0x1000)} /* IEEE Reserved Multicast CPU Index <%n> */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0200080c ,0)}, FORMULA_SINGLE_PARAMETER(2 , 0x10)} /* IEEE table select0/1 */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02010800, 0)}, FORMULA_SINGLE_PARAMETER(3 , 0x1000)} /* IPv6 ICMP Message Type Configuration + IPv6 ICMP Command */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02020800, 0)}, FORMULA_THREE_PARAMETERS(3, 0x10000, 2, 0x8000, 8, 0x1000)} /* IPv4/IPv6 Multicast Link-Local Configuration  */
                };                                                                                                                                   /* IPv4/IPv6 Multicast Link-Local CPU Code Index  */

                GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);

                smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &devMemInfoPtr->xCatExtraMem.brdgMngUnitMem);
            }
            {
                #define QOS_PROFILE_TO_EXP_DIM      {16, 0x4}, SMEM_FORMULA_END_CNS
                #define EGRESS_TCP_UDP_PORT_RANGE_COMPARATOR_DIM {8, 0x4}, SMEM_FORMULA_END_CNS
                #define ERSPAN_DIM {8, 0x4}, {6, 0x18}, SMEM_FORMULA_END_CNS

                SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
                {
                    /* HA top */
                     {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0f000004 ,0x0f000060)}}/* HA config registers */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0f000100 ,0x0f000130)}}/* HA config registers - continue */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0F000300, 0)}, {QOS_PROFILE_TO_EXP_DIM}}     /* QoSProfile to EXP Registers */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0F000340, 0)}, FORMULA_SINGLE_PARAMETER(8 , 0x4)}     /* QoSProfile to DP Registers */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0f0003e0, 0)}, FORMULA_SINGLE_PARAMETER(5 , 0x4)}     /* TxQ to HA descriptor */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0f000400 ,0x0f000484)}}/* HA config registers - continue */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0f000490 ,0x0f00049c)}}/* Ingress ERSPAN<%n> Register */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0f0004b0, 0x0f0004bc)}}/* Egress ERSPAN<%n> Register */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0xf0004d0, 0)}, {ERSPAN_DIM}}     /* ERSPAN */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0f018000 ,2048   )   }}/*DFX Memory*/
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0f040000 ,32768  )   }}/*Router ARP DA and Tunnel Start Table*/
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0f008000 ,16640  )  }}/*Port/VLAN MAC SA Table*/
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0f010000 ,16384  )   }}/*VLAN Translation Table*/

                    /* EPCL top */
                   /* EPCL top */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC(0x0f800000, 0x0f800040)}}/*EPCL config registers*/
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC(0x0f800200, 0x0f800200)}}/*Egress Policer Global Configuration*/
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0f800100, 0)}, {EGRESS_TCP_UDP_PORT_RANGE_COMPARATOR_DIM}}   /* Egress TCP Port Range Comparator Configuration */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0f800140, 0)}, {EGRESS_TCP_UDP_PORT_RANGE_COMPARATOR_DIM}}   /* Egress UDP Port Range Comparator Configuration */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0f808000, 16640 ) }}/*Egress PCL Configuration Table*/
                };
                GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);

                smemInitMemChunkExt(devObjPtr,chunksMem,numOfChunks,&devMemInfoPtr->xCatExtraMem.haAndEpclUnitMem);
            }

            if (devObjPtr->deviceRevisionId >= 4)
            {
                SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
                {
                    /*TS TP-ID Configuration Register<%n> ,
                      Port TS Tag TP-ID Select<%n>*/
                     {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0f000700, 0x0f00071C )}}
                    /*Passenger TPID Index Source*/
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0f000730, 0x0f000730 )}}
                    /*Passenger TPID0_TPID1<%n>*/
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0f000750, 0x0f00075c )}}
                    /*MPLS Control Word<%n>*/
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0f000764, 0x0f00077C )}}
                    /*FROM_CPU constants to fanout devices*/
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0f000780, 0x0f000780 )}}
                    /*Fanout Device Port*/
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0f000790, 0x0f000790 )}}

                };
                GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
                SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

                smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

                /*add the tmp unit chunks to the main unit */
                smemInitMemCombineUnitChunks(devObjPtr,&devMemInfoPtr->xCatExtraMem.haAndEpclUnitMem,&tmpUnitChunk);
            }

            {
                #define PORT_METERING_POINTER_INDEX_DIM     {32, 0x4}, SMEM_FORMULA_END_CNS

                SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
                {
                     {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0c800000, 0x0c8000e0)}}  /* Policer configuration registers */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0c800100, 0x0c80010c)}}  /* Policer interrupt and Policer Shadow */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0c800200, 36)    }}      /* Policer Timer Memory */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0c800400, 96)    }}      /* Policer Descriptor Sample Memory */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0c800500, 192), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(74, 16)   }}      /* Policer Management Counters Memory */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0c800800, 256)   }}      /* IPfix wrap around alert Memory */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0c800900, 256)   }}      /* IPfix aging alert Memory */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0c801000, 2048)  }}      /* DFX table */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0c801800, 0)}, {PORT_METERING_POINTER_INDEX_DIM}}    /* Port metering pointer index */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0c840000, 131072), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(229, 32)}}      /* Policer Metering Memory */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0c860000, 131072), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(229, 32)}}      /* Policer Counting Memory */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0c880000, 2560), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(14, 8)  }}      /* Ingress Policer Re-Marking Memory */

                };
                GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);

                smemInitMemChunkExt(devObjPtr,chunksMem,numOfChunks,&devMemInfoPtr->xCatExtraMem.policerMemIngress0UnitMem);
            }
            {
                SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
                {
                     {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x03800000, 0x03800094)}}  /* Policer configuration registers */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x038000a0, 0x038000e0)}}  /* Policer configuration registers - continue */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x03800100, 0x0380010C)}}  /* Policer configuration registers - continue */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x03800200, 36)    }}      /* Policer Timer Memory */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x03800400, 96)    }}      /* Policer Descriptor Sample Memory */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x03800500, 192), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(74, 16)   }}      /* Policer Management Counters Memory */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x03800800, 256)   }}      /* IPfix wrap around alert Memory */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x03800900, 256)   }}      /* IPfix aging alert Memory */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x03801000, 2048)  }}      /* DFX table */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x03801800, 0)}, {PORT_METERING_POINTER_INDEX_DIM}}    /* Port metering pointer index */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x03840000, 131072), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(229, 32)}}      /* Policer Metering and Counting Memory */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x03860000, 131072), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(229, 32)}}      /* Policer Metering and Counting Memory */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x03880000, 2560), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(51, 8)  }}      /* Egress Policer Re-Marking Memory */

                };
                GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);

                smemInitMemChunkExt(devObjPtr,chunksMem,numOfChunks,&devMemInfoPtr->xCatExtraMem.policerMemEgressUnitMem);
            }
            {
                SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
                {
                     {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0d000000, 0x0d0000e0)}}  /* Policer configuration registers */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0d000100, 0x0d00010c)}}  /* Policer interrupt and Policer Shadow */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0d000200, 36)    }}      /* Policer Timer Memory */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0d000400, 96)    }}      /* Policer Descriptor Sample Memory */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0d000500, 192), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(74, 16)   }}      /* Policer Management Counters Memory */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0d000800, 256)   }}      /* IPfix wrap around alert Memory */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0d000900, 256)   }}      /* IPfix aging alert Memory */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0d001000, 2048)  }}      /* DFX table */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0d001800, 0)}, {PORT_METERING_POINTER_INDEX_DIM}}    /* Port metering pointer index */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0d040000, 131072), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(229, 32)}}      /* Policer Metering Memory */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0d060000, 131072), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(229, 32)}}      /* Policer Counting Memory */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0d080000, 2560), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(14, 8)  }}      /* Ingress Policer Re-Marking Memory */

                };
                GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);

                smemInitMemChunkExt(devObjPtr,chunksMem,numOfChunks,&devMemInfoPtr->xCatExtraMem.policerMemIngress1UnitMem);
            }
            {
                #define MLL_OUT_DIM     {2, 0x100}, SMEM_FORMULA_END_CNS
                #define QOS_PROFILE_TO_MULTI_TARGET_TC_QUEUES_DIM       {8, 0x4}, SMEM_FORMULA_END_CNS

                SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
                {
                     {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0d800000, 0x0d800004)}} /* MLL Global Control and  Multi Target TC Queues Global Configuration */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0d800014, 0x0d800014)}} /* DFT1 */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0d80000C, 0x0d80000C)}} /* MLL Metal Fix */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0d800200, 0x0d800214)}} /* Multi-target TC Queues Strict Priority Enable Configuration, TC Queues Weight Configuration */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0d800450, 0x0d800450)}} /* Table Access Control */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0d800900, 0)}, {MLL_OUT_DIM}}    /* MLL Out Multicast Packets Counter */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0d800980, 0)}, {MLL_OUT_DIM}}    /* MLL Out Multicast Packets Counter */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0d800984, 0x0d800984)}} /* MLL MC FIFO Drop Counter */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0d800100, 0)}, {QOS_PROFILE_TO_MULTI_TARGET_TC_QUEUES_DIM}}      /* QoS Profile To Multi Target TC Queues Register  */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0d880000, 32768) }}       /* MLL Entry */
                };
                GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);

                smemInitMemChunkExt(devObjPtr,chunksMem,numOfChunks,&devMemInfoPtr->xCatExtraMem.mllMemUnitMem);
            }

            if (devObjPtr->deviceRevisionId >= 4)
            {
                SMEM_CHUNK_BASIC_STC  chunksMem[]=
                {

                    /* VPLS drop counter*/
                     {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x0d800018 ,0x0d800018)}
                    /*L2 MLL VIDX enable access control+data*/
                    ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x0D800460 ,0x0D800464)}
                    /*L2 MLL pointer map access control+data*/
                    ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x0D800470 ,0x0D800474)}
                    /*L2 MLL VIDX Enable*/
                    ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0d808000 , 512    ),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32,4),SMEM_BIND_TABLE_MAC(l2MllVidxToMllMapping)}
                    /*Virtual Port to MLL mapping table*/
                    ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0d809000 , 4096   ),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(12,4),SMEM_BIND_TABLE_MAC(l2MllVirtualPortToMllMapping)}
                };
                GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);
                SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

                smemInitMemChunk(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

                /*add the tmp unit chunks to the main unit */
                smemInitMemCombineUnitChunks(devObjPtr,&devMemInfoPtr->xCatExtraMem.mllMemUnitMem,&tmpUnitChunk);
            }

            {
                SMEM_CHUNK_BASIC_STC  chunksMem[]=
                {
                     {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0b800200, 112)       }      /* PCL to Bridge descriptor */
                    ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0b800800, 2048)      }      /* PCL DFX Area */
                    ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0b810000, 33792)     }      /* IPCL0 Configuration Table */
                    ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0b820000, 33792)     }      /* IPCL1 Configuration Table */
                    ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0b830000, 33280)     }      /* Internal InLIF Memory */
                };
                GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

                smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,&devMemInfoPtr->xCatExtraMem.pclMemUnitMem);
            }
            {
                SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
                {
                     {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0e000084 ,0)}, FORMULA_TWO_PARAMETERS(2, 0x4, 3, 0x8)} /* IPCL%p Key Type 2 Lookup ID Register 0/1 */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0e00009c, 0x0e00009c)}}      /* Active Power Save */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0e000100, 0x0e000138)}}      /* Action Table and Policy TCAM Access Control */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0e000148, 0x0e000170)}}      /* Policy TCAM Control */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0e000198, 0x0e0001a8)}}      /* TCAM Error Counter and TCC Interrupt Cause/Mask */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0e000208, 0x0e000238)}}      /* Policy TCAM Configuration */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0e002080, 128)       }}      /* TCAM Array Compare Enable */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0e040000, 196608)    }}      /* Policy TCAM */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0e0b0000, 49152)     }}      /* Policy Action Table */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0e0c0000, 12288)     }}      /* Policy ECC X */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0e0d0000, 12288)     }}      /* Policy ECC Y */
                };
                GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);

                smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &devMemInfoPtr->xCatExtraMem.pclTccMemUnitMem);
            }
            {
                SMEM_CHUNK_BASIC_STC  chunksMem[]=
                {
                     {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0f800000, 0x0f800040)} /* Egress Policy Configuration Registers */
                    ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0f800100, 0x0f800120)} /* Egress TCP Port Range Comparator Configuration */
                    ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0f800140, 0x0f800160)} /* Egress UDP Port Range Comparator Configuration */
                    ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0f800200, 0x0F800200)} /* Egress Policer Global Configuration */
                    ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0f808000, 16640)    }  /* Egress PCL Configuration Table */
                };
                GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

                smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,&devMemInfoPtr->xCatExtraMem.pclMemEgrUnitMem);
            }
            {
                SMEM_CHUNK_BASIC_STC  chunksMem[]=
                {
                     {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0e800054 ,0x0e800064)} /* TCC Error and Interrupt Registers*/
                    ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0e800960 ,0x0e800d78)} /* TCC_Registers  */
                    ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0e800080 ,0x0e8000a4)} /* Active Power Save */
                    ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0e802080 ,128)   }     /* TCAM Array Compare Enable */
                    ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0e800400 ,96)    }     /* Router TCAM Access Data Register */
                    /*,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0e800418 ,0x0e80041c)} -- already in (0x0e800400 ,96)*/ /* LTT and Router TCAM Access control */
                    ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0e8c0000 ,106496)    } /* Lookup Translation Table */
                    ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0e840000 ,212992)    } /* Router and TT TCAM */
                    ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0e890000 ,13312)     } /* ECC Table X */
                    ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0e8a0000 ,13312)     } /* ECC Table Y */

                };
                GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

                smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,&devMemInfoPtr->xCatExtraMem.ipvxTccUnitMem);
            }
            {
                #define L2_L3_PORT_ISOLATION_TABLE_DIM     {2176, 0x10}, SMEM_FORMULA_END_CNS
                SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
                {
                     {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01e40004 ,0)}, {L2_L3_PORT_ISOLATION_TABLE_DIM}}     /* L2 Port Isolation Table */
                    ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01e40008 ,0)}, {L2_L3_PORT_ISOLATION_TABLE_DIM}}     /* L3 Port Isolation Table */
                };
                GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);

                smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &devMemInfoPtr->xCatExtraMem.egrMemUnitMem);
            }
            {
                SMEM_CHUNK_BASIC_STC  chunksMem[]=
                {
                     {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x07800000 ,0x07800000)}/*Buffer Memory Write Access Register*/
                    /* HA top */
                    ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x07800004 ,0x07800420)}/*HA config registers*/
                    ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x07E80000 ,131072 )   }/*Router ARP DA and Tunnel Start Table*/
                    ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x07F80000 ,16640  )   }/*Port/VLAN MAC SA Table*/
                    ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x07FC0000 ,16384  )   }/*VLAN Translation Table*/

                    /* EPCL top */
                    ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC(0x07C00000,0x07C00140+8*4)}/*EPCL config registers*/
                    ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x07F00000,16640 )  }/*Egress PCL Configuration Table*/
                    /* Buffer memory */
                    ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x07E00000,264*2 )  }/*Prefetch Descriptors*/

                };
                GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

                smemInitMemChunk(devObjPtr,chunksMem, numOfChunks, &devMemInfoPtr->haAndEpclUnitMem);
            }
            {
                SMEM_CHUNK_BASIC_STC  chunksMem[]=
                {
                    {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0b00b000 ,0x0b00b040)}  /* Mirror to analyzer port configs */
                };
                GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

                smemInitMemChunk(devObjPtr,chunksMem, numOfChunks, &devMemInfoPtr->xCatExtraMem.eqUnitUnitMem);
            }
            {
                SMEM_CHUNK_BASIC_STC  chunksMem[]=
                {
                    {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x03020000, 24576)}  /* Control */
                   ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x03040000, 24576)}  /* McCnt */
                };
                GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

                smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,&devMemInfoPtr->xCatExtraMem.bufMngMemUnitMem);
            }
            {
                #define DSCP_MAP_QOS_PROFILE_DIM     {16, 0x4}, SMEM_FORMULA_END_CNS
                #define CFI_MAP_QOS_PROFILE_DIM      {2, 0x4}, {2, 0x10}, SMEM_FORMULA_END_CNS
                #define EXP_MAP_QOS_PROFILE_DIM      {2, 0x4}, SMEM_FORMULA_END_CNS
                #define DSCP_DSCP_MAP_TABLE_DIM      DSCP_MAP_QOS_PROFILE_DIM

                SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
                {
                    {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC(0x0c000000, 0x0c00003c)}}  /* TTI Unit Global Configuration */
                   ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC(0x0c000040, 0x0c00005c)}}  /* IPv6 Neighbor Solicitation MC */
                   ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC(0x0c000060, 0x0c000068)}}  /* DSA Configuration */
                   ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC(0x0c000070, 0x0c000078)}}  /* Trunk Hash */
                   ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC(0x0c000080, 0x0c00009c)}}  /* Protocol Match */
                   ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC(0x0c0000a0, 0x0c0000dc)}}  /* User-Defined Bytes */
                   ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC(0x0c000100, 0x0c000120)}}  /* Bridge Log Flow */
                   ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC(0x0c000140, 0x0c000188)}}  /* Router Log Flow */
                   ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0c000200 ,0)}, FORMULA_SINGLE_PARAMETER(38, 0x4)} /* TTI to PCL descriptor */
                   ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0c000298 ,0)}, FORMULA_SINGLE_PARAMETER(2, 0x4)}  /* wrDma to TTI descriptor */
                   ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC(0x0c0002b0 ,0x0c0002b0)}}  /* Metal Fix Register */
                   ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC(0x0c000300, 0x0c00030c)}}  /* VLAN Assignment */
                   ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC(0x0c000310, 0x0c0003d4)}}  /* Ingress VLAN Select */
                   ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0xc000400, 0)}, {DSCP_MAP_QOS_PROFILE_DIM}}    /* DSCP To QoSProfile Map Table */
                   ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0xc000440, 0)}, {CFI_MAP_QOS_PROFILE_DIM}}     /* CFI To QoSProfile Map Table */
                   ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0xc000460, 0)}, {EXP_MAP_QOS_PROFILE_DIM}}     /* EXP To QoSProfile Map Table */
                   ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC(0x0c000500, 0x0c000520)}}  /* Map To Parameters */
                   ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0c000600, 512)}}  /* DSA Tag QoS 2 QoS Parameters Map Memory */
                   ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0C000800, 256)}}  /* DSCP 2 QoS Parameters Map Memory */
                   ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0c000900, 0)}, {DSCP_MAP_QOS_PROFILE_DIM}}    /* DSCP To DSCP Map Table */
                   ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0c001600 ,0)}, FORMULA_TWO_PARAMETERS(4, 0x4, 8, 0x10)} /* MAC2ME Registers */
                   ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC(0x0c001700, 0x0c001720 )}} /* TCAM Configuration Registers */
                   ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC(0x0c001780, 0x0c001780)}}  /* Pointer <%n> Rel */
                   ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC(0x0c0017d0, 0x0c0017d8)}}  /* TCAM Bist Config Status Register, Opcode Registers */
                   ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0c001000, 1024), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(66, 16)}}  /* Port VLAN and QoS Configuration Table */
                   ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0c001800, 2048)}}  /* TTI DFX Area */
                   ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0c004000, 8192), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 8)}}  /* Port Protocol VID and QoS Configuration Table */
                   ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0c006000, 4096), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(128, 16)}}  /* MAC2ME TCAM */
                   ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0c00c000, 16384)}}  /* VLAN Translation Table */
                   ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0c010000, 256), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(255, 32)}}  /* User Defined Bytes Configuration Memory */
                   ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0c020000, 4208)}}  /* Virtual Port ID Assignment Memory */
                   ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0c030000, 16640)}}  /* User Defined Bytes ID Memory */
                };

                GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);

                smemInitMemChunkExt(devObjPtr,chunksMem,numOfChunks,&devMemInfoPtr->xCatExtraMem.ttiMemUnitMem);
            }

            if (devObjPtr->deviceRevisionId >= 4)
            {
                SMEM_CHUNK_BASIC_STC  chunksMem[]=
                {
                    /*Passenger Ingress TPID Select%n*/
                     {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0c000524, 0x0c0005a0)}

                };
                GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);
                SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

                smemInitMemChunk(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

                /*add the tmp unit chunks to the main unit */
                smemInitMemCombineUnitChunks(devObjPtr,&devMemInfoPtr->xCatExtraMem.ttiMemUnitMem,&tmpUnitChunk);
            }


            {
                SMEM_CHUNK_BASIC_STC  chunksMem[]=
                {
                     {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00880000 ,0x0088000C)}  /* Debug Bus */
                };
                GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

                smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,&devMemInfoPtr->xCatExtraMem.dfxUnitMem);
            }


        }
        else
        {
            {
                SMEM_CHUNK_BASIC_STC  chunksMem[]=
                {
                     {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x07800000 ,0x07800000)}/*Buffer Memory Write Access Register*/
                    /* HA top */
                    ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x07800004 ,0x07800420)}/*HA config registers*/
                    ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x07e80000 ,32768  )   }/*Router ARP DA and Tunnel Start Table*/
                    ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x07F80000 ,32768  )   }/*Port/VLAN MAC SA Table*/
                    ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x07FC0000 ,16384  )   }/*VLAN Translation Table*/

                    /* EPCL top */
                    ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC(0x07c00000,0x07c00140+8*4)}/*EPCL config registers*/
                    ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x07F00000,16640 )  }/*Egress PCL Configuration Table*/
                    /* Buffer memory */
                    ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x07E00000,264*2 )  }/*Prefetch Descriptors*/

                };
                GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

                smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,&devMemInfoPtr->haAndEpclUnitMem);
            }
            {
                SMEM_CHUNK_BASIC_STC  chunksMem[]=
                {
                     {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0D800400 ,0x0D80041C)} /*LTTAndRouterTCAMAccessCtrl_E*/
                    ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0D800960 ,0x0D800D78)} /*TCC_Registers               */
                    ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0D840000 ,212992)  } /*routerAndTTTCAM_E           */
                    ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0D880000 ,53248)   } /*lookup_Translation_Table_E  */
                    ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0D890000 ,13312)   } /*ECC_Table_X                 */
                    ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0D8a0000 ,13312)   } /*ECC_Table_y                 */
                };
                GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

                smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,&devMemInfoPtr->ipvxTccUnitMem);
            }
        }

        /* Allocate MBus unit memory */
        unitChunkPtr = &devMemInfoPtr->common.pciExtMemArr[SMEM_UNIT_PCI_BUS_MBUS_E].unitMem;
        smemXCatA1UnitPex(devObjPtr, unitChunkPtr, devMemInfoPtr->common.pciExtMemArr[SMEM_UNIT_PCI_BUS_MBUS_E].unitBaseAddr);
    }
    {
        GT_U32 callocMemBytes;
        devMemInfoPtr->common.unitChunksBasePtr = (GT_U8 *)&devMemInfoPtr->haAndEpclUnitMem;
        callocMemBytes = devMemInfoPtr->common.unitChunksBasePtr - (GT_U8 *)&devMemInfoPtr->common;
        /* Calculate summary of chunk memory structures - for future SW reset use */
        devMemInfoPtr->common.unitChunksSizeOf =
            sizeof(SMEM_CHT3_DEV_MEM_INFO) - callocMemBytes;
    }

    devObjPtr->devFindMemFunPtr = (void *)smemCht3FindMem;
}


/*******************************************************************************
*   smemCht3PciReg
*
* DESCRIPTION:
*       PCI memory access.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
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
static GT_U32 *  smemCht3PciReg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SMEM_CHT3_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32                  * regValPtr;
    GT_U32                    index;
    GT_U32                    group;

    regValPtr = 0;


    devMemInfoPtr = (SMEM_CHT3_DEV_MEM_INFO  *)devObjPtr->deviceMemory;
    if ((address & 0xFFFFFF00) == 0x00071800)
    {
        group = 0;
        index = (address & 0xFF) / 0x4;
    }
    else
    if ((address & 0xFFFFFF00) == 0x00071900)
    {
        group = 1;
        index = (address & 0xFF) / 0x4;
    }
    else
    if ((address & 0xFFFFFF00) == 0x00071A00)
    {
        group = 2;
        index = (address & 0xFF) / 0x4;
    }
    else
    {
        group = 3;
        index = (address & 0x1FF) / 0x4;
    }

    CHECK_MEM_BOUNDS(devMemInfoPtr->pciMem.pciReg[group],
                     devMemInfoPtr->pciMem.pciRegNum,
                     index, memSize);

    regValPtr = &devMemInfoPtr->pciMem.pciReg[group][index];

    return regValPtr;
}

/*******************************************************************************
*   smemCht3PclReg
*
* DESCRIPTION:
*       PCL registers and TCAM memory space access.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
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
static GT_U32 *  smemCht3PclReg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SMEM_CHT3_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32                  * regValPtr;
    GT_U32                    index;
    GT_U32                    group;

    regValPtr = 0;
    devMemInfoPtr = (SMEM_CHT3_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    if ((address & 0xFFFFE000) == 0x0b800000)
    {
        index = (address & 0x1FFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->pclMem.ingrPclCommonReg,
                         devMemInfoPtr->pclMem.ingrPclCommonRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->pclMem.ingrPclCommonReg[index];
    }
    else
    /* PCL tcam internal configuration */
    if ((address & 0xFFFFF000) == 0x0b820000)
    {
        index = (address & 0x0FFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->pclMem.pclTcamInternalConfigReg,
                         devMemInfoPtr->pclMem.pclTcamInternalConfigRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->pclMem.pclTcamInternalConfigReg[index];
    }
    else
    /* PCL tcam test configuration */
    if ((address & 0xFFFFF000) == 0x0b830000)
    {
        index = (address & 0x0FFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->pclMem.pclTcamTestConfigReg,
                         devMemInfoPtr->pclMem.pclTcamTestConfigRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->pclMem.pclTcamTestConfigReg[index];
    }
    else
    /* PCL configuration table */
    if ((address & 0xFFFF0000) == 0x0b840000)
    {
        index = (address & 0xFFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->pclMem.ingrPclConfigTableReg,
                         devMemInfoPtr->pclMem.ingrPclConfigTableRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->pclMem.ingrPclConfigTableReg[index];
    }
    else
    /* PCL action table -  14336 entries * 16 bytes */
    if (SKERNEL_DEVICE_FAMILY_CHEETAH_3_ONLY(devObjPtr) &&
        ((address & 0xFFFC0000) == 0x0b880000))/* xcat moved to IPCL TCC -- Policy Action Table 0x0D0B0000,49152 bytes*/
    {
        index = (address & 0xFFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->pclMem.policyActionTableReg,
                         devMemInfoPtr->pclMem.policyActionTableRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->pclMem.policyActionTableReg[index];
    }
    else
    if (SKERNEL_DEVICE_FAMILY_CHEETAH_3_ONLY(devObjPtr) && /*xcat move to IPCL TCC -
                                                            Policy ECC X 0x0D0C0000 , 12288 bytes,
                                                            Policy ECC Y 0x0D0D0000 , 12288 bytes */
        (address & 0xFFF00000) == 0x0b900000)
    {
        group = ((address >> 16) & 0xF) / 0x4;
        index = (address & 0xFFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->pclMem.policyEccReg[group],
                         devMemInfoPtr->pclMem.policyEccRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->pclMem.policyEccReg[group][index];
    }
    else
    /* TCAM Registers */
    if (SKERNEL_DEVICE_FAMILY_CHEETAH_3_ONLY(devObjPtr) &&  /*xcat move to IPCL TCC -
                                                             Policy TCAM 0x0D040000, 196608 bytes */
        (address & 0xFFF00000) == 0x0ba00000)
    {
        index = (address & 0xFFFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->pclMem.policyTcamReg,
                         devMemInfoPtr->pclMem.policyTcamRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->pclMem.policyTcamReg[index];
    }
    else
    /* VLAN translation table */
    if (address >= 0x0b804000 && address < 0x0b808000)
    {
        index = (address & 0x3FFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->pclMem.vlanTranslationTableReg,
                         devMemInfoPtr->pclMem.vlanTranslationTableRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->pclMem.vlanTranslationTableReg[index];
    }
    else
    /* Port protocol VID and QoS configuration registers */
    if ((address & 0xFFFFF800) == 0x0b810800)
    {
        index = (address & 0x7FF)/4 ;
        CHECK_MEM_BOUNDS(devMemInfoPtr->pclMem.portProtVidQoSConfigReg,
                         devMemInfoPtr->pclMem.portProtVidQoSConfigRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->pclMem.portProtVidQoSConfigReg[index];
    }
    else
    /* Port VLAN and QoS configuration registers */
    if ((address & 0xFFFFF000) == 0x0b810000)
    {
        index = (address & 0x7FF)/4 ;
        CHECK_MEM_BOUNDS(devMemInfoPtr->pclMem.portVlanQoSConfigReg,
                         devMemInfoPtr->pclMem.portVlanQoSConfigRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->pclMem.portVlanQoSConfigReg[index];
    }

    return regValPtr;
}

/*******************************************************************************
*   smemCht3PolicerReg
*
* DESCRIPTION:
*       Policer registers and meters memory space access
*
* INPUTS:
*       devObjPtr   - pointer to device object.
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
static GT_U32 *  smemCht3PolicerReg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SMEM_CHT3_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32                  * regValPtr;
    GT_32                     index;

    regValPtr = 0;
    devMemInfoPtr = (SMEM_CHT3_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    /* Policer Configuration Registers */
    if ((address & 0xFFFFFE00) == 0x0C000000)
    {
        index = (address & 0x1FF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->policerMem.policerReg,
                         devMemInfoPtr->policerMem.policerRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->policerMem.policerReg[index];
    }
    else
    if ((address & 0xFFFFFF00) == 0x0C000200)
    {
        index = (address & 0xFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->policerMem.policerTimerReg,
                         devMemInfoPtr->policerMem.policerTimerRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->policerMem.policerTimerReg[index];
    }
    else
    if ((address & 0xFFFFFF00) == 0x0C000400)
    {
        index = (address & 0xFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->policerMem.policerDescrSampleReg,
                         devMemInfoPtr->policerMem.policerDescrSampleRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->policerMem.policerDescrSampleReg[index];
    }
    else
    if ((address & 0xFFFF8000) == 0x0C040000)
    {
        index = (address & 0x7FFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->policerMem.policerTblReg,
                         devMemInfoPtr->policerMem.policerTblRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->policerMem.policerTblReg[index];
    }
    else
    if ((address & 0xFFFF8000) == 0x0C048000)
    {
        index = (address & 0x7FFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->policerMem.policerCntTblReg,
                         devMemInfoPtr->policerMem.policerCntTblRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->policerMem.policerCntTblReg[index];
    }
    else
    if ((address & 0xFFFF0000) == 0x0C080000)
    {
        index = (address & 0xFFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->policerMem.policerQosRmTblReg,
                         devMemInfoPtr->policerMem.policerQosRmTblRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->policerMem.policerQosRmTblReg[index];
    }
    else
    if ((address & 0xFFFF0000) == 0x0C0C0000)
    {
        index = (address & 0xFFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->policerMem.policerMngCntTblReg,
                         devMemInfoPtr->policerMem.policerMngCntTblRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->policerMem.policerMngCntTblReg[index];
    }

    return regValPtr;
}


/*******************************************************************************
*   smemCht3TriSpeedReg
*
* DESCRIPTION:
*      Describe a Tri-Speed Ports MAC, CPU Port MAC, and 1.25 Gbps SERDES
*      Configuration Registers Map Table,  Hyper.GStack Ports MAC and
*      XAUI PHYs Configuration Registers Map Table memory access
*
* INPUTS:
*       devObjPtr   - pointer to device object.
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
static GT_U32 *  smemCht3TriSpeedReg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SMEM_CHT3_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32                  * regValPtr;
    GT_32                     index;

    regValPtr = 0;
    devMemInfoPtr = (SMEM_CHT3_DEV_MEM_INFO  *)devObjPtr->deviceMemory;


    /*NOTE: in xcat the function can also be called from the 'Stacking ports unit'*/
    /*
        meaning not only with addresses of : 0x0a80.... but also 0x0880....

        there is no overlap because:
            the 'ports' in 0x0a80.... are : 0..23
        and the 'ports' in 0x0880.... are : 24..27
    */

    index = (address & 0xFFFF) / 0x4;
    CHECK_MEM_BOUNDS(devMemInfoPtr->triSpeedPortsMem.triSpeedPortsReg,
                     devMemInfoPtr->triSpeedPortsMem.triSpeedPortsRegNum,
                     index, memSize);
    regValPtr = &devMemInfoPtr->triSpeedPortsMem.triSpeedPortsReg[index];

    return regValPtr;
}

/*******************************************************************************
*   smemCht3PreEgressReg
*
* DESCRIPTION:
*      Pre-Egress Engine Registers Map Table, Trunk Table,
*      QoSProfile to QoS Table, CPU Code Table,
*      Data Access Statistical Rate Limits Table Data Access,
*      Ingress STC Table Registers memory access
*
* INPUTS:
*       devObjPtr   - pointer to device object.
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
static GT_U32 *  smemCht3PreEgressReg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    return  smemDevFindInUnitChunk(devObjPtr, accessType, address, memSize,
        (GT_UINTPTR)&(((SMEM_CHT3_DEV_MEM_INFO*)devObjPtr->deviceMemory)->eqUnitMem));
}

/*******************************************************************************
*   smemCht3VlanTableReg
*
* DESCRIPTION:
*       Describe a device's Vlan table
*
* INPUTS:
*       devObjPtr   - pointer to device object.
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
static GT_U32 *  smemCht3VlanTableReg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SMEM_CHT3_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32              * regValPtr;
    GT_U32                index;

    regValPtr = 0;
    devMemInfoPtr = (SMEM_CHT3_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

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
    if ((address & 0xFFF00000) == 0x0A300000)
    {
        /* VRF table */
        index = (address & 0xFFFF) / 0x10; /* 12 bits for 4096 registers , those registers in offset of 0x10 */
        if(address & 0xF)/* I.E address 0x0A300000 + 0x4/0x8/0xC + 0x10*j    */
        {
            return NULL;
              /*skernelFatalError(" not defined memory");*/
        }
        CHECK_MEM_BOUNDS(devMemInfoPtr->vlanTblMem.vrfTblReg,
                         devMemInfoPtr->vlanTblMem.vrfTblRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->vlanTblMem.vrfTblReg[index];
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
*  smemCht3MllEngineReg
*
* DESCRIPTION:
*       MLL engine registers
*
* INPUTS:
*       devObjPtr   - device object PTR.
*       address     - Address for ASIC memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers' specific parameter.
*       inMemPtr    - Pointer to the memory to get register's content.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static GT_U32 * smemCht3MllEngineReg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SMEM_CHT3_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32                  * regValPtr;
    GT_U32                    index;

    regValPtr = 0;
    devMemInfoPtr = (SMEM_CHT3_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    if(address >= 0x0c880000)/* MLL table*/
    {
        index = (address - 0x0c880000) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->mllMem.mllEntryReg,
                         devMemInfoPtr->mllMem.mllEntryRegNum,
                         index, memSize);

        regValPtr = &devMemInfoPtr->mllMem.mllEntryReg[index];
    }
    else
    {
        index = (address & 0xffff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->mllMem.mllConfReg,
                         devMemInfoPtr->mllMem.mllConfRegNum,
                         index, memSize);

        regValPtr = &devMemInfoPtr->mllMem.mllConfReg[index];
    }


    return regValPtr;
}

/*******************************************************************************
*  smemCht3SataSerdesReg
*
* DESCRIPTION:
*       SATA SerDes registers
*
* INPUTS:
*       devObjPtr - device object PTR.
*       address     - Address for ASIC memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers' specific parameter.
*       inMemPtr    - Pointer to the memory to get register's content.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static GT_U32 * smemCht3SataSerdesReg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    if(!SKERNEL_IS_CHEETAH3_ONLY_DEV(devObjPtr))
    {
        return smemDevFindInUnitChunk(devObjPtr, accessType, address, memSize,
            (GT_UINTPTR)&(((SMEM_CHT3_DEV_MEM_INFO*)devObjPtr->deviceMemory)->uniphySerdesUnitMem));
    }
    else  /*ch3*/
    {
        static GT_U32 dummySerdesSpace[1];
        GT_U32  index;

        /* simulate all SERDESs registers into single one */
        index = 0;

        return &dummySerdesSpace[index];
    }
}

/*******************************************************************************
*  smemCht3pExtraMem
*
* DESCRIPTION:
*       access the extra memory of ch3+
*       this is needed for quick development of simulation and SW.
*
* INPUTS:
*       devObjPtr - device object PTR.
*       address     - Address for ASIC memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers' specific parameter.
*       inMemPtr    - Pointer to the memory to get register's content.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static GT_U32 * smemCht3pExtraMem
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SMEM_CHT3_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32                    index;

    devMemInfoPtr = (SMEM_CHT3_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    index = (address & 0x007FFFFC) / 0x4;

    CHECK_MEM_BOUNDS(devMemInfoPtr->ch3pExtraMem.extraRegPtr,
                     devMemInfoPtr->ch3pExtraMem.extraRegNum,
                     index, memSize);

    return &devMemInfoPtr->ch3pExtraMem.extraRegPtr[index];
}



/*******************************************************************************
*  smemCht3ActiveWriteRouterAction
*
* DESCRIPTION:
*    The Tcam router engine table and TT write access.
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
void smemCht3ActiveWriteRouterAction
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
)
{
    GT_U32     rtLineNumber;    /* Router Tcam Line Number */
    GT_U32     rtWr;            /* Write operation */
    GT_U32     rtDataType;      /* Router Tcam data type */
    GT_U32     rtValid;          /* TCAM valid entry bit */
    GT_U32     rtBankEnBmp;     /* bmp of enabled banks */
    GT_U32     rtCompMode;      /* TCAM compare mode , IPv4 ... IPv6 */
    GT_U32     rtSpare;         /* Entry spare bit */
    GT_U32     actionTarget;    /*Target Data Structure.
                                  0x0 = Router TCAM; RouterTCAM Write.
                                  0x1 = LTT; Action Table access for read or write.
                                  0x2 = RouterECC Table;*/

    GT_U32     fldValue;

    /* Read router and TT tcam Access Control Register 1*/
    rtLineNumber = (*inMemPtr >> 2) & 0x3FFF;
    rtDataType = (*inMemPtr >> 21) & 0x1;
    rtBankEnBmp = (*inMemPtr >> 16) & 0xf;
    rtWr = (*inMemPtr & 0x1);    /* write action */
    if(SKERNEL_DEVICE_FAMILY_CHEETAH_3_ONLY(devObjPtr))
    {
        /* ch3 support only Router TCAM */
        actionTarget = 0;
    }
    else
    {
        actionTarget = (*inMemPtr >> 22) & 0x3;
    }

    /* Read router and TT tcam Access Control Register 0*/
    smemRegGet(devObjPtr, SMEM_CHT3_ROUTER_ACCESS_DATA_CTRL_REG(devObjPtr) , &fldValue);
    rtCompMode = (fldValue >> 16) & 0xff;
    rtValid    = (fldValue & 0xf);
    rtSpare    = (fldValue >> 8) & 0xf;

    switch(actionTarget)
    {
        case 0:
            smemCht3ReadWriteRouterTTTcam(devObjPtr, rtWr ?  GT_TRUE : GT_FALSE,rtLineNumber, rtDataType,
                                        rtBankEnBmp, rtValid, rtCompMode,rtSpare);
            break;
        case 1:
            /* accessing the LLT  */

            /* not implemented -- see CQ# 105927 */
            break;
        case 2:
            /* accessing the ECC */

            /* not need to be implemented , since no ECC meaning in simulation */
            break;
        case 3:
        default:
            /* should not happen */

            /* simulation ignore it */
            break;
    }


    /* Cleared by the device when the read or write action is completed. */
    *memPtr = *inMemPtr & (~1);
}

/*******************************************************************************
*  smemCht3ActiveWritePclAction
*
* DESCRIPTION:
*    The action table and policy TCAM write access.
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
void smemCht3ActiveWritePclAction
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
)
{
    GT_U32     lineNumber;      /* line number in TCAM and policy action table*/
    GT_U32     rdWr;            /* Read or write operation */
    GT_U32     rdWrTarget;      /* Read or Write Target Data Structure(Tcam/Action) */
    GT_U32     tcamDataType;    /* TCAM data type */
    GT_U32     tcamCompMode;    /* TCAM compare mode , IPv4 ... IPv6 */

    /* Read Action Table and Policy TCAM Access Control Register */
    rdWrTarget = (*inMemPtr >> SMEM_CHT_GROUP_INDEX_FIRST_BIT_CNS) & 0x3;
    tcamDataType = (*inMemPtr >> 15) & 0x1;
    lineNumber = ((*inMemPtr >> 2) & 0x1FFF) | (((*inMemPtr >> 18) & 1 ) << 13);
    rdWr = (*inMemPtr >> 1) & 0x1;    /* 0 - read , 1 - write*/
    tcamCompMode = (*inMemPtr >> 19) & 0x1;

    /* stored as is to be retrieved in called functions */
    *memPtr = *inMemPtr ;

    if (rdWrTarget == 0) /* Read or Write the TCAM policy table */
    {
        smemCht3ReadWritePolicyTcam(devObjPtr, lineNumber,
                                    tcamDataType, rdWr,tcamCompMode);
    }
    else if(rdWrTarget == 1)
    {   /* Read or Write the PCL action table */
        smemCht3ReadWritePclAction(devObjPtr, lineNumber, rdWr);
    }
    else
    {
        /* ECC not supported */
    }

    /* Cleared by the device when the read or write action is completed. */
    *memPtr = *inMemPtr & (~1);
}


/*******************************************************************************
*  smemCht3ActiveReadTcamBistConfigAction
*
* DESCRIPTION:
*    Policy/Router TCAM BIST config read access.
*
* INPUTS:
*    devObjPtr  - device object PTR.
*    address    - Address for ASIC memory.
*    memPtr     - Pointer to the register's memory in the simulation.
*    memSize    - Size of memory to be written
*    param      - Registers' specific parameter.
*    outMemPtr   - Pointer to the memory to get register's content.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void smemCht3ActiveReadTcamBistConfigAction
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * outMemPtr
)
{
    /* Simulate BIST done */
    *outMemPtr = 0x1028B;
}

/*******************************************************************************
*  smemCht3ActiveWritePolicerTbl
*
* DESCRIPTION:
*    The Policers table write access.
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
void smemCht3ActiveWritePolicerTbl
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
)
{
    GT_U32 trigCmd;             /* type of triggered action that the Policer
                                   needs to perform */
    GT_U32 entryOffset;         /* the offset from the base address of the
                                   accessed entry */
                                /* device memory pointer */
    GT_U32 * regPtr;            /* table entry pointer */
    GT_U32 regAddr;             /* table base address */
    GT_U32 cycle = param;       /* ingress policer unit (could be 0 or 1) */

    trigCmd = SMEM_U32_GET_FIELD(*inMemPtr, 1, 3);
    entryOffset = SMEM_U32_GET_FIELD(*inMemPtr, 16, 16);

    switch (trigCmd)
    {
    /* Counter entry will be read to the Ingress Policer Table Access Data Registers */
    case 0:
    case 1:
        regAddr = SMEM_CHT_POLICER_CNT_TBL_MEM(devObjPtr, cycle, entryOffset);
        regPtr = smemMemGet(devObjPtr, regAddr);

        regAddr = SMEM_CHT3_INGRESS_POLICER_TBL_DATA_TBL_MEM(devObjPtr, cycle, 0);
        smemMemSet(devObjPtr, regAddr, regPtr, 8);
        if (trigCmd == 0)
        {
            /* The counter entry will be reset and written back to memory. */
            memset(regPtr, 0, 8 * sizeof(GT_U32));
        }
        break;

    /* Sets metering entry fields, according to the information placed in the
       Ingress Policer Table Access Data Registers. */
    case 4:
        regAddr = SMEM_CHT3_INGRESS_POLICER_TBL_DATA_TBL_MEM(devObjPtr, cycle, 0);
        regPtr = smemMemGet(devObjPtr, regAddr);

        regAddr = SMEM_CHT_POLICER_TBL_MEM(devObjPtr, cycle, entryOffset);
        if(!SKERNEL_IS_CHEETAH3_ONLY_DEV(devObjPtr))
        {
            smemMemSet(devObjPtr, regAddr, regPtr, 8);
        }
        else
        {
            /* In Cheetah3 according to formula:
            0xC000030 + n*4+max(n-5,0)*4: where n (0-6) represents Reg */
            smemMemSet(devObjPtr, regAddr, regPtr, 6);
            /* Copy word 6 from offset 0x1c in Table Access Data Registers to offset 0x18 */
            smemMemSet(devObjPtr, regAddr + 0x18, &regPtr[7], 1);
        }
        break;

    /* Refresh the metering entry status fields */
    case 5:
        break;
    default:
        return;
    }

    /* Cleared by the device when the read or write action is completed. */
    SMEM_U32_SET_FIELD(*inMemPtr, 0, 1, 0);

    *memPtr = *inMemPtr;
}

/*******************************************************************************
*   smemXCatPolicerIPFixTimeStampUpload
*
* DESCRIPTION:
*        Upload Policer IPFix timestamp
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       cycle       - policer stage cycle
*
* OUTPUT:
*       ipFixTimerPtr - pointer to uploaded IPFix timer stamp
*
*
*******************************************************************************/
static GT_VOID smemXCatPolicerIPFixTimeStampUpload
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 cycle,
    OUT SNET_TOD_TIMER_STC * ipFixTimerPtr
)
{
    GT_U32 clocks;              /* TOD value in clocks */
    GT_U32 seconds;             /* TOD seconds */
    GT_U32 nanoSeconds;         /* TOD nanoseconds */
    GT_U64 seconds64;           /* 64 bits seconds */
    GT_U32 regAddr;             /* register address */
    GT_U32 *regPtr;             /* register data pointer */

    ASSERT_PTR(ipFixTimerPtr);

    /* Current TOD value */
    clocks = SNET_XCAT_IPFIX_TOD_CLOCK_GET_MAC(devObjPtr, cycle);

    /* Convert tick clocks to seconds and nanoseconds */
    SNET_TOD_CLOCK_FORMAT_MAC(clocks, seconds, nanoSeconds);

    /* Policer Timer Memory starting from word six:
    IPfix nano timer, second timer [31:0] and second timer [63:32] */
    regAddr = SMEM_XCAT_POLICER_TIMER_TBL_MEM(devObjPtr, cycle, 6);
    regPtr = smemMemGet(devObjPtr, regAddr);

    /* IPfix nano timer is incremented by the value of time stamp upload registers */
    ipFixTimerPtr->nanoSecondTimer = regPtr[0] + nanoSeconds;

    /* Convert timer data to 64 bit value*/
    seconds64.l[0] = seconds;
    seconds64.l[1] = 0;
    /* Second timer low word [31:0] */
    ipFixTimerPtr->secondTimer.l[0] = regPtr[1];
    /* Second timer hi word [63:32] */
    ipFixTimerPtr->secondTimer.l[1] = regPtr[2];

    /* IPfix timer is incremented by the value of time stamp upload registers */
    ipFixTimerPtr->secondTimer =
        prvSimMathAdd64(ipFixTimerPtr->secondTimer, seconds64);
}

/*******************************************************************************
*  smemXCatActiveWritePolicerTbl
*
* DESCRIPTION:
*    The Policers table write access.
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
void smemXCatActiveWritePolicerTbl
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
)
{
    DECLARE_FUNC_NAME(smemXCatActiveWritePolicerTbl);
    GT_U32 trigCmd;             /* type of triggered action that the Policer
                                   needs to perform */
    GT_U32 entryOffset;         /* the offset from the base address of the
                                   accessed entry */
                                /* device memory pointer */
    GT_U32 * regPtr;            /* table entry pointer */
    GT_U32 regAddr;             /* table base address */
    GT_U32 cntMode;             /* counter mode: Billing/IPfix */
    GT_U32 direction;           /* Ingress/Egress policer engine */
    GT_U32 cycle;               /* Ingress policer cycle */
    GT_U32 clockVal;            /* Number of clock ticks */
    GT_U32 goodPckts, timeStamp, dropPckts; /* good packets, time stamp, drop packets value */
    GT_U64 bytesCnt;            /* bytes counter value */
    GT_U32*entryPtr;            /* pointer to entry in memory */
    GT_U32 entryAddr;           /* address of counting entry */
    GT_U32 entry[XCAT_POLICER_CNTR_ENTRY_NUM_WORDS_CNS]; /* memory to store counting entry */
    GT_U32 countingEntryFormat; /* Long or short format of counting entry */
    GT_U32 wordsArr[2];
    GT_U32 meterEntryAddr;
    GT_U32 meterCfgEntryAddr;
    GT_U32 counterEntryAddr;
    GT_U32 max_bucket_size0, max_bucket_size1; /* maximal bucket sizes configured by application */
    GT_U32 bucket_size0, bucket_size1;         /* internal current sizes of leaky  buckets       */
    GT_U32 rate_type0, rate_type1;             /* rate types (maximal bucket size resolution)    */
    GT_U32 bytesFactor0, bytesFactor1;         /* calculated maximal bucket size resolution      */
    GT_U32 tokenBucket0Sign , tokenBucket1Sign;/* calculated TB 0,1 sign */
    GT_U32 policerMru;
    GT_U32 policer_mode;                      /* policer mode */
    GT_U32 meterBaseAddr = 0;                 /* metering table base address */
    GT_U32 meterMemoryCycle;                  /* policer stage for meter tables access */

    /* Ingress/Egress Policer engine */
    direction = (param == 2) ?
            SMAIN_DIRECTION_EGRESS_E : SMAIN_DIRECTION_INGRESS_E;

    /* Ingress Policer cycle number */
    meterMemoryCycle = cycle = param;

    trigCmd = SMEM_U32_GET_FIELD(*inMemPtr, 1, 3);
    entryOffset = SMEM_U32_GET_FIELD(*inMemPtr, 16, 16);

    /* check Update Billing Entry Counter Mode bit for Lion B0 and above
       but only when Trigger Command  is 0x0 = CounterReadAndReset.
       Value 1 of this bit means read entry to Policer Table Access Data Registers
       and update entry by content of Policer Table Access Data Registers. */
    if (SKERNEL_IS_LION_REVISON_B0_DEV(devObjPtr) &&
        (0 == trigCmd) &&
        (1 == SMEM_U32_GET_FIELD(*inMemPtr, 7, 1)))
    {
        /* read and update command */
        trigCmd = 8;
    }

    if(SMEM_CHT_IS_SIP5_15_GET(devObjPtr))
    {
        /* read metering base address */
        smemRegFldGet(devObjPtr,
                      SMEM_LION3_POLICER_METERING_BASE_ADDR_REG(devObjPtr, cycle),
                      0, 24, &meterBaseAddr);

        /* meter memories became shared between IPLR0 and IPLR1 starting from SIP 5.15 (BobK)
           Direct access done through IPLR0. And base address is used to get absolute address in memory.  */
        meterMemoryCycle = (cycle == 1) ? 0 : cycle;
        meterEntryAddr   = SMEM_XCAT_POLICER_METER_ENTRY_TBL_MEM(devObjPtr, direction, meterMemoryCycle, entryOffset + meterBaseAddr);
        __LOG(("In case of meter update write data to IPLR[%d] index %d\n", meterMemoryCycle, (entryOffset + meterBaseAddr)));
    }
    else
    {
        meterEntryAddr   = SMEM_XCAT_POLICER_METER_ENTRY_TBL_MEM(devObjPtr, direction, cycle, entryOffset);
    }

    counterEntryAddr = SMEM_XCAT_POLICER_CNT_ENTRY_TBL_MEM  (devObjPtr, direction, cycle, entryOffset);

    if(counterEntryAddr == SMAIN_NOT_VALID_CNS ||
       meterEntryAddr == SMAIN_NOT_VALID_CNS)
    {
        skernelFatalError( "smemXCatActiveWritePolicerTbl: plr[%d] have no memory. Check Policer Memory Control configuration \n",
            cycle);
    }


    switch (trigCmd)
    {
    /* Counter entry will be read to the Policer Table Access Data Registers */
    case 0:
    case 1:
        regAddr = counterEntryAddr;
        regPtr = smemMemGet(devObjPtr, regAddr);
        /* pointer to the registers that CPU read/write for 'indirect access' */
        regAddr = SMEM_XCAT_POLICER_INDIRECT_ACCESS_DATA_ENTRY_REG(devObjPtr, cycle, 0);
        smemMemSet(devObjPtr, regAddr, regPtr,  (devObjPtr->tablesInfo.policer.paramInfo[0].step / 4));
        if (trigCmd == 0)
        {
            countingEntryFormat =
                snetXCatPolicerCountingEntryFormatGet(devObjPtr, direction, cycle);
            if (countingEntryFormat)    /* Short entry format */
            {
                cntMode = 0;  /* set Billing mode in "short" format, IPFix not supported */
            }
            else    /* Full entry format */
            {

                if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
                {
                    cntMode =
                        SMEM_LION3_PLR_BILLING_ENTRY_FIELD_GET(devObjPtr,regPtr,
                            entryOffset,
                            SMEM_LION3_PLR_BILLING_TABLE_FIELDS_COUNTER_MODE_E);
                }
                else
                {
                    /* Counter Mode */
                    cntMode = SMEM_U32_GET_FIELD(regPtr[7], 4, 1);
                }
            }
            /* IPfix */
            if(cntMode)
            {
                goodPckts = dropPckts = 0;
                bytesCnt.l[0] = bytesCnt.l[1] = 0;
                clockVal = SNET_XCAT_IPFIX_TOD_CLOCK_GET_MAC(devObjPtr, cycle);

                /* Convert clock value in ticks to time stamp format */
                snetXcatIpfixTimestampFormat(devObjPtr, clockVal, &timeStamp);
                /* The <Packet Counter>, <Byte Counter>, and <Dropped Packet Counter> fields are cleared */
                snetXcatIpfixCounterWrite(devObjPtr, NULL/*no descriptor to give*/ , regPtr, &bytesCnt, goodPckts,
                                          timeStamp, dropPckts);
                /* The <Last Sampled Value> is cleared */
                if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
                {
                    wordsArr[0] = wordsArr[1] = 0;
                    SMEM_LION3_PLR_IPFIX_ENTRY_LARGE_FIELD_SET(devObjPtr,regPtr,
                        entryOffset,
                        SMEM_LION3_PLR_IPFIX_TABLE_FIELDS_LAST_SAMPLED_VALUE_E,
                        wordsArr);
                }
                else
                {
                    SMEM_U32_SET_FIELD(regPtr[4], 2, 30, 0);
                    SMEM_U32_SET_FIELD(regPtr[5], 0, 2, 0);
                    SMEM_U32_SET_FIELD(regPtr[5], 2, 4, 0);
                }
            }
            else
            {
                if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
                {
                    /* reset all fields , exclude : SMEM_LION3_PLR_BILLING_TABLE_FIELDS_COUNTER_MODE_E */

                    wordsArr[0] = wordsArr[1] = 0;
                    /* Green Counter */
                    SMEM_LION3_PLR_BILLING_ENTRY_LARGE_FIELD_SET(devObjPtr,regPtr,
                        entryOffset,
                        SMEM_LION3_PLR_BILLING_TABLE_FIELDS_GREEN_COUNTER_E,
                        wordsArr);
                    /* Yellow Counter */
                    SMEM_LION3_PLR_BILLING_ENTRY_LARGE_FIELD_SET(devObjPtr,regPtr,
                        entryOffset,
                        SMEM_LION3_PLR_BILLING_TABLE_FIELDS_YELLOW_COUNTER_E,
                        wordsArr);
                    /* Red Counter */
                    SMEM_LION3_PLR_BILLING_ENTRY_LARGE_FIELD_SET(devObjPtr,regPtr,
                        entryOffset,
                        SMEM_LION3_PLR_BILLING_TABLE_FIELDS_RED_COUNTER_E,
                        wordsArr);

                    SMEM_LION3_PLR_BILLING_ENTRY_LARGE_FIELD_SET(devObjPtr,regPtr,
                        entryOffset,
                        SMEM_LION3_PLR_BILLING_TABLE_FIELDS_RED_COUNTER_E,
                        wordsArr);

                    SMEM_LION3_PLR_BILLING_ENTRY_FIELD_SET(devObjPtr,regPtr,
                        entryOffset,
                        SMEM_LION3_PLR_BILLING_TABLE_FIELDS_BILLING_COUNTERS_MODE_E,
                        0);

                    SMEM_LION3_PLR_BILLING_ENTRY_FIELD_SET(devObjPtr,regPtr,
                        entryOffset,
                        SMEM_LION3_PLR_BILLING_TABLE_FIELDS_BILLING_COUNT_ALL_EN_E,
                        0);
                    SMEM_LION3_PLR_BILLING_ENTRY_FIELD_SET(devObjPtr,regPtr,
                        entryOffset,
                        SMEM_LION3_PLR_BILLING_TABLE_FIELDS_LM_COUNTER_CAPTURE_MODE_E,
                        0);
                    SMEM_LION3_PLR_BILLING_ENTRY_LARGE_FIELD_SET(devObjPtr,regPtr,
                        entryOffset,
                        SMEM_LION3_PLR_BILLING_TABLE_FIELDS_GREEN_COUNTER_SNAPSHOT_E,
                        wordsArr);
                    SMEM_LION3_PLR_BILLING_ENTRY_FIELD_SET(devObjPtr,regPtr,
                        entryOffset,
                        SMEM_LION3_PLR_BILLING_TABLE_FIELDS_GREEN_COUNTER_SNAPSHOT_VALID_E,
                        0);
                    SMEM_LION3_PLR_BILLING_ENTRY_FIELD_SET(devObjPtr,regPtr,
                        entryOffset,
                        SMEM_LION3_PLR_BILLING_TABLE_FIELDS_PACKET_SIZE_MODE_E,
                        0);
                    SMEM_LION3_PLR_BILLING_ENTRY_FIELD_SET(devObjPtr,regPtr,
                        entryOffset,
                        SMEM_LION3_PLR_BILLING_TABLE_FIELDS_TUNNEL_TERMINATION_PACKET_SIZE_MODE_E,
                        0);
                    SMEM_LION3_PLR_BILLING_ENTRY_FIELD_SET(devObjPtr,regPtr,
                        entryOffset,
                        SMEM_LION3_PLR_BILLING_TABLE_FIELDS_INCLUDE_LAYER1_OVERHEAD_E,
                        0);
                    SMEM_LION3_PLR_BILLING_ENTRY_FIELD_SET(devObjPtr,regPtr,
                        entryOffset,
                        SMEM_LION3_PLR_BILLING_TABLE_FIELDS_DSA_TAG_COUNTING_MODE_E,
                        0);
                    SMEM_LION3_PLR_BILLING_ENTRY_FIELD_SET(devObjPtr,regPtr,
                        entryOffset,
                        SMEM_LION3_PLR_BILLING_TABLE_FIELDS_TIMESTAMP_TAG_COUNTING_MODE_E,
                        0);
                }
                else
                {
                    /* Green Counter */
                    regPtr[0] = 0;
                    SMEM_U32_SET_FIELD(regPtr[1], 0, 10, 0);
                    /* Yellow Counter */
                    SMEM_U32_SET_FIELD(regPtr[1], 10, 22, 0);
                    SMEM_U32_SET_FIELD(regPtr[2], 0, 20, 0);
                    /* Red Counter */
                    SMEM_U32_SET_FIELD(regPtr[2], 20, 12, 0);
                    SMEM_U32_SET_FIELD(regPtr[3], 0, 30,  0);
                }
            }
        }

        break;

    /* Sets metering entry fields, according to the information placed in the
       Policer Table Access Data Registers. */
    case 4:
        /* pointer to the registers that CPU read/write for 'indirect access' */
        regAddr = SMEM_XCAT_POLICER_INDIRECT_ACCESS_DATA_ENTRY_REG(devObjPtr, cycle, 0);
        regPtr = smemMemGet(devObjPtr, regAddr);

        /* copy to table entry */
        regAddr = meterEntryAddr;
        smemMemSet(devObjPtr, regAddr, regPtr, (devObjPtr->tablesInfo.policer.paramInfo[0].step / 4));

        if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            /* update bucketet_size0-1 fields in table entry */
            regPtr = smemMemGet(devObjPtr, meterEntryAddr);

            max_bucket_size0 = SMEM_LION3_PLR_METERING_ENTRY_FIELD_GET(
                devObjPtr, regPtr, entryOffset,
                SMEM_LION3_PLR_METERING_TABLE_FIELDS_MAX_BURST_SIZE0);

            max_bucket_size1 = SMEM_LION3_PLR_METERING_ENTRY_FIELD_GET(
                devObjPtr, regPtr, entryOffset,
                SMEM_LION3_PLR_METERING_TABLE_FIELDS_MAX_BURST_SIZE1);

            rate_type0 = SMEM_LION3_PLR_METERING_ENTRY_FIELD_GET(
                devObjPtr, regPtr, entryOffset,
                SMEM_LION3_PLR_METERING_TABLE_FIELDS_RATE_TYPE0);

            rate_type1 = SMEM_LION3_PLR_METERING_ENTRY_FIELD_GET(
                devObjPtr, regPtr, entryOffset,
                SMEM_LION3_PLR_METERING_TABLE_FIELDS_RATE_TYPE1);

            bytesFactor0 = 1 << (3*rate_type0);/*8^rate_type0;*/ /*1,8,64,512,4K,32K */
            bytesFactor1 = 1 << (3*rate_type1);/*8^rate_type1;*/ /*1,8,64,512,4K,32K */

            bucket_size0 = max_bucket_size0 * bytesFactor0;
            bucket_size1 = max_bucket_size1 * bytesFactor1;

            SMEM_LION3_PLR_METERING_ENTRY_FIELD_SET(
                devObjPtr, regPtr, entryOffset,
                SMEM_LION3_PLR_METERING_TABLE_FIELDS_BUCKET_SIZE0,
                bucket_size0);

            SMEM_LION3_PLR_METERING_ENTRY_FIELD_SET(
                devObjPtr, regPtr, entryOffset,
                SMEM_LION3_PLR_METERING_TABLE_FIELDS_BUCKET_SIZE1,
                bucket_size1);

            if(SMEM_CHT_IS_SIP5_15_GET(devObjPtr))
            {
                meterCfgEntryAddr   = SMEM_SIP5_15_POLICER_CONFIG_ENTRY_TBL_MEM(
                    devObjPtr, meterMemoryCycle, entryOffset + meterBaseAddr);
                regPtr = smemMemGet(devObjPtr, meterCfgEntryAddr);

                /* envelope entry has at least one of these values not zero */
                policer_mode  = SMEM_SIP5_15_PLR_METERING_CONFIG_ENTRY_FIELD_GET(
                    devObjPtr, regPtr, entryOffset,
                    SMEM_SIP5_15_PLR_METERING_CONFIG_TABLE_FIELDS_POLICER_MODE_E);

                if ((policer_mode != SNET_LION3_POLICER_METERING_MODE_MEF10_3_START_E) &&
                    (policer_mode != SNET_LION3_POLICER_METERING_MODE_MEF10_3_NOT_START_E))
                {
                    /* not member of metering envelope */
                    regAddr = SMEM_XCAT_POLICER_MRU_REG(devObjPtr,cycle);
                    smemRegFldGet(devObjPtr, regAddr, 0, 32, &policerMru);

                    /* update metering conformance level sign table fields in table entry */
                    regPtr = smemMemGet(devObjPtr,
                            SMEM_SIP5_15_POLICER_METERING_CONFORMANCE_LEVEL_SIGN_TBL_MEM(
                                devObjPtr, cycle, entryOffset + meterBaseAddr));
                    tokenBucket0Sign = SIP_5_15_CONF_SIGN_CALC(bucket_size0,policerMru);
                    tokenBucket1Sign = SIP_5_15_CONF_SIGN_CALC(bucket_size1,policerMru);
                    SMEM_U32_SET_FIELD(*regPtr, 0, 1, tokenBucket0Sign);
                    SMEM_U32_SET_FIELD(*regPtr, 1, 1, tokenBucket1Sign);

                    __LOG_NO_LOCATION_META_DATA((
                        "update metering conformance level sign table fields in table entry\n"));
                    __LOG_NO_LOCATION_META_DATA((
                        "cycle[%d], entryOffset[%d], tokenBucket0Sign[%d], tokenBucket1Sign[%d] \n",
                        cycle, entryOffset, tokenBucket1Sign,tokenBucket0Sign));
                }
            }
        }
        break;

    /* Refresh the metering entry status fields */
    case 5:
        break;

    case 8:
        /* Counter entry will be copied to the Policer Table Access Data
           Registers and content of Policer Table Access Data Registers
           will be written to counting entry .*/
        entryAddr = counterEntryAddr;
        entryPtr = smemMemGet(devObjPtr, entryAddr);
        /* pointer to the registers that CPU read/write for 'indirect access' */
        regAddr = SMEM_XCAT_POLICER_INDIRECT_ACCESS_DATA_ENTRY_REG(devObjPtr, cycle, 0);
        regPtr = smemMemGet(devObjPtr, regAddr);

        /* store content of Policer Table Access Data Registers */
        memcpy(entry, regPtr, sizeof(entry));

        /* write counting entry to Policer Table Access Data Registers */
        smemMemSet(devObjPtr, regAddr, entryPtr, (devObjPtr->tablesInfo.policerCounters.paramInfo[0].step / 4));

        /* write content of Policer Table Access Data Registers to counting entry */
        smemMemSet(devObjPtr, entryAddr, entry, (devObjPtr->tablesInfo.policerCounters.paramInfo[0].step / 4));
        break;
    default:
        return;
    }

    /* Cleared by the device when the read or write action is completed. */
    SMEM_U32_SET_FIELD(*inMemPtr, 0, 1, 0);

    *memPtr = *inMemPtr;
}

/*******************************************************************************
*  smemXCatActiveReadIPFixNanoTimeStamp
*
* DESCRIPTION:
*    The IPFix Nano Time Of Day register read access.
*
* INPUTS:
*    devObjPtr  - device object PTR.
*    address    - Address for ASIC memory.
*    memPtr     - Pointer to the register's memory in the simulation.
*    memSize    - Size of memory to be written
*    param      - Registers' specific parameter.
*
* OUTPUTS:
*    outMemPtr   - Pointer to the memory to copy register's content.
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void smemXCatActiveReadIPFixNanoTimeStamp
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    OUT        GT_U32 * outMemPtr
)
{
    SNET_TOD_TIMER_STC ipFixTimer;

    smemXCatPolicerIPFixTimeStampUpload(devObjPtr, param, &ipFixTimer);

    /* IPfix nano timer is incremented by the value of time stamp upload registers */
    *outMemPtr = ipFixTimer.nanoSecondTimer;
}

/*******************************************************************************
*  smemXCatActiveReadIPFixSecMsbTimeStamp
*
* DESCRIPTION:
*    The IPFix Seconds MSB Time Of Day register read access.
*
* INPUTS:
*    devObjPtr  - device object PTR.
*    address    - Address for ASIC memory.
*    memPtr     - Pointer to the register's memory in the simulation.
*    memSize    - Size of memory to be written
*    param      - Registers' specific parameter.
*
* OUTPUTS:
*    outMemPtr   - Pointer to the memory to copy register's content.
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void smemXCatActiveReadIPFixSecMsbTimeStamp
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    OUT        GT_U32 * outMemPtr
)
{
    SNET_TOD_TIMER_STC ipFixTimer;

    smemXCatPolicerIPFixTimeStampUpload(devObjPtr, param, &ipFixTimer);

    /* Ouput value is incremented IPfix second timer high word */
    *outMemPtr = ipFixTimer.secondTimer.l[1];
}

/*******************************************************************************
*  smemXCatActiveReadIPFixSecLsbTimeStamp
*
* DESCRIPTION:
*    The IPFix Seconds LSB Time Of Day register read access.
*
* INPUTS:
*    devObjPtr  - device object PTR.
*    address    - Address for ASIC memory.
*    memPtr     - Pointer to the register's memory in the simulation.
*    memSize    - Size of memory to be written
*    param      - Registers' specific parameter.
*
* OUTPUTS:
*    outMemPtr   - Pointer to the memory to copy register's content.
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void smemXCatActiveReadIPFixSecLsbTimeStamp
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    OUT        GT_U32 * outMemPtr
)
{
    SNET_TOD_TIMER_STC ipFixTimer;

    smemXCatPolicerIPFixTimeStampUpload(devObjPtr, param, &ipFixTimer);

    /* Ouput value is incremented IPfix second timer low word */
    *outMemPtr = ipFixTimer.secondTimer.l[0];
}

/*******************************************************************************
*  smemXCatActiveReadIPFixSampleLog
*
* DESCRIPTION:
*    The IPFix IPFIX Sample Entries Log read access.
*
* INPUTS:
*    devObjPtr  - device object PTR.
*    address    - Address for ASIC memory.
*    memPtr     - Pointer to the register's memory in the simulation.
*    memSize    - Size of memory to be written
*    param      - Registers' specific parameter.
*
* OUTPUTS:
*    outMemPtr   - Pointer to the memory to copy register's content.
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void smemXCatActiveReadIPFixSampleLog
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    OUT        GT_U32 * outMemPtr
)
{
    /* Copy registers content to the output memory */
    *outMemPtr = *memPtr;
    /* Clear register after read */
    *memPtr = 0;
}

/*******************************************************************************
*  smemXCatActiveReadPolicerManagementCounters
*
* DESCRIPTION:
*    Policer : when Read a Data Unit counter part(Entry LSB) of Management Counters Entry.
*              then value of LSB and MSB copied to Shadow registers
*
* INPUTS:
*    devObjPtr  - device object PTR.
*    address    - Address for ASIC memory.
*    memPtr     - Pointer to the register's memory in the simulation.
*    memSize    - Size of memory to be written
*    param      - Registers' specific parameter.
*
* OUTPUTS:
*    outMemPtr   - Pointer to the memory to copy register's content.
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void smemXCatActiveReadPolicerManagementCounters
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    OUT        GT_U32 * outMemPtr
)
{
    GT_U32  policerBaseAddress;/* policer base address */
    GT_U32 regAddr;             /* memory address */

    /* Copy registers content to the output memory */
    *outMemPtr = *memPtr;

    if (!SKERNEL_IS_XCAT_REVISON_A1_DEV(devObjPtr))
    {   /* ch3 , xcat a0 ,lion A*/
        if((address & 0x70) > (0x58))
        {
            /* address match the mask of : POLICER_MANAGEMENT_COUNTER_MASK_CNS
               but it is not part of the management counters active memory */
            return;
        }
        policerBaseAddress =  (address >> 23) << 23;/* policer base address */
        regAddr = policerBaseAddress + 0x00000108;
    }
    else
    {
        if((address & 0xF0) > (0xB0))
        {

            /* address match the mask of : POLICER_MANAGEMENT_COUNTER_MASK_CNS
               but it is not part of the management counters active memory */
            return;
        }

        /* get register Policer Shadow<n> Register (n=01) */
        regAddr = SMEM_XCAT_POLICER_MANAGEMENT_COUNTERS_POLICER_SHADOW_REG(devObjPtr,param);
    }

    /* copy the content of the next 2 word into the memory of :
       Policer Shadow<n> Register (n=01) */
    smemMemSet(devObjPtr,regAddr , &memPtr[1] , 2);

    return;
}


/*******************************************************************************
*  smemXCatActiveReadIeeeMcConfReg
*
* DESCRIPTION:
*    IEEE Reserved Multicast Configuration register reading.
*    Reading of IEEE Reserved Multicast Configuration registers
*    should be performed from:
*    0x200X820 instead of 0x200X810 (table 2)
*    0x200X828 instead of 0x200X818 (table 3) - for xCat A1 and above
*
* INPUTS:
*    devObjPtr  - device object PTR.
*    address    - Address for ASIC memory.
*    memPtr     - Pointer to the register's memory in the simulation.
*    memSize    - Size of memory to be written
*    param      - Registers' specific parameter.
*
* OUTPUTS:
*    outMemPtr   - Pointer to the memory to copy register's content.
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void smemXCatActiveReadIeeeMcConfReg
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    OUT        GT_U32 * outMemPtr
)
{
    GT_U32 regAddr;             /* memory address */

    if (devObjPtr->errata.ieeeReservedMcConfigRegRead)
    {
        /* get value of IEEE Reserved Multicast Configuration register */
        regAddr = address - 0x10;
        smemRegGet(devObjPtr,regAddr,outMemPtr);
    }
    else
    {
        *outMemPtr =  *memPtr;
    }
}

static GT_U32 getIplrMemoryStep
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address
)
{
    GT_BIT          isMeter = 0 , isCounter = 0 , isMeterConfig = 0 /*, isConfLevelSign = 0*/;
    GT_U32          unitMask = SMEM_CHT_IS_SIP5_GET(devObjPtr) ? 0x00FFFFFF : 0x007FFFFF;
    GT_U32          relativeAddr = address & unitMask;
    GT_U32          masked_meterTblBaseAddr = devObjPtr->policerSupport.meterTblBaseAddr & unitMask;
    GT_U32          masked_countTblBaseAddr = devObjPtr->policerSupport.countTblBaseAddr & unitMask;
    GT_U32          masked_meterConfigTblBaseAddr = devObjPtr->policerSupport.meterConfigTblBaseAddr & unitMask;
    GT_U32          masked_policerConformanceLevelSignTblBaseAddr = devObjPtr->policerSupport.policerConformanceLevelSignTblBaseAddr & unitMask;
    GT_U32      memoryStep;

    if(relativeAddr >= masked_meterTblBaseAddr &&
        relativeAddr < masked_countTblBaseAddr)
    {
        isMeter = 1;
    }
    else
    if(relativeAddr >= masked_countTblBaseAddr)
    {
        if(SMEM_CHT_IS_SIP5_15_GET(devObjPtr) &&
           relativeAddr >= masked_meterConfigTblBaseAddr)
        {
            if(relativeAddr >= masked_policerConformanceLevelSignTblBaseAddr)
            {
                /*isConfLevelSign = 1;*/
            }
            else
            {
                isMeterConfig = 1;
            }
        }
        else
        {
            isCounter = 1;
        }
    }

    if(isMeter)
    {
        memoryStep = devObjPtr->tablesInfo.policer.paramInfo[0].step;
    }
    else
    if(isCounter)
    {
        memoryStep = devObjPtr->tablesInfo.policerCounters.paramInfo[0].step;
    }
    else
    if(isMeterConfig)
    {
        memoryStep = devObjPtr->tablesInfo.policerConfig.paramInfo[0].step;
    }
    else /*isConfLevelSign*/
    {
        memoryStep = devObjPtr->tablesInfo.policerConformanceLevelSign.paramInfo[0].step;
    }

    return memoryStep;
}

/*******************************************************************************
*  smemBobKIsPlrMemoryShared
*
* DESCRIPTION:
*    Returns isShared flag for Policer memories.
*
* INPUTS:
*    devObjPtr  - device object PTR.
*    address    - Address for ASIC memory.
*
* OUTPUTS:
*    GT_TRUE  - memory is shared between IPLR0 and IPLR1
*    GT_FALSE - not shared
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static GT_U32 smemBobKIsPlrMemoryShared
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address
)
{
    DECLARE_FUNC_NAME(smemBobKIsPlrMemoryShared);
    GT_U32          isSharedMemory;   /* is memory shared between IPLR0 and IPLR1 */
    GT_U32          mask;             /* mask to get memory relative base address */
    GT_U32          countersBaseAddr; /* relative base address of counting memory  */

    if(SMEM_CHT_IS_SIP5_15_GET(devObjPtr))
    {
        /* meterring tables became shared between IPLR0 and IPLR1 starting from SIP_5_15 (BobK).
           counting memory works as before.
           Direct access to shared memories done as for usual memories. */
        if(SMEM_CHT_IS_SIP5_20_GET(devObjPtr))
        {
            mask = 0x00F80000;
            countersBaseAddr = 0x200000;
        }
        else
        {
            mask = 0x00FC0000;
            countersBaseAddr = 0x140000;
        }

        isSharedMemory = ((address & mask) == countersBaseAddr) ? 0 : 1;
    }
    else
    {
        isSharedMemory = 0;
    }

    __LOG(("PLR memory access isSharedMemory flag is %d\n", isSharedMemory));
    return isSharedMemory;
}

/*******************************************************************************
*  smemXCatActiveReadIplr0Tables
*
* DESCRIPTION:
*    Read iplr0 metering, counting tables (tablesInfo.policer, tablesInfo.policerCounters).
*         Redirected to "end of" iplr0 tables (lower part) if needed. See Policer Memory Control
*
* INPUTS:
*    devObjPtr  - device object PTR.
*    address    - Address for ASIC memory.
*    memPtr     - Pointer to the register's memory in the simulation.
*    memSize    - Size of memory to be written
*    param      - Registers' specific parameter.
*
* OUTPUTS:
*    outMemPtr   - Pointer to the memory to copy register's content.
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void smemXCatActiveReadIplr0Tables
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    OUT        GT_U32 * outMemPtr
)
{
    /* Active memory of IPLR-0. This code covers metering and counting
    because offsets are 'relative', even using only 'policer' table */
    GT_U32         newAddr;
    GT_U32        *regPtr;
    GT_U32          memoryStep;
    GT_U32          isSharedMemory;   /* is memory shared between IPLR0 and IPLR1 */

    if (SKERNEL_DEVICE_FAMILY_XCAT2_ONLY(devObjPtr))
    {
        if ((address & 0x0000FFFF) > POLICER_XCAT2_SIZE_CNS)
        {
            /* copy  to the output memory (out of active memory range) */
            memcpy(outMemPtr, memPtr, memSize * sizeof(GT_U32));
            return;
        }
    }

    isSharedMemory = smemBobKIsPlrMemoryShared(devObjPtr, address);

    newAddr = address;
    if (devObjPtr->policerSupport.iplr1EntriesFirst && (0 == isSharedMemory))
    {
        memoryStep = getIplrMemoryStep(devObjPtr,address);

        /* policer0 entries are after policer1 entries - do shift addr by length of policer1 part */
        newAddr += devObjPtr->policerSupport.iplr1TableSize * memoryStep;

        /* pointer to simulation memory of iplr0 table (policer or policerCounters) */
        regPtr = smemMemGet(devObjPtr, newAddr);
        /* copy iplr0 table contents to the output memory */
        memcpy(outMemPtr, regPtr, memSize * sizeof(GT_U32));
    }
    else
    {
        memcpy(outMemPtr, memPtr, memSize * sizeof(GT_U32));
    }

}

/*******************************************************************************
*  smemXCatActiveWriteIplr0Tables
*
* DESCRIPTION:
*    Write to iplr0 metering, counting tables (tablesInfo.policer, tablesInfo.policerCounters).
*         Redirected to "end of" iplr0 tables (lower part) if needed. See Policer Memory Control
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
void smemXCatActiveWriteIplr0Tables
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
)
{
    /* Active memory of IPLR-0. This code covers metering and counting
    because offsets are 'relative', even using only 'policer' table */
    GT_U32         newAddr;
    GT_U32          *targetMemoryPtr;
    GT_U32          memoryStep;
    GT_U32          isSharedMemory;   /* is memory shared between IPLR0 and IPLR1 */

    if (SKERNEL_DEVICE_FAMILY_XCAT2_ONLY(devObjPtr))
    {
        if ((address & 0x0000FFFF) > POLICER_XCAT2_SIZE_CNS)
        {
            /* not active memory, write as usual memory */
            memcpy(memPtr, inMemPtr, memSize * sizeof(GT_U32));
            return;
        }
    }

    isSharedMemory = smemBobKIsPlrMemoryShared(devObjPtr, address);

    newAddr = address;
    if (devObjPtr->policerSupport.iplr1EntriesFirst && (0 == isSharedMemory))    
    {
        memoryStep = getIplrMemoryStep(devObjPtr,address);

        /* policer0 entries are after policer1 entries - do shift addr by length of policer1 part */
        newAddr += devObjPtr->policerSupport.iplr1TableSize * memoryStep;

        /* pointer to simulation memory of iplr0 table (policer or policerCounters) */
        targetMemoryPtr = smemMemGet(devObjPtr, newAddr);
        /* write input memory to the iplr0 table */
    }
    else
    {
        targetMemoryPtr = memPtr;
    }

    memcpy(targetMemoryPtr, inMemPtr, memSize * sizeof(GT_U32));

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        smemLion3ActiveWriteMeterEntry(devObjPtr,targetMemoryPtr,newAddr,0/*plr0*/);
    }
}

/*******************************************************************************
*  smemXCatActiveReadIplr1Tables
*
* DESCRIPTION:
*    Read iplr1 metering, counting tables (tablesInfo.policer, tablesInfo.policerCounters).
*         Redirected to iplr0 tables. See Policer Memory Control
*
* INPUTS:
*    devObjPtr  - device object PTR.
*    address    - Address for ASIC memory.
*    memPtr     - Pointer to the register's memory in the simulation.
*    memSize    - Size of memory to be written
*    param      - Registers' specific parameter.
*
* OUTPUTS:
*    outMemPtr   - Pointer to the memory to copy register's content.
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void smemXCatActiveReadIplr1Tables
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    OUT        GT_U32 * outMemPtr
)
{
    /* Active memory of IPLR-1. This code covers metering and counting
    because offsets are 'relative', even using only 'policer' table */
    GT_U32         addrOffset, newAddr;
    GT_U32        *regPtr;
    GT_U32          memoryStep;

    if (SKERNEL_DEVICE_FAMILY_XCAT2_ONLY(devObjPtr))
    {
        if ((address & 0x0000FFFF) > POLICER_XCAT2_SIZE_CNS)
        {
            /* copy  to the output memory (out of active memory range) */
            memcpy(outMemPtr, memPtr, memSize * sizeof(GT_U32));
            return;
        }
    }

    if (devObjPtr->tablesInfo.policer.paramInfo[1].step == SMAIN_NOT_VALID_CNS)
    {
        skernelFatalError( "smemXCatActiveReadIplr1Tables: iplr1 have no memory. Check Policer Memory Control configuration \n");
    }

    addrOffset  = address - devObjPtr->memUnitBaseAddrInfo.policer[1];
    if (!devObjPtr->policerSupport.iplr1EntriesFirst)
    {
        memoryStep = getIplrMemoryStep(devObjPtr,address);
        /* policer1 entries are after policer0 entries - do shift addr by length of policer0 part */
        addrOffset += devObjPtr->policerSupport.iplr0TableSize * memoryStep;
    }
    newAddr = addrOffset + devObjPtr->memUnitBaseAddrInfo.policer[0];

    /* pointer to simulation memory of iplr0 table (policer or policerCounters) */
    regPtr = smemMemGet(devObjPtr, newAddr);
    /* copy iplr0 table contents to the output memory */
    memcpy(outMemPtr, regPtr, memSize * sizeof(GT_U32));
}

/*******************************************************************************
*  smemXCatActiveWriteIplr1Tables
*
* DESCRIPTION:
*    Write to iplr1 metering, counting tables (tablesInfo.policer, tablesInfo.policerCounters).
*         Redirected to iplr0 tables. See Policer Memory Control
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
void smemXCatActiveWriteIplr1Tables
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
)
{
    /* Active memory of IPLR-1. This code covers metering and counting
       because offsets are 'relative', even using only 'policer' table */
    GT_U32         addrOffset, newAddr;
    GT_U32        *regPtr;
    GT_U32          memoryStep;

    if (SKERNEL_DEVICE_FAMILY_XCAT2_ONLY(devObjPtr))
    {
        if ((address & 0x0000FFFF) > POLICER_XCAT2_SIZE_CNS)
        {
            /* not active memory, write as usual memory */
            memcpy(memPtr, inMemPtr, memSize * sizeof(GT_U32));
            return;
        }
    }

    if (devObjPtr->tablesInfo.policer.paramInfo[1].step == SMAIN_NOT_VALID_CNS)
    {
         skernelFatalError( "smemXCatActiveWriteIplr1Tables: iplr1 have no memory. Check Policer Memory Control configuration \n");
    }

    addrOffset  = address - devObjPtr->memUnitBaseAddrInfo.policer[1];
    if (!devObjPtr->policerSupport.iplr1EntriesFirst)
    {
        memoryStep = getIplrMemoryStep(devObjPtr,address);
        /* policer1 entries are after policer0 entries - do shift addr by length of policer0 part */
        addrOffset += devObjPtr->policerSupport.iplr0TableSize * memoryStep;
    }
    newAddr = addrOffset + devObjPtr->memUnitBaseAddrInfo.policer[0];

    /* pointer to simulation memory of iplr0 table (policer or policerCounters) */
    regPtr = smemMemGet(devObjPtr, newAddr);
    /* write input memory to the iplr0 table */
    memcpy(regPtr, inMemPtr, memSize * sizeof(GT_U32));


    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        smemLion3ActiveWriteMeterEntry(devObjPtr,regPtr,newAddr,1/*plr1*/);
    }

}

/*******************************************************************************
*  smemXCatActiveWritePolicerMemoryControl
*
* DESCRIPTION:
*    Set the Policer Control0 register in iplr0, and update Policer Memory Control
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
void smemXCatActiveWritePolicerMemoryControl
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

    /* get "Policer Memory Control" field - bits 12:11 */
    fldValue = SMEM_U32_GET_FIELD(*inMemPtr, 11, 2);
    switch (fldValue)
    {
        case 0: /* PLR0_UP_PLR1_LW:Policer 0 uses the upper memory (1792) Policer 1 uses the lower memory (256) */
            devObjPtr->tablesInfo.policer.commonInfo.baseAddress = meterBase;
            devObjPtr->tablesInfo.policer.paramInfo[1].step = POLICER_MEMORY_1792_CNS * meterEntrySize;
            devObjPtr->tablesInfo.policerCounters.commonInfo.baseAddress = countBase;
            devObjPtr->tablesInfo.policerCounters.paramInfo[1].step = POLICER_MEMORY_1792_CNS * countEntrySize;
            devObjPtr->policerSupport.iplr0TableSize = POLICER_MEMORY_1792_CNS;
            devObjPtr->policerSupport.iplr1TableSize = POLICER_MEMORY_256_CNS;
            devObjPtr->policerSupport.iplr1EntriesFirst = 0;
            break;
        case 1: /* PLR0_UP_AND_LW:Policer 0 uses both memories */
            devObjPtr->tablesInfo.policer.commonInfo.baseAddress = meterBase;
            devObjPtr->tablesInfo.policer.paramInfo[1].step = SMAIN_NOT_VALID_CNS;
            devObjPtr->tablesInfo.policerCounters.commonInfo.baseAddress = countBase;
            devObjPtr->tablesInfo.policerCounters.paramInfo[1].step = SMAIN_NOT_VALID_CNS;
            devObjPtr->policerSupport.iplr0TableSize = 2048;
            devObjPtr->policerSupport.iplr1TableSize = 0;
            devObjPtr->policerSupport.iplr1EntriesFirst = 0;
            break;
        case 2: /* PLR1_UP_AND_LW:Policer 1 uses both memories */
            devObjPtr->tablesInfo.policer.commonInfo.baseAddress = SMAIN_NOT_VALID_CNS;
            devObjPtr->tablesInfo.policer.paramInfo[1].step = meterBase - SMAIN_NOT_VALID_CNS;
            devObjPtr->tablesInfo.policerCounters.commonInfo.baseAddress = SMAIN_NOT_VALID_CNS;
            devObjPtr->tablesInfo.policerCounters.paramInfo[1].step = countBase - SMAIN_NOT_VALID_CNS;
            /* Note: (commonInfo.baseAddress + paramInfo[1].step) now points to start of table */
            devObjPtr->policerSupport.iplr0TableSize = 0;
            devObjPtr->policerSupport.iplr1TableSize = 2048;
            devObjPtr->policerSupport.iplr1EntriesFirst = 0;
            break;
        case 3: /* PLR1_UP_PLR0_LW:Policer 1 uses the upper memory (1792) Policer 0 uses the lower memory (256) */
            devObjPtr->tablesInfo.policer.commonInfo.baseAddress = meterBase + POLICER_MEMORY_1792_CNS * meterEntrySize;
            devObjPtr->tablesInfo.policer.paramInfo[1].step = 0 - (POLICER_MEMORY_1792_CNS * meterEntrySize);
            devObjPtr->tablesInfo.policerCounters.commonInfo.baseAddress = countBase + POLICER_MEMORY_1792_CNS * countEntrySize;
            devObjPtr->tablesInfo.policerCounters.paramInfo[1].step = 0 - (POLICER_MEMORY_1792_CNS * countEntrySize);
            devObjPtr->policerSupport.iplr0TableSize = POLICER_MEMORY_256_CNS;
            devObjPtr->policerSupport.iplr1TableSize = POLICER_MEMORY_1792_CNS;
            devObjPtr->policerSupport.iplr1EntriesFirst = 1;
            break;
        default:
            break;
    }
}

/*******************************************************************************
*  smemXCatActiveWriteIPFixTimeStamp
*
* DESCRIPTION:
*    The Time Of Day register write access.
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
void smemXCatActiveWriteIPFixTimeStamp
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
)
{
    GT_U32 * regPtr;            /* memory data pointer */
    GT_U32 regAddr;             /* memory address */
    GT_U32 cycle;               /* policer cycle */
    GT_U32 cycleStart,cycleEnd; /* start/end policer cycle */
    GT_U32 increment;           /* Time stamp upload mode */
    GT_U32 timerData[3];        /* Upload timer data */
    GT_U32 uploadTrigger;       /* Upload timer trigger */
    GT_U32 nanoSeconds = 0;     /* IPfix nano timer value */
    GT_U32 seconds = 0;         /* IPfix second timer */
    GT_U32 * ctrlRegDataPtr;    /* pointer IPFIX Control data */
    GT_U64 seconds64;           /* 64 bits seconds value */
    SNET_TOD_TIMER_STC ipFixTimer; /* IPFix timer */
    GT_U32 clocks;              /* TOD value in clocks */

    *memPtr = *inMemPtr;

    seconds64.l[0] = seconds64.l[1] = 0;

    uploadTrigger = SMEM_U32_GET_FIELD(*inMemPtr, 6, 1);
    /* Update the IPfix time stamp according to the time stamp upload mode */
    if(uploadTrigger)
    {
        /* Get current TOD clock value and convert seconds to 64 bits value */
        clocks = SNET_XCAT_IPFIX_TOD_CLOCK_GET_MAC(devObjPtr, 0);
        SNET_TOD_CLOCK_FORMAT_MAC(clocks, seconds, nanoSeconds);

        seconds64.l[0] = (seconds & 0xffff);
        seconds64.l[1] = (seconds >> 16);

        cycle = param;
        if(cycle == 0)
        {
            /* NOTE:
               This bit in IPLR0 controls the IPLR1 and EPLR as well.
               The upload mode and upload values are taken in each PLR from its local RegFile */
            cycleStart = 0;
            cycleEnd = 3;
        }
        else
        {
            /* impact only local unit */
            cycleStart = cycle;
            cycleEnd   = cycle + 1;
        }


        for(cycle = cycleStart; cycle < cycleEnd; cycle++)
        {
            /* Policer IPFIX Control */
            regAddr =
                SMEM_XCAT_POLICER_IPFIX_CTRL_REG(devObjPtr, cycle);

            /* IPLR0/IPLR1/EPLR control */
            ctrlRegDataPtr = smemMemGet(devObjPtr, regAddr);

            /* Time stamp upload mode: 0 - absolute, 1 - incremental */
            increment = SMEM_U32_GET_FIELD(*ctrlRegDataPtr, 7, 1);

            /* IPFIX nano timer stamp upload */
            regAddr =
                SMEM_XCAT_POLICER_IPFIX_NANO_TS_UPLOAD_REG(devObjPtr,
                                                                    cycle);
            regPtr = smemMemGet(devObjPtr, regAddr);

            /* Copy IPFix timer data: nano timer, second LSb timer, second MSb timer */
            memcpy(timerData, regPtr, sizeof(timerData));

            /* Policer Timer Memory */
            regAddr = SMEM_XCAT_POLICER_TIMER_TBL_MEM(devObjPtr,  cycle, 6);
            regPtr = smemMemGet(devObjPtr, regAddr);

            /* Nano timer */
            ipFixTimer.nanoSecondTimer = timerData[0];

            /* Convert second timer data to 64 bit value */
            ipFixTimer.secondTimer.l[0] = timerData[1];
            ipFixTimer.secondTimer.l[1] = timerData[2];

            if(increment)
            {
                /* value of LS word before the update */
                GT_U32  oldLsValue = ipFixTimer.secondTimer.l[0];
                /* value of MS word before the update */
                GT_U32  oldMsValue = ipFixTimer.secondTimer.l[1];

                ipFixTimer.nanoSecondTimer += nanoSeconds;

                if(ipFixTimer.nanoSecondTimer >= NANO_SEC_MAX_VALUE_CNS)
                {
                    GT_U64  oneSecond;

                    oneSecond.l[0] = 1;
                    oneSecond.l[1] = 0;

                    /* the nano seconds wrap ...
                        1. reduce the 10^9 nanos from the value.
                        2. give carry to the seconds
                    */
                    ipFixTimer.nanoSecondTimer -= NANO_SEC_MAX_VALUE_CNS;

                    ipFixTimer.secondTimer =
                        prvSimMathAdd64(ipFixTimer.secondTimer, oneSecond);
                }

                ipFixTimer.secondTimer =
                    prvSimMathAdd64(ipFixTimer.secondTimer, seconds64);

                if(devObjPtr->errata.ipfixTodIncrementModeWraparoundIssues)
                {
                    /* check for LS word wraparound */
                    if(ipFixTimer.secondTimer.l[0] < oldLsValue)
                    {
                        /* the LS word did wrap around */
                        /* but due to the erratum the carry should not go
                           to the MS word  !!! */
                        ipFixTimer.secondTimer.l[1] --;

                        /* the LS word get the carry instead of MS word !!! */
                        ipFixTimer.secondTimer.l[0] ++;
                    }
                    else
                    /* check for MS word wraparound (when LS word did not wraparound) */
                    if(ipFixTimer.secondTimer.l[1] < oldMsValue)
                    {
                        /* the MS word did wrap around */
                        /* but due to the erratum the wrap around should be
                           added 1 !!! */
                        ipFixTimer.secondTimer.l[1] ++;
                    }
                }
            }

            /* IPfix timer is uploaded to the absolute/incremented value of time stamp upload registers */
            regPtr[0] = ipFixTimer.nanoSecondTimer;
            regPtr[1] = ipFixTimer.secondTimer.l[0];
            regPtr[2] = ipFixTimer.secondTimer.l[1];

            /* Cleared by the device when the write action is completed */
            SMEM_U32_SET_FIELD(*ctrlRegDataPtr, 6, 1, 0);

            /* Set IPFix timestamp TOD offset */
            devObjPtr->ipFixTimeStampClockDiff[cycle] = SIM_OS_MAC(simOsTickGet)();
        }
    }
}

/*******************************************************************************
*  smemXCatActiveWriteLogTargetMap
*
* DESCRIPTION:
*    The Logical Target Mapping Table write access.
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
void smemXCatActiveWriteLogTargetMap
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
)
{
    GT_U32     lineNumber;      /* line number in Logical Target Mapping Table */
    GT_U32     rdWr;            /* read/write operation */
    GT_U32     regAddr;         /* register address */
    GT_U32    *regPtr;          /* register pointer */
    GT_U32    *logTrgMapPtr;    /* pointer to logical target mapping table entry */

    /* Action 0 - read , 1 - write */
    rdWr = SMEM_U32_GET_FIELD(*inMemPtr, 1, 1);
    /* Line Number */
    if(devObjPtr->supportLogicalMapTableInfo.supportFullRange)
    {
        /* the device supports all 32 (0..31 not only 24..31) devices for this table */
        lineNumber = SMEM_U32_GET_FIELD(*inMemPtr, 2, 11);/*6 bits port , 5 bits device */
    }
    else
    {
        /* the device supports only 8 (24..31) devices for the table */
        lineNumber = SMEM_U32_GET_FIELD(*inMemPtr, 2, 9);
    }

    /* Logical Target Mapping Table */
    regAddr = SMEM_XCAT_LOGICAL_TRG_MAPPING_TBL_MEM(devObjPtr,
                                                   (lineNumber >> 6),
                                                   (lineNumber & 0x3f));
    logTrgMapPtr = smemMemGet(devObjPtr, regAddr);

    /* Logical Target Mapping Table Data Access */
    regAddr = SMEM_XCAT_LOGICAL_TRG_MAPPING_TBL_DATA_REG(devObjPtr);
    regPtr = smemMemGet(devObjPtr, regAddr);

    if (rdWr == 0)
    {
        /* Read Logical Target Mapping entry */
        *regPtr = *logTrgMapPtr;
    }
    else
    {
        /* Write Logical Target Mapping entry */
        *logTrgMapPtr = *regPtr;
    }

    if(devObjPtr->tablesInfo.logicalTrgMappingTable.paramInfo[1].step >= 8)
    {
        /* there is second register for the indirect to use */
        /* Logical Target Mapping Table Data Access2 */
        regAddr = SMEM_XCAT_LOGICAL_TRG_MAPPING_TBL_DATA2_REG(devObjPtr);
        regPtr = smemMemGet(devObjPtr, regAddr);

        if (rdWr == 0)
        {
            /* Read Logical Target Mapping entry */
            *regPtr = logTrgMapPtr[1];
        }
        else
        {
            /* Write Logical Target Mapping entry */
            logTrgMapPtr[1] = *regPtr;
        }
    }

    /* Cleared by the device when the read or write action is completed. */
    *memPtr = *inMemPtr & (~1);
}

/*******************************************************************************
*  smemXCatActiveWritePolicyTcamConfig_0
*
* DESCRIPTION:
*    set the register of PCL TCAM configuration
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
void smemXCatActiveWritePolicyTcamConfig_0
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
)
{
    GT_U32     tcamSize;  /* */
    GT_U32     reduceNumOfEntries;/* number of entries to reduce from the max support */

    /* set register content */
    *memPtr = *inMemPtr ;

    tcamSize = SMEM_U32_GET_FIELD(*inMemPtr, 1, 4);

    switch(tcamSize)
    {
        case 0:/*Full Tcam Used*/
            reduceNumOfEntries = 0;
            break;
        case 1:/*Full Tcam Minus 1K entries*/
            reduceNumOfEntries = 1*1024;
            break;
        case 2:/*Full Tcam Minus 2K entries*/
            reduceNumOfEntries = 2*1024;
            break;
        default:/* not described in the documentation */
            /*simulation will treat it as Full Tcam Minus 3K entries*/
            reduceNumOfEntries = 3*1024;
            break;
    }

    if(reduceNumOfEntries > devObjPtr->pclTcamMaxNumEntries)
    {
        reduceNumOfEntries = devObjPtr->pclTcamMaxNumEntries;
    }

    devObjPtr->pclTcamInfoPtr->bankEntriesNum =
            (devObjPtr->pclTcamMaxNumEntries - reduceNumOfEntries) / 4;


    return;

}

/*******************************************************************************
*  smemXCatActiveWriteTtiInternalMetalFix
*
* DESCRIPTION:
*    Set the register of TTI Internal, Metal Fix
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
void smemXCatActiveWriteTtiInternalMetalFix
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

    /* set register content */
    *memPtr = *inMemPtr ;

    if (devObjPtr->errata.supportSrcTrunkToCpuIndicationEnable)
    {
        /* Disable Routed Bug Fix */
        fldValue = SMEM_U32_GET_FIELD(*inMemPtr, 17, 1);
        /* Enable/Disable simulation of erratum:
        "Wrong Trunk-ID/Source Port Information of Packet to CPU" */
        devObjPtr->errata.srcTrunkToCpuIndication = (fldValue) ? 0 : 1;
        /*If enable, the port isolation take its members from the source port and not from the source trunk */
        devObjPtr->errata.srcTrunkPortIsolationAsSrcPort =
            !devObjPtr->errata.srcTrunkToCpuIndication;
    }
}

/*******************************************************************************
*  smemCht3ReadWriteRouterTTTcam
*
* DESCRIPTION:
*      Handler for read/write  policy TCAM memory.
*
* INPUTS:
*       devObjPtr       -
*       writeAction     - GT_TRUE - write action , GT_FALSE - read action
*       rtLineNumber    -
*       rtDataType      -
*       rtBankEnBmp     -  bmp of valid banks
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
static void smemCht3ReadWriteRouterTTTcam
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_BOOL  writeAction,
    IN         GT_U32  rtLineNumber,
    IN         GT_U32  rtDataType,
    IN         GT_U32  rtBankEnBmp,
    IN         GT_U32  rtValidBit ,
    IN         GT_U32  rtCompMode,
    IN         GT_U32  rtSpare
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
    GT_U32  spareBit;

    if(writeAction == GT_TRUE)
    {
        if (rtDataType == 0)
        {
            /* Write is done to the data part of the Router TCAM and CTRL entries */
            tcamRegAddr0 = SMEM_CHT3_TCAM_X_DATA_TBL_MEM(devObjPtr,rtLineNumber);
            ctrlRegAddr0 = SMEM_CHT3_TCAM_ROUTER_TT_X_CONTROL_TBL_MEM(devObjPtr,rtLineNumber);
        }
        else
        {
            tcamRegAddr0 = SMEM_CHT3_TCAM_Y_DATA_TBL_MEM(devObjPtr,rtLineNumber);
            ctrlRegAddr0 = SMEM_CHT3_TCAM_ROUTER_TT_Y_CONTROL_TBL_MEM(devObjPtr,rtLineNumber);
        }

        /* Read TCAM data register word 0 - long key */
        regPtr = smemMemGet(devObjPtr, SMEM_CHT3_ROUTER_TCAM_ACCESS_DATA_TBL_MEM(devObjPtr,0));

        for (ii = 0; (ii < 4) ; ii++)
        {
           switch (ii)
           {
                case 0x0: /* Write tcam word1 from data 0 and word2 from data2 16 LSB*/
                    word1 =  regPtr[ii];
                    word2 = (regPtr[ii + 1] ) & 0xFFFF;
                break;

           case 0x1: /* Write tcam word1 from data 2 and 3 and word2 from data4 16 MSB*/
                    word1 = 0;
                    word1 =   (((regPtr[ii ] & 0xffff0000) >> 16)
                            |    (((regPtr[ii + 1]) & 0xffff)) << 16);


                    word2 = regPtr[ii + 1 ] >> 16 & 0x0000FFFF;
                break;
                case 0x2: /* Write tcam word1 from data 0 32 bit*/
                    word1 =  regPtr[ii + 1];
                    word2 = (regPtr[ii + 2]) & 0xFFFF;
                break;
           case 0x3: /* Write tcam word3 from data 4 ,5*/
                    word1 = 0;
                    word1 =   (((regPtr[ii + 1] & 0xffff0000) >> 16)
                            |    (((regPtr[ii + 2]) & 0xffff)) << 16);

                    word2 = regPtr[ii + 2] >> 16 & 0x0000FFFF;
                break;

           }
           if ((1 << ii) & rtBankEnBmp)
           {

                smemRegSet(devObjPtr, (tcamRegAddr0 + (devObjPtr->routeTcamInfo.bankWidth * ii)) , word1);
                /*compareBit  = ((rtCompare & (1 << ii)) ? 1 : 0) << 17; */
                validBit    = ((rtValidBit & (1 << ii)) ? 1 : 0);
                compModeBit = (rtCompMode >> (2 * ii)) & 0x3;
                spareBit    = ((rtSpare & (1 << ii)) ? 1 : 0);
                word2 |=  spareBit  << 16 |validBit  << 17 | (compModeBit << 18);

                smemRegSet(devObjPtr , (ctrlRegAddr0 + (devObjPtr->routeTcamInfo.bankWidth * ii)) , word2);
            }
        }
    }
    else
    {
        /* not implemented -- see CQ# 105927 */
    }
}

/*******************************************************************************
*  smemCht3ReadWritePolicyTcam
*
* DESCRIPTION:
*      Handler for read/write router TT TCAM memory.
*
* INPUTS:
*       devObjPtr       - device object PTR.
*       lineNumber      - table line number to be read or write
*       tcamDataType    - Mask/Data part of entry.
*       rdWr            - read/write action. (1 = for write)
*       tcamCompMode    - compare mode
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void smemCht3ReadWritePolicyTcam
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32  lineNumber,
    IN GT_U32  tcamDataType,
    IN GT_U32  rdWr,
    IN GT_U32  tcamCompMode
)
{
    if (rdWr)
    {
        smemCht3WritePolicyTcamData(devObjPtr, lineNumber, tcamDataType,
                                    tcamCompMode);
    }
    else
    {
        smemCht3ReadPolicyTcamData(devObjPtr, lineNumber, tcamDataType);
    }
}

/*******************************************************************************
*  smemCht3WritePolicyTcamData
*
* DESCRIPTION:
*      Handler for write  policy TCAM data memory.
*
* INPUTS:
*       devObjPtr           - device object PTR.
*       lineNumber          - table line number to be read or write.
*       tcamDataType        - TCAM data(=0) or mask(=1) part of entry
*       tcamCompMode        - compare mode
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void smemCht3WritePolicyTcamData
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32  line ,
    IN         GT_U32  type ,
    IN         GT_U32  tcamCompMode
)
{
    GT_U32                  regAddr,*regPtr;
    GT_U32                  data[12];/* 12 words from the user*/
    GT_U32                  ii;
    GT_U32                  regParam1,regParam2;/*registers of extra parameters*/
    GT_U32                  mode=1;/* always - long key */


    smemRegGet(devObjPtr,
        SMEM_CHT3_PCL_ACTION_TCAM_DATA_PARAM1_REG(devObjPtr),
        &regParam1);
    smemRegGet(devObjPtr,
        SMEM_CHT3_PCL_ACTION_TCAM_DATA_PARAM2_REG(devObjPtr),
        &regParam2);
    /*
        The data that is written to the policy TCAM or the action table,
            or the data read from the action table, as follows:
        The following registers are used for a write access of an extended rule
            (48B) to the Policy TCAM as follows:

        Data0 [31:0] <= Array0Word0[31:0] Entry<LineNumber>
        Data1 [15:0] <= Array0Word0[47:32]Entry<LineNumber>

        Data1[31:16] <= Array0Word1[15:0] Entry<LineNumber>
        Data2[31:0] <= Array0Word1[47:16]Entry<LineNumber>

        Data3 [31:0] <= Array0Word2[31:0] Entry<LineNumber>
        Data4 [15:0] <= Array0Word2[47:32]Entry<LineNumber>

        Data4[31:16] <= Array0Word3[15:0] Entry<LineNumber>
        Data5[31:0] <= Array0Word3[47:16]Entry<LineNumber>

        Data6 [31:0] <= Array1Word0[31:0] Entry<LineNumber>
        Data7 [15:0] <= Array1Word0[47:32]Entry<LineNumber>

        Data7[31:16] <= Array1Word1[15:0] Entry<LineNumber>
        Data8[31:0] <= Array1Word1[47:16]Entry<LineNumber>

        Data9 [31:0] <= Array1Word2[31:0] Entry<LineNumber>
        Data10 [15:0] <= Array1Word2[47:32]Entry<LineNumber>

        Data10[31:16] <= Array1Word3[15:0] Entry<LineNumber>
        Data11[31:0] <= Array1Word3[47:16]Entry<LineNumber>
    */

    /* Read TCAM data register word 0 - long key, 6 - short */
    regPtr = smemMemGet(devObjPtr, SMEM_CHT3_PCL_ACTION_TCAM_DATA_TBL_MEM(devObjPtr,
        ((mode == 0) ? 6 : 0)));

    for(ii=0;ii<12;ii++)
    {
        data[ii] = regPtr[ii];
    }

    /* TCAM data or mask / x or y */
    if(type == 1)   /*data*/
        regAddr = SMEM_CHT3_PCL_TCAM_DATA_TBL_MEM(devObjPtr,line, 0);
    else            /*mask*/
        regAddr = SMEM_CHT3_PCL_TCAM_MASK_TBL_MEM(devObjPtr,line, 0);

    regPtr = smemMemGet(devObjPtr, regAddr);

    for(ii = 0 ; ii < 2; ii++)
    {
        if(0 == SMEM_U32_GET_FIELD(regParam1,ii*4,4))
        {
            /* check fields of Enable array number */
            continue;
        }

        regPtr[(ii*4)+0]   = data[(ii*3)+0] ;

        regPtr[(ii*4)+1]   = data[(ii*3)+1] & 0xFFFF;
        /* set control bits */
        regPtr[(ii*4)+1]  |= (SMEM_U32_GET_FIELD(regParam2,(ii*4+0)+ 8,1))<<16; /*spare*/
        regPtr[(ii*4)+1]  |= (SMEM_U32_GET_FIELD(regParam2,(ii*4+0)+ 0,1))<<17; /*valid*/
        regPtr[(ii*4)+1]  |= (SMEM_U32_GET_FIELD(regParam1,(ii*4+0)+16,2))<<18; /*compare mode*/

        regPtr[(ii*4)+2]   = data[(ii*3)+1] >> 16;
        regPtr[(ii*4)+2]  |= data[(ii*3)+2] << 16;

        regPtr[(ii*4)+3]   = data[(ii*3)+2] >> 16;

        /* set control bits */
        regPtr[(ii*4)+3]  |= (SMEM_U32_GET_FIELD(regParam2,(ii*4+1)+ 8,1))<<16; /*spare*/
        regPtr[(ii*4)+3]  |= (SMEM_U32_GET_FIELD(regParam2,(ii*4+1)+ 0,1))<<17; /*valid*/
        regPtr[(ii*4)+3]  |= (SMEM_U32_GET_FIELD(regParam1,(ii*4+1)+16,2))<<18; /*compare mode*/
    }

    /* A long key */
    if (mode)
    {
        /* TCAM data or mask / x or y */
        if(type == 1)   /*data*/
            regAddr = SMEM_CHT3_PCL_TCAM_DATA_TBL_MEM(devObjPtr,line+512, 0);
        else            /*mask*/
            regAddr = SMEM_CHT3_PCL_TCAM_MASK_TBL_MEM(devObjPtr,line+512, 0);

        regPtr = smemMemGet(devObjPtr, regAddr);

        for(ii = 0 ; ii < 2; ii++)
        {
            if(0 == SMEM_U32_GET_FIELD(regParam1,(ii+2)*4,4))
            {
                /* check fields of Enable array number */
                continue;
            }

            regPtr[(ii*4)+0+6]   = data[(ii*3)+0] ;

            regPtr[(ii*4)+1+6]   = data[(ii*3)+1] & 0xFFFF;
            /* set control bits */
            regPtr[(ii*4)+1+6]  |= (SMEM_U32_GET_FIELD(regParam2,(ii*4+2)+ 8,1))<<16; /*spare*/
            regPtr[(ii*4)+1+6]  |= (SMEM_U32_GET_FIELD(regParam2,(ii*4+2)+ 0,1))<<17; /*valid*/
            regPtr[(ii*4)+1+6]  |= (SMEM_U32_GET_FIELD(regParam1,(ii*4+2)+16,2))<<18; /*compare mode*/

            regPtr[(ii*4)+2+6]   = data[(ii*3)+1] >> 16;
            regPtr[(ii*4)+2+6]  |= data[(ii*3)+2] << 16;

            regPtr[(ii*4)+3+6]   = data[(ii*3)+2] >> 16;
            /* set control bits */
            regPtr[(ii*4)+3+6]  |= (SMEM_U32_GET_FIELD(regParam2,(ii*4+3)+ 8,1))<<16; /*spare*/
            regPtr[(ii*4)+3+6]  |= (SMEM_U32_GET_FIELD(regParam2,(ii*4+3)+ 0,1))<<17; /*valid*/
            regPtr[(ii*4)+3+6]  |= (SMEM_U32_GET_FIELD(regParam1,(ii*4+3)+16,2))<<18; /*compare mode*/
        }
    }
}


/*******************************************************************************
*  smemCht3ReadPolicyTcamData
*
* DESCRIPTION:
*      Handler for read/write  policy TCAM memory .
*
* INPUTS:
*       devObjPtr   - device object PTR.
*       line        - table line number to be read or write
*       type        - TCAM data or mask
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void smemCht3ReadPolicyTcamData
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32  line,
    IN         GT_U32  type
)
{
    GT_U32                  regAddr,*regPtr;
    GT_U32                  data[12];
    GT_U32                  ii;
    GT_U32                  mode=1;/* always - long key */

    /*
        The data that is written to the policy TCAM or the action table,
            or the data read from the action table, as follows:
        The following registers are used for a write access of an extended rule
            (48B) to the Policy TCAM as follows:

        Data0 [31:0] <= Array0Word0[31:0] Entry<LineNumber>
        Data1 [15:0] <= Array0Word0[47:32]Entry<LineNumber>

        Data1[31:16] <= Array0Word1[15:0] Entry<LineNumber>
        Data2[31:0] <= Array0Word1[47:16]Entry<LineNumber>

        Data3 [31:0] <= Array0Word2[31:0] Entry<LineNumber>
        Data4 [15:0] <= Array0Word2[47:32]Entry<LineNumber>

        Data4[31:16] <= Array0Word3[15:0] Entry<LineNumber>
        Data5[31:0] <= Array0Word3[47:16]Entry<LineNumber>

        Data6 [31:0] <= Array1Word0[31:0] Entry<LineNumber>
        Data7 [15:0] <= Array1Word0[47:32]Entry<LineNumber>

        Data7[31:16] <= Array1Word1[15:0] Entry<LineNumber>
        Data8[31:0] <= Array1Word1[47:16]Entry<LineNumber>

        Data9 [31:0] <= Array1Word2[31:0] Entry<LineNumber>
        Data10 [15:0] <= Array1Word2[47:32]Entry<LineNumber>

        Data10[31:16] <= Array1Word3[15:0] Entry<LineNumber>
        Data11[31:0] <= Array1Word3[47:16]Entry<LineNumber>
    */

    /* TCAM data or mask / x or y */
    if(type == 1)   /*data*/
        regAddr = SMEM_CHT3_PCL_TCAM_DATA_TBL_MEM(devObjPtr,line, 0);
    else            /*mask*/
        regAddr = SMEM_CHT3_PCL_TCAM_MASK_TBL_MEM(devObjPtr,line, 0);

    regPtr = smemMemGet(devObjPtr, regAddr);

    for(ii = 0 ; ii < 2; ii++)
    {
        data[(ii*3)+0]     = regPtr[(ii*4)+0] ;

        data[(ii*3)+1]     = regPtr[(ii*4)+1] & 0xFFFF;
        data[(ii*3)+1]    |= regPtr[(ii*4)+2] << 16;

        data[(ii*3)+2]     = regPtr[(ii*4)+2] >> 16;
        data[(ii*3)+2]    |= regPtr[(ii*4)+3] << 16;
    }

    /* A long key */
    if (mode)
    {
        /* TCAM data or mask / x or y */
        if(type == 1)   /*data*/
            regAddr = SMEM_CHT3_PCL_TCAM_DATA_TBL_MEM(devObjPtr,line+512, 0);
        else            /*mask*/
            regAddr = SMEM_CHT3_PCL_TCAM_MASK_TBL_MEM(devObjPtr,line+512, 0);

        regPtr = smemMemGet(devObjPtr, regAddr);

        for(ii = 0 ; ii < 2; ii++)
        {
            data[(ii*3)+0+6]     = regPtr[(ii*4)+0] ;

            data[(ii*3)+1+6]     = regPtr[(ii*4)+1] & 0xFFFF;
            data[(ii*3)+1+6]    |= regPtr[(ii*4)+2] << 16;

            data[(ii*3)+2+6]     = regPtr[(ii*4)+2] >> 16;
            data[(ii*3)+2+6]    |= regPtr[(ii*4)+3] << 16;
        }

        /* Write TCAM data register word 0 */
        smemMemSet(devObjPtr, SMEM_CHT_PCL_ACTION_TCAM_DATA0_REG(devObjPtr), data, 12);
    }
    else
    {
        /* Write TCAM data register word 6*/
        smemMemSet(devObjPtr, SMEM_CHT_PCL_ACTION_TCAM_DATA6_REG(devObjPtr), data, 6);
    }
}


/*******************************************************************************
*  smemCht3ReadWritePclAction
*
* DESCRIPTION:
*      Handler write for Cheetah3 PCL indirect action table.
*
* INPUTS:
*       devObjPtr - device object PTR.
*       address     - Address for ASIC memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers' specific parameter.
*       inMemPtr    - Pointer to the memory to get register's content.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void smemCht3ReadWritePclAction (
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32  line,
    IN         GT_U32  rdWr
)
{
    GT_U32     *regPtr;           /* register's entry pointer   */
    GT_U32      regAddr;          /* register's address         */
    GT_U32      accParam1Data;    /* ACCESS param1 reg data     */
    GT_U32      dataRegAddr;      /* address of data  register  */
    GT_U32      i;                /* loop index                 */
    GT_U32      bankSize;         /* loop index                 */

    /* amount of actions in each of 4 banks (action is 4 registers) */
    bankSize = PCL_ACTION_TBL_REGS_NUM(devObjPtr) / (4 /*banks*/ * 4 /*regs-of-action*/);

    /* indirect write parameters1 data */
    regPtr = smemMemGet(devObjPtr, SMEM_CHT3_PCL_ACTION_TCAM_DATA_PARAM1_REG(devObjPtr));
    accParam1Data = *regPtr;

    for (i = 0; (i < 4); i++)
    {
        if ((accParam1Data & (0x0F << (i * 4))) == 0)
        {
            /* all 4 words copying to the current bank masked */
            continue;
        }

        /* copy all 4 words to/from action table line     */
        /* partial line copy not supported bu simulation */

        /* PCL action table address */
        regAddr = SMEM_CHT_PCL_ACTION_TBL_MEM(devObjPtr, (line + (bankSize * i)));
        dataRegAddr =  SMEM_CHT3_PCL_ACTION_TCAM_DATA_TBL_MEM(devObjPtr,0) + 0x18 /* word6 */;

        if (rdWr)
        {
            /* Write Words 6-9 to Action Table and Policy TCAM Access Data */
            regPtr = smemMemGet(devObjPtr, dataRegAddr);
            smemMemSet(devObjPtr, regAddr, regPtr, 4);
        }
        else
        {
            /* Read words 6-9 from Action Table and Policy TCAM Access Data */
            regPtr = smemMemGet(devObjPtr, regAddr);
            smemMemSet(devObjPtr, dataRegAddr, regPtr, 4);
        }
    }
}


#if 0
/*******************************************************************************
*  smemCht3ActiveReadXgPortIntReg
*
* DESCRIPTION:
*      Port<n> Interrupt Cause Registers
* INPUTS:
*       devObjPtr - device object PTR.
*       address     - Address for ASIC memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers' specific parameter -
*                      global interrupt bit number.
*
* OUTPUTS:
*       outMemPtr   - Pointer to the memory to copy register's content.
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void smemCht3ActiveReadXgPortIntReg (
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    OUT        GT_U32 * outMemPtr
)
{
    GT_U32 fldFirstBit;
    GT_U32 regValue, regAddr;
    GT_U32 port;

    /* copy registers content to the output memory */
    *outMemPtr = *memPtr;

    /* clear register */
    smemRegSet(devObjPtr, address, 0);

    port = (address - 0x08800014) / 0x400;

    /* get value of XG ports Interrupt Cause -- 0x00000080 */
    regAddr = SMEM_CHT3_XG_PORTS_INTERRUPT_CAUSE_REG;
    smemRegGet(devObjPtr,regAddr,&regValue);

    fldFirstBit = (port == 0) ? 31 : port;

    /* reset summary bit for port */
    SMEM_U32_SET_FIELD(regValue, fldFirstBit, 1, 0);

    /* there are another summary bits are set */
    if(regValue > 1)
    {
        smemRegSet(devObjPtr, address, regValue);
        return;
    }

    /* reset all bits in the reg */
    smemRegSet(devObjPtr, address, 0);


    /* XGPortsIntSum */
    fldFirstBit = 20;

    /* get value of Global Interrupt Cause */
    regAddr = 0x30;
    smemRegGet(devObjPtr,regAddr,&regValue);

    /* reset summary bit for XGPortsIntSum */
    SMEM_U32_SET_FIELD(regValue, fldFirstBit, 1, 0);

    /* there are another summary bits are set */
    if(regValue > 1)
    {
        smemRegSet(devObjPtr, address, regValue);
        return;
    }

    /* reset all bits in the reg */
    smemRegSet(devObjPtr, address, 0);
}
#endif /*0*/

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
)
{
    GT_U32  mask;
    if(!SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        /* no CNC units */
        return 0;
    }

    mask = 0xFF000000;

    if((address & mask) == (devObjPtr->memUnitBaseAddrInfo.CNC[0] & mask))
    {
        return 0;
    }

    if((address & mask) == (devObjPtr->memUnitBaseAddrInfo.CNC[1] & mask))
    {
        return 1;
    }

    /* should not happen */
    skernelFatalError("cncUnitIndexFromAddrGet: address [0x%8.8x] not belong to CNC0[0x%8.8x] and not to CNC1[[0x%8.8x]] \n",
        address,
        devObjPtr->memUnitBaseAddrInfo.CNC[0],
        devObjPtr->memUnitBaseAddrInfo.CNC[1]);

    return 0;
}


/*******************************************************************************
*  smemCht3ActiveWriteCncFastDumpTrigger
*
* DESCRIPTION:
*
*   Trigger for start of Centralized Counter Block Upload.
*   Called from Application task context by write to
*   CNC Fast Dump Trigger Register
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
*       relevant only for Cheetah3 packet processor
*******************************************************************************/
void smemCht3ActiveWriteCncFastDumpTrigger
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
)
{
    SBUF_BUF_ID bufferId;               /* Buffer ID */
    GT_U32 SelCNCOrFU;                 /* Type of updates to send to the CPU Fu/Cnc */
    GT_U8  * dataPtr;                   /* Pointer to the data in the buffer */
    GT_U32 dataSize;                    /* Data size */
    GT_U32 cncUnitIndex = 0;            /* index of one of the CNC units*/

    *memPtr = *inMemPtr;

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        /* the device always allowed to send CNC messages (when triggered) */

        /* get which one of the CNC units was triggered by the CPU */
        /* <CNC Dump Selector> */
        smemRegFldGet(devObjPtr, SMEM_LION3_MG_GENERAL_CONFIG_REG(devObjPtr),28 , 1, &cncUnitIndex);
    }
    else
    {

        if (!SKERNEL_IS_XCAT2_DEV(devObjPtr))
        {
            smemRegFldGet(devObjPtr, SMEM_CHT_GLB_CTRL_REG(devObjPtr), 14, 1, &SelCNCOrFU );
            /* the bit must be reset to be 0 to permit CNC dump. */
            if (SelCNCOrFU)
            {
                return;
            }
        }
        else
        {
            /* the device always allowed to send CNC messages (when triggered) */
        }

    }

    /* Get buffer */
    bufferId = sbufAlloc(devObjPtr->bufPool, 2 * sizeof(GT_U32));

    if (bufferId == NULL)
    {
        simWarningPrintf("smemCht3ActiveWriteCncFastDumpTrigger: no buffers to upload fdb table\n");
        return;
    }

    /* Get actual data pointer */
    sbufDataGet(bufferId, &dataPtr, &dataSize);

    /* Copy CNC Fast Dump Trigger Register to buffer */
    memcpy(dataPtr, memPtr, sizeof(GT_U32));

    /* Copy cncUnitIndex to buffer */
    memcpy(dataPtr + sizeof(GT_U32), &cncUnitIndex, sizeof(GT_U32));

    /* Set source type of buffer */
    bufferId->srcType = SMAIN_SRC_TYPE_CPU_E;

    /* Set message type of buffer */
    bufferId->dataType = SMAIN_MSG_TYPE_CNC_FAST_DUMP_E;

    /* Put buffer to queue */
    squeBufPut(devObjPtr->queueId, SIM_CAST_BUFF(bufferId));
}

/*******************************************************************************
*  smemCht3ActiveCncBlockRead
*
* DESCRIPTION:
*
*   Centralized Counter Block Read.
*
* INPUTS:
*    devObjPtr  - device object PTR.
*    address    - Address for ASIC memory.
*    memPtr     - Pointer to the register's memory in the simulation.
*    memSize    - Size of memory to be written
*    param      - Registers' specific parameter.
*
* OUTPUTS:
*    outMemPtr   - Pointer to the memory to copy register's content.
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void smemCht3ActiveCncBlockRead
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    OUT        GT_U32 * outMemPtr
)
{
    GT_U32 wordIndex;               /* CNC block word index */
    GT_U32 start;                   /* CNC entry index */
    GT_U32 block;                   /* CNC block number */
    GT_U32 cncUnitIndex = smemCht3CncUnitIndexFromAddrGet(devObjPtr,address);
    GT_U32 mask;
    GT_U32 numBits;
    GT_U32 baseAddressBlock0;/*base address of block 0*/
    GT_U32 addressToUse;/*address to use */

    *outMemPtr = *memPtr;

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        /* support 1K entries of 8 bytes --> 13 bits */
        numBits = 13;
        baseAddressBlock0 = param;/*base address of block 0*/
    }
    else
    {
        /* support 2K entries of 8 bytes --> 14 bits */
        numBits = 14;
        baseAddressBlock0 = 0;
    }

    addressToUse = address - baseAddressBlock0;

    mask = SMEM_BIT_MASK(numBits);
    /* Calculate CNC block word index  */
    wordIndex = (addressToUse & mask) / 4;

    if (wordIndex % 2)
    {
        /* Calculate block and entry index */
        block = (addressToUse >> numBits) & 0xF;
        start = (addressToUse & mask) / 8;

        /* Reset CNC block entry after two words read */
        snetCht3CncBlockReset(devObjPtr, block, start, 1,cncUnitIndex);
    }

}

/*******************************************************************************
*  smemCht3ActiveCncWrapAroundStatusRead
*
* DESCRIPTION:
*
*   CNC Block Wraparound Status Register Read.
*
* INPUTS:
*    devObjPtr  - device object PTR.
*    address    - Address for ASIC memory.
*    memPtr     - Pointer to the register's memory in the simulation.
*    memSize    - Size of memory to be written
*    param      - Registers' specific parameter.
*
* OUTPUTS:
*    outMemPtr   - Pointer to the memory to copy register's content.
*
* RETURNS:
*
* COMMENTS:
*       relevant only for Cheetah3 packet processor
*******************************************************************************/
void smemCht3ActiveCncWrapAroundStatusRead
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    OUT        GT_U32 * outMemPtr
)
{
    *outMemPtr = *memPtr;

    *memPtr &= ~0x10001;
}

/*******************************************************************************
*  smemCht3ReadRssiExceptionReg1
*
* DESCRIPTION:
*      Read the second register of the CAPWAP RSSI exception registers .
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
*
*
*******************************************************************************/
static void smemCht3ReadRssiExceptionReg1
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    OUT        GT_U32 * outMemPtr
)
{
    GT_U32  *internalMemPtr;

    internalMemPtr = CHT_INTERNAL_MEM_PTR(devObjPtr,
        CHT_INTERNAL_SIMULATION_USE_MEM_CAPWAP_RSSI_EXCEPTION_STATUS_E);

    *outMemPtr = *memPtr;

    *memPtr = 0;

    (*internalMemPtr) |= 1;/* set bit 0 -- state that PP can update those registers */

    return;
}

/*******************************************************************************
*  smemCht3ActiveWriteFDBGlobalCfgReg
*
* DESCRIPTION:
*   The function updates FDB table size according to FDB_Global_Configuration[bit24]
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
void smemCht3ActiveWriteFDBGlobalCfgReg (
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

    /* bit[24] - FDBSize */
    fdbSize = (*memPtr >> 24) & 0x1;

    switch(devObjPtr->deviceFamily)
    {
        case SKERNEL_XCAT_FAMILY:
            if(devObjPtr->deviceRevisionId)
            {
                /* bit[24],[25]  - FDBSize */
                fdbSize = (*memPtr >> 24) & 0x3;
            }
            break;
        case SKERNEL_XCAT3_FAMILY:
        case SKERNEL_XCAT2_FAMILY:
        case SKERNEL_LION2_PORT_GROUP_FAMILY: /* lion2 case , because lion3 uses smemLion2ActiveWriteFDBGlobalCfgReg */
        case SKERNEL_LION3_PORT_GROUP_FAMILY:
            /* bit[24],[25]  - FDBSize */
            fdbSize = (*memPtr >> 24) & 0x3;
            break;
        default:
            break;
    }

    switch(fdbSize)
    {
        case 0:
            if(!SKERNEL_DEVICE_FAMILY_XCAT_ONLY(devObjPtr))
            {
                devObjPtr->fdbNumEntries = SMEM_MAC_TABLE_SIZE_32KB;
            }
            break;
        case 1:
            devObjPtr->fdbNumEntries = SMEM_MAC_TABLE_SIZE_16KB;
            break;
        case 2:
            devObjPtr->fdbNumEntries = SMEM_MAC_TABLE_SIZE_8KB;
            break;
        case 3:
            if (SKERNEL_IS_XCAT2_DEV(devObjPtr))
            {
                devObjPtr->fdbNumEntries = SMEM_MAC_TABLE_SIZE_4KB;
            }
            break;
        default:
            break;
    }

    if(devObjPtr->fdbMaxNumEntries < devObjPtr->fdbNumEntries)
    {
        skernelFatalError("smemCht3ActiveWriteFDBGlobalCfgReg: FDB num entries in HW [0x%4.4x] , but set to use[0x%4.4x] \n",
            devObjPtr->fdbMaxNumEntries , devObjPtr->fdbNumEntries);
    }

    return;
}

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
)
{
    {
        GT_U32 i;

        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001820 ,0x000018c4 )}  /* PCI Express Address Window Control */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001804 ,0x0000180c )}  /* PCI Express BAR Control */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000018f8 ,0x000018fc )}  /* PCI Express Configuration Cycles Generation */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000000 ,0x00000128 )}  /* PCI Express Configuration Header  */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001a00 ,0x00001B00 )}  /* PCI Express Control and Status  */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001900 ,0x00001910  )} /* PCI Express Interrupt  */
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
*  smemXcatA1ActiveWriteVlanTbl
*
* DESCRIPTION:
*    VLT Tables Access Control Register.
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
void smemXcatA1ActiveWriteVlanTbl
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
)
{
    GT_U32                  rdWr;            /* Read or write operation */
    GT_U32                  entry;           /* table entry number */
    GT_U32                  trgTbl;          /* target table */
    GT_U32                * regPtr;          /* register's entry pointer */
    GT_U32                  regAddr;         /* register's address */
    GT_U32                  hwWords[XCAT_A1_VLAN_NUM_WORDS_CNS];/* vlan entry */
    GT_U32                  ii;
    GT_U32                  numWords;/* number of words to read/write */
    GT_U32                  accessRegAddress;/*address of the register of the
                                            access registers */

    rdWr   = (*inMemPtr >> 12) & 0x1;
    entry  = *inMemPtr & 0xFFF;
    trgTbl = (*inMemPtr >> 13)& 0x3;

    switch(trgTbl)
    {
        case 0:
            /* VLAN table */
            regAddr = SMEM_CHT_VLAN_TBL_MEM(devObjPtr, entry);
            numWords = XCAT_A1_VLAN_NUM_WORDS_CNS;
            break;
        case 1:
            /* Multicast Groups table */
            regAddr = SMEM_CHT_MCST_TBL_MEM(devObjPtr, entry);
            numWords = 1;
            break;
        case 2:
            /* Span Stage Groups table */
            regAddr = SMEM_CHT_STP_TBL_MEM(devObjPtr, entry);
            numWords = 2;
            break;
        default:
            regAddr = 0;
            numWords = 0;
            break;
    }

    accessRegAddress = SMEM_XCAT_VLAN_TBL_ACCESS_BASE_REG(devObjPtr) +
            (XCAT_A1_VLAN_NUM_WORDS_CNS - numWords)* 4;


    if (rdWr)
    {
        /* VLT Tables Access Data Register */
        regPtr = smemMemGet(devObjPtr, accessRegAddress);
        /* Swap entry words */
        for(ii = 0 ; ii < numWords ; ii++)
        {
            hwWords[ii] = regPtr[numWords-(ii+1)];
        }
        smemMemSet(devObjPtr, regAddr, hwWords, numWords);
    }
    else
    {
        regPtr = smemMemGet(devObjPtr, regAddr);
        /* Swap entry words */
        for(ii = 0 ; ii < numWords ; ii++)
        {
            hwWords[ii] = regPtr[numWords-(ii+1)];
        }
        /* VLT Tables Access Data */
        smemMemSet(devObjPtr, accessRegAddress, hwWords, numWords);
    }

    /* Cleared by the device when the read or write action is completed. */
    *memPtr = *inMemPtr & (~(1 << 15));
}


/*******************************************************************************
*  smemCht3IpclTccEngineReg
*
* DESCRIPTION:
*      Router Engine registers and tables.
*
* INPUTS:
*       devObjPtr - device object PTR.
*       address     - Address for ASIC memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers' specific parameter.
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
static GT_U32 * smemCht3IpclTccEngineReg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    return smemDevFindInUnitChunk(devObjPtr, accessType, address, memSize,
        (GT_UINTPTR)&(((SMEM_CHT3_DEV_MEM_INFO*)devObjPtr->deviceMemory)->ipclTccUnitMem));
}

/*******************************************************************************
*  smemCht3IPvxTccEngineReg
*
* DESCRIPTION:
*      Router Engine registers and tables.
*
* INPUTS:
*       devObjPtr - device object PTR.
*       address     - Address for ASIC memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers' specific parameter.
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
static GT_U32 * smemCht3IPvxTccEngineReg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    return smemDevFindInUnitChunk(devObjPtr, accessType, address, memSize,
        (GT_UINTPTR)&(((SMEM_CHT3_DEV_MEM_INFO*)devObjPtr->deviceMemory)->ipvxTccUnitMem));
}

/*******************************************************************************
*  smemCht3ActiveWriteSerdesSpeed
*
* DESCRIPTION:
*   The function check if the speed of the serdes changed , and due to that the
*   'mode' of this port , from working with the XG registers or the GIGA registers
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
static void smemCht3ActiveWriteSerdesSpeed (
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
)
{
    GT_U32 serdes;
    GT_U32  port;

    /* data to be written */
    *memPtr = *inMemPtr;

    if(SKERNEL_DEVICE_FAMILY_CHEETAH_3_ONLY(devObjPtr) ||
       SKERNEL_IS_XCAT_REVISON_A1_DEV(devObjPtr))
    {
        /* only for xcat A0 */
        return;
    }

    serdes = (address / 0x400) & 0x3f;

    /* every 4 serdess represent the ports of 24..27 */
    port = 24 + (serdes - 32)/4;

    if(port >= devObjPtr->portsNumber)
    {
        skernelFatalError("smemCht3ActiveWriteSerdesSpeed: port [%d] out of range \n",port);
    }

    if(devObjPtr->portsArr[port].supportMultiState != GT_TRUE)
    {
        return;
    }

    /* check the SpeedPll bit 10 in: SERDES Speed Register1 */
    if(SMEM_U32_GET_FIELD((*inMemPtr),10,1))
    {
        devObjPtr->portsArr[port].state = SKERNEL_PORT_STATE_XG_E;
    }
    else
    {
        /* only in the xcat-A0 (and not in lion A0) the GE registers for
           the stacking where in the memory space of the XG registers */
        devObjPtr->portsArr[port].state = SKERNEL_PORT_STATE_GE_STACK_A0_E;
    }


    return;
}

/*******************************************************************************
*  smemCht3ActiveWriteCncInterruptsMaskReg
*
* DESCRIPTION:
*   the application changed the value of the interrupts mask register.
*   check if there is waiting interrupt for that.
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
void smemCht3ActiveWriteCncInterruptsMaskReg (
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
)
{
    GT_U32 dataArray[3];                     /* Data array to put into buffer */
    GT_U32 cncUnitIndex = smemCht3CncUnitIndexFromAddrGet(devObjPtr,address);

    /* Address of CNC interrupt cause register */
    dataArray[0] = SMEM_CHT3_CNC_INTR_CAUSE_REG(devObjPtr,cncUnitIndex);
    /* Address of CNC  interrupt mask register */
    dataArray[1] = SMEM_CHT3_CNC_INTR_MASK_REG(devObjPtr,cncUnitIndex);
    /* Summary bit in global interrupt cause register */
    dataArray[2] = SMEM_CHT_CNC_ENGINE_INT(devObjPtr,cncUnitIndex);

    /* Call for common interrupt mask active write function */
    smemChtActiveWriteInterruptsMaskReg(devObjPtr, address, memSize, memPtr,
                                        (GT_UINTPTR)dataArray, inMemPtr);
}

/*******************************************************************************
*  smemXcatActiveWriteMacModeSelect
*
* DESCRIPTION:
*   The function check if the mac mode of the port changed , and due to that the
*   'mode' of this port , from working with the XG registers or the GIGA registers
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
void smemXcatActiveWriteMacModeSelect (
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
)
{
    GT_U32  port;
    GT_U32  xgMode;
    GT_U32  numBits;

    /* data to be written */
    *memPtr = *inMemPtr;

    port = CHT_FROM_ADDRESS_GET_PORT_CNS(devObjPtr,address);

    if(port >= devObjPtr->portsNumber)
    {
        /* the XLG port registers are in memory space that match the 'port 12' .
        about the 'XLG port' - the port in Lion B that can be in the
        40G speed is port 10 but the actual memory space is as if the
        port is 12 */
        if(devObjPtr->portsArr[port].state == SKERNEL_PORT_STATE_XLG_40G_E)
            return;
        else
            skernelFatalError("smemXcatActiveWriteMacModeSelect: port [%d] out of range \n",port);
    }

    if(devObjPtr->portsArr[port].supportMultiState != GT_TRUE)
    {
        return;
    }

    if(SKERNEL_IS_LION_REVISON_B0_DEV(devObjPtr))
    {
        numBits = 3;
    }
    else
    {
        numBits = 2;
    }

    if(SMEM_U32_GET_FIELD((*inMemPtr), 13, numBits))
    {
        /* mode for 10,20,40 G*/
        /* for lion b also HGL */
        devObjPtr->portsArr[port].state = SKERNEL_PORT_STATE_XG_E;

        xgMode = 1;
    }
    else
    {
        /* mode for 1,2.5G */
        devObjPtr->portsArr[port].state = SKERNEL_PORT_STATE_GE_STACK_A1_E;

        xgMode = 0;
    }

    if(! skernelIsLionPortGroupOnlyDev(devObjPtr))
    {
        /* set the 'Sampled at reset' register */

        /* set bits 26..29 in the 'Sampled at reset' to be sync with the
           SERDESs */
        smemRegFldSet(devObjPtr, SAMPLED_AT_RESET_ADDR, (port+2), 1, xgMode);
    }


    return;
}


/*******************************************************************************
*   smemXcatTransQueReg
*
* DESCRIPTION:
*       Includes xCat revision A1 and above memory.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
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
static GT_U32 *  smemXcatTransQueReg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    GT_U32*                 memPtr;

    memPtr =  smemDevFindInUnitChunk(devObjPtr, accessType, address, memSize,
        (GT_UINTPTR)&(((SMEM_CHT3_DEV_MEM_INFO*)devObjPtr->deviceMemory)->xCatExtraMem.egrMemUnitMem));

    if(memPtr)
    {
        /* memory was found */
        return memPtr;
    }

    /* Check Ch3 in memory allocations */
    return smemCht3TransQueReg(devObjPtr, accessType, address, memSize, param);
}

/*******************************************************************************
*   smemXcatEtherBrdgReg
*
* DESCRIPTION:
*       Bridge Configuration Registers
*
* INPUTS:
*       devObjPtr   - pointer to device object.
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
static GT_U32 *  smemXcatEtherBrdgReg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    GT_U32*                 memPtr;

    memPtr =  smemDevFindInUnitChunk(devObjPtr, accessType, address, memSize,
        (GT_UINTPTR)&(((SMEM_CHT3_DEV_MEM_INFO*)devObjPtr->deviceMemory)->xCatExtraMem.brdgMngUnitMem));

    if(memPtr)
    {
        /* memory was found */
        return memPtr;
    }

    /* Check Ch3 in memory allocations */
    return smemCht3EtherBrdgReg(devObjPtr, accessType, address, memSize, param);
}

/*******************************************************************************
*   smemXcatHaAndEpclConfUnitReg
*
* DESCRIPTION:
*       Describe a device's HA registers and EPCL memory object
*
* INPUTS:
*       devObjPtr   - pointer to device object.
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
static GT_U32 *  smemXcatHaAndEpclConfUnitReg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    return  smemDevFindInUnitChunk(devObjPtr, accessType, address, memSize,
        (GT_UINTPTR)&(((SMEM_CHT3_DEV_MEM_INFO*)devObjPtr->deviceMemory)->xCatExtraMem.haAndEpclUnitMem));
}

/*******************************************************************************
*   smemXcatBufMngUnitReg
*
* DESCRIPTION:
*       Describe a device's buffer management registers memory object.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
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
static GT_U32 *  smemXcatBufMngUnitReg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    GT_U32*                 memPtr;

    memPtr =  smemDevFindInUnitChunk(devObjPtr, accessType, address, memSize,
        (GT_UINTPTR)&(((SMEM_CHT3_DEV_MEM_INFO*)devObjPtr->deviceMemory)->xCatExtraMem.bufMngMemUnitMem));

    if(memPtr)
    {
        /* memory was found */
        return memPtr;
    }

    return smemCht3BufMngReg(devObjPtr, accessType, address, memSize, param);
}

/*******************************************************************************
*   smemXcatPclReg
*
* DESCRIPTION:
*       PCL registers memory space access.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
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
static GT_U32 *  smemXcatPclReg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    GT_U32*                 memPtr;

    memPtr =  smemDevFindInUnitChunk(devObjPtr, accessType, address, memSize,
        (GT_UINTPTR)&(((SMEM_CHT3_DEV_MEM_INFO*)devObjPtr->deviceMemory)->xCatExtraMem.pclMemUnitMem));

    if(memPtr)
    {
        /* memory was found */
        return memPtr;
    }

    return smemCht3PclReg(devObjPtr, accessType, address, memSize, param);
}

/*******************************************************************************
*   smemXcatPclTccReg
*
* DESCRIPTION:
*       PCL registers and TCAM memory space access.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
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
static GT_U32 *  smemXcatPclTccReg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    return  smemDevFindInUnitChunk(devObjPtr, accessType, address, memSize,
        (GT_UINTPTR)&(((SMEM_CHT3_DEV_MEM_INFO*)devObjPtr->deviceMemory)->xCatExtraMem.pclTccMemUnitMem));
}

/*******************************************************************************
*   smemXcatEgrPclReg
*
* DESCRIPTION:
*       Egress PCL registers and TCAM memory space access.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
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
static GT_U32 *  smemXcatEgrPclReg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    return  smemDevFindInUnitChunk(devObjPtr, accessType, address, memSize,
        (GT_UINTPTR)&(((SMEM_CHT3_DEV_MEM_INFO*)devObjPtr->deviceMemory)->xCatExtraMem.pclMemEgrUnitMem));
}

/*******************************************************************************
*   smemXcatPolicerReg
*
* DESCRIPTION:
*       Policer registers and meters memory space access
*
* INPUTS:
*       devObjPtr   - pointer to device object.
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
static GT_U32 *  smemXcatPolicerReg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SMEM_UNIT_CHUNKS_STC    *unitChunksPtr;

    switch(param)
    {
        case 0:
            unitChunksPtr = &((SMEM_CHT3_DEV_MEM_INFO*)devObjPtr->deviceMemory)->xCatExtraMem.policerMemIngress0UnitMem;
            break;
        case 1:
            unitChunksPtr = &((SMEM_CHT3_DEV_MEM_INFO*)devObjPtr->deviceMemory)->xCatExtraMem.policerMemIngress1UnitMem;
            break;
        case 2:
            unitChunksPtr = &((SMEM_CHT3_DEV_MEM_INFO*)devObjPtr->deviceMemory)->xCatExtraMem.policerMemEgressUnitMem;
            break;
        default:
            return NULL;
    }

    return  smemDevFindInUnitChunk(devObjPtr, accessType, address, memSize,
        (GT_UINTPTR)unitChunksPtr);
}

/*******************************************************************************
*   smemCht3XgPortsReg
*
* DESCRIPTION:
*      Describe XG ports MAC control registers, FIFO threshold configuration,
*      ports interrupt mask and cause registers, port per priority FC status,
*      metal fix and MIB counters control registers.
*
*
*
* INPUTS:
*       devObjPtr   - pointer to device object.
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
static GT_U32 *  smemCht3XgPortsReg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SMEM_CHT3_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32                  * regValPtr;
    GT_32                     index;
    GT_U32                    port;

    regValPtr = 0;
    devMemInfoPtr = (SMEM_CHT3_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    STACK_PORT_NUM_FROM_ADDRESS_MAC(address, port);

    if(IS_CHT_GIG_STACK_A0_PORT(devObjPtr,port))
    {
        /* the port is in the 0x08800000 range but the registers are 'Stack GIG mode' */
        return smemCht3TriSpeedReg(devObjPtr, accessType, address,memSize,param);
    }


    index = (address & 0xFFFF) / 0x4;
    CHECK_MEM_BOUNDS(devMemInfoPtr->xgCtrlPortsMem.xgPortsReg,
                     devMemInfoPtr->xgCtrlPortsMem.xgPortsRegNum,
                     index, memSize);
    regValPtr = &devMemInfoPtr->xgCtrlPortsMem.xgPortsReg[index];


    return regValPtr;
}
/*******************************************************************************
*   smemCht3XgMibReg
*
* DESCRIPTION:
*      Describe XG ports MIB Counters memory
*
*
*
* INPUTS:
*       devObjPtr   - pointer to device object.
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
static GT_U32 *  smemCht3XgMibReg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SMEM_CHT3_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32                  * regValPtr;
    GT_U32                    portNum; /* port number [0..27]*/
    GT_U32                    portIdx; /* port index 0 for port 0,
                                                     1 for port 4,
                                                     2 for port 10,
                                                     3 for port 12
                                                     4 for port 16,
                                                     5 for port 22
                                                     6..9 for port 24..27*/
    GT_32                     index;

    regValPtr = 0;
    devMemInfoPtr = (SMEM_CHT3_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    /* XG MIB Memory */
    if ((address & 0xFF000000) == 0x09000000)
    {
        portNum = (address - 0x09000000) / 0x20000;
        if(portNum >= devObjPtr->portsNumber)
        {
            /* memory not found */
            return 0;
        }

        if(SKERNEL_DEVICE_FAMILY_CHEETAH_3_ONLY(devObjPtr))
        {
            if(portNum >= sizeof(smemCht3XgPortToIndexArray)/sizeof(smemCht3XgPortToIndexArray[0]))
            {
                skernelFatalError("ch3 array size not match ports number\n");
            }

            portIdx = smemCht3XgPortToIndexArray[portNum];
        }
        else
        {
            if(address == 0x09000000 && (simulationInitReady == 0))
            {
                /* dummy memory for simulation testing of : devObjPtr->tablesInfo.xgPortMibCounters */
                static GT_U32   dummyMem_0x09000000 = 0;
                return &dummyMem_0x09000000;
            }

            if(portNum >= sizeof(smemXcat24_4XgPortToIndexArray)/sizeof(smemXcat24_4XgPortToIndexArray[0]))
            {
                skernelFatalError("xcat array size not match ports number\n");
            }

            portIdx = smemXcat24_4XgPortToIndexArray[portNum];
        }

        if(portIdx == 0xff)
        {
            /* wrong XG port - memory not found */
            return 0;
        }

        index = (address & 0xFFFF) / 0x4 +
                 portIdx * XG_MIB_CNTRS_REGS_PER_PORT_NUM;

        CHECK_MEM_BOUNDS(devMemInfoPtr->xgMibCntrsMem.mibReg,
                         devMemInfoPtr->xgMibCntrsMem.mibRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->xgMibCntrsMem.mibReg[index];
    }

    return regValPtr;
}

/*******************************************************************************
*   smemXcatPreEgressUnitReg
*
* DESCRIPTION:
*      Pre-Egress Engine Registers Map Table, Trunk Table,
*      QoSProfile to QoS Table, CPU Code Table,
*      Data Access Statistical Rate Limits Table Data Access,
*      Ingress STC Table Registers memory access
*
* INPUTS:
*       devObjPtr   - pointer to device object.
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
static GT_U32 *  smemXcatPreEgressUnitReg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    GT_U32*                 memPtr;

    memPtr =  smemDevFindInUnitChunk(devObjPtr, accessType, address, memSize,
        (GT_UINTPTR)&(((SMEM_CHT3_DEV_MEM_INFO*)devObjPtr->deviceMemory)->xCatExtraMem.eqUnitUnitMem));

    if(memPtr)
    {
        /* memory was found */
        return memPtr;
    }

    return smemCht3PreEgressReg(devObjPtr, accessType, address, memSize, param);
}
/*******************************************************************************
*  smemXcatMllEngineReg
*
* DESCRIPTION:
*       MLL engine registers
*
* INPUTS:
*       devObjPtr   - device object PTR.
*       address     - Address for ASIC memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers' specific parameter.
*       inMemPtr    - Pointer to the memory to get register's content.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static GT_U32 * smemXcatMllEngineReg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    return  smemDevFindInUnitChunk(devObjPtr, accessType, address, memSize,
        (GT_UINTPTR)&(((SMEM_CHT3_DEV_MEM_INFO*)devObjPtr->deviceMemory)->xCatExtraMem.mllMemUnitMem));
}

/*******************************************************************************
*  smemXcatIPvxTccEngineUniReg
*
* DESCRIPTION:
*      Router Engine registers and tables.
*
* INPUTS:
*       devObjPtr - device object PTR.
*       address     - Address for ASIC memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers' specific parameter.
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
static GT_U32 * smemXcatIPvxTccEngineUniReg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    return  smemDevFindInUnitChunk(devObjPtr, accessType, address, memSize,
        (GT_UINTPTR)&(((SMEM_CHT3_DEV_MEM_INFO*)devObjPtr->deviceMemory)->xCatExtraMem.ipvxTccUnitMem));
}

/*******************************************************************************
*   smemXcatDfxMemReg
*
* DESCRIPTION:
*       Describe a device's DFX memory
*
* INPUTS:
*       devObjPtr   - pointer to device object.
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
static GT_U32 *  smemXcatDfxMemReg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    return  smemDevFindInUnitChunk(devObjPtr, accessType, address, memSize,
        (GT_UINTPTR)&(((SMEM_CHT3_DEV_MEM_INFO*)devObjPtr->deviceMemory)->xCatExtraMem.dfxUnitMem));
}

/*******************************************************************************
*   smemXcatVlanTableReg
*
* DESCRIPTION:
*       Describe a device's Vlan table
*
* INPUTS:
*       devObjPtr   - pointer to device object.
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
static GT_U32 *  smemXcatVlanTableReg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    return  smemDevFindInUnitChunk(devObjPtr, accessType, address, memSize,
        (GT_UINTPTR)&(((SMEM_CHT3_DEV_MEM_INFO*)devObjPtr->deviceMemory)->xCatExtraMem.vlanTblUnitMem));
}

/*******************************************************************************
*   smemXcatTtiReg
*
* DESCRIPTION:
*       Describe a device's TTI unit
*
* INPUTS:
*       devObjPtr   - pointer to device object.
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
static GT_U32 *  smemXcatTtiReg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    return  smemDevFindInUnitChunk(devObjPtr, accessType, address, memSize,
        (GT_UINTPTR)&(((SMEM_CHT3_DEV_MEM_INFO*)devObjPtr->deviceMemory)->xCatExtraMem.ttiMemUnitMem));
}

/*******************************************************************************
*  smemXCatActiveReadIntrCauseFlexPortA0Reg
*
* DESCRIPTION:
*      Read and reset interrupts cause registers and summary bits - special for
*      xcat A0 flex ports registers that the register may NOT be interrupt cause , when
*      in XG mode.
*      Port number calculation is in macro PORT_NUM_FROM_ADDRESS_MAC(address)
* INPUTS:
*       devObjPtr   - device object PTR.
*       address     - address for ASIC memory.
*       memSize     - memory size to be read.
*       memPtr      - pointer to the register's memory in the simulation.
*       sumBit      - global summary interrupt bit
*
* OUTPUTS:
*       outMemPtr   - Pointer to the memory to copy register's content.
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static GT_VOID smemXCatActiveReadIntrCauseFlexPortA0Reg
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR sumBit,
    OUT        GT_U32 * outMemPtr
)
{
    GT_U32  ii;
    GT_U32  port;

    STACK_PORT_NUM_FROM_ADDRESS_MAC(address, port);

    if(port >= devObjPtr->portsNumber)
    {
        skernelFatalError("smemXCatActiveReadIntrCauseFlexPortA0Reg: port [%d] out of range \n",port);
    }

    if(IS_CHT_GIG_STACK_A0_PORT(devObjPtr, port))
    {
        /* Port work with GE registers of stack ports for XCat A0 */
        smemChtActiveReadIntrCauseReg(devObjPtr, address, memSize, memPtr,
                                      sumBit, outMemPtr);
        return;
    }

    /* non active memory -- do direct read */
    for (ii = 0; ii < memSize; ii++)
    {
        outMemPtr[ii] = memPtr[ii];
    }

    return;
}

/*******************************************************************************
*  smemXCatActiveWriteIntrCauseFlexPortA0Reg
*
* DESCRIPTION:
*      Write interrupts cause registers and summary bits - special for
*      xcat A0 flex ports registers that the register may NOT be interrupt cause , when
*      in XG mode.
*      Port number calculation is in macro PORT_NUM_FROM_ADDRESS_MAC(address)
*
* INPUTS:
*       devObjPtr   - device object PTR.
*       address     - Address for ASIC memory.
*       memSize     - memory size to be read.
*       memPtr      - Pointer to the register's memory in the simulation.
*       writeMask   - 32 bits mask of writable bits.
*       inMemPtr    - Pointer to the memory to set register's content.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static GT_VOID smemXCatActiveWriteIntrCauseFlexPortA0Reg
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR writeMask,
    IN         GT_U32 * inMemPtr
)
{
    GT_U32  ii;
    GT_U32  port;

    STACK_PORT_NUM_FROM_ADDRESS_MAC(address, port);

    if(port >= devObjPtr->portsNumber)
    {
        skernelFatalError("smemXCatActiveWriteIntrCauseFlexPortA0Reg: port [%d] out of range \n",port);
    }

    if(IS_CHT_GIG_STACK_A0_PORT(devObjPtr, port))
    {
        /* Port work with GE registers of stack ports for XCat A0 */
        smemChtActiveWriteIntrCauseReg(devObjPtr, address, memSize, memPtr,
                                       writeMask, inMemPtr);
        return;
    }

    /* non active memory -- do direct write */
    for (ii = 0; ii < memSize; ii++)
    {
        memPtr[ii] = inMemPtr[ii];
    }

    return;
}

/*******************************************************************************
*  smemXCatActiveWritePortInterruptsMaskFlexPortA0Reg
*
* DESCRIPTION:
*      Write interrupts mask registers - special for
*      xcat A0 flex ports registers that the register may NOT be interrupt mask, when
*      in XG mode.
*      Port number calculation is in macro PORT_NUM_FROM_ADDRESS_MAC(address)
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
static GT_VOID smemXCatActiveWritePortInterruptsMaskFlexPortA0Reg
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
)
{
    GT_U32  ii;
    GT_U32  port;

    STACK_PORT_NUM_FROM_ADDRESS_MAC(address, port);

    if(port >= devObjPtr->portsNumber)
    {
        skernelFatalError("smemXCatActiveWritePortInterruptsMaskFlexPortA0Reg: port [%d] out of range \n",port);
    }

    if(IS_CHT_GIG_STACK_A0_PORT(devObjPtr, port))
    {
        /* Port work with GE registers of stack ports for XCat A0 */
        smemChtActiveWritePortInterruptsMaskReg(devObjPtr, address, memSize,
                                                memPtr, param, inMemPtr);
        return;
    }

    /* non active memory -- do direct write */
    for (ii = 0; ii < memSize; ii++)
    {
        memPtr[ii] = inMemPtr[ii];
    }

    return;
}

/*******************************************************************************
*  smemXCatActiveWriteForceLinkDownFlexPortA0Reg
*
* DESCRIPTION:
*      Write Message to the main task - Link change on port - special for
*      xcat A0 flex ports registers that the register may NOT be Auto-Negotiation,
*      when in XG mode.
*      Port number calculation is in macro PORT_NUM_FROM_ADDRESS_MAC(address)
*      Write Message to the main task - Link change on port.
*
* INPUTS:
*       devObjPtr - device object PTR.
*       address     - Address for ASIC memory.
*       memSize     - size of the requested memory
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers' specific parameter.
*       inMemPtr    - Pointer to the memory to get register's content.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*       The function invoked when the linkdown bit (0) on
*       port<n> Auto-Negotiation has been changed.
*******************************************************************************/
static GT_VOID smemXCatActiveWriteForceLinkDownFlexPortA0Reg
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
)
{
    GT_U32  ii;
    GT_U32  port;

    STACK_PORT_NUM_FROM_ADDRESS_MAC(address, port);

    if(port >= devObjPtr->portsNumber)
    {
        skernelFatalError("smemXCatActiveWriteForceLinkDownFlexPortA0Reg: port [%d] out of range \n",port);
    }

    if(IS_CHT_GIG_STACK_A0_PORT(devObjPtr,port))
    {
        /* Port work with GE registers of stack ports for XCat A0 */
        smemChtActiveWriteForceLinkDown(devObjPtr, address, memSize,
                                        memPtr, param, inMemPtr);
        return;
    }

    /* non active memory -- do direct write */
    for (ii = 0; ii < memSize; ii++)
    {
        memPtr[ii] = inMemPtr[ii];
    }

    return;
}


/*write active memory*/
void smemXCatActiveWriteBridgeGlobalConfig2Reg (
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
)
{
    /* save the new value */
    *memPtr = *inMemPtr;

    if(devObjPtr->supportVpls)
    {
        devObjPtr->vplsModeEnable.bridge = SMEM_U32_GET_FIELD(*inMemPtr,17,1);
    }
}

void smemXCatActiveWriteEqGlobalConfigReg (
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
)
{
    /* save the new value */
    *memPtr = *inMemPtr;

    if(devObjPtr->supportVpls)
    {
        devObjPtr->vplsModeEnable.eq = SMEM_U32_GET_FIELD(*inMemPtr,24,1);
    }
}

void smemXCatActiveWriteIpclGlobalConfigReg (
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
)
{
    /* save the new value */
    *memPtr = *inMemPtr;

    if(devObjPtr->supportVpls)
    {
        devObjPtr->vplsModeEnable.ipcl = SMEM_U32_GET_FIELD(*inMemPtr,15,1);
    }
}

void smemXCatActiveWriteTtiGlobalConfigReg (
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
)
{
    /* save the new value */
    *memPtr = *inMemPtr;

    if(devObjPtr->supportVpls)
    {
        devObjPtr->vplsModeEnable.tti = SMEM_U32_GET_FIELD(*inMemPtr,29,1);
    }
}

void smemXCatActiveWriteHaGlobalConfigReg (
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
)
{
    /* save the new value */
    *memPtr = *inMemPtr;

    if(devObjPtr->supportVpls)
    {
        devObjPtr->vplsModeEnable.ha = SMEM_U32_GET_FIELD(*inMemPtr,16,1);
    }
}

void smemXCatActiveWriteMllGlobalConfigReg (
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
)
{
    /* save the new value */
    *memPtr = *inMemPtr;

    if(devObjPtr->supportVpls)
    {
        devObjPtr->vplsModeEnable.mll = SMEM_U32_GET_FIELD(*inMemPtr,14,1);
    }
}

/*******************************************************************************
*  smemXCatActiveWriteEgressFilterVlanMap
*
* DESCRIPTION:
*    The EQ Egress Filter Vlan Map Table write access.
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
void smemXCatActiveWriteEgressFilterVlanMap
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
)
{
    GT_U32     lineNumber;      /* line number in the Table */
    GT_U32     rdWr;            /* read/write operation */
    GT_U32     regAddrTbl,regAddrDataAccess;         /* register address */
    GT_U32    *dataAccessRegPtr;/* register pointer */
    GT_U32    *tableEntryPtr;    /* pointer to table entry */

    if(devObjPtr->tablesInfo.eqVlanMapping.commonInfo.baseAddress == 0 || /* this device not support active memory on this register */
        ((*inMemPtr & 1) == 0))/* the triggering was not operated*/
    {
        *memPtr = *inMemPtr;
        return;
    }

    /* Action 0 - read , 1 - write */
    rdWr = SMEM_U32_GET_FIELD(*inMemPtr, 1, 1);
    /* Line Number */
    lineNumber = SMEM_U32_GET_FIELD(*inMemPtr, 2, 12);

    /* the table entry */
    regAddrTbl = SMEM_XCAT_EQ_VLAN_MAPPING_TABLE_TBL_MEM(devObjPtr,
                                                   lineNumber);
    tableEntryPtr = smemMemGet(devObjPtr, regAddrTbl);

    /* Table Data Access */
    regAddrDataAccess = SMEM_XCAT_EQ_EGRESS_FILTER_VLAN_MAP_TBL_DATA_REG(devObjPtr);
    dataAccessRegPtr = smemMemGet(devObjPtr, regAddrDataAccess);

    if (rdWr == 0)
    {
        /* Read the entry --> copy info from the table */
        smemMemSet(devObjPtr,
            regAddrDataAccess,
            tableEntryPtr,
            devObjPtr->tablesInfo.eqVlanMapping.paramInfo[0].step/4);
    }
    else
    {
        /* Write to the entry --> copy info into the table */
        smemMemSet(devObjPtr,
            regAddrTbl,
            dataAccessRegPtr,
            devObjPtr->tablesInfo.eqVlanMapping.paramInfo[0].step/4);
    }

    /* Cleared by the device when the read or write action is completed. */
    *memPtr = *inMemPtr & (~1);
}

/*******************************************************************************
*  smemXCatActiveWriteEgressFilterVlanMember
*
* DESCRIPTION:
*    The EQ Egress Filter Vlan Member Table write access.
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
void smemXCatActiveWriteEgressFilterVlanMember
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
)
{
    GT_U32     lineNumber;      /* line number in the Table */
    GT_U32     rdWr;            /* read/write operation */
    GT_U32     regAddrTbl,regAddrDataAccess;         /* register address */
    GT_U32    *dataAccessRegPtr;/* register pointer */
    GT_U32    *tableEntryPtr;    /* pointer to table entry */

    if(devObjPtr->tablesInfo.eqLogicalPortEgressVlanMember.commonInfo.baseAddress == 0 || /* this device not support active memory on this register */
        ((*inMemPtr & 1) == 0))/* the triggering was not operated*/
    {
        *memPtr = *inMemPtr;
        return;
    }

    /* Action 0 - read , 1 - write */
    rdWr = SMEM_U32_GET_FIELD(*inMemPtr, 1, 1);
    /* Line Number */
    lineNumber = SMEM_U32_GET_FIELD(*inMemPtr, 2, 12);

    /* the table entry */
    regAddrTbl = SMEM_XCAT_EQ_LOGICAL_PORT_EGRESS_VLAN_MEMBER_TBL_MEM(devObjPtr,
                                                   lineNumber);
    tableEntryPtr = smemMemGet(devObjPtr, regAddrTbl);

    /* Table Data Access */
    regAddrDataAccess = SMEM_XCAT_EQ_EGRESS_FILTER_VLAN_MEMBER_TABLE_TBL_DATA_REG(devObjPtr);
    dataAccessRegPtr = smemMemGet(devObjPtr, regAddrDataAccess);

    if (rdWr == 0)
    {
        /* Read the entry --> copy info from the table */
        smemMemSet(devObjPtr,
            regAddrDataAccess,
            tableEntryPtr,
            devObjPtr->tablesInfo.eqLogicalPortEgressVlanMember.paramInfo[0].step/4);
    }
    else
    {
        /* Write to the entry --> copy info into the table */
        smemMemSet(devObjPtr,
            regAddrTbl,
            dataAccessRegPtr,
            devObjPtr->tablesInfo.eqLogicalPortEgressVlanMember.paramInfo[0].step/4);
    }

    /* Cleared by the device when the read or write action is completed. */
    *memPtr = *inMemPtr & (~1);
}

/*******************************************************************************
*  smemXCatActiveWriteL2MllVidxEnable
*
* DESCRIPTION:
*    The L2 MLL VIDX Enable Table write access.
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
void smemXCatActiveWriteL2MllVidxEnable
(
    IN SKERNEL_DEVICE_OBJECT *devObjPtr,
    IN GT_U32                 address,
    IN GT_U32                 memSize,
    IN GT_U32                *memPtr,
    IN GT_UINTPTR             param,
    IN GT_U32                *inMemPtr
)
{
    GT_U32  lineNumber;                   /* line number in the Table */
    GT_U32  rdWr;                         /* read/write operation */
    GT_U32  regAddrTbl,regAddrDataAccess; /* register address */
    GT_U32 *dataAccessRegPtr;             /* register pointer */
    GT_U32 *tableEntryPtr;                /* pointer to table entry */

    if(devObjPtr->tablesInfo.l2MllVidxToMllMapping.commonInfo.baseAddress == 0 || /* this device not support active memory on this register */
        ((*inMemPtr & 1) == 0))/* the triggering was not operated*/
    {
        *memPtr = *inMemPtr;
        return;
    }

    /* Action 0 - read , 1 - write */
    rdWr = SMEM_U32_GET_FIELD(*inMemPtr, 1, 1);

    /* Line Number */
    lineNumber = SMEM_U32_GET_FIELD(*inMemPtr, 2, 12);

    /* the table entry */
    regAddrTbl = SMEM_XCAT_L2_MLL_VIDX_TO_MLL_MAPPING_TBL_MEM(devObjPtr, lineNumber);
    tableEntryPtr = smemMemGet(devObjPtr, regAddrTbl);

    /* Table Data Access */
    regAddrDataAccess = SMEM_XCAT_L2_MLL_VIDX_ENABLE_TBL_DATA_REG(devObjPtr);
    dataAccessRegPtr = smemMemGet(devObjPtr, regAddrDataAccess);

    if (rdWr == 0)
    {
        /* Read the entry --> copy info from the table */
        smemMemSet(devObjPtr,
            regAddrDataAccess,
            tableEntryPtr,
            devObjPtr->tablesInfo.l2MllVidxToMllMapping.paramInfo[0].step/4);
    }
    else
    {
        /* Write to the entry --> copy info into the table */
        smemMemSet(devObjPtr,
            regAddrTbl,
            dataAccessRegPtr,
            devObjPtr->tablesInfo.l2MllVidxToMllMapping.paramInfo[0].step/4);
    }

    /* Cleared by the device when the read or write action is completed. */
    *memPtr = *inMemPtr & (~1);

}

/*******************************************************************************
*  smemXCatActiveWriteL2MllPointerMap
*
* DESCRIPTION:
*    The L2 MLL Pointer Map.
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
void smemXCatActiveWriteL2MllPointerMap
(
    IN SKERNEL_DEVICE_OBJECT *devObjPtr,
    IN GT_U32                 address,
    IN GT_U32                 memSize,
    IN GT_U32                *memPtr,
    IN GT_UINTPTR             param,
    IN GT_U32                *inMemPtr
)
{
    GT_U32  lineNumber;                   /* line number in the Table */
    GT_U32  rdWr;                         /* read/write operation */
    GT_U32  regAddrTbl,regAddrDataAccess; /* register address */
    GT_U32 *dataAccessRegPtr;             /* register pointer */
    GT_U32 *tableEntryPtr;                /* pointer to table entry */

    if(devObjPtr->tablesInfo.l2MllVirtualPortToMllMapping.commonInfo.baseAddress == 0 || /* this device not support active memory on this register */
        ((*inMemPtr & 1) == 0))/* the triggering was not operated*/
    {
        *memPtr = *inMemPtr;
        return;
    }

    /* Action 0 - read , 1 - write */
    rdWr = SMEM_U32_GET_FIELD(*inMemPtr, 1, 1);

    /* Line Number */
    lineNumber = SMEM_U32_GET_FIELD(*inMemPtr, 2, 12);

    /* the table entry */
    regAddrTbl = SMEM_XCAT_L2_MLL_VIRTUAL_PORT_TO_MLL_MAPPING_TBL_MEM(devObjPtr, lineNumber);
    tableEntryPtr = smemMemGet(devObjPtr, regAddrTbl);

    /* Table Data Access */
    regAddrDataAccess = SMEM_XCAT_L2_MLL_POINTER_MAP_TBL_DATA_REG(devObjPtr);
    dataAccessRegPtr = smemMemGet(devObjPtr, regAddrDataAccess);

    if (rdWr == 0)
    {
        /* Read the entry --> copy info from the table */
        smemMemSet(devObjPtr,
            regAddrDataAccess,
            tableEntryPtr,
            devObjPtr->tablesInfo.l2MllVirtualPortToMllMapping.paramInfo[0].step/4);
    }
    else
    {
        /* Write to the entry --> copy info into the table */
        smemMemSet(devObjPtr,
            regAddrTbl,
            dataAccessRegPtr,
            devObjPtr->tablesInfo.l2MllVirtualPortToMllMapping.paramInfo[0].step/4);
    }

    /* Cleared by the device when the read or write action is completed. */
    *memPtr = *inMemPtr & (~1);
}

/*******************************************************************************
*  smemCht3TableInfoSet
*
* DESCRIPTION:
*       set the table info for the device --> fill devObjPtr->tablesInfo
*       override values set by smemCht2TableInfoSet
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
)
{
    smemCht2TableInfoSet(devObjPtr);

    devObjPtr->globalInterruptCauseRegister.pexInt     = 1;
    devObjPtr->globalInterruptCauseRegister.pexErr     = 2;
    devObjPtr->globalInterruptCauseRegister.cnc[0]     = 3;
    devObjPtr->globalInterruptCauseRegister.global1    = 20;/* AKA 'xg ports' */
    devObjPtr->globalInterruptCauseRegister.port24     = SMAIN_NOT_VALID_CNS;
    devObjPtr->globalInterruptCauseRegister.port25     = SMAIN_NOT_VALID_CNS;
    devObjPtr->globalInterruptCauseRegister.port26     = SMAIN_NOT_VALID_CNS;
    devObjPtr->globalInterruptCauseRegister.port27     = SMAIN_NOT_VALID_CNS;
    devObjPtr->globalInterruptCauseRegister.iplr0      = 24;

    if(devObjPtr->policerSupport.tablesBaseAddrSetByOrigDev == 0)
    {
        devObjPtr->tablesInfo.policer.commonInfo.baseAddress = 0x0C040000;
        devObjPtr->tablesInfo.policer.paramInfo[0].step = 0x20; /*entry*/
        devObjPtr->tablesInfo.policer.paramInfo[1].step = 0; /* not supported*/

        devObjPtr->tablesInfo.policerQos.commonInfo.baseAddress = 0x0C080000;

        devObjPtr->tablesInfo.policerCounters.commonInfo.baseAddress = devObjPtr->tablesInfo.policer.commonInfo.baseAddress + (1024 * 0x20);
        devObjPtr->tablesInfo.policerCounters.paramInfo[0].step = 0x20;/*entry*/
        devObjPtr->tablesInfo.policerCounters.paramInfo[1].step = 0; /* not supported*/

        devObjPtr->tablesInfo.policerTblAccessData.commonInfo.baseAddress = 0xC000030;
        devObjPtr->tablesInfo.policerTblAccessData.paramInfo[0].step = 0x4; /*entry*/
    }

    devObjPtr->tablesInfo.pclConfig.commonInfo.baseAddress = 0x0B840000;
    devObjPtr->tablesInfo.pclConfig.paramInfo[0].step = 0x8; /*entry*/

    devObjPtr->tablesInfo.pclAction.commonInfo.baseAddress = 0x0B880000;

    devObjPtr->tablesInfo.pclTcam.commonInfo.baseAddress = 0x0BA00000;
    devObjPtr->tablesInfo.pclTcam.paramInfo[0].step = 0x40;  /*entry*/
    devObjPtr->tablesInfo.pclTcam.paramInfo[1].step = 0x8;   /*word*/

    devObjPtr->tablesInfo.pclTcamMask.commonInfo.baseAddress = devObjPtr->tablesInfo.pclTcam.commonInfo.baseAddress + 0x20;
    devObjPtr->tablesInfo.pclTcamMask.paramInfo[0].step = devObjPtr->tablesInfo.pclTcam.paramInfo[0].step;   /*entry*/
    devObjPtr->tablesInfo.pclTcamMask.paramInfo[1].step = devObjPtr->tablesInfo.pclTcam.paramInfo[1].step;   /*word*/

    devObjPtr->tablesInfo.ttiAction.commonInfo.baseAddress = 0x02900000;
    devObjPtr->tablesInfo.ttiAction.paramInfo[0].step = 0x4;     /*entry*/
    devObjPtr->tablesInfo.ttiAction.paramInfo[1].step = 0x5000;  /*word*/


    devObjPtr->tablesInfo.vrfId.commonInfo.baseAddress = 0x0A300000;
    devObjPtr->tablesInfo.vrfId.paramInfo[0].step = 0x10;

    devObjPtr->tablesInfo.ingressVlanTranslation.commonInfo.baseAddress = SMEM_CHT3_INGRESS_VLAN_TRANSLATION_TBL_MEM;
    devObjPtr->tablesInfo.ingressVlanTranslation.paramInfo[0].step = 0x4;

    devObjPtr->tablesInfo.egressVlanTranslation.commonInfo.baseAddress = SMEM_CHT3_EGRESS_VLAN_TRANSLATION_TBL_MEM;
    devObjPtr->tablesInfo.egressVlanTranslation.paramInfo[0].step = 0x4;

    devObjPtr->tablesInfo.macToMe.commonInfo.baseAddress = 0x0B800700;
    devObjPtr->tablesInfo.macToMe.paramInfo[0].step = 0x10;

    devObjPtr->tablesInfo.ieeeRsrvMcCpuIndex.commonInfo.baseAddress = 0x02000804;
    devObjPtr->tablesInfo.ieeeRsrvMcCpuIndex.paramInfo[0].step = 0x1000;
    devObjPtr->tablesInfo.ieeeRsrvMcCpuIndex.paramInfo[0].divider = 16;

    devObjPtr->tablesInfo.routeNextHopAgeBits.commonInfo.baseAddress = 0x2801000; /*Router Next Hop Table Age Bits*/
    devObjPtr->tablesInfo.routeNextHopAgeBits.paramInfo[0].step = 0x4;

    devObjPtr->tablesInfo.xgPortMibCounters.commonInfo.baseAddress = 0x09000000;
    devObjPtr->tablesInfo.xgPortMibCounters.paramInfo[0].step = 0x20000;

    devObjPtr->tablesInfo.secondTargetPort.commonInfo.baseAddress = 0x01E40000;
    devObjPtr->tablesInfo.secondTargetPort.paramInfo[0].step = 0x10;

    devObjPtr->tablesInfo.upToQoSProfile.commonInfo.baseAddress   = 0x0B8004A0;
    devObjPtr->tablesInfo.upToQoSProfile.paramInfo[1].step = 0x10;   /* CFI */


    if(SKERNEL_IS_XCAT_DEV(devObjPtr))/* xcat a1,a0 */
    {
        /* note those values may be override by the xcat A1 device -->
            see function smemXcatA1TableInfoSet */

        devObjPtr->tablesInfo.pclActionTcamData.commonInfo.baseAddress = 0x0d000100;

        devObjPtr->tablesInfo.pclAction.commonInfo.baseAddress = 0x0D0B0000;

        devObjPtr->tablesInfo.pclTcam.commonInfo.baseAddress = 0x0D040000;
        devObjPtr->tablesInfo.pclTcamMask.commonInfo.baseAddress = devObjPtr->tablesInfo.pclTcam.commonInfo.baseAddress + 0x20;

        devObjPtr->tablesInfo.routerTcam.commonInfo.baseAddress = 0x0D840000;
        devObjPtr->tablesInfo.routerTcam.paramInfo[0].step = devObjPtr->routeTcamInfo.entryWidth;  /*entry*/
        devObjPtr->tablesInfo.routerTcam.paramInfo[1].step = 0x4;                                  /*word*/

        devObjPtr->tablesInfo.ttiAction.commonInfo.baseAddress = 0x0d880000;
        devObjPtr->tablesInfo.ttiAction.paramInfo[0].step = 0x10;   /*entry*/
        devObjPtr->tablesInfo.ttiAction.paramInfo[1].step = 0x4;    /*word*/
    }

    if(devObjPtr->xgCountersStepPerPort == 0)
    {
        devObjPtr->xgCountersStepPerPort = 0x20000;
    }

    devObjPtr->tablesInfo.cncMemory.commonInfo.baseAddress = 0x08080000;
    devObjPtr->tablesInfo.cncMemory.paramInfo[0].step = 0x4000;/* block*/
    devObjPtr->tablesInfo.cncMemory.paramInfo[1].step = 8;/*index*/

}

/*******************************************************************************
*  smemXcatA1TableInfoSet
*
* DESCRIPTION:
*       set the table info for the device --> fill devObjPtr->tablesInfo
*       override values set by smemCht3TableInfoSet
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
)
{
    GT_U32  iplr0ToIplr1Offset;
    GT_BIT  updatePlr = devObjPtr->policerSupport.tablesBaseAddrSetByOrigDev ? 0 : 1;

    smemCht3TableInfoSet(devObjPtr);

    if(devObjPtr->memUnitBaseAddrInfo.policer[0] == 0)
    {
        /* DON'T OVERRIDE ADDRESS !!!
           ALLOW MORE ADVANCED DEVICE TO SET OTHER ADDRESSES */
        devObjPtr->memUnitBaseAddrInfo.policer[0] = 0x0C800000;
        devObjPtr->memUnitBaseAddrInfo.policer[1] = 0x0D000000;
        devObjPtr->memUnitBaseAddrInfo.policer[2] = 0x03800000;
    }


    devObjPtr->globalInterruptCauseRegister.tti        = 4;
    devObjPtr->globalInterruptCauseRegister.tccLower   = 5;
    devObjPtr->globalInterruptCauseRegister.tccUpper   = 6;
    devObjPtr->globalInterruptCauseRegister.bcn        = 7;
    devObjPtr->globalInterruptCauseRegister.misc       = 9;
    devObjPtr->globalInterruptCauseRegister.mem        = 10;
    devObjPtr->globalInterruptCauseRegister.txq        = 11;
    devObjPtr->globalInterruptCauseRegister.l2i        = 12;
    devObjPtr->globalInterruptCauseRegister.bm0        = 13;
    devObjPtr->globalInterruptCauseRegister.bm1        = 14;
    devObjPtr->globalInterruptCauseRegister.fdb        = 15;
    devObjPtr->globalInterruptCauseRegister.ports      = 16;
    devObjPtr->globalInterruptCauseRegister.cpuPort    = 17;
    devObjPtr->globalInterruptCauseRegister.global1    = 18;/* AKA 'xg ports' */
    devObjPtr->globalInterruptCauseRegister.txSdma     = 19;
    devObjPtr->globalInterruptCauseRegister.rxSdma     = 20;
    devObjPtr->globalInterruptCauseRegister.pcl        = 21;
    devObjPtr->globalInterruptCauseRegister.iplr0      = 22;
    devObjPtr->globalInterruptCauseRegister.iplr1      = 23;
    devObjPtr->globalInterruptCauseRegister.eq         = 24;
    devObjPtr->globalInterruptCauseRegister.eplr       = 8;

    devObjPtr->tablesInfo.stp.commonInfo.baseAddress = 0x0a080000;

    devObjPtr->tablesInfo.mcast.commonInfo.baseAddress = 0x0a100000;

    devObjPtr->tablesInfo.vlan.commonInfo.baseAddress = 0x0a200000;
    devObjPtr->tablesInfo.vlan.paramInfo[0].step = 0x20;

    devObjPtr->tablesInfo.portVlanQosConfig.commonInfo.baseAddress = 0x0c001000;
    devObjPtr->tablesInfo.portVlanQosConfig.paramInfo[0].step = 0x10;

    devObjPtr->tablesInfo.arp.commonInfo.baseAddress = 0x0F040000;

    devObjPtr->tablesInfo.vlanPortMacSa.commonInfo.baseAddress = 0x0F008000;

    devObjPtr->tablesInfo.pclConfig.commonInfo.baseAddress = 0x0b810000;
    devObjPtr->tablesInfo.pclConfig.paramInfo[0].step = 0x8; /*entry*/
    devObjPtr->tablesInfo.pclConfig.paramInfo[1].step = 0x10000; /* IPCL0/IPCL1 */

    devObjPtr->tablesInfo.pclAction.commonInfo.baseAddress = 0x0e0b0000;

    devObjPtr->tablesInfo.pclTcam.commonInfo.baseAddress = 0x0e040000;

    devObjPtr->tablesInfo.pclTcamMask.commonInfo.baseAddress = devObjPtr->tablesInfo.pclTcam.commonInfo.baseAddress + 0x20;

    /* eplr tables */
    {
        {
            SET_IF_CONDITION_TRUE_MAC(updatePlr,devObjPtr->tablesInfo.egressPolicerCounters.commonInfo.baseAddress,
                        SMEM_XCAT_POLICER_OFFSET_MAC(devObjPtr,2,0x00060000));
            SET_IF_CONDITION_TRUE_MAC(updatePlr,devObjPtr->tablesInfo.egressPolicerCounters.paramInfo[0].step , 0x20);

            SET_IF_CONDITION_TRUE_MAC(updatePlr,devObjPtr->tablesInfo.egressPolicerMeters.commonInfo.baseAddress ,
                        SMEM_XCAT_POLICER_OFFSET_MAC(devObjPtr,2,0x00040000));
            SET_IF_CONDITION_TRUE_MAC(updatePlr,devObjPtr->tablesInfo.egressPolicerMeters.paramInfo[0].step , 0x20);
        }
    }

    /* iplr tables */
    {
        iplr0ToIplr1Offset = devObjPtr->memUnitBaseAddrInfo.policer[1] -
                             devObjPtr->memUnitBaseAddrInfo.policer[0];

        SET_IF_CONDITION_TRUE_MAC(updatePlr,devObjPtr->tablesInfo.policer.commonInfo.baseAddress ,
                    SMEM_XCAT_POLICER_OFFSET_MAC(devObjPtr,0,0x00040000));
        SET_IF_CONDITION_TRUE_MAC(updatePlr,devObjPtr->tablesInfo.policer.paramInfo[0].step , 0x20); /*entry*/
        SET_IF_CONDITION_TRUE_MAC(updatePlr,devObjPtr->tablesInfo.policer.paramInfo[1].step , iplr0ToIplr1Offset);/*cycle*/

        SET_IF_CONDITION_TRUE_MAC(updatePlr,devObjPtr->tablesInfo.policerCounters.commonInfo.baseAddress ,
                    SMEM_XCAT_POLICER_OFFSET_MAC(devObjPtr,0,0x00060000));
        SET_IF_CONDITION_TRUE_MAC(updatePlr,devObjPtr->tablesInfo.policerCounters.paramInfo[0].step , 0x20);/*entry*/
        SET_IF_CONDITION_TRUE_MAC(updatePlr,devObjPtr->tablesInfo.policerCounters.paramInfo[1].step , iplr0ToIplr1Offset);/*cycle*/
    }

    /* policer tables that are 'duplicated' for IPLR0,1,EPLR */
    {
        SET_IF_CONDITION_TRUE_MAC(updatePlr,devObjPtr->tablesInfo.policerQos.commonInfo.baseAddress ,
                    SMEM_XCAT_POLICER_OFFSET_MAC(devObjPtr,0,0x00080000));
        SET_IF_CONDITION_TRUE_MAC(updatePlr,devObjPtr->tablesInfo.policerQos.paramInfo[0].step , 0x8); /*entry*/

        devObjPtr->tablesInfo.policerQos.commonInfo.multiInstanceInfo.numBaseAddresses = 3;
        devObjPtr->tablesInfo.policerQos.commonInfo.multiInstanceInfo.multiUnitsBaseAddressPtr = &devObjPtr->memUnitBaseAddrInfo.policer[0];

        SET_IF_CONDITION_TRUE_MAC(updatePlr,devObjPtr->tablesInfo.policerTblAccessData.commonInfo.baseAddress ,
                    SMEM_XCAT_POLICER_OFFSET_MAC(devObjPtr,0,0x00000074));
        SET_IF_CONDITION_TRUE_MAC(updatePlr,devObjPtr->tablesInfo.policerTblAccessData.paramInfo[0].step , 0x4); /*entry*/

        devObjPtr->tablesInfo.policerTblAccessData.commonInfo.multiInstanceInfo.numBaseAddresses = 3;
        devObjPtr->tablesInfo.policerTblAccessData.commonInfo.multiInstanceInfo.multiUnitsBaseAddressPtr = &devObjPtr->memUnitBaseAddrInfo.policer[0];

        SET_IF_CONDITION_TRUE_MAC(updatePlr,devObjPtr->tablesInfo.policerMeterPointer.commonInfo.baseAddress ,
                    SMEM_XCAT_POLICER_OFFSET_MAC(devObjPtr,0,0x00001800));
        SET_IF_CONDITION_TRUE_MAC(updatePlr,devObjPtr->tablesInfo.policerMeterPointer.paramInfo[0].step , 0x4); /*entry*/

        devObjPtr->tablesInfo.policerMeterPointer.commonInfo.multiInstanceInfo.numBaseAddresses = 3;
        devObjPtr->tablesInfo.policerMeterPointer.commonInfo.multiInstanceInfo.multiUnitsBaseAddressPtr = &devObjPtr->memUnitBaseAddrInfo.policer[0];
        if(!SMEM_CHT_IS_SIP5_20_GET(devObjPtr))
        {
            devObjPtr->tablesInfo.policerMeterPointer.paramInfo[0].divider = 2;
        }

        SET_IF_CONDITION_TRUE_MAC(updatePlr,devObjPtr->tablesInfo.policerIpfixWaAlert.commonInfo.baseAddress ,
                    SMEM_XCAT_POLICER_OFFSET_MAC(devObjPtr,0,0x00000800));
        SET_IF_CONDITION_TRUE_MAC(updatePlr,devObjPtr->tablesInfo.policerIpfixWaAlert.paramInfo[0].step , 0x4); /*entry*/

        devObjPtr->tablesInfo.policerIpfixWaAlert.commonInfo.multiInstanceInfo.numBaseAddresses = 3;
        devObjPtr->tablesInfo.policerIpfixWaAlert.commonInfo.multiInstanceInfo.multiUnitsBaseAddressPtr = &devObjPtr->memUnitBaseAddrInfo.policer[0];
        devObjPtr->tablesInfo.policerIpfixWaAlert.paramInfo[0].divider = 32;

        SET_IF_CONDITION_TRUE_MAC(updatePlr,devObjPtr->tablesInfo.policerIpfixAgingAlert.commonInfo.baseAddress ,
                    SMEM_XCAT_POLICER_OFFSET_MAC(devObjPtr,0,0x00000900));
        SET_IF_CONDITION_TRUE_MAC(updatePlr,devObjPtr->tablesInfo.policerIpfixAgingAlert.paramInfo[0].step , 0x4); /*entry*/

        devObjPtr->tablesInfo.policerIpfixAgingAlert.commonInfo.multiInstanceInfo.numBaseAddresses = 3;
        devObjPtr->tablesInfo.policerIpfixAgingAlert.commonInfo.multiInstanceInfo.multiUnitsBaseAddressPtr = &devObjPtr->memUnitBaseAddrInfo.policer[0];
        devObjPtr->tablesInfo.policerIpfixAgingAlert.paramInfo[0].divider = 32;

        SET_IF_CONDITION_TRUE_MAC(updatePlr,devObjPtr->tablesInfo.policerTimer.commonInfo.baseAddress ,
                    SMEM_XCAT_POLICER_OFFSET_MAC(devObjPtr,0,0x00000200));
        SET_IF_CONDITION_TRUE_MAC(updatePlr,devObjPtr->tablesInfo.policerTimer.paramInfo[0].step , 0x4); /* entry */

        devObjPtr->tablesInfo.policerTimer.commonInfo.multiInstanceInfo.numBaseAddresses = 3;
        devObjPtr->tablesInfo.policerTimer.commonInfo.multiInstanceInfo.multiUnitsBaseAddressPtr = &devObjPtr->memUnitBaseAddrInfo.policer[0];

        SET_IF_CONDITION_TRUE_MAC(updatePlr,devObjPtr->tablesInfo.policerManagementCounters.commonInfo.baseAddress ,
                    SMEM_XCAT_POLICER_OFFSET_MAC(devObjPtr,0,0x00000500));
        devObjPtr->tablesInfo.policerManagementCounters.paramInfo[0].step = 0x10; /* entry */ /* 4 entries are 'counter set'*/
        devObjPtr->tablesInfo.policerManagementCounters.commonInfo.multiInstanceInfo.numBaseAddresses = 3;
        devObjPtr->tablesInfo.policerManagementCounters.commonInfo.multiInstanceInfo.multiUnitsBaseAddressPtr = &devObjPtr->memUnitBaseAddrInfo.policer[0];

        devObjPtr->policerSupport.meterTblBaseAddr = devObjPtr->tablesInfo.policer.commonInfo.baseAddress;
        devObjPtr->policerSupport.countTblBaseAddr = devObjPtr->tablesInfo.policerCounters.commonInfo.baseAddress;
    }

    devObjPtr->tablesInfo.ipclUserDefinedBytesConf.commonInfo.baseAddress = 0x0C010000;
    devObjPtr->tablesInfo.ipclUserDefinedBytesConf.paramInfo[0].step = 0x20;

    devObjPtr->tablesInfo.tunnelStart.commonInfo.baseAddress = devObjPtr->tablesInfo.arp.commonInfo.baseAddress;
    devObjPtr->tablesInfo.tunnelStart.paramInfo[0].step = devObjPtr->tablesInfo.arp.paramInfo[0].step;

    devObjPtr->tablesInfo.routerTcam.commonInfo.baseAddress = 0x0e840000;
    devObjPtr->tablesInfo.routerTcam.paramInfo[0].step = devObjPtr->routeTcamInfo.entryWidth;/*entry*/
    devObjPtr->tablesInfo.routerTcam.paramInfo[1].step = 0x4;                                /*word*/


    devObjPtr->tablesInfo.ttiAction.commonInfo.baseAddress = 0x0e8c0000;
    devObjPtr->tablesInfo.ttiAction.paramInfo[0].step = 0x20;   /*entry*/
    devObjPtr->tablesInfo.ttiAction.paramInfo[1].step = 0x4;

    devObjPtr->tablesInfo.vrfId.commonInfo.baseAddress = 0x0a180000;

    devObjPtr->tablesInfo.ingressVlanTranslation.commonInfo.baseAddress = 0x0c00c000;

    devObjPtr->tablesInfo.egressVlanTranslation.commonInfo.baseAddress = 0x0f010000;

    devObjPtr->tablesInfo.macToMe.commonInfo.baseAddress = 0x0C001600;


    devObjPtr->tablesInfo.mll.commonInfo.baseAddress = 0x0d880000;

    devObjPtr->tablesInfo.mllOutInterfaceConfig.commonInfo.baseAddress = 0x0d800980;

    devObjPtr->tablesInfo.mllOutInterfaceCounter.commonInfo.baseAddress = 0x0d800900;

    devObjPtr->memUnitBaseAddrInfo.mll = 0x0d800000;

    devObjPtr->tablesInfo.logicalTrgMappingTable.commonInfo.baseAddress = 0x0b600000;
    devObjPtr->tablesInfo.logicalTrgMappingTable.paramInfo[0].step = 0x100; /* target device */
    devObjPtr->tablesInfo.logicalTrgMappingTable.paramInfo[1].step = 0x4; /* target port */

    devObjPtr->tablesInfo.ieeeRsrvMcConfTable.commonInfo.baseAddress = 0x02000800;
    devObjPtr->tablesInfo.ieeeRsrvMcConfTable.paramInfo[0].step = 0x1000;
    devObjPtr->tablesInfo.ieeeRsrvMcConfTable.paramInfo[0].divider = 16;
    devObjPtr->tablesInfo.ieeeRsrvMcConfTable.paramInfo[1].step = 0x8;

    devObjPtr->tablesInfo.ieeeTblSelect.commonInfo.baseAddress = 0x0200080C;
    devObjPtr->tablesInfo.ieeeTblSelect.paramInfo[0].step = 0x10;
    devObjPtr->tablesInfo.ieeeTblSelect.paramInfo[0].divider = 16;

    devObjPtr->tablesInfo.dscpToDscpMap.commonInfo.baseAddress    = 0x0C000900;
    devObjPtr->tablesInfo.dscpToQoSProfile.commonInfo.baseAddress = 0x0C000400;

    /* CFI UP -> QosProfile */
    devObjPtr->tablesInfo.upToQoSProfile.commonInfo.baseAddress   = 0x0C000440;
    devObjPtr->tablesInfo.expToQoSProfile.commonInfo.baseAddress  = 0x0c000460;

    devObjPtr->tablesInfo.l2PortIsolation.commonInfo.baseAddress = 0x01E40004;
    devObjPtr->tablesInfo.l2PortIsolation.paramInfo[0].step = 0x10;

    devObjPtr->tablesInfo.l3PortIsolation.commonInfo.baseAddress = 0x01E40008;
    devObjPtr->tablesInfo.l3PortIsolation.paramInfo[0].step = 0x10;

    devObjPtr->tablesInfo.portProtocolVidQoSConf.commonInfo.baseAddress = 0x0C004000;
    devObjPtr->tablesInfo.portProtocolVidQoSConf.paramInfo[0].step = 0x80;
    devObjPtr->tablesInfo.portProtocolVidQoSConf.paramInfo[1].step = 0x8;

}

/*******************************************************************************
*  smemXcatA1TableInfoSetPart2
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
void smemXcatA1TableInfoSetPart2
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    if (SKERNEL_DEVICE_FAMILY_XCAT_ONLY(devObjPtr) && devObjPtr->deviceRevisionId >= 4)
    {
        devObjPtr->tablesInfo.logicalTrgMappingTable.paramInfo[0].step = 0x200; /* target device */
        devObjPtr->tablesInfo.logicalTrgMappingTable.paramInfo[1].step = 0x8; /* target port */
    }
}

