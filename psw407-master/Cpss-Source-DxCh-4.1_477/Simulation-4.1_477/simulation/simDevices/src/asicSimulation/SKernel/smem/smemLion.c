/******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* smemLion.c
*
* DESCRIPTION:
*       Lion memory mapping implementation: the 'port group' and the shared memory
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 79 $
*
*******************************************************************************/
#include <asicSimulation/SInit/sinit.h>
#include <asicSimulation/SKernel/smem/smemLion.h>
#include <asicSimulation/SKernel/cheetahCommon/sregCheetah.h>
#include <asicSimulation/SKernel/suserframes/snetCheetah3Pcl.h>
#include <asicSimulation/SKernel/suserframes/snetLion.h>
#include <asicSimulation/SKernel/suserframes/snetXCatPcl.h>
#include <asicSimulation/SKernel/suserframes/snetCheetahTxQ.h>

/* the port group# that used by CPU to access the shared memory -->
   the PEX address is the same one  */
#define PORT_GROUP_ID_FOR_PORT_GROUP_SHARED_ACCESS_CNS  0

/*write active memory*/

#define LION_IP_ROUT_TCAM_CNS   16*1024

static SKERNEL_DEVICE_ROUTE_TCAM_INFO_STC  lionPortGroupRoutTcamInfo =
{
    LION_IP_ROUT_TCAM_CNS,
    LION_IP_ROUT_TCAM_CNS / 4,
    1,
    4,
    16
};

/* MAC Counters address mask */
#define     SMEM_LION_XG_MIB_COUNT_MSK_CNS              0xff81ff00


/* Active memory table */
static SMEM_ACTIVE_MEM_ENTRY_STC smemPortGroupActiveTable[] =
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

    /* Set Incoming Packet Count */
    {0x020400E0, 0xFFFFFFF0, smemChtActiveReadCntrs, 0, NULL,0}, /* Set #0 counters */
    {0x020400F4, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0}, /* Set #1 Incoming counters */
    {0x020400F8, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0}, /* Set #1 VLAN ingress filtered counters */
    {0x020400FC, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0}, /* Set #1 Security filtered counters */
    {0x02040100, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0}, /* Set #1 Bridge filtered counters */

    /* XG port MAC MIB Counters */
    {0x09000000, SMEM_LION_XG_MIB_COUNT_MSK_CNS, smemChtActiveReadCntrs, 0, NULL,0},

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
    {0x01cc0000, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteXSmii, 0},

    /* Ports VLAN, QoS and Protocol Access Control Register */
    {0x0B800328, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteVlanQos, 0},

    /* Action Table and Policy TCAM Access Control Register */
    {0x0D000138, SMEM_FULL_MASK_CNS, NULL, 0 , smemCht3ActiveWritePclAction, 0},

    /* Router and TT TCAM Access Control 1 Register  */
    {0x0D80041C, SMEM_FULL_MASK_CNS, NULL, 0 , smemCht3ActiveWriteRouterAction, 0},

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
    {0x0B000020, SMEM_FULL_MASK_CNS, smemChtActiveReadIntrCauseReg, 28 , smemChtActiveWriteIntrCauseReg,0},

    /* VLT Tables Access Control Register */
    {0x0A000118, SMEM_FULL_MASK_CNS, NULL, 0 , smemXcatA1ActiveWriteVlanTbl, 0},

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
    {0x08800000, 0xFFFF03FF, NULL, 0, smemChtActiveWriteForceLinkDownXg, 0},
    /* HyperG.Stack Ports MIB Counters and XSMII Configuration Register */
    {0x08800030, 0xFFFF03FF, NULL, 0, smemChtActiveWriteHgsMibCntCtrl, 0},
    /* XG Port<n> Interrupt Cause Register  */
    {0x08800014, 0xFFFF03FF,  smemChtActiveReadIntrCauseReg, 20, smemChtActiveWriteIntrCauseReg, 0},
    /* XG Port<n> Interrupt Mask Register */
    {0x08800018, 0xFFFF03FF, NULL, 0, smemChtActiveWritePortInterruptsMaskReg, 0},
    /* stack gig ports - Port<n> Interrupt Cause Register  */
    {0x08800020, 0xFFFF03FF,smemChtActiveReadIntrCauseReg, 18, smemChtActiveWriteIntrCauseReg, 0},
    /* stack gig ports - Tri-Speed Port<n> Interrupt Mask Register */
    {0x08800024, 0xFFFF03FF, NULL, 0, smemChtActiveWritePortInterruptsMaskReg, 0},
    /* stack gig ports - Port<n> Auto-Negotiation Configuration Register */
    {0x0880000C, 0xFFFF03FF, NULL, 0, smemChtActiveWriteForceLinkDown, 0},
    /*Port MAC Control Register3*/
    {0x0880001C, 0xFFFF03FF, NULL, 0, smemXcatActiveWriteMacModeSelect, 0},

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
    {0x06000000, SMEM_FULL_MASK_CNS, NULL, 0 , smemCht3ActiveWriteFDBGlobalCfgReg, 0},

    /* Transmit Queue Control Register */
    {0x01800000, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteTxQuCtrlReg, 0},

    /* PEX Interrupt Cause Register */
    {0x00071900, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteIntrCauseReg, 0x00FF1F1F},

    /* read interrupts cause registers CNC -- ROC register */
    {0x08000100, SMEM_FULL_MASK_CNS,
        smemChtActiveReadIntrCauseReg, 3, smemChtActiveWriteIntrCauseReg, 0},

    /* Write Interrupt Mask CNC Register */
    {0x08000104, SMEM_FULL_MASK_CNS, NULL, 0 , smemCht3ActiveWriteCncInterruptsMaskReg, 0},

    /* Policer : when Read a Data Unit counter part(Entry LSB) of Management Counters Entry.
                 then value of LSB and MSB copied to Shadow registers */
    {0x0C0C0000, CH3_POLICER_MANAGEMENT_COUNTER_MASK_CNS, smemXCatActiveReadPolicerManagementCounters, 0 , NULL, 0},

    /* SMI0 Management Register : 0x04004054  */
    {0x04004054, 0xFFFFFFFF, NULL, 0 , smemChtActiveWriteSmi,0},

    /* CPU Port MIB counters registers */
    {0x00000060, 0xFFFFFFE0, smemChtActiveReadCntrs, 0, NULL,0},

    /* must be last anyway */
    {END_OF_TABLE, SMEM_FULL_MASK_CNS, NULL,0,NULL,0}
};

static GT_U32 policerBaseAddr[3] = {0x0C000000,0x00800000,0x07800000};
/* define policer memory with 1792 entries */
#define POLICER_MEMORY_1792_CNS         1792
/* define policer memory with 256 entries */
#define POLICER_MEMORY_256_CNS         256

/* Timestamp ingress and egress base addresses */
static GT_U32 timestampBaseAddr[2] = {0x01002000,0x0E800800};

/* Active memory table */
static SMEM_ACTIVE_MEM_ENTRY_STC smemLionB0PortGroupActiveTable[] =
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

    /* Set Incoming Packet Count */
    {0x020400E0, 0xFFFFFFF0, smemChtActiveReadCntrs, 0, NULL,0}, /* Set #0 counters */
    {0x020400F4, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0}, /* Set #1 Incoming counters */
    {0x020400F8, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0}, /* Set #1 VLAN ingress filtered counters */
    {0x020400FC, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0}, /* Set #1 Security filtered counters */
    {0x02040100, SMEM_FULL_MASK_CNS, smemChtActiveReadCntrs, 0, NULL,0}, /* Set #1 Bridge filtered counters */

    /* XG port MAC MIB Counters */
    {0x09000000, SMEM_LION_XG_MIB_COUNT_MSK_CNS, smemChtActiveReadCntrs, 0, NULL,0},

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
    {0x0A093210, 0xFFFFFFF0, smemChtActiveReadCntrs, 0, NULL,0},
    {0x0A093220, 0xFFFFFFF0, smemChtActiveReadCntrs, 0, NULL,0},
    {0x0A093230, 0xFFFFFFF0, smemChtActiveReadCntrs, 0, NULL,0},
    {0x0A093240, 0xFFFFFFF0, smemChtActiveReadCntrs, 0, NULL,0},
    {0x0A093250, 0xFFFFFFF0, smemChtActiveReadCntrs, 0, NULL,0},
    {0x0A093260, 0xFFFFFFF0, smemChtActiveReadCntrs, 0, NULL,0},
    {0x0A093270, 0xFFFFFFF0, smemChtActiveReadCntrs, 0, NULL,0},
    {0x0A093280, 0xFFFFFFF0, smemChtActiveReadCntrs, 0, NULL,0},

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

    /* Policer: when write iplr0 control0 register, "Policer memory control" configuration is
                            changed ang ingress policer tables are redistributed between plr0,1 accordingly */
    {0x0C000000, SMEM_FULL_MASK_CNS, NULL, 0 , smemXCatActiveWritePolicerMemoryControl,  0},

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
    {0x0B000020, SMEM_FULL_MASK_CNS, smemChtActiveReadIntrCauseReg, 24 , smemChtActiveWriteIntrCauseReg,0},

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
    {0x08800014, 0xFFFF03FF,  smemChtActiveReadIntrCauseReg, 20, smemChtActiveWriteIntrCauseReg, 0},
    /* XG Port<n> Interrupt Mask Register */
    {0x08800018, 0xFFFF03FF, NULL, 0, smemChtActiveWritePortInterruptsMaskReg, 0},
    /* stack gig ports - Port<n> Interrupt Cause Register  */
    {0x08800020, 0xFFFF03FF,smemChtActiveReadIntrCauseReg, 18, smemChtActiveWriteIntrCauseReg, 0},
    /* stack gig ports - Tri-Speed Port<n> Interrupt Mask Register */
    {0x08800024, 0xFFFF03FF, NULL, 0, smemChtActiveWritePortInterruptsMaskReg, 0},
    /* stack gig ports - Port<n> Auto-Negotiation Configuration Register */
    {0x0880000C, 0xFFFF03FF, NULL, 0 , smemChtActiveWriteForceLinkDown, 0},
    /*Port MAC Control Register3*/
    {0x0880001C, 0xFFFF03FF, NULL, 0 , smemXcatActiveWriteMacModeSelect, 0},

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
    {0x06000000, SMEM_FULL_MASK_CNS, NULL, 0 , smemCht3ActiveWriteFDBGlobalCfgReg, 0},

    /* Scheduler Configuration Register */
    {0x11001000, SMEM_FULL_MASK_CNS, NULL, 0 , smemLionActiveWriteSchedulerConfigReg, 0},

    /* Dequeue enable Register */
    {0x11002000, 0xFFFFFF00, NULL, 0, smemLionActiveWriteDequeueEnableReg, 0},

    /* PEX Interrupt Cause Register */
    {0x00071900, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteIntrCauseReg, 0x00FF1F1F},

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

    /* CPU Port MIB counters registers */
    {0x00000060, 0xFFFFFFE0, smemChtActiveReadCntrs, 0, NULL,0},

    /* must be last anyway */
    {END_OF_TABLE, SMEM_FULL_MASK_CNS, NULL,0,NULL,0}
};

static GT_U32   smemLionB0PortGroupActiveTableSizeOf =
    sizeof(smemLionB0PortGroupActiveTable)/sizeof(smemLionB0PortGroupActiveTable[0]);

/* Active memory table -- for the port group Shared memory */
static SMEM_ACTIVE_MEM_ENTRY_STC smemLionPortGroupSharedActiveTable[] =
{
    /* must be last anyway */
    {END_OF_TABLE, SMEM_FULL_MASK_CNS, NULL,0,NULL,0}
};
/* declare index of the unit */
#define STATE_ID_OF_UNIT_MAC(unitName,unitIndex)    \
    static GT_U32   unitName##_UnitId = unitIndex

