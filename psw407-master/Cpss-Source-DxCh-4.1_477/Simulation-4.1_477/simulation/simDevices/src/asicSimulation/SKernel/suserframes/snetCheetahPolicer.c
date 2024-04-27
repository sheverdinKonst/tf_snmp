/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetCheetahPolicer.c
*
* DESCRIPTION:
*       (Cheetah) Policing Engine processing for frame -- simulation
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 8 $
*
*******************************************************************************/

#include <asicSimulation/SKernel/smem/smem.h>
#include <asicSimulation/SKernel/smem/smemCheetah.h>
#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SKernel/suserframes/snet.h>
#include <asicSimulation/SKernel/cheetahCommon/sregCheetah.h>
#include <asicSimulation/SKernel/suserframes/snetCheetahPolicer.h>
#include <asicSimulation/SKernel/suserframes/snetCheetah3Policer.h>
#include <asicSimulation/SLog/simLog.h>

/* Private declarations */
static GT_BOOL snetChtPolicerTriggeringCheck(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    OUT GT_U32 * policerInfoPtr
);

static GT_VOID snetChtPolicerConformanceLevelGet(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_U32 * policerInfoPtr,
    OUT SKERNEL_CONFORMANCE_LEVEL_ENT * conformanceLevelPtr,
    OUT GT_U32 * qosProfileForOutOfProfilePtr
);

static GT_VOID snetChtPolicerCountersUpdate(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_U32 * policerInfoPtr,
    IN SKERNEL_CONFORMANCE_LEVEL_ENT conformanceLevel
);

static GT_VOID snetChtPolicerQosRemark(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_U32 * policerInfoPtr,
    IN GT_U32 qosProfileForOutOfProfile
);

typedef enum {
    SNET_CHT_CONFORM_GREEN_E = 0,
    SNET_CHT_SKERNEL_CONFORM_RED_E
}SNET_CHT_CONFORM_LEVEL_ENT;

typedef enum {
    SNET_CHT_LAYER1_COUNT_E = 0,
    SNET_CHT_LAYER2_COUNT_E,
    SNET_CHT_LAYER3_COUNT_E
} SNET_CHT_POLICING_MODE_ENT;

/*******************************************************************************
*   snetChtPolicer
*
* DESCRIPTION:
*        Policer Processing  --- Policer Counters updates
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - frame data buffer Id
*
* RETURN:
*
*******************************************************************************/
GT_VOID snetChtPolicer
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetChtPolicer);

    GT_BOOL doPolicer;          /* do we need to enter the policer engine */
    GT_U32 policerInfo[3];      /* Policers Table Entry */

    SKERNEL_CONFORMANCE_LEVEL_ENT conformanceLevel;
                                /* conformance level  0 - GREEN , 1 - RED */

    GT_U32 qosProfileForOutOfProfile;
                                /* qos profile (index) for out-of-profile
                                   traffic */

    /* Check if policer needed */
    __LOG(("Check if policer needed"));
    doPolicer =
        snetChtPolicerTriggeringCheck(devObjPtr, descrPtr, &policerInfo[0]);

    if (doPolicer == GT_FALSE)
    {
        /* The Policer is bypassed  */
        __LOG(("The Policer is bypassed.\n"));

        return;
    }

    /* Get conformance level and qos profile for out-of-profile traffic */
    __LOG(("Get conformance level and qos profile for out-of-profile traffic"));
    snetChtPolicerConformanceLevelGet(devObjPtr, descrPtr, &policerInfo[0],
                                      &conformanceLevel,
                                      &qosProfileForOutOfProfile);
    /* Update policer counters */
    __LOG(("Update policer counters"));
    snetChtPolicerCountersUpdate(devObjPtr, descrPtr, &policerInfo[0],
                                 conformanceLevel);

    if (conformanceLevel == (SKERNEL_CONFORMANCE_LEVEL_ENT)SNET_CHT_CONFORM_GREEN_E)
    {
        return;
    }

    /* Remark out-of-profile traffic */
    __LOG(("Remark out-of-profile traffic"));
    snetChtPolicerQosRemark(devObjPtr, descrPtr, &policerInfo[0],
                            qosProfileForOutOfProfile);
}

/*******************************************************************************
*   snetChtPolicerTriggeringCheck
*
* DESCRIPTION:
*        check if need to do policer
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - frame data buffer Id
*
* OUTPUT:
*       policerInfoPtr  - policer entry pointer
*
* RETURN:
*
*******************************************************************************/
static GT_BOOL snetChtPolicerTriggeringCheck
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    OUT GT_U32 * policerInfoPtr
)
{
    GT_U32 regAddr;                     /* register's address */
    GT_U32 * regPtr;                    /* register's entry pointer */
    GT_U32 fldValue;                    /* register's entry field */

    if ((descrPtr->packetCmd != SKERNEL_EXT_PKT_CMD_FORWARD_E &&
         descrPtr->packetCmd != SKERNEL_EXT_PKT_CMD_MIRROR_TO_CPU_E) ||
         descrPtr->policerEn == 0)
    {
        return GT_FALSE;
    }

    regAddr = SMEM_CHT_POLICER_TBL_MEM(devObjPtr, 0, descrPtr->policerPtr);
    /* Policer table entry pointer */
    regPtr = smemMemGet(devObjPtr, regAddr);

    policerInfoPtr[0] = regPtr[0];
    policerInfoPtr[1] = regPtr[1];
    policerInfoPtr[2] = regPtr[2];

    /* Policer Enable */
    fldValue = SMEM_U32_GET_FIELD(regPtr[1], 13, 1);
    if (fldValue == 0)
    {
        return GT_FALSE;
    }

    return GT_TRUE;
}

