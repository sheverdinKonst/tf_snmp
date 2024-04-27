/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetCheetah3Policer.c
*
* DESCRIPTION:
*       (Cheetah) Policing Engine processing for frame -- simulation
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 29 $
*
*******************************************************************************/

#include <asicSimulation/SKernel/smem/smem.h>
#include <asicSimulation/SKernel/smem/smemCheetah.h>
#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SKernel/suserframes/snet.h>
#include <asicSimulation/SKernel/cheetahCommon/sregCheetah.h>
#include <asicSimulation/SKernel/suserframes/snetCheetah3Policer.h>
#include <asicSimulation/SKernel/suserframes/snetXCatPolicer.h>
#include <asicSimulation/SKernel/suserframes/snetLion2Oam.h>
#include <asicSimulation/SLog/simLog.h>

/* Private declarations */
static GT_BOOL snetCht3PolicerTriggeringCheck(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    OUT GT_U32 ** policerMeterEntryPtr
);

static GT_VOID snetCht3PolicerConformanceLevelGet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_LION3_POLICER_METERING_DATA_STC *meteringDataPtr,
    OUT SNET_CHT3_POLICER_QOS_INFO_STC * qosProfileInfoPtr
);

static GT_VOID snetCht3PolicerCountersUpdate(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_U32 * policerMeterEntryPtr,
    IN SKERNEL_CONFORMANCE_LEVEL_ENT cl,
    OUT SNET_CHT3_POLICER_MNG_COUNT_STC * mngCountDataPtr
);

static GT_VOID snetCht3PolicerQosRemark(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_U32 * policerMeterEntryPtr,
    IN SNET_CHT3_POLICER_QOS_INFO_STC * qosProfileInfoPtr,
    IN SNET_CHT3_POLICER_MNG_COUNT_STC * mngCountDataPtr
);

static GT_VOID snetCht3PolicerHQosTableApply
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
);

#define SMEM_CHT_POLICER_CNT_TBL_UPDATE(dev, address, dp, size) \
{ \
    (address) = ((dp) == SKERNEL_CONFORM_GREEN) ? (address) : \
                ((dp) == SKERNEL_CONFORM_YELLOW) ? (address) + 0x4 : \
                ((dp) == SKERNEL_CONFORM_RED) ? (address) + 0x8 : 0; \
    SMEM_CHT3_INCREMENT_COUNTER(dev, address, dp, size); \
}

#define SMEM_CHT3_INCREMENT_COUNTER(dev, address, dp, increment) \
{ \
    GT_U32 val; \
    smemRegGet((dev), (address), &val); \
    val += increment; \
    smemRegSet((dev), (address), val); \
}

#define SMEM_CHT_POLICER_MNG_CNT_TBL_UPDATE(dev, cycle, counter, dp, size) \
{ \
    GT_U32 address; \
    if(SKERNEL_IS_XCAT_REVISON_A1_DEV(dev) == 0) \
    { \
        address = ((dp) == SKERNEL_CONFORM_GREEN) ? \
        SMEM_CHT3_IPLR_MNG_CNT_SET_GREEN_TBL_MEM(dev,counter) : \
              ((dp) == SKERNEL_CONFORM_YELLOW) ? \
        SMEM_CHT3_IPLR_MNG_CNT_SET_YELLOW_TBL_MEM(dev,counter) : \
              ((dp) == SKERNEL_CONFORM_RED) ? \
        SMEM_CHT3_IPLR_MNG_CNT_SET_RED_TBL_MEM(dev,counter) : 0; \
        SMEM_CHT3_INCREMENT_COUNTER(dev, address, dp, size); \
        SMEM_CHT3_INCREMENT_COUNTER(dev, address + 0x4, dp, 1); \
    } \
    else \
    { \
        address = SMEM_CHT3_POLICER_MNG_CNT_TBL_MEM(dev, cycle, counter); \
        snetXCatPolicerMngCounterIncrement(dev, address, dp, size); \
    } \
}

#define SNET_CHT3_POLICER_COLOR_MODE_GET_MAC(dev, entry) \
    (SKERNEL_IS_XCAT_REVISON_A1_DEV(dev)) ? \
        SMEM_U32_GET_FIELD(entry[6], 2, 1) : \
        SMEM_U32_GET_FIELD(entry[4], 1, 1)

/* Management counter set in which packets and DUs will be counted */
#define SNET_CHT3_POLICER_MNG_COUNT_GET_MAC(dev, entry) \
    (SKERNEL_IS_XCAT_REVISON_A1_DEV(dev)) ? \
        SMEM_U32_GET_FIELD(entry[6], 4, 2) : \
        SMEM_U32_GET_FIELD(entry[5], 30, 2)

/* Yellow Policer Command */
#define SNET_CHT3_POLICER_YELLOW_CMD_GET_MAC(dev, entry) \
    (SKERNEL_IS_XCAT_REVISON_A1_DEV(dev)) ? \
        SMEM_U32_GET_FIELD(entry[6], 24, 2) : \
        SMEM_U32_GET_FIELD(entry[3], 28, 2)

/* Red Policer Command */
#define SNET_CHT3_POLICER_RED_CMD_GET_MAC(dev, entry) \
    (SKERNEL_IS_XCAT_REVISON_A1_DEV(dev)) ? \
        SMEM_U32_GET_FIELD(entry[6], 22, 2) : \
        SMEM_U32_GET_FIELD(entry[3], 26, 2)

/* Policer Modify DSCP */
#define SNET_CHT3_POLICER_MODIFY_DSCP_GET_MAC(dev, entry) \
    (SKERNEL_IS_XCAT_REVISON_A1_DEV(dev)) ? \
        SMEM_U32_GET_FIELD(entry[6], 26, 2) : \
        SMEM_U32_GET_FIELD(entry[5], 26, 2)

/* Policer Modify UP */
#define SNET_CHT3_POLICER_MODIFY_UP_GET_MAC(dev, entry) \
    (SKERNEL_IS_XCAT_REVISON_A1_DEV(dev)) ? \
        SMEM_U32_GET_FIELD(entry[6], 28, 2) : \
        SMEM_U32_GET_FIELD(entry[5], 28, 2)


/* Policer QoS Profile */
#define SNET_CHT3_POLICER_QOS_PROFILE_GET_MAC(dev, entry) \
        snetFieldValueGet(entry,222,7)

