/******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* smemXCat3.c
*
* DESCRIPTION:
*       xCat3 memory mapping implementation
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 8 $
*
*******************************************************************************/
#include <os/simTypes.h>
#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SKernel/smem/smemXCat3.h>
#include <asicSimulation/SKernel/smem/smemLion3.h>
#include <asicSimulation/SKernel/cheetahCommon/sregCheetah.h>
#include <asicSimulation/SKernel/suserframes/snetCheetah3Pcl.h>
#include <asicSimulation/SKernel/suserframes/snetXCatPolicer.h>

static GT_U32 policerBaseAddr[3] = {0x17000000,0x18000000,0x06000000};

static GT_U32 mppmBankBaseAddr[2] = {0x0C000000,0x0D000000};

#define MSM_MIB_COUNTER_STEP_PER_PORT_CNS   0x400
#define SMEM_XCAT3_MSM_MIB_COUNT_MSK_CNS    (GT_U32)((~(MSM_MIB_COUNTER_STEP_PER_PORT_CNS * 0x1F)) & (~(0xFF)))

BUILD_STRING_FOR_UNIT_NAME(DFX_SERVER);

static SMEM_REGISTER_DEFAULT_VALUE_STC registersDfxDefaultValueArr[] =
{     {&STRING_FOR_UNIT_NAME(DFX_SERVER),          0x000f800c,         0x00000806,      1,    0x0                        }
     ,{&STRING_FOR_UNIT_NAME(DFX_SERVER),          0x000f8010,         0x0000000f,      1,    0x0                        }
     ,{&STRING_FOR_UNIT_NAME(DFX_SERVER),          0x000f8014,         0xfffffff3,      1,    0x0                        }
     ,{&STRING_FOR_UNIT_NAME(DFX_SERVER),          0x000f8020,         0x00000181,      1,    0x0                        }
     ,{&STRING_FOR_UNIT_NAME(DFX_SERVER),          0x000f8030,         0x00000381,      1,    0x0                        }
     ,{&STRING_FOR_UNIT_NAME(DFX_SERVER),          0x000f8060,         0x00000181,      1,    0x0                        }
     ,{&STRING_FOR_UNIT_NAME(DFX_SERVER),          0x000f8064,         0x00000181,      1,    0x0                        }
     ,{&STRING_FOR_UNIT_NAME(DFX_SERVER),          0x000f8068,         0x00000181,      1,    0x0                        }
     ,{&STRING_FOR_UNIT_NAME(DFX_SERVER),          0x000f806c,         0x00000181,      1,    0x0                        }
     ,{&STRING_FOR_UNIT_NAME(DFX_SERVER),          0x000f8070,         0x01064044,      1,    0x0                        }
     ,{&STRING_FOR_UNIT_NAME(DFX_SERVER),          0x000f8074,         0x63414819,      1,    0x0                        }
     ,{&STRING_FOR_UNIT_NAME(DFX_SERVER),          0x000f8290,         0x20c00000,      1,    0x0                        }
     ,{NULL,                                           0,                     0x00000000,                          0,    0x0                        }
};

static SMEM_LINK_LIST_REGISTER_DEFAULT_VALUE_STC  element0_DfxDefault_RegistersDefaults =
    {registersDfxDefaultValueArr , NULL};

/* Active memory table */
static SMEM_ACTIVE_MEM_ENTRY_STC smemXCat3ActiveTable[] =
{
    /* Global interrupt cause register */
    {0x00000030, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteIntrCauseReg, 0},

    /* read interrupts cause registers Misc -- ROC register */
    {0x00000038, SMEM_FULL_MASK_CNS,
        smemChtActiveReadIntrCauseReg, 9, smemChtActiveWriteIntrCauseReg, 0},

    /* Write Interrupt Mask MIsc Register */
    {0x0000003c, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteMiscInterruptsMaskReg, 0},

    /* Global control register */
    {0x00000058, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteGlobalReg, 0},

    /* Address Update Queue Base Address */
    {0x000000C0, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveAuqBaseWrite, 0},

    /* FDB Upload Queue Base Address */
    {0x000000C8, SMEM_FULL_MASK_CNS, NULL, 0 , smemCht2ActiveFuqBaseWrite, 0},

    /* AUQHostConfiguration */
    {0x000000D8, SMEM_FULL_MASK_CNS, NULL, 0 , smemLion3ActiveAuqHostCfgWrite, 0},/* Lion3 function !!!
        need to debug as it cause crash in UT of 'dsma enable queue' ?! */

    /* Receive SDMA Queue Command */
    {0x00002680, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteSdmaCommand,0},

    /* SDMA configuration register */
    {0x00002800, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteSdmaConfigReg, 0},

    /* Receive SDMA Interrupt Cause Register (RxSDMAInt) */
    {0x0000280C, SMEM_FULL_MASK_CNS,
        smemChtActiveReadIntrCauseReg, 20, smemChtActiveWriteIntrCauseReg, 0},

    /* Transmit SDMA Interrupt Cause Register */
    {0x00002810, SMEM_FULL_MASK_CNS,
        smemChtActiveReadIntrCauseReg, 19, smemChtActiveWriteIntrCauseReg,0},

    /* Receive SDMA Interrupt Mask Register */
    {0x00002814, SMEM_FULL_MASK_CNS,
        NULL, 0, smemChtActiveWriteRcvSdmaInterruptsMaskReg, 0},

    /* Transmit SDMA Interrupt Mask Register */
    {0x00002818, SMEM_FULL_MASK_CNS,
        NULL, 0, smemChtActiveWriteTransSdmaInterruptsMaskReg, 0},

    /* The SDMA packet count registers */
    {0x00002820, 0xFFFFFFF0, smemChtActiveReadCntrs, 0, NULL,0},
    {0x00002830, 0xFFFFFFF0, smemChtActiveReadCntrs, 0, NULL,0},

     /* The SDMA byte count registers */
    {0x00002840, 0xFFFFFFF0, smemChtActiveReadCntrs, 0, NULL,0},
    {0x00002850, 0xFFFFFFF0, smemChtActiveReadCntrs, 0, NULL,0},

    /* The SDMA resource error registers */
    {0x00002860, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},
    {0x00002864, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},

    /* Transmit SDMA Queue Command Register */
    {0x00002868, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteStartFromCpuDma, 0},

    /* Port<%n> Rate Limit Counter */
    {0x01000400, 0XFFFE0FFF,
        smemChtActiveReadRateLimitCntReg, 0 , smemChtActiveWriteToReadOnlyReg, 0},
    /* IEEE Reserved Multicast Configuration register reading */
    {0x01000810, 0xFFFF0FFF, smemXCatActiveReadIeeeMcConfReg, 0 , NULL,  0},
    {0x01000818, 0xFFFF0FFF, smemXCatActiveReadIeeeMcConfReg, 0 , NULL,  0},
    {0x01000820, 0xFFFF0FFF, smemXCatActiveReadIeeeMcConfReg, 0 , NULL,  0},
    {0x01000828, 0xFFFF0FFF, smemXCatActiveReadIeeeMcConfReg, 0 , NULL,  0},

    /*Bridge Global Configuration2*/
    {0x0104000C, SMEM_FULL_MASK_CNS, NULL, 0 , smemXCatActiveWriteBridgeGlobalConfig2Reg, 0},

    /* Host Incoming Packets Count */
    {0x010400BC, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},
    /* Host Outgoing Packets Count */
    {0x010400C0, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},
    /* Host Outgoing Multicast Packet Count */
    {0x010400CC, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},
    /* Host Outgoing Broadcast Packet Count */
    {0x010400D0, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0},
    /* Matrix Bridge Register */
    {0x010400d4, 0xFFFFFFFF, smemChtActiveReadCntrs, 0, NULL, 0},

    {0x010400E0, 0xFFFFFFF0, smemChtActiveReadCntrs, 0, NULL,0}, /* Set #0 counters */
    {0x010400F4, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0}, /* Set #1 Incoming counters */
    {0x010400F8, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0}, /* Set #1 VLAN ingress filtered counters */
    {0x010400FC, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0}, /* Set #1 Security filtered counters */
    {0x01040100, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0}, /* Set #1 Bridge filtered counters */

    /* Bridge Interrupt Cause Register */
    {0x01040130, SMEM_FULL_MASK_CNS,
        smemChtActiveReadIntrCauseReg, 12, smemChtActiveWriteIntrCauseReg, 0},
    /* Bridge Interrupt Mask Register */
    {0x01040134, SMEM_FULL_MASK_CNS,
        NULL, 0, smemChtActiveWriteBridgeInterruptsMaskReg, 0},

    /* Security Breach Status Register2 */
    {0x010401A8, SMEM_FULL_MASK_CNS, smemChtActiveReadBreachStatusReg, 0, NULL, 0},


    /* Transmit Queue Control Register */
    {0x02800000, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteTxQuCtrlReg, 0},

    /* Transmit Queue Config Register */
    {0x02800080, 0XFFFF01FF, NULL, 0 , smemChtActiveWriteTxQConfigReg, 0},

    /*Egress STC interrupt register*/
    {0x02800130, SMEM_FULL_MASK_CNS, smemChtActiveReadIntrCauseReg, 11 , smemChtActiveWriteIntrCauseReg,0},

     /* The Egress packet count registers */
    {0x02B40144, 0xFFFFFFFF, smemChtActiveReadCntrs, 0, NULL,0},
    {0x02B40148, 0xFFFFFFFF, smemChtActiveReadCntrs, 0, NULL,0},
    {0x02B4014C, 0xFFFFFFFF, smemChtActiveReadCntrs, 0, NULL,0},
    {0x02B40150, 0xFFFFFFF0, smemChtActiveReadCntrs, 0, NULL,0},
    {0x02B40160, 0xFFFFFFF0, smemChtActiveReadCntrs, 0, NULL,0},
    {0x02B40170, 0xFFFFFFF0, smemChtActiveReadCntrs, 0, NULL,0},
    {0x02B40180, 0xFFFFFFFF, smemChtActiveReadCntrs, 0, NULL,0},
    {0x02B40184, 0xFFFFFFFF, smemChtActiveReadCntrs, 0, NULL,0},

    /* XSMI Management Register */
    {0x02CC0000, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteXSmii, 0},

    /* The MAC MIB Counters */
    {0x07010000, SMEM_CHT_COUNT_MSK_CNS, smemChtActiveReadCntrs, 0, NULL, 0}, /* The MAC MIB Counters */
    {0x08010000, SMEM_CHT_COUNT_MSK_CNS, smemChtActiveReadCntrs, 1, NULL, 0},
    {0x09010000, SMEM_CHT_COUNT_MSK_CNS, smemChtActiveReadCntrs, 2, NULL, 0},
    {0x0a010000, SMEM_CHT_COUNT_MSK_CNS, smemChtActiveReadCntrs, 3, NULL, 0},

    /* Port MIB Counters Capture */
    {0x07004020, SMEM_FULL_MASK_CNS, NULL, 0, smemChtActiveWriteMibCntCtrl, 0},
    {0x08004020, SMEM_FULL_MASK_CNS, NULL, 0, smemChtActiveWriteMibCntCtrl, 1},
    {0x09004020, SMEM_FULL_MASK_CNS, NULL, 0, smemChtActiveWriteMibCntCtrl, 2},
    {0x0a004020, SMEM_FULL_MASK_CNS, NULL, 0, smemChtActiveWriteMibCntCtrl, 3},

    /* SMI0 Management Register : 0x07004054 , 0x09004054 */
    {0x07004054, 0xFFFFFFFF, NULL, 0 , smemChtActiveWriteSmi,0},
    {0x09004054, 0xFFFFFFFF, NULL, 0 , smemChtActiveWriteSmi,0},

    /* FDB Global Configuration register */
    {0x0b000000, SMEM_FULL_MASK_CNS, NULL, 0 , smemCht3ActiveWriteFDBGlobalCfgReg, 0},

    /* MAC Table Action0 Register */
    {0x0b000004, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteFdbActionTrigger, 0},

    /* bridge Interrupt Cause Register */
    {0x0b000018, SMEM_FULL_MASK_CNS,
        smemChtActiveReadIntrCauseReg, 15, smemChtActiveWriteIntrCauseReg, 0},

    /* MAC Table Interrupt Mask Register */
    {0x0b00001C, SMEM_FULL_MASK_CNS,
        NULL, 0, smemChtActiveWriteMacInterruptsMaskReg, 0},

    /* Message to CPU register  */
    {0x0b000034, SMEM_FULL_MASK_CNS, smemChtActiveReadMsg2Cpu, 0 , NULL,0},

    /* Message From CPU Management */
    {0x0b000050, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteNewFdbMsgFromCpu, 0},

    /* Mac Table Access Control Register */
    {0x0b000064, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteFdbMsg,0},

    /* CNC Fast Dump Trigger Register Register */
    {0x10000030, SMEM_FULL_MASK_CNS, NULL, 0 , smemCht3ActiveWriteCncFastDumpTrigger, 0},

    /* CPU direct read from the counters */
    {0x10080000, 0xFFF80000, smemCht3ActiveCncBlockRead, 0, NULL, 0},

    /* CNC Block Wraparound Status Register */
    {0x100010A0, 0xFFFFF0F0, smemCht3ActiveCncWrapAroundStatusRead, 0, NULL, 0},

    /* read interrupts cause registers CNC -- ROC register */
    {0x10000100, SMEM_FULL_MASK_CNS,
        smemChtActiveReadIntrCauseReg, 3, smemChtActiveWriteIntrCauseReg, 0},

    /* Write Interrupt Mask CNC Register */
    {0x10000104, SMEM_FULL_MASK_CNS, NULL, 0 , smemCht3ActiveWriteCncInterruptsMaskReg, 0},

    /* XG port MAC MIB Counters */
    {0x11000000, SMEM_XCAT3_MSM_MIB_COUNT_MSK_CNS, smemLion2ActiveReadMsmMibCounters, 0, NULL, 0},

    /* Port<n> Auto-Negotiation Configuration Register */
    {0x1200000C, 0xFFFF0FFF, NULL, 0 , smemChtActiveWriteForceLinkDown, 0},
    {0x1200000C, 0xFFFE8FFF, NULL, 0 , smemChtActiveWriteForceLinkDown, 0},

    /* Port<n> Interrupt Cause Register  */
    {0x12000020, 0xFFFF0FFF,
        smemChtActiveReadIntrCauseReg, 16, smemChtActiveWriteIntrCauseReg, 0},
    {0x12000020, 0xFFFE8FFF,
        smemChtActiveReadIntrCauseReg, 16, smemChtActiveWriteIntrCauseReg, 0},

    /* Tri-Speed Port<n> Interrupt Mask Register */
    {0x12000024, 0xFFFF0FFF, NULL, 0, smemChtActiveWritePortInterruptsMaskReg, 0},
    {0x12000024, 0xFFFE8FFF, NULL, 0, smemChtActiveWritePortInterruptsMaskReg, 0},

    /* Port MAC Control Register0 */
    {0x120D8000, 0xFFFFCFFF, NULL, 0 , smemChtActiveWriteForceLinkDownXg, 0},

    /* XG Port<n> Interrupt Cause Register  */
    {0x120D8014, 0xFFFFCFFF,
        smemChtActiveReadIntrCauseReg, 18, smemChtActiveWriteIntrCauseReg, 0},

    /* XG Port<n> Interrupt Mask Register */
    {0x120D8018, 0xFFFFCFFF, NULL, 0, smemChtActiveWritePortInterruptsMaskReg, 0},

    /* Ports MIB Counters Configuration Register */
    {0x120D8030, 0xFFFFCFFF, NULL, 0, smemChtActiveWriteHgsMibCntCtrl, 0},



    /* Trunk table Access Control Register */
    {0x14000028, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteTrunkTbl, 0},

    /* QoSProfile to QoS Access Control Register */
    {0x1400002C, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteQosAccess, 0},

    /* CPU Code Access Control Register */
    {0x14000030, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteCpuCodeAccess, 0},

    /* Statistical Rate Limits Table Access Control Register */
    {0x14000034, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteStatRateLimTbl, 0},

    /* Ingress STC Table Access Control Register */
    {0x14000038, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteIngrStcTbl, 0},

    /*ingress STC interrupt register*/
    {0x14000020, SMEM_FULL_MASK_CNS, smemChtActiveReadIntrCauseReg, 24 , smemChtActiveWriteIntrCauseReg,0},

    {0x140000A0, SMEM_FULL_MASK_CNS, NULL, 0 , smemXCatActiveWriteLogTargetMap,  0},
    {0x140000A4, SMEM_FULL_MASK_CNS, NULL, 0 , smemXCatActiveWriteEgressFilterVlanMap,  0},
    {0x140000AC, SMEM_FULL_MASK_CNS, NULL, 0 , smemXCatActiveWriteEgressFilterVlanMember,  0},

    /*Pre-Egress Engine Global Configuration*/
    {0x14050000, SMEM_FULL_MASK_CNS, NULL, 0 , smemXCatActiveWriteEqGlobalConfigReg, 0},

    /*Ingress Policy Global Configuration*/
    {0x15000000, SMEM_FULL_MASK_CNS, NULL, 0 , smemXCatActiveWriteIpclGlobalConfigReg, 0},

    /*TTI Global Configuration*/
    {0x16000000, SMEM_FULL_MASK_CNS, NULL, 0 , smemXCatActiveWriteTtiGlobalConfigReg, 0},

    /* TTI Internal, Metal Fix */
    {0x160002B0, SMEM_FULL_MASK_CNS, NULL, 0 , smemXCatActiveWriteTtiInternalMetalFix,  0},

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
    {0x17000000, SMEM_FULL_MASK_CNS, NULL, 0 , smemXCatActiveWritePolicerMemoryControl,  0},

    /* iplr0 policer table 0x17040000 - 0x1704FFFF  xCat3 */
    {0x17040000, 0xFFFF0000, smemXCatActiveReadIplr0Tables, 0 , smemXCatActiveWriteIplr0Tables,  0},
    /* iplr0 policerCounters table 0x17060000 - 0x1706FFFF  xCat3 */
    {0x17060000, 0xFFFF0000, smemXCatActiveReadIplr0Tables, 0 , smemXCatActiveWriteIplr0Tables,  0},

    /* iplr1 policer table 0x18040000 - 0x1804FFFF  xCat3 */
    {0x18040000, 0xFFFF0000, smemXCatActiveReadIplr1Tables, 0 , smemXCatActiveWriteIplr1Tables,  0},
    /* iplr1 policerCounters table 0x18060000 - 0x1806FFFF  xCat3 */
    {0x18060000, 0xFFFF0000, smemXCatActiveReadIplr1Tables, 0 , smemXCatActiveWriteIplr1Tables,  0},

    {0x1A000208, SMEM_FULL_MASK_CNS, NULL, 0 , smemXCatActiveWritePolicyTcamConfig_0,  0},
    {0x1A000138, SMEM_FULL_MASK_CNS, NULL, 0 , smemCht3ActiveWritePclAction, 0},

    /* Router and TT TCAM Access Control 1 Register  */
    {0x1B00041C, SMEM_FULL_MASK_CNS, NULL, 0 , smemCht3ActiveWriteRouterAction, 0},

    /*MLL Global Configuration*/
    {0x19000000, SMEM_FULL_MASK_CNS, NULL, 0 , smemXCatActiveWriteMllGlobalConfigReg, 0},

    /*L2 MLL*/
    {0x19000464, SMEM_FULL_MASK_CNS, NULL, 0 , smemXCatActiveWriteL2MllVidxEnable,  0},
    {0x19000474, SMEM_FULL_MASK_CNS, NULL, 0 , smemXCatActiveWriteL2MllPointerMap,  0},

    /*HA Global Configuration*/
    {0x1C000100, SMEM_FULL_MASK_CNS, NULL, 0 , smemXCatActiveWriteHaGlobalConfigReg, 0},


    /* Router TCAM BIST Config Register */
    {0x04002050, SMEM_FULL_MASK_CNS, smemCht3ActiveReadTcamBistConfigAction, 0 , NULL, 0},

    /* must be last anyway */
    {END_OF_TABLE, SMEM_FULL_MASK_CNS, NULL,0,NULL,0}
};

/* the units that the xCat3 use */
static GT_U32   usedUnitsNumbersArray[] =
{
/*    SMEM_XCAT3_UNIT_MG_E,               */ (0x00000000  >> 23) ,
/*    SMEM_XCAT3_UNIT_EGR_TXQ_E,          */ (0x02800000  >> 23) ,
/*    SMEM_XCAT3_UNIT_L2I_E,              */ (0x01000000  >> 23) ,
/*    SMEM_XCAT3_UNIT_IPVX_E,             */ (0x04000000  >> 23) ,
/*    SMEM_XCAT3_UNIT_BM_E,               */ (0x05000000  >> 23) ,
/*    SMEM_XCAT3_UNIT_EPLR_E,             */ (0x06000000  >> 23) ,
/*    SMEM_XCAT3_UNIT_LMS_E,              */ (0x07000000  >> 23) ,
/*    SMEM_XCAT3_UNIT_FDB_E,              */ (0x0B000000  >> 23) ,
/*    SMEM_XCAT3_UNIT_MPPM_BANK0_E,       */ (0x0C000000  >> 23) ,
/*    SMEM_XCAT3_UNIT_MPPM_BANK1_E,       */ (0x0D000000  >> 23) ,
/*    SMEM_XCAT3_UNIT_MEM_E,              */ (0x0E000000  >> 23) ,
/*    SMEM_XCAT3_UNIT_CENTRALIZED_COUNT_E,*/ (0x10000000  >> 23) ,
/*    SMEM_XCAT3_UNIT_MSM,                */ (0x11000000  >> 23) ,
/*    SMEM_XCAT3_UNIT_GOP_E,              */ (0x12000000  >> 23) ,
/*    SMEM_XCAT3_UNIT_SERDES_E,           */ (0x13000000  >> 23) ,
/*    SMEM_XCAT3_UNIT_VLAN_MC_E           */ (0x03800000  >> 23) ,
/*    SMEM_XCAT3_UNIT_EQ_E,               */ (0x14000000  >> 23) ,
/*    SMEM_XCAT3_UNIT_IPCL_E,             */ (0x15000000  >> 23) ,
/*    SMEM_XCAT3_UNIT_TTI_E,              */ (0x16000000  >> 23) ,
/*    SMEM_XCAT3_UNIT_IPLR0_E,            */ (0x17000000  >> 23) ,
/*    SMEM_XCAT3_UNIT_IPLR1_E,            */ (0x18000000  >> 23) ,
/*    SMEM_XCAT3_UNIT_MLL_E,              */ (0x19000000  >> 23) ,
/*    SMEM_XCAT3_UNIT_TCC_LOWER_E,        */ (0x1A000000  >> 23) ,
/*    SMEM_XCAT3_UNIT_TCC_UPPER_E,        */ (0x1B000000  >> 23) ,
/*    SMEM_XCAT3_UNIT_HA_E,               */ (0x1C000000  >> 23) ,
/*    SMEM_XCAT3_UNIT_EPCL_E,             */ (0x1D000000  >> 23) ,
/*    SMEM_XCAT3_UNIT_CCFC_E,             */ (0x03400000  >> 23) ,
      END_OF_TABLE /* must be last*/
};

/* Return unit chunk pointer by real unit index */
#define UNIT_CHUNK_PTR_MAC(dev_info, unit) \
    &dev_info->unitMemArr[usedUnitsNumbersArray[unit]]

enum{
    pexIntSum            = 1,
    pexErrSum            = 2,
    FuncUnitsIntsSum     = 3,
    DataPathIntSum       = 4,
    PortsIntSum          = 5,
    MgInternalIntSum     = 7,/*misc*/
    TxSDMASum            = 8,
    RxSDMASum            = 9,
};

enum{
    FuncUnitsInterrupts_CNCInterruptSummaryCauseReg = 1,
    FuncUnitsInterrupts_eqInterruptSummary = 3,
    FuncUnitsInterrupts_bridgeInterruptCause = 4,
    FuncUnitsInterrupts_policerInterruptCause_2 = 6, /*EPLR*/
    FuncUnitsInterrupts_FDBInterruptCauseReg = 7,
    FuncUnitsInterrupts_TTIEngineInterruptCause = 8,
    FuncUnitsInterrupts_policerInterruptCause_0 = 11, /*IPLR0*/
    FuncUnitsInterrupts_policerInterruptCause_1 = 12, /*IPLR1*/
    FuncUnitsInterrupts_txQInterruptSummary = 14,
};

static SKERNEL_INTERRUPT_REG_INFO_STC GlobalInterruptsSummary =
{
    /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(MG.globalInterrupt.globalInterruptCause)},/*SMEM_CHT_GLB_INT_CAUSE_REG*/
    /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(MG.globalInterrupt.globalInterruptMask)},
    /*myFatherInfo*/{
        /*myBitIndex*/0 ,
        /*interruptPtr*/NULL /* no father for me , thank you */
        }
};

static SKERNEL_INTERRUPT_REG_INFO_STC  FuncUnitsInterruptsSummary =
{
    /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(MG.globalInterrupt.functionalInterruptsSummaryCause)},
    /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(MG.globalInterrupt.functionalInterruptsSummaryMask)},
    /*myFatherInfo*/{
        /*myBitIndex*/FuncUnitsIntsSum ,
        /*interruptPtr*/&GlobalInterruptsSummary
        }
};

static SKERNEL_INTERRUPT_REG_INFO_STC  eqInterruptSummary =
{
    /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(EQ.preEgrInterrupt.preEgrInterruptSummary)}, /*SMEM_CHT_EQ_INT_CAUSE_REG*/
    /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(EQ.preEgrInterrupt.preEgrInterruptSummaryMask)},  /*SMEM_CHT_EQ_INT_MASK_REG*/
    /*myFatherInfo*/{
        /*myBitIndex*/FuncUnitsInterrupts_eqInterruptSummary ,
        /*interruptPtr*/&FuncUnitsInterruptsSummary
        }
};

static SKERNEL_INTERRUPT_REG_INFO_STC  txQInterruptSummary =
{
    /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(egrAndTxqVer0.transmitQueueInterrupt.transmitQueueInterruptSummaryCause)},/*SMEM_CHT_TX_QUEUE_INTERRUPT_CAUSE_REG , SMEM_LION_TXQ_DQ_INTR_SUM_CAUSE_REG*/
    /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(egrAndTxqVer0.transmitQueueInterrupt.transmitQueueInterruptSummaryMask)},
    /*myFatherInfo*/{
        /*myBitIndex*/FuncUnitsInterrupts_txQInterruptSummary ,
        /*interruptPtr*/&FuncUnitsInterruptsSummary
        }
};

static SKERNEL_INTERRUPT_REG_INFO_STC  dataPathInterruptsSummary =
{
    /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(MG.globalInterrupt.SERInterruptsSummary)},
    /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(MG.globalInterrupt.SERInterruptsMask)},
    /*myFatherInfo*/{
        /*myBitIndex*/DataPathIntSum ,
        /*interruptPtr*/&GlobalInterruptsSummary
        }
};

static SKERNEL_INTERRUPT_REG_INFO_STC  mibCountersInterruptsSummary =
{
    /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(LMS.LMS0.MIBCntrsInterruptSummary)},
    /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(LMS.LMS0LMS0Group0.portsMIBCntrsInterruptSummaryMask)},
    /*myFatherInfo*/{
        /*myBitIndex*/1 ,
        /*interruptPtr*/&dataPathInterruptsSummary
        }
};

static SKERNEL_INTERRUPT_REG_INFO_STC  lmsPortsInterruptsSummary =
{
    /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(LMS.LMS1.portsInterruptSummary)},
    /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(LMS.LMS0LMS0Group0.portsInterruptSummaryMask)},
    /*myFatherInfo*/{
        /*myBitIndex*/5 ,
        /*interruptPtr*/&mibCountersInterruptsSummary
        }
};

static SKERNEL_INTERRUPT_REG_INFO_STC  portsInterruptsSummary =
{
    /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(MG.globalInterrupt.stackPortsInterruptCause)},/*SMEM_CHT3_XG_PORTS_INTERRUPT_CAUSE_REG*/
    /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(MG.globalInterrupt.stackPortsInterruptMask)},/*SMEM_CHT3_XG_PORTS_INTERRUPT_MASK_REG*/
    /*myFatherInfo*/{
        /*myBitIndex*/PortsIntSum ,
        /*interruptPtr*/&GlobalInterruptsSummary
        }
};

/* Gige port summary */
#define  GIGE_SUMMARY_PORT_INTERRUPT_SUMMARY_MAC(port,bitInReg,regIn) \
static SKERNEL_INTERRUPT_REG_INFO_STC  gigaInterruptSummary_##port =                                            \
{                                                                                                               \
    /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(GOP.ports.gigPort[port].portInterruptSummaryCause)},    \
    /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(GOP.ports.gigPort[port].portInterruptSummaryMask)},     \
    /*myFatherInfo*/{                                                                                           \
        /*myBitIndex*/bitInReg ,                                                                                \
        /*interruptPtr*/&regIn                                                                                  \
        },                                                                                                      \
}

/* Gige port summary directed LMS ports summary */
#define GIGE_SUMMARY_PORT_IN_LMS_INTERRUPT_SUMMARY_MAC(port) \
        GIGE_SUMMARY_PORT_INTERRUPT_SUMMARY_MAC(port,(port+1),lmsPortsInterruptsSummary)

/* Gige port summary directed from XLG summary */
#define GIGE_SUMMARY_PORT_IN_XLG_INTERRUPT_SUMMARY_MAC(port) \
        GIGE_SUMMARY_PORT_INTERRUPT_SUMMARY_MAC(port,2,xlgInterruptSummary_##port)

/* Gige mac port leaf */
#define  GIGE_MAC_INTERRUPT_CAUSE_MAC(port) \
{                                                         \
    /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(GOP.ports.gigPort[port].portInterruptCause)}, /*SMEM_CHT_PORT_INT_CAUSE_REG*/ \
    /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(GOP.ports.gigPort[port].portInterruptMask)},  /**/  \
    /*myFatherInfo*/{                                     \
        /*myBitIndex*/1 ,                                 \
        /*interruptPtr*/&gigaInterruptSummary_##port      \
        }                                                 \
}

/* XLG port summary directed from MG ports summary */
#define  XLG_SUMMARY_PORT_IN_MG_INTERRUPT_SUMMARY_MAC(port,bitInMg,regInMg) \
static SKERNEL_INTERRUPT_REG_INFO_STC  xlgInterruptSummary_##port =  \
{                                                  \
    /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(GOP.ports.XLGIP[port].externalUnitsInterruptsCause)}, /*SMEM_LION_XG_PORTS_SUMMARY_INTERRUPT_CAUSE_REG*/  \
    /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(GOP.ports.XLGIP[port].externalUnitsInterruptsMask)},  /*SMEM_LION2_XG_PORTS_SUMMARY_INTERRUPT_MASK_REG*/  \
    /*myFatherInfo*/{                              \
        /*myBitIndex*/ bitInMg ,                   \
        /*interruptPtr*/&regInMg                   \
        },                                         \
}

/* XLG mac ports leaf */
#define  XLG_MAC_INTERRUPT_CAUSE_MAC(port) \
{                                                         \
    /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(GOP.ports.XLGIP[port].portInterruptCause)}, /*SMEM_CHT_PORT_INT_CAUSE_REG*/ \
    /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(GOP.ports.XLGIP[port].portInterruptMask)},  /**/  \
    /*myFatherInfo*/{                                     \
        /*myBitIndex*/1 ,                                 \
        /*interruptPtr*/&xlgInterruptSummary_##port      \
        }                                                 \
}

GIGE_SUMMARY_PORT_IN_LMS_INTERRUPT_SUMMARY_MAC(0);
GIGE_SUMMARY_PORT_IN_LMS_INTERRUPT_SUMMARY_MAC(1);
GIGE_SUMMARY_PORT_IN_LMS_INTERRUPT_SUMMARY_MAC(2);
GIGE_SUMMARY_PORT_IN_LMS_INTERRUPT_SUMMARY_MAC(3);
GIGE_SUMMARY_PORT_IN_LMS_INTERRUPT_SUMMARY_MAC(4);
GIGE_SUMMARY_PORT_IN_LMS_INTERRUPT_SUMMARY_MAC(5);
GIGE_SUMMARY_PORT_IN_LMS_INTERRUPT_SUMMARY_MAC(6);
GIGE_SUMMARY_PORT_IN_LMS_INTERRUPT_SUMMARY_MAC(7);
GIGE_SUMMARY_PORT_IN_LMS_INTERRUPT_SUMMARY_MAC(8);
GIGE_SUMMARY_PORT_IN_LMS_INTERRUPT_SUMMARY_MAC(9);
GIGE_SUMMARY_PORT_IN_LMS_INTERRUPT_SUMMARY_MAC(10);
GIGE_SUMMARY_PORT_IN_LMS_INTERRUPT_SUMMARY_MAC(11);
GIGE_SUMMARY_PORT_IN_LMS_INTERRUPT_SUMMARY_MAC(12);
GIGE_SUMMARY_PORT_IN_LMS_INTERRUPT_SUMMARY_MAC(13);
GIGE_SUMMARY_PORT_IN_LMS_INTERRUPT_SUMMARY_MAC(14);
GIGE_SUMMARY_PORT_IN_LMS_INTERRUPT_SUMMARY_MAC(15);
GIGE_SUMMARY_PORT_IN_LMS_INTERRUPT_SUMMARY_MAC(16);
GIGE_SUMMARY_PORT_IN_LMS_INTERRUPT_SUMMARY_MAC(17);
GIGE_SUMMARY_PORT_IN_LMS_INTERRUPT_SUMMARY_MAC(18);
GIGE_SUMMARY_PORT_IN_LMS_INTERRUPT_SUMMARY_MAC(19);
GIGE_SUMMARY_PORT_IN_LMS_INTERRUPT_SUMMARY_MAC(20);
GIGE_SUMMARY_PORT_IN_LMS_INTERRUPT_SUMMARY_MAC(21);
GIGE_SUMMARY_PORT_IN_LMS_INTERRUPT_SUMMARY_MAC(22);
GIGE_SUMMARY_PORT_IN_LMS_INTERRUPT_SUMMARY_MAC(23);

/* CPU port */
XLG_SUMMARY_PORT_IN_MG_INTERRUPT_SUMMARY_MAC(31,25,lmsPortsInterruptsSummary);
GIGE_SUMMARY_PORT_IN_XLG_INTERRUPT_SUMMARY_MAC(31);

XLG_SUMMARY_PORT_IN_MG_INTERRUPT_SUMMARY_MAC(24,25,portsInterruptsSummary);
XLG_SUMMARY_PORT_IN_MG_INTERRUPT_SUMMARY_MAC(25,26,portsInterruptsSummary);
XLG_SUMMARY_PORT_IN_MG_INTERRUPT_SUMMARY_MAC(26,27,portsInterruptsSummary);
XLG_SUMMARY_PORT_IN_MG_INTERRUPT_SUMMARY_MAC(27,28,portsInterruptsSummary);
XLG_SUMMARY_PORT_IN_MG_INTERRUPT_SUMMARY_MAC(28,26,portsInterruptsSummary);
XLG_SUMMARY_PORT_IN_MG_INTERRUPT_SUMMARY_MAC(29,28,portsInterruptsSummary);

GIGE_SUMMARY_PORT_IN_XLG_INTERRUPT_SUMMARY_MAC(24);
GIGE_SUMMARY_PORT_IN_XLG_INTERRUPT_SUMMARY_MAC(25);
GIGE_SUMMARY_PORT_IN_XLG_INTERRUPT_SUMMARY_MAC(26);
GIGE_SUMMARY_PORT_IN_XLG_INTERRUPT_SUMMARY_MAC(27);
GIGE_SUMMARY_PORT_IN_XLG_INTERRUPT_SUMMARY_MAC(28);
GIGE_SUMMARY_PORT_IN_XLG_INTERRUPT_SUMMARY_MAC(29);

