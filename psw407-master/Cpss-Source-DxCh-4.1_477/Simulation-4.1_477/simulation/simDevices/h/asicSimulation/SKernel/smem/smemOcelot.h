/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* smemOcelot.h
*
* DESCRIPTION:
*       This file includes the declaration of the structure to hold the
*       addresses of registers and tables in Ocelot simulation.
*       -- Ocelot devices
*
* FILE REVISION NUMBER:
*       $Revision: 6 $
*
*******************************************************************************/
#ifndef __smemOceloth
#define __smemOceloth

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <asicSimulation/SKernel/smem/smemFAP20M.h>

typedef struct
{
        GT_U32    rxPktsCntrLSB;
        GT_U32    rxPktsCntrMSB;
        GT_U32    rxBadPktsCntrLSB;
        GT_U32    rxBadPktsCntrMSB;

        GT_U32    laneConfig0;
        GT_U32    laneConfig1;

        GT_U32    cyclicDataReg0;
        GT_U32    cyclicDataReg1;
        GT_U32    cyclicDataReg2;
        GT_U32    cyclicDataReg3;

        GT_U32    symbolErrorCntr;
        GT_U32    disparityErrorCntr;
        GT_U32    PRBSErrorCntr;

        GT_U32    laneStatus;
        GT_U32    laneInterruptCause;
        GT_U32    laneInterruptMask;

}LANE_REGS;