/* Policer engines number */
#define     SNET_CHT3_POLICER_NUM_MAC(dev)  ((dev)->policerEngineNum)

/*******************************************************************************
*   snetCht3Policer
*
* DESCRIPTION:
*        Policer Processing, Policer Counters updates
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - frame data buffer Id
*
* RETURN:
*
*******************************************************************************/
GT_VOID snetCht3Policer
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetCht3Policer);

    GT_U32 * policerMeterEntryPtr;   /* Policers Metering Entry Pointer */
    SNET_CHT3_POLICER_QOS_INFO_STC qosProfileInfo;
                                        /* Initial conformance level,
                                           qos profile (index) for out-of-profile
                                           traffic */
    OUT SNET_CHT3_POLICER_MNG_COUNT_STC mngCountData;
                                /* Management counters data for dropped  packets */
    GT_BOOL retVal;
    SNET_LION3_POLICER_METERING_DATA_STC meteringData;

    if(descrPtr->bypassRouterAndPolicer)
    {
        return;
    }
    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        /* Ingress OAM processing */
        __LOG(("Ingress OAM processing \n"));
        snetLion2IOamProcess(devObjPtr, descrPtr);
    }

    for(descrPtr->policerCycle = 0;
         descrPtr->policerCycle < SNET_CHT3_POLICER_NUM_MAC(devObjPtr);
         descrPtr->policerCycle++)
    {
        __LOG(("Start IPLR[%d] \n",
            descrPtr->policerCycle));

        policerMeterEntryPtr = NULL;
        /* Check if policer needed */
        __LOG(("Check if metering needed"));
        retVal = snetCht3PolicerTriggeringCheck(devObjPtr, descrPtr,
                                                &policerMeterEntryPtr);
        if(retVal == GT_FALSE)
        {
            __LOG(("metering not needed \n"));
            policerMeterEntryPtr = NULL;
        }
        else
        {
            __LOG(("metering is needed \n"));
        }

        retVal = snetXCatCommonPolicerMeteringEnvelopeDataGet(
            devObjPtr, descrPtr, policerMeterEntryPtr, &meteringData);
        if (retVal == GT_OK)
        {
            /* for SIP5_15 policerMeterEntryPtr not relevant for any algorithm */
            /* it points buckets entry only                                    */
            /* for metering MEF10.3 envelope fields below should be updated    */
            descrPtr->policerMeterConfigEntryMemoryPtr          =
                meteringData.meterCfgEntryPtr;
            descrPtr->policerConformanceLevelSignEntryMemoryPtr =
                meteringData.meterConformSignLevelPtr;
            descrPtr->policerActuallAccessedIndex =
                (meteringData.envelopeBaseIndex + meteringData.packetRank);
        }
        else
        {
            snetXCatCommonPolicerMeteringSingleDataGet(
                devObjPtr, descrPtr, policerMeterEntryPtr, &meteringData);
        }

        /* Get conformance level and qos profile for out-of-profile traffic */
        __LOG(("Get conformance level and qos profile for out-of-profile traffic"));
        snetCht3PolicerConformanceLevelGet(
            devObjPtr, descrPtr, &meteringData, &qosProfileInfo);
        /* Update policer counters */
        __LOG(("Update policer counters"));
        snetCht3PolicerCountersUpdate(devObjPtr, descrPtr, policerMeterEntryPtr,
                                      qosProfileInfo.cl, &mngCountData);

        /* Remark out-of-profile traffic */
        __LOG(("Remark out-of-profile traffic"));
        snetCht3PolicerQosRemark(devObjPtr, descrPtr, policerMeterEntryPtr,
                                 &qosProfileInfo, &mngCountData);

        /* Hierarchical Ingress Policing */
        __LOG(("Hierarchical Ingress Policing"));
        snetCht3PolicerHQosTableApply(devObjPtr, descrPtr);

        /*reset the value before next cycle */
        descrPtr->policerCounterGlobalNumBytes = 0;

        __LOG(("Ended IPLR[%d] \n",
            descrPtr->policerCycle));
    }
}

