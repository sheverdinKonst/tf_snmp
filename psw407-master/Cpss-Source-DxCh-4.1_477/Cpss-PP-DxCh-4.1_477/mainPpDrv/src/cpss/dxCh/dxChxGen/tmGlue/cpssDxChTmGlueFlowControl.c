/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
*
* cpssDxChTmGlueFlowControl.c
*
* DESCRIPTION:
*       Traffic Manager Glue - Flow Control API implementation.
*
* FILE REVISION NUMBER:
*       $Revision: 2 $
*******************************************************************************/
#define CPSS_LOG_IN_MODULE_ENABLE
#include <cpss/dxCh/dxChxGen/tmGlue/private/prvCpssDxChTmGlueLog.h>

#include <cpss/dxCh/dxChxGen/config/private/prvCpssDxChInfo.h>
#include <cpss/dxCh/dxChxGen/cos/private/prvCpssDxChCoS.h>
#include <cpss/dxCh/dxChxGen/tmGlue/cpssDxChTmGlueFlowControl.h>
#include <cpss/dxCh/dxChxGen/tmGlue/private/prvCpssDxChTmGluePfc.h>

/* check that the TM port index is valid return GT_BAD_PARAM on error */
#define PRV_CPSS_DXCH_TM_GLUE_FLOW_CONTROL_PORT_INDEX_CHECK_MAC(_tmPortInd)        \
    if ((_tmPortInd) >= 192) \
    {                                                                 \
        return GT_BAD_PARAM;                                          \
    }
/* resolves register index and start bit by TM port index */
#define PRV_CPSS_DXCH_TM_GLUE_FLOW_CONTROL_REG_INDEX_BIT_OFSSET_MAC(_tmPortInd, _index, _bit) \
    _index = _tmPortInd / 3; _bit = 10 * (_tmPortInd % 3);

/*******************************************************************************
* internal_cpssDxChTmGlueFlowControlEgressEnableSet
*
* DESCRIPTION:
*       Enable/disable Egress FIFOs Flow Control for TM.
*
* APPLICABLE DEVICES:
*       Bobcat2.
*
* NOT APPLICABLE DEVICES:
*       DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2; Lion; Lion2; Lion3.
*
* INPUTS:
*       devNum       - device number.
*       enable       - GT_TRUE - enable egress FIFO Flow Control for TM.
*                      GT_FALSE - disable egress FIFO Flow Control for TM.
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK                    - on success.
*       GT_BAD_PARAM             - on wrong device number.
*       GT_HW_ERROR              - on hardware error.
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static GT_STATUS internal_cpssDxChTmGlueFlowControlEgressEnableSet
(
    IN GT_U8                    devNum,
    IN GT_BOOL                  enable
)
{
    GT_U32 regAddr;         /* register address */
    GT_U32 data;            /* HW value */

    PRV_CPSS_DXCH_DEV_CHECK_MAC(devNum);
    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum,
          CPSS_CH1_E| CPSS_CH1_DIAMOND_E | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E | CPSS_XCAT3_E |
          CPSS_XCAT2_E | CPSS_LION_E | CPSS_LION2_E  | CPSS_BOBCAT3_E );

    PRV_CPSS_DXCH_PP_HW_INFO_E_ARCH_SUPPORTED_CHECK_MAC(devNum);

    data = (enable == GT_FALSE) ? 0 : 1;

    regAddr = PRV_DXCH_REG1_UNIT_TM_FCU_MAC(devNum).TMFCUGlobalConfigs;

    return prvCpssHwPpSetRegField(devNum, regAddr, 1, 1, data);
}