/* define the 'leafs' of the interrupts of xCat3 device */
static SKERNEL_INTERRUPT_REG_INFO_STC xCat3InterruptsTreeDb[] = {
    {
        /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(MG.SDMARegs.receiveSDMAInterruptCause)}, /*SMEM_CHT_SDMA_INT_CAUSE_REG*/
        /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(MG.SDMARegs.receiveSDMAInterruptMask)},
        /*myFatherInfo*/{
            /*myBitIndex*/RxSDMASum ,
            /*interruptPtr*/&GlobalInterruptsSummary
            }
    }
    ,{
        /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(MG.SDMARegs.transmitSDMAInterruptCause)},/*SMEM_CHT_TX_SDMA_INT_CAUSE_REG*/
        /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(MG.SDMARegs.transmitSDMAInterruptMask)},
        /*myFatherInfo*/{
            /*myBitIndex*/TxSDMASum ,
            /*interruptPtr*/&GlobalInterruptsSummary
            }
    }
    ,{
        /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(TTI.TTIEngineInterruptCause)},/*SMEM_LION3_TTI_INTERRUPT_CAUSE_REG*/
        /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(TTI.TTIEngineInterruptMask)},
        /*myFatherInfo*/{
            /*myBitIndex*/FuncUnitsInterrupts_TTIEngineInterruptCause ,
            /*interruptPtr*/&FuncUnitsInterruptsSummary
            }
    }
    ,{
        /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(L2I.bridgeEngineInterrupts.bridgeInterruptCause)},/*SMEM_CHT_BRIDGE_INT_CAUSE_REG*/
        /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(L2I.bridgeEngineInterrupts.bridgeInterruptMask)},
        /*myFatherInfo*/{
            /*myBitIndex*/FuncUnitsInterrupts_bridgeInterruptCause ,
            /*interruptPtr*/&FuncUnitsInterruptsSummary
            }
    }
    ,{
        /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(FDB.FDBInterrupt.FDBInterruptCauseReg)},/*SMEM_CHT_MAC_TBL_INTR_CAUSE_REG*/
        /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(FDB.FDBInterrupt.FDBInterruptMaskReg)},
        /*myFatherInfo*/{
            /*myBitIndex*/FuncUnitsInterrupts_FDBInterruptCauseReg ,
            /*interruptPtr*/&FuncUnitsInterruptsSummary
            }
    }
   ,{
        /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(centralizedCntrs.globalRegs.CNCInterruptSummaryCauseReg)},/*SMEM_CHT3_CNC_INTR_CAUSE_REG*/
        /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(centralizedCntrs.globalRegs.CNCInterruptSummaryMaskReg)},
        /*myFatherInfo*/{
            /*myBitIndex*/FuncUnitsInterrupts_CNCInterruptSummaryCauseReg ,
            /*interruptPtr*/&FuncUnitsInterruptsSummary
            }
    }
    ,{
        /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(PLR[0].policerInterruptCause)},/*SMEM_XCAT_POLICER_IPFIX_INT_CAUSE_REG*/
        /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(PLR[0].policerInterruptMask)},
        /*myFatherInfo*/{
            /*myBitIndex*/FuncUnitsInterrupts_policerInterruptCause_0 ,
            /*interruptPtr*/&FuncUnitsInterruptsSummary
            }
    }
    ,{
        /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(PLR[1].policerInterruptCause)},/*SMEM_XCAT_POLICER_IPFIX_INT_CAUSE_REG*/
        /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(PLR[1].policerInterruptMask)},
        /*myFatherInfo*/{
            /*myBitIndex*/FuncUnitsInterrupts_policerInterruptCause_1 ,
            /*interruptPtr*/&FuncUnitsInterruptsSummary
            }
    }
    ,{
        /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(PLR[2].policerInterruptCause)},/*SMEM_XCAT_POLICER_IPFIX_INT_CAUSE_REG*/
        /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(PLR[2].policerInterruptMask)},
        /*myFatherInfo*/{
            /*myBitIndex*/FuncUnitsInterrupts_policerInterruptCause_2 ,
            /*interruptPtr*/&FuncUnitsInterruptsSummary
            }
    }
    ,{
        /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(MG.globalInterrupt.miscellaneousInterruptCause)},/*SMEM_CHT_MISC_INTR_CAUSE_REG*/
        /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(MG.globalInterrupt.miscellaneousInterruptMask)},
        /*myFatherInfo*/{
            /*myBitIndex*/MgInternalIntSum ,
            /*interruptPtr*/&GlobalInterruptsSummary
            }
    }
    ,{
        /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(EQ.ingrSTCInterrupt.ingrSTCInterruptCause)}, /*SMEM_CHT_INGRESS_STC_INT_CAUSE_REG*/
        /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(EQ.ingrSTCInterrupt.ingrSTCInterruptMask)},  /*SMEM_CHT_INGRESS_STC_INT_MASK_REG*/
        /*myFatherInfo*/{
            /*myBitIndex*/1 ,
            /*interruptPtr*/&eqInterruptSummary
            }
    }
    ,{                                                         \
        /*causeReg*/ {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(egrAndTxqVer0.transmitQueueInterrupt.egrSTCInterruptCause)}, /*SMEM_CHT_STC_INT_CAUSE_REG , SMEM_LION_TXQ_EGR_STC_INTR_CAUSE_REG*/
        /*maskReg*/  {SKERNEL_INTERRUPT_BIND_REG_LEGACY_MAC(egrAndTxqVer0.transmitQueueInterrupt.egrSTCInterruptMask)},  /*SMEM_CHT_STC_INT_MASK_REG , SMEM_LION_TXQ_EGR_STC_INTR_MASK_REG*/
        /*myFatherInfo*/{
        /*myBitIndex*/5 ,
        /*interruptPtr*/&txQInterruptSummary
        }
    }

    /* the Gige ports 0..23 */
    ,GIGE_MAC_INTERRUPT_CAUSE_MAC(0 )
    ,GIGE_MAC_INTERRUPT_CAUSE_MAC(1 )
    ,GIGE_MAC_INTERRUPT_CAUSE_MAC(2 )
    ,GIGE_MAC_INTERRUPT_CAUSE_MAC(3 )
    ,GIGE_MAC_INTERRUPT_CAUSE_MAC(4 )
    ,GIGE_MAC_INTERRUPT_CAUSE_MAC(5 )
    ,GIGE_MAC_INTERRUPT_CAUSE_MAC(6 )
    ,GIGE_MAC_INTERRUPT_CAUSE_MAC(7 )
    ,GIGE_MAC_INTERRUPT_CAUSE_MAC(8 )
    ,GIGE_MAC_INTERRUPT_CAUSE_MAC(9 )
    ,GIGE_MAC_INTERRUPT_CAUSE_MAC(10)
    ,GIGE_MAC_INTERRUPT_CAUSE_MAC(11)
    ,GIGE_MAC_INTERRUPT_CAUSE_MAC(12)
    ,GIGE_MAC_INTERRUPT_CAUSE_MAC(13)
    ,GIGE_MAC_INTERRUPT_CAUSE_MAC(14)
    ,GIGE_MAC_INTERRUPT_CAUSE_MAC(15)
    ,GIGE_MAC_INTERRUPT_CAUSE_MAC(16)
    ,GIGE_MAC_INTERRUPT_CAUSE_MAC(17)
    ,GIGE_MAC_INTERRUPT_CAUSE_MAC(18)
    ,GIGE_MAC_INTERRUPT_CAUSE_MAC(19)
    ,GIGE_MAC_INTERRUPT_CAUSE_MAC(20)
    ,GIGE_MAC_INTERRUPT_CAUSE_MAC(21)
    ,GIGE_MAC_INTERRUPT_CAUSE_MAC(22)
    ,GIGE_MAC_INTERRUPT_CAUSE_MAC(23)

    /* CPU port*/
    ,GIGE_MAC_INTERRUPT_CAUSE_MAC(31)

    ,GIGE_MAC_INTERRUPT_CAUSE_MAC(24)
    ,GIGE_MAC_INTERRUPT_CAUSE_MAC(25)
    ,GIGE_MAC_INTERRUPT_CAUSE_MAC(26)
    ,GIGE_MAC_INTERRUPT_CAUSE_MAC(27)
    ,GIGE_MAC_INTERRUPT_CAUSE_MAC(28)
    ,GIGE_MAC_INTERRUPT_CAUSE_MAC(29)

    /* the XLG ports 24..29 */
    ,XLG_MAC_INTERRUPT_CAUSE_MAC (24)
    ,XLG_MAC_INTERRUPT_CAUSE_MAC (25)
    ,XLG_MAC_INTERRUPT_CAUSE_MAC (26)
    ,XLG_MAC_INTERRUPT_CAUSE_MAC (27)
    ,XLG_MAC_INTERRUPT_CAUSE_MAC (28)
    ,XLG_MAC_INTERRUPT_CAUSE_MAC (29)

    ,SKERNEL_INTERRUPT_REG_INFO__LAST_LINE__CNS /* must be last */
};

/*******************************************************************************
*   smemXCat3InitFuncArray
*
* DESCRIPTION:
*       Init specific xCat3 functions array.
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
static void smemXCat3InitFuncArray
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
    for (unit = SMEM_XCAT3_UNIT_MG_E; unit < SMEM_XCAT3_UNIT_LAST_E; unit++)
    {
        unitIndex = usedUnitsNumbersArray[unit];
        devMemInfoPtr->unitMemArr[unitIndex].chunkIndex = unitIndex;

        commonDevMemInfoPtr->specFunTbl[unitIndex].specFun = smemDevFindInUnitChunk;

        /* Set the parameter to be (casting of) the pointer to the unit chunk */
        commonDevMemInfoPtr->specFunTbl[unitIndex].specParam  =
            smemConvertChunkIndexToPointerAsParam(devObjPtr, unitIndex);

        if(commonDevMemInfoPtr->specFunTbl[unitIndex].specParam ==
            (GT_UINTPTR)(void*)(UNIT_CHUNK_PTR_MAC(devMemInfoPtr, SMEM_XCAT3_UNIT_LMS_E)))
        {

            commonDevMemInfoPtr->specFunTbl[unitIndex+2].specFun    =
            commonDevMemInfoPtr->specFunTbl[unitIndex+4].specFun    =
            commonDevMemInfoPtr->specFunTbl[unitIndex+6].specFun    =
            commonDevMemInfoPtr->specFunTbl[unitIndex].specFun;

            /* set the parameter to be (casting of) the pointer to the unit chunk */
            commonDevMemInfoPtr->specFunTbl[unitIndex+2].specParam  =
            commonDevMemInfoPtr->specFunTbl[unitIndex+4].specParam  =
            commonDevMemInfoPtr->specFunTbl[unitIndex+6].specParam  =
            commonDevMemInfoPtr->specFunTbl[unitIndex].specParam;

            devMemInfoPtr->unitMemArr[unitIndex+2].chunkIndex = unitIndex + 2;
            devMemInfoPtr->unitMemArr[unitIndex+4].chunkIndex = unitIndex + 4;
            devMemInfoPtr->unitMemArr[unitIndex+6].chunkIndex = unitIndex + 6;

        }
    }
}

/*******************************************************************************
*   smemXCat3FindMem
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
static void * smemXCat3FindMem
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
*   smemXcat3UnitDfx
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  DFX unit
*
* INPUTS:
*       devObjPtr           - pointer to device memory object.
*       unitPtr             - pointer to the unit chunk
*       unitBaseAddr        - unit base address
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
void smemXcat3UnitDfx
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = devObjPtr->deviceMemory;
    SMEM_UNIT_CHUNKS_STC * unitChunkPtr = &devMemInfoPtr->common.pciExtMemArr[SMEM_UNIT_PCI_BUS_DFX_E].unitMem;
    GT_U32 unitBaseAddr = devMemInfoPtr->common.pciExtMemArr[SMEM_UNIT_PCI_BUS_DFX_E].unitBaseAddr;

    /* chunks with flat memory (no formulas) */
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000000, 1015804)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000F8000, 0x000F8018)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000F8020, 0x000F8038)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000F8044, 0x000F8048)},
            /* manually fixed to add 0x000F8058 */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000F8058, 0x000F8058)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000F8060, 0x000F8078)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000F80A0, 0x000F80A4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000F8100, 0x000F8114)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000F8120, 0x000F813C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000F8200, 0x000F8230)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000F8240, 0x000F8248)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000F8250, 0x000F8294)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000F82D0, 0x000F82D8)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000F82E0, 0x000F82F4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000F8340, 0x000F8348)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000F83C0, 0x000F83DC)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000F8450, 0x000F8494)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000F8C80, 0x000F8C84)},

            /* DFX memories */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x000F80B0, 4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x000F80C0, 4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x000F8F00, 64)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x000F9000, 1024)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x000FC000, 12288)},

        };

        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);
        GT_U32  ii;

        for(ii = 0 ; ii < numOfChunks ; ii ++)
        {
            chunksMem[ii].memFirstAddr += unitBaseAddr;
        }

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitChunkPtr);
    }
}

/*******************************************************************************
*   smemXCat3UnitMg
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
static void smemXCat3UnitMg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = devObjPtr->deviceMemory;
    SMEM_UNIT_CHUNKS_STC * unitChunkPtr = UNIT_CHUNK_PTR_MAC(devMemInfoPtr, SMEM_XCAT3_UNIT_MG_E);
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000000, 0x00000000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000008, 0x00000008)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000010, 0x00000028)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000030, 0x00000044)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000004C, 0x00000084)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000090, 0x00000098)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000000A4, 0x00000100)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000110, 0x00000110)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000120, 0x00000140)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000150, 0x0000017C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000200, 0x00000288)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000380, 0x00000390)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000003A0, 0x000003C0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000003F0, 0x000003FC)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000500, 0x0000051C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000530, 0x00000530)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000600, 0x00000604)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000610, 0x00000654)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000670, 0x000006B4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002600, 0x00002684)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000026C0, 0x000026DC)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002700, 0x00002708)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002710, 0x00002718)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002720, 0x00002728)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002730, 0x00002738)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002740, 0x00002748)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002750, 0x00002758)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002760, 0x00002768)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002770, 0x00002778)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002780, 0x00002780)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002800, 0x00002800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000280C, 0x00002868)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002870, 0x000028F0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00020000, 32768)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00030000, 65536)},

            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00040000, 24)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00042000, 24)},

            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00080000, 0x0008000C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0008001C, 0x0008001C)},

            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00091000, 0x0009100C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00091010, 0x00091010)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0009101C, 0x0009101C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000910C0, 0x000910E0)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem, numOfChunks, unitChunkPtr);
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
*       numOfChunks - number of chunks.
*       unitPtr     - pointer to the unit chunks.
*       plrUnit     - PLR unit
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
static void policerTablesSupport
(
    IN GT_U32  numOfChunks,
    INOUT SMEM_CHUNK_BASIC_STC  chunksMem[],
    IN SMEM_XCAT3_UNIT_NAME_ENT   plrUnit
)
{
    GT_BIT  eplr  = (plrUnit == SMEM_XCAT3_UNIT_EPLR_E) ? 1 : 0;
    GT_BIT  iplr1 = (plrUnit == SMEM_XCAT3_UNIT_IPLR1_E) ? 1 : 0;
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
*   smemXCat3UnitPolicerUnify
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
static void smemXCat3UnitPolicerUnify
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SMEM_XCAT3_UNIT_NAME_ENT unitName,
    IN GT_U32   baseAddr
)
{
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = devObjPtr->deviceMemory;
    SMEM_UNIT_CHUNKS_STC * unitChunkPtr = UNIT_CHUNK_PTR_MAC(devMemInfoPtr, unitName);

    /* chunks with flat memory (no formulas) */
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000000, 0x00000054)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000070, 0x00000070)},

            /*registers -- not table/memory !! -- Policer Table Access Data<%n> */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00000074 ,8*4),
             SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32,4),SMEM_BIND_TABLE_MAC(policerTblAccessData)},

            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000000A0, 0x000000B8)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000000C0, 0x000000DC)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000100, 0x0000010C)},

            /*Policer Timer Memory */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000200, 36),
             SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32,4),SMEM_BIND_TABLE_MAC(policerTimer)},

            /*Policer Descriptor Sample Memory */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000400, 96)},

            /*Policer Management Counters Memory */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000500, 192),
             SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(74, 16),SMEM_BIND_TABLE_MAC(policerManagementCounters)},

            /*IPFIX wrap around alert Memory */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000800, 256),
             SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32,4),SMEM_BIND_TABLE_MAC(policerIpfixWaAlert)},

            /*IPFIX aging alert Memory */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000900, 256),
             SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32,4),SMEM_BIND_TABLE_MAC(policerIpfixAgingAlert)},

            /* DFX table */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00001000, 2048)},

            /*registers -- not table/memory !! -- Port%p and Packet Type Translation Table*/
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00001800 , 32*4),
             SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32,4),SMEM_BIND_TABLE_MAC(policerMeterPointer)},

            /*Policer Metering Memory */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00040000, 65536),
             SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(229, 32),SMEM_BIND_TABLE_MAC(policer)},

            /*Policer Counting Memory */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00060000, 65536),
             SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(229, 32),SMEM_BIND_TABLE_MAC(policerCounters)},

            /*Ingress Policer Re-Marking Memory */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00080000, 1024),
             SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(51, 8),SMEM_BIND_TABLE_MAC(policerQos)}
        };

        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);
        GT_U32  ii;

        for(ii = 0 ; ii < numOfChunks ; ii ++)
        {
            chunksMem[ii].memFirstAddr += baseAddr;
        }

        policerTablesSupport(numOfChunks,chunksMem,unitName);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitChunkPtr);
    }
}

/*******************************************************************************
*   smemXCat3UnitIplr1
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
static void smemXCat3UnitIplr1
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    smemXCat3UnitPolicerUnify(devObjPtr, SMEM_XCAT3_UNIT_IPLR1_E, policerBaseAddr[1]);
}

/*******************************************************************************
*   smemXCat3UnitTti
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
static void smemXCat3UnitTti
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = devObjPtr->deviceMemory;
    SMEM_UNIT_CHUNKS_STC * unitChunkPtr = UNIT_CHUNK_PTR_MAC(devMemInfoPtr, SMEM_XCAT3_UNIT_TTI_E);

    /* chunks with flat memory (no formulas) */
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x16000000, 0x16000068)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x16000070, 0x16000078)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x16000080, 0x160000E8)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x16000100, 0x16000120)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x16000140, 0x16000188)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x16000200, 0x16000264)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x16000298, 0x160002A4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x160002B0, 0x160002B4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x16000300, 0x1600038C)},

            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC  (0x16000400, 64),
             SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4),SMEM_BIND_TABLE_MAC(dscpToQoSProfile)},

            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x16000440, 0x16000444)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x16000450, 0x16000454)},

            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC  (0x16000460, 8),
             SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4),SMEM_BIND_TABLE_MAC(expToQoSProfile)},

            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x16000500, 0x160005A0)},

            /* DSA Tag QoS 2 QoS Parameters Map Memory */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x16000600, 512)},

            /* DSCP 2 QoS Parameters Map Memory */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x16000800, 256)},

            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x16000900, 64),
             SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4),SMEM_BIND_TABLE_MAC(dscpToDscpMap)},

            /* Port VLAN and QoS Configuration Table Table */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x16001000, 1024),
             SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(66, 16),SMEM_BIND_TABLE_MAC(portVlanQosConfig)},

            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x16001600, 0x1600167C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x16001700, 0x16001720)},

            /* MAC2ME TCAM Bist Registers Table */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x16001780, 92)},

            /* TTI DFX Area Table */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x16001800, 2048)},

            /* Port Protocol VID and QoS Configuration Table */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x16004000, 8192),
             SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 8),SMEM_BIND_TABLE_MAC(portProtocolVidQoSConf)},

            /* MAC2ME TCAM Table */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x16006000, 4096)},

            /* VLAN Translation Table */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x1600C000, 16384),
             SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(12, 4),SMEM_BIND_TABLE_MAC(ingressVlanTranslation)},

            /* User Defined Bytes Configuration Memory */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x16010000, 256),
             SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(255, 32),SMEM_BIND_TABLE_MAC(ipclUserDefinedBytesConf)},

            /* Virtual Port ID Assignment Memory */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x16020000, 4208)},

            /* User Defined Bytes ID Memory */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x16030000, 16640)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitChunkPtr);
    }
}

