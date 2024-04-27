/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetLion2Oam.c
*
* DESCRIPTION:
*       Lion2 OAM Engine processing for frame -- simulation
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 48 $
*
*******************************************************************************/

#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SKernel/smem/smemCheetah.h>
#include <asicSimulation/SKernel/suserframes/snet.h>
#include <asicSimulation/SKernel/cheetahCommon/sregCheetah.h>
#include <asicSimulation/SKernel/suserframes/snetLion2Oam.h>
#include <asicSimulation/SKernel/suserframes/snetXCatPolicer.h>
#include <asicSimulation/SKernel/suserframes/snetCheetah3TTermination.h>
#include <asicSimulation/SKernel/suserframes/snetXCatPcl.h>
#include <asicSimulation/SKernel/suserframes/snetCheetahEgress.h>
#include <asicSimulation/SKernel/suserframes/snetLion.h>
#include <common/Utils/Math/sMath.h>
#include <asicSimulation/SLog/simLog.h>
#include <asicSimulation/SLog/simLogInfoTypePacket.h>

/* Delay between OAM aging request bitmap sampling  */
#define OAM_DAEMON_DELAY_MILISECONDS_E              1000

/* OAM message has two parts:
   - pointer to the function GENERIC_MSG_FUNC
   - GT_U32 value of period index  */
#define OAM_KEEPALIVE_AGING_MESSAGE_GENERIC_MSG_FUNC_SIZE (sizeof(GENERIC_MSG_FUNC))
#define OAM_KEEPALIVE_AGING_MESSAGE_PERIOD_INDEX_SIZE (sizeof(GT_U32))

/* OAM keepalive aging message size in bytes */
#define OAM_KEEPALIVE_AGING_MESSAGE_SIZE                    \
    (OAM_KEEPALIVE_AGING_MESSAGE_GENERIC_MSG_FUNC_SIZE +    \
     OAM_KEEPALIVE_AGING_MESSAGE_PERIOD_INDEX_SIZE)

#define SNET_OAM_STAGE_2_DIRECTION_MAC(stage) \
    (((stage) == SNET_LION2_OAM_STAGE_INGRESS_E) ? \
        SMAIN_DIRECTION_INGRESS_E : SMAIN_DIRECTION_EGRESS_E)

#define SNET_OAM_PROCESS_ENABLE_SET(descr, stage, enable) \
{ \
        if ((stage) == SNET_LION2_OAM_STAGE_INGRESS_E) \
            (descr)->oamInfo.oamProcessEnable = enable; \
        else \
            (descr)->oamInfo.oamEgressProcessEnable = enable; \
}

#define SNET_OAM_PROCESS_ENABLE_GET(descr, stage) \
    (((stage) == SNET_LION2_OAM_STAGE_INGRESS_E) ? \
        (descr)->oamInfo.oamProcessEnable : (descr)->oamInfo.oamEgressProcessEnable)

/*******************************************************************************
*   snetLion2OamOpCodeConfigGet
*
* DESCRIPTION:
*       Find opcode in OAM configuration registers
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - pointer to the frame's descriptor.
*       stage       - ingress or egress OAM stage.
*       opCodeType  - opcode type
* OUTPUT:
*       opCodePtr   - opcode value if found and zero if not
*       opCodeIndexPtr - opcode index if opCode found
*
*
*******************************************************************************/
static GT_VOID snetLion2OamOpCodeConfigGet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_LION2_OAM_STAGE_TYPE_ENT stage,
    IN SNET_LION2_OAM_OPCODE_TYPE_ENT opCodeType,
    OUT GT_U32 * opCodePtr,
    OUT GT_U32 * opCodeIndexPtr
)
{
    GT_U32 regAddr;                 /* Register address (OAM Opcode) */
    GT_U32 opCodeIndex;             /* Opcode index */
    GT_U32 opCodeTotal;             /* Total opcodes */
    GT_U32 * regPtr;                /* Register pointer */

    DECLARE_FUNC_NAME(snetLion2OamOpCodeConfigGet);

    switch(opCodeType)
    {
        case SNET_LION2_OAM_OPCODE_TYPE_LM_COUNTED_E:
            regAddr =
                SMEM_LION2_OAM_LOSS_MEASURMENT_COUNTED_OPCODE_REG(devObjPtr, stage);
            __LOG(("Iterate over OAM LM Counted Opcode"));
            opCodeTotal = 8;
            break;
        case SNET_LION2_OAM_OPCODE_TYPE_LM_SINGLE_ENDED_E:
            regAddr =
                SMEM_LION2_OAM_LOSS_MEASURMENT_OPCODE_REG(devObjPtr, stage);
            __LOG(("Iterate over Single-Ended OAM LM Opcode"));
            opCodeTotal = 16;
            break;
        case SNET_LION2_OAM_OPCODE_TYPE_LM_DUAL_ENDED_E:
            __LOG(("Find Dual-Ended OAM LM Opcode"));
            regAddr =
                SMEM_LION2_OAM_DUAL_ENDED_LM_REG(devObjPtr, stage);
            opCodeTotal = 1;
            break;
        case SNET_LION2_OAM_OPCODE_TYPE_DM_E:
            regAddr =
                SMEM_LION2_OAM_DELAY_MEASURMENT_OPCODE_REG(devObjPtr, stage);
            __LOG(("Iterate over Delay Measurement OAM LM Opcode"));
            opCodeTotal = 16;
            break;
        case SNET_LION2_OAM_OPCODE_TYPE_KEEPALIVE_E:
            regAddr =
                SMEM_LION2_OAM_KEEP_ALIVE_OPCODE_REG(devObjPtr, stage);
            __LOG(("Iterate over Keepalive OAM LM Opcode"));
            opCodeTotal = 16;
            break;
        default:
            skernelFatalError("snetLion2OamOpCodeConfigGet : wrong OAM opcode type\n");
            return;
    }

    regPtr = smemMemGet(devObjPtr, regAddr);
    for(opCodeIndex = 0; opCodeIndex < opCodeTotal; opCodeIndex++)
    {
        if(descrPtr->oamInfo.opCode ==
           SMEM_U32_GET_FIELD(regPtr[opCodeIndex/4], (opCodeIndex%4) * 8, 8))
        {
            __LOG(("OAM LM Opcode found"));
            *opCodePtr = descrPtr->oamInfo.opCode;
            *opCodeIndexPtr = opCodeIndex;
            return;
        }
    }

    __LOG(("OAM LM Opcode not found"));
    *opCodePtr = 0;
}

/*******************************************************************************
*   snetLion2OamInfoGet
*
* DESCRIPTION:
*       Get OAM info
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       stage       - ingress or egress OAM stage.
*       oamInfoPtr  - pointer to OAM info structure
*
* OUTPUT:
*       oamInfoPtr  - pointer to OAM info structure
*
*
*******************************************************************************/
static GT_VOID snetLion2OamInfoGet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SNET_LION2_OAM_STAGE_TYPE_ENT stage,
    INOUT SNET_LION2_OAM_INFO_STC * oamInfoPtr
)
{
    GT_U32 regAddr;                             /* Register address */
    GT_U32 * regPtr;                            /* Register pointer */
    GT_U32 regValue;                            /* Register value */

    DECLARE_FUNC_NAME(snetLion2OamInfoGet);

    /* Get OAM entry by OAM stage and flow ID */
    __LOG(("Get OAM entry by OAM index [%d], stage [%d] \n", oamInfoPtr->entryIndex, stage));

    regAddr = SMEM_LION2_OAM_BASE_TBL_MEM(devObjPtr, stage, oamInfoPtr->entryIndex);
    regPtr = smemMemGet(devObjPtr, regAddr);

    oamInfoPtr->opCodeParsingEnable =
        snetFieldValueGet(regPtr, 0, 1);
    oamInfoPtr->lmCounterCaptureEnable =
        snetFieldValueGet(regPtr, 1, 1);
    oamInfoPtr->dualEndedLmEnable =
        snetFieldValueGet(regPtr, 2, 1);
    regValue =
        snetFieldValueGet(regPtr, 3, 2);
    switch (regValue)
    {
        case 0:
            oamInfoPtr->lmCountingMode = SNET_LION2_OAM_LM_COUNTING_MODE_RETAIN_E;
            break;
        case 1:
            oamInfoPtr->lmCountingMode = SNET_LION2_OAM_LM_COUNTING_MODE_DISABLE_E;
            break;
        case 2:
            oamInfoPtr->lmCountingMode = SNET_LION2_OAM_LM_COUNTING_MODE_ENABLE_E;
            break;
    default:
            __LOG(("WARNING: value [%d] is not applicable for bad lmCountingMode \n", regValue));
            oamInfoPtr->lmCountingMode = SNET_LION2_OAM_LM_COUNTING_MODE_RETAIN_E;
            break;
    }

    oamInfoPtr->megLevelCheckEnable =
        snetFieldValueGet(regPtr, 5, 1);
    oamInfoPtr->megLevel =
        snetFieldValueGet(regPtr, 6, 3);
    oamInfoPtr->keepaliveAgingEnable =
        snetFieldValueGet(regPtr, 9, 1);
    oamInfoPtr->ageState =
        snetFieldValueGet(regPtr, 10, 4);
    oamInfoPtr->agingPeriodIndex =
        snetFieldValueGet(regPtr, 14, 3);
    oamInfoPtr->agingThreshold =
        snetFieldValueGet(regPtr, 17, 4);
    oamInfoPtr->hashVerifyEnable =
        snetFieldValueGet(regPtr, 21, 1);
    oamInfoPtr->lockHashValueEnable =
        snetFieldValueGet(regPtr, 22, 1);
    oamInfoPtr->flowHash =
        snetFieldValueGet(regPtr, 23, 12);
    oamInfoPtr->offsetIndex =
        snetFieldValueGet(regPtr, 35, 7);
    oamInfoPtr->timestampEnable =
        snetFieldValueGet(regPtr, 42, 1);
    oamInfoPtr->packetCommandProfile =
        snetFieldValueGet(regPtr, 43, 3);
    oamInfoPtr->cpuCodeOffset =
        snetFieldValueGet(regPtr, 46, 2);
    oamInfoPtr->sourceInterfaceCheckEnable =
        snetFieldValueGet(regPtr, 48, 1);
    oamInfoPtr->sourceIsTrunk =
        snetFieldValueGet(regPtr, 49, 1);
    oamInfoPtr->sourceInterfaceCheckMode =
        snetFieldValueGet(regPtr, 50, 1);
    oamInfoPtr->localDevSrcEPortTrunk =
        snetFieldValueGet(regPtr, 51, 17);
    oamInfoPtr->protectionLocUpdateEnable =
        snetFieldValueGet(regPtr, 68, 1);
    oamInfoPtr->excessKeepaliveDetectionEnable =
        snetFieldValueGet(regPtr, 69, 1);
    oamInfoPtr->excessKeepalivePeriodCounter =
        snetFieldValueGet(regPtr, 70, 4);
    oamInfoPtr->excessKeepalivePeriodThreshold =
        snetFieldValueGet(regPtr, 74, 4);
    oamInfoPtr->excessKeepaliveMessageCounter =
        snetFieldValueGet(regPtr, 78, 3);
    oamInfoPtr->excessKeepaliveMessageThreshold =
        snetFieldValueGet(regPtr, 81, 3);
    oamInfoPtr->rdiCheckEnable =
        snetFieldValueGet(regPtr, 84, 1);
    oamInfoPtr->rdiStatus =
        snetFieldValueGet(regPtr, 85, 1);
    oamInfoPtr->periodCheckEnable =
        snetFieldValueGet(regPtr, 86, 1);
    oamInfoPtr->keepaliveTxPeriod =
        snetFieldValueGet(regPtr, 87, 3);

    if(simLogIsOpen())
    {
        __LOG(("Parse the OAM entry OAM index [%d], stage [%d] \n", oamInfoPtr->entryIndex, stage));
        __LOG_PARAM(oamInfoPtr->opCodeParsingEnable);
        __LOG_PARAM(oamInfoPtr->lmCounterCaptureEnable);
        __LOG_PARAM(oamInfoPtr->dualEndedLmEnable);
        __LOG_PARAM(oamInfoPtr->lmCountingMode);
        __LOG_PARAM(oamInfoPtr->megLevelCheckEnable);
        __LOG_PARAM(oamInfoPtr->megLevel);
        __LOG_PARAM(oamInfoPtr->keepaliveAgingEnable);
        __LOG_PARAM(oamInfoPtr->ageState);
        __LOG_PARAM(oamInfoPtr->agingPeriodIndex);
        __LOG_PARAM(oamInfoPtr->agingThreshold);
        __LOG_PARAM(oamInfoPtr->hashVerifyEnable);
        __LOG_PARAM(oamInfoPtr->lockHashValueEnable);
        __LOG_PARAM(oamInfoPtr->flowHash);
        __LOG_PARAM(oamInfoPtr->offsetIndex);
        __LOG_PARAM(oamInfoPtr->timestampEnable);
        __LOG_PARAM(oamInfoPtr->packetCommandProfile);
        __LOG_PARAM(oamInfoPtr->cpuCodeOffset);
        __LOG_PARAM(oamInfoPtr->sourceInterfaceCheckEnable);
        __LOG_PARAM(oamInfoPtr->sourceIsTrunk);
        __LOG_PARAM(oamInfoPtr->sourceInterfaceCheckMode);
        __LOG_PARAM(oamInfoPtr->localDevSrcEPortTrunk);
        __LOG_PARAM(oamInfoPtr->protectionLocUpdateEnable);
        __LOG_PARAM(oamInfoPtr->excessKeepaliveDetectionEnable);
        __LOG_PARAM(oamInfoPtr->excessKeepalivePeriodCounter);
        __LOG_PARAM(oamInfoPtr->excessKeepalivePeriodThreshold);
        __LOG_PARAM(oamInfoPtr->excessKeepaliveMessageCounter);
        __LOG_PARAM(oamInfoPtr->excessKeepaliveMessageThreshold);
        __LOG_PARAM(oamInfoPtr->rdiCheckEnable);
        __LOG_PARAM(oamInfoPtr->rdiStatus);
        __LOG_PARAM(oamInfoPtr->periodCheckEnable);
        __LOG_PARAM(oamInfoPtr->keepaliveTxPeriod);
    }

    if(0 == SMEM_CHT_IS_SIP5_15_GET(devObjPtr))
    {
        /* no more fields */
        return ;
    }

    /* new fields in sip5_15*/
    oamInfoPtr->locDetectionEnable =
        snetFieldValueGet(regPtr, 93, 1);

    if(simLogIsOpen())
    {
        __LOG_PARAM(oamInfoPtr->locDetectionEnable);
    }

}