/*******************************************************************************
* cpssDxChTmGlueFlowControlEgressEnableSet
*
* DESCRIPTION:
*       Enable/disable Egress FIFOs Flow Control for TM.
*
* APPLICABLE DEVICES:
*       Bobcat2.
*
* NOT APPLICABLE DEVICES:
*       DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2; Lion; Lion2; Lion3.
*
* INPUTS:
*       devNum       - device number.
*       enable       - GT_TRUE - enable egress FIFO Flow Control for TM.
*                      GT_FALSE - disable egress FIFO Flow Control for TM.
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK                    - on success.
*       GT_BAD_PARAM             - on wrong device number.
*       GT_HW_ERROR              - on hardware error.
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
GT_STATUS cpssDxChTmGlueFlowControlEgressEnableSet
(
    IN GT_U8                    devNum,
    IN GT_BOOL                  enable
)
{
    GT_STATUS rc;
    CPSS_LOG_FUNC_VARIABLE_DECLARE_MAC(funcId, cpssDxChTmGlueFlowControlEgressEnableSet);

    CPSS_API_LOCK_MAC(0,0);
    CPSS_LOG_API_ENTER_MAC((funcId, devNum, enable));

    rc = internal_cpssDxChTmGlueFlowControlEgressEnableSet(devNum, enable);

    CPSS_LOG_API_EXIT_MAC(funcId, rc);
    CPSS_APP_SPECIFIC_CB_MAC((funcId, rc, devNum, enable));
    CPSS_API_UNLOCK_MAC(0,0);

    return rc;
}

/*******************************************************************************
* internal_cpssDxChTmGlueFlowControlEgressEnableGet
*
* DESCRIPTION:
*       Get state of egress FIFOs Flow Control for TM.
*
* APPLICABLE DEVICES:
*       Bobcat2.
*
* NOT APPLICABLE DEVICES:
*       DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2; Lion; Lion2; Lion3
*
* INPUTS:
*       devNum       - device number.
*
* OUTPUTS:
*       enablePtr    - (pointer to) flow control state.
*                      GT_TRUE - enable egress FIFO Flow Control for TM.
*                      GT_FALSE - disable egress FIFO Flow Control for TM.
*
* RETURNS:
*       GT_OK                    - on success.
*       GT_BAD_PARAM             - on wrong device number.
*       GT_BAD_PTR               - on NULL pointer.
*       GT_HW_ERROR              - on hardware error.
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device.
*
* COMMENTS:
*       None
*
*******************************************************************************/
static GT_STATUS internal_cpssDxChTmGlueFlowControlEgressEnableGet
(
    IN GT_U8                    devNum,
    OUT GT_BOOL                *enablePtr
)
{
    GT_STATUS rc;      /* return code */
    GT_U32    regAddr; /* register address */
    GT_U32    data;    /* HW data */

    PRV_CPSS_DXCH_DEV_CHECK_MAC(devNum);
    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum,
          CPSS_CH1_E| CPSS_CH1_DIAMOND_E | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E | CPSS_XCAT3_E |
          CPSS_XCAT2_E | CPSS_LION_E | CPSS_LION2_E  | CPSS_BOBCAT3_E );

    PRV_CPSS_DXCH_PP_HW_INFO_E_ARCH_SUPPORTED_CHECK_MAC(devNum);
    CPSS_NULL_PTR_CHECK_MAC(enablePtr);

    regAddr = PRV_DXCH_REG1_UNIT_TM_FCU_MAC(devNum).TMFCUGlobalConfigs;
    rc = prvCpssHwPpGetRegField(devNum, regAddr, 1, 1, &data);
    if(rc != GT_OK)
    {
        return rc;
    }

    *enablePtr = (data == 0) ? GT_FALSE : GT_TRUE;

    return GT_OK;
}

/*******************************************************************************
* cpssDxChTmGlueFlowControlEgressEnableGet
*
* DESCRIPTION:
*       Get state of egress FIFOs Flow Control for TM.
*
* APPLICABLE DEVICES:
*       Bobcat2.
*
* NOT APPLICABLE DEVICES:
*       DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2; Lion; Lion2; Lion3
*
* INPUTS:
*       devNum       - device number.
*
* OUTPUTS:
*       enablePtr    - (pointer to) flow control state.
*                      GT_TRUE - enable egress FIFO Flow Control for TM.
*                      GT_FALSE - disable egress FIFO Flow Control for TM.
*
* RETURNS:
*       GT_OK                    - on success.
*       GT_BAD_PARAM             - on wrong device number.
*       GT_BAD_PTR               - on NULL pointer.
*       GT_HW_ERROR              - on hardware error.
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device.
*
* COMMENTS:
*       None
*
*******************************************************************************/
GT_STATUS cpssDxChTmGlueFlowControlEgressEnableGet
(
    IN GT_U8                    devNum,
    OUT GT_BOOL                *enablePtr
)
{
    GT_STATUS rc;
    CPSS_LOG_FUNC_VARIABLE_DECLARE_MAC(funcId, cpssDxChTmGlueFlowControlEgressEnableGet);

    CPSS_API_LOCK_MAC(0,0);
    CPSS_LOG_API_ENTER_MAC((funcId, devNum, enablePtr));

    rc = internal_cpssDxChTmGlueFlowControlEgressEnableGet(devNum, enablePtr);

    CPSS_LOG_API_EXIT_MAC(funcId, rc);
    CPSS_APP_SPECIFIC_CB_MAC((funcId, rc, devNum, enablePtr));
    CPSS_API_UNLOCK_MAC(0,0);

    return rc;
}