/*******************************************************************************
*   smemXCat3UnitTxqEgr
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
static void smemXCat3UnitTxqEgr
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = devObjPtr->deviceMemory;
    SMEM_UNIT_CHUNKS_STC * unitChunkPtr = UNIT_CHUNK_PTR_MAC(devMemInfoPtr, SMEM_XCAT3_UNIT_EGR_TXQ_E);

    /* chunks with flat memory (no formulas) */
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02800000, 0x02800048)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02800050, 0x02800058)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02800060, 0x02800068)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02800070, 0x02800078)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02800080, 0x02800098)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028000A0, 0x028000E4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02800100, 0x02800134)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02800140, 0x02800140)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02800160, 0x02800160)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02800180, 0x02800180)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02800188, 0x0280018C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02800194, 0x028001A0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028001A8, 0x028001B4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028001BC, 0x028001E0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028001E8, 0x028001F8)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02800240, 0x02800244)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02800280, 0x02800280)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02800480, 0x02800480)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02800680, 0x02800680)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02800880, 0x02800880)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02800A80, 0x02800A80)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02800C80, 0x02800C80)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02800E80, 0x02800E80)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02800F00, 0x02800F08)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02800F10, 0x02800F18)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02800F20, 0x02800F28)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02800F30, 0x02800F38)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02800F50, 0x02800FD4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02801080, 0x02801080)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02801280, 0x02801280)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02801480, 0x02801480)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02801680, 0x02801680)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02801880, 0x02801880)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02801A80, 0x02801A80)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02801C80, 0x02801C80)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02801E80, 0x02801E80)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02802080, 0x02802080)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02802280, 0x02802280)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02802480, 0x02802480)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02802680, 0x02802680)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02802880, 0x02802880)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02802A80, 0x02802A80)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02802C80, 0x02802C80)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02802E80, 0x02802E80)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02803080, 0x02803080)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02803280, 0x02803280)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02803480, 0x02803480)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02803680, 0x02803680)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02807E80, 0x02807E80)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02840000, 0x02840000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02840200, 0x02840200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02840400, 0x02840400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02840600, 0x02840600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02840800, 0x02840800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02840A00, 0x02840A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02840C00, 0x02840C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02840E00, 0x02840E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02841000, 0x02841000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02841200, 0x02841200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02841400, 0x02841400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02841600, 0x02841600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02841800, 0x02841800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02841A00, 0x02841A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02841C00, 0x02841C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02841E00, 0x02841E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02842000, 0x02842000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02842200, 0x02842200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02842400, 0x02842400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02842600, 0x02842600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02842800, 0x02842800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02842A00, 0x02842A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02842C00, 0x02842C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02842E00, 0x02842E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02843000, 0x02843000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02843200, 0x02843200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02843400, 0x02843400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02843600, 0x02843600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02847E00, 0x02847E00)},

            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02880000, 0x02880000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02880200, 0x02880200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02880400, 0x02880400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02880600, 0x02880600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02880800, 0x02880800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02880A00, 0x02880A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02880C00, 0x02880C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02880E00, 0x02880E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02881000, 0x02881000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02881200, 0x02881200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02881400, 0x02881400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02881600, 0x02881600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02881800, 0x02881800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02881A00, 0x02881A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02881C00, 0x02881C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02881E00, 0x02881E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02882000, 0x02882000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02882200, 0x02882200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02882400, 0x02882400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02882600, 0x02882600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02882800, 0x02882800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02882A00, 0x02882A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02882C00, 0x02882C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02882E00, 0x02882E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02883000, 0x02883000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02883200, 0x02883200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02883400, 0x02883400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02883600, 0x02883600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02887E00, 0x02887E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02888000, 0x02888000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02888200, 0x02888200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02888400, 0x02888400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02888600, 0x02888600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02888800, 0x02888800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02888A00, 0x02888A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02888C00, 0x02888C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02888E00, 0x02888E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02889000, 0x02889000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02889200, 0x02889200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02889400, 0x02889400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02889600, 0x02889600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02889800, 0x02889800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02889A00, 0x02889A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02889C00, 0x02889C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02889E00, 0x02889E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0288A000, 0x0288A000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0288A200, 0x0288A200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0288A400, 0x0288A400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0288A600, 0x0288A600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0288A800, 0x0288A800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0288AA00, 0x0288AA00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0288AC00, 0x0288AC00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0288AE00, 0x0288AE00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0288B000, 0x0288B000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0288B200, 0x0288B200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0288B400, 0x0288B400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0288B600, 0x0288B600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0288FE00, 0x0288FE00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02890000, 0x02890000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02890200, 0x02890200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02890400, 0x02890400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02890600, 0x02890600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02890800, 0x02890800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02890A00, 0x02890A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02890C00, 0x02890C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02890E00, 0x02890E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02891000, 0x02891000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02891200, 0x02891200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02891400, 0x02891400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02891600, 0x02891600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02891800, 0x02891800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02891A00, 0x02891A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02891C00, 0x02891C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02891E00, 0x02891E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02892000, 0x02892000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02892200, 0x02892200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02892400, 0x02892400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02892600, 0x02892600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02892800, 0x02892800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02892A00, 0x02892A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02892C00, 0x02892C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02892E00, 0x02892E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02893000, 0x02893000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02893200, 0x02893200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02893400, 0x02893400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02893600, 0x02893600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02897E00, 0x02897E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02898000, 0x02898000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02898200, 0x02898200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02898400, 0x02898400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02898600, 0x02898600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02898800, 0x02898800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02898A00, 0x02898A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02898C00, 0x02898C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02898E00, 0x02898E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02899000, 0x02899000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02899200, 0x02899200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02899400, 0x02899400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02899600, 0x02899600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02899800, 0x02899800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02899A00, 0x02899A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02899C00, 0x02899C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02899E00, 0x02899E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0289A000, 0x0289A000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0289A200, 0x0289A200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0289A400, 0x0289A400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0289A600, 0x0289A600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0289A800, 0x0289A800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0289AA00, 0x0289AA00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0289AC00, 0x0289AC00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0289AE00, 0x0289AE00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0289B000, 0x0289B000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0289B200, 0x0289B200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0289B400, 0x0289B400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0289B600, 0x0289B600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0289FE00, 0x0289FE00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028A0000, 0x028A0000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028A0200, 0x028A0200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028A0400, 0x028A0400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028A0600, 0x028A0600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028A0800, 0x028A0800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028A0A00, 0x028A0A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028A0C00, 0x028A0C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028A0E00, 0x028A0E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028A1000, 0x028A1000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028A1200, 0x028A1200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028A1400, 0x028A1400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028A1600, 0x028A1600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028A1800, 0x028A1800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028A1A00, 0x028A1A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028A1C00, 0x028A1C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028A1E00, 0x028A1E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028A2000, 0x028A2000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028A2200, 0x028A2200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028A2400, 0x028A2400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028A2600, 0x028A2600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028A2800, 0x028A2800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028A2A00, 0x028A2A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028A2C00, 0x028A2C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028A2E00, 0x028A2E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028A3000, 0x028A3000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028A3200, 0x028A3200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028A3400, 0x028A3400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028A3600, 0x028A3600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028A7E00, 0x028A7E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028A8000, 0x028A8000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028A8200, 0x028A8200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028A8400, 0x028A8400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028A8600, 0x028A8600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028A8800, 0x028A8800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028A8A00, 0x028A8A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028A8C00, 0x028A8C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028A8E00, 0x028A8E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028A9000, 0x028A9000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028A9200, 0x028A9200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028A9400, 0x028A9400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028A9600, 0x028A9600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028A9800, 0x028A9800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028A9A00, 0x028A9A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028A9C00, 0x028A9C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028A9E00, 0x028A9E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028AA000, 0x028AA000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028AA200, 0x028AA200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028AA400, 0x028AA400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028AA600, 0x028AA600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028AA800, 0x028AA800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028AAA00, 0x028AAA00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028AAC00, 0x028AAC00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028AAE00, 0x028AAE00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028AB000, 0x028AB000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028AB200, 0x028AB200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028AB400, 0x028AB400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028AB600, 0x028AB600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028AFE00, 0x028AFE00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028B0000, 0x028B0000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028B0200, 0x028B0200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028B0400, 0x028B0400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028B0600, 0x028B0600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028B0800, 0x028B0800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028B0A00, 0x028B0A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028B0C00, 0x028B0C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028B0E00, 0x028B0E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028B1000, 0x028B1000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028B1200, 0x028B1200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028B1400, 0x028B1400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028B1600, 0x028B1600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028B1800, 0x028B1800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028B1A00, 0x028B1A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028B1C00, 0x028B1C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028B1E00, 0x028B1E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028B2000, 0x028B2000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028B2200, 0x028B2200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028B2400, 0x028B2400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028B2600, 0x028B2600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028B2800, 0x028B2800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028B2A00, 0x028B2A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028B2C00, 0x028B2C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028B2E00, 0x028B2E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028B3000, 0x028B3000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028B3200, 0x028B3200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028B3400, 0x028B3400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028B3600, 0x028B3600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028B7E00, 0x028B7E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028B8000, 0x028B8000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028B8200, 0x028B8200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028B8400, 0x028B8400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028B8600, 0x028B8600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028B8800, 0x028B8800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028B8A00, 0x028B8A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028B8C00, 0x028B8C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028B8E00, 0x028B8E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028B9000, 0x028B9000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028B9200, 0x028B9200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028B9400, 0x028B9400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028B9600, 0x028B9600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028B9800, 0x028B9800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028B9A00, 0x028B9A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028B9C00, 0x028B9C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028B9E00, 0x028B9E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028BA000, 0x028BA000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028BA200, 0x028BA200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028BA400, 0x028BA400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028BA600, 0x028BA600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028BA800, 0x028BA800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028BAA00, 0x028BAA00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028BAC00, 0x028BAC00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028BAE00, 0x028BAE00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028BB000, 0x028BB000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028BB200, 0x028BB200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028BB400, 0x028BB400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028BB600, 0x028BB600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028BFE00, 0x028BFE00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028C0000, 0x028C0000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028C0200, 0x028C0200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028C0400, 0x028C0400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028C0600, 0x028C0600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028C0800, 0x028C0800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028C0A00, 0x028C0A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028C0C00, 0x028C0C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028C0E00, 0x028C0E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028C1000, 0x028C1000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028C1200, 0x028C1200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028C1400, 0x028C1400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028C1600, 0x028C1600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028C1800, 0x028C1800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028C1A00, 0x028C1A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028C1C00, 0x028C1C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028C1E00, 0x028C1E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028C2000, 0x028C2000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028C2200, 0x028C2200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028C2400, 0x028C2400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028C2600, 0x028C2600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028C2800, 0x028C2800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028C2A00, 0x028C2A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028C2C00, 0x028C2C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028C2E00, 0x028C2E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028C3000, 0x028C3000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028C3200, 0x028C3200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028C3400, 0x028C3400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028C3600, 0x028C3600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x028C7E00, 0x028C7E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02900000, 0x02900000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02900200, 0x02900200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02900400, 0x02900400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02900600, 0x02900600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02900800, 0x02900800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02900A00, 0x02900A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02900C00, 0x02900C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02900E00, 0x02900E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02901000, 0x02901000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02901200, 0x02901200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02901400, 0x02901400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02901600, 0x02901600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02901800, 0x02901800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02901A00, 0x02901A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02901C00, 0x02901C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02901E00, 0x02901E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02902000, 0x02902000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02902200, 0x02902200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02902400, 0x02902400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02902600, 0x02902600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02902800, 0x02902800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02902A00, 0x02902A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02902C00, 0x02902C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02902E00, 0x02902E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02903000, 0x02903000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02903200, 0x02903200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02903400, 0x02903400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02903600, 0x02903600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02907E00, 0x02907E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02908000, 0x02908000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02908200, 0x02908200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02908400, 0x02908400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02908600, 0x02908600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02908800, 0x02908800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02908A00, 0x02908A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02908C00, 0x02908C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02908E00, 0x02908E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02909000, 0x02909000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02909200, 0x02909200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02909400, 0x02909400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02909600, 0x02909600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02909800, 0x02909800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02909A00, 0x02909A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02909C00, 0x02909C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02909E00, 0x02909E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0290A000, 0x0290A000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0290A200, 0x0290A200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0290A400, 0x0290A400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0290A600, 0x0290A600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0290A800, 0x0290A800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0290AA00, 0x0290AA00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0290AC00, 0x0290AC00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0290AE00, 0x0290AE00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0290B000, 0x0290B000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0290B200, 0x0290B200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0290B400, 0x0290B400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0290B600, 0x0290B600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0290FE00, 0x0290FE00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02910000, 0x02910000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02910200, 0x02910200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02910400, 0x02910400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02910600, 0x02910600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02910800, 0x02910800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02910A00, 0x02910A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02910C00, 0x02910C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02910E00, 0x02910E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02911000, 0x02911000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02911200, 0x02911200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02911400, 0x02911400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02911600, 0x02911600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02911800, 0x02911800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02911A00, 0x02911A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02911C00, 0x02911C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02911E00, 0x02911E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02912000, 0x02912000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02912200, 0x02912200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02912400, 0x02912400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02912600, 0x02912600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02912800, 0x02912800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02912A00, 0x02912A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02912C00, 0x02912C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02912E00, 0x02912E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02913000, 0x02913000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02913200, 0x02913200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02913400, 0x02913400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02913600, 0x02913600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02917E00, 0x02917E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02918000, 0x02918000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02918200, 0x02918200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02918400, 0x02918400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02918600, 0x02918600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02918800, 0x02918800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02918A00, 0x02918A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02918C00, 0x02918C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02918E00, 0x02918E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02919000, 0x02919000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02919200, 0x02919200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02919400, 0x02919400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02919600, 0x02919600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02919800, 0x02919800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02919A00, 0x02919A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02919C00, 0x02919C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02919E00, 0x02919E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0291A000, 0x0291A000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0291A200, 0x0291A200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0291A400, 0x0291A400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0291A600, 0x0291A600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0291A800, 0x0291A800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0291AA00, 0x0291AA00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0291AC00, 0x0291AC00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0291AE00, 0x0291AE00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0291B000, 0x0291B000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0291B200, 0x0291B200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0291B400, 0x0291B400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0291B600, 0x0291B600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0291FE00, 0x0291FE00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02920000, 0x02920000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02920200, 0x02920200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02920400, 0x02920400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02920600, 0x02920600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02920800, 0x02920800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02920A00, 0x02920A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02920C00, 0x02920C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02920E00, 0x02920E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02921000, 0x02921000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02921200, 0x02921200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02921400, 0x02921400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02921600, 0x02921600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02921800, 0x02921800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02921A00, 0x02921A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02921C00, 0x02921C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02921E00, 0x02921E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02922000, 0x02922000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02922200, 0x02922200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02922400, 0x02922400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02922600, 0x02922600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02922800, 0x02922800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02922A00, 0x02922A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02922C00, 0x02922C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02922E00, 0x02922E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02923000, 0x02923000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02923200, 0x02923200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02923400, 0x02923400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02923600, 0x02923600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02927E00, 0x02927E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02928000, 0x02928000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02928200, 0x02928200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02928400, 0x02928400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02928600, 0x02928600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02928800, 0x02928800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02928A00, 0x02928A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02928C00, 0x02928C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02928E00, 0x02928E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02929000, 0x02929000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02929200, 0x02929200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02929400, 0x02929400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02929600, 0x02929600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02929800, 0x02929800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02929A00, 0x02929A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02929C00, 0x02929C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02929E00, 0x02929E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0292A000, 0x0292A000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0292A200, 0x0292A200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0292A400, 0x0292A400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0292A600, 0x0292A600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0292A800, 0x0292A800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0292AA00, 0x0292AA00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0292AC00, 0x0292AC00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0292AE00, 0x0292AE00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0292B000, 0x0292B000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0292B200, 0x0292B200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0292B400, 0x0292B400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0292B600, 0x0292B600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0292FE00, 0x0292FE00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02930000, 0x02930000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02930200, 0x02930200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02930400, 0x02930400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02930600, 0x02930600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02930800, 0x02930800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02930A00, 0x02930A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02930C00, 0x02930C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02930E00, 0x02930E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02931000, 0x02931000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02931200, 0x02931200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02931400, 0x02931400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02931600, 0x02931600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02931800, 0x02931800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02931A00, 0x02931A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02931C00, 0x02931C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02931E00, 0x02931E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02932000, 0x02932000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02932200, 0x02932200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02932400, 0x02932400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02932600, 0x02932600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02932800, 0x02932800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02932A00, 0x02932A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02932C00, 0x02932C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02932E00, 0x02932E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02933000, 0x02933000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02933200, 0x02933200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02933400, 0x02933400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02933600, 0x02933600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02937E00, 0x02937E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02938000, 0x02938000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02938200, 0x02938200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02938400, 0x02938400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02938600, 0x02938600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02938800, 0x02938800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02938A00, 0x02938A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02938C00, 0x02938C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02938E00, 0x02938E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02939000, 0x02939000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02939200, 0x02939200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02939400, 0x02939400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02939600, 0x02939600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02939800, 0x02939800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02939A00, 0x02939A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02939C00, 0x02939C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02939E00, 0x02939E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0293A000, 0x0293A000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0293A200, 0x0293A200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0293A400, 0x0293A400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0293A600, 0x0293A600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0293A800, 0x0293A800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0293AA00, 0x0293AA00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0293AC00, 0x0293AC00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0293AE00, 0x0293AE00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0293B000, 0x0293B000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0293B200, 0x0293B200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0293B400, 0x0293B400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0293B600, 0x0293B600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0293FE00, 0x0293FE00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02940000, 0x0294003C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02940400, 0x0294043C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02940800, 0x0294083C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02940C00, 0x02940C3C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02941000, 0x0294103C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02941400, 0x0294143C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02941800, 0x0294183C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02941C00, 0x02941C3C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029C0000, 0x029C0000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029C0200, 0x029C0200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029C0400, 0x029C0400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029C0600, 0x029C0600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029C0800, 0x029C0800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029C0A00, 0x029C0A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029C0C00, 0x029C0C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029C0E00, 0x029C0E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029C1000, 0x029C1000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029C1200, 0x029C1200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029C1400, 0x029C1400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029C1600, 0x029C1600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029C1800, 0x029C1800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029C1A00, 0x029C1A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029C1C00, 0x029C1C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029C1E00, 0x029C1E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029C2000, 0x029C2000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029C2200, 0x029C2200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029C2400, 0x029C2400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029C2600, 0x029C2600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029C2800, 0x029C2800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029C2A00, 0x029C2A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029C2C00, 0x029C2C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029C2E00, 0x029C2E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029C3000, 0x029C3000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029C3200, 0x029C3200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029C3400, 0x029C3400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029C3600, 0x029C3600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029C7E00, 0x029C7E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029C8000, 0x029C8000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029C8200, 0x029C8200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029C8400, 0x029C8400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029C8600, 0x029C8600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029C8800, 0x029C8800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029C8A00, 0x029C8A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029C8C00, 0x029C8C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029C8E00, 0x029C8E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029C9000, 0x029C9000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029C9200, 0x029C9200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029C9400, 0x029C9400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029C9600, 0x029C9600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029C9800, 0x029C9800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029C9A00, 0x029C9A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029C9C00, 0x029C9C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029C9E00, 0x029C9E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029CA000, 0x029CA000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029CA200, 0x029CA200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029CA400, 0x029CA400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029CA600, 0x029CA600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029CA800, 0x029CA800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029CAA00, 0x029CAA00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029CAC00, 0x029CAC00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029CAE00, 0x029CAE00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029CB000, 0x029CB000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029CB200, 0x029CB200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029CB400, 0x029CB400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029CB600, 0x029CB600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029CFE00, 0x029CFE00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029D0000, 0x029D0000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029D0200, 0x029D0200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029D0400, 0x029D0400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029D0600, 0x029D0600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029D0800, 0x029D0800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029D0A00, 0x029D0A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029D0C00, 0x029D0C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029D0E00, 0x029D0E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029D1000, 0x029D1000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029D1200, 0x029D1200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029D1400, 0x029D1400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029D1600, 0x029D1600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029D1800, 0x029D1800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029D1A00, 0x029D1A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029D1C00, 0x029D1C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029D1E00, 0x029D1E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029D2000, 0x029D2000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029D2200, 0x029D2200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029D2400, 0x029D2400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029D2600, 0x029D2600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029D2800, 0x029D2800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029D2A00, 0x029D2A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029D2C00, 0x029D2C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029D2E00, 0x029D2E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029D3000, 0x029D3000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029D3200, 0x029D3200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029D3400, 0x029D3400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029D3600, 0x029D3600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029D7E00, 0x029D7E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029D8000, 0x029D8000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029D8200, 0x029D8200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029D8400, 0x029D8400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029D8600, 0x029D8600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029D8800, 0x029D8800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029D8A00, 0x029D8A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029D8C00, 0x029D8C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029D8E00, 0x029D8E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029D9000, 0x029D9000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029D9200, 0x029D9200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029D9400, 0x029D9400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029D9600, 0x029D9600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029D9800, 0x029D9800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029D9A00, 0x029D9A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029D9C00, 0x029D9C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029D9E00, 0x029D9E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029DA000, 0x029DA000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029DA200, 0x029DA200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029DA400, 0x029DA400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029DA600, 0x029DA600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029DA800, 0x029DA800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029DAA00, 0x029DAA00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029DAC00, 0x029DAC00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029DAE00, 0x029DAE00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029DB000, 0x029DB000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029DB200, 0x029DB200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029DB400, 0x029DB400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029DB600, 0x029DB600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029DFE00, 0x029DFE00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029E0000, 0x029E0000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029E0200, 0x029E0200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029E0400, 0x029E0400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029E0600, 0x029E0600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029E0800, 0x029E0800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029E0A00, 0x029E0A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029E0C00, 0x029E0C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029E0E00, 0x029E0E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029E1000, 0x029E1000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029E1200, 0x029E1200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029E1400, 0x029E1400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029E1600, 0x029E1600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029E1800, 0x029E1800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029E1A00, 0x029E1A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029E1C00, 0x029E1C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029E1E00, 0x029E1E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029E2000, 0x029E2000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029E2200, 0x029E2200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029E2400, 0x029E2400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029E2600, 0x029E2600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029E2800, 0x029E2800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029E2A00, 0x029E2A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029E2C00, 0x029E2C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029E2E00, 0x029E2E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029E3000, 0x029E3000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029E3200, 0x029E3200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029E3400, 0x029E3400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029E3600, 0x029E3600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029E7E00, 0x029E7E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029E8000, 0x029E8000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029E8200, 0x029E8200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029E8400, 0x029E8400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029E8600, 0x029E8600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029E8800, 0x029E8800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029E8A00, 0x029E8A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029E8C00, 0x029E8C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029E8E00, 0x029E8E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029E9000, 0x029E9000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029E9200, 0x029E9200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029E9400, 0x029E9400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029E9600, 0x029E9600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029E9800, 0x029E9800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029E9A00, 0x029E9A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029E9C00, 0x029E9C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029E9E00, 0x029E9E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029EA000, 0x029EA000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029EA200, 0x029EA200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029EA400, 0x029EA400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029EA600, 0x029EA600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029EA800, 0x029EA800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029EAA00, 0x029EAA00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029EAC00, 0x029EAC00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029EAE00, 0x029EAE00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029EB000, 0x029EB000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029EB200, 0x029EB200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029EB400, 0x029EB400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029EB600, 0x029EB600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029EFE00, 0x029EFE00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029F0000, 0x029F0000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029F0200, 0x029F0200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029F0400, 0x029F0400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029F0600, 0x029F0600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029F0800, 0x029F0800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029F0A00, 0x029F0A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029F0C00, 0x029F0C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029F0E00, 0x029F0E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029F1000, 0x029F1000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029F1200, 0x029F1200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029F1400, 0x029F1400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029F1600, 0x029F1600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029F1800, 0x029F1800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029F1A00, 0x029F1A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029F1C00, 0x029F1C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029F1E00, 0x029F1E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029F2000, 0x029F2000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029F2200, 0x029F2200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029F2400, 0x029F2400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029F2600, 0x029F2600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029F2800, 0x029F2800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029F2A00, 0x029F2A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029F2C00, 0x029F2C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029F2E00, 0x029F2E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029F3000, 0x029F3000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029F3200, 0x029F3200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029F3400, 0x029F3400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029F3600, 0x029F3600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029F7E00, 0x029F7E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029F8000, 0x029F8000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029F8200, 0x029F8200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029F8400, 0x029F8400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029F8600, 0x029F8600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029F8800, 0x029F8800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029F8A00, 0x029F8A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029F8C00, 0x029F8C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029F8E00, 0x029F8E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029F9000, 0x029F9000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029F9200, 0x029F9200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029F9400, 0x029F9400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029F9600, 0x029F9600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029F9800, 0x029F9800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029F9A00, 0x029F9A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029F9C00, 0x029F9C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029F9E00, 0x029F9E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029FA000, 0x029FA000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029FA200, 0x029FA200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029FA400, 0x029FA400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029FA600, 0x029FA600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029FA800, 0x029FA800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029FAA00, 0x029FAA00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029FAC00, 0x029FAC00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029FAE00, 0x029FAE00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029FB000, 0x029FB000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029FB200, 0x029FB200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029FB400, 0x029FB400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029FB600, 0x029FB600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x029FFE00, 0x029FFE00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A00000, 0x02A00000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A00200, 0x02A00200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A00400, 0x02A00400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A00600, 0x02A00600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A00800, 0x02A00800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A00A00, 0x02A00A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A00C00, 0x02A00C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A00E00, 0x02A00E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A01000, 0x02A01000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A01200, 0x02A01200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A01400, 0x02A01400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A01600, 0x02A01600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A01800, 0x02A01800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A01A00, 0x02A01A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A01C00, 0x02A01C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A01E00, 0x02A01E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A02000, 0x02A02000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A02200, 0x02A02200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A02400, 0x02A02400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A02600, 0x02A02600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A02800, 0x02A02800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A02A00, 0x02A02A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A02C00, 0x02A02C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A02E00, 0x02A02E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A03000, 0x02A03000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A03200, 0x02A03200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A03400, 0x02A03400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A03600, 0x02A03600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A07E00, 0x02A07E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A08000, 0x02A08000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A08200, 0x02A08200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A08400, 0x02A08400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A08600, 0x02A08600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A08800, 0x02A08800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A08A00, 0x02A08A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A08C00, 0x02A08C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A08E00, 0x02A08E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A09000, 0x02A09000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A09200, 0x02A09200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A09400, 0x02A09400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A09600, 0x02A09600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A09800, 0x02A09800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A09A00, 0x02A09A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A09C00, 0x02A09C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A09E00, 0x02A09E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A0A000, 0x02A0A000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A0A200, 0x02A0A200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A0A400, 0x02A0A400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A0A600, 0x02A0A600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A0A800, 0x02A0A800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A0AA00, 0x02A0AA00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A0AC00, 0x02A0AC00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A0AE00, 0x02A0AE00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A0B000, 0x02A0B000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A0B200, 0x02A0B200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A0B400, 0x02A0B400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A0B600, 0x02A0B600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A0FE00, 0x02A0FE00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A10000, 0x02A10000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A10200, 0x02A10200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A10400, 0x02A10400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A10600, 0x02A10600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A10800, 0x02A10800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A10A00, 0x02A10A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A10C00, 0x02A10C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A10E00, 0x02A10E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A11000, 0x02A11000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A11200, 0x02A11200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A11400, 0x02A11400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A11600, 0x02A11600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A11800, 0x02A11800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A11A00, 0x02A11A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A11C00, 0x02A11C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A11E00, 0x02A11E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A12000, 0x02A12000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A12200, 0x02A12200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A12400, 0x02A12400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A12600, 0x02A12600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A12800, 0x02A12800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A12A00, 0x02A12A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A12C00, 0x02A12C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A12E00, 0x02A12E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A13000, 0x02A13000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A13200, 0x02A13200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A13400, 0x02A13400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A13600, 0x02A13600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A17E00, 0x02A17E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A18000, 0x02A18000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A18200, 0x02A18200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A18400, 0x02A18400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A18600, 0x02A18600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A18800, 0x02A18800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A18A00, 0x02A18A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A18C00, 0x02A18C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A18E00, 0x02A18E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A19000, 0x02A19000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A19200, 0x02A19200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A19400, 0x02A19400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A19600, 0x02A19600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A19800, 0x02A19800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A19A00, 0x02A19A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A19C00, 0x02A19C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A19E00, 0x02A19E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A1A000, 0x02A1A000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A1A200, 0x02A1A200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A1A400, 0x02A1A400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A1A600, 0x02A1A600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A1A800, 0x02A1A800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A1AA00, 0x02A1AA00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A1AC00, 0x02A1AC00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A1AE00, 0x02A1AE00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A1B000, 0x02A1B000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A1B200, 0x02A1B200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A1B400, 0x02A1B400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A1B600, 0x02A1B600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A1FE00, 0x02A1FE00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A20000, 0x02A20000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A20200, 0x02A20200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A20400, 0x02A20400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A20600, 0x02A20600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A20800, 0x02A20800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A20A00, 0x02A20A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A20C00, 0x02A20C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A20E00, 0x02A20E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A21000, 0x02A21000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A21200, 0x02A21200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A21400, 0x02A21400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A21600, 0x02A21600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A21800, 0x02A21800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A21A00, 0x02A21A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A21C00, 0x02A21C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A21E00, 0x02A21E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A22000, 0x02A22000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A22200, 0x02A22200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A22400, 0x02A22400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A22600, 0x02A22600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A22800, 0x02A22800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A22A00, 0x02A22A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A22C00, 0x02A22C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A22E00, 0x02A22E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A23000, 0x02A23000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A23200, 0x02A23200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A23400, 0x02A23400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A23600, 0x02A23600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A27E00, 0x02A27E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A28000, 0x02A28000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A28200, 0x02A28200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A28400, 0x02A28400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A28600, 0x02A28600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A28800, 0x02A28800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A28A00, 0x02A28A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A28C00, 0x02A28C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A28E00, 0x02A28E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A29000, 0x02A29000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A29200, 0x02A29200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A29400, 0x02A29400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A29600, 0x02A29600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A29800, 0x02A29800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A29A00, 0x02A29A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A29C00, 0x02A29C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A29E00, 0x02A29E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A2A000, 0x02A2A000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A2A200, 0x02A2A200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A2A400, 0x02A2A400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A2A600, 0x02A2A600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A2A800, 0x02A2A800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A2AA00, 0x02A2AA00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A2AC00, 0x02A2AC00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A2AE00, 0x02A2AE00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A2B000, 0x02A2B000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A2B200, 0x02A2B200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A2B400, 0x02A2B400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A2B600, 0x02A2B600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A2FE00, 0x02A2FE00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A30000, 0x02A30000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A30200, 0x02A30200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A30400, 0x02A30400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A30600, 0x02A30600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A30800, 0x02A30800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A30A00, 0x02A30A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A30C00, 0x02A30C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A30E00, 0x02A30E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A31000, 0x02A31000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A31200, 0x02A31200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A31400, 0x02A31400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A31600, 0x02A31600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A31800, 0x02A31800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A31A00, 0x02A31A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A31C00, 0x02A31C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A31E00, 0x02A31E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A32000, 0x02A32000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A32200, 0x02A32200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A32400, 0x02A32400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A32600, 0x02A32600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A32800, 0x02A32800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A32A00, 0x02A32A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A32C00, 0x02A32C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A32E00, 0x02A32E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A33000, 0x02A33000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A33200, 0x02A33200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A33400, 0x02A33400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A33600, 0x02A33600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A37E00, 0x02A37E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A38000, 0x02A38000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A38200, 0x02A38200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A38400, 0x02A38400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A38600, 0x02A38600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A38800, 0x02A38800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A38A00, 0x02A38A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A38C00, 0x02A38C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A38E00, 0x02A38E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A39000, 0x02A39000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A39200, 0x02A39200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A39400, 0x02A39400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A39600, 0x02A39600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A39800, 0x02A39800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A39A00, 0x02A39A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A39C00, 0x02A39C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A39E00, 0x02A39E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A3A000, 0x02A3A000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A3A200, 0x02A3A200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A3A400, 0x02A3A400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A3A600, 0x02A3A600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A3A800, 0x02A3A800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A3AA00, 0x02A3AA00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A3AC00, 0x02A3AC00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A3AE00, 0x02A3AE00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A3B000, 0x02A3B000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A3B200, 0x02A3B200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A3B400, 0x02A3B400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A3B600, 0x02A3B600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A3FE00, 0x02A3FE00)},

            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A40000, 0x02A40084)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A4008C, 0x02A40094)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A4009C, 0x02A400A4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A400AC, 0x02A400B4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A400BC, 0x02A400C4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A400CC, 0x02A400D4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A400DC, 0x02A400E4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A400EC, 0x02A400F4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A400FC, 0x02A40104)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A4010C, 0x02A40114)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A4011C, 0x02A40124)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A4012C, 0x02A40134)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A4013C, 0x02A40144)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A4014C, 0x02A40154)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A4015C, 0x02A40164)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A4016C, 0x02A40174)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A4017C, 0x02A40184)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A4018C, 0x02A40194)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A4019C, 0x02A401A4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A401AC, 0x02A401B4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A401BC, 0x02A401C4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A401CC, 0x02A401D4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A401DC, 0x02A401E4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A401EC, 0x02A401F4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A401FC, 0x02A40200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A40210, 0x02A40210)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A40220, 0x02A40220)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A40230, 0x02A40230)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A40240, 0x02A40240)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A40250, 0x02A40250)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A40260, 0x02A40260)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A40270, 0x02A40270)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A40280, 0x02A40280)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A40290, 0x02A40290)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A402A0, 0x02A402A0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A402B0, 0x02A402B0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A402C0, 0x02A402C0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A402D0, 0x02A402D0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A402E0, 0x02A402E0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A402F0, 0x02A402F0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A40300, 0x02A40300)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A40310, 0x02A40310)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A40320, 0x02A40320)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A40330, 0x02A40330)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A40340, 0x02A40340)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A40350, 0x02A40350)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A40360, 0x02A40360)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A40370, 0x02A40370)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A40380, 0x02A40380)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A40390, 0x02A40390)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A403A0, 0x02A403A0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A403B0, 0x02A403B0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A403C0, 0x02A403C0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A403D0, 0x02A403D0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A403E0, 0x02A403E0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A403F0, 0x02A403F0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A40400, 0x02A40400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A40410, 0x02A40410)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A40420, 0x02A40420)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A40430, 0x02A40430)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A40440, 0x02A40440)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A40450, 0x02A40450)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A40460, 0x02A40460)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A40470, 0x02A40470)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A40480, 0x02A40480)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A40490, 0x02A40490)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A404A0, 0x02A404A0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A404B0, 0x02A404B0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A404C0, 0x02A404C0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A404D0, 0x02A404D0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A404E0, 0x02A404E0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A404F0, 0x02A404F0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A40500, 0x02A40500)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A40510, 0x02A40510)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A40520, 0x02A40520)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A40530, 0x02A40530)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A40540, 0x02A40540)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A40550, 0x02A40550)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A40560, 0x02A40560)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A40570, 0x02A40570)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A40580, 0x02A40580)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A40590, 0x02A40590)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A405A0, 0x02A405A0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A405B0, 0x02A405B0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A405C0, 0x02A405C0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A405D0, 0x02A405D0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A405E0, 0x02A405E0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A405F0, 0x02A405F0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A40600, 0x02A40600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A40610, 0x02A40610)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A40620, 0x02A40620)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A40630, 0x02A40630)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A40640, 0x02A40640)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A40650, 0x02A40650)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A40660, 0x02A40660)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A40670, 0x02A40670)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A40680, 0x02A40680)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A40690, 0x02A40690)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A406A0, 0x02A406A0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A406B0, 0x02A406B0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A406C0, 0x02A406C0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A406D0, 0x02A406D0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A406E0, 0x02A406E0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A406F0, 0x02A406F0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A40700, 0x02A40700)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A40710, 0x02A40710)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A40720, 0x02A40720)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A40730, 0x02A40730)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A40740, 0x02A40740)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A40750, 0x02A40750)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A40760, 0x02A40760)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A40770, 0x02A40770)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A40780, 0x02A40780)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A40790, 0x02A40790)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A407A0, 0x02A407A0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A407B0, 0x02A407B0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A407C0, 0x02A407C0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A407D0, 0x02A407D0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A407E0, 0x02A407E0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A407F0, 0x02A407F0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A80000, 0x02A80000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A80200, 0x02A80200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A80400, 0x02A80400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A80600, 0x02A80600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A80800, 0x02A80800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A80A00, 0x02A80A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A80C00, 0x02A80C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A80E00, 0x02A80E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A81000, 0x02A81000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A81200, 0x02A81200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A81400, 0x02A81400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A81600, 0x02A81600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A81800, 0x02A81800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A81A00, 0x02A81A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A81C00, 0x02A81C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A81E00, 0x02A81E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A82000, 0x02A82000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A82200, 0x02A82200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A82400, 0x02A82400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A82600, 0x02A82600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A82800, 0x02A82800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A82A00, 0x02A82A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A82C00, 0x02A82C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A82E00, 0x02A82E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A83000, 0x02A83000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A83200, 0x02A83200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A83400, 0x02A83400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A83600, 0x02A83600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02A87E00, 0x02A87E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AC0000, 0x02AC000C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AC0200, 0x02AC020C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AC0400, 0x02AC040C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AC0600, 0x02AC060C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AC0800, 0x02AC080C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AC0A00, 0x02AC0A0C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AC0C00, 0x02AC0C0C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AC0E00, 0x02AC0E0C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AC1000, 0x02AC100C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AC1200, 0x02AC120C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AC1400, 0x02AC140C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AC1600, 0x02AC160C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AC1800, 0x02AC180C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AC1A00, 0x02AC1A0C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AC1C00, 0x02AC1C0C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AC1E00, 0x02AC1E0C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AC2000, 0x02AC200C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AC2200, 0x02AC220C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AC2400, 0x02AC240C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AC2600, 0x02AC260C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AC2800, 0x02AC280C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AC2A00, 0x02AC2A0C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AC2C00, 0x02AC2C0C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AC2E00, 0x02AC2E0C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AC3000, 0x02AC300C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AC3200, 0x02AC320C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AC3400, 0x02AC340C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AC3600, 0x02AC360C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AC7E00, 0x02AC7E0C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AC8000, 0x02AC8004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AC8200, 0x02AC8204)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AC8400, 0x02AC8404)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AC8600, 0x02AC8604)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AC8800, 0x02AC8804)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AC8A00, 0x02AC8A04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AC8C00, 0x02AC8C04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AC8E00, 0x02AC8E04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AC9000, 0x02AC9004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AC9200, 0x02AC9204)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AC9400, 0x02AC9404)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AC9600, 0x02AC9604)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AC9800, 0x02AC9804)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AC9A00, 0x02AC9A04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AC9C00, 0x02AC9C04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AC9E00, 0x02AC9E04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02ACA000, 0x02ACA004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02ACA200, 0x02ACA204)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02ACA400, 0x02ACA404)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02ACA600, 0x02ACA604)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02ACA800, 0x02ACA804)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02ACAA00, 0x02ACAA04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02ACAC00, 0x02ACAC04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02ACAE00, 0x02ACAE04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02ACB000, 0x02ACB004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02ACB200, 0x02ACB204)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02ACB400, 0x02ACB404)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02ACB600, 0x02ACB604)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02ACFE00, 0x02ACFE04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AD0000, 0x02AD0004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AD0200, 0x02AD0204)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AD0400, 0x02AD0404)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AD0600, 0x02AD0604)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AD0800, 0x02AD0804)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AD0A00, 0x02AD0A04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AD0C00, 0x02AD0C04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AD0E00, 0x02AD0E04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AD1000, 0x02AD1004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AD1200, 0x02AD1204)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AD1400, 0x02AD1404)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AD1600, 0x02AD1604)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AD1800, 0x02AD1804)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AD1A00, 0x02AD1A04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AD1C00, 0x02AD1C04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AD1E00, 0x02AD1E04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AD2000, 0x02AD2004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AD2200, 0x02AD2204)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AD2400, 0x02AD2404)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AD2600, 0x02AD2604)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AD2800, 0x02AD2804)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AD2A00, 0x02AD2A04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AD2C00, 0x02AD2C04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AD2E00, 0x02AD2E04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AD3000, 0x02AD3004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AD3200, 0x02AD3204)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AD3400, 0x02AD3404)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AD3600, 0x02AD3604)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AD7E00, 0x02AD7E04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AD8000, 0x02AD8004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AD8200, 0x02AD8204)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AD8400, 0x02AD8404)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AD8600, 0x02AD8604)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AD8800, 0x02AD8804)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AD8A00, 0x02AD8A04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AD8C00, 0x02AD8C04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AD8E00, 0x02AD8E04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AD9000, 0x02AD9004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AD9200, 0x02AD9204)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AD9400, 0x02AD9404)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AD9600, 0x02AD9604)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AD9800, 0x02AD9804)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AD9A00, 0x02AD9A04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AD9C00, 0x02AD9C04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AD9E00, 0x02AD9E04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02ADA000, 0x02ADA004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02ADA200, 0x02ADA204)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02ADA400, 0x02ADA404)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02ADA600, 0x02ADA604)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02ADA800, 0x02ADA804)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02ADAA00, 0x02ADAA04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02ADAC00, 0x02ADAC04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02ADAE00, 0x02ADAE04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02ADB000, 0x02ADB004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02ADB200, 0x02ADB204)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02ADB400, 0x02ADB404)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02ADB600, 0x02ADB604)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02ADFE00, 0x02ADFE04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AE0000, 0x02AE0004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AE0200, 0x02AE0204)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AE0400, 0x02AE0404)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AE0600, 0x02AE0604)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AE0800, 0x02AE0804)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AE0A00, 0x02AE0A04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AE0C00, 0x02AE0C04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AE0E00, 0x02AE0E04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AE1000, 0x02AE1004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AE1200, 0x02AE1204)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AE1400, 0x02AE1404)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AE1600, 0x02AE1604)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AE1800, 0x02AE1804)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AE1A00, 0x02AE1A04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AE1C00, 0x02AE1C04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AE1E00, 0x02AE1E04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AE2000, 0x02AE2004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AE2200, 0x02AE2204)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AE2400, 0x02AE2404)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AE2600, 0x02AE2604)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AE2800, 0x02AE2804)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AE2A00, 0x02AE2A04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AE2C00, 0x02AE2C04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AE2E00, 0x02AE2E04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AE3000, 0x02AE3004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AE3200, 0x02AE3204)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AE3400, 0x02AE3404)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AE3600, 0x02AE3604)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AE7E00, 0x02AE7E04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AE8000, 0x02AE8004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AE8200, 0x02AE8204)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AE8400, 0x02AE8404)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AE8600, 0x02AE8604)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AE8800, 0x02AE8804)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AE8A00, 0x02AE8A04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AE8C00, 0x02AE8C04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AE8E00, 0x02AE8E04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AE9000, 0x02AE9004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AE9200, 0x02AE9204)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AE9400, 0x02AE9404)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AE9600, 0x02AE9604)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AE9800, 0x02AE9804)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AE9A00, 0x02AE9A04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AE9C00, 0x02AE9C04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AE9E00, 0x02AE9E04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AEA000, 0x02AEA004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AEA200, 0x02AEA204)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AEA400, 0x02AEA404)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AEA600, 0x02AEA604)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AEA800, 0x02AEA804)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AEAA00, 0x02AEAA04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AEAC00, 0x02AEAC04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AEAE00, 0x02AEAE04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AEB000, 0x02AEB004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AEB200, 0x02AEB204)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AEB400, 0x02AEB404)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AEB600, 0x02AEB604)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AEFE00, 0x02AEFE04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AF0000, 0x02AF0004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AF0200, 0x02AF0204)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AF0400, 0x02AF0404)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AF0600, 0x02AF0604)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AF0800, 0x02AF0804)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AF0A00, 0x02AF0A04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AF0C00, 0x02AF0C04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AF0E00, 0x02AF0E04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AF1000, 0x02AF1004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AF1200, 0x02AF1204)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AF1400, 0x02AF1404)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AF1600, 0x02AF1604)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AF1800, 0x02AF1804)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AF1A00, 0x02AF1A04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AF1C00, 0x02AF1C04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AF1E00, 0x02AF1E04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AF2000, 0x02AF2004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AF2200, 0x02AF2204)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AF2400, 0x02AF2404)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AF2600, 0x02AF2604)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AF2800, 0x02AF2804)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AF2A00, 0x02AF2A04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AF2C00, 0x02AF2C04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AF2E00, 0x02AF2E04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AF3000, 0x02AF3004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AF3200, 0x02AF3204)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AF3400, 0x02AF3404)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AF3600, 0x02AF3604)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AF7E00, 0x02AF7E04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AF8000, 0x02AF8004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AF8200, 0x02AF8204)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AF8400, 0x02AF8404)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AF8600, 0x02AF8604)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AF8800, 0x02AF8804)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AF8A00, 0x02AF8A04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AF8C00, 0x02AF8C04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AF8E00, 0x02AF8E04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AF9000, 0x02AF9004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AF9200, 0x02AF9204)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AF9400, 0x02AF9404)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AF9600, 0x02AF9604)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AF9800, 0x02AF9804)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AF9A00, 0x02AF9A04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AF9C00, 0x02AF9C04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AF9E00, 0x02AF9E04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AFA000, 0x02AFA004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AFA200, 0x02AFA204)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AFA400, 0x02AFA404)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AFA600, 0x02AFA604)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AFA800, 0x02AFA804)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AFAA00, 0x02AFAA04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AFAC00, 0x02AFAC04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AFAE00, 0x02AFAE04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AFB000, 0x02AFB004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AFB200, 0x02AFB204)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AFB400, 0x02AFB404)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AFB600, 0x02AFB604)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02AFFE00, 0x02AFFE04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B00000, 0x02B00004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B00200, 0x02B00204)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B00400, 0x02B00404)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B00600, 0x02B00604)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B00800, 0x02B00804)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B00A00, 0x02B00A04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B00C00, 0x02B00C04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B00E00, 0x02B00E04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B01000, 0x02B01004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B01200, 0x02B01204)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B01400, 0x02B01404)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B01600, 0x02B01604)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B01800, 0x02B01804)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B01A00, 0x02B01A04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B01C00, 0x02B01C04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B01E00, 0x02B01E04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B02000, 0x02B02004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B02200, 0x02B02204)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B02400, 0x02B02404)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B02600, 0x02B02604)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B02800, 0x02B02804)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B02A00, 0x02B02A04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B02C00, 0x02B02C04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B02E00, 0x02B02E04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B03000, 0x02B03004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B03200, 0x02B03204)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B03400, 0x02B03404)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B03600, 0x02B03604)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B07E00, 0x02B07E04)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B08000, 0x02B08000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B08200, 0x02B08200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B08400, 0x02B08400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B08600, 0x02B08600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B08800, 0x02B08800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B08A00, 0x02B08A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B08C00, 0x02B08C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B08E00, 0x02B08E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B09000, 0x02B09000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B09200, 0x02B09200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B09400, 0x02B09400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B09600, 0x02B09600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B09800, 0x02B09800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B09A00, 0x02B09A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B09C00, 0x02B09C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B09E00, 0x02B09E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B0A000, 0x02B0A000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B0A200, 0x02B0A200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B0A400, 0x02B0A400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B0A600, 0x02B0A600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B0A800, 0x02B0A800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B0AA00, 0x02B0AA00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B0AC00, 0x02B0AC00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B0AE00, 0x02B0AE00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B0B000, 0x02B0B000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B0B200, 0x02B0B200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B0B400, 0x02B0B400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B0B600, 0x02B0B600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B0FE00, 0x02B0FE00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B10000, 0x02B10000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B10200, 0x02B10200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B10400, 0x02B10400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B10600, 0x02B10600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B10800, 0x02B10800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B10A00, 0x02B10A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B10C00, 0x02B10C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B10E00, 0x02B10E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B11000, 0x02B11000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B11200, 0x02B11200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B11400, 0x02B11400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B11600, 0x02B11600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B11800, 0x02B11800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B11A00, 0x02B11A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B11C00, 0x02B11C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B11E00, 0x02B11E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B12000, 0x02B12000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B12200, 0x02B12200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B12400, 0x02B12400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B12600, 0x02B12600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B12800, 0x02B12800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B12A00, 0x02B12A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B12C00, 0x02B12C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B12E00, 0x02B12E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B13000, 0x02B13000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B13200, 0x02B13200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B13400, 0x02B13400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B13600, 0x02B13600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B17E00, 0x02B17E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B18000, 0x02B18000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B18200, 0x02B18200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B18400, 0x02B18400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B18600, 0x02B18600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B18800, 0x02B18800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B18A00, 0x02B18A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B18C00, 0x02B18C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B18E00, 0x02B18E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B19000, 0x02B19000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B19200, 0x02B19200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B19400, 0x02B19400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B19600, 0x02B19600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B19800, 0x02B19800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B19A00, 0x02B19A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B19C00, 0x02B19C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B19E00, 0x02B19E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B1A000, 0x02B1A000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B1A200, 0x02B1A200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B1A400, 0x02B1A400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B1A600, 0x02B1A600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B1A800, 0x02B1A800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B1AA00, 0x02B1AA00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B1AC00, 0x02B1AC00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B1AE00, 0x02B1AE00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B1B000, 0x02B1B000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B1B200, 0x02B1B200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B1B400, 0x02B1B400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B1B600, 0x02B1B600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B1FE00, 0x02B1FE00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B20000, 0x02B20000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B20200, 0x02B20200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B20400, 0x02B20400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B20600, 0x02B20600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B20800, 0x02B20800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B20A00, 0x02B20A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B20C00, 0x02B20C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B20E00, 0x02B20E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B21000, 0x02B21000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B21200, 0x02B21200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B21400, 0x02B21400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B21600, 0x02B21600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B21800, 0x02B21800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B21A00, 0x02B21A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B21C00, 0x02B21C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B21E00, 0x02B21E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B22000, 0x02B22000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B22200, 0x02B22200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B22400, 0x02B22400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B22600, 0x02B22600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B22800, 0x02B22800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B22A00, 0x02B22A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B22C00, 0x02B22C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B22E00, 0x02B22E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B23000, 0x02B23000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B23200, 0x02B23200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B23400, 0x02B23400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B23600, 0x02B23600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B27E00, 0x02B27E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B28000, 0x02B28000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B28200, 0x02B28200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B28400, 0x02B28400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B28600, 0x02B28600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B28800, 0x02B28800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B28A00, 0x02B28A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B28C00, 0x02B28C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B28E00, 0x02B28E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B29000, 0x02B29000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B29200, 0x02B29200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B29400, 0x02B29400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B29600, 0x02B29600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B29800, 0x02B29800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B29A00, 0x02B29A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B29C00, 0x02B29C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B29E00, 0x02B29E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B2A000, 0x02B2A000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B2A200, 0x02B2A200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B2A400, 0x02B2A400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B2A600, 0x02B2A600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B2A800, 0x02B2A800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B2AA00, 0x02B2AA00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B2AC00, 0x02B2AC00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B2AE00, 0x02B2AE00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B2B000, 0x02B2B000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B2B200, 0x02B2B200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B2B400, 0x02B2B400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B2B600, 0x02B2B600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B2FE00, 0x02B2FE00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B30000, 0x02B30000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B30200, 0x02B30200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B30400, 0x02B30400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B30600, 0x02B30600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B30800, 0x02B30800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B30A00, 0x02B30A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B30C00, 0x02B30C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B30E00, 0x02B30E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B31000, 0x02B31000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B31200, 0x02B31200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B31400, 0x02B31400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B31600, 0x02B31600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B31800, 0x02B31800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B31A00, 0x02B31A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B31C00, 0x02B31C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B31E00, 0x02B31E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B32000, 0x02B32000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B32200, 0x02B32200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B32400, 0x02B32400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B32600, 0x02B32600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B32800, 0x02B32800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B32A00, 0x02B32A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B32C00, 0x02B32C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B32E00, 0x02B32E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B33000, 0x02B33000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B33200, 0x02B33200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B33400, 0x02B33400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B33600, 0x02B33600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B37E00, 0x02B37E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B38000, 0x02B38000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B38200, 0x02B38200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B38400, 0x02B38400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B38600, 0x02B38600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B38800, 0x02B38800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B38A00, 0x02B38A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B38C00, 0x02B38C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B38E00, 0x02B38E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B39000, 0x02B39000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B39200, 0x02B39200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B39400, 0x02B39400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B39600, 0x02B39600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B39800, 0x02B39800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B39A00, 0x02B39A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B39C00, 0x02B39C00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B39E00, 0x02B39E00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B3A000, 0x02B3A000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B3A200, 0x02B3A200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B3A400, 0x02B3A400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B3A600, 0x02B3A600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B3A800, 0x02B3A800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B3AA00, 0x02B3AA00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B3AC00, 0x02B3AC00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B3AE00, 0x02B3AE00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B3B000, 0x02B3B000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B3B200, 0x02B3B200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B3B400, 0x02B3B400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B3B600, 0x02B3B600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B3FE00, 0x02B3FE00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B40144, 0x02B4015C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B40164, 0x02B40184)},

            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02BE0000, 0x02BF0004)},

            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02CC0000, 0x02CC0000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02CC0008, 0x02CC0008)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02D40000, 0x02D40008)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02D40200, 0x02D40208)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02D40400, 0x02D40408)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02D40600, 0x02D40608)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02D40800, 0x02D40808)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02D40A00, 0x02D40A08)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02D40C00, 0x02D40C08)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02D40E00, 0x02D40E08)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02D41000, 0x02D41008)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02D41200, 0x02D41208)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02D41400, 0x02D41408)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02D41600, 0x02D41608)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02D41800, 0x02D41808)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02D41A00, 0x02D41A08)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02D41C00, 0x02D41C08)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02D41E00, 0x02D41E08)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02D42000, 0x02D42008)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02D42200, 0x02D42208)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02D42400, 0x02D42408)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02D42600, 0x02D42608)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02D42800, 0x02D42808)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02D42A00, 0x02D42A08)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02D42C00, 0x02D42C08)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02D42E00, 0x02D42E08)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02D43000, 0x02D43008)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02D43200, 0x02D43208)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02D43400, 0x02D43408)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02D43600, 0x02D43608)},

            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02E40000, 0x02E40108)},


            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02E80000, 0x02E8001C)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitChunkPtr);
    }


    /* chunks with formulas */
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
            {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02B80000, 0x02B80000)}, FORMULA_SINGLE_PARAMETER(4096, 0x10)},
            {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02E40110, 0x02E40118)}, FORMULA_SINGLE_PARAMETER(11, 0x10)},
            {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02E401C4, 0x02E401C8)}, FORMULA_SINGLE_PARAMETER(34354, 0x10)}

            /*{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02880000, 0x02880000)}, FORMULA_SINGLE_PARAMETER(540, 0x200)}*/

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
*   smemXCat3UnitTxqEgrVlan
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
static void smemXCat3UnitTxqEgrVlan
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = devObjPtr->deviceMemory;
    SMEM_UNIT_CHUNKS_STC * unitChunkPtr = UNIT_CHUNK_PTR_MAC(devMemInfoPtr, SMEM_XCAT3_UNIT_VLAN_MC_E);

    /* chunks with flat memory (no formulas) */
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x03800000, 0x03800000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x03800100, 0x03800118)},

            /* Spanning Tree State table */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x03880000, 4096),
             SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(64, 16),SMEM_BIND_TABLE_MAC(stp)},


            /* Multicast Group Table */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x03900000, 196608),
             SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(29, 16),SMEM_BIND_TABLE_MAC(mcast)},

            /* VRF-ID Table*/
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x03980000, 65536),
             SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(12, 16),SMEM_BIND_TABLE_MAC(vrfId)},

            /* VLAN Table*/
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x03A00000, 131072),
             SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(179, 32),SMEM_BIND_TABLE_MAC(vlan)},

            /* Ingress ECID Table*/
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x03A80000, 65536),
             SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(29, 16),SMEM_BIND_TABLE_MAC(egressAndTxqIngressEcid)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitChunkPtr);
    }
}