/*******************************************************************************
*   snetLion2OamInfoSet
*
* DESCRIPTION:
*       Set OAM table entry
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       stage       - ingress or egress OAM stage.
*       oamInfoPtr  - pointer to OAM info structure.
*
*******************************************************************************/
static GT_VOID snetLion2OamInfoSet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SNET_LION2_OAM_STAGE_TYPE_ENT stage,
    IN SNET_LION2_OAM_INFO_STC * oamInfoPtr
)
{
    GT_U32 hwData[SMEM_LION2_OAM_TABLE_WORDS];  /* 4 words of OAM HW entry  */
    GT_U32 regAddr;                             /* Register address */
    GT_U32 lmCountingMode = 0;                  /* Determines which packets are counted by the LM counters */



    memset(hwData, 0, SMEM_LION2_OAM_ENTRY_BYTES);

    snetFieldValueSet(hwData, 0,  1, oamInfoPtr->opCodeParsingEnable);
    snetFieldValueSet(hwData, 1,  1, oamInfoPtr->lmCounterCaptureEnable);
    snetFieldValueSet(hwData, 2,  1, oamInfoPtr->dualEndedLmEnable);
    switch(oamInfoPtr->lmCountingMode)
    {
        case SNET_LION2_OAM_LM_COUNTING_MODE_RETAIN_E:
            lmCountingMode = 0;
            break;
        case SNET_LION2_OAM_LM_COUNTING_MODE_DISABLE_E:
            lmCountingMode = 1;
            break;
        case SNET_LION2_OAM_LM_COUNTING_MODE_ENABLE_E:
            lmCountingMode = 2;
            break;
        default:
            skernelFatalError("snetLion2OamInfoSet: bad lmCountingMode[%d]\n", oamInfoPtr->lmCountingMode);
    }
    snetFieldValueSet(hwData, 3,  2, lmCountingMode);
    snetFieldValueSet(hwData, 5,  1, oamInfoPtr->megLevelCheckEnable);
    snetFieldValueSet(hwData, 6,  3, oamInfoPtr->megLevel);
    snetFieldValueSet(hwData, 9,  1, oamInfoPtr->keepaliveAgingEnable);
    snetFieldValueSet(hwData, 10, 4, oamInfoPtr->ageState);
    snetFieldValueSet(hwData, 14, 3, oamInfoPtr->agingPeriodIndex);
    snetFieldValueSet(hwData, 17, 4, oamInfoPtr->agingThreshold);
    snetFieldValueSet(hwData, 21, 1, oamInfoPtr->hashVerifyEnable);
    snetFieldValueSet(hwData, 22, 1, oamInfoPtr->lockHashValueEnable);
    snetFieldValueSet(hwData, 23, 12, oamInfoPtr->flowHash);
    snetFieldValueSet(hwData, 35, 7, oamInfoPtr->offsetIndex);
    snetFieldValueSet(hwData, 42, 1, oamInfoPtr->timestampEnable);
    snetFieldValueSet(hwData, 43, 3, oamInfoPtr->packetCommandProfile);
    snetFieldValueSet(hwData, 46, 2, oamInfoPtr->cpuCodeOffset);
    snetFieldValueSet(hwData, 48, 1, oamInfoPtr->sourceInterfaceCheckEnable);
    snetFieldValueSet(hwData, 49, 1, oamInfoPtr->sourceIsTrunk);
    snetFieldValueSet(hwData, 50, 1, oamInfoPtr->sourceInterfaceCheckMode);
    snetFieldValueSet(hwData, 51, 17, oamInfoPtr->localDevSrcEPortTrunk);
    snetFieldValueSet(hwData, 68, 1, oamInfoPtr->protectionLocUpdateEnable);
    snetFieldValueSet(hwData, 69, 1, oamInfoPtr->excessKeepaliveDetectionEnable);
    snetFieldValueSet(hwData, 70, 4, oamInfoPtr->excessKeepalivePeriodCounter);
    snetFieldValueSet(hwData, 74, 4, oamInfoPtr->excessKeepalivePeriodThreshold);
    snetFieldValueSet(hwData, 78, 3, oamInfoPtr->excessKeepaliveMessageCounter);
    snetFieldValueSet(hwData, 81, 3, oamInfoPtr->excessKeepaliveMessageThreshold);
    snetFieldValueSet(hwData, 84, 1, oamInfoPtr->rdiCheckEnable);
    snetFieldValueSet(hwData, 85, 1, oamInfoPtr->rdiStatus);
    snetFieldValueSet(hwData, 86, 1, oamInfoPtr->periodCheckEnable);
    snetFieldValueSet(hwData, 87, 3, oamInfoPtr->keepaliveTxPeriod);

    if(SMEM_CHT_IS_SIP5_15_GET(devObjPtr))
    {
        /* new fields in sip5_15*/
        snetFieldValueSet(hwData, 93, 1, oamInfoPtr->locDetectionEnable);
    }

    /* Set OAM entry address by OAM stage and flow ID */
    regAddr = SMEM_LION2_OAM_BASE_TBL_MEM(devObjPtr, stage, oamInfoPtr->entryIndex);
    smemMemSet(devObjPtr, regAddr, hwData, SMEM_LION2_OAM_TABLE_WORDS);
}

/*******************************************************************************
*   snetLion2OamMegLevelCheck
*
* DESCRIPTION:
*       MEG Level (MEL) Check
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - pointer to the frame's descriptor.
*       stage       - ingress or egress OAM stage.
*       oamInfoPtr  - pointer to OAM info structure.
* OUTPUT:
*       descrPtr    - pointer to the frame's descriptor.
*
*
*******************************************************************************/
static GT_VOID snetLion2OamMegLevelCheck
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_LION2_OAM_STAGE_TYPE_ENT stage,
    IN SNET_LION2_OAM_INFO_STC * oamInfoPtr
)
{
    DECLARE_FUNC_NAME(snetLion2OamMegLevelCheck);

    SMAIN_DIRECTION_ENT direction;              /* Ingress/Egress pipe */
    GT_U32 regAddr;                             /* Register address */
    GT_U32 fldValue;                            /* Register field */
    GT_U32 * regPtr;                            /* Register pointer */
    SKERNEL_EXT_PACKET_CMD_ENT packetCmd;       /* Current packet command */
    GT_U32 cpuCode;                             /* Current packet CPU code */
    GT_U32 megLevelSummaryEn;                   /* MEG level Summary Bit */

    if (SNET_OAM_PROCESS_ENABLE_GET(descrPtr, stage) == 0)
    {
        /* OAM table logic disabled */
        __LOG(("OAM table logic disabled"));
        return;
    }

    if(oamInfoPtr->megLevelCheckEnable == 0)
    {
        /* The packets MEG level is not checked */
        __LOG(("The packets MEG level is not checked"));
        return;
    }

    direction = SNET_OAM_STAGE_2_DIRECTION_MAC(stage);

    if (descrPtr->oamInfo.megLevel > oamInfoPtr->megLevel)
    {
        /* Disable policer billing counters */
        __LOG(("Disable policer billing counters"));
        SNET_XCAT_POLICER_POLICY_COUNT_ENABLE_SET_MAC(devObjPtr, descrPtr,
                                                      direction, 0);

        /* Stop other OAM logic and forward packet for further processing  */
        SNET_OAM_PROCESS_ENABLE_SET(descrPtr, stage, 0);
    }
    else if (descrPtr->oamInfo.megLevel < oamInfoPtr->megLevel)
    {
        /* Disable policer billing counters */
        __LOG(("Disable policer billing counters"));
        SNET_XCAT_POLICER_POLICY_COUNT_ENABLE_SET_MAC(devObjPtr, descrPtr,
                                                      direction, 0);

        regAddr = SMEM_LION2_OAM_EXCEPTION_CONF_REG(devObjPtr, stage);
        regPtr = smemMemGet(devObjPtr, regAddr);

        packetCmd = SMEM_U32_GET_FIELD(regPtr[0], 0, 3);
        cpuCode = SMEM_U32_GET_FIELD(regPtr[0], 4, 8);
        megLevelSummaryEn = SMEM_U32_GET_FIELD(regPtr[0], 29, 1);
        /* Assign new packet command */
        __LOG(("Assign new packet command"));
        snetChtIngressCommandAndCpuCodeResolution(devObjPtr, descrPtr,
                                                  descrPtr->packetCmd, packetCmd,
                                                  descrPtr->cpuCode, cpuCode,
                                                  SNET_CHEETAH_ENGINE_UNIT_OAM_E,
                                                  GT_TRUE);

        /* Generate interrupt  "Bridge Interrupt Cause Register" bit 28 */
        snetChetahDoInterrupt(devObjPtr,
                              SMEM_LION2_OAM_INTR_CAUSE_REG(devObjPtr, stage),
                              SMEM_LION2_OAM_INTR_MASK_REG(devObjPtr, stage),
                              SNET_LION2_OAM_INTR_MEG_LEVEL_EXCEPTION_E,
                              SMEM_CHT_L2I_SUM_INT(devObjPtr));

        regAddr = SMEM_LION2_MEG_EXCEPTION_TBL_MEM(devObjPtr, stage,
                                                   oamInfoPtr->entryIndex / 32);
        regPtr = smemMemGet(devObjPtr, regAddr);
        /* Set bit in MEG level exception entry that has had a MEG Level exception */
        __LOG(("Set bit in MEG level exception entry that has had a MEG Level exception"));
        SMEM_U32_SET_FIELD(regPtr[0], oamInfoPtr->entryIndex % 32, 1, 1);

        if (megLevelSummaryEn)
        {
            regAddr = SMEM_LION2_OAM_EXCEPTION_SUMMARY_TBL_MEM(devObjPtr,stage,
                                                               oamInfoPtr->entryIndex / 32);
            regPtr = smemMemGet(devObjPtr, regAddr);
            /* Set for every entry in the OAM table that has triggered an exception */
            __LOG(("Set for every entry in the OAM table that has triggered an exception"));
            SMEM_U32_SET_FIELD(regPtr[0], oamInfoPtr->entryIndex % 32, 1, 1);
        }

        regAddr = SMEM_LION2_OAM_MEG_LEVEL_EXCEPTION_COUNTER_REG(devObjPtr, stage);
        smemRegFldGet(devObjPtr, regAddr, 0, 32, &fldValue);
        smemRegFldSet(devObjPtr, regAddr, 0, 32, ++fldValue);
        __LOG(("stage[%d] : counter [%s] changed from [%d] to [%d] \n" ,
            stage ,
            "OAM_MEG_LEVEL_EXCEPTION_COUNTER" ,
            fldValue - 1 ,
            fldValue));

        /* Stop other OAM logic and forward packet for further processing  */
        __LOG(("Stop other OAM logic and forward packet for further processing"));
        SNET_OAM_PROCESS_ENABLE_SET(descrPtr, stage, 0);
    }
}

/*******************************************************************************
*   snetLion2OamSourceInterfaceVerification
*
* DESCRIPTION:
*       Source Interface Verification
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - pointer to the frame's descriptor.
*       stage       - ingress or egress OAM stage.
*       oamInfoPtr  - pointer to OAM info structure.
* OUTPUT:
*       descrPtr     - pointer to the frame's descriptor.
*
*
*******************************************************************************/
static GT_VOID snetLion2OamSourceInterfaceVerification
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_LION2_OAM_STAGE_TYPE_ENT stage,
    IN SNET_LION2_OAM_INFO_STC * oamInfoPtr
)
{
    DECLARE_FUNC_NAME(snetLion2OamSourceInterfaceVerification);

    SMAIN_DIRECTION_ENT direction;              /* Ingress/Egress pipe */
    GT_U32 regAddr;                             /* Register address */
    GT_U32 * regPtr;                            /* Register pointer */
    GT_U32 fldValue;                            /* Register field */
    SKERNEL_EXT_PACKET_CMD_ENT packetCmd;       /* Current packet command */
    GT_U32 cpuCode;                             /* Current packet CPU code */
    GT_U32 srcInterfaceSummaryEn;               /* SRC interface exception summary bit */
    GT_BOOL srcMatch;                           /* Indicate SRC interface matching */
    GT_U32 localSrcPortTrunkToMatch;            /* Local SRC Port/Trunk */

    if (SNET_OAM_PROCESS_ENABLE_GET(descrPtr, stage) == 0)
    {
        /* OAM table logic disabled */
        __LOG(("OAM table logic disabled"));
        return;
    }

    if(oamInfoPtr->sourceInterfaceCheckEnable == 0)
    {
        /* The packets source interface is not checked */
        __LOG(("The packets source interface is not checked"));
        return;
    }

    direction = SNET_OAM_STAGE_2_DIRECTION_MAC(stage);

    if (oamInfoPtr->sourceIsTrunk)
    {
        localSrcPortTrunkToMatch = descrPtr->localDevSrcTrunkId;
    }
    else
    {
        localSrcPortTrunkToMatch =
            (devObjPtr->supportEArch && devObjPtr->unitEArchEnable.pcl[direction]) ?
                descrPtr->eArchExtInfo.localDevSrcEPort :
                descrPtr->localDevSrcPort;
    }

    srcMatch = (localSrcPortTrunkToMatch == oamInfoPtr->localDevSrcEPortTrunk) ?
        GT_TRUE : GT_FALSE;

    /* Source interface is OK */
    if((srcMatch == GT_TRUE && oamInfoPtr->sourceInterfaceCheckMode == 1) ||
       (srcMatch == GT_FALSE && oamInfoPtr->sourceInterfaceCheckMode == 0))
    {
        /* Continue to Opcode Packet Command table */
        __LOG(("Continue to Opcode Packet Command table"));
        return;
    }

    /* Source interface failed */

    /* Disable policer billing counters */
    __LOG(("Disable policer billing counters"));
    SNET_XCAT_POLICER_POLICY_COUNT_ENABLE_SET_MAC(devObjPtr, descrPtr,
                                                  direction, 0);

    regAddr = SMEM_LION2_OAM_EXCEPTION_CONF1_REG(devObjPtr, stage);
    regPtr = smemMemGet(devObjPtr, regAddr);

    packetCmd = SMEM_U32_GET_FIELD(regPtr[0], 0, 3);
    cpuCode = SMEM_U32_GET_FIELD(regPtr[0], 3, 8);

    regAddr = SMEM_LION2_OAM_EXCEPTION_CONF_REG(devObjPtr, stage);
    regPtr = smemMemGet(devObjPtr, regAddr);
    srcInterfaceSummaryEn = SMEM_U32_GET_FIELD(regPtr[0], 30, 1);
    /* Assign new packet command */
    __LOG(("Assign new packet command"));
    snetChtIngressCommandAndCpuCodeResolution(devObjPtr, descrPtr,
                                              descrPtr->packetCmd, packetCmd,
                                              descrPtr->cpuCode, cpuCode,
                                              SNET_CHEETAH_ENGINE_UNIT_OAM_E,
                                              GT_TRUE);

    /* Generate interrupt  "Bridge Interrupt Cause Register" bit 28 */
    snetChetahDoInterrupt(devObjPtr,
                          SMEM_LION2_OAM_INTR_CAUSE_REG(devObjPtr, stage),
                          SMEM_LION2_OAM_INTR_MASK_REG(devObjPtr, stage),
                          SNET_LION2_OAM_INTR_SRC_INTERFACE_EXCEPTION_E,
                          SMEM_CHT_L2I_SUM_INT(devObjPtr));

    regAddr = SMEM_LION2_SRC_INTERFACE_EXCEPTION_TBL_MEM(devObjPtr, stage,
                                                         oamInfoPtr->entryIndex / 32);
    regPtr = smemMemGet(devObjPtr, regAddr);
    /* Set bit in Source Interface exception entry that has had a source interface mismatch */
    SMEM_U32_SET_FIELD(regPtr[0], oamInfoPtr->entryIndex % 32, 1, 1);

    if (srcInterfaceSummaryEn)
    {
        regAddr = SMEM_LION2_OAM_EXCEPTION_SUMMARY_TBL_MEM(devObjPtr,stage,
                                                           oamInfoPtr->entryIndex / 32);
        regPtr = smemMemGet(devObjPtr, regAddr);
        /* Set for every entry in the OAM table that has triggered an exception */
        __LOG(("Set for every entry in the OAM table that has triggered an exception"));
        SMEM_U32_SET_FIELD(regPtr[0], oamInfoPtr->entryIndex % 32, 1, 1);
    }

    regAddr = SMEM_LION2_OAM_SOURCE_INTERFACE_MISMATCH_COUNTER_REG(devObjPtr, stage);
    smemRegFldGet(devObjPtr, regAddr, 0, 32, &fldValue);
    smemRegFldSet(devObjPtr, regAddr, 0, 32, ++fldValue);
    __LOG(("stage[%d] : counter [%s] changed from [%d] to [%d] \n" ,
        stage ,
        "OAM_SOURCE_INTERFACE_MISMATCH_COUNTER" ,
        fldValue - 1 ,
        fldValue));

    /* Stop other OAM logic and forward packet for further processing  */
    SNET_OAM_PROCESS_ENABLE_SET(descrPtr, stage, 0);
}