/* get the index of the unit */
#define GET_UNIT_INDEX_MAC(unitName)                \
    (unitName##_UnitId)

STATE_ID_OF_UNIT_MAC(MG_UnitMem,0);
STATE_ID_OF_UNIT_MAC(IPLR1_UnitMem,1);
STATE_ID_OF_UNIT_MAC(TTI_UnitMem,2);
STATE_ID_OF_UNIT_MAC(TXQ_EGR_UnitMem,3);
STATE_ID_OF_UNIT_MAC(Egress_and_Txq_UnitMem,3); /* in Lion_A */
STATE_ID_OF_UNIT_MAC(L2I_UnitMem,4);
STATE_ID_OF_UNIT_MAC(IPvX_UnitMem,5);
STATE_ID_OF_UNIT_MAC(BM_UnitMem,6);
STATE_ID_OF_UNIT_MAC(BMA_UnitMem,7);/* shared unit */
STATE_ID_OF_UNIT_MAC(LMS_UnitMem,8);
STATE_ID_OF_UNIT_MAC(FDB_UnitMem,12);
STATE_ID_OF_UNIT_MAC(MPPM_UnitMem,13);
STATE_ID_OF_UNIT_MAC(CTU_UnitMem,14);
STATE_ID_OF_UNIT_MAC(EPLR_UnitMem,15);
STATE_ID_OF_UNIT_MAC(HA_and_EPCL_UnitMem,15);/* in Lion_A */
STATE_ID_OF_UNIT_MAC(Centralized_Counters_UnitMem,16);
STATE_ID_OF_UNIT_MAC(XGPorts_UnitMem,17);
STATE_ID_OF_UNIT_MAC(XGPort_MIB_UnitMem,18);
STATE_ID_OF_UNIT_MAC(LP_SERDES_UnitMem,19);
STATE_ID_OF_UNIT_MAC(TXQ_Queue_UnitMem,20);
STATE_ID_OF_UNIT_MAC(_VLAN_UnitMem,20);/* in Lion_A */
STATE_ID_OF_UNIT_MAC(GigPorts_UnitMem,21);
STATE_ID_OF_UNIT_MAC(Eq_UnitMem,22);
STATE_ID_OF_UNIT_MAC(IPCL_UnitMem,23);
STATE_ID_OF_UNIT_MAC(IPLR_UnitMem,24);
STATE_ID_OF_UNIT_MAC(MLL_UnitMem,25);
STATE_ID_OF_UNIT_MAC(IPCL_TCC_UnitMem,26);
STATE_ID_OF_UNIT_MAC(IPvX_TCC_UnitMem,27);
STATE_ID_OF_UNIT_MAC(EPCL_UnitMem,28);
STATE_ID_OF_UNIT_MAC(HA_UnitMem,29);
STATE_ID_OF_UNIT_MAC(RX_DMA_UnitMem,30);
STATE_ID_OF_UNIT_MAC(TX_DMA_UnitMem,31);
STATE_ID_OF_UNIT_MAC(_MPPM_UnitMem,32);
STATE_ID_OF_UNIT_MAC(TXQ_LL_UnitMem,33);
STATE_ID_OF_UNIT_MAC(TXQ_DQ_UnitMem,34);
STATE_ID_OF_UNIT_MAC(TXQ_SHT_UnitMem,35);

/* the units that the Lion A0 port group use */
static GT_U32   usedUnitsNumbersArray[]=
{
/*    SMEM_UNIT_CHUNKS_STC   MG_UnitMem;                   0x00000000 */  0 ,
/*    SMEM_UNIT_CHUNKS_STC   Device_SMI_UnitMem;
                             SMI mem not simulated         0x00000000 */  /*unit#??  */
/*    SMEM_UNIT_CHUNKS_STC   Egress_and_Txq_UnitMem;       0x01800000 */  3 ,
/*    SMEM_UNIT_CHUNKS_STC   L2I_UnitMem;                  0x02000000 */  4 ,
/*    SMEM_UNIT_CHUNKS_STC   IPvX_UnitMem;                 0x02800000 */  5 ,
/*    SMEM_UNIT_CHUNKS_STC   BM_UnitMem;                   0x03000000 */  6 ,
/*    SMEM_UNIT_CHUNKS_STC   BMA_UnitMem;                  0x03800000 */ /* 7 , shared memory */
/*    SMEM_UNIT_CHUNKS_STC   LMS_UnitMem;                  0x04000000 */  8/*9,10,11*/ ,
/*    SMEM_UNIT_CHUNKS_STC   FDB_UnitMem;                  0x06000000 */  12,
/*    SMEM_UNIT_CHUNKS_STC   MPPM_UnitMem;                 0x06800000 */  13,
/*    SMEM_UNIT_CHUNKS_STC   HA_and_EPCL_UnitMem;          0x07800000 */  15,
/*    SMEM_UNIT_CHUNKS_STC   MEM_UnitMem;                  0x07800000 */  /*15,*/
/*    SMEM_UNIT_CHUNKS_STC   TXDMA_UnitMem;                0x07800000 */  /*15,*/
/*    SMEM_UNIT_CHUNKS_STC   Centralized_Counters_UnitMem; 0x08000000 */  16,
/*    SMEM_UNIT_CHUNKS_STC   XGPorts_UnitMem;              0x08800000 */  17,
/*    SMEM_UNIT_CHUNKS_STC   XGPort_MIB_UnitMem;           0x09000000 */  18,
/*    SMEM_UNIT_CHUNKS_STC   LP_SERDES_UnitMem;            0x09800000 */  19,
/*--> SMEM_UNIT_CHUNKS_STC   _VLAN_UnitMem;                0x0A000000 */  20,
/*    SMEM_UNIT_CHUNKS_STC   GigPorts_UnitMem;             0x0A800000 */  21,
/*    SMEM_UNIT_CHUNKS_STC   Eq_UnitMem;                   0x0B000000 */  22,
/*    SMEM_UNIT_CHUNKS_STC   IPCL_UnitMem;                 0x0B800000 */  23,
/*    SMEM_UNIT_CHUNKS_STC   IPLR_UnitMem;                 0x0C000000 */  24,
/*    SMEM_UNIT_CHUNKS_STC   MLL_UnitMem;                  0x0C800000 */  25,
/*    SMEM_UNIT_CHUNKS_STC   IPCL_TCC_UnitMem;             0x0D000000 */  26,
/*    SMEM_UNIT_CHUNKS_STC   IPvX_TCC_UnitMem;             0x0D800000 */  27,
/*--> SMEM_UNIT_CHUNKS_STC   _MPPM_UnitMem;                0x10000000 */  32,


/* PEX - must be after all regular units */
/*    SMEM_UNIT_CHUNKS_STC   PEX_UnitMem;                  0x00070000 */  /*unit#0   */

    END_OF_TABLE /* must be last*/
};

/* (no casting) Return unit chunk pointer by real unit index */
#define UNIT_CHUNK_PTR_NO_CAST_MAC(dev_info, unit) \
    (&dev_info->unitMemArr[GET_UNIT_INDEX_MAC(unit)])

/* Return unit chunk pointer by real unit index */
#define UNIT_CHUNK_PTR_UINTPTR_CAST_MAC(dev_info, unit) \
    ((GT_UINTPTR)UNIT_CHUNK_PTR_NO_CAST_MAC(dev_info, unit))


/* the units that are shared between all Lion port groups */
static GT_U32   usedPortGroupSharedUnitsNumbersArray[]=
{
/*    SMEM_UNIT_CHUNKS_STC   BMA_UnitMem;                  0x03800000 */  7 , /*shared memory */
    END_OF_TABLE /* must be last*/
};

/*******************************************************************************
*   smemLionSubUnitMemoryGet
*
* DESCRIPTION:
*       Get the port-group object for specific sub-unit.

* INPUTS:
*       deviceObj   - pointer to device object.
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
static SKERNEL_DEVICE_OBJECT * smemLionSubUnitMemoryGet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32                  address
)
{
    GT_U32 index;           /* Memory unit index */
    SKERNEL_DEVICE_OBJECT * sharedObjPtr;

    SMEM_CHT_GENERIC_DEV_MEM_INFO  * devMemInfoPtr = devObjPtr->deviceMemory;

    /* Device is not port group */
    if(devObjPtr->portGroupSharedDevObjPtr == 0)
    {
        /* Return pointer to original object */
        return devObjPtr;
    }

    /* Get memory subunit index */
    index =
        (address & REG_SPEC_FUNC_INDEX) >> SMEM_CHT_UNIT_INDEX_FIRST_BIT_CNS;

    /* Get pointer to shared device */
    sharedObjPtr = devObjPtr->portGroupSharedDevObjPtr;

    /* Device is port group and accessed from SKERNEL scope */
    if (SMEM_ACCESS_SKERNEL_FULL_MAC(accessType))
    {
        if(devMemInfoPtr->common.specFunTbl[index].specParam  ==
           UNIT_CHUNK_PTR_UINTPTR_CAST_MAC(devMemInfoPtr, TXQ_SHT_UnitMem))
        {
         /* Get object pointer for subunit - pair of port groups.
            The unit exists only in port group 0, 1(so port groups 0, 3 access port group 0
            and port groups 1, 2 access port group 3) */
            if (devObjPtr->portGroupId == 0 || devObjPtr->portGroupId == 3)
            {
                devObjPtr = sharedObjPtr->coreDevInfoPtr[0].devObjPtr;
            }
            else
            {
                devObjPtr = sharedObjPtr->coreDevInfoPtr[1].devObjPtr;
            }
        }
        else
        if(devMemInfoPtr->common.specFunTbl[index].specParam  ==
                UNIT_CHUNK_PTR_UINTPTR_CAST_MAC(devMemInfoPtr, TXQ_LL_UnitMem) ||
           devMemInfoPtr->common.specFunTbl[index].specParam  ==
                UNIT_CHUNK_PTR_UINTPTR_CAST_MAC(devMemInfoPtr, TXQ_Queue_UnitMem)
          )
        {
         /* Get object pointer for subunit - single instance of ports group 0.
            The unit exists only in port group 0 (so all port groups access to it) */
            devObjPtr = sharedObjPtr->coreDevInfoPtr[0].devObjPtr;
        }
    }
    else
    {
        if(devMemInfoPtr->common.specFunTbl[index].specParam  ==
           UNIT_CHUNK_PTR_UINTPTR_CAST_MAC(devMemInfoPtr, TXQ_SHT_UnitMem))
        {
            if (devObjPtr->portGroupId != 0 && devObjPtr->portGroupId != 1)
            {
                /* The unit exists only in port group 0, 1 */
                skernelFatalError("smemLionSubUnitMemoryGet: illegal memory access - port group %d (only port groups 0 and 1 valid), sub-unit TXQ_SHT_UnitMem, address [0x%8.8x] \n",
                                  devObjPtr->portGroupId, address);
            }
        }
        else
        if(devMemInfoPtr->common.specFunTbl[index].specParam  ==
                UNIT_CHUNK_PTR_UINTPTR_CAST_MAC(devMemInfoPtr, TXQ_LL_UnitMem))
        {
            if (devObjPtr->portGroupId != 0)
            {
                /* The unit exists only in port group 0 */
                skernelFatalError("smemLionSubUnitMemoryGet: illegal memory access - port group %d (only port group 0 valid), sub-unit TXQ_LL_UnitMem, address [0x%8.8x] \n",
                                  devObjPtr->portGroupId, address);
            }
        }
        else
        if(devMemInfoPtr->common.specFunTbl[index].specParam  ==
                UNIT_CHUNK_PTR_UINTPTR_CAST_MAC(devMemInfoPtr, TXQ_Queue_UnitMem))
        {
            if (devObjPtr->portGroupId != 0)
            {
                /* The unit exists only in port group 0 */
                skernelFatalError("smemLionSubUnitMemoryGet: illegal memory access - port group %d (only port group 0 valid), sub-unit TXQ_Queue_UnitMem, address [0x%8.8x] \n",
                                  devObjPtr->portGroupId, address);
            }
        }
    }

    return devObjPtr;
}


/*******************************************************************************
*   smemLionPortGroupSharedFindInUnitChunk
*
* DESCRIPTION:
*       find the memory in the specified unit chunk -- in the shared port group
*       function called from the port group device and need to convert to the portGroupSharedDevObjPtr
*       device
*
* INPUTS:
*       devObjPtr   - pointer to !!! port group !!! device object.
*       unitChunksPtr - pointer to the unit chunk
*       address     - address of memory(register or table).
*       memSize     - size of the requested memory
*       param       - extra parameter --> used as pointer to the memory unit chunk
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
static GT_U32 *  smemLionPortGroupSharedFindInUnitChunk
(
    IN SKERNEL_DEVICE_OBJECT *devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    return smemDevFindInUnitChunk(devObjPtr->portGroupSharedDevObjPtr,/* convert the port group device to the shared port group device */
                                   accessType, address, memSize, param);
}

/*******************************************************************************
*   smemLionB0InitFuncArray
*
* DESCRIPTION:
*       Init specific Lion B0 functions array.
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
static void smemLionB0InitFuncArray
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_CHT_DEV_COMMON_MEM_INFO  * commonDevMemInfoPtr
)
{
    SMEM_CHT_GENERIC_DEV_MEM_INFO * devMemInfoPtr = devObjPtr->deviceMemory;

    /* set the parameter to be (casting of) the pointer to the unit chunk */
    commonDevMemInfoPtr->specFunTbl[1].specParam  =
        UNIT_CHUNK_PTR_UINTPTR_CAST_MAC(devMemInfoPtr, IPLR1_UnitMem);
    commonDevMemInfoPtr->specFunTbl[1].specFun    = smemDevFindInUnitChunk;

    commonDevMemInfoPtr->specFunTbl[2].specParam  =
        UNIT_CHUNK_PTR_UINTPTR_CAST_MAC(devMemInfoPtr, TTI_UnitMem);
    commonDevMemInfoPtr->specFunTbl[2].specFun    = smemDevFindInUnitChunk;

    commonDevMemInfoPtr->specFunTbl[3].specParam  =
        UNIT_CHUNK_PTR_UINTPTR_CAST_MAC(devMemInfoPtr, TXQ_EGR_UnitMem);
    commonDevMemInfoPtr->specFunTbl[3].specFun    = smemDevFindInUnitChunk;

    commonDevMemInfoPtr->specFunTbl[14].specParam  =
        UNIT_CHUNK_PTR_UINTPTR_CAST_MAC(devMemInfoPtr, CTU_UnitMem);
    commonDevMemInfoPtr->specFunTbl[14].specFun    = smemDevFindInUnitChunk;

    commonDevMemInfoPtr->specFunTbl[15].specParam  =
        UNIT_CHUNK_PTR_UINTPTR_CAST_MAC(devMemInfoPtr, EPLR_UnitMem);
    commonDevMemInfoPtr->specFunTbl[15].specFun    = smemDevFindInUnitChunk;

    commonDevMemInfoPtr->specFunTbl[20].specParam  =
        UNIT_CHUNK_PTR_UINTPTR_CAST_MAC(devMemInfoPtr, TXQ_Queue_UnitMem);
    commonDevMemInfoPtr->specFunTbl[20].specFun    = smemDevFindInUnitChunk;

    commonDevMemInfoPtr->specFunTbl[28].specParam  =
        UNIT_CHUNK_PTR_UINTPTR_CAST_MAC(devMemInfoPtr, EPCL_UnitMem);
    commonDevMemInfoPtr->specFunTbl[28].specFun    = smemDevFindInUnitChunk;

    commonDevMemInfoPtr->specFunTbl[29].specParam  =
        UNIT_CHUNK_PTR_UINTPTR_CAST_MAC(devMemInfoPtr, HA_UnitMem);
    commonDevMemInfoPtr->specFunTbl[29].specFun    = smemDevFindInUnitChunk;

    commonDevMemInfoPtr->specFunTbl[30].specParam  =
        UNIT_CHUNK_PTR_UINTPTR_CAST_MAC(devMemInfoPtr, RX_DMA_UnitMem);
    commonDevMemInfoPtr->specFunTbl[30].specFun    = smemDevFindInUnitChunk;

    commonDevMemInfoPtr->specFunTbl[31].specParam  =
        UNIT_CHUNK_PTR_UINTPTR_CAST_MAC(devMemInfoPtr, TX_DMA_UnitMem);
    commonDevMemInfoPtr->specFunTbl[31].specFun    = smemDevFindInUnitChunk;

    commonDevMemInfoPtr->specFunTbl[33].specParam  =
        UNIT_CHUNK_PTR_UINTPTR_CAST_MAC(devMemInfoPtr, TXQ_LL_UnitMem);
    commonDevMemInfoPtr->specFunTbl[33].specFun    = smemDevFindInUnitChunk;

    commonDevMemInfoPtr->specFunTbl[34].specParam  =
        UNIT_CHUNK_PTR_UINTPTR_CAST_MAC(devMemInfoPtr, TXQ_DQ_UnitMem);
    commonDevMemInfoPtr->specFunTbl[34].specFun    = smemDevFindInUnitChunk;

    commonDevMemInfoPtr->specFunTbl[35].specParam  =
        UNIT_CHUNK_PTR_UINTPTR_CAST_MAC(devMemInfoPtr, TXQ_SHT_UnitMem);
    commonDevMemInfoPtr->specFunTbl[35].specFun    = smemDevFindInUnitChunk;
}

/*******************************************************************************
*   smemLionInitFuncArray
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
static void smemLionInitFuncArray
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_CHT_DEV_COMMON_MEM_INFO  * commonDevMemInfoPtr
)
{
    GT_U32              ii,jj,index;
    GT_U32              *usedUnitsPtr;/*pointer to the used units in the device*/
    SKERNEL_DEVICE_OBJECT *memoryDevObjPtr;/* pointer to the device that actually hold the memory*/
    SMEM_SPEC_MEMORY_FIND_FUN  specFun;
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr;

    /* set by default that all units bind to 'Fatal error' */
    for (ii = 0; ii < 64; ii++)
    {
        commonDevMemInfoPtr->specFunTbl[ii].specFun    = smemChtFatalError;
    }

    if(! devObjPtr->portGroupSharedDevObjPtr)
    {
        /* we not map the memory of the shared port group as a 'Stand alone' memory
           because it is accessed Only via port group #0 --> see PORT_GROUP_ID_FOR_PORT_GROUP_SHARED_ACCESS_CNS */
        return;
    }

    usedUnitsPtr = usedUnitsNumbersArray;
    specFun = smemDevFindInUnitChunk;
    memoryDevObjPtr = devObjPtr;
    devMemInfoPtr = devObjPtr->deviceMemory;

bindMemoryUnits_lbl:
    /* bind the units to the specific unit chunk with the generic function */
    for (ii = 0; usedUnitsPtr[ii] != END_OF_TABLE ; ii++)
    {
        /* use the unit ID as index in the array */
        jj = usedUnitsPtr[ii];

        if(specFun == smemLionPortGroupSharedFindInUnitChunk)
        {
            /* shared memory unit */
            index = jj;
            commonDevMemInfoPtr->specFunTbl[jj].specFun    = specFun;

            /* set the parameter to be (casting of) the pointer to the unit chunk */
            commonDevMemInfoPtr->specFunTbl[jj].specParam  =
                smemConvertChunkIndexToPointerAsParam(memoryDevObjPtr,index);
        }
        else
        {
            if(jj == 0)
            {
                /* unit that both for MG and for PEX */
                index = jj;

                /* NOTE: the PEX is no longer accessed via the MG unit !!!! */
                /* when devMemInfoPtr->PEX_UnitMem is used */
                /* so all units can get the generic function */
                commonDevMemInfoPtr->specFunTbl[jj].specFun    = specFun;

                /* set the parameter to be (casting of) the pointer to the unit chunk */
                commonDevMemInfoPtr->specFunTbl[jj].specParam  =
                    smemConvertChunkIndexToPointerAsParam(memoryDevObjPtr,index);
            }
            else
            {
                commonDevMemInfoPtr->specFunTbl[jj].specFun    = specFun;

                index = jj;

                /* set the parameter to be (casting of) the pointer to the unit chunk */
                commonDevMemInfoPtr->specFunTbl[jj].specParam  =
                    smemConvertChunkIndexToPointerAsParam(memoryDevObjPtr,index);

                /* the LMS unit actually take 4 unit space */
                if(commonDevMemInfoPtr->specFunTbl[jj].specParam ==
                    UNIT_CHUNK_PTR_UINTPTR_CAST_MAC(devMemInfoPtr, LMS_UnitMem))
                {
                    commonDevMemInfoPtr->specFunTbl[jj+1].specFun    =
                    commonDevMemInfoPtr->specFunTbl[jj+2].specFun    =
                    commonDevMemInfoPtr->specFunTbl[jj+3].specFun    =
                    commonDevMemInfoPtr->specFunTbl[jj].specFun;

                    /* set the parameter to be (casting of) the pointer to the unit chunk */
                    commonDevMemInfoPtr->specFunTbl[jj+1].specParam  =
                    commonDevMemInfoPtr->specFunTbl[jj+2].specParam  =
                    commonDevMemInfoPtr->specFunTbl[jj+3].specParam  =
                    commonDevMemInfoPtr->specFunTbl[jj].specParam  ;
                }
            }
        }
    }

    if(SKERNEL_IS_LION_REVISON_B0_DEV(devObjPtr))
    {
        smemLionB0InitFuncArray(devObjPtr, commonDevMemInfoPtr);
    }

    if(devObjPtr->portGroupId == PORT_GROUP_ID_FOR_PORT_GROUP_SHARED_ACCESS_CNS)
    {
        /* we need to reflect the memory of the port group shared via the selected port group */
        if(memoryDevObjPtr != devObjPtr->portGroupSharedDevObjPtr) /*a check to prevent endless loop */
        {
            /* the port group shared units to reflect */
            usedUnitsPtr = usedPortGroupSharedUnitsNumbersArray;
            /* the function in the port group that will be called in order to
               access shared units */
            specFun = smemLionPortGroupSharedFindInUnitChunk;
            /* the device of the port group shared that the units belongs to */
            memoryDevObjPtr = devObjPtr->portGroupSharedDevObjPtr;

            devMemInfoPtr = NULL;

            goto bindMemoryUnits_lbl;
        }
    }
}