/*******************************************************************************
*   snetCht3PolicerTriggeringCheck
*
* DESCRIPTION:
*        check if need to do policer
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - frame data buffer Id
*
* OUTPUT:
*       policerMeterEntryPtr  - policer info entry pointer
*
* RETURN:
*
*******************************************************************************/
static GT_BOOL snetCht3PolicerTriggeringCheck
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    OUT GT_U32 ** policerMeterEntryPtrPtr
)
{
    DECLARE_FUNC_NAME(snetCht3PolicerTriggeringCheck);

    GT_U32 regAddr;                     /* register address */
    GT_U32 fldValue;                    /* register entry field */
    GT_U32 policerIdx = 0;              /* policer entry offset */
    GT_U32 meterBaseAddr;               /* metering table base address */
    GT_U32 meterMemoryCycle;            /* policer stage for meter tables access */
    GT_BOOL skipPhysicalPort = GT_FALSE;

    /* initialize the trigger mode */
    descrPtr->policerTriggerMode = SKERNEL_POLICER_TRIGGER_MODE_NONE_E;
    __LOG_PARAM(descrPtr->policerTriggerMode);

    if (descrPtr->packetCmd != SKERNEL_EXT_PKT_CMD_FORWARD_E &&
        descrPtr->packetCmd != SKERNEL_EXT_PKT_CMD_MIRROR_TO_CPU_E)
    {
        __LOG(("No metering for non FORWARD/MIRROR_TO_CPU \n"));
        return GT_FALSE;
    }

    regAddr = SMEM_CHT_POLICER_GLB_CONF_REG(devObjPtr, descrPtr->policerCycle);
    smemRegFldGet(devObjPtr, regAddr, 0, 1, &fldValue);
    /* Metering Disabled */
    if(fldValue == 0)
    {
        __LOG(("Metering is globally Disabled \n"));
        return GT_FALSE;
    }

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        if(GT_TRUE == snetLion2PolicerEArchIndexGet(devObjPtr, descrPtr,
                                      SMAIN_DIRECTION_INGRESS_E, 0 , &policerIdx))
        {
            /* the eport/evlan/ flow needs metering/counting */
            if (descrPtr->policerTriggerMode == SKERNEL_POLICER_TRIGGER_MODE_NONE_E)
            {
                /* although eport/evlan used , still forced 'no merer' */
                __LOG(("when eArch (eport/evlan metering is used , then the per physical port registers are not used"));
                return GT_FALSE;
            }

            if(descrPtr->policerActuallAccessedIndex == SMAIN_NOT_VALID_CNS)
            {
                /* restore to 0 , so no indication in the LOG */
                descrPtr->policerActuallAccessedIndex = 0;
                __LOG(("Counting only , Metering not needed \n"));

                return GT_FALSE;
            }

            /* from here we need to skip the 'physical port' logic because
              PCL/TTI and ePort/eVlan triggering have precedence over physical
              ports triggering. */
            skipPhysicalPort = GT_TRUE;
        }

        descrPtr->policerActuallAccessedIndex = 0;
    }

    if(skipPhysicalPort == GT_FALSE)
    {
        regAddr = SMEM_CHT3_POLICER_PORT_METER_REG(devObjPtr,
                                                        descrPtr->policerCycle,
                                                        descrPtr->localDevSrcPort);

        smemRegFldGet(devObjPtr, regAddr, descrPtr->localDevSrcPort % 32, 1, &fldValue);
        if (fldValue)
        {
            /* Ingress Policing triggered per Source Port */
            __LOG(("Ingress Policing triggered per Source Port"));
            policerIdx = descrPtr->localDevSrcPort;

            /* state the trigger was by 'port mode' */
            __LOG(("state the trigger was by 'port mode'"));
            descrPtr->policerTriggerMode = SKERNEL_POLICER_TRIGGER_MODE_PORT_E;
            __LOG_PARAM(descrPtr->policerTriggerMode);
        }
        else
        {
            /* PCL trigger */
            if (descrPtr->policerEn == 0)
            {
                __LOG(("flow mode but TTI/PCL not trigger the IPLR \n"));
                return GT_FALSE;
            }

            /* state the trigger was by 'flow mode' */
            __LOG(("state the trigger was by 'flow mode'"));
            descrPtr->policerTriggerMode = SKERNEL_POLICER_TRIGGER_MODE_FLOW_E;
            __LOG_PARAM(descrPtr->policerTriggerMode);
        }


        if(SKERNEL_IS_XCAT_REVISON_A1_DEV(devObjPtr) == 0)
        {
            /* PCL trigger can override Per port triggering */
            __LOG(("PCL trigger can override Per port triggering"));
            if (descrPtr->policerEn)
            {
                descrPtr->policerTriggerMode = SKERNEL_POLICER_TRIGGER_MODE_FLOW_E;
                __LOG_PARAM(descrPtr->policerTriggerMode);
                policerIdx = descrPtr->policerPtr;
            }
        }
        else
        {
            snetXCatPolicerOverrideMeterIndex(devObjPtr, descrPtr,
                                              SMAIN_DIRECTION_INGRESS_E,
                                              &policerIdx);

            if (descrPtr->policerTriggerMode == SKERNEL_POLICER_TRIGGER_MODE_NONE_E)
            {
                return GT_FALSE;
            }
        }
    }

    descrPtr->policerActuallAccessedIndex = policerIdx;

    __LOG_PARAM(descrPtr->policerActuallAccessedIndex);

    if(SMEM_CHT_IS_SIP5_15_GET(devObjPtr))
    {
        /*  Meter memories became shared between IPLR0 and IPLR1 starting from SIP 5.15 (BobK).
            Direct access done through IPLR0. And base address is used to get absolute address in memory.  */

        /* read metering base address */
        smemRegFldGet(devObjPtr,
                      SMEM_LION3_POLICER_METERING_BASE_ADDR_REG(devObjPtr, descrPtr->policerCycle),
                      0, 24, &meterBaseAddr);

         /* use IPLR0 unit for meter memories access for IPLR1 processing */
         meterMemoryCycle = (descrPtr->policerCycle == 1) ? 0 : descrPtr->policerCycle;
         policerIdx += meterBaseAddr;
         __LOG(("Use metering memory of IPLR[%d] index %d\n", meterMemoryCycle, policerIdx));
    }
    else
    {
        meterMemoryCycle = descrPtr->policerCycle;
    }

    regAddr = SMEM_CHT_POLICER_TBL_MEM(devObjPtr, meterMemoryCycle,
                                        policerIdx);

    /* Get Policer Table entry to buffer */
    *policerMeterEntryPtrPtr = smemMemGet(devObjPtr, regAddr);

    if(SMEM_CHT_IS_SIP5_15_GET(devObjPtr))
    {
        regAddr = SMEM_SIP5_15_POLICER_CONFIG_ENTRY_TBL_MEM(devObjPtr, meterMemoryCycle, policerIdx);
        /* Get Policer config Table entry to buffer */
        descrPtr->policerMeterConfigEntryMemoryPtr = smemMemGet(devObjPtr, regAddr);

        /* each PLR unit has it's own instance of the sign table */
        regAddr = SMEM_SIP5_15_POLICER_METERING_CONFORMANCE_LEVEL_SIGN_TBL_MEM(devObjPtr, descrPtr->policerCycle, policerIdx);
        /* Get Policer Metering Conformance Level Sign Table entry to buffer */
        descrPtr->policerConformanceLevelSignEntryMemoryPtr = smemMemGet(devObjPtr, regAddr);
    }

    return GT_TRUE;
}