/*******************************************************************************
*   snetChtPolicerConformanceLevelGet
*
* DESCRIPTION:
*        Get conformance level and QoSProfile for out-of-profile traffic
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       descrPtr            - frame data buffer Id
*       policerInfoPtr      - policer entry pointer
*
* OUTPUT:
*       conformanceLevelPtr - (pointer to)conformance level 0- GREEN , 1-RED
*       qosProfileForOutOfProfilePtr
*                           - (pointer to)qos profile (index)
*                              for out-of-profile traffic
*
* RETURN:
*
*******************************************************************************/
static GT_VOID snetChtPolicerConformanceLevelGet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_U32 * policerInfoPtr,
    OUT SKERNEL_CONFORMANCE_LEVEL_ENT * conformanceLevelPtr,
    OUT GT_U32 * qosProfileForOutOfProfilePtr
)
{
    DECLARE_FUNC_NAME(snetChtPolicerConformanceLevelGet);

    GT_U32 fldValue;                    /* register's entry field */
    GT_U32 regAddr;                     /* register's address */
    GT_U32 * regPtr;                    /* register's entry pointer */

    /* Policers QoS Remarking and Initial DP Table Entry */
    regAddr = SMEM_CHT_POLICER_QOS_TBL_MEM(devObjPtr, 0,
                                            descrPtr->qos.qosProfile);
    regPtr = smemMemGet(devObjPtr, regAddr);

    /* Intial DP */
    (*conformanceLevelPtr) = SMEM_U32_GET_FIELD(regPtr[0], 7, 1);
    /* Remark Table QosProfile[6:0] */
    (*qosProfileForOutOfProfilePtr) = SMEM_U32_GET_FIELD(regPtr[0], 0, 7);

    /* Policer color mode */
    fldValue = SMEM_U32_GET_FIELD(policerInfoPtr[0], 29, 1);
    if (fldValue == 0)
    {
        /* When color blind we are always GREEN */
        __LOG(("When color blind we are always GREEN"));
        (*conformanceLevelPtr) = SNET_CHT_CONFORM_GREEN_E;
    }

    /* Check simulation debug info if we force out-of-profile traffic */
    __LOG(("Check simulation debug info if we force out-of-profile traffic"));
    if (skernelUserDebugInfo.policerConformanceLevel == SKERNEL_CONFORM_RED)
    {
        /* force out-of-profile traffic */
        (*conformanceLevelPtr) = SNET_CHT_SKERNEL_CONFORM_RED_E;
    }
}

/*******************************************************************************
*   snetChtPolicerCountersUpdate
*
* DESCRIPTION:
*        count the bytes of packets in the policer
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       descrPtr            - frame data buffer Id
*       policerInfoPtr      - policer entry pointer
*       conformanceLevel    - conformance level 0- GREEN , 1-RED
*
* RETURN:
*
*******************************************************************************/
static GT_VOID snetChtPolicerCountersUpdate
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_U32 * policerInfoPtr,
    IN SKERNEL_CONFORMANCE_LEVEL_ENT conformanceLevel
)
{
    DECLARE_FUNC_NAME(snetChtPolicerCountersUpdate);

    GT_U32 fldValue;                    /* register's entry field */
    GT_U32 regAddr;                     /* register's address */
    GT_U32 bytesCount = 0;              /* number of bytes to add to counter */

    /* Enables counting In Profile and Out of Profile packets */
    __LOG(("Enables counting In Profile and Out of Profile packets"));
    fldValue = SMEM_U32_GET_FIELD(policerInfoPtr[1], 14, 1);
    if (fldValue == 0)
    {
        return;
    }

    /* Policing Mode[1:0] */
    __LOG(("Policing Mode[1:0]"));
    smemRegFldGet(devObjPtr, SMEM_CHT_POLICER_GLB_CONF_REG(devObjPtr, 0), 0, 2, &fldValue);
    if (fldValue == SNET_CHT_LAYER1_COUNT_E)
    {
        /* 4 - for CRC 12 - for IPG 8 - Preamble */
        __LOG(("4 - for CRC 12 - for IPG 8 - Preamble"));
        bytesCount = descrPtr->byteCount + 12 + 8 + 4;
    }
    else
    if (fldValue == SNET_CHT_LAYER2_COUNT_E)
    {
        /* 4 - for CRC */
        __LOG(("4 - for CRC"));
        bytesCount = descrPtr->byteCount + 4;
    }
    else
    if (fldValue == SNET_CHT_LAYER3_COUNT_E)
    {
        /* Get the offset between L3 and L2 */
        __LOG(("Get the offset between L3 and L2"));
        bytesCount = descrPtr->byteCount -
                     descrPtr->l2HeaderSize;
    }

    /* Pointer to the counters pairs in the Policers Counters Table Entry */
    fldValue = SMEM_U32_GET_FIELD(policerInfoPtr[1], 15, 4);

    /* Policers Counters Table Entry */
    regAddr = SMEM_CHT_POLICER_CNT_TBL_MEM(devObjPtr, 0, fldValue);

    if (conformanceLevel == (SKERNEL_CONFORMANCE_LEVEL_ENT)SNET_CHT_CONFORM_GREEN_E )
    {
        /* In-Profile bytes counter */
        smemRegGet(devObjPtr, regAddr, &fldValue);
        fldValue += bytesCount;
        smemRegSet(devObjPtr, regAddr, fldValue);
    }
    else
    {
        /* Out-of-Profile bytes counter */
        regAddr += 0x4;
        smemRegGet(devObjPtr, regAddr, &fldValue);
        fldValue += bytesCount;
        smemRegSet(devObjPtr, regAddr, fldValue);
    }
}