/*******************************************************************************
*   snetLion2OamOpCodeParsing
*
* DESCRIPTION:
*       OpCode-to-Packet Command mapping
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - pointer to the frame's descriptor.
*       stage       - ingress or egress OAM stage.
*       oamInfoPtr  - pointer to OAM info structure.
* OUTPUT:
*       descrPtr     - pointer to the frame's descriptor.
*       origDescrPtr - pointer to the frame's original descriptor.
*
*
*******************************************************************************/
static GT_VOID snetLion2OamOpCodeParsing
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_LION2_OAM_INFO_STC * oamInfoPtr,
    IN SNET_LION2_OAM_STAGE_TYPE_ENT stage,
    OUT SKERNEL_FRAME_CHEETAH_DESCR_STC * origDescrPtr
)
{
    DECLARE_FUNC_NAME(snetLion2OamOpCodeParsing);

    GT_U32 regAddr;                             /* Register address */
    GT_U32 * regPtr;                            /* Register pointer */
    SKERNEL_EXT_PACKET_CMD_ENT packetCmd;       /* Current packet command */
    GT_U32 cpuCode;                             /* Current packet CPU code */
    GT_U32 cpuCodeBase;                         /* OAM CPU Code Base */
    GT_U32 profile;                             /* Profile index */
    GT_U32 mcProfileEnable;                     /* MC profile enable */
    SMAIN_DIRECTION_ENT direction;              /* Ingress/Egress pipe */

    if (SNET_OAM_PROCESS_ENABLE_GET(descrPtr, stage) == 0)
    {
        /* OAM table logic disabled */
        __LOG(("OAM table logic disabled"));
        return;
    }

    if(oamInfoPtr->opCodeParsingEnable == 0)
    {
        /* OAM Opcode Parsing Disable */
        __LOG(("OAM Opcode Parsing Disable"));
        return;
    }

    direction = SNET_OAM_STAGE_2_DIRECTION_MAC(stage);

    /* Save original descriptor.
    It could be restored in Dual-End LM packet command assignment */
    __LOG(("Save original descriptor"));
    *origDescrPtr = *descrPtr;

    regAddr = SMEM_LION2_OAM_GLOBAL_CONTROL1_REG(devObjPtr, stage);
    regPtr = smemMemGet(devObjPtr, regAddr);
    /* MC profile enable */
    mcProfileEnable = SMEM_U32_GET_FIELD(regPtr[0], 0, 1);

    profile = (mcProfileEnable && descrPtr->macDaType == SKERNEL_MULTICAST_MAC_E) ?
        oamInfoPtr->packetCommandProfile + 1 :
        oamInfoPtr->packetCommandProfile;

    regAddr = SMEM_LION2_OAM_OPCODE_PACKET_COMMAND_TBL_MEM(devObjPtr,
                                                           stage,
                                                           descrPtr->oamInfo.opCode);
    regPtr = smemMemGet(devObjPtr, regAddr);
    /* Profile <P> Opcode <n> Packet Command */
    packetCmd = snetFieldValueGet(regPtr, profile * 5, 3);
    /* Two least significant bits of the CPU code */
    cpuCode = snetFieldValueGet(regPtr, 3 + (profile * 5), 2);

    if(packetCmd <= SKERNEL_EXT_PKT_CMD_SOFT_DROP_E)
    {
        /* Apply packet new command */
        __LOG(("Apply packet new command"));

       if(direction == SMAIN_DIRECTION_EGRESS_E)
       {
            if(packetCmd == SKERNEL_EXT_PKT_CMD_HARD_DROP_E)
            {
                /* the egress pipe can indicate that the packet need to be dropped .
                   we can not call snetChtPktCmdResolution because it is not allowed for
                   descrPtr->packetCmd = TO_TRG_SNIFFER_E,FROM_CSCD_TO_CPU_E,FROM_CPU_E,
                */

                descrPtr->packetCmd = SKERNEL_EXT_PKT_CMD_HARD_DROP_E;

                /* Set DROP packet command to EPLR descriptor */
                descrPtr->eplrAction.drop = SKERNEL_EXT_PKT_CMD_HARD_DROP_E;
                __LOG(("Packet Command : DROP \n"));
                __LOG_PARAM(descrPtr->eplrAction.drop);
            }
            else
            {
                __LOG(("non hard drop command ignored on egress pipe \n"));
            }
        }
        else
        {
            descrPtr->packetCmd = snetChtPktCmdResolution(descrPtr->packetCmd, packetCmd);
        }

        if(descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_MIRROR_TO_CPU_E ||
           descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_TRAP_TO_CPU_E)
        {
            /* OAM CPU Code Base */
            __LOG(("OAM CPU Code Base"));
            regAddr = SMEM_LION2_OAM_EXCEPTION_CONF_REG(devObjPtr, stage);
            smemRegFldGet(devObjPtr, regAddr, 12, 8, &cpuCodeBase);
            /* CPU code is determined by Configuration<OAM CPU Code> plus
            CPU code offset from OpCode table plus CPU code offset from OAM table */
            descrPtr->cpuCode = cpuCodeBase + cpuCode + oamInfoPtr->cpuCodeOffset;
        }
        return;
    }

    if(packetCmd != 7)
    {
        /* Invalid OAM opCode packet command */
        __LOG(("Invalid OAM opCode packet command ... ignored (no packet resolution) \n"));
        return;
    }

    /* Loopback .. no really command resolution */

    if(direction == SMAIN_DIRECTION_EGRESS_E)
    {
        __LOG(("Loopback command not relevant for egress pipe \n"))
        return;
    }

    if(descrPtr->packetCmd != SKERNEL_EXT_PKT_CMD_FORWARD_E &&
       descrPtr->packetCmd != SKERNEL_EXT_PKT_CMD_MIRROR_TO_CPU_E)
    {
        __LOG(("Loopback command not relevant for non FORWARD non MIRROR_TO_CPU \n"))
        return;
    }

    __LOG(("LOOPBACK operation \n"))

    descrPtr->VntL2Echo = 1;
    __LOG_PARAM(descrPtr->VntL2Echo);
    descrPtr->cfm = 1;
    __LOG_PARAM(descrPtr->cfm);
    if(descrPtr->macDaType == SKERNEL_MULTICAST_MAC_E)
    {
        descrPtr->doRouterHa = 1;
        __LOG_PARAM(descrPtr->doRouterHa);
    }
    descrPtr->useVidx = 0;
    __LOG_PARAM(descrPtr->useVidx);
    descrPtr->eArchExtInfo.isTrgPhyPortValid = 0;
    __LOG_PARAM(descrPtr->eArchExtInfo.isTrgPhyPortValid);
    if (descrPtr->origIsTrunk)
    {
        if(descrPtr->marvellTagged)
        {
            descrPtr->targetIsTrunk = 1;
            descrPtr->trgTrunkId = descrPtr->localDevSrcTrunkId;
        }
        else
        {
            descrPtr->targetIsTrunk = 0;
            descrPtr->trgEPort = descrPtr->eArchExtInfo.localDevSrcEPort;
            descrPtr->trgDev = descrPtr->srcDev;
        }
    }
    else
    {
        descrPtr->targetIsTrunk = 0;
        descrPtr->trgEPort = descrPtr->origSrcEPortOrTrnk;
        descrPtr->trgDev = descrPtr->srcDev;
    }

    __LOG_PARAM(descrPtr->targetIsTrunk);
    __LOG_PARAM(descrPtr->trgEPort);
    __LOG_PARAM(descrPtr->trgDev);
    __LOG_PARAM(descrPtr->trgTrunkId);


}

/*******************************************************************************
*   snetLion2OamDualEndLossMeasurmentPacketCommand
*
* DESCRIPTION:
*       Assign packet command for Dual-Ended loss measurement packets are CCM
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - pointer to the frame's descriptor.
*       stage       - ingress or egress OAM stage.
*       oamInfoPtr  - pointer to OAM info structure.
*       origDescrPtr- pointer to the frame's original descriptor.
* OUTPUT:
*       descrPtr     - pointer to the frame's descriptor.
*
*
*******************************************************************************/
static GT_VOID snetLion2OamDualEndLossMeasurmentPacketCommand
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_LION2_OAM_STAGE_TYPE_ENT stage,
    IN SNET_LION2_OAM_INFO_STC * oamInfoPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * origDescrPtr
)
{
    DECLARE_FUNC_NAME(snetLion2OamDualEndLossMeasurmentPacketCommand);

    GT_U32 regAddr;                             /* Register address */
    GT_U32 * regPtr;                            /* Register pointer */
    SKERNEL_EXT_PACKET_CMD_ENT packetCmd;       /* Current packet command */
    GT_U32 cpuCode;                             /* Current packet CPU code */
    GT_U32 cpuCodeBase;                         /* OAM CPU Code Base */
    GT_U32 dualEndedOpCode;                     /* Dual-ended OpCode */
    GT_U32 opCodeIndex;                         /* Opcode index */

    if (SNET_OAM_PROCESS_ENABLE_GET(descrPtr, stage) == 0)
    {
        /* OAM table logic disabled */
        __LOG(("OAM table logic disabled"));
        return;
    }

    if(oamInfoPtr->opCodeParsingEnable == 0)
    {
        /* OAM Opcode Parsing Disable */
        __LOG(("OAM Opcode Parsing Disable"));
        return;
    }

    snetLion2OamOpCodeConfigGet(devObjPtr, descrPtr, stage,
                            SNET_LION2_OAM_OPCODE_TYPE_LM_DUAL_ENDED_E,
                            &dualEndedOpCode, &opCodeIndex);

    if (oamInfoPtr->dualEndedLmEnable &&
       (descrPtr->oamInfo.opCode == dualEndedOpCode))
    {
        regAddr = SMEM_LION2_OAM_DUAL_ENDED_LM_REG(devObjPtr, stage);
        regPtr = smemMemGet(devObjPtr, regAddr);

        /* Ignore packet command and avoid all other descriptor changes
        by OAM opcode to packet command table (e.g. changes caused by the loopback command) */

        /* Restore original  descriptor */
        *descrPtr = *origDescrPtr;

        packetCmd = SMEM_U32_GET_FIELD(regPtr[0], 8, 3);
        /* Dual-Ended LM CPU Code LSbits */
        cpuCode = SMEM_U32_GET_FIELD(regPtr[0], 11, 2);

        /* Apply packet new command */
        descrPtr->packetCmd = snetChtPktCmdResolution(descrPtr->packetCmd, packetCmd);

        if(descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_MIRROR_TO_CPU_E ||
           descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_TRAP_TO_CPU_E)
        {
            /* OAM CPU Code Base */
            __LOG(("OAM CPU Code Base"));
            regAddr = SMEM_LION2_OAM_EXCEPTION_CONF_REG(devObjPtr, stage);
            smemRegFldGet(devObjPtr, regAddr, 12, 8, &cpuCodeBase);

            /* CPU code is determined by Configuration<OAM CPU Code> plus
            Dual-Ended LM CPU Code 2 LSbits */
            descrPtr->cpuCode = cpuCodeBase + cpuCode;
        }
    }
}

/*******************************************************************************
*   snetLion2OamTimeStampTriggering
*
* DESCRIPTION:
*       Decision whether a packet should be timestamped.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - pointer to the frame's descriptor.
*       stage       - ingress or egress OAM stage.
*       oamInfoPtr  - pointer to OAM info structure.
* OUTPUT:
*       descrPtr     - pointer to the frame's descriptor.
*
*
*******************************************************************************/
static GT_VOID snetLion2OamTimeStampTriggering
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_LION2_OAM_STAGE_TYPE_ENT stage,
    IN SNET_LION2_OAM_INFO_STC * oamInfoPtr
)
{
    DECLARE_FUNC_NAME(snetLion2OamTimeStampTriggering);

    GT_U32 opCode;                              /* Opcode */
    GT_U32 opCodeIndex;                         /* Opcode index */

    if (SNET_OAM_PROCESS_ENABLE_GET(descrPtr, stage) == 0)
    {
        /* OAM table logic disabled */
        __LOG(("OAM table logic disabled"));
        return;
    }

    if(oamInfoPtr->timestampEnable == 0)
    {
        /* Timestamp disabled */
        __LOG(("Timestamp disabled"));
        return;
    }

    /* All packets that access the OAM table are triggered for timestamping */
    if(oamInfoPtr->opCodeParsingEnable == 0)
    {
        descrPtr->oamInfo.timeStampEnable = 1;
        descrPtr->oamInfo.offsetIndex = oamInfoPtr->offsetIndex;
        return;
    }




    /* DM messages OpCode parsing enable */
    __LOG(("DM messages OpCode parsing enable"));
    snetLion2OamOpCodeConfigGet(devObjPtr, descrPtr, stage,
                                SNET_LION2_OAM_OPCODE_TYPE_DM_E,
                                &opCode, &opCodeIndex);

    if(opCode)
    {
        descrPtr->oamInfo.timeStampEnable = 1;
        descrPtr->oamInfo.offsetIndex = oamInfoPtr->offsetIndex + opCodeIndex;
    }
}