/*******************************************************************************
*   snetCht3PolicerConformanceLevelGet
*
* DESCRIPTION:
*        Get conformance level and QoSProfile for out-of-profile traffic
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       descrPtr            - frame data buffer Id
*       meteringDataPtr     - related policer entries info pointer
*
* OUTPUT:
*       qosProfileInfoPtr   - pointer to qos profile info
*                             for out-of-profile traffic
*
* RETURN:
*
*******************************************************************************/
static GT_VOID snetCht3PolicerConformanceLevelGet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_LION3_POLICER_METERING_DATA_STC *meteringDataPtr,
    OUT SNET_CHT3_POLICER_QOS_INFO_STC * qosProfileInfoPtr
)
{
    DECLARE_FUNC_NAME(snetCht3PolicerConformanceLevelGet);

    GT_U32  fldValue;                    /* register's entry field */
    GT_U32  regAddr;                     /* register's address */
    GT_U32  * regPtr;                    /* register's entry pointer */
    GT_BOOL clCalculated;                /* CL Calculated by metering */
    GT_U32 * policerMeterEntryPtr;       /* pointer to metering entry */

    /* for SIP5_15 buckets entry - not used */
    policerMeterEntryPtr = meteringDataPtr->meterGreenBucketEntryPtr;

    /* clear before fulfill */
    memset(qosProfileInfoPtr, 0, sizeof(SNET_CHT3_POLICER_QOS_INFO_STC));

    if(descrPtr->qos.ingressExtendedMode)
    {
        __LOG(("IPLR QoS profile to initial DP table is skipped in extended QoS mode"));
        qosProfileInfoPtr->initialDp = SMEM_U32_GET_FIELD(descrPtr->qos.qosProfile,0,2);
    }
    else
    {
        /* Intial DP */
        regAddr =
            SMEM_CHT3_POLICER_INITIAL_DP_REG(devObjPtr, descrPtr->policerCycle , 0);
        regPtr = smemMemGet(devObjPtr, regAddr);
        /* get the 2 bits that associated with the QOS profile index */
        qosProfileInfoPtr->initialDp = snetFieldValueGet(regPtr, 2 * descrPtr->qos.qosProfile, 2);
    }

    __LOG_PARAM(qosProfileInfoPtr->initialDp);

    /* default value - if no metering */
    clCalculated = GT_FALSE;

    /* Ingress Policer Relative Qos Remarking Memory  */
    regAddr = SMEM_CHT_POLICER_QOS_TBL_MEM(devObjPtr,
                                            descrPtr->policerCycle,
                                            descrPtr->qos.qosProfile);
    regPtr = smemMemGet(devObjPtr, regAddr);

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        /* Red Remark Table QosProfile */
        qosProfileInfoPtr->qosProfRedIndex = SMEM_U32_GET_FIELD(regPtr[0], 0, 10);

        /* Yellow Remark Table QosProfile */
        qosProfileInfoPtr->qosProfYellowIndex = SMEM_U32_GET_FIELD(regPtr[0], 10, 10);

        /* Green Remark Table QosProfile */
        qosProfileInfoPtr->qosProfGreenIndex = SMEM_U32_GET_FIELD(regPtr[0], 20, 10);
    }
    else
    {
        /* Red Remark Table QosProfile[6:0] */
        qosProfileInfoPtr->qosProfRedIndex = SMEM_U32_GET_FIELD(regPtr[0], 0, 7);

        /* Yellow Remark Table QosProfile[13:7] */
        qosProfileInfoPtr->qosProfYellowIndex = SMEM_U32_GET_FIELD(regPtr[0], 7, 7);
    }

    if(policerMeterEntryPtr)
    {
        /* Policer color mode */

        if(SMEM_CHT_IS_SIP5_15_GET(devObjPtr))
        {
            fldValue =
                SMEM_SIP5_15_PLR_METERING_CONFIG_ENTRY_FIELD_GET(devObjPtr,
                    descrPtr->policerMeterConfigEntryMemoryPtr,
                    descrPtr->policerActuallAccessedIndex,
                    SMEM_SIP5_15_PLR_METERING_CONFIG_TABLE_FIELDS_COLOR_MODE_E);
        }
        else
        if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            fldValue =
                SMEM_LION3_PLR_METERING_ENTRY_FIELD_GET(devObjPtr,policerMeterEntryPtr,
                    descrPtr->policerActuallAccessedIndex,
                    SMEM_LION3_PLR_METERING_TABLE_FIELDS_COLOR_MODE);

        }
        else
        {
            fldValue = SNET_CHT3_POLICER_COLOR_MODE_GET_MAC(devObjPtr, policerMeterEntryPtr);
        }

        if (fldValue == 0)
        {
            /* When color blind we are always GREEN */
            __LOG(("Color Mode : BLIND : When color blind we are always GREEN \n"));
            qosProfileInfoPtr->initialDp = SKERNEL_CONFORM_GREEN;
        }
        else
        {
            __LOG(("Color Mode : AWARE \n"));
        }

        if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            /* do simple 'Token bucket' for current packet , without considering
               time/other previous packets */
            snetLion3PolicerMeterSinglePacketTokenBucketApply(devObjPtr, descrPtr,
                                          SMAIN_DIRECTION_INGRESS_E,
                                          meteringDataPtr,
                                          qosProfileInfoPtr);
            /* use 'final value' as 'init value'*/
            qosProfileInfoPtr->initialDp = qosProfileInfoPtr->cl;
            clCalculated = GT_TRUE;
        }
    }

    /* Check simulation debug info if we force out-of-profile traffic */
    if (clCalculated == GT_FALSE &&
        skernelUserDebugInfo.policerConformanceLevel != SKERNEL_CONFORM_GREEN)
    {
        /* force out-of-profile traffic */
        __LOG(("--simulation only -- force out-of-profile traffic \n"));
        qosProfileInfoPtr->initialDp =
            skernelUserDebugInfo.policerConformanceLevel;
        __LOG_PARAM(qosProfileInfoPtr->initialDp);
    }

    descrPtr->dp = qosProfileInfoPtr->initialDp;
    /* DP assigned by metering engine  */
    descrPtr->meterDp = qosProfileInfoPtr->initialDp;
    __LOG(("DP assigned by metering engine[%d] \n",
        descrPtr->meterDp));

    if (clCalculated == GT_FALSE)
    {
        qosProfileInfoPtr->cl = qosProfileInfoPtr->initialDp;
    }

}