/*******************************************************************************
*   smemLionFindMem
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
static void * smemLionFindMem
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32                  address,
    IN GT_U32                  memSize,
    OUT SMEM_ACTIVE_MEM_ENTRY_STC ** activeMemPtrPtr
)
{
    if(SKERNEL_IS_LION_REVISON_B0_DEV(devObjPtr))
    {
        /* Return pointer to object for specific memory subunit */
        devObjPtr = smemLionSubUnitMemoryGet(devObjPtr, accessType, address);
    }

    return smemGenericFindMem(devObjPtr, accessType, address, memSize,
                                  activeMemPtrPtr);
}


/*******************************************************************************
*   smemLionUnitMg
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  MG unit
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
static void smemLionUnitMg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000000 ,0x000000fc)}  /* Global Configuration */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000364 ,0x0000036c)}  /* Address Decoding  */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002600 ,0x00002880)}  /* SDMA */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00400000 ,0x0040001C)}  /* TWSI */
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
}

/*******************************************************************************
*   smemLionB0UnitMg
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Lion B0 MG unit
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
static void smemLionB0UnitMg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    /* Allocate Lion A0 memory first */
    smemLionUnitMg(devObjPtr, unitPtr);
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            /* MG Miscellaneous */
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000370 ,0x00000398)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000668 ,0x00000668)}
        };
        GT_U32 numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

        smemInitMemChunk(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitPtr, &tmpUnitChunk);
    }
}

/*******************************************************************************
*   smemLionB0UnitPolicerUnify
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
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemLionB0UnitPolicerUnify
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr,
    IN GT_U32   baseAddr
)
{
    /* chunks with flat memory (no formulas) */
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000000 , 0x0000003C )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000048 , 0x00000054 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000100 , 0x00000110 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000120 , 0x00000124 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00000200 , 36)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00000400 , 96)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00000500 , 192), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(74, 16)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00000800 , 256)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00000900 , 256)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00001000 , 2048)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00040000 , 131072/2), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(229, 32)}/* error in cider */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00060000 , 131072/2), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(229, 32)}/* error in cider */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00080000 , 2560), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(14, 8)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);
        GT_U32  ii;

        for(ii = 0 ; ii < numOfChunks ; ii ++)
        {
            chunksMem[ii].memFirstAddr += baseAddr;
        }

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }

    /* chunks with formulas */
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
             {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00000070 ,0)}, FORMULA_SINGLE_PARAMETER(9 , 0x4)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x000000C0 ,0)}, FORMULA_SINGLE_PARAMETER(8 , 0x4)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00001800 ,0)}, FORMULA_SINGLE_PARAMETER(32 , 0x4)}

        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;
        GT_U32  ii;

        for(ii = 0 ; ii < numOfChunks ; ii ++)
        {
            chunksMem[ii].memChunkBasic.memFirstAddr += baseAddr;
        }

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);
    }
}

/*******************************************************************************
*   smemLionB0UnitIplr1
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
static void smemLionB0UnitIplr1
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    smemLionB0UnitPolicerUnify(devObjPtr,unitPtr,policerBaseAddr[1]);
}

/*******************************************************************************
*   smemLionB0UnitTti
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
static void smemLionB0UnitTti
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
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01000098 ,0x0100009C )}
            /* Protocol Match */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x01800080 , 4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x01800090 , 4)}
            /* User Defined Bytes */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x018000A0 , 4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x018000B0 , 4)}
            /* 0x01000190 */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01000190 ,0x010001AC )}
            /* 0x010001BC */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x010001BC , 4)}
            /* 0x010001C0 */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x010001C0 ,0x010001C8 )}
           /* TTI Internal configurations */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01000200, 0x01000204)}
            /* Received Flow Control Packets Counter and Dropped Flow Control Packets Counter */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x010014E0 ,0x010014E4 )}
            /* Port VLAN and QoS Configuration Table */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x01001000 , 1024), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(71, 16)}
            /* TTI DFX Area */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x01001800 , 2048)}
            /* ITS address space */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x01002000 , 2048)}
            /* Port Protocol VID and QoS Configuration Table */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x01004000 , 8192), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(28, 8)}
            /* VLAN Translation Table */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0100C000 , 16384)}
            /* User Defined Bytes Configuration Memory */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x01010000 , 384), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(255, 32)}
            /* MAC2ME TCAM Registers */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC(0x01001700  ,0x0100171c)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }

    /* chunks with formulas */
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
             {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01000080 ,0)}, FORMULA_SINGLE_PARAMETER(6 , 0x4)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x010000A0 ,0)}, FORMULA_SINGLE_PARAMETER(8 , 0x4)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x010000C0 ,0)}, FORMULA_SINGLE_PARAMETER(8 , 0x4)}
            /* VLAN Assignment */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01000300 ,0)}, FORMULA_SINGLE_PARAMETER(4 , 0x4)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01000310 ,0)}, FORMULA_SINGLE_PARAMETER(32 , 0x4)}
            /* QoS Mapping Tables */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01000400 ,0)}, FORMULA_SINGLE_PARAMETER(16 , 0x4)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01000440 ,0)}, FORMULA_THREE_PARAMETERS(2,0x4, 2,0x8, 2,0x10)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01000460 ,0)}, FORMULA_SINGLE_PARAMETER(2 , 0x4)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01000470 ,0)}, FORMULA_SINGLE_PARAMETER(16 , 0x4)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01000900 ,0)}, FORMULA_SINGLE_PARAMETER(16 , 0x4)}
            /* TTI Descriptor to PCL Word<%n> Register */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01001400 ,0)}, FORMULA_SINGLE_PARAMETER(38 , 0x4)}
            /* Wr_DMA Descriptor to TTI Word<%n> Register */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01001498 ,0)}, FORMULA_SINGLE_PARAMETER(2 , 0x4)}
            /* MAC2ME Registers */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01001600 ,0)}, FORMULA_TWO_PARAMETERS(4, 0x4 , 8 , 0x10)}
            /* User Defined Ethertypes and Port Group Ring Port Enable */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x010000F0  ,0)}, FORMULA_SINGLE_PARAMETER(4 , 0x4)}

        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);
    }
}

/*******************************************************************************
*   smemLionB0UnitTxqEgr
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  TXQ_EGR unit
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
static void smemLionB0UnitTxqEgr
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    /* chunks with flat memory (no formulas) */
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            /* base of 0x01800000 (Global)*/
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01800000, 0x01800000)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01801000, 0x01801000)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01801100, 0x01801100)}
            /* Secondary Target Port Table */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x01801300, 256)}
            /* Egress Filters Global Enables */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01810000, 0x01810000)}
            /* Designated Port Table */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x01810200, 512), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(64, 8) }
            /* Source ID Members Table */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x01810500, 256), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(64, 8)}
            /* Non Trunk Members Table */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x01810800, 1024), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(64, 8)}
            /* EGR RAMS TEST CTR */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01800800, 0x01800810)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }

    /* chunks with formulas */
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
            /* Global */
             {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01800080 ,0)}, FORMULA_SINGLE_PARAMETER(4 , 0x10)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00800070 ,0)}, FORMULA_SINGLE_PARAMETER(9 , 0x4)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x008000C0 ,0)}, FORMULA_SINGLE_PARAMETER(8 , 0x4)}
            /* MC FIFO */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01801010 ,0)}, FORMULA_TWO_PARAMETERS(2 , 0x4, 2, 0x10)}
            /* Local Src/Trg Port Map Own Dev En */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01801110 ,0)}, FORMULA_TWO_PARAMETERS(2 , 0x4, 2, 0x10)}
            /* UN/BC/MC Egress Filters */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01810010 ,0)}, FORMULA_TWO_PARAMETERS(2 , 0x4, 9, 0x10)}

        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);
    }
}

/*******************************************************************************
*   smemLionUnitEgressAndTxq
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  EgressAndTxq unit
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
static void smemLionUnitEgressAndTxq
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    /* chunks with flat memory (no formulas) */
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            /* base of 0x01800000 */
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01800000 ,0x01800200 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01800240 ,0x01800264 )}

            /* base of 0x01b40000 */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01b40144 ,0x01b40184 )}

            /* base of 0x01cc0000 */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x01cc0000 ,0x01cc0008 )}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }

    /* chunks with formulas */
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
            /* base of 0x01800000 */
             {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01800040 ,0)}, FORMULA_TWO_PARAMETERS(3,0x4 , 4 , 0x10)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01800040 + 0xEC0 ,0)}, FORMULA_TWO_PARAMETERS(3,0x4 , 4 , 0x10)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01800080 ,0)}, FORMULA_SINGLE_PARAMETER(64 , 0x200)}

            /* base of 0x01840000 */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01840000 ,0)}, FORMULA_SINGLE_PARAMETER(64 , 0x200)}

            /* base of 0x01880000 */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01880000 ,0)}, FORMULA_TWO_PARAMETERS(64 , 0x200 , 8 , 0x8000)}

            /* base of 0x018c0000 */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x018c0000 ,0)}, FORMULA_SINGLE_PARAMETER(64 , 0x200)}

            /* base of 0x01900000 */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01900000 ,0)}, FORMULA_TWO_PARAMETERS(64 , 0x200 , 8 , 0x8000)}

            /* base of 0x01940000 */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01940000 ,0)}, FORMULA_TWO_PARAMETERS(8 , 0x8 , 8 , 0x400)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01940004 ,0)}, FORMULA_TWO_PARAMETERS(8 , 0x8 , 8 , 0x400)}

            /* base of 0x019c0000 */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x019c0000 ,0)}, FORMULA_TWO_PARAMETERS(64 , 0x200 , 8 , 0x8000)}

            /* base of 0x01a00000 */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01a00000 ,0)}, FORMULA_TWO_PARAMETERS(64 , 0x200 , 8 , 0x8000)}

            /* base of 0x01a40000 */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01a40000 ,0)}, FORMULA_SINGLE_PARAMETER(128,  0x10)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01a40004 ,0)}, FORMULA_TWO_PARAMETERS(8 , 0x10 , 32 , 0x80)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01a40008 ,0)}, FORMULA_SINGLE_PARAMETER( 8 ,  0x10)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01a4000c ,0)}, FORMULA_SINGLE_PARAMETER(32 ,  0x10)}

            /* base of 0x01a80000 */
/*TBD-->?*/ ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01a80000 ,0)}, FORMULA_SINGLE_PARAMETER(64 , 0x200)}

            /* base of 0x01ac0000 */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01ac0000 ,0)}, FORMULA_TWO_PARAMETERS(64 , 0x200 , 8 , 0x8000)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01ac0004 ,0)}, FORMULA_TWO_PARAMETERS(64 , 0x200 , 8 , 0x8000)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01ac0008 ,0)}, FORMULA_SINGLE_PARAMETER(64 , 0x200)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01ac000c ,0)}, FORMULA_TWO_PARAMETERS(64 , 0x200 , 8 , 0x8000)}

            /* base of 0x01b00000 */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01b00000 ,0)}, FORMULA_TWO_PARAMETERS(64 , 0x200 , 8 , 0x8000)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01b00004 ,0)}, FORMULA_SINGLE_PARAMETER(64 , 0x200)}

            /* base of 0x01b00000 */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01b00000 ,0)}, FORMULA_TWO_PARAMETERS(64 , 0x200 , 8 , 0x8000)}

            /* base of 0x01b80000 */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01b80000 ,0)}, FORMULA_SINGLE_PARAMETER(4096 , 0x10)}

            /* base of 0x01be0000 */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01be0000 ,0)}, FORMULA_TWO_PARAMETERS(5 , 0x4 , 4096 , 0x20)}

            /* base of 0x01d40000 */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01d40000 ,0)}, FORMULA_TWO_PARAMETERS(3, 0x4 , 28 , 0x200)}

            /* base of 0x01e40000 */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01e40000 ,0)}, FORMULA_SINGLE_PARAMETER(32,  0x10)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01e40004 ,0)}, FORMULA_SINGLE_PARAMETER(2176,  0x10)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01e40008 ,0)}, FORMULA_SINGLE_PARAMETER(2176,  0x10)}

            /* base of 0x01e80000 */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x01e80000 ,0)}, FORMULA_SINGLE_PARAMETER(8,  0x4)}


        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);
    }
}

/*******************************************************************************
*   smemLionUnitL2i
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  L2i unit
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
static void smemLionUnitL2i
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
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02000800 ,0)}, FORMULA_SINGLE_PARAMETER(16 , 0x1000)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02000804 ,0)}, FORMULA_SINGLE_PARAMETER(16 , 0x1000)}

            /* base of 0x02010000 */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02010800 ,0)}, FORMULA_SINGLE_PARAMETER(2 , 0x1000)}

            /* base of 0x02020000 */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02020800 ,0)}, FORMULA_SINGLE_PARAMETER(8 , 0x1000)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02028800 ,0)}, FORMULA_SINGLE_PARAMETER(8 , 0x1000)}

            /* base of 0x02030000 */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02030800 ,0)}, FORMULA_TWO_PARAMETERS(8 , 0x1000 , 2 , 0x8000)}

            /* base of 0x02040000 */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02040800 ,0)}, FORMULA_TWO_PARAMETERS(8 , 0x1000 , 2 , 0x8000)}

        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);
    }

}
/*******************************************************************************
*   smemLionB0UnitL2i
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Lion B0 L2i unit
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
static void smemLionB0UnitL2i
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    /* Allocate Lion A0 memory first */
    smemLionUnitL2i(devObjPtr, unitPtr);

    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            /* Ingress Rate Limit Configuration */
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0200080C ,0x0200080C )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0200081C ,0x0200081C )}
        };
        GT_U32 numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

        smemInitMemChunk(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitPtr, &tmpUnitChunk);
    }

    /* chunks with formulas */
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
            /* IEEE Reserved Multicast Configuration1/2/3 */
            {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02000808 ,0)}, FORMULA_TWO_PARAMETERS(3, 0x8, 16 , 0x1000)}
            /* IEEE Reserved Multicast Configuration2 errata */
           ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02000820 ,0)}, FORMULA_SINGLE_PARAMETER(16 , 0x1000)}
            /* IEEE Reserved Multicast Configuration3 errata*/
           ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02000828 ,0)}, FORMULA_SINGLE_PARAMETER(16 , 0x1000)}
           /* Set0Word<%n>Desc Bits */
           ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02040020 ,0)}, FORMULA_SINGLE_PARAMETER(20 , 0x4)}
           /* Set1Word<%n>Desc Bits */
           ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02040A00 ,0)}, FORMULA_SINGLE_PARAMETER(20 , 0x4)}
        };
        GT_U32 numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitPtr, &tmpUnitChunk);
    }
}
/*******************************************************************************
*   smemLionUnitIpvx
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  IPVX unit
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
static void smemLionUnitIpvx
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
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
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02800940 ,0x02800964 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02800d00 ,0x02800d70 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x02800e3c ,0x02800e3c )}

            /* base of 0x02801000 */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02801000 , 2048 )}

            /* base of 0x02b00000 */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02b00000 , 131072)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }

    if(0 == SKERNEL_IS_LION_REVISON_B0_DEV(devObjPtr))
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            /* base of 0x02801000 */
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02801000 , 2048 )}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

        smemInitMemChunk(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitPtr, &tmpUnitChunk);
    }

    /* chunks with formulas */
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
            /* base of 0x02800000 */
             {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02800900 ,0)}, FORMULA_TWO_PARAMETERS(10 , 0x4 , 4 , 0x100)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02800980 ,0)}, FORMULA_SINGLE_PARAMETER(4 , 0x100)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitPtr, &tmpUnitChunk);
    }
}