/*******************************************************************************
*   snetChtPolicerQosRemark
*
* DESCRIPTION:
*        remark QoS parameters - only for out-of-profile traffic !!!
*        Out-of-profile packets are subject to out-of-profile commands
*        and may be discarded or have their packet QoS information remarked.
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       descrPtr            - frame data buffer Id
*       policerInfoPtr      - policer entry pointer
*       qosProfileForOutOfProfile
*                           - qos profile (index) for out-of-profile traffic
*
* RETURN:
*
*******************************************************************************/
static GT_VOID snetChtPolicerQosRemark
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_U32 * policerInfoPtr,
    IN GT_U32 qosProfileForOutOfProfile
)
{
    DECLARE_FUNC_NAME(snetChtPolicerQosRemark);

    GT_U32 fldValue;                    /* register's entry field */
    GT_U32 modifyAttr;

    /* Policer Cmd */
    fldValue = SMEM_U32_GET_FIELD(policerInfoPtr[1], 0, 2);
    if (fldValue == SNET_CHT_POLICER_CMD_DROP_E)
    {
        /* Type of the policer out-of-profile drop action */
        __LOG(("Type of the policer out-of-profile drop action"));
        smemRegFldGet(devObjPtr, SMEM_CHT_POLICER_GLB_CONF_REG(devObjPtr, 0),
                      2, 1, &fldValue);
        if (fldValue)
        {
            /* Hard Drop */
            __LOG(("Hard Drop"));
            descrPtr->packetCmd = SKERNEL_EXT_PKT_CMD_HARD_DROP_E;
        }
        else
        {
            /* Soft Drop */
            __LOG(("Soft Drop"));
            descrPtr->packetCmd = SKERNEL_EXT_PKT_CMD_SOFT_DROP_E;
        }
    }
    else
    if (fldValue == SNET_CHT_POLICER_CMD_REM_QOS_E)
    {
        /* The QoSProfile assigned to non-conforming packets */
        __LOG(("The QoSProfile assigned to non-conforming packets"));
        fldValue = SMEM_U32_GET_FIELD(policerInfoPtr[1], 2, 7);
        descrPtr->qos.qosProfile = fldValue;

        /* Policer Modify DSCP */
        __LOG(("Policer Modify DSCP"));
        modifyAttr = SMEM_U32_GET_FIELD(policerInfoPtr[1], 9, 2);
        descrPtr->modifyDscp = (modifyAttr == 1) ? 1 : (modifyAttr == 2) ? 0 :
                                descrPtr->modifyDscp;
        /* Policer Modify UP */
        __LOG(("Policer Modify UP"));
        modifyAttr = SMEM_U32_GET_FIELD(policerInfoPtr[1], 11, 2);
        descrPtr->modifyUp = (modifyAttr == 1) ? 1 : (modifyAttr == 2) ? 0 :
                                descrPtr->modifyUp;
    }
    else
    if (fldValue == SNET_CHT_POLICER_CMD_REM_QOS_BY_ENTRY_E)
    {
        descrPtr->qos.qosProfile = qosProfileForOutOfProfile;

        /* Policer Modify DSCP */
        __LOG(("Policer Modify DSCP"));
        modifyAttr = SMEM_U32_GET_FIELD(policerInfoPtr[1], 9, 2);
        descrPtr->modifyDscp =(modifyAttr == 1) ? 1 : (modifyAttr == 2) ? 0 :
                                descrPtr->modifyDscp;

        /* Policer Modify UP */
        __LOG(("Policer Modify UP"));
        modifyAttr = SMEM_U32_GET_FIELD(policerInfoPtr[1], 11, 2);
        descrPtr->modifyUp = (modifyAttr == 1) ? 1 : (modifyAttr == 2) ? 0 :
                                descrPtr->modifyUp;
    }
}