typedef struct{

    struct /*MG - Management unit (controls access to registers and tables) */{

        struct /*globalConfig*/{

            GT_U32    addrCompletion0;
            GT_U32    addrCompletion1;
            GT_U32    addrCompletion2;
            GT_U32    lastReadTimeStamp;
            GT_U32    deviceID;
            GT_U32    vendorID;
            GT_U32    globalCtrlReg;
            GT_U32    mgGlueConfig;

        }globalConfig;

        struct /*globalInterrupt*/{

            GT_U32    globalInterruptCause;
            GT_U32    globalInterruptMask;
            GT_U32    miscellaneousInterruptCause;
            GT_U32    miscellaneousInterruptMask;

        }globalInterrupt;

        struct /*interruptCoalescingConfig*/{

            GT_U32    interruptCoalescingConfig;

        }interruptCoalescingConfig;

        struct /*TWSIConfig*/{

            GT_U32    TWSIGlobalConfig;
            GT_U32    TWSILastAddr;
            GT_U32    TWSITimeoutLimit;
            GT_U32    TWSIStateHistory;
            GT_U32    TWSIInternalBaudRate;

        }TWSIConfig;

        struct /*userDefined*/{

            GT_U32    userDefinedReg0;
            GT_U32    userDefinedReg1;
            GT_U32    userDefinedReg2;
            GT_U32    userDefinedReg3;

        }userDefined;

    }MG /*- Management unit (controls access to registers and tables)*/ ;

    struct /*PEX - PCI express */{

        struct /*PEXAddrWindowCtrl*/{

            GT_U32    PEXWindow0Ctrl;
            GT_U32    PEXWindow0Base;
            GT_U32    PEXWindow0Remap;
            GT_U32    PEXWindow1Ctrl;
            GT_U32    PEXWindow1Base;
            GT_U32    PEXWindow1Remap;
            GT_U32    PEXWindow2Ctrl;
            GT_U32    PEXWindow2Base;
            GT_U32    PEXWindow2Remap;
            GT_U32    PEXWindow3Ctrl;
            GT_U32    PEXWindow3Base;
            GT_U32    PEXWindow3Remap;
            GT_U32    PEXWindow4Ctrl;
            GT_U32    PEXWindow4Base;
            GT_U32    PEXWindow4Remap;
            GT_U32    PEXWindow5Ctrl;
            GT_U32    PEXWindow5Base;
            GT_U32    PEXWindow5Remap;
            GT_U32    PEXDefaultWindowCtrl;

        }PEXAddrWindowCtrl;

        struct /*PEXBARCtrl*/{

            GT_U32    PEXBAR1Ctrl;
            GT_U32    PEXBAR2Ctrl;

        }PEXBARCtrl;

        struct /*PEXConfigCyclesGeneration*/{

            GT_U32    PEXConfigAddr;
            GT_U32    PEXConfigData;

        }PEXConfigCyclesGeneration;

        struct /*PEXConfigHeader*/{

            GT_U32    PEXDeviceAndVendorID;
            GT_U32    PEXCommandAndStatus;
            GT_U32    PEXClassCodeAndRevisionID;
            GT_U32    PEXBISTHeaderTypeAndCacheLineSize;
            GT_U32    PEXBAR0Internal;
            GT_U32    PEXBAR0InternalHigh;
            GT_U32    PEXBAR1;
            GT_U32    PEXBAR1High;
            GT_U32    PEXBAR2;
            GT_U32    PEXBAR2High;
            GT_U32    PEXSubsystemDeviceAndVendorID;
            GT_U32    PEXCapability;
            GT_U32    PEXDeviceCapabilities;
            GT_U32    PEXDeviceCtrlStatus;
            GT_U32    PEXLinkCapabilities;
            GT_U32    PEXLinkCtrlStatus;
            GT_U32    PEXAdvancedErrorReportHeader;
            GT_U32    PEXUncorrectableErrorStatus;
            GT_U32    PEXUncorrectableErrorMask;
            GT_U32    PEXUncorrectableErrorSeverity;
            GT_U32    PEXCorrectableErrorStatus;
            GT_U32    PEXCorrectableErrorMask;
            GT_U32    PEXAdvancedErrorCapabilityAndCtrl;
            GT_U32    PEXHeaderLogFirstDWORD;
            GT_U32    PEXHeaderLogSecondDWORD;
            GT_U32    PEXHeaderLogThirdDWORD;
            GT_U32    PEXHeaderLogFourthDWORD;

        }PEXConfigHeader;

        struct /*PEXCtrlAndStatus*/{

            GT_U32    PEXCtrl;
            GT_U32    PEXStatus;
            GT_U32    PEXCompletionTimeout;
            GT_U32    PEXFlowCtrl;
            GT_U32    PEXAcknowledgeTimers1X;
            GT_U32    PEXTLCtrl;

        }PEXCtrlAndStatus;

        struct /*PEXInterrupt*/{

            GT_U32    PEXInterruptCause;
            GT_U32    PEXInterruptMask;

        }PEXInterrupt;


    }PEX /*- PCI express*/ ;

    /* Unit 1 */
    SMEM_FAP20M_DEV_MEM_INFO  Dcore;  /* Dune registers */

    /* Unit 2 */
    struct /*ingr - ingress */{

        struct /*choppingConfig*/{

            GT_U32    choppingSize;
            GT_U32    choppedPktsCntr;
            GT_U32    outgoingChunksCntr;
            GT_U32    targetChoppingEnable[4]/*dev*/;
            GT_U32    choppingEnablers;
            GT_U32    perTrafficClassChoppingEnablers;

        }choppingConfig;

        struct /*cntr*/{

            GT_U32    queueNotValidCntr;
            GT_U32    CRCErrorLinkCntr[2]/*link*/;
            GT_U32    LLFCCntr[2]/*link*/;
            GT_U32    linkCPUMailCntr[2]/*link*/;
            GT_U32    linkPktErrorCntr[2]/*link*/;
            GT_U32    linkByteCountErrorCntr[2]/*link*/;
            GT_U32    UCLinkPktPassCntr[2]/*link*/;
            GT_U32    MCLinkPktPassCntr[2]/*link*/;
            GT_U32    linkUcFifoFullDropCntr[2]/*link*/;
            GT_U32    linkMcFifoFullDropCntr[2]/*link*/;
            GT_U32    linkDescFifoFullDropCntr[2]/*link*/;
            GT_U32    linkByteCountToBigDropCntr[2]/*link*/;

        }cntr;

        struct /*globalConfig*/{

            GT_U32    generalConfig;
            GT_U32    linkConfig[2]/*link*/;
            GT_U32    MCIDCalculation;
            GT_U32    destinationDescType[4]/*dev*/;
            GT_U32    interruptCauseReg;
            GT_U32    interruptMaskReg;
            GT_U32    descFIFOThreshold[2]/*uc_mc_link*/;
            GT_U32    dataFIFOThreshold[2][2]/*type*//*link*/;

        }globalConfig;

        struct /*labelingConfig*/{

            GT_U32    labelingGlobalConfig;
            GT_U32    basePerLink;
            GT_U32    UCPriorityMap;
            GT_U32    MCPriorityMap;
            GT_U32    ucQOS2PrioMap[12]/*qos*/;
            GT_U32    ucQOS2PrioMap12;
            GT_U32    mcQOS2PrioMap[12]/*qos*/;
            GT_U32    mcQOS2PrioMap12;
            GT_U32    analyzerParameters;
            GT_U32    mcBase;
            GT_U32    DXDPAssignment;
            GT_U32    QOS2DpMap[8]/*qos*/;

        }labelingConfig;


    }ingr /*- ingress*/ ;
    struct /*egr - egress */{

        struct /*EDQUnit*/{

            GT_U32    dequeueGlobalConfig;
            GT_U32    linkLevel2SDWRRWeight[2]/*link*/;
            GT_U32    linkSchedulerConfig[2]/*link number*/;
            GT_U32    linkSDWRRWeights[2]/*link number*/;

        }EDQUnit;

        struct /*EFCUnit*/{

            GT_U32    flowCtrlGlobalConfig;
            GT_U32    linkLevelFCThresholds;
            GT_U32    linkLevelFCGlobalThreshold;
            GT_U32    linkMCIBEThresholds[2]/*link number*/;
            GT_U32    linkMCIGuaranteedThresholds[2]/*link number*/;

        }EFCUnit;

        struct /*FMUnit*/{

            GT_U32    FMDXConfig;
            GT_U32    FMTC2TCUcMapTable;
            GT_U32    FMTC2TCMcMapTable;
            GT_U32    FMDXUcQosProfile;
            GT_U32    FMDXMcQosProfile;
            GT_U32    FMOutgoingMH;
            GT_U32    FMExtOutgoingMH;
            GT_U32    FMDXQoS2QoSUcMapTable[4]/*num of mapping register*/;
            GT_U32    FMDXQoS2QoSMcMapTable[4]/*num of mapping register*/;
            GT_U32    FMDXTargetDevID;
            GT_U32    FMHGLCellTCMapTable;
            GT_U32    FMDXSrcID2SrcIDMapTable[8]/*num of table entry*/;

        }FMUnit;

        struct /*globalConfig*/{

            GT_U32    egrGlobalConfig;
            GT_U32    egrQoSConfig;

        }globalConfig;

        struct /*PDPUnit*/{

            GT_U32    queueSelectionGlobalConfig;
            GT_U32    CMConstantContextID;
            GT_U32    tailDropLinkThresholds;
            GT_U32    tailDropPktThresholds;
            GT_U32    tailDropTotalThreshold;
            GT_U32    tailDropQueueThresholds[2]/*queue number*/;
            GT_U32    tailDropQueueThresholds1[2]/*queue number*/;
            GT_U32    qosProfileToPriorityMapTable[4]/*bits 5 to 6 of QoS Profile*/;

        }PDPUnit;

        struct /*WRDMAUnit*/{

            GT_U32    DXQosProfile2DPMapTable[8]/*entry number*/;

        }WRDMAUnit;


    }egr /*- egress*/ ;
    struct /*statistics - statistics */{

        struct /*copyOfSERDESConfigRegs*/{

            GT_U32    analogTestAndTBGCtrlReg;
            GT_U32    analogReceiverTransmitCtrlReg;
            GT_U32    analogAllLaneCtrlReg0;
            GT_U32    analogAllLaneCtrlReg1;
            GT_U32    VCOCalibrationCtrlReg;
            GT_U32    SERDESPowerAndResetCtrl;
            GT_U32    analogLaneTransmitterCtrlReg[6]/*Lane*/;
            GT_U32    laneVCOCalibrationCtrlReg[6]/*Lane*/;

        }copyOfSERDESConfigRegs;

        struct /*debug*/{

            GT_U32    ingrDebugFifoStatus;
            GT_U32    ingrDebugFifoData;
            GT_U32    egrDebugFifoStatus;
            GT_U32    egrDebugFifoData;

        }debug;

        struct /*egrConfig*/{

            GT_U32    egrStatisticConfig;
            GT_U32    egrMACParameters;
            GT_U32    egrIncomingEventsCntr;
            GT_U32    egrOutgoingEventsCntr;
            GT_U32    egrDropMsgCntr;

        }egrConfig;

        struct /*ingrConfig*/{

            GT_U32    ingrStatisticConfig;
            GT_U32    ingrMACParameters;
            GT_U32    ingrByteCountCompensation;
            GT_U32    ingrIncomingEventsCntr;
            GT_U32    ingrOutgoingEventsCntr;
            GT_U32    ingrParityErrorCntr;
            GT_U32    ingrDropMsgCntr;

        }ingrConfig;

        struct /*TMMAC*/{

            GT_U32    globalCtrl;
            GT_U32    transmitCellParameters;
            GT_U32    CRCErrorsCntr;

            GT_U32    laneDropCellsCntr[2]/*lane*/;
            GT_U32    laneInterruptCause[2]/*lane*/;
            GT_U32    laneInterruptMask[2]/*lane*/;

        }TMMAC;


    }statistics /*- statistics*/ ;
    struct /*flowCtrl - Flow Control */{

        GT_U32    schedulerStatusffset[8]/*offset*/;
        GT_U32    schedulerDefaultffset[8]/*offset*/;
        GT_U32    schedulerEnffset[8]/*offset*/;
        GT_U32    inBandFCConfigs;
        GT_U32    calendarConfigs;
        GT_U32    FCDropCntr[2]/*link*/;

    }flowCtrl /*- Flow Control*/ ;
    struct /*misc - misc */{

        struct /*debugConfig*/{

            GT_U32    debugCtrlReg;

        }debugConfig;

        struct /*globalConfig*/{

            GT_U32    sampledAtResetReg0;
            GT_U32    sampledAtResetReg1;

        }globalConfig;

        struct /*GPPRegs*/{

            GT_U32    GPPReg[2]/*gpp*/;

        }GPPRegs;

        struct /*PLLConfigRegs*/{

            GT_U32    PLLsConfig0Reg;
            GT_U32    PLLsConfig1Reg;
            GT_U32    PLLsConfig2Reg;

        }PLLConfigRegs;

        struct /*XSMI*/{

            GT_U32    XSMICtrlReg;

        }XSMI;


    }misc /*- misc*/ ;
    struct /*hyperGlink0shared - hyperGlink0 (shared)  */{

        struct /*hyperGLinkPortInterrupt*/{

            GT_U32    hyperGLinkMainInterruptCause;
            GT_U32    hyperGLinkMainInterruptMask;

        }hyperGLinkPortInterrupt;

        GT_U32    hyperGLinkPingCellTx;
        GT_U32    hyperGLinkPingCellRx;
        GT_U32    hyperGLinkMACConfig;
        GT_U32    hyperGLinkLinkLevelFlowCtrlTxConfig;
        GT_U32    hyperGLinkLinkLevelFlowCtrlEnable[2]/*offset*/;
        GT_U32    hyperGLinkRxFlowCtrlCellsCntr;
        GT_U32    hyperGLinkTxFlowCtrlCellsCntr;
        GT_U32    hyperGLinkMACDroppedReceivedCellCntrs;
        GT_U32    hyperGLinkLinkLevelFlowCtrlStatus[2]/*offset*/;


    }hyperGlink0shared /*- hyperGlink0 (shared) */ ;
    struct /*hyperGStack0shared - hyperG.Stack0 (shared)  */{

        struct /*hyperGStackPortsInterrupt*/{

            GT_U32    HGSPortInterruptCause;
            GT_U32    HGSPortInterruptMask;

        }hyperGStackPortsInterrupt;

        struct /*hyperGStackPortsMACConfig*/{

            GT_U32    portMACCtrlReg0;
            GT_U32    portMACCtrlReg1;
            GT_U32    portMACCtrlReg2;

        }hyperGStackPortsMACConfig;

        struct /*hyperGStackPortsStatus*/{

            GT_U32    portStatus;

        }hyperGStackPortsStatus;

    }hyperGStack0shared /*- hyperG.Stack0 (shared) */ ;

    struct /*convertor0shared - convertor0 (shared)  */{

        struct /*SERDESConfigRegs*/{

            GT_U32    analogTestAndTBGCtrlReg;
            GT_U32    analogReceiverTransmitCtrlReg;
            GT_U32    analogAllLaneCtrlReg0;
            GT_U32    analogAllLaneCtrlReg1;
            GT_U32    VCOCalibrationCtrlReg;
            GT_U32    SERDESPowerAndResetCtrl;
            GT_U32    analogLaneTransmitterCtrlReg[6]/*Lane*/;
            GT_U32    laneVCOCalibrationCtrlReg[6]/*Lane*/;

        }SERDESConfigRegs;

        GT_U32    convertorPortMACsAndMIBxReg;
        GT_U32    convertorPortGeneralReg;
        GT_U32    convertorMainInterruptCauseReg;
        GT_U32    convertorMainInterruptMaskReg;
        GT_U32    sourceAddrMiddle;
        GT_U32    sourceAddrHigh;
        GT_U32    convertorSummaryInterruptCauseReg;
        GT_U32    convertorSummaryInterruptMaskReg;

    }convertor0shared /*- convertor0 (shared) */ ;

    struct /*genericXPCS0shared - generic XPCS0 (shared)  */{

        struct /*globalRegs*/{

            GT_U32    txPktsCntrLSB;
            GT_U32    txPktsCntrMSB;

            GT_U32    globalConfig0;
            GT_U32    globalConfig1;

            GT_U32    globalDeskewErrorCntr;

            GT_U32    metalFix;

            GT_U32    globalStatus;
            GT_U32    globalInterruptCause;
            GT_U32    globalInterruptMask;

        }globalRegs;

        LANE_REGS laneRegs[6];

    }genericXPCS0shared /*- generic XPCS0 (shared) */ ;

    struct /*hyperGlink1shared - hyperGlink1 (shared)  */{

        struct /*hyperGLinkPortInterrupt*/{

            GT_U32    hyperGLinkMainInterruptCause;
            GT_U32    hyperGLinkMainInterruptMask;

        }hyperGLinkPortInterrupt;

        GT_U32    hyperGLinkPingCellTx;
        GT_U32    hyperGLinkPingCellRx;
        GT_U32    hyperGLinkMACConfig;
        GT_U32    hyperGLinkLinkLevelFlowCtrlTxConfig;
        GT_U32    hyperGLinkLinkLevelFlowCtrlEnable[2]/*offset*/;
        GT_U32    hyperGLinkRxFlowCtrlCellsCntr;
        GT_U32    hyperGLinkTxFlowCtrlCellsCntr;
        GT_U32    hyperGLinkMACDroppedReceivedCellCntrs;
        GT_U32    hyperGLinkLinkLevelFlowCtrlStatus[2]/*offset*/;

    }hyperGlink1shared /*- hyperGlink1 (shared) */ ;
    struct /*hyperGStack1shared - hyperG.Stack1 (shared)  */{

        struct /*hyperGStackPortsInterrupt*/{

            GT_U32    HGSPortInterruptCause;
            GT_U32    HGSPortInterruptMask;

        }hyperGStackPortsInterrupt;

        struct /*hyperGStackPortsMACConfig*/{

            GT_U32    portMACCtrlReg0;
            GT_U32    portMACCtrlReg1;
            GT_U32    portMACCtrlReg2;

        }hyperGStackPortsMACConfig;

        struct /*hyperGStackPortsStatus*/{

            GT_U32    portStatus;

        }hyperGStackPortsStatus;

    }hyperGStack1shared /*- hyperG.Stack1 (shared) */ ;
    struct /*convertor1shared - convertor1 (shared)  */{

        struct /*SERDESConfigRegs*/{

            GT_U32    analogTestAndTBGCtrlReg;
            GT_U32    analogReceiverTransmitCtrlReg;
            GT_U32    analogAllLaneCtrlReg0;
            GT_U32    analogAllLaneCtrlReg1;
            GT_U32    VCOCalibrationCtrlReg;
            GT_U32    SERDESPowerAndResetCtrl;
            GT_U32    analogLaneTransmitterCtrlReg[6]/*Lane*/;
            GT_U32    laneVCOCalibrationCtrlReg[6]/*Lane*/;

        }SERDESConfigRegs;

        GT_U32    convertorPortMACsAndMIBxReg;
        GT_U32    convertorPortGeneralReg;
        GT_U32    convertorMainInterruptCauseReg;
        GT_U32    convertorMainInterruptMaskReg;
        GT_U32    sourceAddrMiddle;
        GT_U32    sourceAddrHigh;
        GT_U32    convertorSummaryInterruptCauseReg;
        GT_U32    convertorSummaryInterruptMaskReg;

    }convertor1shared /*- convertor1 (shared) */ ;
    struct /*genericXPCS1shared - generic XPCS1 (shared)  */{

        struct /*globalRegs*/{

            GT_U32    txPktsCntrLSB;
            GT_U32    txPktsCntrMSB;

            GT_U32    globalConfig0;
            GT_U32    globalConfig1;

            GT_U32    globalDeskewErrorCntr;

            GT_U32    metalFix;

            GT_U32    globalStatus;
            GT_U32    globalInterruptCause;
            GT_U32    globalInterruptMask;

        }globalRegs;

        LANE_REGS laneRegs[6];

    }genericXPCS1shared /*- generic XPCS1 (shared) */ ;
    struct /*ftdll_Calibshared - ftdll_Calib (shared)  */{

        struct /*CALIBRATION UNIT*/{

            GT_U32    DDRPadsConfig0;
            GT_U32    DDRPadsConfig1;
            GT_U32    calibrationGlobalCtrlReg;
            GT_U32    calibrationStatusReg;
            GT_U32    grpPadCalibrationCtrlReg[10]/*calibration group number*/;

        }calibrationUnit;

        struct /*FTDLL UNIT REGISTERS DOC ONLY*/{

            GT_U32    FTDLLCtrlReg;
            GT_U32    FTDLLFilterReg;
            GT_U32    FTDLLSramAddrReg;
            GT_U32    FTDLLSramWriteData0Reg;
            GT_U32    FTDLLSramWriteData1Reg;
            GT_U32    FTDLLSramWriteData2Reg;
            GT_U32    FTDLLDfvReg;
            GT_U32    FTDLLSramReadData0Reg;
            GT_U32    FTDLLSramReadData1Reg;
            GT_U32    FTDLLSramReadData2Reg;

        }ftdllUnitRegsDocOnly;

    }ftdll_Calibshared /*- ftdll_Calib (shared) */ ;

    struct /*ftdll_Calib_1 - ftdll_Calib (shared)  */{

        struct /*CALIBRATION UNIT*/{

            GT_U32    DDRPadsConfig0;
            GT_U32    DDRPadsConfig1;
            GT_U32    calibrationGlobalCtrlReg;
            GT_U32    calibrationStatusReg;
            GT_U32    grpPadCalibrationCtrlReg[10]/*calibration group number*/;

        }calibrationUnit;

        struct /*FTDLL UNIT REGISTERS DOC ONLY*/{

            GT_U32    FTDLLCtrlReg;
            GT_U32    FTDLLFilterReg;
            GT_U32    FTDLLSramAddrReg;
            GT_U32    FTDLLSramWriteData0Reg;
            GT_U32    FTDLLSramWriteData1Reg;
            GT_U32    FTDLLSramWriteData2Reg;
            GT_U32    FTDLLDfvReg;
            GT_U32    FTDLLSramReadData0Reg;
            GT_U32    FTDLLSramReadData1Reg;
            GT_U32    FTDLLSramReadData2Reg;

        }ftdllUnitRegsDocOnly;


    }ftdll_Calib_1shared /*- ftdll_Calib 1(shared) */ ;

    struct /*ftdll_Calib_2 - ftdll_Calib 2(shared)  */{

        struct /*CALIBRATION UNIT*/{

            GT_U32    DDRPadsConfig0;
            GT_U32    DDRPadsConfig1;
            GT_U32    calibrationGlobalCtrlReg;
            GT_U32    calibrationStatusReg;
            GT_U32    grpPadCalibrationCtrlReg[10]/*calibration group number*/;

        }calibrationUnit;

        struct /*FTDLL UNIT REGISTERS DOC ONLY*/{

            GT_U32    FTDLLCtrlReg;
            GT_U32    FTDLLFilterReg;
            GT_U32    FTDLLSramAddrReg;
            GT_U32    FTDLLSramWriteData0Reg;
            GT_U32    FTDLLSramWriteData1Reg;
            GT_U32    FTDLLSramWriteData2Reg;
            GT_U32    FTDLLDfvReg;
            GT_U32    FTDLLSramReadData0Reg;
            GT_U32    FTDLLSramReadData1Reg;
            GT_U32    FTDLLSramReadData2Reg;

        }ftdllUnitRegsDocOnly;


    }ftdll_Calib_2shared /*- ftdll_Calib 2(shared) */ ;

    struct /*BM - BM */{

        struct /*bufferManagementInterrupt*/{

            GT_U32    bufferManagementInterruptCauseReg0;
            GT_U32    bufferManagementInterruptMaskReg0;

        }bufferManagementInterrupt;

        struct /*buffersManagementAgingConfig*/{

            GT_U32    bufferManagementAgingConfig;

        }buffersManagementAgingConfig;

        struct /*buffersManagementGlobalConfig*/{

            GT_U32    bufferManagementGlobalBuffersLimitsConfig;
            GT_U32    bufferManagementSharedBuffersConfig;

        }buffersManagementGlobalConfig;


    }BM /*- BM*/ ;
    struct /*statisticsPCSshared - statistics PCS (shared)  */{

        struct /*globalRegs*/{

            GT_U32    txPktsCntrLSB;
            GT_U32    txPktsCntrMSB;

            GT_U32    globalConfig0;
            GT_U32    globalConfig1;

            GT_U32    globalDeskewErrorCntr;

            GT_U32    metalFix;

            GT_U32    globalStatus;
            GT_U32    globalInterruptCause;
            GT_U32    globalInterruptMask;

        }globalRegs;

        LANE_REGS laneRegs[6];

    }statisticsPCSshared /*- statistics PCS (shared) */ ;

}SMEM_FX950_REGS_ADDR_STC;