/*******************************************************************************
*   smemLionB0UnitIpvx
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Lion B0 IPVX unit
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
static void smemLionB0UnitIpvx
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    /* Allocate Lion A0 memory first */
    smemLionUnitIpvx(devObjPtr, unitPtr);
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            /* Router Desc Log %n */
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02800CE0 , 32)}
            /* Router Vlan uRPF table */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02801400 , 1024)}
            /* Router Next Hop Table Age Bits */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02801000 , 1024)}
        };

        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

        smemInitMemChunk(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /* Update the main unit chunks */
        smemInitMemUpdateUnitChunks(devObjPtr,unitPtr, &tmpUnitChunk);
    }
}

/*******************************************************************************
*   smemLionUnitBm
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  BM unit
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
static void smemLionUnitBm
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            /* base of 0x03000000 */
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x03000000 ,0x03000084 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x03000200 ,0x03000210 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x03000300 ,0x03000314 )}

        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }

    if(0 == SKERNEL_IS_LION_REVISON_B0_DEV(devObjPtr))
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            /* base of 0x03010000 */
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x03010000 , 16384)}

            /* base of 0x03020000 */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x03020000 , 16384)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

        smemInitMemChunk(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitPtr, &tmpUnitChunk);
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

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);
    }
}

/*******************************************************************************
*   smemLionB0UnitBm
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Lion B0 BM unit
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
static void smemLionB0UnitBm
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    /* Allocate Lion A0 memory first */
    smemLionUnitBm(devObjPtr, unitPtr);
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            /* Link List */
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x03010000 , 8192)}

            /* Control */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x03020000 , 8192)}

        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

        smemInitMemChunk(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /* Update the main unit chunks */
        smemInitMemUpdateUnitChunks(devObjPtr,unitPtr, &tmpUnitChunk);
    }
}

/*******************************************************************************
*   smemLionUnitBma
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
static void smemLionUnitBma
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            /* base of 0x03800000 */
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x03800000 ,0x03800030 )}

            /* base of 0x03810000 */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x03810000 , 65536)}

        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
}


/*******************************************************************************
*   smemLionUnitLms
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
static void smemLionUnitLms
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            /* base of 0x04004000 */
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x04004024 ,0x04004034 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x04004054 ,0x04004054 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x04004000, 0x04004144 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x04004200 ,0x04004200 )}

            /* base of 0x04804000 */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x04804024 ,0x04804034 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x04804000, 0x04804144 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x04805100, 0x04805114 )}

            /* base of 0x05004000 */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x05004000, 0x05004054 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x05004140 ,0x05004144 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x05804000, 0x05804144 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x05005100 ,0x05005100 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x05005110 ,0x05005110 )}

        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
}

/*******************************************************************************
*   smemLionUnitFdb
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  FDB unit
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
static void smemLionUnitFdb
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            /* base of 0x06000000 */
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x06000000 ,0x06000068 )}

            /* base of 0x06400000 */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x06400000 , 262144 * 2)} /*FDB support 32K entries*/

        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
}

/*******************************************************************************
*   smemLionB0UnitFdb
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the LION b0 FDB unit
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
static void smemLionB0UnitFdb
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    /* Allocate Lion A0 memory first */
    smemLionUnitFdb(devObjPtr, unitPtr);

    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            /* Bad Parity Address */
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x06000088 ,0x06000088 )}
            /* Recognition Bad Parity Address */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x060000F0 ,0x06000100 )}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

        smemInitMemChunk(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /* Update the main unit chunks */
        smemInitMemUpdateUnitChunks(devObjPtr,unitPtr, &tmpUnitChunk);
    }
}

/*******************************************************************************
*   smemLionUnitMppm
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  MPPM unit
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
static void smemLionUnitMppm
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            /* base of 0x06800000 */
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x06800000 ,0x06800008 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x06800100 ,0x06800100 )}

        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
}

/*******************************************************************************
*   smemLionB0UnitMppm
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Lion B0 MPPM unit
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
static void smemLionB0UnitMppm
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    /* Allocate Lion A0 memory first */
    smemLionUnitMppm(devObjPtr, unitPtr);
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            /* XSMI Memory */
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC(0x06E40000 , 0x06e4000C)}
        };
        GT_U32 numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

        smemInitMemChunk(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitPtr, &tmpUnitChunk);
    }
}

/*******************************************************************************
*   smemLionUnitHaAndEpcl
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  HA_and_EPCL unit
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
static void smemLionUnitHaAndEpcl
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            /* base of 0x07800000 */
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x07800004 ,0x07800004 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x07800018 ,0x07800020 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x07800100 ,0x07800110 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x07800120 ,0x07800130 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x07800400 ,0x07800420 )}

            /* base of 0x07C00000 */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x07C00000 ,0x07C0003c )}

            /* base of 0x07E80000 */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x07E80000 , 32768)}

            /* base of 0x07F00000 */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x07F00000 , 16640)}

            /* base of 0x07F80000 */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x07F80000 , 16640)}

            /* base of 0x07FC0000 */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x07FC0000 , 16384)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }

    /* chunks with formulas */
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
            /* base of 0x07800000 */
             {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0780002c ,0)}, FORMULA_SINGLE_PARAMETER( 7 , 0x4)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x07800300 ,0)}, FORMULA_SINGLE_PARAMETER(16 , 0x4)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x07800444 ,0)}, FORMULA_SINGLE_PARAMETER( 9 , 0x4)}

            /* base of 0x07C00000 */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x07C00100 ,0)}, FORMULA_SINGLE_PARAMETER( 8 , 0x4)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x07C00140 ,0)}, FORMULA_SINGLE_PARAMETER( 8 , 0x4)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);
    }

}

/*******************************************************************************
*   smemLionB0UnitHa
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Lion B0 HA_and_EPCL unit
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
static void smemLionB0UnitHa
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
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0E800080 ,0x0E800084 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0E800100 ,0x0E800118 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0E800370 ,0x0E800374 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0E8003A0 ,0x0E8003B0 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0E800400 ,0x0E800434 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0E800550 ,0x0E800558 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0E800710 ,0x0E800710 )}

            /* ETS regfile */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0E800800 , 2048)}
            /* Port/VLAN MAC SA Table */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0E808000 , 16640)}
            /* VLAN Translation Table */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0E810000 , 16384)}
            /* DFX Memory */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0E818000 , 2048)}
            /* Router ARP DA and Tunnel Start Table */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0E840000 , 65536), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(195, 32)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }

    /* chunks with formulas */
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
            /* Port<4n...4n+3> TrunkNum Configuration Register<%n> */
             {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0E80002C ,0)}, FORMULA_SINGLE_PARAMETER( 16 , 0x4)}
            /* Router Header Alteration MAC SA Modification Mode */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0E800120 ,0)}, FORMULA_SINGLE_PARAMETER( 4 , 0x4)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0E800130 ,0)}, FORMULA_TWO_PARAMETERS(2 ,0x4, 3, 0x10)}
            /* QoSProfile to EXP Register */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0E800300 ,0)}, FORMULA_SINGLE_PARAMETER( 16, 0x4)}
            /* QoSProfile to DP Register */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0E800340 ,0)}, FORMULA_SINGLE_PARAMETER( 8 , 0x4)}
            /* Incoming Descriptor Word<<%w>> */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0E8003E0 ,0)}, FORMULA_SINGLE_PARAMETER( 5 , 0x4)}
            /* Keep VLAN1  */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0E800440 ,0)}, FORMULA_SINGLE_PARAMETER( 16 , 0x4)}
            /* TP-ID Configuration Register  */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0E800500 ,0)}, FORMULA_SINGLE_PARAMETER( 4 , 0x4)}
            /* Port Tag0 TP-ID Select */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0E800510 ,0)}, FORMULA_TWO_PARAMETERS( 8 , 0x4, 2, 0x20)}
        };

        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);
    }

}

/*******************************************************************************
*   smemLionB0UnitRxDma
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Lion B0 RX DMA unit
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
static void smemLionB0UnitRxDma
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            /* Arbiter configuration */
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0F000000 ,0x0F000000 )}
             /* Cut Through */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0F000010 ,0x0F000020 )}
            /* Interrupts */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0F000030 ,0x0F00003C )}
            /* DFX */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0F000048 ,0x0F00004C )}
            /* Debug */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0F000060 ,0x0F000060 )}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
    /* chunks with formulas */
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
            /* Data error status register */
             {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0F000050 ,0)}, FORMULA_SINGLE_PARAMETER( 4 , 0x4)}
        };

        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);
    }

}

/*******************************************************************************
*   smemLionB0UnitTxDma
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Lion B0 TX DMA unit
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
static void smemLionB0UnitTxDma
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            /* TXDMA Engine Configuration */
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0F800000 ,0x0F800048 )}
             /* DFT */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0F800050 ,0x0F800058 )}
            /* Cut Through */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0F800080 ,0x0F800098 )}
            /* Burst Size Limit */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0F800100 ,0x0F800100 )}
            /* Interrupts */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0F800180 ,0x0F800190 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0F8001B4 ,0x0F8001E8 )}
            /* Descriptor prefetch */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0F800200 , 512)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
    /* chunks with formulas */
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
            /* base of 0x07800074 */
             {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0F800104 ,0)}, FORMULA_SINGLE_PARAMETER(13 , 0x4)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitPtr, &tmpUnitChunk);
    }

}

/*******************************************************************************
*   smemLionB0UnitCtu
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Lion B0 CTU unit
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
static void smemLionB0UnitCtu
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            /* CTU interrupt cause & mask registers */
            {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x07003000 ,0x07003004)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
}

/*******************************************************************************
*   smemLionB0UnitEplr
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Lion B0 EPLR unit
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
static void smemLionB0UnitEplr
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    smemLionB0UnitPolicerUnify(devObjPtr, unitPtr, policerBaseAddr[2]);
    {
        /* Update DB for EPLR unit */
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00080000 , 2560), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(51, 8)}

        };

        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);
        GT_U32  ii;
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

        for(ii = 0 ; ii < numOfChunks ; ii++)
        {
            chunksMem[ii].memFirstAddr += policerBaseAddr[2];
        }

        smemInitMemChunk(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /* Update the main unit chunks */
        smemInitMemUpdateUnitChunks(devObjPtr,unitPtr, &tmpUnitChunk);
    }
}

/*******************************************************************************
*   smemLionUnitMem
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  MEM unit
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
static void smemLionUnitMem
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            /* base of 0x07800000 */
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x07800000 ,0x07800000 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x07800024 ,0x07800028 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x07800064 ,0x07800064 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x07800090 ,0x0780009c )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x078000A0 ,0x078000A0 )}

            /* base of 0x07E00000 */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x07E00000 ,264 *2 )}

            /* base of 0x07e40000 */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x07e40000 ,0x07e40008 )}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }

}

/*******************************************************************************
*   smemLionUnitTxdma
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  TXDMA unit
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
static void smemLionUnitTxdma
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            /* base of 0x07800000 */
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x07800008 ,0x07800008 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x07800048 ,0x07800048 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x07800060 ,0x07800070 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x07800080 ,0x07800084 )}

        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
}

/*******************************************************************************
*   smemLionUnitCentralizedCounters
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  Centralized_Counters unit
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
static void smemLionUnitCentralizedCounters
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            /* base of 0x08000000 */
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x08000000 ,0x08000000 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x08000030 ,0x08000030 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x08000040 ,0x08000044 )}

        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }

    if(0 == SKERNEL_IS_LION_REVISON_B0_DEV(devObjPtr))
    {
        {
            SMEM_CHUNK_BASIC_STC  chunksMem[]=
            {
                /* base of 0x08000000 */
                {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x08000100 ,0x08000104 )}

                /* base of 0x08080000 */
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x08080000 , 16384)}
            };
            GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);
            SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

            smemInitMemChunk(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

            /*add the tmp unit chunks to the main unit */
            smemInitMemCombineUnitChunks(devObjPtr,unitPtr, &tmpUnitChunk);
        }

        /* chunks with formulas */
        {
            SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
            {
                /* base of 0x08001000 */
                 {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x08001080 ,0)}, FORMULA_SINGLE_PARAMETER(5 , 0x4)}
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x080010a0 ,0)}, FORMULA_SINGLE_PARAMETER(4 , 0x4)}
            };
            GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
            SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

            smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

            /*add the tmp unit chunks to the main unit */
            smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);
        }
    }
}

/*******************************************************************************
*   smemLionB0UnitCentralizedCounters
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  Lion B0 Centralized_Counters unit
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
static void smemLionB0UnitCentralizedCounters
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    /* Allocate Lion A0 memory first */
    smemLionUnitCentralizedCounters(devObjPtr, unitPtr);

    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            /* base of 0x08000100 */
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x08000100 ,0x0800010C )}
            /* Data Error */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC(0x08000110 , 0x08000118)}

        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

        smemInitMemChunk(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /* Update the main unit chunks */
        smemInitMemUpdateUnitChunks(devObjPtr,unitPtr, &tmpUnitChunk);
    }

    /* chunks with formulas */
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
            /* CNC Block %n Configuration Register%m  */
             {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x08001180 ,0)}, FORMULA_TWO_PARAMETERS(8, 0x100, 3, 0x4)}
            /* CNC Block %n Wraparound Status Register */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x080010A0 ,0)}, FORMULA_TWO_PARAMETERS(8, 0x100, 4, 0x4)}
            /* CNC Blocks 0..7 */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x08080000 ,0)}, FORMULA_TWO_PARAMETERS(1024, 0x4, 8, 0x4000)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /* Update the main unit chunks */
        smemInitMemUpdateUnitChunks(devObjPtr,unitPtr, &tmpUnitChunk);
    }
}

/*******************************************************************************
*   smemLionUnitXgPorts
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  XGPort unit
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
static void smemLionUnitXgPorts
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    /* chunks with formulas */
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
            /* base of 0x08800000 */                                               /*changed 58 to 5c to support int mask reg of int cause reg 58 */
              {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x08800000 ,0)}, FORMULA_TWO_PARAMETERS((0x5c/4) +1 , 0x4 , 12 , 0x400)}
             ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x08800200 ,0)}, FORMULA_TWO_PARAMETERS(512/4 , 0x4 , 12 , 0x400)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, unitPtr);
    }
}

/*******************************************************************************
*   smemLionB0UnitXgPorts
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Lion B0 XGPort unit
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
static void smemLionB0UnitXgPorts
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{

    /* chunks with formulas */
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
            /* base of 0x08800000 */
             {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x08800000 ,0)}, FORMULA_TWO_PARAMETERS((0x84/4)+1 , 0x4, 12, 0x400)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x08800200 ,0)}, FORMULA_TWO_PARAMETERS(512/4 , 0x4 , 12 , 0x400)}

            /* base of 0x08803000 */
            /* 40G MAC */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x08803000 ,0)}, FORMULA_TWO_PARAMETERS((0x84/4)+1 , 0x4, 1, 0x400)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x08803200 ,0)}, FORMULA_TWO_PARAMETERS(512/4 , 0x4 , 1 , 0x400)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, unitPtr);
    }
}

/*******************************************************************************
*   smemLionUnitXGPortMib
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
static void smemLionUnitXGPortMib
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    /* chunks with formulas */
    if(SKERNEL_IS_LION_REVISON_B0_DEV(devObjPtr)) /*LionB*/
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
            /* base of 0x09000000 */
            /* NOTE : 13 include the XLG counters (represented by port 12) */
             {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x09000000 ,0)}, FORMULA_TWO_PARAMETERS(256/4 , 0x4 , 13 , 0x20000)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, unitPtr);
    }
    else  /*LionA*/
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
            /* base of 0x09000000 */
             {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x09000000 ,0)}, FORMULA_TWO_PARAMETERS(256/4 , 0x4 , 12 , 0x20000)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, unitPtr);
    }
}

/*******************************************************************************
*   smemLionUnitLpSerdes
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  LpSerdes unit
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
static void smemLionUnitLpSerdes
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    /* chunks with formulas */
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
            /* base of 0x09800000 */
            {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x09800200 ,0)}, FORMULA_THREE_PARAMETERS(512/4 , 0x4 , 4 , 0x400 , 6, 0x1000)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, unitPtr);
    }

    /* patch until CPSS will support SERDESs correctly , code taken from XCAT A0 */
    {
        /* the register of SERDESs 0..23 */
        GT_U32  serdessIdArray[]=
            {0,1,2,3,4,5,6,7,
             8,9,10,11,12,13,14,15,
             16,17,18,19,20,21,22,23};/* patch to support 4 SERDESs per port */

        #define LION_SERDESS_NUM (sizeof(serdessIdArray)/sizeof(serdessIdArray[0]))

        SMEM_CHUNK_BASIC_STC  chunksMem[2*LION_SERDESS_NUM]=
        {
            /* the register of unit 0*/
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC(0x09800000,0x098000d8)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC(0x09808000,0x09808000)}
            /* the register of unit 0..23 */
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        GT_U32  ii;
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

        for(ii = 1;ii < LION_SERDESS_NUM;ii++)
        {
            chunksMem[ii*2].memFirstAddr    = 0x400*serdessIdArray[ii] + chunksMem[0].memFirstAddr;
            chunksMem[ii*2].numOfRegisters  = chunksMem[0].numOfRegisters;

            chunksMem[ii*2+1].memFirstAddr    = 0x400*serdessIdArray[ii] + chunksMem[1].memFirstAddr;
            chunksMem[ii*2+1].numOfRegisters  = chunksMem[1].numOfRegisters;
        }


        smemInitMemChunk(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);
    }
}