/*******************************************************************************
* internal_cpssDxChTmGlueFlowControlEgressCounterSet
*
* DESCRIPTION:
*       Set value of the TM Egress Flow Control counter.
*
* APPLICABLE DEVICES:
*       Bobcat2.
*
* NOT APPLICABLE DEVICES:
*       DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2; Lion; Lion2; Lion3.
*
* INPUTS:
*       devNum          - device number.
*       tmPortInd       - TM port index.
*                         (APPLICABLE RANGES: 0..191).
*       value           - value of the TM egress flow control counter.
*                         (APPLICABLE RANGES: 0..1023).
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK                    - on success.
*       GT_BAD_PARAM             - on wrong device number, TM port index.
*       GT_OUT_OF_RANGE          - on wrong counter value.
*       GT_HW_ERROR              - on hardware error.
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device.
*
* COMMENTS:
*        None.
*
*******************************************************************************/
static GT_STATUS internal_cpssDxChTmGlueFlowControlEgressCounterSet
(
    IN GT_U8 devNum,
    IN GT_U32 tmPortInd,
    IN GT_U32 value
)
{
    GT_U32    regAddr;  /* register address */
    GT_U32    index;    /* word index */
    GT_U32    bit;      /* register bit */

    PRV_CPSS_DXCH_DEV_CHECK_MAC(devNum);
    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum,
          CPSS_CH1_E| CPSS_CH1_DIAMOND_E | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E | CPSS_XCAT3_E |
          CPSS_XCAT2_E | CPSS_LION_E | CPSS_LION2_E  | CPSS_BOBCAT3_E );

    PRV_CPSS_DXCH_PP_HW_INFO_E_ARCH_SUPPORTED_CHECK_MAC(devNum);
    PRV_CPSS_DXCH_TM_GLUE_FLOW_CONTROL_PORT_INDEX_CHECK_MAC(tmPortInd);

    if(value >= BIT_10)
    {
        return GT_OUT_OF_RANGE;
    }

    PRV_CPSS_DXCH_TM_GLUE_FLOW_CONTROL_REG_INDEX_BIT_OFSSET_MAC(tmPortInd, index, bit);

    regAddr = PRV_DXCH_REG1_UNIT_TM_FCU_MAC(devNum).TMEgrFlowCtrlCntrs[index];

    /* Set TM Egress Flow Control Counters - TM FC Queue Counter */
    return prvCpssHwPpSetRegField(devNum, regAddr, bit, 10, value);
}

/*******************************************************************************
* cpssDxChTmGlueFlowControlEgressCounterSet
*
* DESCRIPTION:
*       Set value of the TM Egress Flow Control counter.
*
* APPLICABLE DEVICES:
*       Bobcat2.
*
* NOT APPLICABLE DEVICES:
*       DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2; Lion; Lion2; Lion3.
*
* INPUTS:
*       devNum          - device number.
*       tmPortInd       - TM port index.
*                         (APPLICABLE RANGES: 0..191).
*       value           - value of the TM egress flow control counter.
*                         (APPLICABLE RANGES: 0..1023).
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK                    - on success.
*       GT_BAD_PARAM             - on wrong device number, TM port index.
*       GT_OUT_OF_RANGE          - on wrong counter value.
*       GT_HW_ERROR              - on hardware error.
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device.
*
* COMMENTS:
*        None.
*
*******************************************************************************/
GT_STATUS cpssDxChTmGlueFlowControlEgressCounterSet
(
    IN GT_U8 devNum,
    IN GT_U32 tmPortInd,
    IN GT_U32 value
)
{
    GT_STATUS rc;
    CPSS_LOG_FUNC_VARIABLE_DECLARE_MAC(funcId, cpssDxChTmGlueFlowControlEgressCounterSet);

    CPSS_API_LOCK_MAC(0,0);
    CPSS_LOG_API_ENTER_MAC((funcId, devNum, tmPortInd, value));

    rc = internal_cpssDxChTmGlueFlowControlEgressCounterSet(devNum, tmPortInd, value);

    CPSS_LOG_API_EXIT_MAC(funcId, rc);
    CPSS_APP_SPECIFIC_CB_MAC((funcId, rc, devNum, tmPortInd, value));
    CPSS_API_UNLOCK_MAC(0,0);

    return rc;
}