typedef struct{
#ifdef DEBUG_REG_AND_TBL_NAME
    GT_CHAR     *tableName;
#endif /*DEBUG_REG_AND_TBL_NAME*/
    GT_U32      baseAddr;/*address of start memory (register) of table*/
    GT_U32      memType;/*0-internal , 1 - external*/
    GT_U32      entrySize;/* in bits -- number of bits actually used */
    GT_U32      lineAddrAlign;/* in words -- number of words between 2 entries*/
    GT_U32      numOfEntries;
    GT_U32      *memPtr;/* pointer to allocated memory
                        NOTE : we will add another entry to the table for the use of :
                        buffer for write actions -->
                        the write to entry not written to actual memory until writing
                        the last word in entry . (the write is into this 'buffer'
                        until download to memory) */
    GT_U32      lastAddr;/*address of last memory (register) of table that is in the table*/

    SMEM_ACTIVE_MEM_ENTRY_STC activeMemInfo;/*used for writing table entry into memory
                    only after writing to last word in entry */
}SMEM_FX950_TABLE_INFO_STC;

typedef struct{
    struct /*ingr - ingress */{

        SMEM_FX950_TABLE_INFO_STC    UC_Config_Table;
        SMEM_FX950_TABLE_INFO_STC    CPU_Config_Table;
        SMEM_FX950_TABLE_INFO_STC    UC_Link_0_packet_Memory_High;
        SMEM_FX950_TABLE_INFO_STC    UC_Link_0_packet_Memory_Low;
        SMEM_FX950_TABLE_INFO_STC    MC_Link_0_packet_Memory_High;
        SMEM_FX950_TABLE_INFO_STC    MCLink_0_packet_Memory_Low;
        SMEM_FX950_TABLE_INFO_STC    UC_Link_1_packet_Memory_High;
        SMEM_FX950_TABLE_INFO_STC    UC_Link_1_packet_Memory_Low;
        SMEM_FX950_TABLE_INFO_STC    MC_Link_1_packet_Memory_High;
        SMEM_FX950_TABLE_INFO_STC    MC_Link_1_packet_Memory_Low;
        SMEM_FX950_TABLE_INFO_STC    link_0_Desc_Memory_High;
        SMEM_FX950_TABLE_INFO_STC    link_0_Desc_Memory_Low;
        SMEM_FX950_TABLE_INFO_STC    link_1_Desc_Memory_High;
        SMEM_FX950_TABLE_INFO_STC    link_1_Desc_Memory_Low;
        SMEM_FX950_TABLE_INFO_STC    data_Delay_memory;
        SMEM_FX950_TABLE_INFO_STC    desc_Delay_memory;

    }ingr /*- ingress*/ ;
    struct /*egr - egress */{

        struct /*EDQUnit*/{

            SMEM_FX950_TABLE_INFO_STC    PD_mem;
            SMEM_FX950_TABLE_INFO_STC    LL0_mem;
            SMEM_FX950_TABLE_INFO_STC    LL1_mem;

        }EDQUnit;

        struct /*PDPUnit*/{

            SMEM_FX950_TABLE_INFO_STC    statistic_Report_Masking_table;
            SMEM_FX950_TABLE_INFO_STC    MCDP_table;
            SMEM_FX950_TABLE_INFO_STC    flowID_2_link_prio_table;
            SMEM_FX950_TABLE_INFO_STC    flowID_2_contextID_table;

        }PDPUnit;

        SMEM_FX950_TABLE_INFO_STC    pkts_memory_bank0;
        SMEM_FX950_TABLE_INFO_STC    pkts_memory_bank1;

    }egr /*- egress*/ ;
    struct /*statistics - statistics */{

        struct /*egrConfig*/{

            SMEM_FX950_TABLE_INFO_STC    egr_Msg_FIFO_High;
            SMEM_FX950_TABLE_INFO_STC    egr_Msg_FIFO_Low;

        }egrConfig;

        struct /*ingrConfig*/{

            SMEM_FX950_TABLE_INFO_STC    ingr_Msg_FIFO_High;
            SMEM_FX950_TABLE_INFO_STC    ingr_Msg_FIFO_Low;
            SMEM_FX950_TABLE_INFO_STC    ingr_Chunk_Cntrs;

        }ingrConfig;


    }statistics /*- statistics*/ ;
    struct /*misc - misc */{

        struct /*XSMI*/{

            SMEM_FX950_TABLE_INFO_STC    XSMI_Addr;

        }XSMI;

    }misc /*- misc*/ ;
    struct /*convertor0shared - convertor0 (shared)  */{

        SMEM_FX950_TABLE_INFO_STC    hyperG_Stack_Ports_MAC_MIB_Cntrs;

    }convertor0shared /*- convertor0 (shared) */ ;

    struct /*ftdll_Calibshared - ftdll_Calib (shared)  */{

        SMEM_FX950_TABLE_INFO_STC    FTDLL_UNIT;

    }ftdll_Calibshared /*- ftdll_Calib (shared) */ ;

    struct /*ftdll_Calibshared - ftdll_Calib (shared)  */{

        SMEM_FX950_TABLE_INFO_STC    FTDLL_UNIT;

    }ftdll_Calib_1shared /*- ftdll_Calib (shared) */ ;

    struct /*ftdll_Calibshared - ftdll_Calib (shared)  */{

        SMEM_FX950_TABLE_INFO_STC    FTDLL_UNIT;

    }ftdll_Calib_2shared /*- ftdll_Calib (shared) */ ;

}SMEM_FX950_TBLS_ADDR_STC;