/*******************************************************************************
*   smemXCat3UnitCcfc
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  CCFC unit
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
static void smemXCat3UnitCcfc
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = devObjPtr->deviceMemory;
    SMEM_UNIT_CHUNKS_STC * unitChunkPtr = UNIT_CHUNK_PTR_MAC(devMemInfoPtr, SMEM_XCAT3_UNIT_CCFC_E);

    /* chunks with flat memory (no formulas) */
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x03400000, 0x03400000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0340000C, 0x03400028)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x03400030, 4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x03400034, 0x0340003C)},

            /* BCN Profiles Table */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x03400100, 128)},

            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x034001FC, 0x034001FC)},

            /* Table Ports buffers counters*/
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x03400200, 112)},

            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x03401000, 0x03401020)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitChunkPtr);
    }
}

/*******************************************************************************
*   smemXCat3UnitL2i
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
static void smemXCat3UnitL2i
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = devObjPtr->deviceMemory;
    SMEM_UNIT_CHUNKS_STC * unitChunkPtr = UNIT_CHUNK_PTR_MAC(devMemInfoPtr, SMEM_XCAT3_UNIT_L2I_E);
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01000000, 0x01000000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01000010, 0x01000010)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01000200, 0x01000218)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01000300, 0x0100030C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01000400, 0x01000400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01000500, 0x0100052C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01000600, 0x0100061C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01000800, 0x01000810)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01000818, 0x0100081C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01001000, 0x01001000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01001010, 0x01001010)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01001400, 0x01001400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01001800, 0x01001808)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01001810, 0x01001810)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01001818, 0x01001818)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01002000, 0x01002000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01002010, 0x01002010)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01002400, 0x01002400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01002800, 0x01002808)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01002810, 0x01002810)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01002818, 0x01002818)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01003000, 0x01003000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01003010, 0x01003010)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01003400, 0x01003400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01003800, 0x01003808)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01003810, 0x01003810)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01003818, 0x01003818)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01004000, 0x01004000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01004010, 0x01004010)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01004400, 0x01004400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01004800, 0x01004808)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01004810, 0x01004810)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01004818, 0x01004818)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01005000, 0x01005000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01005010, 0x01005010)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01005400, 0x01005400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01005800, 0x01005808)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01005810, 0x01005810)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01005818, 0x01005818)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01006000, 0x01006000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01006010, 0x01006010)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01006400, 0x01006400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01006800, 0x01006808)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01006810, 0x01006810)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01006818, 0x01006818)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01007000, 0x01007000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01007010, 0x01007010)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01007400, 0x01007400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01007800, 0x01007808)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01007810, 0x01007810)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01007818, 0x01007818)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01008000, 0x01008000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01008010, 0x01008010)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01008400, 0x01008400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01008800, 0x01008808)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01008810, 0x01008810)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01008818, 0x01008818)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01009000, 0x01009000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01009010, 0x01009010)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01009400, 0x01009400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01009800, 0x01009808)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01009810, 0x01009810)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01009818, 0x01009818)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0100A000, 0x0100A000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0100A010, 0x0100A010)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0100A400, 0x0100A400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0100A800, 0x0100A808)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0100A810, 0x0100A810)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0100A818, 0x0100A818)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0100B000, 0x0100B000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0100B010, 0x0100B010)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0100B400, 0x0100B400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0100B800, 0x0100B808)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0100B810, 0x0100B810)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0100B818, 0x0100B818)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0100C000, 0x0100C000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0100C010, 0x0100C010)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0100C400, 0x0100C400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0100C800, 0x0100C808)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0100C810, 0x0100C810)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0100C818, 0x0100C818)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0100D000, 0x0100D000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0100D010, 0x0100D010)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0100D400, 0x0100D400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0100D800, 0x0100D808)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0100D810, 0x0100D810)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0100D818, 0x0100D818)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0100E000, 0x0100E000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0100E010, 0x0100E010)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0100E400, 0x0100E400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0100E800, 0x0100E808)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0100E810, 0x0100E810)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0100E818, 0x0100E818)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0100F000, 0x0100F000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0100F010, 0x0100F010)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0100F400, 0x0100F400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0100F800, 0x0100F808)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0100F810, 0x0100F810)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0100F818, 0x0100F818)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01010000, 0x01010000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01010010, 0x01010010)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01010400, 0x01010400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01010800, 0x01010800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01011000, 0x01011000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01011010, 0x01011010)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01011400, 0x01011400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01011800, 0x01011800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01012000, 0x01012000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01012010, 0x01012010)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01012400, 0x01012400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01012800, 0x01012800)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01013000, 0x01013000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01013010, 0x01013010)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01013400, 0x01013400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01014000, 0x01014000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01014010, 0x01014010)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01014400, 0x01014400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01015000, 0x01015000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01015010, 0x01015010)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01015400, 0x01015400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01016000, 0x01016000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01016010, 0x01016010)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01016400, 0x01016400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01017000, 0x01017000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01017010, 0x01017010)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01017400, 0x01017400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01018000, 0x01018000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01018010, 0x01018010)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01018400, 0x01018400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01019000, 0x01019000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01019010, 0x01019010)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01019400, 0x01019400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0101A000, 0x0101A000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0101A010, 0x0101A010)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0101A400, 0x0101A400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0101B000, 0x0101B000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0101B010, 0x0101B010)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0101B400, 0x0101B400)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0103F000, 0x0103F000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0103F010, 0x0103F010)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01040000, 0x01040014)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01040020, 0x01040070)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x010400A8, 0x010400A8)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x010400B0, 0x010400C0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x010400CC, 0x010400D4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x010400DC, 0x01040108)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01040130, 0x01040134)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01040140, 0x01040150)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x010401A0, 0x010401A8)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01040700, 0x01040730)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01040A00, 0x01040A4C)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitChunkPtr);
    }

    /* chunks with formulas */
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
            {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01020800, 0x01020800)}, FORMULA_SINGLE_PARAMETER(48, 0x1000)},
             /* IEEE Reserved Multicast Configuration register reading - memory space for 'read' Errata  */
            {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01000820 ,0)}, FORMULA_TWO_PARAMETERS(16 , 0x1000 , 2 , 0x8)}
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
static void smemXCat3UnitIpvx
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = devObjPtr->deviceMemory;
    SMEM_UNIT_CHUNKS_STC * unitChunkPtr = UNIT_CHUNK_PTR_MAC(devMemInfoPtr, SMEM_XCAT3_UNIT_IPVX_E);
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x04000000, 0x04000010)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x04000100, 0x0400010C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x04000200, 0x0400020C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x04000250, 0x0400026C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x04000278, 0x04000294)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x04000300, 0x0400033C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x04000440, 0x0400045C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x04000900, 0x04000924)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x04000940, 0x04000948)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x04000950, 0x04000958)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x04000964, 0x04000968)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x04000980, 0x04000980)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x04000A00, 0x04000A24)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x04000A80, 0x04000A80)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x04000B00, 0x04000B24)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x04000B80, 0x04000B80)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x04000C00, 0x04000C24)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x04000C80, 0x04000C80)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x04000D00, 0x04000D48)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x04000D50, 0x04000D64)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x04000D70, 0x04000D70)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x04000E3C, 0x04000E3C)},

            /* Router Next Hop Table Age Bits */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x04001000, 512),
             SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4),SMEM_BIND_TABLE_MAC(routeNextHopAgeBits)},

            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x04002000, 0x04002058)},

            /* Router Next Hop Table */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x04300000, 65536)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitChunkPtr);
    }
}

/*******************************************************************************
*   smemXCat3UnitBm
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
static void smemXCat3UnitBm
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = devObjPtr->deviceMemory;
    SMEM_UNIT_CHUNKS_STC * unitChunkPtr = UNIT_CHUNK_PTR_MAC(devMemInfoPtr, SMEM_XCAT3_UNIT_BM_E);
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x05000000, 0x05000014)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x05000020, 0x05000034)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x05000040, 0x0500004C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x05000054, 0x05000060)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x05000068, 0x05000068)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x05000080, 0x05000084)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x05000100, 0x0500016C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0500017C, 0x0500017C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x05000200, 0x05000210)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x05000300, 0x05000308)},

            /* LinkList Table */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x05010000, 24576)},

            /* Control Table */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x05020000, 24576)},

            /* McCnt Table */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x05040000, 24567)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitChunkPtr);
    }
}

/*******************************************************************************
*   smemXCat3UnitLms
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
static void smemXCat3UnitLms
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = devObjPtr->deviceMemory;
    SMEM_UNIT_CHUNKS_STC * unitChunkPtr = UNIT_CHUNK_PTR_MAC(devMemInfoPtr, SMEM_XCAT3_UNIT_LMS_E);

    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x07004000, 0x07004004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x07004010, 0x07004010)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x07004020, 0x07004028)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x07004030, 0x07004034)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x07004054, 0x07004054)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x07004100, 0x0700410C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x07004140, 0x07004144)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x07004200, 0x07004200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x07005100, 0x07005104)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x07005110, 0x07005114)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x07010000, 896)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x08004000, 0x08004004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x08004020, 0x08004024)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x08004030, 0x08004034)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x08004100, 0x0800410C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x08004140, 0x08004144)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x08005100, 0x08005104)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x08005110, 0x08005114)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x08010000, 896)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x09004000, 0x09004004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x09004010, 0x09004010)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x09004020, 0x09004020)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x09004030, 0x09004034)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x09004054, 0x09004054)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x09004100, 0x0900410C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x09004140, 0x09004144)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x09004200, 0x09004200)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x09005100, 0x09005104)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x09005110, 0x09005114)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x09010000, 896)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0A004000, 0x0A004004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0A004020, 0x0A004024)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0A004030, 0x0A004034)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0A004100, 0x0A00410C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0A004140, 0x0A004144)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0A005100, 0x0A005104)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0A010000, 896)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitChunkPtr);
    }
}

/*******************************************************************************
*   smemXCat3UnitFdb
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
static void smemXCat3UnitFdb
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = devObjPtr->deviceMemory;
    SMEM_UNIT_CHUNKS_STC * unitChunkPtr = UNIT_CHUNK_PTR_MAC(devMemInfoPtr, SMEM_XCAT3_UNIT_FDB_E);
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0B000000, 0x0B000028)},
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0B000030, 0x0B000084)},
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0B000090, 0x0B0000CC)},

             /* FDB Table */
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0B400000, 262144),
              SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(114, 16),SMEM_BIND_TABLE_MAC(fdb)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitChunkPtr);
    }
}


/*******************************************************************************
*   smemXCat3UnitMppm
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the xCat3 MPPM unit
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
static void smemXCat3UnitMppm
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SMEM_XCAT3_UNIT_NAME_ENT unitName,
    IN GT_U32   baseAddr
)
{
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = devObjPtr->deviceMemory;
    SMEM_UNIT_CHUNKS_STC * unitChunkPtr = UNIT_CHUNK_PTR_MAC(devMemInfoPtr, unitName);
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000000, 393216)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00100000, 0x00100000)}
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
*   smemXCat3UnitMppm0
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the xCat3 MPPM0 unit
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
static void smemXCat3UnitMppm0
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    smemXCat3UnitMppm(devObjPtr, SMEM_XCAT3_UNIT_MPPM_BANK0_E, mppmBankBaseAddr[0]);
}

/*******************************************************************************
*   smemXCat3UnitMppm0
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the xCat3 MPPM0 unit
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
static void smemXCat3UnitMppm1
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    smemXCat3UnitMppm(devObjPtr, SMEM_XCAT3_UNIT_MPPM_BANK1_E, mppmBankBaseAddr[1]);
}

/*******************************************************************************
*   smemXCat3UnitHa
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the xCat3 HA  unit
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
static void smemXCat3UnitHa
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = devObjPtr->deviceMemory;
    SMEM_UNIT_CHUNKS_STC * unitChunkPtr = UNIT_CHUNK_PTR_MAC(devMemInfoPtr, SMEM_XCAT3_UNIT_HA_E);
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1C000004, 0x1C000004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1C000014, 0x1C000014)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1C000020, 0x1C000020)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1C00002C, 0x1C000044)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1C000050, 0x1C000050)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1C000058, 0x1C000058)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1C000060, 0x1C000060)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1C000100, 0x1C000110)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1C000120, 0x1C000124)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1C000130, 0x1C000130)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1C000300, 0x1C00035C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1C000400, 0x1C000418)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1C000420, 0x1C00044C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1C000460, 0x1C00046C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1C000480, 0x1C000484)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1C000490, 0x1C00049C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1C0004B0, 0x1C0004BC)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1C0004D0, 0x1C00058C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1C000600, 0x1C000600)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1C000700, 0x1C00071C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1C000730, 0x1C000730)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1C000750, 0x1C00075C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1C000764, 0x1C000780)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1C000790, 0x1C000790)},

            /* Port\VLAM MAC SA Table */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x1C008000, 16640),
             SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(8, 4),SMEM_BIND_TABLE_MAC(vlanPortMacSa)},

            /* VLAN TranslationTable */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x1C010000, 16384),
             SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(12, 4),SMEM_BIND_TABLE_MAC(egressVlanTranslation)},

            /* DFX Memory */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x1C018000, 2048)},

            /* Router ARP DA and Tunnel Start Table */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x1C040000, 32768),
             SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(192, 32),SMEM_BIND_TABLE_MAC(arp)}

        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitChunkPtr);
    }
}

/*******************************************************************************
*   smemXCat3UnitMem
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the xCat3 MEM unit
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
static void smemXCat3UnitMem
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = devObjPtr->deviceMemory;
    SMEM_UNIT_CHUNKS_STC * unitChunkPtr = UNIT_CHUNK_PTR_MAC(devMemInfoPtr, SMEM_XCAT3_UNIT_MEM_E);
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0E000000, 0x0E000000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0E00000C, 0x0E00000C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0E000024, 0x0E000028)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0E00005C, 0x0E00005C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0E000064, 0x0E000064)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0E00007C, 0x0E00007C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0E000088, 0x0E0000A0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0E0000B0, 0x0E0000CC)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0E000428, 0x0E00043C)},

            /* TXDMA */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0E000008, 0x0E000008)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0E000010, 0x0E000010)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0E000048, 0x0E000048)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0E000060, 0x0E000060)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0E000068, 0x0E000070)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0E000080, 0x0E000084)},

            /* Prefetch Descriptors Table */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0E600000, 264)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitChunkPtr);
    }
}

/*******************************************************************************
*   smemXCat3UnitEplr
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the xCat3 EPLR unit
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
static void smemXCat3UnitEplr
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    smemXCat3UnitPolicerUnify(devObjPtr, SMEM_XCAT3_UNIT_EPLR_E, policerBaseAddr[2]);
}

/*******************************************************************************
*   smemXCat3UnitMll
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the xCat3 EPLR unit
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
static void smemXCat3UnitMll
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = devObjPtr->deviceMemory;
    SMEM_UNIT_CHUNKS_STC * unitChunkPtr = UNIT_CHUNK_PTR_MAC(devMemInfoPtr, SMEM_XCAT3_UNIT_MLL_E);
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x19000000, 0x19000018)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x19000100, 0x1900011C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x19000200, 0x19000204)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x19000210, 0x19000214)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x19000440, 0x19000450)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x19000460, 0x19000464)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x19000470, 0x19000474)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x19000900, 0x19000900)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x19000980, 0x19000984)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x19000A00, 0x19000A00)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x19000A80, 0x19000A80)},

            /* L2 MLL VIDX Enable Table */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x19008000, 512),
             SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4),SMEM_BIND_TABLE_MAC(l2MllVidxToMllMapping)},

            /* L2 MLL pointer map Table */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x19009000, 4096),
             SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(12, 4),SMEM_BIND_TABLE_MAC(l2MllVirtualPortToMllMapping)},

            /* MLL Table */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x19080000, 32768)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitChunkPtr);
    }
}


/*******************************************************************************
*   smemXCat3UnitCentralizedCounters
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
static void smemXCat3UnitCentralizedCounters
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = devObjPtr->deviceMemory;
    SMEM_UNIT_CHUNKS_STC * unitChunkPtr = UNIT_CHUNK_PTR_MAC(devMemInfoPtr, SMEM_XCAT3_UNIT_CENTRALIZED_COUNT_E);
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x10000000, 0x10000000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x10000024, 0x10000024)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x10000030, 0x10000030)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x10000040, 0x10000044)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x10000100, 0x10000104)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x10000180, 0x1000018C)},

            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x10080000, 32768),
             SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(64, 8),SMEM_BIND_TABLE_MAC(cncMemory)}

        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitChunkPtr);
    }

    /* chunks with formulas */
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
            {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x10001080 ,0)}, FORMULA_TWO_PARAMETERS(12, 0x100, 7, 0x4)},
            {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x1000109C ,0)}, FORMULA_TWO_PARAMETERS(2, 0x100, 1, 0x4)},
            {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x100010A0 ,0)}, FORMULA_TWO_PARAMETERS(12, 0x100, 4, 0x4)},
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
*   smemXCat3UnitMsm
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  Msm unit
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
static void smemXCat3UnitMsm
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = devObjPtr->deviceMemory;
    SMEM_UNIT_CHUNKS_STC * unitChunkPtr = UNIT_CHUNK_PTR_MAC(devMemInfoPtr, SMEM_XCAT3_UNIT_MSM_E);

    /* chunks with formulas */
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
            {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x11000000 ,0)},
              FORMULA_TWO_PARAMETERS(128/4 , 0x4 , 30 , MSM_MIB_COUNTER_STEP_PER_PORT_CNS)},
            {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x11000000+31*MSM_MIB_COUNTER_STEP_PER_PORT_CNS ,0)},
              FORMULA_SINGLE_PARAMETER(128/4 , 0x4)}
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
*   smemXCat3UnitSerdes
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Serdes (SD) unit
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
static void smemXCat3UnitSerdes
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = devObjPtr->deviceMemory;
    SMEM_UNIT_CHUNKS_STC * unitChunkPtr = UNIT_CHUNK_PTR_MAC(devMemInfoPtr, SMEM_XCAT3_UNIT_SERDES_E);

    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x13000000, 0x1300000C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x13000014, 0x13000038)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x13000800, 2048)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x13001000, 0x1300100C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x13001014, 0x13001038)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x13001800, 2048)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x13002000, 0x1300200C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x13002014, 0x13002038)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x13002800, 2048)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x13003000, 0x1300300C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x13003014, 0x13003038)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x13003800, 2048)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x13004000, 0x1300400C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x13004014, 0x13004038)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x13004800, 2048)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x13005000, 0x1300500C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x13005014, 0x13005038)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x13005800, 2048)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x13006000, 0x1300600C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x13006014, 0x13006038)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x13006800, 2048)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x13007000, 0x1300700C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x13007014, 0x13007038)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x13007800, 2048)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x13008000, 0x1300800C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x13008014, 0x13008038)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x13008800, 2048)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x13009000, 0x1300900C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x13009014, 0x13009038)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x13009800, 2048)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1300A000, 0x1300A00C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1300A014, 0x1300A038)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x1300A800, 2048)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1300B000, 0x1300B00C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1300B014, 0x1300B038)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x1300B800, 2048)}
        };

        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem, numOfChunks, unitChunkPtr);
    }
}

/*******************************************************************************
*   smemXCat3UnitGop
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  GOP unit
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
static void smemXCat3UnitGop
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = devObjPtr->deviceMemory;
    SMEM_UNIT_CHUNKS_STC * unitChunkPtr = UNIT_CHUNK_PTR_MAC(devMemInfoPtr, SMEM_XCAT3_UNIT_GOP_E);

    /* chunks with formulas */
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12000000, 0x12000094)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120000A0, 0x120000A4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120000C0, 0x120000D8)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12001000, 0x12001094)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120010A0, 0x120010A4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120010C0, 0x120010D8)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12002000, 0x12002094)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120020A0, 0x120020A4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120020C0, 0x120020D8)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12003000, 0x12003094)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120030A0, 0x120030A4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120030C0, 0x120030D8)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12004000, 0x12004094)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120040A0, 0x120040A4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120040C0, 0x120040D8)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12005000, 0x12005094)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120050A0, 0x120050A4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120050C0, 0x120050D8)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12006000, 0x12006094)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120060A0, 0x120060A4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120060C0, 0x120060D8)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12007000, 0x12007094)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120070A0, 0x120070A4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120070C0, 0x120070D8)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12008000, 0x12008094)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120080A0, 0x120080A4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120080C0, 0x120080D8)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12009000, 0x12009094)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120090A0, 0x120090A4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120090C0, 0x120090D8)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1200A000, 0x1200A094)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1200A0A0, 0x1200A0A4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1200A0C0, 0x1200A0D8)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1200B000, 0x1200B094)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1200B0A0, 0x1200B0A4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1200B0C0, 0x1200B0D8)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1200C000, 0x1200C094)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1200C0A0, 0x1200C0A4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1200C0C0, 0x1200C0D8)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1200D000, 0x1200D094)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1200D0A0, 0x1200D0A4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1200D0C0, 0x1200D0D8)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1200E000, 0x1200E094)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1200E0A0, 0x1200E0A4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1200E0C0, 0x1200E0D8)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1200F000, 0x1200F094)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1200F0A0, 0x1200F0A4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1200F0C0, 0x1200F0D8)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12010000, 0x12010094)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120100A0, 0x120100A4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120100C0, 0x120100D8)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12011000, 0x12011094)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120110A0, 0x120110A4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120110C0, 0x120110D8)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12012000, 0x12012094)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120120A0, 0x120120A4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120120C0, 0x120120D8)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12013000, 0x12013094)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120130A0, 0x120130A4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120130C0, 0x120130D8)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12014000, 0x12014094)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120140A0, 0x120140A4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120140C0, 0x120140D8)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12015000, 0x12015094)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120150A0, 0x120150A4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120150C0, 0x120150D8)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12016000, 0x12016094)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120160A0, 0x120160A4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120160C0, 0x120160D8)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12017000, 0x12017094)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120170A0, 0x120170A4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120170C0, 0x120170D8)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12018000, 0x12018094)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120180A0, 0x120180A4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120180C0, 0x120180D8)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12019000, 0x12019094)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120190A0, 0x120190A4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120190C0, 0x120190D8)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1201A000, 0x1201A094)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1201A0A0, 0x1201A0A4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1201A0C0, 0x1201A0D8)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1201B000, 0x1201B094)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1201B0A0, 0x1201B0A4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1201B0C0, 0x1201B0D8)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1201C000, 0x1201C094)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1201C0A0, 0x1201C0A4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1201C0C0, 0x1201C0D8)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1201D000, 0x1201D094)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1201D0A0, 0x1201D0A4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1201D0C0, 0x1201D0D8)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1201F000, 0x1201F094)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1201F0A0, 0x1201F0A4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1201F0C0, 0x1201F0D8)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120D8000, 0x120D8024)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120D802C, 0x120D8030)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120D8038, 0x120D8060)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120D8068, 0x120D807C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120D8084, 0x120D8088)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120D8090, 0x120D8094)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120D9000, 0x120D9024)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120D902C, 0x120D9030)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120D9038, 0x120D9060)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120D9068, 0x120D907C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120D9084, 0x120D9088)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120D9090, 0x120D9094)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120DA000, 0x120DA024)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120DA02C, 0x120DA030)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120DA038, 0x120DA060)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120DA068, 0x120DA07C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120DA084, 0x120DA088)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120DA090, 0x120DA094)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120DB000, 0x120DB024)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120DB02C, 0x120DB030)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120DB038, 0x120DB060)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120DB068, 0x120DB07C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120DB084, 0x120DB088)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120DB090, 0x120DB094)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120DC000, 0x120DC024)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120DC02C, 0x120DC030)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120DC038, 0x120DC060)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120DC068, 0x120DC07C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120DC084, 0x120DC088)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120DC090, 0x120DC094)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120DD000, 0x120DD024)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120DD02C, 0x120DD030)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120DD038, 0x120DD060)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120DD068, 0x120DD07C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120DD084, 0x120DD088)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120DD090, 0x120DD094)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120DF000, 0x120DF024)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120DF02C, 0x120DF030)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120DF038, 0x120DF060)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120DF068, 0x120DF07C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120DF084, 0x120DF088)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x120DF090, 0x120DF094)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12180600, 0x121806A0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12180704, 0x12180718)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12180C00, 0x12180C2C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12180C40, 0x12180C40)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12180C48, 0x12180CCC)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12181600, 0x121816A0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12181704, 0x12181718)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12182600, 0x121826A0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12182704, 0x12182718)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12183600, 0x121836A0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12183704, 0x12183718)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12184600, 0x121846A0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12184704, 0x12184718)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12184C00, 0x12184C2C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12184C40, 0x12184C40)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12184C48, 0x12184CCC)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12185600, 0x121856A0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12185704, 0x12185718)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12186600, 0x121866A0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12186704, 0x12186718)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12187600, 0x121876A0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12187704, 0x12187718)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12188600, 0x121886A0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12188704, 0x12188718)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12188C00, 0x12188C2C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12188C40, 0x12188C40)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12188C48, 0x12188CCC)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12189600, 0x121896A0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12189704, 0x12189718)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1218A600, 0x1218A6A0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1218A704, 0x1218A718)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1218B600, 0x1218B6A0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1218B704, 0x1218B718)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1218C600, 0x1218C6A0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1218C704, 0x1218C718)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1218CC00, 0x1218CC2C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1218CC40, 0x1218CC40)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1218CC48, 0x1218CCCC)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1218D600, 0x1218D6A0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1218D704, 0x1218D718)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1218E600, 0x1218E6A0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1218E704, 0x1218E718)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1218F600, 0x1218F6A0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1218F704, 0x1218F718)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12190600, 0x121906A0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12190704, 0x12190718)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12190C00, 0x12190C2C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12190C40, 0x12190C40)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12190C48, 0x12190CCC)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12191600, 0x121916A0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12191704, 0x12191718)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12192600, 0x121926A0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12192704, 0x12192718)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12193600, 0x121936A0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12193704, 0x12193718)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12194600, 0x121946A0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12194704, 0x12194718)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12194C00, 0x12194C2C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12194C40, 0x12194C40)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12194C48, 0x12194CCC)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12195600, 0x121956A0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12195704, 0x12195718)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12196600, 0x121966A0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12196704, 0x12196718)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12197600, 0x121976A0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12197704, 0x12197718)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12198008, 0x12198014)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12198030, 0x12198030)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219803C, 0x121980C8)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x121980D0, 0x12198120)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12198128, 0x1219814C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219815C, 0x1219817C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12198200, 0x1219820C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12198214, 0x12198218)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12198238, 0x12198278)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12198280, 0x121982A0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12198400, 0x12198418)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12198420, 0x12198420)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12198430, 0x12198434)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12198440, 0x12198440)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12198450, 0x12198454)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219845C, 0x12198498)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x121984A0, 0x121984DC)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x121984E4, 0x12198520)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12198528, 0x12198564)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219856C, 0x121985A8)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x121985B0, 0x121985E4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12198600, 0x121986A0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12198704, 0x12198718)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12198C00, 0x12198C2C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12198C40, 0x12198C40)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12198C48, 0x12198CCC)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12199008, 0x12199014)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12199030, 0x12199030)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219903C, 0x121990C8)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x121990D0, 0x12199120)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12199128, 0x1219914C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219915C, 0x1219917C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12199200, 0x1219920C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12199214, 0x12199218)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12199238, 0x12199278)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12199280, 0x121992A0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12199600, 0x121996A0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x12199704, 0x12199718)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219A008, 0x1219A014)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219A030, 0x1219A030)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219A03C, 0x1219A0C8)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219A0D0, 0x1219A120)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219A128, 0x1219A14C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219A15C, 0x1219A17C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219A200, 0x1219A20C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219A214, 0x1219A218)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219A238, 0x1219A278)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219A280, 0x1219A2A0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219A400, 0x1219A418)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219A420, 0x1219A420)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219A430, 0x1219A434)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219A440, 0x1219A440)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219A450, 0x1219A454)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219A45C, 0x1219A498)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219A4A0, 0x1219A4DC)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219A4E4, 0x1219A520)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219A528, 0x1219A564)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219A56C, 0x1219A5A8)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219A5B0, 0x1219A5E4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219A600, 0x1219A6A0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219A704, 0x1219A718)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219B008, 0x1219B014)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219B030, 0x1219B030)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219B03C, 0x1219B0C8)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219B0D0, 0x1219B120)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219B128, 0x1219B14C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219B15C, 0x1219B17C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219B200, 0x1219B20C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219B214, 0x1219B218)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219B238, 0x1219B278)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219B280, 0x1219B2A0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219B600, 0x1219B6A0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219B704, 0x1219B718)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219C008, 0x1219C014)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219C030, 0x1219C030)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219C03C, 0x1219C0C8)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219C0D0, 0x1219C120)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219C128, 0x1219C14C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219C15C, 0x1219C17C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219C200, 0x1219C20C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219C214, 0x1219C218)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219C238, 0x1219C278)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219C280, 0x1219C2A0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219C600, 0x1219C6A0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219C704, 0x1219C718)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219D008, 0x1219D014)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219D030, 0x1219D030)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219D03C, 0x1219D0C8)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219D0D0, 0x1219D120)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219D128, 0x1219D14C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219D15C, 0x1219D17C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219D200, 0x1219D20C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219D214, 0x1219D218)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219D238, 0x1219D278)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219D280, 0x1219D2A0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219D600, 0x1219D6A0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219D704, 0x1219D718)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219DC00, 0x1219DC2C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219DC40, 0x1219DC40)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219DC48, 0x1219DCCC)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219F008, 0x1219F014)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219F030, 0x1219F030)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219F03C, 0x1219F0C8)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219F0D0, 0x1219F120)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219F128, 0x1219F14C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219F15C, 0x1219F17C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219F200, 0x1219F20C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219F214, 0x1219F218)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219F238, 0x1219F278)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219F280, 0x1219F2A0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219F600, 0x1219F6A0)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1219F704, 0x1219F718)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x121B2008, 0x121B2014)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x121B2030, 0x121B2030)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x121B203C, 0x121B20C8)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x121B20D0, 0x121B2120)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x121B2128, 0x121B214C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x121B215C, 0x121B217C)}
        };

        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem, numOfChunks, unitChunkPtr);
    }
}

/*******************************************************************************
*   smemXCat3UnitEq
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
static void smemXCat3UnitEq
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = devObjPtr->deviceMemory;
    SMEM_UNIT_CHUNKS_STC * unitChunkPtr = UNIT_CHUNK_PTR_MAC(devMemInfoPtr, SMEM_XCAT3_UNIT_EQ_E);

    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x14000000, 0x14000068)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x14000074, 0x14000074)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x140000A0, 0x140000B4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x14001000, 0x14001000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x14002000, 0x14002000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x14003000, 0x14003000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x14004000, 0x14004000)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x14005000, 0x14005008)},

            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x14007000, 128),
             SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4),SMEM_BIND_TABLE_MAC(tcpUdpDstPortRangeCpuCode)},

            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x14008000, 16),
             SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4),SMEM_BIND_TABLE_MAC(ipProtCpuCode)},

            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x14008010, 0x14008010)},

            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x14009000, 64),
             SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4),SMEM_BIND_TABLE_MAC(trunkNumOfMembers)},

            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1400A000, 0x1400A004)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1400B000, 0x1400B008)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1400B010, 0x1400B018)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1400B020, 0x1400B038)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1400B040, 0x1400B040)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1400B050, 0x1400B054)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x14020000, 0x1402000C)},

            /* Ingress STC Table */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x14040000, 448),
             SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(80, 16),SMEM_BIND_TABLE_MAC(ingrStc)},

            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x14050000, 0x14050008)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x14080004, 0x1408007C)},

            /* Statistical Rate Limits Table */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x14100000, 128),
             SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4),SMEM_BIND_TABLE_MAC(statisticalRateLimit)},

            /* CPU Code Table */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x14200000, 1024),
             SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4),SMEM_BIND_TABLE_MAC(cpuCode)},

            /* QoS Profile to QoS Table */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x14300000, 512),
             SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(14, 4),SMEM_BIND_TABLE_MAC(qosProfile)},

            /* Trunks Members Table */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x14400000, 4096),
             SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(11, 4),SMEM_BIND_TABLE_MAC(trunkMembers)},

            /* TO CPU Rate Limiter Configuration Table */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x14500000, 124)},

            /* Logical Target Mapping Table */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x14600000, 16384),
             SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(45, 8),SMEM_BIND_TABLE_MAC(logicalTrgMappingTable)},

            /* EgressFilterVlanMap Table */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x14700000, 16384),
             SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(12, 4),SMEM_BIND_TABLE_MAC(eqVlanMapping)},

            /* EgressFilterVlanMember Table */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x14704000, 16384),
             SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(64, 8),SMEM_BIND_TABLE_MAC(eqLogicalPortEgressVlanMember)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitChunkPtr);
    }
}

/*******************************************************************************
*   smemXCat3UnitIpcl
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
static void smemXCat3UnitIpcl
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = devObjPtr->deviceMemory;
    SMEM_UNIT_CHUNKS_STC * unitChunkPtr = UNIT_CHUNK_PTR_MAC(devMemInfoPtr, SMEM_XCAT3_UNIT_IPCL_E);

    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x15000000, 0x1500001C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x15000040, 0x15000050)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x15000068, 0x15000070)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x15000080, 0x15000088)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x15000200, 0x1500026C)},

            /* PCL DFX Area Table */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x15000800, 2048)},

            /* IPCL0 Configuration Table */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x15010000, 33792)},

            /* IPCL1 Configuration Table */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x15020000, 33792)},

            /* Internal Inlif memory */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x15030000, 33280)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitChunkPtr);
    }
}

/*******************************************************************************
*   smemXCat3UnitIplr0
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the xCat3 IPLR0 unit
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
static void smemXCat3UnitIplr0
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    smemXCat3UnitPolicerUnify(devObjPtr, SMEM_XCAT3_UNIT_IPLR0_E, policerBaseAddr[0]);
}

/*******************************************************************************
*   smemXCat3UnitIpclTcc
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
static void smemXCat3UnitIpclTcc
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = devObjPtr->deviceMemory;
    SMEM_UNIT_CHUNKS_STC * unitChunkPtr = UNIT_CHUNK_PTR_MAC(devMemInfoPtr, SMEM_XCAT3_UNIT_TCC_LOWER_E);

    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1A000084, 0x1A00009C)},

            /* Action Table and Policy TCAM Access Data */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x1A000100, 48),
             SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4),SMEM_BIND_TABLE_MAC(pclActionTcamData)},

            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1A000130, 0x1A000138)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1A000148, 0x1A000148)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1A000168, 0x1A000170)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1A000198, 0x1A000198)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1A0001A4, 0x1A0001A8)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1A000208, 0x1A000208)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1A000218, 0x1A000218)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1A000220, 0x1A000220)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1A000238, 0x1A000238)},

            /* TCAM Array Compare Enable Table */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x1A002080, 128)},

            /* Policy TCAM Table */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x1A040000, 196608),
             SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(52, 8),SMEM_BIND_TABLE_MAC(pclTcam)},

            /* Policy Action Table */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x1A0B0000, 49152),
             SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(128, 16),SMEM_BIND_TABLE_MAC(pclAction)},

            /* Policy ECC X Table */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x1A0C0000, 12288)},

            /* Policy ECC Y Table */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x1A0D0000, 12288)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitChunkPtr);
    }
}