/*******************************************************************************
* internal_cpssDxChTmGlueFlowControlEgressCounterGet
*
* DESCRIPTION:
*       Get value of the TM Egress Flow Control counter.
*
* APPLICABLE DEVICES:
*       Bobcat2.
*
* NOT APPLICABLE DEVICES:
*       DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2; Lion; Lion2; Lion3.
*
* INPUTS:
*       devNum          - device number.
*       tmPortInd       - TM port index.
*                         (APPLICABLE RANGES: 0..191).
* OUTPUTS:
*       valuePtr        - (poiter to) value of the TM egress flow control counter.
*
* RETURNS:
*       GT_OK                    - on success.
*       GT_BAD_PARAM             - on wrong device number or TM port index.
*       GT_BAD_PTR               - on NULL pointer
*       GT_HW_ERROR              - on hardware error.
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device.
*
* COMMENTS:
*        None.
*
*******************************************************************************/
static GT_STATUS internal_cpssDxChTmGlueFlowControlEgressCounterGet
(
    IN GT_U8 devNum,
    IN GT_U32 tmPortInd,
    OUT GT_U32 *valuePtr
)
{
    GT_U32    regAddr;  /* register address */
    GT_U32    index;    /* word index */
    GT_U32    bit;      /* register bit */

    PRV_CPSS_DXCH_DEV_CHECK_MAC(devNum);
    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum,
          CPSS_CH1_E| CPSS_CH1_DIAMOND_E | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E | CPSS_XCAT3_E |
          CPSS_XCAT2_E | CPSS_LION_E | CPSS_LION2_E  | CPSS_BOBCAT3_E );

    PRV_CPSS_DXCH_PP_HW_INFO_E_ARCH_SUPPORTED_CHECK_MAC(devNum);
    PRV_CPSS_DXCH_TM_GLUE_FLOW_CONTROL_PORT_INDEX_CHECK_MAC(tmPortInd);
    CPSS_NULL_PTR_CHECK_MAC(valuePtr);

    PRV_CPSS_DXCH_TM_GLUE_FLOW_CONTROL_REG_INDEX_BIT_OFSSET_MAC(tmPortInd, index, bit);

    regAddr = PRV_DXCH_REG1_UNIT_TM_FCU_MAC(devNum).TMEgrFlowCtrlCntrs[index];

    /* Get TM Egress Flow Control Counters - TM FC Queue Counter */
    return prvCpssHwPpGetRegField(devNum, regAddr, bit, 10, valuePtr);
}

/*******************************************************************************
* cpssDxChTmGlueFlowControlEgressCounterGet
*
* DESCRIPTION:
*       Get value of the TM Egress Flow Control counter.
*
* APPLICABLE DEVICES:
*       Bobcat2.
*
* NOT APPLICABLE DEVICES:
*       DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2; Lion; Lion2; Lion3.
*
* INPUTS:
*       devNum          - device number.
*       tmPortInd       - TM port index.
*                         (APPLICABLE RANGES: 0..191).
* OUTPUTS:
*       valuePtr        - (poiter to) value of the TM egress flow control counter.
*
* RETURNS:
*       GT_OK                    - on success.
*       GT_BAD_PARAM             - on wrong device number or TM port index.
*       GT_BAD_PTR               - on NULL pointer
*       GT_HW_ERROR              - on hardware error.
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device.
*
* COMMENTS:
*        None.
*
*******************************************************************************/
GT_STATUS cpssDxChTmGlueFlowControlEgressCounterGet
(
    IN GT_U8 devNum,
    IN GT_U32 tmPortInd,
    OUT GT_U32 *valuePtr
)
{
    GT_STATUS rc;
    CPSS_LOG_FUNC_VARIABLE_DECLARE_MAC(funcId, cpssDxChTmGlueFlowControlEgressCounterGet);

    CPSS_API_LOCK_MAC(0,0);
    CPSS_LOG_API_ENTER_MAC((funcId, devNum, tmPortInd, valuePtr));

    rc = internal_cpssDxChTmGlueFlowControlEgressCounterGet(devNum, tmPortInd, valuePtr);

    CPSS_LOG_API_EXIT_MAC(funcId, rc);
    CPSS_APP_SPECIFIC_CB_MAC((funcId, rc, devNum, tmPortInd, valuePtr));
    CPSS_API_UNLOCK_MAC(0,0);

    return rc;
}