/*******************************************************************************
*   snetCht3PolicerPacketSizeGet
*
* DESCRIPTION:
*       Get packet size based on packet size mode
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - frame data buffer Id
*       direction   - ingress/egress policer direction
*       policerCtrlRegPtr - ingress policer control data
*
* OUTPUT
*       bytesCountPtr  - counter increment in bytes
*
* RETURN:
*
*******************************************************************************/
GT_VOID snetCht3PolicerPacketSizeGet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SMAIN_DIRECTION_ENT direction,
    IN GT_U32 * policerCtrlRegPtr,
    OUT GT_U32 * bytesCountPtr
)
{
    DECLARE_FUNC_NAME(snetCht3PolicerPacketSizeGet);

    GT_U32 fldValue;
    GT_U32 bytesCount;

    ASSERT_PTR(bytesCountPtr);

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        skernelFatalError("snetCht3PolicerPacketSizeGet: the function replaced with snetLion3PolicerPacketSizeGet \n");
        return;
    }


    *bytesCountPtr = bytesCount = 0;

    /* Controls packet byte count calculation for metering and counting */
    __LOG(("Controls packet byte count calculation for metering and counting"));
    fldValue = SMEM_U32_GET_FIELD(*policerCtrlRegPtr, 16, 1);
    if (fldValue == SNET_CHT3_LAYER3_PCKT_SIZE_E)
    {
        /* Get the offset between L3 and L2 and 4 bytes of CRC */
        __LOG(("Count LAYER3 only bytes \n"));
        bytesCount = descrPtr->byteCount - descrPtr->l2HeaderSize - 4;
    }
    else
    /*if (fldValue == SNET_CHT3_LAYER2_PCKT_SIZE_E)*/
    {
        __LOG(("Count LAYER2 packet bytes , descrPtr->byteCount [0x%x]\n",
            descrPtr->byteCount));
        bytesCount = descrPtr->byteCount;
        /* Include Layer 1 Overhead */
        fldValue = SMEM_U32_GET_FIELD(*policerCtrlRegPtr, 17, 1);
        if (fldValue)
        {
            /* Packet byte count includes 20 bytes of layer-1 overhead */
            __LOG(("include LAYER1 (20 bytes) \n"));
            bytesCount += 20;
        }

        fldValue = SMEM_U32_GET_FIELD(*policerCtrlRegPtr, 23, 1);
        /* Enable Compensation of DSA Tag added byte count when metering and counting
           packets received via DSA-enabled ports */
        if (fldValue && descrPtr->marvellTagged)
        {
            /* decrement the bytes of the DSA tag */
            __LOG(("decrement the bytes of the DSA tag (remove [%d] bytes) \n",
                ((descrPtr->marvellTaggedExtended + 1)*4)));
            bytesCount -= (descrPtr->marvellTaggedExtended + 1)*4;
            if(descrPtr->origSrcTagged)
            {
                /* add the bytes of the original tag (that used in first word of the DSA tag) */
                __LOG(("add the 4 bytes of the original tag (that used in first word of the DSA tag)"));
                bytesCount += 4;
            }
        }
    }

    /* Final bytes count  */
    *bytesCountPtr = bytesCount;
    __LOG(("Final bytes count[%d] for PLR[%d] \n" ,
        bytesCount,
        descrPtr->policerCycle));
}