/*******************************************************************************
*   smemXCat3UnitEpcl
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the xCat3 EPCL unit
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
static void smemXCat3UnitEpcl
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = devObjPtr->deviceMemory;
    SMEM_UNIT_CHUNKS_STC * unitChunkPtr = UNIT_CHUNK_PTR_MAC(devMemInfoPtr, SMEM_XCAT3_UNIT_EPCL_E);

    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1D000000, 0x1D000040)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1D000100, 0x1D00011C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1D000140, 0x1D00015C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1D000200, 0x1D000200)},

            /* EPCL Configuration Table*/
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x1D008000, 16640),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(17, 4),SMEM_BIND_TABLE_MAC(epclConfigTable)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitChunkPtr);
    }
}

/*******************************************************************************
*   smemXCat3UnitIpvxTcc
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
static void smemXCat3UnitIpvxTcc
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = devObjPtr->deviceMemory;
    SMEM_UNIT_CHUNKS_STC * unitChunkPtr = UNIT_CHUNK_PTR_MAC(devMemInfoPtr, SMEM_XCAT3_UNIT_TCC_UPPER_E);

    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1B000054, 0x1B000054)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1B000060, 0x1B000064)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1B000080, 0x1B000080)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1B0000A0, 0x1B0000A4)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1B000400, 0x1B00041C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1B000960, 0x1B000960)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1B00096C, 0x1B00097C)},
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x1B000D74, 0x1B000D78)},

            /* TCAM Array Compare Enable Table */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x1B002080, 128)},

            /* Router and TT TCAM Table */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x1B040000, 212992),
             SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(52, 8),SMEM_BIND_TABLE_MAC(routerTcam)},

            /* ECC X Table */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x1B090000, 6656)},

            /* ECC Y Table */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x1B0A0000, 6656)},

            /* Lookup Translation Table */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x1B0C0000, 106496),
             SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(148, 32),SMEM_BIND_TABLE_MAC(ttiAction)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitChunkPtr);
    }
}

/*******************************************************************************
*   smemXCat3AllocSpecMemory
*
* DESCRIPTION:
*       Allocate xCat3 address type specific memories.
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
static void smemXCat3AllocSpecMemory
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = devObjPtr->deviceMemory;

    /* init the internal memory */
    smemChtInitInternalSimMemory(devObjPtr,&devMemInfoPtr->common);

    smemLion2UnitPex(devObjPtr,&devMemInfoPtr->PEX_UnitMem, devMemInfoPtr->common.pciUnitBaseAddr);

    smemXcat3UnitDfx(devObjPtr);

    /* Create all the memories of the units */
    smemXCat3UnitMg(devObjPtr);
    smemXCat3UnitTxqEgr(devObjPtr);
    smemXCat3UnitEq(devObjPtr);
    smemXCat3UnitIplr0(devObjPtr);
    smemXCat3UnitIplr1(devObjPtr);
    smemXCat3UnitEplr(devObjPtr);
    smemXCat3UnitMll(devObjPtr);
    smemXCat3UnitTti(devObjPtr);
    smemXCat3UnitL2i(devObjPtr);
    smemXCat3UnitIpvx(devObjPtr);
    smemXCat3UnitBm(devObjPtr);
    smemXCat3UnitLms(devObjPtr);
    smemXCat3UnitFdb(devObjPtr);
    smemXCat3UnitEpcl(devObjPtr);
    smemXCat3UnitHa(devObjPtr);
    smemXCat3UnitMppm0(devObjPtr);
    smemXCat3UnitMppm1(devObjPtr);
    smemXCat3UnitCentralizedCounters(devObjPtr);
    smemXCat3UnitMsm(devObjPtr);
    smemXCat3UnitSerdes(devObjPtr);
    smemXCat3UnitGop(devObjPtr);
    smemXCat3UnitTxqEgrVlan(devObjPtr);
    smemXCat3UnitCcfc(devObjPtr);
    smemXCat3UnitIpcl(devObjPtr);
    smemXCat3UnitIpclTcc(devObjPtr);
    smemXCat3UnitIpvxTcc(devObjPtr);
    smemXCat3UnitMem(devObjPtr);

    devMemInfoPtr->common.unitChunksBasePtr = (GT_U8 *)devMemInfoPtr->unitMemArr;
    /* Calculate summary of all memory structures - for future SW reset use */
    devMemInfoPtr->common.unitChunksSizeOf = sizeof(devMemInfoPtr->unitMemArr);

}

/*******************************************************************************
*  smemXCat3TableInfoSet
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
void smemXCat3TableInfoSet
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    /* must be set before calling smemXcatA1TableInfoSet(...) */

    /* use xcat A1 table addresses */
    smemXcatA1TableInfoSet(devObjPtr);

    devObjPtr->tablesInfo.ieeeRsrvMcConfTable.commonInfo.baseAddress = 0x01000800;
    devObjPtr->tablesInfo.ieeeRsrvMcConfTable.paramInfo[0].step = 0x1000;
    devObjPtr->tablesInfo.ieeeRsrvMcConfTable.paramInfo[1].step = 0x8;
    devObjPtr->tablesInfo.ieeeRsrvMcConfTable.paramInfo[0].divider = 16;

    devObjPtr->tablesInfo.ieeeRsrvMcCpuIndex.commonInfo.baseAddress = 0x01000804;
    devObjPtr->tablesInfo.ieeeRsrvMcCpuIndex.paramInfo[0].step = 0x1000;
    devObjPtr->tablesInfo.ieeeRsrvMcCpuIndex.paramInfo[0].divider = 16;

    devObjPtr->tablesInfo.ieeeTblSelect.commonInfo.baseAddress = 0x0100080c;
    devObjPtr->tablesInfo.ieeeTblSelect.paramInfo[0].step = 0x10;
    devObjPtr->tablesInfo.ieeeTblSelect.paramInfo[0].divider = 16;

    devObjPtr->tablesInfo.nonTrunkMembers.commonInfo.baseAddress = 0x02a40000;
    devObjPtr->tablesInfo.nonTrunkMembers.paramInfo[0].step = 0x10;

    devObjPtr->tablesInfo.deviceMapTable.commonInfo.baseAddress = 0x02a40004;
    devObjPtr->tablesInfo.deviceMapTable.paramInfo[0].step = 0x10;

    devObjPtr->tablesInfo.sst.commonInfo.baseAddress = 0x02a4000c;
    devObjPtr->tablesInfo.sst.paramInfo[0].step = 0x10;

    devObjPtr->tablesInfo.egressStc.commonInfo.baseAddress = 0x02d40000;
    devObjPtr->tablesInfo.egressStc.paramInfo[0].step = 0x200;

    devObjPtr->tablesInfo.secondTargetPort.commonInfo.baseAddress = 0x02e40000;
    devObjPtr->tablesInfo.secondTargetPort.paramInfo[0].step = 0x10;

    devObjPtr->tablesInfo.l2PortIsolation.commonInfo.baseAddress = 0x02e40004;
    devObjPtr->tablesInfo.l2PortIsolation.paramInfo[0].step = 0x10;

    devObjPtr->tablesInfo.l3PortIsolation.commonInfo.baseAddress = 0x02e40008;
    devObjPtr->tablesInfo.l3PortIsolation.paramInfo[0].step = 0x10;

    devObjPtr->tablesInfo.designatedPorts.commonInfo.baseAddress = 0x02e4000c;
    devObjPtr->tablesInfo.designatedPorts.paramInfo[0].step = 0x10;

    devObjPtr->tablesInfo.xgPortMibCounters.commonInfo.baseAddress = 0x11000000;
    devObjPtr->tablesInfo.xgPortMibCounters.paramInfo[0].step = MSM_MIB_COUNTER_STEP_PER_PORT_CNS;

    devObjPtr->xgCountersStepPerPort = MSM_MIB_COUNTER_STEP_PER_PORT_CNS;

    /* devObjPtr->tablesInfo.tcpUdpDstPortRangeCpuCode.commonInfo.baseAddress = 0x14007000 */
    devObjPtr->tablesInfo.tcpUdpDstPortRangeCpuCode.paramInfo[1].step = 0x40;

    /* devObjPtr->tablesInfo.ipProtCpuCode.commonInfo.baseAddress = 0x14008000 */
    devObjPtr->tablesInfo.ipProtCpuCode.paramInfo[0].divider = 2;

    /* devObjPtr->tablesInfo.trunkNumOfMembers.commonInfo.baseAddress = 0x14009000 */
    devObjPtr->tablesInfo.trunkNumOfMembers.paramInfo[0].divider = 8;

    /* devObjPtr->tablesInfo.trunkMembers.commonInfo.baseAddress = 0x14400000 */
    devObjPtr->tablesInfo.trunkMembers.paramInfo[1].step = 0x20;

    devObjPtr->tablesInfo.pclConfig.commonInfo.baseAddress = 0x15010000;
    devObjPtr->tablesInfo.pclConfig.paramInfo[0].step = 0x8; /*entry*/
    devObjPtr->tablesInfo.pclConfig.paramInfo[1].step = 0x10000; /* IPCL0/IPCL1 */

    /* devObjPtr->tablesInfo.dscpToQoSProfile.commonInfo.baseAddress = 0x16000400 */
    devObjPtr->tablesInfo.dscpToQoSProfile.paramInfo[0].divider = 4;

    devObjPtr->tablesInfo.upToQoSProfile.commonInfo.baseAddress = 0x16000440;
    devObjPtr->tablesInfo.upToQoSProfile.paramInfo[0].divider = 4;
    devObjPtr->tablesInfo.upToQoSProfile.paramInfo[0].step = 0x4;
    devObjPtr->tablesInfo.upToQoSProfile.paramInfo[1].step = 0x10;

    /* devObjPtr->tablesInfo.expToQoSProfile.commonInfo.baseAddress = 0x16000460 */
    devObjPtr->tablesInfo.expToQoSProfile.paramInfo[0].divider = 4;

    /* devObjPtr->tablesInfo.dscpToDscpMap.commonInfo.baseAddress = 0x16000900 */
    devObjPtr->tablesInfo.expToQoSProfile.paramInfo[0].divider = 4;

    devObjPtr->tablesInfo.macToMe.commonInfo.baseAddress = 0x16001600;
    devObjPtr->tablesInfo.macToMe.paramInfo[0].step = 0x10;

    devObjPtr->tablesInfo.pclTcamMask.commonInfo.baseAddress = 0x1A040020;
    devObjPtr->tablesInfo.pclTcamMask.paramInfo[0].step = 0x40;
    devObjPtr->tablesInfo.pclTcamMask.paramInfo[1].step = 0x8;

    /* devObjPtr->tablesInfo.policerIpfixWaAlert.commonInfo.baseAddress = 0x17000800 */
    /* devObjPtr->tablesInfo.policerIpfixWaAlert.paramInfo[0].divider = 32; */

    /* devObjPtr->tablesInfo.policerIpfixAgingAlert.commonInfo.baseAddress = 0x17000900 */
    /* devObjPtr->tablesInfo.policerIpfixAgingAlert.paramInfo[0].divider = 32; */

    devObjPtr->memUnitBaseAddrInfo.mll = 0x19000000;

    devObjPtr->tablesInfo.mllOutInterfaceCounter.commonInfo.baseAddress = 0x19000900;
    devObjPtr->tablesInfo.mllOutInterfaceCounter.paramInfo[0].step = 0x100;

    devObjPtr->tablesInfo.mllOutInterfaceConfig.commonInfo.baseAddress = 0x19000980;
    devObjPtr->tablesInfo.mllOutInterfaceConfig.paramInfo[0].step = 0x100;

    devObjPtr->tablesInfo.mll.commonInfo.baseAddress = 0x19080000;
    devObjPtr->tablesInfo.mll.paramInfo[0].step = 0x10;

    /* devObjPtr->tablesInfo.routerTcam.commonInfo.baseAddress = 0x1B040000 */
    devObjPtr->tablesInfo.routerTcam.paramInfo[1].step = 0x4;

    /* devObjPtr->tablesInfo.ttiAction.commonInfo.baseAddress = 0x1B0C0000 */
    devObjPtr->tablesInfo.ttiAction.paramInfo[1].step = 0x4;

    /* devObjPtr->tablesInfo.arp.commonInfo.baseAddress = 0x1C040000 */
    devObjPtr->tablesInfo.arp.paramInfo[0].divider = 4;

    devObjPtr->globalInterruptCauseRegister.ha  = 25;
}

/*******************************************************************************
*  smemXCat3TableInfoSetPart2
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
void smemXCat3TableInfoSetPart2
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    /* devObjPtr->tablesInfo.cncMemory.commonInfo.baseAddress = 0x10080000 */
    devObjPtr->tablesInfo.cncMemory.paramInfo[0].step = 0x4000;
    devObjPtr->tablesInfo.cncMemory.paramInfo[1].step = 8;

    /* devObjPtr->tablesInfo.logicalTrgMappingTable.commonInfo.baseAddress = 0x14600000 */
    devObjPtr->tablesInfo.logicalTrgMappingTable.paramInfo[0].step = 0x200;
    devObjPtr->tablesInfo.logicalTrgMappingTable.paramInfo[1].step = 0x8;

    /* devObjPtr->tablesInfo.portProtocolVidQoSConf.commonInfo.baseAddress = 0x16004000 */
    devObjPtr->tablesInfo.portProtocolVidQoSConf.paramInfo[0].step = 0x80;
    devObjPtr->tablesInfo.portProtocolVidQoSConf.paramInfo[1].step = 0x8;

    /* devObjPtr->tablesInfo.pclTcam.commonInfo.baseAddress = 0x1A040000 */
    devObjPtr->tablesInfo.pclTcam.paramInfo[0].step = 0x40;
    devObjPtr->tablesInfo.pclTcam.paramInfo[1].step = 0x8;

    devObjPtr->tablesInfo.tunnelStart.commonInfo.baseAddress = devObjPtr->tablesInfo.arp.commonInfo.baseAddress;
    devObjPtr->tablesInfo.tunnelStart.paramInfo[0].step = devObjPtr->tablesInfo.arp.paramInfo[0].step;

    devObjPtr->policerSupport.meterTblBaseAddr = devObjPtr->tablesInfo.policer.commonInfo.baseAddress;
    devObjPtr->policerSupport.countTblBaseAddr = devObjPtr->tablesInfo.policerCounters.commonInfo.baseAddress;
}