/*******************************************************************************
* internal_cpssDxChTmGlueFlowControlEgressThresholdsSet
*
* DESCRIPTION:
*       Set XON/XOFF TM Egress Flow Control thresholds values.
*
* APPLICABLE DEVICES:
*       Bobcat2.
*
* NOT APPLICABLE DEVICES:
*       DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2; Lion; Lion2; Lion3.
*
* INPUTS:
*       devNum          - device number.
*       tmPortInd       - TM port index.
*                         (APPLICABLE RANGES: 0..191).
*       xOffThreshold    - TM FC Queue XOFF threshold.
*                         (APPLICABLE RANGES: 0..1023).
*       xOnThreshold     - TM FC Queue XON threshold.
*                         (APPLICABLE RANGES: 0..1023).
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK                    - on success.
*       GT_BAD_PARAM             - on wrong device number, TM port index.
*       GT_HW_ERROR              - on hardware error.
*       GT_OUT_OF_RANGE          - on wrong XOFF/XON threshold.
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device.
*
* COMMENTS:
*        None.
*
*******************************************************************************/
static GT_STATUS internal_cpssDxChTmGlueFlowControlEgressThresholdsSet
(
    IN GT_U8 devNum,
    IN GT_U32 tmPortInd,
    IN GT_U32 xOffThreshold,
    IN GT_U32 xOnThreshold
)
{
    GT_U32    regAddr;  /* register address */
    GT_U32    index;    /* word index */
    GT_U32    bit;      /* register bit */
    GT_STATUS rc;       /* return code */

    PRV_CPSS_DXCH_DEV_CHECK_MAC(devNum);
    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum,
          CPSS_CH1_E| CPSS_CH1_DIAMOND_E | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E | CPSS_XCAT3_E |
          CPSS_XCAT2_E | CPSS_LION_E | CPSS_LION2_E  | CPSS_BOBCAT3_E );

    PRV_CPSS_DXCH_PP_HW_INFO_E_ARCH_SUPPORTED_CHECK_MAC(devNum);
    PRV_CPSS_DXCH_TM_GLUE_FLOW_CONTROL_PORT_INDEX_CHECK_MAC(tmPortInd);

    if(xOffThreshold >= BIT_10)
    {
        return GT_OUT_OF_RANGE;
    }

    if(xOnThreshold >= BIT_10)
    {
        return GT_OUT_OF_RANGE;
    }

    PRV_CPSS_DXCH_TM_GLUE_FLOW_CONTROL_REG_INDEX_BIT_OFSSET_MAC(tmPortInd, index, bit);

    regAddr = PRV_DXCH_REG1_UNIT_TM_FCU_MAC(devNum).TMEgrFlowCtrlXOFFThresholds[index];
    /* Set TM Egress Flow Control XOFF Thresholds - TM FC Queue XOFF */
    rc = prvCpssHwPpSetRegField(devNum, regAddr, bit, 10, xOffThreshold);
    if(rc != GT_OK)
    {
        return rc;
    }

    regAddr = PRV_DXCH_REG1_UNIT_TM_FCU_MAC(devNum).TMEgrFlowCtrlXONThresholds[index];
    /* Set TM Egress Flow Control XON Thresholds - TM FC Queue XON */
    return prvCpssHwPpSetRegField(devNum, regAddr, bit, 10, xOnThreshold);
}

/*******************************************************************************
* cpssDxChTmGlueFlowControlEgressThresholdsSet
*
* DESCRIPTION:
*       Set XON/XOFF TM Egress Flow Control thresholds values.
*
* APPLICABLE DEVICES:
*       Bobcat2.
*
* NOT APPLICABLE DEVICES:
*       DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2; Lion; Lion2; Lion3.
*
* INPUTS:
*       devNum          - device number.
*       tmPortInd       - TM port index.
*                         (APPLICABLE RANGES: 0..191).
*       xOffThreshold    - TM FC Queue XOFF threshold.
*                         (APPLICABLE RANGES: 0..1023).
*       xOnThreshold     - TM FC Queue XON threshold.
*                         (APPLICABLE RANGES: 0..1023).
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK                    - on success.
*       GT_BAD_PARAM             - on wrong device number, TM port index.
*       GT_HW_ERROR              - on hardware error.
*       GT_OUT_OF_RANGE          - on wrong XOFF/XON threshold.
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device.
*
* COMMENTS:
*        None.
*
*******************************************************************************/
GT_STATUS cpssDxChTmGlueFlowControlEgressThresholdsSet
(
    IN GT_U8 devNum,
    IN GT_U32 tmPortInd,
    IN GT_U32 xOffThreshold,
    IN GT_U32 xOnThreshold
)
{
    GT_STATUS rc;
    CPSS_LOG_FUNC_VARIABLE_DECLARE_MAC(funcId, cpssDxChTmGlueFlowControlEgressThresholdsSet);

    CPSS_API_LOCK_MAC(0,0);
    CPSS_LOG_API_ENTER_MAC((funcId, devNum, tmPortInd, xOffThreshold, xOnThreshold));

    rc = internal_cpssDxChTmGlueFlowControlEgressThresholdsSet(devNum, tmPortInd, xOffThreshold, xOnThreshold);

    CPSS_LOG_API_EXIT_MAC(funcId, rc);
    CPSS_APP_SPECIFIC_CB_MAC((funcId, rc, devNum, tmPortInd, xOffThreshold, xOnThreshold));
    CPSS_API_UNLOCK_MAC(0,0);

    return rc;
}