/*******************************************************************************
*   snetCht3PolicerCountersUpdate
*
* DESCRIPTION:
*        Count the bytes of packets in the policer
*
* INPUTS:
*       devObjPtr            - pointer to device object.
*       descrPtr             - frame data buffer Id
*       policerMeterEntryPtr - policer entry pointer
*       cl                   -  conformance level
*
*
* OUTPUT:
*       mngCountDataPtr     - management counters data pointer
*
* RETURN:
*
*******************************************************************************/
static GT_VOID snetCht3PolicerCountersUpdate
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_U32 * policerMeterEntryPtr,
    IN SKERNEL_CONFORMANCE_LEVEL_ENT cl,
    OUT SNET_CHT3_POLICER_MNG_COUNT_STC * mngCountDataPtr
)
{
    DECLARE_FUNC_NAME(snetCht3PolicerCountersUpdate);

    GT_U32 fldValue;                    /* register's entry field */
    GT_U32 regAddr;                     /* register's address */
    GT_U32 bytesCount = 0;              /* number of bytes in packet */
    GT_U32 unitSizeCount;               /* number of unit size to add to counter */
    GT_U32 * policerCtrlRegPtr;         /* ingress policer control data */

    /* Set init values */
    memset(mngCountDataPtr, 0, sizeof(SNET_CHT3_POLICER_MNG_COUNT_STC));

    policerCtrlRegPtr = smemMemGet(devObjPtr,
                                   SMEM_CHT_POLICER_GLB_CONF_REG(devObjPtr, descrPtr->policerCycle));

    if(SKERNEL_IS_XCAT_REVISON_A1_DEV(devObjPtr) == 0)
    {
        /* No billing counters without metering in Xcat A0 */
        __LOG(("No billing counters without metering in Xcat A0"));
        if(policerMeterEntryPtr == NULL)
            return;
        /* Check - Enables counting In Profile and Out of Profile packets */
        __LOG(("Check - Enables counting In Profile and Out of Profile packets"));
        if (SMEM_U32_GET_FIELD(*(policerCtrlRegPtr), 3, 1) == 0)
        {
            return;
        }

        snetCht3PolicerPacketSizeGet(devObjPtr, descrPtr,
                                     SMAIN_DIRECTION_INGRESS_E,
                                     policerCtrlRegPtr, &bytesCount);

        /* Billing Counters Mode: 1 Byte/16 Bytes/1 Packet */
        __LOG(("Billing Counters Mode: 1 Byte/16 Bytes/1 Packet"));
        fldValue = SMEM_U32_GET_FIELD(*policerCtrlRegPtr, 1, 2);
        /* Convert packet size in bytes to unit size */
        __LOG(("Convert packet size in bytes to unit size"));
        unitSizeCount = SNET_CHT3_PCKT_SIZE_RESOLVE(bytesCount, fldValue);
        /* Pointer to the counters pairs in the Policers Counters Table Entry */
        __LOG(("Pointer to the counters pairs in the Policers Counters Table Entry"));
        fldValue = SMEM_U32_GET_FIELD(policerMeterEntryPtr[6], 0, 14);

        /* Policers Counters Table Entry */
        __LOG(("Policers Counters Table Entry"));
        regAddr = SMEM_CHT_POLICER_CNT_TBL_MEM(devObjPtr, descrPtr->policerCycle, fldValue);

        /* Update billing Counters Entry */
        __LOG(("Update billing Counters Entry"));
        SMEM_CHT_POLICER_CNT_TBL_UPDATE(devObjPtr, regAddr,
                                        descrPtr->dp, unitSizeCount);
    }
    else
    {
        /* Increment Policer Billing/Policy/VLAN Counters */
        __LOG(("Increment Policer Billing/Policy/VLAN Counters"));
        snetXCatPolicerCounterIncrement(devObjPtr, descrPtr,
                                        SMAIN_DIRECTION_INGRESS_E,
                                        policerCtrlRegPtr,
                                        policerMeterEntryPtr,
                                        0, /* egress port number - not relevant here */
                                        cl,
                                        &bytesCount);
        if(policerMeterEntryPtr == NULL)
            return;
    }

    /* Set number 0-2 and enable of the Management Counters */
    if(SMEM_CHT_IS_SIP5_15_GET(devObjPtr))
    {
        mngCountDataPtr->countSet =
            SMEM_SIP5_15_PLR_METERING_CONFIG_ENTRY_FIELD_GET(devObjPtr,
                descrPtr->policerMeterConfigEntryMemoryPtr,
                descrPtr->policerActuallAccessedIndex,
                SMEM_SIP5_15_PLR_METERING_CONFIG_TABLE_FIELDS_MG_COUNTERS_SET_EN_E);
    }
    else
    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        mngCountDataPtr->countSet =
            SMEM_LION3_PLR_METERING_ENTRY_FIELD_GET(devObjPtr,policerMeterEntryPtr,
                descrPtr->policerActuallAccessedIndex,
                SMEM_LION3_PLR_METERING_TABLE_FIELDS_MG_COUNTERS_SET_EN);
    }
    else
    {
        mngCountDataPtr->countSet =
            SNET_CHT3_POLICER_MNG_COUNT_GET_MAC(devObjPtr, policerMeterEntryPtr);
    }

    if (mngCountDataPtr->countSet == SNET_CHT3_MNG_SET_0_E)
    {
        /* Controls the resolution of Data Unit (DU) counters in counter set 0 */
        __LOG(("Controls the resolution of Data Unit (DU) counters in counter set 0"));
        mngCountDataPtr->countScale =
            SMEM_U32_GET_FIELD(*policerCtrlRegPtr, 8, 1);
    }
    else if (mngCountDataPtr->countSet == SNET_CHT3_MNG_SET_1_E)
    {
        /* Controls the resolution of Data Unit (DU) counters in counter set 1 */
        __LOG(("Controls the resolution of Data Unit (DU) counters in counter set 1"));
        mngCountDataPtr->countScale =
            SMEM_U32_GET_FIELD(*policerCtrlRegPtr, 9, 1);
    }
    else if (mngCountDataPtr->countSet == SNET_CHT3_MNG_SET_2_E)
    {
        /* Controls the resolution of Data Unit (DU) counters in counter set 2 */
        __LOG(("Controls the resolution of Data Unit (DU) counters in counter set 2"));
        mngCountDataPtr->countScale =
            SMEM_U32_GET_FIELD(*policerCtrlRegPtr, 10, 1);
    }
    else
    {
        /* counter set disabled */
        __LOG(("no counter SET enabled \n"));
        return;
    }

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        /* byte count will be calculated when needed */
        __LOG(("Calc num bytes needed for [%s] purpose \n",
            "Management counters"));
        snetLion3PolicerPacketSizeGet(devObjPtr, descrPtr,
                                     SMAIN_DIRECTION_INGRESS_E,
                                     SNET_LION3_POLICER_PACKET_SIZE_FROM_GLOBAL_CONFIG_E,
                                     NULL,
                                     &bytesCount);
    }

    mngCountDataPtr->countByteSize = bytesCount;

    /* Convert byte size to acceptable counter resolution 1 byte/16 byte */
    __LOG(("Convert byte size to acceptable counter resolution 1 byte/16 byte"));
    unitSizeCount = SNET_CHT3_PCKT_SIZE_RESOLVE(mngCountDataPtr->countByteSize,
                                                mngCountDataPtr->countScale);
    /* Update management counter table for NON RED packets.
       RED packets counting increment will be after remarking if remarking
       command is NOT DROP */
    if ((descrPtr->dp == SKERNEL_CONFORM_GREEN) &&
        (!SMEM_CHT_IS_SIP5_GET(devObjPtr)))
    {
        SMEM_CHT_POLICER_MNG_CNT_TBL_UPDATE(devObjPtr,
                                            descrPtr->policerCycle,
                                            mngCountDataPtr->countSet,
                                            descrPtr->dp,
                                            unitSizeCount);
    }
}