/*******************************************************************************
* smemXCat3RegsInfoSet
*
* DESCRIPTION:
*       Init memory module for xCat3 and above devices.
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
void smemXCat3RegsInfoSet
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
        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,egrAndTxqVer0      ,0x02000000);
    /*    SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,egrAndTxqVer1      ,0); not use unit */
        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,L2I                ,0x01000000);
    /*    SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,ucRouter           ,0); not use unit */
        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,IPvX               ,0x04000000);
        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,BM                 ,0x05000000);
        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,LMS                ,0x07000000);
    /*    SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,LMS1               ,0); not used unit */
        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,FDB                ,0x0b000000);
    /*    SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,MPPM0              ,0); not used unit */
    /*    SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,MPPM1              ,0); not used unit */
    /*    SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,CTU0               ,0); not used unit */
    /*    SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,CTU1               ,0); not used unit */
        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,EPCL               ,0x1d000000);
        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,HA                 ,0x1c000000);
    /*    SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,RXDMA              ,0); not used unit */
    /*    SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,TXDMA              ,0); not used unit */
        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,MEM                ,0x0e000000);
        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,centralizedCntrs   ,0x10000000);
    /*    SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,uniphySERDES       ,0); not used unit */
        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,GOP.SERDESes       ,0x13000000);
    /*    SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,networkPorts       ,0); not used unit */
        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,GOP.ports          ,0x12000000);
        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,EQ                 ,0x14000000);
        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,IPCL               ,0x15000000);
        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,PLR[0]             ,0x17000000);
        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,PLR[1]             ,0x18000000);
        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,MLL                ,0x19000000);
        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,TCCLowerIPCL       ,0x1a000000);
        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,TCCUpperIPvX       ,0x1b000000);
        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,TTI                ,0x16000000);
        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,PLR[2]             ,0x06000000);
    /*        SMEM_CHT_PP_REGS_UNIT_START_INFO_SET_MAC(devObjPtr,temp               ,0); not used unit */
    }



    {/*start of unit MG - MG  */
        {/*start of unit TWSIInternalRegs */
            regAddrDbPtr->MG.TWSIConfig.TWSIInternalBaudRate = 0x0008000c;
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

            regAddrDbPtr->MG.genxsReadDMARegs.genxsReadDMABufferPointer = 0x00000090;
            regAddrDbPtr->MG.genxsReadDMARegs.genxsReadDMAStartingAddr = 0x00000094;
            regAddrDbPtr->MG.genxsReadDMARegs.genxsReadDMACtrl = 0x00000098;

            regAddrDbPtr->MG.globalConfig.globalCtrl = 0x00000058;
            regAddrDbPtr->MG.globalConfig.extGlobalCtrl = 0x0000005c;

            regAddrDbPtr->MG.globalInterrupt.functionalInterruptsSummaryCause = 0x000003F8;
            regAddrDbPtr->MG.globalInterrupt.functionalInterruptsSummaryMask = 0x000003FC;

            regAddrDbPtr->MG.globalInterrupt.ports1InterruptsSummaryCause = 0x00000150;
            regAddrDbPtr->MG.globalInterrupt.ports1InterruptsSummaryMask  = 0x00000154;

            regAddrDbPtr->MG.globalInterrupt.ports2InterruptsSummaryCause = 0x00000158;
            regAddrDbPtr->MG.globalInterrupt.ports2InterruptsSummaryMask  = 0x0000015c;

            regAddrDbPtr->MG.addrUpdateQueueConfig.GeneralControl = 0x000000D4;
            regAddrDbPtr->MG.addrUpdateQueueConfig.AUQHostConfiguration = 0x000000D8;

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
        }/*end of unit MGRegs */

        {/* start of unit MGRegs */
            regAddrDbPtr->MG.XSMI.XSMIManagement = 0x00040000;
            regAddrDbPtr->MG.XSMI.XSMIAddr = 0x00040008;
            regAddrDbPtr->MG.XSMI.XSMIConfig = 0x0004000c;
            regAddrDbPtr->MG.XSMI.XSMIInterruptCause = 0x00040010;
            regAddrDbPtr->MG.XSMI.XSMIInterruptMask = 0x00040014;
        }/* end of unit MGRegs */

        regAddrDbPtr->MG.globalConfig.addrCompletion = 0x00000000;
        regAddrDbPtr->MG.globalConfig.sampledAtResetReg = 0x00000028;
        regAddrDbPtr->MG.globalConfig.lastReadTimeStamp = 0x00000040;
        regAddrDbPtr->MG.globalConfig.deviceID = 0x0000004c;
        regAddrDbPtr->MG.globalConfig.vendorID = 0x00000050;
        regAddrDbPtr->MG.globalConfig.mediaInterface = 0x0000038c;
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

        regAddrDbPtr->MG.SDMARegs.transmitSDMAQueueCommand = 0x00002868;
        regAddrDbPtr->MG.SDMARegs.transmitSDMAFixedPriorityConfig = 0x00002870;
        regAddrDbPtr->MG.SDMARegs.transmitSDMAWRRTokenParameters = 0x00002874;

        regAddrDbPtr->MG.SDMARegs.receiveSDMAInterruptCause = 0x0000280c;
        regAddrDbPtr->MG.SDMARegs.transmitSDMAInterruptCause = 0x00002810;
        regAddrDbPtr->MG.SDMARegs.receiveSDMAInterruptMask = 0x00002814;
        regAddrDbPtr->MG.SDMARegs.transmitSDMAInterruptMask = 0x00002818;

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

    {/*start of unit egrAndTxqVer0 - Egress and Txq  */
        {/*start of unit oam8023ahLoopbackEnablePerPortReg */
            regAddrDbPtr->egrAndTxqVer0.oam8023ahLoopbackEnablePerPortReg.oam8023ahLoopbackEnablePerPortReg = 0x02800094;

        }/*end of unit oam8023ahLoopbackEnablePerPortReg */


        {/*start of unit bridgeEgrConfig */
            regAddrDbPtr->egrAndTxqVer0.bridgeEgrConfig.mcLocalEnableConfig = 0x02800008;
            regAddrDbPtr->egrAndTxqVer0.bridgeEgrConfig.egrFilteringReg0 = 0x02800010;
            regAddrDbPtr->egrAndTxqVer0.bridgeEgrConfig.egrFilteringReg1 = 0x02800014;
            regAddrDbPtr->egrAndTxqVer0.bridgeEgrConfig.IPMcRoutedPktsEgrFilterConfig = 0x028001d0;
            regAddrDbPtr->egrAndTxqVer0.bridgeEgrConfig.egrFilteringReg2 = 0x028001f4;

        }/*end of unit bridgeEgrConfig */


        {/*start of unit CCFC */
            regAddrDbPtr->egrAndTxqVer0.CCFC.BCNCtrl = 0x03400000;
            regAddrDbPtr->egrAndTxqVer0.CCFC.pauseTriggerEnable = 0x0340000c;
            regAddrDbPtr->egrAndTxqVer0.CCFC.BCNGlobalPauseThresholds = 0x03400014;
            regAddrDbPtr->egrAndTxqVer0.CCFC.BCNInterruptCause = 0x03400018;
            regAddrDbPtr->egrAndTxqVer0.CCFC.BCNInterruptMask = 0x0340001c;
            regAddrDbPtr->egrAndTxqVer0.CCFC.portsBCNProfileReg[0] = 0x03400020;
            regAddrDbPtr->egrAndTxqVer0.CCFC.portsBCNProfileReg[1] = 0x03400024;
            regAddrDbPtr->egrAndTxqVer0.CCFC.portsBCNProfileReg[2] = 0x03400028;
            regAddrDbPtr->egrAndTxqVer0.CCFC.portsSpeedIndexesReg[0] = 0x03400034;
            regAddrDbPtr->egrAndTxqVer0.CCFC.portsSpeedIndexesReg[1] = 0x03400038;
            regAddrDbPtr->egrAndTxqVer0.CCFC.portsSpeedIndexesReg[2] = 0x0340003c;
            regAddrDbPtr->egrAndTxqVer0.CCFC.globalBuffersCntr = 0x034001fc;
            regAddrDbPtr->egrAndTxqVer0.CCFC.BCNPortsTriggerCntrsConfig = 0x03401000;
            regAddrDbPtr->egrAndTxqVer0.CCFC.BCNQueuesTriggerCntrsConfig = 0x03401004;
            regAddrDbPtr->egrAndTxqVer0.CCFC.globalXoffCntr = 0x0340100c;
            regAddrDbPtr->egrAndTxqVer0.CCFC.portBCNTriggerCntr = 0x03401014;
            regAddrDbPtr->egrAndTxqVer0.CCFC.queueBCNTriggerCntr = 0x03401018;

        }/*end of unit CCFC */


        {/*start of unit CNCModes */
            regAddrDbPtr->egrAndTxqVer0.CNCModes.CNCModesReg = 0x028000a0;

        }/*end of unit CNCModes */


        {/*start of unit dequeueSWRR */
            regAddrDbPtr->egrAndTxqVer0.dequeueSWRR.dequeueSWRRWeightReg[0] = 0x028000b0;
            regAddrDbPtr->egrAndTxqVer0.dequeueSWRR.dequeueSWRRWeightReg[1] = 0x028000b4;
            regAddrDbPtr->egrAndTxqVer0.dequeueSWRR.dequeueSWRRWeightReg[2] = 0x028000b8;
            regAddrDbPtr->egrAndTxqVer0.dequeueSWRR.dequeueSWRRWeightReg[3] = 0x028000bc;

        }/*end of unit dequeueSWRR */


        {/*start of unit deviceMapTable */
            {/*0x2a40004+n*0x10*/
                GT_U32    n;
                for(n = 0 ; n <= 31 ; n++) {
                    regAddrDbPtr->egrAndTxqVer0.deviceMapTable.deviceMapTableEntry[n] =
                        0x2a40004+n*0x10;
                }/* end of loop n */
            }/*0x2a40004+n*0x10*/



        }/*end of unit deviceMapTable */


        {/*start of unit DPToCFIMap */
            regAddrDbPtr->egrAndTxqVer0.DPToCFIMap.DP2CFIEnablePerPortReg = 0x028000a4;
            regAddrDbPtr->egrAndTxqVer0.DPToCFIMap.DP2CFIReg = 0x028000a8;

        }/*end of unit DPToCFIMap */


        {/*start of unit egrForwardingRestrictions */
            regAddrDbPtr->egrAndTxqVer0.egrForwardingRestrictions.fromCPUPktsFilterConfig = 0x028001d8;
            regAddrDbPtr->egrAndTxqVer0.egrForwardingRestrictions.bridgedPktsFilterConfig = 0x028001dc;
            regAddrDbPtr->egrAndTxqVer0.egrForwardingRestrictions.routedPktsFilterConfig = 0x028001e0;

        }/*end of unit egrForwardingRestrictions */


        {/*start of unit egrRateShapersConfig */
            regAddrDbPtr->egrAndTxqVer0.egrRateShapersConfig.tokenBucketUpdateRateAndMCFIFOConfig = 0x0280000c;
            {/*0x2ac0000+k * 0x8000 + n * 0x200 and 0x2ac0000+k * 0x8000 + n * 0x200*/
                GT_U32    n,k;
                for(n = 0 ; n <= 27 ; n++) {
                    for(k = 0 ; k <= 7 ; k++) {
                        regAddrDbPtr->egrAndTxqVer0.egrRateShapersConfig.portTCTokenBucketConfig[n][k] =
                            0x2ac0000+k * 0x8000 + n * 0x200;
                    }/* end of loop k */
                }/* end of loop n */
                for(n = 63 ; n <= 63 ; n++) {
                    for(k = 0 ; k <= 7 ; k++) {
                        regAddrDbPtr->egrAndTxqVer0.egrRateShapersConfig.portTCTokenBucketConfig[(n - 63) + 28][k] =
                            0x2ac0000+k * 0x8000 + n * 0x200;
                    }/* end of loop k */
                }/* end of loop n */
            }/*0x2ac0000+k * 0x8000 + n * 0x200 and 0x2ac0000+k * 0x8000 + n * 0x200*/


            {/*0x2ac0008+n * 0x200 and 0x2ac0008+n * 0x200*/
                GT_U32    n;
                for(n = 0 ; n <= 27 ; n++) {
                    regAddrDbPtr->egrAndTxqVer0.egrRateShapersConfig.portTokenBucketConfig[n] =
                        0x2ac0008+n * 0x200;
                }/* end of loop n */
                for(n = 63 ; n <= 63 ; n++) {
                    regAddrDbPtr->egrAndTxqVer0.egrRateShapersConfig.portTokenBucketConfig[(n - 63) + 28] =
                        0x2ac0008+n * 0x200;
                }/* end of loop n */
            }/*0x2ac0008+n * 0x200 and 0x2ac0008+n * 0x200*/



        }/*end of unit egrRateShapersConfig */


        {/*start of unit egrSTCTable */
            {/*0x2d40000+n*0x200*/
                GT_U32    n;
                for(n = 0 ; n <= 27 ; n++) {
                    regAddrDbPtr->egrAndTxqVer0.egrSTCTable.portEgrSTCTableEntryWord0[n] =
                        0x2d40000+n*0x200;
                }/* end of loop n */
            }/*0x2d40000+n*0x200*/


            {/*0x2d40004+n*0x200*/
                GT_U32    n;
                for(n = 0 ; n <= 27 ; n++) {
                    regAddrDbPtr->egrAndTxqVer0.egrSTCTable.portEgrSTCTableEntryWord1[n] =
                        0x2d40004+n*0x200;
                }/* end of loop n */
            }/*0x2d40004+n*0x200*/


            {/*0x2d40008+n*0x200*/
                GT_U32    n;
                for(n = 0 ; n <= 27 ; n++) {
                    regAddrDbPtr->egrAndTxqVer0.egrSTCTable.portEgrSTCTableEntryWord2[n] =
                        0x2d40008+n*0x200;
                }/* end of loop n */
            }/*0x2d40008+n*0x200*/



        }/*end of unit egrSTCTable */


        {/*start of unit fastStackFailover */
            {/*0x2e40000+n*0x10*/
                GT_U32    n;
                for(n = 0 ; n <= 27 ; n++) {
                    regAddrDbPtr->egrAndTxqVer0.fastStackFailover.secondaryTargetPortMapTableEntry[n] =
                        0x2e40000+n*0x10;
                }/* end of loop n */
            }/*0x2e40000+n*0x10*/



        }/*end of unit fastStackFailover */


        {/*start of unit GPPRegs */
            regAddrDbPtr->egrAndTxqVer0.GPPRegs.GPPInputReg = 0x028001bc;
            regAddrDbPtr->egrAndTxqVer0.GPPRegs.GPPOutputReg = 0x028001c4;
            regAddrDbPtr->egrAndTxqVer0.GPPRegs.GPPIOCtrlReg = 0x028001c8;

        }/*end of unit GPPRegs */


        {/*start of unit linkListCtrl */
            regAddrDbPtr->egrAndTxqVer0.linkListCtrl.linkListCtrlConfig = 0x028000d0;

        }/*end of unit linkListCtrl */


        {/*start of unit mirroredPktsToAnalyzerPortDescsLimit */
            regAddrDbPtr->egrAndTxqVer0.mirroredPktsToAnalyzerPortDescsLimit.mirroredPktsToAnalyzerPortDescsLimitConfig = 0x02800024;

        }/*end of unit mirroredPktsToAnalyzerPortDescsLimit */


        {/*start of unit perPortTransmitQueueConfig */
            {/*0x2800080+n * 0x200 and 0x2800080+n * 0x200*/
                GT_U32    n;
                for(n = 0 ; n <= 27 ; n++) {
                    regAddrDbPtr->egrAndTxqVer0.perPortTransmitQueueConfig.portTxqConfig[n] =
                        0x2800080 + n * 0x200;
                }/* end of loop n */
                for(n = 63 ; n <= 63 ; n++) {
                    regAddrDbPtr->egrAndTxqVer0.perPortTransmitQueueConfig.portTxqConfig[(n - 63) + 28] =
                        0x2800080 + n * 0x200;
                }/* end of loop n */
            }/*0x2800080+n * 0x200 and 0x2800080+n * 0x200*/



        }/*end of unit perPortTransmitQueueConfig */


        {/*start of unit portIsolationConfig */
            {/*0x2e40004+0x10*n*/
                GT_U32    n;
                for(n = 0 ; n <= 2175 ; n++) {
                    regAddrDbPtr->egrAndTxqVer0.portIsolationConfig.L2PortIsolationTableEntry[n] =
                        0x2e40004+0x10*n;
                }/* end of loop n */
            }/*0x2e40004+0x10*n*/


            {/*0x2e40008+0x10*n*/
                GT_U32    n;
                for(n = 0 ; n <= 2175 ; n++) {
                    regAddrDbPtr->egrAndTxqVer0.portIsolationConfig.L3PortIsolationTableEntry[n] =
                        0x2e40008+0x10*n;
                }/* end of loop n */
            }/*0x2e40008+0x10*n*/



        }/*end of unit portIsolationConfig */


        {/*start of unit portL1OverheadConfig */
            {/*0x2a80000+n * 0x200 and 0x2a80000+n * 0x200*/
                GT_U32    n;
                for(n = 0 ; n <= 27 ; n++) {
                    regAddrDbPtr->egrAndTxqVer0.portL1OverheadConfig.portL1OverheadConfig[n] =
                        0x2a80000+n * 0x200;
                }/* end of loop n */
                for(n = 63 ; n <= 63 ; n++) {
                    regAddrDbPtr->egrAndTxqVer0.portL1OverheadConfig.portL1OverheadConfig[(n - 63) + 28] =
                        0x2a80000+n * 0x200;
                }/* end of loop n */
            }/*0x2a80000+n * 0x200 and 0x2a80000+n * 0x200*/



        }/*end of unit portL1OverheadConfig */


        {/*start of unit resourceSharingRegs */
            {/*0x2800028+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->egrAndTxqVer0.resourceSharingRegs.priorityReg[n] =
                        0x2800028+n*0x4;
                }/* end of loop n */
            }/*0x2800028+n*0x4*/



        }/*end of unit resourceSharingRegs */


        {/*start of unit sourceIDEgrFiltering */
            regAddrDbPtr->egrAndTxqVer0.sourceIDEgrFiltering.ucSrcIDEgrFilteringConfig = 0x028001d4;
            {/*0x2a4000c+m*0x10*/
                GT_U32    m;
                for(m = 0 ; m <= 31 ; m++) {
                    regAddrDbPtr->egrAndTxqVer0.sourceIDEgrFiltering.srcIDEgrFilteringTableEntry[m] =
                        0x2a4000c+m*0x10;
                }/* end of loop m */
            }/*0x2a4000c+m*0x10*/



        }/*end of unit sourceIDEgrFiltering */


        {/*start of unit stackTrafficSeparation */
            regAddrDbPtr->egrAndTxqVer0.stackTrafficSeparation.isStackPortReg = 0x02800098;
            {/*0x2e80000+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 7 ; n++) {
                    regAddrDbPtr->egrAndTxqVer0.stackTrafficSeparation.stackTCRemapTableEntry[n] =
                        0x2e80000+n*0x4;
                }/* end of loop n */
            }/*0x2e80000+n*0x4*/



        }/*end of unit stackTrafficSeparation */


        {/*start of unit statisticalAndCPUTrigEgrMirrToAnalyzerPort */
            regAddrDbPtr->egrAndTxqVer0.statisticalAndCPUTrigEgrMirrToAnalyzerPort.statisticalAndCPUTrigEgrMirrToAnalyzerPortConfig = 0x02800020;

        }/*end of unit statisticalAndCPUTrigEgrMirrToAnalyzerPort */


        {/*start of unit tailDropProfileConfig */
            {/*0x2800030+n*0x4 and 0x2800030+0x90+(n-4)*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->egrAndTxqVer0.tailDropProfileConfig.profilePortTailDropLimitsConfig[n] =
                        0x2800030+n*0x4;
                }/* end of loop n */
                for(n = 4 ; n <= 7 ; n++) {
                    regAddrDbPtr->egrAndTxqVer0.tailDropProfileConfig.profilePortTailDropLimitsConfig[(n - 4) + 4] =
                        0x2800030+0x90+(n-4)*0x4;
                }/* end of loop n */
            }/*0x2800030+n*0x4 and 0x2800030+0x90+(n-4)*0x4*/


            {/*0x2940000+n*0x08+p*0x400*/
                GT_U32    p,n;
                for(p = 0 ; p <= 7 ; p++) {
                    for(n = 0 ; n <= 7 ; n++) {
                        regAddrDbPtr->egrAndTxqVer0.tailDropProfileConfig.profileTCDP0TailDropLimits[p][n] =
                            0x2940000+n*0x08+p*0x400;
                    }/* end of loop n */
                }/* end of loop p */
            }/*0x2940000+n*0x08+p*0x400*/


            {/*0x2940004+n*0x08+p*0x400*/
                GT_U32    p,n;
                for(p = 0 ; p <= 7 ; p++) {
                    for(n = 0 ; n <= 7 ; n++) {
                        regAddrDbPtr->egrAndTxqVer0.tailDropProfileConfig.profileTCDP1TailDropLimits[p][n] =
                            0x2940004+n*0x08+p*0x400;
                    }/* end of loop n */
                }/* end of loop p */
            }/*0x2940004+n*0x08+p*0x400*/



        }/*end of unit tailDropProfileConfig */


        {/*start of unit transmitQueueTxAndEgrMIBCntrs */
            {/*0x2800140+n*0x20*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->egrAndTxqVer0.transmitQueueTxAndEgrMIBCntrs.txqMIBCntrsSetConfig[n] =
                        0x2800140+n*0x20;
                }/* end of loop n */
            }/*0x2800140+n*0x20*/


            {/*0x2b40144+n*0x20*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->egrAndTxqVer0.transmitQueueTxAndEgrMIBCntrs.setOutgoingUcPktCntr[n] =
                        0x2b40144+n*0x20;
                }/* end of loop n */
            }/*0x2b40144+n*0x20*/


            {/*0x2b40148+n*0x20*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->egrAndTxqVer0.transmitQueueTxAndEgrMIBCntrs.setOutgoingMcPktCntr[n] =
                        0x2b40148+n*0x20;
                }/* end of loop n */
            }/*0x2b40148+n*0x20*/


            {/*0x2b4014c+n*0x20*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->egrAndTxqVer0.transmitQueueTxAndEgrMIBCntrs.setOutgoingBcPktCount[n] =
                        0x2b4014c+n*0x20;
                }/* end of loop n */
            }/*0x2b4014c+n*0x20*/


            {/*0x2b40150+n*0x20*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->egrAndTxqVer0.transmitQueueTxAndEgrMIBCntrs.setBridgeEgrFilteredPktCount[n] =
                        0x2b40150+n*0x20;
                }/* end of loop n */
            }/*0x2b40150+n*0x20*/


            {/*0x2b40154+n*0x20*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->egrAndTxqVer0.transmitQueueTxAndEgrMIBCntrs.setTailDroppedPktCntr[n] =
                        0x2b40154+n*0x20;
                }/* end of loop n */
            }/*0x2b40154+n*0x20*/


            {/*0x2b40158+n*0x20*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->egrAndTxqVer0.transmitQueueTxAndEgrMIBCntrs.setCtrlPktCntr[n] =
                        0x2b40158+n*0x20;
                }/* end of loop n */
            }/*0x2b40158+n*0x20*/


            {/*0x2b4015c+n*0x20*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->egrAndTxqVer0.transmitQueueTxAndEgrMIBCntrs.setEgrForwardingRestrictionDroppedPktsCntr[n] =
                        0x2b4015c+n*0x20;
                }/* end of loop n */
            }/*0x2b4015c+n*0x20*/


            {/*0x2b40180+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->egrAndTxqVer0.transmitQueueTxAndEgrMIBCntrs.setMcFIFODroppedPktsCntr[n] =
                        0x2b40180+n*0x4;
                }/* end of loop n */
            }/*0x2b40180+n*0x4*/

        }/*end of unit transmitQueueTxAndEgrMIBCntrs */


        {/*start of unit transmitQueueGlobalConfig */
            regAddrDbPtr->egrAndTxqVer0.transmitQueueGlobalConfig.transmitQueueCtrl = 0x02800000;
            regAddrDbPtr->egrAndTxqVer0.transmitQueueGlobalConfig.transmitQueueExtCtrl = 0x02800004;
            regAddrDbPtr->egrAndTxqVer0.transmitQueueGlobalConfig.transmitQueueExtCtrl1 = 0x028001b0;
            regAddrDbPtr->egrAndTxqVer0.transmitQueueGlobalConfig.transmitQueueResourceSharing = 0x028001f0;

        }/*end of unit transmitQueueGlobalConfig */


        {/*start of unit transmitQueueInterrupt */
            regAddrDbPtr->egrAndTxqVer0.transmitQueueInterrupt.transmitQueueInterruptSummaryCause = 0x02800100;
            regAddrDbPtr->egrAndTxqVer0.transmitQueueInterrupt.transmitQueueInterruptSummaryMask = 0x02800104;
            regAddrDbPtr->egrAndTxqVer0.transmitQueueInterrupt.transmitQueueFlushDoneInterruptCause = 0x02800110;
            regAddrDbPtr->egrAndTxqVer0.transmitQueueInterrupt.transmitQueueFlushInterruptMask = 0x02800114;
            regAddrDbPtr->egrAndTxqVer0.transmitQueueInterrupt.transmitQueueGeneralInterruptCause = 0x02800118;
            regAddrDbPtr->egrAndTxqVer0.transmitQueueInterrupt.transmitQueueGeneralInterruptMask = 0x0280011c;
            regAddrDbPtr->egrAndTxqVer0.transmitQueueInterrupt.egrSTCInterruptCause = 0x02800130;
            regAddrDbPtr->egrAndTxqVer0.transmitQueueInterrupt.egrSTCInterruptMask = 0x02800134;
            regAddrDbPtr->egrAndTxqVer0.transmitQueueInterrupt.transmitQueueDescFullInterruptSummaryCause = 0x0280018c;
            regAddrDbPtr->egrAndTxqVer0.transmitQueueInterrupt.GPPInterruptMaskReg = 0x028001c0;
            regAddrDbPtr->egrAndTxqVer0.transmitQueueInterrupt.GPPInterruptCauseReg = 0x028001cc;
            regAddrDbPtr->egrAndTxqVer0.transmitQueueInterrupt.transmitQueueFullInterruptMask = 0x028001ec;

        }/*end of unit transmitQueueInterrupt */


        {/*start of unit transmitQueueTotalBuffersLimit */
            regAddrDbPtr->egrAndTxqVer0.transmitQueueTotalBuffersLimit.totalBufferLimitConfig = 0x02800084;

        }/*end of unit transmitQueueTotalBuffersLimit */


        {/*start of unit transmitSchedulerProfilesConfig */
            {/*0x2800040+p*0x10 and 0x2800040+0xEC0+(p-4)*0x10*/
                GT_U32    p;
                for(p = 0 ; p <= 3 ; p++) {
                    regAddrDbPtr->egrAndTxqVer0.transmitSchedulerProfilesConfig.profileSDWRRWeightsConfigReg0[p] =
                        0x2800040+p*0x10;
                }/* end of loop p */
                for(p = 4 ; p <= 7 ; p++) {
                    regAddrDbPtr->egrAndTxqVer0.transmitSchedulerProfilesConfig.profileSDWRRWeightsConfigReg0[(p - 4) + 4] =
                        0x2800040+0xEC0+(p-4)*0x10;
                }/* end of loop p */
            }/*0x2800040+p*0x10 and 0x2800040+0xEC0+(p-4)*0x10*/


            {/*0x2800044+p*0x10 and 0x2800044+0xEC0+(p-4)*0x10*/
                GT_U32    p;
                for(p = 0 ; p <= 3 ; p++) {
                    regAddrDbPtr->egrAndTxqVer0.transmitSchedulerProfilesConfig.profileSDWRRWeightsConfigReg1[p] =
                        0x2800044+p*0x10;
                }/* end of loop p */
                for(p = 4 ; p <= 7 ; p++) {
                    regAddrDbPtr->egrAndTxqVer0.transmitSchedulerProfilesConfig.profileSDWRRWeightsConfigReg1[(p - 4) + 4] =
                        0x2800044+0xEC0+(p-4)*0x10;
                }/* end of loop p */
            }/*0x2800044+p*0x10 and 0x2800044+0xEC0+(p-4)*0x10*/


            {/*0x2800048+p*0x10 and 0x2800048+0xEC0+(p-4)*0x10*/
                GT_U32    p;
                for(p = 0 ; p <= 3 ; p++) {
                    regAddrDbPtr->egrAndTxqVer0.transmitSchedulerProfilesConfig.profileSDWRRAndStrictPriorityConfig[p] =
                        0x2800048+p*0x10;
                }/* end of loop p */
                for(p = 4 ; p <= 7 ; p++) {
                    regAddrDbPtr->egrAndTxqVer0.transmitSchedulerProfilesConfig.profileSDWRRAndStrictPriorityConfig[(p - 4) + 4] =
                        0x2800048+0xEC0+(p-4)*0x10;
                }/* end of loop p */
            }/*0x2800048+p*0x10 and 0x2800048+0xEC0+(p-4)*0x10*/



        }/*end of unit transmitSchedulerProfilesConfig */


        {/*start of unit trunksFilteringAndMcDistConfig */
            {/*0x2a40000+n*0x10*/
                GT_U32    n;
                for(n = 0 ; n <= 127 ; n++) {
                    regAddrDbPtr->egrAndTxqVer0.trunksFilteringAndMcDistConfig.trunkNonTrunkMembersTable[n] =
                        0x2a40000+n*0x10;
                }/* end of loop n */
            }/*0x2a40000+n*0x10*/


            {/*0x2A40008+n*0x10*/
                GT_U32    n;
                for(n = 0 ; n <= 7 ; n++) {
                    regAddrDbPtr->egrAndTxqVer0.trunksFilteringAndMcDistConfig.old_designatedTrunkPortEntryTable[n] =
                        0x2A40008+n*0x10;
                }/* end of loop n */
            }/*0x2A40008+n*0x10*/


            {/*0x2e4000c+n*0x10*/
                GT_U32    n;
                for(n = 0 ; n <= 15 ; n++) {
                    regAddrDbPtr->egrAndTxqVer0.trunksFilteringAndMcDistConfig.designatedTrunkPortEntryTable[n] =
                        0x2e4000c+n*0x10;
                }/* end of loop n */
            }/*0x2e4000c+n*0x10*/



        }/*end of unit trunksFilteringAndMcDistConfig */


        {/*start of unit txqCntrs */
            regAddrDbPtr->egrAndTxqVer0.txqCntrs.totalBuffersCntr = 0x02800088;
            regAddrDbPtr->egrAndTxqVer0.txqCntrs.totalDescCntr = 0x0280008c;
            regAddrDbPtr->egrAndTxqVer0.txqCntrs.totalBufferCntr = 0x02800090;
            regAddrDbPtr->egrAndTxqVer0.txqCntrs.totalAllocatedDescsCntr = 0x02800120;
            regAddrDbPtr->egrAndTxqVer0.txqCntrs.mcDescsCntr = 0x02800124;
            regAddrDbPtr->egrAndTxqVer0.txqCntrs.snifferDescsCntr = 0x02800188;
            {/*0x2840000+n*0x200 and 0x2840000+n*0x200*/
                GT_U32    n;
                for(n = 0 ; n <= 27 ; n++) {
                    regAddrDbPtr->egrAndTxqVer0.txqCntrs.portDescsCntr[n] =
                        0x2840000+n*0x200;
                }/* end of loop n */
                for(n = 63 ; n <= 63 ; n++) {
                    regAddrDbPtr->egrAndTxqVer0.txqCntrs.portDescsCntr[(n - 63) + 28] =
                        0x2840000+n*0x200;
                }/* end of loop n */
            }/*0x2840000+n*0x200 and 0x2840000+n*0x200*/


            {/*0x2880000+t*0x8000 + n * 0x200 and 0x2880000+t*0x8000 + n * 0x200*/
                GT_U32    n,t;
                for(n = 0 ; n <= 27 ; n++) {
                    for(t = 0 ; t <= 7 ; t++) {
                        regAddrDbPtr->egrAndTxqVer0.txqCntrs.portTCDescCntrs[n][t] =
                            0x2880000+t*0x8000 + n * 0x200;
                    }/* end of loop t */
                }/* end of loop n */
                for(n = 63 ; n <= 63 ; n++) {
                    for(t = 0 ; t <= 7 ; t++) {
                        regAddrDbPtr->egrAndTxqVer0.txqCntrs.portTCDescCntrs[(n - 63) + 28][t] =
                            0x2880000+t*0x8000 + n * 0x200;
                    }/* end of loop t */
                }/* end of loop n */
            }/*0x2880000+t*0x8000 + n * 0x200 and 0x2880000+t*0x8000 + n * 0x200*/


            {/*0x28c0000+n*0x200 and 0x28c0000+n*0x200*/
                GT_U32    n;
                for(n = 0 ; n <= 27 ; n++) {
                    regAddrDbPtr->egrAndTxqVer0.txqCntrs.portBuffersCntr[n] =
                        0x28c0000+n*0x200;
                }/* end of loop n */
                for(n = 63 ; n <= 63 ; n++) {
                    regAddrDbPtr->egrAndTxqVer0.txqCntrs.portBuffersCntr[(n - 63) + 28] =
                        0x28c0000+n*0x200;
                }/* end of loop n */
            }/*0x28c0000+n*0x200 and 0x28c0000+n*0x200*/


            {/*0x2900000+t*0x8000+n*0x200 and 0x2900000+t*0x8000+n*0x200*/
                GT_U32    n,t;
                for(n = 0 ; n <= 27 ; n++) {
                    for(t = 0 ; t <= 7 ; t++) {
                        regAddrDbPtr->egrAndTxqVer0.txqCntrs.portTCBuffersCntrs[n][t] =
                            0x2900000+t*0x8000+n*0x200;
                    }/* end of loop t */
                }/* end of loop n */
                for(n = 63 ; n <= 63 ; n++) {
                    for(t = 0 ; t <= 7 ; t++) {
                        regAddrDbPtr->egrAndTxqVer0.txqCntrs.portTCBuffersCntrs[(n - 63) + 28][t] =
                            0x2900000+t*0x8000+n*0x200;
                    }/* end of loop t */
                }/* end of loop n */
            }/*0x2900000+t*0x8000+n*0x200 and 0x2900000+t*0x8000+n*0x200*/



        }/*end of unit txqCntrs */


        {/*start of unit VLANAndMcGroupAndSpanStateGroupTables */
            regAddrDbPtr->egrAndTxqVer0.VLANAndMcGroupAndSpanStateGroupTables.globalConfig = 0x03800000;
            regAddrDbPtr->egrAndTxqVer0.VLANAndMcGroupAndSpanStateGroupTables.VLTTablesAccessDataReg[5] = 0x03800100;
            regAddrDbPtr->egrAndTxqVer0.VLANAndMcGroupAndSpanStateGroupTables.VLTTablesAccessDataReg[4] = 0x03800104;
            regAddrDbPtr->egrAndTxqVer0.VLANAndMcGroupAndSpanStateGroupTables.VLTTablesAccessDataReg[3] = 0x03800108;
            regAddrDbPtr->egrAndTxqVer0.VLANAndMcGroupAndSpanStateGroupTables.VLTTablesAccessDataReg[2] = 0x0380010c;
            regAddrDbPtr->egrAndTxqVer0.VLANAndMcGroupAndSpanStateGroupTables.VLTTablesAccessDataReg[1] = 0x03800110;
            regAddrDbPtr->egrAndTxqVer0.VLANAndMcGroupAndSpanStateGroupTables.VLTTablesAccessDataReg[0] = 0x03800114;
            regAddrDbPtr->egrAndTxqVer0.VLANAndMcGroupAndSpanStateGroupTables.VLTTablesAccessCtrl = 0x03800118;

        }/*end of unit VLANAndMcGroupAndSpanStateGroupTables */


        {/*start of unit XGCtrl */
            regAddrDbPtr->egrAndTxqVer0.XGCtrl.XGCtrlReg = 0x02800180;

        }/*end of unit XGCtrl */


        {/*start of unit XSMIConfig */
            regAddrDbPtr->egrAndTxqVer0.XSMIConfig.XSMIManagement = 0x02cc0000;
            regAddrDbPtr->egrAndTxqVer0.XSMIConfig.XSMIAddr = 0x02cc0008;

        }/*end of unit XSMIConfig */

        {/*start of unit br8021Config */

            regAddrDbPtr->egrAndTxqVer0.br8021Config.globalConfig = 0x02800f50;
            regAddrDbPtr->egrAndTxqVer0.br8021Config.dp2cfiEnablePerPortReg = 0x02800f54;

            {/*0x2800f58+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 31 ; n++) {
                    regAddrDbPtr->egrAndTxqVer0.br8021Config.pcidOfTrgPort[n] =
                        0x2800f58+n*0x4;
                }/* end of loop n */

            }/*0x2800f58+n*0x4*/

        }/*end of unit br8021Config */

    }/*end of unit egrAndTxqVer0 - Egress and Txq  */

    {/*start of unit L2I - L2I  */
        {/*start of unit bridgeAccessMatrix */
            {/*0x1000600+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 7 ; n++) {
                    regAddrDbPtr->L2I.bridgeAccessMatrix.bridgeAccessMatrixLine[n] =
                        0x1000600+n*0x4;
                }/* end of loop n */
            }/*0x1000600+n*0x4*/



        }/*end of unit bridgeAccessMatrix */


        {/*start of unit bridgeDropCntrAndSecurityBreachDropCntrs */
            regAddrDbPtr->L2I.bridgeDropCntrAndSecurityBreachDropCntrs.globalSecurityBreachFilterCntr = 0x01040104;
            regAddrDbPtr->L2I.bridgeDropCntrAndSecurityBreachDropCntrs.portVLANSecurityBreachDropCntr = 0x01040108;
            regAddrDbPtr->L2I.bridgeDropCntrAndSecurityBreachDropCntrs.bridgeFilterCntr = 0x01040150;

        }/*end of unit bridgeDropCntrAndSecurityBreachDropCntrs */


        {/*start of unit bridgeEngineConfig */
            regAddrDbPtr->L2I.bridgeEngineConfig.bridgeGlobalConfig[0] = 0x01040000;
            regAddrDbPtr->L2I.bridgeEngineConfig.bridgeGlobalConfig[1] = 0x01040004;
            regAddrDbPtr->L2I.bridgeEngineConfig.bridgeGlobalConfig[2] = 0x0104000c;
            regAddrDbPtr->L2I.bridgeEngineConfig.sourceIDAssignmentModeConfigPorts = 0x01040070;

        }/*end of unit bridgeEngineConfig */


        {/*start of unit bridgeEngineInterrupts */
            regAddrDbPtr->L2I.bridgeEngineInterrupts.bridgeInterruptCause = 0x01040130;
            regAddrDbPtr->L2I.bridgeEngineInterrupts.bridgeInterruptMask = 0x01040134;

        }/*end of unit bridgeEngineInterrupts */


        {/*start of unit ctrlTrafficToCPUConfig */
            {/*0x1000800+n*0x1000*/
                GT_U32    n;
                for(n = 0 ; n <= 15 ; n++) {
                    regAddrDbPtr->L2I.ctrlTrafficToCPUConfig.IEEEReservedMcConfig0[n] =
                        0x1000800+n*0x1000;
                }/* end of loop n */
            }/*0x1000800+n*0x1000*/


            {/*0x1000804+n*0x1000*/
                GT_U32    n;
                for(n = 0 ; n <= 15 ; n++) {
                    regAddrDbPtr->L2I.ctrlTrafficToCPUConfig.IEEEReservedMcCPUIndex[n] =
                        0x1000804+n*0x1000;
                }/* end of loop n */
            }/*0x1000804+n*0x1000*/


            {/*0x1000808+n*0x1000*/
                GT_U32    n;
                for(n = 0 ; n <= 15 ; n++) {
                    regAddrDbPtr->L2I.ctrlTrafficToCPUConfig.IEEEReservedMcConfig1[n] =
                        0x1000808+n*0x1000;
                }/* end of loop n */
            }/*0x1000808+n*0x1000*/


            regAddrDbPtr->L2I.ctrlTrafficToCPUConfig.IEEETableSelect0 = 0x0100080c;
            {/*0x1000810+n*0x1000*/
                GT_U32    n;
                for(n = 0 ; n <= 15 ; n++) {
                    regAddrDbPtr->L2I.ctrlTrafficToCPUConfig.IEEEReservedMcConfig2[n] =
                        0x1000810+n*0x1000;
                }/* end of loop n */
            }/*0x1000810+n*0x1000*/


            {/*0x1000818+n*0x1000*/
                GT_U32    n;
                for(n = 0 ; n <= 15 ; n++) {
                    regAddrDbPtr->L2I.ctrlTrafficToCPUConfig.IEEEReservedMcConfig3[n] =
                        0x1000818+n*0x1000;
                }/* end of loop n */
            }/*0x1000818+n*0x1000*/


            regAddrDbPtr->L2I.ctrlTrafficToCPUConfig.IEEETableSelect1 = 0x0100081c;
            {/*0x1010800+n*0x1000*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->L2I.ctrlTrafficToCPUConfig.IPv6ICMPMsgTypeConfig[n] =
                        0x1010800+n*0x1000;
                }/* end of loop n */
            }/*0x1010800+n*0x1000*/


            regAddrDbPtr->L2I.ctrlTrafficToCPUConfig.IPv6ICMPCommand = 0x01012800;
            {/*0x1020800+n*0x1000*/
                GT_U32    n;
                for(n = 0 ; n <= 7 ; n++) {
                    regAddrDbPtr->L2I.ctrlTrafficToCPUConfig.IPv4McLinkLocalConfig[n] =
                        0x1020800+n*0x1000;
                }/* end of loop n */
            }/*0x1020800+n*0x1000*/


            {/*0x1028800+n*0x1000*/
                GT_U32    n;
                for(n = 0 ; n <= 7 ; n++) {
                    regAddrDbPtr->L2I.ctrlTrafficToCPUConfig.IPv6McLinkLocalConfig[n] =
                        0x1028800+n*0x1000;
                }/* end of loop n */
            }/*0x1028800+n*0x1000*/


            {/*0x1030800+n*0x1000+ t*0x8000*/
                GT_U32    n,t;
                for(n = 0 ; n <= 7 ; n++) {
                    for(t = 0 ; t <= 1 ; t++) {
                        regAddrDbPtr->L2I.ctrlTrafficToCPUConfig.IPv4McLinkLocalCPUCodeIndex[n][t] =
                            0x1030800+n*0x1000+ t*0x8000;
                    }/* end of loop t */
                }/* end of loop n */
            }/*0x1030800+n*0x1000+ t*0x8000*/


            {/*0x1040800+n*0x1000 +t*0x8000*/
                GT_U32    n,t;
                for(n = 0 ; n <= 7 ; n++) {
                    for(t = 0 ; t <= 1 ; t++) {
                        regAddrDbPtr->L2I.ctrlTrafficToCPUConfig.IPv6McLinkLocalCPUCodeIndex[n][t] =
                            0x1040800+n*0x1000 +t*0x8000;
                    }/* end of loop t */
                }/* end of loop n */
            }/*0x1040800+n*0x1000 +t*0x8000*/



        }/*end of unit ctrlTrafficToCPUConfig */


        {/*start of unit ingrLog */
            regAddrDbPtr->L2I.ingrLog.ingrLogEtherType = 0x01040700;
            regAddrDbPtr->L2I.ingrLog.ingrLogDALow = 0x01040704;
            regAddrDbPtr->L2I.ingrLog.ingrLogDAHigh = 0x01040708;
            regAddrDbPtr->L2I.ingrLog.ingrLogDALowMask = 0x0104070c;
            regAddrDbPtr->L2I.ingrLog.ingrLogDAHighMask = 0x01040710;
            regAddrDbPtr->L2I.ingrLog.ingrLogSALow = 0x01040714;
            regAddrDbPtr->L2I.ingrLog.ingrLogSAHigh = 0x01040718;
            regAddrDbPtr->L2I.ingrLog.ingrLogSALowMask = 0x0104071c;
            regAddrDbPtr->L2I.ingrLog.ingrLogSAHighMask = 0x01040720;
            regAddrDbPtr->L2I.ingrLog.ingrLogMacDAResult = 0x01040724;
            regAddrDbPtr->L2I.ingrLog.ingrLogMacSAResult = 0x01040728;

        }/*end of unit ingrLog */


        {/*start of unit ingrPortsBridgeConfig */
            {/*0x1000000+n*0x1000 and 0x1000000+n*0x1000*/
                GT_U32    n;
                for(n = 0 ; n <= 27 ; n++) {
                    regAddrDbPtr->L2I.ingrPortsBridgeConfig.ingrPortBridgeConfig0[n] =
                        0x1000000+n*0x1000;
                }/* end of loop n */
                for(n = 63 ; n <= 63 ; n++) {
                    regAddrDbPtr->L2I.ingrPortsBridgeConfig.ingrPortBridgeConfig0[(n - 63) + 28] =
                        0x1000000+n*0x1000;
                }/* end of loop n */
            }/*0x1000000+n*0x1000 and 0x1000000+n*0x1000*/


            {/*0x1000010+n*0x1000 and 0x1000010+n*0x1000*/
                GT_U32    n;
                for(n = 0 ; n <= 27 ; n++) {
                    regAddrDbPtr->L2I.ingrPortsBridgeConfig.ingrPortBridgeConfig1[n] =
                        0x1000010+n*0x1000;
                }/* end of loop n */
                for(n = 63 ; n <= 63 ; n++) {
                    regAddrDbPtr->L2I.ingrPortsBridgeConfig.ingrPortBridgeConfig1[(n - 63) + 28] =
                        0x1000010+n*0x1000;
                }/* end of loop n */
            }/*0x1000010+n*0x1000 and 0x1000010+n*0x1000*/



        }/*end of unit ingrPortsBridgeConfig */


        {/*start of unit ingrPortsRateLimitConfig */
            {/*0x1000400+0x1000*n*/
                GT_U32    n;
                for(n = 0 ; n <= 27 ; n++) {
                    regAddrDbPtr->L2I.ingrPortsRateLimitConfig.portRateLimitCntr[n] =
                        0x1000400+0x1000*n;
                }/* end of loop n */
            }/*0x1000400+0x1000*n*/


            regAddrDbPtr->L2I.ingrPortsRateLimitConfig.ingrRateLimitConfig0 = 0x01040140;
            regAddrDbPtr->L2I.ingrPortsRateLimitConfig.ingrRateLimitConfig1 = 0x01040144;
            regAddrDbPtr->L2I.ingrPortsRateLimitConfig.ingrRateLimitDropCntr310 = 0x01040148;
            regAddrDbPtr->L2I.ingrPortsRateLimitConfig.ingrRateLimitDropCntr3932 = 0x0104014c;

        }/*end of unit ingrPortsRateLimitConfig */


        {/*start of unit IPv6McBridgingBytesSelection */
            regAddrDbPtr->L2I.IPv6McBridgingBytesSelection.IPv6MCBridgingBytesSelectionConfig = 0x01040010;

        }/*end of unit IPv6McBridgingBytesSelection */


        {/*start of unit layer2BridgeMIBCntrs */
            regAddrDbPtr->L2I.layer2BridgeMIBCntrs.MACAddrCount[0] = 0x010400b0;
            regAddrDbPtr->L2I.layer2BridgeMIBCntrs.MACAddrCount[1] = 0x010400b4;
            regAddrDbPtr->L2I.layer2BridgeMIBCntrs.MACAddrCount[2] = 0x010400b8;
            regAddrDbPtr->L2I.layer2BridgeMIBCntrs.hostIncomingPktsCount = 0x010400bc;
            regAddrDbPtr->L2I.layer2BridgeMIBCntrs.hostOutgoingPktsCount = 0x010400c0;
            regAddrDbPtr->L2I.layer2BridgeMIBCntrs.hostOutgoingMcPktCount = 0x010400cc;
            regAddrDbPtr->L2I.layer2BridgeMIBCntrs.hostOutgoingBcPktCount = 0x010400d0;
            regAddrDbPtr->L2I.layer2BridgeMIBCntrs.matrixSourceDestinationPktCount = 0x010400d4;
            {/*0x10400dc+0x14*n*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->L2I.layer2BridgeMIBCntrs.cntrsSetConfig[n] =
                        0x10400dc+0x14*n;
                }/* end of loop n */
            }/*0x10400dc+0x14*n*/


            {/*0x10400e0+n*0x14*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->L2I.layer2BridgeMIBCntrs.setIncomingPktCount[n] =
                        0x10400e0+n*0x14;
                }/* end of loop n */
            }/*0x10400e0+n*0x14*/


            {/*0x10400e4+n*0x14*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->L2I.layer2BridgeMIBCntrs.setVLANIngrFilteredPktCount[n] =
                        0x10400e4+n*0x14;
                }/* end of loop n */
            }/*0x10400e4+n*0x14*/


            {/*0x10400e8+n*0x14*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->L2I.layer2BridgeMIBCntrs.setSecurityFilteredPktCount[n] =
                        0x10400e8+n*0x14;
                }/* end of loop n */
            }/*0x10400e8+n*0x14*/


            {/*0x10400ec+n*0x14*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->L2I.layer2BridgeMIBCntrs.setBridgeFilteredPktCount[n] =
                        0x10400ec+n*0x14;
                }/* end of loop n */
            }/*0x10400ec+n*0x14*/



        }/*end of unit layer2BridgeMIBCntrs */


        {/*start of unit MACBasedQoSTable */
            {/*0x1000200+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 6 ; n++) {
                    regAddrDbPtr->L2I.MACBasedQoSTable.MACQoSTableEntry[n] =
                        0x1000200+n*0x4;
                }/* end of loop n */
            }/*0x1000200+n*0x4*/



        }/*end of unit MACBasedQoSTable */


        {/*start of unit securityBreachStatus */
            regAddrDbPtr->L2I.securityBreachStatus.securityBreachStatus[0] = 0x010401a0;
            regAddrDbPtr->L2I.securityBreachStatus.securityBreachStatus[1] = 0x010401a4;
            regAddrDbPtr->L2I.securityBreachStatus.securityBreachStatus[2] = 0x010401a8;

        }/*end of unit securityBreachStatus */


        {/*start of unit UDPBcMirrorTrapUDPRelayConfig */
            {/*0x1000500+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 11 ; n++) {
                    regAddrDbPtr->L2I.UDPBcMirrorTrapUDPRelayConfig.UDPBcDestinationPortConfigTable[n] =
                        0x1000500+n*0x4;
                }/* end of loop n */
            }/*0x1000500+n*0x4*/



        }/*end of unit UDPBcMirrorTrapUDPRelayConfig */


        {/*start of unit VLANMRUProfilesConfig */
            {/*0x1000300+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->L2I.VLANMRUProfilesConfig.VLANMRUProfileConfig[n] =
                        0x1000300+n*0x4;
                }/* end of loop n */
            }/*0x1000300+n*0x4*/



        }/*end of unit VLANMRUProfilesConfig */


        {/*start of unit VLANRangesConfig */
            regAddrDbPtr->L2I.VLANRangesConfig.ingrVLANRangeConfig = 0x010400a8;

        }/*end of unit VLANRangesConfig */

    }/*end of unit L2I - L2I  */


    {/*start of unit ucRouter - Unicast Router  */
        regAddrDbPtr->ucRouter.ucRoutingEngineConfigReg = 0x04000e3c;
    }/*end of unit ucRouter - Unicast Router  */

    {/*start of unit IPVX - IPVX  */

            {/*0x04000d00+n*4*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->IPvX.IPLog.IPHitLogDIPAddrReg[n] =
                        0x04000d00+n*4;
                }/* end of loop n */
            }/*0x04000d00+n*4*/


            {/*0x04000d10+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->IPvX.IPLog.IPHitLogDIPMaskAddrReg[n] =
                        0x04000d10+n*0x4;
                }/* end of loop n */
            }/*0x04000d10+n*0x4*/


            {/*0x04000d20+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->IPvX.IPLog.IPHitLogSIPAddrReg[n] =
                        0x04000d20+n*0x4;
                }/* end of loop n */
            }/*0x04000d20+n*0x4*/


            {/*0x04000d30+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->IPvX.IPLog.IPHitLogSIPMaskAddrReg[n] =
                        0x04000d30+n*0x4;
                }/* end of loop n */
            }/*0x04000d30+n*0x4*/


            regAddrDbPtr->IPvX.IPLog.IPHitLogLayer4DestinationPortReg = 0x04000d40;
            regAddrDbPtr->IPvX.IPLog.IPHitLogLayer4SourcePortReg = 0x04000d44;
            regAddrDbPtr->IPvX.IPLog.IPHitLogProtocolReg = 0x04000d48;
            {/*0x04000d50+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 2 ; n++) {
                    regAddrDbPtr->IPvX.IPLog.IPHitLogDestinationIPNHEEntryRegWord[n] =
                        0x04000d50+n*0x4;
                }/* end of loop n */
            }/*0x04000d50+n*0x4*/


            {/*0x04000d5c+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 2 ; n++) {
                    regAddrDbPtr->IPvX.IPLog.IPHitLogSourceIPNHEEntryRegWord[n] =
                        0x04000d5c+n*0x4;
                }/* end of loop n */
            }/*0x04000d5c+n*0x4*/



            regAddrDbPtr->IPvX.IPv4GlobalCtrl.IPv4CtrlReg0 = 0x04000100;
            regAddrDbPtr->IPvX.IPv4GlobalCtrl.IPv4CtrlReg1 = 0x04000104;
            regAddrDbPtr->IPvX.IPv4GlobalCtrl.IPv4UcEnableCtrl = 0x04000108;
            regAddrDbPtr->IPvX.IPv4GlobalCtrl.IPv4McEnableCtrl = 0x0400010c;

            regAddrDbPtr->IPvX.IPv6GlobalCtrl.IPv6CtrlReg0 = 0x04000200;
            regAddrDbPtr->IPvX.IPv6GlobalCtrl.IPv6CtrlReg1 = 0x04000204;
            regAddrDbPtr->IPvX.IPv6GlobalCtrl.IPv6UcEnableCtrl = 0x04000208;
            regAddrDbPtr->IPvX.IPv6GlobalCtrl.IPv6McEnableCtrl = 0x0400020c;

            {/*0x04000250+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->IPvX.IPv6Scope.IPv6UcScopePrefix[n] =
                        0x04000250+n*0x4;
                }/* end of loop n */
            }/*0x04000250+n*0x4*/


            {/*0x04000260+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->IPvX.IPv6Scope.IPv6UcScopeLevel[n] =
                        0x04000260+n*0x4;
                }/* end of loop n */
            }/*0x04000260+n*0x4*/


            {/*0x04000278+n*4*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->IPvX.IPv6Scope.IPv6UcScopeTableReg[n] =
                        0x04000278+n*4;
                }/* end of loop n */
            }/*0x04000278+n*4*/


            {/*0x04000288+n*4*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->IPvX.IPv6Scope.IPv6McScopeTableReg[n] =
                        0x04000288+n*4;
                }/* end of loop n */
            }/*0x04000288+n*4*/



            {/*0x04000300+n*4*/
                GT_U32    n;
                for(n = 0 ; n <= 15 ; n++) {
                    regAddrDbPtr->IPvX.qosRoutingConfig.qoSProfileToRouteBlockOffsetEntry[n] =
                        0x04000300+n*4;
                }/* end of loop n */
            }/*0x04000300+n*4*/



            {/*0x04000440+0x4*n*/
                GT_U32    n;
                for(n = 0 ; n <= 7 ; n++) {
                    regAddrDbPtr->IPvX.routerAccessMatrix.routerAccessMatrixLine[n] =
                        0x04000440+0x4*n;
                }/* end of loop n */
            }/*0x04000440+0x4*n*/



            regAddrDbPtr->IPvX.routerBridgedPktsExceptionCntr.routerBridgedPktExceptionsCntr = 0x04000940;

            regAddrDbPtr->IPvX.routerDropCntr.routerDropCntr = 0x04000950;
            regAddrDbPtr->IPvX.routerDropCntr.routerDropCntrConfig = 0x04000954;

            regAddrDbPtr->IPvX.routerGlobalCtrl.routerGlobalCtrl0 = 0x04000000;
            {/*0x04000004+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->IPvX.routerGlobalCtrl.routerMTUConfigReg[n] =
                        0x04000004+n*0x4;
                }/* end of loop n */
            }/*0x04000004+n*0x4*/

            regAddrDbPtr->IPvX.routerGlobalCtrl.routerAdditionalCtrl = 0x04000964;

            {/*0x04000900+0x100*n*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->IPvX.routerManagementCntrsSets.routerManagementInUcPktsCntrSet[n] =
                        0x04000900+0x100*n;
                }/* end of loop n */
            }/*0x04000900+0x100*n*/


            {/*0x04000904+0x100*n*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->IPvX.routerManagementCntrsSets.routerManagementInMcPktsCntrSet[n] =
                        0x04000904+0x100*n;
                }/* end of loop n */
            }/*0x04000904+0x100*n*/


            {/*0x04000908+0x100*n*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->IPvX.routerManagementCntrsSets.routerManagementInUcNonRoutedNonException[n] =
                        0x04000908+0x100*n;
                }/* end of loop n */
            }/*0x04000908+0x100*n*/


            {/*0x0400090c+0x100*n*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->IPvX.routerManagementCntrsSets.routerManagementInUcNonRoutedExceptionPktsCntrSet[n] =
                        0x0400090c+0x100*n;
                }/* end of loop n */
            }/*0x0400090c+0x100*n*/


            {/*0x04000910+0x100*n*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->IPvX.routerManagementCntrsSets.routerManagementInMcNonRoutedNonExceptionPktsCntrSet[n] =
                        0x04000910+0x100*n;
                }/* end of loop n */
            }/*0x04000910+0x100*n*/


            {/*0x04000914+0x100*n*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->IPvX.routerManagementCntrsSets.routerManagementInMcNonRoutedExceptionPktsCntrSet[n] =
                        0x04000914+0x100*n;
                }/* end of loop n */
            }/*0x04000914+0x100*n*/


            {/*0x04000918+0x100*n*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->IPvX.routerManagementCntrsSets.routerManagementInUcTrappedMirroredPktCntrSet[n] =
                        0x04000918+0x100*n;
                }/* end of loop n */
            }/*0x04000918+0x100*n*/


            {/*0x0400091c+0x100*n*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->IPvX.routerManagementCntrsSets.routerManagementInMcTrappedMirroredPktCntrSet[n] =
                        0x0400091c+0x100*n;
                }/* end of loop n */
            }/*0x0400091c+0x100*n*/


            {/*0x04000920+0x100*n*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->IPvX.routerManagementCntrsSets.routerManagementInMcRPFFailCntrSet[n] =
                        0x04000920+0x100*n;
                }/* end of loop n */
            }/*0x04000920+0x100*n*/


            {/*0x04000924+0x100*n*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->IPvX.routerManagementCntrsSets.routerManagementOutUcPktCntrSet[n] =
                        0x04000924+0x100*n;
                }/* end of loop n */
            }/*0x04000924+0x100*n*/


            regAddrDbPtr->IPvX.routerManagementCntrsSets.routerManagementIncomingPktCntr = 0x04000944;
            regAddrDbPtr->IPvX.routerManagementCntrsSets.routerManagementOutgoingPktCntr = 0x04000948;
            regAddrDbPtr->IPvX.routerManagementCntrsSets.routerManagementInSipFilterCntr = 0x04000958;
            {/*0x04000980+0x100*n*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->IPvX.routerManagementCntrsSets.routerManagementCntrsSetConfig[n] =
                        0x04000980+0x100*n;
                }/* end of loop n */
            }/*0x04000980+0x100*n*/


    }/*end of unit IPVX - IPVX  */

    {/*start of unit BM - BM  */
        {/*start of unit BMRegs */
            regAddrDbPtr->BM.BMRegs.bufferManagementInterruptCause0 = 0x05000040;
            regAddrDbPtr->BM.BMRegs.bufferManagementInterruptMask0 = 0x05000044;
            regAddrDbPtr->BM.BMRegs.bufferManagementInterruptCause1 = 0x05000048;
            regAddrDbPtr->BM.BMRegs.bufferManagementInterruptMask1 = 0x0500004c;

            regAddrDbPtr->BM.BMRegs.bufferManagementNumberOfAgedBuffers = 0x05000054;
            regAddrDbPtr->BM.BMRegs.totalBuffersAllocationCntr = 0x05000058;
            regAddrDbPtr->BM.BMRegs.bufferManagementPktCntr = 0x0500005c;
            regAddrDbPtr->BM.BMRegs.networkPortsGroupAllocationCntr = 0x05000080;
            regAddrDbPtr->BM.BMRegs.hyperGStackPortsGroupAllocationCntr = 0x05000084;
            {/*0x5000100+n * 0x4 and 0x5000100+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 27 ; n++) {
                    regAddrDbPtr->BM.BMRegs.portBuffersAllocationCntr[n] =
                        0x5000100+n * 0x4;
                }/* end of loop n */
                for(n = 31 ; n <= 31 ; n++) {
                    regAddrDbPtr->BM.BMRegs.portBuffersAllocationCntr[(n - 31) + 28] =
                        0x5000100+n*0x4;
                }/* end of loop n */
            }/*0x5000100+n * 0x4 and 0x5000100+n*0x4*/



            regAddrDbPtr->BM.BMRegs.bufferManagementAgingConfig = 0x0500000c;

            regAddrDbPtr->BM.BMRegs.bufferManagementGlobalBuffersLimitsConfig = 0x05000000;
            regAddrDbPtr->BM.BMRegs.bufferManagementNetworkPortGroupLimitsConfig = 0x05000004;
            regAddrDbPtr->BM.BMRegs.bufferManagementHyperGStackPortsGroupLimits = 0x05000008;
            regAddrDbPtr->BM.BMRegs.bufferManagementBuffersConfig = 0x05000014;

            regAddrDbPtr->BM.BMRegs.portsBuffersLimitProfileConfig[0] = 0x05000020;
            regAddrDbPtr->BM.BMRegs.portsBuffersLimitProfileConfig[1] = 0x05000024;
            regAddrDbPtr->BM.BMRegs.portsBuffersLimitProfileConfig[2] = 0x05000028;
            regAddrDbPtr->BM.BMRegs.portsBuffersLimitProfileConfig[3] = 0x0500002c;
            regAddrDbPtr->BM.BMRegs.buffersLimitsProfileAssociation0 = 0x05000030;
            regAddrDbPtr->BM.BMRegs.buffersLimitsProfileAssociation1 = 0x05000034;

        }/*end of unit BMRegs */

    }/*end of unit BM - BM  */

    {/*start of unit LMS - LMS  */
        {/*start of unit LMS1LMS1Group0 */
            regAddrDbPtr->LMS.LMS1LMS1Group0.LEDInterface1CtrlReg1AndClass6ManipulationforPorts12Through23 = 0x0a004100;
            regAddrDbPtr->LMS.LMS1LMS1Group0.LEDInterface1Group23ConfigRegforPorts12Through23 = 0x0a004104;
            regAddrDbPtr->LMS.LMS1LMS1Group0.LEDInterface1Class23ManipulationRegforPorts12Through23 = 0x0a004108;
            regAddrDbPtr->LMS.LMS1LMS1Group0.LEDInterface1Class5ManipulationRegforPorts12Through23 = 0x0a00410c;
            regAddrDbPtr->LMS.LMS1LMS1Group0.stackLEDInterface1Class1011ManipulationRegforStackPort = 0x0a005100;
            regAddrDbPtr->LMS.LMS1LMS1Group0.stackLEDInterface1Group01ConfigRegforStackPort = 0x0a005104;

            regAddrDbPtr->LMS.LMS1LMS1Group0.MIBCntrsCtrlReg0forPorts18Through23 = 0x0a004020;

            regAddrDbPtr->LMS.LMS1LMS1Group0.portsGOP3AndStackMIBsInterruptCause = 0x0a004000;
            regAddrDbPtr->LMS.LMS1LMS1Group0.portsGOP3AndStackMIBsInterruptMask = 0x0a004004;

            regAddrDbPtr->LMS.LMS1LMS1Group0.flowCtrlAdvertiseForFiberMediaSelectedConfigReg1forPorts12Through23 = 0x0a004024;
            regAddrDbPtr->LMS.LMS1LMS1Group0.PHYAddrReg3forPorts18Through23 = 0x0a004030;
            regAddrDbPtr->LMS.LMS1LMS1Group0.PHYAutoNegConfigReg3 = 0x0a004034;

        }/*end of unit LMS1LMS1Group0 */


        {/*start of unit LMS1 */
            regAddrDbPtr->LMS.LMS1.LEDInterface1CtrlReg0forPorts12Through23AndStackPort = 0x09004100;
            regAddrDbPtr->LMS.LMS1.LEDInterface1Group01ConfigRegforPorts12Through23 = 0x09004104;
            regAddrDbPtr->LMS.LMS1.LEDInterface1Class01ManipulationRegforPorts12Through23 = 0x09004108;
            regAddrDbPtr->LMS.LMS1.LEDInterface1Class4ManipulationRegforPorts12Through23 = 0x0900410c;
            regAddrDbPtr->LMS.LMS1.stackLEDInterface1Class04ManipulationRegforStackPorts = 0x09005100;
            regAddrDbPtr->LMS.LMS1.stackLEDInterface1Class59ManipulationRegforStackPorts = 0x09005104;
            regAddrDbPtr->LMS.LMS1.LEDInterface1FlexlinkPortsDebugSelectRegforStackPort = 0x09005110;
            regAddrDbPtr->LMS.LMS1.LEDInterface1FlexlinkPortsDebugSelectReg1forStackPort = 0x09005114;

            regAddrDbPtr->LMS.LMS1.portsInterruptSummary = 0x09004010;

            regAddrDbPtr->LMS.LMS1.MIBCntrsCtrlReg0forPorts12Through17 = 0x09004020;

            regAddrDbPtr->LMS.LMS1.portsGOP2MIBsInterruptCause = 0x09004000;
            regAddrDbPtr->LMS.LMS1.portsGOP2MIBsInterruptMask = 0x09004004;

            regAddrDbPtr->LMS.LMS1.PHYAddrReg2forPorts12Through17 = 0x09004030;
            regAddrDbPtr->LMS.LMS1.PHYAutoNegConfigReg2 = 0x09004034;
            regAddrDbPtr->LMS.LMS1.SMI1Management = 0x09004054;
            regAddrDbPtr->LMS.LMS1.LMS1MiscConfigs = 0x09004200;

        }/*end of unit LMS1 */


        {/*start of unit LMS0LMS0Group0 */
            regAddrDbPtr->LMS.LMS0LMS0Group0.LEDInterface0CtrlReg1AndClass6ManipulationRegforPorts0Through11 = 0x08004100;
            regAddrDbPtr->LMS.LMS0LMS0Group0.LEDInterface0Group23ConfigRegforPorts0Through11 = 0x08004104;
            regAddrDbPtr->LMS.LMS0LMS0Group0.LEDInterface0Class23ManipulationRegforPorts0Through11 = 0x08004108;
            regAddrDbPtr->LMS.LMS0LMS0Group0.LEDInterface0Class5ManipulationRegforPorts0Through11 = 0x0800410c;
            regAddrDbPtr->LMS.LMS0LMS0Group0.stackLEDInterface0Class1011ManipulationRegforStackPorts = 0x08005100;
            regAddrDbPtr->LMS.LMS0LMS0Group0.stackLEDInterface0Group01ConfigRegforStackPort = 0x08005104;

            regAddrDbPtr->LMS.LMS0LMS0Group0.portsMIBCntrsInterruptSummaryMask = 0x08005110;
            regAddrDbPtr->LMS.LMS0LMS0Group0.portsInterruptSummaryMask = 0x08005114;

            regAddrDbPtr->LMS.LMS0LMS0Group0.MIBCntrsCtrlReg0forPorts6Through11 = 0x08004020;

            regAddrDbPtr->LMS.LMS0LMS0Group0.triSpeedPortsGOP1MIBsInterruptCause = 0x08004000;
            regAddrDbPtr->LMS.LMS0LMS0Group0.triSpeedPortsGOP1MIBsInterruptMask = 0x08004004;

            regAddrDbPtr->LMS.LMS0LMS0Group0.flowCtrlAdvertiseForFiberMediaSelectedConfigReg0forPorts0Through11 = 0x08004024;
            regAddrDbPtr->LMS.LMS0LMS0Group0.PHYAddrReg1forPorts6Through11 = 0x08004030;
            regAddrDbPtr->LMS.LMS0LMS0Group0.PHYAutoNegConfigReg1 = 0x08004034;

        }/*end of unit LMS0LMS0Group0 */


        {/*start of unit LMS0 */
            regAddrDbPtr->LMS.LMS0.LEDInterface0CtrlReg0forPorts0Through11AndStackPort = 0x07004100;
            regAddrDbPtr->LMS.LMS0.LEDInterface0Group01ConfigRegforPorts0Through11 = 0x07004104;
            regAddrDbPtr->LMS.LMS0.LEDInterface0Class01ManipulationRegforPorts0Through11 = 0x07004108;
            regAddrDbPtr->LMS.LMS0.LEDInterface0Class4ManipulationRegforPorts0Through11 = 0x0700410c;
            regAddrDbPtr->LMS.LMS0.stackLEDInterface0Class04ManipulationRegforStackPorts = 0x07005100;
            regAddrDbPtr->LMS.LMS0.stackLEDInterface0Class59ManipulationRegforStackPorts = 0x07005104;
            regAddrDbPtr->LMS.LMS0.LEDInterface0FlexlinkPortsDebugSelectRegforStackPort = 0x07005110;
            regAddrDbPtr->LMS.LMS0.LEDInterface0FlexlinkPortsDebugSelectReg1forStackPort = 0x07005114;

            regAddrDbPtr->LMS.LMS0.MIBCntrsInterruptSummary = 0x07004010;

            regAddrDbPtr->LMS.LMS0.MIBCntrsCtrlReg0forPorts0Through5 = 0x07004020;

            regAddrDbPtr->LMS.LMS0.sourceAddrMiddle = 0x07004024;
            regAddrDbPtr->LMS.LMS0.sourceAddrHigh = 0x07004028;

            regAddrDbPtr->LMS.LMS0.portsGOP0MIBsInterruptCause = 0x07004000;
            regAddrDbPtr->LMS.LMS0.portsGOP0MIBsInterruptMask = 0x07004004;

            regAddrDbPtr->LMS.LMS0.PHYAddrReg0forPorts0Through5 = 0x07004030;
            regAddrDbPtr->LMS.LMS0.PHYAutoNegConfigReg0 = 0x07004034;
            regAddrDbPtr->LMS.LMS0.SMI0Management = 0x07004054;
            regAddrDbPtr->LMS.LMS0.LMS0MiscConfigs = 0x07004200;

        }/*end of unit LMS0 */


        regAddrDbPtr->LMS.stackPortsModeReg = 0x08004144;
    }/*end of unit LMS - LMS  */

    {/*start of unit FDB - FDB  */
        {/*start of unit deviceTable */
            regAddrDbPtr->FDB.deviceTable.deviceTable = 0x0b000068;

        }/*end of unit deviceTable */


        {/*start of unit FDBAction */
            regAddrDbPtr->FDB.FDBAction.FDBAction[0] = 0x0b000004;
            regAddrDbPtr->FDB.FDBAction.FDBAction[1] = 0x0b000008;
            regAddrDbPtr->FDB.FDBAction.FDBAction[2] = 0x0b000020;

        }/*end of unit FDBAction */


        {/*start of unit FDBAddrUpdateMsgs */
            regAddrDbPtr->FDB.FDBAddrUpdateMsgs.AUMsgToCPU = 0x0b000034;
            regAddrDbPtr->FDB.FDBAddrUpdateMsgs.AUFIFOToCPUConfig = 0x0b000038;
            {/*0xb000040+n*4*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->FDB.FDBAddrUpdateMsgs.AUMsgFromCPUWord[n] =
                        0xb000040+n*4;
                }/* end of loop n */
            }/*0xb000040+n*4*/


            regAddrDbPtr->FDB.FDBAddrUpdateMsgs.AUMsgFromCPUCtrl = 0x0b000050;

        }/*end of unit FDBAddrUpdateMsgs */


        {/*start of unit FDBDirectAccess */
            {/*0xb000054+n*4*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->FDB.FDBDirectAccess.FDBDirectAccessData[n] =
                        0xb000054+n*4;
                }/* end of loop n */
            }/*0xb000054+n*4*/


            regAddrDbPtr->FDB.FDBDirectAccess.FDBDirectAccessCtrl = 0x0b000064;

        }/*end of unit FDBDirectAccess */


        {/*start of unit FDBGlobalConfig */
            regAddrDbPtr->FDB.FDBGlobalConfig.FDBGlobalConfig = 0x0b000000;
            regAddrDbPtr->FDB.FDBGlobalConfig.bridgeAccessLevelConfig = 0x0b00003c;

        }/*end of unit FDBGlobalConfig */


        {/*start of unit FDBInternal */
            regAddrDbPtr->FDB.FDBInternal.metalFix = 0x0b00006c;
            regAddrDbPtr->FDB.FDBInternal.FDBIdleState = 0x0b000070;

        }/*end of unit FDBInternal */


        {/*start of unit FDBInterrupt */
            regAddrDbPtr->FDB.FDBInterrupt.FDBInterruptCauseReg = 0x0b000018;
            regAddrDbPtr->FDB.FDBInterrupt.FDBInterruptMaskReg = 0x0b00001c;

        }/*end of unit FDBInterrupt */


        {/*start of unit sourceAddrNotLearntCntr */
            regAddrDbPtr->FDB.sourceAddrNotLearntCntr.learnedEntryDiscardsCount = 0x0b000030;

        }/*end of unit sourceAddrNotLearntCntr */

    }/*end of unit FDB - FDB  */

    {/*start of unit EPCL - EPCL  */

        regAddrDbPtr->EPCL.egrPolicyGlobalConfig = 0x1d000000;
        regAddrDbPtr->EPCL.egrPolicyConfigTableAccessModeConfig[0] = 0x1d000004;
        regAddrDbPtr->EPCL.egrPolicyNonTSDataPktsConfig[0]         = 0x1d000008;
        regAddrDbPtr->EPCL.egrPolicyTSDataPktsConfig[0]            = 0x1d00000c;
        regAddrDbPtr->EPCL.egrPolicyToCpuConfig[0]                 = 0x1d000010;
        regAddrDbPtr->EPCL.egrPolicyFromCpuDataConfig[0]           = 0x1d000014;
        regAddrDbPtr->EPCL.egrPolicyFromCpuCtrlConfig[0]           = 0x1d000018;
        regAddrDbPtr->EPCL.egrPolicyToAnalyzerConfig[0]            = 0x1d00001c;

        regAddrDbPtr->EPCL.egrPolicyDIPSolicitationData[0] = 0x1d000020;
        regAddrDbPtr->EPCL.egrPolicyDIPSolicitationData[1] = 0x1d000024;
        regAddrDbPtr->EPCL.egrPolicyDIPSolicitationData[2] = 0x1d000028;
        regAddrDbPtr->EPCL.egrPolicyDIPSolicitationData[3] = 0x1d00002c;

        {/*0x1d000030+n*4*/
            GT_U32    n;
            for(n = 0 ; n <= 3 ; n++) {
                regAddrDbPtr->EPCL.egrPolicyDIPSolicitationMask[n] =
                    0x1d000030+n*4;
            }/* end of loop n */
        }/*0x1d000030+n*4*/

        {/*0x1d000100+n*0x4*/
            GT_U32    n;
            for(n = 0 ; n <= 7 ; n++) {
                regAddrDbPtr->EPCL.egrTCPPortRangeComparatorConfig[n] =
                    0x1d000100+n*0x4;
            }/* end of loop n */
        }/*0x1d000100+n*0x4*/

        {/*0x1d000140+n*0x4*/
            GT_U32    n;
            for(n = 0 ; n <= 7 ; n++) {
                regAddrDbPtr->EPCL.egrUDPPortRangeComparatorConfig[n] =
                    0x1d000140+n*0x4;
            }/* end of loop n */
        }/*0x1d000140+n*0x4*/

        regAddrDbPtr->EPCL.egrPolicerGlobalConfig = 0x1d000200;

    }/*end of unit EPCL - EPCL  */

    {/*start of unit HA - HA  */

        {/*0x1c000004+w*4*/
            GT_U32    w;
            for(w = 0 ; w <= 0 ; w++) {
                regAddrDbPtr->HA.cascadingAndHeaderInsertionConfig[w] =
                    0x1c000004+w*4;
            }/* end of loop w */
        }/*0x1c000004+w*4*/


        {/*0x1c000020+w*4*/
            GT_U32    w;
            for(w = 0 ; w <= 0 ; w++) {
                regAddrDbPtr->HA.egrDSATagTypeConfig[w] =
                    0x1c000020+w*4;
            }/* end of loop w */
        }/*0x1c000020+w*4*/


        {/*0x1c00002c+n*0x4*/
            GT_U32    n;
            for(n = 0 ; n <= 6 ; n++) {
                regAddrDbPtr->HA.portTrunkNumConfigReg[n] =
                    0x1c00002c+n*0x4;
            }/* end of loop n */
        }/*0x1c00002c+n*0x4*/

        regAddrDbPtr->HA.DP2CFITable = 0x1c000050;
        {/*0x1c000060+4*w*/
            GT_U32    w;
            for(w = 0 ; w <= 0 ; w++) {
                regAddrDbPtr->HA.mirrorToAnalyzerHeaderConfig[w] =
                    0x1c000060+4*w;
            }/* end of loop w */
        }/*0x1c000060+4*w*/


        regAddrDbPtr->HA.haGlobalConfig = 0x1c000100;

        {
            GT_U32    w;
            for(w = 0 ; w <= 0 ; w++) {
                regAddrDbPtr->HA.routerHaEnableMACSAModification[w] =
                    0x1c000104+4*w;
            }/* end of loop w */
        }/*0x1c000104+4*w*/


        regAddrDbPtr->HA.routerMACSABaseReg0 = 0x1c000108;
        regAddrDbPtr->HA.routerMACSABaseReg1 = 0x1c00010c;

        {/*0x1c000110+4*w*/
            GT_U32    w;
            for(w = 0 ; w <= 0 ; w++) {
                regAddrDbPtr->HA.deviceIDModificationEnable[w] =
                    0x1c000110+4*w;
            }/* end of loop w */
        }/*0x1c000110+4*w*/


        {/*0x1c000120+k*0x4*/
            GT_U32    k;
            for(k = 0 ; k <= 1 ; k++) {
                regAddrDbPtr->HA.routerHaMACSAModificationMode[k] =
                    0x1c000120+k*0x4;
            }/* end of loop k */
        }/*0x1c000120+k*0x4*/


        {/*0x1c000130+4*w*/
            GT_U32    w;
            for(w = 0 ; w <= 0 ; w++) {
                regAddrDbPtr->HA.haVLANTranslationEnable[w] =
                    0x1c000130+4*w;
            }/* end of loop w */
        }/*0x1c000130+4*w*/


        {/*0x1c000300+4*n*/
            GT_U32    n;
            for(n = 0 ; n <= 15 ; n++) {
                regAddrDbPtr->HA.qoSProfileToEXPReg[n] =
                    0x1c000300+4*n;
            }/* end of loop n */
        }/*0x1c000300+4*n*/


        {/*0x1c000340+4*n*/
            GT_U32    n;
            for(n = 0 ; n <= 7 ; n++) {
                regAddrDbPtr->HA.qoSProfileToDPReg[n] =
                    0x1c000340+4*n;
            }/* end of loop n */
        }/*0x1c000340+4*n*/

        {/*0x1c000404+4*w*/
            GT_U32    w;
            for(w = 0 ; w <= 0 ; w++) {
                regAddrDbPtr->HA.toAnalyzerVLANTagAddEnableConfig[w] =
                    0x1c000404+4*w;
            }/* end of loop w */
        }/*0x1c000404+4*w*/


        regAddrDbPtr->HA.ingrAnalyzerVLANTagConfig = 0x1c000408;
        regAddrDbPtr->HA.egrAnalyzerVLANTagConfig = 0x1c00040c;
        regAddrDbPtr->HA.tunnelStartFragmentIDConfig = 0x1c000410;

        regAddrDbPtr->HA.CPIDReg0 = 0x1c000428;
        regAddrDbPtr->HA.CPIDReg1 = 0x1c00042c;

        {/*0x1c000430+n*4*/
            GT_U32    n;
            for(n = 0 ; n <= 3 ; n++) {
                regAddrDbPtr->HA.TPIDConfigReg[n] =
                    0x1c000430+n*4;
            }/* end of loop n */
        }/*0x1c000430+n*4*/

        {/*0x1c000440+n*4*/
            GT_U32    n;
            for(n = 0 ; n <= 3 ; n++) {
                regAddrDbPtr->HA.portTag0TPIDSelect[n] =
                    0x1c000440+n*4;
            }/* end of loop n */
        }/*0x1c000440+n*4*/


        {/*0x1c000460+n*4*/
            GT_U32    n;
            for(n = 0 ; n <= 3 ; n++) {
                regAddrDbPtr->HA.portTag1TPIDSelect[n] =
                    0x1c000460+n*4;
            }/* end of loop n */
        }/*0x1c000460+n*4*/

        regAddrDbPtr->HA.MPLSEthertype = 0x1c000480;
        regAddrDbPtr->HA.IEthertype = 0x1c000484;
        regAddrDbPtr->HA.BpeInternalConfig = 0x1c0004B0;
        regAddrDbPtr->HA.PerPortBpeEnable = 0x1c000600;

        {/*0x1c000700+n*4*/
            GT_U32    n;
            for(n = 0 ; n <= 3 ; n++) {
                regAddrDbPtr->HA.tsTPIDConfigReg[n] =
                    0x1c000700+n*4;
            }/* end of loop n */
        }/*0x1c000700+n*4*/

        {/*0x1c000710+n*4*/
            GT_U32    n;
            for(n = 0 ; n <= 3 ; n++) {
                regAddrDbPtr->HA.portTsTagTpidSelect[n] =
                    0x1c000710+n*4;
            }/* end of loop n */
        }/*0x1c000710+n*4*/

        regAddrDbPtr->HA.passengerTpidIndexSource = 0x1c000730;

        {/*0x1c000750+n*4*/
            GT_U32    n;
            for(n = 0 ; n <= 3 ; n++) {
                regAddrDbPtr->HA.passengerTpid0Tpid1[n] =
                    0x1c000750+n*4;
            }/* end of loop n */
        }/*0x1c000750+n*4*/

        {/*0x1c000764+n*4*/
            GT_U32    n;
            for(n = 0 ; n <= 3 ; n++) {
                regAddrDbPtr->HA.mplsControlWord[n] =
                    0x1c000764+n*4;
            }/* end of loop n */
        }/*0x1c000764+n*4*/

        regAddrDbPtr->HA.from_cpuConstantBits = 0x1c000780;
        regAddrDbPtr->HA.dsaPortisFanoutDevice = 0x1c000790;

    }/*end of unit HA - HA  */

    {/*start of unit MEM - MEM  */
        {/*start of unit arbitersConfigRegs */
            regAddrDbPtr->MEM.arbitersConfigRegs.arbitersConfigReg0 = 0x0e0000a0;

        }/*end of unit arbitersConfigRegs */


        {/*start of unit buffersMemoryAndMACErrorsIndicationsInterrupts */
            regAddrDbPtr->MEM.buffersMemoryAndMACErrorsIndicationsInterrupts.bufferMemoryInterruptCauseReg0 = 0x0e000024;
            regAddrDbPtr->MEM.buffersMemoryAndMACErrorsIndicationsInterrupts.bufferMemoryInterruptMaskReg0 = 0x0e000028;
            regAddrDbPtr->MEM.buffersMemoryAndMACErrorsIndicationsInterrupts.bufferMemoryMainInterruptCauseReg = 0x0e000090;
            regAddrDbPtr->MEM.buffersMemoryAndMACErrorsIndicationsInterrupts.bufferMemoryMainInterruptMaskReg = 0x0e000094;
            regAddrDbPtr->MEM.buffersMemoryAndMACErrorsIndicationsInterrupts.bufferMemoryInterruptCauseReg1 = 0x0e000098;
            regAddrDbPtr->MEM.buffersMemoryAndMACErrorsIndicationsInterrupts.bufferMemoryInterruptMaskReg1 = 0x0e00009c;

        }/*end of unit buffersMemoryAndMACErrorsIndicationsInterrupts */



    }/*end of unit MEM - MEM  */

    {/*start of unit centralizedCntrs - Centralized Counters  */

        {/*start of unit globalRegs */
            regAddrDbPtr->centralizedCntrs.globalRegs.CNCGlobalConfigReg = 0x10000000;
            regAddrDbPtr->centralizedCntrs.globalRegs.CNCFastDumpTriggerReg = 0x10000030;
            regAddrDbPtr->centralizedCntrs.globalRegs.CNCClearByReadValueRegWord0 = 0x10000040;
            regAddrDbPtr->centralizedCntrs.globalRegs.CNCClearByReadValueRegWord1 = 0x10000044;
            regAddrDbPtr->centralizedCntrs.globalRegs.CNCInterruptSummaryCauseReg = 0x10000100;
            regAddrDbPtr->centralizedCntrs.globalRegs.CNCInterruptSummaryMaskReg = 0x10000104;

        }/*end of unit globalRegs */


        {/*start of unit perBlockRegs */
            {/*0x100010a0+n*0x100+m*0x4*/
                GT_U32    n,m;
                for(n = 0 ; n <= 1 ; n++) {
                    for(m = 0 ; m <= 3 ; m++) {
                        regAddrDbPtr->centralizedCntrs.perBlockRegs.CNCBlockWraparoundStatusReg[n][m] =
                            0x100010a0+n*0x100+m*0x4;
                    }/* end of loop m */
                }/* end of loop n */
            }/*0x100010a0+n*0x100+m*0x4*/


            {/*0x10001080+n*0x100+m*0x4*/
                GT_U32    n,m;
                for(n = 0 ; n <= 1 ; n++) {
                    for(m = 0 ; m <= 7 ; m++) {
                    regAddrDbPtr->centralizedCntrs.perBlockRegs.CNCBlockConfigReg[n][m] =
                        0x10001080+n*0x100+m*0x4;
                    }/* end of loop m */
                }/* end of loop n */
            }/*0x10001080+n*0x100+m*0x4*/

        }/*end of unit perBlockRegs */

    }/*end of unit centralizedCntrs - Centralized Counters  */


    {/*start of unit EQ - EQ  */
        {/*start of unit applicationSpecificCPUCodes */
            {/*0x14007000+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 15 ; n++) {
                    regAddrDbPtr->EQ.applicationSpecificCPUCodes.TCPUDPDestPortRangeCPUCodeEntryWord0[n] =
                        0x14007000+n*0x4;
                }/* end of loop n */
            }/*0x14007000+n*0x4*/


            {/*0x14007040+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 15 ; n++) {
                    regAddrDbPtr->EQ.applicationSpecificCPUCodes.TCPUDPDestPortRangeCPUCodeEntryWord1[n] =
                        0x14007040+n*0x4;
                }/* end of loop n */
            }/*0x14007040+n*0x4*/


            {/*0x14008000+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->EQ.applicationSpecificCPUCodes.IPProtocolCPUCodeEntry[n] =
                        0x14008000+n*0x4;
                }/* end of loop n */
            }/*0x14008000+n*0x4*/


            regAddrDbPtr->EQ.applicationSpecificCPUCodes.IPProtocolCPUCodeValidConfig = 0x14008010;

        }/*end of unit applicationSpecificCPUCodes */


        {/*start of unit CPUCodeTable */
            regAddrDbPtr->EQ.CPUCodeTable.CPUCodeAccessCtrl = 0x14000030;
            regAddrDbPtr->EQ.CPUCodeTable.CPUCodeTableDataAccess = 0x14003000;

        }/*end of unit CPUCodeTable */


        {/*start of unit CPUTargetDeviceConfig */
            regAddrDbPtr->EQ.CPUTargetDeviceConfig.CPUTargetDeviceConfigReg0 = 0x14000010;
            regAddrDbPtr->EQ.CPUTargetDeviceConfig.CPUTargetDeviceConfigReg1 = 0x14000014;

        }/*end of unit CPUTargetDeviceConfig */


        {/*start of unit egress Filter Vlan Map*/
            regAddrDbPtr->EQ.egressFilterVlanMap.egressFilterVlanMapTableAccessCtrl = 0x140000a4;
            regAddrDbPtr->EQ.egressFilterVlanMap.egressFilterVlanMapTableDataAccess = 0x140000a8;
        }/*end of unit egress Filter Vlan Map*/


        {/*start of unit egress Filter Vlan Member Table*/
             regAddrDbPtr->EQ.egressFilterVlanMemberTable.egressFilterVlanMemberTableAccessCtrl = 0x140000ac;
             regAddrDbPtr->EQ.egressFilterVlanMemberTable.egressFilterVlanMemberTableDataAccess = 0x140000b0;
             regAddrDbPtr->EQ.egressFilterVlanMemberTable.egressFilterVlanMemberTableDataAccess2 = 0x140000b4;
        }/*end of unit egress Filter Vlan Member Table*/


        {/*start of unit ingrDropCntr */
            regAddrDbPtr->EQ.ingrDropCntr.ingrDropCntrConfig = 0x1400003c;
            regAddrDbPtr->EQ.ingrDropCntr.ingrDropCntr = 0x14000040;

        }/*end of unit ingrDropCntr */


        {/*start of unit ingrForwardingRestrictions */
            regAddrDbPtr->EQ.ingrForwardingRestrictions.toCpuIngrForwardingRestrictionsConfig = 0x14020000;
            regAddrDbPtr->EQ.ingrForwardingRestrictions.TONETWORKIngrForwardingRestrictionsConfig = 0x14020004;
            regAddrDbPtr->EQ.ingrForwardingRestrictions.toAnalyzerIngrForwardingRestrictionsConfig = 0x14020008;
            regAddrDbPtr->EQ.ingrForwardingRestrictions.ingrForwardingRestrictionsDroppedPktsCntr = 0x1402000c;

        }/*end of unit ingrForwardingRestrictions */


        {/*start of unit ingrSTCConfig */
            regAddrDbPtr->EQ.ingrSTCConfig.ingrSTCConfig = 0x1400001c;

        }/*end of unit ingrSTCConfig */


        {/*start of unit ingrSTCInterrupt */
            regAddrDbPtr->EQ.ingrSTCInterrupt.ingrSTCInterruptCause = 0x14000020;
            regAddrDbPtr->EQ.ingrSTCInterrupt.ingrSTCInterruptMask = 0x14000024;

        }/*end of unit ingrSTCInterrupt */


        {/*start of unit ingrSTCTable */
            regAddrDbPtr->EQ.ingrSTCTable.ingrSTCTableAccessCtrl = 0x14000038;
            regAddrDbPtr->EQ.ingrSTCTable.ingrSTCTableWord0Access = 0x14005000;
            regAddrDbPtr->EQ.ingrSTCTable.ingrSTCTableWord1Read = 0x14005004;
            regAddrDbPtr->EQ.ingrSTCTable.ingrSTCTableWord2Access = 0x14005008;

        }/*end of unit ingrSTCTable */


        {/*start of unit logicalTargetMapTable */
            regAddrDbPtr->EQ.logicalTargetMapTable.logicalTargetMapTableAccessCtrl = 0x140000a0;
            regAddrDbPtr->EQ.logicalTargetMapTable.logicalTargetMapTableDataAccess = 0x1400a000;
            regAddrDbPtr->EQ.logicalTargetMapTable.logicalTargetMapTableDataAccess2 = 0x1400a004;

        }/*end of unit logicalTargetMapTable */


        {/*start of unit mirrToAnalyzerPortConfigs */
            regAddrDbPtr->EQ.mirrToAnalyzerPortConfigs.ingrStatisticMirrToAnalyzerPortConfig = 0x14000000;
            regAddrDbPtr->EQ.mirrToAnalyzerPortConfigs.ingrAndEgrMonitoringToAnalyzerQoSConfig = 0x14000004;
            regAddrDbPtr->EQ.mirrToAnalyzerPortConfigs.egrMonitoringEnableConfig[0] = 0x1400000c;

            regAddrDbPtr->EQ.mirrToAnalyzerPortConfigs.portIngrMirrorIndex[0] = 0x1400b000;
            regAddrDbPtr->EQ.mirrToAnalyzerPortConfigs.portIngrMirrorIndex[1] = 0x1400b004;
            regAddrDbPtr->EQ.mirrToAnalyzerPortConfigs.portIngrMirrorIndex[2] = 0x1400b008;
            regAddrDbPtr->EQ.mirrToAnalyzerPortConfigs.portEgrMirrorIndex[0][0] = 0x1400b010;
            regAddrDbPtr->EQ.mirrToAnalyzerPortConfigs.portEgrMirrorIndex[0][1] = 0x1400b014;
            regAddrDbPtr->EQ.mirrToAnalyzerPortConfigs.portEgrMirrorIndex[0][2] = 0x1400b018;
            {/*0x1400b020+i*4*/
                GT_U32    i;
                for(i = 0 ; i <= 6 ; i++) {
                    regAddrDbPtr->EQ.mirrToAnalyzerPortConfigs.mirrorInterfaceParameterReg[i] =
                        0x1400b020+i*4;
                }/* end of loop i */
            }/*0x1400b020+i*4*/


            regAddrDbPtr->EQ.mirrToAnalyzerPortConfigs.analyzerPortGlobalConfig = 0x1400b040;

        }/*end of unit mirrToAnalyzerPortConfigs */


        {/*start of unit numberOfTrunkMembersTable */
            {/*0x14009000+e*0x4*/
                GT_U32    e;
                for(e = 0 ; e <= 15 ; e++) {
                    regAddrDbPtr->EQ.numberOfTrunkMembersTable.numberOfTrunkMembersTableEntry[e] =
                        0x14009000+e*0x4;
                }/* end of loop e */
            }/*0x14009000+e*0x4*/



        }/*end of unit numberOfTrunkMembersTable */


        {/*start of unit preEgrEngineGlobalConfig */
            regAddrDbPtr->EQ.preEgrEngineGlobalConfig.duplicationOfPktsToCPUConfig = 0x14000018;
            regAddrDbPtr->EQ.preEgrEngineGlobalConfig.preEgrEngineGlobalConfig = 0x14050000;
            regAddrDbPtr->EQ.preEgrEngineGlobalConfig.logicalTargetDeviceMapConfig = 0x14050004;

        }/*end of unit preEgrEngineGlobalConfig */


        {/*start of unit preEgrEngineInternal */
            {/*0x14000044+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 4 ; n++) {
                    regAddrDbPtr->EQ.preEgrEngineInternal.preEgrEngineOutGoingDescWord[n] =
                        0x14000044+n*0x4;
                }/* end of loop n */
            }/*0x14000044+n*0x4*/


            {/*0x1400b050+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->EQ.preEgrEngineInternal.preEgrEngineOutGoingDescWord1[n] =
                        0x1400b050+n*0x4;
                }/* end of loop n */
            }/*0x1400b050+n*0x4*/



        }/*end of unit preEgrEngineInternal */


        {/*start of unit preEgrInterrupt */
            regAddrDbPtr->EQ.preEgrInterrupt.preEgrInterruptSummary = 0x14000058;
            regAddrDbPtr->EQ.preEgrInterrupt.preEgrInterruptSummaryMask = 0x1400005c;

        }/*end of unit preEgrInterrupt */


        {/*start of unit qoSProfileToQoSTable */
            regAddrDbPtr->EQ.qoSProfileToQoSTable.qoSProfileToQoSAccessCtrl = 0x1400002c;
            regAddrDbPtr->EQ.qoSProfileToQoSTable.qoSProfileToQoSTableDataAccess = 0x14002000;

        }/*end of unit qoSProfileToQoSTable */


        {/*start of unit statisticalRateLimitsTable */
            regAddrDbPtr->EQ.statisticalRateLimitsTable.statisticalRateLimitsTableAccessCtrl = 0x14000034;
            regAddrDbPtr->EQ.statisticalRateLimitsTable.statisticalRateLimitsTableDataAccess = 0x14004000;

        }/*end of unit statisticalRateLimitsTable */


        {/*start of unit STCRateLimitersInterrupt */
            regAddrDbPtr->EQ.STCRateLimitersInterrupt.CPUCodeRateLimitersInterruptCause = 0x14000060;
            regAddrDbPtr->EQ.STCRateLimitersInterrupt.CPUCodeRateLimitersInterruptMask = 0x14000064;

        }/*end of unit STCRateLimitersInterrupt */


        {/*start of unit toCpuPktRateLimiters */
            regAddrDbPtr->EQ.toCpuPktRateLimiters.CPUCodeRateLimiterDropCntr = 0x14000068;
            {/*0x14080000+n*0x4*/
                GT_U32    n;
                for(n = 1 ; n <= 31 ; n++) {
                    regAddrDbPtr->EQ.toCpuPktRateLimiters.toCpuRateLimiterPktCntr[n-1] =
                        0x14080000+n*0x4;
                }/* end of loop n */
            }/*0x14080000+n*0x4*/



        }/*end of unit toCpuPktRateLimiters */


        {/*start of unit trunksMembersTable */
            regAddrDbPtr->EQ.trunksMembersTable.trunkTableAccessCtrl = 0x14000028;
            regAddrDbPtr->EQ.trunksMembersTable.trunkTableDataAccess = 0x14001000;

        }/*end of unit trunksMembersTable */

        {/*start of unit Vlan Egress Filter Counter */
            regAddrDbPtr->EQ.vlanEgressFilter.vlanEgressFilterCounter = 0x14000074;
        }/*end of unit Vlan Egress Filter Counter */

    }/*end of unit EQ - EQ  */

    {/*start of unit IPCL - IPCL  */
        regAddrDbPtr->IPCL.ingrPolicyGlobalConfig = 0x15000000;
        regAddrDbPtr->IPCL.policyEngineInterruptCause = 0x15000004;
        regAddrDbPtr->IPCL.policyEngineInterruptMask = 0x15000008;
        regAddrDbPtr->IPCL.policyEngineConfig = 0x1500000c;
        regAddrDbPtr->IPCL.policyEngineUserDefinedBytesConfig = 0x15000014;

        {
            GT_U32    i;
            for(i = 0 ; i <= 1 ; i++) {
                regAddrDbPtr->IPCL.L2L3IngrVLANCountingEnable[i] =
                    0x15000080+4*i;
            }/* end of loop i */
        }/*0x15000080+4*i*/

        regAddrDbPtr->IPCL.countingModeConfig = 0x15000088;

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

            {/*0x17000074+n*4*/
                GT_U32    n;
                for(n = 0 ; n <= 7 ; n++) {
                    regAddrDbPtr->PLR[cycle].policerTableAccessData[n] =
                        SMEM_XCAT_POLICER_OFFSET_MAC(devObjPtr, cycle, 0x00000074+n*4);
                }/* end of loop n */
            }/*0x17000074+n*4*/


            {/*0x170000c0+n*4*/
                GT_U32    n;
                for(n = 0 ; n <= 7 ; n++) {
                    regAddrDbPtr->PLR[cycle].policerInitialDP[n] =
                        SMEM_XCAT_POLICER_OFFSET_MAC(devObjPtr, cycle, 0x000000c0+n*4);
                }/* end of loop n */
            }/*0x170000c0+n*4*/


            regAddrDbPtr->PLR[cycle].policerInterruptCause =
                SMEM_XCAT_POLICER_OFFSET_MAC(devObjPtr, cycle, 0x00000100);
            regAddrDbPtr->PLR[cycle].policerInterruptMask =
                SMEM_XCAT_POLICER_OFFSET_MAC(devObjPtr, cycle, 0x00000104);
            {/*0x17000108+n*4*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->PLR[cycle].policerShadow[n] =
                        SMEM_XCAT_POLICER_OFFSET_MAC(devObjPtr, cycle, 0x00000108+n*4);
                }/* end of loop n */
            }/*0x17000108+n*4*/


            {/*0x17001800+p*4*/
                GT_U32    p;
                for(p = 0 ; p <= 31 ; p++) {
                    regAddrDbPtr->PLR[cycle].portAndPktTypeTranslationTable[p] =
                        SMEM_XCAT_POLICER_OFFSET_MAC(devObjPtr, cycle, 0x00001800+p*4);
                }/* end of loop p */
            }/*0x17001800+p*4*/
        }
    }/*end of unit PLR[0],1, PLR[2]  - added manually */

    {/*start of unit TCCLowerIPCL - TCC Lower (IPCL)  */
        {/*start of unit TCCIPCL */
            {/*0x1a000100+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 11 ; n++) {
                    regAddrDbPtr->TCCLowerIPCL.TCCIPCL.actionTableAndPolicyTCAMAccessData[n] =
                        0x1a000100+n*0x4;
                }/* end of loop n */
            }/*0x1a000100+n*0x4*/


            regAddrDbPtr->TCCLowerIPCL.TCCIPCL.actionTableAndPolicyTCAMAccessCtrlParameters1 = 0x1a000130;
            regAddrDbPtr->TCCLowerIPCL.TCCIPCL.actionTableAndPolicyTCAMAccessCtrlParameters2 = 0x1a000134;
            regAddrDbPtr->TCCLowerIPCL.TCCIPCL.actionTableAndPolicyTCAMAccessCtrl = 0x1a000138;

            {/*0x1a000084+8*p*/
                GT_U32    p;
                for(p = 0 ; p <= 2 ; p++) {
                    regAddrDbPtr->TCCLowerIPCL.TCCIPCL.IPCLKeyType2LookupIDReg0[p] =
                        0x1a000084+8*p;
                }/* end of loop p */
            }/*0x1a000084+8*p*/


            {/*0x1a000088+8*p*/
                GT_U32    p;
                for(p = 0 ; p <= 2 ; p++) {
                    regAddrDbPtr->TCCLowerIPCL.TCCIPCL.IPCLKeyType2LookupIDReg1[p] =
                        0x1a000088+8*p;
                }/* end of loop p */
            }/*0x1a000088+8*p*/


            regAddrDbPtr->TCCLowerIPCL.TCCIPCL.EPCLKeyType2LookupIDReg = 0x1a00009c;

            regAddrDbPtr->TCCLowerIPCL.TCCIPCL.TCCECCErrorInformation = 0x1a000170;
            regAddrDbPtr->TCCLowerIPCL.TCCIPCL.TCAMErrorCntr = 0x1a000198;
            regAddrDbPtr->TCCLowerIPCL.TCCIPCL.TCCInterruptCause = 0x1a0001a4;
            regAddrDbPtr->TCCLowerIPCL.TCCIPCL.TCCInterruptMask = 0x1a0001a8;

        }/*end of unit TCCIPCL */

    }/*end of unit TCCLowerIPCL - TCC Lower (IPCL)  */

    {/*start of unit TCCUpperIPvX - TCC Upper (IPvX)  */
        {/*start of unit TCCIPvX */
            regAddrDbPtr->TCCUpperIPvX.TCCIPvX.TTIKeyType2LookupID = 0x1b000080;
            regAddrDbPtr->TCCUpperIPvX.TCCIPvX.routerDIPLookupKeyType2LookupID = 0x1b0000a0;
            regAddrDbPtr->TCCUpperIPvX.TCCIPvX.routerSIPLookupKeyType2LookupID = 0x1b0000a4;

            {/*0x1b000400+n*4*/
                GT_U32    n;
                for(n = 0 ; n <= 5 ; n++) {
                    regAddrDbPtr->TCCUpperIPvX.TCCIPvX.routerTCAMAccessDataReg[n] =
                        0x1b000400+n*4;
                }/* end of loop n */
            }/*0x1b000400+n*4*/


            regAddrDbPtr->TCCUpperIPvX.TCCIPvX.routerTCAMAccessCtrlReg0 = 0x1b000418;
            regAddrDbPtr->TCCUpperIPvX.TCCIPvX.routerTCAMAccessCtrlReg1 = 0x1b00041c;

            regAddrDbPtr->TCCUpperIPvX.TCCIPvX.TCAMErrorCntr = 0x1b000054;
            regAddrDbPtr->TCCUpperIPvX.TCCIPvX.TCCInterruptCause = 0x1b000060;
            regAddrDbPtr->TCCUpperIPvX.TCCIPvX.TCCInterruptMask = 0x1b000064;
            regAddrDbPtr->TCCUpperIPvX.TCCIPvX.TCCECCErrorInformation = 0x1b000d78;

        }/*end of unit TCCIPvX */

    }/*end of unit TCCUpperIPvX - TCC Upper (IPvX)  */

    {/*start of unit TTI - TTI  */

        {/*start of unit logFlow */
            regAddrDbPtr->TTI.logFlow.bridgeLogFlowMACDALow = 0x16000100;
            regAddrDbPtr->TTI.logFlow.bridgeLogFlowMACDAHigh = 0x16000104;
            regAddrDbPtr->TTI.logFlow.bridgeLogFlowMACDALowMask = 0x16000108;
            regAddrDbPtr->TTI.logFlow.bridgeLogFlowMACDAHighMask = 0x1600010c;
            regAddrDbPtr->TTI.logFlow.bridgeLogFlowMACSALow = 0x16000110;
            regAddrDbPtr->TTI.logFlow.bridgeLogFlowMACSAHigh = 0x16000114;
            regAddrDbPtr->TTI.logFlow.bridgeLogFlowMACSALowMask = 0x16000118;
            regAddrDbPtr->TTI.logFlow.bridgeLogFlowMACSAHighMask = 0x1600011c;
            regAddrDbPtr->TTI.logFlow.bridgeLogFlowEtherType = 0x16000120;
            {/*0x16000140+n*4*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->TTI.logFlow.routerLogFlowDIP[n] =
                        0x16000140+n*4;
                }/* end of loop n */
            }/*0x16000140+n*4*/


            {/*0x16000150+n*4*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->TTI.logFlow.routerLogFlowDIPMask[n] =
                        0x16000150+n*4;
                }/* end of loop n */
            }/*0x16000150+n*4*/


            {/*0x16000160+n*4*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->TTI.logFlow.routerLogFlowSIP[n] =
                        0x16000160+n*4;
                }/* end of loop n */
            }/*0x16000160+n*4*/


            {/*0x16000170+n*4*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->TTI.logFlow.routerLogFlowSIPMask[n] =
                        0x16000170+n*4;
                }/* end of loop n */
            }/*0x16000170+n*4*/


            regAddrDbPtr->TTI.logFlow.routerLogFlowIPProtocol = 0x16000180;
            regAddrDbPtr->TTI.logFlow.routerLogFlowSourceTCPUDPPort = 0x16000184;
            regAddrDbPtr->TTI.logFlow.routerLogFlowDestinationTCPUDPPort = 0x16000188;

        }/*end of unit logFlow */


        {/*start of unit MAC2ME */
            {/*0x16001600+n*0x10*/
                GT_U32    n;
                for(n = 0 ; n <= 7 ; n++) {
                    regAddrDbPtr->TTI.MAC2ME.MAC2MELow[n] =
                        0x16001600+n*0x10;
                }/* end of loop n */
            }/*0x16001600+n*0x10*/


            {/*0x16001604+n*0x10*/
                GT_U32    n;
                for(n = 0 ; n <= 7 ; n++) {
                    regAddrDbPtr->TTI.MAC2ME.MAC2MEHigh[n] =
                        0x16001604+n*0x10;
                }/* end of loop n */
            }/*0x16001604+n*0x10*/


            {/*0x16001608+n*0x10*/
                GT_U32    n;
                for(n = 0 ; n <= 7 ; n++) {
                    regAddrDbPtr->TTI.MAC2ME.MAC2MELowMask[n] =
                        0x16001608+n*0x10;
                }/* end of loop n */
            }/*0x16001608+n*0x10*/


            {/*0x1600160c+n*0x10*/
                GT_U32    n;
                for(n = 0 ; n <= 7 ; n++) {
                    regAddrDbPtr->TTI.MAC2ME.MAC2MEHighMask[n] =
                        0x1600160c+n*0x10;
                }/* end of loop n */
            }/*0x1600160c+n*0x10*/



            {/*0x16001700+n*4*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->TTI.MAC2ME.MGLookupToInternalTCAMData[n] =
                        0x16001700+n*4;
                }/* end of loop n */
            }/*0x16001700+n*4*/


            regAddrDbPtr->TTI.MAC2ME.MGLookupToInternalTCAMCtrl = 0x16001708;
            regAddrDbPtr->TTI.MAC2ME.MGLookupToInternalTCAMResult = 0x1600170c;

        }/*end of unit MAC2ME */


        {/*start of unit protocolMatch */
            {/*0x16000080+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 5 ; n++) {
                    regAddrDbPtr->TTI.protocolMatch.protocolsConfig[n] =
                        0x16000080+n*0x4;
                }/* end of loop n */
            }/*0x16000080+n*0x4*/


            regAddrDbPtr->TTI.protocolMatch.protocolsEncapsulationConfig0 = 0x16000098;
            regAddrDbPtr->TTI.protocolMatch.protocolsEncapsulationConfig1 = 0x1600009c;

        }/*end of unit protocolMatch */


        {/*start of unit qoSMapTables */
            {/*0x16000500+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->TTI.qoSMapTables.UPToQoSParametersMapTableReg[n] =
                        0x16000500+n*0x4;
                }/* end of loop n */
            }/*0x16000500+n*0x4*/


            {/*0x16000510+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    regAddrDbPtr->TTI.qoSMapTables.EXPToQoSParametersMapTableReg[n] =
                        0x16000510+n*0x4;
                }/* end of loop n */
            }/*0x16000510+n*0x4*/


            regAddrDbPtr->TTI.qoSMapTables.UP2UPMapTable = 0x16000520;

            {/*0x16000400+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 15 ; n++) {
                    regAddrDbPtr->TTI.qoSMapTables.DSCP_ToQoSProfileMap[n] =
                        0x16000400+n*0x4;
                }/* end of loop n */
            }/*0x16000400+n*0x4*/


            {/*0x16000440+n*0x4+m*0x10*/
                GT_U32    n,m;
                for(n = 0 ; n <= 1 ; n++) {
                    for(m = 0 ; m <= 1 ; m++) {
                        regAddrDbPtr->TTI.qoSMapTables.CFIUP_ToQoSProfileMapTable[n][m][0] =
                                0x16000440+n*0x4+m*0x10;
                    }/* end of loop m */
                }/* end of loop n */
            }/*0x16000440+n*0x4+m*0x10*/


            {/*0x16000460+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->TTI.qoSMapTables.EXPToQoSProfileMap[n] =
                        0x16000460+n*0x4;
                }/* end of loop n */
            }/*0x16000460+n*0x4*/


        }/*end of unit qoSMapTables */


        {/*start of unit trunkHash */
            regAddrDbPtr->TTI.trunkHash.trunkHashConfigReg0 = 0x16000070;
            regAddrDbPtr->TTI.trunkHash.trunkHashConfigReg1 = 0x16000074;
            regAddrDbPtr->TTI.trunkHash.trunkHashConfigReg2 = 0x16000078;

        }/*end of unit trunkHash */


        {/*start of unit userDefinedBytes */
            {/*0x160000a0+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 7 ; n++) {
                    regAddrDbPtr->TTI.userDefinedBytes.IPCLTCPPortRangeComparatorConfig[n] =
                        0x160000a0+n*0x4;
                }/* end of loop n */
            }/*0x160000a0+n*0x4*/


            {/*0x160000c0+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 7 ; n++) {
                    regAddrDbPtr->TTI.userDefinedBytes.IPCLUDPPortRangeComparatorConfig[n] =
                        0x160000c0+n*0x4;
                }/* end of loop n */
            }/*0x160000c0+n*0x4*/



        }/*end of unit userDefinedBytes */


        {/*start of unit VLANAssignment */
            {/*0x16000300+i*4*/
                GT_U32    i;
                for(i = 0 ; i <= 3 ; i++) {
                    regAddrDbPtr->TTI.VLANAssignment.ingrTPIDConfig[i] =
                        0x16000300+i*4;
                }/* end of loop i */
            }/*0x16000300+i*4*/


            {/*0x16000310+n*4*/
                GT_U32    n;
                for(n = 0 ; n <= 31 ; n++) {
                    regAddrDbPtr->TTI.VLANAssignment.ingrTPIDSelect[n] =
                        0x16000310+n*4;
                }/* end of loop n */
            }/*0x16000310+n*4*/

            regAddrDbPtr->TTI.VLANAssignment.passengerIngrTPIDSelect[0] = 0x16000524;

        }/*end of unit VLANAssignment */


        regAddrDbPtr->TTI.TTIUnitGlobalConfig = 0x16000000;
        regAddrDbPtr->TTI.TTIEngineInterruptCause = 0x16000004;
        regAddrDbPtr->TTI.TTIEngineInterruptMask = 0x16000008;
        regAddrDbPtr->TTI.TTIEngineConfig = 0x1600000c;
        regAddrDbPtr->TTI.TTIPCLIDConfig0 = 0x16000010;
        regAddrDbPtr->TTI.TTIPCLIDConfig1 = 0x16000014;
        regAddrDbPtr->TTI.TTIIPv4GREEthertype = 0x16000018;
        regAddrDbPtr->TTI.SrcPortBpeEnable = 0x1600001C;
        regAddrDbPtr->TTI.CNAndFCConfig = 0x16000024;
        regAddrDbPtr->TTI.specialEtherTypes = 0x16000028;
        regAddrDbPtr->TTI.ingrCPUTrigSamplingConfig = 0x1600002c;
        regAddrDbPtr->TTI.MPLSEtherTypes = 0x16000030;
        regAddrDbPtr->TTI.pseudoWireConfig = 0x16000034;
        regAddrDbPtr->TTI.IPv6ExtensionValue = 0x16000038;
        regAddrDbPtr->TTI.IPv6SolicitedNodeMcAddrConfigReg[0] = 0x16000040;
        regAddrDbPtr->TTI.IPv6SolicitedNodeMcAddrConfigReg[1] = 0x16000044;
        regAddrDbPtr->TTI.IPv6SolicitedNodeMcAddrConfigReg[2] = 0x16000048;
        regAddrDbPtr->TTI.IPv6SolicitedNodeMcAddrConfigReg[3] = 0x1600004c;
        regAddrDbPtr->TTI.IPv6SolicitedNodeMcAddrMaskReg[0] = 0x16000050;
        regAddrDbPtr->TTI.IPv6SolicitedNodeMcAddrMaskReg[1] = 0x16000054;
        regAddrDbPtr->TTI.IPv6SolicitedNodeMcAddrMaskReg[2] = 0x16000058;
        regAddrDbPtr->TTI.IPv6SolicitedNodeMcAddrMaskReg[3] = 0x1600005c;
        regAddrDbPtr->TTI.DSAConfig = 0x16000060;

        {/*0x16000064+n*4*/
            GT_U32    n;
            for(n = 0 ; n <= 1 ; n++) {
                regAddrDbPtr->TTI.loopPort[n] =
                    0x16000064+n*4;
            }/* end of loop n */
        }/*0x16000064+n*4*/

        regAddrDbPtr->TTI.metalFix = 0x160002b0;
        regAddrDbPtr->TTI.TTIInternalConfigurations = 0x160002b4;

        {/*0x16000900+n*0x4*/
            GT_U32    n;
            for(n = 0 ; n <= 15 ; n++) {
                regAddrDbPtr->TTI.DSCP2DSCPMapTable[n] =
                    0x16000900+n*0x4;
            }/* end of loop n */
        }/*0x16000900+n*0x4*/

    }/*end of unit TTI - TTI  */


    {/*start of unit MLL - MLL  */
        {/*start of unit l2MllVidxEnableTable */
            regAddrDbPtr->MLL.l2MllVidxEnableTable.l2MllVidxEnableTableAccessCtrl = 0x19000464;
            regAddrDbPtr->MLL.l2MllVidxEnableTable.l2MllVidxEnableTableDataAccess = 0x19000460;
        }/*end of unit l2MllVidxEnableTable */

        {/*start of unit l2MllPointerMapTable */
            regAddrDbPtr->MLL.l2MllPointerMapTable.l2MllPointerMapTableAccessCtrl = 0x19000474;
            regAddrDbPtr->MLL.l2MllPointerMapTable.l2MllPointerMapTableDataAccess = 0x19000470;
        }/*end of unit l2MllPointerMapTable */

        {/*start of unit MLLGlobalCtrl */
            regAddrDbPtr->MLL.MLLGlobalCtrl.MLLGlobalCtrl = 0x19000000;

        }/*end of unit MLLGlobalCtrl */

        {/*start of unit MLLOutInterfaceCntrs */
            {/*0x19000900+n*0x100*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->MLL.MLLOutInterfaceCntrs.MLLOutMcPktsCntr[n] =
                        0x19000900+n*0x100;
                }/* end of loop n */
            }/*0x19000900+n*0x100*/


            {/*0x19000980+n*0x100*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    regAddrDbPtr->MLL.MLLOutInterfaceCntrs.MLLOutInterfaceCntrConfig[n] =
                        0x19000980+n*0x100;
                }/* end of loop n */
            }/*0x19000980+n*0x100*/


            regAddrDbPtr->MLL.MLLOutInterfaceCntrs.MLLMCFIFODropCntr = 0x19000984;

        }/*end of unit MLLOutInterfaceCntrs */


        {/*start of unit multiTargetTCQueuesAndArbitrationConfig */
            regAddrDbPtr->MLL.multiTargetTCQueuesAndArbitrationConfig.multiTargetTCQueuesGlobalConfig = 0x19000004;
            regAddrDbPtr->MLL.multiTargetTCQueuesAndArbitrationConfig.multitargetTCQueuesStrictPriorityEnableConfig = 0x19000200;
            regAddrDbPtr->MLL.multiTargetTCQueuesAndArbitrationConfig.multitargetTCQueuesWeightConfig = 0x19000204;

        }/*end of unit multiTargetTCQueuesAndArbitrationConfig */


        {/*start of unit multiTargetVsUcSDWRRAndStrictPriorityScheduler */
            regAddrDbPtr->MLL.multiTargetVsUcSDWRRAndStrictPriorityScheduler.mcUcSDWRRAndStrictPriorityConfig = 0x19000214;

        }/*end of unit multiTargetVsUcSDWRRAndStrictPriorityScheduler */


        {/*start of unit multiTargetRateShape */
            regAddrDbPtr->MLL.multiTargetRateShape.multiTargetRateShapeConfig = 0x19000210;

        }/*end of unit multiTargetRateShape */


        {/*start of unit routerMcLinkedListMLLTables */
            {/*0x19000100+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 7 ; n++) {
                    regAddrDbPtr->MLL.routerMcLinkedListMLLTables.qoSProfileToMultiTargetTCQueuesReg[n] =
                        0x19000100+n*0x4;
                }/* end of loop n */
            }/*0x19000100+n*0x4*/

        }/*end of unit routerMcLinkedListMLLTables */

    }/*end of unit MLL - MLL  */


    {/*start of unit GOP - GOP  */

        {/*start of unit SERDES */
            GT_U32    s;
            for(s = 0 ; s < 12 /*numOfSerdeses*/ ; s++) {
                smemLion2RegsInfoSet_GOP_SERDES(devObjPtr, regAddrDbPtr, s, 0x13000000, 1);
            }/* end of loop s */
        }/*end of unit SERDES */

        {/*start of unit gigPort */
            GT_U32    p;
            for(p = 0 ; p < 28 /*numOfGigMacPorts*/ ; p++) {
                smemLion2RegsInfoSet_GOP_gigPort(devObjPtr, regAddrDbPtr, p, 0, 0x12000000, 1);
            }/* end of loop p */

            p = 31;
            smemLion2RegsInfoSet_GOP_gigPort(devObjPtr, regAddrDbPtr, p, 0, 0x12000000, 1);
        }/*end of unit gigPort */

        {/*start of unit XLGIP */
            GT_U32    p;
            for(p = 24 ; p <= 29 ; p++) {
                smemLion2RegsInfoSet_GOP_XLGIP(devObjPtr, regAddrDbPtr, p, 0, 0x12000000);
            }/* end of loop p */

            p = 31;
            smemLion2RegsInfoSet_GOP_XLGIP(devObjPtr, regAddrDbPtr, p, 0, 0x12000000);
        }/*end of unit XLGIP */

        {/*start of unit MPCSIP */
            GT_U32    p;
            for(p = 24 ; p <= 29 ; p++) {
                smemLion2RegsInfoSet_GOP_MPCSIP(devObjPtr, regAddrDbPtr, p, 0, 0x120C0000);
            }/* end of loop p */

            p = 31;
            smemLion2RegsInfoSet_GOP_MPCSIP(devObjPtr, regAddrDbPtr, p, 0, 0x120C0000);

            p = 50;
            smemLion2RegsInfoSet_GOP_MPCSIP(devObjPtr, regAddrDbPtr, p, 0, 0x120C0000);
        }/*end of unit MPCSIP */

    }/*end of unit GOP - GOP  */

    {/*start of unit DFXServerUnitsBC2SpecificRegs */
        SMEM_CHT_MAC_REG_DB_DFX_SERVER_GET(devObjPtr)->resetAndInitCtrller.DFXServerUnitsBC2SpecificRegs.deviceResetCtrl = 0x000f800c;
        SMEM_CHT_MAC_REG_DB_DFX_SERVER_GET(devObjPtr)->resetAndInitCtrller.DFXServerUnitsBC2SpecificRegs.initializationStatusDone = 0x000f8014;
        SMEM_CHT_MAC_REG_DB_DFX_SERVER_GET(devObjPtr)->resetAndInitCtrller.DFXServerUnitsBC2SpecificRegs.configSkipInitializationMatrix = 0x000f8020;
        SMEM_CHT_MAC_REG_DB_DFX_SERVER_GET(devObjPtr)->resetAndInitCtrller.DFXServerUnitsBC2SpecificRegs.RAMInitSkipInitializationMatrix = 0x000f8030;
        SMEM_CHT_MAC_REG_DB_DFX_SERVER_GET(devObjPtr)->resetAndInitCtrller.DFXServerUnitsBC2SpecificRegs.tableSkipInitializationMatrix = 0x000f8060;
        SMEM_CHT_MAC_REG_DB_DFX_SERVER_GET(devObjPtr)->resetAndInitCtrller.DFXServerUnitsBC2SpecificRegs.SERDESSkipInitializationMatrix = 0x000f8064;
        SMEM_CHT_MAC_REG_DB_DFX_SERVER_GET(devObjPtr)->resetAndInitCtrller.DFXServerUnitsBC2SpecificRegs.EEPROMSkipInitializationMatrix = 0x000f8068;
        SMEM_CHT_MAC_REG_DB_DFX_SERVER_GET(devObjPtr)->resetAndInitCtrller.DFXServerUnitsBC2SpecificRegs.PCIeSkipInitializationMatrix = 0x000f806c;
        SMEM_CHT_MAC_REG_DB_DFX_SERVER_GET(devObjPtr)->resetAndInitCtrller.DFXServerUnitsBC2SpecificRegs.deviceSAR1 = 0x000f8200;
        SMEM_CHT_MAC_REG_DB_DFX_SERVER_GET(devObjPtr)->resetAndInitCtrller.DFXServerUnitsBC2SpecificRegs.deviceSAR2 = 0x000f8204;

    }/*end of unit DFXServerUnitsBC2SpecificRegs */

    {/*start of unit DFXServerUnits */
        SMEM_CHT_MAC_REG_DB_DFX_SERVER_GET(devObjPtr)->resetAndInitCtrller.DFXServerUnits.DFXServerRegs.serverStatus = 0x000f8010;
        SMEM_CHT_MAC_REG_DB_DFX_SERVER_GET(devObjPtr)->resetAndInitCtrller.DFXServerUnits.DFXServerRegs.temperatureSensor28nmCtrlLSB = 0x000f8070;
        SMEM_CHT_MAC_REG_DB_DFX_SERVER_GET(devObjPtr)->resetAndInitCtrller.DFXServerUnits.DFXServerRegs.temperatureSensor28nmCtrlMSB = 0x000f8074;
        SMEM_CHT_MAC_REG_DB_DFX_SERVER_GET(devObjPtr)->resetAndInitCtrller.DFXServerUnits.DFXServerRegs.temperatureSensorStatus = 0x000f8078;

    }/*end of unit DFXServerUnits */
}