/*******************************************************************************
* internal_cpssDxChTmGlueFlowControlEgressThresholdsGet
*
* DESCRIPTION:
*       Get XON/XOFF TM Egress Flow Control thresholds values.
*
* APPLICABLE DEVICES:
*       Bobcat2.
*
* NOT APPLICABLE DEVICES:
*       DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2; Lion; Lion2; Lion3.
*
* INPUTS:
*       devNum          - device number.
*       tmPortInd       - TM port index.
*                         (APPLICABLE RANGES: 0..191).
* OUTPUTS:
*       xOffThresholdPtr - (pointer to) TM FC Queue XOFF threshold.
*       xOnThresholdPtr  - (pointer to) TM FC Queue XON threshold.
*
* RETURNS:
*       GT_OK                    - on success.
*       GT_BAD_PARAM             - on wrong device number or TM port index.
*       GT_BAD_PTR               - on NULL pointer
*       GT_HW_ERROR              - on hardware error.
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device.
*
* COMMENTS:
*        None.
*
*******************************************************************************/
static GT_STATUS internal_cpssDxChTmGlueFlowControlEgressThresholdsGet
(
    IN GT_U8 devNum,
    IN GT_U32 tmPortInd,
    OUT GT_U32 *xOffThresholdPtr,
    OUT GT_U32 *xOnThresholdPtr
)
{
    GT_U32    regAddr;  /* register address */
    GT_U32    index;    /* word index */
    GT_U32    bit;      /* register bit */
    GT_STATUS rc;       /* return code */

    PRV_CPSS_DXCH_DEV_CHECK_MAC(devNum);
    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum,
          CPSS_CH1_E| CPSS_CH1_DIAMOND_E | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E | CPSS_XCAT3_E |
          CPSS_XCAT2_E | CPSS_LION_E | CPSS_LION2_E  | CPSS_BOBCAT3_E );

    PRV_CPSS_DXCH_PP_HW_INFO_E_ARCH_SUPPORTED_CHECK_MAC(devNum);
    PRV_CPSS_DXCH_TM_GLUE_FLOW_CONTROL_PORT_INDEX_CHECK_MAC(tmPortInd);
    CPSS_NULL_PTR_CHECK_MAC(xOffThresholdPtr);
    CPSS_NULL_PTR_CHECK_MAC(xOnThresholdPtr);

    PRV_CPSS_DXCH_TM_GLUE_FLOW_CONTROL_REG_INDEX_BIT_OFSSET_MAC(tmPortInd, index, bit);

    regAddr = PRV_DXCH_REG1_UNIT_TM_FCU_MAC(devNum).TMEgrFlowCtrlXOFFThresholds[index];
    /* Get TM Egress Flow Control XOFF Thresholds - TM FC Queue XOFF */
    rc = prvCpssHwPpGetRegField(devNum, regAddr, bit, 10, xOffThresholdPtr);
    if(rc != GT_OK)
    {
        return rc;
    }

    regAddr = PRV_DXCH_REG1_UNIT_TM_FCU_MAC(devNum).TMEgrFlowCtrlXONThresholds[index];
    /* Get TM Egress Flow Control XON Thresholds - TM FC Queue XON */
    return prvCpssHwPpGetRegField(devNum, regAddr, bit, 10, xOnThresholdPtr);
}