/*******************************************************************************
*   smemLionB0UnitLpSerdes
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Lion B0 LpSerdes unit
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
static void smemLionB0UnitLpSerdes
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    /* Allocate Lion A0 memory first */
    smemLionUnitLpSerdes(devObjPtr, unitPtr);
}

/*******************************************************************************
*   smemLionUnitVlan_
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  EgressAndTxq unit
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
static void smemLionUnitVlan_
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    /* chunks with flat memory (no formulas) */
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            /* base of 0x0a000000 */
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0a000000 ,0x0a000000 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0a000100 ,0x0a000118 )}

            /* base of 0x0A080000 */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0A080000 , 4096)}

            /* base of 0x0A100000 */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0A100000 , 65536)}

            /* base of 0x0A180000 */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0A180000 , 65536)}

            /* base of 0x0A200000 */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0A200000 , 131072)}

        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
}

/*******************************************************************************
*   smemLionB0UnitTxq
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Lion B0 Txq unit
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
static void smemLionB0UnitTxq
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    /* chunks with flat memory (no formulas) */
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            /* base of 0A090000 */
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0A090000, 0x0A09000C )}
            /* Port Descriptor Full Interrupt */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0A090040 ,0x0A090054 )}
            /* Port Buffer Full Interrupt  */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0A090060 ,0x0A090074 )}
            /* PFC Interrupts */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0A090080 ,0x0A090114 )}
            /* Congestion Notification Interrupt Cause/Mask */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0A090120 ,0x0A090124 )}
            /* TXQ_QUEUE_DFX Area */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0A090800, 2048)}
            /* Distributor General Configurations */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0A091000 ,0x0A091000 )}
            /* CNC Modes Register */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0A0936A0 ,0x0A0936A0 )}
            /* Tail Drop Configuration */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0A0A0000 ,0x0A0A0000 )}
            /* Byte Count for Tail Drop */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0A0A0008 ,0x0A0A0008 )}
            /* Global Descriptors Limit */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0A0A0300 ,0x0A0A0300 )}
            /* Global Buffer Limit */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0A0A0310 ,0x0A0A0310 )}
            /* Shared Queue Limits */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0A0A0500 , 256)}
            /* Eq Queue Limits DP0 */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0A0A0800 , 256)}
            /* Dq Queue Buf Limits */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0A0A0900 , 256)}
            /* Dq Queue Desc Limits */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0A0A0A00 , 256)}
            /* Eq Queue Limits DP12 */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0A0A0C00 , 512)}
            /* Total Desc Counter */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0A0A1000 ,0x0A0A1004 )}
            /*Port<%n> Desc Counter  Port<%n> Desc Counter*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0A0A1010 , 64*4)}
            /*Port<%n> Buffers Counter  Port<%n> Buffers Counter*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0A0A1110 , 64*4)}
            /*TC<%t> Shared Desc Counter  TC<%t> Shared Desc Counter*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0A0A1210 , 8*4)}
            /*TC<%t> Shared Buffers Counter  TC<%t> Shared Buffers Counter*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0A0A1250 , 8*4)}
            /* Q Main Buff */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0A0A4000 , 2048)}
            /* CN Global Configurations */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0A0B0000 ,0x0A0B0000 )}
            /* CN Sample Intervals Table */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0A0B1000 , 4096)}
            /*PFC Global Configurations*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0A0C0000 ,0x0A0C0000 )}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
    /* chunks with formulas */
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
            /* TC DP Remapping */
             {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0A091010, 0)}, FORMULA_SINGLE_PARAMETER(2, 0x4)}
            /* Stack TC <%tc> Remapping */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0A091020, 0)}, FORMULA_SINGLE_PARAMETER(8, 0x4)}
            /* Stack Remap En */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0A091050, 0)}, FORMULA_TWO_PARAMETERS(2, 0x4, 7, 0x10)}
            /* Peripheral Access Misc */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0A093000, 0)}, FORMULA_SINGLE_PARAMETER(5, 0x4)}
            /* Egress MIB Counters */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0A093200, 0)}, FORMULA_TWO_PARAMETERS(2, 0x4, 9, 0x10)}
            /* Port Enqueue Enable */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0A0A0010, 0)}, FORMULA_SINGLE_PARAMETER(64, 0x4)}
            /* Port Tail Drop/CN Profile, TC Profile Enable Shared Pool Usage, Profile Enable WRTD DP,
               Profile Priority Queue to Shared Pool Association, WRTD Mask 0/1 */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0A0A0120, 0)}, FORMULA_SINGLE_PARAMETER(86, 0x4)}
            /* Profile Port Limits */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0A0A0320, 0)}, FORMULA_SINGLE_PARAMETER(8, 0x4)}
            /* Shared Pool Limits */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0A0A0400, 0)}, FORMULA_SINGLE_PARAMETER(8, 0x4)}
            /* Multicast Filter Limits */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0A0A3300, 0)}, FORMULA_SINGLE_PARAMETER(4, 0x10)}
            /* Muliticast Filter Counters */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0A0A6000, 0)}, FORMULA_SINGLE_PARAMETER(4, 0x4)}
            /* Resource Histogram Limits */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0A0A6300, 0)}, FORMULA_SINGLE_PARAMETER(2, 0x4)}
            /* Resource_Histogram_Counters */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0A0A6310, 0)}, FORMULA_SINGLE_PARAMETER(4, 0x4)}
            /* CN Priority Queue En Profile and En CN Frame Tx on Port */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0A0B0010, 0)}, FORMULA_TWO_PARAMETERS(2, 0x4, 3, 0x10)}
            /* Feedback Calc Configurations Feedback MIN/Max */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0A0B0040, 0)}, FORMULA_SINGLE_PARAMETER(3, 0x10)}
            /* CN Sample Tbl  */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0A0B0090, 0)}, FORMULA_SINGLE_PARAMETER(8, 0x4)}
            /* CN Buffer FIFO Overruns Counter, CN Buffer FIFO Parity Errors Counter and CN Drop Counter  */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0A0B00D0, 0)}, FORMULA_SINGLE_PARAMETER(3, 0x10)}
            /* Profile Tc CN Threshold */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0A0B0300, 0)}, FORMULA_THREE_PARAMETERS(8, 0x20, 8, 0x4, 4, 0x200)}
            /* Src Port FC Mode */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0A0C0010, 0)}, FORMULA_SINGLE_PARAMETER(2, 0x4)}
            /* PFC Pipe Counters Status */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0A0C0030, 0)}, FORMULA_SINGLE_PARAMETER(4, 0x4)}
            /* PFC Pipe */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0A0C0050, 0)}, FORMULA_SINGLE_PARAMETER(20, 0x4)}
            /* PFC Source Port Profile */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0A0C00B0, 0)}, FORMULA_SINGLE_PARAMETER(8, 0x4)}
            /* Link Level Flow Control Target Port Remap and PFC LLFC Source Port Remap */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0A0C0100, 0)}, FORMULA_TWO_PARAMETERS(16, 0x4, 2, 0x100)}
            /* Global PFC Group of Ports TC Counter and Global PFC Group of Ports TC Thresholds */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0A0C0500, 0)}, FORMULA_THREE_PARAMETERS(4, 0x20, 8, 0x4, 2, 0x100)}
            /* FC Mode Profile TC XOff Thresholds and FC Mode Profile TC XOn Thresholds */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0A0C0800, 0)}, FORMULA_TWO_PARAMETERS((256/4), 0x4, 2, 0x200)}
            /* Pipe 0/1/2/3 PFC LLFC Counters */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0A0C1000, 0)}, FORMULA_TWO_PARAMETERS((4096/4), 0x4, 4, 0x1000)}
        };

        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);
        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);
    }
}

/*******************************************************************************
*   smemLionB0UnitTxqLl
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Lion B0 TxqLl unit
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
static void smemLionB0UnitTxqLl
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
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
            /* TXQ_LL_DFX */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x108A0800 ,0x108A0808 )}
        };

        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

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

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);
        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);
    }
}

/*******************************************************************************
*   smemLionB0UnitTxqDq
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Lion B0 TxqDq unit
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
static void smemLionB0UnitTxqDq
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
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
             /* Egress STC Table*/
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x11004400, 256), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(80, 16)}
             /* DFX Configuration */
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x11000800, 0x11000800)}
        };

        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
    /* chunks with formulas */
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
            /* Port TxQ Flush Trigger */
             {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x11000090, 0)}, FORMULA_SINGLE_PARAMETER(16, 0x4)}
            /* Port <%n> Request Mask Selector */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x1100100C, 0)}, FORMULA_SINGLE_PARAMETER(16, 0x4)}
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
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x11001400, 0)}, FORMULA_SINGLE_PARAMETER(3, 0x4)}
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

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);
        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);
    }
}

/*******************************************************************************
*   smemLionB0UnitTxqSht
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Lion B0 TxqSht unit
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
static void smemLionB0UnitTxqSht
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    /* chunks with flat memory (no formulas) */
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             /* Egress vlan table */
              {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x11800000 , 262144), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(267, 64)}
             /* Egress Spanning Tree State Table */
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x11840000 , 4096), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(126, 16)}
             /* Multicast Groups Table */
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x11860000 , 65536), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(65, 16)}
             /* Device Map Table */
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x11870000 , 8192)}
             /* L2 Port Isolation Table */
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x11880000 , 34816), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(65, 16)}
             /* L3 Port Isolation Table */
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x11890000 , 34816), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(65, 16)}
             /* Ingress Vlan Table */
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x118A0000 , 131072), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(140, 32)}
             /* Ingress Spanning Tree State Table */
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x118D0000 , 4096), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(126, 16)}

            /* SHT Global Configurations */
             ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x118F0000, 0x118F0000)}
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
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x118F0100, 0)}, FORMULA_SINGLE_PARAMETER(4, 0x10)}
        };

        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);
        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);
    }
}

/*******************************************************************************
*   smemLionUnitGigPorts
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  GigPorts unit
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
static void smemLionUnitGigPorts
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    /* chunks with formulas */
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
            /* base of 0x0A800000 */
             {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0A800000 ,0)}, FORMULA_TWO_PARAMETERS((0x48/4) + 1 , 0x4 , 12, 0x400)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0A800000 + 63*0x400,0)}, FORMULA_TWO_PARAMETERS((0x48/4) + 1 , 0x4 , 1, 0x400)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, unitPtr);
    }
}

/*******************************************************************************
*   smemLionB0UnitGigPorts
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Lion B0 GigPorts unit
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
static void smemLionB0UnitGigPorts
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    /* chunks with formulas */
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
            /* base of 0x0A800000 */
             {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0A800000 ,0)}, FORMULA_TWO_PARAMETERS(14, 0x4 , 12, 0x400)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0A800038 ,0)}, FORMULA_TWO_PARAMETERS( 8, 0x4 , 12, 0x400)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0A800058 ,0)}, FORMULA_TWO_PARAMETERS( 8, 0x4 , 12, 0x400)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0A800078 ,0)}, FORMULA_TWO_PARAMETERS( 6, 0x4 , 12, 0x400)}

            /* base of 0x0A800000 */

            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC((0x0A800000+63*0x400) ,0)}, FORMULA_SINGLE_PARAMETER(14, 0x4)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC((0x0A800038+63*0x400) ,0)}, FORMULA_SINGLE_PARAMETER( 8, 0x4)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC((0x0A800058+63*0x400) ,0)}, FORMULA_SINGLE_PARAMETER( 8, 0x4)}


            /* base of 0x0A800100 */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0A800100 ,0)}, FORMULA_TWO_PARAMETERS( 5, 0x4 , 6, 0x800)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0A800118 ,0)}, FORMULA_TWO_PARAMETERS(16, 0x4 , 6, 0x800)}

        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, unitPtr);
    }
}

/*******************************************************************************
*   smemLionUnitEq
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  Eq unit
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
static void smemLionUnitEq
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            /* base of 0x0b000000 */
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0b000000 ,0x0b000074 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0b0000A0 ,0x0b0000A0 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0b005000 ,0x0b005008 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0b007000 ,0x0b007080 - 4 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0b008000 ,0x0b008010)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0b009000 ,0x0b009040 - 4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0b00a000 ,0x0b00a000)}

            /* base of 0x0b020000 */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0b020000 ,0x0b02000c )}

            /* base of 0x0b040000 */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0b040000 ,448 )}

            /* base of 0x0b050000 */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0b050000 ,0x0b050004 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0b05000C ,0x0b05000C )}

            /* base of 0x0b100000 */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0b100000 ,128 )}

            /* base of 0x0b200000 */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0b200000 ,1024 )}

            /* base of 0x0b300000 */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0b300000 ,512 )}

            /* base of 0x0b400000 */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0b400000 ,4094 )}

            /* base of 0x0b500000 */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0b500000 ,124 )}

            /* base of 0x0b600000 */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0b600000 ,2048 )}

        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }

    /* chunks with formulas */
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
            /* Pre-Egress Engine OutGoing Desc Word<%n+5> */
             {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0b00b050  ,0)}, FORMULA_SINGLE_PARAMETER(2 , 0x4)}
            /* base of 0x0b000000 */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0b001000 ,0)}, FORMULA_SINGLE_PARAMETER(4 , 0x1000)}
            /* base of 0x0b080000 -- support 32 registers to support address 0x0b080000 for diag on cpss */
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0b080000  ,0)}, FORMULA_SINGLE_PARAMETER(32 , 0x4)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);
    }
}

/*******************************************************************************
*   smemLionB0UnitEq
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Lion B0 Eq unit
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
static void smemLionB0UnitEq
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    /* Allocate Lion A0 memory first */
    smemLionUnitEq(devObjPtr, unitPtr);
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            /* Ingress Drop Codes Counter */
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0B0000A4 ,0x0B0000A8 )}
            /* Mirroring to Analyzer Port Configurations */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0B00B000 ,0x0B00B008 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0B00B010 ,0x0B00B018 )}
            /* Mirror Interface Parameter Register */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0B00B020 ,0x0B00B03C )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0B00B040 ,0x0B00B04C )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0B00B070 ,0x0B00B070 )}
            /* Multi-Port Group Lookup */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0B00007C ,0x0B000084 )}
            /*  Parity Error */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0B000088 ,0x0B00009C )}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

        smemInitMemChunk(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /* Update the main unit chunks */
        smemInitMemCombineUnitChunks(devObjPtr,unitPtr, &tmpUnitChunk);
    }
}

/*******************************************************************************
*   smemLionUnitIpcl
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  IPCL unit
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
static void smemLionUnitIpcl
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            /* base of 0x0b800000 */
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0b800000 ,0x0b800080 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0b800200 ,0x0b800208 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0b800300 ,0x0b800328 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0b800400 ,0x0b8004bc )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0b800500 ,0x0b800520 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0b800600 ,0x0b80061c )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0b800640 ,0x0b80065c )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0b800700 ,0x0b80077c )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0b800800 ,0x0b800824 )}

            /* base of 0x0b804000 */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0b804000 ,16384 )}

            /* base of 0x0b810000 */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0b810000 ,28*0x20 )}/*for ports 0..27 0x20 for each entry*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0b810000+(63*0x20) ,16 )}/*for port 63*/

            /* base of 0x0b810800 */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0b810800 ,600 +12)}

            /* base of 0x0b840000 */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0b840000 ,33792 )}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }

}

/*******************************************************************************
*   smemLionB0UnitIpcl
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Lion B0 IPCL unit
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
static void smemLionB0UnitIpcl
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
            /* PCL Unit Ram Test */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC(0x0B800804 ,0x0B800808 )}
            /* Data error */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC(0x0B800810 ,0x0B800828 )}

            /* CRC Hash Mask Memory */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0B800400 ,448), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(70, 16)}
            /* PCL DFX Area */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0B800800 ,2048), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4)}
            /* IPCL0 Configuration Table */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0B810000 ,33792), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(33, 8)}
            /* IPCL1 Configuration Table */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0B820000 ,33792), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(23, 8)}

        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
    /* chunks with formulas */
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
            /* base of 0x0c800000 */
              {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0B80006C ,0)}, FORMULA_SINGLE_PARAMETER(2 , 0x4)}
             ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0B8005C0 ,0)}, FORMULA_SINGLE_PARAMETER(16 , 0x4)}
             /* Policy Descriptor to Bridge Word<%n> Register */
             ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0B800200 ,0)}, FORMULA_SINGLE_PARAMETER(28 , 0x4)}

        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);
    }


}