/*******************************************************************************
*   snetLion2OamLossMeasurmentCounterUpdateTrigger
*
* DESCRIPTION:
*       Determines whether OAM traffic is counted or not.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - pointer to the frame's descriptor.
*       stage       - ingress or egress OAM stage.
*       oamInfoPtr  - pointer to OAM info structure.
* OUTPUT:
*       descrPtr     - pointer to the frame's descriptor.
*
*
*******************************************************************************/
static GT_VOID snetLion2OamLossMeasurmentCounterUpdateTrigger
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_LION2_OAM_STAGE_TYPE_ENT stage,
    IN SNET_LION2_OAM_INFO_STC * oamInfoPtr
)
{
    DECLARE_FUNC_NAME(snetLion2OamLossMeasurmentCounterUpdateTrigger);

    SMAIN_DIRECTION_ENT direction;              /* Ingress/Egress pipe */
    GT_U32 dualEndedOpCode;                     /* Dual-ended OpCode */
    GT_U32 opCode;                              /* LM counted opcode */
    GT_U32 opCodeIndex;                         /* Opcode index */

    if (SNET_OAM_PROCESS_ENABLE_GET(descrPtr, stage) == 0)
    {
        /* OAM table logic disabled */
        __LOG(("OAM table logic disabled"));
        return;
    }

    if(oamInfoPtr->lmCountingMode == SNET_LION2_OAM_LM_COUNTING_MODE_RETAIN_E)
    {
        /* Retain: Keep the previous decision of Policer counting */
        __LOG(("Retain: Keep the previous decision of Policer counting"));
        return;
    }

    direction = SNET_OAM_STAGE_2_DIRECTION_MAC(stage);

    if(oamInfoPtr->lmCountingMode == SNET_LION2_OAM_LM_COUNTING_MODE_DISABLE_E)
    {
        /* Packets are NOT counted by the LM counters */
        /* Disable policer billing counters */
        SNET_XCAT_POLICER_POLICY_COUNT_ENABLE_SET_MAC(devObjPtr, descrPtr,
                                                      direction, 0);
        return;
    }

    /* All packets that access the OAM table are counted by the LM counter */
    if(oamInfoPtr->opCodeParsingEnable == 0)
    {
        /* Enable policer billing counters */
        __LOG(("Enable policer billing counters"));
        SNET_XCAT_POLICER_POLICY_COUNT_ENABLE_SET_MAC(devObjPtr, descrPtr,
                                                      direction, 1);
        return;
    }

    snetLion2OamOpCodeConfigGet(devObjPtr, descrPtr, stage,
                                SNET_LION2_OAM_OPCODE_TYPE_LM_COUNTED_E,
                                &opCode, &opCodeIndex);
    if (opCode)
    {
        snetLion2OamOpCodeConfigGet(devObjPtr, descrPtr, stage,
                                SNET_LION2_OAM_OPCODE_TYPE_LM_DUAL_ENDED_E,
                                &dualEndedOpCode, &opCodeIndex);

        if (oamInfoPtr->dualEndedLmEnable &&
           (descrPtr->oamInfo.opCode == dualEndedOpCode))
        {
            __LOG(("Dual-ended OAM LM Counted Opcode enable. Dual-ended opcode found"));

            /* Disable policer billing counters */
            __LOG(("Disable policer billing counters"));
            SNET_XCAT_POLICER_POLICY_COUNT_ENABLE_SET_MAC(devObjPtr, descrPtr,
                                                          direction, 0);
        }
        else
        {
            /* Enable policer billing counters */
            __LOG(("Enable policer billing counters"));
            SNET_XCAT_POLICER_POLICY_COUNT_ENABLE_SET_MAC(devObjPtr, descrPtr,
                                                          direction, 1);
        }
    }
    else
    {
        /* Disable policer billing counters */
        __LOG(("Disable policer billing counters"));
        SNET_XCAT_POLICER_POLICY_COUNT_ENABLE_SET_MAC(devObjPtr, descrPtr,
                                                      direction, 0);
    }
}

/*******************************************************************************
*   snetLion2OamLossMeasurmentCounterCaptureTrigger
*
* DESCRIPTION:
*       Determines whether OAM Loss Measurement counters are captured (copied
*       to <Green Counter Snapshot>) or not.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - pointer to the frame's descriptor.
*       stage       - ingress or egress OAM stage.
*       oamInfoPtr  - pointer to OAM info structure.
* OUTPUT:
*       descrPtr     - pointer to the frame's descriptor.
*
*
*******************************************************************************/
static GT_VOID snetLion2OamLossMeasurmentCounterCaptureTrigger
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_LION2_OAM_STAGE_TYPE_ENT stage,
    IN SNET_LION2_OAM_INFO_STC * oamInfoPtr
)
{
    DECLARE_FUNC_NAME(snetLion2OamLossMeasurmentCounterCaptureTrigger);

    GT_U32 opCode;                              /* Opcode */
    GT_U32 opCodeIndex;                         /* Opcode index */

    if (SNET_OAM_PROCESS_ENABLE_GET(descrPtr, stage) == 0)
    {
        /* OAM table logic disabled */
        __LOG(("OAM table logic disabled"));
        return;
    }

    if(oamInfoPtr->lmCounterCaptureEnable == 0)
    {
        /* OAM LM counter is NOT captured */
        __LOG(("OAM LM counter is NOT captured"));
        return;
    }

    /* Perform OAM Loss Measurement counters capture triggering */
    __LOG(("Perform OAM Loss Measurement counters capture triggering"));
    if(oamInfoPtr->opCodeParsingEnable &&
       oamInfoPtr->lmCounterCaptureEnable == GT_TRUE)
    {
        __LOG(("OAM LM counter capture enabled. with opcode parsing"));
        snetLion2OamOpCodeConfigGet(devObjPtr, descrPtr, stage,
                                    SNET_LION2_OAM_OPCODE_TYPE_LM_DUAL_ENDED_E,
                                    &opCode, &opCodeIndex);

        if(oamInfoPtr->dualEndedLmEnable && (descrPtr->oamInfo.opCode == opCode))
        {
            /* The LM counter is captured for dual-ended OAM packets  */
            __LOG(("The OAM LM counter is captured for dual-ended OAM packets"));
            descrPtr->oamInfo.lmCounterCaptureEnable = 1;
            descrPtr->oamInfo.offsetIndex = oamInfoPtr->offsetIndex;
        }
        else
        {
            snetLion2OamOpCodeConfigGet(devObjPtr, descrPtr, stage,
                                        SNET_LION2_OAM_OPCODE_TYPE_LM_SINGLE_ENDED_E,
                                        &opCode, &opCodeIndex);
            if(opCode)
            {
                descrPtr->oamInfo.lmCounterCaptureEnable = 1;
                descrPtr->oamInfo.offsetIndex =
                    oamInfoPtr->offsetIndex + opCodeIndex;
            }
        }
    }
    else if(oamInfoPtr->lmCounterCaptureEnable == GT_TRUE)
    {
        __LOG(("OAM LM counter capture enabled. No opcode parsing"));
        descrPtr->oamInfo.lmCounterCaptureEnable = 1;
        descrPtr->oamInfo.offsetIndex = oamInfoPtr->offsetIndex;
    }
}

/*******************************************************************************
*   snetLion2OamKeepAliveMessageTrigger
*
* DESCRIPTION:
*       Keepalive message triggering
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - pointer to the frame's descriptor.
*       stage       - ingress or egress OAM stage.
*       oamInfoPtr  - pointer to OAM info structure.
* OUTPUT:
*       descrPtr    - pointer to the frame's descriptor.
*
* RETURN:
*       GT_TRUE     - Process keepalive of OAM entry
*       GT_FALSE    - Don't process keepalive of OAM entry
*
*******************************************************************************/
static GT_BOOL snetLion2OamKeepAliveMessageTrigger
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_LION2_OAM_STAGE_TYPE_ENT stage,
    IN SNET_LION2_OAM_INFO_STC * oamInfoPtr
)
{
    DECLARE_FUNC_NAME(snetLion2OamKeepAliveMessageTrigger);

    GT_U32 opCode;                  /* Opcode */
    GT_U32 opCodeIndex;             /* Opcode index */
    GT_U32 regAddr;                 /* Register address */
    GT_U32 * regPtr;                /* Register pointer */

    if (SNET_OAM_PROCESS_ENABLE_GET(descrPtr, stage) == 0)
    {
        /* OAM table logic disabled */
        __LOG(("OAM table logic disabled"));
        return GT_FALSE;
    }

    if(oamInfoPtr->keepaliveAgingEnable == 0)
    {
        /* The current OAM entry is NOT subject to the aging mechanism */
        __LOG(("The current OAM entry is NOT subject to the aging mechanism"));
        return GT_FALSE;
    }

    regAddr = SMEM_LION2_OAM_INVALID_KEEPALIVE_EXCEPTION_CONF_REG(devObjPtr, stage);
    regPtr = smemMemGet(devObjPtr, regAddr);

    if(descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_HARD_DROP_E)
    {
        if (SMEM_U32_GET_FIELD(regPtr[0], 12, 1) == 0)
        {
            __LOG(("The packet command is SKERNEL_EXT_PKT_CMD_HARD_DROP_E and Keepalive for Hard Drop is disabled\n"));
            __LOG(("The current OAM entry is NOT subject to keepalive processing"));
            return GT_FALSE;
        }
        else
        {
            __LOG(("The packet command is SKERNEL_EXT_PKT_CMD_HARD_DROP_E and Keepalive for Hard Drop is enabled\n"));
        }
    }

    if(descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_SOFT_DROP_E)
    {
        if (SMEM_U32_GET_FIELD(regPtr[0], 13, 1) == 0)
        {
            __LOG(("The packet command is SKERNEL_EXT_PKT_CMD_SOFT_DROP_E and Keepalive for Soft Drop is disabled\n"));
            __LOG(("The current OAM entry is NOT subject to keepalive processing"));
            return GT_FALSE;
        }
        else
        {
            __LOG(("The packet command is SKERNEL_EXT_PKT_CMD_SOFT_DROP_E and Keepalive for Soft Drop is enabled\n"));
        }
    }


    /* All OAM Packets are to the aging mechanism */
    if(oamInfoPtr->opCodeParsingEnable == 0)
    {
        /* Proceed to keepalive processing */
        __LOG(("Proceed to keepalive processing"));
        return GT_TRUE;
    }

    /* The opcode/function type of keepalive messages (e.g. CCM/CV/FFD */
    snetLion2OamOpCodeConfigGet(devObjPtr, descrPtr, stage,
                                SNET_LION2_OAM_OPCODE_TYPE_KEEPALIVE_E,
                                &opCode, &opCodeIndex);
    if (opCode)
    {
        /* Proceed to keepalive processing */
        __LOG(("Proceed to keepalive processing"));
        return GT_TRUE;
    }

    __LOG(("The current OAM entry is NOT subject to keepalive processing"));
    return GT_FALSE;
}

/*******************************************************************************
*   snetLion2OamKeepAliveExcessDetect
*
* DESCRIPTION:
*       Keepalive Excess Detection
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - pointer to the frame's descriptor.
*       stage       - ingress or egress OAM stage.
*       oamInfoPtr  - pointer to OAM info structure.
* OUTPUT:
*       descrPtr    - pointer to the frame's descriptor.
*
*******************************************************************************/
static GT_VOID snetLion2OamKeepAliveExcessDetect
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_LION2_OAM_STAGE_TYPE_ENT stage,
    INOUT SNET_LION2_OAM_INFO_STC * oamInfoPtr
)
{
    DECLARE_FUNC_NAME(snetLion2OamKeepAliveExcessDetect);

    GT_U32 regAddr;                             /* Register address */
    GT_U32 fldValue;                            /* Register field */
    GT_U32 * regPtr;                            /* Register pointer */
    GT_U32 keepAliveExcessSummaryEn;            /* Keepalive excess summary bit */

    /* Keepalive Excess Detection */
    if(oamInfoPtr->excessKeepaliveDetectionEnable == 0)
    {
        return;
    }

    if(oamInfoPtr->excessKeepaliveMessageCounter !=
       oamInfoPtr->excessKeepaliveMessageThreshold)
    {
        oamInfoPtr->excessKeepaliveMessageCounter++;
    }
    else
    {
        /* Generate interrupt  "Bridge Interrupt Cause Register" bit 28 */
        snetChetahDoInterrupt(devObjPtr,
                              SMEM_LION2_OAM_INTR_CAUSE_REG(devObjPtr, stage),
                              SMEM_LION2_OAM_INTR_MASK_REG(devObjPtr, stage),
                              SNET_LION2_OAM_INTR_EXCESS_KEEPALIVE_E,
                              SMEM_CHT_L2I_SUM_INT(devObjPtr));

        regAddr = SMEM_LION2_EXCESS_KEEPALIVE_TBL_MEM(devObjPtr, stage,
                                                      oamInfoPtr->entryIndex / 32);
        regPtr = smemMemGet(devObjPtr, regAddr);
        /* Set bit in the Keepalive Table that has had an Excess Keepalive exception detected */
        SMEM_U32_SET_FIELD(regPtr[0], oamInfoPtr->entryIndex % 32, 1, 1);

        regAddr = SMEM_LION2_OAM_EXCEPTION_CONF_REG(devObjPtr, stage);
        regPtr = smemMemGet(devObjPtr, regAddr);
        keepAliveExcessSummaryEn = SMEM_U32_GET_FIELD(regPtr[0], 27, 1);
        /* Whether keepalive excess exceptions are included in the OAM exception bitmap */
        if(keepAliveExcessSummaryEn)
        {
            regAddr = SMEM_LION2_OAM_EXCEPTION_SUMMARY_TBL_MEM(devObjPtr,stage,
                                                               oamInfoPtr->entryIndex / 32);
            regPtr = smemMemGet(devObjPtr, regAddr);
            /* Set for every entry in the OAM table that has triggered an exception */
            __LOG(("Set for every entry in the OAM table that has triggered an exception"));
            SMEM_U32_SET_FIELD(regPtr[0], oamInfoPtr->entryIndex % 32, 1, 1);
        }
        /* Counts the Excess Keepalive exceptions in the OAM stage */
        regAddr = SMEM_LION2_OAM_EXCESS_KEEPALIVE_COUNTER_REG(devObjPtr, stage);
        smemRegFldGet(devObjPtr, regAddr, 0, 32, &fldValue);
        smemRegFldSet(devObjPtr, regAddr, 0, 32, ++fldValue);
        __LOG(("stage[%d] : counter [%s] changed from [%d] to [%d] \n" ,
            stage ,
            "OAM_EXCESS_KEEPALIVE_COUNTER" ,
            fldValue - 1 ,
            fldValue));
    }
}