/*******************************************************************************
* cpssDxChTmGlueFlowControlEgressThresholdsGet
*
* DESCRIPTION:
*       Get XON/XOFF TM Egress Flow Control thresholds values.
*
* APPLICABLE DEVICES:
*       Bobcat2.
*
* NOT APPLICABLE DEVICES:
*       DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2; Lion; Lion2; Lion3.
*
* INPUTS:
*       devNum          - device number.
*       tmPortInd       - TM port index.
*                         (APPLICABLE RANGES: 0..191).
* OUTPUTS:
*       xOffThresholdPtr - (pointer to) TM FC Queue XOFF threshold.
*       xOnThresholdPtr  - (pointer to) TM FC Queue XON threshold.
*
* RETURNS:
*       GT_OK                    - on success.
*       GT_BAD_PARAM             - on wrong device number or TM port index.
*       GT_BAD_PTR               - on NULL pointer
*       GT_HW_ERROR              - on hardware error.
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device.
*
* COMMENTS:
*        None.
*
*******************************************************************************/
GT_STATUS cpssDxChTmGlueFlowControlEgressThresholdsGet
(
    IN GT_U8 devNum,
    IN GT_U32 tmPortInd,
    OUT GT_U32 *xOffThresholdPtr,
    OUT GT_U32 *xOnThresholdPtr
)
{
    GT_STATUS rc;
    CPSS_LOG_FUNC_VARIABLE_DECLARE_MAC(funcId, cpssDxChTmGlueFlowControlEgressThresholdsGet);

    CPSS_API_LOCK_MAC(0,0);
    CPSS_LOG_API_ENTER_MAC((funcId, devNum, tmPortInd, xOffThresholdPtr, xOnThresholdPtr));

    rc = internal_cpssDxChTmGlueFlowControlEgressThresholdsGet(devNum, tmPortInd, xOffThresholdPtr, xOnThresholdPtr);

    CPSS_LOG_API_EXIT_MAC(funcId, rc);
    CPSS_APP_SPECIFIC_CB_MAC((funcId, rc, devNum, tmPortInd, xOffThresholdPtr, xOnThresholdPtr));
    CPSS_API_UNLOCK_MAC(0,0);

    return rc;
}

/*******************************************************************************
* prvCpssDxChTmGlueFlowControlPortSpeedSet
*
* DESCRIPTION:
*       Set port speed calibration value.
*
* APPLICABLE DEVICES:
*       Bobcat2.
*
* NOT APPLICABLE DEVICES:
*       DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2; Lion; Lion2; Lion3.
*
* INPUTS:
*       devNum      - device number.
*       portNum     - physical port number.
*       speed       - port speed
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK                    - on success.
*       GT_BAD_PARAM             - on wrong device number.
*       GT_HW_ERROR              - on hardware error.
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device.
*
* COMMENTS:
*       This table is the calibration value to be multiplied to the value in the PFC header
*           to be alligned to the number of cycles according to the port speed.
*
*******************************************************************************/
GT_STATUS prvCpssDxChTmGlueFlowControlPortSpeedSet
(
    IN GT_U8                    devNum,
    IN GT_PHYSICAL_PORT_NUM                portNum,
    IN CPSS_PORT_SPEED_ENT             speed
)
{
    GT_U32 value;           /* register value */
    GT_U32 speedForCalc;    /* port speed in Mbps */
    GT_FLOAT32 tmpValue;    /* temporary value */
        GT_STATUS rc = GT_OK;
        GT_PHYSICAL_PORT_NUM pfcPortNum;

    PRV_CPSS_DXCH_DEV_CHECK_MAC(devNum);
    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum,
          CPSS_CH1_E| CPSS_CH1_DIAMOND_E | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E | CPSS_XCAT3_E |
          CPSS_XCAT2_E | CPSS_LION_E | CPSS_LION2_E  | CPSS_BOBCAT3_E );

        /* PFC with TM supported only for first 63 physical ports for BC2 version < B0 */
        rc = prvCpssDxChTmGluePfcPortMappingGet(devNum, portNum, &pfcPortNum);

        if (rc != GT_OK)
        {
                return rc;
        }

    switch(speed)
    {
        case CPSS_PORT_SPEED_10_E:
            speedForCalc = 10;
            break;
        case CPSS_PORT_SPEED_100_E:
            speedForCalc = 100;
            break;
        case CPSS_PORT_SPEED_1000_E:
            speedForCalc = 1000;
            break;
        case CPSS_PORT_SPEED_2500_E:
            speedForCalc = 2500;
            break;
        case CPSS_PORT_SPEED_5000_E:
            speedForCalc = 5000;
            break;
        case CPSS_PORT_SPEED_10000_E:
            speedForCalc = 10000;
            break;
        case CPSS_PORT_SPEED_12000_E:
            speedForCalc = 12000;
            break;
        case CPSS_PORT_SPEED_20000_E:
            speedForCalc = 20000;
            break;
        case CPSS_PORT_SPEED_40000_E:
            speedForCalc = 40000;
            break;
        default:
            return GT_BAD_PARAM;
    }

    /* Calculate port speed calibration value.
       value is number of PP core clocks needed to pause TX for 512 bits of
           line rate.
       Jira TMIP-187:
        ReqVal = CORE_CLK*1000000*PFC_UNITS/(PortSpeed*UPDATE_RATE)
        ConfigVal = Round(ReqVal * 2^15)
        */
    value = (1 << 15);
    tmpValue = (GT_FLOAT32)(512 * PRV_CPSS_PP_MAC(devNum)->coreClock) / (320 * speedForCalc);

    value = (GT_U32)((tmpValue * value) + 0.5);

    /* Set Port Speed Entry - Port Speed */
    return prvCpssDxChWriteTableEntryField(devNum,
                 PRV_CPSS_DXCH_LION3_TABLE_TM_FCU_PORT_INGRESS_TIMERS_CONFIG_E,
                 pfcPortNum,
                 PRV_CPSS_DXCH_TABLE_WORD_INDICATE_GLOBAL_BIT_CNS,
                 0, 32, value);
}