/*******************************************************************************
*   smemXCat3ForcedPhysicalToVirtualPortMapping
*
* DESCRIPTION:
*       Hard-coded physical to virtual port number mapping for the xCat3 device.
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
void smemXCat3ForcedPhysicalToVirtualPortMapping
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    devObjPtr->portsArr[28].physicalToVirtualPortMappingEn = GT_TRUE;
    devObjPtr->portsArr[28].virtualMapping = 25;
    devObjPtr->portsArr[29].physicalToVirtualPortMappingEn = GT_TRUE;
    devObjPtr->portsArr[29].virtualMapping = 27;

    /* CPU port*/
    devObjPtr->portsArr[devObjPtr->cpuGopPortNumber].physicalToVirtualPortMappingEn = GT_TRUE;
    devObjPtr->portsArr[devObjPtr->cpuGopPortNumber].virtualMapping = SNET_CHT_CPU_PORT_CNS;
}

/*******************************************************************************
*   smemXCat3Init
*
* DESCRIPTION:
*       Init memory module for the xCat3 device.
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
void smemXCat3Init
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    GT_U32  ii;
    SMEM_CHT_DEV_COMMON_MEM_INFO * commonDevMemInfoPtr;
    SMEM_CHT_GENERIC_DEV_MEM_INFO  * devMemInfoPtr;

    if(devObjPtr->registersDfxDefaultsPtr == NULL)
    {
        devObjPtr->registersDfxDefaultsPtr = &element0_DfxDefault_RegistersDefaults;
    }

    devMemInfoPtr = (SMEM_CHT_GENERIC_DEV_MEM_INFO *)smemDeviceObjMemoryAlloc(devObjPtr,1, sizeof(SMEM_CHT_GENERIC_DEV_MEM_INFO));
    if (devMemInfoPtr == 0)
    {
        skernelFatalError("smemXCat3Init: allocation error\n");
    }

    devObjPtr->deviceMemory = devMemInfoPtr;
    commonDevMemInfoPtr= &devMemInfoPtr->common;
    commonDevMemInfoPtr->isPartOfGeneric = 1;

    devObjPtr->supportRegistersDb = 1;

    /* PCI base address conflict with the MG address space.                     */
    /* Please remember that access to PCI registers (through CPSS) must be done */
    /* with the dedicated PCI APIs!!!                                           */
    commonDevMemInfoPtr->pciUnitBaseAddrMask = 0x000f0000;
    commonDevMemInfoPtr->pciUnitBaseAddr = 0x00040000;
    commonDevMemInfoPtr->accessPexMemorySpaceOnlyOnExplicitAction = 1;

    devObjPtr->supportMacSaAssignModePerPort = 1;

    devObjPtr->cncBlockMaxRangeIndex = 2048;
    devObjPtr->cncClientSupportBitmap = SNET_CHT3_CNC_CLIENTS_BMP_ALL_CNS;
    devObjPtr->supportForwardFcPackets = 1;

    /* set router tcam , PCL tacm info -->
       must be done before calling smemCht3AllocSpecMemory(...)*/
    devObjPtr->supportCpuInternalInterface = 1;


    devObjPtr->routeTcamInfo =  xcatRoutTcamInfo;
    devObjPtr->pclTcamInfoPtr = &xcatGlobalPclData;
    devObjPtr->pclTcamMaxNumEntries = devObjPtr->pclTcamInfoPtr->bankEntriesNum * 4;
    devObjPtr->cncBlocksNum = 2;
    devObjPtr->fdbNumEntries = SMEM_MAC_TABLE_SIZE_16KB;
    devObjPtr->policerEngineNum = 2;
    devObjPtr->errata.lttRedirectIndexError = 1;
    devObjPtr->errata.routerGlueResetPartsOfDescriptor = 1;
    devObjPtr->errata.ttCopyToCpuWithAdditionalTag = 1;

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

    devObjPtr->supportRemoveVlanTag1WhenEmpty = 1;
    devObjPtr->errata.supportSrcTrunkToCpuIndicationEnable = 1;

    devObjPtr->supportVpls = 1;
    devObjPtr->supportLogicalMapTableInfo.tableFormatVersion = 1;
    devObjPtr->supportLogicalMapTableInfo.supportFullRange = 1;
    devObjPtr->supportLogicalMapTableInfo.supportEgressVlanFiltering = 1;

    devObjPtr->l2MllVersionSupport = 1;

    devObjPtr->supportTunnelStartEthOverIpv4 = 1;
    devObjPtr->supportTunnelstartIpTotalLengthAddValue = 1;

    devObjPtr->fdbMaxNumEntries = devObjPtr->fdbNumEntries;
    devObjPtr->supportEqEgressMonitoringNumPorts = 32;

    devObjPtr->isMsmMibOnAllPorts = 1;
    devObjPtr->unitBaseAddrAlignmentNumBits = 24;

    SMEM_CHT_IS_DFX_SERVER(devObjPtr) = GT_TRUE;

    /* The device supports split designated trunk tables */
    devObjPtr->supportSplitDesignatedTrunkTable = 1;

    /* The device supports for IEEE 802.1BR for 'Port Extender' */
    devObjPtr->support802_1br_PortExtender = 1;

    /* init specific functions array */
    smemXCat3InitFuncArray(devObjPtr, commonDevMemInfoPtr);

    /* allocate address type specific memories */
    smemXCat3AllocSpecMemory(devObjPtr);

    /* Set the pointer to the active memory */
    devObjPtr->activeMemPtr = smemXCat3ActiveTable;

    devObjPtr->devFindMemFunPtr = (void *)smemXCat3FindMem;

    devObjPtr->memUnitBaseAddrInfo.policer[0] = policerBaseAddr[0];
    devObjPtr->memUnitBaseAddrInfo.policer[1] = policerBaseAddr[1];
    devObjPtr->memUnitBaseAddrInfo.policer[2] = policerBaseAddr[2];

    devObjPtr->memUnitBaseAddrInfo.lms[0] = 0x07000000;

    for(ii = 0 ; ii < sizeof(smemXCat3ActiveTable) / sizeof(smemXCat3ActiveTable[0]) ; ii++)
    {
        if(smemXCat3ActiveTable[ii].mask ==
            POLICER_ACTIVE_MEMORY_PLACE_HOLDER_MASK_CNS)
        {
            GT_U32  tmpMask = SMEM_FULL_MASK_CNS;

            if(smemXCat3ActiveTable[ii + 0].address == POLICER_MANAGEMENT_COUNTER_ADDR_CNS)
            {
                /* management counters */
                /* 0x40 between sets , 0x10 between counters */
                /* 0x00 , 0x10 , 0x20 , 0x30 */
                /* 0x40 , 0x50 , 0x60 , 0x70 */
                /* 0x80 , 0x90 , 0xa0 , 0xb0 */
                tmpMask = POLICER_MANAGEMENT_COUNTER_MASK_CNS;
            }

            smemXCat3ActiveTable[ii + 0].address += policerBaseAddr[0];
            smemXCat3ActiveTable[ii + 0].mask = tmpMask;

            smemXCat3ActiveTable[ii + 1].address += policerBaseAddr[1];
            smemXCat3ActiveTable[ii + 1].mask = tmpMask;

            smemXCat3ActiveTable[ii + 2].address += policerBaseAddr[2];
            smemXCat3ActiveTable[ii + 2].mask = tmpMask;

            ii += 2;/* the jump of 2 here + the one of the loop will
                       jump to next 3 policer if exists */
        }
    }

    devObjPtr->cpuGopPortNumber = 31;
    smemXCat3ForcedPhysicalToVirtualPortMapping(devObjPtr);

    smemXCat3RegsInfoSet(devObjPtr);

    devObjPtr->myInterruptsDbPtr = xCat3InterruptsTreeDb;

    smemXCat3TableInfoSet(devObjPtr);

    /* bind now tables to memories --
       MUST be done after calling smemXCat3TableInfoSet(...)
       so the tables can override previous settings */
    smemBindTablesToMemories(devObjPtr,
        /* array of x units !!! (the units are not set as the first units only , but can be spread)*/
        devMemInfoPtr->unitMemArr,
        SMEM_CHT_NUM_UNITS_MAX_CNS);

    smemXCat3TableInfoSetPart2(devObjPtr);

    /*
        bind 'non PP' per unit active memory to the unit.
        the memory spaces are : 'PEX' and 'DFX' (and 'MBUS')
    */
    smemLion3BindNonPpPerUnitActiveMem(devObjPtr,devMemInfoPtr);

}

/*******************************************************************************
*   smemXCat3Init2
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
void smemXCat3Init2
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    /* GT_U32 fieldVal; */

    smemChtInit2(devObjPtr);

    /* Do specific init for XCat3 */

    return;
}