/*******************************************************************************
*   snetCht3PolicerQosRemark
*
* DESCRIPTION:
*        remark QoS parameters - only for out-of-profile traffic !!!
*        Out-of-profile packets are subject to out-of-profile commands
*        and may be discarded or have their packet QoS information remarked.
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       descrPtr            - frame data buffer Id
*       policerMeterEntryPtr      - policer entry pointer
*       qosProfileInfoPtr   - qos profile (index) for out-of-profile traffic
*       mngCountDataPtr     - management counters data pointer
*
* RETURN:
*
*******************************************************************************/
static GT_VOID snetCht3PolicerQosRemark
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_U32 * policerMeterEntryPtr,
    IN SNET_CHT3_POLICER_QOS_INFO_STC * qosProfileInfoPtr,
    IN SNET_CHT3_POLICER_MNG_COUNT_STC * mngCountDataPtr
)
{
    DECLARE_FUNC_NAME(snetCht3PolicerQosRemark);

    GT_U32 fldValue;                    /* register's entry field */
    GT_U32 qosProfileNew;               /* new QOS profile index */
    GT_U32 unitSizeCount;               /* number of unit size to add to counter */
    GT_BOOL drop = GT_FALSE;            /* soft/hard drop */
    GT_U32 mngCntrDp;                   /* DP for management counters update */
    GT_CHAR*   dpName[3] = {"green","yellow","red"};
    GT_U32  dpNameIndex;/* index into dpName[]*/
    GT_CHAR*   modifyName[3] = {"force modify","force not modify","not care"};

    if(policerMeterEntryPtr == NULL)
        return;

    if ((descrPtr->dp == SKERNEL_CONFORM_GREEN) &&
        (!SMEM_CHT_IS_SIP5_GET(devObjPtr)))
    {
        return;
    }

    if (qosProfileInfoPtr->initialDp == SKERNEL_CONFORM_YELLOW)
    {
        if(SMEM_CHT_IS_SIP5_15_GET(devObjPtr))
        {
            fldValue =
                SMEM_SIP5_15_PLR_METERING_CONFIG_ENTRY_FIELD_GET(devObjPtr,
                    descrPtr->policerMeterConfigEntryMemoryPtr,
                    descrPtr->policerActuallAccessedIndex,
                    SMEM_SIP5_15_PLR_METERING_CONFIG_TABLE_FIELDS_INGRESS_YELLOW_CMD_E);
        }
        else
        if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            fldValue =
                SMEM_LION3_PLR_METERING_ENTRY_FIELD_GET(devObjPtr,policerMeterEntryPtr,
                    descrPtr->policerActuallAccessedIndex,
                    SMEM_LION3_PLR_METERING_TABLE_FIELDS_INGRESS_YELLOW_CMD);
        }
        else
        {
            fldValue =
                SNET_CHT3_POLICER_YELLOW_CMD_GET_MAC(devObjPtr, policerMeterEntryPtr);
        }

        /* Apply yellow policer command */
        dpNameIndex = SKERNEL_CONFORM_YELLOW;
        qosProfileNew = qosProfileInfoPtr->qosProfYellowIndex;
    }
    else if(SMEM_CHT_IS_SIP5_GET(devObjPtr) && (qosProfileInfoPtr->initialDp == SKERNEL_CONFORM_GREEN))
    {
        if(SMEM_CHT_IS_SIP5_15_GET(devObjPtr))
        {
            fldValue =
                SMEM_SIP5_15_PLR_METERING_CONFIG_ENTRY_FIELD_GET(devObjPtr,
                    descrPtr->policerMeterConfigEntryMemoryPtr,
                    descrPtr->policerActuallAccessedIndex,
                    SMEM_SIP5_15_PLR_METERING_CONFIG_TABLE_FIELDS_INGRESS_GREEN_CMD_E);
        }
        else
        {
            fldValue =
                SMEM_LION3_PLR_METERING_ENTRY_FIELD_GET(devObjPtr,policerMeterEntryPtr,
                    descrPtr->policerActuallAccessedIndex,
                    SMEM_LION3_PLR_METERING_TABLE_FIELDS_INGRESS_GREEN_CMD);
        }

        /* Apply red policer command */
        dpNameIndex = SKERNEL_CONFORM_GREEN;
        qosProfileNew = qosProfileInfoPtr->qosProfGreenIndex;
    }
    else
    {
        if(SMEM_CHT_IS_SIP5_15_GET(devObjPtr))
        {
            fldValue =
                SMEM_SIP5_15_PLR_METERING_CONFIG_ENTRY_FIELD_GET(devObjPtr,
                    descrPtr->policerMeterConfigEntryMemoryPtr,
                    descrPtr->policerActuallAccessedIndex,
                    SMEM_SIP5_15_PLR_METERING_CONFIG_TABLE_FIELDS_INGRESS_RED_CMD_E);
        }
        else
        if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            fldValue =
                SMEM_LION3_PLR_METERING_ENTRY_FIELD_GET(devObjPtr,policerMeterEntryPtr,
                    descrPtr->policerActuallAccessedIndex,
                    SMEM_LION3_PLR_METERING_TABLE_FIELDS_INGRESS_RED_CMD);
        }
        else
        {
            fldValue =
                SNET_CHT3_POLICER_RED_CMD_GET_MAC(devObjPtr, policerMeterEntryPtr);
        }

        /* Apply red policer command */
        dpNameIndex = SKERNEL_CONFORM_RED;
        qosProfileNew = qosProfileInfoPtr->qosProfRedIndex;
    }

    __LOG(("Apply [%s] DP command , with new QOS index [%d] \n",
        dpName[dpNameIndex],
        qosProfileNew));

    if (fldValue == SNET_CHT_POLICER_CMD_DROP_E)
    {
        /* Type of the policer out-of-profile drop action */
        __LOG(("Type of the policer out-of-profile drop action"));
        smemRegFldGet(devObjPtr,
                      SMEM_CHT_POLICER_GLB_CONF_REG(devObjPtr, descrPtr->policerCycle),
                      5, 1, &fldValue);

        /* Apply packet command */
        __LOG(("Apply packet command"));
        descrPtr->packetCmd = snetChtPktCmdResolution(descrPtr->packetCmd,
                                fldValue ?
                                    SKERNEL_EXT_PKT_CMD_HARD_DROP_E :
                                    SKERNEL_EXT_PKT_CMD_SOFT_DROP_E);
        drop = GT_TRUE;
    }
    else if ((fldValue == SNET_CHT_POLICER_CMD_REM_QOS_E) ||
             (fldValue == SNET_CHT_POLICER_CMD_REM_QOS_BY_ENTRY_E))
    {
        if (fldValue == SNET_CHT_POLICER_CMD_REM_QOS_E)
        {
            /* The QoSProfile assigned to non-conforming packets */
            __LOG(("The QoSProfile assigned to non-conforming packets"));
            descrPtr->qos.qosProfile = qosProfileNew;
        }
        else
        {   /* take QoS profile from meter entry */
            __LOG(("take QoS profile from meter entry"));
            if (!SKERNEL_IS_XCAT_REVISON_A1_DEV(devObjPtr))
            {
                /* only xCat A1 and above supports this option */
                __LOG(("only xCat A1 and above supports this option"));
                skernelFatalError("snetCht3PolicerQosRemark: wrong remark command\n");
                return;
            }

            if(SMEM_CHT_IS_SIP5_15_GET(devObjPtr))
            {
                descrPtr->qos.qosProfile =
                    SMEM_SIP5_15_PLR_METERING_CONFIG_ENTRY_FIELD_GET(devObjPtr,
                        descrPtr->policerMeterConfigEntryMemoryPtr,
                        descrPtr->policerActuallAccessedIndex,
                        SMEM_SIP5_15_PLR_METERING_CONFIG_TABLE_FIELDS_INGRESS_QOS_PROFILE_E);
            }
            else
            if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
            {
                descrPtr->qos.qosProfile =
                    SMEM_LION3_PLR_METERING_ENTRY_FIELD_GET(devObjPtr,policerMeterEntryPtr,
                        descrPtr->policerActuallAccessedIndex,
                        SMEM_LION3_PLR_METERING_TABLE_FIELDS_INGRESS_QOS_PROFILE);
            }
            else
            {
                descrPtr->qos.qosProfile =
                    SNET_CHT3_POLICER_QOS_PROFILE_GET_MAC(devObjPtr, policerMeterEntryPtr);
            }
        }

        /* Policer Modify DSCP */
        if(SMEM_CHT_IS_SIP5_15_GET(devObjPtr))
        {
            fldValue =
                SMEM_SIP5_15_PLR_METERING_CONFIG_ENTRY_FIELD_GET(devObjPtr,
                    descrPtr->policerMeterConfigEntryMemoryPtr,
                    descrPtr->policerActuallAccessedIndex,
                    SMEM_SIP5_15_PLR_METERING_CONFIG_TABLE_FIELDS_INGRESS_MODIFY_DSCP_E);
        }
        else
        if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            fldValue =
                SMEM_LION3_PLR_METERING_ENTRY_FIELD_GET(devObjPtr,policerMeterEntryPtr,
                    descrPtr->policerActuallAccessedIndex,
                    SMEM_LION3_PLR_METERING_TABLE_FIELDS_INGRESS_MODIFY_DSCP);
        }
        else
        {
            fldValue =
                SNET_CHT3_POLICER_MODIFY_DSCP_GET_MAC(devObjPtr, policerMeterEntryPtr);
        }

        if(fldValue <=2)
        {
            __LOG(("Policer [%s] DSCP \n",modifyName[fldValue]));
        }
        descrPtr->modifyDscp = (fldValue == 1) ? 1 : (fldValue == 2) ? 0 :
                                descrPtr->modifyDscp;

        /* Policer Modify UP */
        if(SMEM_CHT_IS_SIP5_15_GET(devObjPtr))
        {
            fldValue =
                SMEM_SIP5_15_PLR_METERING_CONFIG_ENTRY_FIELD_GET(devObjPtr,
                    descrPtr->policerMeterConfigEntryMemoryPtr,
                    descrPtr->policerActuallAccessedIndex,
                    SMEM_SIP5_15_PLR_METERING_CONFIG_TABLE_FIELDS_INGRESS_MODIFY_UP_E);
        }
        else
        if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            fldValue =
                SMEM_LION3_PLR_METERING_ENTRY_FIELD_GET(devObjPtr,policerMeterEntryPtr,
                    descrPtr->policerActuallAccessedIndex,
                    SMEM_LION3_PLR_METERING_TABLE_FIELDS_INGRESS_MODIFY_UP);
        }
        else
        {
            fldValue =
                SNET_CHT3_POLICER_MODIFY_UP_GET_MAC(devObjPtr, policerMeterEntryPtr);
        }

        if(fldValue <=2)
        {
            __LOG(("Policer [%s] UP \n",modifyName[fldValue]));
        }

        descrPtr->modifyUp = (fldValue == 1) ? 1 : (fldValue == 2) ? 0 :
                                descrPtr->modifyUp;
    }

    /* update management counters if NOT disabled */
    if (mngCountDataPtr->countSet == SNET_CHT3_MNG_SET_DISABLE_E)
        return;

    if ((descrPtr->dp == SKERNEL_CONFORM_RED) ||
        (descrPtr->dp == SKERNEL_CONFORM_YELLOW) ||
        (descrPtr->dp == SKERNEL_CONFORM_GREEN))
    {
        /* Convert byte size to acceptable counter resolution 1 byte/16 byte */
        __LOG(("Convert byte size to acceptable counter resolution 1 byte/16 byte"));
        unitSizeCount = SNET_CHT3_PCKT_SIZE_RESOLVE(mngCountDataPtr->countByteSize,
                                                    mngCountDataPtr->countScale);

        if(drop == GT_TRUE)
        {
            /* update DROP counter */
            __LOG(("update DROP counter"));
            mngCntrDp = SKERNEL_CONFORM_DROP;
        }
        else
        {
            mngCntrDp = descrPtr->dp;
            __LOG(("updated %d [0-GREEN\1-YELLOW\2-RED] counter \n",mngCntrDp));
        }

        /* update counters */
        __LOG(("update counters"));
        SMEM_CHT_POLICER_MNG_CNT_TBL_UPDATE(devObjPtr,
                                            descrPtr->policerCycle,
                                            mngCountDataPtr->countSet,
                                            mngCntrDp,
                                            unitSizeCount);
    }
}