/*******************************************************************************
*   snetLion2OamRdiBitCheck
*
* DESCRIPTION:
*       Checking the RDI Bit
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - pointer to the frame's descriptor.
*       stage       - ingress or egress OAM stage.
*       oamInfoPtr  - pointer to OAM info structure.
* OUTPUT:
*       descrPtr    - pointer to the frame's descriptor.
*
*******************************************************************************/
static GT_VOID snetLion2OamRdiBitCheck
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_LION2_OAM_STAGE_TYPE_ENT stage,
    INOUT SNET_LION2_OAM_INFO_STC * oamInfoPtr
)
{
    DECLARE_FUNC_NAME(snetLion2OamRdiBitCheck);

    GT_U32 regAddr;                             /* Register address */
    GT_U32 fldValue;                            /* Register field */
    GT_U32 * regPtr;                            /* Register pointer */
    SKERNEL_EXT_PACKET_CMD_ENT packetCmd;       /* Current packet command */

    if(oamInfoPtr->rdiCheckEnable == 0)
    {
        /* RDI Bit check disabled */
        __LOG(("RDI Bit check disabled"));
        return;
    }

    if(oamInfoPtr->rdiStatus == descrPtr->oamInfo.oamRdi)
    {
        /* RDI status of the current OAM flow not changed */
        __LOG(("RDI status of the current OAM flow not changed"));
        return;
    }
    regAddr = SMEM_LION2_OAM_EXCEPTION_CONF1_REG(devObjPtr, stage);
    regPtr = smemMemGet(devObjPtr, regAddr);

    packetCmd = SMEM_U32_GET_FIELD(regPtr[0], 11, 3);
    /* Apply packet new command */
    descrPtr->packetCmd = snetChtPktCmdResolution(descrPtr->packetCmd, packetCmd);

    if(descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_MIRROR_TO_CPU_E ||
       descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_TRAP_TO_CPU_E)
    {
        descrPtr->cpuCode = SMEM_U32_GET_FIELD(regPtr[0], 14, 8);
    }

    oamInfoPtr->rdiStatus = descrPtr->oamInfo.oamRdi;

    /* Generate interrupt  "Bridge Interrupt Cause Register" bit 28 */
    snetChetahDoInterrupt(devObjPtr,
                          SMEM_LION2_OAM_INTR_CAUSE_REG(devObjPtr, stage),
                          SMEM_LION2_OAM_INTR_MASK_REG(devObjPtr, stage),
                          SNET_LION2_OAM_INTR_RDI_STATUS_E,
                          SMEM_CHT_L2I_SUM_INT(devObjPtr));

    regAddr = SMEM_LION2_RDI_STATUS_CHANGE_EXCEPTION_TBL_MEM(devObjPtr, stage,
                          oamInfoPtr->entryIndex / 32);
    regPtr = smemMemGet(devObjPtr, regAddr);
    /* Set bit in RDI level exception entry that has had a RDI bit exception */
    SMEM_U32_SET_FIELD(regPtr[0], oamInfoPtr->entryIndex % 32, 1, 1);

    regAddr = SMEM_LION2_OAM_EXCEPTION_CONF_REG(devObjPtr, stage);
    regPtr = smemMemGet(devObjPtr, regAddr);

    fldValue = SMEM_U32_GET_FIELD(regPtr[0], 25, 1);
    /* RDI status changes is included in the OAM exception bitmap */
    if(fldValue)
    {
        regAddr = SMEM_LION2_OAM_EXCEPTION_SUMMARY_TBL_MEM(devObjPtr,stage,
                                                           oamInfoPtr->entryIndex / 32);
        regPtr = smemMemGet(devObjPtr, regAddr);
        /* Set for every entry in the OAM table that has triggered an exception */
        SMEM_U32_SET_FIELD(regPtr[0], oamInfoPtr->entryIndex % 32, 1, 1);
    }
    /* Counts the RDI status change in the OAM stage */
    regAddr = SMEM_LION2_OAM_RDI_STATUS_CHANGE_COUNTER_REG(devObjPtr, stage);
    smemRegFldGet(devObjPtr, regAddr, 0, 32, &fldValue);
    smemRegFldSet(devObjPtr, regAddr, 0, 32, ++fldValue);
    __LOG(("stage[%d] : counter [%s] changed from [%d] to [%d] \n" ,
        stage ,
        "OAM_RDI_STATUS_CHANGE_COUNTER" ,
        fldValue - 1 ,
        fldValue));
}

/*******************************************************************************
*   snetLion2OamTxPeriodFieldCheck
*
* DESCRIPTION:
*       Checking the period field
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - pointer to the frame's descriptor.
*       stage       - ingress or egress OAM stage.
*       oamInfoPtr  - pointer to OAM info structure.
* OUTPUT:
*       descrPtr    - pointer to the frame's descriptor.
*
*******************************************************************************/
static GT_VOID snetLion2OamTxPeriodFieldCheck
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_LION2_OAM_STAGE_TYPE_ENT stage,
    INOUT SNET_LION2_OAM_INFO_STC * oamInfoPtr
)
{
    DECLARE_FUNC_NAME(snetLion2OamTxPeriodFieldCheck);

    GT_U32 regAddr;                             /* Register address */
    GT_U32 fldValue;                            /* Register field */
    GT_U32 * regPtr;                            /* Register pointer */
    SKERNEL_EXT_PACKET_CMD_ENT packetCmd;       /* Packet command */

    if(oamInfoPtr->periodCheckEnable == 0)
    {
        /* Period check disabled */
        __LOG(("Period check disabled"));
        return;
    }

    if(oamInfoPtr->keepaliveTxPeriod == descrPtr->oamInfo.oamTxPeriod)
    {
        return;
    }

    regAddr = SMEM_LION2_OAM_EXCEPTION_CONF2_REG(devObjPtr, stage);
    regPtr = smemMemGet(devObjPtr, regAddr);

    packetCmd = SMEM_U32_GET_FIELD(regPtr[0], 0, 3);
    /* Apply packet new command */
    __LOG(("Apply packet new command"));
    descrPtr->packetCmd = snetChtPktCmdResolution(descrPtr->packetCmd, packetCmd);

    if(descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_MIRROR_TO_CPU_E ||
       descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_TRAP_TO_CPU_E)
    {
        descrPtr->cpuCode = SMEM_U32_GET_FIELD(regPtr[0], 3, 8);
    }


    regAddr = SMEM_LION2_OAM_TX_PERIOD_EXCEPTION_TBL_MEM(devObjPtr, stage,
                                                         oamInfoPtr->entryIndex / 32);
    regPtr = smemMemGet(devObjPtr, regAddr);
    /* Set bit in the Tx Period Exception Table */
    SMEM_U32_SET_FIELD(regPtr[0], oamInfoPtr->entryIndex % 32, 1, 1);

    /* Generate interrupt  "Bridge Interrupt Cause Register" bit 28 */
    snetChetahDoInterrupt(devObjPtr,
                          SMEM_LION2_OAM_INTR_CAUSE_REG(devObjPtr, stage),
                          SMEM_LION2_OAM_INTR_MASK_REG(devObjPtr, stage),
                          SNET_LION2_OAM_INTR_TX_PERIOD_E,
                          SMEM_CHT_L2I_SUM_INT(devObjPtr));

    /* Counts the TX Period Exception in the OAM stage */
    regAddr = SMEM_LION2_OAM_TX_PERIOD_EXCEPTION_COUNTER_REG(devObjPtr, stage);
    smemRegFldGet(devObjPtr, regAddr, 0, 32, &fldValue);
    smemRegFldSet(devObjPtr, regAddr, 0, 32, ++fldValue);
    __LOG(("stage[%d] : counter [%s] changed from [%d] to [%d] \n" ,
        stage ,
        "OAM_TX_PERIOD_EXCEPTION_COUNTE" ,
        fldValue - 1 ,
        fldValue));

    regAddr = SMEM_LION2_OAM_EXCEPTION_CONF_REG(devObjPtr, stage);
    regPtr = smemMemGet(devObjPtr, regAddr);

    fldValue = SMEM_U32_GET_FIELD(regPtr[0], 31, 1);
    /* TX Period Exception is included in the OAM exception bitmap */
    if(fldValue)
    {
        regAddr = SMEM_LION2_OAM_EXCEPTION_SUMMARY_TBL_MEM(devObjPtr,stage,
                                                           oamInfoPtr->entryIndex / 32);
        regPtr = smemMemGet(devObjPtr, regAddr);
        /* Set for every entry in the OAM table that has triggered an exception */
        SMEM_U32_SET_FIELD(regPtr[0], oamInfoPtr->entryIndex % 32, 1, 1);
    }
}

/*******************************************************************************
*   snetLion2OamKeepAliveFlowHashCheck
*
* DESCRIPTION:
*       Checking the flow hash
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - pointer to the frame's descriptor.
*       stage       - ingress or egress OAM stage.
*       oamInfoPtr  - pointer to OAM info structure.
* OUTPUT:
*       descrPtr    - pointer to the frame's descriptor.
*
*******************************************************************************/
static GT_VOID snetLion2OamKeepAliveFlowHashCheck
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_LION2_OAM_STAGE_TYPE_ENT stage,
    INOUT SNET_LION2_OAM_INFO_STC * oamInfoPtr
)
{
    DECLARE_FUNC_NAME(snetLion2OamKeepAliveFlowHashCheck);

    GT_U32 regAddr;                             /* Register address */
    GT_U32 * regPtr;                            /* Register pointer */
    GT_U32 fldValue;                            /* Register field */
    GT_U32 invalidKeepAliveSummaryEn;           /* Invalid keepalive hash summary bit */
    SKERNEL_EXT_PACKET_CMD_ENT packetCmd;       /* Current packet command */
    GT_U32 cpuCode;                             /* Current packet CPU code */
    GT_U32 flowHash;                            /* Hash value used in hash based verification */
    SMAIN_DIRECTION_ENT direction;              /* Ingress/Egress pipe */
    GT_U32 firstBit, lastBit;                   /* First bit and last bit to be used from the hash value */

    /* Hash Verify Disabled */
    if(oamInfoPtr->hashVerifyEnable == 0)
    {
        return;
    }

    direction = SNET_OAM_STAGE_2_DIRECTION_MAC(stage);
    if(direction == SMAIN_DIRECTION_INGRESS_E)
    {
        /* Ingress select subset of the bits from the descriptor hash */
        regAddr = SMEM_LION2_OAM_HASH_BIT_SELECTION_CONFIG_REG(devObjPtr, stage);
        regPtr = smemMemGet(devObjPtr, regAddr);
        firstBit =  snetFieldValueGet(regPtr, 0, 5);
        lastBit  =  snetFieldValueGet(regPtr, 5, 5);
        flowHash = SMEM_U32_GET_FIELD(descrPtr->pktHash, firstBit, (lastBit - firstBit) + 1);
    }
    else
    {
        /* Egress - choose the 12 lsb bits */
        flowHash = descrPtr->pktHash & 0xfff;
    }

    if(flowHash != oamInfoPtr->flowHash ||
       oamInfoPtr->lockHashValueEnable == 0)
    {
        regAddr = SMEM_LION2_OAM_INVALID_KEEPALIVE_EXCEPTION_CONF_REG(devObjPtr, stage);
        regPtr = smemMemGet(devObjPtr, regAddr);

        /* the device does not use Hash mismatch exception packet command when
           lockHashValueEnable = GT_FALSE starting from SIP 5.15 (BobK)
           Bobcat2 use Hash mismatch exception packet command regardless of lockHashValueEnable */
        if((0 == SMEM_CHT_IS_SIP5_15_GET(devObjPtr)) || (oamInfoPtr->lockHashValueEnable == 1))
        {
            packetCmd = SMEM_U32_GET_FIELD(regPtr[0], 0, 3);
            cpuCode = SMEM_U32_GET_FIELD(regPtr[0], 4, 8);

            /* Apply packet new command */
            __LOG(("Apply packet new command"));
            descrPtr->packetCmd = snetChtPktCmdResolution(descrPtr->packetCmd, packetCmd);

            if(descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_MIRROR_TO_CPU_E ||
               descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_TRAP_TO_CPU_E)
            {
                descrPtr->cpuCode = cpuCode;
            }
        }

        /* Generate interrupt  "Bridge Interrupt Cause Register" bit 28 */
        snetChetahDoInterrupt(devObjPtr,
                              SMEM_LION2_OAM_INTR_CAUSE_REG(devObjPtr, stage),
                              SMEM_LION2_OAM_INTR_MASK_REG(devObjPtr, stage),
                              SNET_LION2_OAM_INTR_INVALID_KEEPALIVE_HASH_E,
                              SMEM_CHT_L2I_SUM_INT(devObjPtr));

        regAddr = SMEM_LION2_INVALID_KEEPALIVE_HASH_TBL_MEM(devObjPtr, stage,
                                                            oamInfoPtr->entryIndex / 32);
        regPtr = smemMemGet(devObjPtr, regAddr);
        /* Set bit the Keepalive Table that has had a hash mismatch */
        __LOG(("Set bit the Keepalive Table that has had a hash mismatch"));
        SMEM_U32_SET_FIELD(regPtr[0], oamInfoPtr->entryIndex % 32, 1, 1);

        regAddr = SMEM_LION2_OAM_EXCEPTION_CONF_REG(devObjPtr, stage);
        regPtr = smemMemGet(devObjPtr, regAddr);
        /* Invalid Keepalive Summary Enable */
        invalidKeepAliveSummaryEn = SMEM_U32_GET_FIELD(regPtr[0], 28, 1);

        if (invalidKeepAliveSummaryEn)
        {
            regAddr = SMEM_LION2_OAM_EXCEPTION_SUMMARY_TBL_MEM(devObjPtr,stage,
                                                               oamInfoPtr->entryIndex / 32);
            regPtr = smemMemGet(devObjPtr, regAddr);
            /* Set for every entry in the OAM table that has triggered an exception */
            SMEM_U32_SET_FIELD(regPtr[0], oamInfoPtr->entryIndex % 32, 1, 1);
        }
        /* Counts the hash mismatches in the OAM stage */
        regAddr = SMEM_LION2_OAM_INVALID_KEEPALIVE_HASH_COUNTER_REG(devObjPtr, stage);
        smemRegFldGet(devObjPtr, regAddr, 0, 32, &fldValue);
        smemRegFldSet(devObjPtr, regAddr, 0, 32, ++fldValue);
        __LOG(("stage[%d] : counter [%s] changed from [%d] to [%d] \n" ,
            stage ,
            "OAM_INVALID_KEEPALIVE_HASH_COUNTER" ,
            fldValue - 1 ,
            fldValue));

        /* Update <Flow Hash> field, and set the <Lock Hash Value Enable> field */
        if(oamInfoPtr->lockHashValueEnable == 0)
        {
            /* OAM flow hash is 12 bits value */
            __LOG(("OAM flow hash is 12 bits value"));
            oamInfoPtr->flowHash = flowHash;
            oamInfoPtr->lockHashValueEnable = 1;
        }
    }
}