/*******************************************************************************
* internal_cpssDxChTmGlueFlowControlPortSpeedSet
*
* DESCRIPTION:
*       Set PFC calibration value by port speed.
*
* APPLICABLE DEVICES:
*       Bobcat2.
*
* NOT APPLICABLE DEVICES:
*       DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2; Lion; Lion2; Lion3.
*
* INPUTS:
*       devNum      - device number.
*       portNum     - physical port number.
*       speed       - port speed
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK                    - on success.
*       GT_BAD_PARAM             - on wrong device number.
*       GT_HW_ERROR              - on hardware error.
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device.
*
* COMMENTS:
*       This table is the calibration value to be multiplied to the value in the PFC header
*       to be alligned to the number of cycles according to the port speed.
*
*******************************************************************************/
static GT_STATUS internal_cpssDxChTmGlueFlowControlPortSpeedSet
(
    IN GT_U8                    devNum,
    IN GT_PHYSICAL_PORT_NUM                portNum,
    IN CPSS_PORT_SPEED_ENT             speed
)
{
        GT_STATUS rc;

        rc = prvCpssDxChTmGlueFlowControlPortSpeedSet(devNum, portNum, speed);

        return rc;
}

/*******************************************************************************
* cpssDxChTmGlueFlowControlPortSpeedSet
*
* DESCRIPTION:
*       Set PFC calibration value by port speed.
*
* APPLICABLE DEVICES:
*       Bobcat2.
*
* NOT APPLICABLE DEVICES:
*       DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2; Lion; Lion2; Lion3.
*
* INPUTS:
*       devNum      - device number.
*       portNum     - physical port number.
*       speed       - port speed
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK                    - on success.
*       GT_BAD_PARAM             - on wrong device number.
*       GT_HW_ERROR              - on hardware error.
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device.
*
* COMMENTS:
*           This API should be called on changing of TM Enabled        physical port speed
*           mapped to the PFC port, as a result of:
*                   1) port speed has changed. (cpssDxChPortModeSpeedSet)
*                   2) PFC port mapping has changed (cpssDxChTmGluePfcPortMappingSet).
*
*       This table is the calibration value to be multiplied to the value in the PFC header
*       to be alligned to the number of cycles according to the port speed.
*
*******************************************************************************/
GT_STATUS cpssDxChTmGlueFlowControlPortSpeedSet
(
    IN GT_U8                    devNum,
    IN GT_PHYSICAL_PORT_NUM                portNum,
    IN CPSS_PORT_SPEED_ENT             speed
)
{
        GT_STATUS rc;
        CPSS_LOG_FUNC_VARIABLE_DECLARE_MAC(funcId, cpssDxChTmGlueFlowControlPortSpeedSet);

        CPSS_API_LOCK_MAC(0,0);
        CPSS_LOG_API_ENTER_MAC((funcId, devNum, portNum, speed));

        rc = internal_cpssDxChTmGlueFlowControlPortSpeedSet(devNum, portNum, speed);

        CPSS_LOG_API_EXIT_MAC(funcId, rc);
        CPSS_APP_SPECIFIC_CB_MAC((funcId, rc, devNum, portNum, speed));
        CPSS_API_UNLOCK_MAC(0,0);

        return rc;
}