/*******************************************************************************
*   smemLionUnitIplr
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  IPLR unit
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
static void smemLionUnitIplr
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            /* base of 0x0c000000 */
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0c000000 ,0x0c00007c )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0c000100 ,0x0c00010c )}

            /* base of 0x0c000000 */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0c000200 ,24 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0c000400 ,64 )}

            /* base of 0x0c040000 */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0c040000 ,65536 )}

            /* base of 0x0c080000 */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0c080000 ,512 )}

            /* base of 0x0c0c0000 */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0c0c0000 ,96 )}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }

}

/*******************************************************************************
*   smemLionB0UnitIplr0
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Lion B0 IPLR unit
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
static void smemLionB0UnitIplr0
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    smemLionB0UnitPolicerUnify(devObjPtr,unitPtr,policerBaseAddr[0]);
}

/*******************************************************************************
*   smemLionUnitMll
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
static void smemLionUnitMll
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
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0c800100 ,0x0c80011c )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0c800200 ,0x0c800214 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0c800984 ,0x0c800984 )}

            /* base of 0x0c880000 */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0c880000 ,65536), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(128, 16)}      /* MLL Entry */
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }

    /* chunks with formulas */
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
            /* base of 0x0c800000 */
              {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0c800900 ,0)}, FORMULA_SINGLE_PARAMETER(2 , 0x100)}
             ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0c800980 ,0)}, FORMULA_SINGLE_PARAMETER(2 , 0x100)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);
    }
}

/*******************************************************************************
*   smemLionUnitIpclTcc
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  IPCL_TCC unit
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
static void smemLionUnitIpclTcc
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
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

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }

}

/*******************************************************************************
*   smemLionB0UnitEpclTcc
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Lion B0 EPCL_TCC unit
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
static void smemLionB0UnitEpclTcc
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0E000000 ,0x0E000000)} /*Egress Policy Global Configuration*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0E000080 ,0x0E00008C)} /*Egress Policy DIP Solicitation          */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0E000090 ,0x0E0000A0 )} /*Egress Policy DIP Solicitation Mask          */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0E000200 ,0x0E000200 )} /*Egress Policer Global Configuration          */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0E008000 ,16640)    } /*Egress PCL Configuration Table                     */
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
    /* chunks with formulas */
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
            /* Egress Policy Configuration */
              {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0E000010 ,0)}, FORMULA_TWO_PARAMETERS(2 , 0x4, 7, 0x10)}
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
*   smemLionB0UnitIpclTcc
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Lion B0 IPCL_TCC unit
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
static void smemLionB0UnitIpclTcc
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    /* Allocate Lion A0 memory first */
    smemLionUnitIpclTcc(devObjPtr,unitPtr);
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            /* TCC Registers */
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0D000198 ,0x0D000198 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0D0001A4 ,0x0D0001C0 )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0D000304 ,0x0D00031C )}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0D002004 ,0x0D002028 )}

            /* Tcam Bist Area */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0D000400 ,256 )}
            /* TCAM Array Compare Enable */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0D001000 ,256 )}
            /* LOWER TCC DFX Area */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0D002000 ,2048 )}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

        smemInitMemChunk(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /* Update the main unit chunks */
        smemInitMemUpdateUnitChunks(devObjPtr,unitPtr, &tmpUnitChunk);
    }
}

/*******************************************************************************
*   smemLionUnitIpvxTcc
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  IPvX_TCC unit
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
static void smemLionUnitIpvxTcc
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0D800400 ,0x0D80041C)} /*LTTAndRouterTCAMAccessCtrl_E*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0D800960 ,0x0D800D78)} /*TCC_Registers               */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0D840000 ,(131072*2))  } /*routerAndTTTCAM_E --
                            manually changed from  131072 ,
                            because there are 16K entries of 16 bytes --> total of 256K*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0D880000 ,65536)   } /*lookup_Translation_Table_E  */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0D890000 ,16384)   } /*ECC_Table_X                 */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0D8a0000 ,16384)   } /*ECC_Table_y                 */
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }

}

/*******************************************************************************
*   smemLionB0UnitIpvxTcc
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Lion B0 IPvX_TCC unit
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
static void smemLionB0UnitIpvxTcc
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0D800400 ,0x0D80041C)} /*LTTAndRouterTCAMAccessCtrl_E*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0D800054 ,0x0D80007C)} /*TCC_Registers               */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0D800080 ,0x0D800080)} /*TTI Key Type 2 Lookup ID              */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0D8000A0 ,0x0D8000A4)} /*Router DIP/SIP Lookup Key Type 2 Lookup ID             */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0D800960 ,0x0D800D78)} /* Router_TCAM_Configuration 0  */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0D802004 ,0x0D802004)} /*TCC RAM mask             */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0D802050 ,0x0D802058)} /*Data Error             */

            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0D800100 ,256)  } /* Tcam Bist Area           */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0D801000 ,256)  } /* TCAM Array Compare Enable      */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0D802000 ,2048 )  } /* UPPER TCC DFX Area      */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0D840000 ,262144)  } /*routerAndTTTCAM_E           */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0D8C0000 ,131072)   } /*lookup_Translation_Table_E  */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0D890000 ,8192)   } /*ECC_Table_X                 */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0D8a0000 ,8192)   } /*ECC_Table_y                 */

        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem, numOfChunks, unitPtr);
    }

}

/*******************************************************************************
*   smemLionUnitMppm_
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  _MPPM unit
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
static void smemLionUnitMppm_
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        if(0 == SKERNEL_IS_LION_REVISON_B0_DEV(devObjPtr))
        {
            /* Lion A0 */
            SMEM_CHUNK_BASIC_STC chunksMem[] =
            {
                {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC(0x10000000 , 0x1007FFFC)} /* 4K entries */
               ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC(0x10080000 , 0x100FFFFC)} /* 4K entries */
            };

            GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

            smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);

        }
        else
        {
            /* Lion B0 */
            SMEM_CHUNK_BASIC_STC chunksMem[] =
            {
                {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC(0x10000000 , 0x100FFF7C)} /* 4K entries */
               ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC(0x10100000 , 0x101FFF7C)} /* 4K entries */

            };

            GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

            smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);

        }

    }
}

/*******************************************************************************
*   smemLionB0AllocSpecMemory
*
* DESCRIPTION:
*       Allocate Lion B0 address type specific memories.
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
static void smemLionB0AllocSpecMemory
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = devObjPtr->deviceMemory;

    /* create all the memories of the units */
    smemLionB0UnitMg(devObjPtr, UNIT_CHUNK_PTR_NO_CAST_MAC(devMemInfoPtr, MG_UnitMem));
    smemXCatA1UnitPex(devObjPtr, &devMemInfoPtr->PEX_UnitMem, devMemInfoPtr->common.pciUnitBaseAddr);
    smemLionB0UnitIplr1(devObjPtr, UNIT_CHUNK_PTR_NO_CAST_MAC(devMemInfoPtr, IPLR1_UnitMem));
    smemLionB0UnitTti(devObjPtr, UNIT_CHUNK_PTR_NO_CAST_MAC(devMemInfoPtr, TTI_UnitMem));
    smemLionB0UnitTxqEgr(devObjPtr, UNIT_CHUNK_PTR_NO_CAST_MAC(devMemInfoPtr, TXQ_EGR_UnitMem));
    smemLionB0UnitL2i(devObjPtr, UNIT_CHUNK_PTR_NO_CAST_MAC(devMemInfoPtr, L2I_UnitMem));
    smemLionB0UnitIpvx(devObjPtr,UNIT_CHUNK_PTR_NO_CAST_MAC(devMemInfoPtr, IPvX_UnitMem));
    smemLionB0UnitBm(devObjPtr,UNIT_CHUNK_PTR_NO_CAST_MAC(devMemInfoPtr, BM_UnitMem));
    smemLionUnitLms(devObjPtr,UNIT_CHUNK_PTR_NO_CAST_MAC(devMemInfoPtr, LMS_UnitMem));
    smemLionB0UnitFdb(devObjPtr,UNIT_CHUNK_PTR_NO_CAST_MAC(devMemInfoPtr, FDB_UnitMem));
    smemLionB0UnitMppm(devObjPtr,UNIT_CHUNK_PTR_NO_CAST_MAC(devMemInfoPtr, MPPM_UnitMem));
    smemLionB0UnitCtu(devObjPtr,UNIT_CHUNK_PTR_NO_CAST_MAC(devMemInfoPtr, CTU_UnitMem));
    smemLionB0UnitEplr(devObjPtr,UNIT_CHUNK_PTR_NO_CAST_MAC(devMemInfoPtr, EPLR_UnitMem));
    smemLionB0UnitCentralizedCounters(devObjPtr,UNIT_CHUNK_PTR_NO_CAST_MAC(devMemInfoPtr, Centralized_Counters_UnitMem));
    smemLionB0UnitXgPorts(devObjPtr,UNIT_CHUNK_PTR_NO_CAST_MAC(devMemInfoPtr, XGPorts_UnitMem));
    smemLionUnitXGPortMib(devObjPtr,UNIT_CHUNK_PTR_NO_CAST_MAC(devMemInfoPtr, XGPort_MIB_UnitMem));
    smemLionB0UnitLpSerdes(devObjPtr,UNIT_CHUNK_PTR_NO_CAST_MAC(devMemInfoPtr, LP_SERDES_UnitMem));
    smemLionB0UnitTxq(devObjPtr,UNIT_CHUNK_PTR_NO_CAST_MAC(devMemInfoPtr, TXQ_Queue_UnitMem));
    smemLionB0UnitTxqLl(devObjPtr, UNIT_CHUNK_PTR_NO_CAST_MAC(devMemInfoPtr, TXQ_LL_UnitMem));
    smemLionB0UnitTxqDq(devObjPtr, UNIT_CHUNK_PTR_NO_CAST_MAC(devMemInfoPtr, TXQ_DQ_UnitMem));
    smemLionB0UnitTxqSht(devObjPtr, UNIT_CHUNK_PTR_NO_CAST_MAC(devMemInfoPtr, TXQ_SHT_UnitMem));
    smemLionB0UnitGigPorts(devObjPtr, UNIT_CHUNK_PTR_NO_CAST_MAC(devMemInfoPtr, GigPorts_UnitMem));
    smemLionB0UnitEq(devObjPtr, UNIT_CHUNK_PTR_NO_CAST_MAC(devMemInfoPtr, Eq_UnitMem));
    smemLionB0UnitIpcl(devObjPtr, UNIT_CHUNK_PTR_NO_CAST_MAC(devMemInfoPtr, IPCL_UnitMem));
    smemLionB0UnitIplr0(devObjPtr, UNIT_CHUNK_PTR_NO_CAST_MAC(devMemInfoPtr, IPLR_UnitMem));
    smemLionUnitMll(devObjPtr, UNIT_CHUNK_PTR_NO_CAST_MAC(devMemInfoPtr, MLL_UnitMem));
    smemLionB0UnitIpclTcc(devObjPtr, UNIT_CHUNK_PTR_NO_CAST_MAC(devMemInfoPtr, IPCL_TCC_UnitMem));
    smemLionB0UnitIpvxTcc(devObjPtr, UNIT_CHUNK_PTR_NO_CAST_MAC(devMemInfoPtr, IPvX_TCC_UnitMem));
    smemLionB0UnitEpclTcc(devObjPtr, UNIT_CHUNK_PTR_NO_CAST_MAC(devMemInfoPtr, EPCL_UnitMem));
    smemLionB0UnitHa(devObjPtr, UNIT_CHUNK_PTR_NO_CAST_MAC(devMemInfoPtr, HA_UnitMem));
    smemLionB0UnitRxDma(devObjPtr,UNIT_CHUNK_PTR_NO_CAST_MAC(devMemInfoPtr, RX_DMA_UnitMem));
    smemLionB0UnitTxDma(devObjPtr,UNIT_CHUNK_PTR_NO_CAST_MAC(devMemInfoPtr, TX_DMA_UnitMem));
    smemLionUnitMppm_(devObjPtr,UNIT_CHUNK_PTR_NO_CAST_MAC(devMemInfoPtr, _MPPM_UnitMem));
}

/*******************************************************************************
*   smemLionAllocSpecMemory
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
static void smemLionAllocSpecMemory
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_CHT_DEV_COMMON_MEM_INFO  * commonDevMemInfoPtr
)
{
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = devObjPtr->deviceMemory;
    SMEM_UNIT_CHUNKS_STC   MEM_UnitMem;
    SMEM_UNIT_CHUNKS_STC   TXDMA_UnitMem;

    /* init the internal memory */
    smemChtInitInternalSimMemory(devObjPtr,commonDevMemInfoPtr);

    if(SKERNEL_IS_LION_REVISON_B0_DEV(devObjPtr))
    {
        smemLionB0AllocSpecMemory(devObjPtr);
        return;
    }
    /* create all the memories of the units */
    smemLionUnitMg(devObjPtr,UNIT_CHUNK_PTR_NO_CAST_MAC(devMemInfoPtr, MG_UnitMem));
    smemXCatA1UnitPex(devObjPtr,&devMemInfoPtr->PEX_UnitMem, devMemInfoPtr->common.pciUnitBaseAddr);
    smemLionUnitEgressAndTxq(devObjPtr,UNIT_CHUNK_PTR_NO_CAST_MAC(devMemInfoPtr, Egress_and_Txq_UnitMem));
    smemLionUnitL2i(devObjPtr,UNIT_CHUNK_PTR_NO_CAST_MAC(devMemInfoPtr, L2I_UnitMem));
    smemLionUnitIpvx(devObjPtr,UNIT_CHUNK_PTR_NO_CAST_MAC(devMemInfoPtr, IPvX_UnitMem));
    smemLionUnitBm(devObjPtr,UNIT_CHUNK_PTR_NO_CAST_MAC(devMemInfoPtr, BM_UnitMem));
    smemLionUnitLms(devObjPtr,UNIT_CHUNK_PTR_NO_CAST_MAC(devMemInfoPtr, LMS_UnitMem));
    smemLionUnitFdb(devObjPtr,UNIT_CHUNK_PTR_NO_CAST_MAC(devMemInfoPtr, FDB_UnitMem));
    smemLionUnitMppm(devObjPtr,UNIT_CHUNK_PTR_NO_CAST_MAC(devMemInfoPtr, MPPM_UnitMem));
    smemLionUnitHaAndEpcl(devObjPtr,UNIT_CHUNK_PTR_NO_CAST_MAC(devMemInfoPtr, HA_and_EPCL_UnitMem));
    smemLionUnitMem(devObjPtr,&MEM_UnitMem);
    smemLionUnitTxdma(devObjPtr,&TXDMA_UnitMem);
    smemLionUnitCentralizedCounters(devObjPtr,UNIT_CHUNK_PTR_NO_CAST_MAC(devMemInfoPtr, Centralized_Counters_UnitMem));
    smemLionUnitXgPorts(devObjPtr,UNIT_CHUNK_PTR_NO_CAST_MAC(devMemInfoPtr, XGPorts_UnitMem));
    smemLionUnitXGPortMib(devObjPtr,UNIT_CHUNK_PTR_NO_CAST_MAC(devMemInfoPtr, XGPort_MIB_UnitMem));
    smemLionUnitLpSerdes(devObjPtr,UNIT_CHUNK_PTR_NO_CAST_MAC(devMemInfoPtr, LP_SERDES_UnitMem));
    smemLionUnitVlan_(devObjPtr,UNIT_CHUNK_PTR_NO_CAST_MAC(devMemInfoPtr, _VLAN_UnitMem));
    smemLionUnitGigPorts(devObjPtr,UNIT_CHUNK_PTR_NO_CAST_MAC(devMemInfoPtr, GigPorts_UnitMem));
    smemLionUnitEq(devObjPtr,UNIT_CHUNK_PTR_NO_CAST_MAC(devMemInfoPtr, Eq_UnitMem));
    smemLionUnitIpcl(devObjPtr,UNIT_CHUNK_PTR_NO_CAST_MAC(devMemInfoPtr, IPCL_UnitMem));
    smemLionUnitIplr(devObjPtr,UNIT_CHUNK_PTR_NO_CAST_MAC(devMemInfoPtr, IPLR_UnitMem));
    smemLionUnitMll(devObjPtr,UNIT_CHUNK_PTR_NO_CAST_MAC(devMemInfoPtr, MLL_UnitMem));
    smemLionUnitIpclTcc(devObjPtr,UNIT_CHUNK_PTR_NO_CAST_MAC(devMemInfoPtr, IPCL_TCC_UnitMem));
    smemLionUnitIpvxTcc(devObjPtr,UNIT_CHUNK_PTR_NO_CAST_MAC(devMemInfoPtr, IPvX_TCC_UnitMem));

    smemLionUnitMppm_(devObjPtr,UNIT_CHUNK_PTR_NO_CAST_MAC(devMemInfoPtr, _MPPM_UnitMem));

    /*add the MEM unit chunks to the HA_and_EPCL unit */
    smemInitMemCombineUnitChunks(devObjPtr,UNIT_CHUNK_PTR_NO_CAST_MAC(devMemInfoPtr, HA_and_EPCL_UnitMem),&MEM_UnitMem);
    /*add the TXDMA unit chunks to the HA_and_EPCL unit */
    smemInitMemCombineUnitChunks(devObjPtr,UNIT_CHUNK_PTR_NO_CAST_MAC(devMemInfoPtr, HA_and_EPCL_UnitMem),&TXDMA_UnitMem);

    devMemInfoPtr->common.unitChunksBasePtr = (GT_U8 *)UNIT_CHUNK_PTR_NO_CAST_MAC(devMemInfoPtr, MG_UnitMem);
    /* Calculate summary of all memory structures - for future SW reset use */
    devMemInfoPtr->common.unitChunksSizeOf =
        sizeof(SMEM_CHT_GENERIC_DEV_MEM_INFO) - sizeof(SMEM_CHT_DEV_COMMON_MEM_INFO);

}