/*******************************************************************************
*   snetLion2OamKeepAliveMessageProcess
*
* DESCRIPTION:
*       Keepalive message processing
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - pointer to the frame's descriptor.
*       stage       - ingress or egress OAM stage.
*       oamInfoPtr  - pointer to OAM info structure.
* OUTPUT:
*       descrPtr    - pointer to the frame's descriptor.
*
*******************************************************************************/
static GT_VOID snetLion2OamKeepAliveMessageProcess
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_LION2_OAM_STAGE_TYPE_ENT stage,
    IN SNET_LION2_OAM_INFO_STC * oamInfoPtr
)
{
    DECLARE_FUNC_NAME(snetLion2OamKeepAliveMessageProcess);

    GT_U32 i;

    /* Cleared each time a keepalive message arrives */
    __LOG(("Cleared each time a keepalive message arrives"));
    oamInfoPtr->ageState = 0;
    for(i = 0; i < 8; i++)
    {
        /* Reset timers to initial value */
        devObjPtr->oamSupport.keepAlivePeriodCounter[stage].keepAlivePeriodCounterTimer[i].l[0] =
        devObjPtr->oamSupport.keepAlivePeriodCounter[stage].keepAlivePeriodCounterTimer[i].l[1] = 0;
    }
    /* Keepalive Excess Detection */
    snetLion2OamKeepAliveExcessDetect(devObjPtr, descrPtr, stage, oamInfoPtr);

    /* Checking the RDI Bit */
    snetLion2OamRdiBitCheck(devObjPtr, descrPtr, stage, oamInfoPtr);

    /* Checking the Period Field */
    snetLion2OamTxPeriodFieldCheck(devObjPtr, descrPtr, stage, oamInfoPtr);

    /* Checking the flow hash */
    snetLion2OamKeepAliveFlowHashCheck(devObjPtr, descrPtr, stage, oamInfoPtr);

    /* Set OAM table entry */
    snetLion2OamInfoSet(devObjPtr, stage, oamInfoPtr);
}

/*******************************************************************************
*   snetLion2IOamProcess
*
* DESCRIPTION:
*       Ingress OAM processing
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - pointer to the frame's descriptor.
*
* OUTPUT:
*
*
*******************************************************************************/
GT_VOID snetLion2IOamProcess
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetLion2IOamProcess);

    GT_U32 regAddr;                             /* Register address */
    GT_U32 * regPtr;                            /* Register pointer */
    GT_U32 fieldVal;                            /* Register filed value */
    SNET_LION2_OAM_INFO_STC oamInfo;            /* OAM table structure */
    SNET_LION2_OAM_STAGE_TYPE_ENT stage;                               /* ingress or egress OAM stage. */
    SKERNEL_FRAME_CHEETAH_DESCR_STC origDescrPtr;/* Original descriptor */
    GT_BOOL retVal;                             /* Return value */

    /* Ingress OAM stage */
    stage = SNET_LION2_OAM_STAGE_INGRESS_E;

    if(SNET_OAM_PROCESS_ENABLE_GET(descrPtr, stage) == 0)
    {
        /* Ingress OAM processing disabled */
        __LOG(("Ingress OAM processing disabled"));
        return;
    }

    regAddr = SMEM_LION2_OAM_GLOBAL_CONTROL_REG(devObjPtr, stage);
    regPtr = smemMemGet(devObjPtr, regAddr);

    /* OAM enable */
    fieldVal = SMEM_U32_GET_FIELD(regPtr[0], 0, 1);
    if(fieldVal == 0)
    {
        /* OAM disabled in this Policer stage */
        __LOG(("OAM disabled in this Policer stage"));
        return;
    }

    /* OAM table base Flow Id */
    fieldVal = SMEM_U32_GET_FIELD(regPtr[0], 16, 16);

    /* Set OAM entry index */
    oamInfo.entryIndex = descrPtr->flowId - fieldVal;

    /* Access OAM0, OAM1 */
    snetLion2OamInfoGet(devObjPtr, stage, &oamInfo);

    /* MEG Level(MEL) Check */
    snetLion2OamMegLevelCheck(devObjPtr, descrPtr, stage, &oamInfo);

    /* Source Interface Verification */
    snetLion2OamSourceInterfaceVerification(devObjPtr, descrPtr, stage, &oamInfo);

    /* OAM Packet Command Resolution */
    snetLion2OamOpCodeParsing(devObjPtr, descrPtr, &oamInfo, stage, &origDescrPtr);

    /* Packet Command for Dual-Ended Loss Measurement packets */
    snetLion2OamDualEndLossMeasurmentPacketCommand(devObjPtr, descrPtr, stage,
                                                   &oamInfo, &origDescrPtr);
    /* Timestamp triggering */
    snetLion2OamTimeStampTriggering(devObjPtr, descrPtr, stage, &oamInfo);

    /* Loss Measurement Counter Update Triggering */
    snetLion2OamLossMeasurmentCounterUpdateTrigger(devObjPtr, descrPtr, stage,
                                                   &oamInfo);
    /* Loss Measurement Counter Capture Triggering */
    snetLion2OamLossMeasurmentCounterCaptureTrigger(devObjPtr, descrPtr, stage,
                                                    &oamInfo);
    /* Keepalive process triggering */
    retVal = snetLion2OamKeepAliveMessageTrigger(devObjPtr, descrPtr, stage, &oamInfo);
    if(retVal == GT_TRUE)
    {
        /* Keepalive process */
        __LOG(("Keepalive process"));
        snetLion2OamKeepAliveMessageProcess(devObjPtr, descrPtr, stage, &oamInfo);
    }
}

/*******************************************************************************
*   snetLion2EOamProcess
*
* DESCRIPTION:
*       Egress OAM processing
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - pointer to the frame's descriptor.
*
* OUTPUT:
*
*
*******************************************************************************/
GT_VOID snetLion2EOamProcess
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetLion2EOamProcess);

    GT_U32 regAddr;                             /* Register address */
    GT_U32 * regPtr;                            /* Register pointer */
    GT_U32 fieldVal;                            /* Register filed value */
    SNET_LION2_OAM_INFO_STC oamInfo;            /* OAM table structure */
    SNET_LION2_OAM_STAGE_TYPE_ENT stage;                               /* ingress or egress OAM stage. */
    SKERNEL_FRAME_CHEETAH_DESCR_STC origDescrPtr;/* Original descriptor */
    GT_BOOL retVal;                             /* Return value */

    /* Egress OAM stage */
    stage = SNET_LION2_OAM_STAGE_EGRESS_E;

    if(SNET_OAM_PROCESS_ENABLE_GET(descrPtr, stage) == 0)
    {
        /* Egress OAM processing disabled */
        __LOG(("Egress OAM processing disabled"));
        return;
    }

    regAddr = SMEM_LION2_OAM_GLOBAL_CONTROL_REG(devObjPtr, stage);
    regPtr = smemMemGet(devObjPtr, regAddr);

    /* OAM enable */
    fieldVal = SMEM_U32_GET_FIELD(regPtr[0], 0, 1);
    if(fieldVal == 0)
    {
        /* OAM disabled in this Policer stage */
        __LOG(("OAM disabled in this Policer stage"));
        return;
    }

    /* OAM table base Flow Id */
    fieldVal = SMEM_U32_GET_FIELD(regPtr[0], 16, 16);

    /* Set OAM entry index */
    oamInfo.entryIndex = descrPtr->flowId - fieldVal;

    /* Access OAM0, OAM1 */
    snetLion2OamInfoGet(devObjPtr, stage, &oamInfo);

    /* MEG Level(MEL) Check */
    snetLion2OamMegLevelCheck(devObjPtr, descrPtr, stage, &oamInfo);

    /* Source Interface Verification */
    snetLion2OamSourceInterfaceVerification(devObjPtr, descrPtr, stage, &oamInfo);

    /* OAM Packet Command Resolution */
    snetLion2OamOpCodeParsing(devObjPtr, descrPtr, &oamInfo, stage, &origDescrPtr);

    /* Packet Command for Dual-Ended Loss Measurement packets */
    snetLion2OamDualEndLossMeasurmentPacketCommand(devObjPtr, descrPtr, stage,
                                                   &oamInfo, &origDescrPtr);
    /* Timestamp triggering */
    snetLion2OamTimeStampTriggering(devObjPtr, descrPtr, stage, &oamInfo);

    /* Loss Measurement Counter Update Triggering */
    snetLion2OamLossMeasurmentCounterUpdateTrigger(devObjPtr, descrPtr, stage,
                                                   &oamInfo);
    /* Loss Measurement Counter Capture Triggering */
    snetLion2OamLossMeasurmentCounterCaptureTrigger(devObjPtr, descrPtr, stage,
                                                    &oamInfo);
    /* Keepalive process triggering */
    retVal = snetLion2OamKeepAliveMessageTrigger(devObjPtr, descrPtr, stage, &oamInfo);
    if(retVal == GT_TRUE)
    {
        /* Keepalive process */
        __LOG(("Keepalive process"));
        snetLion2OamKeepAliveMessageProcess(devObjPtr, descrPtr, stage, &oamInfo);
    }
}

/*******************************************************************************
*   snetLion2OamAgeProcess
*
* DESCRIPTION:
*       Access the OAM table and update the keepalive aging state
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       dataPtr         - pointer to OAM keepalive aging info
*       dataSize        - data size
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID snetLion2OamAgeProcess
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U8                 * dataPtr,
    IN GT_U32                  dataSize
)
{
    DECLARE_FUNC_NAME(snetLion2OamAgeProcess);

    GT_U32  regAddr;                            /* Register address */
    GT_U32 * regPtr;                            /* Register pointer */
    GT_U32  fldValue;                           /* Register field value */
    GT_U32 agingPeriodIndex;                    /* OAM aging period index */
    GT_U32 agingBitmapUpdateMode;               /* Aging Bitmap Update Mode */
    GT_U32 * agingBitmapPtr;                    /* Pointer to aging bitmap table */
    SNET_LION2_OAM_INFO_STC oamInfo;            /* OAM table structure */
    GT_U32 keepAliveAgingSummaryEn;             /* Include keepalive aging exceptions
                                                 in the OAM exception bitmap */
    GT_BOOL updateKeepAliveEntry;               /* Indicate weather to update keepalive aging bitmap */
    GT_BOOL updateOamEntry;                     /* Indicate weather to update OAM entry */
    GT_BOOL agedOutEntry;                       /* Indicate weather keepalive aging bitmap entry is aged out */
    GT_U32  doAging;                            /* indication to do aging */
    SNET_LION2_OAM_STAGE_TYPE_ENT stage = SNET_LION2_OAM_STAGE_INGRESS_E;

    /* allow using __LOG .. to debug the 'oam' feature */
    SIM_OS_MAC(simOsTaskOwnTaskPurposeSet)(
        SIM_OS_TASK_PURPOSE_TYPE_PP_PIPE_OAM_KEEP_ALIVE_DAEMON_E,
        &devObjPtr->task_oamKeepAliveAgingCookieInfo.generic);

    simLogPacketFrameUnitSet(SIM_LOG_FRAME_UNIT___ALLOW_ALL_UNITS___E);/* wild card for LOG without the unit filter */
    simLogPacketFrameCommandSet(SIM_LOG_FRAME_COMMAND_TYPE_GENERAL_E);

    regAddr = SMEM_LION2_OAM_GLOBAL_CONTROL1_REG(devObjPtr, 0);
    smemRegFldGet(devObjPtr, regAddr, 1, 1, &agingBitmapUpdateMode);

    /* Copy OAM aging period index from buffer */
    memcpy(&agingPeriodIndex, (GT_U32*)dataPtr, sizeof(GT_U32));

    SCIB_SEM_TAKE;
    for(oamInfo.entryIndex = 0;
        oamInfo.entryIndex < devObjPtr->oamNumEntries;
        oamInfo.entryIndex++)
    {
        /* Read current OAM entry */
        snetLion2OamInfoGet(devObjPtr, stage, &oamInfo);
        if(oamInfo.agingPeriodIndex != agingPeriodIndex)
        {
            continue;
        }

        if(oamInfo.keepaliveAgingEnable == 0)
        {
            continue;
        }

        doAging = 1;

        if(SMEM_CHT_IS_SIP5_15_GET(devObjPtr))
        {
            if(oamInfo.locDetectionEnable == 0)
            {
                /* the loc detection disabled ... no aging update !? */
                __LOG(("OAM entry [%d] no aging update due to <loc Detection Enable> == 0 \n",
                       oamInfo.entryIndex));

                doAging = 0;
            }
        }

        updateKeepAliveEntry = GT_TRUE;
        agedOutEntry = GT_FALSE;
        updateOamEntry = GT_FALSE;

        if (doAging == 0)
        {
            /* no aging relate done */
            /* still the  "Keepalive Excess Detection" is checked */
        }
        else
        if(oamInfo.ageState != oamInfo.agingThreshold)
        {
            updateOamEntry = GT_TRUE;
            oamInfo.ageState++;
            /* mode is update all */
            if(agingBitmapUpdateMode == 0)
            {
                updateKeepAliveEntry = GT_FALSE;
            }
            __LOG(("OAM entry [%d] age state [%d] age treshold [%d] \n",
                   oamInfo.entryIndex, oamInfo.ageState, oamInfo.agingThreshold));

        }
        else
        {
            /* Generate interrupt  "Bridge Interrupt Cause Register" bit 28 */
            snetChetahDoInterrupt(devObjPtr,
                                  SMEM_LION2_OAM_INTR_CAUSE_REG(devObjPtr, 0),
                                  SMEM_LION2_OAM_INTR_MASK_REG(devObjPtr, 0),
                                  SNET_LION2_OAM_INTR_KEEP_ALIVE_E,
                                  SMEM_CHT_L2I_SUM_INT(devObjPtr));

            agedOutEntry = GT_TRUE;

            if(oamInfo.protectionLocUpdateEnable)
            {
                __LOG(("Set the corresponding bit [%d] in the <Protection LOC Table>\n", oamInfo.entryIndex));
                regAddr = SMEM_LION2_OAM_PROTECTION_LOC_STATUS_TBL_MEM(devObjPtr,
                                                                       oamInfo.entryIndex);
                regPtr = smemMemGet(devObjPtr, regAddr);
                /* Set for every entry in the LOC Protection table that has triggered an exception */
                SMEM_U32_SET_FIELD(regPtr[0], oamInfo.entryIndex % 32, 1, 1);
            }

            regAddr = SMEM_LION2_OAM_EXCEPTION_CONF_REG(devObjPtr, 0);
            regPtr = smemMemGet(devObjPtr, regAddr);
            keepAliveAgingSummaryEn = SMEM_U32_GET_FIELD(regPtr[0], 26, 1);
            if (keepAliveAgingSummaryEn)
            {
                regAddr = SMEM_LION2_OAM_EXCEPTION_SUMMARY_TBL_MEM(devObjPtr, 0,
                                                                   oamInfo.entryIndex / 32);
                regPtr = smemMemGet(devObjPtr, regAddr);
                /* Set for every entry in the OAM table that has triggered an exception */
                SMEM_U32_SET_FIELD(regPtr[0], oamInfo.entryIndex % 32, 1, 1);
            }
            regAddr = SMEM_LION2_OAM_KEEPALIVE_AGING_COUNTER_REG(devObjPtr, 0);
            smemRegFldGet(devObjPtr, regAddr, 0, 32, &fldValue);
            smemRegFldSet(devObjPtr, regAddr, 0, 32, ++fldValue);
            __LOG(("stage[%d] : counter [%s] changed from [%d] to [%d] \n" ,
                stage ,
                "OAM_KEEPALIVE_AGING_COUNTER" ,
                fldValue - 1 ,
                fldValue));

            __LOG(("OAM entry [%d] aged out\n", oamInfo.entryIndex));
        }

        /* Keepalive Excess Detection */
        if(oamInfo.excessKeepaliveDetectionEnable)
        {
            updateOamEntry = GT_TRUE;
            if(oamInfo.excessKeepalivePeriodCounter + 1 ==
               oamInfo.excessKeepalivePeriodThreshold)
            {
                oamInfo.excessKeepalivePeriodCounter = 0;
                oamInfo.excessKeepaliveMessageCounter = 0;
            }
            else
            {
                oamInfo.excessKeepalivePeriodCounter++;
            }
        }

        if(updateKeepAliveEntry == GT_TRUE)
        {
            regAddr = SMEM_LION2_OAM_AGING_TBL_MEM(devObjPtr, 0, oamInfo.entryIndex / 32);
            agingBitmapPtr = smemMemGet(devObjPtr, regAddr);

            /* Set OAM aging status bit */
            SMEM_U32_SET_FIELD(agingBitmapPtr[0], oamInfo.entryIndex % 32, 1,
                               (agedOutEntry == GT_TRUE) ? 1 : 0);
        }

        /* for performance ... do not update entry , if nothing changed . */
        if(updateOamEntry == GT_TRUE)
        {
            /* Update current OAM entry */
            snetLion2OamInfoSet(devObjPtr, stage, &oamInfo);
        }
    }

    SCIB_SEM_SIGNAL;
}