/*******************************************************************************
*   snetCht3PolicerHQosTableApply
*
* DESCRIPTION:
*        Enables applying metering/counting to flows in a hierarchical manner.
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       descrPtr            - frame data buffer Id
*
* RETURN:
*
*******************************************************************************/
static GT_VOID snetCht3PolicerHQosTableApply
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
)
{
    GT_U32 * memPtr;                        /* pointer to table entry */
    GT_U32 entryIndex;                      /* table entry index */

    DECLARE_FUNC_NAME(snetCht3PolicerHQosTableApply);

    if(0 == SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        return;
    }

    if((descrPtr->policerTriggerMode == SKERNEL_POLICER_TRIGGER_MODE_FLOW_E) &&
       (descrPtr->policerCycle == 0))
    {
        entryIndex = descrPtr->policerPtr;
        __LOG(("Hierarchical Ingress Policing: descrPtr->policerCycle = 0 \n"));
        memPtr = smemMemGet(devObjPtr,
            SMEM_XCAT_POLICER_HIERARCHCIAL_TBL_MEM(devObjPtr, 0, entryIndex));

        descrPtr->policerPtr =
            SMEM_LION3_PLR_HIERARCHIAL_ENTRY_FIELD_GET(devObjPtr, memPtr, entryIndex,
                SMEM_LION3_PLR_HIERARCHICAL_TABLE_FIELDS_POLICER_PTR_E);
        descrPtr->policerEn =
            SMEM_LION3_PLR_HIERARCHIAL_ENTRY_FIELD_GET(devObjPtr, memPtr, entryIndex,
                SMEM_LION3_PLR_HIERARCHICAL_TABLE_FIELDS_METERING_ENABLE_E);
        descrPtr->policerCounterEn =
            SMEM_LION3_PLR_HIERARCHIAL_ENTRY_FIELD_GET(devObjPtr, memPtr, entryIndex,
                SMEM_LION3_PLR_HIERARCHICAL_TABLE_FIELDS_COUNTING_ENABLE_E);
    }
}