/*******************************************************************************
*   smemLionAllocSpecMemoryPortGroupShared
*
* DESCRIPTION:
*       Allocate address type specific memories.
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
static void smemLionAllocSpecMemoryPortGroupShared
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_CHT_GENERIC_DEV_MEM_INFO  * devMemInfoPtr = devObjPtr->deviceMemory;

    /* create all the memories of the units */
    smemLionUnitBma(devObjPtr,UNIT_CHUNK_PTR_NO_CAST_MAC(devMemInfoPtr, BMA_UnitMem));
}

/*******************************************************************************
*  smemLionTableInfoSet
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
void smemLionTableInfoSet
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    /* Override Lion B0 tables info */
    if(SKERNEL_IS_LION_REVISON_B0_DEV(devObjPtr))
    {
        /* must be set before calling smemXcatA1TableInfoSet(...) */
        /* use xcat A1 table addresses */
        smemXcatA1TableInfoSet(devObjPtr);

        devObjPtr->globalInterruptCauseRegister.txqSht     = 5;
        devObjPtr->globalInterruptCauseRegister.txqDq      = 6;
        devObjPtr->globalInterruptCauseRegister.tti        = 7;
        devObjPtr->globalInterruptCauseRegister.tccLower   = 8;
        devObjPtr->globalInterruptCauseRegister.tccUpper   = 9;
        devObjPtr->globalInterruptCauseRegister.bcn        = SMAIN_NOT_VALID_CNS;
        devObjPtr->globalInterruptCauseRegister.ha         = 10;
        devObjPtr->globalInterruptCauseRegister.misc       = 11;
        devObjPtr->globalInterruptCauseRegister.mem        = SMAIN_NOT_VALID_CNS;
        devObjPtr->globalInterruptCauseRegister.txq        = SMAIN_NOT_VALID_CNS;
        devObjPtr->globalInterruptCauseRegister.txqll      = 12;
        devObjPtr->globalInterruptCauseRegister.txqQueue   = 13;
        devObjPtr->globalInterruptCauseRegister.l2i        = 14;
        devObjPtr->globalInterruptCauseRegister.bm0        = SMAIN_NOT_VALID_CNS;
        devObjPtr->globalInterruptCauseRegister.bm1        = SMAIN_NOT_VALID_CNS;
        devObjPtr->globalInterruptCauseRegister.txqEgr     = 15;
        devObjPtr->globalInterruptCauseRegister.xlgPort    = 16;
        devObjPtr->globalInterruptCauseRegister.fdb        = 17;
        devObjPtr->globalInterruptCauseRegister.ports      = 18;
        devObjPtr->globalInterruptCauseRegister.cpuPort    = 19;
        devObjPtr->globalInterruptCauseRegister.global1    = 20;
        devObjPtr->globalInterruptCauseRegister.txSdma     = 21;
        devObjPtr->globalInterruptCauseRegister.rxSdma     = 22;
        devObjPtr->globalInterruptCauseRegister.pcl        = 23;
        devObjPtr->globalInterruptCauseRegister.port24     = SMAIN_NOT_VALID_CNS;
        devObjPtr->globalInterruptCauseRegister.port25     = SMAIN_NOT_VALID_CNS;
        devObjPtr->globalInterruptCauseRegister.port26     = SMAIN_NOT_VALID_CNS;
        devObjPtr->globalInterruptCauseRegister.port27     = SMAIN_NOT_VALID_CNS;
        devObjPtr->globalInterruptCauseRegister.iplr0      = 24;
        devObjPtr->globalInterruptCauseRegister.iplr1      = 25;
        devObjPtr->globalInterruptCauseRegister.rxDma      = 26;
        devObjPtr->globalInterruptCauseRegister.txDma      = 27;
        devObjPtr->globalInterruptCauseRegister.eq         = 28;
        devObjPtr->globalInterruptCauseRegister.bma        = SMAIN_NOT_VALID_CNS;
        devObjPtr->globalInterruptCauseRegister.eplr       = 31;

        /* like in xcat A0 */
        {/* ipcl TCC */
            devObjPtr->tablesInfo.pclAction.commonInfo.baseAddress = 0x0D0B0000;
            devObjPtr->tablesInfo.pclTcam.commonInfo.baseAddress = 0x0D040000;
            devObjPtr->tablesInfo.pclTcamMask.commonInfo.baseAddress = devObjPtr->tablesInfo.pclTcam.commonInfo.baseAddress + 0x20;
        }

        devObjPtr->tablesInfo.routerTcam.commonInfo.baseAddress = 0x0D840000;

        {/* mll */
            devObjPtr->tablesInfo.mll.commonInfo.baseAddress = 0x0C880000;
            devObjPtr->tablesInfo.mllOutInterfaceConfig.commonInfo.baseAddress = 0x0C800980;
            devObjPtr->tablesInfo.mllOutInterfaceCounter.commonInfo.baseAddress = 0x0C800900;
            devObjPtr->memUnitBaseAddrInfo.mll = 0x0C800000;
        }

        devObjPtr->tablesInfo.stp.commonInfo.baseAddress = 0x118D0000;

        devObjPtr->tablesInfo.mcast.commonInfo.baseAddress = 0x11860000;

        devObjPtr->tablesInfo.vlan.commonInfo.baseAddress = 0x118A0000;
        devObjPtr->tablesInfo.vlan.paramInfo[0].step = 0x20;

        /* not valid table any more -- the vrfId is part of the vlan entry */
        devObjPtr->tablesInfo.vrfId.commonInfo.baseAddress = 0;

        /* override with values that like xCat-A1 for next tables */
        devObjPtr->tablesInfo.arp.commonInfo.baseAddress = 0x0E840000;
        devObjPtr->tablesInfo.tunnelStart.commonInfo.baseAddress = devObjPtr->tablesInfo.arp.commonInfo.baseAddress;
        devObjPtr->tablesInfo.tunnelStart.paramInfo[0].step = devObjPtr->tablesInfo.arp.paramInfo[0].step;

        devObjPtr->tablesInfo.vlanPortMacSa.commonInfo.baseAddress = 0x0E808000;
        devObjPtr->tablesInfo.ingressVlanTranslation.commonInfo.baseAddress = 0x0100C000;/*TTI*/
        devObjPtr->tablesInfo.egressVlanTranslation.commonInfo.baseAddress = 0x0E810000;/*HA*/

        devObjPtr->tablesInfo.ttiAction.commonInfo.baseAddress = 0x0D8C0000;

        devObjPtr->tablesInfo.deviceMapTable.commonInfo.baseAddress = 0x11870000;
        devObjPtr->tablesInfo.deviceMapTable.paramInfo[0].step       = 0x4;
        devObjPtr->tablesInfo.deviceMapTable.paramInfo[1].step       = 0;

        devObjPtr->tablesInfo.dscpToDscpMap.commonInfo.baseAddress    = 0x01000900;
        devObjPtr->tablesInfo.dscpToQoSProfile.commonInfo.baseAddress = 0x01000400;
        /* CFI UP -> QosProfile - table 0 */
        devObjPtr->tablesInfo.upToQoSProfile.commonInfo.baseAddress   = 0x01000440;
        devObjPtr->tablesInfo.upToQoSProfile.paramInfo[1].step  = 0x8;    /* CFI */
        devObjPtr->tablesInfo.upToQoSProfile.paramInfo[2].step = 0x10;    /*upProfile*/

        devObjPtr->tablesInfo.expToQoSProfile.commonInfo.baseAddress  = 0x01000460;

        devObjPtr->tablesInfo.cfiUpQoSProfileSelect.commonInfo.baseAddress = 0x01000470;
        devObjPtr->tablesInfo.cfiUpQoSProfileSelect.paramInfo[0].step = 0x4;

        devObjPtr->tablesInfo.macToMe.commonInfo.baseAddress = 0x01001600;

        devObjPtr->tablesInfo.portVlanQosConfig.commonInfo.baseAddress = 0x01001000;
        devObjPtr->tablesInfo.portVlanQosConfig.paramInfo[0].step = 0x10;

        devObjPtr->tablesInfo.portProtocolVidQoSConf.commonInfo.baseAddress = 0x01004000;

        devObjPtr->tablesInfo.ipclUserDefinedBytesConf.commonInfo.baseAddress = 0x01010000;

        devObjPtr->tablesInfo.pearsonHash.commonInfo.baseAddress = 0x0B8005C0;
        devObjPtr->tablesInfo.pearsonHash.paramInfo[0].step = 0x4;

        devObjPtr->tablesInfo.crcHashMask.commonInfo.baseAddress = 0x0B800400;
        devObjPtr->tablesInfo.crcHashMask.paramInfo[0].step = 0x10;

        devObjPtr->tablesInfo.pclTcam.commonInfo.baseAddress = 0x0D040000;
        devObjPtr->tablesInfo.pclTcamMask.commonInfo.baseAddress = devObjPtr->tablesInfo.pclTcam.commonInfo.baseAddress + 0x20;

        devObjPtr->tablesInfo.tcamBistArea.commonInfo.baseAddress = 0x0D000400;
        devObjPtr->tablesInfo.tcamBistArea.paramInfo[0].step = 0x4;

        devObjPtr->tablesInfo.tcamArrayCompareEn.commonInfo.baseAddress = 0x0D001000;
        devObjPtr->tablesInfo.tcamArrayCompareEn.paramInfo[0].step = 0x4;

        devObjPtr->tablesInfo.haUp0PortKeepVlan1.commonInfo.baseAddress = 0x0E800440;
        devObjPtr->tablesInfo.haUp0PortKeepVlan1.paramInfo[0].step = 0x4;

        devObjPtr->tablesInfo.egressVlan.commonInfo.baseAddress = 0x11800000;
        devObjPtr->tablesInfo.egressVlan.paramInfo[0].step = 0x40;

        devObjPtr->tablesInfo.egressStp.commonInfo.baseAddress = 0x11840000;
        devObjPtr->tablesInfo.egressStp.paramInfo[0].step = 0x10;

        devObjPtr->tablesInfo.l2PortIsolation.commonInfo.baseAddress = 0x11880000;
        devObjPtr->tablesInfo.l2PortIsolation.paramInfo[0].step = 0x10;

        devObjPtr->tablesInfo.l3PortIsolation.commonInfo.baseAddress = 0x11890000;
        devObjPtr->tablesInfo.l3PortIsolation.paramInfo[0].step = 0x10;

        devObjPtr->tablesInfo.nonTrunkMembers.commonInfo.baseAddress = 0x01810800;
        devObjPtr->tablesInfo.nonTrunkMembers.paramInfo[0].step = 0x8;

        devObjPtr->tablesInfo.designatedPorts.commonInfo.baseAddress = 0x01810200;
        devObjPtr->tablesInfo.designatedPorts.paramInfo[0].step = 0x8;

        devObjPtr->tablesInfo.sst.commonInfo.baseAddress = 0x01810500;
        devObjPtr->tablesInfo.sst.paramInfo[0].step = 0x8;

        devObjPtr->tablesInfo.secondTargetPort.commonInfo.baseAddress = 0x01801300;
        devObjPtr->tablesInfo.secondTargetPort.paramInfo[0].step = 0x4;

        devObjPtr->tablesInfo.egressStc.commonInfo.baseAddress = 0x11004400;
        devObjPtr->tablesInfo.egressStc.paramInfo[0].step = 0x10;/*local port*/
    }
    else
    {
        /* use xcat A0 table addresses */
        smemCht3TableInfoSet(devObjPtr);

        devObjPtr->globalInterruptCauseRegister.bma        = 29;

        /* restore override table */
        devObjPtr->tablesInfo.portProtocolVidQoSConf.commonInfo.baseAddress = 0x0B810800;
        devObjPtr->tablesInfo.portProtocolVidQoSConf.paramInfo[0].step = 0x20;
        devObjPtr->tablesInfo.portProtocolVidQoSConf.paramInfo[1].step = 0x4;

        /* override with values that like xCat-A1 for next tables */
        devObjPtr->tablesInfo.stp.commonInfo.baseAddress = 0x0A080000;

        devObjPtr->tablesInfo.mcast.commonInfo.baseAddress = 0x0A100000;

        devObjPtr->tablesInfo.vlan.commonInfo.baseAddress = 0x0A200000;
        devObjPtr->tablesInfo.vlan.paramInfo[0].step = 0x20;

        devObjPtr->tablesInfo.vrfId.commonInfo.baseAddress = 0x0A180000;

        devObjPtr->tablesInfo.pclConfig.commonInfo.baseAddress = 0x0B840000;
        devObjPtr->tablesInfo.pclConfig.paramInfo[0].step = 0x8; /*entry*/
        devObjPtr->tablesInfo.pclConfig.paramInfo[1].step = 0;

        devObjPtr->tablesInfo.deviceMapTable.paramInfo[0].step       = 0x80;/*per device*/
        devObjPtr->tablesInfo.deviceMapTable.paramInfo[1].step       = 0x10;/*per port>>3*/
    }

}