/*******************************************************************************
*  SMEM_FX950_SPEC_MEMORY_FIND_FUN
*
* DESCRIPTION:
*      Definition of memory address type specific search function of SKERNEL. - in FX950
* INPUTS:
*       deviceObjPtr - device object PTR.
*       address     - Address for ASIC memory.
*       memSize     - Size of ASIC memory to read or write.
*       param       - Function specific parameter.*
*
* OUTPUTS:
*       specificTblInfoPtrPtr - (pointer to) pointer to table info --> in case of access to
*                           table memory that uses active memory
*                           when (*specificTblInfoPtrPtr)== NULL after return from function
*                           means that there is no table accessing
* RETURNS:
*                pointer to the memory location in the database
*       NULL - if address not exist, or memSize > than existed size
*
* COMMENTS:
*
*******************************************************************************/
typedef GT_U32 * (* SMEM_FX950_SPEC_MEMORY_FIND_FUN )
(
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32 address,
    IN         GT_U32 memSize,
    IN         GT_UINTPTR param,
    OUT        SMEM_FX950_TABLE_INFO_STC **specificTblInfoPtrPtr
);

/*
 * Typedef: struct SMEM_FX950_SPEC_FIND_FUN_ENTRY_STC
 *
 * Description:
 *      Describe the entry in the address type specific R/W functions table.
 *
 * Fields:
 *      specFun         : Entry point of the function.
 *      specParam       : Additional address type specific parameter specFun .
 * Comments:
 */