/*******************************************************************************
*   snetLion2OamKeepAliveAgingRequestBitSample
*
* DESCRIPTION:
*       Sample aging request bit for period counter
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       stage           - OAM stage
*       counterIndex    - period counter index
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID snetLion2OamKeepAliveAgingRequestBitSample
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SNET_LION2_OAM_STAGE_TYPE_ENT stage,
    IN GT_U32 counterIndex
)
{
    GT_U32  regAddr;                            /* Register address */
    GT_U32 * regPtr;                            /* Register pointer */
    GT_U32  clocks;                             /* clocks */
    GT_U64  currentTimeSecNanosec;              /* current time in seconds plus nanoseconds */
    GT_U64  diffTimeCurrentTimeSecNanosec;      /* difference between current and previous time */
    GT_U64  periodCounterHigh;                  /* period counter high word  */
    GT_U64  periodCounterLow;                   /* period counter  low word  */
    GT_U64  decrementCounterValue;              /* Decrement counter value */
    GT_U64 * agingPeriodCounterTimerPtr;        /* Pointer to current aging period counter timer */
    GT_U64 * counterValuePtr;                   /* Pointer to current aging period counter value */

    DECLARE_FUNC_NAME(snetLion2OamKeepAliveAgingRequestBitSample);

    /* Get current clock */
    clocks = SIM_OS_MAC(simOsTickGet)();

    /* Convert tick clocks to seconds and nanoseconds */
    SNET_TOD_CLOCK_FORMAT_MAC(clocks,
                              currentTimeSecNanosec.l[0],
                              currentTimeSecNanosec.l[1]);

    /* Get current aging period counter timer */
    agingPeriodCounterTimerPtr =
        &devObjPtr->oamSupport.keepAlivePeriodCounter[stage].keepAlivePeriodCounterTimer[counterIndex];

    /* Get current aging period counter value */
    counterValuePtr =
        &devObjPtr->oamSupport.keepAlivePeriodCounter[stage].keepAlivePeriodCounterValue[counterIndex];

    /* Aging Period Table */
    regAddr =
        SMEM_LION2_OAM_AGING_PERIOD_ENTRY_LOW_REG(devObjPtr, stage, counterIndex);
    regPtr = smemMemGet(devObjPtr, regAddr);

    if(agingPeriodCounterTimerPtr->l[0] == 0 && agingPeriodCounterTimerPtr->l[1] == 0)
    {
        /* Init period counter timer */
        agingPeriodCounterTimerPtr->l[0] = currentTimeSecNanosec.l[0];
        agingPeriodCounterTimerPtr->l[1] = currentTimeSecNanosec.l[1];
        /* Reload value from the Aging Period Table */
        counterValuePtr->l[0] = *regPtr;
        counterValuePtr->l[1] = SMEM_U32_GET_FIELD(regPtr[1], 0, 1);
        __LOG(("Init period counter [%d] timer [0%8.8x] and counter value [0%8.8x] \n",
               counterIndex, agingPeriodCounterTimerPtr->l[0], counterValuePtr->l[0]));
        /* Return until next clock cycle */
        return;
    }

    if(counterValuePtr->l[0] == 0 && counterValuePtr->l[1] == 0)
    {
        if (regPtr[0] != 0 || regPtr[1] != 0)
        {
            /* Reload value from the Aging Period Table */
            counterValuePtr->l[0] = *regPtr;
            counterValuePtr->l[1] = SMEM_U32_GET_FIELD(regPtr[1], 0, 1);
            __LOG(("Reload counter value [0%8.8x] \n", counterValuePtr->l[0]));
        }
        else
        {
            /* Return until next clock cycle */
            return;
        }
    }

    /* Get difference between current and previous time  */
    diffTimeCurrentTimeSecNanosec =
        prvSimMathSub64(currentTimeSecNanosec, *agingPeriodCounterTimerPtr);

    /* Store current time */
    agingPeriodCounterTimerPtr->l[0] = currentTimeSecNanosec.l[0];
    agingPeriodCounterTimerPtr->l[1] = currentTimeSecNanosec.l[1];

    /* Convert nanoseconds in granularity of 40 ns to period counter high word decrement */
    CNV_U32_TO_U64(((diffTimeCurrentTimeSecNanosec.l[0] * 1000000000) / 40),
                   periodCounterHigh);
    /* Convert nanoseconds in granularity of 40 ns to period counter low word decrement */
    CNV_U32_TO_U64((diffTimeCurrentTimeSecNanosec.l[1] / 40),
                   periodCounterLow);

    /* Calculate decrement value for period counter */
    decrementCounterValue = prvSimMathAdd64(periodCounterHigh, periodCounterLow);

    if (COMPARE_TWO_U64_VALUES_MAC(*counterValuePtr, decrementCounterValue) > 0)
    {
        /* Decrement aging period counter */
        *counterValuePtr =
            prvSimMathSub64(*counterValuePtr, decrementCounterValue);
        __LOG(("Decremented aging period counter [0%8.8x] \n", *counterValuePtr));
    }
    else
    {
        counterValuePtr->l[0] = counterValuePtr->l[1] = 0;
        devObjPtr->oamSupport.
            keepAlivePeriodCounter[stage].keepAliveAgingRequestBit[counterIndex] = 1;
        __LOG(("Keepalive aging request bit for period counter [%d] is 1 \n", counterIndex));
    }
}

/*******************************************************************************
*   snetLion2OamKeepAliveAging
*
* DESCRIPTION:
*       Age out OAM table entries.
*
* INPUTS:
*       devObjPtr - pointer to device object.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
GT_VOID snetLion2OamKeepAliveAging
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{

    GT_U32  regAddr;                    /* Register address */
    GT_U32 * regPtr;                    /* Register pointer */
    GT_U32  fldValue;                   /* Register field value */
    GT_U32 entryIndex;                  /* OAM aging period index */
    SBUF_BUF_ID bufferId;               /* Buffer */
    GT_U8 * dataPtr;                    /* Pointer to the data in the buffer */
    GT_U32 dataSize;                    /* Data size */
    GENERIC_MSG_FUNC oamAgingProc;      /* OAM aging process function*/

    /* Sleep to finish init of the simulation */
    devObjPtr = skernelSleep(devObjPtr,5000);

    oamAgingProc = snetLion2OamAgeProcess;
    simLogPacketFrameUnitSet(SIM_LOG_FRAME_UNIT___ALLOW_ALL_UNITS___E);/* wild card for LOG without the unit filter */
    simLogPacketFrameCommandSet(SIM_LOG_FRAME_COMMAND_TYPE_GENERAL_E);

    while(1)
    {
        /* support possible change of the device during 'soft reset' */
        SIM_OS_MAC(simOsTaskOwnTaskPurposeSet)(
            SIM_OS_TASK_PURPOSE_TYPE_PP_PIPE_OAM_KEEP_ALIVE_DAEMON_E,
            &devObjPtr->task_oamKeepAliveAgingCookieInfo.generic);

        regAddr = SMEM_LION2_OAM_GLOBAL_CONTROL_REG(devObjPtr, 0);
        regPtr = smemMemGet(devObjPtr, regAddr);
        /* Enable Aging Daemon */
        fldValue = SMEM_U32_GET_FIELD(regPtr[0], 1, 1);
        if(fldValue == 0)
        {
            /* Daemon is not active -- try again later ... */
            devObjPtr = skernelSleep(devObjPtr,1000);
            continue;
        }

        /* Sample aging request bitmap */
        for(entryIndex = 0; entryIndex < 8; entryIndex++)
        {
            snetLion2OamKeepAliveAgingRequestBitSample(devObjPtr, 0, entryIndex);

            /* Check if Aging request is triggered */
            if(devObjPtr->oamSupport.
               keepAlivePeriodCounter[0].keepAliveAgingRequestBit[entryIndex])
            {
                /* Get buffer */
                bufferId = sbufAlloc(devObjPtr->bufPool,
                                     OAM_KEEPALIVE_AGING_MESSAGE_SIZE);
                if (bufferId == NULL)
                {
                    simWarningPrintf("snetLion2OamAging: no buffers for OAM keepalive aging aging \n");
                    devObjPtr = skernelSleep(devObjPtr,1000);   /* currently no buffers for OAM keepalive aging
                                           sleep for 2 seconds and then try again ,
                                           maybe there will be buffers free then */
                    continue;
                }

               /* Get actual data pointer */
                sbufDataGet(bufferId, (GT_U8**)&dataPtr, &dataSize);

                /* Put the name of the function into the message */
                memcpy(dataPtr, &oamAgingProc, OAM_KEEPALIVE_AGING_MESSAGE_GENERIC_MSG_FUNC_SIZE);
                dataPtr+=OAM_KEEPALIVE_AGING_MESSAGE_GENERIC_MSG_FUNC_SIZE;

                /* Copy OAM aging period index to buffer */
                memcpy(dataPtr, &entryIndex, OAM_KEEPALIVE_AGING_MESSAGE_PERIOD_INDEX_SIZE);
                dataPtr+=OAM_KEEPALIVE_AGING_MESSAGE_PERIOD_INDEX_SIZE;

                /* Set source type of buffer */
                bufferId->srcType = SMAIN_SRC_TYPE_CPU_E;

                /* Set message type of buffer */
                bufferId->dataType = SMAIN_MSG_TYPE_GENERIC_FUNCTION_E;

                /* Put buffer to queue */
                squeBufPut(devObjPtr->queueId, SIM_CAST_BUFF(bufferId));
            }

            /* Clear Aging Request Bit */
            devObjPtr->oamSupport.
                keepAlivePeriodCounter[0].keepAliveAgingRequestBit[entryIndex] = 0;
        }

        devObjPtr = skernelSleep(devObjPtr,OAM_DAEMON_DELAY_MILISECONDS_E);
    }
}