/*******************************************************************************
*   smemLionInit
*
* DESCRIPTION:
*       Init memory module for the Lion device.
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
void smemLionInit
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_CHT_DEV_COMMON_MEM_INFO * commonDevMemInfoPtr;

    GT_U32  ii;

    if(devObjPtr->numOfCoreDevs)
    {
        SMEM_CHT_GENERIC_DEV_MEM_INFO  * devMemInfoPtr;
        GT_U32  ii;
        /* this is the port group shared device */
        /* so we need to fill info about it's port group devices */
        for( ii = 0 ; ii < devObjPtr->numOfCoreDevs ;ii++)
        {
            /* each of the port group devices start his ports at multiple of 16
               (each port group has 12 ports) */
            devObjPtr->coreDevInfoPtr[ii].startPortNum = 16*ii;
        }

        devMemInfoPtr = smemDeviceObjMemoryAlloc(devObjPtr,1, sizeof(SMEM_CHT_GENERIC_DEV_MEM_INFO));
        if (devMemInfoPtr == 0)
        {
            skernelFatalError("smemLionInit: allocation error\n");
        }

        devObjPtr->deviceMemory = devMemInfoPtr;
        commonDevMemInfoPtr = &devMemInfoPtr->common;
        commonDevMemInfoPtr->isPartOfGeneric = 1;

        /* set the pointer to the active memory */
        devObjPtr->activeMemPtr = smemLionPortGroupSharedActiveTable;
    }
    else
    {
        SMEM_CHT_GENERIC_DEV_MEM_INFO  * devMemInfoPtr;

        devMemInfoPtr = smemDeviceObjMemoryAlloc(devObjPtr,1, sizeof(SMEM_CHT_GENERIC_DEV_MEM_INFO));
        if (devMemInfoPtr == 0)
        {
            skernelFatalError("smemLionInit: allocation error\n");
        }

        devObjPtr->deviceMemory = devMemInfoPtr;
        commonDevMemInfoPtr= &devMemInfoPtr->common;
        commonDevMemInfoPtr->isPartOfGeneric = 1;


        /* bind the router TCAM info */
        devObjPtr->routeTcamInfo =  lionPortGroupRoutTcamInfo;
        /* bind the PCL TCAM info */
        devObjPtr->pclTcamInfoPtr = &lionPortGroupGlobalPclData;
        devObjPtr->pclTcamMaxNumEntries = devObjPtr->pclTcamInfoPtr->bankEntriesNum * 4;

        devObjPtr->fdbNumEntries = SMEM_MAC_TABLE_SIZE_32KB;
        devObjPtr->fdbMaxNumEntries = devObjPtr->fdbNumEntries;
        devObjPtr->cncBlocksNum = 1;
        devObjPtr->cncBlockMaxRangeIndex = 2048;
        devObjPtr->supportEqEgressMonitoringNumPorts = 64;
        devObjPtr->policerEngineNum = 1;
        devObjPtr->deviceMapTableUseDstPort = 1;
        devObjPtr->supportVlanEntryVidxFloodMode = 1;
        devObjPtr->errata.routerGlueResetPartsOfDescriptor = 1;
        devObjPtr->errata.ttCopyToCpuWithAdditionalTag = 1;

        if(devObjPtr->deviceRevisionId == 0)
        {
            /* set the pointer to the active memory */
            devObjPtr->activeMemPtr = smemPortGroupActiveTable;

            devObjPtr->isXcatA0Features = 1;
            devObjPtr->errata.lttRedirectIndexError = 1;

        }
        else
        {
            devObjPtr->errata.fastStackFailover_Drop_on_source = 1;

            devObjPtr->memUnitBaseAddrInfo.policer[0] = policerBaseAddr[0];
            devObjPtr->memUnitBaseAddrInfo.policer[1] = policerBaseAddr[1];
            devObjPtr->memUnitBaseAddrInfo.policer[2] = policerBaseAddr[2];
            /* Timestamp base addresses */
            devObjPtr->memUnitBaseAddrInfo.gts[0] = timestampBaseAddr[0];
            devObjPtr->memUnitBaseAddrInfo.gts[1] = timestampBaseAddr[1];

            /* set the pointer to the active memory */
            devObjPtr->activeMemPtr = smemLionB0PortGroupActiveTable;

            devObjPtr->isXcatA0Features = 0;
            devObjPtr->supportTunnelStartEthOverIpv4 = 1;
            devObjPtr->errata.lttRedirectIndexError = 0;
            devObjPtr->errata.ieeeReservedMcConfigRegRead = 1;
            devObjPtr->errata.ipfixWrapArroundFreezeMode = 1;
            devObjPtr->errata.srcTrunkPortIsolationAsSrcPort = 1;
            devObjPtr->errata.crcTrunkHashL4InfoInIpv4Other = 1;

            devObjPtr->cncBlocksNum = 8;
            devObjPtr->cncBlockMaxRangeIndex = 512;
            devObjPtr->supportPortIsolation = 1;
            devObjPtr->supportOamPduTrap = 1;
            devObjPtr->supportPortOamLoopBack = 1;
            devObjPtr->policerEngineNum = 2;
            devObjPtr->supportMultiAnalyzerMirroring = 1;
            devObjPtr->supportCascadeTrunkHashDstPort = 1;
            devObjPtr->supportMultiPortGroupFdbLookUpMode = 1;
            devObjPtr->supportMultiPortGroupTTILookUpMode = 1;
            devObjPtr->supportTxQGlobalPorts = 1;
            devObjPtr->supportCrcTrunkHashMode = 1;
            devObjPtr->supportMacHeaderModification = 1;
            devObjPtr->supportHa64Ports = 1;
            devObjPtr->txqRevision = 1;
            devObjPtr->supportXgMacMibCountersControllOnAllPorts = 1;

            devObjPtr->supportLogicalTargetMapping = 1;
            devObjPtr->ipFixTimeStampClockDiff[0] =
            devObjPtr->ipFixTimeStampClockDiff[1] =
            devObjPtr->ipFixTimeStampClockDiff[2] = 0;
            devObjPtr->supportConfigurableTtiPclId = 1;
            devObjPtr->supportVlanEntryVidxFloodMode = 1;
            devObjPtr->supportForceNewDsaToCpu = 1;
            devObjPtr->supportCutThrough = 1;
            devObjPtr->supportGemUserId = 1;
            devObjPtr->supportKeepVlan1 = 1;
            devObjPtr->supportResourceHistogram = 1;

            devObjPtr->ipvxSupport.lttMaxNumberOfPaths = SKERNEL_LTT_MAX_NUMBER_OF_PATHS_64_E;
            devObjPtr->ipvxSupport.ecmpIndexFormula = SKERNEL_ECMP_ROUTE_INDEX_FORMULA_TYPE_1_E;
            devObjPtr->ipvxSupport.qosIndexFormula = SKERNEL_QOS_ROUTE_INDEX_FORMULA_TYPE_1_E;
            devObjPtr->ipvxSupport.supportPerVlanURpfMode = 1;
            devObjPtr->ipvxSupport.supportPerPortSipSaCheckEnable = 1;

            devObjPtr->supportMacSaAssignModePerPort = 1;
            devObjPtr->supportMaskAuFuMessageToCpuOnNonLocal = 1;

            devObjPtr->pclSupport.iPclSupport5Ude = 1;
            devObjPtr->pclSupport.iPclSupportTrunkHash = 1;
            devObjPtr->pclSupport.ePclSupportVidUpTag1 = 1;

            devObjPtr->supportDevMapTableOnOwnDev   = 1;
            devObjPtr->policerSupport.supportPolicerEnableCountingTriggerByPort   = 1;
            devObjPtr->policerSupport.supportIpfixTimeStamp = 1;
            devObjPtr->supportPtp = 1;

            devObjPtr->policerSupport.supportPolicerMemoryControl = 1;
            devObjPtr->policerSupport.iplr0TableSize = POLICER_MEMORY_1792_CNS;
            devObjPtr->policerSupport.iplr1TableSize = POLICER_MEMORY_256_CNS;
            devObjPtr->policerSupport.iplrTableSize =
                devObjPtr->policerSupport.iplr0TableSize +
                devObjPtr->policerSupport.iplr1TableSize;
            devObjPtr->cncClientSupportBitmap = SNET_CHT3_CNC_CLIENTS_BMP_ALL_CNS;
            devObjPtr->notSupportIngressPortRateLimit = 1;
            devObjPtr->supportForwardFcPackets = 1;

            for(ii = 0 ; ii < smemLionB0PortGroupActiveTableSizeOf ; ii++)
            {
                if(smemLionB0PortGroupActiveTable[ii].mask ==
                    POLICER_ACTIVE_MEMORY_PLACE_HOLDER_MASK_CNS)
                {
                    GT_U32  tmpMask = SMEM_FULL_MASK_CNS;

                    if(smemLionB0PortGroupActiveTable[ii + 0].address == POLICER_MANAGEMENT_COUNTER_ADDR_CNS)
                    {
                        /* management counters */
                        /* 0x40 between sets , 0x10 between counters */
                        /* 0x00 , 0x10 , 0x20 , 0x30 */
                        /* 0x40 , 0x50 , 0x60 , 0x70 */
                        /* 0x80 , 0x90 , 0xa0 , 0xb0 */
                        tmpMask = POLICER_MANAGEMENT_COUNTER_MASK_CNS;
                    }

                    smemLionB0PortGroupActiveTable[ii + 0].address += policerBaseAddr[0];
                    smemLionB0PortGroupActiveTable[ii + 0].mask = tmpMask;

                    smemLionB0PortGroupActiveTable[ii + 1].address += policerBaseAddr[1];
                    smemLionB0PortGroupActiveTable[ii + 1].mask = tmpMask;

                    smemLionB0PortGroupActiveTable[ii + 2].address += policerBaseAddr[2];
                    smemLionB0PortGroupActiveTable[ii + 2].mask = tmpMask;

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

            devObjPtr->portsArr[SMEM_LION_XLG_PORT_12_NUM_CNS].state = SKERNEL_PORT_STATE_XLG_40G_E;

            /* Lion B1 and above related features */
            if (devObjPtr->deviceRevisionId >= 3)
            {
                devObjPtr->supportRemoveVlanTag1WhenEmpty = 1;
            }
        }

    }

    devObjPtr->devFindMemFunPtr = (void *)smemLionFindMem;

    commonDevMemInfoPtr->pciUnitBaseAddrMask = 0x000f0000;
    commonDevMemInfoPtr->pciUnitBaseAddr = 0x00070000;

    /* init specific functions array */
    smemLionInitFuncArray(devObjPtr,commonDevMemInfoPtr);

    /* allocate address type specific memories */
    if(! devObjPtr->portGroupSharedDevObjPtr)
    {
        smemLionAllocSpecMemoryPortGroupShared(devObjPtr);
    }
    else
    {
        smemLionAllocSpecMemory(devObjPtr,commonDevMemInfoPtr);
    }

    smemLionTableInfoSet(devObjPtr);
}

/*******************************************************************************
*   smemLionInit2
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
void smemLionInit2
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    if(devObjPtr->numOfCoreDevs == 0)
    {
        /* call the DXCH generic initialization of 'part2' */
        smemChtInit2(devObjPtr);

        /* Set port group ID */
        smemRegFldSet(devObjPtr, SMEM_CHT_GLB_CTRL_REG(devObjPtr), 9, 2, devObjPtr->portGroupId);

        /* need to handle initialization of the SERDESs , like done in smemCht3Init2 */
    }

    return;
}

/*******************************************************************************
*  smemLionActiveWriteSchedulerConfigReg
*
* DESCRIPTION:
*       Set the <UpdateSchedVarTrigger> field in the Scheduler Configuration
*       Register which loads the new profile configuration.
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
void smemLionActiveWriteSchedulerConfigReg
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

    /* Clear bit[6] in Scheduler Configuration Register in order to flag that updates were performed */
    reg_value &= ~(1<<6);

    /* write to simulation memory */
    *memPtr = reg_value;

    return;
}

/*******************************************************************************
*  smemLionActiveReadPtpMessage
*
* DESCRIPTION:
*      Read PTP Message to CPU Register - simulate read from timestamp fifo.
*
* INPUTS:
*       devObjPtr - device object PTR.
*       address     - Address for ASIC memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers' specific parameter: 0  ingress GTS , 1  egress GTS
*
* OUTPUTS:
*       outMemPtr   - Pointer to the memory to copy register's content.
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void smemLionActiveReadPtpMessage
(
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    OUT        GT_U32 * outMemPtr
)
{
    GT_U32 * fifoBufferPtr;                 /* fifo memory pointer */
    GT_U32 i;

    /* Set pointer to the common memory */
    SMEM_CHT_DEV_COMMON_MEM_INFO  * memInfoPtr =
        (SMEM_CHT_DEV_COMMON_MEM_INFO *)(deviceObjPtr->deviceMemory);

    /* Get pointer to the start of timestamp fifo */
    if(param == SMAIN_DIRECTION_INGRESS_E)
    {
        fifoBufferPtr = &memInfoPtr->gtsIngressFifoMem.gtsFifoRegs[0];
    }
    else
    {
        fifoBufferPtr = &memInfoPtr->gtsEgressFifoMem.gtsFifoRegs[0];
    }

    /* Output value */
    *outMemPtr = *fifoBufferPtr;

    if (*fifoBufferPtr == 0xffffffff)
    {
        /* Buffer is empty, there is no new message */
        return;
    }

    /* Shift other words up */
    for (i = 1; i < GTS_FIFO_REGS_NUM; i++)
    {
        fifoBufferPtr[i - 1] = fifoBufferPtr[i];
    }

    /* Invalidate last word */
    fifoBufferPtr[GTS_FIFO_REGS_NUM - 1] = 0xffffffff;
}

/*******************************************************************************
*  smemLionActiveWriteTodShadowNanosecondsReg
*
* DESCRIPTION:
*       Global configuration settings and TOD function triggering
*
* INPUTS:
*       devObjPtr   - device object PTR.
*       address     - Address for ASIC memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers' specific parameter: 0  ingress GTS , 1  egress GTS
*       inMemPtr    - Pointer to the memory to get register's content.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void smemLionActiveWriteTodGlobalReg
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    INOUT      GT_U32 * inMemPtr
)
{
    GT_U32 fieldVal;
    SNET_LION_PTP_TOD_EVENT_ENT event;

    /* Output value */
    *memPtr = *inMemPtr;

    fieldVal = SMEM_U32_GET_FIELD(*inMemPtr, 6, 1);
    /* TOD Counter Function not triggered */
    if(fieldVal == 0)
    {
        return;
    }

    fieldVal = SMEM_U32_GET_FIELD(*inMemPtr, 4, 2);
    /* TOD Counter Function  */
    switch(fieldVal)
    {
        case 0:
            event = SNET_LION_PTP_TOD_UPDATE_E;
            break;
        case 1:
            event = SNET_LION_PTP_TOD_INCREMENT_E;
            break;
        case 2:
            event = SNET_LION_PTP_TOD_CAPTURE_E;
            break;
        case 3:
            event = SNET_LION_PTP_TOD_GENERATE_E;
            break;
        default:
            return;
    }

    snetLionPtpTodCounterApply(devObjPtr, param, event);

    if( (event != SNET_LION_PTP_TOD_CAPTURE_E) &&
        (event != SNET_LION_PTP_TOD_GENERATE_E) )
    {
        /* Update clock reference value for TOD calculation */
        devObjPtr->todTimeStampClockDiff[param] = SIM_OS_MAC(simOsTickGet)();
    }

    /* Clear TOD Counter Function Trigger bit once the action is done */
    SMEM_U32_SET_FIELD(*memPtr, 6, 1, 0);
}

/*******************************************************************************
*  smemLionActiveReadPtpTodSeconds
*
* DESCRIPTION:
*      Read PTP TOD seconds register.
*
* INPUTS:
*       devObjPtr - device object PTR.
*       address     - Address for ASIC memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers' specific parameter: 0  ingress GTS , 1  egress GTS
*
* OUTPUTS:
*       outMemPtr   - Pointer to the memory to copy register's content.
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void smemLionActiveReadPtpTodSeconds
(
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    OUT        GT_U32 * outMemPtr
)
{
    GT_U64 seconds64;

    /* Read TOD counter 64 bit seconds */
    snetLionPtpTodCounterSecondsRead(deviceObjPtr, param, &seconds64);

    if((address & 0xff) == 0x14)
    {
        /* Low word of second field of the TOD Counter */
        *outMemPtr = seconds64.l[0];
    }
    else
    {
        /* High word of second field of the TOD Counter */
        *outMemPtr = seconds64.l[1];
    }

}

/*******************************************************************************
*  smemLionActiveReadPtpTodNanoSeconds
*
* DESCRIPTION:
*      Read PTP TOD nano seconds register.
*
* INPUTS:
*       devObjPtr - device object PTR.
*       address     - Address for ASIC memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers' specific parameter: 0  ingress GTS , 1  egress GTS
*
* OUTPUTS:
*       outMemPtr   - Pointer to the memory to copy register's content.
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void smemLionActiveReadPtpTodNanoSeconds
(
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    OUT        GT_U32 * outMemPtr
)
{
    GT_U32 nanoseconds;

    /* Read TOD counter nanoseconds */
    snetLionPtpTodCounterNanosecondsRead(deviceObjPtr, param, &nanoseconds);

    *outMemPtr = nanoseconds;
}

/*******************************************************************************
*  smemLionActiveWriteDequeueEnableReg
*
* DESCRIPTION:
*       Process Dequeue Enable Register which loads the new
*       tc configuration (tc enable/disable).
*
* INPUTS:
*       devObjPtr   - device object PTR.
*       address     - Address for ASIC memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers' specific parameter
*       inMemPtr    - Pointer to the memory to get register's content.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
GT_VOID smemLionActiveWriteDequeueEnableReg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32   address,
    IN GT_U32   memSize,
    IN GT_U32 * memPtr,
    IN GT_UINTPTR   param,
    IN GT_U32 * inMemPtr
)
{
    GT_U32  port;

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        /* exact match for all 72 registers is achieved */
        /* get port num */
        port = param/*the start port from 0x00*/ + ((address & 0xFF) / 0x4);
        smemTxqSendDequeueMessages(devObjPtr, *inMemPtr, port, 0);
    }
    else
    /* check address first byte range 0x14...0x58 */
    if((address & 0xFF) >= 0x14 && (address & 0xFF) <= 0x58)
    {
        /* get port num */
        port = ((address & 0xFF) - 0x14) / 0x4;

        smemTxqSendDequeueMessages(devObjPtr, *inMemPtr, port, 0);
    }

    /* Update the register value */
    *memPtr = *inMemPtr;
    return;
}