typedef struct  {
    SMEM_FX950_SPEC_MEMORY_FIND_FUN  specFun;
    GT_U32                   specParam;
} SMEM_FX950_SPEC_FIND_FUN_ENTRY_STC;


typedef struct {
    /*register addresses */
    SMEM_FX950_REGS_ADDR_STC        regsAddr;
    /*register content */
    SMEM_FX950_REGS_ADDR_STC        regsValues;

    SMEM_FX950_TBLS_ADDR_STC        tblsAddr;
    SMEM_FX950_SPEC_FIND_FUN_ENTRY_STC    specFunTbl[256];

}SMEM_FX950_DEV_MEM_INFO;

/*#define MEM_PTR_ALLOC(name)   \
    devMemInfoPtr->tblsAddr.name.memPtr =          \
    calloc(         \
        devMemInfoPtr->tblsAddr.name.numOfEntries + 1, \
        devMemInfoPtr->tblsAddr.name.entrySize);    \
    if (devMemInfoPtr->tblsAddr.name.memPtr == 0)  \
        skernelFatalError("smemFX950Init: allocation error - name\n"); \
    devMemInfoPtr->tblsAddr.name.lastAddr = \
        (devMemInfoPtr->tblsAddr.name.baseAddr + \
         (devMemInfoPtr->tblsAddr.name.numOfEntries * \
         CONVERT_BITS_TO_WORDS(devMemInfoPtr->tblsAddr.name.entrySize)) - 4); \
    activeMemInfo->tblsAddr.name.activeMemInfo.readFun = NULL; \
    activeMemInfo->tblsAddr.name.activeMemInfo.writeFun = smemFx950ActiveWriteTable; \
    activeMemInfo->tblsAddr.name.activeMemInfo.writeFunParam = (GT_U32)(void*)(&devMemInfoPtr->tblsAddr.name)
*/

/*******************************************************************************
* smemOcelotInit
*
* DESCRIPTION:
*       Init memory module for a Ocelot device.
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
void smemOcelotInit
(
    IN SKERNEL_DEVICE_OBJECT * deviceObj
);


/*******************************************************************************
* smemFX950Init
*
* DESCRIPTION:
*       Init memory module for a FX950 device.
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
void smemFX950Init
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
);





#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __smemOceloth */