/*******************************************************************************
*   snetLion2OamPsuPacketModify
*
* DESCRIPTION:
*       Packet Stamping Unit logic
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - pointer to the frame's descriptor.
*
* OUTPUT:
*       None
*
* COMMENTS:
*       It is safe to assume that the application must prevent a situation
*       where one packet is triggered for both timestamping and loss measurement
*       counter capture.
*
*******************************************************************************/
GT_VOID snetLion2OamPsuPacketModify
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetLion2OamPsuPacketModify);

    GT_U32 regAddr;                         /* Register address */
    GT_U32 * regPtr;                        /* Register pointer */
    GT_U32 byteIndex;                       /* Byte index */
    GT_U8  byteVal;

    if(devObjPtr->oamSupport.psuSupportPacketModification == 0)
    {
        /* PSU packet modification not supported */
        __LOG(("PSU packet modification not supported"));
        return;
    }

    if(descrPtr->oamInfo.lmCounterInsertEnable == 0)
    {
        __LOG(("LM counter insert disabled"));
        return;
    }

    regAddr = SMEM_LION2_OAM_LM_OFFSET_TBL_MEM(devObjPtr,
                                               descrPtr->oamInfo.offsetIndex);
    regPtr = smemMemGet(devObjPtr, regAddr);
    /* Offset in bytes from the beginning of the layer 3 header, where
    the LM counter should be inserted into the packet */
    byteIndex = SMEM_U32_GET_FIELD(regPtr[0], 0, 8);

    byteVal = SMEM_U32_GET_FIELD(descrPtr->oamInfo.lmCounter, 24, 8);
    descrPtr->haToEpclInfo.l3StartOffsetPtr[byteIndex++] = byteVal;
    __LOG(("LM counter: l3StartOffsetPtr[%d] = [0x%2.2x]", byteIndex, byteVal));

    byteVal = SMEM_U32_GET_FIELD(descrPtr->oamInfo.lmCounter, 16, 8);
    descrPtr->haToEpclInfo.l3StartOffsetPtr[byteIndex++] = byteVal;
    __LOG(("LM counter: l3StartOffsetPtr[%d] = [0x%2.2x]", byteIndex, byteVal));

    byteVal = SMEM_U32_GET_FIELD(descrPtr->oamInfo.lmCounter, 8, 8);
    descrPtr->haToEpclInfo.l3StartOffsetPtr[byteIndex++] = byteVal;
    __LOG(("LM counter: l3StartOffsetPtr[%d] = [0x%2.2x]", byteIndex, byteVal));

    byteVal = SMEM_U32_GET_FIELD(descrPtr->oamInfo.lmCounter, 0, 8);
    descrPtr->haToEpclInfo.l3StartOffsetPtr[byteIndex++] = byteVal;
    __LOG(("LM counter: l3StartOffsetPtr[%d] = [0x%2.2x]", byteIndex, byteVal));
}

/*******************************************************************************
*   snetLion2PclOamDescriptorSet
*
* DESCRIPTION:
*       Set OAM relevant fields in descriptor
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       descrPtr            - pointer to the frame descriptor.
*       oamInfoPtr          - pointer to OAM info from the IPCL/EPCL action
* OUTPUT:
*       descrPtr    - pointer to the frame descriptor.
*
*
*******************************************************************************/
GT_VOID snetLion2PclOamDescriptorSet
(
    IN SKERNEL_DEVICE_OBJECT                 * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC    * descrPtr,
    IN SMAIN_DIRECTION_ENT                   direction,
    IN SNET_LION3_PCL_ACTION_OAM_STC         * oamInfoPtr
)
{
    DECLARE_FUNC_NAME(snetLion2PclOamDescriptorSet);

    GT_U32 regAddr;                             /* Register address */
    GT_U32 *regPtr;                             /* Register data pointer */
    GT_U32 fieldVal;                            /* Register field value */
    GT_U32 fieldOffset;                         /* Register field offset */
    GT_U32 fieldMask;                           /* Register field value mask */
    GT_U8 byteValue;
    GT_U32 udbIndex;
    GT_U32 oamProfile;                          /* OAM profile index */
    GT_U32 expOrQosProfile;
    GT_U32 channelTypeToOpcodeMappingEn;
    GT_U32  mappedOpcode;
    GT_U32 rBit;
    GT_STATUS rc;

    if(devObjPtr->oamSupport.oamSupport == 0)
    {
        /* Device doesn't support OAM */
        return;
    }

    if(direction == SMAIN_DIRECTION_EGRESS_E)
    {
        /* Clear OAM info structure for the egress pipe */
        memset(&descrPtr->oamInfo, 0, sizeof(SKERNEL_OAM_INFO_STC));
    }

    __LOG_PARAM(oamInfoPtr->oamProcessEnable);

    if(oamInfoPtr->oamProcessEnable == 0)
    {
        __LOG(("In current action from [%s] the OAM processing is not enabled \n",
            (direction == SMAIN_DIRECTION_INGRESS_E) ? "TTI/IPCL" : "EPCL"));
        return;
    }

    __LOG_PARAM(oamInfoPtr->oamProfile);
    __LOG_PARAM(oamInfoPtr->ptpTimeStampEnable);
    __LOG_PARAM(oamInfoPtr->ptpTimeStampOffsetIndex);
    __LOG_PARAM(oamInfoPtr->channelTypeToOpcodeMappingEn);


    if(direction == SMAIN_DIRECTION_INGRESS_E)
    {
        __LOG(("OAM from ingress pipe (ipcl/tti) is enabled\n"));
        expOrQosProfile = descrPtr->qos.qosProfile;
        channelTypeToOpcodeMappingEn = descrPtr->channelTypeToOpcodeMappingEn;
    }
    else /* SMAIN_DIRECTION_EGRESS_E */
    {
        __LOG(("OAM from egress pipe (epcl) is enabled \n"));
        expOrQosProfile = descrPtr->epclAction.exp;
        channelTypeToOpcodeMappingEn = oamInfoPtr->channelTypeToOpcodeMappingEn;

        /* EPCL OAM Packet Detection Configuration */
        regAddr = SMEM_LION2_EPCL_OAM_PACKET_DETECTION_REG(devObjPtr);
        regPtr = smemMemGet(devObjPtr, regAddr);
        /* OAM Packet Detection Mode */
        fieldVal = SMEM_U32_GET_FIELD(*regPtr, 0, 1);
        if(fieldVal)
        {
            /* SrcID that is used to detect OAM message from the CPU. */
            fieldVal = SMEM_U32_GET_FIELD(*regPtr, 1, 12);
            if(descrPtr->sstId != fieldVal)
            {
                /* OAM packets are detected based on EPCL rule and NOT a dedicated SrcID value */
                __LOG(("OAM packets are detected based on EPCL rule and NOT a dedicated SrcID value"));
                return;
            }
        }
    }

    if(direction == SMAIN_DIRECTION_INGRESS_E)
    {
        /* Ingress OAM processing enable */
        descrPtr->oamInfo.oamProcessEnable = 1;
    }
    else
    {
        /* Egress OAM processing enable */
        descrPtr->oamInfo.oamEgressProcessEnable = 1;
    }

    if(oamInfoPtr->ptpTimeStampEnable)
    {
        /* Override relevant timestamp fields from previous engine */
        descrPtr->oamInfo.timeStampEnable = 1;
        descrPtr->oamInfo.offsetIndex = oamInfoPtr->ptpTimeStampOffsetIndex;
    }

    oamProfile = descrPtr->oamInfo.oamProfile = oamInfoPtr->oamProfile;

    if( 0 == channelTypeToOpcodeMappingEn )
    {
        udbIndex = 2*SNET_LION_OAM_PROFILE_OPCODE_E + oamProfile;

        /* UDB% of the current lookup */
        __LOG(("UDB[%d] of the current lookup",udbIndex));
        rc = snetLionPclUdbKeyValueGet(devObjPtr, descrPtr, direction,
                                       udbIndex, &byteValue);
        if(rc != GT_OK)
        {
            skernelFatalError("snetLion2PclOamDescriptorSet: fail getting UDB[%d].\n", udbIndex);
        }

        descrPtr->oamInfo.opCode = byteValue;
    }
    else /* 1 == channelTypeToOpcodeMappingEn */
    {
        if( 0 == descrPtr->channelTypeProfile )
        {
            /* <channelTypeProfile> == 0 => mappedOpcode = 0 */
            __LOG(("<channelTypeProfile> == 0 => mappedOpcode = 0"));
            mappedOpcode = 0;
        }
        else
        {
            /* OAM Opcode */
            if(SMAIN_DIRECTION_INGRESS_E == direction)
            {
                regAddr = SMEM_LION2_PCL_OAM_CHANNEL_TYPE_OPCODE_REG(devObjPtr,(descrPtr->channelTypeProfile));
                fieldOffset = 0;
            }
            else /* SMAIN_DIRECTION_EGRESS_E == direction */
            {
                regAddr = SMEM_LION2_EPCL_OAM_CHANNEL_TYPE_OPCODE_REG(devObjPtr,((descrPtr->channelTypeProfile-1)/3));
                fieldOffset = ((descrPtr->channelTypeProfile-1)%3)*9;
            }

            regPtr = smemMemGet(devObjPtr, regAddr);
            mappedOpcode = SMEM_U32_GET_FIELD(*regPtr, fieldOffset, 8);
            __LOG(("mappedOpcode = %d", mappedOpcode));

            /* R Bit Assignment Enable */
            if(SMAIN_DIRECTION_INGRESS_E == direction)
            {
                regAddr = SMEM_LION2_PCL_OAM_R_BIT_ASSIGN_EN_REG(devObjPtr);
                fieldOffset = descrPtr->channelTypeProfile;
            }
            else /* SMAIN_DIRECTION_EGRESS_E == direction */
            {
                fieldOffset = ((descrPtr->channelTypeProfile-1)/3)*9 + 8;
            }

            regPtr = smemMemGet(devObjPtr, regAddr);
            fieldVal = SMEM_U32_GET_FIELD(*regPtr, fieldOffset, 1);

            if( fieldVal )
            {
                /* R bit assignment enable */
                udbIndex = 2*SNET_LION_OAM_PROFILE_R_BIT_E + oamProfile;

                /* UDB% of the current lookup */
                __LOG(("UDB[%d] of the current lookup",udbIndex));
                rc = snetLionPclUdbKeyValueGet(devObjPtr, descrPtr, direction,
                                               udbIndex, &byteValue);
                if(rc != GT_OK)
                {
                    skernelFatalError("snetLion2PclOamDescriptorSet: fail getting UDB[%d].\n", udbIndex);
                }

                /* The R bit */
                if(SMAIN_DIRECTION_INGRESS_E == direction)
                {
                    regAddr = SMEM_LION2_PCL_OAM_R_FLAG_CONFIGURATION_REG(devObjPtr);
                }
                else /* SMAIN_DIRECTION_EGRESS_E == direction */
                {
                    if( devObjPtr->errata.wrongEpclOamRBitReg )
                    {
                        regAddr = SMEM_LION2_EPCL_OAM_RDI_CONFIGURATION_REG(devObjPtr);
                    }
                    else
                    {
                        skernelFatalError("snetLion2PclOamDescriptorSet: R Bit configuration register not defined.\n");
                    }

                }

                fieldOffset = oamProfile * 16;

                regPtr = smemMemGet(devObjPtr, regAddr);
                fieldMask = SMEM_U32_GET_FIELD(*regPtr, fieldOffset, 8);
                fieldVal = SMEM_U32_GET_FIELD(*regPtr, fieldOffset + 8, 8);

                /* The R bit */
                rBit = ((byteValue & fieldMask) == fieldVal) ? 0 : 1;

                mappedOpcode = (mappedOpcode & 0xFE) + rBit;
                __LOG(("R bit assignment enable, mappedOpcode = %d, RBit = %d", mappedOpcode,rBit));
            }
        }

        descrPtr->oamInfo.opCode = mappedOpcode;
    }

    /* OAM MEG Level Mode Configuration  */
    udbIndex = 2*SNET_LION_OAM_PROFILE_MEG_LEVEL_E + oamProfile;

    if(SMAIN_DIRECTION_INGRESS_E == direction)
    {
        regAddr = SMEM_LION2_PCL_OAM_MEG_LEVEL_MODE_CONFIGURATION_REG(devObjPtr);
    }
    else /* SMAIN_DIRECTION_EGRESS_E == direction */
    {
        regAddr = SMEM_LION2_EPCL_OAM_MEG_LEVEL_MODE_CONFIGURATION_REG(devObjPtr);
    }

    regPtr = smemMemGet(devObjPtr, regAddr);
    fieldOffset = oamProfile * 2;
    fieldVal = SMEM_U32_GET_FIELD(*regPtr, fieldOffset, 2);
    switch(fieldVal)
    {
        case 0:
            /* MEG Level is taken from bits [7:5] of UDB% of the current lookup */
            rc = snetLionPclUdbKeyValueGet(devObjPtr, descrPtr, direction,
                                           udbIndex, &byteValue);
            if(rc != GT_OK)
            {
                skernelFatalError("snetLion2IPclOamDescriptorSet: fail getting UDB[%d].\n", udbIndex);
            }
            descrPtr->oamInfo.megLevel = byteValue >> 5;
            break;
        case 1:
            /* MEG Level is taken from bits [2:0] of the MAC DA */
            descrPtr->oamInfo.megLevel = descrPtr->macDaPtr[5] & 0x7;
            break;
        case 2:
            /* MEG Level is taken from bits [2:0] of the EXP or QoS Profile */
            descrPtr->oamInfo.megLevel = expOrQosProfile & 0x7;
            break;
        default:
            skernelFatalError("snetLion2PclOamDescriptorSet: unknown MEG Level Mode!\n");
    }

    udbIndex = 2*SNET_LION_OAM_PROFILE_TX_PERIOD_E + oamProfile;

    /* UDB% of the current lookup */
    rc = snetLionPclUdbKeyValueGet(devObjPtr, descrPtr, direction,
                                   udbIndex, &byteValue);
    if(rc != GT_OK)
    {
        skernelFatalError("snetLion2IPclOamDescriptorSet: fail getting UDB[%d].\n", udbIndex);
    }

    /* 3 lsbits */
    descrPtr->oamInfo.oamTxPeriod = byteValue & 0x7;

    udbIndex = 2*SNET_LION_OAM_PROFILE_RDI_E + oamProfile;

    /* UDB% of the current lookup */
    rc = snetLionPclUdbKeyValueGet(devObjPtr, descrPtr, direction,
                                   udbIndex, &byteValue);

    if(rc != GT_OK)
    {
        skernelFatalError("snetLion2PclOamDescriptorSet: fail getting UDB[%d].\n", udbIndex);
    }
    /* OAM RDI Configuration  */
    fieldOffset = oamProfile * 16;
    if(direction == SMAIN_DIRECTION_INGRESS_E)
    {
        regAddr = SMEM_LION2_PCL_OAM_RDI_CONFIGURATION_REG(devObjPtr);
    }
    else
    {
        regAddr = SMEM_LION2_EPCL_OAM_RDI_CONFIGURATION_REG(devObjPtr);
    }

    regPtr = smemMemGet(devObjPtr, regAddr);
    fieldMask = SMEM_U32_GET_FIELD(*regPtr, fieldOffset, 8);
    fieldVal = SMEM_U32_GET_FIELD(*regPtr, fieldOffset + 8, 8);
    /* The RDI (Remote Defect Indication) bit */
    descrPtr->oamInfo.oamRdi = ((byteValue & fieldMask) == fieldVal) ? 0 : 1;
}

